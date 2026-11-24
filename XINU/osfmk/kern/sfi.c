/*
 * Copyright (c) 2013 Apple Inc. All rights reserved.
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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
#include <mach/mach_types.h>
#include <kern/assert.h>
#include <kern/clock.h>
#include <kern/coalition.h>
#include <kern/debug.h>
#include <kern/startup.h>
#include <kern/host.h>
#include <kern/kern_types.h>
#include <kern/machine.h>
#include <kern/simple_lock.h>
#include <kern/misc_protos.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/sfi.h>
#include <kern/timer_call.h>
#include <kern/waitq.h>
#include <kern/ledger.h>
#include <kern/policy_internal.h>

#include <machine/atomic.h>

#include <pexpert/pexpert.h>

#include <libkern/kernel_mach_header.h>

#include <sys/kdebug.h>

#if CONFIG_SCHED_SFI

#define SFI_DEBUG 0

#if SFI_DEBUG
#define dprintf(...) kprintf(__VA_ARGS__)
#else
#define dprintf(...) do { } while(0)
#endif

  
                                                            
                                                                  
                                                          
                                                              
                                                         
                                                         
                                                                
                                               
   

  
                                                                   
                                                                
                                                                      
                                                                    
                                                                   
                                                                     
                                      
  
                                                                       
                                                                          
                                                                
                                              
  
                                                                          
                                                                         
                                                                     
                            
  
                                                                           
  
                                                                           
  
                                                     
                                               
                           
             
                            
   

decl_simple_lock_data(static, sfi_lock);
static timer_call_data_t        sfi_timer_call_entry;
volatile boolean_t      sfi_is_enabled;

boolean_t sfi_window_is_set;
uint64_t sfi_window_usecs;
uint64_t sfi_window_interval;
uint64_t sfi_next_off_deadline;

typedef struct {
	sfi_class_id_t  class_id;
	thread_continue_t       class_continuation;
	const char *    class_name;
	const char *    class_ledger_name;
} sfi_class_registration_t;

  
                          
  
                                                
                                                                                                    
                                            
                                                                                                            
                                          
   

static inline void _sfi_wait_cleanup(void);

static void sfi_class_register(sfi_class_registration_t *);

#define SFI_CLASS_REGISTER(clsid, ledger_name)                                  \
                                                                                \
static void __attribute__((noinline, noreturn))                                 \
SFI_ ## clsid ## _THREAD_IS_WAITING(void *arg __unused, wait_result_t wret __unused) \
{                                                                               \
	_sfi_wait_cleanup();                                                    \
	thread_exception_return();                                              \
}                                                                               \
                                                                                \
static_assert(SFI_CLASS_ ## clsid < MAX_SFI_CLASS_ID, "Invalid ID");            \
                                                                                \
static __startup_data sfi_class_registration_t                                  \
SFI_ ## clsid ## _registration = {                                              \
	.class_id = SFI_CLASS_ ## clsid,                                        \
	.class_continuation = SFI_ ## clsid ## _THREAD_IS_WAITING,              \
	.class_name = "SFI_CLASS_" # clsid,                                     \
	.class_ledger_name = "SFI_CLASS_" # ledger_name,                        \
};                                                                              \
STARTUP_ARG(TUNABLES, STARTUP_RANK_MIDDLE,                                      \
    sfi_class_register, &SFI_ ## clsid ## _registration)

                                             
SFI_CLASS_REGISTER(MAINTENANCE, MAINTENANCE);
SFI_CLASS_REGISTER(DARWIN_BG, DARWIN_BG);
SFI_CLASS_REGISTER(APP_NAP, APP_NAP);
SFI_CLASS_REGISTER(MANAGED_FOCAL, MANAGED);
SFI_CLASS_REGISTER(MANAGED_NONFOCAL, MANAGED);
SFI_CLASS_REGISTER(UTILITY, UTILITY);
SFI_CLASS_REGISTER(DEFAULT_FOCAL, DEFAULT);
SFI_CLASS_REGISTER(DEFAULT_NONFOCAL, DEFAULT);
SFI_CLASS_REGISTER(LEGACY_FOCAL, LEGACY);
SFI_CLASS_REGISTER(LEGACY_NONFOCAL, LEGACY);
SFI_CLASS_REGISTER(USER_INITIATED_FOCAL, USER_INITIATED);
SFI_CLASS_REGISTER(USER_INITIATED_NONFOCAL, USER_INITIATED);
SFI_CLASS_REGISTER(USER_INTERACTIVE_FOCAL, USER_INTERACTIVE);
SFI_CLASS_REGISTER(USER_INTERACTIVE_NONFOCAL, USER_INTERACTIVE);
SFI_CLASS_REGISTER(KERNEL, OPTED_OUT);
SFI_CLASS_REGISTER(OPTED_OUT, OPTED_OUT);

struct sfi_class_state {
	uint64_t        off_time_usecs;
	uint64_t        off_time_interval;

	timer_call_data_t       on_timer;
	uint64_t        on_timer_deadline;
	boolean_t                       on_timer_programmed;

	boolean_t       class_sfi_is_enabled;
	volatile boolean_t      class_in_on_phase;

	struct waitq            waitq;                              
	thread_continue_t       continuation;

	const char *    class_name;
	const char *    class_ledger_name;
};

                                                        
struct sfi_class_state sfi_classes[MAX_SFI_CLASS_ID];

int sfi_enabled_class_count;                                             

static void sfi_timer_global_off(
	timer_call_param_t      param0,
	timer_call_param_t      param1);

static void sfi_timer_per_class_on(
	timer_call_param_t      param0,
	timer_call_param_t      param1);

                                                          
__startup_func
static void
sfi_class_register(sfi_class_registration_t *reg)
{
	sfi_class_id_t class_id = reg->class_id;

	if (class_id >= MAX_SFI_CLASS_ID) {
		panic("Invalid SFI class 0x%x", class_id);
	}
	if (sfi_classes[class_id].continuation != NULL) {
		panic("Duplicate SFI registration for class 0x%x", class_id);
	}
	sfi_classes[class_id].class_sfi_is_enabled = FALSE;
	sfi_classes[class_id].class_in_on_phase = TRUE;
	sfi_classes[class_id].continuation = reg->class_continuation;
	sfi_classes[class_id].class_name = reg->class_name;
	sfi_classes[class_id].class_ledger_name = reg->class_ledger_name;
}

void
sfi_init(void)
{
	sfi_class_id_t i;
	kern_return_t kret;

	simple_lock_init(&sfi_lock, 0);
	timer_call_setup(&sfi_timer_call_entry, sfi_timer_global_off, NULL);
	sfi_window_is_set = FALSE;
	os_atomic_init(&sfi_enabled_class_count, 0);
	sfi_is_enabled = FALSE;

	for (i = 0; i < MAX_SFI_CLASS_ID; i++) {
		                                                                              
		if (sfi_classes[i].continuation) {
			timer_call_setup(&sfi_classes[i].on_timer, sfi_timer_per_class_on, (void *)(uintptr_t)i);
			sfi_classes[i].on_timer_programmed = FALSE;

			kret = waitq_init(&sfi_classes[i].waitq, SYNC_POLICY_FIFO | SYNC_POLICY_DISABLE_IRQ);
			assert(kret == KERN_SUCCESS);
		} else {
			                                                       
			if (i != SFI_CLASS_UNSPECIFIED) {
				panic("Gap in registered SFI classes");
			}
		}
	}
}

                                                                                        
sfi_class_id_t
sfi_get_ledger_alias_for_class(sfi_class_id_t class_id)
{
	sfi_class_id_t i;
	const char *ledger_name = NULL;

	ledger_name = sfi_classes[class_id].class_ledger_name;

	                                                                          
	if (ledger_name) {
		for (i = SFI_CLASS_UNSPECIFIED + 1; i < class_id; i++) {
			if (0 == strcmp(sfi_classes[i].class_ledger_name, ledger_name)) {
				dprintf("sfi_get_ledger_alias_for_class(0x%x) -> 0x%x\n", class_id, i);
				return i;
			}
		}

		                                                                        
		dprintf("sfi_get_ledger_alias_for_class(0x%x) -> 0x%x\n", class_id, SFI_CLASS_UNSPECIFIED);
		return SFI_CLASS_UNSPECIFIED;
	}

	                                                                                   
	return SFI_CLASS_UNSPECIFIED;
}

int
sfi_ledger_entry_add(ledger_template_t template, sfi_class_id_t class_id)
{
	const char *ledger_name = NULL;

	ledger_name = sfi_classes[class_id].class_ledger_name;

	dprintf("sfi_ledger_entry_add(%p, 0x%x) -> %s\n", template, class_id, ledger_name);
	return ledger_entry_add(template, ledger_name, "sfi", "MATUs");
}

static void
sfi_timer_global_off(
	timer_call_param_t      param0 __unused,
	timer_call_param_t      param1 __unused)
{
	uint64_t        now = mach_absolute_time();
	sfi_class_id_t  i;
	processor_set_t pset, nset;
	processor_t             processor;
	uint32_t                needs_cause_ast_mask = 0x0;
	spl_t           s;

	s = splsched();

	simple_lock(&sfi_lock, LCK_GRP_NULL);
	if (!sfi_is_enabled) {
		                                                                   
		KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_OFF_TIMER) | DBG_FUNC_NONE, 1, 0, 0, 0, 0);

		simple_unlock(&sfi_lock);
		splx(s);
		return;
	}

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_OFF_TIMER) | DBG_FUNC_START, 0, 0, 0, 0, 0);

	                                                                                       
	for (i = 0; i < MAX_SFI_CLASS_ID; i++) {
		if (sfi_classes[i].class_sfi_is_enabled) {
			uint64_t on_timer_deadline;

			sfi_classes[i].class_in_on_phase = FALSE;
			sfi_classes[i].on_timer_programmed = TRUE;

			                       
			on_timer_deadline = now + sfi_classes[i].off_time_interval;
			sfi_classes[i].on_timer_deadline = on_timer_deadline;

			timer_call_enter1(&sfi_classes[i].on_timer, NULL, on_timer_deadline, TIMER_CALL_SYS_CRITICAL);
		} else {
			                                                                         
			sfi_classes[i].class_in_on_phase = TRUE;
			if (sfi_classes[i].on_timer_programmed) {
				sfi_classes[i].on_timer_programmed = FALSE;
				sfi_classes[i].on_timer_deadline = ~0ULL;
				timer_call_cancel(&sfi_classes[i].on_timer);
			}
		}
	}
	simple_unlock(&sfi_lock);

	                                                                                                             
	processor = processor_list;
	pset = processor->processor_set;

	pset_lock(pset);

	do {
		nset = processor->processor_set;
		if (nset != pset) {
			pset_unlock(pset);
			pset = nset;
			pset_lock(pset);
		}

		                                         
		if (processor->state == PROCESSOR_RUNNING) {
			if (AST_NONE != sfi_processor_needs_ast(processor)) {
				needs_cause_ast_mask |= (1U << processor->cpu_id);
			}
		}
	} while ((processor = processor->processor_list) != NULL);

	pset_unlock(pset);

	for (int cpuid = lsb_first(needs_cause_ast_mask); cpuid >= 0; cpuid = lsb_next(needs_cause_ast_mask, cpuid)) {
		processor = processor_array[cpuid];
		if (processor == current_processor()) {
			ast_on(AST_SFI);
		} else {
			cause_ast_check(processor);
		}
	}

	                                   
	simple_lock(&sfi_lock, LCK_GRP_NULL);
	if (sfi_is_enabled) {
		clock_deadline_for_periodic_event(sfi_window_interval,
		    now,
		    &sfi_next_off_deadline);
		timer_call_enter1(&sfi_timer_call_entry,
		    NULL,
		    sfi_next_off_deadline,
		    TIMER_CALL_SYS_CRITICAL);
	}

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_OFF_TIMER) | DBG_FUNC_END, 0, 0, 0, 0, 0);

	simple_unlock(&sfi_lock);

	splx(s);
}

static void
sfi_timer_per_class_on(
	timer_call_param_t      param0,
	timer_call_param_t      param1 __unused)
{
	sfi_class_id_t sfi_class_id = (sfi_class_id_t)(uintptr_t)param0;
	struct sfi_class_state  *sfi_class = &sfi_classes[sfi_class_id];
	kern_return_t   kret;
	spl_t           s;

	s = splsched();

	simple_lock(&sfi_lock, LCK_GRP_NULL);

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_ON_TIMER) | DBG_FUNC_START, sfi_class_id, 0, 0, 0, 0);

	  
                                                                                                   
                                                                                   
                                                                                       
    

	sfi_class->class_in_on_phase = TRUE;
	sfi_class->on_timer_programmed = FALSE;

	kret = waitq_wakeup64_all(&sfi_class->waitq,
	    CAST_EVENT64_T(sfi_class_id),
	    THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);
	assert(kret == KERN_SUCCESS || kret == KERN_NOT_WAITING);

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_ON_TIMER) | DBG_FUNC_END, 0, 0, 0, 0, 0);

	simple_unlock(&sfi_lock);

	splx(s);
}


kern_return_t
sfi_set_window(uint64_t window_usecs)
{
	uint64_t        interval, deadline;
	uint64_t        now = mach_absolute_time();
	sfi_class_id_t  i;
	spl_t           s;
	uint64_t        largest_class_off_interval = 0;

	if (window_usecs < MIN_SFI_WINDOW_USEC) {
		window_usecs = MIN_SFI_WINDOW_USEC;
	}

	if (window_usecs > UINT32_MAX) {
		return KERN_INVALID_ARGUMENT;
	}

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_SET_WINDOW), window_usecs, 0, 0, 0, 0);

	clock_interval_to_absolutetime_interval((uint32_t)window_usecs, NSEC_PER_USEC, &interval);
	deadline = now + interval;

	s = splsched();

	simple_lock(&sfi_lock, LCK_GRP_NULL);

	                                                                             
	for (i = 0; i < MAX_SFI_CLASS_ID; i++) {
		if (sfi_classes[i].class_sfi_is_enabled) {
			largest_class_off_interval = MAX(largest_class_off_interval, sfi_classes[i].off_time_interval);
		}
	}

	  
                                                                 
                                                                             
    
	if (interval <= largest_class_off_interval) {
		simple_unlock(&sfi_lock);
		splx(s);
		return KERN_INVALID_ARGUMENT;
	}

	  
                                                                               
                                                                                         
                                                                                
                                                                                  
                                                         
    
	sfi_window_usecs = window_usecs;
	sfi_window_interval = interval;
	sfi_window_is_set = TRUE;

	if (os_atomic_load(&sfi_enabled_class_count, relaxed) == 0) {
		                             
	} else if (!sfi_is_enabled) {
		sfi_is_enabled = TRUE;
		sfi_next_off_deadline = deadline;
		timer_call_enter1(&sfi_timer_call_entry,
		    NULL,
		    sfi_next_off_deadline,
		    TIMER_CALL_SYS_CRITICAL);
	} else if (deadline >= sfi_next_off_deadline) {
		sfi_next_off_deadline = deadline;
	} else {
		sfi_next_off_deadline = deadline;
		timer_call_enter1(&sfi_timer_call_entry,
		    NULL,
		    sfi_next_off_deadline,
		    TIMER_CALL_SYS_CRITICAL);
	}

	simple_unlock(&sfi_lock);
	splx(s);

	return KERN_SUCCESS;
}

kern_return_t
sfi_window_cancel(void)
{
	spl_t           s;

	s = splsched();

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_CANCEL_WINDOW), 0, 0, 0, 0, 0);

	                                                                
	simple_lock(&sfi_lock, LCK_GRP_NULL);
	sfi_window_is_set = FALSE;
	sfi_window_usecs = 0;
	sfi_window_interval = 0;
	sfi_next_off_deadline = 0;
	sfi_is_enabled = FALSE;
	simple_unlock(&sfi_lock);

	splx(s);

	return KERN_SUCCESS;
}

                                                                             
                                                                          
                                                                                
                                                                               
                                                                                  
                                                                                   
   

kern_return_t
sfi_defer(uint64_t sfi_defer_matus)
{
	spl_t           s;
	kern_return_t kr = KERN_FAILURE;
	s = splsched();

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_GLOBAL_DEFER), sfi_defer_matus, 0, 0, 0, 0);

	simple_lock(&sfi_lock, LCK_GRP_NULL);
	if (!sfi_is_enabled) {
		goto sfi_defer_done;
	}

	assert(sfi_next_off_deadline != 0);

	sfi_next_off_deadline += sfi_defer_matus;
	timer_call_enter1(&sfi_timer_call_entry, NULL, sfi_next_off_deadline, TIMER_CALL_SYS_CRITICAL);

	int i;
	for (i = 0; i < MAX_SFI_CLASS_ID; i++) {
		if (sfi_classes[i].class_sfi_is_enabled) {
			if (sfi_classes[i].on_timer_programmed) {
				uint64_t new_on_deadline = sfi_classes[i].on_timer_deadline + sfi_defer_matus;
				sfi_classes[i].on_timer_deadline = new_on_deadline;
				timer_call_enter1(&sfi_classes[i].on_timer, NULL, new_on_deadline, TIMER_CALL_SYS_CRITICAL);
			}
		}
	}

	kr = KERN_SUCCESS;
sfi_defer_done:
	simple_unlock(&sfi_lock);

	splx(s);

	return kr;
}


kern_return_t
sfi_get_window(uint64_t *window_usecs)
{
	spl_t           s;
	uint64_t        off_window_us;

	s = splsched();
	simple_lock(&sfi_lock, LCK_GRP_NULL);

	off_window_us = sfi_window_usecs;

	simple_unlock(&sfi_lock);
	splx(s);

	*window_usecs = off_window_us;

	return KERN_SUCCESS;
}


kern_return_t
sfi_set_class_offtime(sfi_class_id_t class_id, uint64_t offtime_usecs)
{
	uint64_t        interval;
	spl_t           s;
	uint64_t        off_window_interval;

	if (offtime_usecs < MIN_SFI_WINDOW_USEC) {
		offtime_usecs = MIN_SFI_WINDOW_USEC;
	}

	if (class_id == SFI_CLASS_UNSPECIFIED || class_id >= MAX_SFI_CLASS_ID) {
		return KERN_INVALID_ARGUMENT;
	}

	if (offtime_usecs > UINT32_MAX) {
		return KERN_INVALID_ARGUMENT;
	}

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_SET_CLASS_OFFTIME), offtime_usecs, class_id, 0, 0, 0);

	clock_interval_to_absolutetime_interval((uint32_t)offtime_usecs, NSEC_PER_USEC, &interval);

	s = splsched();

	simple_lock(&sfi_lock, LCK_GRP_NULL);
	off_window_interval = sfi_window_interval;

	                                                                                 
	if (off_window_interval && (interval >= off_window_interval)) {
		simple_unlock(&sfi_lock);
		splx(s);
		return KERN_INVALID_ARGUMENT;
	}

	                                                                                         
	if (!sfi_classes[class_id].class_sfi_is_enabled) {
		os_atomic_inc(&sfi_enabled_class_count, relaxed);
	}
	sfi_classes[class_id].off_time_usecs = offtime_usecs;
	sfi_classes[class_id].off_time_interval = interval;
	sfi_classes[class_id].class_sfi_is_enabled = TRUE;

	if (sfi_window_is_set && !sfi_is_enabled) {
		                            
		sfi_is_enabled = TRUE;
		sfi_next_off_deadline = mach_absolute_time() + sfi_window_interval;
		timer_call_enter1(&sfi_timer_call_entry,
		    NULL,
		    sfi_next_off_deadline,
		    TIMER_CALL_SYS_CRITICAL);
	}

	simple_unlock(&sfi_lock);

	splx(s);

	return KERN_SUCCESS;
}

kern_return_t
sfi_class_offtime_cancel(sfi_class_id_t class_id)
{
	spl_t           s;

	if (class_id == SFI_CLASS_UNSPECIFIED || class_id >= MAX_SFI_CLASS_ID) {
		return KERN_INVALID_ARGUMENT;
	}

	s = splsched();

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_CANCEL_CLASS_OFFTIME), class_id, 0, 0, 0, 0);

	simple_lock(&sfi_lock, LCK_GRP_NULL);

	                                                                                         
	if (sfi_classes[class_id].class_sfi_is_enabled) {
		os_atomic_dec(&sfi_enabled_class_count, relaxed);
	}
	sfi_classes[class_id].off_time_usecs = 0;
	sfi_classes[class_id].off_time_interval = 0;
	sfi_classes[class_id].class_sfi_is_enabled = FALSE;

	if (os_atomic_load(&sfi_enabled_class_count, relaxed) == 0) {
		sfi_is_enabled = FALSE;
	}

	simple_unlock(&sfi_lock);

	splx(s);

	return KERN_SUCCESS;
}

kern_return_t
sfi_get_class_offtime(sfi_class_id_t class_id, uint64_t *offtime_usecs)
{
	uint64_t        off_time_us;
	spl_t           s;

	if (class_id == SFI_CLASS_UNSPECIFIED || class_id >= MAX_SFI_CLASS_ID) {
		return 0;
	}

	s = splsched();

	simple_lock(&sfi_lock, LCK_GRP_NULL);
	off_time_us = sfi_classes[class_id].off_time_usecs;
	simple_unlock(&sfi_lock);

	splx(s);

	*offtime_usecs = off_time_us;

	return KERN_SUCCESS;
}

  
                                                                                    
                                                                                     
                                                                                
                                                                               
                                                                               
                                                                               
                                                                           
                                                                                   
                                                                              
                                                                                   
                              
   

  
                                                                
                                        
   
sfi_class_id_t
sfi_thread_classify(thread_t thread)
{
	task_t task = thread->task;
	boolean_t is_kernel_thread = (task == kernel_task);
	sched_mode_t thmode = thread->sched_mode;
	boolean_t focal = FALSE;

	                                                                                           
	if (is_kernel_thread) {
		return SFI_CLASS_KERNEL;
	}

	                                                                                   
	if (os_atomic_load(&sfi_enabled_class_count, relaxed) == 0) {
		return SFI_CLASS_OPTED_OUT;
	}

	int task_role       = proc_get_effective_task_policy(task, TASK_POLICY_ROLE);
	int latency_qos     = proc_get_effective_task_policy(task, TASK_POLICY_LATENCY_QOS);
	int managed_task    = proc_get_effective_task_policy(task, TASK_POLICY_SFI_MANAGED);

	int thread_qos      = proc_get_effective_thread_policy(thread, TASK_POLICY_QOS);
	int thread_bg       = proc_get_effective_thread_policy(thread, TASK_POLICY_DARWIN_BG);

	if (thread_qos == THREAD_QOS_MAINTENANCE) {
		return SFI_CLASS_MAINTENANCE;
	}

	if (thread_bg || thread_qos == THREAD_QOS_BACKGROUND) {
		return SFI_CLASS_DARWIN_BG;
	}

	if (latency_qos != 0) {
		int latency_qos_wtf = latency_qos - 1;

		if ((latency_qos_wtf >= 4) && (latency_qos_wtf <= 5)) {
			return SFI_CLASS_APP_NAP;
		}
	}

	  
                                                                            
                                                                  
    
	if (thmode == TH_MODE_REALTIME || thmode == TH_MODE_FIXED || task_role == TASK_GRAPHICS_SERVER) {
		return SFI_CLASS_OPTED_OUT;
	}

	  
                                                                                              
    
	switch (task_role) {
	case TASK_CONTROL_APPLICATION:
	case TASK_FOREGROUND_APPLICATION:
		focal = TRUE;
		break;
	case TASK_BACKGROUND_APPLICATION:
	case TASK_DEFAULT_APPLICATION:
	case TASK_UNSPECIFIED:
		                                                             
		if (task_coalition_focal_count(thread->task) > 0) {
			focal = TRUE;
		}
		break;
	case TASK_THROTTLE_APPLICATION:
	case TASK_DARWINBG_APPLICATION:
	case TASK_NONUI_APPLICATION:
	                          
	default:
		break;
	}

	if (managed_task) {
		switch (thread_qos) {
		case THREAD_QOS_UNSPECIFIED:
		case THREAD_QOS_LEGACY:
		case THREAD_QOS_USER_INITIATED:
			if (focal) {
				return SFI_CLASS_MANAGED_FOCAL;
			} else {
				return SFI_CLASS_MANAGED_NONFOCAL;
			}
		default:
			break;
		}
	}

	if (thread_qos == THREAD_QOS_UTILITY) {
		return SFI_CLASS_UTILITY;
	}

	  
                                         
    
	if (focal) {
		switch (thread_qos) {
		case THREAD_QOS_USER_INTERACTIVE:
			return SFI_CLASS_USER_INTERACTIVE_FOCAL;
		case THREAD_QOS_USER_INITIATED:
			return SFI_CLASS_USER_INITIATED_FOCAL;
		case THREAD_QOS_LEGACY:
			return SFI_CLASS_LEGACY_FOCAL;
		default:
			return SFI_CLASS_DEFAULT_FOCAL;
		}
	} else {
		switch (thread_qos) {
		case THREAD_QOS_USER_INTERACTIVE:
			return SFI_CLASS_USER_INTERACTIVE_NONFOCAL;
		case THREAD_QOS_USER_INITIATED:
			return SFI_CLASS_USER_INITIATED_NONFOCAL;
		case THREAD_QOS_LEGACY:
			return SFI_CLASS_LEGACY_NONFOCAL;
		default:
			return SFI_CLASS_DEFAULT_NONFOCAL;
		}
	}
}

  
                       
   
sfi_class_id_t
sfi_processor_active_thread_classify(processor_t processor)
{
	return processor->current_sfi_class;
}

  
                                                                       
                                                                     
                      
   
ast_t
sfi_thread_needs_ast(thread_t thread, sfi_class_id_t *out_class)
{
	sfi_class_id_t class_id;

	class_id = sfi_thread_classify(thread);

	if (out_class) {
		*out_class = class_id;
	}

	                                                  
	if (!sfi_classes[class_id].class_in_on_phase) {
		return AST_SFI;
	} else {
		return AST_NONE;
	}
}

  
                                                 
                                                  
                                                     
                                                         
                                                    
                                                
   
ast_t
sfi_processor_needs_ast(processor_t processor)
{
	sfi_class_id_t class_id;

	class_id = sfi_processor_active_thread_classify(processor);

	                                                  
	if (!sfi_classes[class_id].class_in_on_phase) {
		return AST_SFI;
	} else {
		return AST_NONE;
	}
}

static inline void
_sfi_wait_cleanup(void)
{
	thread_t self = current_thread();

	spl_t s = splsched();
	simple_lock(&sfi_lock, LCK_GRP_NULL);

	sfi_class_id_t current_sfi_wait_class = self->sfi_wait_class;

	assert((SFI_CLASS_UNSPECIFIED < current_sfi_wait_class) &&
	    (current_sfi_wait_class < MAX_SFI_CLASS_ID));

	self->sfi_wait_class = SFI_CLASS_UNSPECIFIED;

	simple_unlock(&sfi_lock);
	splx(s);

	  
                                                                     
                                                       
                                     
   
                                                                        
                                                                        
                             
   
                                                                   
                                                                      
                                                                        
                                                                
    

	if (self->wait_sfi_begin_time != 0) {
		uint64_t made_runnable = os_atomic_load(&self->last_made_runnable_time, relaxed);
		int64_t sfi_wait_time = made_runnable - self->wait_sfi_begin_time;
		assert(sfi_wait_time >= 0);

		ledger_credit(self->task->ledger, task_ledgers.sfi_wait_times[current_sfi_wait_class],
		    sfi_wait_time);

		self->wait_sfi_begin_time = 0;
	}
}

  
                                                                
                                                                    
                                                                        
                                   
   
void
sfi_ast(thread_t thread)
{
	sfi_class_id_t class_id;
	spl_t           s;
	struct sfi_class_state  *sfi_class;
	wait_result_t   waitret;
	boolean_t       did_wait = FALSE;
	thread_continue_t       continuation;

	s = splsched();

	simple_lock(&sfi_lock, LCK_GRP_NULL);

	if (!sfi_is_enabled) {
		  
                                                       
                                                              
                                                           
                                                       
     
		simple_unlock(&sfi_lock);
		splx(s);

		return;
	}

	thread_lock(thread);
	thread->sfi_class = class_id = sfi_thread_classify(thread);
	thread_unlock(thread);

	  
                                                                                
                                                                                           
                                                                                       
                                                                           
                                                                                  
                                                                                 
                   
    

	sfi_class = &sfi_classes[class_id];
	if (!sfi_class->class_in_on_phase) {
		                                        
		KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_THREAD_DEFER),
		    thread_tid(thread), class_id, 0, 0, 0);

		waitret = waitq_assert_wait64(&sfi_class->waitq,
		    CAST_EVENT64_T(class_id),
		    THREAD_INTERRUPTIBLE | THREAD_WAIT_NOREPORT, 0);
		if (waitret == THREAD_WAITING) {
			thread->sfi_wait_class = class_id;
			did_wait = TRUE;
			continuation = sfi_class->continuation;
		} else {
			                                                                    
			assert(waitret == THREAD_INTERRUPTED);
		}
	}
	simple_unlock(&sfi_lock);

	splx(s);

	if (did_wait) {
		assert(thread->wait_sfi_begin_time == 0);

		thread_block_reason(continuation, NULL, AST_SFI);
	}
}

                             
void
sfi_reevaluate(thread_t thread)
{
	kern_return_t kret;
	spl_t           s;
	sfi_class_id_t class_id, current_class_id;
	ast_t           sfi_ast;

	s = splsched();

	simple_lock(&sfi_lock, LCK_GRP_NULL);

	thread_lock(thread);
	sfi_ast = sfi_thread_needs_ast(thread, &class_id);
	thread->sfi_class = class_id;

	  
                                                                   
                                                                
   
                                                                    
                                                                  
                 
   
                                                                        
                                                                    
                                                                       
   
                                                                       
                                                                         
              
    

	if ((current_class_id = thread->sfi_wait_class) != SFI_CLASS_UNSPECIFIED) {
		thread_unlock(thread);                         

		assert(current_class_id < MAX_SFI_CLASS_ID);

		if ((sfi_ast == AST_NONE) || (class_id != current_class_id)) {
			struct sfi_class_state  *sfi_class = &sfi_classes[current_class_id];

			KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SFI, SFI_WAIT_CANCELED), thread_tid(thread), current_class_id, class_id, 0, 0);

			kret = waitq_wakeup64_thread(&sfi_class->waitq,
			    CAST_EVENT64_T(current_class_id),
			    thread,
			    THREAD_AWAKENED);
			assert(kret == KERN_SUCCESS || kret == KERN_NOT_WAITING);
		}
	} else {
		  
                                                               
                                         
     

		if ((thread->state & (TH_RUN | TH_IDLE)) == TH_RUN) {
			if (sfi_ast != AST_NONE) {
				if (thread == current_thread()) {
					ast_on(sfi_ast);
				} else {
					processor_t             processor = thread->last_processor;

					if (processor != PROCESSOR_NULL &&
					    processor->state == PROCESSOR_RUNNING &&
					    processor->active_thread == thread) {
						cause_ast_check(processor);
					} else {
						  
                                                                        
                                                                         
                                         
         
					}
				}
			}
		}

		thread_unlock(thread);
	}

	simple_unlock(&sfi_lock);
	splx(s);
}

#else                        

kern_return_t
sfi_set_window(uint64_t window_usecs __unused)
{
	return KERN_NOT_SUPPORTED;
}

kern_return_t
sfi_window_cancel(void)
{
	return KERN_NOT_SUPPORTED;
}


kern_return_t
sfi_get_window(uint64_t *window_usecs __unused)
{
	return KERN_NOT_SUPPORTED;
}


kern_return_t
sfi_set_class_offtime(sfi_class_id_t class_id __unused, uint64_t offtime_usecs __unused)
{
	return KERN_NOT_SUPPORTED;
}

kern_return_t
sfi_class_offtime_cancel(sfi_class_id_t class_id __unused)
{
	return KERN_NOT_SUPPORTED;
}

kern_return_t
sfi_get_class_offtime(sfi_class_id_t class_id __unused, uint64_t *offtime_usecs __unused)
{
	return KERN_NOT_SUPPORTED;
}

void
sfi_reevaluate(thread_t thread __unused)
{
	return;
}

sfi_class_id_t
sfi_thread_classify(thread_t thread)
{
	task_t task = thread->task;
	boolean_t is_kernel_thread = (task == kernel_task);

	if (is_kernel_thread) {
		return SFI_CLASS_KERNEL;
	}

	return SFI_CLASS_OPTED_OUT;
}

#endif                        
