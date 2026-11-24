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
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
  
   

  
                               
   

#include <mach_kdp.h>
#include <mach_ldebug.h>

#include <types.h>
#include <i386/eflags.h>
#include <i386/trap.h>
#include <i386/pmap.h>
#include <i386/fpu.h>
#include <i386/panic_notify.h>
#include <i386/lapic.h>

#include <mach/exception.h>
#include <mach/kern_return.h>
#include <mach/vm_param.h>
#include <mach/i386/thread_status.h>

#include <vm/vm_kern.h>
#include <vm/vm_fault.h>

#include <kern/kern_types.h>
#include <kern/processor.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <kern/sched.h>
#include <kern/sched_prim.h>
#include <kern/exception.h>
#include <kern/spl.h>
#include <kern/misc_protos.h>
#include <kern/debug.h>
#if CONFIG_TELEMETRY
#include <kern/telemetry.h>
#endif
#include <sys/kdebug.h>
#include <kperf/kperf.h>
#include <prng/random.h>
#include <prng/entropy.h>

#include <string.h>

#include <i386/postcode.h>
#include <i386/mp_desc.h>
#include <i386/proc_reg.h>
#include <i386/machine_routines.h>
#if CONFIG_MCA
#include <i386/machine_check.h>
#endif
#include <mach/i386/syscall_sw.h>

#include <libkern/OSDebug.h>
#include <i386/cpu_threads.h>
#include <machine/pal_routines.h>

extern void throttle_lowpri_io(int);
extern void kprint_state(x86_saved_state64_t *saved_state);
#if DEVELOPMENT || DEBUG
int insnstream_force_cacheline_mismatch = 0;
extern int panic_on_cacheline_mismatch;
extern char panic_on_trap_procname[];
extern uint32_t panic_on_trap_mask;
#endif

extern int insn_copyin_count;

  
                       
   
static void panic_trap(x86_saved_state64_t *saved_state, uint32_t pl, kern_return_t fault_result) __dead2;
static void set_recovery_ip(x86_saved_state64_t *saved_state, vm_offset_t ip);
#if DEVELOPMENT || DEBUG
static __attribute__((noinline)) void copy_instruction_stream(thread_t thread, uint64_t rip, int trap_code, bool inspect_cacheline);
#else
static __attribute__((noinline)) void copy_instruction_stream(thread_t thread, uint64_t rip, int trap_code);
#endif

#if CONFIG_DTRACE
                                  
perfCallback tempDTraceTrapHook = NULL;                                              

extern boolean_t dtrace_tally_fault(user_addr_t);
extern boolean_t dtrace_handle_trap(int, x86_saved_state_t *);
#endif

#ifdef MACH_BSD
extern char *   proc_name_address(void *p);
#endif               

extern boolean_t pmap_smep_enabled;
extern boolean_t pmap_smap_enabled;

__attribute__((noreturn))
void
thread_syscall_return(
	kern_return_t ret)
{
	thread_t        thr_act = current_thread();
	boolean_t       is_mach;
	int             code;

	pal_register_cache_state(thr_act, DIRTY);

	if (thread_is_64bit_addr(thr_act)) {
		x86_saved_state64_t     *regs;

		regs = USER_REGS64(thr_act);

		code = (int) (regs->rax & SYSCALL_NUMBER_MASK);
		is_mach = (regs->rax & SYSCALL_CLASS_MASK)
		    == (SYSCALL_CLASS_MACH << SYSCALL_CLASS_SHIFT);
		if (kdebug_enable && is_mach) {
			               
			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    MACHDBG_CODE(DBG_MACH_EXCP_SC, code) | DBG_FUNC_END,
			    ret, 0, 0, 0, 0);
		}
		regs->rax = ret;
#if DEBUG
		if (is_mach) {
			DEBUG_KPRINT_SYSCALL_MACH(
				"thread_syscall_return: 64-bit mach ret=%u\n",
				ret);
		} else {
			DEBUG_KPRINT_SYSCALL_UNIX(
				"thread_syscall_return: 64-bit unix ret=%u\n",
				ret);
		}
#endif
	} else {
		x86_saved_state32_t     *regs;

		regs = USER_REGS32(thr_act);

		code = ((int) regs->eax);
		is_mach = (code < 0);
		if (kdebug_enable && is_mach) {
			               
			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    MACHDBG_CODE(DBG_MACH_EXCP_SC, -code) | DBG_FUNC_END,
			    ret, 0, 0, 0, 0);
		}
		regs->eax = ret;
#if DEBUG
		if (is_mach) {
			DEBUG_KPRINT_SYSCALL_MACH(
				"thread_syscall_return: 32-bit mach ret=%u\n",
				ret);
		} else {
			DEBUG_KPRINT_SYSCALL_UNIX(
				"thread_syscall_return: 32-bit unix ret=%u\n",
				ret);
		}
#endif
	}

#if DEBUG || DEVELOPMENT
	kern_allocation_name_t
	prior __assert_only = thread_get_kernel_state(thr_act)->allocation_name;
	assertf(prior == NULL, "thread_set_allocation_name(\"%s\") not cleared", kern_allocation_get_name(prior));
#endif                           

	throttle_lowpri_io(1);

	thread_exception_return();
	              
}

  
                                             
   
struct recovery {
	uintptr_t       fault_addr;
	uintptr_t       recover_addr;
};

extern struct recovery  recover_table[];
extern struct recovery  recover_table_end[];

const char *    trap_type[] = {TRAP_NAMES};
unsigned        TRAP_TYPES = sizeof(trap_type) / sizeof(trap_type[0]);

extern void     PE_incoming_interrupt(int interrupt);

#if defined(__x86_64__) && DEBUG
void
kprint_state(x86_saved_state64_t        *saved_state)
{
	kprintf("current_cpu_datap() 0x%lx\n", (uintptr_t)current_cpu_datap());
	kprintf("Current GS base MSR 0x%llx\n", rdmsr64(MSR_IA32_GS_BASE));
	kprintf("Kernel  GS base MSR 0x%llx\n", rdmsr64(MSR_IA32_KERNEL_GS_BASE));
	kprintf("state at 0x%lx:\n", (uintptr_t) saved_state);

	kprintf("      rdi    0x%llx\n", saved_state->rdi);
	kprintf("      rsi    0x%llx\n", saved_state->rsi);
	kprintf("      rdx    0x%llx\n", saved_state->rdx);
	kprintf("      r10    0x%llx\n", saved_state->r10);
	kprintf("      r8     0x%llx\n", saved_state->r8);
	kprintf("      r9     0x%llx\n", saved_state->r9);

	kprintf("      cr2    0x%llx\n", saved_state->cr2);
	kprintf("real  cr2    0x%lx\n", get_cr2());
	kprintf("      r15    0x%llx\n", saved_state->r15);
	kprintf("      r14    0x%llx\n", saved_state->r14);
	kprintf("      r13    0x%llx\n", saved_state->r13);
	kprintf("      r12    0x%llx\n", saved_state->r12);
	kprintf("      r11    0x%llx\n", saved_state->r11);
	kprintf("      rbp    0x%llx\n", saved_state->rbp);
	kprintf("      rbx    0x%llx\n", saved_state->rbx);
	kprintf("      rcx    0x%llx\n", saved_state->rcx);
	kprintf("      rax    0x%llx\n", saved_state->rax);

	kprintf("      gs     0x%x\n", saved_state->gs);
	kprintf("      fs     0x%x\n", saved_state->fs);

	kprintf("  isf.trapno 0x%x\n", saved_state->isf.trapno);
	kprintf("  isf._pad   0x%x\n", saved_state->isf._pad);
	kprintf("  isf.trapfn 0x%llx\n", saved_state->isf.trapfn);
	kprintf("  isf.err    0x%llx\n", saved_state->isf.err);
	kprintf("  isf.rip    0x%llx\n", saved_state->isf.rip);
	kprintf("  isf.cs     0x%llx\n", saved_state->isf.cs);
	kprintf("  isf.rflags 0x%llx\n", saved_state->isf.rflags);
	kprintf("  isf.rsp    0x%llx\n", saved_state->isf.rsp);
	kprintf("  isf.ss     0x%llx\n", saved_state->isf.ss);
}
#endif


  
                                                                    
                       
   

uint64_t interrupt_latency_cap = 0;
boolean_t ilat_assert = FALSE;

void
interrupt_latency_tracker_setup(void)
{
	uint32_t ilat_cap_us;
	if (PE_parse_boot_argn("interrupt_latency_cap_us", &ilat_cap_us, sizeof(ilat_cap_us))) {
		interrupt_latency_cap = ilat_cap_us * NSEC_PER_USEC;
		nanoseconds_to_absolutetime(interrupt_latency_cap, &interrupt_latency_cap);
	} else {
		interrupt_latency_cap = LockTimeOut;
	}
	PE_parse_boot_argn("-interrupt_latency_assert_enable", &ilat_assert, sizeof(ilat_assert));
}

void
interrupt_reset_latency_stats(void)
{
	uint32_t i;
	for (i = 0; i < real_ncpus; i++) {
		cpu_data_ptr[i]->cpu_max_observed_int_latency =
		    cpu_data_ptr[i]->cpu_max_observed_int_latency_vector = 0;
	}
}

void
interrupt_populate_latency_stats(char *buf, unsigned bufsize)
{
	uint32_t i, tcpu = ~0;
	uint64_t cur_max = 0;

	for (i = 0; i < real_ncpus; i++) {
		if (cur_max < cpu_data_ptr[i]->cpu_max_observed_int_latency) {
			cur_max = cpu_data_ptr[i]->cpu_max_observed_int_latency;
			tcpu = i;
		}
	}

	if (tcpu < real_ncpus) {
		snprintf(buf, bufsize, "0x%x 0x%x 0x%llx", tcpu, cpu_data_ptr[tcpu]->cpu_max_observed_int_latency_vector, cpu_data_ptr[tcpu]->cpu_max_observed_int_latency);
	}
}

uint32_t interrupt_timer_coalescing_enabled = 1;
uint64_t interrupt_coalesced_timers;

  
                     
                                                                          
                                                  
   
void
interrupt(x86_saved_state_t *state)
{
	uint64_t        rip;
	uint64_t        rsp;
	int             interrupt_num;
	boolean_t       user_mode = FALSE;
	int             ipl;
	int             cnum = cpu_number();
	cpu_data_t      *cdp = cpu_data_ptr[cnum];
	int             itype = DBG_INTR_TYPE_UNKNOWN;
	int             handled;

	x86_saved_state64_t     *state64 = saved_state64(state);
	rip = state64->isf.rip;
	rsp = state64->isf.rsp;
	interrupt_num = state64->isf.trapno;
	if (state64->isf.cs & 0x03) {
		user_mode = TRUE;
	}

#if DEVELOPMENT || DEBUG
	uint64_t frameptr = is_saved_state64(state) ? state64->rbp : saved_state32(state)->ebp;
	uint32_t traptrace_index = traptrace_start(interrupt_num, rip, mach_absolute_time(), frameptr);
#endif

	if (cpu_data_ptr[cnum]->lcpu.package->num_idle == topoParms.nLThreadsPerPackage) {
		cpu_data_ptr[cnum]->cpu_hwIntpexits[interrupt_num]++;
	}

	if (interrupt_num == (LAPIC_DEFAULT_INTERRUPT_BASE + LAPIC_INTERPROCESSOR_INTERRUPT)) {
		itype = DBG_INTR_TYPE_IPI;
	} else if (interrupt_num == (LAPIC_DEFAULT_INTERRUPT_BASE + LAPIC_TIMER_INTERRUPT)) {
		itype = DBG_INTR_TYPE_TIMER;
	} else {
		itype = DBG_INTR_TYPE_OTHER;
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    MACHDBG_CODE(DBG_MACH_EXCP_INTR, 0) | DBG_FUNC_START,
	    interrupt_num,
	    (user_mode ? rip : VM_KERNEL_UNSLIDE(rip)),
	    user_mode, itype, 0);

	SCHED_STATS_INC(interrupt_count);

#if CONFIG_TELEMETRY
	if (telemetry_needs_record) {
		telemetry_mark_curthread(user_mode, FALSE);
	}
#endif

	ipl = get_preemption_level();

	  
                                
                                          
    
	handled = lapic_interrupt(interrupt_num, state);

	if (!handled) {
		if (interrupt_num == (LAPIC_DEFAULT_INTERRUPT_BASE + LAPIC_CMCI_INTERRUPT)) {
			  
                                                                   
                                                                     
                                                               
                                                                     
                        
      
			PE_incoming_interrupt(interrupt_num);
		} else if (cnum <= lapic_max_interrupt_cpunum) {
			PE_incoming_interrupt((cnum << 8) | interrupt_num);
		}
	}

	if (__improbable(get_preemption_level() != ipl)) {
		panic("Preemption level altered by interrupt vector 0x%x: initial 0x%x, final: 0x%x\n", interrupt_num, ipl, get_preemption_level());
	}


	if (__improbable(cdp->cpu_nested_istack)) {
		cdp->cpu_nested_istack_events++;
	} else {
		uint64_t ctime = mach_absolute_time();
		uint64_t int_latency = ctime - cdp->cpu_int_event_time;
		uint64_t esdeadline, ehdeadline;
		                                                       
                                                               
                          
     
#define TCOAL_ILAT_THRESHOLD (30000ULL)

		if ((int_latency < TCOAL_ILAT_THRESHOLD) &&
		    interrupt_timer_coalescing_enabled) {
			esdeadline = cdp->rtclock_timer.queue.earliest_soft_deadline;
			ehdeadline = cdp->rtclock_timer.deadline;
			if ((ctime >= esdeadline) && (ctime < ehdeadline)) {
				interrupt_coalesced_timers++;
				TCOAL_DEBUG(0x88880000 | DBG_FUNC_START, ctime, esdeadline, ehdeadline, interrupt_coalesced_timers, 0);
				rtclock_intr(state);
				TCOAL_DEBUG(0x88880000 | DBG_FUNC_END, ctime, esdeadline, interrupt_coalesced_timers, 0, 0);
			} else {
				TCOAL_DEBUG(0x77770000, ctime, cdp->rtclock_timer.queue.earliest_soft_deadline, cdp->rtclock_timer.deadline, interrupt_coalesced_timers, 0);
			}
		}

		if (__improbable(ilat_assert && (int_latency > interrupt_latency_cap) && !machine_timeout_suspended())) {
			panic("Interrupt vector 0x%x exceeded interrupt latency threshold, 0x%llx absolute time delta, prior signals: 0x%x, current signals: 0x%x", interrupt_num, int_latency, cdp->cpu_prior_signals, cdp->cpu_signals);
		}

		if (__improbable(int_latency > cdp->cpu_max_observed_int_latency)) {
			cdp->cpu_max_observed_int_latency = int_latency;
			cdp->cpu_max_observed_int_latency_vector = interrupt_num;
		}
	}

	  
                                                                             
    
	if (!user_mode) {
		uint64_t depth = cdp->cpu_kernel_stack
		    + sizeof(struct thread_kernel_state)
		    + sizeof(struct i386_exception_link *)
		    - rsp;
		if (__improbable(depth > kernel_stack_depth_max)) {
			kernel_stack_depth_max = (vm_offset_t)depth;
			KERNEL_DEBUG_CONSTANT(
				MACHDBG_CODE(DBG_MACH_SCHED, MACH_STACK_DEPTH),
				(long) depth, (long) VM_KERNEL_UNSLIDE(rip), 0, 0, 0);
		}
	}

	if (cnum == master_cpu) {
		entropy_collect();
	}

#if KPERF
	kperf_interrupt();
#endif            

	KDBG_RELEASE(MACHDBG_CODE(DBG_MACH_EXCP_INTR, 0) | DBG_FUNC_END,
	    interrupt_num);

	assert(ml_get_interrupts_enabled() == FALSE);

#if DEVELOPMENT || DEBUG
	if (traptrace_index != TRAPTRACE_INVALID_INDEX) {
		traptrace_end(traptrace_index, mach_absolute_time());
	}
#endif
}

static inline void
reset_dr7(void)
{
	long dr7 = 0x400;                                                              
	__asm__ volatile ("mov %0,%%dr7" : : "r" (dr7));
}
#if MACH_KDP
unsigned kdp_has_active_watchpoints = 0;
#define NO_WATCHPOINTS (!kdp_has_active_watchpoints)
#else
#define NO_WATCHPOINTS 1
#endif
  
                                                                  
                                                                
                                                      
   

void
kernel_trap(
	x86_saved_state_t       *state,
	uintptr_t *lo_spp)
{
	x86_saved_state64_t     *saved_state;
	int                     code;
	user_addr_t             vaddr;
	int                     type;
	vm_map_t                map = 0;                                       
	kern_return_t           result = KERN_FAILURE;
	kern_return_t           fault_result = KERN_SUCCESS;
	thread_t                thread;
	boolean_t               intr;
	vm_prot_t               prot;
	struct recovery         *rp;
	vm_offset_t             kern_ip;
	int                     is_user;
	int                     trap_pl = get_preemption_level();

	thread = current_thread();

	if (__improbable(is_saved_state32(state))) {
		panic("kernel_trap(%p) with 32-bit state", state);
	}
	saved_state = saved_state64(state);

	                                         
	saved_state->isf.cpu = cpu_number();

	vaddr = (user_addr_t)saved_state->cr2;
	type  = saved_state->isf.trapno;
	code  = (int)(saved_state->isf.err & 0xffff);
	intr  = (saved_state->isf.rflags & EFL_IF) != 0;                                   
	kern_ip = (vm_offset_t)saved_state->isf.rip;

	is_user = (vaddr < VM_MAX_USER_PAGE_ADDRESS);

#if DEVELOPMENT || DEBUG
	uint32_t traptrace_index = traptrace_start(type, kern_ip, mach_absolute_time(), saved_state->rbp);
#endif

#if CONFIG_DTRACE
	  
                           
    
	if (__improbable(tempDTraceTrapHook != NULL)) {
		if (tempDTraceTrapHook(type, state, lo_spp, 0) == KERN_SUCCESS) {
			  
                                    
      
			goto common_return;
		}
	}

	                                                 
	if (thread != THREAD_NULL && thread->t_dtrace_inprobe) {
		if (dtrace_handle_trap(type, state)) {
			goto common_return;
		}
	}

#endif                    

	  
                                                                
                                                                
                                                
    
	if (__improbable(T_PREEMPT == type)) {
		ast_taken_kernel();

		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (MACHDBG_CODE(DBG_MACH_EXCP_KTRAP_x86, type)) | DBG_FUNC_NONE,
		    0, 0, 0, VM_KERNEL_UNSLIDE(kern_ip), 0);

		goto common_return;
	}

	user_addr_t     kd_vaddr = is_user ? vaddr : VM_KERNEL_UNSLIDE(vaddr);
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (MACHDBG_CODE(DBG_MACH_EXCP_KTRAP_x86, type)) | DBG_FUNC_NONE,
	    (unsigned)(kd_vaddr >> 32), (unsigned)kd_vaddr, is_user,
	    VM_KERNEL_UNSLIDE(kern_ip), 0);


	if (T_PAGE_FAULT == type) {
		  
                                            
     
		map = kernel_map;

		if (__probable((thread != THREAD_NULL) && (thread->map != kernel_map) &&
		    (vaddr < VM_MAX_USER_PAGE_ADDRESS))) {
			                                 
			map = thread->map;

			                                                
                                               
                                              
                                               
                                                    
                     
      
			if (__improbable((code == (T_PF_PROT | T_PF_EXECUTE)) &&
			    (pmap_smep_enabled) && (saved_state->isf.rip == vaddr))) {
				goto debugger_entry;
			}

			  
                                           
                                                 
                                                       
      
			if (__improbable(code & T_PF_PROT &&
			    pmap_smap_enabled &&
			    (saved_state->isf.rflags & EFL_AC) == 0)) {
				goto debugger_entry;
			}

			  
                                            
                               
                                                 
      
			if (no_shared_cr3 &&
			    (thread->machine.specFlags & CopyIOActive) &&
			    map->pmap->pm_cr3 != get_cr3_base()) {
				pmap_assert(current_cpu_datap()->cpu_pmap_pcid_enabled == FALSE);
				set_cr3_raw(map->pmap->pm_cr3);
				return;
			}
			if (__improbable(vaddr < PAGE_SIZE) &&
			    ((thread->machine.specFlags & CopyIOActive) == 0)) {
				goto debugger_entry;
			}
		}
	}

	(void) ml_set_interrupts_enabled(intr);

	switch (type) {
	case T_NO_FPU:
		fpnoextflt();
		goto common_return;

	case T_FPU_FAULT:
		fpextovrflt();
		goto common_return;

	case T_FLOATING_POINT_ERROR:
		fpexterrflt();
		goto common_return;

	case T_SSE_FLOAT_ERROR:
		fpSSEexterrflt();
		goto common_return;

	case T_INVALID_OPCODE:
		fpUDflt(kern_ip);
		goto debugger_entry;

	case T_DEBUG:
		if ((saved_state->isf.rflags & EFL_TF) == 0 && NO_WATCHPOINTS) {
			                                    
                                         
                             
                                    
      
			reset_dr7();
			goto common_return;
		}
		goto debugger_entry;
	case T_INT3:
		goto debugger_entry;
	case T_PAGE_FAULT:

#if CONFIG_DTRACE
		if (thread != THREAD_NULL && thread->t_dtrace_inprobe) {                                    
			if (dtrace_tally_fault(vaddr)) {                                              
				  
                                                                      
                                                                      
                                                              
       
				goto FALL_THROUGH;
			}
		}
#endif                    

		prot = VM_PROT_READ;

		if (code & T_PF_WRITE) {
			prot |= VM_PROT_WRITE;
		}
		if (code & T_PF_EXECUTE) {
			prot |= VM_PROT_EXECUTE;
		}

		fault_result = result = vm_fault(map,
		    vaddr,
		    prot,
		    FALSE, VM_KERN_MEMORY_NONE,
		    THREAD_UNINT, NULL, 0);

		if (result == KERN_SUCCESS) {
			goto common_return;
		}
		  
                 
     
#if CONFIG_DTRACE
FALL_THROUGH:
#endif                    

	case T_GENERAL_PROTECTION:
		  
                                           
                              
     
		for (rp = recover_table; rp < recover_table_end; rp++) {
			if (kern_ip == rp->fault_addr) {
				set_recovery_ip(saved_state, rp->recover_addr);
				goto common_return;
			}
		}

		  
                                        
     
		if (thread != THREAD_NULL && thread->recover) {
			set_recovery_ip(saved_state, thread->recover);
			thread->recover = 0;
			goto common_return;
		}
		  
                                              
                       
    
                    
     
		OS_FALLTHROUGH;
	default:
		  
                                                            
                                                  
     
		if (type == 15) {
			kprintf("kernel_trap() ignoring spurious trap 15\n");
			goto common_return;
		}
debugger_entry:
		                                                       
                                                             
                                                     
                                 
     
		sync_iss_to_iks(state);
#if  MACH_KDP
		if (kdp_i386_trap(type, saved_state, result, (vm_offset_t)vaddr)) {
			goto common_return;
		}
#endif
	}
	pal_cli();
	panic_trap(saved_state, trap_pl, fault_result);
	  
             
    

common_return:
#if DEVELOPMENT || DEBUG
	if (traptrace_index != TRAPTRACE_INVALID_INDEX) {
		traptrace_end(traptrace_index, mach_absolute_time());
	}
#endif
	return;
}

static void
set_recovery_ip(x86_saved_state64_t  *saved_state, vm_offset_t ip)
{
	saved_state->isf.rip = ip;
}

static void
panic_trap(x86_saved_state64_t *regs, uint32_t pl, kern_return_t fault_result)
{
	const char      *trapname = "Unknown";
	pal_cr_t        cr0, cr2, cr3, cr4;
	boolean_t       potential_smep_fault = FALSE, potential_kernel_NX_fault = FALSE;
	boolean_t       potential_smap_fault = FALSE;

	pal_get_control_registers( &cr0, &cr2, &cr3, &cr4 );
	assert(ml_get_interrupts_enabled() == FALSE);
	current_cpu_datap()->cpu_fatal_trap_state = regs;
	  
                                                                 
                                                     
    
	panic_notify();

	kprintf("CPU %d panic trap number 0x%x, rip 0x%016llx\n",
	    cpu_number(), regs->isf.trapno, regs->isf.rip);
	kprintf("cr0 0x%016llx cr2 0x%016llx cr3 0x%016llx cr4 0x%016llx\n",
	    cr0, cr2, cr3, cr4);

	if (regs->isf.trapno < TRAP_TYPES) {
		trapname = trap_type[regs->isf.trapno];
	}

	if ((regs->isf.trapno == T_PAGE_FAULT) && (regs->isf.err == (T_PF_PROT | T_PF_EXECUTE)) && (regs->isf.rip == regs->cr2)) {
		if (pmap_smep_enabled && (regs->isf.rip < VM_MAX_USER_PAGE_ADDRESS)) {
			potential_smep_fault = TRUE;
		} else if (regs->isf.rip >= VM_MIN_KERNEL_AND_KEXT_ADDRESS) {
			potential_kernel_NX_fault = TRUE;
		}
	} else if (pmap_smap_enabled &&
	    regs->isf.trapno == T_PAGE_FAULT &&
	    regs->isf.err & T_PF_PROT &&
	    regs->cr2 < VM_MAX_USER_PAGE_ADDRESS &&
	    regs->isf.rip >= VM_MIN_KERNEL_AND_KEXT_ADDRESS) {
		potential_smap_fault = TRUE;
	}

#undef panic
	panic("Kernel trap at 0x%016llx, type %d=%s, registers:\n"
	    "CR0: 0x%016llx, CR2: 0x%016llx, CR3: 0x%016llx, CR4: 0x%016llx\n"
	    "RAX: 0x%016llx, RBX: 0x%016llx, RCX: 0x%016llx, RDX: 0x%016llx\n"
	    "RSP: 0x%016llx, RBP: 0x%016llx, RSI: 0x%016llx, RDI: 0x%016llx\n"
	    "R8:  0x%016llx, R9:  0x%016llx, R10: 0x%016llx, R11: 0x%016llx\n"
	    "R12: 0x%016llx, R13: 0x%016llx, R14: 0x%016llx, R15: 0x%016llx\n"
	    "RFL: 0x%016llx, RIP: 0x%016llx, CS:  0x%016llx, SS:  0x%016llx\n"
	    "Fault CR2: 0x%016llx, Error code: 0x%016llx, Fault CPU: 0x%x%s%s%s%s, PL: %d, VF: %d\n",
	    regs->isf.rip, regs->isf.trapno, trapname,
	    cr0, cr2, cr3, cr4,
	    regs->rax, regs->rbx, regs->rcx, regs->rdx,
	    regs->isf.rsp, regs->rbp, regs->rsi, regs->rdi,
	    regs->r8, regs->r9, regs->r10, regs->r11,
	    regs->r12, regs->r13, regs->r14, regs->r15,
	    regs->isf.rflags, regs->isf.rip, regs->isf.cs & 0xFFFF,
	    regs->isf.ss & 0xFFFF, regs->cr2, regs->isf.err, regs->isf.cpu,
	    virtualized ? " VMM" : "",
	    potential_kernel_NX_fault ? " Kernel NX fault" : "",
	    potential_smep_fault ? " SMEP/User NX fault" : "",
	    potential_smap_fault ? " SMAP fault" : "",
	    pl,
	    fault_result);
}

#if CONFIG_DTRACE
extern kern_return_t dtrace_user_probe(x86_saved_state_t *);
#endif

#if DEBUG
uint32_t fsigs[2];
uint32_t fsigns, fsigcs;
#endif

  
                       
   
void
user_trap(
	x86_saved_state_t *saved_state)
{
	int                     exc;
	int                     err;
	mach_exception_code_t   code;
	mach_exception_subcode_t subcode;
	int                     type;
	user_addr_t             vaddr;
	vm_prot_t               prot;
	thread_t                thread = current_thread();
	kern_return_t           kret;
	user_addr_t             rip;
	unsigned long           dr6 = 0;                                         
	int                     current_cpu = cpu_number();
#if DEVELOPMENT || DEBUG
	bool                    inspect_cacheline = false;
	uint32_t                traptrace_index;
#endif
	assert((is_saved_state32(saved_state) && !thread_is_64bit_addr(thread)) ||
	    (is_saved_state64(saved_state) && thread_is_64bit_addr(thread)));

	if (is_saved_state64(saved_state)) {
		x86_saved_state64_t     *regs;

		regs = saved_state64(saved_state);

		                                         
		regs->isf.cpu = current_cpu;

		type = regs->isf.trapno;
		err  = (int)regs->isf.err & 0xffff;
		vaddr = (user_addr_t)regs->cr2;
		rip   = (user_addr_t)regs->isf.rip;
#if DEVELOPMENT || DEBUG
		traptrace_index = traptrace_start(type, rip, mach_absolute_time(), regs->rbp);
#endif
	} else {
		x86_saved_state32_t     *regs;

		regs = saved_state32(saved_state);

		                                         
		regs->cpu = current_cpu;

		type  = regs->trapno;
		err   = regs->err & 0xffff;
		vaddr = (user_addr_t)regs->cr2;
		rip   = (user_addr_t)regs->eip;
#if DEVELOPMENT || DEBUG
		traptrace_index = traptrace_start(type, rip, mach_absolute_time(), regs->ebp);
#endif
	}

#if DEVELOPMENT || DEBUG
	  
                                                                             
                                                                                       
                                                                                   
                                                                           
    
	if (thread->machine.insn_copy_optout == false &&
	    (type == T_PAGE_FAULT || type == T_INVALID_OPCODE || type == T_GENERAL_PROTECTION)) {
#define CACHELINE_SIZE 64
		THREAD_TO_PCB(thread)->insn_cacheline[CACHELINE_SIZE] = (uint8_t)(rip & (CACHELINE_SIZE - 1));
		bcopy(&cpu_shadowp(current_cpu)->cpu_rtimes[0],
		    &THREAD_TO_PCB(thread)->insn_cacheline[0],
		    sizeof(THREAD_TO_PCB(thread)->insn_cacheline) - 1);
		inspect_cacheline = true;
	}
#endif

	if (type == T_DEBUG) {
		if (thread->machine.ids) {
			unsigned long clear = 0;
			                                                           
                                     
      
			__asm__ volatile ("mov %%db6, %0" : "=r" (dr6));
			__asm__ volatile ("mov %0, %%db6" : : "r" (clear));
		}
		                                                                                 
		i386_lbr_enable();
	}

	pal_sti();

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (MACHDBG_CODE(DBG_MACH_EXCP_UTRAP_x86, type)) | DBG_FUNC_NONE,
	    (unsigned)(vaddr >> 32), (unsigned)vaddr,
	    (unsigned)(rip >> 32), (unsigned)rip, 0);

	code = 0;
	subcode = 0;
	exc = 0;

#if CONFIG_DTRACE
	  
                                                                 
                                                                 
                           
    
#endif

	DEBUG_KPRINT_SYSCALL_MASK(1,
	    "user_trap: type=0x%x(%s) err=0x%x cr2=%p rip=%p\n",
	    type, trap_type[type], err, (void *)(long) vaddr, (void *)(long) rip);

	switch (type) {
	case T_DIVIDE_ERROR:
		exc = EXC_ARITHMETIC;
		code = EXC_I386_DIV;
		break;

	case T_DEBUG:
	{
		pcb_t   pcb;
		  
                                                   
                                                  
     
		pcb = THREAD_TO_PCB(thread);
		if (pcb->ids) {
			  
                                            
                                            
                                         
                    
      
			if (thread_is_64bit_addr(thread)) {
				x86_debug_state64_t *ids = pcb->ids;
				ids->dr6 = dr6;
			} else {                            
				x86_debug_state32_t *ids = pcb->ids;
				ids->dr6 = (uint32_t) dr6;
			}
		}
		exc = EXC_BREAKPOINT;
		code = EXC_I386_SGL;
		break;
	}
	case T_INT3:
#if CONFIG_DTRACE
		if (dtrace_user_probe(saved_state) == KERN_SUCCESS) {
			return;                                     
		}
#endif
		exc = EXC_BREAKPOINT;
		code = EXC_I386_BPT;
		break;

	case T_OVERFLOW:
		exc = EXC_ARITHMETIC;
		code = EXC_I386_INTO;
		break;

	case T_OUT_OF_BOUNDS:
		exc = EXC_SOFTWARE;
		code = EXC_I386_BOUND;
		break;

	case T_INVALID_OPCODE:
		if (fpUDflt(rip) == 1) {
			exc = EXC_BAD_INSTRUCTION;
			code = EXC_I386_INVOP;
		}
		break;

	case T_NO_FPU:
		fpnoextflt();
		break;

	case T_FPU_FAULT:
		fpextovrflt();
		  
                     
     
		exc = EXC_BAD_ACCESS;
		code = VM_PROT_READ | VM_PROT_EXECUTE;
		subcode = 0;
		break;

	case T_INVALID_TSS:                                               
		exc = EXC_BAD_INSTRUCTION;
		code = EXC_I386_INVTSSFLT;
		subcode = err;
		break;

	case T_SEGMENT_NOT_PRESENT:
		exc = EXC_BAD_INSTRUCTION;
		code = EXC_I386_SEGNPFLT;
		subcode = err;
		break;

	case T_STACK_FAULT:
		exc = EXC_BAD_INSTRUCTION;
		code = EXC_I386_STKFLT;
		subcode = err;
		break;

	case T_GENERAL_PROTECTION:
		  
                                                              
                                                          
                                                        
                                                            
                                                             
                                                           
                                                   
                                                            
                                                              
                                                              
                                                              
         
     
		exc = EXC_BAD_ACCESS;
		code = EXC_I386_GPFLT;
		subcode = err;
		break;

	case T_PAGE_FAULT:
	{
		prot = VM_PROT_READ;

		if (err & T_PF_WRITE) {
			prot |= VM_PROT_WRITE;
		}
		if (__improbable(err & T_PF_EXECUTE)) {
			prot |= VM_PROT_EXECUTE;
		}
#if DEVELOPMENT || DEBUG
		uint32_t fsig = 0;
		fsig = thread_fpsimd_hash(thread);
#if DEBUG
		fsigs[0] = fsig;
#endif
#endif
		kret = vm_fault(thread->map,
		    vaddr,
		    prot, FALSE, VM_KERN_MEMORY_NONE,
		    THREAD_ABORTSAFE, NULL, 0);
#if DEVELOPMENT || DEBUG
		if (fsig) {
			uint32_t fsig2 = thread_fpsimd_hash(thread);
#if DEBUG
			fsigcs++;
			fsigs[1] = fsig2;
#endif
			if (fsig != fsig2) {
				panic("FP/SIMD state hash mismatch across fault thread: %p 0x%x->0x%x", thread, fsig, fsig2);
			}
		} else {
#if DEBUG
			fsigns++;
#endif
		}
#endif
		if (__probable((kret == KERN_SUCCESS) || (kret == KERN_ABORTED))) {
			break;
		} else if (__improbable(kret == KERN_FAILURE)) {
			  
                                                                   
                                                                            
      
			panic("vm_fault() KERN_FAILURE from user fault on thread %p", thread);
		}

		                                   
		pal_dbg_page_fault(thread, vaddr, kret);
		exc = EXC_BAD_ACCESS;
		code = kret;
		subcode = vaddr;
	}
	break;

	case T_SSE_FLOAT_ERROR:
		fpSSEexterrflt();
		exc = EXC_ARITHMETIC;
		code = EXC_I386_SSEEXTERR;
		subcode = ((struct x86_fx_thread_state *)thread->machine.ifps)->fx_MXCSR;
		break;


	case T_FLOATING_POINT_ERROR:
		fpexterrflt();
		exc = EXC_ARITHMETIC;
		code = EXC_I386_EXTERR;
		subcode = ((struct x86_fx_thread_state *)thread->machine.ifps)->fx_status;
		break;

	case T_DTRACE_RET:
#if CONFIG_DTRACE
		if (dtrace_user_probe(saved_state) == KERN_SUCCESS) {
			return;                                     
		}
#endif
		  
                                                    
                                       
     
		exc = EXC_BAD_INSTRUCTION;
		code = EXC_I386_INVOP;
		break;

	default:
		panic("Unexpected user trap, type %d", type);
	}

	if (exc != 0) {
		uint16_t cs;
		boolean_t intrs;

		if (is_saved_state64(saved_state)) {
			cs = saved_state64(saved_state)->isf.cs;
		} else {
			cs = saved_state32(saved_state)->cs;
		}

		if (last_branch_support_enabled) {
			intrs = ml_set_interrupts_enabled(FALSE);
			  
                                                                                       
                                                                                      
                                                                          
      
			if (current_cpu == cpu_number()) {
				i386_lbr_synch(thread);
			}
			ml_set_interrupts_enabled(intrs);
		}

		  
                                                                               
                                           
                                                   
                                                                                 
                  
     
		if (thread->machine.insn_copy_optout == false && insn_copyin_count > 0 &&
		    (cs == USER64_CS || cs == USER_CS) && (type != T_PAGE_FAULT || vaddr != rip)) {
#if DEVELOPMENT || DEBUG
			copy_instruction_stream(thread, rip, type, inspect_cacheline);
#else
			copy_instruction_stream(thread, rip, type);
#endif
		}

#if DEVELOPMENT || DEBUG
		if (traptrace_index != TRAPTRACE_INVALID_INDEX) {
			traptrace_end(traptrace_index, mach_absolute_time());
		}
#endif
		  
                                                                       
                             
     
		i386_exception(exc, code, subcode);
		                
	} else {
#if DEVELOPMENT || DEBUG
		if (traptrace_index != TRAPTRACE_INVALID_INDEX) {
			traptrace_end(traptrace_index, mach_absolute_time());
		}
#endif
	}
}

  
                                                                                             
                                                                                
                                                                               
                                                               
   
static __attribute__((noinline)) void
copy_instruction_stream(thread_t thread, uint64_t rip, int __unused trap_code
#if DEVELOPMENT || DEBUG
    , bool inspect_cacheline
#endif
    )
{
#if x86_INSTRUCTION_STATE_MAX_INSN_BYTES > 4096
#error x86_INSTRUCTION_STATE_MAX_INSN_BYTES cannot exceed a page in size.
#endif
	pcb_t pcb = THREAD_TO_PCB(thread);
	vm_map_offset_t pagemask = ~vm_map_page_mask(current_map());
	vm_map_offset_t rip_page = rip & pagemask;
	vm_map_offset_t start_addr;
	vm_map_offset_t insn_offset;
	vm_map_offset_t end_addr = rip + (insn_copyin_count / 2);
	void *stack_buffer;
	int copyin_err = 0;
#if defined(MACH_BSD) && (DEVELOPMENT || DEBUG)
	void *procname;
#endif

#if DEVELOPMENT || DEBUG
	assert(insn_copyin_count <= x86_INSTRUCTION_STATE_MAX_INSN_BYTES);
#else
	if (insn_copyin_count > x86_INSTRUCTION_STATE_MAX_INSN_BYTES ||
	    insn_copyin_count < 64                     ) {
		return;
	}
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Walloca"
	stack_buffer = __builtin_alloca(insn_copyin_count);
#pragma clang diagnostic pop

	if (rip >= (insn_copyin_count / 2)) {
		start_addr = rip - (insn_copyin_count / 2);
	} else {
		start_addr = 0;
	}

	if (start_addr < rip_page) {
		insn_offset = (insn_copyin_count / 2) - (rip_page - start_addr);
		end_addr += (rip_page - start_addr);
		start_addr = rip_page;
	} else if (end_addr >= (rip_page + (~pagemask + 1))) {
		start_addr -= (end_addr - (rip_page + (~pagemask + 1)));                                    
		                                                           
		insn_offset = (insn_copyin_count / 2) + (end_addr - (rip_page + (~pagemask + 1)));
		end_addr = rip_page + (~pagemask + 1);                                                         
	} else {
		insn_offset = insn_copyin_count / 2;
	}

	disable_preemption();                                                               
	if (
#if DEVELOPMENT || DEBUG
		(insnstream_force_cacheline_mismatch < 2) &&
#endif
		((end_addr > start_addr) && (copyin_err = copyin(start_addr, stack_buffer, end_addr - start_addr)) == 0)) {
		enable_preemption();

		if (pcb->insn_state == 0) {
			pcb->insn_state = kalloc(sizeof(x86_instruction_state_t));
		}

		if (pcb->insn_state != 0) {
			bcopy(stack_buffer, pcb->insn_state->insn_bytes, end_addr - start_addr);
			bzero(&pcb->insn_state->insn_bytes[end_addr - start_addr],
			    insn_copyin_count - (end_addr - start_addr));

			pcb->insn_state->insn_stream_valid_bytes = (int)(end_addr - start_addr);
			pcb->insn_state->insn_offset = (int)insn_offset;

#if DEVELOPMENT || DEBUG
			                                                                                 
                                                                                    
                                                                                       
                          
      
#define CACHELINE_DATA_NOT_PRESENT 0xdeadc0debeefcafeULL
#define CACHELINE_MASK (CACHELINE_SIZE - 1)

			if (inspect_cacheline &&
			    (*(uint64_t *)(uintptr_t)&pcb->insn_cacheline[0] != CACHELINE_DATA_NOT_PRESENT &&
			    *(uint64_t *)(uintptr_t)&pcb->insn_cacheline[8] != CACHELINE_DATA_NOT_PRESENT)) {
				  
                                                                           
                                           
       
				if (__improbable((rip & CACHELINE_MASK) > insn_offset)) {
					printf("thread %p code cacheline @ %p clipped wrt copied-in code (offset %d)\n",
					    thread, (void *)(rip & ~CACHELINE_MASK), (int)(rip & CACHELINE_MASK));
				} else if (bcmp(&pcb->insn_state->insn_bytes[insn_offset - (rip & CACHELINE_MASK)],
				    &pcb->insn_cacheline[0], CACHELINE_SIZE) != 0
				    || insnstream_force_cacheline_mismatch
				    ) {
#if x86_INSTRUCTION_STATE_CACHELINE_SIZE != CACHELINE_SIZE
#error cacheline size mismatch
#endif
					bcopy(&pcb->insn_cacheline[0], &pcb->insn_state->insn_cacheline[0],
					    x86_INSTRUCTION_STATE_CACHELINE_SIZE);
					                                                       
					pcb->insn_state->out_of_synch = 1;

					printf("thread %p code cacheline @ %p mismatches with copied-in code [trap 0x%x]\n",
					    thread, (void *)(rip & ~CACHELINE_MASK), trap_code);
					for (int i = 0; i < 8; i++) {
						printf("\t[%d] cl=0x%08llx vs. ci=0x%08llx\n", i, *(uint64_t *)(uintptr_t)&pcb->insn_cacheline[i * 8],
						    *(uint64_t *)(uintptr_t)&pcb->insn_state->insn_bytes[(i * 8) + insn_offset - (rip & CACHELINE_MASK)]);
					}
					if (panic_on_cacheline_mismatch) {
						panic("Cacheline mismatch while processing unhandled exception.");
					}
				} else {
					pcb->insn_state->out_of_synch = 0;
				}
			} else if (inspect_cacheline) {
				printf("thread %p could not capture code cacheline at fault IP %p [offset %d]\n",
				    (void *)thread, (void *)rip, (int)(insn_offset - (rip & CACHELINE_MASK)));
				pcb->insn_state->out_of_synch = 0;
			}
#else
			pcb->insn_state->out_of_synch = 0;
#endif                           

#if defined(MACH_BSD) && (DEVELOPMENT || DEBUG)
			if (panic_on_trap_procname[0] != 0) {
				char procnamebuf[65] = {0};

				if (thread->task->bsd_info != NULL) {
					procname = proc_name_address(thread->task->bsd_info);
					strlcpy(procnamebuf, procname, sizeof(procnamebuf));

					if (strcasecmp(panic_on_trap_procname, procnamebuf) == 0 &&
					    ((1U << trap_code) & panic_on_trap_mask) != 0) {
						panic("Panic requested on trap type 0x%x for process `%s'", trap_code,
						    panic_on_trap_procname);
						            
					}
				}
			}
#endif                                         
		}
	} else {
		enable_preemption();

		pcb->insn_state_copyin_failure_errorcode = copyin_err;
#if DEVELOPMENT || DEBUG
		if (inspect_cacheline && pcb->insn_state == 0) {
			pcb->insn_state = kalloc(sizeof(x86_instruction_state_t));
		}
		if (pcb->insn_state != 0) {
			pcb->insn_state->insn_stream_valid_bytes = 0;
			pcb->insn_state->insn_offset = 0;

			if (inspect_cacheline &&
			    (*(uint64_t *)(uintptr_t)&pcb->insn_cacheline[0] != CACHELINE_DATA_NOT_PRESENT &&
			    *(uint64_t *)(uintptr_t)&pcb->insn_cacheline[8] != CACHELINE_DATA_NOT_PRESENT)) {
				  
                                                                           
                                
       
				pcb->insn_state->out_of_synch = 1;
				bcopy(&pcb->insn_cacheline[0], &pcb->insn_state->insn_cacheline[0],
				    x86_INSTRUCTION_STATE_CACHELINE_SIZE);
			}
		}
#endif                           
	}
}

  
                              
  
                                                              
                                    
  
                                                            
                                                               
            
   
void
i386_exception(
	int     exc,
	mach_exception_code_t code,
	mach_exception_subcode_t subcode)
{
	mach_exception_data_type_t   codes[EXCEPTION_CODE_MAX];

	DEBUG_KPRINT_SYSCALL_MACH("i386_exception: exc=%d code=0x%llx subcode=0x%llx\n",
	    exc, code, subcode);
	codes[0] = code;                                             
	codes[1] = subcode;
	exception_triage(exc, codes, 2);
	              
}


                                                                  
                                                                  
                                                                   
                                                                   
                                                                      
   

void
sync_iss_to_iks(x86_saved_state_t *saved_state)
{
	struct x86_kernel_state *iks = NULL;
	vm_offset_t kstack;
	boolean_t record_active_regs = FALSE;

	                                                      
	if (saved_state && saved_state->flavor == THREAD_STATE_NONE) {
		pal_get_kern_regs( saved_state );
	}

	if (current_thread() != NULL &&
	    (kstack = current_thread()->kernel_stack) != 0) {
		x86_saved_state64_t     *regs = saved_state64(saved_state);

		iks = STACK_IKS(kstack);

		                                                    
		if (saved_state == NULL ||                                
		    regs == USER_REGS64(current_thread())) {
			record_active_regs = TRUE;
		} else {
			iks->k_rbx = regs->rbx;
			iks->k_rsp = regs->isf.rsp;
			iks->k_rbp = regs->rbp;
			iks->k_r12 = regs->r12;
			iks->k_r13 = regs->r13;
			iks->k_r14 = regs->r14;
			iks->k_r15 = regs->r15;
			iks->k_rip = regs->isf.rip;
		}
	}

	if (record_active_regs == TRUE) {
		                                
		__asm__ volatile ("movq %%rbx, %0" : "=m" (iks->k_rbx));
		__asm__ volatile ("movq %%rsp, %0" : "=m" (iks->k_rsp));
		__asm__ volatile ("movq %%rbp, %0" : "=m" (iks->k_rbp));
		__asm__ volatile ("movq %%r12, %0" : "=m" (iks->k_r12));
		__asm__ volatile ("movq %%r13, %0" : "=m" (iks->k_r13));
		__asm__ volatile ("movq %%r14, %0" : "=m" (iks->k_r14));
		__asm__ volatile ("movq %%r15, %0" : "=m" (iks->k_r15));
		                                   
		__asm__ volatile ("leaq 1f(%%rip), %%rax; mov %%rax, %0\n1:"
                                  : "=m" (iks->k_rip)
                                  :
                                  : "rax");
	}
}

  
                                                           
                                                          
                                                        
                 
   
void
sync_iss_to_iks_unconditionally(__unused x86_saved_state_t *saved_state)
{
	struct x86_kernel_state *iks;
	vm_offset_t kstack;

	if ((kstack = current_thread()->kernel_stack) != 0) {
		iks = STACK_IKS(kstack);
		                                   
		__asm__ volatile ("movq %%rbx, %0" : "=m" (iks->k_rbx));
		__asm__ volatile ("movq %%rsp, %0" : "=m" (iks->k_rsp));
		__asm__ volatile ("movq %%rbp, %0" : "=m" (iks->k_rbp));
		__asm__ volatile ("movq %%r12, %0" : "=m" (iks->k_r12));
		__asm__ volatile ("movq %%r13, %0" : "=m" (iks->k_r13));
		__asm__ volatile ("movq %%r14, %0" : "=m" (iks->k_r14));
		__asm__ volatile ("movq %%r15, %0" : "=m" (iks->k_r15));
		                                   
		__asm__ volatile ("leaq 1f(%%rip), %%rax; mov %%rax, %0\n1:" : "=m" (iks->k_rip)::"rax");
	}
}

#if DEBUG
#define TERI 1
#endif

#if TERI
extern void     thread_exception_return_internal(void) __dead2;

void
thread_exception_return(void)
{
	thread_t thread = current_thread();
	ml_set_interrupts_enabled(FALSE);
	if (thread_is_64bit_addr(thread) != task_has_64Bit_addr(thread->task)) {
		panic("Task/thread bitness mismatch %p %p, task: %d, thread: %d", thread, thread->task, thread_is_64bit_addr(thread), task_has_64Bit_addr(thread->task));
	}

	if (thread_is_64bit_addr(thread)) {
		if ((gdt_desc_p(USER64_CS)->access & ACC_PL_U) == 0) {
			panic("64-GDT mismatch %p, descriptor: %p", thread, gdt_desc_p(USER64_CS));
		}
	} else {
		if ((gdt_desc_p(USER_CS)->access & ACC_PL_U) == 0) {
			panic("32-GDT mismatch %p, descriptor: %p", thread, gdt_desc_p(USER_CS));
		}
	}
	assert(get_preemption_level() == 0);
	thread_exception_return_internal();
}
#endif
