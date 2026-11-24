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
  
   
  
                        
                      
             
  
                                                       
                                               
   

#ifndef _IPC_IPC_ENTRY_H_
#define _IPC_IPC_ENTRY_H_

#include <mach/mach_types.h>
#include <mach/port.h>
#include <mach/kern_return.h>

#include <kern/kern_types.h>
#include <kern/zalloc.h>

#include <ipc/ipc_types.h>

  
                                               
                                                                 
                                                        
  
                                                        
                                                                
  
                                                            
                                                 
  
                                                    
                                                
  
                                                              
                                                             
                                                                      
  
                                                         
                                           
   

#define IPC_ENTRY_DIST_BITS   12
#define IPC_ENTRY_DIST_MAX    ((1 << IPC_ENTRY_DIST_BITS) - 1)
#ifdef __LP64__
#define IPC_ENTRY_INDEX_BITS  32
#define IPC_ENTRY_INDEX_MAX   (UINT32_MAX)
#else
#define IPC_ENTRY_INDEX_BITS  20
#define IPC_ENTRY_INDEX_MAX   ((1 << IPC_ENTRY_INDEX_BITS) - 1)
#endif

struct ipc_entry {
	struct ipc_object  *XNU_PTRAUTH_SIGNED_PTR("ipc_entry.ie_object") ie_object;
	ipc_entry_bits_t    ie_bits;
	uint32_t            ie_dist  : IPC_ENTRY_DIST_BITS;
	mach_port_index_t   ie_index : IPC_ENTRY_INDEX_BITS;
	union {
		mach_port_index_t next;                                       
		ipc_table_index_t request;                                    
	} index;
};

#define ie_request      index.request
#define ie_next         index.next

#define IE_REQ_NONE             0                               

#define IE_BITS_UREFS_MASK      0x0000ffff                                     
#define IE_BITS_UREFS(bits)     ((bits) & IE_BITS_UREFS_MASK)

#define IE_BITS_TYPE_MASK       0x001f0000                                     
#define IE_BITS_TYPE(bits)      ((bits) & IE_BITS_TYPE_MASK)

#define IE_BITS_EXTYPE_MASK     0x00200000                                         

#ifndef NO_PORT_GEN
#define IE_BITS_GEN_MASK        0xff000000                                 
#define IE_BITS_GEN(bits)       ((bits) & IE_BITS_GEN_MASK)
#define IE_BITS_GEN_ONE         0x04000000                                 
#define IE_BITS_ROLL_POS        22                                                  
#define IE_BITS_ROLL_BITS       2                                                       
#define IE_BITS_ROLL_MASK       (((1 << IE_BITS_ROLL_BITS) - 1) << IE_BITS_ROLL_POS)
#define IE_BITS_ROLL(bits)      ((((bits) & IE_BITS_ROLL_MASK) << 8) ^ IE_BITS_GEN_MASK)

  
                                                                               
                                         
                          
             
             
             
             
   
static inline ipc_entry_bits_t
ipc_entry_new_rollpoint(
	ipc_entry_bits_t rollbits)
{
	rollbits = (rollbits << IE_BITS_ROLL_POS) & IE_BITS_ROLL_MASK;
	ipc_entry_bits_t newgen = IE_BITS_GEN_MASK + IE_BITS_GEN_ONE;
	return newgen | rollbits;
}

  
                                                                                   
                                                                                
   
static inline ipc_entry_bits_t
ipc_entry_new_gen(
	ipc_entry_bits_t oldgen)
{
	ipc_entry_bits_t sum  = (oldgen + IE_BITS_GEN_ONE) & IE_BITS_GEN_MASK;
	ipc_entry_bits_t roll = oldgen & IE_BITS_ROLL_MASK;
	ipc_entry_bits_t newgen = (sum % IE_BITS_ROLL(oldgen)) | roll;
	return newgen;
}

                                                     
static inline boolean_t
ipc_entry_gen_rolled(
	ipc_entry_bits_t oldgen,
	ipc_entry_bits_t newgen)
{
	return (oldgen & IE_BITS_GEN_MASK) > (newgen & IE_BITS_GEN_MASK);
}

#else
#define IE_BITS_GEN_MASK        0
#define IE_BITS_GEN(bits)       0
#define IE_BITS_GEN_ONE         0
#define IE_BITS_ROLL_POS        0
#define IE_BITS_ROLL_MASK       0
#define IE_BITS_ROLL(bits)      (bits)

static inline ipc_entry_bits_t
ipc_entry_new_rollpoint(
	ipc_entry_bits_t rollbits)
{
	return 0;
}

static inline ipc_entry_bits_t
ipc_entry_new_gen(
	ipc_entry_bits_t oldgen)
{
	return 0;
}

static inline boolean_t
ipc_entry_gen_rolled(
	ipc_entry_bits_t oldgen,
	ipc_entry_bits_t newgen)
{
	return FALSE;
}

#endif                     

#define IE_BITS_RIGHT_MASK      0x007fffff                                 
  
                      
   

                                         
extern ipc_entry_t ipc_entry_lookup(
	ipc_space_t             space,
	mach_port_name_t        name);

                                                
extern kern_return_t ipc_entries_hold(
	ipc_space_t             space,
	natural_t               count);

                                                         
extern kern_return_t ipc_entry_claim(
	ipc_space_t             space,
	mach_port_name_t        *namep,
	ipc_entry_t             *entryp);

                                                                  
extern kern_return_t ipc_entry_alloc(
	ipc_space_t             space,
	mach_port_name_t        *namep,
	ipc_entry_t             *entryp);

                                                            
extern kern_return_t ipc_entry_alloc_name(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             *entryp);

                                      
extern void ipc_entry_dealloc(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             entry);

                                        
extern void ipc_entry_modified(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             entry);

                               
extern kern_return_t ipc_entry_grow_table(
	ipc_space_t             space,
	ipc_table_elems_t       target_size);

                                               
extern mach_port_name_t ipc_entry_name_mask(
	mach_port_name_t name);
#endif                         
