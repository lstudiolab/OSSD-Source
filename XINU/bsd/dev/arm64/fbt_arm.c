/*
 * Copyright (c) 2007-2018 Apple Inc. All rights reserved.
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

#include <kern/thread.h>
#include <mach/thread_status.h>
#include <arm/proc_reg.h>
#include <arm/caches_internal.h>

#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <libkern/kernel_mach_header.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <miscfs/devfs/devfs.h>

#include <sys/dtrace.h>
#include <sys/dtrace_impl.h>
#include <sys/fbt.h>

#include <sys/dtrace_glue.h>

#if __has_include(<ptrauth.h>)
#include <ptrauth.h>
#endif

#define DTRACE_INVOP_PUSH_FRAME 11

#define DTRACE_INVOP_NOP_SKIP           4
#define DTRACE_INVOP_ADD_FP_SP_SKIP     4

#define DTRACE_INVOP_POP_PC_SKIP 2

  
                         
                          
   
#define FBT_IS_ARM64_FRAME_PUSH(x)      \
	(((x) & 0xffc07fff) == 0xa9007bfd || ((x) & 0xffc07fff) == 0xa9807bfd)

  
                           
                            
   
#define FBT_IS_ARM64_PUSH(x)            \
	(((x) & 0xffc003e0) == 0xa90003e0 || ((x) & 0xffc003e0) == 0xa98003e0)

  
                          
                         
   
#define FBT_IS_ARM64_FRAME_POP(x)       \
	(((x) & 0xffc07fff) == 0xa9407bfd || ((x) & 0xffc07fff) == 0xa8c07bfd)

#define FBT_IS_ARM64_ADD_FP_SP(x)       (((x) & 0xffc003ff) == 0x910003fd)                                                            
#define FBT_IS_ARM64_RET(x)             (((x) == 0xd65f03c0) || ((x) == 0xd65f0fff))                                    


#define FBT_B_MASK                      0xff000000
#define FBT_B_IMM_MASK                  0x00ffffff
#define FBT_B_INSTR                     0x14000000

#define FBT_IS_ARM64_B_INSTR(x)         ((x & FBT_B_MASK) == FBT_B_INSTR)
#define FBT_GET_ARM64_B_IMM(x)          ((x & FBT_B_IMM_MASK) << 2)

#define FBT_PATCHVAL                    0xe7eeee7e
#define FBT_AFRAMES_ENTRY               7
#define FBT_AFRAMES_RETURN              7

#define FBT_ENTRY       "entry"
#define FBT_RETURN      "return"
#define FBT_ADDR2NDX(addr)      ((((uintptr_t)(addr)) >> 4) & fbt_probetab_mask)

extern dtrace_provider_id_t     fbt_id;
extern fbt_probe_t               **fbt_probetab;
extern int                      fbt_probetab_mask;

kern_return_t fbt_perfCallback(int, struct arm_saved_state *, __unused int, __unused int);

int
fbt_invop(uintptr_t addr, uintptr_t * stack, uintptr_t rval)
{
	fbt_probe_t    *fbt = fbt_probetab[FBT_ADDR2NDX(addr)];

	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
		if ((uintptr_t) fbt->fbtp_patchpoint == addr) {
			if (0 == CPU->cpu_dtrace_invop_underway) {
				CPU->cpu_dtrace_invop_underway = 1;                            
                                                                   

				  
                             
      
                              
      
                           
                                 
                                    
                            
                                                                                       
                             
                                                                                          
                                                                  
                                    
                         
                                    
                                                                              
                                                                                           
                                                                                        
                                                                                             
                                                           
                                   
                 
                 
                                       
                 
                 
                 
                            
      
                                                                                             
                                                                                           
                                                                                         
                                                                                         
                                                         
       
				arm_saved_state_t *regs = (arm_saved_state_t *)(&((arm_context_t *)stack)->ss);

				CPU->cpu_dtrace_caller = get_saved_state_lr(regs);

				                                                                                 
				if (fbt->fbtp_roffset == 0) {
					dtrace_probe(fbt->fbtp_id, get_saved_state_reg(regs, 0), get_saved_state_reg(regs, 1),
					    get_saved_state_reg(regs, 2), get_saved_state_reg(regs, 3), get_saved_state_reg(regs, 4));
				} else {
					dtrace_probe(fbt->fbtp_id, fbt->fbtp_roffset, rval, 0, 0, 0);
				}

				CPU->cpu_dtrace_caller = 0;
				CPU->cpu_dtrace_invop_underway = 0;
			}

			  
                                                                                            
                                                                                                 
                                                                                                  
      
			return fbt->fbtp_savedval;
		}
	}

	return 0;
}

#define IS_USER_TRAP(regs)   (PSR64_IS_USER(get_saved_state_cpsr(regs)))
#define T_INVALID_OPCODE EXC_BAD_INSTRUCTION
#define FBT_EXCEPTION_CODE T_INVALID_OPCODE

kern_return_t
fbt_perfCallback(
	int trapno,
	struct arm_saved_state * regs,
	__unused int unused1,
	__unused int unused2)
{
	kern_return_t   retval = KERN_FAILURE;

	if (FBT_EXCEPTION_CODE == trapno && !IS_USER_TRAP(regs)) {
		boolean_t oldlevel = 0;
		machine_inst_t emul = 0;
		uint64_t sp, lr;
		uint32_t imm;

		oldlevel = ml_set_interrupts_enabled(FALSE);

		__asm__ volatile (
                         "Ldtrace_invop_callsite_pre_label:\n"
                         ".data\n"
                         ".private_extern _dtrace_invop_callsite_pre\n"
                         "_dtrace_invop_callsite_pre:\n"
                         "  .quad Ldtrace_invop_callsite_pre_label\n"
                         ".text\n"
                );

		emul = dtrace_invop(get_saved_state_pc(regs), (uintptr_t*) regs, get_saved_state_reg(regs, 0));

		__asm__ volatile (
                         "Ldtrace_invop_callsite_post_label:\n"
                         ".data\n"
                         ".private_extern _dtrace_invop_callsite_post\n"
                         "_dtrace_invop_callsite_post:\n"
                         "  .quad Ldtrace_invop_callsite_post_label\n"
                         ".text\n"
                );

		if (emul == DTRACE_INVOP_NOP) {
			  
                                              
      
			add_saved_state_pc(regs, DTRACE_INVOP_NOP_SKIP);
			retval = KERN_SUCCESS;
		} else if (FBT_IS_ARM64_ADD_FP_SP(emul)) {
			                               
			uint64_t val = (emul >> 10) & 0xfff;
			assert(val < 4096);

			                 
			sp = get_saved_state_sp(regs);

			  
                              
                               
      
			assert(sp < (UINT64_MAX - val));
			set_saved_state_fp(regs, sp + val);

			                                                    
			add_saved_state_pc(regs, DTRACE_INVOP_ADD_FP_SP_SKIP);

			retval = KERN_SUCCESS;
		} else if (FBT_IS_ARM64_RET(emul)) {
			lr = get_saved_state_lr(regs);
#if __has_feature(ptrauth_calls)
			lr = (user_addr_t) ptrauth_strip((void *)lr, ptrauth_key_return_address);
#endif
			set_saved_state_pc(regs, lr);
			retval = KERN_SUCCESS;
		} else if (FBT_IS_ARM64_B_INSTR(emul)) {
			imm = FBT_GET_ARM64_B_IMM(emul);
			add_saved_state_pc(regs, imm);
			retval = KERN_SUCCESS;
		} else if (emul == FBT_PATCHVAL) {
			                                                                       
			retval = KERN_SUCCESS;
		} else {
			retval = KERN_FAILURE;
		}

		ml_set_interrupts_enabled(oldlevel);
	}

	return retval;
}

void
fbt_provide_probe(struct modctl *ctl, const char *modname, const char* symbolName, machine_inst_t* symbolStart, machine_inst_t *instrHigh)
{
	int             doenable = 0;
	dtrace_id_t     thisid;

	fbt_probe_t     *newfbt, *retfbt, *entryfbt;
	machine_inst_t *instr, *pushinstr = NULL, *limit, theInstr;
	int             foundPushLR, savedRegs;

	  
                                          
    
	if (!symbolStart || !instrHigh || instrHigh < symbolStart) {
		kprintf("dtrace: %s has an invalid address\n", symbolName);
		return;
	}

	  
                                                                      
    
	foundPushLR = 0;
	savedRegs = -1;
	limit = (machine_inst_t *)instrHigh;

	assert(sizeof(*instr) == 4);

	for (instr = symbolStart, theInstr = 0; instr < instrHigh; instr++) {
		  
                                                                
                                                                 
                                                                 
                     
     
		theInstr = *instr;
		if (FBT_IS_ARM64_FRAME_PUSH(theInstr)) {
			foundPushLR = 1;
			pushinstr = instr;
		}

		if (foundPushLR && (FBT_IS_ARM64_ADD_FP_SP(theInstr))) {
			                                                                                        
			break;
		}
		if (FBT_IS_ARM64_RET(theInstr)) {                                
			break;
		}
		if (FBT_IS_ARM64_FRAME_POP(theInstr)) {                                
			break;
		}
	}

	if (!(foundPushLR && (FBT_IS_ARM64_ADD_FP_SP(theInstr)))) {
		return;
	}

	thisid = dtrace_probe_lookup(fbt_id, modname, symbolName, FBT_ENTRY);
	newfbt = kmem_zalloc(sizeof(fbt_probe_t), KM_SLEEP);
	newfbt->fbtp_next = NULL;
	strlcpy((char *)&(newfbt->fbtp_name), symbolName, MAX_FBTP_NAME_CHARS );

	if (thisid != 0) {
		  
                                                            
                                                        
           
                                                           
                                                            
                                               
     
		entryfbt = dtrace_probe_arg(fbt_id, thisid);
		ASSERT(entryfbt != NULL);
		for (; entryfbt != NULL; entryfbt = entryfbt->fbtp_next) {
			if (entryfbt->fbtp_currentval == entryfbt->fbtp_patchval) {
				doenable++;
			}

			if (entryfbt->fbtp_next == NULL) {
				entryfbt->fbtp_next = newfbt;
				newfbt->fbtp_id = entryfbt->fbtp_id;
				break;
			}
		}
	} else {
		  
                                                     
                                                          
                                                            
     
		newfbt->fbtp_id = dtrace_probe_create(fbt_id, modname, symbolName, FBT_ENTRY, FBT_AFRAMES_ENTRY, newfbt);
		doenable = 0;
	}

	newfbt->fbtp_patchpoint = instr;
	newfbt->fbtp_ctl = ctl;
	newfbt->fbtp_loadcnt = ctl->mod_loadcnt;
	newfbt->fbtp_rval = DTRACE_INVOP_PUSH_FRAME;
	newfbt->fbtp_savedval = theInstr;
	newfbt->fbtp_patchval = FBT_PATCHVAL;
	newfbt->fbtp_currentval = 0;
	newfbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = newfbt;

	if (doenable) {
		fbt_enable(NULL, newfbt->fbtp_id, newfbt);
	}

	  
                                                                
                                                            
           
                                                 
    

	doenable = 0;

	thisid = dtrace_probe_lookup(fbt_id, modname, symbolName, FBT_RETURN);

	if (thisid != 0) {
		                                                     
                                                        
                                                       
                                                       
                                                
     
		retfbt = dtrace_probe_arg(fbt_id, thisid);
		ASSERT(retfbt != NULL);
		for (; retfbt != NULL; retfbt =  retfbt->fbtp_next) {
			if (retfbt->fbtp_currentval == retfbt->fbtp_patchval) {
				doenable++;
			}
			if (retfbt->fbtp_next == NULL) {
				break;
			}
		}
	} else {
		doenable = 0;
		retfbt = NULL;
	}

	  
                                                 
                                         
                           
    
	instr = pushinstr + 1;
again:
	if (instr >= limit) {
		return;
	}

	                                               

	  
                            
    
	theInstr = *instr;

	                                                                                   
	if (FBT_IS_ARM64_FRAME_PUSH(theInstr)) {
		if (!retfbt) {
			kprintf("dtrace: fbt: No return probe for %s, walked to next routine at 0x%016llx\n", symbolName, (uint64_t)instr);
		}
		return;
	}

	                                                                    

	  
             
                               
                                
    
	if (!FBT_IS_ARM64_FRAME_POP(theInstr)) {
		instr++;
		goto again;
	}

	                                
	instr++;

	                                                           
	for (; instr < limit; instr++) {
		theInstr = *instr;
		if (FBT_IS_ARM64_RET(theInstr)) {
			break;
		}
		if (FBT_IS_ARM64_B_INSTR(theInstr)) {
			machine_inst_t *dest = instr + FBT_GET_ARM64_B_IMM(theInstr);
			  
                                                 
                                
      
			if (dest >= limit || dest < symbolStart) {
				break;
			}
		}
	}

	if (!FBT_IS_ARM64_RET(theInstr) && !FBT_IS_ARM64_B_INSTR(theInstr)) {
		return;
	}

	newfbt = kmem_zalloc(sizeof(fbt_probe_t), KM_SLEEP);
	newfbt->fbtp_next = NULL;
	strlcpy((char *)&(newfbt->fbtp_name), symbolName, MAX_FBTP_NAME_CHARS );

	if (retfbt == NULL) {
		newfbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
		    symbolName, FBT_RETURN, FBT_AFRAMES_RETURN, newfbt);
	} else {
		retfbt->fbtp_next = newfbt;
		newfbt->fbtp_id = retfbt->fbtp_id;
	}

	retfbt = newfbt;
	newfbt->fbtp_patchpoint = instr;
	newfbt->fbtp_ctl = ctl;
	newfbt->fbtp_loadcnt = ctl->mod_loadcnt;

	ASSERT(FBT_IS_ARM64_RET(theInstr) || FBT_IS_ARM64_B_INSTR(theInstr));
	newfbt->fbtp_rval = DTRACE_INVOP_RET;
	newfbt->fbtp_roffset = (uintptr_t) ((uint8_t*) instr - (uint8_t *)symbolStart);
	newfbt->fbtp_savedval = theInstr;
	newfbt->fbtp_patchval = FBT_PATCHVAL;
	newfbt->fbtp_currentval = 0;
	newfbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = newfbt;

	if (doenable) {
		fbt_enable(NULL, newfbt->fbtp_id, newfbt);
	}

	instr++;
	goto again;
}
