/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
  
                                                                         
                                                                          
                                                                        
               
   
  
   
  
                       
                      
             
  
                                     
   

#include <mach_assert.h>

#include <mach/port.h>
#include <mach/kern_return.h>
#include <kern/ipc_kobject.h>
#include <kern/thread.h>
#include <kern/misc_protos.h>
#include <kern/waitq.h>
#include <kern/policy_internal.h>
#include <kern/debug.h>
#include <kern/kcdata.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_object.h>
#include <ipc/ipc_right.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_pset.h>
#include <ipc/ipc_kmsg.h>
#include <ipc/ipc_mqueue.h>
#include <ipc/ipc_notify.h>
#include <ipc/ipc_table.h>
#include <ipc/ipc_importance.h>
#include <machine/limits.h>
#include <kern/turnstile.h>
#include <kern/machine.h>

#include <security/mac_mach_internal.h>

#include <string.h>

static TUNABLE(bool, prioritize_launch, "prioritize_launch", true);
TUNABLE_WRITEABLE(int, ipc_portbt, "ipc_portbt", false);

LCK_SPIN_DECLARE_ATTR(ipc_port_multiple_lock_data, &ipc_lck_grp, &ipc_lck_attr);
ipc_port_timestamp_t ipc_port_timestamp_data;

#if     MACH_ASSERT
void    ipc_port_init_debug(
	ipc_port_t      port,
	uintptr_t       *callstack,
	unsigned int    callstack_max);

void    ipc_port_callstack_init_debug(
	uintptr_t       *callstack,
	unsigned int    callstack_max);

#endif                   

static void
ipc_port_send_turnstile_recompute_push_locked(
	ipc_port_t port);

static thread_t
ipc_port_get_watchport_inheritor(
	ipc_port_t port);

void
ipc_port_release(ipc_port_t port)
{
	ip_release(port);
}

void
ipc_port_reference(ipc_port_t port)
{
	ip_reference(port);
}

  
                              
           
                               
   

ipc_port_timestamp_t
ipc_port_timestamp(void)
{
	return OSIncrementAtomic(&ipc_port_timestamp_data);
}

  
                                  
           
                                   
                                             
                           
              
                                  
           
                                            
                                      
   

#if IMPORTANCE_INHERITANCE
kern_return_t
ipc_port_request_alloc(
	ipc_port_t                      port,
	mach_port_name_t                name,
	ipc_port_t                      soright,
	boolean_t                       send_possible,
	boolean_t                       immediate,
	ipc_port_request_index_t        *indexp,
	boolean_t                       *importantp)
#else
kern_return_t
ipc_port_request_alloc(
	ipc_port_t                      port,
	mach_port_name_t                name,
	ipc_port_t                      soright,
	boolean_t                       send_possible,
	boolean_t                       immediate,
	ipc_port_request_index_t        *indexp)
#endif                             
{
	ipc_port_request_t ipr, table;
	ipc_port_request_index_t index;
	uintptr_t mask = 0;

#if IMPORTANCE_INHERITANCE
	*importantp = FALSE;
#endif                             

	require_ip_active(port);
	assert(name != MACH_PORT_NULL);
	assert(soright != IP_NULL);

	table = port->ip_requests;

	if (table == IPR_NULL) {
		return KERN_NO_SPACE;
	}

	index = table->ipr_next;
	if (index == 0) {
		return KERN_NO_SPACE;
	}

	ipr = &table[index];
	assert(ipr->ipr_name == MACH_PORT_NULL);

	table->ipr_next = ipr->ipr_next;
	ipr->ipr_name = name;

	if (send_possible) {
		mask |= IPR_SOR_SPREQ_MASK;
		if (immediate) {
			mask |= IPR_SOR_SPARM_MASK;
			if (port->ip_sprequests == 0) {
				port->ip_sprequests = 1;
#if IMPORTANCE_INHERITANCE
				                                                    
				if (port->ip_impdonation != 0 &&
				    port->ip_spimportant == 0 &&
				    (task_is_importance_donor(current_task()))) {
					*importantp = TRUE;
				}
#endif                            
			}
		}
	}
	ipr->ipr_soright = IPR_SOR_MAKE(soright, mask);

	*indexp = index;

	return KERN_SUCCESS;
}

  
                                 
           
                                    
              
                                       
                                              
                                     
           
                                 
                                               
                                
                                                       
                                                
   

kern_return_t
ipc_port_request_grow(
	ipc_port_t              port,
	ipc_table_elems_t       target_size)
{
	ipc_table_size_t its;
	ipc_port_request_t otable, ntable;
	require_ip_active(port);

	otable = port->ip_requests;
	if (otable == IPR_NULL) {
		its = &ipc_table_requests[0];
	} else {
		its = otable->ipr_size + 1;
	}

	if (target_size != ITS_SIZE_NONE) {
		if ((otable != IPR_NULL) &&
		    (target_size <= otable->ipr_size->its_size)) {
			ip_unlock(port);
			return KERN_SUCCESS;
		}
		while ((its->its_size) && (its->its_size < target_size)) {
			its++;
		}
		if (its->its_size == 0) {
			ip_unlock(port);
			return KERN_NO_SPACE;
		}
	}

	ip_reference(port);
	ip_unlock(port);

	if ((its->its_size == 0) ||
	    ((ntable = it_requests_alloc(its)) == IPR_NULL)) {
		ip_release(port);
		return KERN_RESOURCE_SHORTAGE;
	}

	ip_lock(port);

	  
                                                        
                                                        
                                                        
                                          
    

	if (ip_active(port) && (port->ip_requests == otable) &&
	    ((otable == IPR_NULL) || (otable->ipr_size + 1 == its))) {
		ipc_table_size_t oits;
		ipc_table_elems_t osize, nsize;
		ipc_port_request_index_t free, i;

		                                 

		if (otable != IPR_NULL) {
			oits = otable->ipr_size;
			osize = oits->its_size;
			free = otable->ipr_next;

			(void) memcpy((void *)(ntable + 1),
			    (const void *)(otable + 1),
			    (osize - 1) * sizeof(struct ipc_port_request));
		} else {
			osize = 1;
			oits = 0;
			free = 0;
		}

		nsize = its->its_size;
		assert(nsize > osize);

		                                                   

		for (i = osize; i < nsize; i++) {
			ipc_port_request_t ipr = &ntable[i];

			ipr->ipr_name = MACH_PORT_NULL;
			ipr->ipr_next = free;
			free = i;
		}

		ntable->ipr_next = free;
		ntable->ipr_size = its;
		port->ip_requests = ntable;
		ip_unlock(port);
		ip_release(port);

		if (otable != IPR_NULL) {
			it_requests_free(oits, otable);
		}
	} else {
		ip_unlock(port);
		ip_release(port);
		it_requests_free(its, ntable);
	}

	return KERN_SUCCESS;
}

  
                                  
           
                                      
              
                                       
  
                                         
                                               
   

boolean_t
ipc_port_request_sparm(
	ipc_port_t                      port,
	__assert_only mach_port_name_t  name,
	ipc_port_request_index_t        index,
	mach_msg_option_t               option,
	mach_msg_priority_t             priority)
{
	if (index != IE_REQ_NONE) {
		ipc_port_request_t ipr, table;

		require_ip_active(port);

		table = port->ip_requests;
		assert(table != IPR_NULL);

		ipr = &table[index];
		assert(ipr->ipr_name == name);

		                                   
		if (IPR_SOR_SPREQ(ipr->ipr_soright)) {
			ipr->ipr_soright = IPR_SOR_MAKE(ipr->ipr_soright, IPR_SOR_SPARM_MASK);
			port->ip_sprequests = 1;

			if (option & MACH_SEND_OVERRIDE) {
				                                     
				mach_msg_qos_t qos_ovr;
				if (mach_msg_priority_is_pthread_priority(priority)) {
					qos_ovr = _pthread_priority_thread_qos(priority);
				} else {
					qos_ovr = mach_msg_priority_overide_qos(priority);
				}
				if (qos_ovr) {
					ipc_mqueue_override_send(&port->ip_messages, qos_ovr);
				}
			}

#if IMPORTANCE_INHERITANCE
			if (((option & MACH_SEND_NOIMPORTANCE) == 0) &&
			    (port->ip_impdonation != 0) &&
			    (port->ip_spimportant == 0) &&
			    (((option & MACH_SEND_IMPORTANCE) != 0) ||
			    (task_is_importance_donor(current_task())))) {
				return TRUE;
			}
#else
			return TRUE;
#endif                             
		}
	}
	return FALSE;
}

  
                                 
           
                                                              
              
                                                                
                                                                   
   
mach_port_type_t
ipc_port_request_type(
	ipc_port_t                      port,
	__assert_only mach_port_name_t  name,
	ipc_port_request_index_t        index)
{
	ipc_port_request_t ipr, table;
	mach_port_type_t type = 0;

	table = port->ip_requests;
	assert(table != IPR_NULL);

	assert(index != IE_REQ_NONE);
	ipr = &table[index];
	assert(ipr->ipr_name == name);

	if (IP_VALID(IPR_SOR_PORT(ipr->ipr_soright))) {
		type |= MACH_PORT_TYPE_DNREQUEST;

		if (IPR_SOR_SPREQ(ipr->ipr_soright)) {
			type |= MACH_PORT_TYPE_SPREQUEST;

			if (!IPR_SOR_SPARMED(ipr->ipr_soright)) {
				type |= MACH_PORT_TYPE_SPREQUEST_DELAYED;
			}
		}
	}
	return type;
}

  
                                   
           
                                                                            
              
                                       
                                                                     
   

ipc_port_t
ipc_port_request_cancel(
	ipc_port_t                      port,
	__assert_only mach_port_name_t  name,
	ipc_port_request_index_t        index)
{
	ipc_port_request_t ipr, table;
	ipc_port_t request = IP_NULL;

	require_ip_active(port);
	table = port->ip_requests;
	assert(table != IPR_NULL);

	assert(index != IE_REQ_NONE);
	ipr = &table[index];
	assert(ipr->ipr_name == name);
	request = IPR_SOR_PORT(ipr->ipr_soright);

	                                                  
	ipr->ipr_name = MACH_PORT_NULL;
	ipr->ipr_next = table->ipr_next;
	table->ipr_next = index;

	return request;
}

  
                              
           
                                              
                                          
                                                           
              
                                                   
                                                
                                              
   

void
ipc_port_pdrequest(
	ipc_port_t      port,
	ipc_port_t      notify,
	ipc_port_t      *previousp)
{
	ipc_port_t previous;
	require_ip_active(port);

	previous = port->ip_pdrequest;
	port->ip_pdrequest = notify;
	ip_unlock(port);

	*previousp = previous;
}

  
                              
           
                                            
                                          
                                                           
              
                                                   
                                                
                                              
   

void
ipc_port_nsrequest(
	ipc_port_t              port,
	mach_port_mscount_t     sync,
	ipc_port_t              notify,
	ipc_port_t              *previousp)
{
	ipc_port_t previous;
	mach_port_mscount_t mscount;
	require_ip_active(port);

	previous = port->ip_nsrequest;
	mscount = port->ip_mscount;

	if ((port->ip_srights == 0) && (sync <= mscount) &&
	    (notify != IP_NULL)) {
		port->ip_nsrequest = IP_NULL;
		ip_unlock(port);
		ipc_notify_no_senders(notify, mscount);
	} else {
		port->ip_nsrequest = notify;
		ip_unlock(port);
	}

	*previousp = previous;
}


  
                                   
           
                                                          
                                                                
  
              
                                  
           
                                                              
                                                                
                                                   
  
                                                                     
   

boolean_t
ipc_port_clear_receiver(
	ipc_port_t      port,
	boolean_t       should_destroy)
{
	ipc_mqueue_t    mqueue = &port->ip_messages;
	boolean_t       reap_messages = FALSE;

	  
                                                      
                                                                      
                                                                 
    
	if (port->ip_in_pset != 0) {
		ipc_pset_remove_from_all(port);
		assert(port->ip_in_pset == 0);
	}

	  
                                                          
                                             
    
	imq_lock(mqueue);
	if (port->ip_receiver_name) {
		ipc_mqueue_changed(port->ip_receiver, mqueue);
	} else {
		ipc_mqueue_changed(NULL, mqueue);
	}
	port->ip_mscount = 0;
	mqueue->imq_seqno = 0;
	port->ip_context = port->ip_guarded = port->ip_strict_guard = 0;
	  
                                                                         
                                     
    
	port->ip_immovable_receive = 0;

	if (should_destroy) {
		  
                                                                      
                                                              
                                                            
                                                           
                            
    
                                                                  
               
     
		port->ip_object.io_bits &= ~IO_BITS_ACTIVE;
		port->ip_timestamp = ipc_port_timestamp();
		reap_messages = ipc_mqueue_destroy_locked(mqueue);
	} else {
		                           
		port->ip_receiver_name = MACH_PORT_NULL;
		port->ip_destination = IP_NULL;
	}

	imq_unlock(&port->ip_messages);

	return reap_messages;
}

  
                         
           
                                       
                                       
  
                                                                          
   

void
ipc_port_init(
	ipc_port_t              port,
	ipc_space_t             space,
	ipc_port_init_flags_t   flags,
	mach_port_name_t        name)
{
	                                                     

	port->ip_receiver = space;
	port->ip_receiver_name = name;

	if (flags & IPC_PORT_INIT_MAKE_SEND_RIGHT) {
		port->ip_srights = 1;
		port->ip_mscount = 1;
	}

	if (flags & IPC_PORT_INIT_FILTER_MESSAGE) {
		port->ip_object.io_bits |= IP_BIT_FILTER_MSG;
	}

	port->ip_tg_block_tracking = (flags & IPC_PORT_INIT_TG_BLOCK_TRACKING) != 0;

	if (flags & IPC_PORT_INIT_SPECIAL_REPLY) {
		port->ip_specialreply = true;
		port->ip_immovable_receive = true;
	}

	port->ip_sync_link_state = PORT_SYNC_LINK_ANY;

	ipc_mqueue_kind_t kind = IPC_MQUEUE_KIND_NONE;
	if (flags & IPC_PORT_INIT_MESSAGE_QUEUE) {
		kind = IPC_MQUEUE_KIND_PORT;
	}
	ipc_mqueue_init(&port->ip_messages, kind);
}

  
                          
           
                    
              
                                                        
                                                   
           
                                        
                                        
                                                     
                                                    
   

kern_return_t
ipc_port_alloc(
	ipc_space_t             space,
	ipc_port_init_flags_t   flags,
	mach_port_name_t        *namep,
	ipc_port_t              *portp)
{
	ipc_port_t port;
	mach_port_name_t name;
	kern_return_t kr;
	mach_port_type_t type = MACH_PORT_TYPE_RECEIVE;
	mach_port_urefs_t urefs = 0;

#if     MACH_ASSERT
	uintptr_t buf[IP_CALLSTACK_MAX];
	ipc_port_callstack_init_debug(&buf[0], IP_CALLSTACK_MAX);
#endif                  

	if (flags & IPC_PORT_INIT_MAKE_SEND_RIGHT) {
		type |= MACH_PORT_TYPE_SEND;
		urefs = 1;
	}
	kr = ipc_object_alloc(space, IOT_PORT, type, urefs,
	    &name, (ipc_object_t *) &port);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	                               
	ipc_port_init(port, space, flags, name);

#if     MACH_ASSERT
	ipc_port_init_debug(port, &buf[0], IP_CALLSTACK_MAX);
#endif                   

	                             
	is_write_unlock(space);

	*namep = name;
	*portp = port;

	return KERN_SUCCESS;
}

  
                               
           
                                          
              
                                                        
                                                   
           
                                        
                                        
                                                      
                                                    
   

kern_return_t
ipc_port_alloc_name(
	ipc_space_t             space,
	ipc_port_init_flags_t   flags,
	mach_port_name_t        name,
	ipc_port_t              *portp)
{
	ipc_port_t port;
	kern_return_t kr;
	mach_port_type_t type = MACH_PORT_TYPE_RECEIVE;
	mach_port_urefs_t urefs = 0;

#if     MACH_ASSERT
	uintptr_t buf[IP_CALLSTACK_MAX];
	ipc_port_callstack_init_debug(&buf[0], IP_CALLSTACK_MAX);
#endif                  

	if (flags & IPC_PORT_INIT_MAKE_SEND_RIGHT) {
		type |= MACH_PORT_TYPE_SEND;
		urefs = 1;
	}
	kr = ipc_object_alloc_name(space, IOT_PORT, type, urefs,
	    name, (ipc_object_t *) &port);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	                    

	ipc_port_init(port, space, flags, name);

#if     MACH_ASSERT
	ipc_port_init_debug(port, &buf[0], IP_CALLSTACK_MAX);
#endif                   

	*portp = port;

	return KERN_SUCCESS;
}

  
                                  
           
                                              
              
                                           
   
void
ipc_port_spnotify(
	ipc_port_t      port)
{
	ipc_port_request_index_t index = 0;
	ipc_table_elems_t size = 0;

	  
                                            
                                         
    
	if (port->ip_sprequests == 0) {
		return;
	}

	ip_lock(port);

#if IMPORTANCE_INHERITANCE
	if (port->ip_spimportant != 0) {
		port->ip_spimportant = 0;
		if (ipc_port_importance_delta(port, IPID_OPTION_NORMAL, -1) == TRUE) {
			ip_lock(port);
		}
	}
#endif                             

	if (port->ip_sprequests == 0) {
		ip_unlock(port);
		return;
	}
	port->ip_sprequests = 0;

revalidate:
	if (ip_active(port)) {
		ipc_port_request_t requests;

		                                                       
		requests = port->ip_requests;
		assert(requests != IPR_NULL);

		  
                                               
                                                 
     
		if (size == 0) {
			size = requests->ipr_size->its_size;
		}

		                                 
		while (++index < size) {
			ipc_port_request_t ipr = &requests[index];
			mach_port_name_t name = ipr->ipr_name;
			ipc_port_t soright = IPR_SOR_PORT(ipr->ipr_soright);
			boolean_t armed = IPR_SOR_SPARMED(ipr->ipr_soright);

			if (MACH_PORT_VALID(name) && armed && IP_VALID(soright)) {
				                                              
				ipr->ipr_soright = IP_NULL;
				ip_unlock(port);

				ipc_notify_send_possible(soright, name);

				ip_lock(port);
				goto revalidate;
			}
		}
	}
	ip_unlock(port);
	return;
}

  
                                  
           
                                        
                                       
                      
              
                   
                          
                           
   
void
ipc_port_dnnotify(
	ipc_port_t      port)
{
	ipc_port_request_t requests = port->ip_requests;

	assert(!ip_active(port));
	if (requests != IPR_NULL) {
		ipc_table_size_t its = requests->ipr_size;
		ipc_table_elems_t size = its->its_size;
		ipc_port_request_index_t index;
		for (index = 1; index < size; index++) {
			ipc_port_request_t ipr = &requests[index];
			mach_port_name_t name = ipr->ipr_name;
			ipc_port_t soright = IPR_SOR_PORT(ipr->ipr_soright);

			if (MACH_PORT_VALID(name) && IP_VALID(soright)) {
				ipc_notify_dead_name(soright, name);
			}
		}
	}
}


  
                            
           
                                                
  
                                                       
                                                               
              
                                                      
                                                  
                                              
   

void
ipc_port_destroy(ipc_port_t port)
{
	ipc_port_t pdrequest, nsrequest;
	ipc_mqueue_t mqueue;
	ipc_kmsg_t kmsg;
	boolean_t special_reply = port->ip_specialreply;
	struct task_watchport_elem *watchport_elem = NULL;

#if IMPORTANCE_INHERITANCE
	ipc_importance_task_t release_imp_task = IIT_NULL;
	thread_t self = current_thread();
	boolean_t top = (self->ith_assertions == 0);
	natural_t assertcnt = 0;
#endif                             

	require_ip_active(port);
	                                       
	                                                       

	                                  
	port->ip_reply_context = 0;

	                             
	pdrequest = port->ip_pdrequest;

	  
                                                             
                                                            
                
    
	if (special_reply && (pdrequest || port->ip_tempowner)) {
		panic("ipc_port_destroy: invalid state");
	}

#if IMPORTANCE_INHERITANCE
	                                                         
	if (port->ip_tempowner != 0) {
		assert(top);
		release_imp_task = port->ip_imp_task;
		if (IIT_NULL != release_imp_task) {
			port->ip_imp_task = IIT_NULL;
			assertcnt = port->ip_impcount;
		}
		                                
	} else {
		assertcnt = port->ip_impcount;
		if (pdrequest != IP_NULL) {
			                                   
			port->ip_tempowner = 1;
		}
	}

	if (top) {
		self->ith_assertions = assertcnt;
	}
#endif                             

	if (pdrequest != IP_NULL) {
		                                            
		(void)ipc_port_clear_receiver(port, FALSE);
		assert(port->ip_in_pset == 0);
		assert(port->ip_mscount == 0);

		                                     
		port->ip_pdrequest = IP_NULL;

		imq_lock(&port->ip_messages);
		watchport_elem = ipc_port_clear_watchport_elem_internal(port);
		ipc_port_send_turnstile_recompute_push_locked(port);
		                              

		if (special_reply) {
			ipc_port_adjust_special_reply_port(port,
			    IPC_PORT_ADJUST_SR_ALLOW_SYNC_LINKAGE);
		}

		if (watchport_elem) {
			task_watchport_elem_deallocate(watchport_elem);
			watchport_elem = NULL;
		}
		                                              
		ipc_notify_port_destroyed(pdrequest, port);

		goto drop_assertions;
	}

	  
                                                                 
                                                                 
                                                                    
                                                            
                                                                    
                                                                   
                                                                
                                      
    
	boolean_t reap_msgs = FALSE;
	reap_msgs = ipc_port_clear_receiver(port, TRUE);                                     
	assert(port->ip_in_pset == 0);
	assert(port->ip_mscount == 0);

	imq_lock(&port->ip_messages);
	watchport_elem = ipc_port_clear_watchport_elem_internal(port);
	imq_unlock(&port->ip_messages);
	nsrequest = port->ip_nsrequest;

	  
                                                                 
                                                                 
                                                                     
                         
   
                                                                     
    
	if (IP_PREALLOC(port)) {
		ipc_port_t inuse_port;

		kmsg = port->ip_premsg;
		assert(kmsg != IKM_NULL);
		inuse_port = ikm_prealloc_inuse_port(kmsg);
		ipc_kmsg_clear_prealloc(kmsg, port);

		imq_lock(&port->ip_messages);
		ipc_port_send_turnstile_recompute_push_locked(port);
		                              

		if (inuse_port != IP_NULL) {
			assert(inuse_port == port);
		} else {
			ipc_kmsg_free(kmsg);
		}
	} else {
		imq_lock(&port->ip_messages);
		ipc_port_send_turnstile_recompute_push_locked(port);
		                              
	}

	                                      
	if (watchport_elem) {
		task_watchport_elem_deallocate(watchport_elem);
		watchport_elem = NULL;
	}

	                                             
	if (special_reply) {
		ipc_port_adjust_special_reply_port(port,
		    IPC_PORT_ADJUST_SR_ALLOW_SYNC_LINKAGE);
	}

	                                   
	if (nsrequest != IP_NULL) {
		ipc_notify_send_once(nsrequest);                   
	}
	  
                                                  
                                                         
    
	if (reap_msgs) {
		ipc_kmsg_reap_delayed();
	}

	mqueue = &port->ip_messages;

	                                     
	ipc_mqueue_deinit(mqueue);

	                                      
	ipc_port_dnnotify(port);

	ipc_kobject_destroy(port);

	ip_release(port);                           

drop_assertions:
#if IMPORTANCE_INHERITANCE
	if (release_imp_task != IIT_NULL) {
		if (assertcnt > 0) {
			assert(top);
			self->ith_assertions = 0;
			assert(ipc_importance_task_is_any_receiver_type(release_imp_task));
			ipc_importance_task_drop_internal_assertion(release_imp_task, assertcnt);
		}
		ipc_importance_task_release(release_imp_task);
	} else if (assertcnt > 0) {
		if (top) {
			self->ith_assertions = 0;
			release_imp_task = current_task()->task_imp_base;
			if (ipc_importance_task_is_any_receiver_type(release_imp_task)) {
				ipc_importance_task_drop_internal_assertion(release_imp_task, assertcnt);
			}
		}
	}
#endif                             
}

  
                                           
           
                                                            
                                                           
  
                                                   
  
              
                    
                                
   
boolean_t
ipc_port_destination_chain_lock(
	ipc_port_t port,
	ipc_port_t *base)
{
	for (;;) {
		ip_lock(port);

		if (!ip_active(port)) {
			  
                                                       
     
                                              
                                                
                                                     
                                             
     
                                                   
                                                    
                                                 
     
                                                    
                                   
      
			ip_reference(port);
			*base = port;
			return true;
		}
		if ((port->ip_receiver_name != MACH_PORT_NULL) ||
		    (port->ip_destination == IP_NULL)) {
			*base = port;
			return false;
		}

		port = port->ip_destination;
	}
}


  
                                      
           
                                                    
                                                        
  
                                                   
                                               
  
                                                         
                                                      
                                                     
  
              
                                                            
   

boolean_t
ipc_port_check_circularity(
	ipc_port_t      port,
	ipc_port_t      dest)
{
#if IMPORTANCE_INHERITANCE
	                                               
	return ipc_importance_check_circularity(port, dest);
#else
	ipc_port_t base;
	struct task_watchport_elem *watchport_elem = NULL;
	bool took_base_ref = false;

	assert(port != IP_NULL);
	assert(dest != IP_NULL);

	if (port == dest) {
		return TRUE;
	}
	base = dest;

	                                            
	ipc_port_send_turnstile_prepare(dest);

	  
                                                     
                                             
    
	ip_lock(port);
	if (ip_lock_try(dest)) {
		if (!ip_active(dest) ||
		    (dest->ip_receiver_name != MACH_PORT_NULL) ||
		    (dest->ip_destination == IP_NULL)) {
			goto not_circular;
		}

		                                                    

		ip_unlock(dest);
	}
	ip_unlock(port);

	ipc_port_multiple_lock();                            

	  
                                                            
                                  
    

	took_base_ref = ipc_port_destination_chain_lock(dest, &base);
	                                                                 

	if (port == base) {
		                           

		ipc_port_multiple_unlock();

		                                
		require_ip_active(port);
		assert(port->ip_receiver_name == MACH_PORT_NULL);
		assert(port->ip_destination == IP_NULL);
		assert(!took_base_ref);

		base = dest;
		while (base != IP_NULL) {
			ipc_port_t next;

			                                    
			require_ip_active(base);
			assert(base->ip_receiver_name == MACH_PORT_NULL);

			next = base->ip_destination;
			ip_unlock(base);
			base = next;
		}

		ipc_port_send_turnstile_complete(dest);
		return TRUE;
	}

	  
                                                               
                                                         
                                                 
    

	ip_lock(port);
	ipc_port_multiple_unlock();

not_circular:
	imq_lock(&port->ip_messages);

	                      
	require_ip_active(port);
	assert(port->ip_receiver_name == MACH_PORT_NULL);
	assert(port->ip_destination == IP_NULL);

	                               
	watchport_elem = ipc_port_clear_watchport_elem_internal(port);

	                                                                             
	if (dest->ip_specialreply && dest->ip_sync_bootstrap_checkin) {
		port->ip_sync_bootstrap_checkin = 1;
	}

	ip_reference(dest);
	port->ip_destination = dest;

	                                                           
	struct turnstile *send_turnstile = TURNSTILE_NULL;
	if (port_send_turnstile(port)) {
		send_turnstile = turnstile_prepare((uintptr_t)port,
		    port_send_turnstile_address(port),
		    TURNSTILE_NULL, TURNSTILE_SYNC_IPC);

		  
                                               
                                                    
                                                   
     
		ipc_port_adjust_sync_link_state_locked(port, PORT_SYNC_LINK_ANY, NULL);

		turnstile_update_inheritor(send_turnstile, port_send_turnstile(dest),
		    (TURNSTILE_INHERITOR_TURNSTILE | TURNSTILE_IMMEDIATE_UPDATE));

		                                                                            
	}
	imq_unlock(&port->ip_messages);

	                      

	ip_unlock(port);

	for (;;) {
		ipc_port_t next;

		if (dest == base) {
			break;
		}

		                        
		require_ip_active(dest);
		assert(dest->ip_receiver_name == MACH_PORT_NULL);
		assert(dest->ip_destination != IP_NULL);

		next = dest->ip_destination;
		ip_unlock(dest);
		dest = next;
	}

	                            
	assert(!ip_active(base) ||
	    (base->ip_receiver_name != MACH_PORT_NULL) ||
	    (base->ip_destination == IP_NULL));

	ip_unlock(base);
	if (took_base_ref) {
		ip_release(base);
	}

	                                                                                             
	if (send_turnstile) {
		turnstile_update_inheritor_complete(send_turnstile, TURNSTILE_INTERLOCK_NOT_HELD);

		                                                 
		imq_lock(&port->ip_messages);
		turnstile_complete((uintptr_t)port, port_send_turnstile_address(port), NULL, TURNSTILE_SYNC_IPC);
		send_turnstile = TURNSTILE_NULL;
		imq_unlock(&port->ip_messages);
		turnstile_cleanup();
	}

	if (watchport_elem) {
		task_watchport_elem_deallocate(watchport_elem);
	}

	return FALSE;
#endif                              
}

  
                                   
           
                                       
  
              
                 
   
static struct task_watchport_elem *
ipc_port_watchport_elem(ipc_port_t port)
{
	return port->ip_messages.imq_wait_queue.waitq_tspriv;
}

  
                                          
           
                                       
  
              
                 
   
static inline struct task_watchport_elem *
ipc_port_update_watchport_elem(ipc_port_t port, struct task_watchport_elem *we)
{
	assert(!port->ip_specialreply);
	struct task_watchport_elem *old_we = ipc_port_watchport_elem(port);
	port->ip_messages.imq_wait_queue.waitq_tspriv = we;
	return old_we;
}

  
                                              
           
                                                                                 
  
              
               
   
static inline void
ipc_special_reply_stash_pid_locked(ipc_port_t port, int pid)
{
	assert(port->ip_specialreply);
	port->ip_messages.imq_wait_queue.waitq_priv_pid = pid;
	return;
}

  
                                            
           
                                                                                 
  
              
               
   
int
ipc_special_reply_get_pid_locked(ipc_port_t port)
{
	assert(port->ip_specialreply);
	return port->ip_messages.imq_wait_queue.waitq_priv_pid;
}

  
                                                  
  
                                                                           
                                           
  
                                                                              
                         
  
                                                                                
                                        
  
                                                                                
                                            
   
void
ipc_port_recv_update_inheritor(
	ipc_port_t port,
	struct turnstile *rcv_turnstile,
	turnstile_update_flags_t flags)
{
	struct turnstile *inheritor = TURNSTILE_NULL;
	struct knote *kn;

	if (ip_active(port) && port->ip_specialreply) {
		imq_held(&port->ip_messages);

		switch (port->ip_sync_link_state) {
		case PORT_SYNC_LINK_PORT:
			if (port->ip_sync_inheritor_port != NULL) {
				inheritor = port_send_turnstile(port->ip_sync_inheritor_port);
			}
			break;

		case PORT_SYNC_LINK_WORKLOOP_KNOTE:
			kn = port->ip_sync_inheritor_knote;
			inheritor = filt_ipc_kqueue_turnstile(kn);
			break;

		case PORT_SYNC_LINK_WORKLOOP_STASH:
			inheritor = port->ip_sync_inheritor_ts;
			break;
		}
	}

	turnstile_update_inheritor(rcv_turnstile, inheritor,
	    flags | TURNSTILE_INHERITOR_TURNSTILE);
}

  
                                                  
  
                                                                                
  
                                                                              
                                           
  
                                                                            
                         
  
                                                                             
                  
  
                                                                              
                                                                              
                                 
  
                                                                              
                                                    
                                                               
  
                                                                             
                                                                   
  
                                                                          
                                                                             
                                                                              
              
   
void
ipc_port_send_update_inheritor(
	ipc_port_t port,
	struct turnstile *send_turnstile,
	turnstile_update_flags_t flags)
{
	ipc_mqueue_t mqueue = &port->ip_messages;
	turnstile_inheritor_t inheritor = TURNSTILE_INHERITOR_NULL;
	struct knote *kn;
	turnstile_update_flags_t inheritor_flags = TURNSTILE_INHERITOR_TURNSTILE;

	imq_held(mqueue);

	if (!ip_active(port)) {
		                                                                
	} else if (port->ip_specialreply) {
		             
		if (port->ip_sync_bootstrap_checkin && prioritize_launch) {
			inheritor = port->ip_messages.imq_srp_owner_thread;
			inheritor_flags = TURNSTILE_INHERITOR_THREAD;
		}
	} else if (port->ip_receiver_name == MACH_PORT_NULL &&
	    port->ip_destination != NULL) {
		             
		inheritor = port_send_turnstile(port->ip_destination);
	} else if (ipc_port_watchport_elem(port) != NULL) {
		             
		if (prioritize_launch) {
			assert(port->ip_sync_link_state == PORT_SYNC_LINK_ANY);
			inheritor = ipc_port_get_watchport_inheritor(port);
			inheritor_flags = TURNSTILE_INHERITOR_THREAD;
		}
	} else if (port->ip_sync_link_state == PORT_SYNC_LINK_WORKLOOP_KNOTE) {
		             
		inheritor = filt_ipc_kqueue_turnstile(mqueue->imq_inheritor_knote);
	} else if (port->ip_sync_link_state == PORT_SYNC_LINK_WORKLOOP_STASH) {
		             
		inheritor = mqueue->imq_inheritor_turnstile;
	} else if (port->ip_sync_link_state == PORT_SYNC_LINK_RCV_THREAD) {
		             
		if (prioritize_launch) {
			inheritor = port->ip_messages.imq_inheritor_thread_ref;
			inheritor_flags = TURNSTILE_INHERITOR_THREAD;
		}
	} else if ((kn = SLIST_FIRST(&mqueue->imq_klist))) {
		                                                   
		if (filt_machport_kqueue_has_turnstile(kn)) {
			assert(port->ip_sync_link_state == PORT_SYNC_LINK_ANY);
			inheritor = filt_ipc_kqueue_turnstile(kn);
		}
	}

	turnstile_update_inheritor(send_turnstile, inheritor,
	    flags | inheritor_flags);
}

  
                                           
           
                                                
                                                          
  
              
                      
   
void
ipc_port_send_turnstile_prepare(ipc_port_t port)
{
	struct turnstile *turnstile = TURNSTILE_NULL;
	struct turnstile *send_turnstile = TURNSTILE_NULL;

retry_alloc:
	imq_lock(&port->ip_messages);

	if (port_send_turnstile(port) == NULL ||
	    port_send_turnstile(port)->ts_port_ref == 0) {
		if (turnstile == TURNSTILE_NULL) {
			imq_unlock(&port->ip_messages);
			turnstile = turnstile_alloc();
			goto retry_alloc;
		}

		send_turnstile = turnstile_prepare((uintptr_t)port,
		    port_send_turnstile_address(port),
		    turnstile, TURNSTILE_SYNC_IPC);
		turnstile = TURNSTILE_NULL;

		ipc_port_send_update_inheritor(port, send_turnstile,
		    TURNSTILE_IMMEDIATE_UPDATE);

		                                                                           
	}

	                                 
	port_send_turnstile(port)->ts_port_ref++;
	imq_unlock(&port->ip_messages);

	if (send_turnstile) {
		turnstile_update_inheritor_complete(send_turnstile,
		    TURNSTILE_INTERLOCK_NOT_HELD);
	}
	if (turnstile != TURNSTILE_NULL) {
		turnstile_deallocate(turnstile);
	}
}


  
                                            
           
                                                   
                                               
  
              
                                                   
   
void
ipc_port_send_turnstile_complete(ipc_port_t port)
{
	struct turnstile *turnstile = TURNSTILE_NULL;

	                                       
	imq_lock(&port->ip_messages);

	port_send_turnstile(port)->ts_port_ref--;
	if (port_send_turnstile(port)->ts_port_ref == 0) {
		turnstile_complete((uintptr_t)port, port_send_turnstile_address(port),
		    &turnstile, TURNSTILE_SYNC_IPC);
		assert(turnstile != TURNSTILE_NULL);
	}
	imq_unlock(&port->ip_messages);
	turnstile_cleanup();

	if (turnstile != TURNSTILE_NULL) {
		turnstile_deallocate_safe(turnstile);
		turnstile = TURNSTILE_NULL;
	}
}

  
                                  
           
                                    
  
              
                                                           
   
static struct turnstile *
ipc_port_rcv_turnstile(ipc_port_t port)
{
	return *port_rcv_turnstile_address(port);
}


  
                                            
           
                                                          
                                                 
  
              
                      
   
void
ipc_port_link_special_reply_port(
	ipc_port_t special_reply_port,
	ipc_port_t dest_port,
	boolean_t sync_bootstrap_checkin)
{
	boolean_t drop_turnstile_ref = FALSE;
	boolean_t special_reply = FALSE;

	                                          
	ipc_port_send_turnstile_prepare(dest_port);

	                                                           
	ip_lock(special_reply_port);
	imq_lock(&special_reply_port->ip_messages);

	special_reply = special_reply_port->ip_specialreply;

	if (sync_bootstrap_checkin && special_reply) {
		special_reply_port->ip_sync_bootstrap_checkin = 1;
	}

	                                                                      
	if (!special_reply ||
	    special_reply_port->ip_sync_link_state != PORT_SYNC_LINK_ANY ||
	    special_reply_port->ip_sync_inheritor_port != IPC_PORT_NULL) {
		drop_turnstile_ref = TRUE;
	} else {
		                                   
		ip_reference(dest_port);
		special_reply_port->ip_sync_inheritor_port = dest_port;
		special_reply_port->ip_sync_link_state = PORT_SYNC_LINK_PORT;
	}

	imq_unlock(&special_reply_port->ip_messages);
	ip_unlock(special_reply_port);

	if (special_reply) {
		  
                                                        
                                                        
                                           
     
		ipc_port_thread_group_blocked(dest_port);
	}

	if (drop_turnstile_ref) {
		ipc_port_send_turnstile_complete(dest_port);
	}

	return;
}

  
                                         
           
                                                 
                                                          
                                             
  
              
                      
   
void
ipc_port_thread_group_blocked(ipc_port_t port __unused)
{
#if CONFIG_THREAD_GROUPS
	bool port_tg_block_tracking = false;
	thread_t self = current_thread();

	if (self->thread_group == NULL ||
	    (self->options & TH_OPT_IPC_TG_BLOCKED)) {
		return;
	}

	port_tg_block_tracking = port->ip_tg_block_tracking;
	if (!port_tg_block_tracking) {
		return;
	}

	machine_thread_group_blocked(self->thread_group, NULL,
	    PERFCONTROL_CALLOUT_BLOCKING_TG_RENDER_SERVER, self);

	self->options |= TH_OPT_IPC_TG_BLOCKED;
#endif
}

  
                                           
           
                                              
                                                     
                                                      
                        
  
              
                      
   
void
ipc_port_thread_group_unblocked(void)
{
#if CONFIG_THREAD_GROUPS
	thread_t self = current_thread();

	if (!(self->options & TH_OPT_IPC_TG_BLOCKED)) {
		return;
	}

	machine_thread_group_unblocked(self->thread_group, NULL,
	    PERFCONTROL_CALLOUT_BLOCKING_TG_RENDER_SERVER, self);

	self->options &= ~TH_OPT_IPC_TG_BLOCKED;
#endif
}

#if DEVELOPMENT || DEBUG
inline void
ipc_special_reply_port_bits_reset(ipc_port_t special_reply_port)
{
	special_reply_port->ip_srp_lost_link = 0;
	special_reply_port->ip_srp_msg_sent = 0;
}

static inline void
ipc_special_reply_port_msg_sent_reset(ipc_port_t special_reply_port)
{
	if (special_reply_port->ip_specialreply == 1) {
		special_reply_port->ip_srp_msg_sent = 0;
	}
}

inline void
ipc_special_reply_port_msg_sent(ipc_port_t special_reply_port)
{
	if (special_reply_port->ip_specialreply == 1) {
		special_reply_port->ip_srp_msg_sent = 1;
	}
}

static inline void
ipc_special_reply_port_lost_link(ipc_port_t special_reply_port)
{
	if (special_reply_port->ip_specialreply == 1 && special_reply_port->ip_srp_msg_sent == 0) {
		special_reply_port->ip_srp_lost_link = 1;
	}
}

#else                           
inline void
ipc_special_reply_port_bits_reset(__unused ipc_port_t special_reply_port)
{
	return;
}

static inline void
ipc_special_reply_port_msg_sent_reset(__unused ipc_port_t special_reply_port)
{
	return;
}

inline void
ipc_special_reply_port_msg_sent(__unused ipc_port_t special_reply_port)
{
	return;
}

static inline void
ipc_special_reply_port_lost_link(__unused ipc_port_t special_reply_port)
{
	return;
}
#endif                           

  
                                                     
           
                                                              
             
                                       
                                          
                                               
           
         
   
void
ipc_port_adjust_special_reply_port_locked(
	ipc_port_t special_reply_port,
	struct knote *kn,
	uint8_t flags,
	boolean_t get_turnstile)
{
	ipc_port_t dest_port = IPC_PORT_NULL;
	int sync_link_state = PORT_SYNC_LINK_NO_LINKAGE;
	turnstile_inheritor_t inheritor = TURNSTILE_INHERITOR_NULL;
	struct turnstile *ts = TURNSTILE_NULL;

	ip_lock_held(special_reply_port);                                 
	imq_lock(&special_reply_port->ip_messages);

	if (!special_reply_port->ip_specialreply) {
		                                                                    
		assert(get_turnstile);
		goto not_special;
	}

	if (flags & IPC_PORT_ADJUST_SR_RECEIVED_MSG) {
		ipc_special_reply_port_msg_sent_reset(special_reply_port);
	}

	if (flags & IPC_PORT_ADJUST_UNLINK_THREAD) {
		special_reply_port->ip_messages.imq_srp_owner_thread = NULL;
	}

	if (flags & IPC_PORT_ADJUST_RESET_BOOSTRAP_CHECKIN) {
		special_reply_port->ip_sync_bootstrap_checkin = 0;
	}

	                                                           
	if (special_reply_port->ip_sync_link_state == PORT_SYNC_LINK_ANY) {
not_special:
		if (get_turnstile) {
			turnstile_complete((uintptr_t)special_reply_port,
			    port_rcv_turnstile_address(special_reply_port), NULL, TURNSTILE_SYNC_IPC);
		}
		imq_unlock(&special_reply_port->ip_messages);
		ip_unlock(special_reply_port);
		if (get_turnstile) {
			turnstile_cleanup();
		}
		return;
	}

	if (flags & IPC_PORT_ADJUST_SR_LINK_WORKLOOP) {
		if (ITH_KNOTE_VALID(kn, MACH_MSG_TYPE_PORT_SEND_ONCE)) {
			inheritor = filt_machport_stash_port(kn, special_reply_port,
			    &sync_link_state);
		}
	} else if (flags & IPC_PORT_ADJUST_SR_ALLOW_SYNC_LINKAGE) {
		sync_link_state = PORT_SYNC_LINK_ANY;
	}

	                                    
	if (!get_turnstile && sync_link_state == PORT_SYNC_LINK_NO_LINKAGE &&
	    special_reply_port->ip_sync_link_state == PORT_SYNC_LINK_NO_LINKAGE) {
		imq_unlock(&special_reply_port->ip_messages);
		ip_unlock(special_reply_port);
		return;
	}

	switch (special_reply_port->ip_sync_link_state) {
	case PORT_SYNC_LINK_PORT:
		dest_port = special_reply_port->ip_sync_inheritor_port;
		special_reply_port->ip_sync_inheritor_port = IPC_PORT_NULL;
		break;
	case PORT_SYNC_LINK_WORKLOOP_KNOTE:
		special_reply_port->ip_sync_inheritor_knote = NULL;
		break;
	case PORT_SYNC_LINK_WORKLOOP_STASH:
		special_reply_port->ip_sync_inheritor_ts = NULL;
		break;
	}

	  
                                                                       
                                                                           
                  
    
	if (special_reply_port->ip_sync_link_state == PORT_SYNC_LINK_PORT &&
	    sync_link_state == PORT_SYNC_LINK_NO_LINKAGE) {
		ipc_special_reply_stash_pid_locked(special_reply_port, pid_from_task(current_task()));
	} else if (special_reply_port->ip_sync_link_state == PORT_SYNC_LINK_NO_LINKAGE &&
	    sync_link_state == PORT_SYNC_LINK_ANY) {
		                                                                         
		ipc_special_reply_stash_pid_locked(special_reply_port, 0);
	}

	special_reply_port->ip_sync_link_state = sync_link_state;

	switch (sync_link_state) {
	case PORT_SYNC_LINK_WORKLOOP_KNOTE:
		special_reply_port->ip_sync_inheritor_knote = kn;
		break;
	case PORT_SYNC_LINK_WORKLOOP_STASH:
		special_reply_port->ip_sync_inheritor_ts = inheritor;
		break;
	case PORT_SYNC_LINK_NO_LINKAGE:
		if (flags & IPC_PORT_ADJUST_SR_ENABLE_EVENT) {
			ipc_special_reply_port_lost_link(special_reply_port);
		}
		break;
	}

	                                                          
	if (get_turnstile) {
		turnstile_complete((uintptr_t)special_reply_port,
		    port_rcv_turnstile_address(special_reply_port), NULL, TURNSTILE_SYNC_IPC);
	} else {
		ts = ipc_port_rcv_turnstile(special_reply_port);
		if (ts) {
			turnstile_reference(ts);
			ipc_port_recv_update_inheritor(special_reply_port, ts,
			    TURNSTILE_IMMEDIATE_UPDATE);
		}
	}

	imq_unlock(&special_reply_port->ip_messages);
	ip_unlock(special_reply_port);

	if (get_turnstile) {
		turnstile_cleanup();
	} else if (ts) {
		                                                         
		turnstile_update_inheritor_complete(ts, TURNSTILE_INTERLOCK_NOT_HELD);
		turnstile_deallocate_safe(ts);
	}

	                                                        
	if (dest_port) {
		ipc_port_send_turnstile_complete(dest_port);
		                                            
		ip_release(dest_port);
	}
}

  
                                              
           
                                                              
             
                   
           
         
   
void
ipc_port_adjust_special_reply_port(
	ipc_port_t port,
	uint8_t flags)
{
	if (port->ip_specialreply) {
		ip_lock(port);
		ipc_port_adjust_special_reply_port_locked(port, NULL, flags, FALSE);
	}
}

  
                                                  
           
                                                  
                        
             
                                    
                                     
           
                     
   
void
ipc_port_adjust_sync_link_state_locked(
	ipc_port_t port,
	int sync_link_state,
	turnstile_inheritor_t inheritor)
{
	switch (port->ip_sync_link_state) {
	case PORT_SYNC_LINK_RCV_THREAD:
		                                                       
		thread_deallocate_safe(port->ip_messages.imq_inheritor_thread_ref);
		OS_FALLTHROUGH;
	default:
		klist_init(&port->ip_messages.imq_klist);
	}

	switch (sync_link_state) {
	case PORT_SYNC_LINK_WORKLOOP_KNOTE:
		port->ip_messages.imq_inheritor_knote = inheritor;
		break;
	case PORT_SYNC_LINK_WORKLOOP_STASH:
		port->ip_messages.imq_inheritor_turnstile = inheritor;
		break;
	case PORT_SYNC_LINK_RCV_THREAD:
		                                                                          
		thread_reference((thread_t)inheritor);
		port->ip_messages.imq_inheritor_thread_ref = inheritor;
		break;
	default:
		klist_init(&port->ip_messages.imq_klist);
		sync_link_state = PORT_SYNC_LINK_ANY;
	}

	port->ip_sync_link_state = sync_link_state;
}


  
                                       
           
                                                      
             
                         
                            
           
         
   
void
ipc_port_adjust_port_locked(
	ipc_port_t port,
	struct knote *kn,
	boolean_t sync_bootstrap_checkin)
{
	int sync_link_state = PORT_SYNC_LINK_ANY;
	turnstile_inheritor_t inheritor = TURNSTILE_INHERITOR_NULL;

	ip_lock_held(port);                                 
	imq_held(&port->ip_messages);

	assert(!port->ip_specialreply);

	if (kn) {
		inheritor = filt_machport_stash_port(kn, port, &sync_link_state);
		if (sync_link_state == PORT_SYNC_LINK_WORKLOOP_KNOTE) {
			inheritor = kn;
		}
	} else if (sync_bootstrap_checkin) {
		inheritor = current_thread();
		sync_link_state = PORT_SYNC_LINK_RCV_THREAD;
	}

	ipc_port_adjust_sync_link_state_locked(port, sync_link_state, inheritor);
	port->ip_sync_bootstrap_checkin = 0;

	ipc_port_send_turnstile_recompute_push_locked(port);
	                              
}

  
                                                       
           
                                                   
             
                        
                         
                            
           
         
   
void
ipc_port_clear_sync_rcv_thread_boost_locked(
	ipc_port_t port)
{
	ip_lock_held(port);                                 

	if (port->ip_sync_link_state != PORT_SYNC_LINK_RCV_THREAD) {
		ip_unlock(port);
		return;
	}

	imq_lock(&port->ip_messages);
	ipc_port_adjust_sync_link_state_locked(port, PORT_SYNC_LINK_ANY, NULL);

	ipc_port_send_turnstile_recompute_push_locked(port);
	                              
}

  
                                              
           
                                                                   
             
                         
                            
           
                           
                           
   
kern_return_t
ipc_port_add_watchport_elem_locked(
	ipc_port_t                 port,
	struct task_watchport_elem *watchport_elem,
	struct task_watchport_elem **old_elem)
{
	ip_lock_held(port);
	imq_held(&port->ip_messages);

	                                                                                    
	if (!ip_active(port) || port->ip_specialreply ||
	    port->ip_receiver_name == MACH_PORT_NULL) {
		imq_unlock(&port->ip_messages);
		ip_unlock(port);
		return KERN_FAILURE;
	}

	if (port->ip_sync_link_state != PORT_SYNC_LINK_ANY) {
		                                                        
		ipc_port_adjust_sync_link_state_locked(port, PORT_SYNC_LINK_ANY, NULL);
	}

	*old_elem = ipc_port_update_watchport_elem(port, watchport_elem);

	ipc_port_send_turnstile_recompute_push_locked(port);
	                              
	return KERN_SUCCESS;
}

  
                                                                     
           
                                                                   
             
                         
                            
           
                           
                           
   
kern_return_t
ipc_port_clear_watchport_elem_internal_conditional_locked(
	ipc_port_t                 port,
	struct task_watchport_elem *watchport_elem)
{
	ip_lock_held(port);
	imq_held(&port->ip_messages);

	if (ipc_port_watchport_elem(port) != watchport_elem) {
		imq_unlock(&port->ip_messages);
		ip_unlock(port);
		return KERN_FAILURE;
	}

	ipc_port_clear_watchport_elem_internal(port);
	ipc_port_send_turnstile_recompute_push_locked(port);
	                              
	return KERN_SUCCESS;
}

  
                                                              
           
                                                                    
             
                         
                            
           
                           
                           
   
kern_return_t
ipc_port_replace_watchport_elem_conditional_locked(
	ipc_port_t                 port,
	struct task_watchport_elem *old_watchport_elem,
	struct task_watchport_elem *new_watchport_elem)
{
	ip_lock_held(port);
	imq_held(&port->ip_messages);

	if (ipc_port_watchport_elem(port) != old_watchport_elem) {
		imq_unlock(&port->ip_messages);
		ip_unlock(port);
		return KERN_FAILURE;
	}

	ipc_port_update_watchport_elem(port, new_watchport_elem);
	ipc_port_send_turnstile_recompute_push_locked(port);
	                              
	return KERN_SUCCESS;
}

  
                                                  
           
                                            
             
                         
                          
           
                                     
   
struct task_watchport_elem *
ipc_port_clear_watchport_elem_internal(
	ipc_port_t                 port)
{
	ip_lock_held(port);
	imq_held(&port->ip_messages);

	if (port->ip_specialreply) {
		return NULL;
	}

	return ipc_port_update_watchport_elem(port, NULL);
}

  
                                                         
           
                                                                   
             
                         
                            
           
         
   
static void
ipc_port_send_turnstile_recompute_push_locked(
	ipc_port_t port)
{
	struct turnstile *send_turnstile = port_send_turnstile(port);
	if (send_turnstile) {
		turnstile_reference(send_turnstile);
		ipc_port_send_update_inheritor(port, send_turnstile,
		    TURNSTILE_IMMEDIATE_UPDATE);
	}
	imq_unlock(&port->ip_messages);
	ip_unlock(port);

	if (send_turnstile) {
		turnstile_update_inheritor_complete(send_turnstile,
		    TURNSTILE_INTERLOCK_NOT_HELD);
		turnstile_deallocate_safe(send_turnstile);
	}
}

  
                                            
           
                                    
  
              
                  
           
                        
   
static thread_t
ipc_port_get_watchport_inheritor(
	ipc_port_t port)
{
	imq_held(&port->ip_messages);
	return ipc_port_watchport_elem(port)->twe_task->watchports->tw_thread;
}

  
                                      
           
                                                                
  
              
                   
   
pid_t
ipc_port_get_receiver_task(ipc_port_t port, uintptr_t *task)
{
	task_t receiver = TASK_NULL;
	pid_t pid = -1;

	if (!port) {
		goto out;
	}

	ip_lock(port);
	if (ip_active(port) &&
	    MACH_PORT_VALID(port->ip_receiver_name) &&
	    port->ip_receiver &&
	    port->ip_receiver != ipc_space_kernel &&
	    port->ip_receiver != ipc_space_reply) {
		receiver = port->ip_receiver->is_task;
		pid = task_pid(receiver);
	}
	ip_unlock(port);

out:
	if (task) {
		*task = (uintptr_t)receiver;
	}
	return pid;
}

  
                                   
           
                                                            
                                                             
                                
  
                                                            
                                                            
                                                            
                                         
              
                                      
                           
   
mach_port_delta_t
ipc_port_impcount_delta(
	ipc_port_t        port,
	mach_port_delta_t delta,
	ipc_port_t        __unused base)
{
	mach_port_delta_t absdelta;

	if (!ip_active(port)) {
		return 0;
	}

	                                  
	if (delta >= 0) {
		port->ip_impcount += delta;
		return delta;
	}

	absdelta = 0 - delta;
	if (port->ip_impcount >= absdelta) {
		port->ip_impcount -= absdelta;
		return delta;
	}

#if (DEVELOPMENT || DEBUG)
	if (port->ip_receiver_name != MACH_PORT_NULL) {
		task_t target_task = port->ip_receiver->is_task;
		ipc_importance_task_t target_imp = target_task->task_imp_base;
		const char *target_procname;
		int target_pid;

		if (target_imp != IIT_NULL) {
			target_procname = target_imp->iit_procname;
			target_pid = target_imp->iit_bsd_pid;
		} else {
			target_procname = "unknown";
			target_pid = -1;
		}
		printf("Over-release of importance assertions for port 0x%x receiver pid %d (%s), "
		    "dropping %d assertion(s) but port only has %d remaining.\n",
		    port->ip_receiver_name,
		    target_pid, target_procname,
		    absdelta, port->ip_impcount);
	} else if (base != IP_NULL) {
		task_t target_task = base->ip_receiver->is_task;
		ipc_importance_task_t target_imp = target_task->task_imp_base;
		const char *target_procname;
		int target_pid;

		if (target_imp != IIT_NULL) {
			target_procname = target_imp->iit_procname;
			target_pid = target_imp->iit_bsd_pid;
		} else {
			target_procname = "unknown";
			target_pid = -1;
		}
		printf("Over-release of importance assertions for port 0x%lx "
		    "enqueued on port 0x%x with receiver pid %d (%s), "
		    "dropping %d assertion(s) but port only has %d remaining.\n",
		    (unsigned long)VM_KERNEL_UNSLIDE_OR_PERM((uintptr_t)port),
		    base->ip_receiver_name,
		    target_pid, target_procname,
		    absdelta, port->ip_impcount);
	}
#endif

	delta = 0 - port->ip_impcount;
	port->ip_impcount = 0;
	return delta;
}

  
                                              
           
                                                       
                                                         
                                                      
                                                      
                               
              
                                               
                             
                           
                                    
                                     
   
#if IMPORTANCE_INHERITANCE

boolean_t
ipc_port_importance_delta_internal(
	ipc_port_t              port,
	natural_t               options,
	mach_port_delta_t       *deltap,
	ipc_importance_task_t   *imp_task)
{
	ipc_port_t next, base;
	bool dropped = false;
	bool took_base_ref = false;

	*imp_task = IIT_NULL;

	if (*deltap == 0) {
		return FALSE;
	}

	assert(options == IPID_OPTION_NORMAL || options == IPID_OPTION_SENDPOSSIBLE);

	base = port;

	                                                            
	if (ip_active(port) &&
	    port->ip_destination != IP_NULL &&
	    port->ip_receiver_name == MACH_PORT_NULL) {
		dropped = true;

		ip_unlock(port);
		ipc_port_multiple_lock();                            

		took_base_ref = ipc_port_destination_chain_lock(port, &base);
		                                                                 

		ipc_port_multiple_unlock();
	}

	  
                                                                      
                                                                     
                                                  
   
                                                                      
                                                                  
                                                                      
                                                                
                                 
    
	if (options & IPID_OPTION_SENDPOSSIBLE) {
		assert(*deltap == 1);
		if (port->ip_sprequests && port->ip_spimportant == 0) {
			port->ip_spimportant = 1;
		} else {
			*deltap = 0;
		}
	}

	                                                               
	for (;;) {
		*deltap = ipc_port_impcount_delta(port, *deltap, base);

		if (port == base) {
			break;
		}

		                        
		assert(port->ip_tempowner == 0);
		next = port->ip_destination;
		ip_unlock(port);
		port = next;
	}

	                                                           
	if (ip_active(base)) {
		if (base->ip_tempowner != 0) {
			if (IIT_NULL != base->ip_imp_task) {
				*imp_task = base->ip_imp_task;
			}
			                           
		} else if (base->ip_receiver_name != MACH_PORT_NULL) {
			ipc_space_t space = base->ip_receiver;

			                                            
			if (space->is_task != TASK_NULL &&
			    ipc_importance_task_is_any_receiver_type(space->is_task->task_imp_base)) {
				*imp_task = space->is_task->task_imp_base;
			}
		}
	}

	  
                                                             
                                                                
    
	if (*imp_task != IIT_NULL) {
		                                            
		ipc_importance_task_reference(*imp_task);
	}

	if (dropped) {
		ip_unlock(base);
		if (took_base_ref) {
			ip_release(base);
		}
	}

	return dropped;
}
#endif                             

  
                                     
           
                                                       
                                                         
              
  
                                                                
                                    
              
                                               
                           
                                    
                                     
   
#if IMPORTANCE_INHERITANCE

boolean_t
ipc_port_importance_delta(
	ipc_port_t              port,
	natural_t               options,
	mach_port_delta_t       delta)
{
	ipc_importance_task_t imp_task = IIT_NULL;
	boolean_t dropped;

	dropped = ipc_port_importance_delta_internal(port, options, &delta, &imp_task);

	if (IIT_NULL == imp_task || delta == 0) {
		return dropped;
	}

	if (!dropped) {
		ip_unlock(port);
	}

	assert(ipc_importance_task_is_any_receiver_type(imp_task));

	if (delta > 0) {
		ipc_importance_task_hold_internal_assertion(imp_task, delta);
	} else {
		ipc_importance_task_drop_internal_assertion(imp_task, -delta);
	}

	ipc_importance_task_release(imp_task);
	return TRUE;
}
#endif                             

  
                                     
           
                                                 
  
              
                           
   
ipc_port_t
ipc_port_make_send_locked(
	ipc_port_t      port)
{
	require_ip_active(port);
	port->ip_mscount++;
	port->ip_srights++;
	ip_reference(port);
	return port;
}

  
                              
           
                                                 
   

ipc_port_t
ipc_port_make_send(
	ipc_port_t      port)
{
	if (!IP_VALID(port)) {
		return port;
	}

	ip_lock(port);
	if (ip_active(port)) {
		ipc_port_make_send_locked(port);
		ip_unlock(port);
		return port;
	}
	ip_unlock(port);
	return IP_DEAD;
}

  
                                     
           
                                                          
              
                           
   
void
ipc_port_copy_send_locked(
	ipc_port_t      port)
{
	assert(port->ip_srights > 0);
	port->ip_srights++;
	ip_reference(port);
}

  
                              
           
                                                          
                        
                        
                         
                            
              
                                           
   

ipc_port_t
ipc_port_copy_send(
	ipc_port_t      port)
{
	ipc_port_t sright;

	if (!IP_VALID(port)) {
		return port;
	}

	ip_lock(port);
	if (ip_active(port)) {
		ipc_port_copy_send_locked(port);
		sright = port;
	} else {
		sright = IP_DEAD;
	}
	ip_unlock(port);

	return sright;
}

  
                                 
           
                                                    
                                        
              
                   
   

static mach_port_name_t
ipc_port_copyout_send_internal(
	ipc_port_t      sright,
	ipc_space_t     space,
	ipc_object_copyout_flags_t flags)
{
	mach_port_name_t name;

	if (IP_VALID(sright)) {
		kern_return_t kr;

		kr = ipc_object_copyout(space, ip_to_object(sright),
		    MACH_MSG_TYPE_PORT_SEND, flags, NULL, NULL, &name);
		if (kr != KERN_SUCCESS) {
			if (kr == KERN_INVALID_CAPABILITY) {
				name = MACH_PORT_DEAD;
			} else {
				name = MACH_PORT_NULL;
			}
		}
	} else {
		name = CAST_MACH_PORT_TO_NAME(sright);
	}

	return name;
}

mach_port_name_t
ipc_port_copyout_send(
	ipc_port_t      sright,
	ipc_space_t     space)
{
	return ipc_port_copyout_send_internal(sright, space, IPC_OBJECT_COPYOUT_FLAGS_NONE);
}

mach_port_name_t
ipc_port_copyout_send_pinned(
	ipc_port_t      sright,
	ipc_space_t     space)
{
	return ipc_port_copyout_send_internal(sright, space, IPC_OBJECT_COPYOUT_FLAGS_PINNED);
}

  
                                            
           
                               
                                
              
                                     
                             
   
void
ipc_port_release_send_and_unlock(
	ipc_port_t      port)
{
	ipc_port_t nsrequest = IP_NULL;
	mach_port_mscount_t mscount;

	assert(port->ip_srights > 0);
	if (port->ip_srights == 0) {
		panic("Over-release of port %p send right!", port);
	}

	port->ip_srights--;

	if (!ip_active(port)) {
		ip_unlock(port);
		ip_release(port);
		return;
	}

	if (port->ip_srights == 0 &&
	    port->ip_nsrequest != IP_NULL) {
		nsrequest = port->ip_nsrequest;
		port->ip_nsrequest = IP_NULL;
		mscount = port->ip_mscount;
		ip_unlock(port);
		ip_release(port);
		ipc_notify_no_senders(nsrequest, mscount);
	} else {
		ip_unlock(port);
		ip_release(port);
	}
}

  
                                 
           
                               
                                
              
                   
   

void
ipc_port_release_send(
	ipc_port_t      port)
{
	if (IP_VALID(port)) {
		ip_lock(port);
		ipc_port_release_send_and_unlock(port);
	}
}

  
                                      
           
                                                      
              
                                  
   

ipc_port_t
ipc_port_make_sonce_locked(
	ipc_port_t      port)
{
	require_ip_active(port);
	port->ip_sorights++;
	ip_reference(port);
	return port;
}

  
                               
           
                                                      
              
                           
   

ipc_port_t
ipc_port_make_sonce(
	ipc_port_t      port)
{
	if (!IP_VALID(port)) {
		return port;
	}

	ip_lock(port);
	if (ip_active(port)) {
		ipc_port_make_sonce_locked(port);
		ip_unlock(port);
		return port;
	}
	ip_unlock(port);
	return IP_DEAD;
}

  
                                  
           
                                    
                                
  
                                             
                                           
                                                 
                    
              
                                           
   

void
ipc_port_release_sonce(
	ipc_port_t      port)
{
	if (!IP_VALID(port)) {
		return;
	}

	ipc_port_adjust_special_reply_port(port, IPC_PORT_ADJUST_RESET_BOOSTRAP_CHECKIN);

	ip_lock(port);

	assert(port->ip_sorights > 0);
	if (port->ip_sorights == 0) {
		panic("Over-release of port %p send-once right!", port);
	}

	port->ip_sorights--;

	ip_unlock(port);
	ip_release(port);
}

  
                                    
           
                                                           
                                                   
              
                   
   

void
ipc_port_release_receive(
	ipc_port_t      port)
{
	ipc_port_t dest;

	if (!IP_VALID(port)) {
		return;
	}

	ip_lock(port);
	require_ip_active(port);
	assert(port->ip_receiver_name == MACH_PORT_NULL);
	dest = port->ip_destination;

	ipc_port_destroy(port);                            

	if (dest != IP_NULL) {
		ipc_port_send_turnstile_complete(dest);
		ip_release(dest);
	}
}

  
                                  
           
                                       
                                          
                                         
              
                   
   

ipc_port_t
ipc_port_alloc_special(
	ipc_space_t             space,
	ipc_port_init_flags_t   flags)
{
	ipc_port_t port;

	port = ip_object_to_port(io_alloc(IOT_PORT, Z_WAITOK | Z_ZERO));
	if (port == IP_NULL) {
		return IP_NULL;
	}

#if MACH_ASSERT
	uintptr_t buf[IP_CALLSTACK_MAX];
	ipc_port_callstack_init_debug(&buf[0], IP_CALLSTACK_MAX);
#endif                  

	io_lock_init(ip_to_object(port));
	port->ip_references = 1;
	port->ip_object.io_bits = io_makebits(TRUE, IOT_PORT, 0);

	ipc_port_init(port, space, flags, 1);

#if     MACH_ASSERT
	ipc_port_init_debug(port, &buf[0], IP_CALLSTACK_MAX);
#endif                   

	return port;
}

  
                                    
           
                                         
                                  
              
                   
   

void
ipc_port_dealloc_special(
	ipc_port_t                      port,
	__assert_only ipc_space_t       space)
{
	ip_lock(port);
	require_ip_active(port);
                                                    
	assert(port->ip_receiver == space);

	  
                                                         
                                                  
    

	imq_lock(&port->ip_messages);
	port->ip_receiver_name = MACH_PORT_NULL;
	port->ip_receiver = IS_NULL;
	imq_unlock(&port->ip_messages);

	                                              
	port->ip_mscount = 0;
	port->ip_messages.imq_seqno = 0;

	ipc_port_destroy(port);
}

  
                             
           
                                          
                                               
         
              
                   
   
void
ipc_port_finalize(
	ipc_port_t              port)
{
	ipc_port_request_t requests = port->ip_requests;

	assert(port_send_turnstile(port) == TURNSTILE_NULL);
	if (imq_is_turnstile_proxy(&port->ip_messages)) {
		assert(ipc_port_rcv_turnstile(port) == TURNSTILE_NULL);
	}

	if (ip_active(port)) {
		panic("Trying to free an active port. port %p", port);
	}

	if (requests != IPR_NULL) {
		ipc_table_size_t its = requests->ipr_size;
		it_requests_free(its, requests);
		port->ip_requests = IPR_NULL;
	}

	ipc_mqueue_deinit(&port->ip_messages);

#if     MACH_ASSERT
	ipc_port_track_dealloc(port);
#endif                   
}

  
                                      
           
                                                                
                                                           
                                         
              
                                                            
                        
        
                                                          
                                                          
   
void
kdp_mqueue_send_find_owner(struct waitq * waitq, __assert_only event64_t event, thread_waitinfo_t * waitinfo)
{
	struct turnstile *turnstile;
	assert(waitinfo->wait_type == kThreadWaitPortSend);
	assert(event == IPC_MQUEUE_FULL);
	assert(waitq_is_turnstile_queue(waitq));

	turnstile = waitq_to_turnstile(waitq);
	ipc_port_t port = (ipc_port_t)turnstile->ts_proprietor;                              

	zone_id_require(ZONE_ID_IPC_PORT, sizeof(struct ipc_port), port);

	waitinfo->owner = 0;
	waitinfo->context  = VM_KERNEL_UNSLIDE_OR_PERM(port);
	if (ip_lock_held_kdp(port)) {
		  
                                                 
                             
     
		waitinfo->owner = STACKSHOT_WAITOWNER_PORT_LOCKED;
		return;
	}

	if (ip_active(port)) {
		if (port->ip_tempowner) {
			if (port->ip_imp_task != IIT_NULL && port->ip_imp_task->iit_task != NULL) {
				                                 
				waitinfo->owner = pid_from_task(port->ip_imp_task->iit_task);
			} else {
				waitinfo->owner = STACKSHOT_WAITOWNER_INTRANSIT;
			}
		} else if (port->ip_receiver_name) {
			                     
			if (port->ip_receiver == ipc_space_kernel) {
				  
                                     
                                        
                               
       
				waitinfo->owner = STACKSHOT_WAITOWNER_KERNEL;
			} else {
				waitinfo->owner = pid_from_task(port->ip_receiver->is_task);
			}
		} else if (port->ip_destination != IP_NULL) {
			                     
			waitinfo->wait_type = kThreadWaitPortSendInTransit;
			waitinfo->owner     = VM_KERNEL_UNSLIDE_OR_PERM(port->ip_destination);
		}
	}
}

  
                                      
           
                                                                  
                                                              
                          
              
                                                            
                           
        
                                                                  
                                        
   
void
kdp_mqueue_recv_find_owner(struct waitq * waitq, __assert_only event64_t event, thread_waitinfo_t * waitinfo)
{
	assert(waitinfo->wait_type == kThreadWaitPortReceive);
	assert(event == IPC_MQUEUE_RECEIVE);

	ipc_mqueue_t mqueue = imq_from_waitq(waitq);
	waitinfo->owner     = 0;
	if (imq_is_set(mqueue)) {                                   
		ipc_pset_t set = ips_from_mq(mqueue);

		zone_id_require(ZONE_ID_IPC_PORT_SET, sizeof(struct ipc_pset), set);

		                                                            
		waitinfo->wait_type = kThreadWaitPortSetReceive;
		waitinfo->context   = VM_KERNEL_UNSLIDE_OR_PERM(set);
		if (ips_lock_held_kdp(set)) {
			waitinfo->owner = STACKSHOT_WAITOWNER_PSET_LOCKED;
		}
		                                                                                  
	} else {
		ipc_port_t port   = ip_from_mq(mqueue);

		zone_id_require(ZONE_ID_IPC_PORT, sizeof(struct ipc_port), port);

		waitinfo->context = VM_KERNEL_UNSLIDE_OR_PERM(port);
		if (ip_lock_held_kdp(port)) {
			waitinfo->owner = STACKSHOT_WAITOWNER_PORT_LOCKED;
			return;
		}

		if (ip_active(port)) {
			if (port->ip_receiver_name != MACH_PORT_NULL) {
				waitinfo->owner = port->ip_receiver_name;
			} else {
				waitinfo->owner = STACKSHOT_WAITOWNER_INTRANSIT;
			}
		}
	}
}

#if     MACH_ASSERT
#include <kern/machine.h>

  
                                      
                                               
                                           
   
#if 0
queue_head_t    port_alloc_queue = QUEUE_HEAD_INITIALIZER(port_alloc_queue);
LCK_SPIN_DECLARE(port_alloc_queue_lock, &ipc_lck_grp, &ipc_lck_attr);
#endif

unsigned long   port_count = 0;
unsigned long   port_count_warning = 20000;
unsigned long   port_timestamp = 0;

void            db_port_stack_trace(
	ipc_port_t      port);
void            db_ref(
	int             refs);
int             db_port_walk(
	unsigned int    verbose,
	unsigned int    display,
	unsigned int    ref_search,
	unsigned int    ref_target);

#ifdef MACH_BSD
extern int proc_pid(struct proc*);
#endif               

  
                                                   
                                                             
   
void
ipc_port_init_debug(
	ipc_port_t      port,
	uintptr_t       *callstack,
	unsigned int    callstack_max)
{
	unsigned int    i;

	port->ip_thread = current_thread();
	port->ip_timetrack = port_timestamp++;
	for (i = 0; i < callstack_max; ++i) {
		port->ip_callstack[i] = callstack[i];
	}
	for (i = 0; i < IP_NSPARES; ++i) {
		port->ip_spares[i] = 0;
	}

#ifdef MACH_BSD
	task_t task = current_task();
	if (task != TASK_NULL) {
		struct proc* proc = (struct proc*) get_bsdtask_info(task);
		if (proc) {
			port->ip_spares[0] = proc_pid(proc);
		}
	}
#endif               

#if 0
	lck_spin_lock(&port_alloc_queue_lock);
	++port_count;
	if (port_count_warning > 0 && port_count >= port_count_warning) {
		assert(port_count < port_count_warning);
	}
	queue_enter(&port_alloc_queue, port, ipc_port_t, ip_port_links);
	lck_spin_unlock(&port_alloc_queue_lock);
#endif
}

  
                                         
           
                                          
                                                
                                   
              
                          
   
void
ipc_port_callstack_init_debug(
	uintptr_t       *callstack,
	unsigned int    callstack_max)
{
	unsigned int    i;

	                                            
	for (i = 0; i < callstack_max; i++) {
		callstack[i] = 0;
	}

	if (ipc_portbt) {
		machine_callstack(callstack, callstack_max);
	}
}

  
                                                   
                                               
                                                       
   
#if 1
void
ipc_port_track_dealloc(
	__unused ipc_port_t     port)
{
}
#else
void
ipc_port_track_dealloc(
	ipc_port_t              port)
{
	lck_spin_lock(&port_alloc_queue_lock);
	assert(port_count > 0);
	--port_count;
	queue_remove(&port_alloc_queue, port, ipc_port_t, ip_port_links);
	lck_spin_unlock(&port_alloc_queue_lock);
}
#endif


#endif                   
