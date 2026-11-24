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

#ifndef MOCKFS_FSNODE_H
#define MOCKFS_FSNODE_H

#if MOCKFS

#include <sys/kernel_types.h>

  
                                                                                         
                                                                                         
                                                                                     
                                                                              
  
                                                                                        
                                                                                           
                                    
   
enum mockfs_fsnode_type {
	MOCKFS_ROOT,
	MOCKFS_DEV,
	MOCKFS_FILE
};

  
                                                                                      
                                                                                    
                                         
  
                                                                                       
                  
   
struct mockfs_fsnode {
	uint64_t               size;                                                     
	uint8_t                type;                                               
	mount_t                mnt;                                              
	vnode_t                vp;                                               
	struct mockfs_fsnode * parent;                                           
	                                                                                                
	struct mockfs_fsnode * child_a;                
	struct mockfs_fsnode * child_b;                
};

typedef struct mockfs_fsnode * mockfs_fsnode_t;

  
                                            
   
int mockfs_fsnode_create(mount_t mp, uint8_t type, mockfs_fsnode_t * fsnpp);
int mockfs_fsnode_destroy(mockfs_fsnode_t fsnp);
int mockfs_fsnode_adopt(mockfs_fsnode_t parent, mockfs_fsnode_t child);
int mockfs_fsnode_orphan(mockfs_fsnode_t fsnp);
int mockfs_fsnode_child_by_type(mockfs_fsnode_t parent, uint8_t type, mockfs_fsnode_t * child);
int mockfs_fsnode_vnode(mockfs_fsnode_t fsnp, vnode_t * vpp);
int mockfs_fsnode_drop_vnode(mockfs_fsnode_t fsnp);

#endif             

#endif                      
