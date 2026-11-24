/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
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

#include <sys/fasttrap_isa.h>
#include <sys/fasttrap_impl.h>
#include <sys/dtrace.h>
#include <sys/dtrace_impl.h>
#include <kern/task.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <mach/mach_vm.h>
#include <arm/proc_reg.h>
#include <arm/caches_internal.h>

#include <sys/dtrace_ptss.h>
#include <kern/debug.h>

#include <pexpert/pexpert.h>

extern dtrace_id_t dtrace_probeid_error;

                                                                       
#define proc_t struct proc                                                   

extern int dtrace_decode_arm(uint32_t instr);
extern int dtrace_decode_thumb(uint32_t instr);

  
                                    
                                    
  
                                                                           
                                                             
  
                            
  
                                                                            
                                                                             
                                                                            
                                                                        
                                     
  
                                                                       
                                                                           
                                                                             
                                                                          
                                                                           
                                                                           
                                     
  
   

#define THUMB_INSTR(x) (*(uint16_t*) &(x))

#define SIGNEXTEND(x, v) ((((int) (x)) << (32-(v))) >> (32-(v)))
#define ALIGNADDR(x, v) (((x) >> (v)) << (v))
#define GETITSTATE(x) ((((x) >> 8) & 0xFC) | (((x) >> 25) & 0x3))
#define ISLASTINIT(x) (((x) & 0xF) == 8)

#define SET16(x, w) *((uint16_t*) (x)) = (w)
#define SET32(x, w) *((uint32_t*) (x)) = (w)

#define IS_ARM_NOP(x) ((x) == 0xE1A00000)
                                  
#define IS_ARM_IS_ENABLED(x) ((x) == 0xE0200000)

#define IS_THUMB_NOP(x) ((x) == 0x46C0)
                                  
#define IS_THUMB_IS_ENABLED(x) ((x) == 0x4040)

#define ARM_LDM_UF (1 << 23)
#define ARM_LDM_PF (1 << 24)
#define ARM_LDM_WF (1 << 21)

#define ARM_LDR_UF (1 << 23)
#define ARM_LDR_BF (1 << 22)

extern int dtrace_arm_condition_true(int cond, int cpsr);

int
fasttrap_tracepoint_init(proc_t *p, fasttrap_tracepoint_t *tp,
    user_addr_t pc, fasttrap_probe_type_t type)
{
#pragma unused(type)
	uint32_t instr;

	  
                                                                  
                                                          
                                                                  
                                                                   
                                                                   
                                                                     
    
	  
                                                                          
    

	if (uread(p, &instr, 4, pc) != 0) {
		return -1;
	}

	                                                                            
                                                                         
                                                                   
    
	tp->ftt_instr = instr;

	if (tp->ftt_fntype != FASTTRAP_FN_DONE_INIT) {
		switch (tp->ftt_fntype) {
		case FASTTRAP_FN_UNKNOWN:
			                                                                                              
			return -1;

		case FASTTRAP_FN_USDT:
			if (IS_ARM_NOP(instr) || IS_ARM_IS_ENABLED(instr)) {
				tp->ftt_thumb = 0;
			} else if (IS_THUMB_NOP(THUMB_INSTR(instr)) || IS_THUMB_IS_ENABLED(THUMB_INSTR(instr))) {
				tp->ftt_thumb = 1;
			} else {
				                                                       
                                                         
       
				return -1;
			}
			tp->ftt_fntype = FASTTRAP_FN_DONE_INIT;
			break;

		case FASTTRAP_FN_ARM:
			tp->ftt_thumb = 0;
			tp->ftt_fntype = FASTTRAP_FN_DONE_INIT;
			break;

		case FASTTRAP_FN_THUMB:
			tp->ftt_thumb = 1;
			tp->ftt_fntype = FASTTRAP_FN_DONE_INIT;
			break;

		default:
			return -1;
		}
	}

	if (tp->ftt_thumb) {
		tp->ftt_type = dtrace_decode_thumb(instr);
	} else {
		tp->ftt_type = dtrace_decode_arm(instr);
	}

	if (tp->ftt_type == FASTTRAP_T_INV) {
		                                                                          
		printf("dtrace: fasttrap: Unrecognized instruction: %08x at %08x\n",
		    (tp->ftt_thumb && dtrace_instr_size(tp->ftt_instr, tp->ftt_thumb) == 2) ? tp->ftt_instr1 : instr, pc);
		return -1;
	}

	return 0;
}

int
fasttrap_tracepoint_install(proc_t *p, fasttrap_tracepoint_t *tp)
{
	                                                                                                
	uint32_t instr;
	int size = tp->ftt_thumb ? 2 : 4;

	if (tp->ftt_thumb) {
		*((uint16_t*) &instr) = FASTTRAP_THUMB_INSTR;
	} else {
		instr = FASTTRAP_ARM_INSTR;
	}

	if (uwrite(p, &instr, size, tp->ftt_pc) != 0) {
		return -1;
	}

	tp->ftt_installed = 1;

	return 0;
}

int
fasttrap_tracepoint_remove(proc_t *p, fasttrap_tracepoint_t *tp)
{
	                                                                                                
	uint32_t instr;
	int size = tp->ftt_thumb ? 2 : 4;

	  
                                                            
                
    
	if (uread(p, &instr, size, tp->ftt_pc) != 0) {
		goto end;
	}
	if (tp->ftt_thumb) {
		if (*((uint16_t*) &instr) != FASTTRAP_THUMB_INSTR) {
			goto end;
		}
	} else {
		if (instr != FASTTRAP_ARM_INSTR) {
			goto end;
		}
	}
	if (uwrite(p, &tp->ftt_instr, size, tp->ftt_pc) != 0) {
		return -1;
	}

end:
	tp->ftt_installed = 0;

	return 0;
}

static void
fasttrap_return_common(proc_t *p, arm_saved_state_t *regs, user_addr_t pc, user_addr_t new_pc)
{
	pid_t pid = p->p_pid;
	fasttrap_tracepoint_t *tp;
	fasttrap_bucket_t *bucket;
	fasttrap_id_t *id;
	lck_mtx_t *pid_mtx;
	int retire_tp = 1;

	pid_mtx = &cpu_core[CPU->cpu_id].cpuc_pid_lock;
	lck_mtx_lock(pid_mtx);
	bucket = &fasttrap_tpoints.fth_table[FASTTRAP_TPOINTS_INDEX(pid, pc)];

	for (tp = bucket->ftb_data; tp != NULL; tp = tp->ftt_next) {
		if (pid == tp->ftt_pid && pc == tp->ftt_pc &&
		    tp->ftt_proc->ftpc_acount != 0) {
			break;
		}
	}

	  
                                                                
                                                                   
                                                             
    
	if (tp == NULL) {
		lck_mtx_unlock(pid_mtx);
		return;
	}

	for (id = tp->ftt_retids; id != NULL; id = id->fti_next) {
		fasttrap_probe_t *probe = id->fti_probe;
		  
                                                            
                                                               
                              
     
		if (tp->ftt_type != FASTTRAP_T_LDM_PC &&
		    tp->ftt_type != FASTTRAP_T_POP_PC &&
		    new_pc - probe->ftp_faddr < probe->ftp_fsize) {
			continue;
		}

		if (probe->ftp_prov->ftp_provider_type == DTFTP_PROVIDER_ONESHOT) {
			if (os_atomic_xchg(&probe->ftp_triggered, 1, relaxed)) {
				                       
				continue;
			}
		}
		  
                                                  
                                             
               
     
		else {
			retire_tp = 0;
		}
#if defined(XNU_TARGET_OS_OSX)
		if (ISSET(current_proc()->p_lflag, P_LNOATTACH)) {
			dtrace_probe(dtrace_probeid_error, 0            , id->fti_probe->ftp_id,
			    1          , -1             , DTRACEFLT_UPRIV);
#else
		if (FALSE) {
#endif                                 
		} else {
			dtrace_probe(id->fti_probe->ftp_id,
			    pc - id->fti_probe->ftp_faddr,
			    regs->r[0], 0, 0, 0);
		}
	}
	if (retire_tp) {
		fasttrap_tracepoint_retire(p, tp);
	}

	lck_mtx_unlock(pid_mtx);
}

#if DEBUG
__dead2
#endif
static void
fasttrap_sigsegv(proc_t *p, uthread_t t, user_addr_t addr, arm_saved_state_t *regs)
{
	                                                                                
                                                                            
    
#if DEBUG
#pragma unused(p,t,addr,arm_saved_state)
	panic("fasttrap: sigsegv not yet implemented");
#else
#pragma unused(p,t,addr)
	                      
	regs->pc = 0;
#endif

#if 0
	proc_lock(p);

	                                       
	t->uu_code = addr;
	t->uu_siglist |= sigmask(SIGSEGV);

	  
                                                             
                                                          
                                                                
    
	t->uu_exception = KERN_INVALID_ADDRESS;                      
	t->uu_subcode = 0;                   

	proc_unlock(p);

	                  
	signal_setast(t->uu_context.vc_thread);
#endif
}

static void
fasttrap_usdt_args(fasttrap_probe_t *probe, arm_saved_state_t *regs, int argc,
    uint32_t *argv)
{
	int i, x, cap = MIN(argc, probe->ftp_nargs);

	for (i = 0; i < cap; i++) {
		x = probe->ftp_argmap[i];

		if (x < 4) {
			argv[i] = regs->r[x];
		} else {
			fasttrap_fuword32_noerr(regs->sp + (x - 4) * sizeof(uint32_t), &argv[i]);
		}
	}

	for (; i < argc; i++) {
		argv[i] = 0;
	}
}

static void
set_thumb_flag(arm_saved_state_t *regs, user_addr_t pc)
{
	if (pc & 1) {
		regs->cpsr |= PSR_TF;
	} else {
		regs->cpsr &= ~PSR_TF;
	}
}

int
fasttrap_pid_probe(arm_saved_state_t *regs)
{
	proc_t *p = current_proc();
	user_addr_t new_pc = 0;
	fasttrap_bucket_t *bucket;
	lck_mtx_t *pid_mtx;
	fasttrap_tracepoint_t *tp, tp_local;
	pid_t pid;
	dtrace_icookie_t cookie;
	uint_t is_enabled = 0;
	int instr_size;
	int was_simulated = 1, retire_tp = 1;

	user_addr_t pc = regs->pc;

	uthread_t uthread = (uthread_t) get_bsdthread_info(current_thread());

	  
                                                                   
                                                                      
                                                                   
                                        
    
	if (uthread->t_dtrace_step) {
		ASSERT(uthread->t_dtrace_on);
		fasttrap_sigtrap(p, uthread, pc);
		return 0;
	}

	  
                                 
    
	uthread->t_dtrace_ft = 0;
	uthread->t_dtrace_pc = 0;
	uthread->t_dtrace_npc = 0;
	uthread->t_dtrace_scrpc = 0;
	uthread->t_dtrace_astpc = 0;

	  
                                                                 
                                                                     
                      
    
	if (p->p_lflag & P_LINVFORK) {
		proc_list_lock();
		while (p->p_lflag & P_LINVFORK) {
			p = p->p_pptr;
		}
		proc_list_unlock();
	}

	pid = p->p_pid;
	pid_mtx = &cpu_core[CPU->cpu_id].cpuc_pid_lock;
	lck_mtx_lock(pid_mtx);
	bucket = &fasttrap_tpoints.fth_table[FASTTRAP_TPOINTS_INDEX(pid, pc)];

	  
                                                    
    
	for (tp = bucket->ftb_data; tp != NULL; tp = tp->ftt_next) {
		if (pid == tp->ftt_pid && pc == tp->ftt_pc &&
		    tp->ftt_proc->ftpc_acount != 0) {
			break;
		}
	}

	  
                                                                      
                                                                 
                                                                
    
	if (tp == NULL) {
		lck_mtx_unlock(pid_mtx);
		return -1;
	}

	                               
	int condition_code = 0xE;
	if (tp->ftt_thumb) {
		uint32_t itstate = GETITSTATE(regs->cpsr);
		if (itstate != 0) {
			                                                                 
			if (ISLASTINIT(itstate)) {
				condition_code = itstate >> 4;
			} else {
				printf("dtrace: fasttrap: Tried to trace instruction %08x at %08x but not at end of IT block\n",
				    (tp->ftt_thumb && dtrace_instr_size(tp->ftt_instr, tp->ftt_thumb) == 2) ? tp->ftt_instr1 : tp->ftt_instr, pc);

				fasttrap_tracepoint_remove(p, tp);
				lck_mtx_unlock(pid_mtx);
				return -1;
			}
		}
	} else {
		condition_code = ARM_CONDCODE(tp->ftt_instr);
	}

	if (!tp->ftt_thumb != !(regs->cpsr & PSR_TF)) {
		                                                                     
                                
     
		fasttrap_tracepoint_remove(p, tp);
		lck_mtx_unlock(pid_mtx);
		return -1;
	}

	if (tp->ftt_ids != NULL) {
		fasttrap_id_t *id;

		uint32_t s4;
		uint32_t *stack = (uint32_t *)regs->sp;

		                                                   
		fasttrap_fuword32_noerr((user_addr_t)(uint32_t)stack, &s4);

		for (id = tp->ftt_ids; id != NULL; id = id->fti_next) {
			fasttrap_probe_t *probe = id->fti_probe;

#if defined(XNU_TARGET_OS_OSX)
			if (ISSET(current_proc()->p_lflag, P_LNOATTACH)) {
				dtrace_probe(dtrace_probeid_error, 0            , probe->ftp_id,
				    1          , -1             , DTRACEFLT_UPRIV);
#else
			if (FALSE) {
#endif                                 
			} else {
				if (probe->ftp_prov->ftp_provider_type == DTFTP_PROVIDER_ONESHOT) {
					if (os_atomic_xchg(&probe->ftp_triggered, 1, relaxed)) {
						                       
						continue;
					}
				}
				  
                                                
                                               
                 
       
				else {
					retire_tp = 0;
				}
				if (id->fti_ptype == DTFTP_ENTRY) {
					  
                                      
                                       
                     
        
					cookie = dtrace_interrupt_disable();
					DTRACE_CPUFLAG_SET(CPU_DTRACE_ENTRY);
					dtrace_probe(probe->ftp_id, regs->r[0], regs->r[1], regs->r[2], regs->r[3], s4);
					DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_ENTRY);
					dtrace_interrupt_enable(cookie);
				} else if (id->fti_ptype == DTFTP_IS_ENABLED) {
					  
                                        
                                           
                                           
                                      
                                  
        
					is_enabled = 1;
				} else if (probe->ftp_argmap == NULL) {
					dtrace_probe(probe->ftp_id, regs->r[0], regs->r[1], regs->r[2], regs->r[3], s4);
				} else {
					uint32_t t[5];

					fasttrap_usdt_args(probe, regs, 5, t);
					dtrace_probe(probe->ftp_id, t[0], t[1], t[2], t[3], t[4]);
				}
			}
		}
		if (retire_tp) {
			fasttrap_tracepoint_retire(p, tp);
		}
	}
	  
                                                                 
                                                                
                                                                    
    
	tp_local = *tp;
	lck_mtx_unlock(pid_mtx);
	tp = &tp_local;

	  
                                                                  
                                         
                                                                   
                                                                  
                                                           
                                                                      
                                                                 
                                                                   
                                            
    

	if (is_enabled) {
		regs->r[0] = 1;
		new_pc = regs->pc + (tp->ftt_thumb ? 2 : 4);
		goto done;
	}

	                                                                             
	if ((tp->ftt_thumb && IS_THUMB_NOP(THUMB_INSTR(tp->ftt_instr))) ||
	    (!tp->ftt_thumb && IS_ARM_NOP(tp->ftt_instr))) {
		new_pc = regs->pc + (tp->ftt_thumb ? 2 : 4);
		goto done;
	}

	instr_size = dtrace_instr_size(tp->ftt_instr, tp->ftt_thumb);

	switch (tp->ftt_type) {
	case FASTTRAP_T_MOV_PC_REG:
	case FASTTRAP_T_CPY_PC:
	{
		if (!dtrace_arm_condition_true(condition_code, regs->cpsr)) {
			new_pc = pc + instr_size;
			break;
		}

		int rm;
		if (tp->ftt_thumb) {
			rm = THUMB16_HRM(tp->ftt_instr1);
		} else {
			rm = tp->ftt_instr & 0xF;
		}
		new_pc = regs->r[rm];

		                                                      

		break;
	}

	case FASTTRAP_T_STM_LR:
	case FASTTRAP_T_PUSH_LR:
	{
		  
                                                                          
                                                                             
                                                                              
                                                                              
                                                                              
                                                                             
                                          
    
                                                                           
     

		int reglist;
		int ret;
		uintptr_t* base;

		if (!dtrace_arm_condition_true(condition_code, regs->cpsr)) {
			new_pc = pc + instr_size;
			break;
		}

		base = (uintptr_t*) regs->sp;
		if (((((uintptr_t) base) - 16 * 4) >> PAGE_SHIFT) != (((uintptr_t) base) >> PAGE_SHIFT)) {
			                                                
			goto instr_emulate;
		}

		if (tp->ftt_thumb) {
			if (instr_size == 4) {
				                                                     
				reglist = tp->ftt_instr2 & 0x1FFF;
			} else {
				reglist = tp->ftt_instr1 & 0xFF;
			}
		} else {
			                                                     
			reglist = tp->ftt_instr & 0x1FFF;
		}

		                            
		base--;
		ret = fasttrap_suword32((uint32_t) base, regs->lr);
		if (ret == -1) {
			fasttrap_sigsegv(p, uthread, (user_addr_t) base, regs);
			new_pc = regs->pc;
			break;
		}

		                             
		int regmask = 1 << 12;
		int regnum = 12;

		while (regmask) {
			if (reglist & regmask) {
				base--;
				ret = fasttrap_suword32((uint32_t) base, regs->r[regnum]);
				if (ret == -1) {
					fasttrap_sigsegv(p, uthread, (user_addr_t) base, regs);
					new_pc = regs->pc;
					break;
				}
			}
			regmask >>= 1;
			regnum--;
		}

		regs->sp = (uintptr_t) base;

		new_pc = pc + instr_size;

		break;
	}


	case FASTTRAP_T_LDM_PC:
	case FASTTRAP_T_POP_PC:
	{
		                                                            
    
                                                                                
                                                                              
                                                                                
                                                                  
    
                                                                            
                                                                            
                                                                               
                                                   
    
                                           
                                                                            
                                                                                    
    
     

		                                 

		int regnum = 0, reglist;
		int ret;
		uintptr_t* base;

		if (!dtrace_arm_condition_true(condition_code, regs->cpsr)) {
			new_pc = pc + instr_size;
			break;
		}

		if (tp->ftt_thumb) {
			if (instr_size == 4) {
				                                                       
				reglist = tp->ftt_instr2 & 0x7FFF;
			} else {
				reglist = tp->ftt_instr1 & 0xFF;
			}
		} else {
			                                                       
			reglist = tp->ftt_instr & 0x7FFF;
		}

		base = (uintptr_t*) regs->sp;
		while (reglist) {
			if (reglist & 1) {
				ret = fasttrap_fuword32((uint32_t) base, &regs->r[regnum]);
				if (ret == -1) {
					fasttrap_sigsegv(p, uthread, (user_addr_t) base, regs);
					new_pc = regs->pc;
					break;
				}
				base++;
			}
			reglist >>= 1;
			regnum++;
		}

		ret = fasttrap_fuword32((uint32_t) base, &new_pc);
		if (ret == -1) {
			fasttrap_sigsegv(p, uthread, (user_addr_t) base, regs);
			new_pc = regs->pc;
			break;
		}
		base++;

		regs->sp = (uintptr_t) base;

		set_thumb_flag(regs, new_pc);

		break;
	}

	case FASTTRAP_T_CB_N_Z:
	{
		                                                                                             
		int rn = tp->ftt_instr1 & 0x7;
		int offset = (((tp->ftt_instr1 & 0x00F8) >> 2) | ((tp->ftt_instr1 & 0x0200) >> 3)) + 4;
		int nonzero = tp->ftt_instr1 & 0x0800;
		if (!nonzero != !(regs->r[rn] == 0)) {
			new_pc = pc + offset;
		} else {
			new_pc = pc + instr_size;
		}
		break;
	}

	case FASTTRAP_T_B_COND:
	{
		                                                                      

		int code, offset;
		if (tp->ftt_thumb) {
			if (instr_size == 4) {
				code = (tp->ftt_instr1 >> 6) & 0xF;
				if (code == 14 || code == 15) {
					panic("fasttrap: Emulation of invalid branch");
				}
				int S = (tp->ftt_instr1 >> 10) & 1,
				    J1 = (tp->ftt_instr2 >> 13) & 1,
				    J2 = (tp->ftt_instr2 >> 11) & 1;
				offset = 4 + SIGNEXTEND(
					(S << 20) | (J2 << 19) | (J1 << 18) |
					((tp->ftt_instr1 & 0x003F) << 12) |
					((tp->ftt_instr2 & 0x07FF) << 1),
					21);
			} else {
				code = (tp->ftt_instr1 >> 8) & 0xF;
				if (code == 14 || code == 15) {
					panic("fasttrap: Emulation of invalid branch");
				}
				offset = 4 + (SIGNEXTEND(tp->ftt_instr1 & 0xFF, 8) << 1);
			}
		} else {
			code = ARM_CONDCODE(tp->ftt_instr);
			if (code == 15) {
				panic("fasttrap: Emulation of invalid branch");
			}
			offset = 8 + (SIGNEXTEND(tp->ftt_instr & 0x00FFFFFF, 24) << 2);
		}

		if (dtrace_arm_condition_true(code, regs->cpsr)) {
			new_pc = pc + offset;
		} else {
			new_pc = pc + instr_size;
		}

		break;
	}

	case FASTTRAP_T_B_UNCOND:
	{
		int offset;

		                                                              
		                                                                         
		ASSERT(tp->ftt_thumb == 1);

		if (!dtrace_arm_condition_true(condition_code, regs->cpsr)) {
			new_pc = pc + instr_size;
			break;
		}

		if (instr_size == 4) {
			int S = (tp->ftt_instr1 >> 10) & 1,
			    J1 = (tp->ftt_instr2 >> 13) & 1,
			    J2 = (tp->ftt_instr2 >> 11) & 1;
			int I1 = (J1 != S) ? 0 : 1, I2 = (J2 != S) ? 0 : 1;
			offset = 4 + SIGNEXTEND(
				(S << 24) | (I1 << 23) | (I2 << 22) |
				((tp->ftt_instr1 & 0x03FF) << 12) |
				((tp->ftt_instr2 & 0x07FF) << 1),
				25);
		} else {
			uint32_t instr1 = tp->ftt_instr1;
			offset = 4 + (SIGNEXTEND(instr1 & 0x7FF, 11) << 1);
		}

		new_pc = pc + offset;

		break;
	}

	case FASTTRAP_T_BX_REG:
	{
		int reg;

		if (!dtrace_arm_condition_true(condition_code, regs->cpsr)) {
			new_pc = pc + instr_size;
			break;
		}

		if (tp->ftt_thumb) {
			reg = THUMB16_HRM(tp->ftt_instr1);
		} else {
			reg = ARM_RM(tp->ftt_instr);
		}
		new_pc = regs->r[reg];
		set_thumb_flag(regs, new_pc);

		break;
	}

	case FASTTRAP_T_LDR_PC_IMMED:
	case FASTTRAP_T_VLDR_PC_IMMED:
		                                                                                
                                                                                   
                                                       
     

		                  

instr_emulate:
	case FASTTRAP_T_COMMON:
	{
		user_addr_t addr;
		uint8_t scratch[32];
		uint_t i = 0;
		fasttrap_instr_t emul_instr;
		emul_instr.instr32 = tp->ftt_instr;
		int emul_instr_size;

		  
                                                                                    
                                                                                
                                                                                  
                                                                               
                            
     
		uint8_t emul_thumb = tp->ftt_thumb;
		int save_reg = -1;
		uint32_t save_val = 0;

		  
                                                
                                                                                        
                                                                                           
                                                                                         
                                                                                        
                                                                                        
                                                                                         
                                                                                      
                                                   
     
		if (tp->ftt_type == FASTTRAP_T_LDR_PC_IMMED) {
			                                                                       
                                                               
      
			int new_reg;
			if (tp->ftt_thumb) {
				                                     
				if (instr_size == 2) {
					  
                                                                
                                                                   
                                                                  
                                                                     
        
					if (!dtrace_arm_condition_true(condition_code, regs->cpsr)) {
						new_pc = pc + instr_size;
						break;
					}

					new_reg = (tp->ftt_instr1 >> 8) & 0x7;
					regs->r[new_reg] = ALIGNADDR(regs->pc + 4, 2);
					emul_thumb = 0;
					emul_instr.instr32 = 0xE5900000 | (new_reg << 16) | (new_reg << 12) | ((tp->ftt_instr1 & 0xFF) << 2);
				} else {
					                                        
					new_reg = (tp->ftt_instr2 >> 12) & 0xF;
					regs->r[new_reg] = ALIGNADDR(regs->pc + 4, 2);
					emul_instr.instr16.instr1 &= ~0x000F;
					emul_instr.instr16.instr1 |= new_reg;
				}
			} else {
				                                     
				new_reg = (tp->ftt_instr >> 12) & 0xF;
				regs->r[new_reg] = ALIGNADDR(regs->pc + 8, 2);
				emul_instr.instr32 &= ~0x000F0000;
				emul_instr.instr32 |= new_reg << 16;
			}
		} else if (tp->ftt_type == FASTTRAP_T_VLDR_PC_IMMED) {
			                                                                      
                                                          
      
			save_reg = 0;
			save_val = regs->r[0];
			regs->r[save_reg] = ALIGNADDR(regs->pc + (tp->ftt_thumb ? 4 : 8), 2);
			if (tp->ftt_thumb) {
				emul_instr.instr16.instr1 &= ~0x000F;
			} else {
				emul_instr.instr32 &= ~0x000F0000;
			}
		}

		emul_instr_size = dtrace_instr_size(emul_instr.instr32, emul_thumb);

		  
                   
                                                         
                                            
                                                                            
                                                      
     

		addr = uthread->t_dtrace_scratch->addr;

		if (addr == 0LL) {
			fasttrap_sigtrap(p, uthread, pc);                                         
			new_pc = pc;
			break;
		}

		uthread->t_dtrace_scrpc = addr;
		if (emul_thumb) {
			  
                                                                           
                                                                          
                                                                               
                                                                             
                                 
     
                                             
                            
                                                                                 
                       
                        
                        
                      
                                                        
                                                        
      

			bcopy(&emul_instr, &scratch[i], emul_instr_size); i += emul_instr_size;

			if (save_reg != -1) {
				uint16_t restore_inst = 0x4803;
				restore_inst |= (save_reg & 0x7) << 8;
				SET16(scratch + i, restore_inst); i += 2;                                     
			}

			SET16(scratch + i, 0xB403); i += 2;                                               
			SET16(scratch + i, 0x4801); i += 2;                                                
			SET16(scratch + i, 0x9001); i += 2;                                                
			SET16(scratch + i, 0xBD01); i += 2;                                              

			if (i % 4) {
				SET16(scratch + i, 0); i += 2;                                                                         
			}
			SET32(scratch + i, pc + instr_size + (tp->ftt_thumb ? 1 : 0)); i += 4;                           
			if (save_reg != -1) {
				SET32(scratch + i, save_val); i += 4;                                                       
			}

			uthread->t_dtrace_astpc = addr + i;
			bcopy(&emul_instr, &scratch[i], emul_instr_size); i += emul_instr_size;
			SET16(scratch + i, FASTTRAP_THUMB_RET_INSTR); i += 2;
		} else {
			  
                                           
                            
                                                                                 
                        
                                                        
                                                        
      

			bcopy(&emul_instr, &scratch[i], emul_instr_size); i += emul_instr_size;

			if (save_reg != -1) {
				uint32_t restore_inst = 0xE59F0004;
				restore_inst |= save_reg << 12;
				SET32(scratch + i, restore_inst); i += 4;                                    
			}
			SET32(scratch + i, 0xE51FF004); i += 4;                                            

			SET32(scratch + i, pc + instr_size + (tp->ftt_thumb ? 1 : 0)); i += 4;                           
			if (save_reg != -1) {
				SET32(scratch + i, save_val); i += 4;                                                       
			}

			uthread->t_dtrace_astpc = addr + i;
			bcopy(&emul_instr, &scratch[i], emul_instr_size); i += emul_instr_size;
			SET32(scratch + i, FASTTRAP_ARM_RET_INSTR); i += 4;
		}

		if (uwrite(p, scratch, i, uthread->t_dtrace_scratch->write_addr) != KERN_SUCCESS) {
			fasttrap_sigtrap(p, uthread, pc);
			new_pc = pc;
			break;
		}

		if (tp->ftt_retids != NULL) {
			uthread->t_dtrace_step = 1;
			uthread->t_dtrace_ret = 1;
			new_pc = uthread->t_dtrace_astpc + (emul_thumb ? 1 : 0);
		} else {
			new_pc = uthread->t_dtrace_scrpc + (emul_thumb ? 1 : 0);
		}

		uthread->t_dtrace_pc = pc;
		uthread->t_dtrace_npc = pc + instr_size;
		uthread->t_dtrace_on = 1;
		was_simulated = 0;
		set_thumb_flag(regs, new_pc);
		break;
	}

	default:
		panic("fasttrap: mishandled an instruction");
	}

done:
	  
               
   
                                                                    
                                                                  
                                                                    
                                                               
    
	regs->pc = new_pc;

	  
                                                                      
                                                                     
                                                                    
                
    
	if (tp->ftt_retids != NULL) {
		  
                                                             
                                                        
                                              
                                                             
                                                              
     
		  
                                                                
                                                               
                                                                   
                                                     
     
		if (was_simulated) {
			fasttrap_return_common(p, regs, pc, new_pc);
		} else {
			ASSERT(uthread->t_dtrace_ret != 0);
			ASSERT(uthread->t_dtrace_pc == pc);
			ASSERT(uthread->t_dtrace_scrpc != 0);
			ASSERT(new_pc == uthread->t_dtrace_astpc);
		}
	}

	return 0;
}

int
fasttrap_return_probe(arm_saved_state_t *regs)
{
	proc_t *p = current_proc();
	uthread_t uthread = (uthread_t)get_bsdthread_info(current_thread());
	user_addr_t pc = uthread->t_dtrace_pc;
	user_addr_t npc = uthread->t_dtrace_npc;

	uthread->t_dtrace_pc = 0;
	uthread->t_dtrace_npc = 0;
	uthread->t_dtrace_scrpc = 0;
	uthread->t_dtrace_astpc = 0;

	  
                                                                 
                                                                     
                      
    
	if (p->p_lflag & P_LINVFORK) {
		proc_list_lock();
		while (p->p_lflag & P_LINVFORK) {
			p = p->p_pptr;
		}
		proc_list_unlock();
	}

	  
                                                               
                                                                
                                                             
                                                           
                                           
    
	regs->pc = pc;

	fasttrap_return_common(p, regs, pc, npc);

	return 0;
}

uint64_t
fasttrap_pid_getarg(void *arg, dtrace_id_t id, void *parg, int argno,
    int aframes)
{
#pragma unused(arg, id, parg, aframes)
	arm_saved_state_t* regs = find_user_regs(current_thread());

	                                           
	if (argno < 4) {
		return regs->r[argno];
	}

	                                    
	uint32_t value;
	uint32_t* sp = (uint32_t*) regs->sp;
	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	value = dtrace_fuword32((user_addr_t) (sp + argno - 4));
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT | CPU_DTRACE_BADADDR);

	return value;
}

uint64_t
fasttrap_usdt_getarg(void *arg, dtrace_id_t id, void *parg, int argno, int aframes)
{
#pragma unused(arg, id, parg, argno, aframes)
#if 0
	return fasttrap_anarg(ttolwp(curthread)->lwp_regs, 0, argno);
#endif

	return 0;
}
