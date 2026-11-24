/*
 * Copyright (c) 2005-2018 Apple Inc. All rights reserved.
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

#include <kern/thread.h>
#include <mach/thread_status.h>

typedef x86_saved_state_t savearea_t;

#include <stdarg.h>
#include <string.h>
#include <sys/malloc.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/dtrace.h>
#include <sys/dtrace_impl.h>
#include <libkern/OSAtomic.h>
#include <kern/thread_call.h>
#include <kern/task.h>
#include <kern/sched_prim.h>
#include <miscfs/devfs/devfs.h>
#include <mach/vm_param.h>
#include <machine/pal_routines.h>
#include <i386/mp.h>
#include <machine/trap.h>

  
                                                                         
                                                                     
                                                                            
                                                                        
                                          
   
#include "fasttrap_regset.h"
static const uint8_t regmap[19] = {
    REG_GS,		        
    REG_FS,		        
    REG_ES,		        
    REG_DS,		        
    REG_RDI,		         
    REG_RSI,		         
    REG_RBP,		                  
    REG_RSP,		         
    REG_RBX,		         
    REG_RDX,		                  
    REG_RCX,		         
    REG_RAX,		                  
    REG_TRAPNO,		            
    REG_ERR,		         
    REG_RIP,		                  
    REG_CS,		        
    REG_RFL,		                  
    REG_RSP,		                  
    REG_SS		        
};    

extern dtrace_id_t      dtrace_probeid_error;                            

void
dtrace_probe_error(dtrace_state_t *state, dtrace_epid_t epid, int which,
    int fltoffs, int fault, uint64_t illval)
{
      
                                                  
                                                                                    
       
    state->dts_arg_error_illval = illval;
    dtrace_probe( dtrace_probeid_error, (uint64_t)(uintptr_t)state, epid, which, fltoffs, fault );
}

  
                                
   
void
dtrace_membar_producer(void)
{
	__asm__ volatile("sfence");
}

void
dtrace_membar_consumer(void)
{
	__asm__ volatile("lfence");
}

  
                         
                                                        
   
int
dtrace_getipl(void)
{
	  
                                                        
                            
    
	                                   
	return (ml_at_interrupt_context() ? 1: 0);
}

  
                  
   
typedef struct xcArg {
	processorid_t cpu;
	dtrace_xcall_t f;
	void *arg;
} xcArg_t;

static void
xcRemote( void *foo )
{
	xcArg_t *pArg = (xcArg_t *)foo;
	
	if ( pArg->cpu == CPU->cpu_id || pArg->cpu == DTRACE_CPUALL ) {
		(pArg->f)(pArg->arg);
	}
}


  
                                                   
   
void
dtrace_xcall(processorid_t cpu, dtrace_xcall_t f, void *arg)
{
	xcArg_t xcArg;
	
	xcArg.cpu = cpu;
	xcArg.f = f;
	xcArg.arg = arg;

	if (cpu == DTRACE_CPUALL) {
		mp_cpus_call (CPUMASK_ALL, ASYNC, xcRemote, (void*)&xcArg);
	}
	else {
		mp_cpus_call (cpu_to_cpumask((cpu_t)cpu), ASYNC, xcRemote, (void*)&xcArg);
	}
}

  
                  
   
uint64_t
dtrace_getreg(struct regs *savearea, uint_t reg)
{
	boolean_t is64Bit = proc_is64bit(current_proc());
	x86_saved_state_t *regs = (x86_saved_state_t *)savearea;

	if (regs == NULL) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
		return (0);
	}

	if (is64Bit) {
	    if (reg <= SS) {
		reg = regmap[reg];
	    } else {
		reg -= (SS + 1);
	    }

	    switch (reg) {
	    case REG_RDI:
		return (uint64_t)(regs->ss_64.rdi);
	    case REG_RSI:
		return (uint64_t)(regs->ss_64.rsi);
	    case REG_RDX:
		return (uint64_t)(regs->ss_64.rdx);
	    case REG_RCX:
		return (uint64_t)(regs->ss_64.rcx);
	    case REG_R8:
		return (uint64_t)(regs->ss_64.r8);
	    case REG_R9:
		return (uint64_t)(regs->ss_64.r9);
	    case REG_RAX:
		return (uint64_t)(regs->ss_64.rax);
	    case REG_RBX:
		return (uint64_t)(regs->ss_64.rbx);
	    case REG_RBP:
		return (uint64_t)(regs->ss_64.rbp);
	    case REG_R10:
		return (uint64_t)(regs->ss_64.r10);
	    case REG_R11:
		return (uint64_t)(regs->ss_64.r11);
	    case REG_R12:
		return (uint64_t)(regs->ss_64.r12);
	    case REG_R13:
		return (uint64_t)(regs->ss_64.r13);
	    case REG_R14:
		return (uint64_t)(regs->ss_64.r14);
	    case REG_R15:
		return (uint64_t)(regs->ss_64.r15);
	    case REG_FS:
		return (uint64_t)(regs->ss_64.fs);
	    case REG_GS:
		return (uint64_t)(regs->ss_64.gs);
	    case REG_TRAPNO:
		return (uint64_t)(regs->ss_64.isf.trapno);
	    case REG_ERR:
		return (uint64_t)(regs->ss_64.isf.err);
	    case REG_RIP:
		return (uint64_t)(regs->ss_64.isf.rip);
	    case REG_CS:
		return (uint64_t)(regs->ss_64.isf.cs);
	    case REG_SS:
		return (uint64_t)(regs->ss_64.isf.ss);
	    case REG_RFL:
		return (uint64_t)(regs->ss_64.isf.rflags);
	    case REG_RSP:
		return (uint64_t)(regs->ss_64.isf.rsp);
	    case REG_DS:
	    case REG_ES:
	    default:
		DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
		return (0);
	    }
	
	} else {                      
		                        
		if (reg > x86_SAVED_STATE32_COUNT - 1) {
			DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
			return (0);
		}
		return (uint64_t)((unsigned int *)(&(regs->ss_32.gs)))[reg];
	}
}

uint64_t
dtrace_getvmreg(uint_t ndx)
{
	uint64_t reg = 0;
	bool failed = false;

	                                                                                          
	__asm__ __volatile__(
		"vmread %2, %0\n"
		"ja 1f\n"
		"mov $1, %1\n"
		"1:\n"
	: "=a" (reg), "+r" (failed) : "D" ((uint64_t)ndx));

	  
                                                                              
                                                             
    
	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_ILLOP)) {
		return 0;
	}

	                                                                       
	if (failed) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_BADADDR);
		cpu_core[CPU->cpu_id].cpuc_dtrace_illval = ndx;
		return 0;
	}

	return reg;
}

#define RETURN_OFFSET 4
#define RETURN_OFFSET64 8

static int
dtrace_getustack_common(uint64_t *pcstack, int pcstack_limit, user_addr_t pc,
    user_addr_t sp)
{
	volatile uint16_t *flags =
	    (volatile uint16_t *)&cpu_core[CPU->cpu_id].cpuc_dtrace_flags;

#if 0
	uintptr_t oldcontext = lwp->lwp_oldcontext;                             
	size_t s1, s2;
#endif
	int ret = 0;
	boolean_t is64Bit = proc_is64bit(current_proc());

	ASSERT(pcstack == NULL || pcstack_limit > 0);
	
#if 0                             
	if (p->p_model == DATAMODEL_NATIVE) {
		s1 = sizeof (struct frame) + 2 * sizeof (long);
		s2 = s1 + sizeof (siginfo_t);
	} else {
		s1 = sizeof (struct frame32) + 3 * sizeof (int);
		s2 = s1 + sizeof (siginfo32_t);
	}
#endif

	while (pc != 0) {
		ret++;
		if (pcstack != NULL) {
			*pcstack++ = (uint64_t)pc;
			pcstack_limit--;
			if (pcstack_limit <= 0)
				break;
		}

		if (sp == 0)
			break;

#if 0                             
		if (oldcontext == sp + s1 || oldcontext == sp + s2) {
			if (p->p_model == DATAMODEL_NATIVE) {
				ucontext_t *ucp = (ucontext_t *)oldcontext;
				greg_t *gregs = ucp->uc_mcontext.gregs;

				sp = dtrace_fulword(&gregs[REG_FP]);
				pc = dtrace_fulword(&gregs[REG_PC]);

				oldcontext = dtrace_fulword(&ucp->uc_link);
			} else {
				ucontext32_t *ucp = (ucontext32_t *)oldcontext;
				greg32_t *gregs = ucp->uc_mcontext.gregs;

				sp = dtrace_fuword32(&gregs[EBP]);
				pc = dtrace_fuword32(&gregs[EIP]);

				oldcontext = dtrace_fuword32(&ucp->uc_link);
			}
		} 
		else
#endif
		{
			if (is64Bit) {
				pc = dtrace_fuword64((sp + RETURN_OFFSET64));
				sp = dtrace_fuword64(sp);
			} else {
				pc = dtrace_fuword32((sp + RETURN_OFFSET));
				sp = dtrace_fuword32(sp);
			}
		}

		                                                   
		if (*flags & CPU_DTRACE_FAULT) {
			*flags &= ~CPU_DTRACE_FAULT;
			break;
		}
	}

	return (ret);
}


  
                                                          
   
static int
dtrace_adjust_stack(uint64_t **pcstack, int *pcstack_limit, user_addr_t *pc,
                    user_addr_t sp)
{
    volatile uint16_t *flags = (volatile uint16_t *) &cpu_core[CPU->cpu_id].cpuc_dtrace_flags;
    int64_t missing_tos;
    int rc = 0;
    boolean_t is64Bit = proc_is64bit(current_proc());

    ASSERT(pc != NULL);

    if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_ENTRY)) {
          
                                                                             
                                               
                                                       
                                                 
                                                                    
                                                       
                                                 
                      
           

        missing_tos = *pc;

        if (is64Bit)
            *pc = dtrace_fuword64(sp);
        else
            *pc = dtrace_fuword32(sp);

	                                                   
	if (*flags & CPU_DTRACE_FAULT) {
		*flags &= ~CPU_DTRACE_FAULT;
	}
    } else {
          
                                                                       
                                                            
                                                          
                                                        
                                                          
                        
           
        missing_tos = cpu_core[CPU->cpu_id].cpuc_missing_tos;
    }

    if (missing_tos != 0) {
        if (pcstack != NULL && pcstack_limit != NULL) {
              
                                                             
                                      
               
	    *(*pcstack)++ = missing_tos;
	    (*pcstack_limit)--;
	}
          
                                              
                                                
                                      
    
         rc = 1;
    }
    return rc;
}

void
dtrace_getupcstack(uint64_t *pcstack, int pcstack_limit)
{
	thread_t thread = current_thread();
	x86_saved_state_t *regs;
	user_addr_t pc, sp, fp;
	volatile uint16_t *flags =
	    (volatile uint16_t *)&cpu_core[CPU->cpu_id].cpuc_dtrace_flags;
	int n;
	boolean_t is64Bit = proc_is64bit(current_proc());

	if (*flags & CPU_DTRACE_FAULT)
		return;

	if (pcstack_limit <= 0)
		return;

	  
                                                               
    
	if (thread == NULL)
		goto zero;

	pal_register_cache_state(thread, VALID);
	regs = (x86_saved_state_t *)find_user_regs(thread);
	if (regs == NULL)
		goto zero;
		
	*pcstack++ = (uint64_t)dtrace_proc_selfpid();
	pcstack_limit--;

	if (pcstack_limit <= 0)
		return;

	if (is64Bit) {
		pc = regs->ss_64.isf.rip;
		sp = regs->ss_64.isf.rsp;
		fp = regs->ss_64.rbp;
	} else {
		pc = regs->ss_32.eip;
		sp = regs->ss_32.uesp;
		fp = regs->ss_32.ebp;
	}

          
                                                           
                                                         
                                 
    
	(void)dtrace_adjust_stack(&pcstack, &pcstack_limit, &pc, sp);

	if(pcstack_limit <= 0)
	    return;

	  
                                                   
                                                    
                                                   
              
    
	n = dtrace_getustack_common(pcstack, pcstack_limit, pc, fp);
	ASSERT(n >= 0);
	ASSERT(n <= pcstack_limit);

	pcstack += n;
	pcstack_limit -= n;

zero:
	while (pcstack_limit-- > 0)
		*pcstack++ = 0;
}

int
dtrace_getustackdepth(void)
{
	thread_t thread = current_thread();
	x86_saved_state_t *regs;
	user_addr_t pc, sp, fp;
	int n = 0;
	boolean_t is64Bit = proc_is64bit(current_proc());

	if (thread == NULL)
		return 0;

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_FAULT))
		return (-1);

	pal_register_cache_state(thread, VALID);
	regs = (x86_saved_state_t *)find_user_regs(thread);
	if (regs == NULL)
		return 0;

	if (is64Bit) {
		pc = regs->ss_64.isf.rip;
		sp = regs->ss_64.isf.rsp;
		fp = regs->ss_64.rbp;
	} else {
		pc = regs->ss_32.eip;
		sp = regs->ss_32.uesp;
		fp = regs->ss_32.ebp;
	}

	if (dtrace_adjust_stack(NULL, NULL, &pc, sp) == 1) {
	      
                                                  
                                                  
                                                 
                                                
                                      
        
	    n++;
	}
	
	  
                                                   
                                                    
                                                   
              
    

	n += dtrace_getustack_common(NULL, 0, pc, fp);

	return (n);
}

void
dtrace_getufpstack(uint64_t *pcstack, uint64_t *fpstack, int pcstack_limit)
{
	thread_t thread = current_thread();
	savearea_t *regs;
	user_addr_t pc, sp;
	volatile uint16_t *flags =
	    (volatile uint16_t *)&cpu_core[CPU->cpu_id].cpuc_dtrace_flags;
#if 0
	uintptr_t oldcontext;
	size_t s1, s2;
#endif
	boolean_t is64Bit = proc_is64bit(current_proc());

	if (*flags & CPU_DTRACE_FAULT)
		return;

	if (pcstack_limit <= 0)
		return;

	  
                                                               
    
	if (thread == NULL)
		goto zero;

	regs = (savearea_t *)find_user_regs(thread);
	if (regs == NULL)
		goto zero;
		
	*pcstack++ = (uint64_t)dtrace_proc_selfpid();
	pcstack_limit--;

	if (pcstack_limit <= 0)
		return;

	pc = regs->ss_32.eip;
	sp = regs->ss_32.ebp;
	
#if 0                             
	oldcontext = lwp->lwp_oldcontext;

	if (p->p_model == DATAMODEL_NATIVE) {
		s1 = sizeof (struct frame) + 2 * sizeof (long);
		s2 = s1 + sizeof (siginfo_t);
	} else {
		s1 = sizeof (struct frame32) + 3 * sizeof (int);
		s2 = s1 + sizeof (siginfo32_t);
	}
#endif

	if(dtrace_adjust_stack(&pcstack, &pcstack_limit, &pc, sp) == 1) {
              
                         
        
	    *fpstack++ = 0;
	    if (pcstack_limit <= 0)
		return;
	}

	while (pc != 0) {
		*pcstack++ = (uint64_t)pc;
		*fpstack++ = sp;
		pcstack_limit--;
		if (pcstack_limit <= 0)
			break;

		if (sp == 0)
			break;

#if 0                             
		if (oldcontext == sp + s1 || oldcontext == sp + s2) {
			if (p->p_model == DATAMODEL_NATIVE) {
				ucontext_t *ucp = (ucontext_t *)oldcontext;
				greg_t *gregs = ucp->uc_mcontext.gregs;

				sp = dtrace_fulword(&gregs[REG_FP]);
				pc = dtrace_fulword(&gregs[REG_PC]);

				oldcontext = dtrace_fulword(&ucp->uc_link);
			} else {
				ucontext_t *ucp = (ucontext_t *)oldcontext;
				greg_t *gregs = ucp->uc_mcontext.gregs;

				sp = dtrace_fuword32(&gregs[EBP]);
				pc = dtrace_fuword32(&gregs[EIP]);

				oldcontext = dtrace_fuword32(&ucp->uc_link);
			}
		} 
		else
#endif
		{
			if (is64Bit) {
				pc = dtrace_fuword64((sp + RETURN_OFFSET64));
				sp = dtrace_fuword64(sp);
			} else {
				pc = dtrace_fuword32((sp + RETURN_OFFSET));
				sp = dtrace_fuword32(sp);
			}
		}

		                                                   
		if (*flags & CPU_DTRACE_FAULT) {
			*flags &= ~CPU_DTRACE_FAULT;
			break;
		}
	}

zero:
	while (pcstack_limit-- > 0)
		*pcstack++ = 0;
}

void
dtrace_getpcstack(pc_t *pcstack, int pcstack_limit, int aframes,
		  uint32_t *intrpc)
{
	struct frame *fp = (struct frame *)__builtin_frame_address(0);
	struct frame *nextfp, *minfp, *stacktop;
	int depth = 0;
	int last = 0;
	uintptr_t pc;
	uintptr_t caller = CPU->cpu_dtrace_caller;
	int on_intr;

	if ((on_intr = CPU_ON_INTR(CPU)) != 0)
		stacktop = (struct frame *)dtrace_get_cpu_int_stack_top();
	else
		stacktop = (struct frame *)(dtrace_get_kernel_stack(current_thread()) + kernel_stack_size);

	minfp = fp;

	aframes++;

	if (intrpc != NULL && depth < pcstack_limit)
		pcstack[depth++] = (pc_t)intrpc;

	while (depth < pcstack_limit) {
		nextfp = *(struct frame **)fp;
		pc = *(uintptr_t *)(((uintptr_t)fp) + RETURN_OFFSET64);

		if (nextfp <= minfp || nextfp >= stacktop) {
			if (on_intr) {
				  
                                                
       
				vm_offset_t kstack_base = dtrace_get_kernel_stack(current_thread());

				minfp = (struct frame *)kstack_base;
				stacktop = (struct frame *)(kstack_base + kernel_stack_size);

				on_intr = 0;
				continue;
			}
			  
                                                     
                                                        
      
			last = 1;
		}

		if (aframes > 0) {
			if (--aframes == 0 && caller != 0) {
				  
                                               
                                               
           
       
				ASSERT(depth < pcstack_limit);
				pcstack[depth++] = (pc_t)caller;
				caller = 0;
			}
		} else {
			if (depth < pcstack_limit)
				pcstack[depth++] = (pc_t)pc;
		}

		if (last) {
			while (depth < pcstack_limit)
				pcstack[depth++] = 0;
			return;
		}

		fp = nextfp;
		minfp = fp;
	}
}

struct frame {
	struct frame *backchain;
	uintptr_t retaddr;
};

uint64_t
dtrace_getarg(int arg, int aframes, dtrace_mstate_t *mstate, dtrace_vstate_t *vstate)
{
	uint64_t val = 0;
	struct frame *fp = (struct frame *)__builtin_frame_address(0);
	uintptr_t *stack;
	uintptr_t pc;
	int i;


      
                                                                         
                                                      
       
    int inreg = 5;

	for (i = 1; i <= aframes; i++) {
		fp = fp->backchain;
		pc = fp->retaddr;

		if (dtrace_invop_callsite_pre != NULL
			&& pc  >  (uintptr_t)dtrace_invop_callsite_pre
			&& pc  <= (uintptr_t)dtrace_invop_callsite_post) {
			  
                                                           
                                                          
                                                     
                                        
                                                         
                                                        
                                                        
                                                       
                                                       
                       
      

			                                                      
			fp = fp->backchain;                                        
			fp = fp->backchain;                                   
			fp = fp->backchain;                                        
			
			x86_saved_state_t   *tagged_regs = (x86_saved_state_t *)&fp[1];
			x86_saved_state64_t *saved_state = saved_state64(tagged_regs);

			if (arg <= inreg) {
				stack = (uintptr_t *)(void*)&saved_state->rdi;
			} else {
				fp = (struct frame *)(saved_state->isf.rsp);
				stack = (uintptr_t *)&fp[1];                   
                    
				arg -= inreg + 1;
			}
			goto load;
		}
	}

	  
                                                           
                                                           
                                   
                                                         
                                                
    
	arg++;                           

	if (arg <= inreg) {
		  
                                                           
                                                         
                
     
		DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
		return (0);
	}

	arg -= (inreg + 1);
	stack = (uintptr_t *)&fp[1];                                

load:
	if (dtrace_canload((uint64_t)(stack + arg), sizeof(uint64_t),
		mstate, vstate)) {
		                                                          
		val = dtrace_load64((uint64_t)(stack + arg));
	}

	return (val);
}

  
                    
   
void
dtrace_toxic_ranges(void (*func)(uintptr_t base, uintptr_t limit))
{
	  
                                                                           
                                      
    
	func(0x0, VM_MIN_KERNEL_AND_KEXT_ADDRESS);
	if (VM_MAX_KERNEL_ADDRESS < ~(uintptr_t)0)
			func(VM_MAX_KERNEL_ADDRESS + 1, ~(uintptr_t)0);
}

  
              
   
extern boolean_t dtrace_handle_trap(int, x86_saved_state_t *);

boolean_t
dtrace_handle_trap(int trapno, x86_saved_state_t *state)
{
	x86_saved_state64_t *saved_state = saved_state64(state);

	if (!DTRACE_CPUFLAG_ISSET(CPU_DTRACE_NOFAULT)) {
		return FALSE;
	}

	  
                                                                                   
                                                                    
    
	if (trapno == T_INVALID_OPCODE) {
		uint8_t *inst = (uint8_t *)saved_state->isf.rip;

		                       
		if (inst[0] == 0x0f && inst[1] == 0x78 && inst[2] == 0xf8) {
			DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
			saved_state->isf.rip += 3;
			return TRUE;
		}
	}

	return FALSE;
}
