/*
 * Copyright (c) 1993-1995, 1999-2008 Apple Inc. All rights reserved.
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

   
                         
                                                             
   

#ifndef _KERN_THREAD_CALL_H_
#define _KERN_THREAD_CALL_H_

#include <mach/mach_types.h>

#include <kern/clock.h>

#include <sys/cdefs.h>

struct thread_call;
typedef struct thread_call *thread_call_t;

typedef void *thread_call_param_t;
typedef void (*thread_call_func_t)(
	thread_call_param_t     param0,
	thread_call_param_t     param1);
   
                                
                                                                                 
                                                                                
                                                                            
                                                                                 
                                                                                 
                                                                                   
                                                                                         
                                                                                   
                   
                                                                                     
            
                                                                                          
                                                           
                                                                                 
            
   
typedef enum {
	THREAD_CALL_PRIORITY_HIGH        = 0,
	THREAD_CALL_PRIORITY_KERNEL      = 1,
	THREAD_CALL_PRIORITY_USER        = 2,
	THREAD_CALL_PRIORITY_LOW         = 3,
	THREAD_CALL_PRIORITY_KERNEL_HIGH = 4
} thread_call_priority_t;

enum {
	                                                                                                                      
	THREAD_CALL_OPTIONS_ONCE   = 0x00000001,
#ifdef XNU_KERNEL_PRIVATE
	                                                                                                                                  
	THREAD_CALL_OPTIONS_SIGNAL = 0x00000002,
#endif                         
};
typedef uint32_t thread_call_options_t;

__BEGIN_DECLS

   
                               
                                                                     
                                                                                       
                                                                                    
                                                                                      
                                               
                                                                                
                               
                                           
   
extern boolean_t        thread_call_enter(
	thread_call_t           call);
   
                                
                                                                                              
                                                                             
                                                      
                                                                                
                               
                                           
                                             
   
extern boolean_t        thread_call_enter1(
	thread_call_t           call,
	thread_call_param_t     param1);

   
                                       
                                                                              
                                                                                         
                                                                                            
                                                                                                  
                                                                                
                                                                                
                               
                                           
                                                                               
   
extern boolean_t        thread_call_enter_delayed(
	thread_call_t           call,
	uint64_t                deadline);
   
                                        
                                                                                                       
                                                                         
                                                                
                                                                                
                               
                                           
                                               
                                                                               
   
extern boolean_t        thread_call_enter1_delayed(
	thread_call_t           call,
	thread_call_param_t     param1,
	uint64_t                deadline);
#ifdef XNU_KERNEL_PRIVATE

  
                                                               
                              
  
                                                               
                                                                     
                                                                 
                                   
  
                                                                          
                                                                        
                                             
   
#define THREAD_CALL_DELAY_SYS_NORMAL            TIMEOUT_URGENCY_SYS_NORMAL
#define THREAD_CALL_DELAY_SYS_CRITICAL          TIMEOUT_URGENCY_SYS_CRITICAL
#define THREAD_CALL_DELAY_SYS_BACKGROUND        TIMEOUT_URGENCY_SYS_BACKGROUND

#define THREAD_CALL_DELAY_USER_MASK             TIMEOUT_URGENCY_USER_MASK
#define THREAD_CALL_DELAY_USER_NORMAL           TIMEOUT_URGENCY_USER_NORMAL
#define THREAD_CALL_DELAY_USER_CRITICAL         TIMEOUT_URGENCY_USER_CRITICAL
#define THREAD_CALL_DELAY_USER_BACKGROUND       TIMEOUT_URGENCY_USER_BACKGROUND

#define THREAD_CALL_DELAY_URGENCY_MASK          TIMEOUT_URGENCY_MASK

  
                                                                     
                                                                      
                                                                   
                          
   
#define THREAD_CALL_DELAY_LEEWAY                TIMEOUT_URGENCY_LEEWAY

  
                                                              
                                                                            
                                                  
   
#define THREAD_CALL_CONTINUOUS  0x100

   
                                                   
                                                                              
                                                                                         
                                                                                            
                                                                                                  
                                                                                
                                                                                
                               
                                           
                                               
                                                                               
                                                                                              
                                           
                                                    
   
extern boolean_t        thread_call_enter_delayed_with_leeway(
	thread_call_t           call,
	thread_call_param_t     param1,
	uint64_t                deadline,
	uint64_t                leeway,
	uint32_t                flags);

#endif                         

   
                                
                                                                      
                                                                        
                                                                         
                                                                          
                                                                    
                                                                         
                                                               
                                                                         
   
extern boolean_t        thread_call_cancel(
	thread_call_t           call);
   
                                     
                                                                      
                                                               
                                                                        
                                                                         
                                                                          
                                                                    
                                                                                
                                                                                       
                                                                              
                                                                                    
                                                                                
                                    
                                                                         
   
extern boolean_t        thread_call_cancel_wait(
	thread_call_t           call);

   
                                  
                                                                             
                                                                         
                                                                           
                                                                 
                                                     
                                                                          
   
extern thread_call_t    thread_call_allocate(
	thread_call_func_t      func,
	thread_call_param_t     param0);

   
                                                
                                                                          
                                                                       
                           
                                                                 
                                                     
                                
                                                                          
   
extern thread_call_t    thread_call_allocate_with_priority(
	thread_call_func_t      func,
	thread_call_param_t     param0,
	thread_call_priority_t  pri);

   
                                               
                                                                          
                                                                       
                                        
                                                                 
                                                     
                                
                                    
                                                                          
   
extern thread_call_t    thread_call_allocate_with_options(
	thread_call_func_t      func,
	thread_call_param_t     param0,
	thread_call_priority_t  pri,
	thread_call_options_t   options);

#ifdef KERNEL_PRIVATE
   
                                           
                                                                     
                                                                                                 
                                           
                                                                 
                                                     
                                                                             
                                                                                         
                                                                          
   
extern thread_call_t
thread_call_allocate_with_qos(thread_call_func_t        func,
    thread_call_param_t       param0,
    int                       qos_tier,
    thread_call_options_t     options);

   
                                   
                                                                                             
                                                            
                                           
                                                       
   
extern boolean_t
thread_call_wait_once(thread_call_t call);
#endif                     

   
                              
                                    
                                                                                       
                                                                                  
                                                                                 
                                                                                  
                                                                                
                                                    
                                                                               
                                           
   
extern boolean_t        thread_call_free(
	thread_call_t           call);

   
                                  
                                                                                
                                       
                                                                                  
                                                              
   
boolean_t               thread_call_isactive(
	thread_call_t call);
__END_DECLS

#ifdef  MACH_KERNEL_PRIVATE

#include <kern/queue.h>
#include <kern/priority_queue.h>

__enum_closed_decl(thread_call_index_t, uint16_t, {
	THREAD_CALL_INDEX_HIGH          = 0,
	THREAD_CALL_INDEX_KERNEL        = 1,
	THREAD_CALL_INDEX_USER          = 2,
	THREAD_CALL_INDEX_LOW           = 3,
	THREAD_CALL_INDEX_KERNEL_HIGH   = 4,
	THREAD_CALL_INDEX_QOS_UI        = 5,
	THREAD_CALL_INDEX_QOS_IN        = 6,
	THREAD_CALL_INDEX_QOS_UT        = 7,
	THREAD_CALL_INDEX_MAX           = 8,                                      
});

__options_closed_decl(thread_call_flags_t, uint16_t, {
	THREAD_CALL_ALLOC               = 0x0001,                                          
	THREAD_CALL_WAIT                = 0x0002,                                                      
	THREAD_CALL_DELAYED             = 0x0004,                           
	THREAD_CALL_RUNNING             = 0x0008,                                            
	THREAD_CALL_SIGNAL              = 0x0010,                                                        
	THREAD_CALL_ONCE                = 0x0020,                                                       
	THREAD_CALL_RESCHEDULE          = 0x0040,                                                           
	THREAD_CALL_RATELIMITED         = 0x0080,                                                   
	THREAD_CALL_FLAG_CONTINUOUS     = 0x0100,                                           
});

struct thread_call {
	                                   
	uint64_t                                tc_soft_deadline;
	                                                                               
	struct priority_queue_entry_deadline    tc_pqlink;
	                                               
	queue_head_t                            *tc_queue;
	queue_chain_t                           tc_qlink;
	thread_call_index_t                     tc_index;
	thread_call_flags_t                     tc_flags;
	int32_t                                 tc_refs;
	                                  
	uint64_t                                tc_ttd;
	                                           
	uint64_t                                tc_pending_timestamp;
	thread_call_func_t                      tc_func;
	thread_call_param_t                     tc_param0;
	thread_call_param_t                     tc_param1;
	uint64_t                                tc_submit_count;
	uint64_t                                tc_finish_count;
};

typedef struct thread_call thread_call_data_t;

extern void             thread_call_initialize(void);

extern void             thread_call_setup(
	thread_call_t                   call,
	thread_call_func_t              func,
	thread_call_param_t             param0);

extern void             thread_call_setup_with_options(
	thread_call_t                   call,
	thread_call_func_t              func,
	thread_call_param_t             param0,
	thread_call_priority_t          pri,
	thread_call_options_t           options);

extern void             thread_call_delayed_timer_rescan_all(void);
extern uint64_t         thread_call_get_armed_deadline(thread_call_t call);

struct thread_call_thread_state;

#endif                           

#ifdef  XNU_KERNEL_PRIVATE

__BEGIN_DECLS

  
                                                               
                                                         
                                                                
                
   

extern void             thread_call_func_delayed(
	thread_call_func_t              func,
	thread_call_param_t             param,
	uint64_t                        deadline);

extern void             thread_call_func_delayed_with_leeway(
	thread_call_func_t              func,
	thread_call_param_t             param,
	uint64_t                deadline,
	uint64_t                leeway,
	uint32_t                flags);

  
                                                                         
                               
  
                                                                  
   
extern boolean_t        thread_call_func_cancel(
	thread_call_func_t      func,
	thread_call_param_t     param,
	boolean_t               cancel_all);

  
                                                                                        
   
extern void adjust_cont_time_thread_calls(void);

                                                                                 
extern void thread_call_start_iotes_invocation(thread_call_t call);

__END_DECLS

#endif                          

#endif                            
