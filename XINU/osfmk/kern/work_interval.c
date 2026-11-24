/*
 * Copyright (c) 2017 Apple Inc. All rights reserved.
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


#include <sys/work_interval.h>

#include <kern/work_interval.h>

#include <kern/thread.h>
#include <kern/sched_prim.h>
#include <kern/machine.h>
#include <kern/thread_group.h>
#include <kern/ipc_kobject.h>
#include <kern/task.h>
#include <kern/coalition.h>
#include <kern/policy_internal.h>
#include <kern/mpsc_queue.h>

#include <mach/kern_return.h>
#include <mach/notify.h>
#include <os/refcnt.h>

#include <stdatomic.h>

  
                                                                    
                                                                          
                                                                       
                                                                       
                                                                     
                     
   

__options_decl(thread_work_interval_options_t, uint32_t, {
	                                                            
	THREAD_WI_EXPLICIT_JOIN_POLICY = 0x1,
	                                                        
	THREAD_WI_AUTO_JOIN_POLICY     = 0x2,
	                                          
	THREAD_WI_THREAD_LOCK_HELD     = 0x4,
	                                          
	THREAD_WI_THREAD_LOCK_NEEDED   = 0x8,
	                                                                                                    
	THREAD_WI_THREAD_CTX_SWITCH    = 0x10,
});

static kern_return_t thread_set_work_interval(thread_t, struct work_interval *, thread_work_interval_options_t);

#if CONFIG_SCHED_AUTO_JOIN
                                                        
static struct mpsc_daemon_queue work_interval_deallocate_queue;

static void work_interval_deferred_release(struct work_interval *);

  
                                 
  
                                                                                                
                                      
                                                                      
                                                                
   
#define WORK_INTERVAL_STATUS_DEFERRED_FINISH_MASK    ((uint32_t)(1 << 31))
#define WORK_INTERVAL_STATUS_AUTO_JOIN_COUNT_MASK    ((uint32_t)(WORK_INTERVAL_STATUS_DEFERRED_FINISH_MASK - 1))
#define WORK_INTERVAL_STATUS_AUTO_JOIN_COUNT_MAX     WORK_INTERVAL_STATUS_AUTO_JOIN_COUNT_MASK
typedef uint32_t work_interval_auto_join_status_t;

static inline bool __unused
work_interval_status_deferred_finish(work_interval_auto_join_status_t status)
{
	return (status & WORK_INTERVAL_STATUS_DEFERRED_FINISH_MASK) ? true : false;
}

static inline uint32_t __unused
work_interval_status_auto_join_count(work_interval_auto_join_status_t status)
{
	return (uint32_t)(status & WORK_INTERVAL_STATUS_AUTO_JOIN_COUNT_MASK);
}

  
                                             
  
                                                                           
   
struct work_interval_deferred_finish_state {
	uint64_t instance_id;
	uint64_t start;
	uint64_t deadline;
	uint64_t complexity;
};

struct work_interval_auto_join_info {
	struct work_interval_deferred_finish_state deferred_finish_state;
	work_interval_auto_join_status_t _Atomic status;
};
#endif                             

  
                        
  
                                                                     
                                                     
  
                                                               
                        
  
                                               
                                                         
                                                            
                                
  
                                                                 
                                  
                                                               
   

struct work_interval {
	uint64_t wi_id;
	struct os_refcnt wi_ref_count;
	uint32_t wi_create_flags;

	                                                              
	ipc_port_t wi_port;

	  
                                                   
                                  
                                                            
    
	uint64_t wi_creator_uniqueid;
	uint32_t wi_creator_pid;
	int wi_creator_pidversion;

#if CONFIG_THREAD_GROUPS
	struct thread_group *wi_group;                             
#endif                           

#if CONFIG_SCHED_AUTO_JOIN
	                                                                            
	struct work_interval_auto_join_info wi_auto_join_info;

	  
                                                      
                                                       
                                                               
                                               
    
	struct mpsc_queue_chain   wi_deallocate_link;
#endif                             
};

#if CONFIG_SCHED_AUTO_JOIN

  
                                          
  
                                                                                                     
                                                                                                    
                                                                                                    
                                                                                                     
                                     
   
static inline void
work_interval_perform_deferred_finish(__unused struct work_interval_deferred_finish_state *deferred_finish_state,
    __unused struct work_interval *work_interval, __unused thread_t thread)
{

	KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_WI_DEFERRED_FINISH),
	    thread_tid(thread), thread_group_get_id(work_interval->wi_group));
}

  
                                      
  
                                                                             
                     
   
static void
work_interval_auto_join_increment(struct work_interval *work_interval)
{
	struct work_interval_auto_join_info *join_info = &work_interval->wi_auto_join_info;
	__assert_only work_interval_auto_join_status_t old_status = os_atomic_add_orig(&join_info->status, 1, relaxed);
	assert(work_interval_status_auto_join_count(old_status) < WORK_INTERVAL_STATUS_AUTO_JOIN_COUNT_MAX);
}

  
                                      
  
                                                                                             
                                                                                             
                                                                                    
   
static void
work_interval_auto_join_decrement(struct work_interval *work_interval, thread_t thread)
{
	struct work_interval_auto_join_info *join_info = &work_interval->wi_auto_join_info;
	work_interval_auto_join_status_t old_status, new_status;
	struct work_interval_deferred_finish_state deferred_finish_state;
	bool perform_finish;

	                                                                 
	os_atomic_rmw_loop(&join_info->status, old_status, new_status, acquire, {
		perform_finish = false;
		new_status = old_status;
		assert(work_interval_status_auto_join_count(old_status) > 0);
		new_status -= 1;
		if (new_status == WORK_INTERVAL_STATUS_DEFERRED_FINISH_MASK) {
		                                                                     
		        new_status = 0;
		        perform_finish = true;
		          
                                                                                    
                                                         
             
		        deferred_finish_state = join_info->deferred_finish_state;
		}
	});

	if (perform_finish == true) {
		  
                                                                
                                                               
                                                             
                                                                 
                
     
		assert(thread->thread_group == work_interval->wi_group);
		work_interval_perform_deferred_finish(&deferred_finish_state, work_interval, thread);
	}
}

  
                                    
  
                                                                  
   
static inline bool
work_interval_auto_join_enabled(struct work_interval *work_interval)
{
	return (work_interval->wi_create_flags & WORK_INTERVAL_FLAG_ENABLE_AUTO_JOIN) != 0;
}

  
                                          
  
                                                                        
   
static inline bool __unused
work_interval_deferred_finish_enabled(struct work_interval *work_interval)
{
	return (work_interval->wi_create_flags & WORK_INTERVAL_FLAG_ENABLE_DEFERRED_FINISH) != 0;
}

#endif                             

static inline void
work_interval_retain(struct work_interval *work_interval)
{
	  
                                                              
                                                        
                                                            
                                                                     
    
	os_ref_retain(&work_interval->wi_ref_count);
}

static inline void
work_interval_deallocate(struct work_interval *work_interval)
{
	KDBG_RELEASE(MACHDBG_CODE(DBG_MACH_WORKGROUP, WORKGROUP_INTERVAL_DESTROY),
	    work_interval->wi_id);
#if CONFIG_THREAD_GROUPS
	thread_group_release(work_interval->wi_group);
	work_interval->wi_group = NULL;
#endif                           
	kfree(work_interval, sizeof(struct work_interval));
}

  
                          
  
                                                                                 
                                                       
  
                                                                           
  
                                                                                   
                                                                            
                                                           
   
static void
work_interval_release(struct work_interval *work_interval, __unused thread_work_interval_options_t options)
{
	if (os_ref_release(&work_interval->wi_ref_count) == 0) {
#if CONFIG_SCHED_AUTO_JOIN
		if (options & THREAD_WI_THREAD_LOCK_HELD) {
			work_interval_deferred_release(work_interval);
		} else {
			work_interval_deallocate(work_interval);
		}
#else                             
		work_interval_deallocate(work_interval);
#endif                             
	}
}

#if CONFIG_SCHED_AUTO_JOIN

  
                                   
  
                                                                       
   
static void
work_interval_deferred_release(struct work_interval *work_interval)
{
	mpsc_daemon_enqueue(&work_interval_deallocate_queue,
	    &work_interval->wi_deallocate_link, MPSC_QUEUE_NONE);
}

  
                                   
  
                                                                     
                                                                        
                                                                           
                                                                        
                                                      
   
inline bool
work_interval_should_propagate(thread_t cthread, thread_t thread)
{
	                                                                                                       
	if ((cthread->th_work_interval == NULL) || (thread->th_work_interval != NULL)) {
		return false;
	}

	                                                                
	if (work_interval_auto_join_enabled(cthread->th_work_interval) == false) {
		return false;
	}

	                                                                    
	if ((cthread->sched_mode != TH_MODE_REALTIME) || (thread->sched_mode != TH_MODE_REALTIME)) {
		return false;
	}


	                                                                                      
	struct thread_group *thread_home_tg = thread_group_get_home_group(thread);
	if (thread_group_get_home_group(cthread) != thread_home_tg) {
		return false;
	}

	                                                                                                  
	if (thread->thread_group != thread_home_tg) {
		return false;
	}

	                                                                                        
	if ((!cthread->active) || (!thread->active)) {
		return false;
	}

	return true;
}

  
                                      
  
                                                                    
  
                                                                     
                                                                        
                
   
void
work_interval_auto_join_propagate(thread_t from, thread_t to)
{
	assert(from == current_thread());
	work_interval_retain(from->th_work_interval);
	work_interval_auto_join_increment(from->th_work_interval);
	__assert_only kern_return_t kr = thread_set_work_interval(to, from->th_work_interval,
	    THREAD_WI_AUTO_JOIN_POLICY | THREAD_WI_THREAD_LOCK_HELD | THREAD_WI_THREAD_CTX_SWITCH);
	assert(kr == KERN_SUCCESS);
}

  
                                   
  
                                                                                 
  
                               
   
void
work_interval_auto_join_unwind(thread_t thread)
{
	__assert_only kern_return_t kr = thread_set_work_interval(thread, NULL,
	    THREAD_WI_AUTO_JOIN_POLICY | THREAD_WI_THREAD_LOCK_HELD | THREAD_WI_THREAD_CTX_SWITCH);
	assert(kr == KERN_SUCCESS);
}

  
                                   
  
                                                                                 
                                                                              
                                                                                 
                                                          
  
                               
   
void
work_interval_auto_join_demote(thread_t thread)
{
	__assert_only kern_return_t kr = thread_set_work_interval(thread, NULL,
	    THREAD_WI_AUTO_JOIN_POLICY | THREAD_WI_THREAD_LOCK_HELD);
	assert(kr == KERN_SUCCESS);
}

static void
work_interval_deallocate_queue_invoke(mpsc_queue_chain_t e,
    __assert_only mpsc_daemon_queue_t dq)
{
	struct work_interval *work_interval = NULL;
	work_interval = mpsc_queue_element(e, struct work_interval, wi_deallocate_link);
	assert(dq == &work_interval_deallocate_queue);
	assert(os_ref_get_count(&work_interval->wi_ref_count) == 0);
	work_interval_deallocate(work_interval);
}

#endif                             

void
work_interval_subsystem_init(void)
{
#if CONFIG_SCHED_AUTO_JOIN
	  
                                                                          
                                                                           
                                                                       
                                                                        
                                                                        
    
	mpsc_daemon_queue_init_with_thread_call(&work_interval_deallocate_queue,
	    work_interval_deallocate_queue_invoke, THREAD_CALL_PRIORITY_KERNEL);
#endif                             
}

  
                             
  
                                                              
                                                     
   
static struct work_interval *
work_interval_port_convert_locked(ipc_port_t port)
{
	struct work_interval *work_interval = NULL;

	if (!IP_VALID(port)) {
		return NULL;
	}

	if (!ip_active(port)) {
		return NULL;
	}

	if (IKOT_WORK_INTERVAL != ip_kotype(port)) {
		return NULL;
	}

	work_interval = (struct work_interval *) ip_get_kobject(port);

	work_interval_retain(work_interval);

	return work_interval;
}

  
                             
  
                                                                                     
  
                                                      
  
                                                                              
                                                                                
   
static kern_return_t
port_name_to_work_interval(mach_port_name_t     name,
    struct work_interval **work_interval)
{
	if (!MACH_PORT_VALID(name)) {
		return KERN_INVALID_NAME;
	}

	ipc_port_t port = IPC_PORT_NULL;
	kern_return_t kr = KERN_SUCCESS;

	kr = ipc_port_translate_send(current_space(), name, &port);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	                    

	assert(IP_VALID(port));

	struct work_interval *converted_work_interval;

	converted_work_interval = work_interval_port_convert_locked(port);

	                                                           
	if (converted_work_interval == NULL) {
		kr = KERN_INVALID_CAPABILITY;
	}

	ip_unlock(port);

	if (kr == KERN_SUCCESS) {
		*work_interval = converted_work_interval;
	}

	return kr;
}


  
                            
  
                                                                          
                                                                                  
  
                                                               
  
                                                                                   
                                                                              
                                                                       
   
void
work_interval_port_notify(mach_msg_header_t *msg)
{
	mach_no_senders_notification_t *notification = (void *)msg;
	ipc_port_t port = notification->not_header.msgh_remote_port;
	struct work_interval *work_interval = NULL;

	if (!IP_VALID(port)) {
		panic("work_interval_port_notify(): invalid port");
	}

	ip_lock(port);

	if (!ip_active(port)) {
		panic("work_interval_port_notify(): inactive port %p", port);
	}

	if (ip_kotype(port) != IKOT_WORK_INTERVAL) {
		panic("work_interval_port_notify(): not the right kobject: %p, %d\n",
		    port, ip_kotype(port));
	}

	if (port->ip_mscount != notification->not_count) {
		panic("work_interval_port_notify(): unexpected make-send count: %p, %d, %d",
		    port, port->ip_mscount, notification->not_count);
	}

	if (port->ip_srights != 0) {
		panic("work_interval_port_notify(): unexpected send right count: %p, %d",
		    port, port->ip_srights);
	}

	work_interval = (struct work_interval *) ip_get_kobject(port);

	if (work_interval == NULL) {
		panic("work_interval_port_notify(): missing kobject: %p", port);
	}

	ipc_kobject_set_atomically(port, IKO_NULL, IKOT_NONE);

	work_interval->wi_port = MACH_PORT_NULL;

	ip_unlock(port);

	ipc_port_dealloc_kernel(port);
	work_interval_release(work_interval, THREAD_WI_THREAD_LOCK_NEEDED);
}

  
                            
  
                                                                           
  
                                                                        
                                  
   
static uint32_t
work_interval_port_type(mach_port_name_t port_name)
{
	struct work_interval *work_interval = NULL;
	kern_return_t kr;
	uint32_t work_interval_type;

	if (port_name == MACH_PORT_NULL) {
		return WORK_INTERVAL_TYPE_LAST;
	}

	kr = port_name_to_work_interval(port_name, &work_interval);
	if (kr != KERN_SUCCESS) {
		return WORK_INTERVAL_TYPE_LAST;
	}
	                                

	assert(work_interval != NULL);
	work_interval_type = work_interval->wi_create_flags & WORK_INTERVAL_TYPE_MASK;
	work_interval_release(work_interval, THREAD_WI_THREAD_LOCK_NEEDED);
	return work_interval_type;
}


  
                             
  
                                                                     
                                                 
  
                                                           
                                                      
                                                                             
  
                                                                                   
                                                                                  
                                                                 
   
static kern_return_t
thread_set_work_interval(thread_t thread,
    struct work_interval *work_interval, thread_work_interval_options_t options)
{
	                                                                                    
	if (options & THREAD_WI_EXPLICIT_JOIN_POLICY) {
		assert(thread == current_thread());
	}

	                                                                                 
	if (options & THREAD_WI_THREAD_LOCK_NEEDED) {
		assert((options & THREAD_WI_EXPLICIT_JOIN_POLICY) != 0);
	}

	                                                                       
	if (options & THREAD_WI_AUTO_JOIN_POLICY) {
		assert((options & THREAD_WI_THREAD_LOCK_HELD) != 0);
	}

	if (work_interval) {
		uint32_t work_interval_type = work_interval->wi_create_flags & WORK_INTERVAL_TYPE_MASK;

		if ((work_interval_type == WORK_INTERVAL_TYPE_COREAUDIO) &&
		    (thread->sched_mode != TH_MODE_REALTIME) && (thread->saved_mode != TH_MODE_REALTIME)) {
			return KERN_INVALID_ARGUMENT;
		}
	}

	struct work_interval *old_th_wi = thread->th_work_interval;
#if CONFIG_SCHED_AUTO_JOIN
	bool old_wi_auto_joined = ((thread->sched_flags & TH_SFLAG_THREAD_GROUP_AUTO_JOIN) != 0);

	spl_t s;
	                                    
	if (options & THREAD_WI_THREAD_LOCK_NEEDED) {
		s = splsched();
		thread_lock(thread);
	}

	  
                                                   
   
                                                                                              
                                                                                      
                                                                                            
                                                                                     
                                                                                          
                                 
   
                                                                                    
                                                                                      
                                                                                           
    
	bool thread_on_remote_core = ((thread != current_thread()) && (thread->state & TH_RUN) && (thread->runq == PROCESSOR_NULL));

	if (thread_on_remote_core && ((options & THREAD_WI_THREAD_CTX_SWITCH) == 0)) {
		assert((options & THREAD_WI_THREAD_LOCK_NEEDED) == 0);
		os_atomic_or(&thread->th_work_interval_flags, TH_WORK_INTERVAL_FLAGS_AUTO_JOIN_LEAK, relaxed);
		return KERN_SUCCESS;
	}

	old_wi_auto_joined = ((thread->sched_flags & TH_SFLAG_THREAD_GROUP_AUTO_JOIN) != 0);

	if ((options & THREAD_WI_AUTO_JOIN_POLICY) || old_wi_auto_joined) {
		__kdebug_only uint64_t old_tg_id = (old_th_wi) ? thread_group_get_id(old_th_wi->wi_group) : ~0;
		__kdebug_only uint64_t new_tg_id = (work_interval) ? thread_group_get_id(work_interval->wi_group) : ~0;
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_WI_AUTO_JOIN),
		    thread_tid(thread), old_tg_id, new_tg_id, options);
	}

	if (old_wi_auto_joined) {
		  
                                                                                   
                                                
     
		if (thread->sched_mode != TH_MODE_REALTIME) {
			assert((thread->th_work_interval_flags & TH_WORK_INTERVAL_FLAGS_AUTO_JOIN_LEAK) != 0);
		}

		os_atomic_andnot(&thread->th_work_interval_flags, TH_WORK_INTERVAL_FLAGS_AUTO_JOIN_LEAK, relaxed);
		work_interval_auto_join_decrement(old_th_wi, thread);
		thread->sched_flags &= ~TH_SFLAG_THREAD_GROUP_AUTO_JOIN;
	}

#endif                             

	KDBG_RELEASE(MACHDBG_CODE(DBG_MACH_WORKGROUP, WORKGROUP_INTERVAL_CHANGE),
	    thread_tid(thread), (old_th_wi ? old_th_wi->wi_id : 0), (work_interval ? work_interval->wi_id : 0), !!(options & THREAD_WI_AUTO_JOIN_POLICY));

	                               
	thread->th_work_interval = work_interval;

#if CONFIG_SCHED_AUTO_JOIN

	if ((options & THREAD_WI_AUTO_JOIN_POLICY) && work_interval) {
		assert(work_interval_auto_join_enabled(work_interval) == true);
		thread->sched_flags |= TH_SFLAG_THREAD_GROUP_AUTO_JOIN;
	}

	if (options & THREAD_WI_THREAD_LOCK_NEEDED) {
		thread_unlock(thread);
		splx(s);
	}
#endif                             

#if CONFIG_THREAD_GROUPS
	struct thread_group *new_tg = (work_interval) ? (work_interval->wi_group) : NULL;
	thread_set_work_interval_thread_group(thread, new_tg, (options & THREAD_WI_AUTO_JOIN_POLICY));
#endif                           

	if (old_th_wi != NULL) {
		work_interval_release(old_th_wi, options);
	}

	return KERN_SUCCESS;
}

static kern_return_t
thread_set_work_interval_explicit_join(thread_t thread, struct work_interval *work_interval)
{
	assert(thread == current_thread());
	return thread_set_work_interval(thread, work_interval, THREAD_WI_EXPLICIT_JOIN_POLICY | THREAD_WI_THREAD_LOCK_NEEDED);
}

kern_return_t
work_interval_thread_terminate(thread_t thread)
{
	assert(thread == current_thread());
	if (thread->th_work_interval != NULL) {
		return thread_set_work_interval(thread, NULL, THREAD_WI_EXPLICIT_JOIN_POLICY | THREAD_WI_THREAD_LOCK_NEEDED);
	}
	return KERN_SUCCESS;
}

kern_return_t
kern_work_interval_notify(thread_t thread, struct kern_work_interval_args* kwi_args)
{
	assert(thread == current_thread());
	assert(kwi_args->work_interval_id != 0);

	struct work_interval *work_interval = thread->th_work_interval;

	if (work_interval == NULL ||
	    work_interval->wi_id != kwi_args->work_interval_id) {
		                                                                          
		return KERN_INVALID_ARGUMENT;
	}

	task_t notifying_task = current_task();

	if (work_interval->wi_creator_uniqueid != get_task_uniqueid(notifying_task) ||
	    work_interval->wi_creator_pidversion != get_task_version(notifying_task)) {
		                                            
		return KERN_INVALID_ARGUMENT;
	}

	spl_t s = splsched();

#if CONFIG_THREAD_GROUPS
	assert(work_interval->wi_group == thread->thread_group);
#endif                           

	uint64_t urgency_param1, urgency_param2;
	kwi_args->urgency = (uint16_t)thread_get_urgency(thread, &urgency_param1, &urgency_param2);

	splx(s);

	                                        
	machine_work_interval_notify(thread, kwi_args);

	return KERN_SUCCESS;
}

                                                   
static _Atomic uint64_t unique_work_interval_id = 1;

kern_return_t
kern_work_interval_create(thread_t thread,
    struct kern_work_interval_create_args *create_params)
{
	assert(thread == current_thread());

	uint32_t create_flags = create_params->wica_create_flags;

	if (((create_flags & WORK_INTERVAL_FLAG_JOINABLE) == 0) &&
	    thread->th_work_interval != NULL) {
		  
                                                              
                                                     
    
                                                 
     
		return KERN_FAILURE;
	}

	  
                                                                     
             
    
	task_t creating_task = current_task();
	if ((create_flags & WORK_INTERVAL_TYPE_MASK) == WORK_INTERVAL_TYPE_CA_CLIENT) {
		  
                                                              
                                                                               
                              
     
		if (create_flags & WORK_INTERVAL_FLAG_GROUP) {
			return KERN_FAILURE;
		}
		if (!task_is_app(creating_task)) {
#if XNU_TARGET_OS_OSX
			  
                                                         
                                                           
                                         
      
			create_flags |= WORK_INTERVAL_FLAG_IGNORED;
#else
			  
                                                     
                                             
      
			return KERN_NOT_SUPPORTED;
#endif                        
		}
		if (task_set_ca_client_wi(creating_task, true) == false) {
			return KERN_FAILURE;
		}
	}

#if CONFIG_SCHED_AUTO_JOIN
	if (create_flags & WORK_INTERVAL_FLAG_ENABLE_AUTO_JOIN) {
		uint32_t type = (create_flags & WORK_INTERVAL_TYPE_MASK);
		if (type != WORK_INTERVAL_TYPE_COREAUDIO) {
			return KERN_NOT_SUPPORTED;
		}
		if ((create_flags & WORK_INTERVAL_FLAG_GROUP) == 0) {
			return KERN_NOT_SUPPORTED;
		}
	}

	if (create_flags & WORK_INTERVAL_FLAG_ENABLE_DEFERRED_FINISH) {
		if ((create_flags & WORK_INTERVAL_FLAG_ENABLE_AUTO_JOIN) == 0) {
			return KERN_NOT_SUPPORTED;
		}
	}
#endif                             

	struct work_interval *work_interval = kalloc_flags(sizeof(*work_interval),
	    Z_WAITOK | Z_ZERO);
	assert(work_interval != NULL);

	uint64_t work_interval_id = os_atomic_inc(&unique_work_interval_id, relaxed);

	*work_interval = (struct work_interval) {
		.wi_id                  = work_interval_id,
		.wi_ref_count           = {},
		.wi_create_flags        = create_flags,
		.wi_creator_pid         = pid_from_task(creating_task),
		.wi_creator_uniqueid    = get_task_uniqueid(creating_task),
		.wi_creator_pidversion  = get_task_version(creating_task),
	};
	os_ref_init(&work_interval->wi_ref_count, NULL);

	__kdebug_only uint64_t tg_id = 0;
#if CONFIG_THREAD_GROUPS
	struct thread_group *tg;
	if (create_flags & WORK_INTERVAL_FLAG_GROUP) {
		                                                      
		char name[THREAD_GROUP_MAXNAME] = "";

		snprintf(name, sizeof(name), "WI[%d] #%lld",
		    work_interval->wi_creator_pid, work_interval_id);

		tg = thread_group_create_and_retain();

		thread_group_set_name(tg, name);

		work_interval->wi_group = tg;
	} else {
		                                                     
		tg = thread_group_get_home_group(thread);

		thread_group_retain(tg);

		work_interval->wi_group = tg;
	}

	                                            
	tg_id = thread_group_get_id(work_interval->wi_group);

#endif                           

	if (create_flags & WORK_INTERVAL_FLAG_JOINABLE) {
		mach_port_name_t name = MACH_PORT_NULL;

		                                                   
		work_interval->wi_port = ipc_kobject_alloc_port(
			(ipc_kobject_t)work_interval, IKOT_WORK_INTERVAL,
			IPC_KOBJECT_ALLOC_MAKE_SEND | IPC_KOBJECT_ALLOC_NSREQUEST);

		name = ipc_port_copyout_send(work_interval->wi_port, current_space());

		if (!MACH_PORT_VALID(name)) {
			  
                                                  
                                          
                                                   
      
			return KERN_RESOURCE_SHORTAGE;
		}

		create_params->wica_port = name;
	} else {
		                                                     
		kern_return_t kr = thread_set_work_interval_explicit_join(thread, work_interval);
		if (kr != KERN_SUCCESS) {
			                                                             
                                                           
			work_interval_release(work_interval, THREAD_WI_THREAD_LOCK_NEEDED);
			return kr;
		}
		create_params->wica_port = MACH_PORT_NULL;
	}

	create_params->wica_id = work_interval_id;

	KDBG_RELEASE(MACHDBG_CODE(DBG_MACH_WORKGROUP, WORKGROUP_INTERVAL_CREATE),
	    work_interval_id, create_flags, pid_from_task(creating_task), tg_id);
	return KERN_SUCCESS;
}

kern_return_t
kern_work_interval_get_flags_from_port(mach_port_name_t port_name, uint32_t *flags)
{
	assert(flags != NULL);

	kern_return_t kr;
	struct work_interval *work_interval;

	kr = port_name_to_work_interval(port_name, &work_interval);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	assert(work_interval != NULL);
	*flags = work_interval->wi_create_flags;

	work_interval_release(work_interval, THREAD_WI_THREAD_LOCK_NEEDED);

	return KERN_SUCCESS;
}


kern_return_t
kern_work_interval_destroy(thread_t thread, uint64_t work_interval_id)
{
	if (work_interval_id == 0) {
		return KERN_INVALID_ARGUMENT;
	}

	if (thread->th_work_interval == NULL ||
	    thread->th_work_interval->wi_id != work_interval_id) {
		                                                                  
		return KERN_INVALID_ARGUMENT;
	}

	return thread_set_work_interval_explicit_join(thread, NULL);
}

kern_return_t
kern_work_interval_join(thread_t            thread,
    mach_port_name_t    port_name)
{
	struct work_interval *work_interval = NULL;
	kern_return_t kr;

	if (port_name == MACH_PORT_NULL) {
		                                         
		return thread_set_work_interval_explicit_join(thread, NULL);
	}

	kr = port_name_to_work_interval(port_name, &work_interval);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	                                

	assert(work_interval != NULL);

	kr = thread_set_work_interval_explicit_join(thread, work_interval);
	                                                                         
	if (kr != KERN_SUCCESS) {
		work_interval_release(work_interval, THREAD_WI_THREAD_LOCK_NEEDED);
	}
	return kr;
}

  
                                          
  
                                                      
                                                     
   
bool
work_interval_port_type_render_server(mach_port_name_t port_name)
{
	return work_interval_port_type(port_name) == WORK_INTERVAL_TYPE_CA_RENDER_SERVER;
}
