/*
 * Copyright (c) 2008 Apple Inc. All rights reserved.
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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
#ifndef _SYS_DECMPFS_H_
#define _SYS_DECMPFS_H_ 1

#include <stdbool.h>
#include <sys/kdebug.h>
#include <sys/kernel_types.h>
#include <sys/vnode.h>

  
                                                                        
                                                                           
                  
  
                                         
   
#if DECMPFS_ENABLE_KDEBUG_TRACES
#define DECMPFS_EMIT_TRACE_ENTRY(D, ...) \
	KDBG_FILTERED((D) | DBG_FUNC_START, ## __VA_ARGS__)
#define DECMPFS_EMIT_TRACE_RETURN(D, ...) \
	KDBG_FILTERED((D) | DBG_FUNC_END, ##__VA_ARGS__)
#else
#define DECMPFS_EMIT_TRACE_ENTRY(D, ...) do {} while (0)
#define DECMPFS_EMIT_TRACE_RETURN(D, ...) do {} while (0)
#endif                                   

  
                                                
  
                                                                            
                                                
   
#define DECMPDBG_CODE(code)  FSDBG_CODE(DBG_DECMP, code)

enum {
	DECMPDBG_DECOMPRESS_FILE            = DECMPDBG_CODE(0),                
	DECMPDBG_FETCH_COMPRESSED_HEADER    = DECMPDBG_CODE(1),                
	DECMPDBG_FETCH_UNCOMPRESSED_DATA    = DECMPDBG_CODE(2),                
	DECMPDBG_FREE_COMPRESSED_DATA       = DECMPDBG_CODE(4),                
	DECMPDBG_FILE_IS_COMPRESSED         = DECMPDBG_CODE(5),                
};

#define MAX_DECMPFS_XATTR_SIZE 3802

  
                                                              
   

#define DECMPFS_MAGIC 0x636d7066           

#define DECMPFS_XATTR_NAME "com.apple.decmpfs"                                            

  
                                                                      
                           
                                                         
                                                                               
                                                                                
   
typedef struct __attribute__((packed)) {
	uint64_t  value;
} decmpfs_raw_item_size;

#define DECMPFS_PKG_SIZE_MASK           0x000000ffffffffffULL
#define DECMPFS_PKG_COUNT_MASK          0xffffff
#define DECMPFS_PKG_CHLD_COUNT_SHIFT    40

#define DECMPFS_PKG_SIZE(x)             ((x).value & DECMPFS_PKG_SIZE_MASK)
#define DECMPFS_PKG_CHLD_COUNT(x)       ((uint32_t)(((x).value >> DECMPFS_PKG_CHLD_COUNT_SHIFT) & DECMPFS_PKG_COUNT_MASK))
#define DECMPFS_PKG_VALUE_FROM_SIZE_COUNT(size, count) \
	(((size) & DECMPFS_PKG_SIZE_MASK) | ((uint64_t)(count) << DECMPFS_PKG_CHLD_COUNT_SHIFT))

                                
#define DATALESS_CMPFS_TYPE     0x80000001

                                                                                   
#define DATALESS_PKG_CMPFS_TYPE 0x80000002


static inline bool
decmpfs_type_is_dataless(uint32_t cmp_type)
{
	return cmp_type == DATALESS_CMPFS_TYPE || cmp_type == DATALESS_PKG_CMPFS_TYPE;
}

typedef struct __attribute__((packed)) {
	                                                                                     
	uint32_t compression_magic;
	uint32_t compression_type;                           
	union {
		uint64_t uncompressed_size;                            
		decmpfs_raw_item_size _size;
	};
	unsigned char attr_bytes[0];                                                  
} decmpfs_disk_header;

typedef struct __attribute__((packed)) {
	                                                                                     
	uint32_t attr_size;
	uint32_t compression_magic;
	uint32_t compression_type;
	union {
		  
                                                                                    
                                                                                    
          
     
		uint64_t uncompressed_size;
		decmpfs_raw_item_size _size;
	};
	unsigned char attr_bytes[0];                                                  
} decmpfs_header;

static inline uint64_t
decmpfs_get_uncompressed_size(const decmpfs_header *hdr)
{
	if (hdr->compression_magic == DECMPFS_MAGIC && hdr->compression_type == DATALESS_PKG_CMPFS_TYPE) {
		return DECMPFS_PKG_SIZE(hdr->_size);
	}

	return hdr->uncompressed_size;
}

static inline uint32_t
decmpfs_get_directory_entries(const decmpfs_header *hdr)
{
	if (hdr->compression_magic == DECMPFS_MAGIC && hdr->compression_type == DATALESS_PKG_CMPFS_TYPE) {
		return DECMPFS_PKG_CHLD_COUNT(hdr->_size);
	}

	return (uint32_t)hdr->uncompressed_size;
}

                             
enum {
	CMP_Type1       = 1,                                

	                                                            

	CMP_MAX         = 255                                        
};

typedef struct {
	void *buf;
	user_ssize_t size;
} decmpfs_vector;

#ifdef KERNEL

#ifdef XNU_KERNEL_PRIVATE

#include <kern/locks.h>

struct decmpfs_cnode {
	uint8_t cmp_state;
	uint8_t cmp_minimal_xattr;                                                                                                        
	uint32_t cmp_type;
	uint32_t lockcount;
	void    *lockowner;                                                                                       
	uint64_t uncompressed_size __attribute__((aligned(8)));
	uint64_t nchildren __attribute__((aligned(8)));                                                
	uint64_t total_size __attribute__((aligned(8)));                                               
	uint64_t decompression_flags;
	lck_rw_t compressed_data_lock;
};

#endif                      

typedef struct decmpfs_cnode decmpfs_cnode;

                                                   
enum {
	FILE_TYPE_UNKNOWN      = 0,
	FILE_IS_NOT_COMPRESSED = 1,
	FILE_IS_COMPRESSED     = 2,
	FILE_IS_CONVERTING     = 3                                                        
};

                     
extern vfs_context_t decmpfs_ctx;

                                   
void decmpfs_init(void);
decmpfs_cnode *decmpfs_cnode_alloc(void);
void decmpfs_cnode_free(decmpfs_cnode *dp);
void decmpfs_cnode_init(decmpfs_cnode *cp);
void decmpfs_cnode_destroy(decmpfs_cnode *cp);

int decmpfs_hides_rsrc(vfs_context_t ctx, decmpfs_cnode *cp);
int decmpfs_hides_xattr(vfs_context_t ctx, decmpfs_cnode *cp, const char *xattr);

bool decmpfs_trylock_compressed_data(decmpfs_cnode *cp, int exclusive);
void decmpfs_lock_compressed_data(decmpfs_cnode *cp, int exclusive);
void decmpfs_unlock_compressed_data(decmpfs_cnode *cp, int exclusive);

uint32_t decmpfs_cnode_get_vnode_state(decmpfs_cnode *cp);
void decmpfs_cnode_set_vnode_state(decmpfs_cnode *cp, uint32_t state, int skiplock);
uint64_t decmpfs_cnode_get_vnode_cached_size(decmpfs_cnode *cp);
uint64_t decmpfs_cnode_get_vnode_cached_nchildren(decmpfs_cnode *cp);
uint64_t decmpfs_cnode_get_vnode_cached_total_size(decmpfs_cnode *cp);
void decmpfs_cnode_set_vnode_cached_size(decmpfs_cnode *cp, uint64_t size);
void decmpfs_cnode_set_vnode_cached_nchildren(decmpfs_cnode *cp, uint64_t nchildren);
void decmpfs_cnode_set_vnode_cached_total_size(decmpfs_cnode *cp, uint64_t total_sz);
uint32_t decmpfs_cnode_cmp_type(decmpfs_cnode *cp);

int decmpfs_file_is_compressed(vnode_t vp, decmpfs_cnode *cp);
errno_t decmpfs_validate_compressed_file(vnode_t vp, decmpfs_cnode *cp);
int decmpfs_decompress_file(vnode_t vp, decmpfs_cnode *cp, off_t toSize, int truncate_okay, int skiplock);                                                   
int decmpfs_free_compressed_data(vnode_t vp, decmpfs_cnode *cp);
int decmpfs_update_attributes(vnode_t vp, struct vnode_attr *vap);
                                                                                                                                                                          
errno_t decmpfs_pagein_compressed(struct vnop_pagein_args *ap, int *is_compressed, decmpfs_cnode *cp);
errno_t decmpfs_read_compressed(struct vnop_read_args *ap, int *is_compressed, decmpfs_cnode *cp);

                                               
typedef int (*decmpfs_validate_compressed_file_func)(vnode_t vp, vfs_context_t ctx, decmpfs_header *hdr);
typedef void (*decmpfs_adjust_fetch_region_func)(vnode_t vp, vfs_context_t ctx, decmpfs_header *hdr, off_t *offset, user_ssize_t *size);
typedef int (*decmpfs_fetch_uncompressed_data_func)(vnode_t vp, vfs_context_t ctx, decmpfs_header *hdr, off_t offset, user_ssize_t size, int nvec, decmpfs_vector *vec, uint64_t *bytes_read);
typedef int (*decmpfs_free_compressed_data_func)(vnode_t vp, vfs_context_t ctx, decmpfs_header *hdr);
typedef uint64_t (*decmpfs_get_decompression_flags_func)(vnode_t vp, vfs_context_t ctx, decmpfs_header *hdr);                                                          

enum {
	DECMPFS_FLAGS_FORCE_FLUSH_ON_DECOMPRESS = 1 << 0,
};

                                                          
#define DECMPFS_REGISTRATION_VERSION_V1 1
#define DECMPFS_REGISTRATION_VERSION_V3 3

#define DECMPFS_REGISTRATION_VERSION (DECMPFS_REGISTRATION_VERSION_V3)

typedef struct {
	int                                   decmpfs_registration;
	decmpfs_validate_compressed_file_func validate;
	decmpfs_adjust_fetch_region_func      adjust_fetch;
	decmpfs_fetch_uncompressed_data_func  fetch;
	decmpfs_free_compressed_data_func     free_data;
	decmpfs_get_decompression_flags_func  get_flags;
} decmpfs_registration;

                             
errno_t register_decmpfs_decompressor(uint32_t compression_type, const decmpfs_registration *registration);
errno_t unregister_decmpfs_decompressor(uint32_t compression_type, decmpfs_registration *registration);

#endif             

#endif                      
