/*
 * Copyright (c) 2008-2018 Apple Inc. All rights reserved.
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
#if !FS_COMPRESSION

                                                                 

#define UNUSED_SYMBOL(x)        asm(".global _" #x "\n.set _" #x ", 0\n");

UNUSED_SYMBOL(register_decmpfs_decompressor)
UNUSED_SYMBOL(unregister_decmpfs_decompressor)
UNUSED_SYMBOL(decmpfs_init)
UNUSED_SYMBOL(decmpfs_read_compressed)
UNUSED_SYMBOL(decmpfs_cnode_cmp_type)
UNUSED_SYMBOL(decmpfs_cnode_get_vnode_state)
UNUSED_SYMBOL(decmpfs_cnode_get_vnode_cached_size)
UNUSED_SYMBOL(decmpfs_cnode_get_vnode_cached_nchildren)
UNUSED_SYMBOL(decmpfs_cnode_get_vnode_cached_total_size)
UNUSED_SYMBOL(decmpfs_lock_compressed_data)
UNUSED_SYMBOL(decmpfs_cnode_free)
UNUSED_SYMBOL(decmpfs_cnode_alloc)
UNUSED_SYMBOL(decmpfs_cnode_destroy)
UNUSED_SYMBOL(decmpfs_decompress_file)
UNUSED_SYMBOL(decmpfs_unlock_compressed_data)
UNUSED_SYMBOL(decmpfs_cnode_init)
UNUSED_SYMBOL(decmpfs_cnode_set_vnode_state)
UNUSED_SYMBOL(decmpfs_hides_xattr)
UNUSED_SYMBOL(decmpfs_ctx)
UNUSED_SYMBOL(decmpfs_file_is_compressed)
UNUSED_SYMBOL(decmpfs_update_attributes)
UNUSED_SYMBOL(decmpfs_hides_rsrc)
UNUSED_SYMBOL(decmpfs_pagein_compressed)
UNUSED_SYMBOL(decmpfs_validate_compressed_file)

#else                     
#include <sys/kernel.h>
#include <sys/vnode_internal.h>
#include <sys/file_internal.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/xattr.h>
#include <sys/namei.h>
#include <sys/user.h>
#include <sys/mount_internal.h>
#include <sys/ubc.h>
#include <sys/decmpfs.h>
#include <sys/uio_internal.h>
#include <libkern/OSByteOrder.h>
#include <libkern/section_keywords.h>

#include <ptrauth.h>

#pragma mark --- debugging ---

#define COMPRESSION_DEBUG 0
#define COMPRESSION_DEBUG_VERBOSE 0
#define MALLOC_DEBUG 0

#if COMPRESSION_DEBUG
static char*
vnpath(vnode_t vp, char *path, int len)
{
	int origlen = len;
	path[0] = 0;
	vn_getpath(vp, path, &len);
	path[origlen - 1] = 0;
	return path;
}
#endif

#define ErrorLog(x, args...) \
	printf("%s:%d:%s: " x, __FILE_NAME__, __LINE__, __FUNCTION__, ## args)
#if COMPRESSION_DEBUG
#define ErrorLogWithPath(x, args...) do { \
	char *path = zalloc(ZV_NAMEI); \
	printf("%s:%d:%s: %s: " x, __FILE_NAME__, __LINE__, __FUNCTION__, \
	    vnpath(vp, path, PATH_MAX), ## args); \
	zfree(ZV_NAMEI, path); \
} while(0)
#else
#define ErrorLogWithPath(x, args...) do { \
	(void*)vp; \
	printf("%s:%d:%s: %s: " x, __FILE_NAME__, __LINE__, __FUNCTION__, \
	    "<private>", ## args); \
} while(0)
#endif

#if COMPRESSION_DEBUG
#define DebugLog ErrorLog
#define DebugLogWithPath ErrorLogWithPath
#else
#define DebugLog(x...) do { } while(0)
#define DebugLogWithPath(x...) do { } while(0)
#endif

#if COMPRESSION_DEBUG_VERBOSE
#define VerboseLog ErrorLog
#define VerboseLogWithPath ErrorLogWithPath
#else
#define VerboseLog(x...) do { } while(0)
#define VerboseLogWithPath(x...) do { } while(0)
#endif

#pragma mark --- globals ---

static LCK_GRP_DECLARE(decmpfs_lockgrp, "VFSCOMP");
static LCK_RW_DECLARE(decompressorsLock, &decmpfs_lockgrp);
static LCK_MTX_DECLARE(decompress_channel_mtx, &decmpfs_lockgrp);

static const decmpfs_registration *decompressors[CMP_MAX];                                 
static int decompress_channel;                                                         

vfs_context_t decmpfs_ctx;

#pragma mark --- decmp_get_func ---

#define offsetof_func(func) ((uintptr_t)offsetof(decmpfs_registration, func))

static void *
_func_from_offset(uint32_t type, uintptr_t offset, uint32_t discriminator)
{
	                                                                                 
	const decmpfs_registration *reg = decompressors[type];

	switch (reg->decmpfs_registration) {
	case DECMPFS_REGISTRATION_VERSION_V1:
		if (offset > offsetof_func(free_data)) {
			return NULL;
		}
		break;
	case DECMPFS_REGISTRATION_VERSION_V3:
		if (offset > offsetof_func(get_flags)) {
			return NULL;
		}
		break;
	default:
		return NULL;
	}

	void *ptr = *(void * const *)((const void *)reg + offset);
	if (ptr != NULL) {
		                                   
		ptr = ptrauth_auth_and_resign(ptr, ptrauth_key_asia, discriminator, ptrauth_key_asia, 0);
	}
	return ptr;
}

extern void IOServicePublishResource( const char * property, boolean_t value );
extern boolean_t IOServiceWaitForMatchingResource( const char * property, uint64_t timeout );
extern boolean_t IOCatalogueMatchingDriversPresent( const char * property );

static void *
_decmp_get_func(vnode_t vp, uint32_t type, uintptr_t offset, uint32_t discriminator)
{
	  
                                                                                     
                                       
    

	if (type >= CMP_MAX) {
		return NULL;
	}

	if (decompressors[type] != NULL) {
		                                                                       
		return _func_from_offset(type, offset, discriminator);
	}

	                                                                      
	char providesName[80];
	snprintf(providesName, sizeof(providesName), "com.apple.AppleFSCompression.providesType%u", type);
	if (IOCatalogueMatchingDriversPresent(providesName)) {
		                                                                                 
		char resourceName[80];
		uint64_t delay = 10000000ULL;                    
		snprintf(resourceName, sizeof(resourceName), "com.apple.AppleFSCompression.Type%u", type);
		ErrorLogWithPath("waiting for %s\n", resourceName);
		while (decompressors[type] == NULL) {
			lck_rw_unlock_shared(&decompressorsLock);                                                   
			if (IOServiceWaitForMatchingResource(resourceName, delay)) {
				lck_rw_lock_shared(&decompressorsLock);
				break;
			}
			if (!IOCatalogueMatchingDriversPresent(providesName)) {
				  
				ErrorLogWithPath("the kext with %s is no longer present\n", providesName);
				lck_rw_lock_shared(&decompressorsLock);
				break;
			}
			ErrorLogWithPath("still waiting for %s\n", resourceName);
			delay *= 2;
			lck_rw_lock_shared(&decompressorsLock);
		}
		                                                                 
		if (decompressors[type] == NULL) {
			ErrorLogWithPath("we found %s, but the type still isn't registered\n", providesName);
			return NULL;
		}
		                                                    
		return _func_from_offset(type, offset, discriminator);
	}

	                                                                                          
	ErrorLogWithPath("tried to access a compressed file of unregistered type %d\n", type);
	return NULL;
}

#define decmp_get_func(vp, type, func) (typeof(decompressors[0]->func))_decmp_get_func(vp, type, offsetof_func(func), ptrauth_function_pointer_type_discriminator(typeof(decompressors[0]->func)))

#pragma mark --- utilities ---

#if COMPRESSION_DEBUG
static int
vnsize(vnode_t vp, uint64_t *size)
{
	struct vnode_attr va;
	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_data_size);
	int error = vnode_getattr(vp, &va, decmpfs_ctx);
	if (error != 0) {
		ErrorLogWithPath("vnode_getattr err %d\n", error);
		return error;
	}
	*size = va.va_data_size;
	return 0;
}
#endif                        

#pragma mark --- cnode routines ---

ZONE_DECLARE(decmpfs_cnode_zone, "decmpfs_cnode",
    sizeof(struct decmpfs_cnode), ZC_NONE);

decmpfs_cnode *
decmpfs_cnode_alloc(void)
{
	return zalloc(decmpfs_cnode_zone);
}

void
decmpfs_cnode_free(decmpfs_cnode *dp)
{
	zfree(decmpfs_cnode_zone, dp);
}

void
decmpfs_cnode_init(decmpfs_cnode *cp)
{
	memset(cp, 0, sizeof(*cp));
	lck_rw_init(&cp->compressed_data_lock, &decmpfs_lockgrp, NULL);
}

void
decmpfs_cnode_destroy(decmpfs_cnode *cp)
{
	lck_rw_destroy(&cp->compressed_data_lock, &decmpfs_lockgrp);
}

bool
decmpfs_trylock_compressed_data(decmpfs_cnode *cp, int exclusive)
{
	void *thread = current_thread();
	bool retval = false;

	if (cp->lockowner == thread) {
		                                                                         
		cp->lockcount++;
		retval = true;
	} else if (exclusive) {
		if ((retval = lck_rw_try_lock_exclusive(&cp->compressed_data_lock))) {
			cp->lockowner = thread;
			cp->lockcount = 1;
		}
	} else {
		if ((retval = lck_rw_try_lock_shared(&cp->compressed_data_lock))) {
			cp->lockowner = (void *)-1;
		}
	}
	return retval;
}

void
decmpfs_lock_compressed_data(decmpfs_cnode *cp, int exclusive)
{
	void *thread = current_thread();

	if (cp->lockowner == thread) {
		                                                                         
		cp->lockcount++;
	} else if (exclusive) {
		lck_rw_lock_exclusive(&cp->compressed_data_lock);
		cp->lockowner = thread;
		cp->lockcount = 1;
	} else {
		lck_rw_lock_shared(&cp->compressed_data_lock);
		cp->lockowner = (void *)-1;
	}
}

void
decmpfs_unlock_compressed_data(decmpfs_cnode *cp, __unused int exclusive)
{
	void *thread = current_thread();

	if (cp->lockowner == thread) {
		                                                                      
		if ((--cp->lockcount) > 0) {
			                                                           
			return;
		}
		cp->lockowner = NULL;
	}

	lck_rw_done(&cp->compressed_data_lock);
}

uint32_t
decmpfs_cnode_get_vnode_state(decmpfs_cnode *cp)
{
	return cp->cmp_state;
}

void
decmpfs_cnode_set_vnode_state(decmpfs_cnode *cp, uint32_t state, int skiplock)
{
	if (!skiplock) {
		decmpfs_lock_compressed_data(cp, 1);
	}
	cp->cmp_state = (uint8_t)state;
	if (state == FILE_TYPE_UNKNOWN) {
		                                        
		cp->cmp_type = 0;
	}
	if (!skiplock) {
		decmpfs_unlock_compressed_data(cp, 1);
	}
}

static void
decmpfs_cnode_set_vnode_cmp_type(decmpfs_cnode *cp, uint32_t cmp_type, int skiplock)
{
	if (!skiplock) {
		decmpfs_lock_compressed_data(cp, 1);
	}
	cp->cmp_type = cmp_type;
	if (!skiplock) {
		decmpfs_unlock_compressed_data(cp, 1);
	}
}

static void
decmpfs_cnode_set_vnode_minimal_xattr(decmpfs_cnode *cp, int minimal_xattr, int skiplock)
{
	if (!skiplock) {
		decmpfs_lock_compressed_data(cp, 1);
	}
	cp->cmp_minimal_xattr = !!minimal_xattr;
	if (!skiplock) {
		decmpfs_unlock_compressed_data(cp, 1);
	}
}

uint64_t
decmpfs_cnode_get_vnode_cached_size(decmpfs_cnode *cp)
{
	return cp->uncompressed_size;
}

uint64_t
decmpfs_cnode_get_vnode_cached_nchildren(decmpfs_cnode *cp)
{
	return cp->nchildren;
}

uint64_t
decmpfs_cnode_get_vnode_cached_total_size(decmpfs_cnode *cp)
{
	return cp->total_size;
}

void
decmpfs_cnode_set_vnode_cached_size(decmpfs_cnode *cp, uint64_t size)
{
	while (1) {
		uint64_t old = cp->uncompressed_size;
		if (OSCompareAndSwap64(old, size, (UInt64*)&cp->uncompressed_size)) {
			return;
		} else {
			                                        
		}
	}
}

void
decmpfs_cnode_set_vnode_cached_nchildren(decmpfs_cnode *cp, uint64_t nchildren)
{
	while (1) {
		uint64_t old = cp->nchildren;
		if (OSCompareAndSwap64(old, nchildren, (UInt64*)&cp->nchildren)) {
			return;
		} else {
			                                        
		}
	}
}

void
decmpfs_cnode_set_vnode_cached_total_size(decmpfs_cnode *cp, uint64_t total_sz)
{
	while (1) {
		uint64_t old = cp->total_size;
		if (OSCompareAndSwap64(old, total_sz, (UInt64*)&cp->total_size)) {
			return;
		} else {
			                                        
		}
	}
}

static uint64_t
decmpfs_cnode_get_decompression_flags(decmpfs_cnode *cp)
{
	return cp->decompression_flags;
}

static void
decmpfs_cnode_set_decompression_flags(decmpfs_cnode *cp, uint64_t flags)
{
	while (1) {
		uint64_t old = cp->decompression_flags;
		if (OSCompareAndSwap64(old, flags, (UInt64*)&cp->decompression_flags)) {
			return;
		} else {
			                                        
		}
	}
}

uint32_t
decmpfs_cnode_cmp_type(decmpfs_cnode *cp)
{
	return cp->cmp_type;
}

#pragma mark --- decmpfs state routines ---

static int
decmpfs_fetch_compressed_header(vnode_t vp, decmpfs_cnode *cp, decmpfs_header **hdrOut, int returnInvalid, size_t *hdr_size)
{
	  
                                                                                            
                                                                                           
                                   
    

	size_t read_size             = 0;
	size_t attr_size             = 0;
	size_t alloc_size            = 0;
	uio_t attr_uio               = NULL;
	int err                      = 0;
	char *data                   = NULL;
	const bool no_additional_data = ((cp != NULL)
	    && (cp->cmp_type != 0)
	    && (cp->cmp_minimal_xattr != 0));
	char uio_buf[UIO_SIZEOF(1)];
	decmpfs_header *hdr = NULL;

	  
                                                                    
   
                                                                   
                                                                           
                                                                            
    
	DECMPFS_EMIT_TRACE_ENTRY(DECMPDBG_FETCH_COMPRESSED_HEADER, vp->v_id,
	    no_additional_data, returnInvalid);

	if (no_additional_data) {
		                                                                                                                               

		alloc_size = sizeof(decmpfs_header);
		data = kheap_alloc(KHEAP_TEMP, alloc_size, Z_WAITOK);
		if (!data) {
			err = ENOMEM;
			goto out;
		}
		hdr = (decmpfs_header*)data;
		hdr->attr_size = sizeof(decmpfs_disk_header);
		hdr->compression_magic = DECMPFS_MAGIC;
		hdr->compression_type  = cp->cmp_type;
		if (hdr->compression_type == DATALESS_PKG_CMPFS_TYPE) {
			if (!vnode_isdir(vp)) {
				err = EINVAL;
				goto out;
			}
			hdr->_size.value = DECMPFS_PKG_VALUE_FROM_SIZE_COUNT(
				decmpfs_cnode_get_vnode_cached_size(cp),
				decmpfs_cnode_get_vnode_cached_nchildren(cp));
		} else if (vnode_isdir(vp)) {
			hdr->_size.value = decmpfs_cnode_get_vnode_cached_nchildren(cp);
		} else {
			hdr->_size.value = decmpfs_cnode_get_vnode_cached_size(cp);
		}
	} else {
		                                             
		err = vn_getxattr(vp, DECMPFS_XATTR_NAME, NULL, &attr_size, XATTR_NOSECURITY, decmpfs_ctx);
		if (err != 0) {
			goto out;
		}
		alloc_size = attr_size + sizeof(hdr->attr_size);

		if (attr_size < sizeof(decmpfs_disk_header) || attr_size > MAX_DECMPFS_XATTR_SIZE) {
			err = EINVAL;
			goto out;
		}

		                                                                                    
		data = kheap_alloc(KHEAP_TEMP, alloc_size, Z_WAITOK);
		if (!data) {
			err = ENOMEM;
			goto out;
		}

		                                                                                        
		attr_uio = uio_createwithbuffer(1, 0, UIO_SYSSPACE, UIO_READ, &uio_buf[0], sizeof(uio_buf));
		uio_addiov(attr_uio, CAST_USER_ADDR_T(data + sizeof(hdr->attr_size)), attr_size);

		err = vn_getxattr(vp, DECMPFS_XATTR_NAME, attr_uio, &read_size, XATTR_NOSECURITY, decmpfs_ctx);
		if (err != 0) {
			goto out;
		}
		if (read_size != attr_size) {
			err = EINVAL;
			goto out;
		}
		hdr = (decmpfs_header*)data;
		hdr->attr_size = (uint32_t)attr_size;
		                                      
		hdr->compression_magic = OSSwapLittleToHostInt32(hdr->compression_magic);
		hdr->compression_type  = OSSwapLittleToHostInt32(hdr->compression_type);
		hdr->uncompressed_size = OSSwapLittleToHostInt64(hdr->uncompressed_size);
	}

	if (hdr->compression_magic != DECMPFS_MAGIC) {
		ErrorLogWithPath("invalid compression_magic 0x%08x, should be 0x%08x\n", hdr->compression_magic, DECMPFS_MAGIC);
		err = EINVAL;
		goto out;
	}

	  
                                                                    
                                                                 
                                                                  
                                                               
                            
    
	if (hdr->compression_type >= CMP_MAX && !decmpfs_type_is_dataless(hdr->compression_type)) {
		if (returnInvalid) {
			                                                            
			err = ERANGE;
		} else {
			ErrorLogWithPath("compression_type %d out of range\n", hdr->compression_type);
			err = EINVAL;
		}
		goto out;
	}

out:
	if (err && (err != ERANGE)) {
		DebugLogWithPath("err %d\n", err);
		kheap_free(KHEAP_TEMP, data, alloc_size);
		*hdrOut = NULL;
	} else {
		*hdrOut = hdr;
		*hdr_size = alloc_size;
	}
	  
                                                                      
   
                                                                   
                                                       
    
	DECMPFS_EMIT_TRACE_RETURN(DECMPDBG_FETCH_COMPRESSED_HEADER, vp->v_id, err);
	return err;
}

static int
decmpfs_fast_get_state(decmpfs_cnode *cp)
{
	  
                            
                                                                                                       
                                                        
    
	int cmp_state = decmpfs_cnode_get_vnode_state(cp);

	switch (cmp_state) {
	case FILE_IS_NOT_COMPRESSED:
	case FILE_IS_COMPRESSED:
	case FILE_IS_CONVERTING:
		return cmp_state;
	case FILE_TYPE_UNKNOWN:
		  
                                                                                                 
                                  
     
		ErrorLog("decmpfs_fast_get_state called on unknown file\n");
		return FILE_IS_NOT_COMPRESSED;
	default:
		     
		ErrorLog("unknown cmp_state %d\n", cmp_state);
		return FILE_IS_NOT_COMPRESSED;
	}
}

static int
decmpfs_fast_file_is_compressed(decmpfs_cnode *cp)
{
	int cmp_state = decmpfs_cnode_get_vnode_state(cp);

	switch (cmp_state) {
	case FILE_IS_NOT_COMPRESSED:
		return 0;
	case FILE_IS_COMPRESSED:
	case FILE_IS_CONVERTING:
		return 1;
	case FILE_TYPE_UNKNOWN:
		  
                                                                                                 
                                  
     
		ErrorLog("decmpfs_fast_get_state called on unknown file\n");
		return 0;
	default:
		     
		ErrorLog("unknown cmp_state %d\n", cmp_state);
		return 0;
	}
}

errno_t
decmpfs_validate_compressed_file(vnode_t vp, decmpfs_cnode *cp)
{
	                                                                              
	decmpfs_header *hdr = NULL;
	size_t alloc_size = 0;
	errno_t err = decmpfs_fetch_compressed_header(vp, cp, &hdr, 0, &alloc_size);

	if (err) {
		                                
		if (decmpfs_fast_get_state(cp) == FILE_IS_NOT_COMPRESSED) {
			                                                         
			err = 0;
		}
		goto out;
	}

	if (!decmpfs_type_is_dataless(hdr->compression_type)) {
		lck_rw_lock_shared(&decompressorsLock);
		decmpfs_validate_compressed_file_func validate = decmp_get_func(vp, hdr->compression_type, validate);
		if (validate) {                                                  
			                       
			err = validate(vp, decmpfs_ctx, hdr);
		} else if (decmp_get_func(vp, hdr->compression_type, fetch) == NULL) {
			                               
			err = EIO;
		} else {
			                                              
			err = 0;
		}
		lck_rw_unlock_shared(&decompressorsLock);
	}
out:
	if (hdr != NULL) {
		kheap_free(KHEAP_TEMP, hdr, alloc_size);
	}
#if COMPRESSION_DEBUG
	if (err) {
		DebugLogWithPath("decmpfs_validate_compressed_file ret %d, vp->v_flag %d\n", err, vp->v_flag);
	}
#endif
	return err;
}

int
decmpfs_file_is_compressed(vnode_t vp, decmpfs_cnode *cp)
{
	  
                                                      
   
                                                                                               
                                                                 
   
    

	int ret = 0;
	int error = 0;
	uint32_t cmp_state;
	struct vnode_attr va_fetch;
	decmpfs_header *hdr = NULL;
	size_t alloc_size = 0;
	mount_t mp = NULL;
	int cnode_locked = 0;
	int saveInvalid = 0;                                                              
	uint64_t decompression_flags = 0;
	bool is_mounted, is_local_fs;

	if (vnode_isnamedstream(vp)) {
		  
                                       
                                                                
                                                                    
     
		return 0;
	}

	                                              
	cmp_state = decmpfs_cnode_get_vnode_state(cp);
	switch (cmp_state) {
	case FILE_IS_NOT_COMPRESSED:
		return 0;
	case FILE_IS_COMPRESSED:
		return 1;
	case FILE_IS_CONVERTING:
		                                                                                                                 
		return 1;
	case FILE_TYPE_UNKNOWN:
		                                                                          
		break;
	default:
		                                                  
		ErrorLogWithPath("unknown cmp_state %d\n", cmp_state);
		return 0;
	}

	is_mounted = false;
	is_local_fs = false;
	mp = vnode_mount(vp);
	if (mp) {
		is_mounted = true;
	}
	if (is_mounted) {
		is_local_fs = ((mp->mnt_flag & MNT_LOCAL));
	}
	  
                                                                     
   
                                                        
                                                                
                                                            
    
	DECMPFS_EMIT_TRACE_ENTRY(DECMPDBG_FILE_IS_COMPRESSED, vp->v_id,
	    is_mounted, is_local_fs);

	if (!is_mounted) {
		  
                                                                  
                                                                        
                                      
     
		ret = FILE_IS_NOT_COMPRESSED;
		goto done;
	}

	if (!is_local_fs) {
		                                                     
		ret = FILE_IS_NOT_COMPRESSED;
		goto done;
	}

	                                                                                  
	decmpfs_lock_compressed_data(cp, 1);
	cnode_locked = 1;

	VATTR_INIT(&va_fetch);
	VATTR_WANTED(&va_fetch, va_flags);
	error = vnode_getattr(vp, &va_fetch, decmpfs_ctx);
	if (error) {
		                                                               
		ret = FILE_IS_NOT_COMPRESSED;
		goto done;
	}
	if (va_fetch.va_flags & UF_COMPRESSED) {
		                                                                              
		error = decmpfs_fetch_compressed_header(vp, cp, &hdr, 1, &alloc_size);
		if ((hdr != NULL) && (error == ERANGE)) {
			saveInvalid = 1;
		}
		if (error) {
			                                                           
			ret = FILE_IS_NOT_COMPRESSED;
			goto done;
		}
		  
                                                                 
                                                                     
                                                 
     
		if (!vnode_isreg(vp) &&
		    !(decmpfs_type_is_dataless(hdr->compression_type) && vnode_isdir(vp))) {
			ret = FILE_IS_NOT_COMPRESSED;
			goto done;
		}
		ret = FILE_IS_COMPRESSED;
		goto done;
	}
	                                                          
	ret = FILE_IS_NOT_COMPRESSED;

done:
	if (((ret == FILE_IS_COMPRESSED) || saveInvalid) && hdr) {
		  
                                                   
     

		if (!cnode_locked) {
			  
                                                                                     
                                                              
      
			decmpfs_lock_compressed_data(cp, 1);
			cnode_locked = 1;
		}

		if (vnode_isdir(vp)) {
			decmpfs_cnode_set_vnode_cached_size(cp, 64);
			decmpfs_cnode_set_vnode_cached_nchildren(cp, decmpfs_get_directory_entries(hdr));
			if (hdr->compression_type == DATALESS_PKG_CMPFS_TYPE) {
				decmpfs_cnode_set_vnode_cached_total_size(cp, DECMPFS_PKG_SIZE(hdr->_size));
			}
		} else {
			decmpfs_cnode_set_vnode_cached_size(cp, hdr->uncompressed_size);
		}
		decmpfs_cnode_set_vnode_state(cp, ret, 1);
		decmpfs_cnode_set_vnode_cmp_type(cp, hdr->compression_type, 1);
		                                                                 
		if (hdr->attr_size == sizeof(decmpfs_disk_header)) {
			decmpfs_cnode_set_vnode_minimal_xattr(cp, 1, 1);
		}
		if (ret == FILE_IS_COMPRESSED) {
			                                         
			ubc_setsize(vp, hdr->uncompressed_size);

			                                                         
			lck_rw_lock_shared(&decompressorsLock);
			decmpfs_get_decompression_flags_func get_flags = decmp_get_func(vp, hdr->compression_type, get_flags);
			if (get_flags) {
				decompression_flags = get_flags(vp, decmpfs_ctx, hdr);
			}
			lck_rw_unlock_shared(&decompressorsLock);
			decmpfs_cnode_set_decompression_flags(cp, decompression_flags);
		}
	} else {
		                                                                                                                               
		decmpfs_cnode_set_vnode_state(cp, ret, cnode_locked);
	}

	if (cnode_locked) {
		decmpfs_unlock_compressed_data(cp, 1);
	}

	if (hdr != NULL) {
		kheap_free(KHEAP_TEMP, hdr, alloc_size);
	}

	  
                                                                      
   
                                                        
                                                    
    
	switch (ret) {
	case FILE_IS_NOT_COMPRESSED:
		DECMPFS_EMIT_TRACE_RETURN(DECMPDBG_FILE_IS_COMPRESSED, vp->v_id, 0);
		return 0;
	case FILE_IS_COMPRESSED:
	case FILE_IS_CONVERTING:
		DECMPFS_EMIT_TRACE_RETURN(DECMPDBG_FILE_IS_COMPRESSED, vp->v_id, 1);
		return 1;
	default:
		                                                  
		DECMPFS_EMIT_TRACE_RETURN(DECMPDBG_FILE_IS_COMPRESSED, vp->v_id, 0);
		ErrorLogWithPath("unknown ret %d\n", ret);
		return 0;
	}
}

int
decmpfs_update_attributes(vnode_t vp, struct vnode_attr *vap)
{
	int error = 0;

	if (VATTR_IS_ACTIVE(vap, va_flags)) {
		                                     
		if (vap->va_flags & UF_COMPRESSED) {
			                                                     
			struct vnode_attr va_fetch;
			int old_flags = 0;
			VATTR_INIT(&va_fetch);
			VATTR_WANTED(&va_fetch, va_flags);
			error = vnode_getattr(vp, &va_fetch, decmpfs_ctx);
			if (error) {
				return error;
			}

			old_flags = va_fetch.va_flags;

			if (!(old_flags & UF_COMPRESSED)) {
				  
                                                                                              
                                                                                                
                                                                                    
       

				if (VATTR_IS_ACTIVE(vap, va_data_size)) {
					  
                                                                                
                                            
        
					vap->va_flags &= ~UF_COMPRESSED;
					return 0;
				}

				decmpfs_header *hdr = NULL;
				size_t alloc_size = 0;
				error = decmpfs_fetch_compressed_header(vp, NULL, &hdr, 1, &alloc_size);
				if (error == 0) {
					  
                                                            
                   
       
                                                           
                                                             
                                                             
                                                            
             
        
					if (!decmpfs_type_is_dataless(hdr->compression_type)) {
						VATTR_SET_ACTIVE(vap, va_data_size);
						vap->va_data_size = 0;
					}
				} else if (error == ERANGE) {
					                                                                                                             
				} else {
					                                                         
					vap->va_flags &= ~UF_COMPRESSED;
				}
				if (hdr != NULL) {
					kheap_free(KHEAP_TEMP, hdr, alloc_size);
				}
			}
		}
	}

	return 0;
}

static int
wait_for_decompress(decmpfs_cnode *cp)
{
	int state;
	lck_mtx_lock(&decompress_channel_mtx);
	do {
		state = decmpfs_fast_get_state(cp);
		if (state != FILE_IS_CONVERTING) {
			                               
			lck_mtx_unlock(&decompress_channel_mtx);
			return state;
		}
		msleep((caddr_t)&decompress_channel, &decompress_channel_mtx, PINOD, "wait_for_decompress", NULL);
	} while (1);
}

#pragma mark --- decmpfs hide query routines ---

int
decmpfs_hides_rsrc(vfs_context_t ctx, decmpfs_cnode *cp)
{
	  
               
                                                                     
                                 
   
                                                                                                        
    

	if (ctx == decmpfs_ctx) {
		return 0;
	}

	if (!decmpfs_fast_file_is_compressed(cp)) {
		return 0;
	}

	                                                   
	return 1;
}

int
decmpfs_hides_xattr(vfs_context_t ctx, decmpfs_cnode *cp, const char *xattr)
{
	  
               
                                                                     
                                 
   
                                                                                                        
    

	if (ctx == decmpfs_ctx) {
		return 0;
	}
	if (strncmp(xattr, XATTR_RESOURCEFORK_NAME, sizeof(XATTR_RESOURCEFORK_NAME) - 1) == 0) {
		return decmpfs_hides_rsrc(ctx, cp);
	}
	if (!decmpfs_fast_file_is_compressed(cp)) {
		                                                      
		return 0;
	}
	if (strncmp(xattr, DECMPFS_XATTR_NAME, sizeof(DECMPFS_XATTR_NAME) - 1) == 0) {
		                                
		return 1;
	}
	                           
	return 0;
}

#pragma mark --- registration/validation routines ---

static inline int
registration_valid(const decmpfs_registration *registration)
{
	return registration && ((registration->decmpfs_registration == DECMPFS_REGISTRATION_VERSION_V1) || (registration->decmpfs_registration == DECMPFS_REGISTRATION_VERSION_V3));
}

errno_t
register_decmpfs_decompressor(uint32_t compression_type, const decmpfs_registration *registration)
{
	                                               

	errno_t ret = 0;
	int locked = 0;
	char resourceName[80];

	if ((compression_type >= CMP_MAX) || !registration_valid(registration)) {
		ret = EINVAL;
		goto out;
	}

	lck_rw_lock_exclusive(&decompressorsLock); locked = 1;

	                                                      
	if (decompressors[compression_type] != NULL) {
		ret = EEXIST;
		goto out;
	}
	decompressors[compression_type] = registration;
	snprintf(resourceName, sizeof(resourceName), "com.apple.AppleFSCompression.Type%u", compression_type);
	IOServicePublishResource(resourceName, TRUE);

out:
	if (locked) {
		lck_rw_unlock_exclusive(&decompressorsLock);
	}
	return ret;
}

errno_t
unregister_decmpfs_decompressor(uint32_t compression_type, decmpfs_registration *registration)
{
	                                                 

	errno_t ret = 0;
	int locked = 0;
	char resourceName[80];

	if ((compression_type >= CMP_MAX) || !registration_valid(registration)) {
		ret = EINVAL;
		goto out;
	}

	lck_rw_lock_exclusive(&decompressorsLock); locked = 1;
	if (decompressors[compression_type] != registration) {
		ret = EEXIST;
		goto out;
	}
	decompressors[compression_type] = NULL;
	snprintf(resourceName, sizeof(resourceName), "com.apple.AppleFSCompression.Type%u", compression_type);
	IOServicePublishResource(resourceName, FALSE);

out:
	if (locked) {
		lck_rw_unlock_exclusive(&decompressorsLock);
	}
	return ret;
}

static int
compression_type_valid(vnode_t vp, decmpfs_header *hdr)
{
	                                                                        
	int ret = 0;

	                                                          
	lck_rw_lock_shared(&decompressorsLock);
	if (decmp_get_func(vp, hdr->compression_type, fetch) != NULL) {
		ret = 1;
	}
	lck_rw_unlock_shared(&decompressorsLock);

	return ret;
}

#pragma mark --- compression/decompression routines ---

static int
decmpfs_fetch_uncompressed_data(vnode_t vp, decmpfs_cnode *cp, decmpfs_header *hdr, off_t offset, user_ssize_t size, int nvec, decmpfs_vector *vec, uint64_t *bytes_read)
{
	                                                                                                           

	int err          = 0;

	*bytes_read = 0;

	if (offset >= (off_t)hdr->uncompressed_size) {
		                                             
		err = 0;
		goto out;
	}
	if (offset < 0) {
		                                             
		err = EINVAL;
		goto out;
	}
	if (hdr->uncompressed_size - offset < size) {
		                                                           
		size = (user_ssize_t)(hdr->uncompressed_size - offset);
	}
	if (size == 0) {
		                     
		err = 0;
		goto out;
	}

	  
                                                                     
   
                                                             
                                             
                                                                  
                                                          
   
                                                             
                    
    
	DECMPFS_EMIT_TRACE_ENTRY(DECMPDBG_FETCH_UNCOMPRESSED_DATA, vp->v_id,
	    hdr->compression_type, (int)offset, (int)size);
	lck_rw_lock_shared(&decompressorsLock);
	decmpfs_fetch_uncompressed_data_func fetch = decmp_get_func(vp, hdr->compression_type, fetch);
	if (fetch) {
		err = fetch(vp, decmpfs_ctx, hdr, offset, size, nvec, vec, bytes_read);
		lck_rw_unlock_shared(&decompressorsLock);
		if (err == 0) {
			uint64_t decompression_flags = decmpfs_cnode_get_decompression_flags(cp);
			if (decompression_flags & DECMPFS_FLAGS_FORCE_FLUSH_ON_DECOMPRESS) {
#if     !defined(__i386__) && !defined(__x86_64__)
				int i;
				for (i = 0; i < nvec; i++) {
					assert(vec[i].size >= 0 && vec[i].size <= UINT_MAX);
					flush_dcache64((addr64_t)(uintptr_t)vec[i].buf, (unsigned int)vec[i].size, FALSE);
				}
#endif
			}
		}
	} else {
		err = ENOTSUP;
		lck_rw_unlock_shared(&decompressorsLock);
	}
	  
                                                                      
   
                                                             
                                                                   
                                                       
   
                                                                        
    
	DECMPFS_EMIT_TRACE_RETURN(DECMPDBG_FETCH_UNCOMPRESSED_DATA, vp->v_id,
	    (int)*bytes_read, err);
out:
	return err;
}

static kern_return_t
commit_upl(upl_t upl, upl_offset_t pl_offset, size_t uplSize, int flags, int abort)
{
	kern_return_t kr = 0;

#if CONFIG_IOSCHED
	upl_unmark_decmp(upl);
#endif                     

	                          
	if (abort) {
		VerboseLog("aborting upl, flags 0x%08x\n", flags);
		kr = ubc_upl_abort_range(upl, pl_offset, (upl_size_t)uplSize, flags);
		if (kr != KERN_SUCCESS) {
			ErrorLog("ubc_upl_abort_range error %d\n", (int)kr);
		}
	} else {
		VerboseLog("committing upl, flags 0x%08x\n", flags | UPL_COMMIT_CLEAR_DIRTY);
		kr = ubc_upl_commit_range(upl, pl_offset, (upl_size_t)uplSize, flags | UPL_COMMIT_CLEAR_DIRTY | UPL_COMMIT_WRITTEN_BY_KERNEL);
		if (kr != KERN_SUCCESS) {
			ErrorLog("ubc_upl_commit_range error %d\n", (int)kr);
		}
	}
	return kr;
}


errno_t
decmpfs_pagein_compressed(struct vnop_pagein_args *ap, int *is_compressed, decmpfs_cnode *cp)
{
	                                                              

	int err                      = 0;
	vnode_t vp                   = ap->a_vp;
	upl_t pl                     = ap->a_pl;
	upl_offset_t pl_offset       = ap->a_pl_offset;
	off_t f_offset               = ap->a_f_offset;
	size_t size                  = ap->a_size;
	int flags                    = ap->a_flags;
	off_t uplPos                 = 0;
	user_ssize_t uplSize         = 0;
	size_t verify_block_size     = 0;
	void *data                   = NULL;
	decmpfs_header *hdr = NULL;
	size_t alloc_size            = 0;
	uint64_t cachedSize          = 0;
	int cmpdata_locked           = 0;
	bool file_tail_page_valid    = false;
	int  num_valid_pages         = 0;
	int  num_invalid_pages       = 0;

	if (!decmpfs_trylock_compressed_data(cp, 0)) {
		return EAGAIN;
	}
	cmpdata_locked = 1;


	if (flags & ~(UPL_IOSYNC | UPL_NOCOMMIT | UPL_NORDAHEAD)) {
		DebugLogWithPath("pagein: unknown flags 0x%08x\n", (flags & ~(UPL_IOSYNC | UPL_NOCOMMIT | UPL_NORDAHEAD)));
	}

	err = decmpfs_fetch_compressed_header(vp, cp, &hdr, 0, &alloc_size);
	if (err != 0) {
		goto out;
	}

	cachedSize = hdr->uncompressed_size;

	if (!compression_type_valid(vp, hdr)) {
		                               
		err = ENOTSUP;
		goto out;
	}

	  
                                                                        
                                                                          
                                                                  
                           
    
	err = VNOP_VERIFY(vp, f_offset, NULL, 0, &verify_block_size,
	    VNODE_VERIFY_DEFAULT, NULL);
	if (err) {
		goto out;
	} else if (verify_block_size) {
		if (verify_block_size & (verify_block_size - 1)) {
			ErrorLogWithPath("verify block size is not power of 2, no verification will be done\n");
			err = EINVAL;
		} else if (size % verify_block_size) {
			ErrorLogWithPath("upl size is not a multiple of verify block size\n");
			err = EINVAL;
		}
		if (err) {
			goto out;
		}
	}

#if CONFIG_IOSCHED
	                                                          
	upl_mark_decmp(pl);
#endif                     

	                                         
	kern_return_t kr = ubc_upl_map(pl, (vm_offset_t*)&data);
	if ((kr != KERN_SUCCESS) || (data == NULL)) {
		err = ENOSPC;
		data = NULL;
#if CONFIG_IOSCHED
		upl_unmark_decmp(pl);
#endif                     
		goto out;
	}

	uplPos = f_offset;
	uplSize = size;

	                                           
	if ((uint64_t)uplPos + uplSize > cachedSize) {
		                                               
		uplSize = (user_ssize_t)(cachedSize - uplPos);
	}

	                  
	decmpfs_vector vec;

decompress:
	                                                                                                                                 
	vec = (decmpfs_vector) {
		.buf = (char*)data + pl_offset,
		.size = size,
	};

	uint64_t did_read = 0;
	if (decmpfs_fast_get_state(cp) == FILE_IS_CONVERTING) {
		ErrorLogWithPath("unexpected pagein during decompress\n");
		  
                                                                                                                           
                                                                                                           
     
		err = 0;
	} else {
		if (!verify_block_size || (verify_block_size <= PAGE_SIZE)) {
			err = decmpfs_fetch_uncompressed_data(vp, cp, hdr, uplPos, uplSize, 1, &vec, &did_read);
		} else {
			off_t l_uplPos = uplPos;
			off_t l_pl_offset = pl_offset;
			user_ssize_t l_uplSize = uplSize;
			upl_page_info_t *pl_info = ubc_upl_pageinfo(pl);

			err = 0;
			  
                                                                     
                                                            
                                                              
                                            
     
                                                             
     
                                                                            
                                                                
                                                        
     
                                                             
                                                          
                                                           
                                                   
                                                        
     
                                                  
                                            
                                                               
                                                           
                                                  
                                  
                                         
                                                  
                                  
                                        
     
                                                           
                    
     
      
			while (l_uplSize) {
				uint64_t l_did_read = 0;
				int pl_offset_pg = (int)(l_pl_offset / PAGE_SIZE);
				int pages_left_in_upl;
				int start_pg;
				int last_pg;

				  
                                                             
                                                       
                                    
       
				pages_left_in_upl = (int)(round_page((vm_offset_t)l_uplSize) / PAGE_SIZE);

				  
                                                               
                                                            
                                         
                                                  
                                                    
                                           
       
				for (start_pg = 0; start_pg < pages_left_in_upl; start_pg++) {
					if (!upl_valid_page(pl_info, pl_offset_pg + start_pg)) {
						break;
					}
				}

				num_valid_pages += start_pg;

				  
                                                      
                                                
                                                   
                                                                    
       
				for (last_pg = start_pg; last_pg < pages_left_in_upl; last_pg++) {
					if (upl_valid_page(pl_info, pl_offset_pg + last_pg)) {
						break;
					}
				}

				if (start_pg < last_pg) {
					off_t inval_offset = start_pg * PAGE_SIZE;
					int inval_pages = last_pg - start_pg;
					int inval_size = inval_pages * PAGE_SIZE;
					decmpfs_vector l_vec;

					num_invalid_pages += inval_pages;
					if (inval_offset) {
						did_read += inval_offset;
						l_pl_offset += inval_offset;
						l_uplPos += inval_offset;
						l_uplSize -= inval_offset;
					}

					l_vec = (decmpfs_vector) {
						.buf = (char*)data + l_pl_offset,
						.size = inval_size,
					};

					err = decmpfs_fetch_uncompressed_data(vp, cp, hdr, l_uplPos,
					    MIN(l_uplSize, inval_size), 1, &l_vec, &l_did_read);

					if (!err && (l_did_read != inval_size) && (l_uplSize > inval_size)) {
						ErrorLogWithPath("Unexpected size fetch of decompressed data, l_uplSize = %d, l_did_read = %d, inval_size = %d\n",
						    (int)l_uplSize, (int)l_did_read, (int)inval_size);
						err = EINVAL;
					}
				} else {
					                           
					l_did_read = l_uplSize;
					if (uplSize < size) {
						file_tail_page_valid = true;
					}
				}

				if (err) {
					break;
				}

				did_read += l_did_read;
				l_pl_offset += l_did_read;
				l_uplPos += l_did_read;
				l_uplSize -= l_did_read;
			}
		}
	}
	if (err) {
		DebugLogWithPath("decmpfs_fetch_uncompressed_data err %d\n", err);
		int cmp_state = decmpfs_fast_get_state(cp);
		if (cmp_state == FILE_IS_CONVERTING) {
			DebugLogWithPath("cmp_state == FILE_IS_CONVERTING\n");
			cmp_state = wait_for_decompress(cp);
			if (cmp_state == FILE_IS_COMPRESSED) {
				DebugLogWithPath("cmp_state == FILE_IS_COMPRESSED\n");
				                                                                             
				goto decompress;
			}
		}
		if (cmp_state == FILE_IS_NOT_COMPRESSED) {
			DebugLogWithPath("cmp_state == FILE_IS_NOT_COMPRESSED\n");
			                                                           
			*is_compressed = 0;                                                      
		}
	}

	                                                                                
	uint64_t total_size = (size + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);
	if (did_read < total_size && !(verify_block_size && err)) {
		uint64_t rounded_up_did_read = file_tail_page_valid ? (uint64_t)(round_page((vm_offset_t)did_read)) : did_read;
		memset((char*)vec.buf + rounded_up_did_read, 0, (size_t)(total_size - rounded_up_did_read));
	}

	if (!err && verify_block_size) {
		size_t cur_verify_block_size = verify_block_size;

		if ((err = VNOP_VERIFY(vp, uplPos, vec.buf, size, &cur_verify_block_size, 0, NULL))) {
			ErrorLogWithPath("Verification failed with error %d, uplPos = %lld, uplSize = %d, did_read = %d, total_size = %d, valid_pages = %d, invalid_pages = %d, tail_page_valid = %d\n",
			    err, (long long)uplPos, (int)uplSize, (int)did_read, (int)total_size, num_valid_pages, num_invalid_pages, file_tail_page_valid);
		}
		                                                           
	}

#if CONFIG_IOSCHED
	upl_unmark_decmp(pl);
#endif                     

	kr = ubc_upl_unmap(pl); data = NULL;                                                                         
	if (kr != KERN_SUCCESS) {
		ErrorLogWithPath("ubc_upl_unmap error %d\n", (int)kr);
	} else {
		if (!err) {
			                      
			kr = commit_upl(pl, pl_offset, (size_t)total_size, UPL_COMMIT_FREE_ON_EMPTY, 0);
		}
	}

out:
	if (data) {
		ubc_upl_unmap(pl);
	}
	if (hdr != NULL) {
		kheap_free(KHEAP_TEMP, hdr, alloc_size);
	}
	if (cmpdata_locked) {
		decmpfs_unlock_compressed_data(cp, 0);
	}
	if (err) {
#if 0
		if (err != ENXIO && err != ENOSPC) {
			char *path = zalloc(ZV_NAMEI);
			panic("%s: decmpfs_pagein_compressed: err %d", vnpath(vp, path, PATH_MAX), err);
			zfree(ZV_NAMEI, path);
		}
#endif        
		ErrorLogWithPath("err %d\n", err);
	}
	return err;
}

errno_t
decmpfs_read_compressed(struct vnop_read_args *ap, int *is_compressed, decmpfs_cnode *cp)
{
	                                                           

	uio_t uio                    = ap->a_uio;
	vnode_t vp                   = ap->a_vp;
	int err                      = 0;
	int countInt                 = 0;
	off_t uplPos                 = 0;
	user_ssize_t uplSize         = 0;
	user_ssize_t uplRemaining    = 0;
	off_t curUplPos              = 0;
	user_ssize_t curUplSize      = 0;
	kern_return_t kr             = KERN_SUCCESS;
	int abort_read               = 0;
	void *data                   = NULL;
	uint64_t did_read            = 0;
	upl_t upl                    = NULL;
	upl_page_info_t *pli         = NULL;
	decmpfs_header *hdr          = NULL;
	size_t alloc_size            = 0;
	uint64_t cachedSize          = 0;
	off_t uioPos                 = 0;
	user_ssize_t uioRemaining    = 0;
	size_t verify_block_size     = 0;
	size_t alignment_size        = PAGE_SIZE;
	int cmpdata_locked           = 0;

	decmpfs_lock_compressed_data(cp, 0); cmpdata_locked = 1;

	uplPos = uio_offset(uio);
	uplSize = uio_resid(uio);
	VerboseLogWithPath("uplPos %lld uplSize %lld\n", uplPos, uplSize);

	cachedSize = decmpfs_cnode_get_vnode_cached_size(cp);

	if ((uint64_t)uplPos + uplSize > cachedSize) {
		                                               
		uplSize = (user_ssize_t)(cachedSize - uplPos);
	}

	                                                                        
	countInt = (uplSize > INT_MAX) ? INT_MAX : (int)uplSize;
	err = cluster_copy_ubc_data(vp, uio, &countInt, 0);
	if (err != 0) {
		goto out;
	}

	                            
	uioPos = uio_offset(uio);
	uioRemaining = uio_resid(uio);
	if ((uint64_t)uioPos + uioRemaining > cachedSize) {
		                                               
		uioRemaining = (user_ssize_t)(cachedSize - uioPos);
	}

	if (uioRemaining <= 0) {
		                  
		goto out;
	}

	err = decmpfs_fetch_compressed_header(vp, cp, &hdr, 0, &alloc_size);
	if (err != 0) {
		goto out;
	}
	if (!compression_type_valid(vp, hdr)) {
		err = ENOTSUP;
		goto out;
	}

	uplPos = uioPos;
	uplSize = uioRemaining;
#if COMPRESSION_DEBUG
	DebugLogWithPath("uplPos %lld uplSize %lld\n", (uint64_t)uplPos, (uint64_t)uplSize);
#endif

	lck_rw_lock_shared(&decompressorsLock);
	decmpfs_adjust_fetch_region_func adjust_fetch = decmp_get_func(vp, hdr->compression_type, adjust_fetch);
	if (adjust_fetch) {
		                                                                                 
		adjust_fetch(vp, decmpfs_ctx, hdr, &uplPos, &uplSize);
		VerboseLogWithPath("adjusted uplPos %lld uplSize %lld\n", (uint64_t)uplPos, (uint64_t)uplSize);
	}
	lck_rw_unlock_shared(&decompressorsLock);

	                                                    
	if ((uint64_t)uplPos + uplSize > cachedSize) {
		                                               
		uplSize = (user_ssize_t)(cachedSize - uplPos);
	}

	if (uplSize <= 0) {
		                  
		goto out;
	}

	  
                                                                        
                                       
    

	                                                                                           
	err = VNOP_VERIFY(vp, uplPos, NULL, 0, &verify_block_size, VNODE_VERIFY_DEFAULT, NULL);
	if (err) {
		goto out;
	} else if (verify_block_size) {
		if (verify_block_size & (verify_block_size - 1)) {
			ErrorLogWithPath("verify block size is not power of 2, no verification will be done\n");
			verify_block_size = 0;
		} else if (verify_block_size > PAGE_SIZE) {
			alignment_size = verify_block_size;
		}
	}

	if (uplPos & (alignment_size - 1)) {
		                                          
		uplSize += (uplPos & (alignment_size - 1));
		uplPos &= ~(alignment_size - 1);
	}

	                                               
	uplSize = (uplSize + (alignment_size - 1)) & ~(alignment_size - 1);

	VerboseLogWithPath("new uplPos %lld uplSize %lld\n", (uint64_t)uplPos, (uint64_t)uplSize);

	uplRemaining = uplSize;
	curUplPos = uplPos;
	curUplSize = 0;

	while (uplRemaining > 0) {
		                              
		curUplPos += curUplSize;

		                          
		curUplSize = uplRemaining;
		if (curUplSize > MAX_UPL_SIZE_BYTES) {
			curUplSize = MAX_UPL_SIZE_BYTES;
		}

		                    
		kr = ubc_create_upl_kernel(vp, curUplPos, (int)curUplSize, &upl, &pli, UPL_SET_LITE, VM_KERN_MEMORY_FILE);
		if (kr != KERN_SUCCESS) {
			ErrorLogWithPath("ubc_create_upl error %d\n", (int)kr);
			err = EINVAL;
			goto out;
		}
		VerboseLogWithPath("curUplPos %lld curUplSize %lld\n", (uint64_t)curUplPos, (uint64_t)curUplSize);

#if CONFIG_IOSCHED
		                                                          
		upl_mark_decmp(upl);
#endif                     

		                 
		kr = ubc_upl_map(upl, (vm_offset_t*)&data);
		if (kr != KERN_SUCCESS) {
			commit_upl(upl, 0, curUplSize, UPL_ABORT_FREE_ON_EMPTY, 1);
#if 0
			char *path = zalloc(ZV_NAMEI);
			panic("%s: decmpfs_read_compressed: ubc_upl_map error %d", vnpath(vp, path, PATH_MAX), (int)kr);
			zfree(ZV_NAMEI, path);
#else        
			ErrorLogWithPath("ubc_upl_map kr=0x%x\n", (int)kr);
#endif        
			err = EINVAL;
			goto out;
		}

		                                 
		if (!data) {
			commit_upl(upl, 0, curUplSize, UPL_ABORT_FREE_ON_EMPTY, 1);

			ErrorLogWithPath("ubc_upl_map mapped null\n");
			err = EINVAL;
			goto out;
		}

		                                                 
		decmpfs_vector vec;
decompress:
		vec = (decmpfs_vector){ .buf = data, .size = curUplSize };
		err = decmpfs_fetch_uncompressed_data(vp, cp, hdr, curUplPos, curUplSize, 1, &vec, &did_read);
		if (err) {
			ErrorLogWithPath("decmpfs_fetch_uncompressed_data err %d\n", err);

			                                                  
			int cmp_state = decmpfs_fast_get_state(cp);
			if (cmp_state == FILE_IS_CONVERTING) {
				ErrorLogWithPath("cmp_state == FILE_IS_CONVERTING\n");
				cmp_state = wait_for_decompress(cp);
				if (cmp_state == FILE_IS_COMPRESSED) {
					ErrorLogWithPath("cmp_state == FILE_IS_COMPRESSED\n");
					                                                                        
					goto decompress;
				}
			}
			if (cmp_state == FILE_IS_NOT_COMPRESSED) {
				ErrorLogWithPath("cmp_state == FILE_IS_NOT_COMPRESSED\n");
				                                                           
				abort_read = 1;                                         
				*is_compressed = 0;                                                      
			}
			kr = KERN_FAILURE;
			did_read = 0;
		}

		                                             
		memset((char*)data + did_read, 0, (size_t)(curUplSize - did_read));
		if (!err && verify_block_size) {
			size_t cur_verify_block_size = verify_block_size;

			if ((err = VNOP_VERIFY(vp, curUplPos, data, curUplSize, &cur_verify_block_size, 0, NULL))) {
				ErrorLogWithPath("Verification failed with error %d\n", err);
				abort_read = 1;
			}
			                                                           
		}

		kr = ubc_upl_unmap(upl);
		if (kr == KERN_SUCCESS) {
			if (abort_read) {
				kr = commit_upl(upl, 0, curUplSize, UPL_ABORT_FREE_ON_EMPTY, 1);
			} else {
				VerboseLogWithPath("uioPos %lld uioRemaining %lld\n", (uint64_t)uioPos, (uint64_t)uioRemaining);
				if (uioRemaining) {
					off_t uplOff = uioPos - curUplPos;
					if (uplOff < 0) {
						ErrorLogWithPath("uplOff %lld should never be negative\n", (int64_t)uplOff);
						err = EINVAL;
					} else if (uplOff > INT_MAX) {
						ErrorLogWithPath("uplOff %lld too large\n", (int64_t)uplOff);
						err = EINVAL;
					} else {
						off_t count = curUplPos + curUplSize - uioPos;
						if (count < 0) {
							                                         
						} else {
							if (count > uioRemaining) {
								count = uioRemaining;
							}
							int icount = (count > INT_MAX) ? INT_MAX : (int)count;
							int io_resid = icount;
							err = cluster_copy_upl_data(uio, upl, (int)uplOff, &io_resid);
							int copied = icount - io_resid;
							VerboseLogWithPath("uplOff %lld count %lld copied %lld\n", (uint64_t)uplOff, (uint64_t)count, (uint64_t)copied);
							if (err) {
								ErrorLogWithPath("cluster_copy_upl_data err %d\n", err);
							}
							uioPos += copied;
							uioRemaining -= copied;
						}
					}
				}
				kr = commit_upl(upl, 0, curUplSize, UPL_COMMIT_FREE_ON_EMPTY | UPL_COMMIT_INACTIVATE, 0);
				if (err) {
					goto out;
				}
			}
		} else {
			ErrorLogWithPath("ubc_upl_unmap error %d\n", (int)kr);
		}

		uplRemaining -= curUplSize;
	}

out:

	if (hdr != NULL) {
		kheap_free(KHEAP_TEMP, hdr, alloc_size);
	}
	if (cmpdata_locked) {
		decmpfs_unlock_compressed_data(cp, 0);
	}
	if (err) {                          
		ErrorLogWithPath("err %d\n", err);
		return err;
	}

#if COMPRESSION_DEBUG
	uplSize = uio_resid(uio);
	if (uplSize) {
		VerboseLogWithPath("still %lld bytes to copy\n", uplSize);
	}
#endif
	return 0;
}

int
decmpfs_free_compressed_data(vnode_t vp, decmpfs_cnode *cp)
{
	  
                                                                                              
                                             
    
	decmpfs_header *hdr = NULL;
	size_t alloc_size = 0;

	  
                                                                     
   
                                                                            
    
	DECMPFS_EMIT_TRACE_ENTRY(DECMPDBG_FREE_COMPRESSED_DATA, vp->v_id);

	int err = decmpfs_fetch_compressed_header(vp, cp, &hdr, 0, &alloc_size);
	if (err) {
		ErrorLogWithPath("decmpfs_fetch_compressed_header err %d\n", err);
	} else {
		lck_rw_lock_shared(&decompressorsLock);
		decmpfs_free_compressed_data_func free_data = decmp_get_func(vp, hdr->compression_type, free_data);
		if (free_data) {
			err = free_data(vp, decmpfs_ctx, hdr);
		} else {
			                                
			err = 0;
		}
		lck_rw_unlock_shared(&decompressorsLock);

		if (err != 0) {
			ErrorLogWithPath("decompressor err %d\n", err);
		}
	}
	  
                                                                      
   
                                                                            
                                                       
    
	DECMPFS_EMIT_TRACE_RETURN(DECMPDBG_FREE_COMPRESSED_DATA, vp->v_id, err);

	                      
	err = vn_removexattr(vp, DECMPFS_XATTR_NAME, 0, decmpfs_ctx);

	if (hdr != NULL) {
		kheap_free(KHEAP_TEMP, hdr, alloc_size);
	}
	return err;
}

#pragma mark --- file conversion routines ---

static int
unset_compressed_flag(vnode_t vp)
{
	int err = 0;
	struct vnode_attr va;
	int new_bsdflags = 0;

	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_flags);
	err = vnode_getattr(vp, &va, decmpfs_ctx);

	if (err != 0) {
		ErrorLogWithPath("vnode_getattr err %d\n", err);
	} else {
		new_bsdflags = va.va_flags & ~UF_COMPRESSED;

		VATTR_INIT(&va);
		VATTR_SET(&va, va_flags, new_bsdflags);
		err = vnode_setattr(vp, &va, decmpfs_ctx);
		if (err != 0) {
			ErrorLogWithPath("vnode_setattr err %d\n", err);
		}
	}
	return err;
}

int
decmpfs_decompress_file(vnode_t vp, decmpfs_cnode *cp, off_t toSize, int truncate_okay, int skiplock)
{
	                                                       

	int err                      = 0;
	char *data                   = NULL;
	uio_t uio_w                  = 0;
	off_t offset                 = 0;
	uint32_t old_state           = 0;
	uint32_t new_state           = 0;
	int update_file_state        = 0;
	size_t allocSize             = 0;
	decmpfs_header *hdr          = NULL;
	size_t hdr_size              = 0;
	int cmpdata_locked           = 0;
	off_t remaining              = 0;
	uint64_t uncompressed_size   = 0;

	  
                                                                     
   
                                                        
                                                 
                                                  
                                                              
   
                                                                    
    
	DECMPFS_EMIT_TRACE_ENTRY(DECMPDBG_DECOMPRESS_FILE, vp->v_id,
	    (int)toSize, truncate_okay, skiplock);

	if (!skiplock) {
		decmpfs_lock_compressed_data(cp, 1); cmpdata_locked = 1;
	}

decompress:
	old_state = decmpfs_fast_get_state(cp);

	switch (old_state) {
	case FILE_IS_NOT_COMPRESSED:
	{
		                                        
		err = 0;
		goto out;
	}

	case FILE_TYPE_UNKNOWN:
	{
		                                                                    
		(void)decmpfs_file_is_compressed(vp, cp);

		               
		goto decompress;
	}

	case FILE_IS_COMPRESSED:
	{
		                                              
		break;
	}

	default:
	{
		  
                                                                                                
                                                                                         
                                               
     
		err = EINVAL;
		goto out;
	}
	}

	err = decmpfs_fetch_compressed_header(vp, cp, &hdr, 0, &hdr_size);
	if (err != 0) {
		goto out;
	}

	uncompressed_size = hdr->uncompressed_size;
	if (toSize == -1) {
		toSize = hdr->uncompressed_size;
	}

	if (toSize == 0) {
		                                                    
		goto nodecmp;
	} else if ((uint64_t)toSize > hdr->uncompressed_size) {
		                                                                                 
		toSize = hdr->uncompressed_size;
	}

	allocSize = MIN(64 * 1024, (size_t)toSize);
	data = kheap_alloc(KHEAP_TEMP, allocSize, Z_WAITOK);
	if (!data) {
		err = ENOMEM;
		goto out;
	}

	uio_w = uio_create(1, 0LL, UIO_SYSSPACE, UIO_WRITE);
	if (!uio_w) {
		err = ENOMEM;
		goto out;
	}
	uio_w->uio_flags |= UIO_FLAGS_IS_COMPRESSED_FILE;

	remaining = toSize;

	                                                      
	ubc_setsize(vp, 0);

	                                                    
	decmpfs_cnode_set_vnode_state(cp, FILE_IS_CONVERTING, 1);

	while (remaining > 0) {
		                                                                             

		uint64_t bytes_read = 0;
		decmpfs_vector vec = { .buf = data, .size = (user_ssize_t)MIN(allocSize, remaining) };
		err = decmpfs_fetch_uncompressed_data(vp, cp, hdr, offset, vec.size, 1, &vec, &bytes_read);
		if (err != 0) {
			ErrorLogWithPath("decmpfs_fetch_uncompressed_data err %d\n", err);
			goto out;
		}

		if (bytes_read == 0) {
			                             
			break;
		}

		uio_reset(uio_w, offset, UIO_SYSSPACE, UIO_WRITE);
		err = uio_addiov(uio_w, CAST_USER_ADDR_T(data), (user_size_t)bytes_read);
		if (err != 0) {
			ErrorLogWithPath("uio_addiov err %d\n", err);
			err = ENOMEM;
			goto out;
		}

		err = VNOP_WRITE(vp, uio_w, 0, decmpfs_ctx);
		if (err != 0) {
			                                                          
			ErrorLogWithPath("VNOP_WRITE err %d\n", err);
			break;
		}
		offset += bytes_read;
		remaining -= bytes_read;
	}

	if (err == 0) {
		if (offset != toSize) {
			ErrorLogWithPath("file decompressed to %lld instead of %lld\n", offset, toSize);
			err = EINVAL;
			goto out;
		}
	}

	if (err == 0) {
		                                
		err = VNOP_FSYNC(vp, MNT_WAIT, decmpfs_ctx);
		if (err != 0) {
			ErrorLogWithPath("VNOP_FSYNC err %d\n", err);
			goto out;
		}
	}

	if (err != 0) {
		                                     
		ErrorLogWithPath("aborting decompress, err %d\n", err);
		if (truncate_okay) {
			                                             
			int error = vnode_setsize(vp, 0, 0, decmpfs_ctx);
			ErrorLogWithPath("vnode_setsize err %d\n", error);
		}
		goto out;
	}

nodecmp:
	                                                                          

	                               
	unset_compressed_flag(vp);

	                                                        
	err = decmpfs_free_compressed_data(vp, cp);
	if (err != 0) {
		ErrorLogWithPath("decmpfs_free_compressed_data err %d\n", err);
	}

	  
                                                                                       
                                                                        
    
	err = 0;

	                                                                
	update_file_state = 1;
	new_state = FILE_IS_NOT_COMPRESSED;

#if COMPRESSION_DEBUG
	{
		uint64_t filesize = 0;
		vnsize(vp, &filesize);
		DebugLogWithPath("new file size %lld\n", filesize);
	}
#endif

out:
	if (hdr != NULL) {
		kheap_free(KHEAP_TEMP, hdr, hdr_size);
	}
	kheap_free(KHEAP_TEMP, data, allocSize);
	if (uio_w) {
		uio_free(uio_w);
	}

	if (err != 0) {
		                                                                                                
		update_file_state = 1;
		new_state = FILE_TYPE_UNKNOWN;
		if (uncompressed_size) {
			ubc_setsize(vp, 0);
			ubc_setsize(vp, uncompressed_size);
		}
	}

	if (update_file_state) {
		lck_mtx_lock(&decompress_channel_mtx);
		decmpfs_cnode_set_vnode_state(cp, new_state, 1);
		wakeup((caddr_t)&decompress_channel);                                                                   
		lck_mtx_unlock(&decompress_channel_mtx);
	}

	if (cmpdata_locked) {
		decmpfs_unlock_compressed_data(cp, 1);
	}
	  
                                                                      
   
                                                       
                                            
    
	DECMPFS_EMIT_TRACE_RETURN(DECMPDBG_DECOMPRESS_FILE, vp->v_id, err);
	return err;
}

#pragma mark --- Type1 compressor ---

  
                                                                                 
   

static int
decmpfs_validate_compressed_file_Type1(__unused vnode_t vp, __unused vfs_context_t ctx, decmpfs_header *hdr)
{
	int err          = 0;

	if (hdr->uncompressed_size + sizeof(decmpfs_disk_header) != (uint64_t)hdr->attr_size) {
		err = EINVAL;
		goto out;
	}
out:
	return err;
}

static int
decmpfs_fetch_uncompressed_data_Type1(__unused vnode_t vp, __unused vfs_context_t ctx, decmpfs_header *hdr, off_t offset, user_ssize_t size, int nvec, decmpfs_vector *vec, uint64_t *bytes_read)
{
	int err          = 0;
	int i;
	user_ssize_t remaining;

	if (hdr->uncompressed_size + sizeof(decmpfs_disk_header) != (uint64_t)hdr->attr_size) {
		err = EINVAL;
		goto out;
	}

#if COMPRESSION_DEBUG
	static int dummy = 0;                                          
	DebugLogWithPath("%d memcpy %lld at %lld\n", dummy++, size, (uint64_t)offset);
#endif

	remaining = size;
	for (i = 0; (i < nvec) && (remaining > 0); i++) {
		user_ssize_t curCopy = vec[i].size;
		if (curCopy > remaining) {
			curCopy = remaining;
		}
		memcpy(vec[i].buf, hdr->attr_bytes + offset, curCopy);
		offset += curCopy;
		remaining -= curCopy;
	}

	if ((bytes_read) && (err == 0)) {
		*bytes_read = (size - remaining);
	}

out:
	return err;
}

SECURITY_READ_ONLY_EARLY(static decmpfs_registration) Type1Reg =
{
	.decmpfs_registration = DECMPFS_REGISTRATION_VERSION,
	.validate          = decmpfs_validate_compressed_file_Type1,
	.adjust_fetch      = NULL,                         
	.fetch             = decmpfs_fetch_uncompressed_data_Type1,
	.free_data         = NULL,                       
	.get_flags         = NULL              
};

#pragma mark --- decmpfs initialization ---

void
decmpfs_init(void)
{
	static int done = 0;
	if (done) {
		return;
	}

	decmpfs_ctx = vfs_context_create(vfs_context_kernel());

	register_decmpfs_decompressor(CMP_Type1, &Type1Reg);

	done = 1;
}
#endif                     
