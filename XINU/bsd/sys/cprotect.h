/*
 * Copyright (c) 2009-2014 Apple Inc. All rights reserved.
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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef _SYS_CPROTECT_H_
#define _SYS_CPROTECT_H_

#ifdef KERNEL_PRIVATE

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/buf.h>
#include <sys/kdebug.h>
#include <crypto/aes.h>
#include <stdbool.h>
#include <uuid/uuid.h>
#include <libkern/crypto/sha1.h>

__BEGIN_DECLS

#define CP_CODE(code) FSDBG_CODE(DBG_CONTENT_PROT, code)
  
                            
                                    
                                               
   

enum {
	CPDBG_OFFSET_IO = CP_CODE(0),                   
};

                                          
#define CP_DEBUG(x, a, b, c, d, e) do {} while (0);

                       
#if !SECURE_KERNEL

                                                        
#if 0
#undef CP_DEBUG
#define CP_DEBUG KERNEL_DEBUG_CONSTANT
#endif

#endif

#define CP_MAX_WRAPPEDKEYSIZE     128                                            
#define VFS_CP_MAX_CACHEBUFLEN    64                                        

                                    
enum {
	CP_ACTION_LOCKED        = 0,
	CP_ACTION_UNLOCKED      = 1,
};
  
                                                                     
                                       
   
typedef int cp_key_store_action_t;

  
                                                                     
                                                            
                                                                    
                                                              
   
typedef unsigned char cp_lock_state_t;
enum {
	CP_LOCKED_STATE         = 0,
	CP_UNLOCKED_STATE       = 1,
};

typedef uint32_t cp_key_class_t;
typedef uint32_t cp_key_os_version_t;
typedef uint16_t cp_key_revision_t;
typedef uint64_t cp_crypto_id_t;

typedef struct cprotect *cprotect_t;
typedef struct cpx *cpx_t;

#ifdef BSD_KERNEL_PRIVATE
                                        
typedef uint32_t cpx_flags_t;
  
                                                                                           
                                                                                     
   
typedef struct fcpx {
#ifdef DEBUG
	uint32_t                cpx_magic1;
#endif         
	aes_encrypt_ctx         *cpx_iv_aes_ctx_ptr;                                     
	cpx_flags_t             cpx_flags;
	uint16_t                cpx_max_key_len;
	uint16_t                cpx_key_len;
	uint8_t                 cpx_cached_key[VFS_CP_MAX_CACHEBUFLEN];
	                                  
} fcpx_t;

#endif                      

typedef struct cp_key {
	uint8_t len;
	void *key;
} cp_key_t;

                           
typedef struct {
	void     *key;
	unsigned key_len;
	void     *iv_key;
	unsigned iv_key_len;
	uint32_t flags;
} cp_raw_key_s;

typedef cp_raw_key_s* cp_raw_key_t;

typedef struct {
	void     *key;
	unsigned key_len;
	cp_key_class_t dp_class;
} cp_wrapped_key_s;

typedef cp_wrapped_key_s* cp_wrapped_key_t;

typedef struct {
	union {
		ino64_t                 inode;
		cp_crypto_id_t  crypto_id;
	};
	uint32_t                        volume;
	pid_t                           pid;
	uid_t                           uid;
	cp_key_revision_t       key_revision;
} cp_cred_s;

typedef cp_cred_s* cp_cred_t;

                                              
typedef int unwrapper_t(cp_cred_t access, const cp_wrapped_key_t wrapped_key_in, cp_raw_key_t key_out);
typedef int rewrapper_t(cp_cred_t access, cp_key_class_t dp_class, const cp_wrapped_key_t wrapped_key_in, cp_wrapped_key_t wrapped_key_out);
typedef int new_key_t(cp_cred_t access, cp_key_class_t dp_class, cp_raw_key_t key_out, cp_wrapped_key_t wrapped_key_out);
typedef int invalidater_t(cp_cred_t access);                       
typedef int backup_key_t(cp_cred_t access, const cp_wrapped_key_t wrapped_key_in, cp_wrapped_key_t wrapped_key_out);

  
                                             
                                                               
                               
   
#define CP_RAW_KEY_WRAPPEDKEY   0x00000001

  
                                                                        
                                                                            
                                  
   
cpx_t cpx_alloc(size_t key_size, bool needs_ctx);
int cpx_alloc_ctx(cpx_t cpx);
void cpx_free_ctx(cpx_t cpx);
void cpx_init(cpx_t, size_t key_len);
void cpx_init_ctx_ptr(cpx_t cpx);
void cpx_free(cpx_t);
void cpx_writeprotect(cpx_t cpx);
__attribute__((const)) size_t cpx_size(size_t key_len);
__attribute__((pure)) bool cpx_is_sep_wrapped_key(const struct cpx *);
void cpx_set_is_sep_wrapped_key(struct cpx *, bool);
__attribute__((pure)) bool cpx_is_composite_key(const struct cpx *);
void cpx_set_is_composite_key(struct cpx *, bool);
__attribute__((pure)) bool cpx_use_offset_for_iv(const struct cpx *);
void cpx_set_use_offset_for_iv(struct cpx *, bool);
__attribute__((pure)) bool cpx_synthetic_offset_for_iv(const struct cpx *);
void cpx_set_synthetic_offset_for_iv(struct cpx *, bool);
__attribute__((pure)) uint16_t cpx_key_len(const struct cpx *);
void cpx_set_key_len(struct cpx *, uint16_t key_len);
__attribute__((pure)) void *cpx_key(const struct cpx *);
aes_encrypt_ctx *cpx_iv_aes_ctx(struct cpx *);
void cpx_flush(cpx_t cpx);
bool cpx_can_copy(const struct cpx *src, const struct cpx *dst);
void cpx_copy(const struct cpx *src, cpx_t dst);
uint16_t cpx_max_key_len(const struct cpx *cpx);
bool cpx_has_key(const struct cpx *cpx);
size_t cpx_sizex(const struct cpx *cpx);
void cpx_set_aes_iv_key(struct cpx *cpx, void *iv_key);

int cp_key_store_action(cp_key_store_action_t);
int cp_key_store_action_for_volume(uuid_t volume_uuid, cp_key_store_action_t action);
cp_key_os_version_t cp_os_version(void);
                                                                
int cp_is_valid_class(int isdir, int32_t protectionclass);

__END_DECLS

#endif                     
#endif                        
