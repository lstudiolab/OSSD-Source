/*
 * Copyright (c) 2000-2005 Apple Computer, Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
   

#ifndef _MACH_DEBUG_ZONE_INFO_H_
#define _MACH_DEBUG_ZONE_INFO_H_

#include <mach/boolean.h>
#include <mach/machine/vm_types.h>

  
                                                                
                                                             
                                             
   

#define ZONE_NAME_MAX_LEN               80

typedef struct zone_name {
	char            zn_name[ZONE_NAME_MAX_LEN];
} zone_name_t;

typedef zone_name_t *zone_name_array_t;


typedef struct zone_info {
	integer_t       zi_count;                                        
	vm_size_t       zi_cur_size;                                    
	vm_size_t       zi_max_size;                                      
	vm_size_t       zi_elem_size;                           
	vm_size_t       zi_alloc_size;                                 
	integer_t       zi_pageable;                        
	integer_t       zi_sleepable;                        
	integer_t       zi_exhaustible;                              
	integer_t       zi_collectable;                                
} zone_info_t;

typedef zone_info_t *zone_info_array_t;


  
                                              
                                                        
   

#define MACH_ZONE_NAME_MAX_LEN          80

typedef struct mach_zone_name {
	char            mzn_name[ZONE_NAME_MAX_LEN];
} mach_zone_name_t;

typedef mach_zone_name_t *mach_zone_name_array_t;

typedef struct mach_zone_info_data {
	uint64_t        mzi_count;                                    
	uint64_t        mzi_cur_size;                                   
	uint64_t        mzi_max_size;                                     
	uint64_t        mzi_elem_size;                          
	uint64_t        mzi_alloc_size;                                
	uint64_t        mzi_sum_size;                                         
	uint64_t        mzi_exhaustible;                                     
	uint64_t        mzi_collectable;                                                     
} mach_zone_info_t;

typedef mach_zone_info_t *mach_zone_info_array_t;

  
                                                                      
                                                                         
                         
   
#define GET_MZI_COLLECTABLE_BYTES(val)  ((val) >> 1)
#define GET_MZI_COLLECTABLE_FLAG(val)   ((val) & 1)

#define SET_MZI_COLLECTABLE_BYTES(val, size)    \
	(val) = ((val) & 1) | ((size) << 1)
#define SET_MZI_COLLECTABLE_FLAG(val, flag)             \
	(val) = (flag) ? ((val) | 1) : (val)

typedef struct task_zone_info_data {
	uint64_t        tzi_count;                                    
	uint64_t        tzi_cur_size;                                   
	uint64_t        tzi_max_size;                                     
	uint64_t        tzi_elem_size;                          
	uint64_t        tzi_alloc_size;                                
	uint64_t        tzi_sum_size;                                         
	uint64_t        tzi_exhaustible;                                     
	uint64_t        tzi_collectable;                                       
	uint64_t        tzi_caller_acct;                                           
	uint64_t        tzi_task_alloc;                                     
	uint64_t        tzi_task_free;                                     
} task_zone_info_t;

typedef task_zone_info_t *task_zone_info_array_t;

#define MACH_MEMORY_INFO_NAME_MAX_LEN   80

typedef struct mach_memory_info {
	uint64_t flags;
	uint64_t site;
	uint64_t size;
	uint64_t free;
	uint64_t largest;
	uint64_t collectable_bytes;
	uint64_t mapped;
	uint64_t peak;
	uint16_t tag;
	uint16_t zone;
	uint16_t _resvA[2];
	uint64_t _resv[3];
	char     name[MACH_MEMORY_INFO_NAME_MAX_LEN];
} mach_memory_info_t;

typedef mach_memory_info_t *mach_memory_info_array_t;

  
                                                                                                             
                                                                                                         
                                         
  
                                                                                                              
                                                                                                    
                                         
   

#define MAX_ZTRACE_DEPTH        15

  
                                         
   

#define ZOP_ALLOC       1
#define ZOP_FREE        0

  
                                                                          
                                 
   
typedef struct zone_btrecord {
	uint32_t    ref_count;                                                                              
	uint32_t        operation_type;                                                          
	uint64_t        bt[MAX_ZTRACE_DEPTH];                          
} zone_btrecord_t;

typedef zone_btrecord_t *zone_btrecord_array_t;

#endif                                
