/*
 * Copyright (c) 2007-2008 Apple Inc. All rights reserved.
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
#ifndef _KXLD_DICT_H_
#define _KXLD_DICT_H_

#include <sys/types.h>
#if KERNEL
    #include <libkern/kxld_types.h>
#else
    #include "kxld_types.h"
#endif

#include "kxld_array.h"

                                                                                
                                                                             
                                                                          
                                                                          
                                                              
                                         
 
                                                                        
                                                                         
                                                                       
                                   
                                                                                

struct kxld_dict;
typedef struct kxld_dict KXLDDict;
typedef struct kxld_dict_iterator KXLDDictIterator;

typedef u_int (*kxld_dict_hash)(const KXLDDict *dict, const void *key);
typedef u_int (*kxld_dict_cmp)(const void *key1, const void *key2);

struct kxld_dict {
	KXLDArray buckets;                             
	KXLDArray resize_buckets;                               
	kxld_dict_hash hash;                    
	kxld_dict_cmp cmp;                            
	u_int num_entries;                                      
	u_int resize_threshold;                                               
};

struct kxld_dict_iterator {
	u_int idx;
	const KXLDDict *dict;
};

                                                                                
                              
                                                                                

                                       
                                                                               
   
kern_return_t kxld_dict_init(KXLDDict *dict, kxld_dict_hash hash,
    kxld_dict_cmp cmp, u_int num_entries)
__attribute__((nonnull, visibility("hidden")));

                                           
void kxld_dict_iterator_init(KXLDDictIterator *iter, const KXLDDict *dict)
__attribute__((nonnull, visibility("hidden")));

                                                                   
                                             
   
void kxld_dict_clear(KXLDDict *dict)
__attribute__((nonnull, visibility("hidden")));

                                                  
void kxld_dict_deinit(KXLDDict *dict)
__attribute__((nonnull, visibility("hidden")));

                                                                                
           
                                                                                

                                                     
u_int kxld_dict_get_num_entries(const KXLDDict *dict)
__attribute__((pure, nonnull, visibility("hidden")));

                                                                               
                  
   
void * kxld_dict_find(const KXLDDict *dict, const void *key)
__attribute__((pure, nonnull, visibility("hidden")));

                                                                                
           
                                                                                

                                                                               
                           
   
kern_return_t kxld_dict_insert(KXLDDict *dict, const void *key, void *value)
__attribute__((nonnull, visibility("hidden")));

                                                                         
                                                                           
                                                             
   
void kxld_dict_remove(KXLDDict *dict, const void *key, void **value)
__attribute__((nonnull(1, 2), visibility("hidden")));

                                          
void kxld_dict_iterator_get_next(KXLDDictIterator *iter, const void **key,
    void **value)
__attribute__((nonnull, visibility("hidden")));

                                                             
void kxld_dict_iterator_reset(KXLDDictIterator *iter)
__attribute__((nonnull, visibility("hidden")));

                                                                                
         
                                                                                

u_int kxld_dict_string_hash(const KXLDDict *dict, const void *key)
__attribute__((pure, nonnull, visibility("hidden")));
u_int kxld_dict_uint32_hash(const KXLDDict *dict, const void *key)
__attribute__((pure, nonnull, visibility("hidden")));
u_int kxld_dict_kxldaddr_hash(const KXLDDict *dict, const void *key)
__attribute__((pure, nonnull, visibility("hidden")));

u_int kxld_dict_string_cmp(const void *key1, const void *key2)
__attribute__((pure, visibility("hidden")));
u_int kxld_dict_uint32_cmp(const void *key1, const void *key2)
__attribute__((pure, visibility("hidden")));
u_int kxld_dict_kxldaddr_cmp(const void *key1, const void *key2)
__attribute__((pure, visibility("hidden")));

#endif                    
