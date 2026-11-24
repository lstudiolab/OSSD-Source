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
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
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
  
   

  
                         
  
                                                              
                                       
   

#ifndef _I386_THREAD_H_
#define _I386_THREAD_H_

#include <mach/boolean.h>
#include <mach/i386/vm_types.h>
#include <mach/i386/fp_reg.h>
#include <mach/thread_status.h>

#include <kern/simple_lock.h>

#include <i386/fpu.h>
#include <i386/iopb.h>
#include <i386/seg.h>
#include <i386/tss.h>
#include <i386/eflags.h>

#include <i386/cpu_data.h>
#include <i386/proc_reg.h>

#include <machine/pal_routines.h>

  
                                                 
  
                                                              
                                                                    
   

struct x86_kernel_state {
	uint64_t        k_rbx;                      
	uint64_t        k_rsp;
	uint64_t        k_rbp;
	uint64_t        k_r12;
	uint64_t        k_r13;
	uint64_t        k_r14;
	uint64_t        k_r15;
	uint64_t        k_rip;
};

#ifdef  MACH_KERNEL_PRIVATE
typedef struct x86_kernel_state machine_thread_kernel_state;
#include <kern/thread_kernel_state.h>
#endif

  
                                                     
   
extern unsigned int _MachineStateCount[];

  
                                                                            
                                                                        
                                                                             
              
   
struct machine_thread {
	x86_saved_state_t       *iss;
	void                    *ifps;
	void                    *ids;
	decl_simple_lock_data(, lock);                                      
	xstate_t                xstate;

#ifdef  MACH_BSD
	uint64_t                cthread_self;                                   
	struct real_descriptor  cthread_desc;
	unsigned long           uldt_selector;                                
	struct real_descriptor  uldt_desc;                                   
#endif

	struct pal_pcb          pal_pcb;
	uint32_t                specFlags;
	                                                                        
                                                                      
                                                                       
                                                                       
                                             
    
#define         OnProc          0x1
#define         CopyIOActive    0x2                                                                 
	uint64_t                thread_gpu_ns;
	uint32_t                last_xcpm_ttd;
	uint8_t                 last_xcpm_index;
	int                     mthr_do_segchk;
	int                     insn_state_copyin_failure_errorcode;                                                      
	x86_instruction_state_t *insn_state;
#if DEVELOPMENT || DEBUG
	                                                                               
	uint8_t                 insn_cacheline[65];                                         
#endif
	x86_lbrs_t              lbrs;
	bool                    insn_copy_optout;
};
typedef struct machine_thread *pcb_t;

#define THREAD_TO_PCB(Thr)      (&(Thr)->machine)

#define USER_STATE(Thr)         ((Thr)->machine.iss)
#define USER_REGS32(Thr)        (saved_state32(USER_STATE(Thr)))
#define USER_REGS64(Thr)        (saved_state64(USER_STATE(Thr)))

#define user_pc(Thr)            (is_saved_state32(USER_STATE(Thr)) ?    \
	                                USER_REGS32(Thr)->eip :         \
	                                USER_REGS64(Thr)->isf.rip )

extern void *get_user_regs(thread_t);

extern void *act_thread_csave(void);
extern void act_thread_catt(void *ctx);
extern void act_thread_cfree(void *ctx);

#define FIND_PERFCONTROL_STATE(th)      (PERFCONTROL_STATE_NULL)

  
                          
             
                              
                          
   


#define STACK_IKS(stack)        \
	(&(((struct thread_kernel_state *)((stack) + kernel_stack_size)) - 1)->machine)

  
                                                               
   
static inline vm_offset_t
current_stack_depth(void)
{
	vm_offset_t     stack_ptr;

	assert(get_preemption_level() > 0 || !ml_get_interrupts_enabled());

#if defined(__x86_64__)
	__asm__ volatile ("mov %%rsp, %0" : "=m" (stack_ptr));
#else
	__asm__ volatile ("mov %%esp, %0" : "=m" (stack_ptr));
#endif
	return current_cpu_datap()->cpu_kernel_stack
	       + sizeof(struct thread_kernel_state)
	       - stack_ptr;
}

  
                                                                     
                                                      
   
#define GET_RETURN_PC(addr)     (__builtin_return_address(0))

#endif                       
