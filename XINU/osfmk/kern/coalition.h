/*
 * Copyright (c) 2013 Apple Computer, Inc. All rights reserved.
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

#ifndef _KERN_COALITION_H_
#define _KERN_COALITION_H_

                                    
#ifdef XNU_KERNEL_PRIVATE
#include <mach/coalition.h>
#include <kern/thread_group.h>

#if CONFIG_COALITIONS

void coalitions_init(void);

                    
                                                                                             
                                                                                       
   
kern_return_t coalitions_adopt_task(coalition_t *coaltions, task_t task);
kern_return_t coalitions_adopt_init_task(task_t task);
kern_return_t coalitions_adopt_corpse_task(task_t task);

                                                                         
                                                             
   
kern_return_t coalitions_remove_task(task_t task);
void          task_release_coalitions(task_t task);

  
  
   
kern_return_t coalitions_set_roles(coalition_t coalitions[COALITION_NUM_TYPES],
    task_t task, int roles[COALITION_NUM_TYPES]);

uint64_t coalition_id(coalition_t coal);
void     task_coalition_ids(task_t task, uint64_t ids[COALITION_NUM_TYPES]);
void     task_coalition_roles(task_t task, int roles[COALITION_NUM_TYPES]);
int      coalition_type(coalition_t coal);

void     task_coalition_update_gpu_stats(task_t task, uint64_t gpu_ns_delta);
boolean_t task_coalition_adjust_focal_count(task_t task, int count, uint32_t *new_count);
uint32_t task_coalition_focal_count(task_t task);
boolean_t task_coalition_adjust_nonfocal_count(task_t task, int count, uint32_t *new_count);
uint32_t task_coalition_nonfocal_count(task_t task);
struct thread_group *task_coalition_get_thread_group(task_t task);
void     coalition_set_thread_group(coalition_t coal, struct thread_group *tg);
struct thread_group *kdp_coalition_get_thread_group(coalition_t coal);
struct thread_group *coalition_get_thread_group(coalition_t coal);
void task_coalition_thread_group_focal_update(task_t task);

void coalition_for_each_task(coalition_t coal, void *ctx,
    void (*callback)(coalition_t, void *, task_t));

void coalition_set_efficient(coalition_t coal);

                        
struct coalition_ledger_indices {
	int logical_writes;
};
void init_coalition_ledgers(void);
int coalition_ledger_set_logical_writes_limit(coalition_t coal, int64_t limit);
void coalition_io_monitor_ctl(struct coalition *coalition, uint32_t flags, int64_t limit);
ledger_t coalition_ledger_get_from_task(task_t task);
void coalition_io_rate_exceeded(int warning, const void *param0, __unused const void *param1);
void coalition_io_ledger_update(task_t task, int32_t flavor, boolean_t is_credit, uint32_t io_size);

                                                                           
#define COALITION_MAX_LOGICAL_WRITES_LIMIT ((ledger_amount_t)(1ULL << 24))
                                                  
#define COALITION_LEDGER_MONITOR_INTERVAL_SECS (24 * 60 * 60)


typedef void (*coalition_iterate_fn_t)(void*, int, coalition_t);
kern_return_t coalition_iterate_stackshot(coalition_iterate_fn_t callout, void *arg, uint32_t coalition_type);

                                               
                                   
   
coalition_t coalition_find_by_id(uint64_t coal_id);

                                                                 
                                   
   
coalition_t coalition_find_and_activate_by_id(uint64_t coal_id);

void coalition_remove_active(coalition_t coal);

void coalition_release(coalition_t coal);

  
                                                                
                                                                         
   

                   
                                                                         
                       
                                                    
                                                                          
   
kern_return_t coalition_reap_internal(coalition_t coal);

                   
                                                                         
                       
                                                                        
                                                      
   
kern_return_t coalition_request_terminate_internal(coalition_t coal);

                   
                                                                   
                    
   
kern_return_t coalition_create_internal(int type, int role, boolean_t privileged, coalition_t *out, uint64_t *cid);

boolean_t coalition_term_requested(coalition_t coal);
boolean_t coalition_is_terminated(coalition_t coal);
boolean_t coalition_is_reaped(coalition_t coal);
boolean_t coalition_is_privileged(coalition_t coal);
boolean_t task_is_in_privileged_coalition(task_t task, int type);

kern_return_t coalition_resource_usage_internal(coalition_t coal, struct coalition_resource_usage *cru_out);

task_t kdp_coalition_get_leader(coalition_t coal);

  
                               
   
#if DEVELOPMENT || DEBUG
int coalition_should_notify(coalition_t coal);
void coalition_set_notify(coalition_t coal, int notify);
#endif

#else                         

static inline void
task_coalition_update_gpu_stats(__unused task_t task,
    __unused uint64_t gpu_ns_delta)
{
	return;
}

static inline boolean_t
task_coalition_adjust_focal_count(__unused task_t task,
    __unused int count,
    __unused uint32_t *new_count)
{
	return FALSE;
}

static inline boolean_t
task_coalition_adjust_nonfocal_count(__unused task_t task,
    __unused int count,
    __unused uint32_t *new_count)
{
	return FALSE;
}

static inline uint32_t
task_coalition_focal_count(__unused task_t task)
{
	return 0;
}

static inline void
coalition_for_each_task(__unused coalition_t coal,
    __unused void *ctx,
    __unused void (*callback)(coalition_t, void *, task_t))
{
	return;
}

#endif                        
#endif                         
#endif                        
