/*
 * Copyright (c) 1993-2008 Apple Inc. All rights reserved.
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
  
                                  
   

#include <mach/mach_types.h>

#include <kern/clock.h>
#include <kern/smp.h>
#include <kern/processor.h>
#include <kern/timer_call.h>
#include <kern/timer_queue.h>
#include <kern/thread.h>
#include <kern/policy_internal.h>

#include <sys/kdebug.h>

#if CONFIG_DTRACE
#include <mach/sdt.h>
#endif


#if DEBUG
#define TIMER_ASSERT    1
#endif

                        
                     

#if TIMER_DBG
#define DBG(x...) kprintf("DBG: " x);
#else
#define DBG(x...)
#endif

#if TIMER_TRACE
#define TIMER_KDEBUG_TRACE      KERNEL_DEBUG_CONSTANT_IST
#else
#define TIMER_KDEBUG_TRACE(x...)
#endif

LCK_GRP_DECLARE(timer_call_lck_grp, "timer_call");
LCK_GRP_DECLARE(timer_longterm_lck_grp, "timer_longterm");

                                                                                   
#define timer_queue_lock_spin(queue)                                    \
	lck_mtx_lock_spin_always(&queue->lock_data)

#define timer_queue_unlock(queue)               \
	lck_mtx_unlock_always(&queue->lock_data)

  
                                                                     
                                                                         
                                                                
                                                           
   

                                 
#define TIMER_LONGTERM_NONE             EndOfAllTime
                                                                            
#if defined(__x86_64__)
#define TIMER_LONGTERM_THRESHOLD        (1ULL * NSEC_PER_SEC)              
#else
#define TIMER_LONGTERM_THRESHOLD        TIMER_LONGTERM_NONE                   
#endif

  
                                                             
                                                            
                                                                  
                                            
   
#define TIMER_LONGTERM_SCAN_LIMIT       (100ULL * NSEC_PER_USEC)                    
#define TIMER_LONGTERM_SCAN_INTERVAL    (100ULL * NSEC_PER_USEC)                    
                                         
#define TIMER_LONGTERM_SCAN_AGAIN       0

typedef struct {
	uint64_t        interval;                                    
	uint64_t        margin;                                            
	uint64_t        deadline;                                            
	uint64_t        preempted;                                       
	timer_call_t    call;                                                  
	uint64_t        deadline_set;                       
	timer_call_data_t timer;                                                
	                                            
	uint64_t        scans;                                           
	uint64_t        preempts;                                       
	uint64_t        latency;                                         
	uint64_t        latency_min;                                     
	uint64_t        latency_max;                                     
} threshold_t;

typedef struct {
	mpqueue_head_t  queue;                                   
	uint64_t        enqueues;                              
	uint64_t        dequeues;                                
	uint64_t        escalates;                                         
	uint64_t        scan_time;                                          
	threshold_t     threshold;                                    
	uint64_t        scan_limit;                            
	uint64_t        scan_interval;                                              
	uint64_t        scan_pauses;                                        
} timer_longterm_t;

timer_longterm_t                timer_longterm = {
	.scan_limit = TIMER_LONGTERM_SCAN_LIMIT,
	.scan_interval = TIMER_LONGTERM_SCAN_INTERVAL,
};

static mpqueue_head_t           *timer_longterm_queue = NULL;

static void                     timer_longterm_init(void);
static void                     timer_longterm_callout(
	timer_call_param_t      p0,
	timer_call_param_t      p1);
extern void                     timer_longterm_scan(
	timer_longterm_t        *tlp,
	uint64_t                now);
static void                     timer_longterm_update(
	timer_longterm_t *tlp);
static void                     timer_longterm_update_locked(
	timer_longterm_t *tlp);
static mpqueue_head_t *         timer_longterm_enqueue_unlocked(
	timer_call_t            call,
	uint64_t                now,
	uint64_t                deadline,
	mpqueue_head_t **       old_queue,
	uint64_t                soft_deadline,
	uint64_t                ttd,
	timer_call_param_t      param1,
	uint32_t                callout_flags);
static void                     timer_longterm_dequeued_locked(
	timer_call_t            call);

uint64_t past_deadline_timers;
uint64_t past_deadline_deltas;
uint64_t past_deadline_longest;
uint64_t past_deadline_shortest = ~0ULL;
enum {PAST_DEADLINE_TIMER_ADJUSTMENT_NS = 10 * 1000};

uint64_t past_deadline_timer_adjustment;

static boolean_t timer_call_enter_internal(timer_call_t call, timer_call_param_t param1, uint64_t deadline, uint64_t leeway, uint32_t flags, boolean_t ratelimited);
boolean_t       mach_timer_coalescing_enabled = TRUE;

mpqueue_head_t  *timer_call_enqueue_deadline_unlocked(
	timer_call_t            call,
	mpqueue_head_t          *queue,
	uint64_t                deadline,
	uint64_t                soft_deadline,
	uint64_t                ttd,
	timer_call_param_t      param1,
	uint32_t                flags);

mpqueue_head_t  *timer_call_dequeue_unlocked(
	timer_call_t            call);

timer_coalescing_priority_params_t tcoal_prio_params;

#if TCOAL_PRIO_STATS
int32_t nc_tcl, rt_tcl, bg_tcl, kt_tcl, fp_tcl, ts_tcl, qos_tcl;
#define TCOAL_PRIO_STAT(x) (x++)
#else
#define TCOAL_PRIO_STAT(x)
#endif

static void
timer_call_init_abstime(void)
{
	int i;
	uint64_t result;
	timer_coalescing_priority_params_ns_t * tcoal_prio_params_init = timer_call_get_priority_params();
	nanoseconds_to_absolutetime(PAST_DEADLINE_TIMER_ADJUSTMENT_NS, &past_deadline_timer_adjustment);
	nanoseconds_to_absolutetime(tcoal_prio_params_init->idle_entry_timer_processing_hdeadline_threshold_ns, &result);
	tcoal_prio_params.idle_entry_timer_processing_hdeadline_threshold_abstime = (uint32_t)result;
	nanoseconds_to_absolutetime(tcoal_prio_params_init->interrupt_timer_coalescing_ilat_threshold_ns, &result);
	tcoal_prio_params.interrupt_timer_coalescing_ilat_threshold_abstime = (uint32_t)result;
	nanoseconds_to_absolutetime(tcoal_prio_params_init->timer_resort_threshold_ns, &result);
	tcoal_prio_params.timer_resort_threshold_abstime = (uint32_t)result;
	tcoal_prio_params.timer_coalesce_rt_shift = tcoal_prio_params_init->timer_coalesce_rt_shift;
	tcoal_prio_params.timer_coalesce_bg_shift = tcoal_prio_params_init->timer_coalesce_bg_shift;
	tcoal_prio_params.timer_coalesce_kt_shift = tcoal_prio_params_init->timer_coalesce_kt_shift;
	tcoal_prio_params.timer_coalesce_fp_shift = tcoal_prio_params_init->timer_coalesce_fp_shift;
	tcoal_prio_params.timer_coalesce_ts_shift = tcoal_prio_params_init->timer_coalesce_ts_shift;

	nanoseconds_to_absolutetime(tcoal_prio_params_init->timer_coalesce_rt_ns_max,
	    &tcoal_prio_params.timer_coalesce_rt_abstime_max);
	nanoseconds_to_absolutetime(tcoal_prio_params_init->timer_coalesce_bg_ns_max,
	    &tcoal_prio_params.timer_coalesce_bg_abstime_max);
	nanoseconds_to_absolutetime(tcoal_prio_params_init->timer_coalesce_kt_ns_max,
	    &tcoal_prio_params.timer_coalesce_kt_abstime_max);
	nanoseconds_to_absolutetime(tcoal_prio_params_init->timer_coalesce_fp_ns_max,
	    &tcoal_prio_params.timer_coalesce_fp_abstime_max);
	nanoseconds_to_absolutetime(tcoal_prio_params_init->timer_coalesce_ts_ns_max,
	    &tcoal_prio_params.timer_coalesce_ts_abstime_max);

	for (i = 0; i < NUM_LATENCY_QOS_TIERS; i++) {
		tcoal_prio_params.latency_qos_scale[i] = tcoal_prio_params_init->latency_qos_scale[i];
		nanoseconds_to_absolutetime(tcoal_prio_params_init->latency_qos_ns_max[i],
		    &tcoal_prio_params.latency_qos_abstime_max[i]);
		tcoal_prio_params.latency_tier_rate_limited[i] = tcoal_prio_params_init->latency_tier_rate_limited[i];
	}
}


void
timer_call_init(void)
{
	timer_longterm_init();
	timer_call_init_abstime();
}


void
timer_call_queue_init(mpqueue_head_t *queue)
{
	DBG("timer_call_queue_init(%p)\n", queue);
	mpqueue_init(queue, &timer_call_lck_grp, LCK_ATTR_NULL);
}


void
timer_call_setup(
	timer_call_t                    call,
	timer_call_func_t               func,
	timer_call_param_t              param0)
{
	DBG("timer_call_setup(%p,%p,%p)\n", call, func, param0);

	*call = (struct timer_call) {
		.tc_func = func,
		.tc_param0 = param0,
		.tc_async_dequeue = false,
	};

	simple_lock_init(&(call)->tc_lock, 0);
}

static mpqueue_head_t*
mpqueue_for_timer_call(timer_call_t entry)
{
	queue_t queue_entry_is_on = entry->tc_queue;
	                                                  
	return __container_of(queue_entry_is_on, struct mpqueue_head, head);
}


static __inline__ mpqueue_head_t *
timer_call_entry_dequeue(
	timer_call_t            entry)
{
	mpqueue_head_t  *old_mpqueue = mpqueue_for_timer_call(entry);

	                                     
	assert(old_mpqueue != NULL);

#if TIMER_ASSERT
	if (!hw_lock_held((hw_lock_t)&entry->tc_lock)) {
		panic("_call_entry_dequeue() "
		    "entry %p is not locked\n", entry);
	}

	  
                                                       
                                                    
                                      
    
	if (!hw_lock_held((hw_lock_t)&old_mpqueue->lock_data)) {
		panic("_call_entry_dequeue() "
		    "queue %p is not locked\n", old_mpqueue);
	}
#endif                   

	if (old_mpqueue != timer_longterm_queue) {
		priority_queue_remove(&old_mpqueue->mpq_pqhead,
		    &entry->tc_pqlink);
	}

	remqueue(&entry->tc_qlink);

	entry->tc_queue = NULL;

	old_mpqueue->count--;

	return old_mpqueue;
}

static __inline__ mpqueue_head_t *
timer_call_entry_enqueue_deadline(
	timer_call_t                    entry,
	mpqueue_head_t                  *new_mpqueue,
	uint64_t                        deadline)
{
	mpqueue_head_t  *old_mpqueue = mpqueue_for_timer_call(entry);

#if TIMER_ASSERT
	if (!hw_lock_held((hw_lock_t)&entry->tc_lock)) {
		panic("_call_entry_enqueue_deadline() "
		    "entry %p is not locked\n", entry);
	}

	                              
	if (!hw_lock_held((hw_lock_t)&new_mpqueue->lock_data)) {
		panic("_call_entry_enqueue_deadline() "
		    "queue %p is not locked\n", new_mpqueue);
	}

	if (old_mpqueue != NULL && old_mpqueue != new_mpqueue) {
		panic("_call_entry_enqueue_deadline() "
		    "old_mpqueue %p != new_mpqueue", old_mpqueue);
	}
#endif                   

	                                
	assert(new_mpqueue != timer_longterm_queue);
	assert(old_mpqueue != timer_longterm_queue);

	if (old_mpqueue == new_mpqueue) {
		                                                            
		uint64_t old_deadline = entry->tc_pqlink.deadline;
		entry->tc_pqlink.deadline = deadline;

		if (old_deadline < deadline) {
			priority_queue_entry_increased(&new_mpqueue->mpq_pqhead,
			    &entry->tc_pqlink);
		} else {
			priority_queue_entry_decreased(&new_mpqueue->mpq_pqhead,
			    &entry->tc_pqlink);
		}
	} else {
		if (old_mpqueue != NULL) {
			priority_queue_remove(&old_mpqueue->mpq_pqhead,
			    &entry->tc_pqlink);

			re_queue_tail(&new_mpqueue->head, &entry->tc_qlink);
		} else {
			enqueue_tail(&new_mpqueue->head, &entry->tc_qlink);
		}

		entry->tc_queue = &new_mpqueue->head;
		entry->tc_pqlink.deadline = deadline;

		priority_queue_insert(&new_mpqueue->mpq_pqhead, &entry->tc_pqlink);
	}


	                                                                 
                                                                  
    

	timer_call_t thead = priority_queue_min(&new_mpqueue->mpq_pqhead, struct timer_call, tc_pqlink);

	new_mpqueue->earliest_soft_deadline = thead->tc_flags & TIMER_CALL_RATELIMITED ? thead->tc_pqlink.deadline : thead->tc_soft_deadline;

	if (old_mpqueue) {
		old_mpqueue->count--;
	}
	new_mpqueue->count++;

	return old_mpqueue;
}

static __inline__ void
timer_call_entry_enqueue_tail(
	timer_call_t                    entry,
	mpqueue_head_t                  *queue)
{
	                                               
	assert(entry->tc_queue == NULL);

	  
                                                                  
                                       
    
	assert(queue == timer_longterm_queue);

	enqueue_tail(&queue->head, &entry->tc_qlink);

	entry->tc_queue = &queue->head;

	queue->count++;
	return;
}

  
                                                                       
                                                           
   
static __inline__ void
timer_call_entry_dequeue_async(
	timer_call_t            entry)
{
	mpqueue_head_t  *old_mpqueue = mpqueue_for_timer_call(entry);
	if (old_mpqueue) {
		old_mpqueue->count--;

		if (old_mpqueue != timer_longterm_queue) {
			priority_queue_remove(&old_mpqueue->mpq_pqhead,
			    &entry->tc_pqlink);
		}

		remqueue(&entry->tc_qlink);
		entry->tc_async_dequeue = true;
	}
	return;
}

#if TIMER_ASSERT
unsigned timer_call_enqueue_deadline_unlocked_async1;
unsigned timer_call_enqueue_deadline_unlocked_async2;
#endif
  
                                                               
   
__inline__ mpqueue_head_t *
timer_call_enqueue_deadline_unlocked(
	timer_call_t                    call,
	mpqueue_head_t                  *queue,
	uint64_t                        deadline,
	uint64_t                        soft_deadline,
	uint64_t                        ttd,
	timer_call_param_t              param1,
	uint32_t                        callout_flags)
{
	DBG("timer_call_enqueue_deadline_unlocked(%p,%p,)\n", call, queue);

	simple_lock(&call->tc_lock, LCK_GRP_NULL);

	mpqueue_head_t  *old_queue = mpqueue_for_timer_call(call);

	if (old_queue != NULL) {
		timer_queue_lock_spin(old_queue);
		if (call->tc_async_dequeue) {
			                                                        
#if TIMER_ASSERT
			TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
			    DECR_TIMER_ASYNC_DEQ | DBG_FUNC_NONE,
			    VM_KERNEL_UNSLIDE_OR_PERM(call),
			    call->tc_async_dequeue,
			    VM_KERNEL_UNSLIDE_OR_PERM(call->tc_queue),
			    0x1c, 0);
			timer_call_enqueue_deadline_unlocked_async1++;
#endif
			call->tc_async_dequeue = false;
			call->tc_queue = NULL;
		} else if (old_queue != queue) {
			timer_call_entry_dequeue(call);
#if TIMER_ASSERT
			timer_call_enqueue_deadline_unlocked_async2++;
#endif
		}
		if (old_queue == timer_longterm_queue) {
			timer_longterm_dequeued_locked(call);
		}
		if (old_queue != queue) {
			timer_queue_unlock(old_queue);
			timer_queue_lock_spin(queue);
		}
	} else {
		timer_queue_lock_spin(queue);
	}

	call->tc_soft_deadline = soft_deadline;
	call->tc_flags = callout_flags;
	call->tc_param1 = param1;
	call->tc_ttd = ttd;

	timer_call_entry_enqueue_deadline(call, queue, deadline);
	timer_queue_unlock(queue);
	simple_unlock(&call->tc_lock);

	return old_queue;
}

#if TIMER_ASSERT
unsigned timer_call_dequeue_unlocked_async1;
unsigned timer_call_dequeue_unlocked_async2;
#endif
mpqueue_head_t *
timer_call_dequeue_unlocked(
	timer_call_t            call)
{
	DBG("timer_call_dequeue_unlocked(%p)\n", call);

	simple_lock(&call->tc_lock, LCK_GRP_NULL);

	mpqueue_head_t  *old_queue = mpqueue_for_timer_call(call);

#if TIMER_ASSERT
	TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
	    DECR_TIMER_ASYNC_DEQ | DBG_FUNC_NONE,
	    VM_KERNEL_UNSLIDE_OR_PERM(call),
	    call->tc_async_dequeue,
	    VM_KERNEL_UNSLIDE_OR_PERM(call->tc_queue),
	    0, 0);
#endif
	if (old_queue != NULL) {
		timer_queue_lock_spin(old_queue);
		if (call->tc_async_dequeue) {
			                                                        
#if TIMER_ASSERT
			TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
			    DECR_TIMER_ASYNC_DEQ | DBG_FUNC_NONE,
			    VM_KERNEL_UNSLIDE_OR_PERM(call),
			    call->tc_async_dequeue,
			    VM_KERNEL_UNSLIDE_OR_PERM(call->tc_queue),
			    0x1c, 0);
			timer_call_dequeue_unlocked_async1++;
#endif
			call->tc_async_dequeue = false;
			call->tc_queue = NULL;
		} else {
			timer_call_entry_dequeue(call);
		}
		if (old_queue == timer_longterm_queue) {
			timer_longterm_dequeued_locked(call);
		}
		timer_queue_unlock(old_queue);
	}
	simple_unlock(&call->tc_lock);
	return old_queue;
}

uint64_t
timer_call_past_deadline_timer_handle(uint64_t deadline, uint64_t ctime)
{
	uint64_t delta = (ctime - deadline);

	past_deadline_timers++;
	past_deadline_deltas += delta;
	if (delta > past_deadline_longest) {
		past_deadline_longest = deadline;
	}
	if (delta < past_deadline_shortest) {
		past_deadline_shortest = delta;
	}

	return ctime + past_deadline_timer_adjustment;
}

  
                                 
                                 
  
                                                                            
                                                                      
  
                                                                         
  
                                                           
                                           
                                                                           
                                                                               
                                                                      
                                                                   
                                                            
  
                                                              
                                                                               
                  
                                                                             
                                                                               
                                                                               
                                                                             
                                                                        
                                                        
                                                                    
                                                                              
                                                                        
                                                                          
                                       
   

  
                                                                    
                   
   

static boolean_t
timer_call_enter_internal(
	timer_call_t            call,
	timer_call_param_t      param1,
	uint64_t                deadline,
	uint64_t                leeway,
	uint32_t                flags,
	boolean_t               ratelimited)
{
	mpqueue_head_t          *queue = NULL;
	mpqueue_head_t          *old_queue;
	spl_t                   s;
	uint64_t                slop;
	uint32_t                urgency;
	uint64_t                sdeadline, ttd;

	assert(call->tc_func != NULL);
	s = splclock();

	sdeadline = deadline;
	uint64_t ctime = mach_absolute_time();

	TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
	    DECR_TIMER_ENTER | DBG_FUNC_START,
	    VM_KERNEL_UNSLIDE_OR_PERM(call),
	    VM_KERNEL_ADDRHIDE(param1), deadline, flags, 0);

	urgency = (flags & TIMER_CALL_URGENCY_MASK);

	boolean_t slop_ratelimited = FALSE;
	slop = timer_call_slop(deadline, ctime, urgency, current_thread(), &slop_ratelimited);

	if ((flags & TIMER_CALL_LEEWAY) != 0 && leeway > slop) {
		slop = leeway;
	}

	if (UINT64_MAX - deadline <= slop) {
		deadline = UINT64_MAX;
	} else {
		deadline += slop;
	}

	if (__improbable(deadline < ctime)) {
		deadline = timer_call_past_deadline_timer_handle(deadline, ctime);
		sdeadline = deadline;
	}

	if (ratelimited || slop_ratelimited) {
		flags |= TIMER_CALL_RATELIMITED;
	} else {
		flags &= ~TIMER_CALL_RATELIMITED;
	}

	ttd =  sdeadline - ctime;
#if CONFIG_DTRACE
	DTRACE_TMR7(callout__create, timer_call_func_t, call->tc_func,
	    timer_call_param_t, call->tc_param0, uint32_t, flags,
	    (deadline - sdeadline),
	    (ttd >> 32), (unsigned) (ttd & 0xFFFFFFFF), call);
#endif

	                                                                    
                                                                      
                                                 
    
	if (!ratelimited && !slop_ratelimited) {
		queue = timer_longterm_enqueue_unlocked(call, ctime, deadline, &old_queue, sdeadline, ttd, param1, flags);
	}

	if (queue == NULL) {
		queue = timer_queue_assign(deadline);
		old_queue = timer_call_enqueue_deadline_unlocked(call, queue, deadline, sdeadline, ttd, param1, flags);
	}

#if TIMER_TRACE
	call->tc_entry_time = ctime;
#endif

	TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
	    DECR_TIMER_ENTER | DBG_FUNC_END,
	    VM_KERNEL_UNSLIDE_OR_PERM(call),
	    (old_queue != NULL), deadline, queue->count, 0);

	splx(s);

	return old_queue != NULL;
}

  
                 
                                                                    
   
boolean_t
timer_call_enter(
	timer_call_t            call,
	uint64_t                deadline,
	uint32_t                flags)
{
	return timer_call_enter_internal(call, NULL, deadline, 0, flags, FALSE);
}

boolean_t
timer_call_enter1(
	timer_call_t            call,
	timer_call_param_t      param1,
	uint64_t                deadline,
	uint32_t                flags)
{
	return timer_call_enter_internal(call, param1, deadline, 0, flags, FALSE);
}

boolean_t
timer_call_enter_with_leeway(
	timer_call_t            call,
	timer_call_param_t      param1,
	uint64_t                deadline,
	uint64_t                leeway,
	uint32_t                flags,
	boolean_t               ratelimited)
{
	return timer_call_enter_internal(call, param1, deadline, leeway, flags, ratelimited);
}

boolean_t
timer_call_cancel(
	timer_call_t            call)
{
	mpqueue_head_t          *old_queue;
	spl_t                   s;

	s = splclock();

	TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
	    DECR_TIMER_CANCEL | DBG_FUNC_START,
	    VM_KERNEL_UNSLIDE_OR_PERM(call),
	    call->tc_pqlink.deadline, call->tc_soft_deadline, call->tc_flags, 0);

	old_queue = timer_call_dequeue_unlocked(call);

	if (old_queue != NULL) {
		timer_queue_lock_spin(old_queue);

		timer_call_t new_head = priority_queue_min(&old_queue->mpq_pqhead, struct timer_call, tc_pqlink);

		if (new_head) {
			timer_queue_cancel(old_queue, call->tc_pqlink.deadline, new_head->tc_pqlink.deadline);
			old_queue->earliest_soft_deadline = new_head->tc_flags & TIMER_CALL_RATELIMITED ? new_head->tc_pqlink.deadline : new_head->tc_soft_deadline;
		} else {
			timer_queue_cancel(old_queue, call->tc_pqlink.deadline, UINT64_MAX);
			old_queue->earliest_soft_deadline = UINT64_MAX;
		}

		timer_queue_unlock(old_queue);
	}
	TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
	    DECR_TIMER_CANCEL | DBG_FUNC_END,
	    VM_KERNEL_UNSLIDE_OR_PERM(call),
	    VM_KERNEL_UNSLIDE_OR_PERM(old_queue),
	    call->tc_pqlink.deadline - mach_absolute_time(),
	    call->tc_pqlink.deadline - call->tc_entry_time, 0);
	splx(s);

#if CONFIG_DTRACE
	DTRACE_TMR6(callout__cancel, timer_call_func_t, call->tc_func,
	    timer_call_param_t, call->tc_param0, uint32_t, call->tc_flags, 0,
	    (call->tc_ttd >> 32), (unsigned) (call->tc_ttd & 0xFFFFFFFF));
#endif                    

	return old_queue != NULL;
}

static uint32_t timer_queue_shutdown_lock_skips;
static uint32_t timer_queue_shutdown_discarded;

void
timer_queue_shutdown(
	mpqueue_head_t          *queue)
{
	timer_call_t            call;
	mpqueue_head_t          *new_queue;
	spl_t                   s;


	DBG("timer_queue_shutdown(%p)\n", queue);

	s = splclock();

	while (TRUE) {
		timer_queue_lock_spin(queue);

		call = qe_queue_first(&queue->head, struct timer_call, tc_qlink);

		if (call == NULL) {
			break;
		}

		if (!simple_lock_try(&call->tc_lock, LCK_GRP_NULL)) {
			  
                                                      
                                                    
                                      
      
			timer_queue_shutdown_lock_skips++;
			timer_call_entry_dequeue_async(call);
#if TIMER_ASSERT
			TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
			    DECR_TIMER_ASYNC_DEQ | DBG_FUNC_NONE,
			    VM_KERNEL_UNSLIDE_OR_PERM(call),
			    call->tc_async_dequeue,
			    VM_KERNEL_UNSLIDE_OR_PERM(call->tc_queue),
			    0x2b, 0);
#endif
			timer_queue_unlock(queue);
			continue;
		}

		boolean_t call_local = ((call->tc_flags & TIMER_CALL_LOCAL) != 0);

		                                 
		timer_call_entry_dequeue(call);
		timer_queue_unlock(queue);

		if (call_local == FALSE) {
			                                                  
			new_queue = timer_queue_assign(call->tc_pqlink.deadline);
			timer_queue_lock_spin(new_queue);
			timer_call_entry_enqueue_deadline(
				call, new_queue, call->tc_pqlink.deadline);
			timer_queue_unlock(new_queue);
		} else {
			timer_queue_shutdown_discarded++;
		}

		assert(call_local == FALSE);
		simple_unlock(&call->tc_lock);
	}

	timer_queue_unlock(queue);
	splx(s);
}


static uint32_t timer_queue_expire_lock_skips;
uint64_t
timer_queue_expire_with_options(
	mpqueue_head_t          *queue,
	uint64_t                deadline,
	boolean_t               rescan)
{
	timer_call_t    call = NULL;
	uint32_t tc_iterations = 0;
	DBG("timer_queue_expire(%p,)\n", queue);

	                                                             
                                                                  
                                                                            
   
                                                                 
                                                            
    

	uint64_t cur_deadline = deadline;
	timer_queue_lock_spin(queue);

	while (!queue_empty(&queue->head)) {
		                                                       
                                                        
     
		if (++tc_iterations > 1) {
			cur_deadline = mach_absolute_time();
		}

		if (call == NULL) {
			if (rescan == FALSE) {
				call = priority_queue_min(&queue->mpq_pqhead, struct timer_call, tc_pqlink);
			} else {
				call = qe_queue_first(&queue->head, struct timer_call, tc_qlink);
			}
		}

		if (call->tc_soft_deadline <= cur_deadline) {
			timer_call_func_t               func;
			timer_call_param_t              param0, param1;

			TCOAL_DEBUG(0xDDDD0000, queue->earliest_soft_deadline, call->tc_soft_deadline, 0, 0, 0);
			TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
			    DECR_TIMER_EXPIRE | DBG_FUNC_NONE,
			    VM_KERNEL_UNSLIDE_OR_PERM(call),
			    call->tc_soft_deadline,
			    call->tc_pqlink.deadline,
			    call->tc_entry_time, 0);

			if ((call->tc_flags & TIMER_CALL_RATELIMITED) &&
			    (call->tc_pqlink.deadline > cur_deadline)) {
				if (rescan == FALSE) {
					break;
				}
			}

			if (!simple_lock_try(&call->tc_lock, LCK_GRP_NULL)) {
				                                                
				timer_queue_expire_lock_skips++;
				timer_call_entry_dequeue_async(call);
				call = NULL;
				continue;
			}

			timer_call_entry_dequeue(call);

			func = call->tc_func;
			param0 = call->tc_param0;
			param1 = call->tc_param1;

			simple_unlock(&call->tc_lock);
			timer_queue_unlock(queue);

			TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
			    DECR_TIMER_CALLOUT | DBG_FUNC_START,
			    VM_KERNEL_UNSLIDE_OR_PERM(call), VM_KERNEL_UNSLIDE(func),
			    VM_KERNEL_ADDRHIDE(param0),
			    VM_KERNEL_ADDRHIDE(param1),
			    0);

#if CONFIG_DTRACE
			DTRACE_TMR7(callout__start, timer_call_func_t, func,
			    timer_call_param_t, param0, unsigned, call->tc_flags,
			    0, (call->tc_ttd >> 32),
			    (unsigned) (call->tc_ttd & 0xFFFFFFFF), call);
#endif
			                                                  
                                                      
      
			uint64_t *ttdp = &current_processor()->timer_call_ttd;
			*ttdp = call->tc_ttd;
			(*func)(param0, param1);
			*ttdp = 0;
#if CONFIG_DTRACE
			DTRACE_TMR4(callout__end, timer_call_func_t, func,
			    param0, param1, call);
#endif

			TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
			    DECR_TIMER_CALLOUT | DBG_FUNC_END,
			    VM_KERNEL_UNSLIDE_OR_PERM(call), VM_KERNEL_UNSLIDE(func),
			    VM_KERNEL_ADDRHIDE(param0),
			    VM_KERNEL_ADDRHIDE(param1),
			    0);
			call = NULL;
			timer_queue_lock_spin(queue);
		} else {
			if (__probable(rescan == FALSE)) {
				break;
			} else {
				int64_t skew = call->tc_pqlink.deadline - call->tc_soft_deadline;
				assert(call->tc_pqlink.deadline >= call->tc_soft_deadline);

				                                                     
                                                            
                                            
                                              
                                                         
                                                 
                                                   
                                 
       

				if (timer_resort_threshold(skew)) {
					if (__probable(simple_lock_try(&call->tc_lock, LCK_GRP_NULL))) {
						                                                
						timer_call_entry_dequeue(call);
						timer_call_entry_enqueue_deadline(call, queue, call->tc_soft_deadline);
						simple_unlock(&call->tc_lock);
						call = NULL;
					}
				}
				if (call) {
					call = qe_queue_next(&queue->head, call, struct timer_call, tc_qlink);

					if (call == NULL) {
						break;
					}
				}
			}
		}
	}

	call = priority_queue_min(&queue->mpq_pqhead, struct timer_call, tc_pqlink);

	if (call) {
		cur_deadline = call->tc_pqlink.deadline;
		queue->earliest_soft_deadline = (call->tc_flags & TIMER_CALL_RATELIMITED) ? call->tc_pqlink.deadline: call->tc_soft_deadline;
	} else {
		queue->earliest_soft_deadline = cur_deadline = UINT64_MAX;
	}

	timer_queue_unlock(queue);

	return cur_deadline;
}

uint64_t
timer_queue_expire(
	mpqueue_head_t          *queue,
	uint64_t                deadline)
{
	return timer_queue_expire_with_options(queue, deadline, FALSE);
}

extern int serverperfmode;
static uint32_t timer_queue_migrate_lock_skips;
  
                                                               
                                                               
                                      
   
int
timer_queue_migrate(mpqueue_head_t *queue_from, mpqueue_head_t *queue_to)
{
	timer_call_t    call;
	timer_call_t    head_to;
	int             timers_migrated = 0;

	DBG("timer_queue_migrate(%p,%p)\n", queue_from, queue_to);

	assert(!ml_get_interrupts_enabled());
	assert(queue_from != queue_to);

	if (serverperfmode) {
		  
                                       
                                         
                                          
                     
     
		return -4;
	}

	  
                                                                  
                                                                   
                                                           
                                                  
                                    
                                   
                                                                  
                                                               
                                               
    

	timer_queue_lock_spin(queue_to);

	head_to = priority_queue_min(&queue_to->mpq_pqhead, struct timer_call, tc_pqlink);

	if (head_to == NULL) {
		timers_migrated = -1;
		goto abort1;
	}

	timer_queue_lock_spin(queue_from);

	call = priority_queue_min(&queue_from->mpq_pqhead, struct timer_call, tc_pqlink);

	if (call == NULL) {
		timers_migrated = -2;
		goto abort2;
	}

	if (call->tc_pqlink.deadline < head_to->tc_pqlink.deadline) {
		timers_migrated = 0;
		goto abort2;
	}

	                                            
	qe_foreach_element(call, &queue_from->head, tc_qlink) {
		if (call->tc_flags & TIMER_CALL_LOCAL) {
			timers_migrated = -3;
			goto abort2;
		}
	}

	                                                     
	qe_foreach_element_safe(call, &queue_from->head, tc_qlink) {
		if (!simple_lock_try(&call->tc_lock, LCK_GRP_NULL)) {
			                                                  
#ifdef TIMER_ASSERT
			TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
			    DECR_TIMER_ASYNC_DEQ | DBG_FUNC_NONE,
			    VM_KERNEL_UNSLIDE_OR_PERM(call),
			    VM_KERNEL_UNSLIDE_OR_PERM(call->tc_queue),
			    0,
			    0x2b, 0);
#endif
			timer_queue_migrate_lock_skips++;
			timer_call_entry_dequeue_async(call);
			continue;
		}
		timer_call_entry_dequeue(call);
		timer_call_entry_enqueue_deadline(
			call, queue_to, call->tc_pqlink.deadline);
		timers_migrated++;
		simple_unlock(&call->tc_lock);
	}
	queue_from->earliest_soft_deadline = UINT64_MAX;
abort2:
	timer_queue_unlock(queue_from);
abort1:
	timer_queue_unlock(queue_to);

	return timers_migrated;
}

void
timer_queue_trace_cpu(int ncpu)
{
	timer_call_nosync_cpu(
		ncpu,
		(void (*)(void *))timer_queue_trace,
		(void*) timer_queue_cpu(ncpu));
}

void
timer_queue_trace(
	mpqueue_head_t                  *queue)
{
	timer_call_t    call;
	spl_t           s;

	if (!kdebug_enable) {
		return;
	}

	s = splclock();
	timer_queue_lock_spin(queue);

	TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
	    DECR_TIMER_QUEUE | DBG_FUNC_START,
	    queue->count, mach_absolute_time(), 0, 0, 0);

	qe_foreach_element(call, &queue->head, tc_qlink) {
		TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
		    DECR_TIMER_QUEUE | DBG_FUNC_NONE,
		    call->tc_soft_deadline,
		    call->tc_pqlink.deadline,
		    call->tc_entry_time,
		    VM_KERNEL_UNSLIDE(call->tc_func),
		    0);
	}

	TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
	    DECR_TIMER_QUEUE | DBG_FUNC_END,
	    queue->count, mach_absolute_time(), 0, 0, 0);

	timer_queue_unlock(queue);
	splx(s);
}

void
timer_longterm_dequeued_locked(timer_call_t call)
{
	timer_longterm_t        *tlp = &timer_longterm;

	tlp->dequeues++;
	if (call == tlp->threshold.call) {
		tlp->threshold.call = NULL;
	}
}

  
                                          
                                                                        
   
mpqueue_head_t *
timer_longterm_enqueue_unlocked(timer_call_t    call,
    uint64_t        now,
    uint64_t        deadline,
    mpqueue_head_t  **old_queue,
    uint64_t        soft_deadline,
    uint64_t        ttd,
    timer_call_param_t      param1,
    uint32_t        callout_flags)
{
	timer_longterm_t        *tlp = &timer_longterm;
	boolean_t               update_required = FALSE;
	uint64_t                longterm_threshold;

	longterm_threshold = now + tlp->threshold.interval;

	  
                                          
                              
                                             
                                  
    
	if ((callout_flags & TIMER_CALL_LOCAL) != 0 ||
	    (tlp->threshold.interval == TIMER_LONGTERM_NONE) ||
	    (deadline <= longterm_threshold)) {
		return NULL;
	}

	  
                                                
    
	*old_queue = timer_call_dequeue_unlocked(call);

	  
                                                      
                                   
    
	assert(!ml_get_interrupts_enabled());
	simple_lock(&call->tc_lock, LCK_GRP_NULL);
	timer_queue_lock_spin(timer_longterm_queue);
	call->tc_pqlink.deadline = deadline;
	call->tc_param1 = param1;
	call->tc_ttd = ttd;
	call->tc_soft_deadline = soft_deadline;
	call->tc_flags = callout_flags;
	timer_call_entry_enqueue_tail(call, timer_longterm_queue);

	tlp->enqueues++;

	  
                                                          
                                                                    
    
	if (deadline < tlp->threshold.deadline &&
	    deadline < tlp->threshold.preempted) {
		tlp->threshold.preempted = deadline;
		tlp->threshold.call = call;
		update_required = TRUE;
	}
	timer_queue_unlock(timer_longterm_queue);
	simple_unlock(&call->tc_lock);

	if (update_required) {
		  
                                                        
                                                  
     
		timer_call_nosync_cpu(
			master_cpu,
			(void (*)(void *))timer_longterm_update,
			(void *)tlp);
	}

	return timer_longterm_queue;
}

  
                                                
                                                                          
                              
                                                                 
                                                                        
                                                           
                                        
                                                        
                
                                                                
                                                                              
                                                                            
                                                                 
   
void
timer_longterm_scan(timer_longterm_t    *tlp,
    uint64_t            time_start)
{
	timer_call_t    call;
	uint64_t        threshold;
	uint64_t        deadline;
	uint64_t        time_limit = time_start + tlp->scan_limit;
	mpqueue_head_t  *timer_master_queue;

	assert(!ml_get_interrupts_enabled());
	assert(cpu_number() == master_cpu);

	if (tlp->threshold.interval != TIMER_LONGTERM_NONE) {
		threshold = time_start + tlp->threshold.interval;
	}

	tlp->threshold.deadline = TIMER_LONGTERM_NONE;
	tlp->threshold.call = NULL;

	if (queue_empty(&timer_longterm_queue->head)) {
		return;
	}

	timer_master_queue = timer_queue_cpu(master_cpu);
	timer_queue_lock_spin(timer_master_queue);

	qe_foreach_element_safe(call, &timer_longterm_queue->head, tc_qlink) {
		deadline = call->tc_soft_deadline;
		if (!simple_lock_try(&call->tc_lock, LCK_GRP_NULL)) {
			                                                  
#ifdef TIMER_ASSERT
			TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
			    DECR_TIMER_ASYNC_DEQ | DBG_FUNC_NONE,
			    VM_KERNEL_UNSLIDE_OR_PERM(call),
			    VM_KERNEL_UNSLIDE_OR_PERM(call->tc_queue),
			    0,
			    0x2c, 0);
#endif
			timer_call_entry_dequeue_async(call);
			continue;
		}
		if (deadline < threshold) {
			  
                                          
                                            
      
#ifdef TIMER_ASSERT
			if (deadline < time_start) {
				TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
				    DECR_TIMER_OVERDUE | DBG_FUNC_NONE,
				    VM_KERNEL_UNSLIDE_OR_PERM(call),
				    deadline,
				    time_start,
				    threshold,
				    0);
			}
#endif
			TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
			    DECR_TIMER_ESCALATE | DBG_FUNC_NONE,
			    VM_KERNEL_UNSLIDE_OR_PERM(call),
			    call->tc_pqlink.deadline,
			    call->tc_entry_time,
			    VM_KERNEL_UNSLIDE(call->tc_func),
			    0);
			tlp->escalates++;
			timer_call_entry_dequeue(call);
			timer_call_entry_enqueue_deadline(
				call, timer_master_queue, call->tc_pqlink.deadline);
			  
                                                      
                                               
      
			(void) timer_queue_assign(deadline);
		} else {
			if (deadline < tlp->threshold.deadline) {
				tlp->threshold.deadline = deadline;
				tlp->threshold.call = call;
			}
		}
		simple_unlock(&call->tc_lock);

		                                          
		if (mach_absolute_time() > time_limit) {
			tlp->threshold.deadline = TIMER_LONGTERM_SCAN_AGAIN;
			tlp->scan_pauses++;
			DBG("timer_longterm_scan() paused %llu, qlen: %llu\n",
			    time_limit, tlp->queue.count);
			break;
		}
	}

	timer_queue_unlock(timer_master_queue);
}

void
timer_longterm_callout(timer_call_param_t p0, __unused timer_call_param_t p1)
{
	timer_longterm_t        *tlp = (timer_longterm_t *) p0;

	timer_longterm_update(tlp);
}

void
timer_longterm_update_locked(timer_longterm_t *tlp)
{
	uint64_t        latency;

	TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
	    DECR_TIMER_UPDATE | DBG_FUNC_START,
	    VM_KERNEL_UNSLIDE_OR_PERM(&tlp->queue),
	    tlp->threshold.deadline,
	    tlp->threshold.preempted,
	    tlp->queue.count, 0);

	tlp->scan_time = mach_absolute_time();
	if (tlp->threshold.preempted != TIMER_LONGTERM_NONE) {
		tlp->threshold.preempts++;
		tlp->threshold.deadline = tlp->threshold.preempted;
		tlp->threshold.preempted = TIMER_LONGTERM_NONE;
		  
                                                            
                                                             
                                           
     
	} else {
		tlp->threshold.scans++;

		  
                                                     
                                                            
     
		if (tlp->scan_time > tlp->threshold.deadline_set) {
			latency = tlp->scan_time - tlp->threshold.deadline_set;
		} else {
			latency = 0;
		}
		if (latency < tlp->threshold.interval) {
			tlp->threshold.latency_min =
			    MIN(tlp->threshold.latency_min, latency);
			tlp->threshold.latency_max =
			    MAX(tlp->threshold.latency_max, latency);
			tlp->threshold.latency =
			    (tlp->threshold.latency * 99 + latency) / 100;
		}

		timer_longterm_scan(tlp, tlp->scan_time);
	}

	tlp->threshold.deadline_set = tlp->threshold.deadline;
	                                                   
	if (tlp->threshold.deadline != TIMER_LONGTERM_NONE &&
	    tlp->threshold.deadline != TIMER_LONGTERM_SCAN_AGAIN) {
		tlp->threshold.deadline_set -= tlp->threshold.margin;
		tlp->threshold.deadline_set -= tlp->threshold.latency;
	}

	                             
	uint64_t scan_clamp = mach_absolute_time() + tlp->scan_interval;
	if (tlp->threshold.deadline_set < scan_clamp) {
		tlp->threshold.deadline_set = scan_clamp;
	}

	TIMER_KDEBUG_TRACE(KDEBUG_TRACE,
	    DECR_TIMER_UPDATE | DBG_FUNC_END,
	    VM_KERNEL_UNSLIDE_OR_PERM(&tlp->queue),
	    tlp->threshold.deadline,
	    tlp->threshold.scans,
	    tlp->queue.count, 0);
}

void
timer_longterm_update(timer_longterm_t *tlp)
{
	spl_t   s = splclock();

	timer_queue_lock_spin(timer_longterm_queue);

	if (cpu_number() != master_cpu) {
		panic("timer_longterm_update_master() on non-boot cpu");
	}

	timer_longterm_update_locked(tlp);

	if (tlp->threshold.deadline != TIMER_LONGTERM_NONE) {
		timer_call_enter(
			&tlp->threshold.timer,
			tlp->threshold.deadline_set,
			TIMER_CALL_LOCAL | TIMER_CALL_SYS_CRITICAL);
	}

	timer_queue_unlock(timer_longterm_queue);
	splx(s);
}

void
timer_longterm_init(void)
{
	uint32_t                longterm;
	timer_longterm_t        *tlp = &timer_longterm;

	DBG("timer_longterm_init() tlp: %p, queue: %p\n", tlp, &tlp->queue);

	  
                                                                          
                                                 
                                
    
	tlp->threshold.interval = serverperfmode ? TIMER_LONGTERM_NONE
	    : TIMER_LONGTERM_THRESHOLD;
	if (PE_parse_boot_argn("longterm", &longterm, sizeof(longterm))) {
		tlp->threshold.interval = (longterm == 0) ?
		    TIMER_LONGTERM_NONE :
		    longterm * NSEC_PER_MSEC;
	}
	if (tlp->threshold.interval != TIMER_LONGTERM_NONE) {
		printf("Longterm timer threshold: %llu ms\n",
		    tlp->threshold.interval / NSEC_PER_MSEC);
		kprintf("Longterm timer threshold: %llu ms\n",
		    tlp->threshold.interval / NSEC_PER_MSEC);
		nanoseconds_to_absolutetime(tlp->threshold.interval,
		    &tlp->threshold.interval);
		tlp->threshold.margin = tlp->threshold.interval / 10;
		tlp->threshold.latency_min = EndOfAllTime;
		tlp->threshold.latency_max = 0;
	}

	tlp->threshold.preempted = TIMER_LONGTERM_NONE;
	tlp->threshold.deadline = TIMER_LONGTERM_NONE;

	mpqueue_init(&tlp->queue, &timer_longterm_lck_grp, LCK_ATTR_NULL);

	timer_call_setup(&tlp->threshold.timer,
	    timer_longterm_callout, (timer_call_param_t) tlp);

	timer_longterm_queue = &tlp->queue;
}

enum {
	THRESHOLD, QCOUNT,
	ENQUEUES, DEQUEUES, ESCALATES, SCANS, PREEMPTS,
	LATENCY, LATENCY_MIN, LATENCY_MAX, SCAN_LIMIT, SCAN_INTERVAL, PAUSES
};
uint64_t
timer_sysctl_get(int oid)
{
	timer_longterm_t        *tlp = &timer_longterm;

	switch (oid) {
	case THRESHOLD:
		return (tlp->threshold.interval == TIMER_LONGTERM_NONE) ?
		       0 : tlp->threshold.interval / NSEC_PER_MSEC;
	case QCOUNT:
		return tlp->queue.count;
	case ENQUEUES:
		return tlp->enqueues;
	case DEQUEUES:
		return tlp->dequeues;
	case ESCALATES:
		return tlp->escalates;
	case SCANS:
		return tlp->threshold.scans;
	case PREEMPTS:
		return tlp->threshold.preempts;
	case LATENCY:
		return tlp->threshold.latency;
	case LATENCY_MIN:
		return tlp->threshold.latency_min;
	case LATENCY_MAX:
		return tlp->threshold.latency_max;
	case SCAN_LIMIT:
		return tlp->scan_limit;
	case SCAN_INTERVAL:
		return tlp->scan_interval;
	case PAUSES:
		return tlp->scan_pauses;
	default:
		return 0;
	}
}

  
                                                              
                                                      
   
static void
timer_master_scan(timer_longterm_t      *tlp,
    uint64_t              now)
{
	timer_call_t    call;
	uint64_t        threshold;
	uint64_t        deadline;
	mpqueue_head_t  *timer_master_queue;

	if (tlp->threshold.interval != TIMER_LONGTERM_NONE) {
		threshold = now + tlp->threshold.interval;
	} else {
		threshold = TIMER_LONGTERM_NONE;
	}

	timer_master_queue = timer_queue_cpu(master_cpu);
	timer_queue_lock_spin(timer_master_queue);

	qe_foreach_element_safe(call, &timer_master_queue->head, tc_qlink) {
		deadline = call->tc_pqlink.deadline;
		if ((call->tc_flags & TIMER_CALL_LOCAL) != 0) {
			continue;
		}
		if (!simple_lock_try(&call->tc_lock, LCK_GRP_NULL)) {
			                                                  
			timer_call_entry_dequeue_async(call);
			continue;
		}
		if (deadline > threshold) {
			                                  
			timer_call_entry_dequeue(call);
			timer_call_entry_enqueue_tail(call, timer_longterm_queue);
			if (deadline < tlp->threshold.deadline) {
				tlp->threshold.deadline = deadline;
				tlp->threshold.call = call;
			}
		}
		simple_unlock(&call->tc_lock);
	}
	timer_queue_unlock(timer_master_queue);
}

static void
timer_sysctl_set_threshold(uint64_t value)
{
	timer_longterm_t        *tlp = &timer_longterm;
	spl_t                   s = splclock();
	boolean_t               threshold_increase;

	timer_queue_lock_spin(timer_longterm_queue);

	timer_call_cancel(&tlp->threshold.timer);

	  
                                                          
    
	if (value == 0) {
		tlp->threshold.interval = TIMER_LONGTERM_NONE;
		threshold_increase = TRUE;
		timer_call_cancel(&tlp->threshold.timer);
	} else {
		uint64_t        old_interval = tlp->threshold.interval;
		tlp->threshold.interval = value * NSEC_PER_MSEC;
		nanoseconds_to_absolutetime(tlp->threshold.interval,
		    &tlp->threshold.interval);
		tlp->threshold.margin = tlp->threshold.interval / 10;
		if (old_interval == TIMER_LONGTERM_NONE) {
			threshold_increase = FALSE;
		} else {
			threshold_increase = (tlp->threshold.interval > old_interval);
		}
	}

	if (threshold_increase                 ) {
		                                             
		timer_longterm_scan(tlp, mach_absolute_time());
	} else {                            
		  
                                                            
                                                                
                                                                  
                                  
     
		timer_master_scan(tlp, mach_absolute_time());
	}

	                               
	tlp->threshold.deadline_set = tlp->threshold.deadline;
	if (tlp->threshold.deadline != TIMER_LONGTERM_NONE) {
		tlp->threshold.deadline_set -= tlp->threshold.margin;
		tlp->threshold.deadline_set -= tlp->threshold.latency;
		timer_call_enter(
			&tlp->threshold.timer,
			tlp->threshold.deadline_set,
			TIMER_CALL_LOCAL | TIMER_CALL_SYS_CRITICAL);
	}

	                 
	tlp->enqueues = 0;
	tlp->dequeues = 0;
	tlp->escalates = 0;
	tlp->scan_pauses = 0;
	tlp->threshold.scans = 0;
	tlp->threshold.preempts = 0;
	tlp->threshold.latency = 0;
	tlp->threshold.latency_min = EndOfAllTime;
	tlp->threshold.latency_max = 0;

	timer_queue_unlock(timer_longterm_queue);
	splx(s);
}

int
timer_sysctl_set(int oid, uint64_t value)
{
	switch (oid) {
	case THRESHOLD:
		timer_call_cpu(
			master_cpu,
			(void (*)(void *))timer_sysctl_set_threshold,
			(void *) value);
		return KERN_SUCCESS;
	case SCAN_LIMIT:
		timer_longterm.scan_limit = value;
		return KERN_SUCCESS;
	case SCAN_INTERVAL:
		timer_longterm.scan_interval = value;
		return KERN_SUCCESS;
	default:
		return KERN_INVALID_ARGUMENT;
	}
}


                                                                               
static boolean_t
tcoal_qos_adjust(thread_t t, int32_t *tshift, uint64_t *tmax_abstime, boolean_t *pratelimited)
{
	uint32_t latency_qos;
	boolean_t adjusted = FALSE;
	task_t ctask = t->task;

	if (ctask) {
		latency_qos = proc_get_effective_thread_policy(t, TASK_POLICY_LATENCY_QOS);

		assert(latency_qos <= NUM_LATENCY_QOS_TIERS);

		if (latency_qos) {
			*tshift = tcoal_prio_params.latency_qos_scale[latency_qos - 1];
			*tmax_abstime = tcoal_prio_params.latency_qos_abstime_max[latency_qos - 1];
			*pratelimited = tcoal_prio_params.latency_tier_rate_limited[latency_qos - 1];
			adjusted = TRUE;
		}
	}
	return adjusted;
}


                                                                 
                                                                        
                                                                    
                                                                  
                                                                 
                                                                       
   
static void
timer_compute_leeway(thread_t cthread, int32_t urgency, int32_t *tshift, uint64_t *tmax_abstime, boolean_t *pratelimited)
{
	int16_t tpri = cthread->sched_pri;
	if ((urgency & TIMER_CALL_USER_MASK) != 0) {
		if (tpri >= BASEPRI_RTQUEUES ||
		    urgency == TIMER_CALL_USER_CRITICAL) {
			*tshift = tcoal_prio_params.timer_coalesce_rt_shift;
			*tmax_abstime = tcoal_prio_params.timer_coalesce_rt_abstime_max;
			TCOAL_PRIO_STAT(rt_tcl);
		} else if (proc_get_effective_thread_policy(cthread, TASK_POLICY_DARWIN_BG) ||
		    (urgency == TIMER_CALL_USER_BACKGROUND)) {
			                                                           
			if (tcoal_qos_adjust(cthread, tshift, tmax_abstime, pratelimited)) {
				if (*tmax_abstime > tcoal_prio_params.timer_coalesce_bg_abstime_max) {
					return;
				} else {
					*pratelimited = FALSE;
				}
			}
			*tshift = tcoal_prio_params.timer_coalesce_bg_shift;
			*tmax_abstime = tcoal_prio_params.timer_coalesce_bg_abstime_max;
			TCOAL_PRIO_STAT(bg_tcl);
		} else if (tpri >= MINPRI_KERNEL) {
			*tshift = tcoal_prio_params.timer_coalesce_kt_shift;
			*tmax_abstime = tcoal_prio_params.timer_coalesce_kt_abstime_max;
			TCOAL_PRIO_STAT(kt_tcl);
		} else if (cthread->sched_mode == TH_MODE_FIXED) {
			*tshift = tcoal_prio_params.timer_coalesce_fp_shift;
			*tmax_abstime = tcoal_prio_params.timer_coalesce_fp_abstime_max;
			TCOAL_PRIO_STAT(fp_tcl);
		} else if (tcoal_qos_adjust(cthread, tshift, tmax_abstime, pratelimited)) {
			TCOAL_PRIO_STAT(qos_tcl);
		} else if (cthread->sched_mode == TH_MODE_TIMESHARE) {
			*tshift = tcoal_prio_params.timer_coalesce_ts_shift;
			*tmax_abstime = tcoal_prio_params.timer_coalesce_ts_abstime_max;
			TCOAL_PRIO_STAT(ts_tcl);
		} else {
			TCOAL_PRIO_STAT(nc_tcl);
		}
	} else if (urgency == TIMER_CALL_SYS_BACKGROUND) {
		*tshift = tcoal_prio_params.timer_coalesce_bg_shift;
		*tmax_abstime = tcoal_prio_params.timer_coalesce_bg_abstime_max;
		TCOAL_PRIO_STAT(bg_tcl);
	} else {
		*tshift = tcoal_prio_params.timer_coalesce_kt_shift;
		*tmax_abstime = tcoal_prio_params.timer_coalesce_kt_abstime_max;
		TCOAL_PRIO_STAT(kt_tcl);
	}
}


int timer_user_idle_level;

uint64_t
timer_call_slop(uint64_t deadline, uint64_t now, uint32_t flags, thread_t cthread, boolean_t *pratelimited)
{
	int32_t tcs_shift = 0;
	uint64_t tcs_max_abstime = 0;
	uint64_t adjval;
	uint32_t urgency = (flags & TIMER_CALL_URGENCY_MASK);

	if (mach_timer_coalescing_enabled &&
	    (deadline > now) && (urgency != TIMER_CALL_SYS_CRITICAL)) {
		timer_compute_leeway(cthread, urgency, &tcs_shift, &tcs_max_abstime, pratelimited);

		if (tcs_shift >= 0) {
			adjval =  MIN((deadline - now) >> tcs_shift, tcs_max_abstime);
		} else {
			adjval =  MIN((deadline - now) << (-tcs_shift), tcs_max_abstime);
		}
		                                                                
		adjval += (adjval * timer_user_idle_level) >> 7;
		return adjval;
	} else {
		return 0;
	}
}

int
timer_get_user_idle_level(void)
{
	return timer_user_idle_level;
}

kern_return_t
timer_set_user_idle_level(int ilevel)
{
	boolean_t do_reeval = FALSE;

	if ((ilevel < 0) || (ilevel > 128)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (ilevel < timer_user_idle_level) {
		do_reeval = TRUE;
	}

	timer_user_idle_level = ilevel;

	if (do_reeval) {
		ml_timer_evaluate();
	}

	return KERN_SUCCESS;
}

#pragma mark - running timers

#define RUNNING_TIMER_FAKE_FLAGS (TIMER_CALL_SYS_CRITICAL | \
    TIMER_CALL_LOCAL)

  
                                                                          
                                                       
   

static void
timer_call_trace_enter_before(struct timer_call *call, uint64_t deadline,
    uint32_t flags, uint64_t now)
{
#pragma unused(call, deadline, flags, now)
	TIMER_KDEBUG_TRACE(KDEBUG_TRACE, DECR_TIMER_ENTER | DBG_FUNC_START,
	    VM_KERNEL_UNSLIDE_OR_PERM(call), VM_KERNEL_ADDRHIDE(call->tc_param1),
	    deadline, flags, 0);
#if CONFIG_DTRACE
	uint64_t ttd = deadline - now;
	DTRACE_TMR7(callout__create, timer_call_func_t, call->tc_func,
	    timer_call_param_t, call->tc_param0, uint32_t, flags, 0,
	    (ttd >> 32), (unsigned int)(ttd & 0xFFFFFFFF), NULL);
#endif                    
	TIMER_KDEBUG_TRACE(KDEBUG_TRACE, DECR_TIMER_ENTER | DBG_FUNC_END,
	    VM_KERNEL_UNSLIDE_OR_PERM(call), 0, deadline, 0, 0);
}

static void
timer_call_trace_enter_after(struct timer_call *call, uint64_t deadline)
{
#pragma unused(call, deadline)
	TIMER_KDEBUG_TRACE(KDEBUG_TRACE, DECR_TIMER_ENTER | DBG_FUNC_END,
	    VM_KERNEL_UNSLIDE_OR_PERM(call), 0, deadline, 0, 0);
}

static void
timer_call_trace_cancel(struct timer_call *call)
{
#pragma unused(call)
	__unused uint64_t deadline = call->tc_pqlink.deadline;
	TIMER_KDEBUG_TRACE(KDEBUG_TRACE, DECR_TIMER_CANCEL | DBG_FUNC_START,
	    VM_KERNEL_UNSLIDE_OR_PERM(call), deadline, 0,
	    call->tc_flags, 0);
	TIMER_KDEBUG_TRACE(KDEBUG_TRACE, DECR_TIMER_CANCEL | DBG_FUNC_END,
	    VM_KERNEL_UNSLIDE_OR_PERM(call), 0, deadline - mach_absolute_time(),
	    deadline - call->tc_entry_time, 0);
#if CONFIG_DTRACE
#if TIMER_TRACE
	uint64_t ttd = deadline - call->tc_entry_time;
#else
	uint64_t ttd = UINT64_MAX;
#endif                  
	DTRACE_TMR6(callout__cancel, timer_call_func_t, call->tc_func,
	    timer_call_param_t, call->tc_param0, uint32_t, call->tc_flags, 0,
	    (ttd >> 32), (unsigned int)(ttd & 0xFFFFFFFF));
#endif                    
}

static void
timer_call_trace_expire_entry(struct timer_call *call)
{
#pragma unused(call)
	TIMER_KDEBUG_TRACE(KDEBUG_TRACE, DECR_TIMER_CALLOUT | DBG_FUNC_START,
	    VM_KERNEL_UNSLIDE_OR_PERM(call), VM_KERNEL_UNSLIDE(call->tc_func),
	    VM_KERNEL_ADDRHIDE(call->tc_param0),
	    VM_KERNEL_ADDRHIDE(call->tc_param1),
	    0);
#if CONFIG_DTRACE
#if TIMER_TRACE
	uint64_t ttd = call->tc_pqlink.deadline - call->tc_entry_time;
#else                  
	uint64_t ttd = UINT64_MAX;
#endif                  
	DTRACE_TMR7(callout__start, timer_call_func_t, call->tc_func,
	    timer_call_param_t, call->tc_param0, unsigned, call->tc_flags,
	    0, (ttd >> 32), (unsigned int)(ttd & 0xFFFFFFFF), NULL);
#endif                    
}

static void
timer_call_trace_expire_return(struct timer_call *call)
{
#pragma unused(call)
#if CONFIG_DTRACE
	DTRACE_TMR4(callout__end, timer_call_func_t, call->tc_func,
	    call->tc_param0, call->tc_param1, NULL);
#endif                    
	TIMER_KDEBUG_TRACE(KDEBUG_TRACE, DECR_TIMER_CALLOUT | DBG_FUNC_END,
	    VM_KERNEL_UNSLIDE_OR_PERM(call),
	    VM_KERNEL_UNSLIDE(call->tc_func),
	    VM_KERNEL_ADDRHIDE(call->tc_param0),
	    VM_KERNEL_ADDRHIDE(call->tc_param1),
	    0);
}

  
                                                            
   
void
running_timer_setup(processor_t processor, enum running_timer timer,
    void *param, uint64_t deadline, uint64_t now)
{
	assert(timer < RUNNING_TIMER_MAX);
	assert(ml_get_interrupts_enabled() == FALSE);

	struct timer_call *call = &processor->running_timers[timer];

	timer_call_trace_enter_before(call, deadline, RUNNING_TIMER_FAKE_FLAGS,
	    now);

	if (__improbable(deadline < now)) {
		deadline = timer_call_past_deadline_timer_handle(deadline, now);
	}

	call->tc_pqlink.deadline = deadline;
#if TIMER_TRACE
	call->tc_entry_time = now;
#endif                  
	call->tc_param1 = param;

	timer_call_trace_enter_after(call, deadline);
}

void
running_timers_sync(void)
{
	timer_resync_deadlines();
}

void
running_timer_enter(processor_t processor, unsigned int timer,
    void *param, uint64_t deadline, uint64_t now)
{
	running_timer_setup(processor, timer, param, deadline, now);
	running_timers_sync();
}

  
                                                                          
                                                       
   
bool
running_timers_expire(processor_t processor, uint64_t now)
{
	bool expired = false;

	if (!processor->running_timers_active) {
		return expired;
	}

	for (int i = 0; i < RUNNING_TIMER_MAX; i++) {
		struct timer_call *call = &processor->running_timers[i];

		uint64_t deadline = call->tc_pqlink.deadline;
		if (deadline > now) {
			continue;
		}

		expired = true;
		timer_call_trace_expire_entry(call);
		call->tc_func(call->tc_param0, call->tc_param1);
		timer_call_trace_expire_return(call);
	}

	return expired;
}

void
running_timer_clear(processor_t processor, enum running_timer timer)
{
	struct timer_call *call = &processor->running_timers[timer];
	uint64_t deadline = call->tc_pqlink.deadline;
	if (deadline == EndOfAllTime) {
		return;
	}

	call->tc_pqlink.deadline = EndOfAllTime;
#if TIMER_TRACE
	call->tc_entry_time = 0;
#endif                  
	timer_call_trace_cancel(call);
}

void
running_timer_cancel(processor_t processor, unsigned int timer)
{
	running_timer_clear(processor, timer);
	running_timers_sync();
}

uint64_t
running_timers_deadline(processor_t processor)
{
	if (!processor->running_timers_active) {
		return EndOfAllTime;
	}

	uint64_t deadline = EndOfAllTime;
	for (int i = 0; i < RUNNING_TIMER_MAX; i++) {
		uint64_t candidate =
		    processor->running_timers[i].tc_pqlink.deadline;
		if (candidate != 0 && candidate < deadline) {
			deadline = candidate;
		}
	}

	return deadline;
}

void
running_timers_activate(processor_t processor)
{
	processor->running_timers_active = true;
	running_timers_sync();
}

void
running_timers_deactivate(processor_t processor)
{
	assert(processor->running_timers_active == true);
	processor->running_timers_active = false;
	running_timers_sync();
}
