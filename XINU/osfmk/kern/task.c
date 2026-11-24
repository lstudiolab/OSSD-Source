/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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

#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <mach/host_priv.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_param.h>
#include <mach/mach_vm.h>
#include <mach/semaphore.h>
#include <mach/task_info.h>
#include <mach/task_inspect.h>
#include <mach/task_special_ports.h>
#include <mach/sdt.h>

#include <ipc/ipc_importance.h>
#include <ipc/ipc_types.h>
#include <ipc/ipc_space.h>
#include <ipc/ipc_entry.h>
#include <ipc/ipc_hash.h>

#include <kern/kern_types.h>
#include <kern/mach_param.h>
#include <kern/misc_protos.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/coalition.h>
#include <kern/zalloc.h>
#include <kern/kalloc.h>
#include <kern/kern_cdata.h>
#include <kern/processor.h>
#include <kern/sched_prim.h>                           
#include <kern/ipc_tt.h>
#include <kern/host.h>
#include <kern/clock.h>
#include <kern/timer.h>
#include <kern/assert.h>
#include <kern/sync_lock.h>
#include <kern/affinity.h>
#include <kern/exc_resource.h>
#include <kern/machine.h>
#include <kern/policy_internal.h>
#include <kern/restartable.h>

#include <corpses/task_corpse.h>
#if CONFIG_TELEMETRY
#include <kern/telemetry.h>
#endif

#if MONOTONIC
#include <kern/monotonic.h>
#include <machine/monotonic.h>
#endif                

#include <os/log.h>

#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <vm/vm_kern.h>                                             
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>
#include <vm/vm_purgeable_internal.h>
#include <vm/vm_compressor_pager.h>

#include <sys/resource.h>
#include <sys/signalvar.h>                   
#include <sys/bsdtask_info.h>
  
                      
   

#include <mach/task_server.h>
#include <mach/mach_host_server.h>
#include <mach/host_security_server.h>
#include <mach/mach_port_server.h>

#include <vm/vm_shared_region.h>

#include <libkern/OSDebug.h>
#include <libkern/OSAtomic.h>
#include <libkern/section_keywords.h>

#include <mach-o/loader.h>
#include <kdp/kdp_dyld.h>

#include <kern/sfi.h>                                  

#if CONFIG_MACF
#include <security/mac_mach_internal.h>
#endif

#include <IOKit/IOBSD.h>

#if KPERF
extern int kpc_force_all_ctrs(task_t, int);
#endif

SECURITY_READ_ONLY_LATE(task_t) kernel_task;

static SECURITY_READ_ONLY_LATE(zone_t) task_zone;
ZONE_INIT(&task_zone, "tasks", sizeof(struct task),
    ZC_NOENCRYPT | ZC_ZFREE_CLEARMEM,
    ZONE_ID_TASK, NULL);

extern int exc_via_corpse_forking;
extern int corpse_for_fatal_memkill;
extern boolean_t proc_send_synchronous_EXC_RESOURCE(void *p);
extern void task_disown_frozen_csegs(task_t owner_task);

                                                                                                       
int audio_active = 0;

  
                                    
                                                                        
   
typedef struct zinfo_usage_store_t {
	                                                                           
	uint64_t        alloc __attribute__((aligned(8)));                                      
	uint64_t        free __attribute__((aligned(8)));                                 
} zinfo_usage_store_t;

zinfo_usage_store_t tasks_tkm_private;
zinfo_usage_store_t tasks_tkm_shared;

                                                            
expired_task_statistics_t               dead_task_statistics;
LCK_SPIN_DECLARE_ATTR(dead_task_statistics_lock, &task_lck_grp, &task_lck_attr);

ledger_template_t task_ledger_template = NULL;

                                                                                    
LCK_GRP_DECLARE(g_dyldinfo_mtx_grp, "g_dyldinfo");
LCK_MTX_DECLARE(g_dyldinfo_mtx, &g_dyldinfo_mtx_grp);

SECURITY_READ_ONLY_LATE(struct _task_ledger_indices) task_ledgers __attribute__((used)) =
{.cpu_time = -1,
 .tkm_private = -1,
 .tkm_shared = -1,
 .phys_mem = -1,
 .wired_mem = -1,
 .internal = -1,
 .iokit_mapped = -1,
 .alternate_accounting = -1,
 .alternate_accounting_compressed = -1,
 .page_table = -1,
 .phys_footprint = -1,
 .internal_compressed = -1,
 .purgeable_volatile = -1,
 .purgeable_nonvolatile = -1,
 .purgeable_volatile_compressed = -1,
 .purgeable_nonvolatile_compressed = -1,
 .tagged_nofootprint = -1,
 .tagged_footprint = -1,
 .tagged_nofootprint_compressed = -1,
 .tagged_footprint_compressed = -1,
 .network_volatile = -1,
 .network_nonvolatile = -1,
 .network_volatile_compressed = -1,
 .network_nonvolatile_compressed = -1,
 .media_nofootprint = -1,
 .media_footprint = -1,
 .media_nofootprint_compressed = -1,
 .media_footprint_compressed = -1,
 .graphics_nofootprint = -1,
 .graphics_footprint = -1,
 .graphics_nofootprint_compressed = -1,
 .graphics_footprint_compressed = -1,
 .neural_nofootprint = -1,
 .neural_footprint = -1,
 .neural_nofootprint_compressed = -1,
 .neural_footprint_compressed = -1,
 .platform_idle_wakeups = -1,
 .interrupt_wakeups = -1,
#if CONFIG_SCHED_SFI
 .sfi_wait_times = { 0                             },
#endif                       
 .cpu_time_billed_to_me = -1,
 .cpu_time_billed_to_others = -1,
 .physical_writes = -1,
 .logical_writes = -1,
 .logical_writes_to_external = -1,
#if DEBUG || DEVELOPMENT
 .pages_grabbed = -1,
 .pages_grabbed_kern = -1,
 .pages_grabbed_iopl = -1,
 .pages_grabbed_upl = -1,
#endif
#if CONFIG_FREEZE
 .frozen_to_swap = -1,
#endif                    
 .energy_billed_to_me = -1,
 .energy_billed_to_others = -1,
#if CONFIG_PHYS_WRITE_ACCT
 .fs_metadata_writes = -1,
#endif                             
};

                        
boolean_t tasks_suspend_state;


void init_task_ledgers(void);
void task_footprint_exceeded(int warning, __unused const void *param0, __unused const void *param1);
void task_wakeups_rate_exceeded(int warning, __unused const void *param0, __unused const void *param1);
void task_io_rate_exceeded(int warning, const void *param0, __unused const void *param1);
void __attribute__((noinline)) SENDING_NOTIFICATION__THIS_PROCESS_IS_CAUSING_TOO_MANY_WAKEUPS(void);
void __attribute__((noinline)) PROC_CROSSED_HIGH_WATERMARK__SEND_EXC_RESOURCE_AND_SUSPEND(int max_footprint_mb, boolean_t is_fatal);
void __attribute__((noinline)) SENDING_NOTIFICATION__THIS_PROCESS_IS_CAUSING_TOO_MUCH_IO(int flavor);

kern_return_t task_suspend_internal(task_t);
kern_return_t task_resume_internal(task_t);
static kern_return_t task_start_halt_locked(task_t task, boolean_t should_mark_corpse);

extern kern_return_t iokit_task_terminate(task_t task);
extern void          iokit_task_app_suspended_changed(task_t task);

extern kern_return_t exception_deliver(thread_t, exception_type_t, mach_exception_data_t, mach_msg_type_number_t, struct exception_action *, lck_mtx_t *);
extern void bsd_copythreadname(void *dst_uth, void *src_uth);
extern kern_return_t thread_resume(thread_t thread);

                                                      
#define PHYS_FOOTPRINT_WARNING_LEVEL 80

#define TASK_WAKEUPS_MONITOR_DEFAULT_LIMIT              150                         
#define TASK_WAKEUPS_MONITOR_DEFAULT_INTERVAL   300                  

  
                                                                                               
  
                                                                               
                                      
   
#define TASK_WAKEUPS_MONITOR_DEFAULT_USTACKSHOTS_TRIGGER        70

int task_wakeups_monitor_interval;                                                                  
int task_wakeups_monitor_rate;                                                                                

unsigned int task_wakeups_monitor_ustackshots_trigger_pct;                                                               

int disable_exc_resource;                                                                               

ledger_amount_t max_task_footprint = 0;                                                                  
unsigned int max_task_footprint_warning_level = 0;                                         
int max_task_footprint_mb = 0;                                                                  

                        
#define IOMON_DEFAULT_LIMIT                     (20480ull)                                      
#define IOMON_DEFAULT_INTERVAL                  (86400ull)                      

uint64_t task_iomon_limit_mb;                                                  
uint64_t task_iomon_interval_secs;                                                 

#define IO_TELEMETRY_DEFAULT_LIMIT              (10ll * 1024ll * 1024ll)
int64_t io_telemetry_limit;                                                                                                       
int64_t global_logical_writes_count = 0;                                             
int64_t global_logical_writes_to_external_count = 0;                                                                
static boolean_t global_update_logical_writes(int64_t, int64_t*);

#define TASK_MAX_THREAD_LIMIT 256

#if MACH_ASSERT
int pmap_ledgers_panic = 1;
int pmap_ledgers_panic_leeway = 3;
#endif                  

int task_max = CONFIG_TASK_MAX;                          

#if CONFIG_COREDUMP
int hwm_user_cores = 0;                                                         
#endif

#ifdef MACH_BSD
extern uint32_t proc_platform(const struct proc *);
extern uint32_t proc_min_sdk(struct proc *);
extern void     proc_getexecutableuuid(void *, unsigned char *, unsigned long);
extern int      proc_pid(struct proc *p);
extern int      proc_selfpid(void);
extern struct proc *current_proc(void);
extern char     *proc_name_address(struct proc *p);
extern uint64_t get_dispatchqueue_offset_from_proc(void *);
extern int kevent_proc_copy_uptrs(void *proc, uint64_t *buf, uint32_t bufsize);
extern void workq_proc_suspended(struct proc *p);
extern void workq_proc_resumed(struct proc *p);

#if CONFIG_MEMORYSTATUS
extern void     proc_memstat_terminated(struct proc* p, boolean_t set);
extern void     memorystatus_on_ledger_footprint_exceeded(int warning, boolean_t memlimit_is_active, boolean_t memlimit_is_fatal);
extern void     memorystatus_log_exception(const int max_footprint_mb, boolean_t memlimit_is_active, boolean_t memlimit_is_fatal);
extern boolean_t memorystatus_allowed_vm_map_fork(task_t task);
extern uint64_t  memorystatus_available_memory_internal(struct proc *p);

#if DEVELOPMENT || DEBUG
extern void memorystatus_abort_vm_map_fork(task_t);
#endif

#endif                          

#endif               

#if DEVELOPMENT || DEBUG
int exc_resource_threads_enabled;
#endif                           

#if (DEVELOPMENT || DEBUG)
uint32_t task_exc_guard_default = TASK_EXC_GUARD_MP_DELIVER | TASK_EXC_GUARD_MP_ONCE | TASK_EXC_GUARD_MP_CORPSE |
    TASK_EXC_GUARD_VM_DELIVER | TASK_EXC_GUARD_VM_ONCE | TASK_EXC_GUARD_VM_CORPSE;
#else
uint32_t task_exc_guard_default = 0;
#endif

              

static void task_hold_locked(task_t task);
static void task_wait_locked(task_t task, boolean_t until_not_runnable);
static void task_release_locked(task_t task);

static void task_synchronizer_destroy_all(task_t task);
static os_ref_count_t
task_add_turnstile_watchports_locked(
	task_t                      task,
	struct task_watchports      *watchports,
	struct task_watchport_elem  **previous_elem_array,
	ipc_port_t                  *portwatch_ports,
	uint32_t                    portwatch_count);

static os_ref_count_t
task_remove_turnstile_watchports_locked(
	task_t                 task,
	struct task_watchports *watchports,
	ipc_port_t             *port_freelist);

static struct task_watchports *
task_watchports_alloc_init(
	task_t        task,
	thread_t      thread,
	uint32_t      count);

static void
task_watchports_deallocate(
	struct task_watchports *watchports);

void
task_set_64bit(
	task_t task,
	boolean_t is_64bit,
	boolean_t is_64bit_data)
{
#if defined(__i386__) || defined(__x86_64__) || defined(__arm64__)
	thread_t thread;
#endif                                                                     

	task_lock(task);

	  
                                           
    
	if (is_64bit) {
		if (!task_has_64Bit_addr(task)) {
			task_set_64Bit_addr(task);
		}
	} else {
		if (task_has_64Bit_addr(task)) {
			task_clear_64Bit_addr(task);
		}
	}

	  
                                            
    
	if (is_64bit_data) {
		if (task_has_64Bit_data(task)) {
			goto out;
		}

		task_set_64Bit_data(task);
	} else {
		if (!task_has_64Bit_data(task)) {
			goto out;
		}

		task_clear_64Bit_data(task);
	}

	                                                                   
                                                                     
                                                                 
                                                                       
                                                
    

#if defined(__x86_64__) || defined(__arm64__)
	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		thread_mtx_lock(thread);
		machine_thread_switch_addrmode(thread);
		thread_mtx_unlock(thread);
	}
#endif                                                

out:
	task_unlock(task);
}

boolean_t
task_get_64bit_data(task_t task)
{
	return task_has_64Bit_data(task);
}

void
task_set_platform_binary(
	task_t task,
	boolean_t is_platform)
{
	task_lock(task);
	if (is_platform) {
		task->t_flags |= TF_PLATFORM;
		                                                         
		task->task_exc_guard = (task_exc_guard_default & TASK_EXC_GUARD_ALL);
	} else {
		task->t_flags &= ~(TF_PLATFORM);
		                                                         
		task->task_exc_guard = ((task_exc_guard_default >> TASK_EXC_GUARD_THIRD_PARTY_DEFAULT_SHIFT) & TASK_EXC_GUARD_ALL);
	}
	task_unlock(task);
}

  
                                                                              
                                                                     
   
bool
task_set_ca_client_wi(
	task_t task,
	boolean_t set_or_clear)
{
	bool ret = true;
	task_lock(task);
	if (set_or_clear) {
		                                                     
		if (task->t_flags & TF_CA_CLIENT_WI) {
			ret = false;
		} else {
			task->t_flags |= TF_CA_CLIENT_WI;
		}
	} else {
		task->t_flags &= ~TF_CA_CLIENT_WI;
	}
	task_unlock(task);
	return ret;
}

void
task_set_dyld_info(
	task_t task,
	mach_vm_address_t addr,
	mach_vm_size_t size)
{
	task_lock(task);
	task->all_image_info_addr = addr;
	task->all_image_info_size = size;
	task_unlock(task);
}

void
task_set_mach_header_address(
	task_t task,
	mach_vm_address_t addr)
{
	task_lock(task);
	task->mach_header_vm_address = addr;
	task_unlock(task);
}

void
task_bank_reset(__unused task_t task)
{
	if (task->bank_context != NULL) {
		bank_task_destroy(task);
	}
}

  
                                                         
                                                    
                                  
   
void
task_bank_init(__unused task_t task)
{
	if (task->bank_context != NULL) {
		panic("Task bank init called with non null bank context for task: %p and bank_context: %p", task, task->bank_context);
	}
	bank_task_initialize(task);
}

void
task_set_did_exec_flag(task_t task)
{
	task->t_procflags |= TPF_DID_EXEC;
}

void
task_clear_exec_copy_flag(task_t task)
{
	task->t_procflags &= ~TPF_EXEC_COPY;
}

event_t
task_get_return_wait_event(task_t task)
{
	return (event_t)&task->returnwait_inheritor;
}

void
task_clear_return_wait(task_t task, uint32_t flags)
{
	if (flags & TCRW_CLEAR_INITIAL_WAIT) {
		thread_wakeup(task_get_return_wait_event(task));
	}

	if (flags & TCRW_CLEAR_FINAL_WAIT) {
		is_write_lock(task->itk_space);

		task->t_returnwaitflags &= ~TRW_LRETURNWAIT;
		task->returnwait_inheritor = NULL;

		if (task->t_returnwaitflags & TRW_LRETURNWAITER) {
			struct turnstile *turnstile = turnstile_prepare((uintptr_t) task_get_return_wait_event(task),
			    NULL, TURNSTILE_NULL, TURNSTILE_ULOCK);

			waitq_wakeup64_all(&turnstile->ts_waitq,
			    CAST_EVENT64_T(task_get_return_wait_event(task)),
			    THREAD_AWAKENED, 0);

			turnstile_update_inheritor(turnstile, NULL,
			    TURNSTILE_IMMEDIATE_UPDATE | TURNSTILE_INHERITOR_THREAD);
			turnstile_update_inheritor_complete(turnstile, TURNSTILE_INTERLOCK_HELD);

			turnstile_complete((uintptr_t) task_get_return_wait_event(task), NULL, NULL, TURNSTILE_ULOCK);
			turnstile_cleanup();
			task->t_returnwaitflags &= ~TRW_LRETURNWAITER;
		}
		is_write_unlock(task->itk_space);
	}
}

void __attribute__((noreturn))
task_wait_to_return(void)
{
	task_t task = current_task();

	is_write_lock(task->itk_space);

	if (task->t_returnwaitflags & TRW_LRETURNWAIT) {
		struct turnstile *turnstile = turnstile_prepare((uintptr_t) task_get_return_wait_event(task),
		    NULL, TURNSTILE_NULL, TURNSTILE_ULOCK);

		do {
			task->t_returnwaitflags |= TRW_LRETURNWAITER;
			turnstile_update_inheritor(turnstile, task->returnwait_inheritor,
			    (TURNSTILE_DELAYED_UPDATE | TURNSTILE_INHERITOR_THREAD));

			waitq_assert_wait64(&turnstile->ts_waitq,
			    CAST_EVENT64_T(task_get_return_wait_event(task)),
			    THREAD_UNINT, TIMEOUT_WAIT_FOREVER);

			is_write_unlock(task->itk_space);

			turnstile_update_inheritor_complete(turnstile, TURNSTILE_INTERLOCK_NOT_HELD);

			thread_block(THREAD_CONTINUE_NULL);

			is_write_lock(task->itk_space);
		} while (task->t_returnwaitflags & TRW_LRETURNWAIT);

		turnstile_complete((uintptr_t) task_get_return_wait_event(task), NULL, NULL, TURNSTILE_ULOCK);
	}

	is_write_unlock(task->itk_space);
	turnstile_cleanup();


#if CONFIG_MACF
	  
                                                                              
                                  
    
	mac_proc_notify_exec_complete(current_proc());
#endif

	thread_bootstrap_return();
}

#ifdef CONFIG_32BIT_TELEMETRY
boolean_t
task_consume_32bit_log_flag(task_t task)
{
	if ((task->t_procflags & TPF_LOG_32BIT_TELEMETRY) != 0) {
		task->t_procflags &= ~TPF_LOG_32BIT_TELEMETRY;
		return TRUE;
	} else {
		return FALSE;
	}
}

void
task_set_32bit_log_flag(task_t task)
{
	task->t_procflags |= TPF_LOG_32BIT_TELEMETRY;
}
#endif                             

boolean_t
task_is_exec_copy(task_t task)
{
	return task_is_exec_copy_internal(task);
}

boolean_t
task_did_exec(task_t task)
{
	return task_did_exec_internal(task);
}

boolean_t
task_is_active(task_t task)
{
	return task->active;
}

boolean_t
task_is_halting(task_t task)
{
	return task->halting;
}

#if TASK_REFERENCE_LEAK_DEBUG
#include <kern/btlog.h>

static btlog_t *task_ref_btlog;
#define TASK_REF_OP_INCR        0x1
#define TASK_REF_OP_DECR        0x2

#define TASK_REF_NUM_RECORDS    100000
#define TASK_REF_BTDEPTH        7

void
task_reference_internal(task_t task)
{
	void *       bt[TASK_REF_BTDEPTH];
	int             numsaved = 0;

	task_require(task);
	os_ref_retain(&task->ref_count);

	numsaved = OSBacktrace(bt, TASK_REF_BTDEPTH);
	btlog_add_entry(task_ref_btlog, task, TASK_REF_OP_INCR,
	    bt, numsaved);
}

os_ref_count_t
task_deallocate_internal(task_t task)
{
	void *       bt[TASK_REF_BTDEPTH];
	int             numsaved = 0;

	numsaved = OSBacktrace(bt, TASK_REF_BTDEPTH);
	btlog_add_entry(task_ref_btlog, task, TASK_REF_OP_DECR,
	    bt, numsaved);

	return os_ref_release(&task->ref_count);
}

#endif                                

void
task_init(void)
{
	  
                                    
                                             
                                              
                                                   
    
	if (!PE_parse_boot_argn("max_task_pmem", &max_task_footprint_mb,
	    sizeof(max_task_footprint_mb))) {
		  
                                                                    
     
		if (!PE_get_default("kern.max_task_pmem", &max_task_footprint_mb,
		    sizeof(max_task_footprint_mb))) {
			  
                                        
      
			max_task_footprint_mb = 0;
		}
	}

	if (max_task_footprint_mb != 0) {
#if CONFIG_MEMORYSTATUS
		if (max_task_footprint_mb < 50) {
			printf("Warning: max_task_pmem %d below minimum.\n",
			    max_task_footprint_mb);
			max_task_footprint_mb = 50;
		}
		printf("Limiting task physical memory footprint to %d MB\n",
		    max_task_footprint_mb);

		max_task_footprint = (ledger_amount_t)max_task_footprint_mb * 1024 * 1024;                       

		  
                                                       
                                      
     
		max_task_footprint_warning_level = 0;

		if (max_mem < 0x40000000) {
			  
                                      
                                                          
      
			if (max_task_footprint_mb > 50) {
				max_task_footprint_warning_level = ((max_task_footprint_mb - 50) * 100) / max_task_footprint_mb;
			}
		} else {
			  
                                       
                                                           
      
			if (max_task_footprint_mb > 100) {
				max_task_footprint_warning_level = ((max_task_footprint_mb - 100) * 100) / max_task_footprint_mb;
			}
		}

		  
                                                         
     
		if (max_task_footprint_warning_level < PHYS_FOOTPRINT_WARNING_LEVEL) {
			max_task_footprint_warning_level = PHYS_FOOTPRINT_WARNING_LEVEL;
		}

		printf("Limiting task physical memory warning to %d%%\n", max_task_footprint_warning_level);

#else
		printf("Warning: max_task_pmem specified, but jetsam not configured; ignoring.\n");
#endif                          
	}

#if DEVELOPMENT || DEBUG
	if (!PE_parse_boot_argn("exc_resource_threads",
	    &exc_resource_threads_enabled,
	    sizeof(exc_resource_threads_enabled))) {
		exc_resource_threads_enabled = 1;
	}
	PE_parse_boot_argn("task_exc_guard_default",
	    &task_exc_guard_default,
	    sizeof(task_exc_guard_default));
#endif                           

#if CONFIG_COREDUMP
	if (!PE_parse_boot_argn("hwm_user_cores", &hwm_user_cores,
	    sizeof(hwm_user_cores))) {
		hwm_user_cores = 0;
	}
#endif

	proc_init_cpumon_params();

	if (!PE_parse_boot_argn("task_wakeups_monitor_rate", &task_wakeups_monitor_rate, sizeof(task_wakeups_monitor_rate))) {
		task_wakeups_monitor_rate = TASK_WAKEUPS_MONITOR_DEFAULT_LIMIT;
	}

	if (!PE_parse_boot_argn("task_wakeups_monitor_interval", &task_wakeups_monitor_interval, sizeof(task_wakeups_monitor_interval))) {
		task_wakeups_monitor_interval = TASK_WAKEUPS_MONITOR_DEFAULT_INTERVAL;
	}

	if (!PE_parse_boot_argn("task_wakeups_monitor_ustackshots_trigger_pct", &task_wakeups_monitor_ustackshots_trigger_pct,
	    sizeof(task_wakeups_monitor_ustackshots_trigger_pct))) {
		task_wakeups_monitor_ustackshots_trigger_pct = TASK_WAKEUPS_MONITOR_DEFAULT_USTACKSHOTS_TRIGGER;
	}

	if (!PE_parse_boot_argn("disable_exc_resource", &disable_exc_resource,
	    sizeof(disable_exc_resource))) {
		disable_exc_resource = 0;
	}

	if (!PE_parse_boot_argn("task_iomon_limit_mb", &task_iomon_limit_mb, sizeof(task_iomon_limit_mb))) {
		task_iomon_limit_mb = IOMON_DEFAULT_LIMIT;
	}

	if (!PE_parse_boot_argn("task_iomon_interval_secs", &task_iomon_interval_secs, sizeof(task_iomon_interval_secs))) {
		task_iomon_interval_secs = IOMON_DEFAULT_INTERVAL;
	}

	if (!PE_parse_boot_argn("io_telemetry_limit", &io_telemetry_limit, sizeof(io_telemetry_limit))) {
		io_telemetry_limit = IO_TELEMETRY_DEFAULT_LIMIT;
	}

  
                                                                              
                                                                              
                               
   
#if CONFIG_COALITIONS
	assert(task_ledger_template);
#else                        
	init_task_ledgers();
#endif                        

#if TASK_REFERENCE_LEAK_DEBUG
	task_ref_btlog = btlog_create(TASK_REF_NUM_RECORDS, TASK_REF_BTDEPTH, TRUE                                              );
	assert(task_ref_btlog);
#endif

	  
                                             
    
#ifdef __LP64__
	if (task_create_internal(TASK_NULL, NULL, FALSE, TRUE, TRUE, TF_NONE, TPF_NONE, TWF_NONE, &kernel_task) != KERN_SUCCESS)
#else
	if (task_create_internal(TASK_NULL, NULL, FALSE, FALSE, FALSE, TF_NONE, TPF_NONE, TWF_NONE, &kernel_task) != KERN_SUCCESS)
#endif
	{ panic("task_init\n");}

#if defined(HAS_APPLE_PAC)
	kernel_task->rop_pid = ml_default_rop_pid();
	kernel_task->jop_pid = ml_default_jop_pid();
	                                                                                           
	                                                                                                          
	ml_task_set_disable_user_jop(kernel_task, FALSE);
#endif

	vm_map_deallocate(kernel_task->map);
	kernel_task->map = kernel_map;
}

  
                                                             
                                                                 
   
kern_return_t
kernel_task_create(
	__unused task_t         parent_task,
	__unused vm_offset_t            map_base,
	__unused vm_size_t              map_size,
	__unused task_t         *child_task)
{
	return KERN_INVALID_ARGUMENT;
}

kern_return_t
task_create(
	task_t                          parent_task,
	__unused ledger_port_array_t    ledger_ports,
	__unused mach_msg_type_number_t num_ledger_ports,
	__unused boolean_t              inherit_memory,
	__unused task_t                 *child_task)             
{
	if (parent_task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	  
                                                                      
                     
    
	return KERN_FAILURE;
}

kern_return_t
host_security_create_task_token(
	host_security_t                 host_security,
	task_t                          parent_task,
	__unused security_token_t       sec_token,
	__unused audit_token_t          audit_token,
	__unused host_priv_t            host_priv,
	__unused ledger_port_array_t    ledger_ports,
	__unused mach_msg_type_number_t num_ledger_ports,
	__unused boolean_t              inherit_memory,
	__unused task_t                 *child_task)             
{
	if (parent_task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (host_security == HOST_NULL) {
		return KERN_INVALID_SECURITY;
	}

	  
                        
    
	return KERN_FAILURE;
}

  
               
               
  
                 
                                            
                                          
                                                                
                     
                              
                                         
                   
  
           
                                                                  
  
                      
                                                                           
                                                                                          
                                                                                          
                           
  
               
                                                                                     
                                              
  
                       
                                                                                                    
                                                                                                      
                     
  
                
                                                                                              
                                                 
   
void
init_task_ledgers(void)
{
	ledger_template_t t;

	assert(task_ledger_template == NULL);
	assert(kernel_task == TASK_NULL);

#if MACH_ASSERT
	PE_parse_boot_argn("pmap_ledgers_panic",
	    &pmap_ledgers_panic,
	    sizeof(pmap_ledgers_panic));
	PE_parse_boot_argn("pmap_ledgers_panic_leeway",
	    &pmap_ledgers_panic_leeway,
	    sizeof(pmap_ledgers_panic_leeway));
#endif                  

	if ((t = ledger_template_create("Per-task ledger")) == NULL) {
		panic("couldn't create task ledger template");
	}

	task_ledgers.cpu_time = ledger_entry_add(t, "cpu_time", "sched", "ns");
	task_ledgers.tkm_private = ledger_entry_add(t, "tkm_private",
	    "physmem", "bytes");
	task_ledgers.tkm_shared = ledger_entry_add(t, "tkm_shared", "physmem",
	    "bytes");
	task_ledgers.phys_mem = ledger_entry_add(t, "phys_mem", "physmem",
	    "bytes");
	task_ledgers.wired_mem = ledger_entry_add(t, "wired_mem", "physmem",
	    "bytes");
	task_ledgers.internal = ledger_entry_add(t, "internal", "physmem",
	    "bytes");
	task_ledgers.iokit_mapped = ledger_entry_add(t, "iokit_mapped", "mappings",
	    "bytes");
	task_ledgers.alternate_accounting = ledger_entry_add(t, "alternate_accounting", "physmem",
	    "bytes");
	task_ledgers.alternate_accounting_compressed = ledger_entry_add(t, "alternate_accounting_compressed", "physmem",
	    "bytes");
	task_ledgers.page_table = ledger_entry_add(t, "page_table", "physmem",
	    "bytes");
	task_ledgers.phys_footprint = ledger_entry_add(t, "phys_footprint", "physmem",
	    "bytes");
	task_ledgers.internal_compressed = ledger_entry_add(t, "internal_compressed", "physmem",
	    "bytes");
	task_ledgers.purgeable_volatile = ledger_entry_add(t, "purgeable_volatile", "physmem", "bytes");
	task_ledgers.purgeable_nonvolatile = ledger_entry_add(t, "purgeable_nonvolatile", "physmem", "bytes");
	task_ledgers.purgeable_volatile_compressed = ledger_entry_add(t, "purgeable_volatile_compress", "physmem", "bytes");
	task_ledgers.purgeable_nonvolatile_compressed = ledger_entry_add(t, "purgeable_nonvolatile_compress", "physmem", "bytes");
#if DEBUG || DEVELOPMENT
	task_ledgers.pages_grabbed = ledger_entry_add(t, "pages_grabbed", "physmem", "count");
	task_ledgers.pages_grabbed_kern = ledger_entry_add(t, "pages_grabbed_kern", "physmem", "count");
	task_ledgers.pages_grabbed_iopl = ledger_entry_add(t, "pages_grabbed_iopl", "physmem", "count");
	task_ledgers.pages_grabbed_upl = ledger_entry_add(t, "pages_grabbed_upl", "physmem", "count");
#endif
	task_ledgers.tagged_nofootprint = ledger_entry_add(t, "tagged_nofootprint", "physmem", "bytes");
	task_ledgers.tagged_footprint = ledger_entry_add(t, "tagged_footprint", "physmem", "bytes");
	task_ledgers.tagged_nofootprint_compressed = ledger_entry_add(t, "tagged_nofootprint_compressed", "physmem", "bytes");
	task_ledgers.tagged_footprint_compressed = ledger_entry_add(t, "tagged_footprint_compressed", "physmem", "bytes");
	task_ledgers.network_volatile = ledger_entry_add(t, "network_volatile", "physmem", "bytes");
	task_ledgers.network_nonvolatile = ledger_entry_add(t, "network_nonvolatile", "physmem", "bytes");
	task_ledgers.network_volatile_compressed = ledger_entry_add(t, "network_volatile_compressed", "physmem", "bytes");
	task_ledgers.network_nonvolatile_compressed = ledger_entry_add(t, "network_nonvolatile_compressed", "physmem", "bytes");
	task_ledgers.media_nofootprint = ledger_entry_add(t, "media_nofootprint", "physmem", "bytes");
	task_ledgers.media_footprint = ledger_entry_add(t, "media_footprint", "physmem", "bytes");
	task_ledgers.media_nofootprint_compressed = ledger_entry_add(t, "media_nofootprint_compressed", "physmem", "bytes");
	task_ledgers.media_footprint_compressed = ledger_entry_add(t, "media_footprint_compressed", "physmem", "bytes");
	task_ledgers.graphics_nofootprint = ledger_entry_add(t, "graphics_nofootprint", "physmem", "bytes");
	task_ledgers.graphics_footprint = ledger_entry_add(t, "graphics_footprint", "physmem", "bytes");
	task_ledgers.graphics_nofootprint_compressed = ledger_entry_add(t, "graphics_nofootprint_compressed", "physmem", "bytes");
	task_ledgers.graphics_footprint_compressed = ledger_entry_add(t, "graphics_footprint_compressed", "physmem", "bytes");
	task_ledgers.neural_nofootprint = ledger_entry_add(t, "neural_nofootprint", "physmem", "bytes");
	task_ledgers.neural_footprint = ledger_entry_add(t, "neural_footprint", "physmem", "bytes");
	task_ledgers.neural_nofootprint_compressed = ledger_entry_add(t, "neural_nofootprint_compressed", "physmem", "bytes");
	task_ledgers.neural_footprint_compressed = ledger_entry_add(t, "neural_footprint_compressed", "physmem", "bytes");

#if CONFIG_FREEZE
	task_ledgers.frozen_to_swap = ledger_entry_add(t, "frozen_to_swap", "physmem", "bytes");
#endif                    

	task_ledgers.platform_idle_wakeups = ledger_entry_add(t, "platform_idle_wakeups", "power",
	    "count");
	task_ledgers.interrupt_wakeups = ledger_entry_add(t, "interrupt_wakeups", "power",
	    "count");

#if CONFIG_SCHED_SFI
	sfi_class_id_t class_id, ledger_alias;
	for (class_id = SFI_CLASS_UNSPECIFIED; class_id < MAX_SFI_CLASS_ID; class_id++) {
		task_ledgers.sfi_wait_times[class_id] = -1;
	}

	                                   
	for (class_id = SFI_CLASS_UNSPECIFIED + 1; class_id < MAX_SFI_CLASS_ID; class_id++) {
		ledger_alias = sfi_get_ledger_alias_for_class(class_id);
		if (ledger_alias != SFI_CLASS_UNSPECIFIED) {
			                                                   
			if (task_ledgers.sfi_wait_times[ledger_alias] != -1) {
				task_ledgers.sfi_wait_times[class_id] = task_ledgers.sfi_wait_times[ledger_alias];
			} else {
				                                    
				task_ledgers.sfi_wait_times[class_id] = task_ledgers.sfi_wait_times[ledger_alias] = sfi_ledger_entry_add(t, ledger_alias);
			}
		} else {
			task_ledgers.sfi_wait_times[class_id] = sfi_ledger_entry_add(t, class_id);
		}

		if (task_ledgers.sfi_wait_times[class_id] < 0) {
			panic("couldn't create entries for task ledger template for SFI class 0x%x", class_id);
		}
	}

	assert(task_ledgers.sfi_wait_times[MAX_SFI_CLASS_ID - 1] != -1);
#endif                       

	task_ledgers.cpu_time_billed_to_me = ledger_entry_add(t, "cpu_time_billed_to_me", "sched", "ns");
	task_ledgers.cpu_time_billed_to_others = ledger_entry_add(t, "cpu_time_billed_to_others", "sched", "ns");
	task_ledgers.physical_writes = ledger_entry_add(t, "physical_writes", "res", "bytes");
	task_ledgers.logical_writes = ledger_entry_add(t, "logical_writes", "res", "bytes");
	task_ledgers.logical_writes_to_external = ledger_entry_add(t, "logical_writes_to_external", "res", "bytes");
#if CONFIG_PHYS_WRITE_ACCT
	task_ledgers.fs_metadata_writes = ledger_entry_add(t, "fs_metadata_writes", "res", "bytes");
#endif                             
	task_ledgers.energy_billed_to_me = ledger_entry_add(t, "energy_billed_to_me", "power", "nj");
	task_ledgers.energy_billed_to_others = ledger_entry_add(t, "energy_billed_to_others", "power", "nj");

	if ((task_ledgers.cpu_time < 0) ||
	    (task_ledgers.tkm_private < 0) ||
	    (task_ledgers.tkm_shared < 0) ||
	    (task_ledgers.phys_mem < 0) ||
	    (task_ledgers.wired_mem < 0) ||
	    (task_ledgers.internal < 0) ||
	    (task_ledgers.iokit_mapped < 0) ||
	    (task_ledgers.alternate_accounting < 0) ||
	    (task_ledgers.alternate_accounting_compressed < 0) ||
	    (task_ledgers.page_table < 0) ||
	    (task_ledgers.phys_footprint < 0) ||
	    (task_ledgers.internal_compressed < 0) ||
	    (task_ledgers.purgeable_volatile < 0) ||
	    (task_ledgers.purgeable_nonvolatile < 0) ||
	    (task_ledgers.purgeable_volatile_compressed < 0) ||
	    (task_ledgers.purgeable_nonvolatile_compressed < 0) ||
	    (task_ledgers.tagged_nofootprint < 0) ||
	    (task_ledgers.tagged_footprint < 0) ||
	    (task_ledgers.tagged_nofootprint_compressed < 0) ||
	    (task_ledgers.tagged_footprint_compressed < 0) ||
#if CONFIG_FREEZE
	    (task_ledgers.frozen_to_swap < 0) ||
#endif                    
	    (task_ledgers.network_volatile < 0) ||
	    (task_ledgers.network_nonvolatile < 0) ||
	    (task_ledgers.network_volatile_compressed < 0) ||
	    (task_ledgers.network_nonvolatile_compressed < 0) ||
	    (task_ledgers.media_nofootprint < 0) ||
	    (task_ledgers.media_footprint < 0) ||
	    (task_ledgers.media_nofootprint_compressed < 0) ||
	    (task_ledgers.media_footprint_compressed < 0) ||
	    (task_ledgers.graphics_nofootprint < 0) ||
	    (task_ledgers.graphics_footprint < 0) ||
	    (task_ledgers.graphics_nofootprint_compressed < 0) ||
	    (task_ledgers.graphics_footprint_compressed < 0) ||
	    (task_ledgers.neural_nofootprint < 0) ||
	    (task_ledgers.neural_footprint < 0) ||
	    (task_ledgers.neural_nofootprint_compressed < 0) ||
	    (task_ledgers.neural_footprint_compressed < 0) ||
	    (task_ledgers.platform_idle_wakeups < 0) ||
	    (task_ledgers.interrupt_wakeups < 0) ||
	    (task_ledgers.cpu_time_billed_to_me < 0) || (task_ledgers.cpu_time_billed_to_others < 0) ||
	    (task_ledgers.physical_writes < 0) ||
	    (task_ledgers.logical_writes < 0) ||
	    (task_ledgers.logical_writes_to_external < 0) ||
#if CONFIG_PHYS_WRITE_ACCT
	    (task_ledgers.fs_metadata_writes < 0) ||
#endif                             
	    (task_ledgers.energy_billed_to_me < 0) ||
	    (task_ledgers.energy_billed_to_others < 0)
	    ) {
		panic("couldn't create entries for task ledger template");
	}

	ledger_track_credit_only(t, task_ledgers.phys_footprint);
	ledger_track_credit_only(t, task_ledgers.page_table);
	ledger_track_credit_only(t, task_ledgers.internal);
	ledger_track_credit_only(t, task_ledgers.internal_compressed);
	ledger_track_credit_only(t, task_ledgers.iokit_mapped);
	ledger_track_credit_only(t, task_ledgers.alternate_accounting);
	ledger_track_credit_only(t, task_ledgers.alternate_accounting_compressed);
	ledger_track_credit_only(t, task_ledgers.purgeable_volatile);
	ledger_track_credit_only(t, task_ledgers.purgeable_nonvolatile);
	ledger_track_credit_only(t, task_ledgers.purgeable_volatile_compressed);
	ledger_track_credit_only(t, task_ledgers.purgeable_nonvolatile_compressed);
#if DEBUG || DEVELOPMENT
	ledger_track_credit_only(t, task_ledgers.pages_grabbed);
	ledger_track_credit_only(t, task_ledgers.pages_grabbed_kern);
	ledger_track_credit_only(t, task_ledgers.pages_grabbed_iopl);
	ledger_track_credit_only(t, task_ledgers.pages_grabbed_upl);
#endif

	ledger_track_credit_only(t, task_ledgers.tagged_nofootprint);
	ledger_track_credit_only(t, task_ledgers.tagged_footprint);
	ledger_track_credit_only(t, task_ledgers.tagged_nofootprint_compressed);
	ledger_track_credit_only(t, task_ledgers.tagged_footprint_compressed);
	ledger_track_credit_only(t, task_ledgers.network_volatile);
	ledger_track_credit_only(t, task_ledgers.network_nonvolatile);
	ledger_track_credit_only(t, task_ledgers.network_volatile_compressed);
	ledger_track_credit_only(t, task_ledgers.network_nonvolatile_compressed);
	ledger_track_credit_only(t, task_ledgers.media_nofootprint);
	ledger_track_credit_only(t, task_ledgers.media_footprint);
	ledger_track_credit_only(t, task_ledgers.media_nofootprint_compressed);
	ledger_track_credit_only(t, task_ledgers.media_footprint_compressed);
	ledger_track_credit_only(t, task_ledgers.graphics_nofootprint);
	ledger_track_credit_only(t, task_ledgers.graphics_footprint);
	ledger_track_credit_only(t, task_ledgers.graphics_nofootprint_compressed);
	ledger_track_credit_only(t, task_ledgers.graphics_footprint_compressed);
	ledger_track_credit_only(t, task_ledgers.neural_nofootprint);
	ledger_track_credit_only(t, task_ledgers.neural_footprint);
	ledger_track_credit_only(t, task_ledgers.neural_nofootprint_compressed);
	ledger_track_credit_only(t, task_ledgers.neural_footprint_compressed);

	ledger_track_maximum(t, task_ledgers.phys_footprint, 60);
#if MACH_ASSERT
	if (pmap_ledgers_panic) {
		ledger_panic_on_negative(t, task_ledgers.phys_footprint);
		ledger_panic_on_negative(t, task_ledgers.page_table);
		ledger_panic_on_negative(t, task_ledgers.internal);
		ledger_panic_on_negative(t, task_ledgers.internal_compressed);
		ledger_panic_on_negative(t, task_ledgers.iokit_mapped);
		ledger_panic_on_negative(t, task_ledgers.alternate_accounting);
		ledger_panic_on_negative(t, task_ledgers.alternate_accounting_compressed);
		ledger_panic_on_negative(t, task_ledgers.purgeable_volatile);
		ledger_panic_on_negative(t, task_ledgers.purgeable_nonvolatile);
		ledger_panic_on_negative(t, task_ledgers.purgeable_volatile_compressed);
		ledger_panic_on_negative(t, task_ledgers.purgeable_nonvolatile_compressed);
#if CONFIG_PHYS_WRITE_ACCT
		ledger_panic_on_negative(t, task_ledgers.fs_metadata_writes);
#endif                             

		ledger_panic_on_negative(t, task_ledgers.tagged_nofootprint);
		ledger_panic_on_negative(t, task_ledgers.tagged_footprint);
		ledger_panic_on_negative(t, task_ledgers.tagged_nofootprint_compressed);
		ledger_panic_on_negative(t, task_ledgers.tagged_footprint_compressed);
		ledger_panic_on_negative(t, task_ledgers.network_volatile);
		ledger_panic_on_negative(t, task_ledgers.network_nonvolatile);
		ledger_panic_on_negative(t, task_ledgers.network_volatile_compressed);
		ledger_panic_on_negative(t, task_ledgers.network_nonvolatile_compressed);
		ledger_panic_on_negative(t, task_ledgers.media_nofootprint);
		ledger_panic_on_negative(t, task_ledgers.media_footprint);
		ledger_panic_on_negative(t, task_ledgers.media_nofootprint_compressed);
		ledger_panic_on_negative(t, task_ledgers.media_footprint_compressed);
		ledger_panic_on_negative(t, task_ledgers.graphics_nofootprint);
		ledger_panic_on_negative(t, task_ledgers.graphics_footprint);
		ledger_panic_on_negative(t, task_ledgers.graphics_nofootprint_compressed);
		ledger_panic_on_negative(t, task_ledgers.graphics_footprint_compressed);
		ledger_panic_on_negative(t, task_ledgers.neural_nofootprint);
		ledger_panic_on_negative(t, task_ledgers.neural_footprint);
		ledger_panic_on_negative(t, task_ledgers.neural_nofootprint_compressed);
		ledger_panic_on_negative(t, task_ledgers.neural_footprint_compressed);
	}
#endif                  

#if CONFIG_MEMORYSTATUS
	ledger_set_callback(t, task_ledgers.phys_footprint, task_footprint_exceeded, NULL, NULL);
#endif                          

	ledger_set_callback(t, task_ledgers.interrupt_wakeups,
	    task_wakeups_rate_exceeded, NULL, NULL);
	ledger_set_callback(t, task_ledgers.physical_writes, task_io_rate_exceeded, (void *)FLAVOR_IO_PHYSICAL_WRITES, NULL);

#if XNU_MONITOR
	ledger_template_complete_secure_alloc(t);
#else                  
	ledger_template_complete(t);
#endif                  
	task_ledger_template = t;
}

os_refgrp_decl(static, task_refgrp, "task", NULL);

kern_return_t
task_create_internal(
	task_t          parent_task,
	coalition_t     *parent_coalitions __unused,
	boolean_t       inherit_memory,
	__unused boolean_t      is_64bit,
	boolean_t is_64bit_data,
	uint32_t        t_flags,
	uint32_t        t_procflags,
	uint8_t         t_returnwaitflags,
	task_t          *child_task)                     
{
	task_t                  new_task;
	vm_shared_region_t      shared_region;
	ledger_t                ledger = NULL;

	new_task = (task_t) zalloc(task_zone);

	if (new_task == TASK_NULL) {
		return KERN_RESOURCE_SHORTAGE;
	}

	                                                      
	os_ref_init_count(&new_task->ref_count, &task_refgrp, 2);

	                                  
	assert(task_ledger_template != NULL);
	if ((ledger = ledger_instantiate(task_ledger_template,
	    LEDGER_CREATE_ACTIVE_ENTRIES)) == NULL) {
		zfree(task_zone, new_task);
		return KERN_RESOURCE_SHORTAGE;
	}

	counter_alloc(&(new_task->faults));

#if defined(HAS_APPLE_PAC)
	ml_task_set_rop_pid(new_task, parent_task, inherit_memory);
	ml_task_set_jop_pid(new_task, parent_task, inherit_memory);
	ml_task_set_disable_user_jop(new_task, inherit_memory ? parent_task->disable_user_jop : FALSE);
#endif


	new_task->ledger = ledger;

#if defined(CONFIG_SCHED_MULTIQ)
	new_task->sched_group = sched_group_create();
#endif

	                                                             
	if (!(t_flags & TF_CORPSE_FORK) && inherit_memory) {
		new_task->map = vm_map_fork(ledger, parent_task->map, 0);
	} else {
		unsigned int pmap_flags = is_64bit ? PMAP_CREATE_64BIT : 0;
		new_task->map = vm_map_create(pmap_create_options(ledger, 0, pmap_flags),
		    (vm_map_offset_t)(VM_MIN_ADDRESS),
		    (vm_map_offset_t)(VM_MAX_ADDRESS), TRUE);
	}

	                                       
	if (parent_task) {
		vm_map_set_user_wire_limit(new_task->map, (vm_size_t)parent_task->map->user_wire_limit);
	}

	lck_mtx_init(&new_task->lock, &task_lck_grp, &task_lck_attr);
	queue_init(&new_task->threads);
	new_task->suspend_count = 0;
	new_task->thread_count = 0;
	new_task->active_thread_count = 0;
	new_task->user_stop_count = 0;
	new_task->legacy_stop_count = 0;
	new_task->active = TRUE;
	new_task->halting = FALSE;
	new_task->priv_flags = 0;
	new_task->t_flags = t_flags;
	new_task->t_procflags = t_procflags;
	new_task->t_returnwaitflags = t_returnwaitflags;
	new_task->returnwait_inheritor = current_thread();
	new_task->importance = 0;
	new_task->crashed_thread_id = 0;
	new_task->exec_token = 0;
	new_task->watchports = NULL;
	new_task->restartable_ranges = NULL;
	new_task->task_exc_guard = 0;

	new_task->bank_context = NULL;

#ifdef MACH_BSD
	new_task->bsd_info = NULL;
	new_task->corpse_info = NULL;
#endif               

#if CONFIG_MACF
	new_task->crash_label = NULL;

	new_task->mach_trap_filter_mask = NULL;
	new_task->mach_kobj_filter_mask = NULL;
#endif

#if CONFIG_MEMORYSTATUS
	if (max_task_footprint != 0) {
		ledger_set_limit(ledger, task_ledgers.phys_footprint, max_task_footprint, PHYS_FOOTPRINT_WARNING_LEVEL);
	}
#endif                          

	if (task_wakeups_monitor_rate != 0) {
		uint32_t flags = WAKEMON_ENABLE | WAKEMON_SET_DEFAULTS;
		int32_t  rate;                                           
		task_wakeups_monitor_ctl(new_task, &flags, &rate);
	}

#if CONFIG_IO_ACCOUNTING
	uint32_t flags = IOMON_ENABLE;
	task_io_monitor_ctl(new_task, &flags);
#endif                           

	machine_task_init(new_task, parent_task, inherit_memory);

	new_task->task_debug = NULL;

#if DEVELOPMENT || DEBUG
	new_task->task_unnested = FALSE;
	new_task->task_disconnected_count = 0;
#endif
	queue_init(&new_task->semaphore_list);
	new_task->semaphores_owned = 0;

	ipc_task_init(new_task, parent_task);

	new_task->vtimers = 0;

	new_task->shared_region = NULL;

	new_task->affinity_space = NULL;

	new_task->t_kpc = 0;

	new_task->pidsuspended = FALSE;
	new_task->frozen = FALSE;
	new_task->changing_freeze_state = FALSE;
	new_task->rusage_cpu_flags = 0;
	new_task->rusage_cpu_percentage = 0;
	new_task->rusage_cpu_interval = 0;
	new_task->rusage_cpu_deadline = 0;
	new_task->rusage_cpu_callt = NULL;
#if MACH_ASSERT
	new_task->suspends_outstanding = 0;
#endif

#if HYPERVISOR
	new_task->hv_task_target = NULL;
#endif                 

#if CONFIG_TASKWATCH
	queue_init(&new_task->task_watchers);
	new_task->num_taskwatchers  = 0;
	new_task->watchapplying  = 0;
#endif                       

	new_task->mem_notify_reserved = 0;
	new_task->memlimit_attrs_reserved = 0;

	new_task->requested_policy = default_task_requested_policy;
	new_task->effective_policy = default_task_effective_policy;

	new_task->task_shared_region_slide = -1;

	task_importance_init_from_parent(new_task, parent_task);

	if (parent_task != TASK_NULL) {
		new_task->sec_token = parent_task->sec_token;
		new_task->audit_token = parent_task->audit_token;

		                                        
		shared_region = vm_shared_region_get(parent_task);
		vm_shared_region_set(new_task, shared_region);

#if __has_feature(ptrauth_calls)
		                                   
		char *shared_region_id = task_get_vm_shared_region_id_and_jop_pid(parent_task, NULL);
		if (shared_region_id != NULL) {
			shared_region_key_alloc(shared_region_id, FALSE, 0);                        
		}
		task_set_shared_region_id(new_task, shared_region_id);
#endif                                   

		if (task_has_64Bit_addr(parent_task)) {
			task_set_64Bit_addr(new_task);
		}

		if (task_has_64Bit_data(parent_task)) {
			task_set_64Bit_data(new_task);
		}

		new_task->all_image_info_addr = parent_task->all_image_info_addr;
		new_task->all_image_info_size = parent_task->all_image_info_size;
		new_task->mach_header_vm_address = 0;

		if (inherit_memory && parent_task->affinity_space) {
			task_affinity_create(parent_task, new_task);
		}

		new_task->pset_hint = parent_task->pset_hint = task_choose_pset(parent_task);

		if (parent_task->t_flags & TF_NO_SMT) {
			new_task->t_flags |= TF_NO_SMT;
		}

		if (parent_task->t_flags & TF_TECS) {
			new_task->t_flags |= TF_TECS;
		}

		if (parent_task->t_flags & TF_FILTER_MSG) {
			new_task->t_flags |= TF_FILTER_MSG;
		}

#if defined(__x86_64__)
		if (parent_task->t_flags & TF_INSN_COPY_OPTOUT) {
			new_task->t_flags |= TF_INSN_COPY_OPTOUT;
		}
#endif
		new_task->priority = BASEPRI_DEFAULT;
		new_task->max_priority = MAXPRI_USER;

		task_policy_create(new_task, parent_task);
	} else {
		new_task->sec_token = KERNEL_SECURITY_TOKEN;
		new_task->audit_token = KERNEL_AUDIT_TOKEN;
#ifdef __LP64__
		if (is_64bit) {
			task_set_64Bit_addr(new_task);
		}
#endif

		if (is_64bit_data) {
			task_set_64Bit_data(new_task);
		}

		new_task->all_image_info_addr = (mach_vm_address_t)0;
		new_task->all_image_info_size = (mach_vm_size_t)0;

		new_task->pset_hint = PROCESSOR_SET_NULL;

		if (kernel_task == TASK_NULL) {
			new_task->priority = BASEPRI_KERNEL;
			new_task->max_priority = MAXPRI_KERNEL;
		} else {
			new_task->priority = BASEPRI_DEFAULT;
			new_task->max_priority = MAXPRI_USER;
		}
	}

	bzero(new_task->coalition, sizeof(new_task->coalition));
	for (int i = 0; i < COALITION_NUM_TYPES; i++) {
		queue_chain_init(new_task->task_coalition[i]);
	}

	                             
	new_task->task_io_stats = kheap_alloc(KHEAP_DATA_BUFFERS,
	    sizeof(struct io_stat_info), Z_WAITOK | Z_ZERO);
	assert(new_task->task_io_stats != NULL);

	bzero(&(new_task->cpu_time_eqos_stats), sizeof(new_task->cpu_time_eqos_stats));
	bzero(&(new_task->cpu_time_rqos_stats), sizeof(new_task->cpu_time_rqos_stats));

	bzero(&new_task->extmod_statistics, sizeof(new_task->extmod_statistics));

	                                                                
	if (parent_task != NULL && (t_flags & TF_CORPSE_FORK)) {
		task_rollup_accounting_info(new_task, parent_task);
	} else {
		                                                     
		new_task->total_user_time = 0;
		new_task->total_system_time = 0;
		new_task->total_ptime = 0;
		new_task->total_runnable_time = 0;
		new_task->pageins = 0;
		new_task->cow_faults = 0;
		new_task->messages_sent = 0;
		new_task->messages_received = 0;
		new_task->syscalls_mach = 0;
		new_task->syscalls_unix = 0;
		new_task->c_switch = 0;
		new_task->p_switch = 0;
		new_task->ps_switch = 0;
		new_task->decompressions = 0;
		new_task->low_mem_notified_warn = 0;
		new_task->low_mem_notified_critical = 0;
		new_task->purged_memory_warn = 0;
		new_task->purged_memory_critical = 0;
		new_task->low_mem_privileged_listener = 0;
		new_task->memlimit_is_active = 0;
		new_task->memlimit_is_fatal = 0;
		new_task->memlimit_active_exc_resource = 0;
		new_task->memlimit_inactive_exc_resource = 0;
		new_task->task_timer_wakeups_bin_1 = 0;
		new_task->task_timer_wakeups_bin_2 = 0;
		new_task->task_gpu_ns = 0;
		new_task->task_writes_counters_internal.task_immediate_writes = 0;
		new_task->task_writes_counters_internal.task_deferred_writes = 0;
		new_task->task_writes_counters_internal.task_invalidated_writes = 0;
		new_task->task_writes_counters_internal.task_metadata_writes = 0;
		new_task->task_writes_counters_external.task_immediate_writes = 0;
		new_task->task_writes_counters_external.task_deferred_writes = 0;
		new_task->task_writes_counters_external.task_invalidated_writes = 0;
		new_task->task_writes_counters_external.task_metadata_writes = 0;
#if CONFIG_PHYS_WRITE_ACCT
		new_task->task_fs_metadata_writes = 0;
#endif                             

		new_task->task_energy = 0;
#if MONOTONIC
		memset(&new_task->task_monotonic, 0, sizeof(new_task->task_monotonic));
#endif                
	}


#if CONFIG_COALITIONS
	if (!(t_flags & TF_CORPSE_FORK)) {
		                                                     
		if (parent_coalitions && parent_coalitions[COALITION_TYPE_RESOURCE]) {
			coalitions_adopt_task(parent_coalitions, new_task);
		} else if (parent_task && parent_task->coalition[COALITION_TYPE_RESOURCE]) {
			  
                                                      
                                                       
                             
      
			coalitions_adopt_task(parent_task->coalition, new_task);
		} else {
			                                                        
			coalitions_adopt_init_task(new_task);
		}
		  
                                                              
                                       
     
		if (parent_task && (t_procflags & TPF_EXEC_COPY)) {
			int coal_roles[COALITION_NUM_TYPES];
			task_coalition_roles(parent_task, coal_roles);
			(void)coalitions_set_roles(new_task->coalition, new_task, coal_roles);
		}
	} else {
		coalitions_adopt_corpse_task(new_task);
	}

	if (new_task->coalition[COALITION_TYPE_RESOURCE] == COALITION_NULL) {
		panic("created task is not a member of a resource coalition");
	}
#endif                        

	new_task->dispatchqueue_offset = 0;
	if (parent_task != NULL) {
		new_task->dispatchqueue_offset = parent_task->dispatchqueue_offset;
	}

	new_task->task_can_transfer_memory_ownership = FALSE;
	new_task->task_volatile_objects = 0;
	new_task->task_nonvolatile_objects = 0;
	new_task->task_objects_disowning = FALSE;
	new_task->task_objects_disowned = FALSE;
	new_task->task_owned_objects = 0;
	queue_init(&new_task->task_objq);

#if CONFIG_FREEZE
	queue_init(&new_task->task_frozen_cseg_q);
#endif                    

	task_objq_lock_init(new_task);

#if __arm64__
	new_task->task_legacy_footprint = FALSE;
	new_task->task_extra_footprint_limit = FALSE;
	new_task->task_ios13extended_footprint_limit = FALSE;
#endif                
	new_task->task_region_footprint = FALSE;
	new_task->task_has_crossed_thread_limit = FALSE;
	new_task->task_thread_limit = 0;
#if CONFIG_SECLUDED_MEMORY
	new_task->task_can_use_secluded_mem = FALSE;
	new_task->task_could_use_secluded_mem = FALSE;
	new_task->task_could_also_use_secluded_mem = FALSE;
	new_task->task_suppressed_secluded = FALSE;
#endif                             

	  
                                               
                                            
                                               
    
	new_task->t_flags &= ~(TF_DARKWAKE_MODE);

	queue_init(&new_task->io_user_clients);
	new_task->loadTag = 0;

	ipc_task_enable(new_task);

	lck_mtx_lock(&tasks_threads_lock);
	queue_enter(&tasks, new_task, task_t, tasks);
	tasks_count++;
	if (tasks_suspend_state) {
		task_suspend_internal(new_task);
	}
	lck_mtx_unlock(&tasks_threads_lock);

	*child_task = new_task;
	return KERN_SUCCESS;
}

  
                              
  
                                                 
                                      
   
void
task_rollup_accounting_info(task_t to_task, task_t from_task)
{
	assert(from_task != to_task);

	to_task->total_user_time = from_task->total_user_time;
	to_task->total_system_time = from_task->total_system_time;
	to_task->total_ptime = from_task->total_ptime;
	to_task->total_runnable_time = from_task->total_runnable_time;
	counter_add(&to_task->faults, counter_load(&from_task->faults));
	to_task->pageins = from_task->pageins;
	to_task->cow_faults = from_task->cow_faults;
	to_task->decompressions = from_task->decompressions;
	to_task->messages_sent = from_task->messages_sent;
	to_task->messages_received = from_task->messages_received;
	to_task->syscalls_mach = from_task->syscalls_mach;
	to_task->syscalls_unix = from_task->syscalls_unix;
	to_task->c_switch = from_task->c_switch;
	to_task->p_switch = from_task->p_switch;
	to_task->ps_switch = from_task->ps_switch;
	to_task->extmod_statistics = from_task->extmod_statistics;
	to_task->low_mem_notified_warn = from_task->low_mem_notified_warn;
	to_task->low_mem_notified_critical = from_task->low_mem_notified_critical;
	to_task->purged_memory_warn = from_task->purged_memory_warn;
	to_task->purged_memory_critical = from_task->purged_memory_critical;
	to_task->low_mem_privileged_listener = from_task->low_mem_privileged_listener;
	*to_task->task_io_stats = *from_task->task_io_stats;
	to_task->cpu_time_eqos_stats = from_task->cpu_time_eqos_stats;
	to_task->cpu_time_rqos_stats = from_task->cpu_time_rqos_stats;
	to_task->task_timer_wakeups_bin_1 = from_task->task_timer_wakeups_bin_1;
	to_task->task_timer_wakeups_bin_2 = from_task->task_timer_wakeups_bin_2;
	to_task->task_gpu_ns = from_task->task_gpu_ns;
	to_task->task_writes_counters_internal.task_immediate_writes = from_task->task_writes_counters_internal.task_immediate_writes;
	to_task->task_writes_counters_internal.task_deferred_writes = from_task->task_writes_counters_internal.task_deferred_writes;
	to_task->task_writes_counters_internal.task_invalidated_writes = from_task->task_writes_counters_internal.task_invalidated_writes;
	to_task->task_writes_counters_internal.task_metadata_writes = from_task->task_writes_counters_internal.task_metadata_writes;
	to_task->task_writes_counters_external.task_immediate_writes = from_task->task_writes_counters_external.task_immediate_writes;
	to_task->task_writes_counters_external.task_deferred_writes = from_task->task_writes_counters_external.task_deferred_writes;
	to_task->task_writes_counters_external.task_invalidated_writes = from_task->task_writes_counters_external.task_invalidated_writes;
	to_task->task_writes_counters_external.task_metadata_writes = from_task->task_writes_counters_external.task_metadata_writes;
#if CONFIG_PHYS_WRITE_ACCT
	to_task->task_fs_metadata_writes = from_task->task_fs_metadata_writes;
#endif                             
	to_task->task_energy = from_task->task_energy;

	                                                       
	ledger_rollup_entry(to_task->ledger, from_task->ledger, task_ledgers.cpu_time);
	ledger_rollup_entry(to_task->ledger, from_task->ledger, task_ledgers.platform_idle_wakeups);
	ledger_rollup_entry(to_task->ledger, from_task->ledger, task_ledgers.interrupt_wakeups);
#if CONFIG_SCHED_SFI
	for (sfi_class_id_t class_id = SFI_CLASS_UNSPECIFIED; class_id < MAX_SFI_CLASS_ID; class_id++) {
		ledger_rollup_entry(to_task->ledger, from_task->ledger, task_ledgers.sfi_wait_times[class_id]);
	}
#endif
	ledger_rollup_entry(to_task->ledger, from_task->ledger, task_ledgers.cpu_time_billed_to_me);
	ledger_rollup_entry(to_task->ledger, from_task->ledger, task_ledgers.cpu_time_billed_to_others);
	ledger_rollup_entry(to_task->ledger, from_task->ledger, task_ledgers.physical_writes);
	ledger_rollup_entry(to_task->ledger, from_task->ledger, task_ledgers.logical_writes);
	ledger_rollup_entry(to_task->ledger, from_task->ledger, task_ledgers.energy_billed_to_me);
	ledger_rollup_entry(to_task->ledger, from_task->ledger, task_ledgers.energy_billed_to_others);
}

int task_dropped_imp_count = 0;

  
                   
  
                              
   
void
task_deallocate(
	task_t          task)
{
	ledger_amount_t credit, debit, interrupt_wakeups, platform_idle_wakeups;
	os_ref_count_t refs;

	if (task == TASK_NULL) {
		return;
	}

	refs = task_deallocate_internal(task);

#if IMPORTANCE_INHERITANCE
	if (refs == 1) {
		  
                                                              
                                                           
                                                            
                                                              
                                                     
     
		if (IIT_NULL != task->task_imp_base) {
			ipc_importance_disconnect_task(task);
		}
		return;
	}
#endif                             

	if (refs > 0) {
		return;
	}

	  
                                                                 
                                                     
    
	assert(queue_empty(&task->threads));
	assert(task->bsd_info == NULL);
	assert(!is_active(task->itk_space));
	assert(!task->active);
	assert(task->active_thread_count == 0);

	lck_mtx_lock(&tasks_threads_lock);
	assert(terminated_tasks_count > 0);
	queue_remove(&terminated_tasks, task, task_t, tasks);
	terminated_tasks_count--;
	lck_mtx_unlock(&tasks_threads_lock);

	  
                                        
    
	task_bank_reset(task);

	if (task->task_io_stats) {
		kheap_free(KHEAP_DATA_BUFFERS, task->task_io_stats,
		    sizeof(struct io_stat_info));
	}

	  
                                            
                                           
             
    
	machine_task_terminate(task);

	ipc_task_terminate(task);

	                    
	iokit_task_terminate(task);

	if (task->affinity_space) {
		task_affinity_deallocate(task);
	}

#if MACH_ASSERT
	if (task->ledger != NULL &&
	    task->map != NULL &&
	    task->map->pmap != NULL &&
	    task->map->pmap->ledger != NULL) {
		assert(task->ledger == task->map->pmap->ledger);
	}
#endif                  

	vm_owned_objects_disown(task);
	assert(task->task_objects_disowned);
	if (task->task_volatile_objects != 0 ||
	    task->task_nonvolatile_objects != 0 ||
	    task->task_owned_objects != 0) {
		panic("task_deallocate(%p): "
		    "volatile_objects=%d nonvolatile_objects=%d owned=%d\n",
		    task,
		    task->task_volatile_objects,
		    task->task_nonvolatile_objects,
		    task->task_owned_objects);
	}

	vm_map_deallocate(task->map);
	is_release(task->itk_space);
	if (task->restartable_ranges) {
		restartable_ranges_release(task->restartable_ranges);
	}

	ledger_get_entries(task->ledger, task_ledgers.interrupt_wakeups,
	    &interrupt_wakeups, &debit);
	ledger_get_entries(task->ledger, task_ledgers.platform_idle_wakeups,
	    &platform_idle_wakeups, &debit);

#if defined(CONFIG_SCHED_MULTIQ)
	sched_group_destroy(task->sched_group);
#endif

	                                          
	lck_spin_lock(&dead_task_statistics_lock);
	dead_task_statistics.total_user_time += task->total_user_time;
	dead_task_statistics.total_system_time += task->total_system_time;

	dead_task_statistics.task_interrupt_wakeups += interrupt_wakeups;
	dead_task_statistics.task_platform_idle_wakeups += platform_idle_wakeups;

	dead_task_statistics.task_timer_wakeups_bin_1 += task->task_timer_wakeups_bin_1;
	dead_task_statistics.task_timer_wakeups_bin_2 += task->task_timer_wakeups_bin_2;
	dead_task_statistics.total_ptime += task->total_ptime;
	dead_task_statistics.total_pset_switches += task->ps_switch;
	dead_task_statistics.task_gpu_ns += task->task_gpu_ns;
	dead_task_statistics.task_energy += task->task_energy;

	lck_spin_unlock(&dead_task_statistics_lock);
	lck_mtx_destroy(&task->lock, &task_lck_grp);

	if (!ledger_get_entries(task->ledger, task_ledgers.tkm_private, &credit,
	    &debit)) {
		OSAddAtomic64(credit, (int64_t *)&tasks_tkm_private.alloc);
		OSAddAtomic64(debit, (int64_t *)&tasks_tkm_private.free);
	}
	if (!ledger_get_entries(task->ledger, task_ledgers.tkm_shared, &credit,
	    &debit)) {
		OSAddAtomic64(credit, (int64_t *)&tasks_tkm_shared.alloc);
		OSAddAtomic64(debit, (int64_t *)&tasks_tkm_shared.free);
	}
	ledger_dereference(task->ledger);

#if TASK_REFERENCE_LEAK_DEBUG
	btlog_remove_entries_for_element(task_ref_btlog, task);
#endif

	counter_free(&task->faults);

#if CONFIG_COALITIONS
	task_release_coalitions(task);
#endif                        

	bzero(task->coalition, sizeof(task->coalition));

#if MACH_BSD
	                                                                         
	if (task->corpse_info) {
		void *corpse_info_kernel = kcdata_memory_get_begin_addr(task->corpse_info);
		task_crashinfo_destroy(task->corpse_info);
		task->corpse_info = NULL;
		if (corpse_info_kernel) {
			kheap_free(KHEAP_DATA_BUFFERS, corpse_info_kernel,
			    CORPSEINFO_ALLOCATION_SIZE);
		}
	}
#endif

#if CONFIG_MACF
	if (task->crash_label) {
		mac_exc_free_label(task->crash_label);
		task->crash_label = NULL;
	}
#endif

	assert(queue_empty(&task->task_objq));
	task_objq_lock_destroy(task);

	zfree(task_zone, task);
}

  
                        
  
                                   
   
void
task_name_deallocate(
	task_name_t             task_name)
{
	return task_deallocate((task_t)task_name);
}

  
                              
  
                                   
   
void
task_policy_set_deallocate(task_policy_set_t task_policy_set)
{
	return task_deallocate((task_t)task_policy_set);
}

  
                              
  
                                   
   
void
task_policy_get_deallocate(task_policy_get_t task_policy_get)
{
	return task_deallocate((task_t)task_policy_get);
}

  
                           
  
                                    
   
void
task_inspect_deallocate(
	task_inspect_t          task_inspect)
{
	return task_deallocate((task_t)task_inspect);
}

  
                        
  
                                      
   
void
task_read_deallocate(
	task_read_t          task_read)
{
	return task_deallocate((task_t)task_read);
}

  
                                    
  
                                               
   
void
task_suspension_token_deallocate(
	task_suspension_token_t         token)
{
	return task_deallocate((task_t)token);
}


  
                           
  
                                                  
   
kern_return_t
task_collect_crash_info(
	task_t task,
#ifdef CONFIG_MACF
	struct label *crash_label,
#endif
	int is_corpse_fork)
{
	kern_return_t kr = KERN_SUCCESS;

	kcdata_descriptor_t crash_data = NULL;
	kcdata_descriptor_t crash_data_release = NULL;
	mach_msg_type_number_t size = CORPSEINFO_ALLOCATION_SIZE;
	mach_vm_offset_t crash_data_ptr = 0;
	void *crash_data_kernel = NULL;
	void *crash_data_kernel_release = NULL;
#if CONFIG_MACF
	struct label *label, *free_label;
#endif

	if (!corpses_enabled()) {
		return KERN_NOT_SUPPORTED;
	}

#if CONFIG_MACF
	free_label = label = mac_exc_create_label();
#endif

	task_lock(task);

	assert(is_corpse_fork || task->bsd_info != NULL);
	if (task->corpse_info == NULL && (is_corpse_fork || task->bsd_info != NULL)) {
#if CONFIG_MACF
		                                                                  
		free_label = task->crash_label;                     
		task->crash_label = label;
		mac_exc_update_task_crash_label(task, crash_label);
#endif
		task_unlock(task);

		crash_data_kernel = kheap_alloc(KHEAP_DATA_BUFFERS,
		    CORPSEINFO_ALLOCATION_SIZE, Z_WAITOK | Z_ZERO);
		if (crash_data_kernel == NULL) {
			kr = KERN_RESOURCE_SHORTAGE;
			goto out_no_lock;
		}
		crash_data_ptr = (mach_vm_offset_t) crash_data_kernel;

		                                             
		crash_data = task_crashinfo_alloc_init((mach_vm_address_t)crash_data_ptr, size,
		    is_corpse_fork ? 0 : CORPSE_CRASHINFO_HAS_REF,
		    KCFLAG_USE_MEMCOPY);
		if (crash_data) {
			task_lock(task);
			crash_data_release = task->corpse_info;
			crash_data_kernel_release = kcdata_memory_get_begin_addr(crash_data_release);
			task->corpse_info = crash_data;

			task_unlock(task);
			kr = KERN_SUCCESS;
		} else {
			kheap_free(KHEAP_DATA_BUFFERS, crash_data_kernel,
			    CORPSEINFO_ALLOCATION_SIZE);
			kr = KERN_FAILURE;
		}

		if (crash_data_release != NULL) {
			task_crashinfo_destroy(crash_data_release);
		}
		if (crash_data_kernel_release != NULL) {
			kheap_free(KHEAP_DATA_BUFFERS, crash_data_kernel_release,
			    CORPSEINFO_ALLOCATION_SIZE);
		}
	} else {
		task_unlock(task);
	}

out_no_lock:
#if CONFIG_MACF
	if (free_label != NULL) {
		mac_exc_free_label(free_label);
	}
#endif
	return kr;
}

  
                                   
  
                                                      
   
kern_return_t
task_deliver_crash_notification(
	task_t task,
	thread_t thread,
	exception_type_t etype,
	mach_exception_subcode_t subcode)
{
	kcdata_descriptor_t crash_info = task->corpse_info;
	thread_t th_iter = NULL;
	kern_return_t kr = KERN_SUCCESS;
	wait_interrupt_t wsave;
	mach_exception_data_type_t code[EXCEPTION_CODE_MAX];
	ipc_port_t task_port, old_notify;

	if (crash_info == NULL) {
		return KERN_FAILURE;
	}

	task_lock(task);
	if (task_is_a_corpse_fork(task)) {
		                                                             
		code[0] = etype;
		code[1] = subcode;
	} else {
		                                              
		code[0] = EXC_CRASH;
		code[1] = 0;
		                                                                        
		if (corpse_for_fatal_memkill) {
			code[1] = subcode;
		}
	}

	queue_iterate(&task->threads, th_iter, thread_t, task_threads)
	{
		if (th_iter->corpse_dup == FALSE) {
			ipc_thread_reset(th_iter);
		}
	}
	task_unlock(task);

	                                                 
	task_reference(task);
	task_port = convert_task_to_port(task);
	ip_lock(task_port);
	require_ip_active(task_port);
	ipc_port_nsrequest(task_port, task_port->ip_mscount, ipc_port_make_sonce_locked(task_port), &old_notify);
	                   
	assert(IP_NULL == old_notify);

	wsave = thread_interrupt_level(THREAD_UNINT);
	kr = exception_triage_thread(EXC_CORPSE_NOTIFY, code, EXCEPTION_CODE_MAX, thread);
	if (kr != KERN_SUCCESS) {
		printf("Failed to send exception EXC_CORPSE_NOTIFY. error code: %d for pid %d\n", kr, task_pid(task));
	}

	(void)thread_interrupt_level(wsave);

	  
                                                   
                                                       
    
	ipc_port_release_send(task_port);
	return kr;
}

  
                  
  
                                                                  
                                                                      
   

kern_return_t
task_terminate(
	task_t          task)
{
	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (task->bsd_info) {
		return KERN_FAILURE;
	}

	return task_terminate_internal(task);
}

#if MACH_ASSERT
extern int proc_pid(struct proc *);
extern void proc_name_kdp(task_t t, char *buf, int size);
#endif                  

#define VM_MAP_PARTIAL_REAP 0x54             
static void
__unused task_partial_reap(task_t task, __unused int pid)
{
	unsigned int    reclaimed_resident = 0;
	unsigned int    reclaimed_compressed = 0;
	uint64_t        task_page_count;

	task_page_count = (get_task_phys_footprint(task) / PAGE_SIZE_64);

	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_MAP_PARTIAL_REAP) | DBG_FUNC_START),
	    pid, task_page_count, 0, 0, 0);

	vm_map_partial_reap(task->map, &reclaimed_resident, &reclaimed_compressed);

	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_MAP_PARTIAL_REAP) | DBG_FUNC_END),
	    pid, reclaimed_resident, reclaimed_compressed, 0, 0);
}

kern_return_t
task_mark_corpse(task_t task)
{
	kern_return_t kr = KERN_SUCCESS;
	thread_t self_thread;
	(void) self_thread;
	wait_interrupt_t wsave;
#if CONFIG_MACF
	struct label *crash_label = NULL;
#endif

	assert(task != kernel_task);
	assert(task == current_task());
	assert(!task_is_a_corpse(task));

#if CONFIG_MACF
	crash_label = mac_exc_create_label_for_proc((struct proc*)task->bsd_info);
#endif

	kr = task_collect_crash_info(task,
#if CONFIG_MACF
	    crash_label,
#endif
	    FALSE);
	if (kr != KERN_SUCCESS) {
		goto out;
	}

	self_thread = current_thread();

	wsave = thread_interrupt_level(THREAD_UNINT);
	task_lock(task);

	task_set_corpse_pending_report(task);
	task_set_corpse(task);
	task->crashed_thread_id = thread_tid(self_thread);

	kr = task_start_halt_locked(task, TRUE);
	assert(kr == KERN_SUCCESS);

	ipc_task_reset(task);
	                                                                                     
	task_set_special_port_internal(task, TASK_KERNEL_PORT, IPC_PORT_NULL);
	ipc_task_enable(task);

	task_unlock(task);
	                             
	ipc_space_terminate(task->itk_space);

	                                       
	task_add_to_corpse_task_list(task);

	task_start_halt(task);
	thread_terminate_internal(self_thread, TH_TERMINATE_OPTION_NONE);

	(void) thread_interrupt_level(wsave);
	assert(task->halting == TRUE);

out:
#if CONFIG_MACF
	mac_exc_free_label(crash_label);
#endif
	return kr;
}

  
                    
  
                                         
                                         
   
void
task_clear_corpse(task_t task)
{
	thread_t th_iter = NULL;

	task_lock(task);
	queue_iterate(&task->threads, th_iter, thread_t, task_threads)
	{
		thread_mtx_lock(th_iter);
		th_iter->inspection = FALSE;
		ipc_thread_disable(th_iter);
		thread_mtx_unlock(th_iter);
	}

	thread_terminate_crashed_threads();
	                                           
	task_clear_corpse_pending_report(task);

	task_unlock(task);
}

  
                   
  
                                                             
                             
                                                                      
   
void
task_port_notify(mach_msg_header_t *msg)
{
	mach_no_senders_notification_t *notification = (void *)msg;
	ipc_port_t port = notification->not_header.msgh_remote_port;
	task_t task;

	require_ip_active(port);
	assert(IKOT_TASK_CONTROL == ip_kotype(port));
	task = (task_t) ip_get_kobject(port);

	assert(task_is_a_corpse(task));

	                                                  
	task_remove_from_corpse_task_list(task);

	task_clear_corpse(task);
	task_terminate_internal(task);
}

  
                               
  
                                                             
                                                                      
                                                                  
   
void
task_port_with_flavor_notify(mach_msg_header_t *msg)
{
	mach_no_senders_notification_t *notification = (void *)msg;
	ipc_port_t port = notification->not_header.msgh_remote_port;
	task_t task;
	mach_task_flavor_t flavor;
	ipc_kobject_type_t kotype;

	ip_lock(port);
	if (port->ip_srights > 0) {
		ip_unlock(port);
		return;
	}
	task = (task_t)ipc_kobject_get(port);
	kotype = ip_kotype(port);
	if (task != TASK_NULL) {
		assert((IKOT_TASK_READ == kotype) || (IKOT_TASK_INSPECT == kotype));
		task_reference_internal(task);
	}
	ip_unlock(port);

	if (task == TASK_NULL) {
		                                                                             
		return;
	}

	if (kotype == IKOT_TASK_READ) {
		flavor = TASK_FLAVOR_READ;
	} else {
		flavor = TASK_FLAVOR_INSPECT;
	}

	itk_lock(task);
	ip_lock(port);
	  
                                                                  
                                                               
                            
   
                                                              
                                                               
                                  
   
                                                                     
                                                                      
                                                                      
    
	if (!ip_active(port) ||
	    task->itk_task_ports[flavor] != port ||
	    port->ip_srights > 0) {
		ip_unlock(port);
		itk_unlock(task);
		task_deallocate(task);
		return;
	}

	assert(task->itk_task_ports[flavor] == port);
	task->itk_task_ports[flavor] = IP_NULL;

	ipc_kobject_set_atomically(port, IKO_NULL, IKOT_NONE);
	ip_unlock(port);
	itk_unlock(task);
	task_deallocate(task);

	ipc_port_dealloc_kernel(port);
}

  
                                          
  
                                                        
                                                 
   
void
task_wait_till_threads_terminate_locked(task_t task)
{
	                                                       
	while (task->active_thread_count != 0) {
		assert_wait((event_t)&task->active_thread_count, THREAD_UNINT);
		task_unlock(task);
		thread_block(THREAD_CONTINUE_NULL);

		task_lock(task);
	}
}

  
                                 
  
                             
                                                            
                                                  
   
kern_return_t
task_duplicate_map_and_threads(
	task_t task,
	void *p,
	task_t new_task,
	thread_t *thread_ret,
	uint64_t **udata_buffer,
	int *size,
	int *num_udata)
{
	kern_return_t kr = KERN_SUCCESS;
	int active;
	thread_t thread, self, thread_return = THREAD_NULL;
	thread_t new_thread = THREAD_NULL, first_thread = THREAD_NULL;
	thread_t *thread_array;
	uint32_t active_thread_count = 0, array_count = 0, i;
	vm_map_t oldmap;
	uint64_t *buffer = NULL;
	int buf_size = 0;
	int est_knotes = 0, num_knotes = 0;

	self = current_thread();

	  
                                                           
                                                    
                          
    
	kr = task_suspend_internal(task);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	if (task->map->disable_vmentry_reuse == TRUE) {
		  
                                                      
                                                        
                                                     
                                            
    
             
     
#if DEVELOPMENT || DEBUG
		memorystatus_abort_vm_map_fork(task);
#endif
		task_resume_internal(task);
		return KERN_FAILURE;
	}

	                                                           
	if (memorystatus_allowed_vm_map_fork(task)) {
		                                                                         
		oldmap = new_task->map;
		new_task->map = vm_map_fork(new_task->ledger,
		    task->map,
		    (VM_MAP_FORK_SHARE_IF_INHERIT_NONE |
		    VM_MAP_FORK_PRESERVE_PURGEABLE |
		    VM_MAP_FORK_CORPSE_FOOTPRINT));
		vm_map_deallocate(oldmap);

		                                                   
		vm_map_copy_footprint_ledgers(task, new_task);

		                                            
		est_knotes = kevent_proc_copy_uptrs(p, NULL, 0);
		if (est_knotes > 0) {
			buf_size = (est_knotes + 32) * sizeof(uint64_t);
			buffer = kheap_alloc(KHEAP_DATA_BUFFERS, buf_size, Z_WAITOK);
			num_knotes = kevent_proc_copy_uptrs(p, buffer, buf_size);
			if (num_knotes > est_knotes + 32) {
				num_knotes = est_knotes + 32;
			}
		}
	}

	active_thread_count = task->active_thread_count;
	if (active_thread_count == 0) {
		if (buffer != NULL) {
			kheap_free(KHEAP_DATA_BUFFERS, buffer, buf_size);
		}
		task_resume_internal(task);
		return KERN_FAILURE;
	}

	thread_array = kheap_alloc(KHEAP_TEMP,
	    sizeof(thread_t) * active_thread_count, Z_WAITOK);

	                                                                                                   
	task_lock(task);
	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		                           
		active = thread->active;
		if (!active) {
			continue;
		}

		if (array_count >= active_thread_count) {
			break;
		}

		thread_array[array_count++] = thread;
		thread_reference(thread);
	}
	task_unlock(task);

	for (i = 0; i < array_count; i++) {
		kr = thread_create_with_continuation(new_task, &new_thread, (thread_continue_t)thread_corpse_continue);
		if (kr != KERN_SUCCESS) {
			break;
		}

		                                            
		if (thread_array[i] == self) {
			thread_return = new_thread;
			new_task->crashed_thread_id = thread_tid(new_thread);
		} else if (first_thread == NULL) {
			first_thread = new_thread;
		} else {
			                                                                    
			thread_deallocate(new_thread);
		}

		kr = thread_dup2(thread_array[i], new_thread);
		if (kr != KERN_SUCCESS) {
			thread_mtx_lock(new_thread);
			new_thread->corpse_dup = TRUE;
			thread_mtx_unlock(new_thread);
			continue;
		}

		                      
		bsd_copythreadname(new_thread->uthread, thread_array[i]->uthread);
		new_thread->thread_tag = thread_array[i]->thread_tag;
		thread_copy_resource_info(new_thread, thread_array[i]);
	}

	                                                                           
	if (thread_return == THREAD_NULL) {
		thread_return = first_thread;
	} else if (first_thread != THREAD_NULL) {
		                                                                    
		thread_deallocate(first_thread);
	}

	task_resume_internal(task);

	for (i = 0; i < array_count; i++) {
		thread_deallocate(thread_array[i]);
	}
	kheap_free(KHEAP_TEMP, thread_array, sizeof(thread_t) * active_thread_count);

	if (kr == KERN_SUCCESS) {
		*thread_ret = thread_return;
		*udata_buffer = buffer;
		*size = buf_size;
		*num_udata = num_knotes;
	} else {
		if (thread_return != THREAD_NULL) {
			thread_deallocate(thread_return);
		}
		if (buffer != NULL) {
			kheap_free(KHEAP_DATA_BUFFERS, buffer, buf_size);
		}
	}

	return kr;
}

#if CONFIG_SECLUDED_MEMORY
extern void task_set_can_use_secluded_mem_locked(
	task_t          task,
	boolean_t       can_use_secluded_mem);
#endif                             

#if MACH_ASSERT
int debug4k_panic_on_terminate = 0;
#endif                  
kern_return_t
task_terminate_internal(
	task_t                  task)
{
	thread_t                        thread, self;
	task_t                          self_task;
	boolean_t                       interrupt_save;
	int                             pid = 0;

	assert(task != kernel_task);

	self = current_thread();
	self_task = self->task;

	  
                                                            
                                             
    
	if (task == self_task) {
		task_lock(task);
	} else if (task < self_task) {
		task_lock(task);
		task_lock(self_task);
	} else {
		task_lock(self_task);
		task_lock(task);
	}

#if CONFIG_SECLUDED_MEMORY
	if (task->task_can_use_secluded_mem) {
		task_set_can_use_secluded_mem_locked(task, FALSE);
	}
	task->task_could_use_secluded_mem = FALSE;
	task->task_could_also_use_secluded_mem = FALSE;

	if (task->task_suppressed_secluded) {
		stop_secluded_suppression(task);
	}
#endif                             

	if (!task->active) {
		  
                                      
                                                     
                                                    
                                                          
     
		task_unlock(task);
		if (self_task != task) {
			task_unlock(self_task);
		}

		return KERN_FAILURE;
	}

	if (task_corpse_pending_report(task)) {
		  
                                            
                                    
                                                    
                                            
     
		task_unlock(task);
		if (self_task != task) {
			task_unlock(self_task);
		}

		return KERN_FAILURE;
	}

	if (self_task != task) {
		task_unlock(self_task);
	}

	  
                                                            
                                      
    
	interrupt_save = thread_interrupt_level(THREAD_UNINT);

	  
                                                           
                                                         
                                                            
                                                         
                                                     
                                              
    
	task_hold_locked(task);
	task->active = FALSE;
	ipc_task_disable(task);

#if CONFIG_TELEMETRY
	  
                                                  
    
	telemetry_task_ctl_locked(task, TF_TELEMETRY, 0);
#endif

	  
                                      
    
	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		thread_terminate_internal(thread, TH_TERMINATE_OPTION_NONE);
	}

#ifdef MACH_BSD
	if (task->bsd_info != NULL && !task_is_exec_copy(task)) {
		pid = proc_pid(task->bsd_info);
	}
#endif               

	task_unlock(task);

	proc_set_task_policy(task, TASK_POLICY_ATTRIBUTE,
	    TASK_POLICY_TERMINATED, TASK_POLICY_ENABLE);

	                             

                                      
                                       

#if CONFIG_TASKWATCH
	  
                            
    
	task_removewatchers(task);

#endif                       

	  
                                                
    
	task_synchronizer_destroy_all(task);

	  
                                          
    
	task_remove_turnstile_watchports(task);

	  
                                                           
    
	ipc_space_terminate(task->itk_space);

#if 00
	                                                       
	ledger_disable_panic_on_negative(task->map->pmap->ledger,
	    task_ledgers.phys_footprint);
	ledger_disable_panic_on_negative(task->map->pmap->ledger,
	    task_ledgers.internal);
	ledger_disable_panic_on_negative(task->map->pmap->ledger,
	    task_ledgers.internal_compressed);
	ledger_disable_panic_on_negative(task->map->pmap->ledger,
	    task_ledgers.iokit_mapped);
	ledger_disable_panic_on_negative(task->map->pmap->ledger,
	    task_ledgers.alternate_accounting);
	ledger_disable_panic_on_negative(task->map->pmap->ledger,
	    task_ledgers.alternate_accounting_compressed);
#endif

	  
                                                 
                                                
                                                 
                                              
                                                 
                                          
    

	vm_map_lock(task->map);
	vm_map_disable_hole_optimization(task->map);
	vm_map_unlock(task->map);

#if MACH_ASSERT
	  
                                                               
                             
    
	char procname[17];
	if (task->bsd_info && !task_is_exec_copy(task)) {
		pid = proc_pid(task->bsd_info);
		proc_name_kdp(task, procname, sizeof(procname));
	} else {
		pid = 0;
		strlcpy(procname, "<unknown>", sizeof(procname));
	}
	pmap_set_process(task->map->pmap, pid, procname);
	if (vm_map_page_shift(task->map) < (int)PAGE_SHIFT) {
		DEBUG4K_LIFE("map %p procname: %s\n", task->map, procname);
		if (debug4k_panic_on_terminate) {
			panic("DEBUG4K: %s:%d %d[%s] map %p\n", __FUNCTION__, __LINE__, pid, procname, task->map);
		}
	}
#endif                  

	vm_map_terminate(task->map);

	                               
	vm_shared_region_set(task, NULL);

#if __has_feature(ptrauth_calls)
	task_set_shared_region_id(task, NULL);
#endif                                   

	lck_mtx_lock(&tasks_threads_lock);
	queue_remove(&tasks, task, task_t, tasks);
	queue_enter(&terminated_tasks, task, task_t, tasks);
	tasks_count--;
	terminated_tasks_count++;
	lck_mtx_unlock(&tasks_threads_lock);

	  
                                                                
                                     
    
	thread_interrupt_level(interrupt_save);

#if KPC
	                                        
	if (task->t_kpc & TASK_KPC_FORCED_ALL_CTRS) {
		kpc_force_all_ctrs(task, 0);
	}
#endif          

#if CONFIG_COALITIONS
	  
                                                             
    
	coalitions_remove_task(task);
#endif

#if CONFIG_FREEZE
	extern int      vm_compressor_available;
	if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE && vm_compressor_available) {
		task_disown_frozen_csegs(task);
		assert(queue_empty(&task->task_frozen_cseg_q));
	}
#endif                    

	  
                                                   
    
	task_deallocate(task);

	return KERN_SUCCESS;
}

void
tasks_system_suspend(boolean_t suspend)
{
	task_t task;

	lck_mtx_lock(&tasks_threads_lock);
	assert(tasks_suspend_state != suspend);
	tasks_suspend_state = suspend;
	queue_iterate(&tasks, task, task_t, tasks) {
		if (task == kernel_task) {
			continue;
		}
		suspend ? task_suspend_internal(task) : task_resume_internal(task);
	}
	lck_mtx_unlock(&tasks_threads_lock);
}

  
                   
  
                                                                     
                                                                
                                                              
               
   
kern_return_t
task_start_halt(task_t task)
{
	kern_return_t kr = KERN_SUCCESS;
	task_lock(task);
	kr = task_start_halt_locked(task, FALSE);
	task_unlock(task);
	return kr;
}

static kern_return_t
task_start_halt_locked(task_t task, boolean_t should_mark_corpse)
{
	thread_t thread, self;
	uint64_t dispatchqueue_offset;

	assert(task != kernel_task);

	self = current_thread();

	if (task != self->task && !task_is_a_corpse_fork(task)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (task->halting || !task->active || !self->active) {
		  
                                                        
                                                          
                                                        
               
     
		return KERN_FAILURE;
	}

	task->halting = TRUE;

	  
                                                            
                                                             
                                                              
                                                         
    
	task_hold_locked(task);
	dispatchqueue_offset = get_dispatchqueue_offset_from_proc(task->bsd_info);

	  
                                                
    
	queue_iterate(&task->threads, thread, thread_t, task_threads)
	{
		if (should_mark_corpse) {
			thread_mtx_lock(thread);
			thread->inspection = TRUE;
			thread_mtx_unlock(thread);
		}
		if (thread != self) {
			thread_terminate_internal(thread, TH_TERMINATE_OPTION_NONE);
		}
	}
	task->dispatchqueue_offset = dispatchqueue_offset;

	task_release_locked(task);

	return KERN_SUCCESS;
}


  
                      
  
                                                                     
                                                                  
                                                             
  
                                                                     
                                     
   
void
task_complete_halt(task_t task)
{
	task_lock(task);
	assert(task->halting);
	assert(task == current_task());

	  
                                                
                                                       
             
    
	if (task->thread_count > 1) {
		assert_wait((event_t)&task->halting, THREAD_UNINT);
		task_unlock(task);
		thread_block(THREAD_CONTINUE_NULL);
	} else {
		task_unlock(task);
	}

	  
                                            
                                              
                                             
                           
    
	machine_task_terminate(task);

	  
                                                
    
	task_synchronizer_destroy_all(task);

	  
                                                       
                       
    
	ipc_space_clean(task->itk_space);

	  
                                                      
                      
    
	vm_map_remove(task->map, task->map->min_offset,
	    task->map->max_offset,
	      
                      
                      
                                   
                                 
        
	    (VM_MAP_REMOVE_NO_UNNESTING |
	    VM_MAP_REMOVE_IMMUTABLE |
	    VM_MAP_REMOVE_GAPS_OK));

	  
                                                                      
                                           
    
	iokit_task_terminate(task);
}

  
                    
  
                                           
                                                               
                          
  
                                             
   
void
task_hold_locked(
	task_t          task)
{
	thread_t        thread;

	assert(task->active);

	if (task->suspend_count++ > 0) {
		return;
	}

	if (task->bsd_info) {
		workq_proc_suspended(task->bsd_info);
	}

	  
                                                  
    
	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		thread_mtx_lock(thread);
		thread_hold(thread);
		thread_mtx_unlock(thread);
	}
}

  
             
  
                                                               
                                                                      
                                                                 
                                                                    
                                        
  
                                                            
   
kern_return_t
task_hold(
	task_t          task)
{
	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);

	if (!task->active) {
		task_unlock(task);

		return KERN_FAILURE;
	}

	task_hold_locked(task);
	task_unlock(task);

	return KERN_SUCCESS;
}

kern_return_t
task_wait(
	task_t          task,
	boolean_t       until_not_runnable)
{
	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);

	if (!task->active) {
		task_unlock(task);

		return KERN_FAILURE;
	}

	task_wait_locked(task, until_not_runnable);
	task_unlock(task);

	return KERN_SUCCESS;
}

  
                    
  
                                        
  
              
                                             
   
void
task_wait_locked(
	task_t          task,
	boolean_t               until_not_runnable)
{
	thread_t        thread, self;

	assert(task->active);
	assert(task->suspend_count > 0);

	self = current_thread();

	  
                                                        
                                                             
             
    
	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		if (thread != self) {
			thread_wait(thread, until_not_runnable);
		}
	}
}

boolean_t
task_is_app_suspended(task_t task)
{
	return task->pidsuspended;
}

  
                       
  
                                   
  
                                                 
   
void
task_release_locked(
	task_t          task)
{
	thread_t        thread;

	assert(task->active);
	assert(task->suspend_count > 0);

	if (--task->suspend_count > 0) {
		return;
	}

	if (task->bsd_info) {
		workq_proc_resumed(task->bsd_info);
	}

	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		thread_mtx_lock(thread);
		thread_release(thread);
		thread_mtx_unlock(thread);
	}
}

  
                
  
                                                               
                                      
  
                                                            
   
kern_return_t
task_release(
	task_t          task)
{
	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);

	if (!task->active) {
		task_unlock(task);

		return KERN_FAILURE;
	}

	task_release_locked(task);
	task_unlock(task);

	return KERN_SUCCESS;
}

static kern_return_t
task_threads_internal(
	task_t                      task,
	thread_act_array_t         *threads_out,
	mach_msg_type_number_t     *count,
	mach_thread_flavor_t        flavor)
{
	mach_msg_type_number_t  actual;
	thread_t                                *thread_list;
	thread_t                                thread;
	vm_size_t                               size, size_needed;
	void                                    *addr;
	unsigned int                    i, j;

	size = 0; addr = NULL;

	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	assert(flavor <= THREAD_FLAVOR_INSPECT);

	for (;;) {
		task_lock(task);
		if (!task->active) {
			task_unlock(task);

			if (size != 0) {
				kfree(addr, size);
			}

			return KERN_FAILURE;
		}

		actual = task->thread_count;

		                                    
		size_needed = actual * sizeof(mach_port_t);
		if (size_needed <= size) {
			break;
		}

		                                              
		task_unlock(task);

		if (size != 0) {
			kfree(addr, size);
		}

		assert(size_needed > 0);
		size = size_needed;

		addr = kalloc(size);
		if (addr == 0) {
			return KERN_RESOURCE_SHORTAGE;
		}
	}

	                                                     
	thread_list = (thread_t *)addr;

	i = j = 0;

	for (thread = (thread_t)queue_first(&task->threads); i < actual;
	    ++i, thread = (thread_t)queue_next(&thread->task_threads)) {
		thread_reference_internal(thread);
		thread_list[j++] = thread;
	}

	assert(queue_end(&task->threads, (queue_entry_t)thread));

	actual = j;
	size_needed = actual * sizeof(mach_port_t);

	                                                        
	task_unlock(task);

	if (actual == 0) {
		                                                              

		*threads_out = NULL;
		*count = 0;

		if (size != 0) {
			kfree(addr, size);
		}
	} else {
		                                         

		if (size_needed < size) {
			void *newaddr;

			newaddr = kalloc(size_needed);
			if (newaddr == 0) {
				for (i = 0; i < actual; ++i) {
					thread_deallocate(thread_list[i]);
				}
				kfree(addr, size);
				return KERN_RESOURCE_SHORTAGE;
			}

			bcopy(addr, newaddr, size_needed);
			kfree(addr, size);
			thread_list = (thread_t *)newaddr;
		}

		*threads_out = thread_list;
		*count = actual;

		                                              

		switch (flavor) {
		case THREAD_FLAVOR_CONTROL:
			if (task == current_task()) {
				for (i = 0; i < actual; ++i) {
					((ipc_port_t *) thread_list)[i] = convert_thread_to_port_pinned(thread_list[i]);
				}
			} else {
				for (i = 0; i < actual; ++i) {
					((ipc_port_t *) thread_list)[i] = convert_thread_to_port(thread_list[i]);
				}
			}
			break;
		case THREAD_FLAVOR_READ:
			for (i = 0; i < actual; ++i) {
				((ipc_port_t *) thread_list)[i] = convert_thread_read_to_port(thread_list[i]);
			}
			break;
		case THREAD_FLAVOR_INSPECT:
			for (i = 0; i < actual; ++i) {
				((ipc_port_t *) thread_list)[i] = convert_thread_inspect_to_port(thread_list[i]);
			}
			break;
		}
	}

	return KERN_SUCCESS;
}

kern_return_t
task_threads(
	task_t                      task,
	thread_act_array_t         *threads_out,
	mach_msg_type_number_t     *count)
{
	return task_threads_internal(task, threads_out, count, THREAD_FLAVOR_CONTROL);
}


kern_return_t
task_threads_from_user(
	mach_port_t                 port,
	thread_act_array_t         *threads_out,
	mach_msg_type_number_t     *count)
{
	ipc_kobject_type_t kotype;
	kern_return_t kr;

	task_t task = convert_port_to_task_check_type(port, &kotype, TASK_FLAVOR_INSPECT, FALSE);

	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	switch (kotype) {
	case IKOT_TASK_CONTROL:
		kr = task_threads_internal(task, threads_out, count, THREAD_FLAVOR_CONTROL);
		break;
	case IKOT_TASK_READ:
		kr = task_threads_internal(task, threads_out, count, THREAD_FLAVOR_READ);
		break;
	case IKOT_TASK_INSPECT:
		kr = task_threads_internal(task, threads_out, count, THREAD_FLAVOR_INSPECT);
		break;
	default:
		panic("strange kobject type");
		break;
	}

	task_deallocate(task);
	return kr;
}

#define TASK_HOLD_NORMAL        0
#define TASK_HOLD_PIDSUSPEND    1
#define TASK_HOLD_LEGACY        2
#define TASK_HOLD_LEGACY_ALL    3

static kern_return_t
place_task_hold(
	task_t task,
	int mode)
{
	if (!task->active && !task_is_a_corpse(task)) {
		return KERN_FAILURE;
	}

	                                    
	if (task_is_a_corpse(task)) {
		return KERN_SUCCESS;
	}

	KDBG_RELEASE(MACHDBG_CODE(DBG_MACH_IPC, MACH_TASK_SUSPEND),
	    task_pid(task),
	    task->thread_count > 0 ?((thread_t)queue_first(&task->threads))->thread_id : 0,
	    task->user_stop_count, task->user_stop_count + 1);

#if MACH_ASSERT
	current_task()->suspends_outstanding++;
#endif

	if (mode == TASK_HOLD_LEGACY) {
		task->legacy_stop_count++;
	}

	if (task->user_stop_count++ > 0) {
		  
                                                
                                     
     
		return KERN_SUCCESS;
	}

	  
                                                           
                                                          
                                                            
                                
    
	task_hold_locked(task);
	task_wait_locked(task, FALSE);

	return KERN_SUCCESS;
}

static kern_return_t
release_task_hold(
	task_t          task,
	int                     mode)
{
	boolean_t release = FALSE;

	if (!task->active && !task_is_a_corpse(task)) {
		return KERN_FAILURE;
	}

	                                    
	if (task_is_a_corpse(task)) {
		return KERN_SUCCESS;
	}

	if (mode == TASK_HOLD_PIDSUSPEND) {
		if (task->pidsuspended == FALSE) {
			return KERN_FAILURE;
		}
		task->pidsuspended = FALSE;
	}

	if (task->user_stop_count > (task->pidsuspended ? 1 : 0)) {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    MACHDBG_CODE(DBG_MACH_IPC, MACH_TASK_RESUME) | DBG_FUNC_NONE,
		    task_pid(task), ((thread_t)queue_first(&task->threads))->thread_id,
		    task->user_stop_count, mode, task->legacy_stop_count);

#if MACH_ASSERT
		  
                                                                                          
                                                                                        
                        
     
		current_task()->suspends_outstanding--;
#endif

		if (mode == TASK_HOLD_LEGACY_ALL) {
			if (task->legacy_stop_count >= task->user_stop_count) {
				task->user_stop_count = 0;
				release = TRUE;
			} else {
				task->user_stop_count -= task->legacy_stop_count;
			}
			task->legacy_stop_count = 0;
		} else {
			if (mode == TASK_HOLD_LEGACY && task->legacy_stop_count > 0) {
				task->legacy_stop_count--;
			}
			if (--task->user_stop_count == 0) {
				release = TRUE;
			}
		}
	} else {
		return KERN_FAILURE;
	}

	  
                                  
    
	if (release) {
		task_release_locked(task);
	}

	return KERN_SUCCESS;
}

boolean_t
get_task_suspended(task_t task)
{
	return 0 != task->user_stop_count;
}

  
                
  
                                                                
  
                                                                  
                                                                     
                                                              
                                                                           
                                                                    
                           
  
              
                                                
   
kern_return_t
task_suspend(
	task_t          task)
{
	kern_return_t                   kr;
	mach_port_t                     port;
	mach_port_name_t                name;

	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);

	  
                                    
    
	kr = place_task_hold(task, TASK_HOLD_LEGACY);
	if (kr != KERN_SUCCESS) {
		task_unlock(task);
		return kr;
	}

	  
                                                                        
                                                    
    
	(void)ipc_kobject_make_send_lazy_alloc_port((ipc_port_t *) &task->itk_resume,
	    (ipc_kobject_t)task, IKOT_TASK_RESUME, IPC_KOBJECT_ALLOC_NONE, true,
	    OS_PTRAUTH_DISCRIMINATOR("task.itk_resume"));
	port = task->itk_resume;
	task_unlock(task);

	  
                                                                                         
                                                                                    
                                                               
    
	if (IP_VALID(port)) {
		kr = ipc_object_copyout(current_space(), ip_to_object(port),
		    MACH_MSG_TYPE_MOVE_SEND, IPC_OBJECT_COPYOUT_FLAGS_NONE,
		    NULL, NULL, &name);
	} else {
		kr = KERN_SUCCESS;
	}
	if (kr != KERN_SUCCESS) {
		printf("warning: %s(%d) failed to copyout suspension "
		    "token for pid %d with error: %d\n",
		    proc_name_address(current_task()->bsd_info),
		    proc_pid(current_task()->bsd_info),
		    task_pid(task), kr);
	}

	return kr;
}

  
               
                                  
  
              
                                            
   
kern_return_t
task_resume(
	task_t  task)
{
	kern_return_t    kr;
	mach_port_name_t resume_port_name;
	ipc_entry_t              resume_port_entry;
	ipc_space_t              space = current_task()->itk_space;

	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	                                
	task_lock(task);
	kr = release_task_hold(task, TASK_HOLD_LEGACY);
	task_unlock(task);

	is_write_lock(space);
	if (is_active(space) && IP_VALID(task->itk_resume) &&
	    ipc_hash_lookup(space, ip_to_object(task->itk_resume), &resume_port_name, &resume_port_entry) == TRUE) {
		  
                                                                                                 
                                                                                              
                                                                                               
             
     
		if (kr == KERN_SUCCESS) {
			ipc_right_dealloc(space, resume_port_name, resume_port_entry);
		} else {
			ipc_right_destroy(space, resume_port_name, resume_port_entry, FALSE, 0);
		}
		                    
	} else {
		is_write_unlock(space);
		if (kr == KERN_SUCCESS) {
			printf("warning: %s(%d) performed out-of-band resume on pid %d\n",
			    proc_name_address(current_task()->bsd_info), proc_pid(current_task()->bsd_info),
			    task_pid(task));
		}
	}

	return kr;
}

  
                           
                                                                       
   
kern_return_t
task_suspend_internal(task_t task)
{
	kern_return_t    kr;

	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);
	kr = place_task_hold(task, TASK_HOLD_NORMAL);
	task_unlock(task);
	return kr;
}

  
                                                                    
                                                
   
kern_return_t
task_suspend2(
	task_t                  task,
	task_suspension_token_t *suspend_token)
{
	kern_return_t    kr;

	kr = task_suspend_internal(task);
	if (kr != KERN_SUCCESS) {
		*suspend_token = TASK_NULL;
		return kr;
	}

	  
                                                                     
                                                                       
                                         
    
	task_reference_internal(task);
	*suspend_token = task;

	return KERN_SUCCESS;
}

  
                  
                                                                
   
kern_return_t
task_resume_internal(
	task_suspension_token_t         task)
{
	kern_return_t kr;

	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);
	kr = release_task_hold(task, TASK_HOLD_NORMAL);
	task_unlock(task);
	return kr;
}

  
                                                                      
   
kern_return_t
task_resume2(
	task_suspension_token_t         task)
{
	kern_return_t kr;

	kr = task_resume_internal(task);
	task_suspension_token_deallocate(task);

	return kr;
}

boolean_t
task_suspension_notify(mach_msg_header_t *request_header)
{
	ipc_port_t port = request_header->msgh_remote_port;
	task_t task = convert_port_to_task_suspension_token(port);
	mach_msg_type_number_t not_count;

	if (task == TASK_NULL || task == kernel_task) {
		return TRUE;                     
	}
	switch (request_header->msgh_id) {
	case MACH_NOTIFY_SEND_ONCE:
		                                                            
		task_lock(task);
		release_task_hold(task, TASK_HOLD_NORMAL);
		task_unlock(task);
		break;

	case MACH_NOTIFY_NO_SENDERS:
		not_count = ((mach_no_senders_notification_t *)request_header)->not_count;

		task_lock(task);
		ip_lock(port);
		if (port->ip_mscount == not_count) {
			                                                          
			assert(port->ip_nsrequest == IP_NULL);
			ip_unlock(port);
			release_task_hold(task, TASK_HOLD_LEGACY_ALL);
			task_unlock(task);
		} else if (port->ip_nsrequest == IP_NULL) {
			ipc_port_t old_notify;

			task_unlock(task);
			                                                                     
			ipc_port_nsrequest(port, port->ip_mscount, ipc_port_make_sonce_locked(port), &old_notify);
			assert(old_notify == IP_NULL);
			                   
		} else {
			ip_unlock(port);
			task_unlock(task);
		}
		break;

	default:
		break;
	}

	task_suspension_token_deallocate(task);                           
	return TRUE;
}

static kern_return_t
task_pidsuspend_locked(task_t task)
{
	kern_return_t kr;

	if (task->pidsuspended) {
		kr = KERN_FAILURE;
		goto out;
	}

	task->pidsuspended = TRUE;

	kr = place_task_hold(task, TASK_HOLD_PIDSUSPEND);
	if (kr != KERN_SUCCESS) {
		task->pidsuspended = FALSE;
	}
out:
	return kr;
}


  
                   
  
                                                    
  
              
                                                
   
kern_return_t
task_pidsuspend(
	task_t          task)
{
	kern_return_t    kr;

	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);

	kr = task_pidsuspend_locked(task);

	task_unlock(task);

	if ((KERN_SUCCESS == kr) && task->message_app_suspended) {
		iokit_task_app_suspended_changed(task);
	}

	return kr;
}

  
                  
                                        
  
              
                                            
   
kern_return_t
task_pidresume(
	task_t  task)
{
	kern_return_t    kr;

	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);

#if CONFIG_FREEZE

	while (task->changing_freeze_state) {
		assert_wait((event_t)&task->changing_freeze_state, THREAD_UNINT);
		task_unlock(task);
		thread_block(THREAD_CONTINUE_NULL);

		task_lock(task);
	}
	task->changing_freeze_state = TRUE;
#endif

	kr = release_task_hold(task, TASK_HOLD_PIDSUSPEND);

	task_unlock(task);

	if ((KERN_SUCCESS == kr) && task->message_app_suspended) {
		iokit_task_app_suspended_changed(task);
	}

#if CONFIG_FREEZE

	task_lock(task);

	if (kr == KERN_SUCCESS) {
		task->frozen = FALSE;
	}
	task->changing_freeze_state = FALSE;
	thread_wakeup(&task->changing_freeze_state);

	task_unlock(task);
#endif

	return kr;
}

os_refgrp_decl(static, task_watchports_refgrp, "task_watchports", NULL);

  
                                 
                                                          
  
             
                            
                               
                                        
                                         
  
              
                   
   
void
task_add_turnstile_watchports(
	task_t          task,
	thread_t        thread,
	ipc_port_t      *portwatch_ports,
	uint32_t        portwatch_count)
{
	struct task_watchports *watchports = NULL;
	struct task_watchport_elem *previous_elem_array[TASK_MAX_WATCHPORT_COUNT] = {};
	os_ref_count_t refs;

	                                      
	if (!task->active) {
		return;
	}

	assert(portwatch_count <= TASK_MAX_WATCHPORT_COUNT);

	watchports = task_watchports_alloc_init(task, thread, portwatch_count);

	                        
	is_write_lock(task->itk_space);

	                                               
	refs = task_add_turnstile_watchports_locked(task,
	    watchports, previous_elem_array, portwatch_ports,
	    portwatch_count);

	                         
	is_write_unlock(task->itk_space);

	if (refs == 0) {
		task_watchports_deallocate(watchports);
	}

	                                         
	for (uint32_t i = 0; i < portwatch_count && previous_elem_array[i] != NULL; i++) {
		task_watchport_elem_deallocate(previous_elem_array[i]);
	}
}

  
                                    
                                                          
  
             
                               
  
              
                   
   
void
task_remove_turnstile_watchports(
	task_t          task)
{
	os_ref_count_t refs = TASK_MAX_WATCHPORT_COUNT;
	struct task_watchports *watchports = NULL;
	ipc_port_t port_freelist[TASK_MAX_WATCHPORT_COUNT] = {};
	uint32_t portwatch_count;

	                        
	is_write_lock(task->itk_space);

	                                    
	if (task->watchports == NULL) {
		is_write_unlock(task->itk_space);
		return;
	}
	watchports = task->watchports;
	portwatch_count = watchports->tw_elem_array_count;

	refs = task_remove_turnstile_watchports_locked(task, watchports,
	    port_freelist);

	is_write_unlock(task->itk_space);

	                                  
	for (uint32_t i = 0; i < portwatch_count && port_freelist[i] != NULL; i++) {
		ip_release(port_freelist[i]);
	}

	                                                             
	if (refs == 0) {
		task_watchports_deallocate(watchports);
	}
}

  
                                      
                                                                     
  
             
                               
                              
                                   
  
              
                   
   
void
task_transfer_turnstile_watchports(
	task_t   old_task,
	task_t   new_task,
	thread_t new_thread)
{
	struct task_watchports *old_watchports = NULL;
	struct task_watchports *new_watchports = NULL;
	os_ref_count_t old_refs = TASK_MAX_WATCHPORT_COUNT;
	os_ref_count_t new_refs = TASK_MAX_WATCHPORT_COUNT;
	uint32_t portwatch_count;

	if (old_task->watchports == NULL || !new_task->active) {
		return;
	}

	                                                
	is_write_lock(old_task->itk_space);
	if (old_task->watchports == NULL) {
		is_write_unlock(old_task->itk_space);
		return;
	}

	portwatch_count = old_task->watchports->tw_elem_array_count;
	is_write_unlock(old_task->itk_space);

	new_watchports = task_watchports_alloc_init(new_task, new_thread, portwatch_count);

	                                     
	is_write_lock(old_task->itk_space);

	                                     
	is_write_lock(new_task->itk_space);

	                                    
	if (old_task->watchports == NULL || !new_task->active) {
		is_write_unlock(new_task->itk_space);
		is_write_unlock(old_task->itk_space);
		(void)task_watchports_release(new_watchports);
		task_watchports_deallocate(new_watchports);
		return;
	}

	old_watchports = old_task->watchports;
	assert(portwatch_count == old_task->watchports->tw_elem_array_count);

	                               
	new_task->watchports = new_watchports;

	for (uint32_t i = 0; i < portwatch_count; i++) {
		ipc_port_t port = old_watchports->tw_elem[i].twe_port;

		if (port == NULL) {
			task_watchport_elem_clear(&new_watchports->tw_elem[i]);
			continue;
		}

		                                                 
		ip_lock(port);
		imq_lock(&port->ip_messages);

		task_watchport_elem_init(&new_watchports->tw_elem[i], new_task, port);

		if (ipc_port_replace_watchport_elem_conditional_locked(port,
		    &old_watchports->tw_elem[i], &new_watchports->tw_elem[i]) == KERN_SUCCESS) {
			task_watchport_elem_clear(&old_watchports->tw_elem[i]);

			task_watchports_retain(new_watchports);
			old_refs = task_watchports_release(old_watchports);

			                                    
			if (old_refs == 0) {
				old_task->watchports = NULL;
			}
		} else {
			task_watchport_elem_clear(&new_watchports->tw_elem[i]);
		}
		                                                                                    
	}

	                                                                                             
	new_refs = task_watchports_release(new_watchports);
	if (new_refs == 0) {
		new_task->watchports = NULL;
	}

	is_write_unlock(new_task->itk_space);
	is_write_unlock(old_task->itk_space);

	                                                            
	if (old_refs == 0) {
		task_watchports_deallocate(old_watchports);
	}

	                                                            
	if (new_refs == 0) {
		task_watchports_deallocate(new_watchports);
	}
}

  
                                        
                                                          
  
             
                       
                                                              
                                                                                
                                        
                                         
  
              
                                 
                                                              
   
static os_ref_count_t
task_add_turnstile_watchports_locked(
	task_t                      task,
	struct task_watchports      *watchports,
	struct task_watchport_elem  **previous_elem_array,
	ipc_port_t                  *portwatch_ports,
	uint32_t                    portwatch_count)
{
	os_ref_count_t refs = TASK_MAX_WATCHPORT_COUNT;

	                                       
	if (!task->active) {
		refs = task_watchports_release(watchports);
		return refs;
	}

	assert(task->watchports == NULL);
	task->watchports = watchports;

	for (uint32_t i = 0, j = 0; i < portwatch_count; i++) {
		ipc_port_t port = portwatch_ports[i];

		task_watchport_elem_init(&watchports->tw_elem[i], task, port);
		if (port == NULL) {
			task_watchport_elem_clear(&watchports->tw_elem[i]);
			continue;
		}

		ip_lock(port);
		imq_lock(&port->ip_messages);

		                                                              
		if (ipc_port_add_watchport_elem_locked(port, &watchports->tw_elem[i],
		    &previous_elem_array[j]) != KERN_SUCCESS) {
			task_watchport_elem_clear(&watchports->tw_elem[i]);
			continue;
		}
		                                        

		ip_reference(port);
		task_watchports_retain(watchports);
		if (previous_elem_array[j] != NULL) {
			j++;
		}
	}

	                                                                         
	refs = task_watchports_release(watchports);
	if (refs == 0) {
		task->watchports = NULL;
	}

	return refs;
}

  
                                           
                                                          
  
             
                                        
                                                 
                                                             
  
  
              
                                 
                                                          
   
static os_ref_count_t
task_remove_turnstile_watchports_locked(
	task_t                 task,
	struct task_watchports *watchports,
	ipc_port_t             *port_freelist)
{
	os_ref_count_t refs = TASK_MAX_WATCHPORT_COUNT;

	for (uint32_t i = 0, j = 0; i < watchports->tw_elem_array_count; i++) {
		ipc_port_t port = watchports->tw_elem[i].twe_port;
		if (port == NULL) {
			continue;
		}

		                                                 
		ip_lock(port);
		imq_lock(&port->ip_messages);
		if (ipc_port_clear_watchport_elem_internal_conditional_locked(port,
		    &watchports->tw_elem[i]) == KERN_SUCCESS) {
			task_watchport_elem_clear(&watchports->tw_elem[i]);
			port_freelist[j++] = port;
			refs = task_watchports_release(watchports);

			                                    
			if (refs == 0) {
				task->watchports = NULL;
				break;
			}
		}
		                                                                                           
	}
	return refs;
}

  
                              
                                                  
  
              
                   
   
static struct task_watchports *
task_watchports_alloc_init(
	task_t        task,
	thread_t      thread,
	uint32_t      count)
{
	struct task_watchports *watchports = kalloc(sizeof(struct task_watchports) +
	    count * sizeof(struct task_watchport_elem));

	task_reference(task);
	thread_reference(thread);
	watchports->tw_task = task;
	watchports->tw_thread = thread;
	watchports->tw_elem_array_count = count;
	os_ref_init(&watchports->tw_refcount, &task_watchports_refgrp);

	return watchports;
}

  
                              
                                     
  
              
                   
   
static void
task_watchports_deallocate(
	struct task_watchports *watchports)
{
	uint32_t portwatch_count = watchports->tw_elem_array_count;

	task_deallocate(watchports->tw_task);
	thread_deallocate(watchports->tw_thread);
	kfree(watchports, sizeof(struct task_watchports) + portwatch_count * sizeof(struct task_watchport_elem));
}

  
                                  
                                                                            
  
              
                   
   
void
task_watchport_elem_deallocate(
	struct task_watchport_elem *watchport_elem)
{
	os_ref_count_t refs = TASK_MAX_WATCHPORT_COUNT;
	task_t task = watchport_elem->twe_task;
	struct task_watchports *watchports = NULL;
	ipc_port_t port = NULL;

	assert(task != NULL);

	                                               
	is_write_lock(task->itk_space);

	watchports = task->watchports;
	assert(watchports != NULL);

	port = watchport_elem->twe_port;
	assert(port != NULL);

	task_watchport_elem_clear(watchport_elem);
	refs = task_watchports_release(watchports);

	if (refs == 0) {
		task->watchports = NULL;
	}

	is_write_unlock(task->itk_space);

	ip_release(port);
	if (refs == 0) {
		task_watchports_deallocate(watchports);
	}
}

  
                       
                                             
  
              
                   
   
boolean_t
task_has_watchports(task_t task)
{
	return task->watchports != NULL;
}

#if DEVELOPMENT || DEBUG

extern void IOSleep(int);

kern_return_t
task_disconnect_page_mappings(task_t task)
{
	int     n;

	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	  
                                                           
                                                        
                                                          
                                                        
                                                        
                                                                
                                                                   
                                                                        
                                                                            
                                                           
                                     
                                                        
                                                                     
                                            
    
	for (n = 0; n < 100; n++) {
		thread_t        thread;
		boolean_t       runnable;
		boolean_t       do_unnest;
		int             page_count;

		runnable = FALSE;
		do_unnest = FALSE;

		task_lock(task);

		queue_iterate(&task->threads, thread, thread_t, task_threads) {
			if (thread->state & TH_RUN) {
				runnable = TRUE;
				break;
			}
		}
		if (n == 0) {
			task->task_disconnected_count++;
		}

		if (task->task_unnested == FALSE) {
			if (runnable == TRUE) {
				task->task_unnested = TRUE;
				do_unnest = TRUE;
			}
		}
		task_unlock(task);

		if (runnable == FALSE) {
			break;
		}

		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_DISCONNECT_TASK_PAGE_MAPPINGS)) | DBG_FUNC_START,
		    task, do_unnest, task->task_disconnected_count, 0, 0);

		page_count = vm_map_disconnect_page_mappings(task->map, do_unnest);

		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_DISCONNECT_TASK_PAGE_MAPPINGS)) | DBG_FUNC_END,
		    task, page_count, 0, 0, 0);

		if ((n % 5) == 4) {
			IOSleep(1);
		}
	}
	return KERN_SUCCESS;
}

#endif


#if CONFIG_FREEZE

  
               
  
                 
  
              
                                                
   
extern void             vm_wake_compactor_swapper(void);
extern queue_head_t     c_swapout_list_head;
extern struct freezer_context freezer_context_global;

kern_return_t
task_freeze(
	task_t    task,
	uint32_t           *purgeable_count,
	uint32_t           *wired_count,
	uint32_t           *clean_count,
	uint32_t           *dirty_count,
	uint32_t           dirty_budget,
	uint32_t           *shared_count,
	int                *freezer_error_code,
	boolean_t          eval_only)
{
	kern_return_t kr = KERN_SUCCESS;

	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);

	while (task->changing_freeze_state) {
		assert_wait((event_t)&task->changing_freeze_state, THREAD_UNINT);
		task_unlock(task);
		thread_block(THREAD_CONTINUE_NULL);

		task_lock(task);
	}
	if (task->frozen) {
		task_unlock(task);
		return KERN_FAILURE;
	}
	task->changing_freeze_state = TRUE;

	freezer_context_global.freezer_ctx_task = task;

	task_unlock(task);

	kr = vm_map_freeze(task,
	    purgeable_count,
	    wired_count,
	    clean_count,
	    dirty_count,
	    dirty_budget,
	    shared_count,
	    freezer_error_code,
	    eval_only);

	task_lock(task);

	if ((kr == KERN_SUCCESS) && (eval_only == FALSE)) {
		task->frozen = TRUE;

		freezer_context_global.freezer_ctx_task = NULL;
		freezer_context_global.freezer_ctx_uncompressed_pages = 0;

		if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
			  
                                                                  
                                                                
      

			*dirty_count = (uint32_t) (freezer_context_global.freezer_ctx_swapped_bytes / PAGE_SIZE_64);                           
		}

		freezer_context_global.freezer_ctx_swapped_bytes = 0;
	}

	task->changing_freeze_state = FALSE;
	thread_wakeup(&task->changing_freeze_state);

	task_unlock(task);

	if (VM_CONFIG_COMPRESSOR_IS_PRESENT &&
	    (kr == KERN_SUCCESS) &&
	    (eval_only == FALSE)) {
		vm_wake_compactor_swapper();
		  
                                                        
                                                     
                                                           
                                                        
                    
     
		thread_wakeup((event_t)&c_swapout_list_head);
	}

	return kr;
}

  
             
  
                                
  
              
                                                
   
kern_return_t
task_thaw(
	task_t          task)
{
	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);

	while (task->changing_freeze_state) {
		assert_wait((event_t)&task->changing_freeze_state, THREAD_UNINT);
		task_unlock(task);
		thread_block(THREAD_CONTINUE_NULL);

		task_lock(task);
	}
	if (!task->frozen) {
		task_unlock(task);
		return KERN_FAILURE;
	}
	task->frozen = FALSE;

	task_unlock(task);

	return KERN_SUCCESS;
}

void
task_update_frozen_to_swap_acct(task_t task, int64_t amount, freezer_acct_op_t op)
{
	  
                                                                   
                                                                   
                                                                      
         
                                                                        
                                                                     
                 
    

	assert(task);
	if (amount == 0) {
		return;
	}

	if (op == CREDIT_TO_SWAP) {
		ledger_credit_nocheck(task->ledger, task_ledgers.frozen_to_swap, amount);
	} else if (op == DEBIT_FROM_SWAP) {
		ledger_debit_nocheck(task->ledger, task_ledgers.frozen_to_swap, amount);
	} else {
		panic("task_update_frozen_to_swap_acct: Invalid ledger op\n");
	}
}
#endif                    

kern_return_t
host_security_set_task_token(
	host_security_t  host_security,
	task_t           task,
	security_token_t sec_token,
	audit_token_t    audit_token,
	host_priv_t      host_priv)
{
	ipc_port_t       host_port;
	kern_return_t    kr;

	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (host_security == HOST_NULL) {
		return KERN_INVALID_SECURITY;
	}

	task_lock(task);
	task->sec_token = sec_token;
	task->audit_token = audit_token;
	task_unlock(task);

	if (host_priv != HOST_PRIV_NULL) {
		kr = host_get_host_priv_port(host_priv, &host_port);
	} else {
		kr = host_get_host_port(host_priv_self(), &host_port);
	}
	assert(kr == KERN_SUCCESS);

	kr = task_set_special_port_internal(task, TASK_HOST_PORT, host_port);
	return kr;
}

kern_return_t
task_send_trace_memory(
	__unused task_t   target_task,
	__unused uint32_t pid,
	__unused uint64_t uniqueid)
{
	return KERN_INVALID_ARGUMENT;
}

  
                                                                       
                                                                    
                                                                       
                                                                       
                                                                    
                                                                  
   
kern_return_t
task_set_info(
	task_t          task,
	task_flavor_t   flavor,
	__unused task_info_t    task_info_in,                                    
	__unused mach_msg_type_number_t task_info_count)
{
	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}
	switch (flavor) {
#if CONFIG_ATM
	case TASK_TRACE_MEMORY_INFO:
		return KERN_NOT_SUPPORTED;
#endif              
	default:
		return KERN_INVALID_ARGUMENT;
	}
}

int radar_20146450 = 1;
kern_return_t
task_info(
	task_t                  task,
	task_flavor_t           flavor,
	task_info_t             task_info_out,
	mach_msg_type_number_t  *task_info_count)
{
	kern_return_t error = KERN_SUCCESS;
	mach_msg_type_number_t  original_task_info_count;
	bool is_kernel_task = (task == kernel_task);

	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	original_task_info_count = *task_info_count;
	task_lock(task);

	if ((task != current_task()) && (!task->active)) {
		task_unlock(task);
		return KERN_INVALID_ARGUMENT;
	}


	switch (flavor) {
	case TASK_BASIC_INFO_32:
	case TASK_BASIC2_INFO_32:
#if defined(__arm__) || defined(__arm64__)
	case TASK_BASIC_INFO_64:
#endif
		{
			task_basic_info_32_t    basic_info;
			vm_map_t                                map;
			clock_sec_t                             secs;
			clock_usec_t                    usecs;

			if (*task_info_count < TASK_BASIC_INFO_32_COUNT) {
				error = KERN_INVALID_ARGUMENT;
				break;
			}

			basic_info = (task_basic_info_32_t)task_info_out;

			map = (task == kernel_task)? kernel_map: task->map;
			basic_info->virtual_size = (typeof(basic_info->virtual_size))vm_map_adjusted_size(map);
			if (flavor == TASK_BASIC2_INFO_32) {
				  
                                                    
                                                   
       
				basic_info->resident_size = pmap_resident_max(map->pmap);
			} else {
				basic_info->resident_size = pmap_resident_count(map->pmap);
			}
			basic_info->resident_size *= PAGE_SIZE;

			basic_info->policy = ((task != kernel_task)?
			    POLICY_TIMESHARE: POLICY_RR);
			basic_info->suspend_count = task->user_stop_count;

			absolutetime_to_microtime(task->total_user_time, &secs, &usecs);
			basic_info->user_time.seconds =
			    (typeof(basic_info->user_time.seconds))secs;
			basic_info->user_time.microseconds = usecs;

			absolutetime_to_microtime(task->total_system_time, &secs, &usecs);
			basic_info->system_time.seconds =
			    (typeof(basic_info->system_time.seconds))secs;
			basic_info->system_time.microseconds = usecs;

			*task_info_count = TASK_BASIC_INFO_32_COUNT;
			break;
		}

#if defined(__arm__) || defined(__arm64__)
	case TASK_BASIC_INFO_64_2:
	{
		task_basic_info_64_2_t  basic_info;
		vm_map_t                                map;
		clock_sec_t                             secs;
		clock_usec_t                    usecs;

		if (*task_info_count < TASK_BASIC_INFO_64_2_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		basic_info = (task_basic_info_64_2_t)task_info_out;

		map = (task == kernel_task)? kernel_map: task->map;
		basic_info->virtual_size  = vm_map_adjusted_size(map);
		basic_info->resident_size =
		    (mach_vm_size_t)(pmap_resident_count(map->pmap))
		    * PAGE_SIZE_64;

		basic_info->policy = ((task != kernel_task)?
		    POLICY_TIMESHARE: POLICY_RR);
		basic_info->suspend_count = task->user_stop_count;

		absolutetime_to_microtime(task->total_user_time, &secs, &usecs);
		basic_info->user_time.seconds =
		    (typeof(basic_info->user_time.seconds))secs;
		basic_info->user_time.microseconds = usecs;

		absolutetime_to_microtime(task->total_system_time, &secs, &usecs);
		basic_info->system_time.seconds =
		    (typeof(basic_info->system_time.seconds))secs;
		basic_info->system_time.microseconds = usecs;

		*task_info_count = TASK_BASIC_INFO_64_2_COUNT;
		break;
	}

#else                                             
	case TASK_BASIC_INFO_64:
	{
		task_basic_info_64_t    basic_info;
		vm_map_t                                map;
		clock_sec_t                             secs;
		clock_usec_t                    usecs;

		if (*task_info_count < TASK_BASIC_INFO_64_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		basic_info = (task_basic_info_64_t)task_info_out;

		map = (task == kernel_task)? kernel_map: task->map;
		basic_info->virtual_size  = vm_map_adjusted_size(map);
		basic_info->resident_size =
		    (mach_vm_size_t)(pmap_resident_count(map->pmap))
		    * PAGE_SIZE_64;

		basic_info->policy = ((task != kernel_task)?
		    POLICY_TIMESHARE: POLICY_RR);
		basic_info->suspend_count = task->user_stop_count;

		absolutetime_to_microtime(task->total_user_time, &secs, &usecs);
		basic_info->user_time.seconds =
		    (typeof(basic_info->user_time.seconds))secs;
		basic_info->user_time.microseconds = usecs;

		absolutetime_to_microtime(task->total_system_time, &secs, &usecs);
		basic_info->system_time.seconds =
		    (typeof(basic_info->system_time.seconds))secs;
		basic_info->system_time.microseconds = usecs;

		*task_info_count = TASK_BASIC_INFO_64_COUNT;
		break;
	}
#endif                                             

	case MACH_TASK_BASIC_INFO:
	{
		mach_task_basic_info_t  basic_info;
		vm_map_t                map;
		clock_sec_t             secs;
		clock_usec_t            usecs;

		if (*task_info_count < MACH_TASK_BASIC_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		basic_info = (mach_task_basic_info_t)task_info_out;

		map = (task == kernel_task) ? kernel_map : task->map;

		basic_info->virtual_size  = vm_map_adjusted_size(map);

		basic_info->resident_size =
		    (mach_vm_size_t)(pmap_resident_count(map->pmap));
		basic_info->resident_size *= PAGE_SIZE_64;

		basic_info->resident_size_max =
		    (mach_vm_size_t)(pmap_resident_max(map->pmap));
		basic_info->resident_size_max *= PAGE_SIZE_64;

		basic_info->policy = ((task != kernel_task) ?
		    POLICY_TIMESHARE : POLICY_RR);

		basic_info->suspend_count = task->user_stop_count;

		absolutetime_to_microtime(task->total_user_time, &secs, &usecs);
		basic_info->user_time.seconds =
		    (typeof(basic_info->user_time.seconds))secs;
		basic_info->user_time.microseconds = usecs;

		absolutetime_to_microtime(task->total_system_time, &secs, &usecs);
		basic_info->system_time.seconds =
		    (typeof(basic_info->system_time.seconds))secs;
		basic_info->system_time.microseconds = usecs;

		*task_info_count = MACH_TASK_BASIC_INFO_COUNT;
		break;
	}

	case TASK_THREAD_TIMES_INFO:
	{
		task_thread_times_info_t        times_info;
		thread_t                                        thread;

		if (*task_info_count < TASK_THREAD_TIMES_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		times_info = (task_thread_times_info_t) task_info_out;
		times_info->user_time.seconds = 0;
		times_info->user_time.microseconds = 0;
		times_info->system_time.seconds = 0;
		times_info->system_time.microseconds = 0;


		queue_iterate(&task->threads, thread, thread_t, task_threads) {
			time_value_t    user_time, system_time;

			if (thread->options & TH_OPT_IDLE_THREAD) {
				continue;
			}

			thread_read_times(thread, &user_time, &system_time, NULL);

			time_value_add(&times_info->user_time, &user_time);
			time_value_add(&times_info->system_time, &system_time);
		}

		*task_info_count = TASK_THREAD_TIMES_INFO_COUNT;
		break;
	}

	case TASK_ABSOLUTETIME_INFO:
	{
		task_absolutetime_info_t        info;
		thread_t                        thread;

		if (*task_info_count < TASK_ABSOLUTETIME_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		info = (task_absolutetime_info_t)task_info_out;
		info->threads_user = info->threads_system = 0;


		info->total_user = task->total_user_time;
		info->total_system = task->total_system_time;

		queue_iterate(&task->threads, thread, thread_t, task_threads) {
			uint64_t        tval;
			spl_t           x;

			if (thread->options & TH_OPT_IDLE_THREAD) {
				continue;
			}

			x = splsched();
			thread_lock(thread);

			tval = timer_grab(&thread->user_timer);
			info->threads_user += tval;
			info->total_user += tval;

			tval = timer_grab(&thread->system_timer);
			if (thread->precise_user_kernel_time) {
				info->threads_system += tval;
				info->total_system += tval;
			} else {
				                                                   
				info->threads_user += tval;
				info->total_user += tval;
			}

			thread_unlock(thread);
			splx(x);
		}


		*task_info_count = TASK_ABSOLUTETIME_INFO_COUNT;
		break;
	}

	case TASK_DYLD_INFO:
	{
		task_dyld_info_t info;

		  
                                                             
                                                           
                                                               
                                           
     
#define TASK_LEGACY_DYLD_INFO_COUNT \
	        offsetof(struct task_dyld_info, all_image_info_format)/sizeof(natural_t)

		if (*task_info_count < TASK_LEGACY_DYLD_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		info = (task_dyld_info_t)task_info_out;
		info->all_image_info_addr = task->all_image_info_addr;
		info->all_image_info_size = task->all_image_info_size;

		                                                      
		if (*task_info_count >= TASK_DYLD_INFO_COUNT) {
			info->all_image_info_format = task_has_64Bit_addr(task) ?
			    TASK_DYLD_ALL_IMAGE_INFO_64 :
			    TASK_DYLD_ALL_IMAGE_INFO_32;
			*task_info_count = TASK_DYLD_INFO_COUNT;
		} else {
			*task_info_count = TASK_LEGACY_DYLD_INFO_COUNT;
		}
		break;
	}

	case TASK_EXTMOD_INFO:
	{
		task_extmod_info_t info;
		void *p;

		if (*task_info_count < TASK_EXTMOD_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		info = (task_extmod_info_t)task_info_out;

		p = get_bsdtask_info(task);
		if (p) {
			proc_getexecutableuuid(p, info->task_uuid, sizeof(info->task_uuid));
		} else {
			bzero(info->task_uuid, sizeof(info->task_uuid));
		}
		info->extmod_statistics = task->extmod_statistics;
		*task_info_count = TASK_EXTMOD_INFO_COUNT;

		break;
	}

	case TASK_KERNELMEMORY_INFO:
	{
		task_kernelmemory_info_t        tkm_info;
		ledger_amount_t                 credit, debit;

		if (*task_info_count < TASK_KERNELMEMORY_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		tkm_info = (task_kernelmemory_info_t) task_info_out;
		tkm_info->total_palloc = 0;
		tkm_info->total_pfree = 0;
		tkm_info->total_salloc = 0;
		tkm_info->total_sfree = 0;

		if (task == kernel_task) {
			  
                                                            
                                                            
                                                       
      
			task_unlock(task);

			                                                                         
			tkm_info->total_palloc = tasks_tkm_private.alloc + tasks_tkm_shared.alloc;
			tkm_info->total_pfree = tasks_tkm_private.free + tasks_tkm_shared.free;

			                                                                      
			lck_mtx_lock(&tasks_threads_lock);

			                                                                                      
			queue_iterate(&tasks, task, task_t, tasks) {
				if (task == kernel_task) {
					if (ledger_get_entries(task->ledger,
					    task_ledgers.tkm_private, &credit,
					    &debit) == KERN_SUCCESS) {
						tkm_info->total_palloc += credit;
						tkm_info->total_pfree += debit;
					}
				}
				if (!ledger_get_entries(task->ledger,
				    task_ledgers.tkm_shared, &credit, &debit)) {
					tkm_info->total_palloc += credit;
					tkm_info->total_pfree += debit;
				}
			}
			lck_mtx_unlock(&tasks_threads_lock);
		} else {
			if (!ledger_get_entries(task->ledger,
			    task_ledgers.tkm_private, &credit, &debit)) {
				tkm_info->total_palloc = credit;
				tkm_info->total_pfree = debit;
			}
			if (!ledger_get_entries(task->ledger,
			    task_ledgers.tkm_shared, &credit, &debit)) {
				tkm_info->total_salloc = credit;
				tkm_info->total_sfree = debit;
			}
			task_unlock(task);
		}

		*task_info_count = TASK_KERNELMEMORY_INFO_COUNT;
		return KERN_SUCCESS;
	}

	              
	case TASK_SCHED_FIFO_INFO:
	{
		if (*task_info_count < POLICY_FIFO_BASE_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		error = KERN_INVALID_POLICY;
		break;
	}

	              
	case TASK_SCHED_RR_INFO:
	{
		policy_rr_base_t        rr_base;
		uint32_t quantum_time;
		uint64_t quantum_ns;

		if (*task_info_count < POLICY_RR_BASE_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		rr_base = (policy_rr_base_t) task_info_out;

		if (task != kernel_task) {
			error = KERN_INVALID_POLICY;
			break;
		}

		rr_base->base_priority = task->priority;

		quantum_time = SCHED(initial_quantum_size)(THREAD_NULL);
		absolutetime_to_nanoseconds(quantum_time, &quantum_ns);

		rr_base->quantum = (uint32_t)(quantum_ns / 1000 / 1000);

		*task_info_count = POLICY_RR_BASE_COUNT;
		break;
	}

	              
	case TASK_SCHED_TIMESHARE_INFO:
	{
		policy_timeshare_base_t ts_base;

		if (*task_info_count < POLICY_TIMESHARE_BASE_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		ts_base = (policy_timeshare_base_t) task_info_out;

		if (task == kernel_task) {
			error = KERN_INVALID_POLICY;
			break;
		}

		ts_base->base_priority = task->priority;

		*task_info_count = POLICY_TIMESHARE_BASE_COUNT;
		break;
	}

	case TASK_SECURITY_TOKEN:
	{
		security_token_t        *sec_token_p;

		if (*task_info_count < TASK_SECURITY_TOKEN_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		sec_token_p = (security_token_t *) task_info_out;

		*sec_token_p = task->sec_token;

		*task_info_count = TASK_SECURITY_TOKEN_COUNT;
		break;
	}

	case TASK_AUDIT_TOKEN:
	{
		audit_token_t   *audit_token_p;

		if (*task_info_count < TASK_AUDIT_TOKEN_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		audit_token_p = (audit_token_t *) task_info_out;

		*audit_token_p = task->audit_token;

		*task_info_count = TASK_AUDIT_TOKEN_COUNT;
		break;
	}

	case TASK_SCHED_INFO:
		error = KERN_INVALID_ARGUMENT;
		break;

	case TASK_EVENTS_INFO:
	{
		task_events_info_t      events_info;
		thread_t                        thread;

		if (*task_info_count < TASK_EVENTS_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		events_info = (task_events_info_t) task_info_out;


		events_info->faults = (int32_t) MIN(counter_load(&task->faults), INT32_MAX);
		events_info->pageins = task->pageins;
		events_info->cow_faults = task->cow_faults;
		events_info->messages_sent = task->messages_sent;
		events_info->messages_received = task->messages_received;
		events_info->syscalls_mach = task->syscalls_mach;
		events_info->syscalls_unix = task->syscalls_unix;

		events_info->csw = task->c_switch;

		queue_iterate(&task->threads, thread, thread_t, task_threads) {
			events_info->csw           += thread->c_switch;
			events_info->syscalls_mach += thread->syscalls_mach;
			events_info->syscalls_unix += thread->syscalls_unix;
		}


		*task_info_count = TASK_EVENTS_INFO_COUNT;
		break;
	}
	case TASK_AFFINITY_TAG_INFO:
	{
		if (*task_info_count < TASK_AFFINITY_TAG_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		error = task_affinity_info(task, task_info_out, task_info_count);
		break;
	}
	case TASK_POWER_INFO:
	{
		if (*task_info_count < TASK_POWER_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		task_power_info_locked(task, (task_power_info_t)task_info_out, NULL, NULL, NULL);
		break;
	}

	case TASK_POWER_INFO_V2:
	{
		if (*task_info_count < TASK_POWER_INFO_V2_COUNT_OLD) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}
		task_power_info_v2_t tpiv2 = (task_power_info_v2_t) task_info_out;
		task_power_info_locked(task, &tpiv2->cpu_energy, &tpiv2->gpu_energy, tpiv2, NULL);
		break;
	}

	case TASK_VM_INFO:
	case TASK_VM_INFO_PURGEABLE:
	{
		task_vm_info_t          vm_info;
		vm_map_t                map;

#if __arm64__
		struct proc *p;
		uint32_t platform, sdk;
		p = current_proc();
		platform = proc_platform(p);
		sdk = proc_min_sdk(p);
		if (original_task_info_count > TASK_VM_INFO_REV2_COUNT &&
		    platform == PLATFORM_IOS &&
		    sdk != 0 &&
		    (sdk >> 16) <= 12) {
			  
                                               
                                                   
                                                
                                                    
                                                         
                                                          
                                                     
                      
      
#if DEVELOPMENT || DEBUG
			printf("%s:%d %d[%s] rdar://49484582 task_info_count %d -> %d platform %d sdk %d.%d.%d\n", __FUNCTION__, __LINE__, proc_pid(p), proc_name_address(p), original_task_info_count, TASK_VM_INFO_REV2_COUNT, platform, (sdk >> 16), ((sdk >> 8) & 0xff), (sdk & 0xff));
#endif                           
			DTRACE_VM4(workaround_task_vm_info_count,
			    mach_msg_type_number_t, original_task_info_count,
			    mach_msg_type_number_t, TASK_VM_INFO_REV2_COUNT,
			    uint32_t, platform,
			    uint32_t, sdk);
			original_task_info_count = TASK_VM_INFO_REV2_COUNT;
			*task_info_count = original_task_info_count;
		}
#endif                

		if (*task_info_count < TASK_VM_INFO_REV0_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		vm_info = (task_vm_info_t)task_info_out;

		  
                                             
                                                   
                                           
     
		if (is_kernel_task) {
			map = kernel_map;
			task_unlock(task);
			                           
		} else {
			map = task->map;
			vm_map_reference(map);
			task_unlock(task);
			vm_map_lock_read(map);
		}

		vm_info->virtual_size = (typeof(vm_info->virtual_size))vm_map_adjusted_size(map);
		vm_info->region_count = map->hdr.nentries;
		vm_info->page_size = vm_map_page_size(map);

		vm_info->resident_size = pmap_resident_count(map->pmap);
		vm_info->resident_size *= PAGE_SIZE;
		vm_info->resident_size_peak = pmap_resident_max(map->pmap);
		vm_info->resident_size_peak *= PAGE_SIZE;

#define _VM_INFO(_name) \
	vm_info->_name = ((mach_vm_size_t) map->pmap->stats._name) * PAGE_SIZE

		_VM_INFO(device);
		_VM_INFO(device_peak);
		_VM_INFO(external);
		_VM_INFO(external_peak);
		_VM_INFO(internal);
		_VM_INFO(internal_peak);
		_VM_INFO(reusable);
		_VM_INFO(reusable_peak);
		_VM_INFO(compressed);
		_VM_INFO(compressed_peak);
		_VM_INFO(compressed_lifetime);

		vm_info->purgeable_volatile_pmap = 0;
		vm_info->purgeable_volatile_resident = 0;
		vm_info->purgeable_volatile_virtual = 0;
		if (is_kernel_task) {
			  
                                                   
                                              
                   
      
			vm_info->internal = vm_info->resident_size;
			  
                                                        
                                                        
                                                         
                                                         
                                                          
                                                         
                                               
      
			vm_info->internal -= (VM_PAGE_COMPRESSOR_COUNT *
			    PAGE_SIZE);
		} else {
			mach_vm_size_t  volatile_virtual_size;
			mach_vm_size_t  volatile_resident_size;
			mach_vm_size_t  volatile_compressed_size;
			mach_vm_size_t  volatile_pmap_size;
			mach_vm_size_t  volatile_compressed_pmap_size;
			kern_return_t   kr;

			if (flavor == TASK_VM_INFO_PURGEABLE) {
				kr = vm_map_query_volatile(
					map,
					&volatile_virtual_size,
					&volatile_resident_size,
					&volatile_compressed_size,
					&volatile_pmap_size,
					&volatile_compressed_pmap_size);
				if (kr == KERN_SUCCESS) {
					vm_info->purgeable_volatile_pmap =
					    volatile_pmap_size;
					if (radar_20146450) {
						vm_info->compressed -=
						    volatile_compressed_pmap_size;
					}
					vm_info->purgeable_volatile_resident =
					    volatile_resident_size;
					vm_info->purgeable_volatile_virtual =
					    volatile_virtual_size;
				}
			}
		}
		*task_info_count = TASK_VM_INFO_REV0_COUNT;

		if (original_task_info_count >= TASK_VM_INFO_REV2_COUNT) {
			                                                       
			vm_info->min_address = map->min_offset;
			vm_info->max_address = map->max_offset;
		}

		  
                                                                  
                                 
    
                                                
     
		if (!is_kernel_task) {
			vm_map_unlock_read(map);
			vm_map_deallocate(map);
		}
		map = VM_MAP_NULL;

		task_lock(task);

		if ((task != current_task()) && (!task->active)) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		if (original_task_info_count >= TASK_VM_INFO_REV1_COUNT) {
			vm_info->phys_footprint =
			    (mach_vm_size_t) get_task_phys_footprint(task);
			*task_info_count = TASK_VM_INFO_REV1_COUNT;
		}
		if (original_task_info_count >= TASK_VM_INFO_REV2_COUNT) {
			                             
			*task_info_count = TASK_VM_INFO_REV2_COUNT;
		}

		if (original_task_info_count >= TASK_VM_INFO_REV3_COUNT) {
			ledger_get_lifetime_max(task->ledger,
			    task_ledgers.phys_footprint,
			    &vm_info->ledger_phys_footprint_peak);
			ledger_get_balance(task->ledger,
			    task_ledgers.purgeable_nonvolatile,
			    &vm_info->ledger_purgeable_nonvolatile);
			ledger_get_balance(task->ledger,
			    task_ledgers.purgeable_nonvolatile_compressed,
			    &vm_info->ledger_purgeable_novolatile_compressed);
			ledger_get_balance(task->ledger,
			    task_ledgers.purgeable_volatile,
			    &vm_info->ledger_purgeable_volatile);
			ledger_get_balance(task->ledger,
			    task_ledgers.purgeable_volatile_compressed,
			    &vm_info->ledger_purgeable_volatile_compressed);
			ledger_get_balance(task->ledger,
			    task_ledgers.network_nonvolatile,
			    &vm_info->ledger_tag_network_nonvolatile);
			ledger_get_balance(task->ledger,
			    task_ledgers.network_nonvolatile_compressed,
			    &vm_info->ledger_tag_network_nonvolatile_compressed);
			ledger_get_balance(task->ledger,
			    task_ledgers.network_volatile,
			    &vm_info->ledger_tag_network_volatile);
			ledger_get_balance(task->ledger,
			    task_ledgers.network_volatile_compressed,
			    &vm_info->ledger_tag_network_volatile_compressed);
			ledger_get_balance(task->ledger,
			    task_ledgers.media_footprint,
			    &vm_info->ledger_tag_media_footprint);
			ledger_get_balance(task->ledger,
			    task_ledgers.media_footprint_compressed,
			    &vm_info->ledger_tag_media_footprint_compressed);
			ledger_get_balance(task->ledger,
			    task_ledgers.media_nofootprint,
			    &vm_info->ledger_tag_media_nofootprint);
			ledger_get_balance(task->ledger,
			    task_ledgers.media_nofootprint_compressed,
			    &vm_info->ledger_tag_media_nofootprint_compressed);
			ledger_get_balance(task->ledger,
			    task_ledgers.graphics_footprint,
			    &vm_info->ledger_tag_graphics_footprint);
			ledger_get_balance(task->ledger,
			    task_ledgers.graphics_footprint_compressed,
			    &vm_info->ledger_tag_graphics_footprint_compressed);
			ledger_get_balance(task->ledger,
			    task_ledgers.graphics_nofootprint,
			    &vm_info->ledger_tag_graphics_nofootprint);
			ledger_get_balance(task->ledger,
			    task_ledgers.graphics_nofootprint_compressed,
			    &vm_info->ledger_tag_graphics_nofootprint_compressed);
			ledger_get_balance(task->ledger,
			    task_ledgers.neural_footprint,
			    &vm_info->ledger_tag_neural_footprint);
			ledger_get_balance(task->ledger,
			    task_ledgers.neural_footprint_compressed,
			    &vm_info->ledger_tag_neural_footprint_compressed);
			ledger_get_balance(task->ledger,
			    task_ledgers.neural_nofootprint,
			    &vm_info->ledger_tag_neural_nofootprint);
			ledger_get_balance(task->ledger,
			    task_ledgers.neural_nofootprint_compressed,
			    &vm_info->ledger_tag_neural_nofootprint_compressed);
			*task_info_count = TASK_VM_INFO_REV3_COUNT;
		}
		if (original_task_info_count >= TASK_VM_INFO_REV4_COUNT) {
			if (task->bsd_info) {
				vm_info->limit_bytes_remaining =
				    memorystatus_available_memory_internal(task->bsd_info);
			} else {
				vm_info->limit_bytes_remaining = 0;
			}
			*task_info_count = TASK_VM_INFO_REV4_COUNT;
		}
		if (original_task_info_count >= TASK_VM_INFO_REV5_COUNT) {
			thread_t thread;
			integer_t total = task->decompressions;
			queue_iterate(&task->threads, thread, thread_t, task_threads) {
				total += thread->decompressions;
			}
			vm_info->decompressions = total;
			*task_info_count = TASK_VM_INFO_REV5_COUNT;
		}

		break;
	}

	case TASK_WAIT_STATE_INFO:
	{
		  
                                                                       
                                                      
     
		task_wait_state_info_t  wait_state_info;
		uint64_t total_sfi_ledger_val = 0;

		if (*task_info_count < TASK_WAIT_STATE_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		wait_state_info = (task_wait_state_info_t) task_info_out;

		wait_state_info->total_wait_state_time = 0;
		bzero(wait_state_info->_reserved, sizeof(wait_state_info->_reserved));

#if CONFIG_SCHED_SFI
		int i, prev_lentry = -1;
		int64_t  val_credit, val_debit;

		for (i = 0; i < MAX_SFI_CLASS_ID; i++) {
			val_credit = 0;
			  
                                                                 
                                                              
                                                                        
      
			if (prev_lentry != task_ledgers.sfi_wait_times[i] &&
			    KERN_SUCCESS == ledger_get_entries(task->ledger,
			    task_ledgers.sfi_wait_times[i], &val_credit, &val_debit)) {
				total_sfi_ledger_val += val_credit;
			}
			prev_lentry = task_ledgers.sfi_wait_times[i];
		}

#endif                       
		wait_state_info->total_wait_sfi_state_time = total_sfi_ledger_val;
		*task_info_count = TASK_WAIT_STATE_INFO_COUNT;

		break;
	}
	case TASK_VM_INFO_PURGEABLE_ACCOUNT:
	{
#if DEVELOPMENT || DEBUG
		pvm_account_info_t      acnt_info;

		if (*task_info_count < PVM_ACCOUNT_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		if (task_info_out == NULL) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		acnt_info = (pvm_account_info_t) task_info_out;

		error = vm_purgeable_account(task, acnt_info);

		*task_info_count = PVM_ACCOUNT_INFO_COUNT;

		break;
#else                           
		error = KERN_NOT_SUPPORTED;
		break;
#endif                           
	}
	case TASK_FLAGS_INFO:
	{
		task_flags_info_t               flags_info;

		if (*task_info_count < TASK_FLAGS_INFO_COUNT) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}

		flags_info = (task_flags_info_t)task_info_out;

		                                              
		flags_info->flags = task->t_flags & (TF_64B_ADDR | TF_64B_DATA);

		*task_info_count = TASK_FLAGS_INFO_COUNT;
		break;
	}

	case TASK_DEBUG_INFO_INTERNAL:
	{
#if DEVELOPMENT || DEBUG
		task_debug_info_internal_t dbg_info;
		ipc_space_t space = task->itk_space;
		if (*task_info_count < TASK_DEBUG_INFO_INTERNAL_COUNT) {
			error = KERN_NOT_SUPPORTED;
			break;
		}

		if (task_info_out == NULL) {
			error = KERN_INVALID_ARGUMENT;
			break;
		}
		dbg_info = (task_debug_info_internal_t) task_info_out;
		dbg_info->ipc_space_size = 0;

		if (space) {
			is_read_lock(space);
			dbg_info->ipc_space_size = space->is_table_size;
			is_read_unlock(space);
		}

		dbg_info->suspend_count = task->suspend_count;

		error = KERN_SUCCESS;
		*task_info_count = TASK_DEBUG_INFO_INTERNAL_COUNT;
		break;
#else                           
		error = KERN_NOT_SUPPORTED;
		break;
#endif                           
	}
	default:
		error = KERN_INVALID_ARGUMENT;
	}

	task_unlock(task);
	return error;
}

  
                      
  
                                          
                                                    
                                              
                                                      
                       
  
                                                     
                                                                    
  
   
kern_return_t
task_info_from_user(
	mach_port_t             task_port,
	task_flavor_t           flavor,
	task_info_t             task_info_out,
	mach_msg_type_number_t  *task_info_count)
{
	task_t task;
	kern_return_t ret;

	if (flavor == TASK_DYLD_INFO) {
		task = convert_port_to_task_read(task_port);
	} else {
		task = convert_port_to_task_name(task_port);
	}

	ret = task_info(task, flavor, task_info_out, task_info_count);

	task_deallocate(task);

	return ret;
}

  
                                                
  
                                        
  
                                                                                
                                
  
             
                                                               
                                                                     
   
void
task_dyld_process_info_update_helper(
	task_t                  task,
	size_t                  active_count,
	vm_map_address_t        magic_addr,                             
	ipc_port_t             *release_ports,
	size_t                  release_count)
{
	void *notifiers_ptr = NULL;

	assert(release_count <= DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT);

	if (active_count == 0) {
		assert(task->itk_dyld_notify != NULL);
		notifiers_ptr = task->itk_dyld_notify;
		task->itk_dyld_notify = NULL;
		itk_unlock(task);

		kfree(notifiers_ptr, (vm_size_t)sizeof(ipc_port_t) * DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT);
		(void)copyoutmap_atomic32(task->map, MACH_PORT_NULL, magic_addr);                  
	} else {
		itk_unlock(task);
		(void)copyoutmap_atomic32(task->map, (mach_port_name_t)DYLD_PROCESS_INFO_NOTIFY_MAGIC,
		    magic_addr);                      
	}

	lck_mtx_unlock(&g_dyldinfo_mtx);

	for (size_t i = 0; i < release_count; i++) {
		ipc_port_release_send(release_ports[i]);
	}
}

  
                                                  
  
                                                                              
                                                                             
                                      
  
                                                                          
  
        
                                                
                            
  
           
                                            
                                          
                                             
                                                     
                                                               
                                                                         
                                                                             
  
                                        
  
            
                                                                   
   
kern_return_t
task_dyld_process_info_notify_register(
	task_t                  task,
	ipc_port_t              sright)
{
	struct task_dyld_info dyld_info;
	mach_msg_type_number_t info_count = TASK_DYLD_INFO_COUNT;
	ipc_port_t release_ports[DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT];
	uint32_t release_count = 0, active_count = 0;
	mach_vm_address_t ports_addr;                           
	kern_return_t kr;
	boolean_t right_exists = false;
	ipc_port_t *notifiers_ptr = NULL;
	ipc_port_t *portp;

	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_TASK;
	}

	if (!IP_VALID(sright)) {
		return KERN_INVALID_RIGHT;
	}

#if CONFIG_MACF
	if (mac_task_check_dyld_process_info_notify_register()) {
		return KERN_DENIED;
	}
#endif

	kr = task_info(task, TASK_DYLD_INFO, (task_info_t)&dyld_info, &info_count);
	if (kr) {
		return kr;
	}

	if (dyld_info.all_image_info_format == TASK_DYLD_ALL_IMAGE_INFO_32) {
		ports_addr = (mach_vm_address_t)(dyld_info.all_image_info_addr +
		    offsetof(struct user32_dyld_all_image_infos, notifyMachPorts));
	} else {
		ports_addr = (mach_vm_address_t)(dyld_info.all_image_info_addr +
		    offsetof(struct user64_dyld_all_image_infos, notifyMachPorts));
	}

	if (task->itk_dyld_notify == NULL) {
		notifiers_ptr = (ipc_port_t *)
		    kalloc_flags(sizeof(ipc_port_t) * DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT, Z_ZERO);
		if (!notifiers_ptr) {
			return KERN_RESOURCE_SHORTAGE;
		}
	}

	lck_mtx_lock(&g_dyldinfo_mtx);
	itk_lock(task);

	if (task->itk_dyld_notify == NULL) {
		task->itk_dyld_notify = notifiers_ptr;
		notifiers_ptr = NULL;
	}

	assert(task->itk_dyld_notify != NULL);
	                                                                       
	for (int slot = 0; slot < DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT; slot++) {
		portp = &task->itk_dyld_notify[slot];
		if (*portp != IPC_PORT_NULL && !ip_active(*portp)) {
			release_ports[release_count++] = *portp;
			*portp = IPC_PORT_NULL;
		} else if (*portp == sright) {
			                                                  
			right_exists = true;
		}

		if (*portp != IPC_PORT_NULL) {
			active_count++;
		}
	}

	if (right_exists) {
		                      
		kr = KERN_RIGHT_EXISTS;
		goto out;
	}

	                                    
	kr = KERN_NO_SPACE;
	for (int slot = 0; slot < DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT; slot++) {
		portp = &task->itk_dyld_notify[slot];
		if (*portp == IPC_PORT_NULL) {
			*portp = sright;
			active_count++;
			kr = KERN_SUCCESS;
			break;
		}
	}

out:
	assert(active_count > 0);

	task_dyld_process_info_update_helper(task, active_count,
	    (vm_map_address_t)ports_addr, release_ports, release_count);
	                                                       

	if (notifiers_ptr) {
		kfree(notifiers_ptr, sizeof(ipc_port_t) * DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT);
	}

	return kr;
}

  
                                                    
  
                                                                                  
                                                                                        
                                                                                            
  
                                  
  
        
                                                
                                                                       
  
           
                                                                       
                                          
                                          
                                                     
                                                   
                                                                                   
  
                                        
  
            
                                                                   
   
kern_return_t
task_dyld_process_info_notify_deregister(
	task_t                  task,
	mach_port_name_t        rcv_name)
{
	struct task_dyld_info dyld_info;
	mach_msg_type_number_t info_count = TASK_DYLD_INFO_COUNT;
	ipc_port_t release_ports[DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT];
	uint32_t release_count = 0, active_count = 0;
	boolean_t port_found = false;
	mach_vm_address_t ports_addr;                           
	ipc_port_t sright;
	kern_return_t kr;
	ipc_port_t *portp;

	if (task == TASK_NULL || task == kernel_task) {
		return KERN_INVALID_TASK;
	}

	if (!MACH_PORT_VALID(rcv_name)) {
		return KERN_INVALID_NAME;
	}

#if CONFIG_MACF
	if (mac_task_check_dyld_process_info_notify_register()) {
		return KERN_DENIED;
	}
#endif

	kr = task_info(task, TASK_DYLD_INFO, (task_info_t)&dyld_info, &info_count);
	if (kr) {
		return kr;
	}

	if (dyld_info.all_image_info_format == TASK_DYLD_ALL_IMAGE_INFO_32) {
		ports_addr = (mach_vm_address_t)(dyld_info.all_image_info_addr +
		    offsetof(struct user32_dyld_all_image_infos, notifyMachPorts));
	} else {
		ports_addr = (mach_vm_address_t)(dyld_info.all_image_info_addr +
		    offsetof(struct user64_dyld_all_image_infos, notifyMachPorts));
	}

	kr = ipc_port_translate_receive(current_space(), rcv_name, &sright);                                
	if (kr) {
		return KERN_INVALID_RIGHT;
	}

	ip_reference(sright);
	ip_unlock(sright);

	assert(sright != IPC_PORT_NULL);

	lck_mtx_lock(&g_dyldinfo_mtx);
	itk_lock(task);

	if (task->itk_dyld_notify == NULL) {
		itk_unlock(task);
		lck_mtx_unlock(&g_dyldinfo_mtx);
		ip_release(sright);
		return KERN_FAILURE;
	}

	for (int slot = 0; slot < DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT; slot++) {
		portp = &task->itk_dyld_notify[slot];
		if (*portp == sright) {
			release_ports[release_count++] = *portp;
			*portp = IPC_PORT_NULL;
			port_found = true;
		} else if ((*portp != IPC_PORT_NULL && !ip_active(*portp))) {
			release_ports[release_count++] = *portp;
			*portp = IPC_PORT_NULL;
		}

		if (*portp != IPC_PORT_NULL) {
			active_count++;
		}
	}

	task_dyld_process_info_update_helper(task, active_count,
	    (vm_map_address_t)ports_addr, release_ports, release_count);
	                                                       

	ip_release(sright);

	return port_found ? KERN_SUCCESS : KERN_FAILURE;
}

  
                  
  
                                    
                                 
   
void
task_power_info_locked(
	task_t                  task,
	task_power_info_t       info,
	gpu_energy_data_t       ginfo,
	task_power_info_v2_t    infov2,
	uint64_t                *runnable_time)
{
	thread_t                thread;
	ledger_amount_t         tmp;

	uint64_t                runnable_time_sum = 0;

	task_lock_assert_owned(task);

	ledger_get_entries(task->ledger, task_ledgers.interrupt_wakeups,
	    (ledger_amount_t *)&info->task_interrupt_wakeups, &tmp);
	ledger_get_entries(task->ledger, task_ledgers.platform_idle_wakeups,
	    (ledger_amount_t *)&info->task_platform_idle_wakeups, &tmp);

	info->task_timer_wakeups_bin_1 = task->task_timer_wakeups_bin_1;
	info->task_timer_wakeups_bin_2 = task->task_timer_wakeups_bin_2;

	info->total_user = task->total_user_time;
	info->total_system = task->total_system_time;
	runnable_time_sum = task->total_runnable_time;

#if defined(__arm__) || defined(__arm64__)
	if (infov2) {
		infov2->task_energy = task->task_energy;
	}
#endif                                             

	if (ginfo) {
		ginfo->task_gpu_utilisation = task->task_gpu_ns;
	}

	if (infov2) {
		infov2->task_ptime = task->total_ptime;
		infov2->task_pset_switches = task->ps_switch;
	}

	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		uint64_t        tval;
		spl_t           x;

		if (thread->options & TH_OPT_IDLE_THREAD) {
			continue;
		}

		x = splsched();
		thread_lock(thread);

		info->task_timer_wakeups_bin_1 += thread->thread_timer_wakeups_bin_1;
		info->task_timer_wakeups_bin_2 += thread->thread_timer_wakeups_bin_2;

#if defined(__arm__) || defined(__arm64__)
		if (infov2) {
			infov2->task_energy += ml_energy_stat(thread);
		}
#endif                                             

		tval = timer_grab(&thread->user_timer);
		info->total_user += tval;

		if (infov2) {
			tval = timer_grab(&thread->ptime);
			infov2->task_ptime += tval;
			infov2->task_pset_switches += thread->ps_switch;
		}

		tval = timer_grab(&thread->system_timer);
		if (thread->precise_user_kernel_time) {
			info->total_system += tval;
		} else {
			                                                   
			info->total_user += tval;
		}

		tval = timer_grab(&thread->runnable_timer);

		runnable_time_sum += tval;

		if (ginfo) {
			ginfo->task_gpu_utilisation += ml_gpu_stat(thread);
		}
		thread_unlock(thread);
		splx(x);
	}

	if (runnable_time) {
		*runnable_time = runnable_time_sum;
	}
}

  
                       
  
                                                                     
                         
   
uint64_t
task_gpu_utilisation(
	task_t  task)
{
	uint64_t gpu_time = 0;
#if defined(__x86_64__)
	thread_t thread;

	task_lock(task);
	gpu_time += task->task_gpu_ns;

	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		spl_t x;
		x = splsched();
		thread_lock(thread);
		gpu_time += ml_gpu_stat(thread);
		thread_unlock(thread);
		splx(x);
	}

	task_unlock(task);
#else                          
	                              
	(void)task;
#endif                          
	return gpu_time;
}

  
              
  
                                                                   
                         
   
uint64_t
task_energy(
	task_t  task)
{
	uint64_t energy = 0;
	thread_t thread;

	task_lock(task);
	energy += task->task_energy;

	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		spl_t x;
		x = splsched();
		thread_lock(thread);
		energy += ml_energy_stat(thread);
		thread_unlock(thread);
		splx(x);
	}

	task_unlock(task);
	return energy;
}

#if __AMP__

uint64_t
task_cpu_ptime(
	task_t  task)
{
	uint64_t cpu_ptime = 0;
	thread_t thread;

	task_lock(task);
	cpu_ptime += task->total_ptime;

	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		cpu_ptime += timer_grab(&thread->ptime);
	}

	task_unlock(task);
	return cpu_ptime;
}

#else              

uint64_t
task_cpu_ptime(
	__unused task_t  task)
{
	return 0;
}

#endif              

                                                            
                                    
   
void
task_update_cpu_time_qos_stats(
	task_t  task,
	uint64_t *eqos_stats,
	uint64_t *rqos_stats)
{
	if (!eqos_stats && !rqos_stats) {
		return;
	}

	task_lock(task);
	thread_t thread;
	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		if (thread->options & TH_OPT_IDLE_THREAD) {
			continue;
		}

		thread_update_qos_cpu_time(thread);
	}

	if (eqos_stats) {
		eqos_stats[THREAD_QOS_DEFAULT] += task->cpu_time_eqos_stats.cpu_time_qos_default;
		eqos_stats[THREAD_QOS_MAINTENANCE] += task->cpu_time_eqos_stats.cpu_time_qos_maintenance;
		eqos_stats[THREAD_QOS_BACKGROUND] += task->cpu_time_eqos_stats.cpu_time_qos_background;
		eqos_stats[THREAD_QOS_UTILITY] += task->cpu_time_eqos_stats.cpu_time_qos_utility;
		eqos_stats[THREAD_QOS_LEGACY] += task->cpu_time_eqos_stats.cpu_time_qos_legacy;
		eqos_stats[THREAD_QOS_USER_INITIATED] += task->cpu_time_eqos_stats.cpu_time_qos_user_initiated;
		eqos_stats[THREAD_QOS_USER_INTERACTIVE] += task->cpu_time_eqos_stats.cpu_time_qos_user_interactive;
	}

	if (rqos_stats) {
		rqos_stats[THREAD_QOS_DEFAULT] += task->cpu_time_rqos_stats.cpu_time_qos_default;
		rqos_stats[THREAD_QOS_MAINTENANCE] += task->cpu_time_rqos_stats.cpu_time_qos_maintenance;
		rqos_stats[THREAD_QOS_BACKGROUND] += task->cpu_time_rqos_stats.cpu_time_qos_background;
		rqos_stats[THREAD_QOS_UTILITY] += task->cpu_time_rqos_stats.cpu_time_qos_utility;
		rqos_stats[THREAD_QOS_LEGACY] += task->cpu_time_rqos_stats.cpu_time_qos_legacy;
		rqos_stats[THREAD_QOS_USER_INITIATED] += task->cpu_time_rqos_stats.cpu_time_qos_user_initiated;
		rqos_stats[THREAD_QOS_USER_INTERACTIVE] += task->cpu_time_rqos_stats.cpu_time_qos_user_interactive;
	}

	task_unlock(task);
}

kern_return_t
task_purgable_info(
	task_t                  task,
	task_purgable_info_t    *stats)
{
	if (task == TASK_NULL || stats == NULL) {
		return KERN_INVALID_ARGUMENT;
	}
	                         
	task_reference(task);
	vm_purgeable_stats((vm_purgeable_info_t)stats, task);
	                         
	task_deallocate(task);
	return KERN_SUCCESS;
}

void
task_vtimer_set(
	task_t          task,
	integer_t       which)
{
	thread_t        thread;
	spl_t           x;

	task_lock(task);

	task->vtimers |= which;

	switch (which) {
	case TASK_VTIMER_USER:
		queue_iterate(&task->threads, thread, thread_t, task_threads) {
			x = splsched();
			thread_lock(thread);
			if (thread->precise_user_kernel_time) {
				thread->vtimer_user_save = timer_grab(&thread->user_timer);
			} else {
				thread->vtimer_user_save = timer_grab(&thread->system_timer);
			}
			thread_unlock(thread);
			splx(x);
		}
		break;

	case TASK_VTIMER_PROF:
		queue_iterate(&task->threads, thread, thread_t, task_threads) {
			x = splsched();
			thread_lock(thread);
			thread->vtimer_prof_save = timer_grab(&thread->user_timer);
			thread->vtimer_prof_save += timer_grab(&thread->system_timer);
			thread_unlock(thread);
			splx(x);
		}
		break;

	case TASK_VTIMER_RLIM:
		queue_iterate(&task->threads, thread, thread_t, task_threads) {
			x = splsched();
			thread_lock(thread);
			thread->vtimer_rlim_save = timer_grab(&thread->user_timer);
			thread->vtimer_rlim_save += timer_grab(&thread->system_timer);
			thread_unlock(thread);
			splx(x);
		}
		break;
	}

	task_unlock(task);
}

void
task_vtimer_clear(
	task_t          task,
	integer_t       which)
{
	assert(task == current_task());

	task_lock(task);

	task->vtimers &= ~which;

	task_unlock(task);
}

void
task_vtimer_update(
	__unused
	task_t          task,
	integer_t       which,
	uint32_t        *microsecs)
{
	thread_t        thread = current_thread();
	uint32_t        tdelt = 0;
	clock_sec_t     secs = 0;
	uint64_t        tsum;

	assert(task == current_task());

	spl_t s = splsched();
	thread_lock(thread);

	if ((task->vtimers & which) != (uint32_t)which) {
		thread_unlock(thread);
		splx(s);
		return;
	}

	switch (which) {
	case TASK_VTIMER_USER:
		if (thread->precise_user_kernel_time) {
			tdelt = (uint32_t)timer_delta(&thread->user_timer,
			    &thread->vtimer_user_save);
		} else {
			tdelt = (uint32_t)timer_delta(&thread->system_timer,
			    &thread->vtimer_user_save);
		}
		absolutetime_to_microtime(tdelt, &secs, microsecs);
		break;

	case TASK_VTIMER_PROF:
		tsum = timer_grab(&thread->user_timer);
		tsum += timer_grab(&thread->system_timer);
		tdelt = (uint32_t)(tsum - thread->vtimer_prof_save);
		absolutetime_to_microtime(tdelt, &secs, microsecs);
		                                                      
		if (*microsecs != 0) {
			thread->vtimer_prof_save = tsum;
		}
		break;

	case TASK_VTIMER_RLIM:
		tsum = timer_grab(&thread->user_timer);
		tsum += timer_grab(&thread->system_timer);
		tdelt = (uint32_t)(tsum - thread->vtimer_rlim_save);
		thread->vtimer_rlim_save = tsum;
		absolutetime_to_microtime(tdelt, &secs, microsecs);
		break;
	}

	thread_unlock(thread);
	splx(s);
}

  
               
  
                                                 
   
kern_return_t
task_assign(
	__unused task_t         task,
	__unused processor_set_t        new_pset,
	__unused boolean_t      assign_threads)
{
	return KERN_FAILURE;
}

  
                       
  
                                                             
   
kern_return_t
task_assign_default(
	task_t          task,
	boolean_t       assign_threads)
{
	return task_assign(task, &pset0, assign_threads);
}

  
                      
  
                                                         
   
kern_return_t
task_get_assignment(
	task_t          task,
	processor_set_t *pset)
{
	if (!task || !task->active) {
		return KERN_FAILURE;
	}

	*pset = &pset0;

	return KERN_SUCCESS;
}

uint64_t
get_task_dispatchqueue_offset(
	task_t          task)
{
	return task->dispatchqueue_offset;
}

  
                   
  
                                                                 
                                                                   
                                                        
   
kern_return_t
task_policy(
	__unused task_t                 task,
	__unused policy_t                       policy_id,
	__unused policy_base_t          base,
	__unused mach_msg_type_number_t count,
	__unused boolean_t                      set_limit,
	__unused boolean_t                      change)
{
	return KERN_FAILURE;
}

  
                  
  
                                                                 
                                                              
                                                                  
                
   
kern_return_t
task_set_policy(
	__unused task_t                 task,
	__unused processor_set_t                pset,
	__unused policy_t                       policy_id,
	__unused policy_base_t          base,
	__unused mach_msg_type_number_t base_count,
	__unused policy_limit_t         limit,
	__unused mach_msg_type_number_t limit_count,
	__unused boolean_t                      change)
{
	return KERN_FAILURE;
}

kern_return_t
task_set_ras_pc(
	__unused task_t task,
	__unused vm_offset_t    pc,
	__unused vm_offset_t    endpc)
{
	return KERN_FAILURE;
}

void
task_synchronizer_destroy_all(task_t task)
{
	  
                             
    
	semaphore_destroy_all(task);
}

  
                                                           
                                                                  
                                                                
                                                                             
   
kern_return_t
task_set_state(
	task_t task,
	int flavor,
	thread_state_t state,
	mach_msg_type_number_t state_count)
{
	kern_return_t ret;

	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);

	if (!task->active) {
		task_unlock(task);
		return KERN_FAILURE;
	}

	ret = machine_task_set_state(task, flavor, state, state_count);

	task_unlock(task);
	return ret;
}

  
                                                               
                                                                   
                                                              
   
kern_return_t
task_get_state(
	task_t  task,
	int     flavor,
	thread_state_t state,
	mach_msg_type_number_t *state_count)
{
	kern_return_t ret;

	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);

	if (!task->active) {
		task_unlock(task);
		return KERN_FAILURE;
	}

	ret = machine_task_get_state(task, flavor, state, state_count);

	task_unlock(task);
	return ret;
}


static kern_return_t __attribute__((noinline, not_tail_called))
PROC_VIOLATED_GUARD__SEND_EXC_GUARD_AND_SUSPEND(
	mach_exception_code_t code,
	mach_exception_subcode_t subcode,
	void *reason)
{
#ifdef MACH_BSD
	if (1 == proc_selfpid()) {
		return KERN_NOT_SUPPORTED;                                   
	}
#endif
	mach_exception_data_type_t codes[EXCEPTION_CODE_MAX] = {
		[0] = code,
		[1] = subcode,
	};
	task_t task = current_task();
	kern_return_t kr;

	                                         

	proc_memstat_terminated(task->bsd_info, TRUE);
	kr = task_enqueue_exception_with_corpse(task, EXC_GUARD, codes, 2, reason);
	proc_memstat_terminated(task->bsd_info, FALSE);
	return kr;
}

kern_return_t
task_violated_guard(
	mach_exception_code_t code,
	mach_exception_subcode_t subcode,
	void *reason)
{
	return PROC_VIOLATED_GUARD__SEND_EXC_GUARD_AND_SUSPEND(code, subcode, reason);
}


#if CONFIG_MEMORYSTATUS

boolean_t
task_get_memlimit_is_active(task_t task)
{
	assert(task != NULL);

	if (task->memlimit_is_active == 1) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void
task_set_memlimit_is_active(task_t task, boolean_t memlimit_is_active)
{
	assert(task != NULL);

	if (memlimit_is_active) {
		task->memlimit_is_active = 1;
	} else {
		task->memlimit_is_active = 0;
	}
}

boolean_t
task_get_memlimit_is_fatal(task_t task)
{
	assert(task != NULL);

	if (task->memlimit_is_fatal == 1) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void
task_set_memlimit_is_fatal(task_t task, boolean_t memlimit_is_fatal)
{
	assert(task != NULL);

	if (memlimit_is_fatal) {
		task->memlimit_is_fatal = 1;
	} else {
		task->memlimit_is_fatal = 0;
	}
}

boolean_t
task_has_triggered_exc_resource(task_t task, boolean_t memlimit_is_active)
{
	boolean_t triggered = FALSE;

	assert(task == current_task());

	  
                                                                          
    

	if (memlimit_is_active) {
		triggered = (task->memlimit_active_exc_resource ? TRUE : FALSE);
	} else {
		triggered = (task->memlimit_inactive_exc_resource ? TRUE : FALSE);
	}

	return triggered;
}

void
task_mark_has_triggered_exc_resource(task_t task, boolean_t memlimit_is_active)
{
	assert(task == current_task());

	  
                                                                    
                                                        
    

	if (memlimit_is_active) {
		task->memlimit_active_exc_resource = 1;
	} else {
		task->memlimit_inactive_exc_resource = 1;
	}
}

#define HWM_USERCORE_MINSPACE 250                                                          

void __attribute__((noinline))
PROC_CROSSED_HIGH_WATERMARK__SEND_EXC_RESOURCE_AND_SUSPEND(int max_footprint_mb, boolean_t is_fatal)
{
	task_t                                          task            = current_task();
	int                                                     pid         = 0;
	const char                                      *procname       = "unknown";
	mach_exception_data_type_t      code[EXCEPTION_CODE_MAX];
	boolean_t send_sync_exc_resource = FALSE;

#ifdef MACH_BSD
	pid = proc_selfpid();

	if (pid == 1) {
		  
                                      
                          
     
		return;
	}

	if (task->bsd_info != NULL) {
		procname = proc_name_address(current_task()->bsd_info);
		send_sync_exc_resource = proc_send_synchronous_EXC_RESOURCE(current_task()->bsd_info);
	}
#endif
#if CONFIG_COREDUMP
	if (hwm_user_cores) {
		int                             error;
		uint64_t                starttime, end;
		clock_sec_t             secs = 0;
		uint32_t                microsecs = 0;

		starttime = mach_absolute_time();
		  
                                                                              
                                                                             
               
     
		if ((error = coredump(current_task()->bsd_info, HWM_USERCORE_MINSPACE, COREDUMP_IGNORE_ULIMIT)) != 0) {
			printf("couldn't take coredump of %s[%d]: %d\n", procname, pid, error);
		}
		  
                                          
     
		task_resume_internal(current_task());

		end = mach_absolute_time();
		absolutetime_to_microtime(end - starttime, &secs, &microsecs);
		printf("coredump of %s[%d] taken in %d secs %d microsecs\n",
		    proc_name_address(current_task()->bsd_info), pid, (int)secs, microsecs);
	}
#endif                      

	if (disable_exc_resource) {
		printf("process %s[%d] crossed memory high watermark (%d MB); EXC_RESOURCE "
		    "supressed by a boot-arg.\n", procname, pid, max_footprint_mb);
		return;
	}

	  
                                                            
                                                             
                                                            
                                                        
    
	proc_memstat_terminated(current_task()->bsd_info, TRUE);

	code[0] = code[1] = 0;
	EXC_RESOURCE_ENCODE_TYPE(code[0], RESOURCE_TYPE_MEMORY);
	EXC_RESOURCE_ENCODE_FLAVOR(code[0], FLAVOR_HIGH_WATERMARK);
	EXC_RESOURCE_HWM_ENCODE_LIMIT(code[0], max_footprint_mb);

	  
                                                                 
                                                             
    
	if (is_fatal || send_sync_exc_resource || exc_via_corpse_forking == 0) {
		                                                                   
		if (send_sync_exc_resource || corpse_for_fatal_memkill == 0) {
			  
                                                      
                                                
      
			task_suspend_internal(task);
			exception_triage(EXC_RESOURCE, code, EXCEPTION_CODE_MAX);
			task_resume_internal(task);
		}
	} else {
		if (audio_active) {
			printf("process %s[%d] crossed memory high watermark (%d MB); EXC_RESOURCE "
			    "supressed due to audio playback.\n", procname, pid, max_footprint_mb);
		} else {
			task_enqueue_exception_with_corpse(task, EXC_RESOURCE,
			    code, EXCEPTION_CODE_MAX, NULL);
		}
	}

	  
                                                              
                                                              
                                                              
    
	proc_memstat_terminated(current_task()->bsd_info, FALSE);                      
}

  
                                                                     
   
void
task_footprint_exceeded(int warning, __unused const void *param0, __unused const void *param1)
{
	ledger_amount_t max_footprint, max_footprint_mb;
	task_t task;
	boolean_t is_warning;
	boolean_t memlimit_is_active;
	boolean_t memlimit_is_fatal;

	if (warning == LEDGER_WARNING_DIPPED_BELOW) {
		  
                                                             
     
		return;
	} else if (warning == LEDGER_WARNING_ROSE_ABOVE) {
		  
                                                        
                                                     
     
		is_warning = TRUE;
	} else {
		  
                                                        
                                                      
     
		is_warning = FALSE;
	}

	task = current_task();

	ledger_get_limit(task->ledger, task_ledgers.phys_footprint, &max_footprint);
	max_footprint_mb = max_footprint >> 20;

	memlimit_is_active = task_get_memlimit_is_active(task);
	memlimit_is_fatal = task_get_memlimit_is_fatal(task);

	  
                                                                                         
                                                                                         
                                                                                        
                                                                      
    
	if ((is_warning == FALSE) && (!task_has_triggered_exc_resource(task, memlimit_is_active))) {
		PROC_CROSSED_HIGH_WATERMARK__SEND_EXC_RESOURCE_AND_SUSPEND((int)max_footprint_mb, memlimit_is_fatal);
		memorystatus_log_exception((int)max_footprint_mb, memlimit_is_active, memlimit_is_fatal);
		task_mark_has_triggered_exc_resource(task, memlimit_is_active);
	}

	memorystatus_on_ledger_footprint_exceeded(is_warning, memlimit_is_active, memlimit_is_fatal);
}

extern int proc_check_footprint_priv(void);

kern_return_t
task_set_phys_footprint_limit(
	task_t task,
	int new_limit_mb,
	int *old_limit_mb)
{
	kern_return_t error;

	boolean_t memlimit_is_active;
	boolean_t memlimit_is_fatal;

	if ((error = proc_check_footprint_priv())) {
		return KERN_NO_ACCESS;
	}

	  
                                           
                                             
    
	memlimit_is_active = task_get_memlimit_is_active(task);
	memlimit_is_fatal = task_get_memlimit_is_fatal(task);

	return task_set_phys_footprint_limit_internal(task, new_limit_mb, old_limit_mb, memlimit_is_active, memlimit_is_fatal);
}

kern_return_t
task_convert_phys_footprint_limit(
	int limit_mb,
	int *converted_limit_mb)
{
	if (limit_mb == -1) {
		  
             
     
		if (max_task_footprint != 0) {
			*converted_limit_mb = (int)(max_task_footprint / 1024 / 1024);                    
		} else {
			*converted_limit_mb = (int)(LEDGER_LIMIT_INFINITY >> 20);
		}
	} else {
		                        
		*converted_limit_mb = limit_mb;
	}
	return KERN_SUCCESS;
}


kern_return_t
task_set_phys_footprint_limit_internal(
	task_t task,
	int new_limit_mb,
	int *old_limit_mb,
	boolean_t memlimit_is_active,
	boolean_t memlimit_is_fatal)
{
	ledger_amount_t old;
	kern_return_t ret;

	ret = ledger_get_limit(task->ledger, task_ledgers.phys_footprint, &old);

	if (ret != KERN_SUCCESS) {
		return ret;
	}

	  
                                                               
                                                                     
                                     
    
	assert(((old & 0xFFF0000000000000LL) == 0) || (old == LEDGER_LIMIT_INFINITY));

	if (old_limit_mb) {
		*old_limit_mb = (int)(old >> 20);
	}

	if (new_limit_mb == -1) {
		  
                                       
     
		ledger_set_limit(task->ledger, task_ledgers.phys_footprint,
		    max_task_footprint ? max_task_footprint : LEDGER_LIMIT_INFINITY,
		    max_task_footprint ? (uint8_t)max_task_footprint_warning_level : 0);

		task_lock(task);
		task_set_memlimit_is_active(task, memlimit_is_active);
		task_set_memlimit_is_fatal(task, memlimit_is_fatal);
		task_unlock(task);

		return KERN_SUCCESS;
	}

#ifdef CONFIG_NOMONITORS
	return KERN_SUCCESS;
#endif                        

	task_lock(task);

	if ((memlimit_is_active == task_get_memlimit_is_active(task)) &&
	    (memlimit_is_fatal == task_get_memlimit_is_fatal(task)) &&
	    (((ledger_amount_t)new_limit_mb << 20) == old)) {
		  
                                   
     
		task_unlock(task);
		return KERN_SUCCESS;
	}

	task_set_memlimit_is_active(task, memlimit_is_active);
	task_set_memlimit_is_fatal(task, memlimit_is_fatal);

	ledger_set_limit(task->ledger, task_ledgers.phys_footprint,
	    (ledger_amount_t)new_limit_mb << 20, PHYS_FOOTPRINT_WARNING_LEVEL);

	if (task == current_task()) {
		ledger_check_new_balance(current_thread(), task->ledger,
		    task_ledgers.phys_footprint);
	}

	task_unlock(task);

	return KERN_SUCCESS;
}

kern_return_t
task_get_phys_footprint_limit(
	task_t task,
	int *limit_mb)
{
	ledger_amount_t limit;
	kern_return_t ret;

	ret = ledger_get_limit(task->ledger, task_ledgers.phys_footprint, &limit);
	if (ret != KERN_SUCCESS) {
		return ret;
	}

	  
                                                                       
                                                                     
                                     
    
	assert(((limit & 0xFFF0000000000000LL) == 0) || (limit == LEDGER_LIMIT_INFINITY));
	*limit_mb = (int)(limit >> 20);

	return KERN_SUCCESS;
}
#else                          
kern_return_t
task_set_phys_footprint_limit(
	__unused task_t task,
	__unused int new_limit_mb,
	__unused int *old_limit_mb)
{
	return KERN_FAILURE;
}

kern_return_t
task_get_phys_footprint_limit(
	__unused task_t task,
	__unused int *limit_mb)
{
	return KERN_FAILURE;
}
#endif                          

void
task_set_thread_limit(task_t task, uint16_t thread_limit)
{
	assert(task != kernel_task);
	if (thread_limit <= TASK_MAX_THREAD_LIMIT) {
		task_lock(task);
		task->task_thread_limit = thread_limit;
		task_unlock(task);
	}
}

#if XNU_TARGET_OS_OSX
boolean_t
task_has_system_version_compat_enabled(task_t task)
{
	boolean_t enabled = FALSE;

	task_lock(task);
	enabled = (task->t_flags & TF_SYS_VERSION_COMPAT);
	task_unlock(task);

	return enabled;
}

void
task_set_system_version_compat_enabled(task_t task, boolean_t enable_system_version_compat)
{
	assert(task == current_task());
	assert(task != kernel_task);

	task_lock(task);
	if (enable_system_version_compat) {
		task->t_flags |= TF_SYS_VERSION_COMPAT;
	} else {
		task->t_flags &= ~TF_SYS_VERSION_COMPAT;
	}
	task_unlock(task);
}
#endif                        

  
                                                            
                                                       
                                                              
   
boolean_t
is_kerneltask(task_t t)
{
	if (t == kernel_task) {
		return TRUE;
	}

	return FALSE;
}

boolean_t
is_corpsetask(task_t t)
{
	return task_is_a_corpse(t);
}

#undef current_task
task_t current_task(void);
task_t
current_task(void)
{
	return current_task_fast();
}

#undef task_reference
void task_reference(task_t task);
void
task_reference(
	task_t          task)
{
	if (task != TASK_NULL) {
		task_reference_internal(task);
	}
}

                                     
extern int get_audit_token_pid(audit_token_t *audit_token);

int
task_pid(task_t task)
{
	if (task) {
		return get_audit_token_pid(&task->audit_token);
	}
	return -1;
}

#if __has_feature(ptrauth_calls)
  
                                                             
                                                        
                                                          
                                             
   
char *
task_get_vm_shared_region_id_and_jop_pid(task_t task, uint64_t *jop_pid)
{
	size_t len;
	char *shared_region_id = NULL;

	task_lock(task);
	if (task->shared_region_id == NULL) {
		task_unlock(task);
		return NULL;
	}
	len = strlen(task->shared_region_id) + 1;

	                                           
	task_unlock(task);
	shared_region_id = kheap_alloc(KHEAP_DATA_BUFFERS, len, Z_WAITOK);
	task_lock(task);

	if (task->shared_region_id == NULL) {
		task_unlock(task);
		kheap_free(KHEAP_DATA_BUFFERS, shared_region_id, len);
		return NULL;
	}
	assert(len == strlen(task->shared_region_id) + 1);                               
	strlcpy(shared_region_id, task->shared_region_id, len);
	task_unlock(task);

	                                  
	if (jop_pid != NULL) {
		*jop_pid = shared_region_find_key(shared_region_id);
	}

	return shared_region_id;
}

  
                                      
   
void
task_set_shared_region_id(task_t task, char *id)
{
	char *old_id;

	task_lock(task);
	old_id = task->shared_region_id;
	task->shared_region_id = id;
	task->shared_region_auth_remapped = FALSE;
	task_unlock(task);

	                                            
	if (old_id != NULL) {
		shared_region_key_dealloc(old_id);
		kheap_free(KHEAP_DATA_BUFFERS, old_id, strlen(old_id) + 1);
	}
}
#endif                                   

  
                                                         
                                                                         
  
                                                                          
                                                                                 
  
                                                    
   
thread_t
task_findtid(task_t task, uint64_t tid)
{
	thread_t self           = current_thread();
	thread_t found_thread   = THREAD_NULL;
	thread_t iter_thread    = THREAD_NULL;

	                                                            
	if (tid == self->thread_id || tid == TID_NULL) {
		assert(self->task == task);

		thread_reference(self);

		return self;
	}

	task_lock(task);

	queue_iterate(&task->threads, iter_thread, thread_t, task_threads) {
		if (iter_thread->thread_id == tid) {
			found_thread = iter_thread;
			thread_reference(found_thread);
			break;
		}
	}

	task_unlock(task);

	return found_thread;
}

int
pid_from_task(task_t task)
{
	int pid = -1;

	if (task->bsd_info) {
		pid = proc_pid(task->bsd_info);
	} else {
		pid = task_pid(task);
	}

	return pid;
}

  
                                            
   
kern_return_t
task_cpu_usage_monitor_ctl(task_t task, uint32_t *flags)
{
	int error = KERN_SUCCESS;

	if (*flags & CPUMON_MAKE_FATAL) {
		task->rusage_cpu_flags |= TASK_RUSECPU_FLAGS_FATAL_CPUMON;
	} else {
		error = KERN_INVALID_ARGUMENT;
	}

	return error;
}

  
                                          
   
kern_return_t
task_wakeups_monitor_ctl(task_t task, uint32_t *flags, int32_t *rate_hz)
{
	ledger_t ledger = task->ledger;

	task_lock(task);
	if (*flags & WAKEMON_GET_PARAMS) {
		ledger_amount_t limit;
		uint64_t                period;

		ledger_get_limit(ledger, task_ledgers.interrupt_wakeups, &limit);
		ledger_get_period(ledger, task_ledgers.interrupt_wakeups, &period);

		if (limit != LEDGER_LIMIT_INFINITY) {
			  
                                                           
      
			*rate_hz = (int32_t)(limit / (int64_t)(period / NSEC_PER_SEC));
			*flags = WAKEMON_ENABLE;
			if (task->rusage_cpu_flags & TASK_RUSECPU_FLAGS_FATAL_WAKEUPSMON) {
				*flags |= WAKEMON_MAKE_FATAL;
			}
		} else {
			*flags = WAKEMON_DISABLE;
			*rate_hz = -1;
		}

		  
                                                                            
     
		task_unlock(task);
		return KERN_SUCCESS;
	}

	if (*flags & WAKEMON_ENABLE) {
		if (*flags & WAKEMON_SET_DEFAULTS) {
			*rate_hz = task_wakeups_monitor_rate;
		}

#ifndef CONFIG_NOMONITORS
		if (*flags & WAKEMON_MAKE_FATAL) {
			task->rusage_cpu_flags |= TASK_RUSECPU_FLAGS_FATAL_WAKEUPSMON;
		}
#endif                        

		if (*rate_hz <= 0) {
			task_unlock(task);
			return KERN_INVALID_ARGUMENT;
		}

#ifndef CONFIG_NOMONITORS
		ledger_set_limit(ledger, task_ledgers.interrupt_wakeups, *rate_hz * task_wakeups_monitor_interval,
		    (uint8_t)task_wakeups_monitor_ustackshots_trigger_pct);
		ledger_set_period(ledger, task_ledgers.interrupt_wakeups, task_wakeups_monitor_interval * NSEC_PER_SEC);
		ledger_enable_callback(ledger, task_ledgers.interrupt_wakeups);
#endif                        
	} else if (*flags & WAKEMON_DISABLE) {
		  
                                                          
    
                                                                      
                                                             
     
#if CONFIG_TELEMETRY
		telemetry_task_ctl_locked(task, TF_WAKEMON_WARNING, 0);
#endif
		ledger_disable_refill(ledger, task_ledgers.interrupt_wakeups);
		ledger_disable_callback(ledger, task_ledgers.interrupt_wakeups);
	}

	task_unlock(task);
	return KERN_SUCCESS;
}

void
task_wakeups_rate_exceeded(int warning, __unused const void *param0, __unused const void *param1)
{
	if (warning == LEDGER_WARNING_ROSE_ABOVE) {
#if CONFIG_TELEMETRY
		  
                                                                                           
                                                                                   
     
		telemetry_task_ctl(current_task(), TF_WAKEMON_WARNING, 1);
#endif
		return;
	}

#if CONFIG_TELEMETRY
	  
                                                                                      
                                                        
    
	telemetry_task_ctl(current_task(), TF_WAKEMON_WARNING, 0);
#endif

	if (warning == 0) {
		SENDING_NOTIFICATION__THIS_PROCESS_IS_CAUSING_TOO_MANY_WAKEUPS();
	}
}

void __attribute__((noinline))
SENDING_NOTIFICATION__THIS_PROCESS_IS_CAUSING_TOO_MANY_WAKEUPS(void)
{
	task_t                      task        = current_task();
	int                         pid         = 0;
	const char                  *procname   = "unknown";
	boolean_t                   fatal;
	kern_return_t               kr;
#ifdef EXC_RESOURCE_MONITORS
	mach_exception_data_type_t  code[EXCEPTION_CODE_MAX];
#endif                            
	struct ledger_entry_info    lei;

#ifdef MACH_BSD
	pid = proc_selfpid();
	if (task->bsd_info != NULL) {
		procname = proc_name_address(current_task()->bsd_info);
	}
#endif

	ledger_get_entry_info(task->ledger, task_ledgers.interrupt_wakeups, &lei);

	  
                                                            
                                                                
                                                                           
    
	uint32_t flags = WAKEMON_DISABLE;
	task_wakeups_monitor_ctl(task, &flags, NULL);

	fatal = task->rusage_cpu_flags & TASK_RUSECPU_FLAGS_FATAL_WAKEUPSMON;
	trace_resource_violation(RMON_CPUWAKES_VIOLATED, &lei);
	os_log(OS_LOG_DEFAULT, "process %s[%d] caught waking the CPU %llu times "
	    "over ~%llu seconds, averaging %llu wakes / second and "
	    "violating a %slimit of %llu wakes over %llu seconds.\n",
	    procname, pid,
	    lei.lei_balance, lei.lei_last_refill / NSEC_PER_SEC,
	    lei.lei_last_refill == 0 ? 0 :
	    (NSEC_PER_SEC * lei.lei_balance / lei.lei_last_refill),
	    fatal ? "FATAL " : "",
	    lei.lei_limit, lei.lei_refill_period / NSEC_PER_SEC);

	kr = send_resource_violation(send_cpu_wakes_violation, task, &lei,
	    fatal ? kRNFatalLimitFlag : 0);
	if (kr) {
		printf("send_resource_violation(CPU wakes, ...): error %#x\n", kr);
	}

#ifdef EXC_RESOURCE_MONITORS
	if (disable_exc_resource) {
		printf("process %s[%d] caught causing excessive wakeups. EXC_RESOURCE "
		    "supressed by a boot-arg\n", procname, pid);
		return;
	}
	if (audio_active) {
		os_log(OS_LOG_DEFAULT, "process %s[%d] caught causing excessive wakeups. EXC_RESOURCE "
		    "supressed due to audio playback\n", procname, pid);
		return;
	}
	if (lei.lei_last_refill == 0) {
		os_log(OS_LOG_DEFAULT, "process %s[%d] caught causing excessive wakeups. EXC_RESOURCE "
		    "supressed due to lei.lei_last_refill = 0 \n", procname, pid);
	}

	code[0] = code[1] = 0;
	EXC_RESOURCE_ENCODE_TYPE(code[0], RESOURCE_TYPE_WAKEUPS);
	EXC_RESOURCE_ENCODE_FLAVOR(code[0], FLAVOR_WAKEUPS_MONITOR);
	EXC_RESOURCE_CPUMONITOR_ENCODE_WAKEUPS_PERMITTED(code[0],
	    NSEC_PER_SEC * lei.lei_limit / lei.lei_refill_period);
	EXC_RESOURCE_CPUMONITOR_ENCODE_OBSERVATION_INTERVAL(code[0],
	    lei.lei_last_refill);
	EXC_RESOURCE_CPUMONITOR_ENCODE_WAKEUPS_OBSERVED(code[1],
	    NSEC_PER_SEC * lei.lei_balance / lei.lei_last_refill);
	exception_triage(EXC_RESOURCE, code, EXCEPTION_CODE_MAX);
#endif                            

	if (fatal) {
		task_terminate_internal(task);
	}
}

static boolean_t
global_update_logical_writes(int64_t io_delta, int64_t *global_write_count)
{
	int64_t old_count, new_count;
	boolean_t needs_telemetry;

	do {
		new_count = old_count = *global_write_count;
		new_count += io_delta;
		if (new_count >= io_telemetry_limit) {
			new_count = 0;
			needs_telemetry = TRUE;
		} else {
			needs_telemetry = FALSE;
		}
	} while (!OSCompareAndSwap64(old_count, new_count, global_write_count));
	return needs_telemetry;
}

void
task_update_physical_writes(__unused task_t task, __unused task_physical_write_flavor_t flavor, __unused uint64_t io_size, __unused task_balance_flags_t flags)
{
#if CONFIG_PHYS_WRITE_ACCT
	if (!io_size) {
		return;
	}

	  
                                                                 
    
	if (!task) {
		task = kernel_task;
	}

	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_PHYS_WRITE_ACCT)) | DBG_FUNC_NONE,
	    task_pid(task), flavor, io_size, flags, 0);
	DTRACE_IO4(physical_writes, struct task *, task, task_physical_write_flavor_t, flavor, uint64_t, io_size, task_balance_flags_t, flags);

	if (flags & TASK_BALANCE_CREDIT) {
		if (flavor == TASK_PHYSICAL_WRITE_METADATA) {
			OSAddAtomic64(io_size, (SInt64 *)&(task->task_fs_metadata_writes));
			ledger_credit_nocheck(task->ledger, task_ledgers.fs_metadata_writes, io_size);
		}
	} else if (flags & TASK_BALANCE_DEBIT) {
		if (flavor == TASK_PHYSICAL_WRITE_METADATA) {
			OSAddAtomic64(-1 * io_size, (SInt64 *)&(task->task_fs_metadata_writes));
			ledger_debit_nocheck(task->ledger, task_ledgers.fs_metadata_writes, io_size);
		}
	}
#endif                             
}

void
task_update_logical_writes(task_t task, uint32_t io_size, int flags, void *vp)
{
	int64_t io_delta = 0;
	int64_t * global_counter_to_update;
	boolean_t needs_telemetry = FALSE;
	boolean_t is_external_device = FALSE;
	int ledger_to_update = 0;
	struct task_writes_counters * writes_counters_to_update;

	if ((!task) || (!io_size) || (!vp)) {
		return;
	}

	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_DATA_WRITE)) | DBG_FUNC_NONE,
	    task_pid(task), io_size, flags, (uintptr_t)VM_KERNEL_ADDRPERM(vp), 0);
	DTRACE_IO4(logical_writes, struct task *, task, uint32_t, io_size, int, flags, vnode *, vp);

	                                                                      
	if (vnode_isonexternalstorage(vp) == false) {
		global_counter_to_update = &global_logical_writes_count;
		ledger_to_update = task_ledgers.logical_writes;
		writes_counters_to_update = &task->task_writes_counters_internal;
		is_external_device = FALSE;
	} else {
		global_counter_to_update = &global_logical_writes_to_external_count;
		ledger_to_update = task_ledgers.logical_writes_to_external;
		writes_counters_to_update = &task->task_writes_counters_external;
		is_external_device = TRUE;
	}

	switch (flags) {
	case TASK_WRITE_IMMEDIATE:
		OSAddAtomic64(io_size, (SInt64 *)&(writes_counters_to_update->task_immediate_writes));
		ledger_credit(task->ledger, ledger_to_update, io_size);
		if (!is_external_device) {
			coalition_io_ledger_update(task, FLAVOR_IO_LOGICAL_WRITES, TRUE, io_size);
		}
		break;
	case TASK_WRITE_DEFERRED:
		OSAddAtomic64(io_size, (SInt64 *)&(writes_counters_to_update->task_deferred_writes));
		ledger_credit(task->ledger, ledger_to_update, io_size);
		if (!is_external_device) {
			coalition_io_ledger_update(task, FLAVOR_IO_LOGICAL_WRITES, TRUE, io_size);
		}
		break;
	case TASK_WRITE_INVALIDATED:
		OSAddAtomic64(io_size, (SInt64 *)&(writes_counters_to_update->task_invalidated_writes));
		ledger_debit(task->ledger, ledger_to_update, io_size);
		if (!is_external_device) {
			coalition_io_ledger_update(task, FLAVOR_IO_LOGICAL_WRITES, FALSE, io_size);
		}
		break;
	case TASK_WRITE_METADATA:
		OSAddAtomic64(io_size, (SInt64 *)&(writes_counters_to_update->task_metadata_writes));
		ledger_credit(task->ledger, ledger_to_update, io_size);
		if (!is_external_device) {
			coalition_io_ledger_update(task, FLAVOR_IO_LOGICAL_WRITES, TRUE, io_size);
		}
		break;
	}

	io_delta = (flags == TASK_WRITE_INVALIDATED) ? ((int64_t)io_size * -1ll) : ((int64_t)io_size);
	if (io_telemetry_limit != 0) {
		                                                                          
		needs_telemetry = global_update_logical_writes(io_delta, global_counter_to_update);
		if (needs_telemetry && !is_external_device) {
			act_set_io_telemetry_ast(current_thread());
		}
	}
}

  
                                      
   
kern_return_t
task_io_monitor_ctl(task_t task, uint32_t *flags)
{
	ledger_t ledger = task->ledger;

	task_lock(task);
	if (*flags & IOMON_ENABLE) {
		                                       
		ledger_set_limit(ledger, task_ledgers.physical_writes, (task_iomon_limit_mb * 1024 * 1024), 0);
		ledger_set_period(ledger, task_ledgers.physical_writes, (task_iomon_interval_secs * NSEC_PER_SEC));
	} else if (*flags & IOMON_DISABLE) {
		  
                                                      
     
		ledger_disable_refill(ledger, task_ledgers.physical_writes);
		ledger_disable_callback(ledger, task_ledgers.physical_writes);
	}

	task_unlock(task);
	return KERN_SUCCESS;
}

void
task_io_rate_exceeded(int warning, const void *param0, __unused const void *param1)
{
	if (warning == 0) {
		SENDING_NOTIFICATION__THIS_PROCESS_IS_CAUSING_TOO_MUCH_IO((int)param0);
	}
}

void __attribute__((noinline))
SENDING_NOTIFICATION__THIS_PROCESS_IS_CAUSING_TOO_MUCH_IO(int flavor)
{
	int                             pid = 0;
	task_t                          task = current_task();
#ifdef EXC_RESOURCE_MONITORS
	mach_exception_data_type_t      code[EXCEPTION_CODE_MAX];
#endif                            
	struct ledger_entry_info        lei;
	kern_return_t                   kr;

#ifdef MACH_BSD
	pid = proc_selfpid();
#endif
	  
                                                                                
                                         
    
	switch (flavor) {
	case FLAVOR_IO_PHYSICAL_WRITES:
		ledger_get_entry_info(task->ledger, task_ledgers.physical_writes, &lei);
		break;
	}


	  
                                                            
                                                                
                                                                           
    
	uint32_t flags = IOMON_DISABLE;
	task_io_monitor_ctl(task, &flags);

	if (flavor == FLAVOR_IO_LOGICAL_WRITES) {
		trace_resource_violation(RMON_LOGWRITES_VIOLATED, &lei);
	}
	os_log(OS_LOG_DEFAULT, "process [%d] caught causing excessive I/O (flavor: %d). Task I/O: %lld MB. [Limit : %lld MB per %lld secs]\n",
	    pid, flavor, (lei.lei_balance / (1024 * 1024)), (lei.lei_limit / (1024 * 1024)), (lei.lei_refill_period / NSEC_PER_SEC));

	kr = send_resource_violation(send_disk_writes_violation, task, &lei, kRNFlagsNone);
	if (kr) {
		printf("send_resource_violation(disk_writes, ...): error %#x\n", kr);
	}

#ifdef EXC_RESOURCE_MONITORS
	code[0] = code[1] = 0;
	EXC_RESOURCE_ENCODE_TYPE(code[0], RESOURCE_TYPE_IO);
	EXC_RESOURCE_ENCODE_FLAVOR(code[0], flavor);
	EXC_RESOURCE_IO_ENCODE_INTERVAL(code[0], (lei.lei_refill_period / NSEC_PER_SEC));
	EXC_RESOURCE_IO_ENCODE_LIMIT(code[0], (lei.lei_limit / (1024 * 1024)));
	EXC_RESOURCE_IO_ENCODE_OBSERVED(code[1], (lei.lei_balance / (1024 * 1024)));
	exception_triage(EXC_RESOURCE, code, EXCEPTION_CODE_MAX);
#endif                            
}

                                                          
kern_return_t
task_get_mach_voucher(
	task_t                  task,
	mach_voucher_selector_t __unused which,
	ipc_voucher_t           *voucher)
{
	if (TASK_NULL == task) {
		return KERN_INVALID_TASK;
	}

	*voucher = NULL;
	return KERN_SUCCESS;
}

kern_return_t
task_set_mach_voucher(
	task_t                  task,
	ipc_voucher_t           __unused voucher)
{
	if (TASK_NULL == task) {
		return KERN_INVALID_TASK;
	}

	return KERN_SUCCESS;
}

kern_return_t
task_swap_mach_voucher(
	__unused task_t         task,
	__unused ipc_voucher_t  new_voucher,
	ipc_voucher_t          *in_out_old_voucher)
{
	  
                                                               
                                                              
                                                                     
                                             
    
	ipc_voucher_release(*in_out_old_voucher);
	return KERN_NOT_SUPPORTED;
}

void
task_set_gpu_denied(task_t task, boolean_t denied)
{
	task_lock(task);

	if (denied) {
		task->t_flags |= TF_GPU_DENIED;
	} else {
		task->t_flags &= ~TF_GPU_DENIED;
	}

	task_unlock(task);
}

boolean_t
task_is_gpu_denied(task_t task)
{
	                                              
	return (task->t_flags & TF_GPU_DENIED) ? TRUE : FALSE;
}


uint64_t
get_task_memory_region_count(task_t task)
{
	vm_map_t map;
	map = (task == kernel_task) ? kernel_map: task->map;
	return (uint64_t)get_map_nentries(map);
}

static void
kdebug_trace_dyld_internal(uint32_t base_code,
    struct dyld_kernel_image_info *info)
{
	static_assert(sizeof(info->uuid) >= 16);

#if defined(__LP64__)
	uint64_t *uuid = (uint64_t *)&(info->uuid);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    KDBG_EVENTID(DBG_DYLD, DBG_DYLD_UUID, base_code), uuid[0],
	    uuid[1], info->load_addr,
	    (uint64_t)info->fsid.val[0] | ((uint64_t)info->fsid.val[1] << 32),
	    0);
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    KDBG_EVENTID(DBG_DYLD, DBG_DYLD_UUID, base_code + 1),
	    (uint64_t)info->fsobjid.fid_objno |
	    ((uint64_t)info->fsobjid.fid_generation << 32),
	    0, 0, 0, 0);
#else                        
	uint32_t *uuid = (uint32_t *)&(info->uuid);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    KDBG_EVENTID(DBG_DYLD, DBG_DYLD_UUID, base_code + 2), uuid[0],
	    uuid[1], uuid[2], uuid[3], 0);
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    KDBG_EVENTID(DBG_DYLD, DBG_DYLD_UUID, base_code + 3),
	    (uint32_t)info->load_addr, info->fsid.val[0], info->fsid.val[1],
	    info->fsobjid.fid_objno, 0);
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    KDBG_EVENTID(DBG_DYLD, DBG_DYLD_UUID, base_code + 4),
	    info->fsobjid.fid_generation, 0, 0, 0, 0);
#endif                         
}

static kern_return_t
kdebug_trace_dyld(task_t task, uint32_t base_code,
    vm_map_copy_t infos_copy, mach_msg_type_number_t infos_len)
{
	kern_return_t kr;
	dyld_kernel_image_info_array_t infos;
	vm_map_offset_t map_data;
	vm_offset_t data;

	if (!infos_copy) {
		return KERN_INVALID_ADDRESS;
	}

	if (!kdebug_enable ||
	    !kdebug_debugid_enabled(KDBG_EVENTID(DBG_DYLD, DBG_DYLD_UUID, 0))) {
		vm_map_copy_discard(infos_copy);
		return KERN_SUCCESS;
	}

	if (task == NULL || task != current_task()) {
		return KERN_INVALID_TASK;
	}

	kr = vm_map_copyout(ipc_kernel_map, &map_data, (vm_map_copy_t)infos_copy);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	infos = CAST_DOWN(dyld_kernel_image_info_array_t, map_data);

	for (mach_msg_type_number_t i = 0; i < infos_len; i++) {
		kdebug_trace_dyld_internal(base_code, &(infos[i]));
	}

	data = CAST_DOWN(vm_offset_t, map_data);
	mach_vm_deallocate(ipc_kernel_map, data, infos_len * sizeof(infos[0]));
	return KERN_SUCCESS;
}

kern_return_t
task_register_dyld_image_infos(task_t task,
    dyld_kernel_image_info_array_t infos_copy,
    mach_msg_type_number_t infos_len)
{
	return kdebug_trace_dyld(task, DBG_DYLD_UUID_MAP_A,
	           (vm_map_copy_t)infos_copy, infos_len);
}

kern_return_t
task_unregister_dyld_image_infos(task_t task,
    dyld_kernel_image_info_array_t infos_copy,
    mach_msg_type_number_t infos_len)
{
	return kdebug_trace_dyld(task, DBG_DYLD_UUID_UNMAP_A,
	           (vm_map_copy_t)infos_copy, infos_len);
}

kern_return_t
task_get_dyld_image_infos(__unused task_t task,
    __unused dyld_kernel_image_info_array_t * dyld_images,
    __unused mach_msg_type_number_t * dyld_imagesCnt)
{
	return KERN_NOT_SUPPORTED;
}

kern_return_t
task_register_dyld_shared_cache_image_info(task_t task,
    dyld_kernel_image_info_t cache_img,
    __unused boolean_t no_cache,
    __unused boolean_t private_cache)
{
	if (task == NULL || task != current_task()) {
		return KERN_INVALID_TASK;
	}

	kdebug_trace_dyld_internal(DBG_DYLD_UUID_SHARED_CACHE_A, &cache_img);
	return KERN_SUCCESS;
}

kern_return_t
task_register_dyld_set_dyld_state(__unused task_t task,
    __unused uint8_t dyld_state)
{
	return KERN_NOT_SUPPORTED;
}

kern_return_t
task_register_dyld_get_process_state(__unused task_t task,
    __unused dyld_kernel_process_info_t * dyld_process_state)
{
	return KERN_NOT_SUPPORTED;
}

kern_return_t
task_inspect(task_inspect_t task_insp, task_inspect_flavor_t flavor,
    task_inspect_info_t info_out, mach_msg_type_number_t *size_in_out)
{
#if MONOTONIC
	task_t task = (task_t)task_insp;
	kern_return_t kr = KERN_SUCCESS;
	mach_msg_type_number_t size;

	if (task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	size = *size_in_out;

	switch (flavor) {
	case TASK_INSPECT_BASIC_COUNTS: {
		struct task_inspect_basic_counts *bc;
		uint64_t task_counts[MT_CORE_NFIXED] = { 0 };

		if (size < TASK_INSPECT_BASIC_COUNTS_COUNT) {
			kr = KERN_INVALID_ARGUMENT;
			break;
		}

		mt_fixed_task_counts(task, task_counts);
		bc = (struct task_inspect_basic_counts *)info_out;
#ifdef MT_CORE_INSTRS
		bc->instructions = task_counts[MT_CORE_INSTRS];
#else                              
		bc->instructions = 0;
#endif                               
		bc->cycles = task_counts[MT_CORE_CYCLES];
		size = TASK_INSPECT_BASIC_COUNTS_COUNT;
		break;
	}
	default:
		kr = KERN_INVALID_ARGUMENT;
		break;
	}

	if (kr == KERN_SUCCESS) {
		*size_in_out = size;
	}
	return kr;
#else                
#pragma unused(task_insp, flavor, info_out, size_in_out)
	return KERN_NOT_SUPPORTED;
#endif                 
}

#if CONFIG_SECLUDED_MEMORY
int num_tasks_can_use_secluded_mem = 0;

void
task_set_can_use_secluded_mem(
	task_t          task,
	boolean_t       can_use_secluded_mem)
{
	if (!task->task_could_use_secluded_mem) {
		return;
	}
	task_lock(task);
	task_set_can_use_secluded_mem_locked(task, can_use_secluded_mem);
	task_unlock(task);
}

void
task_set_can_use_secluded_mem_locked(
	task_t          task,
	boolean_t       can_use_secluded_mem)
{
	assert(task->task_could_use_secluded_mem);
	if (can_use_secluded_mem &&
	    secluded_for_apps &&                      
	    !task->task_can_use_secluded_mem) {
		assert(num_tasks_can_use_secluded_mem >= 0);
		OSAddAtomic(+1,
		    (volatile SInt32 *)&num_tasks_can_use_secluded_mem);
		task->task_can_use_secluded_mem = TRUE;
	} else if (!can_use_secluded_mem &&
	    task->task_can_use_secluded_mem) {
		assert(num_tasks_can_use_secluded_mem > 0);
		OSAddAtomic(-1,
		    (volatile SInt32 *)&num_tasks_can_use_secluded_mem);
		task->task_can_use_secluded_mem = FALSE;
	}
}

void
task_set_could_use_secluded_mem(
	task_t          task,
	boolean_t       could_use_secluded_mem)
{
	task->task_could_use_secluded_mem = !!could_use_secluded_mem;
}

void
task_set_could_also_use_secluded_mem(
	task_t          task,
	boolean_t       could_also_use_secluded_mem)
{
	task->task_could_also_use_secluded_mem = !!could_also_use_secluded_mem;
}

boolean_t
task_can_use_secluded_mem(
	task_t          task,
	boolean_t       is_alloc)
{
	if (task->task_can_use_secluded_mem) {
		assert(task->task_could_use_secluded_mem);
		assert(num_tasks_can_use_secluded_mem > 0);
		return TRUE;
	}
	if (task->task_could_also_use_secluded_mem &&
	    num_tasks_can_use_secluded_mem > 0) {
		assert(num_tasks_can_use_secluded_mem > 0);
		return TRUE;
	}

	  
                                                            
                                                             
                                                           
                                                                    
    
	if (is_alloc && secluded_shutoff_trigger != 0) {
		uint64_t phys_used = get_task_phys_footprint(task);
		uint64_t limit = get_task_phys_footprint_limit(task);
		if (phys_used > secluded_shutoff_trigger &&
		    limit > secluded_shutoff_trigger &&
		    phys_used > limit - secluded_shutoff_headroom) {
			start_secluded_suppression(task);
			return TRUE;
		}
	}

	return FALSE;
}

boolean_t
task_could_use_secluded_mem(
	task_t  task)
{
	return task->task_could_use_secluded_mem;
}

boolean_t
task_could_also_use_secluded_mem(
	task_t  task)
{
	return task->task_could_also_use_secluded_mem;
}
#endif                             

queue_head_t *
task_io_user_clients(task_t task)
{
	return &task->io_user_clients;
}

void
task_set_message_app_suspended(task_t task, boolean_t enable)
{
	task->message_app_suspended = enable;
}

void
task_copy_fields_for_exec(task_t dst_task, task_t src_task)
{
	dst_task->vtimers = src_task->vtimers;
}

#if DEVELOPMENT || DEBUG
int vm_region_footprint = 0;
#endif                           

boolean_t
task_self_region_footprint(void)
{
#if DEVELOPMENT || DEBUG
	if (vm_region_footprint) {
		                          
		return TRUE;
	}
#endif                           
	return current_task()->task_region_footprint;
}

void
task_self_region_footprint_set(
	boolean_t newval)
{
	task_t  curtask;

	curtask = current_task();
	task_lock(curtask);
	if (newval) {
		curtask->task_region_footprint = TRUE;
	} else {
		curtask->task_region_footprint = FALSE;
	}
	task_unlock(curtask);
}

void
task_set_darkwake_mode(task_t task, boolean_t set_mode)
{
	assert(task);

	task_lock(task);

	if (set_mode) {
		task->t_flags |= TF_DARKWAKE_MODE;
	} else {
		task->t_flags &= ~(TF_DARKWAKE_MODE);
	}

	task_unlock(task);
}

boolean_t
task_get_darkwake_mode(task_t task)
{
	assert(task);
	return (task->t_flags & TF_DARKWAKE_MODE) != 0;
}

kern_return_t
task_get_exc_guard_behavior(
	task_t task,
	task_exc_guard_behavior_t *behaviorp)
{
	if (task == TASK_NULL) {
		return KERN_INVALID_TASK;
	}
	*behaviorp = task->task_exc_guard;
	return KERN_SUCCESS;
}

#ifndef TASK_EXC_GUARD_ALL
                                                    
#define TASK_EXC_GUARD_ALL (TASK_EXC_GUARD_VM_ALL | 0xf0)
#endif

kern_return_t
task_set_exc_guard_behavior(
	task_t task,
	task_exc_guard_behavior_t behavior)
{
	if (task == TASK_NULL) {
		return KERN_INVALID_TASK;
	}
	if (behavior & ~TASK_EXC_GUARD_ALL) {
		return KERN_INVALID_VALUE;
	}
	task->task_exc_guard = behavior;
	return KERN_SUCCESS;
}

#if __arm64__
extern int legacy_footprint_entitlement_mode;
extern void memorystatus_act_on_legacy_footprint_entitlement(struct proc *, boolean_t);
extern void memorystatus_act_on_ios13extended_footprint_entitlement(struct proc *);


void
task_set_legacy_footprint(
	task_t task)
{
	task_lock(task);
	task->task_legacy_footprint = TRUE;
	task_unlock(task);
}

void
task_set_extra_footprint_limit(
	task_t task)
{
	if (task->task_extra_footprint_limit) {
		return;
	}
	task_lock(task);
	if (task->task_extra_footprint_limit) {
		task_unlock(task);
		return;
	}
	task->task_extra_footprint_limit = TRUE;
	task_unlock(task);
	memorystatus_act_on_legacy_footprint_entitlement(task->bsd_info, TRUE);
}

void
task_set_ios13extended_footprint_limit(
	task_t task)
{
	if (task->task_ios13extended_footprint_limit) {
		return;
	}
	task_lock(task);
	if (task->task_ios13extended_footprint_limit) {
		task_unlock(task);
		return;
	}
	task->task_ios13extended_footprint_limit = TRUE;
	task_unlock(task);
	memorystatus_act_on_ios13extended_footprint_entitlement(task->bsd_info);
}
#endif                

static inline ledger_amount_t
task_ledger_get_balance(
	ledger_t        ledger,
	int             ledger_idx)
{
	ledger_amount_t amount;
	amount = 0;
	ledger_get_balance(ledger, ledger_idx, &amount);
	return amount;
}

  
                                                                   
                                      
   
void
task_ledgers_footprint(
	ledger_t        ledger,
	ledger_amount_t *ledger_resident,
	ledger_amount_t *ledger_compressed)
{
	*ledger_resident = 0;
	*ledger_compressed = 0;

	                                   
	*ledger_resident += task_ledger_get_balance(ledger, task_ledgers.purgeable_nonvolatile);
	*ledger_compressed += task_ledger_get_balance(ledger, task_ledgers.purgeable_nonvolatile_compressed);

	                             
	*ledger_resident += task_ledger_get_balance(ledger, task_ledgers.tagged_footprint);
	*ledger_compressed += task_ledger_get_balance(ledger, task_ledgers.tagged_footprint_compressed);

	                                                          

	                           
	*ledger_resident += task_ledger_get_balance(ledger, task_ledgers.media_footprint);
	*ledger_compressed += task_ledger_get_balance(ledger, task_ledgers.media_footprint_compressed);

	                              
	*ledger_resident += task_ledger_get_balance(ledger, task_ledgers.graphics_footprint);
	*ledger_compressed += task_ledger_get_balance(ledger, task_ledgers.graphics_footprint_compressed);

	                            
	*ledger_resident += task_ledger_get_balance(ledger, task_ledgers.neural_footprint);
	*ledger_compressed += task_ledger_get_balance(ledger, task_ledgers.neural_footprint_compressed);
}

void
task_set_memory_ownership_transfer(
	task_t    task,
	boolean_t value)
{
	task_lock(task);
	task->task_can_transfer_memory_ownership = !!value;
	task_unlock(task);
}

void
task_copy_vmobjects(task_t task, vm_object_query_t query, size_t len, size_t *num)
{
	vm_object_t find_vmo;
	size_t size = 0;

	task_objq_lock(task);
	if (query != NULL) {
		queue_iterate(&task->task_objq, find_vmo, vm_object_t, task_objq)
		{
			vm_object_query_t p = &query[size++];

			                              
			if (size * sizeof(vm_object_query_data_t) > len) {
				--size;
				break;
			}

			bzero(p, sizeof(*p));
			p->object_id = (vm_object_id_t) VM_KERNEL_ADDRPERM(find_vmo);
			p->virtual_size = find_vmo->internal ? find_vmo->vo_size : 0;
			p->resident_size = find_vmo->resident_page_count * PAGE_SIZE;
			p->wired_size = find_vmo->wired_page_count * PAGE_SIZE;
			p->reusable_size = find_vmo->reusable_page_count * PAGE_SIZE;
			p->vo_no_footprint = find_vmo->vo_no_footprint;
			p->vo_ledger_tag = find_vmo->vo_ledger_tag;
			p->purgable = find_vmo->purgable;

			if (find_vmo->internal && find_vmo->pager_created && find_vmo->pager != NULL) {
				p->compressed_size = vm_compressor_pager_get_count(find_vmo->pager) * PAGE_SIZE;
			} else {
				p->compressed_size = 0;
			}
		}
	} else {
		size = (size_t)task->task_owned_objects;
	}
	task_objq_unlock(task);

	*num = size;
}

void
task_set_filter_msg_flag(
	task_t task,
	boolean_t flag)
{
	assert(task != TASK_NULL);

	task_lock(task);
	if (flag) {
		task->t_flags |= TF_FILTER_MSG;
	} else {
		task->t_flags &= ~TF_FILTER_MSG;
	}
	task_unlock(task);
}

boolean_t
task_get_filter_msg_flag(
	task_t task)
{
	uint32_t flags = 0;

	if (!task) {
		return false;
	}

	flags = os_atomic_load(&task->t_flags, relaxed);
	return (flags & TF_FILTER_MSG) ? TRUE : FALSE;
}
bool
task_is_exotic(
	task_t task)
{
	if (task == TASK_NULL) {
		return false;
	}
	return vm_map_is_exotic(get_task_map(task));
}

bool
task_is_alien(
	task_t task)
{
	if (task == TASK_NULL) {
		return false;
	}
	return vm_map_is_alien(get_task_map(task));
}



#if CONFIG_MACF
                                         
void
mac_task_set_mach_filter_mask(task_t task, uint8_t *maskptr)
{
	assert(task);

	task->mach_trap_filter_mask = maskptr;
}

                                           
void
mac_task_set_kobj_filter_mask(task_t task, uint8_t *maskptr)
{
	assert(task);

	task->mach_kobj_filter_mask = maskptr;
}

                                                     
mac_task_mach_filter_cbfunc_t mac_task_mach_trap_evaluate = NULL;

                                                     
mac_task_kobj_filter_cbfunc_t mac_task_kobj_msg_evaluate = NULL;

                                                      
int
mac_task_register_filter_callbacks(
	const mac_task_mach_filter_cbfunc_t mach_cbfunc,
	const mac_task_kobj_filter_cbfunc_t kobj_cbfunc)
{
	if (mach_cbfunc != NULL) {
		if (mac_task_mach_trap_evaluate != NULL) {
			return KERN_FAILURE;
		}
		mac_task_mach_trap_evaluate = mach_cbfunc;
	}
	if (kobj_cbfunc != NULL) {
		if (mac_task_kobj_msg_evaluate != NULL) {
			return KERN_FAILURE;
		}
		mac_task_kobj_msg_evaluate = kobj_cbfunc;
	}

	return KERN_SUCCESS;
}
#endif                  

void
task_transfer_mach_filter_bits(
	task_t new_task,
	task_t old_task)
{
#ifdef CONFIG_MACF
	                                                                 
	new_task->mach_trap_filter_mask = old_task->mach_trap_filter_mask;
	new_task->mach_kobj_filter_mask = old_task->mach_kobj_filter_mask;
#endif
	                                                                
	if (task_get_filter_msg_flag(old_task)) {
		new_task->t_flags |= TF_FILTER_MSG;
	}
}


#if __has_feature(ptrauth_calls)

#define PAC_EXCEPTION_ENTITLEMENT "com.apple.private.pac.exception"

void
task_set_pac_exception_fatal_flag(
	task_t task)
{
	assert(task != TASK_NULL);

	if (!IOTaskHasEntitlement(task, PAC_EXCEPTION_ENTITLEMENT)) {
		return;
	}

	task_lock(task);
	task->t_flags |= TF_PAC_EXC_FATAL;
	task_unlock(task);
}

bool
task_is_pac_exception_fatal(
	task_t task)
{
	uint32_t flags = 0;

	assert(task != TASK_NULL);

	flags = os_atomic_load(&task->t_flags, relaxed);
	return (bool)(flags & TF_PAC_EXC_FATAL);
}
#endif                                   

void
task_set_tecs(task_t task)
{
	if (task == TASK_NULL) {
		task = current_task();
	}

	if (!machine_csv(CPUVN_CI)) {
		return;
	}

	LCK_MTX_ASSERT(&task->lock, LCK_MTX_ASSERT_NOTOWNED);

	task_lock(task);

	task->t_flags |= TF_TECS;

	thread_t thread;
	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		machine_tecs(thread);
	}
	task_unlock(task);
}

#if defined(__x86_64__)
bool
curtask_get_insn_copy_optout(void)
{
	bool optout;
	task_t cur_task = current_task();

	task_lock(cur_task);
	optout = (cur_task->t_flags & TF_INSN_COPY_OPTOUT) ? true : false;
	task_unlock(cur_task);

	return optout;
}

void
curtask_set_insn_copy_optout(void)
{
	task_t cur_task = current_task();

	task_lock(cur_task);

	cur_task->t_flags |= TF_INSN_COPY_OPTOUT;

	thread_t thread;
	queue_iterate(&cur_task->threads, thread, thread_t, task_threads) {
		machine_thread_set_insn_copy_optout(thread);
	}
	task_unlock(cur_task);
}
#endif                          
