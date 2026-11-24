/*
 * Copyright (c) 2000-2008 Apple Inc. All rights reserved.
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
 * @APPLE_FREE_COPYRIGHT@
 */
  
                 
                                                                
   

#include <mach/mach_types.h>

#include <kern/timer_queue.h>
#include <kern/timer_call.h>
#include <kern/clock.h>
#include <kern/thread.h>
#include <kern/processor.h>
#include <kern/macro_help.h>
#include <kern/spl.h>
#include <kern/timer_queue.h>
#include <kern/pms.h>

#include <machine/commpage.h>
#include <machine/machine_routines.h>

#include <sys/kdebug.h>
#include <i386/cpu_data.h>
#include <i386/cpu_topology.h>
#include <i386/cpu_threads.h>

uint32_t spurious_timers;

  
                         
  
                                                                                               
                                                    
  
                                                                                        
   
void
timer_intr(int user_mode, uint64_t rip)
{
	uint64_t        orig_abstime, abstime;
	rtclock_timer_t *mytimer;
	cpu_data_t      *pp;
	uint64_t        pmdeadline;
	uint64_t        min_deadline = EndOfAllTime;
	uint64_t        run_deadline = EndOfAllTime;
	bool            timer_processed = false;

	pp = current_cpu_datap();

	SCHED_STATS_INC(timer_pop_count);

	orig_abstime = abstime = mach_absolute_time();

	  
                                      
    
	mytimer = &pp->rtclock_timer;
	timer_processed = (mytimer->deadline <= abstime ||
	    abstime >= mytimer->queue.earliest_soft_deadline);
	if (timer_processed) {
		uint64_t rtclock_deadline = MAX(mytimer->deadline, mytimer->when_set);
		  
                                                                     
                                        
     
		if (abstime > rtclock_deadline) {
			TCOAL_DEBUG(0xEEEE0000, abstime,
			    mytimer->queue.earliest_soft_deadline,
			    abstime - mytimer->queue.earliest_soft_deadline, 0, 0);
		} else {
			min_deadline = rtclock_deadline;
		}

		mytimer->has_expired = TRUE;
		mytimer->deadline = timer_queue_expire(&mytimer->queue, abstime);
		mytimer->has_expired = FALSE;

		  
                                                                       
     
		abstime = mach_absolute_time();
		mytimer->when_set = abstime;
	}

	  
                                        
    
	run_deadline = running_timers_expire(pp->cpu_processor, abstime);
	if (run_deadline != EndOfAllTime) {
		if (run_deadline < min_deadline) {
			min_deadline = run_deadline;
		}
		timer_processed = true;
		abstime = mach_absolute_time();
	}

	  
                                                               
    
	if (__probable(timer_processed)) {
		  
                                                                      
     
		int64_t latency = min_deadline == EndOfAllTime ? 0 :
		    (int64_t)(abstime - min_deadline);
		  
                                                               
                                     
                                                               
                                                  
     
		KDBG_RELEASE(DECR_TRAP_LATENCY, -latency,
		    user_mode != 0 ? rip : VM_KERNEL_UNSLIDE(rip), user_mode);
	}

	  
                                                 
    
	if ((pmdeadline = pmCPUGetDeadline(pp)) && (pmdeadline <= abstime)) {
		KDBG_RELEASE(DECR_PM_DEADLINE | DBG_FUNC_START);
		pmCPUDeadline(pp);
		KDBG_RELEASE(DECR_PM_DEADLINE | DBG_FUNC_END);
		timer_processed = true;
		  
                                                                    
     
	}

	  
                               
    
	x86_lcpu()->rtcDeadline = EndOfAllTime;
	timer_resync_deadlines();

	if (__improbable(!timer_processed)) {
		spurious_timers++;
	}
}

  
                          
   
void
timer_set_deadline(uint64_t deadline)
{
	rtclock_timer_t         *mytimer;
	spl_t                   s;
	cpu_data_t              *pp;

	s = splclock();                                               
	pp = current_cpu_datap();

	mytimer = &pp->rtclock_timer;                                          
	mytimer->deadline = deadline;                                        
	mytimer->when_set = mach_absolute_time();

	timer_resync_deadlines();

	splx(s);
}

  
                                                                       
  
                                
   
void
timer_resync_deadlines(void)
{
	uint64_t                deadline = EndOfAllTime;
	uint64_t                pmdeadline;
	rtclock_timer_t         *mytimer;
	spl_t                   s = splclock();
	cpu_data_t              *pp;
	uint32_t                decr;

	pp = current_cpu_datap();
	if (!pp->cpu_running) {
		                                                            
		return;
	}

	  
                                            
    
	mytimer = &pp->rtclock_timer;
	if (!mytimer->has_expired &&
	    0 < mytimer->deadline && mytimer->deadline < EndOfAllTime) {
		deadline = mytimer->deadline;
	}

	  
                                                                  
    
	pmdeadline = pmCPUGetDeadline(pp);
	if (0 < pmdeadline && pmdeadline < deadline) {
		deadline = pmdeadline;
	}

	uint64_t run_deadline = running_timers_deadline(pp->cpu_processor);
	if (run_deadline < deadline) {
		deadline = run_deadline;
	}

	  
                               
    
	decr = (uint32_t) setPop(deadline);

	                                             
	if (decr != 0 && deadline != pmdeadline) {
		uint64_t queue_count = 0;
		if (deadline != run_deadline) {
			  
                                               
                        
      
			queue_count = mytimer->queue.count;
		}
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    DECR_SET_DEADLINE | DBG_FUNC_NONE,
		    decr, 2,
		    deadline,
		    queue_count, 0);
	}
	splx(s);
}

void
timer_queue_expire_local(
	__unused void                   *arg)
{
	rtclock_timer_t         *mytimer;
	uint64_t                        abstime;
	cpu_data_t                      *pp;

	pp = current_cpu_datap();

	mytimer = &pp->rtclock_timer;
	abstime = mach_absolute_time();

	mytimer->has_expired = TRUE;
	mytimer->deadline = timer_queue_expire(&mytimer->queue, abstime);
	mytimer->has_expired = FALSE;
	mytimer->when_set = mach_absolute_time();

	timer_resync_deadlines();
}

void
timer_queue_expire_rescan(
	__unused void                   *arg)
{
	rtclock_timer_t         *mytimer;
	uint64_t                abstime;
	cpu_data_t              *pp;

	assert(ml_get_interrupts_enabled() == FALSE);
	pp = current_cpu_datap();

	mytimer = &pp->rtclock_timer;
	abstime = mach_absolute_time();

	mytimer->has_expired = TRUE;
	mytimer->deadline = timer_queue_expire_with_options(&mytimer->queue, abstime, TRUE);
	mytimer->has_expired = FALSE;
	mytimer->when_set = mach_absolute_time();

	timer_resync_deadlines();
}

#define TIMER_RESORT_THRESHOLD_ABSTIME (50 * NSEC_PER_MSEC)

#if TCOAL_PRIO_STATS
int32_t nc_tcl, rt_tcl, bg_tcl, kt_tcl, fp_tcl, ts_tcl, qos_tcl;
#define TCOAL_PRIO_STAT(x) (x++)
#else
#define TCOAL_PRIO_STAT(x)
#endif

boolean_t
timer_resort_threshold(uint64_t skew)
{
	if (skew >= TIMER_RESORT_THRESHOLD_ABSTIME) {
		return TRUE;
	} else {
		return FALSE;
	}
}

  
                                                       
                                                
   
mpqueue_head_t *
timer_queue_assign(
	uint64_t        deadline)
{
	cpu_data_t              *cdp = current_cpu_datap();
	mpqueue_head_t          *queue;

	if (cdp->cpu_running) {
		queue = &cdp->rtclock_timer.queue;

		if (deadline < cdp->rtclock_timer.deadline) {
			timer_set_deadline(deadline);
		}
	} else {
		queue = &cpu_datap(master_cpu)->rtclock_timer.queue;
	}

	return queue;
}

void
timer_queue_cancel(
	mpqueue_head_t  *queue,
	uint64_t        deadline,
	uint64_t        new_deadline)
{
	if (queue == &current_cpu_datap()->rtclock_timer.queue) {
		if (deadline < new_deadline) {
			timer_set_deadline(new_deadline);
		}
	}
}

  
                                                                     
                                                                        
                                                                       
                                                                
                                                                            
                                                                        
   
uint32_t
timer_queue_migrate_cpu(int target_cpu)
{
	cpu_data_t      *target_cdp = cpu_datap(target_cpu);
	cpu_data_t      *cdp = current_cpu_datap();
	int             ntimers_moved;

	assert(!ml_get_interrupts_enabled());
	assert(target_cpu != cdp->cpu_number);
	assert(target_cpu == master_cpu);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    DECR_TIMER_MIGRATE | DBG_FUNC_START,
	    target_cpu,
	    cdp->rtclock_timer.deadline, (cdp->rtclock_timer.deadline >> 32),
	    0, 0);

	  
                                                                       
                                                                   
                                                                     
                                                                   
                                                               
    
	ntimers_moved = timer_queue_migrate(&cdp->rtclock_timer.queue,
	    &target_cdp->rtclock_timer.queue);

	  
                                                  
    
	if (ntimers_moved > 0) {
		cdp->rtclock_timer.deadline = EndOfAllTime;
		setPop(EndOfAllTime);
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    DECR_TIMER_MIGRATE | DBG_FUNC_END,
	    target_cpu, ntimers_moved, 0, 0, 0);

	return ntimers_moved;
}

mpqueue_head_t *
timer_queue_cpu(int cpu)
{
	return &cpu_datap(cpu)->rtclock_timer.queue;
}

void
timer_call_cpu(int cpu, void (*fn)(void *), void *arg)
{
	mp_cpus_call(cpu_to_cpumask(cpu), SYNC, fn, arg);
}

void
timer_call_nosync_cpu(int cpu, void (*fn)(void *), void *arg)
{
	                                        
	mp_cpus_call(cpu_to_cpumask(cpu), NOSYNC, fn, arg);
}


static timer_coalescing_priority_params_ns_t tcoal_prio_params_init =
{
	.idle_entry_timer_processing_hdeadline_threshold_ns = 5000ULL * NSEC_PER_USEC,
	.interrupt_timer_coalescing_ilat_threshold_ns = 30ULL * NSEC_PER_USEC,
	.timer_resort_threshold_ns = 50 * NSEC_PER_MSEC,
	.timer_coalesce_rt_shift = 0,
	.timer_coalesce_bg_shift = -5,
	.timer_coalesce_kt_shift = 3,
	.timer_coalesce_fp_shift = 3,
	.timer_coalesce_ts_shift = 3,
	.timer_coalesce_rt_ns_max = 0ULL,
	.timer_coalesce_bg_ns_max = 100 * NSEC_PER_MSEC,
	.timer_coalesce_kt_ns_max = 1 * NSEC_PER_MSEC,
	.timer_coalesce_fp_ns_max = 1 * NSEC_PER_MSEC,
	.timer_coalesce_ts_ns_max = 1 * NSEC_PER_MSEC,
	.latency_qos_scale = {3, 2, 1, -2, -15, -15},
	.latency_qos_ns_max = {1 * NSEC_PER_MSEC, 5 * NSEC_PER_MSEC, 20 * NSEC_PER_MSEC,
		               75 * NSEC_PER_MSEC, 10000 * NSEC_PER_MSEC, 10000 * NSEC_PER_MSEC},
	.latency_tier_rate_limited = {FALSE, FALSE, FALSE, FALSE, TRUE, TRUE},
};

timer_coalescing_priority_params_ns_t *
timer_call_get_priority_params(void)
{
	return &tcoal_prio_params_init;
}
