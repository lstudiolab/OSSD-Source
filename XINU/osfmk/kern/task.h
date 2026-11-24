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
 * @OSF_FREE_COPYRIGHT@
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
  
   
  
               
                               
  
                                                          
  
   
/*
 * Copyright (c) 1993 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 */
/*
 * NOTICE: This file was modified by McAfee Research in 2004 to introduce
 * support for mandatory and extensible security protections.  This notice
 * is included in support of clause 2.2 (b) of the Apple Public License,
 * Version 2.0.
 * Copyright (c) 2005 SPARTA, Inc.
 */

#ifndef _KERN_TASK_H_
#define _KERN_TASK_H_

#include <kern/kern_types.h>
#include <mach/mach_types.h>
#include <sys/cdefs.h>

#ifdef XNU_KERNEL_PRIVATE
#include <kern/kern_cdata.h>
#include <mach/sfi_class.h>
#include <kern/counter.h>
#include <kern/queue.h>
#include <sys/kern_sysctl.h>
#endif                         

#ifdef  MACH_KERNEL_PRIVATE

#include <mach/boolean.h>
#include <mach/port.h>
#include <mach/time_value.h>
#include <mach/message.h>
#include <mach/mach_param.h>
#include <mach/task_info.h>
#include <mach/exception_types.h>
#include <mach/vm_statistics.h>
#include <machine/task.h>

#if MONOTONIC
#include <machine/monotonic.h>
#endif                

#include <kern/cpu_data.h>
#include <kern/queue.h>
#include <kern/exception.h>
#include <kern/locks.h>
#include <security/_label.h>
#include <ipc/ipc_port.h>

#include <kern/thread.h>
#include <mach/coalition.h>
#include <stdatomic.h>
#include <os/refcnt.h>

struct _cpu_time_qos_stats {
	uint64_t cpu_time_qos_default;
	uint64_t cpu_time_qos_maintenance;
	uint64_t cpu_time_qos_background;
	uint64_t cpu_time_qos_utility;
	uint64_t cpu_time_qos_legacy;
	uint64_t cpu_time_qos_user_initiated;
	uint64_t cpu_time_qos_user_interactive;
};

struct task_writes_counters {
	uint64_t task_immediate_writes;
	uint64_t task_deferred_writes;
	uint64_t task_invalidated_writes;
	uint64_t task_metadata_writes;
};

struct task_watchports;
#include <bank/bank_internal.h>

struct task {
	                                             
	decl_lck_mtx_data(, lock);                       
	os_refcnt_t     ref_count;                                      
	bool            active;                                           
	bool            ipc_active;                                             
	bool            halting;                                  
	bool            message_app_suspended;                                        

	                    
	uint32_t                vtimers;

	                   
	vm_map_t        XNU_PTRAUTH_SIGNED_PTR("task.map") map;                                
	queue_chain_t   tasks;                            
	struct task_watchports *watchports;                                 
	turnstile_inheritor_t returnwait_inheritor;                              

#if defined(CONFIG_SCHED_MULTIQ)
	sched_group_t sched_group;
#endif                                   

	                          
	queue_head_t            threads;
	struct restartable_ranges *restartable_ranges;

	processor_set_t         pset_hint;
	struct affinity_space   *affinity_space;

	int                     thread_count;
	uint32_t                active_thread_count;
	int                     suspend_count;                                

	                                         
	integer_t               user_stop_count;                               
	integer_t               legacy_stop_count;                                    

	int16_t                 priority;                                              
	int16_t                 max_priority;                                             

	integer_t               importance;                                                     

	                                    
	security_token_t sec_token;
	audit_token_t   audit_token;

	                
	uint64_t                total_user_time;                                     
	uint64_t                total_system_time;
	uint64_t                total_ptime;
	uint64_t                total_runnable_time;

	                    
	decl_lck_mtx_data(, itk_lock_data);
	  
                                   
                                                           
    
	struct ipc_port * XNU_PTRAUTH_SIGNED_PTR("task.itk_task_ports") itk_task_ports[TASK_SELF_PORT_COUNT];
	struct ipc_port * XNU_PTRAUTH_SIGNED_PTR("task.itk_settable_self") itk_settable_self;                     
	struct ipc_port * XNU_PTRAUTH_SIGNED_PTR("task.itk_self") itk_self;                                                                          
	struct exception_action exc_actions[EXC_TYPES_COUNT];
	                                      
	struct ipc_port * XNU_PTRAUTH_SIGNED_PTR("task.itk_host") itk_host;                                       
	struct ipc_port * XNU_PTRAUTH_SIGNED_PTR("task.itk_bootstrap") itk_bootstrap;                             
	struct ipc_port * XNU_PTRAUTH_SIGNED_PTR("task.itk_seatbelt") itk_seatbelt;                               
	struct ipc_port * XNU_PTRAUTH_SIGNED_PTR("task.itk_gssd") itk_gssd;                                                 
	struct ipc_port * XNU_PTRAUTH_SIGNED_PTR("task.itk_debug_control") itk_debug_control;                                                
	struct ipc_port * XNU_PTRAUTH_SIGNED_PTR("task.itk_task_access") itk_task_access;                                   
	struct ipc_port * XNU_PTRAUTH_SIGNED_PTR("task.itk_resume") itk_resume;                                                          
	struct ipc_port * XNU_PTRAUTH_SIGNED_PTR("task.itk_registered") itk_registered[TASK_PORT_REGISTER_MAX];
	                     
	ipc_port_t * XNU_PTRAUTH_SIGNED_PTR("task.itk_dyld_notify") itk_dyld_notify;                                                                        

	struct ipc_space * XNU_PTRAUTH_SIGNED_PTR("task.itk_space") itk_space;

	ledger_t        ledger;
	                                        
	queue_head_t    semaphore_list;                                         
	int             semaphores_owned;                                       

	unsigned int    priv_flags;                                                   
#define VM_BACKING_STORE_PRIV   0x1

	MACHINE_TASK

	counter_t faults;                                  
	integer_t decompressions;                                 
	integer_t pageins;                                  
	integer_t cow_faults;                                           
	integer_t messages_sent;                                  
	integer_t messages_received;                                  
	integer_t syscalls_mach;                                     
	integer_t syscalls_unix;                                     
	uint32_t  c_switch;                                                            
	uint32_t  p_switch;                                                              
	uint32_t  ps_switch;                                                

#ifdef  MACH_BSD
	void * XNU_PTRAUTH_SIGNED_PTR("task.bsd_info") bsd_info;
#endif
	kcdata_descriptor_t             corpse_info;
	uint64_t                        crashed_thread_id;
	queue_chain_t                   corpse_tasks;
#ifdef CONFIG_MACF
	struct label *                  crash_label;
	uint8_t *                       mach_trap_filter_mask;                                                                    
	uint8_t *                       mach_kobj_filter_mask;                                                                       
#endif
	struct vm_shared_region         *shared_region;
#if __has_feature(ptrauth_calls)
	char                            *shared_region_id;                                                        
	bool                            shared_region_auth_remapped;                                              
#endif                                   
	volatile uint32_t t_flags;                                                                                                  
#define TF_NONE                 0
#define TF_64B_ADDR             0x00000001                                                              
#define TF_64B_DATA             0x00000002                                                                  
#define TF_CPUMON_WARNING       0x00000004                                                                                          
#define TF_WAKEMON_WARNING      0x00000008                                                                           
#define TF_TELEMETRY            (TF_CPUMON_WARNING | TF_WAKEMON_WARNING)                                      
#define TF_GPU_DENIED           0x00000010                                                                         
#define TF_CORPSE               0x00000020                                                    
#define TF_PENDING_CORPSE       0x00000040                                                                         
#define TF_CORPSE_FORK          0x00000080                                                           
#define TF_PLATFORM             0x00000400                                                             
#define TF_CA_CLIENT_WI         0x00000800                                                                    
#define TF_DARKWAKE_MODE        0x00001000                                                            
#define TF_NO_SMT               0x00002000                                                                                    
#define TF_FILTER_MSG           0x00004000                                                                                                    
#define TF_SYS_VERSION_COMPAT   0x00008000                                                                                                     
#define TF_PAC_EXC_FATAL        0x00010000                                                                                     
#define TF_TECS                 0x00020000                                                                         
#if defined(__x86_64__)
#define TF_INSN_COPY_OPTOUT     0x00040000                                                                                                         
#endif

  
                                                 
   
#define task_has_64Bit_addr(task)       \
	(((task)->t_flags & TF_64B_ADDR) != 0)
#define task_set_64Bit_addr(task)       \
	((task)->t_flags |= TF_64B_ADDR)
#define task_clear_64Bit_addr(task)     \
	((task)->t_flags &= ~TF_64B_ADDR)

  
                                      
   
#define task_has_64Bit_data(task)       \
	(((task)->t_flags & TF_64B_DATA) != 0)
#define task_set_64Bit_data(task)       \
	((task)->t_flags |= TF_64B_DATA)
#define task_clear_64Bit_data(task)     \
	((task)->t_flags &= ~TF_64B_DATA)

#define task_is_a_corpse(task)      \
	 (((task)->t_flags & TF_CORPSE) != 0)

#define task_set_corpse(task)       \
	 ((task)->t_flags |= TF_CORPSE)

#define task_corpse_pending_report(task)        \
	 (((task)->t_flags & TF_PENDING_CORPSE) != 0)

#define task_set_corpse_pending_report(task)       \
	 ((task)->t_flags |= TF_PENDING_CORPSE)

#define task_clear_corpse_pending_report(task)       \
	 ((task)->t_flags &= ~TF_PENDING_CORPSE)

#define task_is_a_corpse_fork(task)     \
	(((task)->t_flags & TF_CORPSE_FORK) != 0)

	uint32_t t_procflags;                                                                                                        
#define TPF_NONE                 0
#define TPF_DID_EXEC             0x00000001                                                                      
#define TPF_EXEC_COPY            0x00000002                                                                   
#ifdef CONFIG_32BIT_TELEMETRY
#define TPF_LOG_32BIT_TELEMETRY  0x00000004                                                                           
#endif

#define task_did_exec_internal(task)            \
	(((task)->t_procflags & TPF_DID_EXEC) != 0)

#define task_is_exec_copy_internal(task)        \
	(((task)->t_procflags & TPF_EXEC_COPY) != 0)

	uint8_t                  t_returnwaitflags;
#define TWF_NONE                 0
#define TRW_LRETURNWAIT          0x01                                                                      
#define TRW_LRETURNWAITER        0x02                                                                   

	mach_vm_address_t       all_image_info_addr;                                
	mach_vm_size_t          all_image_info_size;                                

#if KPC
#define TASK_KPC_FORCED_ALL_CTRS        0x2                                                                  
	uint32_t t_kpc;                
#endif          

	boolean_t pidsuspended;                                                 
	boolean_t frozen;                                                             
	boolean_t changing_freeze_state;                                                   
	uint16_t policy_ru_cpu          :4,
	    policy_ru_cpu_ext      :4,
	    applied_ru_cpu         :4,
	    applied_ru_cpu_ext     :4;
	uint8_t  rusage_cpu_flags;
	uint8_t  rusage_cpu_percentage;                                             
	uint8_t  rusage_cpu_perthr_percentage;                                       
#if MACH_ASSERT
	int8_t          suspends_outstanding;                                                          
#endif
	uint64_t rusage_cpu_interval;                                             
	uint64_t rusage_cpu_perthr_interval;                                       
	uint64_t rusage_cpu_deadline;
	thread_call_t rusage_cpu_callt;
#if CONFIG_TASKWATCH
	queue_head_t    task_watchers;                                         
	int     num_taskwatchers;
	int             watchapplying;
#endif                       

	struct bank_task *bank_context;                                          

#if IMPORTANCE_INHERITANCE
	struct ipc_importance_task  *task_imp_base;                                       
#endif                             

	vm_extmod_statistics_data_t     extmod_statistics;

	struct task_requested_policy requested_policy;
	struct task_effective_policy effective_policy;

	  
                                                                                       
    
	uint32_t        low_mem_notified_warn           :1,                                                              
	    low_mem_notified_critical       :1,                                                                           
	    purged_memory_warn              :1,                                                                                        
	    purged_memory_critical          :1,                                                                                         
	    low_mem_privileged_listener     :1,                                                                                                              
	    mem_notify_reserved             :27;                                             

	uint32_t memlimit_is_active                 :1,                                                                       
	    memlimit_is_fatal                   :1,                                                                          
	    memlimit_active_exc_resource        :1,                                                                                        
	    memlimit_inactive_exc_resource      :1,                                                                                          
	    memlimit_attrs_reserved             :28;                                  

	io_stat_info_t          task_io_stats;

	struct task_writes_counters task_writes_counters_internal;
	struct task_writes_counters task_writes_counters_external;

	  
                                                                
    
	struct _cpu_time_qos_stats      cpu_time_eqos_stats;
	struct _cpu_time_qos_stats      cpu_time_rqos_stats;

	                                                                  
	uint32_t        task_timer_wakeups_bin_1;
	uint32_t        task_timer_wakeups_bin_2;
	uint64_t        task_gpu_ns;
	uint64_t        task_energy;

#if MONOTONIC
	                                      
	struct mt_task task_monotonic;
#endif                

	uint8_t         task_can_transfer_memory_ownership;
	uint8_t         task_objects_disowning;
	uint8_t         task_objects_disowned;
	                                                            
	int             task_volatile_objects;
	                                                                    
	int             task_nonvolatile_objects;
	int             task_owned_objects;
	queue_head_t    task_objq;
	decl_lck_mtx_data(, task_objq_lock);                           

	unsigned int    task_thread_limit:16;
#if __arm64__
	unsigned int    task_legacy_footprint:1;
	unsigned int    task_extra_footprint_limit:1;
	unsigned int    task_ios13extended_footprint_limit:1;
#endif                
	unsigned int    task_region_footprint:1;
	unsigned int    task_has_crossed_thread_limit:1;
	uint32_t        exec_token;
	  
                                                               
                                                               
                                            
                                                        
                            
    
	coalition_t     coalition[COALITION_NUM_TYPES];
	queue_chain_t   task_coalition[COALITION_NUM_TYPES];
	uint64_t        dispatchqueue_offset;

#if DEVELOPMENT || DEBUG
	boolean_t       task_unnested;
	int             task_disconnected_count;
#endif

#if HYPERVISOR
	void * XNU_PTRAUTH_SIGNED_PTR("task.hv_task_target") hv_task_target;                                                                  
#endif                 

#if CONFIG_SECLUDED_MEMORY
	uint8_t task_can_use_secluded_mem;
	uint8_t task_could_use_secluded_mem;
	uint8_t task_could_also_use_secluded_mem;
	uint8_t task_suppressed_secluded;
#endif                             

	task_exc_guard_behavior_t task_exc_guard;

	queue_head_t    io_user_clients;

	mach_vm_address_t mach_header_vm_address;

	uint32_t loadTag;                                        
#if CONFIG_FREEZE
	queue_head_t   task_frozen_cseg_q;                                     
#endif                    
#if CONFIG_PHYS_WRITE_ACCT
	uint64_t        task_fs_metadata_writes;
#endif                             
	uint32_t task_shared_region_slide;                                                      
	uuid_t   task_shared_region_uuid;
};

  
                                                                            
                                         
   
extern task_exc_guard_behavior_t task_exc_guard_default;

extern kern_return_t
    task_violated_guard(mach_exception_code_t, mach_exception_subcode_t, void *);

static inline void
task_require(struct task *task)
{
	zone_id_require(ZONE_ID_TASK, sizeof(struct task), task);
}

#define task_lock(task)                 lck_mtx_lock(&(task)->lock)
#define task_lock_assert_owned(task)    LCK_MTX_ASSERT(&(task)->lock, LCK_MTX_ASSERT_OWNED)
#define task_lock_try(task)             lck_mtx_try_lock(&(task)->lock)
#define task_unlock(task)               lck_mtx_unlock(&(task)->lock)

#define task_objq_lock_init(task)       lck_mtx_init(&(task)->task_objq_lock, &vm_object_lck_grp, &vm_object_lck_attr)
#define task_objq_lock_destroy(task)    lck_mtx_destroy(&(task)->task_objq_lock, &vm_object_lck_grp)
#define task_objq_lock(task)            lck_mtx_lock(&(task)->task_objq_lock)
#define task_objq_lock_assert_owned(task)       LCK_MTX_ASSERT(&(task)->task_objq_lock, LCK_MTX_ASSERT_OWNED)
#define task_objq_lock_try(task)        lck_mtx_try_lock(&(task)->task_objq_lock)
#define task_objq_unlock(task)          lck_mtx_unlock(&(task)->task_objq_lock)

#define itk_lock_init(task)     lck_mtx_init(&(task)->itk_lock_data, &ipc_lck_grp, &ipc_lck_attr)
#define itk_lock_destroy(task)  lck_mtx_destroy(&(task)->itk_lock_data, &ipc_lck_grp)
#define itk_lock(task)          lck_mtx_lock(&(task)->itk_lock_data)
#define itk_unlock(task)        lck_mtx_unlock(&(task)->itk_lock_data)

                                  
#define TCRW_CLEAR_INITIAL_WAIT   0x1
#define TCRW_CLEAR_FINAL_WAIT     0x2
#define TCRW_CLEAR_ALL_WAIT       (TCRW_CLEAR_INITIAL_WAIT | TCRW_CLEAR_FINAL_WAIT)

#define TASK_REFERENCE_LEAK_DEBUG 0

#if TASK_REFERENCE_LEAK_DEBUG
extern void task_reference_internal(task_t task);
extern os_ref_count_t task_deallocate_internal(task_t task);
#else
#define task_reference_internal(task) \
MACRO_BEGIN                           \
	task_require(task);               \
	os_ref_retain(&(task)->ref_count); \
MACRO_END
#define task_deallocate_internal(task) os_ref_release(&(task)->ref_count)
#endif

#define task_reference(task)                                    \
MACRO_BEGIN                                                                             \
	if ((task) != TASK_NULL)                                        \
	        task_reference_internal(task);                  \
MACRO_END

extern kern_return_t    kernel_task_create(
	task_t                  task,
	vm_offset_t             map_base,
	vm_size_t               map_size,
	task_t                  *child);

                            
extern void             task_init(void);

                                                                          
extern void             init_task_ledgers(void);

#define current_task_fast()     (current_thread()->task)
#define current_task()          current_task_fast()

extern bool task_is_driver(task_t task);

extern lck_attr_t      task_lck_attr;
extern lck_grp_t       task_lck_grp;

struct task_watchport_elem {
	task_t                          twe_task;
	ipc_port_t                      twe_port;                       
};

struct task_watchports {
	os_refcnt_t                     tw_refcount;                             
	task_t                          tw_task;                                                    
	thread_t                        tw_thread;                                                  
	uint32_t                        tw_elem_array_count;                     
	struct task_watchport_elem      tw_elem[];                                                        
};

#define task_watchports_retain(x)   (os_ref_retain(&(x)->tw_refcount))
#define task_watchports_release(x)  (os_ref_release(&(x)->tw_refcount))

#define task_watchport_elem_init(elem, task, port) \
do {                                               \
	(elem)->twe_task = (task);                 \
	(elem)->twe_port = (port);                 \
} while(0)

#define task_watchport_elem_clear(elem) task_watchport_elem_init((elem), NULL, NULL)

extern void
task_add_turnstile_watchports(
	task_t          task,
	thread_t        thread,
	ipc_port_t      *portwatch_ports,
	uint32_t        portwatch_count);

extern void
task_watchport_elem_deallocate(
	struct          task_watchport_elem *watchport_elem);

extern boolean_t
task_has_watchports(task_t task);

void
task_dyld_process_info_update_helper(
	task_t                  task,
	size_t                  active_count,
	vm_map_address_t        magic_addr,
	ipc_port_t             *release_ports,
	size_t                  release_count);

#else                            

__BEGIN_DECLS

extern task_t   current_task(void);

extern void             task_reference(task_t   task);
extern bool task_is_driver(task_t task);

#define TF_NONE                 0

#define TWF_NONE                 0
#define TRW_LRETURNWAIT          0x01                                                                      
#define TRW_LRETURNWAITER        0x02                                                                   

                                  
#define TCRW_CLEAR_INITIAL_WAIT   0x1
#define TCRW_CLEAR_FINAL_WAIT     0x2
#define TCRW_CLEAR_ALL_WAIT       (TCRW_CLEAR_INITIAL_WAIT | TCRW_CLEAR_FINAL_WAIT)


#define TPF_NONE                0
#define TPF_EXEC_COPY           0x00000002                                                                   


__END_DECLS

#endif                           

__BEGIN_DECLS

#ifdef KERNEL_PRIVATE
extern boolean_t                task_is_app_suspended(task_t task);
extern bool task_is_exotic(task_t task);
extern bool task_is_alien(task_t task);
#endif

#ifdef  XNU_KERNEL_PRIVATE

                                
extern kern_return_t    task_hold(
	task_t          task);

                                                                                          
extern kern_return_t    task_wait(
	task_t          task,
	boolean_t       until_not_runnable);

                                           
extern kern_return_t    task_release(
	task_t          task);

                                                                   
extern kern_return_t    task_suspend_internal(          task_t          task);
extern kern_return_t    task_resume_internal(           task_t          task);

                                                      
extern kern_return_t    task_pidsuspend(
	task_t          task);

                                      
extern kern_return_t    task_pidresume(
	task_t          task);

extern kern_return_t    task_send_trace_memory(
	task_t          task,
	uint32_t        pid,
	uint64_t        uniqueid);

extern void             task_remove_turnstile_watchports(
	task_t          task);

extern void             task_transfer_turnstile_watchports(
	task_t          old_task,
	task_t          new_task,
	thread_t        new_thread);

#if DEVELOPMENT || DEBUG

extern kern_return_t    task_disconnect_page_mappings(
	task_t          task);
#endif

extern void                     tasks_system_suspend(boolean_t suspend);

#if CONFIG_FREEZE

                                    
extern kern_return_t    task_freeze(
	task_t          task,
	uint32_t        *purgeable_count,
	uint32_t        *wired_count,
	uint32_t        *clean_count,
	uint32_t        *dirty_count,
	uint32_t        dirty_budget,
	uint32_t        *shared_count,
	int             *freezer_error_code,
	boolean_t       eval_only);

                                  
extern kern_return_t    task_thaw(
	task_t          task);

typedef enum {
	CREDIT_TO_SWAP = 1,
	DEBIT_FROM_SWAP = 2
} freezer_acct_op_t;

extern void task_update_frozen_to_swap_acct(
	task_t  task,
	int64_t amount,
	freezer_acct_op_t op);

#endif                    

                                                
extern kern_return_t    task_start_halt(
	task_t          task);

                                                                    
extern void             task_complete_halt(
	task_t          task);

extern kern_return_t    task_terminate_internal(
	task_t                  task);

extern kern_return_t    task_create_internal(
	task_t          parent_task,
	coalition_t     *parent_coalitions,
	boolean_t       inherit_memory,
	boolean_t       is_64bit,
	boolean_t       is_64bit_data,
	uint32_t        flags,
	uint32_t        procflags,
	uint8_t         t_returnwaitflags,
	task_t          *child_task);                                                            

extern kern_return_t    task_set_special_port_internal(
	task_t                  task,
	int                     which,
	ipc_port_t              port);

extern kern_return_t    task_info(
	task_t                  task,
	task_flavor_t           flavor,
	task_info_t             task_info_out,
	mach_msg_type_number_t  *task_info_count);

extern void             task_power_info_locked(
	task_t                  task,
	task_power_info_t       info,
	gpu_energy_data_t       gpu_energy,
	task_power_info_v2_t    infov2,
	uint64_t                *runnable_time);

extern uint64_t         task_gpu_utilisation(
	task_t   task);

extern uint64_t         task_energy(
	task_t   task);

extern uint64_t         task_cpu_ptime(
	task_t   task);
extern void             task_update_cpu_time_qos_stats(
	task_t   task,
	uint64_t *eqos_stats,
	uint64_t *rqos_stats);

extern void             task_vtimer_set(
	task_t          task,
	integer_t       which);

extern void             task_vtimer_clear(
	task_t          task,
	integer_t       which);

extern void             task_vtimer_update(
	task_t          task,
	integer_t       which,
	uint32_t        *microsecs);

#define TASK_VTIMER_USER                0x01
#define TASK_VTIMER_PROF                0x02
#define TASK_VTIMER_RLIM                0x04

extern void             task_set_64bit(
	task_t          task,
	boolean_t       is_64bit,
	boolean_t       is_64bit_data);

extern boolean_t        task_get_64bit_data(
	task_t task);

extern void     task_set_platform_binary(
	task_t task,
	boolean_t is_platform);
extern bool     task_set_ca_client_wi(
	task_t task,
	boolean_t ca_client_wi);

extern void             task_set_dyld_info(
	task_t          task,
	mach_vm_address_t addr,
	mach_vm_size_t size);

extern void task_set_mach_header_address(
	task_t task,
	mach_vm_address_t addr);

                                         
extern int              get_task_numacts(
	task_t          task);

struct label;
extern kern_return_t task_collect_crash_info(
	task_t task,
#if CONFIG_MACF
	struct label *crash_label,
#endif
	int is_corpse_fork);
void task_port_notify(mach_msg_header_t *msg);
void task_port_with_flavor_notify(mach_msg_header_t *msg);
void task_wait_till_threads_terminate_locked(task_t task);

                                                                       
extern void     set_bsdtask_info(task_t, void *);
extern uint32_t set_task_loadTag(task_t task, uint32_t loadTag);
extern vm_map_t get_task_map_reference(task_t);
extern vm_map_t swap_task_map(task_t, thread_t, vm_map_t);
extern pmap_t   get_task_pmap(task_t);
extern uint64_t get_task_resident_size(task_t);
extern uint64_t get_task_compressed(task_t);
extern uint64_t get_task_resident_max(task_t);
extern uint64_t get_task_phys_footprint(task_t);
#if CONFIG_LEDGER_INTERVAL_MAX
extern uint64_t get_task_phys_footprint_interval_max(task_t, int reset);
#endif                                    
extern uint64_t get_task_phys_footprint_lifetime_max(task_t);
extern uint64_t get_task_phys_footprint_limit(task_t);
extern uint64_t get_task_purgeable_size(task_t);
extern uint64_t get_task_cpu_time(task_t);
extern uint64_t get_task_dispatchqueue_offset(task_t);
extern uint64_t get_task_dispatchqueue_serialno_offset(task_t);
extern uint64_t get_task_dispatchqueue_label_offset(task_t);
extern uint64_t get_task_uniqueid(task_t task);
extern int      get_task_version(task_t task);

extern uint64_t get_task_internal(task_t);
extern uint64_t get_task_internal_compressed(task_t);
extern uint64_t get_task_purgeable_nonvolatile(task_t);
extern uint64_t get_task_purgeable_nonvolatile_compressed(task_t);
extern uint64_t get_task_iokit_mapped(task_t);
extern uint64_t get_task_alternate_accounting(task_t);
extern uint64_t get_task_alternate_accounting_compressed(task_t);
extern uint64_t get_task_memory_region_count(task_t);
extern uint64_t get_task_page_table(task_t);
#if CONFIG_FREEZE
extern uint64_t get_task_frozen_to_swap(task_t);
#endif
extern uint64_t get_task_network_nonvolatile(task_t);
extern uint64_t get_task_network_nonvolatile_compressed(task_t);
extern uint64_t get_task_wired_mem(task_t);
extern uint32_t get_task_loadTag(task_t task);

extern uint64_t get_task_tagged_footprint(task_t task);
extern uint64_t get_task_tagged_footprint_compressed(task_t task);
extern uint64_t get_task_media_footprint(task_t task);
extern uint64_t get_task_media_footprint_compressed(task_t task);
extern uint64_t get_task_graphics_footprint(task_t task);
extern uint64_t get_task_graphics_footprint_compressed(task_t task);
extern uint64_t get_task_neural_footprint(task_t task);
extern uint64_t get_task_neural_footprint_compressed(task_t task);

extern kern_return_t task_convert_phys_footprint_limit(int, int *);
extern kern_return_t task_set_phys_footprint_limit_internal(task_t, int, int *, boolean_t, boolean_t);
extern kern_return_t task_get_phys_footprint_limit(task_t task, int *limit_mb);

                                
extern boolean_t task_get_memlimit_is_active(task_t task);
extern boolean_t task_get_memlimit_is_fatal(task_t task);
extern void task_set_memlimit_is_active(task_t task, boolean_t memlimit_is_active);
extern void task_set_memlimit_is_fatal(task_t task, boolean_t memlimit_is_fatal);
extern boolean_t task_has_triggered_exc_resource(task_t task, boolean_t memlimit_is_active);
extern void task_mark_has_triggered_exc_resource(task_t task, boolean_t memlimit_is_active);

extern void task_set_thread_limit(task_t task, uint16_t thread_limit);

#if XNU_TARGET_OS_OSX
extern boolean_t task_has_system_version_compat_enabled(task_t task);
extern void task_set_system_version_compat_enabled(task_t task, boolean_t enable_system_version_compat);
#endif

extern boolean_t        is_kerneltask(task_t task);
extern boolean_t        is_corpsetask(task_t task);

extern kern_return_t check_actforsig(task_t task, thread_t thread, int setast);

extern kern_return_t machine_task_get_state(
	task_t task,
	int flavor,
	thread_state_t state,
	mach_msg_type_number_t *state_count);

extern kern_return_t machine_task_set_state(
	task_t task,
	int flavor,
	thread_state_t state,
	mach_msg_type_number_t state_count);

extern void machine_task_terminate(task_t task);

struct _task_ledger_indices {
	int cpu_time;
	int tkm_private;
	int tkm_shared;
	int phys_mem;
	int wired_mem;
	int internal;
	int iokit_mapped;
	int alternate_accounting;
	int alternate_accounting_compressed;
	int page_table;
	int phys_footprint;
	int internal_compressed;
	int purgeable_volatile;
	int purgeable_nonvolatile;
	int purgeable_volatile_compressed;
	int purgeable_nonvolatile_compressed;
	int tagged_nofootprint;
	int tagged_footprint;
	int tagged_nofootprint_compressed;
	int tagged_footprint_compressed;
	int network_volatile;
	int network_nonvolatile;
	int network_volatile_compressed;
	int network_nonvolatile_compressed;
	int media_nofootprint;
	int media_footprint;
	int media_nofootprint_compressed;
	int media_footprint_compressed;
	int graphics_nofootprint;
	int graphics_footprint;
	int graphics_nofootprint_compressed;
	int graphics_footprint_compressed;
	int neural_nofootprint;
	int neural_footprint;
	int neural_nofootprint_compressed;
	int neural_footprint_compressed;
	int platform_idle_wakeups;
	int interrupt_wakeups;
#if CONFIG_SCHED_SFI
	int sfi_wait_times[MAX_SFI_CLASS_ID];
#endif                       
	int cpu_time_billed_to_me;
	int cpu_time_billed_to_others;
	int physical_writes;
	int logical_writes;
	int logical_writes_to_external;
	int energy_billed_to_me;
	int energy_billed_to_others;
#if DEBUG || DEVELOPMENT
	int pages_grabbed;
	int pages_grabbed_kern;
	int pages_grabbed_iopl;
	int pages_grabbed_upl;
#endif
#if CONFIG_FREEZE
	int frozen_to_swap;
#endif                    
#if CONFIG_PHYS_WRITE_ACCT
	int fs_metadata_writes;
#endif                             
};
extern struct _task_ledger_indices task_ledgers;

                                                               
thread_t task_findtid(task_t task, uint64_t tid);
int pid_from_task(task_t task);

extern kern_return_t task_wakeups_monitor_ctl(task_t task, uint32_t *rate_hz, int32_t *flags);
extern kern_return_t task_cpu_usage_monitor_ctl(task_t task, uint32_t *flags);
extern void task_rollup_accounting_info(task_t new_task, task_t parent_task);
extern kern_return_t task_io_monitor_ctl(task_t task, uint32_t *flags);
extern void task_set_did_exec_flag(task_t task);
extern void task_clear_exec_copy_flag(task_t task);
extern boolean_t task_is_exec_copy(task_t);
extern boolean_t task_did_exec(task_t task);
#ifdef CONFIG_32BIT_TELEMETRY
extern boolean_t task_consume_32bit_log_flag(task_t task);
extern void task_set_32bit_log_flag(task_t task);
#endif                             
extern boolean_t task_is_active(task_t task);
extern boolean_t task_is_halting(task_t task);
extern void task_clear_return_wait(task_t task, uint32_t flags);
extern void task_wait_to_return(void) __attribute__((noreturn));
extern event_t task_get_return_wait_event(task_t task);

extern void task_bank_reset(task_t task);
extern void task_bank_init(task_t task);

#if CONFIG_ARCADE
extern void task_prep_arcade(task_t task, thread_t thread);
#endif                    

extern int task_pid(task_t task);

#if __has_feature(ptrauth_calls)
char *task_get_vm_shared_region_id_and_jop_pid(task_t task, uint64_t *);
void task_set_shared_region_id(task_t task, char *id);
#endif                                   

extern boolean_t task_has_assertions(task_t task);
                     

extern void      task_set_gpu_denied(task_t task, boolean_t denied);
extern boolean_t task_is_gpu_denied(task_t task);

extern queue_head_t * task_io_user_clients(task_t task);
extern void     task_set_message_app_suspended(task_t task, boolean_t enable);

extern void task_copy_fields_for_exec(task_t dst_task, task_t src_task);

extern void task_copy_vmobjects(task_t task, vm_object_query_t query, size_t len, size_t *num);

extern void task_set_filter_msg_flag(task_t task, boolean_t flag);
extern boolean_t task_get_filter_msg_flag(task_t task);

extern void task_transfer_mach_filter_bits(task_t new_task, task_t old_mask);

#if __has_feature(ptrauth_calls)
extern bool task_is_pac_exception_fatal(task_t task);
extern void task_set_pac_exception_fatal_flag(task_t task);
#endif                                 

extern void task_set_tecs(task_t task);

#endif                          

#ifdef  KERNEL_PRIVATE

extern void     *get_bsdtask_info(task_t);
extern void     *get_bsdthreadtask_info(thread_t);
extern void task_bsdtask_kill(task_t);
extern vm_map_t get_task_map(task_t);
extern ledger_t get_task_ledger(task_t);

extern boolean_t get_task_pidsuspended(task_t);
extern boolean_t get_task_suspended(task_t);
extern boolean_t get_task_frozen(task_t);

                                   
extern ipc_port_t convert_task_to_port(task_t);
extern ipc_port_t convert_task_to_port_pinned(task_t);
extern ipc_port_t convert_task_name_to_port(task_name_t);
extern ipc_port_t convert_task_inspect_to_port(task_inspect_t);
extern ipc_port_t convert_task_read_to_port(task_read_t);
extern ipc_port_t convert_task_suspension_token_to_port(task_suspension_token_t task);

                                                                                        
extern task_suspension_token_t convert_port_to_task_suspension_token(ipc_port_t port);

extern boolean_t task_suspension_notify(mach_msg_header_t *);

#define TASK_WRITE_IMMEDIATE                 0x1
#define TASK_WRITE_DEFERRED                  0x2
#define TASK_WRITE_INVALIDATED               0x4
#define TASK_WRITE_METADATA                  0x8
extern void     task_update_logical_writes(task_t task, uint32_t io_size, int flags, void *vp);

__enum_decl(task_balance_flags_t, uint8_t, {
	TASK_BALANCE_CREDIT                 = 0x1,
	TASK_BALANCE_DEBIT                  = 0x2,
});

__enum_decl(task_physical_write_flavor_t, uint8_t, {
	TASK_PHYSICAL_WRITE_METADATA        = 0x1,
});
extern void     task_update_physical_writes(task_t task, task_physical_write_flavor_t flavor,
    uint64_t io_size, task_balance_flags_t flags);

#if CONFIG_SECLUDED_MEMORY
extern void task_set_can_use_secluded_mem(
	task_t task,
	boolean_t can_use_secluded_mem);
extern void task_set_could_use_secluded_mem(
	task_t task,
	boolean_t could_use_secluded_mem);
extern void task_set_could_also_use_secluded_mem(
	task_t task,
	boolean_t could_also_use_secluded_mem);
extern boolean_t task_can_use_secluded_mem(
	task_t task,
	boolean_t is_allocate);
extern boolean_t task_could_use_secluded_mem(task_t task);
extern boolean_t task_could_also_use_secluded_mem(task_t task);
#endif                             

extern void task_set_darkwake_mode(task_t, boolean_t);
extern boolean_t task_get_darkwake_mode(task_t);

#if __arm64__
extern void task_set_legacy_footprint(task_t task);
extern void task_set_extra_footprint_limit(task_t task);
extern void task_set_ios13extended_footprint_limit(task_t task);
#endif                

#if CONFIG_MACF
extern struct label *get_task_crash_label(task_t task);
#endif                  

extern int get_task_cdhash(task_t task, char cdhash[]);

#endif                      

extern task_t   kernel_task;

extern void             task_deallocate(
	task_t          task);

extern void             task_name_deallocate(
	task_name_t             task_name);

extern void             task_policy_set_deallocate(
	task_policy_set_t       task_policy_set);

extern void             task_policy_get_deallocate(
	task_policy_get_t       task_policy_get);

extern void             task_inspect_deallocate(
	task_inspect_t          task_inspect);

extern void             task_read_deallocate(
	task_read_t          task_read);

extern void             task_suspension_token_deallocate(
	task_suspension_token_t token);

extern boolean_t task_self_region_footprint(void);
extern void task_self_region_footprint_set(boolean_t newval);
extern void task_ledgers_footprint(ledger_t ledger,
    ledger_amount_t *ledger_resident,
    ledger_amount_t *ledger_compressed);
extern void task_set_memory_ownership_transfer(
	task_t task,
	boolean_t value);


__END_DECLS

#endif                     
