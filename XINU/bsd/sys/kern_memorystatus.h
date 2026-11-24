/*
 * Copyright (c) 2006-2018 Apple Computer, Inc. All rights reserved.
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

#ifndef SYS_MEMORYSTATUS_H
#define SYS_MEMORYSTATUS_H

#include <sys/time.h>
#include <mach_debug/zone_info.h>
#include <sys/proc.h>

#define MEMORYSTATUS_ENTITLEMENT "com.apple.private.memorystatus"

#define JETSAM_PRIORITY_REVISION                  2

#define JETSAM_PRIORITY_IDLE_HEAD                -2
                                                         
#define JETSAM_PRIORITY_IDLE                      0
#define JETSAM_PRIORITY_IDLE_DEFERRED             1                                                                             
#define JETSAM_PRIORITY_AGING_BAND1               JETSAM_PRIORITY_IDLE_DEFERRED
#define JETSAM_PRIORITY_BACKGROUND_OPPORTUNISTIC  2
#define JETSAM_PRIORITY_AGING_BAND2               JETSAM_PRIORITY_BACKGROUND_OPPORTUNISTIC
#define JETSAM_PRIORITY_BACKGROUND                3
#define JETSAM_PRIORITY_ELEVATED_INACTIVE         JETSAM_PRIORITY_BACKGROUND
#define JETSAM_PRIORITY_MAIL                      4
#define JETSAM_PRIORITY_PHONE                     5
#define JETSAM_PRIORITY_UI_SUPPORT                8
#define JETSAM_PRIORITY_FOREGROUND_SUPPORT        9
#define JETSAM_PRIORITY_FOREGROUND               10
#define JETSAM_PRIORITY_AUDIO_AND_ACCESSORY      12
#define JETSAM_PRIORITY_CONDUCTOR                13
#define JETSAM_PRIORITY_DRIVER_APPLE             15
#define JETSAM_PRIORITY_HOME                     16
#define JETSAM_PRIORITY_EXECUTIVE                17
#define JETSAM_PRIORITY_IMPORTANT                18
#define JETSAM_PRIORITY_CRITICAL                 19

#define JETSAM_PRIORITY_MAX                      21

                                                         
#define JETSAM_PRIORITY_DEFAULT                  18
#define JETSAM_PRIORITY_TELEPHONY                19

                   
#define DEFAULT_JETSAM_PRIORITY                  18

  
                                                                      
                                                              
                                
  
                                                
  
                                                                    
                                                                        
                                                                  
                                                                      
  
                                                                    
                                    
   
#define DEFERRED_IDLE_EXIT_TIME_SECS             10

#define KEV_MEMORYSTATUS_SUBCLASS                 3

enum {
	kMemorystatusLevelNote = 1,
	kMemorystatusSnapshotNote = 2,
	kMemorystatusFreezeNote = 3,
	kMemorystatusPressureNote = 4
};

enum {
	kMemorystatusLevelAny = -1,
	kMemorystatusLevelNormal = 0,
	kMemorystatusLevelWarning = 1,
	kMemorystatusLevelUrgent = 2,
	kMemorystatusLevelCritical = 3
};

#define KEV_DIRTYSTATUS_SUBCLASS                  4

enum {
	kDirtyStatusChangeNote = 1
};

typedef struct memorystatus_priority_entry {
	pid_t pid;
	int32_t priority;
	uint64_t user_data;
	int32_t limit;          
	uint32_t state;
} memorystatus_priority_entry_t;

  
                                                               
                                                                  
                                                                      
                                                                     
  
                                                                       
                              
   
#define MEMORYSTATUS_MPE_VERSION_1              1

#define MEMORYSTATUS_MPE_VERSION_1_SIZE         sizeof(struct memorystatus_properties_entry_v1)

typedef struct memorystatus_properties_entry_v1 {
	int version;
	pid_t pid;
	int32_t priority;
	int use_probability;
	uint64_t user_data;
	int32_t limit;          
	uint32_t state;
	char proc_name[MAXCOMLEN + 1];
	char __pad1[3];
} memorystatus_properties_entry_v1_t;

typedef struct memorystatus_kernel_stats {
	uint32_t free_pages;
	uint32_t active_pages;
	uint32_t inactive_pages;
	uint32_t throttled_pages;
	uint32_t purgeable_pages;
	uint32_t wired_pages;
	uint32_t speculative_pages;
	uint32_t filebacked_pages;
	uint32_t anonymous_pages;
	uint32_t compressor_pages;
	uint64_t compressions;
	uint64_t decompressions;
	uint64_t total_uncompressed_pages_in_compressor;
	uint64_t zone_map_size;
	uint64_t zone_map_capacity;
	uint64_t largest_zone_size;
	char     largest_zone_name[MACH_ZONE_NAME_MAX_LEN];
} memorystatus_kernel_stats_t;

typedef enum memorystatus_freeze_skip_reason {
	kMemorystatusFreezeSkipReasonNone = 0,
	kMemorystatusFreezeSkipReasonExcessSharedMemory = 1,
	kMemorystatusFreezeSkipReasonLowPrivateSharedRatio = 2,
	kMemorystatusFreezeSkipReasonNoCompressorSpace = 3,
	kMemorystatusFreezeSkipReasonNoSwapSpace = 4,
	kMemorystatusFreezeSkipReasonBelowMinPages = 5,
	kMemorystatusFreezeSkipReasonLowProbOfUse = 6,
	kMemorystatusFreezeSkipReasonOther = 7,
	kMemorystatusFreezeSkipReasonOutOfBudget = 8,
	kMemorystatusFreezeSkipReasonOutOfSlots = 9,
	kMemorystatusFreezeSkipReasonDisabled = 10,
	_kMemorystatusFreezeSkipReasonMax
} memorystatus_freeze_skip_reason_t;
  
                                    
                                                                                            
  

typedef struct jetsam_snapshot_entry {
	pid_t    pid;
	char     name[(2 * MAXCOMLEN) + 1];
	int32_t  priority;
	uint32_t state;
	uint32_t fds;
	memorystatus_freeze_skip_reason_t jse_freeze_skip_reason;                                      
	uint8_t  uuid[16];
	uint64_t user_data;
	uint64_t killed;
	uint64_t pages;
	uint64_t max_pages_lifetime;
	uint64_t purgeable_pages;
	uint64_t jse_internal_pages;
	uint64_t jse_internal_compressed_pages;
	uint64_t jse_purgeable_nonvolatile_pages;
	uint64_t jse_purgeable_nonvolatile_compressed_pages;
	uint64_t jse_alternate_accounting_pages;
	uint64_t jse_alternate_accounting_compressed_pages;
	uint64_t jse_iokit_mapped_pages;
	uint64_t jse_page_table_pages;
	uint64_t jse_memory_region_count;
	uint64_t jse_gencount;                                                              
	uint64_t jse_starttime;                                                        
	uint64_t jse_killtime;                                                                           
	uint64_t jse_idle_delta;                                             
	uint64_t jse_coalition_jetsam_id;                                                                  
	struct timeval64 cpu_time;
	uint64_t jse_thaw_count;
	uint64_t jse_frozen_to_swap_pages;
} memorystatus_jetsam_snapshot_entry_t;

typedef struct jetsam_snapshot {
	uint64_t snapshot_time;                                                             
	uint64_t notification_time;                                                      
	uint64_t js_gencount;                                                               
	memorystatus_kernel_stats_t stats;                                                    
	size_t entry_count;
	memorystatus_jetsam_snapshot_entry_t entries[];
} memorystatus_jetsam_snapshot_t;

typedef enum dirty_status_change_event_type {
	kDirtyStatusChangedDirty = 0x0,
	kDirtyStatusChangedClean = 0x1
} dirty_status_change_event_type_t;

typedef struct dirty_status_change_event {
	pid_t    dsc_pid;
	char     dsc_process_name[(2 * MAXCOMLEN) + 1];
	dirty_status_change_event_type_t dsc_event_type;
	uint64_t dsc_time;
	uint64_t dsc_pages;
	int32_t dsc_priority;
} dirty_status_change_event_t;

                                                     
#define kMaxSnapshotEntries 192

  
                                  
   
extern memorystatus_jetsam_snapshot_t *memorystatus_jetsam_snapshot;
extern memorystatus_jetsam_snapshot_t *memorystatus_jetsam_snapshot_copy;
#if CONFIG_FREEZE
extern memorystatus_jetsam_snapshot_t *memorystatus_jetsam_snapshot_freezer;
extern unsigned int memorystatus_jetsam_snapshot_freezer_max;
extern unsigned int memorystatus_jetsam_snapshot_freezer_size;
#endif                    
extern unsigned int memorystatus_jetsam_snapshot_count;
extern unsigned int memorystatus_jetsam_snapshot_copy_count;
extern unsigned int memorystatus_jetsam_snapshot_max;
extern unsigned int memorystatus_jetsam_snapshot_size;
extern uint64_t memorystatus_jetsam_snapshot_last_timestamp;
extern uint64_t memorystatus_jetsam_snapshot_timeout;
#define memorystatus_jetsam_snapshot_list memorystatus_jetsam_snapshot->entries
#define JETSAM_SNAPSHOT_TIMEOUT_SECS 30

                                

extern uint64_t memorystatus_sysprocs_idle_delay_time;
extern uint64_t memorystatus_apps_idle_delay_time;

           
#define kMemorystatusSuspended        0x01
#define kMemorystatusFrozen           0x02
#define kMemorystatusWasThawed        0x04
#define kMemorystatusTracked          0x08
#define kMemorystatusSupportsIdleExit 0x10
#define kMemorystatusDirty            0x20
#define kMemorystatusAssertion        0x40

  
                                                           
  
                                            
                                 
                                    
                                 
   
#define JETSAM_REASON_INVALID                                                           0
#define JETSAM_REASON_GENERIC                                                           1
#define JETSAM_REASON_MEMORY_HIGHWATER                                          2
#define JETSAM_REASON_VNODE                                                                     3
#define JETSAM_REASON_MEMORY_VMPAGESHORTAGE                                     4
#define JETSAM_REASON_MEMORY_PROCTHRASHING                                      5
#define JETSAM_REASON_MEMORY_FCTHRASHING                                        6
#define JETSAM_REASON_MEMORY_PERPROCESSLIMIT                            7
#define JETSAM_REASON_MEMORY_DISK_SPACE_SHORTAGE                        8
#define JETSAM_REASON_MEMORY_IDLE_EXIT                                          9
#define JETSAM_REASON_ZONE_MAP_EXHAUSTION                                       10
#define JETSAM_REASON_MEMORY_VMCOMPRESSOR_THRASHING                     11
#define JETSAM_REASON_MEMORY_VMCOMPRESSOR_SPACE_SHORTAGE        12
#define JETSAM_REASON_LOWSWAP                                   13
#define JETSAM_REASON_MEMORYSTATUS_MAX  JETSAM_REASON_LOWSWAP

  
                                                               
   
#define JETSAM_REASON_CPULIMIT                  100

           
enum {
	kMemorystatusInvalid                                                    = JETSAM_REASON_INVALID,
	kMemorystatusKilled                                                             = JETSAM_REASON_GENERIC,
	kMemorystatusKilledHiwat                                                = JETSAM_REASON_MEMORY_HIGHWATER,
	kMemorystatusKilledVnodes                                               = JETSAM_REASON_VNODE,
	kMemorystatusKilledVMPageShortage                               = JETSAM_REASON_MEMORY_VMPAGESHORTAGE,
	kMemorystatusKilledProcThrashing                                = JETSAM_REASON_MEMORY_PROCTHRASHING,
	kMemorystatusKilledFCThrashing                                  = JETSAM_REASON_MEMORY_FCTHRASHING,
	kMemorystatusKilledPerProcessLimit                              = JETSAM_REASON_MEMORY_PERPROCESSLIMIT,
	kMemorystatusKilledDiskSpaceShortage                    = JETSAM_REASON_MEMORY_DISK_SPACE_SHORTAGE,
	kMemorystatusKilledIdleExit                                             = JETSAM_REASON_MEMORY_IDLE_EXIT,
	kMemorystatusKilledZoneMapExhaustion                    = JETSAM_REASON_ZONE_MAP_EXHAUSTION,
	kMemorystatusKilledVMCompressorThrashing                = JETSAM_REASON_MEMORY_VMCOMPRESSOR_THRASHING,
	kMemorystatusKilledVMCompressorSpaceShortage    = JETSAM_REASON_MEMORY_VMCOMPRESSOR_SPACE_SHORTAGE,
	kMemorystatusKilledLowSwap                      = JETSAM_REASON_LOWSWAP,
};

  
                              
                                                                       
                                           
   
#define kMemorystatusKilledDiagnostic           kMemorystatusKilledDiskSpaceShortage
#define kMemorystatusKilledVMThrashing          kMemorystatusKilledVMCompressorThrashing
#define JETSAM_REASON_MEMORY_VMTHRASHING        JETSAM_REASON_MEMORY_VMCOMPRESSOR_THRASHING

                          
#define MEMORYSTATUS_BUFFERSIZE_MAX 65536

#ifndef KERNEL
int memorystatus_get_level(user_addr_t level);
int memorystatus_control(uint32_t command, int32_t pid, uint32_t flags, void *buffer, size_t buffersize);
#endif

              
#define MEMORYSTATUS_CMD_GET_PRIORITY_LIST            1
#define MEMORYSTATUS_CMD_SET_PRIORITY_PROPERTIES      2
#define MEMORYSTATUS_CMD_GET_JETSAM_SNAPSHOT          3
#define MEMORYSTATUS_CMD_GET_PRESSURE_STATUS          4
#define MEMORYSTATUS_CMD_SET_JETSAM_HIGH_WATER_MARK   5                                                                         
#define MEMORYSTATUS_CMD_SET_JETSAM_TASK_LIMIT        6                                                                     
#define MEMORYSTATUS_CMD_SET_MEMLIMIT_PROPERTIES      7                                                           
#define MEMORYSTATUS_CMD_GET_MEMLIMIT_PROPERTIES      8                                               
#define MEMORYSTATUS_CMD_PRIVILEGED_LISTENER_ENABLE   9                                                                                    
#define MEMORYSTATUS_CMD_PRIVILEGED_LISTENER_DISABLE  10                                                                                     
#define MEMORYSTATUS_CMD_AGGRESSIVE_JETSAM_LENIENT_MODE_ENABLE  11                                                                                                           
#define MEMORYSTATUS_CMD_AGGRESSIVE_JETSAM_LENIENT_MODE_DISABLE 12                                                          
#define MEMORYSTATUS_CMD_GET_MEMLIMIT_EXCESS          13                                                                                  
#define MEMORYSTATUS_CMD_ELEVATED_INACTIVEJETSAMPRIORITY_ENABLE         14                                                                                      
#define MEMORYSTATUS_CMD_ELEVATED_INACTIVEJETSAMPRIORITY_DISABLE        15                                                                          
#define MEMORYSTATUS_CMD_SET_PROCESS_IS_MANAGED       16                                                                                                     
#define MEMORYSTATUS_CMD_GET_PROCESS_IS_MANAGED       17                                                 
#define MEMORYSTATUS_CMD_SET_PROCESS_IS_FREEZABLE     18                                                                                                               
                                                                                                                                                     
#define MEMORYSTATUS_CMD_GET_PROCESS_IS_FREEZABLE     19                                                 

#define MEMORYSTATUS_CMD_FREEZER_CONTROL              20

#define MEMORYSTATUS_CMD_GET_AGGRESSIVE_JETSAM_LENIENT_MODE      21                                                                    

#define MEMORYSTATUS_CMD_INCREASE_JETSAM_TASK_LIMIT   22                                                                                         

#if PRIVATE
#define MEMORYSTATUS_CMD_SET_TESTING_PID 23                                                         
#endif              

#define MEMORYSTATUS_CMD_GET_PROCESS_IS_FROZEN 24                                      

                                               
#define MEMORYSTATUS_CMD_GRP_SET_PROPERTIES           100

#if PRIVATE
                   

                           
#define MEMORYSTATUS_CMD_TEST_JETSAM            1000
#define MEMORYSTATUS_CMD_TEST_JETSAM_SORT       1001

                             
typedef struct memorystatus_jetsam_panic_options {
	uint32_t data;
	uint32_t mask;
} memorystatus_jetsam_panic_options_t;

#define MEMORYSTATUS_CMD_SET_JETSAM_PANIC_BITS        1002

                                     
#define JETSAM_SORT_NOSORT      0
#define JETSAM_SORT_DEFAULT     1

#endif              

                                  

#define MEMORYSTATUS_FLAGS_SNAPSHOT_ON_DEMAND           0x1                                                            
#define MEMORYSTATUS_FLAGS_SNAPSHOT_AT_BOOT             0x2                                                             
#define MEMORYSTATUS_FLAGS_SNAPSHOT_COPY                0x4                                                                          
#define MEMORYSTATUS_FLAGS_GRP_SET_PRIORITY             0x8                                                    
#define MEMORYSTATUS_FLAGS_GRP_SET_PROBABILITY          0x10                                                         

#if PRIVATE
#define MEMORYSTATUS_FLAGS_SET_TESTING_PID     0x20                                   
#define MEMORYSTATUS_FLAGS_UNSET_TESTING_PID   0x40                                   
#endif              

#define MEMORYSTATUS_FLAGS_SNAPSHOT_FREEZER             0x80                                                                       
  
                                     
                                       
  
                                                   
                                                
                                                 
            
  
                                     
                                               
                                          
  
                                    
                                        
                                       
                                      
                                        
  
                                                 
                                                      
                                
  
                                                 
                                               
   

                                                                
#define MEMORYSTATUS_SNAPSHOT_ON_DEMAND         MEMORYSTATUS_FLAGS_SNAPSHOT_ON_DEMAND
#define MEMORYSTATUS_SNAPSHOT_AT_BOOT           MEMORYSTATUS_FLAGS_SNAPSHOT_AT_BOOT
#define MEMORYSTATUS_SNAPSHOT_COPY              MEMORYSTATUS_FLAGS_SNAPSHOT_COPY

  
                                     
                                           
   
typedef struct memorystatus_priority_properties {
	int32_t  priority;
	uint64_t user_data;
} memorystatus_priority_properties_t;

  
                                                                                
   
#define MEMORYSTATUS_SET_PRIORITY_ASSERTION     0x1

  
                                     
                                           
                                           
   
typedef struct memorystatus_memlimit_properties {
	int32_t memlimit_active;                                                                        
	uint32_t memlimit_active_attr;
	int32_t memlimit_inactive;                                                                        
	uint32_t memlimit_inactive_attr;
} memorystatus_memlimit_properties_t;

typedef struct memorystatus_memlimit_properties2 {
	memorystatus_memlimit_properties_t v1;
	uint32_t memlimit_increase;                                                                                      
	uint32_t memlimit_increase_bytes;                                                                                                     
} memorystatus_memlimit_properties2_t;

#define MEMORYSTATUS_MEMLIMIT_ATTR_FATAL        0x1                                                  

#ifdef XNU_KERNEL_PRIVATE

  
                                                                                     
                            
                                   
                                 
  
                                                                                      
                                                                 
                                
                          
  
                                                       
                                 
                        
                          
  
                                                    
                                      
                                      
                                        
   

  
                             
                                               
   

#define P_MEMSTAT_SUSPENDED            0x00000001                                                       
#define P_MEMSTAT_FROZEN               0x00000002                                                             
#define P_MEMSTAT_FREEZE_DISABLED      0x00000004                                                           
#define P_MEMSTAT_ERROR                0x00000008                                                                                                                
#define P_MEMSTAT_LOCKED               0x00000010                                                                    
#define P_MEMSTAT_TERMINATED           0x00000020                         
#define P_MEMSTAT_FREEZE_IGNORE        0x00000040                                                                                                                
#define P_MEMSTAT_PRIORITYUPDATED      0x00000080                                              
#define P_MEMSTAT_FOREGROUND           0x00000100                                                   
#define P_MEMSTAT_REFREEZE_ELIGIBLE    0x00000400                                                                                                     
#define P_MEMSTAT_MANAGED              0x00000800                                                                               
#define P_MEMSTAT_INTERNAL             0x00001000                                                                         
#define P_MEMSTAT_FATAL_MEMLIMIT                  0x00002000                                                      
#define P_MEMSTAT_MEMLIMIT_ACTIVE_FATAL           0x00004000                                                                      
#define P_MEMSTAT_MEMLIMIT_INACTIVE_FATAL         0x00008000                                                                      
#define P_MEMSTAT_USE_ELEVATED_INACTIVE_BAND      0x00010000                                                                                            
                                                                                                       
#define P_MEMSTAT_PRIORITY_ASSERTION              0x00020000                                                        
#define P_MEMSTAT_FREEZE_CONSIDERED               0x00040000                                                          

  
                                 
                                          
   
#define P_MEMSTAT_RELAUNCH_UNKNOWN      0x0
#define P_MEMSTAT_RELAUNCH_LOW          0x1
#define P_MEMSTAT_RELAUNCH_MED          0x2
#define P_MEMSTAT_RELAUNCH_HIGH         0x4

  
                                                                         
                                                                           
  
                                                                                                                
                                                                                                                
   
#define isApp(p)            ((p->p_memstat_state & P_MEMSTAT_MANAGED) || ! (p->p_memstat_dirty & P_DIRTY_TRACK))
#define isSysProc(p)            ( ! (p->p_memstat_state & P_MEMSTAT_MANAGED) || (p->p_memstat_dirty & P_DIRTY_TRACK))

#define MEMSTAT_BUCKET_COUNT (JETSAM_PRIORITY_MAX + 1)

typedef struct memstat_bucket {
	TAILQ_HEAD(, proc) list;
	int count;
	int relaunch_high_count;
} memstat_bucket_t;

extern memstat_bucket_t memstat_bucket[MEMSTAT_BUCKET_COUNT];

  
                                                    
                               
   
typedef struct memorystatus_internal_probabilities {
	char proc_name[MAXCOMLEN + 1];
	int use_probability;
} memorystatus_internal_probabilities_t;

extern memorystatus_internal_probabilities_t *memorystatus_global_probabilities_table;
extern size_t memorystatus_global_probabilities_size;


extern void memorystatus_init(void);

extern void memorystatus_init_at_boot_snapshot(void);

extern int memorystatus_add(proc_t p, boolean_t locked);
extern int memorystatus_update(proc_t p, int priority, uint64_t user_data, boolean_t is_assertion, boolean_t effective,
    boolean_t update_memlimit, int32_t memlimit_active, boolean_t memlimit_active_is_fatal,
    int32_t memlimit_inactive, boolean_t memlimit_inactive_is_fatal);

                                                                 
                                            
                                   
                                                                                                                   
   
extern int memorystatus_remove(proc_t p);

int memorystatus_update_inactive_jetsam_priority_band(pid_t pid, uint32_t opflags, int priority, boolean_t effective_now);
int memorystatus_relaunch_flags_update(proc_t p, int relaunch_flags);

extern int memorystatus_dirty_track(proc_t p, uint32_t pcontrol);
extern int memorystatus_dirty_set(proc_t p, boolean_t self, uint32_t pcontrol);
extern int memorystatus_dirty_get(proc_t p, boolean_t locked);
extern int memorystatus_dirty_clear(proc_t p, uint32_t pcontrol);

extern int memorystatus_on_terminate(proc_t p);

extern void memorystatus_on_suspend(proc_t p);
extern void memorystatus_on_resume(proc_t p);
extern void memorystatus_on_inactivity(proc_t p);

extern void memorystatus_on_pageout_scan_end(void);

                         

void memorystatus_kevent_init(lck_grp_t *grp, lck_attr_t *attr);

int memorystatus_knote_register(struct knote *kn);
void memorystatus_knote_unregister(struct knote *kn);

#if CONFIG_MEMORYSTATUS
void memorystatus_log_exception(const int max_footprint_mb, boolean_t memlimit_is_active, boolean_t memlimit_is_fatal);
void memorystatus_on_ledger_footprint_exceeded(int warning, boolean_t memlimit_is_active, boolean_t memlimit_is_fatal);
void proc_memstat_terminated(proc_t p, boolean_t set);
void memorystatus_proc_flags_unsafe(void * v, boolean_t *is_dirty, boolean_t *is_dirty_tracked, boolean_t *allow_idle_exit);

#if __arm64__
void memorystatus_act_on_legacy_footprint_entitlement(proc_t p, boolean_t footprint_increase);
void memorystatus_act_on_ios13extended_footprint_entitlement(proc_t p);
void memorystatus_act_on_entitled_task_limit(proc_t p);
#endif                

#endif                          

int memorystatus_get_pressure_status_kdp(void);

#if CONFIG_JETSAM

typedef enum memorystatus_policy {
	kPolicyDefault        = 0x0,
	kPolicyMoreFree       = 0x1,
} memorystatus_policy_t;

boolean_t memorystatus_kill_on_VM_page_shortage(boolean_t async);
boolean_t memorystatus_kill_on_FC_thrashing(boolean_t async);
boolean_t memorystatus_kill_on_VM_compressor_thrashing(boolean_t async);
boolean_t memorystatus_kill_on_vnode_limit(void);

void jetsam_on_ledger_cpulimit_exceeded(void);
void memorystatus_fast_jetsam_override(boolean_t enable_override);

#endif                    

                                                
                         
   
#if MEMORYSTATUS_DEBUG_LOG
#define MEMORYSTATUS_DEBUG(cond, format, ...)      \
do {                                              \
if (cond) { printf(format, ##__VA_ARGS__); } \
} while(0)
#else
#define MEMORYSTATUS_DEBUG(cond, format, ...)
#endif

boolean_t memorystatus_kill_on_zone_map_exhaustion(pid_t pid);
boolean_t memorystatus_kill_on_VM_compressor_space_shortage(boolean_t async);
void memorystatus_pages_update(unsigned int pages_avail);
boolean_t memorystatus_idle_exit_from_VM(void);
proc_t memorystatus_get_first_proc_locked(unsigned int *bucket_index, boolean_t search);
proc_t memorystatus_get_next_proc_locked(unsigned int *bucket_index, proc_t p, boolean_t search);
void memorystatus_get_task_page_counts(task_t task, uint32_t *footprint, uint32_t *max_footprint_lifetime, uint32_t *purgeable_pages);
void memorystatus_invalidate_idle_demotion_locked(proc_t p, boolean_t clean_state);
void memorystatus_update_priority_locked(proc_t p, int priority, boolean_t head_insert, boolean_t skip_demotion_check);

#if VM_PRESSURE_EVENTS

extern kern_return_t memorystatus_update_vm_pressure(boolean_t);

#if CONFIG_MEMORYSTATUS
           
extern int memorystatus_low_mem_privileged_listener(uint32_t op_flags);
extern int memorystatus_send_pressure_note(int pid);
extern boolean_t memorystatus_is_foreground_locked(proc_t p);
extern boolean_t memorystatus_bg_pressure_eligible(proc_t p);
#endif                          

#endif                         

#endif                         

#endif                         
