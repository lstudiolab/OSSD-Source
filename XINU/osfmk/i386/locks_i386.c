/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
                    
                                                    
             
  
                                    
   

#define LOCK_PRIVATE 1

#include <mach_ldebug.h>

#include <kern/lock_stat.h>
#include <kern/locks.h>
#include <kern/zalloc.h>
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
#include <kern/cpu_number.h>
#include <os/hash.h>

#if     CONFIG_DTRACE
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

#define LCK_RW_LCK_EX_WRITER_SPIN_CODE  0x106
#define LCK_RW_LCK_EX_WRITER_WAIT_CODE  0x107
#define LCK_RW_LCK_EX_READER_SPIN_CODE  0x108
#define LCK_RW_LCK_EX_READER_WAIT_CODE  0x109
#define LCK_RW_LCK_SHARED_SPIN_CODE     0x110
#define LCK_RW_LCK_SHARED_WAIT_CODE     0x111
#define LCK_RW_LCK_SH_TO_EX_SPIN_CODE   0x112
#define LCK_RW_LCK_SH_TO_EX_WAIT_CODE   0x113


#define ANY_LOCK_DEBUG  (USLOCK_DEBUG || LOCK_DEBUG || MUTEX_DEBUG)

              

#if     USLOCK_DEBUG
  
                              
   
int     uslock_check = 1;
int     max_lock_loops  = 100000000;
decl_simple_lock_data(extern, printf_lock);
decl_simple_lock_data(extern, panic_lock);
#endif                    

extern unsigned int not_in_kdp;

#if !LOCK_STATS
#define usimple_lock_nopreempt(lck, grp) \
	usimple_lock_nopreempt(lck)
#define usimple_lock_try_nopreempt(lck, grp) \
	usimple_lock_try_nopreempt(lck)
#endif
static void usimple_lock_nopreempt(usimple_lock_t, lck_grp_t *);
static unsigned int usimple_lock_try_nopreempt(usimple_lock_t, lck_grp_t *);

  
                                                     
                                                           
                                             
   
typedef void    *pc_t;
#define INVALID_PC      ((void *) VM_MAX_KERNEL_ADDRESS)
#define INVALID_THREAD  ((void *) VM_MAX_KERNEL_ADDRESS)
#if     ANY_LOCK_DEBUG
#define OBTAIN_PC(pc)   ((pc) = GET_RETURN_PC())
#define DECL_PC(pc)     pc_t pc;
#else                       
#define DECL_PC(pc)
#ifdef  lint
  
                                                             
   
#define OBTAIN_PC(pc)   ++pc
#else             
#define OBTAIN_PC(pc)
#endif            
#endif                    

ZONE_VIEW_DEFINE(ZV_LCK_SPIN, "lck_spin",
    KHEAP_ID_DEFAULT, sizeof(lck_spin_t));

ZONE_VIEW_DEFINE(ZV_LCK_MTX, "lck_mtx",
    KHEAP_ID_DEFAULT, sizeof(lck_mtx_t));

ZONE_VIEW_DEFINE(ZV_LCK_MTX_EXT, "lck_mtx_ext",
    KHEAP_ID_DEFAULT, sizeof(lck_mtx_ext_t));

ZONE_VIEW_DEFINE(ZV_LCK_RW, "lck_rw",
    KHEAP_ID_DEFAULT, sizeof(lck_rw_t));

  
                                                                   
                                                                           
                                                                         
                                          
  
                                                                      
                                                    
                                                                                    
   
static uint32_t
atomic_exchange_begin32(uint32_t *target, uint32_t *previous, enum memory_order ord)
{
	uint32_t        val;

	(void)ord;                                              
	val = os_atomic_load(target, relaxed);
	*previous = val;
	return val;
}

static boolean_t
atomic_exchange_complete32(uint32_t *target, uint32_t previous, uint32_t newval, enum memory_order ord)
{
	return __c11_atomic_compare_exchange_strong((_Atomic uint32_t *)target, &previous, newval, ord, memory_order_relaxed);
}

static void
atomic_exchange_abort(void)
{
}

static boolean_t
atomic_test_and_set32(uint32_t *target, uint32_t test_mask, uint32_t set_mask, enum memory_order ord, boolean_t wait)
{
	uint32_t        value, prev;

	for (;;) {
		value = atomic_exchange_begin32(target, &prev, ord);
		if (value & test_mask) {
			if (wait) {
				cpu_pause();
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

  
                                                         
   

#if     USLOCK_DEBUG
#define USLDBG(stmt)    stmt
void            usld_lock_init(usimple_lock_t, unsigned short);
void            usld_lock_pre(usimple_lock_t, pc_t);
void            usld_lock_post(usimple_lock_t, pc_t);
void            usld_unlock(usimple_lock_t, pc_t);
void            usld_lock_try_pre(usimple_lock_t, pc_t);
void            usld_lock_try_post(usimple_lock_t, pc_t);
int             usld_lock_common_checks(usimple_lock_t, char *);
#else                     
#define USLDBG(stmt)
#endif                    

  
                      
   

static void lck_rw_lock_shared_gen(lck_rw_t *lck);
static void lck_rw_lock_exclusive_gen(lck_rw_t *lck);
static boolean_t lck_rw_lock_shared_to_exclusive_success(lck_rw_t *lck);
static boolean_t lck_rw_lock_shared_to_exclusive_failure(lck_rw_t *lck, uint32_t prior_lock_state);
static void lck_rw_lock_exclusive_to_shared_gen(lck_rw_t *lck, uint32_t prior_lock_state);
static lck_rw_type_t lck_rw_done_gen(lck_rw_t *lck, uint32_t prior_lock_state);
void lck_rw_clear_promotions_x86(thread_t thread);
static boolean_t lck_rw_held_read_or_upgrade(lck_rw_t *lock);
static boolean_t lck_rw_grab_want(lck_rw_t *lock);
static boolean_t lck_rw_grab_shared(lck_rw_t *lock);
static void lck_mtx_unlock_wakeup_tail(lck_mtx_t *mutex, uint32_t state, boolean_t indirect);
static void lck_mtx_interlock_lock(lck_mtx_t *mutex, uint32_t *new_state);
static void lck_mtx_interlock_lock_clear_flags(lck_mtx_t *mutex, uint32_t and_flags, uint32_t *new_state);
static int lck_mtx_interlock_try_lock_set_flags(lck_mtx_t *mutex, uint32_t or_flags, uint32_t *new_state);
static boolean_t lck_mtx_lock_wait_interlock_to_clear(lck_mtx_t *lock, uint32_t *new_state);
static boolean_t lck_mtx_try_lock_wait_interlock_to_clear(lck_mtx_t *lock, uint32_t *new_state);


  
                                           
   
lck_spin_t *
lck_spin_alloc_init(
	lck_grp_t       *grp,
	lck_attr_t      *attr)
{
	lck_spin_t *lck;

	lck = zalloc(ZV_LCK_SPIN);
	lck_spin_init(lck, grp, attr);
	return lck;
}

  
                                     
   
void
lck_spin_free(
	lck_spin_t      *lck,
	lck_grp_t       *grp)
{
	lck_spin_destroy(lck, grp);
	zfree(ZV_LCK_SPIN, lck);
}

  
                                     
   
void
lck_spin_init(
	lck_spin_t      *lck,
	lck_grp_t       *grp,
	__unused lck_attr_t     *attr)
{
	usimple_lock_init((usimple_lock_t) lck, 0);
	if (grp) {
		lck_grp_reference(grp);
		lck_grp_lckcnt_incr(grp, LCK_TYPE_SPIN);
	}
}

  
                                        
   
void
lck_spin_destroy(
	lck_spin_t      *lck,
	lck_grp_t       *grp)
{
	if (lck->interlock == LCK_SPIN_TAG_DESTROYED) {
		return;
	}
	lck->interlock = LCK_SPIN_TAG_DESTROYED;
	if (grp) {
		lck_grp_lckcnt_decr(grp, LCK_TYPE_SPIN);
		lck_grp_deallocate(grp);
	}
	return;
}

  
                                     
   
void
lck_spin_lock_grp(
	lck_spin_t      *lck,
	lck_grp_t       *grp)
{
#pragma unused(grp)
	usimple_lock((usimple_lock_t) lck, grp);
}

void
lck_spin_lock(
	lck_spin_t      *lck)
{
	usimple_lock((usimple_lock_t) lck, NULL);
}

void
lck_spin_lock_nopreempt(
	lck_spin_t      *lck)
{
	usimple_lock_nopreempt((usimple_lock_t) lck, NULL);
}

void
lck_spin_lock_nopreempt_grp(
	lck_spin_t      *lck,
	lck_grp_t       *grp)
{
#pragma unused(grp)
	usimple_lock_nopreempt((usimple_lock_t) lck, grp);
}

  
                                       
   
void
lck_spin_unlock(
	lck_spin_t      *lck)
{
	usimple_unlock((usimple_lock_t) lck);
}

void
lck_spin_unlock_nopreempt(
	lck_spin_t      *lck)
{
	usimple_unlock_nopreempt((usimple_lock_t) lck);
}

boolean_t
lck_spin_try_lock_grp(
	lck_spin_t      *lck,
	lck_grp_t       *grp)
{
#pragma unused(grp)
	boolean_t lrval = (boolean_t)usimple_lock_try((usimple_lock_t) lck, grp);
#if     DEVELOPMENT || DEBUG
	if (lrval) {
		pltrace(FALSE);
	}
#endif
	return lrval;
}


  
                                         
   
boolean_t
lck_spin_try_lock(
	lck_spin_t      *lck)
{
	boolean_t lrval = (boolean_t)usimple_lock_try((usimple_lock_t) lck, LCK_GRP_NULL);
#if     DEVELOPMENT || DEBUG
	if (lrval) {
		pltrace(FALSE);
	}
#endif
	return lrval;
}

int
lck_spin_try_lock_nopreempt(
	lck_spin_t      *lck)
{
	boolean_t lrval = (boolean_t)usimple_lock_try_nopreempt((usimple_lock_t) lck, LCK_GRP_NULL);
#if     DEVELOPMENT || DEBUG
	if (lrval) {
		pltrace(FALSE);
	}
#endif
	return lrval;
}

int
lck_spin_try_lock_nopreempt_grp(
	lck_spin_t      *lck,
	lck_grp_t       *grp)
{
#pragma unused(grp)
	boolean_t lrval = (boolean_t)usimple_lock_try_nopreempt((usimple_lock_t) lck, grp);
#if     DEVELOPMENT || DEBUG
	if (lrval) {
		pltrace(FALSE);
	}
#endif
	return lrval;
}

  
                           
   
void
lck_spin_assert(lck_spin_t *lock, unsigned int type)
{
	thread_t thread, holder;
	uintptr_t state;

	if (__improbable(type != LCK_ASSERT_OWNED && type != LCK_ASSERT_NOTOWNED)) {
		panic("lck_spin_assert(): invalid arg (%u)", type);
	}

	state = lock->interlock;
	holder = (thread_t)state;
	thread = current_thread();
	if (type == LCK_ASSERT_OWNED) {
		if (__improbable(holder == THREAD_NULL)) {
			panic("Lock not owned %p = %lx", lock, state);
		}
		if (__improbable(holder != thread)) {
			panic("Lock not owned by current thread %p = %lx", lock, state);
		}
	} else if (type == LCK_ASSERT_NOTOWNED) {
		if (__improbable(holder != THREAD_NULL)) {
			if (holder == thread) {
				panic("Lock owned by current thread %p = %lx", lock, state);
			}
		}
	}
}

  
                                         
                                                                       
                                          
   
boolean_t
kdp_lck_spin_is_acquired(lck_spin_t *lck)
{
	if (not_in_kdp) {
		panic("panic: spinlock acquired check done outside of kernel debugger");
	}
	return (lck->interlock != 0)? TRUE : FALSE;
}

  
                             
  
                                 
   
void
usimple_lock_init(
	usimple_lock_t  l,
	__unused unsigned short tag)
{
	USLDBG(usld_lock_init(l, tag));
	hw_lock_init(&l->interlock);
}

volatile uint32_t spinlock_owner_cpu = ~0;
volatile usimple_lock_t spinlock_timed_out;

uint32_t
spinlock_timeout_NMI(uintptr_t thread_addr)
{
	uint32_t i;

	for (i = 0; i < real_ncpus; i++) {
		if ((cpu_data_ptr[i] != NULL) && ((uintptr_t)cpu_data_ptr[i]->cpu_active_thread == thread_addr)) {
			spinlock_owner_cpu = i;
			if ((uint32_t) cpu_number() != i) {
				                                            
				NMIPI_panic(cpu_to_cpumask(i), SPINLOCK_TIMEOUT);
			}
			break;
		}
	}

	return spinlock_owner_cpu;
}

__abortlike
static void
usimple_lock_acquire_timeout_panic(usimple_lock_t l)
{
	uintptr_t lowner = (uintptr_t)l->interlock.lock_data;
	uint32_t lock_cpu;

	spinlock_timed_out = l;                                         
	lock_cpu = spinlock_timeout_NMI(lowner);
	panic("Spinlock acquisition timed out: lock=%p, "
	    "lock owner thread=0x%lx, current_thread: %p, "
	    "lock owner active on CPU 0x%x, current owner: 0x%lx, time: %llu",
	    l, lowner, current_thread(), lock_cpu,
	    (uintptr_t)l->interlock.lock_data, mach_absolute_time());
}

  
                          
  
                                          
                                                
                                
   
void
(usimple_lock)(
	usimple_lock_t  l
	LCK_GRP_ARG(lck_grp_t *grp))
{
	DECL_PC(pc);

	OBTAIN_PC(pc);
	USLDBG(usld_lock_pre(l, pc));

	while (__improbable(hw_lock_to(&l->interlock, LockTimeOutTSC, grp) == 0)) {
		if (!machine_timeout_suspended()) {
			usimple_lock_acquire_timeout_panic(l);
		}
		enable_preemption();
	}

#if DEVELOPMENT || DEBUG
	pltrace(FALSE);
#endif

	USLDBG(usld_lock_post(l, pc));
#if CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_SPIN_LOCK_ACQUIRE, l, 0, (uintptr_t)LCK_GRP_PROBEARG(grp));
#endif
}

  
                                   
  
                                                     
                                                
                                
   
static void
usimple_lock_nopreempt(
	usimple_lock_t  l,
	lck_grp_t *grp)
{
	DECL_PC(pc);

	OBTAIN_PC(pc);
	USLDBG(usld_lock_pre(l, pc));

	while (__improbable(hw_lock_to_nopreempt(&l->interlock, LockTimeOutTSC, grp) == 0)) {
		if (!machine_timeout_suspended()) {
			usimple_lock_acquire_timeout_panic(l);
		}
		enable_preemption();
	}

#if DEVELOPMENT || DEBUG
	pltrace(FALSE);
#endif

	USLDBG(usld_lock_post(l, pc));
#if CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_SPIN_LOCK_ACQUIRE, l, 0, (uintptr_t)LCK_GRP_PROBEARG(grp));
#endif
}


  
                          
  
                                         
                                                
                                
   
void
usimple_unlock(
	usimple_lock_t  l)
{
	DECL_PC(pc);

	OBTAIN_PC(pc);
	USLDBG(usld_unlock(l, pc));
#if DEVELOPMENT || DEBUG
	pltrace(TRUE);
#endif
	hw_lock_unlock(&l->interlock);
}

  
                                      
  
                                                    
                                                
                                
   
void
usimple_unlock_nopreempt(
	usimple_lock_t  l)
{
	DECL_PC(pc);

	OBTAIN_PC(pc);
	USLDBG(usld_unlock(l, pc));
#if DEVELOPMENT || DEBUG
	pltrace(TRUE);
#endif
	hw_lock_unlock_nopreempt(&l->interlock);
}

  
                                        
  
                                                
                                                        
                                                         
                                                    
  
                                                        
                                                         
                                            
   
unsigned int
usimple_lock_try(
	usimple_lock_t  l,
	lck_grp_t *grp)
{
	unsigned int    success;
	DECL_PC(pc);

	OBTAIN_PC(pc);
	USLDBG(usld_lock_try_pre(l, pc));
	if ((success = hw_lock_try(&l->interlock, grp))) {
#if DEVELOPMENT || DEBUG
		pltrace(FALSE);
#endif
		USLDBG(usld_lock_try_post(l, pc));
	}
	return success;
}

  
                                        
  
                                                     
                                                
                                
  
                                                        
                                                         
                                            
   
static unsigned int
usimple_lock_try_nopreempt(
	usimple_lock_t  l,
	lck_grp_t *grp)
{
	unsigned int    success;
	DECL_PC(pc);

	OBTAIN_PC(pc);
	USLDBG(usld_lock_try_pre(l, pc));
	if ((success = hw_lock_try_nopreempt(&l->interlock, grp))) {
#if DEVELOPMENT || DEBUG
		pltrace(FALSE);
#endif
		USLDBG(usld_lock_try_post(l, pc));
	}
	return success;
}

  
                                                               
                          
  
   
unsigned
int
(usimple_lock_try_lock_mp_signal_safe_loop_deadline)(usimple_lock_t l,
    uint64_t deadline
    LCK_GRP_ARG(lck_grp_t *grp))
{
	boolean_t istate = ml_get_interrupts_enabled();

	if (deadline < mach_absolute_time()) {
		return 0;
	}

	while (!simple_lock_try(l, grp)) {
		if (!istate) {
			cpu_signal_handler(NULL);
		}

		if (deadline < mach_absolute_time()) {
			return 0;
		}

		cpu_pause();
	}

	return 1;
}

void
(usimple_lock_try_lock_loop)(usimple_lock_t l
    LCK_GRP_ARG(lck_grp_t *grp))
{
	                                                           
	if (!simple_lock_try(l, grp)) {
		usimple_lock_try_lock_mp_signal_safe_loop_deadline(l, ULLONG_MAX, grp);
	}
}

unsigned
int
(usimple_lock_try_lock_mp_signal_safe_loop_duration)(usimple_lock_t l,
    uint64_t duration
    LCK_GRP_ARG(lck_grp_t *grp))
{
	uint64_t deadline;
	uint64_t base_at;
	uint64_t duration_at;

	                                      
	if (simple_lock_try(l, grp)) {
		return 1;
	}

	base_at = mach_absolute_time();

	nanoseconds_to_absolutetime(duration, &duration_at);
	deadline = base_at + duration_at;
	if (deadline < base_at) {
		                                               
		deadline = ULLONG_MAX;
	}

	return usimple_lock_try_lock_mp_signal_safe_loop_deadline(l, deadline, grp);
}

#if     USLOCK_DEBUG
  
                                                           
                                                      
   
#define USLOCK_CHECKED          0x0001                                     
#define USLOCK_TAKEN            0x0002                                   
#define USLOCK_INIT             0xBAA0                                         
#define USLOCK_INITIALIZED      (USLOCK_INIT|USLOCK_CHECKED)
#define USLOCK_CHECKING(l)      (uslock_check &&                        \
	                         ((l)->debug.state & USLOCK_CHECKED))

  
                                                 
                     
   
void
usld_lock_init(
	usimple_lock_t  l,
	__unused unsigned short tag)
{
	if (l == USIMPLE_LOCK_NULL) {
		panic("lock initialization:  null lock pointer");
	}
	l->lock_type = USLOCK_TAG;
	l->debug.state = uslock_check ? USLOCK_INITIALIZED : 0;
	l->debug.lock_cpu = l->debug.unlock_cpu = 0;
	l->debug.lock_pc = l->debug.unlock_pc = INVALID_PC;
	l->debug.lock_thread = l->debug.unlock_thread = INVALID_THREAD;
	l->debug.duration[0] = l->debug.duration[1] = 0;
	l->debug.unlock_cpu = l->debug.unlock_cpu = 0;
	l->debug.unlock_pc = l->debug.unlock_pc = INVALID_PC;
	l->debug.unlock_thread = l->debug.unlock_thread = INVALID_THREAD;
}


  
                                                    
                                       
   
int
usld_lock_common_checks(
	usimple_lock_t  l,
	char            *caller)
{
	if (l == USIMPLE_LOCK_NULL) {
		panic("%s:  null lock pointer", caller);
	}
	if (l->lock_type != USLOCK_TAG) {
		panic("%s:  %p is not a usimple lock, 0x%x", caller, l, l->lock_type);
	}
	if (!(l->debug.state & USLOCK_INIT)) {
		panic("%s:  %p is not an initialized lock, 0x%x", caller, l, l->debug.state);
	}
	return USLOCK_CHECKING(l);
}


  
                                                        
                 
   
              
void
usld_lock_pre(
	usimple_lock_t  l,
	pc_t            pc)
{
	char    caller[] = "usimple_lock";


	if (!usld_lock_common_checks(l, caller)) {
		return;
	}

  
                                                                          
                                                                        
                                                                             
                                                                       
                                               
   

	if ((l->debug.state & USLOCK_TAKEN) && l->debug.lock_thread &&
	    l->debug.lock_thread == (void *) current_thread()) {
		printf("%s:  lock %p already locked (at %p) by",
		    caller, l, l->debug.lock_pc);
		printf(" current thread %p (new attempt at pc %p)\n",
		    l->debug.lock_thread, pc);
		panic("%s", caller);
	}
	mp_disable_preemption();
	mp_enable_preemption();
}


  
                                                          
  
                                               
                                      
   
void
usld_lock_post(
	usimple_lock_t  l,
	pc_t            pc)
{
	unsigned int mycpu;
	char    caller[] = "successful usimple_lock";


	if (!usld_lock_common_checks(l, caller)) {
		return;
	}

	if (!((l->debug.state & ~USLOCK_TAKEN) == USLOCK_INITIALIZED)) {
		panic("%s:  lock %p became uninitialized",
		    caller, l);
	}
	if ((l->debug.state & USLOCK_TAKEN)) {
		panic("%s:  lock 0x%p became TAKEN by someone else",
		    caller, l);
	}

	mycpu = (unsigned int)cpu_number();
	assert(mycpu <= UCHAR_MAX);

	l->debug.lock_thread = (void *)current_thread();
	l->debug.state |= USLOCK_TAKEN;
	l->debug.lock_pc = pc;
	l->debug.lock_cpu = (unsigned char)mycpu;
}


  
                                             
                                              
                                  
  
                                           
                               
   
void
usld_unlock(
	usimple_lock_t  l,
	pc_t            pc)
{
	unsigned int mycpu;
	char    caller[] = "usimple_unlock";


	if (!usld_lock_common_checks(l, caller)) {
		return;
	}

	mycpu = cpu_number();
	assert(mycpu <= UCHAR_MAX);

	if (!(l->debug.state & USLOCK_TAKEN)) {
		panic("%s:  lock 0x%p hasn't been taken",
		    caller, l);
	}
	if (l->debug.lock_thread != (void *) current_thread()) {
		panic("%s:  unlocking lock 0x%p, owned by thread %p",
		    caller, l, l->debug.lock_thread);
	}
	if (l->debug.lock_cpu != mycpu) {
		printf("%s:  unlocking lock 0x%p on cpu 0x%x",
		    caller, l, mycpu);
		printf(" (acquired on cpu 0x%x)\n", l->debug.lock_cpu);
		panic("%s", caller);
	}

	l->debug.unlock_thread = l->debug.lock_thread;
	l->debug.lock_thread = INVALID_PC;
	l->debug.state &= ~USLOCK_TAKEN;
	l->debug.unlock_pc = pc;
	l->debug.unlock_cpu = (unsigned char)mycpu;
}


  
                                             
                            
  
                                              
   
void
usld_lock_try_pre(
	usimple_lock_t  l,
	__unused pc_t   pc)
{
	char    caller[] = "usimple_lock_try";

	if (!usld_lock_common_checks(l, caller)) {
		return;
	}
}


  
                                            
                                         
  
                                      
                                         
                     
   
void
usld_lock_try_post(
	usimple_lock_t  l,
	pc_t            pc)
{
	unsigned int mycpu;
	char    caller[] = "successful usimple_lock_try";

	if (!usld_lock_common_checks(l, caller)) {
		return;
	}

	if (!((l->debug.state & ~USLOCK_TAKEN) == USLOCK_INITIALIZED)) {
		panic("%s:  lock 0x%p became uninitialized",
		    caller, l);
	}
	if ((l->debug.state & USLOCK_TAKEN)) {
		panic("%s:  lock 0x%p became TAKEN by someone else",
		    caller, l);
	}

	mycpu = cpu_number();
	assert(mycpu <= UCHAR_MAX);

	l->debug.lock_thread = (void *) current_thread();
	l->debug.state |= USLOCK_TAKEN;
	l->debug.lock_pc = pc;
	l->debug.lock_cpu = (unsigned char)mycpu;
}
#endif                    

  
                                         
   
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
	lck_attr_t      *lck_attr = (attr != LCK_ATTR_NULL) ?
	    attr : &LockDefaultLckAttr;

	hw_lock_byte_init(&lck->lck_rw_interlock);
	lck->lck_rw_want_write = FALSE;
	lck->lck_rw_want_upgrade = FALSE;
	lck->lck_rw_shared_count = 0;
	lck->lck_rw_can_sleep = TRUE;
	lck->lck_r_waiting = lck->lck_w_waiting = 0;
	lck->lck_rw_tag = 0;
	lck->lck_rw_priv_excl = ((lck_attr->lck_attr_val &
	    LCK_ATTR_RW_SHARED_PRIORITY) == 0);

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

  
                                                                
                                                                
                                                     
   

#define DECREMENTER_TIMEOUT 1000000

  
                                                                     
                                         
                                                                             
   
static inline boolean_t
lck_interlock_lock(lck_rw_t *lck)
{
	boolean_t       istate;

	istate = ml_set_interrupts_enabled(FALSE);
	hw_lock_byte_lock(&lck->lck_rw_interlock);
	return istate;
}

static inline void
lck_interlock_unlock(lck_rw_t *lck, boolean_t istate)
{
	hw_lock_byte_unlock(&lck->lck_rw_interlock);
	ml_set_interrupts_enabled(istate);
}

  
                                                        
                                                                  
                                                   
                                                                   
   
static inline void
lck_rw_lock_pause(boolean_t interrupts_enabled)
{
	if (!interrupts_enabled) {
		handle_pending_TLB_flushes();
	}
	cpu_pause();
}

static inline boolean_t
lck_rw_held_read_or_upgrade(lck_rw_t *lock)
{
	if (ordered_load(&lock->data) & (LCK_RW_SHARED_MASK | LCK_RW_INTERLOCK | LCK_RW_WANT_UPGRADE)) {
		return TRUE;
	}
	return FALSE;
}

  
                                            
                                              
   
static inline uint64_t
lck_rw_deadline_for_spin(lck_rw_t *lck)
{
	if (lck->lck_rw_can_sleep) {
		if (lck->lck_r_waiting || lck->lck_w_waiting || lck->lck_rw_shared_count > machine_info.max_cpus) {
			  
                                                            
                                                                    
                                                                                 
          
                                                                                      
                                                                                           
                                                                                   
                      
      
			return mach_absolute_time();
		}
		return mach_absolute_time() + MutexSpin;
	} else {
		return mach_absolute_time() + (100000LL * 1000000000LL);
	}
}


  
                                
   

static inline void
lck_rw_interlock_spin(lck_rw_t *lock)
{
	while (ordered_load(&lock->data) & LCK_RW_INTERLOCK) {
		cpu_pause();
	}
}

static boolean_t
lck_rw_grab_want(lck_rw_t *lock)
{
	uint32_t        data, prev;

	for (;;) {
		data = atomic_exchange_begin32(&lock->data, &prev, memory_order_relaxed);
		if ((data & LCK_RW_INTERLOCK) == 0) {
			break;
		}
		atomic_exchange_abort();
		lck_rw_interlock_spin(lock);
	}
	if (data & LCK_RW_WANT_WRITE) {
		atomic_exchange_abort();
		return FALSE;
	}
	data |= LCK_RW_WANT_WRITE;
	return atomic_exchange_complete32(&lock->data, prev, data, memory_order_relaxed);
}

static boolean_t
lck_rw_grab_shared(lck_rw_t *lock)
{
	uint32_t        data, prev;

	for (;;) {
		data = atomic_exchange_begin32(&lock->data, &prev, memory_order_acquire_smp);
		if ((data & LCK_RW_INTERLOCK) == 0) {
			break;
		}
		atomic_exchange_abort();
		lck_rw_interlock_spin(lock);
	}
	if (data & (LCK_RW_WANT_WRITE | LCK_RW_WANT_UPGRADE)) {
		if (((data & LCK_RW_SHARED_MASK) == 0) || (data & LCK_RW_PRIV_EXCL)) {
			atomic_exchange_abort();
			return FALSE;
		}
	}
	data += LCK_RW_SHARED_READER;
	return atomic_exchange_complete32(&lock->data, prev, data, memory_order_acquire_smp);
}

  
                                             
   
static void
lck_rw_lock_exclusive_gen(
	lck_rw_t        *lck)
{
	__kdebug_only uintptr_t trace_lck = unslide_for_kdebug(lck);
	uint64_t        deadline = 0;
	int             slept = 0;
	int             gotlock = 0;
	int             lockheld = 0;
	wait_result_t   res = 0;
	boolean_t       istate = -1;

#if     CONFIG_DTRACE
	boolean_t dtrace_ls_initialized = FALSE;
	boolean_t dtrace_rwl_excl_spin, dtrace_rwl_excl_block, dtrace_ls_enabled = FALSE;
	uint64_t wait_interval = 0;
	int readers_at_sleep = 0;
#endif

	  
                                             
    
	while (!lck_rw_grab_want(lck)) {
#if     CONFIG_DTRACE
		if (dtrace_ls_initialized == FALSE) {
			dtrace_ls_initialized = TRUE;
			dtrace_rwl_excl_spin = (lockstat_probemap[LS_LCK_RW_LOCK_EXCL_SPIN] != 0);
			dtrace_rwl_excl_block = (lockstat_probemap[LS_LCK_RW_LOCK_EXCL_BLOCK] != 0);
			dtrace_ls_enabled = dtrace_rwl_excl_spin || dtrace_rwl_excl_block;
			if (dtrace_ls_enabled) {
				  
                                                
                                                 
       
				readers_at_sleep = lck->lck_rw_shared_count;
				wait_interval = mach_absolute_time();
			}
		}
#endif
		if (istate == -1) {
			istate = ml_get_interrupts_enabled();
		}

		deadline = lck_rw_deadline_for_spin(lck);

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_WRITER_SPIN_CODE) | DBG_FUNC_START, trace_lck, 0, 0, 0, 0);

		while (((gotlock = lck_rw_grab_want(lck)) == 0) && mach_absolute_time() < deadline) {
			lck_rw_lock_pause(istate);
		}

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_WRITER_SPIN_CODE) | DBG_FUNC_END, trace_lck, 0, 0, gotlock, 0);

		if (gotlock) {
			break;
		}
		  
                                                    
                                            
                                                       
     
		if (lck->lck_rw_can_sleep) {
			istate = lck_interlock_lock(lck);

			if (lck->lck_rw_want_write) {
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_WRITER_WAIT_CODE) | DBG_FUNC_START, trace_lck, 0, 0, 0, 0);

				lck->lck_w_waiting = TRUE;

				thread_set_pending_block_hint(current_thread(), kThreadWaitKernelRWLockWrite);
				res = assert_wait(RW_LOCK_WRITER_EVENT(lck),
				    THREAD_UNINT | THREAD_WAIT_NOREPORT_USER);
				lck_interlock_unlock(lck, istate);

				if (res == THREAD_WAITING) {
					res = thread_block(THREAD_CONTINUE_NULL);
					slept++;
				}
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_WRITER_WAIT_CODE) | DBG_FUNC_END, trace_lck, res, slept, 0, 0);
			} else {
				lck->lck_rw_want_write = TRUE;
				lck_interlock_unlock(lck, istate);
				break;
			}
		}
	}
	  
                                                
                                                                  
                                                 
                                                                  
                                                                 
                                                              
                                                         
                                                                      
                                                      
    
	while (lck_rw_held_read_or_upgrade(lck)) {
#if     CONFIG_DTRACE
		  
                                                    
                                                      
                                                         
                                                   
     
		if (dtrace_ls_initialized == FALSE) {
			dtrace_ls_initialized = TRUE;
			dtrace_rwl_excl_spin = (lockstat_probemap[LS_LCK_RW_LOCK_EXCL_SPIN] != 0);
			dtrace_rwl_excl_block = (lockstat_probemap[LS_LCK_RW_LOCK_EXCL_BLOCK] != 0);
			dtrace_ls_enabled = dtrace_rwl_excl_spin || dtrace_rwl_excl_block;
			if (dtrace_ls_enabled) {
				  
                                                
                                                 
       
				readers_at_sleep = lck->lck_rw_shared_count;
				wait_interval = mach_absolute_time();
			}
		}
#endif
		if (istate == -1) {
			istate = ml_get_interrupts_enabled();
		}

		deadline = lck_rw_deadline_for_spin(lck);

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_READER_SPIN_CODE) | DBG_FUNC_START, trace_lck, 0, 0, 0, 0);

		while ((lockheld = lck_rw_held_read_or_upgrade(lck)) && mach_absolute_time() < deadline) {
			lck_rw_lock_pause(istate);
		}

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_READER_SPIN_CODE) | DBG_FUNC_END, trace_lck, 0, 0, lockheld, 0);

		if (!lockheld) {
			break;
		}
		  
                                                    
                                            
                                                       
     
		if (lck->lck_rw_can_sleep) {
			istate = lck_interlock_lock(lck);

			if (lck->lck_rw_shared_count != 0 || lck->lck_rw_want_upgrade) {
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_READER_WAIT_CODE) | DBG_FUNC_START, trace_lck, 0, 0, 0, 0);

				lck->lck_w_waiting = TRUE;

				thread_set_pending_block_hint(current_thread(), kThreadWaitKernelRWLockWrite);
				res = assert_wait(RW_LOCK_WRITER_EVENT(lck),
				    THREAD_UNINT | THREAD_WAIT_NOREPORT_USER);
				lck_interlock_unlock(lck, istate);

				if (res == THREAD_WAITING) {
					res = thread_block(THREAD_CONTINUE_NULL);
					slept++;
				}
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_READER_WAIT_CODE) | DBG_FUNC_END, trace_lck, res, slept, 0, 0);
			} else {
				lck_interlock_unlock(lck, istate);
				  
                                                  
                                                    
                                                          
       
				break;
			}
		}
	}

#if     CONFIG_DTRACE
	  
                                                             
                                                               
                                                                 
                                                           
                   
                                                                           
                                                            
    
	if (dtrace_ls_enabled == TRUE) {
		if (slept == 0) {
			LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_SPIN, lck,
			    mach_absolute_time() - wait_interval, 1);
		} else {
			  
                                                              
                                                          
                                                          
                                             
      
			LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_BLOCK, lck,
			    mach_absolute_time() - wait_interval, 1,
			    (readers_at_sleep == 0 ? 1 : 0), readers_at_sleep);
		}
	}
	LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_ACQUIRE, lck, 1);
#endif
}

  
                                   
   

lck_rw_type_t
lck_rw_done(lck_rw_t *lock)
{
	uint32_t        data, prev;

	for (;;) {
		data = atomic_exchange_begin32(&lock->data, &prev, memory_order_release_smp);
		if (data & LCK_RW_INTERLOCK) {                                           
			atomic_exchange_abort();
			lck_rw_interlock_spin(lock);
			continue;
		}
		if (data & LCK_RW_SHARED_MASK) {
			data -= LCK_RW_SHARED_READER;
			if ((data & LCK_RW_SHARED_MASK) == 0) {                                                           
				goto check_waiters;
			}
		} else {                                                                                          
			if (data & LCK_RW_WANT_UPGRADE) {
				data &= ~(LCK_RW_WANT_UPGRADE);
			} else {
				if (data & LCK_RW_WANT_WRITE) {
					data &= ~(LCK_RW_WANT_EXCL);
				} else {                                                                
					panic("Releasing non-exclusive RW lock without a reader refcount!");
				}
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
		if (atomic_exchange_complete32(&lock->data, prev, data, memory_order_release_smp)) {
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
	lck_rw_t        *fake_lck;
	lck_rw_type_t   lock_type;
	thread_t        thread;
	uint32_t        rwlock_count;

	thread = current_thread();
	rwlock_count = thread->rwlock_count--;
	fake_lck = (lck_rw_t *)&prior_lock_state;

	if (lck->lck_rw_can_sleep) {
		  
                                                          
                                                      
                                                           
                                    
     

		if (fake_lck->lck_rw_shared_count <= 1) {
			if (fake_lck->lck_w_waiting) {
				thread_wakeup(RW_LOCK_WRITER_EVENT(lck));
			}

			if (!(fake_lck->lck_rw_priv_excl && fake_lck->lck_w_waiting) && fake_lck->lck_r_waiting) {
				thread_wakeup(RW_LOCK_READER_EVENT(lck));
			}
		}
#if MACH_LDEBUG
		if (rwlock_count == 0) {
			panic("rw lock count underflow for thread %p", thread);
		}
#endif
		                                                                

		if ((rwlock_count == 1                  ) && (thread->sched_flags & TH_SFLAG_RW_PROMOTED)) {
			                                                                            
			lck_rw_clear_promotion(thread, unslide_for_kdebug(lck));
		}
	}
	if (fake_lck->lck_rw_shared_count) {
		lock_type = LCK_RW_TYPE_SHARED;
	} else {
		lock_type = LCK_RW_TYPE_EXCLUSIVE;
	}

#if CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_RW_DONE_RELEASE, lck, lock_type == LCK_RW_TYPE_SHARED ? 0 : 1);
#endif

	return lock_type;
}


  
                         
   
void
lck_rw_unlock(
	lck_rw_t        *lck,
	lck_rw_type_t   lck_rw_type)
{
	if (lck_rw_type == LCK_RW_TYPE_SHARED) {
		lck_rw_unlock_shared(lck);
	} else if (lck_rw_type == LCK_RW_TYPE_EXCLUSIVE) {
		lck_rw_unlock_exclusive(lck);
	} else {
		panic("lck_rw_unlock(): Invalid RW lock type: %d\n", lck_rw_type);
	}
}


  
                                
   
void
lck_rw_unlock_shared(
	lck_rw_t        *lck)
{
	lck_rw_type_t   ret;

	assertf(lck->lck_rw_shared_count > 0, "lck %p has shared_count=0x%x", lck, lck->lck_rw_shared_count);
	ret = lck_rw_done(lck);

	if (ret != LCK_RW_TYPE_SHARED) {
		panic("lck_rw_unlock_shared(): lock %p held in mode: %d\n", lck, ret);
	}
}


  
                                   
   
void
lck_rw_unlock_exclusive(
	lck_rw_t        *lck)
{
	lck_rw_type_t   ret;

	ret = lck_rw_done(lck);

	if (ret != LCK_RW_TYPE_EXCLUSIVE) {
		panic("lck_rw_unlock_exclusive(): lock held in mode: %d\n", ret);
	}
}


  
                       
   
void
lck_rw_lock(
	lck_rw_t        *lck,
	lck_rw_type_t   lck_rw_type)
{
	if (lck_rw_type == LCK_RW_TYPE_SHARED) {
		lck_rw_lock_shared(lck);
	} else if (lck_rw_type == LCK_RW_TYPE_EXCLUSIVE) {
		lck_rw_lock_exclusive(lck);
	} else {
		panic("lck_rw_lock(): Invalid RW lock type: %x\n", lck_rw_type);
	}
}

  
                              
   
void
lck_rw_lock_shared(lck_rw_t *lock)
{
	uint32_t        data, prev;

	current_thread()->rwlock_count++;
	for (;;) {
		data = atomic_exchange_begin32(&lock->data, &prev, memory_order_acquire_smp);
		if (data & (LCK_RW_WANT_EXCL | LCK_RW_WANT_UPGRADE | LCK_RW_INTERLOCK)) {
			atomic_exchange_abort();
			if (lock->lck_rw_can_sleep) {
				lck_rw_lock_shared_gen(lock);
			} else {
				cpu_pause();
				continue;
			}
			break;
		}
		data += LCK_RW_SHARED_READER;
		if (atomic_exchange_complete32(&lock->data, prev, data, memory_order_acquire_smp)) {
			break;
		}
		cpu_pause();
	}
#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_ACQUIRE, lock, DTRACE_RW_SHARED);
#endif                     
	return;
}

  
                                  
            
                                                         
                                                      
                                                    
   
static void
lck_rw_lock_shared_gen(
	lck_rw_t        *lck)
{
	__kdebug_only uintptr_t trace_lck = unslide_for_kdebug(lck);
	uint64_t        deadline = 0;
	int             gotlock = 0;
	int             slept = 0;
	wait_result_t   res = 0;
	boolean_t       istate = -1;

#if     CONFIG_DTRACE
	uint64_t wait_interval = 0;
	int readers_at_sleep = 0;
	boolean_t dtrace_ls_initialized = FALSE;
	boolean_t dtrace_rwl_shared_spin, dtrace_rwl_shared_block, dtrace_ls_enabled = FALSE;
#endif

	while (!lck_rw_grab_shared(lck)) {
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
		if (istate == -1) {
			istate = ml_get_interrupts_enabled();
		}

		deadline = lck_rw_deadline_for_spin(lck);

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SHARED_SPIN_CODE) | DBG_FUNC_START,
		    trace_lck, lck->lck_rw_want_write, lck->lck_rw_want_upgrade, 0, 0);

		while (((gotlock = lck_rw_grab_shared(lck)) == 0) && mach_absolute_time() < deadline) {
			lck_rw_lock_pause(istate);
		}

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SHARED_SPIN_CODE) | DBG_FUNC_END,
		    trace_lck, lck->lck_rw_want_write, lck->lck_rw_want_upgrade, gotlock, 0);

		if (gotlock) {
			break;
		}
		  
                                                    
                                         
                                                       
     
		if (lck->lck_rw_can_sleep) {
			istate = lck_interlock_lock(lck);

			if ((lck->lck_rw_want_write || lck->lck_rw_want_upgrade) &&
			    ((lck->lck_rw_shared_count == 0) || lck->lck_rw_priv_excl)) {
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SHARED_WAIT_CODE) | DBG_FUNC_START,
				    trace_lck, lck->lck_rw_want_write, lck->lck_rw_want_upgrade, 0, 0);

				lck->lck_r_waiting = TRUE;

				thread_set_pending_block_hint(current_thread(), kThreadWaitKernelRWLockRead);
				res = assert_wait(RW_LOCK_READER_EVENT(lck),
				    THREAD_UNINT | THREAD_WAIT_NOREPORT_USER);
				lck_interlock_unlock(lck, istate);

				if (res == THREAD_WAITING) {
					res = thread_block(THREAD_CONTINUE_NULL);
					slept++;
				}
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SHARED_WAIT_CODE) | DBG_FUNC_END,
				    trace_lck, res, slept, 0, 0);
			} else {
				lck->lck_rw_shared_count++;
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

#define LCK_RW_LOCK_EXCLUSIVE_TAS(lck) (atomic_test_and_set32(&(lck)->data, \
	    (LCK_RW_SHARED_MASK | LCK_RW_WANT_EXCL | LCK_RW_WANT_UPGRADE | LCK_RW_INTERLOCK), \
	    LCK_RW_WANT_EXCL, memory_order_acquire_smp, FALSE))

  
                                                 
   

bool
lck_rw_lock_exclusive_check_contended(lck_rw_t *lock)
{
	bool contended = false;
	current_thread()->rwlock_count++;
	if (LCK_RW_LOCK_EXCLUSIVE_TAS(lock)) {
#if     CONFIG_DTRACE
		LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_ACQUIRE, lock, DTRACE_RW_EXCL);
#endif                     
	} else {
		contended = true;
		lck_rw_lock_exclusive_gen(lock);
	}
	return contended;
}

  
                                 
   

void
lck_rw_lock_exclusive(lck_rw_t *lock)
{
	current_thread()->rwlock_count++;
	if (LCK_RW_LOCK_EXCLUSIVE_TAS(lock)) {
#if     CONFIG_DTRACE
		LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_ACQUIRE, lock, DTRACE_RW_EXCL);
#endif                     
	} else {
		lck_rw_lock_exclusive_gen(lock);
	}
}


  
                                           
  
                                                                        
   

boolean_t
lck_rw_lock_shared_to_exclusive(lck_rw_t *lock)
{
	uint32_t        data, prev;

	for (;;) {
		data = atomic_exchange_begin32(&lock->data, &prev, memory_order_acquire_smp);
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
			if (atomic_exchange_complete32(&lock->data, prev, data, memory_order_acquire_smp)) {
				return lck_rw_lock_shared_to_exclusive_failure(lock, prev);
			}
		} else {
			data |= LCK_RW_WANT_UPGRADE;                                      
			data -= LCK_RW_SHARED_READER;                                        
			if (atomic_exchange_complete32(&lock->data, prev, data, memory_order_acquire_smp)) {
				break;
			}
		}
		cpu_pause();
	}
	                                 
	if (data & LCK_RW_SHARED_MASK) {                                                            
		lck_rw_lock_shared_to_exclusive_success(lock);                                  
	}
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
	lck_rw_t        *fake_lck;
	thread_t        thread = current_thread();
	uint32_t        rwlock_count;

	                                                                
	rwlock_count = thread->rwlock_count--;
#if MACH_LDEBUG
	if (rwlock_count == 0) {
		panic("rw lock count underflow for thread %p", thread);
	}
#endif
	fake_lck = (lck_rw_t *)&prior_lock_state;

	if (fake_lck->lck_w_waiting && fake_lck->lck_rw_shared_count == 1) {
		  
                                        
                                             
                                   
     
		thread_wakeup(RW_LOCK_WRITER_EVENT(lck));
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
	lck_rw_t        *lck)
{
	__kdebug_only uintptr_t trace_lck = unslide_for_kdebug(lck);
	uint64_t        deadline = 0;
	int             slept = 0;
	int             still_shared = 0;
	wait_result_t   res;
	boolean_t       istate = -1;

#if     CONFIG_DTRACE
	uint64_t wait_interval = 0;
	int readers_at_sleep = 0;
	boolean_t dtrace_ls_initialized = FALSE;
	boolean_t dtrace_rwl_shared_to_excl_spin, dtrace_rwl_shared_to_excl_block, dtrace_ls_enabled = FALSE;
#endif

	while (lck->lck_rw_shared_count != 0) {
#if     CONFIG_DTRACE
		if (dtrace_ls_initialized == FALSE) {
			dtrace_ls_initialized = TRUE;
			dtrace_rwl_shared_to_excl_spin = (lockstat_probemap[LS_LCK_RW_LOCK_SHARED_TO_EXCL_SPIN] != 0);
			dtrace_rwl_shared_to_excl_block = (lockstat_probemap[LS_LCK_RW_LOCK_SHARED_TO_EXCL_BLOCK] != 0);
			dtrace_ls_enabled = dtrace_rwl_shared_to_excl_spin || dtrace_rwl_shared_to_excl_block;
			if (dtrace_ls_enabled) {
				  
                                                
                                                 
       
				readers_at_sleep = lck->lck_rw_shared_count;
				wait_interval = mach_absolute_time();
			}
		}
#endif
		if (istate == -1) {
			istate = ml_get_interrupts_enabled();
		}

		deadline = lck_rw_deadline_for_spin(lck);

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX_SPIN_CODE) | DBG_FUNC_START,
		    trace_lck, lck->lck_rw_shared_count, 0, 0, 0);

		while ((still_shared = lck->lck_rw_shared_count) && mach_absolute_time() < deadline) {
			lck_rw_lock_pause(istate);
		}

		KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX_SPIN_CODE) | DBG_FUNC_END,
		    trace_lck, lck->lck_rw_shared_count, 0, 0, 0);

		if (!still_shared) {
			break;
		}
		  
                                                 
                                            
                                                       
     
		if (lck->lck_rw_can_sleep) {
			istate = lck_interlock_lock(lck);

			if (lck->lck_rw_shared_count != 0) {
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX_WAIT_CODE) | DBG_FUNC_START,
				    trace_lck, lck->lck_rw_shared_count, 0, 0, 0);

				lck->lck_w_waiting = TRUE;

				thread_set_pending_block_hint(current_thread(), kThreadWaitKernelRWLockUpgrade);
				res = assert_wait(RW_LOCK_WRITER_EVENT(lck),
				    THREAD_UNINT | THREAD_WAIT_NOREPORT_USER);
				lck_interlock_unlock(lck, istate);

				if (res == THREAD_WAITING) {
					res = thread_block(THREAD_CONTINUE_NULL);
					slept++;
				}
				KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_SH_TO_EX_WAIT_CODE) | DBG_FUNC_END,
				    trace_lck, res, slept, 0, 0);
			} else {
				lck_interlock_unlock(lck, istate);
				break;
			}
		}
	}
#if     CONFIG_DTRACE
	  
                                                                                    
    
	if (dtrace_ls_enabled == TRUE) {
		if (slept == 0) {
			LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_TO_EXCL_SPIN, lck, mach_absolute_time() - wait_interval, 0);
		} else {
			LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_TO_EXCL_BLOCK, lck,
			    mach_absolute_time() - wait_interval, 1,
			    (readers_at_sleep == 0 ? 1 : 0), readers_at_sleep);
		}
	}
	LOCKSTAT_RECORD(LS_LCK_RW_LOCK_SHARED_TO_EXCL_UPGRADE, lck, 1);
#endif
	return TRUE;
}

  
                                           
   

void
lck_rw_lock_exclusive_to_shared(lck_rw_t *lock)
{
	uint32_t        data, prev;

	for (;;) {
		data = atomic_exchange_begin32(&lock->data, &prev, memory_order_release_smp);
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
		if (atomic_exchange_complete32(&lock->data, prev, data, memory_order_release_smp)) {
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
	__kdebug_only uintptr_t trace_lck = unslide_for_kdebug(lck);
	lck_rw_t                *fake_lck;

	fake_lck = (lck_rw_t *)&prior_lock_state;

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_TO_SH_CODE) | DBG_FUNC_START,
	    trace_lck, fake_lck->lck_rw_want_write, fake_lck->lck_rw_want_upgrade, 0, 0);

	  
                                                             
                                                                 
                              
   
                                                                     
                                                                      
    
	if (!(fake_lck->lck_rw_priv_excl && fake_lck->lck_w_waiting) && fake_lck->lck_r_waiting) {
		thread_wakeup(RW_LOCK_READER_EVENT(lck));
	}

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_RW_LCK_EX_TO_SH_CODE) | DBG_FUNC_END,
	    trace_lck, lck->lck_rw_want_write, lck->lck_rw_want_upgrade, lck->lck_rw_shared_count, 0);

#if CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_RW_LOCK_EXCL_TO_SHARED_DOWNGRADE, lck, 0);
#endif
}


  
                                       
   
boolean_t
lck_rw_try_lock(
	lck_rw_t        *lck,
	lck_rw_type_t   lck_rw_type)
{
	if (lck_rw_type == LCK_RW_TYPE_SHARED) {
		return lck_rw_try_lock_shared(lck);
	} else if (lck_rw_type == LCK_RW_TYPE_EXCLUSIVE) {
		return lck_rw_try_lock_exclusive(lck);
	} else {
		panic("lck_rw_try_lock(): Invalid rw lock type: %x\n", lck_rw_type);
	}
	return FALSE;
}

  
                                  
   

boolean_t
lck_rw_try_lock_shared(lck_rw_t *lock)
{
	uint32_t        data, prev;

	for (;;) {
		data = atomic_exchange_begin32(&lock->data, &prev, memory_order_acquire_smp);
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
		if (atomic_exchange_complete32(&lock->data, prev, data, memory_order_acquire_smp)) {
			break;
		}
		cpu_pause();
	}
	current_thread()->rwlock_count++;
	                                                                                          
#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_RW_TRY_LOCK_SHARED_ACQUIRE, lock, DTRACE_RW_SHARED);
#endif                     
	return TRUE;
}


  
                                     
   

boolean_t
lck_rw_try_lock_exclusive(lck_rw_t *lock)
{
	uint32_t        data, prev;

	for (;;) {
		data = atomic_exchange_begin32(&lock->data, &prev, memory_order_acquire_smp);
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
		if (atomic_exchange_complete32(&lock->data, prev, data, memory_order_acquire_smp)) {
			break;
		}
		cpu_pause();
	}

	current_thread()->rwlock_count++;
#if     CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_RW_TRY_LOCK_EXCL_ACQUIRE, lock, DTRACE_RW_EXCL);
#endif                     
	return TRUE;
}


void
lck_rw_assert(
	lck_rw_t        *lck,
	unsigned int    type)
{
	switch (type) {
	case LCK_RW_ASSERT_SHARED:
		if (lck->lck_rw_shared_count != 0) {
			return;
		}
		break;
	case LCK_RW_ASSERT_EXCLUSIVE:
		if ((lck->lck_rw_want_write ||
		    lck->lck_rw_want_upgrade) &&
		    lck->lck_rw_shared_count == 0) {
			return;
		}
		break;
	case LCK_RW_ASSERT_HELD:
		if (lck->lck_rw_want_write ||
		    lck->lck_rw_want_upgrade ||
		    lck->lck_rw_shared_count != 0) {
			return;
		}
		break;
	case LCK_RW_ASSERT_NOTHELD:
		if (!(lck->lck_rw_want_write ||
		    lck->lck_rw_want_upgrade ||
		    lck->lck_rw_shared_count != 0)) {
			return;
		}
		break;
	default:
		break;
	}

	panic("rw lock (%p)%s held (mode=%u), first word %08x\n", lck, (type == LCK_RW_ASSERT_NOTHELD ? "" : " not"), type, *(uint32_t *)lck);
}

                                                                                              
#if MACH_LDEBUG
__dead2
#endif
void
lck_rw_clear_promotions_x86(thread_t thread)
{
#if MACH_LDEBUG
	                                                                  
	panic("%u rw lock(s) held on return to userspace for thread %p", thread->rwlock_count, thread);
#else
	                          
	thread->rwlock_count = 0;
	lck_rw_clear_promotion(thread, 0);
#endif
}

boolean_t
lck_rw_lock_yield_shared(lck_rw_t *lck, boolean_t force_yield)
{
	lck_rw_assert(lck, LCK_RW_ASSERT_SHARED);

	if (lck->lck_rw_want_write || lck->lck_rw_want_upgrade || force_yield) {
		lck_rw_unlock_shared(lck);
		mutex_pause(2);
		lck_rw_lock_shared(lck);
		return TRUE;
	}

	return FALSE;
}

  
                                                 
                                                                  
   
boolean_t
kdp_lck_rw_lock_is_acquired_exclusive(lck_rw_t *lck)
{
	if (not_in_kdp) {
		panic("panic: rw lock exclusive check done outside of kernel debugger");
	}
	return ((lck->lck_rw_want_upgrade || lck->lck_rw_want_write) && (lck->lck_rw_shared_count == 0)) ? TRUE : FALSE;
}

  
                                                                  
  
                                                               
                                                                               
                                                     
  
                                                                                        
                                                                                             
                                                                                      
                                                                                                  
  
                                                                                              
                                                                                              
                                                                                             
                                                                                   
                                                                          
  
                                                                                   
                                                                                           
                                                                              
   

  
                         
  
                                                                                  
  
                                                                               
                                                                              
                                            
  
                                                                       
                                                                                      
                                                                                  
            
  
                                                                                         
                                                                                         
                    
   

  
                                          
   
lck_mtx_t *
lck_mtx_alloc_init(
	lck_grp_t       *grp,
	lck_attr_t      *attr)
{
	lck_mtx_t *lck;

	lck = zalloc(ZV_LCK_MTX);
	lck_mtx_init(lck, grp, attr);
	return lck;
}

  
                                    
   
void
lck_mtx_free(
	lck_mtx_t       *lck,
	lck_grp_t       *grp)
{
	lck_mtx_destroy(lck, grp);
	zfree(ZV_LCK_MTX, lck);
}

  
                                        
   
static void
lck_mtx_ext_init(
	lck_mtx_ext_t   *lck,
	lck_grp_t       *grp,
	lck_attr_t      *attr)
{
	bzero((void *)lck, sizeof(lck_mtx_ext_t));

	if ((attr->lck_attr_val) & LCK_ATTR_DEBUG) {
		lck->lck_mtx_deb.type = MUTEX_TAG;
		lck->lck_mtx_attr |= LCK_MTX_ATTR_DEBUG;
	}

	lck->lck_mtx_grp = grp;

	if (grp->lck_grp_attr & LCK_GRP_ATTR_STAT) {
		lck->lck_mtx_attr |= LCK_MTX_ATTR_STAT;
	}

	lck->lck_mtx.lck_mtx_is_ext = 1;
	lck->lck_mtx.lck_mtx_pad32 = 0xFFFFFFFF;
}

  
                                    
   
void
lck_mtx_init(
	lck_mtx_t       *lck,
	lck_grp_t       *grp,
	lck_attr_t      *attr)
{
	lck_mtx_ext_t   *lck_ext;
	lck_attr_t      *lck_attr;

	if (attr != LCK_ATTR_NULL) {
		lck_attr = attr;
	} else {
		lck_attr = &LockDefaultLckAttr;
	}

	if ((lck_attr->lck_attr_val) & LCK_ATTR_DEBUG) {
		lck_ext = zalloc(ZV_LCK_MTX_EXT);
		lck_mtx_ext_init(lck_ext, grp, lck_attr);
		lck->lck_mtx_tag = LCK_MTX_TAG_INDIRECT;
		lck->lck_mtx_ptr = lck_ext;
	} else {
		lck->lck_mtx_owner = 0;
		lck->lck_mtx_state = 0;
	}
	lck->lck_mtx_pad32 = 0xFFFFFFFF;
	lck_grp_reference(grp);
	lck_grp_lckcnt_incr(grp, LCK_TYPE_MTX);
}

  
                                        
   
void
lck_mtx_init_ext(
	lck_mtx_t       *lck,
	lck_mtx_ext_t   *lck_ext,
	lck_grp_t       *grp,
	lck_attr_t      *attr)
{
	lck_attr_t      *lck_attr;

	if (attr != LCK_ATTR_NULL) {
		lck_attr = attr;
	} else {
		lck_attr = &LockDefaultLckAttr;
	}

	if ((lck_attr->lck_attr_val) & LCK_ATTR_DEBUG) {
		lck_mtx_ext_init(lck_ext, grp, lck_attr);
		lck->lck_mtx_tag = LCK_MTX_TAG_INDIRECT;
		lck->lck_mtx_ptr = lck_ext;
	} else {
		lck->lck_mtx_owner = 0;
		lck->lck_mtx_state = 0;
	}
	lck->lck_mtx_pad32 = 0xFFFFFFFF;

	lck_grp_reference(grp);
	lck_grp_lckcnt_incr(grp, LCK_TYPE_MTX);
}

static void
lck_mtx_lock_mark_destroyed(
	lck_mtx_t *mutex,
	boolean_t indirect)
{
	uint32_t state;

	if (indirect) {
		                                
		ordered_store_mtx_state_release(mutex, LCK_MTX_TAG_DESTROYED);
		return;
	}

	state = ordered_load_mtx_state(mutex);
	lck_mtx_interlock_lock(mutex, &state);

	ordered_store_mtx_state_release(mutex, LCK_MTX_TAG_DESTROYED);

	enable_preemption();
}

  
                                       
   
void
lck_mtx_destroy(
	lck_mtx_t       *lck,
	lck_grp_t       *grp)
{
	boolean_t indirect;

	if (lck->lck_mtx_tag == LCK_MTX_TAG_DESTROYED) {
		return;
	}
#if MACH_LDEBUG
	lck_mtx_assert(lck, LCK_MTX_ASSERT_NOTOWNED);
#endif
	indirect = (lck->lck_mtx_tag == LCK_MTX_TAG_INDIRECT);

	lck_mtx_lock_mark_destroyed(lck, indirect);

	if (indirect) {
		zfree(ZV_LCK_MTX_EXT, lck->lck_mtx_ptr);
	}
	lck_grp_lckcnt_decr(grp, LCK_TYPE_MTX);
	lck_grp_deallocate(grp);
	return;
}


#if DEVELOPMENT | DEBUG
__attribute__((noinline))
void
lck_mtx_owner_check_panic(
	lck_mtx_t       *lock)
{
	thread_t owner = (thread_t)lock->lck_mtx_owner;
	panic("Mutex unlock attempted from non-owner thread. Owner=%p lock=%p", owner, lock);
}
#endif

__attribute__((always_inline))
static boolean_t
get_indirect_mutex(
	lck_mtx_t       **lock,
	uint32_t        *state)
{
	*lock = &((*lock)->lck_mtx_ptr->lck_mtx);
	*state = ordered_load_mtx_state(*lock);
	return TRUE;
}

  
                                   
  
                                          
  
                                        
  
                         
   
__attribute__((noinline))
void
lck_mtx_unlock_slow(
	lck_mtx_t       *lock)
{
	thread_t        thread;
	uint32_t        state, prev;
	boolean_t       indirect = FALSE;

	state = ordered_load_mtx_state(lock);

	                                
	if (__improbable(state == LCK_MTX_TAG_INDIRECT)) {
		indirect = get_indirect_mutex(&lock, &state);
	}

	thread = current_thread();

#if DEVELOPMENT | DEBUG
	thread_t owner = (thread_t)lock->lck_mtx_owner;
	if (__improbable(owner != thread)) {
		lck_mtx_owner_check_panic(lock);
	}
#endif

	                                       
	if (__improbable((state & LCK_MTX_MLOCKED_MSK) == 0)) {
		goto unlock;
	}

	lck_mtx_interlock_lock_clear_flags(lock, LCK_MTX_MLOCKED_MSK, &state);

unlock:
	                                                            

	                 
	ordered_store_mtx_owner(lock, 0);
	                                                      
	prev = state;

	if (__improbable(state & LCK_MTX_WAITERS_MSK)) {
#if     MACH_LDEBUG
		if (thread) {
			thread->mutex_count--;
		}
#endif
		return lck_mtx_unlock_wakeup_tail(lock, state, indirect);
	}

	                                                      
	state &= (~(LCK_MTX_ILOCKED_MSK | LCK_MTX_SPIN_MSK));
	ordered_store_mtx_state_release(lock, state);                                                                         

#if     MACH_LDEBUG
	                                                         
	if (thread) {
		thread->mutex_count--;                              
	}
#endif                   

	                          
	lck_mtx_unlock_finish_inline(lock, FALSE);

	return;
}

#define LCK_MTX_LCK_WAIT_CODE           0x20
#define LCK_MTX_LCK_WAKEUP_CODE         0x21
#define LCK_MTX_LCK_SPIN_CODE           0x22
#define LCK_MTX_LCK_ACQUIRE_CODE        0x23
#define LCK_MTX_LCK_DEMOTE_CODE         0x24

  
                                         
  
                                  
                                             
                                   
  
                                         
  
                                                                  
                                                               
                                 
  
                                                          
                                               
                                              
  
                                                  
                                 
   
__attribute__((noinline))
static void
lck_mtx_unlock_wakeup_tail(
	lck_mtx_t       *mutex,
	uint32_t        state,
	boolean_t       indirect)
{
	struct turnstile *ts;

	__kdebug_only uintptr_t trace_lck = unslide_for_kdebug(mutex);
	kern_return_t did_wake;

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_MTX_LCK_WAKEUP_CODE) | DBG_FUNC_START,
	    trace_lck, 0, mutex->lck_mtx_waiters, 0, 0);

	ts = turnstile_prepare((uintptr_t)mutex, NULL, TURNSTILE_NULL, TURNSTILE_KERNEL_MUTEX);

	if (mutex->lck_mtx_waiters > 1) {
		                                                                                      
		did_wake = waitq_wakeup64_one(&ts->ts_waitq, CAST_EVENT64_T(LCK_MTX_EVENT(mutex)), THREAD_AWAKENED, WAITQ_PROMOTE_ON_WAKE);
	} else {
		did_wake = waitq_wakeup64_one(&ts->ts_waitq, CAST_EVENT64_T(LCK_MTX_EVENT(mutex)), THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);
		turnstile_update_inheritor(ts, NULL, TURNSTILE_IMMEDIATE_UPDATE);
	}
	assert(did_wake == KERN_SUCCESS);

	turnstile_update_inheritor_complete(ts, TURNSTILE_INTERLOCK_HELD);
	turnstile_complete((uintptr_t)mutex, NULL, NULL, TURNSTILE_KERNEL_MUTEX);

	state -= LCK_MTX_WAITER;
	state &= (~(LCK_MTX_SPIN_MSK | LCK_MTX_ILOCKED_MSK));
	ordered_store_mtx_state_release(mutex, state);

	assert(current_thread()->turnstile != NULL);

	turnstile_cleanup();

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_MTX_LCK_WAKEUP_CODE) | DBG_FUNC_END,
	    trace_lck, 0, mutex->lck_mtx_waiters, 0, 0);

	lck_mtx_unlock_finish_inline(mutex, indirect);
}

  
                                        
  
                                               
                                                  
                                   
  
                                                                 
   
__attribute__((always_inline))
static void
lck_mtx_lock_acquire_inline(
	lck_mtx_t       *mutex,
	struct turnstile *ts)
{
	__kdebug_only uintptr_t trace_lck = unslide_for_kdebug(mutex);

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_MTX_LCK_ACQUIRE_CODE) | DBG_FUNC_START,
	    trace_lck, 0, mutex->lck_mtx_waiters, 0, 0);

	thread_t thread = (thread_t)mutex->lck_mtx_owner;                                         
	assert(thread->waiting_for_mutex == NULL);

	if (mutex->lck_mtx_waiters > 0) {
		if (ts == NULL) {
			ts = turnstile_prepare((uintptr_t)mutex, NULL, TURNSTILE_NULL, TURNSTILE_KERNEL_MUTEX);
		}

		turnstile_update_inheritor(ts, thread, (TURNSTILE_IMMEDIATE_UPDATE | TURNSTILE_INHERITOR_THREAD));
		turnstile_update_inheritor_complete(ts, TURNSTILE_INTERLOCK_HELD);
	}

	if (ts != NULL) {
		turnstile_complete((uintptr_t)mutex, NULL, NULL, TURNSTILE_KERNEL_MUTEX);
	}

	assert(current_thread()->turnstile != NULL);

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_MTX_LCK_ACQUIRE_CODE) | DBG_FUNC_END,
	    trace_lck, 0, mutex->lck_mtx_waiters, 0, 0);
}

void
lck_mtx_lock_acquire_x86(
	lck_mtx_t       *mutex)
{
	return lck_mtx_lock_acquire_inline(mutex, NULL);
}

  
                                                    
                                                                            
                              
   

__attribute__((noinline))
static void
lck_mtx_lock_acquire_tail(
	lck_mtx_t       *mutex,
	boolean_t       indirect,
	struct turnstile *ts)
{
	lck_mtx_lock_acquire_inline(mutex, ts);
	lck_mtx_lock_finish_inline_with_cleanup(mutex, ordered_load_mtx_state(mutex), indirect);
}

__attribute__((noinline))
static boolean_t
lck_mtx_try_lock_acquire_tail(
	lck_mtx_t       *mutex)
{
	lck_mtx_lock_acquire_inline(mutex, NULL);
	lck_mtx_try_lock_finish_inline(mutex, ordered_load_mtx_state(mutex));

	return TRUE;
}

__attribute__((noinline))
static void
lck_mtx_convert_spin_acquire_tail(
	lck_mtx_t       *mutex)
{
	lck_mtx_lock_acquire_inline(mutex, NULL);
	lck_mtx_convert_spin_finish_inline(mutex, ordered_load_mtx_state(mutex));
}

boolean_t
lck_mtx_ilk_unlock(
	lck_mtx_t       *mutex)
{
	lck_mtx_ilk_unlock_inline(mutex, ordered_load_mtx_state(mutex));
	return TRUE;
}

static inline void
lck_mtx_interlock_lock_set_and_clear_flags(
	lck_mtx_t *mutex,
	uint32_t xor_flags,
	uint32_t and_flags,
	uint32_t *new_state)
{
	uint32_t state, prev;
	state = *new_state;

	for (;;) {
		                                         
		while (__improbable(state & (LCK_MTX_ILOCKED_MSK | xor_flags))) {
			cpu_pause();
			state = ordered_load_mtx_state(mutex);
		}
		prev = state;                                                                            
		state |= LCK_MTX_ILOCKED_MSK | xor_flags;                              
		state &= ~and_flags;                                             

		disable_preemption();
		if (os_atomic_cmpxchg(&mutex->lck_mtx_state, prev, state, acquire)) {
			break;
		}
		enable_preemption();
		cpu_pause();
		state = ordered_load_mtx_state(mutex);
	}
	*new_state = state;
	return;
}

static inline void
lck_mtx_interlock_lock_clear_flags(
	lck_mtx_t *mutex,
	uint32_t and_flags,
	uint32_t *new_state)
{
	return lck_mtx_interlock_lock_set_and_clear_flags(mutex, 0, and_flags, new_state);
}

static inline void
lck_mtx_interlock_lock(
	lck_mtx_t *mutex,
	uint32_t *new_state)
{
	return lck_mtx_interlock_lock_set_and_clear_flags(mutex, 0, 0, new_state);
}

static inline int
lck_mtx_interlock_try_lock_set_flags(
	lck_mtx_t *mutex,
	uint32_t or_flags,
	uint32_t *new_state)
{
	uint32_t state, prev;
	state = *new_state;

	                                         
	if (state & (LCK_MTX_ILOCKED_MSK | or_flags)) {
		return 0;
	}
	prev = state;                                                                            
	state |= LCK_MTX_ILOCKED_MSK | or_flags;                               
	disable_preemption();
	if (os_atomic_cmpxchg(&mutex->lck_mtx_state, prev, state, acquire)) {
		*new_state = state;
		return 1;
	}

	enable_preemption();
	return 0;
}

__attribute__((noinline))
static void
lck_mtx_lock_contended(
	lck_mtx_t       *lock,
	boolean_t indirect,
	boolean_t *first_miss)
{
	lck_mtx_spinwait_ret_type_t ret;
	uint32_t state;
	thread_t thread;
	struct turnstile *ts = NULL;

try_again:

	if (indirect) {
		lck_grp_mtx_update_miss((struct _lck_mtx_ext_*)lock, first_miss);
	}

	ret = lck_mtx_lock_spinwait_x86(lock);
	state = ordered_load_mtx_state(lock);
	switch (ret) {
	case LCK_MTX_SPINWAIT_NO_SPIN:
		  
                                                             
                 
     
		if (indirect) {
			lck_grp_mtx_update_direct_wait((struct _lck_mtx_ext_*)lock);
		}

		                                                  
		OS_FALLTHROUGH;
	case LCK_MTX_SPINWAIT_SPUN_HIGH_THR:
	case LCK_MTX_SPINWAIT_SPUN_OWNER_NOT_CORE:
	case LCK_MTX_SPINWAIT_SPUN_NO_WINDOW_CONTENTION:
	case LCK_MTX_SPINWAIT_SPUN_SLIDING_THR:
		  
                                                                   
                       
     
		lck_mtx_interlock_lock(lock, &state);
		assert(state & LCK_MTX_ILOCKED_MSK);

		if (state & LCK_MTX_MLOCKED_MSK) {
			if (indirect) {
				lck_grp_mtx_update_wait((struct _lck_mtx_ext_*)lock, first_miss);
			}
			lck_mtx_lock_wait_x86(lock, &ts);
			  
                                 
      
			goto try_again;
		} else {
			                    
			state |= LCK_MTX_MLOCKED_MSK;
			ordered_store_mtx_state_release(lock, state);
			thread = current_thread();
			ordered_store_mtx_owner(lock, (uintptr_t)thread);
#if     MACH_LDEBUG
			if (thread) {
				thread->mutex_count++;
			}
#endif                   
		}

		break;
	case LCK_MTX_SPINWAIT_ACQUIRED:
		  
                                                         
                                              
                                            
                           
     
		break;
	default:
		panic("lck_mtx_lock_spinwait_x86 returned %d for mutex %p\n", ret, lock);
	}

	  
                                      
    

	                             
	thread = (thread_t)lock->lck_mtx_owner;
	if (state & LCK_MTX_WAITERS_MSK) {
		  
                                        
                        
     
		return lck_mtx_lock_acquire_tail(lock, indirect, ts);
	}

	if (ts != NULL) {
		turnstile_complete((uintptr_t)lock, NULL, NULL, TURNSTILE_KERNEL_MUTEX);
	}

	assert(current_thread()->turnstile != NULL);

	                           
	lck_mtx_lock_finish_inline_with_cleanup(lock, ordered_load_mtx_state(lock), indirect);
}

  
                                        
                                   
   

__attribute__((noinline)) __abortlike
static void
lck_mtx_destroyed(
	lck_mtx_t       *lock)
{
	panic("trying to interlock destroyed mutex (%p)", lock);
}

__attribute__((noinline))
static boolean_t
lck_mtx_try_destroyed(
	lck_mtx_t       *lock)
{
	panic("trying to interlock destroyed mutex (%p)", lock);
	return FALSE;
}

__attribute__((always_inline))
static boolean_t
lck_mtx_lock_wait_interlock_to_clear(
	lck_mtx_t       *lock,
	uint32_t*        new_state)
{
	uint32_t state;

	for (;;) {
		cpu_pause();
		state = ordered_load_mtx_state(lock);
		if (!(state & (LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK))) {
			*new_state = state;
			return TRUE;
		}
		if (state & LCK_MTX_MLOCKED_MSK) {
			                                       
			return FALSE;
		}
	}
}

__attribute__((always_inline))
static boolean_t
lck_mtx_try_lock_wait_interlock_to_clear(
	lck_mtx_t       *lock,
	uint32_t*        new_state)
{
	uint32_t state;

	for (;;) {
		cpu_pause();
		state = ordered_load_mtx_state(lock);
		if (state & (LCK_MTX_MLOCKED_MSK | LCK_MTX_SPIN_MSK)) {
			                                               
			return FALSE;
		}
		if (!(state & LCK_MTX_ILOCKED_MSK)) {
			*new_state = state;
			return TRUE;
		}
	}
}

  
                             
  
                                    
                                               
         
  
                                  
   
__attribute__((noinline))
void
lck_mtx_lock_slow(
	lck_mtx_t       *lock)
{
	boolean_t       indirect = FALSE;
	uint32_t        state;
	int             first_miss = 0;

	state = ordered_load_mtx_state(lock);

	                                    
	if (__improbable(state & ((LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK)))) {
		  
                                                              
                                                     
                                        
     


		                                                
		if (__improbable(!(state & LCK_MTX_ILOCKED_MSK))) {
			                                  
			return lck_mtx_lock_contended(lock, indirect, &first_miss);
		}

		                                            
		if (__improbable(state == LCK_MTX_TAG_DESTROYED)) {
			lck_mtx_destroyed(lock);
		}

		                                
		if (__improbable(state == LCK_MTX_TAG_INDIRECT)) {
			indirect = get_indirect_mutex(&lock, &state);

			first_miss = 0;
			lck_grp_mtx_update_held((struct _lck_mtx_ext_*)lock);

			if (state & LCK_MTX_SPIN_MSK) {
				                                                               
				assert(state & LCK_MTX_ILOCKED_MSK);
				lck_grp_mtx_update_miss((struct _lck_mtx_ext_*)lock, &first_miss);
			}
		}

		if (!lck_mtx_lock_wait_interlock_to_clear(lock, &state)) {
			return lck_mtx_lock_contended(lock, indirect, &first_miss);
		}
	}

	                                                 
	while (__improbable(!lck_mtx_interlock_try_lock_set_flags(lock, LCK_MTX_MLOCKED_MSK, &state))) {
		if (!lck_mtx_lock_wait_interlock_to_clear(lock, &state)) {
			return lck_mtx_lock_contended(lock, indirect, &first_miss);
		}
	}

	                                 

	thread_t thread = current_thread();
	                           
	ordered_store_mtx_owner(lock, (uintptr_t)thread);

#if MACH_LDEBUG
	if (thread) {
		thread->mutex_count++;                              
	}
#endif
	  
                                 
                           
    
	if (__improbable(state & LCK_MTX_WAITERS_MSK)) {
		return lck_mtx_lock_acquire_tail(lock, indirect, NULL);
	}

	                           
	lck_mtx_lock_finish_inline(lock, ordered_load_mtx_state(lock), indirect);

	return;
}

__attribute__((noinline))
boolean_t
lck_mtx_try_lock_slow(
	lck_mtx_t       *lock)
{
	boolean_t       indirect = FALSE;
	uint32_t        state;
	int             first_miss = 0;

	state = ordered_load_mtx_state(lock);

	                                    
	if (__improbable(state & ((LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK)))) {
		  
                                                              
                                                     
                                        
     

		                                                
		if (__improbable(!(state & LCK_MTX_ILOCKED_MSK))) {
			return FALSE;
		}

		                                            
		if (__improbable(state == LCK_MTX_TAG_DESTROYED)) {
			lck_mtx_try_destroyed(lock);
		}

		                                
		if (__improbable(state == LCK_MTX_TAG_INDIRECT)) {
			indirect = get_indirect_mutex(&lock, &state);

			first_miss = 0;
			lck_grp_mtx_update_held((struct _lck_mtx_ext_*)lock);
		}

		if (!lck_mtx_try_lock_wait_interlock_to_clear(lock, &state)) {
			if (indirect) {
				lck_grp_mtx_update_miss((struct _lck_mtx_ext_*)lock, &first_miss);
			}
			return FALSE;
		}
	}

	                                                 
	while (__improbable(!lck_mtx_interlock_try_lock_set_flags(lock, LCK_MTX_MLOCKED_MSK, &state))) {
		if (!lck_mtx_try_lock_wait_interlock_to_clear(lock, &state)) {
			if (indirect) {
				lck_grp_mtx_update_miss((struct _lck_mtx_ext_*)lock, &first_miss);
			}
			return FALSE;
		}
	}

	                                 

	thread_t thread = current_thread();
	                           
	ordered_store_mtx_owner(lock, (uintptr_t)thread);

#if MACH_LDEBUG
	if (thread) {
		thread->mutex_count++;                              
	}
#endif
	  
                                 
                           
    
	if (__improbable(state & LCK_MTX_WAITERS_MSK)) {
		return lck_mtx_try_lock_acquire_tail(lock);
	}

	                           
	lck_mtx_try_lock_finish_inline(lock, ordered_load_mtx_state(lock));

	return TRUE;
}

__attribute__((noinline))
void
lck_mtx_lock_spin_slow(
	lck_mtx_t       *lock)
{
	boolean_t       indirect = FALSE;
	uint32_t        state;
	int             first_miss = 0;

	state = ordered_load_mtx_state(lock);

	                                    
	if (__improbable(state & ((LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK)))) {
		  
                                                              
                                                     
                                        
     


		                                                
		if (__improbable(!(state & LCK_MTX_ILOCKED_MSK))) {
			                                  
			return lck_mtx_lock_contended(lock, indirect, &first_miss);
		}

		                                            
		if (__improbable(state == LCK_MTX_TAG_DESTROYED)) {
			lck_mtx_destroyed(lock);
		}

		                                
		if (__improbable(state == LCK_MTX_TAG_INDIRECT)) {
			indirect = get_indirect_mutex(&lock, &state);

			first_miss = 0;
			lck_grp_mtx_update_held((struct _lck_mtx_ext_*)lock);

			if (state & LCK_MTX_SPIN_MSK) {
				                                                               
				assert(state & LCK_MTX_ILOCKED_MSK);
				lck_grp_mtx_update_miss((struct _lck_mtx_ext_*)lock, &first_miss);
			}
		}

		if (!lck_mtx_lock_wait_interlock_to_clear(lock, &state)) {
			return lck_mtx_lock_contended(lock, indirect, &first_miss);
		}
	}

	                                                 
	while (__improbable(!lck_mtx_interlock_try_lock_set_flags(lock, LCK_MTX_SPIN_MSK, &state))) {
		if (!lck_mtx_lock_wait_interlock_to_clear(lock, &state)) {
			return lck_mtx_lock_contended(lock, indirect, &first_miss);
		}
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

__attribute__((noinline))
boolean_t
lck_mtx_try_lock_spin_slow(
	lck_mtx_t       *lock)
{
	boolean_t       indirect = FALSE;
	uint32_t        state;
	int             first_miss = 0;

	state = ordered_load_mtx_state(lock);

	                                    
	if (__improbable(state & ((LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK)))) {
		  
                                                              
                                                     
                                        
     

		                                                
		if (__improbable(!(state & LCK_MTX_ILOCKED_MSK))) {
			return FALSE;
		}

		                                            
		if (__improbable(state == LCK_MTX_TAG_DESTROYED)) {
			lck_mtx_try_destroyed(lock);
		}

		                                
		if (__improbable(state == LCK_MTX_TAG_INDIRECT)) {
			indirect = get_indirect_mutex(&lock, &state);

			first_miss = 0;
			lck_grp_mtx_update_held((struct _lck_mtx_ext_*)lock);
		}

		if (!lck_mtx_try_lock_wait_interlock_to_clear(lock, &state)) {
			if (indirect) {
				lck_grp_mtx_update_miss((struct _lck_mtx_ext_*)lock, &first_miss);
			}
			return FALSE;
		}
	}

	                                                 
	while (__improbable(!lck_mtx_interlock_try_lock_set_flags(lock, LCK_MTX_SPIN_MSK, &state))) {
		if (!lck_mtx_try_lock_wait_interlock_to_clear(lock, &state)) {
			if (indirect) {
				lck_grp_mtx_update_miss((struct _lck_mtx_ext_*)lock, &first_miss);
			}
			return FALSE;
		}
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

__attribute__((noinline))
void
lck_mtx_convert_spin(
	lck_mtx_t       *lock)
{
	uint32_t state;

	state = ordered_load_mtx_state(lock);

	                                
	if (__improbable(state == LCK_MTX_TAG_INDIRECT)) {
		                             
		get_indirect_mutex(&lock, &state);
	}

	assertf((thread_t)lock->lck_mtx_owner == current_thread(), "lock %p not owned by thread %p (current owner %p)", lock, current_thread(), (thread_t)lock->lck_mtx_owner );

	if (__improbable(state & LCK_MTX_MLOCKED_MSK)) {
		                                           
		return;
	}

	assert(get_preemption_level() > 0);
	assert(state & LCK_MTX_ILOCKED_MSK);
	assert(state & LCK_MTX_SPIN_MSK);

	  
                                 
                           
    
	if (__improbable(state & LCK_MTX_WAITERS_MSK)) {
		return lck_mtx_convert_spin_acquire_tail(lock);
	}

	lck_mtx_convert_spin_finish_inline(lock, ordered_load_mtx_state(lock));

	return;
}

static inline boolean_t
lck_mtx_lock_grab_mutex(
	lck_mtx_t       *lock)
{
	uint32_t state;

	state = ordered_load_mtx_state(lock);

	if (!lck_mtx_interlock_try_lock_set_flags(lock, LCK_MTX_MLOCKED_MSK, &state)) {
		return FALSE;
	}

	                                 

	thread_t thread = current_thread();
	                           
	ordered_store_mtx_owner(lock, (uintptr_t)thread);

#if MACH_LDEBUG
	if (thread) {
		thread->mutex_count++;                              
	}
#endif
	return TRUE;
}

__attribute__((noinline))
void
lck_mtx_assert(
	lck_mtx_t       *lock,
	unsigned int    type)
{
	thread_t thread, owner;
	uint32_t state;

	thread = current_thread();
	state = ordered_load_mtx_state(lock);

	if (state == LCK_MTX_TAG_INDIRECT) {
		get_indirect_mutex(&lock, &state);
	}

	owner = (thread_t)lock->lck_mtx_owner;

	if (type == LCK_MTX_ASSERT_OWNED) {
		if (owner != thread || !(state & (LCK_MTX_ILOCKED_MSK | LCK_MTX_MLOCKED_MSK))) {
			panic("mutex (%p) not owned\n", lock);
		}
	} else {
		assert(type == LCK_MTX_ASSERT_NOTOWNED);
		if (owner == thread) {
			panic("mutex (%p) owned\n", lock);
		}
	}
}

  
                                         
  
                                                                 
                                                                          
                                            
  
                                      
                                                      
                                           
                                                                                   
   
__attribute__((noinline))
lck_mtx_spinwait_ret_type_t
lck_mtx_lock_spinwait_x86(
	lck_mtx_t       *mutex)
{
	__kdebug_only uintptr_t trace_lck = unslide_for_kdebug(mutex);
	thread_t        owner, prev_owner;
	uint64_t        window_deadline, sliding_deadline, high_deadline;
	uint64_t        start_time, cur_time, avg_hold_time, bias, delta;
	lck_mtx_spinwait_ret_type_t             retval = LCK_MTX_SPINWAIT_SPUN_HIGH_THR;
	int             loopcount = 0;
	int             total_hold_time_samples, window_hold_time_samples, unfairness;
	uint            i, prev_owner_cpu;
	bool            owner_on_core, adjust;

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_MTX_LCK_SPIN_CODE) | DBG_FUNC_START,
	    trace_lck, VM_KERNEL_UNSLIDE_OR_PERM(mutex->lck_mtx_owner), mutex->lck_mtx_waiters, 0, 0);

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
	bias = (os_hash_kernel_pointer(mutex) + cpu_number()) % real_ncpus;

	prev_owner = (thread_t) mutex->lck_mtx_owner;
	  
               
                            
                                       
                                                  
                                        
    
	do {
		  
                             
     
		if (__probable(lck_mtx_lock_grab_mutex(mutex))) {
			retval = LCK_MTX_SPINWAIT_ACQUIRED;
			break;
		}

		cur_time = mach_absolute_time();

		  
                                   
     
		if (cur_time >= high_deadline) {
			retval = LCK_MTX_SPINWAIT_SPUN_HIGH_THR;
			break;
		}

		  
                                             
     
		owner = (thread_t) mutex->lck_mtx_owner;
		if (owner) {
			i = prev_owner_cpu;
			owner_on_core = FALSE;

			disable_preemption();
			owner = (thread_t) mutex->lck_mtx_owner;

			  
                                                              
                                          
                                                                           
                                                                     
                                                          
                                                                              
      
			if (owner) {
				do {
					if ((cpu_data_ptr[i] != NULL) && (cpu_data_ptr[i]->cpu_active_thread == owner)) {
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
					owner = (thread_t) mutex->lck_mtx_owner;
					if (owner == prev_owner) {
						  
                              
                       
         
						if (loopcount == 0) {
							retval = LCK_MTX_SPINWAIT_NO_SPIN;
						} else {
							retval = LCK_MTX_SPINWAIT_SPUN_OWNER_NOT_CORE;
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
				retval = LCK_MTX_SPINWAIT_SPUN_NO_WINDOW_CONTENTION;
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
			retval = LCK_MTX_SPINWAIT_SPUN_SLIDING_THR;
			break;
		}

		if ((thread_t) mutex->lck_mtx_owner != NULL) {
			cpu_pause();
		}

		loopcount++;
	} while (TRUE);

#if     CONFIG_DTRACE
	  
                                                                 
                                                          
                                                           
                                      
    
	if (__probable(mutex->lck_mtx_is_ext == 0)) {
		LOCKSTAT_RECORD(LS_LCK_MTX_LOCK_SPIN, mutex,
		    mach_absolute_time() - start_time);
	} else {
		LOCKSTAT_RECORD(LS_LCK_MTX_EXT_LOCK_SPIN, mutex,
		    mach_absolute_time() - start_time);
	}
	                                                                             
#endif

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_MTX_LCK_SPIN_CODE) | DBG_FUNC_END,
	    trace_lck, VM_KERNEL_UNSLIDE_OR_PERM(mutex->lck_mtx_owner), mutex->lck_mtx_waiters, retval, 0);

	return retval;
}



  
                                     
  
                                          
  
                                       
                         
                                                  
  
                                                         
                                                            
                                          
                                                            
  
            
                                                                                     
                                                                              
                                                                                          
                                                                          
                                                              
  
                                                                              
                                                                                   
                                                                             
                                             
   
__attribute__((noinline))
void
lck_mtx_lock_wait_x86(
	lck_mtx_t       *mutex,
	struct turnstile **ts)
{
	thread_t self = current_thread();

#if     CONFIG_DTRACE
	uint64_t sleep_start = 0;

	if (lockstat_probemap[LS_LCK_MTX_LOCK_BLOCK] || lockstat_probemap[LS_LCK_MTX_EXT_LOCK_BLOCK]) {
		sleep_start = mach_absolute_time();
	}
#endif
	__kdebug_only uintptr_t trace_lck = unslide_for_kdebug(mutex);

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_MTX_LCK_WAIT_CODE) | DBG_FUNC_START,
	    trace_lck, VM_KERNEL_UNSLIDE_OR_PERM(mutex->lck_mtx_owner),
	    mutex->lck_mtx_waiters, 0, 0);

	assert(self->waiting_for_mutex == NULL);
	self->waiting_for_mutex = mutex;
	mutex->lck_mtx_waiters++;

	thread_t holder = (thread_t)mutex->lck_mtx_owner;
	assert(holder != NULL);

	  
                                                                                     
                                                                                     
                                                              
    
	if (*ts == NULL) {
		*ts = turnstile_prepare((uintptr_t)mutex, NULL, TURNSTILE_NULL, TURNSTILE_KERNEL_MUTEX);
	}

	struct turnstile *turnstile = *ts;
	thread_set_pending_block_hint(self, kThreadWaitKernelMutex);
	turnstile_update_inheritor(turnstile, holder, (TURNSTILE_DELAYED_UPDATE | TURNSTILE_INHERITOR_THREAD));

	waitq_assert_wait64(&turnstile->ts_waitq, CAST_EVENT64_T(LCK_MTX_EVENT(mutex)), THREAD_UNINT | THREAD_WAIT_NOREPORT_USER, TIMEOUT_WAIT_FOREVER);

	lck_mtx_ilk_unlock(mutex);

	turnstile_update_inheritor_complete(turnstile, TURNSTILE_INTERLOCK_NOT_HELD);

	thread_block(THREAD_CONTINUE_NULL);

	self->waiting_for_mutex = NULL;

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_LOCKS, LCK_MTX_LCK_WAIT_CODE) | DBG_FUNC_END,
	    trace_lck, VM_KERNEL_UNSLIDE_OR_PERM(mutex->lck_mtx_owner),
	    mutex->lck_mtx_waiters, 0, 0);

#if     CONFIG_DTRACE
	  
                                                             
                                       
    
	if (sleep_start) {
		if (mutex->lck_mtx_is_ext == 0) {
			LOCKSTAT_RECORD(LS_LCK_MTX_LOCK_BLOCK, mutex,
			    mach_absolute_time() - sleep_start);
		} else {
			LOCKSTAT_RECORD(LS_LCK_MTX_EXT_LOCK_BLOCK, mutex,
			    mach_absolute_time() - sleep_start);
		}
	}
#endif
}

  
                                                  
                                                                       
                                          
   
boolean_t
kdp_lck_mtx_lock_spin_is_acquired(lck_mtx_t     *lck)
{
	if (not_in_kdp) {
		panic("panic: kdp_lck_mtx_lock_spin_is_acquired called outside of kernel debugger");
	}

	if (lck->lck_mtx_ilocked || lck->lck_mtx_mlocked) {
		return TRUE;
	}

	return FALSE;
}

void
kdp_lck_mtx_find_owner(__unused struct waitq * waitq, event64_t event, thread_waitinfo_t * waitinfo)
{
	lck_mtx_t * mutex = LCK_EVENT_TO_MUTEX(event);
	waitinfo->context = VM_KERNEL_UNSLIDE_OR_PERM(mutex);
	thread_t holder   = (thread_t)mutex->lck_mtx_owner;
	waitinfo->owner   = thread_tid(holder);
}

void
kdp_rwlck_find_owner(__unused struct waitq * waitq, event64_t event, thread_waitinfo_t * waitinfo)
{
	lck_rw_t *rwlck = NULL;
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
	waitinfo->owner = 0;
}
