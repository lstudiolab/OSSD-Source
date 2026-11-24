/*
 * Copyright (c) 2017 Apple, Inc. All rights reserved.
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

#ifndef _PTHREAD_WORKQUEUE_PRIVATE_H_
#define _PTHREAD_WORKQUEUE_PRIVATE_H_

#if XNU_KERNEL_PRIVATE && !defined(__PTHREAD_EXPOSE_INTERNALS__)
#define __PTHREAD_EXPOSE_INTERNALS__ 1
#endif                      

#ifdef __PTHREAD_EXPOSE_INTERNALS__
                               
#define WQOPS_THREAD_RETURN              0x004                                            
#define WQOPS_QUEUE_NEWSPISUPP           0x010                                             
#define WQOPS_QUEUE_REQTHREADS           0x020                                          
#define WQOPS_QUEUE_REQTHREADS2          0x030                                                             
#define WQOPS_THREAD_KEVENT_RETURN       0x040                                                                
#define WQOPS_SET_EVENT_MANAGER_PRIORITY 0x080                                                                              
#define WQOPS_THREAD_WORKLOOP_RETURN     0x100                                                                
#define WQOPS_SHOULD_NARROW              0x200                                                      
#define WQOPS_SETUP_DISPATCH             0x400                                                 

                                                                           
#define WQ_FLAG_THREAD_PRIO_SCHED               0x00008000
#define WQ_FLAG_THREAD_PRIO_QOS                 0x00004000
#define WQ_FLAG_THREAD_PRIO_MASK                0x00000fff

#define WQ_FLAG_THREAD_OVERCOMMIT               0x00010000                                      
#define WQ_FLAG_THREAD_REUSE                    0x00020000                              
#define WQ_FLAG_THREAD_NEWSPI                   0x00040000                                 
#define WQ_FLAG_THREAD_KEVENT                   0x00080000                                        
#define WQ_FLAG_THREAD_EVENT_MANAGER            0x00100000                            
#define WQ_FLAG_THREAD_TSD_BASE_SET             0x00200000                                     
#define WQ_FLAG_THREAD_WORKLOOP                 0x00400000                       
#define WQ_FLAG_THREAD_OUTSIDEQOS               0x00800000                                                       

#define WQ_KEVENT_LIST_LEN  16                                 
#define WQ_KEVENT_DATA_SIZE (32 * 1024)

                                  
#define KQ_WORKLOOP_CREATE                              0x01
#define KQ_WORKLOOP_DESTROY                             0x02

                                                                  
#define KQ_WORKLOOP_CREATE_SCHED_PRI    0x01
#define KQ_WORKLOOP_CREATE_SCHED_POL    0x02
#define KQ_WORKLOOP_CREATE_CPU_PERCENT  0x04

struct kqueue_workloop_params {
	int kqwlp_version;
	int kqwlp_flags;
	uint64_t kqwlp_id;
	int kqwlp_sched_pri;
	int kqwlp_sched_pol;
	int kqwlp_cpu_percent;
	int kqwlp_cpu_refillms;
} __attribute__((packed));

_Static_assert(offsetof(struct kqueue_workloop_params, kqwlp_version) == 0,
    "kqwlp_version should be first");

int
__workq_open(void);

int
__workq_kernreturn(int op, void *arg2, int arg3, int arg4);

int
__kqueue_workloop_ctl(uintptr_t cmd, uint64_t options, void *addr, size_t sz);

                                                                 
#define WQ_SETUP_NONE           0
#define WQ_SETUP_FIRST_USE      1
#define WQ_SETUP_CLEAR_VOUCHER  2
                                 
#define WQ_SETUP_EXIT_THREAD    8

#endif                                

#define WORKQ_DISPATCH_CONFIG_VERSION        2
#define WORKQ_DISPATCH_MIN_SUPPORTED_VERSION 1
#define WORKQ_DISPATCH_SUPPORTED_FLAGS       0
struct workq_dispatch_config {
	uint32_t wdc_version;
	uint32_t wdc_flags;
	uint64_t wdc_queue_serialno_offs;
	uint64_t wdc_queue_label_offs;
} __attribute__((packed, aligned(4)));

#endif                                 
