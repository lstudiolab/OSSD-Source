/*
 * Copyright (c) 2000-2015 Apple Inc. All rights reserved.
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
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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

  
                         
  
                                  
   

#ifndef _MACH_HOST_INFO_H_
#define _MACH_HOST_INFO_H_

#include <mach/message.h>
#include <mach/vm_statistics.h>
#include <mach/machine.h>
#include <mach/machine/vm_types.h>
#include <mach/time_value.h>

#include <sys/cdefs.h>

  
                                                        
   
typedef integer_t       *host_info_t;                                      
typedef integer_t       *host_info64_t;                                    

#define HOST_INFO_MAX   (1024)                              
typedef integer_t       host_info_data_t[HOST_INFO_MAX];

#define KERNEL_VERSION_MAX (512)
typedef char    kernel_version_t[KERNEL_VERSION_MAX];

#define KERNEL_BOOT_INFO_MAX (4096)
typedef char    kernel_boot_info_t[KERNEL_BOOT_INFO_MAX];

  
                                 
   
                 
typedef integer_t       host_flavor_t;
#define HOST_BASIC_INFO         1                       
#define HOST_SCHED_INFO         3                            
#define HOST_RESOURCE_SIZES     4                                
#define HOST_PRIORITY_INFO      5                                 
#define HOST_SEMAPHORE_TRAPS    7                                
#define HOST_MACH_MSG_TRAP      8                              
#define HOST_VM_PURGABLE        9                                    
#define HOST_DEBUG_INFO_INTERNAL 10                                                          
#define HOST_CAN_HAS_DEBUGGER   11
#define HOST_PREFERRED_USER_ARCH 12                                                    

#ifdef MACH_KERNEL_PRIVATE
struct host_basic_info_old {
	integer_t       max_cpus;                                        
	uint32_t        avail_cpus;                                       
	natural_t       memory_size;                                 
	cpu_type_t      cpu_type;                     
	cpu_subtype_t   cpu_subtype;                     
};

typedef struct host_basic_info_old      host_basic_info_data_old_t;
typedef struct host_basic_info_old      *host_basic_info_old_t;
#define HOST_BASIC_INFO_OLD_COUNT ((mach_msg_type_number_t) \
	        (sizeof(host_basic_info_data_old_t)/sizeof(integer_t)))
#endif                          

struct host_can_has_debugger_info {
	boolean_t       can_has_debugger;
};
typedef struct host_can_has_debugger_info       host_can_has_debugger_info_data_t;
typedef struct host_can_has_debugger_info       *host_can_has_debugger_info_t;
#define HOST_CAN_HAS_DEBUGGER_COUNT ((mach_msg_type_number_t) \
	        (sizeof(host_can_has_debugger_info_data_t)/sizeof(integer_t)))

#pragma pack(push, 4)

struct host_basic_info {
	integer_t               max_cpus;                                                
	integer_t               avail_cpus;                                               
	natural_t               memory_size;                                                         
	cpu_type_t              cpu_type;                             
	cpu_subtype_t           cpu_subtype;                             
	cpu_threadtype_t        cpu_threadtype;                             
	integer_t               physical_cpu;                                                      
	integer_t               physical_cpu_max;                                                 
	integer_t               logical_cpu;                                                     
	integer_t               logical_cpu_max;                                                  
	uint64_t                max_mem;                                                    
};

#pragma pack(pop)

typedef struct host_basic_info  host_basic_info_data_t;
typedef struct host_basic_info  *host_basic_info_t;
#define HOST_BASIC_INFO_COUNT ((mach_msg_type_number_t) \
	        (sizeof(host_basic_info_data_t)/sizeof(integer_t)))

struct host_sched_info {
	integer_t       min_timeout;                                         
	integer_t       min_quantum;                                         
};

typedef struct host_sched_info  host_sched_info_data_t;
typedef struct host_sched_info  *host_sched_info_t;
#define HOST_SCHED_INFO_COUNT ((mach_msg_type_number_t) \
	        (sizeof(host_sched_info_data_t)/sizeof(integer_t)))

struct kernel_resource_sizes {
	natural_t       task;
	natural_t       thread;
	natural_t       port;
	natural_t       memory_region;
	natural_t       memory_object;
};

typedef struct kernel_resource_sizes    kernel_resource_sizes_data_t;
typedef struct kernel_resource_sizes    *kernel_resource_sizes_t;
#define HOST_RESOURCE_SIZES_COUNT ((mach_msg_type_number_t) \
	        (sizeof(kernel_resource_sizes_data_t)/sizeof(integer_t)))

struct host_priority_info {
	integer_t       kernel_priority;
	integer_t       system_priority;
	integer_t       server_priority;
	integer_t       user_priority;
	integer_t       depress_priority;
	integer_t       idle_priority;
	integer_t       minimum_priority;
	integer_t       maximum_priority;
};

typedef struct host_priority_info       host_priority_info_data_t;
typedef struct host_priority_info       *host_priority_info_t;
#define HOST_PRIORITY_INFO_COUNT ((mach_msg_type_number_t) \
	        (sizeof(host_priority_info_data_t)/sizeof(integer_t)))

                       
#define HOST_LOAD_INFO          1                                 
#define HOST_VM_INFO            2                                 
#define HOST_CPU_LOAD_INFO      3                           

                         
#define HOST_VM_INFO64          4                                        
#define HOST_EXTMOD_INFO64      5                                        
#define HOST_EXPIRED_TASK_INFO  6                                         

#ifdef XNU_KERNEL_PRIVATE
void host_statistics_init(void);
#endif

struct host_load_info {
	integer_t       avenrun[3];                               
	integer_t       mach_factor[3];                           
};

typedef struct host_load_info   host_load_info_data_t;
typedef struct host_load_info   *host_load_info_t;
#define HOST_LOAD_INFO_COUNT ((mach_msg_type_number_t) \
	        (sizeof(host_load_info_data_t)/sizeof(integer_t)))

typedef struct vm_purgeable_info        host_purgable_info_data_t;
typedef struct vm_purgeable_info        *host_purgable_info_t;
#define HOST_VM_PURGABLE_COUNT ((mach_msg_type_number_t) \
	        (sizeof(host_purgable_info_data_t)/sizeof(integer_t)))

                               
                     
#define HOST_VM_INFO64_COUNT ((mach_msg_type_number_t) \
	        (sizeof(vm_statistics64_data_t)/sizeof(integer_t)))

                                                 
#define HOST_VM_INFO64_LATEST_COUNT HOST_VM_INFO64_COUNT
#define HOST_VM_INFO64_REV1_COUNT HOST_VM_INFO64_LATEST_COUNT
                                                                              
#define HOST_VM_INFO64_REV0_COUNT                                                    \
	((mach_msg_type_number_t) \
	 (HOST_VM_INFO64_REV1_COUNT - 14))

                               
                          
#define HOST_EXTMOD_INFO64_COUNT ((mach_msg_type_number_t) \
	    (sizeof(vm_extmod_statistics_data_t)/sizeof(integer_t)))

                                                 
#define HOST_EXTMOD_INFO64_LATEST_COUNT HOST_EXTMOD_INFO64_COUNT

                   
#define HOST_VM_INFO_COUNT ((mach_msg_type_number_t) \
	        (sizeof(vm_statistics_data_t)/sizeof(integer_t)))

                                                 
#define HOST_VM_INFO_LATEST_COUNT HOST_VM_INFO_COUNT
#define HOST_VM_INFO_REV2_COUNT HOST_VM_INFO_LATEST_COUNT
                                                                              
#define HOST_VM_INFO_REV1_COUNT                                         \
	((mach_msg_type_number_t) \
	 (HOST_VM_INFO_REV2_COUNT - 1))
#define HOST_VM_INFO_REV0_COUNT                                         \
	((mach_msg_type_number_t) \
	 (HOST_VM_INFO_REV1_COUNT - 2))

struct host_cpu_load_info {                                                   
	natural_t       cpu_ticks[CPU_STATE_MAX];                            
};

typedef struct host_cpu_load_info       host_cpu_load_info_data_t;
typedef struct host_cpu_load_info       *host_cpu_load_info_t;
#define HOST_CPU_LOAD_INFO_COUNT ((mach_msg_type_number_t) \
	        (sizeof (host_cpu_load_info_data_t) / sizeof (integer_t)))

struct host_preferred_user_arch {
	cpu_type_t      cpu_type;                                          
	cpu_subtype_t   cpu_subtype;                                          
};

typedef struct host_preferred_user_arch host_preferred_user_arch_data_t;
typedef struct host_preferred_user_arch *host_preferred_user_arch_t;
#define HOST_PREFERRED_USER_ARCH_COUNT ((mach_msg_type_number_t) \
	        (sizeof(host_preferred_user_arch_data_t)/sizeof(integer_t)))

#ifdef PRIVATE
  
                             
   
struct _processor_statistics_np  {
	int32_t                 ps_cpuid;

	uint32_t                ps_csw_count;
	uint32_t                ps_preempt_count;
	uint32_t                ps_preempted_rt_count;
	uint32_t                ps_preempted_by_rt_count;

	uint32_t                ps_rt_sched_count;

	uint32_t                ps_interrupt_count;
	uint32_t                ps_ipi_count;
	uint32_t                ps_timer_pop_count;

	uint64_t                ps_runq_count_sum __attribute((aligned(8)));

	uint32_t                ps_idle_transitions;
	uint32_t                ps_quantum_timer_expirations;
};

struct host_debug_info_internal {
	uint64_t config_bank:1,                             
	    config_atm:1,
	    config_csr:1,
	    config_coalitions:1,
	    config_unused:60;
};

typedef struct host_debug_info_internal *host_debug_info_internal_t;
typedef struct host_debug_info_internal  host_debug_info_internal_data_t;
#define HOST_DEBUG_INFO_INTERNAL_COUNT  ((mach_msg_type_number_t)\
	(sizeof (host_debug_info_internal_data_t) / sizeof(integer_t)))

#endif              

#ifdef KERNEL_PRIVATE

extern kern_return_t    set_sched_stats_active(
	boolean_t active);

extern kern_return_t    get_sched_statistics(
	struct _processor_statistics_np *out,
	uint32_t *count);
#endif                      


#endif                          
