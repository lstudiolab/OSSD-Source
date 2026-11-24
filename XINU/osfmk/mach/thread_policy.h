/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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

#ifndef _MACH_THREAD_POLICY_H_
#define _MACH_THREAD_POLICY_H_

#include <mach/mach_types.h>

  
                                                          
                          
  
                                                          
                     
                                                             
                                                            
                                                             
                                                           
                                                           
                                                       
                                                             
                                                              
                                                   
   

typedef natural_t       thread_policy_flavor_t;
typedef integer_t       *thread_policy_t;

  
                                    
                                                             
                                                                        
                                                                        
                                                                        
  
                                    
                                                             
                                                                        
                                                                        
                                                                        
                                                                     
   

  
                   
   
  
                          
  
                                                               
                                                                
                                                             
                
  
              
         
   

#define THREAD_STANDARD_POLICY                  1

struct thread_standard_policy {
	natural_t               no_data;
};

typedef struct thread_standard_policy   thread_standard_policy_data_t;
typedef struct thread_standard_policy   *thread_standard_policy_t;

#define THREAD_STANDARD_POLICY_COUNT    0

  
                          
  
                                                            
                                                              
  
              
  
                                                                
                                      
   

#define THREAD_EXTENDED_POLICY                  1

struct thread_extended_policy {
	boolean_t               timeshare;
};

typedef struct thread_extended_policy   thread_extended_policy_data_t;
typedef struct thread_extended_policy   *thread_extended_policy_t;

#define THREAD_EXTENDED_POLICY_COUNT    ((mach_msg_type_number_t) \
	(sizeof (thread_extended_policy_data_t) / sizeof (integer_t)))

  
                                 
  
                                                           
                                  
  
              
  
                                                              
                                                            
                                                                
                   
  
                                                         
                                                              
                          
  
                                                           
                                                             
                                                               
                                                               
                                                  
  
                                                          
                                                          
   

#define THREAD_TIME_CONSTRAINT_POLICY   2

struct thread_time_constraint_policy {
	uint32_t                period;
	uint32_t                computation;
	uint32_t                constraint;
	boolean_t               preemptible;
};

typedef struct thread_time_constraint_policy    \
        thread_time_constraint_policy_data_t;
typedef struct thread_time_constraint_policy    \
        *thread_time_constraint_policy_t;

#define THREAD_TIME_CONSTRAINT_POLICY_COUNT     ((mach_msg_type_number_t) \
	(sizeof (thread_time_constraint_policy_data_t) / sizeof (integer_t)))

  
                            
  
                                                         
                                                         
  
              
  
                                                             
   

#define THREAD_PRECEDENCE_POLICY                3

struct thread_precedence_policy {
	integer_t               importance;
};

typedef struct thread_precedence_policy         thread_precedence_policy_data_t;
typedef struct thread_precedence_policy         *thread_precedence_policy_t;

#define THREAD_PRECEDENCE_POLICY_COUNT  ((mach_msg_type_number_t) \
	(sizeof (thread_precedence_policy_data_t) / sizeof (integer_t)))

  
                          
  
                               
                                                     
                                                                       
                                                                        
                                                    
  
                                                                          
                                                                           
                                                                         
                                                   
  
              
                                    
   

#define THREAD_AFFINITY_POLICY          4

struct thread_affinity_policy {
	integer_t       affinity_tag;
};

#define THREAD_AFFINITY_TAG_NULL                0

typedef struct thread_affinity_policy           thread_affinity_policy_data_t;
typedef struct thread_affinity_policy           *thread_affinity_policy_t;

#define THREAD_AFFINITY_POLICY_COUNT    ((mach_msg_type_number_t) \
	(sizeof (thread_affinity_policy_data_t) / sizeof (integer_t)))

  
                            
   

#define THREAD_BACKGROUND_POLICY        5

struct thread_background_policy {
	integer_t       priority;
};

#define THREAD_BACKGROUND_POLICY_DARWIN_BG 0x1000

typedef struct thread_background_policy         thread_background_policy_data_t;
typedef struct thread_background_policy         *thread_background_policy_t;

#define THREAD_BACKGROUND_POLICY_COUNT  ((mach_msg_type_number_t) \
	(sizeof (thread_background_policy_data_t) / sizeof (integer_t)))


#define THREAD_LATENCY_QOS_POLICY       7
typedef integer_t       thread_latency_qos_t;

struct thread_latency_qos_policy {
	thread_latency_qos_t thread_latency_qos_tier;
};

typedef struct thread_latency_qos_policy        thread_latency_qos_policy_data_t;
typedef struct thread_latency_qos_policy        *thread_latency_qos_policy_t;

#define THREAD_LATENCY_QOS_POLICY_COUNT ((mach_msg_type_number_t)       \
	    (sizeof (thread_latency_qos_policy_data_t) / sizeof (integer_t)))

#define THREAD_THROUGHPUT_QOS_POLICY    8
typedef integer_t       thread_throughput_qos_t;

struct thread_throughput_qos_policy {
	thread_throughput_qos_t thread_throughput_qos_tier;
};

typedef struct thread_throughput_qos_policy     thread_throughput_qos_policy_data_t;
typedef struct thread_throughput_qos_policy     *thread_throughput_qos_policy_t;

#define THREAD_THROUGHPUT_QOS_POLICY_COUNT      ((mach_msg_type_number_t) \
	    (sizeof (thread_throughput_qos_policy_data_t) / sizeof (integer_t)))

#ifdef PRIVATE

  
                       
   
#define THREAD_POLICY_STATE             6

#define THREAD_POLICY_STATE_FLAG_STATIC_PARAM   0x1

struct thread_policy_state {
	integer_t requested;
	integer_t effective;
	integer_t pending;
	integer_t flags;
	uint64_t thps_requested_policy;
	uint64_t thps_effective_policy;
	uint32_t thps_user_promotions;
	uint32_t thps_user_promotion_basepri;
	uint32_t thps_ipc_overrides;
	uint32_t reserved32;
	uint64_t reserved[2];
};

typedef struct thread_policy_state              thread_policy_state_data_t;
typedef struct thread_policy_state              *thread_policy_state_t;

#define THREAD_POLICY_STATE_COUNT       ((mach_msg_type_number_t) \
	(sizeof (thread_policy_state_data_t) / sizeof (integer_t)))

  
                     
   
#define THREAD_QOS_POLICY               9
#define THREAD_QOS_POLICY_OVERRIDE      10

typedef uint8_t thread_qos_t;
#define THREAD_QOS_UNSPECIFIED          0
#define THREAD_QOS_DEFAULT              THREAD_QOS_UNSPECIFIED                        
#define THREAD_QOS_MAINTENANCE          1
#define THREAD_QOS_BACKGROUND           2
#define THREAD_QOS_UTILITY              3
#define THREAD_QOS_LEGACY               4                                       
#define THREAD_QOS_USER_INITIATED       5
#define THREAD_QOS_USER_INTERACTIVE     6

#define THREAD_QOS_LAST                 7

#define THREAD_QOS_MIN_TIER_IMPORTANCE  (-15)

  
                                                             
                                                                     
                                                            
                                                                      
                                                                   
                                                                 
                                                               
                                                               
                                                             
                                                                       
                                                               
                                                  
  
                                                               
                                                                      
                                                                  
                                                              
                                                          
  
                                                                      
                       
  
                                                                    
                                                                  
                                                                      
                                       
  
                                                                          
                                                 
                                                                       
   

#define THREAD_QOS_OVERRIDE_TYPE_UNKNOWN                        (0)
#define THREAD_QOS_OVERRIDE_TYPE_PTHREAD_MUTEX                  (1)
#define THREAD_QOS_OVERRIDE_TYPE_PTHREAD_RWLOCK                 (2)
#define THREAD_QOS_OVERRIDE_TYPE_PTHREAD_EXPLICIT_OVERRIDE      (3)
#define THREAD_QOS_OVERRIDE_TYPE_WILDCARD                       (5)

                                                              
#define THREAD_QOS_OVERRIDE_RESOURCE_WILDCARD (~((user_addr_t)0))

struct thread_qos_policy {
	integer_t qos_tier;
	integer_t tier_importance;
};

typedef struct thread_qos_policy       thread_qos_policy_data_t;
typedef struct thread_qos_policy      *thread_qos_policy_t;

#define THREAD_QOS_POLICY_COUNT    ((mach_msg_type_number_t) \
	(sizeof (thread_qos_policy_data_t) / sizeof (integer_t)))

#endif              

#ifdef PRIVATE

  
                                                                                                        
  
                                                        
                                                                      
   

#define THREAD_POLICY_INTERNAL_STRUCT_VERSION 6

struct thread_requested_policy {
	uint64_t        thrp_int_darwinbg       :1,                                             
	    thrp_ext_darwinbg       :1,
	    thrp_int_iotier         :2,                                       
	    thrp_ext_iotier         :2,
	    thrp_int_iopassive      :1,                                                                   
	    thrp_ext_iopassive      :1,
	    thrp_latency_qos        :3,                                        
	    thrp_through_qos        :3,                                                 

	    thrp_pidbind_bg         :1,                                                            
	    thrp_qos                :3,                                       
	    thrp_qos_relprio        :4,                                                                                  
	    thrp_qos_override       :3,                                                
	    thrp_qos_promote        :3,                                                      
	    thrp_qos_kevent_override:3,                                                            
	    thrp_terminated         :1,                                              
	    thrp_qos_workq_override :3,                                                        
	    thrp_qos_wlsvc_override :3,                                                           

	    thrp_reserved           :26;
};

struct thread_effective_policy {
	uint64_t        thep_darwinbg           :1,                                                                         
	    thep_io_tier            :2,                                              
	    thep_io_passive         :1,                                                                   
	    thep_all_sockets_bg     :1,                                                                                                       
	    thep_new_sockets_bg     :1,                                                                   
	    thep_terminated         :1,                                                                                         
	    thep_qos_ui_is_urgent   :1,                                                                               
	    thep_latency_qos        :3,                                              
	    thep_through_qos        :3,                                                       

	    thep_qos                :3,                                       
	    thep_qos_relprio        :4,                                                                                  
	    thep_qos_promote        :3,                                                          
	    thep_promote_above_task :1,                                                                

	    thep_reserved           :39;
};

#endif              


#endif                              
