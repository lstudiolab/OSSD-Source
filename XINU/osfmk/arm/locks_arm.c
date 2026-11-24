/*
 * Copyright (c) 2007-2018 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System Copyright (c) 1991,1990,1989,1988,1987 Carnegie
 * Mellon University All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright notice
 * and this permission notice appear in all copies of the software,
 * derivative works or modified versions, and any portions thereof, and that
 * both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION.
 * CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 * Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 * School of Computer Science Carnegie Mellon University Pittsburgh PA
 * 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon the
 * rights to redistribute these changes.
 */
  
                    
                                                    
             
  
                                    
   

#define LOCK_PRIVATE 1

#include <mach_ldebug.h>

#include <kern/zalloc.h>
#include <kern/lock_stat.h>
#include <kern/locks.h>
#include <kern/misc_protos.h>
#include <kern/thread.h>
#include <kern/processor.h>
#include <kern/sched_prim.h>
#include <kern/debug.h>
#include <kern/kcdata.h>
#include <string.h>
#include <arm/cpu_internal.h>
#include <os/hash.h>
#include <arm/cpu_data.h>

#include <arm/cpu_data_internal.h>
#include <arm/proc_reg.h>
#include <arm/smp.h>
#include <machine/atomic.h>
#include <machine/machine_cpu.h>

#include <sys/kdebug.h>

#if CONFIG_DTRACE
#define DTRACE_RW_SHARED        0x0             
#define DTRACE_RW_EXCL          0x1             
#define DTRACE_NO_FLAG          0x0                     
#endif                     

#define LCK_RW_LCK_EXCLUSIVE_CODE       0x100
#define LCK_RW_LCK_EXCLUSIVE1_CODE      0x101
#define LCK_RW_LCK_SHARED_CODE          0x102
#define LCK_RW_LCK_SH_TO_EX_CODE        0x103
#define LCK_RW_LCK_SH_TO_EX1_CODE       0x104
#define LCK_RW_LCK_EX_TO_SH_CODE        0x105


#define ANY_LOCK_DEBUG  (USLOCK_DEBUG || LOCK_DEBUG || MUTEX_DEBUG)

                                                   
                                                                   
#define LOCK_CORRECTNESS_PANIC() (kernel_debugger_entry_count == 0)

#define ADAPTIVE_SPIN_ENABLE 0x1

int lck_mtx_adaptive_spin_mode = ADAPTIVE_SPIN_ENABLE;

#define SPINWAIT_OWNER_CHECK_COUNT 4

typedef enum {
	SPINWAIT_ACQUIRED,                        
	SPINWAIT_INTERLOCK,                                                                                 
	SPINWAIT_DID_SPIN_HIGH_THR,                                                           
	SPINWAIT_DID_SPIN_OWNER_NOT_CORE,                                                           
	SPINWAIT_DID_SPIN_NO_WINDOW_CONTENTION,                                                           
	SPINWAIT_DID_SPIN_SLIDING_THR,                                                          
	SPINWAIT_DID_NOT_SPIN,                                       
} spinwait_result_t;

#if CONFIG_DTRACE
extern uint64_t dtrace_spin_threshold;
#endif

              

extern unsigned int not_in_kdp;

  
                                                     
                                                           
                                             
   
typedef void   *pc_t;
#define INVALID_PC      ((void *) VM_MAX_KERNEL_ADDRESS)
#define INVALID_THREAD  ((void *) VM_MAX_KERNEL_ADDRESS)

#ifdef  lint
  
                                                             
   
#define OBTAIN_PC(pc, l) ++pc
#else                                     
#define OBTAIN_PC(pc, l)
#endif                                    


  
                                                         
   

  
                                                   
   
#define LCK_MTX_SPIN_TAG  0xfffffff0


#define interlock_lock(lock)    hw_lock_bit    ((hw_lock_bit_t*)(&(lock)->lck_mtx_data), LCK_ILOCK_BIT, LCK_GRP_NULL)
#define interlock_try(lock)             hw_lock_bit_try((hw_lock_bit_t*)(&(lock)->lck_mtx_data), LCK_ILOCK_BIT, LCK_GRP_NULL)
#define interlock_unlock(lock)  hw_unlock_bit  ((hw_lock_bit_t*)(&(lock)->lck_mtx_data), LCK_ILOCK_BIT)
#define lck_rw_ilk_lock(lock)   hw_lock_bit  ((hw_lock_bit_t*)(&(lock)->lck_rw_tag), LCK_RW_INTERLOCK_BIT, LCK_GRP_NULL)
#define lck_rw_ilk_unlock(lock) hw_unlock_bit((hw_lock_bit_t*)(&(lock)->lck_rw_tag), LCK_RW_INTERLOCK_BIT)

#define load_memory_barrier()   os_atomic_thread_fence(acquire)

                                             
#define ordered_load(target) \
	        os_atomic_load(target, compiler_acq_rel)
#define ordered_store(target, value) \
	        os_atomic_store(target, value, compiler_acq_rel)

#define ordered_load_mtx(lock)                  ordered_load(&(lock)->lck_mtx_data)
#define ordered_store_mtx(lock, value)  ordered_store(&(lock)->lck_mtx_data, (value))
#define ordered_load_rw(lock)                   ordered_load(&(lock)->lck_rw_data)
#define ordered_store_rw(lock, value)   ordered_store(&(lock)->lck_rw_data, (value))
#define ordered_load_rw_owner(lock)             ordered_load(&(lock)->lck_rw_owner)
#define ordered_store_rw_owner(lock, value)     ordered_store(&(lock)->lck_rw_owner, (value))
#define ordered_load_hw(lock)                   ordered_load(&(lock)->lock_data)
#define ordered_store_hw(lock, value)   ordered_store(&(lock)->lock_data, (value))
#define ordered_load_bit(lock)                  ordered_load((lock))
#define ordered_store_bit(lock, value)  ordered_store((lock), (value))


                                                                     
#define compiler_memory_fence() __asm__ volatile ("" ::: "memory")

#define LOCK_PANIC_TIMEOUT      0xc00000
#define NOINLINE                __attribute__((noinline))


#if __arm__
#define interrupts_disabled(mask) (mask & PSR_INTMASK)
#else
#define interrupts_disabled(mask) (mask & DAIF_IRQF)
#endif


#if __arm__
#define enable_fiq()            __asm__ volatile ("cpsie  f" ::: "memory");
#define enable_interrupts()     __asm__ volatile ("cpsie if" ::: "memory");
#endif

ZONE_VIEW_DEFINE(ZV_LCK_SPIN, "lck_spin",
    KHEAP_ID_DEFAULT, sizeof(lck_spin_t));

ZONE_VIEW_DEFINE(ZV_LCK_MTX, "lck_mtx",
    KHEAP_ID_DEFAULT, sizeof(lck_mtx_t));

ZONE_VIEW_DEFINE(ZV_LCK_MTX_EXT, "lck_mtx_ext",
    KHEAP_ID_DEFAULT, sizeof(lck_mtx_ext_t));

ZONE_VIEW_DEFINE(ZV_LCK_RW, "lck_rw",
    KHEAP_ID_DEFAULT, sizeof(lck_rw_t));

  
                       
   

static void lck_rw_lock_shared_gen(lck_rw_t *lck);
static void lck_rw_lock_exclusive_gen(lck_rw_t *lck);
static boolean_t lck_rw_lock_shared_to_exclusive_success(lck_rw_t *lck);
static boolean_t lck_rw_lock_shared_to_exclusive_failure(lck_rw_t *lck, uint32_t prior_lock_state);
static void lck_rw_lock_exclusive_to_shared_gen(lck_rw_t *lck, uint32_t prior_lock_state);
static lck_rw_type_t lck_rw_done_gen(lck_rw_t *lck, uint32_t prior_lock_state);
static boolean_t lck_rw_grab(lck_rw_t *lock, int mode, boolean_t wait);

  
                                                                   
                                                                           
                                                                         
                                          
  
                                                                      
                                                    
                                                                                    
   
__unused static uint32_t
load_exclusive32(uint32_t *target, enum memory_order ord)
{
	uint32_t        value;

#if __arm__
	if (_os_atomic_mo_has_release(ord)) {
		                           
		atomic_thread_fence(memory_order_release);
	}
	value = __builtin_arm_ldrex(target);
#else
	if (_os_atomic_mo_has_acquire(ord)) {
		value = __builtin_arm_ldaex(target);            
	} else {
		value = __builtin_arm_ldrex(target);           
	}
#endif            
	return value;
}

__unused static boolean_t
store_exclusive32(uint32_t *target, uint32_t value, enum memory_order ord)
{
	boolean_t err;

#if __arm__
	err = __builtin_arm_strex(value, target);
	if (_os_atomic_mo_has_acquire(ord)) {
		                             
		atomic_thread_fence(memory_order_acquire);
	}
#else
	if (_os_atomic_mo_has_release(ord)) {
		err = __builtin_arm_stlex(value, target);               
	} else {
		err = __builtin_arm_strex(value, target);              
	}
#endif            
	return !err;
}

static uint32_t
atomic_exchange_begin32(uint32_t *target, uint32_t *previous, enum memory_order ord)
{
	uint32_t        val;

#if __ARM_ATOMICS_8_1
	ord = memory_order_relaxed;
#endif
	val = load_exclusive32(target, ord);
	*previous = val;
	return val;
}

static boolean_t
atomic_exchange_complete32(uint32_t *target, uint32_t previous, uint32_t newval, enum memory_order ord)
{
#if __ARM_ATOMICS_8_1
	return __c11_atomic_compare_exchange_strong((_Atomic uint32_t *)target, &previous, newval, ord, memory_order_relaxed);
#else
	(void)previous;                                                
	return store_exclusive32(target, newval, ord);
#endif
}

static void
atomic_exchange_abort(void)
{
	os_atomic_clear_exclusive();
}

static boolean_t
atomic_test_and_set32(uint32_t *target, uint32_t test_mask, uint32_t set_mask, enum memory_order ord, boolean_t wait)
{
	uint32_t                value, prev;

	for (;;) {
		value = atomic_exchange_begin32(target, &prev, ord);
		if (value & test_mask) {
			if (wait) {
				wait_for_event();                                
			} else {
				atomic_exchange_abort();                                  
			}
			return FALSE;
		}
		value |= set_mask;
		if (atomic_exchange_complete32(target, prev, value, ord)) {
			return TRUE;
		}
	}
}

inline boolean_t
hw_atomic_test_and_set32(uint32_t *target, uint32_t test_mask, uint32_t set_mask, enum memory_order ord, boolean_t wait)
{
	return atomic_test_and_set32(target, test_mask, set_mask, ord, wait);
}

  
                                                            
                                                             
                                                                     
   
__abortlike
static void
_disable_preemption_overflow(void)
{
	panic("Preemption count overflow");
}

void
_disable_preemption(void)
{
	thread_t     thread = current_thread();
	unsigned int count  = thread->machine.preemption_count;

	if (__improbable(++count == 0)) {
		_disable_preemption_overflow();
	}

	os_atomic_store(&thread->machine.preemption_count, count, compiler_acq_rel);
}

  
                                                              
  
                                                                              
                                                                               
                                                  
  
                                                                               
                                                                                
                              
  
                                                                          
                                                                        
                                            
   
static NOINLINE void
kernel_preempt_check(thread_t thread)
{
	cpu_data_t *cpu_data_ptr;
	long        state;

#if __arm__
#define INTERRUPT_MASK PSR_IRQF
#else             
#define INTERRUPT_MASK DAIF_IRQF
#endif            

	  
                                                                          
                                                            
    
	cpu_data_ptr = os_atomic_load(&thread->machine.CpuDatap, compiler_acq_rel);
	if (__probable((cpu_data_ptr->cpu_pending_ast & AST_URGENT) == 0)) {
		return;
	}

	                                                         
	state = get_interrupts();
	if ((state & INTERRUPT_MASK) == 0) {
		disable_interrupts_noread();                                         

		  
                                                                    
                                                                          
     
		cpu_data_ptr = os_atomic_load(&thread->machine.CpuDatap, compiler_acq_rel);
		if (thread->machine.CpuDatap->cpu_pending_ast & AST_URGENT) {
#if __arm__
#if __ARM_USER_PROTECT__
			uintptr_t up = arm_user_protect_begin(thread);
#endif                         
			enable_fiq();
#endif            
			ast_taken_kernel();                                     
#if __arm__
#if __ARM_USER_PROTECT__
			arm_user_protect_end(thread, up, TRUE);
#endif                         
			enable_interrupts();
			return;                                                                            
#endif            
		}
		restore_interrupts(state);                                  
	}
}

  
                                                           
                                                             
                                                                     
   
__abortlike
static void
_enable_preemption_underflow(void)
{
	panic("Preemption count underflow");
}

void
_enable_preemption(void)
{
	thread_t     thread = current_thread();
	unsigned int count  = thread->machine.preemption_count;

	if (__improbable(count == 0)) {
		_enable_preemption_underflow();
	}
	count -= 1;

	os_atomic_store(&thread->machine.preemption_count, count, compiler_acq_rel);
	if (count == 0) {
		kernel_preempt_check(thread);
	}

	os_compiler_barrier();
}

int
get_preemption_level(void)
{
	return current_thread()->machine.preemption_count;
}

  
                                           
   
lck_spin_t     *
lck_spin_alloc_init(
	lck_grp_t * grp,
	lck_attr_t * attr)
{
	lck_spin_t *lck;

	lck = zalloc(ZV_LCK_SPIN);
	lck_spin_init(lck, grp, attr);
	return lck;
}

  
                                     
   
void
lck_spin_free(
	lck_spin_t * lck,
	lck_grp_t * grp)
{
	lck_spin_destroy(lck, grp);
	zfree(ZV_LCK_SPIN, lck);
}

  
                                     
   
void
lck_spin_init(
	lck_spin_t * lck,
	lck_grp_t * grp,
	__unused lck_attr_t * attr)
{
	lck->type = LCK_SPIN_TYPE;
	hw_lock_init(&lck->hwlock);
	if (grp) {
		lck_grp_reference(grp);
		lck_grp_lckcnt_incr(grp, LCK_TYPE_SPIN);
	}
}

  
                                                                 
   
MARK_AS_HIBERNATE_TEXT void inline
arm_usimple_lock_init(simple_lock_t lck, __unused unsigned short initial_value)
{
	lck->type = LCK_SPIN_TYPE;
	hw_lock_init(&lck->hwlock);
}


  
                                     
   
void
lck_spin_lock(lck_spin_t *lock)
{
#if     DEVELOPMENT || DEBUG
	if (lock->type != LCK_SPIN_TYPE) {
		panic("Invalid spinlock %p", lock);
	}
#endif                         
	hw_lock_lock(&lock->hwlock, LCK_GRP_NULL);
}

void
lck_spin_lock_grp(lck_spin_t *lock, lck_grp_t *grp)
{
#pragma unused(grp)
#if     DEVELOPMENT || DEBUG
	if (lock->type != LCK_SPIN_TYPE) {
		panic("Invalid spinlock %p", lock);
	}
#endif                         
	hw_lock_lock(&lock->hwlock, grp);
}

  
                                               
   
void
lck_spin_lock_nopreempt(lck_spin_t *lock)
{
#if     DEVELOPMENT || DEBUG
	if (lock->type != LCK_SPIN_TYPE) {
		panic("Invalid spinlock %p", lock);
	}
#endif                         
	hw_lock_lock_nopreempt(&lock->hwlock, LCK_GRP_NULL);
}

void
lck_spin_lock_nopreempt_grp(lck_spin_t *lock, lck_grp_t *grp)
{
#pragma unused(grp)
#if     DEVELOPMENT || DEBUG
	if (lock->type != LCK_SPIN_TYPE) {
		panic("Invalid spinlock %p", lock);
	}
#endif                         
	hw_lock_lock_nopreempt(&lock->hwlock, grp);
}

  
                                         
   
int
lck_spin_try_lock(lck_spin_t *lock)
{
	return hw_lock_try(&lock->hwlock, LCK_GRP_NULL);
}

int
lck_spin_try_lock_grp(lck_spin_t *lock, lck_grp_t *grp)
{
#pragma unused(grp)
	return hw_lock_try(&lock->hwlock, grp);
}

  
                                                   
   
int
lck_spin_try_lock_nopreempt(lck_spin_t *lock)
{
	return hw_lock_try_nopreempt(&lock->hwlock, LCK_GRP_NULL);
}

int
lck_spin_try_lock_nopreempt_grp(lck_spin_t *lock, lck_grp_t *grp)
{
#pragma unused(grp)
	return hw_lock_try_nopreempt(&lock->hwlock, grp);
}

  
                                       
   
void
lck_spin_unlock(lck_spin_t *lock)
{
#if     DEVELOPMENT || DEBUG
	if ((LCK_MTX_STATE_TO_THREAD(lock->lck_spin_data) != current_thread()) && LOCK_CORRECTNESS_PANIC()) {
		panic("Spinlock not owned by thread %p = %lx", lock, lock->lck_spin_data);
	}
	if (lock->type != LCK_SPIN_TYPE) {
		panic("Invalid spinlock type %p", lock);
	}
#endif                         
	hw_lock_unlock(&lock->hwlock);
}

  
                                                 
   
void
lck_spin_unlock_nopreempt(lck_spin_t *lock)
{
#if     DEVELOPMENT || DEBUG
	if ((LCK_MTX_STATE_TO_THREAD(lock->lck_spin_data) != current_thread()) && LOCK_CORRECTNESS_PANIC()) {
		panic("Spinlock not owned by thread %p = %lx", lock, lock->lck_spin_data);
	}
	if (lock->type != LCK_SPIN_TYPE) {
		panic("Invalid spinlock type %p", lock);
	}
#endif                         
	hw_lock_unlock_nopreempt(&lock->hwlock);
}

  
                                        
   
void
lck_spin_destroy(
	lck_spin_t * lck,
	lck_grp_t * grp)
{
	if (lck->lck_spin_data == LCK_SPIN_TAG_DESTROYED) {
		return;
	}
	lck->lck_spin_data = LCK_SPIN_TAG_DESTROYED;
	if (grp) {
		lck_grp_lckcnt_decr(grp, LCK_TYPE_SPIN);
		lck_grp_deallocate(grp);
	}
}

  
                                    
                                                                  
   
boolean_t
kdp_lck_spin_is_acquired(lck_spin_t *lck)
{
	if (not_in_kdp) {
		panic("panic: spinlock acquired check done outside of kernel debugger");
	}
	return ((lck->lck_spin_data & ~LCK_SPIN_TAG_DESTROYED) != 0) ? TRUE:FALSE;
}

  
                             
  
                                 
   
void
usimple_lock_init(
	usimple_lock_t l,
	unsigned short tag)
{
	simple_lock_init((simple_lock_t) l, tag);
}


  
                          
  
                                          
                                                
                                
   
void
(usimple_lock)(
	usimple_lock_t l
	LCK_GRP_ARG(lck_grp_t *grp))
{
	simple_lock((simple_lock_t) l, LCK_GRP_PROBEARG(grp));
}


extern void     sync(void);

  
                          
  
                                         
                                                
                                
   
void
(usimple_unlock)(
	usimple_lock_t l)
{
	simple_unlock((simple_lock_t)l);
}


  
                                        
  
                                                
                                                        
                                                         
                                                    
  
                                                        
                                                         
                                            
   
unsigned
int
(usimple_lock_try)(
	usimple_lock_t l
	LCK_GRP_ARG(lck_grp_t *grp))
{
	return simple_lock_try((simple_lock_t) l, grp);
}

  
                                                       
   

  
                                            
                                              
   
static inline uint64_t
lck_rw_deadline_for_spin(lck_rw_t *lck)
{
	lck_rw_word_t   word;

	word.data = ordered_load_rw(lck);
	if (word.can_sleep) {
		if (word.r_waiting || word.w_waiting || (word.shared_count > machine_info.max_cpus)) {
			  
                                                            
                                                                    
                                                                                 
          
                                                                                      
                                                                                           
                                                                                   
                      
      
			return mach_absolute_time();
		}
		return mach_absolute_time() + MutexSpin;
	} else {
		return mach_absolute_time() + (100000LL * 1000000000LL);
	}
}

static boolean_t
lck_rw_drain_status(lck_rw_t *lock, uint32_t status_mask, boolean_t wait __unused)
{
	uint64_t        deadline = 0;
	uint32_t        data;

	if (wait) {
		deadline = lck_rw_deadline_for_spin(lock);
	}

	for (;;) {
		data = load_exclusive32(&lock->lck_rw_data, memory_order_acquire_smp);
		if ((data & status_mask) == 0) {
			break;
		}
		if (wait) {
			wait_for_event();
		} else {
			os_atomic_clear_exclusive();
		}
		if (!wait || (mach_absolute_time() >= deadline)) {
			return FALSE;
		}
	}
	os_atomic_clear_exclusive();
	return TRUE;
}

  
                                
   
static inline void
lck_rw_interlock_spin(lck_rw_t *lock)
{
	uint32_t        data;

	for (;;) {
		data = load_exclusive32(&lock->lck_rw_data, memory_order_relaxed);
		if (data & LCK_RW_INTERLOCK) {
			wait_for_event();
		} else {
			os_atomic_clear_exclusive();
			return;
		}
	}
}

  
                                                                     
                                         
                                                                             
   
static inline boolean_t
lck_interlock_lock(lck_rw_t *lck)
{
	boolean_t       istate;

	istate = ml_set_interrupts_enabled(FALSE);
	lck_rw_ilk_lock(lck);
	return istate;
}

static inline void
lck_interlock_unlock(lck_rw_t *lck, boolean_t istate)
{
	lck_rw_ilk_unlock(lck);
	ml_set_interrupts_enabled(istate);
}


#define LCK_RW_GRAB_WANT        0
#define LCK_RW_GRAB_SHARED      1

static boolean_t
lck_rw_grab(lck_rw_t *lock, int mode, boolean_t wait)
{
	uint64_t        deadline = 0;
	uint32_t        data, prev;
	boolean_t       do_exch;

	if (wait) {
		deadline = lck_rw_deadline_for_spin(lock);
	}

	for (;;) {
		data = atomic_exchange_begin32(&lock->lck_rw_data, &prev, memory_order_acquire_smp);
		if (data & LCK_RW_INTERLOCK) {
			atomic_exchange_abort();
			lck_rw_interlock_spin(lock);
			continue;
		}
		do_exch = FALSE;
		if (mode == LCK_RW_GRAB_WANT) {
			if ((data & LCK_RW_WANT_EXCL) == 0) {
				data |= LCK_RW_WANT_EXCL;
				do_exch = TRUE;
			}
		} else {                             
			if (((data & (LCK_RW_WANT_EXCL | LCK_RW_WANT_UPGRADE)) == 0) ||
			    (((data & LCK_RW_SHARED_MASK)) && ((data & LCK_RW_PRIV_EXCL) == 0))) {
				data += LCK_RW_SHARED_READER;
				do_exch = TRUE;
			}
		}
		if (do_exch) {
			if (atomic_exchange_complete32(&lock->lck_rw_data, prev, data, memory_order_acquire_smp)) {
				return TRUE;
			}
		} else {
			if (wait) {                                                           
				wait_for_event();
			} else {
				atomic_exchange_abort();
			}
			if (!wait || (mach_absolute_time() >= deadline)) {
				return FALSE;
			}
		}
	}
}


  
                                         
   
lck_rw_t *
lck_rw_alloc_init(
	lck_grp_t       *grp,
	lck_attr_t      *attr)
{
	lck_rw_t *lck;

	lck = zalloc_flags(ZV_LCK_RW, Z_WAITOK | Z_ZERO);
	lck_rw_init(lck, grp, attr);
	return lck;
}

  
                                   
   
void
lck_rw_free(
	lck_rw_t        *lck,
	lck_grp_t       *grp)
{
	lck_rw_destroy(lck, grp);
	zfree(ZV_LCK_RW, lck);
}

  
                                   
   
void
lck_rw_init(
	lck_rw_t        *lck,
	lck_grp_t       *grp,
	lck_attr_t      *attr)
{
	if (attr == LCK_ATTR_NULL) {
		attr = &LockDefaultLckAttr;
	}
	memset(lck, 0, sizeof(lck_rw_t));
	lck->lck_rw_can_sleep = TRUE;
	if ((attr->lck_attr_val & LCK_ATTR_RW_SHARED_PRIORITY) == 0) {
		lck->lck_rw_priv_excl = TRUE;
	}

	lck_grp_reference(grp);
	lck_grp_lckcnt_incr(grp, LCK_TYPE_RW);
}


  
                                      
   
void
lck_rw_destroy(
	lck_rw_t        *lck,
	lck_grp_t       *grp)
{
	if (lck->lck_rw_tag == LCK_RW_TAG_DESTROYED) {
		return;
	}
#if MACH_LDEBUG
	lck_rw_assert(lck, LCK_RW_ASSERT_NOTHELD);
#endif
	lck->lck_rw_tag = LCK_RW_TAG_DESTROYED;
	lck_grp_lckcnt_decr(grp, LCK_TYPE_RW);
	lck_grp_deallocate(grp);
	return;
}

  
                       
   
void
lck_rw_lock(
	lck_rw_t                *lck,
	lck_rw_type_t   lck_rw_type)
{
	if (lck_rw_type == LCK_RW_TYPE_SHARED) {
		lck_rw_lock_shared(lck);
	} else if (lck_rw_type == LCK_RW_TYPE_EXCLUSIVE) {
		lck_rw_lock_exclusive(lck);
	} else {
		panic("lck_rw_lock(): Invalid RW lock type: %x", lck_rw_type);
	}
}

#define LCK_RW_LOCK_EXCLUSIVE_TAS(lck) (atomic_test_and_set32(&(lck)->lck_rw_data, \
	    (LCK_RW_SHARED_MASK | LCK_RW_WANT_EXCL | LCK_RW_WANT_UPGRADE | LCK_RW_INTERLOCK), \
	    LCK_RW_WANT_EXCL, memory_order_acquire_smp, FALSE))

  
                                                 
   
bool
lck_rw_lock_exclusive_check_contended(lck_rw_t *lock)
{
	thread_t        thread = current_thread();
	bool            contended  = false;

	if (lock->lck_rw_can_sleep) {
		thread->rwlock_count++;
	} else if (get_preemption_level() == 0) {
		panic("Taking non-sleepable RW lock with preemption enabled");
	}
	if (LCK_RW_LOCK_EXCLUSIVE_TAS(lock)) {
#if     CONFIG_DTRACE
		LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_ACQUIRE, lock, DTRACE_RW_EXCL);
#endif                     
	} else {
		contended = true;
		lck_rw_lock_exclusive_gen(lock);
	}
#if MACH_ASSERT
	thread_t owner = ordered_load_rw_owner(lock);
	assertf(owner == THREAD_NULL, "state=0x%x, owner=%p", ordered_load_rw(lock), owner);
#endif
	ordered_store_rw_owner(lock, thread);
	return contended;
}

  
                                 
   
void
lck_rw_lock_exclusive(lck_rw_t *lock)
{
	thread_t        thread = current_thread();

	if (lock->lck_rw_can_sleep) {
		thread->rwlock_count++;
	} else if (get_preemption_level() == 0) {
		panic("Taking non-sleepable RW lock with preemption enabled");
	}
	if (LCK_RW_LOCK_EXCLUSIVE_TAS(lock)) {
#if     CONFIG_DTRACE
		LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_ACQUIRE, lock, DTRACE_RW_EXCL);
#endif                     
	} else {
		lck_rw_lock_exclusive_gen(lock);
	}
#if MACH_ASSERT
	thread_t owner = ordered_load_rw_owner(lock);
	assertf(owner == THREAD_NULL, "state=0x%x, owner=%p", ordered_load_rw(lock), owner);
#endif
	ordered_store_rw_owner(lock, thread);
}

  
                              
   
void
lck_rw_lock_shared(lck_rw_t *lock)
{
	uint32_t        data, prev;

	if (lock->lck_rw_can_sleep) {
		current_thread()->rwlock_count++;
	} else if (get_preemption_level() == 0) {
		panic("Taking non-sleepable RW lock with preemption enabled");
	}
	for (;;) {
		data = atomic_exchange_begin32(&lock->lck_rw_data, &prev, memory_order_acquire_smp);
		if (data & (LCK_RW_WANT_EXCL | LCK_RW_WANT_UPGRADE | LCK_RW_INTERLOCK)) {
			atomic_exchange_abort();
			lck_rw_lock_shared_gen(lock);
			break;
		}
		data += LCK_RW_SHARED_READER;
		if (atomic_exchange_complete32(&lock->lck_rw_data, prev, data, memory_order_acquire_smp)) {
			break;
		}
		cpu_pause();
	}
#if MACH_ASSERT
	thread_t owner = ordered_load_rw_owner(lock);
	assertf(owner == THREAD_NULL, "state=0x%x, owner=%p", ordered_load_rw(lock), owner);
#endif
#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_ACQUIRE, lock, DTRACE_RW_SHARED);
#endif                     
	return;
}

  
                                           
  
                                                                        
   
boolean_t
lck_rw_lock_shared_to_exclusive(lck_rw_t *lock)
{
	uint32_t        data, prev;

	for (;;) {
		data = atomic_exchange_begin32(&lock->lck_rw_data, &prev, memory_order_acquire_smp);
		if (data & LCK_RW_INTERLOCK) {
			atomic_exchange_abort();
			lck_rw_interlock_spin(lock);
			continue;
		}
		if (data & LCK_RW_WANT_UPGRADE) {
			data -= LCK_RW_SHARED_READER;
			if ((data & LCK_RW_SHARED_MASK) == 0) {                                      
				data &= ~(LCK_RW_W_WAITING);                                             
			}
			if (atomic_exchange_complete32(&lock->lck_rw_data, prev, data, memory_order_acquire_smp)) {
				return lck_rw_lock_shared_to_exclusive_failure(lock, prev);
			}
		} else {
			data |= LCK_RW_WANT_UPGRADE;                                      
			data -= LCK_RW_SHARED_READER;                                        
			if (atomic_exchange_complete32(&lock->lck_rw_data, prev, data, memory_order_acquire_smp)) {
				break;
			}
		}
		cpu_pause();
	}
	                                 
	if (data & LCK_RW_SHARED_MASK) {                                                            
		lck_rw_lock_shared_to_exclusive_success(lock);                                  
	}
#if MACH_ASSERT
	thread_t owner = ordered_load_rw_owner(lock);
	assertf(owner == THREAD_NULL, "state=0x%x, owner=%p", ordered_load_rw(lock), owner);
#endif
	ordered_store_rw_owner(lock, current_thread());
#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_TO_EXCL_UPGRADE, lock, 0);
#endif                     
	return TRUE;
}


  
                                                   
            
                                               
                                                                     
                                                                           
                                                             
   
static boolean_t
lck_rw_lock_shared_to_exclusive_failure(
	lck_rw_t        *lck,
	uint32_t        prior_lock_state)
{
	thread_t        thread = current_thread();
	uint32_t        rwlock_count;

	                                                                
	if (lck->lck_rw_can_sleep) {
		rwlock_count = thread->rwlock_count--;
	} else {
		rwlock_count = UINT32_MAX;
	}
#if MACH_LDEBUG
	if (rwlock_count == 0) {
		panic("rw lock count underflow for thread %p", thread);
	}
#endif
	if ((prior_lock_state & LCK_RW_W_WAITING) &&
	    ((prior_lock_state & LCK_RW_SHARED_MASK) == LCK_RW_SHARED_READER)) {
		  
                                        
                                             
                                   
     
		thread_wakeup(LCK_RW_WRITER_EVENT(lck));
	}

	if ((rwlock_count == 1                  ) && (thread->sched_flags & TH_SFLAG_RW_PROMOTED)) {
		                                                                            
		lck_rw_clear_promotion(thread, unslide_for_kdebug(lck));
	}

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX_CODE) | DBG_FUNC_NONE,
	    VM_KERNEL_UNSLIDE_OR_PERM(lck), lck->lck_rw_shared_count, lck->lck_rw_want_upgrade, 0, 0);

	return FALSE;
}

  
                                                   
            
                                                        
                                                         
                                                             
                                                               
   
static boolean_t
lck_rw_lock_shared_to_exclusive_success(
	lck_rw_t        *lock)
{
	__kdebug_only uintptr_t trace_lck = VM_KERNEL_UNSLIDE_OR_PERM(lock);
	int                     slept = 0;
	lck_rw_word_t           word;
	wait_result_t           res;
	boolean_t               istate;
	boolean_t               not_shared;

#if     CONFIG_DTRACE
	uint64_t                wait_interval = 0;
	int                     readers_at_sleep = 0;
	boolean_t               dtrace_ls_initialized = FALSE;
	boolean_t               dtrace_rwl_shared_to_excl_spin, dtrace_rwl_shared_to_excl_block, dtrace_ls_enabled = FALSE;
#endif

	while (!lck_rw_drain_status(lock, LCK_RW_SHARED_MASK, FALSE)) {
		word.data = ordered_load_rw(lock);
#if     CONFIG_DTRACE
		if (dtrace_ls_initialized == FALSE) {
			dtrace_ls_initialized = TRUE;
			dtrace_rwl_shared_to_excl_spin = (lockstat_probemap[LS_LCK_RW_LOCK_SHARED_TO_EXCL_SPIN] != 0);
			dtrace_rwl_shared_to_excl_block = (lockstat_probemap[LS_LCK_RW_LOCK_SHARED_TO_EXCL_BLOCK] != 0);
			dtrace_ls_enabled = dtrace_rwl_shared_to_excl_spin || dtrace_rwl_shared_to_excl_block;
			if (dtrace_ls_enabled) {
				  
                                                
                                                 
       
				readers_at_sleep = word.shared_count;
				wait_interval = mach_absolute_time();
			}
		}
#endif

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX_SPIN_CODE) | DBG_FUNC_START,
		    trace_lck, word.shared_count, 0, 0, 0);

		not_shared = lck_rw_drain_status(lock, LCK_RW_SHARED_MASK, TRUE);

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX_SPIN_CODE) | DBG_FUNC_END,
		    trace_lck, lock->lck_rw_shared_count, 0, 0, 0);

		if (not_shared) {
			break;
		}

		  
                                                                 
                                                            
                                                       
     
		if (word.can_sleep) {
			istate = lck_interlock_lock(lock);

			word.data = ordered_load_rw(lock);
			if (word.shared_count != 0) {
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX_WAIT_CODE) | DBG_FUNC_START,
				    trace_lck, word.shared_count, 0, 0, 0);

				word.w_waiting = 1;
				ordered_store_rw(lock, word.data);

				thread_set_pending_block_hint(current_thread(), kThreadWaitKernelRWLockUpgrade);
				res = assert_wait(LCK_RW_WRITER_EVENT(lock),
				    THREAD_UNINT | THREAD_WAIT_NOREPORT_USER);
				lck_interlock_unlock(lock, istate);

				if (res == THREAD_WAITING) {
					res = thread_block(THREAD_CONTINUE_NULL);
					slept++;
				}
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX_WAIT_CODE) | DBG_FUNC_END,
				    trace_lck, res, slept, 0, 0);
			} else {
				lck_interlock_unlock(lock, istate);
				break;
			}
		}
	}
#if     CONFIG_DTRACE
	  
                                                                                    
    
	if (dtrace_ls_enabled == TRUE) {
		if (slept == 0) {
			LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_TO_EXCL_SPIN, lock, mach_absolute_time() - wait_interval, 0);
		} else {
			LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_TO_EXCL_BLOCK, lock,
			    mach_absolute_time() - wait_interval, 1,
			    (readers_at_sleep == 0 ? 1 : 0), readers_at_sleep);
		}
	}
	LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_TO_EXCL_UPGRADE, lock, 1);
#endif
	return TRUE;
}


  
                                           
   

void
lck_rw_lock_exclusive_to_shared(lck_rw_t *lock)
{
	uint32_t        data, prev;

	assertf(lock->lck_rw_owner == current_thread(), "state=0x%x, owner=%p", lock->lck_rw_data, lock->lck_rw_owner);
	ordered_store_rw_owner(lock, THREAD_NULL);
	for (;;) {
		data = atomic_exchange_begin32(&lock->lck_rw_data, &prev, memory_order_release_smp);
		if (data & LCK_RW_INTERLOCK) {
			atomic_exchange_abort();
			lck_rw_interlock_spin(lock);                                     
			continue;
		}
		data += LCK_RW_SHARED_READER;
		if (data & LCK_RW_WANT_UPGRADE) {
			data &= ~(LCK_RW_WANT_UPGRADE);
		} else {
			data &= ~(LCK_RW_WANT_EXCL);
		}
		if (!((prev & LCK_RW_W_WAITING) && (prev & LCK_RW_PRIV_EXCL))) {
			data &= ~(LCK_RW_W_WAITING);
		}
		if (atomic_exchange_complete32(&lock->lck_rw_data, prev, data, memory_order_release_smp)) {
			break;
		}
		cpu_pause();
	}
	return lck_rw_lock_exclusive_to_shared_gen(lock, prev);
}

  
                                                           
                 
                                 
                                                      
                                                 
                         
   
static void
lck_rw_lock_exclusive_to_shared_gen(
	lck_rw_t        *lck,
	uint32_t        prior_lock_state)
{
	__kdebug_only uintptr_t trace_lck = VM_KERNEL_UNSLIDE_OR_PERM(lck);
	lck_rw_word_t   fake_lck;

	  
                                                         
                                                     
                                                          
                                   
    
	fake_lck.data = prior_lock_state;

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_TO_SH_CODE) | DBG_FUNC_START,
	    trace_lck, fake_lck->want_excl, fake_lck->want_upgrade, 0, 0);

	  
                                                             
                                                                 
                              
   
                                                                     
                                                                      
    
	if (!(fake_lck.priv_excl && fake_lck.w_waiting) && fake_lck.r_waiting) {
		thread_wakeup(LCK_RW_READER_EVENT(lck));
	}

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_TO_SH_CODE) | DBG_FUNC_END,
	    trace_lck, lck->lck_rw_want_excl, lck->lck_rw_want_upgrade, lck->lck_rw_shared_count, 0);

#if CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_TO_SHARED_DOWNGRADE, lck, 0);
#endif
}


  
                                       
   
boolean_t
lck_rw_try_lock(
	lck_rw_t                *lck,
	lck_rw_type_t   lck_rw_type)
{
	if (lck_rw_type == LCK_RW_TYPE_SHARED) {
		return lck_rw_try_lock_shared(lck);
	} else if (lck_rw_type == LCK_RW_TYPE_EXCLUSIVE) {
		return lck_rw_try_lock_exclusive(lck);
	} else {
		panic("lck_rw_try_lock(): Invalid rw lock type: %x", lck_rw_type);
	}
	return FALSE;
}

  
                                  
   

boolean_t
lck_rw_try_lock_shared(lck_rw_t *lock)
{
	uint32_t        data, prev;

	for (;;) {
		data = atomic_exchange_begin32(&lock->lck_rw_data, &prev, memory_order_acquire_smp);
		if (data & LCK_RW_INTERLOCK) {
			atomic_exchange_abort();
			lck_rw_interlock_spin(lock);
			continue;
		}
		if (data & (LCK_RW_WANT_EXCL | LCK_RW_WANT_UPGRADE)) {
			atomic_exchange_abort();
			return FALSE;                                                             
		}
		data += LCK_RW_SHARED_READER;                                                  
		if (atomic_exchange_complete32(&lock->lck_rw_data, prev, data, memory_order_acquire_smp)) {
			break;
		}
		cpu_pause();
	}
#if MACH_ASSERT
	thread_t owner = ordered_load_rw_owner(lock);
	assertf(owner == THREAD_NULL, "state=0x%x, owner=%p", ordered_load_rw(lock), owner);
#endif

	if (lock->lck_rw_can_sleep) {
		current_thread()->rwlock_count++;
	} else if (get_preemption_level() == 0) {
		panic("Taking non-sleepable RW lock with preemption enabled");
	}

#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_RW_TRY_LOCK_SHARED_ACQUIRE, lock, DTRACE_RW_SHARED);
#endif                     
	return TRUE;
}


  
                                     
   

boolean_t
lck_rw_try_lock_exclusive(lck_rw_t *lock)
{
	uint32_t        data, prev;
	thread_t        thread;

	for (;;) {
		data = atomic_exchange_begin32(&lock->lck_rw_data, &prev, memory_order_acquire_smp);
		if (data & LCK_RW_INTERLOCK) {
			atomic_exchange_abort();
			lck_rw_interlock_spin(lock);
			continue;
		}
		if (data & (LCK_RW_SHARED_MASK | LCK_RW_WANT_EXCL | LCK_RW_WANT_UPGRADE)) {
			atomic_exchange_abort();
			return FALSE;
		}
		data |= LCK_RW_WANT_EXCL;
		if (atomic_exchange_complete32(&lock->lck_rw_data, prev, data, memory_order_acquire_smp)) {
			break;
		}
		cpu_pause();
	}
	thread = current_thread();
	if (lock->lck_rw_can_sleep) {
		thread->rwlock_count++;
	} else if (get_preemption_level() == 0) {
		panic("Taking non-sleepable RW lock with preemption enabled");
	}
#if MACH_ASSERT
	thread_t owner = ordered_load_rw_owner(lock);
	assertf(owner == THREAD_NULL, "state=0x%x, owner=%p", ordered_load_rw(lock), owner);
#endif
	ordered_store_rw_owner(lock, thread);
#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_RW_TRY_LOCK_EXCL_ACQUIRE, lock, DTRACE_RW_EXCL);
#endif                     
	return TRUE;
}


  
                         
   
void
lck_rw_unlock(
	lck_rw_t                *lck,
	lck_rw_type_t   lck_rw_type)
{
	if (lck_rw_type == LCK_RW_TYPE_SHARED) {
		lck_rw_unlock_shared(lck);
	} else if (lck_rw_type == LCK_RW_TYPE_EXCLUSIVE) {
		lck_rw_unlock_exclusive(lck);
	} else {
		panic("lck_rw_unlock(): Invalid RW lock type: %d", lck_rw_type);
	}
}


  
                                
   
void
lck_rw_unlock_shared(
	lck_rw_t        *lck)
{
	lck_rw_type_t   ret;

	assertf(lck->lck_rw_owner == THREAD_NULL, "state=0x%x, owner=%p", lck->lck_rw_data, lck->lck_rw_owner);
	assertf(lck->lck_rw_shared_count > 0, "shared_count=0x%x", lck->lck_rw_shared_count);
	ret = lck_rw_done(lck);

	if (ret != LCK_RW_TYPE_SHARED) {
		panic("lck_rw_unlock_shared(): lock %p held in mode: %d", lck, ret);
	}
}


  
                                   
   
void
lck_rw_unlock_exclusive(
	lck_rw_t        *lck)
{
	lck_rw_type_t   ret;

	assertf(lck->lck_rw_owner == current_thread(), "state=0x%x, owner=%p", lck->lck_rw_data, lck->lck_rw_owner);
	ret = lck_rw_done(lck);

	if (ret != LCK_RW_TYPE_EXCLUSIVE) {
		panic("lck_rw_unlock_exclusive(): lock %p held in mode: %d", lck, ret);
	}
}


  
                                                 
   
static void
lck_rw_lock_exclusive_gen(
	lck_rw_t        *lock)
{
	__kdebug_only uintptr_t trace_lck = VM_KERNEL_UNSLIDE_OR_PERM(lock);
	lck_rw_word_t           word;
	int                     slept = 0;
	boolean_t               gotlock = 0;
	boolean_t               not_shared_or_upgrade = 0;
	wait_result_t           res = 0;
	boolean_t               istate;

#if     CONFIG_DTRACE
	boolean_t dtrace_ls_initialized = FALSE;
	boolean_t dtrace_rwl_excl_spin, dtrace_rwl_excl_block, dtrace_ls_enabled = FALSE;
	uint64_t wait_interval = 0;
	int readers_at_sleep = 0;
#endif

	  
                                            
    
	while (!lck_rw_grab(lock, LCK_RW_GRAB_WANT, FALSE)) {
#if     CONFIG_DTRACE
		if (dtrace_ls_initialized == FALSE) {
			dtrace_ls_initialized = TRUE;
			dtrace_rwl_excl_spin = (lockstat_probemap[LS_LCK_RW_LOCK_EXCL_SPIN] != 0);
			dtrace_rwl_excl_block = (lockstat_probemap[LS_LCK_RW_LOCK_EXCL_BLOCK] != 0);
			dtrace_ls_enabled = dtrace_rwl_excl_spin || dtrace_rwl_excl_block;
			if (dtrace_ls_enabled) {
				  
                                                
                                                 
       
				readers_at_sleep = lock->lck_rw_shared_count;
				wait_interval = mach_absolute_time();
			}
		}
#endif

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_WRITER_SPIN_CODE) | DBG_FUNC_START, trace_lck, 0, 0, 0, 0);

		gotlock = lck_rw_grab(lock, LCK_RW_GRAB_WANT, TRUE);

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_WRITER_SPIN_CODE) | DBG_FUNC_END, trace_lck, 0, 0, gotlock, 0);

		if (gotlock) {
			break;
		}
		  
                                                    
                                            
                                                       
     
		word.data = ordered_load_rw(lock);
		if (word.can_sleep) {
			istate = lck_interlock_lock(lock);
			word.data = ordered_load_rw(lock);

			if (word.want_excl) {
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_WRITER_WAIT_CODE) | DBG_FUNC_START, trace_lck, 0, 0, 0, 0);

				word.w_waiting = 1;
				ordered_store_rw(lock, word.data);

				thread_set_pending_block_hint(current_thread(), kThreadWaitKernelRWLockWrite);
				res = assert_wait(LCK_RW_WRITER_EVENT(lock),
				    THREAD_UNINT | THREAD_WAIT_NOREPORT_USER);
				lck_interlock_unlock(lock, istate);

				if (res == THREAD_WAITING) {
					res = thread_block(THREAD_CONTINUE_NULL);
					slept++;
				}
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_WRITER_WAIT_CODE) | DBG_FUNC_END, trace_lck, res, slept, 0, 0);
			} else {
				word.want_excl = 1;
				ordered_store_rw(lock, word.data);
				lck_interlock_unlock(lock, istate);
				break;
			}
		}
	}
	  
                                                
    
	while (!lck_rw_drain_status(lock, LCK_RW_SHARED_MASK | LCK_RW_WANT_UPGRADE, FALSE)) {
#if     CONFIG_DTRACE
		  
                                                    
                                                      
                                                         
                                                   
     
		if (dtrace_ls_initialized == FALSE) {
			dtrace_ls_initialized = TRUE;
			dtrace_rwl_excl_spin = (lockstat_probemap[LS_LCK_RW_LOCK_EXCL_SPIN] != 0);
			dtrace_rwl_excl_block = (lockstat_probemap[LS_LCK_RW_LOCK_EXCL_BLOCK] != 0);
			dtrace_ls_enabled = dtrace_rwl_excl_spin || dtrace_rwl_excl_block;
			if (dtrace_ls_enabled) {
				  
                                                
                                                 
       
				readers_at_sleep = lock->lck_rw_shared_count;
				wait_interval = mach_absolute_time();
			}
		}
#endif

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_READER_SPIN_CODE) | DBG_FUNC_START, trace_lck, 0, 0, 0, 0);

		not_shared_or_upgrade = lck_rw_drain_status(lock, LCK_RW_SHARED_MASK | LCK_RW_WANT_UPGRADE, TRUE);

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_READER_SPIN_CODE) | DBG_FUNC_END, trace_lck, 0, 0, not_shared_or_upgrade, 0);

		if (not_shared_or_upgrade) {
			break;
		}
		  
                                                    
                                            
                                                       
     
		word.data = ordered_load_rw(lock);
		if (word.can_sleep) {
			istate = lck_interlock_lock(lock);
			word.data = ordered_load_rw(lock);

			if (word.shared_count != 0 || word.want_upgrade) {
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_READER_WAIT_CODE) | DBG_FUNC_START, trace_lck, 0, 0, 0, 0);

				word.w_waiting = 1;
				ordered_store_rw(lock, word.data);

				thread_set_pending_block_hint(current_thread(), kThreadWaitKernelRWLockWrite);
				res = assert_wait(LCK_RW_WRITER_EVENT(lock),
				    THREAD_UNINT | THREAD_WAIT_NOREPORT_USER);
				lck_interlock_unlock(lock, istate);

				if (res == THREAD_WAITING) {
					res = thread_block(THREAD_CONTINUE_NULL);
					slept++;
				}
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_READER_WAIT_CODE) | DBG_FUNC_END, trace_lck, res, slept, 0, 0);
			} else {
				lck_interlock_unlock(lock, istate);
				  
                                                  
                                                    
                                                  
       
				break;
			}
		}
	}

#if     CONFIG_DTRACE
	  
                                                             
                                                               
                                                                 
                                                           
                   
                                                                           
                                                            
    
	if (dtrace_ls_enabled == TRUE) {
		if (slept == 0) {
			LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_SPIN, lock,
			    mach_absolute_time() - wait_interval, 1);
		} else {
			  
                                                              
                                                          
                                                          
                                             
      
			LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_BLOCK, lock,
			    mach_absolute_time() - wait_interval, 1,
			    (readers_at_sleep == 0 ? 1 : 0), readers_at_sleep);
		}
	}
	LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_ACQUIRE, lock, 1);
#endif                     
}

  
                                   
   

lck_rw_type_t
lck_rw_done(lck_rw_t *lock)
{
	uint32_t        data, prev;
	boolean_t       once = FALSE;

	for (;;) {
		data = atomic_exchange_begin32(&lock->lck_rw_data, &prev, memory_order_release_smp);
		if (data & LCK_RW_INTERLOCK) {                                           
			atomic_exchange_abort();
			lck_rw_interlock_spin(lock);
			continue;
		}
		if (data & LCK_RW_SHARED_MASK) {                                 
			assertf(lock->lck_rw_owner == THREAD_NULL, "state=0x%x, owner=%p", lock->lck_rw_data, lock->lck_rw_owner);
			data -= LCK_RW_SHARED_READER;
			if ((data & LCK_RW_SHARED_MASK) == 0) {                                                           
				goto check_waiters;
			}
		} else {                                                                                          
			if (data & LCK_RW_WANT_UPGRADE) {
				data &= ~(LCK_RW_WANT_UPGRADE);
			} else {
				if (data & LCK_RW_WANT_EXCL) {
					data &= ~(LCK_RW_WANT_EXCL);
				} else {                                                                
					panic("Releasing non-exclusive RW lock without a reader refcount!");
				}
			}
			if (!once) {
				                                          
				assertf(lock->lck_rw_owner == current_thread(), "state=0x%x, owner=%p", lock->lck_rw_data, lock->lck_rw_owner);
				ordered_store_rw_owner(lock, THREAD_NULL);
				once = TRUE;
			}
check_waiters:
			  
                                            
                                                 
                                     
     
                                                                 
      
			if (prev & LCK_RW_W_WAITING) {
				data &= ~(LCK_RW_W_WAITING);
				if ((prev & LCK_RW_PRIV_EXCL) == 0) {
					data &= ~(LCK_RW_R_WAITING);
				}
			} else {
				data &= ~(LCK_RW_R_WAITING);
			}
		}
		if (atomic_exchange_complete32(&lock->lck_rw_data, prev, data, memory_order_release_smp)) {
			break;
		}
		cpu_pause();
	}
	return lck_rw_done_gen(lock, prev);
}

  
                                       
  
                                               
                                           
                                                    
                                                    
                                                     
                                                    
                                            
                                               
                                                         
                                      
                                                   
                                                 
                
   
static lck_rw_type_t
lck_rw_done_gen(
	lck_rw_t        *lck,
	uint32_t        prior_lock_state)
{
	lck_rw_word_t   fake_lck;
	lck_rw_type_t   lock_type;
	thread_t                thread;
	uint32_t                rwlock_count;

	  
                                                         
                                                     
                                                          
                                   
    
	fake_lck.data = prior_lock_state;

	if (fake_lck.shared_count <= 1) {
		if (fake_lck.w_waiting) {
			thread_wakeup(LCK_RW_WRITER_EVENT(lck));
		}

		if (!(fake_lck.priv_excl && fake_lck.w_waiting) && fake_lck.r_waiting) {
			thread_wakeup(LCK_RW_READER_EVENT(lck));
		}
	}
	if (fake_lck.shared_count) {
		lock_type = LCK_RW_TYPE_SHARED;
	} else {
		lock_type = LCK_RW_TYPE_EXCLUSIVE;
	}

	                                                                
	thread = current_thread();
	if (fake_lck.can_sleep) {
		rwlock_count = thread->rwlock_count--;
	} else {
		rwlock_count = UINT32_MAX;
	}
#if MACH_LDEBUG
	if (rwlock_count == 0) {
		panic("rw lock count underflow for thread %p", thread);
	}
#endif
	if ((rwlock_count == 1                  ) && (thread->sched_flags & TH_SFLAG_RW_PROMOTED)) {
		                                                                            
		lck_rw_clear_promotion(thread, unslide_for_kdebug(lck));
	}
#if CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_RW_DONE_RELEASE, lck, lock_type == LCK_RW_TYPE_SHARED ? 0 : 1);
#endif
	return lock_type;
}

  
                                  
            
                                                
                                                      
                                                    
   
static void
lck_rw_lock_shared_gen(
	lck_rw_t        *lck)
{
	__kdebug_only uintptr_t trace_lck = VM_KERNEL_UNSLIDE_OR_PERM(lck);
	lck_rw_word_t           word;
	boolean_t               gotlock = 0;
	int                     slept = 0;
	wait_result_t           res = 0;
	boolean_t               istate;

#if     CONFIG_DTRACE
	uint64_t wait_interval = 0;
	int readers_at_sleep = 0;
	boolean_t dtrace_ls_initialized = FALSE;
	boolean_t dtrace_rwl_shared_spin, dtrace_rwl_shared_block, dtrace_ls_enabled = FALSE;
#endif                    

	while (!lck_rw_grab(lck, LCK_RW_GRAB_SHARED, FALSE)) {
#if     CONFIG_DTRACE
		if (dtrace_ls_initialized == FALSE) {
			dtrace_ls_initialized = TRUE;
			dtrace_rwl_shared_spin = (lockstat_probemap[LS_LCK_RW_LOCK_SHARED_SPIN] != 0);
			dtrace_rwl_shared_block = (lockstat_probemap[LS_LCK_RW_LOCK_SHARED_BLOCK] != 0);
			dtrace_ls_enabled = dtrace_rwl_shared_spin || dtrace_rwl_shared_block;
			if (dtrace_ls_enabled) {
				  
                                                
                                                 
       
				readers_at_sleep = lck->lck_rw_shared_count;
				wait_interval = mach_absolute_time();
			}
		}
#endif

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SHARED_SPIN_CODE) | DBG_FUNC_START,
		    trace_lck, lck->lck_rw_want_excl, lck->lck_rw_want_upgrade, 0, 0);

		gotlock = lck_rw_grab(lck, LCK_RW_GRAB_SHARED, TRUE);

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SHARED_SPIN_CODE) | DBG_FUNC_END,
		    trace_lck, lck->lck_rw_want_excl, lck->lck_rw_want_upgrade, gotlock, 0);

		if (gotlock) {
			break;
		}
		  
                                                    
                                         
                                                       
     
		if (lck->lck_rw_can_sleep) {
			istate = lck_interlock_lock(lck);

			word.data = ordered_load_rw(lck);
			if ((word.want_excl || word.want_upgrade) &&
			    ((word.shared_count == 0) || word.priv_excl)) {
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SHARED_WAIT_CODE) | DBG_FUNC_START,
				    trace_lck, word.want_excl, word.want_upgrade, 0, 0);

				word.r_waiting = 1;
				ordered_store_rw(lck, word.data);

				thread_set_pending_block_hint(current_thread(), kThreadWaitKernelRWLockRead);
				res = assert_wait(LCK_RW_READER_EVENT(lck),
				    THREAD_UNINT | THREAD_WAIT_NOREPORT_USER);
				lck_interlock_unlock(lck, istate);

				if (res == THREAD_WAITING) {
					res = thread_block(THREAD_CONTINUE_NULL);
					slept++;
				}
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SHARED_WAIT_CODE) | DBG_FUNC_END,
				    trace_lck, res, slept, 0, 0);
			} else {
				word.shared_count++;
				ordered_store_rw(lck, word.data);
				lck_interlock_unlock(lck, istate);
				break;
			}
		}
	}

#if     CONFIG_DTRACE
	if (dtrace_ls_enabled == TRUE) {
		if (slept == 0) {
			LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_SPIN, lck, mach_absolute_time() - wait_interval, 0);
		} else {
			LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_BLOCK, lck,
			    mach_absolute_time() - wait_interval, 0,
			    (readers_at_sleep == 0 ? 1 : 0), readers_at_sleep);
		}
	}
	LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_ACQUIRE, lck, 0);
#endif                     
}

  
                                                                           
        
   
void
lck_rw_assert(
	lck_rw_t                *lck,
	unsigned int    type)
{
	switch (type) {
	case LCK_RW_ASSERT_SHARED:
		if ((lck->lck_rw_shared_count != 0) &&
		    (lck->lck_rw_owner == THREAD_NULL)) {
			return;
		}
		break;
	case LCK_RW_ASSERT_EXCLUSIVE:
		if ((lck->lck_rw_want_excl || lck->lck_rw_want_upgrade) &&
		    (lck->lck_rw_shared_count == 0) &&
		    (lck->lck_rw_owner == current_thread())) {
			return;
		}
		break;
	case LCK_RW_ASSERT_HELD:
		if (lck->lck_rw_shared_count != 0) {
			return;                       
		}
		if ((lck->lck_rw_want_excl || lck->lck_rw_want_upgrade) &&
		    (lck->lck_rw_owner == current_thread())) {
			return;                          
		}
		break;
	case LCK_RW_ASSERT_NOTHELD:
		if ((lck->lck_rw_shared_count == 0) &&
		    !(lck->lck_rw_want_excl || lck->lck_rw_want_upgrade) &&
		    (lck->lck_rw_owner == THREAD_NULL)) {
			return;
		}
		break;
	default:
		break;
	}
	panic("rw lock (%p)%s held (mode=%u)", lck, (type == LCK_RW_ASSERT_NOTHELD ? "" : " not"), type);
}


  
                                                 
                                                                  
   
boolean_t
kdp_lck_rw_lock_is_acquired_exclusive(lck_rw_t *lck)
{
	if (not_in_kdp) {
		panic("panic: rw lock exclusive check done outside of kernel debugger");
	}
	return ((lck->lck_rw_want_upgrade || lck->lck_rw_want_excl) && (lck->lck_rw_shared_count == 0)) ? TRUE : FALSE;
}

  
                                                                       
                                                         
   

  
                      
   

void
lck_mtx_ext_init(
	lck_mtx_ext_t * lck,
	lck_grp_t * grp,
	lck_attr_t * attr);

  
                                          
   
lck_mtx_t      *
lck_mtx_alloc_init(
	lck_grp_t * grp,
	lck_attr_t * attr)
{
	lck_mtx_t      *lck;

	lck = zalloc(ZV_LCK_MTX);
	lck_mtx_init(lck, grp, attr);
	return lck;
}

  
                                    
   
void
lck_mtx_free(
	lck_mtx_t * lck,
	lck_grp_t * grp)
{
	lck_mtx_destroy(lck, grp);
	zfree(ZV_LCK_MTX, lck);
}

  
                                    
   
void
lck_mtx_init(
	lck_mtx_t * lck,
	lck_grp_t * grp,
	lck_attr_t * attr)
{
#ifdef  BER_XXX
	lck_mtx_ext_t  *lck_ext;
#endif
	lck_attr_t     *lck_attr;

	if (attr != LCK_ATTR_NULL) {
		lck_attr = attr;
	} else {
		lck_attr = &LockDefaultLckAttr;
	}

#ifdef  BER_XXX
	if ((lck_attr->lck_attr_val) & LCK_ATTR_DEBUG) {
		lck_ext = zalloc(ZV_LCK_MTX_EXT);
		lck_mtx_ext_init(lck_ext, grp, lck_attr);
		lck->lck_mtx_tag = LCK_MTX_TAG_INDIRECT;
		lck->lck_mtx_ptr = lck_ext;
		lck->lck_mtx_type = LCK_MTX_TYPE;
	} else
#endif
	{
		lck->lck_mtx_ptr = NULL;                                                              
		lck->lck_mtx_waiters = 0;
		lck->lck_mtx_type = LCK_MTX_TYPE;
		ordered_store_mtx(lck, 0);
	}
	lck_grp_reference(grp);
	lck_grp_lckcnt_incr(grp, LCK_TYPE_MTX);
}

  
                                        
   
void
lck_mtx_init_ext(
	lck_mtx_t * lck,
	lck_mtx_ext_t * lck_ext,
	lck_grp_t * grp,
	lck_attr_t * attr)
{
	lck_attr_t     *lck_attr;

	if (attr != LCK_ATTR_NULL) {
		lck_attr = attr;
	} else {
		lck_attr = &LockDefaultLckAttr;
	}

	if ((lck_attr->lck_attr_val) & LCK_ATTR_DEBUG) {
		lck_mtx_ext_init(lck_ext, grp, lck_attr);
		lck->lck_mtx_tag = LCK_MTX_TAG_INDIRECT;
		lck->lck_mtx_ptr = lck_ext;
		lck->lck_mtx_type = LCK_MTX_TYPE;
	} else {
		lck->lck_mtx_waiters = 0;
		lck->lck_mtx_type = LCK_MTX_TYPE;
		ordered_store_mtx(lck, 0);
	}
	lck_grp_reference(grp);
	lck_grp_lckcnt_incr(grp, LCK_TYPE_MTX);
}

  
                                        
   
void
lck_mtx_ext_init(
	lck_mtx_ext_t * lck,
	lck_grp_t * grp,
	lck_attr_t * attr)
{
	bzero((void *) lck, sizeof(lck_mtx_ext_t));

	lck->lck_mtx.lck_mtx_type = LCK_MTX_TYPE;

	if ((attr->lck_attr_val) & LCK_ATTR_DEBUG) {
		lck->lck_mtx_deb.type = MUTEX_TAG;
		lck->lck_mtx_attr |= LCK_MTX_ATTR_DEBUG;
	}
	lck->lck_mtx_grp = grp;

	if (grp->lck_grp_attr & LCK_GRP_ATTR_STAT) {
		lck->lck_mtx_attr |= LCK_MTX_ATTR_STAT;
	}
}

                       
static void lck_mtx_lock_contended(lck_mtx_t *lock, thread_t thread, boolean_t interlocked);
static boolean_t lck_mtx_try_lock_contended(lck_mtx_t *lock, thread_t thread);
static void lck_mtx_unlock_contended(lck_mtx_t *lock, thread_t thread, boolean_t interlocked);

                                
static spinwait_result_t lck_mtx_lock_contended_spinwait_arm(lck_mtx_t *lock, thread_t thread, boolean_t interlocked);

  
                          
  
                             
   
static inline void
lck_mtx_verify(lck_mtx_t *lock)
{
	if (lock->lck_mtx_type != LCK_MTX_TYPE) {
		panic("Invalid mutex %p", lock);
	}
#if     DEVELOPMENT || DEBUG
	if (lock->lck_mtx_tag == LCK_MTX_TAG_DESTROYED) {
		panic("Mutex destroyed %p", lock);
	}
#endif                            
}

  
                                    
  
                                                                   
   

static inline void
lck_mtx_check_preemption(lck_mtx_t *lock)
{
#if     DEVELOPMENT || DEBUG
	if (current_cpu_datap()->cpu_hibernate) {
		return;
	}

	int pl = get_preemption_level();

	if (pl != 0) {
		panic("Attempt to take mutex with preemption disabled. Lock=%p, level=%d", lock, pl);
	}
#else
	(void)lock;
#endif
}

  
                        
   
void
lck_mtx_lock(lck_mtx_t *lock)
{
	thread_t        thread;

	lck_mtx_verify(lock);
	lck_mtx_check_preemption(lock);
	thread = current_thread();
	if (os_atomic_cmpxchg(&lock->lck_mtx_data,
	    0, LCK_MTX_THREAD_TO_STATE(thread), acquire)) {
#if     CONFIG_DTRACE
		LOCKSTAT_RECORD(LS_LCK_MTX_LOCK_ACQUIRE, lock, 0);
#endif                    
		return;
	}
	lck_mtx_lock_contended(lock, thread, FALSE);
}

  
                                                   
   
static void NOINLINE
lck_mtx_lock_contended(lck_mtx_t *lock, thread_t thread, boolean_t interlocked)
{
	thread_t                holding_thread;
	uintptr_t               state;
	int                     waiters = 0;
	spinwait_result_t       sw_res;
	struct turnstile        *ts = NULL;

	                                                        
	for (;;) {
		sw_res = lck_mtx_lock_contended_spinwait_arm(lock, thread, interlocked);
		interlocked = FALSE;

		switch (sw_res) {
		case SPINWAIT_ACQUIRED:
			if (ts != NULL) {
				interlock_lock(lock);
				turnstile_complete((uintptr_t)lock, NULL, NULL, TURNSTILE_KERNEL_MUTEX);
				interlock_unlock(lock);
			}
			goto done;
		case SPINWAIT_INTERLOCK:
			goto set_owner;
		default:
			break;
		}

		state = ordered_load_mtx(lock);
		holding_thread = LCK_MTX_STATE_TO_THREAD(state);
		if (holding_thread == NULL) {
			break;
		}
		ordered_store_mtx(lock, (state | LCK_ILOCK | ARM_LCK_WAITERS));                            
		lck_mtx_lock_wait(lock, holding_thread, &ts);
		                                
	}

set_owner:
	                                
	state = ordered_load_mtx(lock);

	if (state & ARM_LCK_WAITERS) {
		                                                        
		waiters = lck_mtx_lock_acquire(lock, ts);
		  
                                   
                       
     
	} else {
		if (ts != NULL) {
			turnstile_complete((uintptr_t)lock, NULL, NULL, TURNSTILE_KERNEL_MUTEX);
		}
	}

	state = LCK_MTX_THREAD_TO_STATE(thread);
	if (waiters != 0) {
		state |= ARM_LCK_WAITERS;
	}
	state |= LCK_ILOCK;                                                  
	ordered_store_mtx(lock, state);                 
	interlock_unlock(lock);                                                        

done:
	load_memory_barrier();

	assert(thread->turnstile != NULL);

	if (ts != NULL) {
		turnstile_cleanup();
	}

#if CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_MTX_LOCK_ACQUIRE, lock, 0);
#endif                    
}

  
                                     
  
                                                                 
                                                                          
                                            
   
static spinwait_result_t
lck_mtx_lock_contended_spinwait_arm(lck_mtx_t *lock, thread_t thread, boolean_t interlocked)
{
	int                     has_interlock = (int)interlocked;
	__kdebug_only uintptr_t trace_lck = VM_KERNEL_UNSLIDE_OR_PERM(lock);
	thread_t        owner, prev_owner;
	uint64_t        window_deadline, sliding_deadline, high_deadline;
	uint64_t        start_time, cur_time, avg_hold_time, bias, delta;
	int             loopcount = 0;
	uint            i, prev_owner_cpu;
	int             total_hold_time_samples, window_hold_time_samples, unfairness;
	bool            owner_on_core, adjust;
	uintptr_t       state, new_state, waiters;
	spinwait_result_t       retval = SPINWAIT_DID_SPIN_HIGH_THR;

	if (__improbable(!(lck_mtx_adaptive_spin_mode & ADAPTIVE_SPIN_ENABLE))) {
		if (!has_interlock) {
			interlock_lock(lock);
		}

		return SPINWAIT_DID_NOT_SPIN;
	}

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_MTX_LCK_SPIN_CODE) | DBG_FUNC_START,
	    trace_lck, VM_KERNEL_UNSLIDE_OR_PERM(LCK_MTX_STATE_TO_THREAD(state)), lock->lck_mtx_waiters, 0, 0);

	start_time = mach_absolute_time();
	  
                                                    
                                                        
                                                                        
                                                  
             
   
                                                 
    
	window_deadline = start_time + low_MutexSpin;
	  
                                                  
                                        
    
	sliding_deadline = window_deadline;
	  
                                               
                                     
    
	if (high_MutexSpin >= 0) {
		high_deadline = start_time + high_MutexSpin;
	} else {
		high_deadline = start_time + low_MutexSpin * real_ncpus;
	}

	  
                                           
                                            
    
	prev_owner_cpu = (cpu_number() + 1) % real_ncpus;
	total_hold_time_samples = 0;
	window_hold_time_samples = 0;
	avg_hold_time = 0;
	adjust = TRUE;
	bias = (os_hash_kernel_pointer(lock) + cpu_number()) % real_ncpus;

	                          
	state = ordered_load_mtx(lock);
	owner = LCK_MTX_STATE_TO_THREAD(state);
	prev_owner = owner;

	if (has_interlock) {
		if (owner == NULL) {
			retval = SPINWAIT_INTERLOCK;
			goto done_spinning;
		} else {
			  
                                      
                                      
      
			if (!machine_thread_on_core(owner) || (owner->state & TH_IDLE)) {
				retval = SPINWAIT_DID_NOT_SPIN;
				goto done_spinning;
			}
		}
		interlock_unlock(lock);
		has_interlock = 0;
	}

	  
               
                            
                                       
                                                  
                                        
    
	do {
		  
                             
     
		owner = LCK_MTX_STATE_TO_THREAD(state);
		if (owner == NULL) {
			waiters = state & ARM_LCK_WAITERS;
			if (waiters) {
				  
                              
                                     
                                         
                                        
                        
       
				new_state = ARM_LCK_WAITERS | LCK_ILOCK;
				has_interlock = 1;
				retval = SPINWAIT_INTERLOCK;
				disable_preemption();
			} else {
				new_state = LCK_MTX_THREAD_TO_STATE(thread);
				retval = SPINWAIT_ACQUIRED;
			}

			  
                                              
                                              
                                
                                            
      
			if (os_atomic_cmpxchgv(&lock->lck_mtx_data,
			    waiters, new_state, &state, acquire)) {
				goto done_spinning;
			} else {
				if (waiters) {
					has_interlock = 0;
					enable_preemption();
				}
			}
		}

		cur_time = mach_absolute_time();

		  
                                   
     
		if (cur_time >= high_deadline) {
			retval = SPINWAIT_DID_SPIN_HIGH_THR;
			break;
		}

		  
                                             
     
		owner = LCK_MTX_STATE_TO_THREAD(state);
		if (owner) {
			i = prev_owner_cpu;
			owner_on_core = FALSE;

			disable_preemption();
			state = ordered_load_mtx(lock);
			owner = LCK_MTX_STATE_TO_THREAD(state);

			  
                                                              
                                          
                                                                           
                                                                     
                                                          
                                                                              
      
			if (owner) {
				do {
					cpu_data_t *cpu_data_ptr = CpuDataEntries[i].cpu_data_vaddr;
					if ((cpu_data_ptr != NULL) && (cpu_data_ptr->cpu_active_thread == owner)) {
						owner_on_core = TRUE;
						break;
					}
					if (++i >= real_ncpus) {
						i = 0;
					}
				} while (i != prev_owner_cpu);
				enable_preemption();

				if (owner_on_core) {
					prev_owner_cpu = i;
				} else {
					prev_owner = owner;
					state = ordered_load_mtx(lock);
					owner = LCK_MTX_STATE_TO_THREAD(state);
					if (owner == prev_owner) {
						  
                              
                       
         
						if (loopcount == 0) {
							retval = SPINWAIT_DID_NOT_SPIN;
						} else {
							retval = SPINWAIT_DID_SPIN_OWNER_NOT_CORE;
						}
						break;
					}
					  
                                                                 
                                                           
              
        
				}
			} else {
				enable_preemption();
			}
		}

		  
                                                   
                                               
                                     
     
		if (owner != prev_owner) {
			prev_owner = owner;
			total_hold_time_samples++;
			window_hold_time_samples++;
		}

		  
                             
                                        
     
		if (cur_time >= window_deadline) {
			  
                                                   
                    
      
			if (window_hold_time_samples < 1) {
				retval = SPINWAIT_DID_SPIN_NO_WINDOW_CONTENTION;
				break;
			}

			if (adjust) {
				  
                                                               
                                                                    
       
				unfairness = total_hold_time_samples / real_ncpus;

				if (unfairness == 0) {
					  
                                                                            
                                                                            
                         
                                                                          
       
                                                                 
        
					delta = cur_time - start_time;
					sliding_deadline = start_time + (delta * (real_ncpus - 1)) / total_hold_time_samples;
				} else {
					  
                                                                                  
                             
                                                                                   
                                                       
       
                                                          
                                        
       
                                                    
                                                                                
        
					delta = high_deadline - cur_time;
					sliding_deadline = cur_time + ((delta * bias) / real_ncpus);
					adjust = FALSE;
				}
			}

			window_deadline += low_MutexSpin;
			window_hold_time_samples = 0;
		}

		  
                             
                           
     
		if (cur_time >= sliding_deadline) {
			retval = SPINWAIT_DID_SPIN_SLIDING_THR;
			break;
		}

		  
                                        
                                  
    
          
                                      
                                             
                                           
                                                   
     
		state = os_atomic_load_exclusive(&lock->lck_mtx_data, relaxed);
		owner = LCK_MTX_STATE_TO_THREAD(state);
		if (owner != NULL) {
			wait_for_event();
			state = ordered_load_mtx(lock);
		} else {
			atomic_exchange_abort();
		}

		loopcount++;
	} while (TRUE);

done_spinning:
#if     CONFIG_DTRACE
	  
                                                                 
                                                          
                                                           
                                      
    
	if (__probable(lock->lck_mtx_tag != LCK_MTX_TAG_INDIRECT)) {
		LOCKSTAT_RECORD(LS_LCK_MTX_LOCK_SPIN, lock,
		    mach_absolute_time() - start_time);
	} else {
		LOCKSTAT_RECORD(LS_LCK_MTX_EXT_LOCK_SPIN, lock,
		    mach_absolute_time() - start_time);
	}
	                                                           
#endif

	state = ordered_load_mtx(lock);

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_MTX_LCK_SPIN_CODE) | DBG_FUNC_END,
	    trace_lck, VM_KERNEL_UNSLIDE_OR_PERM(LCK_MTX_STATE_TO_THREAD(state)), lock->lck_mtx_waiters, retval, 0);
	if ((!has_interlock) && (retval != SPINWAIT_ACQUIRED)) {
		                                                             
		interlock_lock(lock);
	}

	return retval;
}


  
                                            
   
static inline void
lck_mtx_lock_spin_internal(lck_mtx_t *lock, boolean_t allow_held_as_mutex)
{
	uintptr_t       state;

	interlock_lock(lock);
	state = ordered_load_mtx(lock);
	if (LCK_MTX_STATE_TO_THREAD(state)) {
		if (allow_held_as_mutex) {
			lck_mtx_lock_contended(lock, current_thread(), TRUE);
		} else {
			                                                                                     
			                                                                                
			             
			panic("Attempting to block on a lock taken as spin-always %p", lock);
		}
		return;
	}
	state &= ARM_LCK_WAITERS;                                                                      
	state |= (LCK_MTX_SPIN_TAG | LCK_ILOCK);                                              
	ordered_store_mtx(lock, state);
	load_memory_barrier();

#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_MTX_LOCK_SPIN_ACQUIRE, lock, 0);
#endif                    
}

  
                             
   
void
lck_mtx_lock_spin(lck_mtx_t *lock)
{
	lck_mtx_check_preemption(lock);
	lck_mtx_lock_spin_internal(lock, TRUE);
}

  
                                    
   
void
lck_mtx_lock_spin_always(lck_mtx_t *lock)
{
	lck_mtx_lock_spin_internal(lock, FALSE);
}

  
                            
   
boolean_t
lck_mtx_try_lock(lck_mtx_t *lock)
{
	thread_t        thread = current_thread();

	lck_mtx_verify(lock);
	if (os_atomic_cmpxchg(&lock->lck_mtx_data,
	    0, LCK_MTX_THREAD_TO_STATE(thread), acquire)) {
#if     CONFIG_DTRACE
		LOCKSTAT_RECORD(LS_LCK_MTX_TRY_LOCK_ACQUIRE, lock, 0);
#endif                    
		return TRUE;
	}
	return lck_mtx_try_lock_contended(lock, thread);
}

static boolean_t NOINLINE
lck_mtx_try_lock_contended(lck_mtx_t *lock, thread_t thread)
{
	thread_t        holding_thread;
	uintptr_t       state;
	int             waiters;

	interlock_lock(lock);
	state = ordered_load_mtx(lock);
	holding_thread = LCK_MTX_STATE_TO_THREAD(state);
	if (holding_thread) {
		interlock_unlock(lock);
		return FALSE;
	}
	waiters = lck_mtx_lock_acquire(lock, NULL);
	state = LCK_MTX_THREAD_TO_STATE(thread);
	if (waiters != 0) {
		state |= ARM_LCK_WAITERS;
	}
	state |= LCK_ILOCK;                                                  
	ordered_store_mtx(lock, state);                 
	interlock_unlock(lock);                                                        
	load_memory_barrier();

	turnstile_cleanup();

	return TRUE;
}

static inline boolean_t
lck_mtx_try_lock_spin_internal(lck_mtx_t *lock, boolean_t allow_held_as_mutex)
{
	uintptr_t       state;

	if (!interlock_try(lock)) {
		return FALSE;
	}
	state = ordered_load_mtx(lock);
	if (LCK_MTX_STATE_TO_THREAD(state)) {
		                        
		if (allow_held_as_mutex) {
			interlock_unlock(lock);
		} else {
			                                                                           
			                                                                                
			             
			panic("Spin-mutex held as full mutex %p", lock);
		}
		return FALSE;
	}
	state &= ARM_LCK_WAITERS;                                                                      
	state |= (LCK_MTX_SPIN_TAG | LCK_ILOCK);                                              
	ordered_store_mtx(lock, state);
	load_memory_barrier();

#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_MTX_TRY_SPIN_LOCK_ACQUIRE, lock, 0);
#endif                    
	return TRUE;
}

  
                                 
   
boolean_t
lck_mtx_try_lock_spin(lck_mtx_t *lock)
{
	return lck_mtx_try_lock_spin_internal(lock, TRUE);
}

  
                                        
   
boolean_t
lck_mtx_try_lock_spin_always(lck_mtx_t *lock)
{
	return lck_mtx_try_lock_spin_internal(lock, FALSE);
}



  
                          
   
void
lck_mtx_unlock(lck_mtx_t *lock)
{
	thread_t        thread = current_thread();
	uintptr_t       state;
	boolean_t       ilk_held = FALSE;

	lck_mtx_verify(lock);

	state = ordered_load_mtx(lock);
	if (state & LCK_ILOCK) {
		if (LCK_MTX_STATE_TO_THREAD(state) == (thread_t)LCK_MTX_SPIN_TAG) {
			ilk_held = TRUE;                                                        
		}
		goto slow_case;
	}
	                    
	if (os_atomic_cmpxchg(&lock->lck_mtx_data,
	    LCK_MTX_THREAD_TO_STATE(thread), 0, release)) {
#if     CONFIG_DTRACE
		LOCKSTAT_RECORD(LS_LCK_MTX_UNLOCK_RELEASE, lock, 0);
#endif                    
		return;
	}
slow_case:
	lck_mtx_unlock_contended(lock, thread, ilk_held);
}

static void NOINLINE
lck_mtx_unlock_contended(lck_mtx_t *lock, thread_t thread, boolean_t ilk_held)
{
	uintptr_t       state;
	boolean_t               cleanup = FALSE;

	if (ilk_held) {
		state = ordered_load_mtx(lock);
	} else {
		interlock_lock(lock);
		state = ordered_load_mtx(lock);
		if (thread != LCK_MTX_STATE_TO_THREAD(state)) {
			panic("lck_mtx_unlock(): Attempt to release lock not owned by thread (%p)", lock);
		}
		if (state & ARM_LCK_WAITERS) {
			if (lck_mtx_unlock_wakeup(lock, thread)) {
				state = ARM_LCK_WAITERS;
			} else {
				state = 0;
			}
			cleanup = TRUE;
			goto unlock;
		}
	}
	state &= ARM_LCK_WAITERS;                                        
unlock:
	state |= LCK_ILOCK;
	ordered_store_mtx(lock, state);
	interlock_unlock(lock);
	if (cleanup) {
		  
                                                              
                                                                     
                                           
                                                               
                                             
     
		turnstile_cleanup();
	}

#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_MTX_UNLOCK_RELEASE, lock, 0);
#endif                    
}

  
                          
   
void
lck_mtx_assert(lck_mtx_t *lock, unsigned int type)
{
	thread_t        thread, holder;
	uintptr_t       state;

	state = ordered_load_mtx(lock);
	holder = LCK_MTX_STATE_TO_THREAD(state);
	if (holder == (thread_t)LCK_MTX_SPIN_TAG) {
		                                               
		return;        
	}
	thread = current_thread();
	if (type == LCK_MTX_ASSERT_OWNED) {
		if (thread != holder) {
			panic("lck_mtx_assert(): mutex (%p) owned", lock);
		}
	} else if (type == LCK_MTX_ASSERT_NOTOWNED) {
		if (thread == holder) {
			panic("lck_mtx_assert(): mutex (%p) not owned", lock);
		}
	} else {
		panic("lck_mtx_assert(): invalid arg (%u)", type);
	}
}

  
                              
   
boolean_t
lck_mtx_ilk_unlock(lck_mtx_t *lock)
{
	interlock_unlock(lock);
	return TRUE;
}

  
                                
  
                                                       
   
void
lck_mtx_convert_spin(lck_mtx_t *lock)
{
	thread_t        thread = current_thread();
	uintptr_t       state;
	int                     waiters;

	state = ordered_load_mtx(lock);
	if (LCK_MTX_STATE_TO_THREAD(state) == thread) {
		return;                                          
	}
	if ((state & LCK_ILOCK) == 0 || (LCK_MTX_STATE_TO_THREAD(state) != (thread_t)LCK_MTX_SPIN_TAG)) {
		panic("lck_mtx_convert_spin: Not held as spinlock (%p)", lock);
	}
	state &= ~(LCK_MTX_THREAD_MASK);                                     
	ordered_store_mtx(lock, state);
	waiters = lck_mtx_lock_acquire(lock, NULL);                                       
	state = LCK_MTX_THREAD_TO_STATE(thread);
	if (waiters != 0) {
		state |= ARM_LCK_WAITERS;
	}
	state |= LCK_ILOCK;
	ordered_store_mtx(lock, state);                                 
	interlock_unlock(lock);                                                                        
	turnstile_cleanup();
}


  
                                       
   
void
lck_mtx_destroy(
	lck_mtx_t * lck,
	lck_grp_t * grp)
{
	if (lck->lck_mtx_type != LCK_MTX_TYPE) {
		panic("Destroying invalid mutex %p", lck);
	}
	if (lck->lck_mtx_tag == LCK_MTX_TAG_DESTROYED) {
		panic("Destroying previously destroyed lock %p", lck);
	}
	lck_mtx_assert(lck, LCK_MTX_ASSERT_NOTOWNED);
	lck->lck_mtx_tag = LCK_MTX_TAG_DESTROYED;
	lck_grp_lckcnt_decr(grp, LCK_TYPE_MTX);
	lck_grp_deallocate(grp);
	return;
}

  
                           
   
void
lck_spin_assert(lck_spin_t *lock, unsigned int type)
{
	thread_t        thread, holder;
	uintptr_t       state;

	if (lock->type != LCK_SPIN_TYPE) {
		panic("Invalid spinlock %p", lock);
	}

	state = lock->lck_spin_data;
	holder = (thread_t)(state & ~LCK_ILOCK);
	thread = current_thread();
	if (type == LCK_ASSERT_OWNED) {
		if (holder == 0) {
			panic("Lock not owned %p = %lx", lock, state);
		}
		if (holder != thread) {
			panic("Lock not owned by current thread %p = %lx", lock, state);
		}
		if ((state & LCK_ILOCK) == 0) {
			panic("Lock bit not set %p = %lx", lock, state);
		}
	} else if (type == LCK_ASSERT_NOTOWNED) {
		if (holder != 0) {
			if (holder == thread) {
				panic("Lock owned by current thread %p = %lx", lock, state);
			}
		}
	} else {
		panic("lck_spin_assert(): invalid arg (%u)", type);
	}
}

boolean_t
lck_rw_lock_yield_shared(lck_rw_t *lck, boolean_t force_yield)
{
	lck_rw_word_t   word;

	lck_rw_assert(lck, LCK_RW_ASSERT_SHARED);

	word.data = ordered_load_rw(lck);
	if (word.want_excl || word.want_upgrade || force_yield) {
		lck_rw_unlock_shared(lck);
		mutex_pause(2);
		lck_rw_lock_shared(lck);
		return TRUE;
	}

	return FALSE;
}

  
                                             
                                                                  
   
boolean_t
kdp_lck_mtx_lock_spin_is_acquired(lck_mtx_t *lck)
{
	uintptr_t       state;

	if (not_in_kdp) {
		panic("panic: spinlock acquired check done outside of kernel debugger");
	}
	state = ordered_load_mtx(lck);
	if (state == LCK_MTX_TAG_DESTROYED) {
		return FALSE;
	}
	if (LCK_MTX_STATE_TO_THREAD(state) || (state & LCK_ILOCK)) {
		return TRUE;
	}
	return FALSE;
}

void
kdp_lck_mtx_find_owner(__unused struct waitq * waitq, event64_t event, thread_waitinfo_t * waitinfo)
{
	lck_mtx_t * mutex = LCK_EVENT_TO_MUTEX(event);
	waitinfo->context = VM_KERNEL_UNSLIDE_OR_PERM(mutex);
	uintptr_t state   = ordered_load_mtx(mutex);
	thread_t holder   = LCK_MTX_STATE_TO_THREAD(state);
	if ((uintptr_t)holder == (uintptr_t)LCK_MTX_SPIN_TAG) {
		waitinfo->owner = STACKSHOT_WAITOWNER_MTXSPIN;
	} else {
		assertf(state != (uintptr_t)LCK_MTX_TAG_DESTROYED, "state=0x%llx", (uint64_t)state);
		assertf(state != (uintptr_t)LCK_MTX_TAG_INDIRECT, "state=0x%llx", (uint64_t)state);
		waitinfo->owner = thread_tid(holder);
	}
}

void
kdp_rwlck_find_owner(__unused struct waitq * waitq, event64_t event, thread_waitinfo_t * waitinfo)
{
	lck_rw_t        *rwlck = NULL;
	switch (waitinfo->wait_type) {
	case kThreadWaitKernelRWLockRead:
		rwlck = READ_EVENT_TO_RWLOCK(event);
		break;
	case kThreadWaitKernelRWLockWrite:
	case kThreadWaitKernelRWLockUpgrade:
		rwlck = WRITE_EVENT_TO_RWLOCK(event);
		break;
	default:
		panic("%s was called with an invalid blocking type", __FUNCTION__);
		break;
	}
	waitinfo->context = VM_KERNEL_UNSLIDE_OR_PERM(rwlck);
	waitinfo->owner = thread_tid(rwlck->lck_rw_owner);
}
