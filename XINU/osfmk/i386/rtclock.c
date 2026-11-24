/*
 * Copyright (c) 2000-2012 Apple Inc. All rights reserved.
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

  
                        
                                                       
                                                 
                                                      
                                                  
                                                      
                                                
   


#include <mach/mach_types.h>

#include <kern/cpu_data.h>
#include <kern/cpu_number.h>
#include <kern/clock.h>
#include <kern/host_notify.h>
#include <kern/macro_help.h>
#include <kern/misc_protos.h>
#include <kern/spl.h>
#include <kern/assert.h>
#include <kern/timer_queue.h>
#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>                             
#include <architecture/i386/pio.h>
#include <i386/machine_cpu.h>
#include <i386/cpuid.h>
#include <i386/cpu_threads.h>
#include <i386/mp.h>
#include <i386/machine_routines.h>
#include <i386/pal_routines.h>
#include <i386/proc_reg.h>
#include <i386/misc_protos.h>
#include <pexpert/pexpert.h>
#include <machine/limits.h>
#include <machine/commpage.h>
#include <sys/kdebug.h>
#include <i386/tsc.h>
#include <i386/rtclock_protos.h>
#define UI_CPUFREQ_ROUNDING_FACTOR      10000000

int             rtclock_init(void);

uint64_t        tsc_rebase_abs_time = 0;

volatile uint64_t gAcpiLastSleepTscBase = 0;
volatile uint64_t gAcpiLastSleepNanoBase = 0;
volatile uint64_t gAcpiLastWakeTscBase = 0;
volatile uint64_t gAcpiLastWakeNanoBase = 0;

static void     rtc_set_timescale(uint64_t cycles);
static uint64_t rtc_export_speed(uint64_t cycles);

void
rtc_timer_start(void)
{
	  
                                                      
    
	x86_lcpu()->rtcDeadline = EndOfAllTime;
	timer_resync_deadlines();
}

static inline uint32_t
_absolutetime_to_microtime(uint64_t abstime, clock_sec_t *secs, clock_usec_t *microsecs)
{
	uint32_t remain;
	*secs = abstime / (uint64_t)NSEC_PER_SEC;
	remain = (uint32_t)(abstime % (uint64_t)NSEC_PER_SEC);
	*microsecs = remain / NSEC_PER_USEC;
	return remain;
}

static inline void
_absolutetime_to_nanotime(uint64_t abstime, clock_sec_t *secs, clock_usec_t *nanosecs)
{
	*secs = abstime / (uint64_t)NSEC_PER_SEC;
	*nanosecs = (clock_usec_t)(abstime % (uint64_t)NSEC_PER_SEC);
}

  
                                
                                
                                                                              
                                                                                
                                                                             
                                                                                
                                                                                    
  
                                                       
                                         
                                                          
                          
                                                            
  
                                                                          
                                              
  
                                                                                   
                               
   
static inline void
rtc_nanotime_set_commpage(pal_rtc_nanotime_t *rntp)
{
	commpage_set_nanotime(rntp->tsc_base, rntp->ns_base, rntp->scale, rntp->shift);
}

  
                     
  
                                                  
   
static inline void
_rtc_nanotime_init(pal_rtc_nanotime_t *rntp, uint64_t base)
{
	uint64_t        tsc = rdtsc64();

	_pal_rtc_nanotime_store(tsc, base, rntp->scale, rntp->shift, rntp);
}

void
rtc_nanotime_init(uint64_t base)
{
	gAcpiLastSleepTscBase = pal_rtc_nanotime_info.tsc_base;
	gAcpiLastSleepNanoBase = pal_rtc_nanotime_info.ns_base;

	_rtc_nanotime_init(&pal_rtc_nanotime_info, base);

	gAcpiLastWakeTscBase = pal_rtc_nanotime_info.tsc_base;
	gAcpiLastWakeNanoBase = pal_rtc_nanotime_info.ns_base;

	rtc_nanotime_set_commpage(&pal_rtc_nanotime_info);
}

  
                              
  
                                                
                                                   
                               
   
void
rtc_nanotime_init_commpage(void)
{
	spl_t                   s = splclock();

	rtc_nanotime_set_commpage(&pal_rtc_nanotime_info);
	splx(s);
}

  
                     
  
                                                          
           
   
static inline uint64_t
rtc_nanotime_read(void)
{
	return _rtc_nanotime_read(&pal_rtc_nanotime_info);
}

  
                    
  
                                                                        
                                                                            
                                                                     
   
void
rtc_clock_napped(uint64_t base, uint64_t tsc_base)
{
	pal_rtc_nanotime_t      *rntp = &pal_rtc_nanotime_info;
	uint64_t        oldnsecs;
	uint64_t        newnsecs;
	uint64_t        tsc;

	assert(!ml_get_interrupts_enabled());
	tsc = rdtsc64();
	oldnsecs = rntp->ns_base + _rtc_tsc_to_nanoseconds(tsc - rntp->tsc_base, rntp);
	newnsecs = base + _rtc_tsc_to_nanoseconds(tsc - tsc_base, rntp);

	  
                                                                 
                                                     
    
	if (oldnsecs < newnsecs) {
		_pal_rtc_nanotime_store(tsc_base, base, rntp->scale, rntp->shift, rntp);
		rtc_nanotime_set_commpage(rntp);
	}
}

  
                                                                             
                                                                             
                                                                              
                                                                     
   
void
rtc_clock_adjust(uint64_t tsc_base_delta)
{
	pal_rtc_nanotime_t  *rntp = &pal_rtc_nanotime_info;

	assert(!ml_get_interrupts_enabled());
	assert(tsc_base_delta < 100ULL);                      
	_rtc_nanotime_adjust(tsc_base_delta, rntp);
	rtc_nanotime_set_commpage(rntp);
}

  
                    
  
                                                                      
                                                                        
                                                                              
  
                                            
   
void
rtc_sleep_wakeup(
	uint64_t                base)
{
	                                              
	rtc_timer->rtc_config();

	  
                   
                                              
                                         
    
	rtc_nanotime_init(base);
}

void
rtc_decrementer_configure(void)
{
	rtc_timer->rtc_config();
}
  
                                                       
                                                     
   
void
rtclock_early_init(void)
{
	assert(tscFreq);
	rtc_set_timescale(tscFreq);
}

  
                                         
                                                                            
   
int
rtclock_init(void)
{
	uint64_t        cycles;

	assert(!ml_get_interrupts_enabled());

	if (cpu_number() == master_cpu) {
		assert(tscFreq);

		  
                                           
     
		cycles = rtc_export_speed(tscFreq);

		  
                           
                                                               
     
		gPEClockFrequencyInfo.cpu_frequency_min_hz = cycles;
		gPEClockFrequencyInfo.cpu_frequency_max_hz = cycles;

		rtc_timer_init();
		clock_timebase_init();
		ml_init_lock_timeout();
		ml_init_delay_spin_threshold(10);
	}

	                                              
	rtc_timer->rtc_config();
	rtc_timer_start();

	return 1;
}

                  
                                                                 

static void
rtc_set_timescale(uint64_t cycles)
{
	pal_rtc_nanotime_t      *rntp = &pal_rtc_nanotime_info;
	uint32_t    shift = 0;

	                                                                       

	while (cycles <= SLOW_TSC_THRESHOLD) {
		shift++;
		cycles <<= 1;
	}

	rntp->scale = (uint32_t)(((uint64_t)NSEC_PER_SEC << 32) / cycles);

	rntp->shift = shift;

	  
                                                                 
                                                                    
                                                                      
                   
    
	if (tsc_rebase_abs_time == 0) {
		tsc_rebase_abs_time = _rtc_tsc_to_nanoseconds(
			rdtsc64() - tsc_at_boot, rntp);
	}

	rtc_nanotime_init(0);
}

static uint64_t
rtc_export_speed(uint64_t cyc_per_sec)
{
	pal_rtc_nanotime_t      *rntp = &pal_rtc_nanotime_info;
	uint64_t        cycles;

	if (rntp->shift != 0) {
		printf("Slow TSC, rtc_nanotime.shift == %d\n", rntp->shift);
	}

	            
	cycles = ((cyc_per_sec + (UI_CPUFREQ_ROUNDING_FACTOR / 2))
	    / UI_CPUFREQ_ROUNDING_FACTOR)
	    * UI_CPUFREQ_ROUNDING_FACTOR;

	  
                               
    
	if (cycles >= 0x100000000ULL) {
		gPEClockFrequencyInfo.cpu_clock_rate_hz = 0xFFFFFFFFUL;
	} else {
		gPEClockFrequencyInfo.cpu_clock_rate_hz = (unsigned long)cycles;
	}
	gPEClockFrequencyInfo.cpu_frequency_hz = cycles;

	kprintf("[RTCLOCK] frequency %llu (%llu)\n", cycles, cyc_per_sec);
	return cycles;
}

void
clock_get_system_microtime(
	clock_sec_t                     *secs,
	clock_usec_t            *microsecs)
{
	uint64_t        now = rtc_nanotime_read();

	_absolutetime_to_microtime(now, secs, microsecs);
}

void
clock_get_system_nanotime(
	clock_sec_t                     *secs,
	clock_nsec_t            *nanosecs)
{
	uint64_t        now = rtc_nanotime_read();

	_absolutetime_to_nanotime(now, secs, nanosecs);
}

void
clock_gettimeofday_set_commpage(uint64_t abstime, uint64_t sec, uint64_t frac, uint64_t scale, uint64_t tick_per_sec)
{
	commpage_set_timestamp(abstime, sec, frac, scale, tick_per_sec);
}

void
clock_timebase_info(
	mach_timebase_info_t    info)
{
	info->numer = info->denom =  1;
}

  
                                    
   
void
rtclock_intr(
	x86_saved_state_t       *tregs)
{
	uint64_t        rip;
	boolean_t       user_mode = FALSE;

	assert(get_preemption_level() > 0);
	assert(!ml_get_interrupts_enabled());

	if (is_saved_state64(tregs) == TRUE) {
		x86_saved_state64_t     *regs;

		regs = saved_state64(tregs);

		if (regs->isf.cs & 0x03) {
			user_mode = TRUE;
		}
		rip = regs->isf.rip;
	} else {
		x86_saved_state32_t     *regs;

		regs = saved_state32(tregs);

		if (regs->cs & 0x03) {
			user_mode = TRUE;
		}
		rip = regs->eip;
	}

	                             
	timer_intr(user_mode, rip);
}


  
                                      
   

uint64_t
setPop(uint64_t time)
{
	uint64_t        now;
	uint64_t        pop;

	                                                                
	if (time == 0 || time == EndOfAllTime) {
		time = EndOfAllTime;
		now = 0;
		pop = rtc_timer->rtc_set(0, 0);
	} else {
		now = rtc_nanotime_read();                                   
		pop = rtc_timer->rtc_set(time, now);
	}

	                                               
	x86_lcpu()->rtcDeadline = time;
	x86_lcpu()->rtcPop      = pop;

	return pop - now;
}

uint64_t
mach_absolute_time(void)
{
	return rtc_nanotime_read();
}

uint64_t
mach_approximate_time(void)
{
	return rtc_nanotime_read();
}

void
clock_interval_to_absolutetime_interval(
	uint32_t                interval,
	uint32_t                scale_factor,
	uint64_t                *result)
{
	*result = (uint64_t)interval * scale_factor;
}

void
absolutetime_to_microtime(
	uint64_t                        abstime,
	clock_sec_t                     *secs,
	clock_usec_t            *microsecs)
{
	_absolutetime_to_microtime(abstime, secs, microsecs);
}

void
nanotime_to_absolutetime(
	clock_sec_t                     secs,
	clock_nsec_t            nanosecs,
	uint64_t                        *result)
{
	*result = ((uint64_t)secs * NSEC_PER_SEC) + nanosecs;
}

void
absolutetime_to_nanoseconds(
	uint64_t                abstime,
	uint64_t                *result)
{
	*result = abstime;
}

void
nanoseconds_to_absolutetime(
	uint64_t                nanoseconds,
	uint64_t                *result)
{
	*result = nanoseconds;
}

void
machine_delay_until(
	uint64_t interval,
	uint64_t                deadline)
{
	(void)interval;
	while (mach_absolute_time() < deadline) {
		cpu_pause();
	}
}
