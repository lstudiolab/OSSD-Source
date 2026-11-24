/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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

#define LOCK_PRIVATE 1

#include <mach_ldebug.h>

#include <kern/locks.h>
#include <kern/misc_protos.h>
#include <kern/thread.h>
#include <kern/processor.h>
#include <kern/cpu_data.h>
#include <kern/cpu_number.h>
#include <kern/sched_prim.h>
#include <kern/debug.h>
#include <string.h>

#include <i386/machine_routines.h>                                  
#include <machine/atomic.h>
#include <machine/machine_cpu.h>
#include <i386/mp.h>
#include <machine/atomic.h>
#include <sys/kdebug.h>
#include <i386/locks_i386_inlines.h>

  
                                                                  
                                                                                        
                                                                            
  
                                                               
                                                                               
                                                     
  
                                                                                          
                                           
  
                                                                                        
                                                                                              
                                                                                             
                                                                                   
                                                                          
  
                                                             
   

#if DEVELOPMENT || DEBUG
TUNABLE(bool, LckDisablePreemptCheck, "-disable_mtx_chk", false);

  
                                                             
                                                              
                                                               
                        
   
void __inline__
lck_mtx_check_preemption(void)
{
	if (get_preemption_level() == 0) {
		return;
	}
	if (LckDisablePreemptCheck) {
		return;
	}
	if (current_cpu_datap()->cpu_hibernate) {
		return;
	}

	panic("preemption_level(%d) != 0\n", get_preemption_level());
}

#else                           

void __inline__
lck_mtx_check_preemption(void)
{
	return;
}

#endif                           

  
                        
  
                                    
                                   
                                      
                 
  
                                                               
                                        
   
__attribute__((noinline))
void
lck_mtx_lock(
	lck_mtx_t       *lock)
{
	uint32_t prev, state;

	lck_mtx_check_preemption();
	state = ordered_load_mtx_state(lock);

	  
                                           
                                                        
                                                       
                              
    

	prev = state & ~(LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK | LCK_MTX_WAITERS_MSK);
	state = prev | LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK;

	disable_preemption();
	if (!os_atomic_cmpxchg(&lock->lck_mtx_state, prev, state, acquire)) {
		enable_preemption();
		return lck_mtx_lock_slow(lock);
	}

	                                                                

	thread_t thread = current_thread();
	                           
	ordered_store_mtx_owner(lock, (uintptr_t)thread);

#if MACH_LDEBUG
	if (thread) {
		thread->mutex_count++;                              
	}
#endif

	                                                
	lck_mtx_lock_finish_inline(lock, state, FALSE);
}

  
                            
  
                                          
                                   
                                      
                 
  
                                                               
  
                                                      
                                                               
   
__attribute__((noinline))
boolean_t
lck_mtx_try_lock(
	lck_mtx_t       *lock)
{
	uint32_t prev, state;

	state = ordered_load_mtx_state(lock);

	  
                                           
                                                        
                                                       
                              
    

	prev = state & ~(LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK | LCK_MTX_WAITERS_MSK);
	state = prev | LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK;

	disable_preemption();
	if (!os_atomic_cmpxchg(&lock->lck_mtx_state, prev, state, acquire)) {
		enable_preemption();
		return lck_mtx_try_lock_slow(lock);
	}

	                                                                

	thread_t thread = current_thread();
	                           
	ordered_store_mtx_owner(lock, (uintptr_t)thread);

#if MACH_LDEBUG
	if (thread) {
		thread->mutex_count++;                              
	}
#endif

	                                                
	lck_mtx_try_lock_finish_inline(lock, state);

	return TRUE;
}

  
                                    
  
                                                       
                                   
                                      
                 
  
                                                               
  
                                                       
                                                                           
  
                                                                
          
  
                                        
   
__attribute__((noinline))
void
lck_mtx_lock_spin_always(
	lck_mtx_t       *lock)
{
	uint32_t prev, state;

	state = ordered_load_mtx_state(lock);

	  
                                           
                                    
                               
                                                       
                              
    

	if (state & (LCK_MTX_ILOCKED_MSK | LCK_MTX_SPIN_MSK)) {
		return lck_mtx_lock_spin_slow(lock);
	}

	                                                                     
	prev = state & ~(LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK);
	state = prev | LCK_MTX_ILOCKED_MSK | LCK_MTX_SPIN_MSK;

	disable_preemption();
	if (!os_atomic_cmpxchg(&lock->lck_mtx_state, prev, state, acquire)) {
		enable_preemption();
		return lck_mtx_lock_spin_slow(lock);
	}

	                                                                            

	thread_t thread = current_thread();
	                           
	ordered_store_mtx_owner(lock, (uintptr_t)thread);

#if MACH_LDEBUG
	if (thread) {
		thread->mutex_count++;                              
	}
#endif

#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_MTX_LOCK_SPIN_ACQUIRE, lock, 0);
#endif
	                                                            
	return;
}

  
                             
  
                                                       
                                   
                                      
                 
  
                                                               
  
                                                       
                                                                           
  
                                                                
          
  
                                        
   
void
lck_mtx_lock_spin(
	lck_mtx_t       *lock)
{
	lck_mtx_check_preemption();
	lck_mtx_lock_spin_always(lock);
}

  
                                        
  
                                                       
                                   
                                      
                 
  
                                                               
  
                                                      
                                                                 
             
  
   
__attribute__((noinline))
boolean_t
lck_mtx_try_lock_spin_always(
	lck_mtx_t       *lock)
{
	uint32_t prev, state;

	state = ordered_load_mtx_state(lock);

	  
                                           
                                    
                               
                                                       
                              
    

	                                                                     
	prev = state & ~(LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK);
	state = prev | LCK_MTX_ILOCKED_MSK | LCK_MTX_SPIN_MSK;

	disable_preemption();
	if (!os_atomic_cmpxchg(&lock->lck_mtx_state, prev, state, acquire)) {
		enable_preemption();
		return lck_mtx_try_lock_spin_slow(lock);
	}

	                                                                            

	thread_t thread = current_thread();
	                           
	ordered_store_mtx_owner(lock, (uintptr_t)thread);

#if MACH_LDEBUG
	if (thread) {
		thread->mutex_count++;                              
	}
#endif

#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_MTX_TRY_SPIN_LOCK_ACQUIRE, lock, 0);
#endif

	                                                            
	return TRUE;
}

  
                                 
  
                                                       
                                   
                                      
                 
  
                                                               
  
                                                      
                                                                 
             
  
   
boolean_t
lck_mtx_try_lock_spin(
	lck_mtx_t       *lock)
{
	return lck_mtx_try_lock_spin_always(lock);
}

  
                              
  
                                          
                                          
                                                
               
  
                                                
                                  
   
__attribute__((noinline))
void
lck_mtx_unlock(
	lck_mtx_t       *lock)
{
	uint32_t prev, state;

	state = ordered_load_mtx_state(lock);

	if (state & LCK_MTX_SPIN_MSK) {
		return lck_mtx_unlock_slow(lock);
	}

	  
                                                 
                                                   
                                
                                     
                          
                                                         
    

	  
                    
                                                                
    
	prev = state & ~(LCK_MTX_ILOCKED_MSK | LCK_MTX_WAITERS_MSK);
	prev |= LCK_MTX_MLOCKED_MSK;

	state = prev | LCK_MTX_ILOCKED_MSK;
	state &= ~LCK_MTX_MLOCKED_MSK;

	disable_preemption();

	                                                                                
	if (!os_atomic_cmpxchg(&lock->lck_mtx_state, prev, state, acquire)) {
		enable_preemption();
		return lck_mtx_unlock_slow(lock);
	}

	                                                                

#if DEVELOPMENT | DEBUG
	thread_t owner = (thread_t)lock->lck_mtx_owner;
	if (__improbable(owner != current_thread())) {
		lck_mtx_owner_check_panic(lock);
	}
#endif

	                 
	ordered_store_mtx_owner(lock, 0);
	                       
	state &= ~LCK_MTX_ILOCKED_MSK;
	ordered_store_mtx_state_release(lock, state);

#if     MACH_LDEBUG
	thread_t thread = current_thread();
	if (thread) {
		thread->mutex_count--;
	}
#endif                   

	                          
	lck_mtx_unlock_finish_inline(lock, FALSE);
}
