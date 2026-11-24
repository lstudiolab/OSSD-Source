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

#include <kern/assert.h>
#include <kern/monotonic.h>
#include <kern/thread.h>
#include <machine/atomic.h>
#include <machine/monotonic.h>
#include <mach/mach_traps.h>
#include <stdatomic.h>
#include <sys/errno.h>

bool mt_debug = false;
_Atomic uint64_t mt_pmis = 0;
_Atomic uint64_t mt_retrograde = 0;

#define MT_KDBG_INSTRS_CYCLES(CODE) \
	KDBG_EVENTID(DBG_MONOTONIC, DBG_MT_INSTRS_CYCLES, CODE)

#define MT_KDBG_IC_CPU_CSWITCH MT_KDBG_INSTRS_CYCLES(1)

  
                                                                             
                                                                              
                                                                           
                                                                           
                            
  
                                                                                
                                                                              
                                                                           
                                                                              
              
   

#define MAXSPINS   100
#define MAXRETRIES 10

  
                                                                            
  
                                                                          
                                                      
   
int
mt_fixed_thread_counts(thread_t thread, uint64_t *counts_out)
{
	uint64_t start_gen, end_gen;
	uint64_t spins = 0, retries = 0;
	uint64_t counts[MT_CORE_NFIXED];

	  
                                                                      
                                                                      
             
   
                                     
    
spin:
	start_gen = atomic_load_explicit(&thread->t_monotonic.mth_gen,
	    memory_order_acquire);
retry:
	if (start_gen & 1) {
		spins++;
		if (spins > MAXSPINS) {
			return EBUSY;
		}
		goto spin;
	}

	for (int i = 0; i < MT_CORE_NFIXED; i++) {
		counts[i] = thread->t_monotonic.mth_counts[i];
	}

	  
                                                                    
                                                                   
                                                                      
                     
   
                                                                       
          
    
	end_gen = atomic_load_explicit(&thread->t_monotonic.mth_gen,
	    memory_order_acquire);
	if (end_gen != start_gen) {
		retries++;
		if (retries > MAXRETRIES) {
			return EAGAIN;
		}
		start_gen = end_gen;
		goto retry;
	}

	  
                                                                      
                                        
    
	for (int i = 0; i < MT_CORE_NFIXED; i++) {
		counts_out[i] = counts[i];
	}
	return 0;
}

static void mt_fixed_counts_internal(uint64_t *counts, uint64_t *counts_since);

bool
mt_update_thread(thread_t thread)
{
	if (!mt_core_supported) {
		return false;
	}

	assert(ml_get_interrupts_enabled() == FALSE);

	uint64_t counts[MT_CORE_NFIXED], counts_since[MT_CORE_NFIXED];
	mt_fixed_counts_internal(counts, counts_since);

	  
                                                                     
                                                                         
         
    
	__assert_only uint64_t enter_gen = atomic_fetch_add_explicit(
		&thread->t_monotonic.mth_gen, 1, memory_order_release);
	  
                                                            
    
	assert((enter_gen & 1) == 0);

	for (int i = 0; i < MT_CORE_NFIXED; i++) {
		thread->t_monotonic.mth_counts[i] += counts_since[i];
	}

	  
                                                                      
                                                                        
                                         
    
	__assert_only uint64_t exit_gen = atomic_fetch_add_explicit(
		&thread->t_monotonic.mth_gen, 1, memory_order_release);
	  
                                                      
    
	assert(exit_gen == (enter_gen + 1));

	return true;
}

void
mt_sched_update(thread_t thread)
{
	bool updated = mt_update_thread(thread);
	if (!updated) {
		return;
	}

	if (kdebug_debugid_explicitly_enabled(MT_KDBG_IC_CPU_CSWITCH)) {
		struct mt_cpu *mtc = mt_cur_cpu();

		KDBG_RELEASE(MT_KDBG_IC_CPU_CSWITCH,
#ifdef MT_CORE_INSTRS
		    mtc->mtc_counts[MT_CORE_INSTRS],
#else                              
		    0,
#endif                               
		    mtc->mtc_counts[MT_CORE_CYCLES]);
	}
}

int
mt_fixed_task_counts(task_t task, uint64_t *counts_out)
{
	assert(task != TASK_NULL);
	assert(counts_out != NULL);

	if (!mt_core_supported) {
		memset(counts_out, 0, sizeof(*counts_out) * MT_CORE_NFIXED);
		return 1;
	}

	task_lock(task);

	uint64_t counts[MT_CORE_NFIXED] = { 0 };
	for (int i = 0; i < MT_CORE_NFIXED; i++) {
		counts[i] = task->task_monotonic.mtk_counts[i];
	}

	uint64_t thread_counts[MT_CORE_NFIXED] = { 0 };
	thread_t thread = THREAD_NULL;
	thread_t curthread = current_thread();
	bool needs_current = false;
	int r = 0;
	queue_iterate(&task->threads, thread, thread_t, task_threads) {
		  
                                                       
                                               
     
		if (thread == curthread) {
			needs_current = true;
			continue;
		} else {
			r = mt_fixed_thread_counts(thread, thread_counts);
			if (r) {
				goto error;
			}
		}

		for (int i = 0; i < MT_CORE_NFIXED; i++) {
			counts[i] += thread_counts[i];
		}
	}

	task_unlock(task);

	if (needs_current) {
		mt_cur_thread_fixed_counts(thread_counts);
	}

	for (int i = 0; i < MT_CORE_NFIXED; i++) {
		if (needs_current) {
			counts[i] += thread_counts[i];
		}
		counts_out[i] = counts[i];
	}
	return 0;

error:
	task_unlock(task);
	return r;
}

uint64_t
mt_mtc_update_count(struct mt_cpu *mtc, unsigned int ctr)
{
	uint64_t snap = mt_core_snap(ctr);
	if (snap < mtc->mtc_snaps[ctr]) {
		if (mt_debug) {
			kprintf("monotonic: cpu %d: thread %#llx: "
			    "retrograde counter %u value: %llu, last read = %llu\n",
			    cpu_number(), thread_tid(current_thread()), ctr, snap,
			    mtc->mtc_snaps[ctr]);
		}
		(void)atomic_fetch_add_explicit(&mt_retrograde, 1,
		    memory_order_relaxed);
		mtc->mtc_snaps[ctr] = snap;
		return 0;
	}

	uint64_t count = snap - mtc->mtc_snaps[ctr];
	mtc->mtc_snaps[ctr] = snap;

	return count;
}

uint64_t
mt_cpu_update_count(cpu_data_t *cpu, unsigned int ctr)
{
	return mt_mtc_update_count(&cpu->cpu_monotonic, ctr);
}

static void
mt_fixed_counts_internal(uint64_t *counts, uint64_t *counts_since)
{
	assert(ml_get_interrupts_enabled() == FALSE);

	struct mt_cpu *mtc = mt_cur_cpu();
	assert(mtc != NULL);

	mt_mtc_update_fixed_counts(mtc, counts, counts_since);
}

void
mt_mtc_update_fixed_counts(struct mt_cpu *mtc, uint64_t *counts,
    uint64_t *counts_since)
{
	if (!mt_core_supported) {
		return;
	}

	for (int i = 0; i < MT_CORE_NFIXED; i++) {
		uint64_t last_delta;
		uint64_t count;

		last_delta = mt_mtc_update_count(mtc, i);
		count = mtc->mtc_counts[i] + last_delta;

		if (counts) {
			counts[i] = count;
		}
		if (counts_since) {
			assert(counts != NULL);
			counts_since[i] = count - mtc->mtc_counts_last[i];
			mtc->mtc_counts_last[i] = count;
		}

		mtc->mtc_counts[i] = count;
	}
}

void
mt_update_fixed_counts(void)
{
	assert(ml_get_interrupts_enabled() == FALSE);

#if defined(__x86_64__)
	__builtin_ia32_lfence();
#elif defined(__arm__) || defined(__arm64__)
	__builtin_arm_isb(ISB_SY);
#endif                                                                       

	mt_fixed_counts_internal(NULL, NULL);
}

void
mt_fixed_counts(uint64_t *counts)
{
#if defined(__x86_64__)
	__builtin_ia32_lfence();
#elif defined(__arm__) || defined(__arm64__)
	__builtin_arm_isb(ISB_SY);
#endif                                                                       

	int intrs_en = ml_set_interrupts_enabled(FALSE);
	mt_fixed_counts_internal(counts, NULL);
	ml_set_interrupts_enabled(intrs_en);
}

void
mt_cur_thread_fixed_counts(uint64_t *counts)
{
	if (!mt_core_supported) {
		memset(counts, 0, sizeof(*counts) * MT_CORE_NFIXED);
		return;
	}

	thread_t curthread = current_thread();
	int intrs_en = ml_set_interrupts_enabled(FALSE);
	(void)mt_update_thread(curthread);
	for (int i = 0; i < MT_CORE_NFIXED; i++) {
		counts[i] = curthread->t_monotonic.mth_counts[i];
	}
	ml_set_interrupts_enabled(intrs_en);
}

void
mt_cur_task_fixed_counts(uint64_t *counts)
{
	task_t curtask = current_task();

	mt_fixed_task_counts(curtask, counts);
}

                                                                       

uint64_t
mt_cur_thread_instrs(void)
{
#ifdef MT_CORE_INSTRS
	thread_t curthread = current_thread();
	boolean_t intrs_en;
	uint64_t count;

	if (!mt_core_supported) {
		return 0;
	}

	intrs_en = ml_set_interrupts_enabled(FALSE);
	(void)mt_update_thread(curthread);
	count = curthread->t_monotonic.mth_counts[MT_CORE_INSTRS];
	ml_set_interrupts_enabled(intrs_en);

	return count;
#else                              
	return 0;
#endif                               
}

uint64_t
mt_cur_thread_cycles(void)
{
	thread_t curthread = current_thread();
	boolean_t intrs_en;
	uint64_t count;

	if (!mt_core_supported) {
		return 0;
	}

	intrs_en = ml_set_interrupts_enabled(FALSE);
	(void)mt_update_thread(curthread);
	count = curthread->t_monotonic.mth_counts[MT_CORE_CYCLES];
	ml_set_interrupts_enabled(intrs_en);

	return count;
}

uint64_t
mt_cur_cpu_instrs(void)
{
#ifdef MT_CORE_INSTRS
	uint64_t counts[MT_CORE_NFIXED];

	if (!mt_core_supported) {
		return 0;
	}

	mt_fixed_counts(counts);
	return counts[MT_CORE_INSTRS];
#else                              
	return 0;
#endif                               
}

uint64_t
mt_cur_cpu_cycles(void)
{
	uint64_t counts[MT_CORE_NFIXED];

	if (!mt_core_supported) {
		return 0;
	}

	mt_fixed_counts(counts);
	return counts[MT_CORE_CYCLES];
}

void
mt_update_task(task_t task, thread_t thread)
{
	task_lock_assert_owned(task);

	if (!mt_core_supported) {
		return;
	}

	for (int i = 0; i < MT_CORE_NFIXED; i++) {
		task->task_monotonic.mtk_counts[i] += thread->t_monotonic.mth_counts[i];
	}
}

void
mt_terminate_update(task_t task, thread_t thread)
{
	mt_update_task(task, thread);
}

void
mt_perfcontrol(uint64_t *instrs, uint64_t *cycles)
{
	if (!mt_core_supported) {
		*instrs = 0;
		*cycles = 0;
		return;
	}

	struct mt_cpu *mtc = mt_cur_cpu();

	  
                                                                            
                                                                       
                              
    

#ifdef MT_CORE_INSTRS
	*instrs = mtc->mtc_snaps[MT_CORE_INSTRS];
#else                              
	*instrs = 0;
#endif                               

	*cycles = mtc->mtc_snaps[MT_CORE_CYCLES];
}

void
mt_stackshot_thread(thread_t thread, uint64_t *instrs, uint64_t *cycles)
{
	assert(mt_core_supported);

#ifdef MT_CORE_INSTRS
	*instrs = thread->t_monotonic.mth_counts[MT_CORE_INSTRS];
#else                              
	*instrs = 0;
#endif                               

	*cycles = thread->t_monotonic.mth_counts[MT_CORE_CYCLES];
}

void
mt_stackshot_task(task_t task, uint64_t *instrs, uint64_t *cycles)
{
	assert(mt_core_supported);

#ifdef MT_CORE_INSTRS
	*instrs = task->task_monotonic.mtk_counts[MT_CORE_INSTRS];
#else                              
	*instrs = 0;
#endif                               

	*cycles = task->task_monotonic.mtk_counts[MT_CORE_CYCLES];
}

  
                                                                        
                                                                               
                               
   

bool mt_microstackshots = false;
unsigned int mt_microstackshot_ctr = 0;
mt_pmi_fn mt_microstackshot_pmi_handler = NULL;
void *mt_microstackshot_ctx = NULL;
uint64_t mt_core_reset_values[MT_CORE_NFIXED] = { 0 };

#define MT_MIN_FIXED_PERIOD (10 * 1000 * 1000)

int
mt_microstackshot_start(unsigned int ctr, uint64_t period, mt_pmi_fn handler,
    void *ctx)
{
	assert(ctr < MT_CORE_NFIXED);

	if (period < MT_MIN_FIXED_PERIOD) {
		return EINVAL;
	}
	if (mt_microstackshots) {
		return EBUSY;
	}

	mt_microstackshot_ctr = ctr;
	mt_microstackshot_pmi_handler = handler;
	mt_microstackshot_ctx = ctx;

	int error = mt_microstackshot_start_arch(period);
	if (error) {
		mt_microstackshot_ctr = 0;
		mt_microstackshot_pmi_handler = NULL;
		mt_microstackshot_ctx = NULL;
		return error;
	}

	mt_microstackshots = true;

	return 0;
}

int
mt_microstackshot_stop(void)
{
	mt_microstackshots = false;
	memset(mt_core_reset_values, 0, sizeof(mt_core_reset_values));

	return 0;
}
