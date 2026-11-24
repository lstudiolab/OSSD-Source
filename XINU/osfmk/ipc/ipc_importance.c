/*
 * Copyright (c) 2013-2020 Apple Inc. All rights reserved.
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

#include <mach/mach_types.h>
#include <mach/notify.h>
#include <ipc/ipc_types.h>
#include <ipc/ipc_importance.h>
#include <ipc/ipc_port.h>
#include <ipc/ipc_voucher.h>
#include <kern/ipc_kobject.h>
#include <kern/ipc_tt.h>
#include <kern/mach_param.h>
#include <kern/misc_protos.h>
#include <kern/zalloc.h>
#include <kern/queue.h>
#include <kern/task.h>
#include <kern/policy_internal.h>

#include <sys/kdebug.h>

#include <mach/mach_voucher_attr_control.h>
#include <mach/machine/sdt.h>

extern int      proc_pid(void *);
extern int      proc_selfpid(void);
extern uint64_t proc_uniqueid(void *p);
extern char     *proc_name_address(void *p);

  
                                             
   
static queue_head_t ipc_importance_delayed_drop_queue;
static thread_call_t ipc_importance_delayed_drop_call;
static uint64_t ipc_importance_delayed_drop_timestamp;
static boolean_t ipc_importance_delayed_drop_call_requested = FALSE;

#define DENAP_DROP_TARGET (1000 * NSEC_PER_MSEC)                          
#define DENAP_DROP_SKEW    (100 * NSEC_PER_MSEC)                              
#define DENAP_DROP_LEEWAY  (2 * DENAP_DROP_SKEW)                               

#define DENAP_DROP_DELAY (DENAP_DROP_TARGET + DENAP_DROP_SKEW)
#define DENAP_DROP_FLAGS (THREAD_CALL_DELAY_SYS_NORMAL | THREAD_CALL_DELAY_LEEWAY)

  
                                       
   
static LCK_SPIN_DECLARE_ATTR(ipc_importance_lock_data, &ipc_lck_grp, &ipc_lck_attr);

#define ipc_importance_lock() \
	lck_spin_lock_grp(&ipc_importance_lock_data, &ipc_lck_grp)
#define ipc_importance_lock_try() \
	lck_spin_try_lock_grp(&ipc_importance_lock_data, &ipc_lck_grp)
#define ipc_importance_unlock() \
	lck_spin_unlock(&ipc_importance_lock_data)
#define ipc_importance_assert_held() \
	lck_spin_assert(&ipc_importance_lock_data, LCK_ASSERT_OWNED)

#if IIE_REF_DEBUG
#define incr_ref_counter(x) (os_atomic_inc(&(x), relaxed))

static inline
uint32_t
ipc_importance_reference_internal(ipc_importance_elem_t elem)
{
	incr_ref_counter(elem->iie_refs_added);
	return os_atomic_inc(&elem->iie_bits, relaxed) & IIE_REFS_MASK;
}

static inline
uint32_t
ipc_importance_release_internal(ipc_importance_elem_t elem)
{
	incr_ref_counter(elem->iie_refs_dropped);
	return os_atomic_dec(&elem->iie_bits, relaxed) & IIE_REFS_MASK;
}

static inline
uint32_t
ipc_importance_task_reference_internal(ipc_importance_task_t task_imp)
{
	uint32_t out;
	out = ipc_importance_reference_internal(&task_imp->iit_elem);
	incr_ref_counter(task_imp->iit_elem.iie_task_refs_added);
	return out;
}

static inline
uint32_t
ipc_importance_task_release_internal(ipc_importance_task_t task_imp)
{
	uint32_t out;

	assert(1 < IIT_REFS(task_imp));
	incr_ref_counter(task_imp->iit_elem.iie_task_refs_dropped);
	out = ipc_importance_release_internal(&task_imp->iit_elem);
	return out;
}

static inline
void
ipc_importance_counter_init(ipc_importance_elem_t elem)
{
	elem->iie_refs_added = 0;
	elem->iie_refs_dropped = 0;
	elem->iie_kmsg_refs_added = 0;
	elem->iie_kmsg_refs_inherited = 0;
	elem->iie_kmsg_refs_coalesced = 0;
	elem->iie_kmsg_refs_dropped = 0;
	elem->iie_task_refs_added = 0;
	elem->iie_task_refs_added_inherit_from = 0;
	elem->iie_task_refs_added_transition = 0;
	elem->iie_task_refs_self_added = 0;
	elem->iie_task_refs_inherited = 0;
	elem->iie_task_refs_coalesced = 0;
	elem->iie_task_refs_dropped = 0;
}
#else
#define incr_ref_counter(x)
#endif

#if DEVELOPMENT || DEBUG
static queue_head_t global_iit_alloc_queue =
    QUEUE_HEAD_INITIALIZER(global_iit_alloc_queue);
#endif

static ZONE_DECLARE(ipc_importance_task_zone, "ipc task importance",
    sizeof(struct ipc_importance_task), ZC_NOENCRYPT);
static ZONE_DECLARE(ipc_importance_inherit_zone, "ipc importance inherit",
    sizeof(struct ipc_importance_inherit), ZC_NOENCRYPT);
static zone_t ipc_importance_inherit_zone;

static ipc_voucher_attr_control_t ipc_importance_control;

static boolean_t ipc_importance_task_check_transition(ipc_importance_task_t task_imp,
    iit_update_type_t type, uint32_t delta);

static void ipc_importance_task_propagate_assertion_locked(ipc_importance_task_t task_imp,
    iit_update_type_t type, boolean_t update_task_imp);

static ipc_importance_inherit_t ipc_importance_inherit_from_task(task_t from_task, task_t to_task);

  
                                    
           
                                                         
                                                         
                                                         
                                    
              
                         
                                                                
   
static void
ipc_importance_kmsg_link(
	ipc_kmsg_t              kmsg,
	ipc_importance_elem_t   elem)
{
	ipc_importance_elem_t link_elem;

	assert(IIE_NULL == kmsg->ikm_importance);

	link_elem = (IIE_TYPE_INHERIT == IIE_TYPE(elem)) ?
	    (ipc_importance_elem_t)((ipc_importance_inherit_t)elem)->iii_to_task :
	    elem;

	queue_enter(&link_elem->iie_kmsgs, kmsg, ipc_kmsg_t, ikm_inheritance);
	kmsg->ikm_importance = elem;
}

  
                                      
           
                                                       
                                                           
                                                             
                                    
           
                                                             
              
                         
                                                                  
   
static ipc_importance_elem_t
ipc_importance_kmsg_unlink(
	ipc_kmsg_t              kmsg)
{
	ipc_importance_elem_t elem = kmsg->ikm_importance;

	if (IIE_NULL != elem) {
		ipc_importance_elem_t unlink_elem;

		unlink_elem = (IIE_TYPE_INHERIT == IIE_TYPE(elem)) ?
		    (ipc_importance_elem_t)((ipc_importance_inherit_t)elem)->iii_to_task :
		    elem;

		queue_remove(&unlink_elem->iie_kmsgs, kmsg, ipc_kmsg_t, ikm_inheritance);
		kmsg->ikm_importance = IIE_NULL;
	}
	return elem;
}

  
                                       
           
                                                            
                                                         
                                                         
                                    
              
                         
                                                                   
   
static void
ipc_importance_inherit_link(
	ipc_importance_inherit_t inherit,
	ipc_importance_elem_t elem)
{
	ipc_importance_task_t link_task;

	assert(IIE_NULL == inherit->iii_from_elem);
	link_task = (IIE_TYPE_INHERIT == IIE_TYPE(elem)) ?
	    ((ipc_importance_inherit_t)elem)->iii_to_task :
	    (ipc_importance_task_t)elem;

	queue_enter(&link_task->iit_inherits, inherit,
	    ipc_importance_inherit_t, iii_inheritance);
	inherit->iii_from_elem = elem;
}

  
                                       
           
                                                               
                                                             
                                                                    
                                                               
                                   
           
                                                        
              
                         
   
static ipc_importance_inherit_t
ipc_importance_inherit_find(
	ipc_importance_elem_t from,
	ipc_importance_task_t to_task,
	unsigned int depth)
{
	ipc_importance_task_t link_task;
	ipc_importance_inherit_t inherit;

	link_task = (IIE_TYPE_INHERIT == IIE_TYPE(from)) ?
	    ((ipc_importance_inherit_t)from)->iii_to_task :
	    (ipc_importance_task_t)from;

	queue_iterate(&link_task->iit_inherits, inherit,
	    ipc_importance_inherit_t, iii_inheritance) {
		if (inherit->iii_to_task == to_task && inherit->iii_depth == depth) {
			return inherit;
		}
	}
	return III_NULL;
}

  
                                         
           
                                                          
                                                           
                                                             
                                    
           
                                                             
              
                         
                                                                  
   
static ipc_importance_elem_t
ipc_importance_inherit_unlink(
	ipc_importance_inherit_t inherit)
{
	ipc_importance_elem_t elem = inherit->iii_from_elem;

	if (IIE_NULL != elem) {
		ipc_importance_task_t unlink_task;

		unlink_task = (IIE_TYPE_INHERIT == IIE_TYPE(elem)) ?
		    ((ipc_importance_inherit_t)elem)->iii_to_task :
		    (ipc_importance_task_t)elem;

		queue_remove(&unlink_task->iit_inherits, inherit,
		    ipc_importance_inherit_t, iii_inheritance);
		inherit->iii_from_elem = IIE_NULL;
	}
	return elem;
}

  
                                    
           
                                              
              
                                                
   
void
ipc_importance_reference(ipc_importance_elem_t elem)
{
	assert(0 < IIE_REFS(elem));
	ipc_importance_reference_internal(elem);
}

  
                                         
           
                                                         
                                                                   
              
                                                               
   
static void
ipc_importance_release_locked(ipc_importance_elem_t elem)
{
	assert(0 < IIE_REFS(elem));

#if IMPORTANCE_DEBUG
	ipc_importance_inherit_t temp_inherit;
	ipc_importance_task_t link_task;
	ipc_kmsg_t temp_kmsg;
	uint32_t expected = 0;

	if (0 < elem->iie_made) {
		expected++;
	}

	link_task = (IIE_TYPE_INHERIT == IIE_TYPE(elem)) ?
	    ((ipc_importance_inherit_t)elem)->iii_to_task :
	    (ipc_importance_task_t)elem;

	queue_iterate(&link_task->iit_kmsgs, temp_kmsg, ipc_kmsg_t, ikm_inheritance)
	if (temp_kmsg->ikm_importance == elem) {
		expected++;
	}
	queue_iterate(&link_task->iit_inherits, temp_inherit,
	    ipc_importance_inherit_t, iii_inheritance)
	if (temp_inherit->iii_from_elem == elem) {
		expected++;
	}
	if (IIE_REFS(elem) < expected + 1) {
		panic("ipc_importance_release_locked (%p)", elem);
	}
#endif                       

	if (0 < ipc_importance_release_internal(elem)) {
		ipc_importance_unlock();
		return;
	}

	              

	switch (IIE_TYPE(elem)) {
	                                          
	case IIE_TYPE_TASK:
	{
		ipc_importance_task_t task_elem;

		task_elem = (ipc_importance_task_t)elem;

		                                                                  
		assert(TASK_NULL == task_elem->iit_task);

#if DEVELOPMENT || DEBUG
		queue_remove(&global_iit_alloc_queue, task_elem, ipc_importance_task_t, iit_allocation);
#endif

		ipc_importance_unlock();

		zfree(ipc_importance_task_zone, task_elem);
		break;
	}

	                                 
	case IIE_TYPE_INHERIT:
	{
		ipc_importance_inherit_t inherit = (ipc_importance_inherit_t)elem;
		ipc_importance_task_t to_task = inherit->iii_to_task;
		ipc_importance_elem_t from_elem;

		assert(IIT_NULL != to_task);
		assert(ipc_importance_task_is_any_receiver_type(to_task));

		                                                
		from_elem = ipc_importance_inherit_unlink(inherit);
		assert(IIE_NULL != from_elem);

		  
                                                                      
                                                                          
          
    
                                                                   
                                                                       
                                                                     
                                                                     
                                                     
    
                                                                 
                                                                
                                                                
                                 
     
		assert(inherit->iii_externcnt >= inherit->iii_externdrop);
		if (inherit->iii_donating) {
			uint32_t assertcnt = III_EXTERN(inherit);

			assert(ipc_importance_task_is_any_receiver_type(to_task));
			assert(to_task->iit_externcnt >= inherit->iii_externcnt);
			assert(to_task->iit_externdrop >= inherit->iii_externdrop);
			to_task->iit_externcnt -= inherit->iii_externcnt;
			to_task->iit_externdrop -= inherit->iii_externdrop;
			inherit->iii_externcnt = 0;
			inherit->iii_externdrop = 0;
			inherit->iii_donating = FALSE;

			                                                              
			if (ipc_importance_task_check_transition(to_task, IIT_UPDATE_DROP, assertcnt)) {
				ipc_importance_task_propagate_assertion_locked(to_task, IIT_UPDATE_DROP, TRUE);
			}
		} else {
			inherit->iii_externcnt = 0;
			inherit->iii_externdrop = 0;
		}

		                                                 
		ipc_importance_release_locked(from_elem);
		                        

		                                                   
		ipc_importance_task_release(to_task);

		                      
		zfree(ipc_importance_inherit_zone, inherit);
		break;
	}
	}
}

  
                                  
           
                                                         
                                                                   
              
                                                       
              
   
void
ipc_importance_release(ipc_importance_elem_t elem)
{
	if (IIE_NULL == elem) {
		return;
	}

	ipc_importance_lock();
	ipc_importance_release_locked(elem);
	              
}

  
                                         
  
  
           
                                                            
              
                                                       
                                     
   
void
ipc_importance_task_reference(ipc_importance_task_t task_elem)
{
	if (IIT_NULL == task_elem) {
		return;
	}
#if IIE_REF_DEBUG
	incr_ref_counter(task_elem->iit_elem.iie_task_refs_added);
#endif
	ipc_importance_reference(&task_elem->iit_elem);
}

  
                                       
           
                                                             
                                                                   
              
                                                       
              
   
void
ipc_importance_task_release(ipc_importance_task_t task_elem)
{
	if (IIT_NULL == task_elem) {
		return;
	}

	ipc_importance_lock();
#if IIE_REF_DEBUG
	incr_ref_counter(task_elem->iit_elem.iie_task_refs_dropped);
#endif
	ipc_importance_release_locked(&task_elem->iit_elem);
	              
}

  
                                              
           
                                                             
                                                                   
              
                                                          
              
   
static void
ipc_importance_task_release_locked(ipc_importance_task_t task_elem)
{
	if (IIT_NULL == task_elem) {
		ipc_importance_unlock();
		return;
	}
#if IIE_REF_DEBUG
	incr_ref_counter(task_elem->iit_elem.iie_task_refs_dropped);
#endif
	ipc_importance_release_locked(&task_elem->iit_elem);
	              
}

  
                                                        
   


  
                                                                         
                                                                      
                                                                                             
   

  
                                                
           
                                                                     
                                                                 
                       
  
                                                                           
                                                        
  
              
                                         
                                                                       
                                                                  
   
static boolean_t
ipc_importance_task_check_transition(
	ipc_importance_task_t task_imp,
	iit_update_type_t type,
	uint32_t delta)
{
#if IMPORTANCE_TRACE
	task_t target_task = task_imp->iit_task;
#endif
	boolean_t boost = (IIT_UPDATE_HOLD == type);
	boolean_t before_boosted, after_boosted;

	ipc_importance_assert_held();

	if (!ipc_importance_task_is_any_receiver_type(task_imp)) {
		return FALSE;
	}

#if IMPORTANCE_TRACE
	int target_pid = task_pid(target_task);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (IMPORTANCE_CODE(IMP_ASSERTION, (((boost) ? IMP_HOLD : IMP_DROP) | TASK_POLICY_INTERNAL))) | DBG_FUNC_START,
	    proc_selfpid(), target_pid, task_imp->iit_assertcnt, IIT_EXTERN(task_imp), 0);
#endif

	                                                                      
	before_boosted = (task_imp->iit_assertcnt > 0);

	                                        
	if (boost) {
		task_imp->iit_assertcnt += delta;
#if IMPORTANCE_TRACE
		DTRACE_BOOST6(send_boost, task_t, target_task, int, target_pid,
		    task_t, current_task(), int, proc_selfpid(), int, delta, int, task_imp->iit_assertcnt);
#endif
	} else {
		                                            
		if (task_imp->iit_assertcnt < delta + IIT_EXTERN(task_imp)) {
			                                                                 
			task_imp->iit_assertcnt = IIT_EXTERN(task_imp);
		} else {
			task_imp->iit_assertcnt -= delta;
		}
#if IMPORTANCE_TRACE
		                                                         
		DTRACE_BOOST4(drop_boost, task_t, target_task, int, target_pid, int, delta, int, task_imp->iit_assertcnt);
#endif
	}

#if IMPORTANCE_TRACE
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (IMPORTANCE_CODE(IMP_ASSERTION, (((boost) ? IMP_HOLD : IMP_DROP) | TASK_POLICY_INTERNAL))) | DBG_FUNC_END,
	    proc_selfpid(), target_pid, task_imp->iit_assertcnt, IIT_EXTERN(task_imp), 0);
#endif

	                                                                
	after_boosted = (task_imp->iit_assertcnt > 0);

	if (after_boosted != before_boosted) {
		  
                                                                                          
                                                                                           
                                                                                           
                                                                                          
                                 
    
                                                                                       
                                                                    
    
                                                   
     
		assert(0 == task_imp->iit_updatepolicy);
		if (NULL != task_imp->iit_updateq) {
			if (&ipc_importance_delayed_drop_queue != task_imp->iit_updateq) {
				queue_remove(task_imp->iit_updateq, task_imp, ipc_importance_task_t, iit_updates);
				task_imp->iit_updateq = NULL;
				ipc_importance_task_release_internal(task_imp);                        
			}
		} else {
			task_imp->iit_updatepolicy = 1;
		}
		return TRUE;
	}

	return FALSE;
}


  
                                                
           
                                                                     
                                                                       
                                                                     
                                  
              
                                         
                                                                        
                                                              
   
static void
ipc_importance_task_propagate_helper(
	ipc_importance_task_t task_imp,
	iit_update_type_t type,
	queue_t propagation)
{
	ipc_importance_task_t temp_task_imp;

	  
                                                      
                                                        
    

	ipc_kmsg_t temp_kmsg;

	queue_iterate(&task_imp->iit_kmsgs, temp_kmsg, ipc_kmsg_t, ikm_inheritance) {
		mach_msg_header_t *hdr = temp_kmsg->ikm_header;
		mach_port_delta_t delta;
		ipc_port_t port;

		                                                                     
		if (IIT_UPDATE_HOLD == type) {
			if (MACH_MSGH_BITS_RAISED_IMPORTANCE(hdr->msgh_bits)) {
				continue;
			}

			                                                 
			hdr->msgh_bits |= MACH_MSGH_BITS_RAISEIMP;
			delta = 1;
		} else {
			if (!MACH_MSGH_BITS_RAISED_IMPORTANCE(hdr->msgh_bits)) {
				continue;
			}

			                                                  
			hdr->msgh_bits &= ~MACH_MSGH_BITS_RAISEIMP;
			delta = -1;
		}

		                                                                
		port = hdr->msgh_remote_port;
		assert(IP_VALID(port));
		ip_lock(port);
		temp_task_imp = IIT_NULL;
		if (!ipc_port_importance_delta_internal(port, IPID_OPTION_NORMAL, &delta, &temp_task_imp)) {
			ip_unlock(port);
		}

		                                                            
		if (IIT_NULL == temp_task_imp) {
			continue;
		}

		                                       

		                                                                     
		if (ipc_importance_task_check_transition(temp_task_imp, type, 1)) {
			incr_ref_counter(temp_task_imp->iit_elem.iie_task_refs_added_transition);
			queue_enter(propagation, temp_task_imp, ipc_importance_task_t, iit_props);
			                       
		} else {
			ipc_importance_task_release_internal(temp_task_imp);
		}
	}

	  
                                              
                                                          
    
	ipc_importance_inherit_t temp_inherit;

	queue_iterate(&task_imp->iit_inherits, temp_inherit, ipc_importance_inherit_t, iii_inheritance) {
		uint32_t assertcnt = III_EXTERN(temp_inherit);

		temp_task_imp = temp_inherit->iii_to_task;
		assert(IIT_NULL != temp_task_imp);

		if (IIT_UPDATE_HOLD == type) {
			                                                              
			if (0 == assertcnt) {
				assert(temp_inherit->iii_donating == FALSE);
				continue;
			}

			                                                                        
			if (temp_inherit->iii_donating) {
				continue;
			}

			                                                            
			temp_inherit->iii_donating = TRUE;
			temp_task_imp->iit_externcnt += temp_inherit->iii_externcnt;
			temp_task_imp->iit_externdrop += temp_inherit->iii_externdrop;
		} else {
			                                            
			if (0 == assertcnt) {
				assert(temp_inherit->iii_donating == FALSE);
				continue;
			}

			                                                  
			if (!temp_inherit->iii_donating) {
				continue;
			}

			                                
			temp_inherit->iii_donating = FALSE;

			                                                             
			assert(IIT_EXTERN(temp_task_imp) >= III_EXTERN(temp_inherit));
			assert(temp_task_imp->iit_externcnt >= temp_inherit->iii_externcnt);
			assert(temp_task_imp->iit_externdrop >= temp_inherit->iii_externdrop);
			temp_task_imp->iit_externcnt -= temp_inherit->iii_externcnt;
			temp_task_imp->iit_externdrop -= temp_inherit->iii_externdrop;
		}

		                                                                     
		assert(ipc_importance_task_is_any_receiver_type(temp_task_imp));
		if (ipc_importance_task_check_transition(temp_task_imp, type, assertcnt)) {
			ipc_importance_task_reference(temp_task_imp);
			incr_ref_counter(temp_task_imp->iit_elem.iie_task_refs_added_transition);
			queue_enter(propagation, temp_task_imp, ipc_importance_task_t, iit_props);
		}
	}
}

  
                                               
           
                                                                     
                                                               
                                                
              
                                              
                                                   
   
static void
ipc_importance_task_process_updates(
	queue_t   supplied_queue,
	boolean_t boost,
	uint64_t  max_timestamp)
{
	ipc_importance_task_t task_imp;
	queue_head_t second_chance;
	queue_t queue = supplied_queue;

	  
                                                                      
                                                                      
                                                             
    
	queue_init(&second_chance);

	                                          
retry:
	while (!queue_empty(queue)) {
		task_t target_task;
		struct task_pend_token pend_token = {};

		task_imp = (ipc_importance_task_t)queue_first(queue);
		assert(0 == task_imp->iit_updatepolicy);
		assert(queue == task_imp->iit_updateq);

		                                         
		if (task_imp->iit_updatetime > max_timestamp) {
			break;
		}

		                                                         

		                                       
		queue_remove(queue, task_imp, ipc_importance_task_t, iit_updates);
		task_imp->iit_updateq = NULL;

		target_task = task_imp->iit_task;

		                                       
		if (TASK_NULL == target_task) {
			ipc_importance_task_release_locked(task_imp);
			                         
			ipc_importance_lock();
			continue;
		}

		                                                           
		if (0 < task_imp->iit_assertcnt &&
		    queue == &ipc_importance_delayed_drop_queue) {
			ipc_importance_task_release_locked(task_imp);
			                         
			ipc_importance_lock();
			continue;
		}

		  
                                           
                                                        
     
		if (!task_lock_try(target_task)) {
			boolean_t should_wait_lock = (queue == &second_chance);
			task_imp->iit_updateq = &second_chance;

			  
                                                   
                                                      
                                                  
                                                 
                                                     
                       
      
			if (should_wait_lock) {
				task_reference(target_task);
				queue_enter_first(&second_chance, task_imp,
				    ipc_importance_task_t, iit_updates);
			} else {
				queue_enter(&second_chance, task_imp,
				    ipc_importance_task_t, iit_updates);
			}
			ipc_importance_unlock();

			if (should_wait_lock) {
				task_lock(target_task);
				task_unlock(target_task);
				task_deallocate(target_task);
			}

			ipc_importance_lock();
			continue;
		}

		                       
		if (!target_task->active) {
			task_unlock(target_task);
			ipc_importance_task_release_locked(task_imp);
			                         
			ipc_importance_lock();
			continue;
		}

		                                                                       
		task_reference(target_task);

		                          
		if (boost) {
			task_imp->iit_transitions++;
		}

		ipc_importance_unlock();

		                                                                           
		task_update_boost_locked(target_task, boost, &pend_token);

		                                                       
		ipc_importance_task_release(task_imp);
		task_unlock(target_task);
		task_policy_update_complete_unlocked(target_task, &pend_token);
		task_deallocate(target_task);

		ipc_importance_lock();
	}

	                                                                     
	if (!queue_empty(&second_chance)) {
		queue = &second_chance;
		goto retry;
	}
}


  
                                                 
           
                                                                  
                                                                
                                                                  
                            
                             
              
                  
   
static void
ipc_importance_task_delayed_drop_scan(
	__unused void *arg1,
	__unused void *arg2)
{
	ipc_importance_lock();

	                                                                            
	ipc_importance_task_process_updates(&ipc_importance_delayed_drop_queue,
	    FALSE,
	    ipc_importance_delayed_drop_timestamp);

	                                                       

	                                                                      
	if (!queue_empty(&ipc_importance_delayed_drop_queue)) {
		ipc_importance_task_t task_imp;
		uint64_t deadline;
		uint64_t leeway;

		task_imp = (ipc_importance_task_t)queue_first(&ipc_importance_delayed_drop_queue);

		nanoseconds_to_absolutetime(DENAP_DROP_DELAY, &deadline);
		deadline += task_imp->iit_updatetime;
		ipc_importance_delayed_drop_timestamp = deadline;

		nanoseconds_to_absolutetime(DENAP_DROP_LEEWAY, &leeway);

		thread_call_enter_delayed_with_leeway(
			ipc_importance_delayed_drop_call,
			NULL,
			deadline,
			leeway,
			DENAP_DROP_FLAGS);
	} else {
		ipc_importance_delayed_drop_call_requested = FALSE;
	}
	ipc_importance_unlock();
}

  
                                            
           
                                                          
                             
              
                                         
   
static void
ipc_importance_task_delayed_drop(ipc_importance_task_t task_imp)
{
	uint64_t timestamp = mach_absolute_time();                                           

	assert(ipc_importance_delayed_drop_call != NULL);

	  
                                                       
                                                              
                                                    
    
	if (NULL != task_imp->iit_updateq) {
		queue_remove(task_imp->iit_updateq, task_imp,
		    ipc_importance_task_t, iit_updates);
	} else {
		ipc_importance_task_reference_internal(task_imp);
	}

	task_imp->iit_updateq = &ipc_importance_delayed_drop_queue;
	task_imp->iit_updatetime = timestamp;

	queue_enter(&ipc_importance_delayed_drop_queue, task_imp,
	    ipc_importance_task_t, iit_updates);

	                                                              
	if (!ipc_importance_delayed_drop_call_requested) {
		uint64_t deadline;
		uint64_t leeway;

		nanoseconds_to_absolutetime(DENAP_DROP_DELAY, &deadline);
		deadline += task_imp->iit_updatetime;
		ipc_importance_delayed_drop_timestamp = deadline;

		nanoseconds_to_absolutetime(DENAP_DROP_LEEWAY, &leeway);

		ipc_importance_delayed_drop_call_requested = TRUE;
		thread_call_enter_delayed_with_leeway(
			ipc_importance_delayed_drop_call,
			NULL,
			deadline,
			leeway,
			DENAP_DROP_FLAGS);
	}
}


  
                                                          
           
                                                          
                                                           
                                      
              
                                         
   
static void
ipc_importance_task_propagate_assertion_locked(
	ipc_importance_task_t task_imp,
	iit_update_type_t type,
	boolean_t update_task_imp)
{
	boolean_t boost = (IIT_UPDATE_HOLD == type);
	ipc_importance_task_t temp_task_imp;
	queue_head_t propagate;
	queue_head_t updates;

	queue_init(&updates);
	queue_init(&propagate);

	ipc_importance_assert_held();

	  
                                                              
                                                              
                              
    
	if (update_task_imp) {
		ipc_importance_task_reference(task_imp);
		incr_ref_counter(task_imp->iit_elem.iie_task_refs_added_transition);
		queue_enter(&propagate, task_imp, ipc_importance_task_t, iit_props);
	} else {
		ipc_importance_task_propagate_helper(task_imp, type, &propagate);
	}

	  
                                                                           
                                                                        
    
	while (!queue_empty(&propagate)) {
		boolean_t need_update;

		queue_remove_first(&propagate, temp_task_imp, ipc_importance_task_t, iit_props);
		                                       

		assert(IIT_NULL != temp_task_imp);

		                                                                
		if (!ipc_importance_task_is_marked_donor(temp_task_imp) &&
		    ipc_importance_task_is_marked_receiver(temp_task_imp)) {
			ipc_importance_task_propagate_helper(temp_task_imp, type, &propagate);
		}

		                                                                                   
		need_update = (0 != temp_task_imp->iit_updatepolicy);
		temp_task_imp->iit_updatepolicy = 0;
		if (need_update && TASK_NULL != temp_task_imp->iit_task) {
			if (NULL == temp_task_imp->iit_updateq) {
				  
                                                                       
                                                                 
                           
       
				if (!boost && temp_task_imp != task_imp &&
				    ipc_importance_delayed_drop_call != NULL &&
				    ipc_importance_task_is_marked_denap_receiver(temp_task_imp)) {
					ipc_importance_task_delayed_drop(temp_task_imp);
				} else {
					temp_task_imp->iit_updatetime = 0;
					temp_task_imp->iit_updateq = &updates;
					ipc_importance_task_reference_internal(temp_task_imp);
					if (boost) {
						queue_enter(&updates, temp_task_imp,
						    ipc_importance_task_t, iit_updates);
					} else {
						queue_enter_first(&updates, temp_task_imp,
						    ipc_importance_task_t, iit_updates);
					}
				}
			} else {
				                                                    
				assert(ipc_importance_delayed_drop_call != NULL);
				assert(ipc_importance_task_is_marked_denap_receiver(temp_task_imp));
			}
		}

		ipc_importance_task_release_internal(temp_task_imp);
	}

	                                                      
	if (!queue_empty(&updates)) {
		ipc_importance_task_process_updates(&updates, boost, 0);
	}
}

  
                                                              
           
                                                         
                                                         
                                                       
                                                      
                                                           
                
              
                                        
                                                   
   
static kern_return_t
ipc_importance_task_hold_internal_assertion_locked(ipc_importance_task_t task_imp, uint32_t count)
{
	if (ipc_importance_task_check_transition(task_imp, IIT_UPDATE_HOLD, count)) {
		ipc_importance_task_propagate_assertion_locked(task_imp, IIT_UPDATE_HOLD, TRUE);
	}
	return KERN_SUCCESS;
}

  
                                                              
           
                                                         
                                                         
                                                       
                                                      
                                                           
                
              
                                        
                                                   
   
static kern_return_t
ipc_importance_task_drop_internal_assertion_locked(ipc_importance_task_t task_imp, uint32_t count)
{
	if (ipc_importance_task_check_transition(task_imp, IIT_UPDATE_DROP, count)) {
		ipc_importance_task_propagate_assertion_locked(task_imp, IIT_UPDATE_DROP, TRUE);
	}
	return KERN_SUCCESS;
}

  
                                                                   
                
                                                                     
                                                               
                                                               
                                                               
                                                      
                   
                              
                                                         
   
int
ipc_importance_task_hold_internal_assertion(ipc_importance_task_t task_imp, uint32_t count)
{
	int ret = KERN_SUCCESS;

	if (ipc_importance_task_is_any_receiver_type(task_imp)) {
		ipc_importance_lock();
		ret = ipc_importance_task_hold_internal_assertion_locked(task_imp, count);
		ipc_importance_unlock();
	}
	return ret;
}

  
                                                       
           
                                                         
                                                   
                                                   
                                                        
                                        
              
                            
                                             
   
kern_return_t
ipc_importance_task_drop_internal_assertion(ipc_importance_task_t task_imp, uint32_t count)
{
	kern_return_t ret = KERN_SUCCESS;

	if (ipc_importance_task_is_any_receiver_type(task_imp)) {
		ipc_importance_lock();
		ret = ipc_importance_task_drop_internal_assertion_locked(task_imp, count);
		ipc_importance_unlock();
	}
	return ret;
}

  
                                                                    
                
                                                                               
                                                               
                                                               
                                                               
                                                      
                   
                              
                                                         
   
kern_return_t
ipc_importance_task_hold_file_lock_assertion(ipc_importance_task_t task_imp, uint32_t count)
{
	kern_return_t ret = KERN_SUCCESS;

	if (ipc_importance_task_is_any_receiver_type(task_imp)) {
		ipc_importance_lock();
		ret = ipc_importance_task_hold_internal_assertion_locked(task_imp, count);
		if (KERN_SUCCESS == ret) {
			task_imp->iit_filelocks += count;
		}
		ipc_importance_unlock();
	}
	return ret;
}

  
                                                        
           
                                                         
                                                   
                                                   
                                                        
                                        
              
                            
                                             
   
kern_return_t
ipc_importance_task_drop_file_lock_assertion(ipc_importance_task_t task_imp, uint32_t count)
{
	kern_return_t ret = KERN_SUCCESS;

	if (ipc_importance_task_is_any_receiver_type(task_imp)) {
		ipc_importance_lock();
		if (count <= task_imp->iit_filelocks) {
			task_imp->iit_filelocks -= count;
			ret = ipc_importance_task_drop_internal_assertion_locked(task_imp, count);
		} else {
			ret = KERN_INVALID_ARGUMENT;
		}
		ipc_importance_unlock();
	}
	return ret;
}

  
                                                              
           
                                                                  
                                                                
                                   
              
                            
                                             
                                                     
                                                        
                                                           
   
kern_return_t
ipc_importance_task_hold_legacy_external_assertion(ipc_importance_task_t task_imp, uint32_t count)
{
	task_t target_task;
	uint32_t target_assertcnt;
	uint32_t target_externcnt;
	uint32_t target_legacycnt;

	kern_return_t ret;

	ipc_importance_lock();
	target_task = task_imp->iit_task;

#if IMPORTANCE_TRACE
	int target_pid = task_pid(target_task);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (IMPORTANCE_CODE(IMP_ASSERTION, (IMP_HOLD | TASK_POLICY_EXTERNAL))) | DBG_FUNC_START,
	    proc_selfpid(), target_pid, task_imp->iit_assertcnt, IIT_LEGACY_EXTERN(task_imp), 0);
#endif

	if (IIT_LEGACY_EXTERN(task_imp) == 0) {
		                                                                               
		                                           
		target_assertcnt = task_imp->iit_assertcnt;
		target_externcnt = IIT_EXTERN(task_imp);
		target_legacycnt = IIT_LEGACY_EXTERN(task_imp);
		ret = KERN_FAILURE;
		count = 0;
	} else {
		assert(ipc_importance_task_is_any_receiver_type(task_imp));
		assert(0 < task_imp->iit_assertcnt);
		assert(0 < IIT_EXTERN(task_imp));
		task_imp->iit_assertcnt += count;
		task_imp->iit_externcnt += count;
		task_imp->iit_legacy_externcnt += count;
		ret = KERN_SUCCESS;
	}
	ipc_importance_unlock();

#if IMPORTANCE_TRACE
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (IMPORTANCE_CODE(IMP_ASSERTION, (IMP_HOLD | TASK_POLICY_EXTERNAL))) | DBG_FUNC_END,
	    proc_selfpid(), target_pid, task_imp->iit_assertcnt, IIT_LEGACY_EXTERN(task_imp), 0);
	                                                                 
	DTRACE_BOOST5(receive_boost, task_t, target_task, int, target_pid, int, proc_selfpid(), int, count, int, task_imp->iit_assertcnt);
#endif

	if (KERN_FAILURE == ret && target_task != TASK_NULL) {
		printf("BUG in process %s[%d]: "
		    "attempt to acquire an additional legacy external boost assertion without holding an existing legacy external assertion. "
		    "(%d total, %d external, %d legacy-external)\n",
		    proc_name_address(target_task->bsd_info), task_pid(target_task),
		    target_assertcnt, target_externcnt, target_legacycnt);
	}

	return ret;
}

  
                                                              
           
                                                            
                                                             
                                            
  
                                                     
                                                          
                                                        
                                   
              
                            
   
kern_return_t
ipc_importance_task_drop_legacy_external_assertion(ipc_importance_task_t task_imp, uint32_t count)
{
	int ret = KERN_SUCCESS;
	task_t target_task;
	uint32_t target_assertcnt;
	uint32_t target_externcnt;
	uint32_t target_legacycnt;

	if (count > 1) {
		return KERN_INVALID_ARGUMENT;
	}

	ipc_importance_lock();
	target_task = task_imp->iit_task;

#if IMPORTANCE_TRACE
	int target_pid = task_pid(target_task);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (IMPORTANCE_CODE(IMP_ASSERTION, (IMP_DROP | TASK_POLICY_EXTERNAL))) | DBG_FUNC_START,
	    proc_selfpid(), target_pid, task_imp->iit_assertcnt, IIT_LEGACY_EXTERN(task_imp), 0);
#endif

	if (count > IIT_LEGACY_EXTERN(task_imp)) {
		                                                                             
		                                                                                            
		target_assertcnt = task_imp->iit_assertcnt;
		target_externcnt = IIT_EXTERN(task_imp);
		target_legacycnt = IIT_LEGACY_EXTERN(task_imp);
		ret = KERN_FAILURE;
	} else {
		  
                                                                   
                                                       
     
		assert(ipc_importance_task_is_any_receiver_type(task_imp));
		assert(IIT_EXTERN(task_imp) >= count);

		task_imp->iit_legacy_externdrop += count;
		task_imp->iit_externdrop += count;

		                                            
		if (IIT_LEGACY_EXTERN(task_imp) == 0) {
			if (IIT_EXTERN(task_imp) != 0) {
				task_imp->iit_externcnt -= task_imp->iit_legacy_externcnt;
				task_imp->iit_externdrop -= task_imp->iit_legacy_externdrop;
			} else {
				task_imp->iit_externcnt = 0;
				task_imp->iit_externdrop = 0;
			}
			task_imp->iit_legacy_externcnt = 0;
			task_imp->iit_legacy_externdrop = 0;
		}

		                                                                                         
		if (ipc_importance_task_check_transition(task_imp, IIT_UPDATE_DROP, count)) {
			ipc_importance_task_propagate_assertion_locked(task_imp, IIT_UPDATE_DROP, TRUE);
		}
		ret = KERN_SUCCESS;
	}

#if IMPORTANCE_TRACE
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (IMPORTANCE_CODE(IMP_ASSERTION, (IMP_DROP | TASK_POLICY_EXTERNAL))) | DBG_FUNC_END,
	    proc_selfpid(), target_pid, task_imp->iit_assertcnt, IIT_LEGACY_EXTERN(task_imp), 0);
#endif

	ipc_importance_unlock();

	                                                    
	if (KERN_FAILURE == ret && TASK_NULL != target_task) {
		printf("BUG in process %s[%d]: over-released legacy external boost assertions (%d total, %d external, %d legacy-external)\n",
		    proc_name_address(target_task->bsd_info), task_pid(target_task),
		    target_assertcnt, target_externcnt, target_legacycnt);
	}

	return ret;
}


#if LEGACY_IMPORTANCE_DELIVERY
                                                              
static kern_return_t
ipc_importance_task_externalize_legacy_assertion(ipc_importance_task_t task_imp, uint32_t count, __unused int sender_pid)
{
	task_t target_task;

	assert(IIT_NULL != task_imp);
	target_task = task_imp->iit_task;

	if (TASK_NULL == target_task ||
	    !ipc_importance_task_is_any_receiver_type(task_imp)) {
		return KERN_FAILURE;
	}

#if IMPORTANCE_TRACE
	int target_pid = task_pid(target_task);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (IMPORTANCE_CODE(IMP_ASSERTION, IMP_EXTERN)) | DBG_FUNC_START,
	    proc_selfpid(), target_pid, task_imp->iit_assertcnt, IIT_EXTERN(task_imp), 0);
#endif

	ipc_importance_lock();
	                                                                      
	assert(IIT_EXTERN(task_imp) >= IIT_LEGACY_EXTERN(task_imp));
	task_imp->iit_legacy_externcnt += count;
	task_imp->iit_externcnt += count;
	ipc_importance_unlock();

#if IMPORTANCE_TRACE
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (IMPORTANCE_CODE(IMP_ASSERTION, IMP_EXTERN)) | DBG_FUNC_END,
	    proc_selfpid(), target_pid, task_imp->iit_assertcnt, IIT_LEGACY_EXTERN(task_imp), 0);
	                                   
	DTRACE_BOOST5(receive_boost, task_t, target_task, int, target_pid, int, sender_pid, int, count, int, IIT_LEGACY_EXTERN(task_imp));
#endif                       

	return KERN_SUCCESS;
}
#endif                                 

  
                                                 
           
                                                                                                
              
                                                       
  
                                                  
   
void
ipc_importance_task_update_live_donor(ipc_importance_task_t task_imp)
{
	uint32_t task_live_donor;
	boolean_t before_donor;
	boolean_t after_donor;
	task_t target_task;

	assert(task_imp != NULL);

	  
                                                        
                       
    
	if (!ipc_importance_task_is_marked_live_donor(task_imp)) {
		return;
	}

	ipc_importance_lock();

	                                                                                                   
	target_task = task_imp->iit_task;
	if (TASK_NULL == target_task) {
		ipc_importance_unlock();
		return;
	}
	before_donor = ipc_importance_task_is_marked_donor(task_imp);

	                                                                                              
	task_live_donor = target_task->effective_policy.tep_live_donor;

#if IMPORTANCE_TRACE
	int target_pid = task_pid(target_task);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (IMPORTANCE_CODE(IMP_DONOR_CHANGE, IMP_DONOR_UPDATE_LIVE_DONOR_STATE)) | DBG_FUNC_START,
	    target_pid, task_imp->iit_donor, task_live_donor, before_donor, 0);
#endif

	                                                                            
	task_imp->iit_donor = task_live_donor;

	after_donor = ipc_importance_task_is_marked_donor(task_imp);

	                                                                                
	if (before_donor != after_donor) {
		iit_update_type_t type;

		                                                                                     
		if (0 == before_donor) {
			task_imp->iit_transitions++;
			type = IIT_UPDATE_HOLD;
		} else {
			type = IIT_UPDATE_DROP;
		}
		ipc_importance_task_propagate_assertion_locked(task_imp, type, FALSE);
	}

#if IMPORTANCE_TRACE
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (IMPORTANCE_CODE(IMP_DONOR_CHANGE, IMP_DONOR_UPDATE_LIVE_DONOR_STATE)) | DBG_FUNC_END,
	    target_pid, task_imp->iit_donor, task_live_donor, after_donor, 0);
#endif

	ipc_importance_unlock();
}


  
                                          
           
                                       
              
                                                       
  
                                                            
                                                             
   
void
ipc_importance_task_mark_donor(ipc_importance_task_t task_imp, boolean_t donating)
{
	assert(task_imp != NULL);

	ipc_importance_lock();

	int old_donor = task_imp->iit_donor;

	task_imp->iit_donor = (donating ? 1 : 0);

	if (task_imp->iit_donor > 0 && old_donor == 0) {
		task_imp->iit_transitions++;
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (IMPORTANCE_CODE(IMP_DONOR_CHANGE, IMP_DONOR_INIT_DONOR_STATE)) | DBG_FUNC_NONE,
	    task_pid(task_imp->iit_task), donating,
	    old_donor, task_imp->iit_donor, 0);

	ipc_importance_unlock();
}

  
                                            
           
                                                       
              
                                                     
                                                     
                                            
   
boolean_t
ipc_importance_task_is_marked_donor(ipc_importance_task_t task_imp)
{
	if (IIT_NULL == task_imp) {
		return FALSE;
	}
	return 0 != task_imp->iit_donor;
}

  
                                               
           
                                                              
              
                                                       
  
                                                            
   
void
ipc_importance_task_mark_live_donor(ipc_importance_task_t task_imp, boolean_t live_donating)
{
	assert(task_imp != NULL);

	ipc_importance_lock();
	task_imp->iit_live_donor = (live_donating ? 1 : 0);
	ipc_importance_unlock();
}

  
                                                    
           
                                                                       
              
                                                     
                                                     
                                            
   
boolean_t
ipc_importance_task_is_marked_live_donor(ipc_importance_task_t task_imp)
{
	if (IIT_NULL == task_imp) {
		return FALSE;
	}
	return 0 != task_imp->iit_live_donor;
}

  
                                        
           
                                                              
              
                                                     
                                                     
                                            
   
boolean_t
ipc_importance_task_is_donor(ipc_importance_task_t task_imp)
{
	if (IIT_NULL == task_imp) {
		return FALSE;
	}
	return ipc_importance_task_is_marked_donor(task_imp) ||
	       (ipc_importance_task_is_marked_receiver(task_imp) &&
	       task_imp->iit_assertcnt > 0);
}

  
                                              
           
                                                     
              
                                                     
                                           
   
boolean_t
ipc_importance_task_is_never_donor(ipc_importance_task_t task_imp)
{
	if (IIT_NULL == task_imp) {
		return FALSE;
	}
	return !ipc_importance_task_is_marked_donor(task_imp) &&
	       !ipc_importance_task_is_marked_live_donor(task_imp) &&
	       !ipc_importance_task_is_marked_receiver(task_imp);
}

  
                                             
           
                                             
              
                                                       
                                                             
                                    
   
void
ipc_importance_task_mark_receiver(ipc_importance_task_t task_imp, boolean_t receiving)
{
	assert(task_imp != NULL);

	ipc_importance_lock();
	if (receiving) {
		assert(task_imp->iit_assertcnt == 0);
		assert(task_imp->iit_externcnt == 0);
		assert(task_imp->iit_externdrop == 0);
		assert(task_imp->iit_denap == 0);
		task_imp->iit_receiver = 1;                                         
	} else if (task_imp->iit_receiver) {
		assert(task_imp->iit_denap == 0);
		if (task_imp->iit_assertcnt != 0 || IIT_EXTERN(task_imp) != 0) {
			panic("disabling imp_receiver on task with pending importance boosts!");
		}
		task_imp->iit_receiver = 0;
	}
	ipc_importance_unlock();
}


  
                                               
           
                                                          
              
                                                       
                                                         
   
boolean_t
ipc_importance_task_is_marked_receiver(ipc_importance_task_t task_imp)
{
	return IIT_NULL != task_imp && 0 != task_imp->iit_receiver;
}


  
                                                   
           
                                                    
              
                                                       
                                                             
                                    
   
void
ipc_importance_task_mark_denap_receiver(ipc_importance_task_t task_imp, boolean_t denap)
{
	assert(task_imp != NULL);

	ipc_importance_lock();
	if (denap) {
		assert(task_imp->iit_assertcnt == 0);
		assert(task_imp->iit_externcnt == 0);
		assert(task_imp->iit_receiver == 0);
		task_imp->iit_denap = 1;                                     
	} else if (task_imp->iit_denap) {
		assert(task_imp->iit_receiver == 0);
		if (0 < task_imp->iit_assertcnt || 0 < IIT_EXTERN(task_imp)) {
			panic("disabling de-nap on task with pending de-nap boosts!");
		}
		task_imp->iit_denap = 0;
	}
	ipc_importance_unlock();
}


  
                                                     
           
                                                                 
              
                                                       
                                                     
   
boolean_t
ipc_importance_task_is_marked_denap_receiver(ipc_importance_task_t task_imp)
{
	return IIT_NULL != task_imp && 0 != task_imp->iit_denap;
}

  
                                                 
           
                                                                        
                                                             
              
                                                       
                                                              
   
boolean_t
ipc_importance_task_is_denap_receiver(ipc_importance_task_t task_imp)
{
	return ipc_importance_task_is_marked_denap_receiver(task_imp);
}

  
                                                    
           
                                                          
                        
              
                                                            
                                                             
                    
   
boolean_t
ipc_importance_task_is_any_receiver_type(ipc_importance_task_t task_imp)
{
	return ipc_importance_task_is_marked_receiver(task_imp) ||
	       ipc_importance_task_is_marked_denap_receiver(task_imp);
}

#if 0                       

  
                                            
           
                                                      
              
                                                        
   
static inline void
ipc_importance_inherit_reference(ipc_importance_inherit_t inherit)
{
	ipc_importance_reference(&inherit->iii_elem);
}
#endif                       

  
                                                 
           
                                                                 
                                                                   
              
                                                               
   
static inline void
ipc_importance_inherit_release_locked(ipc_importance_inherit_t inherit)
{
	ipc_importance_release_locked(&inherit->iii_elem);
}

#if 0                       
  
                                          
           
                                                                 
                                                                   
              
                                                       
              
   
void
ipc_importance_inherit_release(ipc_importance_inherit_t inherit)
{
	if (III_NULL != inherit) {
		ipc_importance_release(&inherit->iii_elem);
	}
}
#endif                         

  
                                   
           
                                                               
                                                                       
                                                                     
                                       
              
                                                                    
                                                                      
  
                                        
   
ipc_importance_task_t
ipc_importance_for_task(task_t task, boolean_t made)
{
	ipc_importance_task_t task_elem;
	boolean_t first_pass = TRUE;

	assert(TASK_NULL != task);

retry:
	                                                 
	if (!task->active) {
		return IIT_NULL;
	}

	ipc_importance_lock();
	task_elem = task->task_imp_base;
	if (IIT_NULL != task_elem) {
		                                                               
		if (made) {
			if (0 == task_elem->iit_made++) {
				assert(IIT_REFS_MAX > IIT_REFS(task_elem));
				ipc_importance_task_reference_internal(task_elem);
			}
		} else {
			assert(IIT_REFS_MAX > IIT_REFS(task_elem));
			ipc_importance_task_reference_internal(task_elem);
		}
		ipc_importance_unlock();
		return task_elem;
	}
	ipc_importance_unlock();

	if (!first_pass) {
		return IIT_NULL;
	}
	first_pass = FALSE;

	                                                                   
	task_elem = zalloc_flags(ipc_importance_task_zone, Z_WAITOK | Z_ZERO);
	if (IIT_NULL == task_elem) {
		goto retry;
	}

	task_elem->iit_bits = IIE_TYPE_TASK | 2;                                        
	task_elem->iit_made = (made) ? 1 : 0;
	task_elem->iit_task = task;                                      
#if IIE_REF_DEBUG
	ipc_importance_counter_init(&task_elem->iit_elem);
#endif
	queue_init(&task_elem->iit_kmsgs);
	queue_init(&task_elem->iit_inherits);

	ipc_importance_lock();
	if (!task->active) {
		ipc_importance_unlock();
		zfree(ipc_importance_task_zone, task_elem);
		return IIT_NULL;
	}

	                           
	if (IIT_NULL != task->task_imp_base) {
		ipc_importance_unlock();
		zfree(ipc_importance_task_zone, task_elem);
		goto retry;
	}

	                     
	task->task_imp_base = task_elem;
	task_reference(task);
#if DEVELOPMENT || DEBUG
	queue_enter(&global_iit_alloc_queue, task_elem, ipc_importance_task_t, iit_allocation);
	task_importance_update_owner_info(task);
#endif
	ipc_importance_unlock();

	return task_elem;
}

#if DEVELOPMENT || DEBUG
void
task_importance_update_owner_info(task_t task)
{
	if (task != TASK_NULL && task->task_imp_base != IIT_NULL) {
		ipc_importance_task_t task_elem = task->task_imp_base;

		task_elem->iit_bsd_pid = task_pid(task);
		if (task->bsd_info) {
			strncpy(&task_elem->iit_procname[0], proc_name_address(task->bsd_info), 16);
			task_elem->iit_procname[16] = '\0';
		} else {
			strncpy(&task_elem->iit_procname[0], "unknown", 16);
		}
	}
}
#endif

static int
task_importance_task_get_pid(ipc_importance_task_t iit)
{
#if DEVELOPMENT || DEBUG
	return (int)iit->iit_bsd_pid;
#else
	return task_pid(iit->iit_task);
#endif
}

  
                                       
           
                                                                      
  
                                                                    
                                                                           
              
                         
   

static void
ipc_importance_reset_locked(ipc_importance_task_t task_imp, boolean_t donor)
{
	boolean_t before_donor, after_donor;

	                                                                  
	before_donor = ipc_importance_task_is_donor(task_imp);
	if (donor) {
		task_imp->iit_donor = 0;
	}
	assert(IIT_LEGACY_EXTERN(task_imp) <= IIT_EXTERN(task_imp));
	assert(task_imp->iit_legacy_externcnt <= task_imp->iit_externcnt);
	assert(task_imp->iit_legacy_externdrop <= task_imp->iit_externdrop);
	task_imp->iit_externcnt -= task_imp->iit_legacy_externcnt;
	task_imp->iit_externdrop -= task_imp->iit_legacy_externdrop;

	                                                                     
	if (IIT_EXTERN(task_imp) < task_imp->iit_assertcnt) {
		task_imp->iit_assertcnt -= IIT_LEGACY_EXTERN(task_imp);
	} else {
		task_imp->iit_assertcnt = IIT_EXTERN(task_imp);
	}
	task_imp->iit_legacy_externcnt = 0;
	task_imp->iit_legacy_externdrop = 0;
	after_donor = ipc_importance_task_is_donor(task_imp);

	                                                                       
	if (after_donor != before_donor) {
		ipc_importance_task_propagate_assertion_locked(task_imp, IIT_UPDATE_DROP, FALSE);
	}
}

  
                                
           
                                 
  
                                                                         
                                                         
              
                             
   

void
ipc_importance_reset(ipc_importance_task_t task_imp, boolean_t donor)
{
	if (IIT_NULL == task_imp) {
		return;
	}
	ipc_importance_lock();
	ipc_importance_reset_locked(task_imp, donor);
	ipc_importance_unlock();
}

  
                                          
           
                                          
  
                                                           
                                                             
                                                           
                                                                  
  
                                                                 
                                                               
              
              
                   
   
void
ipc_importance_disconnect_task(task_t task)
{
	ipc_importance_task_t task_imp;

	task_lock(task);
	ipc_importance_lock();
	task_imp = task->task_imp_base;

	                                 
	if (IIT_NULL == task_imp) {
		ipc_importance_unlock();
		task_unlock(task);
		return;
	}

	                                              
	assert(task_imp->iit_task == task);
	task_imp->iit_task = TASK_NULL;
	task->task_imp_base = IIT_NULL;
	task_unlock(task);

	                                                               
	ipc_importance_reset_locked(task_imp, TRUE);

	ipc_importance_task_release_locked(task_imp);
	                         

	                                                             
	task_deallocate(task);
}

  
                                           
           
                                                                  
  
                                                               
                                                                
                                                                 
                                                               
                                                         
              
                   
                                              
   
ipc_importance_inherit_t
ipc_importance_exec_switch_task(
	task_t old_task,
	task_t new_task)
{
	ipc_importance_inherit_t inherit = III_NULL;
	ipc_importance_task_t old_task_imp = IIT_NULL;
	ipc_importance_task_t new_task_imp = IIT_NULL;

	task_importance_reset(old_task);

	                                                            
	inherit = ipc_importance_inherit_from_task(old_task, new_task);

	                                                           
	ipc_importance_lock();

	old_task_imp = old_task->task_imp_base;
	new_task_imp = new_task->task_imp_base;

	old_task_imp->iit_task = new_task;
	new_task_imp->iit_task = old_task;

	old_task->task_imp_base = new_task_imp;
	new_task->task_imp_base = old_task_imp;

#if DEVELOPMENT || DEBUG
	  
                                                          
    
	task_importance_update_owner_info(new_task);
#endif
	ipc_importance_unlock();

	return inherit;
}

  
                                            
           
                                                    
                                                        
  
                                                   
                                               
  
                                                         
                                                      
                                                     
  
                                                                   
                                                              
                                                                
                                    
  
              
                                                            
   

boolean_t
ipc_importance_check_circularity(
	ipc_port_t      port,
	ipc_port_t      dest)
{
	ipc_importance_task_t imp_task = IIT_NULL;
	ipc_importance_task_t release_imp_task = IIT_NULL;
	boolean_t imp_lock_held = FALSE;
	int assertcnt = 0;
	ipc_port_t base;
	struct turnstile *send_turnstile = TURNSTILE_NULL;
	struct task_watchport_elem *watchport_elem = NULL;
	bool took_base_ref = false;

	assert(port != IP_NULL);
	assert(dest != IP_NULL);

	if (port == dest) {
		return TRUE;
	}
	base = dest;

	                                            
	ipc_port_send_turnstile_prepare(dest);

	                                                           
	if (port->ip_impdonation != 0) {
		imp_lock_held = TRUE;
		ipc_importance_lock();
	}

	  
                                                     
                                             
    
	ip_lock(port);

	  
                                                        
                                
    
	if (port->ip_impcount > 0 && !imp_lock_held) {
		if (!ipc_importance_lock_try()) {
			ip_unlock(port);
			ipc_importance_lock();
			ip_lock(port);
		}
		imp_lock_held = TRUE;
	}

	if (ip_lock_try(dest)) {
		if (!ip_active(dest) ||
		    (dest->ip_receiver_name != MACH_PORT_NULL) ||
		    (dest->ip_destination == IP_NULL)) {
			goto not_circular;
		}

		                                                    

		ip_unlock(dest);
	}
	ip_unlock(port);

	  
                                             
                                           
    
	if (!imp_lock_held) {
		ipc_importance_lock();
		imp_lock_held = TRUE;
	}

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

		if (imp_lock_held) {
			ipc_importance_unlock();
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

	                                                      
	assert(port->ip_tempowner != 0);

	  
                                                        
                                                      
                          
    
	release_imp_task = port->ip_imp_task;
	if (IIT_NULL != release_imp_task) {
		port->ip_imp_task = IIT_NULL;
	}
	assertcnt = port->ip_impcount;

	                                                  
	port->ip_tempowner = 0;

	  
                                                                        
                                                                         
                                   
    
	if (port_send_turnstile(port)) {
		send_turnstile = turnstile_prepare((uintptr_t)port,
		    port_send_turnstile_address(port),
		    TURNSTILE_NULL, TURNSTILE_SYNC_IPC);

		turnstile_update_inheritor(send_turnstile, port_send_turnstile(dest),
		    (TURNSTILE_INHERITOR_TURNSTILE | TURNSTILE_IMMEDIATE_UPDATE));

		                                                                            
	}
	imq_unlock(&port->ip_messages);

	                      

	ip_unlock(port);

	for (;;) {
		ipc_port_t next;
		                                                       
		ipc_port_impcount_delta(dest, assertcnt, base);

		if (dest == base) {
			break;
		}

		                        

		require_ip_active(dest);
		assert(dest->ip_receiver_name == MACH_PORT_NULL);
		assert(dest->ip_destination != IP_NULL);
		assert(dest->ip_tempowner == 0);

		next = dest->ip_destination;
		ip_unlock(dest);
		dest = next;
	}

	                            
	assert(!ip_active(base) ||
	    (base->ip_receiver_name != MACH_PORT_NULL) ||
	    (base->ip_destination == IP_NULL));

	  
                                    
                                                 
                                                    
       
    
	if (ip_active(base) && (assertcnt > 0)) {
		assert(imp_lock_held);
		if (base->ip_tempowner != 0) {
			if (IIT_NULL != base->ip_imp_task) {
				                              
				imp_task = base->ip_imp_task;
				assert(ipc_importance_task_is_any_receiver_type(imp_task));
			}
			                                        
		} else if (base->ip_receiver_name != MACH_PORT_NULL) {
			ipc_space_t space = base->ip_receiver;

			                                            
			if (space->is_task != TASK_NULL &&
			    ipc_importance_task_is_any_receiver_type(space->is_task->task_imp_base)) {
				imp_task = space->is_task->task_imp_base;
			}
		}

		                                          
		if (imp_task != IIT_NULL) {
			ipc_importance_task_reference(imp_task);
		}
	}

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

	  
                                                        
                                                               
   
                                                             
                                                                
                                                                
                                      
    
	boolean_t transfer_assertions = (imp_task != release_imp_task);

	if (imp_task != IIT_NULL) {
		assert(imp_lock_held);
		if (transfer_assertions) {
			ipc_importance_task_hold_internal_assertion_locked(imp_task, assertcnt);
		}
	}

	if (release_imp_task != IIT_NULL) {
		assert(imp_lock_held);
		if (transfer_assertions) {
			ipc_importance_task_drop_internal_assertion_locked(release_imp_task, assertcnt);
		}
	}

	if (imp_lock_held) {
		ipc_importance_unlock();
	}

	if (imp_task != IIT_NULL) {
		ipc_importance_task_release(imp_task);
	}

	if (release_imp_task != IIT_NULL) {
		ipc_importance_task_release(release_imp_task);
	}

	if (watchport_elem) {
		task_watchport_elem_deallocate(watchport_elem);
	}

	return FALSE;
}

  
                               
           
                                                               
                                                           
              
                                                                              
           
                                                                  
   
boolean_t
ipc_importance_send(
	ipc_kmsg_t              kmsg,
	mach_msg_option_t       option)
{
	ipc_port_t port = kmsg->ikm_header->msgh_remote_port;
	boolean_t port_lock_dropped = FALSE;
	ipc_importance_elem_t elem;
	task_t task;
	ipc_importance_task_t task_imp;
	kern_return_t kr;

	assert(IP_VALID(port));

	                                               
	if ((port->ip_impdonation == 0) ||
	    (option & MACH_SEND_NOIMPORTANCE) != 0) {
		return port_lock_dropped;
	}

	task = current_task();

	                                                          
	if ((option & MACH_SEND_IMPORTANCE) != 0) {
		                                                                      
		if (!ipc_importance_lock_try()) {
			port_lock_dropped = TRUE;
			ip_unlock(port);
			ipc_importance_lock();
		}
		goto portupdate;
	}

	task_imp = task->task_imp_base;
	assert(IIT_NULL != task_imp);

	                                                              
	if (ipc_importance_task_is_never_donor(task_imp)) {
		return port_lock_dropped;
	}

	elem = IIE_NULL;

	                                                                                
	if (IP_VALID(kmsg->ikm_voucher) &&
	    ipc_importance_task_is_marked_receiver(task_imp)) {
		mach_voucher_attr_value_handle_t vals[MACH_VOUCHER_ATTR_VALUE_MAX_NESTED];
		mach_voucher_attr_value_handle_array_size_t val_count;
		ipc_voucher_t voucher;

		assert(ip_kotype(kmsg->ikm_voucher) == IKOT_VOUCHER);
		voucher = (ipc_voucher_t)ip_get_kobject(kmsg->ikm_voucher);

		                                                             
		val_count = MACH_VOUCHER_ATTR_VALUE_MAX_NESTED;
		kr = mach_voucher_attr_control_get_values(ipc_importance_control, voucher,
		    vals, &val_count);
		assert(KERN_SUCCESS == kr);

		  
                                                                  
                                                     
     
		if (0 < val_count) {
			ipc_importance_elem_t check_elem;

			check_elem = (ipc_importance_elem_t)vals[0];
			assert(IIE_NULL != check_elem);
			if (IIE_TYPE_INHERIT == IIE_TYPE(check_elem)) {
				ipc_importance_inherit_t inherit;
				inherit = (ipc_importance_inherit_t) check_elem;
				if (inherit->iii_to_task == task_imp) {
					elem = check_elem;
				}
			} else if (check_elem == (ipc_importance_elem_t)task_imp) {
				elem = check_elem;
			}
		}
	}

	                                                                             
	if (IIE_NULL == elem) {
		elem = (ipc_importance_elem_t)task_imp;
	}

	                                              
	ipc_importance_reference_internal(elem);

	                                                                      
	if (!ipc_importance_lock_try()) {
		port_lock_dropped = TRUE;
		ip_unlock(port);
		ipc_importance_lock();
	}

	                                                        
	ipc_importance_kmsg_link(kmsg, elem);
	                                       

	incr_ref_counter(elem->iie_kmsg_refs_added);

	                                                                   
	if (!ipc_importance_task_is_donor(task_imp)) {
		ipc_importance_unlock();

		                                     
		if (TRUE == port_lock_dropped) {
			ip_lock(port);
		}

		return port_lock_dropped;
	}

portupdate:
	                                                                         
	kmsg->ikm_header->msgh_bits |= MACH_MSGH_BITS_RAISEIMP;

	  
                                                                          
                                                                       
                                                                     
                                 
    
	if (TRUE == port_lock_dropped) {
		ip_lock(port);
	}

	ipc_importance_assert_held();

#if IMPORTANCE_TRACE
	if (kdebug_enable) {
		mach_msg_max_trailer_t *dbgtrailer = (mach_msg_max_trailer_t *)
		    ((vm_offset_t)kmsg->ikm_header + mach_round_msg(kmsg->ikm_header->msgh_size));
		unsigned int sender_pid = dbgtrailer->msgh_audit.val[5];
		mach_msg_id_t imp_msgh_id = kmsg->ikm_header->msgh_id;
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (IMPORTANCE_CODE(IMP_MSG, IMP_MSG_SEND)) | DBG_FUNC_START,
		    task_pid(task), sender_pid, imp_msgh_id, 0, 0);
	}
#endif                       

	mach_port_delta_t delta = 1;
	boolean_t need_port_lock;
	task_imp = IIT_NULL;

	                                                               
	need_port_lock = ipc_port_importance_delta_internal(port, IPID_OPTION_NORMAL, &delta, &task_imp);
	                                  

	                                                                         
	if (IIT_NULL != task_imp && delta != 0) {
		assert(delta == 1);

		                                                                     
		if (ipc_importance_task_check_transition(task_imp, IIT_UPDATE_HOLD, delta)) {
			                                                        
			if (!need_port_lock) {
				need_port_lock = TRUE;
				ip_unlock(port);
			}
			ipc_importance_task_propagate_assertion_locked(task_imp, IIT_UPDATE_HOLD, TRUE);
		}
	}

	if (task_imp) {
		ipc_importance_task_release_locked(task_imp);
		                         
	} else {
		ipc_importance_unlock();
	}

	if (need_port_lock) {
		port_lock_dropped = TRUE;
		ip_lock(port);
	}

	return port_lock_dropped;
}

  
                                            
           
                                                                      
                                                                      
                                                                    
                                                                  
  
                                                                     
  
                                                                            
              
                                        
   
static ipc_importance_inherit_t
ipc_importance_inherit_from_kmsg(ipc_kmsg_t kmsg)
{
	ipc_importance_task_t   task_imp = IIT_NULL;
	ipc_importance_elem_t   from_elem = kmsg->ikm_importance;
	ipc_importance_elem_t   elem;
	task_t  task_self = current_task();

	ipc_port_t port = kmsg->ikm_header->msgh_remote_port;
	ipc_importance_inherit_t inherit = III_NULL;
	ipc_importance_inherit_t alloc = III_NULL;
	boolean_t cleared_self_donation = FALSE;
	boolean_t donating;
	uint32_t depth = 1;

	                                                                       
	if (IIE_NULL == kmsg->ikm_importance &&
	    !MACH_MSGH_BITS_RAISED_IMPORTANCE(kmsg->ikm_header->msgh_bits)) {
		return III_NULL;
	}

	  
                                                                     
                             
    
	if (!ipc_importance_task_is_any_receiver_type(task_self->task_imp_base)) {
		ipc_importance_lock();
		goto out_locked;
	}

	                                                           
	task_imp = ipc_importance_for_task(task_self, FALSE);

	ipc_importance_lock();

	if (IIT_NULL == task_imp) {
		goto out_locked;
	}

	incr_ref_counter(task_imp->iit_elem.iie_task_refs_added_inherit_from);

	                                                         
	if (IIE_TYPE_INHERIT == IIE_TYPE(from_elem)) {
		ipc_importance_inherit_t from_inherit = (ipc_importance_inherit_t)from_elem;

		                                               
		if (from_inherit->iii_to_task == task_imp) {
			                                                        
			if (!from_inherit->iii_donating &&
			    MACH_MSGH_BITS_RAISED_IMPORTANCE(kmsg->ikm_header->msgh_bits)) {
				kmsg->ikm_header->msgh_bits &= ~MACH_MSGH_BITS_RAISEIMP;
				cleared_self_donation = TRUE;
			}
			inherit = from_inherit;
		} else if (III_DEPTH_MAX == III_DEPTH(from_inherit)) {
			ipc_importance_task_t to_task;
			ipc_importance_elem_t unlinked_from;

			  
                                       
                                            
                                  
      
			to_task = from_inherit->iii_to_task;
			ipc_importance_task_reference(to_task);
			from_elem = (ipc_importance_elem_t)to_task;
			depth = III_DEPTH_RESET | 1;

			                                              
			unlinked_from = ipc_importance_kmsg_unlink(kmsg);
			assert(unlinked_from == (ipc_importance_elem_t)from_inherit);
			ipc_importance_kmsg_link(kmsg, from_elem);
			ipc_importance_inherit_release_locked(from_inherit);
			                         
			ipc_importance_lock();
		} else {
			                                
			depth = from_inherit->iii_depth + 1;
		}
	}

	  
                                                                        
                                                            
    

	if (from_elem == (ipc_importance_elem_t)task_imp) {
		goto out_locked;
	}

	  
                                                                     
                                                                   
                                                             
    
	if (IIE_NULL == from_elem) {
		assert(MACH_MSGH_BITS_RAISED_IMPORTANCE(kmsg->ikm_header->msgh_bits));
		ipc_importance_task_reference_internal(task_imp);
		from_elem = (ipc_importance_elem_t)task_imp;
	}

	  
                                               
                                                               
    
	while (III_NULL == inherit) {
		inherit = ipc_importance_inherit_find(from_elem, task_imp, depth);

		                                          
		if (III_NULL == inherit) {
			if (III_NULL != alloc) {
				break;
			}

			                    
			ipc_importance_unlock();
			alloc = (ipc_importance_inherit_t)
			    zalloc(ipc_importance_inherit_zone);
			ipc_importance_lock();
		}
	}

	                                                                  
	donating = MACH_MSGH_BITS_RAISED_IMPORTANCE(kmsg->ikm_header->msgh_bits);

	if (III_NULL != inherit) {
		                                     
		assert(0 < III_REFS(inherit));
		assert(0 < IIE_REFS(inherit->iii_from_elem));
		assert(inherit->iii_externcnt >= inherit->iii_made);

		                             
		if (0 == inherit->iii_made++) {
			assert(III_REFS_MAX > III_REFS(inherit));
			ipc_importance_inherit_reference_internal(inherit);
		}

		                                                                 
		if (0 == III_EXTERN(inherit)) {
			assert(!inherit->iii_donating);
			inherit->iii_donating = donating;
			if (donating) {
				task_imp->iit_externcnt += inherit->iii_externcnt;
				task_imp->iit_externdrop += inherit->iii_externdrop;
			}
		} else {
			assert(donating == inherit->iii_donating);
		}

		                                                             
		inherit->iii_externcnt++;
	} else {
		                                               
		inherit = alloc;
		inherit->iii_bits = IIE_TYPE_INHERIT | 1;
		inherit->iii_made = 1;
		inherit->iii_externcnt = 1;
		inherit->iii_externdrop = 0;
		inherit->iii_depth = depth;
		inherit->iii_to_task = task_imp;
		inherit->iii_from_elem = IIE_NULL;
		queue_init(&inherit->iii_kmsgs);

		if (donating) {
			inherit->iii_donating = TRUE;
		} else {
			inherit->iii_donating = FALSE;
		}

		  
                                                           
                                                      
     
		ipc_importance_inherit_link(inherit, from_elem);

#if IIE_REF_DEBUG
		ipc_importance_counter_init(&inherit->iii_elem);
		from_elem->iie_kmsg_refs_inherited++;
		task_imp->iit_elem.iie_task_refs_inherited++;
#endif
	}

out_locked:
	  
                                                                         
                                                          
    
	donating = MACH_MSGH_BITS_RAISED_IMPORTANCE(kmsg->ikm_header->msgh_bits);

	                                          
	elem = ipc_importance_kmsg_unlink(kmsg);
	assert(elem == from_elem);

	                                                                       
	if (III_NULL != inherit && donating) {
		task_imp->iit_externcnt++;
		                                                                                
		ipc_importance_task_hold_internal_assertion_locked(task_imp, 1);
		                                                  
	}

	                                                                          
	if (III_NULL == inherit || inherit != alloc) {
		if (IIE_NULL != from_elem) {
			if (III_NULL != inherit) {
				incr_ref_counter(from_elem->iie_kmsg_refs_coalesced);
			} else {
				incr_ref_counter(from_elem->iie_kmsg_refs_dropped);
			}
			ipc_importance_release_locked(from_elem);
			                         
		} else {
			ipc_importance_unlock();
		}

		if (IIT_NULL != task_imp) {
			if (III_NULL != inherit) {
				incr_ref_counter(task_imp->iit_elem.iie_task_refs_coalesced);
			}
			ipc_importance_task_release(task_imp);
		}

		if (III_NULL != alloc) {
			zfree(ipc_importance_inherit_zone, alloc);
		}
	} else {
		                                                                  
		ipc_importance_unlock();
	}

	  
                              
                                                                 
                                                                   
                       
    
	if (donating || cleared_self_donation) {
		ip_lock(port);
		                                                    
		if (ipc_port_importance_delta(port, IPID_OPTION_NORMAL, -1) == FALSE) {
			ip_unlock(port);
		}
	}

	if (III_NULL != inherit) {
		                                                                        
		kmsg->ikm_header->msgh_bits |= MACH_MSGH_BITS_RAISEIMP;
	} else {
		                                                                       
		kmsg->ikm_header->msgh_bits &= ~MACH_MSGH_BITS_RAISEIMP;
	}

	return inherit;
}

  
                                            
           
                                                               
                                                             
                                                   
  
                                                                           
  
                                                                            
              
                                        
                                                   
   
static ipc_importance_inherit_t
ipc_importance_inherit_from_task(
	task_t from_task,
	task_t to_task)
{
	ipc_importance_task_t   to_task_imp = IIT_NULL;
	ipc_importance_task_t   from_task_imp = IIT_NULL;
	ipc_importance_elem_t   from_elem = IIE_NULL;

	ipc_importance_inherit_t inherit = III_NULL;
	ipc_importance_inherit_t alloc = III_NULL;
	boolean_t donating;
	uint32_t depth = 1;

	to_task_imp = ipc_importance_for_task(to_task, FALSE);
	from_task_imp = ipc_importance_for_task(from_task, FALSE);
	from_elem = (ipc_importance_elem_t)from_task_imp;

	ipc_importance_lock();

	if (IIT_NULL == to_task_imp || IIT_NULL == from_task_imp) {
		goto out_locked;
	}

	  
                                                                    
                                              
    
	if (!ipc_importance_task_is_any_receiver_type(to_task_imp) ||
	    !ipc_importance_task_is_any_receiver_type(from_task_imp)) {
		goto out_locked;
	}

	                                              
	if (to_task_imp == from_task_imp) {
		goto out_locked;
	}

	incr_ref_counter(to_task_imp->iit_elem.iie_task_refs_added_inherit_from);
	incr_ref_counter(from_elem->iie_kmsg_refs_added);

	  
                                               
                                                               
    
	while (III_NULL == inherit) {
		inherit = ipc_importance_inherit_find(from_elem, to_task_imp, depth);

		                                          
		if (III_NULL == inherit) {
			if (III_NULL != alloc) {
				break;
			}

			                    
			ipc_importance_unlock();
			alloc = (ipc_importance_inherit_t)
			    zalloc(ipc_importance_inherit_zone);
			ipc_importance_lock();
		}
	}

	                                                                  
	donating = ipc_importance_task_is_donor(from_task_imp);

	if (III_NULL != inherit) {
		                                     
		assert(0 < III_REFS(inherit));
		assert(0 < IIE_REFS(inherit->iii_from_elem));

		                                  
		assert(III_REFS_MAX > III_REFS(inherit));
		ipc_importance_inherit_reference_internal(inherit);

		                                                                 
		if (0 == III_EXTERN(inherit)) {
			assert(!inherit->iii_donating);
			inherit->iii_donating = donating;
			if (donating) {
				to_task_imp->iit_externcnt += inherit->iii_externcnt;
				to_task_imp->iit_externdrop += inherit->iii_externdrop;
			}
		} else {
			assert(donating == inherit->iii_donating);
		}

		                                                             
		inherit->iii_externcnt++;
	} else {
		                                               
		inherit = alloc;
		inherit->iii_bits = IIE_TYPE_INHERIT | 1;
		inherit->iii_made = 0;
		inherit->iii_externcnt = 1;
		inherit->iii_externdrop = 0;
		inherit->iii_depth = depth;
		inherit->iii_to_task = to_task_imp;
		inherit->iii_from_elem = IIE_NULL;
		queue_init(&inherit->iii_kmsgs);

		if (donating) {
			inherit->iii_donating = TRUE;
		} else {
			inherit->iii_donating = FALSE;
		}

		  
                                                           
                                                      
     
		ipc_importance_inherit_link(inherit, from_elem);

#if IIE_REF_DEBUG
		ipc_importance_counter_init(&inherit->iii_elem);
		from_elem->iie_kmsg_refs_inherited++;
		task_imp->iit_elem.iie_task_refs_inherited++;
#endif
	}

out_locked:

	                                                                       
	if (III_NULL != inherit && donating) {
		to_task_imp->iit_externcnt++;
		                                 
		ipc_importance_task_hold_internal_assertion_locked(to_task_imp, 1);
		                                                  
	}

	                                                                          
	if (III_NULL == inherit || inherit != alloc) {
		if (IIE_NULL != from_elem) {
			if (III_NULL != inherit) {
				incr_ref_counter(from_elem->iie_kmsg_refs_coalesced);
			} else {
				incr_ref_counter(from_elem->iie_kmsg_refs_dropped);
			}
			ipc_importance_release_locked(from_elem);
			                         
		} else {
			ipc_importance_unlock();
		}

		if (IIT_NULL != to_task_imp) {
			if (III_NULL != inherit) {
				incr_ref_counter(to_task_imp->iit_elem.iie_task_refs_coalesced);
			}
			ipc_importance_task_release(to_task_imp);
		}

		if (III_NULL != alloc) {
			zfree(ipc_importance_inherit_zone, alloc);
		}
	} else {
		                                                                     
		ipc_importance_unlock();
	}

	return inherit;
}

  
                                  
           
                                                        
  
                                                          
                                                           
                                    
  
                                                          
                                                  
              
                   
   
void
ipc_importance_receive(
	ipc_kmsg_t              kmsg,
	mach_msg_option_t       option)
{
	int impresult = -1;

#if IMPORTANCE_TRACE || LEGACY_IMPORTANCE_DELIVERY
	task_t task_self = current_task();
	unsigned int sender_pid = ((mach_msg_max_trailer_t *)
	    ((vm_offset_t)kmsg->ikm_header +
	    mach_round_msg(kmsg->ikm_header->msgh_size)))->msgh_audit.val[5];
#endif

	                                                                
	if ((option & MACH_RCV_VOUCHER) != 0) {
		uint8_t recipes[2 * sizeof(ipc_voucher_attr_recipe_data_t) +
		sizeof(mach_voucher_attr_value_handle_t)];
		ipc_voucher_attr_raw_recipe_array_size_t recipe_size = 0;
		ipc_voucher_attr_recipe_t recipe = (ipc_voucher_attr_recipe_t)recipes;
		ipc_voucher_t recv_voucher;
		mach_voucher_attr_value_handle_t handle;
		ipc_importance_inherit_t inherit;
		kern_return_t kr;

		                                           
		if (IP_VALID(kmsg->ikm_voucher)) {
			ipc_voucher_t sent_voucher = (ipc_voucher_t)ip_get_kobject(kmsg->ikm_voucher);

			recipe->key = MACH_VOUCHER_ATTR_KEY_ALL;
			recipe->command = MACH_VOUCHER_ATTR_COPY;
			recipe->previous_voucher = sent_voucher;
			recipe->content_size = 0;
			recipe_size += sizeof(*recipe);
		}

		  
                                                                
                                                              
                                                 
     
		inherit = ipc_importance_inherit_from_kmsg(kmsg);
		handle = (mach_voucher_attr_value_handle_t)inherit;

		assert(IIE_NULL == kmsg->ikm_importance);

		  
                                                           
                                                                
                                                            
                                                                
     
		if (IP_VALID(kmsg->ikm_voucher) || inherit != III_NULL) {
			                                                                 
			                                                                  
			recipe = (ipc_voucher_attr_recipe_t)&recipes[recipe_size];
			recipe->key = MACH_VOUCHER_ATTR_KEY_IMPORTANCE;
			recipe->command = MACH_VOUCHER_ATTR_SET_VALUE_HANDLE;
			recipe->previous_voucher = IPC_VOUCHER_NULL;
			recipe->content_size = sizeof(mach_voucher_attr_value_handle_t);
			*(mach_voucher_attr_value_handle_t *)(void *)recipe->content = handle;
			recipe_size += sizeof(*recipe) + sizeof(mach_voucher_attr_value_handle_t);

			kr = ipc_voucher_attr_control_create_mach_voucher(ipc_importance_control,
			    recipes,
			    recipe_size,
			    &recv_voucher);
			assert(KERN_SUCCESS == kr);

			                                                                                  
			kmsg->ikm_header->msgh_bits |= (MACH_MSG_TYPE_MOVE_SEND << 16);
			ipc_port_release_send(kmsg->ikm_voucher);
			kmsg->ikm_voucher = convert_voucher_to_port(recv_voucher);
			if (III_NULL != inherit) {
				impresult = 2;
			}
		}
	} else {                           
		                                         
		if (IIE_NULL != kmsg->ikm_importance) {
			ipc_importance_elem_t elem;

			ipc_importance_lock();
			elem = ipc_importance_kmsg_unlink(kmsg);
#if IIE_REF_DEBUG
			elem->iie_kmsg_refs_dropped++;
#endif
			ipc_importance_release_locked(elem);
			                         
		}

		                                                                          
		if (MACH_MSGH_BITS_RAISED_IMPORTANCE(kmsg->ikm_header->msgh_bits)) {
			ipc_port_t port = kmsg->ikm_header->msgh_remote_port;
#if LEGACY_IMPORTANCE_DELIVERY
			ipc_importance_task_t task_imp = task_self->task_imp_base;

			                                                                                
			if (KERN_SUCCESS == ipc_importance_task_hold_internal_assertion(task_imp, 1)) {
				ipc_importance_task_externalize_legacy_assertion(task_imp, 1, sender_pid);
				impresult = 1;
			} else
#endif
			{
				                                                                
				kmsg->ikm_header->msgh_bits &= ~MACH_MSGH_BITS_RAISEIMP;
				impresult = 0;
			}

			                                                                   
			ip_lock(port);
			if (ipc_port_importance_delta(port, IPID_OPTION_NORMAL, -1) == FALSE) {
				ip_unlock(port);
			}
		}
	}

#if IMPORTANCE_TRACE
	if (-1 < impresult) {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (IMPORTANCE_CODE(IMP_MSG, IMP_MSG_DELV)) | DBG_FUNC_NONE,
		    sender_pid, task_pid(task_self),
		    kmsg->ikm_header->msgh_id, impresult, 0);
	}
	if (impresult == 2) {
		  
                                                                      
                                                                          
                                      
     
		DTRACE_BOOST5(receive_boost, task_t, task_self, int, task_pid(task_self),
		    int, sender_pid, int, 1, int, task_self->task_imp_base->iit_assertcnt);
	}
#endif                       
}

  
                                    
           
                                                       
  
              
                   
   
void
ipc_importance_unreceive(
	ipc_kmsg_t              kmsg,
	mach_msg_option_t       __unused option)
{
	                                                                     
	assert(IIE_NULL == kmsg->ikm_importance);

	                                                                
	if (MACH_MSGH_BITS_RAISED_IMPORTANCE(kmsg->ikm_header->msgh_bits)) {
		ipc_importance_task_t task_imp;

		kmsg->ikm_header->msgh_bits &= ~MACH_MSGH_BITS_RAISEIMP;
		task_imp = current_task()->task_imp_base;
		if (!IP_VALID(kmsg->ikm_voucher) && IIT_NULL != task_imp) {
			ipc_importance_task_drop_legacy_external_assertion(task_imp, 1);
		}
		  
                                                     
                                  
     
	}
}

  
                                
           
                                                              
                                                           
                                                             
                                                             
           
              
                   
   
void
ipc_importance_clean(
	ipc_kmsg_t              kmsg)
{
	ipc_port_t              port;

	                                                        
	if (IIE_NULL != kmsg->ikm_importance) {
		ipc_importance_elem_t   elem;

		ipc_importance_lock();
		elem = ipc_importance_kmsg_unlink(kmsg);
		assert(IIE_NULL != elem);
		ipc_importance_release_locked(elem);
		                         
	}

	                                                                       
	if (MACH_MSGH_BITS_RAISED_IMPORTANCE(kmsg->ikm_header->msgh_bits)) {
		kmsg->ikm_header->msgh_bits &= ~MACH_MSGH_BITS_RAISEIMP;
		port = kmsg->ikm_header->msgh_remote_port;
		if (IP_VALID(port)) {
			ip_lock(port);
			                                                                
			if (!ip_active(port) ||
			    ipc_port_importance_delta(port, IPID_OPTION_NORMAL, -1) == FALSE) {
				ip_unlock(port);
			}
		}
	}
}

void
ipc_importance_assert_clean(__assert_only ipc_kmsg_t kmsg)
{
	assert(IIE_NULL == kmsg->ikm_importance);
	assert(!MACH_MSGH_BITS_RAISED_IMPORTANCE(kmsg->ikm_header->msgh_bits));
}

  
                                              
   

static kern_return_t
ipc_importance_release_value(
	ipc_voucher_attr_manager_t              manager,
	mach_voucher_attr_key_t                 key,
	mach_voucher_attr_value_handle_t        value,
	mach_voucher_attr_value_reference_t     sync);

static kern_return_t
ipc_importance_get_value(
	ipc_voucher_attr_manager_t                      manager,
	mach_voucher_attr_key_t                         key,
	mach_voucher_attr_recipe_command_t              command,
	mach_voucher_attr_value_handle_array_t          prev_values,
	mach_voucher_attr_value_handle_array_size_t     prev_value_count,
	mach_voucher_attr_content_t                     content,
	mach_voucher_attr_content_size_t                content_size,
	mach_voucher_attr_value_handle_t                *out_value,
	mach_voucher_attr_value_flags_t                 *out_flags,
	ipc_voucher_t                                   *out_value_voucher);

static kern_return_t
ipc_importance_extract_content(
	ipc_voucher_attr_manager_t                      manager,
	mach_voucher_attr_key_t                         key,
	mach_voucher_attr_value_handle_array_t          values,
	mach_voucher_attr_value_handle_array_size_t     value_count,
	mach_voucher_attr_recipe_command_t              *out_command,
	mach_voucher_attr_content_t                     out_content,
	mach_voucher_attr_content_size_t                *in_out_content_size);

static kern_return_t
ipc_importance_command(
	ipc_voucher_attr_manager_t                      manager,
	mach_voucher_attr_key_t                         key,
	mach_voucher_attr_value_handle_array_t          values,
	mach_msg_type_number_t                          value_count,
	mach_voucher_attr_command_t                     command,
	mach_voucher_attr_content_t                     in_content,
	mach_voucher_attr_content_size_t                in_content_size,
	mach_voucher_attr_content_t                     out_content,
	mach_voucher_attr_content_size_t                *out_content_size);

static void
ipc_importance_manager_release(
	ipc_voucher_attr_manager_t              manager);

const struct ipc_voucher_attr_manager ipc_importance_manager = {
	.ivam_release_value =   ipc_importance_release_value,
	.ivam_get_value =       ipc_importance_get_value,
	.ivam_extract_content = ipc_importance_extract_content,
	.ivam_command =         ipc_importance_command,
	.ivam_release =         ipc_importance_manager_release,
	.ivam_flags =           IVAM_FLAGS_NONE,
};

#define IMPORTANCE_ASSERT_KEY(key) assert(MACH_VOUCHER_ATTR_KEY_IMPORTANCE == (key))
#define IMPORTANCE_ASSERT_MANAGER(manager) assert(&ipc_importance_manager == (manager))

  
                                                                              
           
                                                                         
                                                                           
                                                                           
                                    
              
                                        
   
static kern_return_t
ipc_importance_release_value(
	ipc_voucher_attr_manager_t              __assert_only manager,
	mach_voucher_attr_key_t                 __assert_only key,
	mach_voucher_attr_value_handle_t        value,
	mach_voucher_attr_value_reference_t     sync)
{
	ipc_importance_elem_t elem;

	IMPORTANCE_ASSERT_MANAGER(manager);
	IMPORTANCE_ASSERT_KEY(key);
	assert(0 < sync);

	elem = (ipc_importance_elem_t)value;

	ipc_importance_lock();

	                               
	if (sync != elem->iie_made) {
		assert(sync < elem->iie_made);
		ipc_importance_unlock();
		return KERN_FAILURE;
	}

	                
	elem->iie_made = 0;

	  
                                                                       
                                      
    
	if (IIE_TYPE_INHERIT == IIE_TYPE(elem)) {
		ipc_importance_inherit_t inherit = (ipc_importance_inherit_t)elem;

		assert(inherit->iii_externcnt >= inherit->iii_externdrop);

		if (inherit->iii_donating) {
			ipc_importance_task_t imp_task = inherit->iii_to_task;
			uint32_t assertcnt = III_EXTERN(inherit);

			assert(ipc_importance_task_is_any_receiver_type(imp_task));
			assert(imp_task->iit_externcnt >= inherit->iii_externcnt);
			assert(imp_task->iit_externdrop >= inherit->iii_externdrop);
			imp_task->iit_externcnt -= inherit->iii_externcnt;
			imp_task->iit_externdrop -= inherit->iii_externdrop;
			inherit->iii_externcnt = 0;
			inherit->iii_externdrop = 0;
			inherit->iii_donating = FALSE;

			                                                              
			if (ipc_importance_task_check_transition(imp_task, IIT_UPDATE_DROP, assertcnt)) {
				ipc_importance_task_propagate_assertion_locked(imp_task, IIT_UPDATE_DROP, TRUE);
			}
		} else {
			inherit->iii_externcnt = 0;
			inherit->iii_externdrop = 0;
		}
	}

	                                     
	ipc_importance_release_locked(elem);
	                      

	return KERN_SUCCESS;
}


  
                                                                          
           
                                                                            
                                                                          
                                                                         
                                                     
              
                                        
   
static kern_return_t
ipc_importance_get_value(
	ipc_voucher_attr_manager_t                      __assert_only manager,
	mach_voucher_attr_key_t                         __assert_only key,
	mach_voucher_attr_recipe_command_t              command,
	mach_voucher_attr_value_handle_array_t          prev_values,
	mach_voucher_attr_value_handle_array_size_t     prev_value_count,
	mach_voucher_attr_content_t                     __unused content,
	mach_voucher_attr_content_size_t                content_size,
	mach_voucher_attr_value_handle_t                *out_value,
	mach_voucher_attr_value_flags_t                 *out_flags,
	ipc_voucher_t                                   *out_value_voucher)
{
	ipc_importance_elem_t elem;
	task_t self;

	IMPORTANCE_ASSERT_MANAGER(manager);
	IMPORTANCE_ASSERT_KEY(key);

	if (0 != content_size) {
		return KERN_INVALID_ARGUMENT;
	}

	*out_flags = MACH_VOUCHER_ATTR_VALUE_FLAGS_NONE;
	                          

	switch (command) {
	case MACH_VOUCHER_ATTR_REDEEM:

		                                            
		if (0 < prev_value_count) {
			elem = (ipc_importance_elem_t)prev_values[0];
			assert(IIE_NULL != elem);

			ipc_importance_lock();
			assert(0 < elem->iie_made);
			elem->iie_made++;
			ipc_importance_unlock();

			*out_value = prev_values[0];
			return KERN_SUCCESS;
		}

		                                  
		*out_value = 0;
		*out_value_voucher = IPC_VOUCHER_NULL;
		return KERN_SUCCESS;

	case MACH_VOUCHER_ATTR_IMPORTANCE_SELF:
		self = current_task();

		elem = (ipc_importance_elem_t)ipc_importance_for_task(self, TRUE);
		                                                               

		*out_value = (mach_voucher_attr_value_handle_t)elem;
		*out_value_voucher = IPC_VOUCHER_NULL;
		return KERN_SUCCESS;

	default:
		  
                                   
    
                                                                 
                                                               
                                                                  
                         
     
		return KERN_INVALID_ARGUMENT;
	}
}

  
                                                                                
           
                                                                         
                                                                        
                                                                          
                                                                                  
              
                                        
   
static kern_return_t
ipc_importance_extract_content(
	ipc_voucher_attr_manager_t                      __assert_only manager,
	mach_voucher_attr_key_t                         __assert_only key,
	mach_voucher_attr_value_handle_array_t          values,
	mach_voucher_attr_value_handle_array_size_t     value_count,
	mach_voucher_attr_recipe_command_t              *out_command,
	mach_voucher_attr_content_t                     out_content,
	mach_voucher_attr_content_size_t                *in_out_content_size)
{
	ipc_importance_elem_t elem;
	unsigned int i;

	char *buf = (char *)out_content;
	mach_voucher_attr_content_size_t size = *in_out_content_size;
	mach_voucher_attr_content_size_t pos = 0;
	__unused int pid;

	IMPORTANCE_ASSERT_MANAGER(manager);
	IMPORTANCE_ASSERT_KEY(key);

	                                                   
	for (i = 0; i < value_count; i++) {
		elem = (ipc_importance_elem_t)values[i];
		if (IIE_NULL == elem) {
			continue;
		}

		pos += scnprintf(buf + pos, size - pos, "Importance for ");

		for (;;) {
			ipc_importance_inherit_t inherit = III_NULL;
			ipc_importance_task_t task_imp;

			if (IIE_TYPE_TASK == IIE_TYPE(elem)) {
				task_imp = (ipc_importance_task_t)elem;
			} else {
				inherit = (ipc_importance_inherit_t)elem;
				task_imp = inherit->iii_to_task;
			}
#if DEVELOPMENT || DEBUG
			pos += scnprintf(buf + pos, size - pos, "%s[%d]",
			    task_imp->iit_procname, task_imp->iit_bsd_pid);
#else
			ipc_importance_lock();
			pid = task_importance_task_get_pid(task_imp);
			ipc_importance_unlock();
			pos += scnprintf(buf + pos, size - pos, "pid %d", pid);
#endif                           

			if (III_NULL == inherit) {
				break;
			}
			pos += scnprintf(buf + pos, size - pos,
			    " (%d of %d boosts) %s from ",
			    III_EXTERN(inherit), inherit->iii_externcnt,
			    (inherit->iii_donating) ? "donated" : "linked");
			elem = inherit->iii_from_elem;
		}

		pos++;                                 
		break;
	}
	*out_command = MACH_VOUCHER_ATTR_NOOP;                                         
	*in_out_content_size = pos;
	return KERN_SUCCESS;
}

  
                                                                        
           
                                                                           
                                             
              
                                        
   
static kern_return_t
ipc_importance_command(
	ipc_voucher_attr_manager_t              __assert_only manager,
	mach_voucher_attr_key_t                 __assert_only key,
	mach_voucher_attr_value_handle_array_t  values,
	mach_msg_type_number_t                  value_count,
	mach_voucher_attr_command_t             command,
	mach_voucher_attr_content_t             in_content,
	mach_voucher_attr_content_size_t        in_content_size,
	mach_voucher_attr_content_t             out_content,
	mach_voucher_attr_content_size_t        *out_content_size)
{
	ipc_importance_inherit_t inherit;
	ipc_importance_task_t to_task;
	uint32_t refs, *outrefsp;
	mach_msg_type_number_t i;
	uint32_t externcnt;

	IMPORTANCE_ASSERT_MANAGER(manager);
	IMPORTANCE_ASSERT_KEY(key);

	if (in_content_size != sizeof(refs) ||
	    (*out_content_size != 0 && *out_content_size != sizeof(refs))) {
		return KERN_INVALID_ARGUMENT;
	}
	refs = *(uint32_t *)(void *)in_content;
	outrefsp = (*out_content_size != 0) ? (uint32_t *)(void *)out_content : NULL;

	if (MACH_VOUCHER_IMPORTANCE_ATTR_DROP_EXTERNAL != command) {
		return KERN_NOT_SUPPORTED;
	}

	                                                                       
	inherit = III_NULL;
	for (i = 0; i < value_count; i++) {
		ipc_importance_elem_t elem = (ipc_importance_elem_t)values[i];

		if (IIE_NULL != elem && IIE_TYPE_INHERIT == IIE_TYPE(elem)) {
			inherit = (ipc_importance_inherit_t)elem;
			break;
		}
	}
	if (III_NULL == inherit) {
		return KERN_INVALID_ARGUMENT;
	}

	ipc_importance_lock();

	if (0 == refs) {
		if (NULL != outrefsp) {
			*outrefsp = III_EXTERN(inherit);
		}
		ipc_importance_unlock();
		return KERN_SUCCESS;
	}

	to_task = inherit->iii_to_task;
	assert(ipc_importance_task_is_any_receiver_type(to_task));

	                                                                    
	if (!ipc_importance_task_is_marked_denap_receiver(to_task)) {
		ipc_importance_unlock();
		return KERN_INVALID_TASK;                           
	}

	                                              
	if (III_EXTERN(inherit) < refs) {
		ipc_importance_unlock();
		return KERN_FAILURE;
	}

	                                                                                        
	if (inherit->iii_donating) {
		assert(IIT_EXTERN(to_task) >= III_EXTERN(inherit));
		assert(to_task->iit_externcnt >= inherit->iii_externcnt);
		assert(to_task->iit_externdrop >= inherit->iii_externdrop);
		inherit->iii_externdrop += refs;
		to_task->iit_externdrop += refs;
		externcnt = III_EXTERN(inherit);
		if (0 == externcnt) {
			inherit->iii_donating = FALSE;
			to_task->iit_externcnt -= inherit->iii_externcnt;
			to_task->iit_externdrop -= inherit->iii_externdrop;


			                                                                              
			if (ipc_importance_delayed_drop_call != NULL &&
			    ipc_importance_task_is_marked_denap_receiver(to_task)) {
				ipc_importance_task_delayed_drop(to_task);
			}

			                                                             
			if (ipc_importance_task_check_transition(to_task, IIT_UPDATE_DROP, refs)) {
				ipc_importance_task_propagate_assertion_locked(to_task, IIT_UPDATE_DROP, TRUE);
				                                                  
			}
		} else {
			                                                         
			                                                        
			if (to_task->iit_assertcnt > refs + externcnt) {
				to_task->iit_assertcnt -= refs;
			} else {
				to_task->iit_assertcnt = externcnt;
			}
		}
	} else {
		inherit->iii_externdrop += refs;
		externcnt = III_EXTERN(inherit);
	}

	                                   
	if (NULL != outrefsp) {
		*outrefsp = externcnt;
	}

	ipc_importance_unlock();
	return KERN_SUCCESS;
}

  
                                                                                
           
                                                                          
            
              
                                                                        
                                                                            
                                         
   
__abortlike
static void
ipc_importance_manager_release(
	ipc_voucher_attr_manager_t              __assert_only manager)
{
	IMPORTANCE_ASSERT_MANAGER(manager);
	panic("Voucher importance manager released");
}

  
                               
           
                                           
              
                                               
   
void
ipc_importance_init(void)
{
	kern_return_t kr;

	kr = ipc_register_well_known_mach_voucher_attr_manager(&ipc_importance_manager,
	    (mach_voucher_attr_value_handle_t)0,
	    MACH_VOUCHER_ATTR_KEY_IMPORTANCE,
	    &ipc_importance_control);
	if (KERN_SUCCESS != kr) {
		printf("Voucher importance manager register returned %d", kr);
	}
}

  
                                           
           
                                                     
                                        
              
                                                 
   
void
ipc_importance_thread_call_init(void)
{
	                                                   
	queue_init(&ipc_importance_delayed_drop_queue);
	ipc_importance_delayed_drop_call =
	    thread_call_allocate(ipc_importance_task_delayed_drop_scan, NULL);
	if (NULL == ipc_importance_delayed_drop_call) {
		panic("ipc_importance_init");
	}
}

  
                                     
                                                        
                                                         
                                               
   
extern int
task_importance_list_pids(task_t task, int flags, char *pid_list, unsigned int max_count)
{
	if (kdp_lck_spin_is_acquired(&ipc_importance_lock_data) ||
	    max_count < 1 ||
	    task->task_imp_base == IIT_NULL ||
	    pid_list == NULL ||
	    flags != TASK_IMP_LIST_DONATING_PIDS) {
		return 0;
	}
	unsigned int pidcount = 0;
	task_t temp_task;
	ipc_importance_task_t task_imp = task->task_imp_base;
	ipc_kmsg_t temp_kmsg;
	ipc_importance_inherit_t temp_inherit;
	ipc_importance_elem_t elem;
	int target_pid = 0, previous_pid;

	queue_iterate(&task_imp->iit_inherits, temp_inherit, ipc_importance_inherit_t, iii_inheritance) {
		                           
		if (pidcount >= max_count) {
			break;
		}
		previous_pid = target_pid;
		target_pid = -1;

		if (temp_inherit->iii_donating) {
			target_pid = task_importance_task_get_pid(temp_inherit->iii_to_task);
		}

		if (target_pid != -1 && previous_pid != target_pid) {
			memcpy(pid_list, &target_pid, sizeof(target_pid));
			pid_list += sizeof(target_pid);
			pidcount++;
		}
	}

	target_pid = 0;
	queue_iterate(&task_imp->iit_kmsgs, temp_kmsg, ipc_kmsg_t, ikm_inheritance) {
		if (pidcount >= max_count) {
			break;
		}
		previous_pid = target_pid;
		target_pid = -1;
		elem = temp_kmsg->ikm_importance;
		temp_task = TASK_NULL;

		if (elem == IIE_NULL) {
			continue;
		}

		if (!(temp_kmsg->ikm_header && MACH_MSGH_BITS_RAISED_IMPORTANCE(temp_kmsg->ikm_header->msgh_bits))) {
			continue;
		}

		if (IIE_TYPE_TASK == IIE_TYPE(elem)) {
			ipc_importance_task_t temp_iit = (ipc_importance_task_t)elem;
			target_pid = task_importance_task_get_pid(temp_iit);
		} else {
			temp_inherit = (ipc_importance_inherit_t)elem;
			target_pid = task_importance_task_get_pid(temp_inherit->iii_to_task);
		}

		if (target_pid != -1 && previous_pid != target_pid) {
			memcpy(pid_list, &target_pid, sizeof(target_pid));
			pid_list += sizeof(target_pid);
			pidcount++;
		}
	}

	return pidcount;
}
