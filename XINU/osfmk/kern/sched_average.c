/*
 * Copyright (c) 2000-2007 Apple Computer, Inc. All rights reserved.
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
  
   
  
                               
             
  
                            
   

#include <mach/mach_types.h>

#include <kern/sched.h>
#include <kern/assert.h>
#include <kern/processor.h>
#include <kern/thread.h>
#if CONFIG_TELEMETRY
#include <kern/telemetry.h>
#endif
#include <kern/zalloc_internal.h>

#include <sys/kdebug.h>

uint32_t        avenrun[3] = {0, 0, 0};
uint32_t        mach_factor[3] = {0, 0, 0};

uint32_t        sched_load_average, sched_mach_factor;

#if defined(CONFIG_SCHED_TIMESHARE_CORE)
  
                                                               
   
#define base(n)         ((n) << SCHED_TICK_SHIFT)
#define frac(n)         (((base(n) - 1) * LOAD_SCALE) /	base(n))

static uint32_t         fract[3] = {
	frac(5),                                      
	frac(30),                                      
	frac(60),                                     
};

#undef base
#undef frac

#endif                                  

static unsigned int             sched_nrun;

typedef void    (*sched_avg_comp_t)(
	void                    *param);

static struct sched_average {
	sched_avg_comp_t        comp;
	void                    *param;
	int                     period;                 
	uint64_t                deadline;
} sched_average[] = {
	{ compute_averunnable, &sched_nrun, 5, 0 },
	{ compute_stack_target, NULL, 5, 1 },
	{ compute_pageout_gc_throttle, NULL, 1, 0 },
	{ compute_pmap_gc_throttle, NULL, 60, 0 },
	{ compute_zone_working_set_size, NULL, ZONE_WSS_UPDATE_PERIOD, 0 },
#if CONFIG_TELEMETRY
	{ compute_telemetry, NULL, 1, 0 },
#endif
	{ NULL, NULL, 0, 0 }
};

typedef struct sched_average    *sched_average_t;

  
                                       
  
                                                                          
                                                                      
                                                                          
                                                                          
                                                                            
  
                                                                          
                                                                            
                                                                          
                                                                             
                        
  
                                         
                                   
                              
  
                                                                           
                                                                             
                                                                         
                                                 
                                                                           
                          
                                                                            
                                                                     
                                                                         
                                                            
   

                                                                         
uint32_t sched_load[TH_BUCKET_MAX];

  
                                                                        
                                                                         
                                                                              
                                                                        
                                                                      
                                                   
   
#define SCHED_LOAD_EWMA_ALPHA_OLD      6
#define SCHED_LOAD_EWMA_ALPHA_NEW      10
#define SCHED_LOAD_EWMA_ALPHA_SHIFT    4
static_assert((SCHED_LOAD_EWMA_ALPHA_OLD + SCHED_LOAD_EWMA_ALPHA_NEW) == (1ul << SCHED_LOAD_EWMA_ALPHA_SHIFT));

                                                             
#define SCHED_LOAD_EWMA_ROUNDUP(load)   (((load) & (1ul << (SCHED_LOAD_EWMA_ALPHA_SHIFT - 1))) != 0)

                                                             
#define SCHED_LOAD_EWMA_UNSCALE(load)   (((load) >> SCHED_LOAD_EWMA_ALPHA_SHIFT) + SCHED_LOAD_EWMA_ROUNDUP(load))

  
                                                                                                            
   
void
compute_sched_load(void)
{
	  
                                                  
   
                                                                                    
                          
    
	uint32_t ncpus = processor_avail_count;
	uint32_t load_now[TH_BUCKET_MAX];

	load_now[TH_BUCKET_RUN]      = os_atomic_load(&sched_run_buckets[TH_BUCKET_RUN], relaxed);
	load_now[TH_BUCKET_FIXPRI]   = os_atomic_load(&sched_run_buckets[TH_BUCKET_FIXPRI], relaxed);
	load_now[TH_BUCKET_SHARE_FG] = os_atomic_load(&sched_run_buckets[TH_BUCKET_SHARE_FG], relaxed);
	load_now[TH_BUCKET_SHARE_DF] = os_atomic_load(&sched_run_buckets[TH_BUCKET_SHARE_DF], relaxed);
	load_now[TH_BUCKET_SHARE_UT] = os_atomic_load(&sched_run_buckets[TH_BUCKET_SHARE_UT], relaxed);
	load_now[TH_BUCKET_SHARE_BG] = os_atomic_load(&sched_run_buckets[TH_BUCKET_SHARE_BG], relaxed);

	assert(load_now[TH_BUCKET_RUN] >= 0);
	assert(load_now[TH_BUCKET_FIXPRI] >= 0);

	uint32_t nthreads = load_now[TH_BUCKET_RUN];
	uint32_t nfixpri  = load_now[TH_BUCKET_FIXPRI];

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_LOAD) | DBG_FUNC_NONE,
	    load_now[TH_BUCKET_FIXPRI], (load_now[TH_BUCKET_SHARE_FG] + load_now[TH_BUCKET_SHARE_DF]),
	    load_now[TH_BUCKET_SHARE_BG], load_now[TH_BUCKET_SHARE_UT], 0);

	  
                                                                      
                                                        
                                                        
                                                                           
                                                 
    
	uint32_t timeshare_threads = (nthreads - nfixpri);
	for (uint32_t i = TH_BUCKET_SHARE_FG; i <= TH_BUCKET_SHARE_BG; i++) {
		if (load_now[i] > timeshare_threads) {
			load_now[i] = timeshare_threads;
		}
	}

	  
                                                                      
    
	if (load_now[TH_BUCKET_SHARE_DF] <= (ncpus * 2)) {
		load_now[TH_BUCKET_SHARE_FG] += load_now[TH_BUCKET_SHARE_DF];
	} else {
		load_now[TH_BUCKET_SHARE_FG] += (ncpus * 2);
	}

	  
                                                                     
    
	if (load_now[TH_BUCKET_SHARE_UT] <= ncpus) {
		load_now[TH_BUCKET_SHARE_FG] += load_now[TH_BUCKET_SHARE_UT];
		load_now[TH_BUCKET_SHARE_DF] += load_now[TH_BUCKET_SHARE_UT];
	} else {
		load_now[TH_BUCKET_SHARE_FG] += ncpus;
		load_now[TH_BUCKET_SHARE_DF] += ncpus;
	}

	  
                                                                               
    
	if (load_now[TH_BUCKET_SHARE_BG] > 0) {
		load_now[TH_BUCKET_SHARE_FG] += 1;
		load_now[TH_BUCKET_SHARE_DF] += 1;
		load_now[TH_BUCKET_SHARE_UT] += 1;
	}

	  
                                                     
                                                                      
                                                            
   
                                                    
    

	for (uint32_t i = TH_BUCKET_SHARE_FG; i <= TH_BUCKET_SHARE_BG; i++) {
		uint32_t bucket_load = 0;

		if (load_now[i] > ncpus) {
			                                          
			if (ncpus > 1) {
				bucket_load = load_now[i] / ncpus;
			} else {
				bucket_load = load_now[i];
			}

			if (bucket_load > MAX_LOAD) {
				bucket_load = MAX_LOAD;
			}
		}
		                                                                                                
		sched_load[i] = (sched_load[i] * SCHED_LOAD_EWMA_ALPHA_OLD) + ((bucket_load << SCHED_LOAD_EWMA_ALPHA_SHIFT) * SCHED_LOAD_EWMA_ALPHA_NEW);
		sched_load[i] = sched_load[i] >> SCHED_LOAD_EWMA_ALPHA_SHIFT;
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_SCHED_LOAD_EFFECTIVE) | DBG_FUNC_NONE,
	    SCHED_LOAD_EWMA_UNSCALE(sched_load[TH_BUCKET_SHARE_FG]), SCHED_LOAD_EWMA_UNSCALE(sched_load[TH_BUCKET_SHARE_DF]),
	    SCHED_LOAD_EWMA_UNSCALE(sched_load[TH_BUCKET_SHARE_UT]), SCHED_LOAD_EWMA_UNSCALE(sched_load[TH_BUCKET_SHARE_BG]), 0);
}

void
compute_averages(uint64_t stdelta)
{
	uint32_t nthreads = os_atomic_load(&sched_run_buckets[TH_BUCKET_RUN], relaxed) - 1;
	uint32_t ncpus = processor_avail_count;

	                                                             
	for (uint32_t i = TH_BUCKET_SHARE_FG; i <= TH_BUCKET_SHARE_BG; i++) {
		uint32_t bucket_load = SCHED_LOAD_EWMA_UNSCALE(sched_load[i]);
		uint32_t shift = sched_fixed_shift - sched_load_shifts[bucket_load];

		if (shift > SCHED_PRI_SHIFT_MAX) {
			sched_pri_shifts[i] = INT8_MAX;
		} else {
			sched_pri_shifts[i] = shift;
		}
	}

	  
                                                                  
    
	sched_nrun = nthreads;

	  
                                                 
                                       
    
	uint32_t average_now = nthreads * LOAD_SCALE;
	uint32_t factor_now;

	if (nthreads > ncpus) {
		factor_now = (ncpus * LOAD_SCALE) / (nthreads + 1);
	} else {
		factor_now = (ncpus - nthreads) * LOAD_SCALE;
	}

	  
                                                                 
                                                                      
                                                                      
    
	for (uint32_t index = 0; index < stdelta; index++) {
		sched_mach_factor = ((sched_mach_factor << 2) + factor_now) / 5;
		sched_load_average = ((sched_load_average << 2) + average_now) / 5;
	}

	  
                                         
    
	for (uint32_t index = 0; index < stdelta; index++) {
		for (uint32_t i = 0; i < 3; i++) {
			mach_factor[i] = ((mach_factor[i] * fract[i]) +
			    (factor_now * (LOAD_SCALE - fract[i]))) / LOAD_SCALE;

			avenrun[i] = ((avenrun[i] * fract[i]) +
			    (average_now * (LOAD_SCALE - fract[i]))) / LOAD_SCALE;
		}
	}

	  
                                         
    
	uint64_t abstime = mach_absolute_time();

	for (sched_average_t avg = sched_average; avg->comp != NULL; ++avg) {
		if (abstime >= avg->deadline) {
			uint64_t period_abs = (avg->period * sched_one_second_interval);
			uint64_t ninvokes = 1;

			ninvokes += (abstime - avg->deadline) / period_abs;
			ninvokes = MIN(ninvokes, SCHED_TICK_MAX_DELTA);

			for (uint32_t index = 0; index < ninvokes; index++) {
				(*avg->comp)(avg->param);
			}
			avg->deadline = abstime + period_abs;
		}
	}
}
