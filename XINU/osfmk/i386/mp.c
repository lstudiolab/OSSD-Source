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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_COPYRIGHT@
 */

#include <mach_kdp.h>
#include <kdp/kdp_internal.h>
#include <mach_ldebug.h>

#include <mach/mach_types.h>
#include <mach/kern_return.h>

#include <kern/kern_types.h>
#include <kern/startup.h>
#include <kern/timer_queue.h>
#include <kern/processor.h>
#include <kern/cpu_number.h>
#include <kern/cpu_data.h>
#include <kern/assert.h>
#include <kern/lock_group.h>
#include <kern/machine.h>
#include <kern/pms.h>
#include <kern/misc_protos.h>
#include <kern/timer_call.h>
#include <kern/zalloc.h>
#include <kern/queue.h>
#include <prng/random.h>

#include <vm/vm_map.h>
#include <vm/vm_kern.h>

#include <i386/bit_routines.h>
#include <i386/proc_reg.h>
#include <i386/cpu_threads.h>
#include <i386/mp_desc.h>
#include <i386/misc_protos.h>
#include <i386/trap.h>
#include <i386/postcode.h>
#include <i386/machine_routines.h>
#include <i386/mp.h>
#include <i386/mp_events.h>
#include <i386/lapic.h>
#include <i386/cpuid.h>
#include <i386/fpu.h>
#include <i386/machine_cpu.h>
#include <i386/pmCPU.h>
#if CONFIG_MCA
#include <i386/machine_check.h>
#endif
#include <i386/acpi.h>

#include <sys/kdebug.h>

#include <console/serial_protos.h>

#if MONOTONIC
#include <kern/monotonic.h>
#endif                

#if     MP_DEBUG
#define PAUSE           delay(1000000)
#define DBG(x...)       kprintf(x)
#else
#define DBG(x...)
#define PAUSE
#endif                

                                  
#define TRACE_MP_TLB_FLUSH              MACHDBG_CODE(DBG_MACH_MP, 0)
#define TRACE_MP_CPUS_CALL              MACHDBG_CODE(DBG_MACH_MP, 1)
#define TRACE_MP_CPUS_CALL_LOCAL        MACHDBG_CODE(DBG_MACH_MP, 2)
#define TRACE_MP_CPUS_CALL_ACTION       MACHDBG_CODE(DBG_MACH_MP, 3)
#define TRACE_MP_CPUS_CALL_NOBUF        MACHDBG_CODE(DBG_MACH_MP, 4)
#define TRACE_MP_CPU_FAST_START         MACHDBG_CODE(DBG_MACH_MP, 5)
#define TRACE_MP_CPU_START              MACHDBG_CODE(DBG_MACH_MP, 6)
#define TRACE_MP_CPU_DEACTIVATE         MACHDBG_CODE(DBG_MACH_MP, 7)

#define ABS(v)          (((v) > 0)?(v):-(v))

void            slave_boot_init(void);
void            i386_cpu_IPI(int cpu);

#if MACH_KDP
static void     mp_kdp_wait(boolean_t flush, boolean_t isNMI);
#endif               

#if MACH_KDP
static boolean_t        cpu_signal_pending(int cpu, mp_event_t event);
#endif               
static int              NMIInterruptHandler(x86_saved_state_t *regs);

boolean_t               smp_initialized = FALSE;
uint32_t                TSC_sync_margin = 0xFFF;
volatile boolean_t      force_immediate_debugger_NMI = FALSE;
volatile boolean_t      pmap_tlb_flush_timeout = FALSE;
#if DEBUG || DEVELOPMENT
boolean_t               mp_interrupt_watchdog_enabled = TRUE;
uint32_t                mp_interrupt_watchdog_events = 0;
#endif

SIMPLE_LOCK_DECLARE(debugger_callback_lock, 0);
struct debugger_callback *debugger_callback = NULL;

static LCK_GRP_DECLARE(smp_lck_grp, "i386_smp");
static LCK_MTX_EARLY_DECLARE(mp_cpu_boot_lock, &smp_lck_grp);

                                         
SIMPLE_LOCK_DECLARE(mp_rv_lock, 0);
static void     (*mp_rv_setup_func)(void *arg);
static void     (*mp_rv_action_func)(void *arg);
static void     (*mp_rv_teardown_func)(void *arg);
static void     *mp_rv_func_arg;
static volatile int     mp_rv_ncpus;
                             
static volatile long    mp_rv_entry    __attribute__((aligned(64)));
static volatile long    mp_rv_exit     __attribute__((aligned(64)));
static volatile long    mp_rv_complete __attribute__((aligned(64)));

volatile        uint64_t        debugger_entry_time;
volatile        uint64_t        debugger_exit_time;
#if MACH_KDP
#include <kdp/kdp.h>
extern int kdp_snapshot;
static struct _kdp_xcpu_call_func {
	kdp_x86_xcpu_func_t func;
	void     *arg0, *arg1;
	volatile long     ret;
	volatile uint16_t cpu;
} kdp_xcpu_call_func = {
	.cpu  = KDP_XCPU_NONE
};

#endif

                                        
static void        (*mp_bc_action_func)(void *arg);
static void        *mp_bc_func_arg;
static int      mp_bc_ncpus;
static volatile long   mp_bc_count;
static LCK_MTX_EARLY_DECLARE(mp_bc_lock, &smp_lck_grp);
static  volatile int    debugger_cpu = -1;
volatile long    NMIPI_acks = 0;
volatile long    NMI_count = 0;
static NMI_reason_t     NMI_panic_reason = NONE;
static int              vector_timed_out;

extern void     NMI_cpus(void);

static void     mp_cpus_call_init(void);
static void     mp_cpus_call_action(void);
static void     mp_call_PM(void);

char            mp_slave_stack[PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));                             

                          
boolean_t i386_smp_init(int nmi_vector, i386_intr_func_t nmi_handler,
    int ipi_vector, i386_intr_func_t ipi_handler);
void i386_start_cpu(int lapic_id, int cpu_num);
void i386_send_NMI(int cpu);
void NMIPI_enable(boolean_t);

#define NUM_CPU_WARM_CALLS      20
struct timer_call       cpu_warm_call_arr[NUM_CPU_WARM_CALLS];
queue_head_t            cpu_warm_call_list;
decl_simple_lock_data(static, cpu_warm_lock);

typedef struct cpu_warm_data {
	timer_call_t    cwd_call;
	uint64_t        cwd_deadline;
	int             cwd_result;
} *cpu_warm_data_t;

static void             cpu_prewarm_init(void);
static void             cpu_warm_timer_call_func(timer_call_param_t p0, timer_call_param_t p1);
static void             _cpu_warm_setup(void *arg);
static timer_call_t     grab_warm_timer_call(void);
static void             free_warm_timer_call(timer_call_t call);

void
smp_init(void)
{
	console_init();

	if (!i386_smp_init(LAPIC_NMI_INTERRUPT, NMIInterruptHandler,
	    LAPIC_VECTOR(INTERPROCESSOR), cpu_signal_handler)) {
		return;
	}

	cpu_thread_init();

	DBGLOG_CPU_INIT(master_cpu);

	mp_cpus_call_init();
	mp_cpus_call_cpu_init(master_cpu);

#if DEBUG || DEVELOPMENT
	if (PE_parse_boot_argn("interrupt_watchdog",
	    &mp_interrupt_watchdog_enabled,
	    sizeof(mp_interrupt_watchdog_enabled))) {
		kprintf("Interrupt watchdog %sabled\n",
		    mp_interrupt_watchdog_enabled ? "en" : "dis");
	}
#endif

	if (PE_parse_boot_argn("TSC_sync_margin",
	    &TSC_sync_margin, sizeof(TSC_sync_margin))) {
		kprintf("TSC sync Margin 0x%x\n", TSC_sync_margin);
	} else if (cpuid_vmm_present()) {
		kprintf("TSC sync margin disabled\n");
		TSC_sync_margin = 0;
	}
	smp_initialized = TRUE;

	cpu_prewarm_init();

	return;
}

typedef struct {
	int                     target_cpu;
	int                     target_lapic;
	int                     starter_cpu;
} processor_start_info_t;
static processor_start_info_t   start_info        __attribute__((aligned(64)));

  
                                                                    
   
static volatile long            tsc_entry_barrier __attribute__((aligned(64)));
static volatile long            tsc_exit_barrier  __attribute__((aligned(64)));
static volatile uint64_t        tsc_target        __attribute__((aligned(64)));

  
                                                          
   
static void
mp_wait_for_cpu_up(int slot_num, unsigned int iters, unsigned int usecdelay)
{
	while (iters-- > 0) {
		if (cpu_datap(slot_num)->cpu_running) {
			break;
		}
		delay(usecdelay);
	}
}

  
                                                         
   
kern_return_t
intel_startCPU_fast(int slot_num)
{
	kern_return_t   rc;

	  
                                 
    
	rc = pmCPUExitHalt(slot_num);
	if (rc != KERN_SUCCESS) {
		  
                                                 
     
		return rc;
	}

	KERNEL_DEBUG_CONSTANT(
		TRACE_MP_CPU_FAST_START | DBG_FUNC_START,
		slot_num, 0, 0, 0, 0);

	  
                                      
    
	mp_disable_preemption();

	  
                                                                    
                                                                 
                     
    

	mp_wait_for_cpu_up(slot_num, 30000, 1);
	mp_enable_preemption();

	KERNEL_DEBUG_CONSTANT(
		TRACE_MP_CPU_FAST_START | DBG_FUNC_END,
		slot_num, cpu_datap(slot_num)->cpu_running, 0, 0, 0);

	  
                                                               
                             
    
	if (cpu_datap(slot_num)->cpu_running) {
		return KERN_SUCCESS;
	} else {
		return KERN_FAILURE;
	}
}

static void
started_cpu(void)
{
	                                                       

	if (TSC_sync_margin &&
	    start_info.target_cpu == cpu_number()) {
		  
                                                                 
                          
     
		tsc_target   = 0;
		atomic_decl(&tsc_entry_barrier, 1);
		while (tsc_entry_barrier != 0) {
			;                                                   
		}
		tsc_target = rdtsc64();
		atomic_decl(&tsc_exit_barrier, 1);
	}
}

static void
start_cpu(void *arg)
{
	int                     i = 1000;
	processor_start_info_t  *psip = (processor_start_info_t *) arg;

	                                                             
	if (cpu_number() != psip->starter_cpu) {
		return;
	}

	DBG("start_cpu(%p) about to start cpu %d, lapic %d\n",
	    arg, psip->target_cpu, psip->target_lapic);

	KERNEL_DEBUG_CONSTANT(
		TRACE_MP_CPU_START | DBG_FUNC_START,
		psip->target_cpu,
		psip->target_lapic, 0, 0, 0);

	i386_start_cpu(psip->target_lapic, psip->target_cpu);

#ifdef  POSTCODE_DELAY
	                                                                     
	i *= 10000;
#endif
	DBG("start_cpu(%p) about to wait for cpu %d\n",
	    arg, psip->target_cpu);

	mp_wait_for_cpu_up(psip->target_cpu, i * 100, 100);

	KERNEL_DEBUG_CONSTANT(
		TRACE_MP_CPU_START | DBG_FUNC_END,
		psip->target_cpu,
		cpu_datap(psip->target_cpu)->cpu_running, 0, 0, 0);

	if (TSC_sync_margin &&
	    cpu_datap(psip->target_cpu)->cpu_running) {
		  
                                                          
                                                     
                                                         
                                                               
     
		uint64_t        tsc_starter;
		int64_t         tsc_delta;
		atomic_decl(&tsc_entry_barrier, 1);
		while (tsc_entry_barrier != 0) {
			;                                                
		}
		tsc_starter = rdtsc64();
		atomic_decl(&tsc_exit_barrier, 1);
		while (tsc_exit_barrier != 0) {
			;                                             
		}
		tsc_delta = tsc_target - tsc_starter;
		kprintf("TSC sync for cpu %d: 0x%016llx delta 0x%llx (%lld)\n",
		    psip->target_cpu, tsc_target, tsc_delta, tsc_delta);
#if DEBUG || DEVELOPMENT
		  
                                                          
                                                  
     
		cpu_datap(psip->target_cpu)->tsc_sync_delta = tsc_delta;
#endif
		if (ABS(tsc_delta) > (int64_t) TSC_sync_margin) {
#if DEBUG
			panic(
#else
			kprintf(
#endif
				"Unsynchronized  TSC for cpu %d: "
				"0x%016llx, delta 0x%llx\n",
				psip->target_cpu, tsc_target, tsc_delta);
		}
	}
}

kern_return_t
intel_startCPU(
	int     slot_num)
{
	int             lapic = cpu_to_lapic[slot_num];
	boolean_t       istate;

	assert(lapic != -1);

	DBGLOG_CPU_INIT(slot_num);

	DBG("intel_startCPU(%d) lapic_id=%d\n", slot_num, lapic);
	DBG("IdlePTD(%p): 0x%x\n", &IdlePTD, (int) (uintptr_t)IdlePTD);

	  
                                                                     
                                      
    
	cpu_desc_init(cpu_datap(slot_num));

	                                                 
	lck_mtx_lock(&mp_cpu_boot_lock);

	istate = ml_set_interrupts_enabled(FALSE);
	if (slot_num == get_cpu_number()) {
		ml_set_interrupts_enabled(istate);
		lck_mtx_unlock(&mp_cpu_boot_lock);
		return KERN_SUCCESS;
	}

	start_info.starter_cpu  = cpu_number();
	start_info.target_cpu   = slot_num;
	start_info.target_lapic = lapic;
	tsc_entry_barrier = 2;
	tsc_exit_barrier = 2;

	  
                                                           
                                                                  
                                                             
    
	mp_rendezvous_no_intrs(start_cpu, (void *) &start_info);

	start_info.target_cpu = 0;

	ml_set_interrupts_enabled(istate);
	lck_mtx_unlock(&mp_cpu_boot_lock);

	if (!cpu_datap(slot_num)->cpu_running) {
		kprintf("Failed to start CPU %02d\n", slot_num);
		printf("Failed to start CPU %02d, rebooting...\n", slot_num);
		delay(1000000);
		halt_cpu();
		return KERN_SUCCESS;
	} else {
		kprintf("Started cpu %d (lapic id %08x)\n", slot_num, lapic);
		return KERN_SUCCESS;
	}
}

#if     MP_DEBUG
cpu_signal_event_log_t  *cpu_signal[MAX_CPUS];
cpu_signal_event_log_t  *cpu_handle[MAX_CPUS];

MP_EVENT_NAME_DECL();

#endif                

  
                                                                           
   
int
cpu_signal_handler(x86_saved_state_t *regs)
{
#if     !MACH_KDP
#pragma unused (regs)
#endif                
	int             my_cpu;
	volatile int    *my_word;

	SCHED_STATS_INC(ipi_count);

	my_cpu = cpu_number();
	my_word = &cpu_data_ptr[my_cpu]->cpu_signals;
	                                                        
                                                        
                                
    

	cpu_data_ptr[my_cpu]->cpu_prior_signals = *my_word;

	do {
#if     MACH_KDP
		if (i_bit(MP_KDP, my_word)) {
			DBGLOG(cpu_handle, my_cpu, MP_KDP);
			i_bit_clear(MP_KDP, my_word);
                                                       
                                                           
                                                        
                               
   
			sync_iss_to_iks(regs);
			if (pmsafe_debug && !kdp_snapshot) {
				pmSafeMode(&current_cpu_datap()->lcpu, PM_SAFE_FL_SAFE);
			}
			mp_kdp_wait(TRUE, FALSE);
			if (pmsafe_debug && !kdp_snapshot) {
				pmSafeMode(&current_cpu_datap()->lcpu, PM_SAFE_FL_NORMAL);
			}
		} else
#endif                
		if (i_bit(MP_TLB_FLUSH, my_word)) {
			DBGLOG(cpu_handle, my_cpu, MP_TLB_FLUSH);
			i_bit_clear(MP_TLB_FLUSH, my_word);
			pmap_update_interrupt();
		} else if (i_bit(MP_CALL, my_word)) {
			DBGLOG(cpu_handle, my_cpu, MP_CALL);
			i_bit_clear(MP_CALL, my_word);
			mp_cpus_call_action();
		} else if (i_bit(MP_CALL_PM, my_word)) {
			DBGLOG(cpu_handle, my_cpu, MP_CALL_PM);
			i_bit_clear(MP_CALL_PM, my_word);
			mp_call_PM();
		}
		if (regs == NULL) {
			                                                       
			break;
		} else if (i_bit(MP_AST, my_word)) {
			DBGLOG(cpu_handle, my_cpu, MP_AST);
			i_bit_clear(MP_AST, my_word);
			ast_check(cpu_to_processor(my_cpu));
		}
	} while (*my_word);

	return 0;
}

extern void kprintf_break_lock(void);
int
NMIInterruptHandler(x86_saved_state_t *regs)
{
	void            *stackptr;
	char            pstr[256];
	uint64_t        now = mach_absolute_time();

	if (panic_active() && !panicDebugging) {
		if (pmsafe_debug) {
			pmSafeMode(&current_cpu_datap()->lcpu, PM_SAFE_FL_SAFE);
		}
		for (;;) {
			cpu_pause();
		}
	}

	atomic_incl(&NMIPI_acks, 1);
	atomic_incl(&NMI_count, 1);
	sync_iss_to_iks_unconditionally(regs);
	__asm__ volatile ("movq %%rbp, %0" : "=m" (stackptr));

	if (cpu_number() == debugger_cpu) {
		goto NMExit;
	}

	if (NMI_panic_reason == SPINLOCK_TIMEOUT) {
		snprintf(&pstr[0], sizeof(pstr),
		    "Panic(CPU %d, time %llu): NMIPI for spinlock acquisition timeout, spinlock: %p, spinlock owner: %p, current_thread: %p, spinlock_owner_cpu: 0x%x\n",
		    cpu_number(), now, spinlock_timed_out, (void *) spinlock_timed_out->interlock.lock_data, current_thread(), spinlock_owner_cpu);
		panic_i386_backtrace(stackptr, 64, &pstr[0], TRUE, regs);
	} else if (NMI_panic_reason == TLB_FLUSH_TIMEOUT) {
		snprintf(&pstr[0], sizeof(pstr),
		    "Panic(CPU %d, time %llu): NMIPI for unresponsive processor: TLB flush timeout, TLB state:0x%x\n",
		    cpu_number(), now, current_cpu_datap()->cpu_tlb_invalid);
		panic_i386_backtrace(stackptr, 48, &pstr[0], TRUE, regs);
	} else if (NMI_panic_reason == CROSSCALL_TIMEOUT) {
		snprintf(&pstr[0], sizeof(pstr),
		    "Panic(CPU %d, time %llu): NMIPI for unresponsive processor: cross-call timeout\n",
		    cpu_number(), now);
		panic_i386_backtrace(stackptr, 64, &pstr[0], TRUE, regs);
	} else if (NMI_panic_reason == INTERRUPT_WATCHDOG) {
		snprintf(&pstr[0], sizeof(pstr),
		    "Panic(CPU %d, time %llu): NMIPI for unresponsive processor: interrupt watchdog for vector 0x%x\n",
		    cpu_number(), now, vector_timed_out);
		panic_i386_backtrace(stackptr, 64, &pstr[0], TRUE, regs);
	}

#if MACH_KDP
	if (pmsafe_debug && !kdp_snapshot) {
		pmSafeMode(&current_cpu_datap()->lcpu, PM_SAFE_FL_SAFE);
	}
	current_cpu_datap()->cpu_NMI_acknowledged = TRUE;
	i_bit_clear(MP_KDP, &current_cpu_datap()->cpu_signals);
	if (panic_active() || NMI_panic_reason != NONE) {
		mp_kdp_wait(FALSE, TRUE);
	} else if (!mp_kdp_trap &&
	    !mp_kdp_is_NMI &&
	    virtualized && (debug_boot_arg & DB_NMI)) {
		  
                                                            
                                                                
                                                                
                                                        
     
		if (__sync_bool_compare_and_swap(&mp_kdp_is_NMI, FALSE, TRUE)) {
			kprintf_break_lock();
			kprintf("Debugger entry requested by NMI\n");
			kdp_i386_trap(T_DEBUG, saved_state64(regs), 0, 0);
			printf("Debugger entry requested by NMI\n");
			mp_kdp_is_NMI = FALSE;
		} else {
			mp_kdp_wait(FALSE, FALSE);
		}
	} else {
		mp_kdp_wait(FALSE, FALSE);
	}
	if (pmsafe_debug && !kdp_snapshot) {
		pmSafeMode(&current_cpu_datap()->lcpu, PM_SAFE_FL_NORMAL);
	}
#endif
NMExit:
	return 1;
}


  
                                                              
                                                               
                                              
   
void
cpu_interrupt(int cpu)
{
	boolean_t did_IPI = FALSE;

	if (smp_initialized
	    && pmCPUExitIdle(cpu_datap(cpu))) {
		i386_cpu_IPI(cpu);
		did_IPI = TRUE;
	}

	KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_REMOTE_AST), cpu, did_IPI, 0, 0, 0);
}

  
                                                           
   
void
cpu_NMI_interrupt(int cpu)
{
	if (smp_initialized) {
		i386_send_NMI(cpu);
	}
}

void
NMI_cpus(void)
{
	unsigned int    cpu;
	boolean_t       intrs_enabled;
	uint64_t        tsc_timeout;

	intrs_enabled = ml_set_interrupts_enabled(FALSE);
	NMIPI_enable(TRUE);
	for (cpu = 0; cpu < real_ncpus; cpu++) {
		if (!cpu_is_running(cpu)) {
			continue;
		}
		cpu_datap(cpu)->cpu_NMI_acknowledged = FALSE;
		cpu_NMI_interrupt(cpu);
		tsc_timeout = !machine_timeout_suspended() ?
		    rdtsc64() + (1000 * 1000 * 1000 * 10ULL) :
		    ~0ULL;
		while (!cpu_datap(cpu)->cpu_NMI_acknowledged) {
			handle_pending_TLB_flushes();
			cpu_pause();
			if (rdtsc64() > tsc_timeout) {
				panic("NMI_cpus() timeout cpu %d", cpu);
			}
		}
		cpu_datap(cpu)->cpu_NMI_acknowledged = FALSE;
	}
	NMIPI_enable(FALSE);

	ml_set_interrupts_enabled(intrs_enabled);
}

static void(*volatile mp_PM_func)(void) = NULL;

static void
mp_call_PM(void)
{
	assert(!ml_get_interrupts_enabled());

	if (mp_PM_func != NULL) {
		mp_PM_func();
	}
}

void
cpu_PM_interrupt(int cpu)
{
	assert(!ml_get_interrupts_enabled());

	if (mp_PM_func != NULL) {
		if (cpu == cpu_number()) {
			mp_PM_func();
		} else {
			i386_signal_cpu(cpu, MP_CALL_PM, ASYNC);
		}
	}
}

void
PM_interrupt_register(void (*fn)(void))
{
	mp_PM_func = fn;
}

void
i386_signal_cpu(int cpu, mp_event_t event, mp_sync_t mode)
{
	volatile int    *signals = &cpu_datap(cpu)->cpu_signals;
	uint64_t        tsc_timeout;


	if (!cpu_datap(cpu)->cpu_running) {
		return;
	}

	if (event == MP_TLB_FLUSH) {
		KERNEL_DEBUG(TRACE_MP_TLB_FLUSH | DBG_FUNC_START, cpu, 0, 0, 0, 0);
	}

	DBGLOG(cpu_signal, cpu, event);

	i_bit_set(event, signals);
	i386_cpu_IPI(cpu);
	if (mode == SYNC) {
again:
		tsc_timeout = !machine_timeout_suspended() ?
		    rdtsc64() + (1000 * 1000 * 1000) :
		    ~0ULL;
		while (i_bit(event, signals) && rdtsc64() < tsc_timeout) {
			cpu_pause();
		}
		if (i_bit(event, signals)) {
			DBG("i386_signal_cpu(%d, 0x%x, SYNC) timed out\n",
			    cpu, event);
			goto again;
		}
	}
	if (event == MP_TLB_FLUSH) {
		KERNEL_DEBUG(TRACE_MP_TLB_FLUSH | DBG_FUNC_END, cpu, 0, 0, 0, 0);
	}
}

  
                                                              
                                                            
   
static boolean_t
mp_spin_timeout(uint64_t tsc_start)
{
	uint64_t        tsc_timeout;

	cpu_pause();
	if (machine_timeout_suspended()) {
		return FALSE;
	}

	  
                                                  
                                                            
                                                  
    
	tsc_timeout = disable_serial_output ? LockTimeOutTSC << 2
	        : LockTimeOutTSC << 4;
	return rdtsc64() > tsc_start + tsc_timeout;
}

  
                                                                       
                                                             
                                   
   
boolean_t
mp_safe_spin_lock(usimple_lock_t lock)
{
	if (ml_get_interrupts_enabled()) {
		simple_lock(lock, LCK_GRP_NULL);
		return TRUE;
	} else {
		uint64_t tsc_spin_start = rdtsc64();
		while (!simple_lock_try(lock, LCK_GRP_NULL)) {
			cpu_signal_handler(NULL);
			if (mp_spin_timeout(tsc_spin_start)) {
				uint32_t lock_cpu;
				uintptr_t lowner = (uintptr_t)
				    lock->interlock.lock_data;
				spinlock_timed_out = lock;
				lock_cpu = spinlock_timeout_NMI(lowner);
				NMIPI_panic(cpu_to_cpumask(lock_cpu), SPINLOCK_TIMEOUT);
				panic("mp_safe_spin_lock() timed out, lock: %p, owner thread: 0x%lx, current_thread: %p, owner on CPU 0x%x, time: %llu",
				    lock, lowner, current_thread(), lock_cpu, mach_absolute_time());
			}
		}
		return FALSE;
	}
}

  
                      
                             
                                                   
                                                
                                                    
                      
                                                           
            
  
                                                                          
                                                                    
   

static void
mp_rendezvous_action(__unused void *null)
{
	boolean_t       intrs_enabled;
	uint64_t        tsc_spin_start;

	  
                                                                      
                                                          
                           
    
	current_cpu_datap()->cpu_rendezvous_in_progress = TRUE;

	                    
	if (mp_rv_setup_func != NULL) {
		mp_rv_setup_func(mp_rv_func_arg);
	}

	intrs_enabled = ml_get_interrupts_enabled();

	                              
	atomic_incl(&mp_rv_entry, 1);
	tsc_spin_start = rdtsc64();

	while (mp_rv_entry < mp_rv_ncpus) {
		                                                       
		if (!intrs_enabled) {
			handle_pending_TLB_flushes();
		}
		if (mp_spin_timeout(tsc_spin_start)) {
			panic("mp_rv_action() entry: %ld of %d responses, start: 0x%llx, cur: 0x%llx", mp_rv_entry, mp_rv_ncpus, tsc_spin_start, rdtsc64());
		}
	}

	                     
	if (mp_rv_action_func != NULL) {
		mp_rv_action_func(mp_rv_func_arg);
	}

	                             
	atomic_incl(&mp_rv_exit, 1);
	tsc_spin_start = rdtsc64();
	while (mp_rv_exit < mp_rv_ncpus) {
		if (!intrs_enabled) {
			handle_pending_TLB_flushes();
		}
		if (mp_spin_timeout(tsc_spin_start)) {
			panic("mp_rv_action() exit: %ld of %d responses, start: 0x%llx, cur: 0x%llx", mp_rv_exit, mp_rv_ncpus, tsc_spin_start, rdtsc64());
		}
	}

	                       
	if (mp_rv_teardown_func != NULL) {
		mp_rv_teardown_func(mp_rv_func_arg);
	}

	current_cpu_datap()->cpu_rendezvous_in_progress = FALSE;

	                           
	atomic_incl(&mp_rv_complete, 1);
}

void
mp_rendezvous(void (*setup_func)(void *),
    void (*action_func)(void *),
    void (*teardown_func)(void *),
    void *arg)
{
	uint64_t        tsc_spin_start;

	if (!smp_initialized) {
		if (setup_func != NULL) {
			setup_func(arg);
		}
		if (action_func != NULL) {
			action_func(arg);
		}
		if (teardown_func != NULL) {
			teardown_func(arg);
		}
		return;
	}

	                            
	mp_rendezvous_lock();

	                                  
	mp_rv_setup_func = setup_func;
	mp_rv_action_func = action_func;
	mp_rv_teardown_func = teardown_func;
	mp_rv_func_arg = arg;

	mp_rv_entry    = 0;
	mp_rv_exit     = 0;
	mp_rv_complete = 0;

	  
                                                                   
                            
    
	mp_rv_ncpus = mp_cpus_call(CPUMASK_OTHERS, NOSYNC, &mp_rendezvous_action, NULL) + 1;

	                                        
	mp_rendezvous_action(NULL);

	  
                                  
                                                                  
                                                                     
    
	tsc_spin_start = rdtsc64();
	while (mp_rv_complete < mp_rv_ncpus) {
		if (mp_spin_timeout(tsc_spin_start)) {
			panic("mp_rendezvous() timeout: %ld of %d responses, start: 0x%llx, cur: 0x%llx", mp_rv_complete, mp_rv_ncpus, tsc_spin_start, rdtsc64());
		}
	}

	             
	mp_rv_setup_func = NULL;
	mp_rv_action_func = NULL;
	mp_rv_teardown_func = NULL;
	mp_rv_func_arg = NULL;

	                  
	mp_rendezvous_unlock();
}

void
mp_rendezvous_lock(void)
{
	(void) mp_safe_spin_lock(&mp_rv_lock);
}

void
mp_rendezvous_unlock(void)
{
	simple_unlock(&mp_rv_lock);
}

void
mp_rendezvous_break_lock(void)
{
	simple_lock_init(&mp_rv_lock, 0);
}

static void
setup_disable_intrs(__unused void * param_not_used)
{
	                                                 
	boolean_t intr = ml_set_interrupts_enabled(FALSE);

	current_cpu_datap()->cpu_iflag = intr;
	DBG("CPU%d: %s\n", get_cpu_number(), __FUNCTION__);
}

static void
teardown_restore_intrs(__unused void * param_not_used)
{
	                                                   
	ml_set_interrupts_enabled(current_cpu_datap()->cpu_iflag);
	DBG("CPU%d: %s\n", get_cpu_number(), __FUNCTION__);
}

  
                                                                               
                                     
   
void
mp_rendezvous_no_intrs(
	void (*action_func)(void *),
	void *arg)
{
	mp_rendezvous(setup_disable_intrs,
	    action_func,
	    teardown_restore_intrs,
	    arg);
}


typedef struct {
	queue_chain_t   link;                                      
	void            (*func)(void *, void *);                      
	void            *arg0;                                         
	void            *arg1;                                         
	cpumask_t       *maskp;                                               
} mp_call_t;


typedef struct {
	queue_head_t            queue;
	decl_simple_lock_data(, lock);
} mp_call_queue_t;
#define MP_CPUS_CALL_BUFS_PER_CPU       MAX_CPUS
static mp_call_queue_t  mp_cpus_call_freelist;
static mp_call_queue_t  mp_cpus_call_head[MAX_CPUS];

static inline boolean_t
mp_call_head_lock(mp_call_queue_t *cqp)
{
	boolean_t       intrs_enabled;

	intrs_enabled = ml_set_interrupts_enabled(FALSE);
	simple_lock(&cqp->lock, LCK_GRP_NULL);

	return intrs_enabled;
}

  
                                                                   
   
void
NMIPI_panic(cpumask_t cpu_mask, NMI_reason_t why)
{
	unsigned int cpu;
	cpumask_t cpu_bit;
	uint64_t deadline;

	NMIPI_enable(TRUE);
	NMI_panic_reason = why;

	for (cpu = 0, cpu_bit = 1; cpu < real_ncpus; cpu++, cpu_bit <<= 1) {
		if ((cpu_mask & cpu_bit) == 0) {
			continue;
		}
		cpu_datap(cpu)->cpu_NMI_acknowledged = FALSE;
		cpu_NMI_interrupt(cpu);
	}

	                                                    
	deadline = mach_absolute_time() + LockTimeOut;
	for (cpu = 0, cpu_bit = 1; cpu < real_ncpus; cpu++, cpu_bit <<= 1) {
		if ((cpu_mask & cpu_bit) == 0) {
			continue;
		}
		while (!cpu_datap(cpu)->cpu_NMI_acknowledged &&
		    mach_absolute_time() < deadline) {
			cpu_pause();
		}
	}
}

#if MACH_ASSERT
static inline boolean_t
mp_call_head_is_locked(mp_call_queue_t *cqp)
{
	return !ml_get_interrupts_enabled() &&
	       hw_lock_held((hw_lock_t)&cqp->lock);
}
#endif

static inline void
mp_call_head_unlock(mp_call_queue_t *cqp, boolean_t intrs_enabled)
{
	simple_unlock(&cqp->lock);
	ml_set_interrupts_enabled(intrs_enabled);
}

static inline mp_call_t *
mp_call_alloc(void)
{
	mp_call_t       *callp = NULL;
	boolean_t       intrs_enabled;
	mp_call_queue_t *cqp = &mp_cpus_call_freelist;

	intrs_enabled = mp_call_head_lock(cqp);
	if (!queue_empty(&cqp->queue)) {
		queue_remove_first(&cqp->queue, callp, typeof(callp), link);
	}
	mp_call_head_unlock(cqp, intrs_enabled);

	return callp;
}

static inline void
mp_call_free(mp_call_t *callp)
{
	boolean_t       intrs_enabled;
	mp_call_queue_t *cqp = &mp_cpus_call_freelist;

	intrs_enabled = mp_call_head_lock(cqp);
	queue_enter_first(&cqp->queue, callp, typeof(callp), link);
	mp_call_head_unlock(cqp, intrs_enabled);
}

static inline mp_call_t *
mp_call_dequeue_locked(mp_call_queue_t *cqp)
{
	mp_call_t       *callp = NULL;

	assert(mp_call_head_is_locked(cqp));
	if (!queue_empty(&cqp->queue)) {
		queue_remove_first(&cqp->queue, callp, typeof(callp), link);
	}
	return callp;
}

static inline void
mp_call_enqueue_locked(
	mp_call_queue_t *cqp,
	mp_call_t       *callp)
{
	queue_enter(&cqp->queue, callp, typeof(callp), link);
}

                                                                  
static void
mp_cpus_call_init(void)
{
	mp_call_queue_t *cqp = &mp_cpus_call_freelist;

	DBG("mp_cpus_call_init()\n");
	simple_lock_init(&cqp->lock, 0);
	queue_init(&cqp->queue);
}

  
                                                                        
                                            
   
void
mp_cpus_call_cpu_init(int cpu)
{
	int             i;
	mp_call_queue_t *cqp = &mp_cpus_call_head[cpu];
	mp_call_t       *callp;

	simple_lock_init(&cqp->lock, 0);
	queue_init(&cqp->queue);
	for (i = 0; i < MP_CPUS_CALL_BUFS_PER_CPU; i++) {
		callp = zalloc_permanent_type(mp_call_t);
		mp_call_free(callp);
	}

	DBG("mp_cpus_call_init(%d) done\n", cpu);
}

  
                                                                         
                                                                         
   
static void
mp_cpus_call_action(void)
{
	mp_call_queue_t *cqp;
	boolean_t       intrs_enabled;
	mp_call_t       *callp;
	mp_call_t       call;

	assert(!ml_get_interrupts_enabled());
	cqp = &mp_cpus_call_head[cpu_number()];
	intrs_enabled = mp_call_head_lock(cqp);
	while ((callp = mp_call_dequeue_locked(cqp)) != NULL) {
		                                                   
		call = *callp;
		mp_call_free(callp);
		if (call.func != NULL) {
			mp_call_head_unlock(cqp, intrs_enabled);
			KERNEL_DEBUG_CONSTANT(
				TRACE_MP_CPUS_CALL_ACTION,
				VM_KERNEL_UNSLIDE(call.func), VM_KERNEL_UNSLIDE_OR_PERM(call.arg0),
				VM_KERNEL_UNSLIDE_OR_PERM(call.arg1), VM_KERNEL_ADDRPERM(call.maskp), 0);
			call.func(call.arg0, call.arg1);
			(void) mp_call_head_lock(cqp);
		}
		if (call.maskp != NULL) {
			i_bit_set(cpu_number(), call.maskp);
		}
	}
	mp_call_head_unlock(cqp, intrs_enabled);
}

  
                                                                              
                      
                                                                           
                                                                 
                                                         
                                                                     
                                     
                                                             
                                   
                                                                    
                                                                               
   
cpu_t
mp_cpus_call(
	cpumask_t       cpus,
	mp_sync_t       mode,
	void            (*action_func)(void *),
	void            *arg)
{
	return mp_cpus_call1(
		cpus,
		mode,
		(void (*)(void *, void *))action_func,
		arg,
		NULL,
		NULL);
}

static void
mp_cpus_call_wait(boolean_t     intrs_enabled,
    cpumask_t     cpus_called,
    cpumask_t     *cpus_responded)
{
	mp_call_queue_t         *cqp;
	uint64_t                tsc_spin_start;

	assert(ml_get_interrupts_enabled() == 0 || get_preemption_level() != 0);
	cqp = &mp_cpus_call_head[cpu_number()];

	tsc_spin_start = rdtsc64();
	while (*cpus_responded != cpus_called) {
		if (!intrs_enabled) {
			                          
			if (!queue_empty(&cqp->queue)) {
				mp_cpus_call_action();
			}
			cpu_signal_handler(NULL);
		}
		if (mp_spin_timeout(tsc_spin_start)) {
			cpumask_t       cpus_unresponsive;

			cpus_unresponsive = cpus_called & ~(*cpus_responded);
			NMIPI_panic(cpus_unresponsive, CROSSCALL_TIMEOUT);
			panic("mp_cpus_call_wait() timeout, cpus: 0x%llx",
			    cpus_unresponsive);
		}
	}
}

cpu_t
mp_cpus_call1(
	cpumask_t       cpus,
	mp_sync_t       mode,
	void            (*action_func)(void *, void *),
	void            *arg0,
	void            *arg1,
	cpumask_t       *cpus_calledp)
{
	cpu_t           cpu = 0;
	boolean_t       intrs_enabled = FALSE;
	boolean_t       call_self = FALSE;
	cpumask_t       cpus_called = 0;
	cpumask_t       cpus_responded = 0;
	long            cpus_call_count = 0;
	uint64_t        tsc_spin_start;
	boolean_t       topo_lock;

	KERNEL_DEBUG_CONSTANT(
		TRACE_MP_CPUS_CALL | DBG_FUNC_START,
		cpus, mode, VM_KERNEL_UNSLIDE(action_func), VM_KERNEL_UNSLIDE_OR_PERM(arg0), VM_KERNEL_UNSLIDE_OR_PERM(arg1));

	if (!smp_initialized) {
		if ((cpus & CPUMASK_SELF) == 0) {
			goto out;
		}
		if (action_func != NULL) {
			intrs_enabled = ml_set_interrupts_enabled(FALSE);
			action_func(arg0, arg1);
			ml_set_interrupts_enabled(intrs_enabled);
		}
		call_self = TRUE;
		goto out;
	}

	  
                                                    
                                                               
                                                               
                                                                  
                                                                    
                                                                        
    
	mp_disable_preemption();
	intrs_enabled = ml_get_interrupts_enabled();
	topo_lock = (cpus != cpu_to_cpumask(master_cpu));
	if (topo_lock) {
		ml_set_interrupts_enabled(FALSE);
		(void) mp_safe_spin_lock(&x86_topo_lock);
	}
	for (cpu = 0; cpu < (cpu_t) real_ncpus; cpu++) {
		if (((cpu_to_cpumask(cpu) & cpus) == 0) ||
		    !cpu_is_running(cpu)) {
			continue;
		}
		tsc_spin_start = rdtsc64();
		if (cpu == (cpu_t) cpu_number()) {
			  
                                                         
                                                           
      
			call_self = TRUE;
			if (mode == SYNC && action_func != NULL) {
				KERNEL_DEBUG_CONSTANT(
					TRACE_MP_CPUS_CALL_LOCAL,
					VM_KERNEL_UNSLIDE(action_func),
					VM_KERNEL_UNSLIDE_OR_PERM(arg0), VM_KERNEL_UNSLIDE_OR_PERM(arg1), 0, 0);
				action_func(arg0, arg1);
			}
		} else {
			  
                                          
      
			mp_call_t       *callp = NULL;
			mp_call_queue_t *cqp = &mp_cpus_call_head[cpu];
			boolean_t       intrs_inner;

queue_call:
			if (callp == NULL) {
				callp = mp_call_alloc();
			}
			intrs_inner = mp_call_head_lock(cqp);
			if (callp == NULL) {
				mp_call_head_unlock(cqp, intrs_inner);
				KERNEL_DEBUG_CONSTANT(
					TRACE_MP_CPUS_CALL_NOBUF,
					cpu, 0, 0, 0, 0);
				if (!intrs_inner) {
					                          
					if (!queue_empty(&cqp->queue)) {
						mp_cpus_call_action();
					}
					handle_pending_TLB_flushes();
				}
				if (mp_spin_timeout(tsc_spin_start)) {
					panic("mp_cpus_call1() timeout start: 0x%llx, cur: 0x%llx",
					    tsc_spin_start, rdtsc64());
				}
				goto queue_call;
			}
			callp->maskp = (mode == NOSYNC) ? NULL : &cpus_responded;
			callp->func = action_func;
			callp->arg0 = arg0;
			callp->arg1 = arg1;
			mp_call_enqueue_locked(cqp, callp);
			cpus_call_count++;
			cpus_called |= cpu_to_cpumask(cpu);
			i386_signal_cpu(cpu, MP_CALL, ASYNC);
			mp_call_head_unlock(cqp, intrs_inner);
			if (mode == SYNC) {
				mp_cpus_call_wait(intrs_inner, cpus_called, &cpus_responded);
			}
		}
	}
	if (topo_lock) {
		simple_unlock(&x86_topo_lock);
		ml_set_interrupts_enabled(intrs_enabled);
	}

	                                   
	if (mode != SYNC && call_self) {
		KERNEL_DEBUG_CONSTANT(
			TRACE_MP_CPUS_CALL_LOCAL,
			VM_KERNEL_UNSLIDE(action_func), VM_KERNEL_UNSLIDE_OR_PERM(arg0), VM_KERNEL_UNSLIDE_OR_PERM(arg1), 0, 0);
		if (action_func != NULL) {
			ml_set_interrupts_enabled(FALSE);
			action_func(arg0, arg1);
			ml_set_interrupts_enabled(intrs_enabled);
		}
	}

	                                                                       
	if (mode == ASYNC) {
		mp_cpus_call_wait(intrs_enabled, cpus_called, &cpus_responded);
	}

	                                   
	mp_enable_preemption();

out:
	if (call_self) {
		cpus_called |= cpu_to_cpumask(cpu);
		cpus_call_count++;
	}

	if (cpus_calledp) {
		*cpus_calledp = cpus_called;
	}

	KERNEL_DEBUG_CONSTANT(
		TRACE_MP_CPUS_CALL | DBG_FUNC_END,
		cpus_call_count, cpus_called, 0, 0, 0);

	return (cpu_t) cpus_call_count;
}


static void
mp_broadcast_action(__unused void *null)
{
	                          
	if (mp_bc_action_func != NULL) {
		mp_bc_action_func(mp_bc_func_arg);
	}

	                                                           
	if (atomic_decl_and_test(&mp_bc_count, 1)) {
		thread_wakeup(((event_t)(uintptr_t) &mp_bc_count));
	}
}

  
                                                           
                                                             
                                                                    
   
void
mp_broadcast(
	void (*action_func)(void *),
	void *arg)
{
	if (!smp_initialized) {
		if (action_func != NULL) {
			action_func(arg);
		}
		return;
	}

	                           
	lck_mtx_lock(&mp_bc_lock);

	                                  
	mp_bc_action_func = action_func;
	mp_bc_func_arg = arg;

	assert_wait((event_t)(uintptr_t)&mp_bc_count, THREAD_UNINT);

	  
                                                                  
    
	mp_bc_count = real_ncpus;                                                       
	mp_bc_ncpus = mp_cpus_call(CPUMASK_ALL, NOSYNC, *mp_broadcast_action, NULL);
	atomic_decl(&mp_bc_count, real_ncpus - mp_bc_ncpus);                        

	                                                  
	if (mp_bc_ncpus > 1) {
		thread_block(THREAD_CONTINUE_NULL);
	} else {
		clear_wait(current_thread(), THREAD_AWAKENED);
	}

	                  
	lck_mtx_unlock(&mp_bc_lock);
}

void
mp_cpus_kick(cpumask_t cpus)
{
	cpu_t           cpu;
	boolean_t       intrs_enabled = FALSE;

	intrs_enabled = ml_set_interrupts_enabled(FALSE);
	mp_safe_spin_lock(&x86_topo_lock);

	for (cpu = 0; cpu < (cpu_t) real_ncpus; cpu++) {
		if (((cpu_to_cpumask(cpu) & cpus) == 0)
		    || !cpu_is_running(cpu)) {
			continue;
		}

		lapic_send_ipi(cpu, LAPIC_VECTOR(KICK));
	}

	simple_unlock(&x86_topo_lock);
	ml_set_interrupts_enabled(intrs_enabled);
}

void
i386_activate_cpu(void)
{
	cpu_data_t      *cdp = current_cpu_datap();

	assert(!ml_get_interrupts_enabled());

	if (!smp_initialized) {
		cdp->cpu_running = TRUE;
		return;
	}

	mp_safe_spin_lock(&x86_topo_lock);
	cdp->cpu_running = TRUE;
	started_cpu();
	pmap_tlbi_range(0, ~0ULL, true, 0);
	simple_unlock(&x86_topo_lock);
}

void
i386_deactivate_cpu(void)
{
	cpu_data_t      *cdp = current_cpu_datap();

	assert(!ml_get_interrupts_enabled());

	KERNEL_DEBUG_CONSTANT(
		TRACE_MP_CPU_DEACTIVATE | DBG_FUNC_START,
		0, 0, 0, 0, 0);

	mp_safe_spin_lock(&x86_topo_lock);
	cdp->cpu_running = FALSE;
	simple_unlock(&x86_topo_lock);

	  
                                                         
                                                                     
    
	timer_queue_shutdown(&cdp->rtclock_timer.queue);
	mp_cpus_call(cpu_to_cpumask(master_cpu), ASYNC, timer_queue_expire_local, NULL);

#if MONOTONIC
	mt_cpu_down(cdp);
#endif                

	  
                            
                                                   
    
	mp_disable_preemption();
	ml_set_interrupts_enabled(TRUE);

	while (cdp->cpu_signals && x86_lcpu()->rtcDeadline != EndOfAllTime) {
		cpu_pause();
	}
	  
                                                  
                                     
    
	setPop(0);

	ml_set_interrupts_enabled(FALSE);
	mp_enable_preemption();

	KERNEL_DEBUG_CONSTANT(
		TRACE_MP_CPU_DEACTIVATE | DBG_FUNC_END,
		0, 0, 0, 0, 0);
}

int     pmsafe_debug    = 1;

#if     MACH_KDP
volatile boolean_t      mp_kdp_trap = FALSE;
volatile boolean_t      mp_kdp_is_NMI = FALSE;
volatile unsigned long  mp_kdp_ncpus;
boolean_t               mp_kdp_state;


void
mp_kdp_enter(boolean_t proceed_on_failure)
{
	unsigned int    cpu;
	unsigned int    ncpus = 0;
	unsigned int    my_cpu;
	uint64_t        tsc_timeout;

	DBG("mp_kdp_enter()\n");

	  
                               
                                                                
                    
    
	mp_kdp_state = ml_set_interrupts_enabled(FALSE);
	my_cpu = cpu_number();

	if (my_cpu == (unsigned) debugger_cpu) {
		kprintf("\n\nRECURSIVE DEBUGGER ENTRY DETECTED\n\n");
		kdp_reset();
		return;
	}

	uint64_t start_time = cpu_datap(my_cpu)->debugger_entry_time = mach_absolute_time();
	int locked = 0;
	while (!locked || mp_kdp_trap) {
		if (locked) {
			simple_unlock(&x86_topo_lock);
		}
		if (proceed_on_failure) {
			if (mach_absolute_time() - start_time > 500000000ll) {
				paniclog_append_noflush("mp_kdp_enter() can't get x86_topo_lock! Debugging anyway! #YOLO\n");
				break;
			}
			locked = simple_lock_try(&x86_topo_lock, LCK_GRP_NULL);
			if (!locked) {
				cpu_pause();
			}
		} else {
			mp_safe_spin_lock(&x86_topo_lock);
			locked = TRUE;
		}

		if (locked && mp_kdp_trap) {
			simple_unlock(&x86_topo_lock);
			DBG("mp_kdp_enter() race lost\n");
#if MACH_KDP
			mp_kdp_wait(TRUE, FALSE);
#endif
			locked = FALSE;
		}
	}

	if (pmsafe_debug && !kdp_snapshot) {
		pmSafeMode(&current_cpu_datap()->lcpu, PM_SAFE_FL_SAFE);
	}

	debugger_cpu = my_cpu;
	ncpus = 1;
	atomic_incl((volatile long *)&mp_kdp_ncpus, 1);
	mp_kdp_trap = TRUE;
	debugger_entry_time = cpu_datap(my_cpu)->debugger_entry_time;

	  
                                                    
    
	DBG("mp_kdp_enter() signaling other processors\n");
	if (force_immediate_debugger_NMI == FALSE) {
		for (cpu = 0; cpu < real_ncpus; cpu++) {
			if (cpu == my_cpu || !cpu_is_running(cpu)) {
				continue;
			}
			ncpus++;
			i386_signal_cpu(cpu, MP_KDP, ASYNC);
		}
		  
                                         
     
		DBG("mp_kdp_enter() waiting for (%d) processors to suspend\n", ncpus);

		  
                                                           
                                                 
                                                          
                                                              
     
		tsc_timeout = rdtsc64() + (LockTimeOutTSC);

		while (mp_kdp_ncpus != ncpus && rdtsc64() < tsc_timeout) {
			  
                                                        
                                                   
                                                            
                                                   
      
			handle_pending_TLB_flushes();
			cpu_pause();
		}
		                                                                    
                                                                  
                 
     
		if (panic_active()) {
			NMIPI_enable(TRUE);
		}
		if (mp_kdp_ncpus != ncpus) {
			unsigned int wait_cycles = 0;
			if (proceed_on_failure) {
				paniclog_append_noflush("mp_kdp_enter() timed-out on cpu %d, NMI-ing\n", my_cpu);
			} else {
				DBG("mp_kdp_enter() timed-out on cpu %d, NMI-ing\n", my_cpu);
			}
			for (cpu = 0; cpu < real_ncpus; cpu++) {
				if (cpu == my_cpu || !cpu_is_running(cpu)) {
					continue;
				}
				if (cpu_signal_pending(cpu, MP_KDP)) {
					cpu_datap(cpu)->cpu_NMI_acknowledged = FALSE;
					cpu_NMI_interrupt(cpu);
				}
			}
			                                     
			tsc_timeout = rdtsc64() + (LockTimeOutTSC);
			while (mp_kdp_ncpus != ncpus && rdtsc64() < tsc_timeout) {
				handle_pending_TLB_flushes();
				cpu_pause();
				++wait_cycles;
			}
			if (mp_kdp_ncpus != ncpus) {
				paniclog_append_noflush("mp_kdp_enter() NMI pending on cpus:");
				for (cpu = 0; cpu < real_ncpus; cpu++) {
					if (cpu_is_running(cpu) && !cpu_datap(cpu)->cpu_NMI_acknowledged) {
						paniclog_append_noflush(" %d", cpu);
					}
				}
				paniclog_append_noflush("\n");
				if (proceed_on_failure) {
					paniclog_append_noflush("mp_kdp_enter() timed-out during %s wait after NMI;"
					    "expected %u acks but received %lu after %u loops in %llu ticks\n",
					    (locked ? "locked" : "unlocked"), ncpus, mp_kdp_ncpus, wait_cycles, LockTimeOutTSC);
				} else {
					panic("mp_kdp_enter() timed-out during %s wait after NMI;"
					    "expected %u acks but received %lu after %u loops in %llu ticks",
					    (locked ? "locked" : "unlocked"), ncpus, mp_kdp_ncpus, wait_cycles, LockTimeOutTSC);
				}
			}
		}
	} else {
		for (cpu = 0; cpu < real_ncpus; cpu++) {
			if (cpu == my_cpu || !cpu_is_running(cpu)) {
				continue;
			}
			cpu_NMI_interrupt(cpu);
		}
	}

	if (locked) {
		simple_unlock(&x86_topo_lock);
	}

	DBG("mp_kdp_enter() %d processors done %s\n",
	    (int)mp_kdp_ncpus, (mp_kdp_ncpus == ncpus) ? "OK" : "timed out");

	postcode(MP_KDP_ENTER);
}

boolean_t
mp_kdp_all_cpus_halted()
{
	unsigned int ncpus = 0, cpu = 0, my_cpu = 0;

	my_cpu = cpu_number();
	ncpus = 1;                  
	for (cpu = 0; cpu < real_ncpus; cpu++) {
		if (cpu == my_cpu || !cpu_is_running(cpu)) {
			continue;
		}
		ncpus++;
	}

	return mp_kdp_ncpus == ncpus;
}

static boolean_t
cpu_signal_pending(int cpu, mp_event_t event)
{
	volatile int    *signals = &cpu_datap(cpu)->cpu_signals;
	boolean_t retval = FALSE;

	if (i_bit(event, signals)) {
		retval = TRUE;
	}
	return retval;
}

long
kdp_x86_xcpu_invoke(const uint16_t lcpu, kdp_x86_xcpu_func_t func,
    void *arg0, void *arg1)
{
	if (lcpu > (real_ncpus - 1)) {
		return -1;
	}

	if (func == NULL) {
		return -1;
	}

	kdp_xcpu_call_func.func = func;
	kdp_xcpu_call_func.ret  = -1;
	kdp_xcpu_call_func.arg0 = arg0;
	kdp_xcpu_call_func.arg1 = arg1;
	kdp_xcpu_call_func.cpu  = lcpu;
	DBG("Invoking function %p on CPU %d\n", func, (int32_t)lcpu);
	while (kdp_xcpu_call_func.cpu != KDP_XCPU_NONE) {
		cpu_pause();
	}
	return kdp_xcpu_call_func.ret;
}

static void
kdp_x86_xcpu_poll(void)
{
	if ((uint16_t)cpu_number() == kdp_xcpu_call_func.cpu) {
		kdp_xcpu_call_func.ret =
		    kdp_xcpu_call_func.func(kdp_xcpu_call_func.arg0,
		    kdp_xcpu_call_func.arg1,
		    cpu_number());
		kdp_xcpu_call_func.cpu = KDP_XCPU_NONE;
	}
}

static void
mp_kdp_wait(boolean_t flush, boolean_t isNMI)
{
	DBG("mp_kdp_wait()\n");

	current_cpu_datap()->debugger_ipi_time = mach_absolute_time();
#if CONFIG_MCA
	                                                                 
	mca_check_save();
#endif

	atomic_incl((volatile long *)&mp_kdp_ncpus, 1);
	while (mp_kdp_trap || (isNMI == TRUE)) {
		  
                                                              
                                                              
                                     
                                                  
     
		if (flush) {
			handle_pending_TLB_flushes();
		}

		kdp_x86_xcpu_poll();
		cpu_pause();
	}

	atomic_decl((volatile long *)&mp_kdp_ncpus, 1);
	DBG("mp_kdp_wait() done\n");
}

void
mp_kdp_exit(void)
{
	DBG("mp_kdp_exit()\n");
	debugger_cpu = -1;
	atomic_decl((volatile long *)&mp_kdp_ncpus, 1);

	debugger_exit_time = mach_absolute_time();

	mp_kdp_trap = FALSE;
	mfence();

	                                                               
	DBG("mp_kdp_exit() waiting for processors to resume\n");
	while (mp_kdp_ncpus > 0) {
		  
                                                                                  
                                                                                
                                                  
     
		handle_pending_TLB_flushes();

		cpu_pause();
	}

	if (pmsafe_debug && !kdp_snapshot) {
		pmSafeMode(&current_cpu_datap()->lcpu, PM_SAFE_FL_NORMAL);
	}

	debugger_exit_time = mach_absolute_time();

	DBG("mp_kdp_exit() done\n");
	(void) ml_set_interrupts_enabled(mp_kdp_state);
	postcode(MP_KDP_EXIT);
}

#endif                

boolean_t
mp_recent_debugger_activity(void)
{
	uint64_t abstime = mach_absolute_time();
	return ((abstime - debugger_entry_time) < LastDebuggerEntryAllowance) ||
	       ((abstime - debugger_exit_time) < LastDebuggerEntryAllowance);
}

            
void
init_ast_check(
	__unused processor_t    processor)
{
}

void
cause_ast_check(
	processor_t     processor)
{
	int     cpu = processor->cpu_id;

	if (cpu != cpu_number()) {
		i386_signal_cpu(cpu, MP_AST, ASYNC);
		KERNEL_DEBUG_CONSTANT(MACHDBG_CODE(DBG_MACH_SCHED, MACH_REMOTE_AST), cpu, 1, 0, 0, 0);
	}
}

void
slave_machine_init(void *param)
{
	  
                                                          
    
	DBG("slave_machine_init() CPU%d\n", get_cpu_number());

	if (param == FULL_SLAVE_INIT) {
		  
               
     
		clock_init();
	}
	cpu_machine_init();                                       
}

#undef cpu_number
int
cpu_number(void)
{
	return get_cpu_number();
}

vm_offset_t
current_percpu_base(void)
{
	return get_current_percpu_base();
}

static void
cpu_prewarm_init()
{
	int i;

	simple_lock_init(&cpu_warm_lock, 0);
	queue_init(&cpu_warm_call_list);
	for (i = 0; i < NUM_CPU_WARM_CALLS; i++) {
		enqueue_head(&cpu_warm_call_list, (queue_entry_t)&cpu_warm_call_arr[i]);
	}
}

static timer_call_t
grab_warm_timer_call()
{
	spl_t x;
	timer_call_t call = NULL;

	x = splsched();
	simple_lock(&cpu_warm_lock, LCK_GRP_NULL);
	if (!queue_empty(&cpu_warm_call_list)) {
		call = (timer_call_t) dequeue_head(&cpu_warm_call_list);
	}
	simple_unlock(&cpu_warm_lock);
	splx(x);

	return call;
}

static void
free_warm_timer_call(timer_call_t call)
{
	spl_t x;

	x = splsched();
	simple_lock(&cpu_warm_lock, LCK_GRP_NULL);
	enqueue_head(&cpu_warm_call_list, (queue_entry_t)call);
	simple_unlock(&cpu_warm_lock);
	splx(x);
}

  
                                                    
   
static void
cpu_warm_timer_call_func(
	timer_call_param_t p0,
	__unused timer_call_param_t p1)
{
	free_warm_timer_call((timer_call_t)p0);
	return;
}

  
                                                                         
   
static void
_cpu_warm_setup(
	void *arg)
{
	cpu_warm_data_t cwdp = (cpu_warm_data_t)arg;

	timer_call_enter(cwdp->cwd_call, cwdp->cwd_deadline, TIMER_CALL_SYS_CRITICAL | TIMER_CALL_LOCAL);
	cwdp->cwd_result = 0;

	return;
}

  
                                             
   
kern_return_t
ml_interrupt_prewarm(
	uint64_t        deadline)
{
	struct cpu_warm_data cwd;
	timer_call_t call;
	cpu_t ct;

	if (ml_get_interrupts_enabled() == FALSE) {
		panic("%s: Interrupts disabled?\n", __FUNCTION__);
	}

	  
                                                                 
    
	if (!ml_get_interrupt_prewake_applicable()) {
		return KERN_SUCCESS;
	}

	  
                             
    
	call = grab_warm_timer_call();
	if (call == NULL) {
		return KERN_RESOURCE_SHORTAGE;
	}

	timer_call_setup(call, cpu_warm_timer_call_func, call);
	cwd.cwd_call = call;
	cwd.cwd_deadline = deadline;
	cwd.cwd_result = 0;

	  
                                                                 
    
	ct = mp_cpus_call(cpu_to_cpumask(master_cpu), SYNC, _cpu_warm_setup, &cwd);
	if (ct == 0) {
		free_warm_timer_call(call);
		return KERN_FAILURE;
	} else {
		return cwd.cwd_result;
	}
}

#if DEBUG || DEVELOPMENT
void
kernel_spin(uint64_t spin_ns)
{
	boolean_t       istate;
	uint64_t        spin_abs;
	uint64_t        deadline;
	cpu_data_t      *cdp;

	kprintf("kernel_spin(%llu) spinning uninterruptibly\n", spin_ns);
	istate = ml_set_interrupts_enabled(FALSE);
	cdp = current_cpu_datap();
	nanoseconds_to_absolutetime(spin_ns, &spin_abs);

	                                                                      
	cdp->cpu_int_event_time = mach_absolute_time();
	cdp->cpu_int_state = (void *) USER_STATE(current_thread());

	deadline = mach_absolute_time() + spin_ns;
	while (mach_absolute_time() < deadline) {
		cpu_pause();
	}

	cdp->cpu_int_event_time = 0;
	cdp->cpu_int_state = NULL;

	ml_set_interrupts_enabled(istate);
	kprintf("kernel_spin() continuing\n");
}

  
                                                  
                                                     
                                          
                                 
   
void
mp_interrupt_watchdog(void)
{
	cpu_t                   cpu;
	boolean_t               intrs_enabled = FALSE;
	uint16_t                cpu_int_num;
	uint64_t                cpu_int_event_time;
	uint64_t                cpu_rip;
	uint64_t                cpu_int_duration;
	uint64_t                now;
	x86_saved_state_t       *cpu_int_state;

	if (__improbable(!mp_interrupt_watchdog_enabled)) {
		return;
	}

	intrs_enabled = ml_set_interrupts_enabled(FALSE);
	now = mach_absolute_time();
	  
                                     
                                                                       
    
	for (cpu = 0;
	    cpu < (cpu_t) real_ncpus && !machine_timeout_suspended();
	    cpu++) {
		if ((cpu == (cpu_t) cpu_number()) ||
		    (!cpu_is_running(cpu))) {
			continue;
		}
		cpu_int_event_time = cpu_datap(cpu)->cpu_int_event_time;
		if (cpu_int_event_time == 0) {
			continue;
		}
		if (__improbable(now < cpu_int_event_time)) {
			continue;                                             
		}
		cpu_int_state = cpu_datap(cpu)->cpu_int_state;
		if (__improbable(cpu_int_state == NULL)) {
			                                           
			continue;
		}

		                                           

		cpu_int_duration = now - cpu_int_event_time;
		if (__improbable(cpu_int_duration > LockTimeOut)) {
			cpu_int_num = saved_state64(cpu_int_state)->isf.trapno;
			cpu_rip = saved_state64(cpu_int_state)->isf.rip;
			vector_timed_out = cpu_int_num;
			NMIPI_panic(cpu_to_cpumask(cpu), INTERRUPT_WATCHDOG);
			panic("Interrupt watchdog, "
			    "cpu: %d interrupt: 0x%x time: %llu..%llu state: %p RIP: 0x%llx",
			    cpu, cpu_int_num, cpu_int_event_time, now, cpu_int_state, cpu_rip);
			                 
		} else if (__improbable(cpu_int_duration > (uint64_t) std_quantum)) {
			mp_interrupt_watchdog_events++;
			cpu_int_num = saved_state64(cpu_int_state)->isf.trapno;
			cpu_rip = saved_state64(cpu_int_state)->isf.rip;
			ml_set_interrupts_enabled(intrs_enabled);
			printf("Interrupt watchdog, "
			    "cpu: %d interrupt: 0x%x time: %llu..%llu RIP: 0x%llx\n",
			    cpu, cpu_int_num, cpu_int_event_time, now, cpu_rip);
			return;
		}
	}

	ml_set_interrupts_enabled(intrs_enabled);
}
#endif
