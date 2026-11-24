/*
 * Copyright (c) 2003-2019 Apple Inc. All rights reserved.
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

#ifndef _KERN_LOCKS_H_
#define _KERN_LOCKS_H_

#include <sys/cdefs.h>
#include <sys/appleapiopts.h>
#include <mach/boolean.h>
#include <mach/mach_types.h>
#include <kern/kern_types.h>
#include <kern/lock_group.h>
#include <machine/locks.h>

__BEGIN_DECLS

typedef unsigned int            lck_sleep_action_t;

#define LCK_SLEEP_DEFAULT       0x00                                                                    
                                            
#define LCK_SLEEP_UNLOCK        0x01                                            
#define LCK_SLEEP_SHARED        0x02                                                   
#define LCK_SLEEP_EXCLUSIVE     0x04                                                      
#define LCK_SLEEP_SPIN          0x08                                                    
#define LCK_SLEEP_PROMOTED_PRI  0x10                                      
#define LCK_SLEEP_SPIN_ALWAYS   0x20                                                           

#define LCK_SLEEP_MASK          0x3f                       

typedef unsigned int            lck_wake_action_t;

#define LCK_WAKE_DEFAULT                0x00                                                                        
#define LCK_WAKE_DO_NOT_TRANSFER_PUSH   0x01                                                  

#ifdef  XNU_KERNEL_PRIVATE
#include <kern/startup.h>

typedef struct _lck_attr_ {
	unsigned int    lck_attr_val;
} lck_attr_t;

extern lck_attr_t      LockDefaultLckAttr;

#define LCK_ATTR_NONE                   0
#define LCK_ATTR_DEBUG                  0x00000001
#define LCK_ATTR_RW_SHARED_PRIORITY     0x00010000
#else                          
typedef struct __lck_attr__ lck_attr_t;
#endif                          

#define LCK_ATTR_NULL (lck_attr_t *)NULL

extern  lck_attr_t      *lck_attr_alloc_init(void);

extern  void            lck_attr_setdefault(
	lck_attr_t              *attr);

extern  void            lck_attr_setdebug(
	lck_attr_t              *attr);

extern  void            lck_attr_cleardebug(
	lck_attr_t              *attr);

#ifdef  XNU_KERNEL_PRIVATE

#if __x86_64__
  
                                                  
   
#define HAS_EXT_MUTEXES 1
#endif                 

typedef union {
	uint16_t tcurnext;
	struct {
		uint8_t cticket;
		uint8_t nticket;
	};
} lck_ticket_internal;

typedef struct {
	lck_ticket_internal tu;
	uintptr_t lck_owner;
} lck_ticket_t;

void lck_ticket_init(lck_ticket_t *tlock, lck_grp_t *grp);

#if LOCK_STATS
void lck_ticket_lock(lck_ticket_t *tlock, lck_grp_t *grp);
#else
void lck_ticket_lock(lck_ticket_t *tlock);
#define lck_ticket_lock(tlock, grp) lck_ticket_lock(tlock)
#endif                 

void lck_ticket_unlock(lck_ticket_t *tlock);
void lck_ticket_assert_owned(lck_ticket_t *tlock);

extern  void            lck_attr_rw_shared_priority(
	lck_attr_t              *attr);
#endif

extern  void            lck_attr_free(
	lck_attr_t              *attr);

#define decl_lck_spin_data(class, name)     class lck_spin_t name

extern lck_spin_t      *lck_spin_alloc_init(
	lck_grp_t               *grp,
	lck_attr_t              *attr);

extern void             lck_spin_init(
	lck_spin_t              *lck,
	lck_grp_t               *grp,
	lck_attr_t              *attr);

extern void             lck_spin_lock(
	lck_spin_t              *lck);

extern void             lck_spin_lock_grp(
	lck_spin_t              *lck,
	lck_grp_t               *grp);

extern void             lck_spin_unlock(
	lck_spin_t              *lck);

extern void             lck_spin_destroy(
	lck_spin_t              *lck,
	lck_grp_t               *grp);

extern void             lck_spin_free(
	lck_spin_t              *lck,
	lck_grp_t               *grp);

extern wait_result_t    lck_spin_sleep(
	lck_spin_t              *lck,
	lck_sleep_action_t      lck_sleep_action,
	event_t                 event,
	wait_interrupt_t        interruptible);

extern wait_result_t    lck_spin_sleep_grp(
	lck_spin_t              *lck,
	lck_sleep_action_t      lck_sleep_action,
	event_t                 event,
	wait_interrupt_t        interruptible,
	lck_grp_t               *grp);

extern wait_result_t    lck_spin_sleep_deadline(
	lck_spin_t              *lck,
	lck_sleep_action_t      lck_sleep_action,
	event_t                 event,
	wait_interrupt_t        interruptible,
	uint64_t                deadline);

#ifdef  KERNEL_PRIVATE

extern void             lck_spin_lock_nopreempt(
	lck_spin_t              *lck);

extern void             lck_spin_lock_nopreempt_grp(
	lck_spin_t              *lck, lck_grp_t *grp);

extern void             lck_spin_unlock_nopreempt(
	lck_spin_t              *lck);

extern boolean_t        lck_spin_try_lock_grp(
	lck_spin_t              *lck,
	lck_grp_t               *grp);

extern boolean_t        lck_spin_try_lock(
	lck_spin_t              *lck);

extern boolean_t        lck_spin_try_lock_nopreempt(
	lck_spin_t              *lck);

extern boolean_t        lck_spin_try_lock_nopreempt_grp(
	lck_spin_t              *lck,
	lck_grp_t               *grp);

                                                                     
extern boolean_t        kdp_lck_spin_is_acquired(
	lck_spin_t              *lck);

struct _lck_mtx_ext_;
extern void lck_mtx_init_ext(
	lck_mtx_t               *lck,
	struct _lck_mtx_ext_    *lck_ext,
	lck_grp_t               *grp,
	lck_attr_t              *attr);

#endif

#define decl_lck_mtx_data(class, name)     class lck_mtx_t name

extern lck_mtx_t        *lck_mtx_alloc_init(
	lck_grp_t               *grp,
	lck_attr_t              *attr);

extern void             lck_mtx_init(
	lck_mtx_t               *lck,
	lck_grp_t               *grp,
	lck_attr_t              *attr);
extern void             lck_mtx_lock(
	lck_mtx_t               *lck);

extern void             lck_mtx_unlock(
	lck_mtx_t               *lck);

extern void             lck_mtx_destroy(
	lck_mtx_t               *lck,
	lck_grp_t               *grp);

extern void             lck_mtx_free(
	lck_mtx_t               *lck,
	lck_grp_t               *grp);

extern wait_result_t    lck_mtx_sleep(
	lck_mtx_t               *lck,
	lck_sleep_action_t      lck_sleep_action,
	event_t                 event,
	wait_interrupt_t        interruptible);

extern wait_result_t    lck_mtx_sleep_deadline(
	lck_mtx_t               *lck,
	lck_sleep_action_t      lck_sleep_action,
	event_t                 event,
	wait_interrupt_t        interruptible,
	uint64_t                deadline);

#ifdef KERNEL_PRIVATE
  
                                      
  
                                                                                                         
                                                                                                                          
                                                       
                                                                                                    
  
        
                                                                                                                                                                     
                                                             
                            
                                                 
                                       
                             
  
                                                                                                     
                                                  
                                                                                                                       
                                              
  
                               
   
extern wait_result_t lck_spin_sleep_with_inheritor(lck_spin_t *lock, lck_sleep_action_t lck_sleep_action, event_t event, thread_t inheritor, wait_interrupt_t interruptible, uint64_t deadline);

  
                                     
  
                                                                                                         
                                                                                                                          
                                                       
                                                                                                    
  
        
                                                                                                                                                                    
                                                                                                    
                            
                                                 
                                       
                             
  
                                                                                                     
                                                  
                                                                                                                       
                                              
  
                               
   
extern wait_result_t lck_mtx_sleep_with_inheritor(lck_mtx_t *lock, lck_sleep_action_t lck_sleep_action, event_t event, thread_t inheritor, wait_interrupt_t interruptible, uint64_t deadline);

  
                                     
  
                                                                                                         
                                                                                                                          
                                                       
                                                                                                    
  
        
                                                                                                                                                                   
                                                                                  
                            
                                                 
                                       
                             
  
                                                                                                     
                                                  
                                                                                                                       
                                              
  
                               
   
extern wait_result_t lck_rw_sleep_with_inheritor(lck_rw_t *lock, lck_sleep_action_t lck_sleep_action, event_t event, thread_t inheritor, wait_interrupt_t interruptible, uint64_t deadline);

  
                                  
  
                                                                                                                                         
                                                                                                          
  
        
                              
                                                      
                                                  
  
                                                                                
  
                                                                                                                         
                                              
                                                              
                                                                  
   
extern kern_return_t wakeup_one_with_inheritor(event_t event, wait_result_t result, lck_wake_action_t action, thread_t *thread_wokenup);

  
                                  
  
                                                                                            
  
        
                              
                                                       
  
                                                                                
  
                                                                  
   
extern kern_return_t wakeup_all_with_inheritor(event_t event, wait_result_t result);

  
                               
  
                                                                                                 
  
        
                                      
                                   
  
                                                                                
  
                                                                                                                                     
                                              
                                                                  
   
extern kern_return_t change_sleep_inheritor(event_t event, thread_t inheritor);

  
                 
   
typedef struct gate {
	uintptr_t gate_data;                                               
	struct turnstile *turnstile;                                  
} gate_t;

#define GATE_ILOCK_BIT   0
#define GATE_WAITERS_BIT 1

#define GATE_ILOCK (1 << GATE_ILOCK_BIT)
#define GATE_WAITERS (1 << GATE_WAITERS_BIT)

#define gate_ilock(gate) hw_lock_bit((hw_lock_bit_t*)(&(gate)->gate_data), GATE_ILOCK_BIT, LCK_GRP_NULL)
#define gate_iunlock(gate) hw_unlock_bit((hw_lock_bit_t*)(&(gate)->gate_data), GATE_ILOCK_BIT)
#define gate_has_waiters(state) ((state & GATE_WAITERS) != 0)
#define ordered_load_gate(gate) os_atomic_load(&(gate)->gate_data, compiler_acq_rel)
#define ordered_store_gate(gate, value)  os_atomic_store(&(gate)->gate_data, value, compiler_acq_rel)

#define GATE_THREAD_MASK (~(uintptr_t)(GATE_ILOCK | GATE_WAITERS))
#define GATE_STATE_TO_THREAD(state) (thread_t)(state & GATE_THREAD_MASK)
#define GATE_THREAD_TO_STATE(thread) ((uintptr_t)thread)

  
                                      
   
typedef int gate_wait_result_t;
#define GATE_HANDOFF       0
#define GATE_OPENED        1
#define GATE_TIMED_OUT     2
#define GATE_INTERRUPTED   3

  
                                 
   
#define GATE_ASSERT_CLOSED         0
#define GATE_ASSERT_OPEN           1
#define GATE_ASSERT_HELD           2

  
                                  
   
#define GATE_HANDOFF_DEFAULT                    0
#define GATE_HANDOFF_OPEN_IF_NO_WAITERS         1

#define GATE_EVENT(gate)     ((event_t) gate)
#define EVENT_TO_GATE(event) ((gate_t *) event)

  
                              
  
                                                                      
                                                                                                     
                                                                                                            
                         
                                                                                                                 
                                        
  
                                                                                                         
                                                              
  
                                                                     
  
        
                         
                         
   
#define decl_lck_rw_gate_data(class, name)                              class gate_t name

  
                         
  
                                                                           
  
        
                                                  
                                                                        
   
extern void lck_rw_gate_init(lck_rw_t *lock, gate_t *gate);

  
                            
  
                                                           
  
        
                                                  
                                                                        
   
extern void lck_rw_gate_destroy(lck_rw_t *lock, gate_t *gate);

  
                              
  
                                        
                                                                    
                                       
  
        
                                                  
                                                                        
  
                                                             
  
           
                                                                                                       
                        
                                                                                              
                                                                                  
                                                                                                         
                                                                                
  
                                                                                                                                  
                                                                                                              
                                                                              
                                                                                     
   
extern kern_return_t lck_rw_gate_try_close(lck_rw_t *lock, gate_t *gate);

  
                          
  
                                                                  
                                                                                 
                                                                                                  
                                                                                      
  
        
                                                  
                                                                        
  
                                                             
                                     
  
   
extern void lck_rw_gate_close(lck_rw_t *lock, gate_t *gate);


  
                         
  
                                                             
  
        
                                                  
                                                                        
  
                                                             
                                                                 
  
   
extern void lck_rw_gate_open(lck_rw_t *lock, gate_t *gate);

  
                            
  
                                                                                          
                                                                                      
                                                                       
                                                                                            
                                 
                                                                                                           
                                           
  
  
        
                                                  
                                                                        
                                                                          
  
                                                             
                                                                 
  
           
                                                                          
                                                           
  
   
extern kern_return_t lck_rw_gate_handoff(lck_rw_t *lock, gate_t *gate, int flags);

  
                          
  
                                                                                        
                                       
                                                                                                  
                                                                                      
                                                                                                     
                        
  
  
        
                                                  
                                                                        
  
                                                             
                                                                                             
  
   
extern void lck_rw_gate_steal(lck_rw_t *lock, gate_t *gate);

  
                         
  
                                                                                        
                                                                                        
                                                
  
        
                                                  
                                                                        
                                                                                  
                                       
                   
  
                                                                                                     
                                                  
                                       
  
                                               
                                                                                       
                                                                                                             
                                                                                                 
                                                            
                                                                  
                                                                         
  
   
extern gate_wait_result_t lck_rw_gate_wait(lck_rw_t *lock, gate_t *gate, lck_sleep_action_t lck_sleep_action, wait_interrupt_t interruptible, uint64_t deadline);

  
                           
  
                                                                
  
        
                                                  
                                                                        
                                          
                                                            
                                                          
                                                                                               
   
extern void lck_rw_gate_assert(lck_rw_t *lock, gate_t *gate, int flags);

  
                               
  
                                                                      
                                                                                                     
                                                                                                            
                         
                                                                                                                 
                                        
  
                                                                                                          
                                                               
  
                                                                      
  
        
                         
                         
   
#define decl_lck_mtx_gate_data(class, name)                             class gate_t name

  
                          
  
                                                                            
  
        
                                                   
                                                                         
   
extern void lck_mtx_gate_init(lck_mtx_t *lock, gate_t *gate);

  
                             
  
                                                          
  
        
                                                   
                                                                         
   
extern void lck_mtx_gate_destroy(lck_mtx_t *lock, gate_t *gate);

  
                               
  
                                        
                                                                    
                                       
  
        
                                                   
                                                                         
  
                                                             
  
           
                                                                                                       
                        
                                                                                                
                                                                                  
                                                                                                         
                                                                                  
  
                                                                                                                                  
                                                                                                               
                                                                                
                                                                                     
   
extern kern_return_t lck_mtx_gate_try_close(lck_mtx_t *lock, gate_t *gate);

  
                           
  
                                                                  
                                                                                 
                                                                                                    
                                                                                      
  
        
                                                   
                                                                         
  
                                                             
                                     
  
   
extern void lck_mtx_gate_close(lck_mtx_t *lock, gate_t *gate);

  
                          
  
                                                                
  
        
                                                   
                                                                         
  
                                                             
                                                                 
  
   
extern void lck_mtx_gate_open(lck_mtx_t *lock, gate_t *gate);

  
                             
  
                                                                                    
                                                                                      
                                                                       
                                                                                            
                                 
                                                                                           
                                           
  
  
        
                                                   
                                                                         
                                                   
  
                                                             
                                                                 
  
           
                                                                          
                                                           
  
   
extern kern_return_t lck_mtx_gate_handoff(lck_mtx_t *lock, gate_t *gate, int flags);

  
                           
  
                                                                                   
                                       
                                                                                                    
                                                                                      
                                                                                                       
                        
  
  
        
                                                   
                                                                         
  
                                                             
                                                                                             
  
   
extern void lck_mtx_gate_steal(lck_mtx_t *lock, gate_t *gate);

  
                          
  
                                                                                        
                                                                                        
                                                
  
        
                                                   
                                                                         
                                                                                                    
                                       
                   
  
                                                                                                     
                                                  
                                       
  
                                               
                                                                                       
                                                                                                               
                                                                                                 
                                                            
                                                                  
                                                                         
  
   
extern gate_wait_result_t lck_mtx_gate_wait(lck_mtx_t *lock, gate_t *gate, lck_sleep_action_t lck_sleep_action, wait_interrupt_t interruptible, uint64_t deadline);

  
                            
  
                                                                
  
        
                                                   
                                                                         
                                          
                                                            
                                                          
                                                                                               
   
extern void lck_mtx_gate_assert(lck_mtx_t *lock, gate_t *gate, int flags);


#endif                 

#if DEVELOPMENT || DEBUG
#define FULL_CONTENDED 0
#define HALF_CONTENDED 1
#define MAX_CONDENDED  2

extern void             erase_all_test_mtx_stats(void);
extern int              get_test_mtx_stats_string(char* buffer, int buffer_size);
extern void             lck_mtx_test_init(void);
extern void             lck_mtx_test_lock(void);
extern void             lck_mtx_test_unlock(void);
extern int              lck_mtx_test_mtx_uncontended(int iter, char* buffer, int buffer_size);
extern int              lck_mtx_test_mtx_contended(int iter, char* buffer, int buffer_size, int type);
extern int              lck_mtx_test_mtx_uncontended_loop_time(int iter, char* buffer, int buffer_size);
extern int              lck_mtx_test_mtx_contended_loop_time(int iter, char* buffer, int buffer_size, int type);
#endif
#ifdef  KERNEL_PRIVATE

extern boolean_t        lck_mtx_try_lock(
	lck_mtx_t               *lck);

extern void             mutex_pause(uint32_t);

extern void             lck_mtx_yield(
	lck_mtx_t               *lck);

extern boolean_t        lck_mtx_try_lock_spin(
	lck_mtx_t               *lck);

extern void             lck_mtx_lock_spin(
	lck_mtx_t               *lck);

extern boolean_t        kdp_lck_mtx_lock_spin_is_acquired(
	lck_mtx_t               *lck);

extern void             lck_mtx_convert_spin(
	lck_mtx_t               *lck);

extern void             lck_mtx_lock_spin_always(
	lck_mtx_t               *lck);

extern boolean_t        lck_mtx_try_lock_spin_always(
	lck_mtx_t               *lck);

#define lck_mtx_unlock_always(l)        lck_mtx_unlock(l)

extern void             lck_spin_assert(
	lck_spin_t              *lck,
	unsigned                int    type);

extern boolean_t        kdp_lck_rw_lock_is_acquired_exclusive(
	lck_rw_t                *lck);

#endif                      

extern void             lck_mtx_assert(
	lck_mtx_t               *lck,
	unsigned                int    type);

#if MACH_ASSERT
#define LCK_MTX_ASSERT(lck, type) lck_mtx_assert((lck),(type))
#define LCK_SPIN_ASSERT(lck, type) lck_spin_assert((lck),(type))
#define LCK_RW_ASSERT(lck, type) lck_rw_assert((lck),(type))
#else                  
#define LCK_MTX_ASSERT(lck, type)
#define LCK_SPIN_ASSERT(lck, type)
#define LCK_RW_ASSERT(lck, type)
#endif                  

#if DEBUG
#define LCK_MTX_ASSERT_DEBUG(lck, type) lck_mtx_assert((lck),(type))
#define LCK_SPIN_ASSERT_DEBUG(lck, type) lck_spin_assert((lck),(type))
#define LCK_RW_ASSERT_DEBUG(lck, type) lck_rw_assert((lck),(type))
#else            
#define LCK_MTX_ASSERT_DEBUG(lck, type)
#define LCK_SPIN_ASSERT_DEBUG(lck, type)
#define LCK_RW_ASSERT_DEBUG(lck, type)
#endif            

#define LCK_ASSERT_OWNED                1
#define LCK_ASSERT_NOTOWNED             2

#define LCK_MTX_ASSERT_OWNED    LCK_ASSERT_OWNED
#define LCK_MTX_ASSERT_NOTOWNED LCK_ASSERT_NOTOWNED

#ifdef  MACH_KERNEL_PRIVATE
struct turnstile;
extern void             lck_mtx_lock_wait(
	lck_mtx_t               *lck,
	thread_t                holder,
	struct turnstile        **ts);

extern int              lck_mtx_lock_acquire(
	lck_mtx_t               *lck,
	struct turnstile        *ts);

extern  boolean_t       lck_mtx_unlock_wakeup(
	lck_mtx_t               *lck,
	thread_t                holder);

extern boolean_t        lck_mtx_ilk_unlock(
	lck_mtx_t               *lck);

extern boolean_t        lck_mtx_ilk_try_lock(
	lck_mtx_t               *lck);

extern void lck_mtx_wakeup_adjust_pri(thread_t thread, integer_t priority);

#endif

#define decl_lck_rw_data(class, name)     class lck_rw_t name

typedef unsigned int     lck_rw_type_t;

#define LCK_RW_TYPE_SHARED                      0x01
#define LCK_RW_TYPE_EXCLUSIVE           0x02

#ifdef XNU_KERNEL_PRIVATE
#define LCK_RW_ASSERT_SHARED    0x01
#define LCK_RW_ASSERT_EXCLUSIVE 0x02
#define LCK_RW_ASSERT_HELD              0x03
#define LCK_RW_ASSERT_NOTHELD   0x04
#endif

extern lck_rw_t         *lck_rw_alloc_init(
	lck_grp_t               *grp,
	lck_attr_t              *attr);

extern void             lck_rw_init(
	lck_rw_t                *lck,
	lck_grp_t               *grp,
	lck_attr_t              *attr);

extern void             lck_rw_lock(
	lck_rw_t                *lck,
	lck_rw_type_t           lck_rw_type);

extern void             lck_rw_unlock(
	lck_rw_t                *lck,
	lck_rw_type_t           lck_rw_type);

extern void             lck_rw_lock_shared(
	lck_rw_t                *lck);

extern void             lck_rw_unlock_shared(
	lck_rw_t                *lck);

extern boolean_t        lck_rw_lock_yield_shared(
	lck_rw_t                *lck,
	boolean_t               force_yield);

extern void             lck_rw_lock_exclusive(
	lck_rw_t                *lck);
  
                            
                                                                  
                    
  
                                                                   
              
  
                                
                                                                      
                  
   
extern bool                             lck_rw_lock_exclusive_check_contended(
	lck_rw_t                *lck);

extern void             lck_rw_unlock_exclusive(
	lck_rw_t                *lck);

#ifdef  XNU_KERNEL_PRIVATE
  
          
                                                                          
                                                                               
   
extern void             lck_rw_assert(
	lck_rw_t                *lck,
	unsigned int            type);

extern void lck_rw_clear_promotion(thread_t thread, uintptr_t trace_obj);
extern void lck_rw_set_promotion_locked(thread_t thread);

uintptr_t unslide_for_kdebug(void* object);
#endif                         

#ifdef  KERNEL_PRIVATE

extern lck_rw_type_t    lck_rw_done(
	lck_rw_t                *lck);
#endif

extern void             lck_rw_destroy(
	lck_rw_t                *lck,
	lck_grp_t               *grp);

extern void             lck_rw_free(
	lck_rw_t                *lck,
	lck_grp_t               *grp);

extern wait_result_t    lck_rw_sleep(
	lck_rw_t                *lck,
	lck_sleep_action_t      lck_sleep_action,
	event_t                 event,
	wait_interrupt_t        interruptible);

extern wait_result_t    lck_rw_sleep_deadline(
	lck_rw_t                *lck,
	lck_sleep_action_t      lck_sleep_action,
	event_t                 event,
	wait_interrupt_t        interruptible,
	uint64_t                deadline);

extern boolean_t        lck_rw_lock_shared_to_exclusive(
	lck_rw_t                *lck);

extern void             lck_rw_lock_exclusive_to_shared(
	lck_rw_t                *lck);

extern boolean_t        lck_rw_try_lock(
	lck_rw_t                *lck,
	lck_rw_type_t           lck_rw_type);

#ifdef  KERNEL_PRIVATE

extern boolean_t        lck_rw_try_lock_shared(
	lck_rw_t                *lck);

extern boolean_t        lck_rw_try_lock_exclusive(
	lck_rw_t                *lck);

#endif
#if XNU_KERNEL_PRIVATE

struct lck_attr_startup_spec {
	lck_attr_t              *lck_attr;
	uint32_t                lck_attr_set_flags;
	uint32_t                lck_attr_clear_flags;
};

struct lck_spin_startup_spec {
	lck_spin_t              *lck;
	lck_grp_t               *lck_grp;
	lck_attr_t              *lck_attr;
};

struct lck_mtx_startup_spec {
	lck_mtx_t               *lck;
	struct _lck_mtx_ext_    *lck_ext;
	lck_grp_t               *lck_grp;
	lck_attr_t              *lck_attr;
};

struct lck_rw_startup_spec {
	lck_rw_t                *lck;
	lck_grp_t               *lck_grp;
	lck_attr_t              *lck_attr;
};

extern void             lck_attr_startup_init(
	struct lck_attr_startup_spec *spec);

extern void             lck_spin_startup_init(
	struct lck_spin_startup_spec *spec);

extern void             lck_mtx_startup_init(
	struct lck_mtx_startup_spec *spec);

extern void             lck_rw_startup_init(
	struct lck_rw_startup_spec *spec);

  
                                       
                                       
  
                                                                 
                                                                
                                                      
  
                                    
                                                                
                              
                                 
                                            
  
                                                             
                                                                   
                                   
   
#define LCK_ATTR_DECLARE(var, set_flags, clear_flags) \
	SECURITY_READ_ONLY_LATE(lck_attr_t) var; \
	static __startup_data struct lck_attr_startup_spec \
	__startup_lck_attr_spec_ ## var = { &var, set_flags, clear_flags }; \
	STARTUP_ARG(LOCKS_EARLY, STARTUP_RANK_SECOND, lck_attr_startup_init, \
	    &__startup_lck_attr_spec_ ## var)

#define LCK_SPIN_DECLARE_ATTR(var, grp, attr) \
	lck_spin_t var; \
	static __startup_data struct lck_spin_startup_spec \
	__startup_lck_spin_spec_ ## var = { &var, grp, attr }; \
	STARTUP_ARG(LOCKS_EARLY, STARTUP_RANK_FOURTH, lck_spin_startup_init, \
	    &__startup_lck_spin_spec_ ## var)

#define LCK_SPIN_DECLARE(var, grp) \
	LCK_SPIN_DECLARE_ATTR(var, grp, LCK_ATTR_NULL)

#define LCK_MTX_DECLARE_ATTR(var, grp, attr) \
	lck_mtx_t var; \
	static __startup_data struct lck_mtx_startup_spec \
	__startup_lck_mtx_spec_ ## var = { &var, NULL, grp, attr }; \
	STARTUP_ARG(LOCKS, STARTUP_RANK_FIRST, lck_mtx_startup_init, \
	    &__startup_lck_mtx_spec_ ## var)

#define LCK_MTX_DECLARE(var, grp) \
	LCK_MTX_DECLARE_ATTR(var, grp, LCK_ATTR_NULL)

#define LCK_MTX_EARLY_DECLARE_ATTR(var, grp, attr) \
	lck_mtx_ext_t var ## _ext; \
	lck_mtx_t var; \
	static __startup_data struct lck_mtx_startup_spec \
	__startup_lck_mtx_spec_ ## var = { &var, &var ## _ext, grp, attr }; \
	STARTUP_ARG(LOCKS_EARLY, STARTUP_RANK_FOURTH, lck_mtx_startup_init, \
	    &__startup_lck_mtx_spec_ ## var)

#define LCK_MTX_EARLY_DECLARE(var, grp) \
	LCK_MTX_EARLY_DECLARE_ATTR(var, grp, LCK_ATTR_NULL)

#define LCK_RW_DECLARE_ATTR(var, grp, attr) \
	lck_rw_t var; \
	static __startup_data struct lck_rw_startup_spec \
	__startup_lck_rw_spec_ ## var = { &var, grp, attr }; \
	STARTUP_ARG(LOCKS_EARLY, STARTUP_RANK_FOURTH, lck_rw_startup_init, \
	    &__startup_lck_rw_spec_ ## var)

#define LCK_RW_DECLARE(var, grp) \
	LCK_RW_DECLARE_ATTR(var, grp, LCK_ATTR_NULL)

#endif                         

__END_DECLS

#endif                     
