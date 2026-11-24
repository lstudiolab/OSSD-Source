/*
 * Copyright (c) 2010-2012 Apple Inc. All rights reserved.
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
#include <mach_debug.h>
#include <mach_ldebug.h>

#include <mach/kern_return.h>
#include <mach/mach_traps.h>
#include <mach/thread_status.h>
#include <mach/vm_param.h>

#include <kern/cpu_data.h>
#include <kern/mach_param.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>
#include <kern/debug.h>
#include <kern/spl.h>
#include <kern/syscall_sw.h>
#include <ipc/ipc_port.h>
#include <vm/vm_kern.h>
#include <vm/pmap.h>

#include <i386/cpu_number.h>
#include <i386/eflags.h>
#include <i386/proc_reg.h>
#include <i386/tss.h>
#include <i386/user_ldt.h>
#include <i386/fpu.h>
#include <i386/machdep_call.h>
#include <i386/vmparam.h>
#include <i386/mp_desc.h>
#include <i386/misc_protos.h>
#include <i386/thread.h>
#include <i386/trap.h>
#include <i386/seg.h>
#include <mach/i386/syscall_sw.h>
#include <sys/syscall.h>
#include <sys/kdebug.h>
#include <sys/errno.h>
#include <../bsd/sys/sysent.h>


  
                                  
                 
   
kern_return_t
machine_thread_dup(
	thread_t            parent,
	thread_t            child,
	__unused boolean_t  is_corpse
	)
{
	pcb_t           parent_pcb = THREAD_TO_PCB(parent);
	pcb_t           child_pcb = THREAD_TO_PCB(child);

	  
                                           
    
	if (thread_is_64bit_addr(parent)) {
		bcopy(USER_REGS64(parent), USER_REGS64(child), sizeof(x86_saved_state64_t));
	} else {
		bcopy(USER_REGS32(parent), USER_REGS32(child), sizeof(x86_saved_state32_t));
	}

	  
                                                  
                                              
    
	fpu_dup_fxstate(parent, child);

#ifdef  MACH_BSD
	  
                                                                        
    
	child_pcb->cthread_self = parent_pcb->cthread_self;
	if (!thread_is_64bit_addr(parent)) {
		child_pcb->cthread_desc = parent_pcb->cthread_desc;
	}

	  
                                                         
                                           
    
	                                                                        
	if (parent_pcb->uldt_selector != 0) {
		child_pcb->uldt_selector = parent_pcb->uldt_selector;
		child_pcb->uldt_desc = parent_pcb->uldt_desc;
	}
#endif

	return KERN_SUCCESS;
}

void thread_set_parent(thread_t parent, int pid);

void
thread_set_parent(thread_t parent, int pid)
{
	pal_register_cache_state(parent, DIRTY);

	if (thread_is_64bit_addr(parent)) {
		x86_saved_state64_t     *iss64;

		iss64 = USER_REGS64(parent);

		iss64->rax = pid;
		iss64->rdx = 0;
		iss64->isf.rflags &= ~EFL_CF;
	} else {
		x86_saved_state32_t     *iss32;

		iss32 = USER_REGS32(parent);

		iss32->eax = pid;
		iss32->edx = 0;
		iss32->efl &= ~EFL_CF;
	}
}

  
                                                                         
                                                                           
  
                                                          
  
                                                 
                                                     
   
kern_return_t
thread_fast_set_cthread_self(uint32_t self)
{
	machine_thread_set_tsd_base(current_thread(), self);
	return USER_CTHREAD;                                 
}

  
                                                                           
                                                                           
  
                                                   
  
                                                 
                                                     
   
kern_return_t
thread_fast_set_cthread_self64(uint64_t self)
{
	machine_thread_set_tsd_base(current_thread(), self);
	return USER_CTHREAD;                                 
}

  
                                                                  
                                                                   
                                                                   
                                                                   
                                                                     
  
                                                                  
                                                                     
                                                                
                                                                  
                                                                     
                                                       
  
                                                                    
                                                                   
                                                                   
                                                                 
          
  
                                                                   
                                                                   
                                                                   
                                                                      
                                                                 
  
                                                                   
   
kern_return_t
thread_set_user_ldt(uint32_t address, uint32_t size, uint32_t flags)
{
	pcb_t pcb;
	struct fake_descriptor temp;

	if (flags != 0) {
		return -1;                                    
	}
	if (size > 0xFFFFF) {
		return -1;                                                 
	}
	mp_disable_preemption();

	                                                      
	                         
	                                  
	                                                 
	pcb = THREAD_TO_PCB(current_thread());
	temp.offset = address;
	temp.lim_or_seg = size;
	temp.size_or_wdct = SZ_32;
	temp.access = ACC_P | ACC_PL_U | ACC_DATA_W;

	                                   
	fix_desc(&temp, 1);

	                             
	pcb->uldt_desc = *(struct real_descriptor*)&temp;
	pcb->uldt_selector = USER_SETTABLE;                                      

	                                        
	*ldt_desc_p(USER_SETTABLE) = *(struct real_descriptor*)&temp;

	mp_enable_preemption();

	return USER_SETTABLE;
}
