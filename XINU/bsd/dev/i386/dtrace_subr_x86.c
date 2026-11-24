/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/dtrace.h>
#include <sys/dtrace_glue.h>
#include <sys/dtrace_impl.h>
#include <sys/fasttrap.h>
#include <sys/vm.h>
#include <sys/user.h>
#include <sys/kauth.h>
#include <kern/debug.h>

int (*dtrace_pid_probe_ptr)(x86_saved_state_t *);
int (*dtrace_return_probe_ptr)(x86_saved_state_t *);

  
                                                                    
               
   
#define	T_INT3			3		                       
#define T_DTRACE_RET		0x7f		                       

kern_return_t
dtrace_user_probe(x86_saved_state_t *);

kern_return_t
dtrace_user_probe(x86_saved_state_t *regs)
{
	x86_saved_state64_t *regs64;
	x86_saved_state32_t *regs32;
        int trapno;

	  
          
   
                                                              
                                                              
    
	boolean_t user_mode = TRUE;

        if (is_saved_state64(regs) == TRUE) {
                regs64 = saved_state64(regs);
		regs32 = NULL;
                trapno = regs64->isf.trapno;
                user_mode = TRUE;                                          
        } else {
		regs64 = NULL;
                regs32 = saved_state32(regs);
                if (regs32->cs & 0x03) user_mode = TRUE;
                trapno = regs32->trapno;
        }

	lck_rw_t *rwp;
	struct proc *p = current_proc();

	uthread_t uthread = (uthread_t)get_bsdthread_info(current_thread());
	if (user_mode                          ) {
		  
                                                     
                                                     
                              
     
		kauth_cred_uthread_update(uthread, p);
	}

	if (trapno == T_DTRACE_RET) {
		uint8_t step = uthread->t_dtrace_step;
		uint8_t ret = uthread->t_dtrace_ret;
		user_addr_t npc = uthread->t_dtrace_npc;

		if (uthread->t_dtrace_ast) {
			printf("dtrace_user_probe() should be calling aston()\n");
			                  
			                            
		}

		  
                                  
     
		uthread->t_dtrace_ft = 0;

		  
                                                              
                                                             
                      
     
		if (step == 0) {
			  
                                                             
                                                                           
                                                        
      
	 		return KERN_FAILURE;
		}

		  
                                                           
                                                               
                                                               
                
     
		if (ret == 0) {
			if (regs64) {
				regs64->isf.rip = npc;
			} else {
				regs32->eip = (uint32_t)npc;
			}
			return KERN_SUCCESS;
		}

		  
                                                 
                                                         
     
		rwp = &CPU->cpu_ft_lock;
		lck_rw_lock_shared(rwp);

		if (dtrace_return_probe_ptr != NULL)
			(void) (*dtrace_return_probe_ptr)(regs);
		lck_rw_unlock_shared(rwp);

		if (regs64) {
			regs64->isf.rip = npc;
		} else {
			regs32->eip = (uint32_t)npc;
		}

		return KERN_SUCCESS;
	} else if (trapno == T_INT3) {
		uint8_t instr, instr2;
		rwp = &CPU->cpu_ft_lock;

		  
                                                          
                                                            
                                                             
                                                        
                                                  
     

		  
                                                                 
                                                                   
                                                                
                                                              
                                                                  
                                                               
     
		lck_rw_lock_shared(rwp);
		if (dtrace_pid_probe_ptr != NULL && 
		    (*dtrace_pid_probe_ptr)(regs) == 0) {
			lck_rw_unlock_shared(rwp);
			return KERN_SUCCESS;
		}
		lck_rw_unlock_shared(rwp);


		  
                                                               
                                                               
                                                           
                                                            
     
		user_addr_t pc = (regs64) ? regs64->isf.rip : (user_addr_t)regs32->eip;
		if (fuword8(pc - 1, &instr) == 0 && instr != FASTTRAP_INSTR &&                                   
			!(instr == 3 && fuword8(pc - 2, &instr2) == 0 && instr2 == 0xCD)) {                               
			if (regs64) {
				regs64->isf.rip--;
			} else {
				regs32->eip--;
			}
			return KERN_SUCCESS;
		}

	}

	return KERN_FAILURE;
}

void
dtrace_flush_caches(void)
{

}
