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
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
  
   

  
                                                            
   

#ifndef _KERN_PROCESSOR_H_
#define _KERN_PROCESSOR_H_

#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <kern/kern_types.h>

#include <sys/cdefs.h>

#ifdef  MACH_KERNEL_PRIVATE

#include <mach/mach_types.h>
#include <kern/ast.h>
#include <kern/cpu_number.h>
#include <kern/smp.h>
#include <kern/simple_lock.h>
#include <kern/locks.h>
#include <kern/percpu.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/sched_urgency.h>
#include <kern/timer.h>
#include <mach/sfi_class.h>
#include <kern/sched_clutch.h>
#include <kern/timer_call.h>
#include <kern/assert.h>
#include <machine/limits.h>

  
                                                             
                                  
  
                                          
                                         
                                          
                                                               
                                                       
                                                      
  
                                                              
                                                                
                                                                
                                                                 
                                                                   
                                                                   
                                                    
  
                                                                         
                                                                             
  
                                                                         
                                                                    
                                                                             
   
#if defined(CONFIG_SCHED_DEFERRED_AST)
  
                                          
                                         
                                          
                                                               
                                                       
                                                      
  
                                                                 
                                                                          
                                                                     
                                                                       
            
  
                                                                            
                                                                         
                                                                        
   
#endif

typedef enum {
	PROCESSOR_OFF_LINE      = 0,                       
	PROCESSOR_SHUTDOWN      = 1,                        
	PROCESSOR_START         = 2,                       
	PROCESSOR_UNUSED        = 3,                                         
	PROCESSOR_IDLE          = 4,                          
	PROCESSOR_DISPATCHING   = 5,                                      
	PROCESSOR_RUNNING       = 6,                          
	PROCESSOR_STATE_LEN     = (PROCESSOR_RUNNING + 1)
} processor_state_t;

typedef enum {
	PSET_SMP,
#if __AMP__
	PSET_AMP_E,
	PSET_AMP_P,
#endif
} pset_cluster_type_t;

#if __AMP__

typedef enum {
	SCHED_PERFCTL_POLICY_DEFAULT,                                            
	SCHED_PERFCTL_POLICY_FOLLOW_GROUP,                                                                                  
	SCHED_PERFCTL_POLICY_RESTRICT_E,                                                                   
} sched_perfctl_class_policy_t;

extern _Atomic sched_perfctl_class_policy_t sched_perfctl_policy_util;
extern _Atomic sched_perfctl_class_policy_t sched_perfctl_policy_bg;

#endif              

typedef bitmap_t cpumap_t;

#if __arm64__

  
                        
  
                                                                 
                                                                        
                                                                   
                                                          
   
typedef union {
	struct {
		uint64_t        pset_avg_thread_execution_time;
		uint64_t        pset_execution_time_last_update;
	};
	unsigned __int128       pset_execution_time_packed;
} pset_execution_time_t;

#endif                

struct processor_set {
	int                     pset_id;
	int                     online_processor_count;
	int                     cpu_set_low, cpu_set_hi;
	int                     cpu_set_count;
	int                     last_chosen;

	uint64_t                load_average;
	uint64_t                pset_load_average[TH_BUCKET_SCHED_MAX];
	uint64_t                pset_load_last_update;
	cpumap_t                cpu_bitmask;
	cpumap_t                recommended_bitmask;
	cpumap_t                cpu_state_map[PROCESSOR_STATE_LEN];
	cpumap_t                primary_map;
	cpumap_t                realtime_map;
	cpumap_t                cpu_running_foreign;
	sched_bucket_t          cpu_running_buckets[MAX_CPUS];

#define SCHED_PSET_TLOCK (1)
#if     defined(SCHED_PSET_TLOCK)
                                                      
	__attribute__((aligned(128))) lck_ticket_t      sched_lock;
#else                      
	__attribute__((aligned(128))) lck_spin_t        sched_lock;                         
#endif                      

#if defined(CONFIG_SCHED_TRADITIONAL) || defined(CONFIG_SCHED_MULTIQ)
	struct run_queue        pset_runq;                                       
#endif
	struct rt_queue         rt_runq;                                                  
#if CONFIG_SCHED_CLUTCH
	struct sched_clutch_root pset_clutch_root;                            
#endif                          

#if defined(CONFIG_SCHED_TRADITIONAL)
	int                     pset_runq_bound_count;
	                                                         
#endif

	                                                                                   
	cpumap_t                pending_AST_URGENT_cpu_mask;
	cpumap_t                pending_AST_PREEMPT_cpu_mask;
#if defined(CONFIG_SCHED_DEFERRED_AST)
	  
                                                                                  
                                                                                 
                       
   
                                                                                
                                                                                
                                                                                  
                                                                              
         
    
	cpumap_t                pending_deferred_AST_cpu_mask;
#endif
	cpumap_t                pending_spill_cpu_mask;

	struct ipc_port *       pset_self;                                       
	struct ipc_port *       pset_name_self;                           

	processor_set_t         pset_list;                                             
	pset_node_t             node;
	uint32_t                pset_cluster_id;

	  
                                                                              
                                                                                
    
	pset_cluster_type_t     pset_cluster_type;
	cluster_type_t          pset_type;

#if CONFIG_SCHED_EDGE
	bitmap_t                foreign_psets[BITMAP_LEN(MAX_PSETS)];
	sched_clutch_edge       sched_edges[MAX_PSETS];
	pset_execution_time_t   pset_execution_time[TH_BUCKET_SCHED_MAX];
#endif                        
	bool                    is_SMT;                                                   
};

extern struct processor_set     pset0;

typedef bitmap_t pset_map_t;

struct pset_node {
	processor_set_t         psets;                                                

	pset_node_t             nodes;                                                   
	pset_node_t             node_list;                                             

	pset_node_t             parent;

	pset_map_t              pset_map;                                            
	_Atomic pset_map_t      pset_idle_map;                                                
	_Atomic pset_map_t      pset_idle_primary_map;                                                
	_Atomic pset_map_t      pset_non_rt_map;                                                                                 
	_Atomic pset_map_t      pset_non_rt_primary_map;                                                                                 
};

extern struct pset_node pset_node0;

extern queue_head_t tasks, threads, corpse_tasks;
extern int tasks_count, terminated_tasks_count, threads_count, terminated_threads_count;
decl_lck_mtx_data(extern, tasks_threads_lock);
decl_lck_mtx_data(extern, tasks_corpse_lock);

  
                                                                              
   
extern queue_head_t terminated_tasks;

extern queue_head_t terminated_threads;

struct processor {
	processor_state_t       state;                                 
	bool                    is_SMT;
	bool                    is_recommended;
	bool                    current_is_NO_SMT;                                                    
	bool                    current_is_bound;                                                      
	bool                    current_is_eagerpreempt;                                             
	struct thread          *active_thread;                                           
	struct thread          *idle_thread;                                               
	struct thread          *startup_thread;

	processor_set_t         processor_set;                    

	  
                                                                   
                             
    
	int                     current_pri;                                            
	sfi_class_id_t          current_sfi_class;                                       
	perfcontrol_class_t     current_perfctl_class;                                            
	  
                                                        
    
	pset_cluster_type_t     current_recommended_pset_type;
	thread_urgency_t        current_urgency;                                              

#if CONFIG_SCHED_TRADITIONAL
	int                     runq_bound_count;                                                 
#endif                               

#if CONFIG_THREAD_GROUPS
	struct thread_group    *current_thread_group;                                       
#endif
	int                     starting_pri;                                                                    
	int                     cpu_id;                                          

	uint64_t                quantum_end;                                                
	uint64_t                last_dispatch;                                     

#if KPERF
	uint64_t                kperf_last_sample_time;                                
#endif            

	uint64_t                deadline;                                             
	bool                    first_timeslice;                                                          

	bool                    processor_offlined;                                                                 
	bool                    must_idle;                                                                                                

	bool                    running_timers_active;                                              
	struct timer_call       running_timers[RUNNING_TIMER_MAX];

#if CONFIG_SCHED_TRADITIONAL || CONFIG_SCHED_MULTIQ
	struct run_queue        runq;                                                
#endif                                                      

#if CONFIG_SCHED_GRRR
	struct grrr_run_queue   grrr_runq;                                                
#endif                        

	  
                                                                   
                                                  
    
	processor_t             processor_primary;
	processor_t             processor_secondary;
	struct ipc_port        *processor_self;                                  

	processor_t             processor_list;                                      

	                                
	timer_data_t            idle_state;
	timer_data_t            system_state;
	timer_data_t            user_state;

	timer_t                 current_state;                                                                       

	                             
	timer_t                 thread_timer;                                                                
	timer_t                 kernel_timer;                                                        

	uint64_t                timer_call_ttd;                                                  
};

extern processor_t processor_list;
decl_simple_lock_data(extern, processor_list_lock);

  
                                                                           
                                              
   
#define MAX_SCHED_CPUS          64
extern processor_t              processor_array[MAX_SCHED_CPUS];                             
extern processor_set_t          pset_array[MAX_PSETS];                                         

extern uint32_t                 processor_avail_count;
extern uint32_t                 processor_avail_count_user;
extern uint32_t                 primary_processor_avail_count;
extern uint32_t                 primary_processor_avail_count_user;

#define master_processor PERCPU_GET_MASTER(processor)
PERCPU_DECL(struct processor, processor);

extern processor_t      current_processor(void);

                                                                                     

extern lck_grp_t pset_lck_grp;

#if defined(SCHED_PSET_TLOCK)
#define pset_lock_init(p)               lck_ticket_init(&(p)->sched_lock, &pset_lck_grp)
#define pset_lock(p)                    lck_ticket_lock(&(p)->sched_lock, &pset_lck_grp)
#define pset_unlock(p)                  lck_ticket_unlock(&(p)->sched_lock)
#define pset_assert_locked(p)           lck_ticket_assert_owned(&(p)->sched_lock)
#else                      
#define pset_lock_init(p)               lck_spin_init(&(p)->sched_lock, &pset_lck_grp, NULL)
#define pset_lock(p)                    lck_spin_lock_grp(&(p)->sched_lock, &pset_lck_grp)
#define pset_unlock(p)                  lck_spin_unlock(&(p)->sched_lock)
#define pset_assert_locked(p)           LCK_SPIN_ASSERT(&(p)->sched_lock, LCK_ASSERT_OWNED)
#endif                      

extern void             processor_bootstrap(void);

extern void             processor_init(
	processor_t             processor,
	int                     cpu_id,
	processor_set_t         processor_set);

extern void             processor_set_primary(
	processor_t             processor,
	processor_t             primary);

extern kern_return_t    processor_shutdown(
	processor_t             processor);

extern kern_return_t    processor_start_from_user(
	processor_t             processor);
extern kern_return_t    processor_exit_from_user(
	processor_t             processor);

extern kern_return_t    sched_processor_enable(
	processor_t             processor,
	boolean_t               enable);

extern void             processor_queue_shutdown(
	processor_t             processor);

extern void             processor_queue_shutdown(
	processor_t             processor);

extern processor_set_t  processor_pset(
	processor_t             processor);

extern pset_node_t      pset_node_root(void);

extern processor_set_t  pset_create(
	pset_node_t             node);

extern void             pset_init(
	processor_set_t         pset,
	pset_node_t             node);

extern processor_set_t  pset_find(
	uint32_t                cluster_id,
	processor_set_t         default_pset);

#if !defined(RC_HIDE_XNU_FIRESTORM) && (MAX_CPU_CLUSTERS > 2)

  
                                                                
                                                       
                                                          
   

extern processor_set_t  pset_find_first_by_cluster_type(
	pset_cluster_type_t     pset_cluster_type);

#endif                                                                

extern kern_return_t    processor_info_count(
	processor_flavor_t      flavor,
	mach_msg_type_number_t  *count);

#define pset_deallocate(x)
#define pset_reference(x)

extern void             machine_run_count(
	uint32_t                count);

extern processor_t      machine_choose_processor(
	processor_set_t         pset,
	processor_t             processor);

#define next_pset(p)    (((p)->pset_list != PROCESSOR_SET_NULL)? (p)->pset_list: (p)->node->psets)

#define PSET_THING_TASK         0
#define PSET_THING_THREAD       1

extern pset_cluster_type_t recommended_pset_type(
	thread_t                thread);
#if CONFIG_THREAD_GROUPS
extern pset_cluster_type_t thread_group_pset_recommendation(
	struct thread_group     *tg,
	cluster_type_t          recommendation);
#endif                           

inline static bool
pset_is_recommended(processor_set_t pset)
{
	return (pset->recommended_bitmask & pset->cpu_bitmask) != 0;
}

extern void             processor_state_update_idle(
	processor_t             processor);

extern void             processor_state_update_from_thread(
	processor_t             processor,
	thread_t                thread);

extern void             processor_state_update_explicit(
	processor_t             processor,
	int                     pri,
	sfi_class_id_t          sfi_class,
	pset_cluster_type_t     pset_type,
	perfcontrol_class_t     perfctl_class,
	thread_urgency_t        urgency,
	sched_bucket_t          bucket);

#define PSET_LOAD_NUMERATOR_SHIFT   16
#define PSET_LOAD_FRACTIONAL_SHIFT   4

#if CONFIG_SCHED_EDGE

extern cluster_type_t pset_type_for_id(uint32_t cluster_id);

  
                                                                              
                                                                                  
                          
  
                                      
  
                                                                                       
                                                                                     
                                                                                       
                                                                              
                           
   
#define SCHED_PSET_LOAD_EWMA_FRACTION_BITS 8
#define SCHED_PSET_LOAD_EWMA_ROUND_BIT     (1 << (SCHED_PSET_LOAD_EWMA_FRACTION_BITS - 1))
#define SCHED_PSET_LOAD_EWMA_FRACTION_MASK ((1 << SCHED_PSET_LOAD_EWMA_FRACTION_BITS) - 1)

inline static int
sched_get_pset_load_average(processor_set_t pset, sched_bucket_t sched_bucket)
{
	return (int)(((pset->pset_load_average[sched_bucket] + SCHED_PSET_LOAD_EWMA_ROUND_BIT) >> SCHED_PSET_LOAD_EWMA_FRACTION_BITS) *
	       pset->pset_execution_time[sched_bucket].pset_avg_thread_execution_time);
}

#else                        
inline static int
sched_get_pset_load_average(processor_set_t pset, __unused sched_bucket_t sched_bucket)
{
	return (int)pset->load_average >> (PSET_LOAD_NUMERATOR_SHIFT - PSET_LOAD_FRACTIONAL_SHIFT);
}
#endif                        

extern void sched_update_pset_load_average(processor_set_t pset, uint64_t curtime);
extern void sched_update_pset_avg_execution_time(processor_set_t pset, uint64_t delta, uint64_t curtime, sched_bucket_t sched_bucket);

inline static void
pset_update_processor_state(processor_set_t pset, processor_t processor, uint new_state)
{
	pset_assert_locked(pset);

	uint old_state = processor->state;
	uint cpuid = (uint)processor->cpu_id;

	assert(processor->processor_set == pset);
	assert(bit_test(pset->cpu_bitmask, cpuid));

	assert(old_state < PROCESSOR_STATE_LEN);
	assert(new_state < PROCESSOR_STATE_LEN);

	processor->state = new_state;

	bit_clear(pset->cpu_state_map[old_state], cpuid);
	bit_set(pset->cpu_state_map[new_state], cpuid);

	if ((old_state == PROCESSOR_RUNNING) || (new_state == PROCESSOR_RUNNING)) {
		sched_update_pset_load_average(pset, 0);
		if (new_state == PROCESSOR_RUNNING) {
			assert(processor == current_processor());
		}
	}
	if ((old_state == PROCESSOR_IDLE) || (new_state == PROCESSOR_IDLE)) {
		if (new_state == PROCESSOR_IDLE) {
			bit_clear(pset->realtime_map, cpuid);
		}

		pset_node_t node = pset->node;

		if (bit_count(node->pset_map) == 1) {
			                                                                
			return;
		}

		if (new_state == PROCESSOR_IDLE) {
			if (processor->processor_primary == processor) {
				if (!bit_test(atomic_load(&node->pset_non_rt_primary_map), pset->pset_id)) {
					atomic_bit_set(&node->pset_non_rt_primary_map, pset->pset_id, memory_order_relaxed);
				}
				if (!bit_test(atomic_load(&node->pset_idle_primary_map), pset->pset_id)) {
					atomic_bit_set(&node->pset_idle_primary_map, pset->pset_id, memory_order_relaxed);
				}
			}
			if (!bit_test(atomic_load(&node->pset_non_rt_map), pset->pset_id)) {
				atomic_bit_set(&node->pset_non_rt_map, pset->pset_id, memory_order_relaxed);
			}
			if (!bit_test(atomic_load(&node->pset_idle_map), pset->pset_id)) {
				atomic_bit_set(&node->pset_idle_map, pset->pset_id, memory_order_relaxed);
			}
		} else {
			cpumap_t idle_map = pset->cpu_state_map[PROCESSOR_IDLE];
			if (idle_map == 0) {
				                       
				if (bit_test(atomic_load(&node->pset_idle_map), pset->pset_id)) {
					atomic_bit_clear(&node->pset_idle_map, pset->pset_id, memory_order_relaxed);
				}
			}
			if (processor->processor_primary == processor) {
				idle_map &= pset->primary_map;
				if (idle_map == 0) {
					                               
					if (bit_test(atomic_load(&node->pset_idle_primary_map), pset->pset_id)) {
						atomic_bit_clear(&node->pset_idle_primary_map, pset->pset_id, memory_order_relaxed);
					}
				}
			}
		}
	}
}

#else                            

__BEGIN_DECLS

extern void             pset_deallocate(
	processor_set_t         pset);

extern void             pset_reference(
	processor_set_t         pset);

__END_DECLS

#endif                           

#ifdef KERNEL_PRIVATE
__BEGIN_DECLS
extern unsigned int             processor_count;
extern processor_t      cpu_to_processor(int cpu);

extern kern_return_t    enable_smt_processors(bool enable);

__END_DECLS

#endif                     

#endif                          
