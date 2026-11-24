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
#include <arm/thread.h>

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

#define DTRACE_INVOP_PUSH_LR 8
#define DTRACE_INVOP_BL 9
#define DTRACE_INVOP_POP_PC 10

#define DTRACE_INVOP_THUMB_NOP_SKIP 2
#define DTRACE_INVOP_POP_PC_SKIP 2
#define DTRACE_INVOP_THUMB_SET_R7_SKIP 2
#define DTRACE_INVOP_THUMB_MOV_SP_TO_R7_SKIP 2

#define FBT_IS_THUMB_PUSH_LR(x)         (((x) & 0x0000ff00) == 0x0000b500)
#define FBT_IS_THUMB_POP_R7(x)          (((x) & 0x0000ff80) == 0x0000bc80)
#define FBT_IS_THUMB32_POP_R7LR(x, y)    (((x) == 0x0000e8bd) && (((y) & 0x00004080) == 0x00004080))
#define FBT_IS_THUMB_POP_PC(x)          (((x) & 0x0000ff00) == 0x0000bd00)
#define FBT_IS_THUMB_SET_R7(x)          (((x) & 0x0000ff00) == 0x0000af00)
#define FBT_IS_THUMB_MOV_SP_TO_R7(x)            (((x) & 0x0000ffff) == 0x0000466f)
#define FBT_THUMB_SET_R7_OFFSET(x)      (((x) & 0x000000ff) << 2)
#define FBT_IS_THUMB_LDR_PC(x)          (((x) & 0x0000f800) == 0x00004800)
#define FBT_IS_THUMB32_LDR_PC(x, y)      ((x) == 0x0000f8df)                                                                    
#define FBT_THUMB_STACK_REGS(x)         ((x) & 0x00FF)
#define FBT_IS_THUMB_BX_REG(x)          (((x) & 0x0000ff87) == 0x00004700)

#define FBT_PATCHVAL                    0xdefc
#define FBT_AFRAMES_ENTRY               8
#define FBT_AFRAMES_RETURN              6

#define FBT_ENTRY       "entry"
#define FBT_RETURN      "return"
#define FBT_ADDR2NDX(addr)      ((((uintptr_t)(addr)) >> 4) & fbt_probetab_mask)

#define VFPSAVE_ALIGN_DTRACE    16                                                     

extern dtrace_provider_id_t     fbt_id;
extern fbt_probe_t               **fbt_probetab;
extern int                      fbt_probetab_mask;

kern_return_t fbt_perfCallback(int, struct arm_saved_state *, __unused int, __unused int);

extern int dtrace_arm_condition_true(int cond, int cpsr);


                                                                             
                                                                                  

static uint32_t
thumb_ldr_pc_address(uint32_t address)
{
	return (address & 0xFFFFFFFC) + (*(uint16_t*) address & 0xFF) * 4 + 4;
}

static uint32_t
thumb32_ldr_pc_address(uint32_t address)
{
	return (address & 0xFFFFFFFC) + (*(uint16_t*) (address + 2) & 0xFFF) + 4;
}

                                               
static uint32_t
get_itstate(uint32_t cpsr)
{
	return
	        ((cpsr & 0x06000000) >> 25) |
	        ((cpsr & 0x0000FC00) >> 8);
}

static void
clear_itstate(uint32_t* cpsr)
{
	*cpsr &= ~0x0600FC00;
}

int
fbt_invop(uintptr_t addr, uintptr_t * stack, uintptr_t rval)
{
	fbt_probe_t    *fbt = fbt_probetab[FBT_ADDR2NDX(addr)];

	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
		if ((uintptr_t) fbt->fbtp_patchpoint == addr) {
			if (0 == CPU->cpu_dtrace_invop_underway) {
				CPU->cpu_dtrace_invop_underway = 1;                            
                                                                   

				struct arm_saved_state* regs = (struct arm_saved_state*) stack;
				uintptr_t stack4 = *((uintptr_t*) regs->sp);

				if ((regs->cpsr & PSR_MODE_MASK) == PSR_FIQ_MODE) {
					  
                                                                   
                                                            
                                                              
                                                   
        
					panic("dtrace: fbt: The probe at %08x was called from FIQ_MODE", (unsigned) addr);
				}

				  
                                                                                                    
                                                                                       
       
				uint32_t itstate = get_itstate(regs->cpsr);
				if ((itstate & 0x7) != 0) {
					panic("dtrace: fbt: Instruction stream error: Middle of IT block at %08x", (unsigned) addr);
				}

				if (fbt->fbtp_roffset == 0) {
					  
                                                                                              
                                                                                              
                                                                                              
                                                                        
       
                                                                                          
        
					uint32_t offset = ((uint32_t) regs) + sizeof(struct arm_saved_state);
#if __ARM_VFP__
					                                                               
					offset &= ~(VFPSAVE_ALIGN_DTRACE - 1);
					offset += VFPSAVE_ALIGN_DTRACE + sizeof(struct arm_vfpsaved_state);
#endif                  
					if (FBT_IS_THUMB_SET_R7(fbt->fbtp_savedval)) {
						*((uint32_t*) offset) = regs->sp + FBT_THUMB_SET_R7_OFFSET(fbt->fbtp_savedval);
					} else {
						*((uint32_t*) offset) = regs->sp;
					}

					CPU->cpu_dtrace_caller = regs->lr;
					dtrace_probe(fbt->fbtp_id, regs->r[0], regs->r[1], regs->r[2], regs->r[3], stack4);
					CPU->cpu_dtrace_caller = 0;
				} else {
					                                                         
					if (itstate != 0) {
						  
                                                                                
                                                                       
         
						int condition_it = (itstate & 0xF0) >> 4;

						if (dtrace_arm_condition_true(condition_it, regs->cpsr) == 0) {
							                                              
							clear_itstate(&regs->cpsr);
							CPU->cpu_dtrace_invop_underway = 0;
							return DTRACE_INVOP_NOP;
						}
					}

					dtrace_probe(fbt->fbtp_id, fbt->fbtp_roffset, rval, 0, 0, 0);
					CPU->cpu_dtrace_caller = 0;

					                                                                                      
					clear_itstate(&regs->cpsr);
				}
				CPU->cpu_dtrace_invop_underway = 0;
			}

			  
                                                                                            
                                                                                                 
                                                                                                  
      
			return fbt->fbtp_savedval;
		}
	}

	return 0;
}

#define IS_USER_TRAP(regs)  (((regs)->cpsr & PSR_MODE_MASK) == PSR_USER_MODE)
#define T_INVALID_OPCODE EXC_BAD_INSTRUCTION
#define FBT_EXCEPTION_CODE T_INVALID_OPCODE

kern_return_t
fbt_perfCallback(
	int trapno,
	struct arm_saved_state * regs,
	__unused int unused1,
	__unused int unused2)
{
#pragma unused (unused1)
#pragma unused (unused2)
	kern_return_t   retval = KERN_FAILURE;

	if (FBT_EXCEPTION_CODE == trapno && !IS_USER_TRAP(regs)) {
		boolean_t oldlevel = 0;
		machine_inst_t emul = 0;

		oldlevel = ml_set_interrupts_enabled(FALSE);

		__asm__ volatile (
                         "Ldtrace_invop_callsite_pre_label:\n"
                         ".data\n"
                         ".private_extern _dtrace_invop_callsite_pre\n"
                         "_dtrace_invop_callsite_pre:\n"
                         "  .long Ldtrace_invop_callsite_pre_label\n"
                         ".text\n"
                );

		emul = dtrace_invop(regs->pc, (uintptr_t*) regs, regs->r[0]);

		__asm__ volatile (
                         "Ldtrace_invop_callsite_post_label:\n"
                         ".data\n"
                         ".private_extern _dtrace_invop_callsite_post\n"
                         "_dtrace_invop_callsite_post:\n"
                         "  .long Ldtrace_invop_callsite_post_label\n"
                         ".text\n"
                );

		  
                                                                                      
                                                                                     
                                                                                      
                         
     
		uint32_t itstate = get_itstate(regs->cpsr);
		if (itstate != 0) {
			panic("dtrace: fbt: Not emulated: Middle of IT block at %08x", (unsigned) regs->pc);
		}

		if (emul == DTRACE_INVOP_NOP) {
			regs->pc += DTRACE_INVOP_THUMB_NOP_SKIP;
			retval = KERN_SUCCESS;
		} else if (FBT_IS_THUMB_SET_R7(emul)) {
			regs->r[7] = regs->sp + FBT_THUMB_SET_R7_OFFSET(emul);
			regs->pc += DTRACE_INVOP_THUMB_SET_R7_SKIP;
			retval = KERN_SUCCESS;
		} else if (FBT_IS_THUMB_MOV_SP_TO_R7(emul)) {
			regs->r[7] = regs->sp;
			regs->pc += DTRACE_INVOP_THUMB_MOV_SP_TO_R7_SKIP;
			retval = KERN_SUCCESS;
		} else if (FBT_IS_THUMB_POP_PC(emul)) {
			uintptr_t* sp = (uintptr_t*) regs->sp;

			machine_inst_t mask = 0x0001;
			int regnum = 0;
			while (mask & 0x00ff) {
				if (emul & mask) {
					                       
					regs->r[regnum] = *sp++;
				}
				mask <<= 1;
				regnum++;
			}

			regs->pc = *sp++;
			regs->sp = (uintptr_t) sp;
			if (regs->pc & 1) {
				regs->cpsr |= PSR_TF;
			} else {
				regs->cpsr &= ~PSR_TF;
			}

			retval = KERN_SUCCESS;
		} else if (FBT_IS_THUMB_BX_REG(emul)) {
			regs->pc = regs->r[(emul >> 3) & 0xF];

			if (regs->pc & 1) {
				regs->cpsr |= PSR_TF;
			} else {
				regs->cpsr &= ~PSR_TF;
			}

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
	unsigned int    j;
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
	for (j = 0, instr = symbolStart, theInstr = 0;
	    (j < 8) && instr < instrHigh; j++, instr++) {
		theInstr = *instr;
		if (FBT_IS_THUMB_PUSH_LR(theInstr)) {
			foundPushLR = 1;
			                                                                                 
			savedRegs = FBT_THUMB_STACK_REGS(theInstr);
			pushinstr = instr;
		}
		if (foundPushLR && (FBT_IS_THUMB_SET_R7(theInstr) || FBT_IS_THUMB_MOV_SP_TO_R7(theInstr))) {
			                                                                                        
			break;
		}
		if (FBT_IS_THUMB_BX_REG(theInstr)) {                                
			break;
		}
		if (FBT_IS_THUMB_POP_PC(theInstr)) {                                
			break;
		}

		                                        
		if (dtrace_instr_size(theInstr, 1) == 4) {
			instr++;
		}
	}

	if (!(foundPushLR && (FBT_IS_THUMB_SET_R7(theInstr) || FBT_IS_THUMB_MOV_SP_TO_R7(theInstr)))) {
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
	newfbt->fbtp_rval = DTRACE_INVOP_PUSH_LR;
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

	  
                                                              
                                                                   
                                                            
                                                           
                                                               
                                                            
                                                      
    
	if (((uintptr_t)instr & 0x3) == 0) {
		machine_inst_t *ptr = *(machine_inst_t **)(void *)instr;

		if (ptr >= (machine_inst_t *)symbolStart && ptr < limit) {
			                                                                                           
			instr++;
			goto again;
		}
	}

	  
                            
    
	theInstr = *instr;

	                                                                                   
	if (FBT_IS_THUMB_PUSH_LR(theInstr)) {
		if (!retfbt) {
			kprintf("dtrace: fbt: No return probe for %s, walked to next routine at %08x\n", symbolName, (unsigned)instr);
		}
		return;
	}

	                                                                          
                                                                             
                       
	if (FBT_IS_THUMB_LDR_PC(theInstr)) {
		uint32_t newlimit = thumb_ldr_pc_address((uint32_t) instr);
		if (newlimit < (uint32_t) limit) {
			limit = (machine_inst_t*) newlimit;
		}
	}
	if ((instr + 1) < limit && FBT_IS_THUMB32_LDR_PC(*instr, *(instr + 1))) {
		uint32_t newlimit = thumb32_ldr_pc_address((uint32_t) instr);
		if (newlimit < (uint32_t) limit) {
			limit = (machine_inst_t*) newlimit;
		}
	}

	                                                                                                                    
	if (!FBT_IS_THUMB_POP_PC(theInstr) &&
	    !FBT_IS_THUMB_POP_R7(theInstr) &&
	    !FBT_IS_THUMB32_POP_R7LR(theInstr, *(instr + 1))) {
		instr++;
		if (dtrace_instr_size(theInstr, 1) == 4) {
			instr++;
		}
		goto again;
	}

	if (FBT_IS_THUMB_POP_PC(theInstr)) {
		if (savedRegs != FBT_THUMB_STACK_REGS(theInstr)) {
			                                                                         
                                         
      
			kprintf("dtrace: fbt: No return probe for %s, popped regs don't match at %08x\n", symbolName, (unsigned)instr);
			return;
		}
	} else {
		                           
		for (j = 0; (j < 4) && (instr < limit); j++, instr++) {
			theInstr = *instr;
			if (FBT_IS_THUMB_BX_REG(theInstr)) {
				break;
			}
			if (dtrace_instr_size(theInstr, 1) == 4) {
				instr++;
			}
		}

		if (!FBT_IS_THUMB_BX_REG(theInstr)) {
			return;
		}
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

	ASSERT(FBT_IS_THUMB_POP_PC(theInstr) || FBT_IS_THUMB_BX_REG(theInstr));
	newfbt->fbtp_rval = DTRACE_INVOP_POP_PC;
	newfbt->fbtp_roffset =
	    (uintptr_t) ((uint8_t*) instr - (uint8_t *)symbolStart);
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
