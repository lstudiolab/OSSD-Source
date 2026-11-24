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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
#ifndef _KXLD_ARRAY_H_
#define _KXLD_ARRAY_H_

#include <sys/queue.h>
#include <sys/types.h>
#if KERNEL
    #include <libkern/kxld_types.h>
#else
    #include "kxld_types.h"
#endif

                                                                                
                                                                          
                                                                       
                                                                             
                                                                           
                                                                             
                                                                          
                                       
 
                                                                       
                                                                              
                                                                              
                                                                              
                                                                              
                                                                         
                                                                        
                                                                             
                                                                       
                                                                             
           
 
               
                                                                             
                       
                                                                       
                                                                             
                                                                         
                                                
 
             
                                                                            
                                                                           
                                                                         
                                     
                                                                                
                                                                                

STAILQ_HEAD(kxld_array_head, kxld_array_pool);

struct kxld_array {
	struct kxld_array_head pools;
	size_t itemsize;                                                           
	size_t pool_capacity;                                                
	u_int pool_maxitems;                                                     
                                                                         
                            
	u_int nitems;                                                                
	u_int maxitems;                                                                 
	u_int npools;                                                     
};

struct kxld_array_pool {
	STAILQ_ENTRY(kxld_array_pool) entries;
	u_char *buffer;                                         
	u_int nitems;                                                       
};

typedef struct kxld_array KXLDArray;
typedef struct kxld_array_head KXLDArrayHead;
typedef struct kxld_array_pool KXLDArrayPool;

                                                                                
                              
                                                                                

                                                                        
kern_return_t kxld_array_init(KXLDArray *array, size_t itemsize, u_int nitems)
__attribute__((nonnull, visibility("hidden")));

                                       
kern_return_t kxld_array_copy(KXLDArray *array, const KXLDArray *src)
__attribute__((nonnull, visibility("hidden")));

                                                
void kxld_array_reset(KXLDArray *array)
__attribute__((visibility("hidden")));

                                               
void kxld_array_clear(KXLDArray *array)
__attribute__((visibility("hidden")));

                                       
void kxld_array_deinit(KXLDArray *array)
__attribute__((visibility("hidden")));

                                                                                
           
                                                                                

                                                                      
void *kxld_array_get_item(const KXLDArray *array, u_int idx)
__attribute__((pure, nonnull, visibility("hidden")));

                                                                        
void *kxld_array_get_slot(const KXLDArray *array, u_int idx)
__attribute__((pure, nonnull, visibility("hidden")));

                                                        
kern_return_t kxld_array_get_index(const KXLDArray *array, const void *item,
    u_int *idx)
__attribute__((nonnull, visibility("hidden")));

                                                                                
           
                                                                                

                                                                              
                                                                             
         
   
kern_return_t kxld_array_resize(KXLDArray *array, u_int nitems)
__attribute__((nonnull, visibility("hidden")));

                                                                             
                 
   
kern_return_t kxld_array_remove(KXLDArray *array, u_int idx)
__attribute__((nonnull, visibility("hidden")));

#endif                     
