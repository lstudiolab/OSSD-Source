/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
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
 * @APPLE_FREE_COPYRIGHT@
 */
  
                  
                                                         
                  
   

#include <mach/mach_types.h>

#include <kern/clock.h>
#include <kern/thread.h>
#include <kern/processor.h>
#include <kern/macro_help.h>
#include <kern/spl.h>
#include <kern/timer_queue.h>
#include <kern/timer_call.h>

#include <machine/commpage.h>
#include <machine/machine_routines.h>

#include <sys/kdebug.h>
#include <arm/cpu_data.h>
#include <arm/cpu_data_internal.h>
#include <arm/cpu_internal.h>

  
                              
  
                                                                                               
                                                    
  
                                                                                        
   
void
timer_intr(__unused int inuser, __unused uint64_t iaddr)
{
	uint64_t        abstime, new_idle_timeout_ticks;
	rtclock_timer_t *mytimer;
	cpu_data_t     *cpu_data_ptr;
	processor_t     processor;

	cpu_data_ptr = getCpuDatap();
	mytimer = &cpu_data_ptr->rtclock_timer;                               
	abstime = mach_absolute_time();                       

	                                         
	if ((cpu_data_ptr->idle_timer_deadline > 0) && (cpu_data_ptr->idle_timer_deadline <= abstime)) {
		cpu_data_ptr->idle_timer_deadline = 0x0ULL;
		new_idle_timeout_ticks = 0x0ULL;

		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_COMMON, MACHDBG_CODE(DBG_MACH_EXCP_DECI, 3) | DBG_FUNC_START, 0, 0, 0, 0, 0);
		cpu_data_ptr->idle_timer_notify(cpu_data_ptr->idle_timer_refcon, &new_idle_timeout_ticks);
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_COMMON, MACHDBG_CODE(DBG_MACH_EXCP_DECI, 3) | DBG_FUNC_END, 0, 0, 0, 0, 0);

		                                                                         
		if (new_idle_timeout_ticks != 0x0ULL) {
			clock_absolutetime_interval_to_deadline(new_idle_timeout_ticks, &cpu_data_ptr->idle_timer_deadline);
		}

		abstime = mach_absolute_time();                                            
	}

	                                        
	if (mytimer->deadline <= abstime) {                           
                                                 
		mytimer->has_expired = TRUE;                                 
		mytimer->deadline = EndOfAllTime;                              
                                                                
                                                               
                                                           
		mytimer->deadline = timer_queue_expire(&mytimer->queue, abstime);
		mytimer->has_expired = FALSE;
		abstime = mach_absolute_time();                                            
	}

	processor = PERCPU_GET_RELATIVE(processor, cpu_data, cpu_data_ptr);
	(void)running_timers_expire(processor, abstime);
	  
                              
    

	                                    
	cpu_data_ptr->rtcPop = EndOfAllTime;
	                                
	timer_resync_deadlines();
}

  
                         
   
void
timer_set_deadline(uint64_t deadline)
{
	rtclock_timer_t *mytimer;
	spl_t           s;
	cpu_data_t     *cpu_data_ptr;

	s = splclock();                               
	cpu_data_ptr = getCpuDatap();

	mytimer = &cpu_data_ptr->rtclock_timer;                                
	mytimer->deadline = deadline;                                    

	timer_resync_deadlines();

	splx(s);
}

  
                                                                       
  
                                
   
void
timer_resync_deadlines(void)
{
	uint64_t        deadline;
	rtclock_timer_t *mytimer;
	spl_t           s = splclock();                              
	cpu_data_t     *cpu_data_ptr;

	cpu_data_ptr = getCpuDatap();

	deadline = 0;

	                                                      
	mytimer = &cpu_data_ptr->rtclock_timer;                                
	if ((!mytimer->has_expired) && (mytimer->deadline > 0)) {
		deadline = mytimer->deadline;
	}

	                                                              
	if ((cpu_data_ptr->idle_timer_deadline > 0)
	    && (cpu_data_ptr->idle_timer_deadline < deadline)) {
		deadline = cpu_data_ptr->idle_timer_deadline;
	}

	uint64_t run_deadline = running_timers_deadline(
		PERCPU_GET_RELATIVE(processor, cpu_data, cpu_data_ptr));
	if (run_deadline < deadline) {
		deadline = run_deadline;
	}

	if ((deadline == EndOfAllTime)
	    || ((deadline > 0) && (cpu_data_ptr->rtcPop != deadline))) {
		int             decr;

		decr = setPop(deadline);

		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    MACHDBG_CODE(DBG_MACH_EXCP_DECI, 1) | DBG_FUNC_NONE,
		    decr, 2, 0, 0, 0);
	}
	splx(s);
}

void
timer_queue_expire_local(
	__unused void                   *arg)
{
	rtclock_timer_t         *mytimer = &getCpuDatap()->rtclock_timer;
	uint64_t                abstime;

	abstime = mach_absolute_time();
	mytimer->has_expired = TRUE;
	mytimer->deadline = timer_queue_expire(&mytimer->queue, abstime);
	mytimer->has_expired = FALSE;

	timer_resync_deadlines();
}

boolean_t
timer_resort_threshold(__unused uint64_t skew)
{
	return FALSE;
}

mpqueue_head_t *
timer_queue_assign(
	uint64_t                deadline)
{
	cpu_data_t                              *cpu_data_ptr = getCpuDatap();
	mpqueue_head_t          *queue;

	if (cpu_data_ptr->cpu_running) {
		queue = &cpu_data_ptr->rtclock_timer.queue;

		if (deadline < cpu_data_ptr->rtclock_timer.deadline) {
			timer_set_deadline(deadline);
		}
	} else {
		queue = &cpu_datap(master_cpu)->rtclock_timer.queue;
	}

	return queue;
}

void
timer_queue_cancel(
	mpqueue_head_t          *queue,
	uint64_t                deadline,
	uint64_t                new_deadline)
{
	if (queue == &getCpuDatap()->rtclock_timer.queue) {
		if (deadline < new_deadline) {
			timer_set_deadline(new_deadline);
		}
	}
}

mpqueue_head_t *
timer_queue_cpu(int cpu)
{
	return &cpu_datap(cpu)->rtclock_timer.queue;
}

void
timer_call_cpu(int cpu, void (*fn)(void *), void *arg)
{
	cpu_signal(cpu_datap(cpu), SIGPxcall, (void *) fn, arg);
}

void
timer_call_nosync_cpu(int cpu, void (*fn)(void *), void *arg)
{
	                                        
	cpu_signal(cpu_datap(cpu), SIGPxcall, (void *) fn, arg);
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
#if XNU_TARGET_OS_OSX
	.latency_qos_scale = {3, 2, 1, -2, 3, 3},
	.latency_qos_ns_max = {1 * NSEC_PER_MSEC, 5 * NSEC_PER_MSEC, 20 * NSEC_PER_MSEC,
		               75 * NSEC_PER_MSEC, 1 * NSEC_PER_MSEC, 1 * NSEC_PER_MSEC},
	.latency_tier_rate_limited = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE},
#else                        
	.latency_qos_scale = {3, 2, 1, -2, -15, -15},
	.latency_qos_ns_max = {1 * NSEC_PER_MSEC, 5 * NSEC_PER_MSEC, 20 * NSEC_PER_MSEC,
		               75 * NSEC_PER_MSEC, 10000 * NSEC_PER_MSEC, 10000 * NSEC_PER_MSEC},
	.latency_tier_rate_limited = {FALSE, FALSE, FALSE, FALSE, TRUE, TRUE},
#endif                        
};
timer_coalescing_priority_params_ns_t *
timer_call_get_priority_params(void)
{
	return &tcoal_prio_params_init;
}
