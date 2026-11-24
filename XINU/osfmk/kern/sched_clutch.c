/*
 * Copyright (c) 2018 Apple Inc. All rights reserved.
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

#include <mach/mach_types.h>
#include <mach/machine.h>
#include <machine/machine_routines.h>
#include <machine/sched_param.h>
#include <machine/machine_cpu.h>
#include <kern/kern_types.h>
#include <kern/debug.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/processor.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/sched_clutch.h>
#include <machine/atomic.h>
#include <kern/sched_clutch.h>
#include <sys/kdebug.h>

#if CONFIG_SCHED_EDGE
#include <kern/sched_amp_common.h>
#endif                        

#if CONFIG_SCHED_CLUTCH

                                             

                                     
static void sched_clutch_root_init(sched_clutch_root_t, processor_set_t);
static void sched_clutch_root_bucket_init(sched_clutch_root_bucket_t, sched_bucket_t, bool);
static void sched_clutch_root_pri_update(sched_clutch_root_t);
static void sched_clutch_root_urgency_inc(sched_clutch_root_t, thread_t);
static void sched_clutch_root_urgency_dec(sched_clutch_root_t, thread_t);

__enum_decl(sched_clutch_highest_root_bucket_type_t, uint32_t, {
	SCHED_CLUTCH_HIGHEST_ROOT_BUCKET_NONE           = 0,
	SCHED_CLUTCH_HIGHEST_ROOT_BUCKET_UNBOUND_ONLY   = 1,
	SCHED_CLUTCH_HIGHEST_ROOT_BUCKET_ALL            = 2,
});

static sched_clutch_root_bucket_t sched_clutch_root_highest_root_bucket(sched_clutch_root_t, uint64_t, sched_clutch_highest_root_bucket_type_t);

#if CONFIG_SCHED_EDGE
                                                  
static boolean_t sched_clutch_root_foreign_empty(sched_clutch_root_t);
static thread_t sched_clutch_root_highest_foreign_thread_remove(sched_clutch_root_t);
#endif                        

                                            
static uint64_t sched_clutch_root_bucket_deadline_calculate(sched_clutch_root_bucket_t, uint64_t);
static void sched_clutch_root_bucket_deadline_update(sched_clutch_root_bucket_t, sched_clutch_root_t, uint64_t);

                                                    
__options_decl(sched_clutch_bucket_options_t, uint32_t, {
	SCHED_CLUTCH_BUCKET_OPTIONS_NONE        = 0x0,
	                                                 
	SCHED_CLUTCH_BUCKET_OPTIONS_SAMEPRI_RR  = 0x1,
	                                                          
	SCHED_CLUTCH_BUCKET_OPTIONS_HEADQ       = 0x2,
	                                            
	SCHED_CLUTCH_BUCKET_OPTIONS_TAILQ       = 0x4,
});

                                              
static void sched_clutch_bucket_hierarchy_insert(sched_clutch_root_t, sched_clutch_bucket_t, sched_bucket_t, uint64_t, sched_clutch_bucket_options_t);
static void sched_clutch_bucket_hierarchy_remove(sched_clutch_root_t, sched_clutch_bucket_t, sched_bucket_t, uint64_t, sched_clutch_bucket_options_t);
static boolean_t sched_clutch_bucket_runnable(sched_clutch_bucket_t, sched_clutch_root_t, uint64_t, sched_clutch_bucket_options_t);
static boolean_t sched_clutch_bucket_update(sched_clutch_bucket_t, sched_clutch_root_t, uint64_t, sched_clutch_bucket_options_t);
static void sched_clutch_bucket_empty(sched_clutch_bucket_t, sched_clutch_root_t, uint64_t, sched_clutch_bucket_options_t);
static uint8_t sched_clutch_bucket_pri_calculate(sched_clutch_bucket_t, uint64_t);

                                                     
static void sched_clutch_bucket_group_cpu_usage_update(sched_clutch_bucket_group_t, uint64_t);
static void sched_clutch_bucket_group_cpu_adjust(sched_clutch_bucket_group_t, uint8_t);
static void sched_clutch_bucket_group_timeshare_update(sched_clutch_bucket_group_t, sched_clutch_bucket_t, uint64_t);
static uint8_t sched_clutch_bucket_group_pending_ageout(sched_clutch_bucket_group_t, uint64_t);
static uint32_t sched_clutch_bucket_group_run_count_inc(sched_clutch_bucket_group_t);
static uint32_t sched_clutch_bucket_group_run_count_dec(sched_clutch_bucket_group_t);
static uint8_t sched_clutch_bucket_group_interactivity_score_calculate(sched_clutch_bucket_group_t, uint64_t);

                                         
static uint32_t sched_clutch_run_bucket_incr(sched_clutch_t, sched_bucket_t);
static uint32_t sched_clutch_run_bucket_decr(sched_clutch_t, sched_bucket_t);

                                  
static boolean_t sched_clutch_thread_insert(sched_clutch_root_t, thread_t, integer_t);
static void sched_clutch_thread_remove(sched_clutch_root_t, thread_t, uint64_t, sched_clutch_bucket_options_t);
static thread_t sched_clutch_thread_highest_remove(sched_clutch_root_t);

                               
static uint32_t sched_clutch_root_urgency(sched_clutch_root_t);
static uint32_t sched_clutch_root_count_sum(sched_clutch_root_t);
static int sched_clutch_root_priority(sched_clutch_root_t);
static sched_clutch_bucket_t sched_clutch_root_bucket_highest_clutch_bucket(sched_clutch_root_bucket_t);
static boolean_t sched_thread_sched_pri_promoted(thread_t);

#if CONFIG_SCHED_EDGE
                           
static bool sched_edge_pset_available(processor_set_t);
static uint32_t sched_edge_thread_bound_cluster_id(thread_t);
#endif                        

                               
static inline void sched_clutch_hierarchy_locked_assert(sched_clutch_root_t);

extern processor_set_t pset_array[MAX_PSETS];

  
                                                                    
   
#define SCHED_CLUTCH_INVALID_TIME_32 ((uint32_t)~0)
#define SCHED_CLUTCH_INVALID_TIME_64 ((uint64_t)~0)

  
                          
  
                                                                    
                                                                   
                                                                      
                  
  
   
static uint32_t sched_clutch_root_bucket_wcel_us[TH_BUCKET_SCHED_MAX] = {
	SCHED_CLUTCH_INVALID_TIME_32,                               
	0,                                                      
	37500,                                                           
	75000,                                                         
	150000,                                                         
	250000                                                          
};
static uint64_t sched_clutch_root_bucket_wcel[TH_BUCKET_SCHED_MAX] = {0};

  
                         
  
                                                                      
                                                                     
                                                                   
                                                              
                                                              
   

                                                                                       
#define SCHED_CLUTCH_ROOT_BUCKET_WARP_UNUSED    (SCHED_CLUTCH_INVALID_TIME_64)

                                             
static uint32_t sched_clutch_root_bucket_warp_us[TH_BUCKET_SCHED_MAX] = {
	SCHED_CLUTCH_INVALID_TIME_32,                               
	8000,                                                        
	4000,                                                         
	2000,                                                         
	1000,                                                         
	0                                                             
};
static uint64_t sched_clutch_root_bucket_warp[TH_BUCKET_SCHED_MAX] = {0};

  
                       
  
                                                                      
                                                                    
                                                                     
   

#if XNU_TARGET_OS_OSX
static uint32_t sched_clutch_thread_quantum_us[TH_BUCKET_SCHED_MAX] = {
	10000,                                                             
	10000,                                                         
	10000,                                                         
	10000,                                                         
	4000,                                                         
	2000                                                          
};
#else                        
static uint32_t sched_clutch_thread_quantum_us[TH_BUCKET_SCHED_MAX] = {
	10000,                                                             
	10000,                                                         
	8000,                                                         
	6000,                                                         
	4000,                                                         
	2000                                                          
};
#endif                        

static uint64_t sched_clutch_thread_quantum[TH_BUCKET_SCHED_MAX] = {0};

  
                               
  
                                                              
   
static void
sched_clutch_us_to_abstime(uint32_t *us_vals, uint64_t *abstime_vals)
{
	for (int i = 0; i < TH_BUCKET_SCHED_MAX; i++) {
		if (us_vals[i] == SCHED_CLUTCH_INVALID_TIME_32) {
			abstime_vals[i] = SCHED_CLUTCH_INVALID_TIME_64;
		} else {
			clock_interval_to_absolutetime_interval(us_vals[i],
			    NSEC_PER_USEC, &abstime_vals[i]);
		}
	}
}

                                             
#define SCHED_CLUTCH_DBG_THR_COUNT_PACK(a, b, c)        ((uint64_t)c | ((uint64_t)b << 16) | ((uint64_t)a << 32))

#if DEVELOPMENT || DEBUG

  
                                         
  
                                                                              
                                                                          
                                                                             
                       
   
static inline void
sched_clutch_hierarchy_locked_assert(
	sched_clutch_root_t root_clutch)
{
	pset_assert_locked(root_clutch->scr_pset);
}

#else                           

static inline void
sched_clutch_hierarchy_locked_assert(
	__unused sched_clutch_root_t root_clutch)
{
}

#endif                           

  
                               
  
                                                                    
   
static void
sched_clutch_thr_count_inc(
	uint16_t *thr_count)
{
	if (__improbable(os_inc_overflow(thr_count))) {
		panic("sched_clutch thread count overflowed!");
	}
}

  
                               
  
                                                                     
   
static void
sched_clutch_thr_count_dec(
	uint16_t *thr_count)
{
	if (__improbable(os_dec_overflow(thr_count))) {
		panic("sched_clutch thread count underflowed!");
	}
}

  
                                                                                
                                                                          
                                                                                
                                                                              
   
static uint32_t _Atomic sched_clutch_global_bucket_load[TH_BUCKET_SCHED_MAX];

  
                           
  
                                                      
   
static void
sched_clutch_root_init(
	sched_clutch_root_t root_clutch,
	processor_set_t pset)
{
	root_clutch->scr_thr_count = 0;
	root_clutch->scr_priority = NOPRI;
	root_clutch->scr_urgency = 0;
	root_clutch->scr_pset = pset;
#if CONFIG_SCHED_EDGE
	root_clutch->scr_cluster_id = pset->pset_cluster_id;
#else                        
	root_clutch->scr_cluster_id = 0;
#endif                        

	                                                                                               
	queue_init(&root_clutch->scr_clutch_buckets);

	                                                                                       
	priority_queue_init(&root_clutch->scr_foreign_buckets);
	bzero(&root_clutch->scr_cumulative_run_count, sizeof(root_clutch->scr_cumulative_run_count));
	bitmap_zero(root_clutch->scr_bound_runnable_bitmap, TH_BUCKET_SCHED_MAX);
	bitmap_zero(root_clutch->scr_bound_warp_available, TH_BUCKET_SCHED_MAX);
	priority_queue_init(&root_clutch->scr_bound_root_buckets);

	                                                                       
	priority_queue_init(&root_clutch->scr_unbound_root_buckets);
	bitmap_zero(root_clutch->scr_unbound_runnable_bitmap, TH_BUCKET_SCHED_MAX);
	bitmap_zero(root_clutch->scr_unbound_warp_available, TH_BUCKET_SCHED_MAX);

	                                     
	for (uint32_t i = 0; i < TH_BUCKET_SCHED_MAX; i++) {
		sched_clutch_root_bucket_init(&root_clutch->scr_unbound_buckets[i], i, false);
		sched_clutch_root_bucket_init(&root_clutch->scr_bound_buckets[i], i, true);
	}
}

  
                          
  
                                                                            
                                                                         
                   
  
                                                                        
                                                                           
                                                                 
  
                                                                            
                                                                           
                              
  
   

  
                                  
  
                                   
   
static void
sched_clutch_bucket_runq_init(
	sched_clutch_bucket_runq_t clutch_buckets_rq)
{
	clutch_buckets_rq->scbrq_highq = NOPRI;
	for (uint8_t i = 0; i < BITMAP_LEN(NRQS); i++) {
		clutch_buckets_rq->scbrq_bitmap[i] = 0;
	}
	clutch_buckets_rq->scbrq_count = 0;
	for (int i = 0; i < NRQS; i++) {
		circle_queue_init(&clutch_buckets_rq->scbrq_queues[i]);
	}
}

  
                                   
  
                                            
   
static boolean_t
sched_clutch_bucket_runq_empty(
	sched_clutch_bucket_runq_t clutch_buckets_rq)
{
	return clutch_buckets_rq->scbrq_count == 0;
}

  
                                  
  
                                                          
   
static sched_clutch_bucket_t
sched_clutch_bucket_runq_peek(
	sched_clutch_bucket_runq_t clutch_buckets_rq)
{
	if (clutch_buckets_rq->scbrq_count > 0) {
		circle_queue_t queue = &clutch_buckets_rq->scbrq_queues[clutch_buckets_rq->scbrq_highq];
		return cqe_queue_first(queue, struct sched_clutch_bucket, scb_runqlink);
	} else {
		return NULL;
	}
}

  
                                     
  
                                                                        
   
static void
sched_clutch_bucket_runq_enqueue(
	sched_clutch_bucket_runq_t clutch_buckets_rq,
	sched_clutch_bucket_t clutch_bucket,
	sched_clutch_bucket_options_t options)
{
	circle_queue_t queue = &clutch_buckets_rq->scbrq_queues[clutch_bucket->scb_priority];
	if (circle_queue_empty(queue)) {
		circle_enqueue_tail(queue, &clutch_bucket->scb_runqlink);
		bitmap_set(clutch_buckets_rq->scbrq_bitmap, clutch_bucket->scb_priority);
		if (clutch_bucket->scb_priority > clutch_buckets_rq->scbrq_highq) {
			clutch_buckets_rq->scbrq_highq = clutch_bucket->scb_priority;
		}
	} else {
		if (options & SCHED_CLUTCH_BUCKET_OPTIONS_HEADQ) {
			circle_enqueue_head(queue, &clutch_bucket->scb_runqlink);
		} else {
			  
                                                                   
                                       
      
			circle_enqueue_tail(queue, &clutch_bucket->scb_runqlink);
		}
	}
	clutch_buckets_rq->scbrq_count++;
}

  
                                    
  
                                        
   
static void
sched_clutch_bucket_runq_remove(
	sched_clutch_bucket_runq_t clutch_buckets_rq,
	sched_clutch_bucket_t clutch_bucket)
{
	circle_queue_t queue = &clutch_buckets_rq->scbrq_queues[clutch_bucket->scb_priority];
	circle_dequeue(queue, &clutch_bucket->scb_runqlink);
	assert(clutch_buckets_rq->scbrq_count > 0);
	clutch_buckets_rq->scbrq_count--;
	if (circle_queue_empty(queue)) {
		bitmap_clear(clutch_buckets_rq->scbrq_bitmap, clutch_bucket->scb_priority);
		clutch_buckets_rq->scbrq_highq = bitmap_first(clutch_buckets_rq->scbrq_bitmap, NRQS);
	}
}

static void
sched_clutch_bucket_runq_rotate(
	sched_clutch_bucket_runq_t clutch_buckets_rq,
	sched_clutch_bucket_t clutch_bucket)
{
	circle_queue_t queue = &clutch_buckets_rq->scbrq_queues[clutch_bucket->scb_priority];
	assert(clutch_bucket == cqe_queue_first(queue, struct sched_clutch_bucket, scb_runqlink));
	circle_queue_rotate_head_forward(queue);
}

  
                                  
  
                                      
   
static void
sched_clutch_root_bucket_init(
	sched_clutch_root_bucket_t root_bucket,
	sched_bucket_t bucket,
	bool bound_root_bucket)
{
	root_bucket->scrb_bucket = bucket;
	if (bound_root_bucket) {
		                                                               
		root_bucket->scrb_bound = true;
		run_queue_init(&root_bucket->scrb_bound_thread_runq);
	} else {
		  
                                                                         
                                          
     
		root_bucket->scrb_bound = false;
		sched_clutch_bucket_runq_init(&root_bucket->scrb_clutch_buckets);
	}
	priority_queue_entry_init(&root_bucket->scrb_pqlink);
	root_bucket->scrb_pqlink.deadline = SCHED_CLUTCH_INVALID_TIME_64;
	root_bucket->scrb_warped_deadline = 0;
	root_bucket->scrb_warp_remaining = sched_clutch_root_bucket_warp[root_bucket->scrb_bucket];
	root_bucket->scrb_starvation_avoidance = false;
	root_bucket->scrb_starvation_ts = 0;
}

  
                                               
  
                                                                              
                                           
  
                                                                               
                                                                                        
                                                                                            
                                                                                    
                                                                                     
                                                                                   
                                      
  
                                                                                  
                                                                                   
                                         
   


  
                                             
  
                                                                                          
   
static bool
sched_clutch_root_unbound_select_aboveui(
	sched_clutch_root_t root_clutch)
{
	if (bitmap_test(root_clutch->scr_unbound_runnable_bitmap, TH_BUCKET_FIXPRI)) {
		sched_clutch_root_bucket_t root_bucket_aboveui = &root_clutch->scr_unbound_buckets[TH_BUCKET_FIXPRI];
		sched_clutch_root_bucket_t root_bucket_sharefg = &root_clutch->scr_unbound_buckets[TH_BUCKET_SHARE_FG];
		if (!bitmap_test(root_clutch->scr_unbound_runnable_bitmap, TH_BUCKET_SHARE_FG)) {
			                                                                                        
			return true;
		}
		sched_clutch_bucket_t clutch_bucket_aboveui = sched_clutch_root_bucket_highest_clutch_bucket(root_bucket_aboveui);
		sched_clutch_bucket_t clutch_bucket_sharefg = sched_clutch_root_bucket_highest_clutch_bucket(root_bucket_sharefg);
		if (clutch_bucket_aboveui->scb_priority >= clutch_bucket_sharefg->scb_priority) {
			return true;
		}
	}
	return false;
}

  
                                           
  
                                                                                        
   
static bool
sched_clutch_root_bound_select_aboveui(
	sched_clutch_root_t root_clutch)
{
	sched_clutch_root_bucket_t root_bucket_aboveui = &root_clutch->scr_bound_buckets[TH_BUCKET_FIXPRI];
	sched_clutch_root_bucket_t root_bucket_sharefg = &root_clutch->scr_bound_buckets[TH_BUCKET_SHARE_FG];
	if (root_bucket_aboveui->scrb_bound_thread_runq.count == 0) {
		return false;
	}
	return root_bucket_aboveui->scrb_bound_thread_runq.highq >= root_bucket_sharefg->scrb_bound_thread_runq.highq;
}

  
                                          
  
                                                               
                                                                       
                                                                 
                                                                 
                                                                   
                                                                        
   
static sched_clutch_root_bucket_t
sched_clutch_root_highest_root_bucket(
	sched_clutch_root_t root_clutch,
	uint64_t timestamp,
	sched_clutch_highest_root_bucket_type_t type)
{
	sched_clutch_hierarchy_locked_assert(root_clutch);
	int highest_runnable_bucket = -1;
	if (type == SCHED_CLUTCH_HIGHEST_ROOT_BUCKET_UNBOUND_ONLY) {
		highest_runnable_bucket = bitmap_lsb_first(root_clutch->scr_unbound_runnable_bitmap, TH_BUCKET_SCHED_MAX);
	} else {
		int highest_unbound_runnable_bucket = bitmap_lsb_first(root_clutch->scr_unbound_runnable_bitmap, TH_BUCKET_SCHED_MAX);
		int highest_bound_runnable_bucket = bitmap_lsb_first(root_clutch->scr_bound_runnable_bitmap, TH_BUCKET_SCHED_MAX);
		highest_runnable_bucket = (highest_bound_runnable_bucket != -1) ? ((highest_unbound_runnable_bucket != -1) ? MIN(highest_bound_runnable_bucket, highest_unbound_runnable_bucket) : highest_bound_runnable_bucket) : highest_unbound_runnable_bucket;
	}

	if (highest_runnable_bucket == -1) {
		return NULL;
	}

	                                                                                                       
	bool unbound_aboveui = sched_clutch_root_unbound_select_aboveui(root_clutch);
	if (type == SCHED_CLUTCH_HIGHEST_ROOT_BUCKET_UNBOUND_ONLY) {
		if (unbound_aboveui) {
			return &root_clutch->scr_unbound_buckets[TH_BUCKET_FIXPRI];
		}
		                                                       
	} else {
		bool bound_aboveui = sched_clutch_root_bound_select_aboveui(root_clutch);
		sched_clutch_root_bucket_t unbound_aboveui_root_bucket = &root_clutch->scr_unbound_buckets[TH_BUCKET_FIXPRI];
		sched_clutch_root_bucket_t bound_aboveui_root_bucket = &root_clutch->scr_bound_buckets[TH_BUCKET_FIXPRI];

		if (unbound_aboveui && bound_aboveui) {
			  
                                                                                                        
                                                    
        
			int bound_aboveui_pri = root_clutch->scr_bound_buckets[TH_BUCKET_FIXPRI].scrb_bound_thread_runq.highq;
			sched_clutch_bucket_t clutch_bucket = sched_clutch_root_bucket_highest_clutch_bucket(unbound_aboveui_root_bucket);
			int unbound_aboveui_pri = priority_queue_max_sched_pri(&clutch_bucket->scb_clutchpri_prioq);
			return (bound_aboveui_pri >= unbound_aboveui_pri) ? bound_aboveui_root_bucket : unbound_aboveui_root_bucket;
		}
		if (unbound_aboveui) {
			return unbound_aboveui_root_bucket;
		}
		if (bound_aboveui) {
			return bound_aboveui_root_bucket;
		}
		                                                       
	}

	  
                                                                                  
                                                                                  
                                                                                
                                                           
    

	                                       
	sched_clutch_root_bucket_t edf_bucket = NULL;
	sched_clutch_root_bucket_t warp_bucket = NULL;
	int warp_bucket_index = -1;

evaluate_root_buckets:
	if (type == SCHED_CLUTCH_HIGHEST_ROOT_BUCKET_UNBOUND_ONLY) {
		edf_bucket = priority_queue_min(&root_clutch->scr_unbound_root_buckets, struct sched_clutch_root_bucket, scrb_pqlink);
	} else {
		sched_clutch_root_bucket_t unbound_bucket = priority_queue_min(&root_clutch->scr_unbound_root_buckets, struct sched_clutch_root_bucket, scrb_pqlink);
		sched_clutch_root_bucket_t bound_bucket = priority_queue_min(&root_clutch->scr_bound_root_buckets, struct sched_clutch_root_bucket, scrb_pqlink);
		if (bound_bucket && unbound_bucket) {
			                                                                                              
			edf_bucket = (bound_bucket->scrb_pqlink.deadline <= unbound_bucket->scrb_pqlink.deadline) ? bound_bucket : unbound_bucket;
		} else {
			edf_bucket = (bound_bucket) ? bound_bucket : unbound_bucket;
		}
	}
	  
                                                                                                                 
                                                                                                                 
                                                                                                               
                    
    
	bitmap_t *warp_available_bitmap = (edf_bucket->scrb_bound) ? (root_clutch->scr_bound_warp_available) : (root_clutch->scr_unbound_warp_available);
	warp_bucket_index = bitmap_lsb_first(warp_available_bitmap, TH_BUCKET_SCHED_MAX);

	if ((warp_bucket_index == -1) || (warp_bucket_index >= edf_bucket->scrb_bucket)) {
		                                                                           
		if (edf_bucket->scrb_starvation_avoidance) {
			  
                                                                                                      
                                                                                
     
                                                                                                      
                                                                                                         
                                
      
			uint64_t starvation_window = sched_clutch_thread_quantum[edf_bucket->scrb_bucket] / root_clutch->scr_pset->online_processor_count;
			if (timestamp < (edf_bucket->scrb_starvation_ts + starvation_window)) {
				return edf_bucket;
			} else {
				                                                                              
				edf_bucket->scrb_starvation_avoidance = false;
				edf_bucket->scrb_starvation_ts = 0;
				sched_clutch_root_bucket_deadline_update(edf_bucket, root_clutch, timestamp);
			}
			goto evaluate_root_buckets;
		}

		                                                                                          
		if (highest_runnable_bucket < edf_bucket->scrb_bucket) {
			                                                                                                           
			edf_bucket->scrb_starvation_avoidance = true;
			edf_bucket->scrb_starvation_ts = timestamp;
		} else {
			                                                                                       
			sched_clutch_root_bucket_deadline_update(edf_bucket, root_clutch, timestamp);
			edf_bucket->scrb_warp_remaining = sched_clutch_root_bucket_warp[edf_bucket->scrb_bucket];
			edf_bucket->scrb_warped_deadline = SCHED_CLUTCH_ROOT_BUCKET_WARP_UNUSED;
			if (edf_bucket->scrb_bound) {
				bitmap_set(root_clutch->scr_bound_warp_available, edf_bucket->scrb_bucket);
			} else {
				bitmap_set(root_clutch->scr_unbound_warp_available, edf_bucket->scrb_bucket);
			}
		}
		return edf_bucket;
	}

	  
                                                                             
                                                             
    
	warp_bucket = (edf_bucket->scrb_bound) ? &root_clutch->scr_bound_buckets[warp_bucket_index] : &root_clutch->scr_unbound_buckets[warp_bucket_index];
	if (warp_bucket->scrb_warped_deadline == SCHED_CLUTCH_ROOT_BUCKET_WARP_UNUSED) {
		                                                                                               
		warp_bucket->scrb_warped_deadline = timestamp + warp_bucket->scrb_warp_remaining;
		sched_clutch_root_bucket_deadline_update(warp_bucket, root_clutch, timestamp);
		return warp_bucket;
	}
	if (warp_bucket->scrb_warped_deadline > timestamp) {
		                                                                         
		sched_clutch_root_bucket_deadline_update(warp_bucket, root_clutch, timestamp);
		return warp_bucket;
	}

	                                                                           
                                                                            
                                
    
	warp_bucket->scrb_warp_remaining = 0;
	if (warp_bucket->scrb_bound) {
		bitmap_clear(root_clutch->scr_bound_warp_available, warp_bucket->scrb_bucket);
	} else {
		bitmap_clear(root_clutch->scr_unbound_warp_available, warp_bucket->scrb_bucket);
	}
	goto evaluate_root_buckets;
}

  
                                                
  
                                                          
   
static uint64_t
sched_clutch_root_bucket_deadline_calculate(
	sched_clutch_root_bucket_t root_bucket,
	uint64_t timestamp)
{
	                                                                         
	if (root_bucket->scrb_bucket < TH_BUCKET_SHARE_FG) {
		return 0;
	}

	                                                                                               
	return timestamp + sched_clutch_root_bucket_wcel[root_bucket->scrb_bucket];
}

  
                                             
  
                                                                         
                                                                       
         
   
static void
sched_clutch_root_bucket_deadline_update(
	sched_clutch_root_bucket_t root_bucket,
	sched_clutch_root_t root_clutch,
	uint64_t timestamp)
{
	if (root_bucket->scrb_bucket == TH_BUCKET_FIXPRI) {
		                                                                                   
		return;
	}

	uint64_t old_deadline = root_bucket->scrb_pqlink.deadline;
	uint64_t new_deadline = sched_clutch_root_bucket_deadline_calculate(root_bucket, timestamp);
	if (__improbable(old_deadline > new_deadline)) {
		panic("old_deadline (%llu) > new_deadline (%llu); root_bucket (%d); timestamp (%llu)", old_deadline, new_deadline, root_bucket->scrb_bucket, timestamp);
	}
	if (old_deadline != new_deadline) {
		root_bucket->scrb_pqlink.deadline = new_deadline;
		struct priority_queue_deadline_min *prioq = (root_bucket->scrb_bound) ? &root_clutch->scr_bound_root_buckets : &root_clutch->scr_unbound_root_buckets;
		priority_queue_entry_increased(prioq, &root_bucket->scrb_pqlink);
	}
}

  
                                      
  
                                                                     
                                                              
   
static void
sched_clutch_root_bucket_runnable(
	sched_clutch_root_bucket_t root_bucket,
	sched_clutch_root_t root_clutch,
	uint64_t timestamp)
{
	                                      
	bitmap_t *runnable_bitmap = (root_bucket->scrb_bound) ? root_clutch->scr_bound_runnable_bitmap : root_clutch->scr_unbound_runnable_bitmap;
	bitmap_set(runnable_bitmap, root_bucket->scrb_bucket);

	if (root_bucket->scrb_bucket == TH_BUCKET_FIXPRI) {
		                                                                                                    
		return;
	}

	if (root_bucket->scrb_starvation_avoidance == false) {
		  
                                                                                                      
                                                                                                         
                        
     
		root_bucket->scrb_pqlink.deadline = sched_clutch_root_bucket_deadline_calculate(root_bucket, timestamp);
	}
	struct priority_queue_deadline_min *prioq = (root_bucket->scrb_bound) ? &root_clutch->scr_bound_root_buckets : &root_clutch->scr_unbound_root_buckets;
	priority_queue_insert(prioq, &root_bucket->scrb_pqlink);
	if (root_bucket->scrb_warp_remaining) {
		                                                                                                  
		bitmap_t *warp_bitmap = (root_bucket->scrb_bound) ? root_clutch->scr_bound_warp_available : root_clutch->scr_unbound_warp_available;
		bitmap_set(warp_bitmap, root_bucket->scrb_bucket);
	}
}

  
                                   
  
                                                             
                                                              
   
static void
sched_clutch_root_bucket_empty(
	sched_clutch_root_bucket_t root_bucket,
	sched_clutch_root_t root_clutch,
	uint64_t timestamp)
{
	bitmap_t *runnable_bitmap = (root_bucket->scrb_bound) ? root_clutch->scr_bound_runnable_bitmap : root_clutch->scr_unbound_runnable_bitmap;
	bitmap_clear(runnable_bitmap, root_bucket->scrb_bucket);

	if (root_bucket->scrb_bucket == TH_BUCKET_FIXPRI) {
		                                                                                                    
		return;
	}

	struct priority_queue_deadline_min *prioq = (root_bucket->scrb_bound) ? &root_clutch->scr_bound_root_buckets : &root_clutch->scr_unbound_root_buckets;
	priority_queue_remove(prioq, &root_bucket->scrb_pqlink);

	bitmap_t *warp_bitmap = (root_bucket->scrb_bound) ? root_clutch->scr_bound_warp_available : root_clutch->scr_unbound_warp_available;
	bitmap_clear(warp_bitmap, root_bucket->scrb_bucket);

	if (root_bucket->scrb_warped_deadline > timestamp) {
		  
                                                                 
                                                                
                                                               
                                                               
                      
     
		root_bucket->scrb_warp_remaining = root_bucket->scrb_warped_deadline - timestamp;
	} else if (root_bucket->scrb_warped_deadline != SCHED_CLUTCH_ROOT_BUCKET_WARP_UNUSED) {
		  
                                                                        
                                                                
     
		root_bucket->scrb_warp_remaining = 0;
	}
}

static int
sched_clutch_global_bucket_load_get(
	sched_bucket_t bucket)
{
	return (int)os_atomic_load(&sched_clutch_global_bucket_load[bucket], relaxed);
}

  
                                 
  
                                                                      
         
  
                                                                                
                                                                               
                                                                             
                                                                         
                                                                          
                                                                           
                                                                            
                                                                          
                           
   
static void
sched_clutch_root_pri_update(
	sched_clutch_root_t root_clutch)
{
	sched_clutch_hierarchy_locked_assert(root_clutch);
	int16_t root_bound_pri = NOPRI;
	int16_t root_unbound_pri = NOPRI;

	if (bitmap_lsb_first(root_clutch->scr_bound_runnable_bitmap, TH_BUCKET_SCHED_MAX) == -1) {
		goto root_pri_update_unbound;
	}
	sched_clutch_root_bucket_t root_bucket_bound = NULL;
	if (sched_clutch_root_bound_select_aboveui(root_clutch)) {
		root_bucket_bound = &root_clutch->scr_bound_buckets[TH_BUCKET_FIXPRI];
	} else {
		int root_bucket_index = bitmap_lsb_next(root_clutch->scr_bound_runnable_bitmap, TH_BUCKET_SCHED_MAX, TH_BUCKET_FIXPRI);
		assert(root_bucket_index != -1);
		root_bucket_bound = &root_clutch->scr_bound_buckets[root_bucket_index];
	}
	root_bound_pri = root_bucket_bound->scrb_bound_thread_runq.highq;

root_pri_update_unbound:
	if (bitmap_lsb_first(root_clutch->scr_unbound_runnable_bitmap, TH_BUCKET_SCHED_MAX) == -1) {
		goto root_pri_update_complete;
	}
	sched_clutch_root_bucket_t root_bucket_unbound = NULL;
	if (sched_clutch_root_unbound_select_aboveui(root_clutch)) {
		root_bucket_unbound = &root_clutch->scr_unbound_buckets[TH_BUCKET_FIXPRI];
	} else {
		int root_bucket_index = bitmap_lsb_next(root_clutch->scr_unbound_runnable_bitmap, TH_BUCKET_SCHED_MAX, TH_BUCKET_FIXPRI);
		assert(root_bucket_index != -1);
		root_bucket_unbound = &root_clutch->scr_unbound_buckets[root_bucket_index];
	}
	                                                                           
	sched_clutch_bucket_t clutch_bucket = sched_clutch_root_bucket_highest_clutch_bucket(root_bucket_unbound);
	root_unbound_pri = priority_queue_max_sched_pri(&clutch_bucket->scb_clutchpri_prioq);

root_pri_update_complete:
	root_clutch->scr_priority = MAX(root_bound_pri, root_unbound_pri);
}

  
                                  
  
                                                                         
                                                                       
                                                                   
                                                  
  
                                         
   
static void
sched_clutch_root_urgency_inc(
	sched_clutch_root_t root_clutch,
	thread_t thread)
{
	if (SCHED(priority_is_urgent)(thread->sched_pri)) {
		root_clutch->scr_urgency++;
	}
}

  
                                  
  
                                                                         
                                                                      
                                                                   
                                                  
  
                                         
   
static void
sched_clutch_root_urgency_dec(
	sched_clutch_root_t root_clutch,
	thread_t thread)
{
	if (SCHED(priority_is_urgent)(thread->sched_pri)) {
		root_clutch->scr_urgency--;
	}
}

  
                                 
  
                                                                       
                                                                  
                                                                        
                                                                     
                                                                         
                                                                     
           
  
                                                                      
                                                                
                                                                
  
                                                                   
                                                                       
                                                                     
                                                                         
                                                                   
                                                        
                                                                       
                                          
  
                                                                          
                                                                         
                                             
   

                                            
#define SCHED_CLUTCH_BUCKET_GROUP_INTERACTIVE_PRI_DEFAULT     (8)
uint8_t sched_clutch_bucket_group_interactive_pri = SCHED_CLUTCH_BUCKET_GROUP_INTERACTIVE_PRI_DEFAULT;

                                                                                                           
uint64_t sched_clutch_bucket_group_adjust_threshold = 0;
#define SCHED_CLUTCH_BUCKET_GROUP_ADJUST_THRESHOLD_USECS      (500000)

                                                        
#define SCHED_CLUTCH_BUCKET_GROUP_ADJUST_RATIO                (10)

  
                                                                         
                                                                              
                                                                       
                               
   
static uint8_t sched_clutch_bucket_group_pri_boost[SCHED_CLUTCH_TG_PRI_MAX] = {
	[SCHED_CLUTCH_TG_PRI_LOW]  = 0,
	[SCHED_CLUTCH_TG_PRI_MED]  = 2,
	[SCHED_CLUTCH_TG_PRI_HIGH] = 4,
};

                                                                     
#define SCHED_CLUTCH_BUCKET_GROUP_BLOCKED_TS_INVALID          (uint64_t)(~0)

                                                                 
#define SCHED_CLUTCH_BUCKET_GROUP_PENDING_INVALID             ((uint64_t)(~0))

  
                                        
  
                                                                             
                                                                               
                                                                          
                                                                                
                                                                  
  
                                                                                  
                                                                                 
                                                        
   
static uint32_t sched_clutch_bucket_group_pending_delta_us[TH_BUCKET_SCHED_MAX] = {
	SCHED_CLUTCH_INVALID_TIME_32,                       
	10000,                                          
	37500,                                          
	75000,                                          
	150000,                                         
	250000,                                         
};
static uint64_t sched_clutch_bucket_group_pending_delta[TH_BUCKET_SCHED_MAX] = {0};

  
                             
  
                                  
   
static void
sched_clutch_bucket_init(
	sched_clutch_bucket_t clutch_bucket,
	sched_clutch_bucket_group_t clutch_bucket_group,
	sched_bucket_t bucket)
{
	bzero(clutch_bucket, sizeof(struct sched_clutch_bucket));

	clutch_bucket->scb_bucket = bucket;
	                                                                                      
	clutch_bucket->scb_priority = 0;
#if CONFIG_SCHED_EDGE
	clutch_bucket->scb_foreign = false;
	priority_queue_entry_init(&clutch_bucket->scb_foreignlink);
#endif                        
	clutch_bucket->scb_group = clutch_bucket_group;
	clutch_bucket->scb_root = NULL;
	priority_queue_init(&clutch_bucket->scb_clutchpri_prioq);
	priority_queue_init(&clutch_bucket->scb_thread_runq);
	queue_init(&clutch_bucket->scb_thread_timeshare_queue);
}

  
                                   
  
                                        
   
static void
sched_clutch_bucket_group_init(
	sched_clutch_bucket_group_t clutch_bucket_group,
	sched_clutch_t clutch,
	sched_bucket_t bucket)
{
	bzero(clutch_bucket_group, sizeof(struct sched_clutch_bucket_group));
	clutch_bucket_group->scbg_bucket = bucket;
	clutch_bucket_group->scbg_clutch = clutch;
	for (int i = 0; i < MAX_PSETS; i++) {
		sched_clutch_bucket_init(&clutch_bucket_group->scbg_clutch_buckets[i], clutch_bucket_group, bucket);
	}
	os_atomic_store(&clutch_bucket_group->scbg_timeshare_tick, 0, relaxed);
	os_atomic_store(&clutch_bucket_group->scbg_pri_shift, INT8_MAX, relaxed);
	os_atomic_store(&clutch_bucket_group->scbg_preferred_cluster, pset0.pset_cluster_id, relaxed);
	  
                                                                                             
                                                                       
    
	clutch_bucket_group->scbg_interactivity_data.scct_count = (sched_clutch_bucket_group_interactive_pri * 2);
	clutch_bucket_group->scbg_interactivity_data.scct_timestamp = 0;
	os_atomic_store(&clutch_bucket_group->scbg_cpu_data.cpu_data.scbcd_cpu_blocked, (clutch_cpu_data_t)sched_clutch_bucket_group_adjust_threshold, relaxed);
#if !__LP64__
	lck_spin_init(&clutch_bucket_group->scbg_stats_lock, &pset_lck_grp, NULL);
#endif                
	clutch_bucket_group->scbg_blocked_data.scct_timestamp = SCHED_CLUTCH_BUCKET_GROUP_BLOCKED_TS_INVALID;
	clutch_bucket_group->scbg_pending_data.scct_timestamp = SCHED_CLUTCH_BUCKET_GROUP_PENDING_INVALID;
}

  
                                        
  
                                                                     
   
void
sched_clutch_init_with_thread_group(
	sched_clutch_t clutch,
	struct thread_group *tg)
{
	os_atomic_store(&clutch->sc_thr_count, 0, relaxed);

	                                       
	for (uint32_t i = 0; i < TH_BUCKET_SCHED_MAX; i++) {
		sched_clutch_bucket_group_init(&(clutch->sc_clutch_groups[i]), clutch, i);
	}

	                              
	clutch->sc_tg = tg;
	os_atomic_store(&clutch->sc_tg_priority, 0, relaxed);
}

  
                         
  
                                                                
   
void
sched_clutch_destroy(
	__unused sched_clutch_t clutch)
{
	assert(os_atomic_load(&clutch->sc_thr_count, relaxed) == 0);
}

#if CONFIG_SCHED_EDGE

  
                                                                                       
                                        
                                         
                                  
  
                                                                                        
                         
                                      
   
static uint32_t ecore_cluster_id = 0;
static uint32_t pcore_cluster_id = 1;

  
                                             
  
                                                                                           
                                                                                         
                                                                                          
                                                                                       
                                                                                           
   

static uint32_t
sched_edge_clutch_bucket_group_preferred_cluster(sched_clutch_bucket_group_t clutch_bucket_group)
{
	return os_atomic_load(&clutch_bucket_group->scbg_preferred_cluster, relaxed);
}

static uint32_t
sched_clutch_bucket_preferred_cluster(sched_clutch_bucket_t clutch_bucket)
{
	return sched_edge_clutch_bucket_group_preferred_cluster(clutch_bucket->scb_group);
}

uint32_t
sched_edge_thread_preferred_cluster(thread_t thread)
{
	if (SCHED_CLUTCH_THREAD_CLUSTER_BOUND(thread)) {
		                                                                          
		return sched_edge_thread_bound_cluster_id(thread);
	}

	sched_clutch_t clutch = sched_clutch_for_thread(thread);
	sched_clutch_bucket_group_t clutch_bucket_group = &clutch->sc_clutch_groups[thread->th_sched_bucket];
	return sched_edge_clutch_bucket_group_preferred_cluster(clutch_bucket_group);
}

  
                                        
  
                                                                                              
                                                                                          
                                                                                         
                                                                                             
                   
  
                                                                                           
                                                                                            
                                                        
   

static void
sched_clutch_bucket_mark_native(sched_clutch_bucket_t clutch_bucket, sched_clutch_root_t root_clutch)
{
	if (clutch_bucket->scb_foreign) {
		clutch_bucket->scb_foreign = false;
		priority_queue_remove(&root_clutch->scr_foreign_buckets, &clutch_bucket->scb_foreignlink);
	}
}

static void
sched_clutch_bucket_mark_foreign(sched_clutch_bucket_t clutch_bucket, sched_clutch_root_t root_clutch)
{
	if (!clutch_bucket->scb_foreign) {
		clutch_bucket->scb_foreign = true;
		priority_queue_entry_set_sched_pri(&root_clutch->scr_foreign_buckets, &clutch_bucket->scb_foreignlink, clutch_bucket->scb_priority, 0);
		priority_queue_insert(&root_clutch->scr_foreign_buckets, &clutch_bucket->scb_foreignlink);
	}
}

  
                                         
  
                                                                            
                                                                            
                                                                                
                                  
  
   

static void
sched_edge_cluster_cumulative_count_incr(sched_clutch_root_t root_clutch, sched_bucket_t bucket)
{
	switch (bucket) {
	case TH_BUCKET_FIXPRI:    os_atomic_inc(&root_clutch->scr_cumulative_run_count[TH_BUCKET_FIXPRI], relaxed); OS_FALLTHROUGH;
	case TH_BUCKET_SHARE_FG:  os_atomic_inc(&root_clutch->scr_cumulative_run_count[TH_BUCKET_SHARE_FG], relaxed); OS_FALLTHROUGH;
	case TH_BUCKET_SHARE_IN:  os_atomic_inc(&root_clutch->scr_cumulative_run_count[TH_BUCKET_SHARE_IN], relaxed); OS_FALLTHROUGH;
	case TH_BUCKET_SHARE_DF:  os_atomic_inc(&root_clutch->scr_cumulative_run_count[TH_BUCKET_SHARE_DF], relaxed); OS_FALLTHROUGH;
	case TH_BUCKET_SHARE_UT:  os_atomic_inc(&root_clutch->scr_cumulative_run_count[TH_BUCKET_SHARE_UT], relaxed); OS_FALLTHROUGH;
	case TH_BUCKET_SHARE_BG:  os_atomic_inc(&root_clutch->scr_cumulative_run_count[TH_BUCKET_SHARE_BG], relaxed); break;
	default:
		panic("Unexpected sched_bucket passed to sched_edge_cluster_cumulative_count_incr()");
	}
}

static void
sched_edge_cluster_cumulative_count_decr(sched_clutch_root_t root_clutch, sched_bucket_t bucket)
{
	switch (bucket) {
	case TH_BUCKET_FIXPRI:    os_atomic_dec(&root_clutch->scr_cumulative_run_count[TH_BUCKET_FIXPRI], relaxed); OS_FALLTHROUGH;
	case TH_BUCKET_SHARE_FG:  os_atomic_dec(&root_clutch->scr_cumulative_run_count[TH_BUCKET_SHARE_FG], relaxed); OS_FALLTHROUGH;
	case TH_BUCKET_SHARE_IN:  os_atomic_dec(&root_clutch->scr_cumulative_run_count[TH_BUCKET_SHARE_IN], relaxed); OS_FALLTHROUGH;
	case TH_BUCKET_SHARE_DF:  os_atomic_dec(&root_clutch->scr_cumulative_run_count[TH_BUCKET_SHARE_DF], relaxed); OS_FALLTHROUGH;
	case TH_BUCKET_SHARE_UT:  os_atomic_dec(&root_clutch->scr_cumulative_run_count[TH_BUCKET_SHARE_UT], relaxed); OS_FALLTHROUGH;
	case TH_BUCKET_SHARE_BG:  os_atomic_dec(&root_clutch->scr_cumulative_run_count[TH_BUCKET_SHARE_BG], relaxed); break;
	default:
		panic("Unexpected sched_bucket passed to sched_edge_cluster_cumulative_count_decr()");
	}
}

uint16_t
sched_edge_cluster_cumulative_count(sched_clutch_root_t root_clutch, sched_bucket_t bucket)
{
	return os_atomic_load(&root_clutch->scr_cumulative_run_count[bucket], relaxed);
}

#endif                        

  
                                         
  
                                                                            
   
static void
sched_clutch_bucket_hierarchy_insert(
	sched_clutch_root_t root_clutch,
	sched_clutch_bucket_t clutch_bucket,
	sched_bucket_t bucket,
	uint64_t timestamp,
	sched_clutch_bucket_options_t options)
{
	sched_clutch_hierarchy_locked_assert(root_clutch);
	if (bucket > TH_BUCKET_FIXPRI) {
		                                                                                                                      
		enqueue_tail(&root_clutch->scr_clutch_buckets, &clutch_bucket->scb_listlink);
	}
#if CONFIG_SCHED_EDGE
	                                                                                           
	uint32_t preferred_cluster = sched_clutch_bucket_preferred_cluster(clutch_bucket);
	if (pset_type_for_id(preferred_cluster) != pset_type_for_id(root_clutch->scr_cluster_id)) {
		sched_clutch_bucket_mark_foreign(clutch_bucket, root_clutch);
	}
#endif                        
	sched_clutch_root_bucket_t root_bucket = &root_clutch->scr_unbound_buckets[bucket];

	                                                                                                                
	if (sched_clutch_bucket_runq_empty(&root_bucket->scrb_clutch_buckets)) {
		sched_clutch_root_bucket_runnable(root_bucket, root_clutch, timestamp);
	}

	                                                                                         
	sched_clutch_bucket_runq_enqueue(&root_bucket->scrb_clutch_buckets, clutch_bucket, options);
	os_atomic_store(&clutch_bucket->scb_root, root_clutch, relaxed);
	os_atomic_inc(&sched_clutch_global_bucket_load[bucket], relaxed);
}

  
                                         
  
                                                                   
   
static void
sched_clutch_bucket_hierarchy_remove(
	sched_clutch_root_t root_clutch,
	sched_clutch_bucket_t clutch_bucket,
	sched_bucket_t bucket,
	uint64_t timestamp,
	__unused sched_clutch_bucket_options_t options)
{
	sched_clutch_hierarchy_locked_assert(root_clutch);
	if (bucket > TH_BUCKET_FIXPRI) {
		                                                                                      
		remqueue(&clutch_bucket->scb_listlink);
	}
#if CONFIG_SCHED_EDGE
	sched_clutch_bucket_mark_native(clutch_bucket, root_clutch);
#endif                        

	sched_clutch_root_bucket_t root_bucket = &root_clutch->scr_unbound_buckets[bucket];

	                                                                  
	sched_clutch_bucket_runq_remove(&root_bucket->scrb_clutch_buckets, clutch_bucket);
	os_atomic_store(&clutch_bucket->scb_root, NULL, relaxed);

	                                                                                            
	if (sched_clutch_bucket_runq_empty(&root_bucket->scrb_clutch_buckets)) {
		sched_clutch_root_bucket_empty(root_bucket, root_clutch, timestamp);
	}
	os_atomic_dec(&sched_clutch_global_bucket_load[bucket], relaxed);
}

  
                                 
  
                                                                
                                                                 
                                                              
                                                             
                 
   
static uint8_t
sched_clutch_bucket_base_pri(
	sched_clutch_bucket_t clutch_bucket)
{
	uint8_t clutch_boost = 0;
	assert(priority_queue_empty(&clutch_bucket->scb_thread_runq) == false);

	sched_clutch_t clutch = clutch_bucket->scb_group->scbg_clutch;

	  
                                                                                 
                                                                                  
                                                                                     
                         
    
	uint8_t max_pri = 0;
	if (!priority_queue_empty(&clutch_bucket->scb_clutchpri_prioq)) {
		max_pri = priority_queue_max_sched_pri(&clutch_bucket->scb_clutchpri_prioq);
	}

	sched_clutch_tg_priority_t tg_pri = os_atomic_load(&clutch->sc_tg_priority, relaxed);
	clutch_boost = sched_clutch_bucket_group_pri_boost[tg_pri];
	return max_pri + clutch_boost;
}

  
                                             
  
                                                                                           
   
static uint8_t
sched_clutch_interactivity_from_cpu_data(sched_clutch_bucket_group_t clutch_bucket_group)
{
	sched_clutch_bucket_cpu_data_t scb_cpu_data;
	scb_cpu_data.scbcd_cpu_data_packed = os_atomic_load_wide(&clutch_bucket_group->scbg_cpu_data.scbcd_cpu_data_packed, relaxed);
	clutch_cpu_data_t cpu_used = scb_cpu_data.cpu_data.scbcd_cpu_used;
	clutch_cpu_data_t cpu_blocked = scb_cpu_data.cpu_data.scbcd_cpu_blocked;
	uint8_t interactive_score = 0;

	if ((cpu_blocked == 0) && (cpu_used == 0)) {
		return (uint8_t)clutch_bucket_group->scbg_interactivity_data.scct_count;
	}
	  
                                                                              
                                   
    
	if (cpu_blocked > cpu_used) {
		                                    
		interactive_score = sched_clutch_bucket_group_interactive_pri +
		    ((sched_clutch_bucket_group_interactive_pri * (cpu_blocked - cpu_used)) / cpu_blocked);
	} else {
		                                        
		interactive_score = ((sched_clutch_bucket_group_interactive_pri * cpu_blocked) / cpu_used);
	}
	return interactive_score;
}

  
                                      
  
                                                                       
                                                                         
                                                                       
                                    
   
static uint8_t
sched_clutch_bucket_pri_calculate(
	sched_clutch_bucket_t clutch_bucket,
	uint64_t timestamp)
{
	                                                 
	if (clutch_bucket->scb_thr_count == 0) {
		return 0;
	}

	uint8_t base_pri = sched_clutch_bucket_base_pri(clutch_bucket);
	uint8_t interactive_score = sched_clutch_bucket_group_interactivity_score_calculate(clutch_bucket->scb_group, timestamp);

	assert(((uint64_t)base_pri + interactive_score) <= UINT8_MAX);
	uint8_t pri = base_pri + interactive_score;
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_CLUTCH_TG_BUCKET_PRI) | DBG_FUNC_NONE,
	    thread_group_get_id(clutch_bucket->scb_group->scbg_clutch->sc_tg), clutch_bucket->scb_bucket, pri, interactive_score, 0);
	return pri;
}

  
                                                   
  
                                                     
                          
   
static sched_clutch_bucket_t
sched_clutch_root_bucket_highest_clutch_bucket(
	sched_clutch_root_bucket_t root_bucket)
{
	if (sched_clutch_bucket_runq_empty(&root_bucket->scrb_clutch_buckets)) {
		return NULL;
	}
	return sched_clutch_bucket_runq_peek(&root_bucket->scrb_clutch_buckets);
}

  
                                 
  
                                                                           
                                                                              
                               
   
static boolean_t
sched_clutch_bucket_runnable(
	sched_clutch_bucket_t clutch_bucket,
	sched_clutch_root_t root_clutch,
	uint64_t timestamp,
	sched_clutch_bucket_options_t options)
{
	sched_clutch_hierarchy_locked_assert(root_clutch);
	                                                                                 
	clutch_bucket->scb_priority = sched_clutch_bucket_pri_calculate(clutch_bucket, timestamp);
	sched_clutch_bucket_hierarchy_insert(root_clutch, clutch_bucket, clutch_bucket->scb_bucket, timestamp, options);

	                                                                                         
	sched_clutch_bucket_group_timeshare_update(clutch_bucket->scb_group, clutch_bucket, timestamp);
	int16_t root_old_pri = root_clutch->scr_priority;
	sched_clutch_root_pri_update(root_clutch);
	return root_clutch->scr_priority > root_old_pri;
}

  
                               
  
                                                                        
                                                                         
                                                                      
                                               
   
static boolean_t
sched_clutch_bucket_update(
	sched_clutch_bucket_t clutch_bucket,
	sched_clutch_root_t root_clutch,
	uint64_t timestamp,
	sched_clutch_bucket_options_t options)
{
	sched_clutch_hierarchy_locked_assert(root_clutch);
	uint64_t new_pri = sched_clutch_bucket_pri_calculate(clutch_bucket, timestamp);
	sched_clutch_bucket_runq_t bucket_runq = &root_clutch->scr_unbound_buckets[clutch_bucket->scb_bucket].scrb_clutch_buckets;
	if (new_pri == clutch_bucket->scb_priority) {
		  
                                                                                   
                                                                                   
                          
     
		if (options & SCHED_CLUTCH_BUCKET_OPTIONS_SAMEPRI_RR) {
			sched_clutch_bucket_runq_rotate(bucket_runq, clutch_bucket);
		}
		return false;
	}
	sched_clutch_bucket_runq_remove(bucket_runq, clutch_bucket);
#if CONFIG_SCHED_EDGE
	if (clutch_bucket->scb_foreign) {
		priority_queue_remove(&root_clutch->scr_foreign_buckets, &clutch_bucket->scb_foreignlink);
	}
#endif                        
	clutch_bucket->scb_priority = new_pri;
#if CONFIG_SCHED_EDGE
	if (clutch_bucket->scb_foreign) {
		priority_queue_entry_set_sched_pri(&root_clutch->scr_foreign_buckets, &clutch_bucket->scb_foreignlink, clutch_bucket->scb_priority, 0);
		priority_queue_insert(&root_clutch->scr_foreign_buckets, &clutch_bucket->scb_foreignlink);
	}
#endif                        
	sched_clutch_bucket_runq_enqueue(bucket_runq, clutch_bucket, options);

	int16_t root_old_pri = root_clutch->scr_priority;
	sched_clutch_root_pri_update(root_clutch);
	return root_clutch->scr_priority > root_old_pri;
}

  
                              
  
                                                                                
                                                                                 
                          
   
static void
sched_clutch_bucket_empty(
	sched_clutch_bucket_t clutch_bucket,
	sched_clutch_root_t root_clutch,
	uint64_t timestamp,
	sched_clutch_bucket_options_t options)
{
	sched_clutch_hierarchy_locked_assert(root_clutch);
	sched_clutch_bucket_hierarchy_remove(root_clutch, clutch_bucket, clutch_bucket->scb_bucket, timestamp, options);
	clutch_bucket->scb_priority = sched_clutch_bucket_pri_calculate(clutch_bucket, timestamp);
	sched_clutch_root_pri_update(root_clutch);
}

  
                                  
  
                                                              
   
void
sched_clutch_cpu_usage_update(
	thread_t thread,
	uint64_t delta)
{
	if (!SCHED_CLUTCH_THREAD_ELIGIBLE(thread) || SCHED_CLUTCH_THREAD_CLUSTER_BOUND(thread)) {
		return;
	}

	sched_clutch_t clutch = sched_clutch_for_thread(thread);
	sched_clutch_bucket_group_t clutch_bucket_group = &(clutch->sc_clutch_groups[thread->th_sched_bucket]);
	sched_clutch_bucket_group_cpu_usage_update(clutch_bucket_group, delta);
}

  
                                               
  
                                                        
   
static void
sched_clutch_bucket_group_cpu_usage_update(
	sched_clutch_bucket_group_t clutch_bucket_group,
	uint64_t delta)
{
	if (clutch_bucket_group->scbg_bucket == TH_BUCKET_FIXPRI) {
		                                                                                      
		return;
	}
	delta = MIN(delta, sched_clutch_bucket_group_adjust_threshold);
	os_atomic_add(&(clutch_bucket_group->scbg_cpu_data.cpu_data.scbcd_cpu_used), (clutch_cpu_data_t)delta, relaxed);
}

  
                                                 
  
                                                                                                            
                                                                                          
   
static inline uint64_t
sched_clutch_bucket_group_cpu_pending_adjust(
	uint64_t cpu_used,
	uint64_t cpu_blocked,
	uint8_t pending_intervals)
{
	uint64_t cpu_used_adjusted = 0;
	if (cpu_blocked < cpu_used) {
		cpu_used_adjusted = (sched_clutch_bucket_group_interactive_pri * cpu_blocked * cpu_used);
		cpu_used_adjusted = cpu_used_adjusted / ((sched_clutch_bucket_group_interactive_pri * cpu_blocked) + (cpu_used * pending_intervals));
	} else {
		uint64_t adjust_factor = (cpu_blocked * pending_intervals) / sched_clutch_bucket_group_interactive_pri;
		cpu_used_adjusted = (adjust_factor > cpu_used) ? 0 : (cpu_used - adjust_factor);
	}
	return cpu_used_adjusted;
}

  
                                         
  
                                                                           
                                                                               
                                                                          
                                                                         
                                                                        
                                                                                
   
static void
sched_clutch_bucket_group_cpu_adjust(
	sched_clutch_bucket_group_t clutch_bucket_group,
	uint8_t pending_intervals)
{
	sched_clutch_bucket_cpu_data_t old_cpu_data = {};
	sched_clutch_bucket_cpu_data_t new_cpu_data = {};
	os_atomic_rmw_loop(&clutch_bucket_group->scbg_cpu_data.scbcd_cpu_data_packed, old_cpu_data.scbcd_cpu_data_packed, new_cpu_data.scbcd_cpu_data_packed, relaxed, {
		clutch_cpu_data_t cpu_used = old_cpu_data.cpu_data.scbcd_cpu_used;
		clutch_cpu_data_t cpu_blocked = old_cpu_data.cpu_data.scbcd_cpu_blocked;

		if ((pending_intervals == 0) && (cpu_used + cpu_blocked) < sched_clutch_bucket_group_adjust_threshold) {
		                                                           
		        os_atomic_rmw_loop_give_up();
		}
		if ((cpu_used + cpu_blocked) >= sched_clutch_bucket_group_adjust_threshold) {
		                                                                                               
		        cpu_used = cpu_used / SCHED_CLUTCH_BUCKET_GROUP_ADJUST_RATIO;
		        cpu_blocked = cpu_blocked / SCHED_CLUTCH_BUCKET_GROUP_ADJUST_RATIO;
		}
		                                                     
		cpu_used = (clutch_cpu_data_t)sched_clutch_bucket_group_cpu_pending_adjust(cpu_used, cpu_blocked, pending_intervals);
		new_cpu_data.cpu_data.scbcd_cpu_used = cpu_used;
		new_cpu_data.cpu_data.scbcd_cpu_blocked = cpu_blocked;
	});
}

  
                                    
  
                                                                
                                                                      
                                                                       
                                                                   
                                                                    
                                                                        
                                                                         
                                                                        
                             
   

  
                                                                         
  
                                                                    
          
   
uint32_t
sched_clutch_thread_run_bucket_incr(
	thread_t thread,
	sched_bucket_t bucket)
{
	if (!SCHED_CLUTCH_THREAD_ELIGIBLE(thread)) {
		return 0;
	}
	sched_clutch_t clutch = sched_clutch_for_thread(thread);
	return sched_clutch_run_bucket_incr(clutch, bucket);
}

static uint32_t
sched_clutch_run_bucket_incr(
	sched_clutch_t clutch,
	sched_bucket_t bucket)
{
	assert(bucket != TH_BUCKET_RUN);
	sched_clutch_bucket_group_t clutch_bucket_group = &(clutch->sc_clutch_groups[bucket]);
	return sched_clutch_bucket_group_run_count_inc(clutch_bucket_group);
}

  
                                                                         
  
                                                                    
          
   
uint32_t
sched_clutch_thread_run_bucket_decr(
	thread_t thread,
	sched_bucket_t bucket)
{
	if (!SCHED_CLUTCH_THREAD_ELIGIBLE(thread)) {
		return 0;
	}
	sched_clutch_t clutch = sched_clutch_for_thread(thread);
	return sched_clutch_run_bucket_decr(clutch, bucket);
}

static uint32_t
sched_clutch_run_bucket_decr(
	sched_clutch_t clutch,
	sched_bucket_t bucket)
{
	assert(bucket != TH_BUCKET_RUN);
	sched_clutch_bucket_group_t clutch_bucket_group = &(clutch->sc_clutch_groups[bucket]);
	return sched_clutch_bucket_group_run_count_dec(clutch_bucket_group);
}

  
                                               
  
                                                                            
                                                                                   
                                                                               
                                                                          
                                                                              
                                                                                 
  
                                                                                  
                                                                                 
                                                                             
                                                                                       
                                                              
  
                         
                                                                     
                                                                        
                                                                         
                                                                          
                       
   
static void
sched_clutch_bucket_group_timeshare_update(
	sched_clutch_bucket_group_t clutch_bucket_group,
	sched_clutch_bucket_t clutch_bucket,
	uint64_t ctime)
{
	if (clutch_bucket_group->scbg_bucket < TH_BUCKET_SHARE_FG) {
		                                                               
		return;
	}

	  
                                                               
                                             
    
	uint32_t sched_ts = os_atomic_load(&clutch_bucket_group->scbg_timeshare_tick, relaxed);
	uint32_t current_sched_ts = sched_tick;
	if (sched_ts < current_sched_ts) {
		os_atomic_store(&clutch_bucket_group->scbg_timeshare_tick, current_sched_ts, relaxed);
		                                                     
		uint64_t bucket_group_run_count = os_atomic_load_wide(&clutch_bucket_group->scbg_blocked_data.scct_count, relaxed) - 1;
		uint32_t bucket_group_load = (uint32_t)(bucket_group_run_count / processor_avail_count);
		bucket_group_load = MIN(bucket_group_load, NRQS - 1);
		uint32_t pri_shift = sched_fixed_shift - sched_load_shifts[bucket_group_load];
		                                                   
		pri_shift = (pri_shift > SCHED_PRI_SHIFT_MAX) ? INT8_MAX : pri_shift;
		os_atomic_store(&clutch_bucket_group->scbg_pri_shift, pri_shift, relaxed);
	}

	  
                                                                                        
                                                          
    
	sched_clutch_bucket_update(clutch_bucket, clutch_bucket->scb_root, ctime, SCHED_CLUTCH_BUCKET_OPTIONS_NONE);
}

  
                                      
  
                                                                       
                                                                          
                                                                      
                                                                   
                                                                        
                                 
   
void
sched_clutch_thread_clutch_update(
	thread_t thread,
	sched_clutch_t old_clutch,
	sched_clutch_t new_clutch)
{
	uint32_t cpu_delta;
	assert(current_thread() == thread);

	if (old_clutch) {
		sched_clutch_run_bucket_decr(old_clutch, thread->th_sched_bucket);
		  
                                                                
                                                                  
                                  
     
		thread_timer_delta(thread, cpu_delta);
		if (thread->pri_shift < INT8_MAX) {
			thread->sched_usage += cpu_delta;
		}
		thread->cpu_delta += cpu_delta;
		if (!SCHED_CLUTCH_THREAD_CLUSTER_BOUND(thread)) {
			sched_clutch_bucket_group_t clutch_bucket_group = &(old_clutch->sc_clutch_groups[thread->th_sched_bucket]);
			sched_clutch_bucket_group_cpu_usage_update(clutch_bucket_group, cpu_delta);
		}
	}

	if (new_clutch) {
		sched_clutch_run_bucket_incr(new_clutch, thread->th_sched_bucket);
	}
}

                                                 

#if CONFIG_SCHED_EDGE

  
                                      
  
                                                                                    
                                                                                  
                                                                                  
                                                                               
                                                                          
           
  
                            
                                                                                         
                                                                                     
                       
   

static boolean_t
sched_edge_bound_thread_insert(
	sched_clutch_root_t root_clutch,
	thread_t thread,
	integer_t options)
{
	                                                   
	sched_clutch_thr_count_inc(&root_clutch->scr_thr_count);
	sched_clutch_root_bucket_t root_bucket = &root_clutch->scr_bound_buckets[thread->th_sched_bucket];
	if (root_bucket->scrb_bound_thread_runq.count == 0) {
		sched_clutch_root_bucket_runnable(root_bucket, root_clutch, mach_absolute_time());
	}

	assert((thread->th_bound_cluster_enqueued) == false);
	run_queue_enqueue(&root_bucket->scrb_bound_thread_runq, thread, options);
	thread->th_bound_cluster_enqueued = true;

	int16_t root_old_pri = root_clutch->scr_priority;
	sched_clutch_root_pri_update(root_clutch);
	return root_clutch->scr_priority > root_old_pri;
}

static void
sched_edge_bound_thread_remove(
	sched_clutch_root_t root_clutch,
	thread_t thread)
{
	sched_clutch_root_bucket_t root_bucket = &root_clutch->scr_bound_buckets[thread->th_sched_bucket];
	assert((thread->th_bound_cluster_enqueued) == true);
	run_queue_remove(&root_bucket->scrb_bound_thread_runq, thread);
	thread->th_bound_cluster_enqueued = false;

	                                                   
	sched_clutch_thr_count_dec(&root_clutch->scr_thr_count);
	if (root_bucket->scrb_bound_thread_runq.count == 0) {
		sched_clutch_root_bucket_empty(root_bucket, root_clutch, mach_absolute_time());
	}
	sched_clutch_root_pri_update(root_clutch);
}

#endif                        

  
                                     
  
                                                                                   
   
static thread_t
sched_clutch_thread_bound_lookup(
	__unused sched_clutch_root_t root_clutch,
	sched_clutch_root_bucket_t root_bucket)
{
	return run_queue_peek(&root_bucket->scrb_bound_thread_runq);
}

  
                                                                 
  
                                                                                  
                                                                                     
                                                                                         
                                                                                        
                                                            
  
                                                                                     
                                                                                            
                                                                                          
               
                                                                                           
                                                                              
                                                                                          
                                                                 
  
                                                                                            
                                                                                
   

#if CONFIG_SCHED_EDGE

static void
sched_clutch_bucket_group_thr_count_inc(
	sched_clutch_bucket_group_t clutch_bucket_group,
	uint64_t timestamp)
{
	sched_clutch_counter_time_t old_pending_data;
	sched_clutch_counter_time_t new_pending_data;
	os_atomic_rmw_loop(&clutch_bucket_group->scbg_pending_data.scct_packed, old_pending_data.scct_packed, new_pending_data.scct_packed, relaxed, {
		new_pending_data.scct_count = old_pending_data.scct_count + 1;
		new_pending_data.scct_timestamp = old_pending_data.scct_timestamp;
		if (old_pending_data.scct_count == 0) {
		        new_pending_data.scct_timestamp = timestamp;
		}
	});
}

static void
sched_clutch_bucket_group_thr_count_dec(
	sched_clutch_bucket_group_t clutch_bucket_group,
	uint64_t timestamp)
{
	sched_clutch_counter_time_t old_pending_data;
	sched_clutch_counter_time_t new_pending_data;
	os_atomic_rmw_loop(&clutch_bucket_group->scbg_pending_data.scct_packed, old_pending_data.scct_packed, new_pending_data.scct_packed, relaxed, {
		new_pending_data.scct_count = old_pending_data.scct_count - 1;
		if (new_pending_data.scct_count == 0) {
		        new_pending_data.scct_timestamp = SCHED_CLUTCH_BUCKET_GROUP_PENDING_INVALID;
		} else {
		        new_pending_data.scct_timestamp = timestamp;
		}
	});
}

static uint8_t
sched_clutch_bucket_group_pending_ageout(
	sched_clutch_bucket_group_t clutch_bucket_group,
	uint64_t timestamp)
{
	int bucket_load = sched_clutch_global_bucket_load_get(clutch_bucket_group->scbg_bucket);
	sched_clutch_counter_time_t old_pending_data;
	sched_clutch_counter_time_t new_pending_data;
	uint8_t cpu_usage_shift = 0;

	os_atomic_rmw_loop(&clutch_bucket_group->scbg_pending_data.scct_packed, old_pending_data.scct_packed, new_pending_data.scct_packed, relaxed, {
		cpu_usage_shift = 0;
		uint64_t old_pending_ts = old_pending_data.scct_timestamp;
		bool old_update = (old_pending_ts >= timestamp);
		bool no_pending_time = (old_pending_ts == SCHED_CLUTCH_BUCKET_GROUP_PENDING_INVALID);
		bool no_bucket_load = (bucket_load == 0);
		if (old_update || no_pending_time || no_bucket_load) {
		        os_atomic_rmw_loop_give_up();
		}

		                                                                 
		uint64_t pending_delta = timestamp - old_pending_ts;
		uint64_t interactivity_delta = sched_clutch_bucket_group_pending_delta[clutch_bucket_group->scbg_bucket] * bucket_load;
		if (pending_delta < interactivity_delta) {
		        os_atomic_rmw_loop_give_up();
		}
		cpu_usage_shift = (pending_delta / interactivity_delta);
		new_pending_data.scct_timestamp = old_pending_ts + (cpu_usage_shift * interactivity_delta);
		new_pending_data.scct_count = old_pending_data.scct_count;
	});
	return cpu_usage_shift;
}

static uint8_t
sched_clutch_bucket_group_interactivity_score_calculate(
	sched_clutch_bucket_group_t clutch_bucket_group,
	uint64_t timestamp)
{
	if (clutch_bucket_group->scbg_bucket == TH_BUCKET_FIXPRI) {
		  
                                                                                  
                                                                      
     
		assert(clutch_bucket_group->scbg_interactivity_data.scct_count == (2 * sched_clutch_bucket_group_interactive_pri));
		return (uint8_t)clutch_bucket_group->scbg_interactivity_data.scct_count;
	}
	                                                                                         
	uint8_t pending_intervals = sched_clutch_bucket_group_pending_ageout(clutch_bucket_group, timestamp);
	                                                                                                  
	sched_clutch_bucket_group_cpu_adjust(clutch_bucket_group, pending_intervals);
	uint8_t interactivity_score = sched_clutch_interactivity_from_cpu_data(clutch_bucket_group);
	sched_clutch_counter_time_t old_interactivity_data;
	sched_clutch_counter_time_t new_interactivity_data;

	bool score_updated = os_atomic_rmw_loop(&clutch_bucket_group->scbg_interactivity_data.scct_packed, old_interactivity_data.scct_packed, new_interactivity_data.scct_packed, relaxed, {
		if (old_interactivity_data.scct_timestamp >= timestamp) {
		        os_atomic_rmw_loop_give_up();
		}
		new_interactivity_data.scct_timestamp = timestamp;
		if (old_interactivity_data.scct_timestamp != 0) {
		        new_interactivity_data.scct_count = interactivity_score;
		}
	});
	if (score_updated) {
		return (uint8_t)new_interactivity_data.scct_count;
	} else {
		return (uint8_t)old_interactivity_data.scct_count;
	}
}

#else                        

  
                                                                               
                                                                                 
   
static void
sched_clutch_bucket_group_thr_count_inc(
	sched_clutch_bucket_group_t clutch_bucket_group,
	uint64_t timestamp)
{
	sched_clutch_hierarchy_locked_assert(&pset0.pset_clutch_root);
	if (clutch_bucket_group->scbg_pending_data.scct_count == 0) {
		clutch_bucket_group->scbg_pending_data.scct_timestamp = timestamp;
	}
	clutch_bucket_group->scbg_pending_data.scct_count++;
}

static void
sched_clutch_bucket_group_thr_count_dec(
	sched_clutch_bucket_group_t clutch_bucket_group,
	uint64_t timestamp)
{
	sched_clutch_hierarchy_locked_assert(&pset0.pset_clutch_root);
	clutch_bucket_group->scbg_pending_data.scct_count--;
	if (clutch_bucket_group->scbg_pending_data.scct_count == 0) {
		clutch_bucket_group->scbg_pending_data.scct_timestamp = SCHED_CLUTCH_BUCKET_GROUP_PENDING_INVALID;
	} else {
		clutch_bucket_group->scbg_pending_data.scct_timestamp = timestamp;
	}
}

static uint8_t
sched_clutch_bucket_group_pending_ageout(
	sched_clutch_bucket_group_t clutch_bucket_group,
	uint64_t timestamp)
{
	sched_clutch_hierarchy_locked_assert(&pset0.pset_clutch_root);
	int bucket_load = sched_clutch_global_bucket_load_get(clutch_bucket_group->scbg_bucket);
	uint64_t old_pending_ts = clutch_bucket_group->scbg_pending_data.scct_timestamp;
	bool old_update = (old_pending_ts >= timestamp);
	bool no_pending_time = (old_pending_ts == SCHED_CLUTCH_BUCKET_GROUP_PENDING_INVALID);
	bool no_bucket_load = (bucket_load == 0);
	if (old_update || no_pending_time || no_bucket_load) {
		return 0;
	}
	uint64_t pending_delta = timestamp - old_pending_ts;
	uint64_t interactivity_delta = sched_clutch_bucket_group_pending_delta[clutch_bucket_group->scbg_bucket] * bucket_load;
	if (pending_delta < interactivity_delta) {
		return 0;
	}
	uint8_t cpu_usage_shift = (pending_delta / interactivity_delta);
	clutch_bucket_group->scbg_pending_data.scct_timestamp = old_pending_ts + (cpu_usage_shift * interactivity_delta);
	return cpu_usage_shift;
}

static uint8_t
sched_clutch_bucket_group_interactivity_score_calculate(
	sched_clutch_bucket_group_t clutch_bucket_group,
	uint64_t timestamp)
{
	sched_clutch_hierarchy_locked_assert(&pset0.pset_clutch_root);
	if (clutch_bucket_group->scbg_bucket == TH_BUCKET_FIXPRI) {
		  
                                                                                  
                                                                      
     
		assert(clutch_bucket_group->scbg_interactivity_data.scct_count == (2 * sched_clutch_bucket_group_interactive_pri));
		return (uint8_t)clutch_bucket_group->scbg_interactivity_data.scct_count;
	}
	                                                                                         
	uint8_t pending_intervals = sched_clutch_bucket_group_pending_ageout(clutch_bucket_group, timestamp);
	                                                                                                  
	sched_clutch_bucket_group_cpu_adjust(clutch_bucket_group, pending_intervals);
	uint8_t interactivity_score = sched_clutch_interactivity_from_cpu_data(clutch_bucket_group);
	if (timestamp > clutch_bucket_group->scbg_interactivity_data.scct_timestamp) {
		clutch_bucket_group->scbg_interactivity_data.scct_count = interactivity_score;
		clutch_bucket_group->scbg_interactivity_data.scct_timestamp = timestamp;
		return interactivity_score;
	} else {
		return (uint8_t)clutch_bucket_group->scbg_interactivity_data.scct_count;
	}
}

#endif                        

  
                                                            
  
                                                                                         
                                                                                  
                                                                                          
  
                                                                                         
                                             
                                                                                     
                                                                                    
   

#if !__LP64__

static uint32_t
sched_clutch_bucket_group_run_count_inc(
	sched_clutch_bucket_group_t clutch_bucket_group)
{
	uint64_t blocked_time = 0;
	uint64_t updated_run_count = 0;

	lck_spin_lock(&clutch_bucket_group->scbg_stats_lock);
	if ((clutch_bucket_group->scbg_blocked_data.scct_timestamp != SCHED_CLUTCH_BUCKET_GROUP_BLOCKED_TS_INVALID) &&
	    (clutch_bucket_group->scbg_blocked_data.scct_count == 0)) {
		                                                                                           
		blocked_time = mach_absolute_time() - clutch_bucket_group->scbg_blocked_data.scct_timestamp;
		clutch_bucket_group->scbg_blocked_data.scct_timestamp = SCHED_CLUTCH_BUCKET_GROUP_BLOCKED_TS_INVALID;
	}
	clutch_bucket_group->scbg_blocked_data.scct_count = clutch_bucket_group->scbg_blocked_data.scct_count + 1;
	updated_run_count = clutch_bucket_group->scbg_blocked_data.scct_count;
	lck_spin_unlock(&clutch_bucket_group->scbg_stats_lock);

	blocked_time = MIN(blocked_time, sched_clutch_bucket_group_adjust_threshold);
	os_atomic_add(&(clutch_bucket_group->scbg_cpu_data.cpu_data.scbcd_cpu_blocked), (clutch_cpu_data_t)blocked_time, relaxed);
	return (uint32_t)updated_run_count;
}

static uint32_t
sched_clutch_bucket_group_run_count_dec(
	sched_clutch_bucket_group_t clutch_bucket_group)
{
	uint64_t updated_run_count = 0;

	lck_spin_lock(&clutch_bucket_group->scbg_stats_lock);
	clutch_bucket_group->scbg_blocked_data.scct_count = clutch_bucket_group->scbg_blocked_data.scct_count - 1;
	if (clutch_bucket_group->scbg_blocked_data.scct_count == 0) {
		                                                                     
		clutch_bucket_group->scbg_blocked_data.scct_timestamp = mach_absolute_time();
	}
	updated_run_count = clutch_bucket_group->scbg_blocked_data.scct_count;
	lck_spin_unlock(&clutch_bucket_group->scbg_stats_lock);
	return (uint32_t)updated_run_count;
}

#else                

static uint32_t
sched_clutch_bucket_group_run_count_inc(
	sched_clutch_bucket_group_t clutch_bucket_group)
{
	sched_clutch_counter_time_t old_blocked_data;
	sched_clutch_counter_time_t new_blocked_data;

	bool update_blocked_time = false;
	os_atomic_rmw_loop(&clutch_bucket_group->scbg_blocked_data.scct_packed, old_blocked_data.scct_packed, new_blocked_data.scct_packed, relaxed, {
		new_blocked_data.scct_count = old_blocked_data.scct_count + 1;
		new_blocked_data.scct_timestamp = old_blocked_data.scct_timestamp;
		update_blocked_time = false;
		if (old_blocked_data.scct_count == 0) {
		        new_blocked_data.scct_timestamp = SCHED_CLUTCH_BUCKET_GROUP_BLOCKED_TS_INVALID;
		        update_blocked_time = true;
		}
	});
	if (update_blocked_time && (old_blocked_data.scct_timestamp != SCHED_CLUTCH_BUCKET_GROUP_BLOCKED_TS_INVALID)) {
		uint64_t ctime = mach_absolute_time();
		if (ctime > old_blocked_data.scct_timestamp) {
			uint64_t blocked_time = ctime - old_blocked_data.scct_timestamp;
			blocked_time = MIN(blocked_time, sched_clutch_bucket_group_adjust_threshold);
			os_atomic_add(&(clutch_bucket_group->scbg_cpu_data.cpu_data.scbcd_cpu_blocked), (clutch_cpu_data_t)blocked_time, relaxed);
		}
	}
	return (uint32_t)new_blocked_data.scct_count;
}

static uint32_t
sched_clutch_bucket_group_run_count_dec(
	sched_clutch_bucket_group_t clutch_bucket_group)
{
	sched_clutch_counter_time_t old_blocked_data;
	sched_clutch_counter_time_t new_blocked_data;

	uint64_t ctime = mach_absolute_time();
	os_atomic_rmw_loop(&clutch_bucket_group->scbg_blocked_data.scct_packed, old_blocked_data.scct_packed, new_blocked_data.scct_packed, relaxed, {
		new_blocked_data.scct_count = old_blocked_data.scct_count - 1;
		new_blocked_data.scct_timestamp = old_blocked_data.scct_timestamp;
		if (new_blocked_data.scct_count == 0) {
		        new_blocked_data.scct_timestamp = ctime;
		}
	});
	return (uint32_t)new_blocked_data.scct_count;
}

#endif                

  
                               
  
                                                              
                                                                        
                                                                  
   
static boolean_t
sched_clutch_thread_insert(
	sched_clutch_root_t root_clutch,
	thread_t thread,
	integer_t options)
{
	boolean_t result = FALSE;

	sched_clutch_hierarchy_locked_assert(root_clutch);
#if CONFIG_SCHED_EDGE
	sched_edge_cluster_cumulative_count_incr(root_clutch, thread->th_sched_bucket);
	  
                                                                                    
                                                                                        
                                                                             
                                                                            
           
    
	if (SCHED_CLUTCH_THREAD_CLUSTER_BOUND(thread) && (sched_edge_thread_bound_cluster_id(thread) == root_clutch->scr_cluster_id)) {
		return sched_edge_bound_thread_insert(root_clutch, thread, options);
	}
#endif                        
	sched_clutch_t clutch = sched_clutch_for_thread(thread);
	assert(thread->thread_group == clutch->sc_tg);

	uint64_t current_timestamp = mach_absolute_time();
	sched_clutch_bucket_group_t clutch_bucket_group = &(clutch->sc_clutch_groups[thread->th_sched_bucket]);
	sched_clutch_bucket_t clutch_bucket = &(clutch_bucket_group->scbg_clutch_buckets[root_clutch->scr_cluster_id]);
	assert((clutch_bucket->scb_root == NULL) || (clutch_bucket->scb_root == root_clutch));

	  
                                   
   
                                                         
                                                                                                           
                                                                                                                                 
                                                                                      
    

	                                                                                   
	thread->th_clutch_runq_link.stamp = current_timestamp;
	priority_queue_entry_set_sched_pri(&clutch_bucket->scb_thread_runq, &thread->th_clutch_runq_link, thread->sched_pri,
	    (options & SCHED_TAILQ) ? PRIORITY_QUEUE_ENTRY_NONE : PRIORITY_QUEUE_ENTRY_PREEMPTED);
	priority_queue_insert(&clutch_bucket->scb_thread_runq, &thread->th_clutch_runq_link);

	                                                                                            
	priority_queue_entry_set_sched_pri(&clutch_bucket->scb_clutchpri_prioq, &thread->th_clutch_pri_link,
	    sched_thread_sched_pri_promoted(thread) ? thread->sched_pri : thread->base_pri, false);
	priority_queue_insert(&clutch_bucket->scb_clutchpri_prioq, &thread->th_clutch_pri_link);

	                                                               
	enqueue_tail(&clutch_bucket->scb_thread_timeshare_queue, &thread->th_clutch_timeshare_link);

	                                                             
	sched_clutch_root_urgency_inc(root_clutch, thread);

	os_atomic_inc(&clutch->sc_thr_count, relaxed);
	sched_clutch_bucket_group_thr_count_inc(clutch_bucket->scb_group, current_timestamp);

	                                                                                                                               
	sched_clutch_bucket_options_t scb_options = (options & SCHED_HEADQ) ? SCHED_CLUTCH_BUCKET_OPTIONS_HEADQ : SCHED_CLUTCH_BUCKET_OPTIONS_TAILQ;
	if (clutch_bucket->scb_thr_count == 0) {
		sched_clutch_thr_count_inc(&clutch_bucket->scb_thr_count);
		sched_clutch_thr_count_inc(&root_clutch->scr_thr_count);
		result = sched_clutch_bucket_runnable(clutch_bucket, root_clutch, current_timestamp, scb_options);
	} else {
		sched_clutch_thr_count_inc(&clutch_bucket->scb_thr_count);
		sched_clutch_thr_count_inc(&root_clutch->scr_thr_count);
		result = sched_clutch_bucket_update(clutch_bucket, root_clutch, current_timestamp, scb_options);
	}

	KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_CLUTCH_THR_COUNT) | DBG_FUNC_NONE,
	    root_clutch->scr_cluster_id, thread_group_get_id(clutch_bucket->scb_group->scbg_clutch->sc_tg), clutch_bucket->scb_bucket,
	    SCHED_CLUTCH_DBG_THR_COUNT_PACK(root_clutch->scr_thr_count, os_atomic_load(&clutch->sc_thr_count, relaxed), clutch_bucket->scb_thr_count));
	return result;
}

  
                               
  
                                                              
                                                                        
                                                               
   
static void
sched_clutch_thread_remove(
	sched_clutch_root_t root_clutch,
	thread_t thread,
	uint64_t current_timestamp,
	sched_clutch_bucket_options_t options)
{
	sched_clutch_hierarchy_locked_assert(root_clutch);
#if CONFIG_SCHED_EDGE
	sched_edge_cluster_cumulative_count_decr(root_clutch, thread->th_sched_bucket);
	if (thread->th_bound_cluster_enqueued) {
		sched_edge_bound_thread_remove(root_clutch, thread);
		return;
	}
#endif                        
	sched_clutch_t clutch = sched_clutch_for_thread(thread);
	assert(thread->thread_group == clutch->sc_tg);
	assert(thread->runq != PROCESSOR_NULL);

	sched_clutch_bucket_group_t clutch_bucket_group = &(clutch->sc_clutch_groups[thread->th_sched_bucket]);
	sched_clutch_bucket_t clutch_bucket = &(clutch_bucket_group->scbg_clutch_buckets[root_clutch->scr_cluster_id]);
	assert(clutch_bucket->scb_root == root_clutch);

	                                                             
	sched_clutch_root_urgency_dec(root_clutch, thread);
	                                          
	priority_queue_remove(&clutch_bucket->scb_thread_runq, &thread->th_clutch_runq_link);
	remqueue(&thread->th_clutch_timeshare_link);
	thread->runq = PROCESSOR_NULL;

	priority_queue_remove(&clutch_bucket->scb_clutchpri_prioq, &thread->th_clutch_pri_link);

	                                                      
	os_atomic_dec(&clutch->sc_thr_count, relaxed);
	sched_clutch_bucket_group_thr_count_dec(clutch_bucket->scb_group, current_timestamp);
	sched_clutch_thr_count_dec(&root_clutch->scr_thr_count);
	sched_clutch_thr_count_dec(&clutch_bucket->scb_thr_count);

	                                                                        
	if (clutch_bucket->scb_thr_count == 0) {
		sched_clutch_bucket_empty(clutch_bucket, root_clutch, current_timestamp, options);
	} else {
		sched_clutch_bucket_update(clutch_bucket, root_clutch, current_timestamp, options);
	}
	KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_CLUTCH_THR_COUNT) | DBG_FUNC_NONE,
	    root_clutch->scr_cluster_id, thread_group_get_id(clutch_bucket->scb_group->scbg_clutch->sc_tg), clutch_bucket->scb_bucket,
	    SCHED_CLUTCH_DBG_THR_COUNT_PACK(root_clutch->scr_thr_count, os_atomic_load(&clutch->sc_thr_count, relaxed), clutch_bucket->scb_thr_count));
}

  
                                       
  
                                                                 
                                                               
                  
   
static thread_t
sched_clutch_thread_unbound_lookup(
	sched_clutch_root_t root_clutch,
	sched_clutch_root_bucket_t root_bucket)
{
	sched_clutch_hierarchy_locked_assert(root_clutch);

	                                                                 
	sched_clutch_bucket_t clutch_bucket = sched_clutch_root_bucket_highest_clutch_bucket(root_bucket);
	assert(clutch_bucket != NULL);

	                                                                     
	thread_t thread = priority_queue_max(&clutch_bucket->scb_thread_runq, struct thread, th_clutch_runq_link);
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_CLUTCH_THREAD_SELECT) | DBG_FUNC_NONE,
	    thread_tid(thread), thread_group_get_id(clutch_bucket->scb_group->scbg_clutch->sc_tg), clutch_bucket->scb_bucket, 0, 0);
	return thread;
}

  
                                       
  
                                                         
                                                              
                                                            
                                                       
                  
   
static thread_t
sched_clutch_thread_highest_remove(
	sched_clutch_root_t root_clutch)
{
	sched_clutch_hierarchy_locked_assert(root_clutch);
	uint64_t current_timestamp = mach_absolute_time();

	sched_clutch_root_bucket_t root_bucket = sched_clutch_root_highest_root_bucket(root_clutch, current_timestamp, SCHED_CLUTCH_HIGHEST_ROOT_BUCKET_ALL);
	if (root_bucket == NULL) {
		return THREAD_NULL;
	}

	thread_t highest_thread = THREAD_NULL;
	if (root_bucket->scrb_bound) {
		highest_thread = sched_clutch_thread_bound_lookup(root_clutch, root_bucket);
	} else {
		highest_thread = sched_clutch_thread_unbound_lookup(root_clutch, root_bucket);
	}
	sched_clutch_thread_remove(root_clutch, highest_thread, current_timestamp, SCHED_CLUTCH_BUCKET_OPTIONS_SAMEPRI_RR);
	return highest_thread;
}

                                         

  
                              
  
                                                     
                           
   
static uint32_t
sched_clutch_root_urgency(
	sched_clutch_root_t root_clutch)
{
	return root_clutch->scr_urgency;
}

  
                                
  
                                                     
                                                        
                                                     
                                                       
                                              
   
static uint32_t
sched_clutch_root_count_sum(
	__unused sched_clutch_root_t root_clutch)
{
	return 0;
}

  
                               
  
                                                      
                           
   
static int
sched_clutch_root_priority(
	sched_clutch_root_t root_clutch)
{
	return root_clutch->scr_priority;
}

  
                            
  
                                                             
   
uint32_t
sched_clutch_root_count(
	sched_clutch_root_t root_clutch)
{
	return root_clutch->scr_thr_count;
}

#if CONFIG_SCHED_EDGE

  
                                    
  
                                                                                      
   
static boolean_t
sched_clutch_root_foreign_empty(
	sched_clutch_root_t root_clutch)
{
	return priority_queue_empty(&root_clutch->scr_foreign_buckets);
}

  
                                                    
  
                                                                                   
                                                       
   
static thread_t
sched_clutch_root_highest_foreign_thread_remove(
	sched_clutch_root_t root_clutch)
{
	thread_t thread = THREAD_NULL;
	if (priority_queue_empty(&root_clutch->scr_foreign_buckets)) {
		return thread;
	}
	sched_clutch_bucket_t clutch_bucket = priority_queue_max(&root_clutch->scr_foreign_buckets, struct sched_clutch_bucket, scb_foreignlink);
	thread = priority_queue_max(&clutch_bucket->scb_thread_runq, struct thread, th_clutch_runq_link);
	sched_clutch_thread_remove(root_clutch, thread, mach_absolute_time(), 0);
	return thread;
}

#endif                        

  
                                  
  
                                                        
                                                            
                                                        
                     
   
uint32_t
sched_clutch_thread_pri_shift(
	thread_t thread,
	sched_bucket_t bucket)
{
	if (!SCHED_CLUTCH_THREAD_ELIGIBLE(thread)) {
		return INT8_MAX;
	}
	assert(bucket != TH_BUCKET_RUN);
	sched_clutch_t clutch = sched_clutch_for_thread(thread);
	sched_clutch_bucket_group_t clutch_bucket_group = &(clutch->sc_clutch_groups[bucket]);
	return os_atomic_load(&clutch_bucket_group->scbg_pri_shift, relaxed);
}

#pragma mark -- Clutch Scheduler Algorithm

static void
sched_clutch_init(void);

static thread_t
sched_clutch_steal_thread(processor_set_t pset);

static void
sched_clutch_thread_update_scan(sched_update_scan_context_t scan_context);

static boolean_t
sched_clutch_processor_enqueue(processor_t processor, thread_t thread,
    sched_options_t options);

static boolean_t
sched_clutch_processor_queue_remove(processor_t processor, thread_t thread);

static ast_t
sched_clutch_processor_csw_check(processor_t processor);

static boolean_t
sched_clutch_processor_queue_has_priority(processor_t processor, int priority, boolean_t gte);

static int
sched_clutch_runq_count(processor_t processor);

static boolean_t
sched_clutch_processor_queue_empty(processor_t processor);

static uint64_t
sched_clutch_runq_stats_count_sum(processor_t processor);

static int
sched_clutch_processor_bound_count(processor_t processor);

static void
sched_clutch_pset_init(processor_set_t pset);

static void
sched_clutch_processor_init(processor_t processor);

static thread_t
sched_clutch_choose_thread(processor_t processor, int priority, ast_t reason);

static void
sched_clutch_processor_queue_shutdown(processor_t processor);

static sched_mode_t
sched_clutch_initial_thread_sched_mode(task_t parent_task);

static uint32_t
sched_clutch_initial_quantum_size(thread_t thread);

static bool
sched_clutch_thread_avoid_processor(processor_t processor, thread_t thread);

static uint32_t
sched_clutch_run_incr(thread_t thread);

static uint32_t
sched_clutch_run_decr(thread_t thread);

static void
sched_clutch_update_thread_bucket(thread_t thread);

const struct sched_dispatch_table sched_clutch_dispatch = {
	.sched_name                                     = "clutch",
	.init                                           = sched_clutch_init,
	.timebase_init                                  = sched_timeshare_timebase_init,
	.processor_init                                 = sched_clutch_processor_init,
	.pset_init                                      = sched_clutch_pset_init,
	.maintenance_continuation                       = sched_timeshare_maintenance_continue,
	.choose_thread                                  = sched_clutch_choose_thread,
	.steal_thread_enabled                           = sched_steal_thread_enabled,
	.steal_thread                                   = sched_clutch_steal_thread,
	.compute_timeshare_priority                     = sched_compute_timeshare_priority,
	.choose_node                                    = sched_choose_node,
	.choose_processor                               = choose_processor,
	.processor_enqueue                              = sched_clutch_processor_enqueue,
	.processor_queue_shutdown                       = sched_clutch_processor_queue_shutdown,
	.processor_queue_remove                         = sched_clutch_processor_queue_remove,
	.processor_queue_empty                          = sched_clutch_processor_queue_empty,
	.priority_is_urgent                             = priority_is_urgent,
	.processor_csw_check                            = sched_clutch_processor_csw_check,
	.processor_queue_has_priority                   = sched_clutch_processor_queue_has_priority,
	.initial_quantum_size                           = sched_clutch_initial_quantum_size,
	.initial_thread_sched_mode                      = sched_clutch_initial_thread_sched_mode,
	.can_update_priority                            = can_update_priority,
	.update_priority                                = update_priority,
	.lightweight_update_priority                    = lightweight_update_priority,
	.quantum_expire                                 = sched_default_quantum_expire,
	.processor_runq_count                           = sched_clutch_runq_count,
	.processor_runq_stats_count_sum                 = sched_clutch_runq_stats_count_sum,
	.processor_bound_count                          = sched_clutch_processor_bound_count,
	.thread_update_scan                             = sched_clutch_thread_update_scan,
	.multiple_psets_enabled                         = TRUE,
	.sched_groups_enabled                           = FALSE,
	.avoid_processor_enabled                        = TRUE,
	.thread_avoid_processor                         = sched_clutch_thread_avoid_processor,
	.processor_balance                              = sched_SMT_balance,

	.rt_runq                                        = sched_rtlocal_runq,
	.rt_init                                        = sched_rtlocal_init,
	.rt_queue_shutdown                              = sched_rtlocal_queue_shutdown,
	.rt_runq_scan                                   = sched_rtlocal_runq_scan,
	.rt_runq_count_sum                              = sched_rtlocal_runq_count_sum,

	.qos_max_parallelism                            = sched_qos_max_parallelism,
	.check_spill                                    = sched_check_spill,
	.ipi_policy                                     = sched_ipi_policy,
	.thread_should_yield                            = sched_thread_should_yield,
	.run_count_incr                                 = sched_clutch_run_incr,
	.run_count_decr                                 = sched_clutch_run_decr,
	.update_thread_bucket                           = sched_clutch_update_thread_bucket,
	.pset_made_schedulable                          = sched_pset_made_schedulable,
};

__attribute__((always_inline))
static inline run_queue_t
sched_clutch_bound_runq(processor_t processor)
{
	return &processor->runq;
}

__attribute__((always_inline))
static inline sched_clutch_root_t
sched_clutch_processor_root_clutch(processor_t processor)
{
	return &processor->processor_set->pset_clutch_root;
}

__attribute__((always_inline))
static inline run_queue_t
sched_clutch_thread_bound_runq(processor_t processor, __assert_only thread_t thread)
{
	assert(thread->bound_processor == processor);
	return sched_clutch_bound_runq(processor);
}

static uint32_t
sched_clutch_initial_quantum_size(thread_t thread)
{
	if (thread == THREAD_NULL) {
		return std_quantum;
	}
	assert(sched_clutch_thread_quantum[thread->th_sched_bucket] <= UINT32_MAX);
	return (uint32_t)sched_clutch_thread_quantum[thread->th_sched_bucket];
}

static sched_mode_t
sched_clutch_initial_thread_sched_mode(task_t parent_task)
{
	if (parent_task == kernel_task) {
		return TH_MODE_FIXED;
	} else {
		return TH_MODE_TIMESHARE;
	}
}

static void
sched_clutch_processor_init(processor_t processor)
{
	run_queue_init(&processor->runq);
}

static void
sched_clutch_pset_init(processor_set_t pset)
{
	sched_clutch_root_init(&pset->pset_clutch_root, pset);
}

static void
sched_clutch_tunables_init(void)
{
	sched_clutch_us_to_abstime(sched_clutch_root_bucket_wcel_us, sched_clutch_root_bucket_wcel);
	sched_clutch_us_to_abstime(sched_clutch_root_bucket_warp_us, sched_clutch_root_bucket_warp);
	sched_clutch_us_to_abstime(sched_clutch_thread_quantum_us, sched_clutch_thread_quantum);
	clock_interval_to_absolutetime_interval(SCHED_CLUTCH_BUCKET_GROUP_ADJUST_THRESHOLD_USECS,
	    NSEC_PER_USEC, &sched_clutch_bucket_group_adjust_threshold);
	assert(sched_clutch_bucket_group_adjust_threshold <= CLUTCH_CPU_DATA_MAX);
	sched_clutch_us_to_abstime(sched_clutch_bucket_group_pending_delta_us, sched_clutch_bucket_group_pending_delta);
}

static void
sched_clutch_init(void)
{
	if (!PE_parse_boot_argn("sched_clutch_bucket_group_interactive_pri", &sched_clutch_bucket_group_interactive_pri, sizeof(sched_clutch_bucket_group_interactive_pri))) {
		sched_clutch_bucket_group_interactive_pri = SCHED_CLUTCH_BUCKET_GROUP_INTERACTIVE_PRI_DEFAULT;
	}
	sched_timeshare_init();
	sched_clutch_tunables_init();
}

static thread_t
sched_clutch_choose_thread(
	processor_t      processor,
	int              priority,
	__unused ast_t            reason)
{
	int clutch_pri = sched_clutch_root_priority(sched_clutch_processor_root_clutch(processor));
	uint32_t clutch_count = sched_clutch_root_count(sched_clutch_processor_root_clutch(processor));
	run_queue_t bound_runq = sched_clutch_bound_runq(processor);
	boolean_t choose_from_boundq = false;

	if (bound_runq->highq < priority &&
	    clutch_pri < priority) {
		return THREAD_NULL;
	}

	if (bound_runq->count && clutch_count) {
		if (bound_runq->highq >= clutch_pri) {
			choose_from_boundq = true;
		}
	} else if (bound_runq->count) {
		choose_from_boundq = true;
	} else if (clutch_count) {
		choose_from_boundq = false;
	} else {
		return THREAD_NULL;
	}

	thread_t thread = THREAD_NULL;
	if (choose_from_boundq == false) {
		sched_clutch_root_t pset_clutch_root = sched_clutch_processor_root_clutch(processor);
		thread = sched_clutch_thread_highest_remove(pset_clutch_root);
	} else {
		thread = run_queue_dequeue(bound_runq, SCHED_HEADQ);
	}
	return thread;
}

static boolean_t
sched_clutch_processor_enqueue(
	processor_t       processor,
	thread_t          thread,
	sched_options_t   options)
{
	boolean_t       result;

	thread->runq = processor;
	if (SCHED_CLUTCH_THREAD_ELIGIBLE(thread)) {
		sched_clutch_root_t pset_clutch_root = sched_clutch_processor_root_clutch(processor);
		result = sched_clutch_thread_insert(pset_clutch_root, thread, options);
	} else {
		run_queue_t rq = sched_clutch_thread_bound_runq(processor, thread);
		result = run_queue_enqueue(rq, thread, options);
	}
	return result;
}

static boolean_t
sched_clutch_processor_queue_empty(processor_t processor)
{
	return sched_clutch_root_count(sched_clutch_processor_root_clutch(processor)) == 0 &&
	       sched_clutch_bound_runq(processor)->count == 0;
}

static ast_t
sched_clutch_processor_csw_check(processor_t processor)
{
	boolean_t       has_higher;
	int             pri;

	if (sched_clutch_thread_avoid_processor(processor, current_thread())) {
		return AST_PREEMPT | AST_URGENT;
	}

	run_queue_t bound_runq = sched_clutch_bound_runq(processor);
	int clutch_pri = sched_clutch_root_priority(sched_clutch_processor_root_clutch(processor));

	assert(processor->active_thread != NULL);

	pri = MAX(clutch_pri, bound_runq->highq);

	if (processor->first_timeslice) {
		has_higher = (pri > processor->current_pri);
	} else {
		has_higher = (pri >= processor->current_pri);
	}

	if (has_higher) {
		if (sched_clutch_root_urgency(sched_clutch_processor_root_clutch(processor)) > 0) {
			return AST_PREEMPT | AST_URGENT;
		}

		if (bound_runq->urgency > 0) {
			return AST_PREEMPT | AST_URGENT;
		}

		return AST_PREEMPT;
	}

	return AST_NONE;
}

static boolean_t
sched_clutch_processor_queue_has_priority(processor_t    processor,
    int            priority,
    boolean_t      gte)
{
	run_queue_t bound_runq = sched_clutch_bound_runq(processor);

	int qpri = MAX(sched_clutch_root_priority(sched_clutch_processor_root_clutch(processor)), bound_runq->highq);

	if (gte) {
		return qpri >= priority;
	} else {
		return qpri > priority;
	}
}

static int
sched_clutch_runq_count(processor_t processor)
{
	return (int)sched_clutch_root_count(sched_clutch_processor_root_clutch(processor)) + sched_clutch_bound_runq(processor)->count;
}

static uint64_t
sched_clutch_runq_stats_count_sum(processor_t processor)
{
	uint64_t bound_sum = sched_clutch_bound_runq(processor)->runq_stats.count_sum;

	if (processor->cpu_id == processor->processor_set->cpu_set_low) {
		return bound_sum + sched_clutch_root_count_sum(sched_clutch_processor_root_clutch(processor));
	} else {
		return bound_sum;
	}
}
static int
sched_clutch_processor_bound_count(processor_t processor)
{
	return sched_clutch_bound_runq(processor)->count;
}

static void
sched_clutch_processor_queue_shutdown(processor_t processor)
{
	processor_set_t pset = processor->processor_set;
	sched_clutch_root_t pset_clutch_root = sched_clutch_processor_root_clutch(processor);
	thread_t        thread;
	queue_head_t    tqueue;

	                                                                                      
	if (pset->online_processor_count > 0) {
		pset_unlock(pset);
		return;
	}

	queue_init(&tqueue);
	while (sched_clutch_root_count(pset_clutch_root) > 0) {
		thread = sched_clutch_thread_highest_remove(pset_clutch_root);
		enqueue_tail(&tqueue, &thread->runq_links);
	}

	pset_unlock(pset);

	qe_foreach_element_safe(thread, &tqueue, runq_links) {
		remqueue(&thread->runq_links);
		thread_lock(thread);
		thread_setrun(thread, SCHED_TAILQ);
		thread_unlock(thread);
	}
}

static boolean_t
sched_clutch_processor_queue_remove(
	processor_t processor,
	thread_t    thread)
{
	run_queue_t             rq;
	processor_set_t         pset = processor->processor_set;

	pset_lock(pset);

	if (processor == thread->runq) {
		  
                                                   
                    
     
		if (SCHED_CLUTCH_THREAD_ELIGIBLE(thread)) {
			sched_clutch_root_t pset_clutch_root = sched_clutch_processor_root_clutch(processor);
			sched_clutch_thread_remove(pset_clutch_root, thread, mach_absolute_time(), SCHED_CLUTCH_BUCKET_OPTIONS_NONE);
		} else {
			rq = sched_clutch_thread_bound_runq(processor, thread);
			run_queue_remove(rq, thread);
		}
	} else {
		  
                                                  
                        
     
		assert(thread->runq == PROCESSOR_NULL);
		processor = PROCESSOR_NULL;
	}

	pset_unlock(pset);

	return processor != PROCESSOR_NULL;
}

static thread_t
sched_clutch_steal_thread(__unused processor_set_t pset)
{
	                                                                                  
	return THREAD_NULL;
}

static void
sched_clutch_thread_update_scan(sched_update_scan_context_t scan_context)
{
	boolean_t               restart_needed = FALSE;
	processor_t             processor = processor_list;
	processor_set_t         pset;
	thread_t                thread;
	spl_t                   s;

	  
                                                                                  
                                                       
    

	do {
		do {
			pset = processor->processor_set;

			s = splsched();
			pset_lock(pset);

			restart_needed = runq_scan(sched_clutch_bound_runq(processor), scan_context);

			pset_unlock(pset);
			splx(s);

			if (restart_needed) {
				break;
			}

			thread = processor->idle_thread;
			if (thread != THREAD_NULL && thread->sched_stamp != sched_tick) {
				if (thread_update_add_thread(thread) == FALSE) {
					restart_needed = TRUE;
					break;
				}
			}
		} while ((processor = processor->processor_list) != NULL);

		                                                             
		thread_update_process_threads();
	} while (restart_needed);

	pset_node_t node = &pset_node0;
	pset = node->psets;

	do {
		do {
			restart_needed = FALSE;
			while (pset != NULL) {
				s = splsched();
				pset_lock(pset);

				if (sched_clutch_root_count(&pset->pset_clutch_root) > 0) {
					for (sched_bucket_t bucket = TH_BUCKET_SHARE_FG; bucket < TH_BUCKET_SCHED_MAX; bucket++) {
						restart_needed = runq_scan(&pset->pset_clutch_root.scr_bound_buckets[bucket].scrb_bound_thread_runq, scan_context);
						if (restart_needed) {
							break;
						}
					}
					queue_t clutch_bucket_list = &pset->pset_clutch_root.scr_clutch_buckets;
					sched_clutch_bucket_t clutch_bucket;
					qe_foreach_element(clutch_bucket, clutch_bucket_list, scb_listlink) {
						sched_clutch_bucket_group_timeshare_update(clutch_bucket->scb_group, clutch_bucket, scan_context->sched_tick_last_abstime);
						restart_needed = sched_clutch_timeshare_scan(&clutch_bucket->scb_thread_timeshare_queue, clutch_bucket->scb_thr_count, scan_context);
					}
				}

				pset_unlock(pset);
				splx(s);

				if (restart_needed) {
					break;
				}
				pset = pset->pset_list;
			}

			if (restart_needed) {
				break;
			}
		} while (((node = node->node_list) != NULL) && ((pset = node->psets) != NULL));

		                                                             
		thread_update_process_threads();
	} while (restart_needed);
}

extern int sched_allow_rt_smt;

                                                                              
static bool
sched_clutch_thread_avoid_processor(processor_t processor, thread_t thread)
{
	if (processor->processor_primary != processor) {
		  
                                                                  
                                                                     
     
		if ((processor->processor_primary->current_pri >= BASEPRI_RTQUEUES) && ((thread->sched_pri < BASEPRI_RTQUEUES) || !sched_allow_rt_smt)) {
			return true;
		}
	}

	return false;
}

  
                                                                        
                                                   
   
static uint32_t
sched_clutch_run_incr(thread_t thread)
{
	assert((thread->state & (TH_RUN | TH_IDLE)) == TH_RUN);
	uint32_t new_count = os_atomic_inc(&sched_run_buckets[TH_BUCKET_RUN], relaxed);
	sched_clutch_thread_run_bucket_incr(thread, thread->th_sched_bucket);
	return new_count;
}

static uint32_t
sched_clutch_run_decr(thread_t thread)
{
	assert((thread->state & (TH_RUN | TH_IDLE)) != TH_RUN);
	uint32_t new_count = os_atomic_dec(&sched_run_buckets[TH_BUCKET_RUN], relaxed);
	sched_clutch_thread_run_bucket_decr(thread, thread->th_sched_bucket);
	return new_count;
}

static sched_bucket_t
sched_convert_pri_to_bucket(uint8_t priority)
{
	sched_bucket_t bucket = TH_BUCKET_RUN;

	if (priority > BASEPRI_USER_INITIATED) {
		bucket = TH_BUCKET_SHARE_FG;
	} else if (priority > BASEPRI_DEFAULT) {
		bucket = TH_BUCKET_SHARE_IN;
	} else if (priority > BASEPRI_UTILITY) {
		bucket = TH_BUCKET_SHARE_DF;
	} else if (priority > MAXPRI_THROTTLE) {
		bucket = TH_BUCKET_SHARE_UT;
	} else {
		bucket = TH_BUCKET_SHARE_BG;
	}
	return bucket;
}

  
                                                               
                                                              
                                                            
                                                                
                
   
static boolean_t
sched_thread_sched_pri_promoted(thread_t thread)
{
	return (thread->sched_flags & TH_SFLAG_PROMOTED) ||
	       (thread->sched_flags & TH_SFLAG_PROMOTE_REASON_MASK) ||
	       (thread->sched_flags & TH_SFLAG_DEMOTED_MASK) ||
	       (thread->sched_flags & TH_SFLAG_DEPRESSED_MASK) ||
	       (thread->kern_promotion_schedpri != 0);
}

  
                                                          
  
                                                              
                                                           
                                                              
                                                                 
                                                             
  
                                                                 
                                                                 
                                                               
                                                                     
                                                 
   

void
sched_clutch_update_thread_bucket(thread_t thread)
{
	sched_bucket_t old_bucket = thread->th_sched_bucket;
	sched_bucket_t new_bucket = TH_BUCKET_RUN;
	assert(thread->runq == PROCESSOR_NULL);
	int pri = (sched_thread_sched_pri_promoted(thread)) ? thread->sched_pri : thread->base_pri;

	switch (thread->sched_mode) {
	case TH_MODE_FIXED:
		if (pri >= BASEPRI_FOREGROUND) {
			new_bucket = TH_BUCKET_FIXPRI;
		} else {
			new_bucket = sched_convert_pri_to_bucket(pri);
		}
		break;

	case TH_MODE_REALTIME:
		new_bucket = TH_BUCKET_FIXPRI;
		break;

	case TH_MODE_TIMESHARE:
		new_bucket = sched_convert_pri_to_bucket(pri);
		break;

	default:
		panic("unexpected mode: %d", thread->sched_mode);
		break;
	}

	if (old_bucket == new_bucket) {
		return;
	}

	thread->th_sched_bucket = new_bucket;
	thread->pri_shift = sched_clutch_thread_pri_shift(thread, new_bucket);
	  
                                                                         
                                                                       
                                                            
    
	if ((thread->state & (TH_RUN | TH_IDLE)) == TH_RUN) {
		sched_clutch_thread_run_bucket_decr(thread, old_bucket);
		sched_clutch_thread_run_bucket_incr(thread, new_bucket);
	}
}

#if CONFIG_SCHED_EDGE

                                                               

static void
sched_edge_init(void);

static thread_t
sched_edge_processor_idle(processor_set_t pset);

static ast_t
sched_edge_processor_csw_check(processor_t processor);

static boolean_t
sched_edge_processor_queue_has_priority(processor_t processor, int priority, boolean_t gte);

static boolean_t
sched_edge_processor_queue_empty(processor_t processor);

static thread_t
sched_edge_choose_thread(processor_t processor, int priority, ast_t reason);

static void
sched_edge_processor_queue_shutdown(processor_t processor);

static processor_t
sched_edge_choose_processor(processor_set_t pset, processor_t processor, thread_t thread);

static bool
sched_edge_thread_avoid_processor(processor_t processor, thread_t thread);

static void
sched_edge_balance(processor_t cprocessor, processor_set_t cpset);

static void
sched_edge_check_spill(processor_set_t pset, thread_t thread);

static bool
sched_edge_thread_should_yield(processor_t processor, thread_t thread);

static void
sched_edge_pset_made_schedulable(processor_t processor, processor_set_t dst_pset, boolean_t drop_lock);

static bool
sched_edge_steal_thread_enabled(processor_set_t pset);

static sched_ipi_type_t
sched_edge_ipi_policy(processor_t dst, thread_t thread, boolean_t dst_idle, sched_ipi_event_t event);

static uint32_t
sched_edge_qos_max_parallelism(int qos, uint64_t options);

const struct sched_dispatch_table sched_edge_dispatch = {
	.sched_name                                     = "edge",
	.init                                           = sched_edge_init,
	.timebase_init                                  = sched_timeshare_timebase_init,
	.processor_init                                 = sched_clutch_processor_init,
	.pset_init                                      = sched_clutch_pset_init,
	.maintenance_continuation                       = sched_timeshare_maintenance_continue,
	.choose_thread                                  = sched_edge_choose_thread,
	.steal_thread_enabled                           = sched_edge_steal_thread_enabled,
	.steal_thread                                   = sched_edge_processor_idle,
	.compute_timeshare_priority                     = sched_compute_timeshare_priority,
	.choose_node                                    = sched_choose_node,
	.choose_processor                               = sched_edge_choose_processor,
	.processor_enqueue                              = sched_clutch_processor_enqueue,
	.processor_queue_shutdown                       = sched_edge_processor_queue_shutdown,
	.processor_queue_remove                         = sched_clutch_processor_queue_remove,
	.processor_queue_empty                          = sched_edge_processor_queue_empty,
	.priority_is_urgent                             = priority_is_urgent,
	.processor_csw_check                            = sched_edge_processor_csw_check,
	.processor_queue_has_priority                   = sched_edge_processor_queue_has_priority,
	.initial_quantum_size                           = sched_clutch_initial_quantum_size,
	.initial_thread_sched_mode                      = sched_clutch_initial_thread_sched_mode,
	.can_update_priority                            = can_update_priority,
	.update_priority                                = update_priority,
	.lightweight_update_priority                    = lightweight_update_priority,
	.quantum_expire                                 = sched_default_quantum_expire,
	.processor_runq_count                           = sched_clutch_runq_count,
	.processor_runq_stats_count_sum                 = sched_clutch_runq_stats_count_sum,
	.processor_bound_count                          = sched_clutch_processor_bound_count,
	.thread_update_scan                             = sched_clutch_thread_update_scan,
	.multiple_psets_enabled                         = TRUE,
	.sched_groups_enabled                           = FALSE,
	.avoid_processor_enabled                        = TRUE,
	.thread_avoid_processor                         = sched_edge_thread_avoid_processor,
	.processor_balance                              = sched_edge_balance,

	.rt_runq                                        = sched_amp_rt_runq,
	.rt_init                                        = sched_amp_rt_init,
	.rt_queue_shutdown                              = sched_amp_rt_queue_shutdown,
	.rt_runq_scan                                   = sched_amp_rt_runq_scan,
	.rt_runq_count_sum                              = sched_amp_rt_runq_count_sum,

	.qos_max_parallelism                            = sched_edge_qos_max_parallelism,
	.check_spill                                    = sched_edge_check_spill,
	.ipi_policy                                     = sched_edge_ipi_policy,
	.thread_should_yield                            = sched_edge_thread_should_yield,
	.run_count_incr                                 = sched_clutch_run_incr,
	.run_count_decr                                 = sched_clutch_run_decr,
	.update_thread_bucket                           = sched_clutch_update_thread_bucket,
	.pset_made_schedulable                          = sched_edge_pset_made_schedulable,
	.thread_group_recommendation_change             = NULL,
};

static struct processor_set pset1;
static struct pset_node pset_node1;
static bitmap_t sched_edge_available_pset_bitmask[BITMAP_LEN(MAX_PSETS)];

  
                              
  
                                                              
   
static bool
sched_edge_pset_available(processor_set_t pset)
{
	return bitmap_test(sched_edge_available_pset_bitmask, pset->pset_cluster_id);
}

  
                                       
  
                                                                           
                                                                      
  
                                      
   
static uint32_t
sched_edge_thread_bound_cluster_id(thread_t thread)
{
	assert(SCHED_CLUTCH_THREAD_CLUSTER_BOUND(thread));
	if (thread->sched_flags & TH_SFLAG_ECORE_ONLY) {
		return (pset_array[0]->pset_type == CLUSTER_TYPE_E) ? 0 : 1;
	} else {
		return (pset_array[0]->pset_type == CLUSTER_TYPE_P) ? 0 : 1;
	}
}

                                                            
static boolean_t sched_edge_foreign_runnable_thread_available(processor_set_t pset);
static boolean_t sched_edge_foreign_running_thread_available(processor_set_t pset);
static processor_set_t sched_edge_steal_candidate(processor_set_t pset);
static processor_set_t sched_edge_migrate_candidate(processor_set_t preferred_pset, thread_t thread, processor_set_t locked_pset, bool switch_pset_locks);

  
                          
  
                                                                                             
                             
   
static void
sched_edge_config_set(uint32_t src_cluster, uint32_t dst_cluster, sched_clutch_edge edge_config)
{
	sched_clutch_edge *edge = &pset_array[src_cluster]->sched_edges[dst_cluster];
	edge->sce_edge_packed = edge_config.sce_edge_packed;
}

  
                          
  
                                                                                               
                               
   
static sched_clutch_edge
sched_edge_config_get(uint32_t src_cluster, uint32_t dst_cluster)
{
	return pset_array[src_cluster]->sched_edges[dst_cluster];
}

#if DEVELOPMENT || DEBUG

  
                                                          
  
                                                                
                                      
   

kern_return_t
sched_edge_sysctl_configure_e_to_p(uint64_t edge_config)
{
	pset_array[ecore_cluster_id]->sched_edges[pcore_cluster_id].sce_edge_packed = edge_config;
	return KERN_SUCCESS;
}

kern_return_t
sched_edge_sysctl_configure_p_to_e(uint64_t edge_config)
{
	pset_array[pcore_cluster_id]->sched_edges[ecore_cluster_id].sce_edge_packed = edge_config;
	return KERN_SUCCESS;
}

sched_clutch_edge
sched_edge_e_to_p(void)
{
	return sched_edge_config_get(ecore_cluster_id, pcore_cluster_id);
}

sched_clutch_edge
sched_edge_p_to_e(void)
{
	return sched_edge_config_get(pcore_cluster_id, ecore_cluster_id);
}

#endif                           

  
                          
  
                                                                                      
                                                                                       
                                                                                
   
void
sched_edge_matrix_set(sched_clutch_edge *edge_matrix, bool *edge_changes_bitmap, __unused uint64_t flags, uint64_t matrix_order)
{
	uint32_t edge_index = 0;
	for (uint32_t src_cluster = 0; src_cluster < matrix_order; src_cluster++) {
		for (uint32_t dst_cluster = 0; dst_cluster < matrix_order; dst_cluster++) {
			if (edge_changes_bitmap[edge_index]) {
				sched_edge_config_set(src_cluster, dst_cluster, edge_matrix[edge_index]);
			}
			edge_index++;
		}
	}
}

  
                          
  
                                                                                        
                                                                                         
                                                                                
   
void
sched_edge_matrix_get(sched_clutch_edge *edge_matrix, bool *edge_request_bitmap, __unused uint64_t flags, uint64_t matrix_order)
{
	uint32_t edge_index = 0;
	for (uint32_t src_cluster = 0; src_cluster < matrix_order; src_cluster++) {
		for (uint32_t dst_cluster = 0; dst_cluster < matrix_order; dst_cluster++) {
			if (edge_request_bitmap[edge_index]) {
				edge_matrix[edge_index] = sched_edge_config_get(src_cluster, dst_cluster);
			}
			edge_index++;
		}
	}
}

  
                    
  
                                                                                               
                                                                                                  
                                                                                          
            
                                      
   
static void
sched_edge_init(void)
{
	processor_set_t ecore_set = &pset0;
	processor_set_t pcore_set = &pset1;

	if (ml_get_boot_cluster() == CLUSTER_TYPE_P) {
		                                                            
		pcore_set = &pset0;
		ecore_set = &pset1;
		bitmap_set(sched_edge_available_pset_bitmask, pcore_cluster_id);
	} else {
		bitmap_set(sched_edge_available_pset_bitmask, ecore_cluster_id);
	}

	ecore_set->pset_cluster_type = PSET_AMP_E;
	ecore_set->pset_cluster_id = ecore_cluster_id;

	pcore_set->pset_cluster_type = PSET_AMP_P;
	pcore_set->pset_cluster_id = pcore_cluster_id;

	pset_init(&pset1, &pset_node1);
	pset_node1.psets = &pset1;
	pset_node0.node_list = &pset_node1;

	pset_array[ecore_cluster_id] = ecore_set;
	pset_array[ecore_cluster_id]->pset_type = CLUSTER_TYPE_E;
	bitmap_set(pset_array[ecore_cluster_id]->foreign_psets, pcore_cluster_id);

	sched_edge_config_set(ecore_cluster_id, ecore_cluster_id, (sched_clutch_edge){.sce_migration_weight = 0, .sce_migration_allowed = 0, .sce_steal_allowed = 0});
	sched_edge_config_set(ecore_cluster_id, pcore_cluster_id, (sched_clutch_edge){.sce_migration_weight = 0, .sce_migration_allowed = 0, .sce_steal_allowed = 0});

	pset_array[pcore_cluster_id] = pcore_set;
	pset_array[pcore_cluster_id]->pset_type = CLUSTER_TYPE_P;
	bitmap_set(pset_array[pcore_cluster_id]->foreign_psets, ecore_cluster_id);

	sched_edge_config_set(pcore_cluster_id, pcore_cluster_id, (sched_clutch_edge){.sce_migration_weight = 0, .sce_migration_allowed = 0, .sce_steal_allowed = 0});
	sched_edge_config_set(pcore_cluster_id, ecore_cluster_id, (sched_clutch_edge){.sce_migration_weight = 64, .sce_migration_allowed = 1, .sce_steal_allowed = 1});

	sched_timeshare_init();
	sched_clutch_tunables_init();
}

static thread_t
sched_edge_choose_thread(
	processor_t      processor,
	int              priority,
	__unused ast_t            reason)
{
	int clutch_pri = sched_clutch_root_priority(sched_clutch_processor_root_clutch(processor));
	run_queue_t bound_runq = sched_clutch_bound_runq(processor);
	boolean_t choose_from_boundq = false;

	if ((bound_runq->highq < priority) &&
	    (clutch_pri < priority)) {
		return THREAD_NULL;
	}

	if (bound_runq->highq >= clutch_pri) {
		choose_from_boundq = true;
	}

	thread_t thread = THREAD_NULL;
	if (choose_from_boundq == false) {
		sched_clutch_root_t pset_clutch_root = sched_clutch_processor_root_clutch(processor);
		thread = sched_clutch_thread_highest_remove(pset_clutch_root);
	} else {
		thread = run_queue_dequeue(bound_runq, SCHED_HEADQ);
	}
	return thread;
}

static boolean_t
sched_edge_processor_queue_empty(processor_t processor)
{
	return (sched_clutch_root_count(sched_clutch_processor_root_clutch(processor)) == 0) &&
	       (sched_clutch_bound_runq(processor)->count == 0);
}

static void
sched_edge_check_spill(__unused processor_set_t pset, __unused thread_t thread)
{
	assert(thread->bound_processor == PROCESSOR_NULL);
}

__options_decl(sched_edge_thread_yield_reason_t, uint32_t, {
	SCHED_EDGE_YIELD_RUNQ_NONEMPTY       = 0x0,
	SCHED_EDGE_YIELD_FOREIGN_RUNNABLE    = 0x1,
	SCHED_EDGE_YIELD_FOREIGN_RUNNING     = 0x2,
	SCHED_EDGE_YIELD_STEAL_POSSIBLE      = 0x3,
	SCHED_EDGE_YIELD_DISALLOW            = 0x4,
});

static bool
sched_edge_thread_should_yield(processor_t processor, __unused thread_t thread)
{
	if (!sched_edge_processor_queue_empty(processor) || (rt_runq_count(processor->processor_set) > 0)) {
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_EDGE_SHOULD_YIELD) | DBG_FUNC_NONE,
		    thread_tid(thread), processor->processor_set->pset_cluster_id, 0, SCHED_EDGE_YIELD_RUNQ_NONEMPTY);
		return true;
	}

	  
                                                                               
                                                                            
                                                                            
                                                                              
                         
    
	if (sched_edge_foreign_runnable_thread_available(processor->processor_set)) {
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_EDGE_SHOULD_YIELD) | DBG_FUNC_NONE,
		    thread_tid(thread), processor->processor_set->pset_cluster_id, 0, SCHED_EDGE_YIELD_FOREIGN_RUNNABLE);
		return true;
	}
	if (sched_edge_foreign_running_thread_available(processor->processor_set)) {
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_EDGE_SHOULD_YIELD) | DBG_FUNC_NONE,
		    thread_tid(thread), processor->processor_set->pset_cluster_id, 0, SCHED_EDGE_YIELD_FOREIGN_RUNNING);
		return true;
	}

	processor_set_t steal_candidate = sched_edge_steal_candidate(processor->processor_set);
	if (steal_candidate != NULL) {
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_EDGE_SHOULD_YIELD) | DBG_FUNC_NONE,
		    thread_tid(thread), processor->processor_set->pset_cluster_id, 0, SCHED_EDGE_YIELD_STEAL_POSSIBLE);
		return true;
	}

	KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_EDGE_SHOULD_YIELD) | DBG_FUNC_NONE, thread_tid(thread), processor->processor_set->pset_cluster_id,
	    0, SCHED_EDGE_YIELD_DISALLOW);
	return false;
}

static ast_t
sched_edge_processor_csw_check(processor_t processor)
{
	boolean_t       has_higher;
	int             pri;

	int clutch_pri = sched_clutch_root_priority(sched_clutch_processor_root_clutch(processor));
	run_queue_t bound_runq = sched_clutch_bound_runq(processor);

	assert(processor->active_thread != NULL);

	pri = MAX(clutch_pri, bound_runq->highq);

	if (processor->first_timeslice) {
		has_higher = (pri > processor->current_pri);
	} else {
		has_higher = (pri >= processor->current_pri);
	}

	if (has_higher) {
		if (sched_clutch_root_urgency(sched_clutch_processor_root_clutch(processor)) > 0) {
			return AST_PREEMPT | AST_URGENT;
		}

		if (bound_runq->urgency > 0) {
			return AST_PREEMPT | AST_URGENT;
		}

		return AST_PREEMPT;
	}

	return AST_NONE;
}

static boolean_t
sched_edge_processor_queue_has_priority(processor_t    processor,
    int            priority,
    boolean_t      gte)
{
	run_queue_t bound_runq = sched_clutch_bound_runq(processor);

	int qpri = MAX(sched_clutch_root_priority(sched_clutch_processor_root_clutch(processor)), bound_runq->highq);
	if (gte) {
		return qpri >= priority;
	} else {
		return qpri > priority;
	}
}

static void
sched_edge_processor_queue_shutdown(processor_t processor)
{
	processor_set_t pset = processor->processor_set;
	sched_clutch_root_t pset_clutch_root = sched_clutch_processor_root_clutch(processor);
	thread_t        thread;
	queue_head_t    tqueue;

	                                                                                                          
	if ((pset->online_processor_count > 0) && pset_is_recommended(pset)) {
		pset_unlock(pset);
		return;
	}

	bitmap_clear(sched_edge_available_pset_bitmask, pset->pset_cluster_id);

	queue_init(&tqueue);
	while (sched_clutch_root_count(pset_clutch_root) > 0) {
		thread = sched_clutch_thread_highest_remove(pset_clutch_root);
		enqueue_tail(&tqueue, &thread->runq_links);
	}
	pset_unlock(pset);

	qe_foreach_element_safe(thread, &tqueue, runq_links) {
		remqueue(&thread->runq_links);
		thread_lock(thread);
		thread_setrun(thread, SCHED_TAILQ);
		thread_unlock(thread);
	}
}

  
                                   
  
                                                                               
                                                                                
                                                                                    
                                             
                                                                                   
                                                                          
                                                                                       
                                                                      
   
static uint32_t
sched_edge_cluster_load_metric(processor_set_t pset, sched_bucket_t sched_bucket)
{
	if (sched_edge_pset_available(pset) == false) {
		return UINT32_MAX;
	}
	return (uint32_t)sched_get_pset_load_average(pset, sched_bucket);
}

  
  
                                       
  
                              
  
                                                                                   
                                                                                
                                                                              
                 
  
                                                                                 
                                                                                    
                                        
  
                                             
  
                                                                                   
                                                         
                                                           
                                                              
                                                             
                                                               
                                                                 
                                                                                            
                                                        
                                          
  
                                                  
  
                                                                 
                                                                                   
  
                                                
  
                                                                  
                                                                      
                                                        
                                                                       
                                                
                                                                      
                                 
                                                                          
                          
  
                                 
                                                                                              
                                                                                     
                          
   

static bool
sched_edge_steal_thread_enabled(__unused processor_set_t pset)
{
	  
                                                                                          
    
	return true;
}

static processor_set_t
sched_edge_steal_candidate(processor_set_t pset)
{
	  
                               
   
                                                                              
                                                                                 
                                                                              
                                                                                   
   
                                                                                 
                                                                                  
                                                                                
                                                                            
                      
    
	processor_set_t target_pset = NULL;
	uint32_t dst_cluster_id = pset->pset_cluster_id;

	for (int cluster_id = 0; cluster_id < MAX_PSETS; cluster_id++) {
		processor_set_t candidate_pset = pset_array[cluster_id];

		if (candidate_pset == pset) {
			continue;
		}

		sched_clutch_edge *incoming_edge = &pset_array[cluster_id]->sched_edges[dst_cluster_id];
		if (incoming_edge->sce_steal_allowed == false) {
			continue;
		}

		uint32_t incoming_weight = incoming_edge->sce_migration_weight;
		int highest_runnable_bucket = bitmap_lsb_first(candidate_pset->pset_clutch_root.scr_unbound_runnable_bitmap, TH_BUCKET_SCHED_MAX);
		if (highest_runnable_bucket == -1) {
			                                     
			continue;
		}
		                                                                                      
		uint32_t candidate_load = sched_edge_cluster_load_metric(candidate_pset, (sched_bucket_t)highest_runnable_bucket);
		if (candidate_load > incoming_weight) {
			                                                                                                            
			target_pset = candidate_pset;
			break;
		}
	}

	return target_pset;
}

static boolean_t
sched_edge_foreign_runnable_thread_available(processor_set_t pset)
{
	                                                             
	bitmap_t *foreign_pset_bitmap = pset->foreign_psets;
	for (int cluster = bitmap_first(foreign_pset_bitmap, MAX_PSETS); cluster >= 0; cluster = bitmap_next(foreign_pset_bitmap, cluster)) {
		  
                                                                     
                                                                                  
                     
     
		processor_set_t target_pset = pset_array[cluster];
		if (sched_edge_pset_available(target_pset) == false) {
			continue;
		}

		if (!sched_clutch_root_foreign_empty(&target_pset->pset_clutch_root)) {
			return true;
		}
	}
	return false;
}

static thread_t
sched_edge_foreign_runnable_thread_remove(processor_set_t pset, uint64_t ctime)
{
	thread_t thread = THREAD_NULL;

	                                                             
	bitmap_t *foreign_pset_bitmap = pset->foreign_psets;
	for (int cluster = bitmap_first(foreign_pset_bitmap, MAX_PSETS); cluster >= 0; cluster = bitmap_next(foreign_pset_bitmap, cluster)) {
		  
                                                                     
                                                                                  
                     
     
		processor_set_t target_pset = pset_array[cluster];
		if (sched_edge_pset_available(target_pset) == false) {
			continue;
		}

		if (sched_clutch_root_foreign_empty(&target_pset->pset_clutch_root)) {
			continue;
		}
		  
                                                                                  
                                         
     
		pset_lock(target_pset);
		if (sched_edge_pset_available(target_pset)) {
			thread = sched_clutch_root_highest_foreign_thread_remove(&target_pset->pset_clutch_root);
			sched_update_pset_load_average(target_pset, ctime);
		}
		pset_unlock(target_pset);

		  
                                
    
                                                                                 
                                                                                
                                                                                 
                                                                               
     
		if (thread != THREAD_NULL) {
			KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_EDGE_REBAL_RUNNABLE) | DBG_FUNC_NONE, thread_tid(thread), pset->pset_cluster_id, target_pset->pset_cluster_id, 0);
			break;
		}
		                                                                                                           
	}

	return thread;
}

static boolean_t
sched_edge_foreign_running_thread_available(processor_set_t pset)
{
	bitmap_t *foreign_pset_bitmap = pset->foreign_psets;
	for (int cluster = bitmap_first(foreign_pset_bitmap, MAX_PSETS); cluster >= 0; cluster = bitmap_next(foreign_pset_bitmap, cluster)) {
		                                          
		processor_set_t target_pset = pset_array[cluster];
		if (sched_edge_pset_available(target_pset) == false) {
			continue;
		}

		uint64_t running_foreign_bitmap = target_pset->cpu_state_map[PROCESSOR_RUNNING] & target_pset->cpu_running_foreign;
		if (lsb_first(running_foreign_bitmap) != -1) {
			                                                                          
			return true;
		}
	}
	return false;
}

static thread_t
sched_edge_steal_thread(processor_set_t pset)
{
	thread_t thread = THREAD_NULL;
	processor_set_t steal_from_pset = sched_edge_steal_candidate(pset);
	if (steal_from_pset) {
		  
                                                                                
                                                                  
     
		pset_lock(steal_from_pset);
		if (bitmap_first(steal_from_pset->pset_clutch_root.scr_unbound_runnable_bitmap, TH_BUCKET_SCHED_MAX) != -1) {
			uint64_t current_timestamp = mach_absolute_time();
			sched_clutch_root_bucket_t root_bucket = sched_clutch_root_highest_root_bucket(&steal_from_pset->pset_clutch_root, current_timestamp, SCHED_CLUTCH_HIGHEST_ROOT_BUCKET_UNBOUND_ONLY);
			thread = sched_clutch_thread_unbound_lookup(&steal_from_pset->pset_clutch_root, root_bucket);
			sched_clutch_thread_remove(&steal_from_pset->pset_clutch_root, thread, current_timestamp, SCHED_CLUTCH_BUCKET_OPTIONS_SAMEPRI_RR);
			KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_EDGE_STEAL) | DBG_FUNC_NONE, thread_tid(thread), pset->pset_cluster_id, steal_from_pset->pset_cluster_id, 0);
			sched_update_pset_load_average(steal_from_pset, current_timestamp);
		}
		  
                                
                                                                                         
                                         
     
		pset_unlock(steal_from_pset);
	}
	return thread;
}

  
                              
  
                                                                               
   
static thread_t
sched_edge_processor_idle(processor_set_t pset)
{
	thread_t thread = THREAD_NULL;

	uint64_t ctime = mach_absolute_time();

	                                                                      
	pset_unlock(pset);

	                                                                      
	thread = sched_edge_foreign_runnable_thread_remove(pset, ctime);
	if (thread != THREAD_NULL) {
		return thread;
	}

	                                                                                                     
	boolean_t rebalance_needed = sched_edge_foreign_running_thread_available(pset);
	if (rebalance_needed) {
		return THREAD_NULL;
	}

	                                                                                              
	thread = sched_edge_steal_thread(pset);
	return thread;
}

                                                                              
static bool
sched_edge_thread_avoid_processor(processor_t processor, thread_t thread)
{
	processor_set_t preferred_pset = pset_array[sched_edge_thread_preferred_cluster(thread)];
	  
                                                                                    
                                                                                   
                                                                                   
                                                  
   
                                       
                                                                                 
                               
    
	if (processor->processor_set->pset_type != preferred_pset->pset_type) {
		return true;
	}
	                                                                  
	if (processor->processor_set == preferred_pset) {
		return false;
	}
	  
                                                                        
                                                                      
                                                                        
                                             
   
                                       
                                                                             
                                                                                   
                        
    
	processor_set_t chosen_pset = sched_edge_migrate_candidate(preferred_pset, thread, processor->processor_set, false);
	return chosen_pset != processor->processor_set;
}

static void
sched_edge_balance(__unused processor_t cprocessor, processor_set_t cpset)
{
	assert(cprocessor == current_processor());
	pset_unlock(cpset);

	uint64_t ast_processor_map = 0;
	sched_ipi_type_t ipi_type[MAX_CPUS] = {SCHED_IPI_NONE};

	bitmap_t *foreign_pset_bitmap = cpset->foreign_psets;
	for (int cluster = bitmap_first(foreign_pset_bitmap, MAX_PSETS); cluster >= 0; cluster = bitmap_next(foreign_pset_bitmap, cluster)) {
		                                          
		processor_set_t target_pset = pset_array[cluster];
		if (sched_edge_pset_available(target_pset) == false) {
			continue;
		}

		pset_lock(target_pset);
		uint64_t cpu_running_foreign_map = (target_pset->cpu_running_foreign & target_pset->cpu_state_map[PROCESSOR_RUNNING]);
		for (int cpuid = lsb_first(cpu_running_foreign_map); cpuid >= 0; cpuid = lsb_next(cpu_running_foreign_map, cpuid)) {
			processor_t target_cpu = processor_array[cpuid];
			ipi_type[target_cpu->cpu_id] = sched_ipi_action(target_cpu, NULL, false, SCHED_IPI_EVENT_REBALANCE);
			if (ipi_type[cpuid] != SCHED_IPI_NONE) {
				bit_set(ast_processor_map, cpuid);
			}
		}
		pset_unlock(target_pset);
	}

	for (int cpuid = lsb_first(ast_processor_map); cpuid >= 0; cpuid = lsb_next(ast_processor_map, cpuid)) {
		processor_t ast_processor = processor_array[cpuid];
		sched_ipi_perform(ast_processor, ipi_type[cpuid]);
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_EDGE_REBAL_RUNNING) | DBG_FUNC_NONE, 0, cprocessor->cpu_id, cpuid, 0);
	}
}

  
                                      
  
                                                                            
  
                                                                                                                 
                                                                           
   
static processor_set_t
sched_edge_migrate_edges_evaluate(processor_set_t preferred_pset, uint32_t preferred_cluster_load, thread_t thread)
{
	processor_set_t selected_pset = preferred_pset;
	uint32_t preferred_cluster_id = preferred_pset->pset_cluster_id;
	cluster_type_t preferred_cluster_type = pset_type_for_id(preferred_cluster_id);

	                                                                                   
	sched_clutch_edge *edge = preferred_pset->sched_edges;
	uint32_t max_edge_delta = 0;

	  
                               
   
                                                                               
                                                                              
                                                                          
    
	for (uint32_t cluster_id = 0; cluster_id < MAX_PSETS; cluster_id++) {
		processor_set_t dst_pset = pset_array[cluster_id];
		if (cluster_id == preferred_cluster_id) {
			continue;
		}

		if (edge[cluster_id].sce_migration_allowed == false) {
			continue;
		}

		uint32_t dst_load = sched_edge_cluster_load_metric(dst_pset, thread->th_sched_bucket);
		if (dst_load > preferred_cluster_load) {
			continue;
		}

		  
                                   
    
                                                                 
                                                                   
          
                                        
    
                                                                            
                    
     
		if (dst_load == 0) {
			selected_pset = dst_pset;
			break;
		}

		uint32_t edge_delta = preferred_cluster_load - dst_load;
		if (edge_delta < edge[cluster_id].sce_migration_weight) {
			continue;
		}

		if (edge_delta < max_edge_delta) {
			continue;
		}

		if (edge_delta == max_edge_delta) {
			                                                                                               
			boolean_t selected_homogeneous = (pset_type_for_id(selected_pset->pset_cluster_id) == preferred_cluster_type);
			boolean_t candidate_homogeneous = (pset_type_for_id(dst_pset->pset_cluster_id) == preferred_cluster_type);
			if (selected_homogeneous || !candidate_homogeneous) {
				continue;
			}
		}
		                                                           
		max_edge_delta = edge_delta;
		selected_pset = dst_pset;
	}
	return selected_pset;
}

  
                                     
  
                                                                                 
                                                                                      
                                                                              
                       
   
_Static_assert(MAX_PSETS <= 64, "Unable to fit maximum number of psets in uint64_t bitmask");
static processor_set_t
sched_edge_candidate_alternative(processor_set_t selected_pset, uint64_t candidate_cluster_bitmap)
{
	  
                                                                                
                                                              
    
	bitmap_t *foreign_clusters = selected_pset->foreign_psets;
	uint64_t available_native_clusters = ~(foreign_clusters[0]) & candidate_cluster_bitmap;
	int available_cluster_id = lsb_first(available_native_clusters);
	if (available_cluster_id == -1) {
		                                                                                                 
		available_cluster_id = bit_first(candidate_cluster_bitmap);
	}
	assert(available_cluster_id != -1);
	return pset_array[available_cluster_id];
}

  
                                
  
                                                                                                   
                     
                                                  
   
static processor_set_t
sched_edge_switch_pset_lock(processor_set_t selected_pset, processor_set_t locked_pset, bool switch_pset_locks)
{
	if (!switch_pset_locks) {
		return locked_pset;
	}
	if (selected_pset != locked_pset) {
		pset_unlock(locked_pset);
		pset_lock(selected_pset);
		return selected_pset;
	} else {
		return locked_pset;
	}
}

  
                                 
  
                                                                                                         
                                                                                            
  
                                                                                                     
                                                                                                                   
                                                                                                     
                                                                                                       
                                                                                                     
                                                                                                      
                            
  
                                                                                                          
                               
   
static processor_set_t
sched_edge_migrate_candidate(processor_set_t preferred_pset, thread_t thread, processor_set_t locked_pset, bool switch_pset_locks)
{
	__kdebug_only uint32_t preferred_cluster_id = preferred_pset->pset_cluster_id;
	processor_set_t selected_pset = preferred_pset;

	if (SCHED_CLUTCH_THREAD_CLUSTER_BOUND(thread)) {
		                                                                 
		selected_pset = pset_array[sched_edge_thread_bound_cluster_id(thread)];
		locked_pset = sched_edge_switch_pset_lock(selected_pset, locked_pset, switch_pset_locks);
		if (sched_edge_pset_available(selected_pset) || (SCHED_CLUTCH_THREAD_CLUSTER_BOUND_SOFT(thread) == false)) {
			  
                                                                                                       
                                                                                            
                                                                                       
      
			return selected_pset;
		}
	}

	uint64_t candidate_cluster_bitmap = mask(MAX_PSETS);
	if (thread->sched_pri >= BASEPRI_RTQUEUES) {
		                                                                              
		goto migrate_candidate_available_check;
	}

	  
                                                                                       
                                                 
    
	uint32_t preferred_cluster_load = sched_edge_cluster_load_metric(preferred_pset, thread->th_sched_bucket);
	boolean_t amp_rebalance = (thread->reason & (AST_REBALANCE | AST_QUANTUM)) == (AST_REBALANCE | AST_QUANTUM);
	if ((preferred_cluster_load == 0) || amp_rebalance) {
		goto migrate_candidate_available_check;
	}

	                                                                                       
	selected_pset = sched_edge_migrate_edges_evaluate(preferred_pset, preferred_cluster_load, thread);

migrate_candidate_available_check:
	locked_pset = sched_edge_switch_pset_lock(selected_pset, locked_pset, switch_pset_locks);
	if (sched_edge_pset_available(selected_pset) == true) {
		if (selected_pset != preferred_pset) {
			KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_EDGE_CLUSTER_OVERLOAD) | DBG_FUNC_NONE, thread_tid(thread), preferred_cluster_id, selected_pset->pset_cluster_id, preferred_cluster_load);
		}
		return selected_pset;
	}
	                                                                                                       
	bitmap_clear(&candidate_cluster_bitmap, selected_pset->pset_cluster_id);
	if (__improbable(bitmap_first(&candidate_cluster_bitmap, MAX_PSETS) == -1)) {
		  
                                                                                                        
                                    
     
		selected_pset = &pset0;
		locked_pset = sched_edge_switch_pset_lock(selected_pset, locked_pset, switch_pset_locks);
		if (selected_pset != preferred_pset) {
			KDBG(MACHDBG_CODE(DBG_MACH_SCHED_CLUTCH, MACH_SCHED_EDGE_CLUSTER_OVERLOAD) | DBG_FUNC_NONE, thread_tid(thread), preferred_cluster_id, selected_pset->pset_cluster_id, preferred_cluster_load);
		}
		return selected_pset;
	}
	                                                       
	selected_pset = sched_edge_candidate_alternative(selected_pset, candidate_cluster_bitmap);
	goto migrate_candidate_available_check;
}

static processor_t
sched_edge_choose_processor(processor_set_t pset, processor_t processor, thread_t thread)
{
	                                            
	assert(thread->bound_processor == PROCESSOR_NULL);
	processor_t chosen_processor = PROCESSOR_NULL;

	  
                                                                              
                                                                                         
                         
    
	processor_set_t preferred_pset = pset_array[sched_edge_thread_preferred_cluster(thread)];
	processor_set_t chosen_pset = preferred_pset;
	  
                                                                                           
                                                                         
                                                                                          
   
                                       
                                                                                         
                                             
    
	chosen_pset = sched_edge_migrate_candidate(preferred_pset, thread, pset, true);
	chosen_processor = choose_processor(chosen_pset, processor, thread);
	assert(chosen_processor->processor_set == chosen_pset);
	return chosen_processor;
}

  
                                           
  
                                                                                             
                                                    
   
static void
sched_edge_clutch_bucket_threads_drain(sched_clutch_bucket_t clutch_bucket, sched_clutch_root_t root_clutch, queue_t clutch_threads)
{
	thread_t thread = THREAD_NULL;
	uint64_t current_timestamp = mach_approximate_time();
	qe_foreach_element_safe(thread, &clutch_bucket->scb_thread_timeshare_queue, th_clutch_timeshare_link) {
		sched_clutch_thread_remove(root_clutch, thread, current_timestamp, SCHED_CLUTCH_BUCKET_OPTIONS_NONE);
		enqueue_tail(clutch_threads, &thread->runq_links);
	}
}

  
                                   
  
                                                       
   
static void
sched_edge_run_drained_threads(queue_t clutch_threads)
{
	thread_t thread;
	                                                   
	qe_foreach_element_safe(thread, clutch_threads, runq_links) {
		remqueue(&thread->runq_links);
		thread_lock(thread);
		thread_setrun(thread, SCHED_TAILQ);
		thread_unlock(thread);
	}
}

  
                                        
  
                                                                                 
                                                                                    
   
static void
sched_edge_update_preferred_cluster(
	sched_clutch_t sched_clutch,
	bitmap_t *clutch_bucket_modify_bitmap,
	uint32_t *tg_bucket_preferred_cluster)
{
	for (int bucket = bitmap_first(clutch_bucket_modify_bitmap, TH_BUCKET_SCHED_MAX); bucket >= 0; bucket = bitmap_next(clutch_bucket_modify_bitmap, bucket)) {
		os_atomic_store(&sched_clutch->sc_clutch_groups[bucket].scbg_preferred_cluster, tg_bucket_preferred_cluster[bucket], relaxed);
	}
}

  
                                                     
  
                                                                                   
                                                                 
             
  
                                                                                    
                                                                                    
                                                                                   
               
  
                                                                                 
                                                                              
  
                                                                                
                                                  
  
                                                         
                                    
   
static void
sched_edge_migrate_thread_group_runnable_threads(
	sched_clutch_t sched_clutch,
	sched_clutch_root_t root_clutch,
	bitmap_t *clutch_bucket_modify_bitmap,
	__unused uint32_t *tg_bucket_preferred_cluster,
	bool migrate_immediately)
{
	                                                                                     
	queue_head_t clutch_threads;
	queue_init(&clutch_threads);

	for (int bucket = bitmap_first(clutch_bucket_modify_bitmap, TH_BUCKET_SCHED_MAX); bucket >= 0; bucket = bitmap_next(clutch_bucket_modify_bitmap, bucket)) {
		                                                             
		sched_clutch_bucket_group_t clutch_bucket_group = &(sched_clutch->sc_clutch_groups[bucket]);
		sched_clutch_bucket_t clutch_bucket = &(clutch_bucket_group->scbg_clutch_buckets[root_clutch->scr_cluster_id]);
		sched_clutch_root_t scb_root = os_atomic_load(&clutch_bucket->scb_root, relaxed);
		if (scb_root == NULL) {
			                                                                                      
			assert(clutch_bucket->scb_thr_count == 0);
			continue;
		}
		assert(scb_root == root_clutch);
		uint32_t clutch_bucket_preferred_cluster = sched_clutch_bucket_preferred_cluster(clutch_bucket);

		if (migrate_immediately) {
			  
                                                                                             
                                                                                       
              
      
			if (root_clutch->scr_cluster_id != clutch_bucket_preferred_cluster) {
				sched_edge_clutch_bucket_threads_drain(clutch_bucket, scb_root, &clutch_threads);
			} else {
				sched_clutch_bucket_mark_native(clutch_bucket, root_clutch);
			}
		} else {
			                                                                             
			boolean_t homogeneous_cluster = (pset_type_for_id(root_clutch->scr_cluster_id) == pset_type_for_id(clutch_bucket_preferred_cluster));
			  
                                                                                       
                                
      
			if (homogeneous_cluster) {
				sched_clutch_bucket_mark_native(clutch_bucket, root_clutch);
			} else {
				sched_clutch_bucket_mark_foreign(clutch_bucket, root_clutch);
			}
		}
	}

	pset_unlock(root_clutch->scr_pset);
	sched_edge_run_drained_threads(&clutch_threads);
}

  
                                                    
  
                                                                              
                                                     
   
static void
sched_edge_migrate_thread_group_running_threads(
	sched_clutch_t sched_clutch,
	sched_clutch_root_t root_clutch,
	__unused bitmap_t *clutch_bucket_modify_bitmap,
	uint32_t *tg_bucket_preferred_cluster,
	bool migrate_immediately)
{
	if (migrate_immediately == false) {
		                                                                                 
		return;
	}

	  
                               
   
                                                                                   
                                                                          
    
	uint64_t ast_processor_map = 0;
	sched_ipi_type_t ipi_type[MAX_CPUS] = {SCHED_IPI_NONE};

	uint64_t running_map = root_clutch->scr_pset->cpu_state_map[PROCESSOR_RUNNING];
	  
                                                                                         
                                                                  
    
	for (int cpuid = lsb_first(running_map); cpuid >= 0; cpuid = lsb_next(running_map, cpuid)) {
		processor_t src_processor = processor_array[cpuid];
		boolean_t expected_tg = (src_processor->current_thread_group == sched_clutch->sc_tg);
		sched_bucket_t processor_sched_bucket = src_processor->processor_set->cpu_running_buckets[cpuid];
		boolean_t non_preferred_cluster = tg_bucket_preferred_cluster[processor_sched_bucket] != root_clutch->scr_cluster_id;

		if (expected_tg && non_preferred_cluster) {
			ipi_type[cpuid] = sched_ipi_action(src_processor, NULL, false, SCHED_IPI_EVENT_REBALANCE);
			if (ipi_type[cpuid] != SCHED_IPI_NONE) {
				bit_set(ast_processor_map, cpuid);
			} else if (src_processor == current_processor()) {
				ast_on(AST_PREEMPT);
				bit_set(root_clutch->scr_pset->pending_AST_PREEMPT_cpu_mask, cpuid);
			}
		}
	}

	                          
	if (bit_first(ast_processor_map) != -1) {
		for (int cpuid = lsb_first(ast_processor_map); cpuid >= 0; cpuid = lsb_next(ast_processor_map, cpuid)) {
			processor_t ast_processor = processor_array[cpuid];
			sched_ipi_perform(ast_processor, ipi_type[cpuid]);
		}
		KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_AMP_RECOMMENDATION_CHANGE) | DBG_FUNC_NONE, thread_group_get_id(sched_clutch->sc_tg), ast_processor_map, 0, 0);
	}
}

  
                                           
  
                                                                                                             
                                                                                                          
                                                           
                                                                                                                
                                        
   
void
sched_edge_tg_preferred_cluster_change(struct thread_group *tg, uint32_t *tg_bucket_preferred_cluster, sched_perfcontrol_preferred_cluster_options_t options)
{
	sched_clutch_t clutch = sched_clutch_for_thread_group(tg);
	  
                                                                                         
                                                
    
	bitmap_t clutch_bucket_modify_bitmap[BITMAP_LEN(TH_BUCKET_SCHED_MAX)] = {0};
	for (sched_bucket_t bucket = TH_BUCKET_FIXPRI; bucket < TH_BUCKET_SCHED_MAX; bucket++) {
		uint32_t old_preferred_cluster = sched_edge_clutch_bucket_group_preferred_cluster(&clutch->sc_clutch_groups[bucket]);
		uint32_t new_preferred_cluster = tg_bucket_preferred_cluster[bucket];
		if (old_preferred_cluster != new_preferred_cluster) {
			bitmap_set(clutch_bucket_modify_bitmap, bucket);
		}
	}
	if (bitmap_lsb_first(clutch_bucket_modify_bitmap, TH_BUCKET_SCHED_MAX) == -1) {
		                                                          
		return;
	}

	for (uint32_t cluster_id = 0; cluster_id < MAX_PSETS; cluster_id++) {
		processor_set_t pset = pset_array[cluster_id];
		spl_t s = splsched();
		pset_lock(pset);
		  
                                                                                          
                                                                                          
                   
     
		sched_edge_update_preferred_cluster(clutch, clutch_bucket_modify_bitmap, tg_bucket_preferred_cluster);
		  
                                                                                                          
                                                                                    
    
                                        
     
		                                                                                              
		sched_edge_migrate_thread_group_running_threads(clutch, &pset->pset_clutch_root, clutch_bucket_modify_bitmap,
		    tg_bucket_preferred_cluster, (options & SCHED_PERFCONTROL_PREFERRED_CLUSTER_MIGRATE_RUNNING));
		                                                                                                           
		sched_edge_migrate_thread_group_runnable_threads(clutch, &pset->pset_clutch_root, clutch_bucket_modify_bitmap,
		    tg_bucket_preferred_cluster, (options & SCHED_PERFCONTROL_PREFERRED_CLUSTER_MIGRATE_RUNNABLE));
		                                                                                   
		splx(s);
	}
}

  
                                     
  
                                                                               
                                                                                 
                                                                 
  
                                                           
   
static void
sched_edge_pset_made_schedulable(__unused processor_t processor, processor_set_t dst_pset, boolean_t drop_lock)
{
	if (bitmap_test(sched_edge_available_pset_bitmask, dst_pset->pset_cluster_id)) {
		                                                                 
		if (drop_lock) {
			pset_unlock(dst_pset);
		}
		return;
	}

	bitmap_set(sched_edge_available_pset_bitmask, dst_pset->pset_cluster_id);

	thread_t thread = sched_edge_processor_idle(dst_pset);
	if (thread != THREAD_NULL) {
		thread_lock(thread);
		thread_setrun(thread, SCHED_TAILQ);
		thread_unlock(thread);
	}

	if (!drop_lock) {
		pset_lock(dst_pset);
	}
}

extern int sched_amp_spill_deferred_ipi;
extern int sched_amp_pcores_preempt_immediate_ipi;

int sched_edge_migrate_ipi_immediate = 1;

sched_ipi_type_t
sched_edge_ipi_policy(processor_t dst, thread_t thread, boolean_t dst_idle, sched_ipi_event_t event)
{
	processor_set_t pset = dst->processor_set;
	assert(bit_test(pset->pending_AST_URGENT_cpu_mask, dst->cpu_id) == false);
	assert(dst != current_processor());

	boolean_t deferred_ipi_supported = false;
#if defined(CONFIG_SCHED_DEFERRED_AST)
	deferred_ipi_supported = true;
#endif                                

	switch (event) {
	case SCHED_IPI_EVENT_SPILL:
		                                                                            
		if (deferred_ipi_supported && sched_amp_spill_deferred_ipi) {
			return sched_ipi_deferred_policy(pset, dst, event);
		}
		break;
	case SCHED_IPI_EVENT_PREEMPT:
		                                                             
                                                            
                                                  
     
		if (thread && thread->sched_pri < BASEPRI_RTQUEUES) {
			if (sched_edge_migrate_ipi_immediate) {
				  
                                                                                 
                                                                                  
                                                                                      
                                                                
      
                                          
       
				processor_set_t preferred_pset = pset_array[sched_edge_thread_preferred_cluster(thread)];
				if ((preferred_pset->pset_execution_time[thread->th_sched_bucket].pset_avg_thread_execution_time * NSEC_PER_USEC) >= ml_cpu_signal_deferred_get_timer()) {
					return dst_idle ? SCHED_IPI_IDLE : SCHED_IPI_IMMEDIATE;
				}
			}
			if (sched_amp_pcores_preempt_immediate_ipi && (pset_type_for_id(pset->pset_cluster_id) == CLUSTER_TYPE_P)) {
				return dst_idle ? SCHED_IPI_IDLE : SCHED_IPI_IMMEDIATE;
			}
		}
		break;
	default:
		break;
	}
	                                                               
	return sched_ipi_policy(dst, thread, dst_idle, event);
}

  
                                   
   
uint32_t
sched_edge_qos_max_parallelism(int qos, uint64_t options)
{
	uint32_t ecount = 0;
	uint32_t pcount = 0;

	for (int cluster_id = 0; cluster_id < MAX_PSETS; cluster_id++) {
		processor_set_t pset = pset_array[cluster_id];
		if (pset_type_for_id(cluster_id) == CLUSTER_TYPE_P) {
			pcount += pset->cpu_set_count;
		} else {
			ecount += pset->cpu_set_count;
		}
	}

	if (options & QOS_PARALLELISM_REALTIME) {
		                                                  
                                                    
                                           
     
		return pcount;
	}

	  
                                                                          
                                                                      
                       
   
                                                                        
                                                                          
                                                                       
    
	switch (qos) {
	case THREAD_QOS_BACKGROUND:
	case THREAD_QOS_MAINTENANCE:
		return ecount;
	default:
		return ecount + pcount;
	}
}



#endif                        

#endif                          
