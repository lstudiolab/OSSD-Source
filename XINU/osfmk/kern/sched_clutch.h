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

#ifndef _KERN_SCHED_CLUTCH_H_
#define _KERN_SCHED_CLUTCH_H_

#include <kern/sched.h>
#include <machine/atomic.h>
#include <kern/priority_queue.h>
#include <kern/thread_group.h>
#include <kern/bits.h>

#if CONFIG_SCHED_CLUTCH

  
                                                         
                                                      
                                                 
  
                                                     
                                                           
                                                           
           
   
__enum_decl(sched_clutch_tg_priority_t, uint8_t, {
	SCHED_CLUTCH_TG_PRI_LOW           = 0,
	SCHED_CLUTCH_TG_PRI_MED           = 1,
	SCHED_CLUTCH_TG_PRI_HIGH          = 2,
	SCHED_CLUTCH_TG_PRI_MAX           = 3,
});

  
                                                                
                                                                 
                                            
   
#define SCHED_CLUTCH_THREAD_ELIGIBLE(thread)    ((thread->bound_processor) == PROCESSOR_NULL)

#if CONFIG_SCHED_EDGE
#define SCHED_CLUTCH_THREAD_CLUSTER_BOUND(thread)       ((thread->sched_flags & (TH_SFLAG_ECORE_ONLY | TH_SFLAG_PCORE_ONLY)) != 0)
#define SCHED_CLUTCH_THREAD_CLUSTER_BOUND_SOFT(thread)  ((thread->sched_flags & TH_SFLAG_BOUND_SOFT) != 0)

#else                        
#define SCHED_CLUTCH_THREAD_CLUSTER_BOUND(thread)       (0)
#define SCHED_CLUTCH_THREAD_CLUSTER_BOUND_SOFT(thread)  (0)
#endif                        

  
                                    
   
struct sched_clutch_bucket_runq {
	int                     scbrq_highq;
	int                     scbrq_count;
	bitmap_t                scbrq_bitmap[BITMAP_LEN(NRQS_MAX)];
	circle_queue_head_t     scbrq_queues[NRQS_MAX];
};
typedef struct sched_clutch_bucket_runq *sched_clutch_bucket_runq_t;

  
  
                                    
  
                                                                  
                         
                                                                 
                                                            
                                                              
                                          
   

  
                                  
  
                                                                       
                                                                            
                                                                         
                                                                           
                                                                           
                                                                                         
   
struct sched_clutch_root_bucket {
	                                                      
	uint8_t                         scrb_bucket;
	                                                                    
	bool                            scrb_bound;
	                                                                      
	bool                            scrb_starvation_avoidance;

	union {
		                                                                            
		struct sched_clutch_bucket_runq scrb_clutch_buckets;
		                                                                 
		struct run_queue                scrb_bound_thread_runq;
	};
	                                                                                
	struct priority_queue_entry_deadline scrb_pqlink;
	                                         
	uint64_t                        scrb_warped_deadline;
	                                        
	uint64_t                        scrb_warp_remaining;
	                                                                    
	uint64_t                        scrb_starvation_ts;
};
typedef struct sched_clutch_root_bucket *sched_clutch_root_bucket_t;

  
                           
  
                                                                     
                                                                    
                                                                        
                                                                        
                                                                          
                                                                          
                                                          
   
struct sched_clutch_root {
	                                                                                      
	int16_t                         scr_priority;
	                                                           
	uint16_t                        scr_thr_count;
	                                                                                                    
	int16_t                         scr_urgency;

	uint32_t                        scr_cluster_id;
	                                                 
	processor_set_t                 scr_pset;
	  
                                                              
                                                                  
    
	queue_head_t                    scr_clutch_buckets;

	  
                                                                         
                                                                  
                                                           
    
	struct priority_queue_sched_max scr_foreign_buckets;

	                                  

	                                                       
	bitmap_t                        scr_unbound_runnable_bitmap[BITMAP_LEN(TH_BUCKET_SCHED_MAX)];
	                                                                                  
	bitmap_t                        scr_unbound_warp_available[BITMAP_LEN(TH_BUCKET_SCHED_MAX)];
	                                                     
	bitmap_t                        scr_bound_runnable_bitmap[BITMAP_LEN(TH_BUCKET_SCHED_MAX)];
	                                                                                
	bitmap_t                        scr_bound_warp_available[BITMAP_LEN(TH_BUCKET_SCHED_MAX)];

	                                                                                 
	struct priority_queue_deadline_min scr_unbound_root_buckets;
	                                                                      
	struct priority_queue_deadline_min scr_bound_root_buckets;

	                                                                           
	uint16_t _Atomic                scr_cumulative_run_count[TH_BUCKET_SCHED_MAX];

	                                                     
	struct sched_clutch_root_bucket scr_unbound_buckets[TH_BUCKET_SCHED_MAX];
	                                                   
	struct sched_clutch_root_bucket scr_bound_buckets[TH_BUCKET_SCHED_MAX];
};
typedef struct sched_clutch_root *sched_clutch_root_t;

                                          
struct sched_clutch;

  
                                 
  
                                                                
                                                                  
                 
  
                                                                     
                                                  
   

#if __LP64__

#define CLUTCH_CPU_DATA_MAX             (UINT64_MAX)
typedef uint64_t                        clutch_cpu_data_t;
typedef unsigned __int128               clutch_cpu_data_wide_t;

#else               

#define CLUTCH_CPU_DATA_MAX             (UINT32_MAX)
typedef uint32_t                        clutch_cpu_data_t;
typedef uint64_t                        clutch_cpu_data_wide_t;

#endif               

typedef union sched_clutch_bucket_cpu_data {
	struct {
		                                               
		clutch_cpu_data_t       scbcd_cpu_used;
		                                          
		clutch_cpu_data_t       scbcd_cpu_blocked;
	} cpu_data;
	clutch_cpu_data_wide_t          scbcd_cpu_data_packed;
} sched_clutch_bucket_cpu_data_t;

  
                             
  
                                                                   
                                                                 
                                                                   
                                                                      
                                                                      
                         
  
                                                                     
                                                                   
   
struct sched_clutch_bucket {
#if CONFIG_SCHED_EDGE
	                                                            
	bool                            scb_foreign;
#endif                        
	                                      
	uint8_t                         scb_bucket;
	                                       
	uint8_t                         scb_priority;
	                                                                          
	uint16_t                        scb_thr_count;

	                                                                      
	struct sched_clutch_bucket_group *scb_group;
	                                                                
	struct sched_clutch_root        *scb_root;
	                                                                         
	struct priority_queue_sched_max scb_clutchpri_prioq;
	                                          
	struct priority_queue_sched_stable_max scb_thread_runq;

	                                                                                   
	queue_chain_t                   scb_listlink;
	                                                           
	queue_chain_t                   scb_runqlink;
	                                                   
	queue_head_t                    scb_thread_timeshare_queue;
#if CONFIG_SCHED_EDGE
	                                                                     
	struct priority_queue_entry_sched     scb_foreignlink;
#endif                        
};
typedef struct sched_clutch_bucket *sched_clutch_bucket_t;

  
                              
  
                                                                             
                                                
   
typedef union sched_clutch_counter_time {
	struct {
		uint64_t                scct_count;
		uint64_t                scct_timestamp;
	};
#if __LP64__
	unsigned __int128               scct_packed;
#endif               
} __attribute__((aligned(16))) sched_clutch_counter_time_t;

  
                                   
  
                                                                   
                                                                       
                                                                        
                                                  
   
struct sched_clutch_bucket_group {
	                                            
	uint8_t                         scbg_bucket;
	                                                                          
	uint32_t _Atomic                scbg_timeshare_tick;
	                                                                
	uint32_t _Atomic                scbg_pri_shift;
	                                                
	uint32_t _Atomic                scbg_preferred_cluster;
	                                                          
	struct sched_clutch             *scbg_clutch;
#if !__LP64__
	                                                                                            
	lck_spin_t                      scbg_stats_lock;
#endif                
	                                                                               
	sched_clutch_counter_time_t     scbg_blocked_data;
	                                                                           
	sched_clutch_counter_time_t     scbg_pending_data;
	                                                                          
	sched_clutch_counter_time_t     scbg_interactivity_data;
	                                                           
	sched_clutch_bucket_cpu_data_t  scbg_cpu_data;

	  
                               
   
                                                               
                                                                 
                                                                        
             
   
                                       
    
	                                                                      
	struct sched_clutch_bucket      scbg_clutch_buckets[MAX_PSETS];
};
typedef struct sched_clutch_bucket_group *sched_clutch_bucket_group_t;


  
                      
  
                                                                      
                                                                           
                                           
   
struct sched_clutch {
	  
                                                                      
                                                     
    
	uint16_t _Atomic                sc_thr_count;
	  
                                                                   
                                         
    
	union {
		                                                             
		sched_clutch_tg_priority_t _Atomic sc_tg_priority;
	};
	union {
		                                 
		struct thread_group     *sc_tg;
	};
	                                                        
	struct sched_clutch_bucket_group sc_clutch_groups[TH_BUCKET_SCHED_MAX];
};
typedef struct sched_clutch *sched_clutch_t;


                                 
void sched_clutch_init_with_thread_group(sched_clutch_t, struct thread_group *);
void sched_clutch_destroy(sched_clutch_t);

                                         
void sched_clutch_thread_clutch_update(thread_t, sched_clutch_t, sched_clutch_t);
uint32_t sched_edge_thread_preferred_cluster(thread_t);

                                         
uint32_t sched_clutch_thread_run_bucket_incr(thread_t, sched_bucket_t);
uint32_t sched_clutch_thread_run_bucket_decr(thread_t, sched_bucket_t);
void sched_clutch_cpu_usage_update(thread_t, uint64_t);
uint32_t sched_clutch_thread_pri_shift(thread_t, sched_bucket_t);

                                 
uint32_t sched_clutch_root_count(sched_clutch_root_t);

                                         
extern sched_clutch_t sched_clutch_for_thread(thread_t);
extern sched_clutch_t sched_clutch_for_thread_group(struct thread_group *);

#if CONFIG_SCHED_EDGE

  
                                                                                              
   
void sched_edge_matrix_get(sched_clutch_edge *edge_matrix, bool *edge_request_bitmap, uint64_t flags, uint64_t matrix_order);
void sched_edge_matrix_set(sched_clutch_edge *edge_matrix, bool *edge_changes_bitmap, uint64_t flags, uint64_t matrix_order);
void sched_edge_tg_preferred_cluster_change(struct thread_group *tg, uint32_t *tg_bucket_preferred_cluster, sched_perfcontrol_preferred_cluster_options_t options);

uint16_t sched_edge_cluster_cumulative_count(sched_clutch_root_t root_clutch, sched_bucket_t bucket);

#if DEVELOPMENT || DEBUG
  
                                                              
  
                                      
   
kern_return_t sched_edge_sysctl_configure_e_to_p(uint64_t);
kern_return_t sched_edge_sysctl_configure_p_to_e(uint64_t);
sched_clutch_edge sched_edge_e_to_p(void);
sched_clutch_edge sched_edge_p_to_e(void);
#endif                           

#endif                        

#endif                          

#endif                            
