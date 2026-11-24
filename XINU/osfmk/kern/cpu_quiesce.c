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

#ifdef __x86_64__
#error This file is only needed on weakly-ordered systems!
#endif

#include <machine/atomic.h>
#include <machine/commpage.h>
#include <machine/machine_cpu.h>

#include <kern/sched_prim.h>
#include <kern/percpu.h>
#include <kern/ast.h>

#include <kern/cpu_quiesce.h>

  
                                                                   
  
                                                         
                                                                                             
                                                                                   
  
                                                                                 
                                                                                  
                        
  
                                                                                 
                                                                            
                                                                        
                                        
  
                                                         
                                                                               
  
                                                                  
                                                                           
                                                                                  
                    
  
                                                                       
                                                                     
  
                                                                              
                                                    
   

typedef unsigned long checkin_mask_t;

static _Atomic checkin_mask_t cpu_quiescing_checkin_state;

static uint64_t cpu_checkin_last_commit;

struct cpu_quiesce {
	cpu_quiescent_state_t   state;
	uint64_t                last_checkin;
};

static struct cpu_quiesce PERCPU_DATA(cpu_quiesce);

#define CPU_CHECKIN_MIN_INTERVAL_US     4000          
#define CPU_CHECKIN_MIN_INTERVAL_MAX_US USEC_PER_SEC         
static uint64_t cpu_checkin_min_interval;
static uint32_t cpu_checkin_min_interval_us;

#if __LP64__
#define CPU_CHECKIN_MASK_MAX_CPUS 32
#define CPU_CHECKIN_MASK          0x5555555555555555UL
#define CPU_EXPECTED_MASK         (~CPU_CHECKIN_MASK)
#else
                                                                                
#define CPU_CHECKIN_MASK_MAX_CPUS 16
#define CPU_CHECKIN_MASK          0x55555555UL
#define CPU_EXPECTED_MASK         (~CPU_CHECKIN_MASK)
#endif

static_assert(MAX_CPUS <= CPU_CHECKIN_MASK_MAX_CPUS);
static_assert(CPU_CHECKIN_MASK == CPU_EXPECTED_MASK >> 1);

static inline checkin_mask_t
cpu_checked_in_bit(int cpuid)
{
	return 1UL << (2 * cpuid);
}

static inline checkin_mask_t
cpu_expected_bit(int cpuid)
{
	return 1UL << (2 * cpuid + 1);
}

void
cpu_quiescent_counter_init(void)
{
	assert(CPU_CHECKIN_MASK & cpu_checked_in_bit(MAX_CPUS - 1));
	assert(CPU_EXPECTED_MASK & cpu_expected_bit(MAX_CPUS - 1));
	assert((CPU_CHECKIN_MASK & cpu_expected_bit(MAX_CPUS - 1)) == 0);
	assert((CPU_EXPECTED_MASK & cpu_checked_in_bit(MAX_CPUS - 1)) == 0);

	cpu_quiescent_counter_set_min_interval_us(CPU_CHECKIN_MIN_INTERVAL_US);
}

void
cpu_quiescent_counter_set_min_interval_us(uint32_t new_value_us)
{
	                                     
	if (new_value_us > CPU_CHECKIN_MIN_INTERVAL_MAX_US) {
		new_value_us = CPU_CHECKIN_MIN_INTERVAL_MAX_US;
	}

	cpu_checkin_min_interval_us = new_value_us;

	uint64_t abstime = 0;
	clock_interval_to_absolutetime_interval(cpu_checkin_min_interval_us,
	    NSEC_PER_USEC, &abstime);
	cpu_checkin_min_interval = abstime;
}

uint32_t
cpu_quiescent_counter_get_min_interval_us(void)
{
	return cpu_checkin_min_interval_us;
}


  
                                                
  
                                                                               
                                                          
   
static void
cpu_quiescent_counter_commit(uint64_t ctime)
{
	__kdebug_only uint64_t          old_gen;
	__kdebug_only checkin_mask_t    old_state;

	old_gen = commpage_increment_cpu_quiescent_counter();

	cpu_checkin_last_commit = ctime;

	old_state = os_atomic_andnot(&cpu_quiescing_checkin_state, CPU_CHECKIN_MASK, release);

	KDBG(MACHDBG_CODE(DBG_MACH_SCHED, MACH_QUIESCENT_COUNTER), old_gen, old_state, ctime, 0);
}

  
                                         
   
static bool
cpu_quiescent_counter_needs_commit(checkin_mask_t state)
{
	return (state & CPU_CHECKIN_MASK) == ((state & CPU_EXPECTED_MASK) >> 1);
}

  
                                                                            
                                                      
                                       
                                     
  
                                   
   
void
cpu_quiescent_counter_join(__unused uint64_t ctime)
{
	struct cpu_quiesce *st = PERCPU_GET(cpu_quiesce);
	__assert_only int cpuid = cpu_number();

	assert(cpuid < MAX_CPUS);
	assert(st->state == CPU_QUIESCE_COUNTER_NONE ||
	    st->state == CPU_QUIESCE_COUNTER_LEFT);

	assert((os_atomic_load(&cpu_quiescing_checkin_state, relaxed) &
	    (cpu_expected_bit(cpuid) | cpu_checked_in_bit(cpuid))) == 0);

	st->state = CPU_QUIESCE_COUNTER_PENDING_JOIN;

	  
                                                                  
                              
    
	ast_on(AST_UNQUIESCE);
}

  
                                                                                            
                                                                                       
   
void
cpu_quiescent_counter_ast(void)
{
	struct cpu_quiesce *st = PERCPU_GET(cpu_quiesce);
	int cpuid = cpu_number();

	assert(st->state == CPU_QUIESCE_COUNTER_PENDING_JOIN);

	                                          
	assert((os_atomic_load(&cpu_quiescing_checkin_state, relaxed) &
	    (cpu_expected_bit(cpuid) | cpu_checked_in_bit(cpuid))) == 0);

	  
                                                                        
                                                                
                             
   
                                                             
                   
   
                                                                     
                                                              
                                                                    
                                                                      
                     
    

	st->state = CPU_QUIESCE_COUNTER_JOINED;
	st->last_checkin = mach_absolute_time();

	checkin_mask_t old_mask, new_mask;
	os_atomic_rmw_loop(&cpu_quiescing_checkin_state, old_mask, new_mask, acquire, {
		if (old_mask == 0) {
		        new_mask = old_mask | cpu_expected_bit(cpuid);
		} else {
		        new_mask = old_mask | cpu_expected_bit(cpuid) | cpu_checked_in_bit(cpuid);
		}
	});
}

  
                                                                         
                                                           
  
                                   
  
                                                                            
                                                           
                                                                            
                                                                              
   
void
cpu_quiescent_counter_leave(uint64_t ctime)
{
	struct cpu_quiesce *st = PERCPU_GET(cpu_quiesce);
	int cpuid = cpu_number();

	assert(st->state == CPU_QUIESCE_COUNTER_JOINED ||
	    st->state == CPU_QUIESCE_COUNTER_PENDING_JOIN);

	                                                                          
	ast_off(AST_UNQUIESCE);

	if (st->state == CPU_QUIESCE_COUNTER_PENDING_JOIN) {
		                                                                         
		st->state = CPU_QUIESCE_COUNTER_LEFT;
		return;
	}

	                                                                      
	st->last_checkin = ctime;

	checkin_mask_t mask = cpu_checked_in_bit(cpuid) | cpu_expected_bit(cpuid);

	checkin_mask_t orig_state = os_atomic_andnot_orig(&cpu_quiescing_checkin_state,
	    mask, acq_rel);

	assert((orig_state & cpu_expected_bit(cpuid)));

	st->state = CPU_QUIESCE_COUNTER_LEFT;

	if (cpu_quiescent_counter_needs_commit(orig_state)) {
		  
                                                                    
                                                                   
                                                               
                        
     
		return;
	}

	checkin_mask_t new_state = orig_state & ~mask;

	if (cpu_quiescent_counter_needs_commit(new_state)) {
		cpu_quiescent_counter_commit(ctime);
	}
}

  
                                                           
                                                              
  
                                   
   
void
cpu_quiescent_counter_checkin(uint64_t ctime)
{
	struct cpu_quiesce *st = PERCPU_GET(cpu_quiesce);
	int cpuid = cpu_number();

	assert(st->state != CPU_QUIESCE_COUNTER_NONE);

	                                                        
	if (__probable(st->state != CPU_QUIESCE_COUNTER_JOINED)) {
		return;
	}

	                                                                  
	if (__probable((ctime - st->last_checkin) <= cpu_checkin_min_interval)) {
		return;
	}

	st->last_checkin = ctime;

	checkin_mask_t state = os_atomic_load(&cpu_quiescing_checkin_state, relaxed);

	assert((state & cpu_expected_bit(cpuid)));

	if (__probable((state & cpu_checked_in_bit(cpuid)))) {
		  
                                                                
                                     
     
		return;
	}

	checkin_mask_t orig_state = os_atomic_or_orig(&cpu_quiescing_checkin_state,
	    cpu_checked_in_bit(cpuid), acq_rel);

	checkin_mask_t new_state = orig_state | cpu_checked_in_bit(cpuid);

	if (cpu_quiescent_counter_needs_commit(new_state)) {
		assertf(!cpu_quiescent_counter_needs_commit(orig_state),
		    "old: 0x%lx, new: 0x%lx", orig_state, new_state);
		cpu_quiescent_counter_commit(ctime);
	}
}

#if MACH_ASSERT
  
                                                                                
  
                                                                         
   
void
cpu_quiescent_counter_assert_ast(void)
{
	struct cpu_quiesce *st = PERCPU_GET(cpu_quiesce);
	int cpuid = cpu_number();

	assert(st->state == CPU_QUIESCE_COUNTER_JOINED);

	checkin_mask_t state = os_atomic_load(&cpu_quiescing_checkin_state, relaxed);
	assert((state & cpu_expected_bit(cpuid)));
}
#endif                  
