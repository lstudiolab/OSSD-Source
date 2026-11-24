/*
 * Copyright (c) 2000-2020 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
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
  
   
  
                      
  
                                           
   

#ifndef _VM_VM_FAULT_H_
#define _VM_VM_FAULT_H_

#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <mach/boolean.h>
#include <mach/vm_prot.h>
#include <mach/vm_param.h>
#include <mach/vm_behavior.h>

#ifdef  KERNEL_PRIVATE

typedef kern_return_t   vm_fault_return_t;

#define VM_FAULT_SUCCESS                0
#define VM_FAULT_RETRY                  1
#define VM_FAULT_INTERRUPTED            2
#define VM_FAULT_MEMORY_SHORTAGE        3
#define VM_FAULT_MEMORY_ERROR           5
#define VM_FAULT_SUCCESS_NO_VM_PAGE     6                                   

  
                                                           
   

extern kern_return_t vm_fault(
	vm_map_t        map,
	vm_map_offset_t vaddr,
	vm_prot_t       fault_type,
	boolean_t       change_wiring,
#if XNU_KERNEL_PRIVATE
	vm_tag_t        wire_tag,                                                                       
#endif
	int             interruptible,
	pmap_t          pmap,
	vm_map_offset_t pmap_addr)
#if XNU_KERNEL_PRIVATE
__XNU_INTERNAL(vm_fault)
#endif
;

extern void vm_pre_fault(vm_map_offset_t, vm_prot_t);

#ifdef  MACH_KERNEL_PRIVATE

#include <vm/vm_page.h>
#include <vm/vm_object.h>
#include <vm/vm_map.h>

extern void vm_fault_init(void);

                           
extern kern_return_t vm_fault_external(
	vm_map_t        map,
	vm_map_offset_t vaddr,
	vm_prot_t       fault_type,
	boolean_t       change_wiring,
	int             interruptible,
	pmap_t          caller_pmap,
	vm_map_offset_t caller_pmap_addr);

  
                                               
   

extern vm_fault_return_t vm_fault_page(
	                
	vm_object_t     first_object,                                       
	vm_object_offset_t first_offset,                                
	vm_prot_t       fault_type,                                           
	boolean_t       must_be_resident,                                    
	boolean_t       caller_lookup,                                     
	                        
	vm_prot_t       *protection,                                        
	vm_page_t       *result_page,                                          
	              
	vm_page_t       *top_page,                                       
                                                                       
	int             *type_of_fault,                                                        
                                                                                            
	                     
	kern_return_t   *error_code,                                          
	boolean_t       no_zero_fill,                                        
	boolean_t       data_supply,                                      
	vm_object_fault_info_t fault_info);

extern void vm_fault_cleanup(
	vm_object_t     object,
	vm_page_t       top_page);

extern kern_return_t vm_fault_wire(
	vm_map_t        map,
	vm_map_entry_t  entry,
	vm_prot_t       prot,
	vm_tag_t        wire_tag,
	pmap_t          pmap,
	vm_map_offset_t pmap_addr,
	ppnum_t         *physpage_p);

extern void vm_fault_unwire(
	vm_map_t        map,
	vm_map_entry_t  entry,
	boolean_t       deallocate,
	pmap_t          pmap,
	vm_map_offset_t pmap_addr);

extern kern_return_t    vm_fault_copy(
	vm_object_t             src_object,
	vm_object_offset_t      src_offset,
	vm_map_size_t           *copy_size,                        
	vm_object_t             dst_object,
	vm_object_offset_t      dst_offset,
	vm_map_t                dst_map,
	vm_map_version_t         *dst_version,
	int                     interruptible);

extern kern_return_t vm_fault_enter(
	vm_page_t m,
	pmap_t pmap,
	vm_map_offset_t vaddr,
	vm_map_size_t fault_page_size,
	vm_map_offset_t fault_phys_offset,
	vm_prot_t prot,
	vm_prot_t fault_type,
	boolean_t wired,
	boolean_t change_wiring,
	vm_tag_t  wire_tag,                                                                 
	vm_object_fault_info_t fault_info,
	boolean_t *need_retry,
	int *type_of_fault);

extern vm_offset_t kdp_lightweight_fault(
	vm_map_t map,
	vm_offset_t cur_target_addr);

#endif                           

#if XNU_KERNEL_PRIVATE

boolean_t NEED_TO_HARD_THROTTLE_THIS_TASK(void);

#endif

#endif                      

#endif                       
