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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_FREE_COPYRIGHT@
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

#ifndef _KERN_THREAD_H_
#define _KERN_THREAD_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/boolean.h>
#include <mach/vm_param.h>
#include <mach/thread_info.h>
#include <mach/thread_status.h>
#include <mach/exception_types.h>

#include <kern/kern_types.h>
#include <vm/vm_kern.h>

#include <sys/cdefs.h>

#ifdef XNU_KERNEL_PRIVATE
                                           
__options_closed_decl(thread_tag_t, uint16_t, {
	THREAD_TAG_MAINTHREAD   = 0x01,
	THREAD_TAG_CALLOUT      = 0x02,
	THREAD_TAG_IOWORKLOOP   = 0x04,
	THREAD_TAG_PTHREAD      = 0x10,
	THREAD_TAG_WORKQUEUE    = 0x20,
});
#endif                         

#ifdef  MACH_KERNEL_PRIVATE

#include <mach_assert.h>
#include <mach_ldebug.h>

#include <ipc/ipc_types.h>

#include <mach/port.h>
#include <kern/cpu_number.h>
#include <kern/smp.h>
#include <kern/queue.h>

#include <kern/timer.h>
#include <kern/simple_lock.h>
#include <kern/locks.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <mach/sfi_class.h>
#include <kern/thread_call.h>
#include <kern/thread_group.h>
#include <kern/timer_call.h>
#include <kern/task.h>
#include <kern/exception.h>
#include <kern/affinity.h>
#include <kern/debug.h>
#include <kern/block_hint.h>
#include <kern/turnstile.h>
#include <kern/mpsc_queue.h>

#include <kern/waitq.h>
#include <san/kasan.h>
#include <os/refcnt.h>

#include <ipc/ipc_kmsg.h>

#include <machine/atomic.h>
#include <machine/cpu_data.h>
#include <machine/thread.h>

#ifdef XNU_KERNEL_PRIVATE
                                                                            
#include <kern/priority_queue.h>
#endif                         

#if MONOTONIC
#include <stdatomic.h>
#include <machine/monotonic.h>
#endif                

#if CONFIG_TASKWATCH
                                                      
typedef struct task_watcher task_watch_t;
#endif                       

__options_decl(thread_work_interval_flags_t, uint32_t, {
	TH_WORK_INTERVAL_FLAGS_NONE           = 0x0,
#if CONFIG_SCHED_AUTO_JOIN
	                                                                              
	TH_WORK_INTERVAL_FLAGS_AUTO_JOIN_LEAK = 0x1,
#endif                             
});

struct thread {
#if MACH_ASSERT
#define THREAD_MAGIC 0x1234ABCDDCBA4321ULL
	                                                  
	uint64_t                thread_magic;
#endif                  

	  
                                                               
                                                                 
                                                                 
                                                             
                                                                  
                                                           
                                                 
   
                                                            
                                     
    
	union {
		queue_chain_t                   runq_links;                                  
		queue_chain_t                   wait_links;                                   
		struct mpsc_queue_chain         mpsc_links;                                           
		struct priority_queue_entry_sched wait_prioq_links;                                         
	};

	event64_t               wait_event;                           
	processor_t             runq;                                     
	struct waitq           *waitq;                                                     
	struct turnstile       *turnstile;                                                                 
	void                   *inheritor;                                                               
	struct priority_queue_sched_max sched_inheritor_queue;                                           
	struct priority_queue_sched_max base_inheritor_queue;                                         

#if CONFIG_SCHED_EDGE
	boolean_t               th_bound_cluster_enqueued;
#endif                        

#if CONFIG_SCHED_CLUTCH
	  
                                                                                     
                                                                                       
                                                                   
   
                                                                                                           
                                                                                                                                 
                                                                                      
    
	struct priority_queue_entry_stable      th_clutch_runq_link;
	struct priority_queue_entry_sched       th_clutch_pri_link;
	queue_chain_t                           th_clutch_timeshare_link;
#endif                          

	                                                   
	decl_simple_lock_data(, sched_lock);                                          
	decl_simple_lock_data(, wake_lock);                                                
	uint16_t                options;                                          
#define TH_OPT_INTMASK          0x0003                                       
#define TH_OPT_VMPRIV           0x0004                                            
#define TH_OPT_SYSTEM_CRITICAL  0x0010                                                                            
#define TH_OPT_PROC_CPULIMIT    0x0020                                                              
#define TH_OPT_PRVT_CPULIMIT    0x0040                                                                   
#define TH_OPT_IDLE_THREAD      0x0080                                                     
#define TH_OPT_GLOBAL_FORCED_IDLE       0x0100                                                       
#define TH_OPT_SCHED_VM_GROUP   0x0200                                                            
#define TH_OPT_HONOR_QLIMIT     0x0400                                                                                               
#define TH_OPT_SEND_IMPORTANCE  0x0800                                                                     
#define TH_OPT_ZONE_PRIV        0x1000                                                         
#define TH_OPT_IPC_TG_BLOCKED   0x2000                                                                                        

	bool                    wake_active;                            
	bool                    at_safe_point;                                   
	uint8_t                 sched_saved_run_weight;
	ast_t                   reason;                             
	uint32_t                quantum_remaining;
	wait_result_t           wait_result;                        
                                                                         
                                                             
	thread_continue_t       continuation;                                    
	void                   *parameter;                                  

	                                        
	vm_offset_t             kernel_stack;                             
	vm_offset_t             reserved_stack;                            

	                                 
	struct machine_thread   machine;

#if KASAN
	struct kasan_thread_data kasan_data;
#endif
#if CONFIG_KSANCOV
	void                   *ksancov_data;
#endif

	                   
	int                     state;
  
                             
   
#define TH_WAIT                 0x01                                    
#define TH_SUSP                 0x02                                              
#define TH_RUN                  0x04                                    
#define TH_UNINT                0x08                                        
#define TH_TERMINATE            0x10                                       
#define TH_TERMINATE2           0x20                                            
#define TH_WAIT_REPORT          0x40                                                
                                                                             
#define TH_IDLE                 0x80                                  

	                            
	sched_mode_t            sched_mode;                          
	sched_mode_t            saved_mode;                                                 

	                                                         
	sched_bucket_t          th_sched_bucket;

	sfi_class_id_t          sfi_class;                                                 
	sfi_class_id_t          sfi_wait_class;                                                                  

	uint32_t                sched_flags;                                   
#define TH_SFLAG_NO_SMT                 0x0001                                                                  
#define TH_SFLAG_FAILSAFE               0x0002                                     
#define TH_SFLAG_THROTTLED              0x0004                                                                                                  
#define TH_SFLAG_DEMOTED_MASK      (TH_SFLAG_THROTTLED | TH_SFLAG_FAILSAFE)                                                  

#define TH_SFLAG_PROMOTED               0x0008                                                                              
#define TH_SFLAG_ABORT                  0x0010                                         
#define TH_SFLAG_ABORTSAFELY            0x0020                                                
#define TH_SFLAG_ABORTED_MASK           (TH_SFLAG_ABORT | TH_SFLAG_ABORTSAFELY)
#define TH_SFLAG_DEPRESS                0x0040                                    
#define TH_SFLAG_POLLDEPRESS            0x0080                                    
#define TH_SFLAG_DEPRESSED_MASK         (TH_SFLAG_DEPRESS | TH_SFLAG_POLLDEPRESS)
                                                 
#define TH_SFLAG_EAGERPREEMPT           0x0200                                                                                        
#define TH_SFLAG_RW_PROMOTED            0x0400                                                          
#define TH_SFLAG_BASE_PRI_FROZEN        0x0800                                              
#define TH_SFLAG_WAITQ_PROMOTED         0x1000                                                                        

#if __AMP__
#define TH_SFLAG_ECORE_ONLY             0x2000                                                   
#define TH_SFLAG_PCORE_ONLY             0x4000                                                   
#endif

#define TH_SFLAG_EXEC_PROMOTED          0x8000                                                    

#define TH_SFLAG_THREAD_GROUP_AUTO_JOIN 0x10000                                                          
#if __AMP__
#define TH_SFLAG_BOUND_SOFT             0x20000                                                                                               
#endif              
                                                                                 
#define TH_SFLAG_PROMOTE_REASON_MASK    (TH_SFLAG_RW_PROMOTED | TH_SFLAG_WAITQ_PROMOTED | TH_SFLAG_EXEC_PROMOTED)

#define TH_SFLAG_RW_PROMOTED_BIT        (10)               

	int16_t                 sched_pri;                                                
	int16_t                 base_pri;                                                                                                    
	int16_t                 req_base_pri;                                        
	int16_t                 max_priority;                                          
	int16_t                 task_priority;                                          
	int16_t                 promotion_priority;                                                   

	int                     iotier_override;                                                              
	os_refcnt_t             ref_count;                                              

	lck_mtx_t*              waiting_for_mutex;                                                                 

	uint32_t                rwlock_count;                                                        
	uint32_t                t_temp_alloc_count;                                          
#if DEBUG || DEVELOPMENT
	queue_head_t            t_temp_alloc_list;
#endif                           

	integer_t               importance;                                           

	                                    
	integer_t               depress_timer_active;
	timer_call_data_t       depress_timer;

	                          
	struct {                                                                      
		uint32_t            period;
		uint32_t            computation;
		uint32_t            constraint;
		boolean_t           preemptible;
		uint64_t            deadline;
	}                       realtime;

	uint64_t                last_run_time;                                                       
	uint64_t                last_made_runnable_time;                                                         
	uint64_t                last_basepri_change_time;                                                                        
	uint64_t                same_pri_latency;
#define THREAD_NOT_RUNNABLE (~0ULL)

#if CONFIG_THREAD_GROUPS
	struct thread_group     *thread_group;
#endif

#if defined(CONFIG_SCHED_MULTIQ)
	sched_group_t           sched_group;
#endif                                   

	                                      
	timer_data_t            system_timer;                                  
	processor_t             bound_processor;                                   
	processor_t             last_processor;                                           
	processor_t             chosen_processor;                                             

	                                                                  
	uint64_t                computation_metered;
	uint64_t                computation_epoch;
	uint64_t                safe_release;                                          

	                             
	void                  (*sched_call)(int type, thread_t thread);

#if defined(CONFIG_SCHED_PROTO)
	uint32_t                runqueue_generation;                                        
#endif

	                                             
#if defined(CONFIG_SCHED_TIMESHARE_CORE)
	natural_t               sched_stamp;                                     
	natural_t               sched_usage;                                               
	natural_t               pri_shift;                                               
	natural_t               cpu_usage;                                                 
	natural_t               cpu_delta;                                               
#endif                                  

	uint32_t                c_switch;                                           
	uint32_t                p_switch;                                             
	uint32_t                ps_switch;                                       

	integer_t mutex_count;                                 
	                            
	int                     precise_user_kernel_time;                                                  
	timer_data_t            user_timer;                                  
	uint64_t                user_timer_save;                                    
	uint64_t                system_timer_save;                                    
	uint64_t                vtimer_user_save;                                     
	uint64_t                vtimer_prof_save;
	uint64_t                vtimer_rlim_save;
	uint64_t                vtimer_qos_save;

	timer_data_t            ptime;                                                
	timer_data_t            runnable_timer;                                                              

#if CONFIG_SCHED_SFI
	                           
	uint64_t                wait_sfi_begin_time;                                              
#endif

	  
                            
                                                                    
    
	queue_chain_t           affinity_threads;
	affinity_set_t          affinity_set;

#if CONFIG_TASKWATCH
	task_watch_t           *taskwatch;                              
#endif                       

	                                                                                                       
	union {
		struct {
			mach_msg_return_t       state;                             
			mach_port_seqno_t       seqno;                                      
			ipc_object_t            object;                                 
			vm_address_t            msg_addr;                                   
			mach_msg_size_t         rsize;                                      
			mach_msg_size_t         msize;                                         
			mach_msg_option_t       option;                                  
			mach_port_name_t        receiver_name;                             
			struct knote            *knote;                                  
			union {
				struct ipc_kmsg   *kmsg;                              
				struct ipc_mqueue *peekq;                              
				struct {
					uint32_t       ppri;                                             
					mach_msg_qos_t oqos;                                  
				} received_qos;
			};
			mach_msg_continue_t     continuation;
		} receive;
		struct {
			struct semaphore        *waitsemaphore;                            
			struct semaphore        *signalsemaphore;                          
			int                     options;                                       
			kern_return_t           result;                                     
			mach_msg_continue_t continuation;
		} sema;
		struct {
#define THREAD_SAVE_IOKIT_TLS_COUNT     8
			void                    *tls[THREAD_SAVE_IOKIT_TLS_COUNT];
		} iokit;
	} saved;

	                                                                                                  
	union {
		                                                               
		struct thread_call_thread_state *thc_state;

		                                                         
		struct {
			mach_exception_code_t           code;
			mach_exception_subcode_t        subcode;
		} guard_exc_info;
	};

	                                  
	int16_t                 suspend_count;
	                            
	int16_t                 user_stop_count;

	                         
#if IMPORTANCE_INHERITANCE
	natural_t ith_assertions;                                                    
#endif
	struct ipc_kmsg_queue ith_messages;                                   
	mach_port_t ith_rpc_reply;                                                      

	                           
	ast_t                   ast;

	                              
	vm_offset_t             recover;                                                    

	queue_chain_t           threads;                                                

	                
	queue_chain_t           task_threads;

	                     
	struct task             *task;
	vm_map_t                map;
	thread_t                handoff_thread;
#if DEVELOPMENT || DEBUG
	bool      pmap_footprint_suspended;
#endif                           

	                           
	timer_call_data_t       wait_timer;
	uint16_t                wait_timer_active;
	bool                    wait_timer_is_set;

	                                         
	uint32_t
	    active:1,                                                             
	    ipc_active:1,                                                 
	    started:1,                                                      
	    static_param:1,                                            
	    inspection:1,                                                                
	    policy_reset:1,                                                                   
	    suspend_parked:1,                                          
	    corpse_dup:1,                                                                  
	:0;

	decl_lck_mtx_data(, mutex);

	  
                                     
                                                             
    
	struct ipc_port         *ith_thread_ports[THREAD_SELF_PORT_COUNT];                                 
	struct ipc_port         *ith_settable_self;                          
	struct ipc_port         *ith_self;                                                   
	struct ipc_port         *ith_special_reply_port;                                  
	struct exception_action *exc_actions;

#ifdef  MACH_BSD
	void                    *uthread;
#endif

#if CONFIG_DTRACE
	uint16_t                t_dtrace_flags;                                   
#define TH_DTRACE_EXECSUCCESS   0x01
	uint16_t                t_dtrace_inprobe;                                         
	uint32_t                t_dtrace_predcache;                                                 
	int64_t                 t_dtrace_tracing;                                             
	int64_t                 t_dtrace_vtime;
#endif

	clock_sec_t             t_page_creation_time;
	uint32_t                t_page_creation_count;
	uint32_t                t_page_creation_throttled;
#if (DEVELOPMENT || DEBUG)
	uint64_t                t_page_creation_throttled_hard;
	uint64_t                t_page_creation_throttled_soft;
#endif                           
	int                     t_pagein_error;                                                             

#ifdef KPERF
                                                                             
#define T_KPERF_CALLSTACK_DEPTH_OFFSET     (24)
#define T_KPERF_SET_CALLSTACK_DEPTH(DEPTH) (((uint32_t)(DEPTH)) << T_KPERF_CALLSTACK_DEPTH_OFFSET)
#define T_KPERF_GET_CALLSTACK_DEPTH(FLAGS) ((FLAGS) >> T_KPERF_CALLSTACK_DEPTH_OFFSET)
#define T_KPERF_ACTIONID_OFFSET            (18)
#define T_KPERF_SET_ACTIONID(AID)          (((uint32_t)(AID)) << T_KPERF_ACTIONID_OFFSET)
#define T_KPERF_GET_ACTIONID(FLAGS)        ((FLAGS) >> T_KPERF_ACTIONID_OFFSET)
#endif

#define T_KPERF_AST_CALLSTACK 0x1                                            
#define T_KPERF_AST_DISPATCH  0x2                                       
#define T_KPC_ALLOC           0x4                                       

#define T_KPERF_AST_ALL \
    (T_KPERF_AST_CALLSTACK | T_KPERF_AST_DISPATCH | T_KPC_ALLOC)
                                               

#ifdef KPERF
	uint32_t                kperf_ast;
	uint32_t                kperf_pet_gen;                                                      
	uint32_t                kperf_c_switch;                              
	uint32_t                kperf_pet_cnt;                                                       
#endif

#ifdef KPC
	                                                      
	uint64_t               *kpc_buf;
#endif

#if HYPERVISOR
	                                                               
	void                   *hv_thread_target;
#endif                 

	                                                               
	uint32_t                syscalls_unix;
	uint32_t                syscalls_mach;
	ledger_t                t_ledger;
	ledger_t                t_threadledger;                        
	ledger_t                t_bankledger;                                              
	uint64_t                t_deduct_bank_ledger_time;                                                 
	uint64_t                t_deduct_bank_ledger_energy;                                             

	uint64_t                thread_id;                                             

#if MONOTONIC
	struct mt_thread        t_monotonic;
#endif                

	                                             
	struct thread_requested_policy  requested_policy;
	struct thread_effective_policy  effective_policy;

	                                                                                    
	struct thread_qos_override {
		struct thread_qos_override      *override_next;
		uint32_t        override_contended_resource_count;
		int16_t         override_qos;
		int16_t         override_resource_type;
		user_addr_t     override_resource;
	} *overrides;

	uint32_t                kevent_overrides;
	uint8_t                 user_promotion_basepri;
	uint8_t                 kern_promotion_schedpri;
	_Atomic uint16_t        kevent_ast_bits;

	io_stat_info_t          thread_io_stats;                                

	uint32_t                thread_callout_interrupt_wakeups;
	uint32_t                thread_callout_platform_idle_wakeups;
	uint32_t                thread_timer_wakeups_bin_1;
	uint32_t                thread_timer_wakeups_bin_2;
	thread_tag_t            thread_tag;

	  
                                                                                        
                                                                                          
                        
    
	uint16_t
	    callout_woken_from_icontext:1,
	    callout_woken_from_platform_idle:1,
	    callout_woke_thread:1,
	    guard_exc_fatal:1,
	    thread_bitfield_unused:12;

	mach_port_name_t        ith_voucher_name;
	ipc_voucher_t           ith_voucher;
#if CONFIG_IOSCHED
	void                   *decmp_upl;
#endif                     

	                                                                          
	struct work_interval            *th_work_interval;
	thread_work_interval_flags_t    th_work_interval_flags;

#if     SCHED_TRACE_THREAD_WAKEUPS
	uintptr_t               thread_wakeup_bt[64];
#endif
	turnstile_update_flags_t inheritor_flags;                                                   
	block_hint_t             pending_block_hint;
	block_hint_t             block_hint;                                                                    
	integer_t                decompressions;                                                                                                 
	int                      thread_region_page_shift;                                                         
	                                                                                                           
	                                                                                                                        
};

#define ith_state           saved.receive.state
#define ith_object          saved.receive.object
#define ith_msg_addr        saved.receive.msg_addr
#define ith_rsize           saved.receive.rsize
#define ith_msize           saved.receive.msize
#define ith_option          saved.receive.option
#define ith_receiver_name   saved.receive.receiver_name
#define ith_continuation    saved.receive.continuation
#define ith_kmsg            saved.receive.kmsg
#define ith_peekq           saved.receive.peekq
#define ith_knote           saved.receive.knote
#define ith_ppriority       saved.receive.received_qos.ppri
#define ith_qos_override    saved.receive.received_qos.oqos
#define ith_seqno           saved.receive.seqno

#define sth_waitsemaphore   saved.sema.waitsemaphore
#define sth_signalsemaphore saved.sema.signalsemaphore
#define sth_options         saved.sema.options
#define sth_result          saved.sema.result
#define sth_continuation    saved.sema.continuation

#define ITH_KNOTE_NULL      ((void *)NULL)
#define ITH_KNOTE_PSEUDO    ((void *)0xdeadbeef)
  
                                                                               
                                                                              
                                       
   
#define ITH_KNOTE_VALID(kn, msgt_name) \
	        (((kn) != ITH_KNOTE_NULL && (kn) != ITH_KNOTE_PSEUDO) && \
	         ((msgt_name) == MACH_MSG_TYPE_PORT_RECEIVE || \
	         (msgt_name) == MACH_MSG_TYPE_PORT_SEND_ONCE))

#if MACH_ASSERT
#define assert_thread_magic(thread) assertf((thread)->thread_magic == THREAD_MAGIC, \
	                                    "bad thread magic 0x%llx for thread %p, expected 0x%llx", \
	                                    (thread)->thread_magic, (thread), THREAD_MAGIC)
#else
#define assert_thread_magic(thread) do { (void)(thread); } while (0)
#endif

extern thread_t                 thread_bootstrap(void);

extern void                     thread_machine_init_template(void);

extern void                     thread_init(void);

extern void                     thread_daemon_init(void);

#define thread_reference_internal(thread)       \
	                os_ref_retain(&(thread)->ref_count);

#define thread_reference(thread)                                        \
MACRO_BEGIN                                                                                     \
	if ((thread) != THREAD_NULL)                                    \
	        thread_reference_internal(thread);              \
MACRO_END

extern void                     thread_deallocate(
	thread_t                thread);

extern void                     thread_inspect_deallocate(
	thread_inspect_t        thread);

extern void                     thread_read_deallocate(
	thread_read_t        thread);

extern void                     thread_terminate_self(void);

__options_decl(thread_terminate_options_t, uint32_t, {
	TH_TERMINATE_OPTION_NONE,
	TH_TERMINATE_OPTION_UNPIN
});

extern kern_return_t    thread_terminate_internal(
	thread_t                    thread,
	thread_terminate_options_t  options);

extern void                     thread_start(
	thread_t                        thread) __attribute__ ((noinline));

extern void                     thread_start_in_assert_wait(
	thread_t                        thread,
	event_t             event,
	wait_interrupt_t    interruptible) __attribute__ ((noinline));

extern void                     thread_terminate_enqueue(
	thread_t                thread);

extern void                     thread_exception_enqueue(
	task_t          task,
	thread_t        thread,
	exception_type_t etype);

extern void                     thread_copy_resource_info(
	thread_t dst_thread,
	thread_t src_thread);

extern void                     thread_terminate_crashed_threads(void);

extern void                     thread_stack_enqueue(
	thread_t                thread);

extern void                     thread_hold(
	thread_t        thread);

extern void                     thread_release(
	thread_t        thread);

extern void                     thread_corpse_continue(void) __dead2;

extern boolean_t                thread_is_active(thread_t thread);

extern lck_grp_t                thread_lck_grp;

                                                                                        
#define thread_lock_init(th)    simple_lock_init(&(th)->sched_lock, 0)
#define thread_lock(th)                 simple_lock(&(th)->sched_lock, &thread_lck_grp)
#define thread_unlock(th)               simple_unlock(&(th)->sched_lock)

#define wake_lock_init(th)              simple_lock_init(&(th)->wake_lock, 0)
#define wake_lock(th)                   simple_lock(&(th)->wake_lock, &thread_lck_grp)
#define wake_unlock(th)                 simple_unlock(&(th)->wake_lock)

#define thread_should_halt_fast(thread)         (!(thread)->active)

extern void                             stack_alloc(
	thread_t                thread);

extern void                     stack_handoff(
	thread_t                from,
	thread_t                to);

extern void                             stack_free(
	thread_t                thread);

extern void                             stack_free_reserved(
	thread_t                thread);

extern boolean_t                stack_alloc_try(
	thread_t            thread);

extern void                             stack_collect(void);

extern void                             stack_init(void);


extern kern_return_t    thread_info_internal(
	thread_t                                thread,
	thread_flavor_t                 flavor,
	thread_info_t                   thread_info_out,
	mach_msg_type_number_t  *thread_info_count);



extern kern_return_t    kernel_thread_create(
	thread_continue_t       continuation,
	void                            *parameter,
	integer_t                       priority,
	thread_t                        *new_thread);

extern kern_return_t    kernel_thread_start_priority(
	thread_continue_t       continuation,
	void                            *parameter,
	integer_t                       priority,
	thread_t                        *new_thread);

extern void                             machine_stack_attach(
	thread_t                thread,
	vm_offset_t             stack);

extern vm_offset_t              machine_stack_detach(
	thread_t                thread);

extern void                             machine_stack_handoff(
	thread_t                old,
	thread_t                new);

extern thread_t                 machine_switch_context(
	thread_t                        old_thread,
	thread_continue_t       continuation,
	thread_t                        new_thread);

extern void                             machine_load_context(
	thread_t                thread) __attribute__((noreturn));

extern kern_return_t    machine_thread_state_initialize(
	thread_t                                thread);

extern kern_return_t    machine_thread_set_state(
	thread_t                                thread,
	thread_flavor_t                 flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  count);

extern mach_vm_address_t machine_thread_pc(
	thread_t                thread);

extern void machine_thread_reset_pc(
	thread_t                thread,
	mach_vm_address_t       pc);

extern boolean_t        machine_thread_on_core(
	thread_t                thread);

extern kern_return_t    machine_thread_get_state(
	thread_t                                thread,
	thread_flavor_t                 flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  *count);

extern kern_return_t    machine_thread_state_convert_from_user(
	thread_t                                thread,
	thread_flavor_t                 flavor,
	thread_state_t                  tstate,
	mach_msg_type_number_t  count);

extern kern_return_t    machine_thread_state_convert_to_user(
	thread_t                                thread,
	thread_flavor_t                 flavor,
	thread_state_t                  tstate,
	mach_msg_type_number_t  *count);

extern kern_return_t    machine_thread_dup(
	thread_t                self,
	thread_t                target,
	boolean_t               is_corpse);

extern void             machine_thread_init(void);

extern void             machine_thread_template_init(thread_t thr_template);


extern kern_return_t    machine_thread_create(
	thread_t                thread,
	task_t                  task);
extern void             machine_thread_switch_addrmode(
	thread_t                 thread);

extern void                 machine_thread_destroy(
	thread_t                thread);

extern void                             machine_set_current_thread(
	thread_t                        thread);

extern kern_return_t    machine_thread_get_kern_state(
	thread_t                                thread,
	thread_flavor_t                 flavor,
	thread_state_t                  tstate,
	mach_msg_type_number_t  *count);

extern kern_return_t    machine_thread_inherit_taskwide(
	thread_t                thread,
	task_t                  parent_task);

extern kern_return_t    machine_thread_set_tsd_base(
	thread_t                                thread,
	mach_vm_offset_t                tsd_base);

#define thread_mtx_lock(thread)                 lck_mtx_lock(&(thread)->mutex)
#define thread_mtx_try(thread)                  lck_mtx_try_lock(&(thread)->mutex)
#define thread_mtx_unlock(thread)               lck_mtx_unlock(&(thread)->mutex)
#define thread_mtx_held(thread)                 lck_mtx_assert(&(thread)->mutex, LCK_MTX_ASSERT_OWNED)

extern void thread_apc_ast(thread_t thread);

extern void thread_update_qos_cpu_time(thread_t thread);

void act_machine_sv_free(thread_t, int);

vm_offset_t                     min_valid_stack_address(void);
vm_offset_t                     max_valid_stack_address(void);

extern bool thread_no_smt(thread_t thread);
extern bool processor_active_thread_no_smt(processor_t processor);

extern void thread_set_options(uint32_t thopt);

#if CONFIG_THREAD_GROUPS
struct thread_group *thread_get_current_voucher_thread_group(thread_t thread);
#endif                           

#else                            

__BEGIN_DECLS

extern void thread_mtx_lock(thread_t thread);

extern void thread_mtx_unlock(thread_t thread);

extern thread_t         current_thread(void) __attribute__((const));

extern void                     thread_reference(
	thread_t        thread);

extern void                     thread_deallocate(
	thread_t        thread);

#if BSD_KERNEL_PRIVATE
                                         
__options_decl(port_to_thread_options_t, uint32_t, {
	PORT_TO_THREAD_NONE               = 0x0000,
	PORT_TO_THREAD_IN_CURRENT_TASK    = 0x0001,
	PORT_TO_THREAD_NOT_CURRENT_THREAD = 0x0002,
});

extern thread_t port_name_to_thread(
	mach_port_name_t            port_name,
	port_to_thread_options_t    options);
#endif                         

__END_DECLS

#endif                           

#ifdef  KERNEL_PRIVATE

__BEGIN_DECLS

extern void                     thread_deallocate_safe(
	thread_t                thread);

extern uint64_t                 thread_dispatchqaddr(
	thread_t thread);

extern uint64_t                 thread_rettokern_addr(
	thread_t thread);

extern integer_t        thread_kern_get_pri(thread_t thr) __attribute__((const));

extern void             thread_kern_set_pri(thread_t thr, integer_t pri);

extern integer_t        thread_kern_get_kernel_maxpri(void) __attribute__((const));

__END_DECLS

#endif                      

#ifdef KERNEL
__BEGIN_DECLS

extern uint64_t                 thread_tid(thread_t thread);

__END_DECLS

#endif              

__BEGIN_DECLS

#ifdef  XNU_KERNEL_PRIVATE

uint16_t        thread_set_tag(thread_t thread, uint16_t tag);
uint16_t        thread_get_tag(thread_t thread);

#ifdef MACH_KERNEL_PRIVATE
static inline thread_tag_t
thread_set_tag_internal(thread_t thread, thread_tag_t tag)
{
	return os_atomic_or_orig(&thread->thread_tag, tag, relaxed);
}

static inline thread_tag_t
thread_get_tag_internal(thread_t thread)
{
	return thread->thread_tag;
}
#endif                          

uint64_t        thread_last_run_time(thread_t thread);

extern kern_return_t    thread_state_initialize(
	thread_t                                thread);

extern kern_return_t    thread_setstatus(
	thread_t                                thread,
	int                                             flavor,
	thread_state_t                  tstate,
	mach_msg_type_number_t  count);

extern kern_return_t    thread_setstatus_from_user(
	thread_t                                thread,
	int                                             flavor,
	thread_state_t                  tstate,
	mach_msg_type_number_t  count);

extern kern_return_t    thread_getstatus(
	thread_t                                thread,
	int                                             flavor,
	thread_state_t                  tstate,
	mach_msg_type_number_t  *count);

extern kern_return_t    thread_getstatus_to_user(
	thread_t                                thread,
	int                                             flavor,
	thread_state_t                  tstate,
	mach_msg_type_number_t  *count);

extern kern_return_t    thread_create_with_continuation(
	task_t task,
	thread_t *new_thread,
	thread_continue_t continuation);

                                   
__options_decl(th_create_waiting_options_t, uint32_t, {
	TH_CREATE_WAITING_OPTION_PINNED = 0x10,
	TH_CREATE_WAITING_OPTION_IMMOVABLE = 0x20,
});
#define TH_CREATE_WAITING_OPTION_MASK          0x30

extern kern_return_t thread_create_waiting(task_t    task,
    thread_continue_t              continuation,
    event_t                        event,
    th_create_waiting_options_t    options,
    thread_t                       *new_thread);

extern kern_return_t    thread_create_workq_waiting(
	task_t                  task,
	thread_continue_t       thread_return,
	thread_t                *new_thread);

extern  void    thread_yield_internal(
	mach_msg_timeout_t      interval);

extern void             thread_yield_to_preemption(void);

  
                                                                                                   
  
                                                                            
                                                                                      
                                             
   
#define THREAD_CPULIMIT_BLOCK           0x1
#define THREAD_CPULIMIT_EXCEPTION       0x2
#define THREAD_CPULIMIT_DISABLE         0x3

struct _thread_ledger_indices {
	int cpu_time;
};

extern struct _thread_ledger_indices thread_ledgers;

extern int thread_get_cpulimit(int *action, uint8_t *percentage, uint64_t *interval_ns);
extern int thread_set_cpulimit(int action, uint8_t percentage, uint64_t interval_ns);

extern void                     thread_read_times(
	thread_t                thread,
	time_value_t    *user_time,
	time_value_t    *system_time,
	time_value_t    *runnable_time);

extern uint64_t         thread_get_runtime_self(void);

extern void                     thread_setuserstack(
	thread_t                thread,
	mach_vm_offset_t        user_stack);

extern user_addr_t         thread_adjuserstack(
	thread_t                thread,
	int                             adjust);


extern void                     thread_setentrypoint(
	thread_t                thread,
	mach_vm_offset_t        entry);

extern kern_return_t    thread_set_tsd_base(
	thread_t        thread,
	mach_vm_offset_t tsd_base);

extern kern_return_t    thread_setsinglestep(
	thread_t                thread,
	int                     on);

extern kern_return_t    thread_userstack(
	thread_t,
	int,
	thread_state_t,
	unsigned int,
	mach_vm_offset_t *,
	int *,
	boolean_t);

extern kern_return_t    thread_entrypoint(
	thread_t,
	int,
	thread_state_t,
	unsigned int,
	mach_vm_offset_t *);

extern kern_return_t    thread_userstackdefault(
	mach_vm_offset_t *,
	boolean_t);

extern kern_return_t    thread_wire_internal(
	host_priv_t             host_priv,
	thread_t                thread,
	boolean_t               wired,
	boolean_t               *prev_state);


extern kern_return_t    thread_dup(thread_t);

extern kern_return_t thread_dup2(thread_t, thread_t);

#if !defined(_SCHED_CALL_T_DEFINED)
#define _SCHED_CALL_T_DEFINED
typedef void    (*sched_call_t)(
	int                             type,
	thread_t                thread);
#endif

#define SCHED_CALL_BLOCK                0x1
#define SCHED_CALL_UNBLOCK              0x2

extern void             thread_sched_call(
	thread_t                thread,
	sched_call_t    call);

extern boolean_t        thread_is_static_param(
	thread_t                thread);

extern task_t   get_threadtask(thread_t);

  
                                                   
   
#define thread_is_64bit_addr(thd)       \
	task_has_64Bit_addr(get_threadtask(thd))

  
                                        
   
#define thread_is_64bit_data(thd)       \
	task_has_64Bit_data(get_threadtask(thd))

#if defined(__x86_64__)
extern int              thread_task_has_ldt(thread_t);
#endif
extern void             *get_bsdthread_info(thread_t);
extern void             set_bsdthread_info(thread_t, void *);
extern void             set_thread_pagein_error(thread_t, int);
extern void             *uthread_alloc(task_t, thread_t, int);
extern event_t  workq_thread_init_and_wq_lock(task_t, thread_t);                
extern void             uthread_cleanup_name(void *uthread);
extern void             uthread_cleanup(task_t, void *, void *);
extern void             uthread_zone_free(void *);
extern void             uthread_cred_free(void *);
extern void             uthread_reset_proc_refcount(void *);
#if PROC_REF_DEBUG
extern int              uthread_get_proc_refcount(void *);
extern int              proc_ref_tracking_disabled;
#endif

extern boolean_t        thread_should_halt(
	thread_t                thread);

extern boolean_t        thread_should_abort(
	thread_t);

extern int is_64signalregset(void);

extern void act_set_kperf(thread_t);
extern void act_set_astledger(thread_t thread);
extern void act_set_astledger_async(thread_t thread);
extern void act_set_io_telemetry_ast(thread_t);

extern uint32_t dtrace_get_thread_predcache(thread_t);
extern int64_t dtrace_get_thread_vtime(thread_t);
extern int64_t dtrace_get_thread_tracing(thread_t);
extern uint16_t dtrace_get_thread_inprobe(thread_t);
extern int dtrace_get_thread_last_cpu_id(thread_t);
extern vm_offset_t dtrace_get_kernel_stack(thread_t);
extern void dtrace_set_thread_predcache(thread_t, uint32_t);
extern void dtrace_set_thread_vtime(thread_t, int64_t);
extern void dtrace_set_thread_tracing(thread_t, int64_t);
extern void dtrace_set_thread_inprobe(thread_t, uint16_t);
extern vm_offset_t dtrace_set_thread_recover(thread_t, vm_offset_t);
extern vm_offset_t dtrace_sign_and_set_thread_recover(thread_t, vm_offset_t);
extern void dtrace_thread_bootstrap(void);
extern void dtrace_thread_didexec(thread_t);

extern int64_t dtrace_calc_thread_recent_vtime(thread_t);


extern kern_return_t    thread_set_wq_state32(
	thread_t          thread,
	thread_state_t    tstate);

extern kern_return_t    thread_set_wq_state64(
	thread_t          thread,
	thread_state_t    tstate);

extern vm_offset_t      kernel_stack_mask;
extern vm_offset_t      kernel_stack_size;
extern vm_offset_t      kernel_stack_depth_max;

extern void guard_ast(thread_t);
extern void fd_guard_ast(thread_t,
    mach_exception_code_t, mach_exception_subcode_t);
#if CONFIG_VNGUARD
extern void vn_guard_ast(thread_t,
    mach_exception_code_t, mach_exception_subcode_t);
#endif
extern void mach_port_guard_ast(thread_t,
    mach_exception_code_t, mach_exception_subcode_t);
extern void virt_memory_guard_ast(thread_t,
    mach_exception_code_t, mach_exception_subcode_t);
extern void thread_guard_violation(thread_t,
    mach_exception_code_t, mach_exception_subcode_t, boolean_t);
extern void thread_update_io_stats(thread_t, int size, int io_flags);

extern kern_return_t    thread_set_voucher_name(mach_port_name_t name);
extern kern_return_t thread_get_current_voucher_origin_pid(int32_t *pid);

extern void set_thread_rwlock_boost(void);
extern void clear_thread_rwlock_boost(void);

extern void thread_enable_send_importance(thread_t thread, boolean_t enable);

  
                                                                    
   

extern kern_return_t    machine_thread_siguctx_pointer_convert_to_user(
	thread_t thread,
	user_addr_t *uctxp);

extern void machine_tecs(thread_t thr);

typedef enum cpuvn {
	CPUVN_CI = 1
} cpuvn_e;

extern int machine_csv(cpuvn_e cve);
#if defined(__x86_64__)
extern void machine_thread_set_insn_copy_optout(thread_t thr);
#endif

  
                                                                                      
   

extern kern_return_t    machine_thread_function_pointers_convert_from_user(
	thread_t thread,
	user_addr_t *fptrs,
	uint32_t count);

  
                                                                                     
                         
  
                                                                                   
                                                                      
   

extern int machine_trace_thread(
	thread_t thread,
	char *tracepos,
	char *tracebound,
	int nframes,
	boolean_t user_p,
	uint32_t *thread_trace_flags);

extern int machine_trace_thread64(thread_t thread,
    char *tracepos,
    char *tracebound,
    int nframes,
    boolean_t user_p,
    uint32_t *thread_trace_flags,
    uint64_t *sp,
    vm_offset_t fp);

  
                                                    
   
uint64_t thread_get_last_wait_duration(thread_t thread);

extern bool thread_get_no_smt(void);
#if defined(__x86_64__)
extern bool curtask_get_insn_copy_optout(void);
extern void curtask_set_insn_copy_optout(void);
#endif                          

#endif                          

#ifdef KERNEL_PRIVATE
extern void thread_set_no_smt(bool set);
#endif                     

                                    
                                             
                                                                                                                                                      
                                     
                                                            
   
extern boolean_t thread_has_thread_name(thread_t th);

                                    
                                   
                                                                                                                                                                                                                                                                              
                                      
                                                 
   
extern void thread_set_thread_name(thread_t th, const char* name);

#ifdef XNU_KERNEL_PRIVATE
extern void
thread_get_thread_name(thread_t th, char* name);
#endif                         

                                 
                                      
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
                                                                                    
                                                                           
                                                                                 
                                                                                
   

extern kern_return_t    kernel_thread_start(
	thread_continue_t       continuation,
	void                            *parameter,
	thread_t                        *new_thread);

#ifdef KERNEL_PRIVATE
void thread_set_eager_preempt(thread_t thread);
void thread_clear_eager_preempt(thread_t thread);
void thread_set_honor_qlimit(thread_t thread);
void thread_clear_honor_qlimit(thread_t thread);
extern ipc_port_t convert_thread_to_port(thread_t);
extern ipc_port_t convert_thread_to_port_pinned(thread_t);
extern ipc_port_t convert_thread_inspect_to_port(thread_inspect_t);
extern ipc_port_t convert_thread_read_to_port(thread_read_t);
extern boolean_t is_vm_privileged(void);
extern boolean_t set_vm_privilege(boolean_t);
extern kern_allocation_name_t thread_set_allocation_name(kern_allocation_name_t new_name);
extern void *thread_iokit_tls_get(uint32_t index);
extern void thread_iokit_tls_set(uint32_t index, void * data);
extern void thread_port_with_flavor_notify(mach_msg_header_t *msg);
extern int thread_self_region_page_shift(void);
extern void thread_self_region_page_shift_set(int pgshift);
extern kern_return_t thread_create_pinned(task_t task, thread_t *new_thread);
extern kern_return_t thread_create_immovable(task_t task, thread_t *new_thread);
extern kern_return_t thread_terminate_pinned(thread_t thread);
#endif                     

__END_DECLS

#endif                       
