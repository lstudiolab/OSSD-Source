/*
 * Copyright (c) 2006-2018 Apple Inc. All rights reserved.
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
 *
 */

#include <kern/sched_prim.h>
#include <kern/kalloc.h>
#include <kern/assert.h>
#include <kern/debug.h>
#include <kern/locks.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/host.h>
#include <kern/policy_internal.h>
#include <kern/thread_group.h>

#include <libkern/libkern.h>
#include <mach/coalition.h>
#include <mach/mach_time.h>
#include <mach/task.h>
#include <mach/host_priv.h>
#include <mach/mach_host.h>
#include <os/log.h>
#include <pexpert/pexpert.h>
#include <sys/coalition.h>
#include <sys/kern_event.h>
#include <sys/proc.h>
#include <sys/proc_info.h>
#include <sys/reason.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/sysctl.h>
#include <sys/sysproto.h>
#include <sys/wait.h>
#include <sys/tree.h>
#include <sys/priv.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>
#include <mach/machine/sdt.h>
#include <libkern/section_keywords.h>
#include <stdatomic.h>

#include <IOKit/IOBSD.h>

#if CONFIG_FREEZE
#include <vm/vm_map.h>
#endif                    

#include <sys/kern_memorystatus.h>
#include <sys/kern_memorystatus_freeze.h>
#include <sys/kern_memorystatus_notify.h>

#if CONFIG_JETSAM

extern unsigned int memorystatus_available_pages;
extern unsigned int memorystatus_available_pages_pressure;
extern unsigned int memorystatus_available_pages_critical;
extern unsigned int memorystatus_available_pages_critical_base;
extern unsigned int memorystatus_available_pages_critical_idle_offset;

#else                    

extern uint64_t memorystatus_available_pages;
extern uint64_t memorystatus_available_pages_pressure;
extern uint64_t memorystatus_available_pages_critical;

#endif                    

unsigned int memorystatus_frozen_count = 0;
unsigned int memorystatus_suspended_count = 0;
unsigned long freeze_threshold_percentage = 50;

#if CONFIG_FREEZE

static LCK_GRP_DECLARE(freezer_lck_grp, "freezer");
static LCK_MTX_DECLARE(freezer_mutex, &freezer_lck_grp);

                
unsigned int memorystatus_freeze_threshold = 0;
unsigned int memorystatus_freeze_pages_min = 0;
unsigned int memorystatus_freeze_pages_max = 0;
unsigned int memorystatus_freeze_suspended_threshold = FREEZE_SUSPENDED_THRESHOLD_DEFAULT;
unsigned int memorystatus_freeze_daily_mb_max = FREEZE_DAILY_MB_MAX_DEFAULT;
uint64_t     memorystatus_freeze_budget_pages_remaining = 0;                                                      
boolean_t memorystatus_freeze_degradation = FALSE;                                                                                

unsigned int memorystatus_max_frozen_demotions_daily = 0;
unsigned int memorystatus_thaw_count_demotion_threshold = 0;

boolean_t memorystatus_freeze_enabled = FALSE;
int memorystatus_freeze_wakeup = 0;
int memorystatus_freeze_jetsam_band = 0;                                                                    

#define MAX_XPC_SERVICE_PIDS 10                                                                    

#ifdef XNU_KERNEL_PRIVATE

unsigned int memorystatus_frozen_processes_max = 0;
unsigned int memorystatus_frozen_shared_mb = 0;
unsigned int memorystatus_frozen_shared_mb_max = 0;
unsigned int memorystatus_freeze_shared_mb_per_process_max = 0;                                                          
unsigned int memorystatus_freeze_private_shared_pages_ratio = 2;                                                                          
unsigned int memorystatus_thaw_count = 0;                                                 
uint64_t memorystatus_thaw_count_since_boot = 0;                                     
unsigned int memorystatus_refreeze_eligible_count = 0;                                                                          

struct memorystatus_freezer_stats_t memorystatus_freezer_stats = {0};

#endif                         

static inline boolean_t memorystatus_can_freeze_processes(void);
static boolean_t memorystatus_can_freeze(boolean_t *memorystatus_freeze_swap_low);
static boolean_t memorystatus_is_process_eligible_for_freeze(proc_t p);
static void memorystatus_freeze_thread(void *param __unused, wait_result_t wr __unused);
static void memorystatus_freeze_start_normal_throttle_interval(uint32_t new_budget, mach_timespec_t start_ts);

void memorystatus_disable_freeze(void);

           
static uint64_t memorystatus_freeze_pageouts = 0;

                
#define DEGRADED_WINDOW_MINS    (30)
#define NORMAL_WINDOW_MINS      (24 * 60)

                                    
static throttle_interval_t throttle_intervals[] = {
	{ DEGRADED_WINDOW_MINS, 1, 0, 0, { 0, 0 }},
	{ NORMAL_WINDOW_MINS, 1, 0, 0, { 0, 0 }},
};
throttle_interval_t *degraded_throttle_window = &throttle_intervals[0];
throttle_interval_t *normal_throttle_window = &throttle_intervals[1];
uint32_t memorystatus_freeze_current_interval = 0;

extern uint64_t vm_swap_get_free_space(void);
extern boolean_t vm_swap_max_budget(uint64_t *);
extern int i_coal_jetsam_get_taskrole(coalition_t coal, task_t task);

static void memorystatus_freeze_update_throttle(uint64_t *budget_pages_allowed);
static void memorystatus_demote_frozen_processes(boolean_t force_one);

static void memorystatus_freeze_handle_error(proc_t p, const int freezer_error_code, bool was_refreeze, pid_t pid, const coalition_t coalition, const char* log_prefix);
static void memorystatus_freeze_out_of_slots(void);
static uint64_t memorystatus_freezer_thread_next_run_ts = 0;

                                   

SYSCTL_UINT(_kern, OID_AUTO, memorystatus_freeze_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_frozen_count, 0, "");
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_thaw_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_thaw_count, 0, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_thaw_count_since_boot, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_thaw_count_since_boot, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freeze_pageouts, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freeze_pageouts, "");
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_freeze_interval, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freeze_current_interval, 0, "");
#if DEVELOPMENT || DEBUG
static int sysctl_memorystatus_freeze_budget_pages_remaining SYSCTL_HANDLER_ARGS
{
	#pragma unused(arg1, arg2, oidp)
	int error, changed;
	uint64_t new_budget = memorystatus_freeze_budget_pages_remaining;
	mach_timespec_t now_ts;
	clock_sec_t sec;
	clock_nsec_t nsec;

	lck_mtx_lock(&freezer_mutex);

	error = sysctl_io_number(req, memorystatus_freeze_budget_pages_remaining, sizeof(uint64_t), &new_budget, &changed);
	if (changed) {
		                                            
		clock_get_system_nanotime(&sec, &nsec);
		now_ts.tv_sec = (unsigned int)(MIN(sec, UINT32_MAX));
		now_ts.tv_nsec = nsec;
		memorystatus_freeze_start_normal_throttle_interval((uint32_t) MIN(new_budget, UINT32_MAX), now_ts);
		                                                                           
		normal_throttle_window->pageouts = 0;
		memorystatus_freeze_budget_pages_remaining = normal_throttle_window->max_pageouts;
	}

	lck_mtx_unlock(&freezer_mutex);
	return error;
}

SYSCTL_PROC(_kern, OID_AUTO, memorystatus_freeze_budget_pages_remaining, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED, 0, 0, &sysctl_memorystatus_freeze_budget_pages_remaining, "Q", "");
#else                           
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freeze_budget_pages_remaining, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freeze_budget_pages_remaining, "");
#endif                           
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_error_excess_shared_memory_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_error_excess_shared_memory_count, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_error_low_private_shared_ratio_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_error_low_private_shared_ratio_count, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_error_no_compressor_space_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_error_no_compressor_space_count, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_error_no_swap_space_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_error_no_swap_space_count, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_error_below_min_pages_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_error_below_min_pages_count, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_error_low_probability_of_use_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_error_low_probability_of_use_count, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_error_other_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_error_other_count, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_process_considered_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_process_considered_count, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_below_threshold_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_below_threshold_count, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_skipped_full_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_skipped_full_count, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_skipped_shared_mb_high_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_skipped_shared_mb_high_count, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_shared_pages_skipped, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_shared_pages_skipped, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_bytes_refrozen, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_bytes_refrozen, "");
SYSCTL_QUAD(_kern, OID_AUTO, memorystatus_freezer_refreeze_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freezer_stats.mfs_refreeze_count, "");

static_assert(_kMemorystatusFreezeSkipReasonMax <= UINT8_MAX);


  
                                           
                                                                               
                                
                                                                                
   
static int sysctl_memorystatus_freezer_thaw_percentage SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	uint64_t thaw_count = 0, frozen_count = 0;
	int thaw_percentage = 100;
	frozen_count = os_atomic_load(&(memorystatus_freezer_stats.mfs_processes_frozen), relaxed);
	thaw_count = os_atomic_load(&(memorystatus_freezer_stats.mfs_processes_thawed), relaxed);

	if (frozen_count > 0) {
		if (thaw_count > frozen_count) {
			  
                                                                  
                                              
      
			thaw_percentage = 100;
		} else {
			thaw_percentage = (int)(100 * thaw_count / frozen_count);
		}
	}
	return sysctl_handle_int(oidp, &thaw_percentage, 0, req);
}
SYSCTL_PROC(_kern, OID_AUTO, memorystatus_freezer_thaw_percentage, CTLTYPE_INT | CTLFLAG_RD | CTLFLAG_LOCKED, 0, 0, &sysctl_memorystatus_freezer_thaw_percentage, "I", "");

#define FREEZER_ERROR_STRING_LENGTH 128

EXPERIMENT_FACTOR_UINT(_kern, memorystatus_freeze_pages_min, &memorystatus_freeze_pages_min, 0, UINT32_MAX, "");
EXPERIMENT_FACTOR_UINT(_kern, memorystatus_freeze_pages_max, &memorystatus_freeze_pages_max, 0, UINT32_MAX, "");
EXPERIMENT_FACTOR_UINT(_kern, memorystatus_freeze_processes_max, &memorystatus_frozen_processes_max, 0, UINT32_MAX, "");
EXPERIMENT_FACTOR_UINT(_kern, memorystatus_freeze_jetsam_band, &memorystatus_freeze_jetsam_band, JETSAM_PRIORITY_IDLE, JETSAM_PRIORITY_MAX - 1, "");
EXPERIMENT_FACTOR_UINT(_kern, memorystatus_freeze_private_shared_pages_ratio, &memorystatus_freeze_private_shared_pages_ratio, 0, UINT32_MAX, "");
EXPERIMENT_FACTOR_UINT(_kern, memorystatus_freeze_min_processes, &memorystatus_freeze_suspended_threshold, 0, UINT32_MAX, "");
  
                                                                       
   
EXPERIMENT_FACTOR_UINT(_kern, memorystatus_max_freeze_demotions_daily, &memorystatus_max_frozen_demotions_daily, 0, UINT32_MAX, "");

  
                                                                                            
   
EXPERIMENT_FACTOR_UINT(_kern, memorystatus_thaw_count_demotion_threshold, &memorystatus_thaw_count_demotion_threshold, 0, UINT32_MAX, "");

#if DEVELOPMENT || DEBUG

SYSCTL_UINT(_kern, OID_AUTO, memorystatus_freeze_daily_mb_max, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_freeze_daily_mb_max, 0, "");
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_freeze_degraded_mode, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_freeze_degradation, 0, "");
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_freeze_threshold, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_freeze_threshold, 0, "");
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_refreeze_eligible_count, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_refreeze_eligible_count, 0, "");

  
                                                                                                   
                      
                                            
   

SYSCTL_UINT(_kern, OID_AUTO, memorystatus_freeze_shared_mb_max, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_frozen_shared_mb_max, 0, "");
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_freeze_shared_mb, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_frozen_shared_mb, 0, "");

SYSCTL_UINT(_kern, OID_AUTO, memorystatus_freeze_shared_mb_per_process_max, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_freeze_shared_mb_per_process_max, 0, "");

boolean_t memorystatus_freeze_throttle_enabled = TRUE;
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_freeze_throttle_enabled, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_freeze_throttle_enabled, 0, "");

  
                                                                                                                        
                                                             
   
boolean_t memorystatus_freeze_to_memory = FALSE;
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_freeze_to_memory, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_freeze_to_memory, 0, "");

#define VM_PAGES_FOR_ALL_PROCS    (2)

  
                                                                  
   
static int
sysctl_memorystatus_freeze SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error, pid = 0;
	proc_t p;
	int freezer_error_code = 0;
	pid_t pid_list[MAX_XPC_SERVICE_PIDS];
	int ntasks = 0;
	coalition_t coal = COALITION_NULL;

	if (memorystatus_freeze_enabled == FALSE) {
		printf("sysctl_freeze: Freeze is DISABLED\n");
		return ENOTSUP;
	}

	error = sysctl_handle_int(oidp, &pid, 0, req);
	if (error || !req->newptr) {
		return error;
	}

	if (pid == VM_PAGES_FOR_ALL_PROCS) {
		vm_pageout_anonymous_pages();

		return 0;
	}

	lck_mtx_lock(&freezer_mutex);

again:
	p = proc_find(pid);
	if (p != NULL) {
		memorystatus_freezer_stats.mfs_process_considered_count++;
		uint32_t purgeable, wired, clean, dirty, shared;
		uint32_t max_pages = 0, state = 0;

		if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
			  
                                                       
                                
     
                                                                                            
                                                                                       
                                            
     
     
                                                                                           
                                                           
     
                                                                                               
                                                                                                
                                                                                                
      
			max_pages = memorystatus_freeze_pages_max;
		} else {
			  
                                                   
      
			max_pages = UINT32_MAX - 1;
		}

		proc_list_lock();
		state = p->p_memstat_state;
		proc_list_unlock();

		  
                                                                                                      
                                                                                                    
                                  
     
		if (state & (P_MEMSTAT_TERMINATED | P_MEMSTAT_LOCKED | P_MEMSTAT_FREEZE_DISABLED)) {
			printf("sysctl_freeze: p_memstat_state check failed, process is%s%s%s\n",
			    (state & P_MEMSTAT_TERMINATED) ? " terminated" : "",
			    (state & P_MEMSTAT_LOCKED) ? " locked" : "",
			    (state & P_MEMSTAT_FREEZE_DISABLED) ? " unfreezable" : "");

			proc_rele(p);
			lck_mtx_unlock(&freezer_mutex);
			return EPERM;
		}

		error = task_freeze(p->task, &purgeable, &wired, &clean, &dirty, max_pages, &shared, &freezer_error_code, FALSE                );
		if (!error || freezer_error_code == FREEZER_ERROR_LOW_PRIVATE_SHARED_RATIO) {
			memorystatus_freezer_stats.mfs_shared_pages_skipped += shared;
		}

		if (error) {
			memorystatus_freeze_handle_error(p, freezer_error_code, state & P_MEMSTAT_FROZEN, pid, coal, "sysctl_freeze");
			if (error == KERN_NO_SPACE) {
				                                                                                                        
				error = ENOSPC;
			} else {
				error = EIO;
			}
		} else {
			proc_list_lock();
			if ((p->p_memstat_state & P_MEMSTAT_FROZEN) == 0) {
				p->p_memstat_state |= P_MEMSTAT_FROZEN;
				p->p_memstat_freeze_skip_reason = kMemorystatusFreezeSkipReasonNone;
				memorystatus_frozen_count++;
				os_atomic_inc(&memorystatus_freezer_stats.mfs_processes_frozen, relaxed);
				if (memorystatus_frozen_count == memorystatus_frozen_processes_max) {
					memorystatus_freeze_out_of_slots();
				}
			} else {
				                       
				if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
					memorystatus_freezer_stats.mfs_bytes_refrozen += dirty * PAGE_SIZE;
					memorystatus_freezer_stats.mfs_refreeze_count++;
				}
			}
			p->p_memstat_frozen_count++;


			proc_list_unlock();

			if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
				  
                                                            
       
				error = memorystatus_update_inactive_jetsam_priority_band(pid, MEMORYSTATUS_CMD_ELEVATED_INACTIVEJETSAMPRIORITY_ENABLE,
				    memorystatus_freeze_jetsam_band, TRUE);

				if (error) {
					printf("sysctl_freeze: Elevating frozen process to higher jetsam band failed with %d\n", error);
				}
			}
		}

		if ((error == 0) && (coal == NULL)) {
			  
                                                         
                                                        
                          
                                                                 
                                                         
                                                              
                                                               
                                   
      

			task_t          curr_task = NULL;

			curr_task = proc_task(p);
			coal = task_get_coalition(curr_task, COALITION_TYPE_JETSAM);
			if (coalition_is_leader(curr_task, coal)) {
				ntasks = coalition_get_pid_list(coal, COALITION_ROLEMASK_XPC,
				    COALITION_SORT_DEFAULT, pid_list, MAX_XPC_SERVICE_PIDS);

				if (ntasks > MAX_XPC_SERVICE_PIDS) {
					ntasks = MAX_XPC_SERVICE_PIDS;
				}
			}
		}

		proc_rele(p);

		while (ntasks) {
			pid = pid_list[--ntasks];
			goto again;
		}

		lck_mtx_unlock(&freezer_mutex);
		return error;
	} else {
		printf("sysctl_freeze: Invalid process\n");
	}


	lck_mtx_unlock(&freezer_mutex);
	return EINVAL;
}

SYSCTL_PROC(_kern, OID_AUTO, memorystatus_freeze, CTLTYPE_INT | CTLFLAG_WR | CTLFLAG_LOCKED | CTLFLAG_MASKED,
    0, 0, &sysctl_memorystatus_freeze, "I", "");

  
                                                                            
   
static int
sysctl_memorystatus_demote_frozen_process SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2, oidp, req)
	int error, val;
	  
                                                                
                                            
    
	error = sysctl_handle_int(oidp, &val, 0, req);
	if (error || !req->newptr) {
		return error;
	}
	memorystatus_demote_frozen_processes(false);
	return 0;
}

SYSCTL_PROC(_kern, OID_AUTO, memorystatus_demote_frozen_processes, CTLTYPE_INT | CTLFLAG_WR | CTLFLAG_LOCKED | CTLFLAG_MASKED, 0, 0, &sysctl_memorystatus_demote_frozen_process, "I", "");

static int
sysctl_memorystatus_available_pages_thaw SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)

	int error, pid = 0;
	proc_t p;

	if (memorystatus_freeze_enabled == FALSE) {
		return ENOTSUP;
	}

	error = sysctl_handle_int(oidp, &pid, 0, req);
	if (error || !req->newptr) {
		return error;
	}

	if (pid == VM_PAGES_FOR_ALL_PROCS) {
		do_fastwake_warmup_all();
		return 0;
	} else {
		p = proc_find(pid);
		if (p != NULL) {
			error = task_thaw(p->task);

			if (error) {
				error = EIO;
			} else {
				  
                             
      
                                                                        
                                                                        
                               
      
                        
                                               
                          
       
			}
			proc_rele(p);
			return error;
		}
	}

	return EINVAL;
}

SYSCTL_PROC(_kern, OID_AUTO, memorystatus_thaw, CTLTYPE_INT | CTLFLAG_WR | CTLFLAG_LOCKED | CTLFLAG_MASKED,
    0, 0, &sysctl_memorystatus_available_pages_thaw, "I", "");


typedef struct _global_freezable_status {
	boolean_t       freeze_pages_threshold_crossed;
	boolean_t       freeze_eligible_procs_available;
	boolean_t       freeze_scheduled_in_future;
}global_freezable_status_t;

typedef struct _proc_freezable_status {
	boolean_t    freeze_has_memstat_state;
	boolean_t    freeze_has_pages_min;
	int        freeze_has_probability;
	int        freeze_leader_eligible;
	boolean_t    freeze_attempted;
	uint32_t    p_memstat_state;
	uint32_t    p_pages;
	int        p_freeze_error_code;
	int        p_pid;
	int        p_leader_pid;
	char        p_name[MAXCOMLEN + 1];
}proc_freezable_status_t;

#define MAX_FREEZABLE_PROCESSES 200                                                                       

  
                                                                   
                                                                     
                                                                                       
                                                                                    
  
                                                                        
                                                                              
                                
                                                                               
                                              
                                                                                  
   

#define FREEZE_PROC_LEADER_FREEZABLE_UNKNOWN   (-1)
#define FREEZE_PROC_LEADER_FREEZABLE_SUCCESS    (1)
#define FREEZE_PROC_LEADER_FREEZABLE_FAILURE    (2)

static int
memorystatus_freezer_get_status(user_addr_t buffer, size_t buffer_size, int32_t *retval)
{
	uint32_t            proc_count = 0, freeze_eligible_proc_considered = 0, band = 0, xpc_index = 0, leader_index = 0;
	global_freezable_status_t    *list_head;
	proc_freezable_status_t     *list_entry, *list_entry_start;
	size_t                list_size = 0, entry_count = 0;
	proc_t                p, leader_proc;
	memstat_bucket_t        *bucket;
	uint32_t            state = 0, pages = 0;
	boolean_t            try_freeze = TRUE, xpc_skip_size_probability_check = FALSE;
	int                error = 0, probability_of_use = 0;
	pid_t              leader_pid = 0;


	if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE == FALSE) {
		return ENOTSUP;
	}

	list_size = sizeof(global_freezable_status_t) + (sizeof(proc_freezable_status_t) * MAX_FREEZABLE_PROCESSES);

	if (buffer_size < list_size) {
		return EINVAL;
	}

	list_head = kheap_alloc(KHEAP_TEMP, list_size, Z_WAITOK | Z_ZERO);
	if (list_head == NULL) {
		return ENOMEM;
	}

	list_size = sizeof(global_freezable_status_t);

	proc_list_lock();

	uint64_t curr_time = mach_absolute_time();

	list_head->freeze_pages_threshold_crossed = (memorystatus_available_pages < memorystatus_freeze_threshold);
	list_head->freeze_eligible_procs_available = ((memorystatus_suspended_count - memorystatus_frozen_count) > memorystatus_freeze_suspended_threshold);
	list_head->freeze_scheduled_in_future = (curr_time < memorystatus_freezer_thread_next_run_ts);

	list_entry_start = (proc_freezable_status_t*) ((uintptr_t)list_head + sizeof(global_freezable_status_t));
	list_entry = list_entry_start;

	bucket = &memstat_bucket[JETSAM_PRIORITY_IDLE];

	entry_count = (memorystatus_global_probabilities_size / sizeof(memorystatus_internal_probabilities_t));

	p = memorystatus_get_first_proc_locked(&band, FALSE);
	proc_count++;

	while ((proc_count <= MAX_FREEZABLE_PROCESSES) &&
	    (p) &&
	    (list_size < buffer_size)) {
		if (isSysProc(p)) {
			  
                                                
                                                                                        
                                                 
     
                                                    
      

			coalition_t     coal = COALITION_NULL;
			task_t          leader_task = NULL, curr_task = NULL;
			int             task_role_in_coalition = 0;

			curr_task = proc_task(p);
			coal = task_get_coalition(curr_task, COALITION_TYPE_JETSAM);

			if (coal == COALITION_NULL || coalition_is_leader(curr_task, coal)) {
				  
                                              
                                                    
                 
       
				goto skip_ineligible_xpc;
			}

			leader_task = coalition_get_leader(coal);
			if (leader_task == TASK_NULL) {
				  
                                                      
                                                          
                           
       
				goto skip_ineligible_xpc;
			}

			leader_proc = (proc_t)get_bsdtask_info(leader_task);
			task_deallocate(leader_task);

			if (leader_proc == PROC_NULL) {
				                            
				goto skip_ineligible_xpc;
			}

			task_role_in_coalition = i_coal_jetsam_get_taskrole(coal, curr_task);

			if (task_role_in_coalition == COALITION_TASKROLE_XPC) {
				xpc_skip_size_probability_check = TRUE;
				leader_pid = leader_proc->p_pid;
				goto continue_eval;
			}

skip_ineligible_xpc:
			p = memorystatus_get_next_proc_locked(&band, p, FALSE);
			proc_count++;
			continue;
		}

continue_eval:
		strlcpy(list_entry->p_name, p->p_name, MAXCOMLEN + 1);

		list_entry->p_pid = p->p_pid;

		state = p->p_memstat_state;

		if ((state & (P_MEMSTAT_TERMINATED | P_MEMSTAT_LOCKED | P_MEMSTAT_FREEZE_DISABLED | P_MEMSTAT_FREEZE_IGNORE)) ||
		    !(state & P_MEMSTAT_SUSPENDED)) {
			try_freeze = list_entry->freeze_has_memstat_state = FALSE;
		} else {
			try_freeze = list_entry->freeze_has_memstat_state = TRUE;
		}

		list_entry->p_memstat_state = state;

		if (xpc_skip_size_probability_check == TRUE) {
			  
                                                
                                                     
                                                                    
                                                                 
                                             
      
			list_entry->freeze_has_pages_min = TRUE;
			list_entry->p_pages = -1;
			list_entry->freeze_has_probability = -1;

			list_entry->freeze_leader_eligible = FREEZE_PROC_LEADER_FREEZABLE_UNKNOWN;
			list_entry->p_leader_pid = leader_pid;

			xpc_skip_size_probability_check = FALSE;
		} else {
			list_entry->freeze_leader_eligible = FREEZE_PROC_LEADER_FREEZABLE_SUCCESS;                                                      
			list_entry->p_leader_pid = 0;                                                                        

			memorystatus_get_task_page_counts(p->task, &pages, NULL, NULL);
			if (pages < memorystatus_freeze_pages_min) {
				try_freeze = list_entry->freeze_has_pages_min = FALSE;
			} else {
				list_entry->freeze_has_pages_min = TRUE;
			}

			list_entry->p_pages = pages;

			if (entry_count) {
				uint32_t j = 0;
				for (j = 0; j < entry_count; j++) {
					if (strncmp(memorystatus_global_probabilities_table[j].proc_name,
					    p->p_name,
					    MAXCOMLEN) == 0) {
						probability_of_use = memorystatus_global_probabilities_table[j].use_probability;
						break;
					}
				}

				list_entry->freeze_has_probability = probability_of_use;

				try_freeze = ((probability_of_use > 0) && try_freeze);
			} else {
				list_entry->freeze_has_probability = -1;
			}
		}

		if (try_freeze) {
			uint32_t purgeable, wired, clean, dirty, shared;
			uint32_t max_pages = 0;
			int freezer_error_code = 0;

			error = task_freeze(p->task, &purgeable, &wired, &clean, &dirty, max_pages, &shared, &freezer_error_code, TRUE                );

			if (error) {
				list_entry->p_freeze_error_code = freezer_error_code;
			}

			list_entry->freeze_attempted = TRUE;
		}

		list_entry++;
		freeze_eligible_proc_considered++;

		list_size += sizeof(proc_freezable_status_t);

		p = memorystatus_get_next_proc_locked(&band, p, FALSE);
		proc_count++;
	}

	proc_list_unlock();

	list_entry = list_entry_start;

	for (xpc_index = 0; xpc_index < freeze_eligible_proc_considered; xpc_index++) {
		if (list_entry[xpc_index].freeze_leader_eligible == FREEZE_PROC_LEADER_FREEZABLE_UNKNOWN) {
			leader_pid = list_entry[xpc_index].p_leader_pid;

			leader_proc = proc_find(leader_pid);

			if (leader_proc) {
				if (leader_proc->p_memstat_state & P_MEMSTAT_FROZEN) {
					  
                                       
        
					list_entry[xpc_index].freeze_leader_eligible = FREEZE_PROC_LEADER_FREEZABLE_SUCCESS;
					proc_rele(leader_proc);
					continue;
				}
				proc_rele(leader_proc);
			}

			for (leader_index = 0; leader_index < freeze_eligible_proc_considered; leader_index++) {
				if (list_entry[leader_index].p_pid == leader_pid) {
					if (list_entry[leader_index].freeze_attempted && list_entry[leader_index].p_freeze_error_code == 0) {
						list_entry[xpc_index].freeze_leader_eligible = FREEZE_PROC_LEADER_FREEZABLE_SUCCESS;
					} else {
						list_entry[xpc_index].freeze_leader_eligible = FREEZE_PROC_LEADER_FREEZABLE_FAILURE;
						list_entry[xpc_index].p_freeze_error_code = FREEZER_ERROR_GENERIC;
					}
					break;
				}
			}

			  
                                                                
                                              
      
			if (leader_index == freeze_eligible_proc_considered) {
				list_entry[xpc_index].freeze_leader_eligible = FREEZE_PROC_LEADER_FREEZABLE_FAILURE;
				list_entry[xpc_index].p_freeze_error_code = FREEZER_ERROR_GENERIC;
			}
		}
	}

	buffer_size = MIN(list_size, INT32_MAX);

	error = copyout(list_head, buffer, buffer_size);
	if (error == 0) {
		*retval = (int32_t) buffer_size;
	} else {
		*retval = 0;
	}

	list_size = sizeof(global_freezable_status_t) + (sizeof(proc_freezable_status_t) * MAX_FREEZABLE_PROCESSES);
	kheap_free(KHEAP_TEMP, list_head, list_size);

	MEMORYSTATUS_DEBUG(1, "memorystatus_freezer_get_status: returning %d (%lu - size)\n", error, (unsigned long)*list_size);

	return error;
}

#endif                           

  
                                                                              
                                                           
   
static int
memorystatus_freezer_get_procs(user_addr_t buffer, size_t buffer_size, int32_t *retval)
{
	global_frozen_procs_t *frozen_procs = NULL;
	uint32_t band = memorystatus_freeze_jetsam_band;
	proc_t p;
	uint32_t state;
	int error;
	if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE == FALSE) {
		return ENOTSUP;
	}
	if (buffer_size < sizeof(global_frozen_procs_t)) {
		return EINVAL;
	}
	frozen_procs = kheap_alloc(KHEAP_TEMP, sizeof(global_frozen_procs_t),
	    Z_WAITOK | Z_ZERO);
	if (frozen_procs == NULL) {
		return ENOMEM;
	}

	proc_list_lock();
	p = memorystatus_get_first_proc_locked(&band, FALSE);
	while (p && frozen_procs->gfp_num_frozen < FREEZER_CONTROL_GET_PROCS_MAX_COUNT) {
		state = p->p_memstat_state;
		if (state & P_MEMSTAT_FROZEN) {
			frozen_procs->gfp_procs[frozen_procs->gfp_num_frozen].fp_pid = p->p_pid;
			strlcpy(frozen_procs->gfp_procs[frozen_procs->gfp_num_frozen].fp_name,
			    p->p_name, sizeof(proc_name_t));
			frozen_procs->gfp_num_frozen++;
		}
		p = memorystatus_get_next_proc_locked(&band, p, FALSE);
	}
	proc_list_unlock();

	buffer_size = MIN(buffer_size, sizeof(global_frozen_procs_t));
	error = copyout(frozen_procs, buffer, buffer_size);
	if (error == 0) {
		*retval = (int32_t) buffer_size;
	} else {
		*retval = 0;
	}
	kheap_free(KHEAP_TEMP, frozen_procs, sizeof(global_frozen_procs_t));

	return error;
}

int
memorystatus_freezer_control(int32_t flags, user_addr_t buffer, size_t buffer_size, int32_t *retval)
{
	int err = ENOTSUP;

#if DEVELOPMENT || DEBUG
	if (flags == FREEZER_CONTROL_GET_STATUS) {
		err = memorystatus_freezer_get_status(buffer, buffer_size, retval);
	}
#endif                           
	if (flags == FREEZER_CONTROL_GET_PROCS) {
		err = memorystatus_freezer_get_procs(buffer, buffer_size, retval);
	}

	return err;
}

extern void        vm_swap_consider_defragmenting(int);
extern boolean_t memorystatus_kill_elevated_process(uint32_t, os_reason_t, unsigned int, int, uint32_t *, uint64_t *);

  
                                                  
                                          
  
                                                          
   

void
memorystatus_disable_freeze(void)
{
	memstat_bucket_t *bucket;
	int bucket_count = 0, retries = 0;
	boolean_t retval = FALSE, killed = FALSE;
	uint32_t errors = 0, errors_over_prev_iteration = 0;
	os_reason_t jetsam_reason = 0;
	unsigned int band = 0;
	proc_t p = PROC_NULL, next_p = PROC_NULL;
	uint64_t memory_reclaimed = 0, footprint = 0;

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_FREEZE_DISABLE) | DBG_FUNC_START,
	    memorystatus_available_pages, 0, 0, 0, 0);

	assert(memorystatus_freeze_enabled == FALSE);

	jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_MEMORY_DISK_SPACE_SHORTAGE);
	if (jetsam_reason == OS_REASON_NULL) {
		printf("memorystatus_disable_freeze: failed to allocate jetsam reason\n");
	}

	  
                                                                             
                                                                              
                                                                            
                                               
   
                                                                             
                                       
                                                                                           
                                        
                                                                                         
                                                                        
   
                                                                                           
                                              
    

again:
	proc_list_lock();

	band = JETSAM_PRIORITY_IDLE;
	p = PROC_NULL;
	next_p = PROC_NULL;

	next_p = memorystatus_get_first_proc_locked(&band, TRUE);
	while (next_p) {
		p = next_p;
		next_p = memorystatus_get_next_proc_locked(&band, p, TRUE);

		if (p->p_memstat_effectivepriority > JETSAM_PRIORITY_FOREGROUND) {
			break;
		}

		if ((p->p_memstat_state & P_MEMSTAT_FROZEN) == FALSE) {
			continue;
		}

		if (p->p_memstat_state & P_MEMSTAT_ERROR) {
			p->p_memstat_state &= ~P_MEMSTAT_ERROR;
		}

		if (p->p_memstat_effectivepriority == memorystatus_freeze_jetsam_band) {
			continue;
		}

		  
                                                                        
                                                                                   
                                                       
                                                                           
     
		p->p_memstat_state |= P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND;
		memorystatus_invalidate_idle_demotion_locked(p, TRUE);

		memorystatus_update_priority_locked(p, memorystatus_freeze_jetsam_band, FALSE, TRUE);
	}

	bucket = &memstat_bucket[memorystatus_freeze_jetsam_band];
	bucket_count = bucket->count;
	proc_list_unlock();

	  
                                                                     
                                                                                 
                                                                                   
                                                            
    

	errors_over_prev_iteration = 0;
	while (bucket_count) {
		bucket_count--;

		  
                                                            
                                                                   
                           
     

		os_reason_ref(jetsam_reason);
		retval = memorystatus_kill_elevated_process(
			kMemorystatusKilledDiskSpaceShortage,
			jetsam_reason,
			memorystatus_freeze_jetsam_band,
			0,                                                                                   
			&errors,
			&footprint);

		if (errors > 0) {
			printf("memorystatus_disable_freeze: memorystatus_kill_elevated_process returned %d error(s)\n", errors);
			errors_over_prev_iteration += errors;
			errors = 0;
		}

		if (retval == 0) {
			  
                                       
      
			break;
		}

		killed = TRUE;
		memory_reclaimed += footprint;
	}

	proc_list_lock();

	if (memorystatus_frozen_count) {
		  
                                     
                                    
                                      
                                    
                                     
     
		assert(memorystatus_freeze_enabled == FALSE);

		retries++;
		if (retries < 3) {
			proc_list_unlock();
			goto again;
		}
#if DEVELOPMENT || DEBUG
		panic("memorystatus_disable_freeze: Failed to kill all frozen processes, memorystatus_frozen_count = %d, errors = %d",
		    memorystatus_frozen_count, errors_over_prev_iteration);
#endif                           
	}
	proc_list_unlock();

	os_reason_free(jetsam_reason);

	if (killed) {
		vm_swap_consider_defragmenting(VM_SWAP_FLAGS_FORCE_DEFRAG | VM_SWAP_FLAGS_FORCE_RECLAIM);

		proc_list_lock();
		size_t snapshot_size = sizeof(memorystatus_jetsam_snapshot_t) +
		    sizeof(memorystatus_jetsam_snapshot_entry_t) * (memorystatus_jetsam_snapshot_count);
		uint64_t timestamp_now = mach_absolute_time();
		memorystatus_jetsam_snapshot->notification_time = timestamp_now;
		memorystatus_jetsam_snapshot->js_gencount++;
		if (memorystatus_jetsam_snapshot_count > 0 && (memorystatus_jetsam_snapshot_last_timestamp == 0 ||
		    timestamp_now > memorystatus_jetsam_snapshot_last_timestamp + memorystatus_jetsam_snapshot_timeout)) {
			proc_list_unlock();
			int ret = memorystatus_send_note(kMemorystatusSnapshotNote, &snapshot_size, sizeof(snapshot_size));
			if (!ret) {
				proc_list_lock();
				memorystatus_jetsam_snapshot_last_timestamp = timestamp_now;
				proc_list_unlock();
			}
		} else {
			proc_list_unlock();
		}
	}

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_FREEZE_DISABLE) | DBG_FUNC_END,
	    memorystatus_available_pages, memory_reclaimed, 0, 0, 0);

	return;
}

__private_extern__ void
memorystatus_freeze_init(void)
{
	kern_return_t result;
	thread_t thread;

	  
                                                    
                                                    
                                                                            
                                                
    
	memorystatus_freeze_budget_pages_remaining = (memorystatus_freeze_daily_mb_max * 1024 * 1024) / PAGE_SIZE;

	result = kernel_thread_start(memorystatus_freeze_thread, NULL, &thread);
	if (result == KERN_SUCCESS) {
		proc_set_thread_policy(thread, TASK_POLICY_INTERNAL, TASK_POLICY_IO, THROTTLE_LEVEL_COMPRESSOR_TIER2);
		proc_set_thread_policy(thread, TASK_POLICY_INTERNAL, TASK_POLICY_PASSIVE_IO, TASK_POLICY_ENABLE);
		thread_set_thread_name(thread, "VM_freezer");

		thread_deallocate(thread);
	} else {
		panic("Could not create memorystatus_freeze_thread");
	}
}

static boolean_t
memorystatus_is_process_eligible_for_freeze(proc_t p)
{
	  
                                    
    

	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_OWNED);

	boolean_t should_freeze = FALSE;
	uint32_t state = 0, pages = 0;
	int probability_of_use = 0;
	size_t entry_count = 0, i = 0;
	bool first_consideration = true;

	state = p->p_memstat_state;

	if (state & (P_MEMSTAT_TERMINATED | P_MEMSTAT_LOCKED | P_MEMSTAT_FREEZE_DISABLED | P_MEMSTAT_FREEZE_IGNORE)) {
		if (state & P_MEMSTAT_FREEZE_DISABLED) {
			p->p_memstat_freeze_skip_reason = kMemorystatusFreezeSkipReasonDisabled;
		}
		goto out;
	}

	if (isSysProc(p)) {
		  
                                         
                                                               
                                                
    
                                                   
     

		coalition_t     coal = COALITION_NULL;
		task_t          leader_task = NULL, curr_task = NULL;
		proc_t          leader_proc = NULL;
		int             task_role_in_coalition = 0;

		curr_task = proc_task(p);
		coal = task_get_coalition(curr_task, COALITION_TYPE_JETSAM);

		if (coal == NULL || coalition_is_leader(curr_task, coal)) {
			  
                                             
                                                   
                
      
			goto out;
		}

		leader_task = coalition_get_leader(coal);
		if (leader_task == TASK_NULL) {
			  
                                                     
                                                         
                          
      
			goto out;
		}

		leader_proc = (proc_t)get_bsdtask_info(leader_task);
		task_deallocate(leader_task);

		if (leader_proc == PROC_NULL) {
			                            
			goto out;
		}

		if (!(leader_proc->p_memstat_state & P_MEMSTAT_FROZEN)) {
			goto out;
		}

		task_role_in_coalition = i_coal_jetsam_get_taskrole(coal, curr_task);

		if (task_role_in_coalition == COALITION_TASKROLE_XPC) {
			should_freeze = TRUE;
		}

		goto out;
	} else {
		  
                                                                 
                                                               
     
		if (!(state & P_MEMSTAT_SUSPENDED)) {
			goto out;
		}
	}

	  
                                         
                                                         
                        
                                                        
                                                               
                
    
	first_consideration = !(state & P_MEMSTAT_FREEZE_CONSIDERED);

	if (first_consideration) {
		memorystatus_freezer_stats.mfs_process_considered_count++;
		p->p_memstat_state |= P_MEMSTAT_FREEZE_CONSIDERED;
	}

	                                                                         
	memorystatus_get_task_page_counts(p->task, &pages, NULL, NULL);
	if (pages < memorystatus_freeze_pages_min) {
		if (first_consideration) {
			memorystatus_freezer_stats.mfs_error_below_min_pages_count++;
		}
		p->p_memstat_freeze_skip_reason = kMemorystatusFreezeSkipReasonBelowMinPages;
		goto out;
	}

	                                                                                       
                                                                            
                                                                                       
                                                                                          
    
	if ((p->p_listflag & P_LIST_EXITED) != 0) {
		if (first_consideration) {
			memorystatus_freezer_stats.mfs_error_other_count++;
		}
		p->p_memstat_freeze_skip_reason = kMemorystatusFreezeSkipReasonOther;
		goto out;
	}

	entry_count = (memorystatus_global_probabilities_size / sizeof(memorystatus_internal_probabilities_t));

	if (entry_count) {
		for (i = 0; i < entry_count; i++) {
			  
                                                                              
                                                                          
                                                                          
                                            
      
			if (strncmp(memorystatus_global_probabilities_table[i].proc_name,
			    p->p_name,
			    MAXCOMLEN) == 0) {
				probability_of_use = memorystatus_global_probabilities_table[i].use_probability;
				break;
			}
		}

		if (probability_of_use == 0) {
			if (first_consideration) {
				memorystatus_freezer_stats.mfs_error_low_probability_of_use_count++;
			}
			p->p_memstat_freeze_skip_reason = kMemorystatusFreezeSkipReasonLowProbOfUse;
			goto out;
		}
	}

	should_freeze = TRUE;
out:
	if (should_freeze && !(state & P_MEMSTAT_FROZEN)) {
		  
                                                                                 
                                           
     
		p->p_memstat_freeze_skip_reason = kMemorystatusFreezeSkipReasonNone;
		if (!first_consideration) {
			  
                                                                                        
                                                                  
                    
      
			memorystatus_freezer_stats.mfs_process_considered_count++;
		}
	}
	return should_freeze;
}

  
                                                                                  
  
                     
                                                                 
                                                                             
                                 
  
                                                                       
                                                                               
                                                                                  
                             
  
                                                         
   
int
memorystatus_freeze_process_sync(proc_t p)
{
	int ret = EINVAL;
	pid_t aPid = 0;
	boolean_t memorystatus_freeze_swap_low = FALSE;
	int    freezer_error_code = 0;

	lck_mtx_lock(&freezer_mutex);

	if (p == NULL) {
		printf("memorystatus_freeze_process_sync: Invalid process\n");
		goto exit;
	}

	if (memorystatus_freeze_enabled == FALSE) {
		printf("memorystatus_freeze_process_sync: Freezing is DISABLED\n");
		goto exit;
	}

	if (!memorystatus_can_freeze(&memorystatus_freeze_swap_low)) {
		printf("memorystatus_freeze_process_sync: Low compressor and/or low swap space...skipping freeze\n");
		goto exit;
	}

	memorystatus_freeze_update_throttle(&memorystatus_freeze_budget_pages_remaining);
	if (!memorystatus_freeze_budget_pages_remaining) {
		printf("memorystatus_freeze_process_sync: exit with NO available budget\n");
		goto exit;
	}

	proc_list_lock();

	if (p != NULL) {
		uint32_t purgeable, wired, clean, dirty, shared;
		uint32_t i;
		uint64_t max_pages;

		aPid = p->p_pid;

		                                                 
		if (memorystatus_is_process_eligible_for_freeze(p) == FALSE) {
			proc_list_unlock();
			goto exit;
		}

		if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
			max_pages = MIN(memorystatus_freeze_pages_max, memorystatus_freeze_budget_pages_remaining);
		} else {
			  
                                                   
      
			max_pages = UINT32_MAX - 1;
		}

		                                              
		p->p_memstat_state |= P_MEMSTAT_LOCKED;
		proc_list_unlock();

		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_FREEZE) | DBG_FUNC_START,
		    memorystatus_available_pages, 0, 0, 0, 0);

		max_pages = MIN(max_pages, UINT32_MAX);
		ret = task_freeze(p->task, &purgeable, &wired, &clean, &dirty, (uint32_t) max_pages, &shared, &freezer_error_code, FALSE                );
		if (ret == KERN_SUCCESS || freezer_error_code == FREEZER_ERROR_LOW_PRIVATE_SHARED_RATIO) {
			memorystatus_freezer_stats.mfs_shared_pages_skipped += shared;
		}

		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_FREEZE) | DBG_FUNC_END,
		    memorystatus_available_pages, aPid, 0, 0, 0);

		DTRACE_MEMORYSTATUS6(memorystatus_freeze, proc_t, p, unsigned int, memorystatus_available_pages, boolean_t, purgeable, unsigned int, wired, uint32_t, clean, uint32_t, dirty);

		MEMORYSTATUS_DEBUG(1, "memorystatus_freeze_process_sync: task_freeze %s for pid %d [%s] - "
		    "memorystatus_pages: %d, purgeable: %d, wired: %d, clean: %d, dirty: %d, max_pages %d, shared %d\n",
		    (ret == KERN_SUCCESS) ? "SUCCEEDED" : "FAILED", aPid, (*p->p_name ? p->p_name : "(unknown)"),
		    memorystatus_available_pages, purgeable, wired, clean, dirty, max_pages, shared);

		proc_list_lock();

		if (ret == KERN_SUCCESS) {
			memorystatus_freeze_entry_t data = { aPid, TRUE, dirty };

			p->p_memstat_freeze_sharedanon_pages += shared;

			memorystatus_frozen_shared_mb += shared;

			if ((p->p_memstat_state & P_MEMSTAT_FROZEN) == 0) {
				p->p_memstat_state |= P_MEMSTAT_FROZEN;
				p->p_memstat_freeze_skip_reason = kMemorystatusFreezeSkipReasonNone;
				memorystatus_frozen_count++;
				os_atomic_inc(&memorystatus_freezer_stats.mfs_processes_frozen, relaxed);
				if (memorystatus_frozen_count == memorystatus_frozen_processes_max) {
					memorystatus_freeze_out_of_slots();
				}
			} else {
				                       
				if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
					memorystatus_freezer_stats.mfs_bytes_refrozen += dirty * PAGE_SIZE;
					memorystatus_freezer_stats.mfs_refreeze_count++;
				}
			}

			p->p_memstat_frozen_count++;

			  
                                                                                                    
                                
      
			proc_list_unlock();

			memorystatus_send_note(kMemorystatusFreezeNote, &data, sizeof(data));

			if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
				ret = memorystatus_update_inactive_jetsam_priority_band(p->p_pid, MEMORYSTATUS_CMD_ELEVATED_INACTIVEJETSAMPRIORITY_ENABLE,
				    memorystatus_freeze_jetsam_band, TRUE);

				if (ret) {
					printf("Elevating the frozen process failed with %d\n", ret);
					               
					ret = 0;
				}


				                  
				for (i = 0; i < sizeof(throttle_intervals) / sizeof(struct throttle_interval_t); i++) {
					throttle_intervals[i].pageouts += dirty;
				}
			}
			memorystatus_freeze_update_throttle(&memorystatus_freeze_budget_pages_remaining);
			os_log_with_startup_serial(OS_LOG_DEFAULT, "memorystatus: freezing (specific) pid %d [%s] done memorystatus_freeze_budget_pages_remaining %llu froze %u pages",
			    aPid, ((p && *p->p_name) ? p->p_name : "unknown"), memorystatus_freeze_budget_pages_remaining, dirty);

			proc_list_lock();

			memorystatus_freeze_pageouts += dirty;

			if (memorystatus_frozen_count == (memorystatus_frozen_processes_max - 1)) {
				  
                                                            
                                                             
                                                                 
       
			}
		} else {
			memorystatus_freeze_handle_error(p, freezer_error_code, p->p_memstat_state & P_MEMSTAT_FROZEN, aPid, NULL, "memorystatus_freeze_process_sync");
			p->p_memstat_state |= P_MEMSTAT_FREEZE_IGNORE;
		}

		p->p_memstat_state &= ~P_MEMSTAT_LOCKED;
		wakeup(&p->p_memstat_state);
		proc_list_unlock();
	}

exit:
	lck_mtx_unlock(&freezer_mutex);

	return ret;
}

  
                                                                      
   
static int
memorystatus_freeze_top_process(void)
{
	pid_t aPid = 0, coal_xpc_pid = 0;
	int ret = -1;
	proc_t p = PROC_NULL, next_p = PROC_NULL;
	unsigned int i = 0;
	unsigned int band = JETSAM_PRIORITY_IDLE;
	bool refreeze_processes = false;
	task_t curr_task = NULL;
	coalition_t coal = COALITION_NULL;
	pid_t pid_list[MAX_XPC_SERVICE_PIDS];
	unsigned int    ntasks = 0;
	LCK_MTX_ASSERT(&freezer_mutex, LCK_MTX_ASSERT_OWNED);

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_FREEZE_SCAN) | DBG_FUNC_START, memorystatus_available_pages, 0, 0, 0, 0);

	proc_list_lock();

	if (memorystatus_frozen_count >= memorystatus_frozen_processes_max) {
		  
                                                         
                                                       
                                                         
     
		refreeze_processes = true;
		band = (unsigned int) memorystatus_freeze_jetsam_band;
	}

freeze_process:

	next_p = memorystatus_get_first_proc_locked(&band, FALSE);
	while (next_p) {
		kern_return_t kr;
		uint32_t purgeable, wired, clean, dirty, shared;
		uint64_t max_pages = 0;
		int    freezer_error_code = 0;
		bool was_refreeze = false;

		p = next_p;

		if (coal == NULL) {
			next_p = memorystatus_get_next_proc_locked(&band, p, FALSE);
		} else {
			  
                                                   
                                               
                                                   
                                                  
                              
      

			if (ntasks > 0) {
				coal_xpc_pid = pid_list[--ntasks];
				next_p = proc_findinternal(coal_xpc_pid, 1                          );
				  
                                                                            
                                                             
                                                                     
       
				if (next_p) {
					proc_rele_locked(next_p);
				}
			} else {
				next_p = NULL;
			}
		}

		aPid = p->p_pid;

		if (p->p_memstat_effectivepriority != (int32_t) band) {
			  
                                                    
                      
      
			break;
		}

		                                                      
		if (refreeze_processes) {
			  
                                          
      
			if ((p->p_memstat_state & P_MEMSTAT_FROZEN) == FALSE) {
				continue;
			}

			  
                                           
      
			if ((p->p_memstat_state & P_MEMSTAT_REFREEZE_ELIGIBLE) == FALSE) {
				continue;
			}

			  
                                                  
      
			if (p->p_memstat_state & P_MEMSTAT_LOCKED) {
				continue;
			}

			  
                                                         
                                                           
                                              
      
			memorystatus_frozen_shared_mb -= p->p_memstat_freeze_sharedanon_pages;
			p->p_memstat_freeze_sharedanon_pages = 0;

			p->p_memstat_state &= ~P_MEMSTAT_REFREEZE_ELIGIBLE;
			memorystatus_refreeze_eligible_count--;
		} else {
			if (memorystatus_is_process_eligible_for_freeze(p) == FALSE) {
				continue;                  
			}
		}

		if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
			  
                                                       
                                
      

			max_pages = MIN(memorystatus_freeze_pages_max, memorystatus_freeze_budget_pages_remaining);
		} else {
			  
                                       
      
			max_pages = UINT32_MAX - 1;
		}

		                                              
		p->p_memstat_state |= P_MEMSTAT_LOCKED;

		p = proc_ref_locked(p);
		if (!p) {
			memorystatus_freezer_stats.mfs_error_other_count++;
			break;
		}

		proc_list_unlock();

		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_FREEZE) | DBG_FUNC_START,
		    memorystatus_available_pages, 0, 0, 0, 0);

		max_pages = MIN(max_pages, UINT32_MAX);
		kr = task_freeze(p->task, &purgeable, &wired, &clean, &dirty, (uint32_t) max_pages, &shared, &freezer_error_code, FALSE                );
		if (kr == KERN_SUCCESS || freezer_error_code == FREEZER_ERROR_LOW_PRIVATE_SHARED_RATIO) {
			memorystatus_freezer_stats.mfs_shared_pages_skipped += shared;
		}

		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_FREEZE) | DBG_FUNC_END,
		    memorystatus_available_pages, aPid, 0, 0, 0);

		MEMORYSTATUS_DEBUG(1, "memorystatus_freeze_top_process: task_freeze %s for pid %d [%s] - "
		    "memorystatus_pages: %d, purgeable: %d, wired: %d, clean: %d, dirty: %d, max_pages %d, shared %d\n",
		    (kr == KERN_SUCCESS) ? "SUCCEEDED" : "FAILED", aPid, (*p->p_name ? p->p_name : "(unknown)"),
		    memorystatus_available_pages, purgeable, wired, clean, dirty, max_pages, shared);

		proc_list_lock();

		              
		if (KERN_SUCCESS == kr) {
			memorystatus_freeze_entry_t data = { aPid, TRUE, dirty };

			p->p_memstat_freeze_sharedanon_pages += shared;

			memorystatus_frozen_shared_mb += shared;

			if ((p->p_memstat_state & P_MEMSTAT_FROZEN) == 0) {
				p->p_memstat_state |= P_MEMSTAT_FROZEN;
				p->p_memstat_freeze_skip_reason = kMemorystatusFreezeSkipReasonNone;
				memorystatus_frozen_count++;
				os_atomic_inc(&memorystatus_freezer_stats.mfs_processes_frozen, relaxed);
				if (memorystatus_frozen_count == memorystatus_frozen_processes_max) {
					memorystatus_freeze_out_of_slots();
				}
			} else {
				                       
				if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
					memorystatus_freezer_stats.mfs_bytes_refrozen += dirty * PAGE_SIZE;
					memorystatus_freezer_stats.mfs_refreeze_count++;
				}
				was_refreeze = true;
			}

			p->p_memstat_frozen_count++;

			  
                                                                                                    
                                
      
			proc_list_unlock();

			memorystatus_send_note(kMemorystatusFreezeNote, &data, sizeof(data));

			if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
				ret = memorystatus_update_inactive_jetsam_priority_band(p->p_pid, MEMORYSTATUS_CMD_ELEVATED_INACTIVEJETSAMPRIORITY_ENABLE, memorystatus_freeze_jetsam_band, TRUE);

				if (ret) {
					printf("Elevating the frozen process failed with %d\n", ret);
					               
					ret = 0;
				}

				                  
				for (i = 0; i < sizeof(throttle_intervals) / sizeof(struct throttle_interval_t); i++) {
					throttle_intervals[i].pageouts += dirty;
				}
			}
			memorystatus_freeze_update_throttle(&memorystatus_freeze_budget_pages_remaining);
			os_log_with_startup_serial(OS_LOG_DEFAULT, "memorystatus: %sfreezing (%s) pid %d [%s] done, memorystatus_freeze_budget_pages_remaining %llu %sfroze %u pages\n",
			    was_refreeze ? "re" : "", (coal == NULL ? "general" : "coalition-driven"), aPid, ((p && *p->p_name) ? p->p_name : "unknown"), memorystatus_freeze_budget_pages_remaining, was_refreeze ? "Re" : "", dirty);

			proc_list_lock();

			memorystatus_freeze_pageouts += dirty;

			if (memorystatus_frozen_count == (memorystatus_frozen_processes_max - 1)) {
				  
                                                            
                                                             
                                                                 
       
			}

			                         
			ret = kr;

			  
                                                      
                                                       
                     
     
           
                                                       
                               
                                                      
                                                              
      

			if (coal == NULL) {
				curr_task = proc_task(p);
				coal = task_get_coalition(curr_task, COALITION_TYPE_JETSAM);
				if (coalition_is_leader(curr_task, coal)) {
					ntasks = coalition_get_pid_list(coal, COALITION_ROLEMASK_XPC,
					    COALITION_SORT_DEFAULT, pid_list, MAX_XPC_SERVICE_PIDS);

					if (ntasks > MAX_XPC_SERVICE_PIDS) {
						ntasks = MAX_XPC_SERVICE_PIDS;
					}
				}

				next_p = NULL;

				if (ntasks > 0) {
					  
                                                     
        
					coal_xpc_pid = pid_list[--ntasks];
					next_p = proc_findinternal(coal_xpc_pid, 1                          );

					  
                                                                            
                                                              
                                                                      
        
					if (next_p) {
						proc_rele_locked(next_p);
					}
				}
			}

			p->p_memstat_state &= ~P_MEMSTAT_LOCKED;
			wakeup(&p->p_memstat_state);
			proc_rele_locked(p);

			if (coal && next_p) {
				continue;
			}

			  
                                                 
                                        
     
        
     
                                                  
                                   
     
                                                       
             
      

			break;
		} else {
			p->p_memstat_state &= ~P_MEMSTAT_LOCKED;
			wakeup(&p->p_memstat_state);

			if (refreeze_processes) {
				if ((freezer_error_code == FREEZER_ERROR_EXCESS_SHARED_MEMORY) ||
				    (freezer_error_code == FREEZER_ERROR_LOW_PRIVATE_SHARED_RATIO)) {
					  
                                                                
                                                                
                                                          
                                                                 
                                                             
            
        

					p->p_memstat_state &= ~P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND;
					memorystatus_invalidate_idle_demotion_locked(p, TRUE);
					memorystatus_update_priority_locked(p, JETSAM_PRIORITY_IDLE, TRUE, TRUE);
				}
			} else {
				p->p_memstat_state |= P_MEMSTAT_FREEZE_IGNORE;
			}
			memorystatus_freeze_handle_error(p, freezer_error_code, p->p_memstat_state & P_MEMSTAT_FROZEN, aPid, coal, "memorystatus_freeze_top_process");

			proc_rele_locked(p);

			if (vm_compressor_low_on_space() || vm_swap_low_on_space()) {
				break;
			}
		}
	}

	if ((ret == -1) &&
	    (memorystatus_refreeze_eligible_count >= MIN_THAW_REFREEZE_THRESHOLD) &&
	    (!refreeze_processes)) {
		  
                                                
                                            
                                         
                                             
                                                   
                                            
     

		band = (unsigned int) memorystatus_freeze_jetsam_band;

		refreeze_processes = true;

		goto freeze_process;
	}

	proc_list_unlock();

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_FREEZE_SCAN) | DBG_FUNC_END, memorystatus_available_pages, aPid, 0, 0, 0);

	return ret;
}

#if DEVELOPMENT || DEBUG
                                                 
static int
sysctl_memorystatus_freeze_top_process SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error, val;
	  
                                                                
                                            
    
	error = sysctl_handle_int(oidp, &val, 0, req);
	if (error || !req->newptr) {
		return error;
	}
	lck_mtx_lock(&freezer_mutex);
	int ret = memorystatus_freeze_top_process();
	lck_mtx_unlock(&freezer_mutex);
	if (ret == -1) {
		ret = ESRCH;
	}
	return ret;
}
SYSCTL_PROC(_vm, OID_AUTO, memorystatus_freeze_top_process, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_MASKED,
    0, 0, &sysctl_memorystatus_freeze_top_process, "I", "");
#endif                           

static inline boolean_t
memorystatus_can_freeze_processes(void)
{
	boolean_t ret;

	proc_list_lock();

	if (memorystatus_suspended_count) {
		memorystatus_freeze_suspended_threshold = MIN(memorystatus_freeze_suspended_threshold, FREEZE_SUSPENDED_THRESHOLD_DEFAULT);

		if ((memorystatus_suspended_count - memorystatus_frozen_count) > memorystatus_freeze_suspended_threshold) {
			ret = TRUE;
		} else {
			ret = FALSE;
		}
	} else {
		ret = FALSE;
	}

	proc_list_unlock();

	return ret;
}

static boolean_t
memorystatus_can_freeze(boolean_t *memorystatus_freeze_swap_low)
{
	boolean_t can_freeze = TRUE;

	                                                                               
                                                                                
	if (memorystatus_available_pages > memorystatus_freeze_threshold) {
		return FALSE;
	}

	                                                
	if (!memorystatus_can_freeze_processes()) {
		return FALSE;
	}
	assert(VM_CONFIG_COMPRESSOR_IS_PRESENT);

	if (!VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
		  
                                                                       
     
		if (vm_compressor_low_on_space()) {
			if (*memorystatus_freeze_swap_low) {
				*memorystatus_freeze_swap_low = TRUE;
			}

			can_freeze = FALSE;
		} else {
			if (*memorystatus_freeze_swap_low) {
				*memorystatus_freeze_swap_low = FALSE;
			}

			can_freeze = TRUE;
		}
	} else {
		  
                                        
    
                                        
     
		if (vm_swap_low_on_space()) {
			if (*memorystatus_freeze_swap_low) {
				*memorystatus_freeze_swap_low = TRUE;
			}

			can_freeze = FALSE;
		}
	}

	return can_freeze;
}

  
                                                                    
                                                        
                                       
                                                                   
                                                                         
                                                                             
                                                                         
           
  
                                        
                                                                      
                                               
                                                                     
                                                                   
                                   
   
static void
memorystatus_demote_frozen_processes(boolean_t force_one)
{
	unsigned int band = (unsigned int) memorystatus_freeze_jetsam_band;
	unsigned int demoted_proc_count = 0;
	proc_t p = PROC_NULL, next_p = PROC_NULL;
	                                                                                      
	int maxpriority = JETSAM_PRIORITY_IDLE;

	proc_list_lock();

	if (memorystatus_freeze_enabled == FALSE) {
		  
                                       
                                        
                                       
     
		proc_list_unlock();
		return;
	}

	next_p = memorystatus_get_first_proc_locked(&band, FALSE);
	while (next_p) {
		p = next_p;
		next_p = memorystatus_get_next_proc_locked(&band, p, FALSE);

		if ((p->p_memstat_state & P_MEMSTAT_FROZEN) == FALSE) {
			continue;
		}

		if (p->p_memstat_state & P_MEMSTAT_LOCKED) {
			continue;
		}

		if (force_one == TRUE) {
			if ((p->p_memstat_state & P_MEMSTAT_REFREEZE_ELIGIBLE) == 0) {
				  
                                                              
                                                                
                                       
       
				continue;
			}
		} else {
			if (p->p_memstat_thaw_count >= memorystatus_thaw_count_demotion_threshold) {
				  
                                                                        
                                                 
       
				continue;
			}
		}

		p->p_memstat_state &= ~P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND;
		memorystatus_invalidate_idle_demotion_locked(p, TRUE);

		maxpriority = MAX(p->p_memstat_assertionpriority, maxpriority);
		memorystatus_update_priority_locked(p, maxpriority, FALSE, FALSE);
#if DEVELOPMENT || DEBUG
		os_log_with_startup_serial(OS_LOG_DEFAULT, "memorystatus_demote_frozen_process(%s) pid %d [%s]",
		    (force_one ? "urgent" : "normal"), (p ? p->p_pid : -1), ((p && *p->p_name) ? p->p_name : "unknown"));
#endif                           

		  
                                                                    
                                                         
                                                                     
                                                              
    
                                                                  
                                                                   
                                                                
                          
     
		if (p->p_memstat_state & P_MEMSTAT_REFREEZE_ELIGIBLE) {
			p->p_memstat_state &= ~P_MEMSTAT_REFREEZE_ELIGIBLE;
			memorystatus_refreeze_eligible_count--;
		}

		demoted_proc_count++;

		if ((force_one == TRUE) || (demoted_proc_count == memorystatus_max_frozen_demotions_daily)) {
			break;
		}
	}

	if (force_one == FALSE) {
		  
                                                    
                                                
          
     
		memorystatus_thaw_count = 0;
	}

	proc_list_unlock();
}

  
                                  
                                                                                                                     
                                                              
                                                                          
                                                                   
  
                                         
   
static uint32_t
memorystatus_freeze_calculate_new_budget(
	unsigned int time_since_last_interval_expired_sec,
	unsigned int burst_multiple,
	unsigned int interval_duration_min,
	uint32_t rollover)
{
	uint64_t freeze_daily_budget = 0, freeze_daily_budget_mb = 0, daily_budget_pageouts = 0, budget_missed = 0, freeze_daily_pageouts_max = 0, new_budget = 0;
	const static unsigned int kNumSecondsInDay = 60 * 60 * 24;
	                                                         
	const static unsigned int kFixedPointFactor = 100;
	unsigned int days_missed;

	                                                 
	if (vm_swap_max_budget(&freeze_daily_budget)) {
		freeze_daily_budget_mb = freeze_daily_budget / (1024 * 1024);
		assert(freeze_daily_budget_mb <= UINT32_MAX);
		memorystatus_freeze_daily_mb_max = (unsigned int) freeze_daily_budget_mb;
		os_log_with_startup_serial(OS_LOG_DEFAULT, "memorystatus: memorystatus_freeze_daily_mb_max set to %dMB\n", memorystatus_freeze_daily_mb_max);
	}
	                                        
	freeze_daily_pageouts_max = memorystatus_freeze_daily_mb_max * (1024 * 1024 / PAGE_SIZE);

	daily_budget_pageouts = (burst_multiple * (((uint64_t) interval_duration_min * freeze_daily_pageouts_max) / (kNumSecondsInDay / 60)));

	  
                                                              
                                                                                       
                                                                 
    
	days_missed = time_since_last_interval_expired_sec * kFixedPointFactor / kNumSecondsInDay;
	budget_missed = days_missed * freeze_daily_pageouts_max / kFixedPointFactor;
	new_budget = rollover + daily_budget_pageouts + budget_missed;
	return (uint32_t) MIN(new_budget, UINT32_MAX);
}

  
                                                                                   
                                                                                                       
                                                                                                      
                                 
                                                                                                                               
   
static void
memorystatus_freeze_mark_eligible_processes_with_skip_reason(memorystatus_freeze_skip_reason_t reason, bool locked)
{
	LCK_MTX_ASSERT(&freezer_mutex, LCK_MTX_ASSERT_OWNED);
	LCK_MTX_ASSERT(&proc_list_mlock, locked ? LCK_MTX_ASSERT_OWNED : LCK_MTX_ASSERT_NOTOWNED);
	unsigned int band = JETSAM_PRIORITY_IDLE;
	proc_t p;

	if (!locked) {
		proc_list_lock();
	}
	p = memorystatus_get_first_proc_locked(&band, FALSE);
	while (p) {
		assert(p->p_memstat_effectivepriority == (int32_t) band);
		if (!(p->p_memstat_state & P_MEMSTAT_FROZEN) && memorystatus_is_process_eligible_for_freeze(p)) {
			assert(p->p_memstat_freeze_skip_reason == kMemorystatusFreezeSkipReasonNone);
			p->p_memstat_freeze_skip_reason = (uint8_t) reason;
		}
		p = memorystatus_get_next_proc_locked(&band, p, FALSE);
	}
	if (!locked) {
		proc_list_unlock();
	}
}

  
                                            
                                                                                          
   
static void
memorystatus_freeze_handle_error(
	proc_t p,
	const int freezer_error_code,
	bool was_refreeze,
	pid_t pid,
	const coalition_t coalition,
	const char* log_prefix)
{
	const char *reason;
	memorystatus_freeze_skip_reason_t skip_reason;

	switch (freezer_error_code) {
	case FREEZER_ERROR_EXCESS_SHARED_MEMORY:
		memorystatus_freezer_stats.mfs_error_excess_shared_memory_count++;
		reason = "too much shared memory";
		skip_reason = kMemorystatusFreezeSkipReasonExcessSharedMemory;
		break;
	case FREEZER_ERROR_LOW_PRIVATE_SHARED_RATIO:
		memorystatus_freezer_stats.mfs_error_low_private_shared_ratio_count++;
		reason = "private-shared pages ratio";
		skip_reason = kMemorystatusFreezeSkipReasonLowPrivateSharedRatio;
		break;
	case FREEZER_ERROR_NO_COMPRESSOR_SPACE:
		memorystatus_freezer_stats.mfs_error_no_compressor_space_count++;
		reason = "no compressor space";
		skip_reason = kMemorystatusFreezeSkipReasonNoCompressorSpace;
		break;
	case FREEZER_ERROR_NO_SWAP_SPACE:
		memorystatus_freezer_stats.mfs_error_no_swap_space_count++;
		reason = "no swap space";
		skip_reason = kMemorystatusFreezeSkipReasonNoSwapSpace;
		break;
	default:
		reason = "unknown error";
		skip_reason = kMemorystatusFreezeSkipReasonOther;
	}

	p->p_memstat_freeze_skip_reason = (uint8_t) skip_reason;

	os_log_with_startup_serial(OS_LOG_DEFAULT, "%s: %sfreezing (%s) pid %d [%s]...skipped (%s)\n",
	    log_prefix, was_refreeze ? "re" : "",
	    (coalition == NULL ? "general" : "coalition-driven"), pid,
	    ((p && *p->p_name) ? p->p_name : "unknown"), reason);
}

  
                                                              
                                     
   
static void
memorystatus_freeze_start_normal_throttle_interval(uint32_t new_budget, mach_timespec_t start_ts)
{
	LCK_MTX_ASSERT(&freezer_mutex, LCK_MTX_ASSERT_OWNED);
	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_NOTOWNED);

	normal_throttle_window->max_pageouts = new_budget;
	normal_throttle_window->ts.tv_sec = normal_throttle_window->mins * 60;
	normal_throttle_window->ts.tv_nsec = 0;
	ADD_MACH_TIMESPEC(&normal_throttle_window->ts, &start_ts);
	                                                                              
	if (normal_throttle_window->pageouts > normal_throttle_window->max_pageouts) {
		normal_throttle_window->pageouts -= normal_throttle_window->max_pageouts;
	} else {
		normal_throttle_window->pageouts = 0;
	}
	                                             
	memorystatus_freeze_degradation = FALSE;
	memorystatus_freezer_stats.mfs_shared_pages_skipped = 0;
	  
                                                                           
    
	os_atomic_store(&memorystatus_freezer_stats.mfs_processes_thawed, 0, release);
	os_atomic_store(&memorystatus_freezer_stats.mfs_processes_frozen, memorystatus_frozen_count, release);
	os_atomic_inc(&memorystatus_freeze_current_interval, release);
}

#if DEVELOPMENT || DEBUG

static int
sysctl_memorystatus_freeze_calculate_new_budget SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error = 0;
	unsigned int time_since_last_interval_expired_sec = 0;
	unsigned int new_budget;

	error = sysctl_handle_int(oidp, &time_since_last_interval_expired_sec, 0, req);
	if (error || !req->newptr) {
		return error;
	}
	new_budget = memorystatus_freeze_calculate_new_budget(time_since_last_interval_expired_sec, 1, NORMAL_WINDOW_MINS, 0);
	return copyout(&new_budget, req->oldptr, MIN(sizeof(req->oldlen), sizeof(new_budget)));
}

SYSCTL_PROC(_vm, OID_AUTO, memorystatus_freeze_calculate_new_budget, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_MASKED,
    0, 0, &sysctl_memorystatus_freeze_calculate_new_budget, "I", "");

#endif                           

  
                                                         
                                                            
                                                
   
static void
memorystatus_freeze_out_of_budget(const struct throttle_interval_t *interval)
{
	LCK_MTX_ASSERT(&freezer_mutex, LCK_MTX_ASSERT_OWNED);
	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_NOTOWNED);

	mach_timespec_t time_left = {0, 0};
	mach_timespec_t now_ts;
	clock_sec_t sec;
	clock_nsec_t nsec;

	time_left.tv_sec = interval->ts.tv_sec;
	time_left.tv_nsec = 0;
	clock_get_system_nanotime(&sec, &nsec);
	now_ts.tv_sec = (unsigned int)(MIN(sec, UINT32_MAX));
	now_ts.tv_nsec = nsec;

	SUB_MACH_TIMESPEC(&time_left, &now_ts);
	os_log(OS_LOG_DEFAULT,
	    "memorystatus_freeze: Out of NAND write budget with %u minutes left in the current freezer interval. %u procs are frozen.\n",
	    time_left.tv_sec / 60, memorystatus_frozen_count);

	memorystatus_freeze_mark_eligible_processes_with_skip_reason(kMemorystatusFreezeSkipReasonOutOfBudget, false);
}

  
                                                                               
                                                                     
   
static void
memorystatus_freeze_out_of_slots(void)
{
	LCK_MTX_ASSERT(&freezer_mutex, LCK_MTX_ASSERT_OWNED);
	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_OWNED);
	assert(memorystatus_frozen_count == memorystatus_frozen_processes_max);

	os_log(OS_LOG_DEFAULT,
	    "memorystatus_freeze: Out of slots in the freezer. %u procs are frozen.\n",
	    memorystatus_frozen_count);

	memorystatus_freeze_mark_eligible_processes_with_skip_reason(kMemorystatusFreezeSkipReasonOutOfSlots, true);
}

  
                                  
  
                                                                             
                                                                                
                                                                                                  
  
                                                                                                   
  
                                                               
  
                                                                                                     
                                                                                                  
                          
  
                                        
                                             
   

static void
memorystatus_freeze_update_throttle(uint64_t *budget_pages_allowed)
{
	clock_sec_t sec;
	clock_nsec_t nsec;
	mach_timespec_t now_ts;
	LCK_MTX_ASSERT(&freezer_mutex, LCK_MTX_ASSERT_OWNED);
	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_NOTOWNED);

	unsigned int freeze_daily_pageouts_max = 0;
	uint32_t budget_rollover = 0;
	bool started_with_budget = (*budget_pages_allowed > 0);

#if DEVELOPMENT || DEBUG
	if (!memorystatus_freeze_throttle_enabled) {
		  
                                                          
     
		*budget_pages_allowed = UINT64_MAX;
		return;
	}
#endif

	clock_get_system_nanotime(&sec, &nsec);
	now_ts.tv_sec = (unsigned int)(MIN(sec, UINT32_MAX));
	now_ts.tv_nsec = nsec;

	struct throttle_interval_t *interval = NULL;

	if (memorystatus_freeze_degradation == TRUE) {
		interval = degraded_throttle_window;

		if (CMP_MACH_TIMESPEC(&now_ts, &interval->ts) >= 0) {
			interval->pageouts = 0;
			interval->max_pageouts = 0;
		} else {
			*budget_pages_allowed = interval->max_pageouts - interval->pageouts;
		}
	}

	interval = normal_throttle_window;

	if (CMP_MACH_TIMESPEC(&now_ts, &interval->ts) >= 0) {
		                                                               
		mach_timespec_t expiration_period_ts = now_ts;
		SUB_MACH_TIMESPEC(&expiration_period_ts, &interval->ts);
		                                                                                           
		budget_rollover = interval->pageouts > interval->max_pageouts ?
		    0 : interval->max_pageouts - interval->pageouts;

		memorystatus_freeze_start_normal_throttle_interval(memorystatus_freeze_calculate_new_budget(
			    expiration_period_ts.tv_sec, interval->burst_multiple,
			    interval->mins, budget_rollover),
		    now_ts);
		*budget_pages_allowed = interval->max_pageouts;

		memorystatus_demote_frozen_processes(FALSE);                                           
	} else {
		  
                             
                                             
                                                      
                            
                                                                       
     

		if (memorystatus_freeze_degradation == FALSE) {
			if (interval->pageouts >= interval->max_pageouts) {
				*budget_pages_allowed = 0;
				if (started_with_budget) {
					memorystatus_freeze_out_of_budget(interval);
				}
			} else {
				int budget_left = interval->max_pageouts - interval->pageouts;
				int budget_threshold = (freeze_daily_pageouts_max * FREEZE_DEGRADATION_BUDGET_THRESHOLD) / 100;

				mach_timespec_t time_left = {0, 0};

				time_left.tv_sec = interval->ts.tv_sec;
				time_left.tv_nsec = 0;

				SUB_MACH_TIMESPEC(&time_left, &now_ts);

				if (budget_left <= budget_threshold) {
					  
                                                                                                              
                                                                                                              
                             
        

					unsigned int current_budget_rate_allowed = ((budget_left / time_left.tv_sec) / 60) * DEGRADED_WINDOW_MINS;
					unsigned int normal_budget_rate_allowed = (freeze_daily_pageouts_max / NORMAL_WINDOW_MINS) * DEGRADED_WINDOW_MINS;

					  
                                                                      
                                                                       
        

					if (current_budget_rate_allowed < normal_budget_rate_allowed) {
						memorystatus_freeze_degradation = TRUE;
						degraded_throttle_window->max_pageouts = current_budget_rate_allowed;
						degraded_throttle_window->pageouts = 0;

						  
                                                                  
                                           
         
						interval = degraded_throttle_window;
					}
				}

				*budget_pages_allowed = interval->max_pageouts - interval->pageouts;
			}
		}
	}

	MEMORYSTATUS_DEBUG(1, "memorystatus_freeze_update_throttle_interval: throttle updated - %d frozen (%d max) within %dm; %dm remaining; throttle %s\n",
	    interval->pageouts, interval->max_pageouts, interval->mins, (interval->ts.tv_sec - now_ts->tv_sec) / 60,
	    interval->throttle ? "on" : "off");
}

static void
memorystatus_freeze_thread(void *param __unused, wait_result_t wr __unused)
{
	static boolean_t memorystatus_freeze_swap_low = FALSE;

	lck_mtx_lock(&freezer_mutex);

	if (memorystatus_freeze_enabled) {
		if ((memorystatus_frozen_count < memorystatus_frozen_processes_max) ||
		    (memorystatus_refreeze_eligible_count >= MIN_THAW_REFREEZE_THRESHOLD)) {
			if (memorystatus_can_freeze(&memorystatus_freeze_swap_low)) {
				                                                           
				memorystatus_freeze_update_throttle(&memorystatus_freeze_budget_pages_remaining);

				if (memorystatus_freeze_budget_pages_remaining) {
					memorystatus_freeze_top_process();
				} else {
					memorystatus_demote_frozen_processes(TRUE);                                      
				}
			}
		}
	}

	  
                                                                                               
                              
    
	memorystatus_freezer_thread_next_run_ts = mach_absolute_time() + memorystatus_apps_idle_delay_time;

	assert_wait((event_t) &memorystatus_freeze_wakeup, THREAD_UNINT);
	lck_mtx_unlock(&freezer_mutex);

	thread_block((thread_continue_t) memorystatus_freeze_thread);
}

boolean_t
memorystatus_freeze_thread_should_run(void)
{
	  
                                                       
                                       
    

	boolean_t should_run = FALSE;

	if (memorystatus_freeze_enabled == FALSE) {
		goto out;
	}

	if (memorystatus_available_pages > memorystatus_freeze_threshold) {
		goto out;
	}

	memorystatus_freezer_stats.mfs_below_threshold_count++;

	if ((memorystatus_frozen_count >= memorystatus_frozen_processes_max)) {
		  
                                                                   
                                   
     
		memorystatus_freezer_stats.mfs_skipped_full_count++;
		if (memorystatus_refreeze_eligible_count < MIN_THAW_REFREEZE_THRESHOLD) {
			goto out;
		}
	}

	if (memorystatus_frozen_shared_mb_max && (memorystatus_frozen_shared_mb >= memorystatus_frozen_shared_mb_max)) {
		memorystatus_freezer_stats.mfs_skipped_shared_mb_high_count++;
		goto out;
	}

	uint64_t curr_time = mach_absolute_time();

	if (curr_time < memorystatus_freezer_thread_next_run_ts) {
		goto out;
	}

	should_run = TRUE;

out:
	return should_run;
}

int
memorystatus_get_process_is_freezable(pid_t pid, int *is_freezable)
{
	proc_t p = PROC_NULL;

	if (pid == 0) {
		return EINVAL;
	}

	p = proc_find(pid);
	if (!p) {
		return ESRCH;
	}

	  
                                                
                                                                                  
    
	if (p != current_proc()) {
		proc_rele(p);
		return EPERM;
	}

	proc_list_lock();
	*is_freezable = ((p->p_memstat_state & P_MEMSTAT_FREEZE_DISABLED) ? 0 : 1);
	proc_rele_locked(p);
	proc_list_unlock();

	return 0;
}

errno_t
memorystatus_get_process_is_frozen(pid_t pid, int *is_frozen)
{
	proc_t p = PROC_NULL;

	if (pid == 0) {
		return EINVAL;
	}

	  
                                                
                                                                                  
    
	p = current_proc();
	if (p->p_pid != pid) {
		return EPERM;
	}

	proc_list_lock();
	*is_frozen = (p->p_memstat_state & P_MEMSTAT_FROZEN) != 0;
	proc_list_unlock();

	return 0;
}

int
memorystatus_set_process_is_freezable(pid_t pid, boolean_t is_freezable)
{
	proc_t p = PROC_NULL;

	if (pid == 0) {
		return EINVAL;
	}

	  
                                                                      
    
	if (is_freezable &&
	    !kauth_cred_issuser(kauth_cred_get()) &&
	    !IOTaskHasEntitlement(current_task(), MEMORYSTATUS_ENTITLEMENT)) {
		return EPERM;
	}

	p = proc_find(pid);
	if (!p) {
		return ESRCH;
	}

	  
                                                               
                                           
    
	if (p != current_proc()) {
		coalition_t coal = task_get_coalition(proc_task(p), COALITION_TYPE_JETSAM);
		if (!coalition_is_leader(proc_task(current_proc()), coal)) {
			proc_rele(p);
			return EPERM;
		}
	}

	proc_list_lock();
	if (is_freezable == FALSE) {
		                                                                            
		p->p_memstat_state |= P_MEMSTAT_FREEZE_DISABLED;
		printf("memorystatus_set_process_is_freezable: disabling freeze for pid %d [%s]\n",
		    p->p_pid, (*p->p_name ? p->p_name : "unknown"));
	} else {
		p->p_memstat_state &= ~P_MEMSTAT_FREEZE_DISABLED;
		printf("memorystatus_set_process_is_freezable: enabling freeze for pid %d [%s]\n",
		    p->p_pid, (*p->p_name ? p->p_name : "unknown"));
	}
	proc_rele_locked(p);
	proc_list_unlock();

	return 0;
}

  
                                                                              
   
void
memorystatus_freeze_init_proc(proc_t p)
{
	                                                                                                                        
	if (memorystatus_freeze_budget_pages_remaining == 0) {
		p->p_memstat_freeze_skip_reason = kMemorystatusFreezeSkipReasonOutOfBudget;
	} else if ((memorystatus_frozen_count >= memorystatus_frozen_processes_max)) {
		p->p_memstat_freeze_skip_reason = kMemorystatusFreezeSkipReasonOutOfSlots;
	} else {
		p->p_memstat_freeze_skip_reason = kMemorystatusFreezeSkipReasonNone;
	}
}


static int
sysctl_memorystatus_do_fastwake_warmup_all  SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)

	if (!req->newptr) {
		return EINVAL;
	}

	                                         
	if (!kauth_cred_issuser(kauth_cred_get()) && !IOTaskHasEntitlement(current_task(), MEMORYSTATUS_ENTITLEMENT)) {
		return EPERM;
	}

	if (memorystatus_freeze_enabled == FALSE) {
		return ENOTSUP;
	}

	do_fastwake_warmup_all();

	return 0;
}

SYSCTL_PROC(_kern, OID_AUTO, memorystatus_do_fastwake_warmup_all, CTLTYPE_INT | CTLFLAG_WR | CTLFLAG_LOCKED | CTLFLAG_MASKED,
    0, 0, &sysctl_memorystatus_do_fastwake_warmup_all, "I", "");

#endif                    
