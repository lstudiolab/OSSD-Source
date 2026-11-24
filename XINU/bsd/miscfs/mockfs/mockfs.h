/*
 * Copyright (c) 2012 Apple Inc. All rights reserved.
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

#ifndef MOCKFS_H
#define MOCKFS_H

#if MOCKFS

#include <kern/locks.h>
#include <miscfs/mockfs/mockfs_fsnode.h>
#include <sys/kernel_types.h>

  
                                                                                                        
                                                                                                           
                                                                                                       
                                                                                                           
                                                                                                           
                                                                         
  
                                                                                                             
                                                                                    
  
                                                                                                             
                                                                                                            
                                                                                                            
                
  
                                                                          
   

struct mockfs_mount {
	lck_mtx_t       mockfs_mnt_mtx;                                               
	mockfs_fsnode_t mockfs_root;                                       
	boolean_t       mockfs_memory_backed;                                                
	boolean_t       mockfs_physical_memory;                               
	uint32_t        mockfs_memdev_base;                                                                  
	uint64_t        mockfs_memdev_size;                                                             
};

typedef struct mockfs_mount * mockfs_mount_t;

#endif             

#endif               
