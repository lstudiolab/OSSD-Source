/*
 * Copyright (c) 2013-2020 Apple Inc. All rights reserved.
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
#include <kern/mach_param.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/processor.h>
#include <kern/queue.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/task.h>
#include <kern/thread.h>

#include <sys/kdebug.h>

  
                   
  
                                    
  
                                            
  
                                     
                                               
                                                                  
                                                            
  
                                                                               
  
                                                                                                         
                                                                                  
  
                                                                      
                                                                                                           
            
  
                                                                                                                
                                                                                                                 
                                                                                 
  
                                                                
  
                                
  
                                                                                                          
                                                                                                       
  
                         
  
                                                                          
                                                                                                    
                                                           
                                    
  
                         
                                                 
                                                                                                        
                                                           
                                                                                    
                                                                                           
                                                                                              
                                                                                            
                                                             
  
                                                                                               
                                                                         
                                                                                                                       
                                                              
  
                                                                                      
  
                                                                                                               
                                                                       
                                                                                                                       
  
                                                                     
                                                          
                                   
  
                                                                    
                                                                           
  
                                      
                                                                            
                                                                             
  
                                                                                
                                 
                                                                                
                                
  
                                                                                   
                                                          
  
                                      
  
                                                     
  
                                         
                                                                     
                                                    
  
          
                                                                            
                       
  
                                                                                
  
              
                                                                                       
                                                                     
                                                                                       
  
                                                                                 
                                                                                               
  
                                                                                       
                                                                                   
  
                  
                                                                     
                                                                                  
  
                                                                                            
               
  
                                                                                   
                                                                                      
                                                                              
                       
  
               
                                                                                         
                                                                                
                                                                           
  
   

#if DEBUG || DEVELOPMENT
#define MULTIQ_SANITY_CHECK
#endif

typedef struct sched_entry {
	queue_chain_t           entry_links;
	int16_t                 sched_pri;                                        
	int16_t                 runq;
	int32_t                 pad;
} *sched_entry_t;

typedef run_queue_t entry_queue_t;                                                                                   
typedef run_queue_t group_runq_t;                                                                      

#define SCHED_ENTRY_NULL        ((sched_entry_t) 0)
#define MULTIQ_ERUNQ            (-4)                                                             

                                                                               
struct sched_group {
	struct sched_entry      entries[NRQS];
	struct run_queue        runq;
	queue_chain_t           sched_groups;
};

  
                                                                   
                                                                     
                    
   
static boolean_t        deep_drain = FALSE;

                                                           
static boolean_t        multiq_sanity_check = FALSE;

  
                                                      
                                                   
                          
   
#define DEFAULT_DRAIN_BAND_LIMIT MAXPRI
static integer_t        drain_band_limit;

  
                                                                                    
   
#define DEFAULT_DRAIN_DEPTH_LIMIT MAXPRI_THROTTLE
static integer_t        drain_depth_limit;

  
                                                                                   
   
#define DEFAULT_DRAIN_CEILING BASEPRI_FOREGROUND
static integer_t        drain_ceiling;

static ZONE_DECLARE(sched_group_zone, "sched groups",
    sizeof(struct sched_group), ZC_NOENCRYPT | ZC_NOCALLOUT);

static uint64_t         num_sched_groups = 0;
static queue_head_t     sched_groups;

static LCK_GRP_DECLARE(sched_groups_lock_grp, "sched_groups");
static LCK_MTX_DECLARE(sched_groups_lock, &sched_groups_lock_grp);

static void
sched_multiq_init(void);

static thread_t
sched_multiq_steal_thread(processor_set_t pset);

static void
sched_multiq_thread_update_scan(sched_update_scan_context_t scan_context);

static boolean_t
sched_multiq_processor_enqueue(processor_t processor, thread_t thread,
    sched_options_t options);

static boolean_t
sched_multiq_processor_queue_remove(processor_t processor, thread_t thread);

void
sched_multiq_quantum_expire(thread_t thread);

static ast_t
sched_multiq_processor_csw_check(processor_t processor);

static boolean_t
sched_multiq_processor_queue_has_priority(processor_t processor, int priority, boolean_t gte);

static int
sched_multiq_runq_count(processor_t processor);

static boolean_t
sched_multiq_processor_queue_empty(processor_t processor);

static uint64_t
sched_multiq_runq_stats_count_sum(processor_t processor);

static int
sched_multiq_processor_bound_count(processor_t processor);

static void
sched_multiq_pset_init(processor_set_t pset);

static void
sched_multiq_processor_init(processor_t processor);

static thread_t
sched_multiq_choose_thread(processor_t processor, int priority, ast_t reason);

static void
sched_multiq_processor_queue_shutdown(processor_t processor);

static sched_mode_t
sched_multiq_initial_thread_sched_mode(task_t parent_task);

static bool
sched_multiq_thread_avoid_processor(processor_t processor, thread_t thread);

const struct sched_dispatch_table sched_multiq_dispatch = {
	.sched_name                                     = "multiq",
	.init                                           = sched_multiq_init,
	.timebase_init                                  = sched_timeshare_timebase_init,
	.processor_init                                 = sched_multiq_processor_init,
	.pset_init                                      = sched_multiq_pset_init,
	.maintenance_continuation                       = sched_timeshare_maintenance_continue,
	.choose_thread                                  = sched_multiq_choose_thread,
	.steal_thread_enabled                           = sched_steal_thread_DISABLED,
	.steal_thread                                   = sched_multiq_steal_thread,
	.compute_timeshare_priority                     = sched_compute_timeshare_priority,
	.choose_node                                    = sched_choose_node,
	.choose_processor                               = choose_processor,
	.processor_enqueue                              = sched_multiq_processor_enqueue,
	.processor_queue_shutdown                       = sched_multiq_processor_queue_shutdown,
	.processor_queue_remove                         = sched_multiq_processor_queue_remove,
	.processor_queue_empty                          = sched_multiq_processor_queue_empty,
	.priority_is_urgent                             = priority_is_urgent,
	.processor_csw_check                            = sched_multiq_processor_csw_check,
	.processor_queue_has_priority                   = sched_multiq_processor_queue_has_priority,
	.initial_quantum_size                           = sched_timeshare_initial_quantum_size,
	.initial_thread_sched_mode                      = sched_multiq_initial_thread_sched_mode,
	.can_update_priority                            = can_update_priority,
	.update_priority                                = update_priority,
	.lightweight_update_priority                    = lightweight_update_priority,
	.quantum_expire                                 = sched_multiq_quantum_expire,
	.processor_runq_count                           = sched_multiq_runq_count,
	.processor_runq_stats_count_sum                 = sched_multiq_runq_stats_count_sum,
	.processor_bound_count                          = sched_multiq_processor_bound_count,
	.thread_update_scan                             = sched_multiq_thread_update_scan,
	.multiple_psets_enabled                         = FALSE,
	.sched_groups_enabled                           = TRUE,
	.avoid_processor_enabled                        = TRUE,
	.thread_avoid_processor                         = sched_multiq_thread_avoid_processor,
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


static void
sched_multiq_init(void)
{
#if defined(MULTIQ_SANITY_CHECK)
	PE_parse_boot_argn("-multiq-sanity-check", &multiq_sanity_check, sizeof(multiq_sanity_check));
#endif

	PE_parse_boot_argn("-multiq-deep-drain", &deep_drain, sizeof(deep_drain));

	if (!PE_parse_boot_argn("multiq_drain_ceiling", &drain_ceiling, sizeof(drain_ceiling))) {
		drain_ceiling = DEFAULT_DRAIN_CEILING;
	}

	if (!PE_parse_boot_argn("multiq_drain_depth_limit", &drain_depth_limit, sizeof(drain_depth_limit))) {
		drain_depth_limit = DEFAULT_DRAIN_DEPTH_LIMIT;
	}

	if (!PE_parse_boot_argn("multiq_drain_band_limit", &drain_band_limit, sizeof(drain_band_limit))) {
		drain_band_limit = DEFAULT_DRAIN_BAND_LIMIT;
	}

	printf("multiq scheduler config: deep-drain %d, ceiling %d, depth limit %d, band limit %d, sanity check %d\n",
	    deep_drain, drain_ceiling, drain_depth_limit, drain_band_limit, multiq_sanity_check);

	queue_init(&sched_groups);

	sched_timeshare_init();
}

static void
sched_multiq_processor_init(processor_t processor)
{
	run_queue_init(&processor->runq);
}

static void
sched_multiq_pset_init(processor_set_t pset)
{
	run_queue_init(&pset->pset_runq);
}

static sched_mode_t
sched_multiq_initial_thread_sched_mode(task_t parent_task)
{
	if (parent_task == kernel_task) {
		return TH_MODE_FIXED;
	} else {
		return TH_MODE_TIMESHARE;
	}
}

sched_group_t
sched_group_create(void)
{
	sched_group_t       sched_group;

	if (!SCHED(sched_groups_enabled)) {
		return SCHED_GROUP_NULL;
	}

	sched_group = (sched_group_t)zalloc(sched_group_zone);

	bzero(sched_group, sizeof(struct sched_group));

	run_queue_init(&sched_group->runq);

	for (size_t i = 0; i < NRQS; i++) {
		sched_group->entries[i].runq = 0;
		sched_group->entries[i].sched_pri = (int16_t)i;
	}

	lck_mtx_lock(&sched_groups_lock);
	queue_enter(&sched_groups, sched_group, sched_group_t, sched_groups);
	num_sched_groups++;
	lck_mtx_unlock(&sched_groups_lock);

	return sched_group;
}

void
sched_group_destroy(sched_group_t sched_group)
{
	if (!SCHED(sched_groups_enabled)) {
		assert(sched_group == SCHED_GROUP_NULL);
		return;
	}

	assert(sched_group != SCHED_GROUP_NULL);
	assert(sched_group->runq.count == 0);

	for (int i = 0; i < NRQS; i++) {
		assert(sched_group->entries[i].runq == 0);
		assert(sched_group->entries[i].sched_pri == i);
	}

	lck_mtx_lock(&sched_groups_lock);
	queue_remove(&sched_groups, sched_group, sched_group_t, sched_groups);
	num_sched_groups--;
	lck_mtx_unlock(&sched_groups_lock);

	zfree(sched_group_zone, sched_group);
}

__attribute__((always_inline))
static inline entry_queue_t
multiq_main_entryq(processor_t processor)
{
	return (entry_queue_t)&processor->processor_set->pset_runq;
}

__attribute__((always_inline))
static inline run_queue_t
multiq_bound_runq(processor_t processor)
{
	return &processor->runq;
}

__attribute__((always_inline))
static inline sched_entry_t
group_entry_for_pri(sched_group_t group, integer_t pri)
{
	return &group->entries[pri];
}

__attribute__((always_inline))
static inline sched_group_t
group_for_entry(sched_entry_t entry)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-align"
	sched_group_t group = (sched_group_t)(entry - entry->sched_pri);
#pragma clang diagnostic pop
	return group;
}

                                      
static sched_entry_t
entry_queue_first_entry(entry_queue_t rq)
{
	assert(rq->count != 0);

	circle_queue_t queue = &rq->queues[rq->highq];

	sched_entry_t entry = cqe_queue_first(queue, struct sched_entry, entry_links);

	assert(entry->sched_pri == rq->highq);

	return entry;
}

#if defined(MULTIQ_SANITY_CHECK)

#if MACH_ASSERT
__attribute__((always_inline))
static inline boolean_t
queue_chain_linked(queue_chain_t* chain)
{
	if (chain->next != NULL) {
		assert(chain->prev != NULL);
		return TRUE;
	} else {
		assert(chain->prev == NULL);
		return FALSE;
	}
}
#endif                  

static thread_t
group_first_thread(sched_group_t group)
{
	group_runq_t rq = &group->runq;

	assert(rq->count != 0);

	circle_queue_t queue = &rq->queues[rq->highq];

	thread_t thread = cqe_queue_first(queue, struct thread, runq_links);

	assert(thread != THREAD_NULL);
	assert_thread_magic(thread);

	assert(thread->sched_group == group);

	                           
	assert(thread->sched_pri == rq->highq);

	return thread;
}

                                                  
static void
entry_queue_check_entry(entry_queue_t runq, sched_entry_t entry, int expected_pri)
{
	circle_queue_t q;
	sched_entry_t elem;

	assert(queue_chain_linked(&entry->entry_links));
	assert(entry->runq == MULTIQ_ERUNQ);

	q = &runq->queues[expected_pri];

	cqe_foreach_element(elem, q, entry_links) {
		if (elem == entry) {
			return;
		}
	}

	panic("runq %p doesn't contain entry %p at pri %d", runq, entry, expected_pri);
}

                                                       
static void
sched_group_check_thread(sched_group_t group, thread_t thread)
{
	circle_queue_t q;
	thread_t elem;
	int pri = thread->sched_pri;

	assert(thread->runq != PROCESSOR_NULL);

	q = &group->runq.queues[pri];

	cqe_foreach_element(elem, q, runq_links) {
		if (elem == thread) {
			return;
		}
	}

	panic("group %p doesn't contain thread %p at pri %d", group, thread, pri);
}

static void
global_check_entry_queue(entry_queue_t main_entryq)
{
	if (main_entryq->count == 0) {
		return;
	}

	sched_entry_t entry = entry_queue_first_entry(main_entryq);

	assert(entry->runq == MULTIQ_ERUNQ);

	sched_group_t group = group_for_entry(entry);

	thread_t thread = group_first_thread(group);

	__assert_only sched_entry_t thread_entry = group_entry_for_pri(thread->sched_group, thread->sched_pri);

	assert(entry->sched_pri == group->runq.highq);

	assert(entry == thread_entry);
	assert(thread->runq != PROCESSOR_NULL);
}

static void
group_check_run_queue(entry_queue_t main_entryq, sched_group_t group)
{
	if (group->runq.count == 0) {
		return;
	}

	thread_t thread = group_first_thread(group);

	assert(thread->runq != PROCESSOR_NULL);

	sched_entry_t sched_entry = group_entry_for_pri(thread->sched_group, thread->sched_pri);

	entry_queue_check_entry(main_entryq, sched_entry, thread->sched_pri);

	assert(sched_entry->sched_pri == thread->sched_pri);
	assert(sched_entry->runq == MULTIQ_ERUNQ);
}

#endif                                   

  
                                   
   
static sched_entry_t
entry_queue_dequeue_entry(entry_queue_t rq)
{
	sched_entry_t   sched_entry;
	circle_queue_t  queue = &rq->queues[rq->highq];

	assert(rq->count > 0);
	assert(!circle_queue_empty(queue));

	sched_entry = cqe_dequeue_head(queue, struct sched_entry, entry_links);

	SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
	rq->count--;
	if (SCHED(priority_is_urgent)(rq->highq)) {
		rq->urgency--; assert(rq->urgency >= 0);
	}
	if (circle_queue_empty(queue)) {
		rq_bitmap_clear(rq->bitmap, rq->highq);
		rq->highq = bitmap_first(rq->bitmap, NRQS);
	}

	sched_entry->runq = 0;

	return sched_entry;
}

  
                                   
   
static boolean_t
entry_queue_enqueue_entry(
	entry_queue_t rq,
	sched_entry_t entry,
	integer_t     options)
{
	int             sched_pri = entry->sched_pri;
	circle_queue_t  queue = &rq->queues[sched_pri];
	boolean_t       result = FALSE;

	assert(entry->runq == 0);

	if (circle_queue_empty(queue)) {
		circle_enqueue_tail(queue, &entry->entry_links);

		rq_bitmap_set(rq->bitmap, sched_pri);
		if (sched_pri > rq->highq) {
			rq->highq = sched_pri;
			result = TRUE;
		}
	} else {
		if (options & SCHED_TAILQ) {
			circle_enqueue_tail(queue, &entry->entry_links);
		} else {
			circle_enqueue_head(queue, &entry->entry_links);
		}
	}
	if (SCHED(priority_is_urgent)(sched_pri)) {
		rq->urgency++;
	}
	SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
	rq->count++;

	entry->runq = MULTIQ_ERUNQ;

	return result;
}

  
                                      
   
static void
entry_queue_remove_entry(
	entry_queue_t  rq,
	sched_entry_t  entry)
{
	int sched_pri = entry->sched_pri;

#if defined(MULTIQ_SANITY_CHECK)
	if (multiq_sanity_check) {
		entry_queue_check_entry(rq, entry, sched_pri);
	}
#endif

	remqueue(&entry->entry_links);

	SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
	rq->count--;
	if (SCHED(priority_is_urgent)(sched_pri)) {
		rq->urgency--; assert(rq->urgency >= 0);
	}

	if (circle_queue_empty(&rq->queues[sched_pri])) {
		                             
		rq_bitmap_clear(rq->bitmap, sched_pri);
		rq->highq = bitmap_first(rq->bitmap, NRQS);
	}

	entry->runq = 0;
}

static void
entry_queue_change_entry(
	entry_queue_t rq,
	sched_entry_t entry,
	integer_t     options)
{
	int            sched_pri   = entry->sched_pri;
	circle_queue_t queue       = &rq->queues[sched_pri];

#if defined(MULTIQ_SANITY_CHECK)
	if (multiq_sanity_check) {
		entry_queue_check_entry(rq, entry, sched_pri);
	}
#endif

	circle_dequeue(queue, &entry->entry_links);
	if (options & SCHED_TAILQ) {
		circle_enqueue_tail(queue, &entry->entry_links);
	} else {
		circle_enqueue_head(queue, &entry->entry_links);
	}
}
  
                                   
  
                                                               
   
static thread_t
group_run_queue_dequeue_thread(
	group_runq_t   rq,
	integer_t     *thread_pri,
	boolean_t     *queue_empty)
{
	thread_t        thread;
	circle_queue_t  queue = &rq->queues[rq->highq];

	assert(rq->count > 0);
	assert(!circle_queue_empty(queue));

	*thread_pri = rq->highq;

	thread = cqe_dequeue_head(queue, struct thread, runq_links);
	assert_thread_magic(thread);

	SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
	rq->count--;
	if (SCHED(priority_is_urgent)(rq->highq)) {
		rq->urgency--; assert(rq->urgency >= 0);
	}
	if (circle_queue_empty(queue)) {
		rq_bitmap_clear(rq->bitmap, rq->highq);
		rq->highq = bitmap_first(rq->bitmap, NRQS);
		*queue_empty = TRUE;
	} else {
		*queue_empty = FALSE;
	}

	return thread;
}

  
                                   
                                                
   
static boolean_t
group_run_queue_enqueue_thread(
	group_runq_t   rq,
	thread_t       thread,
	integer_t      thread_pri,
	integer_t      options)
{
	circle_queue_t  queue = &rq->queues[thread_pri];
	boolean_t       result = FALSE;

	assert(thread->runq == PROCESSOR_NULL);
	assert_thread_magic(thread);

	if (circle_queue_empty(queue)) {
		circle_enqueue_tail(queue, &thread->runq_links);

		rq_bitmap_set(rq->bitmap, thread_pri);
		if (thread_pri > rq->highq) {
			rq->highq = thread_pri;
		}
		result = TRUE;
	} else {
		if (options & SCHED_TAILQ) {
			circle_enqueue_tail(queue, &thread->runq_links);
		} else {
			circle_enqueue_head(queue, &thread->runq_links);
		}
	}
	if (SCHED(priority_is_urgent)(thread_pri)) {
		rq->urgency++;
	}
	SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
	rq->count++;

	return result;
}

  
                                       
                                                   
   
static boolean_t
group_run_queue_remove_thread(
	group_runq_t    rq,
	thread_t        thread,
	integer_t       thread_pri)
{
	circle_queue_t  queue = &rq->queues[thread_pri];
	boolean_t       result = FALSE;

	assert_thread_magic(thread);
	assert(thread->runq != PROCESSOR_NULL);

	circle_dequeue(queue, &thread->runq_links);

	SCHED_STATS_RUNQ_CHANGE(&rq->runq_stats, rq->count);
	rq->count--;
	if (SCHED(priority_is_urgent)(thread_pri)) {
		rq->urgency--; assert(rq->urgency >= 0);
	}

	if (circle_queue_empty(queue)) {
		                             
		rq_bitmap_clear(rq->bitmap, thread_pri);
		rq->highq = bitmap_first(rq->bitmap, NRQS);
		result = TRUE;
	}

	thread->runq = PROCESSOR_NULL;

	return result;
}

  
                                                            
                                                              
                                                        
   
static thread_t
sched_global_dequeue_thread(entry_queue_t main_entryq)
{
	boolean_t pri_level_empty = FALSE;
	sched_entry_t entry;
	group_runq_t group_runq;
	thread_t thread;
	integer_t thread_pri;
	sched_group_t group;

	assert(main_entryq->count > 0);

	entry = entry_queue_dequeue_entry(main_entryq);

	group = group_for_entry(entry);
	group_runq = &group->runq;

	thread = group_run_queue_dequeue_thread(group_runq, &thread_pri, &pri_level_empty);

	thread->runq = PROCESSOR_NULL;

	if (!pri_level_empty) {
		entry_queue_enqueue_entry(main_entryq, entry, SCHED_TAILQ);
	}

	return thread;
}

                                                                    
static thread_t
sched_global_deep_drain_dequeue_thread(entry_queue_t main_entryq)
{
	boolean_t pri_level_empty = FALSE;
	sched_entry_t entry;
	group_runq_t group_runq;
	thread_t thread;
	integer_t thread_pri;
	sched_group_t group;

	assert(main_entryq->count > 0);

	entry = entry_queue_first_entry(main_entryq);

	group = group_for_entry(entry);
	group_runq = &group->runq;

	thread = group_run_queue_dequeue_thread(group_runq, &thread_pri, &pri_level_empty);

	thread->runq = PROCESSOR_NULL;

	if (pri_level_empty) {
		entry_queue_remove_entry(main_entryq, entry);
	}

	return thread;
}


static thread_t
sched_group_dequeue_thread(
	entry_queue_t main_entryq,
	sched_group_t group)
{
	group_runq_t group_runq = &group->runq;
	boolean_t pri_level_empty = FALSE;
	thread_t thread;
	integer_t thread_pri;

	thread = group_run_queue_dequeue_thread(group_runq, &thread_pri, &pri_level_empty);

	thread->runq = PROCESSOR_NULL;

	if (pri_level_empty) {
		entry_queue_remove_entry(main_entryq, group_entry_for_pri(group, thread_pri));
	}

	return thread;
}

static void
sched_group_remove_thread(
	entry_queue_t main_entryq,
	sched_group_t group,
	thread_t thread)
{
	integer_t thread_pri = thread->sched_pri;
	sched_entry_t sched_entry = group_entry_for_pri(group, thread_pri);

#if defined(MULTIQ_SANITY_CHECK)
	if (multiq_sanity_check) {
		global_check_entry_queue(main_entryq);
		group_check_run_queue(main_entryq, group);

		sched_group_check_thread(group, thread);
		entry_queue_check_entry(main_entryq, sched_entry, thread_pri);
	}
#endif

	boolean_t pri_level_empty = group_run_queue_remove_thread(&group->runq, thread, thread_pri);

	if (pri_level_empty) {
		entry_queue_remove_entry(main_entryq, sched_entry);
	}

#if defined(MULTIQ_SANITY_CHECK)
	if (multiq_sanity_check) {
		global_check_entry_queue(main_entryq);
		group_check_run_queue(main_entryq, group);
	}
#endif
}

static void
sched_group_enqueue_thread(
	entry_queue_t        main_entryq,
	sched_group_t        group,
	thread_t             thread,
	integer_t            options)
{
#if defined(MULTIQ_SANITY_CHECK)
	if (multiq_sanity_check) {
		global_check_entry_queue(main_entryq);
		group_check_run_queue(main_entryq, group);
	}
#endif

	int sched_pri = thread->sched_pri;

	boolean_t pri_level_was_empty = group_run_queue_enqueue_thread(&group->runq, thread, sched_pri, options);

	if (pri_level_was_empty) {
		  
                                                                           
                                
     
		entry_queue_enqueue_entry(main_entryq, &group->entries[sched_pri], options);
	} else if (options & SCHED_HEADQ) {
		                                                                                
		entry_queue_change_entry(main_entryq, &group->entries[sched_pri], options);
	}
}

  
                                                                
                                                        
  
                                         
  
                                                         
  
                                                                               
                                    
  
                            
   
static thread_t
sched_multiq_choose_thread(
	processor_t      processor,
	int              priority,
	ast_t            reason)
{
	entry_queue_t   main_entryq = multiq_main_entryq(processor);
	run_queue_t     bound_runq  = multiq_bound_runq(processor);

	boolean_t choose_bound_runq = FALSE;

	if (bound_runq->highq < priority &&
	    main_entryq->highq < priority) {
		return THREAD_NULL;
	}

	if (bound_runq->count && main_entryq->count) {
		if (bound_runq->highq >= main_entryq->highq) {
			choose_bound_runq = TRUE;
		} else {
			                   
		}
	} else if (bound_runq->count) {
		choose_bound_runq = TRUE;
	} else if (main_entryq->count) {
		                   
	} else {
		return THREAD_NULL;
	}

	if (choose_bound_runq) {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    MACHDBG_CODE(DBG_MACH_SCHED, MACH_MULTIQ_DEQUEUE) | DBG_FUNC_NONE,
		    MACH_MULTIQ_BOUND, main_entryq->highq, bound_runq->highq, 0, 0);

		return run_queue_dequeue(bound_runq, SCHED_HEADQ);
	}

	sched_group_t group = current_thread()->sched_group;

#if defined(MULTIQ_SANITY_CHECK)
	if (multiq_sanity_check) {
		global_check_entry_queue(main_entryq);
		group_check_run_queue(main_entryq, group);
	}
#endif

	  
                                                                
   
         
                                                                           
                                          
    
	if (group->runq.count != 0 && (reason & AST_PREEMPTION) == 0) {
		boolean_t favor_group = TRUE;

		integer_t global_pri = main_entryq->highq;
		integer_t group_pri  = group->runq.highq;

		  
                                                                        
    
                                                                 
                                                                  
     
		if (global_pri > group_pri) {
			  
                                                           
                                          
      
			if (global_pri > drain_depth_limit && group_pri <= drain_depth_limit) {
				favor_group = FALSE;
			}

			  
                                                   
                            
      
			if (global_pri >= drain_ceiling) {
				favor_group = FALSE;
			}

			  
                                                         
      
			if ((global_pri - group_pri) >= drain_band_limit) {
				favor_group = FALSE;
			}
		}

		if (favor_group) {
			                          
			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    MACHDBG_CODE(DBG_MACH_SCHED, MACH_MULTIQ_DEQUEUE) | DBG_FUNC_NONE,
			    MACH_MULTIQ_GROUP, global_pri, group_pri, 0, 0);

			return sched_group_dequeue_thread(main_entryq, group);
		}
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_SCHED, MACH_MULTIQ_DEQUEUE) | DBG_FUNC_NONE,
	    MACH_MULTIQ_GLOBAL, main_entryq->highq, group->runq.highq, 0, 0);

	                                                                  
	if (deep_drain) {
		return sched_global_deep_drain_dequeue_thread(main_entryq);
	} else {
		return sched_global_dequeue_thread(main_entryq);
	}
}


  
                                                            
                  
   
static boolean_t
sched_multiq_processor_enqueue(
	processor_t      processor,
	thread_t         thread,
	sched_options_t  options)
{
	boolean_t       result;

	assert(processor == thread->chosen_processor);

	if (thread->bound_processor != PROCESSOR_NULL) {
		assert(thread->bound_processor == processor);

		result = run_queue_enqueue(multiq_bound_runq(processor), thread, options);
		thread->runq = processor;

		return result;
	}

	sched_group_enqueue_thread(multiq_main_entryq(processor),
	    thread->sched_group,
	    thread, options);

	thread->runq = processor;

	return FALSE;
}

  
                                                                 
                                                                      
                   
   
void
sched_multiq_quantum_expire(thread_t thread)
{
	if (deep_drain) {
		  
                                                             
                                              
     

		processor_t processor = thread->last_processor;
		processor_set_t pset = processor->processor_set;
		entry_queue_t entryq = multiq_main_entryq(processor);

		pset_lock(pset);

		sched_entry_t entry = group_entry_for_pri(thread->sched_group, processor->current_pri);

		if (entry->runq == MULTIQ_ERUNQ) {
			entry_queue_change_entry(entryq, entry, SCHED_TAILQ);
		}

		pset_unlock(pset);
	}
}

static boolean_t
sched_multiq_processor_queue_empty(processor_t processor)
{
	return multiq_main_entryq(processor)->count == 0 &&
	       multiq_bound_runq(processor)->count == 0;
}

static ast_t
sched_multiq_processor_csw_check(processor_t processor)
{
	boolean_t       has_higher;
	int             pri;

	if (sched_multiq_thread_avoid_processor(processor, current_thread())) {
		return AST_PREEMPT | AST_URGENT;
	}

	entry_queue_t main_entryq = multiq_main_entryq(processor);
	run_queue_t   bound_runq  = multiq_bound_runq(processor);

	assert(processor->active_thread != NULL);

	pri = MAX(main_entryq->highq, bound_runq->highq);

	if (processor->first_timeslice) {
		has_higher = (pri > processor->current_pri);
	} else {
		has_higher = (pri >= processor->current_pri);
	}

	if (has_higher) {
		if (main_entryq->urgency > 0) {
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
sched_multiq_processor_queue_has_priority(
	processor_t   processor,
	int           priority,
	boolean_t     gte)
{
	run_queue_t main_runq  = multiq_main_entryq(processor);
	run_queue_t bound_runq = multiq_bound_runq(processor);

	int qpri = MAX(main_runq->highq, bound_runq->highq);

	if (gte) {
		return qpri >= priority;
	} else {
		return qpri > priority;
	}
}

static int
sched_multiq_runq_count(processor_t processor)
{
	  
                                                                         
                                                       
   
                                                           
   
                                                                   
                                
    
	return multiq_main_entryq(processor)->count + multiq_bound_runq(processor)->count;
}

static uint64_t
sched_multiq_runq_stats_count_sum(processor_t processor)
{
	  
                                                                                      
                        
    

	uint64_t bound_sum = multiq_bound_runq(processor)->runq_stats.count_sum;

	if (processor->cpu_id == processor->processor_set->cpu_set_low) {
		return bound_sum + multiq_main_entryq(processor)->runq_stats.count_sum;
	} else {
		return bound_sum;
	}
}

static int
sched_multiq_processor_bound_count(processor_t processor)
{
	return multiq_bound_runq(processor)->count;
}

static void
sched_multiq_processor_queue_shutdown(processor_t processor)
{
	processor_set_t pset = processor->processor_set;
	entry_queue_t   main_entryq = multiq_main_entryq(processor);
	thread_t        thread;
	queue_head_t    tqueue;

	                                                                                      
	if (pset->online_processor_count > 0) {
		pset_unlock(pset);
		return;
	}

	queue_init(&tqueue);

	                                                                   

	while (main_entryq->count > 0) {
		thread = sched_global_dequeue_thread(main_entryq);
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
sched_multiq_processor_queue_remove(
	processor_t processor,
	thread_t    thread)
{
	boolean_t removed = FALSE;
	processor_set_t pset = processor->processor_set;

	pset_lock(pset);

	if (thread->runq != PROCESSOR_NULL) {
		  
                                                   
                    
     

		assert(thread->runq == processor);

		if (thread->bound_processor != PROCESSOR_NULL) {
			assert(processor == thread->bound_processor);
			run_queue_remove(multiq_bound_runq(processor), thread);
			thread->runq = PROCESSOR_NULL;
		} else {
			sched_group_remove_thread(multiq_main_entryq(processor),
			    thread->sched_group,
			    thread);
		}

		removed = TRUE;
	}

	pset_unlock(pset);

	return removed;
}

                                       
static thread_t
sched_multiq_steal_thread(processor_set_t pset)
{
	pset_unlock(pset);
	return THREAD_NULL;
}

  
                                                                        
                     
  
                                                                                                          
                                                                                              
  
                                   
   
static boolean_t
group_scan(entry_queue_t runq, sched_update_scan_context_t scan_context)
{
	int count       = runq->count;
	int queue_index;

	assert(count >= 0);

	if (count == 0) {
		return FALSE;
	}

	for (queue_index = bitmap_first(runq->bitmap, NRQS);
	    queue_index >= 0;
	    queue_index = bitmap_next(runq->bitmap, queue_index)) {
		sched_entry_t entry;

		cqe_foreach_element(entry, &runq->queues[queue_index], entry_links) {
			assert(count > 0);

			sched_group_t group = group_for_entry(entry);
			if (group->runq.count > 0) {
				if (runq_scan(&group->runq, scan_context)) {
					return TRUE;
				}
			}
			count--;
		}
	}

	return FALSE;
}

static void
sched_multiq_thread_update_scan(sched_update_scan_context_t scan_context)
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

			restart_needed = runq_scan(multiq_bound_runq(processor), scan_context);

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

	pset = &pset0;

	do {
		do {
			s = splsched();
			pset_lock(pset);

			restart_needed = group_scan(&pset->pset_runq, scan_context);

			pset_unlock(pset);
			splx(s);

			if (restart_needed) {
				break;
			}
		} while ((pset = pset->pset_list) != NULL);

		                                                             
		thread_update_process_threads();
	} while (restart_needed);
}

extern int sched_allow_rt_smt;

                                                                              
static bool
sched_multiq_thread_avoid_processor(processor_t processor, thread_t thread)
{
	if (processor->processor_primary != processor) {
		  
                                                                  
                                                                     
     
		if ((processor->processor_primary->current_pri >= BASEPRI_RTQUEUES) && ((thread->sched_pri < BASEPRI_RTQUEUES) || !sched_allow_rt_smt)) {
			return true;
		}
	}

	return false;
}
