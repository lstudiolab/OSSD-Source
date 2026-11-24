/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
  
   
  
                     
                                                    
  
                                          
   

  
                                                                         
                                                                           
                                                                              
                                                                             
                                                                              
                                                                                   
                                                                                  
                
  
                                                                            
                                                                              
                                                                       
                                                                              
                                                                            
                                                                                      
                                                                                
                                                                    
  
                                                                       
                                                    
   

#include <debug.h>

#include <vm_cpm.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>                             
#include <mach/memory_object.h>
#include <mach/std_types.h>                           
#include <mach/upl.h>
#include <mach/vm_attributes.h>
#include <mach/vm_param.h>
#include <mach/vm_statistics.h>
#include <mach/mach_syscalls.h>
#include <mach/sdt.h>

#include <mach/host_priv_server.h>
#include <mach/mach_vm_server.h>
#include <mach/memory_entry_server.h>
#include <mach/vm_map_server.h>

#include <kern/host.h>
#include <kern/kalloc.h>
#include <kern/task.h>
#include <kern/misc_protos.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/memory_object.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>
#include <vm/vm_purgeable_internal.h>
#include <vm/vm_init.h>

#include <san/kasan.h>

#include <libkern/OSDebug.h>
#include <IOKit/IOBSD.h>

vm_size_t        upl_offset_to_pagelist = 0;

#if     VM_CPM
#include <vm/cpm.h>
#endif              

  
                                                                
       
   
kern_return_t
mach_vm_allocate_external(
	vm_map_t                map,
	mach_vm_offset_t        *addr,
	mach_vm_size_t  size,
	int                     flags)
{
	vm_tag_t tag;

	VM_GET_FLAGS_ALIAS(flags, tag);
	return mach_vm_allocate_kernel(map, addr, size, flags, tag);
}

kern_return_t
mach_vm_allocate_kernel(
	vm_map_t                map,
	mach_vm_offset_t        *addr,
	mach_vm_size_t  size,
	int                     flags,
	vm_tag_t    tag)
{
	vm_map_offset_t map_addr;
	vm_map_size_t   map_size;
	kern_return_t   result;
	boolean_t       anywhere;

	                                      
	if (flags & ~VM_FLAGS_USER_ALLOCATE) {
		return KERN_INVALID_ARGUMENT;
	}

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}
	if (size == 0) {
		*addr = 0;
		return KERN_SUCCESS;
	}

	anywhere = ((VM_FLAGS_ANYWHERE & flags) != 0);
	if (anywhere) {
		  
                                                              
                                                                
                                                             
                                                           
                                                         
                                                          
                                                          
                                                     
     
		map_addr = vm_map_min(map);
		if (map_addr == 0) {
			map_addr += VM_MAP_PAGE_SIZE(map);
		}
	} else {
		map_addr = vm_map_trunc_page(*addr,
		    VM_MAP_PAGE_MASK(map));
	}
	map_size = vm_map_round_page(size,
	    VM_MAP_PAGE_MASK(map));
	if (map_size == 0) {
		return KERN_INVALID_ARGUMENT;
	}

	result = vm_map_enter(
		map,
		&map_addr,
		map_size,
		(vm_map_offset_t)0,
		flags,
		VM_MAP_KERNEL_FLAGS_NONE,
		tag,
		VM_OBJECT_NULL,
		(vm_object_offset_t)0,
		FALSE,
		VM_PROT_DEFAULT,
		VM_PROT_ALL,
		VM_INHERIT_DEFAULT);

	*addr = map_addr;
	return result;
}

  
              
                                                                   
                                                         
   
kern_return_t
vm_allocate_external(
	vm_map_t        map,
	vm_offset_t     *addr,
	vm_size_t       size,
	int             flags)
{
	vm_tag_t tag;

	VM_GET_FLAGS_ALIAS(flags, tag);
	return vm_allocate_kernel(map, addr, size, flags, tag);
}

kern_return_t
vm_allocate_kernel(
	vm_map_t        map,
	vm_offset_t     *addr,
	vm_size_t       size,
	int         flags,
	vm_tag_t    tag)
{
	vm_map_offset_t map_addr;
	vm_map_size_t   map_size;
	kern_return_t   result;
	boolean_t       anywhere;

	                                      
	if (flags & ~VM_FLAGS_USER_ALLOCATE) {
		return KERN_INVALID_ARGUMENT;
	}

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}
	if (size == 0) {
		*addr = 0;
		return KERN_SUCCESS;
	}

	anywhere = ((VM_FLAGS_ANYWHERE & flags) != 0);
	if (anywhere) {
		  
                                                              
                                                                
                                                             
                                                           
                                                         
                                                          
                                                          
                                                     
     
		map_addr = vm_map_min(map);
		if (map_addr == 0) {
			map_addr += VM_MAP_PAGE_SIZE(map);
		}
	} else {
		map_addr = vm_map_trunc_page(*addr,
		    VM_MAP_PAGE_MASK(map));
	}
	map_size = vm_map_round_page(size,
	    VM_MAP_PAGE_MASK(map));
	if (map_size == 0) {
		return KERN_INVALID_ARGUMENT;
	}

	result = vm_map_enter(
		map,
		&map_addr,
		map_size,
		(vm_map_offset_t)0,
		flags,
		VM_MAP_KERNEL_FLAGS_NONE,
		tag,
		VM_OBJECT_NULL,
		(vm_object_offset_t)0,
		FALSE,
		VM_PROT_DEFAULT,
		VM_PROT_ALL,
		VM_INHERIT_DEFAULT);

#if KASAN
	if (result == KERN_SUCCESS && map->pmap == kernel_pmap) {
		kasan_notify_address(map_addr, map_size);
	}
#endif

	*addr = CAST_DOWN(vm_offset_t, map_addr);
	return result;
}

  
                       
                                                      
                         
   
kern_return_t
mach_vm_deallocate(
	vm_map_t                map,
	mach_vm_offset_t        start,
	mach_vm_size_t  size)
{
	if ((map == VM_MAP_NULL) || (start + size < start)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (size == (mach_vm_offset_t) 0) {
		return KERN_SUCCESS;
	}

	return vm_map_remove(map,
	           vm_map_trunc_page(start,
	           VM_MAP_PAGE_MASK(map)),
	           vm_map_round_page(start + size,
	           VM_MAP_PAGE_MASK(map)),
	           VM_MAP_REMOVE_NO_FLAGS);
}

  
                  
                                                      
                                                       
                       
   
kern_return_t
vm_deallocate(
	vm_map_t                map,
	vm_offset_t             start,
	vm_size_t               size)
{
	if ((map == VM_MAP_NULL) || (start + size < start)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (size == (vm_offset_t) 0) {
		return KERN_SUCCESS;
	}

	return vm_map_remove(map,
	           vm_map_trunc_page(start,
	           VM_MAP_PAGE_MASK(map)),
	           vm_map_round_page(start + size,
	           VM_MAP_PAGE_MASK(map)),
	           VM_MAP_REMOVE_NO_FLAGS);
}

  
                    
                                                     
                 
   
kern_return_t
mach_vm_inherit(
	vm_map_t                map,
	mach_vm_offset_t        start,
	mach_vm_size_t  size,
	vm_inherit_t            new_inheritance)
{
	if ((map == VM_MAP_NULL) || (start + size < start) ||
	    (new_inheritance > VM_INHERIT_LAST_VALID)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (size == 0) {
		return KERN_SUCCESS;
	}

	return vm_map_inherit(map,
	           vm_map_trunc_page(start,
	           VM_MAP_PAGE_MASK(map)),
	           vm_map_round_page(start + size,
	           VM_MAP_PAGE_MASK(map)),
	           new_inheritance);
}

  
               
                                                     
                                            
   
kern_return_t
vm_inherit(
	vm_map_t                map,
	vm_offset_t             start,
	vm_size_t               size,
	vm_inherit_t            new_inheritance)
{
	if ((map == VM_MAP_NULL) || (start + size < start) ||
	    (new_inheritance > VM_INHERIT_LAST_VALID)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (size == 0) {
		return KERN_SUCCESS;
	}

	return vm_map_inherit(map,
	           vm_map_trunc_page(start,
	           VM_MAP_PAGE_MASK(map)),
	           vm_map_round_page(start + size,
	           VM_MAP_PAGE_MASK(map)),
	           new_inheritance);
}

  
                    
                                                    
                 
   

kern_return_t
mach_vm_protect(
	vm_map_t                map,
	mach_vm_offset_t        start,
	mach_vm_size_t  size,
	boolean_t               set_maximum,
	vm_prot_t               new_protection)
{
	if ((map == VM_MAP_NULL) || (start + size < start) ||
	    (new_protection & ~(VM_PROT_ALL | VM_PROT_COPY))) {
		return KERN_INVALID_ARGUMENT;
	}

	if (size == 0) {
		return KERN_SUCCESS;
	}

	return vm_map_protect(map,
	           vm_map_trunc_page(start,
	           VM_MAP_PAGE_MASK(map)),
	           vm_map_round_page(start + size,
	           VM_MAP_PAGE_MASK(map)),
	           new_protection,
	           set_maximum);
}

  
               
                                                    
                                                     
                                  
   

kern_return_t
vm_protect(
	vm_map_t                map,
	vm_offset_t             start,
	vm_size_t               size,
	boolean_t               set_maximum,
	vm_prot_t               new_protection)
{
	if ((map == VM_MAP_NULL) || (start + size < start) ||
	    (new_protection & ~(VM_PROT_ALL | VM_PROT_COPY))) {
		return KERN_INVALID_ARGUMENT;
	}

	if (size == 0) {
		return KERN_SUCCESS;
	}

	return vm_map_protect(map,
	           vm_map_trunc_page(start,
	           VM_MAP_PAGE_MASK(map)),
	           vm_map_round_page(start + size,
	           VM_MAP_PAGE_MASK(map)),
	           new_protection,
	           set_maximum);
}

  
                               
                                                         
                                    
   
kern_return_t
mach_vm_machine_attribute(
	vm_map_t                        map,
	mach_vm_address_t               addr,
	mach_vm_size_t          size,
	vm_machine_attribute_t  attribute,
	vm_machine_attribute_val_t* value)                          
{
	if ((map == VM_MAP_NULL) || (addr + size < addr)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (size == 0) {
		return KERN_SUCCESS;
	}

	return vm_map_machine_attribute(
		map,
		vm_map_trunc_page(addr,
		VM_MAP_PAGE_MASK(map)),
		vm_map_round_page(addr + size,
		VM_MAP_PAGE_MASK(map)),
		attribute,
		value);
}

  
                         
                                                         
                                                           
                                                    
   
kern_return_t
vm_machine_attribute(
	vm_map_t        map,
	vm_address_t    addr,
	vm_size_t       size,
	vm_machine_attribute_t  attribute,
	vm_machine_attribute_val_t* value)                          
{
	if ((map == VM_MAP_NULL) || (addr + size < addr)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (size == 0) {
		return KERN_SUCCESS;
	}

	return vm_map_machine_attribute(
		map,
		vm_map_trunc_page(addr,
		VM_MAP_PAGE_MASK(map)),
		vm_map_round_page(addr + size,
		VM_MAP_PAGE_MASK(map)),
		attribute,
		value);
}

  
                 
                                                                        
  
                                                                        
                                                                      
                                                                        
                      
  
                                                                     
                                  
  
   
kern_return_t
mach_vm_read(
	vm_map_t                map,
	mach_vm_address_t       addr,
	mach_vm_size_t  size,
	pointer_t               *data,
	mach_msg_type_number_t  *data_size)
{
	kern_return_t   error;
	vm_map_copy_t   ipc_address;

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if ((mach_msg_type_number_t) size != size) {
		return KERN_INVALID_ARGUMENT;
	}

	error = vm_map_copyin(map,
	    (vm_map_address_t)addr,
	    (vm_map_size_t)size,
	    FALSE,                               
	    &ipc_address);

	if (KERN_SUCCESS == error) {
		*data = (pointer_t) ipc_address;
		*data_size = (mach_msg_type_number_t) size;
		assert(*data_size == size);
	}
	return error;
}

  
            
                                                                        
                                                                           
  
                                                                        
                                                                      
                                                                        
                      
   
kern_return_t
vm_read(
	vm_map_t                map,
	vm_address_t            addr,
	vm_size_t               size,
	pointer_t               *data,
	mach_msg_type_number_t  *data_size)
{
	kern_return_t   error;
	vm_map_copy_t   ipc_address;

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	mach_msg_type_number_t dsize;
	if (os_convert_overflow(size, &dsize)) {
		  
                                                       
                                                             
                         
                                         
     
		return KERN_INVALID_ARGUMENT;
	}

	error = vm_map_copyin(map,
	    (vm_map_address_t)addr,
	    (vm_map_size_t)size,
	    FALSE,                               
	    &ipc_address);

	if (KERN_SUCCESS == error) {
		*data = (pointer_t) ipc_address;
		*data_size = dsize;
		assert(*data_size == size);
	}
	return error;
}

  
                      
                                                         
  
                                                         
                                                         
                 
   
kern_return_t
mach_vm_read_list(
	vm_map_t                        map,
	mach_vm_read_entry_t            data_list,
	natural_t                       count)
{
	mach_msg_type_number_t  i;
	kern_return_t   error;
	vm_map_copy_t   copy;

	if (map == VM_MAP_NULL ||
	    count > VM_MAP_ENTRY_MAX) {
		return KERN_INVALID_ARGUMENT;
	}

	error = KERN_SUCCESS;
	for (i = 0; i < count; i++) {
		vm_map_address_t map_addr;
		vm_map_size_t map_size;

		map_addr = (vm_map_address_t)(data_list[i].address);
		map_size = (vm_map_size_t)(data_list[i].size);

		if (map_size != 0) {
			error = vm_map_copyin(map,
			    map_addr,
			    map_size,
			    FALSE,                               
			    &copy);
			if (KERN_SUCCESS == error) {
				error = vm_map_copyout(
					current_task()->map,
					&map_addr,
					copy);
				if (KERN_SUCCESS == error) {
					data_list[i].address = map_addr;
					continue;
				}
				vm_map_copy_discard(copy);
			}
		}
		data_list[i].address = (mach_vm_address_t)0;
		data_list[i].size = (mach_vm_size_t)0;
	}
	return error;
}

  
                 
                                                         
  
                                                         
                                                         
                 
  
                                                         
                                                       
                           
  
                                                         
                                                         
                                                       
                                                       
                                           
   

kern_return_t
vm_read_list(
	vm_map_t                map,
	vm_read_entry_t data_list,
	natural_t               count)
{
	mach_msg_type_number_t  i;
	kern_return_t   error;
	vm_map_copy_t   copy;

	if (map == VM_MAP_NULL ||
	    count > VM_MAP_ENTRY_MAX) {
		return KERN_INVALID_ARGUMENT;
	}

	error = KERN_SUCCESS;
	for (i = 0; i < count; i++) {
		vm_map_address_t map_addr;
		vm_map_size_t map_size;

		map_addr = (vm_map_address_t)(data_list[i].address);
		map_size = (vm_map_size_t)(data_list[i].size);

		if (map_size != 0) {
			error = vm_map_copyin(map,
			    map_addr,
			    map_size,
			    FALSE,                               
			    &copy);
			if (KERN_SUCCESS == error) {
				error = vm_map_copyout(current_task()->map,
				    &map_addr,
				    copy);
				if (KERN_SUCCESS == error) {
					data_list[i].address =
					    CAST_DOWN(vm_offset_t, map_addr);
					continue;
				}
				vm_map_copy_discard(copy);
			}
		}
		data_list[i].address = (mach_vm_address_t)0;
		data_list[i].size = (mach_vm_size_t)0;
	}
	return error;
}

  
                           
                                                                    
                     
  
                                                                  
                                                                
                                                                        
   

kern_return_t
mach_vm_read_overwrite(
	vm_map_t                map,
	mach_vm_address_t       address,
	mach_vm_size_t  size,
	mach_vm_address_t       data,
	mach_vm_size_t  *data_size)
{
	kern_return_t   error;
	vm_map_copy_t   copy;

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	error = vm_map_copyin(map, (vm_map_address_t)address,
	    (vm_map_size_t)size, FALSE, &copy);

	if (KERN_SUCCESS == error) {
		if (copy) {
			assertf(copy->size == (vm_map_size_t) size, "Req size: 0x%llx, Copy size: 0x%llx\n", (uint64_t) size, (uint64_t) copy->size);
		}

		error = vm_map_copy_overwrite(current_thread()->map,
		    (vm_map_address_t)data,
		    copy, (vm_map_size_t) size, FALSE);
		if (KERN_SUCCESS == error) {
			*data_size = size;
			return error;
		}
		vm_map_copy_discard(copy);
	}
	return error;
}

  
                      
                                                                    
                     
  
                                                                  
                                                                  
                                                                    
                                                                 
           
   

kern_return_t
vm_read_overwrite(
	vm_map_t        map,
	vm_address_t    address,
	vm_size_t       size,
	vm_address_t    data,
	vm_size_t       *data_size)
{
	kern_return_t   error;
	vm_map_copy_t   copy;

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	error = vm_map_copyin(map, (vm_map_address_t)address,
	    (vm_map_size_t)size, FALSE, &copy);

	if (KERN_SUCCESS == error) {
		if (copy) {
			assertf(copy->size == (vm_map_size_t) size, "Req size: 0x%llx, Copy size: 0x%llx\n", (uint64_t) size, (uint64_t) copy->size);
		}

		error = vm_map_copy_overwrite(current_thread()->map,
		    (vm_map_address_t)data,
		    copy, (vm_map_size_t) size, FALSE);
		if (KERN_SUCCESS == error) {
			*data_size = size;
			return error;
		}
		vm_map_copy_discard(copy);
	}
	return error;
}


  
                  
                                                               
                          
   
kern_return_t
mach_vm_write(
	vm_map_t                        map,
	mach_vm_address_t               address,
	pointer_t                       data,
	mach_msg_type_number_t          size)
{
	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return vm_map_copy_overwrite(map, (vm_map_address_t)address,
	           (vm_map_copy_t) data, size, FALSE                        );
}

  
             
                                                               
                          
  
                                                               
                                                                  
                                                              
                           
   
kern_return_t
vm_write(
	vm_map_t                        map,
	vm_address_t                    address,
	pointer_t                       data,
	mach_msg_type_number_t          size)
{
	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return vm_map_copy_overwrite(map, (vm_map_address_t)address,
	           (vm_map_copy_t) data, size, FALSE                        );
}

  
                 
                                                                
                                                               
                     
   
kern_return_t
mach_vm_copy(
	vm_map_t                map,
	mach_vm_address_t       source_address,
	mach_vm_size_t  size,
	mach_vm_address_t       dest_address)
{
	vm_map_copy_t copy;
	kern_return_t kr;

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	kr = vm_map_copyin(map, (vm_map_address_t)source_address,
	    (vm_map_size_t)size, FALSE, &copy);

	if (KERN_SUCCESS == kr) {
		if (copy) {
			assertf(copy->size == (vm_map_size_t) size, "Req size: 0x%llx, Copy size: 0x%llx\n", (uint64_t) size, (uint64_t) copy->size);
		}

		kr = vm_map_copy_overwrite(map,
		    (vm_map_address_t)dest_address,
		    copy, (vm_map_size_t) size, FALSE                        );

		if (KERN_SUCCESS != kr) {
			vm_map_copy_discard(copy);
		}
	}
	return kr;
}

kern_return_t
vm_copy(
	vm_map_t        map,
	vm_address_t    source_address,
	vm_size_t       size,
	vm_address_t    dest_address)
{
	vm_map_copy_t copy;
	kern_return_t kr;

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	kr = vm_map_copyin(map, (vm_map_address_t)source_address,
	    (vm_map_size_t)size, FALSE, &copy);

	if (KERN_SUCCESS == kr) {
		if (copy) {
			assertf(copy->size == (vm_map_size_t) size, "Req size: 0x%llx, Copy size: 0x%llx\n", (uint64_t) size, (uint64_t) copy->size);
		}

		kr = vm_map_copy_overwrite(map,
		    (vm_map_address_t)dest_address,
		    copy, (vm_map_size_t) size, FALSE                        );

		if (KERN_SUCCESS != kr) {
			vm_map_copy_discard(copy);
		}
	}
	return kr;
}

  
                
                                                     
  
                                                     
                          
                                                       
                                                    
                        
  
   
kern_return_t
mach_vm_map_external(
	vm_map_t                target_map,
	mach_vm_offset_t        *address,
	mach_vm_size_t          initial_size,
	mach_vm_offset_t        mask,
	int                     flags,
	ipc_port_t              port,
	vm_object_offset_t      offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	vm_tag_t tag;

	VM_GET_FLAGS_ALIAS(flags, tag);
	return mach_vm_map_kernel(target_map, address, initial_size, mask,
	           flags, VM_MAP_KERNEL_FLAGS_NONE, tag,
	           port, offset, copy,
	           cur_protection, max_protection,
	           inheritance);
}

kern_return_t
mach_vm_map_kernel(
	vm_map_t                target_map,
	mach_vm_offset_t        *address,
	mach_vm_size_t  initial_size,
	mach_vm_offset_t        mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	ipc_port_t              port,
	vm_object_offset_t      offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	kern_return_t           kr;
	vm_map_offset_t         vmmaddr;

	vmmaddr = (vm_map_offset_t) *address;

	                                      
	if (flags & ~VM_FLAGS_USER_MAP) {
		return KERN_INVALID_ARGUMENT;
	}

	kr = vm_map_enter_mem_object(target_map,
	    &vmmaddr,
	    initial_size,
	    mask,
	    flags,
	    vmk_flags,
	    tag,
	    port,
	    offset,
	    copy,
	    cur_protection,
	    max_protection,
	    inheritance);

#if KASAN
	if (kr == KERN_SUCCESS && target_map->pmap == kernel_pmap) {
		kasan_notify_address(vmmaddr, initial_size);
	}
#endif

	*address = vmmaddr;
	return kr;
}


                      
kern_return_t
vm_map_64_external(
	vm_map_t                target_map,
	vm_offset_t             *address,
	vm_size_t               size,
	vm_offset_t             mask,
	int                     flags,
	ipc_port_t              port,
	vm_object_offset_t      offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	vm_tag_t tag;

	VM_GET_FLAGS_ALIAS(flags, tag);
	return vm_map_64_kernel(target_map, address, size, mask,
	           flags, VM_MAP_KERNEL_FLAGS_NONE,
	           tag, port, offset, copy,
	           cur_protection, max_protection,
	           inheritance);
}

kern_return_t
vm_map_64_kernel(
	vm_map_t                target_map,
	vm_offset_t             *address,
	vm_size_t               size,
	vm_offset_t             mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	ipc_port_t              port,
	vm_object_offset_t      offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	mach_vm_address_t map_addr;
	mach_vm_size_t map_size;
	mach_vm_offset_t map_mask;
	kern_return_t kr;

	map_addr = (mach_vm_address_t)*address;
	map_size = (mach_vm_size_t)size;
	map_mask = (mach_vm_offset_t)mask;

	kr = mach_vm_map_kernel(target_map, &map_addr, map_size, map_mask,
	    flags, vmk_flags, tag,
	    port, offset, copy,
	    cur_protection, max_protection, inheritance);
	*address = CAST_DOWN(vm_offset_t, map_addr);
	return kr;
}

                                  
kern_return_t
vm_map_external(
	vm_map_t                target_map,
	vm_offset_t             *address,
	vm_size_t               size,
	vm_offset_t             mask,
	int                     flags,
	ipc_port_t              port,
	vm_offset_t             offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	vm_tag_t tag;

	VM_GET_FLAGS_ALIAS(flags, tag);
	return vm_map_kernel(target_map, address, size, mask,
	           flags, VM_MAP_KERNEL_FLAGS_NONE, tag,
	           port, offset, copy,
	           cur_protection, max_protection, inheritance);
}

kern_return_t
vm_map_kernel(
	vm_map_t                target_map,
	vm_offset_t             *address,
	vm_size_t               size,
	vm_offset_t             mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	ipc_port_t              port,
	vm_offset_t             offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	mach_vm_address_t map_addr;
	mach_vm_size_t map_size;
	mach_vm_offset_t map_mask;
	vm_object_offset_t obj_offset;
	kern_return_t kr;

	map_addr = (mach_vm_address_t)*address;
	map_size = (mach_vm_size_t)size;
	map_mask = (mach_vm_offset_t)mask;
	obj_offset = (vm_object_offset_t)offset;

	kr = mach_vm_map_kernel(target_map, &map_addr, map_size, map_mask,
	    flags, vmk_flags, tag,
	    port, obj_offset, copy,
	    cur_protection, max_protection, inheritance);
	*address = CAST_DOWN(vm_offset_t, map_addr);
	return kr;
}

  
                      
                                                                                  
                                       
   
kern_return_t
mach_vm_remap_new_external(
	vm_map_t                target_map,
	mach_vm_offset_t        *address,
	mach_vm_size_t  size,
	mach_vm_offset_t        mask,
	int                     flags,
	mach_port_t             src_tport,
	mach_vm_offset_t        memory_address,
	boolean_t               copy,
	vm_prot_t               *cur_protection,               
	vm_prot_t               *max_protection,               
	vm_inherit_t            inheritance)
{
	vm_tag_t tag;
	vm_map_offset_t         map_addr;
	kern_return_t           kr;
	vm_map_t src_map;

	flags |= VM_FLAGS_RETURN_DATA_ADDR;
	VM_GET_FLAGS_ALIAS(flags, tag);

	                                      
	if (flags & ~VM_FLAGS_USER_REMAP) {
		return KERN_INVALID_ARGUMENT;
	}

	if (target_map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if ((*cur_protection & ~VM_PROT_ALL) ||
	    (*max_protection & ~VM_PROT_ALL) ||
	    (*cur_protection & *max_protection) != *cur_protection) {
		return KERN_INVALID_ARGUMENT;
	}
	if ((*max_protection & (VM_PROT_WRITE | VM_PROT_EXECUTE)) ==
	    (VM_PROT_WRITE | VM_PROT_EXECUTE)) {
		  
                  
                                   
     
		return KERN_PROTECTION_FAILURE;
	}

	if (copy || *max_protection == VM_PROT_READ || *max_protection == VM_PROT_NONE) {
		src_map = convert_port_to_map_read(src_tport);
	} else {
		src_map = convert_port_to_map(src_tport);
	}

	if (src_map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	map_addr = (vm_map_offset_t)*address;

	kr = vm_map_remap(target_map,
	    &map_addr,
	    size,
	    mask,
	    flags,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    tag,
	    src_map,
	    memory_address,
	    copy,
	    cur_protection,                
	    max_protection,                
	    inheritance);

	*address = map_addr;
	vm_map_deallocate(src_map);

	if (kr == KERN_SUCCESS) {
		ipc_port_release_send(src_tport);                          
	}
	return kr;
}

  
                  
                                                      
                                                    
                                                      
                                  
   
kern_return_t
mach_vm_remap_external(
	vm_map_t                target_map,
	mach_vm_offset_t        *address,
	mach_vm_size_t  size,
	mach_vm_offset_t        mask,
	int                     flags,
	vm_map_t                src_map,
	mach_vm_offset_t        memory_address,
	boolean_t               copy,
	vm_prot_t               *cur_protection,             
	vm_prot_t               *max_protection,             
	vm_inherit_t            inheritance)
{
	vm_tag_t tag;
	VM_GET_FLAGS_ALIAS(flags, tag);

	return mach_vm_remap_kernel(target_map, address, size, mask, flags, tag, src_map, memory_address,
	           copy, cur_protection, max_protection, inheritance);
}

kern_return_t
mach_vm_remap_kernel(
	vm_map_t                target_map,
	mach_vm_offset_t        *address,
	mach_vm_size_t  size,
	mach_vm_offset_t        mask,
	int                     flags,
	vm_tag_t                tag,
	vm_map_t                src_map,
	mach_vm_offset_t        memory_address,
	boolean_t               copy,
	vm_prot_t               *cur_protection,            
	vm_prot_t               *max_protection,            
	vm_inherit_t            inheritance)
{
	vm_map_offset_t         map_addr;
	kern_return_t           kr;

	if (VM_MAP_NULL == target_map || VM_MAP_NULL == src_map) {
		return KERN_INVALID_ARGUMENT;
	}

	                                      
	if (flags & ~VM_FLAGS_USER_REMAP) {
		return KERN_INVALID_ARGUMENT;
	}

	map_addr = (vm_map_offset_t)*address;

	*cur_protection = VM_PROT_NONE;
	*max_protection = VM_PROT_NONE;

	kr = vm_map_remap(target_map,
	    &map_addr,
	    size,
	    mask,
	    flags,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    tag,
	    src_map,
	    memory_address,
	    copy,
	    cur_protection,                
	    max_protection,                
	    inheritance);
	*address = map_addr;
	return kr;
}

  
                 
                                                                             
                                       
   
kern_return_t
vm_remap_new_external(
	vm_map_t                target_map,
	vm_offset_t             *address,
	vm_size_t               size,
	vm_offset_t             mask,
	int                     flags,
	mach_port_t             src_tport,
	vm_offset_t             memory_address,
	boolean_t               copy,
	vm_prot_t               *cur_protection,                   
	vm_prot_t               *max_protection,                   
	vm_inherit_t            inheritance)
{
	vm_tag_t tag;
	vm_map_offset_t         map_addr;
	kern_return_t           kr;
	vm_map_t src_map;

	flags |= VM_FLAGS_RETURN_DATA_ADDR;
	VM_GET_FLAGS_ALIAS(flags, tag);

	                                      
	if (flags & ~VM_FLAGS_USER_REMAP) {
		return KERN_INVALID_ARGUMENT;
	}

	if (target_map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if ((*cur_protection & ~VM_PROT_ALL) ||
	    (*max_protection & ~VM_PROT_ALL) ||
	    (*cur_protection & *max_protection) != *cur_protection) {
		return KERN_INVALID_ARGUMENT;
	}
	if ((*max_protection & (VM_PROT_WRITE | VM_PROT_EXECUTE)) ==
	    (VM_PROT_WRITE | VM_PROT_EXECUTE)) {
		  
                  
                                   
     
		return KERN_PROTECTION_FAILURE;
	}

	if (copy || *max_protection == VM_PROT_READ || *max_protection == VM_PROT_NONE) {
		src_map = convert_port_to_map_read(src_tport);
	} else {
		src_map = convert_port_to_map(src_tport);
	}

	if (src_map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	map_addr = (vm_map_offset_t)*address;

	kr = vm_map_remap(target_map,
	    &map_addr,
	    size,
	    mask,
	    flags,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    tag,
	    src_map,
	    memory_address,
	    copy,
	    cur_protection,               
	    max_protection,               
	    inheritance);

	*address = CAST_DOWN(vm_offset_t, map_addr);
	vm_map_deallocate(src_map);

	if (kr == KERN_SUCCESS) {
		ipc_port_release_send(src_tport);                         
	}
	return kr;
}

  
             
                                                      
                                                    
                                                      
                                  
  
                                                      
                                                       
                   
   
kern_return_t
vm_remap_external(
	vm_map_t                target_map,
	vm_offset_t             *address,
	vm_size_t               size,
	vm_offset_t             mask,
	int                     flags,
	vm_map_t                src_map,
	vm_offset_t             memory_address,
	boolean_t               copy,
	vm_prot_t               *cur_protection,             
	vm_prot_t               *max_protection,             
	vm_inherit_t            inheritance)
{
	vm_tag_t tag;
	VM_GET_FLAGS_ALIAS(flags, tag);

	return vm_remap_kernel(target_map, address, size, mask, flags, tag, src_map,
	           memory_address, copy, cur_protection, max_protection, inheritance);
}

kern_return_t
vm_remap_kernel(
	vm_map_t                target_map,
	vm_offset_t             *address,
	vm_size_t               size,
	vm_offset_t             mask,
	int                     flags,
	vm_tag_t                tag,
	vm_map_t                src_map,
	vm_offset_t             memory_address,
	boolean_t               copy,
	vm_prot_t               *cur_protection,             
	vm_prot_t               *max_protection,             
	vm_inherit_t            inheritance)
{
	vm_map_offset_t         map_addr;
	kern_return_t           kr;

	if (VM_MAP_NULL == target_map || VM_MAP_NULL == src_map) {
		return KERN_INVALID_ARGUMENT;
	}

	                                      
	if (flags & ~VM_FLAGS_USER_REMAP) {
		return KERN_INVALID_ARGUMENT;
	}

	map_addr = (vm_map_offset_t)*address;

	*cur_protection = VM_PROT_NONE;
	*max_protection = VM_PROT_NONE;

	kr = vm_map_remap(target_map,
	    &map_addr,
	    size,
	    mask,
	    flags,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    tag,
	    src_map,
	    memory_address,
	    copy,
	    cur_protection,               
	    max_protection,               
	    inheritance);
	*address = CAST_DOWN(vm_offset_t, map_addr);
	return kr;
}

  
                                                                                
                                                            
   
#include <mach/mach_host_server.h>
  
               
                                                      
                                                    
                          
  
                                                 
   
kern_return_t
mach_vm_wire_external(
	host_priv_t             host_priv,
	vm_map_t                map,
	mach_vm_offset_t        start,
	mach_vm_size_t  size,
	vm_prot_t               access)
{
	return mach_vm_wire_kernel(host_priv, map, start, size, access, VM_KERN_MEMORY_MLOCK);
}

kern_return_t
mach_vm_wire_kernel(
	host_priv_t             host_priv,
	vm_map_t                map,
	mach_vm_offset_t        start,
	mach_vm_size_t  size,
	vm_prot_t               access,
	vm_tag_t                tag)
{
	kern_return_t           rc;

	if (host_priv == HOST_PRIV_NULL) {
		return KERN_INVALID_HOST;
	}

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_TASK;
	}

	if (access & ~VM_PROT_ALL || (start + size < start)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (access != VM_PROT_NONE) {
		rc = vm_map_wire_kernel(map,
		    vm_map_trunc_page(start,
		    VM_MAP_PAGE_MASK(map)),
		    vm_map_round_page(start + size,
		    VM_MAP_PAGE_MASK(map)),
		    access, tag,
		    TRUE);
	} else {
		rc = vm_map_unwire(map,
		    vm_map_trunc_page(start,
		    VM_MAP_PAGE_MASK(map)),
		    vm_map_round_page(start + size,
		    VM_MAP_PAGE_MASK(map)),
		    TRUE);
	}
	return rc;
}

  
            
                                                      
                                                    
                          
  
                                                 
   
kern_return_t
vm_wire(
	host_priv_t             host_priv,
	vm_map_t                map,
	vm_offset_t             start,
	vm_size_t               size,
	vm_prot_t               access)
{
	kern_return_t           rc;

	if (host_priv == HOST_PRIV_NULL) {
		return KERN_INVALID_HOST;
	}

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_TASK;
	}

	if ((access & ~VM_PROT_ALL) || (start + size < start)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (size == 0) {
		rc = KERN_SUCCESS;
	} else if (access != VM_PROT_NONE) {
		rc = vm_map_wire_kernel(map,
		    vm_map_trunc_page(start,
		    VM_MAP_PAGE_MASK(map)),
		    vm_map_round_page(start + size,
		    VM_MAP_PAGE_MASK(map)),
		    access, VM_KERN_MEMORY_OSFMK,
		    TRUE);
	} else {
		rc = vm_map_unwire(map,
		    vm_map_trunc_page(start,
		    VM_MAP_PAGE_MASK(map)),
		    vm_map_round_page(start + size,
		    VM_MAP_PAGE_MASK(map)),
		    TRUE);
	}
	return rc;
}

  
           
  
                                                                 
                                                                       
                  
  
                               
                                                           
                         
  
                                                                    
                                              
                                     
  
                                             
                                             
                           
  
                                                              
                                             
                                          
                                     
  
          
                                        
                                                             
                            
                                                        
   

kern_return_t
mach_vm_msync(
	vm_map_t                map,
	mach_vm_address_t       address,
	mach_vm_size_t  size,
	vm_sync_t               sync_flags)
{
	if (map == VM_MAP_NULL) {
		return KERN_INVALID_TASK;
	}

	return vm_map_msync(map, (vm_map_address_t)address,
	           (vm_map_size_t)size, sync_flags);
}

  
           
  
                                                                 
                                                                       
                  
  
                               
                                                           
                         
  
                                                                    
                                              
                                     
  
                                             
                                             
                           
  
                                                              
                                             
                                          
                                     
  
                                                               
                                  
  
          
                                        
                                                             
                            
                                                        
   

kern_return_t
vm_msync(
	vm_map_t        map,
	vm_address_t    address,
	vm_size_t       size,
	vm_sync_t       sync_flags)
{
	if (map == VM_MAP_NULL) {
		return KERN_INVALID_TASK;
	}

	return vm_map_msync(map, (vm_map_address_t)address,
	           (vm_map_size_t)size, sync_flags);
}


int
vm_toggle_entry_reuse(int toggle, int *old_value)
{
	vm_map_t map = current_map();

	assert(!map->is_nested_map);
	if (toggle == VM_TOGGLE_GETVALUE && old_value != NULL) {
		*old_value = map->disable_vmentry_reuse;
	} else if (toggle == VM_TOGGLE_SET) {
		vm_map_entry_t map_to_entry;

		vm_map_lock(map);
		vm_map_disable_hole_optimization(map);
		map->disable_vmentry_reuse = TRUE;
		__IGNORE_WCASTALIGN(map_to_entry = vm_map_to_entry(map));
		if (map->first_free == map_to_entry) {
			map->highest_entry_end = vm_map_min(map);
		} else {
			map->highest_entry_end = map->first_free->vme_end;
		}
		vm_map_unlock(map);
	} else if (toggle == VM_TOGGLE_CLEAR) {
		vm_map_lock(map);
		map->disable_vmentry_reuse = FALSE;
		vm_map_unlock(map);
	} else {
		return KERN_INVALID_ARGUMENT;
	}

	return KERN_SUCCESS;
}

  
                       
  
                                                              
                        
  
                                                                  
                                                                
   
kern_return_t
mach_vm_behavior_set(
	vm_map_t                map,
	mach_vm_offset_t        start,
	mach_vm_size_t          size,
	vm_behavior_t           new_behavior)
{
	vm_map_offset_t align_mask;

	if ((map == VM_MAP_NULL) || (start + size < start)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (size == 0) {
		return KERN_SUCCESS;
	}

	switch (new_behavior) {
	case VM_BEHAVIOR_REUSABLE:
	case VM_BEHAVIOR_REUSE:
	case VM_BEHAVIOR_CAN_REUSE:
		  
                                              
                                                     
                                                    
     
		align_mask = PAGE_MASK;
		break;
	default:
		align_mask = VM_MAP_PAGE_MASK(map);
		break;
	}

	return vm_map_behavior_set(map,
	           vm_map_trunc_page(start, align_mask),
	           vm_map_round_page(start + size, align_mask),
	           new_behavior);
}

  
                  
  
                                                              
                        
  
                                                                  
                                                                
  
                                                               
                                                             
             
   
kern_return_t
vm_behavior_set(
	vm_map_t                map,
	vm_offset_t             start,
	vm_size_t               size,
	vm_behavior_t           new_behavior)
{
	if (start + size < start) {
		return KERN_INVALID_ARGUMENT;
	}

	return mach_vm_behavior_set(map,
	           (mach_vm_offset_t) start,
	           (mach_vm_size_t) size,
	           new_behavior);
}

  
                  
  
                                                    
                                                      
             
  
                                                        
                                                          
                                 
  
                                                                     
   

kern_return_t
mach_vm_region(
	vm_map_t                 map,
	mach_vm_offset_t        *address,                           
	mach_vm_size_t          *size,                           
	vm_region_flavor_t       flavor,                        
	vm_region_info_t         info,                           
	mach_msg_type_number_t  *count,                             
	mach_port_t             *object_name)                    
{
	vm_map_offset_t         map_addr;
	vm_map_size_t           map_size;
	kern_return_t           kr;

	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	map_addr = (vm_map_offset_t)*address;
	map_size = (vm_map_size_t)*size;

	                       
	if (VM_REGION_BASIC_INFO == flavor) {
		flavor = VM_REGION_BASIC_INFO_64;
	}

	kr = vm_map_region(map,
	    &map_addr, &map_size,
	    flavor, info, count,
	    object_name);

	*address = map_addr;
	*size = map_size;
	return kr;
}

  
                              
  
                                                    
                                                      
             
  
                                                        
                                                          
                                 
  
                                                                     
   

kern_return_t
vm_region_64(
	vm_map_t                 map,
	vm_offset_t             *address,                           
	vm_size_t               *size,                           
	vm_region_flavor_t       flavor,                        
	vm_region_info_t         info,                           
	mach_msg_type_number_t  *count,                             
	mach_port_t             *object_name)                    
{
	vm_map_offset_t         map_addr;
	vm_map_size_t           map_size;
	kern_return_t           kr;

	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	map_addr = (vm_map_offset_t)*address;
	map_size = (vm_map_size_t)*size;

	                       
	if (VM_REGION_BASIC_INFO == flavor) {
		flavor = VM_REGION_BASIC_INFO_64;
	}

	kr = vm_map_region(map,
	    &map_addr, &map_size,
	    flavor, info, count,
	    object_name);

	*address = CAST_DOWN(vm_offset_t, map_addr);
	*size = CAST_DOWN(vm_size_t, map_size);

	if (KERN_SUCCESS == kr && map_addr + map_size > VM_MAX_ADDRESS) {
		return KERN_INVALID_ADDRESS;
	}
	return kr;
}

kern_return_t
vm_region(
	vm_map_t                        map,
	vm_address_t                    *address,                   
	vm_size_t                       *size,                   
	vm_region_flavor_t              flavor,         
	vm_region_info_t                info,                    
	mach_msg_type_number_t  *count,             
	mach_port_t                     *object_name)            
{
	vm_map_address_t        map_addr;
	vm_map_size_t           map_size;
	kern_return_t           kr;

	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	map_addr = (vm_map_address_t)*address;
	map_size = (vm_map_size_t)*size;

	kr = vm_map_region(map,
	    &map_addr, &map_size,
	    flavor, info, count,
	    object_name);

	*address = CAST_DOWN(vm_address_t, map_addr);
	*size = CAST_DOWN(vm_size_t, map_size);

	if (KERN_SUCCESS == kr && map_addr + map_size > VM_MAX_ADDRESS) {
		return KERN_INVALID_ADDRESS;
	}
	return kr;
}

  
                                                           
                          
  
   
kern_return_t
mach_vm_region_recurse(
	vm_map_t                        map,
	mach_vm_address_t               *address,
	mach_vm_size_t          *size,
	uint32_t                        *depth,
	vm_region_recurse_info_t        info,
	mach_msg_type_number_t  *infoCnt)
{
	vm_map_address_t        map_addr;
	vm_map_size_t           map_size;
	kern_return_t           kr;

	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	map_addr = (vm_map_address_t)*address;
	map_size = (vm_map_size_t)*size;

	kr = vm_map_region_recurse_64(
		map,
		&map_addr,
		&map_size,
		depth,
		(vm_region_submap_info_64_t)info,
		infoCnt);

	*address = map_addr;
	*size = map_size;
	return kr;
}

  
                                                           
                          
  
   
kern_return_t
vm_region_recurse_64(
	vm_map_t                        map,
	vm_address_t                    *address,
	vm_size_t                       *size,
	uint32_t                        *depth,
	vm_region_recurse_info_64_t     info,
	mach_msg_type_number_t  *infoCnt)
{
	vm_map_address_t        map_addr;
	vm_map_size_t           map_size;
	kern_return_t           kr;

	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	map_addr = (vm_map_address_t)*address;
	map_size = (vm_map_size_t)*size;

	kr = vm_map_region_recurse_64(
		map,
		&map_addr,
		&map_size,
		depth,
		(vm_region_submap_info_64_t)info,
		infoCnt);

	*address = CAST_DOWN(vm_address_t, map_addr);
	*size = CAST_DOWN(vm_size_t, map_size);

	if (KERN_SUCCESS == kr && map_addr + map_size > VM_MAX_ADDRESS) {
		return KERN_INVALID_ADDRESS;
	}
	return kr;
}

kern_return_t
vm_region_recurse(
	vm_map_t                        map,
	vm_offset_t             *address,                   
	vm_size_t                       *size,                   
	natural_t                       *depth,             
	vm_region_recurse_info_t        info32,             
	mach_msg_type_number_t  *infoCnt)                   
{
	vm_region_submap_info_data_64_t info64;
	vm_region_submap_info_t info;
	vm_map_address_t        map_addr;
	vm_map_size_t           map_size;
	kern_return_t           kr;

	if (VM_MAP_NULL == map || *infoCnt < VM_REGION_SUBMAP_INFO_COUNT) {
		return KERN_INVALID_ARGUMENT;
	}


	map_addr = (vm_map_address_t)*address;
	map_size = (vm_map_size_t)*size;
	info = (vm_region_submap_info_t)info32;
	*infoCnt = VM_REGION_SUBMAP_INFO_COUNT_64;

	kr = vm_map_region_recurse_64(map, &map_addr, &map_size,
	    depth, &info64, infoCnt);

	info->protection = info64.protection;
	info->max_protection = info64.max_protection;
	info->inheritance = info64.inheritance;
	info->offset = (uint32_t)info64.offset;                    
	info->user_tag = info64.user_tag;
	info->pages_resident = info64.pages_resident;
	info->pages_shared_now_private = info64.pages_shared_now_private;
	info->pages_swapped_out = info64.pages_swapped_out;
	info->pages_dirtied = info64.pages_dirtied;
	info->ref_count = info64.ref_count;
	info->shadow_depth = info64.shadow_depth;
	info->external_pager = info64.external_pager;
	info->share_mode = info64.share_mode;
	info->is_submap = info64.is_submap;
	info->behavior = info64.behavior;
	info->object_id = info64.object_id;
	info->user_wired_count = info64.user_wired_count;

	*address = CAST_DOWN(vm_address_t, map_addr);
	*size = CAST_DOWN(vm_size_t, map_size);
	*infoCnt = VM_REGION_SUBMAP_INFO_COUNT;

	if (KERN_SUCCESS == kr && map_addr + map_size > VM_MAX_ADDRESS) {
		return KERN_INVALID_ADDRESS;
	}
	return kr;
}

kern_return_t
mach_vm_purgable_control(
	vm_map_t                map,
	mach_vm_offset_t        address,
	vm_purgable_t           control,
	int                     *state)
{
	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	if (control == VM_PURGABLE_SET_STATE_FROM_KERNEL) {
		                                 
		return KERN_INVALID_ARGUMENT;
	}

	return vm_map_purgable_control(map,
	           vm_map_trunc_page(address, VM_MAP_PAGE_MASK(map)),
	           control,
	           state);
}

kern_return_t
vm_purgable_control(
	vm_map_t                map,
	vm_offset_t             address,
	vm_purgable_t           control,
	int                     *state)
{
	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	if (control == VM_PURGABLE_SET_STATE_FROM_KERNEL) {
		                                 
		return KERN_INVALID_ARGUMENT;
	}

	return vm_map_purgable_control(map,
	           vm_map_trunc_page(address, VM_MAP_PAGE_MASK(map)),
	           control,
	           state);
}


  
                                                      
                                                         
                                                        
                                                
   
unsigned int    vm_allocate_cpm_privileged = 0;

  
                                                             
                                                           
                                                         
                                                           
                   
  
                                                        
                                       
   
kern_return_t
vm_allocate_cpm(
	host_priv_t             host_priv,
	vm_map_t                map,
	vm_address_t            *addr,
	vm_size_t               size,
	int                     flags)
{
	vm_map_address_t        map_addr;
	vm_map_size_t           map_size;
	kern_return_t           kr;

	if (vm_allocate_cpm_privileged && HOST_PRIV_NULL == host_priv) {
		return KERN_INVALID_HOST;
	}

	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	map_addr = (vm_map_address_t)*addr;
	map_size = (vm_map_size_t)size;

	kr = vm_map_enter_cpm(map,
	    &map_addr,
	    map_size,
	    flags);

	*addr = CAST_DOWN(vm_address_t, map_addr);
	return kr;
}


kern_return_t
mach_vm_page_query(
	vm_map_t                map,
	mach_vm_offset_t        offset,
	int                     *disposition,
	int                     *ref_count)
{
	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	return vm_map_page_query_internal(
		map,
		vm_map_trunc_page(offset, PAGE_MASK),
		disposition, ref_count);
}

kern_return_t
vm_map_page_query(
	vm_map_t                map,
	vm_offset_t             offset,
	int                     *disposition,
	int                     *ref_count)
{
	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	return vm_map_page_query_internal(
		map,
		vm_map_trunc_page(offset, PAGE_MASK),
		disposition, ref_count);
}

kern_return_t
mach_vm_page_range_query(
	vm_map_t                map,
	mach_vm_offset_t        address,
	mach_vm_size_t          size,
	mach_vm_address_t       dispositions_addr,
	mach_vm_size_t          *dispositions_count)
{
	kern_return_t           kr = KERN_SUCCESS;
	int                     num_pages = 0, i = 0;
	mach_vm_size_t          curr_sz = 0, copy_sz = 0;
	mach_vm_size_t          disp_buf_req_size = 0, disp_buf_total_size = 0;
	mach_msg_type_number_t  count = 0;

	void                    *info = NULL;
	void                    *local_disp = NULL;;
	vm_map_size_t           info_size = 0, local_disp_size = 0;
	mach_vm_offset_t        start = 0, end = 0;
	int                     effective_page_shift, effective_page_size, effective_page_mask;

	if (map == VM_MAP_NULL || dispositions_count == NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	effective_page_shift = vm_self_region_page_shift_safely(map);
	if (effective_page_shift == -1) {
		return KERN_INVALID_ARGUMENT;
	}
	effective_page_size = (1 << effective_page_shift);
	effective_page_mask = effective_page_size - 1;

	if (os_mul_overflow(*dispositions_count, sizeof(int), &disp_buf_req_size)) {
		return KERN_INVALID_ARGUMENT;
	}

	start = vm_map_trunc_page(address, effective_page_mask);
	end = vm_map_round_page(address + size, effective_page_mask);

	if (end < start) {
		return KERN_INVALID_ARGUMENT;
	}

	if ((end - start) < size) {
		  
                                              
     
		return KERN_INVALID_ARGUMENT;
	}

	if (disp_buf_req_size == 0 || (end == start)) {
		return KERN_SUCCESS;
	}

	  
                                               
                                         
    

	curr_sz = MIN(end - start, MAX_PAGE_RANGE_QUERY);
	num_pages = (int) (curr_sz >> effective_page_shift);

	info_size = num_pages * sizeof(vm_page_info_basic_data_t);
	info = kheap_alloc(KHEAP_TEMP, info_size, Z_WAITOK);

	local_disp_size = num_pages * sizeof(int);
	local_disp = kheap_alloc(KHEAP_TEMP, local_disp_size, Z_WAITOK);

	if (info == NULL || local_disp == NULL) {
		kr = KERN_RESOURCE_SHORTAGE;
		goto out;
	}

	while (size) {
		count = VM_PAGE_INFO_BASIC_COUNT;
		kr = vm_map_page_range_info_internal(
			map,
			start,
			vm_map_round_page(start + curr_sz, effective_page_mask),
			effective_page_shift,
			VM_PAGE_INFO_BASIC,
			(vm_page_info_t) info,
			&count);

		assert(kr == KERN_SUCCESS);

		for (i = 0; i < num_pages; i++) {
			((int*)local_disp)[i] = ((vm_page_info_basic_t)info)[i].disposition;
		}

		copy_sz = MIN(disp_buf_req_size, num_pages * sizeof(int)                      );
		kr = copyout(local_disp, (mach_vm_address_t)dispositions_addr, copy_sz);

		start += curr_sz;
		disp_buf_req_size -= copy_sz;
		disp_buf_total_size += copy_sz;

		if (kr != 0) {
			break;
		}

		if ((disp_buf_req_size == 0) || (curr_sz >= size)) {
			  
                                               
                                             
                                              
                                                
      

			size = 0;
		} else {
			dispositions_addr += copy_sz;

			size -= curr_sz;

			curr_sz = MIN(vm_map_round_page(size, effective_page_mask), MAX_PAGE_RANGE_QUERY);
			num_pages = (int)(curr_sz >> effective_page_shift);
		}
	}

	*dispositions_count = disp_buf_total_size / sizeof(int);

out:
	if (local_disp) {
		kheap_free(KHEAP_TEMP, local_disp, local_disp_size);
	}
	if (info) {
		kheap_free(KHEAP_TEMP, info, info_size);
	}
	return kr;
}

kern_return_t
mach_vm_page_info(
	vm_map_t                map,
	mach_vm_address_t       address,
	vm_page_info_flavor_t   flavor,
	vm_page_info_t          info,
	mach_msg_type_number_t  *count)
{
	kern_return_t   kr;

	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	kr = vm_map_page_info(map, address, flavor, info, count);
	return kr;
}

                                             
kern_return_t
vm_upl_map(
	vm_map_t                map,
	upl_t                   upl,
	vm_address_t            *dst_addr)
{
	vm_map_offset_t         map_addr;
	kern_return_t           kr;

	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	kr = vm_map_enter_upl(map, upl, &map_addr);
	*dst_addr = CAST_DOWN(vm_address_t, map_addr);
	return kr;
}

kern_return_t
vm_upl_unmap(
	vm_map_t                map,
	upl_t                   upl)
{
	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	return vm_map_remove_upl(map, upl);
}

                                                                       

kern_return_t
vm_map_get_upl(
	vm_map_t                map,
	vm_map_offset_t         map_offset,
	upl_size_t              *upl_size,
	upl_t                   *upl,
	upl_page_info_array_t   page_list,
	unsigned int            *count,
	upl_control_flags_t     *flags,
	vm_tag_t                tag,
	int                     force_data_sync)
{
	upl_control_flags_t map_flags;
	kern_return_t       kr;

	if (VM_MAP_NULL == map) {
		return KERN_INVALID_ARGUMENT;
	}

	map_flags = *flags & ~UPL_NOZEROFILL;
	if (force_data_sync) {
		map_flags |= UPL_FORCE_DATA_SYNC;
	}

	kr = vm_map_create_upl(map,
	    map_offset,
	    upl_size,
	    upl,
	    page_list,
	    count,
	    &map_flags,
	    tag);

	*flags = (map_flags & ~UPL_FORCE_DATA_SYNC);
	return kr;
}

  
                            
  
                                                          
                                                        
                                                        
                                                  
   
kern_return_t
mach_make_memory_entry_64(
	vm_map_t                target_map,
	memory_object_size_t    *size,
	memory_object_offset_t  offset,
	vm_prot_t               permission,
	ipc_port_t              *object_handle,
	ipc_port_t              parent_handle)
{
	vm_named_entry_kernel_flags_t   vmne_kflags;

	if ((permission & MAP_MEM_FLAGS_MASK) & ~MAP_MEM_FLAGS_USER) {
		  
                                                    
     
		return KERN_INVALID_VALUE;
	}

	vmne_kflags = VM_NAMED_ENTRY_KERNEL_FLAGS_NONE;
	if (permission & MAP_MEM_LEDGER_TAGGED) {
		vmne_kflags.vmnekf_ledger_tag = VM_LEDGER_TAG_DEFAULT;
	}
	return mach_make_memory_entry_internal(target_map,
	           size,
	           offset,
	           permission,
	           vmne_kflags,
	           object_handle,
	           parent_handle);
}

kern_return_t
mach_make_memory_entry_internal(
	vm_map_t                target_map,
	memory_object_size_t    *size,
	memory_object_offset_t  offset,
	vm_prot_t               permission,
	vm_named_entry_kernel_flags_t   vmne_kflags,
	ipc_port_t              *object_handle,
	ipc_port_t              parent_handle)
{
	vm_named_entry_t        parent_entry;
	vm_named_entry_t        user_entry;
	ipc_port_t              user_handle;
	kern_return_t           kr;
	vm_object_t             object;
	vm_map_size_t           map_size;
	vm_map_offset_t         map_start, map_end;

	  
                                                                     
                                                                
    
	vm_object_offset_t      offset_in_page;

	unsigned int            access;
	vm_prot_t               protections;
	vm_prot_t               original_protections, mask_protections;
	unsigned int            wimg_mode;
	boolean_t               use_data_addr;
	boolean_t               use_4K_compat;

	DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x\n", target_map, offset, *size, permission);

	user_entry = NULL;

	if ((permission & MAP_MEM_FLAGS_MASK) & ~MAP_MEM_FLAGS_ALL) {
		  
                                                    
     
		DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_INVALID_VALUE);
		return KERN_INVALID_VALUE;
	}

	if (IP_VALID(parent_handle) &&
	    ip_kotype(parent_handle) == IKOT_NAMED_ENTRY) {
		parent_entry = (vm_named_entry_t) ip_get_kobject(parent_handle);
	} else {
		parent_entry = NULL;
	}

	if (parent_entry && parent_entry->is_copy) {
		DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_INVALID_ARGUMENT);
		return KERN_INVALID_ARGUMENT;
	}

	original_protections = permission & VM_PROT_ALL;
	protections = original_protections;
	mask_protections = permission & VM_PROT_IS_MASK;
	access = GET_MAP_MEM(permission);
	use_data_addr = ((permission & MAP_MEM_USE_DATA_ADDR) != 0);
	use_4K_compat = ((permission & MAP_MEM_4K_DATA_ADDR) != 0);

	user_handle = IP_NULL;
	user_entry = NULL;

	map_start = vm_map_trunc_page(offset, VM_MAP_PAGE_MASK(target_map));

	if (permission & MAP_MEM_ONLY) {
		boolean_t               parent_is_object;

		map_end = vm_map_round_page(offset + *size, VM_MAP_PAGE_MASK(target_map));
		map_size = map_end - map_start;

		if (use_data_addr || use_4K_compat || parent_entry == NULL) {
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_INVALID_ARGUMENT);
			return KERN_INVALID_ARGUMENT;
		}

		parent_is_object = parent_entry->is_object;
		if (!parent_is_object) {
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_INVALID_ARGUMENT);
			return KERN_INVALID_ARGUMENT;
		}
		object = vm_named_entry_to_vm_object(parent_entry);
		if (parent_is_object && object != VM_OBJECT_NULL) {
			wimg_mode = object->wimg_bits;
		} else {
			wimg_mode = VM_WIMG_USE_DEFAULT;
		}
		if ((access != GET_MAP_MEM(parent_entry->protection)) &&
		    !(parent_entry->protection & VM_PROT_WRITE)) {
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_INVALID_RIGHT);
			return KERN_INVALID_RIGHT;
		}
		vm_prot_to_wimg(access, &wimg_mode);
		if (access != MAP_MEM_NOOP) {
			SET_MAP_MEM(access, parent_entry->protection);
		}
		if (parent_is_object && object &&
		    (access != MAP_MEM_NOOP) &&
		    (!(object->nophyscache))) {
			if (object->wimg_bits != wimg_mode) {
				vm_object_lock(object);
				vm_object_change_wimg_mode(object, wimg_mode);
				vm_object_unlock(object);
			}
		}
		if (object_handle) {
			*object_handle = IP_NULL;
		}
		DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_SUCCESS);
		return KERN_SUCCESS;
	} else if (permission & MAP_MEM_NAMED_CREATE) {
		int     ledger_flags = 0;
		task_t  owner;

		map_end = vm_map_round_page(offset + *size, VM_MAP_PAGE_MASK(target_map));
		map_size = map_end - map_start;

		if (use_data_addr || use_4K_compat) {
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_INVALID_ARGUMENT);
			return KERN_INVALID_ARGUMENT;
		}

		if (map_size == 0) {
			*size = 0;
			*object_handle = IPC_PORT_NULL;
			return KERN_SUCCESS;
		}

		kr = mach_memory_entry_allocate(&user_entry, &user_handle);
		if (kr != KERN_SUCCESS) {
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_FAILURE);
			return KERN_FAILURE;
		}

		  
                                             
     
		if (map_size > (vm_map_size_t) ANON_MAX_SIZE) {
			  
                                                       
                                                      
                                                 
      
			kr = KERN_FAILURE;
			goto make_mem_done;
		}

		object = vm_object_allocate(map_size);
		assert(object != VM_OBJECT_NULL);

		  
        
                                                    
                                                 
                               
                                                             
                                                            
                             
     
		object->copy_strategy = MEMORY_OBJECT_COPY_NONE;
		object->true_share = TRUE;

		owner = current_task();
		if ((permission & MAP_MEM_PURGABLE) ||
		    vmne_kflags.vmnekf_ledger_tag) {
			assert(object->vo_owner == NULL);
			assert(object->resident_page_count == 0);
			assert(object->wired_page_count == 0);
			assert(owner != TASK_NULL);
			if (vmne_kflags.vmnekf_ledger_no_footprint) {
				ledger_flags |= VM_LEDGER_FLAG_NO_FOOTPRINT;
				object->vo_no_footprint = TRUE;
			}
			if (permission & MAP_MEM_PURGABLE) {
				if (!(permission & VM_PROT_WRITE)) {
					                                       
					vm_object_deallocate(object);
					kr = KERN_INVALID_ARGUMENT;
					goto make_mem_done;
				}
				object->purgable = VM_PURGABLE_NONVOLATILE;
				if (permission & MAP_MEM_PURGABLE_KERNEL_ONLY) {
					object->purgeable_only_by_kernel = TRUE;
				}
#if __arm64__
				if (owner->task_legacy_footprint) {
					  
                                           
                                         
                                         
                                             
        
					owner = kernel_task;
				}
#endif                
				vm_object_lock(object);
				vm_purgeable_nonvolatile_enqueue(object, owner);
				vm_object_unlock(object);
			}
		}

		if (vmne_kflags.vmnekf_ledger_tag) {
			  
                                            
                                
      
			if (vmne_kflags.vmnekf_ledger_no_footprint) {
				ledger_flags |= VM_LEDGER_FLAG_NO_FOOTPRINT;
			}
			vm_object_lock(object);
			object->vo_ledger_tag = vmne_kflags.vmnekf_ledger_tag;
			kr = vm_object_ownership_change(
				object,
				vmne_kflags.vmnekf_ledger_tag,
				owner,                
				ledger_flags,
				FALSE);                        
			vm_object_unlock(object);
			if (kr != KERN_SUCCESS) {
				vm_object_deallocate(object);
				goto make_mem_done;
			}
		}

#if CONFIG_SECLUDED_MEMORY
		if (secluded_for_iokit &&                      
		    ((permission & MAP_MEM_GRAB_SECLUDED)
#if 11
		                                      
		    || (secluded_for_fbdp && map_size == 97550336)
#endif
		    )) {
#if 11
			if (!(permission & MAP_MEM_GRAB_SECLUDED) &&
			    secluded_for_fbdp) {
				printf("FBDP: object %p size %lld can grab secluded\n", object, (uint64_t) map_size);
			}
#endif
			object->can_grab_secluded = TRUE;
			assert(!object->eligible_for_secluded);
		}
#endif                             

		  
                                                               
                                 
     

		wimg_mode = object->wimg_bits;
		vm_prot_to_wimg(access, &wimg_mode);
		if (access != MAP_MEM_NOOP) {
			object->wimg_bits = wimg_mode;
		}

		                                                             

		kr = vm_named_entry_from_vm_object(
			user_entry,
			object,
			0,
			map_size,
			(protections & VM_PROT_ALL));
		if (kr != KERN_SUCCESS) {
			vm_object_deallocate(object);
			goto make_mem_done;
		}
		user_entry->internal = TRUE;
		user_entry->is_sub_map = FALSE;
		user_entry->offset = 0;
		user_entry->data_offset = 0;
		user_entry->protection = protections;
		SET_MAP_MEM(access, user_entry->protection);
		user_entry->size = map_size;

		                                                        
		                                                

		*size = CAST_DOWN(vm_size_t, (user_entry->size -
		    user_entry->data_offset));
		*object_handle = user_handle;
		DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_SUCCESS);
		return KERN_SUCCESS;
	}

	if (permission & MAP_MEM_VM_COPY) {
		vm_map_copy_t   copy;

		if (target_map == VM_MAP_NULL) {
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_INVALID_TASK);
			return KERN_INVALID_TASK;
		}

		map_end = vm_map_round_page(offset + *size, VM_MAP_PAGE_MASK(target_map));
		map_size = map_end - map_start;
		if (map_size == 0) {
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_INVALID_ARGUMENT);
			return KERN_INVALID_ARGUMENT;
		}

		if (use_data_addr || use_4K_compat) {
			offset_in_page = offset - map_start;
			if (use_4K_compat) {
				offset_in_page &= ~((signed)(0xFFF));
			}
		} else {
			offset_in_page = 0;
		}

		kr = vm_map_copyin_internal(target_map,
		    map_start,
		    map_size,
		    VM_MAP_COPYIN_ENTRY_LIST,
		    &copy);
		if (kr != KERN_SUCCESS) {
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, kr);
			return kr;
		}
		assert(copy != VM_MAP_COPY_NULL);

		kr = mach_memory_entry_allocate(&user_entry, &user_handle);
		if (kr != KERN_SUCCESS) {
			vm_map_copy_discard(copy);
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_FAILURE);
			return KERN_FAILURE;
		}

		user_entry->backing.copy = copy;
		user_entry->internal = FALSE;
		user_entry->is_sub_map = FALSE;
		user_entry->is_copy = TRUE;
		user_entry->offset = 0;
		user_entry->protection = protections;
		user_entry->size = map_size;
		user_entry->data_offset = offset_in_page;

		*size = CAST_DOWN(vm_size_t, (user_entry->size -
		    user_entry->data_offset));
		*object_handle = user_handle;
		DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_SUCCESS);
		return KERN_SUCCESS;
	}

	if ((permission & MAP_MEM_VM_SHARE)
	    || parent_entry == NULL
	    || (permission & MAP_MEM_NAMED_REUSE)) {
		vm_map_copy_t   copy;
		vm_prot_t       cur_prot, max_prot;
		vm_map_kernel_flags_t vmk_flags;
		vm_map_entry_t parent_copy_entry;

		if (target_map == VM_MAP_NULL) {
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_INVALID_TASK);
			return KERN_INVALID_TASK;
		}

		map_end = vm_map_round_page(offset + *size, VM_MAP_PAGE_MASK(target_map));
		vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
		parent_copy_entry = VM_MAP_ENTRY_NULL;
		if (!(permission & MAP_MEM_VM_SHARE)) {
			vm_map_t tmp_map, real_map;
			vm_map_version_t version;
			vm_object_t tmp_object;
			vm_object_offset_t obj_off;
			vm_prot_t prot;
			boolean_t wired;
			bool contended;

			                                                 
			if (protections & VM_PROT_WRITE) {
				tmp_map = target_map;
				vm_map_lock_read(tmp_map);
				kr = vm_map_lookup_locked(&tmp_map,
				    map_start,
				    protections | mask_protections,
				    OBJECT_LOCK_EXCLUSIVE,
				    &version,
				    &tmp_object,
				    &obj_off,
				    &prot,
				    &wired,
				    NULL,                                       
				    &real_map,
				    &contended);
				if (kr != KERN_SUCCESS) {
					vm_map_unlock_read(tmp_map);
				} else {
					vm_object_unlock(tmp_object);
					vm_map_unlock_read(tmp_map);
					if (real_map != tmp_map) {
						vm_map_unlock_read(real_map);
					}
				}
			}
			                      

			                                          
			vmk_flags.vmkf_copy_single_object = TRUE;
			if ((permission & MAP_MEM_NAMED_REUSE) &&
			    parent_entry != NULL &&
			    parent_entry->is_object) {
				vm_map_copy_t parent_copy;
				parent_copy = parent_entry->backing.copy;
				assert(parent_copy->cpy_hdr.nentries == 1);
				parent_copy_entry = vm_map_copy_first_entry(parent_copy);
				assert(!parent_copy_entry->is_sub_map);
			}
		}

		map_size = map_end - map_start;
		if (map_size == 0) {
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_INVALID_ARGUMENT);
			return KERN_INVALID_ARGUMENT;
		}

		if (use_data_addr || use_4K_compat) {
			offset_in_page = offset - map_start;
			if (use_4K_compat) {
				offset_in_page &= ~((signed)(0xFFF));
			}
		} else {
			offset_in_page = 0;
		}

		if (mask_protections) {
			  
                                                     
                                         
      
			cur_prot = VM_PROT_NONE;
			max_prot = VM_PROT_NONE;
		} else {
			  
                                                     
                                                         
      
			cur_prot = protections;
			max_prot = protections;
		}
		if (target_map->pmap == kernel_pmap) {
			  
                                                     
                                                
                                                       
           
      
			vmk_flags.vmkf_copy_pageable = FALSE;
		} else {
			vmk_flags.vmkf_copy_pageable = TRUE;
		}
		vmk_flags.vmkf_copy_same_map = FALSE;
		assert(map_size != 0);
		kr = vm_map_copy_extract(target_map,
		    map_start,
		    map_size,
		    FALSE,                                
		    &copy,
		    &cur_prot,
		    &max_prot,
		    VM_INHERIT_SHARE,
		    vmk_flags);
		if (kr != KERN_SUCCESS) {
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, kr);
			if (VM_MAP_PAGE_SHIFT(target_map) < PAGE_SHIFT) {
                                                                   
			}
			return kr;
		}
		assert(copy != VM_MAP_COPY_NULL);

		if (mask_protections) {
			  
                                                    
                                                 
      
			protections &= cur_prot;
			if (protections == VM_PROT_NONE) {
				                            
				DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_PROTECTION_FAILURE);
				if (VM_MAP_PAGE_SHIFT(target_map) < PAGE_SHIFT) {
                                                                    
				}
				vm_map_copy_discard(copy);
				return KERN_PROTECTION_FAILURE;
			}
		} else {
			  
                                            
                        
      
			assert((cur_prot & protections) == protections);
			assert((max_prot & protections) == protections);
			                                      
			if ((cur_prot & protections) != protections) {
				if (VM_MAP_PAGE_SHIFT(target_map) < PAGE_SHIFT) {
                                                                                         
				}
				vm_map_copy_discard(copy);
				DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_PROTECTION_FAILURE);
				return KERN_PROTECTION_FAILURE;
			}
		}

		if (!(permission & MAP_MEM_VM_SHARE)) {
			vm_map_entry_t copy_entry;

			                                                     
			assert(copy->cpy_hdr.nentries == 1);
			copy_entry = vm_map_copy_first_entry(copy);
			map_size = copy_entry->vme_end - copy_entry->vme_start;

			if ((permission & MAP_MEM_NAMED_REUSE) &&
			    parent_copy_entry != VM_MAP_ENTRY_NULL &&
			    VME_OBJECT(copy_entry) == VME_OBJECT(parent_copy_entry) &&
			    VME_OFFSET(copy_entry) == VME_OFFSET(parent_copy_entry) &&
			    parent_entry->offset == 0 &&
			    parent_entry->size == map_size &&
			    (parent_entry->data_offset == offset_in_page)) {
				                                            

				                            
				vm_map_copy_discard(copy);
				                                    
				ipc_port_copy_send(parent_handle);

				*size = CAST_DOWN(vm_size_t,
				    (parent_entry->size -
				    parent_entry->data_offset));
				*object_handle = parent_handle;
				DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_SUCCESS);
				return KERN_SUCCESS;
			}

			                                             
			object = VME_OBJECT(copy_entry);
			vm_object_lock(object);
			wimg_mode = object->wimg_bits;
			if (!(object->nophyscache)) {
				vm_prot_to_wimg(access, &wimg_mode);
			}
			if (object->wimg_bits != wimg_mode) {
				vm_object_change_wimg_mode(object, wimg_mode);
			}
			vm_object_unlock(object);
		}

		kr = mach_memory_entry_allocate(&user_entry, &user_handle);
		if (kr != KERN_SUCCESS) {
			if (VM_MAP_PAGE_SHIFT(target_map) < PAGE_SHIFT) {
                                                                   
			}
			vm_map_copy_discard(copy);
			DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_FAILURE);
			return KERN_FAILURE;
		}

		user_entry->backing.copy = copy;
		user_entry->is_sub_map = FALSE;
		user_entry->is_object = FALSE;
		user_entry->internal = FALSE;
		user_entry->protection = protections;
		user_entry->size = map_size;
		user_entry->data_offset = offset_in_page;

		if (permission & MAP_MEM_VM_SHARE) {
			user_entry->is_copy = TRUE;
			user_entry->offset = 0;
		} else {
			user_entry->is_object = TRUE;
			user_entry->internal = object->internal;
			user_entry->offset = VME_OFFSET(vm_map_copy_first_entry(copy));
			SET_MAP_MEM(GET_MAP_MEM(permission), user_entry->protection);
		}

		*size = CAST_DOWN(vm_size_t, (user_entry->size -
		    user_entry->data_offset));
		*object_handle = user_handle;
		DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_SUCCESS);
		return KERN_SUCCESS;
	}

	                                                              
	if (parent_entry == NULL) {
		kr = KERN_INVALID_ARGUMENT;
		goto make_mem_done;
	}

	if (parent_entry->is_copy) {
		panic("parent_entry %p is_copy not supported\n", parent_entry);
		kr = KERN_INVALID_ARGUMENT;
		goto make_mem_done;
	}

	if (use_data_addr || use_4K_compat) {
		  
                                                             
                                                                             
     
		if (parent_entry->is_sub_map) {
			panic("Shouldn't be using data address with a parent entry that is a submap.");
		}
		  
                                                   
                                    
     
		if ((offset + *size + parent_entry->data_offset) > parent_entry->size) {
			kr = KERN_INVALID_ARGUMENT;
			goto make_mem_done;
		}

		map_start = vm_map_trunc_page(offset + parent_entry->data_offset, PAGE_MASK);
		offset_in_page = (offset + parent_entry->data_offset) - map_start;
		if (use_4K_compat) {
			offset_in_page &= ~((signed)(0xFFF));
		}
		map_end = vm_map_round_page(offset + parent_entry->data_offset + *size, PAGE_MASK);
		map_size = map_end - map_start;
	} else {
		map_end = vm_map_round_page(offset + *size, PAGE_MASK);
		map_size = map_end - map_start;
		offset_in_page = 0;

		if ((offset + map_size) > parent_entry->size) {
			kr = KERN_INVALID_ARGUMENT;
			goto make_mem_done;
		}
	}

	if (mask_protections) {
		  
                                                    
                                                   
                             
     
		protections &= parent_entry->protection;
	}
	if ((protections & parent_entry->protection) != protections) {
		kr = KERN_PROTECTION_FAILURE;
		goto make_mem_done;
	}

	if (mach_memory_entry_allocate(&user_entry, &user_handle)
	    != KERN_SUCCESS) {
		kr = KERN_FAILURE;
		goto make_mem_done;
	}

	user_entry->size = map_size;
	user_entry->offset = parent_entry->offset + map_start;
	user_entry->data_offset = offset_in_page;
	user_entry->is_sub_map = parent_entry->is_sub_map;
	user_entry->is_copy = parent_entry->is_copy;
	user_entry->internal = parent_entry->internal;
	user_entry->protection = protections;

	if (access != MAP_MEM_NOOP) {
		SET_MAP_MEM(access, user_entry->protection);
	}

	if (parent_entry->is_sub_map) {
		vm_map_t map = parent_entry->backing.map;
		vm_map_reference(map);
		user_entry->backing.map = map;
	} else {
		object = vm_named_entry_to_vm_object(parent_entry);
		assert(object != VM_OBJECT_NULL);
		assert(object->copy_strategy != MEMORY_OBJECT_COPY_SYMMETRIC);
		kr = vm_named_entry_from_vm_object(
			user_entry,
			object,
			user_entry->offset,
			user_entry->size,
			(user_entry->protection & VM_PROT_ALL));
		if (kr != KERN_SUCCESS) {
			goto make_mem_done;
		}
		assert(user_entry->is_object);
		                                          
		vm_object_lock(object);
		vm_object_reference_locked(object);
#if VM_OBJECT_TRACKING_OP_TRUESHARE
		if (!object->true_share &&
		    vm_object_tracking_inited) {
			void *bt[VM_OBJECT_TRACKING_BTDEPTH];
			int num = 0;

			num = OSBacktrace(bt,
			    VM_OBJECT_TRACKING_BTDEPTH);
			btlog_add_entry(vm_object_tracking_btlog,
			    object,
			    VM_OBJECT_TRACKING_OP_TRUESHARE,
			    bt,
			    num);
		}
#endif                                      

		object->true_share = TRUE;
		if (object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {
			object->copy_strategy = MEMORY_OBJECT_COPY_DELAY;
		}
		vm_object_unlock(object);
	}
	*size = CAST_DOWN(vm_size_t, (user_entry->size -
	    user_entry->data_offset));
	*object_handle = user_handle;
	DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, KERN_SUCCESS);
	return KERN_SUCCESS;

make_mem_done:
	if (user_handle != IP_NULL) {
		  
                                                     
                                                      
                        
     
		mach_memory_entry_port_release(user_handle);
	}
	DEBUG4K_MEMENTRY("map %p offset 0x%llx size 0x%llx prot 0x%x -> entry %p kr 0x%x\n", target_map, offset, *size, permission, user_entry, kr);
	return kr;
}

kern_return_t
_mach_make_memory_entry(
	vm_map_t                target_map,
	memory_object_size_t    *size,
	memory_object_offset_t  offset,
	vm_prot_t               permission,
	ipc_port_t              *object_handle,
	ipc_port_t              parent_entry)
{
	memory_object_size_t    mo_size;
	kern_return_t           kr;

	mo_size = (memory_object_size_t)*size;
	kr = mach_make_memory_entry_64(target_map, &mo_size,
	    (memory_object_offset_t)offset, permission, object_handle,
	    parent_entry);
	*size = mo_size;
	return kr;
}

kern_return_t
mach_make_memory_entry(
	vm_map_t                target_map,
	vm_size_t               *size,
	vm_offset_t             offset,
	vm_prot_t               permission,
	ipc_port_t              *object_handle,
	ipc_port_t              parent_entry)
{
	memory_object_size_t    mo_size;
	kern_return_t           kr;

	mo_size = (memory_object_size_t)*size;
	kr = mach_make_memory_entry_64(target_map, &mo_size,
	    (memory_object_offset_t)offset, permission, object_handle,
	    parent_entry);
	*size = CAST_DOWN(vm_size_t, mo_size);
	return kr;
}

  
            
  
                                                                   
                                                              
                                                               
                                                   
   
kern_return_t
task_wire(
	vm_map_t        map,
	boolean_t       must_wire)
{
	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	vm_map_lock(map);
	map->wiring_required = (must_wire == TRUE);
	vm_map_unlock(map);

	return KERN_SUCCESS;
}

kern_return_t
vm_map_exec_lockdown(
	vm_map_t        map)
{
	if (map == VM_MAP_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	vm_map_lock(map);
	map->map_disallow_new_exec = TRUE;
	vm_map_unlock(map);

	return KERN_SUCCESS;
}

#if VM_NAMED_ENTRY_LIST
queue_head_t    vm_named_entry_list = QUEUE_HEAD_INITIALIZER(vm_named_entry_list);
int             vm_named_entry_count = 0;
LCK_MTX_EARLY_DECLARE_ATTR(vm_named_entry_list_lock_data,
    &vm_object_lck_grp, &vm_object_lck_attr);
#endif                          

__private_extern__ kern_return_t
mach_memory_entry_allocate(
	vm_named_entry_t        *user_entry_p,
	ipc_port_t              *user_handle_p)
{
	vm_named_entry_t        user_entry;
	ipc_port_t              user_handle;

	user_entry = (vm_named_entry_t) kalloc(sizeof *user_entry);
	if (user_entry == NULL) {
		return KERN_FAILURE;
	}
	bzero(user_entry, sizeof(*user_entry));

	named_entry_lock_init(user_entry);

	user_entry->backing.copy = NULL;
	user_entry->is_object = FALSE;
	user_entry->is_sub_map = FALSE;
	user_entry->is_copy = FALSE;
	user_entry->internal = FALSE;
	user_entry->size = 0;
	user_entry->offset = 0;
	user_entry->data_offset = 0;
	user_entry->protection = VM_PROT_NONE;
	user_entry->ref_count = 1;

	user_handle = ipc_kobject_alloc_port((ipc_kobject_t)user_entry,
	    IKOT_NAMED_ENTRY,
	    IPC_KOBJECT_ALLOC_MAKE_SEND | IPC_KOBJECT_ALLOC_NSREQUEST);

	*user_entry_p = user_entry;
	*user_handle_p = user_handle;

#if VM_NAMED_ENTRY_LIST
	                                                                              
	user_entry->named_entry_port = user_handle;
	                                                      
	OSBacktrace(&user_entry->named_entry_bt[0],
	    NAMED_ENTRY_BT_DEPTH);

	                                                 
	lck_mtx_lock_spin(&vm_named_entry_list_lock_data);
	queue_enter(&vm_named_entry_list, user_entry,
	    vm_named_entry_t, named_entry_list);
	vm_named_entry_count++;
	lck_mtx_unlock(&vm_named_entry_list_lock_data);
#endif                          

	return KERN_SUCCESS;
}

  
                                     
  
                                                     
  
   
kern_return_t
mach_memory_object_memory_entry_64(
	host_t                  host,
	boolean_t               internal,
	vm_object_offset_t      size,
	vm_prot_t               permission,
	memory_object_t         pager,
	ipc_port_t              *entry_handle)
{
	unsigned int            access;
	vm_named_entry_t        user_entry;
	ipc_port_t              user_handle;
	vm_object_t             object;
	kern_return_t           kr;

	if (host == HOST_NULL) {
		return KERN_INVALID_HOST;
	}

	if (pager == MEMORY_OBJECT_NULL && internal) {
		object = vm_object_allocate(size);
		if (object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {
			object->copy_strategy = MEMORY_OBJECT_COPY_DELAY;
		}
	} else {
		object = memory_object_to_vm_object(pager);
		if (object != VM_OBJECT_NULL) {
			vm_object_reference(object);
		}
	}
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (mach_memory_entry_allocate(&user_entry, &user_handle)
	    != KERN_SUCCESS) {
		vm_object_deallocate(object);
		return KERN_FAILURE;
	}

	user_entry->size = size;
	user_entry->offset = 0;
	user_entry->protection = permission & VM_PROT_ALL;
	access = GET_MAP_MEM(permission);
	SET_MAP_MEM(access, user_entry->protection);
	user_entry->is_sub_map = FALSE;
	assert(user_entry->ref_count == 1);

	kr = vm_named_entry_from_vm_object(user_entry, object, 0, size,
	    (user_entry->protection & VM_PROT_ALL));
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	user_entry->internal = object->internal;
	assert(object->internal == internal);

	*entry_handle = user_handle;
	return KERN_SUCCESS;
}

kern_return_t
mach_memory_object_memory_entry(
	host_t          host,
	boolean_t       internal,
	vm_size_t       size,
	vm_prot_t       permission,
	memory_object_t pager,
	ipc_port_t      *entry_handle)
{
	return mach_memory_object_memory_entry_64( host, internal,
	           (vm_object_offset_t)size, permission, pager, entry_handle);
}


kern_return_t
mach_memory_entry_purgable_control(
	ipc_port_t      entry_port,
	vm_purgable_t   control,
	int             *state)
{
	if (control == VM_PURGABLE_SET_STATE_FROM_KERNEL) {
		                                 
		return KERN_INVALID_ARGUMENT;
	}

	return memory_entry_purgeable_control_internal(entry_port, control, state);
}

kern_return_t
memory_entry_purgeable_control_internal(
	ipc_port_t      entry_port,
	vm_purgable_t   control,
	int             *state)
{
	kern_return_t           kr;
	vm_named_entry_t        mem_entry;
	vm_object_t             object;

	if (!IP_VALID(entry_port) ||
	    ip_kotype(entry_port) != IKOT_NAMED_ENTRY) {
		return KERN_INVALID_ARGUMENT;
	}
	if (control != VM_PURGABLE_SET_STATE &&
	    control != VM_PURGABLE_GET_STATE &&
	    control != VM_PURGABLE_SET_STATE_FROM_KERNEL) {
		return KERN_INVALID_ARGUMENT;
	}

	if ((control == VM_PURGABLE_SET_STATE ||
	    control == VM_PURGABLE_SET_STATE_FROM_KERNEL) &&
	    (((*state & ~(VM_PURGABLE_ALL_MASKS)) != 0) ||
	    ((*state & VM_PURGABLE_STATE_MASK) > VM_PURGABLE_STATE_MASK))) {
		return KERN_INVALID_ARGUMENT;
	}

	mem_entry = (vm_named_entry_t) ip_get_kobject(entry_port);

	named_entry_lock(mem_entry);

	if (mem_entry->is_sub_map ||
	    mem_entry->is_copy) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	assert(mem_entry->is_object);
	object = vm_named_entry_to_vm_object(mem_entry);
	if (object == VM_OBJECT_NULL) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	vm_object_lock(object);

	                                                   
	if (mem_entry->offset != 0 || object->vo_size != mem_entry->size) {
		vm_object_unlock(object);
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	named_entry_unlock(mem_entry);

	kr = vm_object_purgable_control(object, control, state);

	vm_object_unlock(object);

	return kr;
}

kern_return_t
mach_memory_entry_access_tracking(
	ipc_port_t      entry_port,
	int             *access_tracking,
	uint32_t        *access_tracking_reads,
	uint32_t        *access_tracking_writes)
{
	return memory_entry_access_tracking_internal(entry_port,
	           access_tracking,
	           access_tracking_reads,
	           access_tracking_writes);
}

kern_return_t
memory_entry_access_tracking_internal(
	ipc_port_t      entry_port,
	int             *access_tracking,
	uint32_t        *access_tracking_reads,
	uint32_t        *access_tracking_writes)
{
	vm_named_entry_t        mem_entry;
	vm_object_t             object;
	kern_return_t           kr;

	if (!IP_VALID(entry_port) ||
	    ip_kotype(entry_port) != IKOT_NAMED_ENTRY) {
		return KERN_INVALID_ARGUMENT;
	}

	mem_entry = (vm_named_entry_t) ip_get_kobject(entry_port);

	named_entry_lock(mem_entry);

	if (mem_entry->is_sub_map ||
	    mem_entry->is_copy) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	assert(mem_entry->is_object);
	object = vm_named_entry_to_vm_object(mem_entry);
	if (object == VM_OBJECT_NULL) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

#if VM_OBJECT_ACCESS_TRACKING
	vm_object_access_tracking(object,
	    access_tracking,
	    access_tracking_reads,
	    access_tracking_writes);
	kr = KERN_SUCCESS;
#else                                
	(void) access_tracking;
	(void) access_tracking_reads;
	(void) access_tracking_writes;
	kr = KERN_NOT_SUPPORTED;
#endif                                

	named_entry_unlock(mem_entry);

	return kr;
}

kern_return_t
mach_memory_entry_ownership(
	ipc_port_t      entry_port,
	task_t          owner,
	int             ledger_tag,
	int             ledger_flags)
{
	task_t                  cur_task;
	kern_return_t           kr;
	vm_named_entry_t        mem_entry;
	vm_object_t             object;

	cur_task = current_task();
	if (cur_task != kernel_task &&
	    (owner != cur_task ||
	    (ledger_flags & VM_LEDGER_FLAG_NO_FOOTPRINT) ||
	    ledger_tag == VM_LEDGER_TAG_NETWORK)) {
		  
                                   
                                                
                                                               
                                                           
     
		if (!cur_task->task_can_transfer_memory_ownership &&
		    IOTaskHasEntitlement(cur_task,
		    "com.apple.private.memory.ownership_transfer")) {
			cur_task->task_can_transfer_memory_ownership = TRUE;
		}
		if (!cur_task->task_can_transfer_memory_ownership) {
			return KERN_NO_ACCESS;
		}
	}

	if (ledger_flags & ~VM_LEDGER_FLAGS) {
		return KERN_INVALID_ARGUMENT;
	}
	if (ledger_tag <= 0 ||
	    ledger_tag > VM_LEDGER_TAG_MAX) {
		return KERN_INVALID_ARGUMENT;
	}

	if (!IP_VALID(entry_port) ||
	    ip_kotype(entry_port) != IKOT_NAMED_ENTRY) {
		return KERN_INVALID_ARGUMENT;
	}
	mem_entry = (vm_named_entry_t) ip_get_kobject(entry_port);

	named_entry_lock(mem_entry);

	if (mem_entry->is_sub_map ||
	    mem_entry->is_copy) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	assert(mem_entry->is_object);
	object = vm_named_entry_to_vm_object(mem_entry);
	if (object == VM_OBJECT_NULL) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	vm_object_lock(object);

	                                                   
	if (mem_entry->offset != 0 || object->vo_size != mem_entry->size) {
		vm_object_unlock(object);
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	named_entry_unlock(mem_entry);

	kr = vm_object_ownership_change(object,
	    ledger_tag,
	    owner,
	    ledger_flags,
	    FALSE);                                                   
	vm_object_unlock(object);

	return kr;
}

kern_return_t
mach_memory_entry_get_page_counts(
	ipc_port_t      entry_port,
	unsigned int    *resident_page_count,
	unsigned int    *dirty_page_count)
{
	kern_return_t           kr;
	vm_named_entry_t        mem_entry;
	vm_object_t             object;
	vm_object_offset_t      offset;
	vm_object_size_t        size;

	if (!IP_VALID(entry_port) ||
	    ip_kotype(entry_port) != IKOT_NAMED_ENTRY) {
		return KERN_INVALID_ARGUMENT;
	}

	mem_entry = (vm_named_entry_t) ip_get_kobject(entry_port);

	named_entry_lock(mem_entry);

	if (mem_entry->is_sub_map ||
	    mem_entry->is_copy) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	assert(mem_entry->is_object);
	object = vm_named_entry_to_vm_object(mem_entry);
	if (object == VM_OBJECT_NULL) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	vm_object_lock(object);

	offset = mem_entry->offset;
	size = mem_entry->size;
	size = vm_object_round_page(offset + size) - vm_object_trunc_page(offset);
	offset = vm_object_trunc_page(offset);

	named_entry_unlock(mem_entry);

	kr = vm_object_get_page_counts(object, offset, size, resident_page_count, dirty_page_count);

	vm_object_unlock(object);

	return kr;
}

kern_return_t
mach_memory_entry_phys_page_offset(
	ipc_port_t              entry_port,
	vm_object_offset_t      *offset_p)
{
	vm_named_entry_t        mem_entry;
	vm_object_t             object;
	vm_object_offset_t      offset;
	vm_object_offset_t      data_offset;

	if (!IP_VALID(entry_port) ||
	    ip_kotype(entry_port) != IKOT_NAMED_ENTRY) {
		return KERN_INVALID_ARGUMENT;
	}

	mem_entry = (vm_named_entry_t) ipc_kobject_get(entry_port);

	named_entry_lock(mem_entry);

	if (mem_entry->is_sub_map ||
	    mem_entry->is_copy) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	assert(mem_entry->is_object);
	object = vm_named_entry_to_vm_object(mem_entry);
	if (object == VM_OBJECT_NULL) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	offset = mem_entry->offset;
	data_offset = mem_entry->data_offset;

	named_entry_unlock(mem_entry);

	*offset_p = offset - vm_object_trunc_page(offset) + data_offset;
	return KERN_SUCCESS;
}

kern_return_t
mach_memory_entry_map_size(
	ipc_port_t             entry_port,
	vm_map_t               map,
	memory_object_offset_t offset,
	memory_object_offset_t size,
	mach_vm_size_t         *map_size)
{
	vm_named_entry_t        mem_entry;
	vm_object_t             object;
	vm_object_offset_t      object_offset_start, object_offset_end;
	vm_map_copy_t           copy_map, target_copy_map;
	vm_map_offset_t         overmap_start, overmap_end, trimmed_start;
	kern_return_t           kr;

	if (!IP_VALID(entry_port) ||
	    ip_kotype(entry_port) != IKOT_NAMED_ENTRY) {
		return KERN_INVALID_ARGUMENT;
	}

	mem_entry = (vm_named_entry_t) ipc_kobject_get(entry_port);
	named_entry_lock(mem_entry);

	if (mem_entry->is_sub_map) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	if (mem_entry->is_object) {
		object = vm_named_entry_to_vm_object(mem_entry);
		if (object == VM_OBJECT_NULL) {
			named_entry_unlock(mem_entry);
			return KERN_INVALID_ARGUMENT;
		}

		object_offset_start = mem_entry->offset;
		object_offset_start += mem_entry->data_offset;
		object_offset_start += offset;
		object_offset_end = object_offset_start + size;
		object_offset_start = vm_map_trunc_page(object_offset_start,
		    VM_MAP_PAGE_MASK(map));
		object_offset_end = vm_map_round_page(object_offset_end,
		    VM_MAP_PAGE_MASK(map));

		named_entry_unlock(mem_entry);

		*map_size = object_offset_end - object_offset_start;
		return KERN_SUCCESS;
	}

	if (!mem_entry->is_copy) {
		panic("unsupported type of mem_entry %p\n", mem_entry);
	}

	assert(mem_entry->is_copy);
	if (VM_MAP_COPY_PAGE_MASK(mem_entry->backing.copy) == VM_MAP_PAGE_MASK(map)) {
		*map_size = vm_map_round_page(mem_entry->offset + mem_entry->data_offset + offset + size, VM_MAP_PAGE_MASK(map)) - vm_map_trunc_page(mem_entry->offset + mem_entry->data_offset + offset, VM_MAP_PAGE_MASK(map));
		DEBUG4K_SHARE("map %p (%d) mem_entry %p offset 0x%llx + 0x%llx + 0x%llx size 0x%llx -> map_size 0x%llx\n", map, VM_MAP_PAGE_MASK(map), mem_entry, mem_entry->offset, mem_entry->data_offset, offset, size, *map_size);
		named_entry_unlock(mem_entry);
		return KERN_SUCCESS;
	}

	DEBUG4K_SHARE("mem_entry %p copy %p (%d) map %p (%d) offset 0x%llx size 0x%llx\n", mem_entry, mem_entry->backing.copy, VM_MAP_COPY_PAGE_SHIFT(mem_entry->backing.copy), map, VM_MAP_PAGE_SHIFT(map), offset, size);
	copy_map = mem_entry->backing.copy;
	target_copy_map = VM_MAP_COPY_NULL;
	DEBUG4K_ADJUST("adjusting...\n");
	kr = vm_map_copy_adjust_to_target(copy_map,
	    mem_entry->data_offset + offset,
	    size,
	    map,
	    FALSE,
	    &target_copy_map,
	    &overmap_start,
	    &overmap_end,
	    &trimmed_start);
	if (kr == KERN_SUCCESS) {
		if (target_copy_map->size != copy_map->size) {
			DEBUG4K_ADJUST("copy %p (%d) map %p (%d) offset 0x%llx size 0x%llx overmap_start 0x%llx overmap_end 0x%llx trimmed_start 0x%llx map_size 0x%llx -> 0x%llx\n", copy_map, VM_MAP_COPY_PAGE_SHIFT(copy_map), map, VM_MAP_PAGE_SHIFT(map), (uint64_t)offset, (uint64_t)size, (uint64_t)overmap_start, (uint64_t)overmap_end, (uint64_t)trimmed_start, (uint64_t)copy_map->size, (uint64_t)target_copy_map->size);
		}
		*map_size = target_copy_map->size;
		if (target_copy_map != copy_map) {
			vm_map_copy_discard(target_copy_map);
		}
		target_copy_map = VM_MAP_COPY_NULL;
	}
	named_entry_unlock(mem_entry);
	return kr;
}

  
                                  
  
                                                                   
                                                                    
                                                                         
   
void
mach_memory_entry_port_release(
	ipc_port_t      port)
{
	assert(ip_kotype(port) == IKOT_NAMED_ENTRY);
	ipc_port_release_send(port);
}

  
                             
  
                                                                       
                                      
                                                                             
                                                                          
                                                                       
                                                                              
                                                                             
                                                     
   
void
mach_destroy_memory_entry(
	ipc_port_t      port)
{
	vm_named_entry_t        named_entry;
#if MACH_ASSERT
	assert(ip_kotype(port) == IKOT_NAMED_ENTRY);
#endif                  
	named_entry = (vm_named_entry_t) ip_get_kobject(port);

	named_entry_lock(named_entry);
	named_entry->ref_count -= 1;

	if (named_entry->ref_count == 0) {
		if (named_entry->is_sub_map) {
			vm_map_deallocate(named_entry->backing.map);
		} else if (named_entry->is_copy) {
			vm_map_copy_discard(named_entry->backing.copy);
		} else if (named_entry->is_object) {
			assert(named_entry->backing.copy->cpy_hdr.nentries == 1);
			vm_map_copy_discard(named_entry->backing.copy);
		} else {
			assert(named_entry->backing.copy == VM_MAP_COPY_NULL);
		}

		named_entry_unlock(named_entry);
		named_entry_lock_destroy(named_entry);

#if VM_NAMED_ENTRY_LIST
		lck_mtx_lock_spin(&vm_named_entry_list_lock_data);
		queue_remove(&vm_named_entry_list, named_entry,
		    vm_named_entry_t, named_entry_list);
		assert(vm_named_entry_count > 0);
		vm_named_entry_count--;
		lck_mtx_unlock(&vm_named_entry_list_lock_data);
#endif                          

		kfree(named_entry, sizeof(struct vm_named_entry));
	} else {
		named_entry_unlock(named_entry);
	}
}

                                                                            
                                                                              

kern_return_t
mach_memory_entry_page_op(
	ipc_port_t              entry_port,
	vm_object_offset_t      offset,
	int                     ops,
	ppnum_t                 *phys_entry,
	int                     *flags)
{
	vm_named_entry_t        mem_entry;
	vm_object_t             object;
	kern_return_t           kr;

	if (!IP_VALID(entry_port) ||
	    ip_kotype(entry_port) != IKOT_NAMED_ENTRY) {
		return KERN_INVALID_ARGUMENT;
	}

	mem_entry = (vm_named_entry_t) ip_get_kobject(entry_port);

	named_entry_lock(mem_entry);

	if (mem_entry->is_sub_map ||
	    mem_entry->is_copy) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	assert(mem_entry->is_object);
	object = vm_named_entry_to_vm_object(mem_entry);
	if (object == VM_OBJECT_NULL) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	vm_object_reference(object);
	named_entry_unlock(mem_entry);

	kr = vm_object_page_op(object, offset, ops, phys_entry, flags);

	vm_object_deallocate(object);

	return kr;
}

  
                                                                 
                                                                            
                                                                            
                                                                          
                                                                           
                                                                             
                                                                                
   

kern_return_t
mach_memory_entry_range_op(
	ipc_port_t              entry_port,
	vm_object_offset_t      offset_beg,
	vm_object_offset_t      offset_end,
	int                     ops,
	int                     *range)
{
	vm_named_entry_t        mem_entry;
	vm_object_t             object;
	kern_return_t           kr;

	if (!IP_VALID(entry_port) ||
	    ip_kotype(entry_port) != IKOT_NAMED_ENTRY) {
		return KERN_INVALID_ARGUMENT;
	}

	mem_entry = (vm_named_entry_t) ip_get_kobject(entry_port);

	named_entry_lock(mem_entry);

	if (mem_entry->is_sub_map ||
	    mem_entry->is_copy) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	assert(mem_entry->is_object);
	object = vm_named_entry_to_vm_object(mem_entry);
	if (object == VM_OBJECT_NULL) {
		named_entry_unlock(mem_entry);
		return KERN_INVALID_ARGUMENT;
	}

	vm_object_reference(object);
	named_entry_unlock(mem_entry);

	kr = vm_object_range_op(object,
	    offset_beg,
	    offset_end,
	    ops,
	    (uint32_t *) range);

	vm_object_deallocate(object);

	return kr;
}

                                                           

extern int kernel_upl_map(
	vm_map_t        map,
	upl_t           upl,
	vm_offset_t     *dst_addr);

extern int kernel_upl_unmap(
	vm_map_t        map,
	upl_t           upl);

extern int kernel_upl_commit(
	upl_t                   upl,
	upl_page_info_t         *pl,
	mach_msg_type_number_t   count);

extern int kernel_upl_commit_range(
	upl_t                   upl,
	upl_offset_t             offset,
	upl_size_t              size,
	int                     flags,
	upl_page_info_array_t   pl,
	mach_msg_type_number_t  count);

extern int kernel_upl_abort(
	upl_t                   upl,
	int                     abort_type);

extern int kernel_upl_abort_range(
	upl_t                   upl,
	upl_offset_t             offset,
	upl_size_t               size,
	int                     abort_flags);


kern_return_t
kernel_upl_map(
	vm_map_t        map,
	upl_t           upl,
	vm_offset_t     *dst_addr)
{
	return vm_upl_map(map, upl, dst_addr);
}


kern_return_t
kernel_upl_unmap(
	vm_map_t        map,
	upl_t           upl)
{
	return vm_upl_unmap(map, upl);
}

kern_return_t
kernel_upl_commit(
	upl_t                   upl,
	upl_page_info_t        *pl,
	mach_msg_type_number_t  count)
{
	kern_return_t   kr;

	kr = upl_commit(upl, pl, count);
	upl_deallocate(upl);
	return kr;
}


kern_return_t
kernel_upl_commit_range(
	upl_t                   upl,
	upl_offset_t            offset,
	upl_size_t              size,
	int                     flags,
	upl_page_info_array_t   pl,
	mach_msg_type_number_t  count)
{
	boolean_t               finished = FALSE;
	kern_return_t           kr;

	if (flags & UPL_COMMIT_FREE_ON_EMPTY) {
		flags |= UPL_COMMIT_NOTIFY_EMPTY;
	}

	if (flags & UPL_COMMIT_KERNEL_ONLY_FLAGS) {
		return KERN_INVALID_ARGUMENT;
	}

	kr = upl_commit_range(upl, offset, size, flags, pl, count, &finished);

	if ((flags & UPL_COMMIT_NOTIFY_EMPTY) && finished) {
		upl_deallocate(upl);
	}

	return kr;
}

kern_return_t
kernel_upl_abort_range(
	upl_t                   upl,
	upl_offset_t            offset,
	upl_size_t              size,
	int                     abort_flags)
{
	kern_return_t           kr;
	boolean_t               finished = FALSE;

	if (abort_flags & UPL_COMMIT_FREE_ON_EMPTY) {
		abort_flags |= UPL_COMMIT_NOTIFY_EMPTY;
	}

	kr = upl_abort_range(upl, offset, size, abort_flags, &finished);

	if ((abort_flags & UPL_COMMIT_FREE_ON_EMPTY) && finished) {
		upl_deallocate(upl);
	}

	return kr;
}

kern_return_t
kernel_upl_abort(
	upl_t                   upl,
	int                     abort_type)
{
	kern_return_t   kr;

	kr = upl_abort(upl, abort_type);
	upl_deallocate(upl);
	return kr;
}

  
                                                
                                              
                                            
   

kern_return_t
vm_region_object_create(
	__unused vm_map_t       target_map,
	vm_size_t               size,
	ipc_port_t              *object_handle)
{
	vm_named_entry_t        user_entry;
	ipc_port_t              user_handle;

	vm_map_t        new_map;

	if (mach_memory_entry_allocate(&user_entry, &user_handle)
	    != KERN_SUCCESS) {
		return KERN_FAILURE;
	}

	                                                               

	new_map = vm_map_create(PMAP_NULL, VM_MAP_MIN_ADDRESS,
	    vm_map_round_page(size,
	    VM_MAP_PAGE_MASK(target_map)),
	    TRUE);
	vm_map_set_page_shift(new_map, VM_MAP_PAGE_SHIFT(target_map));

	user_entry->backing.map = new_map;
	user_entry->internal = TRUE;
	user_entry->is_sub_map = TRUE;
	user_entry->offset = 0;
	user_entry->protection = VM_PROT_ALL;
	user_entry->size = size;
	assert(user_entry->ref_count == 1);

	*object_handle = user_handle;
	return KERN_SUCCESS;
}

ppnum_t vm_map_get_phys_page(                        
	vm_map_t        map,
	vm_offset_t     offset);

ppnum_t
vm_map_get_phys_page(
	vm_map_t                map,
	vm_offset_t             addr)
{
	vm_object_offset_t      offset;
	vm_object_t             object;
	vm_map_offset_t         map_offset;
	vm_map_entry_t          entry;
	ppnum_t                 phys_page = 0;

	map_offset = vm_map_trunc_page(addr, PAGE_MASK);

	vm_map_lock(map);
	while (vm_map_lookup_entry(map, map_offset, &entry)) {
		if (VME_OBJECT(entry) == VM_OBJECT_NULL) {
			vm_map_unlock(map);
			return (ppnum_t) 0;
		}
		if (entry->is_sub_map) {
			vm_map_t        old_map;
			vm_map_lock(VME_SUBMAP(entry));
			old_map = map;
			map = VME_SUBMAP(entry);
			map_offset = (VME_OFFSET(entry) +
			    (map_offset - entry->vme_start));
			vm_map_unlock(old_map);
			continue;
		}
		if (VME_OBJECT(entry)->phys_contiguous) {
			                                                      
			                                                      
			                                                      
			                       
			if (VME_OBJECT(entry)->vo_shadow_offset == 0) {
				                           
				vm_map_unlock(map);
				vm_fault(map, map_offset, VM_PROT_NONE,
				    FALSE                    , VM_KERN_MEMORY_NONE,
				    THREAD_UNINT, NULL, 0);
				vm_map_lock(map);
				continue;
			}
			offset = (VME_OFFSET(entry) +
			    (map_offset - entry->vme_start));
			phys_page = (ppnum_t)
			    ((VME_OBJECT(entry)->vo_shadow_offset
			    + offset) >> PAGE_SHIFT);
			break;
		}
		offset = (VME_OFFSET(entry) + (map_offset - entry->vme_start));
		object = VME_OBJECT(entry);
		vm_object_lock(object);
		while (TRUE) {
			vm_page_t dst_page = vm_page_lookup(object, offset);
			if (dst_page == VM_PAGE_NULL) {
				if (object->shadow) {
					vm_object_t old_object;
					vm_object_lock(object->shadow);
					old_object = object;
					offset = offset + object->vo_shadow_offset;
					object = object->shadow;
					vm_object_unlock(old_object);
				} else {
					vm_object_unlock(object);
					break;
				}
			} else {
				phys_page = (ppnum_t)(VM_PAGE_GET_PHYS_PAGE(dst_page));
				vm_object_unlock(object);
				break;
			}
		}
		break;
	}

	vm_map_unlock(map);
	return phys_page;
}

#if 0
kern_return_t kernel_object_iopl_request(                    
	vm_named_entry_t        named_entry,
	memory_object_offset_t  offset,
	upl_size_t              *upl_size,
	upl_t                   *upl_ptr,
	upl_page_info_array_t   user_page_list,
	unsigned int            *page_list_count,
	int                     *flags);

kern_return_t
kernel_object_iopl_request(
	vm_named_entry_t        named_entry,
	memory_object_offset_t  offset,
	upl_size_t              *upl_size,
	upl_t                   *upl_ptr,
	upl_page_info_array_t   user_page_list,
	unsigned int            *page_list_count,
	int                     *flags)
{
	vm_object_t             object;
	kern_return_t           ret;

	int                     caller_flags;

	caller_flags = *flags;

	if (caller_flags & ~UPL_VALID_FLAGS) {
		  
                                      
                             
     
		return KERN_INVALID_VALUE;
	}

	                                                     
	if (*upl_size == 0) {
		if (offset >= named_entry->size) {
			return KERN_INVALID_RIGHT;
		}
		*upl_size = (upl_size_t) (named_entry->size - offset);
		if (*upl_size != named_entry->size - offset) {
			return KERN_INVALID_ARGUMENT;
		}
	}
	if (caller_flags & UPL_COPYOUT_FROM) {
		if ((named_entry->protection & VM_PROT_READ)
		    != VM_PROT_READ) {
			return KERN_INVALID_RIGHT;
		}
	} else {
		if ((named_entry->protection &
		    (VM_PROT_READ | VM_PROT_WRITE))
		    != (VM_PROT_READ | VM_PROT_WRITE)) {
			return KERN_INVALID_RIGHT;
		}
	}
	if (named_entry->size < (offset + *upl_size)) {
		return KERN_INVALID_ARGUMENT;
	}

	                                                       
	                                                           
	offset = offset + named_entry->offset;

	if (named_entry->is_sub_map ||
	    named_entry->is_copy) {
		return KERN_INVALID_ARGUMENT;
	}

	named_entry_lock(named_entry);

	                                                    
	                                                   
	                                                
	                                                
	                                                
	                    
	assert(named_entry->is_object);
	object = vm_named_entry_to_vm_object(named_entry);
	vm_object_reference(object);
	named_entry_unlock(named_entry);

	if (!object->private) {
		if (*upl_size > MAX_UPL_TRANSFER_BYTES) {
			*upl_size = MAX_UPL_TRANSFER_BYTES;
		}
		if (object->phys_contiguous) {
			*flags = UPL_PHYS_CONTIG;
		} else {
			*flags = 0;
		}
	} else {
		*flags = UPL_DEV_MEMORY | UPL_PHYS_CONTIG;
	}

	ret = vm_object_iopl_request(object,
	    offset,
	    *upl_size,
	    upl_ptr,
	    user_page_list,
	    page_list_count,
	    (upl_control_flags_t)(unsigned int)caller_flags);
	vm_object_deallocate(object);
	return ret;
}
#endif

  
                                                                
                                                 
   

#if defined(__x86_64__)

kern_return_t
mach_vm_map(
	vm_map_t                target_map,
	mach_vm_offset_t        *address,
	mach_vm_size_t  initial_size,
	mach_vm_offset_t        mask,
	int                     flags,
	ipc_port_t              port,
	vm_object_offset_t      offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance);

kern_return_t
mach_vm_remap(
	vm_map_t                target_map,
	mach_vm_offset_t        *address,
	mach_vm_size_t  size,
	mach_vm_offset_t        mask,
	int                     flags,
	vm_map_t                src_map,
	mach_vm_offset_t        memory_address,
	boolean_t               copy,
	vm_prot_t               *cur_protection,
	vm_prot_t               *max_protection,
	vm_inherit_t            inheritance);

kern_return_t
mach_vm_map(
	vm_map_t                target_map,
	mach_vm_offset_t        *address,
	mach_vm_size_t  initial_size,
	mach_vm_offset_t        mask,
	int                     flags,
	ipc_port_t              port,
	vm_object_offset_t      offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	return mach_vm_map_external(target_map, address, initial_size, mask, flags, port,
	           offset, copy, cur_protection, max_protection, inheritance);
}

kern_return_t
mach_vm_remap(
	vm_map_t                target_map,
	mach_vm_offset_t        *address,
	mach_vm_size_t  size,
	mach_vm_offset_t        mask,
	int                     flags,
	vm_map_t                src_map,
	mach_vm_offset_t        memory_address,
	boolean_t               copy,
	vm_prot_t               *cur_protection,            
	vm_prot_t               *max_protection,            
	vm_inherit_t            inheritance)
{
	return mach_vm_remap_external(target_map, address, size, mask, flags, src_map, memory_address,
	           copy, cur_protection, max_protection, inheritance);
}

kern_return_t
vm_map(
	vm_map_t                target_map,
	vm_offset_t             *address,
	vm_size_t               size,
	vm_offset_t             mask,
	int                     flags,
	ipc_port_t              port,
	vm_offset_t             offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance);

kern_return_t
vm_map(
	vm_map_t                target_map,
	vm_offset_t             *address,
	vm_size_t               size,
	vm_offset_t             mask,
	int                     flags,
	ipc_port_t              port,
	vm_offset_t             offset,
	boolean_t               copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance)
{
	vm_tag_t tag;

	VM_GET_FLAGS_ALIAS(flags, tag);
	return vm_map_kernel(target_map, address, size, mask,
	           flags, VM_MAP_KERNEL_FLAGS_NONE, tag,
	           port, offset, copy,
	           cur_protection, max_protection, inheritance);
}

#endif                 
