/*
 * Copyright (c) 2018 Apple Inc. All rights reserved.
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
#define ATOMIC_PRIVATE 1
#define LOCK_PRIVATE 1

#include <stdint.h>
#include <kern/thread.h>
#include <machine/atomic.h>
#include <kern/locks.h>
#include <kern/lock_stat.h>
#include <machine/machine_cpu.h>

#if defined(__x86_64__)
#include <i386/mp.h>
extern uint64_t LockTimeOutTSC;
#define TICKET_LOCK_PANIC_TIMEOUT LockTimeOutTSC
#endif

#if defined(__arm__) || defined(__arm64__)
extern uint64_t TLockTimeOut;
#define TICKET_LOCK_PANIC_TIMEOUT TLockTimeOut
#endif
                                                  
                                                                               
                                    
   

                                                                       
                                                              
                              
                           
                                                                      
                                                                     
                  
                                                           
                                                               
                                                                         
                                       
   

void
lck_ticket_init(lck_ticket_t *tlock, lck_grp_t *grp)
{
	memset(tlock, 0, sizeof(*tlock));
	                                                               
                        
    
	static_assert(MAX_CPUS < 256);

	__assert_only   lck_ticket_internal *tlocki = &tlock->tu;
	                      
	__assert_only uintptr_t tcn = (uintptr_t) &tlocki->tcurnext;
	__assert_only uintptr_t tc = (uintptr_t) &tlocki->cticket;
	__assert_only uintptr_t tn = (uintptr_t) &tlocki->nticket;

	assert(((tcn & 3) == 0) && (tcn == tc) && (tn == (tc + 1)));

	if (grp) {
		lck_grp_reference(grp);
		lck_grp_lckcnt_incr(grp, LCK_TYPE_TICKET);
	}
}

static void
tlock_mark_owned(lck_ticket_t *tlock, thread_t cthread)
{
	assert(tlock->lck_owner == 0);
	                                                                       
                                                                  
                                                   
                                                                        
                                                            
                                                                         
                                                                                  
    
	__c11_atomic_store((_Atomic thread_t *)&tlock->lck_owner, cthread, __ATOMIC_RELAXED);
}

#if __arm__ || __arm64__
__unused static uint8_t
load_exclusive_acquire8(uint8_t *target)
{
	uint8_t value;
#if __arm__
	value = __builtin_arm_ldrex(target);
	__c11_atomic_thread_fence(__ATOMIC_ACQUIRE);
#else
	value = __builtin_arm_ldaex(target);            
	                                                             
	atomic_signal_fence(memory_order_acquire);
#endif
	return value;
}
#endif

                                    
                                                            
   
static void __attribute__((noinline))
tlock_contended(uint8_t *tp, uint8_t mt, lck_ticket_t *tlock, thread_t cthread LCK_GRP_ARG(lck_grp_t *grp))
{
	uint8_t cticket;
	uint64_t etime = 0, ctime = 0, stime = 0;

#if CONFIG_DTRACE || LOCK_STATS
	uint64_t begin = 0;
	boolean_t stat_enabled = lck_grp_ticket_spin_enabled(tlock LCK_GRP_ARG(grp));

	if (__improbable(stat_enabled)) {
		begin = mach_absolute_time();
	}
#endif                                  

	assertf(tlock->lck_owner != (uintptr_t) cthread, "Recursive ticket lock, owner: %p, current thread: %p", (void *) tlock->lck_owner, (void *) cthread);

	for (;;) {
		for (int i = 0; i < LOCK_SNOOP_SPINS; i++) {
#if (__ARM_ENABLE_WFE_)
			if ((cticket = load_exclusive_acquire8(tp)) != mt) {
				wait_for_event();
			} else {
				                                       
                                  
                                                   
                                         
       
				os_atomic_clear_exclusive();
				tlock_mark_owned(tlock, cthread);
#if CONFIG_DTRACE || LOCK_STATS
				lck_grp_ticket_update_miss(tlock LCK_GRP_ARG(grp));
				if (__improbable(stat_enabled)) {
					lck_grp_ticket_update_spin(tlock LCK_GRP_ARG(grp), mach_absolute_time() - begin);
				}
#endif                                  
				return;
			}
#else           
#if defined(__x86_64__)
			__builtin_ia32_pause();
#endif          
			if ((cticket = __c11_atomic_load((_Atomic uint8_t *) tp, __ATOMIC_SEQ_CST)) == mt) {
				tlock_mark_owned(tlock, cthread);
#if CONFIG_DTRACE || LOCK_STATS
				if (__improbable(stat_enabled)) {
					lck_grp_ticket_update_spin(tlock LCK_GRP_ARG(grp), mach_absolute_time() - begin);
				}
				lck_grp_ticket_update_miss(tlock LCK_GRP_ARG(grp));
#endif                                  
				return;
			}
#endif           
		}

		if (etime == 0) {
			stime = ml_get_timebase();
			etime = stime + TICKET_LOCK_PANIC_TIMEOUT;
		} else if ((ctime = ml_get_timebase()) >= etime) {
			break;
		}
	}
#if defined (__x86_64__)
	uintptr_t lowner = tlock->lck_owner;
	uint32_t ocpu = spinlock_timeout_NMI(lowner);
	panic("Ticket spinlock timeout; start: 0x%llx, end: 0x%llx, current: 0x%llx, lock: %p, *lock: 0x%x, waiting for 0x%x, pre-NMI owner: %p, current owner: %p, owner CPU: 0x%x", stime, etime, ctime, tp, *tp, mt, (void *) lowner, (void *) tlock->lck_owner, ocpu);
#else
	panic("Ticket spinlock timeout; start: 0x%llx, end: 0x%llx, current: 0x%llx, lock: %p, *lock: 0x%x, waiting for 0x%x, owner: %p", stime, etime, ctime, tp, *tp, mt, (void *) tlock->lck_owner);
#endif
}

void
(lck_ticket_lock)(lck_ticket_t * tlock LCK_GRP_ARG(lck_grp_t *grp))
{
	lck_ticket_internal *tlocki = &tlock->tu;
	thread_t cthread = current_thread();
	lck_ticket_internal tlocka;

	disable_preemption_for_thread(cthread);
	                                                                      
                                                               
                                                 
    
	tlocka.tcurnext = __c11_atomic_fetch_add((_Atomic uint16_t *)&tlocki->tcurnext, 1U << 8, __ATOMIC_ACQUIRE);

	                                                       
	if (__improbable(tlocka.cticket != tlocka.nticket)) {
		return tlock_contended(&tlocki->cticket, tlocka.nticket, tlock, cthread LCK_GRP_ARG(grp));
	}

	tlock_mark_owned(tlock, cthread);

	lck_grp_ticket_update_held(tlock LCK_GRP_ARG(grp));
}

void
lck_ticket_unlock(lck_ticket_t *tlock)
{
	lck_ticket_internal *tlocki = &tlock->tu;

	assertf(tlock->lck_owner == (uintptr_t) current_thread(), "Ticket unlock non-owned, owner: %p", (void *) tlock->lck_owner);

	__c11_atomic_store((_Atomic uintptr_t *)&tlock->lck_owner, 0, __ATOMIC_RELAXED);

#if defined(__x86_64__)
	                                                           
	__c11_atomic_thread_fence(__ATOMIC_RELEASE);
	                                                 
	                                                                           
                                                                 
    
	__asm__ volatile ("incb %0" : "+m"(tlocki->cticket) :: "cc");
#else              
	uint8_t cticket = __c11_atomic_load((_Atomic uint8_t *) &tlocki->cticket, __ATOMIC_RELAXED);
	cticket++;
	__c11_atomic_store((_Atomic uint8_t *) &tlocki->cticket, cticket, __ATOMIC_RELEASE);
#if __arm__
	set_event();
#endif            
#endif              
#if CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_TICKET_LOCK_RELEASE, tlock);
#endif                    
	enable_preemption();
}

void
lck_ticket_assert_owned(__assert_only lck_ticket_t *tlock)
{
	assertf(__c11_atomic_load((_Atomic thread_t *)&tlock->lck_owner, __ATOMIC_RELAXED) == current_thread(), "lck_ticket_assert_owned: owner %p, current: %p", (void *) tlock->lck_owner, current_thread());
}
