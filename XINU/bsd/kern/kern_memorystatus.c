/*
 * Copyright (c) 2006-2019 Apple Inc. All rights reserved.
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

#include <corpses/task_corpse.h>
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
#include <vm/pmap.h>
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

                         
static const char *memorystatus_kill_cause_name[] = {
	"",                                                                                                             
	"jettisoned",                                                                                  
	"highwater",                                                                                               
	"vnode-limit",                                                                                     
	"vm-pageshortage",                                                                                       
	"proc-thrashing",                                                                                        
	"fc-thrashing",                                                                                        
	"per-process-limit",                                                                                      
	"disk-space-shortage",                                                                              
	"idle-exit",                                                                                                 
	"zone-map-exhaustion",                                                                              
	"vm-compressor-thrashing",                                                                             
	"vm-compressor-space-shortage",                                                                   
};

static const char *
memorystatus_priority_band_name(int32_t priority)
{
	switch (priority) {
	case JETSAM_PRIORITY_FOREGROUND:
		return "FOREGROUND";
	case JETSAM_PRIORITY_AUDIO_AND_ACCESSORY:
		return "AUDIO_AND_ACCESSORY";
	case JETSAM_PRIORITY_CONDUCTOR:
		return "CONDUCTOR";
	case JETSAM_PRIORITY_DRIVER_APPLE:
		return "DRIVER_APPLE";
	case JETSAM_PRIORITY_HOME:
		return "HOME";
	case JETSAM_PRIORITY_EXECUTIVE:
		return "EXECUTIVE";
	case JETSAM_PRIORITY_IMPORTANT:
		return "IMPORTANT";
	case JETSAM_PRIORITY_CRITICAL:
		return "CRITICAL";
	}

	return "?";
}

                                             
static boolean_t
is_reason_thrashing(unsigned cause)
{
	switch (cause) {
	case kMemorystatusKilledFCThrashing:
	case kMemorystatusKilledVMCompressorThrashing:
	case kMemorystatusKilledVMCompressorSpaceShortage:
		return TRUE;
	default:
		return FALSE;
	}
}

                                  
static boolean_t
is_reason_zone_map_exhaustion(unsigned cause)
{
	if (cause == kMemorystatusKilledZoneMapExhaustion) {
		return TRUE;
	}
	return FALSE;
}

  
                                                                                    
                      
   
extern void get_zone_map_size(uint64_t *current_size, uint64_t *capacity);

  
                                                                                       
                      
   
extern void get_largest_zone_info(char *zone_name, size_t zone_name_len, uint64_t *zone_size);

  
                                  
                           
  
                                                    
                      
  
                                                            
                                         
   

#define SET_ACTIVE_LIMITS_LOCKED(p, limit, is_fatal)                    \
MACRO_BEGIN                                                             \
(p)->p_memstat_memlimit_active = (limit);                               \
   if (is_fatal) {                                                      \
	   (p)->p_memstat_state |= P_MEMSTAT_MEMLIMIT_ACTIVE_FATAL;     \
   } else {                                                             \
	   (p)->p_memstat_state &= ~P_MEMSTAT_MEMLIMIT_ACTIVE_FATAL;    \
   }                                                                    \
MACRO_END

#define SET_INACTIVE_LIMITS_LOCKED(p, limit, is_fatal)                  \
MACRO_BEGIN                                                             \
(p)->p_memstat_memlimit_inactive = (limit);                             \
   if (is_fatal) {                                                      \
	   (p)->p_memstat_state |= P_MEMSTAT_MEMLIMIT_INACTIVE_FATAL;   \
   } else {                                                             \
	   (p)->p_memstat_state &= ~P_MEMSTAT_MEMLIMIT_INACTIVE_FATAL;  \
   }                                                                    \
MACRO_END

#define CACHE_ACTIVE_LIMITS_LOCKED(p, is_fatal)                         \
MACRO_BEGIN                                                             \
(p)->p_memstat_memlimit = (p)->p_memstat_memlimit_active;               \
   if ((p)->p_memstat_state & P_MEMSTAT_MEMLIMIT_ACTIVE_FATAL) {        \
	   (p)->p_memstat_state |= P_MEMSTAT_FATAL_MEMLIMIT;            \
	   is_fatal = TRUE;                                             \
   } else {                                                             \
	   (p)->p_memstat_state &= ~P_MEMSTAT_FATAL_MEMLIMIT;           \
	   is_fatal = FALSE;                                            \
   }                                                                    \
MACRO_END

#define CACHE_INACTIVE_LIMITS_LOCKED(p, is_fatal)                       \
MACRO_BEGIN                                                             \
(p)->p_memstat_memlimit = (p)->p_memstat_memlimit_inactive;             \
   if ((p)->p_memstat_state & P_MEMSTAT_MEMLIMIT_INACTIVE_FATAL) {      \
	   (p)->p_memstat_state |= P_MEMSTAT_FATAL_MEMLIMIT;            \
	   is_fatal = TRUE;                                             \
   } else {                                                             \
	   (p)->p_memstat_state &= ~P_MEMSTAT_FATAL_MEMLIMIT;           \
	   is_fatal = FALSE;                                            \
   }                                                                    \
MACRO_END


                      

unsigned long delta_percentage = 5;
unsigned long critical_threshold_percentage = 5;
                                                                                     
uint64_t config_jetsam_large_memory_cutoff = 3UL * (1UL << 30);
unsigned long critical_threshold_percentage_larger_devices = 4;
unsigned long delta_percentage_larger_devices = 4;
unsigned long idle_offset_percentage = 5;
unsigned long pressure_threshold_percentage = 15;
unsigned long policy_more_free_offset_percentage = 5;
unsigned long sysproc_aging_aggr_threshold_percentage = 7;

  
                                  
   
memorystatus_jetsam_snapshot_t *memorystatus_jetsam_snapshot;
memorystatus_jetsam_snapshot_t *memorystatus_jetsam_snapshot_copy;

#if CONFIG_FREEZE
memorystatus_jetsam_snapshot_t *memorystatus_jetsam_snapshot_freezer;
  
                                                                                                                     
                                                                     
                                                                                             
                                                                               
   
#define JETSAM_SNAPSHOT_FREEZER_MAX_FACTOR 20
unsigned int memorystatus_jetsam_snapshot_freezer_max;
unsigned int memorystatus_jetsam_snapshot_freezer_size;
TUNABLE(bool, memorystatus_jetsam_use_freezer_snapshot, "kern.jetsam_user_freezer_snapshot", true);
#endif                    

unsigned int memorystatus_jetsam_snapshot_count = 0;
unsigned int memorystatus_jetsam_snapshot_copy_count = 0;
unsigned int memorystatus_jetsam_snapshot_max = 0;
unsigned int memorystatus_jetsam_snapshot_size = 0;
uint64_t memorystatus_jetsam_snapshot_last_timestamp = 0;
uint64_t memorystatus_jetsam_snapshot_timeout = 0;

#if DEVELOPMENT || DEBUG
  
                                                                       
                                                                                        
                                                                                                   
                                                                       
                                                                               
   
static pid_t memorystatus_testing_pid = 0;
SYSCTL_INT(_kern, OID_AUTO, memorystatus_testing_pid, CTLTYPE_INT | CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_testing_pid, 0, "");
#endif                           
static void memorystatus_init_jetsam_snapshot_header(memorystatus_jetsam_snapshot_t *snapshot);

                                

uint64_t memorystatus_sysprocs_idle_delay_time = 0;
uint64_t memorystatus_apps_idle_delay_time = 0;
                                                           
#if __arm64__
int32_t memorystatus_entitled_max_task_footprint_mb = 0;

#if DEVELOPMENT || DEBUG
SYSCTL_INT(_kern, OID_AUTO, entitled_max_task_pmem, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_entitled_max_task_footprint_mb, 0, "");
#endif                           
#endif                

static LCK_GRP_DECLARE(memorystatus_jetsam_fg_band_lock_grp,
    "memorystatus_jetsam_fg_band");
LCK_MTX_DECLARE(memorystatus_jetsam_fg_band_lock,
    &memorystatus_jetsam_fg_band_lock_grp);

                         

static int32_t memorystatus_scheduled_idle_demotions_sysprocs = 0;
static int32_t memorystatus_scheduled_idle_demotions_apps = 0;

static void memorystatus_perform_idle_demotion(__unused void *spare1, __unused void *spare2);
static void memorystatus_schedule_idle_demotion_locked(proc_t p, boolean_t set_state);
static void memorystatus_reschedule_idle_demotion_locked(void);
int memorystatus_update_priority_for_appnap(proc_t p, boolean_t is_appnap);
vm_pressure_level_t convert_internal_pressure_level_to_dispatch_level(vm_pressure_level_t);
boolean_t is_knote_registered_modify_task_pressure_bits(struct knote*, int, task_t, vm_pressure_level_t, vm_pressure_level_t);
void memorystatus_klist_reset_all_for_level(vm_pressure_level_t pressure_level_to_clear);
void memorystatus_send_low_swap_note(void);
int memorystatus_get_proccnt_upto_priority(int32_t max_bucket_index);
boolean_t memorystatus_kill_elevated_process(uint32_t cause, os_reason_t jetsam_reason, unsigned int band, int aggr_count,
    uint32_t *errors, uint64_t *memory_reclaimed);
uint64_t memorystatus_available_memory_internal(proc_t p);

unsigned int memorystatus_level = 0;
static int memorystatus_list_count = 0;
memstat_bucket_t memstat_bucket[MEMSTAT_BUCKET_COUNT];
static thread_call_t memorystatus_idle_demotion_call;
uint64_t memstat_idle_demotion_deadline = 0;
int system_procs_aging_band = JETSAM_PRIORITY_AGING_BAND1;
int applications_aging_band = JETSAM_PRIORITY_IDLE;

#define isProcessInAgingBands(p)        ((isSysProc(p) && system_procs_aging_band && (p->p_memstat_effectivepriority == system_procs_aging_band)) || (isApp(p) && applications_aging_band && (p->p_memstat_effectivepriority == applications_aging_band)))

#define kJetsamAgingPolicyNone                          (0)
#define kJetsamAgingPolicyLegacy                        (1)
#define kJetsamAgingPolicySysProcsReclaimedFirst        (2)
#define kJetsamAgingPolicyAppsReclaimedFirst            (3)
#define kJetsamAgingPolicyMax                           kJetsamAgingPolicyAppsReclaimedFirst

unsigned int jetsam_aging_policy = kJetsamAgingPolicySysProcsReclaimedFirst;

extern int corpse_for_fatal_memkill;
extern uint64_t vm_purgeable_purge_task_owned(task_t task);
boolean_t memorystatus_allowed_vm_map_fork(task_t);
#if DEVELOPMENT || DEBUG
void memorystatus_abort_vm_map_fork(task_t);
#endif

  
                                                                                  
                                                         
   
#define kJetsamSysProcsIdleDelayTimeLowRatio    (5)
#define kJetsamSysProcsIdleDelayTimeMedRatio    (2)
#define kJetsamSysProcsIdleDelayTimeHighRatio   (1)
static_assert(kJetsamSysProcsIdleDelayTimeLowRatio <= DEFERRED_IDLE_EXIT_TIME_SECS, "sysproc idle delay time for low relaunch daemons would be 0");

  
                                                                                    
                                      
   
#define kJetsamAppsIdleDelayTimeRatio   (kJetsamSysProcsIdleDelayTimeLowRatio)

static uint64_t
memorystatus_sysprocs_idle_time(proc_t p)
{
	  
                                                                                           
                                                                                       
                                                        
    
	if (jetsam_aging_policy != kJetsamAgingPolicySysProcsReclaimedFirst) {
		return memorystatus_sysprocs_idle_delay_time;
	}

	uint64_t idle_delay_time = 0;
	  
                                                         
                                                           
                                                             
                                      
    
	switch (p->p_memstat_relaunch_flags) {
	case P_MEMSTAT_RELAUNCH_UNKNOWN:
	case P_MEMSTAT_RELAUNCH_LOW:
		idle_delay_time = memorystatus_sysprocs_idle_delay_time / kJetsamSysProcsIdleDelayTimeLowRatio;
		break;
	case P_MEMSTAT_RELAUNCH_MED:
		idle_delay_time = memorystatus_sysprocs_idle_delay_time / kJetsamSysProcsIdleDelayTimeMedRatio;
		break;
	case P_MEMSTAT_RELAUNCH_HIGH:
		idle_delay_time = memorystatus_sysprocs_idle_delay_time / kJetsamSysProcsIdleDelayTimeHighRatio;
		break;
	default:
		panic("Unknown relaunch flags on process!");
		break;
	}
	return idle_delay_time;
}

static uint64_t
memorystatus_apps_idle_time(__unused proc_t p)
{
	  
                                                                                
                                                                                 
                                                       
    
	if (jetsam_aging_policy != kJetsamAgingPolicySysProcsReclaimedFirst) {
		return memorystatus_apps_idle_delay_time;
	}

	return memorystatus_apps_idle_delay_time / kJetsamAppsIdleDelayTimeRatio;
}


#if 0

                                                                                                                    

static int
sysctl_set_jetsam_aging_policy SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)

	int error = 0, val = 0;
	memstat_bucket_t *old_bucket = 0;
	int old_system_procs_aging_band = 0, new_system_procs_aging_band = 0;
	int old_applications_aging_band = 0, new_applications_aging_band = 0;
	proc_t p = NULL, next_proc = NULL;


	error = sysctl_io_number(req, jetsam_aging_policy, sizeof(int), &val, NULL);
	if (error || !req->newptr) {
		return error;
	}

	if ((val < 0) || (val > kJetsamAgingPolicyMax)) {
		printf("jetsam: ordering policy sysctl has invalid value - %d\n", val);
		return EINVAL;
	}

	  
                                                                             
                                                                             
                                                                             
                         
    
	proc_list_lock();

	old_system_procs_aging_band = system_procs_aging_band;
	old_applications_aging_band = applications_aging_band;

	switch (val) {
	case kJetsamAgingPolicyNone:
		new_system_procs_aging_band = JETSAM_PRIORITY_IDLE;
		new_applications_aging_band = JETSAM_PRIORITY_IDLE;
		break;

	case kJetsamAgingPolicyLegacy:
		  
                                                                                                                                                       
     
		new_system_procs_aging_band = JETSAM_PRIORITY_AGING_BAND1;
		new_applications_aging_band = JETSAM_PRIORITY_IDLE;
		break;

	case kJetsamAgingPolicySysProcsReclaimedFirst:
		new_system_procs_aging_band = JETSAM_PRIORITY_AGING_BAND1;
		new_applications_aging_band = JETSAM_PRIORITY_AGING_BAND2;
		break;

	case kJetsamAgingPolicyAppsReclaimedFirst:
		new_system_procs_aging_band = JETSAM_PRIORITY_AGING_BAND2;
		new_applications_aging_band = JETSAM_PRIORITY_AGING_BAND1;
		break;

	default:
		break;
	}

	if (old_system_procs_aging_band && (old_system_procs_aging_band != new_system_procs_aging_band)) {
		old_bucket = &memstat_bucket[old_system_procs_aging_band];
		p = TAILQ_FIRST(&old_bucket->list);

		while (p) {
			next_proc = TAILQ_NEXT(p, p_memstat_list);

			if (isSysProc(p)) {
				if (new_system_procs_aging_band == JETSAM_PRIORITY_IDLE) {
					memorystatus_invalidate_idle_demotion_locked(p, TRUE);
				}

				memorystatus_update_priority_locked(p, new_system_procs_aging_band, false, true);
			}

			p = next_proc;
			continue;
		}
	}

	if (old_applications_aging_band && (old_applications_aging_band != new_applications_aging_band)) {
		old_bucket = &memstat_bucket[old_applications_aging_band];
		p = TAILQ_FIRST(&old_bucket->list);

		while (p) {
			next_proc = TAILQ_NEXT(p, p_memstat_list);

			if (isApp(p)) {
				if (new_applications_aging_band == JETSAM_PRIORITY_IDLE) {
					memorystatus_invalidate_idle_demotion_locked(p, TRUE);
				}

				memorystatus_update_priority_locked(p, new_applications_aging_band, false, true);
			}

			p = next_proc;
			continue;
		}
	}

	jetsam_aging_policy = val;
	system_procs_aging_band = new_system_procs_aging_band;
	applications_aging_band = new_applications_aging_band;

	proc_list_unlock();

	return 0;
}

SYSCTL_PROC(_kern, OID_AUTO, set_jetsam_aging_policy, CTLTYPE_INT | CTLFLAG_RW,
    0, 0, sysctl_set_jetsam_aging_policy, "I", "Jetsam Aging Policy");
#endif      

static int
sysctl_jetsam_set_sysprocs_idle_delay_time SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)

	int error = 0, val = 0, old_time_in_secs = 0;
	uint64_t old_time_in_ns = 0;

	absolutetime_to_nanoseconds(memorystatus_sysprocs_idle_delay_time, &old_time_in_ns);
	old_time_in_secs = (int) (old_time_in_ns / NSEC_PER_SEC);

	error = sysctl_io_number(req, old_time_in_secs, sizeof(int), &val, NULL);
	if (error || !req->newptr) {
		return error;
	}

	if ((val < 0) || (val > INT32_MAX)) {
		printf("jetsam: new idle delay interval has invalid value.\n");
		return EINVAL;
	}

	nanoseconds_to_absolutetime((uint64_t)val * NSEC_PER_SEC, &memorystatus_sysprocs_idle_delay_time);

	return 0;
}

SYSCTL_PROC(_kern, OID_AUTO, memorystatus_sysprocs_idle_delay_time, CTLTYPE_INT | CTLFLAG_RW,
    0, 0, sysctl_jetsam_set_sysprocs_idle_delay_time, "I", "Aging window for system processes");


static int
sysctl_jetsam_set_apps_idle_delay_time SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)

	int error = 0, val = 0, old_time_in_secs = 0;
	uint64_t old_time_in_ns = 0;

	absolutetime_to_nanoseconds(memorystatus_apps_idle_delay_time, &old_time_in_ns);
	old_time_in_secs = (int) (old_time_in_ns / NSEC_PER_SEC);

	error = sysctl_io_number(req, old_time_in_secs, sizeof(int), &val, NULL);
	if (error || !req->newptr) {
		return error;
	}

	if ((val < 0) || (val > INT32_MAX)) {
		printf("jetsam: new idle delay interval has invalid value.\n");
		return EINVAL;
	}

	nanoseconds_to_absolutetime((uint64_t)val * NSEC_PER_SEC, &memorystatus_apps_idle_delay_time);

	return 0;
}

SYSCTL_PROC(_kern, OID_AUTO, memorystatus_apps_idle_delay_time, CTLTYPE_INT | CTLFLAG_RW,
    0, 0, sysctl_jetsam_set_apps_idle_delay_time, "I", "Aging window for applications");

SYSCTL_INT(_kern, OID_AUTO, jetsam_aging_policy, CTLTYPE_INT | CTLFLAG_RD, &jetsam_aging_policy, 0, "");

static unsigned int memorystatus_dirty_count = 0;

SYSCTL_INT(_kern, OID_AUTO, max_task_pmem, CTLFLAG_RD | CTLFLAG_LOCKED | CTLFLAG_MASKED, &max_task_footprint_mb, 0, "");

static int memorystatus_highwater_enabled = 1;                                        
static boolean_t proc_jetsam_state_is_active_locked(proc_t);

#if __arm64__
int legacy_footprint_bonus_mb = 50;                                                          
                                                                                              
                                                                                             
                                       

#if DEVELOPMENT || DEBUG
SYSCTL_INT(_kern, OID_AUTO, legacy_footprint_bonus_mb, CTLFLAG_RW | CTLFLAG_LOCKED, &legacy_footprint_bonus_mb, 0, "");
#endif                           
  
                                                             
                                                                          
                
                                      
   
static void
memorystatus_raise_memlimit(proc_t p, int new_memlimit_active, int new_memlimit_inactive)
{
	int memlimit_mb_active = 0, memlimit_mb_inactive = 0;
	boolean_t memlimit_active_is_fatal = FALSE, memlimit_inactive_is_fatal = FALSE, use_active_limit = FALSE;

	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_OWNED);

	if (p->p_memstat_memlimit_active > 0) {
		memlimit_mb_active = p->p_memstat_memlimit_active;
	} else if (p->p_memstat_memlimit_active == -1) {
		memlimit_mb_active = max_task_footprint_mb;
	} else {
		  
                                   
                                         
                         
     
		return;
	}

	if (p->p_memstat_memlimit_inactive > 0) {
		memlimit_mb_inactive = p->p_memstat_memlimit_inactive;
	} else if (p->p_memstat_memlimit_inactive == -1) {
		memlimit_mb_inactive = max_task_footprint_mb;
	} else {
		  
                                   
                                         
                         
     
		return;
	}

	memlimit_mb_active = MAX(new_memlimit_active, memlimit_mb_active);
	memlimit_mb_inactive = MAX(new_memlimit_inactive, memlimit_mb_inactive);

	memlimit_active_is_fatal = (p->p_memstat_state & P_MEMSTAT_MEMLIMIT_ACTIVE_FATAL);
	memlimit_inactive_is_fatal = (p->p_memstat_state & P_MEMSTAT_MEMLIMIT_INACTIVE_FATAL);

	SET_ACTIVE_LIMITS_LOCKED(p, memlimit_mb_active, memlimit_active_is_fatal);
	SET_INACTIVE_LIMITS_LOCKED(p, memlimit_mb_inactive, memlimit_inactive_is_fatal);

	if (proc_jetsam_state_is_active_locked(p) == TRUE) {
		use_active_limit = TRUE;
		CACHE_ACTIVE_LIMITS_LOCKED(p, memlimit_active_is_fatal);
	} else {
		CACHE_INACTIVE_LIMITS_LOCKED(p, memlimit_inactive_is_fatal);
	}

	if (memorystatus_highwater_enabled) {
		task_set_phys_footprint_limit_internal(p->task,
		    (p->p_memstat_memlimit > 0) ? p->p_memstat_memlimit : -1,
		    NULL,                                                         
		    use_active_limit,                                                     
		    (use_active_limit ? memlimit_active_is_fatal : memlimit_inactive_is_fatal));
	}
}

void
memorystatus_act_on_legacy_footprint_entitlement(proc_t p, boolean_t footprint_increase)
{
	int memlimit_mb_active = 0, memlimit_mb_inactive = 0;

	if (p == NULL) {
		return;
	}

	proc_list_lock();

	if (p->p_memstat_memlimit_active > 0) {
		memlimit_mb_active = p->p_memstat_memlimit_active;
	} else if (p->p_memstat_memlimit_active == -1) {
		memlimit_mb_active = max_task_footprint_mb;
	} else {
		  
                                   
                                         
                         
     
		proc_list_unlock();
		return;
	}

	if (p->p_memstat_memlimit_inactive > 0) {
		memlimit_mb_inactive = p->p_memstat_memlimit_inactive;
	} else if (p->p_memstat_memlimit_inactive == -1) {
		memlimit_mb_inactive = max_task_footprint_mb;
	} else {
		  
                                   
                                         
                         
     
		proc_list_unlock();
		return;
	}

	if (footprint_increase) {
		memlimit_mb_active += legacy_footprint_bonus_mb;
		memlimit_mb_inactive += legacy_footprint_bonus_mb;
	} else {
		memlimit_mb_active -= legacy_footprint_bonus_mb;
		if (memlimit_mb_active == max_task_footprint_mb) {
			memlimit_mb_active = -1;                                             
		}

		memlimit_mb_inactive -= legacy_footprint_bonus_mb;
		if (memlimit_mb_inactive == max_task_footprint_mb) {
			memlimit_mb_inactive = -1;                                             
		}
	}
	memorystatus_raise_memlimit(p, memlimit_mb_active, memlimit_mb_inactive);

	proc_list_unlock();
}

void
memorystatus_act_on_ios13extended_footprint_entitlement(proc_t p)
{
	if (max_mem < 1500ULL * 1024 * 1024 ||
	    max_mem > 2ULL * 1024 * 1024 * 1024) {
		                                                     
		return;
	}
	                           
	proc_list_lock();
	memorystatus_raise_memlimit(p, 1800, 1800);
	proc_list_unlock();
}

void
memorystatus_act_on_entitled_task_limit(proc_t p)
{
	if (memorystatus_entitled_max_task_footprint_mb == 0) {
		                                               
		return;
	}
	proc_list_lock();
	memorystatus_raise_memlimit(p, memorystatus_entitled_max_task_footprint_mb, memorystatus_entitled_max_task_footprint_mb);
	proc_list_unlock();
}
#endif                

SYSCTL_INT(_kern, OID_AUTO, memorystatus_level, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_level, 0, "");

int
memorystatus_get_level(__unused struct proc *p, struct memorystatus_get_level_args *args, __unused int *ret)
{
	user_addr_t     level = 0;

	level = args->level;

	if (copyout(&memorystatus_level, level, sizeof(memorystatus_level)) != 0) {
		return EFAULT;
	}

	return 0;
}

static void memorystatus_thread(void *param __unused, wait_result_t wr __unused);

                   

static boolean_t memorystatus_kill_specific_process(pid_t victim_pid, uint32_t cause, os_reason_t jetsam_reason);
static boolean_t memorystatus_kill_process_sync(pid_t victim_pid, uint32_t cause, os_reason_t jetsam_reason);


static int memorystatus_cmd_set_memlimit_properties(pid_t pid, user_addr_t buffer, size_t buffer_size, __unused int32_t *retval);

static int memorystatus_set_memlimit_properties(pid_t pid, memorystatus_memlimit_properties_t *entry);

static int memorystatus_cmd_get_memlimit_properties(pid_t pid, user_addr_t buffer, size_t buffer_size, __unused int32_t *retval);

static int memorystatus_cmd_get_memlimit_excess_np(pid_t pid, uint32_t flags, user_addr_t buffer, size_t buffer_size, __unused int32_t *retval);

static void memorystatus_get_memlimit_properties_internal(proc_t p, memorystatus_memlimit_properties_t *p_entry);
static int memorystatus_set_memlimit_properties_internal(proc_t p, memorystatus_memlimit_properties_t *p_entry);

int proc_get_memstat_priority(proc_t, boolean_t);

static boolean_t memorystatus_idle_snapshot = 0;

unsigned int memorystatus_delta = 0;

                           
static boolean_t memorystatus_jld_enabled = FALSE;                                                
static uint32_t memorystatus_jld_eval_period_msecs = 0;                                                              
static int      memorystatus_jld_eval_aggressive_count = 3;                                                            
static int      memorystatus_jld_eval_aggressive_priority_band_max = 15;                                              

  
                                                                                                                                       
                                                                                                                                                                                                
  
                
                                                                                                                                   
                                                                                                                   
  
                                                                                                                                    
  
                                                                                                        
   

#define AGGRESSIVE_JETSAM_LENIENT_MODE_THRESHOLD        25
boolean_t       memorystatus_aggressive_jetsam_lenient_allowed = FALSE;
boolean_t       memorystatus_aggressive_jetsam_lenient = FALSE;

#if DEVELOPMENT || DEBUG
  
                                  
   

SYSCTL_UINT(_kern, OID_AUTO, memorystatus_jld_eval_period_msecs, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_jld_eval_period_msecs, 0, "");
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_jld_eval_aggressive_count, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_jld_eval_aggressive_count, 0, "");
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_jld_eval_aggressive_priority_band_max, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_jld_eval_aggressive_priority_band_max, 0, "");
#endif                           

static uint32_t kill_under_pressure_cause = 0;

  
                                                   
   
static memorystatus_jetsam_snapshot_t memorystatus_at_boot_snapshot;

static void memorystatus_init_jetsam_snapshot_locked(memorystatus_jetsam_snapshot_t *od_snapshot, uint32_t ods_list_count);
static boolean_t memorystatus_init_jetsam_snapshot_entry_locked(proc_t p, memorystatus_jetsam_snapshot_entry_t *entry, uint64_t gencount);
static void memorystatus_update_jetsam_snapshot_entry_locked(proc_t p, uint32_t kill_cause, uint64_t killtime);

static void memorystatus_clear_errors(void);
static void memorystatus_get_task_phys_footprint_page_counts(task_t task,
    uint64_t *internal_pages, uint64_t *internal_compressed_pages,
    uint64_t *purgeable_nonvolatile_pages, uint64_t *purgeable_nonvolatile_compressed_pages,
    uint64_t *alternate_accounting_pages, uint64_t *alternate_accounting_compressed_pages,
    uint64_t *iokit_mapped_pages, uint64_t *page_table_pages, uint64_t *frozen_to_swap_pages);

static void memorystatus_get_task_memory_region_count(task_t task, uint64_t *count);

static uint32_t memorystatus_build_state(proc_t p);
                                                                           

static boolean_t memorystatus_kill_top_process(boolean_t any, boolean_t sort_flag, uint32_t cause, os_reason_t jetsam_reason, int32_t *priority,
    uint32_t *errors, uint64_t *memory_reclaimed);
static boolean_t memorystatus_kill_processes_aggressive(uint32_t cause, int aggr_count, int32_t priority_max, uint32_t *errors, uint64_t *memory_reclaimed);
static boolean_t memorystatus_kill_hiwat_proc(uint32_t *errors, boolean_t *purged, uint64_t *memory_reclaimed);

static boolean_t memorystatus_kill_process_async(pid_t victim_pid, uint32_t cause);

                                    
static int  memorystatus_sort_bucket(unsigned int bucket_index, int sort_order);
static int  memorystatus_sort_by_largest_coalition_locked(unsigned int bucket_index, int coal_sort_order);
static void memorystatus_sort_by_largest_process_locked(unsigned int bucket_index);
static int  memorystatus_move_list_locked(unsigned int bucket_index, pid_t *pid_list, int list_sz);

                    
typedef int (*cmpfunc_t)(const void *a, const void *b);
extern void qsort(void *a, size_t n, size_t es, cmpfunc_t cmp);
static int memstat_asc_cmp(const void *a, const void *b);

                 

extern unsigned int    vm_page_free_count;
extern unsigned int    vm_page_active_count;
extern unsigned int    vm_page_inactive_count;
extern unsigned int    vm_page_throttled_count;
extern unsigned int    vm_page_purgeable_count;
extern unsigned int    vm_page_wire_count;
extern unsigned int    vm_page_speculative_count;

#if CONFIG_JETSAM
#define MEMORYSTATUS_LOG_AVAILABLE_PAGES memorystatus_available_pages
#else                    
#define MEMORYSTATUS_LOG_AVAILABLE_PAGES (vm_page_active_count + vm_page_inactive_count + vm_page_free_count + vm_page_speculative_count)
#endif                    
#if CONFIG_SECLUDED_MEMORY
extern unsigned int     vm_page_secluded_count;
extern unsigned int     vm_page_secluded_count_over_target;
#endif                             

                                                                
unsigned int memorystatus_sysproc_aging_aggr_pages = 0;

#if CONFIG_JETSAM
unsigned int memorystatus_available_pages = (unsigned int)-1;
unsigned int memorystatus_available_pages_pressure = 0;
unsigned int memorystatus_available_pages_critical = 0;
unsigned int memorystatus_available_pages_critical_base = 0;
unsigned int memorystatus_available_pages_critical_idle_offset = 0;

#if DEVELOPMENT || DEBUG
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_available_pages, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_available_pages, 0, "");
#else
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_available_pages, CTLFLAG_RD | CTLFLAG_MASKED | CTLFLAG_LOCKED, &memorystatus_available_pages, 0, "");
#endif                           

static unsigned int memorystatus_jetsam_policy = kPolicyDefault;
unsigned int memorystatus_policy_more_free_offset_pages = 0;
static void memorystatus_update_levels_locked(boolean_t critical_only);
static unsigned int memorystatus_thread_wasted_wakeup = 0;

                                                                                
extern void vm_thrashing_jetsam_done(void);
static int memorystatus_cmd_set_jetsam_memory_limit(pid_t pid, int32_t high_water_mark, __unused int32_t *retval, boolean_t is_fatal_limit);
#if DEVELOPMENT || DEBUG
static inline uint32_t
roundToNearestMB(uint32_t in)
{
	return (in + ((1 << 20) - 1)) >> 20;
}

static int memorystatus_cmd_increase_jetsam_task_limit(pid_t pid, uint32_t byte_increase);
#endif

int32_t max_kill_priority = JETSAM_PRIORITY_MAX;

#else                    

uint64_t memorystatus_available_pages = (uint64_t)-1;
uint64_t memorystatus_available_pages_pressure = (uint64_t)-1;
uint64_t memorystatus_available_pages_critical = (uint64_t)-1;

int32_t max_kill_priority = JETSAM_PRIORITY_IDLE;
#endif                    

#if DEVELOPMENT || DEBUG

static LCK_GRP_DECLARE(disconnect_page_mappings_lck_grp, "disconnect_page_mappings");
static LCK_MTX_DECLARE(disconnect_page_mappings_mutex, &disconnect_page_mappings_lck_grp);

extern bool kill_on_no_paging_space;
#endif                           


           

extern struct knote *vm_find_knote_from_pid(pid_t, struct klist *);

#if DEVELOPMENT || DEBUG

static unsigned int memorystatus_debug_dump_this_bucket = 0;

static void
memorystatus_debug_dump_bucket_locked(unsigned int bucket_index)
{
	proc_t p = NULL;
	uint64_t bytes = 0;
	int ledger_limit = 0;
	unsigned int b = bucket_index;
	boolean_t traverse_all_buckets = FALSE;

	if (bucket_index >= MEMSTAT_BUCKET_COUNT) {
		traverse_all_buckets = TRUE;
		b = 0;
	} else {
		traverse_all_buckets = FALSE;
		b = bucket_index;
	}

	  
                                       
                       
                                     
                                                          
                                     
                                       
                           
    

	printf("memorystatus_debug_dump ***START*(PAGE_SIZE_64=%llu)**\n", PAGE_SIZE_64);
	printf("bucket [pid]       [pages / MB]     [state]      [EP / RP / AP]   dirty     deadline [L-limit / C-limit / A-limit / IA-limit] name\n");
	p = memorystatus_get_first_proc_locked(&b, traverse_all_buckets);
	while (p) {
		bytes = get_task_phys_footprint(p->task);
		task_get_phys_footprint_limit(p->task, &ledger_limit);
		printf("%2d     [%5d]     [%5lld /%3lldMB]   0x%-8x   [%2d / %2d / %2d]   0x%-3x   %10lld    [%3d / %3d%s / %3d%s / %3d%s]   %s\n",
		    b, p->p_pid,
		    (bytes / PAGE_SIZE_64),                                                                     
		    (bytes / (1024ULL * 1024ULL)),                                                       
		    p->p_memstat_state, p->p_memstat_effectivepriority, p->p_memstat_requestedpriority, p->p_memstat_assertionpriority,
		    p->p_memstat_dirty, p->p_memstat_idledeadline,
		    ledger_limit,
		    p->p_memstat_memlimit,
		    (p->p_memstat_state & P_MEMSTAT_FATAL_MEMLIMIT ? "F " : "NF"),
		    p->p_memstat_memlimit_active,
		    (p->p_memstat_state & P_MEMSTAT_MEMLIMIT_ACTIVE_FATAL ? "F " : "NF"),
		    p->p_memstat_memlimit_inactive,
		    (p->p_memstat_state & P_MEMSTAT_MEMLIMIT_INACTIVE_FATAL ? "F " : "NF"),
		    (*p->p_name ? p->p_name : "unknown"));
		p = memorystatus_get_next_proc_locked(&b, p, traverse_all_buckets);
	}
	printf("memorystatus_debug_dump ***END***\n");
}

static int
sysctl_memorystatus_debug_dump_bucket SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg2)
	int bucket_index = 0;
	int error;
	error = SYSCTL_OUT(req, arg1, sizeof(int));
	if (error || !req->newptr) {
		return error;
	}
	error = SYSCTL_IN(req, &bucket_index, sizeof(int));
	if (error || !req->newptr) {
		return error;
	}
	if (bucket_index >= MEMSTAT_BUCKET_COUNT) {
		  
                                       
     
	} else {
		  
                                         
     
	}

	proc_list_lock();
	memorystatus_debug_dump_bucket_locked(bucket_index);
	proc_list_unlock();
	memorystatus_debug_dump_this_bucket = bucket_index;
	return error;
}

  
                                                              
                                                        
                                        
                                                                             
   

SYSCTL_PROC(_kern, OID_AUTO, memorystatus_debug_dump_this_bucket, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_debug_dump_this_bucket, 0, sysctl_memorystatus_debug_dump_bucket, "I", "");


                                             

static int
sysctl_memorystatus_highwater_enable SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg2)
	proc_t p;
	unsigned int b = 0;
	int error, enable = 0;
	boolean_t use_active;                                                         
	boolean_t is_fatal;

	error = SYSCTL_OUT(req, arg1, sizeof(int));
	if (error || !req->newptr) {
		return error;
	}

	error = SYSCTL_IN(req, &enable, sizeof(int));
	if (error || !req->newptr) {
		return error;
	}

	if (!(enable == 0 || enable == 1)) {
		return EINVAL;
	}

	proc_list_lock();

	p = memorystatus_get_first_proc_locked(&b, TRUE);
	while (p) {
		use_active = proc_jetsam_state_is_active_locked(p);

		if (enable) {
			if (use_active == TRUE) {
				CACHE_ACTIVE_LIMITS_LOCKED(p, is_fatal);
			} else {
				CACHE_INACTIVE_LIMITS_LOCKED(p, is_fatal);
			}
		} else {
			  
                                                          
                                                          
      
			p->p_memstat_memlimit = -1;
			p->p_memstat_state |= P_MEMSTAT_FATAL_MEMLIMIT;
			is_fatal = TRUE;
		}

		  
                                                       
     
		task_set_phys_footprint_limit_internal(p->task, (p->p_memstat_memlimit > 0) ? p->p_memstat_memlimit: -1, NULL, use_active, is_fatal);

		p = memorystatus_get_next_proc_locked(&b, p, TRUE);
	}

	memorystatus_highwater_enabled = enable;

	proc_list_unlock();

	return 0;
}

SYSCTL_PROC(_kern, OID_AUTO, memorystatus_highwater_enabled, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_highwater_enabled, 0, sysctl_memorystatus_highwater_enable, "I", "");

SYSCTL_INT(_kern, OID_AUTO, memorystatus_idle_snapshot, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_idle_snapshot, 0, "");

#if CONFIG_JETSAM
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_available_pages_critical, CTLFLAG_RD | CTLFLAG_LOCKED, &memorystatus_available_pages_critical, 0, "");
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_available_pages_critical_base, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_available_pages_critical_base, 0, "");
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_available_pages_critical_idle_offset, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_available_pages_critical_idle_offset, 0, "");
SYSCTL_UINT(_kern, OID_AUTO, memorystatus_policy_more_free_offset_pages, CTLFLAG_RW, &memorystatus_policy_more_free_offset_pages, 0, "");

static unsigned int memorystatus_jetsam_panic_debug = 0;

#if VM_PRESSURE_EVENTS

SYSCTL_UINT(_kern, OID_AUTO, memorystatus_available_pages_pressure, CTLFLAG_RW | CTLFLAG_LOCKED, &memorystatus_available_pages_pressure, 0, "");

#endif                         

#endif                    

#endif                           

extern kern_return_t kernel_thread_start_priority(thread_continue_t continuation,
    void *parameter,
    integer_t priority,
    thread_t *new_thread);

#if DEVELOPMENT || DEBUG

static int
sysctl_memorystatus_disconnect_page_mappings SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int     error = 0, pid = 0;
	proc_t  p;

	error = sysctl_handle_int(oidp, &pid, 0, req);
	if (error || !req->newptr) {
		return error;
	}

	lck_mtx_lock(&disconnect_page_mappings_mutex);

	if (pid == -1) {
		vm_pageout_disconnect_all_pages();
	} else {
		p = proc_find(pid);

		if (p != NULL) {
			error = task_disconnect_page_mappings(p->task);

			proc_rele(p);

			if (error) {
				error = EIO;
			}
		} else {
			error = EINVAL;
		}
	}
	lck_mtx_unlock(&disconnect_page_mappings_mutex);

	return error;
}

SYSCTL_PROC(_kern, OID_AUTO, memorystatus_disconnect_page_mappings, CTLTYPE_INT | CTLFLAG_WR | CTLFLAG_LOCKED | CTLFLAG_MASKED,
    0, 0, &sysctl_memorystatus_disconnect_page_mappings, "I", "");

#endif                           

  
                          
  
         
                                                    
                                                        
                                                         
               
  
                                             
   
static void
memorystatus_sort_bucket_locked(unsigned int bucket_index, int sort_order)
{
	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_OWNED);
	if (memstat_bucket[bucket_index].count == 0) {
		return;
	}

	switch (bucket_index) {
	case JETSAM_PRIORITY_FOREGROUND:
		if (memorystatus_sort_by_largest_coalition_locked(bucket_index, sort_order) == 0) {
			  
                                                                      
      
			memorystatus_sort_by_largest_process_locked(bucket_index);
		}
		break;
	default:
		memorystatus_sort_by_largest_process_locked(bucket_index);
		break;
	}
}

  
                                                              
  
         
                                                    
                                                        
                                                         
               
  
          
                   
                        
   
static int
memorystatus_sort_bucket(unsigned int bucket_index, int sort_order)
{
	int coal_sort_order;

	  
                              
    
	if (bucket_index >= MEMSTAT_BUCKET_COUNT) {
		return EINVAL;
	}

#if DEVELOPMENT || DEBUG
	if (sort_order == JETSAM_SORT_DEFAULT) {
		coal_sort_order = COALITION_SORT_DEFAULT;
	} else {
		coal_sort_order = sort_order;                                                
	}
#else
	                    
	if (sort_order == JETSAM_SORT_DEFAULT) {
		coal_sort_order = COALITION_SORT_DEFAULT;
	} else {
		return EINVAL;
	}
#endif

	proc_list_lock();
	memorystatus_sort_bucket_locked(bucket_index, coal_sort_order);
	proc_list_unlock();

	return 0;
}

  
                                                     
   

static void
memorystatus_sort_by_largest_process_locked(unsigned int bucket_index)
{
	proc_t p = NULL, insert_after_proc = NULL, max_proc = NULL;
	proc_t next_p = NULL, prev_max_proc = NULL;
	uint32_t pages = 0, max_pages = 0;
	memstat_bucket_t *current_bucket;

	if (bucket_index >= MEMSTAT_BUCKET_COUNT) {
		return;
	}

	current_bucket = &memstat_bucket[bucket_index];

	p = TAILQ_FIRST(&current_bucket->list);

	while (p) {
		memorystatus_get_task_page_counts(p->task, &pages, NULL, NULL);
		max_pages = pages;
		max_proc = p;
		prev_max_proc = p;

		while ((next_p = TAILQ_NEXT(p, p_memstat_list)) != NULL) {
			                                                        
			p = next_p;
			memorystatus_get_task_page_counts(p->task, &pages, NULL, NULL);
			if (pages > max_pages) {
				max_pages = pages;
				max_proc = p;
			}
		}

		if (prev_max_proc != max_proc) {
			                                                  
			TAILQ_REMOVE(&current_bucket->list, max_proc, p_memstat_list);
			if (insert_after_proc == NULL) {
				TAILQ_INSERT_HEAD(&current_bucket->list, max_proc, p_memstat_list);
			} else {
				TAILQ_INSERT_AFTER(&current_bucket->list, insert_after_proc, max_proc, p_memstat_list);
			}
			prev_max_proc = max_proc;
		}

		insert_after_proc = max_proc;

		p = TAILQ_NEXT(max_proc, p_memstat_list);
	}
}

proc_t
memorystatus_get_first_proc_locked(unsigned int *bucket_index, boolean_t search)
{
	memstat_bucket_t *current_bucket;
	proc_t next_p;

	if ((*bucket_index) >= MEMSTAT_BUCKET_COUNT) {
		return NULL;
	}

	current_bucket = &memstat_bucket[*bucket_index];
	next_p = TAILQ_FIRST(&current_bucket->list);
	if (!next_p && search) {
		while (!next_p && (++(*bucket_index) < MEMSTAT_BUCKET_COUNT)) {
			current_bucket = &memstat_bucket[*bucket_index];
			next_p = TAILQ_FIRST(&current_bucket->list);
		}
	}

	return next_p;
}

proc_t
memorystatus_get_next_proc_locked(unsigned int *bucket_index, proc_t p, boolean_t search)
{
	memstat_bucket_t *current_bucket;
	proc_t next_p;

	if (!p || ((*bucket_index) >= MEMSTAT_BUCKET_COUNT)) {
		return NULL;
	}

	next_p = TAILQ_NEXT(p, p_memstat_list);
	while (!next_p && search && (++(*bucket_index) < MEMSTAT_BUCKET_COUNT)) {
		current_bucket = &memstat_bucket[*bucket_index];
		next_p = TAILQ_FIRST(&current_bucket->list);
	}

	return next_p;
}

  
                                               
                                                   
                                     
   
struct jetsam_thread_state {
	uint8_t       inited;                                             
	uint8_t       limit_to_low_bands;              
	int           memorystatus_wakeup;                   
	int           index;                          
	thread_t      thread;                            
} *jetsam_threads;

                                              
#define JETSAM_THREADS_LIMIT   3

                                     
_Atomic int active_jetsam_threads = 1;

                                                 
int max_jetsam_threads = JETSAM_THREADS_LIMIT;

  
                                                         
                                                              
                     
                                                  
                                                
   
#if __AMP__
int fast_jetsam_enabled = 1;
#else              
int fast_jetsam_enabled = 0;
#endif              

#if CONFIG_DIRTYSTATUS_TRACKING
int dirtystatus_tracking_enabled = 0;
SYSCTL_INT(_kern, OID_AUTO, dirtystatus_tracking_enabled, CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &dirtystatus_tracking_enabled, 0, "");
#endif

                                                                              
static inline struct jetsam_thread_state *
jetsam_current_thread(void)
{
	for (int thr_id = 0; thr_id < max_jetsam_threads; thr_id++) {
		if (jetsam_threads[thr_id].thread == current_thread()) {
			return &(jetsam_threads[thr_id]);
		}
	}
	return NULL;
}


__private_extern__ void
memorystatus_init(void)
{
	kern_return_t result;
	int i;

#if CONFIG_FREEZE
	memorystatus_freeze_jetsam_band = JETSAM_PRIORITY_UI_SUPPORT;
	memorystatus_frozen_processes_max = FREEZE_PROCESSES_MAX;
	memorystatus_frozen_shared_mb_max = ((MAX_FROZEN_SHARED_MB_PERCENT * max_task_footprint_mb) / 100);                                        
	memorystatus_freeze_shared_mb_per_process_max = (memorystatus_frozen_shared_mb_max / 4);
	memorystatus_freeze_pages_min = FREEZE_PAGES_MIN;
	memorystatus_freeze_pages_max = FREEZE_PAGES_MAX;
	memorystatus_max_frozen_demotions_daily = MAX_FROZEN_PROCESS_DEMOTIONS;
	memorystatus_thaw_count_demotion_threshold = MIN_THAW_DEMOTION_THRESHOLD;
#endif

#if DEVELOPMENT || DEBUG
	if (kill_on_no_paging_space) {
		max_kill_priority = JETSAM_PRIORITY_MAX;
	}
#endif

	                  
	for (i = 0; i < MEMSTAT_BUCKET_COUNT; i++) {
		TAILQ_INIT(&memstat_bucket[i].list);
		memstat_bucket[i].count = 0;
		memstat_bucket[i].relaunch_high_count = 0;
	}
	memorystatus_idle_demotion_call = thread_call_allocate((thread_call_func_t)memorystatus_perform_idle_demotion, NULL);

	nanoseconds_to_absolutetime((uint64_t)DEFERRED_IDLE_EXIT_TIME_SECS * NSEC_PER_SEC, &memorystatus_sysprocs_idle_delay_time);
	nanoseconds_to_absolutetime((uint64_t)DEFERRED_IDLE_EXIT_TIME_SECS * NSEC_PER_SEC, &memorystatus_apps_idle_delay_time);

#if CONFIG_JETSAM
	                     
	if (!PE_parse_boot_argn("kern.jetsam_delta", &delta_percentage, sizeof(delta_percentage))) {
		PE_get_default("kern.jetsam_delta", &delta_percentage, sizeof(delta_percentage));
	}
	if (delta_percentage == 0) {
		delta_percentage = 5;
	}
	if (max_mem > config_jetsam_large_memory_cutoff) {
		critical_threshold_percentage = critical_threshold_percentage_larger_devices;
		delta_percentage = delta_percentage_larger_devices;
	}
	assert(delta_percentage < 100);
	if (!PE_parse_boot_argn("kern.jetsam_critical_threshold", &critical_threshold_percentage, sizeof(critical_threshold_percentage))) {
		PE_get_default("kern.jetsam_critical_threshold", &critical_threshold_percentage, sizeof(critical_threshold_percentage));
	}
	assert(critical_threshold_percentage < 100);
	PE_get_default("kern.jetsam_idle_offset", &idle_offset_percentage, sizeof(idle_offset_percentage));
	assert(idle_offset_percentage < 100);
	PE_get_default("kern.jetsam_pressure_threshold", &pressure_threshold_percentage, sizeof(pressure_threshold_percentage));
	assert(pressure_threshold_percentage < 100);
	PE_get_default("kern.jetsam_freeze_threshold", &freeze_threshold_percentage, sizeof(freeze_threshold_percentage));
	assert(freeze_threshold_percentage < 100);


	if (!PE_parse_boot_argn("jetsam_aging_policy", &jetsam_aging_policy,
	    sizeof(jetsam_aging_policy))) {
		if (!PE_get_default("kern.jetsam_aging_policy", &jetsam_aging_policy,
		    sizeof(jetsam_aging_policy))) {
			jetsam_aging_policy = kJetsamAgingPolicySysProcsReclaimedFirst;
		}
	}

	if (jetsam_aging_policy > kJetsamAgingPolicyMax) {
		jetsam_aging_policy = kJetsamAgingPolicySysProcsReclaimedFirst;
	}

	switch (jetsam_aging_policy) {
	case kJetsamAgingPolicyNone:
		system_procs_aging_band = JETSAM_PRIORITY_IDLE;
		applications_aging_band = JETSAM_PRIORITY_IDLE;
		break;

	case kJetsamAgingPolicyLegacy:
		  
                                                                 
                                                                    
                          
     
		system_procs_aging_band = JETSAM_PRIORITY_AGING_BAND1;
		applications_aging_band = JETSAM_PRIORITY_IDLE;
		break;

	case kJetsamAgingPolicySysProcsReclaimedFirst:
		system_procs_aging_band = JETSAM_PRIORITY_AGING_BAND1;
		applications_aging_band = JETSAM_PRIORITY_AGING_BAND2;
		break;

	case kJetsamAgingPolicyAppsReclaimedFirst:
		system_procs_aging_band = JETSAM_PRIORITY_AGING_BAND2;
		applications_aging_band = JETSAM_PRIORITY_AGING_BAND1;
		break;

	default:
		break;
	}

	  
                                                                             
                                                                                
                                                                              
                                                                   
    
	assert(JETSAM_PRIORITY_ELEVATED_INACTIVE > system_procs_aging_band);
	assert(JETSAM_PRIORITY_ELEVATED_INACTIVE > applications_aging_band);

	                                                                                
	if (!PE_parse_boot_argn("jetsam_idle_snapshot", &memorystatus_idle_snapshot, sizeof(memorystatus_idle_snapshot))) {
		                                              
		PE_get_default("kern.jetsam_idle_snapshot", &memorystatus_idle_snapshot, sizeof(memorystatus_idle_snapshot));
	}

	memorystatus_delta = (unsigned int) (delta_percentage * atop_64(max_mem) / 100);
	memorystatus_available_pages_critical_idle_offset = (unsigned int) (idle_offset_percentage * atop_64(max_mem) / 100);
	memorystatus_available_pages_critical_base = (unsigned int) ((critical_threshold_percentage / delta_percentage) * memorystatus_delta);
	memorystatus_policy_more_free_offset_pages = (unsigned int) ((policy_more_free_offset_percentage / delta_percentage) * memorystatus_delta);
	memorystatus_sysproc_aging_aggr_pages = (unsigned int) (sysproc_aging_aggr_threshold_percentage * atop_64(max_mem) / 100);

	                           
	if (max_mem <= (512 * 1024 * 1024)) {
		                    
		memorystatus_jld_eval_period_msecs = 8000;                                         
	} else {
		                            
		memorystatus_jld_eval_period_msecs = 6000;                                         
	}

	memorystatus_jld_enabled = TRUE;

	                                 
	memorystatus_update_levels_locked(FALSE);

#endif                    

#if __arm64__
	if (!PE_parse_boot_argn("entitled_max_task_pmem", &memorystatus_entitled_max_task_footprint_mb,
	    sizeof(memorystatus_entitled_max_task_footprint_mb))) {
		if (!PE_get_default("kern.entitled_max_task_pmem", &memorystatus_entitled_max_task_footprint_mb,
		    sizeof(memorystatus_entitled_max_task_footprint_mb))) {
			                                                          
			memorystatus_entitled_max_task_footprint_mb = 0;
		}
	}
	if (memorystatus_entitled_max_task_footprint_mb > max_mem / (1UL << 20) || memorystatus_entitled_max_task_footprint_mb < 0) {
		os_log_with_startup_serial(OS_LOG_DEFAULT, "Invalid value (%d) for entitled_max_task_pmem. Setting to 0",
		    memorystatus_entitled_max_task_footprint_mb);
	}
#endif                

	memorystatus_jetsam_snapshot_max = maxproc;

	memorystatus_jetsam_snapshot_size = sizeof(memorystatus_jetsam_snapshot_t) +
	    (sizeof(memorystatus_jetsam_snapshot_entry_t) * memorystatus_jetsam_snapshot_max);

	memorystatus_jetsam_snapshot = kalloc_flags(memorystatus_jetsam_snapshot_size, Z_WAITOK | Z_ZERO);
	if (!memorystatus_jetsam_snapshot) {
		panic("Could not allocate memorystatus_jetsam_snapshot");
	}

	memorystatus_jetsam_snapshot_copy = kalloc_flags(memorystatus_jetsam_snapshot_size, Z_WAITOK | Z_ZERO);
	if (!memorystatus_jetsam_snapshot_copy) {
		panic("Could not allocate memorystatus_jetsam_snapshot_copy");
	}

#if CONFIG_FREEZE
	memorystatus_jetsam_snapshot_freezer_max = memorystatus_jetsam_snapshot_max / JETSAM_SNAPSHOT_FREEZER_MAX_FACTOR;
	memorystatus_jetsam_snapshot_freezer_size = sizeof(memorystatus_jetsam_snapshot_t) +
	    (sizeof(memorystatus_jetsam_snapshot_entry_t) * memorystatus_jetsam_snapshot_freezer_max);

	memorystatus_jetsam_snapshot_freezer = kalloc_flags(memorystatus_jetsam_snapshot_freezer_size, Z_WAITOK | Z_ZERO);
	if (!memorystatus_jetsam_snapshot_freezer) {
		panic("Could not allocate memorystatus_jetsam_snapshot_freezer");
	}
#endif                    

	nanoseconds_to_absolutetime((uint64_t)JETSAM_SNAPSHOT_TIMEOUT_SECS * NSEC_PER_SEC, &memorystatus_jetsam_snapshot_timeout);

	memset(&memorystatus_at_boot_snapshot, 0, sizeof(memorystatus_jetsam_snapshot_t));

#if CONFIG_FREEZE
	memorystatus_freeze_threshold = (unsigned int) ((freeze_threshold_percentage / delta_percentage) * memorystatus_delta);
#endif

	                                                         
	if (!PE_parse_boot_argn("fast_jetsam_enabled", &fast_jetsam_enabled, sizeof(fast_jetsam_enabled))) {
		fast_jetsam_enabled = 0;
	}

	                                                                          
	if (!PE_parse_boot_argn("max_jetsam_threads", &max_jetsam_threads, sizeof(max_jetsam_threads))) {
		max_jetsam_threads = JETSAM_THREADS_LIMIT;
	}

	                                                                           
	if (max_jetsam_threads > JETSAM_THREADS_LIMIT) {
		max_jetsam_threads = JETSAM_THREADS_LIMIT;
	}

	                                                       
	if (vm_pageout_state.vm_restricted_to_single_processor == TRUE) {
		max_jetsam_threads = 1;
		fast_jetsam_enabled = 0;
	}

	                                               
	jetsam_threads = zalloc_permanent(sizeof(struct jetsam_thread_state) *
	    max_jetsam_threads, ZALIGN(struct jetsam_thread_state));

	                                       
	for (i = 0; i < max_jetsam_threads; i++) {
		jetsam_threads[i].inited = FALSE;
		jetsam_threads[i].index = i;
		result = kernel_thread_start_priority(memorystatus_thread, NULL, 95                    , &jetsam_threads[i].thread);
		if (result != KERN_SUCCESS) {
			panic("Could not create memorystatus_thread %d", i);
		}
		thread_deallocate(jetsam_threads[i].thread);
	}
}

                                                              
extern void
vm_run_compactor(void);
extern void
vm_wake_compactor_swapper(void);

  
                                 
                            
                                     
   
static int
jetsam_do_kill(proc_t p, int jetsam_flags, os_reason_t jetsam_reason)
{
	int error = 0;
	error = exit_with_reason(p, W_EXITCODE(0, SIGKILL), (int *)NULL, FALSE, FALSE, jetsam_flags, jetsam_reason);
	return error;
}

  
                                                          
   
static boolean_t
memorystatus_do_kill(proc_t p, uint32_t cause, os_reason_t jetsam_reason, uint64_t *footprint_of_killed_proc)
{
	int error = 0;
	__unused pid_t victim_pid = p->p_pid;
	uint64_t footprint = get_task_phys_footprint(p->task);
#if (KDEBUG_LEVEL >= KDEBUG_LEVEL_STANDARD)
	int32_t memstat_effectivepriority = p->p_memstat_effectivepriority;
#endif                                              

	KERNEL_DEBUG_CONSTANT((BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_DO_KILL)) | DBG_FUNC_START,
	    victim_pid, cause, vm_page_free_count, footprint, 0);
	DTRACE_MEMORYSTATUS4(memorystatus_do_kill, proc_t, p, os_reason_t, jetsam_reason, uint32_t, cause, uint64_t, footprint);
#if CONFIG_JETSAM && (DEVELOPMENT || DEBUG)
	if (memorystatus_jetsam_panic_debug & (1 << cause)) {
		panic("memorystatus_do_kill(): jetsam debug panic (cause: %d)", cause);
	}
#else
#pragma unused(cause)
#endif

	if (p->p_memstat_effectivepriority >= JETSAM_PRIORITY_FOREGROUND) {
		printf("memorystatus: killing process %d [%s] in high band %s (%d) - memorystatus_available_pages: %llu\n", p->p_pid,
		    (*p->p_name ? p->p_name : "unknown"),
		    memorystatus_priority_band_name(p->p_memstat_effectivepriority), p->p_memstat_effectivepriority,
		    (uint64_t)MEMORYSTATUS_LOG_AVAILABLE_PAGES);
	}

	  
                                                                                
                                                                                                               
    
	int jetsam_flags = P_LTERM_JETSAM;
	switch (cause) {
	case kMemorystatusKilledHiwat:                                          jetsam_flags |= P_JETSAM_HIWAT; break;
	case kMemorystatusKilledVnodes:                                         jetsam_flags |= P_JETSAM_VNODE; break;
	case kMemorystatusKilledVMPageShortage:                         jetsam_flags |= P_JETSAM_VMPAGESHORTAGE; break;
	case kMemorystatusKilledVMCompressorThrashing:
	case kMemorystatusKilledVMCompressorSpaceShortage:      jetsam_flags |= P_JETSAM_VMTHRASHING; break;
	case kMemorystatusKilledFCThrashing:                            jetsam_flags |= P_JETSAM_FCTHRASHING; break;
	case kMemorystatusKilledPerProcessLimit:                        jetsam_flags |= P_JETSAM_PID; break;
	case kMemorystatusKilledIdleExit:                                       jetsam_flags |= P_JETSAM_IDLEEXIT; break;
	}
	                                       
	os_reason_ref(jetsam_reason);
	error = jetsam_do_kill(p, jetsam_flags, jetsam_reason);
	*footprint_of_killed_proc = ((error == 0) ? footprint : 0);

	KERNEL_DEBUG_CONSTANT((BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_DO_KILL)) | DBG_FUNC_END,
	    victim_pid, memstat_effectivepriority, vm_page_free_count, error, 0);

	KERNEL_DEBUG_CONSTANT((BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_COMPACTOR_RUN)) | DBG_FUNC_START,
	    victim_pid, cause, vm_page_free_count, *footprint_of_killed_proc, 0);

	if (jetsam_reason->osr_code == JETSAM_REASON_VNODE) {
		  
                                                                    
                                                                                              
                                
                                                    
     
		vm_wake_compactor_swapper();
	} else {
		vm_run_compactor();
	}

	KERNEL_DEBUG_CONSTANT((BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_COMPACTOR_RUN)) | DBG_FUNC_END,
	    victim_pid, cause, vm_page_free_count, 0, 0);

	os_reason_free(jetsam_reason);
	return error == 0;
}

  
                    
   

static void
memorystatus_check_levels_locked(void)
{
#if CONFIG_JETSAM
	                   
	memorystatus_update_levels_locked(TRUE);
#else                    
	  
                                                
                                                         
    
#endif                    
}

  
                                                                                                     
                                                          
                                                                             
   

int
memorystatus_update_inactive_jetsam_priority_band(pid_t pid, uint32_t op_flags, int jetsam_prio, boolean_t effective_now)
{
	int error = 0;
	boolean_t enable = FALSE;
	proc_t  p = NULL;

	if (op_flags == MEMORYSTATUS_CMD_ELEVATED_INACTIVEJETSAMPRIORITY_ENABLE) {
		enable = TRUE;
	} else if (op_flags == MEMORYSTATUS_CMD_ELEVATED_INACTIVEJETSAMPRIORITY_DISABLE) {
		enable = FALSE;
	} else {
		return EINVAL;
	}

	p = proc_find(pid);
	if (p != NULL) {
		if ((enable && ((p->p_memstat_state & P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND) == P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND)) ||
		    (!enable && ((p->p_memstat_state & P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND) == 0))) {
			  
                         
      
		} else {
			proc_list_lock();

			if (enable) {
				p->p_memstat_state |= P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND;
				memorystatus_invalidate_idle_demotion_locked(p, TRUE);

				if (effective_now) {
					if (p->p_memstat_effectivepriority < jetsam_prio) {
						if (memorystatus_highwater_enabled) {
							  
                                             
                             
                             
          
							boolean_t is_fatal;
							boolean_t use_active = TRUE;
							CACHE_ACTIVE_LIMITS_LOCKED(p, is_fatal);
							task_set_phys_footprint_limit_internal(p->task, (p->p_memstat_memlimit > 0) ? p->p_memstat_memlimit : -1, NULL, use_active, is_fatal);
						}
						memorystatus_update_priority_locked(p, jetsam_prio, FALSE, FALSE);
					}
				} else {
					if (isProcessInAgingBands(p)) {
						memorystatus_update_priority_locked(p, JETSAM_PRIORITY_IDLE, FALSE, TRUE);
					}
				}
			} else {
				p->p_memstat_state &= ~P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND;
				memorystatus_invalidate_idle_demotion_locked(p, TRUE);

				if (effective_now) {
					if (p->p_memstat_effectivepriority == jetsam_prio) {
						memorystatus_update_priority_locked(p, JETSAM_PRIORITY_IDLE, FALSE, TRUE);
					}
				} else {
					if (isProcessInAgingBands(p)) {
						memorystatus_update_priority_locked(p, JETSAM_PRIORITY_IDLE, FALSE, TRUE);
					}
				}
			}

			proc_list_unlock();
		}
		proc_rele(p);
		error = 0;
	} else {
		error = ESRCH;
	}

	return error;
}

static void
memorystatus_perform_idle_demotion(__unused void *spare1, __unused void *spare2)
{
	proc_t p;
	uint64_t current_time = 0, idle_delay_time = 0;
	int demote_prio_band = 0;
	memstat_bucket_t *demotion_bucket;

	MEMORYSTATUS_DEBUG(1, "memorystatus_perform_idle_demotion()\n");

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_IDLE_DEMOTE) | DBG_FUNC_START, 0, 0, 0, 0, 0);

	current_time = mach_absolute_time();

	proc_list_lock();

	demote_prio_band = JETSAM_PRIORITY_IDLE + 1;

	for (; demote_prio_band < JETSAM_PRIORITY_MAX; demote_prio_band++) {
		if (demote_prio_band != system_procs_aging_band && demote_prio_band != applications_aging_band) {
			continue;
		}

		demotion_bucket = &memstat_bucket[demote_prio_band];
		p = TAILQ_FIRST(&demotion_bucket->list);

		while (p) {
			MEMORYSTATUS_DEBUG(1, "memorystatus_perform_idle_demotion() found %d\n", p->p_pid);

			assert(p->p_memstat_idledeadline);

			assert(p->p_memstat_dirty & P_DIRTY_AGING_IN_PROGRESS);

			if (current_time >= p->p_memstat_idledeadline) {
				if ((isSysProc(p) &&
				    ((p->p_memstat_dirty & (P_DIRTY_IDLE_EXIT_ENABLED | P_DIRTY_IS_DIRTY)) != P_DIRTY_IDLE_EXIT_ENABLED)) ||                              
				    task_has_assertions((struct task *)(p->task))) {                                                                               
					idle_delay_time = (isSysProc(p)) ? memorystatus_sysprocs_idle_time(p) : memorystatus_apps_idle_time(p);

					p->p_memstat_idledeadline += idle_delay_time;
					p = TAILQ_NEXT(p, p_memstat_list);
				} else {
					proc_t next_proc = NULL;

					next_proc = TAILQ_NEXT(p, p_memstat_list);
					memorystatus_invalidate_idle_demotion_locked(p, TRUE);

					memorystatus_update_priority_locked(p, JETSAM_PRIORITY_IDLE, false, true);

					p = next_proc;
					continue;
				}
			} else {
				                        
				break;
			}
		}
	}

	memorystatus_reschedule_idle_demotion_locked();

	proc_list_unlock();

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_IDLE_DEMOTE) | DBG_FUNC_END, 0, 0, 0, 0, 0);
}

static void
memorystatus_schedule_idle_demotion_locked(proc_t p, boolean_t set_state)
{
	boolean_t present_in_sysprocs_aging_bucket = FALSE;
	boolean_t present_in_apps_aging_bucket = FALSE;
	uint64_t  idle_delay_time = 0;

	if (jetsam_aging_policy == kJetsamAgingPolicyNone) {
		return;
	}

	if ((p->p_memstat_state & P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND) ||
	    (p->p_memstat_state & P_MEMSTAT_PRIORITY_ASSERTION)) {
		  
                                                                       
     
		return;
	}

	if (isProcessInAgingBands(p)) {
		if (jetsam_aging_policy != kJetsamAgingPolicyLegacy) {
			assert((p->p_memstat_dirty & P_DIRTY_AGING_IN_PROGRESS) != P_DIRTY_AGING_IN_PROGRESS);
		}

		if (isSysProc(p) && system_procs_aging_band) {
			present_in_sysprocs_aging_bucket = TRUE;
		} else if (isApp(p) && applications_aging_band) {
			present_in_apps_aging_bucket = TRUE;
		}
	}

	assert(!present_in_sysprocs_aging_bucket);
	assert(!present_in_apps_aging_bucket);

	MEMORYSTATUS_DEBUG(1, "memorystatus_schedule_idle_demotion_locked: scheduling demotion to idle band for pid %d (dirty:0x%x, set_state %d, demotions %d).\n",
	    p->p_pid, p->p_memstat_dirty, set_state, (memorystatus_scheduled_idle_demotions_sysprocs + memorystatus_scheduled_idle_demotions_apps));

	if (isSysProc(p)) {
		assert((p->p_memstat_dirty & P_DIRTY_IDLE_EXIT_ENABLED) == P_DIRTY_IDLE_EXIT_ENABLED);
	}

	idle_delay_time = (isSysProc(p)) ? memorystatus_sysprocs_idle_time(p) : memorystatus_apps_idle_time(p);
	if (set_state) {
		p->p_memstat_dirty |= P_DIRTY_AGING_IN_PROGRESS;
		p->p_memstat_idledeadline = mach_absolute_time() + idle_delay_time;
	}

	assert(p->p_memstat_idledeadline);

	if (isSysProc(p) && present_in_sysprocs_aging_bucket == FALSE) {
		memorystatus_scheduled_idle_demotions_sysprocs++;
	} else if (isApp(p) && present_in_apps_aging_bucket == FALSE) {
		memorystatus_scheduled_idle_demotions_apps++;
	}
}

void
memorystatus_invalidate_idle_demotion_locked(proc_t p, boolean_t clear_state)
{
	boolean_t present_in_sysprocs_aging_bucket = FALSE;
	boolean_t present_in_apps_aging_bucket = FALSE;

	if (!system_procs_aging_band && !applications_aging_band) {
		return;
	}

	if ((p->p_memstat_dirty & P_DIRTY_AGING_IN_PROGRESS) == 0) {
		return;
	}

	if (isProcessInAgingBands(p)) {
		if (jetsam_aging_policy != kJetsamAgingPolicyLegacy) {
			assert((p->p_memstat_dirty & P_DIRTY_AGING_IN_PROGRESS) == P_DIRTY_AGING_IN_PROGRESS);
		}

		if (isSysProc(p) && system_procs_aging_band) {
			assert(p->p_memstat_effectivepriority == system_procs_aging_band);
			assert(p->p_memstat_idledeadline);
			present_in_sysprocs_aging_bucket = TRUE;
		} else if (isApp(p) && applications_aging_band) {
			assert(p->p_memstat_effectivepriority == applications_aging_band);
			assert(p->p_memstat_idledeadline);
			present_in_apps_aging_bucket = TRUE;
		}
	}

	MEMORYSTATUS_DEBUG(1, "memorystatus_invalidate_idle_demotion(): invalidating demotion to idle band for pid %d (clear_state %d, demotions %d).\n",
	    p->p_pid, clear_state, (memorystatus_scheduled_idle_demotions_sysprocs + memorystatus_scheduled_idle_demotions_apps));


	if (clear_state) {
		p->p_memstat_idledeadline = 0;
		p->p_memstat_dirty &= ~P_DIRTY_AGING_IN_PROGRESS;
	}

	if (isSysProc(p) && present_in_sysprocs_aging_bucket == TRUE) {
		memorystatus_scheduled_idle_demotions_sysprocs--;
		assert(memorystatus_scheduled_idle_demotions_sysprocs >= 0);
	} else if (isApp(p) && present_in_apps_aging_bucket == TRUE) {
		memorystatus_scheduled_idle_demotions_apps--;
		assert(memorystatus_scheduled_idle_demotions_apps >= 0);
	}

	assert((memorystatus_scheduled_idle_demotions_sysprocs + memorystatus_scheduled_idle_demotions_apps) >= 0);
}

static void
memorystatus_reschedule_idle_demotion_locked(void)
{
	if (0 == (memorystatus_scheduled_idle_demotions_sysprocs + memorystatus_scheduled_idle_demotions_apps)) {
		if (memstat_idle_demotion_deadline) {
			                                            
			thread_call_cancel(memorystatus_idle_demotion_call);
			memstat_idle_demotion_deadline = 0;
		}
	} else {
		memstat_bucket_t *demotion_bucket;
		proc_t p = NULL, p1 = NULL, p2 = NULL;

		if (system_procs_aging_band) {
			demotion_bucket = &memstat_bucket[system_procs_aging_band];
			p1 = TAILQ_FIRST(&demotion_bucket->list);

			p = p1;
		}

		if (applications_aging_band) {
			demotion_bucket = &memstat_bucket[applications_aging_band];
			p2 = TAILQ_FIRST(&demotion_bucket->list);

			if (p1 && p2) {
				p = (p1->p_memstat_idledeadline > p2->p_memstat_idledeadline) ? p2 : p1;
			} else {
				p = (p1 == NULL) ? p2 : p1;
			}
		}

		assert(p);

		if (p != NULL) {
			assert(p && p->p_memstat_idledeadline);
			if (memstat_idle_demotion_deadline != p->p_memstat_idledeadline) {
				thread_call_enter_delayed(memorystatus_idle_demotion_call, p->p_memstat_idledeadline);
				memstat_idle_demotion_deadline = p->p_memstat_idledeadline;
			}
		}
	}
}

  
                    
   

int
memorystatus_add(proc_t p, boolean_t locked)
{
	memstat_bucket_t *bucket;

	MEMORYSTATUS_DEBUG(1, "memorystatus_list_add(): adding pid %d with priority %d.\n", p->p_pid, p->p_memstat_effectivepriority);

	if (!locked) {
		proc_list_lock();
	}

	DTRACE_MEMORYSTATUS2(memorystatus_add, proc_t, p, int32_t, p->p_memstat_effectivepriority);

	                                                            
	if (p->p_memstat_state & P_MEMSTAT_INTERNAL) {
		goto exit;
	}

	  
                                                          
                                                                                                  
    
	if (isSysProc(p)) {
		p->p_memstat_state |= P_MEMSTAT_FREEZE_DISABLED;
	}
#if CONFIG_FREEZE
	memorystatus_freeze_init_proc(p);
#endif

	bucket = &memstat_bucket[p->p_memstat_effectivepriority];

	if (isSysProc(p) && system_procs_aging_band && (p->p_memstat_effectivepriority == system_procs_aging_band)) {
		assert(bucket->count == memorystatus_scheduled_idle_demotions_sysprocs - 1);
	} else if (isApp(p) && applications_aging_band && (p->p_memstat_effectivepriority == applications_aging_band)) {
		assert(bucket->count == memorystatus_scheduled_idle_demotions_apps - 1);
	} else if (p->p_memstat_effectivepriority == JETSAM_PRIORITY_IDLE) {
		  
                            
                            
     
		p->p_memstat_idle_start = mach_absolute_time();
	}

	TAILQ_INSERT_TAIL(&bucket->list, p, p_memstat_list);
	bucket->count++;
	if (p->p_memstat_relaunch_flags & (P_MEMSTAT_RELAUNCH_HIGH)) {
		bucket->relaunch_high_count++;
	}

	memorystatus_list_count++;

	memorystatus_check_levels_locked();

exit:
	if (!locked) {
		proc_list_unlock();
	}

	return 0;
}

  
               
                                                     
                                                 
  
                                                       
                                                
  
                       
                                                  
                                                 
                                                                 
                                                     
  
                                              
                                                                   
      
                                                                           
                                                                             
                                                                          
                                                                          
                                                  
   
void
memorystatus_update_priority_locked(proc_t p, int priority, boolean_t head_insert, boolean_t skip_demotion_check)
{
	memstat_bucket_t *old_bucket, *new_bucket;

	assert(priority < MEMSTAT_BUCKET_COUNT);

	                                                                                            
	if ((p->p_listflag & P_LIST_EXITED) != 0) {
		return;
	}

	MEMORYSTATUS_DEBUG(1, "memorystatus_update_priority_locked(): setting %s(%d) to priority %d, inserting at %s\n",
	    (*p->p_name ? p->p_name : "unknown"), p->p_pid, priority, head_insert ? "head" : "tail");

	DTRACE_MEMORYSTATUS3(memorystatus_update_priority, proc_t, p, int32_t, p->p_memstat_effectivepriority, int, priority);

	old_bucket = &memstat_bucket[p->p_memstat_effectivepriority];

	if (skip_demotion_check == FALSE) {
		if (isSysProc(p)) {
			  
                                                                                          
                                                                           
                                                                                                     
      

			if (p->p_memstat_state & P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND) {
				  
                                                                            
                                                                             
         
                                                                                                   
       
#if CONFIG_FREEZE
				if (p->p_memstat_state & P_MEMSTAT_FROZEN) {
					if (priority <= memorystatus_freeze_jetsam_band) {
						priority = memorystatus_freeze_jetsam_band;
					}
				} else
#endif                    
				{
					if (priority <= JETSAM_PRIORITY_ELEVATED_INACTIVE) {
						priority = JETSAM_PRIORITY_ELEVATED_INACTIVE;
					}
				}
				assert(!(p->p_memstat_dirty & P_DIRTY_AGING_IN_PROGRESS));
			}
		} else if (isApp(p)) {
			  
                                                                                      
                                                                                                 
                                                                                                  
      

			if (p->p_memstat_state & P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND) {
#if CONFIG_FREEZE
				if (p->p_memstat_state & P_MEMSTAT_FROZEN) {
					if (priority <= memorystatus_freeze_jetsam_band) {
						priority = memorystatus_freeze_jetsam_band;
					}
				} else
#endif                    
				{
					if (priority <= JETSAM_PRIORITY_ELEVATED_INACTIVE) {
						priority = JETSAM_PRIORITY_ELEVATED_INACTIVE;
					}
				}
			} else {
				if (applications_aging_band) {
					if (p->p_memstat_effectivepriority == applications_aging_band) {
						assert(old_bucket->count == (memorystatus_scheduled_idle_demotions_apps + 1));
					}

					if ((jetsam_aging_policy != kJetsamAgingPolicyLegacy) && (priority <= applications_aging_band)) {
						assert(!(p->p_memstat_dirty & P_DIRTY_AGING_IN_PROGRESS));
						priority = applications_aging_band;
						memorystatus_schedule_idle_demotion_locked(p, TRUE);
					}
				}
			}
		}
	}

	if ((system_procs_aging_band && (priority == system_procs_aging_band)) || (applications_aging_band && (priority == applications_aging_band))) {
		assert(p->p_memstat_dirty & P_DIRTY_AGING_IN_PROGRESS);
	}

#if DEVELOPMENT || DEBUG
	if (priority == JETSAM_PRIORITY_IDLE &&                                                      
	    skip_demotion_check == FALSE &&                                                                         
	    (p->p_memstat_dirty & P_DIRTY_TRACK) &&                                          
	    ((p->p_memstat_memlimit != p->p_memstat_memlimit_inactive) ||                                                                            
	    ((p->p_memstat_state & P_MEMSTAT_MEMLIMIT_INACTIVE_FATAL) ? (!(p->p_memstat_state & P_MEMSTAT_FATAL_MEMLIMIT)) : (p->p_memstat_state & P_MEMSTAT_FATAL_MEMLIMIT)))) {                                   
		printf("memorystatus_update_priority_locked: on %s with 0x%x, prio: %d and %d\n", p->p_name, p->p_memstat_state, priority, p->p_memstat_memlimit);                              
	}
#endif                           

	TAILQ_REMOVE(&old_bucket->list, p, p_memstat_list);
	old_bucket->count--;
	if (p->p_memstat_relaunch_flags & (P_MEMSTAT_RELAUNCH_HIGH)) {
		old_bucket->relaunch_high_count--;
	}

	new_bucket = &memstat_bucket[priority];
	if (head_insert) {
		TAILQ_INSERT_HEAD(&new_bucket->list, p, p_memstat_list);
	} else {
		TAILQ_INSERT_TAIL(&new_bucket->list, p, p_memstat_list);
	}
	new_bucket->count++;
	if (p->p_memstat_relaunch_flags & (P_MEMSTAT_RELAUNCH_HIGH)) {
		new_bucket->relaunch_high_count++;
	}

	if (memorystatus_highwater_enabled) {
		boolean_t is_fatal;
		boolean_t use_active;

		  
                                                     
                                                
     
		boolean_t ledger_update_needed = TRUE;

		  
                                                             
                              
                              
                         
         
                                      
    
                                                                             
                                                                   
                            
     

		if (p->p_memstat_dirty & P_DIRTY_TRACK) {
			if (skip_demotion_check == TRUE && priority == JETSAM_PRIORITY_IDLE) {
				CACHE_INACTIVE_LIMITS_LOCKED(p, is_fatal);
				use_active = FALSE;
			} else {
				ledger_update_needed = FALSE;
			}
		} else if ((priority >= JETSAM_PRIORITY_FOREGROUND) && (p->p_memstat_effectivepriority < JETSAM_PRIORITY_FOREGROUND)) {
			  
                              
                         
                              
      
			CACHE_ACTIVE_LIMITS_LOCKED(p, is_fatal);
			use_active = TRUE;
		} else if ((priority < JETSAM_PRIORITY_FOREGROUND) && (p->p_memstat_effectivepriority >= JETSAM_PRIORITY_FOREGROUND)) {
			  
                              
                         
                                
      
			CACHE_INACTIVE_LIMITS_LOCKED(p, is_fatal);
			use_active = FALSE;
		} else {
			  
                                                                   
                                       
                                                           
                                                                  
      
			ledger_update_needed = FALSE;
		}

		  
                                                    
     
		if (ledger_update_needed) {
			task_set_phys_footprint_limit_internal(p->task, (p->p_memstat_memlimit > 0) ? p->p_memstat_memlimit : -1, NULL, use_active, is_fatal);

			MEMORYSTATUS_DEBUG(3, "memorystatus_update_priority_locked: new limit on pid %d (%dMB %s) priority old --> new (%d --> %d) dirty?=0x%x %s\n",
			    p->p_pid, (p->p_memstat_memlimit > 0 ? p->p_memstat_memlimit : -1),
			    (p->p_memstat_state & P_MEMSTAT_FATAL_MEMLIMIT ? "F " : "NF"), p->p_memstat_effectivepriority, priority, p->p_memstat_dirty,
			    (p->p_memstat_dirty ? ((p->p_memstat_dirty & P_DIRTY) ? "isdirty" : "isclean") : ""));
		}
	}

	  
                                    
    
	if (p->p_memstat_effectivepriority == priority) {
		  
                                              
                                          
     
	} else if (p->p_memstat_effectivepriority == JETSAM_PRIORITY_IDLE) {
		uint64_t now;
		  
                                                   
                       
     
		assert(p->p_memstat_idle_start != 0);
		now = mach_absolute_time();
		if (now > p->p_memstat_idle_start) {
			p->p_memstat_idle_delta = now - p->p_memstat_idle_start;
		}

		  
                                                                 
                                                                    
     
		if (p->p_memstat_state & P_MEMSTAT_FREEZE_IGNORE) {
			p->p_memstat_state &= ~P_MEMSTAT_FREEZE_IGNORE;
		}
	} else if (priority == JETSAM_PRIORITY_IDLE) {
		  
                                                 
                       
     
		p->p_memstat_idle_start = mach_absolute_time();
	}

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_CHANGE_PRIORITY), p->p_pid, priority, p->p_memstat_effectivepriority, 0, 0);

	p->p_memstat_effectivepriority = priority;

#if CONFIG_SECLUDED_MEMORY
	if (secluded_for_apps &&
	    task_could_use_secluded_mem(p->task)) {
		task_set_can_use_secluded_mem(
			p->task,
			(priority >= JETSAM_PRIORITY_FOREGROUND));
	}
#endif                             

	memorystatus_check_levels_locked();
}

int
memorystatus_relaunch_flags_update(proc_t p, int relaunch_flags)
{
	p->p_memstat_relaunch_flags = relaunch_flags;
	KDBG(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_RELAUNCH_FLAGS), p->p_pid, relaunch_flags, 0, 0, 0);
	return 0;
}

  
  
                                                                                           
  
              
                                            
                                             
                                                       
                                                                         
                                                                       
                                                                                      
  
                                                                                
                                                                 
                                           
  
                                                                                       
                                                                     
  
                                                                                
                                                                   
                                           
  
                                                                                           
                                                                      
  
                         
                 
   

int
memorystatus_update(proc_t p, int priority, uint64_t user_data, boolean_t is_assertion, boolean_t effective, boolean_t update_memlimit,
    int32_t memlimit_active, boolean_t memlimit_active_is_fatal,
    int32_t memlimit_inactive, boolean_t memlimit_inactive_is_fatal)
{
	int ret;
	boolean_t head_insert = false;

	MEMORYSTATUS_DEBUG(1, "memorystatus_update: changing (%s) pid %d: priority %d, user_data 0x%llx\n", (*p->p_name ? p->p_name : "unknown"), p->p_pid, priority, user_data);

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_UPDATE) | DBG_FUNC_START, p->p_pid, priority, user_data, effective, 0);

	if (priority == -1) {
		                                           
		priority = JETSAM_PRIORITY_DEFAULT;
	} else if ((priority == system_procs_aging_band) || (priority == applications_aging_band)) {
		                                                                                                       
		priority = JETSAM_PRIORITY_IDLE;
	} else if (priority == JETSAM_PRIORITY_IDLE_HEAD) {
		                                                                     
		priority = JETSAM_PRIORITY_IDLE;
		head_insert = TRUE;
	} else if ((priority < 0) || (priority >= MEMSTAT_BUCKET_COUNT)) {
		                  
		ret = EINVAL;
		goto out;
	}

	proc_list_lock();

	assert(!(p->p_memstat_state & P_MEMSTAT_INTERNAL));

	if (effective && (p->p_memstat_state & P_MEMSTAT_PRIORITYUPDATED)) {
		ret = EALREADY;
		proc_list_unlock();
		MEMORYSTATUS_DEBUG(1, "memorystatus_update: effective change specified for pid %d, but change already occurred.\n", p->p_pid);
		goto out;
	}

	if ((p->p_memstat_state & P_MEMSTAT_TERMINATED) || ((p->p_listflag & P_LIST_EXITED) != 0)) {
		  
                                                                                            
     
		ret = EBUSY;
		proc_list_unlock();
		goto out;
	}

	p->p_memstat_state |= P_MEMSTAT_PRIORITYUPDATED;
	p->p_memstat_userdata = user_data;

	if (is_assertion) {
		if (priority == JETSAM_PRIORITY_IDLE) {
			  
                                                                        
      
			if (p->p_memstat_state & P_MEMSTAT_PRIORITY_ASSERTION) {
				  
                                                                 
       
				p->p_memstat_state &= ~P_MEMSTAT_PRIORITY_ASSERTION;
			} else {
				  
                                                               
                                                             
                                                                  
       
				os_log(OS_LOG_DEFAULT, "memorystatus: Ignore assertion driven idle priority. Process not previously controlled %s:%d\n",
				    (*p->p_name ? p->p_name : "unknown"), p->p_pid);

				ret = 0;
				proc_list_unlock();
				goto out;
			}
		} else {
			  
                                                 
      
			p->p_memstat_state |= P_MEMSTAT_PRIORITY_ASSERTION;
		}

		                                                       

		p->p_memstat_assertionpriority = priority;

		int memstat_dirty_flags = memorystatus_dirty_get(p, TRUE);                              

		if (memstat_dirty_flags != 0) {
			  
                                                                                 
      
			int maxpriority;
			if (memstat_dirty_flags & PROC_DIRTY_IS_DIRTY) {
				maxpriority = MAX(p->p_memstat_assertionpriority, p->p_memstat_requestedpriority);
			} else {
				           

				if (memstat_dirty_flags & PROC_DIRTY_ALLOWS_IDLE_EXIT) {
					  
                                                                                  
                                                                                
                                       
        

					int newpriority = JETSAM_PRIORITY_IDLE;
					if ((p->p_memstat_dirty & (P_DIRTY_IDLE_EXIT_ENABLED | P_DIRTY_IS_DIRTY)) == P_DIRTY_IDLE_EXIT_ENABLED) {
						newpriority = (p->p_memstat_dirty & P_DIRTY_AGING_IN_PROGRESS) ? system_procs_aging_band : JETSAM_PRIORITY_IDLE;
					}

					maxpriority = MAX(p->p_memstat_assertionpriority, newpriority );

					if (newpriority == system_procs_aging_band) {
						memorystatus_schedule_idle_demotion_locked(p, FALSE);
					}
				} else {
					  
                                                                                     
        
					maxpriority = MAX(p->p_memstat_assertionpriority, p->p_memstat_requestedpriority);
				}
			}
			priority = maxpriority;
		}
	} else {
		p->p_memstat_requestedpriority = priority;
	}

	if (update_memlimit) {
		boolean_t is_fatal;
		boolean_t use_active;

		  
                                                                                 
                                                                               
                                                                
     

		MEMORYSTATUS_DEBUG(3, "memorystatus_update(enter): pid %d, priority %d, dirty=0x%x, Active(%dMB %s), Inactive(%dMB, %s)\n",
		    p->p_pid, priority, p->p_memstat_dirty,
		    memlimit_active, (memlimit_active_is_fatal ? "F " : "NF"),
		    memlimit_inactive, (memlimit_inactive_is_fatal ? "F " : "NF"));

		if (memlimit_active <= 0) {
			  
                                                                  
                                             
                                                             
                                           
      

			  
                                                                        
                                                                          
                                  
      

			if (memlimit_active < 0) {
				memlimit_active = -1;                                       
			}
			memlimit_active_is_fatal = TRUE;
		}

		if (memlimit_inactive <= 0) {
			  
                                                                    
                                             
      

			memlimit_inactive = -1;
			memlimit_inactive_is_fatal = TRUE;
		}

		  
                                                           
     
		SET_ACTIVE_LIMITS_LOCKED(p, memlimit_active, memlimit_active_is_fatal);

		  
                                                             
     
		SET_INACTIVE_LIMITS_LOCKED(p, memlimit_inactive, memlimit_inactive_is_fatal);

		  
                                                     
                                                             
                                                       
                                            
                                                   
     

		if (proc_jetsam_state_is_active_locked(p) == TRUE) {
			CACHE_ACTIVE_LIMITS_LOCKED(p, is_fatal);
			use_active = TRUE;
		} else {
			CACHE_INACTIVE_LIMITS_LOCKED(p, is_fatal);
			use_active = FALSE;
		}

		  
                                                       
     
		if (memorystatus_highwater_enabled) {
			               
			task_set_phys_footprint_limit_internal(p->task, ((p->p_memstat_memlimit > 0) ? p->p_memstat_memlimit : -1), NULL, use_active, is_fatal);

			MEMORYSTATUS_DEBUG(3, "memorystatus_update: init: limit on pid %d (%dMB %s) targeting priority(%d) dirty?=0x%x %s\n",
			    p->p_pid, (p->p_memstat_memlimit > 0 ? p->p_memstat_memlimit : -1),
			    (p->p_memstat_state & P_MEMSTAT_FATAL_MEMLIMIT ? "F " : "NF"), priority, p->p_memstat_dirty,
			    (p->p_memstat_dirty ? ((p->p_memstat_dirty & P_DIRTY) ? "isdirty" : "isclean") : ""));
		}
	}

	  
                                                 
                                                    
                                           
    

	if (isProcessInAgingBands(p)) {
		if ((jetsam_aging_policy != kJetsamAgingPolicyLegacy) && isApp(p) && (priority > applications_aging_band)) {
			  
                                                                           
                                                                         
                                 
     
                                                                                    
                                                                                
                                                                                 
                                                                                    
                                                                                  
                    
      
			memorystatus_invalidate_idle_demotion_locked(p, TRUE);
		} else {
			memorystatus_invalidate_idle_demotion_locked(p, FALSE);
		}
		memorystatus_update_priority_locked(p, JETSAM_PRIORITY_IDLE, FALSE, TRUE);
	} else {
		if (jetsam_aging_policy == kJetsamAgingPolicyLegacy && priority == JETSAM_PRIORITY_IDLE) {
			  
                                                                                 
                                                                                          
                                                                                        
                                                                                  
                                                                                    
                                                                 
      
			memorystatus_update_priority_locked(p, priority, head_insert, TRUE);
		}
	}

	memorystatus_update_priority_locked(p, priority, head_insert, FALSE);

	proc_list_unlock();
	ret = 0;

out:
	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_UPDATE) | DBG_FUNC_END, ret, 0, 0, 0, 0);

	return ret;
}

int
memorystatus_remove(proc_t p)
{
	int ret;
	memstat_bucket_t *bucket;
	boolean_t       reschedule = FALSE;

	MEMORYSTATUS_DEBUG(1, "memorystatus_list_remove: removing pid %d\n", p->p_pid);

	  
                                                                     
                                                              
    
	if (p->p_memstat_state & P_MEMSTAT_LOCKED) {
		return EAGAIN;
	}

	assert(!(p->p_memstat_state & P_MEMSTAT_INTERNAL));

	bucket = &memstat_bucket[p->p_memstat_effectivepriority];

	if (isSysProc(p) && system_procs_aging_band && (p->p_memstat_effectivepriority == system_procs_aging_band)) {
		assert(bucket->count == memorystatus_scheduled_idle_demotions_sysprocs);
		reschedule = TRUE;
	} else if (isApp(p) && applications_aging_band && (p->p_memstat_effectivepriority == applications_aging_band)) {
		assert(bucket->count == memorystatus_scheduled_idle_demotions_apps);
		reschedule = TRUE;
	}

	  
                     
    

	if (p->p_memstat_effectivepriority == JETSAM_PRIORITY_IDLE) {
		uint64_t now = mach_absolute_time();
		if (now > p->p_memstat_idle_start) {
			p->p_memstat_idle_delta = now - p->p_memstat_idle_start;
		}
	}

	TAILQ_REMOVE(&bucket->list, p, p_memstat_list);
	bucket->count--;
	if (p->p_memstat_relaunch_flags & (P_MEMSTAT_RELAUNCH_HIGH)) {
		bucket->relaunch_high_count--;
	}

	memorystatus_list_count--;

	                                                     
	if (reschedule) {
		memorystatus_invalidate_idle_demotion_locked(p, TRUE);
		memorystatus_reschedule_idle_demotion_locked();
	}

	memorystatus_check_levels_locked();

#if CONFIG_FREEZE
	if (p->p_memstat_state & (P_MEMSTAT_FROZEN)) {
		if (p->p_memstat_state & P_MEMSTAT_REFREEZE_ELIGIBLE) {
			p->p_memstat_state &= ~P_MEMSTAT_REFREEZE_ELIGIBLE;
			memorystatus_refreeze_eligible_count--;
		}

		memorystatus_frozen_count--;
		memorystatus_frozen_shared_mb -= p->p_memstat_freeze_sharedanon_pages;
		p->p_memstat_freeze_sharedanon_pages = 0;
	}

	if (p->p_memstat_state & P_MEMSTAT_SUSPENDED) {
		memorystatus_suspended_count--;
	}
#endif

#if DEVELOPMENT || DEBUG
	if (p->p_pid == memorystatus_testing_pid) {
		memorystatus_testing_pid = 0;
	}
#endif                           

	if (p) {
		ret = 0;
	} else {
		ret = ESRCH;
	}

	return ret;
}

  
                                                    
  
          
                   
                        
  
                                            
   

static int
memorystatus_validate_track_flags(struct proc *target_p, uint32_t pcontrol)
{
	                                                       
	if (target_p->p_memstat_dirty & P_DIRTY_TERMINATED) {
		return EBUSY;
	}

	                                                
	if ((pcontrol & PROC_DIRTY_ALLOW_IDLE_EXIT) &&
	    !(pcontrol & PROC_DIRTY_TRACK)) {
		return EINVAL;
	}

	                                                                                          
	if ((pcontrol & PROC_DIRTY_LAUNCH_IN_PROGRESS) &&
	    !(pcontrol & PROC_DIRTY_TRACK)) {
		return EINVAL;
	}

	                                                
	if ((pcontrol & PROC_DIRTY_DEFER) &&
	    (pcontrol & PROC_DIRTY_DEFER_ALWAYS)) {
		return EINVAL;
	}

	                                                         
	if (((pcontrol & PROC_DIRTY_DEFER) ||
	    (pcontrol & PROC_DIRTY_DEFER_ALWAYS)) &&
	    !(pcontrol & PROC_DIRTY_ALLOWS_IDLE_EXIT)) {
		return EINVAL;
	}

	return 0;
}

static void
memorystatus_update_idle_priority_locked(proc_t p)
{
	int32_t priority;

	MEMORYSTATUS_DEBUG(1, "memorystatus_update_idle_priority_locked(): pid %d dirty 0x%X\n", p->p_pid, p->p_memstat_dirty);

	assert(isSysProc(p));

	if ((p->p_memstat_dirty & (P_DIRTY_IDLE_EXIT_ENABLED | P_DIRTY_IS_DIRTY)) == P_DIRTY_IDLE_EXIT_ENABLED) {
		priority = (p->p_memstat_dirty & P_DIRTY_AGING_IN_PROGRESS) ? system_procs_aging_band : JETSAM_PRIORITY_IDLE;
	} else {
		priority = p->p_memstat_requestedpriority;
	}

	if (p->p_memstat_state & P_MEMSTAT_PRIORITY_ASSERTION) {
		  
                                                                
                                          
     
		if (p->p_memstat_assertionpriority > priority) {
			os_log(OS_LOG_DEFAULT, "memorystatus: assertion priority %d overrides priority %d for %s:%d\n",
			    p->p_memstat_assertionpriority, priority,
			    (*p->p_name ? p->p_name : "unknown"), p->p_pid);
			priority = p->p_memstat_assertionpriority;
		}
	}

	if (priority != p->p_memstat_effectivepriority) {
		if ((jetsam_aging_policy == kJetsamAgingPolicyLegacy) &&
		    (priority == JETSAM_PRIORITY_IDLE)) {
			  
                                                                  
                                                                  
                                                                    
                                                                  
                                                                     
                                                       
      

			if (p->p_memstat_state & P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND) {
				  
                                                                      
                                                   
                                                        
                                          
       

				priority = JETSAM_PRIORITY_ELEVATED_INACTIVE;
			}

			memorystatus_update_priority_locked(p, priority, false, true);
		} else {
			memorystatus_update_priority_locked(p, priority, false, false);
		}
	}
}

  
                                                                                                              
                                                                                                                  
                                                                                                                
                                                                                            
  
                                                                                                                      
                                                                                                                             
                                                                                                                    
                                                                                                                    
                                                                            
  
                                                                                                                     
                                                                                                                   
                                                                                             
   

int
memorystatus_dirty_track(proc_t p, uint32_t pcontrol)
{
	unsigned int old_dirty;
	boolean_t reschedule = FALSE;
	boolean_t already_deferred = FALSE;
	boolean_t defer_now = FALSE;
	int ret = 0;

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_DIRTY_TRACK),
	    p->p_pid, p->p_memstat_dirty, pcontrol, 0, 0);

	proc_list_lock();

	if ((p->p_listflag & P_LIST_EXITED) != 0) {
		  
                               
     
		ret = EBUSY;
		goto exit;
	}

	if (p->p_memstat_state & P_MEMSTAT_INTERNAL) {
		ret = EPERM;
		goto exit;
	}

	if ((ret = memorystatus_validate_track_flags(p, pcontrol)) != 0) {
		            
		goto exit;
	}

	old_dirty = p->p_memstat_dirty;

	                                                                 
	if (pcontrol & PROC_DIRTY_TRACK) {
		p->p_memstat_dirty |= P_DIRTY_TRACK;
	}

	if (pcontrol & PROC_DIRTY_ALLOW_IDLE_EXIT) {
		p->p_memstat_dirty |= P_DIRTY_ALLOW_IDLE_EXIT;
	}

	if (pcontrol & PROC_DIRTY_LAUNCH_IN_PROGRESS) {
		p->p_memstat_dirty |= P_DIRTY_LAUNCH_IN_PROGRESS;
	}

	if (old_dirty & P_DIRTY_AGING_IN_PROGRESS) {
		already_deferred = TRUE;
	}


	                                               
	if (pcontrol & (PROC_DIRTY_DEFER | PROC_DIRTY_DEFER_ALWAYS)) {
		if ((pcontrol & (PROC_DIRTY_DEFER)) &&
		    !(old_dirty & P_DIRTY_DEFER)) {
			p->p_memstat_dirty |= P_DIRTY_DEFER;
		}

		if ((pcontrol & (PROC_DIRTY_DEFER_ALWAYS)) &&
		    !(old_dirty & P_DIRTY_DEFER_ALWAYS)) {
			p->p_memstat_dirty |= P_DIRTY_DEFER_ALWAYS;
		}

		defer_now = TRUE;
	}

	MEMORYSTATUS_DEBUG(1, "memorystatus_on_track_dirty(): set idle-exit %s / defer %s / dirty %s for pid %d\n",
	    ((p->p_memstat_dirty & P_DIRTY_IDLE_EXIT_ENABLED) == P_DIRTY_IDLE_EXIT_ENABLED) ? "Y" : "N",
	    defer_now ? "Y" : "N",
	    p->p_memstat_dirty & P_DIRTY ? "Y" : "N",
	    p->p_pid);

	                                                                                   
	if (!(p->p_memstat_dirty & P_DIRTY_IS_DIRTY)) {
		if ((p->p_memstat_dirty & P_DIRTY_IDLE_EXIT_ENABLED) == P_DIRTY_IDLE_EXIT_ENABLED) {
			if (defer_now && !already_deferred) {
				  
                                                                
                                                                 
                  
       
				memorystatus_schedule_idle_demotion_locked(p, TRUE);
				reschedule = TRUE;
			} else if (!defer_now) {
				  
                                                         
                           
       

				if (already_deferred) {
					  
                                                  
                                                
                                        
        

					memorystatus_invalidate_idle_demotion_locked(p, TRUE);
				} else {
					  
                                                       
                                                           
                                                            
        

					memorystatus_schedule_idle_demotion_locked(p, TRUE);
				}

				reschedule = TRUE;
			}
		}
	} else {
		  
                                                                         
                                                                       
                           
    
                                        
                                                   
                                                  
                                                                    
                                                       
    
                                                      
                                                              
                                                                            
                                      
                                                                                                       
                                                                            
                                            
    
                                               
                                                                             
     

		if (!defer_now && already_deferred) {
			memorystatus_invalidate_idle_demotion_locked(p, TRUE);
			reschedule = TRUE;
		} else {
			boolean_t reset_state = (jetsam_aging_policy != kJetsamAgingPolicyLegacy) ? TRUE : FALSE;

			memorystatus_invalidate_idle_demotion_locked(p, reset_state);
			reschedule = TRUE;
		}
	}

	memorystatus_update_idle_priority_locked(p);

	if (reschedule) {
		memorystatus_reschedule_idle_demotion_locked();
	}

	ret = 0;

exit:
	proc_list_unlock();

	return ret;
}

int
memorystatus_dirty_set(proc_t p, boolean_t self, uint32_t pcontrol)
{
	int ret;
	boolean_t kill = false;
	boolean_t reschedule = FALSE;
	boolean_t was_dirty = FALSE;
	boolean_t now_dirty = FALSE;
#if CONFIG_DIRTYSTATUS_TRACKING
	boolean_t notify_change = FALSE;
	dirty_status_change_event_t change_event;
#endif

	MEMORYSTATUS_DEBUG(1, "memorystatus_dirty_set(): %d %d 0x%x 0x%x\n", self, p->p_pid, pcontrol, p->p_memstat_dirty);
	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_DIRTY_SET), p->p_pid, self, pcontrol, 0, 0);

	proc_list_lock();

	if ((p->p_listflag & P_LIST_EXITED) != 0) {
		  
                               
     
		ret = EBUSY;
		goto exit;
	}

	if (p->p_memstat_state & P_MEMSTAT_INTERNAL) {
		ret = EPERM;
		goto exit;
	}

	if (p->p_memstat_dirty & P_DIRTY_IS_DIRTY) {
		was_dirty = TRUE;
	}

	if (!(p->p_memstat_dirty & P_DIRTY_TRACK)) {
		                                
		ret = EINVAL;
	} else if (pcontrol && (p->p_memstat_dirty & P_DIRTY_TERMINATED)) {
		  
                                                                           
                                                                                    
     
		ret = EBUSY;
	} else {
		int flag = (self == TRUE) ? P_DIRTY : P_DIRTY_SHUTDOWN;
		if (pcontrol && !(p->p_memstat_dirty & flag)) {
			                                                           
			p->p_memstat_dirty |= (flag | P_DIRTY_MARKED);
			memorystatus_dirty_count++;
			ret = 0;
		} else if ((pcontrol == 0) && (p->p_memstat_dirty & flag)) {
			if ((flag == P_DIRTY_SHUTDOWN) && (!(p->p_memstat_dirty & P_DIRTY))) {
				                                                                                 
				p->p_memstat_dirty |= P_DIRTY_TERMINATED;
				kill = true;
			} else if ((flag == P_DIRTY) && (p->p_memstat_dirty & P_DIRTY_TERMINATED)) {
				                                                       
				kill = true;
			}
			p->p_memstat_dirty &= ~flag;
			memorystatus_dirty_count--;
			ret = 0;
		} else {
			                 
			ret = EALREADY;
		}
	}

	if (ret != 0) {
		goto exit;
	}

	if (p->p_memstat_dirty & P_DIRTY_IS_DIRTY) {
		now_dirty = TRUE;
	}

	if ((was_dirty == TRUE && now_dirty == FALSE) ||
	    (was_dirty == FALSE && now_dirty == TRUE)) {
#if CONFIG_DIRTYSTATUS_TRACKING
		if (dirtystatus_tracking_enabled) {
			uint32_t pages = 0;
			memorystatus_get_task_page_counts(p->task, &pages, NULL, NULL);
			change_event.dsc_pid = p->p_pid;
			change_event.dsc_event_type = (now_dirty == TRUE) ? kDirtyStatusChangedDirty : kDirtyStatusChangedClean;
			change_event.dsc_time =  mach_absolute_time();
			change_event.dsc_pages = pages;
			change_event.dsc_priority = p->p_memstat_effectivepriority;
			strlcpy(&change_event.dsc_process_name[0], p->p_name, sizeof(change_event.dsc_process_name));
			notify_change = TRUE;
		}
#endif

		                                           
		if ((p->p_memstat_dirty & P_DIRTY_IDLE_EXIT_ENABLED) == P_DIRTY_IDLE_EXIT_ENABLED) {
			  
                                                                                                               
                                                                                                                
                                                               
                                                                                                            
     
                                                                                                                 
                                       
      

			if (p->p_memstat_dirty & P_DIRTY_IS_DIRTY) {
				  
                                                                       
      
                                                                        
                   
       
				boolean_t reset_state = (jetsam_aging_policy != kJetsamAgingPolicyLegacy) ? TRUE : FALSE;

				memorystatus_invalidate_idle_demotion_locked(p, reset_state);
				reschedule = TRUE;
			} else {
				  
                                               
       

				if (jetsam_aging_policy == kJetsamAgingPolicyLegacy) {
					if (((p->p_memstat_dirty & P_DIRTY_DEFER_ALWAYS) == FALSE) &&
					    (mach_absolute_time() >= p->p_memstat_idledeadline)) {
						  
                                                                       
                                                        
                                                     
        
                                                    
                                                              
               
        
                                                           
                                                        
                                                           
         

						memorystatus_invalidate_idle_demotion_locked(p, TRUE);
						reschedule = TRUE;
					} else {
						  
                                                                          
                                                        
                                                       
                                                                
         

						if (p->p_memstat_dirty & P_DIRTY_DEFER_ALWAYS) {
							memorystatus_schedule_idle_demotion_locked(p, TRUE);
							reschedule = TRUE;
						} else if (p->p_memstat_dirty & P_DIRTY_AGING_IN_PROGRESS) {
							memorystatus_schedule_idle_demotion_locked(p, FALSE);
							reschedule = TRUE;
						}
					}
				} else {
					memorystatus_schedule_idle_demotion_locked(p, TRUE);
					reschedule = TRUE;
				}
			}
		}

		memorystatus_update_idle_priority_locked(p);

		if (memorystatus_highwater_enabled) {
			boolean_t ledger_update_needed = TRUE;
			boolean_t use_active;
			boolean_t is_fatal;
			  
                                                                   
                                                               
      

			if (proc_jetsam_state_is_active_locked(p) == TRUE) {
				  
                                         
         
                       
       
				CACHE_ACTIVE_LIMITS_LOCKED(p, is_fatal);
				use_active = TRUE;
				ledger_update_needed = TRUE;
			} else {
				  
                                                                 
                                                                  
                                         
                                                          
       

				if (p->p_memstat_dirty & P_DIRTY_ALLOW_IDLE_EXIT) {
					ledger_update_needed = FALSE;
				} else {
					CACHE_INACTIVE_LIMITS_LOCKED(p, is_fatal);
					use_active = FALSE;
					ledger_update_needed = TRUE;
				}
			}

			  
                                                      
     
                                                                                     
                                                                                         
                                                                             
                          
      

			if (ledger_update_needed && proc_ref_locked(p) == p) {
				int ledger_limit;
				if (p->p_memstat_memlimit > 0) {
					ledger_limit = p->p_memstat_memlimit;
				} else {
					ledger_limit = -1;
				}
				proc_list_unlock();
				task_set_phys_footprint_limit_internal(p->task, ledger_limit, NULL, use_active, is_fatal);
				proc_list_lock();
				proc_rele_locked(p);

				MEMORYSTATUS_DEBUG(3, "memorystatus_dirty_set: new limit on pid %d (%dMB %s) priority(%d) dirty?=0x%x %s\n",
				    p->p_pid, (p->p_memstat_memlimit > 0 ? p->p_memstat_memlimit : -1),
				    (p->p_memstat_state & P_MEMSTAT_FATAL_MEMLIMIT ? "F " : "NF"), p->p_memstat_effectivepriority, p->p_memstat_dirty,
				    (p->p_memstat_dirty ? ((p->p_memstat_dirty & P_DIRTY) ? "isdirty" : "isclean") : ""));
			}
		}

		                                                                  
		if (reschedule) {
			memorystatus_reschedule_idle_demotion_locked();
		}
	}

	if (kill) {
		if (proc_ref_locked(p) == p) {
			proc_list_unlock();
			psignal(p, SIGKILL);
			proc_list_lock();
			proc_rele_locked(p);
		}
	}

exit:
	proc_list_unlock();

#if CONFIG_DIRTYSTATUS_TRACKING
	                                                                    
	if (notify_change) {
		memorystatus_send_dirty_status_change_note(&change_event, sizeof(change_event));
	}
#endif

	return ret;
}

int
memorystatus_dirty_clear(proc_t p, uint32_t pcontrol)
{
	int ret = 0;

	MEMORYSTATUS_DEBUG(1, "memorystatus_dirty_clear(): %d 0x%x 0x%x\n", p->p_pid, pcontrol, p->p_memstat_dirty);

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_DIRTY_CLEAR), p->p_pid, pcontrol, 0, 0, 0);

	proc_list_lock();

	if ((p->p_listflag & P_LIST_EXITED) != 0) {
		  
                               
     
		ret = EBUSY;
		goto exit;
	}

	if (p->p_memstat_state & P_MEMSTAT_INTERNAL) {
		ret = EPERM;
		goto exit;
	}

	if (!(p->p_memstat_dirty & P_DIRTY_TRACK)) {
		                                
		ret = EINVAL;
		goto exit;
	}

	if (!pcontrol || (pcontrol & (PROC_DIRTY_LAUNCH_IN_PROGRESS | PROC_DIRTY_DEFER | PROC_DIRTY_DEFER_ALWAYS)) == 0) {
		ret = EINVAL;
		goto exit;
	}

	if (pcontrol & PROC_DIRTY_LAUNCH_IN_PROGRESS) {
		p->p_memstat_dirty &= ~P_DIRTY_LAUNCH_IN_PROGRESS;
	}

	                                               
	if (pcontrol & (PROC_DIRTY_DEFER | PROC_DIRTY_DEFER_ALWAYS)) {
		if (p->p_memstat_dirty & P_DIRTY_DEFER) {
			p->p_memstat_dirty &= ~(P_DIRTY_DEFER);
		}

		if (p->p_memstat_dirty & P_DIRTY_DEFER_ALWAYS) {
			p->p_memstat_dirty &= ~(P_DIRTY_DEFER_ALWAYS);
		}

		memorystatus_invalidate_idle_demotion_locked(p, TRUE);
		memorystatus_update_idle_priority_locked(p);
		memorystatus_reschedule_idle_demotion_locked();
	}

	ret = 0;
exit:
	proc_list_unlock();

	return ret;
}

int
memorystatus_dirty_get(proc_t p, boolean_t locked)
{
	int ret = 0;

	if (!locked) {
		proc_list_lock();
	}

	if (p->p_memstat_dirty & P_DIRTY_TRACK) {
		ret |= PROC_DIRTY_TRACKED;
		if (p->p_memstat_dirty & P_DIRTY_ALLOW_IDLE_EXIT) {
			ret |= PROC_DIRTY_ALLOWS_IDLE_EXIT;
		}
		if (p->p_memstat_dirty & P_DIRTY) {
			ret |= PROC_DIRTY_IS_DIRTY;
		}
		if (p->p_memstat_dirty & P_DIRTY_LAUNCH_IN_PROGRESS) {
			ret |= PROC_DIRTY_LAUNCH_IS_IN_PROGRESS;
		}
	}

	if (!locked) {
		proc_list_unlock();
	}

	return ret;
}

int
memorystatus_on_terminate(proc_t p)
{
	int sig;

	proc_list_lock();

	p->p_memstat_dirty |= P_DIRTY_TERMINATED;

	if (((p->p_memstat_dirty & (P_DIRTY_TRACK | P_DIRTY_IS_DIRTY)) == P_DIRTY_TRACK) ||
	    (p->p_memstat_state & P_MEMSTAT_SUSPENDED)) {
		  
                                              
                            
                                                             
                                                            
                                  
     
#if DEVELOPMENT || DEBUG
		if (p->p_memstat_state & P_MEMSTAT_SUSPENDED) {
			os_log(OS_LOG_DEFAULT, "memorystatus: sending suspended process %s (pid %d) SIGKILL",
			    (*p->p_name ? p->p_name : "unknown"), p->p_pid);
		}
#endif                           
		sig = SIGKILL;
	} else {
		                                                                                         
		sig = SIGTERM;
	}

	proc_list_unlock();

	return sig;
}

void
memorystatus_on_suspend(proc_t p)
{
#if CONFIG_FREEZE
	uint32_t pages;
	memorystatus_get_task_page_counts(p->task, &pages, NULL, NULL);
#endif
	proc_list_lock();
#if CONFIG_FREEZE
	memorystatus_suspended_count++;
#endif
	p->p_memstat_state |= P_MEMSTAT_SUSPENDED;
	proc_list_unlock();
}

extern uint64_t memorystatus_thaw_count_since_boot;

void
memorystatus_on_resume(proc_t p)
{
#if CONFIG_FREEZE
	boolean_t frozen;
	pid_t pid;
#endif

	proc_list_lock();

#if CONFIG_FREEZE
	frozen = (p->p_memstat_state & P_MEMSTAT_FROZEN);
	if (frozen) {
		  
                                                   
                                                    
                                                        
    
                                 
    
                                                             
                                                                 
                         
     
		if ((p->p_memstat_state & P_MEMSTAT_REFREEZE_ELIGIBLE) == 0) {
			p->p_memstat_state |= P_MEMSTAT_REFREEZE_ELIGIBLE;
			memorystatus_refreeze_eligible_count++;
		}
		if (p->p_memstat_thaw_count == 0 || p->p_memstat_last_thaw_interval < memorystatus_freeze_current_interval) {
			os_atomic_inc(&(memorystatus_freezer_stats.mfs_processes_thawed), relaxed);
		}
		p->p_memstat_last_thaw_interval = memorystatus_freeze_current_interval;
		p->p_memstat_thaw_count++;

		memorystatus_thaw_count++;
		memorystatus_thaw_count_since_boot++;
	}

	memorystatus_suspended_count--;

	pid = p->p_pid;
#endif

	  
                                                            
                                                                    
    
	p->p_memstat_state &= ~P_MEMSTAT_SUSPENDED;

	proc_list_unlock();

#if CONFIG_FREEZE
	if (frozen) {
		memorystatus_freeze_entry_t data = { pid, FALSE, 0 };
		memorystatus_send_note(kMemorystatusFreezeNote, &data, sizeof(data));
	}
#endif
}

void
memorystatus_on_inactivity(proc_t p)
{
#pragma unused(p)
#if CONFIG_FREEZE
	                            
	thread_wakeup((event_t)&memorystatus_freeze_wakeup);
#endif
}

  
                                            
   
static uint32_t
memorystatus_build_state(proc_t p)
{
	uint32_t snapshot_state = 0;

	             
	if (p->p_memstat_state & P_MEMSTAT_SUSPENDED) {
		snapshot_state |= kMemorystatusSuspended;
	}
	if (p->p_memstat_state & P_MEMSTAT_FROZEN) {
		snapshot_state |= kMemorystatusFrozen;
	}
	if (p->p_memstat_state & P_MEMSTAT_REFREEZE_ELIGIBLE) {
		snapshot_state |= kMemorystatusWasThawed;
	}
	if (p->p_memstat_state & P_MEMSTAT_PRIORITY_ASSERTION) {
		snapshot_state |= kMemorystatusAssertion;
	}

	              
	if (p->p_memstat_dirty & P_DIRTY_TRACK) {
		snapshot_state |= kMemorystatusTracked;
	}
	if ((p->p_memstat_dirty & P_DIRTY_IDLE_EXIT_ENABLED) == P_DIRTY_IDLE_EXIT_ENABLED) {
		snapshot_state |= kMemorystatusSupportsIdleExit;
	}
	if (p->p_memstat_dirty & P_DIRTY_IS_DIRTY) {
		snapshot_state |= kMemorystatusDirty;
	}

	return snapshot_state;
}

static boolean_t
kill_idle_exit_proc(void)
{
	proc_t p, victim_p = PROC_NULL;
	uint64_t current_time, footprint_of_killed_proc;
	boolean_t killed = FALSE;
	unsigned int i = 0;
	os_reason_t jetsam_reason = OS_REASON_NULL;

	                                 
	current_time = mach_absolute_time();

	jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_MEMORY_IDLE_EXIT);
	if (jetsam_reason == OS_REASON_NULL) {
		printf("kill_idle_exit_proc: failed to allocate jetsam reason\n");
	}

	proc_list_lock();

	p = memorystatus_get_first_proc_locked(&i, FALSE);
	while (p) {
		                                          
		if (p->p_memstat_effectivepriority != JETSAM_PRIORITY_IDLE) {
			break;
		}

		if ((p->p_memstat_dirty & (P_DIRTY_ALLOW_IDLE_EXIT | P_DIRTY_IS_DIRTY | P_DIRTY_TERMINATED)) == (P_DIRTY_ALLOW_IDLE_EXIT)) {
			if (current_time >= p->p_memstat_idledeadline) {
				p->p_memstat_dirty |= P_DIRTY_TERMINATED;
				victim_p = proc_ref_locked(p);
				break;
			}
		}

		p = memorystatus_get_next_proc_locked(&i, p, FALSE);
	}

	proc_list_unlock();

	if (victim_p) {
		printf("memorystatus: killing_idle_process pid %d [%s] jetsam_reason->osr_code: %llu\n", victim_p->p_pid, (*victim_p->p_name ? victim_p->p_name : "unknown"), jetsam_reason->osr_code);
		killed = memorystatus_do_kill(victim_p, kMemorystatusKilledIdleExit, jetsam_reason, &footprint_of_killed_proc);
		proc_rele(victim_p);
	} else {
		os_reason_free(jetsam_reason);
	}

	return killed;
}

static void
memorystatus_thread_wake(void)
{
	int thr_id = 0;
	int active_thr = atomic_load(&active_jetsam_threads);

	                                   
	for (thr_id = 0; thr_id < active_thr; thr_id++) {
		thread_wakeup((event_t)&jetsam_threads[thr_id].memorystatus_wakeup);
	}
}

#if CONFIG_JETSAM

static void
memorystatus_thread_pool_max()
{
	                                                           
	int max_threads = max_jetsam_threads;
	printf("Expanding memorystatus pool to %d!\n", max_threads);
	atomic_store(&active_jetsam_threads, max_threads);
}

static void
memorystatus_thread_pool_default()
{
	                                                       
	printf("Reverting memorystatus pool back to 1\n");
	atomic_store(&active_jetsam_threads, 1);
}

#endif                    

extern void vm_pressure_response(void);

static int
memorystatus_thread_block(uint32_t interval_ms, thread_continue_t continuation)
{
	struct jetsam_thread_state *jetsam_thread = jetsam_current_thread();

	assert(jetsam_thread != NULL);
	if (interval_ms) {
		assert_wait_timeout(&jetsam_thread->memorystatus_wakeup, THREAD_UNINT, interval_ms, NSEC_PER_MSEC);
	} else {
		assert_wait(&jetsam_thread->memorystatus_wakeup, THREAD_UNINT);
	}

	return thread_block(continuation);
}

static boolean_t
memorystatus_avail_pages_below_pressure(void)
{
#if CONFIG_JETSAM
	return memorystatus_available_pages <= memorystatus_available_pages_pressure;
#else                    
	return FALSE;
#endif                    
}

static boolean_t
memorystatus_avail_pages_below_critical(void)
{
#if CONFIG_JETSAM
	return memorystatus_available_pages <= memorystatus_available_pages_critical;
#else                    
	return FALSE;
#endif                    
}

static boolean_t
memorystatus_post_snapshot(int32_t priority, uint32_t cause)
{
	boolean_t is_idle_priority;

	if (jetsam_aging_policy == kJetsamAgingPolicyLegacy) {
		is_idle_priority = (priority == JETSAM_PRIORITY_IDLE);
	} else {
		is_idle_priority = (priority == JETSAM_PRIORITY_IDLE || priority == JETSAM_PRIORITY_IDLE_DEFERRED);
	}
#if CONFIG_JETSAM
#pragma unused(cause)
	  
                                                         
                                                      
         
    

	return !is_idle_priority || memorystatus_idle_snapshot;

#else                    
	  
                                                         
          
                                                 
         
      
                                                                        
    

	boolean_t snapshot_eligible_kill_cause = (is_reason_thrashing(cause) || is_reason_zone_map_exhaustion(cause));
	return !is_idle_priority || memorystatus_idle_snapshot || snapshot_eligible_kill_cause;
#endif                    
}

static boolean_t
memorystatus_action_needed(void)
{
#if CONFIG_JETSAM
	return is_reason_thrashing(kill_under_pressure_cause) ||
	       is_reason_zone_map_exhaustion(kill_under_pressure_cause) ||
	       memorystatus_available_pages <= memorystatus_available_pages_pressure;
#else                    
	return is_reason_thrashing(kill_under_pressure_cause) ||
	       is_reason_zone_map_exhaustion(kill_under_pressure_cause);
#endif                    
}

static boolean_t
memorystatus_act_on_hiwat_processes(uint32_t *errors, uint32_t *hwm_kill, boolean_t *post_snapshot, __unused boolean_t *is_critical, uint64_t *memory_reclaimed)
{
	boolean_t purged = FALSE, killed = FALSE;

	*memory_reclaimed = 0;
	killed = memorystatus_kill_hiwat_proc(errors, &purged, memory_reclaimed);

	if (killed) {
		*hwm_kill = *hwm_kill + 1;
		*post_snapshot = TRUE;
		return TRUE;
	} else {
		if (purged == FALSE) {
			                                      
			memorystatus_hwm_candidates = FALSE;
		}
	}

#if CONFIG_JETSAM
	                                                               
	if (!is_reason_thrashing(kill_under_pressure_cause) &&
	    !is_reason_zone_map_exhaustion(kill_under_pressure_cause) &&
	    (memorystatus_available_pages > memorystatus_available_pages_critical)) {
		  
                                                                                      
                              
                         
                                  
                                             
     

		if (*hwm_kill == 0) {
			memorystatus_thread_wasted_wakeup++;
		}

		*is_critical = FALSE;

		return TRUE;
	}
#endif                    

	return FALSE;
}

  
                                                                              
                                                                                  
                     
   
#define kJetsamHighRelaunchCandidatesThreshold  (100)

                                                                                
                                                                                 
                                                                                    
                                                                                    
                 
   
#define kJetsamMinCandidatesThreshold           (5)

static boolean_t
memorystatus_aggressive_jetsam_needed_sysproc_aging(__unused int jld_eval_aggressive_count, __unused int *jld_idle_kills, __unused int jld_idle_kill_candidates, int *total_candidates, int *elevated_bucket_count)
{
	boolean_t aggressive_jetsam_needed = false;

	  
                                                                                         
                                                                                           
                                                                                      
                                                                                            
                                                                 
    
	proc_list_lock();
	                                                                 
	*total_candidates = memstat_bucket[JETSAM_PRIORITY_IDLE].count + memstat_bucket[system_procs_aging_band].count;
	                                                                       
	int bad_candidates = memstat_bucket[JETSAM_PRIORITY_IDLE].relaunch_high_count + memstat_bucket[system_procs_aging_band].relaunch_high_count;

	*elevated_bucket_count = memstat_bucket[JETSAM_PRIORITY_ELEVATED_INACTIVE].count;

	proc_list_unlock();

	                                                                                                    
	aggressive_jetsam_needed = (((bad_candidates * 100) / *total_candidates) >= kJetsamHighRelaunchCandidatesThreshold);

	  
                                                                                   
                                                                                      
                                                                                     
           
    
	if (memorystatus_available_pages > memorystatus_sysproc_aging_aggr_pages) {
		aggressive_jetsam_needed = false;
	}

#if DEVELOPMENT || DEBUG
	printf("memorystatus: aggressive%d: [%s] Bad Candidate Threshold Check (total: %d, bad: %d, threshold: %d %%); Memory Pressure Check (available_pgs: %llu, threshold_pgs: %llu)\n",
	    jld_eval_aggressive_count, aggressive_jetsam_needed ? "PASSED" : "FAILED", *total_candidates, bad_candidates,
	    kJetsamHighRelaunchCandidatesThreshold, (uint64_t)MEMORYSTATUS_LOG_AVAILABLE_PAGES, (uint64_t)memorystatus_sysproc_aging_aggr_pages);
#endif                           
	return aggressive_jetsam_needed;
}

  
                                               
                                                                         
                  
   
static void
memorystatus_approaching_fg_band(boolean_t *corpse_list_purged)
{
	assert(corpse_list_purged != NULL);
	pmap_release_pages_fast();
	memorystatus_issue_fg_band_notify();
	if (total_corpses_count() > 0 && !*corpse_list_purged) {
		task_purge_all_corpses();
		*corpse_list_purged = TRUE;
	}
}

static boolean_t
memorystatus_aggressive_jetsam_needed_default(__unused int jld_eval_aggressive_count, int *jld_idle_kills, int jld_idle_kill_candidates, int *total_candidates, int *elevated_bucket_count)
{
	boolean_t aggressive_jetsam_needed = false;
	                                    
	memstat_bucket_t *bucket;
	int             jld_bucket_count = 0;

	proc_list_lock();
	switch (jetsam_aging_policy) {
	case kJetsamAgingPolicyLegacy:
		bucket = &memstat_bucket[JETSAM_PRIORITY_IDLE];
		jld_bucket_count = bucket->count;
		bucket = &memstat_bucket[JETSAM_PRIORITY_AGING_BAND1];
		jld_bucket_count += bucket->count;
		break;
	case kJetsamAgingPolicyAppsReclaimedFirst:
		bucket = &memstat_bucket[JETSAM_PRIORITY_IDLE];
		jld_bucket_count = bucket->count;
		bucket = &memstat_bucket[system_procs_aging_band];
		jld_bucket_count += bucket->count;
		bucket = &memstat_bucket[applications_aging_band];
		jld_bucket_count += bucket->count;
		break;
	case kJetsamAgingPolicyNone:
	default:
		bucket = &memstat_bucket[JETSAM_PRIORITY_IDLE];
		jld_bucket_count = bucket->count;
		break;
	}

	bucket = &memstat_bucket[JETSAM_PRIORITY_ELEVATED_INACTIVE];
	*elevated_bucket_count = bucket->count;
	*total_candidates = jld_bucket_count;
	proc_list_unlock();

	aggressive_jetsam_needed = (*jld_idle_kills > jld_idle_kill_candidates);

#if DEVELOPMENT || DEBUG
	if (aggressive_jetsam_needed) {
		printf("memorystatus: aggressive%d: idle candidates: %d, idle kills: %d\n",
		    jld_eval_aggressive_count,
		    jld_idle_kill_candidates,
		    *jld_idle_kills);
	}
#endif                           
	return aggressive_jetsam_needed;
}

static boolean_t
memorystatus_act_aggressive(uint32_t cause, os_reason_t jetsam_reason, int *jld_idle_kills, boolean_t *corpse_list_purged, boolean_t *post_snapshot, uint64_t *memory_reclaimed)
{
	boolean_t aggressive_jetsam_needed = false;
	boolean_t killed;
	uint32_t errors = 0;
	uint64_t footprint_of_killed_proc = 0;
	int elevated_bucket_count = 0;
	int total_candidates = 0;
	*memory_reclaimed = 0;

	  
                                                                               
                                                                               
                                                           
   
                                                                   
                                                                       
    
	static int       jld_eval_aggressive_count = 0;
	static int32_t   jld_priority_band_max = JETSAM_PRIORITY_UI_SUPPORT;
	static uint64_t  jld_timestamp_msecs = 0;
	static int       jld_idle_kill_candidates = 0;

	if (memorystatus_jld_enabled == FALSE) {
		                                                          
		return FALSE;
	}

	                                        
	struct timeval  jld_now_tstamp = {0, 0};
	uint64_t        jld_now_msecs = 0;
	microuptime(&jld_now_tstamp);
	jld_now_msecs = (jld_now_tstamp.tv_sec * 1000);

	  
                                                                           
                                                                
    
	if (jetsam_aging_policy == kJetsamAgingPolicySysProcsReclaimedFirst) {
		  
                                                                                                    
                                                                                                  
                 
     
		aggressive_jetsam_needed = memorystatus_aggressive_jetsam_needed_sysproc_aging(jld_eval_aggressive_count,
		    jld_idle_kills, jld_idle_kill_candidates, &total_candidates, &elevated_bucket_count);
	} else {
		  
                                                                                                   
                                                                                        
     
		aggressive_jetsam_needed = memorystatus_aggressive_jetsam_needed_default(jld_eval_aggressive_count,
		    jld_idle_kills, jld_idle_kill_candidates, &total_candidates, &elevated_bucket_count);
	}

	  
                                                                        
                                                                                        
                                                                 
    
	boolean_t param_reval = false;

	if ((total_candidates == 0) ||
	    (jld_now_msecs > (jld_timestamp_msecs + memorystatus_jld_eval_period_msecs))) {
		jld_timestamp_msecs      = jld_now_msecs;
		jld_idle_kill_candidates = total_candidates;
		*jld_idle_kills          = 0;
		jld_eval_aggressive_count = 0;
		jld_priority_band_max   = JETSAM_PRIORITY_UI_SUPPORT;
		param_reval = true;
	}

	  
                                                                                               
                                                                                                            
    
	if ((param_reval == true) && (jetsam_aging_policy != kJetsamAgingPolicySysProcsReclaimedFirst)) {
		aggressive_jetsam_needed = (*jld_idle_kills > jld_idle_kill_candidates);
	}

	  
                                                                                                    
                                                                                                   
                                                                        
    
	if (total_candidates < kJetsamMinCandidatesThreshold) {
#if DEVELOPMENT || DEBUG
		printf("memorystatus: aggressive: [FAILED] Low Candidate Count (current: %d, threshold: %d)\n", total_candidates, kJetsamMinCandidatesThreshold);
#endif                           
		aggressive_jetsam_needed = false;
	}

	if (aggressive_jetsam_needed == false) {
		                                                                                                                           
		return FALSE;
	}

	                                            
	jld_eval_aggressive_count++;

	if (jld_eval_aggressive_count == memorystatus_jld_eval_aggressive_count) {
		memorystatus_approaching_fg_band(corpse_list_purged);
	} else if (jld_eval_aggressive_count > memorystatus_jld_eval_aggressive_count) {
		  
                                                             
                                 
     
		if ((memorystatus_jld_eval_aggressive_priority_band_max < 0) ||
		    (memorystatus_jld_eval_aggressive_priority_band_max >= MEMSTAT_BUCKET_COUNT)) {
			  
                                                
      
		} else {
			jld_priority_band_max = memorystatus_jld_eval_aggressive_priority_band_max;
		}
	}

	                                    
	while (elevated_bucket_count) {
		elevated_bucket_count--;

		  
                                                            
                                                                   
                           
     

		os_reason_ref(jetsam_reason);
		killed = memorystatus_kill_elevated_process(
			cause,
			jetsam_reason,
			JETSAM_PRIORITY_ELEVATED_INACTIVE,
			jld_eval_aggressive_count,
			&errors, &footprint_of_killed_proc);
		if (killed) {
			*post_snapshot = TRUE;
			*memory_reclaimed += footprint_of_killed_proc;
			if (memorystatus_avail_pages_below_pressure()) {
				  
                            
                                     
       
				continue;
			} else {
				return TRUE;
			}
		} else {
			  
                                       
                            
      
			break;
		}
	}

	  
                                                              
                                                               
                                                  
    
	killed = memorystatus_kill_processes_aggressive(
		kMemorystatusKilledProcThrashing,
		jld_eval_aggressive_count,
		jld_priority_band_max,
		&errors, &footprint_of_killed_proc);

	if (killed) {
		                                                
		*post_snapshot = TRUE;
		*memory_reclaimed += footprint_of_killed_proc;
		*jld_idle_kills = 0;
		return TRUE;
	}

	return FALSE;
}


static void
memorystatus_thread(void *param __unused, wait_result_t wr __unused)
{
	boolean_t post_snapshot = FALSE;
	uint32_t errors = 0;
	uint32_t hwm_kill = 0;
	boolean_t sort_flag = TRUE;
	boolean_t corpse_list_purged = FALSE;
	int     jld_idle_kills = 0;
	struct jetsam_thread_state *jetsam_thread = jetsam_current_thread();
	uint64_t total_memory_reclaimed = 0;

	assert(jetsam_thread != NULL);
	if (jetsam_thread->inited == FALSE) {
		  
                                                                                             
                                                                                              
     

		char name[32];
		thread_wire(host_priv_self(), current_thread(), TRUE);
		snprintf(name, 32, "VM_memorystatus_%d", jetsam_thread->index + 1);

		                                                                                                  
		if (jetsam_thread->index == 0) {
			if (vm_pageout_state.vm_restricted_to_single_processor == TRUE) {
				thread_vm_bind_group_add();
			}
			jetsam_thread->limit_to_low_bands = FALSE;
		} else {
			jetsam_thread->limit_to_low_bands = TRUE;
		}
#if CONFIG_THREAD_GROUPS
		thread_group_vm_add();
#endif
		thread_set_thread_name(current_thread(), name);
		jetsam_thread->inited = TRUE;
		memorystatus_thread_block(0, memorystatus_thread);
	}

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_SCAN) | DBG_FUNC_START,
	    MEMORYSTATUS_LOG_AVAILABLE_PAGES, memorystatus_jld_enabled, memorystatus_jld_eval_period_msecs, memorystatus_jld_eval_aggressive_count, 0);

	  
                         
   
                                                                                        
   
                                                                                 
                                                                                   
                                                          
   
                                                                                                    
                                                                                                              
    
	while (memorystatus_action_needed()) {
		boolean_t killed;
		int32_t priority;
		uint32_t cause;
		uint64_t memory_reclaimed = 0;
		uint64_t jetsam_reason_code = JETSAM_REASON_INVALID;
		os_reason_t jetsam_reason = OS_REASON_NULL;

		cause = kill_under_pressure_cause;
		switch (cause) {
		case kMemorystatusKilledFCThrashing:
			jetsam_reason_code = JETSAM_REASON_MEMORY_FCTHRASHING;
			break;
		case kMemorystatusKilledVMCompressorThrashing:
			jetsam_reason_code = JETSAM_REASON_MEMORY_VMCOMPRESSOR_THRASHING;
			break;
		case kMemorystatusKilledVMCompressorSpaceShortage:
			jetsam_reason_code = JETSAM_REASON_MEMORY_VMCOMPRESSOR_SPACE_SHORTAGE;
			break;
		case kMemorystatusKilledZoneMapExhaustion:
			jetsam_reason_code = JETSAM_REASON_ZONE_MAP_EXHAUSTION;
			break;
		case kMemorystatusKilledVMPageShortage:
		                   
		default:
			jetsam_reason_code = JETSAM_REASON_MEMORY_VMPAGESHORTAGE;
			cause = kMemorystatusKilledVMPageShortage;
			break;
		}

		               
		boolean_t is_critical = TRUE;
		if (memorystatus_act_on_hiwat_processes(&errors, &hwm_kill, &post_snapshot, &is_critical, &memory_reclaimed)) {
			total_memory_reclaimed += memory_reclaimed;
			if (is_critical == FALSE) {
				  
                                               
       
				break;
			} else {
				goto done;
			}
		}

		jetsam_reason = os_reason_create(OS_REASON_JETSAM, jetsam_reason_code);
		if (jetsam_reason == OS_REASON_NULL) {
			printf("memorystatus_thread: failed to allocate jetsam reason\n");
		}

		                                                         
		if (!jetsam_thread->limit_to_low_bands &&
		    memorystatus_act_aggressive(cause, jetsam_reason, &jld_idle_kills, &corpse_list_purged, &post_snapshot, &memory_reclaimed)) {
			total_memory_reclaimed += memory_reclaimed;
			goto done;
		}

		  
                                                       
                                                                   
                          
     
		os_reason_ref(jetsam_reason);

		         
		killed = memorystatus_kill_top_process(TRUE, sort_flag, cause, jetsam_reason, &priority, &errors, &memory_reclaimed);
		sort_flag = FALSE;

		if (killed) {
			total_memory_reclaimed += memory_reclaimed;
			if (memorystatus_post_snapshot(priority, cause) == TRUE) {
				post_snapshot = TRUE;
			}

			                           
			if (memorystatus_jld_enabled == TRUE) {
				if ((priority == JETSAM_PRIORITY_IDLE) || (priority == system_procs_aging_band) || (priority == applications_aging_band)) {
					jld_idle_kills++;
				} else {
					  
                                                      
                                          
        
				}
			}

			  
                                                                           
                                                                                
                                   
      
			if (priority >= JETSAM_PRIORITY_UI_SUPPORT) {
				memorystatus_approaching_fg_band(&corpse_list_purged);
			}
			goto done;
		}

		if (memorystatus_avail_pages_below_critical()) {
			  
                                                                             
                                            
      
			pmap_release_pages_fast();
			if (total_corpses_count() > 0) {
				task_purge_all_corpses();
				corpse_list_purged = TRUE;
			}

			if (!jetsam_thread->limit_to_low_bands && memorystatus_avail_pages_below_critical()) {
				  
                                                                
       
				panic("memorystatus_jetsam_thread: no victim! available pages:%llu\n", (uint64_t)MEMORYSTATUS_LOG_AVAILABLE_PAGES);
			}
		}

done:

		  
                                                                  
                                                         
                
     
		if (is_reason_thrashing(kill_under_pressure_cause)) {
			kill_under_pressure_cause = 0;
#if CONFIG_JETSAM
			vm_thrashing_jetsam_done();
#endif                    
		} else if (is_reason_zone_map_exhaustion(kill_under_pressure_cause)) {
			kill_under_pressure_cause = 0;
		}

		os_reason_free(jetsam_reason);
	}

	kill_under_pressure_cause = 0;

	if (errors) {
		memorystatus_clear_errors();
	}

	if (post_snapshot) {
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

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_SCAN) | DBG_FUNC_END,
	    MEMORYSTATUS_LOG_AVAILABLE_PAGES, total_memory_reclaimed, 0, 0, 0);

	memorystatus_thread_block(0, memorystatus_thread);
}

  
                
                                             
                 
                                                   
                                                             
   
boolean_t
memorystatus_idle_exit_from_VM(void)
{
	  
                                                        
                                                            
                                                              
   
                                                              
                                                      
                                                               
                                                               
    

	return kill_idle_exit_proc();
}

  
                                                                     
                                 
  
                                                                         
                                           
   
void
memorystatus_on_ledger_footprint_exceeded(boolean_t warning, boolean_t memlimit_is_active, boolean_t memlimit_is_fatal)
{
	os_reason_t jetsam_reason = OS_REASON_NULL;

	proc_t p = current_proc();

#if VM_PRESSURE_EVENTS
	if (warning == TRUE) {
		  
                                                                                    
                                                   
     
		if (memorystatus_warn_process(p, memlimit_is_active, memlimit_is_fatal, FALSE                   ) != TRUE) {
			                                                                                                
			os_log(OS_LOG_DEFAULT, "memorystatus_on_ledger_footprint_exceeded: failed to warn the current task (%d exiting, or no handler registered?).\n", p->p_pid);
		}
		return;
	}
#endif                         

	if (memlimit_is_fatal) {
		  
                                                                                                                
                                                                                     
     
		jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_MEMORY_PERPROCESSLIMIT);
		if (jetsam_reason == NULL) {
			printf("task_exceeded footprint: failed to allocate jetsam reason\n");
		} else if (corpse_for_fatal_memkill != 0 && proc_send_synchronous_EXC_RESOURCE(p) == FALSE) {
			                                                                 
			jetsam_reason->osr_flags |= OS_REASON_FLAG_GENERATE_CRASH_REPORT;
		}

		if (memorystatus_kill_process_sync(p->p_pid, kMemorystatusKilledPerProcessLimit, jetsam_reason) != TRUE) {
			printf("task_exceeded_footprint: failed to kill the current task (exiting?).\n");
		}
	} else {
		  
                                                                
                                                       
     
		memorystatus_hwm_candidates = TRUE;

#if VM_PRESSURE_EVENTS
		  
                                                    
                                                                                        
                                          
     
		(void)memorystatus_warn_process(p, memlimit_is_active, memlimit_is_fatal, TRUE               );

#endif                         
	}
}

void
memorystatus_log_exception(const int max_footprint_mb, boolean_t memlimit_is_active, boolean_t memlimit_is_fatal)
{
	proc_t p = current_proc();

	  
                                                                            
                                                    
                                                                                        
    

	os_log_with_startup_serial(OS_LOG_DEFAULT, "EXC_RESOURCE -> %s[%d] exceeded mem limit: %s%s %d MB (%s)\n",
	    ((p && *p->p_name) ? p->p_name : "unknown"), (p ? p->p_pid : -1), (memlimit_is_active ? "Active" : "Inactive"),
	    (memlimit_is_fatal  ? "Hard" : "Soft"), max_footprint_mb,
	    (memlimit_is_fatal  ? "fatal" : "non-fatal"));

	return;
}


  
               
                                                   
                                                 
  
                                                                    
                                                            
                                
  
                                                       
                                 
                   
                     
  
                                                  
                                      
                                 
                                   
  
                         
                     
   

static boolean_t
proc_jetsam_state_is_active_locked(proc_t p)
{
	if ((p->p_memstat_state & P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND) &&
	    (p->p_memstat_effectivepriority == JETSAM_PRIORITY_ELEVATED_INACTIVE)) {
		  
                                                              
                                                
                         
     
		return TRUE;
	} else if (p->p_memstat_dirty & P_DIRTY_TRACK) {
		  
                                          
                                             
     
		if (p->p_memstat_dirty & P_DIRTY_IS_DIRTY) {
			  
                      
                          
      
			return TRUE;
		} else {
			  
                      
                            
      
			return FALSE;
		}
	} else if (p->p_memstat_effectivepriority >= JETSAM_PRIORITY_FOREGROUND) {
		  
                                    
                         
     
		return TRUE;
	} else {
		  
                                   
                           
     
		return FALSE;
	}
}

static boolean_t
memorystatus_kill_process_sync(pid_t victim_pid, uint32_t cause, os_reason_t jetsam_reason)
{
	boolean_t res;

	uint32_t errors = 0;
	uint64_t memory_reclaimed = 0;

	if (victim_pid == -1) {
		                                   
		res = memorystatus_kill_top_process(TRUE, TRUE, cause, jetsam_reason, NULL, &errors, &memory_reclaimed);
	} else {
		res = memorystatus_kill_specific_process(victim_pid, cause, jetsam_reason);
	}

	if (errors) {
		memorystatus_clear_errors();
	}

	if (res == TRUE) {
		                                    
		proc_list_lock();
		size_t snapshot_size = sizeof(memorystatus_jetsam_snapshot_t) +
		    sizeof(memorystatus_jetsam_snapshot_entry_t) * memorystatus_jetsam_snapshot_count;
		uint64_t timestamp_now = mach_absolute_time();
		memorystatus_jetsam_snapshot->notification_time = timestamp_now;
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

	return res;
}

  
                             
   
static boolean_t
memorystatus_kill_specific_process(pid_t victim_pid, uint32_t cause, os_reason_t jetsam_reason)
{
	boolean_t killed;
	proc_t p;
	uint64_t killtime = 0;
	uint64_t footprint_of_killed_proc;
	clock_sec_t     tv_sec;
	clock_usec_t    tv_usec;
	uint32_t        tv_msec;

	                                                                         

	p = proc_find(victim_pid);
	if (!p) {
		os_reason_free(jetsam_reason);
		return FALSE;
	}

	proc_list_lock();

	if (memorystatus_jetsam_snapshot_count == 0) {
		memorystatus_init_jetsam_snapshot_locked(NULL, 0);
	}

	killtime = mach_absolute_time();
	absolutetime_to_microtime(killtime, &tv_sec, &tv_usec);
	tv_msec = tv_usec / 1000;

	memorystatus_update_jetsam_snapshot_entry_locked(p, cause, killtime);

	proc_list_unlock();

	killed = memorystatus_do_kill(p, cause, jetsam_reason, &footprint_of_killed_proc);

	os_log_with_startup_serial(OS_LOG_DEFAULT, "%lu.%03d memorystatus: killing_specific_process pid %d [%s] (%s %d) %lluKB - memorystatus_available_pages: %llu\n",
	    (unsigned long)tv_sec, tv_msec, victim_pid, ((p && *p->p_name) ? p->p_name : "unknown"),
	    memorystatus_kill_cause_name[cause], (p ? p->p_memstat_effectivepriority: -1),
	    footprint_of_killed_proc >> 10, (uint64_t)MEMORYSTATUS_LOG_AVAILABLE_PAGES);

	proc_rele(p);

	return killed;
}


  
                                         
                            
   
void
proc_memstat_terminated(proc_t p, boolean_t set)
{
#if DEVELOPMENT || DEBUG
	if (p) {
		proc_list_lock();
		if (set == TRUE) {
			p->p_memstat_state |= P_MEMSTAT_TERMINATED;
		} else {
			p->p_memstat_state &= ~P_MEMSTAT_TERMINATED;
		}
		proc_list_unlock();
	}
#else
#pragma unused(p, set)
	  
              
    
#endif                           
	return;
}


#if CONFIG_JETSAM
  
                                                                         
                                                                       
                                                                    
                          
   
void
jetsam_on_ledger_cpulimit_exceeded(void)
{
	int retval = 0;
	int jetsam_flags = 0;                       
	proc_t p = current_proc();
	os_reason_t jetsam_reason = OS_REASON_NULL;

	printf("task_exceeded_cpulimit: killing pid %d [%s]\n",
	    p->p_pid, (*p->p_name ? p->p_name : "(unknown)"));

	jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_CPULIMIT);
	if (jetsam_reason == OS_REASON_NULL) {
		printf("task_exceeded_cpulimit: unable to allocate memory for jetsam reason\n");
	}

	retval = jetsam_do_kill(p, jetsam_flags, jetsam_reason);

	if (retval) {
		printf("task_exceeded_cpulimit: failed to kill current task (exiting?).\n");
	}
}

#endif                    

static void
memorystatus_get_task_memory_region_count(task_t task, uint64_t *count)
{
	assert(task);
	assert(count);

	*count = get_task_memory_region_count(task);
}


#define MEMORYSTATUS_VM_MAP_FORK_ALLOWED     0x100000000
#define MEMORYSTATUS_VM_MAP_FORK_NOT_ALLOWED 0x200000000

#if DEVELOPMENT || DEBUG

  
                                                                    
                             
     
                                   
  
                                                                            
                
                                                                         
                                                                                
                                                                 
   

uint64_t memorystatus_vm_map_fork_pidwatch_val = 0;

static int sysctl_memorystatus_vm_map_fork_pidwatch SYSCTL_HANDLER_ARGS {
#pragma unused(oidp, arg1, arg2)

	uint64_t new_value = 0;
	uint64_t old_value = 0;
	int error = 0;

	  
                                       
                                                 
    
	old_value = memorystatus_vm_map_fork_pidwatch_val;

	error = sysctl_io_number(req, old_value, sizeof(old_value), &new_value, NULL);

	if (error || !req->newptr) {
		  
                            
     
		return error;
	}

	  
                                            
                                                    
    
	memorystatus_vm_map_fork_pidwatch_val = new_value & 0xFFFFFFFF;
	printf("memorystatus: pidwatch old_value = 0x%llx, new_value = 0x%llx \n", old_value, new_value);

	return error;
}

SYSCTL_PROC(_kern, OID_AUTO, memorystatus_vm_map_fork_pidwatch, CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED | CTLFLAG_MASKED,
    0, 0, sysctl_memorystatus_vm_map_fork_pidwatch, "Q", "get/set pid watched for in vm_map_fork");


  
                                                                    
   
void
memorystatus_abort_vm_map_fork(task_t task)
{
	if (memorystatus_vm_map_fork_pidwatch_val != 0) {
		proc_t p = get_bsdtask_info(task);
		if (p != NULL && memorystatus_vm_map_fork_pidwatch_val == (uint64_t)p->p_pid) {
			memorystatus_vm_map_fork_pidwatch_val = -1ull;
		}
	}
}

static void
set_vm_map_fork_pidwatch(task_t task, uint64_t x)
{
	if (memorystatus_vm_map_fork_pidwatch_val != 0) {
		proc_t p = get_bsdtask_info(task);
		if (p && (memorystatus_vm_map_fork_pidwatch_val == (uint64_t)p->p_pid)) {
			memorystatus_vm_map_fork_pidwatch_val |= x;
		}
	}
}

#else                           


static void
set_vm_map_fork_pidwatch(task_t task, uint64_t x)
{
#pragma unused(task)
#pragma unused(x)
}

#endif                           

  
                                                                    
                                                                    
                                                        
                                          
  
                                                   
  
                                   
                                                   
                                                           
                                                           
  
                                                      
                                                          
                                                     
                                                
                                                 
   
extern boolean_t corpse_threshold_system_limit;
boolean_t
memorystatus_allowed_vm_map_fork(task_t task)
{
	boolean_t is_allowed = TRUE;                

	uint64_t footprint_in_bytes;
	uint64_t max_allowed_bytes;

	if (max_task_footprint_mb == 0) {
		set_vm_map_fork_pidwatch(task, MEMORYSTATUS_VM_MAP_FORK_ALLOWED);
		return is_allowed;
	}

	footprint_in_bytes = get_task_phys_footprint(task);

	  
                                                            
    
	max_allowed_bytes = ((uint64_t)max_task_footprint_mb * 1024 * 1024) >> 2;

#if DEBUG || DEVELOPMENT
	if (corpse_threshold_system_limit) {
		max_allowed_bytes = (uint64_t)max_task_footprint_mb * (1UL << 20);
	}
#endif                           

	if (footprint_in_bytes > max_allowed_bytes) {
		printf("memorystatus disallowed vm_map_fork %lld  %lld\n", footprint_in_bytes, max_allowed_bytes);
		set_vm_map_fork_pidwatch(task, MEMORYSTATUS_VM_MAP_FORK_NOT_ALLOWED);
		return !is_allowed;
	}

	set_vm_map_fork_pidwatch(task, MEMORYSTATUS_VM_MAP_FORK_ALLOWED);
	return is_allowed;
}

void
memorystatus_get_task_page_counts(task_t task, uint32_t *footprint, uint32_t *max_footprint_lifetime, uint32_t *purgeable_pages)
{
	assert(task);
	assert(footprint);

	uint64_t pages;

	pages = (get_task_phys_footprint(task) / PAGE_SIZE_64);
	assert(((uint32_t)pages) == pages);
	*footprint = (uint32_t)pages;

	if (max_footprint_lifetime) {
		pages = (get_task_phys_footprint_lifetime_max(task) / PAGE_SIZE_64);
		assert(((uint32_t)pages) == pages);
		*max_footprint_lifetime = (uint32_t)pages;
	}
	if (purgeable_pages) {
		pages = (get_task_purgeable_size(task) / PAGE_SIZE_64);
		assert(((uint32_t)pages) == pages);
		*purgeable_pages = (uint32_t)pages;
	}
}

static void
memorystatus_get_task_phys_footprint_page_counts(task_t task,
    uint64_t *internal_pages, uint64_t *internal_compressed_pages,
    uint64_t *purgeable_nonvolatile_pages, uint64_t *purgeable_nonvolatile_compressed_pages,
    uint64_t *alternate_accounting_pages, uint64_t *alternate_accounting_compressed_pages,
    uint64_t *iokit_mapped_pages, uint64_t *page_table_pages, uint64_t *frozen_to_swap_pages)
{
	assert(task);

	if (internal_pages) {
		*internal_pages = (get_task_internal(task) / PAGE_SIZE_64);
	}

	if (internal_compressed_pages) {
		*internal_compressed_pages = (get_task_internal_compressed(task) / PAGE_SIZE_64);
	}

	if (purgeable_nonvolatile_pages) {
		*purgeable_nonvolatile_pages = (get_task_purgeable_nonvolatile(task) / PAGE_SIZE_64);
	}

	if (purgeable_nonvolatile_compressed_pages) {
		*purgeable_nonvolatile_compressed_pages = (get_task_purgeable_nonvolatile_compressed(task) / PAGE_SIZE_64);
	}

	if (alternate_accounting_pages) {
		*alternate_accounting_pages = (get_task_alternate_accounting(task) / PAGE_SIZE_64);
	}

	if (alternate_accounting_compressed_pages) {
		*alternate_accounting_compressed_pages = (get_task_alternate_accounting_compressed(task) / PAGE_SIZE_64);
	}

	if (iokit_mapped_pages) {
		*iokit_mapped_pages = (get_task_iokit_mapped(task) / PAGE_SIZE_64);
	}

	if (page_table_pages) {
		*page_table_pages = (get_task_page_table(task) / PAGE_SIZE_64);
	}

#if CONFIG_FREEZE
	if (frozen_to_swap_pages) {
		*frozen_to_swap_pages = (get_task_frozen_to_swap(task) / PAGE_SIZE_64);
	}
#else                    
#pragma unused(frozen_to_swap_pages)
#endif                    
}

#if CONFIG_FREEZE
  
                                                         
                                                                      
                                       
   
static bool
memorystatus_jetsam_snapshot_copy_entry_locked(memorystatus_jetsam_snapshot_t *dst_snapshot, unsigned int dst_snapshot_size, const memorystatus_jetsam_snapshot_entry_t *src_entry)
{
	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_OWNED);
	assert(dst_snapshot);

	if (dst_snapshot->entry_count == dst_snapshot_size) {
		                                                                            
		return false;
	}
	if (dst_snapshot->entry_count == 0) {
		memorystatus_init_jetsam_snapshot_header(dst_snapshot);
	}
	memorystatus_jetsam_snapshot_entry_t *dst_entry = &dst_snapshot->entries[dst_snapshot->entry_count++];
	memcpy(dst_entry, src_entry, sizeof(memorystatus_jetsam_snapshot_entry_t));
	return true;
}
#endif                    

static bool
memorystatus_init_jetsam_snapshot_entry_with_kill_locked(memorystatus_jetsam_snapshot_t *snapshot, proc_t p, uint32_t kill_cause, uint64_t killtime, memorystatus_jetsam_snapshot_entry_t **entry)
{
	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_OWNED);
	memorystatus_jetsam_snapshot_entry_t *snapshot_list = snapshot->entries;
	size_t i = snapshot->entry_count;

	if (memorystatus_init_jetsam_snapshot_entry_locked(p, &snapshot_list[i], (snapshot->js_gencount)) == TRUE) {
		*entry = &snapshot_list[i];
		(*entry)->killed       = kill_cause;
		(*entry)->jse_killtime = killtime;

		snapshot->entry_count = i + 1;
		return true;
	}
	return false;
}

  
                                                              
                                                                     
                                                                       
   
static void
memorystatus_update_jetsam_snapshot_entry_locked(proc_t p, uint32_t kill_cause, uint64_t killtime)
{
	memorystatus_jetsam_snapshot_entry_t *entry = NULL;
	memorystatus_jetsam_snapshot_t *snapshot    = NULL;
	memorystatus_jetsam_snapshot_entry_t *snapshot_list = NULL;

	unsigned int i;
#if CONFIG_FREEZE
	bool copied_to_freezer_snapshot = false;
#endif                    

	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_OWNED);

	if (memorystatus_jetsam_snapshot_count == 0) {
		  
                        
                   
     
		goto exit;
	}

	  
                                                      
                            
    
	assert(kill_cause != 0 && killtime != 0);

	snapshot       = memorystatus_jetsam_snapshot;
	snapshot_list  = memorystatus_jetsam_snapshot->entries;

	for (i = 0; i < memorystatus_jetsam_snapshot_count; i++) {
		if (snapshot_list[i].pid == p->p_pid) {
			entry = &snapshot_list[i];

			if (entry->killed || entry->jse_killtime) {
				  
                                           
                                               
                                   
       
				assert(entry->killed && entry->jse_killtime);
				break;
			}

			  
                                                     
      

			entry->killed       = kill_cause;
			entry->jse_killtime = killtime;
			entry->jse_gencount = snapshot->js_gencount;
			entry->jse_idle_delta = p->p_memstat_idle_delta;
#if CONFIG_FREEZE
			entry->jse_thaw_count = p->p_memstat_thaw_count;
			entry->jse_freeze_skip_reason = p->p_memstat_freeze_skip_reason;
#else                    
			entry->jse_thaw_count = 0;
			entry->jse_freeze_skip_reason = kMemorystatusFreezeSkipReasonNone;
#endif                    

			  
                                                             
                                                        
      
			if (entry->priority != p->p_memstat_effectivepriority) {
				strlcpy(entry->name, p->p_name, sizeof(entry->name));
				entry->priority  = p->p_memstat_effectivepriority;
				entry->state     = memorystatus_build_state(p);
				entry->user_data = p->p_memstat_userdata;
				entry->fds       = p->p_fd->fd_nfiles;
			}

			  
                                              
      

			uint32_t pages              = 0;
			uint32_t max_pages_lifetime = 0;
			uint32_t purgeable_pages    = 0;

			memorystatus_get_task_page_counts(p->task, &pages, &max_pages_lifetime, &purgeable_pages);
			entry->pages              = (uint64_t)pages;
			entry->max_pages_lifetime = (uint64_t)max_pages_lifetime;
			entry->purgeable_pages    = (uint64_t)purgeable_pages;

			uint64_t internal_pages                        = 0;
			uint64_t internal_compressed_pages             = 0;
			uint64_t purgeable_nonvolatile_pages           = 0;
			uint64_t purgeable_nonvolatile_compressed_pages = 0;
			uint64_t alternate_accounting_pages            = 0;
			uint64_t alternate_accounting_compressed_pages = 0;
			uint64_t iokit_mapped_pages                    = 0;
			uint64_t page_table_pages                      = 0;
			uint64_t frozen_to_swap_pages                  = 0;

			memorystatus_get_task_phys_footprint_page_counts(p->task, &internal_pages, &internal_compressed_pages,
			    &purgeable_nonvolatile_pages, &purgeable_nonvolatile_compressed_pages,
			    &alternate_accounting_pages, &alternate_accounting_compressed_pages,
			    &iokit_mapped_pages, &page_table_pages, &frozen_to_swap_pages);

			entry->jse_internal_pages = internal_pages;
			entry->jse_internal_compressed_pages = internal_compressed_pages;
			entry->jse_purgeable_nonvolatile_pages = purgeable_nonvolatile_pages;
			entry->jse_purgeable_nonvolatile_compressed_pages = purgeable_nonvolatile_compressed_pages;
			entry->jse_alternate_accounting_pages = alternate_accounting_pages;
			entry->jse_alternate_accounting_compressed_pages = alternate_accounting_compressed_pages;
			entry->jse_iokit_mapped_pages = iokit_mapped_pages;
			entry->jse_page_table_pages = page_table_pages;
			entry->jse_frozen_to_swap_pages = frozen_to_swap_pages;

			uint64_t region_count = 0;
			memorystatus_get_task_memory_region_count(p->task, &region_count);
			entry->jse_memory_region_count = region_count;

			goto exit;
		}
	}

	if (entry == NULL) {
		  
                                                                      
                                                 
                                       
     
		if (memorystatus_jetsam_snapshot_count < memorystatus_jetsam_snapshot_max) {
			  
                                        
                                            
      
			assert(memorystatus_jetsam_snapshot_count == snapshot->entry_count);

			if (memorystatus_init_jetsam_snapshot_entry_with_kill_locked(snapshot, p, kill_cause, killtime, &entry)) {
				memorystatus_jetsam_snapshot_count++;

				if (memorystatus_jetsam_snapshot_count >= memorystatus_jetsam_snapshot_max) {
					  
                                                          
                                                       
                                         
        
					printf("memorystatus: WARNING snapshot buffer is full, count %d\n",
					    memorystatus_jetsam_snapshot_count);
				}
			}
		}
	}

exit:
	if (entry) {
#if CONFIG_FREEZE
		if (memorystatus_jetsam_use_freezer_snapshot && isApp(p)) {
			                                                                                                                 
			copied_to_freezer_snapshot = memorystatus_jetsam_snapshot_copy_entry_locked(memorystatus_jetsam_snapshot_freezer, memorystatus_jetsam_snapshot_freezer_max, entry);
			if (copied_to_freezer_snapshot && memorystatus_jetsam_snapshot_freezer->entry_count == memorystatus_jetsam_snapshot_freezer_max) {
				  
                                                                 
                                                      
                                        
       
				os_log_error(OS_LOG_DEFAULT, "memorystatus: WARNING freezer snapshot buffer is full, count %zu",
				    memorystatus_jetsam_snapshot_freezer->entry_count);
			}
		}
#endif                    
	} else {
		  
                                                                
                                                                 
                                           
    
                                                      
                                                                
     

		MEMORYSTATUS_DEBUG(4, "memorystatus_update_jetsam_snapshot_entry_locked: failed to update pid %d, priority %d, count %d\n",
		    p->p_pid, p->p_memstat_effectivepriority, memorystatus_jetsam_snapshot_count);

#if CONFIG_FREEZE
		                                                             
		if (memorystatus_jetsam_use_freezer_snapshot && isApp(p)) {
			snapshot = memorystatus_jetsam_snapshot_freezer;
			if (snapshot->entry_count < memorystatus_jetsam_snapshot_freezer_max) {
				copied_to_freezer_snapshot = memorystatus_init_jetsam_snapshot_entry_with_kill_locked(snapshot, p, kill_cause, killtime, &entry);
				if (copied_to_freezer_snapshot && memorystatus_jetsam_snapshot_freezer->entry_count == memorystatus_jetsam_snapshot_freezer_max) {
					  
                                                                  
                                                       
                                         
        
					os_log_error(OS_LOG_DEFAULT, "memorystatus: WARNING freezer snapshot buffer is full, count %zu",
					    memorystatus_jetsam_snapshot_freezer->entry_count);
				}
			}
		}
#endif                    
	}

	return;
}

#if CONFIG_JETSAM
void
memorystatus_pages_update(unsigned int pages_avail)
{
	memorystatus_available_pages = pages_avail;

#if VM_PRESSURE_EVENTS
	  
                                                         
                                                     
                                                 
                                                      
    
	vm_pressure_response();

	if (memorystatus_available_pages <= memorystatus_available_pages_pressure) {
		if (memorystatus_hwm_candidates || (memorystatus_available_pages <= memorystatus_available_pages_critical)) {
			memorystatus_thread_wake();
		}
	}
#if CONFIG_FREEZE
	  
                                                                                                     
                                                                                                     
                                                                                                          
                                                              
    

	if (memorystatus_freeze_thread_should_run() == TRUE) {
		  
                                                                                                    
                                                                                              
     
		if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
			thread_wakeup((event_t)&memorystatus_freeze_wakeup);
		}
	}
#endif                    

#else                         

	boolean_t critical, delta;

	if (!memorystatus_delta) {
		return;
	}

	critical = (pages_avail < memorystatus_available_pages_critical) ? TRUE : FALSE;
	delta = ((pages_avail >= (memorystatus_available_pages + memorystatus_delta))
	    || (memorystatus_available_pages >= (pages_avail + memorystatus_delta))) ? TRUE : FALSE;

	if (critical || delta) {
		unsigned int total_pages;

		total_pages = (unsigned int) atop_64(max_mem);
#if CONFIG_SECLUDED_MEMORY
		total_pages -= vm_page_secluded_count;
#endif                             
		memorystatus_level = memorystatus_available_pages * 100 / total_pages;
		memorystatus_thread_wake();
	}
#endif                         
}
#endif                    

static boolean_t
memorystatus_init_jetsam_snapshot_entry_locked(proc_t p, memorystatus_jetsam_snapshot_entry_t *entry, uint64_t gencount)
{
	clock_sec_t                     tv_sec;
	clock_usec_t                    tv_usec;
	uint32_t pages = 0;
	uint32_t max_pages_lifetime = 0;
	uint32_t purgeable_pages = 0;
	uint64_t internal_pages                         = 0;
	uint64_t internal_compressed_pages              = 0;
	uint64_t purgeable_nonvolatile_pages            = 0;
	uint64_t purgeable_nonvolatile_compressed_pages = 0;
	uint64_t alternate_accounting_pages             = 0;
	uint64_t alternate_accounting_compressed_pages  = 0;
	uint64_t iokit_mapped_pages                     = 0;
	uint64_t page_table_pages                       = 0;
	uint64_t frozen_to_swap_pages                   = 0;
	uint64_t region_count                           = 0;
	uint64_t cids[COALITION_NUM_TYPES];

	memset(entry, 0, sizeof(memorystatus_jetsam_snapshot_entry_t));

	entry->pid = p->p_pid;
	strlcpy(&entry->name[0], p->p_name, sizeof(entry->name));
	entry->priority = p->p_memstat_effectivepriority;

	memorystatus_get_task_page_counts(p->task, &pages, &max_pages_lifetime, &purgeable_pages);
	entry->pages              = (uint64_t)pages;
	entry->max_pages_lifetime = (uint64_t)max_pages_lifetime;
	entry->purgeable_pages    = (uint64_t)purgeable_pages;

	memorystatus_get_task_phys_footprint_page_counts(p->task, &internal_pages, &internal_compressed_pages,
	    &purgeable_nonvolatile_pages, &purgeable_nonvolatile_compressed_pages,
	    &alternate_accounting_pages, &alternate_accounting_compressed_pages,
	    &iokit_mapped_pages, &page_table_pages, &frozen_to_swap_pages);

	entry->jse_internal_pages = internal_pages;
	entry->jse_internal_compressed_pages = internal_compressed_pages;
	entry->jse_purgeable_nonvolatile_pages = purgeable_nonvolatile_pages;
	entry->jse_purgeable_nonvolatile_compressed_pages = purgeable_nonvolatile_compressed_pages;
	entry->jse_alternate_accounting_pages = alternate_accounting_pages;
	entry->jse_alternate_accounting_compressed_pages = alternate_accounting_compressed_pages;
	entry->jse_iokit_mapped_pages = iokit_mapped_pages;
	entry->jse_page_table_pages = page_table_pages;
	entry->jse_frozen_to_swap_pages = frozen_to_swap_pages;

	memorystatus_get_task_memory_region_count(p->task, &region_count);
	entry->jse_memory_region_count = region_count;

	entry->state     = memorystatus_build_state(p);
	entry->user_data = p->p_memstat_userdata;
	memcpy(&entry->uuid[0], &p->p_uuid[0], sizeof(p->p_uuid));
	entry->fds       = p->p_fd->fd_nfiles;

	absolutetime_to_microtime(get_task_cpu_time(p->task), &tv_sec, &tv_usec);
	entry->cpu_time.tv_sec = (int64_t)tv_sec;
	entry->cpu_time.tv_usec = (int64_t)tv_usec;

	assert(p->p_stats != NULL);
	entry->jse_starttime =  p->p_stats->ps_start;                                
	entry->jse_killtime = 0;                                                                  
	entry->killed       = 0;                                                   
	entry->jse_gencount = gencount;                                                                                                     

	entry->jse_idle_delta = p->p_memstat_idle_delta;                                              

#if CONFIG_FREEZE
	entry->jse_freeze_skip_reason = p->p_memstat_freeze_skip_reason;
	entry->jse_thaw_count = p->p_memstat_thaw_count;
#else                    
	entry->jse_thaw_count = 0;
	entry->jse_freeze_skip_reason = kMemorystatusFreezeSkipReasonNone;
#endif                    

	proc_coalitionids(p, cids);
	entry->jse_coalition_jetsam_id = cids[COALITION_TYPE_JETSAM];

	return TRUE;
}

static void
memorystatus_init_snapshot_vmstats(memorystatus_jetsam_snapshot_t *snapshot)
{
	kern_return_t kr = KERN_SUCCESS;
	mach_msg_type_number_t  count = HOST_VM_INFO64_COUNT;
	vm_statistics64_data_t  vm_stat;

	if ((kr = host_statistics64(host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stat, &count)) != KERN_SUCCESS) {
		printf("memorystatus_init_jetsam_snapshot_stats: host_statistics64 failed with %d\n", kr);
		memset(&snapshot->stats, 0, sizeof(snapshot->stats));
	} else {
		snapshot->stats.free_pages      = vm_stat.free_count;
		snapshot->stats.active_pages    = vm_stat.active_count;
		snapshot->stats.inactive_pages  = vm_stat.inactive_count;
		snapshot->stats.throttled_pages = vm_stat.throttled_count;
		snapshot->stats.purgeable_pages = vm_stat.purgeable_count;
		snapshot->stats.wired_pages     = vm_stat.wire_count;

		snapshot->stats.speculative_pages = vm_stat.speculative_count;
		snapshot->stats.filebacked_pages  = vm_stat.external_page_count;
		snapshot->stats.anonymous_pages   = vm_stat.internal_page_count;
		snapshot->stats.compressions      = vm_stat.compressions;
		snapshot->stats.decompressions    = vm_stat.decompressions;
		snapshot->stats.compressor_pages  = vm_stat.compressor_page_count;
		snapshot->stats.total_uncompressed_pages_in_compressor = vm_stat.total_uncompressed_pages_in_compressor;
	}

	get_zone_map_size(&snapshot->stats.zone_map_size, &snapshot->stats.zone_map_capacity);

	bzero(snapshot->stats.largest_zone_name, sizeof(snapshot->stats.largest_zone_name));
	get_largest_zone_info(snapshot->stats.largest_zone_name, sizeof(snapshot->stats.largest_zone_name),
	    &snapshot->stats.largest_zone_size);
}

  
                                 
                                     
                                    
   
void
memorystatus_init_at_boot_snapshot()
{
	memorystatus_init_snapshot_vmstats(&memorystatus_at_boot_snapshot);
	memorystatus_at_boot_snapshot.entry_count = 0;
	memorystatus_at_boot_snapshot.notification_time = 0;                              
	memorystatus_at_boot_snapshot.snapshot_time = mach_absolute_time();
}

static void
memorystatus_init_jetsam_snapshot_header(memorystatus_jetsam_snapshot_t *snapshot)
{
	memorystatus_init_snapshot_vmstats(snapshot);
	snapshot->snapshot_time = mach_absolute_time();
	snapshot->notification_time = 0;
	snapshot->js_gencount = 0;
}

static void
memorystatus_init_jetsam_snapshot_locked(memorystatus_jetsam_snapshot_t *od_snapshot, uint32_t ods_list_count )
{
	proc_t p, next_p;
	unsigned int b = 0, i = 0;

	memorystatus_jetsam_snapshot_t *snapshot = NULL;
	memorystatus_jetsam_snapshot_entry_t *snapshot_list = NULL;
	unsigned int snapshot_max = 0;

	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_OWNED);

	if (od_snapshot) {
		  
                                  
     
		snapshot      = od_snapshot;
		snapshot_list = od_snapshot->entries;
		snapshot_max  = ods_list_count;
	} else {
		  
                                    
     
		snapshot      = memorystatus_jetsam_snapshot;
		snapshot_list = memorystatus_jetsam_snapshot->entries;
		snapshot_max  = memorystatus_jetsam_snapshot_max;
	}

	memorystatus_init_jetsam_snapshot_header(snapshot);

	next_p = memorystatus_get_first_proc_locked(&b, TRUE);
	while (next_p) {
		p = next_p;
		next_p = memorystatus_get_next_proc_locked(&b, p, TRUE);

		if (FALSE == memorystatus_init_jetsam_snapshot_entry_locked(p, &snapshot_list[i], snapshot->js_gencount)) {
			continue;
		}

		MEMORYSTATUS_DEBUG(0, "jetsam snapshot pid %d, uuid = %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\n",
		    p->p_pid,
		    p->p_uuid[0], p->p_uuid[1], p->p_uuid[2], p->p_uuid[3], p->p_uuid[4], p->p_uuid[5], p->p_uuid[6], p->p_uuid[7],
		    p->p_uuid[8], p->p_uuid[9], p->p_uuid[10], p->p_uuid[11], p->p_uuid[12], p->p_uuid[13], p->p_uuid[14], p->p_uuid[15]);

		if (++i == snapshot_max) {
			break;
		}
	}

	snapshot->entry_count = i;

	if (!od_snapshot) {
		                                    
		memorystatus_jetsam_snapshot_count = i;
	}
}

#if DEVELOPMENT || DEBUG

#if CONFIG_JETSAM
static int
memorystatus_cmd_set_panic_bits(user_addr_t buffer, size_t buffer_size)
{
	int ret;
	memorystatus_jetsam_panic_options_t debug;

	if (buffer_size != sizeof(memorystatus_jetsam_panic_options_t)) {
		return EINVAL;
	}

	ret = copyin(buffer, &debug, buffer_size);
	if (ret) {
		return ret;
	}

	                                                
	memorystatus_jetsam_panic_debug = (memorystatus_jetsam_panic_debug & ~debug.mask) | (debug.data & debug.mask);

	                       
	debug.data = memorystatus_jetsam_panic_debug;
	ret = copyout(&debug, buffer, sizeof(memorystatus_jetsam_panic_options_t));

	return ret;
}
#endif                    

  
                                                          
  
                                                             
                                                           
                  
  
                                                 
   
static int
memorystatus_verify_sort_order(unsigned int bucket_index, pid_t *expected_order, size_t num_pids)
{
	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_OWNED);

	int error = 0;
	proc_t p = NULL;
	size_t i = 0;

	  
                                                                     
                                                                                    
    
	p = memorystatus_get_first_proc_locked(&bucket_index, FALSE);
	while (p) {
		if (p->p_pid == expected_order[i]) {
			i++;
		}
		if (i == num_pids) {
			break;
		}
		p = memorystatus_get_next_proc_locked(&bucket_index, p, FALSE);
	}
	if (i != num_pids) {
		char buffer[128];
		size_t len = sizeof(buffer);
		size_t buffer_idx = 0;
		os_log_error(OS_LOG_DEFAULT, "memorystatus_verify_sort_order: Processes in bucket %d were not sorted properly\n", bucket_index);
		for (i = 0; i < num_pids; i++) {
			int num_written = snprintf(buffer + buffer_idx, len - buffer_idx, "%d,", expected_order[i]);
			if (num_written <= 0) {
				break;
			}
			if (buffer_idx + (unsigned int) num_written >= len) {
				break;
			}
			buffer_idx += num_written;
		}
		os_log_error(OS_LOG_DEFAULT, "memorystatus_verify_sort_order: Expected order [%s]", buffer);
		memset(buffer, 0, len);
		buffer_idx = 0;
		p = memorystatus_get_first_proc_locked(&bucket_index, FALSE);
		i = 0;
		os_log_error(OS_LOG_DEFAULT, "memorystatus_verify_sort_order: Actual order:");
		while (p) {
			int num_written;
			if (buffer_idx == 0) {
				num_written = snprintf(buffer + buffer_idx, len - buffer_idx, "%zu: %d,", i, p->p_pid);
			} else {
				num_written = snprintf(buffer + buffer_idx, len - buffer_idx, "%d,", p->p_pid);
			}
			if (num_written <= 0) {
				break;
			}
			buffer_idx += (unsigned int) num_written;
			assert(buffer_idx <= len);
			if (i % 10 == 0) {
				os_log_error(OS_LOG_DEFAULT, "memorystatus_verify_sort_order: %s", buffer);
				buffer_idx = 0;
			}
			p = memorystatus_get_next_proc_locked(&bucket_index, p, FALSE);
			i++;
		}
		if (buffer_idx != 0) {
			os_log_error(OS_LOG_DEFAULT, "memorystatus_verify_sort_order: %s", buffer);
		}
		error = EINVAL;
	}
	return error;
}

  
                                                             
                                                                  
            
   
static int
memorystatus_cmd_test_jetsam_sort(int priority,
    int sort_order,
    user_addr_t expected_order_user,
    size_t expected_order_user_len)
{
	int error = 0;
	unsigned int bucket_index = 0;
	static size_t kMaxPids = 8;
	pid_t expected_order[kMaxPids];
	size_t copy_size = sizeof(expected_order);
	size_t num_pids;

	if (expected_order_user_len < copy_size) {
		copy_size = expected_order_user_len;
	}
	num_pids = copy_size / sizeof(pid_t);

	error = copyin(expected_order_user, expected_order, copy_size);
	if (error != 0) {
		return error;
	}

	if (priority == -1) {
		                                           
		bucket_index = JETSAM_PRIORITY_DEFAULT;
	} else {
		bucket_index = (unsigned int)priority;
	}

	  
                                                              
                                 
    
	proc_list_lock();

	memorystatus_sort_bucket_locked(bucket_index, sort_order);

	if (expected_order_user != CAST_USER_ADDR_T(NULL) && expected_order_user_len > 0) {
		error = memorystatus_verify_sort_order(bucket_index, expected_order, num_pids);
	}

	proc_list_unlock();

	return error;
}

#endif                           

  
                                                                             
   
static uint64_t memorystatus_purge_before_jetsam_success = 0;

static boolean_t
memorystatus_kill_proc(proc_t p, uint32_t cause, os_reason_t jetsam_reason, boolean_t *killed, uint64_t *footprint_of_killed_proc)
{
	pid_t aPid = 0;
	uint32_t aPid_ep = 0;

	uint64_t        killtime = 0;
	clock_sec_t     tv_sec;
	clock_usec_t    tv_usec;
	uint32_t        tv_msec;
	boolean_t       retval = FALSE;

	aPid = p->p_pid;
	aPid_ep = p->p_memstat_effectivepriority;

	if (cause != kMemorystatusKilledVnodes && cause != kMemorystatusKilledZoneMapExhaustion) {
		  
                                                                            
     
		boolean_t success = FALSE;
		uint64_t num_pages_purged;
		uint64_t num_pages_reclaimed = 0;
		uint64_t num_pages_unsecluded = 0;

		networking_memstatus_callout(p, cause);
		num_pages_purged = vm_purgeable_purge_task_owned(p->task);
		num_pages_reclaimed += num_pages_purged;
#if CONFIG_SECLUDED_MEMORY
		if (cause == kMemorystatusKilledVMPageShortage &&
		    vm_page_secluded_count > 0 &&
		    task_can_use_secluded_mem(p->task, FALSE)) {
			  
                                                   
                                                     
                                                         
                                                        
                           
                                                         
                                                  
              
      
			num_pages_unsecluded = vm_page_secluded_drain();
			num_pages_reclaimed += num_pages_unsecluded;
		}
#endif                             

		if (num_pages_reclaimed) {
			  
                                                  
                                                 
      
			if (cause == kMemorystatusKilledHiwat) {
				uint64_t footprint_in_bytes = get_task_phys_footprint(p->task);
				uint64_t memlimit_in_bytes  = (((uint64_t)p->p_memstat_memlimit) * 1024ULL * 1024ULL);                           
				success = (footprint_in_bytes <= memlimit_in_bytes);
			} else {
				success = (memorystatus_avail_pages_below_pressure() == FALSE);
#if CONFIG_SECLUDED_MEMORY
				if (!success && num_pages_unsecluded) {
					  
                                         
                                     
                                      
                                        
                                      
                                        
                                         
                                       
                                      
                                       
        
					success = (memorystatus_avail_pages_below_critical() == FALSE);
				}
#endif                             
			}

			if (success) {
				memorystatus_purge_before_jetsam_success++;

				os_log_with_startup_serial(OS_LOG_DEFAULT, "memorystatus: reclaimed %llu pages (%llu purged, %llu unsecluded) from pid %d [%s] and avoided %s\n",
				    num_pages_reclaimed, num_pages_purged, num_pages_unsecluded, aPid, ((p && *p->p_name) ? p->p_name : "unknown"), memorystatus_kill_cause_name[cause]);

				*killed = FALSE;

				return TRUE;
			}
		}
	}

#if CONFIG_JETSAM && (DEVELOPMENT || DEBUG)
	MEMORYSTATUS_DEBUG(1, "jetsam: killing pid %d [%s] - %lld Mb > 1 (%d Mb)\n",
	    aPid, (*p->p_name ? p->p_name : "unknown"),
	    (footprint_in_bytes / (1024ULL * 1024ULL)),                                            
	    p->p_memstat_memlimit);
#endif                                              

	killtime = mach_absolute_time();
	absolutetime_to_microtime(killtime, &tv_sec, &tv_usec);
	tv_msec = tv_usec / 1000;

	proc_list_lock();
	memorystatus_update_jetsam_snapshot_entry_locked(p, cause, killtime);
	proc_list_unlock();

	char kill_reason_string[128];

	if (cause == kMemorystatusKilledHiwat) {
		strlcpy(kill_reason_string, "killing_highwater_process", 128);
	} else {
		if (aPid_ep == JETSAM_PRIORITY_IDLE) {
			strlcpy(kill_reason_string, "killing_idle_process", 128);
		} else {
			strlcpy(kill_reason_string, "killing_top_process", 128);
		}
	}

	  
                                                                         
                                                                      
           
    
	os_reason_ref(jetsam_reason);

	retval = memorystatus_do_kill(p, cause, jetsam_reason, footprint_of_killed_proc);
	*killed = retval;

	os_log_with_startup_serial(OS_LOG_DEFAULT, "%lu.%03d memorystatus: %s pid %d [%s] (%s %d) %lluKB - memorystatus_available_pages: %llu",
	    (unsigned long)tv_sec, tv_msec, kill_reason_string,
	    aPid, ((p && *p->p_name) ? p->p_name : "unknown"),
	    memorystatus_kill_cause_name[cause], aPid_ep,
	    (*footprint_of_killed_proc) >> 10, (uint64_t)MEMORYSTATUS_LOG_AVAILABLE_PAGES);

	return retval;
}

  
                                         
   
static boolean_t
memorystatus_kill_top_process(boolean_t any, boolean_t sort_flag, uint32_t cause, os_reason_t jetsam_reason,
    int32_t *priority, uint32_t *errors, uint64_t *memory_reclaimed)
{
	pid_t aPid;
	proc_t p = PROC_NULL, next_p = PROC_NULL;
	boolean_t new_snapshot = FALSE, force_new_snapshot = FALSE, killed = FALSE, freed_mem = FALSE;
	unsigned int i = 0;
	uint32_t aPid_ep;
	int32_t local_max_kill_prio = JETSAM_PRIORITY_IDLE;
	uint64_t footprint_of_killed_proc = 0;

#ifndef CONFIG_FREEZE
#pragma unused(any)
#endif

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_JETSAM) | DBG_FUNC_START,
	    MEMORYSTATUS_LOG_AVAILABLE_PAGES, 0, 0, 0, 0);


#if CONFIG_JETSAM
	if (sort_flag == TRUE) {
		(void)memorystatus_sort_bucket(JETSAM_PRIORITY_FOREGROUND, JETSAM_SORT_DEFAULT);
	}

	local_max_kill_prio = max_kill_priority;

	force_new_snapshot = FALSE;

#else                    

	if (sort_flag == TRUE) {
		(void)memorystatus_sort_bucket(JETSAM_PRIORITY_IDLE, JETSAM_SORT_DEFAULT);
	}

	  
                                                          
   
                                        
       
                                                
   
                                                                                    
                                                                                     
   
                                                                                         
                                                                                          
                               
    
	if (cause == kMemorystatusKilledZoneMapExhaustion) {
		local_max_kill_prio = JETSAM_PRIORITY_MAX;
	} else {
		local_max_kill_prio = max_kill_priority;
	}

	  
                                                                                      
               
    
	force_new_snapshot = TRUE;

#endif                    

	if (cause != kMemorystatusKilledZoneMapExhaustion &&
	    jetsam_current_thread() != NULL &&
	    jetsam_current_thread()->limit_to_low_bands &&
	    local_max_kill_prio > JETSAM_PRIORITY_BACKGROUND) {
		local_max_kill_prio = JETSAM_PRIORITY_BACKGROUND;
	}

	proc_list_lock();

	next_p = memorystatus_get_first_proc_locked(&i, TRUE);
	while (next_p && (next_p->p_memstat_effectivepriority <= local_max_kill_prio)) {
		p = next_p;
		next_p = memorystatus_get_next_proc_locked(&i, p, TRUE);


		aPid = p->p_pid;
		aPid_ep = p->p_memstat_effectivepriority;

		if (p->p_memstat_state & (P_MEMSTAT_ERROR | P_MEMSTAT_TERMINATED)) {
			continue;                       
		}

		if (cause == kMemorystatusKilledVnodes) {
			  
                                                                
                                                                  
                                                              
                                                                
                                                          
      

			if (p == current_proc()) {
				                                       
				continue;
			}
		}

#if CONFIG_FREEZE
		boolean_t skip;
		boolean_t reclaim_proc = !(p->p_memstat_state & P_MEMSTAT_LOCKED);
		if (any || reclaim_proc) {
			skip = FALSE;
		} else {
			skip = TRUE;
		}

		if (skip) {
			continue;
		} else
#endif
		{
			if (proc_ref_locked(p) == p) {
				  
                                                                                             
                                                                                         
                                                                                            
                                    
       
				p->p_memstat_state |= P_MEMSTAT_TERMINATED;
			} else {
				  
                                                  
                                                    
                                                   
                                 
       
				i = 0;
				next_p = memorystatus_get_first_proc_locked(&i, TRUE);
				continue;
			}

			  
                                            
                                                               
                                                                            
                                                              
                                                                                 
                                                                                     
      
			if ((memorystatus_jetsam_snapshot_count == 0) &&
			    (force_new_snapshot || memorystatus_idle_snapshot || ((!priority) || (priority && (aPid_ep != JETSAM_PRIORITY_IDLE))))) {
				memorystatus_init_jetsam_snapshot_locked(NULL, 0);
				new_snapshot = TRUE;
			}

			proc_list_unlock();

			freed_mem = memorystatus_kill_proc(p, cause, jetsam_reason, &killed, &footprint_of_killed_proc);                               
			              
			if (freed_mem) {
				if (killed) {
					*memory_reclaimed = footprint_of_killed_proc;
					if (priority) {
						*priority = aPid_ep;
					}
				} else {
					            
					proc_list_lock();
					p->p_memstat_state &= ~P_MEMSTAT_TERMINATED;
					proc_list_unlock();
				}
				proc_rele(p);
				goto exit;
			}

			  
                                       
                                              
      
			proc_list_lock();
			proc_rele_locked(p);
			p->p_memstat_state &= ~P_MEMSTAT_TERMINATED;
			p->p_memstat_state |= P_MEMSTAT_ERROR;
			*errors += 1;

			i = 0;
			next_p = memorystatus_get_first_proc_locked(&i, TRUE);
		}
	}

	proc_list_unlock();

exit:
	os_reason_free(jetsam_reason);

	if (!killed) {
		*memory_reclaimed = 0;

		                                                                
		if (new_snapshot) {
			proc_list_lock();
			memorystatus_jetsam_snapshot->entry_count = memorystatus_jetsam_snapshot_count = 0;
			proc_list_unlock();
		}
	}

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_JETSAM) | DBG_FUNC_END,
	    MEMORYSTATUS_LOG_AVAILABLE_PAGES, killed ? aPid : 0, killed, *memory_reclaimed, 0);

	return killed;
}

  
                      
   
static boolean_t
memorystatus_kill_processes_aggressive(uint32_t cause, int aggr_count,
    int32_t priority_max, uint32_t *errors, uint64_t *memory_reclaimed)
{
	pid_t aPid;
	proc_t p = PROC_NULL, next_p = PROC_NULL;
	boolean_t new_snapshot = FALSE, killed = FALSE;
	int kill_count = 0;
	unsigned int i = 0;
	int32_t aPid_ep = 0;
	unsigned int memorystatus_level_snapshot = 0;
	uint64_t killtime = 0;
	clock_sec_t     tv_sec;
	clock_usec_t    tv_usec;
	uint32_t        tv_msec;
	os_reason_t jetsam_reason = OS_REASON_NULL;
	uint64_t footprint_of_killed_proc = 0;

	*memory_reclaimed = 0;

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_JETSAM) | DBG_FUNC_START,
	    MEMORYSTATUS_LOG_AVAILABLE_PAGES, priority_max, 0, 0, 0);

	if (priority_max >= JETSAM_PRIORITY_FOREGROUND) {
		  
                                                                         
                                                                         
                         
     
		memorystatus_sort_bucket(JETSAM_PRIORITY_FOREGROUND, JETSAM_SORT_DEFAULT);
	}

	jetsam_reason = os_reason_create(OS_REASON_JETSAM, cause);
	if (jetsam_reason == OS_REASON_NULL) {
		printf("memorystatus_kill_processes_aggressive: failed to allocate exit reason\n");
	}

	proc_list_lock();

	next_p = memorystatus_get_first_proc_locked(&i, TRUE);
	while (next_p) {
		if (((next_p->p_listflag & P_LIST_EXITED) != 0) ||
		    ((unsigned int)(next_p->p_memstat_effectivepriority) != i)) {
			  
                                                        
                                                           
                                                        
                                                         
                                                                  
                                                                     
                                                                    
                                                                      
      

			MEMORYSTATUS_DEBUG(1, "memorystatus: aggressive%d: rewinding band %d, %s(%d) moved or exiting.\n",
			    aggr_count, i, (*next_p->p_name ? next_p->p_name : "unknown"), next_p->p_pid);

			next_p = memorystatus_get_first_proc_locked(&i, TRUE);
			continue;
		}

		p = next_p;
		next_p = memorystatus_get_next_proc_locked(&i, p, TRUE);

		if (p->p_memstat_effectivepriority > priority_max) {
			  
                                               
                                                  
                                            
                               
      
			proc_list_unlock();
			goto exit;
		}

		aPid = p->p_pid;
		aPid_ep = p->p_memstat_effectivepriority;

		if (p->p_memstat_state & (P_MEMSTAT_ERROR | P_MEMSTAT_TERMINATED)) {
			continue;
		}

		  
                                       
     
		if (memorystatus_jetsam_snapshot_count == 0) {
			memorystatus_init_jetsam_snapshot_locked(NULL, 0);
			new_snapshot = TRUE;
		}

		  
                                                                                           
                                                                                       
                                                                                          
                                  
     
		p->p_memstat_state |= P_MEMSTAT_TERMINATED;

		killtime = mach_absolute_time();
		absolutetime_to_microtime(killtime, &tv_sec, &tv_usec);
		tv_msec = tv_usec / 1000;

		                               
		memorystatus_update_jetsam_snapshot_entry_locked(p, cause, killtime);

		  
                                                                          
                                                                             
                                
                                                                    
                                          
     
		if (proc_ref_locked(p) == p) {
			if (next_p) {
				while (next_p && (proc_ref_locked(next_p) != next_p)) {
					proc_t temp_p;

					  
                                                               
                                                                 
        

					MEMORYSTATUS_DEBUG(1, "memorystatus: aggressive%d: skipping %d [%s] (exiting?)\n",
					    aggr_count, next_p->p_pid, (*next_p->p_name ? next_p->p_name : "(unknown)"));

					temp_p = next_p;
					next_p = memorystatus_get_next_proc_locked(&i, temp_p, TRUE);
				}
			}
			proc_list_unlock();

			printf("%lu.%03d memorystatus: %s%d pid %d [%s] (%s %d) - memorystatus_available_pages: %llu\n",
			    (unsigned long)tv_sec, tv_msec,
			    ((aPid_ep == JETSAM_PRIORITY_IDLE) ? "killing_idle_process_aggressive" : "killing_top_process_aggressive"),
			    aggr_count, aPid, (*p->p_name ? p->p_name : "unknown"),
			    memorystatus_kill_cause_name[cause], aPid_ep, (uint64_t)MEMORYSTATUS_LOG_AVAILABLE_PAGES);

			memorystatus_level_snapshot = memorystatus_level;

			  
                                                                             
                                                                        
              
      
			os_reason_ref(jetsam_reason);
			killed = memorystatus_do_kill(p, cause, jetsam_reason, &footprint_of_killed_proc);

			              
			if (killed) {
				*memory_reclaimed += footprint_of_killed_proc;
				proc_rele(p);
				kill_count++;
				p = NULL;
				killed = FALSE;

				  
                                  
       
				proc_list_lock();
				if (next_p) {
					proc_rele_locked(next_p);
				}

				if (aPid_ep == JETSAM_PRIORITY_FOREGROUND && memorystatus_aggressive_jetsam_lenient == TRUE) {
					if (memorystatus_level > memorystatus_level_snapshot && ((memorystatus_level - memorystatus_level_snapshot) >= AGGRESSIVE_JETSAM_LENIENT_MODE_THRESHOLD)) {
#if DEVELOPMENT || DEBUG
						printf("Disabling Lenient mode after one-time deployment.\n");
#endif                           
						memorystatus_aggressive_jetsam_lenient = FALSE;
						break;
					}
				}

				continue;
			}

			  
                                       
                                              
      
			proc_list_lock();
			proc_rele_locked(p);
			if (next_p) {
				proc_rele_locked(next_p);
			}
			p->p_memstat_state &= ~P_MEMSTAT_TERMINATED;
			p->p_memstat_state |= P_MEMSTAT_ERROR;
			*errors += 1;
			p = NULL;
		}

		  
                                                     
                                         
    
                                                                          
                                                     
    
                                                              
                                                   
    
                                                       
     

		next_p = memorystatus_get_first_proc_locked(&i, TRUE);
	}

	proc_list_unlock();

exit:
	os_reason_free(jetsam_reason);

	                                                                
	if (new_snapshot && (kill_count == 0)) {
		proc_list_lock();
		memorystatus_jetsam_snapshot->entry_count = memorystatus_jetsam_snapshot_count = 0;
		proc_list_unlock();
	}

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_JETSAM) | DBG_FUNC_END,
	    MEMORYSTATUS_LOG_AVAILABLE_PAGES, 0, kill_count, *memory_reclaimed, 0);

	if (kill_count > 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static boolean_t
memorystatus_kill_hiwat_proc(uint32_t *errors, boolean_t *purged, uint64_t *memory_reclaimed)
{
	pid_t aPid = 0;
	proc_t p = PROC_NULL, next_p = PROC_NULL;
	boolean_t new_snapshot = FALSE, killed = FALSE, freed_mem = FALSE;
	unsigned int i = 0;
	uint32_t aPid_ep;
	os_reason_t jetsam_reason = OS_REASON_NULL;
	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_JETSAM_HIWAT) | DBG_FUNC_START,
	    MEMORYSTATUS_LOG_AVAILABLE_PAGES, 0, 0, 0, 0);

	jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_MEMORY_HIGHWATER);
	if (jetsam_reason == OS_REASON_NULL) {
		printf("memorystatus_kill_hiwat_proc: failed to allocate exit reason\n");
	}

	proc_list_lock();

	next_p = memorystatus_get_first_proc_locked(&i, TRUE);
	while (next_p) {
		uint64_t footprint_in_bytes = 0;
		uint64_t memlimit_in_bytes  = 0;
		boolean_t skip = 0;

		p = next_p;
		next_p = memorystatus_get_next_proc_locked(&i, p, TRUE);

		aPid = p->p_pid;
		aPid_ep = p->p_memstat_effectivepriority;

		if (p->p_memstat_state  & (P_MEMSTAT_ERROR | P_MEMSTAT_TERMINATED)) {
			continue;
		}

		                          
		if (p->p_memstat_memlimit <= 0) {
			continue;
		}

		footprint_in_bytes = get_task_phys_footprint(p->task);
		memlimit_in_bytes  = (((uint64_t)p->p_memstat_memlimit) * 1024ULL * 1024ULL);                            
		skip = (footprint_in_bytes <= memlimit_in_bytes);

#if CONFIG_FREEZE
		if (!skip) {
			if (p->p_memstat_state & P_MEMSTAT_LOCKED) {
				skip = TRUE;
			} else {
				skip = FALSE;
			}
		}
#endif

		if (skip) {
			continue;
		} else {
			if (memorystatus_jetsam_snapshot_count == 0) {
				memorystatus_init_jetsam_snapshot_locked(NULL, 0);
				new_snapshot = TRUE;
			}

			if (proc_ref_locked(p) == p) {
				  
                                                                                             
                                                                                         
                                                                                            
                                    
       
				p->p_memstat_state |= P_MEMSTAT_TERMINATED;

				proc_list_unlock();
			} else {
				  
                                                  
                                                    
                                                   
                                 
       
				i = 0;
				next_p = memorystatus_get_first_proc_locked(&i, TRUE);
				continue;
			}

			footprint_in_bytes = 0;
			freed_mem = memorystatus_kill_proc(p, kMemorystatusKilledHiwat, jetsam_reason, &killed, &footprint_in_bytes);                               

			              
			if (freed_mem) {
				if (killed == FALSE) {
					                                                 
					*purged = TRUE;

					proc_list_lock();
					p->p_memstat_state &= ~P_MEMSTAT_TERMINATED;
					proc_list_unlock();
				} else {
					*memory_reclaimed = footprint_in_bytes;
				}
				proc_rele(p);
				goto exit;
			}
			  
                                       
                                              
      
			proc_list_lock();
			proc_rele_locked(p);
			p->p_memstat_state &= ~P_MEMSTAT_TERMINATED;
			p->p_memstat_state |= P_MEMSTAT_ERROR;
			*errors += 1;

			i = 0;
			next_p = memorystatus_get_first_proc_locked(&i, TRUE);
		}
	}

	proc_list_unlock();

exit:
	os_reason_free(jetsam_reason);

	if (!killed) {
		*memory_reclaimed = 0;

		                                                                
		if (new_snapshot) {
			proc_list_lock();
			memorystatus_jetsam_snapshot->entry_count = memorystatus_jetsam_snapshot_count = 0;
			proc_list_unlock();
		}
	}

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_JETSAM_HIWAT) | DBG_FUNC_END,
	    MEMORYSTATUS_LOG_AVAILABLE_PAGES, killed ? aPid : 0, killed, *memory_reclaimed, 0);

	return killed;
}

  
                                                
  
                                                  
                                               
   
boolean_t
memorystatus_kill_elevated_process(uint32_t cause, os_reason_t jetsam_reason, unsigned int band, int aggr_count, uint32_t *errors, uint64_t *memory_reclaimed)
{
	pid_t aPid = 0;
	proc_t p = PROC_NULL, next_p = PROC_NULL;
	boolean_t new_snapshot = FALSE, killed = FALSE;
	int kill_count = 0;
	uint32_t aPid_ep;
	uint64_t killtime = 0;
	clock_sec_t     tv_sec;
	clock_usec_t    tv_usec;
	uint32_t        tv_msec;
	uint64_t footprint_of_killed_proc = 0;


	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_JETSAM) | DBG_FUNC_START,
	    MEMORYSTATUS_LOG_AVAILABLE_PAGES, 0, 0, 0, 0);

#if CONFIG_FREEZE
	boolean_t consider_frozen_only = FALSE;

	if (band == (unsigned int) memorystatus_freeze_jetsam_band) {
		consider_frozen_only = TRUE;
	}
#endif                    

	proc_list_lock();

	next_p = memorystatus_get_first_proc_locked(&band, FALSE);
	while (next_p) {
		p = next_p;
		next_p = memorystatus_get_next_proc_locked(&band, p, FALSE);

		aPid = p->p_pid;
		aPid_ep = p->p_memstat_effectivepriority;

		  
                                                     
     
		if (!(p->p_memstat_state & P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND)) {
			continue;
		}

		if (p->p_memstat_state  & (P_MEMSTAT_ERROR | P_MEMSTAT_TERMINATED)) {
			continue;
		}

#if CONFIG_FREEZE
		if (consider_frozen_only && !(p->p_memstat_state & P_MEMSTAT_FROZEN)) {
			continue;
		}

		if (p->p_memstat_state & P_MEMSTAT_LOCKED) {
			continue;
		}
#endif                    

#if DEVELOPMENT || DEBUG
		MEMORYSTATUS_DEBUG(1, "jetsam: elevated%d process pid %d [%s] - memorystatus_available_pages: %d\n",
		    aggr_count,
		    aPid, (*p->p_name ? p->p_name : "unknown"),
		    MEMORYSTATUS_LOG_AVAILABLE_PAGES);
#endif                           

		if (memorystatus_jetsam_snapshot_count == 0) {
			memorystatus_init_jetsam_snapshot_locked(NULL, 0);
			new_snapshot = TRUE;
		}

		p->p_memstat_state |= P_MEMSTAT_TERMINATED;

		killtime = mach_absolute_time();
		absolutetime_to_microtime(killtime, &tv_sec, &tv_usec);
		tv_msec = tv_usec / 1000;

		memorystatus_update_jetsam_snapshot_entry_locked(p, cause, killtime);

		if (proc_ref_locked(p) == p) {
			proc_list_unlock();

			  
                                                                           
                                                                        
             
      
			os_reason_ref(jetsam_reason);
			killed = memorystatus_do_kill(p, cause, jetsam_reason, &footprint_of_killed_proc);

			os_log_with_startup_serial(OS_LOG_DEFAULT, "%lu.%03d memorystatus: killing_top_process_elevated%d pid %d [%s] (%s %d) %lluKB - memorystatus_available_pages: %llu\n",
			    (unsigned long)tv_sec, tv_msec,
			    aggr_count,
			    aPid, ((p && *p->p_name) ? p->p_name : "unknown"),
			    memorystatus_kill_cause_name[cause], aPid_ep,
			    footprint_of_killed_proc >> 10, (uint64_t)MEMORYSTATUS_LOG_AVAILABLE_PAGES);

			              
			if (killed) {
				*memory_reclaimed = footprint_of_killed_proc;
				proc_rele(p);
				kill_count++;
				goto exit;
			}

			  
                                       
                                              
      
			proc_list_lock();
			proc_rele_locked(p);
			p->p_memstat_state &= ~P_MEMSTAT_TERMINATED;
			p->p_memstat_state |= P_MEMSTAT_ERROR;
			*errors += 1;
		}

		  
                                  
    
                                                                          
                                                     
    
                                                              
                                                         
    
                                                       
     

		next_p = memorystatus_get_first_proc_locked(&band, FALSE);
	}

	proc_list_unlock();

exit:
	os_reason_free(jetsam_reason);

	if (kill_count == 0) {
		*memory_reclaimed = 0;

		                                                                
		if (new_snapshot) {
			proc_list_lock();
			memorystatus_jetsam_snapshot->entry_count = memorystatus_jetsam_snapshot_count = 0;
			proc_list_unlock();
		}
	}

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_JETSAM) | DBG_FUNC_END,
	    MEMORYSTATUS_LOG_AVAILABLE_PAGES, killed ? aPid : 0, kill_count, *memory_reclaimed, 0);

	return killed;
}

static boolean_t
memorystatus_kill_process_async(pid_t victim_pid, uint32_t cause)
{
	  
                                    
   
                                                                                          
                                                 
    
	if ((victim_pid != -1) ||
	    (cause != kMemorystatusKilledVMPageShortage &&
	    cause != kMemorystatusKilledVMCompressorThrashing &&
	    cause != kMemorystatusKilledVMCompressorSpaceShortage &&
	    cause != kMemorystatusKilledFCThrashing &&
	    cause != kMemorystatusKilledZoneMapExhaustion)) {
		return FALSE;
	}

	kill_under_pressure_cause = cause;
	memorystatus_thread_wake();
	return TRUE;
}

boolean_t
memorystatus_kill_on_VM_compressor_space_shortage(boolean_t async)
{
	if (async) {
		return memorystatus_kill_process_async(-1, kMemorystatusKilledVMCompressorSpaceShortage);
	} else {
		os_reason_t jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_MEMORY_VMCOMPRESSOR_SPACE_SHORTAGE);
		if (jetsam_reason == OS_REASON_NULL) {
			printf("memorystatus_kill_on_VM_compressor_space_shortage -- sync: failed to allocate jetsam reason\n");
		}

		return memorystatus_kill_process_sync(-1, kMemorystatusKilledVMCompressorSpaceShortage, jetsam_reason);
	}
}

#if CONFIG_JETSAM
boolean_t
memorystatus_kill_on_VM_compressor_thrashing(boolean_t async)
{
	if (async) {
		return memorystatus_kill_process_async(-1, kMemorystatusKilledVMCompressorThrashing);
	} else {
		os_reason_t jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_MEMORY_VMCOMPRESSOR_THRASHING);
		if (jetsam_reason == OS_REASON_NULL) {
			printf("memorystatus_kill_on_VM_compressor_thrashing -- sync: failed to allocate jetsam reason\n");
		}

		return memorystatus_kill_process_sync(-1, kMemorystatusKilledVMCompressorThrashing, jetsam_reason);
	}
}

boolean_t
memorystatus_kill_on_VM_page_shortage(boolean_t async)
{
	if (async) {
		return memorystatus_kill_process_async(-1, kMemorystatusKilledVMPageShortage);
	} else {
		os_reason_t jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_MEMORY_VMPAGESHORTAGE);
		if (jetsam_reason == OS_REASON_NULL) {
			printf("memorystatus_kill_on_VM_page_shortage -- sync: failed to allocate jetsam reason\n");
		}

		return memorystatus_kill_process_sync(-1, kMemorystatusKilledVMPageShortage, jetsam_reason);
	}
}

boolean_t
memorystatus_kill_on_FC_thrashing(boolean_t async)
{
	if (async) {
		return memorystatus_kill_process_async(-1, kMemorystatusKilledFCThrashing);
	} else {
		os_reason_t jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_MEMORY_FCTHRASHING);
		if (jetsam_reason == OS_REASON_NULL) {
			printf("memorystatus_kill_on_FC_thrashing -- sync: failed to allocate jetsam reason\n");
		}

		return memorystatus_kill_process_sync(-1, kMemorystatusKilledFCThrashing, jetsam_reason);
	}
}

boolean_t
memorystatus_kill_on_vnode_limit(void)
{
	os_reason_t jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_VNODE);
	if (jetsam_reason == OS_REASON_NULL) {
		printf("memorystatus_kill_on_vnode_limit: failed to allocate jetsam reason\n");
	}

	return memorystatus_kill_process_sync(-1, kMemorystatusKilledVnodes, jetsam_reason);
}

#endif                    

boolean_t
memorystatus_kill_on_zone_map_exhaustion(pid_t pid)
{
	boolean_t res = FALSE;
	if (pid == -1) {
		res = memorystatus_kill_process_async(-1, kMemorystatusKilledZoneMapExhaustion);
	} else {
		os_reason_t jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_ZONE_MAP_EXHAUSTION);
		if (jetsam_reason == OS_REASON_NULL) {
			printf("memorystatus_kill_on_zone_map_exhaustion: failed to allocate jetsam reason\n");
		}

		res = memorystatus_kill_process_sync(pid, kMemorystatusKilledZoneMapExhaustion, jetsam_reason);
	}
	return res;
}

void
memorystatus_on_pageout_scan_end(void)
{
	           
}

                                                                                                         
static int
memorystatus_get_priority_list(memorystatus_priority_entry_t **list_ptr, size_t *buffer_size, size_t *list_size, boolean_t size_only)
{
	uint32_t list_count, i = 0;
	memorystatus_priority_entry_t *list_entry;
	proc_t p;

	list_count = memorystatus_list_count;
	*list_size = sizeof(memorystatus_priority_entry_t) * list_count;

	                        
	if (size_only) {
		return 0;
	}

	                                                
	if (*buffer_size < *list_size) {
		return EINVAL;
	}

	*list_ptr = kheap_alloc(KHEAP_TEMP, *list_size, Z_WAITOK | Z_ZERO);
	if (!*list_ptr) {
		return ENOMEM;
	}

	*buffer_size = *list_size;
	*list_size = 0;

	list_entry = *list_ptr;

	proc_list_lock();

	p = memorystatus_get_first_proc_locked(&i, TRUE);
	while (p && (*list_size < *buffer_size)) {
		list_entry->pid = p->p_pid;
		list_entry->priority = p->p_memstat_effectivepriority;
		list_entry->user_data = p->p_memstat_userdata;

		if (p->p_memstat_memlimit <= 0) {
			task_get_phys_footprint_limit(p->task, &list_entry->limit);
		} else {
			list_entry->limit = p->p_memstat_memlimit;
		}

		list_entry->state = memorystatus_build_state(p);
		list_entry++;

		*list_size += sizeof(memorystatus_priority_entry_t);

		p = memorystatus_get_next_proc_locked(&i, p, TRUE);
	}

	proc_list_unlock();

	MEMORYSTATUS_DEBUG(1, "memorystatus_get_priority_list: returning %lu for size\n", (unsigned long)*list_size);

	return 0;
}

static int
memorystatus_get_priority_pid(pid_t pid, user_addr_t buffer, size_t buffer_size)
{
	int error = 0;
	memorystatus_priority_entry_t mp_entry;
	kern_return_t ret;

	                     
	if ((pid == 0) || (buffer == USER_ADDR_NULL) || (buffer_size != sizeof(memorystatus_priority_entry_t))) {
		return EINVAL;
	}

	proc_t p = proc_find(pid);
	if (!p) {
		return ESRCH;
	}

	memset(&mp_entry, 0, sizeof(memorystatus_priority_entry_t));

	mp_entry.pid = p->p_pid;
	mp_entry.priority = p->p_memstat_effectivepriority;
	mp_entry.user_data = p->p_memstat_userdata;
	if (p->p_memstat_memlimit <= 0) {
		ret = task_get_phys_footprint_limit(p->task, &mp_entry.limit);
		if (ret != KERN_SUCCESS) {
			proc_rele(p);
			return EINVAL;
		}
	} else {
		mp_entry.limit = p->p_memstat_memlimit;
	}
	mp_entry.state = memorystatus_build_state(p);

	proc_rele(p);

	error = copyout(&mp_entry, buffer, buffer_size);

	return error;
}

static int
memorystatus_cmd_get_priority_list(pid_t pid, user_addr_t buffer, size_t buffer_size, int32_t *retval)
{
	int error = 0;
	boolean_t size_only;
	size_t list_size;

	  
                                                                   
    

	size_only = ((buffer == USER_ADDR_NULL) ? TRUE: FALSE);

	if (pid != 0) {
		list_size = sizeof(memorystatus_priority_entry_t) * 1;
		if (!size_only) {
			error = memorystatus_get_priority_pid(pid, buffer, buffer_size);
		}
	} else {
		memorystatus_priority_entry_t *list = NULL;
		error = memorystatus_get_priority_list(&list, &buffer_size, &list_size, size_only);

		if (error == 0) {
			if (!size_only) {
				error = copyout(list, buffer, list_size);
			}
		}

		if (list) {
			kheap_free(KHEAP_TEMP, list, buffer_size);
		}
	}

	if (error == 0) {
		assert(list_size <= INT32_MAX);
		*retval = (int32_t) list_size;
	}

	return error;
}

static void
memorystatus_clear_errors(void)
{
	proc_t p;
	unsigned int i = 0;

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_CLEAR_ERRORS) | DBG_FUNC_START, 0, 0, 0, 0, 0);

	proc_list_lock();

	p = memorystatus_get_first_proc_locked(&i, TRUE);
	while (p) {
		if (p->p_memstat_state & P_MEMSTAT_ERROR) {
			p->p_memstat_state &= ~P_MEMSTAT_ERROR;
		}
		p = memorystatus_get_next_proc_locked(&i, p, TRUE);
	}

	proc_list_unlock();

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_CLEAR_ERRORS) | DBG_FUNC_END, 0, 0, 0, 0, 0);
}

#if CONFIG_JETSAM
static void
memorystatus_update_levels_locked(boolean_t critical_only)
{
	memorystatus_available_pages_critical = memorystatus_available_pages_critical_base;

	  
                                                                    
    

	memstat_bucket_t *first_bucket = &memstat_bucket[JETSAM_PRIORITY_IDLE];
	if (first_bucket->count) {
		memorystatus_available_pages_critical += memorystatus_available_pages_critical_idle_offset;

		if (memorystatus_available_pages_critical > memorystatus_available_pages_pressure) {
			  
                                                                     
      
			memorystatus_available_pages_critical = memorystatus_available_pages_pressure;
		}
	}

	if (memorystatus_jetsam_policy & kPolicyMoreFree) {
		memorystatus_available_pages_critical += memorystatus_policy_more_free_offset_pages;
	}

	if (critical_only) {
		return;
	}

#if VM_PRESSURE_EVENTS
	memorystatus_available_pages_pressure = (int32_t)(pressure_threshold_percentage * (atop_64(max_mem) / 100));
#endif
}

void
memorystatus_fast_jetsam_override(boolean_t enable_override)
{
	                                                  
	if (!fast_jetsam_enabled) {
		return;
	}

	if (enable_override) {
		if ((memorystatus_jetsam_policy & kPolicyMoreFree) == kPolicyMoreFree) {
			return;
		}
		proc_list_lock();
		memorystatus_jetsam_policy |= kPolicyMoreFree;
		memorystatus_thread_pool_max();
		memorystatus_update_levels_locked(TRUE);
		proc_list_unlock();
	} else {
		if ((memorystatus_jetsam_policy & kPolicyMoreFree) == 0) {
			return;
		}
		proc_list_lock();
		memorystatus_jetsam_policy &= ~kPolicyMoreFree;
		memorystatus_thread_pool_default();
		memorystatus_update_levels_locked(TRUE);
		proc_list_unlock();
	}
}


static int
sysctl_kern_memorystatus_policy_more_free SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2, oidp)
	int error = 0, more_free = 0;

	  
                                      
   
                                                                 
              
                   
    

	error = sysctl_handle_int(oidp, &more_free, 0, req);
	if (error || !req->newptr) {
		return error;
	}

	if (more_free) {
		memorystatus_fast_jetsam_override(true);
	} else {
		memorystatus_fast_jetsam_override(false);
	}

	return 0;
}
SYSCTL_PROC(_kern, OID_AUTO, memorystatus_policy_more_free, CTLTYPE_INT | CTLFLAG_WR | CTLFLAG_LOCKED | CTLFLAG_MASKED,
    0, 0, &sysctl_kern_memorystatus_policy_more_free, "I", "");

#endif                    

  
                           
   
static int
memorystatus_get_at_boot_snapshot(memorystatus_jetsam_snapshot_t **snapshot, size_t *snapshot_size, boolean_t size_only)
{
	size_t input_size = *snapshot_size;

	  
                                           
    
	*snapshot_size = sizeof(memorystatus_jetsam_snapshot_t);

	if (size_only) {
		return 0;
	}

	  
                                            
    
	if (input_size < *snapshot_size) {
		return EINVAL;
	}

	  
                                     
    
	memorystatus_at_boot_snapshot.notification_time = mach_absolute_time();
	*snapshot = &memorystatus_at_boot_snapshot;

	MEMORYSTATUS_DEBUG(7, "memorystatus_get_at_boot_snapshot: returned inputsize (%ld), snapshot_size(%ld), listcount(%d)\n",
	    (long)input_size, (long)*snapshot_size, 0);
	return 0;
}

  
                                            
   
static int
memorystatus_get_jetsam_snapshot_copy(memorystatus_jetsam_snapshot_t **snapshot, size_t *snapshot_size, boolean_t size_only)
{
	size_t input_size = *snapshot_size;

	if (memorystatus_jetsam_snapshot_copy_count > 0) {
		*snapshot_size = sizeof(memorystatus_jetsam_snapshot_t) + (sizeof(memorystatus_jetsam_snapshot_entry_t) * (memorystatus_jetsam_snapshot_copy_count));
	} else {
		*snapshot_size = 0;
	}

	if (size_only) {
		return 0;
	}

	if (input_size < *snapshot_size) {
		return EINVAL;
	}

	*snapshot = memorystatus_jetsam_snapshot_copy;

	MEMORYSTATUS_DEBUG(7, "memorystatus_get_jetsam_snapshot_copy: returned inputsize (%ld), snapshot_size(%ld), listcount(%ld)\n",
	    (long)input_size, (long)*snapshot_size, (long)memorystatus_jetsam_snapshot_copy_count);

	return 0;
}

#if CONFIG_FREEZE
static int
memorystatus_get_jetsam_snapshot_freezer(memorystatus_jetsam_snapshot_t **snapshot, size_t *snapshot_size, boolean_t size_only)
{
	size_t input_size = *snapshot_size;

	if (memorystatus_jetsam_snapshot_freezer->entry_count > 0) {
		*snapshot_size = sizeof(memorystatus_jetsam_snapshot_t) + (sizeof(memorystatus_jetsam_snapshot_entry_t) * (memorystatus_jetsam_snapshot_freezer->entry_count));
	} else {
		*snapshot_size = 0;
	}
	assert(*snapshot_size <= memorystatus_jetsam_snapshot_freezer_size);

	if (size_only) {
		return 0;
	}

	if (input_size < *snapshot_size) {
		return EINVAL;
	}

	*snapshot = memorystatus_jetsam_snapshot_freezer;

	MEMORYSTATUS_DEBUG(7, "memorystatus_get_jetsam_snapshot_freezer: returned inputsize (%ld), snapshot_size(%ld), listcount(%ld)\n",
	    (long)input_size, (long)*snapshot_size, (long)memorystatus_jetsam_snapshot_freezer->entry_count);

	return 0;
}
#endif                    

static int
memorystatus_get_on_demand_snapshot(memorystatus_jetsam_snapshot_t **snapshot, size_t *snapshot_size, boolean_t size_only)
{
	size_t input_size = *snapshot_size;
	uint32_t ods_list_count = memorystatus_list_count;
	memorystatus_jetsam_snapshot_t *ods = NULL;                                        

	*snapshot_size = sizeof(memorystatus_jetsam_snapshot_t) + (sizeof(memorystatus_jetsam_snapshot_entry_t) * (ods_list_count));

	if (size_only) {
		return 0;
	}

	  
                                             
                                                 
                                                 
                   
    
	if (input_size < *snapshot_size) {
		return EINVAL;
	}

	  
                                              
    
	ods = kalloc(*snapshot_size);
	if (!ods) {
		return ENOMEM;
	}

	memset(ods, 0, *snapshot_size);

	proc_list_lock();
	memorystatus_init_jetsam_snapshot_locked(ods, ods_list_count);
	proc_list_unlock();

	  
                                                  
                                                     
                                                    
           
    
	*snapshot = ods;

	MEMORYSTATUS_DEBUG(7, "memorystatus_get_on_demand_snapshot: returned inputsize (%ld), snapshot_size(%ld), listcount(%ld)\n",
	    (long)input_size, (long)*snapshot_size, (long)ods_list_count);

	return 0;
}

static int
memorystatus_get_jetsam_snapshot(memorystatus_jetsam_snapshot_t **snapshot, size_t *snapshot_size, boolean_t size_only)
{
	size_t input_size = *snapshot_size;

	if (memorystatus_jetsam_snapshot_count > 0) {
		*snapshot_size = sizeof(memorystatus_jetsam_snapshot_t) + (sizeof(memorystatus_jetsam_snapshot_entry_t) * (memorystatus_jetsam_snapshot_count));
	} else {
		*snapshot_size = 0;
	}

	if (size_only) {
		return 0;
	}

	if (input_size < *snapshot_size) {
		return EINVAL;
	}

	*snapshot = memorystatus_jetsam_snapshot;

	MEMORYSTATUS_DEBUG(7, "memorystatus_get_jetsam_snapshot: returned inputsize (%ld), snapshot_size(%ld), listcount(%ld)\n",
	    (long)input_size, (long)*snapshot_size, (long)memorystatus_jetsam_snapshot_count);

	return 0;
}


static int
memorystatus_cmd_get_jetsam_snapshot(int32_t flags, user_addr_t buffer, size_t buffer_size, int32_t *retval)
{
	int error = EINVAL;
	boolean_t size_only;
	boolean_t is_default_snapshot = FALSE;
	boolean_t is_on_demand_snapshot = FALSE;
	boolean_t is_at_boot_snapshot = FALSE;
#if CONFIG_FREEZE
	bool is_freezer_snapshot = false;
#endif                    
	memorystatus_jetsam_snapshot_t *snapshot;

	size_only = ((buffer == USER_ADDR_NULL) ? TRUE : FALSE);

	if (flags == 0) {
		             
		is_default_snapshot = TRUE;
		error = memorystatus_get_jetsam_snapshot(&snapshot, &buffer_size, size_only);
	} else {
		if (flags & ~(MEMORYSTATUS_SNAPSHOT_ON_DEMAND | MEMORYSTATUS_SNAPSHOT_AT_BOOT | MEMORYSTATUS_SNAPSHOT_COPY | MEMORYSTATUS_FLAGS_SNAPSHOT_FREEZER)) {
			  
                                  
      
			return EINVAL;
		}

		if (flags & (flags - 0x1)) {
			  
                                                     
      
			return EINVAL;
		}

		if (flags & MEMORYSTATUS_SNAPSHOT_ON_DEMAND) {
			is_on_demand_snapshot = TRUE;
			  
                                                                         
                                                         
      
			error = memorystatus_get_on_demand_snapshot(&snapshot, &buffer_size, size_only);
		} else if (flags & MEMORYSTATUS_SNAPSHOT_AT_BOOT) {
			is_at_boot_snapshot = TRUE;
			error = memorystatus_get_at_boot_snapshot(&snapshot, &buffer_size, size_only);
		} else if (flags & MEMORYSTATUS_SNAPSHOT_COPY) {
			error = memorystatus_get_jetsam_snapshot_copy(&snapshot, &buffer_size, size_only);
#if CONFIG_FREEZE
		} else if (flags & MEMORYSTATUS_FLAGS_SNAPSHOT_FREEZER) {
			is_freezer_snapshot = true;
			error = memorystatus_get_jetsam_snapshot_freezer(&snapshot, &buffer_size, size_only);
#endif                    
		} else {
			  
                           
      
			return EINVAL;
		}
	}

	if (error) {
		goto out;
	}

	  
                                                                  
                                        
                                               
                                         
                                       
                                        
                                        
                                          
                                        
                                        
                                               
    
	if (!size_only) {
		if ((error = copyout(snapshot, buffer, buffer_size)) == 0) {
#if CONFIG_FREEZE
			if (is_default_snapshot || is_freezer_snapshot) {
#else
			if (is_default_snapshot) {
#endif                    
				  
                                                                     
                                                                       
                                                
       
				proc_list_lock();
#if DEVELOPMENT || DEBUG
				if (memorystatus_testing_pid != 0 && memorystatus_testing_pid != current_proc()->p_pid) {
					                                                                    
					proc_list_unlock();
					goto out;
				}
#endif                            
				if (is_default_snapshot) {
					memcpy(memorystatus_jetsam_snapshot_copy, memorystatus_jetsam_snapshot, memorystatus_jetsam_snapshot_size);
					memorystatus_jetsam_snapshot_copy_count = memorystatus_jetsam_snapshot_count;
					snapshot->entry_count = memorystatus_jetsam_snapshot_count = 0;
					memorystatus_jetsam_snapshot_last_timestamp = 0;
				}
#if CONFIG_FREEZE
				else if (is_freezer_snapshot) {
					memorystatus_jetsam_snapshot_freezer->entry_count = 0;
				}
#endif                    
				proc_list_unlock();
			}
		}

		if (is_on_demand_snapshot) {
			  
                                             
                                 
      
			if (snapshot) {
				kfree(snapshot, buffer_size);
			}
		}
	}

out:
	if (error == 0) {
		assert(buffer_size <= INT32_MAX);
		*retval = (int32_t) buffer_size;
	}
	return error;
}

#if DEVELOPMENT || DEBUG
static int
memorystatus_cmd_set_testing_pid(int32_t flags)
{
	int error = EINVAL;
	proc_t caller = current_proc();
	assert(caller != kernproc);
	proc_list_lock();
	if (flags & MEMORYSTATUS_FLAGS_SET_TESTING_PID) {
		if (memorystatus_testing_pid == 0) {
			memorystatus_testing_pid = caller->p_pid;
			error = 0;
		} else if (memorystatus_testing_pid == caller->p_pid) {
			error = 0;
		} else {
			                                                             
			error = EBUSY;
		}
	} else if (flags & MEMORYSTATUS_FLAGS_UNSET_TESTING_PID) {
		if (memorystatus_testing_pid == caller->p_pid) {
			memorystatus_testing_pid = 0;
			error = 0;
		} else if (memorystatus_testing_pid != 0) {
			                                                             
			error = EPERM;
		}
	}
	proc_list_unlock();

	return error;
}
#endif                           

  
                                                    
                                                       
  
             
                                                   
                                      
                                                             
                                                               
  
                                        
                               
                               
                               
                               
                                      
  
                           
                                                           
  
                                         
  
                                                                    
                                                                     
                                                                   
                                         
                                                                  
                                                                        
                                                                
                                                                  
                                                                
                                                                       
   


static int
memorystatus_cmd_grp_set_priorities(user_addr_t buffer, size_t buffer_size)
{
	  
                                   
               
    

	int error = 0;
	memorystatus_properties_entry_v1_t *entries = NULL;
	size_t entry_count = 0;

	                                        
	typedef struct memorystatus_internal_properties {
		proc_t proc;
		int32_t priority;
	} memorystatus_internal_properties_t;

	memorystatus_internal_properties_t *table = NULL;
	size_t table_size = 0;
	uint32_t table_count = 0;

	size_t i = 0;
	uint32_t bucket_index = 0;
	boolean_t head_insert;
	int32_t new_priority;

	proc_t p;

	                   
	if ((buffer == USER_ADDR_NULL) || (buffer_size == 0)) {
		error = EINVAL;
		goto out;
	}

	entry_count = (buffer_size / sizeof(memorystatus_properties_entry_v1_t));
	if (entry_count == 0) {
		                                                         
		error = EINVAL;
		goto out;
	}

	if ((entries = kheap_alloc(KHEAP_TEMP, buffer_size, Z_WAITOK)) == NULL) {
		error = ENOMEM;
		goto out;
	}

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_GRP_SET_PROP) | DBG_FUNC_START, MEMORYSTATUS_FLAGS_GRP_SET_PRIORITY, entry_count, 0, 0, 0);

	if ((error = copyin(buffer, entries, buffer_size)) != 0) {
		goto out;
	}

	                                       
	if (entries[0].version == MEMORYSTATUS_MPE_VERSION_1) {
		if ((buffer_size % MEMORYSTATUS_MPE_VERSION_1_SIZE) != 0) {
			error = EINVAL;
			goto out;
		}
	} else {
		error = EINVAL;
		goto out;
	}

	for (i = 0; i < entry_count; i++) {
		if (entries[i].priority == -1) {
			                                           
			entries[i].priority = JETSAM_PRIORITY_DEFAULT;
		} else if ((entries[i].priority == system_procs_aging_band) || (entries[i].priority == applications_aging_band)) {
			                                                      
                                                      
			entries[i].priority = JETSAM_PRIORITY_IDLE;
		} else if (entries[i].priority == JETSAM_PRIORITY_IDLE_HEAD) {
			                                                            
              
			                          
		} else if ((entries[i].priority < 0) || (entries[i].priority >= MEMSTAT_BUCKET_COUNT)) {
			                  
			error = EINVAL;
			goto out;
		}
	}

	table_size = sizeof(memorystatus_internal_properties_t) * entry_count;
	if ((table = kheap_alloc(KHEAP_TEMP, table_size, Z_WAITOK | Z_ZERO)) == NULL) {
		error = ENOMEM;
		goto out;
	}


	  
                                                                       
                                                                     
                                                     
                                                            
                        
    

	bucket_index = 0;

	proc_list_lock();

	                              
	p = memorystatus_get_first_proc_locked(&bucket_index, TRUE);
	while (p && (table_count < entry_count)) {
		for (i = 0; i < entry_count; i++) {
			if (p->p_pid == entries[i].pid) {
				                           
				table[table_count].proc = p;
				table[table_count].priority = entries[i].priority;
				table_count++;
				break;
			}
		}
		p = memorystatus_get_next_proc_locked(&bucket_index, p, TRUE);
	}

	                                                     
	for (i = 0; i < table_count; i++) {
		p = table[i].proc;
		assert(p != NULL);

		                                                      
		if (table[i].priority == JETSAM_PRIORITY_IDLE_HEAD) {
			new_priority = JETSAM_PRIORITY_IDLE;
			head_insert = true;
		} else {
			new_priority = table[i].priority;
			head_insert = false;
		}

		                 
		if (p->p_memstat_state & P_MEMSTAT_INTERNAL) {
			continue;
		}

		  
                                                 
                               
     
		if ((p->p_memstat_effectivepriority == system_procs_aging_band) || (p->p_memstat_effectivepriority == applications_aging_band)) {
			memorystatus_invalidate_idle_demotion_locked(p, TRUE);
		}

		memorystatus_update_priority_locked(p, new_priority, head_insert, false);
	}

	proc_list_unlock();

	  
                                   
                                                   
                               
    
out:
	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_GRP_SET_PROP) | DBG_FUNC_END, MEMORYSTATUS_FLAGS_GRP_SET_PRIORITY, entry_count, table_count, 0, 0);

	if (entries) {
		kheap_free(KHEAP_TEMP, entries, buffer_size);
	}
	if (table) {
		kheap_free(KHEAP_TEMP, table, table_size);
	}

	return error;
}

memorystatus_internal_probabilities_t *memorystatus_global_probabilities_table = NULL;
size_t memorystatus_global_probabilities_size = 0;

static int
memorystatus_cmd_grp_set_probabilities(user_addr_t buffer, size_t buffer_size)
{
	int error = 0;
	memorystatus_properties_entry_v1_t *entries = NULL;
	size_t entry_count = 0, i = 0;
	memorystatus_internal_probabilities_t *tmp_table_new = NULL, *tmp_table_old = NULL;
	size_t tmp_table_new_size = 0, tmp_table_old_size = 0;
#if DEVELOPMENT || DEBUG
	if (memorystatus_testing_pid != 0 && memorystatus_testing_pid != current_proc()->p_pid) {
		                                                                          
		error = EPERM;
		goto out;
	}
#endif                            

	                   
	if ((buffer == USER_ADDR_NULL) || (buffer_size == 0)) {
		error = EINVAL;
		goto out;
	}

	entry_count = (buffer_size / sizeof(memorystatus_properties_entry_v1_t));

	if ((entries = kheap_alloc(KHEAP_TEMP, buffer_size, Z_WAITOK)) == NULL) {
		error = ENOMEM;
		goto out;
	}

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_GRP_SET_PROP) | DBG_FUNC_START, MEMORYSTATUS_FLAGS_GRP_SET_PROBABILITY, entry_count, 0, 0, 0);

	if ((error = copyin(buffer, entries, buffer_size)) != 0) {
		goto out;
	}

	if (entries[0].version == MEMORYSTATUS_MPE_VERSION_1) {
		if ((buffer_size % MEMORYSTATUS_MPE_VERSION_1_SIZE) != 0) {
			error = EINVAL;
			goto out;
		}
	} else {
		error = EINVAL;
		goto out;
	}

	                                       
	for (i = 0; i < entry_count; i++) {
		  
                                
                                 
    
                                                      
                                                      
              
     
		if (entries[i].use_probability > 1) {
			error = EINVAL;
			goto out;
		}
	}

	tmp_table_new_size = sizeof(memorystatus_internal_probabilities_t) * entry_count;

	if ((tmp_table_new = kalloc_flags(tmp_table_new_size, Z_WAITOK | Z_ZERO)) == NULL) {
		error = ENOMEM;
		goto out;
	}

	proc_list_lock();

	if (memorystatus_global_probabilities_table) {
		tmp_table_old = memorystatus_global_probabilities_table;
		tmp_table_old_size = memorystatus_global_probabilities_size;
	}

	memorystatus_global_probabilities_table = tmp_table_new;
	memorystatus_global_probabilities_size = tmp_table_new_size;
	tmp_table_new = NULL;

	for (i = 0; i < entry_count; i++) {
		                           
		strlcpy(memorystatus_global_probabilities_table[i].proc_name, entries[i].proc_name, MAXCOMLEN + 1);
		memorystatus_global_probabilities_table[i].use_probability = entries[i].use_probability;
	}

	proc_list_unlock();

out:
	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_GRP_SET_PROP) | DBG_FUNC_END, MEMORYSTATUS_FLAGS_GRP_SET_PROBABILITY, entry_count, tmp_table_new_size, 0, 0);

	if (entries) {
		kheap_free(KHEAP_TEMP, entries, buffer_size);
		entries = NULL;
	}

	if (tmp_table_old) {
		kfree(tmp_table_old, tmp_table_old_size);
		tmp_table_old = NULL;
	}

	return error;
}

static int
memorystatus_cmd_grp_set_properties(int32_t flags, user_addr_t buffer, size_t buffer_size, __unused int32_t *retval)
{
	int error = 0;

	if ((flags & MEMORYSTATUS_FLAGS_GRP_SET_PRIORITY) == MEMORYSTATUS_FLAGS_GRP_SET_PRIORITY) {
		error = memorystatus_cmd_grp_set_priorities(buffer, buffer_size);
	} else if ((flags & MEMORYSTATUS_FLAGS_GRP_SET_PROBABILITY) == MEMORYSTATUS_FLAGS_GRP_SET_PROBABILITY) {
		error = memorystatus_cmd_grp_set_probabilities(buffer, buffer_size);
	} else {
		error = EINVAL;
	}

	return error;
}

  
                                                                                            
                                                                                       
                                             
  
                                                                                              
                                                                                             
                                                                                                
                                                   
  
                                                                                                    
                                                                                                    
                                                                                                    
                                                                                             
                                                    
   

static int
memorystatus_cmd_set_priority_properties(pid_t pid, uint32_t flags, user_addr_t buffer, size_t buffer_size, __unused int32_t *retval)
{
	int error = 0;
	boolean_t is_assertion = FALSE;                                                 
	memorystatus_priority_properties_t mpp_entry;

	                     
	if ((pid == 0) || (buffer == USER_ADDR_NULL) || (buffer_size != sizeof(memorystatus_priority_properties_t))) {
		return EINVAL;
	}

	                    
	if (flags == 0) {
		  
                                                  
     
	} else {
		if (flags & ~(MEMORYSTATUS_SET_PRIORITY_ASSERTION)) {
			  
                                  
      
			return EINVAL;
		} else if (flags & MEMORYSTATUS_SET_PRIORITY_ASSERTION) {
			is_assertion = TRUE;
		}
	}

	error = copyin(buffer, &mpp_entry, buffer_size);

	if (error == 0) {
		proc_t p;

		p = proc_find(pid);
		if (!p) {
			return ESRCH;
		}

		if (p->p_memstat_state & P_MEMSTAT_INTERNAL) {
			proc_rele(p);
			return EPERM;
		}

		if (is_assertion) {
			os_log(OS_LOG_DEFAULT, "memorystatus: set assertion priority(%d) target %s:%d\n",
			    mpp_entry.priority, (*p->p_name ? p->p_name : "unknown"), p->p_pid);
		}

		error = memorystatus_update(p, mpp_entry.priority, mpp_entry.user_data, is_assertion, FALSE, FALSE, 0, 0, FALSE, FALSE);
		proc_rele(p);
	}

	return error;
}

static int
memorystatus_cmd_set_memlimit_properties(pid_t pid, user_addr_t buffer, size_t buffer_size, __unused int32_t *retval)
{
	int error = 0;
	memorystatus_memlimit_properties_t mmp_entry;

	                     
	if ((pid == 0) || (buffer == USER_ADDR_NULL) || (buffer_size != sizeof(memorystatus_memlimit_properties_t))) {
		return EINVAL;
	}

	error = copyin(buffer, &mmp_entry, buffer_size);

	if (error == 0) {
		error = memorystatus_set_memlimit_properties(pid, &mmp_entry);
	}

	return error;
}

static void
memorystatus_get_memlimit_properties_internal(proc_t p, memorystatus_memlimit_properties_t* p_entry)
{
	memset(p_entry, 0, sizeof(memorystatus_memlimit_properties_t));

	if (p->p_memstat_memlimit_active > 0) {
		p_entry->memlimit_active = p->p_memstat_memlimit_active;
	} else {
		task_convert_phys_footprint_limit(-1, &p_entry->memlimit_active);
	}

	if (p->p_memstat_state & P_MEMSTAT_MEMLIMIT_ACTIVE_FATAL) {
		p_entry->memlimit_active_attr |= MEMORYSTATUS_MEMLIMIT_ATTR_FATAL;
	}

	  
                                         
    
	if (p->p_memstat_memlimit_inactive <= 0) {
		task_convert_phys_footprint_limit(-1, &p_entry->memlimit_inactive);
	} else {
		p_entry->memlimit_inactive = p->p_memstat_memlimit_inactive;
	}
	if (p->p_memstat_state & P_MEMSTAT_MEMLIMIT_INACTIVE_FATAL) {
		p_entry->memlimit_inactive_attr |= MEMORYSTATUS_MEMLIMIT_ATTR_FATAL;
	}
}

  
                                                                                            
                                                                                          
                                                                                      
                                                                                              
                                                             
   
static int
memorystatus_cmd_get_memlimit_properties(pid_t pid, user_addr_t buffer, size_t buffer_size, __unused int32_t *retval)
{
	memorystatus_memlimit_properties2_t mmp_entry;

	                     
	if ((pid == 0) || (buffer == USER_ADDR_NULL) ||
	    ((buffer_size != sizeof(memorystatus_memlimit_properties_t)) &&
	    (buffer_size != sizeof(memorystatus_memlimit_properties2_t)))) {
		return EINVAL;
	}

	memset(&mmp_entry, 0, sizeof(memorystatus_memlimit_properties2_t));

	proc_t p = proc_find(pid);
	if (!p) {
		return ESRCH;
	}

	  
                                        
                                                         
    

	memorystatus_get_memlimit_properties_internal(p, &mmp_entry.v1);

#if CONFIG_JETSAM
#if DEVELOPMENT || DEBUG
	  
                                   
    
	mmp_entry.memlimit_increase = roundToNearestMB(p->p_memlimit_increase);
	mmp_entry.memlimit_increase_bytes = p->p_memlimit_increase;
#endif                           
#endif                    

	proc_rele(p);

	int error = copyout(&mmp_entry, buffer, buffer_size);

	return error;
}


  
                           
                                                            
                                                            
                                                       
                                                    
   
static int
memorystatus_cmd_get_memlimit_excess_np(pid_t pid, uint32_t flags, user_addr_t buffer, size_t buffer_size, __unused int32_t *retval)
{
	int error = 0;
	uint64_t footprint_in_bytes = 0;
	uint64_t delta_in_bytes = 0;
	int32_t  memlimit_mb = 0;
	uint64_t memlimit_bytes = 0;

	                     
	if ((pid == 0) || (buffer == USER_ADDR_NULL) || (buffer_size != sizeof(uint64_t)) || (flags != 0)) {
		return EINVAL;
	}

	proc_t p = proc_find(pid);
	if (!p) {
		return ESRCH;
	}

	  
                           
                                                         
    

	if (p->p_memstat_memlimit_inactive <= 0) {
		task_convert_phys_footprint_limit(-1, &memlimit_mb);
	} else {
		memlimit_mb = p->p_memstat_memlimit_inactive;
	}

	footprint_in_bytes = get_task_phys_footprint(p->task);

	proc_rele(p);

	memlimit_bytes = memlimit_mb * 1024 * 1024;                      

	  
                                            
    
	if (footprint_in_bytes > memlimit_bytes) {
		delta_in_bytes = footprint_in_bytes - memlimit_bytes;
	}

	error = copyout(&delta_in_bytes, buffer, sizeof(delta_in_bytes));

	return error;
}


static int
memorystatus_cmd_get_pressure_status(int32_t *retval)
{
	int error;

	                              
	error = priv_check_cred(kauth_cred_get(), PRIV_VM_PRESSURE, 0);
	if (error) {
		return error;
	}

	                                                           
	*retval = (kVMPressureNormal != memorystatus_vm_pressure_level) ? 1 : 0;

	return error;
}

int
memorystatus_get_pressure_status_kdp()
{
	return (kVMPressureNormal != memorystatus_vm_pressure_level) ? 1 : 0;
}

  
                                                                                                         
  
                                                                                              
                                                                         
                                                                         
                                                                                                
  
                                                               
                                                                                                
                                                              
  
                                                          
                                                                                                     
                                                      
   

#if CONFIG_JETSAM
static int
memorystatus_cmd_set_jetsam_memory_limit(pid_t pid, int32_t high_water_mark, __unused int32_t *retval, boolean_t is_fatal_limit)
{
	int error = 0;
	memorystatus_memlimit_properties_t entry;

	entry.memlimit_active = high_water_mark;
	entry.memlimit_active_attr = 0;
	entry.memlimit_inactive = high_water_mark;
	entry.memlimit_inactive_attr = 0;

	if (is_fatal_limit == TRUE) {
		entry.memlimit_active_attr   |= MEMORYSTATUS_MEMLIMIT_ATTR_FATAL;
		entry.memlimit_inactive_attr |= MEMORYSTATUS_MEMLIMIT_ATTR_FATAL;
	}

	error = memorystatus_set_memlimit_properties(pid, &entry);
	return error;
}
#endif                    

static int
memorystatus_set_memlimit_properties_internal(proc_t p, memorystatus_memlimit_properties_t *p_entry)
{
	int error = 0;

	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_OWNED);

	  
                                                
    
	SET_ACTIVE_LIMITS_LOCKED(p, p_entry->memlimit_active, p_entry->memlimit_active_attr);

	  
                                                  
    
	SET_INACTIVE_LIMITS_LOCKED(p, p_entry->memlimit_inactive, p_entry->memlimit_inactive_attr);

	  
                                                                   
                           
                                                           
    

	if (memorystatus_highwater_enabled) {
		boolean_t is_fatal;
		boolean_t use_active;

		if (proc_jetsam_state_is_active_locked(p) == TRUE) {
			CACHE_ACTIVE_LIMITS_LOCKED(p, is_fatal);
			use_active = TRUE;
		} else {
			CACHE_INACTIVE_LIMITS_LOCKED(p, is_fatal);
			use_active = FALSE;
		}

		                                                 
		error = (task_set_phys_footprint_limit_internal(p->task, ((p->p_memstat_memlimit > 0) ? p->p_memstat_memlimit : -1), NULL, use_active, is_fatal) == 0) ? 0 : EINVAL;

		MEMORYSTATUS_DEBUG(3, "memorystatus_set_memlimit_properties: new limit on pid %d (%dMB %s) current priority (%d) dirty_state?=0x%x %s\n",
		    p->p_pid, (p->p_memstat_memlimit > 0 ? p->p_memstat_memlimit : -1),
		    (p->p_memstat_state & P_MEMSTAT_FATAL_MEMLIMIT ? "F " : "NF"), p->p_memstat_effectivepriority, p->p_memstat_dirty,
		    (p->p_memstat_dirty ? ((p->p_memstat_dirty & P_DIRTY) ? "isdirty" : "isclean") : ""));
		DTRACE_MEMORYSTATUS2(memorystatus_set_memlimit, proc_t, p, int32_t, (p->p_memstat_memlimit > 0 ? p->p_memstat_memlimit : -1));
	}

	return error;
}

static int
memorystatus_set_memlimit_properties(pid_t pid, memorystatus_memlimit_properties_t *entry)
{
	memorystatus_memlimit_properties_t set_entry;

	proc_t p = proc_find(pid);
	if (!p) {
		return ESRCH;
	}

	  
                                    
    
	const uint32_t valid_attrs = MEMORYSTATUS_MEMLIMIT_ATTR_FATAL;
	if ((entry->memlimit_active_attr & (~valid_attrs)) != 0) {
		proc_rele(p);
		return EINVAL;
	}
	if ((entry->memlimit_inactive_attr & (~valid_attrs)) != 0) {
		proc_rele(p);
		return EINVAL;
	}

	  
                                        
    
	set_entry.memlimit_active = entry->memlimit_active;
	set_entry.memlimit_active_attr = entry->memlimit_active_attr & MEMORYSTATUS_MEMLIMIT_ATTR_FATAL;

	  
                                          
    
	set_entry.memlimit_inactive = entry->memlimit_inactive;
	set_entry.memlimit_inactive_attr = entry->memlimit_inactive_attr & MEMORYSTATUS_MEMLIMIT_ATTR_FATAL;

	  
                                                           
                                                          
                                                             
                    
    

	if (set_entry.memlimit_active <= 0) {
		  
                                                                      
     
		set_entry.memlimit_active = -1;
		set_entry.memlimit_active_attr = MEMORYSTATUS_MEMLIMIT_ATTR_FATAL;
	}
#if CONFIG_JETSAM
#if DEVELOPMENT || DEBUG
	else {
		                                               
		set_entry.memlimit_active += roundToNearestMB(p->p_memlimit_increase);
	}
#endif                           
#endif                    

	if (set_entry.memlimit_inactive <= 0) {
		  
                                                                        
     
		set_entry.memlimit_inactive = -1;
		set_entry.memlimit_inactive_attr = MEMORYSTATUS_MEMLIMIT_ATTR_FATAL;
	}
#if CONFIG_JETSAM
#if DEVELOPMENT || DEBUG
	else {
		                                               
		set_entry.memlimit_inactive += roundToNearestMB(p->p_memlimit_increase);
	}
#endif                           
#endif                    

	proc_list_lock();

	int error = memorystatus_set_memlimit_properties_internal(p, &set_entry);

	proc_list_unlock();
	proc_rele(p);

	return error;
}

  
                                                                      
                             
   
int
proc_get_memstat_priority(proc_t p, boolean_t effective_priority)
{
	if (p) {
		if (effective_priority) {
			return p->p_memstat_effectivepriority;
		} else {
			return p->p_memstat_requestedpriority;
		}
	}
	return 0;
}

static int
memorystatus_get_process_is_managed(pid_t pid, int *is_managed)
{
	proc_t p = NULL;

	                     
	if (pid == 0) {
		return EINVAL;
	}

	p = proc_find(pid);
	if (!p) {
		return ESRCH;
	}

	proc_list_lock();
	*is_managed = ((p->p_memstat_state & P_MEMSTAT_MANAGED) ? 1 : 0);
	proc_rele_locked(p);
	proc_list_unlock();

	return 0;
}

static int
memorystatus_set_process_is_managed(pid_t pid, boolean_t set_managed)
{
	proc_t p = NULL;

	                     
	if (pid == 0) {
		return EINVAL;
	}

	p = proc_find(pid);
	if (!p) {
		return ESRCH;
	}

	proc_list_lock();
	if (set_managed == TRUE) {
		p->p_memstat_state |= P_MEMSTAT_MANAGED;
		  
                                                             
                                                              
                                                     
     
		p->p_memstat_state &= ~P_MEMSTAT_FREEZE_DISABLED;
	} else {
		p->p_memstat_state &= ~P_MEMSTAT_MANAGED;
	}
	proc_rele_locked(p);
	proc_list_unlock();

	return 0;
}

int
memorystatus_control(struct proc *p __unused, struct memorystatus_control_args *args, int *ret)
{
	int error = EINVAL;
	boolean_t skip_auth_check = FALSE;
	os_reason_t jetsam_reason = OS_REASON_NULL;

#if !CONFIG_JETSAM
    #pragma unused(ret)
    #pragma unused(jetsam_reason)
#endif

	                                                                                                                  
	if (args->command == MEMORYSTATUS_CMD_SET_PROCESS_IS_FREEZABLE ||
	    args->command == MEMORYSTATUS_CMD_GET_PROCESS_IS_FREEZABLE ||
	    args->command == MEMORYSTATUS_CMD_GET_PROCESS_IS_FROZEN) {
		skip_auth_check = TRUE;
	}

	                                          
	if (!kauth_cred_issuser(kauth_cred_get()) && !IOTaskHasEntitlement(current_task(), MEMORYSTATUS_ENTITLEMENT) && !skip_auth_check) {
		error = EPERM;
		goto out;
	}

	  
                 
                                    
    
	if (args->command != MEMORYSTATUS_CMD_GET_JETSAM_SNAPSHOT) {
		if (args->buffersize > MEMORYSTATUS_BUFFERSIZE_MAX) {
			error = EINVAL;
			goto out;
		}
	}

	switch (args->command) {
	case MEMORYSTATUS_CMD_GET_PRIORITY_LIST:
		error = memorystatus_cmd_get_priority_list(args->pid, args->buffer, args->buffersize, ret);
		break;
	case MEMORYSTATUS_CMD_SET_PRIORITY_PROPERTIES:
		error = memorystatus_cmd_set_priority_properties(args->pid, args->flags, args->buffer, args->buffersize, ret);
		break;
	case MEMORYSTATUS_CMD_SET_MEMLIMIT_PROPERTIES:
		error = memorystatus_cmd_set_memlimit_properties(args->pid, args->buffer, args->buffersize, ret);
		break;
	case MEMORYSTATUS_CMD_GET_MEMLIMIT_PROPERTIES:
		error = memorystatus_cmd_get_memlimit_properties(args->pid, args->buffer, args->buffersize, ret);
		break;
	case MEMORYSTATUS_CMD_GET_MEMLIMIT_EXCESS:
		error = memorystatus_cmd_get_memlimit_excess_np(args->pid, args->flags, args->buffer, args->buffersize, ret);
		break;
	case MEMORYSTATUS_CMD_GRP_SET_PROPERTIES:
		error = memorystatus_cmd_grp_set_properties((int32_t)args->flags, args->buffer, args->buffersize, ret);
		break;
	case MEMORYSTATUS_CMD_GET_JETSAM_SNAPSHOT:
		error = memorystatus_cmd_get_jetsam_snapshot((int32_t)args->flags, args->buffer, args->buffersize, ret);
		break;
#if DEVELOPMENT || DEBUG
	case MEMORYSTATUS_CMD_SET_TESTING_PID:
		error = memorystatus_cmd_set_testing_pid((int32_t) args->flags);
		break;
#endif
	case MEMORYSTATUS_CMD_GET_PRESSURE_STATUS:
		error = memorystatus_cmd_get_pressure_status(ret);
		break;
#if CONFIG_JETSAM
	case MEMORYSTATUS_CMD_SET_JETSAM_HIGH_WATER_MARK:
		  
                                                                       
                                                          
                                              
     
		error = memorystatus_cmd_set_jetsam_memory_limit(args->pid, (int32_t)args->flags, ret, FALSE);
		break;
	case MEMORYSTATUS_CMD_SET_JETSAM_TASK_LIMIT:
		  
                                                                       
                                                          
                                          
     
		error = memorystatus_cmd_set_jetsam_memory_limit(args->pid, (int32_t)args->flags, ret, TRUE);
		break;
#endif                    
		                   
#if DEVELOPMENT || DEBUG
	case MEMORYSTATUS_CMD_TEST_JETSAM:
		jetsam_reason = os_reason_create(OS_REASON_JETSAM, JETSAM_REASON_GENERIC);
		if (jetsam_reason == OS_REASON_NULL) {
			printf("memorystatus_control: failed to allocate jetsam reason\n");
		}

		error = memorystatus_kill_process_sync(args->pid, kMemorystatusKilled, jetsam_reason) ? 0 : EINVAL;
		break;
	case MEMORYSTATUS_CMD_TEST_JETSAM_SORT:
		error = memorystatus_cmd_test_jetsam_sort(args->pid, (int32_t)args->flags, args->buffer, args->buffersize);
		break;
#if CONFIG_JETSAM
	case MEMORYSTATUS_CMD_SET_JETSAM_PANIC_BITS:
		error = memorystatus_cmd_set_panic_bits(args->buffer, args->buffersize);
		break;
#endif                    
#else                           
	#pragma unused(jetsam_reason)
#endif                           
	case MEMORYSTATUS_CMD_AGGRESSIVE_JETSAM_LENIENT_MODE_ENABLE:
		if (memorystatus_aggressive_jetsam_lenient_allowed == FALSE) {
#if DEVELOPMENT || DEBUG
			printf("Enabling Lenient Mode\n");
#endif                           

			memorystatus_aggressive_jetsam_lenient_allowed = TRUE;
			memorystatus_aggressive_jetsam_lenient = TRUE;
			error = 0;
		}
		break;
	case MEMORYSTATUS_CMD_AGGRESSIVE_JETSAM_LENIENT_MODE_DISABLE:
#if DEVELOPMENT || DEBUG
		printf("Disabling Lenient mode\n");
#endif                           
		memorystatus_aggressive_jetsam_lenient_allowed = FALSE;
		memorystatus_aggressive_jetsam_lenient = FALSE;
		error = 0;
		break;
	case MEMORYSTATUS_CMD_GET_AGGRESSIVE_JETSAM_LENIENT_MODE:
		*ret = (memorystatus_aggressive_jetsam_lenient ? 1 : 0);
		error = 0;
		break;
	case MEMORYSTATUS_CMD_PRIVILEGED_LISTENER_ENABLE:
	case MEMORYSTATUS_CMD_PRIVILEGED_LISTENER_DISABLE:
		error = memorystatus_low_mem_privileged_listener(args->command);
		break;

	case MEMORYSTATUS_CMD_ELEVATED_INACTIVEJETSAMPRIORITY_ENABLE:
	case MEMORYSTATUS_CMD_ELEVATED_INACTIVEJETSAMPRIORITY_DISABLE:
		error = memorystatus_update_inactive_jetsam_priority_band(args->pid, args->command, JETSAM_PRIORITY_ELEVATED_INACTIVE, args->flags ? TRUE : FALSE);
		break;
	case MEMORYSTATUS_CMD_SET_PROCESS_IS_MANAGED:
		error = memorystatus_set_process_is_managed(args->pid, args->flags);
		break;

	case MEMORYSTATUS_CMD_GET_PROCESS_IS_MANAGED:
		error = memorystatus_get_process_is_managed(args->pid, ret);
		break;

#if CONFIG_FREEZE
	case MEMORYSTATUS_CMD_SET_PROCESS_IS_FREEZABLE:
		error = memorystatus_set_process_is_freezable(args->pid, args->flags ? TRUE : FALSE);
		break;

	case MEMORYSTATUS_CMD_GET_PROCESS_IS_FREEZABLE:
		error = memorystatus_get_process_is_freezable(args->pid, ret);
		break;
	case MEMORYSTATUS_CMD_GET_PROCESS_IS_FROZEN:
		error = memorystatus_get_process_is_frozen(args->pid, ret);
		break;

	case MEMORYSTATUS_CMD_FREEZER_CONTROL:
		error = memorystatus_freezer_control(args->flags, args->buffer, args->buffersize, ret);
		break;
#endif                    

#if CONFIG_JETSAM
#if DEVELOPMENT || DEBUG
	case MEMORYSTATUS_CMD_INCREASE_JETSAM_TASK_LIMIT:
		error = memorystatus_cmd_increase_jetsam_task_limit(args->pid, args->flags);
		break;
#endif                           
#endif                    

	default:
		break;
	}

out:
	return error;
}

                       

                                                   
typedef struct memstat_sort_info {
	coalition_t     msi_coal;
	uint64_t        msi_page_count;
	pid_t           msi_pid;
	int             msi_ntasks;
} memstat_sort_info_t;

  
                                                       
  
                       
                        
                       
   
static int
memstat_asc_cmp(const void *a, const void *b)
{
	const memstat_sort_info_t *msA = (const memstat_sort_info_t *)a;
	const memstat_sort_info_t *msB = (const memstat_sort_info_t *)b;

	return (int)((uint64_t)msA->msi_page_count - (uint64_t)msB->msi_page_count);
}

  
                                                         
   
static int
memorystatus_sort_by_largest_coalition_locked(unsigned int bucket_index, int coal_sort_order)
{
#define MAX_SORT_PIDS           80
#define MAX_COAL_LEADERS        10

	unsigned int b = bucket_index;
	int nleaders = 0;
	int ntasks = 0;
	proc_t p = NULL;
	coalition_t coal = COALITION_NULL;
	int pids_moved = 0;
	int total_pids_moved = 0;
	int i;

	  
                                                              
                                                            
    
	memstat_sort_info_t leaders[MAX_COAL_LEADERS];
	pid_t pid_list[MAX_SORT_PIDS];

	if (bucket_index >= MEMSTAT_BUCKET_COUNT) {
		return 0;
	}

	  
                                                           
    
	for (i = 0; i < MAX_COAL_LEADERS; i++) {
		leaders[i].msi_coal = COALITION_NULL;
		leaders[i].msi_page_count = 0;                                                    
		leaders[i].msi_pid = 0;                                                     
		leaders[i].msi_ntasks = 0;                                                                
	}

	p = memorystatus_get_first_proc_locked(&b, FALSE);
	while (p) {
		coal = task_get_coalition(p->task, COALITION_TYPE_JETSAM);
		if (coalition_is_leader(p->task, coal)) {
			if (nleaders < MAX_COAL_LEADERS) {
				int coal_ntasks = 0;
				uint64_t coal_page_count = coalition_get_page_count(coal, &coal_ntasks);
				leaders[nleaders].msi_coal = coal;
				leaders[nleaders].msi_page_count = coal_page_count;
				leaders[nleaders].msi_pid = p->p_pid;                                     
				leaders[nleaders].msi_ntasks = coal_ntasks;
				nleaders++;
			} else {
				  
                                                                           
                                                                        
                                   
                                       
       
				printf("%s: WARNING: more than %d leaders in priority band [%d]\n",
				    __FUNCTION__, MAX_COAL_LEADERS, bucket_index);
				break;
			}
		}
		p = memorystatus_get_next_proc_locked(&b, p, FALSE);
	}

	if (nleaders == 0) {
		                     
		return 0;
	}

	  
                                                                       
                                                                           
                                                                               
    
	if (nleaders > 1) {
		qsort(leaders, nleaders, sizeof(memstat_sort_info_t), memstat_asc_cmp);
	}

#if 0
	for (i = 0; i < nleaders; i++) {
		printf("%s: coal_leader[%d of %d] pid[%d] pages[%llu] ntasks[%d]\n",
		    __FUNCTION__, i, nleaders, leaders[i].msi_pid, leaders[i].msi_page_count,
		    leaders[i].msi_ntasks);
	}
#endif

	  
                                                                         
                                                                       
                                                                     
                                                                          
   
                                                                     
                                                                    
                                            
   
                                                                 
                                                                         
    

	  
                                                                 
                                  
    
	total_pids_moved = 0;
	for (i = 0; i < nleaders; i++) {
		                          
		pids_moved = 0;

		                                                                    
		pid_list[0] = leaders[i].msi_pid;
		pids_moved += memorystatus_move_list_locked(bucket_index, pid_list, 1);

		                                                 
		ntasks = coalition_get_pid_list(leaders[i].msi_coal, COALITION_ROLEMASK_XPC,
		    coal_sort_order, pid_list, MAX_SORT_PIDS);

		if (ntasks > 0) {
			pids_moved += memorystatus_move_list_locked(bucket_index, pid_list,
			    (ntasks <= MAX_SORT_PIDS ? ntasks : MAX_SORT_PIDS));
		}

		                                                       
		ntasks = coalition_get_pid_list(leaders[i].msi_coal, COALITION_ROLEMASK_EXT,
		    coal_sort_order, pid_list, MAX_SORT_PIDS);

		if (ntasks > 0) {
			pids_moved += memorystatus_move_list_locked(bucket_index, pid_list,
			    (ntasks <= MAX_SORT_PIDS ? ntasks : MAX_SORT_PIDS));
		}

		                                                               
		ntasks = coalition_get_pid_list(leaders[i].msi_coal, COALITION_ROLEMASK_UNDEF,
		    coal_sort_order, pid_list, MAX_SORT_PIDS);

		if (ntasks > 0) {
			pids_moved += memorystatus_move_list_locked(bucket_index, pid_list,
			    (ntasks <= MAX_SORT_PIDS ? ntasks : MAX_SORT_PIDS));
		}

#if 0
		if (pids_moved == leaders[i].msi_ntasks) {
			  
                                                            
      
			printf("%s: pids_moved[%d]  equal  total coalition ntasks[%d] \n", __FUNCTION__,
			    pids_moved, leaders[i].msi_ntasks);
		} else if (pids_moved > leaders[i].msi_ntasks) {
			  
                                                                 
      
			printf("%s: pids_moved[%d] were greater than expected coalition ntasks[%d] \n", __FUNCTION__,
			    pids_moved, leaders[i].msi_ntasks);
		} else {
			  
                                                                           
      
			printf("%s: pids_moved[%d] were less than  expected coalition ntasks[%d] \n", __FUNCTION__,
			    pids_moved, leaders[i].msi_ntasks);
		}
#endif

		total_pids_moved += pids_moved;
	}              

	return total_pids_moved;
}


  
                                                                                 
                                                              
                                                     
  
         
                                       
                                        
                                         
  
                                          
                                                            
                                                  
  
          
                                                                     
   
static int
memorystatus_move_list_locked(unsigned int bucket_index, pid_t *pid_list, int list_sz)
{
	memstat_bucket_t *current_bucket;
	int i;
	int found_pids = 0;

	if ((pid_list == NULL) || (list_sz <= 0)) {
		return 0;
	}

	if (bucket_index >= MEMSTAT_BUCKET_COUNT) {
		return 0;
	}

	current_bucket = &memstat_bucket[bucket_index];
	for (i = 0; i < list_sz; i++) {
		unsigned int b = bucket_index;
		proc_t p = NULL;
		proc_t aProc = NULL;
		pid_t  aPid;
		int list_index;

		list_index = ((list_sz - 1) - i);
		aPid = pid_list[list_index];

		                                               
		p = memorystatus_get_first_proc_locked(&b, FALSE);
		while (p) {
			if (p->p_pid == aPid) {
				aProc = p;
				break;
			}
			p = memorystatus_get_next_proc_locked(&b, p, FALSE);
		}

		if (aProc == NULL) {
			                                              
			continue;
		} else {
			TAILQ_REMOVE(&current_bucket->list, aProc, p_memstat_list);
			TAILQ_INSERT_HEAD(&current_bucket->list, aProc, p_memstat_list);
			found_pids++;
		}
	}
	return found_pids;
}

int
memorystatus_get_proccnt_upto_priority(int32_t max_bucket_index)
{
	int32_t i = JETSAM_PRIORITY_IDLE;
	int count = 0;

	if (max_bucket_index >= MEMSTAT_BUCKET_COUNT) {
		return -1;
	}

	while (i <= max_bucket_index) {
		count += memstat_bucket[i++].count;
	}

	return count;
}

int
memorystatus_update_priority_for_appnap(proc_t p, boolean_t is_appnap)
{
#if !CONFIG_JETSAM
	if (!p || (!isApp(p)) || (p->p_memstat_state & (P_MEMSTAT_INTERNAL | P_MEMSTAT_MANAGED))) {
		  
                                                           
    
                                                                         
                                                                            
                                                                        
                                                                                
                                                                                    
                                                                                   
    
                                                                              
                                                                      
     
		return -1;
	}

	  
                   
                                                                         
                                                               
                                                                         
                                                                       
                                                                                 
   
                                                                              
                                                                   
                                                                                 
                          
   
                                                                                    
                                                                                  
                                        
    

	memstat_bucket_t *current_bucket, *new_bucket;
	int32_t priority = 0;

	proc_list_lock();

	if (((p->p_listflag & P_LIST_EXITED) != 0) ||
	    (p->p_memstat_state & (P_MEMSTAT_ERROR | P_MEMSTAT_TERMINATED))) {
		  
                                        
                                                             
                                                 
     
		proc_list_unlock();
		return 0;
	}

	if (is_appnap) {
		current_bucket = &memstat_bucket[p->p_memstat_effectivepriority];
		new_bucket = &memstat_bucket[JETSAM_PRIORITY_IDLE];
		priority = JETSAM_PRIORITY_IDLE;
	} else {
		if (p->p_memstat_effectivepriority != JETSAM_PRIORITY_IDLE) {
			  
                                                     
                                                       
                 
      
			proc_list_unlock();
			return 0;
		}

		current_bucket = &memstat_bucket[JETSAM_PRIORITY_IDLE];
		new_bucket = &memstat_bucket[p->p_memstat_requestedpriority];
		priority = p->p_memstat_requestedpriority;
	}

	TAILQ_REMOVE(&current_bucket->list, p, p_memstat_list);
	current_bucket->count--;
	if (p->p_memstat_relaunch_flags & (P_MEMSTAT_RELAUNCH_HIGH)) {
		current_bucket->relaunch_high_count--;
	}
	TAILQ_INSERT_TAIL(&new_bucket->list, p, p_memstat_list);
	new_bucket->count++;
	if (p->p_memstat_relaunch_flags & (P_MEMSTAT_RELAUNCH_HIGH)) {
		new_bucket->relaunch_high_count++;
	}
	  
                                    
    
	if (p->p_memstat_effectivepriority == priority) {
		  
                                              
                                          
     
	} else if (p->p_memstat_effectivepriority == JETSAM_PRIORITY_IDLE) {
		uint64_t now;
		  
                                                   
                       
     
		assert(p->p_memstat_idle_start != 0);
		now = mach_absolute_time();
		if (now > p->p_memstat_idle_start) {
			p->p_memstat_idle_delta = now - p->p_memstat_idle_start;
		}
	} else if (priority == JETSAM_PRIORITY_IDLE) {
		  
                                                 
                       
     
		p->p_memstat_idle_start = mach_absolute_time();
	}

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_MEMSTAT, BSD_MEMSTAT_CHANGE_PRIORITY), p->p_pid, priority, p->p_memstat_effectivepriority, 0, 0);

	p->p_memstat_effectivepriority = priority;

	proc_list_unlock();

	return 0;

#else                     
	#pragma unused(p)
	#pragma unused(is_appnap)
	return -1;
#endif                     
}

uint64_t
memorystatus_available_memory_internal(struct proc *p)
{
#ifdef XNU_TARGET_OS_OSX
	if (p->p_memstat_memlimit <= 0) {
		return 0;
	}
#endif                        
	const uint64_t footprint_in_bytes = get_task_phys_footprint(p->task);
	int32_t memlimit_mb;
	int64_t memlimit_bytes;
	int64_t rc;

	if (isApp(p) == FALSE) {
		return 0;
	}

	if (p->p_memstat_memlimit > 0) {
		memlimit_mb = p->p_memstat_memlimit;
	} else if (task_convert_phys_footprint_limit(-1, &memlimit_mb) != KERN_SUCCESS) {
		return 0;
	}

	if (memlimit_mb <= 0) {
		memlimit_bytes = INT_MAX & ~((1 << 20) - 1);
	} else {
		memlimit_bytes = ((int64_t) memlimit_mb) << 20;
	}

	rc = memlimit_bytes - footprint_in_bytes;

	return (rc >= 0) ? rc : 0;
}

int
memorystatus_available_memory(struct proc *p, __unused struct memorystatus_available_memory_args *args, uint64_t *ret)
{
	*ret = memorystatus_available_memory_internal(p);

	return 0;
}

#if CONFIG_JETSAM
#if DEVELOPMENT || DEBUG
static int
memorystatus_cmd_increase_jetsam_task_limit(pid_t pid, uint32_t byte_increase)
{
	memorystatus_memlimit_properties_t mmp_entry;

	                     
	if ((pid == 0) || (byte_increase == 0)) {
		return EINVAL;
	}

	proc_t p = proc_find(pid);

	if (!p) {
		return ESRCH;
	}

	const uint32_t current_memlimit_increase = roundToNearestMB(p->p_memlimit_increase);
	                   
	const int32_t page_aligned_increase = (int32_t) MIN(round_page(p->p_memlimit_increase + byte_increase), INT32_MAX);

	proc_list_lock();

	memorystatus_get_memlimit_properties_internal(p, &mmp_entry);

	if (mmp_entry.memlimit_active > 0) {
		mmp_entry.memlimit_active -= current_memlimit_increase;
		mmp_entry.memlimit_active += roundToNearestMB(page_aligned_increase);
	}

	if (mmp_entry.memlimit_inactive > 0) {
		mmp_entry.memlimit_inactive -= current_memlimit_increase;
		mmp_entry.memlimit_inactive += roundToNearestMB(page_aligned_increase);
	}

	  
                                              
    
	p->p_memlimit_increase = page_aligned_increase;

	int error = memorystatus_set_memlimit_properties_internal(p, &mmp_entry);

	proc_list_unlock();
	proc_rele(p);

	return error;
}
#endif                  
#endif                    
