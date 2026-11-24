/*
 * Copyright (c) 2020 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
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
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef log_mem_h
#define log_mem_h

#include <stddef.h>
#include <stdint.h>

  
                                                                           
                                                                
   
typedef struct logmem_s {
	lck_spin_t *lm_lock;
	uint8_t     *lm_mem;
	uint8_t     *lm_mem_map;
	size_t      lm_cap_order;
	size_t      lm_min_order;
	size_t      lm_max_order;
	uint32_t    lm_cnt_allocations;
	uint32_t    lm_cnt_failed_size;
	uint32_t    lm_cnt_failed_full;
	uint32_t    lm_cnt_free;
} logmem_t;

  
                                                                            
                                                                               
                                                                           
                                                                               
                 
   
#define LOGMEM_STATIC_INIT(name, size_order, min_order, max_order) \
    SIMPLE_LOCK_DECLARE(name##_lck, 0); \
    logmem_t name = { \
	.lm_lock = (lck_spin_t *)&name##_lck, \
	.lm_mem = (uint8_t[(1 << (size_order))]){ 0 }, \
	.lm_mem_map = (uint8_t[MAX(1, (1 << ((size_order) - (min_order) + 1)) / 8)]){ 0 }, \
	.lm_cap_order = (size_order), \
	.lm_max_order = (max_order), \
	.lm_min_order = (min_order), \
	.lm_cnt_free = (1 << (size_order)) \
    };

  
                                                                                
                                                                                
                                                  
   
void *logmem_alloc(logmem_t *, size_t *);

  
                                                                            
                                                                              
   
void logmem_free(logmem_t *, void *, size_t);

  
                                                             
   
size_t logmem_max_size(const logmem_t *);

#endif                
