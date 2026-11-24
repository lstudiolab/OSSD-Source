/*
 * Copyright (c) 2017 Apple Inc. All rights reserved.
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

#ifndef _TURNSTILE_H_
#define _TURNSTILE_H_

#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <sys/cdefs.h>

#if PRIVATE
#define TURNSTILE_MAX_HOP_DEFAULT (10)
struct turnstile_stats {
	uint64_t ts_priority_propagation;
	uint64_t ts_no_inheritor;
	uint64_t ts_thread_runnable;
	uint64_t ts_no_priority_change_required;
	uint64_t ts_above_ui_pri_change;
	uint64_t ts_no_turnstile;
};
#endif

#ifdef KERNEL_PRIVATE
#include <kern/queue.h>
#include <sys/queue.h>
#include <kern/waitq.h>
#include <kern/priority_queue.h>
#include <os/refcnt.h>
#include <kern/assert.h>
#include <kern/kern_types.h>
#include <kern/mpsc_queue.h>
#include <kern/locks.h>

  
                                                                                      
                                                                                                 
   
typedef enum __attribute__((packed)) turnstile_type {
	TURNSTILE_NONE = 0,
	TURNSTILE_KERNEL_MUTEX = 1,
	TURNSTILE_ULOCK = 2,
	TURNSTILE_PTHREAD_MUTEX = 3,
	TURNSTILE_SYNC_IPC = 4,
	TURNSTILE_WORKLOOPS = 5,
	TURNSTILE_WORKQS = 6,
	TURNSTILE_KNOTE = 7,
	TURNSTILE_SLEEP_INHERITOR = 8,
	TURNSTILE_TOTAL_TYPES = 9,
} turnstile_type_t;

  
                                                        
                     
  
                         
                                        
                         
                                              
  
                  
                                  
                         
                                              
  
                          
                                       
                         
                                              
  
                     
                                   
                                                                             
                                                                               
                                                                                  
                                               
  
                      
                
                   
                                                              
                                             
                                             
                                                         
  
                   
                               
                       
                                              
  
                  
                               
                             
  
                            
                                                  
                         
                                              
  
   

typedef enum __attribute__((flag_enum)) turnstile_promote_policy {
	TURNSTILE_PROMOTE_NONE = 0,
	TURNSTILE_KERNEL_PROMOTE = 0x1,
	TURNSTILE_USER_PROMOTE = 0x2,
	TURNSTILE_USER_IPC_PROMOTE = 0x4,
} turnstile_promote_policy_t;

typedef enum __attribute__((flag_enum)) turnstile_hash_lock_policy {
	TURNSTILE_HASH_LOCK_POLICY_NONE = 0,
	TURNSTILE_IRQ_UNSAFE_HASH = 0x1,
	TURNSTILE_LOCKED_HASH = 0x2,
} turnstile_hash_lock_policy_t;

  
                        
  
                                                                            
                           
                                                               
                                                                                          
                                                                                                       
                                                                       
  
                                                                   
     

#define TURNSTILE_STATE_THREAD          0x1
#define TURNSTILE_STATE_FREELIST        0x2
#define TURNSTILE_STATE_HASHTABLE       0x4
#define TURNSTILE_STATE_PROPRIETOR      0x8

                                                      
#if DEVELOPMENT || DEBUG

#define turnstile_state_init(ts, state)         \
MACRO_BEGIN                                     \
	        ts->ts_state = state;           \
MACRO_END

#define turnstile_state_add(ts, state)          \
MACRO_BEGIN                                     \
	        assert((ts->ts_state & (state)) == 0);  \
	        ts->ts_state |= state;                  \
MACRO_END

#define turnstile_state_remove(ts, state)       \
MACRO_BEGIN                                     \
	        assert(ts->ts_state & (state));         \
	        ts->ts_state &= ~(state);               \
MACRO_END

#else                            

#define turnstile_state_init(ts, state)         \
MACRO_BEGIN                                     \
	        (void)ts;                       \
MACRO_END

#define turnstile_state_add(ts, state)          \
MACRO_BEGIN                                     \
	        (void)ts;                       \
MACRO_END

#define turnstile_state_remove(ts, state)       \
MACRO_BEGIN                                     \
	        (void)ts;                       \
MACRO_END

#endif                           

struct knote;
struct turnstile;

  
                         
  
                             
                                               
                                              
                    
  
                           
                                               
                                                
                                         
                  
  
                             
                                                
  
                                
                                                   
  
                            
                                                  
  
                                       
                                                  
  
                             
                                                      
  
                                                              
  
                          
                                                          
                                                   
  
                             
                                                      
                                                     
                                             
  
                                                        
                                                      
                     
   
typedef enum __attribute__((flag_enum)) __attribute__((packed)) turnstile_update_flags {
	TURNSTILE_UPDATE_FLAGS_NONE = 0,
	TURNSTILE_IMMEDIATE_UPDATE = 0x1,
	TURNSTILE_DELAYED_UPDATE = 0x2,
	TURNSTILE_INHERITOR_THREAD = 0x4,
	TURNSTILE_INHERITOR_TURNSTILE = 0x8,
	TURNSTILE_INHERITOR_NEEDS_PRI_UPDATE = 0x10,
	TURNSTILE_NEEDS_PRI_UPDATE = 0x20,
	TURNSTILE_INHERITOR_WORKQ = 0x40,
	TURNSTILE_UPDATE_BOOST = 0x80,
} turnstile_update_flags_t;

#define TURNSTILE_NULL ((struct turnstile *)0)

typedef void * turnstile_inheritor_t;

#define TURNSTILE_INHERITOR_NULL NULL

#ifdef XNU_KERNEL_PRIVATE

                               
  
                            
                                                          
                                      
  
                                
                                                        
                                          
  
                      
                                                             
                                                           
  
                   
                                                           
                                                          
                                                         
  
                   
                                                        
                                                            
                                                             
            
  
                           
                                                           
                                                           
                                                             
                         
  
                      
                                                        
                                                          
                                         
  
                          
                                                               
   
typedef enum __attribute__((flag_enum)) turnstile_stats_update_flags {
	TSU_FLAGS_NONE = 0,
	TSU_TURNSTILE_BLOCK_COUNT = 0x1,
	TSU_REGULAR_WAITQ_BLOCK_COUNT = 0x2,
	TSU_PRI_PROPAGATION = 0x4,
	TSU_NO_INHERITOR = 0x8,
	TSU_NO_TURNSTILE = 0x10,
	TSU_NO_PRI_CHANGE_NEEDED = 0x20,
	TSU_THREAD_RUNNABLE = 0x40,
	TSU_ABOVE_UI_PRI_CHANGE = 0x80,
	TSU_THREAD_ARG = 0x100,
	TSU_TURNSTILE_ARG = 0x200,
	TSU_BOOST_ARG = 0x400,
} turnstile_stats_update_flags_t;

SLIST_HEAD(turnstile_list, turnstile);

struct turnstile {
	struct waitq                  ts_waitq;                                               
	turnstile_inheritor_t         ts_inheritor;                                                                 
	union {
		struct turnstile_list ts_free_turnstiles;                                  
		SLIST_ENTRY(turnstile) ts_free_elm;                                                
	};
	struct priority_queue_sched_max ts_inheritor_queue;                                                         
	union {
		struct priority_queue_entry_sched ts_inheritor_links;                               
		struct mpsc_queue_chain   ts_deallocate_link;                                
	};
	SLIST_ENTRY(turnstile)        ts_htable_link;                                                        
	uintptr_t                     ts_proprietor;                                             
	os_refcnt_t                   ts_refcount;                                               
	_Atomic uint32_t              ts_type_gencount;                                                                             
	uint32_t                      ts_port_ref;                                                                     
	turnstile_update_flags_t      ts_inheritor_flags;                                                
	uint8_t                       ts_priority;                                           

#if DEVELOPMENT || DEBUG
	uint8_t                       ts_state;                                                   
	queue_chain_t                 ts_global_elm;                                     
	thread_t                      ts_thread;                                                      
	thread_t                      ts_prev_thread;                                                               
#endif
};

#define waitq_to_turnstile(waitq) __container_of(waitq, struct turnstile, ts_waitq)

                                                         

#define TURNSTILE_PROPRIETOR_NULL 0ul

  
                        
  
                                                
  
              
  
                 
   
void
turnstiles_init(void);

  
                        
  
                                     
  
              
  
           
                          
   
struct turnstile *
turnstile_alloc(void);

  
                          
  
                                          
  
        
                    
  
                 
   
void
turnstile_destroy(struct turnstile *turnstile);

  
                            
  
                                                  
  
                  
  
                 
   
void
turnstile_reference(struct turnstile *turnstile);

  
                             
  
                                                  
                                                      
  
                  
  
                 
   
void
turnstile_deallocate(struct turnstile *turnstile);

  
                                                  
  
                                                 
  
              
               
  
                           
   
void
turnstile_waitq_add_thread_priority_queue(
	struct waitq* wq,
	thread_t thread);

  
                                  
  
                                                                                  
  
                  
  
                 
   
void
turnstile_deallocate_safe(struct turnstile *turnstile);

  
                                            
  
                                               
                                                             
                          
  
                  
  
                                                                          
                                                                                            
  
                              
   
boolean_t
turnstile_recompute_priority_locked(
	struct turnstile *turnstile);

  
                                     
  
                                               
                                                             
                          
  
                  
  
                                                                          
                                                                                            
   
boolean_t
turnstile_recompute_priority(
	struct turnstile *turnstile);

  
                                                    
  
                                                                          
                                                 
  
                                                                 
                                         
  
                  
  
           
                                     
                                         
   
int
turnstile_workq_proprietor_of_max_turnstile(
	struct turnstile *turnstile,
	uintptr_t *proprietor);

  
                                       
  
                                                                          
                                                                  
  
                           
  
           
                               
                                                                 
                                                            
                                                    
   
int
turnstile_workloop_pusher_info(
	struct turnstile *turnstile,
	thread_t *thread,
	ipc_port_t *port,
	struct knote **knote_out);

  
                          
  
                                                        
                          
  
                                                     
  
                 
   
void
turnstile_cleanup(void);

  
                                               
  
                                                           
                                                           
  
                                                   
  
                 
   
void
turnstile_update_thread_priority_chain(thread_t thread);

  
                                          
  
                                                                   
                                                        
  
        
                    
                                                                            
  
           
                                                                    
   
void
turnstile_update_inheritor_locked(struct turnstile *turnstile);

  
                                                     
  
                                                                         
  
               
  
                                                              
  
                           
   
int
thread_get_inheritor_turnstile_base_priority(thread_t thread);

  
                                                      
  
                                                                          
  
               
  
                                                               
  
                           
   
int
thread_get_inheritor_turnstile_sched_priority(thread_t thread);

  
                                     
  
                                                                          
  
               
  
                                                               
                                      
  
                            
   
struct turnstile *
thread_get_waiting_turnstile(thread_t thread);

  
                                       
  
                                                          
                               
  
             
                              
  
                                                         
                                      
  
                                        
   
struct turnstile *
turnstile_lookup_by_proprietor(uintptr_t proprietor, turnstile_type_t type);

  
                              
  
                                                             
  
                             
  
                                                       
   

boolean_t
turnstile_has_waiters(struct turnstile *turnstile);

  
                               
  
                                                                  
  
                                                             
                                             
                             
  
                   
   
void
turnstile_stats_update(
	int hop __assert_only,
	turnstile_stats_update_flags_t flags __assert_only,
	turnstile_inheritor_t inheritor __assert_only);

#if DEVELOPMENT || DEBUG

#define SYSCTL_TURNSTILE_TEST_USER_DEFAULT              1
#define SYSCTL_TURNSTILE_TEST_USER_HASHTABLE            2
#define SYSCTL_TURNSTILE_TEST_KERNEL_DEFAULT            3
#define SYSCTL_TURNSTILE_TEST_KERNEL_HASHTABLE          4

                                                                
int
tstile_test_prim_lock(int val);

int
tstile_test_prim_unlock(int val);

int
turnstile_get_boost_stats_sysctl(void *req);
int
turnstile_get_unboost_stats_sysctl(void *req);
#endif                           
#endif                         

               

  
                                   
  
                                                                       
                                                                         
                                                            
                                                                              
  
        
                                       
                                           
                         
  
                                                                            
   
unsigned
turnstile_hash_bucket_lock(uintptr_t proprietor, uint32_t *index_proprietor, turnstile_type_t type);

  
                                     
  
                                                                         
                                                                         
                                                            
                                                                              
  
        
                                       
                                           
                         
                                                           
  
   
void
turnstile_hash_bucket_unlock(uintptr_t proprietor, uint32_t *index_proprietor, turnstile_type_t type, unsigned s);

  
                          
  
                                                                                             
                                                                                     
                                                                                                                  
                                                                                                                  
                                                      
  
        
                     
                                                         
                                                             
                            
  
           
               
   
struct turnstile *
turnstile_prepare(
	uintptr_t proprietor,
	struct turnstile **tstore,
	struct turnstile *turnstile,
	turnstile_type_t type);

  
                           
  
                                                                                           
                                                                                     
                                                                                    
  
        
                     
                                                          
                                                                                    
                            
  
           
          
   
void
turnstile_complete(
	uintptr_t proprietor,
	struct turnstile **tstore,
	struct turnstile **turnstile,
	turnstile_type_t type);

  
                                   
  
                                                                   
                                                                            
                                                                
  
        
                    
                    
                                                                                     
  
           
                                                                   
   
void
turnstile_update_inheritor(
	struct turnstile *turnstile,
	turnstile_inheritor_t new_inheritor,
	turnstile_update_flags_t flags);

typedef enum turnstile_update_complete_flags {
	TURNSTILE_INTERLOCK_NOT_HELD = 0x1,
	TURNSTILE_INTERLOCK_HELD = 0x2,
} turnstile_update_complete_flags_t;

  
                                            
  
                                                                       
                                                                    
                                                                
                                                                     
                                                                    
  
        
                    
                         
  
                 
   
void
turnstile_update_inheritor_complete(
	struct turnstile *turnstile,
	turnstile_update_complete_flags_t flags);


  
                                                         
  
                                                                
                        
        
                    
                        
                
  
                               
   
void
turnstile_kernel_update_inheritor_on_wake_locked(
	struct turnstile *turnstile,
	turnstile_inheritor_t new_inheritor,
	turnstile_update_flags_t flags);

  
                                                                                       
                                                                              
                                   
  
                                                                                 
                                                 
   

extern wait_result_t
lck_mtx_sleep_with_inheritor_and_turnstile_type(lck_mtx_t *lock, lck_sleep_action_t lck_sleep_action, event_t event, thread_t inheritor, wait_interrupt_t interruptible, uint64_t deadline, turnstile_type_t type);

extern wait_result_t
lck_rw_sleep_with_inheritor_and_turnstile_type(lck_rw_t *lock, lck_sleep_action_t lck_sleep_action, event_t event, thread_t inheritor, wait_interrupt_t interruptible, uint64_t deadline, turnstile_type_t type);

extern kern_return_t
wakeup_with_inheritor_and_turnstile_type(event_t event, turnstile_type_t type, wait_result_t result, bool wake_one, lck_wake_action_t action, thread_t *thread_wokenup);

extern kern_return_t
change_sleep_inheritor_and_turnstile_type(event_t event, thread_t inheritor, turnstile_type_t type);

#endif                     
#if XNU_KERNEL_PRIVATE

struct workqueue;

                         
extern void workq_reference(struct workqueue *wq);
extern void workq_deallocate_safe(struct workqueue *wq);
extern bool workq_is_current_thread_updating_turnstile(struct workqueue *wq);
extern void workq_schedule_creator_turnstile_redrive(struct workqueue *wq,
    bool locked);

#endif                         

#endif                    
