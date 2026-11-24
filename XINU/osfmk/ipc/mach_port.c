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
/*
 * NOTICE: This file was modified by McAfee Research in 2004 to introduce
 * support for mandatory and extensible security protections.  This notice
 * is included in support of clause 2.2 (b) of the Apple Public License,
 * Version 2.0.
 * Copyright (c) 2005-2006 SPARTA, Inc.
 */
  
   
  
                        
                      
               
  
                                                   
   

#include <mach_debug.h>

#include <mach/port.h>
#include <mach/kern_return.h>
#include <mach/notify.h>
#include <mach/mach_param.h>
#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <mach/vm_map.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/exc_guard.h>
#include <mach/mach_port_server.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <ipc/port.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_notify.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_right.h>
#include <ipc/ipc_kmsg.h>
#include <kern/misc_protos.h>
#include <security/mac_mach_internal.h>
#include <kern/work_interval.h>
#include <kern/policy_internal.h>

#if IMPORTANCE_INHERITANCE
#include <ipc/ipc_importance.h>
#endif

kern_return_t mach_port_get_attributes(ipc_space_t space, mach_port_name_t name,
    int flavor, mach_port_info_t info, mach_msg_type_number_t  *count);
kern_return_t mach_port_get_context(ipc_space_t space, mach_port_name_t name,
    mach_vm_address_t *context);
kern_return_t mach_port_get_set_status(ipc_space_t space, mach_port_name_t name,
    mach_port_name_t **members, mach_msg_type_number_t *membersCnt);

                                  

static mach_port_qos_t  qos_template;

  
                                  
           
                                          
  
              
                                                     
   
static void
mach_port_names_helper(
	ipc_port_timestamp_t    timestamp,
	ipc_entry_t             entry,
	mach_port_name_t        name,
	mach_port_name_t        *names,
	mach_port_type_t        *types,
	ipc_entry_num_t         *actualp)
{
	ipc_entry_bits_t bits;
	ipc_port_request_index_t request;
	mach_port_type_t type = 0;
	ipc_entry_num_t actual;
	ipc_port_t port;

	bits = entry->ie_bits;
	request = entry->ie_request;
	port = ip_object_to_port(entry->ie_object);

	if (bits & MACH_PORT_TYPE_RECEIVE) {
		assert(IP_VALID(port));

		if (request != IE_REQ_NONE) {
			ip_lock(port);
			require_ip_active(port);
			type |= ipc_port_request_type(port, name, request);
			ip_unlock(port);
		}
	} else if (bits & MACH_PORT_TYPE_SEND_RIGHTS) {
		mach_port_type_t reqtype;

		assert(IP_VALID(port));
		ip_lock(port);

		reqtype = (request != IE_REQ_NONE) ?
		    ipc_port_request_type(port, name, request) : 0;

		  
                                                                
                                                                 
                       
     
		if (ip_active(port) || IP_TIMESTAMP_ORDER(timestamp, port->ip_timestamp)) {
			type |= reqtype;
		} else {
			bits &= ~(IE_BITS_TYPE_MASK);
			bits |= MACH_PORT_TYPE_DEAD_NAME;
			                                                                 
			if (reqtype != 0) {
				bits++;
			}
		}
		ip_unlock(port);
	}

	type |= IE_BITS_TYPE(bits);

	actual = *actualp;
	names[actual] = name;
	types[actual] = type;
	*actualp = actual + 1;
}

  
                                         
           
                                                        
                                                   
                                                  
                                                     
                                          
              
                   
           
                                                     
                                        
                                        
                                                    
   

kern_return_t
mach_port_names(
	ipc_space_t             space,
	mach_port_name_t        **namesp,
	mach_msg_type_number_t  *namesCnt,
	mach_port_type_t        **typesp,
	mach_msg_type_number_t  *typesCnt)
{
	ipc_entry_t table;
	ipc_entry_num_t tsize;
	mach_port_index_t index;
	ipc_entry_num_t actual;                      
	ipc_port_timestamp_t timestamp;                                     
	mach_port_name_t *names;
	mach_port_type_t *types;
	kern_return_t kr;

	vm_size_t size;                                       
	vm_offset_t addr1;                                       
	vm_offset_t addr2;                                       
	vm_map_copy_t memory1;                                   
	vm_map_copy_t memory2;                                   

	                                 
	static_assert(sizeof(mach_port_name_t) == sizeof(mach_port_type_t));

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	size = 0;

	for (;;) {
		ipc_entry_num_t bound;
		vm_size_t size_needed;

		is_read_lock(space);
		if (!is_active(space)) {
			is_read_unlock(space);
			if (size != 0) {
				kmem_free(ipc_kernel_map, addr1, size);
				kmem_free(ipc_kernel_map, addr2, size);
			}
			return KERN_INVALID_TASK;
		}

		                                                 
		bound = space->is_table_size;
		size_needed = vm_map_round_page(
			(bound * sizeof(mach_port_name_t)),
			VM_MAP_PAGE_MASK(ipc_kernel_map));

		if (size_needed <= size) {
			break;
		}

		is_read_unlock(space);

		if (size != 0) {
			kmem_free(ipc_kernel_map, addr1, size);
			kmem_free(ipc_kernel_map, addr2, size);
		}
		size = size_needed;

		kr = vm_allocate_kernel(ipc_kernel_map, &addr1, size, VM_FLAGS_ANYWHERE, VM_KERN_MEMORY_IPC);
		if (kr != KERN_SUCCESS) {
			return KERN_RESOURCE_SHORTAGE;
		}

		kr = vm_allocate_kernel(ipc_kernel_map, &addr2, size, VM_FLAGS_ANYWHERE, VM_KERN_MEMORY_IPC);
		if (kr != KERN_SUCCESS) {
			kmem_free(ipc_kernel_map, addr1, size);
			return KERN_RESOURCE_SHORTAGE;
		}

		                                     

		kr = vm_map_wire_kernel(
			ipc_kernel_map,
			vm_map_trunc_page(addr1,
			VM_MAP_PAGE_MASK(ipc_kernel_map)),
			vm_map_round_page(addr1 + size,
			VM_MAP_PAGE_MASK(ipc_kernel_map)),
			VM_PROT_READ | VM_PROT_WRITE, VM_KERN_MEMORY_IPC,
			FALSE);
		if (kr != KERN_SUCCESS) {
			kmem_free(ipc_kernel_map, addr1, size);
			kmem_free(ipc_kernel_map, addr2, size);
			return KERN_RESOURCE_SHORTAGE;
		}

		kr = vm_map_wire_kernel(
			ipc_kernel_map,
			vm_map_trunc_page(addr2,
			VM_MAP_PAGE_MASK(ipc_kernel_map)),
			vm_map_round_page(addr2 + size,
			VM_MAP_PAGE_MASK(ipc_kernel_map)),
			VM_PROT_READ | VM_PROT_WRITE,
			VM_KERN_MEMORY_IPC,
			FALSE);
		if (kr != KERN_SUCCESS) {
			kmem_free(ipc_kernel_map, addr1, size);
			kmem_free(ipc_kernel_map, addr2, size);
			return KERN_RESOURCE_SHORTAGE;
		}
	}
	                                     

	names = (mach_port_name_t *) addr1;
	types = (mach_port_type_t *) addr2;
	actual = 0;

	timestamp = ipc_port_timestamp();

	table = space->is_table;
	tsize = space->is_table_size;

	for (index = 0; index < tsize; index++) {
		ipc_entry_t entry = &table[index];
		ipc_entry_bits_t bits = entry->ie_bits;

		if (IE_BITS_TYPE(bits) != MACH_PORT_TYPE_NONE) {
			mach_port_name_t name;

			name = MACH_PORT_MAKE(index, IE_BITS_GEN(bits));
			mach_port_names_helper(timestamp, entry, name, names,
			    types, &actual);
		}
	}

	is_read_unlock(space);

	if (actual == 0) {
		memory1 = VM_MAP_COPY_NULL;
		memory2 = VM_MAP_COPY_NULL;

		if (size != 0) {
			kmem_free(ipc_kernel_map, addr1, size);
			kmem_free(ipc_kernel_map, addr2, size);
		}
	} else {
		vm_size_t size_used;
		vm_size_t vm_size_used;

		size_used = actual * sizeof(mach_port_name_t);
		vm_size_used =
		    vm_map_round_page(size_used,
		    VM_MAP_PAGE_MASK(ipc_kernel_map));

		  
                                              
                                             
     

		kr = vm_map_unwire(
			ipc_kernel_map,
			vm_map_trunc_page(addr1,
			VM_MAP_PAGE_MASK(ipc_kernel_map)),
			vm_map_round_page(addr1 + vm_size_used,
			VM_MAP_PAGE_MASK(ipc_kernel_map)),
			FALSE);
		assert(kr == KERN_SUCCESS);

		kr = vm_map_unwire(
			ipc_kernel_map,
			vm_map_trunc_page(addr2,
			VM_MAP_PAGE_MASK(ipc_kernel_map)),
			vm_map_round_page(addr2 + vm_size_used,
			VM_MAP_PAGE_MASK(ipc_kernel_map)),
			FALSE);
		assert(kr == KERN_SUCCESS);

		kr = vm_map_copyin(ipc_kernel_map, (vm_map_address_t)addr1,
		    (vm_map_size_t)size_used, TRUE, &memory1);
		assert(kr == KERN_SUCCESS);

		kr = vm_map_copyin(ipc_kernel_map, (vm_map_address_t)addr2,
		    (vm_map_size_t)size_used, TRUE, &memory2);
		assert(kr == KERN_SUCCESS);

		if (vm_size_used != size) {
			kmem_free(ipc_kernel_map,
			    addr1 + vm_size_used, size - vm_size_used);
			kmem_free(ipc_kernel_map,
			    addr2 + vm_size_used, size - vm_size_used);
		}
	}

	*namesp = (mach_port_name_t *) memory1;
	*namesCnt = actual;
	*typesp = (mach_port_type_t *) memory2;
	*typesCnt = actual;
	return KERN_SUCCESS;
}

  
                                        
           
                                               
                                                    
                               
                        
                           
                             
                            
                             
                                                               
                             
                                            
              
                   
           
                                   
                                        
                                        
                                                      
   

kern_return_t
mach_port_type(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_port_type_t        *typep)
{
	mach_port_urefs_t urefs;
	ipc_entry_t entry;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (name == MACH_PORT_NULL) {
		return KERN_INVALID_NAME;
	}

	if (name == MACH_PORT_DEAD) {
		*typep = MACH_PORT_TYPE_DEAD_NAME;
		return KERN_SUCCESS;
	}

	kr = ipc_right_lookup_write(space, name, &entry);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	                                      
	kr = ipc_right_info(space, name, entry, typep, &urefs);
	                       

#if 1
	                                                                                 
	*typep &= ~(MACH_PORT_TYPE_SPREQUEST | MACH_PORT_TYPE_SPREQUEST_DELAYED);
#endif

	return kr;
}

  
                                          
           
                                      
                        
              
                   
           
                                       
                                        
                                        
                                                       
                                                    
                                                       
                                                    
  
                                                     
                           
   

kern_return_t
mach_port_rename(
	__unused ipc_space_t            space,
	__unused mach_port_name_t       oname,
	__unused mach_port_name_t       nname)
{
	return KERN_NOT_SUPPORTED;
}


  
                                                 
           
                                                       
                                        
                            
                             
                              
  
                                                       
                                                        
                                                          
                                                        
                                                   
                                         
  
                                  
  
                                           
              
                   
           
                                         
                                        
                                        
                                                   
                                                           
                                                      
                                                    
  
                                                                    
                                                                  
                 
   

kern_return_t
mach_port_allocate_name(
	ipc_space_t             space,
	mach_port_right_t       right,
	mach_port_name_t        name)
{
	kern_return_t           kr;
	mach_port_qos_t         qos = qos_template;

	qos.name = TRUE;

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_VALUE;
	}

	kr = mach_port_allocate_full(space, right, MACH_PORT_NULL,
	    &qos, &name);
	return kr;
}

  
                                            
           
                                                                
                                                              
                                                            
                             
              
                   
           
                                         
                                        
                                        
                                                           
                                                    
                                                      
   

kern_return_t
mach_port_allocate(
	ipc_space_t             space,
	mach_port_right_t       right,
	mach_port_name_t        *namep)
{
	kern_return_t           kr;
	mach_port_qos_t         qos = qos_template;

	kr = mach_port_allocate_full(space, right, MACH_PORT_NULL,
	    &qos, namep);
	return kr;
}

  
                                                
           
                                                          
                                                           
                                                              
                                                       
              
                   
           
                                         
                                        
                                        
                                                           
                                                        
                                                    
                                                      
   

kern_return_t
mach_port_allocate_qos(
	ipc_space_t             space,
	mach_port_right_t       right,
	mach_port_qos_t         *qosp,
	mach_port_name_t        *namep)
{
	kern_return_t           kr;

	if (qosp->name) {
		return KERN_INVALID_ARGUMENT;
	}
	kr = mach_port_allocate_full(space, right, MACH_PORT_NULL,
	    qosp, namep);
	return kr;
}

  
                                                 
           
                                               
                                                   
                                                   
                             
  
                                                   
                                                   
                                                  
                               
              
                   
           
                                         
                                        
                                        
                                                           
                                                    
                                                      
   

kern_return_t
mach_port_allocate_full(
	ipc_space_t             space,
	mach_port_right_t       right,
	mach_port_t             proto,
	mach_port_qos_t         *qosp,
	mach_port_name_t        *namep)
{
	kern_return_t           kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (proto != MACH_PORT_NULL) {
		return KERN_INVALID_VALUE;
	}

	if (qosp->name) {
		if (!MACH_PORT_VALID(*namep)) {
			return KERN_INVALID_VALUE;
		}
	}

	  
                                                          
                                                                   
                                       
    
	if (qosp->prealloc) {
		if (qosp->len > MACH_MSG_SIZE_MAX - MAX_TRAILER_SIZE) {
			return KERN_RESOURCE_SHORTAGE;
		}
		if (right != MACH_PORT_RIGHT_RECEIVE) {
			return KERN_INVALID_VALUE;
		}
		qosp->prealloc = 0;
	}

	kr = mach_port_allocate_internal(space, right, qosp, namep);
	return kr;
}


  
                                                        
           
                                                      
                                                          
                                                            
                             
              
                   
           
                                         
                                        
                                        
                                                           
                                                    
                                                      
   
kern_return_t
mach_port_allocate_internal(
	ipc_space_t             space,
	mach_port_right_t       right,
	mach_port_qos_t         *qosp,
	mach_port_name_t        *namep)
{
	kern_return_t   kr;

	assert(space != IS_NULL);

	switch (right) {
	case MACH_PORT_RIGHT_RECEIVE:
	{
		ipc_kmsg_t      kmsg = IKM_NULL;
		ipc_port_t      port;

		  
                                                              
                                        
     
		if (qosp->prealloc) {
			mach_msg_size_t size = qosp->len;

			if (size > IKM_SAVED_MSG_SIZE - MAX_TRAILER_SIZE) {
				panic("mach_port_allocate_internal: too large a prealloc kmsg");
			}
			kmsg = (ipc_kmsg_t)ipc_kmsg_prealloc(size + MAX_TRAILER_SIZE);
			if (kmsg == IKM_NULL) {
				return KERN_RESOURCE_SHORTAGE;
			}
		}

		if (qosp->name) {
			kr = ipc_port_alloc_name(space, IPC_PORT_INIT_MESSAGE_QUEUE,
			    *namep, &port);
		} else {
			kr = ipc_port_alloc(space, IPC_PORT_INIT_MESSAGE_QUEUE,
			    namep, &port);
		}
		if (kr == KERN_SUCCESS) {
			if (kmsg != IKM_NULL) {
				ipc_kmsg_set_prealloc(kmsg, port);
			}
			ip_unlock(port);
		} else if (kmsg != IKM_NULL) {
			ipc_kmsg_free(kmsg);
		}
		break;
	}

	case MACH_PORT_RIGHT_PORT_SET:
	{
		ipc_pset_t      pset;

		if (qosp->name) {
			kr = ipc_pset_alloc_name(space, *namep, &pset);
		} else {
			kr = ipc_pset_alloc(space, namep, &pset);
		}
		if (kr == KERN_SUCCESS) {
			ips_unlock(pset);
		}
		break;
	}

	case MACH_PORT_RIGHT_DEAD_NAME:
		kr = ipc_object_alloc_dead(space, namep);
		break;

	default:
		kr = KERN_INVALID_VALUE;
		break;
	}

	return kr;
}

  
                                           
           
                                                       
                                                   
                                                      
                                                             
                          
              
                   
           
                                        
                                        
                                        
                                                      
   

kern_return_t
mach_port_destroy(
	ipc_space_t             space,
	mach_port_name_t        name)
{
	ipc_entry_t entry;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_SUCCESS;
	}

	kr = ipc_right_lookup_write(space, name, &entry);
	if (kr != KERN_SUCCESS) {
		mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_NAME);
		return kr;
	}
	                                      

	kr = ipc_right_destroy(space, name, entry, TRUE, 0);                    
	return kr;
}

  
                                              
           
                                                   
                                                         
                                                       
                                              
                 
              
                   
           
                                          
                                        
                                        
                                                      
                                               
   

kern_return_t
mach_port_deallocate(
	ipc_space_t             space,
	mach_port_name_t        name)
{
	ipc_entry_t entry;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_SUCCESS;
	}

	kr = ipc_right_lookup_write(space, name, &entry);
	if (kr != KERN_SUCCESS) {
		mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_NAME);
		return kr;
	}
	                           

	kr = ipc_right_dealloc(space, name, entry);                    
	return kr;
}

  
                                            
           
                                                            
                                                         
                                                        
                                                    
              
                   
           
                                           
                                        
                                        
                                                   
                                                      
   

kern_return_t
mach_port_get_refs(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_port_right_t       right,
	mach_port_urefs_t       *urefsp)
{
	mach_port_type_t type;
	mach_port_urefs_t urefs;
	ipc_entry_t entry;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (right >= MACH_PORT_RIGHT_NUMBER) {
		return KERN_INVALID_VALUE;
	}

	if (!MACH_PORT_VALID(name)) {
		if (right == MACH_PORT_RIGHT_SEND ||
		    right == MACH_PORT_RIGHT_SEND_ONCE) {
			*urefsp = 1;
			return KERN_SUCCESS;
		}
		return KERN_INVALID_NAME;
	}

	kr = ipc_right_lookup_write(space, name, &entry);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	                                      
	kr = ipc_right_info(space, name, entry, &type, &urefs);
	                       

	if (kr != KERN_SUCCESS) {
		return kr;
	}

	if (type & MACH_PORT_TYPE(right)) {
		switch (right) {
		case MACH_PORT_RIGHT_SEND_ONCE:
			assert(urefs == 1);
			OS_FALLTHROUGH;

		case MACH_PORT_RIGHT_PORT_SET:
		case MACH_PORT_RIGHT_RECEIVE:
			*urefsp = 1;
			break;

		case MACH_PORT_RIGHT_DEAD_NAME:
		case MACH_PORT_RIGHT_SEND:
			assert(urefs > 0);
			*urefsp = urefs;
			break;

		default:
			panic("mach_port_get_refs: strange rights");
		}
	} else {
		*urefsp = 0;
	}

	return kr;
}

  
                              
           
                                                           
                                                                 
                                                         
                                                 
              
                   
           
                                           
                                        
                                        
                                                   
                                                      
                                                           
                                                        
                                             
   

kern_return_t
mach_port_mod_refs(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_port_right_t       right,
	mach_port_delta_t       delta)
{
	ipc_entry_t entry;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (right >= MACH_PORT_RIGHT_NUMBER) {
		return KERN_INVALID_VALUE;
	}

	if (!MACH_PORT_VALID(name)) {
		if (right == MACH_PORT_RIGHT_SEND ||
		    right == MACH_PORT_RIGHT_SEND_ONCE) {
			return KERN_SUCCESS;
		}
		return KERN_INVALID_NAME;
	}

	kr = ipc_right_lookup_write(space, name, &entry);
	if (kr != KERN_SUCCESS) {
		mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_NAME);
		return kr;
	}

	                                      

	kr = ipc_right_delta(space, name, entry, right, delta);              
	return kr;
}


  
                                        
           
                                                       
                                                       
  
                                                      
                                                           
                                                        
           
  
                                                             
                                                     
  
                                            
                                                     
                                                      
                                                       
                                                      
  
                                                           
                                                    
  
                                                                 
                                                               
                                   
  
              
                   
           
                                                             
                                                
                                                      
                                                          
                                                                    
                                                      
   

kern_return_t
mach_port_peek(
	ipc_space_t                     space,
	mach_port_name_t                name,
	mach_msg_trailer_type_t         trailer_type,
	mach_port_seqno_t               *seqnop,
	mach_msg_size_t                 *msg_sizep,
	mach_msg_id_t                   *msg_idp,
	mach_msg_trailer_info_t         trailer_infop,
	mach_msg_type_number_t          *trailer_sizep)
{
	ipc_port_t port;
	kern_return_t kr;
	boolean_t found;
	mach_msg_max_trailer_t max_trailer;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_RIGHT;
	}

	  
                                                                     
                                                                           
    
	if (GET_RCV_ELEMENTS(trailer_type) > MACH_RCV_TRAILER_AUDIT ||
	    REQUESTED_TRAILER_SIZE(TRUE, trailer_type) > *trailer_sizep) {
		mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_VALUE);
		return KERN_INVALID_VALUE;
	}

	*trailer_sizep = REQUESTED_TRAILER_SIZE(TRUE, trailer_type);

	kr = ipc_port_translate_receive(space, name, &port);
	if (kr != KERN_SUCCESS) {
		mach_port_guard_exception(name, 0, 0,
		    ((KERN_INVALID_NAME == kr) ?
		    kGUARD_EXC_INVALID_NAME :
		    kGUARD_EXC_INVALID_RIGHT));
		return kr;
	}

	                            

	found = ipc_mqueue_peek(&port->ip_messages, seqnop,
	    msg_sizep, msg_idp, &max_trailer, NULL);
	ip_unlock(port);

	if (found != TRUE) {
		return KERN_FAILURE;
	}

	max_trailer.msgh_seqno = *seqnop;
	memcpy(trailer_infop, &max_trailer, *trailer_sizep);

	return KERN_SUCCESS;
}

  
                                               
           
                                              
              
                   
           
                                      
                                        
                                        
                                                      
                                                          
   

kern_return_t
mach_port_set_mscount(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_port_mscount_t     mscount)
{
	ipc_port_t port;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_RIGHT;
	}

	kr = ipc_port_translate_receive(space, name, &port);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	                               

	port->ip_mscount = mscount;
	ip_unlock(port);
	return KERN_SUCCESS;
}

  
                                             
           
                                              
              
                   
           
                                      
                                        
                                        
                                                      
                                                          
   

kern_return_t
mach_port_set_seqno(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_port_seqno_t       seqno)
{
	ipc_port_t port;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_RIGHT;
	}

	kr = ipc_port_translate_receive(space, name, &port);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	                               

	ipc_mqueue_set_seqno(&port->ip_messages, seqno);

	ip_unlock(port);
	return KERN_SUCCESS;
}

  
                                               
           
                                              
              
                   
           
                                      
                                        
                                        
                                                      
                                                          
   

kern_return_t
mach_port_get_context(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_vm_address_t       *context)
{
	ipc_port_t port;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_RIGHT;
	}

	kr = ipc_port_translate_receive(space, name, &port);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	                            

	                                                                            
	if (port->ip_strict_guard) {
		*context = 0;
	} else {
		*context = port->ip_context;
	}

	ip_unlock(port);
	return KERN_SUCCESS;
}

kern_return_t
mach_port_get_context_from_user(
	mach_port_t             port,
	mach_port_name_t        name,
	mach_vm_address_t       *context)
{
	kern_return_t kr;

	ipc_space_t space = convert_port_to_space_check_type(port, NULL, TASK_FLAVOR_READ, FALSE);

	if (space == IPC_SPACE_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	kr = mach_port_get_context(space, name, context);

	ipc_space_release(space);
	return kr;
}

  
                                               
           
                                              
              
                   
           
                                      
                                        
                                        
                                                      
                                                          
   

kern_return_t
mach_port_set_context(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_vm_address_t       context)
{
	ipc_port_t port;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_RIGHT;
	}

	kr = ipc_port_translate_receive(space, name, &port);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	                               
	if (port->ip_strict_guard) {
		uint64_t portguard = port->ip_context;
		ip_unlock(port);
		                                                                               
		mach_port_guard_exception(name, context, portguard, kGUARD_EXC_SET_CONTEXT);
		return KERN_INVALID_ARGUMENT;
	}

	port->ip_context = context;

	ip_unlock(port);
	return KERN_SUCCESS;
}


  
                                                  
           
                                              
                                                           
              
                   
           
                                            
                                        
                                        
                                                      
                                                      
                                                    
   

kern_return_t
mach_port_get_set_status(
	ipc_space_t                     space,
	mach_port_name_t                name,
	mach_port_name_t                **members,
	mach_msg_type_number_t          *membersCnt)
{
	ipc_entry_num_t actual;                                
	ipc_entry_num_t maxnames;                                        
	kern_return_t kr;

	vm_size_t size;                                       
	vm_offset_t addr;                             
	vm_map_copy_t memory;                         

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_RIGHT;
	}

	size = VM_MAP_PAGE_SIZE(ipc_kernel_map);                           

	for (;;) {
		mach_port_name_t *names;
		ipc_object_t psobj;
		ipc_pset_t pset;

		kr = vm_allocate_kernel(ipc_kernel_map, &addr, size, VM_FLAGS_ANYWHERE, VM_KERN_MEMORY_IPC);
		if (kr != KERN_SUCCESS) {
			return KERN_RESOURCE_SHORTAGE;
		}

		                                     

		kr = vm_map_wire_kernel(ipc_kernel_map, addr, addr + size,
		    VM_PROT_READ | VM_PROT_WRITE, VM_KERN_MEMORY_IPC, FALSE);
		assert(kr == KERN_SUCCESS);

		kr = ipc_object_translate(space, name, MACH_PORT_RIGHT_PORT_SET, &psobj);
		if (kr != KERN_SUCCESS) {
			kmem_free(ipc_kernel_map, addr, size);
			return kr;
		}

		                                                   
		pset = ips_object_to_pset(psobj);
		ips_reference(pset);
		ips_unlock(pset);

		names = (mach_port_name_t *) addr;
		maxnames = (ipc_entry_num_t)(size / sizeof(mach_port_name_t));

		ipc_mqueue_set_gather_member_names(space, &pset->ips_messages, maxnames, names, &actual);

		                                   
		ips_release(pset);

		if (actual <= maxnames) {
			break;
		}

		                                              
		kmem_free(ipc_kernel_map, addr, size);
		size = vm_map_round_page(
			(actual * sizeof(mach_port_name_t)),
			VM_MAP_PAGE_MASK(ipc_kernel_map)) +
		    VM_MAP_PAGE_SIZE(ipc_kernel_map);
	}

	if (actual == 0) {
		memory = VM_MAP_COPY_NULL;

		kmem_free(ipc_kernel_map, addr, size);
	} else {
		vm_size_t size_used;
		vm_size_t vm_size_used;

		size_used = actual * sizeof(mach_port_name_t);
		vm_size_used = vm_map_round_page(
			size_used,
			VM_MAP_PAGE_MASK(ipc_kernel_map));

		  
                                              
                                             
     

		kr = vm_map_unwire(
			ipc_kernel_map,
			vm_map_trunc_page(addr,
			VM_MAP_PAGE_MASK(ipc_kernel_map)),
			vm_map_round_page(addr + vm_size_used,
			VM_MAP_PAGE_MASK(ipc_kernel_map)),
			FALSE);
		assert(kr == KERN_SUCCESS);

		kr = vm_map_copyin(ipc_kernel_map, (vm_map_address_t)addr,
		    (vm_map_size_t)size_used, TRUE, &memory);
		assert(kr == KERN_SUCCESS);

		if (vm_size_used != size) {
			kmem_free(ipc_kernel_map,
			    addr + vm_size_used, size - vm_size_used);
		}
	}

	*members = (mach_port_name_t *) memory;
	*membersCnt = actual;
	return KERN_SUCCESS;
}

kern_return_t
mach_port_get_set_status_from_user(
	mach_port_t                     port,
	mach_port_name_t                name,
	mach_port_name_t                **members,
	mach_msg_type_number_t          *membersCnt)
{
	kern_return_t kr;

	ipc_space_t space = convert_port_to_space_check_type(port, NULL, TASK_FLAVOR_READ, FALSE);

	if (space == IPC_SPACE_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	kr = mach_port_get_set_status(space, name, members, membersCnt);

	ipc_space_release(space);
	return kr;
}

  
                                               
           
                                              
                                                
                                             
                           
              
                   
           
                                 
                                        
                                        
                                                   
                                                            
                                                  
                                                            
                   
                                                            
   

kern_return_t
mach_port_move_member(
	ipc_space_t             space,
	mach_port_name_t        member,
	mach_port_name_t        after)
{
	ipc_object_t port_obj, ps_obj;
	ipc_port_t port;
	ipc_pset_t nset = IPS_NULL;
	kern_return_t kr;
	uint64_t wq_link_id = 0;
	uint64_t wq_reserved_prepost = 0;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(member)) {
		return KERN_INVALID_RIGHT;
	}

	if (after == MACH_PORT_DEAD) {
		return KERN_INVALID_RIGHT;
	} else if (after == MACH_PORT_NULL) {
		wq_link_id = 0;
	} else {
		  
                                
                                       
                                             
                                            
                                          
                                             
              
     
		wq_link_id = waitq_link_reserve(NULL);
		wq_reserved_prepost = waitq_prepost_reserve(NULL, 10,
		    WAITQ_DONT_LOCK);
		kr = ipc_pset_lazy_allocate(space, after);
		if (kr != KERN_SUCCESS) {
			goto done;
		}
	}

	if (after != MACH_PORT_NULL) {
		kr = ipc_object_translate_two(space,
		    member, MACH_PORT_RIGHT_RECEIVE, &port_obj,
		    after, MACH_PORT_RIGHT_PORT_SET, &ps_obj);
	} else {
		kr = ipc_object_translate(space,
		    member, MACH_PORT_RIGHT_RECEIVE, &port_obj);
	}
	if (kr != KERN_SUCCESS) {
		goto done;
	}

	port = ip_object_to_port(port_obj);
	if (after != MACH_PORT_NULL) {
		nset = ips_object_to_pset(ps_obj);
	}
	                              

	ipc_pset_remove_from_all(port);

	if (after != MACH_PORT_NULL) {
		kr = ipc_pset_add(nset, port, &wq_link_id, &wq_reserved_prepost);
		ips_unlock(nset);
	}

	ip_unlock(port);

done:
	  
                                                             
                                                                       
    
	waitq_link_release(wq_link_id);
	waitq_prepost_release_reserve(wq_reserved_prepost);

	return kr;
}

  
                                                        
           
                                                 
                                                  
                                                  
                                             
  
                              
                                           
                                               
                          
                                             
                                              
                                               
                                                   
                                              
                               
                         
                                                 
                                                
                                                 
                                                   
                               
              
                   
           
                                           
                                        
                                        
                                    
                                                  
                                                             
                                                    
                              
                                       
                                                      
                         
                                                    
                                                     
                                       
                                                   
                                                      
   

kern_return_t
mach_port_request_notification(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_msg_id_t           id,
	mach_port_mscount_t     sync,
	ipc_port_t              notify,
	ipc_port_t              *previousp)
{
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (notify == IP_DEAD) {
		return KERN_INVALID_CAPABILITY;
	}

#if     NOTYET
	  
                                                      
    
	{
		ipc_port_t port;
		ipc_entry_t entry;

		kr = ipc_right_lookup_write(space, name, &entry);
		if (kr != KERN_SUCCESS) {
			return kr;
		}

		port = ip_object_to_port(entry->ie_object);

		if (port->ip_subsystem != NULL) {
			is_write_unlock(space);
			panic("mach_port_request_notification: on RPC port!!");
			return KERN_INVALID_CAPABILITY;
		}
		is_write_unlock(space);
	}
#endif              


	switch (id) {
	case MACH_NOTIFY_PORT_DESTROYED: {
		ipc_port_t port;

		if (sync != 0) {
			return KERN_INVALID_VALUE;
		}

		if (!MACH_PORT_VALID(name)) {
			return KERN_INVALID_RIGHT;
		}

		kr = ipc_port_translate_receive(space, name, &port);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		                               

		  
                                                                   
                                  
     
		if (ip_is_kobject(port) || ip_is_kolabeled(port) ||
		    port->ip_specialreply) {
			ip_unlock(port);
			return KERN_INVALID_RIGHT;
		}

		                                                       
		if (port->ip_pdrequest != IP_NULL) {
			ip_unlock(port);
			mach_port_guard_exception(name, 0, 0, kGUARD_EXC_KERN_FAILURE);
			return KERN_FAILURE;
		}

		ipc_port_pdrequest(port, notify, previousp);
		                      
		assert(*previousp == IP_NULL);
		break;
	}

	case MACH_NOTIFY_NO_SENDERS: {
		ipc_port_t port;

		if (!MACH_PORT_VALID(name)) {
			return KERN_INVALID_RIGHT;
		}

		kr = ipc_port_translate_receive(space, name, &port);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		                               

		ipc_port_nsrequest(port, sync, notify, previousp);
		                      
		break;
	}

	case MACH_NOTIFY_SEND_POSSIBLE:

		if (!MACH_PORT_VALID(name)) {
			return KERN_INVALID_ARGUMENT;
		}

		kr = ipc_right_request_alloc(space, name, sync != 0,
		    TRUE, notify, previousp);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		break;

	case MACH_NOTIFY_DEAD_NAME:

		if (!MACH_PORT_VALID(name)) {
			  
                   
                                          
                                      
      
			return KERN_INVALID_ARGUMENT;
		}

		kr = ipc_right_request_alloc(space, name, sync != 0,
		    FALSE, notify, previousp);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		break;

	default:
		return KERN_INVALID_VALUE;
	}

	return KERN_SUCCESS;
}

  
                                                
           
                                                 
                                                
                                                  
              
                   
           
                                     
                                        
                                        
                                                   
                                                      
                                                          
                                                 
                                                      
                                                          
                                                    
   

kern_return_t
mach_port_insert_right(
	ipc_space_t                     space,
	mach_port_name_t                name,
	ipc_port_t                      poly,
	mach_msg_type_name_t            polyPoly)
{
	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name) ||
	    !MACH_MSG_TYPE_PORT_ANY_RIGHT(polyPoly)) {
		return KERN_INVALID_VALUE;
	}

	if (!IP_VALID(poly)) {
		return KERN_INVALID_CAPABILITY;
	}

	return ipc_object_copyout_name(space, ip_to_object(poly),
	           polyPoly, name);
}

  
                                                 
           
                                                  
                                             
              
                   
           
                                      
                                        
                                        
                                                         
                                                  
                                                             
   

kern_return_t
mach_port_extract_right(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_msg_type_name_t    msgt_name,
	ipc_port_t              *poly,
	mach_msg_type_name_t    *polyPoly)
{
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_MSG_TYPE_PORT_ANY(msgt_name)) {
		return KERN_INVALID_VALUE;
	}

	if (!MACH_PORT_VALID(name)) {
		  
                                                           
                                                        
                   
     
		return KERN_INVALID_RIGHT;
	}

	kr = ipc_object_copyin(space, name, msgt_name, (ipc_object_t *) poly, 0, NULL,
	    (space == current_space() && msgt_name == MACH_MSG_TYPE_COPY_SEND) ?
	    IPC_OBJECT_COPYIN_FLAGS_ALLOW_IMMOVABLE_SEND : IPC_OBJECT_COPYIN_FLAGS_SOFT_FAIL_IMMOVABLE_SEND);

	if (kr == KERN_SUCCESS) {
		*polyPoly = ipc_object_copyin_type(msgt_name);
	}
	return kr;
}

  
                                                
           
                                                 
                     
              
                           
           
         
   
static void
mach_port_get_status_helper(
	ipc_port_t              port,
	mach_port_status_t      *statusp)
{
	imq_lock(&port->ip_messages);
	                                                                      
	statusp->mps_pset = !!(port->ip_in_pset);
	statusp->mps_seqno = port->ip_messages.imq_seqno;
	statusp->mps_qlimit = port->ip_messages.imq_qlimit;
	statusp->mps_msgcount = port->ip_messages.imq_msgcount;
	imq_unlock(&port->ip_messages);

	statusp->mps_mscount = port->ip_mscount;
	statusp->mps_sorights = port->ip_sorights;
	statusp->mps_srights = port->ip_srights > 0;
	statusp->mps_pdrequest = port->ip_pdrequest != IP_NULL;
	statusp->mps_nsrequest = port->ip_nsrequest != IP_NULL;
	statusp->mps_flags = 0;
	if (port->ip_impdonation) {
		statusp->mps_flags |= MACH_PORT_STATUS_FLAG_IMP_DONATION;
		if (port->ip_tempowner) {
			statusp->mps_flags |= MACH_PORT_STATUS_FLAG_TEMPOWNER;
			if (IIT_NULL != port->ip_imp_task) {
				statusp->mps_flags |= MACH_PORT_STATUS_FLAG_TASKPTR;
			}
		}
	}
	if (port->ip_guarded) {
		statusp->mps_flags |= MACH_PORT_STATUS_FLAG_GUARDED;
		if (port->ip_strict_guard) {
			statusp->mps_flags |= MACH_PORT_STATUS_FLAG_STRICT_GUARD;
		}
		if (port->ip_immovable_receive) {
			statusp->mps_flags |= MACH_PORT_STATUS_FLAG_GUARD_IMMOVABLE_RECEIVE;
		}
	}
	if (port->ip_no_grant) {
		statusp->mps_flags |= MACH_PORT_STATUS_FLAG_NO_GRANT;
	}
	return;
}

kern_return_t
mach_port_get_attributes(
	ipc_space_t             space,
	mach_port_name_t        name,
	int                     flavor,
	mach_port_info_t        info,
	mach_msg_type_number_t  *count)
{
	ipc_port_t port;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	switch (flavor) {
	case MACH_PORT_LIMITS_INFO: {
		mach_port_limits_t *lp = (mach_port_limits_t *)info;

		if (*count < MACH_PORT_LIMITS_INFO_COUNT) {
			return KERN_FAILURE;
		}

		if (!MACH_PORT_VALID(name)) {
			*count = 0;
			break;
		}

		kr = ipc_port_translate_receive(space, name, &port);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		                               

		lp->mpl_qlimit = port->ip_messages.imq_qlimit;
		*count = MACH_PORT_LIMITS_INFO_COUNT;
		ip_unlock(port);
		break;
	}

	case MACH_PORT_RECEIVE_STATUS: {
		mach_port_status_t *statusp = (mach_port_status_t *)info;

		if (*count < MACH_PORT_RECEIVE_STATUS_COUNT) {
			return KERN_FAILURE;
		}

		if (!MACH_PORT_VALID(name)) {
			return KERN_INVALID_RIGHT;
		}

		kr = ipc_port_translate_receive(space, name, &port);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		                               
		mach_port_get_status_helper(port, statusp);
		*count = MACH_PORT_RECEIVE_STATUS_COUNT;
		ip_unlock(port);
		break;
	}

	case MACH_PORT_DNREQUESTS_SIZE: {
		ipc_port_request_t      table;

		if (*count < MACH_PORT_DNREQUESTS_SIZE_COUNT) {
			return KERN_FAILURE;
		}

		if (!MACH_PORT_VALID(name)) {
			*(int *)info = 0;
			break;
		}

		kr = ipc_port_translate_receive(space, name, &port);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		                               

		table = port->ip_requests;
		if (table == IPR_NULL) {
			*(int *)info = 0;
		} else {
			*(int *)info = table->ipr_size->its_size;
		}
		*count = MACH_PORT_DNREQUESTS_SIZE_COUNT;
		ip_unlock(port);
		break;
	}

	case MACH_PORT_INFO_EXT: {
		mach_port_info_ext_t *mp_info = (mach_port_info_ext_t *)info;
		if (*count < MACH_PORT_INFO_EXT_COUNT) {
			return KERN_FAILURE;
		}

		if (!MACH_PORT_VALID(name)) {
			return KERN_INVALID_RIGHT;
		}

		kr = ipc_port_translate_receive(space, name, &port);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		                               
		mach_port_get_status_helper(port, &mp_info->mpie_status);
		mp_info->mpie_boost_cnt = port->ip_impcount;
		*count = MACH_PORT_INFO_EXT_COUNT;
		ip_unlock(port);
		break;
	}

	default:
		return KERN_INVALID_ARGUMENT;
		              
	}

	return KERN_SUCCESS;
}

kern_return_t
mach_port_get_attributes_from_user(
	mach_port_t             port,
	mach_port_name_t        name,
	int                     flavor,
	mach_port_info_t        info,
	mach_msg_type_number_t  *count)
{
	kern_return_t kr;

	ipc_space_t space = convert_port_to_space_check_type(port, NULL, TASK_FLAVOR_READ, FALSE);

	if (space == IPC_SPACE_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	kr = mach_port_get_attributes(space, name, flavor, info, count);

	ipc_space_release(space);
	return kr;
}

kern_return_t
mach_port_set_attributes(
	ipc_space_t             space,
	mach_port_name_t        name,
	int                     flavor,
	mach_port_info_t        info,
	mach_msg_type_number_t  count)
{
	ipc_port_t port;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	switch (flavor) {
	case MACH_PORT_LIMITS_INFO: {
		mach_port_limits_t *mplp = (mach_port_limits_t *)info;

		if (count < MACH_PORT_LIMITS_INFO_COUNT) {
			return KERN_FAILURE;
		}

		if (mplp->mpl_qlimit > MACH_PORT_QLIMIT_MAX) {
			return KERN_INVALID_VALUE;
		}

		if (!MACH_PORT_VALID(name)) {
			return KERN_INVALID_RIGHT;
		}

		kr = ipc_port_translate_receive(space, name, &port);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		                               

		ipc_mqueue_set_qlimit(&port->ip_messages, mplp->mpl_qlimit);
		ip_unlock(port);
		break;
	}
	case MACH_PORT_DNREQUESTS_SIZE: {
		if (count < MACH_PORT_DNREQUESTS_SIZE_COUNT) {
			return KERN_FAILURE;
		}

		if (!MACH_PORT_VALID(name)) {
			return KERN_INVALID_RIGHT;
		}

		kr = ipc_port_translate_receive(space, name, &port);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		                               

		kr = ipc_port_request_grow(port, *(int *)info);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		break;
	}
	case MACH_PORT_TEMPOWNER:
		if (!MACH_PORT_VALID(name)) {
			return KERN_INVALID_RIGHT;
		}

		ipc_importance_task_t release_imp_task = IIT_NULL;
		natural_t assertcnt = 0;

		kr = ipc_port_translate_receive(space, name, &port);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
		                               

		  
                                                       
                                                                      
                                
     
		if (ip_is_kobject(port) || port->ip_specialreply) {
			ip_unlock(port);
			return KERN_INVALID_ARGUMENT;
		}

		if (port->ip_tempowner != 0) {
			if (IIT_NULL != port->ip_imp_task) {
				release_imp_task = port->ip_imp_task;
				port->ip_imp_task = IIT_NULL;
				assertcnt = port->ip_impcount;
			}
		} else {
			assertcnt = port->ip_impcount;
		}

		port->ip_impdonation = 1;
		port->ip_tempowner = 1;
		ip_unlock(port);

#if IMPORTANCE_INHERITANCE
		                                                    
		if (release_imp_task != IIT_NULL) {
			assert(ipc_importance_task_is_any_receiver_type(release_imp_task));
			if (assertcnt > 0) {
				ipc_importance_task_drop_internal_assertion(release_imp_task, assertcnt);
			}
			ipc_importance_task_release(release_imp_task);
		} else if (assertcnt > 0) {
			release_imp_task = current_task()->task_imp_base;
			if (release_imp_task != IIT_NULL &&
			    ipc_importance_task_is_any_receiver_type(release_imp_task)) {
				ipc_importance_task_drop_internal_assertion(release_imp_task, assertcnt);
			}
		}
#else
		if (release_imp_task != IIT_NULL) {
			ipc_importance_task_release(release_imp_task);
		}
#endif                             

		break;

#if IMPORTANCE_INHERITANCE
	case MACH_PORT_DENAP_RECEIVER:
	case MACH_PORT_IMPORTANCE_RECEIVER:
		if (!MACH_PORT_VALID(name)) {
			return KERN_INVALID_RIGHT;
		}

		kr = ipc_port_translate_receive(space, name, &port);
		if (kr != KERN_SUCCESS) {
			return kr;
		}

		  
                                                       
                                                           
                                
     
		if (ip_is_kobject(port) || port->ip_specialreply) {
			ip_unlock(port);
			return KERN_INVALID_ARGUMENT;
		}

		                               
		port->ip_impdonation = 1;
		ip_unlock(port);

		break;
#endif                             

	default:
		return KERN_INVALID_ARGUMENT;
		              
	}
	return KERN_SUCCESS;
}

  
                                                 
           
                                                
              
                                                   
              
                   
           
                                 
                                        
                                        
                                                 
                                                          
                                                      
                                                               
                                                          
   

kern_return_t
mach_port_insert_member(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_port_name_t        psname)
{
	ipc_object_t obj;
	ipc_object_t psobj;
	kern_return_t kr;
	uint64_t wq_link_id;
	uint64_t wq_reserved_prepost;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name) || !MACH_PORT_VALID(psname)) {
		return KERN_INVALID_RIGHT;
	}

	wq_link_id = waitq_link_reserve(NULL);
	wq_reserved_prepost = waitq_prepost_reserve(NULL, 10,
	    WAITQ_DONT_LOCK);
	kr = ipc_pset_lazy_allocate(space, psname);
	if (kr != KERN_SUCCESS) {
		goto done;
	}


	kr = ipc_object_translate_two(space,
	    name, MACH_PORT_RIGHT_RECEIVE, &obj,
	    psname, MACH_PORT_RIGHT_PORT_SET, &psobj);
	if (kr != KERN_SUCCESS) {
		goto done;
	}

	                                                              
	assert(psobj != IO_NULL);
	assert(obj != IO_NULL);

	kr = ipc_pset_add(ips_object_to_pset(psobj), ip_object_to_port(obj),
	    &wq_link_id, &wq_reserved_prepost);

	io_unlock(psobj);
	io_unlock(obj);

done:
	                                                                  
	waitq_link_release(wq_link_id);
	waitq_prepost_release_reserve(wq_reserved_prepost);

	return kr;
}

  
                                                  
           
                                                          
              
                   
           
                                 
                                        
                                        
                                                   
                                                            
                                                  
                                                            
                   
                                                            
   

kern_return_t
mach_port_extract_member(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_port_name_t        psname)
{
	ipc_object_t psobj;
	ipc_object_t obj;
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name) || !MACH_PORT_VALID(psname)) {
		return KERN_INVALID_RIGHT;
	}

	kr = ipc_object_translate_two(space,
	    name, MACH_PORT_RIGHT_RECEIVE, &obj,
	    psname, MACH_PORT_RIGHT_PORT_SET, &psobj);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	                                                                   
	assert(psobj != IO_NULL);
	assert(obj != IO_NULL);

	kr = ipc_pset_remove(ips_object_to_pset(psobj), ip_object_to_port(obj));

	io_unlock(psobj);
	io_unlock(obj);

	return kr;
}

  
                       
  
                                                 
   
kern_return_t
task_set_port_space(
	ipc_space_t     space,
	int             table_entries)
{
	kern_return_t kr;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	is_write_lock(space);

	if (!is_active(space)) {
		is_write_unlock(space);
		return KERN_INVALID_TASK;
	}

	kr = ipc_entry_grow_table(space, table_entries);
	if (kr == KERN_SUCCESS) {
		is_write_unlock(space);
	}
	return kr;
}

  
                                                   
           
                                       
              
                
           
                               
                                                                
   
static kern_return_t
mach_port_guard_locked(
	ipc_port_t              port,
	uint64_t                guard,
	uint64_t                flags)
{
	if (port->ip_context) {
		return KERN_INVALID_ARGUMENT;
	}

	int strict = (flags & MPG_STRICT)? 1 : 0;
	int immovable_receive = (flags & MPG_IMMOVABLE_RECEIVE)? 1 : 0;

	imq_lock(&port->ip_messages);
	port->ip_context = guard;
	port->ip_guarded = 1;
	port->ip_strict_guard = strict;
	                                                                
	if (!port->ip_immovable_receive) {
		port->ip_immovable_receive = immovable_receive;
	}
	imq_unlock(&port->ip_messages);

	return KERN_SUCCESS;
}

  
                                                     
           
                                    
              
                
           
                                 
                                                                             
                                              
   
static kern_return_t
mach_port_unguard_locked(
	ipc_port_t              port,
	mach_port_name_t        name,
	uint64_t                guard)
{
	                            
	if (!port->ip_guarded) {
		                                             
		mach_port_guard_exception(name, guard, 0, kGUARD_EXC_UNGUARDED);
		return KERN_INVALID_ARGUMENT;
	}

	if (port->ip_context != guard) {
		                                      
		mach_port_guard_exception(name, guard, port->ip_context, kGUARD_EXC_INCORRECT_GUARD);
		return KERN_INVALID_ARGUMENT;
	}

	imq_lock(&port->ip_messages);
	port->ip_context = 0;
	port->ip_guarded = port->ip_strict_guard = 0;
	                                              
	imq_unlock(&port->ip_messages);

	return KERN_SUCCESS;
}


  
                                                      
           
                                                                  
                                                  
              
         
           
                                               
   
void
mach_port_guard_exception(
	mach_port_name_t        name,
	__unused uint64_t       inguard,
	uint64_t                        portguard,
	unsigned                        reason)
{
	mach_exception_code_t code = 0;
	EXC_GUARD_ENCODE_TYPE(code, GUARD_TYPE_MACH_PORT);
	EXC_GUARD_ENCODE_FLAVOR(code, reason);
	EXC_GUARD_ENCODE_TARGET(code, name);
	mach_exception_subcode_t subcode = (uint64_t)portguard;
	thread_t t = current_thread();
	boolean_t fatal = FALSE;
	if (t->task->task_exc_guard & TASK_EXC_GUARD_MP_FATAL) {
		fatal = TRUE;
	} else if (reason <= MAX_FATAL_kGUARD_EXC_CODE) {
		fatal = TRUE;
	}
	thread_guard_violation(t, code, subcode, fatal);
}

  
                                                             
  
                                                                       
   
void
mach_port_guard_exception_immovable(
	mach_port_name_t        name,
	mach_port_t             port,
	uint64_t                portguard)
{
	if (ip_is_control(port) && immovable_control_port_enabled) {
		mach_port_guard_exception(name, 0, portguard,
		    ipc_control_port_options & IPC_CONTROL_PORT_OPTIONS_IMMOVABLE_HARD ?
		    kGUARD_EXC_IMMOVABLE : kGUARD_EXC_IMMOVABLE_NON_FATAL);
	} else if (!ip_is_control(port)) {
		                                                           
		mach_port_guard_exception(name, 0, portguard, kGUARD_EXC_IMMOVABLE);
	} else {
		                                                            
		panic("mach_port_guard_exception_immovable: condition does not hold.");
	}
}


  
                               
           
                                                      
              
         
           
         
   

void
mach_port_guard_ast(thread_t t,
    mach_exception_data_type_t code, mach_exception_data_type_t subcode)
{
	unsigned int reason = EXC_GUARD_DECODE_GUARD_FLAVOR(code);
	task_t task = t->task;
	unsigned int behavior = task->task_exc_guard;
	assert(task == current_task());
	assert(task != kernel_task);

	switch (reason) {
	  
                                                           
    
	case kGUARD_EXC_DESTROY:
	case kGUARD_EXC_MOD_REFS:
	case kGUARD_EXC_SET_CONTEXT:
	case kGUARD_EXC_UNGUARDED:
	case kGUARD_EXC_INCORRECT_GUARD:
	case kGUARD_EXC_IMMOVABLE:
	case kGUARD_EXC_STRICT_REPLY:
	case kGUARD_EXC_MSG_FILTERED:
		task_exception_notify(EXC_GUARD, code, subcode);
		task_bsdtask_kill(task);
		break;

	default:
		  
                                                  
     

		                         
		if ((behavior & TASK_EXC_GUARD_MP_DELIVER) == 0) {
			return;
		}

		                                             
		while (behavior & TASK_EXC_GUARD_MP_ONCE) {
			uint32_t new_behavior = behavior & ~TASK_EXC_GUARD_MP_DELIVER;

			if (OSCompareAndSwap(behavior, new_behavior, &task->task_exc_guard)) {
				break;
			}
			behavior = task->task_exc_guard;
			if ((behavior & TASK_EXC_GUARD_MP_DELIVER) == 0) {
				return;
			}
		}

		                                                      
		if ((task->task_exc_guard & TASK_EXC_GUARD_MP_CORPSE) &&
		    (task->task_exc_guard & TASK_EXC_GUARD_MP_FATAL) == 0) {
			task_violated_guard(code, subcode, NULL);
		} else {
			task_exception_notify(EXC_GUARD, code, subcode);
		}

		                                   
		if (task->task_exc_guard & TASK_EXC_GUARD_MP_FATAL) {
			task_bsdtask_kill(task);
		}
		break;
	}
}

  
                                             
           
                                                            
              
         
           
                                         
                                        
                                        
                                                    
                                                      
                                                  
   

kern_return_t
mach_port_construct(
	ipc_space_t             space,
	mach_port_options_t     *options,
	uint64_t                context,
	mach_port_name_t        *name)
{
	kern_return_t           kr;
	ipc_port_t              port;
	ipc_port_init_flags_t   init_flags = IPC_PORT_INIT_MESSAGE_QUEUE;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (options->flags & MPO_INSERT_SEND_RIGHT) {
		init_flags |= IPC_PORT_INIT_MAKE_SEND_RIGHT;
	}

	if (options->flags & MPO_FILTER_MSG) {
		init_flags |= IPC_PORT_INIT_FILTER_MESSAGE;
	}

	if (options->flags & MPO_TG_BLOCK_TRACKING) {
		                                                                               
		if (proc_get_effective_task_policy(current_task(),
		    TASK_POLICY_ROLE) != TASK_GRAPHICS_SERVER) {
			return KERN_DENIED;
		}

		  
                                                                                      
                                                                                    
                                                                                    
                                   
     
		mach_port_name_t wi_port_name = options->work_interval_port;
		if (work_interval_port_type_render_server(wi_port_name) == false) {
			return KERN_INVALID_ARGUMENT;
		}
		init_flags |= IPC_PORT_INIT_TG_BLOCK_TRACKING;
	}

	                                          
	kr = ipc_port_alloc(space, init_flags, name, &port);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	                            
	if (options->flags & MPO_CONTEXT_AS_GUARD) {
		uint64_t flags = 0;
		if (options->flags & MPO_STRICT) {
			flags |= MPG_STRICT;
		}
		if (options->flags & MPO_IMMOVABLE_RECEIVE) {
			flags |= MPG_IMMOVABLE_RECEIVE;
		}
		kr = mach_port_guard_locked(port, (uint64_t) context, flags);
		                                                                             
		assert(kr == KERN_SUCCESS);
	} else {
		port->ip_context = context;
	}

	                 
	ip_unlock(port);

	                                      

	if (options->flags & MPO_QLIMIT) {
		kr = mach_port_set_attributes(space, *name, MACH_PORT_LIMITS_INFO,
		    (mach_port_info_t)&options->mpl, sizeof(options->mpl) / sizeof(int));
		if (kr != KERN_SUCCESS) {
			goto cleanup;
		}
	}

	if (options->flags & MPO_TEMPOWNER) {
		kr = mach_port_set_attributes(space, *name, MACH_PORT_TEMPOWNER, NULL, 0);
		if (kr != KERN_SUCCESS) {
			goto cleanup;
		}
	}

	if (options->flags & MPO_IMPORTANCE_RECEIVER) {
		kr = mach_port_set_attributes(space, *name, MACH_PORT_IMPORTANCE_RECEIVER, NULL, 0);
		if (kr != KERN_SUCCESS) {
			goto cleanup;
		}
	}

	if (options->flags & MPO_DENAP_RECEIVER) {
		kr = mach_port_set_attributes(space, *name, MACH_PORT_DENAP_RECEIVER, NULL, 0);
		if (kr != KERN_SUCCESS) {
			goto cleanup;
		}
	}

	return KERN_SUCCESS;

cleanup:
	                                                                                        
	(void) mach_port_destruct(space, *name,
	    (options->flags & MPO_INSERT_SEND_RIGHT) ? -1 : 0, context);
	return kr;
}

  
                                            
           
                                               
              
         
           
                                        
                                        
                                        
                                                      
                                               
                                                             
                                                                             
                                              
   

kern_return_t
mach_port_destruct(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_port_delta_t       srdelta,
	uint64_t                guard)
{
	kern_return_t           kr;
	ipc_entry_t             entry;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_NAME;
	}

	                                        
	kr = ipc_right_lookup_write(space, name, &entry);
	if (kr != KERN_SUCCESS) {
		mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_NAME);
		return kr;
	}
	                                      
	kr = ipc_right_destruct(space, name, entry, srdelta, guard);                 

	return kr;
}

  
                                         
           
                                                 
                                                       
              
         
           
                                        
                                        
                                        
                                                      
                                               
                                                                
   
kern_return_t
mach_port_guard(
	ipc_space_t             space,
	mach_port_name_t        name,
	uint64_t                guard,
	boolean_t               strict)
{
	kern_return_t           kr;
	ipc_port_t              port;
	uint64_t flags = 0;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_NAME;
	}

	                                                 
	kr = ipc_port_translate_receive(space, name, &port);
	if (kr != KERN_SUCCESS) {
		mach_port_guard_exception(name, 0, 0,
		    ((KERN_INVALID_NAME == kr) ?
		    kGUARD_EXC_INVALID_NAME :
		    kGUARD_EXC_INVALID_RIGHT));
		return kr;
	}

	                            
	if (strict) {
		flags = MPG_STRICT;
	}

	kr = mach_port_guard_locked(port, guard, flags);
	ip_unlock(port);

	if (KERN_INVALID_ARGUMENT == kr) {
		mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_ARGUMENT);
	}

	return kr;
}

  
                                           
           
                                                   
              
         
           
                                        
                                        
                                        
                                                      
                                               
                                                                             
                                              
   
kern_return_t
mach_port_unguard(
	ipc_space_t             space,
	mach_port_name_t        name,
	uint64_t                guard)
{
	kern_return_t           kr;
	ipc_port_t              port;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_NAME;
	}

	kr = ipc_port_translate_receive(space, name, &port);
	if (kr != KERN_SUCCESS) {
		mach_port_guard_exception(name, 0, 0,
		    ((KERN_INVALID_NAME == kr) ?
		    kGUARD_EXC_INVALID_NAME :
		    kGUARD_EXC_INVALID_RIGHT));
		return kr;
	}

	                            
	kr = mach_port_unguard_locked(port, name, guard);
	ip_unlock(port);

	return kr;
}

  
                                                    
           
                                                                 
                                                       
              
                                           
           
                                        
                                        
                                        
                                                      
                                               
                                                                
                                                                                 
                                                 
   
kern_return_t
mach_port_guard_with_flags(
	ipc_space_t             space,
	mach_port_name_t        name,
	uint64_t                guard,
	uint64_t                flags)
{
	kern_return_t           kr;
	ipc_port_t              port;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_NAME;
	}

	kr = ipc_port_translate_receive(space, name, &port);
	if (kr != KERN_SUCCESS) {
		mach_port_guard_exception(name, 0, 0,
		    ((KERN_INVALID_NAME == kr) ?
		    kGUARD_EXC_INVALID_NAME :
		    kGUARD_EXC_INVALID_RIGHT));
		return kr;
	}

	                            
	kr = mach_port_guard_locked(port, guard, flags);
	ip_unlock(port);

	if (KERN_INVALID_ARGUMENT == kr) {
		mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_ARGUMENT);
	}

	return kr;
}

  
                                              
           
                     
              
                                   
           
                                        
                                        
                                        
                                                      
                                               
                                                                           
                                                
   
kern_return_t
mach_port_swap_guard(
	ipc_space_t             space,
	mach_port_name_t        name,
	uint64_t                old_guard,
	uint64_t                new_guard)
{
	kern_return_t           kr;
	ipc_port_t              port;

	if (space == IS_NULL) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_NAME;
	}

	kr = ipc_port_translate_receive(space, name, &port);
	if (kr != KERN_SUCCESS) {
		mach_port_guard_exception(name, 0, 0,
		    ((KERN_INVALID_NAME == kr) ?
		    kGUARD_EXC_INVALID_NAME :
		    kGUARD_EXC_INVALID_RIGHT));
		return kr;
	}

	                            
	if (!port->ip_guarded) {
		ip_unlock(port);
		mach_port_guard_exception(name, old_guard, 0, kGUARD_EXC_UNGUARDED);
		return KERN_INVALID_ARGUMENT;
	}

	if (port->ip_strict_guard) {
		uint64_t portguard = port->ip_context;
		ip_unlock(port);
		                                                                               
		mach_port_guard_exception(name, old_guard, portguard, kGUARD_EXC_SET_CONTEXT);
		return KERN_INVALID_ARGUMENT;
	}

	if (port->ip_context != old_guard) {
		uint64_t portguard = port->ip_context;
		ip_unlock(port);
		mach_port_guard_exception(name, old_guard, portguard, kGUARD_EXC_INCORRECT_GUARD);
		return KERN_INVALID_ARGUMENT;
	}

	imq_lock(&port->ip_messages);
	port->ip_context = new_guard;
	imq_unlock(&port->ip_messages);

	ip_unlock(port);

	return KERN_SUCCESS;
}
