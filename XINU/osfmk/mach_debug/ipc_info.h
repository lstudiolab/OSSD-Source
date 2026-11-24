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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
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
  
   
  
                              
                      
                    
  
                                               
   

#ifndef _MACH_DEBUG_IPC_INFO_H_
#define _MACH_DEBUG_IPC_INFO_H_

#include <mach/boolean.h>
#include <mach/port.h>
#include <mach/machine/vm_types.h>

  
                                              
                                                        
   

typedef struct ipc_info_space {
	natural_t iis_genno_mask;                                   
	natural_t iis_table_size;                          
	natural_t iis_table_next;                                        
	natural_t iis_tree_size;                                   
	natural_t iis_tree_small;                                                
	natural_t iis_tree_hash;                                                  
} ipc_info_space_t;

typedef struct ipc_info_space_basic {
	natural_t iisb_genno_mask;                                  
	natural_t iisb_table_size;                         
	natural_t iisb_table_next;                                       
	natural_t iisb_table_inuse;                                   
	natural_t iisb_reserved[2];                           
} ipc_info_space_basic_t;

typedef struct ipc_info_name {
	mach_port_name_t iin_name;                                                   
              integer_t iin_collision;                                 
	mach_port_type_t iin_type;                              
	mach_port_urefs_t iin_urefs;                         
	natural_t iin_object;                                          
	natural_t iin_next;                                              
	natural_t iin_hash;                             
} ipc_info_name_t;

typedef ipc_info_name_t *ipc_info_name_array_t;

            
typedef struct ipc_info_tree_name {
	ipc_info_name_t iitn_name;
	mach_port_name_t iitn_lchild;                           
	mach_port_name_t iitn_rchild;                            
} ipc_info_tree_name_t;

typedef ipc_info_tree_name_t *ipc_info_tree_name_array_t;

typedef struct ipc_info_port {
	natural_t iip_port_object;                                  
	natural_t iip_receiver_object;                                         
} ipc_info_port_t;

typedef ipc_info_port_t *exception_handler_info_array_t;

#endif                               
