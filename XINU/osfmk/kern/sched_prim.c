/*
 * Copyright (c) 2000-2016 Apple Inc. All rights reserved.
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
  
   
  
                     
                               
             
  
                        
  
   

#include <debug.h>

#include <mach/mach_types.h>
#include <mach/machine.h>
#include <mach/policy.h>
#include <mach/sync_policy.h>
#include <mach/thread_act.h>

#include <machine/machine_routines.h>
#include <machine/sched_param.h>
#include <machine/machine_cpu.h>
#include <machine/limits.h>
#include <machine/atomic.h>

#include <machine/commpage.h>

#include <kern/kern_types.h>
#include <kern/backtrace.h>
#include <kern/clock.h>
#include <kern/cpu_number.h>
#include <kern/cpu_data.h>
#include <kern/smp.h>
#include <kern/debug.h>
#include <kern/macro_help.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#if MONOTONIC
#include <kern/monotonic.h>
#endif                
#include <kern/processor.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/sfi.h>
#include <kern/syscall_subr.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/ledger.h>
#include <kern/timer_queue.h>
#include <kern/waitq.h>
#include <kern/policy_internal.h>
#include <kern/cpu_quiesce.h>

#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_pageout.h>

#include <mach/sdt.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>

#include <sys/kdebug.h>
#include <kperf/kperf.h>
#include <kern/kpc.h>
#include <san/kasan.h>
#include <kern/pms.h>
#include <kern/host.h>
#include <stdatomic.h>

struct sched_statistics PERCPU_DATA(sched_stats);
bool sched_stats_active;

int
rt_runq_count(processor_set_t pset)
{
	return atomic_load_explicit(&SCHED(rt_runq)(pset)->count, memory_order_relaxed);
}

void
rt_runq_count_incr(processor_set_t pset)
{
	atomic_fetch_add_explicit(&SCHED(rt_runq)(pset)->count, 1, memory_order_relaxed);
}

void
rt_runq_count_decr(processor_set_t pset)
{
	atomic_fetch_sub_explicit(&SCHED(rt_runq)(pset)->count, 1, memory_order_relaxed);
}

#define         DEFAULT_PREEMPTION_RATE         100                        
TUNABLE(int, default_preemption_rate, "preempt", DEFAULT_PREEMPTION_RATE);

#define         DEFAULT_BG_PREEMPTION_RATE      400                        
TUNABLE(int, default_bg_preemption_rate, "bg_preempt", DEFAULT_BG_PREEMPTION_RATE);

#define         MAX_UNSAFE_QUANTA               800
TUNABLE(int, max_unsafe_quanta, "unsafe", MAX_UNSAFE_QUANTA);

#define         MAX_POLL_QUANTA                 2
TUNABLE(int, max_poll_quanta, "poll", MAX_POLL_QUANTA);

#define         SCHED_POLL_YIELD_SHIFT          4                         
int             sched_poll_yield_shift = SCHED_POLL_YIELD_SHIFT;

uint64_t        max_poll_computation;

uint64_t        max_unsafe_computation;
uint64_t        sched_safe_duration;

#if defined(CONFIG_SCHED_TIMESHARE_CORE)

uint32_t        std_quantum;
uint32_t        min_std_quantum;
uint32_t        bg_quantum;

uint32_t        std_quantum_us;
uint32_t        bg_quantum_us;

#endif                                  

uint32_t        thread_depress_time;
uint32_t        default_timeshare_computation;
uint32_t        default_timeshare_constraint;

uint32_t        max_rt_quantum;
uint32_t        min_rt_quantum;

uint32_t        rt_constraint_threshold;

#if defined(CONFIG_SCHED_TIMESHARE_CORE)

unsigned                sched_tick;
uint32_t                sched_tick_interval;

                                                
uint32_t                sched_load_compute_interval_us = 15000;
uint64_t                sched_load_compute_interval_abs;
static _Atomic uint64_t sched_load_compute_deadline;

uint32_t        sched_pri_shifts[TH_BUCKET_MAX];
uint32_t        sched_fixed_shift;

uint32_t        sched_decay_usage_age_factor = 1;                                   

                                                                  
#define DEFAULT_DECAY_BAND_LIMIT ((BASEPRI_FOREGROUND - BASEPRI_DEFAULT) + 2)
int             sched_pri_decay_band_limit = DEFAULT_DECAY_BAND_LIMIT;

                                           
#define TIMER_DEADLINE_TRACKING_BIN_1_DEFAULT 2000000                            
                                                        
#define TIMER_DEADLINE_TRACKING_BIN_2_DEFAULT 5000000                         
                                                             

uint64_t timer_deadline_tracking_bin_1;
uint64_t timer_deadline_tracking_bin_2;

#endif                                  

thread_t sched_maintenance_thread;

                                                               
decl_simple_lock_data(static, sched_recommended_cores_lock);
static uint64_t    usercontrol_requested_recommended_cores = ALL_CORES_RECOMMENDED;
static void sched_update_recommended_cores(uint64_t recommended_cores);

#if __arm__ || __arm64__
static void sched_recommended_cores_maintenance(void);
uint64_t    perfcontrol_failsafe_starvation_threshold;
extern char *proc_name_address(struct proc *p);
#endif                           

uint64_t        sched_one_second_interval;
boolean_t       allow_direct_handoff = TRUE;

              

#if defined(CONFIG_SCHED_TIMESHARE_CORE)

static void load_shift_init(void);
static void preempt_pri_init(void);

#endif                                  

thread_t        processor_idle(
	thread_t                        thread,
	processor_t                     processor);

static ast_t
csw_check_locked(
	thread_t        thread,
	processor_t     processor,
	processor_set_t pset,
	ast_t           check_reason);

static void processor_setrun(
	processor_t                    processor,
	thread_t                       thread,
	integer_t                      options);

static void
sched_realtime_timebase_init(void);

static void
sched_timer_deadline_tracking_init(void);

#if     DEBUG
extern int debug_task;
#define TLOG(a, fmt, args...) if(debug_task & a) kprintf(fmt, ## args)
#else
#define TLOG(a, fmt, args...) do {} while (0)
#endif

static processor_t
thread_bind_internal(
	thread_t                thread,
	processor_t             processor);

static void
sched_vm_group_maintenance(void);

#if defined(CONFIG_SCHED_TIMESHARE_CORE)
int8_t          sched_load_shifts[NRQS];
bitmap_t        sched_preempt_pri[BITMAP_LEN(NRQS_MAX)];
#endif                                  

  
                                                            
                                                          
                                                                
                                                          
                             
  
                                                               
                                                                   
                                                             
                                                                 
                                                                    
                                                                       
                                                                    
                                                                   
                      
   
char sched_string[SCHED_STRING_MAX_LENGTH];

uint32_t sched_debug_flags = SCHED_DEBUG_FLAG_CHOOSE_PROCESSOR_TRACEPOINTS;

                                                                               
static int cpu_throttle_enabled = 1;

void
sched_init(void)
{
	boolean_t direct_handoff = FALSE;
	kprintf("Scheduler: Default of %s\n", SCHED(sched_name));

	if (!PE_parse_boot_argn("sched_pri_decay_limit", &sched_pri_decay_band_limit, sizeof(sched_pri_decay_band_limit))) {
		                                        
		if (!PE_get_default("kern.sched_pri_decay_limit",
		    &sched_pri_decay_band_limit,
		    sizeof(sched_pri_decay_band_limit))) {
			                                              
			sched_pri_decay_band_limit = DEFAULT_DECAY_BAND_LIMIT;
		}
	}

	kprintf("Setting scheduler priority decay band limit %d\n", sched_pri_decay_band_limit);

	if (PE_parse_boot_argn("sched_debug", &sched_debug_flags, sizeof(sched_debug_flags))) {
		kprintf("Scheduler: Debug flags 0x%08x\n", sched_debug_flags);
	}
	strlcpy(sched_string, SCHED(sched_name), sizeof(sched_string));

	cpu_quiescent_counter_init();

	SCHED(init)();
	SCHED(rt_init)(&pset0);
	sched_timer_deadline_tracking_init();

	SCHED(pset_init)(&pset0);
	SCHED(processor_init)(master_processor);

	if (PE_parse_boot_argn("direct_handoff", &direct_handoff, sizeof(direct_handoff))) {
		allow_direct_handoff = direct_handoff;
	}
}

void
sched_timebase_init(void)
{
	uint64_t        abstime;

	clock_interval_to_absolutetime_interval(1, NSEC_PER_SEC, &abstime);
	sched_one_second_interval = abstime;

	SCHED(timebase_init)();
	sched_realtime_timebase_init();
}

#if defined(CONFIG_SCHED_TIMESHARE_CORE)

void
sched_timeshare_init(void)
{
	  
                                     
          
    
	if (default_preemption_rate < 1) {
		default_preemption_rate = DEFAULT_PREEMPTION_RATE;
	}
	std_quantum_us = (1000 * 1000) / default_preemption_rate;

	printf("standard timeslicing quantum is %d us\n", std_quantum_us);

	if (default_bg_preemption_rate < 1) {
		default_bg_preemption_rate = DEFAULT_BG_PREEMPTION_RATE;
	}
	bg_quantum_us = (1000 * 1000) / default_bg_preemption_rate;

	printf("standard background quantum is %d us\n", bg_quantum_us);

	load_shift_init();
	preempt_pri_init();
	sched_tick = 0;
}

void
sched_timeshare_timebase_init(void)
{
	uint64_t        abstime;
	uint32_t        shift;

	                                  
	clock_interval_to_absolutetime_interval(
		std_quantum_us, NSEC_PER_USEC, &abstime);
	assert((abstime >> 32) == 0 && (uint32_t)abstime != 0);
	std_quantum = (uint32_t)abstime;

	                                         
	clock_interval_to_absolutetime_interval(250, NSEC_PER_USEC, &abstime);
	assert((abstime >> 32) == 0 && (uint32_t)abstime != 0);
	min_std_quantum = (uint32_t)abstime;

	                                  
	clock_interval_to_absolutetime_interval(
		bg_quantum_us, NSEC_PER_USEC, &abstime);
	assert((abstime >> 32) == 0 && (uint32_t)abstime != 0);
	bg_quantum = (uint32_t)abstime;

	                             
	clock_interval_to_absolutetime_interval(USEC_PER_SEC >> SCHED_TICK_SHIFT,
	    NSEC_PER_USEC, &abstime);
	assert((abstime >> 32) == 0 && (uint32_t)abstime != 0);
	sched_tick_interval = (uint32_t)abstime;

	                                                                   
	clock_interval_to_absolutetime_interval(sched_load_compute_interval_us, NSEC_PER_USEC, &sched_load_compute_interval_abs);
	os_atomic_init(&sched_load_compute_deadline, sched_load_compute_interval_abs);

	  
                                           
                                               
    
	abstime = (abstime * 5) / 3;
	for (shift = 0; abstime > BASEPRI_DEFAULT; ++shift) {
		abstime >>= 1;
	}
	sched_fixed_shift = shift;

	for (uint32_t i = 0; i < TH_BUCKET_MAX; i++) {
		sched_pri_shifts[i] = INT8_MAX;
	}

	max_unsafe_computation = ((uint64_t)max_unsafe_quanta) * std_quantum;
	sched_safe_duration = 2 * ((uint64_t)max_unsafe_quanta) * std_quantum;

	max_poll_computation = ((uint64_t)max_poll_quanta) * std_quantum;
	thread_depress_time = 1 * std_quantum;
	default_timeshare_computation = std_quantum / 2;
	default_timeshare_constraint = std_quantum;

#if __arm__ || __arm64__
	perfcontrol_failsafe_starvation_threshold = (2 * sched_tick_interval);
#endif                           
}

#endif                                  

void
pset_rt_init(processor_set_t pset)
{
	os_atomic_init(&pset->rt_runq.count, 0);
	queue_init(&pset->rt_runq.queue);
	memset(&pset->rt_runq.runq_stats, 0, sizeof pset->rt_runq.runq_stats);
}

static void
sched_realtime_timebase_init(void)
{
	uint64_t abstime;

	                                    
	clock_interval_to_absolutetime_interval(50, NSEC_PER_USEC, &abstime);
	assert((abstime >> 32) == 0 && (uint32_t)abstime != 0);
	min_rt_quantum = (uint32_t)abstime;

	                                    
	clock_interval_to_absolutetime_interval(
		50, 1000 * NSEC_PER_USEC, &abstime);
	assert((abstime >> 32) == 0 && (uint32_t)abstime != 0);
	max_rt_quantum = (uint32_t)abstime;

	                                                         
	clock_interval_to_absolutetime_interval(4, NSEC_PER_MSEC, &abstime);
	assert((abstime >> 32) == 0 && (uint32_t)abstime != 0);
	rt_constraint_threshold = (uint32_t)abstime;
}

void
sched_check_spill(processor_set_t pset, thread_t thread)
{
	(void)pset;
	(void)thread;

	return;
}

bool
sched_thread_should_yield(processor_t processor, thread_t thread)
{
	(void)thread;

	return !SCHED(processor_queue_empty)(processor) || rt_runq_count(processor->processor_set) > 0;
}

                                                      
bool
sched_steal_thread_DISABLED(processor_set_t pset)
{
	(void)pset;
	return false;
}

bool
sched_steal_thread_enabled(processor_set_t pset)
{
	return bit_count(pset->node->pset_map) > 1;
}

#if defined(CONFIG_SCHED_TIMESHARE_CORE)

  
                              
                   
   
static void
load_shift_init(void)
{
	int8_t          k, *p = sched_load_shifts;
	uint32_t        i, j;

	uint32_t        sched_decay_penalty = 1;

	if (PE_parse_boot_argn("sched_decay_penalty", &sched_decay_penalty, sizeof(sched_decay_penalty))) {
		kprintf("Overriding scheduler decay penalty %u\n", sched_decay_penalty);
	}

	if (PE_parse_boot_argn("sched_decay_usage_age_factor", &sched_decay_usage_age_factor, sizeof(sched_decay_usage_age_factor))) {
		kprintf("Overriding scheduler decay usage age factor %u\n", sched_decay_usage_age_factor);
	}

	if (sched_decay_penalty == 0) {
		  
                                                                 
                                                                   
                                                            
                                                                    
             
     
		for (i = 0; i < NRQS; i++) {
			sched_load_shifts[i] = INT8_MIN;
		}

		return;
	}

	*p++ = INT8_MIN; *p++ = 0;

	  
                                                        
                                                     
                                                
                                                      
    
	for (i = 2, j = 1 << sched_decay_penalty, k = 1; i < NRQS; ++k) {
		for (j <<= 1; (i < j) && (i < NRQS); ++i) {
			*p++ = k;
		}
	}
}

static void
preempt_pri_init(void)
{
	bitmap_t *p = sched_preempt_pri;

	for (int i = BASEPRI_FOREGROUND; i < MINPRI_KERNEL; ++i) {
		bitmap_set(p, i);
	}

	for (int i = BASEPRI_PREEMPT; i <= MAXPRI; ++i) {
		bitmap_set(p, i);
	}
}

#endif                                  

  
                                
   
void
thread_timer_expire(
	void                    *p0,
	__unused void   *p1)
{
	thread_t                thread = p0;
	spl_t                   s;

	assert_thread_magic(thread);

	s = splsched();
	thread_lock(thread);
	if (--thread->wait_timer_active == 0) {
		if (thread->wait_timer_is_set) {
			thread->wait_timer_is_set = FALSE;
			clear_wait_internal(thread, THREAD_TIMED_OUT);
		}
	}
	thread_unlock(thread);
	splx(s);
}

  
                  
  
                             
  
                                                                   
  
                         
  
                        
   
boolean_t
thread_unblock(
	thread_t                thread,
	wait_result_t   wresult)
{
	boolean_t               ready_for_runq = FALSE;
	thread_t                cthread = current_thread();
	uint32_t                new_run_count;
	int                             old_thread_state;

	  
                    
    
	thread->wait_result = wresult;

	  
                              
    
	if (thread->wait_timer_is_set) {
		if (timer_call_cancel(&thread->wait_timer)) {
			thread->wait_timer_active--;
		}
		thread->wait_timer_is_set = FALSE;
	}

	boolean_t aticontext, pidle;
	ml_get_power_state(&aticontext, &pidle);

	  
                                         
                
    
	old_thread_state = thread->state;
	thread->state = (old_thread_state | TH_RUN) &
	    ~(TH_WAIT | TH_UNINT | TH_WAIT_REPORT);

	if ((old_thread_state & TH_RUN) == 0) {
		uint64_t ctime = mach_approximate_time();
		thread->last_made_runnable_time = thread->last_basepri_change_time = ctime;
		timer_start(&thread->runnable_timer, ctime);

		ready_for_runq = TRUE;

		if (old_thread_state & TH_WAIT_REPORT) {
			(*thread->sched_call)(SCHED_CALL_UNBLOCK, thread);
		}

		                                      
		new_run_count = SCHED(run_count_incr)(thread);

#if CONFIG_SCHED_AUTO_JOIN
		if (aticontext == FALSE && work_interval_should_propagate(cthread, thread)) {
			work_interval_auto_join_propagate(cthread, thread);
		}
#endif                            
	} else {
		  
                                                               
                                                 
     
		assert((thread->state & TH_IDLE) == 0);
		  
                                                                     
                                                                    
     
		new_run_count = os_atomic_load(&sched_run_buckets[TH_BUCKET_RUN], relaxed);
	}

	  
                                             
    
	if (thread->sched_mode == TH_MODE_REALTIME) {
		uint64_t ctime;

		ctime = mach_absolute_time();
		thread->realtime.deadline = thread->realtime.constraint + ctime;
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SET_RT_DEADLINE) | DBG_FUNC_NONE,
		    (uintptr_t)thread_tid(thread), thread->realtime.deadline, thread->realtime.computation, 0);
	}

	  
                                                  
    
	thread->quantum_remaining = 0;
	thread->computation_metered = 0;
	thread->reason = AST_NONE;
	thread->block_hint = kThreadWaitNone;

	                                                                       
                                                         
                                      
                                                                    
                                                   
    

	if (__improbable(aticontext && !(thread_get_tag_internal(thread) & THREAD_TAG_CALLOUT))) {
		DTRACE_SCHED2(iwakeup, struct thread *, thread, struct proc *, thread->task->bsd_info);

		uint64_t ttd = current_processor()->timer_call_ttd;

		if (ttd) {
			if (ttd <= timer_deadline_tracking_bin_1) {
				thread->thread_timer_wakeups_bin_1++;
			} else if (ttd <= timer_deadline_tracking_bin_2) {
				thread->thread_timer_wakeups_bin_2++;
			}
		}

		ledger_credit_thread(thread, thread->t_ledger,
		    task_ledgers.interrupt_wakeups, 1);
		if (pidle) {
			ledger_credit_thread(thread, thread->t_ledger,
			    task_ledgers.platform_idle_wakeups, 1);
		}
	} else if (thread_get_tag_internal(cthread) & THREAD_TAG_CALLOUT) {
		                                                                               
		if (cthread->callout_woken_from_icontext) {
			ledger_credit_thread(thread, thread->t_ledger,
			    task_ledgers.interrupt_wakeups, 1);
			thread->thread_callout_interrupt_wakeups++;

			if (cthread->callout_woken_from_platform_idle) {
				ledger_credit_thread(thread, thread->t_ledger,
				    task_ledgers.platform_idle_wakeups, 1);
				thread->thread_callout_platform_idle_wakeups++;
			}

			cthread->callout_woke_thread = TRUE;
		}
	}

	if (thread_get_tag_internal(thread) & THREAD_TAG_CALLOUT) {
		thread->callout_woken_from_icontext = !!aticontext;
		thread->callout_woken_from_platform_idle = !!pidle;
		thread->callout_woke_thread = FALSE;
	}

#if KPERF
	if (ready_for_runq) {
		kperf_make_runnable(thread, aticontext);
	}
#endif            

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_MAKE_RUNNABLE) | DBG_FUNC_NONE,
	    (uintptr_t)thread_tid(thread), thread->sched_pri, thread->wait_result,
	    sched_run_buckets[TH_BUCKET_RUN], 0);

	DTRACE_SCHED2(wakeup, struct thread *, thread, struct proc *, thread->task->bsd_info);

	return ready_for_runq;
}

  
                                      
           
                                                        
              
                                            
                                                                          
                   
   
boolean_t
thread_allowed_for_handoff(
	thread_t         thread)
{
	thread_t self = current_thread();

	if (allow_direct_handoff &&
	    thread->sched_mode == TH_MODE_REALTIME &&
	    self->sched_mode == TH_MODE_REALTIME) {
		return TRUE;
	}

	return FALSE;
}

  
                     
           
                                
              
                                            
                                                                      
                                 
           
                                         
  
                                
   
kern_return_t
thread_go(
	thread_t        thread,
	wait_result_t   wresult,
	waitq_options_t option)
{
	thread_t self = current_thread();

	assert_thread_magic(thread);

	assert(thread->at_safe_point == FALSE);
	assert(thread->wait_event == NO_EVENT64);
	assert(thread->waitq == NULL);

	assert(!(thread->state & (TH_TERMINATE | TH_TERMINATE2)));
	assert(thread->state & TH_WAIT);


	if (thread_unblock(thread, wresult)) {
#if     SCHED_TRACE_THREAD_WAKEUPS
		backtrace(&thread->thread_wakeup_bt[0],
		    (sizeof(thread->thread_wakeup_bt) / sizeof(uintptr_t)), NULL);
#endif
		if ((option & WQ_OPTION_HANDOFF) &&
		    thread_allowed_for_handoff(thread)) {
			thread_reference(thread);
			assert(self->handoff_thread == NULL);
			self->handoff_thread = thread;
		} else {
			thread_setrun(thread, SCHED_PREEMPT | SCHED_TAILQ);
		}
	}

	return KERN_SUCCESS;
}

  
                                   
           
                                                           
                                                        
                                      
              
                                       
   
__private_extern__
wait_result_t
thread_mark_wait_locked(
	thread_t                        thread,
	wait_interrupt_t        interruptible_orig)
{
	boolean_t                       at_safe_point;
	wait_interrupt_t        interruptible = interruptible_orig;

	if (thread->state & TH_IDLE) {
		panic("Invalid attempt to wait while running the idle thread");
	}

	assert(!(thread->state & (TH_WAIT | TH_IDLE | TH_UNINT | TH_TERMINATE2 | TH_WAIT_REPORT)));

	  
                                                                 
                                                                  
                                                                 
                                                
    
	interruptible &= TH_OPT_INTMASK;
	if (interruptible > (thread->options & TH_OPT_INTMASK)) {
		interruptible = thread->options & TH_OPT_INTMASK;
	}

	at_safe_point = (interruptible == THREAD_ABORTSAFE);

	if (interruptible == THREAD_UNINT ||
	    !(thread->sched_flags & TH_SFLAG_ABORT) ||
	    (!at_safe_point &&
	    (thread->sched_flags & TH_SFLAG_ABORTSAFELY))) {
		if (!(thread->state & TH_TERMINATE)) {
			DTRACE_SCHED(sleep);
		}

		int state_bits = TH_WAIT;
		if (!interruptible) {
			state_bits |= TH_UNINT;
		}
		if (thread->sched_call) {
			wait_interrupt_t mask = THREAD_WAIT_NOREPORT_USER;
			if (is_kerneltask(thread->task)) {
				mask = THREAD_WAIT_NOREPORT_KERNEL;
			}
			if ((interruptible_orig & mask) == 0) {
				state_bits |= TH_WAIT_REPORT;
			}
		}
		thread->state |= state_bits;
		thread->at_safe_point = at_safe_point;

		                                                    
                                                     
		assert(!thread->block_hint);
		thread->block_hint = thread->pending_block_hint;
		thread->pending_block_hint = kThreadWaitNone;

		return thread->wait_result = THREAD_WAITING;
	} else {
		if (thread->sched_flags & TH_SFLAG_ABORTSAFELY) {
			thread->sched_flags &= ~TH_SFLAG_ABORTED_MASK;
		}
	}
	thread->pending_block_hint = kThreadWaitNone;

	return thread->wait_result = THREAD_INTERRUPTED;
}

  
                                  
           
                                                      
                                               
                                              
                           
  
                                                 
                                                
           
                                           
   
__private_extern__
wait_interrupt_t
thread_interrupt_level(
	wait_interrupt_t new_level)
{
	thread_t thread = current_thread();
	wait_interrupt_t result = thread->options & TH_OPT_INTMASK;

	thread->options = (thread->options & ~TH_OPT_INTMASK) | (new_level & TH_OPT_INTMASK);

	return result;
}

  
               
  
                                                   
                                          
   
wait_result_t
assert_wait(
	event_t                         event,
	wait_interrupt_t        interruptible)
{
	if (__improbable(event == NO_EVENT)) {
		panic("%s() called with NO_EVENT", __func__);
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_WAIT) | DBG_FUNC_NONE,
	    VM_KERNEL_UNSLIDE_OR_PERM(event), 0, 0, 0, 0);

	struct waitq *waitq;
	waitq = global_eventq(event);
	return waitq_assert_wait64(waitq, CAST_EVENT64_T(event), interruptible, TIMEOUT_WAIT_FOREVER);
}

  
                     
  
                                                  
   
struct waitq *
assert_wait_queue(
	event_t                         event)
{
	return global_eventq(event);
}

wait_result_t
assert_wait_timeout(
	event_t                         event,
	wait_interrupt_t        interruptible,
	uint32_t                        interval,
	uint32_t                        scale_factor)
{
	thread_t                        thread = current_thread();
	wait_result_t           wresult;
	uint64_t                        deadline;
	spl_t                           s;

	if (__improbable(event == NO_EVENT)) {
		panic("%s() called with NO_EVENT", __func__);
	}

	struct waitq *waitq;
	waitq = global_eventq(event);

	s = splsched();
	waitq_lock(waitq);

	clock_interval_to_deadline(interval, scale_factor, &deadline);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_WAIT) | DBG_FUNC_NONE,
	    VM_KERNEL_UNSLIDE_OR_PERM(event), interruptible, deadline, 0, 0);

	wresult = waitq_assert_wait64_locked(waitq, CAST_EVENT64_T(event),
	    interruptible,
	    TIMEOUT_URGENCY_SYS_NORMAL,
	    deadline, TIMEOUT_NO_LEEWAY,
	    thread);

	waitq_unlock(waitq);
	splx(s);
	return wresult;
}

wait_result_t
assert_wait_timeout_with_leeway(
	event_t                         event,
	wait_interrupt_t        interruptible,
	wait_timeout_urgency_t  urgency,
	uint32_t                        interval,
	uint32_t                        leeway,
	uint32_t                        scale_factor)
{
	thread_t                        thread = current_thread();
	wait_result_t           wresult;
	uint64_t                        deadline;
	uint64_t                        abstime;
	uint64_t                        slop;
	uint64_t                        now;
	spl_t                           s;

	if (__improbable(event == NO_EVENT)) {
		panic("%s() called with NO_EVENT", __func__);
	}

	now = mach_absolute_time();
	clock_interval_to_absolutetime_interval(interval, scale_factor, &abstime);
	deadline = now + abstime;

	clock_interval_to_absolutetime_interval(leeway, scale_factor, &slop);

	struct waitq *waitq;
	waitq = global_eventq(event);

	s = splsched();
	waitq_lock(waitq);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_WAIT) | DBG_FUNC_NONE,
	    VM_KERNEL_UNSLIDE_OR_PERM(event), interruptible, deadline, 0, 0);

	wresult = waitq_assert_wait64_locked(waitq, CAST_EVENT64_T(event),
	    interruptible,
	    urgency, deadline, slop,
	    thread);

	waitq_unlock(waitq);
	splx(s);
	return wresult;
}

wait_result_t
assert_wait_deadline(
	event_t                         event,
	wait_interrupt_t        interruptible,
	uint64_t                        deadline)
{
	thread_t                        thread = current_thread();
	wait_result_t           wresult;
	spl_t                           s;

	if (__improbable(event == NO_EVENT)) {
		panic("%s() called with NO_EVENT", __func__);
	}

	struct waitq *waitq;
	waitq = global_eventq(event);

	s = splsched();
	waitq_lock(waitq);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_WAIT) | DBG_FUNC_NONE,
	    VM_KERNEL_UNSLIDE_OR_PERM(event), interruptible, deadline, 0, 0);

	wresult = waitq_assert_wait64_locked(waitq, CAST_EVENT64_T(event),
	    interruptible,
	    TIMEOUT_URGENCY_SYS_NORMAL, deadline,
	    TIMEOUT_NO_LEEWAY, thread);
	waitq_unlock(waitq);
	splx(s);
	return wresult;
}

wait_result_t
assert_wait_deadline_with_leeway(
	event_t                         event,
	wait_interrupt_t        interruptible,
	wait_timeout_urgency_t  urgency,
	uint64_t                        deadline,
	uint64_t                        leeway)
{
	thread_t                        thread = current_thread();
	wait_result_t           wresult;
	spl_t                           s;

	if (__improbable(event == NO_EVENT)) {
		panic("%s() called with NO_EVENT", __func__);
	}

	struct waitq *waitq;
	waitq = global_eventq(event);

	s = splsched();
	waitq_lock(waitq);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_WAIT) | DBG_FUNC_NONE,
	    VM_KERNEL_UNSLIDE_OR_PERM(event), interruptible, deadline, 0, 0);

	wresult = waitq_assert_wait64_locked(waitq, CAST_EVENT64_T(event),
	    interruptible,
	    urgency, deadline, leeway,
	    thread);
	waitq_unlock(waitq);
	splx(s);
	return wresult;
}

  
                  
  
                                                                     
                                                                      
                                                                  
                                                   
  
                                                            
                                                                        
   
static inline boolean_t
thread_isoncpu(thread_t thread)
{
	                             
	if (!(thread->state & TH_RUN)) {
		return FALSE;
	}

	                                                  
	                                                                                                
	if (thread->runq != PROCESSOR_NULL) {
		return FALSE;
	}

	  
                                    
                                                                   
    
	if (!thread->kernel_stack) {
		return FALSE;
	}

	  
                                             
                                               
                                            
                                              
                                      
                                                
    
	return TRUE;
}

  
               
  
                                                 
                                                 
                                               
                                    
                                               
  
                                                    
                  
  
                                
   
boolean_t
thread_stop(
	thread_t                thread,
	boolean_t       until_not_runnable)
{
	wait_result_t   wresult;
	spl_t                   s = splsched();
	boolean_t               oncpu;

	wake_lock(thread);
	thread_lock(thread);

	while (thread->state & TH_SUSP) {
		thread->wake_active = TRUE;
		thread_unlock(thread);

		wresult = assert_wait(&thread->wake_active, THREAD_ABORTSAFE);
		wake_unlock(thread);
		splx(s);

		if (wresult == THREAD_WAITING) {
			wresult = thread_block(THREAD_CONTINUE_NULL);
		}

		if (wresult != THREAD_AWAKENED) {
			return FALSE;
		}

		s = splsched();
		wake_lock(thread);
		thread_lock(thread);
	}

	thread->state |= TH_SUSP;

	while ((oncpu = thread_isoncpu(thread)) ||
	    (until_not_runnable && (thread->state & TH_RUN))) {
		processor_t             processor;

		if (oncpu) {
			assert(thread->state & TH_RUN);
			processor = thread->chosen_processor;
			cause_ast_check(processor);
		}

		thread->wake_active = TRUE;
		thread_unlock(thread);

		wresult = assert_wait(&thread->wake_active, THREAD_ABORTSAFE);
		wake_unlock(thread);
		splx(s);

		if (wresult == THREAD_WAITING) {
			wresult = thread_block(THREAD_CONTINUE_NULL);
		}

		if (wresult != THREAD_AWAKENED) {
			thread_unstop(thread);
			return FALSE;
		}

		s = splsched();
		wake_lock(thread);
		thread_lock(thread);
	}

	thread_unlock(thread);
	wake_unlock(thread);
	splx(s);

	  
                                                          
                                                        
                                                        
                                                     
    

	return TRUE;
}

  
                 
  
                                          
                                     
  
                                              
   
void
thread_unstop(
	thread_t        thread)
{
	spl_t           s = splsched();

	wake_lock(thread);
	thread_lock(thread);

	assert((thread->state & (TH_RUN | TH_WAIT | TH_SUSP)) != TH_SUSP);

	if (thread->state & TH_SUSP) {
		thread->state &= ~TH_SUSP;

		if (thread->wake_active) {
			thread->wake_active = FALSE;
			thread_unlock(thread);

			thread_wakeup(&thread->wake_active);
			wake_unlock(thread);
			splx(s);

			return;
		}
	}

	thread_unlock(thread);
	wake_unlock(thread);
	splx(s);
}

  
               
  
                                                         
  
   
void
thread_wait(
	thread_t        thread,
	boolean_t       until_not_runnable)
{
	wait_result_t   wresult;
	boolean_t       oncpu;
	processor_t     processor;
	spl_t           s = splsched();

	wake_lock(thread);
	thread_lock(thread);

	  
                                                             
                                                               
                                                                
                                                             
           
    
	while ((oncpu = thread_isoncpu(thread)) ||
	    (until_not_runnable && (thread->state & TH_RUN))) {
		if (oncpu) {
			assert(thread->state & TH_RUN);
			processor = thread->chosen_processor;
			cause_ast_check(processor);
		}

		thread->wake_active = TRUE;
		thread_unlock(thread);

		wresult = assert_wait(&thread->wake_active, THREAD_UNINT);
		wake_unlock(thread);
		splx(s);

		if (wresult == THREAD_WAITING) {
			thread_block(THREAD_CONTINUE_NULL);
		}

		s = splsched();
		wake_lock(thread);
		thread_lock(thread);
	}

	thread_unlock(thread);
	wake_unlock(thread);
	splx(s);
}

  
                               
  
                                                      
                                                      
             
                            
                                               
              
               
                         
           
                                               
                                                             
                                           
   
__private_extern__ kern_return_t
clear_wait_internal(
	thread_t                thread,
	wait_result_t   wresult)
{
	uint32_t        i = LockTimeOutUsec;
	struct waitq *waitq = thread->waitq;

	do {
		if (wresult == THREAD_INTERRUPTED && (thread->state & TH_UNINT)) {
			return KERN_FAILURE;
		}

		if (waitq != NULL) {
			if (!waitq_pull_thread_locked(waitq, thread)) {
				thread_unlock(thread);
				delay(1);
				if (i > 0 && !machine_timeout_suspended()) {
					i--;
				}
				thread_lock(thread);
				if (waitq != thread->waitq) {
					return KERN_NOT_WAITING;
				}
				continue;
			}
		}

		                                                           
		if ((thread->state & (TH_WAIT | TH_TERMINATE)) == TH_WAIT) {
			return thread_go(thread, wresult, WQ_OPTION_NONE);
		} else {
			return KERN_NOT_WAITING;
		}
	} while (i > 0);

	panic("clear_wait_internal: deadlock: thread=%p, wq=%p, cpu=%d\n",
	    thread, waitq, cpu_number());

	return KERN_FAILURE;
}


  
              
  
                                                                       
                                    
  
              
                             
                                                
   
kern_return_t
clear_wait(
	thread_t                thread,
	wait_result_t   result)
{
	kern_return_t ret;
	spl_t           s;

	s = splsched();
	thread_lock(thread);
	ret = clear_wait_internal(thread, result);
	thread_unlock(thread);
	splx(s);
	return ret;
}


  
                      
  
                                                               
                         
  
   
kern_return_t
thread_wakeup_prim(
	event_t          event,
	boolean_t        one_thread,
	wait_result_t    result)
{
	if (__improbable(event == NO_EVENT)) {
		panic("%s() called with NO_EVENT", __func__);
	}

	struct waitq *wq = global_eventq(event);

	if (one_thread) {
		return waitq_wakeup64_one(wq, CAST_EVENT64_T(event), result, WAITQ_ALL_PRIORITIES);
	} else {
		return waitq_wakeup64_all(wq, CAST_EVENT64_T(event), result, WAITQ_ALL_PRIORITIES);
	}
}

  
                                                                       
   
kern_return_t
thread_wakeup_thread(
	event_t         event,
	thread_t        thread)
{
	if (__improbable(event == NO_EVENT)) {
		panic("%s() called with NO_EVENT", __func__);
	}

	if (__improbable(thread == THREAD_NULL)) {
		panic("%s() called with THREAD_NULL", __func__);
	}

	struct waitq *wq = global_eventq(event);

	return waitq_wakeup64_thread(wq, CAST_EVENT64_T(event), thread, THREAD_AWAKENED);
}

  
                                                                    
  
                                                        
   
kern_return_t
thread_wakeup_one_with_pri(
	event_t      event,
	int          priority)
{
	if (__improbable(event == NO_EVENT)) {
		panic("%s() called with NO_EVENT", __func__);
	}

	struct waitq *wq = global_eventq(event);

	return waitq_wakeup64_one(wq, CAST_EVENT64_T(event), THREAD_AWAKENED, priority);
}

  
                                       
                            
                                              
  
                                                        
   
thread_t
thread_wakeup_identify(event_t  event,
    int      priority)
{
	if (__improbable(event == NO_EVENT)) {
		panic("%s() called with NO_EVENT", __func__);
	}

	struct waitq *wq = global_eventq(event);

	return waitq_wakeup64_identify(wq, CAST_EVENT64_T(event), THREAD_AWAKENED, priority);
}

  
               
  
                                                                  
                                              
  
                                                      
             
  
                                          
   
processor_t
thread_bind(
	processor_t             processor)
{
	thread_t                self = current_thread();
	processor_t             prev;
	spl_t                   s;

	s = splsched();
	thread_lock(self);

	prev = thread_bind_internal(self, processor);

	thread_unlock(self);
	splx(s);

	return prev;
}

  
                        
  
                                                                         
                                                                         
                                                                         
                                                               
  
                                                                            
                                                                    
                                                 
  
                                          
   

static processor_t
thread_bind_internal(
	thread_t                thread,
	processor_t             processor)
{
	processor_t             prev;

	                               
	assert(thread->sched_pri < BASEPRI_RTQUEUES);
	                                                                                   
	assert(thread->runq == PROCESSOR_NULL);

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_THREAD_BIND), thread_tid(thread), processor ? (uintptr_t)processor->cpu_id : (uintptr_t)-1, 0, 0, 0);

	prev = thread->bound_processor;
	thread->bound_processor = processor;

	return prev;
}

  
                            
  
                                                                  
                                                                         
                                                                     
                                                                     
                                                                           
                                                                    
                                                                       
                                                       
  
                                                                     
                                                                    
                                                                      
                                                                 
                                                              
                                                                       
                           
   

  
                                      
                                         
                                                                                     
                           
                           
   
#define MAX_VM_BIND_GROUP_COUNT (5)
decl_simple_lock_data(static, sched_vm_group_list_lock);
static thread_t sched_vm_group_thread_list[MAX_VM_BIND_GROUP_COUNT];
static int sched_vm_group_thread_count;
static boolean_t sched_vm_group_temporarily_unbound = FALSE;

void
thread_vm_bind_group_add(void)
{
	thread_t self = current_thread();

	thread_reference_internal(self);
	self->options |= TH_OPT_SCHED_VM_GROUP;

	simple_lock(&sched_vm_group_list_lock, LCK_GRP_NULL);
	assert(sched_vm_group_thread_count < MAX_VM_BIND_GROUP_COUNT);
	sched_vm_group_thread_list[sched_vm_group_thread_count++] = self;
	simple_unlock(&sched_vm_group_list_lock);

	thread_bind(master_processor);

	                                                    
	thread_block(THREAD_CONTINUE_NULL);
}

static void
sched_vm_group_maintenance(void)
{
	uint64_t ctime = mach_absolute_time();
	uint64_t longtime = ctime - sched_tick_interval;
	int i;
	spl_t s;
	boolean_t high_latency_observed = FALSE;
	boolean_t runnable_and_not_on_runq_observed = FALSE;
	boolean_t bind_target_changed = FALSE;
	processor_t bind_target = PROCESSOR_NULL;

	                                                                                
	simple_lock(&sched_vm_group_list_lock, LCK_GRP_NULL);

	s = splsched();

	for (i = 0; i < sched_vm_group_thread_count; i++) {
		thread_t thread = sched_vm_group_thread_list[i];
		assert(thread != THREAD_NULL);
		thread_lock(thread);
		if ((thread->state & (TH_RUN | TH_WAIT)) == TH_RUN) {
			if (thread->runq != PROCESSOR_NULL && thread->last_made_runnable_time < longtime) {
				high_latency_observed = TRUE;
			} else if (thread->runq == PROCESSOR_NULL) {
				                                                                                      
				runnable_and_not_on_runq_observed = TRUE;
			}
		}
		thread_unlock(thread);

		if (high_latency_observed && runnable_and_not_on_runq_observed) {
			                                                              
			break;
		}
	}

	splx(s);

	if (sched_vm_group_temporarily_unbound) {
		                                                                           
		if (!high_latency_observed) {
			            
			bind_target_changed = TRUE;
			bind_target = master_processor;
			sched_vm_group_temporarily_unbound = FALSE;                                                           
		}
	} else {
		  
                                                            
                                                            
                                                              
                                                        
              
     
		if (high_latency_observed && !runnable_and_not_on_runq_observed) {
			            
			bind_target_changed = TRUE;
			bind_target = PROCESSOR_NULL;
			sched_vm_group_temporarily_unbound = TRUE;
		}
	}

	if (bind_target_changed) {
		s = splsched();
		for (i = 0; i < sched_vm_group_thread_count; i++) {
			thread_t thread = sched_vm_group_thread_list[i];
			boolean_t removed;
			assert(thread != THREAD_NULL);

			thread_lock(thread);
			removed = thread_run_queue_remove(thread);
			if (removed || ((thread->state & (TH_RUN | TH_WAIT)) == TH_WAIT)) {
				thread_bind_internal(thread, bind_target);
			} else {
				  
                                                             
                                                                     
                                                          
       
				if (bind_target == PROCESSOR_NULL) {
					thread_bind_internal(thread, bind_target);
				} else {
					sched_vm_group_temporarily_unbound = TRUE;                               
				}
			}

			if (removed) {
				thread_run_queue_reinsert(thread, SCHED_PREEMPT | SCHED_TAILQ);
			}
			thread_unlock(thread);
		}
		splx(s);
	}

	simple_unlock(&sched_vm_group_list_lock);
}

                                                                                 
                                                                            
                                                                                
                                                                                 
                                                                          
                                           
   

#if (DEVELOPMENT || DEBUG)
int sched_smt_balance = 1;
#endif

                                                          
void
sched_SMT_balance(processor_t cprocessor, processor_set_t cpset)
{
	processor_t ast_processor = NULL;

#if (DEVELOPMENT || DEBUG)
	if (__improbable(sched_smt_balance == 0)) {
		goto smt_balance_exit;
	}
#endif

	assert(cprocessor == current_processor());
	if (cprocessor->is_SMT == FALSE) {
		goto smt_balance_exit;
	}

	processor_t sib_processor = cprocessor->processor_secondary ? cprocessor->processor_secondary : cprocessor->processor_primary;

	                                                             
                                              
    
	if (sib_processor->state != PROCESSOR_IDLE) {
		goto smt_balance_exit;
	}

	processor_t sprocessor;

	sched_ipi_type_t ipi_type = SCHED_IPI_NONE;
	uint64_t running_secondary_map = (cpset->cpu_state_map[PROCESSOR_RUNNING] &
	    ~cpset->primary_map);
	for (int cpuid = lsb_first(running_secondary_map); cpuid >= 0; cpuid = lsb_next(running_secondary_map, cpuid)) {
		sprocessor = processor_array[cpuid];
		if ((sprocessor->processor_primary->state == PROCESSOR_RUNNING) &&
		    (sprocessor->current_pri < BASEPRI_RTQUEUES)) {
			ipi_type = sched_ipi_action(sprocessor, NULL, false, SCHED_IPI_EVENT_SMT_REBAL);
			if (ipi_type != SCHED_IPI_NONE) {
				assert(sprocessor != cprocessor);
				ast_processor = sprocessor;
				break;
			}
		}
	}

smt_balance_exit:
	pset_unlock(cpset);

	if (ast_processor) {
		KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_SMT_BALANCE), ast_processor->cpu_id, ast_processor->state, ast_processor->processor_primary->state, 0, 0);
		sched_ipi_perform(ast_processor, ipi_type);
	}
}

static cpumap_t
pset_available_cpumap(processor_set_t pset)
{
	return (pset->cpu_state_map[PROCESSOR_IDLE] | pset->cpu_state_map[PROCESSOR_DISPATCHING] | pset->cpu_state_map[PROCESSOR_RUNNING]) &
	       pset->recommended_bitmask;
}

static cpumap_t
pset_available_but_not_running_cpumap(processor_set_t pset)
{
	return (pset->cpu_state_map[PROCESSOR_IDLE] | pset->cpu_state_map[PROCESSOR_DISPATCHING]) &
	       pset->recommended_bitmask;
}

bool
pset_has_stealable_threads(processor_set_t pset)
{
	pset_assert_locked(pset);

	cpumap_t avail_map = pset_available_but_not_running_cpumap(pset);
	  
                                                                                           
                          
    
	avail_map &= pset->primary_map;

	return (pset->pset_runq.count > 0) && ((pset->pset_runq.count + rt_runq_count(pset)) > bit_count(avail_map));
}

  
                                                                                 
                                                                             
                                             
   
static void
pset_commit_processor_to_new_thread(processor_set_t pset, processor_t processor, thread_t new_thread)
{
	pset_assert_locked(pset);

	if (processor->state == PROCESSOR_DISPATCHING || processor->state == PROCESSOR_IDLE) {
		assert(current_thread() == processor->idle_thread);

		  
                                                                  
                                              
     
		pset_update_processor_state(pset, processor, PROCESSOR_RUNNING);
	} else {
		assert((processor->state == PROCESSOR_RUNNING) || (processor->state == PROCESSOR_SHUTDOWN));
	}

	processor_state_update_from_thread(processor, new_thread);

	if (new_thread->sched_pri >= BASEPRI_RTQUEUES) {
		bit_set(pset->realtime_map, processor->cpu_id);
	} else {
		bit_clear(pset->realtime_map, processor->cpu_id);
	}

	pset_node_t node = pset->node;

	if (bit_count(node->pset_map) == 1) {
		                                                                
		return;
	}

	cpumap_t avail_map = pset_available_cpumap(pset);

	if (new_thread->sched_pri >= BASEPRI_RTQUEUES) {
		if ((avail_map & pset->realtime_map) == avail_map) {
			                                      
			atomic_bit_clear(&node->pset_non_rt_map, pset->pset_id, memory_order_relaxed);
		}
		avail_map &= pset->primary_map;
		if ((avail_map & pset->realtime_map) == avail_map) {
			                                              
			atomic_bit_clear(&node->pset_non_rt_primary_map, pset->pset_id, memory_order_relaxed);
		}
	} else {
		if ((avail_map & pset->realtime_map) != avail_map) {
			if (!bit_test(atomic_load(&node->pset_non_rt_map), pset->pset_id)) {
				atomic_bit_set(&node->pset_non_rt_map, pset->pset_id, memory_order_relaxed);
			}
		}
		avail_map &= pset->primary_map;
		if ((avail_map & pset->realtime_map) != avail_map) {
			if (!bit_test(atomic_load(&node->pset_non_rt_primary_map), pset->pset_id)) {
				atomic_bit_set(&node->pset_non_rt_primary_map, pset->pset_id, memory_order_relaxed);
			}
		}
	}
}

static processor_t choose_processor_for_realtime_thread(processor_set_t pset, processor_t skip_processor, bool consider_secondaries);
static bool all_available_primaries_are_running_realtime_threads(processor_set_t pset);
#if defined(__x86_64__)
static bool these_processors_are_running_realtime_threads(processor_set_t pset, uint64_t these_map);
#endif
static bool sched_ok_to_run_realtime_thread(processor_set_t pset, processor_t processor);
static bool processor_is_fast_track_candidate_for_realtime_thread(processor_set_t pset, processor_t processor);
int sched_allow_rt_smt = 1;
int sched_avoid_cpu0 = 1;

  
                 
  
                                                            
  
                                                       
   
static thread_t
thread_select(thread_t          thread,
    processor_t       processor,
    ast_t            *reason)
{
	processor_set_t         pset = processor->processor_set;
	thread_t                        new_thread = THREAD_NULL;

	assert(processor == current_processor());
	assert((thread->state & (TH_RUN | TH_TERMINATE2)) == TH_RUN);

	do {
		  
                         
     
		if (SCHED(can_update_priority)(thread)) {
			SCHED(update_priority)(thread);
		}

		pset_lock(pset);

		processor_state_update_from_thread(processor, thread);

restart:
		                                                           
		bit_clear(pset->pending_AST_URGENT_cpu_mask, processor->cpu_id);
		bit_clear(pset->pending_AST_PREEMPT_cpu_mask, processor->cpu_id);

#if defined(CONFIG_SCHED_DEFERRED_AST)
		bit_clear(pset->pending_deferred_AST_cpu_mask, processor->cpu_id);
#endif

		bool secondary_can_only_run_realtime_thread = false;

		assert(processor->state != PROCESSOR_OFF_LINE);

		if (!processor->is_recommended) {
			  
                                                                                  
                                                                  
      
			if (!SCHED(processor_bound_count)(processor)) {
				goto idle;
			}
		} else if (processor->processor_primary != processor) {
			  
                                                                                          
                                                                                    
                                                                                             
     
                                                                                            
                                                                                      
      
			if (!SCHED(processor_bound_count)(processor)) {
				if ((pset->recommended_bitmask & pset->primary_map & pset->cpu_state_map[PROCESSOR_IDLE]) != 0) {
					goto idle;
				}

				  
                                                                                    
                                                                                 
       

				                                 

				if (processor->processor_primary->current_pri >= BASEPRI_RTQUEUES) {
					bool secondary_can_run_realtime_thread = sched_allow_rt_smt && rt_runq_count(pset) && all_available_primaries_are_running_realtime_threads(pset);
					if (!secondary_can_run_realtime_thread) {
						goto idle;
					}
					secondary_can_only_run_realtime_thread = true;
				}
			}
		}

		  
                                                      
                                                                       
                                                        
                                                               
    
                                                                      
    
                                       
     

		                                      
		bool still_running = ((thread->state & (TH_TERMINATE | TH_IDLE | TH_WAIT | TH_RUN | TH_SUSP)) == TH_RUN);

		  
                                                                                                      
                                                                                                        
                                    
    
                                                             
     

		bool is_yielding         = (*reason & AST_YIELD) == AST_YIELD;

		bool needs_smt_rebalance = !is_yielding && thread->sched_pri < BASEPRI_RTQUEUES && processor->processor_primary != processor;

		bool affinity_mismatch   = thread->affinity_set != AFFINITY_SET_NULL && thread->affinity_set->aset_pset != pset;

		bool bound_elsewhere     = thread->bound_processor != PROCESSOR_NULL && thread->bound_processor != processor;

		bool avoid_processor     = !is_yielding && SCHED(avoid_processor_enabled) && SCHED(thread_avoid_processor)(processor, thread);

		if (still_running && !needs_smt_rebalance && !affinity_mismatch && !bound_elsewhere && !avoid_processor) {
			  
                                                                
     
                                                           
                                                                
      
			if (thread->sched_pri >= BASEPRI_RTQUEUES && processor->first_timeslice) {
				if (rt_runq_count(pset) > 0) {
					thread_t next_rt = qe_queue_first(&SCHED(rt_runq)(pset)->queue, struct thread, runq_links);

					if (next_rt->realtime.deadline < processor->deadline &&
					    (next_rt->bound_processor == PROCESSOR_NULL ||
					    next_rt->bound_processor == processor)) {
						                                                                
						goto pick_new_rt_thread;
					}
				}

				                                              
				processor->deadline = thread->realtime.deadline;

				sched_update_pset_load_average(pset, 0);

				processor_t next_rt_processor = PROCESSOR_NULL;
				sched_ipi_type_t next_rt_ipi_type = SCHED_IPI_NONE;

				if (rt_runq_count(pset) - bit_count(pset->pending_AST_URGENT_cpu_mask) > 0) {
					next_rt_processor = choose_processor_for_realtime_thread(pset, processor, true);
					if (next_rt_processor) {
						SCHED_DEBUG_CHOOSE_PROCESSOR_KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_CHOOSE_PROCESSOR) | DBG_FUNC_NONE,
						    (uintptr_t)0, (uintptr_t)-4, next_rt_processor->cpu_id, next_rt_processor->state, 0);
						if (next_rt_processor->state == PROCESSOR_IDLE) {
							pset_update_processor_state(pset, next_rt_processor, PROCESSOR_DISPATCHING);
						}
						next_rt_ipi_type = sched_ipi_action(next_rt_processor, NULL, false, SCHED_IPI_EVENT_PREEMPT);
					}
				}
				pset_unlock(pset);

				if (next_rt_processor) {
					sched_ipi_perform(next_rt_processor, next_rt_ipi_type);
				}

				return thread;
			}

			if ((rt_runq_count(pset) == 0) &&
			    SCHED(processor_queue_has_priority)(processor, thread->sched_pri, TRUE) == FALSE) {
				                                                                          
				processor->deadline = UINT64_MAX;

				sched_update_pset_load_average(pset, 0);
				pset_unlock(pset);

				return thread;
			}
		} else {
			  
                                         
                                                                                     
      
			if (needs_smt_rebalance || affinity_mismatch || bound_elsewhere || avoid_processor) {
				*reason |= AST_REBALANCE;
			}
		}

		bool secondary_forced_idle = ((processor->processor_secondary != PROCESSOR_NULL) &&
		    (thread_no_smt(thread) || (thread->sched_pri >= BASEPRI_RTQUEUES)) &&
		    (processor->processor_secondary->state == PROCESSOR_IDLE));

		                                                                             
		bool ok_to_run_realtime_thread = sched_ok_to_run_realtime_thread(pset, processor);
		if ((rt_runq_count(pset) > 0) && ok_to_run_realtime_thread) {
			thread_t next_rt = qe_queue_first(&SCHED(rt_runq)(pset)->queue, struct thread, runq_links);

			if (__probable((next_rt->bound_processor == PROCESSOR_NULL ||
			    (next_rt->bound_processor == processor)))) {
pick_new_rt_thread:
				new_thread = qe_dequeue_head(&SCHED(rt_runq)(pset)->queue, struct thread, runq_links);

				new_thread->runq = PROCESSOR_NULL;
				SCHED_STATS_RUNQ_CHANGE(&SCHED(rt_runq)(pset)->runq_stats, rt_runq_count(pset));
				rt_runq_count_decr(pset);

				processor->deadline = new_thread->realtime.deadline;

				pset_commit_processor_to_new_thread(pset, processor, new_thread);

				sched_update_pset_load_average(pset, 0);

				processor_t ast_processor = PROCESSOR_NULL;
				processor_t next_rt_processor = PROCESSOR_NULL;
				sched_ipi_type_t ipi_type = SCHED_IPI_NONE;
				sched_ipi_type_t next_rt_ipi_type = SCHED_IPI_NONE;

				if (processor->processor_secondary != NULL) {
					processor_t sprocessor = processor->processor_secondary;
					if ((sprocessor->state == PROCESSOR_RUNNING) || (sprocessor->state == PROCESSOR_DISPATCHING)) {
						ipi_type = sched_ipi_action(sprocessor, NULL, false, SCHED_IPI_EVENT_SMT_REBAL);
						ast_processor = sprocessor;
					}
				}
				if (rt_runq_count(pset) - bit_count(pset->pending_AST_URGENT_cpu_mask) > 0) {
					next_rt_processor = choose_processor_for_realtime_thread(pset, processor, true);
					if (next_rt_processor) {
						SCHED_DEBUG_CHOOSE_PROCESSOR_KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_CHOOSE_PROCESSOR) | DBG_FUNC_NONE,
						    (uintptr_t)0, (uintptr_t)-5, next_rt_processor->cpu_id, next_rt_processor->state, 0);
						if (next_rt_processor->state == PROCESSOR_IDLE) {
							pset_update_processor_state(pset, next_rt_processor, PROCESSOR_DISPATCHING);
						}
						next_rt_ipi_type = sched_ipi_action(next_rt_processor, NULL, false, SCHED_IPI_EVENT_PREEMPT);
					}
				}
				pset_unlock(pset);

				if (ast_processor) {
					sched_ipi_perform(ast_processor, ipi_type);
				}

				if (next_rt_processor) {
					sched_ipi_perform(next_rt_processor, next_rt_ipi_type);
				}

				return new_thread;
			}
		}
		if (secondary_can_only_run_realtime_thread) {
			goto idle;
		}

		processor->deadline = UINT64_MAX;

		                                                          
		if ((new_thread = SCHED(choose_thread)(processor, MINPRI, *reason)) != THREAD_NULL) {
			pset_commit_processor_to_new_thread(pset, processor, new_thread);
			sched_update_pset_load_average(pset, 0);

			processor_t ast_processor = PROCESSOR_NULL;
			sched_ipi_type_t ipi_type = SCHED_IPI_NONE;

			processor_t sprocessor = processor->processor_secondary;
			if ((sprocessor != NULL) && (sprocessor->state == PROCESSOR_RUNNING)) {
				if (thread_no_smt(new_thread)) {
					ipi_type = sched_ipi_action(sprocessor, NULL, false, SCHED_IPI_EVENT_SMT_REBAL);
					ast_processor = sprocessor;
				}
			} else if (secondary_forced_idle && !thread_no_smt(new_thread) && pset_has_stealable_threads(pset)) {
				pset_update_processor_state(pset, sprocessor, PROCESSOR_DISPATCHING);
				ipi_type = sched_ipi_action(sprocessor, NULL, true, SCHED_IPI_EVENT_PREEMPT);
				ast_processor = sprocessor;
			}
			pset_unlock(pset);

			if (ast_processor) {
				sched_ipi_perform(ast_processor, ipi_type);
			}
			return new_thread;
		}

		if (processor->must_idle) {
			processor->must_idle = false;
			goto idle;
		}

		if (SCHED(steal_thread_enabled)(pset) && (processor->processor_primary == processor)) {
			  
                                           
                                                            
      

			if ((new_thread = SCHED(steal_thread)(pset)) != THREAD_NULL) {
				  
                                                                         
                                                                                              
                                                                                                   
       
				if (processor->state == PROCESSOR_DISPATCHING || processor->state == PROCESSOR_IDLE) {
					pset_lock(pset);
					pset_commit_processor_to_new_thread(pset, processor, new_thread);
					pset_unlock(pset);
				} else {
					assert((processor->state == PROCESSOR_RUNNING) || (processor->state == PROCESSOR_SHUTDOWN));
					processor_state_update_from_thread(processor, new_thread);
				}

				return new_thread;
			}

			  
                                              
                   
      
			if (!SCHED(processor_queue_empty)(processor) || (ok_to_run_realtime_thread && (rt_runq_count(pset) > 0))) {
				continue;
			}

			pset_lock(pset);

			                                                                   
			if (bit_test(pset->pending_AST_URGENT_cpu_mask, processor->cpu_id)) {
				goto restart;
			}
		}

idle:
		  
                                                          
                 
     
		if ((processor->state == PROCESSOR_RUNNING) || (processor->state == PROCESSOR_DISPATCHING)) {
			pset_update_processor_state(pset, processor, PROCESSOR_IDLE);
			processor_state_update_idle(processor);
		}

		                                                          
		SCHED(processor_balance)(processor, pset);

		new_thread = processor->idle_thread;
	} while (new_thread == THREAD_NULL);

	return new_thread;
}

  
                
  
                                                 
  
                                                               
  
                                                       
                                                     
  
                                                        
                                                  
                                               
   
static boolean_t
thread_invoke(
	thread_t                        self,
	thread_t                        thread,
	ast_t                           reason)
{
	if (__improbable(get_preemption_level() != 0)) {
		int pl = get_preemption_level();
		panic("thread_invoke: preemption_level %d, possible cause: %s",
		    pl, (pl < 0 ? "unlocking an unlocked mutex or spinlock" :
		    "blocking while holding a spinlock, or within interrupt context"));
	}

	thread_continue_t       continuation = self->continuation;
	void                    *parameter   = self->parameter;
	processor_t             processor;

	uint64_t                ctime = mach_absolute_time();

#ifdef CONFIG_MACH_APPROXIMATE_TIME
	commpage_update_mach_approximate_time(ctime);
#endif

#if defined(CONFIG_SCHED_TIMESHARE_CORE)
	if (!((thread->state & TH_IDLE) != 0 ||
	    ((reason & AST_HANDOFF) && self->sched_mode == TH_MODE_REALTIME))) {
		sched_timeshare_consider_maintenance(ctime);
	}
#endif

#if MONOTONIC
	mt_sched_update(self);
#endif                

	assert_thread_magic(self);
	assert(self == current_thread());
	assert(self->runq == PROCESSOR_NULL);
	assert((self->state & (TH_RUN | TH_TERMINATE2)) == TH_RUN);

	thread_lock(thread);

	assert_thread_magic(thread);
	assert((thread->state & (TH_RUN | TH_WAIT | TH_UNINT | TH_TERMINATE | TH_TERMINATE2)) == TH_RUN);
	assert(thread->bound_processor == PROCESSOR_NULL || thread->bound_processor == current_processor());
	assert(thread->runq == PROCESSOR_NULL);

	                                                                
	thread->precise_user_kernel_time = use_precise_user_kernel_time(thread);

	                                             
	thread->sfi_class = sfi_thread_classify(thread);

	                                                                               
	thread->same_pri_latency = ctime - thread->last_basepri_change_time;
	  
                                                                
                          
    
	if (ctime <= thread->last_basepri_change_time) {
		thread->same_pri_latency = ctime - thread->last_made_runnable_time;
	}

	                                                  
	if ((self->sched_mode == TH_MODE_REALTIME) && !self->reserved_stack) {
		self->reserved_stack = self->kernel_stack;
	}

	                                
#if INTERRUPT_MASKED_DEBUG
	ml_spin_debug_clear(thread);
#endif

	if (continuation != NULL) {
		if (!thread->kernel_stack) {
			  
                                         
                                                  
                               
      
			if (self->kernel_stack == self->reserved_stack && !thread->reserved_stack) {
				goto need_stack;
			}

			  
                                                   
                                                           
      
			continuation = thread->continuation;
			parameter = thread->parameter;

			processor = current_processor();
			processor->active_thread = thread;
			processor_state_update_from_thread(processor, thread);

			if (thread->last_processor != processor && thread->last_processor != NULL) {
				if (thread->last_processor->processor_set != processor->processor_set) {
					thread->ps_switch++;
				}
				thread->p_switch++;
			}
			thread->last_processor = processor;
			thread->c_switch++;
			ast_context(thread);

			thread_unlock(thread);

			self->reason = reason;

			processor->last_dispatch = ctime;
			self->last_run_time = ctime;
			processor_timer_switch_thread(ctime, &thread->system_timer);
			timer_update(&thread->runnable_timer, ctime);
			processor->kernel_timer = &thread->system_timer;

			  
                                                                       
                                                            
      
			if (!thread->precise_user_kernel_time) {
				timer_update(processor->current_state, ctime);
			}

			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    MACHDBG_CODE(DBG_MACH_SCHED, MACH_STACK_HANDOFF) | DBG_FUNC_NONE,
			    self->reason, (uintptr_t)thread_tid(thread), self->sched_pri, thread->sched_pri, 0);

			if ((thread->chosen_processor != processor) && (thread->chosen_processor != PROCESSOR_NULL)) {
				SCHED_DEBUG_CHOOSE_PROCESSOR_KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_MOVED) | DBG_FUNC_NONE,
				    (uintptr_t)thread_tid(thread), (uintptr_t)thread->chosen_processor->cpu_id, 0, 0, 0);
			}

			DTRACE_SCHED2(off__cpu, struct thread *, thread, struct proc *, thread->task->bsd_info);

			SCHED_STATS_CSW(processor, self->reason, self->sched_pri, thread->sched_pri);

#if KPERF
			kperf_off_cpu(self);
#endif            

			  
                                                       
                                                       
                                                     
                                                         
      
			TLOG(1, "thread_invoke: calling stack_handoff\n");
			stack_handoff(self, thread);

			                            
			assert(thread == current_thread_volatile());

			DTRACE_SCHED(on__cpu);

#if KPERF
			kperf_on_cpu(thread, continuation, NULL);
#endif            

			thread_dispatch(self, thread);

#if KASAN
			                                                                     
                     
			kasan_unpoison_stack(thread->kernel_stack, kernel_stack_size);
#endif

			thread->continuation = thread->parameter = NULL;

			boolean_t enable_interrupts = TRUE;

			                                                   
			if ((thread->state & TH_IDLE)) {
				enable_interrupts = FALSE;
			}

			assert(continuation);
			call_continuation(continuation, parameter,
			    thread->wait_result, enable_interrupts);
			              
		} else if (thread == self) {
			                                       
			ast_context(self);

			thread_unlock(self);

#if KPERF
			kperf_on_cpu(thread, continuation, NULL);
#endif            

			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED) | DBG_FUNC_NONE,
			    self->reason, (uintptr_t)thread_tid(thread), self->sched_pri, thread->sched_pri, 0);

#if KASAN
			                                                                   
                                        
			kasan_fakestack_drop(self);
			kasan_fakestack_gc(self);
			kasan_unpoison_stack(self->kernel_stack, kernel_stack_size);
#endif

			self->continuation = self->parameter = NULL;

			boolean_t enable_interrupts = TRUE;

			                                                   
			if ((self->state & TH_IDLE)) {
				enable_interrupts = FALSE;
			}

			call_continuation(continuation, parameter,
			    self->wait_result, enable_interrupts);
			              
		}
	} else {
		  
                                            
     
		if (!thread->kernel_stack) {
need_stack:
			if (!stack_alloc_try(thread)) {
				thread_unlock(thread);
				thread_stack_enqueue(thread);
				return FALSE;
			}
		} else if (thread == self) {
			ast_context(self);
			thread_unlock(self);

			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED) | DBG_FUNC_NONE,
			    self->reason, (uintptr_t)thread_tid(thread), self->sched_pri, thread->sched_pri, 0);

			return TRUE;
		}
	}

	  
                                        
    
	processor = current_processor();
	processor->active_thread = thread;
	processor_state_update_from_thread(processor, thread);

	if (thread->last_processor != processor && thread->last_processor != NULL) {
		if (thread->last_processor->processor_set != processor->processor_set) {
			thread->ps_switch++;
		}
		thread->p_switch++;
	}
	thread->last_processor = processor;
	thread->c_switch++;
	ast_context(thread);

	thread_unlock(thread);

	self->reason = reason;

	processor->last_dispatch = ctime;
	self->last_run_time = ctime;
	processor_timer_switch_thread(ctime, &thread->system_timer);
	timer_update(&thread->runnable_timer, ctime);
	processor->kernel_timer = &thread->system_timer;

	  
                                                                     
                                                          
    
	if (!thread->precise_user_kernel_time) {
		timer_update(processor->current_state, ctime);
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED) | DBG_FUNC_NONE,
	    self->reason, (uintptr_t)thread_tid(thread), self->sched_pri, thread->sched_pri, 0);

	if ((thread->chosen_processor != processor) && (thread->chosen_processor != NULL)) {
		SCHED_DEBUG_CHOOSE_PROCESSOR_KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_MOVED) | DBG_FUNC_NONE,
		    (uintptr_t)thread_tid(thread), (uintptr_t)thread->chosen_processor->cpu_id, 0, 0, 0);
	}

	DTRACE_SCHED2(off__cpu, struct thread *, thread, struct proc *, thread->task->bsd_info);

	SCHED_STATS_CSW(processor, self->reason, self->sched_pri, thread->sched_pri);

#if KPERF
	kperf_off_cpu(self);
#endif            

	  
                                                      
                                                    
                                               
   
                                                                      
                                                                        
                                                                          
                                                                            
                                                                              
                                                                              
                                                                            
                                                                      
                       
   
                                                                  
                                     
   
                                                                      
                                             
   
                                                                      
                                                                     
                                                                       
    
	assert(continuation == self->continuation);
	thread = machine_switch_context(self, continuation, thread);
	assert(self == current_thread_volatile());
	TLOG(1, "thread_invoke: returning machine_switch_context: self %p continuation %p thread %p\n", self, continuation, thread);

	assert(continuation == NULL && self->continuation == NULL);

	DTRACE_SCHED(on__cpu);

#if KPERF
	kperf_on_cpu(self, NULL, __builtin_frame_address(0));
#endif            

	                                              
	thread_dispatch(thread, self);

	return TRUE;
}

#if defined(CONFIG_SCHED_DEFERRED_AST)
  
                                 
  
                                                                  
                                                                      
          
  
                                                                   
                                    
   
static void
pset_cancel_deferred_dispatch(
	processor_set_t         pset,
	processor_t             processor)
{
	processor_t             active_processor = NULL;
	uint32_t                sampled_sched_run_count;

	pset_lock(pset);
	sampled_sched_run_count = os_atomic_load(&sched_run_buckets[TH_BUCKET_RUN], relaxed);

	  
                                                                            
                                                                        
                                                                     
                                                                   
   
                                                                           
                                                                        
                                                                            
                                                                      
                                                                        
                                                                           
                                                                          
                                                                            
                                                      
    

	if ((sampled_sched_run_count == 1) && (pset->pending_deferred_AST_cpu_mask)) {
		uint64_t dispatching_map = (pset->cpu_state_map[PROCESSOR_DISPATCHING] &
		    pset->pending_deferred_AST_cpu_mask &
		    ~pset->pending_AST_URGENT_cpu_mask);
		for (int cpuid = lsb_first(dispatching_map); cpuid >= 0; cpuid = lsb_next(dispatching_map, cpuid)) {
			active_processor = processor_array[cpuid];
			  
                                                           
                          
     
                                 
                                                         
                                                           
                                                          
                                                            
                                                            
                                                           
                                                           
                        
     
                                                         
                                                        
                                                     
                                                     
                                                       
                           
      
			if (active_processor != processor) {
				  
                                                     
                                              
       

				processor_state_update_idle(active_processor);
				active_processor->deadline = UINT64_MAX;
				pset_update_processor_state(pset, active_processor, PROCESSOR_IDLE);
				bit_clear(pset->pending_deferred_AST_cpu_mask, active_processor->cpu_id);
				machine_signal_idle_cancel(active_processor);
			}
		}
	}

	pset_unlock(pset);
}
#else
                                                                            
#endif

static void
thread_csw_callout(
	thread_t            old,
	thread_t            new,
	uint64_t            timestamp)
{
	perfcontrol_event event = (new->state & TH_IDLE) ? IDLE : CONTEXT_SWITCH;
	uint64_t same_pri_latency = (new->state & TH_IDLE) ? 0 : new->same_pri_latency;
	machine_switch_perfcontrol_context(event, timestamp, 0,
	    same_pri_latency, old, new);
}


  
                   
  
                                                              
                                                           
                                                              
                             
  
                                                                   
                                                                         
  
                      
  
   
void
thread_dispatch(
	thread_t                thread,
	thread_t                self)
{
	processor_t             processor = self->last_processor;
	bool was_idle = false;

	assert(processor == current_processor());
	assert(self == current_thread_volatile());
	assert(thread != self);

	if (thread != THREAD_NULL) {
		  
                                                   
                                   
                                                                   
                                                                     
                                                         
                                                            
                                                
                                                               
              
                                                     
     
		thread_csw_callout(thread, self, processor->last_dispatch);

#if KASAN
		if (thread->continuation != NULL) {
			  
                                                                   
                                                                  
      
			kasan_fakestack_drop(thread);
			if (thread->kernel_stack) {
				kasan_unpoison_stack(thread->kernel_stack, kernel_stack_size);
			}
		}

		  
                                       
     
		kasan_fakestack_gc(thread);
#endif

		  
                                          
               
     
		if (thread->continuation != NULL && thread->kernel_stack != 0) {
			stack_free(thread);
		}

		if (thread->state & TH_IDLE) {
			was_idle = true;
			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    MACHDBG_CODE(DBG_MACH_SCHED, MACH_DISPATCH) | DBG_FUNC_NONE,
			    (uintptr_t)thread_tid(thread), 0, thread->state,
			    sched_run_buckets[TH_BUCKET_RUN], 0);
		} else {
			int64_t consumed;
			int64_t remainder = 0;

			if (processor->quantum_end > processor->last_dispatch) {
				remainder = processor->quantum_end -
				    processor->last_dispatch;
			}

			consumed = thread->quantum_remaining - remainder;

			if ((thread->reason & AST_LEDGER) == 0) {
				  
                                         
                             
       
				ledger_credit_thread(thread, thread->t_ledger,
				    task_ledgers.cpu_time, consumed);
				ledger_credit_thread(thread, thread->t_threadledger,
				    thread_ledgers.cpu_time, consumed);
				if (thread->t_bankledger) {
					ledger_credit_thread(thread, thread->t_bankledger,
					    bank_ledgers.cpu_time,
					    (consumed - thread->t_deduct_bank_ledger_time));
				}
				thread->t_deduct_bank_ledger_time = 0;
				if (consumed > 0) {
					  
                                                                                 
                                   
                                                                                                                       
        
					sched_update_pset_avg_execution_time(current_processor()->processor_set, consumed, processor->last_dispatch, thread->th_sched_bucket);
				}
			}

			wake_lock(thread);
			thread_lock(thread);

			  
                                                                  
                                                                  
                               
      
			if (__improbable(thread->rwlock_count != 0)) {
				lck_rw_set_promotion_locked(thread);
			}

			boolean_t keep_quantum = processor->first_timeslice;

			  
                                                                    
                                    
      
			if (processor->starting_pri > thread->sched_pri) {
				keep_quantum = FALSE;
			}

			                                           
			if (keep_quantum &&
			    processor->quantum_end > processor->last_dispatch) {
				thread->quantum_remaining = (uint32_t)remainder;
			} else {
				thread->quantum_remaining = 0;
			}

			if (thread->sched_mode == TH_MODE_REALTIME) {
				  
                                            
                                   
       
				if (thread->quantum_remaining == 0) {
					KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_CANCEL_RT_DEADLINE) | DBG_FUNC_NONE,
					    (uintptr_t)thread_tid(thread), thread->realtime.deadline, thread->realtime.computation, 0);
					thread->realtime.deadline = UINT64_MAX;
				}
			} else {
#if defined(CONFIG_SCHED_TIMESHARE_CORE)
				  
                                            
                                              
                                         
       
				if (thread->quantum_remaining < min_std_quantum) {
					thread->reason |= AST_QUANTUM;
					thread->quantum_remaining += SCHED(initial_quantum_size)(thread);
				}
#endif                                  
			}

			  
                                           
                                        
      
			if ((thread->reason & (AST_HANDOFF | AST_QUANTUM)) == AST_HANDOFF) {
				self->quantum_remaining = thread->quantum_remaining;
				thread->reason |= AST_QUANTUM;
				thread->quantum_remaining = 0;
			} else {
#if defined(CONFIG_SCHED_MULTIQ)
				if (SCHED(sched_groups_enabled) &&
				    thread->sched_group == self->sched_group) {
					KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
					    MACHDBG_CODE(DBG_MACH_SCHED, MACH_QUANTUM_HANDOFF),
					    self->reason, (uintptr_t)thread_tid(thread),
					    self->quantum_remaining, thread->quantum_remaining, 0);

					self->quantum_remaining = thread->quantum_remaining;
					thread->quantum_remaining = 0;
					                                                                                      
				}
#endif                                   
			}

			thread->computation_metered += (processor->last_dispatch - thread->computation_epoch);

			if (!(thread->state & TH_WAIT)) {
				  
                      
       
				thread->last_made_runnable_time = thread->last_basepri_change_time = processor->last_dispatch;

				machine_thread_going_off_core(thread, FALSE, processor->last_dispatch, TRUE);

				ast_t reason = thread->reason;
				sched_options_t options = SCHED_NONE;

				if (reason & AST_REBALANCE) {
					options |= SCHED_REBALANCE;
					if (reason & AST_QUANTUM) {
						  
                                                                                     
                                                                                   
                                                                              
                                                                           
                                                                               
         
						options |= SCHED_PREEMPT;
					}
				}

				if (reason & AST_QUANTUM) {
					options |= SCHED_TAILQ;
				} else if (reason & AST_PREEMPT) {
					options |= SCHED_HEADQ;
				} else {
					options |= (SCHED_PREEMPT | SCHED_TAILQ);
				}

				thread_setrun(thread, options);

				KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
				    MACHDBG_CODE(DBG_MACH_SCHED, MACH_DISPATCH) | DBG_FUNC_NONE,
				    (uintptr_t)thread_tid(thread), thread->reason, thread->state,
				    sched_run_buckets[TH_BUCKET_RUN], 0);

				if (thread->wake_active) {
					thread->wake_active = FALSE;
					thread_unlock(thread);

					thread_wakeup(&thread->wake_active);
				} else {
					thread_unlock(thread);
				}

				wake_unlock(thread);
			} else {
				  
               
       
				boolean_t should_terminate = FALSE;
				uint32_t new_run_count;
				int thread_state = thread->state;

				                                         
                                            
                                          
       
				if ((thread_state & (TH_TERMINATE | TH_TERMINATE2)) == TH_TERMINATE) {
					should_terminate = TRUE;
					thread_state |= TH_TERMINATE2;
				}

				timer_stop(&thread->runnable_timer, processor->last_dispatch);

				thread_state &= ~TH_RUN;
				thread->state = thread_state;

				thread->last_made_runnable_time = thread->last_basepri_change_time = THREAD_NOT_RUNNABLE;
				thread->chosen_processor = PROCESSOR_NULL;

				new_run_count = SCHED(run_count_decr)(thread);

#if CONFIG_SCHED_AUTO_JOIN
				if ((thread->sched_flags & TH_SFLAG_THREAD_GROUP_AUTO_JOIN) != 0) {
					work_interval_auto_join_unwind(thread);
				}
#endif                             

#if CONFIG_SCHED_SFI
				if (thread->reason & AST_SFI) {
					thread->wait_sfi_begin_time = processor->last_dispatch;
				}
#endif
				machine_thread_going_off_core(thread, should_terminate, processor->last_dispatch, FALSE);

				KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
				    MACHDBG_CODE(DBG_MACH_SCHED, MACH_DISPATCH) | DBG_FUNC_NONE,
				    (uintptr_t)thread_tid(thread), thread->reason, thread_state,
				    new_run_count, 0);

				if (thread_state & TH_WAIT_REPORT) {
					(*thread->sched_call)(SCHED_CALL_BLOCK, thread);
				}

				if (thread->wake_active) {
					thread->wake_active = FALSE;
					thread_unlock(thread);

					thread_wakeup(&thread->wake_active);
				} else {
					thread_unlock(thread);
				}

				wake_unlock(thread);

				if (should_terminate) {
					thread_terminate_enqueue(thread);
				}
			}
		}
		  
                                                                       
                                    
     
		thread = THREAD_NULL;
	}

	int urgency = THREAD_URGENCY_NONE;
	uint64_t latency = 0;

	                                                              
	thread_lock(self);

	if (!(self->state & TH_IDLE)) {
		uint64_t        arg1, arg2;

#if CONFIG_SCHED_SFI
		ast_t                   new_ast;

		new_ast = sfi_thread_needs_ast(self, NULL);

		if (new_ast != AST_NONE) {
			ast_on(new_ast);
		}
#endif

		assertf(processor->last_dispatch >= self->last_made_runnable_time,
		    "Non-monotonic time? dispatch at 0x%llx, runnable at 0x%llx",
		    processor->last_dispatch, self->last_made_runnable_time);

		assert(self->last_made_runnable_time <= self->last_basepri_change_time);

		latency = processor->last_dispatch - self->last_made_runnable_time;
		assert(latency >= self->same_pri_latency);

		urgency = thread_get_urgency(self, &arg1, &arg2);

		thread_tell_urgency(urgency, arg1, arg2, latency, self);

		  
                                         
     
		if (self->quantum_remaining == 0) {
			thread_quantum_init(self);
		}

		  
                                        
     
		processor->quantum_end = processor->last_dispatch +
		    self->quantum_remaining;

		running_timer_setup(processor, RUNNING_TIMER_QUANTUM, self,
		    processor->quantum_end, processor->last_dispatch);
		if (was_idle) {
			  
                                                                    
                         
      
			kperf_running_setup(processor, processor->last_dispatch);
		}
		running_timers_activate(processor);
		processor->first_timeslice = TRUE;
	} else {
		running_timers_deactivate(processor);
		processor->first_timeslice = FALSE;
		thread_tell_urgency(THREAD_URGENCY_NONE, 0, 0, 0, self);
	}

	assert(self->block_hint == kThreadWaitNone);
	self->computation_epoch = processor->last_dispatch;
	self->reason = AST_NONE;
	processor->starting_pri = self->sched_pri;

	thread_unlock(self);

	machine_thread_going_on_core(self, urgency, latency, self->same_pri_latency,
	    processor->last_dispatch);

#if defined(CONFIG_SCHED_DEFERRED_AST)
	  
                                                                
                  
    
	if ((os_atomic_load(&sched_run_buckets[TH_BUCKET_RUN], relaxed) == 1) && !(self->state & TH_IDLE)) {
		pset_cancel_deferred_dispatch(processor->processor_set, processor);
	}
#endif
}

  
                       
  
                                                     
                     
  
                                                          
                                                          
                                                            
                         
   
wait_result_t
thread_block_reason(
	thread_continue_t       continuation,
	void                            *parameter,
	ast_t                           reason)
{
	thread_t        self = current_thread();
	processor_t     processor;
	thread_t        new_thread;
	spl_t           s;

	s = splsched();

	processor = current_processor();

	                                                              
	if (reason & AST_YIELD) {
		processor->first_timeslice = FALSE;
	}

	                                         
	ast_off(AST_SCHEDULING);

#if PROC_REF_DEBUG
	if ((continuation != NULL) && (self->task != kernel_task)) {
		if (uthread_get_proc_refcount(self->uthread) != 0) {
			panic("thread_block_reason with continuation uthread %p with uu_proc_refcount != 0", self->uthread);
		}
	}
#endif

	self->continuation = continuation;
	self->parameter = parameter;

	if (self->state & ~(TH_RUN | TH_IDLE)) {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    MACHDBG_CODE(DBG_MACH_SCHED, MACH_BLOCK),
		    reason, VM_KERNEL_UNSLIDE(continuation), 0, 0, 0);
	}

	do {
		thread_lock(self);
		new_thread = thread_select(self, processor, &reason);
		thread_unlock(self);
	} while (!thread_invoke(self, new_thread, reason));

	splx(s);

	return self->wait_result;
}

  
                
  
                                                        
   
wait_result_t
thread_block(
	thread_continue_t       continuation)
{
	return thread_block_reason(continuation, NULL, AST_NONE);
}

wait_result_t
thread_block_parameter(
	thread_continue_t       continuation,
	void                            *parameter)
{
	return thread_block_reason(continuation, parameter, AST_NONE);
}

  
              
  
                                                 
                                                      
  
                                                       
  
                      
   
int
thread_run(
	thread_t                        self,
	thread_continue_t       continuation,
	void                            *parameter,
	thread_t                        new_thread)
{
	ast_t reason = AST_NONE;

	if ((self->state & TH_IDLE) == 0) {
		reason = AST_HANDOFF;
	}

	  
                                            
                                                     
    
	if (new_thread->chosen_processor == NULL) {
		new_thread->chosen_processor = current_processor();
	}

	self->continuation = continuation;
	self->parameter = parameter;

	while (!thread_invoke(self, new_thread, reason)) {
		                                                                          
		processor_t processor = current_processor();

		reason = AST_NONE;

		thread_lock(self);
		new_thread = thread_select(self, processor, &reason);
		thread_unlock(self);
	}

	return self->wait_result;
}

  
                   
  
                                                  
                                    
  
                                                 
                                   
   
void
thread_continue(
	thread_t        thread)
{
	thread_t                self = current_thread();
	thread_continue_t       continuation;
	void                    *parameter;

	DTRACE_SCHED(on__cpu);

	continuation = self->continuation;
	parameter = self->parameter;

	assert(continuation != NULL);

#if KPERF
	kperf_on_cpu(self, continuation, NULL);
#endif

	thread_dispatch(thread, self);

	self->continuation = self->parameter = NULL;

#if INTERRUPT_MASKED_DEBUG
	                                                   
	ml_spin_debug_clear(self);
#endif

	TLOG(1, "thread_continue: calling call_continuation\n");

	boolean_t enable_interrupts = TRUE;

	                                                                    
	if (thread == THREAD_NULL || (self->state & TH_IDLE)) {
		enable_interrupts = FALSE;
	}

	call_continuation(continuation, parameter, self->wait_result, enable_interrupts);
	              
}

void
thread_quantum_init(thread_t thread)
{
	if (thread->sched_mode == TH_MODE_REALTIME) {
		thread->quantum_remaining = thread->realtime.computation;
	} else {
		thread->quantum_remaining = SCHED(initial_quantum_size)(thread);
	}
}

uint32_t
sched_timeshare_initial_quantum_size(thread_t thread)
{
	if ((thread != THREAD_NULL) && thread->th_sched_bucket == TH_BUCKET_SHARE_BG) {
		return bg_quantum;
	} else {
		return std_quantum;
	}
}

  
                  
  
                                           
   
void
run_queue_init(
	run_queue_t             rq)
{
	rq->highq = NOPRI;
	for (u_int i = 0; i < BITMAP_LEN(NRQS); i++) {
		rq->bitmap[i] = 0;
	}
	rq->urgency = rq->count = 0;
	for (int i = 0; i < NRQS; i++) {
		circle_queue_init(&rq->queues[i]);
	}
}

  
                     
  
                                              
                                   
  
                                                              
                                 
   
thread_t
run_queue_dequeue(
	run_queue_t     rq,
	sched_options_t options)
{
	thread_t        thread;
	circle_queue_t  queue = &rq->queues[rq->highq];

	if (options & SCHED_HEADQ) {
		thread = cqe_dequeue_head(queue, struct thread, runq_links);
	} else {
		thread = cqe_dequeue_tail(queue, struct thread, runq_links);
	}

	assert(thread != THREAD_NULL);
	assert_thread_magic(thread);

	thread->runq = PROCESSOR_NULL;
	SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
	rq->count--;
	if (SCHED(priority_is_urgent)(rq->highq)) {
		rq->urgency--; assert(rq->urgency >= 0);
	}
	if (circle_queue_empty(queue)) {
		bitmap_clear(rq->bitmap, rq->highq);
		rq->highq = bitmap_first(rq->bitmap, NRQS);
	}

	return thread;
}

  
                     
  
                                              
  
                                                              
                  
   
boolean_t
run_queue_enqueue(
	run_queue_t      rq,
	thread_t         thread,
	sched_options_t  options)
{
	circle_queue_t  queue = &rq->queues[thread->sched_pri];
	boolean_t       result = FALSE;

	assert_thread_magic(thread);

	if (circle_queue_empty(queue)) {
		circle_enqueue_tail(queue, &thread->runq_links);

		rq_bitmap_set(rq->bitmap, thread->sched_pri);
		if (thread->sched_pri > rq->highq) {
			rq->highq = thread->sched_pri;
			result = TRUE;
		}
	} else {
		if (options & SCHED_TAILQ) {
			circle_enqueue_tail(queue, &thread->runq_links);
		} else {
			circle_enqueue_head(queue, &thread->runq_links);
		}
	}
	if (SCHED(priority_is_urgent)(thread->sched_pri)) {
		rq->urgency++;
	}
	SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
	rq->count++;

	return result;
}

  
                    
  
                                            
  
                                
   
void
run_queue_remove(
	run_queue_t    rq,
	thread_t       thread)
{
	circle_queue_t  queue = &rq->queues[thread->sched_pri];

	assert(thread->runq != PROCESSOR_NULL);
	assert_thread_magic(thread);

	circle_dequeue(queue, &thread->runq_links);
	SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
	rq->count--;
	if (SCHED(priority_is_urgent)(thread->sched_pri)) {
		rq->urgency--; assert(rq->urgency >= 0);
	}

	if (circle_queue_empty(queue)) {
		                             
		bitmap_clear(rq->bitmap, thread->sched_pri);
		rq->highq = bitmap_first(rq->bitmap, NRQS);
	}

	thread->runq = PROCESSOR_NULL;
}

  
                      
  
                                               
                                      
  
                                
   
thread_t
run_queue_peek(
	run_queue_t    rq)
{
	if (rq->count > 0) {
		circle_queue_t queue = &rq->queues[rq->highq];
		thread_t thread = cqe_queue_first(queue, struct thread, runq_links);
		assert_thread_magic(thread);
		return thread;
	} else {
		return THREAD_NULL;
	}
}

rt_queue_t
sched_rtlocal_runq(processor_set_t pset)
{
	return &pset->rt_runq;
}

void
sched_rtlocal_init(processor_set_t pset)
{
	pset_rt_init(pset);
}

void
sched_rtlocal_queue_shutdown(processor_t processor)
{
	processor_set_t pset = processor->processor_set;
	thread_t        thread;
	queue_head_t    tqueue;

	pset_lock(pset);

	                                                                                                          
	if ((pset->online_processor_count > 0) && pset_is_recommended(pset)) {
		pset_unlock(pset);
		return;
	}

	queue_init(&tqueue);

	while (rt_runq_count(pset) > 0) {
		thread = qe_dequeue_head(&pset->rt_runq.queue, struct thread, runq_links);
		thread->runq = PROCESSOR_NULL;
		SCHED_STATS_RUNQ_CHANGE(&pset->rt_runq.runq_stats, rt_runq_count(pset));
		rt_runq_count_decr(pset);
		enqueue_tail(&tqueue, &thread->runq_links);
	}
	sched_update_pset_load_average(pset, 0);
	pset_unlock(pset);

	qe_foreach_element_safe(thread, &tqueue, runq_links) {
		remqueue(&thread->runq_links);

		thread_lock(thread);

		thread_setrun(thread, SCHED_TAILQ);

		thread_unlock(thread);
	}
}

                                                                       
void
sched_rtlocal_runq_scan(sched_update_scan_context_t scan_context)
{
	thread_t        thread;

	pset_node_t node = &pset_node0;
	processor_set_t pset = node->psets;

	spl_t s = splsched();
	do {
		while (pset != NULL) {
			pset_lock(pset);

			qe_foreach_element_safe(thread, &pset->rt_runq.queue, runq_links) {
				if (thread->last_made_runnable_time < scan_context->earliest_rt_make_runnable_time) {
					scan_context->earliest_rt_make_runnable_time = thread->last_made_runnable_time;
				}
			}

			pset_unlock(pset);

			pset = pset->pset_list;
		}
	} while (((node = node->node_list) != NULL) && ((pset = node->psets) != NULL));
	splx(s);
}

int64_t
sched_rtlocal_runq_count_sum(void)
{
	pset_node_t node = &pset_node0;
	processor_set_t pset = node->psets;
	int64_t count = 0;

	do {
		while (pset != NULL) {
			count += pset->rt_runq.runq_stats.count_sum;

			pset = pset->pset_list;
		}
	} while (((node = node->node_list) != NULL) && ((pset = node->psets) != NULL));

	return count;
}

  
                         
  
                                           
   
static boolean_t
realtime_queue_insert(processor_t processor, processor_set_t pset, thread_t thread)
{
	queue_t     queue       = &SCHED(rt_runq)(pset)->queue;
	uint64_t    deadline    = thread->realtime.deadline;
	boolean_t   preempt     = FALSE;

	pset_assert_locked(pset);

	if (queue_empty(queue)) {
		enqueue_tail(queue, &thread->runq_links);
		preempt = TRUE;
	} else {
		                                                  
		queue_entry_t iter;
		qe_foreach(iter, queue) {
			thread_t iter_thread = qe_element(iter, struct thread, runq_links);
			assert_thread_magic(iter_thread);

			if (deadline < iter_thread->realtime.deadline) {
				if (iter == queue_first(queue)) {
					preempt = TRUE;
				}
				insque(&thread->runq_links, queue_prev(iter));
				break;
			} else if (iter == queue_last(queue)) {
				enqueue_tail(queue, &thread->runq_links);
				break;
			}
		}
	}

	thread->runq = processor;
	SCHED_STATS_RUNQ_CHANGE(&SCHED(rt_runq)(pset)->runq_stats, rt_runq_count(pset));
	rt_runq_count_incr(pset);

	return preempt;
}

#define MAX_BACKUP_PROCESSORS 7
#if defined(__x86_64__)
#define DEFAULT_BACKUP_PROCESSORS 1
#else
#define DEFAULT_BACKUP_PROCESSORS 0
#endif

int sched_rt_n_backup_processors = DEFAULT_BACKUP_PROCESSORS;

int
sched_get_rt_n_backup_processors(void)
{
	return sched_rt_n_backup_processors;
}

void
sched_set_rt_n_backup_processors(int n)
{
	if (n < 0) {
		n = 0;
	} else if (n > MAX_BACKUP_PROCESSORS) {
		n = MAX_BACKUP_PROCESSORS;
	}

	sched_rt_n_backup_processors = n;
}

  
                   
  
                                            
  
                                               
                                       
   
static void
realtime_setrun(
	processor_t                     chosen_processor,
	thread_t                        thread)
{
	processor_set_t pset = chosen_processor->processor_set;
	pset_assert_locked(pset);
	ast_t preempt;

	int n_backup = 0;

	if (thread->realtime.constraint <= rt_constraint_threshold) {
		n_backup = sched_rt_n_backup_processors;
	}
	assert((n_backup >= 0) && (n_backup <= MAX_BACKUP_PROCESSORS));

	sched_ipi_type_t ipi_type[MAX_BACKUP_PROCESSORS + 1] = {};
	processor_t ipi_processor[MAX_BACKUP_PROCESSORS + 1] = {};

	thread->chosen_processor = chosen_processor;

	                               
	assert(thread->bound_processor == PROCESSOR_NULL);

	realtime_queue_insert(chosen_processor, pset, thread);

	processor_t processor = chosen_processor;
	bool chosen_process_is_secondary = chosen_processor->processor_primary != chosen_processor;

	int count = 0;
	for (int i = 0; i <= n_backup; i++) {
		if (i > 0) {
			processor = choose_processor_for_realtime_thread(pset, chosen_processor, chosen_process_is_secondary);
			if ((processor == PROCESSOR_NULL) || (sched_avoid_cpu0 && (processor->cpu_id == 0))) {
				break;
			}
			SCHED_DEBUG_CHOOSE_PROCESSOR_KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_CHOOSE_PROCESSOR) | DBG_FUNC_NONE,
			    (uintptr_t)thread_tid(thread), (uintptr_t)-3, processor->cpu_id, processor->state, 0);
		}
		ipi_type[i] = SCHED_IPI_NONE;
		ipi_processor[i] = processor;
		count++;

		if (processor->current_pri < BASEPRI_RTQUEUES) {
			preempt = (AST_PREEMPT | AST_URGENT);
		} else if (thread->realtime.deadline < processor->deadline) {
			preempt = (AST_PREEMPT | AST_URGENT);
		} else {
			preempt = AST_NONE;
		}

		if (preempt != AST_NONE) {
			if (processor->state == PROCESSOR_IDLE) {
				processor_state_update_from_thread(processor, thread);
				processor->deadline = thread->realtime.deadline;
				pset_update_processor_state(pset, processor, PROCESSOR_DISPATCHING);
				if (processor == current_processor()) {
					ast_on(preempt);

					if ((preempt & AST_URGENT) == AST_URGENT) {
						bit_set(pset->pending_AST_URGENT_cpu_mask, processor->cpu_id);
					}

					if ((preempt & AST_PREEMPT) == AST_PREEMPT) {
						bit_set(pset->pending_AST_PREEMPT_cpu_mask, processor->cpu_id);
					}
				} else {
					ipi_type[i] = sched_ipi_action(processor, thread, true, SCHED_IPI_EVENT_PREEMPT);
				}
			} else if (processor->state == PROCESSOR_DISPATCHING) {
				if ((processor->current_pri < thread->sched_pri) || (processor->deadline > thread->realtime.deadline)) {
					processor_state_update_from_thread(processor, thread);
					processor->deadline = thread->realtime.deadline;
				}
				bit_set(pset->pending_AST_URGENT_cpu_mask, processor->cpu_id);
			} else {
				if (processor == current_processor()) {
					ast_on(preempt);

					if ((preempt & AST_URGENT) == AST_URGENT) {
						bit_set(pset->pending_AST_URGENT_cpu_mask, processor->cpu_id);
					}

					if ((preempt & AST_PREEMPT) == AST_PREEMPT) {
						bit_set(pset->pending_AST_PREEMPT_cpu_mask, processor->cpu_id);
					}
				} else {
					ipi_type[i] = sched_ipi_action(processor, thread, false, SCHED_IPI_EVENT_PREEMPT);
				}
			}
		} else {
			                                                                                                             
		}
	}

	pset_unlock(pset);

	assert((count > 0) && (count <= (n_backup + 1)));
	for (int i = 0; i < count; i++) {
		assert(ipi_processor[i] != PROCESSOR_NULL);
		sched_ipi_perform(ipi_processor[i], ipi_type[i]);
	}
}


sched_ipi_type_t
sched_ipi_deferred_policy(processor_set_t pset, processor_t dst,
    __unused sched_ipi_event_t event)
{
#if defined(CONFIG_SCHED_DEFERRED_AST)
	if (!bit_test(pset->pending_deferred_AST_cpu_mask, dst->cpu_id)) {
		return SCHED_IPI_DEFERRED;
	}
#else                                
	panic("Request for deferred IPI on an unsupported platform; pset: %p CPU: %d", pset, dst->cpu_id);
#endif                                
	return SCHED_IPI_NONE;
}

sched_ipi_type_t
sched_ipi_action(processor_t dst, thread_t thread, boolean_t dst_idle, sched_ipi_event_t event)
{
	sched_ipi_type_t ipi_type = SCHED_IPI_NONE;
	assert(dst != NULL);

	processor_set_t pset = dst->processor_set;
	if (current_processor() == dst) {
		return SCHED_IPI_NONE;
	}

	if (bit_test(pset->pending_AST_URGENT_cpu_mask, dst->cpu_id)) {
		return SCHED_IPI_NONE;
	}

	ipi_type = SCHED(ipi_policy)(dst, thread, dst_idle, event);
	switch (ipi_type) {
	case SCHED_IPI_NONE:
		return SCHED_IPI_NONE;
#if defined(CONFIG_SCHED_DEFERRED_AST)
	case SCHED_IPI_DEFERRED:
		bit_set(pset->pending_deferred_AST_cpu_mask, dst->cpu_id);
		break;
#endif                                
	default:
		bit_set(pset->pending_AST_URGENT_cpu_mask, dst->cpu_id);
		bit_set(pset->pending_AST_PREEMPT_cpu_mask, dst->cpu_id);
		break;
	}
	return ipi_type;
}

sched_ipi_type_t
sched_ipi_policy(processor_t dst, thread_t thread, boolean_t dst_idle, sched_ipi_event_t event)
{
	sched_ipi_type_t ipi_type = SCHED_IPI_NONE;
	boolean_t deferred_ipi_supported = false;
	processor_set_t pset = dst->processor_set;

#if defined(CONFIG_SCHED_DEFERRED_AST)
	deferred_ipi_supported = true;
#endif                                

	switch (event) {
	case SCHED_IPI_EVENT_SPILL:
	case SCHED_IPI_EVENT_SMT_REBAL:
	case SCHED_IPI_EVENT_REBALANCE:
	case SCHED_IPI_EVENT_BOUND_THR:
		  
                                                             
                                         
     
		ipi_type = dst_idle ? SCHED_IPI_IDLE : SCHED_IPI_IMMEDIATE;
		break;
	case SCHED_IPI_EVENT_PREEMPT:
		                                                               
		if (thread && (thread->sched_pri >= BASEPRI_RTQUEUES)) {
			ipi_type = dst_idle ? SCHED_IPI_IDLE : SCHED_IPI_IMMEDIATE;
			break;
		}

		  
                                   
                                               
                                                                                  
     
		if (deferred_ipi_supported && dst_idle) {
			return sched_ipi_deferred_policy(pset, dst, event);
		}
		ipi_type = dst_idle ? SCHED_IPI_IDLE : SCHED_IPI_IMMEDIATE;
		break;
	default:
		panic("Unrecognized scheduler IPI event type %d", event);
	}
	assert(ipi_type != SCHED_IPI_NONE);
	return ipi_type;
}

void
sched_ipi_perform(processor_t dst, sched_ipi_type_t ipi)
{
	switch (ipi) {
	case SCHED_IPI_NONE:
		break;
	case SCHED_IPI_IDLE:
		machine_signal_idle(dst);
		break;
	case SCHED_IPI_IMMEDIATE:
		cause_ast_check(dst);
		break;
	case SCHED_IPI_DEFERRED:
		machine_signal_idle_deferred(dst);
		break;
	default:
		panic("Unrecognized scheduler IPI type: %d", ipi);
	}
}

#if defined(CONFIG_SCHED_TIMESHARE_CORE)

boolean_t
priority_is_urgent(int priority)
{
	return bitmap_test(sched_preempt_pri, priority) ? TRUE : FALSE;
}

#endif                                  

  
                    
  
                                       
             
  
                                               
                                       
   
static void
processor_setrun(
	processor_t                     processor,
	thread_t                        thread,
	integer_t                       options)
{
	processor_set_t pset = processor->processor_set;
	pset_assert_locked(pset);
	ast_t preempt;
	enum { eExitIdle, eInterruptRunning, eDoNothing } ipi_action = eDoNothing;

	sched_ipi_type_t ipi_type = SCHED_IPI_NONE;

	thread->chosen_processor = processor;

	  
                        
    
#if defined(CONFIG_SCHED_DEFERRED_AST)
	                                                                           
#endif
	if (SCHED(priority_is_urgent)(thread->sched_pri) && thread->sched_pri > processor->current_pri) {
		preempt = (AST_PREEMPT | AST_URGENT);
	} else if (processor->current_is_eagerpreempt) {
		preempt = (AST_PREEMPT | AST_URGENT);
	} else if ((thread->sched_mode == TH_MODE_TIMESHARE) && (thread->sched_pri < thread->base_pri)) {
		if (SCHED(priority_is_urgent)(thread->base_pri) && thread->sched_pri > processor->current_pri) {
			preempt = (options & SCHED_PREEMPT)? AST_PREEMPT: AST_NONE;
		} else {
			preempt = AST_NONE;
		}
	} else {
		preempt = (options & SCHED_PREEMPT)? AST_PREEMPT: AST_NONE;
	}

	if ((options & (SCHED_PREEMPT | SCHED_REBALANCE)) == (SCHED_PREEMPT | SCHED_REBALANCE)) {
		  
                                                                                 
                                                                               
                                                                          
                                                                       
     
		preempt |= AST_PREEMPT;
	}

	SCHED(processor_enqueue)(processor, thread, options);
	sched_update_pset_load_average(pset, 0);

	if (preempt != AST_NONE) {
		if (processor->state == PROCESSOR_IDLE) {
			processor_state_update_from_thread(processor, thread);
			processor->deadline = UINT64_MAX;
			pset_update_processor_state(pset, processor, PROCESSOR_DISPATCHING);
			ipi_action = eExitIdle;
		} else if (processor->state == PROCESSOR_DISPATCHING) {
			if (processor->current_pri < thread->sched_pri) {
				processor_state_update_from_thread(processor, thread);
				processor->deadline = UINT64_MAX;
			}
		} else if ((processor->state == PROCESSOR_RUNNING ||
		    processor->state == PROCESSOR_SHUTDOWN) &&
		    (thread->sched_pri >= processor->current_pri)) {
			ipi_action = eInterruptRunning;
		}
	} else {
		  
                                                                       
                                                       
     
		if (processor->state == PROCESSOR_SHUTDOWN &&
		    thread->sched_pri >= processor->current_pri) {
			ipi_action = eInterruptRunning;
		} else if (processor->state == PROCESSOR_IDLE) {
			processor_state_update_from_thread(processor, thread);
			processor->deadline = UINT64_MAX;
			pset_update_processor_state(pset, processor, PROCESSOR_DISPATCHING);

			ipi_action = eExitIdle;
		}
	}

	if (ipi_action != eDoNothing) {
		if (processor == current_processor()) {
			if ((preempt = csw_check_locked(processor->active_thread, processor, pset, AST_NONE)) != AST_NONE) {
				ast_on(preempt);
			}

			if ((preempt & AST_URGENT) == AST_URGENT) {
				bit_set(pset->pending_AST_URGENT_cpu_mask, processor->cpu_id);
			} else {
				bit_clear(pset->pending_AST_URGENT_cpu_mask, processor->cpu_id);
			}

			if ((preempt & AST_PREEMPT) == AST_PREEMPT) {
				bit_set(pset->pending_AST_PREEMPT_cpu_mask, processor->cpu_id);
			} else {
				bit_clear(pset->pending_AST_PREEMPT_cpu_mask, processor->cpu_id);
			}
		} else {
			sched_ipi_event_t event = (options & SCHED_REBALANCE) ? SCHED_IPI_EVENT_REBALANCE : SCHED_IPI_EVENT_PREEMPT;
			ipi_type = sched_ipi_action(processor, thread, (ipi_action == eExitIdle), event);
		}
	}
	pset_unlock(pset);
	sched_ipi_perform(processor, ipi_type);
}

  
                    
  
                                          
                        
  
                                             
            
   
static processor_set_t
choose_next_pset(
	processor_set_t         pset)
{
	processor_set_t         nset = pset;

	do {
		nset = next_pset(nset);
	} while (nset->online_processor_count < 1 && nset != pset);

	return nset;
}

inline static processor_set_t
change_locked_pset(processor_set_t current_pset, processor_set_t new_pset)
{
	if (current_pset != new_pset) {
		pset_unlock(current_pset);
		pset_lock(new_pset);
	}

	return new_pset;
}

  
                    
  
                                                  
                                                   
            
  
                                                       
  
                                                       
                                              
   
processor_t
choose_processor(
	processor_set_t         starting_pset,
	processor_t             processor,
	thread_t                thread)
{
	processor_set_t pset = starting_pset;
	processor_set_t nset;

	assert(thread->sched_pri <= BASEPRI_RTQUEUES);

	  
                                                  
    

	                                                                      
	if (processor != PROCESSOR_NULL) {
		processor = processor->processor_primary;
	}

	  
                                                        
                                                       
                      
    
	if (pset->online_processor_count) {
		if ((processor == PROCESSOR_NULL) || (processor->processor_set == pset && processor->state == PROCESSOR_IDLE)) {
			processor_t mc_processor = machine_choose_processor(pset, processor);
			if (mc_processor != PROCESSOR_NULL) {
				processor = mc_processor->processor_primary;
			}
		}
	}

	  
                                                                
                                                                
                                                               
             
    
	if (processor != PROCESSOR_NULL) {
		if (processor->processor_set != pset) {
			processor = PROCESSOR_NULL;
		} else if (!processor->is_recommended) {
			processor = PROCESSOR_NULL;
		} else {
			switch (processor->state) {
			case PROCESSOR_START:
			case PROCESSOR_SHUTDOWN:
			case PROCESSOR_OFF_LINE:
				  
                                                                       
       
				processor = PROCESSOR_NULL;
				break;
			case PROCESSOR_IDLE:
				  
                                                                          
                                                                       
                                             
       
				if ((thread->sched_pri < BASEPRI_RTQUEUES) || processor_is_fast_track_candidate_for_realtime_thread(pset, processor)) {
					return processor;
				}
				processor = PROCESSOR_NULL;
				break;
			case PROCESSOR_RUNNING:
			case PROCESSOR_DISPATCHING:
				  
                                                       
                                                       
                                                    
       
				if ((thread->sched_pri >= BASEPRI_RTQUEUES) &&
				    processor_is_fast_track_candidate_for_realtime_thread(pset, processor)) {
					return processor;
				}

				                                                 
				break;
			default:
				processor = PROCESSOR_NULL;
				break;
			}
		}
	}

	  
                                                
                                               
                                                
                                                
                  
   
                                                  
                                             
                                                    
                                        
    

	integer_t lowest_priority = MAXPRI + 1;
	integer_t lowest_secondary_priority = MAXPRI + 1;
	integer_t lowest_unpaired_primary_priority = MAXPRI + 1;
	integer_t lowest_idle_secondary_priority = MAXPRI + 1;
	integer_t lowest_count = INT_MAX;
	uint64_t  furthest_deadline = 1;
	processor_t lp_processor = PROCESSOR_NULL;
	processor_t lp_unpaired_primary_processor = PROCESSOR_NULL;
	processor_t lp_idle_secondary_processor = PROCESSOR_NULL;
	processor_t lp_paired_secondary_processor = PROCESSOR_NULL;
	processor_t lc_processor = PROCESSOR_NULL;
	processor_t fd_processor = PROCESSOR_NULL;

	if (processor != PROCESSOR_NULL) {
		                                                  
		assert(processor->state == PROCESSOR_RUNNING || processor->state == PROCESSOR_DISPATCHING);

		lowest_priority = processor->current_pri;
		lp_processor = processor;

		if (processor->current_pri >= BASEPRI_RTQUEUES) {
			furthest_deadline = processor->deadline;
			fd_processor = processor;
		}

		lowest_count = SCHED(processor_runq_count)(processor);
		lc_processor = processor;
	}

	if (thread->sched_pri >= BASEPRI_RTQUEUES) {
		pset_node_t node = pset->node;
		int consider_secondaries = (!pset->is_SMT) || (bit_count(node->pset_map) == 1) || (node->pset_non_rt_primary_map == 0);
		for (; consider_secondaries < 2; consider_secondaries++) {
			pset = change_locked_pset(pset, starting_pset);
			do {
				processor = choose_processor_for_realtime_thread(pset, PROCESSOR_NULL, consider_secondaries);
				if (processor) {
					return processor;
				}

				                                                                 

				nset = next_pset(pset);

				if (nset != starting_pset) {
					pset = change_locked_pset(pset, nset);
				}
			} while (nset != starting_pset);
		}
		                                                                       
		pset = change_locked_pset(pset, starting_pset);
	}

	do {
		  
                                                      
     

		uint64_t idle_primary_map = (pset->cpu_state_map[PROCESSOR_IDLE] &
		    pset->primary_map &
		    pset->recommended_bitmask);

		                                                               
		assert((idle_primary_map & pset->pending_AST_URGENT_cpu_mask) == 0);

		int cpuid = lsb_first(idle_primary_map);
		if (cpuid >= 0) {
			processor = processor_array[cpuid];
			return processor;
		}

		  
                                                                               
                             
     

		uint64_t active_map = ((pset->cpu_state_map[PROCESSOR_RUNNING] | pset->cpu_state_map[PROCESSOR_DISPATCHING]) &
		    pset->recommended_bitmask &
		    ~pset->pending_AST_URGENT_cpu_mask);

		if (SCHED(priority_is_urgent)(thread->sched_pri) == FALSE) {
			active_map &= ~pset->pending_AST_PREEMPT_cpu_mask;
		}

		active_map = bit_ror64(active_map, (pset->last_chosen + 1));
		for (int rotid = lsb_first(active_map); rotid >= 0; rotid = lsb_next(active_map, rotid)) {
			cpuid = ((rotid + pset->last_chosen + 1) & 63);
			processor = processor_array[cpuid];

			integer_t cpri = processor->current_pri;
			processor_t primary = processor->processor_primary;
			if (primary != processor) {
				                                                                       
				if (!((primary->state == PROCESSOR_RUNNING) && processor_active_thread_no_smt(primary))) {
					if (cpri < lowest_secondary_priority) {
						lowest_secondary_priority = cpri;
						lp_paired_secondary_processor = processor;
					}
				}
			} else {
				if (cpri < lowest_priority) {
					lowest_priority = cpri;
					lp_processor = processor;
				}
			}

			if ((cpri >= BASEPRI_RTQUEUES) && (processor->deadline > furthest_deadline)) {
				furthest_deadline = processor->deadline;
				fd_processor = processor;
			}

			integer_t ccount = SCHED(processor_runq_count)(processor);
			if (ccount < lowest_count) {
				lowest_count = ccount;
				lc_processor = processor;
			}
		}

		  
                                                                                         
                                                               
     
		uint64_t idle_secondary_map = (pset->cpu_state_map[PROCESSOR_IDLE] &
		    ~pset->primary_map &
		    pset->recommended_bitmask);

		                                                               
		assert((idle_secondary_map & pset->pending_AST_URGENT_cpu_mask) == 0);
		assert((idle_secondary_map & pset->pending_AST_PREEMPT_cpu_mask) == 0);

		for (cpuid = lsb_first(idle_secondary_map); cpuid >= 0; cpuid = lsb_next(idle_secondary_map, cpuid)) {
			processor = processor_array[cpuid];

			processor_t cprimary = processor->processor_primary;

			integer_t primary_pri = cprimary->current_pri;

			  
                                                    
                                          
     
                                                         
                                                
      

			                                                                                
			if (cprimary->state == PROCESSOR_RUNNING &&
			    processor_active_thread_no_smt(cprimary)) {
				continue;
			}

			  
                                                                        
     
                                                                      
                         
      
			if (primary_pri < lowest_idle_secondary_priority) {
				lp_idle_secondary_processor = processor;
				lowest_idle_secondary_priority = primary_pri;
			}

			                                                                     
			if (primary_pri < lowest_unpaired_primary_priority) {
				                                                                                            
				if (cprimary->state != PROCESSOR_RUNNING &&
				    cprimary->state != PROCESSOR_DISPATCHING) {
					continue;
				}

				if (!cprimary->is_recommended) {
					continue;
				}

				                                                                      
				if (bit_test(pset->pending_AST_URGENT_cpu_mask, cprimary->cpu_id)) {
					continue;
				}

				if (SCHED(priority_is_urgent)(thread->sched_pri) == FALSE &&
				    bit_test(pset->pending_AST_PREEMPT_cpu_mask, cprimary->cpu_id)) {
					continue;
				}

				lowest_unpaired_primary_priority = primary_pri;
				lp_unpaired_primary_processor = cprimary;
			}
		}

		  
                                                                           
                                                                 
     
		if (thread->sched_pri > lowest_unpaired_primary_priority) {
			pset->last_chosen = lp_unpaired_primary_processor->cpu_id;
			return lp_unpaired_primary_processor;
		}

		  
                                                                         
                                 
                                                            
     
		if (thread->sched_pri > lowest_priority) {
			pset->last_chosen = lp_processor->cpu_id;
			return lp_processor;
		}

		if (thread->sched_pri >= BASEPRI_RTQUEUES) {
			  
                                                        
                                                   
                                                  
                                                      
                             
      

			if (sched_allow_rt_smt && (thread->sched_pri > lowest_secondary_priority)) {
				pset->last_chosen = lp_paired_secondary_processor->cpu_id;
				return lp_paired_secondary_processor;
			}

			if (thread->realtime.deadline < furthest_deadline) {
				return fd_processor;
			}
		}

		  
                                                                      
                                                                       
                                                                    
     
		if (lc_processor == PROCESSOR_NULL) {
			cpumap_t available_map = ((pset->cpu_state_map[PROCESSOR_IDLE] |
			    pset->cpu_state_map[PROCESSOR_RUNNING] |
			    pset->cpu_state_map[PROCESSOR_DISPATCHING]) &
			    pset->recommended_bitmask);
			cpuid = lsb_first(available_map);
			if (cpuid >= 0) {
				lc_processor = processor_array[cpuid];
				lowest_count = SCHED(processor_runq_count)(lc_processor);
			}
		}

		  
                                      
    
                                                                
                                                             
                                                      
                              
     
		nset = next_pset(pset);

		if (nset != starting_pset) {
			pset = change_locked_pset(pset, nset);
		}
	} while (nset != starting_pset);

	  
                                               
                                                 
                                                        
                                    
   
                                                        
                                                     
                                                                 
                                       
    
	boolean_t fallback_processor = false;
	do {
		                                                          
		if (lp_idle_secondary_processor != PROCESSOR_NULL) {
			processor = lp_idle_secondary_processor;
			lp_idle_secondary_processor = PROCESSOR_NULL;
		} else if (lp_paired_secondary_processor != PROCESSOR_NULL) {
			processor = lp_paired_secondary_processor;
			lp_paired_secondary_processor = PROCESSOR_NULL;
		} else if (lc_processor != PROCESSOR_NULL) {
			processor = lc_processor;
			lc_processor = PROCESSOR_NULL;
		} else {
			  
                                                               
                                                
     
                                                           
                                                             
                                                            
                                                    
      
			fallback_processor = true;
#if CONFIG_SCHED_EDGE
			processor = processor_array[lsb_first(starting_pset->primary_map)];
#else                        
			processor = master_processor;
#endif                        
		}

		  
                                            
                     
     
		pset = change_locked_pset(pset, processor->processor_set);

		  
                                                                 
                                                                 
                                                                 
                                                            
                          
    
                                                              
                                                             
                                   
     
		if (processor != master_processor && (fallback_processor == false) && (processor->state == PROCESSOR_SHUTDOWN || processor->state == PROCESSOR_OFF_LINE)) {
			processor = PROCESSOR_NULL;
		}
	} while (processor == PROCESSOR_NULL);

	pset->last_chosen = processor->cpu_id;
	return processor;
}

  
                                                 
                          
   
pset_node_t
sched_choose_node(__unused thread_t thread)
{
	return &pset_node0;
}

  
                        
  
                                                  
                                               
  
                                                  
                         
  
                                                                        
                                                    
   
processor_set_t
choose_starting_pset(pset_node_t node, thread_t thread, processor_t *processor_hint)
{
	processor_set_t pset;
	processor_t processor = PROCESSOR_NULL;

	if (thread->affinity_set != AFFINITY_SET_NULL) {
		  
                                  
     
		pset = thread->affinity_set->aset_pset;
	} else if (thread->last_processor != PROCESSOR_NULL) {
		  
                                           
     
		processor = thread->last_processor;
		pset = processor->processor_set;
	} else {
		  
                      
    
                                                
                                        
                                               
                                                       
     
		task_t          task = thread->task;

		pset = task->pset_hint;
		if (pset == PROCESSOR_SET_NULL) {
			pset = current_processor()->processor_set;
		}

		pset = choose_next_pset(pset);
	}

	if (!bit_test(node->pset_map, pset->pset_id)) {
		                                                      
		int id = lsb_first(node->pset_map);
		assert(id >= 0);
		pset = pset_array[id];
	}

	if (bit_count(node->pset_map) == 1) {
		                                     
		goto out;
	}

	bool avoid_cpu0 = false;

#if defined(__x86_64__)
	if ((thread->sched_pri >= BASEPRI_RTQUEUES) && sched_avoid_cpu0) {
		                                    
		avoid_cpu0 = true;
		                                                                           
		assert(bit_test(pset_array[0]->cpu_bitmask, 0));
	}
#endif

	if (thread->sched_pri >= BASEPRI_RTQUEUES) {
		pset_map_t rt_target_map = atomic_load(&node->pset_non_rt_primary_map);
		if ((avoid_cpu0 && pset->pset_id == 0) || !bit_test(rt_target_map, pset->pset_id)) {
			if (avoid_cpu0) {
				rt_target_map = bit_ror64(rt_target_map, 1);
			}
			int rotid = lsb_first(rt_target_map);
			if (rotid >= 0) {
				int id = avoid_cpu0 ? ((rotid + 1) & 63) : rotid;
				pset = pset_array[id];
				goto out;
			}
		}
		if (!pset->is_SMT || !sched_allow_rt_smt) {
			                                                                                                              
			goto out;
		}
		rt_target_map = atomic_load(&node->pset_non_rt_map);
		if ((avoid_cpu0 && pset->pset_id == 0) || !bit_test(rt_target_map, pset->pset_id)) {
			if (avoid_cpu0) {
				rt_target_map = bit_ror64(rt_target_map, 1);
			}
			int rotid = lsb_first(rt_target_map);
			if (rotid >= 0) {
				int id = avoid_cpu0 ? ((rotid + 1) & 63) : rotid;
				pset = pset_array[id];
				goto out;
			}
		}
		                                                                                                              
	} else {
		pset_map_t idle_map = atomic_load(&node->pset_idle_map);
		if (!bit_test(idle_map, pset->pset_id)) {
			int next_idle_pset_id = lsb_first(idle_map);
			if (next_idle_pset_id >= 0) {
				pset = pset_array[next_idle_pset_id];
			}
		}
	}

out:
	if ((processor != PROCESSOR_NULL) && (processor->processor_set != pset)) {
		processor = PROCESSOR_NULL;
	}
	if (processor != PROCESSOR_NULL) {
		*processor_hint = processor;
	}

	return pset;
}

  
                 
  
                                              
                                                  
                  
  
                         
   
void
thread_setrun(
	thread_t                        thread,
	sched_options_t                 options)
{
	processor_t                     processor;
	processor_set_t         pset;

	assert((thread->state & (TH_RUN | TH_WAIT | TH_UNINT | TH_TERMINATE | TH_TERMINATE2)) == TH_RUN);
	assert(thread->runq == PROCESSOR_NULL);

	  
                              
    
	if (SCHED(can_update_priority)(thread)) {
		SCHED(update_priority)(thread);
	}

	thread->sfi_class = sfi_thread_classify(thread);

	assert(thread->runq == PROCESSOR_NULL);

	if (thread->bound_processor == PROCESSOR_NULL) {
		  
                  
     
		processor_t processor_hint = PROCESSOR_NULL;
		pset_node_t node = SCHED(choose_node)(thread);
		processor_set_t starting_pset = choose_starting_pset(node, thread, &processor_hint);

		pset_lock(starting_pset);

		processor = SCHED(choose_processor)(starting_pset, processor_hint, thread);
		pset = processor->processor_set;
		task_t task = thread->task;
		task->pset_hint = pset;                                                     

		SCHED_DEBUG_CHOOSE_PROCESSOR_KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_CHOOSE_PROCESSOR) | DBG_FUNC_NONE,
		    (uintptr_t)thread_tid(thread), (uintptr_t)-1, processor->cpu_id, processor->state, 0);
	} else {
		  
                
    
                                               
     
		processor = thread->bound_processor;
		pset = processor->processor_set;
		pset_lock(pset);

		SCHED_DEBUG_CHOOSE_PROCESSOR_KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_CHOOSE_PROCESSOR) | DBG_FUNC_NONE,
		    (uintptr_t)thread_tid(thread), (uintptr_t)-2, processor->cpu_id, processor->state, 0);
	}

	  
                                                
                                                           
    
	if (thread->sched_pri >= BASEPRI_RTQUEUES) {
		realtime_setrun(processor, thread);
	} else {
		processor_setrun(processor, thread, options);
	}
	                          
	if (thread->bound_processor == PROCESSOR_NULL) {
		SCHED(check_spill)(pset, thread);
	}
}

processor_set_t
task_choose_pset(
	task_t          task)
{
	processor_set_t         pset = task->pset_hint;

	if (pset != PROCESSOR_SET_NULL) {
		pset = choose_next_pset(pset);
	}

	return pset;
}

  
                                  
                       
  
                                         
   
ast_t
csw_check(
	thread_t                thread,
	processor_t             processor,
	ast_t                   check_reason)
{
	processor_set_t pset = processor->processor_set;

	assert(thread == processor->active_thread);

	pset_lock(pset);

	processor_state_update_from_thread(processor, thread);

	ast_t preempt = csw_check_locked(thread, processor, pset, check_reason);

	                                                      

	if ((preempt & AST_URGENT) == 0) {
		bit_clear(pset->pending_AST_URGENT_cpu_mask, processor->cpu_id);
	}

	if ((preempt & AST_PREEMPT) == 0) {
		bit_clear(pset->pending_AST_PREEMPT_cpu_mask, processor->cpu_id);
	}

	pset_unlock(pset);

	return preempt;
}

  
                                        
                         
   
ast_t
csw_check_locked(
	thread_t                thread,
	processor_t             processor,
	processor_set_t         pset,
	ast_t                   check_reason)
{
	ast_t                   result;

	if (processor->first_timeslice) {
		if (rt_runq_count(pset) > 0) {
			return check_reason | AST_PREEMPT | AST_URGENT;
		}
	} else {
		if (rt_runq_count(pset) > 0) {
			if (BASEPRI_RTQUEUES > processor->current_pri) {
				return check_reason | AST_PREEMPT | AST_URGENT;
			} else {
				return check_reason | AST_PREEMPT;
			}
		}
	}

	  
                                                                                  
                                                              
                                                                                    
    
	if (!processor->is_recommended) {
		return check_reason | AST_PREEMPT | AST_URGENT;
	}

	result = SCHED(processor_csw_check)(processor);
	if (result != AST_NONE) {
		return check_reason | result | (thread_is_eager_preempt(thread) ? AST_URGENT : AST_NONE);
	}

	  
                            
   
                                         
    
	if (SCHED(avoid_processor_enabled) && SCHED(thread_avoid_processor)(processor, thread)) {
		return check_reason | AST_PREEMPT;
	}

	  
                                                                
                                                                       
   
                                                                     
                                                                                
                                                                  
    

	if (processor->current_pri < BASEPRI_RTQUEUES &&
	    processor->processor_primary != processor) {
		return check_reason | AST_PREEMPT;
	}

	if (thread->state & TH_SUSP) {
		return check_reason | AST_PREEMPT;
	}

#if CONFIG_SCHED_SFI
	  
                                                                
                
    
	result = sfi_thread_needs_ast(thread, NULL);
	if (result != AST_NONE) {
		return check_reason | result;
	}
#endif

	return AST_NONE;
}

  
                                                                  
                               
                     
   
void
ast_check(processor_t processor)
{
	if (processor->state != PROCESSOR_RUNNING &&
	    processor->state != PROCESSOR_SHUTDOWN) {
		return;
	}

	thread_t thread = processor->active_thread;

	assert(thread == current_thread());

	thread_lock(thread);

	  
                                      
                                                 
    
	ast_propagate(thread);

	  
                                                           
                           
    
	thread_urgency_t old_urgency = processor->current_urgency;
	perfcontrol_class_t old_perfctl_class = processor->current_perfctl_class;

	ast_t preempt;

	if ((preempt = csw_check(thread, processor, AST_NONE)) != AST_NONE) {
		ast_on(preempt);
	}

	if (old_urgency != processor->current_urgency) {
		  
                                                            
                                             
     
		uint64_t urgency_param1, urgency_param2;

		thread_urgency_t urgency = thread_get_urgency(thread, &urgency_param1, &urgency_param2);
		thread_tell_urgency(urgency, urgency_param1, urgency_param2, 0, thread);
	}

	thread_unlock(thread);

	if (old_perfctl_class != processor->current_perfctl_class) {
		  
                                                                   
                                                              
           
     

		machine_switch_perfcontrol_state_update(PERFCONTROL_ATTR_UPDATE,
		    mach_approximate_time(), 0, thread);
	}
}


  
                 
  
                                                      
  
                                              
  
                         
   
void
set_sched_pri(
	thread_t        thread,
	int16_t         new_priority,
	set_sched_pri_options_t options)
{
	bool is_current_thread = (thread == current_thread());
	bool removed_from_runq = false;
	bool lazy_update = ((options & SETPRI_LAZY) == SETPRI_LAZY);

	int16_t old_priority = thread->sched_pri;

	                                                                          
	if (new_priority == old_priority) {
#if CONFIG_SCHED_CLUTCH
		                                                                  
                                                             
                                                                     
                     
     
		if (thread->th_sched_bucket == TH_BUCKET_RUN) {
			assert(is_current_thread);
			SCHED(update_thread_bucket)(thread);
		}
#endif                          

		return;
	}

	if (is_current_thread) {
		assert(thread->state & TH_RUN);
		assert(thread->runq == PROCESSOR_NULL);
	} else {
		removed_from_runq = thread_run_queue_remove(thread);
	}

	thread->sched_pri = new_priority;

#if CONFIG_SCHED_CLUTCH
	  
                                                                           
                                                                         
                                                                         
    
	SCHED(update_thread_bucket)(thread);

#endif                          

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_CHANGE_PRIORITY),
	    (uintptr_t)thread_tid(thread),
	    thread->base_pri,
	    thread->sched_pri,
	    thread->sched_usage,
	    0);

	if (removed_from_runq) {
		thread_run_queue_reinsert(thread, SCHED_PREEMPT | SCHED_TAILQ);
	} else if (is_current_thread) {
		processor_t processor = thread->last_processor;
		assert(processor == current_processor());

		thread_urgency_t old_urgency = processor->current_urgency;

		  
                                                                              
                                                                          
                              
    
                                                             
                                              
     
		if (!lazy_update && new_priority < old_priority) {
			ast_t preempt;

			if ((preempt = csw_check(thread, processor, AST_NONE)) != AST_NONE) {
				ast_on(preempt);
			}
		} else {
			processor_state_update_from_thread(processor, thread);
		}

		  
                                                                              
                                                                              
                                                                                
                                                         
     
		if (processor->current_urgency != old_urgency) {
			uint64_t urgency_param1, urgency_param2;

			thread_urgency_t new_urgency = thread_get_urgency(thread,
			    &urgency_param1, &urgency_param2);

			thread_tell_urgency(new_urgency, urgency_param1,
			    urgency_param2, 0, thread);
		}

		                                                           
		uint64_t ctime = mach_approximate_time();
		machine_thread_going_on_core(thread, processor->current_urgency, 0, 0, ctime);
	} else if (thread->state & TH_RUN) {
		processor_t processor = thread->last_processor;

		if (!lazy_update &&
		    processor != PROCESSOR_NULL &&
		    processor != current_processor() &&
		    processor->active_thread == thread) {
			cause_ast_check(processor);
		}
	}
}

  
                                      
  
                                                                   
                                       
  
                     
  
                                                                                  
                                                            
   
thread_t
thread_run_queue_remove_for_handoff(thread_t thread)
{
	thread_t pulled_thread = THREAD_NULL;

	thread_lock(thread);

	  
                                                                
                                                         
                                                              
                                                         
   
                                                                            
    
	processor_t processor = current_processor();
	if ((thread->bound_processor == PROCESSOR_NULL || thread->bound_processor == processor)
	    && (!thread_no_smt(thread))
	    && (processor->current_pri < BASEPRI_RTQUEUES)
	    && (thread->sched_pri < BASEPRI_RTQUEUES)
#if __AMP__
	    && ((!(thread->sched_flags & TH_SFLAG_PCORE_ONLY)) ||
	    processor->processor_set->pset_cluster_type == PSET_AMP_P)
	    && ((!(thread->sched_flags & TH_SFLAG_ECORE_ONLY)) ||
	    processor->processor_set->pset_cluster_type == PSET_AMP_E)
#endif              
	    ) {
		if (thread_run_queue_remove(thread)) {
			pulled_thread = thread;
		}
	}

	thread_unlock(thread);

	return pulled_thread;
}

  
                             
  
                                     
                                                                            
                              
  
                                                                                 
                       
  
                     
  
                                                                                  
                                                            
   
thread_t
thread_prepare_for_handoff(thread_t thread, thread_handoff_option_t option)
{
	thread_t pulled_thread = THREAD_NULL;

	if (option & THREAD_HANDOFF_SETRUN_NEEDED) {
		processor_t processor = current_processor();
		thread_lock(thread);

		  
                                                                 
                                                             
                                                               
                                                                
                       
     
		if ((thread->bound_processor == PROCESSOR_NULL || thread->bound_processor == processor)
		    && (!thread_no_smt(thread))
#if __AMP__
		    && ((!(thread->sched_flags & TH_SFLAG_PCORE_ONLY)) ||
		    processor->processor_set->pset_cluster_type == PSET_AMP_P)
		    && ((!(thread->sched_flags & TH_SFLAG_ECORE_ONLY)) ||
		    processor->processor_set->pset_cluster_type == PSET_AMP_E)
#endif              
		    ) {
			pulled_thread = thread;
		} else {
			thread_setrun(thread, SCHED_PREEMPT | SCHED_TAILQ);
		}
		thread_unlock(thread);
	} else {
		pulled_thread = thread_run_queue_remove_for_handoff(thread);
	}

	return pulled_thread;
}

  
                           
  
                                                 
                             
  
                         
  
                                                                      
                                                              
                                                                 
                                                                 
                                           
  
                                                                    
                                                                
                                      
   
boolean_t
thread_run_queue_remove(
	thread_t        thread)
{
	boolean_t removed = FALSE;
	processor_t processor = thread->runq;

	if ((thread->state & (TH_RUN | TH_WAIT)) == TH_WAIT) {
		                           
		assert(thread->runq == PROCESSOR_NULL);
		return FALSE;
	}

	if (processor == PROCESSOR_NULL) {
		  
                                          
                                                       
    
                                                            
                                                                      
                                                                    
             
     

		return FALSE;
	}

	if (thread->sched_pri < BASEPRI_RTQUEUES) {
		return SCHED(processor_queue_remove)(processor, thread);
	}

	processor_set_t pset = processor->processor_set;

	pset_lock(pset);

	if (thread->runq != PROCESSOR_NULL) {
		  
                                                        
                    
     

		remqueue(&thread->runq_links);
		SCHED_STATS_RUNQ_CHANGE(&SCHED(rt_runq)(pset)->runq_stats, rt_runq_count(pset));
		rt_runq_count_decr(pset);

		thread->runq = PROCESSOR_NULL;

		removed = TRUE;
	}

	pset_unlock(pset);

	return removed;
}

  
                                                                    
  
                                                                
  
                             
   
void
thread_run_queue_reinsert(thread_t thread, sched_options_t options)
{
	assert(thread->runq == PROCESSOR_NULL);
	assert(thread->state & (TH_RUN));

	thread_setrun(thread, options);
}

void
sys_override_cpu_throttle(boolean_t enable_override)
{
	if (enable_override) {
		cpu_throttle_enabled = 0;
	} else {
		cpu_throttle_enabled = 1;
	}
}

thread_urgency_t
thread_get_urgency(thread_t thread, uint64_t *arg1, uint64_t *arg2)
{
	uint64_t urgency_param1 = 0, urgency_param2 = 0;

	thread_urgency_t urgency;

	if (thread == NULL || (thread->state & TH_IDLE)) {
		urgency_param1 = 0;
		urgency_param2 = 0;

		urgency = THREAD_URGENCY_NONE;
	} else if (thread->sched_mode == TH_MODE_REALTIME) {
		urgency_param1 = thread->realtime.period;
		urgency_param2 = thread->realtime.deadline;

		urgency = THREAD_URGENCY_REAL_TIME;
	} else if (cpu_throttle_enabled &&
	    (thread->sched_pri <= MAXPRI_THROTTLE) &&
	    (thread->base_pri <= MAXPRI_THROTTLE)) {
		  
                                                         
                                                      
                                                     
                                                       
                                                            
                                                            
     
		boolean_t thread_lacks_qos = (proc_get_effective_thread_policy(thread, TASK_POLICY_QOS) == THREAD_QOS_UNSPECIFIED);                                 
		boolean_t task_is_suppressed = (proc_get_effective_task_policy(thread->task, TASK_POLICY_SUP_ACTIVE) == 0x1);

		  
                                                       
                                                        
                                   
     
		urgency_param1 = thread->sched_pri;
		urgency_param2 = thread->base_pri;

		if (thread_lacks_qos && !task_is_suppressed) {
			urgency = THREAD_URGENCY_LOWPRI;
		} else {
			urgency = THREAD_URGENCY_BACKGROUND;
		}
	} else {
		                                                                          
		urgency_param1 = proc_get_effective_thread_policy(thread, TASK_POLICY_THROUGH_QOS);
		urgency_param2 = proc_get_effective_task_policy(thread->task, TASK_POLICY_THROUGH_QOS);
		urgency = THREAD_URGENCY_NORMAL;
	}

	if (arg1 != NULL) {
		*arg1 = urgency_param1;
	}
	if (arg2 != NULL) {
		*arg2 = urgency_param2;
	}

	return urgency;
}

perfcontrol_class_t
thread_get_perfcontrol_class(thread_t thread)
{
	                           
	if (thread->state & TH_IDLE) {
		return PERFCONTROL_CLASS_IDLE;
	}
	if (thread->task == kernel_task) {
		return PERFCONTROL_CLASS_KERNEL;
	}
	if (thread->sched_mode == TH_MODE_REALTIME) {
		return PERFCONTROL_CLASS_REALTIME;
	}

	                                         
	if (thread->base_pri <= MAXPRI_THROTTLE) {
		return PERFCONTROL_CLASS_BACKGROUND;
	} else if (thread->base_pri <= BASEPRI_UTILITY) {
		return PERFCONTROL_CLASS_UTILITY;
	} else if (thread->base_pri <= BASEPRI_DEFAULT) {
		return PERFCONTROL_CLASS_NONUI;
	} else if (thread->base_pri <= BASEPRI_FOREGROUND) {
		return PERFCONTROL_CLASS_UI;
	} else {
		return PERFCONTROL_CLASS_ABOVEUI;
	}
}

  
                                                                      
                                                                      
                                                         
  
                                                               
   

#if 0
#define IDLE_KERNEL_DEBUG_CONSTANT(...) KERNEL_DEBUG_CONSTANT(__VA_ARGS__)
#else
#define IDLE_KERNEL_DEBUG_CONSTANT(...) do { } while(0)
#endif

thread_t
processor_idle(
	thread_t                        thread,
	processor_t                     processor)
{
	processor_set_t         pset = processor->processor_set;

	(void)splsched();

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_IDLE) | DBG_FUNC_START,
	    (uintptr_t)thread_tid(thread), 0, 0, 0, 0);

	SCHED_STATS_INC(idle_transitions);
	assert(processor->running_timers_active == false);

	uint64_t ctime = mach_absolute_time();

	timer_switch(&processor->system_state, ctime, &processor->idle_state);
	processor->current_state = &processor->idle_state;

	cpu_quiescent_counter_leave(ctime);

	while (1) {
		  
                                                        
                                                             
                                                            
                             
     
		atomic_thread_fence(memory_order_acquire);

		if (processor->state != PROCESSOR_IDLE) {
			break;
		}
		if (bit_test(pset->pending_AST_URGENT_cpu_mask, processor->cpu_id)) {
			break;
		}
#if defined(CONFIG_SCHED_DEFERRED_AST)
		if (bit_test(pset->pending_deferred_AST_cpu_mask, processor->cpu_id)) {
			break;
		}
#endif
		if (processor->is_recommended && (processor->processor_primary == processor)) {
			if (rt_runq_count(pset)) {
				break;
			}
		} else {
			if (SCHED(processor_bound_count)(processor)) {
				break;
			}
		}

		IDLE_KERNEL_DEBUG_CONSTANT(
			MACHDBG_CODE(DBG_MACH_SCHED, MACH_IDLE) | DBG_FUNC_NONE, (uintptr_t)thread_tid(thread), rt_runq_count(pset), SCHED(processor_runq_count)(processor), -1, 0);

		machine_track_platform_idle(TRUE);

		machine_idle();
		                                     

		machine_track_platform_idle(FALSE);

		(void)splsched();

		  
                                                                         
                                                                           
                                                                            
                                                                      
                      
     
		if (processor->state == PROCESSOR_IDLE) {
			sched_timeshare_consider_maintenance(mach_absolute_time());
		}

		IDLE_KERNEL_DEBUG_CONSTANT(
			MACHDBG_CODE(DBG_MACH_SCHED, MACH_IDLE) | DBG_FUNC_NONE, (uintptr_t)thread_tid(thread), rt_runq_count(pset), SCHED(processor_runq_count)(processor), -2, 0);

		if (!SCHED(processor_queue_empty)(processor)) {
			                                                       
                                                                
      
			if (processor->processor_primary == processor) {
				break;
			}
		}
	}

	ctime = mach_absolute_time();

	timer_switch(&processor->idle_state, ctime, &processor->system_state);
	processor->current_state = &processor->system_state;

	cpu_quiescent_counter_join(ctime);

	ast_t reason = AST_NONE;

	                                         
	ast_off(AST_SCHEDULING);

	  
                                                                      
                                               
    
	thread_t current_thread = current_thread();

	thread_lock(current_thread);
	thread_t new_thread = thread_select(current_thread, processor, &reason);
	thread_unlock(current_thread);

	assert(processor->running_timers_active == false);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_IDLE) | DBG_FUNC_END,
	    (uintptr_t)thread_tid(thread), processor->state, (uintptr_t)thread_tid(new_thread), reason, 0);

	return new_thread;
}

  
                                              
                                                   
                    
  
                                                         
   
void
idle_thread(__assert_only void* parameter,
    __unused wait_result_t result)
{
	assert(ml_get_interrupts_enabled() == FALSE);
	assert(parameter == NULL);

	processor_t processor = current_processor();

	  
                                                         
                               
    
	disable_preemption();

	  
                                                                  
                                    
    
	spllo();

	thread_t new_thread = processor_idle(THREAD_NULL, processor);
	                                      

	enable_preemption();

	if (new_thread != THREAD_NULL) {
		thread_run(processor->idle_thread,
		    idle_thread, NULL, new_thread);
		              
	}

	thread_block(idle_thread);
	              
}

kern_return_t
idle_thread_create(
	processor_t             processor)
{
	kern_return_t   result;
	thread_t                thread;
	spl_t                   s;
	char                    name[MAXTHREADNAMESIZE];

	result = kernel_thread_create(idle_thread, NULL, MAXPRI_KERNEL, &thread);
	if (result != KERN_SUCCESS) {
		return result;
	}

	snprintf(name, sizeof(name), "idle #%d", processor->cpu_id);
	thread_set_thread_name(thread, name);

	s = splsched();
	thread_lock(thread);
	thread->bound_processor = processor;
	processor->idle_thread = thread;
	thread->sched_pri = thread->base_pri = IDLEPRI;
	thread->state = (TH_RUN | TH_IDLE);
	thread->options |= TH_OPT_IDLE_THREAD;
	thread_unlock(thread);
	splx(s);

	thread_deallocate(thread);

	return KERN_SUCCESS;
}

  
                 
  
                                
  
                      
   
void
sched_startup(void)
{
	kern_return_t   result;
	thread_t                thread;

	simple_lock_init(&sched_vm_group_list_lock, 0);

#if __arm__ || __arm64__
	simple_lock_init(&sched_recommended_cores_lock, 0);
#endif                           

	result = kernel_thread_start_priority((thread_continue_t)sched_init_thread,
	    NULL, MAXPRI_KERNEL, &thread);
	if (result != KERN_SUCCESS) {
		panic("sched_startup");
	}

	thread_deallocate(thread);

	assert_thread_magic(thread);

	  
                                           
                                                  
            
   
                                               
                         
    
	thread_block(THREAD_CONTINUE_NULL);
}

#if __arm64__
static _Atomic uint64_t sched_perfcontrol_callback_deadline;
#endif                


#if defined(CONFIG_SCHED_TIMESHARE_CORE)

static volatile uint64_t                sched_maintenance_deadline;
static uint64_t                         sched_tick_last_abstime;
static uint64_t                         sched_tick_delta;
uint64_t                                sched_tick_max_delta;


  
                     
  
                                                   
                    
   
void
sched_timeshare_maintenance_continue(void)
{
	uint64_t        sched_tick_ctime, late_time;

	struct sched_update_scan_context scan_context = {
		.earliest_bg_make_runnable_time = UINT64_MAX,
		.earliest_normal_make_runnable_time = UINT64_MAX,
		.earliest_rt_make_runnable_time = UINT64_MAX
	};

	sched_tick_ctime = mach_absolute_time();

	if (__improbable(sched_tick_last_abstime == 0)) {
		sched_tick_last_abstime = sched_tick_ctime;
		late_time = 0;
		sched_tick_delta = 1;
	} else {
		late_time = sched_tick_ctime - sched_tick_last_abstime;
		sched_tick_delta = late_time / sched_tick_interval;
		                                                            
                                                         
               
     
		sched_tick_delta = MAX(sched_tick_delta, 1);

		                                               
                                                    
                                                 
                                                         
                
     
		sched_tick_delta = MIN(sched_tick_delta, SCHED_TICK_MAX_DELTA);

		sched_tick_last_abstime = sched_tick_ctime;
		sched_tick_max_delta = MAX(sched_tick_delta, sched_tick_max_delta);
	}

	scan_context.sched_tick_last_abstime = sched_tick_last_abstime;
	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_MAINTENANCE) | DBG_FUNC_START,
	    sched_tick_delta, late_time, 0, 0, 0);

	                                                                     
                                                               
                                                                   
    

	sched_tick += sched_tick_delta;

	update_vm_info();

	  
                              
    
	compute_averages(sched_tick_delta);

	  
                                          
                                                                                  
                           
    
	SCHED(thread_update_scan)(&scan_context);

	SCHED(rt_runq_scan)(&scan_context);

	uint64_t ctime = mach_absolute_time();

	uint64_t bg_max_latency       = (ctime > scan_context.earliest_bg_make_runnable_time) ?
	    ctime - scan_context.earliest_bg_make_runnable_time : 0;

	uint64_t default_max_latency  = (ctime > scan_context.earliest_normal_make_runnable_time) ?
	    ctime - scan_context.earliest_normal_make_runnable_time : 0;

	uint64_t realtime_max_latency = (ctime > scan_context.earliest_rt_make_runnable_time) ?
	    ctime - scan_context.earliest_rt_make_runnable_time : 0;

	machine_max_runnable_latency(bg_max_latency, default_max_latency, realtime_max_latency);

	  
                                                               
    
	sched_vm_group_maintenance();

#if __arm__ || __arm64__
	                                                              
	sched_recommended_cores_maintenance();
#endif                           


#if DEBUG || DEVELOPMENT
#if __x86_64__
#include <i386/misc_protos.h>
	                                        
	mp_interrupt_watchdog();
#endif                 
#endif                           

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_MAINTENANCE) | DBG_FUNC_END,
	    sched_pri_shifts[TH_BUCKET_SHARE_FG], sched_pri_shifts[TH_BUCKET_SHARE_BG],
	    sched_pri_shifts[TH_BUCKET_SHARE_UT], sched_pri_shifts[TH_BUCKET_SHARE_DF], 0);

	assert_wait((event_t)sched_timeshare_maintenance_continue, THREAD_UNINT);
	thread_block((thread_continue_t)sched_timeshare_maintenance_continue);
	              
}

static uint64_t sched_maintenance_wakeups;

  
                                                                          
                                                                         
                                                                            
                                                                  
                                                                          
                                                                         
                                                                     
                                                                     
   
void
sched_timeshare_consider_maintenance(uint64_t ctime)
{
	cpu_quiescent_counter_checkin(ctime);

	uint64_t deadline = sched_maintenance_deadline;

	if (__improbable(ctime >= deadline)) {
		if (__improbable(current_thread() == sched_maintenance_thread)) {
			return;
		}
		OSMemoryBarrier();

		uint64_t ndeadline = ctime + sched_tick_interval;

		if (__probable(os_atomic_cmpxchg(&sched_maintenance_deadline, deadline, ndeadline, seq_cst))) {
			thread_wakeup((event_t)sched_timeshare_maintenance_continue);
			sched_maintenance_wakeups++;
		}
	}

#if !CONFIG_SCHED_CLUTCH
	  
                                                                                         
                                                                           
    
	uint64_t load_compute_deadline = os_atomic_load_wide(&sched_load_compute_deadline, relaxed);

	if (__improbable(load_compute_deadline && ctime >= load_compute_deadline)) {
		uint64_t new_deadline = 0;
		if (os_atomic_cmpxchg(&sched_load_compute_deadline, load_compute_deadline, new_deadline, relaxed)) {
			compute_sched_load();
			new_deadline = ctime + sched_load_compute_interval_abs;
			os_atomic_store_wide(&sched_load_compute_deadline, new_deadline, relaxed);
		}
	}
#endif                          

#if __arm64__
	uint64_t perf_deadline = os_atomic_load(&sched_perfcontrol_callback_deadline, relaxed);

	if (__improbable(perf_deadline && ctime >= perf_deadline)) {
		                                                                                             
		if (os_atomic_cmpxchg(&sched_perfcontrol_callback_deadline, perf_deadline, 0, relaxed)) {
			machine_perfcontrol_deadline_passed(perf_deadline);
		}
	}
#endif                
}

#endif                                  

void
sched_init_thread(void)
{
	thread_block(THREAD_CONTINUE_NULL);

	thread_t thread = current_thread();

	thread_set_thread_name(thread, "sched_maintenance_thread");

	sched_maintenance_thread = thread;

	SCHED(maintenance_continuation)();

	              
}

#if defined(CONFIG_SCHED_TIMESHARE_CORE)

  
                                  
  
                                                         
                            
  
                                                         
                                                      
  
                                                        
                                                               
  
                                                          
                                         
   

#define THREAD_UPDATE_SIZE              128

static thread_t thread_update_array[THREAD_UPDATE_SIZE];
static uint32_t thread_update_count = 0;

                                                                            
boolean_t
thread_update_add_thread(thread_t thread)
{
	if (thread_update_count == THREAD_UPDATE_SIZE) {
		return FALSE;
	}

	thread_update_array[thread_update_count++] = thread;
	thread_reference_internal(thread);
	return TRUE;
}

void
thread_update_process_threads(void)
{
	assert(thread_update_count <= THREAD_UPDATE_SIZE);

	for (uint32_t i = 0; i < thread_update_count; i++) {
		thread_t thread = thread_update_array[i];
		assert_thread_magic(thread);
		thread_update_array[i] = THREAD_NULL;

		spl_t s = splsched();
		thread_lock(thread);
		if (!(thread->state & (TH_WAIT)) && thread->sched_stamp != sched_tick) {
			SCHED(update_priority)(thread);
		}
		thread_unlock(thread);
		splx(s);

		thread_deallocate(thread);
	}

	thread_update_count = 0;
}

static boolean_t
runq_scan_thread(
	thread_t thread,
	sched_update_scan_context_t scan_context)
{
	assert_thread_magic(thread);

	if (thread->sched_stamp != sched_tick &&
	    thread->sched_mode == TH_MODE_TIMESHARE) {
		if (thread_update_add_thread(thread) == FALSE) {
			return TRUE;
		}
	}

	if (cpu_throttle_enabled && ((thread->sched_pri <= MAXPRI_THROTTLE) && (thread->base_pri <= MAXPRI_THROTTLE))) {
		if (thread->last_made_runnable_time < scan_context->earliest_bg_make_runnable_time) {
			scan_context->earliest_bg_make_runnable_time = thread->last_made_runnable_time;
		}
	} else {
		if (thread->last_made_runnable_time < scan_context->earliest_normal_make_runnable_time) {
			scan_context->earliest_normal_make_runnable_time = thread->last_made_runnable_time;
		}
	}

	return FALSE;
}

  
                                     
  
                                   
   
boolean_t
runq_scan(
	run_queue_t                   runq,
	sched_update_scan_context_t   scan_context)
{
	int count       = runq->count;
	int queue_index;

	assert(count >= 0);

	if (count == 0) {
		return FALSE;
	}

	for (queue_index = bitmap_first(runq->bitmap, NRQS);
	    queue_index >= 0;
	    queue_index = bitmap_next(runq->bitmap, queue_index)) {
		thread_t thread;
		circle_queue_t queue = &runq->queues[queue_index];

		cqe_foreach_element(thread, queue, runq_links) {
			assert(count > 0);
			if (runq_scan_thread(thread, scan_context) == TRUE) {
				return TRUE;
			}
			count--;
		}
	}

	return FALSE;
}

#if CONFIG_SCHED_CLUTCH

boolean_t
sched_clutch_timeshare_scan(
	queue_t thread_queue,
	uint16_t thread_count,
	sched_update_scan_context_t scan_context)
{
	if (thread_count == 0) {
		return FALSE;
	}

	thread_t thread;
	qe_foreach_element_safe(thread, thread_queue, th_clutch_timeshare_link) {
		if (runq_scan_thread(thread, scan_context) == TRUE) {
			return TRUE;
		}
		thread_count--;
	}

	assert(thread_count == 0);
	return FALSE;
}


#endif                          

#endif                                  

bool
thread_is_eager_preempt(thread_t thread)
{
	return thread->sched_flags & TH_SFLAG_EAGERPREEMPT;
}

void
thread_set_eager_preempt(thread_t thread)
{
	spl_t s = splsched();
	thread_lock(thread);

	assert(!thread_is_eager_preempt(thread));

	thread->sched_flags |= TH_SFLAG_EAGERPREEMPT;

	if (thread == current_thread()) {
		                                                                
		ast_t ast = csw_check(thread, current_processor(), AST_NONE);

		thread_unlock(thread);

		if (ast != AST_NONE) {
			thread_block_reason(THREAD_CONTINUE_NULL, NULL, ast);
		}
	} else {
		processor_t last_processor = thread->last_processor;

		if (last_processor != PROCESSOR_NULL &&
		    last_processor->state == PROCESSOR_RUNNING &&
		    last_processor->active_thread == thread) {
			cause_ast_check(last_processor);
		}

		thread_unlock(thread);
	}

	splx(s);
}

void
thread_clear_eager_preempt(thread_t thread)
{
	spl_t s = splsched();
	thread_lock(thread);

	assert(thread_is_eager_preempt(thread));

	thread->sched_flags &= ~TH_SFLAG_EAGERPREEMPT;

	if (thread == current_thread()) {
		current_processor()->current_is_eagerpreempt = false;
	}

	thread_unlock(thread);
	splx(s);
}

  
                        
   
void
sched_stats_handle_csw(processor_t processor, int reasons, int selfpri, int otherpri)
{
	struct sched_statistics *stats;
	boolean_t to_realtime = FALSE;

	stats = PERCPU_GET_RELATIVE(sched_stats, processor, processor);
	stats->csw_count++;

	if (otherpri >= BASEPRI_REALTIME) {
		stats->rt_sched_count++;
		to_realtime = TRUE;
	}

	if ((reasons & AST_PREEMPT) != 0) {
		stats->preempt_count++;

		if (selfpri >= BASEPRI_REALTIME) {
			stats->preempted_rt_count++;
		}

		if (to_realtime) {
			stats->preempted_by_rt_count++;
		}
	}
}

void
sched_stats_handle_runq_change(struct runq_stats *stats, int old_count)
{
	uint64_t timestamp = mach_absolute_time();

	stats->count_sum += (timestamp - stats->last_change_timestamp) * old_count;
	stats->last_change_timestamp = timestamp;
}

  
                                   
   
#undef thread_wakeup
void
thread_wakeup(
	event_t         x);

void
thread_wakeup(
	event_t         x)
{
	thread_wakeup_with_result(x, THREAD_AWAKENED);
}

boolean_t
preemption_enabled(void)
{
	return get_preemption_level() == 0 && ml_get_interrupts_enabled();
}

static void
sched_timer_deadline_tracking_init(void)
{
	nanoseconds_to_absolutetime(TIMER_DEADLINE_TRACKING_BIN_1_DEFAULT, &timer_deadline_tracking_bin_1);
	nanoseconds_to_absolutetime(TIMER_DEADLINE_TRACKING_BIN_2_DEFAULT, &timer_deadline_tracking_bin_2);
}

#if __arm__ || __arm64__

uint32_t    perfcontrol_requested_recommended_cores = ALL_CORES_RECOMMENDED;
uint32_t    perfcontrol_requested_recommended_core_count = MAX_CPUS;
bool        perfcontrol_failsafe_active = false;
bool        perfcontrol_sleep_override = false;

uint64_t    perfcontrol_failsafe_maintenance_runnable_time;
uint64_t    perfcontrol_failsafe_activation_time;
uint64_t    perfcontrol_failsafe_deactivation_time;

                                                              
#define FAILSAFE_NAME_LEN       33                                          
char        perfcontrol_failsafe_name[FAILSAFE_NAME_LEN];
int         perfcontrol_failsafe_pid;
uint64_t    perfcontrol_failsafe_tid;
uint64_t    perfcontrol_failsafe_thread_timer_at_start;
uint64_t    perfcontrol_failsafe_thread_timer_last_seen;
uint32_t    perfcontrol_failsafe_recommended_at_trigger;

  
                                                                     
                                                                       
                                                                               
  
                                                               
  
                                                                                       
                         
  
                                                         
   
void
sched_perfcontrol_update_recommended_cores(uint32_t recommended_cores)
{
	assert(preemption_enabled());

	spl_t s = splsched();
	simple_lock(&sched_recommended_cores_lock, LCK_GRP_NULL);

	perfcontrol_requested_recommended_cores = recommended_cores;
	perfcontrol_requested_recommended_core_count = __builtin_popcountll(recommended_cores);

	if ((perfcontrol_failsafe_active == false) && (perfcontrol_sleep_override == false)) {
		sched_update_recommended_cores(perfcontrol_requested_recommended_cores & usercontrol_requested_recommended_cores);
	} else {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    MACHDBG_CODE(DBG_MACH_SCHED, MACH_REC_CORES_FAILSAFE) | DBG_FUNC_NONE,
		    perfcontrol_requested_recommended_cores,
		    sched_maintenance_thread->last_made_runnable_time, 0, 0, 0);
	}

	simple_unlock(&sched_recommended_cores_lock);
	splx(s);
}

void
sched_override_recommended_cores_for_sleep(void)
{
	spl_t s = splsched();
	simple_lock(&sched_recommended_cores_lock, LCK_GRP_NULL);

	if (perfcontrol_sleep_override == false) {
		perfcontrol_sleep_override = true;
		sched_update_recommended_cores(ALL_CORES_RECOMMENDED);
	}

	simple_unlock(&sched_recommended_cores_lock);
	splx(s);
}

void
sched_restore_recommended_cores_after_sleep(void)
{
	spl_t s = splsched();
	simple_lock(&sched_recommended_cores_lock, LCK_GRP_NULL);

	if (perfcontrol_sleep_override == true) {
		perfcontrol_sleep_override = false;
		sched_update_recommended_cores(perfcontrol_requested_recommended_cores & usercontrol_requested_recommended_cores);
	}

	simple_unlock(&sched_recommended_cores_lock);
	splx(s);
}

  
                                                                      
  
                                                                   
                                                       
   
void
sched_consider_recommended_cores(uint64_t ctime, thread_t cur_thread)
{
	  
                                                     
                                                    
   
                                                                      
                                                               
    

	if (__improbable(perfcontrol_failsafe_active == TRUE)) {
		                                                        

		simple_lock(&sched_recommended_cores_lock, LCK_GRP_NULL);

		if (perfcontrol_failsafe_active == TRUE &&
		    cur_thread->thread_id == perfcontrol_failsafe_tid) {
			perfcontrol_failsafe_thread_timer_last_seen = timer_grab(&cur_thread->user_timer) +
			    timer_grab(&cur_thread->system_timer);
		}

		simple_unlock(&sched_recommended_cores_lock);

		                                                        
		return;
	}

	                                                                       
	if (__probable(perfcontrol_requested_recommended_core_count >= processor_count)) {
		return;
	}

	uint64_t too_long_ago = ctime - perfcontrol_failsafe_starvation_threshold;

	                                                               
	thread_t m_thread = sched_maintenance_thread;

	                                                 
	if (__probable(m_thread->last_made_runnable_time >= too_long_ago)) {
		return;
	}

	                                            
	thread_lock(m_thread);

	if (m_thread->runq == PROCESSOR_NULL ||
	    (m_thread->state & (TH_RUN | TH_WAIT)) != TH_RUN ||
	    m_thread->last_made_runnable_time >= too_long_ago) {
		  
                                                        
                                               
     
		thread_unlock(m_thread);
		return;
	}

	uint64_t maintenance_runnable_time = m_thread->last_made_runnable_time;

	thread_unlock(m_thread);

	  
                                                                     
                                                                  
                                                                        
                                                                             
   
                                                              
                                                                          
   
                                                             
    

	simple_lock(&sched_recommended_cores_lock, LCK_GRP_NULL);

	if (perfcontrol_failsafe_active == TRUE) {
		simple_unlock(&sched_recommended_cores_lock);
		return;
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_REC_CORES_FAILSAFE) | DBG_FUNC_START,
	    perfcontrol_requested_recommended_cores, maintenance_runnable_time, 0, 0, 0);

	perfcontrol_failsafe_active = TRUE;
	perfcontrol_failsafe_activation_time = mach_absolute_time();
	perfcontrol_failsafe_maintenance_runnable_time = maintenance_runnable_time;
	perfcontrol_failsafe_recommended_at_trigger = perfcontrol_requested_recommended_cores;

	                                                                                           
	task_t task = cur_thread->task;
	perfcontrol_failsafe_pid = task_pid(task);
	strlcpy(perfcontrol_failsafe_name, proc_name_address(task->bsd_info), sizeof(perfcontrol_failsafe_name));

	perfcontrol_failsafe_tid = cur_thread->thread_id;

	                                                   
	uint64_t recent_computation = (ctime - cur_thread->computation_epoch) + cur_thread->computation_metered;

	uint64_t last_seen = timer_grab(&cur_thread->user_timer) + timer_grab(&cur_thread->system_timer);

	                                                                                      
	perfcontrol_failsafe_thread_timer_at_start  = last_seen - recent_computation;
	perfcontrol_failsafe_thread_timer_last_seen = last_seen;

	                                                          
	sched_update_recommended_cores(ALL_CORES_RECOMMENDED);

	simple_unlock(&sched_recommended_cores_lock);
}

  
                                                                                     
  
                                                               
   
static void
sched_recommended_cores_maintenance(void)
{
	                                                        
	if (__probable(perfcontrol_failsafe_active == FALSE)) {
		return;
	}

	uint64_t ctime = mach_absolute_time();

	boolean_t print_diagnostic = FALSE;
	char p_name[FAILSAFE_NAME_LEN] = "";

	spl_t s = splsched();
	simple_lock(&sched_recommended_cores_lock, LCK_GRP_NULL);

	                                                 
	if (perfcontrol_failsafe_active == FALSE) {
		goto out;
	}

	  
                                                                         
                                                                         
                                                         
    
	if ((ctime - perfcontrol_failsafe_activation_time) < perfcontrol_failsafe_starvation_threshold) {
		goto out;
	}

	                                                                               

	int      pid = perfcontrol_failsafe_pid;
	uint64_t tid = perfcontrol_failsafe_tid;

	uint64_t thread_usage       = perfcontrol_failsafe_thread_timer_last_seen -
	    perfcontrol_failsafe_thread_timer_at_start;
	uint32_t rec_cores_before   = perfcontrol_failsafe_recommended_at_trigger;
	uint32_t rec_cores_after    = perfcontrol_requested_recommended_cores;
	uint64_t failsafe_duration  = ctime - perfcontrol_failsafe_activation_time;
	strlcpy(p_name, perfcontrol_failsafe_name, sizeof(p_name));

	print_diagnostic = TRUE;

	                                                                                 

	perfcontrol_failsafe_deactivation_time = ctime;
	perfcontrol_failsafe_active = FALSE;

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_REC_CORES_FAILSAFE) | DBG_FUNC_END,
	    perfcontrol_requested_recommended_cores, failsafe_duration, 0, 0, 0);

	sched_update_recommended_cores(perfcontrol_requested_recommended_cores & usercontrol_requested_recommended_cores);

out:
	simple_unlock(&sched_recommended_cores_lock);
	splx(s);

	if (print_diagnostic) {
		uint64_t failsafe_duration_ms = 0, thread_usage_ms = 0;

		absolutetime_to_nanoseconds(failsafe_duration, &failsafe_duration_ms);
		failsafe_duration_ms = failsafe_duration_ms / NSEC_PER_MSEC;

		absolutetime_to_nanoseconds(thread_usage, &thread_usage_ms);
		thread_usage_ms = thread_usage_ms / NSEC_PER_MSEC;

		printf("recommended core failsafe kicked in for %lld ms "
		    "likely due to %s[%d] thread 0x%llx spending "
		    "%lld ms on cpu at realtime priority - "
		    "new recommendation: 0x%x -> 0x%x\n",
		    failsafe_duration_ms, p_name, pid, tid, thread_usage_ms,
		    rec_cores_before, rec_cores_after);
	}
}

#endif                           

kern_return_t
sched_processor_enable(processor_t processor, boolean_t enable)
{
	assert(preemption_enabled());

	spl_t s = splsched();
	simple_lock(&sched_recommended_cores_lock, LCK_GRP_NULL);

	if (enable) {
		bit_set(usercontrol_requested_recommended_cores, processor->cpu_id);
	} else {
		bit_clear(usercontrol_requested_recommended_cores, processor->cpu_id);
	}

#if __arm__ || __arm64__
	if ((perfcontrol_failsafe_active == false) && (perfcontrol_sleep_override == false)) {
		sched_update_recommended_cores(perfcontrol_requested_recommended_cores & usercontrol_requested_recommended_cores);
	} else {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    MACHDBG_CODE(DBG_MACH_SCHED, MACH_REC_CORES_FAILSAFE) | DBG_FUNC_NONE,
		    perfcontrol_requested_recommended_cores,
		    sched_maintenance_thread->last_made_runnable_time, 0, 0, 0);
	}
#else                           
	sched_update_recommended_cores(usercontrol_requested_recommended_cores);
#endif                            

	simple_unlock(&sched_recommended_cores_lock);
	splx(s);

	return KERN_SUCCESS;
}


  
                                                                  
                                            
  
                                                                   
                                                             
                                                                      
                                            
  
                                                            
   
static void
sched_update_recommended_cores(uint64_t recommended_cores)
{
	processor_set_t pset, nset;
	processor_t     processor;
	uint64_t        needs_exit_idle_mask = 0x0;
	uint32_t        avail_count;

	processor = processor_list;
	pset = processor->processor_set;

	KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_UPDATE_REC_CORES) | DBG_FUNC_START,
	    recommended_cores,
#if __arm__ || __arm64__
	    perfcontrol_failsafe_active, 0, 0);
#else                           
	    0, 0, 0);
#endif                             

	if (__builtin_popcountll(recommended_cores) == 0) {
		bit_set(recommended_cores, master_processor->cpu_id);                                    
	}

	                                 
	pset_lock(pset);
	avail_count = 0;
	do {
		nset = processor->processor_set;
		if (nset != pset) {
			pset_unlock(pset);
			pset = nset;
			pset_lock(pset);
		}

		if (bit_test(recommended_cores, processor->cpu_id)) {
			processor->is_recommended = TRUE;
			bit_set(pset->recommended_bitmask, processor->cpu_id);

			if (processor->state == PROCESSOR_IDLE) {
				if (processor != current_processor()) {
					bit_set(needs_exit_idle_mask, processor->cpu_id);
				}
			}
			if (processor->state != PROCESSOR_OFF_LINE) {
				avail_count++;
				SCHED(pset_made_schedulable)(processor, pset, false);
			}
		}
	} while ((processor = processor->processor_list) != NULL);
	pset_unlock(pset);

	                                        
	processor = processor_list;
	pset = processor->processor_set;

	pset_lock(pset);
	do {
		nset = processor->processor_set;
		if (nset != pset) {
			pset_unlock(pset);
			pset = nset;
			pset_lock(pset);
		}

		if (!bit_test(recommended_cores, processor->cpu_id)) {
			sched_ipi_type_t ipi_type = SCHED_IPI_NONE;

			processor->is_recommended = FALSE;
			bit_clear(pset->recommended_bitmask, processor->cpu_id);

			if ((processor->state == PROCESSOR_RUNNING) || (processor->state == PROCESSOR_DISPATCHING)) {
				ipi_type = SCHED_IPI_IMMEDIATE;
			}
			SCHED(processor_queue_shutdown)(processor);
			                   

			SCHED(rt_queue_shutdown)(processor);

			if (ipi_type != SCHED_IPI_NONE) {
				if (processor == current_processor()) {
					ast_on(AST_PREEMPT);
				} else {
					sched_ipi_perform(processor, ipi_type);
				}
			}

			pset_lock(pset);
		}
	} while ((processor = processor->processor_list) != NULL);

	processor_avail_count_user = avail_count;
#if defined(__x86_64__)
	commpage_update_active_cpus();
#endif

	pset_unlock(pset);

	                                                                    
	for (int cpuid = lsb_first(needs_exit_idle_mask); cpuid >= 0; cpuid = lsb_next(needs_exit_idle_mask, cpuid)) {
		processor = processor_array[cpuid];
		machine_signal_idle(processor);
	}

	KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_UPDATE_REC_CORES) | DBG_FUNC_END,
	    needs_exit_idle_mask, 0, 0, 0);
}

void
thread_set_options(uint32_t thopt)
{
	spl_t x;
	thread_t t = current_thread();

	x = splsched();
	thread_lock(t);

	t->options |= thopt;

	thread_unlock(t);
	splx(x);
}

void
thread_set_pending_block_hint(thread_t thread, block_hint_t block_hint)
{
	thread->pending_block_hint = block_hint;
}

uint32_t
qos_max_parallelism(int qos, uint64_t options)
{
	return SCHED(qos_max_parallelism)(qos, options);
}

uint32_t
sched_qos_max_parallelism(__unused int qos, uint64_t options)
{
	host_basic_info_data_t hinfo;
	mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
	                                                  
	__assert_only kern_return_t kret = host_info(host_self(), HOST_BASIC_INFO,
	    (host_info_t)&hinfo, &count);
	assert(kret == KERN_SUCCESS);

	if (options & QOS_PARALLELISM_COUNT_LOGICAL) {
		return hinfo.logical_cpu;
	} else {
		return hinfo.physical_cpu;
	}
}

int sched_allow_NO_SMT_threads = 1;
bool
thread_no_smt(thread_t thread)
{
	return sched_allow_NO_SMT_threads && (thread->bound_processor == PROCESSOR_NULL) && ((thread->sched_flags & TH_SFLAG_NO_SMT) || (thread->task->t_flags & TF_NO_SMT));
}

bool
processor_active_thread_no_smt(processor_t processor)
{
	return sched_allow_NO_SMT_threads && !processor->current_is_bound && processor->current_is_NO_SMT;
}

#if __arm64__

  
                                             
  
                                                          
   
boolean_t
sched_perfcontrol_update_callback_deadline(uint64_t new_deadline)
{
	  
                                                                    
                                                     
    

	return os_atomic_xchg(&sched_perfcontrol_callback_deadline, new_deadline,
	           relaxed) != 0;
}

#endif                

#if CONFIG_SCHED_EDGE

#define SCHED_PSET_LOAD_EWMA_TC_NSECS 10000000u

  
                                          
  
                                                                     
                                                                    
                                                                      
                
   

static void
sched_edge_pset_running_higher_bucket(processor_set_t pset, uint32_t *running_higher)
{
	bitmap_t *active_map = &pset->cpu_state_map[PROCESSOR_RUNNING];

	                                 
	for (int cpu = bitmap_first(active_map, MAX_CPUS); cpu >= 0; cpu = bitmap_next(active_map, cpu)) {
		sched_bucket_t cpu_bucket = os_atomic_load(&pset->cpu_running_buckets[cpu], relaxed);
		for (sched_bucket_t bucket = cpu_bucket; bucket < TH_BUCKET_SCHED_MAX; bucket++) {
			running_higher[bucket]++;
		}
	}
}

  
                                   
  
                                                                
                                                       
   
void
sched_update_pset_load_average(processor_set_t pset, uint64_t curtime)
{
	if (pset->online_processor_count == 0) {
		                                                                      
		return;
	}

	  
                               
   
                                                                           
                                   
    

	if (!curtime) {
		curtime = mach_absolute_time();
	}
	uint64_t last_update = os_atomic_load(&pset->pset_load_last_update, relaxed);
	int64_t delta_ticks = curtime - last_update;
	if (delta_ticks < 0) {
		return;
	}

	uint64_t delta_nsecs = 0;
	absolutetime_to_nanoseconds(delta_ticks, &delta_nsecs);

	if (__improbable(delta_nsecs > UINT32_MAX)) {
		delta_nsecs = UINT32_MAX;
	}

	uint32_t running_higher[TH_BUCKET_SCHED_MAX] = {0};
	sched_edge_pset_running_higher_bucket(pset, running_higher);

	for (sched_bucket_t sched_bucket = TH_BUCKET_FIXPRI; sched_bucket < TH_BUCKET_SCHED_MAX; sched_bucket++) {
		uint64_t old_load_average = os_atomic_load(&pset->pset_load_average[sched_bucket], relaxed);
		uint64_t old_load_average_factor = old_load_average * SCHED_PSET_LOAD_EWMA_TC_NSECS;
		uint32_t current_runq_depth = (sched_edge_cluster_cumulative_count(&pset->pset_clutch_root, sched_bucket) +  rt_runq_count(pset) + running_higher[sched_bucket]) / pset->online_processor_count;

		  
                                                                                                        
                                                                                                   
                                                                                        
     
		uint64_t new_load_average_factor = (current_runq_depth * delta_nsecs) << SCHED_PSET_LOAD_EWMA_FRACTION_BITS;

		  
                                                                                                                
                                                                                                               
                                                                                                             
     
		int old_load_shifted = (int)((old_load_average + SCHED_PSET_LOAD_EWMA_ROUND_BIT) >> SCHED_PSET_LOAD_EWMA_FRACTION_BITS);
		boolean_t load_uptick = (old_load_shifted == 0) && (current_runq_depth != 0);
		boolean_t load_downtick = (old_load_shifted != 0) && (current_runq_depth == 0);
		uint64_t load_average;
		if (load_uptick || load_downtick) {
			load_average = (current_runq_depth << SCHED_PSET_LOAD_EWMA_FRACTION_BITS);
		} else {
			                                                                      
			load_average = (old_load_average_factor + new_load_average_factor) / (delta_nsecs + SCHED_PSET_LOAD_EWMA_TC_NSECS);
		}
		os_atomic_store(&pset->pset_load_average[sched_bucket], load_average, relaxed);
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_EDGE_LOAD_AVG) | DBG_FUNC_NONE, pset->pset_cluster_id, (load_average >> SCHED_PSET_LOAD_EWMA_FRACTION_BITS), load_average & SCHED_PSET_LOAD_EWMA_FRACTION_MASK, sched_bucket);
	}
	os_atomic_store(&pset->pset_load_last_update, curtime, relaxed);
}

void
sched_update_pset_avg_execution_time(processor_set_t pset, uint64_t execution_time, uint64_t curtime, sched_bucket_t sched_bucket)
{
	pset_execution_time_t old_execution_time_packed, new_execution_time_packed;
	uint64_t avg_thread_execution_time = 0;

	os_atomic_rmw_loop(&pset->pset_execution_time[sched_bucket].pset_execution_time_packed,
	    old_execution_time_packed.pset_execution_time_packed,
	    new_execution_time_packed.pset_execution_time_packed, relaxed, {
		uint64_t last_update = old_execution_time_packed.pset_execution_time_last_update;
		int64_t delta_ticks = curtime - last_update;
		if (delta_ticks < 0) {
		          
                                                                                      
                                                                                      
                                                                            
             
		        os_atomic_rmw_loop_give_up(return );
		}
		uint64_t delta_nsecs = 0;
		absolutetime_to_nanoseconds(delta_ticks, &delta_nsecs);

		uint64_t nanotime = 0;
		absolutetime_to_nanoseconds(execution_time, &nanotime);
		uint64_t execution_time_us = nanotime / NSEC_PER_USEC;

		uint64_t old_execution_time = (old_execution_time_packed.pset_avg_thread_execution_time * SCHED_PSET_LOAD_EWMA_TC_NSECS);
		uint64_t new_execution_time = (execution_time_us * delta_nsecs);

		avg_thread_execution_time = (old_execution_time + new_execution_time) / (delta_nsecs + SCHED_PSET_LOAD_EWMA_TC_NSECS);
		new_execution_time_packed.pset_avg_thread_execution_time = avg_thread_execution_time;
		new_execution_time_packed.pset_execution_time_last_update = curtime;
	});
	KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_PSET_AVG_EXEC_TIME) | DBG_FUNC_NONE, pset->pset_cluster_id, avg_thread_execution_time, sched_bucket);
}

#else                        

void
sched_update_pset_load_average(processor_set_t pset, __unused uint64_t curtime)
{
	int non_rt_load = pset->pset_runq.count;
	int load = ((bit_count(pset->cpu_state_map[PROCESSOR_RUNNING]) + non_rt_load + rt_runq_count(pset)) << PSET_LOAD_NUMERATOR_SHIFT);
	int new_load_average = ((int)pset->load_average + load) >> 1;

	pset->load_average = new_load_average;
#if (DEVELOPMENT || DEBUG)
#if __AMP__
	if (pset->pset_cluster_type == PSET_AMP_P) {
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_PSET_LOAD_AVERAGE) | DBG_FUNC_NONE, sched_get_pset_load_average(pset, 0), (bit_count(pset->cpu_state_map[PROCESSOR_RUNNING]) + pset->pset_runq.count + rt_runq_count(pset)));
	}
#endif
#endif
}

void
sched_update_pset_avg_execution_time(__unused processor_set_t pset, __unused uint64_t execution_time, __unused uint64_t curtime, __unused sched_bucket_t sched_bucket)
{
}
#endif                        

                    
static bool
processor_is_fast_track_candidate_for_realtime_thread(processor_set_t pset, processor_t processor)
{
	int cpuid = processor->cpu_id;
#if defined(__x86_64__)
	if (sched_avoid_cpu0 && (cpuid == 0)) {
		return false;
	}
#endif

	cpumap_t fasttrack_map = pset_available_cpumap(pset) & ~pset->pending_AST_URGENT_cpu_mask & ~pset->realtime_map;

	return bit_test(fasttrack_map, cpuid);
}

                    
static processor_t
choose_processor_for_realtime_thread(processor_set_t pset, processor_t skip_processor, bool consider_secondaries)
{
#if defined(__x86_64__)
	bool avoid_cpu0 = sched_avoid_cpu0 && bit_test(pset->cpu_bitmask, 0);
#else
	const bool avoid_cpu0 = false;
#endif

	cpumap_t cpu_map = pset_available_cpumap(pset) & ~pset->pending_AST_URGENT_cpu_mask & ~pset->realtime_map;
	if (skip_processor) {
		bit_clear(cpu_map, skip_processor->cpu_id);
	}

	cpumap_t primary_map = cpu_map & pset->primary_map;
	if (avoid_cpu0) {
		primary_map = bit_ror64(primary_map, 1);
	}

	int rotid = lsb_first(primary_map);
	if (rotid >= 0) {
		int cpuid = avoid_cpu0 ? ((rotid + 1) & 63) : rotid;

		processor_t processor = processor_array[cpuid];

		return processor;
	}

	if (!pset->is_SMT || !sched_allow_rt_smt || !consider_secondaries) {
		goto out;
	}

	                                   
	cpumap_t secondary_map = cpu_map & ~pset->primary_map;
	if (avoid_cpu0) {
		                     
		secondary_map = bit_ror64(secondary_map, 2);
	}
	rotid = lsb_first(secondary_map);
	if (rotid >= 0) {
		int cpuid = avoid_cpu0 ?  ((rotid + 2) & 63) : rotid;

		processor_t processor = processor_array[cpuid];

		return processor;
	}

out:
	if (skip_processor) {
		return PROCESSOR_NULL;
	}

	  
                                                                                   
                                                                                         
                                                                                 
                                          
    
	cpumap_t non_realtime_map = pset_available_cpumap(pset) & pset->primary_map & ~pset->realtime_map;
	if (bit_count(non_realtime_map) > rt_runq_count(pset)) {
		cpu_map = non_realtime_map;
		assert(cpu_map != 0);
		int cpuid = bit_first(cpu_map);
		assert(cpuid >= 0);
		return processor_array[cpuid];
	}

	if (!pset->is_SMT || !sched_allow_rt_smt || !consider_secondaries) {
		goto skip_secondaries;
	}

	non_realtime_map = pset_available_cpumap(pset) & ~pset->realtime_map;
	if (bit_count(non_realtime_map) > rt_runq_count(pset)) {
		cpu_map = non_realtime_map;
		assert(cpu_map != 0);
		int cpuid = bit_first(cpu_map);
		assert(cpuid >= 0);
		return processor_array[cpuid];
	}

skip_secondaries:
	return PROCESSOR_NULL;
}

                    
static bool
all_available_primaries_are_running_realtime_threads(processor_set_t pset)
{
	cpumap_t cpu_map = pset_available_cpumap(pset) & pset->primary_map & ~pset->realtime_map;
	return rt_runq_count(pset) > bit_count(cpu_map);
}

#if defined(__x86_64__)
                    
static bool
these_processors_are_running_realtime_threads(processor_set_t pset, uint64_t these_map)
{
	cpumap_t cpu_map = pset_available_cpumap(pset) & these_map & ~pset->realtime_map;
	return rt_runq_count(pset) > bit_count(cpu_map);
}
#endif

static bool
sched_ok_to_run_realtime_thread(processor_set_t pset, processor_t processor)
{
	bool ok_to_run_realtime_thread = true;
#if defined(__x86_64__)
	if (sched_avoid_cpu0 && processor->cpu_id == 0) {
		ok_to_run_realtime_thread = these_processors_are_running_realtime_threads(pset, pset->primary_map & ~0x1);
	} else if (sched_avoid_cpu0 && (processor->cpu_id == 1) && processor->is_SMT) {
		ok_to_run_realtime_thread = sched_allow_rt_smt && these_processors_are_running_realtime_threads(pset, ~0x2);
	} else if (processor->processor_primary != processor) {
		ok_to_run_realtime_thread = (sched_allow_rt_smt && all_available_primaries_are_running_realtime_threads(pset));
	}
#else
	(void)pset;
	(void)processor;
#endif
	return ok_to_run_realtime_thread;
}

void
sched_pset_made_schedulable(__unused processor_t processor, processor_set_t pset, boolean_t drop_lock)
{
	if (drop_lock) {
		pset_unlock(pset);
	}
}

void
thread_set_no_smt(bool set)
{
	if (!system_is_SMT) {
		                                     
		return;
	}

	thread_t thread = current_thread();

	spl_t s = splsched();
	thread_lock(thread);
	if (set) {
		thread->sched_flags |= TH_SFLAG_NO_SMT;
	}
	thread_unlock(thread);
	splx(s);
}

bool
thread_get_no_smt(void)
{
	return current_thread()->sched_flags & TH_SFLAG_NO_SMT;
}

extern void task_set_no_smt(task_t);
void
task_set_no_smt(task_t task)
{
	if (!system_is_SMT) {
		                                     
		return;
	}

	if (task == TASK_NULL) {
		task = current_task();
	}

	task_lock(task);
	task->t_flags |= TF_NO_SMT;
	task_unlock(task);
}

#if DEBUG || DEVELOPMENT
extern void sysctl_task_set_no_smt(char no_smt);
void
sysctl_task_set_no_smt(char no_smt)
{
	if (!system_is_SMT) {
		                                     
		return;
	}

	task_t task = current_task();

	task_lock(task);
	if (no_smt == '1') {
		task->t_flags |= TF_NO_SMT;
	}
	task_unlock(task);
}

extern char sysctl_task_get_no_smt(void);
char
sysctl_task_get_no_smt(void)
{
	task_t task = current_task();

	if (task->t_flags & TF_NO_SMT) {
		return '1';
	}
	return '0';
}
#endif                           


__private_extern__ void
thread_bind_cluster_type(thread_t thread, char cluster_type, bool soft_bound)
{
#if __AMP__
	spl_t s = splsched();
	thread_lock(thread);
	thread->sched_flags &= ~(TH_SFLAG_ECORE_ONLY | TH_SFLAG_PCORE_ONLY | TH_SFLAG_BOUND_SOFT);
	if (soft_bound) {
		thread->sched_flags |= TH_SFLAG_BOUND_SOFT;
	}
	switch (cluster_type) {
	case 'e':
	case 'E':
		thread->sched_flags |= TH_SFLAG_ECORE_ONLY;
		break;
	case 'p':
	case 'P':
		thread->sched_flags |= TH_SFLAG_PCORE_ONLY;
		break;
	default:
		break;
	}
	thread_unlock(thread);
	splx(s);

	if (thread == current_thread()) {
		thread_block(THREAD_CONTINUE_NULL);
	}
#else              
	(void)thread;
	(void)cluster_type;
	(void)soft_bound;
#endif              
}

#if DEVELOPMENT || DEBUG
extern int32_t sysctl_get_bound_cpuid(void);
int32_t
sysctl_get_bound_cpuid(void)
{
	int32_t cpuid = -1;
	thread_t self = current_thread();

	processor_t processor = self->bound_processor;
	if (processor == NULL) {
		cpuid = -1;
	} else {
		cpuid = processor->cpu_id;
	}

	return cpuid;
}

extern kern_return_t sysctl_thread_bind_cpuid(int32_t cpuid);
kern_return_t
sysctl_thread_bind_cpuid(int32_t cpuid)
{
	processor_t processor = PROCESSOR_NULL;

	if (cpuid == -1) {
		goto unbind;
	}

	if (cpuid < 0 || cpuid >= MAX_SCHED_CPUS) {
		return KERN_INVALID_VALUE;
	}

	processor = processor_array[cpuid];
	if (processor == PROCESSOR_NULL) {
		return KERN_INVALID_VALUE;
	}

#if __AMP__

	thread_t thread = current_thread();

	if (thread->sched_flags & (TH_SFLAG_ECORE_ONLY | TH_SFLAG_PCORE_ONLY)) {
		if ((thread->sched_flags & TH_SFLAG_BOUND_SOFT) == 0) {
			                                                           
			return KERN_NOT_SUPPORTED;
		}
	}

#endif              

unbind:
	thread_bind(processor);

	thread_block(THREAD_CONTINUE_NULL);
	return KERN_SUCCESS;
}
#endif                           
