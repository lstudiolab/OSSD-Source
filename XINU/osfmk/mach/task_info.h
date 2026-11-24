/*
 * Copyright (c) 2000-2007, 2015 Apple Inc. All rights reserved.
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
  
                                                                   
  
                                                                     
                                                                  
                  
  
   

#ifndef _MACH_TASK_INFO_H_
#define _MACH_TASK_INFO_H_

#include <mach/message.h>
#include <mach/machine/vm_types.h>
#include <mach/time_value.h>
#include <mach/policy.h>
#include <mach/vm_statistics.h>                                      
#include <Availability.h>

#include <sys/cdefs.h>

  
                                                        
   
typedef natural_t       task_flavor_t;
typedef integer_t       *task_info_t;                                     

                                                     
#define TASK_INFO_MAX   (1024)                                  
typedef integer_t       task_info_data_t[TASK_INFO_MAX];

  
                                            
   

#pragma pack(push, 4)

                                                      
#define TASK_BASIC_INFO_32      4                              
#define TASK_BASIC2_INFO_32      6

struct task_basic_info_32 {
	integer_t       suspend_count;                              
	natural_t       virtual_size;                                    
	natural_t       resident_size;                                    
	time_value_t    user_time;                                
                                                          
	time_value_t    system_time;                                
                                                          
	policy_t        policy;                                             
};
typedef struct task_basic_info_32       task_basic_info_32_data_t;
typedef struct task_basic_info_32       *task_basic_info_32_t;
#define TASK_BASIC_INFO_32_COUNT   \
	        (sizeof(task_basic_info_32_data_t) / sizeof(natural_t))

                                                      
struct task_basic_info_64 {
	integer_t       suspend_count;                              
#if defined(__arm__) || defined(__arm64__)
#if defined(KERNEL)
	                                                  
	natural_t       virtual_size;                                    
	natural_t       resident_size;                                    
#else
	mach_vm_size_t  virtual_size;                                    
	mach_vm_size_t  resident_size;                                    
#endif
#else                                             
	mach_vm_size_t  virtual_size;                                    
	mach_vm_size_t  resident_size;                                    
#endif                                             
	time_value_t    user_time;                                
                                                          
	time_value_t    system_time;                                
                                                          
	policy_t        policy;                                             
};
typedef struct task_basic_info_64       task_basic_info_64_data_t;
typedef struct task_basic_info_64       *task_basic_info_64_t;

#if defined(__arm__) || defined(__arm64__)
	#if defined(KERNEL)
  
                                                    
                                                              
                                                              
                                                        
                                                        
   

	#define TASK_BASIC_INFO_64      5
	#define TASK_BASIC_INFO_64_COUNT   \
	        (sizeof(task_basic_info_64_data_t) / sizeof(natural_t))

	#elif defined(__arm__) && defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && (__IPHONE_OS_VERSION_MIN_REQUIRED < __IPHONE_5_0)
  
                                                                   
                                                
   
	#define TASK_BASIC_INFO_64      5
	#define TASK_BASIC_INFO_64_COUNT   \
	        (sizeof(task_basic_info_64_data_t) / sizeof(natural_t))

	#else

	#define TASK_BASIC_INFO_64              TASK_BASIC_INFO_64_2
	#define TASK_BASIC_INFO_64_COUNT        TASK_BASIC_INFO_64_2_COUNT
	#endif
#else                                             
#define TASK_BASIC_INFO_64      5                                      
#define TASK_BASIC_INFO_64_COUNT   \
	        (sizeof(task_basic_info_64_data_t) / sizeof(natural_t))
#endif


                                                                                    
                                                      
struct task_basic_info {
	integer_t       suspend_count;                              
	vm_size_t       virtual_size;                                    
	vm_size_t       resident_size;                                    
	time_value_t    user_time;                                
                                                          
	time_value_t    system_time;                                
                                                          
	policy_t        policy;                                             
};

typedef struct task_basic_info          task_basic_info_data_t;
typedef struct task_basic_info          *task_basic_info_t;
#define TASK_BASIC_INFO_COUNT   \
	        (sizeof(task_basic_info_data_t) / sizeof(natural_t))
#if !defined(__LP64__)
#define TASK_BASIC_INFO TASK_BASIC_INFO_32
#else
#define TASK_BASIC_INFO TASK_BASIC_INFO_64
#endif



#define TASK_EVENTS_INFO        2                                 

struct task_events_info {
	integer_t       faults;                                    
	integer_t       pageins;                                      
	integer_t       cow_faults;                                         
	integer_t       messages_sent;                               
	integer_t       messages_received;                                  
	integer_t       syscalls_mach;                                   
	integer_t       syscalls_unix;                                   
	integer_t       csw;                                            
};
typedef struct task_events_info         task_events_info_data_t;
typedef struct task_events_info         *task_events_info_t;
#define TASK_EVENTS_INFO_COUNT          ((mach_msg_type_number_t) \
	        (sizeof(task_events_info_data_t) / sizeof(natural_t)))

#define TASK_THREAD_TIMES_INFO  3                                        
                                                                  

struct task_thread_times_info {
	time_value_t    user_time;                                
                                                    
	time_value_t    system_time;                                
                                                    
};

typedef struct task_thread_times_info   task_thread_times_info_data_t;
typedef struct task_thread_times_info   *task_thread_times_info_t;
#define TASK_THREAD_TIMES_INFO_COUNT    ((mach_msg_type_number_t) \
	        (sizeof(task_thread_times_info_data_t) / sizeof(natural_t)))

#define TASK_ABSOLUTETIME_INFO  1

struct task_absolutetime_info {
	uint64_t                total_user;
	uint64_t                total_system;
	uint64_t                threads_user;                              
	uint64_t                threads_system;
};

typedef struct task_absolutetime_info   task_absolutetime_info_data_t;
typedef struct task_absolutetime_info   *task_absolutetime_info_t;
#define TASK_ABSOLUTETIME_INFO_COUNT    ((mach_msg_type_number_t) \
	        (sizeof (task_absolutetime_info_data_t) / sizeof (natural_t)))

#define TASK_KERNELMEMORY_INFO  7

struct task_kernelmemory_info {
	uint64_t                total_palloc;                                    
	uint64_t                total_pfree;                                  
	uint64_t                total_salloc;                                   
	uint64_t                total_sfree;                                 
};

typedef struct task_kernelmemory_info   task_kernelmemory_info_data_t;
typedef struct task_kernelmemory_info   *task_kernelmemory_info_t;
#define TASK_KERNELMEMORY_INFO_COUNT    ((mach_msg_type_number_t) \
	        (sizeof (task_kernelmemory_info_data_t) / sizeof (natural_t)))

#define TASK_SECURITY_TOKEN             13
#define TASK_SECURITY_TOKEN_COUNT       ((mach_msg_type_number_t) \
	        (sizeof(security_token_t) / sizeof(natural_t)))

#define TASK_AUDIT_TOKEN                15
#define TASK_AUDIT_TOKEN_COUNT  \
	        (sizeof(audit_token_t) / sizeof(natural_t))


#define TASK_AFFINITY_TAG_INFO          16                                 

struct task_affinity_tag_info {
	integer_t               set_count;
	integer_t               min;
	integer_t               max;
	integer_t               task_count;
};
typedef struct task_affinity_tag_info   task_affinity_tag_info_data_t;
typedef struct task_affinity_tag_info   *task_affinity_tag_info_t;
#define TASK_AFFINITY_TAG_INFO_COUNT    \
	        (sizeof(task_affinity_tag_info_data_t) / sizeof(natural_t))

#define TASK_DYLD_INFO                  17

struct task_dyld_info {
	mach_vm_address_t       all_image_info_addr;
	mach_vm_size_t          all_image_info_size;
	integer_t               all_image_info_format;
};
typedef struct task_dyld_info   task_dyld_info_data_t;
typedef struct task_dyld_info   *task_dyld_info_t;
#define TASK_DYLD_INFO_COUNT    \
	        (sizeof(task_dyld_info_data_t) / sizeof(natural_t))
#define TASK_DYLD_ALL_IMAGE_INFO_32     0                         
#define TASK_DYLD_ALL_IMAGE_INFO_64     1                         

#if defined(__arm__) || defined(__arm64__)

                                                      
                                              
#define TASK_BASIC_INFO_64_2     18                                      

struct task_basic_info_64_2 {
	integer_t       suspend_count;                              
	mach_vm_size_t  virtual_size;                                    
	mach_vm_size_t  resident_size;                                    
	time_value_t    user_time;                                
                                                          
	time_value_t    system_time;                                
                                                          
	policy_t        policy;                                             
};
typedef struct task_basic_info_64_2       task_basic_info_64_2_data_t;
typedef struct task_basic_info_64_2       *task_basic_info_64_2_t;
#define TASK_BASIC_INFO_64_2_COUNT   \
	        (sizeof(task_basic_info_64_2_data_t) / sizeof(natural_t))
#endif

#define TASK_EXTMOD_INFO                        19

struct task_extmod_info {
	unsigned char   task_uuid[16];
	vm_extmod_statistics_data_t             extmod_statistics;
};
typedef struct task_extmod_info task_extmod_info_data_t;
typedef struct task_extmod_info *task_extmod_info_t;
#define TASK_EXTMOD_INFO_COUNT  \
	        (sizeof(task_extmod_info_data_t) / sizeof(natural_t))


#define MACH_TASK_BASIC_INFO     20                                       
struct mach_task_basic_info {
	mach_vm_size_t  virtual_size;                                        
	mach_vm_size_t  resident_size;                                        
	mach_vm_size_t  resident_size_max;                                            
	time_value_t    user_time;                                    
                                                              
	time_value_t    system_time;                                    
                                                              
	policy_t        policy;                                                 
	integer_t       suspend_count;                                  
};
typedef struct mach_task_basic_info       mach_task_basic_info_data_t;
typedef struct mach_task_basic_info       *mach_task_basic_info_t;
#define MACH_TASK_BASIC_INFO_COUNT   \
	        (sizeof(mach_task_basic_info_data_t) / sizeof(natural_t))


#define TASK_POWER_INFO 21

struct task_power_info {
	uint64_t                total_user;
	uint64_t                total_system;
	uint64_t                task_interrupt_wakeups;
	uint64_t                task_platform_idle_wakeups;
	uint64_t                task_timer_wakeups_bin_1;
	uint64_t                task_timer_wakeups_bin_2;
};

typedef struct task_power_info  task_power_info_data_t;
typedef struct task_power_info  *task_power_info_t;
#define TASK_POWER_INFO_COUNT   ((mach_msg_type_number_t) \
	        (sizeof (task_power_info_data_t) / sizeof (natural_t)))



#define TASK_VM_INFO            22
#define TASK_VM_INFO_PURGEABLE  23
struct task_vm_info {
	mach_vm_size_t  virtual_size;                                        
	integer_t       region_count;                                     
	integer_t       page_size;
	mach_vm_size_t  resident_size;                                        
	mach_vm_size_t  resident_size_peak;                                 

	mach_vm_size_t  device;
	mach_vm_size_t  device_peak;
	mach_vm_size_t  internal;
	mach_vm_size_t  internal_peak;
	mach_vm_size_t  external;
	mach_vm_size_t  external_peak;
	mach_vm_size_t  reusable;
	mach_vm_size_t  reusable_peak;
	mach_vm_size_t  purgeable_volatile_pmap;
	mach_vm_size_t  purgeable_volatile_resident;
	mach_vm_size_t  purgeable_volatile_virtual;
	mach_vm_size_t  compressed;
	mach_vm_size_t  compressed_peak;
	mach_vm_size_t  compressed_lifetime;

	                    
	mach_vm_size_t  phys_footprint;

	                    
	mach_vm_address_t       min_address;
	mach_vm_address_t       max_address;

	                    
	int64_t ledger_phys_footprint_peak;
	int64_t ledger_purgeable_nonvolatile;
	int64_t ledger_purgeable_novolatile_compressed;
	int64_t ledger_purgeable_volatile;
	int64_t ledger_purgeable_volatile_compressed;
	int64_t ledger_tag_network_nonvolatile;
	int64_t ledger_tag_network_nonvolatile_compressed;
	int64_t ledger_tag_network_volatile;
	int64_t ledger_tag_network_volatile_compressed;
	int64_t ledger_tag_media_footprint;
	int64_t ledger_tag_media_footprint_compressed;
	int64_t ledger_tag_media_nofootprint;
	int64_t ledger_tag_media_nofootprint_compressed;
	int64_t ledger_tag_graphics_footprint;
	int64_t ledger_tag_graphics_footprint_compressed;
	int64_t ledger_tag_graphics_nofootprint;
	int64_t ledger_tag_graphics_nofootprint_compressed;
	int64_t ledger_tag_neural_footprint;
	int64_t ledger_tag_neural_footprint_compressed;
	int64_t ledger_tag_neural_nofootprint;
	int64_t ledger_tag_neural_nofootprint_compressed;

	                    
	uint64_t limit_bytes_remaining;

	                    
	integer_t decompressions;
};
typedef struct task_vm_info     task_vm_info_data_t;
typedef struct task_vm_info     *task_vm_info_t;
#define TASK_VM_INFO_COUNT      ((mach_msg_type_number_t) \
	        (sizeof (task_vm_info_data_t) / sizeof (natural_t)))
#define TASK_VM_INFO_REV5_COUNT TASK_VM_INFO_COUNT
#define TASK_VM_INFO_REV4_COUNT                                      \
	((mach_msg_type_number_t) (TASK_VM_INFO_REV5_COUNT - 1))
#define TASK_VM_INFO_REV3_COUNT                                   \
	((mach_msg_type_number_t) (TASK_VM_INFO_REV4_COUNT - 2))
#define TASK_VM_INFO_REV2_COUNT                                          \
	((mach_msg_type_number_t) (TASK_VM_INFO_REV3_COUNT - 42))
#define TASK_VM_INFO_REV1_COUNT                                           \
	((mach_msg_type_number_t) (TASK_VM_INFO_REV2_COUNT - 4))
#define TASK_VM_INFO_REV0_COUNT                                      \
	((mach_msg_type_number_t) (TASK_VM_INFO_REV1_COUNT - 2))

typedef struct vm_purgeable_info        task_purgable_info_t;


#define TASK_TRACE_MEMORY_INFO  24                           
struct task_trace_memory_info {
	uint64_t  user_memory_address;                                               
	uint64_t  buffer_size;                                               
	uint64_t  mailbox_array_size;                                      
};
typedef struct task_trace_memory_info task_trace_memory_info_data_t;
typedef struct task_trace_memory_info * task_trace_memory_info_t;
#define TASK_TRACE_MEMORY_INFO_COUNT  ((mach_msg_type_number_t) \
	        (sizeof(task_trace_memory_info_data_t) / sizeof(natural_t)))

#define TASK_WAIT_STATE_INFO  25                     
struct task_wait_state_info {
	uint64_t  total_wait_state_time;                                                                              
	uint64_t  total_wait_sfi_state_time;                                                                                              
	uint32_t  _reserved[4];
};
typedef struct task_wait_state_info task_wait_state_info_data_t;
typedef struct task_wait_state_info * task_wait_state_info_t;
#define TASK_WAIT_STATE_INFO_COUNT  ((mach_msg_type_number_t) \
	        (sizeof(task_wait_state_info_data_t) / sizeof(natural_t)))

#define TASK_POWER_INFO_V2      26

typedef struct {
	uint64_t                task_gpu_utilisation;
	uint64_t                task_gpu_stat_reserved0;
	uint64_t                task_gpu_stat_reserved1;
	uint64_t                task_gpu_stat_reserved2;
} gpu_energy_data;

typedef gpu_energy_data *gpu_energy_data_t;
struct task_power_info_v2 {
	task_power_info_data_t  cpu_energy;
	gpu_energy_data gpu_energy;
#if defined(__arm__) || defined(__arm64__)
	uint64_t                task_energy;
#endif                                             
	uint64_t                task_ptime;
	uint64_t                task_pset_switches;
};

typedef struct task_power_info_v2       task_power_info_v2_data_t;
typedef struct task_power_info_v2       *task_power_info_v2_t;
#define TASK_POWER_INFO_V2_COUNT_OLD    \
	        ((mach_msg_type_number_t) (sizeof (task_power_info_v2_data_t) - sizeof(uint64_t)*2) / sizeof (natural_t))
#define TASK_POWER_INFO_V2_COUNT        \
	        ((mach_msg_type_number_t) (sizeof (task_power_info_v2_data_t) / sizeof (natural_t)))

#define TASK_VM_INFO_PURGEABLE_ACCOUNT 27                                           

#ifdef PRIVATE
struct pvm_account_info {
	uint64_t pvm_volatile_count;                                                      
	uint64_t pvm_volatile_compressed_count;                                                                 
	uint64_t pvm_nonvolatile_count;                                                         
	uint64_t pvm_nonvolatile_compressed_count;                                                                    
};

typedef struct pvm_account_info *pvm_account_info_t;
typedef struct pvm_account_info pvm_account_info_data_t;

#define PVM_ACCOUNT_INFO_COUNT ((mach_msg_type_number_t) \
	        (sizeof (pvm_account_info_data_t) / sizeof (natural_t)))
#endif              

#define TASK_FLAGS_INFO  28                                               
struct task_flags_info {
	uint32_t        flags;                                          
};
typedef struct task_flags_info task_flags_info_data_t;
typedef struct task_flags_info * task_flags_info_t;
#define TASK_FLAGS_INFO_COUNT  ((mach_msg_type_number_t) \
	        (sizeof(task_flags_info_data_t) / sizeof (natural_t)))

#define TF_LP64         0x00000001                                 
#define TF_64B_DATA     0x00000002                                     

#define TASK_DEBUG_INFO_INTERNAL    29                                                  

#ifdef PRIVATE
struct task_debug_info_internal {
	integer_t suspend_count;
	uint64_t ipc_space_size;
};
typedef struct task_debug_info_internal *task_debug_info_internal_t;
typedef struct task_debug_info_internal task_debug_info_internal_data_t;
#define TASK_DEBUG_INFO_INTERNAL_COUNT  ((mach_msg_type_number_t) \
	        (sizeof (task_debug_info_internal_data_t) / sizeof(natural_t)))

#endif              

  
                                                        
                                                    
   
typedef uint32_t task_exc_guard_behavior_t;

                                                              
#define TASK_EXC_GUARD_VM_DELIVER            0x01                                                  
#define TASK_EXC_GUARD_VM_ONCE               0x02                             
#define TASK_EXC_GUARD_VM_CORPSE             0x04                                       
#define TASK_EXC_GUARD_VM_FATAL              0x08                                                 
#define TASK_EXC_GUARD_VM_ALL                0x0f

#define TASK_EXC_GUARD_MP_DELIVER            0x10                                             
#define TASK_EXC_GUARD_MP_ONCE               0x20                             
#define TASK_EXC_GUARD_MP_CORPSE             0x40                                       
#define TASK_EXC_GUARD_MP_FATAL              0x80                                            
#define TASK_EXC_GUARD_MP_ALL                0xf0

#define TASK_EXC_GUARD_ALL                   0xff                                    

#ifdef PRIVATE
  
                                                                              
                                                                                                  
   
#define TASK_EXC_GUARD_THIRD_PARTY_DEFAULT_SHIFT 0x8                                               
#endif

  
                       
   

#define TASK_SCHED_TIMESHARE_INFO       10
#define TASK_SCHED_RR_INFO              11
#define TASK_SCHED_FIFO_INFO            12

#define TASK_SCHED_INFO                 14

#pragma pack(pop)

#endif                          
