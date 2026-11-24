/*
 *  Copyright (c) 2007 Apple Inc. All rights reserved.
 */
/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
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
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
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
#include <arm/proc_reg.h>

int             (*dtrace_pid_probe_ptr)(arm_saved_state_t *);
int             (*dtrace_return_probe_ptr) (arm_saved_state_t *);

kern_return_t
dtrace_user_probe(arm_saved_state_t *, unsigned int);

kern_return_t
dtrace_user_probe(arm_saved_state_t *regs, unsigned int instr)
{
	  
         
   
                                                              
                                                              
    

	lck_rw_t *rwp;
	struct proc *p = current_proc();

	uthread_t uthread = (uthread_t)get_bsdthread_info(current_thread());

	kauth_cred_uthread_update(uthread, p);

	if (((regs->cpsr & PSR_TF) && ((uint16_t) instr) == FASTTRAP_THUMB_RET_INSTR) ||
	    ((uint32_t) instr == FASTTRAP_ARM_RET_INSTR)) {
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
			regs->pc = npc;
			return KERN_SUCCESS;
		}

		  
                                                 
                                                             
     
		rwp = &CPU->cpu_ft_lock;
		lck_rw_lock_shared(rwp);

		if (dtrace_return_probe_ptr != NULL) {
			(void) (*dtrace_return_probe_ptr)(regs);
		}
		lck_rw_unlock_shared(rwp);

		regs->pc = npc;

		return KERN_SUCCESS;
	} else {
		rwp = &CPU->cpu_ft_lock;

		  
                                              
                                       
                                            
                                                            
                                                        
                                                  
     

		  
                                                                 
                                                                   
                                                                
                                                              
                                                                  
                                                               
     
		lck_rw_lock_shared(rwp);
		if (dtrace_pid_probe_ptr != NULL &&
		    (*dtrace_pid_probe_ptr)(regs) == 0) {
			lck_rw_unlock_shared(rwp);
			return KERN_SUCCESS;
		}
		lck_rw_unlock_shared(rwp);

		  
                                                               
                                                               
                                                           
                                                            
    
                                                                      
     
		if (regs->cpsr & PSR_TF) {
			uint16_t instr_check;
			if (fuword16(regs->pc, &instr_check) == 0 && instr_check != FASTTRAP_THUMB_INSTR) {
				return KERN_SUCCESS;
			}
		} else {
			uint32_t instr_check;
			if (fuword32(regs->pc, &instr_check) == 0 && instr_check != FASTTRAP_ARM_INSTR) {
				return KERN_SUCCESS;
			}
		}
	}

	return KERN_FAILURE;
}
