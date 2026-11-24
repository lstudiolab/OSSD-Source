/*
 * Copyright (c) 2000-2004 Apple Computer, Inc. All rights reserved.
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
  
   
  
                        
                      
             
  
                                                                  
                                               
   

#ifndef _IPC_IPC_TABLE_H_
#define _IPC_IPC_TABLE_H_

#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <mach/vm_param.h>

#include <ipc/ipc_types.h>

  
                                                      
                                                      
                                              
  
                                                                     
                                                               
                                                             
                                                                 
                    
                                                          
                                                     
                                                             
                        
  
  
                                                        
                                                        
                                                             
                                                          
                                                               
                                                            
  
                                                                 
                                                                   
                                                                  
   

struct ipc_table_size {
	ipc_table_elems_t its_size;                                      
};

extern struct ipc_table_size ipc_table_entries[];
extern struct ipc_table_size ipc_table_requests[];

  
                                               
                                                           
                       
   

                      
extern void * ipc_table_alloc(
	vm_size_t       size);

                  
extern void ipc_table_free(
	vm_size_t       size,
	void *          table);

#define it_entries_alloc(its)                                           \
	((ipc_entry_t)                                                  \
	 ipc_table_alloc((its)->its_size * sizeof(struct ipc_entry)))

#define it_entries_free(its, table)                                     \
	ipc_table_free((its)->its_size * sizeof(struct ipc_entry),      \
	               (void *)(table))


#define it_requests_alloc(its)                                          \
	((ipc_port_request_t)                                           \
	 ipc_table_alloc((its)->its_size *                              \
	                 sizeof(struct ipc_port_request)))

#define it_requests_free(its, table)                                    \
	ipc_table_free((its)->its_size *                                \
	               sizeof(struct ipc_port_request),                 \
	               (void *)(table))

extern unsigned int ipc_table_max_entries(void);
extern unsigned int ipc_table_max_requests(void);

#endif                         
