/*
 * Copyright (c) 1993-1995, 1999-2020 Apple Inc. All rights reserved.
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
#include <mach/thread_act.h>

#include <kern/kern_types.h>
#include <kern/zalloc.h>
#include <kern/sched_prim.h>
#include <kern/clock.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/waitq.h>
#include <kern/ledger.h>
#include <kern/policy_internal.h>

#include <vm/vm_pageout.h>

#include <kern/thread_call.h>
#include <kern/timer_call.h>

#include <libkern/OSAtomic.h>
#include <kern/timer_queue.h>

#include <sys/kdebug.h>
#if CONFIG_DTRACE
#include <mach/sdt.h>
#endif
#include <machine/machine_routines.h>

static ZONE_DECLARE(thread_call_zone, "thread_call",
    sizeof(thread_call_data_t), ZC_NOENCRYPT);

typedef enum {
	TCF_ABSOLUTE    = 0,
	TCF_CONTINUOUS  = 1,
	TCF_COUNT       = 2,
} thread_call_flavor_t;

__options_decl(thread_call_group_flags_t, uint32_t, {
	TCG_NONE                = 0x0,
	TCG_PARALLEL            = 0x1,
	TCG_DEALLOC_ACTIVE      = 0x2,
});

static struct thread_call_group {
	__attribute__((aligned(128))) lck_ticket_t tcg_lock;

	const char *            tcg_name;

	queue_head_t            pending_queue;
	uint32_t                pending_count;

	queue_head_t            delayed_queues[TCF_COUNT];
	struct priority_queue_deadline_min delayed_pqueues[TCF_COUNT];
	timer_call_data_t       delayed_timers[TCF_COUNT];

	timer_call_data_t       dealloc_timer;

	struct waitq            idle_waitq;
	uint64_t                idle_timestamp;
	uint32_t                idle_count, active_count, blocked_count;

	uint32_t                tcg_thread_pri;
	uint32_t                target_thread_count;

	thread_call_group_flags_t tcg_flags;

	struct waitq            waiters_waitq;
} thread_call_groups[THREAD_CALL_INDEX_MAX] = {
	[THREAD_CALL_INDEX_HIGH] = {
		.tcg_name               = "high",
		.tcg_thread_pri         = BASEPRI_PREEMPT_HIGH,
		.target_thread_count    = 4,
		.tcg_flags              = TCG_NONE,
	},
	[THREAD_CALL_INDEX_KERNEL] = {
		.tcg_name               = "kernel",
		.tcg_thread_pri         = BASEPRI_KERNEL,
		.target_thread_count    = 1,
		.tcg_flags              = TCG_PARALLEL,
	},
	[THREAD_CALL_INDEX_USER] = {
		.tcg_name               = "user",
		.tcg_thread_pri         = BASEPRI_DEFAULT,
		.target_thread_count    = 1,
		.tcg_flags              = TCG_PARALLEL,
	},
	[THREAD_CALL_INDEX_LOW] = {
		.tcg_name               = "low",
		.tcg_thread_pri         = MAXPRI_THROTTLE,
		.target_thread_count    = 1,
		.tcg_flags              = TCG_PARALLEL,
	},
	[THREAD_CALL_INDEX_KERNEL_HIGH] = {
		.tcg_name               = "kernel-high",
		.tcg_thread_pri         = BASEPRI_PREEMPT,
		.target_thread_count    = 2,
		.tcg_flags              = TCG_NONE,
	},
	[THREAD_CALL_INDEX_QOS_UI] = {
		.tcg_name               = "qos-ui",
		.tcg_thread_pri         = BASEPRI_FOREGROUND,
		.target_thread_count    = 1,
		.tcg_flags              = TCG_NONE,
	},
	[THREAD_CALL_INDEX_QOS_IN] = {
		.tcg_name               = "qos-in",
		.tcg_thread_pri         = BASEPRI_USER_INITIATED,
		.target_thread_count    = 1,
		.tcg_flags              = TCG_NONE,
	},
	[THREAD_CALL_INDEX_QOS_UT] = {
		.tcg_name               = "qos-ut",
		.tcg_thread_pri         = BASEPRI_UTILITY,
		.target_thread_count    = 1,
		.tcg_flags              = TCG_NONE,
	},
};

typedef struct thread_call_group        *thread_call_group_t;

#define INTERNAL_CALL_COUNT             768
#define THREAD_CALL_DEALLOC_INTERVAL_NS (5 * NSEC_PER_MSEC)           
#define THREAD_CALL_ADD_RATIO           4
#define THREAD_CALL_MACH_FACTOR_CAP     3
#define THREAD_CALL_GROUP_MAX_THREADS   500

struct thread_call_thread_state {
	struct thread_call_group * thc_group;
	struct thread_call *       thc_call;                                        
	uint64_t thc_call_start;
	uint64_t thc_call_soft_deadline;
	uint64_t thc_call_hard_deadline;
	uint64_t thc_call_pending_timestamp;
	uint64_t thc_IOTES_invocation_timestamp;
	thread_call_func_t  thc_func;
	thread_call_param_t thc_param0;
	thread_call_param_t thc_param1;
};

static bool                     thread_call_daemon_awake = true;
  
                                                      
                                                                      
   
static struct waitq             daemon_waitq;

static thread_call_data_t       internal_call_storage[INTERNAL_CALL_COUNT];
static queue_head_t             thread_call_internal_queue;
int                                             thread_call_internal_queue_count = 0;
static uint64_t                 thread_call_dealloc_interval_abs;

static void                     _internal_call_init(void);

static thread_call_t            _internal_call_allocate(thread_call_func_t func, thread_call_param_t param0);
static bool                     _is_internal_call(thread_call_t call);
static void                     _internal_call_release(thread_call_t call);
static bool                     _pending_call_enqueue(thread_call_t call, thread_call_group_t group, uint64_t now);
static bool                     _delayed_call_enqueue(thread_call_t call, thread_call_group_t group,
    uint64_t deadline, thread_call_flavor_t flavor);
static bool                     _call_dequeue(thread_call_t call, thread_call_group_t group);
static void                     thread_call_wake(thread_call_group_t group);
static void                     thread_call_daemon(void *arg);
static void                     thread_call_thread(thread_call_group_t group, wait_result_t wres);
static void                     thread_call_dealloc_timer(timer_call_param_t p0, timer_call_param_t p1);
static void                     thread_call_group_setup(thread_call_group_t group);
static void                     sched_call_thread(int type, thread_t thread);
static void                     thread_call_start_deallocate_timer(thread_call_group_t group);
static void                     thread_call_wait_locked(thread_call_t call, spl_t s);
static bool                     thread_call_wait_once_locked(thread_call_t call, spl_t s);

static boolean_t                thread_call_enter_delayed_internal(thread_call_t call,
    thread_call_func_t alt_func, thread_call_param_t alt_param0,
    thread_call_param_t param1, uint64_t deadline,
    uint64_t leeway, unsigned int flags);

                                                                       
extern void thread_call_delayed_timer(timer_call_param_t p0, timer_call_param_t p1);

LCK_GRP_DECLARE(thread_call_lck_grp, "thread_call");


static void
thread_call_lock_spin(thread_call_group_t group)
{
	lck_ticket_lock(&group->tcg_lock, &thread_call_lck_grp);
}

static void
thread_call_unlock(thread_call_group_t group)
{
	lck_ticket_unlock(&group->tcg_lock);
}

static void __assert_only
thread_call_assert_locked(thread_call_group_t group)
{
	lck_ticket_assert_owned(&group->tcg_lock);
}


static spl_t
disable_ints_and_lock(thread_call_group_t group)
{
	spl_t s = splsched();
	thread_call_lock_spin(group);

	return s;
}

static void
enable_ints_and_unlock(thread_call_group_t group, spl_t s)
{
	thread_call_unlock(group);
	splx(s);
}

               
static thread_call_group_t
thread_call_get_group(thread_call_t call)
{
	thread_call_index_t index = call->tc_index;

	assert(index >= 0 && index < THREAD_CALL_INDEX_MAX);

	return &thread_call_groups[index];
}

               
static thread_call_flavor_t
thread_call_get_flavor(thread_call_t call)
{
	return (call->tc_flags & THREAD_CALL_FLAG_CONTINUOUS) ? TCF_CONTINUOUS : TCF_ABSOLUTE;
}

               
static thread_call_flavor_t
thread_call_set_flavor(thread_call_t call, thread_call_flavor_t flavor)
{
	assert(flavor == TCF_CONTINUOUS || flavor == TCF_ABSOLUTE);
	thread_call_flavor_t old_flavor = thread_call_get_flavor(call);

	if (old_flavor != flavor) {
		if (flavor == TCF_CONTINUOUS) {
			call->tc_flags |= THREAD_CALL_FLAG_CONTINUOUS;
		} else {
			call->tc_flags &= ~THREAD_CALL_FLAG_CONTINUOUS;
		}
	}

	return old_flavor;
}

                                       
static bool
thread_call_enqueue_tail(
	thread_call_t           call,
	queue_t                 new_queue)
{
	queue_t                 old_queue = call->tc_queue;

	thread_call_group_t     group = thread_call_get_group(call);
	thread_call_flavor_t    flavor = thread_call_get_flavor(call);

	if (old_queue != NULL &&
	    old_queue != &group->delayed_queues[flavor]) {
		panic("thread call (%p) on bad queue (old_queue: %p)", call, old_queue);
	}

	if (old_queue == &group->delayed_queues[flavor]) {
		priority_queue_remove(&group->delayed_pqueues[flavor], &call->tc_pqlink);
	}

	if (old_queue == NULL) {
		enqueue_tail(new_queue, &call->tc_qlink);
	} else {
		re_queue_tail(new_queue, &call->tc_qlink);
	}

	call->tc_queue = new_queue;

	return old_queue != NULL;
}

static queue_head_t *
thread_call_dequeue(
	thread_call_t            call)
{
	queue_t                 old_queue = call->tc_queue;

	thread_call_group_t     group = thread_call_get_group(call);
	thread_call_flavor_t    flavor = thread_call_get_flavor(call);

	if (old_queue != NULL &&
	    old_queue != &group->pending_queue &&
	    old_queue != &group->delayed_queues[flavor]) {
		panic("thread call (%p) on bad queue (old_queue: %p)", call, old_queue);
	}

	if (old_queue == &group->delayed_queues[flavor]) {
		priority_queue_remove(&group->delayed_pqueues[flavor], &call->tc_pqlink);
	}

	if (old_queue != NULL) {
		remqueue(&call->tc_qlink);

		call->tc_queue = NULL;
	}
	return old_queue;
}

static queue_head_t *
thread_call_enqueue_deadline(
	thread_call_t           call,
	thread_call_group_t     group,
	thread_call_flavor_t    flavor,
	uint64_t                deadline)
{
	queue_t old_queue = call->tc_queue;
	queue_t new_queue = &group->delayed_queues[flavor];

	thread_call_flavor_t old_flavor = thread_call_set_flavor(call, flavor);

	if (old_queue != NULL &&
	    old_queue != &group->pending_queue &&
	    old_queue != &group->delayed_queues[old_flavor]) {
		panic("thread call (%p) on bad queue (old_queue: %p)", call, old_queue);
	}

	if (old_queue == new_queue) {
		                                                            
		uint64_t old_deadline = call->tc_pqlink.deadline;
		call->tc_pqlink.deadline = deadline;

		if (old_deadline < deadline) {
			priority_queue_entry_increased(&group->delayed_pqueues[flavor],
			    &call->tc_pqlink);
		} else {
			priority_queue_entry_decreased(&group->delayed_pqueues[flavor],
			    &call->tc_pqlink);
		}
	} else {
		if (old_queue == &group->delayed_queues[old_flavor]) {
			priority_queue_remove(&group->delayed_pqueues[old_flavor],
			    &call->tc_pqlink);
		}

		call->tc_pqlink.deadline = deadline;

		priority_queue_insert(&group->delayed_pqueues[flavor], &call->tc_pqlink);
	}

	if (old_queue == NULL) {
		enqueue_tail(new_queue, &call->tc_qlink);
	} else if (old_queue != new_queue) {
		re_queue_tail(new_queue, &call->tc_qlink);
	}

	call->tc_queue = new_queue;

	return old_queue;
}

uint64_t
thread_call_get_armed_deadline(thread_call_t call)
{
	return call->tc_pqlink.deadline;
}


static bool
group_isparallel(thread_call_group_t group)
{
	return (group->tcg_flags & TCG_PARALLEL) != 0;
}

static bool
thread_call_group_should_add_thread(thread_call_group_t group)
{
	if ((group->active_count + group->blocked_count + group->idle_count) >= THREAD_CALL_GROUP_MAX_THREADS) {
		panic("thread_call group '%s' reached max thread cap (%d): active: %d, blocked: %d, idle: %d",
		    group->tcg_name, THREAD_CALL_GROUP_MAX_THREADS,
		    group->active_count, group->blocked_count, group->idle_count);
	}

	if (group_isparallel(group) == false) {
		if (group->pending_count > 0 && group->active_count == 0) {
			return true;
		}

		return false;
	}

	if (group->pending_count > 0) {
		if (group->idle_count > 0) {
			return false;
		}

		uint32_t thread_count = group->active_count;

		  
                                                 
                                                  
                                                     
                                                               
                                                     
               
     
		if ((thread_count == 0) ||
		    (thread_count < group->target_thread_count) ||
		    ((group->pending_count > THREAD_CALL_ADD_RATIO * thread_count) &&
		    (sched_mach_factor < THREAD_CALL_MACH_FACTOR_CAP))) {
			return true;
		}
	}

	return false;
}

static void
thread_call_group_setup(thread_call_group_t group)
{
	lck_ticket_init(&group->tcg_lock, &thread_call_lck_grp);

	queue_init(&group->pending_queue);

	for (thread_call_flavor_t flavor = 0; flavor < TCF_COUNT; flavor++) {
		queue_init(&group->delayed_queues[flavor]);
		priority_queue_init(&group->delayed_pqueues[flavor]);
		timer_call_setup(&group->delayed_timers[flavor], thread_call_delayed_timer, group);
	}

	timer_call_setup(&group->dealloc_timer, thread_call_dealloc_timer, group);

	waitq_init(&group->waiters_waitq, SYNC_POLICY_DISABLE_IRQ);

	                                                                                       
	waitq_init(&group->idle_waitq, SYNC_POLICY_REVERSED | SYNC_POLICY_DISABLE_IRQ);
}

  
                                               
                      
   
static void
thread_call_thread_create(
	thread_call_group_t             group)
{
	thread_t thread;
	kern_return_t result;

	int thread_pri = group->tcg_thread_pri;

	result = kernel_thread_start_priority((thread_continue_t)thread_call_thread,
	    group, thread_pri, &thread);
	if (result != KERN_SUCCESS) {
		panic("cannot create new thread call thread %d", result);
	}

	if (thread_pri <= BASEPRI_KERNEL) {
		  
                                                                         
                                                              
     
		thread_set_eager_preempt(thread);
	}

	char name[MAXTHREADNAMESIZE] = "";

	int group_thread_count = group->idle_count + group->active_count + group->blocked_count;

	snprintf(name, sizeof(name), "thread call %s #%d", group->tcg_name, group_thread_count);
	thread_set_thread_name(thread, name);

	thread_deallocate(thread);
}

  
                          
  
                                 
                                      
   
void
thread_call_initialize(void)
{
	nanotime_to_absolutetime(0, THREAD_CALL_DEALLOC_INTERVAL_NS, &thread_call_dealloc_interval_abs);
	waitq_init(&daemon_waitq, SYNC_POLICY_DISABLE_IRQ | SYNC_POLICY_FIFO);

	for (uint32_t i = 0; i < THREAD_CALL_INDEX_MAX; i++) {
		thread_call_group_setup(&thread_call_groups[i]);
	}

	_internal_call_init();

	thread_t thread;
	kern_return_t result;

	result = kernel_thread_start_priority((thread_continue_t)thread_call_daemon,
	    NULL, BASEPRI_PREEMPT_HIGH + 1, &thread);
	if (result != KERN_SUCCESS) {
		panic("thread_call_initialize");
	}

	thread_deallocate(thread);
}

void
thread_call_setup_with_options(
	thread_call_t                   call,
	thread_call_func_t              func,
	thread_call_param_t             param0,
	thread_call_priority_t          pri,
	thread_call_options_t           options)
{
	bzero(call, sizeof(*call));

	*call = (struct thread_call) {
		.tc_func = func,
		.tc_param0 = param0,
	};

	switch (pri) {
	case THREAD_CALL_PRIORITY_HIGH:
		call->tc_index = THREAD_CALL_INDEX_HIGH;
		break;
	case THREAD_CALL_PRIORITY_KERNEL:
		call->tc_index = THREAD_CALL_INDEX_KERNEL;
		break;
	case THREAD_CALL_PRIORITY_USER:
		call->tc_index = THREAD_CALL_INDEX_USER;
		break;
	case THREAD_CALL_PRIORITY_LOW:
		call->tc_index = THREAD_CALL_INDEX_LOW;
		break;
	case THREAD_CALL_PRIORITY_KERNEL_HIGH:
		call->tc_index = THREAD_CALL_INDEX_KERNEL_HIGH;
		break;
	default:
		panic("Invalid thread call pri value: %d", pri);
		break;
	}

	if (options & THREAD_CALL_OPTIONS_ONCE) {
		call->tc_flags |= THREAD_CALL_ONCE;
	}
	if (options & THREAD_CALL_OPTIONS_SIGNAL) {
		call->tc_flags |= THREAD_CALL_SIGNAL | THREAD_CALL_ONCE;
	}
}

void
thread_call_setup(
	thread_call_t                   call,
	thread_call_func_t              func,
	thread_call_param_t             param0)
{
	thread_call_setup_with_options(call, func, param0,
	    THREAD_CALL_PRIORITY_HIGH, 0);
}

static void
_internal_call_init(void)
{
	                                                                        
	thread_call_group_t group = &thread_call_groups[THREAD_CALL_INDEX_HIGH];

	spl_t s = disable_ints_and_lock(group);

	queue_init(&thread_call_internal_queue);

	for (unsigned i = 0; i < INTERNAL_CALL_COUNT; i++) {
		enqueue_tail(&thread_call_internal_queue, &internal_call_storage[i].tc_qlink);
		thread_call_internal_queue_count++;
	}

	enable_ints_and_unlock(group, s);
}

  
                           
  
                                      
  
                                     
   
static thread_call_t
_internal_call_allocate(thread_call_func_t func, thread_call_param_t param0)
{
	                                                                        
	thread_call_group_t group = &thread_call_groups[THREAD_CALL_INDEX_HIGH];

	spl_t s = disable_ints_and_lock(group);

	thread_call_t call = qe_dequeue_head(&thread_call_internal_queue,
	    struct thread_call, tc_qlink);

	if (call == NULL) {
		panic("_internal_call_allocate: thread_call_internal_queue empty");
	}

	thread_call_internal_queue_count--;

	thread_call_setup(call, func, param0);
	                                                         
	assert((call->tc_flags & THREAD_CALL_ALLOC) == 0);
	enable_ints_and_unlock(group, s);

	return call;
}

                                                                                
static bool
_is_internal_call(thread_call_t call)
{
	if (call >= internal_call_storage &&
	    call < &internal_call_storage[INTERNAL_CALL_COUNT]) {
		assert((call->tc_flags & THREAD_CALL_ALLOC) == 0);
		return true;
	}
	return false;
}

  
                          
  
                                          
                                     
  
                                          
   
static void
_internal_call_release(thread_call_t call)
{
	assert(_is_internal_call(call));

	thread_call_group_t group = thread_call_get_group(call);

	assert(group == &thread_call_groups[THREAD_CALL_INDEX_HIGH]);
	thread_call_assert_locked(group);

	enqueue_head(&thread_call_internal_queue, &call->tc_qlink);
	thread_call_internal_queue_count++;
}

  
                         
  
                                   
                                      
  
                                        
              
  
                                     
   
static bool
_pending_call_enqueue(thread_call_t call,
    thread_call_group_t group,
    uint64_t now)
{
	if ((THREAD_CALL_ONCE | THREAD_CALL_RUNNING)
	    == (call->tc_flags & (THREAD_CALL_ONCE | THREAD_CALL_RUNNING))) {
		call->tc_pqlink.deadline = 0;

		thread_call_flags_t flags = call->tc_flags;
		call->tc_flags |= THREAD_CALL_RESCHEDULE;

		assert(call->tc_queue == NULL);

		return flags & THREAD_CALL_RESCHEDULE;
	}

	call->tc_pending_timestamp = now;

	bool was_on_queue = thread_call_enqueue_tail(call, &group->pending_queue);

	if (!was_on_queue) {
		call->tc_submit_count++;
	}

	group->pending_count++;

	thread_call_wake(group);

	return was_on_queue;
}

  
                         
  
                                       
                                         
                                
  
                                        
              
  
                                     
   
static bool
_delayed_call_enqueue(
	thread_call_t           call,
	thread_call_group_t     group,
	uint64_t                deadline,
	thread_call_flavor_t    flavor)
{
	if ((THREAD_CALL_ONCE | THREAD_CALL_RUNNING)
	    == (call->tc_flags & (THREAD_CALL_ONCE | THREAD_CALL_RUNNING))) {
		call->tc_pqlink.deadline = deadline;

		thread_call_flags_t flags = call->tc_flags;
		call->tc_flags |= THREAD_CALL_RESCHEDULE;

		assert(call->tc_queue == NULL);
		thread_call_set_flavor(call, flavor);

		return flags & THREAD_CALL_RESCHEDULE;
	}

	queue_head_t *old_queue = thread_call_enqueue_deadline(call, group, flavor, deadline);

	if (old_queue == &group->pending_queue) {
		group->pending_count--;
	} else if (old_queue == NULL) {
		call->tc_submit_count++;
	}

	return old_queue != NULL;
}

  
                 
  
                                
  
                                            
  
                                     
   
static bool
_call_dequeue(
	thread_call_t           call,
	thread_call_group_t     group)
{
	queue_head_t *old_queue = thread_call_dequeue(call);

	if (old_queue == NULL) {
		return false;
	}

	call->tc_finish_count++;

	if (old_queue == &group->pending_queue) {
		group->pending_count--;
	}

	return true;
}

  
                           
  
                                                        
                     
  
                                                                   
                             
  
                                                                           
                                             
  
                                     
   
static bool
_arm_delayed_call_timer(thread_call_t           new_call,
    thread_call_group_t     group,
    thread_call_flavor_t    flavor)
{
	                                      
	if (queue_empty(&group->delayed_queues[flavor])) {
		return false;
	}

	thread_call_t call = priority_queue_min(&group->delayed_pqueues[flavor], struct thread_call, tc_pqlink);

	                                                                                     
	if (new_call != NULL && new_call != call) {
		return false;
	}

	assert((call->tc_soft_deadline != 0) && ((call->tc_soft_deadline <= call->tc_pqlink.deadline)));

	uint64_t fire_at = call->tc_soft_deadline;

	if (flavor == TCF_CONTINUOUS) {
		assert(call->tc_flags & THREAD_CALL_FLAG_CONTINUOUS);
		fire_at = continuoustime_to_absolutetime(fire_at);
	} else {
		assert((call->tc_flags & THREAD_CALL_FLAG_CONTINUOUS) == 0);
	}

	  
                                                                                   
                                                                                
                                                                       
                                       
    
	uint64_t leeway = call->tc_pqlink.deadline - call->tc_soft_deadline;

	timer_call_enter_with_leeway(&group->delayed_timers[flavor], (timer_call_param_t)flavor,
	    fire_at, leeway,
	    TIMER_CALL_SYS_CRITICAL | TIMER_CALL_LEEWAY,
	    ((call->tc_flags & THREAD_CALL_RATELIMITED) == THREAD_CALL_RATELIMITED));

	return true;
}

  
                           
  
                                     
                                    
  
                                       
              
  
                                     
   
static boolean_t
_cancel_func_from_queue(thread_call_func_t      func,
    thread_call_param_t     param0,
    thread_call_group_t     group,
    boolean_t               remove_all,
    queue_head_t            *queue)
{
	boolean_t call_removed = FALSE;
	thread_call_t call;

	qe_foreach_element_safe(call, queue, tc_qlink) {
		if (call->tc_func != func ||
		    call->tc_param0 != param0) {
			continue;
		}

		_call_dequeue(call, group);

		if (_is_internal_call(call)) {
			_internal_call_release(call);
		}

		call_removed = TRUE;
		if (!remove_all) {
			break;
		}
	}

	return call_removed;
}

  
                            
  
                                
                            
   
void
thread_call_func_delayed(
	thread_call_func_t              func,
	thread_call_param_t             param,
	uint64_t                        deadline)
{
	(void)thread_call_enter_delayed_internal(NULL, func, param, 0, deadline, 0, 0);
}

  
                                        
  
                                               
                                 
   

void
thread_call_func_delayed_with_leeway(
	thread_call_func_t              func,
	thread_call_param_t             param,
	uint64_t                deadline,
	uint64_t                leeway,
	uint32_t                flags)
{
	(void)thread_call_enter_delayed_internal(NULL, func, param, 0, deadline, leeway, flags);
}

  
                           
  
                              
  
                                              
                                    
                                     
                 
  
                                            
  
                                                                         
                                                                            
  
                                                                            
                                
   
boolean_t
thread_call_func_cancel(
	thread_call_func_t              func,
	thread_call_param_t             param,
	boolean_t                       cancel_all)
{
	boolean_t       result;

	assert(func != NULL);

	                                                                        
	thread_call_group_t group = &thread_call_groups[THREAD_CALL_INDEX_HIGH];

	spl_t s = disable_ints_and_lock(group);

	if (cancel_all) {
		                                                                                    
		result = _cancel_func_from_queue(func, param, group, cancel_all, &group->pending_queue) |
		    _cancel_func_from_queue(func, param, group, cancel_all, &group->delayed_queues[TCF_ABSOLUTE])  |
		    _cancel_func_from_queue(func, param, group, cancel_all, &group->delayed_queues[TCF_CONTINUOUS]);
	} else {
		                                                                        
		result = _cancel_func_from_queue(func, param, group, cancel_all, &group->pending_queue) ||
		    _cancel_func_from_queue(func, param, group, cancel_all, &group->delayed_queues[TCF_ABSOLUTE]) ||
		    _cancel_func_from_queue(func, param, group, cancel_all, &group->delayed_queues[TCF_CONTINUOUS]);
	}

	enable_ints_and_unlock(group, s);

	return result;
}

  
                                                                        
                                                                                       
                                                                                       
                                                       
   
thread_call_t
thread_call_allocate_with_priority(
	thread_call_func_t              func,
	thread_call_param_t             param0,
	thread_call_priority_t          pri)
{
	return thread_call_allocate_with_options(func, param0, pri, 0);
}

thread_call_t
thread_call_allocate_with_options(
	thread_call_func_t              func,
	thread_call_param_t             param0,
	thread_call_priority_t          pri,
	thread_call_options_t           options)
{
	thread_call_t call = zalloc(thread_call_zone);

	thread_call_setup_with_options(call, func, param0, pri, options);
	call->tc_refs = 1;
	call->tc_flags |= THREAD_CALL_ALLOC;

	return call;
}

thread_call_t
thread_call_allocate_with_qos(thread_call_func_t        func,
    thread_call_param_t       param0,
    int                       qos_tier,
    thread_call_options_t     options)
{
	thread_call_t call = thread_call_allocate(func, param0);

	switch (qos_tier) {
	case THREAD_QOS_UNSPECIFIED:
		call->tc_index = THREAD_CALL_INDEX_HIGH;
		break;
	case THREAD_QOS_LEGACY:
		call->tc_index = THREAD_CALL_INDEX_USER;
		break;
	case THREAD_QOS_MAINTENANCE:
	case THREAD_QOS_BACKGROUND:
		call->tc_index = THREAD_CALL_INDEX_LOW;
		break;
	case THREAD_QOS_UTILITY:
		call->tc_index = THREAD_CALL_INDEX_QOS_UT;
		break;
	case THREAD_QOS_USER_INITIATED:
		call->tc_index = THREAD_CALL_INDEX_QOS_IN;
		break;
	case THREAD_QOS_USER_INTERACTIVE:
		call->tc_index = THREAD_CALL_INDEX_QOS_UI;
		break;
	default:
		panic("Invalid thread call qos value: %d", qos_tier);
		break;
	}

	if (options & THREAD_CALL_OPTIONS_ONCE) {
		call->tc_flags |= THREAD_CALL_ONCE;
	}

	                                                 

	return call;
}


  
                        
  
                            
   
thread_call_t
thread_call_allocate(
	thread_call_func_t              func,
	thread_call_param_t             param0)
{
	return thread_call_allocate_with_options(func, param0,
	           THREAD_CALL_PRIORITY_HIGH, 0);
}

  
                    
  
                                                  
                                                   
          
  
                                                        
                                                 
                                                                   
   
boolean_t
thread_call_free(
	thread_call_t           call)
{
	thread_call_group_t group = thread_call_get_group(call);

	spl_t s = disable_ints_and_lock(group);

	if (call->tc_queue != NULL ||
	    ((call->tc_flags & THREAD_CALL_RESCHEDULE) != 0)) {
		thread_call_unlock(group);
		splx(s);

		return FALSE;
	}

	int32_t refs = --call->tc_refs;
	if (refs < 0) {
		panic("Refcount negative: %d\n", refs);
	}

	if ((THREAD_CALL_SIGNAL | THREAD_CALL_RUNNING)
	    == ((THREAD_CALL_SIGNAL | THREAD_CALL_RUNNING) & call->tc_flags)) {
		thread_call_wait_once_locked(call, s);
		                                        
	} else {
		enable_ints_and_unlock(group, s);
	}

	if (refs == 0) {
		assert(call->tc_finish_count == call->tc_submit_count);
		zfree(thread_call_zone, call);
	}

	return TRUE;
}

  
                     
  
                                           
  
                               
                      
   
boolean_t
thread_call_enter(
	thread_call_t           call)
{
	return thread_call_enter1(call, 0);
}

boolean_t
thread_call_enter1(
	thread_call_t                   call,
	thread_call_param_t             param1)
{
	assert(call->tc_func != NULL);
	assert((call->tc_flags & THREAD_CALL_SIGNAL) == 0);

	thread_call_group_t group = thread_call_get_group(call);
	bool result = true;

	spl_t s = disable_ints_and_lock(group);

	if (call->tc_queue != &group->pending_queue) {
		result = _pending_call_enqueue(call, group, mach_absolute_time());
	}

	call->tc_param1 = param1;

	enable_ints_and_unlock(group, s);

	return result;
}

  
                             
  
                                   
                      
  
                               
                      
   
boolean_t
thread_call_enter_delayed(
	thread_call_t           call,
	uint64_t                deadline)
{
	assert(call != NULL);
	return thread_call_enter_delayed_internal(call, NULL, 0, 0, deadline, 0, 0);
}

boolean_t
thread_call_enter1_delayed(
	thread_call_t                   call,
	thread_call_param_t             param1,
	uint64_t                        deadline)
{
	assert(call != NULL);
	return thread_call_enter_delayed_internal(call, NULL, 0, param1, deadline, 0, 0);
}

boolean_t
thread_call_enter_delayed_with_leeway(
	thread_call_t           call,
	thread_call_param_t     param1,
	uint64_t                deadline,
	uint64_t                leeway,
	unsigned int            flags)
{
	assert(call != NULL);
	return thread_call_enter_delayed_internal(call, NULL, 0, param1, deadline, leeway, flags);
}


  
                                      
                                                      
  
                                                  
          
                                                          
                                                                                           
                                          
                                                           
                                                                                              
                                                                                      
                                                                                                         
                                                                                                 
   
boolean_t
thread_call_enter_delayed_internal(
	thread_call_t           call,
	thread_call_func_t      alt_func,
	thread_call_param_t     alt_param0,
	thread_call_param_t     param1,
	uint64_t                deadline,
	uint64_t                leeway,
	unsigned int            flags)
{
	uint64_t                now, sdeadline;

	thread_call_flavor_t flavor = (flags & THREAD_CALL_CONTINUOUS) ? TCF_CONTINUOUS : TCF_ABSOLUTE;

	                                                                                
	uint32_t urgency = (flags & TIMEOUT_URGENCY_MASK);

	if (call == NULL) {
		                                                                                    
		call = _internal_call_allocate(alt_func, alt_param0);
	}

	assert(call->tc_func != NULL);
	thread_call_group_t group = thread_call_get_group(call);

	spl_t s = disable_ints_and_lock(group);

	  
                                                                               
                                                       
    
	if (flavor == TCF_CONTINUOUS) {
		now = mach_continuous_time();
	} else {
		now = mach_absolute_time();
	}

	call->tc_flags |= THREAD_CALL_DELAYED;

	call->tc_soft_deadline = sdeadline = deadline;

	boolean_t ratelimited = FALSE;
	uint64_t slop = timer_call_slop(deadline, now, urgency, current_thread(), &ratelimited);

	if ((flags & THREAD_CALL_DELAY_LEEWAY) != 0 && leeway > slop) {
		slop = leeway;
	}

	if (UINT64_MAX - deadline <= slop) {
		deadline = UINT64_MAX;
	} else {
		deadline += slop;
	}

	if (ratelimited) {
		call->tc_flags |= THREAD_CALL_RATELIMITED;
	} else {
		call->tc_flags &= ~THREAD_CALL_RATELIMITED;
	}

	call->tc_param1 = param1;

	call->tc_ttd = (sdeadline > now) ? (sdeadline - now) : 0;

	bool result = _delayed_call_enqueue(call, group, deadline, flavor);

	_arm_delayed_call_timer(call, group, flavor);

#if CONFIG_DTRACE
	DTRACE_TMR5(thread_callout__create, thread_call_func_t, call->tc_func,
	    uint64_t, (deadline - sdeadline), uint64_t, (call->tc_ttd >> 32),
	    (unsigned) (call->tc_ttd & 0xFFFFFFFF), call);
#endif

	enable_ints_and_unlock(group, s);

	return result;
}

  
                                        
                                    
   
static bool
thread_call_cancel_locked(thread_call_t call)
{
	bool canceled;

	if (call->tc_flags & THREAD_CALL_RESCHEDULE) {
		call->tc_flags &= ~THREAD_CALL_RESCHEDULE;
		canceled = true;

		                                                         
		assert(call->tc_queue == NULL);
	} else {
		bool queue_head_changed = false;

		thread_call_flavor_t flavor = thread_call_get_flavor(call);
		thread_call_group_t  group  = thread_call_get_group(call);

		if (call->tc_pqlink.deadline != 0 &&
		    call == priority_queue_min(&group->delayed_pqueues[flavor], struct thread_call, tc_pqlink)) {
			assert(call->tc_queue == &group->delayed_queues[flavor]);
			queue_head_changed = true;
		}

		canceled = _call_dequeue(call, group);

		if (queue_head_changed) {
			if (_arm_delayed_call_timer(NULL, group, flavor) == false) {
				timer_call_cancel(&group->delayed_timers[flavor]);
			}
		}
	}

#if CONFIG_DTRACE
	DTRACE_TMR4(thread_callout__cancel, thread_call_func_t, call->tc_func,
	    0, (call->tc_ttd >> 32), (unsigned) (call->tc_ttd & 0xFFFFFFFF));
#endif

	return canceled;
}

  
                      
  
                           
  
                               
              
   
boolean_t
thread_call_cancel(thread_call_t call)
{
	thread_call_group_t group = thread_call_get_group(call);

	spl_t s = disable_ints_and_lock(group);

	boolean_t result = thread_call_cancel_locked(call);

	enable_ints_and_unlock(group, s);

	return result;
}

  
                                                         
                                                              
                                                               
                                                                    
                                                            
                                                             
   
boolean_t
thread_call_cancel_wait(thread_call_t call)
{
	thread_call_group_t group = thread_call_get_group(call);

	if ((call->tc_flags & THREAD_CALL_ALLOC) == 0) {
		panic("thread_call_cancel_wait: can't wait on thread call whose storage I don't own");
	}

	if (!ml_get_interrupts_enabled()) {
		panic("unsafe thread_call_cancel_wait");
	}

	thread_t self = current_thread();

	if ((thread_get_tag_internal(self) & THREAD_TAG_CALLOUT) &&
	    self->thc_state && self->thc_state->thc_call == call) {
		panic("thread_call_cancel_wait: deadlock waiting on self from inside call: %p to function %p",
		    call, call->tc_func);
	}

	spl_t s = disable_ints_and_lock(group);

	boolean_t canceled = thread_call_cancel_locked(call);

	if ((call->tc_flags & THREAD_CALL_ONCE) == THREAD_CALL_ONCE) {
		  
                                                    
                                                     
     

		thread_call_wait_once_locked(call, s);
		                               
	} else {
		  
                                                                          
                                           
    
                                                                          
     

		if (canceled == FALSE) {
			thread_call_wait_locked(call, s);
			                               
		} else {
			enable_ints_and_unlock(group, s);
		}
	}

	return canceled;
}


  
                    
  
                                
                                  
                                
                                  
  
                                     
  
                                                                             
           
   
static void
thread_call_wake(
	thread_call_group_t             group)
{
	  
                                                
                                                          
    
	if (group_isparallel(group) || group->active_count == 0) {
		if (group->idle_count) {
			__assert_only kern_return_t kr;

			kr = waitq_wakeup64_one(&group->idle_waitq, CAST_EVENT64_T(group),
			    THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);
			assert(kr == KERN_SUCCESS);

			group->idle_count--;
			group->active_count++;

			if (group->idle_count == 0 && (group->tcg_flags & TCG_DEALLOC_ACTIVE) == TCG_DEALLOC_ACTIVE) {
				if (timer_call_cancel(&group->dealloc_timer) == TRUE) {
					group->tcg_flags &= ~TCG_DEALLOC_ACTIVE;
				}
			}
		} else {
			if (thread_call_group_should_add_thread(group) &&
			    os_atomic_cmpxchg(&thread_call_daemon_awake,
			    false, true, relaxed)) {
				waitq_wakeup64_all(&daemon_waitq, CAST_EVENT64_T(&thread_call_daemon_awake),
				    THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);
			}
		}
	}
}

  
                     
  
                                     
   
static void
sched_call_thread(
	int                             type,
	thread_t                thread)
{
	thread_call_group_t             group;

	assert(thread_get_tag_internal(thread) & THREAD_TAG_CALLOUT);
	assert(thread->thc_state != NULL);

	group = thread->thc_state->thc_group;
	assert((group - &thread_call_groups[0]) < THREAD_CALL_INDEX_MAX);

	thread_call_lock_spin(group);

	switch (type) {
	case SCHED_CALL_BLOCK:
		assert(group->active_count);
		--group->active_count;
		group->blocked_count++;
		if (group->pending_count > 0) {
			thread_call_wake(group);
		}
		break;

	case SCHED_CALL_UNBLOCK:
		assert(group->blocked_count);
		--group->blocked_count;
		group->active_count++;
		break;
	}

	thread_call_unlock(group);
}

  
                                                        
                                                              
                                                             
                                  
   
static bool
thread_call_finish(thread_call_t call, thread_call_group_t group, spl_t *s)
{
	assert(thread_call_get_group(call) == group);

	bool repend = false;
	bool signal = call->tc_flags & THREAD_CALL_SIGNAL;
	bool alloc = call->tc_flags & THREAD_CALL_ALLOC;

	call->tc_finish_count++;

	if (!signal && alloc) {
		                                                                  
		if (call->tc_refs <= 0) {
			panic("thread_call_finish: detected over-released thread call: %p", call);
		}
		call->tc_refs--;
	}

	thread_call_flags_t old_flags = call->tc_flags;
	call->tc_flags &= ~(THREAD_CALL_RESCHEDULE | THREAD_CALL_RUNNING | THREAD_CALL_WAIT);

	if ((!alloc || call->tc_refs != 0) &&
	    (old_flags & THREAD_CALL_RESCHEDULE) != 0) {
		assert(old_flags & THREAD_CALL_ONCE);
		thread_call_flavor_t flavor = thread_call_get_flavor(call);

		if (old_flags & THREAD_CALL_DELAYED) {
			uint64_t now = mach_absolute_time();
			if (flavor == TCF_CONTINUOUS) {
				now = absolutetime_to_continuoustime(now);
			}
			if (call->tc_soft_deadline <= now) {
				                                                              
				call->tc_flags &= ~(THREAD_CALL_DELAYED | THREAD_CALL_RATELIMITED);
				call->tc_pqlink.deadline = 0;
			}
		}

		if (call->tc_pqlink.deadline) {
			_delayed_call_enqueue(call, group, call->tc_pqlink.deadline, flavor);
			if (!signal) {
				_arm_delayed_call_timer(call, group, flavor);
			}
		} else if (signal) {
			call->tc_submit_count++;
			repend = true;
		} else {
			_pending_call_enqueue(call, group, mach_absolute_time());
		}
	}

	if (!signal && alloc && call->tc_refs == 0) {
		if ((old_flags & THREAD_CALL_WAIT) != 0) {
			panic("Someone waiting on a thread call that is scheduled for free: %p\n", call->tc_func);
		}

		assert(call->tc_finish_count == call->tc_submit_count);

		enable_ints_and_unlock(group, *s);

		zfree(thread_call_zone, call);

		*s = disable_ints_and_lock(group);
	}

	if ((old_flags & THREAD_CALL_WAIT) != 0) {
		  
                                                            
                                                             
                          
    
                                                              
                                                            
                                                            
     
		thread_call_unlock(group);

		waitq_wakeup64_all(&group->waiters_waitq, CAST_EVENT64_T(call),
		    THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);

		thread_call_lock_spin(group);
		                                                 
	}

	return repend;
}

  
                     
  
                                                    
  
                                                                                                        
   
static void __attribute__((noinline))
thread_call_invoke(thread_call_func_t func,
    thread_call_param_t param0,
    thread_call_param_t param1,
    __unused thread_call_t call)
{
#if DEVELOPMENT || DEBUG
	KERNEL_DEBUG_CONSTANT(
		MACHDBG_CODE(DBG_MACH_SCHED, MACH_CALLOUT) | DBG_FUNC_START,
		VM_KERNEL_UNSLIDE(func), VM_KERNEL_ADDRHIDE(param0), VM_KERNEL_ADDRHIDE(param1), 0, 0);
#endif                           

#if CONFIG_DTRACE
	uint64_t tc_ttd = call->tc_ttd;
	boolean_t is_delayed = call->tc_flags & THREAD_CALL_DELAYED;
	DTRACE_TMR6(thread_callout__start, thread_call_func_t, func, int, 0, int, (tc_ttd >> 32),
	    (unsigned) (tc_ttd & 0xFFFFFFFF), is_delayed, call);
#endif

	(*func)(param0, param1);

#if CONFIG_DTRACE
	DTRACE_TMR6(thread_callout__end, thread_call_func_t, func, int, 0, int, (tc_ttd >> 32),
	    (unsigned) (tc_ttd & 0xFFFFFFFF), is_delayed, call);
#endif

#if DEVELOPMENT || DEBUG
	KERNEL_DEBUG_CONSTANT(
		MACHDBG_CODE(DBG_MACH_SCHED, MACH_CALLOUT) | DBG_FUNC_END,
		VM_KERNEL_UNSLIDE(func), 0, 0, 0, 0);
#endif                           
}

  
                      
   
static void
thread_call_thread(
	thread_call_group_t             group,
	wait_result_t                   wres)
{
	thread_t self = current_thread();

	if ((thread_get_tag_internal(self) & THREAD_TAG_CALLOUT) == 0) {
		(void)thread_set_tag_internal(self, THREAD_TAG_CALLOUT);
	}

	  
                                                   
                        
    
	if (wres == THREAD_INTERRUPTED) {
		thread_terminate(self);

		                
		panic("thread_terminate() returned?");
	}

	spl_t s = disable_ints_and_lock(group);

	struct thread_call_thread_state thc_state = { .thc_group = group };
	self->thc_state = &thc_state;

	thread_sched_call(self, sched_call_thread);

	while (group->pending_count > 0) {
		thread_call_t call = qe_dequeue_head(&group->pending_queue,
		    struct thread_call, tc_qlink);
		assert(call != NULL);

		group->pending_count--;
		if (group->pending_count == 0) {
			assert(queue_empty(&group->pending_queue));
		}

		thread_call_func_t  func   = call->tc_func;
		thread_call_param_t param0 = call->tc_param0;
		thread_call_param_t param1 = call->tc_param1;

		call->tc_queue = NULL;

		if (_is_internal_call(call)) {
			_internal_call_release(call);
		}

		  
                                                       
                
     
		bool needs_finish = false;
		if (call->tc_flags & THREAD_CALL_ALLOC) {
			call->tc_refs++;                                         
		}
		if (call->tc_flags & (THREAD_CALL_ALLOC | THREAD_CALL_ONCE)) {
			  
                                                       
                                                         
                                                 
                      
     
                                                      
                                                
      
			needs_finish = true;
			call->tc_flags |= THREAD_CALL_RUNNING;
		}

		thc_state.thc_call = call;
		thc_state.thc_call_pending_timestamp = call->tc_pending_timestamp;
		thc_state.thc_call_soft_deadline = call->tc_soft_deadline;
		thc_state.thc_call_hard_deadline = call->tc_pqlink.deadline;
		thc_state.thc_func = func;
		thc_state.thc_param0 = param0;
		thc_state.thc_param1 = param1;
		thc_state.thc_IOTES_invocation_timestamp = 0;

		enable_ints_and_unlock(group, s);

		thc_state.thc_call_start = mach_absolute_time();

		thread_call_invoke(func, param0, param1, call);

		thc_state.thc_call = NULL;

		if (get_preemption_level() != 0) {
			int pl = get_preemption_level();
			panic("thread_call_thread: preemption_level %d, last callout %p(%p, %p)",
			    pl, (void *)VM_KERNEL_UNSLIDE(func), param0, param1);
		}

		s = disable_ints_and_lock(group);

		if (needs_finish) {
			                                                           
			thread_call_finish(call, group, &s);
		}
	}

	thread_sched_call(self, NULL);
	group->active_count--;

	if (self->callout_woken_from_icontext && !self->callout_woke_thread) {
		ledger_credit(self->t_ledger, task_ledgers.interrupt_wakeups, 1);
		if (self->callout_woken_from_platform_idle) {
			ledger_credit(self->t_ledger, task_ledgers.platform_idle_wakeups, 1);
		}
	}

	self->callout_woken_from_icontext = FALSE;
	self->callout_woken_from_platform_idle = FALSE;
	self->callout_woke_thread = FALSE;

	self->thc_state = NULL;

	if (group_isparallel(group)) {
		  
                                                         
                                                       
                                                        
                                                         
                                
     
		group->idle_count++;

		if (group->idle_count == 1) {
			group->idle_timestamp = mach_absolute_time();
		}

		if (((group->tcg_flags & TCG_DEALLOC_ACTIVE) == 0) &&
		    ((group->active_count + group->idle_count) > group->target_thread_count)) {
			thread_call_start_deallocate_timer(group);
		}

		                                         
		wres = waitq_assert_wait64(&group->idle_waitq, CAST_EVENT64_T(group), THREAD_INTERRUPTIBLE, 0);
		if (wres != THREAD_WAITING) {
			panic("kcall worker unable to assert wait?");
		}

		enable_ints_and_unlock(group, s);

		thread_block_parameter((thread_continue_t)thread_call_thread, group);
	} else {
		if (group->idle_count < group->target_thread_count) {
			group->idle_count++;

			waitq_assert_wait64(&group->idle_waitq, CAST_EVENT64_T(group), THREAD_UNINT, 0);                                

			enable_ints_and_unlock(group, s);

			thread_block_parameter((thread_continue_t)thread_call_thread, group);
			                
		}
	}

	enable_ints_and_unlock(group, s);

	thread_terminate(self);
	                
}

void
thread_call_start_iotes_invocation(__assert_only thread_call_t call)
{
	thread_t self = current_thread();

	if ((thread_get_tag_internal(self) & THREAD_TAG_CALLOUT) == 0) {
		                                                         
		return;
	}

	assert(self->thc_state);
	assert(self->thc_state->thc_call == call);

	self->thc_state->thc_IOTES_invocation_timestamp = mach_absolute_time();
}


  
                                                      
                                           
                                          
   
static void
thread_call_daemon_continue(__unused void *arg)
{
	do {
		os_atomic_store(&thread_call_daemon_awake, false, relaxed);

		                                                         
		for (int i = 0; i < THREAD_CALL_INDEX_MAX; i++) {
			thread_call_group_t group = &thread_call_groups[i];

			spl_t s = disable_ints_and_lock(group);

			while (thread_call_group_should_add_thread(group)) {
				group->active_count++;

				enable_ints_and_unlock(group, s);

				thread_call_thread_create(group);

				s = disable_ints_and_lock(group);
			}

			enable_ints_and_unlock(group, s);
		}
	} while (os_atomic_load(&thread_call_daemon_awake, relaxed));

	waitq_assert_wait64(&daemon_waitq, CAST_EVENT64_T(&thread_call_daemon_awake), THREAD_UNINT, 0);

	if (os_atomic_load(&thread_call_daemon_awake, relaxed)) {
		clear_wait(current_thread(), THREAD_AWAKENED);
	}

	thread_block_parameter((thread_continue_t)thread_call_daemon_continue, NULL);
	                
}

static void
thread_call_daemon(
	__unused void    *arg)
{
	thread_t        self = current_thread();

	self->options |= TH_OPT_VMPRIV;
	vm_page_free_reserve(2);                 

	thread_set_thread_name(self, "thread_call_daemon");

	thread_call_daemon_continue(NULL);
	                
}

  
                                                                    
                                                                       
                          
   
static void
thread_call_start_deallocate_timer(thread_call_group_t group)
{
	__assert_only bool already_enqueued;

	assert(group->idle_count > 0);
	assert((group->tcg_flags & TCG_DEALLOC_ACTIVE) == 0);

	group->tcg_flags |= TCG_DEALLOC_ACTIVE;

	uint64_t deadline = group->idle_timestamp + thread_call_dealloc_interval_abs;

	already_enqueued = timer_call_enter(&group->dealloc_timer, deadline, 0);

	assert(already_enqueued == false);
}

                                                                       
void
thread_call_delayed_timer(timer_call_param_t p0, timer_call_param_t p1)
{
	thread_call_group_t  group  = (thread_call_group_t)  p0;
	thread_call_flavor_t flavor = (thread_call_flavor_t) p1;

	thread_call_t   call;
	uint64_t        now;

	thread_call_lock_spin(group);

	if (flavor == TCF_CONTINUOUS) {
		now = mach_continuous_time();
	} else if (flavor == TCF_ABSOLUTE) {
		now = mach_absolute_time();
	} else {
		panic("invalid timer flavor: %d", flavor);
	}

	while ((call = priority_queue_min(&group->delayed_pqueues[flavor],
	    struct thread_call, tc_pqlink)) != NULL) {
		assert(thread_call_get_group(call) == group);
		assert(thread_call_get_flavor(call) == flavor);

		  
                                                     
                            
                                                                
                                            
     
		if (call->tc_soft_deadline > now) {
			break;
		}

		  
                                                              
                                                        
    
                                                      
                                                           
     
		if ((call->tc_flags & THREAD_CALL_RATELIMITED) &&
		    (call->tc_pqlink.deadline > now) &&
		    (ml_timer_forced_evaluation() == FALSE)) {
			break;
		}

		if (THREAD_CALL_SIGNAL & call->tc_flags) {
			__assert_only queue_head_t *old_queue;
			old_queue = thread_call_dequeue(call);
			assert(old_queue == &group->delayed_queues[flavor]);

			do {
				thread_call_func_t  func   = call->tc_func;
				thread_call_param_t param0 = call->tc_param0;
				thread_call_param_t param1 = call->tc_param1;

				call->tc_flags |= THREAD_CALL_RUNNING;

				thread_call_unlock(group);
				thread_call_invoke(func, param0, param1, call);
				thread_call_lock_spin(group);

				                                                        
			} while (thread_call_finish(call, group, NULL));
			                                            
		} else {
			_pending_call_enqueue(call, group, now);
		}
	}

	_arm_delayed_call_timer(call, group, flavor);

	thread_call_unlock(group);
}

static void
thread_call_delayed_timer_rescan(thread_call_group_t group,
    thread_call_flavor_t flavor)
{
	thread_call_t call;
	uint64_t now;

	spl_t s = disable_ints_and_lock(group);

	assert(ml_timer_forced_evaluation() == TRUE);

	if (flavor == TCF_CONTINUOUS) {
		now = mach_continuous_time();
	} else {
		now = mach_absolute_time();
	}

	qe_foreach_element_safe(call, &group->delayed_queues[flavor], tc_qlink) {
		if (call->tc_soft_deadline <= now) {
			_pending_call_enqueue(call, group, now);
		} else {
			uint64_t skew = call->tc_pqlink.deadline - call->tc_soft_deadline;
			assert(call->tc_pqlink.deadline >= call->tc_soft_deadline);
			  
                                                   
                                                            
                                                 
     
                                                        
                                                    
                                                
                         
     
                                                                              
                                                            
     
                                                             
                                                  
      
			if (timer_resort_threshold(skew)) {
				_call_dequeue(call, group);
				_delayed_call_enqueue(call, group, call->tc_soft_deadline, flavor);
			}
		}
	}

	_arm_delayed_call_timer(NULL, group, flavor);

	enable_ints_and_unlock(group, s);
}

void
thread_call_delayed_timer_rescan_all(void)
{
	for (int i = 0; i < THREAD_CALL_INDEX_MAX; i++) {
		for (thread_call_flavor_t flavor = 0; flavor < TCF_COUNT; flavor++) {
			thread_call_delayed_timer_rescan(&thread_call_groups[i], flavor);
		}
	}
}

  
                                                  
                                                         
                        
   
static void
thread_call_dealloc_timer(
	timer_call_param_t              p0,
	__unused timer_call_param_t     p1)
{
	thread_call_group_t group = (thread_call_group_t)p0;
	uint64_t now;
	kern_return_t res;
	bool terminated = false;

	thread_call_lock_spin(group);

	assert(group->tcg_flags & TCG_DEALLOC_ACTIVE);

	now = mach_absolute_time();

	if (group->idle_count > 0) {
		if (now > group->idle_timestamp + thread_call_dealloc_interval_abs) {
			terminated = true;
			group->idle_count--;
			res = waitq_wakeup64_one(&group->idle_waitq, CAST_EVENT64_T(group),
			    THREAD_INTERRUPTED, WAITQ_ALL_PRIORITIES);
			if (res != KERN_SUCCESS) {
				panic("Unable to wake up idle thread for termination?");
			}
		}
	}

	group->tcg_flags &= ~TCG_DEALLOC_ACTIVE;

	  
                                                           
                                
    
	if (group->idle_count > 0 && (group->idle_count + group->active_count > group->target_thread_count)) {
		  
                                                
                   
     
		if (terminated) {
			group->idle_timestamp = now;
		}

		thread_call_start_deallocate_timer(group);
	}

	thread_call_unlock(group);
}

  
                                                         
                                                                 
  
                                                                   
  
                                                                               
                                                                      
                                                                                
  
                                                      
                                                                                 
  
                                                                                    
                                                                            
                                                                           
                                    
   
static bool
thread_call_wait_once_locked(thread_call_t call, spl_t s)
{
	assert(call->tc_flags & THREAD_CALL_ALLOC);
	assert(call->tc_flags & THREAD_CALL_ONCE);

	thread_call_group_t group = thread_call_get_group(call);

	if ((call->tc_flags & THREAD_CALL_RUNNING) == 0) {
		enable_ints_and_unlock(group, s);
		return false;
	}

	                                                
	call->tc_flags |= THREAD_CALL_WAIT;

	wait_result_t res = waitq_assert_wait64(&group->waiters_waitq, CAST_EVENT64_T(call), THREAD_UNINT, 0);
	if (res != THREAD_WAITING) {
		panic("Unable to assert wait: %d", res);
	}

	enable_ints_and_unlock(group, s);

	res = thread_block(THREAD_CONTINUE_NULL);
	if (res != THREAD_AWAKENED) {
		panic("Awoken with %d?", res);
	}

	                      
	return true;
}

  
                                               
                                                                   
                                    
  
                                         
  
                                         
   
boolean_t
thread_call_wait_once(thread_call_t call)
{
	if ((call->tc_flags & THREAD_CALL_ALLOC) == 0) {
		panic("thread_call_wait_once: can't wait on thread call whose storage I don't own");
	}

	if ((call->tc_flags & THREAD_CALL_ONCE) == 0) {
		panic("thread_call_wait_once: can't wait_once on a non-once call");
	}

	if (!ml_get_interrupts_enabled()) {
		panic("unsafe thread_call_wait_once");
	}

	thread_t self = current_thread();

	if ((thread_get_tag_internal(self) & THREAD_TAG_CALLOUT) &&
	    self->thc_state && self->thc_state->thc_call == call) {
		panic("thread_call_wait_once: deadlock waiting on self from inside call: %p to function %p",
		    call, call->tc_func);
	}

	thread_call_group_t group = thread_call_get_group(call);

	spl_t s = disable_ints_and_lock(group);

	bool waited = thread_call_wait_once_locked(call, s);
	                               

	return waited;
}


  
                                                                   
                                                                           
                                                                          
                                
  
                                                                  
   
static void
thread_call_wait_locked(thread_call_t call, spl_t s)
{
	thread_call_group_t group = thread_call_get_group(call);

	assert(call->tc_flags & THREAD_CALL_ALLOC);

	uint64_t submit_count = call->tc_submit_count;

	while (call->tc_finish_count < submit_count) {
		call->tc_flags |= THREAD_CALL_WAIT;

		wait_result_t res = waitq_assert_wait64(&group->waiters_waitq,
		    CAST_EVENT64_T(call), THREAD_UNINT, 0);

		if (res != THREAD_WAITING) {
			panic("Unable to assert wait: %d", res);
		}

		enable_ints_and_unlock(group, s);

		res = thread_block(THREAD_CONTINUE_NULL);
		if (res != THREAD_AWAKENED) {
			panic("Awoken with %d?", res);
		}

		s = disable_ints_and_lock(group);
	}

	enable_ints_and_unlock(group, s);
}

  
                                                          
                            
   
boolean_t
thread_call_isactive(thread_call_t call)
{
	thread_call_group_t group = thread_call_get_group(call);

	spl_t s = disable_ints_and_lock(group);
	boolean_t active = (call->tc_submit_count > call->tc_finish_count);
	enable_ints_and_unlock(group, s);

	return active;
}

  
                                
                                                                               
   
void
adjust_cont_time_thread_calls(void)
{
	for (int i = 0; i < THREAD_CALL_INDEX_MAX; i++) {
		thread_call_group_t group = &thread_call_groups[i];
		spl_t s = disable_ints_and_lock(group);

		                                                    

		_arm_delayed_call_timer(NULL, group, TCF_CONTINUOUS);
		enable_ints_and_unlock(group, s);
	}
}
