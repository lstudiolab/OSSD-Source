/*
 * Copyright (c) 1999-2020 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 *
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
  
                   
                                                
                               
  
                                             
  
                                                                    
  
   

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/lock.h>
#include <sys/mman.h>
#include <sys/mount_internal.h>
#include <sys/vnode_internal.h>
#include <sys/ubc_internal.h>
#include <sys/ucred.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/buf.h>
#include <sys/user.h>
#include <sys/codesign.h>
#include <sys/codedir_internal.h>
#include <sys/fsevents.h>
#include <sys/fcntl.h>
#include <sys/reboot.h>

#include <mach/mach_types.h>
#include <mach/memory_object_types.h>
#include <mach/memory_object_control.h>
#include <mach/vm_map.h>
#include <mach/mach_vm.h>
#include <mach/upl.h>

#include <kern/kern_types.h>
#include <kern/kalloc.h>
#include <kern/zalloc.h>
#include <kern/thread.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <vm/vm_protos.h>           

#include <libkern/crypto/sha1.h>
#include <libkern/crypto/sha2.h>
#include <libkern/libkern.h>
#include <libkern/ptrauth_utils.h>

#include <security/mac_framework.h>
#include <stdbool.h>
#include <stdatomic.h>

                                                                      
extern kern_return_t memory_object_pages_resident(memory_object_control_t,
    boolean_t *);
extern kern_return_t    memory_object_signed(memory_object_control_t control,
    boolean_t is_signed);
extern boolean_t        memory_object_is_signed(memory_object_control_t);
extern void             memory_object_mark_trusted(
	memory_object_control_t         control);

                         

extern void Debugger(const char *message);


                                     
kern_return_t ubc_page_op_with_control(
	memory_object_control_t  control,
	off_t                    f_offset,
	int                      ops,
	ppnum_t                  *phys_entryp,
	int                      *flagsp);


#if DIAGNOSTIC
#if defined(assert)
#undef assert
#endif
#define assert(cond)    \
    ((void) ((cond) ? 0 : panic("Assert failed: %s", # cond)))
#else
#include <kern/assert.h>
#endif                 

static int ubc_info_init_internal(struct vnode *vp, int withfsize, off_t filesize);
static int ubc_umcallback(vnode_t, void *);
static int ubc_msync_internal(vnode_t, off_t, off_t, off_t *, int, int *);
static void ubc_cs_free(struct ubc_info *uip);

static boolean_t ubc_cs_supports_multilevel_hash(struct cs_blob *blob);
static kern_return_t ubc_cs_convert_to_multilevel_hash(struct cs_blob *blob);

ZONE_DECLARE(ubc_info_zone, "ubc_info zone", sizeof(struct ubc_info),
    ZC_NOENCRYPT | ZC_ZFREE_CLEARMEM);
static uint32_t cs_blob_generation_count = 1;

  
              
                                                                     
   

extern int cs_debug;

#define PAGE_SHIFT_4K           (12)

static boolean_t
cs_valid_range(
	const void *start,
	const void *end,
	const void *lower_bound,
	const void *upper_bound)
{
	if (upper_bound < lower_bound ||
	    end < start) {
		return FALSE;
	}

	if (start < lower_bound ||
	    end > upper_bound) {
		return FALSE;
	}

	return TRUE;
}

typedef void (*cs_md_init)(void *ctx);
typedef void (*cs_md_update)(void *ctx, const void *data, size_t size);
typedef void (*cs_md_final)(void *hash, void *ctx);

struct cs_hash {
	uint8_t             cs_type;                                       
	size_t              cs_size;                                                   
	size_t              cs_digest_size;                         
	cs_md_init          cs_init;
	cs_md_update        cs_update;
	cs_md_final         cs_final;
};

uint8_t
cs_hash_type(
	struct cs_hash const * const cs_hash)
{
	return cs_hash->cs_type;
}

static const struct cs_hash cs_hash_sha1 = {
	.cs_type = CS_HASHTYPE_SHA1,
	.cs_size = CS_SHA1_LEN,
	.cs_digest_size = SHA_DIGEST_LENGTH,
	.cs_init = (cs_md_init)SHA1Init,
	.cs_update = (cs_md_update)SHA1Update,
	.cs_final = (cs_md_final)SHA1Final,
};
#if CRYPTO_SHA2
static const struct cs_hash cs_hash_sha256 = {
	.cs_type = CS_HASHTYPE_SHA256,
	.cs_size = SHA256_DIGEST_LENGTH,
	.cs_digest_size = SHA256_DIGEST_LENGTH,
	.cs_init = (cs_md_init)SHA256_Init,
	.cs_update = (cs_md_update)SHA256_Update,
	.cs_final = (cs_md_final)SHA256_Final,
};
static const struct cs_hash cs_hash_sha256_truncate = {
	.cs_type = CS_HASHTYPE_SHA256_TRUNCATED,
	.cs_size = CS_SHA256_TRUNCATED_LEN,
	.cs_digest_size = SHA256_DIGEST_LENGTH,
	.cs_init = (cs_md_init)SHA256_Init,
	.cs_update = (cs_md_update)SHA256_Update,
	.cs_final = (cs_md_final)SHA256_Final,
};
static const struct cs_hash cs_hash_sha384 = {
	.cs_type = CS_HASHTYPE_SHA384,
	.cs_size = SHA384_DIGEST_LENGTH,
	.cs_digest_size = SHA384_DIGEST_LENGTH,
	.cs_init = (cs_md_init)SHA384_Init,
	.cs_update = (cs_md_update)SHA384_Update,
	.cs_final = (cs_md_final)SHA384_Final,
};
#endif

static struct cs_hash const *
cs_find_md(uint8_t type)
{
	if (type == CS_HASHTYPE_SHA1) {
		return &cs_hash_sha1;
#if CRYPTO_SHA2
	} else if (type == CS_HASHTYPE_SHA256) {
		return &cs_hash_sha256;
	} else if (type == CS_HASHTYPE_SHA256_TRUNCATED) {
		return &cs_hash_sha256_truncate;
	} else if (type == CS_HASHTYPE_SHA384) {
		return &cs_hash_sha384;
#endif
	}
	return NULL;
}

union cs_hash_union {
	SHA1_CTX                sha1ctxt;
	SHA256_CTX              sha256ctx;
	SHA384_CTX              sha384ctx;
};


  
                                          
                                           
   
static const uint32_t hashPriorities[] = {
	CS_HASHTYPE_SHA1,
	CS_HASHTYPE_SHA256_TRUNCATED,
	CS_HASHTYPE_SHA256,
	CS_HASHTYPE_SHA384,
};

static unsigned int
hash_rank(const CS_CodeDirectory *cd)
{
	uint32_t type = cd->hashType;
	unsigned int n;

	for (n = 0; n < sizeof(hashPriorities) / sizeof(hashPriorities[0]); ++n) {
		if (hashPriorities[n] == type) {
			return n + 1;
		}
	}
	return 0;                          
}


  
                       
   
static const unsigned char *
hashes(
	const CS_CodeDirectory *cd,
	uint32_t page,
	size_t hash_len,
	const char *lower_bound,
	const char *upper_bound)
{
	const unsigned char *base, *top, *hash;
	uint32_t nCodeSlots = ntohl(cd->nCodeSlots);

	assert(cs_valid_range(cd, cd + 1, lower_bound, upper_bound));

	if ((ntohl(cd->version) >= CS_SUPPORTSSCATTER) && (ntohl(cd->scatterOffset))) {
		                              
		const SC_Scatter *scatter = (const SC_Scatter*)
		    ((const char*)cd + ntohl(cd->scatterOffset));
		uint32_t hashindex = 0, scount, sbase = 0;
		                                 
		do {
			if ((const char*)scatter > (const char*)cd + ntohl(cd->length)) {
				if (cs_debug) {
					printf("CODE SIGNING: Scatter extends past Code Directory\n");
				}
				return NULL;
			}

			scount = ntohl(scatter->count);
			uint32_t new_base = ntohl(scatter->base);

			                   
			if (scount == 0) {
				return NULL;
			}

			if ((hashindex > 0) && (new_base <= sbase)) {
				if (cs_debug) {
					printf("CODE SIGNING: unordered Scatter, prev base %d, cur base %d\n",
					    sbase, new_base);
				}
				return NULL;                                 
			}
			sbase = new_base;

			                                                 
			if (sbase > page) {
				return NULL;
			}

			if (sbase + scount >= page) {
				                                   
                              

				                                                     
				base = (const unsigned char *)cd + ntohl(cd->hashOffset) +
				    hashindex * hash_len;
				                                                    
				top = base + scount * hash_len;
				if (!cs_valid_range(base, top, lower_bound,
				    upper_bound) ||
				    hashindex > nCodeSlots) {
					return NULL;
				}

				break;
			}

			                                                       
                      
			hashindex += scount;
			scatter++;
		} while (1);

		hash = base + (page - sbase) * hash_len;
	} else {
		base = (const unsigned char *)cd + ntohl(cd->hashOffset);
		top = base + nCodeSlots * hash_len;
		if (!cs_valid_range(base, top, lower_bound, upper_bound) ||
		    page > nCodeSlots) {
			return NULL;
		}
		assert(page < nCodeSlots);

		hash = base + page * hash_len;
	}

	if (!cs_valid_range(hash, hash + hash_len,
	    lower_bound, upper_bound)) {
		hash = NULL;
	}

	return hash;
}

  
                            
  
                                                                     
                                                             
  
                                                    
                             
  
                       
                                    
   

static int
cs_validate_codedirectory(const CS_CodeDirectory *cd, size_t length)
{
	struct cs_hash const *hashtype;

	if (length < sizeof(*cd)) {
		return EBADEXEC;
	}
	if (ntohl(cd->magic) != CSMAGIC_CODEDIRECTORY) {
		return EBADEXEC;
	}
	if (cd->pageSize < PAGE_SHIFT_4K || cd->pageSize > PAGE_SHIFT) {
		return EBADEXEC;
	}
	hashtype = cs_find_md(cd->hashType);
	if (hashtype == NULL) {
		return EBADEXEC;
	}

	if (cd->hashSize != hashtype->cs_size) {
		return EBADEXEC;
	}

	if (length < ntohl(cd->hashOffset)) {
		return EBADEXEC;
	}

	                                                                        
	if (ntohl(cd->hashOffset) / hashtype->cs_size < ntohl(cd->nSpecialSlots)) {
		return EBADEXEC;
	}

	                                             
	if ((length - ntohl(cd->hashOffset)) / hashtype->cs_size < ntohl(cd->nCodeSlots)) {
		return EBADEXEC;
	}

	if (ntohl(cd->version) >= CS_SUPPORTSSCATTER && cd->scatterOffset) {
		if (length < ntohl(cd->scatterOffset)) {
			return EBADEXEC;
		}

		const SC_Scatter *scatter = (const SC_Scatter *)
		    (((const uint8_t *)cd) + ntohl(cd->scatterOffset));
		uint32_t nPages = 0;

		  
                                                       
                                                   
                      
     
		while (1) {
			                                                                    
			if (((const uint8_t *)scatter) + sizeof(scatter[0]) > (const uint8_t *)cd + length) {
				return EBADEXEC;
			}
			uint32_t scount = ntohl(scatter->count);
			if (scount == 0) {
				break;
			}
			if (nPages + scount < nPages) {
				return EBADEXEC;
			}
			nPages += scount;
			scatter++;

			                                           
			                                                 
		}
#if 0                      
		if (nPages != ntohl(cd->nCodeSlots)) {
			return EBADEXEC;
		}
#endif
	}

	if (length < ntohl(cd->identOffset)) {
		return EBADEXEC;
	}

	                                         
	if (cd->identOffset) {
		const uint8_t *ptr = (const uint8_t *)cd + ntohl(cd->identOffset);
		if (memchr(ptr, 0, length - ntohl(cd->identOffset)) == NULL) {
			return EBADEXEC;
		}
	}

	                                               
	if (ntohl(cd->version) >= CS_SUPPORTSTEAMID && ntohl(cd->teamOffset)) {
		if (length < ntohl(cd->teamOffset)) {
			return EBADEXEC;
		}

		const uint8_t *ptr = (const uint8_t *)cd + ntohl(cd->teamOffset);
		if (memchr(ptr, 0, length - ntohl(cd->teamOffset)) == NULL) {
			return EBADEXEC;
		}
	}

	                                            
	if (ntohl(cd->version) >= CS_SUPPORTSLINKAGE && cd->linkageHashType != 0) {
		const uintptr_t ptr = (uintptr_t)cd + ntohl(cd->linkageOffset);
		const uintptr_t ptr_end = ptr + ntohl(cd->linkageSize);

		if (ptr_end < ptr || ptr < (uintptr_t)cd || ptr_end > (uintptr_t)cd + length) {
			return EBADEXEC;
		}
	}


	return 0;
}

  
  
   

static int
cs_validate_blob(const CS_GenericBlob *blob, size_t length)
{
	if (length < sizeof(CS_GenericBlob) || length < ntohl(blob->length)) {
		return EBADEXEC;
	}
	return 0;
}

  
                     
  
                                                                    
                                   
  
                                                                    
  
  
                                                 
                             
                                           
  
                       
                                    
   

static int
cs_validate_csblob(
	const uint8_t *addr,
	const size_t blob_size,
	const CS_CodeDirectory **rcd,
	const CS_GenericBlob **rentitlements)
{
	const CS_GenericBlob *blob;
	int error;
	size_t length;

	*rcd = NULL;
	*rentitlements = NULL;

	blob = (const CS_GenericBlob *)(const void *)addr;

	length = blob_size;
	error = cs_validate_blob(blob, length);
	if (error) {
		return error;
	}
	length = ntohl(blob->length);

	if (ntohl(blob->magic) == CSMAGIC_EMBEDDED_SIGNATURE) {
		const CS_SuperBlob *sb;
		uint32_t n, count;
		const CS_CodeDirectory *best_cd = NULL;
		unsigned int best_rank = 0;
#if PLATFORM_WatchOS
		const CS_CodeDirectory *sha1_cd = NULL;
#endif

		if (length < sizeof(CS_SuperBlob)) {
			return EBADEXEC;
		}

		sb = (const CS_SuperBlob *)blob;
		count = ntohl(sb->count);

		                                                                    
		if ((length - sizeof(CS_SuperBlob)) / sizeof(CS_BlobIndex) < count) {
			return EBADEXEC;
		}

		                              
		for (n = 0; n < count; n++) {
			const CS_BlobIndex *blobIndex = &sb->index[n];
			uint32_t type = ntohl(blobIndex->type);
			uint32_t offset = ntohl(blobIndex->offset);
			if (length < offset) {
				return EBADEXEC;
			}

			const CS_GenericBlob *subBlob =
			    (const CS_GenericBlob *)(const void *)(addr + offset);

			size_t subLength = length - offset;

			if ((error = cs_validate_blob(subBlob, subLength)) != 0) {
				return error;
			}
			subLength = ntohl(subBlob->length);

			                                                     
			if (type == CSSLOT_CODEDIRECTORY || (type >= CSSLOT_ALTERNATE_CODEDIRECTORIES && type < CSSLOT_ALTERNATE_CODEDIRECTORY_LIMIT)) {
				const CS_CodeDirectory *candidate = (const CS_CodeDirectory *)subBlob;
				if ((error = cs_validate_codedirectory(candidate, subLength)) != 0) {
					return error;
				}
				unsigned int rank = hash_rank(candidate);
				if (cs_debug > 3) {
					printf("CodeDirectory type %d rank %d at slot 0x%x index %d\n", candidate->hashType, (int)rank, (int)type, (int)n);
				}
				if (best_cd == NULL || rank > best_rank) {
					best_cd = candidate;
					best_rank = rank;

					if (cs_debug > 2) {
						printf("using CodeDirectory type %d (rank %d)\n", (int)best_cd->hashType, best_rank);
					}
					*rcd = best_cd;
				} else if (best_cd != NULL && rank == best_rank) {
					                                                                         
					printf("multiple hash=%d CodeDirectories in signature; rejecting\n", best_cd->hashType);
					return EBADEXEC;
				}
#if PLATFORM_WatchOS
				if (candidate->hashType == CS_HASHTYPE_SHA1) {
					if (sha1_cd != NULL) {
						printf("multiple sha1 CodeDirectories in signature; rejecting\n");
						return EBADEXEC;
					}
					sha1_cd = candidate;
				}
#endif
			} else if (type == CSSLOT_ENTITLEMENTS) {
				if (ntohl(subBlob->magic) != CSMAGIC_EMBEDDED_ENTITLEMENTS) {
					return EBADEXEC;
				}
				if (*rentitlements != NULL) {
					printf("multiple entitlements blobs\n");
					return EBADEXEC;
				}
				*rentitlements = subBlob;
			}
		}

#if PLATFORM_WatchOS
		                                                             
               
    
                                                                
                                                       
                                                            
                                                           
                                                             
                                                               
                                                              
                                                                
                                                               
    
                                                              
                                                                
                       
     

		if (*rcd != NULL && sha1_cd != NULL && (ntohl(sha1_cd->flags) & CS_ADHOC)) {
			if (sha1_cd->flags != (*rcd)->flags) {
				printf("mismatched flags between hash %d (flags: %#x) and sha1 (flags: %#x) cd.\n",
				    (int)(*rcd)->hashType, (*rcd)->flags, sha1_cd->flags);
				*rcd = NULL;
				return EBADEXEC;
			}

			*rcd = sha1_cd;
		}
#endif
	} else if (ntohl(blob->magic) == CSMAGIC_CODEDIRECTORY) {
		if ((error = cs_validate_codedirectory((const CS_CodeDirectory *)(const void *)addr, length)) != 0) {
			return error;
		}
		*rcd = (const CS_CodeDirectory *)blob;
	} else {
		return EBADEXEC;
	}

	if (*rcd == NULL) {
		return EBADEXEC;
	}

	return 0;
}

  
                     
  
                                                                     
                                                             
                                        
  
                                                                 
                           
  
                                                 
                             
                               
                                          
  
                             
                           
   

const CS_GenericBlob *
csblob_find_blob_bytes(const uint8_t *addr, size_t length, uint32_t type, uint32_t magic)
{
	const CS_GenericBlob *blob = (const CS_GenericBlob *)(const void *)addr;

	if (ntohl(blob->magic) == CSMAGIC_EMBEDDED_SIGNATURE) {
		const CS_SuperBlob *sb = (const CS_SuperBlob *)blob;
		size_t n, count = ntohl(sb->count);

		for (n = 0; n < count; n++) {
			if (ntohl(sb->index[n].type) != type) {
				continue;
			}
			uint32_t offset = ntohl(sb->index[n].offset);
			if (length - sizeof(const CS_GenericBlob) < offset) {
				return NULL;
			}
			blob = (const CS_GenericBlob *)(const void *)(addr + offset);
			if (ntohl(blob->magic) != magic) {
				continue;
			}
			return blob;
		}
	} else if (type == CSSLOT_CODEDIRECTORY
	    && ntohl(blob->magic) == CSMAGIC_CODEDIRECTORY
	    && magic == CSMAGIC_CODEDIRECTORY) {
		return blob;
	}
	return NULL;
}


const CS_GenericBlob *
csblob_find_blob(struct cs_blob *csblob, uint32_t type, uint32_t magic)
{
	if ((csblob->csb_flags & CS_VALID) == 0) {
		return NULL;
	}
	return csblob_find_blob_bytes((const uint8_t *)csblob->csb_mem_kaddr, csblob->csb_mem_size, type, magic);
}

static const uint8_t *
find_special_slot(const CS_CodeDirectory *cd, size_t slotsize, uint32_t slot)
{
	                                                                     
	if (ntohl(cd->nSpecialSlots) < slot || slot == 0) {
		return NULL;
	}

	return (const uint8_t *)cd + ntohl(cd->hashOffset) - (slotsize * slot);
}

static uint8_t cshash_zero[CS_HASH_MAX_SIZE] = { 0 };

int
csblob_get_entitlements(struct cs_blob *csblob, void **out_start, size_t *out_length)
{
	uint8_t computed_hash[CS_HASH_MAX_SIZE];
	const CS_GenericBlob *entitlements;
	const CS_CodeDirectory *code_dir;
	const uint8_t *embedded_hash;
	union cs_hash_union context;

	*out_start = NULL;
	*out_length = 0;

	if (csblob->csb_hashtype == NULL || csblob->csb_hashtype->cs_digest_size > sizeof(computed_hash)) {
		return EBADEXEC;
	}

	code_dir = csblob->csb_cd;

	if ((csblob->csb_flags & CS_VALID) == 0) {
		entitlements = NULL;
	} else {
		entitlements = csblob->csb_entitlements_blob;
	}
	embedded_hash = find_special_slot(code_dir, csblob->csb_hashtype->cs_size, CSSLOT_ENTITLEMENTS);

	if (embedded_hash == NULL) {
		if (entitlements) {
			return EBADEXEC;
		}
		return 0;
	} else if (entitlements == NULL) {
		if (memcmp(embedded_hash, cshash_zero, csblob->csb_hashtype->cs_size) != 0) {
			return EBADEXEC;
		} else {
			return 0;
		}
	}

	csblob->csb_hashtype->cs_init(&context);
	ptrauth_utils_auth_blob_generic(entitlements,
	    ntohl(entitlements->length),
	    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_entitlements_blob_signature"),
	    PTRAUTH_ADDR_DIVERSIFY,
	    csblob->csb_entitlements_blob_signature);
	csblob->csb_hashtype->cs_update(&context, entitlements, ntohl(entitlements->length));
	csblob->csb_hashtype->cs_final(computed_hash, &context);

	if (memcmp(computed_hash, embedded_hash, csblob->csb_hashtype->cs_size) != 0) {
		return EBADEXEC;
	}

	*out_start = __DECONST(void *, entitlements);
	*out_length = ntohl(entitlements->length);

	return 0;
}

  
              
                                                                          
   



  
                
  
                                                             
  
                                        
  
                       
                                      
                                                  
  
   
int
ubc_info_init(struct vnode *vp)
{
	return ubc_info_init_internal(vp, 0, 0);
}


  
                         
  
                                                            
  
                                        
                                  
  
                       
                                      
                                                  
   
int
ubc_info_init_withsize(struct vnode *vp, off_t filesize)
{
	return ubc_info_init_internal(vp, 1, filesize);
}


  
                         
  
                                                      
  
                                        
                                                       
                                              
                                                     
  
                       
                                      
                                                  
  
                                                                     
                                                                  
                                                                
                                                                
                   
  
                                                                
                                                                  
                                                                  
  
                                                                   
                                                              
                         
   
static int
ubc_info_init_internal(vnode_t vp, int withfsize, off_t filesize)
{
	struct ubc_info *uip;
	void *  pager;
	int error = 0;
	kern_return_t kret;
	memory_object_control_t control;

	uip = vp->v_ubcinfo;

	  
                                                                
                                                              
    
	if (uip == UBC_INFO_NULL) {
		uip = (struct ubc_info *) zalloc(ubc_info_zone);
		bzero((char *)uip, sizeof(struct ubc_info));

		uip->ui_vnode = vp;
		uip->ui_flags = UI_INITED;
		uip->ui_ucred = NOCRED;
	}
	assert(uip->ui_flags != UI_NONE);
	assert(uip->ui_vnode == vp);

	                                        
	vp->v_ubcinfo = uip;

	  
                                          
   
                                                              
                                                              
                                                   
    
	pager = (void *)vnode_pager_setup(vp, uip->ui_pager);
	assert(pager);

	  
                                                                 
                     
    
	SET(uip->ui_flags, UI_HASPAGER);
	uip->ui_pager = pager;

	  
                                                       
                                                          
                                                        
                                                   
                      
    

	  
                                          
                                                                   
                                                                
                                                                       
                                                              
                                      
    
	kret = memory_object_create_named(pager,
	    (memory_object_size_t)uip->ui_size, &control);
	vnode_pager_deallocate(pager);
	if (kret != KERN_SUCCESS) {
		panic("ubc_info_init: memory_object_create_named returned %d", kret);
	}

	assert(control);
	uip->ui_control = control;                                             
	SET(uip->ui_flags, UI_HASOBJREF);                                   

	if (withfsize == 0) {
		                         
		error = vnode_size(vp, &uip->ui_size, vfs_context_current());
		if (error) {
			uip->ui_size = 0;
		}
	} else {
		uip->ui_size = filesize;
	}
	vp->v_lflag |= VNAMED_UBC;                                           

	return error;
}


  
                
  
                            
  
                                                      
  
                  
  
                                                                        
                                                                
                                 
  
                                                                  
                                     
   
static void
ubc_info_free(struct ubc_info *uip)
{
	if (IS_VALID_CRED(uip->ui_ucred)) {
		kauth_cred_unref(&uip->ui_ucred);
	}

	if (uip->ui_control != MEMORY_OBJECT_CONTROL_NULL) {
		memory_object_control_deallocate(uip->ui_control);
	}

	cluster_release(uip);
	ubc_cs_free(uip);

	zfree(ubc_info_zone, uip);
	return;
}


void
ubc_info_deallocate(struct ubc_info *uip)
{
	ubc_info_free(uip);
}

errno_t
mach_to_bsd_errno(kern_return_t mach_err)
{
	switch (mach_err) {
	case KERN_SUCCESS:
		return 0;

	case KERN_INVALID_ADDRESS:
	case KERN_INVALID_ARGUMENT:
	case KERN_NOT_IN_SET:
	case KERN_INVALID_NAME:
	case KERN_INVALID_TASK:
	case KERN_INVALID_RIGHT:
	case KERN_INVALID_VALUE:
	case KERN_INVALID_CAPABILITY:
	case KERN_INVALID_HOST:
	case KERN_MEMORY_PRESENT:
	case KERN_INVALID_PROCESSOR_SET:
	case KERN_INVALID_POLICY:
	case KERN_ALREADY_WAITING:
	case KERN_DEFAULT_SET:
	case KERN_EXCEPTION_PROTECTED:
	case KERN_INVALID_LEDGER:
	case KERN_INVALID_MEMORY_CONTROL:
	case KERN_INVALID_SECURITY:
	case KERN_NOT_DEPRESSED:
	case KERN_LOCK_OWNED:
	case KERN_LOCK_OWNED_SELF:
		return EINVAL;

	case KERN_PROTECTION_FAILURE:
	case KERN_NOT_RECEIVER:
	case KERN_NO_ACCESS:
	case KERN_POLICY_STATIC:
		return EACCES;

	case KERN_NO_SPACE:
	case KERN_RESOURCE_SHORTAGE:
	case KERN_UREFS_OVERFLOW:
	case KERN_INVALID_OBJECT:
		return ENOMEM;

	case KERN_FAILURE:
		return EIO;

	case KERN_MEMORY_FAILURE:
	case KERN_POLICY_LIMIT:
	case KERN_CODESIGN_ERROR:
		return EPERM;

	case KERN_MEMORY_ERROR:
		return EBUSY;

	case KERN_ALREADY_IN_SET:
	case KERN_NAME_EXISTS:
	case KERN_RIGHT_EXISTS:
		return EEXIST;

	case KERN_ABORTED:
		return EINTR;

	case KERN_TERMINATED:
	case KERN_LOCK_SET_DESTROYED:
	case KERN_LOCK_UNSTABLE:
	case KERN_SEMAPHORE_DESTROYED:
		return ENOENT;

	case KERN_RPC_SERVER_TERMINATED:
		return ECONNRESET;

	case KERN_NOT_SUPPORTED:
		return ENOTSUP;

	case KERN_NODE_DOWN:
		return ENETDOWN;

	case KERN_NOT_WAITING:
		return ENOENT;

	case KERN_OPERATION_TIMED_OUT:
		return ETIMEDOUT;

	default:
		return EIO;
	}
}

  
                 
  
                                                                         
          
  
                                                      
                       
                                             
                    
  
                                   
                                 
                                                                                     
                                                                     
  
                                                                      
                                                           
                                                                 
                                                
  
                                                                
                                                             
                                                                 
                                                         
                                                              
   
errno_t
ubc_setsize_ex(struct vnode *vp, off_t nsize, ubc_setsize_opts_t opts)
{
	off_t osize;                               
	off_t lastpg, olastpgend, lastoff;
	struct ubc_info *uip;
	memory_object_control_t control;
	kern_return_t kret = KERN_SUCCESS;

	if (nsize < (off_t)0) {
		return EINVAL;
	}

	if (!UBCINFOEXISTS(vp)) {
		return ENOENT;
	}

	uip = vp->v_ubcinfo;
	osize = uip->ui_size;

	if (ISSET(opts, UBC_SETSIZE_NO_FS_REENTRY) && nsize < osize) {
		return EAGAIN;
	}

	  
                                          
    
	uip->ui_size = nsize;

	if (nsize >= osize) {                           
		if (nsize > osize) {
			lock_vnode_and_post(vp, NOTE_EXTEND);
		}

		return 0;
	}

	  
                                                          
                                                         
                                                        
                                                       
    
	lastpg = trunc_page_64(nsize);
	olastpgend = round_page_64(osize);
	control = uip->ui_control;
	assert(control);
	lastoff = (nsize & PAGE_MASK_64);

	if (lastoff) {
		upl_t           upl;
		upl_page_info_t *pl;

		  
                                            
                                                 
                         
     
		kret = ubc_create_upl_kernel(vp, lastpg, PAGE_SIZE, &upl, &pl, UPL_SET_LITE | UPL_WILL_MODIFY, VM_KERN_MEMORY_FILE);

		if (kret != KERN_SUCCESS) {
			panic("ubc_setsize: ubc_create_upl (error = %d)\n", kret);
		}

		if (upl_valid_page(pl, 0)) {
			cluster_zero(upl, (uint32_t)lastoff, PAGE_SIZE - (uint32_t)lastoff, NULL);
		}

		ubc_upl_abort_range(upl, 0, PAGE_SIZE, UPL_ABORT_FREE_ON_EMPTY);

		lastpg += PAGE_SIZE_64;
	}
	if (olastpgend > lastpg) {
		int     flags;

		if (lastpg == 0) {
			flags = MEMORY_OBJECT_DATA_FLUSH_ALL;
		} else {
			flags = MEMORY_OBJECT_DATA_FLUSH;
		}
		  
                                                 
    
     
		kret = memory_object_lock_request(control,
		    (memory_object_offset_t)lastpg,
		    (memory_object_size_t)(olastpgend - lastpg), NULL, NULL,
		    MEMORY_OBJECT_RETURN_NONE, flags, VM_PROT_NO_CHANGE);
		if (kret != KERN_SUCCESS) {
			printf("ubc_setsize: invalidate failed (error = %d)\n", kret);
		}
	}
	return mach_to_bsd_errno(kret);
}

                           
int
ubc_setsize(vnode_t vp, off_t nsize)
{
	return ubc_setsize_ex(vp, nsize, 0) == 0;
}

  
              
  
                                                              
  
                                                       
  
                                                    
                                      
                             
  
                                                                
                                                                  
                                                              
                                                               
                                                               
                             
   
off_t
ubc_getsize(struct vnode *vp)
{
	                                                            
                                   
    
	if (!UBCINFOEXISTS(vp)) {
		return (off_t)0;
	}
	return vp->v_ubcinfo->ui_size;
}


  
             
  
                                                                           
              
  
                                   
  
                       
  
                                                           
  
                                                                 
                                                                 
                                                             
                 
  
                                                                   
                                                                  
                                                                  
                                                                
                                                                 
   
__private_extern__ int
ubc_umount(struct mount *mp)
{
	vnode_iterate(mp, 0, ubc_umcallback, 0);
	return 0;
}


  
                 
  
                                                                              
                                                     
   
static int
ubc_umcallback(vnode_t vp, __unused void * args)
{
	if (UBCINFOEXISTS(vp)) {
		(void) ubc_msync(vp, (off_t)0, ubc_getsize(vp), NULL, UBC_PUSHALL);
	}
	return VNODE_RETURNED;
}


  
              
  
                                                                            
         
  
                                                        
                          
  
                                     
                                                   
                                             
                                             
                              
   
kauth_cred_t
ubc_getcred(struct vnode *vp)
{
	if (UBCINFOEXISTS(vp)) {
		return vp->v_ubcinfo->ui_ucred;
	}

	return NOCRED;
}


  
                    
  
                                                                             
                                                                             
              
  
                                                         
             
                                            
                                            
                 
                                                
              
  
                                                     
               
  
                                                                       
                                                               
                                                                 
                                                                
                                                                  
          
  
                                                                
  
                                                     
                               
                                                     
  
                                                              
                                                                  
  
                                                              
  
                                                        
                                                      
                                                     
                       
  
                                                       
                                                           
                                                           
  
                                                               
                                                              
                                                              
                                                              
  
                                                                
                                                            
                                                                  
           
   
int
ubc_setthreadcred(struct vnode *vp, proc_t p, thread_t thread)
{
	struct ubc_info *uip;
	kauth_cred_t credp;
	struct uthread  *uthread = get_bsdthread_info(thread);

	if (!UBCINFOEXISTS(vp)) {
		return 1;
	}

	vnode_lock(vp);

	uip = vp->v_ubcinfo;
	credp = uip->ui_ucred;

	if (!IS_VALID_CRED(credp)) {
		                                                              
		if (uthread == NULL || (uthread->uu_flag & UT_SETUID) == 0) {
			uip->ui_ucred = kauth_cred_proc_ref(p);
		} else {
			uip->ui_ucred = uthread->uu_ucred;
			kauth_cred_ref(uip->ui_ucred);
		}
	}
	vnode_unlock(vp);

	return 0;
}


  
              
  
                                                                             
                                                                          
         
  
                                                         
             
                                            
              
  
                                                     
               
  
                                                                      
                                 
  
                                        
  
                                                                
                                                                   
                                     
  
                                                             
                                
   
int
ubc_setcred(struct vnode *vp, proc_t p)
{
	struct ubc_info *uip;
	kauth_cred_t credp;

	                                                 
	if (!UBCINFOEXISTS(vp)) {
		return 0;
	}

	  
                                                                  
                                                                   
    
	vnode_lock(vp);
	uip = vp->v_ubcinfo;
	credp = uip->ui_ucred;
	if (!IS_VALID_CRED(credp)) {
		uip->ui_ucred = kauth_cred_proc_ref(p);
	}
	vnode_unlock(vp);

	return 1;
}

  
               
  
                                                                        
  
                                                      
  
                                                               
                                                        
  
                                                                     
                                                             
                                                               
                                        
   
__private_extern__ memory_object_t
ubc_getpager(struct vnode *vp)
{
	if (UBCINFOEXISTS(vp)) {
		return vp->v_ubcinfo->ui_pager;
	}

	return 0;
}


  
                
  
                                                                             
            
  
                                                         
           
                      
  
                                       
                              
  
                                                                       
                                                               
                                                             
                                                                
                                                            
                                                             
                                                             
                                                                
                   
   
memory_object_control_t
ubc_getobject(struct vnode *vp, __unused int flags)
{
	if (UBCINFOEXISTS(vp)) {
		return vp->v_ubcinfo->ui_control;
	}

	return MEMORY_OBJECT_CONTROL_NULL;
}

  
               
  
                                                                              
              
  
                                                           
                                       
  
                                                    
                                             
                
                                                
                                        
                                            
                                            
                                              
                                            
  
                                                                        
                                                                
                                                                 
                                                                  
                              
   
off_t
ubc_blktooff(vnode_t vp, daddr64_t blkno)
{
	off_t file_offset = -1;
	int error;

	if (UBCINFOEXISTS(vp)) {
		error = VNOP_BLKTOOFF(vp, blkno, &file_offset);
		if (error) {
			file_offset = -1;
		}
	}

	return file_offset;
}


  
               
  
                                                                              
              
  
                                                    
              
                                               
  
                                           
                                             
                
                                                
                                        
                                            
                                            
                                              
                                            
  
                                                                        
                                                                
                                                                 
                                                               
                                                                  
                 
   
daddr64_t
ubc_offtoblk(vnode_t vp, off_t offset)
{
	daddr64_t blkno = -1;
	int error = 0;

	if (UBCINFOEXISTS(vp)) {
		error = VNOP_OFFTOBLK(vp, offset, &blkno);
		if (error) {
			blkno = -1;
		}
	}

	return blkno;
}


  
                     
  
                                                                           
                                                                        
  
                                                   
  
                   
          
   
int
ubc_pages_resident(vnode_t vp)
{
	kern_return_t           kret;
	boolean_t                       has_pages_resident;

	if (!UBCINFOEXISTS(vp)) {
		return 0;
	}

	  
                                                                      
                                                                       
                                                                       
                               
    
	kret = memory_object_pages_resident(vp->v_ubcinfo->ui_control, &has_pages_resident);

	if (kret != KERN_SUCCESS) {
		return 0;
	}

	if (has_pages_resident == TRUE) {
		return 1;
	}

	return 0;
}

  
            
  
                                                                             
  
                                                         
                                            
                                  
                                                  
                                                
                                                      
                                           
                                    
  
                       
                                      
  
                    
                                                              
                                            
                                            
                                             
                                          
                                          
                                              
                                              
                                            
                                              
                                      
  
                                                                 
  
   
errno_t
ubc_msync(vnode_t vp, off_t beg_off, off_t end_off, off_t *resid_off, int flags)
{
	int retval;
	int io_errno = 0;

	if (resid_off) {
		*resid_off = beg_off;
	}

	retval = ubc_msync_internal(vp, beg_off, end_off, resid_off, flags, &io_errno);

	if (retval == 0 && io_errno == 0) {
		return EINVAL;
	}
	return io_errno;
}


  
                     
  
                                                                             
  
                                                         
                                            
                                  
                                                  
                                                
                                                      
                                           
                                                  
                                        
                                             
                                              
                                       
                                           
                                          
                                                  
                                            
                                        
             
  
                       
               
  
                    
                                                           
                                             
                                         
                                          
                                          
                
                                                              
                                           
                                             
                                              
                                           
                                          
                                              
                                            
                                            
  
                                                                     
                                              
  
                                                                  
                                                                 
              
  
                                                                 
                                                                 
                                                                
                                                                 
                                                             
  
                                                              
                                                                
                                                                
                                                               
                                                  
   
static int
ubc_msync_internal(vnode_t vp, off_t beg_off, off_t end_off, off_t *resid_off, int flags, int *io_errno)
{
	memory_object_size_t    tsize;
	kern_return_t           kret;
	int request_flags = 0;
	int flush_flags   = MEMORY_OBJECT_RETURN_NONE;

	if (!UBCINFOEXISTS(vp)) {
		return 0;
	}
	if ((flags & (UBC_INVALIDATE | UBC_PUSHDIRTY | UBC_PUSHALL)) == 0) {
		return 0;
	}
	if (end_off <= beg_off) {
		return 1;
	}

	if (flags & UBC_INVALIDATE) {
		  
                               
     
		request_flags = (MEMORY_OBJECT_DATA_FLUSH | MEMORY_OBJECT_DATA_NO_CHANGE);
	}

	if (flags & UBC_SYNC) {
		  
                                                      
     
		request_flags |= MEMORY_OBJECT_IO_SYNC;
	}

	if (flags & UBC_PUSHDIRTY) {
		  
                                                
     
		flush_flags = MEMORY_OBJECT_RETURN_DIRTY;
	}

	if (flags & UBC_PUSHALL) {
		  
                                                             
                                     
     
		flush_flags = MEMORY_OBJECT_RETURN_ALL;
	}

	beg_off = trunc_page_64(beg_off);
	end_off = round_page_64(end_off);
	tsize   = (memory_object_size_t)end_off - beg_off;

	                                                          
	kret = memory_object_lock_request(vp->v_ubcinfo->ui_control,
	    beg_off, tsize,
	    (memory_object_offset_t *)resid_off,
	    io_errno, flush_flags, request_flags,
	    VM_PROT_NO_CHANGE);

	return (kret == KERN_SUCCESS) ? 1 : 0;
}


  
          
  
                                                                             
                                                                          
                                                                             
                    
  
                                    
                                                 
                                              
                                           
  
                       
                                 
  
                                                                     
  
                                                                    
                        
  
                                                           
                                                                  
            
  
                                                                
                                                                  
                            
  
                                 
                                                         
                           
                                                          
  
                                                                   
                                                                    
                                                              
                     
  
                                                                   
                                                                
                                                                 
                    
  
                                                              
                                                               
                                                             
                                                            
                  
   
__private_extern__ int
ubc_map(vnode_t vp, int flags)
{
	struct ubc_info *uip;
	int error = 0;
	int need_ref = 0;
	int need_wakeup = 0;

	if (UBCINFOEXISTS(vp)) {
		vnode_lock(vp);
		uip = vp->v_ubcinfo;

		while (ISSET(uip->ui_flags, UI_MAPBUSY)) {
			SET(uip->ui_flags, UI_MAPWAITING);
			(void) msleep(&uip->ui_flags, &vp->v_lock,
			    PRIBIO, "ubc_map", NULL);
		}
		SET(uip->ui_flags, UI_MAPBUSY);
		vnode_unlock(vp);

		error = VNOP_MMAP(vp, flags, vfs_context_current());

		  
                                                              
                                                              
                                                             
                                                                
                                                                     
                     
    
                        
                            
     

		error = 0;

		vnode_lock_spin(vp);

		if (error == 0) {
			if (!ISSET(uip->ui_flags, UI_ISMAPPED)) {
				need_ref = 1;
			}
			SET(uip->ui_flags, (UI_WASMAPPED | UI_ISMAPPED));
			if (flags & PROT_WRITE) {
				SET(uip->ui_flags, UI_MAPPEDWRITE);
			}
		}
		CLR(uip->ui_flags, UI_MAPBUSY);

		if (ISSET(uip->ui_flags, UI_MAPWAITING)) {
			CLR(uip->ui_flags, UI_MAPWAITING);
			need_wakeup = 1;
		}
		vnode_unlock(vp);

		if (need_wakeup) {
			wakeup(&uip->ui_flags);
		}

		if (need_ref) {
			  
                                                         
      
			if (vnode_ref_ext(vp, 0, VNODE_REF_FORCE)) {
				panic("%s : VNODE_REF_FORCE failed\n", __FUNCTION__);
			}
			  
                                                      
                                                         
                                                    
                                                       
                                                      
                                                  
                                                        
                                   
     
                                                          
                                                 
                                                       
                                                         
                                               
      
			if (vp->v_mount != NULL &&
			    ((vp->v_mount->mnt_flag & MNT_ROOTFS) ||
			    vnode_on_reliable_media(vp))) {
				  
                                              
                                  
       
				memory_object_mark_trusted(uip->ui_control);
			} else {
                                                                                                                                                                         
			}
		}
	}
	return error;
}


  
                    
  
                                                                              
                                                                          
                                                             
  
                                        
  
                  
  
                                                                      
                                                                  
                                                                 
                                 
  
                                                              
                                                            
                                                                 
                              
  
                                                            
                                                      
   
__private_extern__ void
ubc_destroy_named(vnode_t vp)
{
	memory_object_control_t control;
	struct ubc_info *uip;
	kern_return_t kret;

	if (UBCINFOEXISTS(vp)) {
		uip = vp->v_ubcinfo;

		                                  
		control = ubc_getobject(vp, UBC_HOLDOBJECT);
		if (control != MEMORY_OBJECT_CONTROL_NULL) {
			kret = memory_object_destroy(control, 0);
			if (kret != KERN_SUCCESS) {
				panic("ubc_destroy_named: memory_object_destroy failed");
			}
		}
	}
}


  
              
  
                                                                            
                                    
  
                                      
                                                     
                                            
                            
  
                                                      
                                        
                
  
                                                                     
                                                                  
                                                                
                                                                  
                                  
  
                                                           
                                                             
   
int
ubc_isinuse(struct vnode *vp, int busycount)
{
	if (!UBCINFOEXISTS(vp)) {
		return 0;
	}
	return ubc_isinuse_locked(vp, busycount, 0);
}


  
                     
  
                                                                            
                                    
  
                                      
                                                     
                                            
                            
                                                   
                 
  
                                                      
                                        
                
  
                                                                 
                                                             
                                                               
                                                             
                                                         
                                                           
                            
  
                                                           
                                                             
   
int
ubc_isinuse_locked(struct vnode *vp, int busycount, int locked)
{
	int retval = 0;


	if (!locked) {
		vnode_lock_spin(vp);
	}

	if ((vp->v_usecount - vp->v_kusecount) > busycount) {
		retval = 1;
	}

	if (!locked) {
		vnode_unlock(vp);
	}
	return retval;
}


  
            
  
                                                            
  
                                           
  
                  
  
                                                                       
                                                                
                                                                 
                                                                 
                                                                 
                             
  
                                                 
                               
   
__private_extern__ void
ubc_unmap(struct vnode *vp)
{
	struct ubc_info *uip;
	int     need_rele = 0;
	int     need_wakeup = 0;

	if (vnode_getwithref(vp)) {
		return;
	}

	if (UBCINFOEXISTS(vp)) {
		bool want_fsevent = false;

		vnode_lock(vp);
		uip = vp->v_ubcinfo;

		while (ISSET(uip->ui_flags, UI_MAPBUSY)) {
			SET(uip->ui_flags, UI_MAPWAITING);
			(void) msleep(&uip->ui_flags, &vp->v_lock,
			    PRIBIO, "ubc_unmap", NULL);
		}
		SET(uip->ui_flags, UI_MAPBUSY);

		if (ISSET(uip->ui_flags, UI_ISMAPPED)) {
			if (ISSET(uip->ui_flags, UI_MAPPEDWRITE)) {
				want_fsevent = true;
			}

			need_rele = 1;

			  
                                                          
                                                  
                                                 
      
		}
		vnode_unlock(vp);

		if (need_rele) {
			vfs_context_t ctx = vfs_context_current();

			(void)VNOP_MNOMAP(vp, ctx);

#if CONFIG_FSE
			  
                                                   
                                                       
                                                         
                                                      
                                                     
                                              
                                                       
                                                         
                                                      
                         
      
			if (want_fsevent && need_fsevent(FSE_CONTENT_MODIFIED, vp)) {
				add_fsevent(FSE_CONTENT_MODIFIED, ctx,
				    FSE_ARG_VNODE, vp,
				    FSE_ARG_DONE);
			}
#endif

			vnode_rele(vp);
		}

		vnode_lock_spin(vp);

		if (need_rele) {
			CLR(uip->ui_flags, UI_ISMAPPED | UI_MAPPEDWRITE);
		}

		CLR(uip->ui_flags, UI_MAPBUSY);

		if (ISSET(uip->ui_flags, UI_MAPWAITING)) {
			CLR(uip->ui_flags, UI_MAPWAITING);
			need_wakeup = 1;
		}
		vnode_unlock(vp);

		if (need_wakeup) {
			wakeup(&uip->ui_flags);
		}
	}
	  
                                          
    
	vnode_put(vp);
}


  
              
  
                                                                           
                                            
  
                                              
                                                
                                                
                                       
                                                      
                
                                                  
                            
  
                                 
                                                                
                        
                                                             
                                
                                                              
                            
                                                 
  
                    
                                                          
                       
                                                          
                                           
  
                                                                     
                                                               
                                                             
                                                                   
                                                                
                                                       
  
                                                                  
                                                                  
                                                                  
                                                            
                                     
  
                                                             
                                     
  
                                                 
                                        
                    
                                           
  
                                                            
                                                               
                   
  
                                      
                                            
                                           
                                        
                                    
  
                                                                   
                                                                
                                                              
  
                                              
                                                
  
                                                                  
                                                                
                                                                   
                                                                  
                                                               
                                    
   
kern_return_t
ubc_page_op(
	struct vnode    *vp,
	off_t           f_offset,
	int             ops,
	ppnum_t *phys_entryp,
	int             *flagsp)
{
	memory_object_control_t         control;

	control = ubc_getobject(vp, UBC_FLAGS_NONE);
	if (control == MEMORY_OBJECT_CONTROL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return memory_object_page_op(control,
	           (memory_object_offset_t)f_offset,
	           ops,
	           phys_entryp,
	           flagsp);
}


  
               
  
                                                                             
                                                                              
                                                                              
                                                                         
                                                                    
  
                                              
                                                          
                                                        
                                                
                                       
                                                 
             
  
                                 
                                                                
                        
                                                              
  
                    
                                                             
                                           
                                        
  
                                                                 
                                            
  
                                                               
                                                                 
                                                                 
                                                              
                                                                
                                                                   
                            
  
                                                           
  
                                                                
               
  
                                                    
                                          
                                       
  
                                                     
                                             
                                      
                
                                                        
                                          
  
                                                                   
                                                                 
                
   
kern_return_t
ubc_range_op(
	struct vnode    *vp,
	off_t           f_offset_beg,
	off_t           f_offset_end,
	int             ops,
	int             *range)
{
	memory_object_control_t         control;

	control = ubc_getobject(vp, UBC_FLAGS_NONE);
	if (control == MEMORY_OBJECT_CONTROL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return memory_object_range_op(control,
	           (memory_object_offset_t)f_offset_beg,
	           (memory_object_offset_t)f_offset_end,
	           ops,
	           range);
}


  
                 
  
                                                                              
                                                                           
                             
  
                                                          
                                                     
                               
                                           
                                              
                                    
                                              
                                            
                
  
                                                            
                                                             
                                
                                                              
                                              
                                           
                                               
                                         
              
                    
                    
                                                           
                                             
                                             
                                           
                                             
  
                                                                     
                                                           
                                              
   
kern_return_t
ubc_create_upl_external(
	struct vnode    *vp,
	off_t           f_offset,
	int             bufsize,
	upl_t           *uplp,
	upl_page_info_t **plp,
	int             uplflags)
{
	return ubc_create_upl_kernel(vp, f_offset, bufsize, uplp, plp, uplflags, vm_tag_bt());
}

kern_return_t
ubc_create_upl_kernel(
	struct vnode    *vp,
	off_t           f_offset,
	int             bufsize,
	upl_t           *uplp,
	upl_page_info_t **plp,
	int             uplflags,
	vm_tag_t tag)
{
	memory_object_control_t         control;
	kern_return_t                   kr;

	if (plp != NULL) {
		*plp = NULL;
	}
	*uplp = NULL;

	if (bufsize & 0xfff) {
		return KERN_INVALID_ARGUMENT;
	}

	if (bufsize > MAX_UPL_SIZE_BYTES) {
		return KERN_INVALID_ARGUMENT;
	}

	if (uplflags & (UPL_UBC_MSYNC | UPL_UBC_PAGEOUT | UPL_UBC_PAGEIN)) {
		if (uplflags & UPL_UBC_MSYNC) {
			uplflags &= UPL_RET_ONLY_DIRTY;

			uplflags |= UPL_COPYOUT_FROM | UPL_CLEAN_IN_PLACE |
			    UPL_SET_INTERNAL | UPL_SET_LITE;
		} else if (uplflags & UPL_UBC_PAGEOUT) {
			uplflags &= UPL_RET_ONLY_DIRTY;

			if (uplflags & UPL_RET_ONLY_DIRTY) {
				uplflags |= UPL_NOBLOCK;
			}

			uplflags |= UPL_FOR_PAGEOUT | UPL_CLEAN_IN_PLACE |
			    UPL_COPYOUT_FROM | UPL_SET_INTERNAL | UPL_SET_LITE;
		} else {
			uplflags |= UPL_RET_ONLY_ABSENT |
			    UPL_NO_SYNC | UPL_CLEAN_IN_PLACE |
			    UPL_SET_INTERNAL | UPL_SET_LITE;

			  
                                                              
                                                              
                                                                   
                                                                     
                                                                      
                                                                  
      
			if (bufsize > PAGE_SIZE) {
				uplflags |= UPL_NOBLOCK;
			}
		}
	} else {
		uplflags &= ~UPL_FOR_PAGEOUT;

		if (uplflags & UPL_WILL_BE_DUMPED) {
			uplflags &= ~UPL_WILL_BE_DUMPED;
			uplflags |= (UPL_NO_SYNC | UPL_SET_INTERNAL);
		} else {
			uplflags |= (UPL_NO_SYNC | UPL_CLEAN_IN_PLACE | UPL_SET_INTERNAL);
		}
	}
	control = ubc_getobject(vp, UBC_FLAGS_NONE);
	if (control == MEMORY_OBJECT_CONTROL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	kr = memory_object_upl_request(control, f_offset, bufsize, uplp, NULL, NULL, uplflags, tag);
	if (kr == KERN_SUCCESS && plp != NULL) {
		*plp = UPL_GET_INTERNAL_PAGE_LIST(*uplp);
	}
	return kr;
}


  
                     
  
                                                          
  
                   
  
                                                                       
   
upl_size_t
ubc_upl_maxbufsize(
	void)
{
	return MAX_UPL_SIZE_BYTES;
}

  
              
  
                                                                            
                                                                           
                           
  
                                   
                                                 
  
                                                 
                                             
                                           
                                     
                                             
                             
   
kern_return_t
ubc_upl_map(
	upl_t           upl,
	vm_offset_t     *dst_addr)
{
	return vm_upl_map(kernel_map, upl, dst_addr);
}


  
                
  
                                                                              
                                             
  
                                     
  
                                                   
                                                 
                                                
   
kern_return_t
ubc_upl_unmap(
	upl_t   upl)
{
	return vm_upl_unmap(kernel_map, upl);
}


  
                 
  
                                                      
  
                                      
  
                                                    
                                                       
                                                     
                                             
                                           
              
  
                                                                        
                                                                  
                                                                  
                                                             
  
                                                                   
                                                                 
                     
   
kern_return_t
ubc_upl_commit(
	upl_t                   upl)
{
	upl_page_info_t *pl;
	kern_return_t   kr;

	pl = UPL_GET_INTERNAL_PAGE_LIST(upl);
	kr = upl_commit(upl, pl, MAX_UPL_SIZE_BYTES >> PAGE_SHIFT);
	upl_deallocate(upl);
	return kr;
}


  
                 
  
                                                                             
  
                                      
                                    
                                                  
                                       
                                   
  
                                                      
                                                       
                                                     
                                             
                                           
              
  
                                                                       
                                                                
                                                                  
                                                                   
                             
  
                                                                   
                                                 
  
                                                      
                                
                               
                                                    
                            
                      
                                                
                                
                
                                             
                                
                             
                                                      
                            
                              
                              
                                
                        
  
                                                                   
                                   
  
                                                                 
                                                   
   
kern_return_t
ubc_upl_commit_range(
	upl_t                   upl,
	upl_offset_t            offset,
	upl_size_t              size,
	int                             flags)
{
	upl_page_info_t *pl;
	boolean_t               empty;
	kern_return_t   kr;

	if (flags & UPL_COMMIT_FREE_ON_EMPTY) {
		flags |= UPL_COMMIT_NOTIFY_EMPTY;
	}

	if (flags & UPL_COMMIT_KERNEL_ONLY_FLAGS) {
		return KERN_INVALID_ARGUMENT;
	}

	pl = UPL_GET_INTERNAL_PAGE_LIST(upl);

	kr = upl_commit_range(upl, offset, size, flags,
	    pl, MAX_UPL_SIZE_BYTES >> PAGE_SHIFT, &empty);

	if ((flags & UPL_COMMIT_FREE_ON_EMPTY) && empty) {
		upl_deallocate(upl);
	}

	return kr;
}


  
                      
  
                                                                 
  
                                     
                                    
                                                
                                       
                                       
  
                                                    
                                                       
                                                     
                                             
                                           
              
  
                                                                      
                                                                
                                                                 
                                                                   
                             
  
                                                                 
                                                        
  
                                                          
                                       
               
                                                       
                                                     
                                            
                                              
                                     
                                     
  
                                                                 
                                                                  
                                                             
                                                               
  
                                                              
                              
   
kern_return_t
ubc_upl_abort_range(
	upl_t                   upl,
	upl_offset_t            offset,
	upl_size_t              size,
	int                             abort_flags)
{
	kern_return_t   kr;
	boolean_t               empty = FALSE;

	if (abort_flags & UPL_ABORT_FREE_ON_EMPTY) {
		abort_flags |= UPL_ABORT_NOTIFY_EMPTY;
	}

	kr = upl_abort_range(upl, offset, size, abort_flags, &empty);

	if ((abort_flags & UPL_ABORT_FREE_ON_EMPTY) && empty) {
		upl_deallocate(upl);
	}

	return kr;
}


  
                
  
                                          
  
                                     
                                      
  
                                                    
                                                       
                                                     
                                             
                                           
              
  
                                                                      
                                                                
                                                                 
                                                                   
                             
  
                                                       
                                                        
  
                                                          
                                       
               
                                                       
                                                     
                                            
                                              
                                     
                                     
  
                                                                 
                                                                  
                                                             
                                                               
  
                                                              
                              
   
kern_return_t
ubc_upl_abort(
	upl_t                   upl,
	int                             abort_type)
{
	kern_return_t   kr;

	kr = upl_abort(upl, abort_type);
	upl_deallocate(upl);
	return kr;
}


  
                   
  
                                                         
  
                                                         
  
                                                        
                               
                                        
  
                                                                   
                                                               
  
                                                                  
                                                                
                                            
   
upl_page_info_t *
ubc_upl_pageinfo(
	upl_t                   upl)
{
	return UPL_GET_INTERNAL_PAGE_LIST(upl);
}


int
UBCINFOEXISTS(const struct vnode * vp)
{
	return (vp) && ((vp)->v_type == VREG) && ((vp)->v_ubcinfo != UBC_INFO_NULL);
}


void
ubc_upl_range_needed(
	upl_t           upl,
	int             index,
	int             count)
{
	upl_range_needed(upl, index, count);
}

boolean_t
ubc_is_mapped(const struct vnode *vp, boolean_t *writable)
{
	if (!UBCINFOEXISTS(vp) || !ISSET(vp->v_ubcinfo->ui_flags, UI_ISMAPPED)) {
		return FALSE;
	}
	if (writable) {
		*writable = ISSET(vp->v_ubcinfo->ui_flags, UI_MAPPEDWRITE);
	}
	return TRUE;
}

boolean_t
ubc_is_mapped_writable(const struct vnode *vp)
{
	boolean_t writable;
	return ubc_is_mapped(vp, &writable) && writable;
}


  
               
   
static atomic_size_t cs_blob_size = 0;
static atomic_uint_fast32_t cs_blob_count = 0;
static atomic_size_t cs_blob_size_peak = 0;
static atomic_size_t cs_blob_size_max = 0;
static atomic_uint_fast32_t cs_blob_count_peak = 0;

SYSCTL_UINT(_vm, OID_AUTO, cs_blob_count, CTLFLAG_RD | CTLFLAG_LOCKED, &cs_blob_count, 0, "Current number of code signature blobs");
SYSCTL_ULONG(_vm, OID_AUTO, cs_blob_size, CTLFLAG_RD | CTLFLAG_LOCKED, &cs_blob_size, "Current size of all code signature blobs");
SYSCTL_UINT(_vm, OID_AUTO, cs_blob_count_peak, CTLFLAG_RD | CTLFLAG_LOCKED, &cs_blob_count_peak, 0, "Peak number of code signature blobs");
SYSCTL_ULONG(_vm, OID_AUTO, cs_blob_size_peak, CTLFLAG_RD | CTLFLAG_LOCKED, &cs_blob_size_peak, "Peak size of code signature blobs");
SYSCTL_ULONG(_vm, OID_AUTO, cs_blob_size_max, CTLFLAG_RD | CTLFLAG_LOCKED, &cs_blob_size_max, "Size of biggest code signature blob");

  
                                
  
                                                              
                                                               
                                                                   
                      
                                                                     
                                                               
                                             
   

static const char *
csblob_parse_teamid(struct cs_blob *csblob)
{
	const CS_CodeDirectory *cd;

	cd = csblob->csb_cd;

	if (ntohl(cd->version) < CS_SUPPORTSTEAMID) {
		return NULL;
	}

	if (cd->teamOffset == 0) {
		return NULL;
	}

	const char *name = ((const char *)cd) + ntohl(cd->teamOffset);
	if (cs_debug > 1) {
		printf("found team-id %s in cdblob\n", name);
	}

	return name;
}


kern_return_t
ubc_cs_blob_allocate(
	vm_offset_t     *blob_addr_p,
	vm_size_t       *blob_size_p)
{
	kern_return_t   kr = KERN_FAILURE;

	{
		*blob_addr_p = (vm_offset_t) kalloc_tag(*blob_size_p, VM_KERN_MEMORY_SECURITY);

		if (*blob_addr_p == 0) {
			kr = KERN_NO_SPACE;
		} else {
			kr = KERN_SUCCESS;
		}
	}

	return kr;
}

void
ubc_cs_blob_deallocate(
	vm_offset_t     blob_addr,
	vm_size_t       blob_size)
{
	{
		kfree(blob_addr, blob_size);
	}
}

  
                                                                     
                                                                    
                                                         
                                                                     
                                                                     
                                                                    
                                                                
                                                                     
                                                               
                                                                 
   
static boolean_t
ubc_cs_supports_multilevel_hash(struct cs_blob *blob __unused)
{
	const CS_CodeDirectory *cd;


	  
                                                         
                               
    
	if (!blob->csb_platform_binary || blob->csb_teamid != NULL) {
		return FALSE;
	}

	  
                                                          
                                 
    
	if (PAGE_SHIFT <= blob->csb_hash_pageshift) {
		return FALSE;
	}

	cd = blob->csb_cd;

	  
                                                     
    
	if (ntohl(cd->nCodeSlots) & (PAGE_MASK >> blob->csb_hash_pageshift)) {
		return FALSE;
	}

	  
                                                                              
    
	if ((ntohl(cd->version) >= CS_SUPPORTSSCATTER) && (ntohl(cd->scatterOffset))) {
		const SC_Scatter *scatter = (const SC_Scatter*)
		    ((const char*)cd + ntohl(cd->scatterOffset));
		                                                                   
		do {
			uint32_t sbase = ntohl(scatter->base);
			uint32_t scount = ntohl(scatter->count);

			                   
			if (scount == 0) {
				break;
			}

			if (sbase & (PAGE_MASK >> blob->csb_hash_pageshift)) {
				return FALSE;
			}

			if (scount & (PAGE_MASK >> blob->csb_hash_pageshift)) {
				return FALSE;
			}

			scatter++;
		} while (1);
	}

	                                                           
	if (ntohl(cd->codeLimit) & PAGE_MASK) {
		return FALSE;
	}

	                     
	return TRUE;
}

  
                                                                   
                                                                   
                                                              
                                                                  
        
  
                                                                      
                                                               
  
                                                                      
                                                                     
                                                      
                                                                     
                                                                  
   
static int
ubc_cs_reconstitute_code_signature(struct cs_blob const *blob, vm_size_t optional_new_cd_size,
    vm_address_t *new_blob_addr_p, vm_size_t *new_blob_size_p,
    CS_CodeDirectory **new_cd_p, CS_GenericBlob const **new_entitlements_p)
{
	const CS_CodeDirectory  *old_cd, *cd;
	CS_CodeDirectory        *new_cd;
	const CS_GenericBlob *entitlements;
	vm_offset_t     new_blob_addr;
	vm_size_t       new_blob_size;
	vm_size_t       new_cdsize;
	kern_return_t   kr;
	int                             error;

	old_cd = blob->csb_cd;

	new_cdsize = optional_new_cd_size != 0 ? optional_new_cd_size : htonl(old_cd->length);

	new_blob_size  = sizeof(CS_SuperBlob);
	new_blob_size += sizeof(CS_BlobIndex);
	new_blob_size += new_cdsize;

	if (blob->csb_entitlements_blob) {
		                                                
		ptrauth_utils_auth_blob_generic(blob->csb_entitlements_blob,
		    ntohl(blob->csb_entitlements_blob->length),
		    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_entitlements_blob_signature"),
		    PTRAUTH_ADDR_DIVERSIFY,
		    blob->csb_entitlements_blob_signature);

		new_blob_size += sizeof(CS_BlobIndex);
		new_blob_size += ntohl(blob->csb_entitlements_blob->length);
	}

	kr = ubc_cs_blob_allocate(&new_blob_addr, &new_blob_size);
	if (kr != KERN_SUCCESS) {
		if (cs_debug > 1) {
			printf("CODE SIGNING: Failed to allocate memory for new Code Signing Blob: %d\n",
			    kr);
		}
		return ENOMEM;
	}

	CS_SuperBlob            *new_superblob;

	new_superblob = (CS_SuperBlob *)new_blob_addr;
	new_superblob->magic = htonl(CSMAGIC_EMBEDDED_SIGNATURE);
	new_superblob->length = htonl((uint32_t)new_blob_size);
	if (blob->csb_entitlements_blob) {
		vm_size_t                       ent_offset, cd_offset;

		cd_offset  = sizeof(CS_SuperBlob) + 2 * sizeof(CS_BlobIndex);
		ent_offset = cd_offset +  new_cdsize;

		new_superblob->count = htonl(2);
		new_superblob->index[0].type = htonl(CSSLOT_CODEDIRECTORY);
		new_superblob->index[0].offset = htonl((uint32_t)cd_offset);
		new_superblob->index[1].type = htonl(CSSLOT_ENTITLEMENTS);
		new_superblob->index[1].offset = htonl((uint32_t)ent_offset);

		ptrauth_utils_auth_blob_generic(blob->csb_entitlements_blob,
		    ntohl(blob->csb_entitlements_blob->length),
		    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_entitlements_blob_signature"),
		    PTRAUTH_ADDR_DIVERSIFY,
		    blob->csb_entitlements_blob_signature);

		memcpy((void *)(new_blob_addr + ent_offset), blob->csb_entitlements_blob, ntohl(blob->csb_entitlements_blob->length));

		new_cd = (CS_CodeDirectory *)(new_blob_addr + cd_offset);
	} else {
		                                        
		new_cd = (CS_CodeDirectory *)new_blob_addr;
	}

	if (optional_new_cd_size == 0) {
		                                       
		memcpy(new_cd, old_cd, new_cdsize);

		vm_size_t length = new_blob_size;

		error = cs_validate_csblob((const uint8_t *)new_blob_addr, length, &cd, &entitlements);

		if (error) {
			printf("CODE SIGNING: Failed to validate new Code Signing Blob: %d\n",
			    error);

			ubc_cs_blob_deallocate(new_blob_addr, new_blob_size);
			return error;
		}
		*new_entitlements_p = entitlements;
	} else {
		                                                    
		memset(new_cd, 0, new_cdsize);
		*new_entitlements_p = NULL;
	}

	*new_blob_addr_p = new_blob_addr;
	*new_blob_size_p = new_blob_size;
	*new_cd_p = new_cd;

	return 0;
}

static int
ubc_cs_convert_to_multilevel_hash(struct cs_blob *blob)
{
	const CS_CodeDirectory  *old_cd, *cd;
	CS_CodeDirectory        *new_cd;
	const CS_GenericBlob *entitlements;
	vm_offset_t     new_blob_addr;
	vm_size_t       new_blob_size;
	vm_size_t       new_cdsize;
	int                             error;

	uint32_t                hashes_per_new_hash_shift = (uint32_t)(PAGE_SHIFT - blob->csb_hash_pageshift);

	if (cs_debug > 1) {
		printf("CODE SIGNING: Attempting to convert Code Directory for %lu -> %lu page shift\n",
		    (unsigned long)blob->csb_hash_pageshift, (unsigned long)PAGE_SHIFT);
	}

	old_cd = blob->csb_cd;

	                                            
	new_cdsize  = ntohl(old_cd->hashOffset);
	new_cdsize += (ntohl(old_cd->nCodeSlots) >> hashes_per_new_hash_shift) * old_cd->hashSize;

	error = ubc_cs_reconstitute_code_signature(blob, new_cdsize,
	    &new_blob_addr, &new_blob_size, &new_cd,
	    &entitlements);
	if (error != 0) {
		printf("CODE SIGNING: Failed to reconsitute code signature: %d\n", error);
		return error;
	}

	memcpy(new_cd, old_cd, ntohl(old_cd->hashOffset));

	                                                   
	new_cd->length = htonl((uint32_t)new_cdsize);

	uint32_t nCodeSlots = ntohl(new_cd->nCodeSlots);
	nCodeSlots >>= hashes_per_new_hash_shift;
	new_cd->nCodeSlots = htonl(nCodeSlots);

	new_cd->pageSize = (uint8_t)PAGE_SHIFT;                       

	if ((ntohl(new_cd->version) >= CS_SUPPORTSSCATTER) && (ntohl(new_cd->scatterOffset))) {
		SC_Scatter *scatter = (SC_Scatter*)
		    ((char *)new_cd + ntohl(new_cd->scatterOffset));
		                                                       
		do {
			uint32_t scount = ntohl(scatter->count);
			uint32_t sbase  = ntohl(scatter->base);

			                   
			if (scount == 0) {
				break;
			}

			scount >>= hashes_per_new_hash_shift;
			scatter->count = htonl(scount);

			sbase >>= hashes_per_new_hash_shift;
			scatter->base = htonl(sbase);

			scatter++;
		} while (1);
	}

	                                                  
	const unsigned char *src_base = (const unsigned char *)old_cd + ntohl(old_cd->hashOffset);
	unsigned char *dst_base = (unsigned char *)new_cd + ntohl(new_cd->hashOffset);

	uint32_t hash_index;
	for (hash_index = 0; hash_index < nCodeSlots; hash_index++) {
		union cs_hash_union     mdctx;

		uint32_t source_hash_len = old_cd->hashSize << hashes_per_new_hash_shift;
		const unsigned char *src = src_base + hash_index * source_hash_len;
		unsigned char *dst = dst_base + hash_index * new_cd->hashSize;

		blob->csb_hashtype->cs_init(&mdctx);
		blob->csb_hashtype->cs_update(&mdctx, src, source_hash_len);
		blob->csb_hashtype->cs_final(dst, &mdctx);
	}

	error = cs_validate_csblob((const uint8_t *)new_blob_addr, new_blob_size, &cd, &entitlements);
	if (error != 0) {
		printf("CODE SIGNING: Failed to validate new Code Signing Blob: %d\n",
		    error);

		ubc_cs_blob_deallocate(new_blob_addr, new_blob_size);
		return error;
	}

	                                                                            
	ubc_cs_blob_deallocate((vm_offset_t)blob->csb_mem_kaddr, blob->csb_mem_size);

	blob->csb_mem_size = new_blob_size;
	blob->csb_mem_kaddr = (void *)new_blob_addr;
	blob->csb_cd = cd;
	blob->csb_entitlements_blob = entitlements;
	if (blob->csb_entitlements_blob != NULL) {
		blob->csb_entitlements_blob_signature = ptrauth_utils_sign_blob_generic(blob->csb_entitlements_blob,
		    ntohl(blob->csb_entitlements_blob->length),
		    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_entitlements_blob_signature"),
		    PTRAUTH_ADDR_DIVERSIFY);
	}

	                                                                                

	blob->csb_hash_firstlevel_pageshift = blob->csb_hash_pageshift;                                  

	blob->csb_hash_pageshift = PAGE_SHIFT;
	blob->csb_end_offset = ntohl(cd->codeLimit);
	if ((ntohl(cd->version) >= CS_SUPPORTSSCATTER) && (ntohl(cd->scatterOffset))) {
		const SC_Scatter *scatter = (const SC_Scatter*)
		    ((const char*)cd + ntohl(cd->scatterOffset));
		blob->csb_start_offset = ((off_t)ntohl(scatter->base)) * PAGE_SIZE;
	} else {
		blob->csb_start_offset = 0;
	}

	return 0;
}

  
                                                                    
                                                                     
                         
  
                                                                       
                                                                     
                                                             
   
int
cs_blob_create_validated(
	vm_address_t * const            addr,
	vm_size_t                       size,
	struct cs_blob ** const         ret_blob,
	CS_CodeDirectory const ** const     ret_cd)
{
	struct cs_blob          *blob;
	int             error = EINVAL;
	const CS_CodeDirectory *cd;
	const CS_GenericBlob *entitlements;
	union cs_hash_union     mdctx;
	size_t                  length;

	if (ret_blob) {
		*ret_blob = NULL;
	}

	blob = (struct cs_blob *) kalloc(sizeof(struct cs_blob));
	if (blob == NULL) {
		return ENOMEM;
	}

	                          
	blob->csb_mem_size = size;
	blob->csb_mem_offset = 0;
	blob->csb_mem_kaddr = (void *)*addr;
	blob->csb_flags = 0;
	blob->csb_signer_type = CS_SIGNER_TYPE_UNKNOWN;
	blob->csb_platform_binary = 0;
	blob->csb_platform_path = 0;
	blob->csb_teamid = NULL;
#if CONFIG_SUPPLEMENTAL_SIGNATURES
	blob->csb_supplement_teamid = NULL;
#endif
	blob->csb_entitlements_blob = NULL;
	blob->csb_entitlements = NULL;
	blob->csb_reconstituted = false;

	                                                                      
	*addr = 0;

	  
                                
    
	length = (size_t) size;
	error = cs_validate_csblob((const uint8_t *)blob->csb_mem_kaddr,
	    length, &cd, &entitlements);
	if (error) {
		if (cs_debug) {
			printf("CODESIGNING: csblob invalid: %d\n", error);
		}
		  
                                                           
                                                    
		goto out;
	} else {
		const unsigned char *md_base;
		uint8_t hash[CS_HASH_MAX_SIZE];
		int md_size;
		vm_offset_t hash_pagemask;

		blob->csb_cd = cd;
		blob->csb_entitlements_blob = entitlements;                                     
		if (blob->csb_entitlements_blob != NULL) {
			blob->csb_entitlements_blob_signature = ptrauth_utils_sign_blob_generic(blob->csb_entitlements_blob,
			    ntohl(blob->csb_entitlements_blob->length),
			    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_entitlements_blob_signature"),
			    PTRAUTH_ADDR_DIVERSIFY);
		}
		blob->csb_hashtype = cs_find_md(cd->hashType);
		if (blob->csb_hashtype == NULL || blob->csb_hashtype->cs_digest_size > sizeof(hash)) {
			panic("validated CodeDirectory but unsupported type");
		}

		blob->csb_hash_pageshift = cd->pageSize;
		hash_pagemask = (1U << cd->pageSize) - 1;
		blob->csb_hash_firstlevel_pageshift = 0;
		blob->csb_flags = (ntohl(cd->flags) & CS_ALLOWED_MACHO) | CS_VALID;
		blob->csb_end_offset = (((vm_offset_t)ntohl(cd->codeLimit) + hash_pagemask) & ~hash_pagemask);
		if ((ntohl(cd->version) >= CS_SUPPORTSSCATTER) && (ntohl(cd->scatterOffset))) {
			const SC_Scatter *scatter = (const SC_Scatter*)
			    ((const char*)cd + ntohl(cd->scatterOffset));
			blob->csb_start_offset = ((off_t)ntohl(scatter->base)) * (1U << blob->csb_hash_pageshift);
		} else {
			blob->csb_start_offset = 0;
		}
		                               
		md_base = (const unsigned char *) cd;
		md_size = ntohl(cd->length);

		blob->csb_hashtype->cs_init(&mdctx);
		blob->csb_hashtype->cs_update(&mdctx, md_base, md_size);
		blob->csb_hashtype->cs_final(hash, &mdctx);

		memcpy(blob->csb_cdhash, hash, CS_CDHASH_LEN);
		blob->csb_cdhash_signature = ptrauth_utils_sign_blob_generic(blob->csb_cdhash,
		    sizeof(blob->csb_cdhash),
		    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_cd_signature"),
		    PTRAUTH_ADDR_DIVERSIFY);

#if CONFIG_SUPPLEMENTAL_SIGNATURES
		blob->csb_linkage_hashtype = NULL;
		if (ntohl(cd->version) >= CS_SUPPORTSLINKAGE && cd->linkageHashType != 0 &&
		    ntohl(cd->linkageSize) >= CS_CDHASH_LEN) {
			blob->csb_linkage_hashtype = cs_find_md(cd->linkageHashType);

			if (blob->csb_linkage_hashtype != NULL) {
				memcpy(blob->csb_linkage, (uint8_t const*)cd + ntohl(cd->linkageOffset),
				    CS_CDHASH_LEN);
			}
		}
#endif
	}

	error = 0;

out:
	if (error != 0) {
		cs_blob_free(blob);
		blob = NULL;
		cd = NULL;
	}

	if (ret_blob != NULL) {
		*ret_blob = blob;
	}
	if (ret_cd != NULL) {
		*ret_cd = cd;
	}

	return error;
}

  
                                                                 
   
void
cs_blob_free(
	struct cs_blob * const blob)
{
	if (blob != NULL) {
		if (blob->csb_mem_kaddr) {
			ubc_cs_blob_deallocate((vm_offset_t)blob->csb_mem_kaddr, blob->csb_mem_size);
			blob->csb_mem_kaddr = NULL;
		}
		if (blob->csb_entitlements != NULL) {
			osobject_release(blob->csb_entitlements);
			blob->csb_entitlements = NULL;
		}
		(kfree)(blob, sizeof(*blob));
	}
}
#if CONFIG_SUPPLEMENTAL_SIGNATURES
static void
cs_blob_supplement_free(struct cs_blob * const blob)
{
	if (blob != NULL) {
		if (blob->csb_supplement_teamid != NULL) {
			vm_size_t teamid_size = strlen(blob->csb_supplement_teamid) + 1;
			kfree(blob->csb_supplement_teamid, teamid_size);
			blob->csb_supplement_teamid = NULL;
		}
		cs_blob_free(blob);
	}
}
#endif

static void
ubc_cs_blob_adjust_statistics(struct cs_blob const *blob)
{
	                                                       
                                                                
                                                                
                                                              
                                                                  
                                        
    

	unsigned int new_cs_blob_count = os_atomic_add(&cs_blob_count, 1, relaxed);
	if (new_cs_blob_count > os_atomic_load(&cs_blob_count_peak, relaxed)) {
		os_atomic_store(&cs_blob_count_peak, new_cs_blob_count, relaxed);
	}

	size_t new_cs_blob_size = os_atomic_add(&cs_blob_size, blob->csb_mem_size, relaxed);

	if (new_cs_blob_size > os_atomic_load(&cs_blob_size_peak, relaxed)) {
		os_atomic_store(&cs_blob_size_peak, new_cs_blob_size, relaxed);
	}
	if (blob->csb_mem_size > os_atomic_load(&cs_blob_size_max, relaxed)) {
		os_atomic_store(&cs_blob_size_max, blob->csb_mem_size, relaxed);
	}
}

int
ubc_cs_blob_add(
	struct vnode    *vp,
	uint32_t        platform,
	cpu_type_t      cputype,
	cpu_subtype_t   cpusubtype,
	off_t           base_offset,
	vm_address_t    *addr,
	vm_size_t       size,
	struct image_params *imgp,
	__unused int    flags,
	struct cs_blob  **ret_blob)
{
	kern_return_t           kr;
	struct ubc_info         *uip;
	struct cs_blob          *blob = NULL, *oblob = NULL;
	int                     error;
	CS_CodeDirectory const *cd;
	off_t                   blob_start_offset, blob_end_offset;
	boolean_t               record_mtime;

	record_mtime = FALSE;
	if (ret_blob) {
		*ret_blob = NULL;
	}

	                                                                        
                                                   
	error = cs_blob_create_validated(addr, size, &blob, &cd);

	if (error != 0) {
		printf("malform code signature blob: %d\n", error);
		return error;
	}

	blob->csb_cpu_type = cputype;
	blob->csb_cpu_subtype = cpusubtype & ~CPU_SUBTYPE_MASK;
	blob->csb_base_offset = base_offset;

	  
                                                                     
    
#if CONFIG_MACF
	unsigned int cs_flags = blob->csb_flags;
	unsigned int signer_type = blob->csb_signer_type;
	error = mac_vnode_check_signature(vp, blob, imgp, &cs_flags, &signer_type, flags, platform);
	blob->csb_flags = cs_flags;
	blob->csb_signer_type = signer_type;

	if (error) {
		if (cs_debug) {
			printf("check_signature[pid: %d], error = %d\n", current_proc()->p_pid, error);
		}
		goto out;
	}
	if ((flags & MAC_VNODE_CHECK_DYLD_SIM) && !(blob->csb_flags & CS_PLATFORM_BINARY)) {
		if (cs_debug) {
			printf("check_signature[pid: %d], is not apple signed\n", current_proc()->p_pid);
		}
		error = EPERM;
		goto out;
	}
#endif

#if CONFIG_ENFORCE_SIGNED_CODE
	  
                               
    
	{
		vm_address_t new_mem_kaddr = 0;
		vm_size_t new_mem_size = 0;

		CS_CodeDirectory *new_cd = NULL;
		CS_GenericBlob const *new_entitlements = NULL;

		error = ubc_cs_reconstitute_code_signature(blob, 0,
		    &new_mem_kaddr, &new_mem_size,
		    &new_cd, &new_entitlements);

		if (error != 0) {
			printf("failed code signature reconstitution: %d\n", error);
			goto out;
		}

		ubc_cs_blob_deallocate((vm_offset_t)blob->csb_mem_kaddr, blob->csb_mem_size);

		blob->csb_mem_kaddr = (void *)new_mem_kaddr;
		blob->csb_mem_size = new_mem_size;
		blob->csb_cd = new_cd;
		blob->csb_entitlements_blob = new_entitlements;
		if (blob->csb_entitlements_blob != NULL) {
			blob->csb_entitlements_blob_signature = ptrauth_utils_sign_blob_generic(blob->csb_entitlements_blob,
			    ntohl(blob->csb_entitlements_blob->length),
			    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_entitlements_blob_signature"),
			    PTRAUTH_ADDR_DIVERSIFY);
		}
		blob->csb_reconstituted = true;
	}
#endif


	if (blob->csb_flags & CS_PLATFORM_BINARY) {
		if (cs_debug > 1) {
			printf("check_signature[pid: %d]: platform binary\n", current_proc()->p_pid);
		}
		blob->csb_platform_binary = 1;
		blob->csb_platform_path = !!(blob->csb_flags & CS_PLATFORM_PATH);
	} else {
		blob->csb_platform_binary = 0;
		blob->csb_platform_path = 0;
		blob->csb_teamid = csblob_parse_teamid(blob);
		if (cs_debug > 1) {
			if (blob->csb_teamid) {
				printf("check_signature[pid: %d]: team-id is %s\n", current_proc()->p_pid, blob->csb_teamid);
			} else {
				printf("check_signature[pid: %d]: no team-id\n", current_proc()->p_pid);
			}
		}
	}

	  
                                
    
	blob_start_offset = blob->csb_base_offset + blob->csb_start_offset;
	blob_end_offset = blob->csb_base_offset + blob->csb_end_offset;

	if (blob_start_offset >= blob_end_offset ||
	    blob_start_offset < 0 ||
	    blob_end_offset <= 0) {
		                                    
		error = EINVAL;
		goto out;
	}

	if (ubc_cs_supports_multilevel_hash(blob)) {
		error = ubc_cs_convert_to_multilevel_hash(blob);
		if (error != 0) {
			printf("failed multilevel hash conversion: %d\n", error);
			goto out;
		}
		blob->csb_reconstituted = true;
	}

	vnode_lock(vp);
	if (!UBCINFOEXISTS(vp)) {
		vnode_unlock(vp);
		error = ENOENT;
		goto out;
	}
	uip = vp->v_ubcinfo;

	                                                           
	for (oblob = uip->cs_blobs;
	    oblob != NULL;
	    oblob = oblob->csb_next) {
		off_t oblob_start_offset, oblob_end_offset;

		if (blob->csb_signer_type != oblob->csb_signer_type) {                                                
			vnode_unlock(vp);
			error = EALREADY;
			goto out;
		} else if (blob->csb_platform_binary) {                                                       
			if (!oblob->csb_platform_binary) {
				vnode_unlock(vp);
				error = EALREADY;
				goto out;
			}
		} else if (blob->csb_teamid) {                                                     
			if (oblob->csb_platform_binary ||
			    oblob->csb_teamid == NULL ||
			    strcmp(oblob->csb_teamid, blob->csb_teamid) != 0) {
				vnode_unlock(vp);
				error = EALREADY;
				goto out;
			}
		} else {                                                          
			if (oblob->csb_platform_binary ||
			    oblob->csb_teamid != NULL) {
				vnode_unlock(vp);
				error = EALREADY;
				goto out;
			}
		}

		oblob_start_offset = (oblob->csb_base_offset +
		    oblob->csb_start_offset);
		oblob_end_offset = (oblob->csb_base_offset +
		    oblob->csb_end_offset);
		if (blob_start_offset >= oblob_end_offset ||
		    blob_end_offset <= oblob_start_offset) {
			                                         
		} else {
			                
			if (blob_start_offset == oblob_start_offset &&
			    blob_end_offset == oblob_end_offset &&
			    blob->csb_mem_size == oblob->csb_mem_size &&
			    blob->csb_flags == oblob->csb_flags &&
			    (blob->csb_cpu_type == CPU_TYPE_ANY ||
			    oblob->csb_cpu_type == CPU_TYPE_ANY ||
			    blob->csb_cpu_type == oblob->csb_cpu_type) &&
			    !bcmp(blob->csb_cdhash,
			    oblob->csb_cdhash,
			    CS_CDHASH_LEN)) {
				  
                                 
                               
                               
       
				if (oblob->csb_cpu_type == CPU_TYPE_ANY) {
					  
                                     
                                      
                                          
                           
        
					oblob->csb_cpu_type = cputype;
				}

				                                                 
                           
				uip->cs_add_gen = cs_blob_generation_count;

				vnode_unlock(vp);
				if (ret_blob) {
					*ret_blob = oblob;
				}
				error = EAGAIN;
				goto out;
			} else {
				                                        
				vnode_unlock(vp);
				error = EALREADY;
				goto out;
			}
		}
	}


	                                                          
	kr = memory_object_signed(uip->ui_control, TRUE);
	if (kr != KERN_SUCCESS) {
		vnode_unlock(vp);
		error = ENOENT;
		goto out;
	}

	if (uip->cs_blobs == NULL) {
		                                                               
		record_mtime = TRUE;
	}

	                                           
	uip->cs_add_gen = cs_blob_generation_count;

	  
                                                      
                                                                
                                                                 
                                                           
                                                                     
    
	blob->csb_next = uip->cs_blobs;
	uip->cs_blobs = blob;

	ubc_cs_blob_adjust_statistics(blob);

	if (cs_debug > 1) {
		proc_t p;
		const char *name = vnode_getname_printable(vp);
		p = current_proc();
		printf("CODE SIGNING: proc %d(%s) "
		    "loaded %s signatures for file (%s) "
		    "range 0x%llx:0x%llx flags 0x%x\n",
		    p->p_pid, p->p_comm,
		    blob->csb_cpu_type == -1 ? "detached" : "embedded",
		    name,
		    blob->csb_base_offset + blob->csb_start_offset,
		    blob->csb_base_offset + blob->csb_end_offset,
		    blob->csb_flags);
		vnode_putname_printable(name);
	}

	vnode_unlock(vp);

	if (record_mtime) {
		vnode_mtime(vp, &uip->cs_mtime, vfs_context_current());
	}

	if (ret_blob) {
		*ret_blob = blob;
	}

	error = 0;                     

out:
	if (error) {
		if (cs_debug) {
			printf("check_signature[pid: %d]: error = %d\n", current_proc()->p_pid, error);
		}

		cs_blob_free(blob);
	}

	if (error == EAGAIN) {
		  
                                                 
                                                       
                                        
     
		error = 0;
	}

	return error;
}

#if CONFIG_SUPPLEMENTAL_SIGNATURES
int
ubc_cs_blob_add_supplement(
	struct vnode    *vp,
	struct vnode    *orig_vp,
	off_t           base_offset,
	vm_address_t    *addr,
	vm_size_t       size,
	struct cs_blob  **ret_blob)
{
	kern_return_t           kr;
	struct ubc_info         *uip, *orig_uip;
	int                     error;
	struct cs_blob          *blob, *orig_blob;
	CS_CodeDirectory const *cd;
	off_t                   blob_start_offset, blob_end_offset;

	if (ret_blob) {
		*ret_blob = NULL;
	}

	                                                                        
                                                   
	error = cs_blob_create_validated(addr, size, &blob, &cd);

	if (error != 0) {
		printf("malformed code signature supplement blob: %d\n", error);
		return error;
	}

	blob->csb_cpu_type = -1;
	blob->csb_base_offset = base_offset;

	blob->csb_reconstituted = false;

	vnode_lock(orig_vp);
	if (!UBCINFOEXISTS(orig_vp)) {
		vnode_unlock(orig_vp);
		error = ENOENT;
		goto out;
	}

	orig_uip = orig_vp->v_ubcinfo;

	                                                                
                     
    

	if (blob->csb_linkage_hashtype == NULL) {
		proc_t p;
		const char *iname = vnode_getname_printable(vp);
		p = current_proc();

		printf("CODE SIGNING: proc %d(%s) supplemental signature for file (%s) "
		    "is not a supplemental.\n",
		    p->p_pid, p->p_comm, iname);

		error = EINVAL;

		vnode_putname_printable(iname);
		vnode_unlock(orig_vp);
		goto out;
	}

	for (orig_blob = orig_uip->cs_blobs; orig_blob != NULL;
	    orig_blob = orig_blob->csb_next) {
		ptrauth_utils_auth_blob_generic(orig_blob->csb_cdhash,
		    sizeof(orig_blob->csb_cdhash),
		    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_cd_signature"),
		    PTRAUTH_ADDR_DIVERSIFY,
		    orig_blob->csb_cdhash_signature);
		if (orig_blob->csb_hashtype == blob->csb_linkage_hashtype &&
		    memcmp(orig_blob->csb_cdhash, blob->csb_linkage, CS_CDHASH_LEN) == 0) {
			               
			break;
		}
	}

	if (orig_blob == NULL) {
		             

		proc_t p;
		const char *iname = vnode_getname_printable(vp);
		p = current_proc();

		printf("CODE SIGNING: proc %d(%s) supplemental signature for file (%s) "
		    "does not match any attached cdhash.\n",
		    p->p_pid, p->p_comm, iname);

		error = ESRCH;

		vnode_putname_printable(iname);
		vnode_unlock(orig_vp);
		goto out;
	}

	vnode_unlock(orig_vp);

	                                         
#if CONFIG_MACF
	unsigned int signer_type = blob->csb_signer_type;
	error = mac_vnode_check_supplemental_signature(vp, blob, orig_vp, orig_blob, &signer_type);
	blob->csb_signer_type = signer_type;


	if (error) {
		if (cs_debug) {
			printf("check_supplemental_signature[pid: %d], error = %d\n", current_proc()->p_pid, error);
		}
		goto out;
	}
#endif

	                                                
	                                                                                                               
	blob->csb_platform_binary = 0;
	blob->csb_platform_path = 0;
	if (orig_blob->csb_platform_binary == 1) {
		blob->csb_platform_binary = orig_blob->csb_platform_binary;
		blob->csb_platform_path = orig_blob->csb_platform_path;
	} else if (orig_blob->csb_teamid != NULL) {
		vm_size_t teamid_size = strlen(orig_blob->csb_teamid) + 1;
		blob->csb_supplement_teamid  = kalloc(teamid_size);
		if (blob->csb_supplement_teamid == NULL) {
			error = ENOMEM;
			goto out;
		}
		strlcpy(blob->csb_supplement_teamid, orig_blob->csb_teamid, teamid_size);
	}
	blob->csb_flags = (orig_blob->csb_flags & CS_DEV_CODE);

	                               
	blob_start_offset = blob->csb_base_offset + blob->csb_start_offset;
	blob_end_offset = blob->csb_base_offset + blob->csb_end_offset;

	if (blob_start_offset >= blob_end_offset || blob_start_offset < 0 || blob_end_offset <= 0) {
		                                    
		error = EINVAL;
		goto out;
	}

	vnode_lock(vp);
	if (!UBCINFOEXISTS(vp)) {
		vnode_unlock(vp);
		error = ENOENT;
		goto out;
	}
	uip = vp->v_ubcinfo;

	struct cs_blob *existing = uip->cs_blob_supplement;
	if (existing != NULL) {
		if (blob->csb_hashtype == existing->csb_hashtype &&
		    memcmp(blob->csb_cdhash, existing->csb_cdhash, CS_CDHASH_LEN) == 0) {
			error = EAGAIN;             
		} else {
			error = EALREADY;         
		}

		vnode_unlock(vp);
		goto out;
	}

	                                                                   
	blob->csb_next = NULL;
	uip->cs_blob_supplement = blob;

	                                                          
	kr = memory_object_signed(uip->ui_control, TRUE);
	if (kr != KERN_SUCCESS) {
		vnode_unlock(vp);
		error = ENOENT;
		goto out;
	}

	vnode_unlock(vp);

	                                                                  
                                    
	ubc_cs_blob_adjust_statistics(blob);

	if (cs_debug > 1) {
		proc_t p;
		const char *name = vnode_getname_printable(vp);
		p = current_proc();
		printf("CODE SIGNING: proc %d(%s) "
		    "loaded supplemental signature for file (%s) "
		    "range 0x%llx:0x%llx\n",
		    p->p_pid, p->p_comm,
		    name,
		    blob->csb_base_offset + blob->csb_start_offset,
		    blob->csb_base_offset + blob->csb_end_offset);
		vnode_putname_printable(name);
	}

	if (ret_blob) {
		*ret_blob = blob;
	}

	error = 0;            
out:
	if (error) {
		if (cs_debug) {
			printf("ubc_cs_blob_add_supplement[pid: %d]: error = %d\n", current_proc()->p_pid, error);
		}

		cs_blob_supplement_free(blob);
	}

	if (error == EAGAIN) {
		                                         
                                             
		error = 0;
	}

	return error;
}
#endif



void
csvnode_print_debug(struct vnode *vp)
{
	const char      *name = NULL;
	struct ubc_info *uip;
	struct cs_blob *blob;

	name = vnode_getname_printable(vp);
	if (name) {
		printf("csvnode: name: %s\n", name);
		vnode_putname_printable(name);
	}

	vnode_lock_spin(vp);

	if (!UBCINFOEXISTS(vp)) {
		blob = NULL;
		goto out;
	}

	uip = vp->v_ubcinfo;
	for (blob = uip->cs_blobs; blob != NULL; blob = blob->csb_next) {
		printf("csvnode: range: %lu -> %lu flags: 0x%08x platform: %s path: %s team: %s\n",
		    (unsigned long)blob->csb_start_offset,
		    (unsigned long)blob->csb_end_offset,
		    blob->csb_flags,
		    blob->csb_platform_binary ? "yes" : "no",
		    blob->csb_platform_path ? "yes" : "no",
		    blob->csb_teamid ? blob->csb_teamid : "<NO-TEAM>");
	}

out:
	vnode_unlock(vp);
}

#if CONFIG_SUPPLEMENTAL_SIGNATURES
struct cs_blob *
ubc_cs_blob_get_supplement(
	struct vnode    *vp,
	off_t           offset)
{
	struct cs_blob *blob;
	off_t offset_in_blob;

	vnode_lock_spin(vp);

	if (!UBCINFOEXISTS(vp)) {
		blob = NULL;
		goto out;
	}

	blob = vp->v_ubcinfo->cs_blob_supplement;

	if (blob == NULL) {
		                       
		goto out;
	}


	if (offset != -1) {
		offset_in_blob = offset - blob->csb_base_offset;
		if (offset_in_blob < blob->csb_start_offset || offset_in_blob >= blob->csb_end_offset) {
			                                    
			blob = NULL;
		}
	}

out:
	vnode_unlock(vp);

	return blob;
}
#endif

struct cs_blob *
ubc_cs_blob_get(
	struct vnode    *vp,
	cpu_type_t      cputype,
	cpu_subtype_t   cpusubtype,
	off_t           offset)
{
	struct ubc_info *uip;
	struct cs_blob  *blob;
	off_t offset_in_blob;

	vnode_lock_spin(vp);

	if (!UBCINFOEXISTS(vp)) {
		blob = NULL;
		goto out;
	}

	uip = vp->v_ubcinfo;
	for (blob = uip->cs_blobs;
	    blob != NULL;
	    blob = blob->csb_next) {
		if (cputype != -1 && blob->csb_cpu_type == cputype && (cpusubtype == -1 || blob->csb_cpu_subtype == (cpusubtype & ~CPU_SUBTYPE_MASK))) {
			break;
		}
		if (offset != -1) {
			offset_in_blob = offset - blob->csb_base_offset;
			if (offset_in_blob >= blob->csb_start_offset &&
			    offset_in_blob < blob->csb_end_offset) {
				                                        
				break;
			}
		}
	}

out:
	vnode_unlock(vp);

	return blob;
}

static void
ubc_cs_free(
	struct ubc_info *uip)
{
	struct cs_blob  *blob, *next_blob;

	for (blob = uip->cs_blobs;
	    blob != NULL;
	    blob = next_blob) {
		next_blob = blob->csb_next;
		os_atomic_add(&cs_blob_count, -1, relaxed);
		os_atomic_add(&cs_blob_size, -blob->csb_mem_size, relaxed);
		cs_blob_free(blob);
	}
#if CHECK_CS_VALIDATION_BITMAP
	ubc_cs_validation_bitmap_deallocate( uip->ui_vnode );
#endif
	uip->cs_blobs = NULL;
#if CONFIG_SUPPLEMENTAL_SIGNATURES
	if (uip->cs_blob_supplement != NULL) {
		blob = uip->cs_blob_supplement;
		os_atomic_add(&cs_blob_count, -1, relaxed);
		os_atomic_add(&cs_blob_size, -blob->csb_mem_size, relaxed);
		cs_blob_supplement_free(uip->cs_blob_supplement);
		uip->cs_blob_supplement = NULL;
	}
#endif
}

                                    
           
                                                          
                                                                        
   
int
ubc_cs_generation_check(
	struct vnode    *vp)
{
	int retval = ENEEDAUTH;

	vnode_lock_spin(vp);

	if (UBCINFOEXISTS(vp) && vp->v_ubcinfo->cs_add_gen == cs_blob_generation_count) {
		retval = 0;
	}

	vnode_unlock(vp);
	return retval;
}

int
ubc_cs_blob_revalidate(
	struct vnode    *vp,
	struct cs_blob *blob,
	struct image_params *imgp,
	int flags,
	uint32_t platform
	)
{
	int error = 0;
	const CS_CodeDirectory *cd = NULL;
	const CS_GenericBlob *entitlements = NULL;
	size_t size;
	assert(vp != NULL);
	assert(blob != NULL);

	size = blob->csb_mem_size;
	error = cs_validate_csblob((const uint8_t *)blob->csb_mem_kaddr,
	    size, &cd, &entitlements);
	if (error) {
		if (cs_debug) {
			printf("CODESIGNING: csblob invalid: %d\n", error);
		}
		goto out;
	}

	unsigned int cs_flags = (ntohl(cd->flags) & CS_ALLOWED_MACHO) | CS_VALID;
	unsigned int signer_type = CS_SIGNER_TYPE_UNKNOWN;

	if (blob->csb_reconstituted) {
		  
                                                             
                                                            
    
                                                                
                                                              
                                                                  
                                                  
     
		if (cs_debug) {
			printf("CODESIGNING: revalidate: not inline revalidating reconstituted signature.\n");
		}

		  
                                                          
                                                            
                                                           
                 
    
                                                         
                                                                
                                                          
                                              
     
		return EAGAIN;
	}

	                                          
#if CONFIG_MACF
	error = mac_vnode_check_signature(vp, blob, imgp, &cs_flags, &signer_type, flags, platform);
	if (cs_debug && error) {
		printf("revalidate: check_signature[pid: %d], error = %d\n", current_proc()->p_pid, error);
	}
#else
	(void)flags;
	(void)signer_type;
#endif

	                                         
	vnode_lock_spin(vp);
	blob->csb_flags = cs_flags;
	blob->csb_signer_type = signer_type;
	if (UBCINFOEXISTS(vp)) {
		if (error == 0) {
			vp->v_ubcinfo->cs_add_gen = cs_blob_generation_count;
		} else {
			vp->v_ubcinfo->cs_add_gen = 0;
		}
	}

	vnode_unlock(vp);

out:
	return error;
}

void
cs_blob_reset_cache()
{
	                                                               
	OSAddAtomic(+2, &cs_blob_generation_count);
	printf("Reseting cs_blob cache from all vnodes. \n");
}

struct cs_blob *
ubc_get_cs_blobs(
	struct vnode    *vp)
{
	struct ubc_info *uip;
	struct cs_blob  *blobs;

	  
                                                                    
                                                                        
                                                                      
                                                                       
         
                                                                      
                                                                         
                                                                     
                                                                      
                          
                                                                      
                            
    

	if (!UBCINFOEXISTS(vp)) {
		blobs = NULL;
		goto out;
	}

	uip = vp->v_ubcinfo;
	blobs = uip->cs_blobs;

out:
	return blobs;
}

#if CONFIG_SUPPLEMENTAL_SIGNATURES
struct cs_blob *
ubc_get_cs_supplement(
	struct vnode    *vp)
{
	struct ubc_info *uip;
	struct cs_blob  *blob;

	  
                                                                    
                                                                        
                                                                      
              
                                                                      
                                                                         
                                                                     
                                                                      
                          
                                                                      
                            
    

	if (!UBCINFOEXISTS(vp)) {
		blob = NULL;
		goto out;
	}

	uip = vp->v_ubcinfo;
	blob = uip->cs_blob_supplement;

out:
	return blob;
}
#endif


void
ubc_get_cs_mtime(
	struct vnode    *vp,
	struct timespec *cs_mtime)
{
	struct ubc_info *uip;

	if (!UBCINFOEXISTS(vp)) {
		cs_mtime->tv_sec = 0;
		cs_mtime->tv_nsec = 0;
		return;
	}

	uip = vp->v_ubcinfo;
	cs_mtime->tv_sec = uip->cs_mtime.tv_sec;
	cs_mtime->tv_nsec = uip->cs_mtime.tv_nsec;
}

unsigned long cs_validate_page_no_hash = 0;
unsigned long cs_validate_page_bad_hash = 0;
static boolean_t
cs_validate_hash(
	struct cs_blob          *blobs,
	memory_object_t         pager,
	memory_object_offset_t  page_offset,
	const void              *data,
	vm_size_t               *bytes_processed,
	unsigned                *tainted)
{
	union cs_hash_union     mdctx;
	struct cs_hash const    *hashtype = NULL;
	unsigned char           actual_hash[CS_HASH_MAX_SIZE];
	unsigned char           expected_hash[CS_HASH_MAX_SIZE];
	boolean_t               found_hash;
	struct cs_blob          *blob;
	const CS_CodeDirectory  *cd;
	const unsigned char     *hash;
	boolean_t               validated;
	off_t                   offset;                              
	size_t                  size;
	off_t                   codeLimit = 0;
	const char              *lower_bound, *upper_bound;
	vm_offset_t             kaddr, blob_addr;

	                                
	found_hash = FALSE;

	for (blob = blobs;
	    blob != NULL;
	    blob = blob->csb_next) {
		offset = page_offset - blob->csb_base_offset;
		if (offset < blob->csb_start_offset ||
		    offset >= blob->csb_end_offset) {
			                                          
			continue;
		}

		                                 
		kaddr = (vm_offset_t)blob->csb_mem_kaddr;
		if (kaddr == 0) {
			continue;
		}

		blob_addr = kaddr + blob->csb_mem_offset;
		lower_bound = CAST_DOWN(char *, blob_addr);
		upper_bound = lower_bound + blob->csb_mem_size;

		cd = blob->csb_cd;
		if (cd != NULL) {
			                                                           

			hashtype = blob->csb_hashtype;
			if (hashtype == NULL) {
				panic("unknown hash type ?");
			}
			if (hashtype->cs_digest_size > sizeof(actual_hash)) {
				panic("hash size too large");
			}
			if (offset & ((1U << blob->csb_hash_pageshift) - 1)) {
				panic("offset not aligned to cshash boundary");
			}

			codeLimit = ntohl(cd->codeLimit);

			hash = hashes(cd, (uint32_t)(offset >> blob->csb_hash_pageshift),
			    hashtype->cs_size,
			    lower_bound, upper_bound);
			if (hash != NULL) {
				bcopy(hash, expected_hash, hashtype->cs_size);
				found_hash = TRUE;
			}

			break;
		}
	}

	if (found_hash == FALSE) {
		  
                                                            
                                                              
                                                             
                     
                                                            
                                                   
     
		cs_validate_page_no_hash++;
		if (cs_debug > 1) {
			printf("CODE SIGNING: cs_validate_page: "
			    "mobj %p off 0x%llx: no hash to validate !?\n",
			    pager, page_offset);
		}
		validated = FALSE;
		*tainted = 0;
	} else {
		*tainted = 0;

		size = (1U << blob->csb_hash_pageshift);
		*bytes_processed = size;

		const uint32_t *asha1, *esha1;
		if ((off_t)(offset + size) > codeLimit) {
			                                    
			assert(offset < codeLimit);
			size = (size_t) (codeLimit & (size - 1));
			*tainted |= CS_VALIDATE_NX;
		}

		hashtype->cs_init(&mdctx);

		if (blob->csb_hash_firstlevel_pageshift) {
			const unsigned char *partial_data = (const unsigned char *)data;
			size_t i;
			for (i = 0; i < size;) {
				union cs_hash_union     partialctx;
				unsigned char partial_digest[CS_HASH_MAX_SIZE];
				size_t partial_size = MIN(size - i, (1U << blob->csb_hash_firstlevel_pageshift));

				hashtype->cs_init(&partialctx);
				hashtype->cs_update(&partialctx, partial_data, partial_size);
				hashtype->cs_final(partial_digest, &partialctx);

				                                        
				hashtype->cs_update(&mdctx, partial_digest, hashtype->cs_size);
				partial_data = partial_data + partial_size;
				i += partial_size;
			}
		} else {
			hashtype->cs_update(&mdctx, data, size);
		}
		hashtype->cs_final(actual_hash, &mdctx);

		asha1 = (const uint32_t *) actual_hash;
		esha1 = (const uint32_t *) expected_hash;

		if (bcmp(expected_hash, actual_hash, hashtype->cs_size) != 0) {
			if (cs_debug) {
				printf("CODE SIGNING: cs_validate_page: "
				    "mobj %p off 0x%llx size 0x%lx: "
				    "actual [0x%x 0x%x 0x%x 0x%x 0x%x] != "
				    "expected [0x%x 0x%x 0x%x 0x%x 0x%x]\n",
				    pager, page_offset, size,
				    asha1[0], asha1[1], asha1[2],
				    asha1[3], asha1[4],
				    esha1[0], esha1[1], esha1[2],
				    esha1[3], esha1[4]);
			}
			cs_validate_page_bad_hash++;
			*tainted |= CS_VALIDATE_TAINTED;
		} else {
			if (cs_debug > 10) {
				printf("CODE SIGNING: cs_validate_page: "
				    "mobj %p off 0x%llx size 0x%lx: "
				    "SHA1 OK\n",
				    pager, page_offset, size);
			}
		}
		validated = TRUE;
	}

	return validated;
}

boolean_t
cs_validate_range(
	struct vnode    *vp,
	memory_object_t         pager,
	memory_object_offset_t  page_offset,
	const void              *data,
	vm_size_t               dsize,
	unsigned                *tainted)
{
	vm_size_t offset_in_range;
	boolean_t all_subranges_validated = TRUE;                                       

	struct cs_blob *blobs = ubc_get_cs_blobs(vp);

#if CONFIG_SUPPLEMENTAL_SIGNATURES
	if (blobs == NULL && proc_is_translated(current_proc())) {
		struct cs_blob *supp = ubc_get_cs_supplement(vp);

		if (supp != NULL) {
			blobs = supp;
		} else {
			return FALSE;
		}
	}
#endif



	*tainted = 0;

	for (offset_in_range = 0;
	    offset_in_range < dsize;
	                                                          ) {
		unsigned subrange_tainted = 0;
		boolean_t subrange_validated;
		vm_size_t bytes_processed = 0;

		subrange_validated = cs_validate_hash(blobs,
		    pager,
		    page_offset + offset_in_range,
		    (const void *)((const char *)data + offset_in_range),
		    &bytes_processed,
		    &subrange_tainted);

		*tainted |= subrange_tainted;

		if (bytes_processed == 0) {
			                                                       
			all_subranges_validated = FALSE;
			break;
		} else if (subrange_validated == FALSE) {
			all_subranges_validated = FALSE;
			                                                               
		}

		offset_in_range += bytes_processed;
	}

	return all_subranges_validated;
}

void
cs_validate_page(
	struct vnode            *vp,
	memory_object_t         pager,
	memory_object_offset_t  page_offset,
	const void              *data,
	int                     *validated_p,
	int                     *tainted_p,
	int                     *nx_p)
{
	vm_size_t offset_in_page;
	struct cs_blob *blobs;

	blobs = ubc_get_cs_blobs(vp);

#if CONFIG_SUPPLEMENTAL_SIGNATURES
	if (blobs == NULL && proc_is_translated(current_proc())) {
		struct cs_blob *supp = ubc_get_cs_supplement(vp);

		if (supp != NULL) {
			blobs = supp;
		}
	}
#endif

	*validated_p = VMP_CS_ALL_FALSE;
	*tainted_p = VMP_CS_ALL_FALSE;
	*nx_p = VMP_CS_ALL_FALSE;

	for (offset_in_page = 0;
	    offset_in_page < PAGE_SIZE;
	                                                         ) {
		unsigned subrange_tainted = 0;
		boolean_t subrange_validated;
		vm_size_t bytes_processed = 0;
		int sub_bit;

		subrange_validated = cs_validate_hash(blobs,
		    pager,
		    page_offset + offset_in_page,
		    (const void *)((const char *)data + offset_in_page),
		    &bytes_processed,
		    &subrange_tainted);

		if (bytes_processed == 0) {
			                                            
			offset_in_page += FOURK_PAGE_SIZE;
			continue;
		}
		if (offset_in_page == 0 &&
		    bytes_processed > PAGE_SIZE - FOURK_PAGE_SIZE) {
			                                      
			if (subrange_validated) {
				*validated_p = VMP_CS_ALL_TRUE;
			}
			if (subrange_tainted & CS_VALIDATE_TAINTED) {
				*tainted_p = VMP_CS_ALL_TRUE;
			}
			if (subrange_tainted & CS_VALIDATE_NX) {
				*nx_p = VMP_CS_ALL_TRUE;
			}
			break;
		}
		                                                          
		assertf(bytes_processed <= FOURK_PAGE_SIZE,
		    "vp %p blobs %p offset 0x%llx + 0x%llx bytes_processed 0x%llx\n",
		    vp, blobs, (uint64_t)page_offset,
		    (uint64_t)offset_in_page, (uint64_t)bytes_processed);
		sub_bit = 1 << (offset_in_page >> FOURK_PAGE_SHIFT);
		if (subrange_validated) {
			*validated_p |= sub_bit;
		}
		if (subrange_tainted & CS_VALIDATE_TAINTED) {
			*tainted_p |= sub_bit;
		}
		if (subrange_tainted & CS_VALIDATE_NX) {
			*nx_p |= sub_bit;
		}
		                         
		offset_in_page += FOURK_PAGE_SIZE;
	}

	return;
}

int
ubc_cs_getcdhash(
	vnode_t         vp,
	off_t           offset,
	unsigned char   *cdhash)
{
	struct cs_blob  *blobs, *blob;
	off_t           rel_offset;
	int             ret;

	vnode_lock(vp);

	blobs = ubc_get_cs_blobs(vp);
	for (blob = blobs;
	    blob != NULL;
	    blob = blob->csb_next) {
		                                          
		rel_offset = offset - blob->csb_base_offset;
		if (rel_offset >= blob->csb_start_offset &&
		    rel_offset < blob->csb_end_offset) {
			                                         
			break;
		}
	}

	if (blob == NULL) {
		                                             
		ret = EBADEXEC;                             
	} else {
		                                    
		ptrauth_utils_auth_blob_generic(blob->csb_cdhash,
		    sizeof(blob->csb_cdhash),
		    OS_PTRAUTH_DISCRIMINATOR("cs_blob.csb_cd_signature"),
		    PTRAUTH_ADDR_DIVERSIFY,
		    blob->csb_cdhash_signature);
		bcopy(blob->csb_cdhash, cdhash, sizeof(blob->csb_cdhash));
		ret = 0;
	}

	vnode_unlock(vp);

	return ret;
}

boolean_t
ubc_cs_is_range_codesigned(
	vnode_t                 vp,
	mach_vm_offset_t        start,
	mach_vm_size_t          size)
{
	struct cs_blob          *csblob;
	mach_vm_offset_t        blob_start;
	mach_vm_offset_t        blob_end;

	if (vp == NULL) {
		                                
		return FALSE;
	}
	if (size == 0) {
		                                 
		return FALSE;
	}
	if (start + size < start) {
		              
		return FALSE;
	}

	csblob = ubc_cs_blob_get(vp, -1, -1, start);
	if (csblob == NULL) {
		return FALSE;
	}

	  
                                                                
                                                              
                                                               
                                                                    
    
	blob_start = (mach_vm_offset_t) (csblob->csb_base_offset +
	    csblob->csb_start_offset);
	blob_end = (mach_vm_offset_t) (csblob->csb_base_offset +
	    csblob->csb_end_offset);
	if (blob_start > start || blob_end < (start + size)) {
		                                                       
		return FALSE;
	}

	return TRUE;
}

#if CHECK_CS_VALIDATION_BITMAP
#define stob(s) (((atop_64(round_page_64(s))) + 07) >> 3)
extern  boolean_t       root_fs_upgrade_try;

  
                                                                             
           
                                                 
                                                                
                                                                 
   
#define USE_CODE_SIGN_BITMAP(vp)        ( (vp != NULL) && (vp->v_mount != NULL) && (vp->v_mount->mnt_flag & MNT_ROOTFS) && !root_fs_upgrade_try)
kern_return_t
ubc_cs_validation_bitmap_allocate(
	vnode_t         vp)
{
	kern_return_t   kr = KERN_SUCCESS;
	struct ubc_info *uip;
	char            *target_bitmap;
	vm_object_size_t        bitmap_size;

	if (!USE_CODE_SIGN_BITMAP(vp) || (!UBCINFOEXISTS(vp))) {
		kr = KERN_INVALID_ARGUMENT;
	} else {
		uip = vp->v_ubcinfo;

		if (uip->cs_valid_bitmap == NULL) {
			bitmap_size = stob(uip->ui_size);
			target_bitmap = (char*) kalloc((vm_size_t)bitmap_size );
			if (target_bitmap == 0) {
				kr = KERN_NO_SPACE;
			} else {
				kr = KERN_SUCCESS;
			}
			if (kr == KERN_SUCCESS) {
				memset( target_bitmap, 0, (size_t)bitmap_size);
				uip->cs_valid_bitmap = (void*)target_bitmap;
				uip->cs_valid_bitmap_size = bitmap_size;
			}
		}
	}
	return kr;
}

kern_return_t
ubc_cs_check_validation_bitmap(
	vnode_t                 vp,
	memory_object_offset_t          offset,
	int                     optype)
{
	kern_return_t   kr = KERN_SUCCESS;

	if (!USE_CODE_SIGN_BITMAP(vp) || !UBCINFOEXISTS(vp)) {
		kr = KERN_INVALID_ARGUMENT;
	} else {
		struct ubc_info *uip = vp->v_ubcinfo;
		char            *target_bitmap = uip->cs_valid_bitmap;

		if (target_bitmap == NULL) {
			kr = KERN_INVALID_ARGUMENT;
		} else {
			uint64_t        bit, byte;
			bit = atop_64( offset );
			byte = bit >> 3;

			if (byte > uip->cs_valid_bitmap_size) {
				kr = KERN_INVALID_ARGUMENT;
			} else {
				if (optype == CS_BITMAP_SET) {
					target_bitmap[byte] |= (1 << (bit & 07));
					kr = KERN_SUCCESS;
				} else if (optype == CS_BITMAP_CLEAR) {
					target_bitmap[byte] &= ~(1 << (bit & 07));
					kr = KERN_SUCCESS;
				} else if (optype == CS_BITMAP_CHECK) {
					if (target_bitmap[byte] & (1 << (bit & 07))) {
						kr = KERN_SUCCESS;
					} else {
						kr = KERN_FAILURE;
					}
				}
			}
		}
	}
	return kr;
}

void
ubc_cs_validation_bitmap_deallocate(
	vnode_t         vp)
{
	struct ubc_info *uip;
	void            *target_bitmap;
	vm_object_size_t        bitmap_size;

	if (UBCINFOEXISTS(vp)) {
		uip = vp->v_ubcinfo;

		if ((target_bitmap = uip->cs_valid_bitmap) != NULL) {
			bitmap_size = uip->cs_valid_bitmap_size;
			kfree( target_bitmap, (vm_size_t) bitmap_size );
			uip->cs_valid_bitmap = NULL;
		}
	}
}
#else
kern_return_t
ubc_cs_validation_bitmap_allocate(__unused vnode_t vp)
{
	return KERN_INVALID_ARGUMENT;
}

kern_return_t
ubc_cs_check_validation_bitmap(
	__unused struct vnode *vp,
	__unused memory_object_offset_t offset,
	__unused int optype)
{
	return KERN_INVALID_ARGUMENT;
}

void
ubc_cs_validation_bitmap_deallocate(__unused vnode_t vp)
{
	return;
}
#endif                                 

