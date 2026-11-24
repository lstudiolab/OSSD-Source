/*
 * Copyright (c) 2000-2005 Apple Computer, Inc. All rights reserved.
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

#ifndef _MACH_TASK_POLICY_H_
#define _MACH_TASK_POLICY_H_

#include <mach/mach_types.h>

  
                                                          
                        
  
                                                          
                     
                                                             
                                                            
                                                             
                                                           
                                                           
                                                       
                                                             
                                                              
                                                   
   

typedef natural_t       task_policy_flavor_t;
typedef integer_t       *task_policy_t;

  
                                  
                                                         
                                                                     
                                                                     
                                                                       
  
                                  
                                                         
                                                                     
                                                                     
                                                                       
                                                                    
   

  
                   
   
  
                        
  
                                                         
                             
  
              
  
                               
  
                                                         
                             
  
                                                          
                                                         
                        
                                                              
  
                                                          
                                                         
                        
  
                                                            
                                                                 
                                                              
                          
  
                                                          
                                                             
                                                        
   

#define TASK_CATEGORY_POLICY            1

#define TASK_SUPPRESSION_POLICY         3
#define TASK_POLICY_STATE               4
#define TASK_BASE_QOS_POLICY            8
#define TASK_OVERRIDE_QOS_POLICY        9
#define TASK_BASE_LATENCY_QOS_POLICY    10
#define TASK_BASE_THROUGHPUT_QOS_POLICY 11

typedef enum task_role {
	TASK_RENICED                    = -1,
	TASK_UNSPECIFIED                = 0,
	TASK_FOREGROUND_APPLICATION     = 1,
	TASK_BACKGROUND_APPLICATION     = 2,
	TASK_CONTROL_APPLICATION        = 3,
	TASK_GRAPHICS_SERVER            = 4,
	TASK_THROTTLE_APPLICATION       = 5,
	TASK_NONUI_APPLICATION          = 6,
	TASK_DEFAULT_APPLICATION        = 7,
	TASK_DARWINBG_APPLICATION       = 8,
} task_role_t;

struct task_category_policy {
	task_role_t             role;
};

typedef struct task_category_policy             task_category_policy_data_t;
typedef struct task_category_policy             *task_category_policy_t;

#define TASK_CATEGORY_POLICY_COUNT      ((mach_msg_type_number_t) \
	(sizeof (task_category_policy_data_t) / sizeof (integer_t)))


enum task_latency_qos {
	LATENCY_QOS_TIER_UNSPECIFIED = 0x0,
	LATENCY_QOS_TIER_0 = ((0xFF << 16) | 1),
	LATENCY_QOS_TIER_1 = ((0xFF << 16) | 2),
	LATENCY_QOS_TIER_2 = ((0xFF << 16) | 3),
	LATENCY_QOS_TIER_3 = ((0xFF << 16) | 4),
	LATENCY_QOS_TIER_4 = ((0xFF << 16) | 5),
	LATENCY_QOS_TIER_5 = ((0xFF << 16) | 6)
};
typedef integer_t       task_latency_qos_t;
enum task_throughput_qos {
	THROUGHPUT_QOS_TIER_UNSPECIFIED = 0x0,
	THROUGHPUT_QOS_TIER_0 = ((0xFE << 16) | 1),
	THROUGHPUT_QOS_TIER_1 = ((0xFE << 16) | 2),
	THROUGHPUT_QOS_TIER_2 = ((0xFE << 16) | 3),
	THROUGHPUT_QOS_TIER_3 = ((0xFE << 16) | 4),
	THROUGHPUT_QOS_TIER_4 = ((0xFE << 16) | 5),
	THROUGHPUT_QOS_TIER_5 = ((0xFE << 16) | 6),
};

#define LATENCY_QOS_LAUNCH_DEFAULT_TIER LATENCY_QOS_TIER_3
#define THROUGHPUT_QOS_LAUNCH_DEFAULT_TIER THROUGHPUT_QOS_TIER_3

typedef integer_t       task_throughput_qos_t;

struct task_qos_policy {
	task_latency_qos_t task_latency_qos_tier;
	task_throughput_qos_t task_throughput_qos_tier;
};

typedef struct task_qos_policy *task_qos_policy_t;
#define TASK_QOS_POLICY_COUNT   ((mach_msg_type_number_t) \
	(sizeof (struct task_qos_policy) / sizeof (integer_t)))

                                                          
#define PROC_FLAG_DARWINBG           0x8000                                      
#define PROC_FLAG_EXT_DARWINBG           0x10000                                                            
#define PROC_FLAG_IOS_APPLEDAEMON    0x20000                                    
#define PROC_FLAG_IOS_IMPPROMOTION   0x80000                                    
#define PROC_FLAG_ADAPTIVE           0x100000                           
#define PROC_FLAG_ADAPTIVE_IMPORTANT 0x200000                                                       
#define PROC_FLAG_IMPORTANCE_DONOR   0x400000                                                
#define PROC_FLAG_SUPPRESSED         0x800000                             
#define PROC_FLAG_APPLICATION        0x1000000                                
#define PROC_FLAG_IOS_APPLICATION PROC_FLAG_APPLICATION                                

#ifdef PRIVATE

  
                                                            
                                                                  
  
                                                        
                                                                    
   

#define TASK_POLICY_INTERNAL_STRUCT_VERSION 4

#define trp_tal_enabled trp_reserved                                                                   

struct task_requested_policy {
	uint64_t        trp_int_darwinbg        :1,                                             
	    trp_ext_darwinbg        :1,
	    trp_int_iotier          :2,                                       
	    trp_ext_iotier          :2,
	    trp_int_iopassive       :1,                                                                   
	    trp_ext_iopassive       :1,
	    trp_bg_iotier           :2,                                                                                                      
	    trp_terminated          :1,                                                                                         
	    trp_base_latency_qos    :3,                                        
	    trp_base_through_qos    :3,                                                 

	    trp_apptype             :3,                                                                            
	    trp_boosted             :1,                                                                
	    trp_role                :5,                                         
	    trp_over_latency_qos    :3,                                                 
	    trp_over_through_qos    :3,                                                          
	    trp_sfi_managed         :1,                                       
	    trp_qos_clamp           :3,                                     

	                                              
	    trp_sup_active          :1,                                        
	    trp_sup_lowpri_cpu      :1,                                                               
	    trp_sup_timer           :3,                                                       
	    trp_sup_disk            :1,                                            
	    trp_sup_throughput      :3,                                                
	    trp_sup_cpu             :1,                                                                        
	    trp_sup_bg_sockets      :1,                                               

	    trp_reserved            :17;
};

struct task_effective_policy {
	uint64_t        tep_darwinbg            :1,                                                                         
	    tep_lowpri_cpu          :1,                                                      
	    tep_io_tier             :2,                                              
	    tep_io_passive          :1,                                                                   
	    tep_all_sockets_bg      :1,                                                                                                       
	    tep_new_sockets_bg      :1,                                                                   
	    tep_bg_iotier           :2,                                                                              
	    tep_terminated          :1,                                                                                         
	    tep_qos_ui_is_urgent    :1,                                                                               
	    tep_latency_qos         :3,                                              
	    tep_through_qos         :3,                                                       

	    tep_tal_engaged         :1,                                            
	    tep_watchers_bg         :1,                                         
	    tep_sup_active          :1,                                                          
	    tep_role                :4,                                         
	    tep_suppressed_cpu      :1,                                                                                
	    tep_sfi_managed         :1,                                       
	    tep_live_donor          :1,                                                            
	    tep_qos_clamp           :3,                                                                           
	    tep_qos_ceiling         :3,                                                                                   
	    tep_adaptive_bg         :1,                                                                         

	    tep_reserved            :30;
};

#endif              

#ifdef MACH_KERNEL_PRIVATE

extern const struct task_requested_policy default_task_requested_policy;
extern const struct task_effective_policy default_task_effective_policy;

extern kern_return_t
    qos_latency_policy_validate(task_latency_qos_t);

extern kern_return_t
    qos_throughput_policy_validate(task_throughput_qos_t);

extern uint32_t
    qos_extract(uint32_t);

extern uint32_t
    qos_latency_policy_package(uint32_t);
extern uint32_t
    qos_throughput_policy_package(uint32_t);

#endif                          

#ifdef PRIVATE

  
                                                                
   
struct task_suppression_policy {
	integer_t active;
	integer_t lowpri_cpu;                                    
	integer_t timer_throttle;
	integer_t disk_throttle;
	integer_t cpu_limit;
	integer_t suspend;
	integer_t throughput_qos;
	integer_t suppressed_cpu;                                     
	integer_t background_sockets;
	integer_t reserved[7];
};

typedef struct task_suppression_policy *task_suppression_policy_t;

#define TASK_SUPPRESSION_POLICY_COUNT   ((mach_msg_type_number_t) \
	(sizeof (struct task_suppression_policy) / sizeof (integer_t)))

struct task_policy_state {
	uint64_t requested;
	uint64_t effective;
	uint64_t pending;
	uint32_t imp_assertcnt;
	uint32_t imp_externcnt;
	uint64_t flags;
	uint64_t imp_transitions;
	uint64_t tps_requested_policy;
	uint64_t tps_effective_policy;
};

typedef struct task_policy_state *task_policy_state_t;

#define TASK_POLICY_STATE_COUNT ((mach_msg_type_number_t) \
	(sizeof (struct task_policy_state) / sizeof (integer_t)))


  
                                                        
                                               
  
                                                                   
                                                                                    
  
                                                                                   
  
                                                                     
   

#define TASK_APPTYPE_NONE                0
#define TASK_APPTYPE_DAEMON_INTERACTIVE  1
#define TASK_APPTYPE_DAEMON_STANDARD     2
#define TASK_APPTYPE_DAEMON_ADAPTIVE     3
#define TASK_APPTYPE_DAEMON_BACKGROUND   4
#define TASK_APPTYPE_APP_DEFAULT         5
#define TASK_APPTYPE_APP_TAL             6             
#define TASK_APPTYPE_DRIVER              7

                             
#define TASK_IMP_RECEIVER                    0x00000001
#define TASK_IMP_DONOR                       0x00000002
#define TASK_IMP_LIVE_DONOR                  0x00000004
#define TASK_DENAP_RECEIVER                  0x00000008
#define TASK_IS_PIDSUSPENDED                 0x00000010

                      
#define POLICY_REQ_INT_DARWIN_BG             0x00000001
#define POLICY_REQ_EXT_DARWIN_BG             0x00000002
#define POLICY_REQ_INT_IO_TIER_MASK          0x0000000c             
#define POLICY_REQ_INT_IO_TIER_SHIFT         2
#define POLICY_REQ_EXT_IO_TIER_MASK          0x00000030             
#define POLICY_REQ_EXT_IO_TIER_SHIFT         4
#define POLICY_REQ_INT_PASSIVE_IO            0x00000040
#define POLICY_REQ_EXT_PASSIVE_IO            0x00000080
#define POLICY_REQ_BG_IOTIER_MASK            0x00000300             
#define POLICY_REQ_BG_IOTIER_SHIFT           8

                             
#define POLICY_REQ_PIDBIND_BG                0x00000400
#define POLICY_REQ_WORKQ_BG                  0x00000800                 
#define POLICY_REQ_TH_QOS_MASK               0x07000000                                  
#define POLICY_REQ_TH_QOS_SHIFT              24
#define POLICY_REQ_TH_QOS_OVER_MASK          0x70000000                                         
#define POLICY_REQ_TH_QOS_OVER_SHIFT         28

                           
#define POLICY_REQ_TERMINATED                0x00001000
#define POLICY_REQ_BOOSTED                   0x00002000
#define POLICY_REQ_INT_GPU_DENY              0x00004000
#define POLICY_REQ_EXT_GPU_DENY              0x00008000
#define POLICY_REQ_APPTYPE_MASK              0x00070000             
#define POLICY_REQ_APPTYPE_SHIFT             16
#define POLICY_REQ_BASE_LATENCY_QOS_MASK     0x00700000             
#define POLICY_REQ_BASE_LATENCY_QOS_SHIFT    20
#define POLICY_REQ_ROLE_MASK                 0x07000000             
#define POLICY_REQ_ROLE_SHIFT                24
#define POLICY_REQ_TAL_ENABLED               0x40000000
#define POLICY_REQ_SFI_MANAGED               0x80000000

                                                                               
#define POLICY_REQ_SUP_ACTIVE                0x0000000100000000
#define POLICY_REQ_SUP_LOWPRI_CPU            0x0000000200000000
#define POLICY_REQ_SUP_CPU                   0x0000000400000000
#define POLICY_REQ_SUP_DISK_THROTTLE         0x0000003000000000             
#define POLICY_REQ_SUP_CPU_LIMIT             0x0000004000000000
#define POLICY_REQ_SUP_SUSPEND               0x0000008000000000
#define POLICY_REQ_OVER_LATENCY_QOS_MASK     0x0000070000000000             
#define POLICY_REQ_OVER_LATENCY_QOS_SHIFT    40
#define POLICY_REQ_BASE_THROUGH_QOS_MASK     0x0000700000000000             
#define POLICY_REQ_BASE_THROUGH_QOS_SHIFT    44
#define POLICY_REQ_OVER_THROUGH_QOS_MASK     0x0007000000000000             
#define POLICY_REQ_OVER_THROUGH_QOS_SHIFT    48
#define POLICY_REQ_SUP_TIMER_THROTTLE_MASK   0x0070000000000000             
#define POLICY_REQ_SUP_TIMER_THROTTLE_SHIFT  52
#define POLICY_REQ_SUP_THROUGHPUT_MASK       0x0700000000000000             
#define POLICY_REQ_SUP_THROUGHPUT_SHIFT      56
#define POLICY_REQ_SUP_BG_SOCKETS            0x0800008000000000
#define POLICY_REQ_QOS_CLAMP_MASK            0x7000000000000000             
#define POLICY_REQ_QOS_CLAMP_SHIFT           60

                      
#define POLICY_EFF_IO_TIER_MASK              0x00000003             
#define POLICY_EFF_IO_TIER_SHIFT             0
#define POLICY_EFF_IO_PASSIVE                0x00000008
#define POLICY_EFF_DARWIN_BG                 0x00000010
#define POLICY_EFF_LOWPRI_CPU                0x00000020
#define POLICY_EFF_ALL_SOCKETS_BG            0x00000040
#define POLICY_EFF_NEW_SOCKETS_BG            0x00000080
#define POLICY_EFF_BG_IOTIER_MASK            0x00000300             
#define POLICY_EFF_BG_IOTIER_SHIFT           8
#define POLICY_EFF_TERMINATED                0x00000400
#define POLICY_EFF_QOS_UI_IS_URGENT          0x80000000

                             
#define POLICY_EFF_TH_QOS_MASK               0x00700000                                  
#define POLICY_EFF_TH_QOS_SHIFT              20
#define POLICY_EFF_LATENCY_QOS_MASK          0x00070000             
#define POLICY_EFF_LATENCY_QOS_SHIFT         16
#define POLICY_EFF_THROUGH_QOS_MASK          0x07000000             
#define POLICY_EFF_THROUGH_QOS_SHIFT         24

                           
#define POLICY_EFF_GPU_DENY                  0x00001000
#define POLICY_EFF_TAL_ENGAGED               0x00002000
#define POLICY_EFF_SUSPENDED                 0x00004000
#define POLICY_EFF_WATCHERS_BG               0x00008000
#define POLICY_EFF_SUP_ACTIVE                0x00080000
#define POLICY_EFF_ROLE_MASK                 0x00700000             
#define POLICY_EFF_ROLE_SHIFT                20
#define POLICY_EFF_SUP_CPU                   0x00800000
#define POLICY_EFF_SFI_MANAGED               0x08000000
#define POLICY_EFF_QOS_CEILING_MASK          0x70000000
#define POLICY_EFF_QOS_CEILING_SHIFT         28

                    
#define POLICY_PEND_UPDATING                 0x00000001
#define POLICY_PEND_SOCKETS                  0x00000002
#define POLICY_PEND_TIMERS                   0x00000004
#define POLICY_PEND_WATCHERS                 0x00000008

#endif              

#endif                            
