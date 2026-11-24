/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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
 * @OSF_FREE_COPYRIGHT@
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
  
   
  
                        
                      
             
  
                                            
   

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/port.h>
#include <mach/message.h>
#include <kern/assert.h>
#include <kern/ipc_kobject.h>
#include <kern/misc_protos.h>
#include <ipc/port.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_hash.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_right.h>
#include <ipc/ipc_notify.h>
#include <ipc/ipc_table.h>
#include <ipc/ipc_importance.h>
#include <security/mac_mach_internal.h>

  
                                  
           
                                              
              
                                                              
           
                                 
                                        
                                                  
   

kern_return_t
ipc_right_lookup_write(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             *entryp)
{
	ipc_entry_t entry;

	assert(space != IS_NULL);

	is_write_lock(space);

	if (!is_active(space)) {
		is_write_unlock(space);
		return KERN_INVALID_TASK;
	}

	if ((entry = ipc_entry_lookup(space, name)) == IE_NULL) {
		is_write_unlock(space);
		return KERN_INVALID_NAME;
	}

	*entryp = entry;
	return KERN_SUCCESS;
}

  
                                      
           
                                                    
                                                    
                                 
              
                                                              
           
                                        
                                                  
   

kern_return_t
ipc_right_lookup_two_write(
	ipc_space_t             space,
	mach_port_name_t        name1,
	ipc_entry_t             *entryp1,
	mach_port_name_t        name2,
	ipc_entry_t             *entryp2)
{
	ipc_entry_t entry1;
	ipc_entry_t entry2;

	assert(space != IS_NULL);

	is_write_lock(space);

	if (!is_active(space)) {
		is_write_unlock(space);
		return KERN_INVALID_TASK;
	}

	if ((entry1 = ipc_entry_lookup(space, name1)) == IE_NULL) {
		is_write_unlock(space);
		mach_port_guard_exception(name1, 0, 0, kGUARD_EXC_INVALID_NAME);
		return KERN_INVALID_NAME;
	}
	if ((entry2 = ipc_entry_lookup(space, name2)) == IE_NULL) {
		is_write_unlock(space);
		mach_port_guard_exception(name2, 0, 0, kGUARD_EXC_INVALID_NAME);
		return KERN_INVALID_NAME;
	}
	*entryp1 = entry1;
	*entryp2 = entry2;
	return KERN_SUCCESS;
}

  
                             
           
                                               
                                   
                                             
                      
              
                                                        
                                 
   

bool
ipc_right_reverse(
	ipc_space_t             space,
	ipc_object_t            object,
	mach_port_name_t        *namep,
	ipc_entry_t             *entryp)
{
	ipc_port_t port;
	mach_port_name_t name;
	ipc_entry_t entry;

	                                                                 

	assert(is_active(space));
	assert(io_otype(object) == IOT_PORT);

	port = ip_object_to_port(object);
	require_ip_active(port);

	ip_lock_held(port);

	if (port->ip_receiver == space) {
		name = port->ip_receiver_name;
		assert(name != MACH_PORT_NULL);

		entry = ipc_entry_lookup(space, name);

		assert(entry != IE_NULL);
		assert(entry->ie_bits & MACH_PORT_TYPE_RECEIVE);
		assert(port == ip_object_to_port(entry->ie_object));

		*namep = name;
		*entryp = entry;
		return true;
	}

	if (ipc_hash_lookup(space, ip_to_object(port), namep, entryp)) {
		assert((entry = *entryp) != IE_NULL);
		assert(IE_BITS_TYPE(entry->ie_bits) == MACH_PORT_TYPE_SEND);
		assert(port == ip_object_to_port(entry->ie_object));

		return true;
	}

	return false;
}

  
                               
           
                                                      
                                                      
                                                   
  
              
                                         
                                             
           
                                                  
                                        
                                                  
                                                            
                                                     
                                             
                                                    
   

kern_return_t
ipc_right_request_alloc(
	ipc_space_t             space,
	mach_port_name_t        name,
	boolean_t               immediate,
	boolean_t               send_possible,
	ipc_port_t              notify,
	ipc_port_t              *previousp)
{
	ipc_port_request_index_t prev_request;
	ipc_port_t previous = IP_NULL;
	ipc_entry_t entry;
	kern_return_t kr;

#if IMPORTANCE_INHERITANCE
	boolean_t needboost = FALSE;
#endif                             

	for (;;) {
		ipc_port_t port = IP_NULL;

		kr = ipc_right_lookup_write(space, name, &entry);
		if (kr != KERN_SUCCESS) {
			return kr;
		}

		                                      

		prev_request = entry->ie_request;

		                                          
		if (notify == IP_NULL && prev_request == IE_REQ_NONE) {
			is_write_unlock(space);
			*previousp = IP_NULL;
			return KERN_SUCCESS;
		}

		                                                     
		if (entry->ie_bits & MACH_PORT_TYPE_PORT_RIGHTS) {
			ipc_port_request_index_t new_request;

			port = ip_object_to_port(entry->ie_object);
			assert(port != IP_NULL);

			if (!ipc_right_check(space, port, name, entry, IPC_OBJECT_COPYIN_FLAGS_NONE)) {
				                               

				                                             
				if (notify == IP_NULL) {
					if (prev_request != IE_REQ_NONE) {
						previous = ipc_port_request_cancel(port, name, prev_request);
					}
					ip_unlock(port);
					entry->ie_request = IE_REQ_NONE;
					ipc_entry_modified(space, name, entry);
					is_write_unlock(space);
					break;
				}

				  
                                                                  
                                                 
       
				if (send_possible && immediate &&
				    ((entry->ie_bits & MACH_PORT_TYPE_SEND_ONCE) ||
				    port->ip_receiver == ipc_space_kernel || !ip_full(port))) {
					if (prev_request != IE_REQ_NONE) {
						previous = ipc_port_request_cancel(port, name, prev_request);
					}
					ip_unlock(port);
					entry->ie_request = IE_REQ_NONE;
					ipc_entry_modified(space, name, entry);
					is_write_unlock(space);

					ipc_notify_send_possible(notify, name);
					break;
				}

				  
                                                               
                                                            
       
				if (prev_request != IE_REQ_NONE) {
					previous = ipc_port_request_cancel(port, name, prev_request);
				}

#if IMPORTANCE_INHERITANCE
				kr = ipc_port_request_alloc(port, name, notify,
				    send_possible, immediate,
				    &new_request, &needboost);
#else
				kr = ipc_port_request_alloc(port, name, notify,
				    send_possible, immediate,
				    &new_request);
#endif                             
				if (kr != KERN_SUCCESS) {
					assert(previous == IP_NULL);
					is_write_unlock(space);

					kr = ipc_port_request_grow(port, ITS_SIZE_NONE);
					                      

					if (kr != KERN_SUCCESS) {
						return kr;
					}

					continue;
				}


				assert(new_request != IE_REQ_NONE);
				entry->ie_request = new_request;
				ipc_entry_modified(space, name, entry);
				is_write_unlock(space);

#if IMPORTANCE_INHERITANCE
				if (needboost == TRUE) {
					if (ipc_port_importance_delta(port, IPID_OPTION_SENDPOSSIBLE, 1) == FALSE) {
						ip_unlock(port);
					}
				} else
#endif                             
				ip_unlock(port);

				break;
			}
			                                                              
		}

		                                                                
		if ((send_possible || immediate) && notify != IP_NULL &&
		    (entry->ie_bits & MACH_PORT_TYPE_DEAD_NAME)) {
			mach_port_urefs_t urefs = IE_BITS_UREFS(entry->ie_bits);

			assert(urefs > 0);

			                                                    
			if (urefs < MACH_PORT_UREFS_MAX) {
				(entry->ie_bits)++;                      
			}
			ipc_entry_modified(space, name, entry);

			is_write_unlock(space);

			if (port != IP_NULL) {
				ip_release(port);
			}

			ipc_notify_dead_name(notify, name);
			previous = IP_NULL;
			break;
		}

		kr = (entry->ie_bits & MACH_PORT_TYPE_PORT_OR_DEAD) ?
		    KERN_INVALID_ARGUMENT : KERN_INVALID_RIGHT;

		is_write_unlock(space);

		if (port != IP_NULL) {
			ip_release(port);
		}

		return kr;
	}

	*previousp = previous;
	return KERN_SUCCESS;
}

  
                                    
           
                                                                 
                                       
              
                                                            
                                                          
   

ipc_port_t
ipc_right_request_cancel(
	__unused ipc_space_t            space,
	ipc_port_t                      port,
	mach_port_name_t                name,
	ipc_entry_t                     entry)
{
	ipc_port_t previous;

	require_ip_active(port);
	assert(port == ip_object_to_port(entry->ie_object));

	if (entry->ie_request == IE_REQ_NONE) {
		return IP_NULL;
	}

	previous = ipc_port_request_cancel(port, name, entry->ie_request);
	entry->ie_request = IE_REQ_NONE;
	ipc_entry_modified(space, name, entry);
	return previous;
}

  
                           
           
                                    
                          
              
                                         
   

bool
ipc_right_inuse(
	ipc_entry_t entry)
{
	return IE_BITS_TYPE(entry->ie_bits) != MACH_PORT_TYPE_NONE;
}

  
                           
           
                                           
                                                                       
                                                           
                                       
              
                                                      
                                              
                                                 
  
                                                       
                            
   

boolean_t
ipc_right_check(
	ipc_space_t              space,
	ipc_port_t               port,
	mach_port_name_t         name,
	ipc_entry_t              entry,
	ipc_object_copyin_flags_t flags)
{
	ipc_entry_bits_t bits;

	assert(is_active(space));
	assert(port == ip_object_to_port(entry->ie_object));

	ip_lock(port);
	if (ip_active(port) ||
	    ((flags & IPC_OBJECT_COPYIN_FLAGS_ALLOW_DEAD_SEND_ONCE) &&
	    entry->ie_request == IE_REQ_NONE &&
	    (entry->ie_bits & MACH_PORT_TYPE_SEND_ONCE))) {
		return FALSE;
	}

	                                                            

	bits = entry->ie_bits;
	assert((bits & MACH_PORT_TYPE_RECEIVE) == 0);
	assert(IE_BITS_UREFS(bits) > 0);

	if (bits & MACH_PORT_TYPE_SEND) {
		assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND);
		assert(IE_BITS_UREFS(bits) > 0);
		assert(port->ip_srights > 0);
		port->ip_srights--;
	} else {
		assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND_ONCE);
		assert(IE_BITS_UREFS(bits) == 1);
		assert(port->ip_sorights > 0);
		port->ip_sorights--;
	}
	ip_unlock(port);

	  
                                     
    

	if ((bits & MACH_PORT_TYPE_SEND) != 0) {
		ipc_hash_delete(space, ip_to_object(port), name, entry);
	}

	                                
	bits = (bits & ~IE_BITS_TYPE_MASK) | MACH_PORT_TYPE_DEAD_NAME;

	  
                                                           
                                                   
                                                      
   
                                                       
                                                       
                                                         
                                                        
                                                           
                                                      
                                         
   
                                                         
                
    
	if (entry->ie_request != IE_REQ_NONE) {
		if (ipc_port_request_type(port, name, entry->ie_request) != 0) {
			                                                            
			if (IE_BITS_UREFS(bits) < MACH_PORT_UREFS_MAX) {
				bits++;                      
			}
		}
		entry->ie_request = IE_REQ_NONE;
	}
	entry->ie_bits = bits;
	entry->ie_object = IO_NULL;
	ipc_entry_modified(space, name, entry);
	return TRUE;
}

  
                               
           
                                             
                                          
                             
              
                                   
   

void
ipc_right_terminate(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             entry)
{
	ipc_entry_bits_t bits;
	mach_port_type_t type;

	bits = entry->ie_bits;
	type = IE_BITS_TYPE(bits);

	assert(!is_active(space));

	  
                                                       
                                                          
                                                     
                                                          
                                                 
                                                       
    

	switch (type) {
	case MACH_PORT_TYPE_DEAD_NAME:
		assert(entry->ie_request == IE_REQ_NONE);
		assert(entry->ie_object == IO_NULL);
		break;

	case MACH_PORT_TYPE_PORT_SET: {
		ipc_pset_t pset = ips_object_to_pset(entry->ie_object);

		assert(entry->ie_request == IE_REQ_NONE);
		assert(pset != IPS_NULL);

		ips_lock(pset);
		assert(ips_active(pset));
		ipc_pset_destroy(space, pset);                            
		break;
	}

	case MACH_PORT_TYPE_SEND:
	case MACH_PORT_TYPE_RECEIVE:
	case MACH_PORT_TYPE_SEND_RECEIVE:
	case MACH_PORT_TYPE_SEND_ONCE: {
		ipc_port_t port = ip_object_to_port(entry->ie_object);
		ipc_port_t request;
		ipc_port_t nsrequest = IP_NULL;
		mach_port_mscount_t mscount = 0;

		assert(port != IP_NULL);
		ip_lock(port);

		if (!ip_active(port)) {
			ip_unlock(port);
			ip_release(port);
			break;
		}

		request = ipc_right_request_cancel_macro(space, port,
		    name, entry);

		if (type & MACH_PORT_TYPE_SEND) {
			assert(port->ip_srights > 0);
			if (--port->ip_srights == 0
			    ) {
				nsrequest = port->ip_nsrequest;
				if (nsrequest != IP_NULL) {
					port->ip_nsrequest = IP_NULL;
					mscount = port->ip_mscount;
				}
			}
		}

		if (type & MACH_PORT_TYPE_RECEIVE) {
			assert(port->ip_receiver_name == name);
			assert(port->ip_receiver == space);

			ipc_port_destroy(port);                                                 
		} else if (type & MACH_PORT_TYPE_SEND_ONCE) {
			assert(port->ip_sorights > 0);
			port->ip_reply_context = 0;
			ip_unlock(port);

			ipc_notify_send_once(port);                       
		} else {
			assert(port->ip_receiver != space);

			ip_unlock(port);
			ip_release(port);
		}

		if (nsrequest != IP_NULL) {
			ipc_notify_no_senders(nsrequest, mscount);
		}

		if (request != IP_NULL) {
			ipc_notify_port_deleted(request, name);
		}
		break;
	}

	default:
		panic("ipc_right_terminate: strange type - 0x%x", type);
	}
}

  
                             
           
                                 
              
                                                 
                             
           
                                          
   

kern_return_t
ipc_right_destroy(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             entry,
	boolean_t               check_guard,
	uint64_t                guard)
{
	ipc_entry_bits_t bits;
	mach_port_type_t type;

	bits = entry->ie_bits;
	entry->ie_bits &= ~IE_BITS_TYPE_MASK;
	type = IE_BITS_TYPE(bits);

	assert(is_active(space));

	switch (type) {
	case MACH_PORT_TYPE_DEAD_NAME:
		assert(entry->ie_request == IE_REQ_NONE);
		assert(entry->ie_object == IO_NULL);

		ipc_entry_dealloc(space, name, entry);
		is_write_unlock(space);
		break;

	case MACH_PORT_TYPE_PORT_SET: {
		ipc_pset_t pset = ips_object_to_pset(entry->ie_object);

		assert(entry->ie_request == IE_REQ_NONE);
		assert(pset != IPS_NULL);

		entry->ie_object = IO_NULL;
		ipc_entry_dealloc(space, name, entry);

		ips_lock(pset);
		is_write_unlock(space);

		assert(ips_active(pset));
		ipc_pset_destroy(space, pset);                            
		break;
	}

	case MACH_PORT_TYPE_SEND:
	case MACH_PORT_TYPE_RECEIVE:
	case MACH_PORT_TYPE_SEND_RECEIVE:
	case MACH_PORT_TYPE_SEND_ONCE: {
		ipc_port_t port = ip_object_to_port(entry->ie_object);
		ipc_port_t nsrequest = IP_NULL;
		mach_port_mscount_t mscount = 0;
		ipc_port_t request;

		assert(port != IP_NULL);

		if (type == MACH_PORT_TYPE_SEND) {
			ipc_hash_delete(space, ip_to_object(port), name, entry);
		}

		ip_lock(port);

		if (!ip_active(port)) {
			assert((type & MACH_PORT_TYPE_RECEIVE) == 0);
			ip_unlock(port);
			entry->ie_request = IE_REQ_NONE;
			entry->ie_object = IO_NULL;
			ipc_entry_dealloc(space, name, entry);
			is_write_unlock(space);
			ip_release(port);
			break;
		}

		                                            
		if ((type & MACH_PORT_TYPE_RECEIVE) &&
		    (check_guard) && (port->ip_guarded) &&
		    (guard != port->ip_context)) {
			                     
			uint64_t portguard = port->ip_context;
			ip_unlock(port);
			is_write_unlock(space);
			                                     
			mach_port_guard_exception(name, 0, portguard, kGUARD_EXC_DESTROY);
			return KERN_INVALID_RIGHT;
		}


		request = ipc_right_request_cancel_macro(space, port, name, entry);

		entry->ie_object = IO_NULL;
		ipc_entry_dealloc(space, name, entry);
		is_write_unlock(space);

		if (type & MACH_PORT_TYPE_SEND) {
			assert(port->ip_srights > 0);
			if (--port->ip_srights == 0) {
				nsrequest = port->ip_nsrequest;
				if (nsrequest != IP_NULL) {
					port->ip_nsrequest = IP_NULL;
					mscount = port->ip_mscount;
				}
			}
		}

		if (type & MACH_PORT_TYPE_RECEIVE) {
			require_ip_active(port);
			assert(port->ip_receiver == space);

			ipc_port_destroy(port);                                                 
		} else if (type & MACH_PORT_TYPE_SEND_ONCE) {
			assert(port->ip_sorights > 0);
			port->ip_reply_context = 0;
			ip_unlock(port);

			ipc_notify_send_once(port);                       
		} else {
			assert(port->ip_receiver != space);

			ip_unlock(port);
			ip_release(port);
		}

		if (nsrequest != IP_NULL) {
			ipc_notify_no_senders(nsrequest, mscount);
		}

		if (request != IP_NULL) {
			ipc_notify_port_deleted(request, name);
		}


		break;
	}

	default:
		panic("ipc_right_destroy: strange type");
	}

	return KERN_SUCCESS;
}

  
                             
           
                                                          
                                            
                                                  
              
                                                           
                             
           
                                          
                                            
                                                             
   

kern_return_t
ipc_right_dealloc(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             entry)
{
	ipc_port_t port = IP_NULL;
	ipc_entry_bits_t bits;
	mach_port_type_t type;

	bits = entry->ie_bits;
	type = IE_BITS_TYPE(bits);


	assert(is_active(space));

	switch (type) {
	case MACH_PORT_TYPE_PORT_SET: {
		ipc_pset_t pset;

		assert(IE_BITS_UREFS(bits) == 0);
		assert(entry->ie_request == IE_REQ_NONE);

		pset = ips_object_to_pset(entry->ie_object);
		assert(pset != IPS_NULL);

		entry->ie_object = IO_NULL;
		ipc_entry_dealloc(space, name, entry);

		ips_lock(pset);
		assert(ips_active(pset));
		is_write_unlock(space);

		ipc_pset_destroy(space, pset);                            
		break;
	}

	case MACH_PORT_TYPE_DEAD_NAME: {
dead_name:

		assert(IE_BITS_UREFS(bits) > 0);
		assert(entry->ie_request == IE_REQ_NONE);
		assert(entry->ie_object == IO_NULL);

		if (IE_BITS_UREFS(bits) == 1) {
			ipc_entry_dealloc(space, name, entry);
		} else {
			                                                            
			if (IE_BITS_UREFS(bits) < MACH_PORT_UREFS_MAX) {
				entry->ie_bits = bits - 1;                      
			}
			ipc_entry_modified(space, name, entry);
		}
		is_write_unlock(space);

		                                                            
		if (port != IP_NULL) {
			ip_release(port);
		}
		break;
	}

	case MACH_PORT_TYPE_SEND_ONCE: {
		ipc_port_t request;

		assert(IE_BITS_UREFS(bits) == 1);

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		if (ipc_right_check(space, port, name, entry, IPC_OBJECT_COPYIN_FLAGS_NONE)) {
			bits = entry->ie_bits;
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_DEAD_NAME);
			goto dead_name;                               
		}
		                               

		assert(port->ip_sorights > 0);

		  
                             
                                                              
                                             
     
		port->ip_reply_context = 0;

		request = ipc_right_request_cancel_macro(space, port, name, entry);
		ip_unlock(port);

		entry->ie_object = IO_NULL;
		ipc_entry_dealloc(space, name, entry);

		is_write_unlock(space);

		ipc_notify_send_once(port);

		if (request != IP_NULL) {
			ipc_notify_port_deleted(request, name);
		}
		break;
	}

	case MACH_PORT_TYPE_SEND: {
		ipc_port_t request = IP_NULL;
		ipc_port_t nsrequest = IP_NULL;
		mach_port_mscount_t mscount =  0;


		assert(IE_BITS_UREFS(bits) > 0);

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		if (ipc_right_check(space, port, name, entry, IPC_OBJECT_COPYIN_FLAGS_NONE)) {
			bits = entry->ie_bits;
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_DEAD_NAME);
			goto dead_name;                               
		}
		                               

		assert(port->ip_srights > 0);

		if (IE_BITS_UREFS(bits) == 1) {
			if (pinned_control_port_enabled && port->ip_pinned != 0) {
				ip_unlock(port);
				is_write_unlock(space);
				mach_port_guard_exception(name, 0, MPG_FLAGS_MOD_REFS_PINNED_DEALLOC,
				    ipc_control_port_options & IPC_CONTROL_PORT_OPTIONS_PINNED_HARD ?
				    kGUARD_EXC_MOD_REFS : kGUARD_EXC_MOD_REFS_NON_FATAL);
				return KERN_INVALID_CAPABILITY;
			}
			if (--port->ip_srights == 0) {
				nsrequest = port->ip_nsrequest;
				if (nsrequest != IP_NULL) {
					port->ip_nsrequest = IP_NULL;
					mscount = port->ip_mscount;
				}
			}

			request = ipc_right_request_cancel_macro(space, port,
			    name, entry);
			ipc_hash_delete(space, ip_to_object(port), name, entry);

			ip_unlock(port);
			entry->ie_object = IO_NULL;
			ipc_entry_dealloc(space, name, entry);
			is_write_unlock(space);
			ip_release(port);
		} else {
			ip_unlock(port);
			                                                            
			if (IE_BITS_UREFS(bits) < MACH_PORT_UREFS_MAX) {
				entry->ie_bits = bits - 1;                      
			}
			ipc_entry_modified(space, name, entry);
			is_write_unlock(space);
		}

		if (nsrequest != IP_NULL) {
			ipc_notify_no_senders(nsrequest, mscount);
		}

		if (request != IP_NULL) {
			ipc_notify_port_deleted(request, name);
		}
		break;
	}

	case MACH_PORT_TYPE_SEND_RECEIVE: {
		ipc_port_t nsrequest = IP_NULL;
		mach_port_mscount_t mscount = 0;

		assert(IE_BITS_UREFS(bits) > 0);

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		ip_lock(port);
		require_ip_active(port);
		assert(port->ip_receiver_name == name);
		assert(port->ip_receiver == space);
		assert(port->ip_srights > 0);

		if (IE_BITS_UREFS(bits) == 1) {
			if (--port->ip_srights == 0) {
				nsrequest = port->ip_nsrequest;
				if (nsrequest != IP_NULL) {
					port->ip_nsrequest = IP_NULL;
					mscount = port->ip_mscount;
				}
			}

			entry->ie_bits = bits & ~(IE_BITS_UREFS_MASK |
			    MACH_PORT_TYPE_SEND);
		} else {
			                                                            
			if (IE_BITS_UREFS(bits) < MACH_PORT_UREFS_MAX) {
				entry->ie_bits = bits - 1;                      
			}
		}
		ip_unlock(port);

		ipc_entry_modified(space, name, entry);
		is_write_unlock(space);

		if (nsrequest != IP_NULL) {
			ipc_notify_no_senders(nsrequest, mscount);
		}
		break;
	}

	default:
		is_write_unlock(space);
		mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_RIGHT);
		return KERN_INVALID_RIGHT;
	}

	return KERN_SUCCESS;
}

  
                           
           
                                                  
                                                        
              
                                                           
                             
           
                                     
                                            
                                               
                                                        
   

kern_return_t
ipc_right_delta(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             entry,
	mach_port_right_t       right,
	mach_port_delta_t       delta)
{
	ipc_port_t port = IP_NULL;
	ipc_entry_bits_t bits;

	bits = entry->ie_bits;

  
                                                                    
                                                                  
                                                                  
                                                                  
                                                           
   

	assert(is_active(space));
	assert(right < MACH_PORT_RIGHT_NUMBER);

	                                                  

	switch (right) {
	case MACH_PORT_RIGHT_PORT_SET: {
		ipc_pset_t pset;

		if ((bits & MACH_PORT_TYPE_PORT_SET) == 0) {
			mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_RIGHT);
			goto invalid_right;
		}

		assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_PORT_SET);
		assert(IE_BITS_UREFS(bits) == 0);
		assert(entry->ie_request == IE_REQ_NONE);

		if (delta == 0) {
			goto success;
		}

		if (delta != -1) {
			goto invalid_value;
		}

		pset = ips_object_to_pset(entry->ie_object);
		assert(pset != IPS_NULL);

		entry->ie_object = IO_NULL;
		ipc_entry_dealloc(space, name, entry);

		ips_lock(pset);
		assert(ips_active(pset));
		is_write_unlock(space);

		ipc_pset_destroy(space, pset);                            
		break;
	}

	case MACH_PORT_RIGHT_RECEIVE: {
		ipc_port_t request = IP_NULL;

		if ((bits & MACH_PORT_TYPE_RECEIVE) == 0) {
			if ((bits & MACH_PORT_TYPE_EX_RECEIVE) == 0) {
				mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_RIGHT);
			}
			goto invalid_right;
		}

		if (delta == 0) {
			goto success;
		}

		if (delta != -1) {
			goto invalid_value;
		}

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		  
                                                    
                                                 
                                               
     

		ip_lock(port);
		require_ip_active(port);
		assert(port->ip_receiver_name == name);
		assert(port->ip_receiver == space);

		                              
		if (port->ip_guarded) {
			uint64_t portguard = port->ip_context;
			ip_unlock(port);
			is_write_unlock(space);
			                                     
			mach_port_guard_exception(name, 0, portguard, kGUARD_EXC_MOD_REFS);
			goto guard_failure;
		}

		if (bits & MACH_PORT_TYPE_SEND) {
			assert(IE_BITS_TYPE(bits) ==
			    MACH_PORT_TYPE_SEND_RECEIVE);
			assert(IE_BITS_UREFS(bits) > 0);
			assert(port->ip_srights > 0);

			if (port->ip_pdrequest != NULL) {
				  
                                         
                                             
                                                   
                                                 
                                                   
                                                
                                               
                           
       
				ipc_entry_modified(space, name, entry);
				entry->ie_bits &= ~MACH_PORT_TYPE_RECEIVE;
				entry->ie_bits |= MACH_PORT_TYPE_EX_RECEIVE;
				ipc_hash_insert(space, ip_to_object(port),
				    name, entry);
				ip_reference(port);
			} else {
				  
                                            
                                            
                                               
                                             
                                 
       
				bits &= ~IE_BITS_TYPE_MASK;
				bits |= (MACH_PORT_TYPE_DEAD_NAME | MACH_PORT_TYPE_EX_RECEIVE);
				if (entry->ie_request) {
					entry->ie_request = IE_REQ_NONE;
					                                                            
					if (IE_BITS_UREFS(bits) < MACH_PORT_UREFS_MAX) {
						bits++;                      
					}
				}
				entry->ie_bits = bits;
				entry->ie_object = IO_NULL;
				ipc_entry_modified(space, name, entry);
			}
		} else {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_RECEIVE);
			assert(IE_BITS_UREFS(bits) == 0);

			request = ipc_right_request_cancel_macro(space, port,
			    name, entry);
			entry->ie_object = IO_NULL;
			ipc_entry_dealloc(space, name, entry);
		}
		is_write_unlock(space);

		ipc_port_destroy(port);                                             

		if (request != IP_NULL) {
			ipc_notify_port_deleted(request, name);
		}
		break;
	}

	case MACH_PORT_RIGHT_SEND_ONCE: {
		ipc_port_t request;

		if ((bits & MACH_PORT_TYPE_SEND_ONCE) == 0) {
			goto invalid_right;
		}

		assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND_ONCE);
		assert(IE_BITS_UREFS(bits) == 1);

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		if (ipc_right_check(space, port, name, entry, IPC_OBJECT_COPYIN_FLAGS_NONE)) {
			assert(!(entry->ie_bits & MACH_PORT_TYPE_SEND_ONCE));
			mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_RIGHT);
			goto invalid_right;
		}
		                               

		assert(port->ip_sorights > 0);

		if ((delta > 0) || (delta < -1)) {
			ip_unlock(port);
			goto invalid_value;
		}

		if (delta == 0) {
			ip_unlock(port);
			goto success;
		}

		  
                             
                                                              
                                             
     
		port->ip_reply_context = 0;

		request = ipc_right_request_cancel_macro(space, port, name, entry);
		ip_unlock(port);

		entry->ie_object = IO_NULL;
		ipc_entry_dealloc(space, name, entry);

		is_write_unlock(space);

		ipc_notify_send_once(port);

		if (request != IP_NULL) {
			ipc_notify_port_deleted(request, name);
		}
		break;
	}

	case MACH_PORT_RIGHT_DEAD_NAME: {
		ipc_port_t relport = IP_NULL;
		mach_port_urefs_t urefs;

		if (bits & MACH_PORT_TYPE_SEND_RIGHTS) {
			port = ip_object_to_port(entry->ie_object);
			assert(port != IP_NULL);

			if (!ipc_right_check(space, port, name, entry, IPC_OBJECT_COPYIN_FLAGS_NONE)) {
				                               
				ip_unlock(port);
				port = IP_NULL;
				mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_RIGHT);
				goto invalid_right;
			}
			bits = entry->ie_bits;
			relport = port;
			port = IP_NULL;
		} else if ((bits & MACH_PORT_TYPE_DEAD_NAME) == 0) {
			mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_RIGHT);
			goto invalid_right;
		}

		assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_DEAD_NAME);
		assert(IE_BITS_UREFS(bits) > 0);
		assert(entry->ie_object == IO_NULL);
		assert(entry->ie_request == IE_REQ_NONE);

		if (delta > ((mach_port_delta_t)MACH_PORT_UREFS_MAX) ||
		    delta < (-((mach_port_delta_t)MACH_PORT_UREFS_MAX))) {
			goto invalid_value;
		}

		urefs = IE_BITS_UREFS(bits);

		if (urefs == MACH_PORT_UREFS_MAX) {
			  
                                         
                                                          
      

			if (delta != (-((mach_port_delta_t)MACH_PORT_UREFS_MAX))) {
				delta = 0;
			}
		} else {
			if (MACH_PORT_UREFS_UNDERFLOW(urefs, delta)) {
				goto invalid_value;
			}
			if (MACH_PORT_UREFS_OVERFLOW(urefs, delta)) {
				                                                    
				delta = MACH_PORT_UREFS_MAX - urefs;
			}
		}

		if ((urefs + delta) == 0) {
			ipc_entry_dealloc(space, name, entry);
		} else if (delta != 0) {
			entry->ie_bits = bits + delta;
			ipc_entry_modified(space, name, entry);
		}

		is_write_unlock(space);

		if (relport != IP_NULL) {
			ip_release(relport);
		}

		break;
	}

	case MACH_PORT_RIGHT_SEND: {
		mach_port_urefs_t urefs;
		ipc_port_t request = IP_NULL;
		ipc_port_t nsrequest = IP_NULL;
		ipc_port_t port_to_release = IP_NULL;
		mach_port_mscount_t mscount = 0;

		if ((bits & MACH_PORT_TYPE_SEND) == 0) {
			                                                               
			if ((bits & MACH_PORT_TYPE_DEAD_NAME) == 0
#if !defined(AE_MAKESENDRIGHT_FIXED)
			      
                                                                                   
                                                                                   
          
			    && (((bits & MACH_PORT_TYPE_RECEIVE) == 0) || (delta != 1))
#endif
			    ) {
				mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_RIGHT);
			}
			goto invalid_right;
		}

		                                                   

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		if (ipc_right_check(space, port, name, entry, IPC_OBJECT_COPYIN_FLAGS_NONE)) {
			assert((entry->ie_bits & MACH_PORT_TYPE_SEND) == 0);
			goto invalid_right;
		}
		                               

		assert(port->ip_srights > 0);

		if (delta > ((mach_port_delta_t)MACH_PORT_UREFS_MAX) ||
		    delta < (-((mach_port_delta_t)MACH_PORT_UREFS_MAX))) {
			ip_unlock(port);
			goto invalid_value;
		}

		urefs = IE_BITS_UREFS(bits);

		if (urefs == MACH_PORT_UREFS_MAX) {
			  
                                         
                                                          
      

			if (delta != (-((mach_port_delta_t)MACH_PORT_UREFS_MAX))) {
				delta = 0;
			}
		} else {
			if (MACH_PORT_UREFS_UNDERFLOW(urefs, delta)) {
				ip_unlock(port);
				goto invalid_value;
			}
			if (MACH_PORT_UREFS_OVERFLOW(urefs, delta)) {
				                                                    
				delta = MACH_PORT_UREFS_MAX - urefs;
			}
		}

		if ((urefs + delta) == 0) {
			if (pinned_control_port_enabled && port->ip_pinned != 0) {
				ip_unlock(port);
				goto pinned_right;
			}

			if (--port->ip_srights == 0) {
				nsrequest = port->ip_nsrequest;
				if (nsrequest != IP_NULL) {
					port->ip_nsrequest = IP_NULL;
					mscount = port->ip_mscount;
				}
			}

			if (bits & MACH_PORT_TYPE_RECEIVE) {
				assert(port->ip_receiver_name == name);
				assert(port->ip_receiver == space);
				ip_unlock(port);
				assert(IE_BITS_TYPE(bits) ==
				    MACH_PORT_TYPE_SEND_RECEIVE);

				entry->ie_bits = bits & ~(IE_BITS_UREFS_MASK |
				    MACH_PORT_TYPE_SEND);
				ipc_entry_modified(space, name, entry);
			} else {
				assert(IE_BITS_TYPE(bits) ==
				    MACH_PORT_TYPE_SEND);

				request = ipc_right_request_cancel_macro(space, port,
				    name, entry);
				ipc_hash_delete(space, ip_to_object(port),
				    name, entry);

				ip_unlock(port);
				port_to_release = port;

				entry->ie_object = IO_NULL;
				ipc_entry_dealloc(space, name, entry);
			}
		} else if (delta != 0) {
			ip_unlock(port);
			entry->ie_bits = bits + delta;
			ipc_entry_modified(space, name, entry);
		} else {
			ip_unlock(port);
		}

		is_write_unlock(space);

		if (port_to_release != IP_NULL) {
			ip_release(port_to_release);
		}

		if (nsrequest != IP_NULL) {
			ipc_notify_no_senders(nsrequest, mscount);
		}

		if (request != IP_NULL) {
			ipc_notify_port_deleted(request, name);
		}
		break;
	}

	case MACH_PORT_RIGHT_LABELH:
		goto invalid_right;

	default:
		panic("ipc_right_delta: strange right %d for 0x%x (%p) in space:%p",
		    right, name, (void *)entry, (void *)space);
	}

	return KERN_SUCCESS;

success:
	is_write_unlock(space);
	return KERN_SUCCESS;

invalid_right:
	is_write_unlock(space);
	if (port != IP_NULL) {
		ip_release(port);
	}
	return KERN_INVALID_RIGHT;

pinned_right:
	assert(pinned_control_port_enabled);

	is_write_unlock(space);
	mach_port_guard_exception(name, 0, MPG_FLAGS_MOD_REFS_PINNED_DEALLOC,
	    ipc_control_port_options & IPC_CONTROL_PORT_OPTIONS_PINNED_HARD ?
	    kGUARD_EXC_MOD_REFS : kGUARD_EXC_MOD_REFS_NON_FATAL);
	return KERN_INVALID_CAPABILITY;

invalid_value:
	is_write_unlock(space);
	mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_VALUE);
	return KERN_INVALID_VALUE;

guard_failure:
	return KERN_INVALID_RIGHT;
}

  
                              
           
                                                  
                                                          
              
                                                           
                             
           
                                     
                                            
                                               
   

kern_return_t
ipc_right_destruct(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             entry,
	mach_port_delta_t       srdelta,
	uint64_t                guard)
{
	ipc_port_t port = IP_NULL;
	ipc_entry_bits_t bits;

	mach_port_urefs_t urefs;
	ipc_port_t request = IP_NULL;
	ipc_port_t nsrequest = IP_NULL;
	mach_port_mscount_t mscount = 0;

	bits = entry->ie_bits;

	assert(is_active(space));

	if ((bits & MACH_PORT_TYPE_RECEIVE) == 0) {
		is_write_unlock(space);

		                                                                  
		if ((bits & MACH_PORT_TYPE_EX_RECEIVE) == 0) {
			mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_RIGHT);
		}
		return KERN_INVALID_RIGHT;
	}

	if (srdelta && (bits & MACH_PORT_TYPE_SEND) == 0) {
		is_write_unlock(space);
		mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_RIGHT);
		return KERN_INVALID_RIGHT;
	}

	if (srdelta > 0) {
		goto invalid_value;
	}

	port = ip_object_to_port(entry->ie_object);
	assert(port != IP_NULL);

	ip_lock(port);
	require_ip_active(port);
	assert(port->ip_receiver_name == name);
	assert(port->ip_receiver == space);

	                              
	if (port->ip_guarded && (guard != port->ip_context)) {
		uint64_t portguard = port->ip_context;
		ip_unlock(port);
		is_write_unlock(space);
		mach_port_guard_exception(name, 0, portguard, kGUARD_EXC_DESTROY);
		return KERN_INVALID_ARGUMENT;
	}

	  
                                                 
                                              
                                                      
                           
    

	if (srdelta) {
		assert(port->ip_srights > 0);

		urefs = IE_BITS_UREFS(bits);

		  
                                                 
                                                  
     
		if (MACH_PORT_UREFS_UNDERFLOW(urefs, srdelta)) {
			ip_unlock(port);
			goto invalid_value;
		}

		if (urefs == MACH_PORT_UREFS_MAX) {
			  
                                         
                                                          
      
			if (srdelta != (-((mach_port_delta_t)MACH_PORT_UREFS_MAX))) {
				srdelta = 0;
			}
		}

		if ((urefs + srdelta) == 0) {
			if (--port->ip_srights == 0) {
				nsrequest = port->ip_nsrequest;
				if (nsrequest != IP_NULL) {
					port->ip_nsrequest = IP_NULL;
					mscount = port->ip_mscount;
				}
			}
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND_RECEIVE);
			entry->ie_bits = bits & ~(IE_BITS_UREFS_MASK |
			    MACH_PORT_TYPE_SEND);
		} else {
			entry->ie_bits = bits + srdelta;
		}
	}

	  
                                                   
                      
    

	bits = entry->ie_bits;
	if (bits & MACH_PORT_TYPE_SEND) {
		assert(IE_BITS_UREFS(bits) > 0);
		assert(IE_BITS_UREFS(bits) <= MACH_PORT_UREFS_MAX);

		if (port->ip_pdrequest != NULL) {
			  
                                        
                                            
                                                  
                                                
                                                  
                                               
                                              
                          
      
			ipc_entry_modified(space, name, entry);
			entry->ie_bits &= ~MACH_PORT_TYPE_RECEIVE;
			entry->ie_bits |= MACH_PORT_TYPE_EX_RECEIVE;
			ipc_hash_insert(space, ip_to_object(port),
			    name, entry);
			ip_reference(port);
		} else {
			  
                                           
                                           
                                              
                                            
                                
      
			bits &= ~IE_BITS_TYPE_MASK;
			bits |= (MACH_PORT_TYPE_DEAD_NAME | MACH_PORT_TYPE_EX_RECEIVE);
			if (entry->ie_request) {
				entry->ie_request = IE_REQ_NONE;
				if (IE_BITS_UREFS(bits) < MACH_PORT_UREFS_MAX) {
					bits++;                      
				}
			}
			entry->ie_bits = bits;
			entry->ie_object = IO_NULL;
			ipc_entry_modified(space, name, entry);
		}
	} else {
		assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_RECEIVE);
		assert(IE_BITS_UREFS(bits) == 0);
		request = ipc_right_request_cancel_macro(space, port,
		    name, entry);
		entry->ie_object = IO_NULL;
		ipc_entry_dealloc(space, name, entry);
	}

	                  
	is_write_unlock(space);

	if (nsrequest != IP_NULL) {
		ipc_notify_no_senders(nsrequest, mscount);
	}

	ipc_port_destroy(port);                                             

	if (request != IP_NULL) {
		ipc_notify_port_deleted(request, name);
	}

	return KERN_SUCCESS;

invalid_value:
	is_write_unlock(space);
	mach_port_guard_exception(name, 0, 0, kGUARD_EXC_INVALID_VALUE);
	return KERN_INVALID_VALUE;
}


  
                          
           
                                          
              
                                         
                                             
           
                                
   

kern_return_t
ipc_right_info(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             entry,
	mach_port_type_t        *typep,
	mach_port_urefs_t       *urefsp)
{
	ipc_port_t port;
	ipc_entry_bits_t bits;
	mach_port_type_t type = 0;
	ipc_port_request_index_t request;

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
		is_write_unlock(space);
	} else if (bits & MACH_PORT_TYPE_SEND_RIGHTS) {
		  
                                                      
                                                     
                                   
     
		if (!ipc_right_check(space, port, name, entry, IPC_OBJECT_COPYIN_FLAGS_NONE)) {
			if (request != IE_REQ_NONE) {
				type |= ipc_port_request_type(port, name, request);
			}
			ip_unlock(port);
			is_write_unlock(space);
		} else {
			bits = entry->ie_bits;
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_DEAD_NAME);
			is_write_unlock(space);
			ip_release(port);
		}
	} else {
		is_write_unlock(space);
	}

	type |= IE_BITS_TYPE(bits);

	*typep = type;
	*urefsp = IE_BITS_UREFS(bits);
	return KERN_SUCCESS;
}

  
                                        
           
                                                                   
                                                                      
                                                                
              
                                                   
   

boolean_t
ipc_right_copyin_check_reply(
	__assert_only ipc_space_t       space,
	mach_port_name_t                reply_name,
	ipc_entry_t                     reply_entry,
	mach_msg_type_name_t            reply_type)
{
	ipc_entry_bits_t bits;
	ipc_port_t reply_port;

	bits = reply_entry->ie_bits;
	assert(is_active(space));

	switch (reply_type) {
	case MACH_MSG_TYPE_MAKE_SEND:
		if ((bits & MACH_PORT_TYPE_RECEIVE) == 0) {
			return FALSE;
		}
		break;

	case MACH_MSG_TYPE_MAKE_SEND_ONCE:
		if ((bits & MACH_PORT_TYPE_RECEIVE) == 0) {
			return FALSE;
		}
		break;

	case MACH_MSG_TYPE_MOVE_RECEIVE:
		                                                   
		return FALSE;

	case MACH_MSG_TYPE_COPY_SEND:
	case MACH_MSG_TYPE_MOVE_SEND:
	case MACH_MSG_TYPE_MOVE_SEND_ONCE: {
		if (bits & MACH_PORT_TYPE_DEAD_NAME) {
			break;
		}

		if ((bits & MACH_PORT_TYPE_SEND_RIGHTS) == 0) {
			return FALSE;
		}

		reply_port = ip_object_to_port(reply_entry->ie_object);
		assert(reply_port != IP_NULL);

		  
                                                            
                                                             
                                                   
     
		if (!ip_active(reply_port)) {
			break;
		}

		  
                                                                 
                                                                
                            
     
		if (reply_port->ip_immovable_send) {
			if (!ip_is_control(reply_port) || immovable_control_port_enabled) {
				mach_port_guard_exception_immovable(reply_name, reply_port, MPG_FLAGS_NONE);
				return FALSE;
			}
		}

		if (reply_type == MACH_MSG_TYPE_MOVE_SEND_ONCE) {
			if ((bits & MACH_PORT_TYPE_SEND_ONCE) == 0) {
				return FALSE;
			}
		} else {
			if ((bits & MACH_PORT_TYPE_SEND) == 0) {
				return FALSE;
			}
		}

		break;
	}

	default:
		panic("ipc_right_copyin_check: strange rights");
	}

	return TRUE;
}

  
                                               
           
                                              
                                                  
                                                     
                                   
              
                       
           
                                          
                                     
                                                                             
                                              
   
static kern_return_t
ipc_right_copyin_check_guard_locked(
	mach_port_name_t name,
	ipc_port_t port,
	mach_port_context_t context,
	mach_msg_guard_flags_t *guard_flags)
{
	mach_msg_guard_flags_t flags = *guard_flags;
	if ((flags & MACH_MSG_GUARD_FLAGS_UNGUARDED_ON_SEND) && !port->ip_guarded && !context) {
		return KERN_SUCCESS;
	} else if (port->ip_guarded && (port->ip_context == context)) {
		return KERN_SUCCESS;
	}

	                                      
	mach_port_guard_exception(name, context, port->ip_context, kGUARD_EXC_INCORRECT_GUARD);
	return KERN_INVALID_ARGUMENT;
}

  
                            
           
                                     
                                        
                                                   
                                               
                                           
  
                                               
                                           
  
                                          
                                          
                                           
                           
              
                                         
           
                                                       
                                                         
                                                                                 
                                                
                                                             
   

kern_return_t
ipc_right_copyin(
	ipc_space_t                space,
	mach_port_name_t           name,
	ipc_entry_t                entry,
	mach_msg_type_name_t       msgt_name,
	ipc_object_copyin_flags_t   flags,
	ipc_object_t               *objectp,
	ipc_port_t                 *sorightp,
	ipc_port_t                 *releasep,
	int                        *assertcntp,
	mach_port_context_t        context,
	mach_msg_guard_flags_t     *guard_flags)
{
	ipc_entry_bits_t bits;
	ipc_port_t port;
	kern_return_t kr;
	boolean_t deadok = !!(flags & IPC_OBJECT_COPYIN_FLAGS_DEADOK);
	boolean_t allow_imm_send = !!(flags & IPC_OBJECT_COPYIN_FLAGS_ALLOW_IMMOVABLE_SEND);
	boolean_t soft_fail_imm_send = !!(flags & IPC_OBJECT_COPYIN_FLAGS_SOFT_FAIL_IMMOVABLE_SEND);

	*releasep = IP_NULL;
	*assertcntp = 0;

	bits = entry->ie_bits;

	assert(is_active(space));

	switch (msgt_name) {
	case MACH_MSG_TYPE_MAKE_SEND: {
		if ((bits & MACH_PORT_TYPE_RECEIVE) == 0) {
			goto invalid_right;
		}

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		ip_lock(port);
		assert(port->ip_receiver_name == name);
		assert(port->ip_receiver == space);

		ipc_port_make_send_locked(port);
		ip_unlock(port);

		*objectp = ip_to_object(port);
		*sorightp = IP_NULL;
		break;
	}

	case MACH_MSG_TYPE_MAKE_SEND_ONCE: {
		if ((bits & MACH_PORT_TYPE_RECEIVE) == 0) {
			goto invalid_right;
		}

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		ip_lock(port);
		require_ip_active(port);
		assert(port->ip_receiver_name == name);
		assert(port->ip_receiver == space);

		ipc_port_make_sonce_locked(port);
		ip_unlock(port);

		*objectp = ip_to_object(port);
		*sorightp = IP_NULL;
		break;
	}

	case MACH_MSG_TYPE_MOVE_RECEIVE: {
		ipc_port_t request = IP_NULL;

		if ((bits & MACH_PORT_TYPE_RECEIVE) == 0) {
			goto invalid_right;
		}

		  
                                                                        
                                                               
                                                                 
                                                               
          
     
		if (io_is_kobject(entry->ie_object) ||
		    io_is_kolabeled(entry->ie_object)) {
			  
                                                        
                                                
                                                         
                                                     
      
			mach_port_guard_exception(name, 0, 0, kGUARD_EXC_IMMOVABLE);
			return KERN_INVALID_CAPABILITY;
		}

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		ip_lock(port);
		require_ip_active(port);
		assert(port->ip_receiver_name == name);
		assert(port->ip_receiver == space);

		if (port->ip_immovable_receive || port->ip_specialreply) {
			assert(port->ip_receiver != ipc_space_kernel);
			ip_unlock(port);
			assert(current_task() != kernel_task);
			mach_port_guard_exception(name, 0, 0, kGUARD_EXC_IMMOVABLE);
			return KERN_INVALID_CAPABILITY;
		}

		if (guard_flags != NULL) {
			kr = ipc_right_copyin_check_guard_locked(name, port, context, guard_flags);
			if (kr != KERN_SUCCESS) {
				ip_unlock(port);
				return kr;
			}
		}

		if (bits & MACH_PORT_TYPE_SEND) {
			assert(IE_BITS_TYPE(bits) ==
			    MACH_PORT_TYPE_SEND_RECEIVE);
			assert(IE_BITS_UREFS(bits) > 0);
			assert(port->ip_srights > 0);

			ipc_hash_insert(space, ip_to_object(port),
			    name, entry);
			ip_reference(port);
		} else {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_RECEIVE);
			assert(IE_BITS_UREFS(bits) == 0);

			request = ipc_right_request_cancel_macro(space, port,
			    name, entry);
			entry->ie_object = IO_NULL;
		}
		entry->ie_bits = bits & ~MACH_PORT_TYPE_RECEIVE;
		entry->ie_bits |= MACH_PORT_TYPE_EX_RECEIVE;
		ipc_entry_modified(space, name, entry);

		                                                                                       
		(void)ipc_port_clear_receiver(port, FALSE);                                    
		if (guard_flags != NULL) {
			                                              
			*guard_flags = *guard_flags | MACH_MSG_GUARD_FLAGS_UNGUARDED_ON_SEND;
		}

#if IMPORTANCE_INHERITANCE
		  
                                                              
                                                            
                                                              
                                                               
                                                                 
                                                       
                                                         
     
		if (port->ip_tempowner == 0) {
			assert(IIT_NULL == port->ip_imp_task);

			                                         
			port->ip_tempowner = 1;
			*assertcntp = port->ip_impcount;
		}
#endif                             

		ip_unlock(port);

		*objectp = ip_to_object(port);
		*sorightp = request;
		break;
	}

	case MACH_MSG_TYPE_COPY_SEND: {
		if (bits & MACH_PORT_TYPE_DEAD_NAME) {
			goto copy_dead;
		}

		                                     

		if ((bits & MACH_PORT_TYPE_SEND_RIGHTS) == 0) {
			goto invalid_right;
		}

		assert(IE_BITS_UREFS(bits) > 0);

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		if (ipc_right_check(space, port, name, entry, IPC_OBJECT_COPYIN_FLAGS_NONE)) {
			bits = entry->ie_bits;
			*releasep = port;
			goto copy_dead;
		}
		                               

		if ((bits & MACH_PORT_TYPE_SEND) == 0) {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND_ONCE);
			assert(port->ip_sorights > 0);

			ip_unlock(port);
			goto invalid_right;
		}

		if (!allow_imm_send && port->ip_immovable_send) {
			if (!ip_is_control(port) || immovable_control_port_enabled) {
				ip_unlock(port);
				if (!soft_fail_imm_send) {
					mach_port_guard_exception_immovable(name, port, MPG_FLAGS_NONE);
				}
				return KERN_INVALID_CAPABILITY;
			}
		}

		ipc_port_copy_send_locked(port);
		ip_unlock(port);

		*objectp = ip_to_object(port);
		*sorightp = IP_NULL;
		break;
	}

	case MACH_MSG_TYPE_MOVE_SEND: {
		ipc_port_t request = IP_NULL;

		if (bits & MACH_PORT_TYPE_DEAD_NAME) {
			goto move_dead;
		}

		                                     

		if ((bits & MACH_PORT_TYPE_SEND_RIGHTS) == 0) {
			goto invalid_right;
		}

		assert(IE_BITS_UREFS(bits) > 0);

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		if (ipc_right_check(space, port, name, entry, IPC_OBJECT_COPYIN_FLAGS_NONE)) {
			bits = entry->ie_bits;
			*releasep = port;
			goto move_dead;
		}
		                               

		if ((bits & MACH_PORT_TYPE_SEND) == 0) {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND_ONCE);
			assert(port->ip_sorights > 0);
			ip_unlock(port);
			goto invalid_right;
		}

		if (!allow_imm_send && port->ip_immovable_send) {
			if (!ip_is_control(port) || immovable_control_port_enabled) {
				ip_unlock(port);
				if (!soft_fail_imm_send) {
					mach_port_guard_exception_immovable(name, port, MPG_FLAGS_NONE);
				}
				return KERN_INVALID_CAPABILITY;
			}
		}

		if (IE_BITS_UREFS(bits) == 1) {
			assert(port->ip_srights > 0);
			if (bits & MACH_PORT_TYPE_RECEIVE) {
				assert(port->ip_receiver_name == name);
				assert(port->ip_receiver == space);
				assert(IE_BITS_TYPE(bits) ==
				    MACH_PORT_TYPE_SEND_RECEIVE);
				assert(port->ip_pinned == 0);

				ip_reference(port);
			} else {
				assert(IE_BITS_TYPE(bits) ==
				    MACH_PORT_TYPE_SEND);

				request = ipc_right_request_cancel_macro(space, port,
				    name, entry);
				ipc_hash_delete(space, ip_to_object(port),
				    name, entry);
				entry->ie_object = IO_NULL;
				                                          
			}
			entry->ie_bits = bits & ~
			    (IE_BITS_UREFS_MASK | MACH_PORT_TYPE_SEND);
		} else {
			ipc_port_copy_send_locked(port);
			                                                            
			if (IE_BITS_UREFS(bits) < MACH_PORT_UREFS_MAX) {
				entry->ie_bits = bits - 1;                      
			}
		}

		ipc_entry_modified(space, name, entry);
		ip_unlock(port);

		*objectp = ip_to_object(port);
		*sorightp = request;
		break;
	}

	case MACH_MSG_TYPE_MOVE_SEND_ONCE: {
		ipc_port_t request;

		if (bits & MACH_PORT_TYPE_DEAD_NAME) {
			goto move_dead;
		}

		                                

		if ((bits & MACH_PORT_TYPE_SEND_RIGHTS) == 0) {
			goto invalid_right;
		}

		assert(IE_BITS_UREFS(bits) > 0);

		port = ip_object_to_port(entry->ie_object);
		assert(port != IP_NULL);

		if (ipc_right_check(space, port, name, entry, flags)) {
			bits = entry->ie_bits;
			*releasep = port;
			goto move_dead;
		}
		  
                                           
                                                                          
                                                                         
           
     

		if ((bits & MACH_PORT_TYPE_SEND_ONCE) == 0) {
			assert(bits & MACH_PORT_TYPE_SEND);
			assert(port->ip_srights > 0);

			ip_unlock(port);
			goto invalid_right;
		}

		if (!allow_imm_send && port->ip_immovable_send) {
			if (!ip_is_control(port) || immovable_control_port_enabled) {
				ip_unlock(port);
				if (!soft_fail_imm_send) {
					mach_port_guard_exception_immovable(name, port, MPG_FLAGS_NONE);
				}
				return KERN_INVALID_CAPABILITY;
			}
		}

		assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND_ONCE);
		assert(IE_BITS_UREFS(bits) == 1);
		assert(port->ip_sorights > 0);

		request = ipc_right_request_cancel_macro(space, port, name, entry);
		ip_unlock(port);

		entry->ie_object = IO_NULL;
		entry->ie_bits = bits & ~
		    (IE_BITS_UREFS_MASK | MACH_PORT_TYPE_SEND_ONCE);
		ipc_entry_modified(space, name, entry);
		*objectp = ip_to_object(port);
		*sorightp = request;
		break;
	}

	default:
invalid_right:
		return KERN_INVALID_RIGHT;
	}

	return KERN_SUCCESS;

copy_dead:
	assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_DEAD_NAME);
	assert(IE_BITS_UREFS(bits) > 0);
	assert(entry->ie_request == IE_REQ_NONE);
	assert(entry->ie_object == 0);

	if (!deadok) {
		goto invalid_right;
	}

	*objectp = IO_DEAD;
	*sorightp = IP_NULL;
	return KERN_SUCCESS;

move_dead:
	assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_DEAD_NAME);
	assert(IE_BITS_UREFS(bits) > 0);
	assert(entry->ie_request == IE_REQ_NONE);
	assert(entry->ie_object == 0);

	if (!deadok) {
		goto invalid_right;
	}

	if (IE_BITS_UREFS(bits) == 1) {
		bits &= ~MACH_PORT_TYPE_DEAD_NAME;
	}
	                                                            
	if (IE_BITS_UREFS(bits) < MACH_PORT_UREFS_MAX) {
		entry->ie_bits = bits - 1;                      
	}
	ipc_entry_modified(space, name, entry);
	*objectp = IO_DEAD;
	*sorightp = IP_NULL;
	return KERN_SUCCESS;
}

  
                                           
           
                                                      
                                                   
                        
              
                                         
                                                     
           
                                     
                                                         
   
static
kern_return_t
ipc_right_copyin_two_move_sends(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             entry,
	ipc_object_t            *objectp,
	ipc_port_t              *sorightp,
	ipc_port_t              *releasep)
{
	ipc_entry_bits_t bits;
	mach_port_urefs_t urefs;
	ipc_port_t port;
	ipc_port_t request = IP_NULL;

	*releasep = IP_NULL;

	assert(is_active(space));

	bits = entry->ie_bits;

	if ((bits & MACH_PORT_TYPE_SEND) == 0) {
		goto invalid_right;
	}

	urefs = IE_BITS_UREFS(bits);
	if (urefs < 2) {
		goto invalid_right;
	}

	port = ip_object_to_port(entry->ie_object);
	assert(port != IP_NULL);

	if (ipc_right_check(space, port, name, entry, IPC_OBJECT_COPYIN_FLAGS_NONE)) {
		*releasep = port;
		goto invalid_right;
	}
	                               

	if (urefs > 2) {
		  
                                                                        
                                                                
                                          
     
		ipc_port_copy_send_locked(port);
		ipc_port_copy_send_locked(port);
		                                                            
		if (IE_BITS_UREFS(bits) < MACH_PORT_UREFS_MAX) {
			entry->ie_bits = bits - 2;                      
		}
	} else {
		  
                                                               
                                                                
                   
    
                                                                        
                                                                  
                                 
     
		ipc_port_copy_send_locked(port);

		if (bits & MACH_PORT_TYPE_RECEIVE) {
			assert(port->ip_receiver_name == name);
			assert(port->ip_receiver == space);
			assert(IE_BITS_TYPE(bits) ==
			    MACH_PORT_TYPE_SEND_RECEIVE);

			                                                            
			ip_reference(port);
		} else {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND);

			                                                   
			request = ipc_right_request_cancel_macro(space, port,
			    name, entry);

			ipc_hash_delete(space, ip_to_object(port),
			    name, entry);
			entry->ie_object = IO_NULL;
		}

		entry->ie_bits = bits & ~(IE_BITS_UREFS_MASK | MACH_PORT_TYPE_SEND);
	}
	ipc_entry_modified(space, name, entry);

	ip_unlock(port);

	*objectp = ip_to_object(port);
	*sorightp = request;
	return KERN_SUCCESS;

invalid_right:
	return KERN_INVALID_RIGHT;
}


  
                                
           
                                                
                                                       
                       
              
                                         
                                                
                                    
           
                                     
                                                            
                                                                           
   
kern_return_t
ipc_right_copyin_two(
	ipc_space_t               space,
	mach_port_name_t          name,
	ipc_entry_t               entry,
	mach_msg_type_name_t      msgt_one,
	mach_msg_type_name_t      msgt_two,
	ipc_object_t              *objectp,
	ipc_port_t                *sorightp,
	ipc_port_t                *releasep)
{
	kern_return_t kr;
	int assertcnt = 0;

	assert(MACH_MSG_TYPE_PORT_ANY_SEND(msgt_one));
	assert(MACH_MSG_TYPE_PORT_ANY_SEND(msgt_two));

	  
                                                    
                                                    
                           
    

	  
                                                         
    
	if (msgt_one == MACH_MSG_TYPE_MOVE_SEND_ONCE ||
	    msgt_two == MACH_MSG_TYPE_MOVE_SEND_ONCE) {
		return KERN_INVALID_RIGHT;
	}

	if ((msgt_one == MACH_MSG_TYPE_MAKE_SEND) ||
	    (msgt_one == MACH_MSG_TYPE_MAKE_SEND_ONCE) ||
	    (msgt_two == MACH_MSG_TYPE_MAKE_SEND) ||
	    (msgt_two == MACH_MSG_TYPE_MAKE_SEND_ONCE)) {
		  
                                                   
    
                                                      
                                                  
                                                    
             
    
                                              
     
		ipc_object_t object_two;

		kr = ipc_right_copyin(space, name, entry,
		    msgt_one, IPC_OBJECT_COPYIN_FLAGS_ALLOW_IMMOVABLE_SEND,
		    objectp, sorightp, releasep,
		    &assertcnt, 0, NULL);
		assert(assertcnt == 0);
		if (kr != KERN_SUCCESS) {
			return kr;
		}

		assert(IO_VALID(*objectp));
		assert(*sorightp == IP_NULL);
		assert(*releasep == IP_NULL);

		  
                                                 
                                                   
                                                 
                   
     
		kr = ipc_right_copyin(space, name, entry,
		    msgt_two, IPC_OBJECT_COPYIN_FLAGS_NONE,
		    &object_two, sorightp, releasep,
		    &assertcnt, 0, NULL);
		assert(assertcnt == 0);
		assert(kr == KERN_SUCCESS);
		assert(*sorightp == IP_NULL);
		assert(*releasep == IP_NULL);
		assert(object_two == *objectp);
		assert(entry->ie_bits & MACH_PORT_TYPE_RECEIVE);
	} else if ((msgt_one == MACH_MSG_TYPE_MOVE_SEND) &&
	    (msgt_two == MACH_MSG_TYPE_MOVE_SEND)) {
		  
                                        
                                            
                                                 
     
		kr = ipc_right_copyin_two_move_sends(space, name, entry,
		    objectp, sorightp,
		    releasep);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
	} else {
		mach_msg_type_name_t msgt_name;

		  
                                            
                                              
                                               
                                              
                                           
     
		if (msgt_one == MACH_MSG_TYPE_MOVE_SEND ||
		    msgt_two == MACH_MSG_TYPE_MOVE_SEND) {
			msgt_name = MACH_MSG_TYPE_MOVE_SEND;
		} else {
			msgt_name = MACH_MSG_TYPE_COPY_SEND;
		}

		kr = ipc_right_copyin(space, name, entry,
		    msgt_name, IPC_OBJECT_COPYIN_FLAGS_ALLOW_IMMOVABLE_SEND,
		    objectp, sorightp, releasep,
		    &assertcnt, 0, NULL);
		assert(assertcnt == 0);
		if (kr != KERN_SUCCESS) {
			return kr;
		}

		  
                                                    
                                                     
                      
     
		(void)ipc_port_copy_send(ip_object_to_port(*objectp));
	}

	return KERN_SUCCESS;
}


  
                             
           
                                    
                                                 
  
                                                       
                                                        
  
                                                            
                                                           
                                                             
                    
              
                                         
                                    
                                            
           
                                        
   

kern_return_t
ipc_right_copyout(
	ipc_space_t             space,
	mach_port_name_t        name,
	ipc_entry_t             entry,
	mach_msg_type_name_t    msgt_name,
	ipc_object_copyout_flags_t flags,
	mach_port_context_t     *context,
	mach_msg_guard_flags_t  *guard_flags,
	ipc_object_t            object)
{
	ipc_entry_bits_t bits;
	ipc_port_t port;

	bits = entry->ie_bits;

	assert(IO_VALID(object));
	assert(io_otype(object) == IOT_PORT);
	assert(io_active(object));
	assert(entry->ie_object == object);

	port = ip_object_to_port(object);

	if (pinned_control_port_enabled && (flags & IPC_OBJECT_COPYOUT_FLAGS_PINNED)) {
		assert(!port->ip_pinned);
		assert(port->ip_immovable_send);
		port->ip_pinned = 1;
	}

	switch (msgt_name) {
	case MACH_MSG_TYPE_PORT_SEND_ONCE:

		assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_NONE);
		assert(IE_BITS_UREFS(bits) == 0);
		assert(port->ip_sorights > 0);

		if (port->ip_specialreply) {
			ipc_port_adjust_special_reply_port_locked(port,
			    current_thread()->ith_knote, IPC_PORT_ADJUST_SR_LINK_WORKLOOP, FALSE);
			                             
		} else {
			ip_unlock(port);
		}

		entry->ie_bits = bits | (MACH_PORT_TYPE_SEND_ONCE | 1);                     
		ipc_entry_modified(space, name, entry);
		break;

	case MACH_MSG_TYPE_PORT_SEND:
		assert(port->ip_srights > 0);

		if (bits & MACH_PORT_TYPE_SEND) {
			mach_port_urefs_t urefs = IE_BITS_UREFS(bits);

			assert(port->ip_srights > 1);
			assert(urefs > 0);
			assert(urefs <= MACH_PORT_UREFS_MAX);

			if (urefs == MACH_PORT_UREFS_MAX) {
				  
                                     
                                 
       

				port->ip_srights--;
				ip_unlock(port);
				ip_release(port);
				return KERN_SUCCESS;
			}

			                                
			port->ip_srights--;
			ip_unlock(port);
			ip_release(port);
		} else if (bits & MACH_PORT_TYPE_RECEIVE) {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_RECEIVE);
			assert(IE_BITS_UREFS(bits) == 0);

			                                               
			ip_unlock(port);
			ip_release(port);
		} else {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_NONE);
			assert(IE_BITS_UREFS(bits) == 0);

			                                          
			ip_unlock(port);

			                                                  

			ipc_hash_insert(space, ip_to_object(port), name, entry);
		}

		entry->ie_bits = (bits | MACH_PORT_TYPE_SEND) + 1;                      
		ipc_entry_modified(space, name, entry);
		break;

	case MACH_MSG_TYPE_PORT_RECEIVE: {
		ipc_port_t dest;
#if IMPORTANCE_INHERITANCE
		natural_t assertcnt = port->ip_impcount;
#endif                             

		assert(port->ip_mscount == 0);
		assert(port->ip_receiver_name == MACH_PORT_NULL);

		  
                                                        
     
		if (io_is_kobject(entry->ie_object) ||
		    io_is_kolabeled(entry->ie_object)) {
			panic("ipc_right_copyout: Copyout kobject/kolabel as receive right");
		}

		imq_lock(&port->ip_messages);
		dest = port->ip_destination;

		port->ip_receiver_name = name;
		port->ip_receiver = space;

		struct knote *kn = current_thread()->ith_knote;

		if ((guard_flags != NULL) && ((*guard_flags & MACH_MSG_GUARD_FLAGS_IMMOVABLE_RECEIVE) != 0)) {
			assert(port->ip_immovable_receive == 0);
			port->ip_guarded = 1;
			port->ip_strict_guard = 0;
			                                                                                       
			if (kn != ITH_KNOTE_PSEUDO) {
				port->ip_immovable_receive = 1;
			}
			port->ip_context = current_thread()->ith_msg_addr;
			*context = port->ip_context;
			*guard_flags = *guard_flags & ~MACH_MSG_GUARD_FLAGS_UNGUARDED_ON_SEND;
		}

		assert((bits & MACH_PORT_TYPE_RECEIVE) == 0);
		if (bits & MACH_PORT_TYPE_SEND) {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_SEND);
			assert(IE_BITS_UREFS(bits) > 0);
			assert(port->ip_srights > 0);
		} else {
			assert(IE_BITS_TYPE(bits) == MACH_PORT_TYPE_NONE);
			assert(IE_BITS_UREFS(bits) == 0);
		}

		boolean_t sync_bootstrap_checkin = FALSE;
		if (kn != ITH_KNOTE_PSEUDO && port->ip_sync_bootstrap_checkin) {
			sync_bootstrap_checkin = TRUE;
		}
		if (!ITH_KNOTE_VALID(kn, MACH_MSG_TYPE_PORT_RECEIVE)) {
			kn = NULL;
		}
		ipc_port_adjust_port_locked(port, kn, sync_bootstrap_checkin);
		                                       

		if (bits & MACH_PORT_TYPE_SEND) {
			ip_release(port);

			                                                  
			ipc_hash_delete(space, ip_to_object(port), name, entry);
		}
		entry->ie_bits = bits | MACH_PORT_TYPE_RECEIVE;
		ipc_entry_modified(space, name, entry);

		if (dest != IP_NULL) {
#if IMPORTANCE_INHERITANCE
			  
                                                   
                                                
                                                 
                       
      
			ip_lock(dest);
			ipc_port_impcount_delta(dest, 0 - assertcnt, IP_NULL);
			ip_unlock(dest);
#endif                             

			                                
			ipc_port_send_turnstile_complete(dest);
			ip_release(dest);
		}
		break;
	}

	default:
		panic("ipc_right_copyout: strange rights");
	}
	return KERN_SUCCESS;
}
