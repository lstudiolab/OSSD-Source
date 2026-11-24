/*
 * Copyright (c) 2008 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
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
 * @APPLE_LICENSE_HEADER_END@
 */

#include <sys/time.h>
#include <mach/mach_time.h>
#include <machine/cpu_capabilities.h>
#include <os/overflow.h>
#include <kern/arithmetic_128.h>

int __commpage_gettimeofday(struct timeval *);

__attribute__((visibility("hidden")))
int __commpage_gettimeofday_internal(struct timeval *tp, uint64_t *tbr_out);

int
__commpage_gettimeofday(struct timeval *tp)
{
	return __commpage_gettimeofday_internal(tp, NULL);
}

int
__commpage_gettimeofday_internal(struct timeval *tp, uint64_t *tbr_out)
{
	uint64_t now, over;
	uint64_t delta, frac;
	uint64_t TimeStamp_tick;
	uint64_t TimeStamp_sec;
	uint64_t TimeStamp_frac;
	uint64_t Tick_scale;
	uint64_t Ticks_per_sec;

	volatile uint64_t *gtod_TimeStamp_tick_p;
	volatile uint64_t *gtod_TimeStamp_sec_p;
	volatile uint64_t *gtod_TimeStamp_frac_p;
	volatile uint64_t *gtod_Ticks_scale_p;
	volatile uint64_t *gtod_Ticks_per_sec_p;

	COMM_PAGE_SLOT_TYPE(new_commpage_timeofday_data_t) commpage_timeofday_datap =
	    COMM_PAGE_SLOT(new_commpage_timeofday_data_t, NEWTIMEOFDAY_DATA);

	gtod_TimeStamp_tick_p = &commpage_timeofday_datap->TimeStamp_tick;
	gtod_TimeStamp_sec_p = &commpage_timeofday_datap->TimeStamp_sec;
	gtod_TimeStamp_frac_p = &commpage_timeofday_datap->TimeStamp_frac;
	gtod_Ticks_scale_p = &commpage_timeofday_datap->Ticks_scale;
	gtod_Ticks_per_sec_p = &commpage_timeofday_datap->Ticks_per_sec;

	do {
		TimeStamp_tick = *gtod_TimeStamp_tick_p;
		  
                                                                         
                                                                        
                       
     
		now = mach_absolute_time();
		TimeStamp_sec = *gtod_TimeStamp_sec_p;
		TimeStamp_frac = *gtod_TimeStamp_frac_p;
		Tick_scale = *gtod_Ticks_scale_p;
		Ticks_per_sec = *gtod_Ticks_per_sec_p;
		  
                                                                                     
                                                                    
     
#if     (__ARM_ARCH__ >= 7)
		__asm__ volatile ("dmb ishld" ::: "memory");
#endif
	} while (TimeStamp_tick != *gtod_TimeStamp_tick_p);

	if (TimeStamp_tick == 0) {
		return 1;
	}

	delta = now - TimeStamp_tick;

	                                             
	if (delta >= Ticks_per_sec) {
		return 1;
	}

	if (TimeStamp_sec > __LONG_MAX__) {
		return 1;
	}

	tp->tv_sec = (__darwin_time_t)TimeStamp_sec;

	over = multi_overflow(Tick_scale, delta);
	if (over) {
		tp->tv_sec += over;
	}

	                                                                      
	frac = TimeStamp_frac;
	frac += Tick_scale * delta;
	if (TimeStamp_frac > frac) {
		tp->tv_sec++;
	}

	  
                                               
                                     
    
	tp->tv_usec = ((uint64_t)1000000 * (uint32_t)(frac >> 32)) >> 32;

	if (tbr_out) {
		*tbr_out = now;
	}

	return 0;
}
