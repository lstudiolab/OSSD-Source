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
  
   
  
                
                               
             
  
                             
  
   

#ifndef _KERN_SCHED_H_
#define _KERN_SCHED_H_

#include <mach/policy.h>
#include <kern/kern_types.h>
#include <kern/smp.h>
#include <kern/circle_queue.h>
#include <kern/macro_help.h>
#include <kern/timer_call.h>
#include <kern/ast.h>
#include <kern/bits.h>

#define NRQS_MAX        (128)                                                                  

#define MAXPRI          (NRQS_MAX-1)
#define MINPRI          0                                                                      
#define IDLEPRI         MINPRI                                                    
#define NOPRI           -1

  
                                  
  
                                                                          
                            
       
       
                
       
       
                           
                       
       
       
                
       
       
                       
                           
       
       
                
       
       
                           
                          
       
       
                
       
       
                          
                                            
       
       
                
       
       
                                            
                                         
                                           
       
       
                
       
       
                                           
                                      
       
       
                
       
       
                                            
                                                                          
   

#define BASEPRI_RTQUEUES        (BASEPRI_REALTIME + 1)                                  
#define BASEPRI_REALTIME        (MAXPRI - (NRQS_MAX / 4) + 1)                           

#define MAXPRI_KERNEL           (BASEPRI_REALTIME - 1)                                  
#define BASEPRI_PREEMPT_HIGH    (BASEPRI_PREEMPT + 1)                                   
#define BASEPRI_PREEMPT         (MAXPRI_KERNEL - 3)                                     
#define BASEPRI_VM              (BASEPRI_PREEMPT - 1)                                   

#define BASEPRI_KERNEL          (MINPRI_KERNEL + 1)                                     
#define MINPRI_KERNEL           (MAXPRI_KERNEL - (NRQS_MAX / 8) + 1)                    

#define MAXPRI_RESERVED         (MINPRI_KERNEL - 1)                                     
#define BASEPRI_GRAPHICS        (MAXPRI_RESERVED - 3)                                   
#define MINPRI_RESERVED         (MAXPRI_RESERVED - (NRQS_MAX / 8) + 1)                  

#define MAXPRI_USER             (MINPRI_RESERVED - 1)                                   
#define BASEPRI_CONTROL         (BASEPRI_DEFAULT + 17)                                  
#define BASEPRI_FOREGROUND      (BASEPRI_DEFAULT + 16)                                  
#define BASEPRI_BACKGROUND      (BASEPRI_DEFAULT + 15)                                  
#define BASEPRI_USER_INITIATED  (BASEPRI_DEFAULT +  6)                                  
#define BASEPRI_DEFAULT         (MAXPRI_USER - (NRQS_MAX / 4))                          
#define MAXPRI_SUPPRESSED       (BASEPRI_DEFAULT - 3)                                   
#define BASEPRI_UTILITY         (BASEPRI_DEFAULT - 11)                                  
#define MAXPRI_THROTTLE         (MINPRI + 4)                                            
#define MINPRI_USER             MINPRI                                                  

#define DEPRESSPRI              (MINPRI)                                      

#define MAXPRI_PROMOTE          (MAXPRI_KERNEL)                                          
#define MINPRI_RWLOCK           (BASEPRI_BACKGROUND)                                         
#define MINPRI_EXEC             (BASEPRI_DEFAULT)                                     
#define MINPRI_WAITQ            (BASEPRI_DEFAULT)                                               

#define NRQS                    (BASEPRI_REALTIME)                                         

                                                                                                
_Static_assert((NRQS == (MAXPRI_PROMOTE + 1)), "Runqueues are too small to hold all non-realtime threads");

                                                     
typedef enum {
	TH_MODE_NONE = 0,                                                                                    
	TH_MODE_REALTIME,                                                                      
	TH_MODE_FIXED,                                                                              
	TH_MODE_TIMESHARE,                                                                     
} sched_mode_t;

  
                                                                         
                                                                            
                                                                             
                                                           
   

                                       
typedef enum {
	TH_BUCKET_FIXPRI = 0,                                       
	TH_BUCKET_SHARE_FG,                                                                 
#if CONFIG_SCHED_CLUTCH
	TH_BUCKET_SHARE_IN,                                                                                              
#endif                          
	TH_BUCKET_SHARE_DF,                                                                                       
	TH_BUCKET_SHARE_UT,                                                                                       
	TH_BUCKET_SHARE_BG,                                                                              
	TH_BUCKET_RUN,                                                    
	TH_BUCKET_SCHED_MAX = TH_BUCKET_RUN,                                     
	TH_BUCKET_MAX,
} sched_bucket_t;

  
                                         
   
#define invalid_pri(pri) ((pri) < MINPRI || (pri) > MAXPRI)

struct runq_stats {
	uint64_t                count_sum;
	uint64_t                last_change_timestamp;
};

#if defined(CONFIG_SCHED_TIMESHARE_CORE) || defined(CONFIG_SCHED_PROTO)

struct run_queue {
	int                     highq;                                                      
	bitmap_t                bitmap[BITMAP_LEN(NRQS)];                                   
	int                     count;                                                  
	int                     urgency;                                                         
	circle_queue_head_t     queues[NRQS];                                      

	struct runq_stats       runq_stats;
};

inline static void
rq_bitmap_set(bitmap_t *map, u_int n)
{
	assert(n < NRQS);
	bitmap_set(map, n);
}

inline static void
rq_bitmap_clear(bitmap_t *map, u_int n)
{
	assert(n < NRQS);
	bitmap_clear(map, n);
}

#endif                                                                          

struct rt_queue {
	_Atomic int             count;                                                  
	queue_head_t            queue;                                                       
	struct runq_stats       runq_stats;
};
typedef struct rt_queue *rt_queue_t;

#if defined(CONFIG_SCHED_GRRR_CORE)

  
                                                                          
                                                                   
   
typedef uint8_t grrr_proportional_priority_t;
typedef uint8_t grrr_group_index_t;

#define NUM_GRRR_PROPORTIONAL_PRIORITIES        256
#define MAX_GRRR_PROPORTIONAL_PRIORITY ((grrr_proportional_priority_t)255)

#if 0
#define NUM_GRRR_GROUPS 8                                                     
#endif

#define NUM_GRRR_GROUPS 64                                                 

struct grrr_group {
	queue_chain_t                   priority_order;                                                         
	grrr_proportional_priority_t            minpriority;
	grrr_group_index_t              index;

	queue_head_t                    clients;
	int                                             count;
	uint32_t                                weight;
#if 0
	uint32_t                                deferred_removal_weight;
#endif
	uint32_t                                work;
	thread_t                                current_client;
};

struct grrr_run_queue {
	int                                     count;
	uint32_t                        last_rescale_tick;
	struct grrr_group       groups[NUM_GRRR_GROUPS];
	queue_head_t            sorted_group_list;
	uint32_t                        weight;
	grrr_group_t            current_group;

	struct runq_stats   runq_stats;
};

#endif                                      

extern int rt_runq_count(processor_set_t);
extern void rt_runq_count_incr(processor_set_t);
extern void rt_runq_count_decr(processor_set_t);

#if defined(CONFIG_SCHED_MULTIQ)
sched_group_t   sched_group_create(void);
void            sched_group_destroy(sched_group_t sched_group);
#endif                                   



  
                      
   

                                                       
extern void             thread_quantum_expire(
	timer_call_param_t      processor,
	timer_call_param_t      thread);

                                                
extern ast_t    csw_check(
	thread_t      thread,
	processor_t   processor,
	ast_t         check_reason);

                            
extern void ast_check(processor_t processor);

extern void sched_update_generation_count(void);

#if defined(CONFIG_SCHED_TIMESHARE_CORE)
extern uint32_t std_quantum, min_std_quantum;
extern uint32_t std_quantum_us;
#endif                                  

extern uint32_t thread_depress_time;
extern uint32_t default_timeshare_computation;
extern uint32_t default_timeshare_constraint;

extern uint32_t max_rt_quantum, min_rt_quantum;

extern int default_preemption_rate;

#if defined(CONFIG_SCHED_TIMESHARE_CORE)

  
                                                                       
                                                                     
                                                       
   
#define SCHED_TICK_SHIFT        3
#define SCHED_TICK_MAX_DELTA    (8)

extern unsigned         sched_tick;
extern uint32_t         sched_tick_interval;

#endif                                  

extern uint64_t         sched_one_second_interval;

                                              
extern void            compute_sched_load(void);

extern void             compute_averages(uint64_t);

extern void             compute_averunnable(
	void                    *nrun);

extern void             compute_stack_target(
	void                    *arg);

extern void             compute_pageout_gc_throttle(
	void                    *arg);

extern void             compute_pmap_gc_throttle(
	void                    *arg);

  
                               
               
   
#if defined(CONFIG_SCHED_TIMESHARE_CORE)

#define MAX_LOAD (NRQS - 1)
#define SCHED_PRI_SHIFT_MAX ((8 * sizeof(uint32_t)) - 1)
extern uint32_t         sched_pri_shifts[TH_BUCKET_MAX];
extern uint32_t         sched_fixed_shift;
extern int8_t           sched_load_shifts[NRQS];
extern uint32_t         sched_decay_usage_age_factor;
void sched_timeshare_consider_maintenance(uint64_t ctime);
#endif                                  

void sched_consider_recommended_cores(uint64_t ctime, thread_t thread);

extern int32_t          sched_poll_yield_shift;
extern uint64_t         sched_safe_duration;

extern uint32_t         sched_load_average, sched_mach_factor;

extern uint32_t         avenrun[3], mach_factor[3];

extern uint64_t         max_unsafe_computation;
extern uint64_t         max_poll_computation;

extern uint32_t         sched_run_buckets[TH_BUCKET_MAX];

extern uint32_t sched_run_incr(thread_t thread);
extern uint32_t sched_run_decr(thread_t thread);
extern void sched_update_thread_bucket(thread_t thread);

extern uint32_t sched_smt_run_incr(thread_t thread);
extern uint32_t sched_smt_run_decr(thread_t thread);
extern void sched_smt_update_thread_bucket(thread_t thread);

#define SCHED_DECAY_TICKS       32
struct shift_data {
	int     shift1;
	int     shift2;
};

  
                                                             
   
#define thread_timer_delta(thread, delta)                                       \
MACRO_BEGIN                                                                     \
	(delta) = (typeof(delta))timer_delta(&(thread)->system_timer,           \
	    &(thread)->system_timer_save);                                      \
	(delta) += (typeof(delta))timer_delta(&(thread)->user_timer,            \
	    &(thread)->user_timer_save);                                        \
MACRO_END

extern bool system_is_SMT;

#endif                      
