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
  
   
  
                   
                               
             
  
                                   
   

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/machine.h>
#include <kern/host.h>
#include <kern/mach_param.h>
#include <kern/sched.h>
#include <sys/kdebug.h>
#include <kern/spl.h>
#include <kern/thread.h>
#include <kern/processor.h>
#include <kern/ledger.h>
#include <machine/machparam.h>
#include <kern/machine.h>
#include <kern/policy_internal.h>
#include <kern/sched_clutch.h>

#ifdef CONFIG_MACH_APPROXIMATE_TIME
#include <machine/commpage.h>                                                 
#endif

#if MONOTONIC
#include <kern/monotonic.h>
#endif                

  
                         
  
                                                     
  
                      
   

void
thread_quantum_expire(
	timer_call_param_t      p0,
	timer_call_param_t      p1)
{
	processor_t                     processor = p0;
	thread_t                        thread = p1;
	ast_t                           preempt;
	uint64_t                        ctime;

	assert(processor == current_processor());
	assert(thread == current_thread());

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_QUANTUM_EXPIRED) | DBG_FUNC_START, 0, 0, 0, 0, 0);

	SCHED_STATS_INC(quantum_timer_expirations);

	  
                                                                
   
                                                                          
                                                                         
                                                                            
             
   
                                                                       
                                                                        
                                          
    
	ledger_credit(thread->t_ledger, task_ledgers.cpu_time, thread->quantum_remaining);
	ledger_credit(thread->t_threadledger, thread_ledgers.cpu_time, thread->quantum_remaining);
	if (thread->t_bankledger) {
		ledger_credit(thread->t_bankledger, bank_ledgers.cpu_time,
		    (thread->quantum_remaining - thread->t_deduct_bank_ledger_time));
	}
	thread->t_deduct_bank_ledger_time = 0;
	ctime = mach_absolute_time();

#ifdef CONFIG_MACH_APPROXIMATE_TIME
	commpage_update_mach_approximate_time(ctime);
#endif
	sched_update_pset_avg_execution_time(processor->processor_set, thread->quantum_remaining, ctime, thread->th_sched_bucket);

#if MONOTONIC
	mt_sched_update(thread);
#endif                

	thread_lock(thread);

	  
                                                                     
                                                                   
    
	processor->last_dispatch = ctime;
	thread->last_run_time = ctime;

	  
                             
    
	if ((thread->sched_mode == TH_MODE_REALTIME || thread->sched_mode == TH_MODE_FIXED) &&
	    !(thread->sched_flags & TH_SFLAG_PROMOTED) &&
	    !(thread->kern_promotion_schedpri != 0) &&
	    !(thread->sched_flags & TH_SFLAG_PROMOTE_REASON_MASK) &&
	    !(thread->options & TH_OPT_SYSTEM_CRITICAL)) {
		uint64_t new_computation;

		new_computation = ctime - thread->computation_epoch;
		new_computation += thread->computation_metered;
		if (new_computation > max_unsafe_computation) {
			KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_FAILSAFE) | DBG_FUNC_NONE,
			    (uintptr_t)thread->sched_pri, (uintptr_t)thread->sched_mode, 0, 0, 0);

			thread->safe_release = ctime + sched_safe_duration;

			sched_thread_mode_demote(thread, TH_SFLAG_FAILSAFE);
		}
	}

	  
                                                
    
	if (SCHED(can_update_priority)(thread)) {
		SCHED(update_priority)(thread);
	} else {
		SCHED(lightweight_update_priority)(thread);
	}

	if (thread->sched_mode != TH_MODE_REALTIME) {
		SCHED(quantum_expire)(thread);
	}

	  
                                                 
    
	processor->first_timeslice = FALSE;

	thread_quantum_init(thread);

	                                                                
	thread->precise_user_kernel_time = use_precise_user_kernel_time(thread);

	  
                                                                            
                                                          
    
	if (!thread->precise_user_kernel_time) {
		timer_update(processor->current_state, ctime);
		timer_update(processor->thread_timer, ctime);
		timer_update(&thread->runnable_timer, ctime);
	}


	processor->quantum_end = ctime + thread->quantum_remaining;

	  
                        
   
                                                                      
                                                                      
    

	ast_t check_reason = AST_QUANTUM;
	if (thread->task == kernel_task) {
		check_reason |= AST_URGENT;
	}

	if ((preempt = csw_check(thread, processor, check_reason)) != AST_NONE) {
		ast_on(preempt);
	}

	  
                                                         
                                                                      
                                                     
    
	ast_propagate(thread);

	thread_unlock(thread);
	running_timer_enter(processor, RUNNING_TIMER_QUANTUM, thread,
	    processor->quantum_end, ctime);

	                                                               
	thread_urgency_t urgency = thread_get_urgency(thread, NULL, NULL);
	machine_thread_going_on_core(thread, urgency, 0, 0, ctime);
	machine_switch_perfcontrol_state_update(QUANTUM_EXPIRY, ctime,
	    0, thread);

#if defined(CONFIG_SCHED_TIMESHARE_CORE)
	sched_timeshare_consider_maintenance(ctime);
#endif                                  

#if __arm__ || __arm64__
	if (thread->sched_mode == TH_MODE_REALTIME) {
		sched_consider_recommended_cores(ctime, thread);
	}
#endif                           

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_QUANTUM_EXPIRED) | DBG_FUNC_END, preempt, 0, 0, 0, 0);
}

  
                                  
  
                                      
                                    
  
                                            
  
                                 
   
void
sched_set_thread_base_priority(thread_t thread, int priority)
{
	assert(priority >= MINPRI);
	uint64_t ctime = 0;

	if (thread->sched_mode == TH_MODE_REALTIME) {
		assert(priority <= BASEPRI_RTQUEUES);
	} else {
		assert(priority < BASEPRI_RTQUEUES);
	}

	int old_base_pri = thread->base_pri;
	thread->req_base_pri = (int16_t)priority;
	if (thread->sched_flags & TH_SFLAG_BASE_PRI_FROZEN) {
		priority = MAX(priority, old_base_pri);
	}
	thread->base_pri = (int16_t)priority;

	if ((thread->state & TH_RUN) == TH_RUN) {
		assert(thread->last_made_runnable_time != THREAD_NOT_RUNNABLE);
		ctime = mach_approximate_time();
		thread->last_basepri_change_time = ctime;
	} else {
		assert(thread->last_basepri_change_time == THREAD_NOT_RUNNABLE);
		assert(thread->last_made_runnable_time == THREAD_NOT_RUNNABLE);
	}

	  
                                                                 
                                                               
                        
    
	if (thread == current_thread() && old_base_pri != priority) {
		if (!ctime) {
			ctime = mach_approximate_time();
		}
		machine_switch_perfcontrol_state_update(PERFCONTROL_ATTR_UPDATE,
		    ctime, PERFCONTROL_CALLOUT_WAKE_UNSAFE, thread);
	}
#if !CONFIG_SCHED_CLUTCH
	                                                                         
	SCHED(update_thread_bucket)(thread);
#endif                           

	thread_recompute_sched_pri(thread, SETPRI_DEFAULT);
}

  
                                    
  
                                               
                                    
  
                                   
   
void
sched_set_kernel_thread_priority(thread_t thread, int new_priority)
{
	spl_t s = splsched();

	thread_lock(thread);

	assert(thread->sched_mode != TH_MODE_REALTIME);
	assert(thread->effective_policy.thep_qos == THREAD_QOS_UNSPECIFIED);

	if (new_priority > thread->max_priority) {
		new_priority = thread->max_priority;
	}
#if !defined(XNU_TARGET_OS_OSX)
	if (new_priority < MAXPRI_THROTTLE) {
		new_priority = MAXPRI_THROTTLE;
	}
#endif                                  

	thread->importance = new_priority - thread->task_priority;

	sched_set_thread_base_priority(thread, new_priority);

	thread_unlock(thread);
	splx(s);
}

  
                              
  
                                             
                                        
                                             
  
                                                                
                                                             
                             
  
                                             
  
                                                                                         
   
void
thread_recompute_sched_pri(thread_t thread, set_sched_pri_options_t options)
{
	uint32_t     sched_flags = thread->sched_flags;
	sched_mode_t sched_mode  = thread->sched_mode;

	int16_t priority = thread->base_pri;

	if (sched_mode == TH_MODE_TIMESHARE) {
		priority = (int16_t)SCHED(compute_timeshare_priority)(thread);
	}

	if (sched_flags & TH_SFLAG_DEPRESS) {
		                                                            
		priority = DEPRESSPRI;
	} else {
		                                                                       
		if ((sched_flags & TH_SFLAG_POLLDEPRESS)) {
			priority = DEPRESSPRI;
		}

		if (thread->kern_promotion_schedpri > 0) {
			priority = MAX(priority, thread->kern_promotion_schedpri);

			if (sched_mode != TH_MODE_REALTIME) {
				priority = MIN(priority, MAXPRI_PROMOTE);
			}
		}

		if (sched_flags & TH_SFLAG_PROMOTED) {
			priority = MAX(priority, thread->promotion_priority);

			if (sched_mode != TH_MODE_REALTIME) {
				priority = MIN(priority, MAXPRI_PROMOTE);
			}
		}

		if (sched_flags & TH_SFLAG_PROMOTE_REASON_MASK) {
			if (sched_flags & TH_SFLAG_RW_PROMOTED) {
				priority = MAX(priority, MINPRI_RWLOCK);
			}

			if (sched_flags & TH_SFLAG_WAITQ_PROMOTED) {
				priority = MAX(priority, MINPRI_WAITQ);
			}

			if (sched_flags & TH_SFLAG_EXEC_PROMOTED) {
				priority = MAX(priority, MINPRI_EXEC);
			}
		}
	}

	set_sched_pri(thread, priority, options);
}

void
sched_default_quantum_expire(thread_t thread __unused)
{
	  
                                                                   
                              
    
}

int smt_timeshare_enabled = 1;
int smt_sched_bonus_16ths = 8;

#if defined(CONFIG_SCHED_TIMESHARE_CORE)

  
                               
  
                                    
                        
  
                                      
  
                                 
   
void
lightweight_update_priority(thread_t thread)
{
	assert(thread->runq == PROCESSOR_NULL);
	assert(thread == current_thread());

	if (thread->sched_mode == TH_MODE_TIMESHARE) {
		int priority;
		uint32_t delta;

		thread_timer_delta(thread, delta);

		  
                                      
                                    
               
     
		if (thread->pri_shift < INT8_MAX) {
			if (thread_no_smt(thread) && smt_timeshare_enabled) {
				thread->sched_usage += (delta + ((delta * smt_sched_bonus_16ths) >> 4));
			} else {
				thread->sched_usage += delta;
			}
		}

		thread->cpu_delta += delta;

#if CONFIG_SCHED_CLUTCH
		  
                                                                           
                                                                        
                                      
     
		sched_clutch_cpu_usage_update(thread, delta);
#endif                          

		priority = sched_compute_timeshare_priority(thread);

		if (priority != thread->sched_pri) {
			thread_recompute_sched_pri(thread, SETPRI_LAZY);
		}
	}
}

  
                                          
  
                                                                  
                                                                      
                                            
   

const struct shift_data        sched_decay_shifts[SCHED_DECAY_TICKS] = {
	{ .shift1 = 1, .shift2 = 1 },
	{ .shift1 = 1, .shift2 = 3 },
	{ .shift1 = 1, .shift2 = -3 },
	{ .shift1 = 2, .shift2 = -7 },
	{ .shift1 = 3, .shift2 = 5 },
	{ .shift1 = 3, .shift2 = -5 },
	{ .shift1 = 4, .shift2 = -8 },
	{ .shift1 = 5, .shift2 = 7 },
	{ .shift1 = 5, .shift2 = -7 },
	{ .shift1 = 6, .shift2 = -10 },
	{ .shift1 = 7, .shift2 = 10 },
	{ .shift1 = 7, .shift2 = -9 },
	{ .shift1 = 8, .shift2 = -11 },
	{ .shift1 = 9, .shift2 = 12 },
	{ .shift1 = 9, .shift2 = -11 },
	{ .shift1 = 10, .shift2 = -13 },
	{ .shift1 = 11, .shift2 = 14 },
	{ .shift1 = 11, .shift2 = -13 },
	{ .shift1 = 12, .shift2 = -15 },
	{ .shift1 = 13, .shift2 = 17 },
	{ .shift1 = 13, .shift2 = -15 },
	{ .shift1 = 14, .shift2 = -17 },
	{ .shift1 = 15, .shift2 = 19 },
	{ .shift1 = 16, .shift2 = 18 },
	{ .shift1 = 16, .shift2 = -19 },
	{ .shift1 = 17, .shift2 = 22 },
	{ .shift1 = 18, .shift2 = 20 },
	{ .shift1 = 18, .shift2 = -20 },
	{ .shift1 = 19, .shift2 = 26 },
	{ .shift1 = 20, .shift2 = 22 },
	{ .shift1 = 20, .shift2 = -22 },
	{ .shift1 = 21, .shift2 = -27 }
};

  
                                    
  
                                                                
   
extern int sched_pri_decay_band_limit;


                                                                           
#if !defined(XNU_TARGET_OS_OSX) && !CONFIG_SCHED_CLUTCH

int
sched_compute_timeshare_priority(thread_t thread)
{
	int decay_amount;
	int decay_limit = sched_pri_decay_band_limit;

	if (thread->base_pri > BASEPRI_FOREGROUND) {
		decay_limit += (thread->base_pri - BASEPRI_FOREGROUND);
	}

	if (thread->pri_shift == INT8_MAX) {
		decay_amount = 0;
	} else {
		decay_amount = (thread->sched_usage >> thread->pri_shift);
	}

	if (decay_amount > decay_limit) {
		decay_amount = decay_limit;
	}

	                              
	int priority = thread->base_pri - decay_amount;

	if (priority < MAXPRI_THROTTLE) {
		if (thread->task->max_priority > MAXPRI_THROTTLE) {
			priority = MAXPRI_THROTTLE;
		} else if (priority < MINPRI_USER) {
			priority = MINPRI_USER;
		}
	} else if (priority > MAXPRI_KERNEL) {
		priority = MAXPRI_KERNEL;
	}

	return priority;
}

#else                                                          

int
sched_compute_timeshare_priority(thread_t thread)
{
	                              
	int priority = thread->base_pri;

	if (thread->pri_shift != INT8_MAX) {
		priority -= (thread->sched_usage >> thread->pri_shift);
	}

	if (priority < MINPRI_USER) {
		priority = MINPRI_USER;
	} else if (priority > MAXPRI_KERNEL) {
		priority = MAXPRI_KERNEL;
	}

	return priority;
}

#endif                                                          

  
                      
  
                                                                             
  
                                 
   
boolean_t
can_update_priority(
	thread_t        thread)
{
	if (sched_tick == thread->sched_stamp) {
		return FALSE;
	} else {
		return TRUE;
	}
}

  
                  
  
                                                            
  
                                 
   
void
update_priority(
	thread_t        thread)
{
	uint32_t ticks, delta;

	ticks = sched_tick - thread->sched_stamp;
	assert(ticks != 0);

	thread->sched_stamp += ticks;

	                                                   
	if (sched_decay_usage_age_factor > 1) {
		ticks *= sched_decay_usage_age_factor;
	}

	  
                          
    
	thread_timer_delta(thread, delta);
	if (ticks < SCHED_DECAY_TICKS) {
		  
                                                                      
                                                                  
                                                      
     
		if (thread->pri_shift < INT8_MAX) {
			if (thread_no_smt(thread) && smt_timeshare_enabled) {
				thread->sched_usage += (delta + ((delta * smt_sched_bonus_16ths) >> 4));
			} else {
				thread->sched_usage += delta;
			}
		}

		thread->cpu_usage += delta + thread->cpu_delta;
		thread->cpu_delta = 0;

#if CONFIG_SCHED_CLUTCH
		  
                                                                           
                                                                        
                                      
     
		sched_clutch_cpu_usage_update(thread, delta);
#endif                          

		const struct shift_data *shiftp = &sched_decay_shifts[ticks];

		if (shiftp->shift2 > 0) {
			thread->cpu_usage =   (thread->cpu_usage >> shiftp->shift1) +
			    (thread->cpu_usage >> shiftp->shift2);
			thread->sched_usage = (thread->sched_usage >> shiftp->shift1) +
			    (thread->sched_usage >> shiftp->shift2);
		} else {
			thread->cpu_usage =   (thread->cpu_usage >>   shiftp->shift1) -
			    (thread->cpu_usage >> -(shiftp->shift2));
			thread->sched_usage = (thread->sched_usage >>   shiftp->shift1) -
			    (thread->sched_usage >> -(shiftp->shift2));
		}
	} else {
		thread->cpu_usage = thread->cpu_delta = 0;
		thread->sched_usage = 0;
	}

	  
                                
    
	if ((thread->sched_flags & TH_SFLAG_FAILSAFE) &&
	    mach_absolute_time() >= thread->safe_release) {
		sched_thread_mode_undemote(thread, TH_SFLAG_FAILSAFE);
	}

	  
                                                                 
                                                               
                                                                  
                                                                
                               
    
#if CONFIG_SCHED_CLUTCH
	thread->pri_shift = sched_clutch_thread_pri_shift(thread, thread->th_sched_bucket);
#else                          
	thread->pri_shift = sched_pri_shifts[thread->th_sched_bucket];
#endif                          

	                                                  
	if (thread->sched_mode == TH_MODE_TIMESHARE) {
		thread_recompute_sched_pri(thread, SETPRI_LAZY);
	}
}

#endif                                  


  
                                                               
                                                                
                                                            
                                         
  
                                                                
                                      
   
uint32_t       sched_run_buckets[TH_BUCKET_MAX];

static void
sched_incr_bucket(sched_bucket_t bucket)
{
	assert(bucket >= TH_BUCKET_FIXPRI &&
	    bucket <= TH_BUCKET_SHARE_BG);

	os_atomic_inc(&sched_run_buckets[bucket], relaxed);
}

static void
sched_decr_bucket(sched_bucket_t bucket)
{
	assert(bucket >= TH_BUCKET_FIXPRI &&
	    bucket <= TH_BUCKET_SHARE_BG);

	assert(os_atomic_load(&sched_run_buckets[bucket], relaxed) > 0);

	os_atomic_dec(&sched_run_buckets[bucket], relaxed);
}

static void
sched_add_bucket(sched_bucket_t bucket, uint8_t run_weight)
{
	assert(bucket >= TH_BUCKET_FIXPRI &&
	    bucket <= TH_BUCKET_SHARE_BG);

	os_atomic_add(&sched_run_buckets[bucket], run_weight, relaxed);
}

static void
sched_sub_bucket(sched_bucket_t bucket, uint8_t run_weight)
{
	assert(bucket >= TH_BUCKET_FIXPRI &&
	    bucket <= TH_BUCKET_SHARE_BG);

	assert(os_atomic_load(&sched_run_buckets[bucket], relaxed) > 0);

	os_atomic_sub(&sched_run_buckets[bucket], run_weight, relaxed);
}

uint32_t
sched_run_incr(thread_t thread)
{
	assert((thread->state & (TH_RUN | TH_IDLE)) == TH_RUN);

	uint32_t new_count = os_atomic_inc(&sched_run_buckets[TH_BUCKET_RUN], relaxed);

	sched_incr_bucket(thread->th_sched_bucket);

	return new_count;
}

uint32_t
sched_run_decr(thread_t thread)
{
	assert((thread->state & (TH_RUN | TH_IDLE)) != TH_RUN);

	sched_decr_bucket(thread->th_sched_bucket);

	uint32_t new_count = os_atomic_dec(&sched_run_buckets[TH_BUCKET_RUN], relaxed);

	return new_count;
}

uint32_t
sched_smt_run_incr(thread_t thread)
{
	assert((thread->state & (TH_RUN | TH_IDLE)) == TH_RUN);

	uint8_t run_weight = (thread_no_smt(thread) && smt_timeshare_enabled) ? 2 : 1;
	thread->sched_saved_run_weight = run_weight;

	uint32_t new_count = os_atomic_add(&sched_run_buckets[TH_BUCKET_RUN], run_weight, relaxed);

	sched_add_bucket(thread->th_sched_bucket, run_weight);

	return new_count;
}

uint32_t
sched_smt_run_decr(thread_t thread)
{
	assert((thread->state & (TH_RUN | TH_IDLE)) != TH_RUN);

	uint8_t run_weight = thread->sched_saved_run_weight;

	sched_sub_bucket(thread->th_sched_bucket, run_weight);

	uint32_t new_count = os_atomic_sub(&sched_run_buckets[TH_BUCKET_RUN], run_weight, relaxed);

	return new_count;
}

void
sched_update_thread_bucket(thread_t thread)
{
	sched_bucket_t old_bucket = thread->th_sched_bucket;
	sched_bucket_t new_bucket = TH_BUCKET_RUN;

	switch (thread->sched_mode) {
	case TH_MODE_FIXED:
	case TH_MODE_REALTIME:
		new_bucket = TH_BUCKET_FIXPRI;
		break;

	case TH_MODE_TIMESHARE:
		if (thread->base_pri > BASEPRI_DEFAULT) {
			new_bucket = TH_BUCKET_SHARE_FG;
		} else if (thread->base_pri > BASEPRI_UTILITY) {
			new_bucket = TH_BUCKET_SHARE_DF;
		} else if (thread->base_pri > MAXPRI_THROTTLE) {
			new_bucket = TH_BUCKET_SHARE_UT;
		} else {
			new_bucket = TH_BUCKET_SHARE_BG;
		}
		break;

	default:
		panic("unexpected mode: %d", thread->sched_mode);
		break;
	}

	if (old_bucket != new_bucket) {
		thread->th_sched_bucket = new_bucket;
		thread->pri_shift = sched_pri_shifts[new_bucket];

		if ((thread->state & (TH_RUN | TH_IDLE)) == TH_RUN) {
			sched_decr_bucket(old_bucket);
			sched_incr_bucket(new_bucket);
		}
	}
}

void
sched_smt_update_thread_bucket(thread_t thread)
{
	sched_bucket_t old_bucket = thread->th_sched_bucket;
	sched_bucket_t new_bucket = TH_BUCKET_RUN;

	switch (thread->sched_mode) {
	case TH_MODE_FIXED:
	case TH_MODE_REALTIME:
		new_bucket = TH_BUCKET_FIXPRI;
		break;

	case TH_MODE_TIMESHARE:
		if (thread->base_pri > BASEPRI_DEFAULT) {
			new_bucket = TH_BUCKET_SHARE_FG;
		} else if (thread->base_pri > BASEPRI_UTILITY) {
			new_bucket = TH_BUCKET_SHARE_DF;
		} else if (thread->base_pri > MAXPRI_THROTTLE) {
			new_bucket = TH_BUCKET_SHARE_UT;
		} else {
			new_bucket = TH_BUCKET_SHARE_BG;
		}
		break;

	default:
		panic("unexpected mode: %d", thread->sched_mode);
		break;
	}

	if (old_bucket != new_bucket) {
		thread->th_sched_bucket = new_bucket;
		thread->pri_shift = sched_pri_shifts[new_bucket];

		if ((thread->state & (TH_RUN | TH_IDLE)) == TH_RUN) {
			sched_sub_bucket(old_bucket, thread->sched_saved_run_weight);
			sched_add_bucket(new_bucket, thread->sched_saved_run_weight);
		}
	}
}

  
                                        
                                             
                                                         
  
                                            
   
void
sched_set_thread_mode(thread_t thread, sched_mode_t new_mode)
{
	assert(thread->runq == PROCESSOR_NULL);

	switch (new_mode) {
	case TH_MODE_FIXED:
	case TH_MODE_REALTIME:
	case TH_MODE_TIMESHARE:
		break;

	default:
		panic("unexpected mode: %d", new_mode);
		break;
	}

#if CONFIG_SCHED_AUTO_JOIN
	  
                                                                    
                                                                          
                                                        
    
	if ((thread->sched_flags & TH_SFLAG_THREAD_GROUP_AUTO_JOIN) && (new_mode != TH_MODE_REALTIME)) {
		assert((thread->sched_mode == TH_MODE_REALTIME) || (thread->th_work_interval_flags & TH_WORK_INTERVAL_FLAGS_AUTO_JOIN_LEAK));
		work_interval_auto_join_demote(thread);
	}
#endif                             

	thread->sched_mode = new_mode;

	SCHED(update_thread_bucket)(thread);
}

  
                                                                              
   
void
sched_thread_mode_demote(thread_t thread, uint32_t reason)
{
	assert(reason & TH_SFLAG_DEMOTED_MASK);
	assert((thread->sched_flags & reason) != reason);

	if (thread->policy_reset) {
		return;
	}

	if (thread->sched_flags & TH_SFLAG_DEMOTED_MASK) {
		                                               
		thread->sched_flags |= reason;
		return;
	}

	assert(thread->saved_mode == TH_MODE_NONE);

	boolean_t removed = thread_run_queue_remove(thread);

	thread->sched_flags |= reason;

	thread->saved_mode = thread->sched_mode;

	sched_set_thread_mode(thread, TH_MODE_TIMESHARE);

	thread_recompute_priority(thread);

	if (removed) {
		thread_run_queue_reinsert(thread, SCHED_TAILQ);
	}
}

  
                                                                                           
   
void
sched_thread_mode_undemote(thread_t thread, uint32_t reason)
{
	assert(reason & TH_SFLAG_DEMOTED_MASK);
	assert((thread->sched_flags & reason) == reason);
	assert(thread->saved_mode != TH_MODE_NONE);
	assert(thread->sched_mode == TH_MODE_TIMESHARE);
	assert(thread->policy_reset == 0);

	thread->sched_flags &= ~reason;

	if (thread->sched_flags & TH_SFLAG_DEMOTED_MASK) {
		                                             
		return;
	}

	boolean_t removed = thread_run_queue_remove(thread);

	sched_set_thread_mode(thread, thread->saved_mode);

	thread->saved_mode = TH_MODE_NONE;

	thread_recompute_priority(thread);

	if (removed) {
		thread_run_queue_reinsert(thread, SCHED_TAILQ);
	}
}

  
                                                                 
  
                                                
                                                   
                                                                             
  
                                        
   
void
sched_thread_promote_reason(thread_t    thread,
    uint32_t    reason,
    __kdebug_only uintptr_t   trace_obj                     )
{
	assert(reason & TH_SFLAG_PROMOTE_REASON_MASK);
	assert((thread->sched_flags & reason) != reason);

	switch (reason) {
	case TH_SFLAG_RW_PROMOTED:
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_RW_PROMOTE),
		    thread_tid(thread), thread->sched_pri,
		    thread->base_pri, trace_obj);
		break;
	case TH_SFLAG_WAITQ_PROMOTED:
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_WAITQ_PROMOTE),
		    thread_tid(thread), thread->sched_pri,
		    thread->base_pri, trace_obj);
		break;
	case TH_SFLAG_EXEC_PROMOTED:
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_EXEC_PROMOTE),
		    thread_tid(thread), thread->sched_pri,
		    thread->base_pri, trace_obj);
		break;
	}

	thread->sched_flags |= reason;

	thread_recompute_sched_pri(thread, SETPRI_DEFAULT);
}

  
                                  
                                                                                
  
                                        
   
void
sched_thread_unpromote_reason(thread_t  thread,
    uint32_t  reason,
    __kdebug_only uintptr_t trace_obj                     )
{
	assert(reason & TH_SFLAG_PROMOTE_REASON_MASK);
	assert((thread->sched_flags & reason) == reason);

	switch (reason) {
	case TH_SFLAG_RW_PROMOTED:
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_RW_DEMOTE),
		    thread_tid(thread), thread->sched_pri,
		    thread->base_pri, trace_obj);
		break;
	case TH_SFLAG_WAITQ_PROMOTED:
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_WAITQ_DEMOTE),
		    thread_tid(thread), thread->sched_pri,
		    thread->base_pri, trace_obj);
		break;
	case TH_SFLAG_EXEC_PROMOTED:
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_EXEC_DEMOTE),
		    thread_tid(thread), thread->sched_pri,
		    thread->base_pri, trace_obj);
		break;
	}

	thread->sched_flags &= ~reason;

	thread_recompute_sched_pri(thread, SETPRI_DEFAULT);
}
