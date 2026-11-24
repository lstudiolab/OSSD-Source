/*
 * Copyright (c) 2012-2020 Apple Inc. All rights reserved.
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
#include <mach/host_priv.h>
#include <mach/host_special_ports.h>
#include <mach/mach_types.h>
#include <mach/telemetry_notification_server.h>

#include <kern/assert.h>
#include <kern/clock.h>
#include <kern/debug.h>
#include <kern/host.h>
#include <kern/kalloc.h>
#include <kern/kern_types.h>
#include <kern/locks.h>
#include <kern/misc_protos.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/telemetry.h>
#include <kern/timer_call.h>
#include <kern/policy_internal.h>
#include <kern/kcdata.h>

#include <pexpert/pexpert.h>

#include <vm/vm_kern.h>
#include <vm/vm_shared_region.h>

#include <kperf/callstack.h>
#include <kern/backtrace.h>
#include <kern/monotonic.h>

#include <sys/kdebug.h>
#include <uuid/uuid.h>
#include <kdp/kdp_dyld.h>

#define TELEMETRY_DEBUG 0

struct proc;
extern int      proc_pid(struct proc *);
extern char     *proc_name_address(void *p);
extern uint64_t proc_uniqueid(void *p);
extern uint64_t proc_was_throttled(void *p);
extern uint64_t proc_did_throttle(void *p);
extern int      proc_selfpid(void);
extern boolean_t task_did_exec(task_t task);
extern boolean_t task_is_exec_copy(task_t task);

struct micro_snapshot_buffer {
	vm_offset_t             buffer;
	uint32_t                size;
	uint32_t                current_position;
	uint32_t                end_point;
};

void telemetry_take_sample(thread_t thread, uint8_t microsnapshot_flags, struct micro_snapshot_buffer * current_buffer);
int telemetry_buffer_gather(user_addr_t buffer, uint32_t *length, boolean_t mark, struct micro_snapshot_buffer * current_buffer);

#define TELEMETRY_DEFAULT_SAMPLE_RATE (1)                              
#define TELEMETRY_DEFAULT_BUFFER_SIZE (16*1024)
#define TELEMETRY_MAX_BUFFER_SIZE (64*1024)

#define TELEMETRY_DEFAULT_NOTIFY_LEEWAY (4*1024)                                                                 
#define TELEMETRY_MAX_UUID_COUNT (128)                                                              

uint32_t                        telemetry_sample_rate = 0;
volatile boolean_t      telemetry_needs_record = FALSE;
volatile boolean_t      telemetry_needs_timer_arming_record = FALSE;

  
                                                         
                                                              
   
boolean_t telemetry_sample_all_tasks = FALSE;
boolean_t telemetry_sample_pmis = FALSE;
uint32_t telemetry_active_tasks = 0;                                        

uint32_t telemetry_timestamp = 0;

  
                                      
                                                             
                                                         
                                                        
   
struct micro_snapshot_buffer telemetry_buffer = {
	.buffer = 0,
	.size = 0,
	.current_position = 0,
	.end_point = 0
};

int                                     telemetry_bytes_since_last_mark = -1;                                            
int                                     telemetry_buffer_notify_at = 0;

LCK_GRP_DECLARE(telemetry_lck_grp, "telemetry group");
LCK_MTX_DECLARE(telemetry_mtx, &telemetry_lck_grp);
LCK_MTX_DECLARE(telemetry_pmi_mtx, &telemetry_lck_grp);

#define TELEMETRY_LOCK() do { lck_mtx_lock(&telemetry_mtx); } while (0)
#define TELEMETRY_TRY_SPIN_LOCK() lck_mtx_try_lock_spin(&telemetry_mtx)
#define TELEMETRY_UNLOCK() do { lck_mtx_unlock(&telemetry_mtx); } while (0)

#define TELEMETRY_PMI_LOCK() do { lck_mtx_lock(&telemetry_pmi_mtx); } while (0)
#define TELEMETRY_PMI_UNLOCK() do { lck_mtx_unlock(&telemetry_pmi_mtx); } while (0)

void
telemetry_init(void)
{
	kern_return_t ret;
	uint32_t          telemetry_notification_leeway;

	if (!PE_parse_boot_argn("telemetry_buffer_size",
	    &telemetry_buffer.size, sizeof(telemetry_buffer.size))) {
		telemetry_buffer.size = TELEMETRY_DEFAULT_BUFFER_SIZE;
	}

	if (telemetry_buffer.size > TELEMETRY_MAX_BUFFER_SIZE) {
		telemetry_buffer.size = TELEMETRY_MAX_BUFFER_SIZE;
	}

	ret = kmem_alloc(kernel_map, &telemetry_buffer.buffer, telemetry_buffer.size, VM_KERN_MEMORY_DIAG);
	if (ret != KERN_SUCCESS) {
		kprintf("Telemetry: Allocation failed: %d\n", ret);
		return;
	}
	bzero((void *) telemetry_buffer.buffer, telemetry_buffer.size);

	if (!PE_parse_boot_argn("telemetry_notification_leeway",
	    &telemetry_notification_leeway, sizeof(telemetry_notification_leeway))) {
		  
                                                                                                        
     
		telemetry_notification_leeway = TELEMETRY_DEFAULT_NOTIFY_LEEWAY;
	}
	if (telemetry_notification_leeway >= telemetry_buffer.size) {
		printf("telemetry: nonsensical telemetry_notification_leeway boot-arg %d changed to %d\n",
		    telemetry_notification_leeway, TELEMETRY_DEFAULT_NOTIFY_LEEWAY);
		telemetry_notification_leeway = TELEMETRY_DEFAULT_NOTIFY_LEEWAY;
	}
	telemetry_buffer_notify_at = telemetry_buffer.size - telemetry_notification_leeway;

	if (!PE_parse_boot_argn("telemetry_sample_rate",
	    &telemetry_sample_rate, sizeof(telemetry_sample_rate))) {
		telemetry_sample_rate = TELEMETRY_DEFAULT_SAMPLE_RATE;
	}

	  
                                                                                           
    
	if (!PE_parse_boot_argn("telemetry_sample_all_tasks",
	    &telemetry_sample_all_tasks, sizeof(telemetry_sample_all_tasks))) {
#if !defined(XNU_TARGET_OS_OSX) && !(DEVELOPMENT || DEBUG)
		telemetry_sample_all_tasks = FALSE;
#else
		telemetry_sample_all_tasks = TRUE;
#endif                                                             
	}

	kprintf("Telemetry: Sampling %stasks once per %u second%s\n",
	    (telemetry_sample_all_tasks) ? "all " : "",
	    telemetry_sample_rate, telemetry_sample_rate == 1 ? "" : "s");
}

  
                                                                            
  
                                  
                                   
   
void
telemetry_global_ctl(int enable_disable)
{
	if (enable_disable == 1) {
		telemetry_sample_all_tasks = TRUE;
	} else {
		telemetry_sample_all_tasks = FALSE;
	}
}

  
                                                          
  
                                                     
                        
                         
  
                                  
                                   
   
void
telemetry_task_ctl(task_t task, uint32_t reasons, int enable_disable)
{
	task_lock(task);
	telemetry_task_ctl_locked(task, reasons, enable_disable);
	task_unlock(task);
}

void
telemetry_task_ctl_locked(task_t task, uint32_t reasons, int enable_disable)
{
	uint32_t origflags;

	assert((reasons != 0) && ((reasons | TF_TELEMETRY) == TF_TELEMETRY));

	task_lock_assert_owned(task);

	origflags = task->t_flags;

	if (enable_disable == 1) {
		task->t_flags |= reasons;
		if ((origflags & TF_TELEMETRY) == 0) {
			OSIncrementAtomic(&telemetry_active_tasks);
#if TELEMETRY_DEBUG
			printf("%s: telemetry OFF -> ON (%d active)\n", proc_name_address(task->bsd_info), telemetry_active_tasks);
#endif
		}
	} else {
		task->t_flags &= ~reasons;
		if (((origflags & TF_TELEMETRY) != 0) && ((task->t_flags & TF_TELEMETRY) == 0)) {
			  
                                                                              
                                                           
      
			OSDecrementAtomic(&telemetry_active_tasks);
#if TELEMETRY_DEBUG
			printf("%s: telemetry ON -> OFF (%d active)\n", proc_name_address(task->bsd_info), telemetry_active_tasks);
#endif
		}
	}
}

  
                                                             
  
                                                                               
                                                   
                                                       
   
static boolean_t
telemetry_is_active(thread_t thread)
{
	task_t task = thread->task;

	if (task == kernel_task) {
		                                                                        
		return FALSE;
	}

	if (telemetry_sample_all_tasks || telemetry_sample_pmis) {
		return TRUE;
	}

	if ((telemetry_active_tasks > 0) && ((thread->task->t_flags & TF_TELEMETRY) != 0)) {
		return TRUE;
	}

	return FALSE;
}

  
                                                                    
                                                                         
                                    
   
int
telemetry_timer_event(__unused uint64_t deadline, __unused uint64_t interval, __unused uint64_t leeway)
{
	if (telemetry_needs_timer_arming_record == TRUE) {
		telemetry_needs_timer_arming_record = FALSE;
		telemetry_take_sample(current_thread(), kTimerArmingRecord | kUserMode, &telemetry_buffer);
	}

	return 0;
}

#if defined(MT_CORE_INSTRS) && defined(MT_CORE_CYCLES)
static void
telemetry_pmi_handler(bool user_mode, __unused void *ctx)
{
	telemetry_mark_curthread(user_mode, TRUE);
}
#endif                                                         

int
telemetry_pmi_setup(enum telemetry_pmi pmi_ctr, uint64_t period)
{
#if defined(MT_CORE_INSTRS) && defined(MT_CORE_CYCLES)
	static boolean_t sample_all_tasks_aside = FALSE;
	static uint32_t active_tasks_aside = FALSE;
	int error = 0;
	const char *name = "?";

	unsigned int ctr = 0;

	TELEMETRY_PMI_LOCK();

	switch (pmi_ctr) {
	case TELEMETRY_PMI_NONE:
		if (!telemetry_sample_pmis) {
			error = 1;
			goto out;
		}

		telemetry_sample_pmis = FALSE;
		telemetry_sample_all_tasks = sample_all_tasks_aside;
		telemetry_active_tasks = active_tasks_aside;
		error = mt_microstackshot_stop();
		if (!error) {
			printf("telemetry: disabling ustackshot on PMI\n");
		}
		goto out;

	case TELEMETRY_PMI_INSTRS:
		ctr = MT_CORE_INSTRS;
		name = "instructions";
		break;

	case TELEMETRY_PMI_CYCLES:
		ctr = MT_CORE_CYCLES;
		name = "cycles";
		break;

	default:
		error = 1;
		goto out;
	}

	telemetry_sample_pmis = TRUE;
	sample_all_tasks_aside = telemetry_sample_all_tasks;
	active_tasks_aside = telemetry_active_tasks;
	telemetry_sample_all_tasks = FALSE;
	telemetry_active_tasks = 0;

	error = mt_microstackshot_start(ctr, period, telemetry_pmi_handler, NULL);
	if (!error) {
		printf("telemetry: ustackshot every %llu %s\n", period, name);
	}

out:
	TELEMETRY_PMI_UNLOCK();
	return error;
#else                                                         
#pragma unused(pmi_ctr, period)
	return 1;
#endif                                                           
}

  
                                                 
                                                            
   
void
telemetry_mark_curthread(boolean_t interrupted_userspace, boolean_t pmi)
{
	uint32_t ast_bits = 0;
	thread_t thread = current_thread();

	  
                                                             
                    
    
	if (telemetry_is_active(thread) == FALSE) {
		return;
	}

	ast_bits |= (interrupted_userspace ? AST_TELEMETRY_USER : AST_TELEMETRY_KERNEL);
	if (pmi) {
		ast_bits |= AST_TELEMETRY_PMI;
	}

	telemetry_needs_record = FALSE;
	thread_ast_set(thread, ast_bits);
	ast_propagate(thread);
}

void
compute_telemetry(void *arg __unused)
{
	if (telemetry_sample_all_tasks || (telemetry_active_tasks > 0)) {
		if ((++telemetry_timestamp) % telemetry_sample_rate == 0) {
			telemetry_needs_record = TRUE;
			telemetry_needs_timer_arming_record = TRUE;
		}
	}
}

  
                                                                               
   
static void
telemetry_notify_user(void)
{
	mach_port_t user_port = MACH_PORT_NULL;

	kern_return_t kr = host_get_telemetry_port(host_priv_self(), &user_port);
	if ((kr != KERN_SUCCESS) || !IPC_PORT_VALID(user_port)) {
		return;
	}

	telemetry_notification(user_port, 0);
	ipc_port_release_send(user_port);
}

void
telemetry_ast(thread_t thread, ast_t reasons)
{
	assert((reasons & AST_TELEMETRY_ALL) != 0);

	uint8_t record_type = 0;
	if (reasons & AST_TELEMETRY_IO) {
		record_type |= kIORecord;
	}
	if (reasons & (AST_TELEMETRY_USER | AST_TELEMETRY_KERNEL)) {
		record_type |= (reasons & AST_TELEMETRY_PMI) ? kPMIRecord :
		    kInterruptRecord;
	}

	uint8_t user_telemetry = (reasons & AST_TELEMETRY_USER) ? kUserMode : 0;

	uint8_t microsnapshot_flags = record_type | user_telemetry;

	telemetry_take_sample(thread, microsnapshot_flags, &telemetry_buffer);
}

void
telemetry_take_sample(thread_t thread, uint8_t microsnapshot_flags, struct micro_snapshot_buffer * current_buffer)
{
	task_t task;
	void *p;
	uint32_t btcount = 0, bti;
	struct micro_snapshot *msnap;
	struct task_snapshot *tsnap;
	struct thread_snapshot *thsnap;
	clock_sec_t secs;
	clock_usec_t usecs;
	vm_size_t framesize;
	uint32_t current_record_start;
	uint32_t tmp = 0;
	boolean_t notify = FALSE;

	if (thread == THREAD_NULL) {
		return;
	}

	task = thread->task;
	if ((task == TASK_NULL) || (task == kernel_task) || task_did_exec(task) || task_is_exec_copy(task)) {
		return;
	}

	                                                                     
	KDBG(MACHDBG_CODE(DBG_MACH_STACKSHOT, MICROSTACKSHOT_RECORD) | DBG_FUNC_START,
	    microsnapshot_flags, telemetry_bytes_since_last_mark, 0,
	    (&telemetry_buffer != current_buffer));

	p = get_bsdtask_info(task);

	  
                                                                                        
                                                                                       
                                                      
    

	uintptr_t frames[128];
	bool user64_regs = false;
	int bterror = 0;
	btcount = backtrace_user(frames,
	    sizeof(frames) / sizeof(frames[0]), &bterror, &user64_regs, NULL);
	if (bterror != 0) {
		return;
	}
	bool user64_va = task_has_64Bit_addr(task);

	  
                                                                
                                                                
   
                                    
    
	uint32_t uuid_info_count = 0;
	mach_vm_address_t uuid_info_addr = 0;
	uint32_t uuid_info_size = 0;
	if (user64_va) {
		uuid_info_size = sizeof(struct user64_dyld_uuid_info);
		struct user64_dyld_all_image_infos task_image_infos;
		if (copyin(task->all_image_info_addr, (char *)&task_image_infos, sizeof(task_image_infos)) == 0) {
			uuid_info_count = (uint32_t)task_image_infos.uuidArrayCount;
			uuid_info_addr = task_image_infos.uuidArray;
		}
	} else {
		uuid_info_size = sizeof(struct user32_dyld_uuid_info);
		struct user32_dyld_all_image_infos task_image_infos;
		if (copyin(task->all_image_info_addr, (char *)&task_image_infos, sizeof(task_image_infos)) == 0) {
			uuid_info_count = task_image_infos.uuidArrayCount;
			uuid_info_addr = task_image_infos.uuidArray;
		}
	}

	  
                                                                                                  
                                                                                                 
                  
    
	if (!uuid_info_addr) {
		uuid_info_count = 0;
	}

	  
                                                                                
                                                                 
    
	if (uuid_info_count > TELEMETRY_MAX_UUID_COUNT) {
		uuid_info_count = TELEMETRY_MAX_UUID_COUNT;
	}

	uint32_t uuid_info_array_size = uuid_info_count * uuid_info_size;
	char     *uuid_info_array = NULL;

	if (uuid_info_count > 0) {
		uuid_info_array = kheap_alloc(KHEAP_TEMP,
		    uuid_info_array_size, Z_WAITOK);
		if (uuid_info_array == NULL) {
			return;
		}

		  
                                 
                                                                                           
     
		if (copyin(uuid_info_addr, uuid_info_array, uuid_info_array_size) != 0) {
			kheap_free(KHEAP_TEMP, uuid_info_array, uuid_info_array_size);
			uuid_info_array = NULL;
			uuid_info_array_size = 0;
		}
	}

	  
                                                                                     
    
	uint64_t dqserialnum = 0;
	int              dqserialnum_valid = 0;

	uint64_t dqkeyaddr = thread_dispatchqaddr(thread);
	if (dqkeyaddr != 0) {
		uint64_t dqaddr = 0;
		uint64_t dq_serialno_offset = get_task_dispatchqueue_serialno_offset(task);
		if ((copyin(dqkeyaddr, (char *)&dqaddr, (user64_va ? 8 : 4)) == 0) &&
		    (dqaddr != 0) && (dq_serialno_offset != 0)) {
			uint64_t dqserialnumaddr = dqaddr + dq_serialno_offset;
			if (copyin(dqserialnumaddr, (char *)&dqserialnum, (user64_va ? 8 : 4)) == 0) {
				dqserialnum_valid = 1;
			}
		}
	}

	clock_get_calendar_microtime(&secs, &usecs);

	TELEMETRY_LOCK();

	  
                                            
                                                                               
                             
    
	if (!current_buffer->buffer) {
		goto cancel_sample;
	}

	  
                                                                                
                                                                             
                                                                           
                                                                              
                      
    

copytobuffer:

	current_record_start = current_buffer->current_position;

	if ((current_buffer->size - current_buffer->current_position) < sizeof(struct micro_snapshot)) {
		  
                                                                                   
                                                                    
     
		current_buffer->end_point = current_record_start;
		current_buffer->current_position = 0;
		if (current_record_start == 0) {
			                                                                                         
			goto cancel_sample;
		}
		goto copytobuffer;
	}

	msnap = (struct micro_snapshot *)(uintptr_t)(current_buffer->buffer + current_buffer->current_position);
	msnap->snapshot_magic = STACKSHOT_MICRO_SNAPSHOT_MAGIC;
	msnap->ms_flags = microsnapshot_flags;
	msnap->ms_opaque_flags = 0;                                     
	msnap->ms_cpu = cpu_number();
	msnap->ms_time = secs;
	msnap->ms_time_microsecs = usecs;

	current_buffer->current_position += sizeof(struct micro_snapshot);

	if ((current_buffer->size - current_buffer->current_position) < sizeof(struct task_snapshot)) {
		current_buffer->end_point = current_record_start;
		current_buffer->current_position = 0;
		if (current_record_start == 0) {
			                                                                                         
			goto cancel_sample;
		}
		goto copytobuffer;
	}

	tsnap = (struct task_snapshot *)(uintptr_t)(current_buffer->buffer + current_buffer->current_position);
	bzero(tsnap, sizeof(*tsnap));
	tsnap->snapshot_magic = STACKSHOT_TASK_SNAPSHOT_MAGIC;
	tsnap->pid = proc_pid(p);
	tsnap->uniqueid = proc_uniqueid(p);
	tsnap->user_time_in_terminated_threads = task->total_user_time;
	tsnap->system_time_in_terminated_threads = task->total_system_time;
	tsnap->suspend_count = task->suspend_count;
	tsnap->task_size = (typeof(tsnap->task_size))(get_task_phys_footprint(task) / PAGE_SIZE);
	tsnap->faults = counter_load(&task->faults);
	tsnap->pageins = task->pageins;
	tsnap->cow_faults = task->cow_faults;
	  
                                                                         
                                                                               
                                                                                   
                                                                           
                                                                
    
	tsnap->was_throttled = (uint32_t) proc_was_throttled(p);
	tsnap->did_throttle = (uint32_t) proc_did_throttle(p);

	if (task->t_flags & TF_TELEMETRY) {
		tsnap->ss_flags |= kTaskRsrcFlagged;
	}

	if (proc_get_effective_task_policy(task, TASK_POLICY_DARWIN_BG)) {
		tsnap->ss_flags |= kTaskDarwinBG;
	}

	proc_get_darwinbgstate(task, &tmp);

	if (proc_get_effective_task_policy(task, TASK_POLICY_ROLE) == TASK_FOREGROUND_APPLICATION) {
		tsnap->ss_flags |= kTaskIsForeground;
	}

	if (tmp & PROC_FLAG_ADAPTIVE_IMPORTANT) {
		tsnap->ss_flags |= kTaskIsBoosted;
	}

	if (tmp & PROC_FLAG_SUPPRESSED) {
		tsnap->ss_flags |= kTaskIsSuppressed;
	}


	tsnap->latency_qos = task_grab_latency_qos(task);

	strlcpy(tsnap->p_comm, proc_name_address(p), sizeof(tsnap->p_comm));
	if (user64_va) {
		tsnap->ss_flags |= kUser64_p;
	}


	if (task->task_shared_region_slide != -1) {
		tsnap->shared_cache_slide = task->task_shared_region_slide;
		bcopy(task->task_shared_region_uuid, tsnap->shared_cache_identifier,
		    sizeof(task->task_shared_region_uuid));
	}

	current_buffer->current_position += sizeof(struct task_snapshot);

	  
                                                                                             
                      
    
	if ((current_buffer->size - current_buffer->current_position) < uuid_info_array_size) {
		current_buffer->end_point = current_record_start;
		current_buffer->current_position = 0;
		if (current_record_start == 0) {
			                                                                                         
			goto cancel_sample;
		}
		goto copytobuffer;
	}

	  
                                             
    
	if (uuid_info_array_size > 0) {
		bcopy(uuid_info_array, (char *)(current_buffer->buffer + current_buffer->current_position), uuid_info_array_size);
		tsnap->nloadinfos = uuid_info_count;
	}

	current_buffer->current_position += uuid_info_array_size;

	  
                                                                               
    

	if ((current_buffer->size - current_buffer->current_position) < sizeof(struct thread_snapshot)) {
		                        
		current_buffer->end_point = current_record_start;
		current_buffer->current_position = 0;
		if (current_record_start == 0) {
			                                                                                         
			goto cancel_sample;
		}
		goto copytobuffer;
	}

	thsnap = (struct thread_snapshot *)(uintptr_t)(current_buffer->buffer + current_buffer->current_position);
	bzero(thsnap, sizeof(*thsnap));

	thsnap->snapshot_magic = STACKSHOT_THREAD_SNAPSHOT_MAGIC;
	thsnap->thread_id = thread_tid(thread);
	thsnap->state = thread->state;
	thsnap->priority = thread->base_pri;
	thsnap->sched_pri = thread->sched_pri;
	thsnap->sched_flags = thread->sched_flags;
	thsnap->ss_flags |= kStacksPCOnly;
	thsnap->ts_qos = thread->effective_policy.thep_qos;
	thsnap->ts_rqos = thread->requested_policy.thrp_qos;
	thsnap->ts_rqos_override = MAX(thread->requested_policy.thrp_qos_override,
	    thread->requested_policy.thrp_qos_workq_override);

	if (proc_get_effective_thread_policy(thread, TASK_POLICY_DARWIN_BG)) {
		thsnap->ss_flags |= kThreadDarwinBG;
	}

	thsnap->user_time = timer_grab(&thread->user_timer);

	uint64_t tval = timer_grab(&thread->system_timer);

	if (thread->precise_user_kernel_time) {
		thsnap->system_time = tval;
	} else {
		thsnap->user_time += tval;
		thsnap->system_time = 0;
	}

	current_buffer->current_position += sizeof(struct thread_snapshot);

	  
                                                                       
    
	if (dqserialnum_valid) {
		if ((current_buffer->size - current_buffer->current_position) < sizeof(dqserialnum)) {
			                        
			current_buffer->end_point = current_record_start;
			current_buffer->current_position = 0;
			if (current_record_start == 0) {
				                                                                                         
				goto cancel_sample;
			}
			goto copytobuffer;
		}

		thsnap->ss_flags |= kHasDispatchSerial;
		bcopy(&dqserialnum, (char *)current_buffer->buffer + current_buffer->current_position, sizeof(dqserialnum));
		current_buffer->current_position += sizeof(dqserialnum);
	}

	if (user64_regs) {
		framesize = 8;
		thsnap->ss_flags |= kUser64_p;
	} else {
		framesize = 4;
	}

	  
                                                                                          
                                                                
    
	if ((current_buffer->size - current_buffer->current_position) / framesize < btcount) {
		current_buffer->end_point = current_record_start;
		current_buffer->current_position = 0;
		if (current_record_start == 0) {
			                                                                                         
			goto cancel_sample;
		}
		goto copytobuffer;
	}

	for (bti = 0; bti < btcount; bti++, current_buffer->current_position += framesize) {
		if (framesize == 8) {
			*(uint64_t *)(uintptr_t)(current_buffer->buffer + current_buffer->current_position) = frames[bti];
		} else {
			*(uint32_t *)(uintptr_t)(current_buffer->buffer + current_buffer->current_position) = (uint32_t)frames[bti];
		}
	}

	if (current_buffer->end_point < current_buffer->current_position) {
		  
                                                                   
                                                                    
                                                                     
                                                             
     
		current_buffer->end_point = current_buffer->current_position;
	}

	thsnap->nuser_frames = btcount;

	  
                       
    
	if (current_buffer == &telemetry_buffer) {
		telemetry_bytes_since_last_mark += (current_buffer->current_position - current_record_start);
		if (telemetry_bytes_since_last_mark > telemetry_buffer_notify_at) {
			notify = TRUE;
		}
	}

cancel_sample:
	TELEMETRY_UNLOCK();

	KDBG(MACHDBG_CODE(DBG_MACH_STACKSHOT, MICROSTACKSHOT_RECORD) | DBG_FUNC_END,
	    notify, telemetry_bytes_since_last_mark,
	    current_buffer->current_position, current_buffer->end_point);

	if (notify) {
		telemetry_notify_user();
	}

	if (uuid_info_array != NULL) {
		kheap_free(KHEAP_TEMP, uuid_info_array, uuid_info_array_size);
	}
}

#if TELEMETRY_DEBUG
static void
log_telemetry_output(vm_offset_t buf, uint32_t pos, uint32_t sz)
{
	struct micro_snapshot *p;
	uint32_t offset;

	printf("Copying out %d bytes of telemetry at offset %d\n", sz, pos);

	buf += pos;

	  
                                                        
    
	for (offset = 0; offset < sz; offset++) {
		p = (struct micro_snapshot *)(buf + offset);
		if (p->snapshot_magic == STACKSHOT_MICRO_SNAPSHOT_MAGIC) {
			printf("telemetry timestamp: %lld\n", p->ms_time);
		}
	}
}
#endif

int
telemetry_gather(user_addr_t buffer, uint32_t *length, boolean_t mark)
{
	return telemetry_buffer_gather(buffer, length, mark, &telemetry_buffer);
}

int
telemetry_buffer_gather(user_addr_t buffer, uint32_t *length, boolean_t mark, struct micro_snapshot_buffer * current_buffer)
{
	int result = 0;
	uint32_t oldest_record_offset;

	KDBG(MACHDBG_CODE(DBG_MACH_STACKSHOT, MICROSTACKSHOT_GATHER) | DBG_FUNC_START,
	    mark, telemetry_bytes_since_last_mark, 0,
	    (&telemetry_buffer != current_buffer));

	TELEMETRY_LOCK();

	if (current_buffer->buffer == 0) {
		*length = 0;
		goto out;
	}

	if (*length < current_buffer->size) {
		result = KERN_NO_SPACE;
		goto out;
	}

	  
                                                                                              
                                                                                             
    
	oldest_record_offset = current_buffer->current_position;
	do {
		if (((oldest_record_offset + sizeof(uint32_t)) > current_buffer->size) ||
		    ((oldest_record_offset + sizeof(uint32_t)) > current_buffer->end_point)) {
			if (*(uint32_t *)(uintptr_t)(current_buffer->buffer) == 0) {
				  
                                                                       
                                           
       
				*length = 0;
				goto out;
			}
			  
                                                                               
                                                                              
                                   
      

			oldest_record_offset = 0;
			assert(*(uint32_t *)(uintptr_t)(current_buffer->buffer) == STACKSHOT_MICRO_SNAPSHOT_MAGIC);
			break;
		}

		if (*(uint32_t *)(uintptr_t)(current_buffer->buffer + oldest_record_offset) == STACKSHOT_MICRO_SNAPSHOT_MAGIC) {
			break;
		}

		  
                                                                                             
                 
     
		oldest_record_offset++;
	} while (oldest_record_offset != current_buffer->current_position);

	  
                                                                                               
                                                                
    
	if (oldest_record_offset != 0) {
#if TELEMETRY_DEBUG
		log_telemetry_output(current_buffer->buffer, oldest_record_offset,
		    current_buffer->end_point - oldest_record_offset);
#endif
		if ((result = copyout((void *)(current_buffer->buffer + oldest_record_offset), buffer,
		    current_buffer->end_point - oldest_record_offset)) != 0) {
			*length = 0;
			goto out;
		}
		*length = current_buffer->end_point - oldest_record_offset;
	} else {
		*length = 0;
	}

#if TELEMETRY_DEBUG
	log_telemetry_output(current_buffer->buffer, 0, current_buffer->current_position);
#endif
	if ((result = copyout((void *)current_buffer->buffer, buffer + *length,
	    current_buffer->current_position)) != 0) {
		*length = 0;
		goto out;
	}
	*length += (uint32_t)current_buffer->current_position;

out:

	if (mark && (*length > 0)) {
		telemetry_bytes_since_last_mark = 0;
	}

	TELEMETRY_UNLOCK();

	KDBG(MACHDBG_CODE(DBG_MACH_STACKSHOT, MICROSTACKSHOT_GATHER) | DBG_FUNC_END,
	    current_buffer->current_position, *length,
	    current_buffer->end_point, (&telemetry_buffer != current_buffer));

	return result;
}

                          
                          
                          
  
                 
  
                                                                                
                                                                                       
                                                                                      
                                                                                   
                                                                          
                                                                           
  
                                  
  
                                                                        
                                                                            
                                                                              
                                                                                
                                                         
  
                 
  
                                                                                 
                                                          
   

#define BOOTPROFILE_MAX_BUFFER_SIZE (64*1024*1024)                                   

vm_offset_t         bootprofile_buffer = 0;
uint32_t            bootprofile_buffer_size = 0;
uint32_t            bootprofile_buffer_current_position = 0;
uint32_t            bootprofile_interval_ms = 0;
uint64_t            bootprofile_stackshot_flags = 0;
uint64_t            bootprofile_interval_abs = 0;
uint64_t            bootprofile_next_deadline = 0;
uint32_t            bootprofile_all_procs = 0;
char                bootprofile_proc_name[17];
uint64_t            bootprofile_delta_since_timestamp = 0;
LCK_GRP_DECLARE(bootprofile_lck_grp, "bootprofile_group");
LCK_MTX_DECLARE(bootprofile_mtx, &bootprofile_lck_grp);


enum {
	kBootProfileDisabled = 0,
	kBootProfileStartTimerAtBoot,
	kBootProfileStartTimerAtWake
} bootprofile_type = kBootProfileDisabled;


static timer_call_data_t        bootprofile_timer_call_entry;

#define BOOTPROFILE_LOCK() do { lck_mtx_lock(&bootprofile_mtx); } while(0)
#define BOOTPROFILE_TRY_SPIN_LOCK() lck_mtx_try_lock_spin(&bootprofile_mtx)
#define BOOTPROFILE_UNLOCK() do { lck_mtx_unlock(&bootprofile_mtx); } while(0)

static void bootprofile_timer_call(
	timer_call_param_t      param0,
	timer_call_param_t      param1);

void
bootprofile_init(void)
{
	kern_return_t ret;
	char type[32];

	if (!PE_parse_boot_argn("bootprofile_buffer_size",
	    &bootprofile_buffer_size, sizeof(bootprofile_buffer_size))) {
		bootprofile_buffer_size = 0;
	}

	if (bootprofile_buffer_size > BOOTPROFILE_MAX_BUFFER_SIZE) {
		bootprofile_buffer_size = BOOTPROFILE_MAX_BUFFER_SIZE;
	}

	if (!PE_parse_boot_argn("bootprofile_interval_ms",
	    &bootprofile_interval_ms, sizeof(bootprofile_interval_ms))) {
		bootprofile_interval_ms = 0;
	}

	if (!PE_parse_boot_argn("bootprofile_stackshot_flags",
	    &bootprofile_stackshot_flags, sizeof(bootprofile_stackshot_flags))) {
		bootprofile_stackshot_flags = 0;
	}

	if (!PE_parse_boot_argn("bootprofile_proc_name",
	    &bootprofile_proc_name, sizeof(bootprofile_proc_name))) {
		bootprofile_all_procs = 1;
		bootprofile_proc_name[0] = '\0';
	}

	if (PE_parse_boot_argn("bootprofile_type", type, sizeof(type))) {
		if (0 == strcmp(type, "boot")) {
			bootprofile_type = kBootProfileStartTimerAtBoot;
		} else if (0 == strcmp(type, "wake")) {
			bootprofile_type = kBootProfileStartTimerAtWake;
		} else {
			bootprofile_type = kBootProfileDisabled;
		}
	} else {
		bootprofile_type = kBootProfileDisabled;
	}

	clock_interval_to_absolutetime_interval(bootprofile_interval_ms, NSEC_PER_MSEC, &bootprofile_interval_abs);

	                                          
	if ((bootprofile_type == kBootProfileDisabled) || (bootprofile_buffer_size == 0) || (bootprofile_interval_abs == 0)) {
		return;
	}

	ret = kmem_alloc(kernel_map, &bootprofile_buffer, bootprofile_buffer_size, VM_KERN_MEMORY_DIAG);
	if (ret != KERN_SUCCESS) {
		kprintf("Boot profile: Allocation failed: %d\n", ret);
		return;
	}
	bzero((void *) bootprofile_buffer, bootprofile_buffer_size);

	kprintf("Boot profile: Sampling %s once per %u ms at %s\n",
	    bootprofile_all_procs ? "all procs" : bootprofile_proc_name, bootprofile_interval_ms,
	    bootprofile_type == kBootProfileStartTimerAtBoot ? "boot" : (bootprofile_type == kBootProfileStartTimerAtWake ? "wake" : "unknown"));

	timer_call_setup(&bootprofile_timer_call_entry,
	    bootprofile_timer_call,
	    NULL);

	if (bootprofile_type == kBootProfileStartTimerAtBoot) {
		bootprofile_next_deadline = mach_absolute_time() + bootprofile_interval_abs;
		timer_call_enter_with_leeway(&bootprofile_timer_call_entry,
		    NULL,
		    bootprofile_next_deadline,
		    0,
		    TIMER_CALL_SYS_NORMAL,
		    FALSE);
	}
}

void
bootprofile_wake_from_sleep(void)
{
	if (bootprofile_type == kBootProfileStartTimerAtWake) {
		bootprofile_next_deadline = mach_absolute_time() + bootprofile_interval_abs;
		timer_call_enter_with_leeway(&bootprofile_timer_call_entry,
		    NULL,
		    bootprofile_next_deadline,
		    0,
		    TIMER_CALL_SYS_NORMAL,
		    FALSE);
	}
}


static void
bootprofile_timer_call(
	timer_call_param_t      param0 __unused,
	timer_call_param_t      param1 __unused)
{
	unsigned retbytes = 0;
	int pid_to_profile = -1;

	if (!BOOTPROFILE_TRY_SPIN_LOCK()) {
		goto reprogram;
	}

	                                                           
	if (!bootprofile_all_procs) {
		  
                                                                 
                                                                   
                                
     

		if ((current_task() != NULL) && (current_task()->bsd_info != NULL) &&
		    (0 == strncmp(bootprofile_proc_name, proc_name_address(current_task()->bsd_info), 17))) {
			pid_to_profile = proc_selfpid();
		} else {
			  
                                                                          
                                         
      
			BOOTPROFILE_UNLOCK();
			goto reprogram;
		}
	}

	                                                                      
	if (bootprofile_buffer_current_position < bootprofile_buffer_size) {
		uint64_t flags = STACKSHOT_KCDATA_FORMAT | STACKSHOT_TRYLOCK | STACKSHOT_SAVE_LOADINFO
		    | STACKSHOT_GET_GLOBAL_MEM_STATS;
#if defined(XNU_TARGET_OS_OSX)
		flags |= STACKSHOT_SAVE_KEXT_LOADINFO;
#endif


		                                        
		flags |= bootprofile_stackshot_flags;
		if ((flags & STACKSHOT_COLLECT_DELTA_SNAPSHOT) && (bootprofile_delta_since_timestamp == 0)) {
			                                           
			flags &= ~STACKSHOT_COLLECT_DELTA_SNAPSHOT;
		}

		uint64_t timestamp = 0;
		if (bootprofile_stackshot_flags & STACKSHOT_COLLECT_DELTA_SNAPSHOT) {
			timestamp = mach_absolute_time();
		}

		kern_return_t r = stack_snapshot_from_kernel(
			pid_to_profile, (void *)(bootprofile_buffer + bootprofile_buffer_current_position),
			bootprofile_buffer_size - bootprofile_buffer_current_position,
			flags, bootprofile_delta_since_timestamp, 0, &retbytes);

		  
                                                                         
                                                                        
                     
     

		if (r == KERN_LOCK_OWNED) {
			BOOTPROFILE_UNLOCK();
			goto reprogram;
		}

		if (bootprofile_stackshot_flags & STACKSHOT_COLLECT_DELTA_SNAPSHOT &&
		    r == KERN_SUCCESS) {
			bootprofile_delta_since_timestamp = timestamp;
		}

		bootprofile_buffer_current_position += retbytes;
	}

	BOOTPROFILE_UNLOCK();

	                                                                               
	if ((retbytes == 0) || (bootprofile_buffer_current_position == bootprofile_buffer_size)) {
		return;
	}


reprogram:
	                                                                
	if (bootprofile_interval_abs == 0) {
		return;
	}

	clock_deadline_for_periodic_event(bootprofile_interval_abs,
	    mach_absolute_time(),
	    &bootprofile_next_deadline);
	timer_call_enter_with_leeway(&bootprofile_timer_call_entry,
	    NULL,
	    bootprofile_next_deadline,
	    0,
	    TIMER_CALL_SYS_NORMAL,
	    FALSE);
}

void
bootprofile_get(void **buffer, uint32_t *length)
{
	BOOTPROFILE_LOCK();
	*buffer = (void*) bootprofile_buffer;
	*length = bootprofile_buffer_current_position;
	BOOTPROFILE_UNLOCK();
}

int
bootprofile_gather(user_addr_t buffer, uint32_t *length)
{
	int result = 0;

	BOOTPROFILE_LOCK();

	if (bootprofile_buffer == 0) {
		*length = 0;
		goto out;
	}

	if (*length < bootprofile_buffer_current_position) {
		result = KERN_NO_SPACE;
		goto out;
	}

	if ((result = copyout((void *)bootprofile_buffer, buffer,
	    bootprofile_buffer_current_position)) != 0) {
		*length = 0;
		goto out;
	}
	*length = bootprofile_buffer_current_position;

	                          
	bootprofile_interval_abs = 0;

out:

	BOOTPROFILE_UNLOCK();

	return result;
}
