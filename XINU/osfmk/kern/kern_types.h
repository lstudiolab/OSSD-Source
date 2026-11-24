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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_COPYRIGHT@
 */

#ifndef _KERN_KERN_TYPES_H_
#define _KERN_KERN_TYPES_H_

#include <stdint.h>
#include <mach/mach_types.h>
#include <mach/machine/vm_types.h>

#ifdef  KERNEL_PRIVATE

#ifndef MACH_KERNEL_PRIVATE

struct zone;

#ifndef __LP64__
struct wait_queue { unsigned int opaque[2]; uintptr_t opaquep[2]; };
#else
struct wait_queue { unsigned char opaque[32]; };
#endif

#endif                           

typedef struct zone                     *zone_t;
#define         ZONE_NULL                       ((zone_t) 0)

typedef struct wait_queue               *wait_queue_t;
#define         WAIT_QUEUE_NULL         ((wait_queue_t) 0)
#define                 SIZEOF_WAITQUEUE        sizeof(struct wait_queue)

typedef void *                          ipc_kobject_t;
#define         IKO_NULL                        ((ipc_kobject_t) 0)

#endif                      

typedef void *event_t;                          
#define         NO_EVENT                        ((event_t) 0)

typedef uint64_t event64_t;                                    
#define         NO_EVENT64              ((event64_t) 0)
#define         CAST_EVENT64_T(a_ptr)   ((event64_t)((uintptr_t)(a_ptr)))

  
                                 
   
typedef int wait_result_t;
#define THREAD_WAITING          -1                                     
#define THREAD_AWAKENED         0                                  
#define THREAD_TIMED_OUT        1                                    
#define THREAD_INTERRUPTED      2                                        
#define THREAD_RESTART          3                                               
#define THREAD_NOT_WAITING      10                                              

typedef void (*thread_continue_t)(void *, wait_result_t);
#define THREAD_CONTINUE_NULL    ((thread_continue_t) NULL)

  
                                
  
                                     
                                                                              
                          
  
                                                                               
                                 
  
                        
                                                                               
                                                          
  
                                                                               
                                                                                     
                                                                                  
                                                                                
                                                                                     
  
                    
                                                                               
                                                                                
                                                                 
  
                                                                               
                                                                              
                                                                                     
                                        
  
                                                                             
                                
  
                        
                                                                           
                                                                               
                                              
  
                                                                        
                                                                                
                                                                          
  
                                                                 
                                                                              
                                    
  
                         
  
                                                                       
                                                                                 
                                                                                  
                             
  
                                    
  
                                                           
  
                                                                                    
                                                                               
                                          
  
                                       
  
                                                                                    
                                                                                             
                                                                                
  
                                                                                   
                                                                             
                                                                            
                                                                            
                                                                    
                                                                                         
                          
  
                                                                                   
                                                                               
  
                                                                                     
                                                                        
   
typedef int wait_interrupt_t;
#define THREAD_UNINT                    0x00000000                              
#define THREAD_INTERRUPTIBLE            0x00000001                              
#define THREAD_ABORTSAFE                0x00000002                              
#define THREAD_WAIT_NOREPORT_KERNEL     0x80000000
#define THREAD_WAIT_NOREPORT_USER       0x40000000
#define THREAD_WAIT_NOREPORT            (THREAD_WAIT_NOREPORT_KERNEL | THREAD_WAIT_NOREPORT_USER)

typedef int wait_timeout_urgency_t;
#define TIMEOUT_URGENCY_SYS_NORMAL      0x00                                                          
#define TIMEOUT_URGENCY_SYS_CRITICAL    0x01                                                           
#define TIMEOUT_URGENCY_SYS_BACKGROUND  0x02                                                             

#define TIMEOUT_URGENCY_USER_MASK       0x10                                                               
#define TIMEOUT_URGENCY_USER_NORMAL     0x10                                                        
#define TIMEOUT_URGENCY_USER_CRITICAL   0x11                                                         
#define TIMEOUT_URGENCY_USER_BACKGROUND 0x12                                                           

#define TIMEOUT_URGENCY_MASK            0x13                                                  

#define TIMEOUT_URGENCY_LEEWAY          0x20                                                    

#define TIMEOUT_URGENCY_FIRST_AVAIL     0x40                                                                    
#define TIMEOUT_URGENCY_RATELIMITED     0x80

  
                                         
                                                                                
   
#define TIMEOUT_NO_LEEWAY               (0ULL)
#define TIMEOUT_WAIT_FOREVER            (0ULL)

#ifdef  KERNEL_PRIVATE

  
                                                                                 
                                       
   

#ifdef  MACH_KERNEL_PRIVATE

#include <kern/misc_protos.h>
typedef  struct clock                   *clock_t;

typedef struct mig_object               *mig_object_t;
#define MIG_OBJECT_NULL                 ((mig_object_t) 0)

typedef struct mig_notify               *mig_notify_t;
#define MIG_NOTIFY_NULL                 ((mig_notify_t) 0)

typedef struct pset_node                *pset_node_t;
#define PSET_NODE_NULL                  ((pset_node_t) 0)

typedef struct affinity_set             *affinity_set_t;
#define AFFINITY_SET_NULL               ((affinity_set_t) 0)

typedef struct run_queue               *run_queue_t;
#define RUN_QUEUE_NULL                 ((run_queue_t) 0)

typedef struct grrr_run_queue               *grrr_run_queue_t;
#define GRRR_RUN_QUEUE_NULL                 ((grrr_run_queue_t) 0)

typedef struct grrr_group                                       *grrr_group_t;
#define GRRR_GROUP_NULL                                         ((grrr_group_t) 0)

#if defined(CONFIG_SCHED_MULTIQ)
typedef struct sched_group              *sched_group_t;
#define SCHED_GROUP_NULL                ((sched_group_t) 0)
#endif                                   

#else                            

struct wait_queue_set;
struct _wait_queue_link;

#endif                           

typedef struct wait_queue_set   *wait_queue_set_t;
#define WAIT_QUEUE_SET_NULL     ((wait_queue_set_t)0)
#define SIZEOF_WAITQUEUE_SET    wait_queue_set_size()

typedef struct _wait_queue_link *wait_queue_link_t;
#define WAIT_QUEUE_LINK_NULL    ((wait_queue_link_t)0)
#define SIZEOF_WAITQUEUE_LINK   wait_queue_link_size()

typedef struct perfcontrol_state        *perfcontrol_state_t;
#define PERFCONTROL_STATE_NULL          ((perfcontrol_state_t)0)

  
                                                         
   
typedef enum perfcontrol_event {
	  
                              
                                                      
                                                          
                                            
    
	QUANTUM_EXPIRY          = 1,
	THREAD_GROUP_UPDATE     = 2,
	PERFCONTROL_ATTR_UPDATE = 3,
	  
                         
                                                       
                                                        
                                    
    
	CONTEXT_SWITCH          = 10,
	IDLE                    = 11
} perfcontrol_event;

  
                                                                          
                                                                                        
            
                               
  
                                                                  
                                                          
                                                                      
                                                                
                                                           
  
                                                                  
                                                                      
                                                                  
                                                                  
                                                                    
                                                                  
                                     
   
#define PERFCONTROL_CALLOUT_WAKE_UNSAFE                 (0x1)
#define PERFCONTROL_CALLOUT_BLOCKING_TG_RENDER_SERVER   (0x2)

  
                                                   
                                                    
                                                     
                                 
   
typedef enum perfcontrol_class {
	                 
	PERFCONTROL_CLASS_IDLE           = 1,
	                   
	PERFCONTROL_CLASS_KERNEL         = 2,
	                     
	PERFCONTROL_CLASS_REALTIME       = 3,
	                       
	PERFCONTROL_CLASS_BACKGROUND     = 4,
	                    
	PERFCONTROL_CLASS_UTILITY        = 5,
	                                    
	PERFCONTROL_CLASS_NONUI          = 6,
	                       
	PERFCONTROL_CLASS_UI             = 7,
	                     
	PERFCONTROL_CLASS_ABOVEUI        = 8,
	                   
	PERFCONTROL_CLASS_MAX            = 9,
} perfcontrol_class_t;

  
                           
  
                                                                        
                                        
                                                                             
                                                                          
                      
                                                                            
                          
                                                                              
                                                                 
  
                                                                          
                   
   
typedef union sched_clutch_edge {
	struct {
		uint32_t
		                sce_migration_allowed : 1,
		                sce_steal_allowed     : 1,
		    _reserved             : 30;
		uint32_t        sce_migration_weight;
	};
	uint64_t sce_edge_packed;
} sched_clutch_edge;

#endif                      

#endif                           
