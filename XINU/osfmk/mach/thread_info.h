/*
 * Copyright (c) 2000-2005, 2015 Apple Computer, Inc. All rights reserved.
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
  
   
  
                         
  
                                                
  
                                                                    
                                                                  
                  
  
   

#ifndef _MACH_THREAD_INFO_H_
#define _MACH_THREAD_INFO_H_

#include <mach/boolean.h>
#include <mach/policy.h>
#include <mach/time_value.h>
#include <mach/message.h>
#include <mach/machine/vm_types.h>

  
                                                        
   
typedef natural_t       thread_flavor_t;
typedef integer_t       *thread_info_t;                                   

#define THREAD_INFO_MAX         (32)                            
typedef integer_t       thread_info_data_t[THREAD_INFO_MAX];

  
                                 
   
#define THREAD_BASIC_INFO               3                            

struct thread_basic_info {
	time_value_t    user_time;                         
	time_value_t    system_time;                         
	integer_t       cpu_usage;                                       
	policy_t        policy;                                          
	integer_t       run_state;                                 
	integer_t       flags;                                         
	integer_t       suspend_count;                                
	integer_t       sleep_time;                                     
                                                         
};

typedef struct thread_basic_info  thread_basic_info_data_t;
typedef struct thread_basic_info  *thread_basic_info_t;
#define THREAD_BASIC_INFO_COUNT   ((mach_msg_type_number_t) \
	        (sizeof(thread_basic_info_data_t) / sizeof(natural_t)))

#define THREAD_IDENTIFIER_INFO          4                                          

struct thread_identifier_info {
	uint64_t        thread_id;                                               
	uint64_t        thread_handle;                                    
	uint64_t        dispatch_qaddr;                                
};

typedef struct thread_identifier_info  thread_identifier_info_data_t;
typedef struct thread_identifier_info  *thread_identifier_info_t;
#define THREAD_IDENTIFIER_INFO_COUNT   ((mach_msg_type_number_t) \
	        (sizeof(thread_identifier_info_data_t) / sizeof(natural_t)))

  
                                
   

#define TH_USAGE_SCALE  1000

  
                                   
   

#define TH_STATE_RUNNING        1                                       
#define TH_STATE_STOPPED        2                              
#define TH_STATE_WAITING        3                                       
#define TH_STATE_UNINTERRUPTIBLE 4                                        
                                            
#define TH_STATE_HALTED         5                               
                                                   

  
                              
   
#define TH_FLAGS_SWAPPED        0x1                                
#define TH_FLAGS_IDLE           0x2                                   
#define TH_FLAGS_GLOBAL_FORCED_IDLE     0x4                                             

  
                                                                                       
   
#define THREAD_EXTENDED_INFO 5
#define MAXTHREADNAMESIZE 64
struct thread_extended_info {                                                                                                         
	uint64_t                pth_user_time;                             
	uint64_t                pth_system_time;                             
	int32_t                 pth_cpu_usage;                                           
	int32_t                 pth_policy;                                                      
	int32_t                 pth_run_state;                                     
	int32_t                 pth_flags;                                             
	int32_t                 pth_sleep_time;                                            
	int32_t                 pth_curpri;                                      
	int32_t                 pth_priority;                         
	int32_t                 pth_maxpriority;                         
	char                    pth_name[MAXTHREADNAMESIZE];                             
};
typedef struct thread_extended_info thread_extended_info_data_t;
typedef struct thread_extended_info * thread_extended_info_t;
#define THREAD_EXTENDED_INFO_COUNT  ((mach_msg_type_number_t) \
	        (sizeof(thread_extended_info_data_t) / sizeof (natural_t)))

#define THREAD_DEBUG_INFO_INTERNAL 6                                              

#if PRIVATE
struct thread_debug_info_internal {
	uint64_t page_creation_count;
};

typedef struct thread_debug_info_internal *thread_debug_info_internal_t;
typedef struct thread_debug_info_internal  thread_debug_info_internal_data_t;

#define THREAD_DEBUG_INFO_INTERNAL_COUNT  ((mach_msg_type_number_t)             \
	                (sizeof (thread_debug_info_internal_data_t) / sizeof (natural_t)))

#endif              

#define IO_NUM_PRIORITIES       4

#define UPDATE_IO_STATS(info, size)                             \
{                                                               \
	info.count++;                                           \
	info.size += size;                                      \
}

#define UPDATE_IO_STATS_ATOMIC(info, io_size)                   \
{                                                               \
	OSIncrementAtomic64((SInt64 *)&(info.count));           \
	OSAddAtomic64(io_size, (SInt64 *)&(info.size));         \
}

struct io_stat_entry {
	uint64_t        count;
	uint64_t        size;
};

struct io_stat_info {
	struct io_stat_entry    disk_reads;
	struct io_stat_entry    io_priority[IO_NUM_PRIORITIES];
	struct io_stat_entry    paging;
	struct io_stat_entry    metadata;
	struct io_stat_entry    total_io;
};

typedef struct io_stat_info *io_stat_info_t;

#if KERNEL_PRIVATE
__BEGIN_DECLS
void thread_group_join_io_storage(void);
void thread_group_join_perf_controller(void);
__END_DECLS
#endif

  
                       
   

#define THREAD_SCHED_TIMESHARE_INFO     10
#define THREAD_SCHED_RR_INFO            11
#define THREAD_SCHED_FIFO_INFO          12

#endif                            
