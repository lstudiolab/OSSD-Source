/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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

#ifndef _KERN_SCHED_AMP_COMMON_H_
#define _KERN_SCHED_AMP_COMMON_H_

#if __AMP__

                                                           
void sched_amp_init(void);

  
                                                                                               
                                                                              
  
                                                                                                            
                                                                                                             
                                              
  
                                                                                                       
                                      
  
                                                                                                       
                                                                     
   

                 
int sched_amp_spill_threshold(processor_set_t pset);
void pset_signal_spill(processor_set_t pset, int spilled_thread_priority);
bool pset_should_accept_spilled_thread(processor_set_t pset, int spilled_thread_priority);
bool should_spill_to_ecores(processor_set_t nset, thread_t thread);
void sched_amp_check_spill(processor_set_t pset, thread_t thread);

                 
int sched_amp_steal_threshold(processor_set_t pset, bool spill_pending);
bool sched_amp_steal_thread_enabled(processor_set_t pset);

                     
void sched_amp_balance(processor_t cprocessor, processor_set_t cpset);

                
sched_ipi_type_t sched_amp_ipi_policy(processor_t dst, thread_t thread, boolean_t dst_idle, sched_ipi_event_t event);

                                  
rt_queue_t sched_amp_rt_runq(processor_set_t pset);
void sched_amp_rt_init(processor_set_t pset);
void sched_amp_rt_queue_shutdown(processor_t processor);
void sched_amp_rt_runq_scan(sched_update_scan_context_t scan_context);
int64_t sched_amp_rt_runq_count_sum(void);

uint32_t sched_amp_qos_max_parallelism(int qos, uint64_t options);
void sched_amp_bounce_thread_group_from_ecores(processor_set_t pset, struct thread_group *tg);

pset_node_t sched_amp_choose_node(thread_t thread);

#endif              

#endif                                
