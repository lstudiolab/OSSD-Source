/*
 * Copyright (c) 2015 Apple Inc. All rights reserved.
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


  
  
                                           
  
                                                                               
                                                                                
                                                                             
                                                         
  
                                                                               
                                                                             
                                                                      
                                                         
  
                                                                             
                                           
  
                               
  
                                                                               
                
  
                                                                             
                
  
                                                                              
                                                                              
                                                                          
                                                                                
                                        
  
                                                                            
  
                                                                          
                                                                               
                   
  
                                            
  
                           
                             
                              
                                       
  
                                
  
                           
                             
                              
                                                                                        
                                 
                                       
  
                                                
  
                                                                                  
                                                                          
  
                                                     
  
                                                                              
                                                                                
                
  
                                                                              
                                                                        
  
                                                                           
                                                                            
                                                                           
                                                                         
  
                                                                     
                                                                          
  
  
  
                                                        
  
  
                                                              
  
                            
  
                                  
                                  
                                  
                                  
                                  
                                  
                                  
                                  
                                  
                                  
                                  
                                  
  
  
                                                                                                                                   
                                                                                                       
  
                                                                                                                           
                                                                                                                             
                                                                                                                       
  
                                                     
                      
                                                                                    
  
                                                                              
                                                        
                                                                                                                                             
  
                 
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                                                                          
                    
  
                                                           
                     
                                                                                                         
                                                                                             
  
                                                                                                           
                                                                                                              
  
                                                                                                                 
                                                               
  
                                                                                                               
  
                                                     
                       
                                                                                                                                    
                                                                                                                                 
                                                                                                          
  
                                
                                        
                                       
                                            
                                        
                              
  
  
                                                           
                                                                                                          
                                                                                                         
                                                                                                                                      
                                                                                                                 
     
  
                                                                 
                                                                                            
                                                                                                       
  
                                   
                       
                                                                                                     
                                                                                                  
                                                                                                   
                                                                                    
  
                                    
                                                                  
         
                                                      
                                                                                       
                                                                              
                                                                        
  
                                   
                                                            
                                                                            
                                                                                         
  
                                                                                                     
                                                                           
                                                         
  
                                                                          
                                                                               
                                                                                     
                                                                                                  
              
  
                                                                   
                                                                                                
                                                                                                   
                                                                                                           
                                                                          
                                                                             
          
  
                                                        
                                                                                                  
                                                                                                     
                         
  
                                                                           
                                                                            
             
   


#ifndef _KCDATA_H_
#define _KCDATA_H_

#include <stdint.h>
#include <string.h>
#include <uuid/uuid.h>

#define KCDATA_DESC_MAXLEN 32                                 

#define KCDATA_FLAGS_STRUCT_PADDING_MASK 0xf
#define KCDATA_FLAGS_STRUCT_HAS_PADDING 0x80

  
                                                
   
#define KCDATA_ALIGNMENT_SIZE       0x10

struct kcdata_item {
	uint32_t type;
	uint32_t size;                 
	                        
                  
                                  
                                                
                                                        
                  
                                                                      
                                                                    
                                                                             
                                                                            
                                                                      
                                                                        
                                                                             
                                         
                  
                                                             
                                                                     
                                                                          
                                                                         
                        
                  
                                                                            
                  
                                                                              
                  
                                                                          
                                                  
                  
                                  
                                          
                  
                              
                                                        
                                                               
                   
	uint64_t flags;
	char data[];                         
};

typedef struct kcdata_item * kcdata_item_t;

enum KCDATA_SUBTYPE_TYPES { KC_ST_CHAR = 1, KC_ST_INT8, KC_ST_UINT8, KC_ST_INT16, KC_ST_UINT16, KC_ST_INT32, KC_ST_UINT32, KC_ST_INT64, KC_ST_UINT64 };
typedef enum KCDATA_SUBTYPE_TYPES kctype_subtype_t;

  
                                               
                                                  
                                                          
                 
   
struct kcdata_subtype_descriptor {
	uint8_t kcs_flags;
#define KCS_SUBTYPE_FLAGS_NONE 0x0
#define KCS_SUBTYPE_FLAGS_ARRAY 0x1
                                              
  
                                                                        
                                                                         
                                                                         
                                                                    
                       
  
                                                                       
                                                                         
                                                               
  
                                                                              
                                                                              
                                       
   
#define KCS_SUBTYPE_FLAGS_STRUCT 0x2                                                                    
#define KCS_SUBTYPE_FLAGS_MERGE 0x4                                                                                  
	uint8_t kcs_elem_type;                                                                  
	uint16_t kcs_elem_offset;                                                                     
	uint32_t kcs_elem_size;                                                                                   
	char                 kcs_name[KCDATA_DESC_MAXLEN];                                      
};

typedef struct kcdata_subtype_descriptor * kcdata_subtype_descriptor_t;

  
                                                         
                                      
                                                           
   
#define KCS_SUBTYPE_PACK_SIZE(e_count, e_size) (((e_count)&0xffffu) << 16 | ((e_size)&0xffffu))

static inline uint32_t
kcs_get_elem_size(kcdata_subtype_descriptor_t d)
{
	if (d->kcs_flags & KCS_SUBTYPE_FLAGS_ARRAY) {
		                                                                      
		return (uint32_t)((d->kcs_elem_size & 0xffff) * ((d->kcs_elem_size & 0xffff0000) >> 16));
	}
	return d->kcs_elem_size;
}

static inline uint32_t
kcs_get_elem_count(kcdata_subtype_descriptor_t d)
{
	if (d->kcs_flags & KCS_SUBTYPE_FLAGS_ARRAY) {
		return (d->kcs_elem_size >> 16) & 0xffff;
	}
	return 1;
}

static inline int
kcs_set_elem_size(kcdata_subtype_descriptor_t d, uint32_t size, uint32_t count)
{
	if (count > 1) {
		                                      
		if (size > 0xffff || count > 0xffff) {
			return -1;                   
		}
		d->kcs_elem_size = ((count & 0xffff) << 16 | (size & 0xffff));
	} else {
		d->kcs_elem_size = size;
	}
	return 0;
}

struct kcdata_type_definition {
	uint32_t kct_type_identifier;
	uint32_t kct_num_elements;
	char kct_name[KCDATA_DESC_MAXLEN];
	struct kcdata_subtype_descriptor kct_elements[];
};


                                                                   
                                                                       
                               
   

  
                                
                                                                 
                                                                    
   
#define KCDATA_TYPE_INVALID 0x0u
#define KCDATA_TYPE_STRING_DESC 0x1u
#define KCDATA_TYPE_UINT32_DESC 0x2u
#define KCDATA_TYPE_UINT64_DESC 0x3u
#define KCDATA_TYPE_INT32_DESC 0x4u
#define KCDATA_TYPE_INT64_DESC 0x5u
#define KCDATA_TYPE_BINDATA_DESC 0x6u

  
                            
   
#define KCDATA_TYPE_ARRAY 0x11u                                                  
#define KCDATA_TYPE_TYPEDEFINTION 0x12u                                                  
#define KCDATA_TYPE_CONTAINER_BEGIN                                       \
	0x13u                                                                  
                                                                     
                                                                        
          
#define KCDATA_TYPE_CONTAINER_END 0x14u

#define KCDATA_TYPE_ARRAY_PAD0 0x20u                                          
#define KCDATA_TYPE_ARRAY_PAD1 0x21u                                          
#define KCDATA_TYPE_ARRAY_PAD2 0x22u                                          
#define KCDATA_TYPE_ARRAY_PAD3 0x23u                                          
#define KCDATA_TYPE_ARRAY_PAD4 0x24u                                          
#define KCDATA_TYPE_ARRAY_PAD5 0x25u                                          
#define KCDATA_TYPE_ARRAY_PAD6 0x26u                                          
#define KCDATA_TYPE_ARRAY_PAD7 0x27u                                          
#define KCDATA_TYPE_ARRAY_PAD8 0x28u                                          
#define KCDATA_TYPE_ARRAY_PAD9 0x29u                                          
#define KCDATA_TYPE_ARRAY_PADa 0x2au                                          
#define KCDATA_TYPE_ARRAY_PADb 0x2bu                                          
#define KCDATA_TYPE_ARRAY_PADc 0x2cu                                          
#define KCDATA_TYPE_ARRAY_PADd 0x2du                                          
#define KCDATA_TYPE_ARRAY_PADe 0x2eu                                          
#define KCDATA_TYPE_ARRAY_PADf 0x2fu                                          

  
                                                 
   
#define KCDATA_TYPE_LIBRARY_LOADINFO 0x30u                                 
#define KCDATA_TYPE_LIBRARY_LOADINFO64 0x31u                               
#define KCDATA_TYPE_TIMEBASE 0x32u                                          
#define KCDATA_TYPE_MACH_ABSOLUTE_TIME 0x33u               
#define KCDATA_TYPE_TIMEVAL 0x34u                                  
#define KCDATA_TYPE_USECS_SINCE_EPOCH 0x35u                              
#define KCDATA_TYPE_PID 0x36u                             
#define KCDATA_TYPE_PROCNAME 0x37u                       
#define KCDATA_TYPE_NESTED_KCDATA 0x38u                                
#define KCDATA_TYPE_LIBRARY_AOTINFO 0x39u                                     

#define KCDATA_TYPE_BUFFER_END 0xF19158EDu

                                                       
  
                                                                           
                                      
   

#define KCDATA_BUFFER_BEGIN_CRASHINFO 0xDEADF157u                                              
                                                                                            
#define KCDATA_BUFFER_BEGIN_STACKSHOT 0x59a25807u                                        
                                                                                            
#define KCDATA_BUFFER_BEGIN_COMPRESSED 0x434f4d50u                                       
                                                                                            
#define KCDATA_BUFFER_BEGIN_DELTA_STACKSHOT 0xDE17A59Au                                  
                                                                                            
#define KCDATA_BUFFER_BEGIN_OS_REASON 0x53A20900u                                     
                                                                                            
#define KCDATA_BUFFER_BEGIN_XNUPOST_CONFIG 0x1e21c09fu                                              
                                                                                            

                                             
                                                                
#define XNUPOST_KCTYPE_TESTCONFIG               0x1040

                                                                  

                                                                             
#define STACKSHOT_IO_NUM_PRIORITIES     4
                                                                
#define STACKSHOT_MAX_THREAD_NAME_SIZE  64

  
                                                                       
                               
   
#define STACKSHOT_KCTYPE_IOSTATS                     0x901u                        
#define STACKSHOT_KCTYPE_GLOBAL_MEM_STATS            0x902u                                 
#define STACKSHOT_KCCONTAINER_TASK                   0x903u
#define STACKSHOT_KCCONTAINER_THREAD                 0x904u
#define STACKSHOT_KCTYPE_TASK_SNAPSHOT               0x905u                       
#define STACKSHOT_KCTYPE_THREAD_SNAPSHOT             0x906u                                             
#define STACKSHOT_KCTYPE_DONATING_PIDS               0x907u            
#define STACKSHOT_KCTYPE_SHAREDCACHE_LOADINFO        0x908u                                 
#define STACKSHOT_KCTYPE_THREAD_NAME                 0x909u             
#define STACKSHOT_KCTYPE_KERN_STACKFRAME             0x90Au                                    
#define STACKSHOT_KCTYPE_KERN_STACKFRAME64           0x90Bu                                    
#define STACKSHOT_KCTYPE_USER_STACKFRAME             0x90Cu                                    
#define STACKSHOT_KCTYPE_USER_STACKFRAME64           0x90Du                                    
#define STACKSHOT_KCTYPE_BOOTARGS                    0x90Eu                       
#define STACKSHOT_KCTYPE_OSVERSION                   0x90Fu                        
#define STACKSHOT_KCTYPE_KERN_PAGE_SIZE              0x910u                                   
#define STACKSHOT_KCTYPE_JETSAM_LEVEL                0x911u                               
#define STACKSHOT_KCTYPE_DELTA_SINCE_TIMESTAMP       0x912u                                             
#define STACKSHOT_KCTYPE_KERN_STACKLR                0x913u               
#define STACKSHOT_KCTYPE_KERN_STACKLR64              0x914u               
#define STACKSHOT_KCTYPE_USER_STACKLR                0x915u               
#define STACKSHOT_KCTYPE_USER_STACKLR64              0x916u               
#define STACKSHOT_KCTYPE_NONRUNNABLE_TIDS            0x917u               
#define STACKSHOT_KCTYPE_NONRUNNABLE_TASKS           0x918u               
#define STACKSHOT_KCTYPE_CPU_TIMES                   0x919u                                                           
#define STACKSHOT_KCTYPE_STACKSHOT_DURATION          0x91au                                
#define STACKSHOT_KCTYPE_STACKSHOT_FAULT_STATS       0x91bu                                   
#define STACKSHOT_KCTYPE_KERNELCACHE_LOADINFO        0x91cu                                                                 
#define STACKSHOT_KCTYPE_THREAD_WAITINFO             0x91du                                       
#define STACKSHOT_KCTYPE_THREAD_GROUP_SNAPSHOT       0x91eu                                                               
#define STACKSHOT_KCTYPE_THREAD_GROUP                0x91fu               
#define STACKSHOT_KCTYPE_JETSAM_COALITION_SNAPSHOT   0x920u                                       
#define STACKSHOT_KCTYPE_JETSAM_COALITION            0x921u               
#define STACKSHOT_KCTYPE_THREAD_POLICY_VERSION       0x922u                                                      
#define STACKSHOT_KCTYPE_INSTRS_CYCLES               0x923u                                    
#define STACKSHOT_KCTYPE_USER_STACKTOP               0x924u                                     
#define STACKSHOT_KCTYPE_ASID                        0x925u               
#define STACKSHOT_KCTYPE_PAGE_TABLES                 0x926u               
#define STACKSHOT_KCTYPE_SYS_SHAREDCACHE_LAYOUT      0x927u                                             
#define STACKSHOT_KCTYPE_THREAD_DISPATCH_QUEUE_LABEL 0x928u                           
#define STACKSHOT_KCTYPE_THREAD_TURNSTILEINFO        0x929u                                            
#define STACKSHOT_KCTYPE_TASK_CPU_ARCHITECTURE       0x92au                                        
#define STACKSHOT_KCTYPE_LATENCY_INFO                0x92bu                                          
#define STACKSHOT_KCTYPE_LATENCY_INFO_TASK           0x92cu                                    
#define STACKSHOT_KCTYPE_LATENCY_INFO_THREAD         0x92du                                      
#define STACKSHOT_KCTYPE_LOADINFO64_TEXT_EXEC        0x92eu                                                                    
#define STACKSHOT_KCTYPE_AOTCACHE_LOADINFO           0x92fu                                      

#define STACKSHOT_KCTYPE_TASK_DELTA_SNAPSHOT 0x940u                               
#define STACKSHOT_KCTYPE_THREAD_DELTA_SNAPSHOT 0x941u                               

struct stack_snapshot_frame32 {
	uint32_t lr;
	uint32_t sp;
};

struct stack_snapshot_frame64 {
	uint64_t lr;
	uint64_t sp;
};

struct dyld_uuid_info_32 {
	uint32_t imageLoadAddress;                                      
	uuid_t   imageUUID;
};

struct dyld_uuid_info_64 {
	uint64_t imageLoadAddress;                      
	uuid_t   imageUUID;
};

  
                                                                   
                                                                      
   
struct dyld_uuid_info_64_v2 {
	uint64_t imageLoadAddress;                      
	uuid_t   imageUUID;
	                                                             
	uint64_t imageSlidBaseAddress;                                                       
};

  
                                                                      
                                                                     
                                                                     
                                                              
  
                                   
                                          
                                         
                                                              
                                                     
   
struct dyld_shared_cache_loadinfo {
	uint64_t sharedCacheSlide;                             
	uuid_t   sharedCacheUUID;
	                                                             
	uint64_t sharedCacheUnreliableSlidBaseAddress;                                                                                 
	                                                             
	uint64_t sharedCacheSlidFirstMapping;                                         
};

struct dyld_aot_cache_uuid_info {
	uint64_t x86SlidBaseAddress;                                                     
	uuid_t x86UUID;                               
	uint64_t aotSlidBaseAddress;                                               
	uuid_t aotUUID;                               
};

struct user32_dyld_uuid_info {
	uint32_t        imageLoadAddress;                                              
	uuid_t                  imageUUID;                                         
};

struct user64_dyld_uuid_info {
	uint64_t        imageLoadAddress;                                              
	uuid_t                  imageUUID;                                         
};

#define DYLD_AOT_IMAGE_KEY_SIZE 32

struct user64_dyld_aot_info {
	uint64_t x86LoadAddress;
	uint64_t aotLoadAddress;
	uint64_t aotImageSize;
	uint8_t  aotImageKey[DYLD_AOT_IMAGE_KEY_SIZE];
};

enum task_snapshot_flags {
	                                                                                   
	kTaskRsrcFlagged                      = 0x4,                                    
	kTerminatedSnapshot                   = 0x8,
	kPidSuspended                         = 0x10,                           
	kFrozen                               = 0x20,                                                      
	kTaskDarwinBG                         = 0x40,
	kTaskExtDarwinBG                      = 0x80,
	kTaskVisVisible                       = 0x100,
	kTaskVisNonvisible                    = 0x200,
	kTaskIsForeground                     = 0x400,
	kTaskIsBoosted                        = 0x800,
	kTaskIsSuppressed                     = 0x1000,
	kTaskIsTimerThrottled                 = 0x2000,                 
	kTaskIsImpDonor                       = 0x4000,
	kTaskIsLiveImpDonor                   = 0x8000,
	kTaskIsDirty                          = 0x10000,
	kTaskWqExceededConstrainedThreadLimit = 0x20000,
	kTaskWqExceededTotalThreadLimit       = 0x40000,
	kTaskWqFlagsAvailable                 = 0x80000,
	kTaskUUIDInfoFaultedIn                = 0x100000,                                             
	kTaskUUIDInfoMissing                  = 0x200000,                                   
	kTaskUUIDInfoTriedFault               = 0x400000,                                  
	kTaskSharedRegionInfoUnavailable      = 0x800000,                                      
	kTaskTALEngaged                       = 0x1000000,
	                      
	kTaskIsDirtyTracked                   = 0x4000000,
	kTaskAllowIdleExit                    = 0x8000000,
	kTaskIsTranslated                     = 0x10000000,
	kTaskSharedRegionNone                 = 0x20000000,                                            
	kTaskSharedRegionSystem               = 0x40000000,                                                   
	kTaskSharedRegionOther                = 0x80000000,                                                        
};

enum thread_snapshot_flags {
	                                                                                   
	kHasDispatchSerial    = 0x4,
	kStacksPCOnly         = 0x8,                                              
	kThreadDarwinBG       = 0x10,                           
	kThreadIOPassive      = 0x20,                               
	kThreadSuspended      = 0x40,                            
	kThreadTruncatedBT    = 0x80,                                                  
	kGlobalForcedIdle     = 0x100,                                          
	kThreadFaultedBT      = 0x200,                                                             
	kThreadTriedFaultBT   = 0x400,                                                             
	kThreadOnCore         = 0x800,                                                           
	kThreadIdleWorker     = 0x1000,                                                 
	kThreadMain           = 0x2000,                                
};

struct mem_and_io_snapshot {
	uint32_t        snapshot_magic;
	uint32_t        free_pages;
	uint32_t        active_pages;
	uint32_t        inactive_pages;
	uint32_t        purgeable_pages;
	uint32_t        wired_pages;
	uint32_t        speculative_pages;
	uint32_t        throttled_pages;
	uint32_t        filebacked_pages;
	uint32_t        compressions;
	uint32_t        decompressions;
	uint32_t        compressor_size;
	int32_t         busy_buffer_count;
	uint32_t        pages_wanted;
	uint32_t        pages_reclaimed;
	uint8_t         pages_wanted_reclaimed_valid;                                         
} __attribute__((packed));

                                      
#define SS_TH_WAIT 0x01                               
#define SS_TH_SUSP 0x02                                         
#define SS_TH_RUN 0x04                                
#define SS_TH_UNINT 0x08                                  
#define SS_TH_TERMINATE 0x10                             
#define SS_TH_TERMINATE2 0x20                                 
#define SS_TH_IDLE 0x80                             

struct thread_snapshot_v2 {
	uint64_t  ths_thread_id;
	uint64_t  ths_wait_event;
	uint64_t  ths_continuation;
	uint64_t  ths_total_syscalls;
	uint64_t  ths_voucher_identifier;
	uint64_t  ths_dqserialnum;
	uint64_t  ths_user_time;
	uint64_t  ths_sys_time;
	uint64_t  ths_ss_flags;
	uint64_t  ths_last_run_time;
	uint64_t  ths_last_made_runnable_time;
	uint32_t  ths_state;
	uint32_t  ths_sched_flags;
	int16_t   ths_base_priority;
	int16_t   ths_sched_priority;
	uint8_t   ths_eqos;
	uint8_t ths_rqos;
	uint8_t ths_rqos_override;
	uint8_t ths_io_tier;
} __attribute__((packed));

struct thread_snapshot_v3 {
	uint64_t ths_thread_id;
	uint64_t ths_wait_event;
	uint64_t ths_continuation;
	uint64_t ths_total_syscalls;
	uint64_t ths_voucher_identifier;
	uint64_t ths_dqserialnum;
	uint64_t ths_user_time;
	uint64_t ths_sys_time;
	uint64_t ths_ss_flags;
	uint64_t ths_last_run_time;
	uint64_t ths_last_made_runnable_time;
	uint32_t ths_state;
	uint32_t ths_sched_flags;
	int16_t ths_base_priority;
	int16_t ths_sched_priority;
	uint8_t ths_eqos;
	uint8_t ths_rqos;
	uint8_t ths_rqos_override;
	uint8_t ths_io_tier;
	uint64_t ths_thread_t;
} __attribute__((packed));


struct thread_snapshot_v4 {
	uint64_t ths_thread_id;
	uint64_t ths_wait_event;
	uint64_t ths_continuation;
	uint64_t ths_total_syscalls;
	uint64_t ths_voucher_identifier;
	uint64_t ths_dqserialnum;
	uint64_t ths_user_time;
	uint64_t ths_sys_time;
	uint64_t ths_ss_flags;
	uint64_t ths_last_run_time;
	uint64_t ths_last_made_runnable_time;
	uint32_t ths_state;
	uint32_t ths_sched_flags;
	int16_t ths_base_priority;
	int16_t ths_sched_priority;
	uint8_t ths_eqos;
	uint8_t ths_rqos;
	uint8_t ths_rqos_override;
	uint8_t ths_io_tier;
	uint64_t ths_thread_t;
	uint64_t ths_requested_policy;
	uint64_t ths_effective_policy;
} __attribute__((packed));


struct thread_group_snapshot {
	uint64_t tgs_id;
	char tgs_name[16];
} __attribute__((packed));

enum thread_group_flags {
	kThreadGroupEfficient = 0x1,
	kThreadGroupUIApp = 0x2
};

struct thread_group_snapshot_v2 {
	uint64_t tgs_id;
	char tgs_name[16];
	uint64_t tgs_flags;
} __attribute__((packed));

enum coalition_flags {
	kCoalitionTermRequested = 0x1,
	kCoalitionTerminated    = 0x2,
	kCoalitionReaped        = 0x4,
	kCoalitionPrivileged    = 0x8,
};

struct jetsam_coalition_snapshot {
	uint64_t jcs_id;
	uint64_t jcs_flags;
	uint64_t jcs_thread_group;
	uint64_t jcs_leader_task_uniqueid;
} __attribute__((packed));

struct instrs_cycles_snapshot {
	uint64_t ics_instructions;
	uint64_t ics_cycles;
} __attribute__((packed));

struct thread_delta_snapshot_v2 {
	uint64_t  tds_thread_id;
	uint64_t  tds_voucher_identifier;
	uint64_t  tds_ss_flags;
	uint64_t  tds_last_made_runnable_time;
	uint32_t  tds_state;
	uint32_t  tds_sched_flags;
	int16_t   tds_base_priority;
	int16_t   tds_sched_priority;
	uint8_t   tds_eqos;
	uint8_t   tds_rqos;
	uint8_t   tds_rqos_override;
	uint8_t   tds_io_tier;
} __attribute__ ((packed));

struct thread_delta_snapshot_v3 {
	uint64_t  tds_thread_id;
	uint64_t  tds_voucher_identifier;
	uint64_t  tds_ss_flags;
	uint64_t  tds_last_made_runnable_time;
	uint32_t  tds_state;
	uint32_t  tds_sched_flags;
	int16_t   tds_base_priority;
	int16_t   tds_sched_priority;
	uint8_t   tds_eqos;
	uint8_t   tds_rqos;
	uint8_t   tds_rqos_override;
	uint8_t   tds_io_tier;
	uint64_t  tds_requested_policy;
	uint64_t  tds_effective_policy;
} __attribute__ ((packed));

struct io_stats_snapshot {
	  
                  
                                       
    
	uint64_t         ss_disk_reads_count;
	uint64_t         ss_disk_reads_size;
	uint64_t         ss_disk_writes_count;
	uint64_t         ss_disk_writes_size;
	uint64_t         ss_io_priority_count[STACKSHOT_IO_NUM_PRIORITIES];
	uint64_t         ss_io_priority_size[STACKSHOT_IO_NUM_PRIORITIES];
	uint64_t         ss_paging_count;
	uint64_t         ss_paging_size;
	uint64_t         ss_non_paging_count;
	uint64_t         ss_non_paging_size;
	uint64_t         ss_data_count;
	uint64_t         ss_data_size;
	uint64_t         ss_metadata_count;
	uint64_t         ss_metadata_size;
	                             
} __attribute__ ((packed));

struct task_snapshot_v2 {
	uint64_t  ts_unique_pid;
	uint64_t  ts_ss_flags;
	uint64_t  ts_user_time_in_terminated_threads;
	uint64_t  ts_system_time_in_terminated_threads;
	uint64_t  ts_p_start_sec;
	uint64_t  ts_task_size;
	uint64_t  ts_max_resident_size;
	uint32_t  ts_suspend_count;
	uint32_t  ts_faults;
	uint32_t  ts_pageins;
	uint32_t  ts_cow_faults;
	uint32_t  ts_was_throttled;
	uint32_t  ts_did_throttle;
	uint32_t  ts_latency_qos;
	int32_t   ts_pid;
	char      ts_p_comm[32];
} __attribute__ ((packed));

struct task_delta_snapshot_v2 {
	uint64_t  tds_unique_pid;
	uint64_t  tds_ss_flags;
	uint64_t  tds_user_time_in_terminated_threads;
	uint64_t  tds_system_time_in_terminated_threads;
	uint64_t  tds_task_size;
	uint64_t  tds_max_resident_size;
	uint32_t  tds_suspend_count;
	uint32_t  tds_faults;
	uint32_t  tds_pageins;
	uint32_t  tds_cow_faults;
	uint32_t  tds_was_throttled;
	uint32_t  tds_did_throttle;
	uint32_t  tds_latency_qos;
} __attribute__ ((packed));

struct stackshot_cpu_times {
	uint64_t user_usec;
	uint64_t system_usec;
} __attribute__((packed));

struct stackshot_cpu_times_v2 {
	uint64_t user_usec;
	uint64_t system_usec;
	uint64_t runnable_usec;
} __attribute__((packed));

struct stackshot_duration {
	uint64_t stackshot_duration;
	uint64_t stackshot_duration_outer;
} __attribute__((packed));

struct stackshot_duration_v2 {
	uint64_t stackshot_duration;
	uint64_t stackshot_duration_outer;
	uint64_t stackshot_duration_prior;
} __attribute__((packed));

struct stackshot_fault_stats {
	uint32_t sfs_pages_faulted_in;                                                           
	uint64_t sfs_time_spent_faulting;                             
	uint64_t sfs_system_max_fault_time;                                           
	uint8_t  sfs_stopped_faulting;                                                             
} __attribute__((packed));

typedef struct stackshot_thread_waitinfo {
	uint64_t owner;                                              
	uint64_t waiter;                                                     
	uint64_t context;                                                      
	uint8_t wait_type;                                                            
} __attribute__((packed)) thread_waitinfo_t;

typedef struct stackshot_thread_turnstileinfo {
	uint64_t waiter;                                                     
	uint64_t turnstile_context;                                                        
	uint8_t turnstile_priority;
	uint8_t number_of_hops;
#define STACKSHOT_TURNSTILE_STATUS_UNKNOWN         0x01                                              
#define STACKSHOT_TURNSTILE_STATUS_LOCKED_WAITQ    0x02                                       
#define STACKSHOT_TURNSTILE_STATUS_WORKQUEUE       0x04                                           
#define STACKSHOT_TURNSTILE_STATUS_THREAD          0x08                                        
#define STACKSHOT_TURNSTILE_STATUS_BLOCKED_ON_TASK 0x10                                            
#define STACKSHOT_TURNSTILE_STATUS_HELD_IPLOCK     0x20                             
	uint64_t turnstile_flags;
} __attribute__((packed)) thread_turnstileinfo_t;

#define STACKSHOT_WAITOWNER_KERNEL         (UINT64_MAX - 1)
#define STACKSHOT_WAITOWNER_PORT_LOCKED    (UINT64_MAX - 2)
#define STACKSHOT_WAITOWNER_PSET_LOCKED    (UINT64_MAX - 3)
#define STACKSHOT_WAITOWNER_INTRANSIT      (UINT64_MAX - 4)
#define STACKSHOT_WAITOWNER_MTXSPIN        (UINT64_MAX - 5)
#define STACKSHOT_WAITOWNER_THREQUESTED    (UINT64_MAX - 6)                                               
#define STACKSHOT_WAITOWNER_SUSPENDED      (UINT64_MAX - 7)                            

struct stackshot_cpu_architecture {
	int32_t cputype;
	int32_t cpusubtype;
} __attribute__((packed));

struct stack_snapshot_stacktop {
	uint64_t sp;
	uint8_t stack_contents[8];
};

                                                                    
struct stackshot_latency_collection {
	uint64_t latency_version;
	uint64_t setup_latency;
	uint64_t total_task_iteration_latency;
	uint64_t total_terminated_task_iteration_latency;
} __attribute__((packed));

                                                                    
struct stackshot_latency_task {
	uint64_t task_uniqueid;
	uint64_t setup_latency;
	uint64_t task_thread_count_loop_latency;
	uint64_t task_thread_data_loop_latency;
	uint64_t cur_tsnap_latency;
	uint64_t pmap_latency;
	uint64_t bsd_proc_ids_latency;
	uint64_t misc_latency;
	uint64_t misc2_latency;
	uint64_t end_latency;
} __attribute__((packed));

                                                                    
struct stackshot_latency_thread {
	uint64_t thread_id;
	uint64_t cur_thsnap1_latency;
	uint64_t dispatch_serial_latency;
	uint64_t dispatch_label_latency;
	uint64_t cur_thsnap2_latency;
	uint64_t thread_name_latency;
	uint64_t sur_times_latency;
	uint64_t user_stack_latency;
	uint64_t kernel_stack_latency;
	uint64_t misc_latency;
} __attribute__((packed));


                                                                  

  
                                                                       
                             
   

                                                                                       

struct crashinfo_proc_uniqidentifierinfo {
	uint8_t                 p_uuid[16];                                              
	uint64_t                p_uniqueid;                                                       
	uint64_t                p_puniqueid;                                                        
	uint64_t                p_reserve2;                                          
	uint64_t                p_reserve3;                                          
	uint64_t                p_reserve4;                                          
} __attribute__((packed));

#define TASK_CRASHINFO_BEGIN                KCDATA_BUFFER_BEGIN_CRASHINFO
#define TASK_CRASHINFO_STRING_DESC          KCDATA_TYPE_STRING_DESC
#define TASK_CRASHINFO_UINT32_DESC          KCDATA_TYPE_UINT32_DESC
#define TASK_CRASHINFO_UINT64_DESC          KCDATA_TYPE_UINT64_DESC

#define TASK_CRASHINFO_EXTMODINFO           0x801
#define TASK_CRASHINFO_BSDINFOWITHUNIQID    0x802                                               
#define TASK_CRASHINFO_TASKDYLD_INFO        0x803
#define TASK_CRASHINFO_UUID                 0x804
#define TASK_CRASHINFO_PID                  0x805
#define TASK_CRASHINFO_PPID                 0x806
#define TASK_CRASHINFO_RUSAGE               0x807                                         
                                                                                                                                  
#define TASK_CRASHINFO_RUSAGE_INFO          0x808                                             
#define TASK_CRASHINFO_PROC_NAME            0x809              
#define TASK_CRASHINFO_PROC_STARTTIME       0x80B                        
#define TASK_CRASHINFO_USERSTACK            0x80C                
#define TASK_CRASHINFO_ARGSLEN              0x80D
#define TASK_CRASHINFO_EXCEPTION_CODES      0x80E                             
#define TASK_CRASHINFO_PROC_PATH            0x80F                                
#define TASK_CRASHINFO_PROC_CSFLAGS         0x810                
#define TASK_CRASHINFO_PROC_STATUS          0x811            
#define TASK_CRASHINFO_UID                  0x812             
#define TASK_CRASHINFO_GID                  0x813             
#define TASK_CRASHINFO_PROC_ARGC            0x814           
#define TASK_CRASHINFO_PROC_FLAGS           0x815                    
#define TASK_CRASHINFO_CPUTYPE              0x816                  
#define TASK_CRASHINFO_WORKQUEUEINFO        0x817                                 
#define TASK_CRASHINFO_RESPONSIBLE_PID      0x818             
#define TASK_CRASHINFO_DIRTY_FLAGS          0x819           
#define TASK_CRASHINFO_CRASHED_THREADID     0x81A                
#define TASK_CRASHINFO_COALITION_ID         0x81B                
#define TASK_CRASHINFO_UDATA_PTRS           0x81C                
#define TASK_CRASHINFO_MEMORY_LIMIT         0x81D                

#define TASK_CRASHINFO_LEDGER_INTERNAL                          0x81E               
#define TASK_CRASHINFO_LEDGER_INTERNAL_COMPRESSED               0x81F               
#define TASK_CRASHINFO_LEDGER_IOKIT_MAPPED                      0x820               
#define TASK_CRASHINFO_LEDGER_ALTERNATE_ACCOUNTING              0x821               
#define TASK_CRASHINFO_LEDGER_ALTERNATE_ACCOUNTING_COMPRESSED   0x822               
#define TASK_CRASHINFO_LEDGER_PURGEABLE_NONVOLATILE             0x823               
#define TASK_CRASHINFO_LEDGER_PURGEABLE_NONVOLATILE_COMPRESSED  0x824               
#define TASK_CRASHINFO_LEDGER_PAGE_TABLE                        0x825               
#define TASK_CRASHINFO_LEDGER_PHYS_FOOTPRINT                    0x826               
#define TASK_CRASHINFO_LEDGER_PHYS_FOOTPRINT_LIFETIME_MAX       0x827               
#define TASK_CRASHINFO_LEDGER_NETWORK_NONVOLATILE               0x828               
#define TASK_CRASHINFO_LEDGER_NETWORK_NONVOLATILE_COMPRESSED    0x829               
#define TASK_CRASHINFO_LEDGER_WIRED_MEM                         0x82A               
#define TASK_CRASHINFO_PROC_PERSONA_ID                          0x82B            
#define TASK_CRASHINFO_MEMORY_LIMIT_INCREASE                    0x82C               
#define TASK_CRASHINFO_LEDGER_TAGGED_FOOTPRINT                  0x82D               
#define TASK_CRASHINFO_LEDGER_TAGGED_FOOTPRINT_COMPRESSED       0x82E               
#define TASK_CRASHINFO_LEDGER_MEDIA_FOOTPRINT                   0x82F               
#define TASK_CRASHINFO_LEDGER_MEDIA_FOOTPRINT_COMPRESSED        0x830               
#define TASK_CRASHINFO_LEDGER_GRAPHICS_FOOTPRINT                0x831               
#define TASK_CRASHINFO_LEDGER_GRAPHICS_FOOTPRINT_COMPRESSED     0x832               
#define TASK_CRASHINFO_LEDGER_NEURAL_FOOTPRINT                  0x833               
#define TASK_CRASHINFO_LEDGER_NEURAL_FOOTPRINT_COMPRESSED       0x834               
#define TASK_CRASHINFO_MEMORYSTATUS_EFFECTIVE_PRIORITY          0x835              

#define TASK_CRASHINFO_END                  KCDATA_TYPE_BUFFER_END

                                                                   

#define EXIT_REASON_SNAPSHOT            0x1001
#define EXIT_REASON_USER_DESC           0x1002                                   
#define EXIT_REASON_USER_PAYLOAD        0x1003                        
#define EXIT_REASON_CODESIGNING_INFO    0x1004
#define EXIT_REASON_WORKLOOP_ID         0x1005
#define EXIT_REASON_DISPATCH_QUEUE_NO   0x1006

struct exit_reason_snapshot {
	uint32_t ers_namespace;
	uint64_t ers_code;
	                                                                
	uint64_t ers_flags;
} __attribute__((packed));

#define EXIT_REASON_CODESIG_PATH_MAX    1024

struct codesigning_exit_reason_info {
	uint64_t  ceri_virt_addr;
	uint64_t  ceri_file_offset;
	char      ceri_pathname[EXIT_REASON_CODESIG_PATH_MAX];
	char      ceri_filename[EXIT_REASON_CODESIG_PATH_MAX];
	uint64_t  ceri_codesig_modtime_secs;
	uint64_t  ceri_codesig_modtime_nsecs;
	uint64_t  ceri_page_modtime_secs;
	uint64_t  ceri_page_modtime_nsecs;
	uint8_t   ceri_path_truncated;
	uint8_t   ceri_object_codesigned;
	uint8_t   ceri_page_codesig_validated;
	uint8_t   ceri_page_codesig_tainted;
	uint8_t   ceri_page_codesig_nx;
	uint8_t   ceri_page_wpmapped;
	uint8_t   ceri_page_slid;
	uint8_t   ceri_page_dirty;
	uint32_t  ceri_page_shadow_depth;
} __attribute__((packed));

#define EXIT_REASON_USER_DESC_MAX_LEN   1024
#define EXIT_REASON_PAYLOAD_MAX_LEN     2048
                                                       

typedef struct kcdata_iter {
	kcdata_item_t item;
	void *end;
} kcdata_iter_t;


static inline
kcdata_iter_t
kcdata_iter(void *buffer, unsigned long size)
{
	kcdata_iter_t iter;
	iter.item = (kcdata_item_t) buffer;
	iter.end = (void*) (((uintptr_t)buffer) + size);
	return iter;
}

static inline
kcdata_iter_t kcdata_iter_unsafe(void *buffer) __attribute__((deprecated));

static inline
kcdata_iter_t
kcdata_iter_unsafe(void *buffer)
{
	kcdata_iter_t iter;
	iter.item = (kcdata_item_t) buffer;
	iter.end = (void*) (uintptr_t) ~0;
	return iter;
}

static const kcdata_iter_t kcdata_invalid_iter = { .item = NULL, .end = NULL };

static inline
int
kcdata_iter_valid(kcdata_iter_t iter)
{
	return
	        ((uintptr_t)iter.item + sizeof(struct kcdata_item) <= (uintptr_t)iter.end) &&
	        ((uintptr_t)iter.item + sizeof(struct kcdata_item) + iter.item->size <= (uintptr_t)iter.end);
}


static inline
kcdata_iter_t
kcdata_iter_next(kcdata_iter_t iter)
{
	iter.item = (kcdata_item_t) (((uintptr_t)iter.item) + sizeof(struct kcdata_item) + (iter.item->size));
	return iter;
}

static inline uint32_t
kcdata_iter_type(kcdata_iter_t iter)
{
	if ((iter.item->type & ~0xfu) == KCDATA_TYPE_ARRAY_PAD0) {
		return KCDATA_TYPE_ARRAY;
	} else {
		return iter.item->type;
	}
}

static inline uint32_t
kcdata_calc_padding(uint32_t size)
{
	                                                                               
	return (-size) & 0xf;
}

static inline uint32_t
kcdata_flags_get_padding(uint64_t flags)
{
	return flags & KCDATA_FLAGS_STRUCT_PADDING_MASK;
}

                                         
static inline int
kcdata_iter_is_legacy_item(kcdata_iter_t iter, uint32_t legacy_size)
{
	uint32_t legacy_size_padded = legacy_size + kcdata_calc_padding(legacy_size);
	return iter.item->size == legacy_size_padded &&
	       (iter.item->flags & (KCDATA_FLAGS_STRUCT_PADDING_MASK | KCDATA_FLAGS_STRUCT_HAS_PADDING)) == 0;
}

static inline uint32_t
kcdata_iter_size(kcdata_iter_t iter)
{
	uint32_t legacy_size = 0;

	switch (kcdata_iter_type(iter)) {
	case KCDATA_TYPE_ARRAY:
	case KCDATA_TYPE_CONTAINER_BEGIN:
		return iter.item->size;
	case STACKSHOT_KCTYPE_THREAD_SNAPSHOT: {
		legacy_size = sizeof(struct thread_snapshot_v2);
		if (kcdata_iter_is_legacy_item(iter, legacy_size)) {
			return legacy_size;
		}

		goto not_legacy;
	}
	case STACKSHOT_KCTYPE_SHAREDCACHE_LOADINFO: {
		legacy_size = sizeof(struct dyld_uuid_info_64);
		if (kcdata_iter_is_legacy_item(iter, legacy_size)) {
			return legacy_size;
		}

		goto not_legacy;
	}
not_legacy:
	default:
		if (iter.item->size < kcdata_flags_get_padding(iter.item->flags)) {
			return 0;
		} else {
			return iter.item->size - kcdata_flags_get_padding(iter.item->flags);
		}
	}
}

static inline uint64_t
kcdata_iter_flags(kcdata_iter_t iter)
{
	return iter.item->flags;
}

static inline
void *
kcdata_iter_payload(kcdata_iter_t iter)
{
	return &iter.item->data;
}


static inline
uint32_t
kcdata_iter_array_elem_type(kcdata_iter_t iter)
{
	return (iter.item->flags >> 32) & UINT32_MAX;
}

static inline
uint32_t
kcdata_iter_array_elem_count(kcdata_iter_t iter)
{
	return (iter.item->flags) & UINT32_MAX;
}

                                                                                
                                                                                
                                                                               
                                                                                
                                                                            
                                                                             
                                            

static inline
uint32_t
kcdata_iter_array_size_switch(kcdata_iter_t iter)
{
	switch (kcdata_iter_array_elem_type(iter)) {
	case KCDATA_TYPE_LIBRARY_LOADINFO:
		return sizeof(struct dyld_uuid_info_32);
	case KCDATA_TYPE_LIBRARY_LOADINFO64:
		return sizeof(struct dyld_uuid_info_64);
	case STACKSHOT_KCTYPE_KERN_STACKFRAME:
	case STACKSHOT_KCTYPE_USER_STACKFRAME:
		return sizeof(struct stack_snapshot_frame32);
	case STACKSHOT_KCTYPE_KERN_STACKFRAME64:
	case STACKSHOT_KCTYPE_USER_STACKFRAME64:
		return sizeof(struct stack_snapshot_frame64);
	case STACKSHOT_KCTYPE_DONATING_PIDS:
		return sizeof(int32_t);
	case STACKSHOT_KCTYPE_THREAD_DELTA_SNAPSHOT:
		return sizeof(struct thread_delta_snapshot_v2);
	                                                                         
	          
	case TASK_CRASHINFO_CRASHED_THREADID:
		return sizeof(uint64_t);
	default:
		return 0;
	}
}

static inline
int
kcdata_iter_array_valid(kcdata_iter_t iter)
{
	if (!kcdata_iter_valid(iter)) {
		return 0;
	}
	if (kcdata_iter_type(iter) != KCDATA_TYPE_ARRAY) {
		return 0;
	}
	if (kcdata_iter_array_elem_count(iter) == 0) {
		return iter.item->size == 0;
	}
	if (iter.item->type == KCDATA_TYPE_ARRAY) {
		uint32_t elem_size = kcdata_iter_array_size_switch(iter);
		if (elem_size == 0) {
			return 0;
		}
		                                          
		return
		        kcdata_iter_array_elem_count(iter) <= iter.item->size / elem_size &&
		        iter.item->size % kcdata_iter_array_elem_count(iter) < 16;
	} else {
		return
		        (iter.item->type & 0xf) <= iter.item->size &&
		        kcdata_iter_array_elem_count(iter) <= iter.item->size - (iter.item->type & 0xf) &&
		        (iter.item->size - (iter.item->type & 0xf)) % kcdata_iter_array_elem_count(iter) == 0;
	}
}


static inline
uint32_t
kcdata_iter_array_elem_size(kcdata_iter_t iter)
{
	if (iter.item->type == KCDATA_TYPE_ARRAY) {
		return kcdata_iter_array_size_switch(iter);
	}
	if (kcdata_iter_array_elem_count(iter) == 0) {
		return 0;
	}
	return (iter.item->size - (iter.item->type & 0xf)) / kcdata_iter_array_elem_count(iter);
}

static inline
int
kcdata_iter_container_valid(kcdata_iter_t iter)
{
	return
	        kcdata_iter_valid(iter) &&
	        kcdata_iter_type(iter) == KCDATA_TYPE_CONTAINER_BEGIN &&
	        iter.item->size >= sizeof(uint32_t);
}

static inline
uint32_t
kcdata_iter_container_type(kcdata_iter_t iter)
{
	return *(uint32_t *) kcdata_iter_payload(iter);
}

static inline
uint64_t
kcdata_iter_container_id(kcdata_iter_t iter)
{
	return iter.item->flags;
}


#define KCDATA_ITER_FOREACH(iter) for(; kcdata_iter_valid(iter) && iter.item->type != KCDATA_TYPE_BUFFER_END; iter = kcdata_iter_next(iter))
#define KCDATA_ITER_FOREACH_FAILED(iter) (!kcdata_iter_valid(iter) || (iter).item->type != KCDATA_TYPE_BUFFER_END)

static inline
kcdata_iter_t
kcdata_iter_find_type(kcdata_iter_t iter, uint32_t type)
{
	KCDATA_ITER_FOREACH(iter)
	{
		if (kcdata_iter_type(iter) == type) {
			return iter;
		}
	}
	return kcdata_invalid_iter;
}

static inline
int
kcdata_iter_data_with_desc_valid(kcdata_iter_t iter, uint32_t minsize)
{
	return
	        kcdata_iter_valid(iter) &&
	        kcdata_iter_size(iter) >= KCDATA_DESC_MAXLEN + minsize &&
	        ((char*)kcdata_iter_payload(iter))[KCDATA_DESC_MAXLEN - 1] == 0;
}

static inline
char *
kcdata_iter_string(kcdata_iter_t iter, uint32_t offset)
{
	if (offset > kcdata_iter_size(iter)) {
		return NULL;
	}
	uint32_t maxlen = kcdata_iter_size(iter) - offset;
	char *s = ((char*)kcdata_iter_payload(iter)) + offset;
	if (strnlen(s, maxlen) < maxlen) {
		return s;
	} else {
		return NULL;
	}
}

static inline void
kcdata_iter_get_data_with_desc(kcdata_iter_t iter, char **desc_ptr, void **data_ptr, uint32_t *size_ptr)
{
	if (desc_ptr) {
		*desc_ptr = (char *)kcdata_iter_payload(iter);
	}
	if (data_ptr) {
		*data_ptr = (void *)((uintptr_t)kcdata_iter_payload(iter) + KCDATA_DESC_MAXLEN);
	}
	if (size_ptr) {
		*size_ptr = kcdata_iter_size(iter) - KCDATA_DESC_MAXLEN;
	}
}

#endif
