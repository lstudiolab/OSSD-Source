/*
 * Copyright (c) 2005-2018 Apple Computer, Inc. All rights reserved.
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

#include <arm/caches_internal.h>
#include <arm/proc_reg.h>

#include <kern/thread.h>
#include <mach/thread_status.h>

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
#include <machine/atomic.h>
#include <kern/simple_lock.h>
#include <kern/sched_prim.h>                                     
#include <kern/thread_call.h>
#include <kern/task.h>
#include <miscfs/devfs/devfs.h>
#include <mach/vm_param.h>

extern struct arm_saved_state *find_kern_regs(thread_t);

extern dtrace_id_t      dtrace_probeid_error;                            
typedef arm_saved_state_t savearea_t;

int dtrace_arm_condition_true(int condition, int cpsr);

  
                                
   
inline void
dtrace_membar_producer(void)
{
	__asm__ volatile ("dmb ish" : : : "memory");
}

inline void
dtrace_membar_consumer(void)
{
	__asm__ volatile ("dmb ish" : : : "memory");
}

  
                         
                                                        
   
int
dtrace_getipl(void)
{
	  
                                                        
                            
    
	                                   
	return ml_at_interrupt_context() ? 1 : 0;
}

  
                  
   

static LCK_MTX_DECLARE_ATTR(dt_xc_lock, &dtrace_lck_grp, &dtrace_lck_attr);
static uint32_t dt_xc_sync;

typedef struct xcArg {
	processorid_t   cpu;
	dtrace_xcall_t  f;
	void           *arg;
} xcArg_t;

static void
xcRemote(void *foo)
{
	xcArg_t *pArg = (xcArg_t *) foo;

	if (pArg->cpu == CPU->cpu_id || pArg->cpu == DTRACE_CPUALL) {
		(pArg->f)(pArg->arg);
	}

	if (os_atomic_dec(&dt_xc_sync, relaxed) == 0) {
		thread_wakeup((event_t) &dt_xc_sync);
	}
}

  
                                                   
   
void
dtrace_xcall(processorid_t cpu, dtrace_xcall_t f, void *arg)
{
	                                             
	lck_mtx_lock(&dt_xc_lock);

	xcArg_t xcArg;

	xcArg.cpu = cpu;
	xcArg.f = f;
	xcArg.arg = arg;

	cpu_broadcast_xcall(&dt_xc_sync, TRUE, xcRemote, (void*) &xcArg);

	lck_mtx_unlock(&dt_xc_lock);
	return;
}

  
                  
   
uint64_t
dtrace_getreg(struct regs * savearea, uint_t reg)
{
	struct arm_saved_state *regs = (struct arm_saved_state *) savearea;
	if (regs == NULL) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
		return 0;
	}
	                            
	if (reg > ARM_SAVED_STATE32_COUNT - 1) {
		DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
		return 0;
	}

	return (uint64_t) ((unsigned int *) (&(regs->r)))[reg];
}

uint64_t
dtrace_getvmreg(uint_t ndx)
{
#pragma unused(ndx)
	DTRACE_CPUFLAG_SET(CPU_DTRACE_ILLOP);
	return 0;
}

#define RETURN_OFFSET 4

static int
dtrace_getustack_common(uint64_t * pcstack, int pcstack_limit, user_addr_t pc,
    user_addr_t sp)
{
	volatile uint16_t *flags = (volatile uint16_t *) &cpu_core[CPU->cpu_id].cpuc_dtrace_flags;
	int ret = 0;

	ASSERT(pcstack == NULL || pcstack_limit > 0);

	while (pc != 0) {
		ret++;
		if (pcstack != NULL) {
			*pcstack++ = (uint64_t) pc;
			pcstack_limit--;
			if (pcstack_limit <= 0) {
				break;
			}
		}

		if (sp == 0) {
			break;
		}

		pc = dtrace_fuword32((sp + RETURN_OFFSET));
		sp = dtrace_fuword32(sp);

		                                                   
		if (*flags & CPU_DTRACE_FAULT) {
			*flags &= ~CPU_DTRACE_FAULT;
			break;
		}
	}

	return ret;
}

void
dtrace_getupcstack(uint64_t * pcstack, int pcstack_limit)
{
	thread_t        thread = current_thread();
	savearea_t     *regs;
	user_addr_t     pc, sp;
	volatile uint16_t *flags = (volatile uint16_t *) &cpu_core[CPU->cpu_id].cpuc_dtrace_flags;
	int n;

	if (*flags & CPU_DTRACE_FAULT) {
		return;
	}

	if (pcstack_limit <= 0) {
		return;
	}

	  
                                                               
    
	if (thread == NULL) {
		goto zero;
	}

	regs = (savearea_t *) find_user_regs(thread);
	if (regs == NULL) {
		goto zero;
	}

	*pcstack++ = (uint64_t)dtrace_proc_selfpid();
	pcstack_limit--;

	if (pcstack_limit <= 0) {
		return;
	}

	pc = regs->pc;
	sp = regs->sp;

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_ENTRY)) {
		*pcstack++ = (uint64_t) pc;
		pcstack_limit--;
		if (pcstack_limit <= 0) {
			return;
		}

		pc = regs->lr;
	}

	n = dtrace_getustack_common(pcstack, pcstack_limit, pc, regs->r[7]);

	ASSERT(n >= 0);
	ASSERT(n <= pcstack_limit);

	pcstack += n;
	pcstack_limit -= n;

zero:
	while (pcstack_limit-- > 0) {
		*pcstack++ = 0ULL;
	}
}

int
dtrace_getustackdepth(void)
{
	thread_t        thread = current_thread();
	savearea_t     *regs;
	user_addr_t     pc, sp;
	int             n = 0;

	if (thread == NULL) {
		return 0;
	}

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_FAULT)) {
		return -1;
	}

	regs = (savearea_t *) find_user_regs(thread);
	if (regs == NULL) {
		return 0;
	}

	pc = regs->pc;
	sp = regs->sp;

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_ENTRY)) {
		n++;
		pc = regs->lr;
	}

	  
                                                   
                                                    
                                                   
              
    

	n += dtrace_getustack_common(NULL, 0, pc, regs->r[7]);

	return n;
}

void
dtrace_getufpstack(uint64_t * pcstack, uint64_t * fpstack, int pcstack_limit)
{
	                        
	thread_t        thread = current_thread();
	savearea_t      *regs;
	user_addr_t     pc, sp;

	volatile        uint16_t  *flags = (volatile uint16_t *) &cpu_core[CPU->cpu_id].cpuc_dtrace_flags;

#if 0
	uintptr_t oldcontext;
	size_t          s1, s2;
#endif

	if (*flags & CPU_DTRACE_FAULT) {
		return;
	}

	if (pcstack_limit <= 0) {
		return;
	}

	  
                                                               
    
	if (thread == NULL) {
		goto zero;
	}

	regs = (savearea_t *) find_user_regs(thread);
	if (regs == NULL) {
		goto zero;
	}

	*pcstack++ = (uint64_t)dtrace_proc_selfpid();
	pcstack_limit--;

	if (pcstack_limit <= 0) {
		return;
	}

	pc = regs->pc;
	sp = regs->sp;

#if 0                                                       
	oldcontext = lwp->lwp_oldcontext;

	if (p->p_model == DATAMODEL_NATIVE) {
		s1 = sizeof(struct frame) + 2 * sizeof(long);
		s2 = s1 + sizeof(siginfo_t);
	} else {
		s1 = sizeof(struct frame32) + 3 * sizeof(int);
		s2 = s1 + sizeof(siginfo32_t);
	}
#endif

	if (DTRACE_CPUFLAG_ISSET(CPU_DTRACE_ENTRY)) {
		*pcstack++ = (uint64_t) pc;
		*fpstack++ = 0;
		pcstack_limit--;
		if (pcstack_limit <= 0) {
			return;
		}

		pc = dtrace_fuword32(sp);
	}
	while (pc != 0 && sp != 0) {
		*pcstack++ = (uint64_t) pc;
		*fpstack++ = sp;
		pcstack_limit--;
		if (pcstack_limit <= 0) {
			break;
		}

#if 0                                                       
		if (oldcontext == sp + s1 || oldcontext == sp + s2) {
			if (p->p_model == DATAMODEL_NATIVE) {
				ucontext_t     *ucp = (ucontext_t *) oldcontext;
				greg_t         *gregs = ucp->uc_mcontext.gregs;

				sp = dtrace_fulword(&gregs[REG_FP]);
				pc = dtrace_fulword(&gregs[REG_PC]);

				oldcontext = dtrace_fulword(&ucp->uc_link);
			} else {
				ucontext_t     *ucp = (ucontext_t *) oldcontext;
				greg_t         *gregs = ucp->uc_mcontext.gregs;

				sp = dtrace_fuword32(&gregs[EBP]);
				pc = dtrace_fuword32(&gregs[EIP]);

				oldcontext = dtrace_fuword32(&ucp->uc_link);
			}
		} else
#endif
		{
			pc = dtrace_fuword32((sp + RETURN_OFFSET));
			sp = dtrace_fuword32(sp);
		}

		                                                   
		if (*flags & CPU_DTRACE_FAULT) {
			*flags &= ~CPU_DTRACE_FAULT;
			break;
		}
	}

zero:
	while (pcstack_limit-- > 0) {
		*pcstack++ = 0ULL;
	}
}

void
dtrace_getpcstack(pc_t * pcstack, int pcstack_limit, int aframes,
    uint32_t * intrpc)
{
	struct frame   *fp = (struct frame *) __builtin_frame_address(0);
	struct frame   *nextfp, *minfp, *stacktop;
	int             depth = 0;
	int             on_intr;
	int             last = 0;
	uintptr_t       pc;
	uintptr_t       caller = CPU->cpu_dtrace_caller;

	if ((on_intr = CPU_ON_INTR(CPU)) != 0) {
		stacktop = (struct frame *) dtrace_get_cpu_int_stack_top();
	} else {
		stacktop = (struct frame *) (dtrace_get_kernel_stack(current_thread()) + kernel_stack_size);
	}

	minfp = fp;

	aframes++;

	if (intrpc != NULL && depth < pcstack_limit) {
		pcstack[depth++] = (pc_t) intrpc;
	}

	while (depth < pcstack_limit) {
		nextfp = *(struct frame **) fp;
		pc = *(uintptr_t *) (((uint32_t) fp) + RETURN_OFFSET);

		if (nextfp <= minfp || nextfp >= stacktop) {
			if (on_intr) {
				  
                                                
       
				arm_saved_state_t *arm_kern_regs = (arm_saved_state_t *) find_kern_regs(current_thread());
				if (arm_kern_regs) {
					nextfp = (struct frame *)arm_kern_regs->r[7];

					vm_offset_t kstack_base = dtrace_get_kernel_stack(current_thread());

					minfp = (struct frame *)kstack_base;
					stacktop = (struct frame *)(kstack_base + kernel_stack_size);

					on_intr = 0;

					if (nextfp <= minfp || nextfp >= stacktop) {
						last = 1;
					}
				} else {
					  
                                                              
                                                             
                                           
        
					last = 1;
				}
			} else {
				  
                                                      
                                                         
       
				last = 1;
			}
		}
		if (aframes > 0) {
			if (--aframes == 0 && caller != (uintptr_t)NULL) {
				  
                                               
                                               
           
       
				ASSERT(depth < pcstack_limit);
				pcstack[depth++] = (pc_t) caller;
				caller = (uintptr_t)NULL;
			}
		} else {
			if (depth < pcstack_limit) {
				pcstack[depth++] = (pc_t) pc;
			}
		}

		if (last) {
			while (depth < pcstack_limit) {
				pcstack[depth++] = (pc_t) NULL;
			}
			return;
		}
		fp = nextfp;
		minfp = fp;
	}
}

int
dtrace_instr_size(uint32_t instr, int thumb_mode)
{
	if (thumb_mode) {
		uint16_t instr16 = *(uint16_t*) &instr;
		if (((instr16 >> 11) & 0x1F) > 0x1C) {
			return 4;
		} else {
			return 2;
		}
	} else {
		return 4;
	}
}

uint64_t
dtrace_getarg(int arg, int aframes, dtrace_mstate_t *mstate, dtrace_vstate_t *vstate)
{
#pragma unused(arg, aframes, mstate, vstate)
#if 0
	                 
	uint64_t val;
	uintptr_t *fp = (uintptr_t *)__builtin_frame_address(0);
	uintptr_t *stack;
	uintptr_t pc;
	int i;

	for (i = 1; i <= aframes; i++) {
		fp = fp[0];
		pc = fp[1];

		if (dtrace_invop_callsite_pre != NULL
		    && pc > (uintptr_t)dtrace_invop_callsite_pre
		    && pc <= (uintptr_t)dtrace_invop_callsite_post) {
			  
                                                        
                                                        
                                                         
                                  
      

			stack = (uintptr_t *)&fp[1];                                
			fp = (struct frame *)stack[1];                             
			stack = (uintptr_t *)&fp[1];                                
			DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
			val = (uint64_t)(stack[arg]);
			DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);
			return val;
		}
	}

	  
                                                               
    
	stack = (uintptr_t *)&fp[1];                                
	stack++;                           

	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	val = *(((uint64_t *)stack) + arg);                                                          
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT);
	return val;
#endif
	return 0xfeedfacedeafbeadLL;
}

void
dtrace_probe_error(dtrace_state_t *state, dtrace_epid_t epid, int which,
    int fltoffs, int fault, uint64_t illval)
{
	                 
	  
                                               
                                                                                 
    
	state->dts_arg_error_illval = illval;
	dtrace_probe( dtrace_probeid_error, (uint64_t)(uintptr_t)state, epid, which, fltoffs, fault );
}

void
dtrace_toxic_ranges(void (*func)(uintptr_t base, uintptr_t limit))
{
	                                          
	  
                                                                           
                                      
    
	func(0x0, VM_MIN_KERNEL_ADDRESS);
	if (VM_MAX_KERNEL_ADDRESS < ~(uintptr_t)0) {
		func(VM_MAX_KERNEL_ADDRESS + 1, ~(uintptr_t)0);
	}
}

int
dtrace_arm_condition_true(int cond, int cpsr)
{
	int taken = 0;
	int zf = (cpsr & PSR_ZF) ? 1 : 0,
	    nf = (cpsr & PSR_NF) ? 1 : 0,
	    cf = (cpsr & PSR_CF) ? 1 : 0,
	    vf = (cpsr & PSR_VF) ? 1 : 0;

	switch (cond) {
	case 0: taken = zf; break;
	case 1: taken = !zf; break;
	case 2: taken = cf; break;
	case 3: taken = !cf; break;
	case 4: taken = nf; break;
	case 5: taken = !nf; break;
	case 6: taken = vf; break;
	case 7: taken = !vf; break;
	case 8: taken = (cf && !zf); break;
	case 9: taken = (!cf || zf); break;
	case 10: taken = (nf == vf); break;
	case 11: taken = (nf != vf); break;
	case 12: taken = (!zf && (nf == vf)); break;
	case 13: taken = (zf || (nf != vf)); break;
	case 14: taken = 1; break;
	case 15: taken = 1; break;                                                              
	}

	return taken;
}

void
dtrace_flush_caches(void)
{
	                                                                                          
                                                                                                   
    
	FlushPoU_Dcache();
	InvalidatePoU_Icache();
}
