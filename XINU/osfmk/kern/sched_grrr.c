/*
 * Copyright (c) 2009-2016 Apple Inc. All rights reserved.
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

#include <mach/mach_types.h>
#include <mach/machine.h>
#include <mach/policy.h>
#include <mach/sync_policy.h>
#include <mach/thread_act.h>

#include <machine/machine_routines.h>
#include <machine/sched_param.h>
#include <machine/machine_cpu.h>

#include <kern/kern_types.h>
#include <kern/clock.h>
#include <kern/counters.h>
#include <kern/cpu_number.h>
#include <kern/cpu_data.h>
#include <kern/debug.h>
#include <kern/macro_help.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/processor.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/syscall_subr.h>
#include <kern/task.h>
#include <kern/thread.h>

#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>

#include <mach/sdt.h>

#include <sys/kdebug.h>

#if defined(CONFIG_SCHED_GRRR_CORE)

static void
grrr_priority_mapping_init(void);

static boolean_t
grrr_enqueue(
	grrr_run_queue_t                     rq,
	thread_t                     thread);

static thread_t
grrr_select(
	grrr_run_queue_t        rq);

static void
grrr_remove(
	grrr_run_queue_t                      rq,
	thread_t              thread);


static void
grrr_sorted_list_insert_group(grrr_run_queue_t rq,
    grrr_group_t group);

static void
grrr_rescale_work(grrr_run_queue_t rq);

static void
grrr_runqueue_init(grrr_run_queue_t             runq);

                                                                   
static grrr_proportional_priority_t grrr_priority_mapping[NRQS];

                                                 
static grrr_group_index_t grrr_group_mapping[NUM_GRRR_PROPORTIONAL_PRIORITIES];

uint32_t                        grrr_rescale_tick;

#endif                                      

#if defined(CONFIG_SCHED_GRRR)

static void
sched_grrr_init(void);

static void
sched_grrr_timebase_init(void);

static void
sched_grrr_processor_init(processor_t processor);

static void
sched_grrr_pset_init(processor_set_t pset);

static void
sched_grrr_maintenance_continuation(void);

static thread_t
sched_grrr_choose_thread(processor_t            processor,
    int                             priority,
    ast_t                           reason);

static thread_t
sched_grrr_steal_thread(processor_set_t         pset);

static int
sched_grrr_compute_priority(thread_t thread);

static processor_t
sched_grrr_choose_processor(    processor_set_t         pset,
    processor_t                     processor,
    thread_t                        thread);

static boolean_t
sched_grrr_processor_enqueue(
	processor_t                    processor,
	thread_t                       thread,
	sched_options_t                options);

static void
sched_grrr_processor_queue_shutdown(
	processor_t                    processor);

static boolean_t
sched_grrr_processor_queue_remove(
	processor_t                 processor,
	thread_t                thread);

static boolean_t
sched_grrr_processor_queue_empty(processor_t            processor);

static boolean_t
sched_grrr_processor_queue_has_priority(processor_t             processor,
    int                            priority,
    boolean_t              gte);

static boolean_t
sched_grrr_priority_is_urgent(int priority);

static ast_t
sched_grrr_processor_csw_check(processor_t processor);

static uint32_t
sched_grrr_initial_quantum_size(thread_t thread);

static sched_mode_t
sched_grrr_initial_thread_sched_mode(task_t parent_task);

static boolean_t
sched_grrr_can_update_priority(thread_t thread);

static void
sched_grrr_update_priority(thread_t     thread);

static void
sched_grrr_lightweight_update_priority(thread_t thread);

static int
sched_grrr_processor_runq_count(processor_t     processor);

static uint64_t
sched_grrr_processor_runq_stats_count_sum(processor_t   processor);

static int
sched_grrr_processor_bound_count(processor_t    processor);

static void
sched_grrr_thread_update_scan(sched_update_scan_context_t scan_context);

const struct sched_dispatch_table sched_grrr_dispatch = {
	.sched_name                                     = "grrr",
	.init                                           = sched_grrr_init,
	.timebase_init                                  = sched_grrr_timebase_init,
	.processor_init                                 = sched_grrr_processor_init,
	.pset_init                                      = sched_grrr_pset_init,
	.maintenance_continuation                       = sched_grrr_maintenance_continuation,
	.choose_thread                                  = sched_grrr_choose_thread,
	.steal_thread_enabled                           = sched_steal_thread_DISABLED,
	.steal_thread                                   = sched_grrr_steal_thread,
	.compute_timeshare_priority                     = sched_grrr_compute_priority,
	.choose_node                                    = sched_choose_node,
	.choose_processor                               = sched_grrr_choose_processor,
	.processor_enqueue                              = sched_grrr_processor_enqueue,
	.processor_queue_shutdown                       = sched_grrr_processor_queue_shutdown,
	.processor_queue_remove                         = sched_grrr_processor_queue_remove,
	.processor_queue_empty                          = sched_grrr_processor_queue_empty,
	.priority_is_urgent                             = sched_grrr_priority_is_urgent,
	.processor_csw_check                            = sched_grrr_processor_csw_check,
	.processor_queue_has_priority                   = sched_grrr_processor_queue_has_priority,
	.initial_quantum_size                           = sched_grrr_initial_quantum_size,
	.initial_thread_sched_mode                      = sched_grrr_initial_thread_sched_mode,
	.can_update_priority                            = sched_grrr_can_update_priority,
	.update_priority                                = sched_grrr_update_priority,
	.lightweight_update_priority                    = sched_grrr_lightweight_update_priority,
	.quantum_expire                                 = sched_default_quantum_expire,
	.processor_runq_count                           = sched_grrr_processor_runq_count,
	.processor_runq_stats_count_sum                 = sched_grrr_processor_runq_stats_count_sum,
	.processor_bound_count                          = sched_grrr_processor_bound_count,
	.thread_update_scan                             = sched_grrr_thread_update_scan,
	.multiple_psets_enabled                         = TRUE,
	.sched_groups_enabled                           = FALSE,
	.avoid_processor_enabled                        = FALSE,
	.thread_avoid_processor                         = NULL,
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
	.run_count_incr                                 = sched_run_incr,
	.run_count_decr                                 = sched_run_decr,
	.update_thread_bucket                           = sched_update_thread_bucket,
	.pset_made_schedulable                          = sched_pset_made_schedulable,
};

extern int      max_unsafe_quanta;

static uint32_t grrr_quantum_us;
static uint32_t grrr_quantum;

static uint64_t                 sched_grrr_tick_deadline;

static void
sched_grrr_init(void)
{
	if (default_preemption_rate < 1) {
		default_preemption_rate = 100;
	}
	grrr_quantum_us = (1000 * 1000) / default_preemption_rate;

	printf("standard grrr timeslicing quantum is %d us\n", grrr_quantum_us);

	grrr_priority_mapping_init();
}

static void
sched_grrr_timebase_init(void)
{
	uint64_t        abstime;

	                                  
	clock_interval_to_absolutetime_interval(
		grrr_quantum_us, NSEC_PER_USEC, &abstime);
	assert((abstime >> 32) == 0 && (uint32_t)abstime != 0);
	grrr_quantum = (uint32_t)abstime;

	thread_depress_time = 1 * grrr_quantum;
	default_timeshare_computation = grrr_quantum / 2;
	default_timeshare_constraint = grrr_quantum;

	max_unsafe_computation = max_unsafe_quanta * grrr_quantum;
	sched_safe_duration = 2 * max_unsafe_quanta * grrr_quantum;
}

static void
sched_grrr_processor_init(processor_t processor)
{
	grrr_runqueue_init(&processor->grrr_runq);
}

static void
sched_grrr_pset_init(processor_set_t pset __unused)
{
}

static void
sched_grrr_maintenance_continuation(void)
{
	uint64_t                        abstime = mach_absolute_time();

	grrr_rescale_tick++;

	  
                              
    
	compute_averages(1);

	if (sched_grrr_tick_deadline == 0) {
		sched_grrr_tick_deadline = abstime;
	}

	clock_deadline_for_periodic_event(10 * sched_one_second_interval, abstime,
	    &sched_grrr_tick_deadline);

	assert_wait_deadline((event_t)sched_grrr_maintenance_continuation, THREAD_UNINT, sched_grrr_tick_deadline);
	thread_block((thread_continue_t)sched_grrr_maintenance_continuation);
	              
}

static thread_t
sched_grrr_choose_thread(processor_t            processor,
    int                           priority __unused,
    ast_t                         reason __unused)
{
	grrr_run_queue_t                rq = &processor->grrr_runq;

	return grrr_select(rq);
}

static thread_t
sched_grrr_steal_thread(processor_set_t         pset)
{
	pset_unlock(pset);

	return THREAD_NULL;
}

static int
sched_grrr_compute_priority(thread_t thread)
{
	return thread->base_pri;
}

static processor_t
sched_grrr_choose_processor(    processor_set_t         pset,
    processor_t                    processor,
    thread_t                       thread)
{
	return choose_processor(pset, processor, thread);
}

static boolean_t
sched_grrr_processor_enqueue(
	processor_t                    processor,
	thread_t                       thread,
	sched_options_t                options __unused)
{
	grrr_run_queue_t                rq = &processor->grrr_runq;
	boolean_t                               result;

	result = grrr_enqueue(rq, thread);

	thread->runq = processor;

	return result;
}

static void
sched_grrr_processor_queue_shutdown(
	processor_t                    processor)
{
	processor_set_t         pset = processor->processor_set;
	thread_t                        thread;
	queue_head_t            tqueue, bqueue;

	queue_init(&tqueue);
	queue_init(&bqueue);

	while ((thread = sched_grrr_choose_thread(processor, IDLEPRI, AST_NONE)) != THREAD_NULL) {
		if (thread->bound_processor == PROCESSOR_NULL) {
			enqueue_tail(&tqueue, (queue_entry_t)thread);
		} else {
			enqueue_tail(&bqueue, (queue_entry_t)thread);
		}
	}

	while ((thread = (thread_t)(void *)dequeue_head(&bqueue)) != THREAD_NULL) {
		sched_grrr_processor_enqueue(processor, thread, SCHED_TAILQ);
	}

	pset_unlock(pset);

	while ((thread = (thread_t)(void *)dequeue_head(&tqueue)) != THREAD_NULL) {
		thread_lock(thread);

		thread_setrun(thread, SCHED_TAILQ);

		thread_unlock(thread);
	}
}

static boolean_t
sched_grrr_processor_queue_remove(
	processor_t                     processor,
	thread_t                thread)
{
	processor_set_t pset = processor->processor_set;

	pset_lock(pset);

	if (processor == thread->runq) {
		  
                                                   
                    
     
		grrr_run_queue_t                rq = &processor->grrr_runq;

		grrr_remove(rq, thread);
	} else {
		  
                                                  
                        
     
		assert(thread->runq == PROCESSOR_NULL);
		processor = PROCESSOR_NULL;
	}

	pset_unlock(pset);

	return processor != PROCESSOR_NULL;
}

static boolean_t
sched_grrr_processor_queue_empty(processor_t            processor __unused)
{
	boolean_t result;

	result = (processor->grrr_runq.count == 0);

	return result;
}

static boolean_t
sched_grrr_processor_queue_has_priority(processor_t             processor,
    int                            priority,
    boolean_t              gte __unused)
{
	grrr_run_queue_t                rq = &processor->grrr_runq;
	unsigned int    i;

	i = grrr_group_mapping[grrr_priority_mapping[priority]];
	for (; i < NUM_GRRR_GROUPS; i++) {
		if (rq->groups[i].count > 0) {
			return TRUE;
		}
	}

	return FALSE;
}

                                         
static boolean_t
sched_grrr_priority_is_urgent(int priority)
{
	if (priority <= BASEPRI_FOREGROUND) {
		return FALSE;
	}

	if (priority < MINPRI_KERNEL) {
		return TRUE;
	}

	if (priority >= BASEPRI_PREEMPT) {
		return TRUE;
	}

	return FALSE;
}

static ast_t
sched_grrr_processor_csw_check(processor_t processor)
{
	int                             count;

	count = sched_grrr_processor_runq_count(processor);

	if (count > 0) {
		return AST_PREEMPT;
	}

	return AST_NONE;
}

static uint32_t
sched_grrr_initial_quantum_size(thread_t thread __unused)
{
	return grrr_quantum;
}

static sched_mode_t
sched_grrr_initial_thread_sched_mode(task_t parent_task)
{
	if (parent_task == kernel_task) {
		return TH_MODE_FIXED;
	} else {
		return TH_MODE_TIMESHARE;
	}
}

static boolean_t
sched_grrr_can_update_priority(thread_t thread __unused)
{
	return FALSE;
}

static void
sched_grrr_update_priority(thread_t     thread __unused)
{
	return;
}

static void
sched_grrr_lightweight_update_priority(thread_t thread __unused)
{
	return;
}

static int
sched_grrr_processor_runq_count(processor_t     processor)
{
	return processor->grrr_runq.count;
}

static uint64_t
sched_grrr_processor_runq_stats_count_sum(processor_t   processor)
{
	return processor->grrr_runq.runq_stats.count_sum;
}

static int
sched_grrr_processor_bound_count(__unused processor_t   processor)
{
	return 0;
}

static void
sched_grrr_thread_update_scan(__unused sched_update_scan_context_t scan_context)
{
	return;
}

#endif                                 

#if defined(CONFIG_SCHED_GRRR_CORE)

static void
grrr_priority_mapping_init(void)
{
	unsigned int i;

	                           
	for (i = 0; i <= 10; i++) {
		grrr_priority_mapping[i] = (grrr_proportional_priority_t)(2 * i);
	}

	                                                
	for (i = 11; i <= 51; i++) {
		grrr_priority_mapping[i] = (grrr_proportional_priority_t)(3 * i);
	}

	                                                
	for (i = 52; i <= 127; i++) {
		grrr_priority_mapping[i] = (grrr_proportional_priority_t)(128 + i);
	}

	for (i = 0; i < NUM_GRRR_PROPORTIONAL_PRIORITIES; i++) {
#if 0
		unsigned j, k;
		                       
		for (j = 0, k = 1; k <= i; j++, k *= 2) {
			;
		}
#endif

		                 
		grrr_group_mapping[i] = (grrr_group_index_t)(i >> 2);
	}
}

static thread_t
grrr_intragroup_schedule(grrr_group_t group)
{
	thread_t thread;

	if (group->count == 0) {
		return THREAD_NULL;
	}

	thread = group->current_client;
	if (thread == THREAD_NULL) {
		thread = (thread_t)(void *)queue_first(&group->clients);
	}

	if (1              ) {
		group->current_client = (thread_t)(void *)queue_next((queue_entry_t)thread);
		if (queue_end(&group->clients, (queue_entry_t)group->current_client)) {
			group->current_client = (thread_t)(void *)queue_first(&group->clients);
		}

		thread = group->current_client;
	}

	return thread;
}

static thread_t
grrr_intergroup_schedule(grrr_run_queue_t rq)
{
	thread_t thread;
	grrr_group_t group;

	if (rq->count == 0) {
		return THREAD_NULL;
	}

	group = rq->current_group;

	if (group == GRRR_GROUP_NULL) {
		group = (grrr_group_t)queue_first(&rq->sorted_group_list);
	}

	thread = grrr_intragroup_schedule(group);

	if ((group->work >= (UINT32_MAX - 256)) || (rq->last_rescale_tick != grrr_rescale_tick)) {
		grrr_rescale_work(rq);
	}
	group->work++;

	if (queue_end(&rq->sorted_group_list, queue_next((queue_entry_t)group))) {
		                                      
		group = (grrr_group_t)queue_first(&rq->sorted_group_list);
	} else {
		grrr_group_t nextgroup = (grrr_group_t)queue_next((queue_entry_t)group);
		uint64_t orderleft, orderright;

		  
                                                             
    
                                                                                  
    
                                                                
    
     
		orderleft = (group->work + 1) * ((uint64_t)nextgroup->weight);
		orderright = (nextgroup->work + 1) * ((uint64_t)group->weight);
		if (orderleft > orderright) {
			group = nextgroup;
		} else {
			group = (grrr_group_t)queue_first(&rq->sorted_group_list);
		}
	}

	rq->current_group = group;

	return thread;
}

static void
grrr_runqueue_init(grrr_run_queue_t             runq)
{
	grrr_group_index_t index;

	runq->count = 0;

	for (index = 0; index < NUM_GRRR_GROUPS; index++) {
		unsigned int prisearch;

		for (prisearch = 0;
		    prisearch < NUM_GRRR_PROPORTIONAL_PRIORITIES;
		    prisearch++) {
			if (grrr_group_mapping[prisearch] == index) {
				runq->groups[index].minpriority = (grrr_proportional_priority_t)prisearch;
				break;
			}
		}

		runq->groups[index].index = index;

		queue_init(&runq->groups[index].clients);
		runq->groups[index].count = 0;
		runq->groups[index].weight = 0;
		runq->groups[index].work = 0;
		runq->groups[index].current_client = THREAD_NULL;
	}

	queue_init(&runq->sorted_group_list);
	runq->weight = 0;
	runq->current_group = GRRR_GROUP_NULL;
}

static void
grrr_rescale_work(grrr_run_queue_t rq)
{
	grrr_group_index_t index;

	                                        
	for (index = 0; index < NUM_GRRR_GROUPS; index++) {
		rq->groups[index].work >>= 3;
	}

	rq->last_rescale_tick = grrr_rescale_tick;
}

static boolean_t
grrr_enqueue(
	grrr_run_queue_t                       rq,
	thread_t                       thread)
{
	grrr_proportional_priority_t    gpriority;
	grrr_group_index_t              gindex;
	grrr_group_t                    group;

	gpriority = grrr_priority_mapping[thread->sched_pri];
	gindex = grrr_group_mapping[gpriority];
	group = &rq->groups[gindex];

	if (group->count == 0) {
		                                           
		enqueue_tail(&group->clients, (queue_entry_t)thread);
		group->count = 1;
		group->weight = gpriority;
		group->current_client = thread;
	} else {
		                                      
		if (group->current_client == THREAD_NULL ||
		    queue_first(&group->clients) == (queue_entry_t)group->current_client) {
			enqueue_head(&group->clients, (queue_entry_t)thread);
		} else {
			insque((queue_entry_t)thread, queue_prev((queue_entry_t)group->current_client));
		}
		SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
		group->count++;
		group->weight += gpriority;

		                                                                                        
		remqueue((queue_entry_t)group);
	}

	grrr_sorted_list_insert_group(rq, group);

	rq->count++;
	rq->weight += gpriority;

	return FALSE;
}

static thread_t
grrr_select(grrr_run_queue_t    rq)
{
	thread_t                thread;

	thread = grrr_intergroup_schedule(rq);
	if (thread != THREAD_NULL) {
		grrr_proportional_priority_t    gpriority;
		grrr_group_index_t              gindex;
		grrr_group_t                    group;

		gpriority = grrr_priority_mapping[thread->sched_pri];
		gindex = grrr_group_mapping[gpriority];
		group = &rq->groups[gindex];

		remqueue((queue_entry_t)thread);
		SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
		group->count--;
		group->weight -= gpriority;
		if (group->current_client == thread) {
			group->current_client = THREAD_NULL;
		}

		remqueue((queue_entry_t)group);
		if (group->count == 0) {
			if (rq->current_group == group) {
				rq->current_group = GRRR_GROUP_NULL;
			}
		} else {
			                                          
			grrr_sorted_list_insert_group(rq, group);
		}

		rq->count--;
		rq->weight -= gpriority;

		thread->runq = PROCESSOR_NULL;
	}

	return thread;
}

static void
grrr_remove(
	grrr_run_queue_t                       rq,
	thread_t               thread)
{
	grrr_proportional_priority_t    gpriority;
	grrr_group_index_t              gindex;
	grrr_group_t                    group;

	gpriority = grrr_priority_mapping[thread->sched_pri];
	gindex = grrr_group_mapping[gpriority];
	group = &rq->groups[gindex];

	remqueue((queue_entry_t)thread);
	SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
	group->count--;
	group->weight -= gpriority;
	if (group->current_client == thread) {
		group->current_client = THREAD_NULL;
	}

	remqueue((queue_entry_t)group);
	if (group->count == 0) {
		if (rq->current_group == group) {
			rq->current_group = GRRR_GROUP_NULL;
		}
	} else {
		                                          
		grrr_sorted_list_insert_group(rq, group);
	}

	rq->count--;
	rq->weight -= gpriority;

	thread->runq = PROCESSOR_NULL;
}

static void
grrr_sorted_list_insert_group(grrr_run_queue_t rq,
    grrr_group_t group)
{
	                           
	if (queue_empty(&rq->sorted_group_list)) {
		enqueue_tail(&rq->sorted_group_list, (queue_entry_t)group);
	} else {
		grrr_group_t search_group;

		                                                                
                                                     
     
		search_group = (grrr_group_t)queue_first(&rq->sorted_group_list);
		while (!queue_end(&rq->sorted_group_list, (queue_entry_t)search_group)) {
			if (search_group->weight < group->weight) {
				                              
				search_group = (grrr_group_t)queue_prev((queue_entry_t)search_group);
				break;
			}
			if (search_group->weight == group->weight) {
				                                      
				if (search_group->index < group->index) {
					search_group = (grrr_group_t)queue_prev((queue_entry_t)search_group);
					break;
				}
			}

			                                                    
			search_group = (grrr_group_t)queue_next((queue_entry_t)search_group);
		}

		if (queue_end(&rq->sorted_group_list, (queue_entry_t)search_group)) {
			enqueue_tail(&rq->sorted_group_list, (queue_entry_t)group);
		} else {
			insque((queue_entry_t)group, (queue_entry_t)search_group);
		}
	}
}

#endif                                      
