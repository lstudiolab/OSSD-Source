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

#include <sys/fasttrap_isa.h>
#include <sys/fasttrap_impl.h>
#include <sys/dtrace.h>
#include <sys/dtrace_impl.h>
#include <kern/task.h>
#include <arm/thread.h>

#include <sys/dtrace_ptss.h>

#if __has_include(<ptrauth.h>)
#include <ptrauth.h>
#endif

extern dtrace_id_t dtrace_probeid_error;

                                                                       
#define proc_t struct proc                                                   

extern uint8_t dtrace_decode_arm64(uint32_t instr);

#define IS_ARM64_NOP(x) ((x) == 0xD503201F)
                                  
#define IS_ARM64_IS_ENABLED(x) ((x) == 0xD2800000)

int
fasttrap_tracepoint_init(proc_t *p, fasttrap_tracepoint_t *tp,
    user_addr_t pc, fasttrap_probe_type_t type)
{
#pragma unused(type)
	uint32_t instr = 0;

	  
                                                                  
                                                          
                                                                  
                                                                   
                                                                   
                                                                     
    
	  
                                                                          
    

	if (uread(p, &instr, 4, pc) != 0) {
		return -1;
	}

	tp->ftt_instr = instr;

	if (tp->ftt_fntype != FASTTRAP_FN_DONE_INIT) {
		switch (tp->ftt_fntype) {
		case FASTTRAP_FN_UNKNOWN:
		case FASTTRAP_FN_ARM64:
		case FASTTRAP_FN_ARM64_32:
			  
                                              
                                           
      
			tp->ftt_fntype = FASTTRAP_FN_DONE_INIT;
			break;

		case FASTTRAP_FN_USDT:
			if (IS_ARM64_NOP(instr) || IS_ARM64_IS_ENABLED(instr)) {
				tp->ftt_fntype = FASTTRAP_FN_DONE_INIT;
			} else {
				  
                                                 
                                              
                           
       
				return -1;
			}

			break;

		case FASTTRAP_FN_ARM:
		case FASTTRAP_FN_THUMB:
		default:
			  
                                         
                                            
      
			return -1;
		}
	}

	tp->ftt_type = dtrace_decode_arm64(instr);

	if (tp->ftt_type == FASTTRAP_T_ARM64_EXCLUSIVE_MEM) {
		kprintf("Detected attempt to place DTrace probe on exclusive memory instruction (pc = 0x%llx); refusing to trace (or exclusive operation could never succeed).\n", pc);
		tp->ftt_type = FASTTRAP_T_INV;
		return -1;
	}

	if (tp->ftt_type == FASTTRAP_T_INV) {
		                                                                          
		printf("dtrace: fasttrap init64: Unrecognized instruction: %08x at %08llx\n", instr, pc);
		return -1;
	}

	return 0;
}

int
fasttrap_tracepoint_install(proc_t *p, fasttrap_tracepoint_t *tp)
{
	uint32_t instr;
	int size;

	if (proc_is64bit_data(p)) {
		size = 4;
		instr = FASTTRAP_ARM64_INSTR;
	} else {
		return -1;
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
	int size = 4;

	if (proc_is64bit_data(p)) {
		  
                                                             
                 
     
		if (uread(p, &instr, size, tp->ftt_pc) != 0) {
			goto end;
		}

		if (instr != FASTTRAP_ARM64_INSTR) {
			goto end;
		}
	} else {
		return -1;
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
		                                            
		if ((tp->ftt_type != FASTTRAP_T_ARM64_RET || tp->ftt_type != FASTTRAP_T_ARM64_RETAB) &&
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
			dtrace_probe(probe->ftp_id,
			    pc - id->fti_probe->ftp_faddr,
			    saved_state64(regs)->x[0], 0, 0, 0);
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
	                      
	set_saved_state_pc(regs, 0);
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
fasttrap_usdt_args64(fasttrap_probe_t *probe, arm_saved_state64_t *regs64, int argc,
    uint64_t *argv)
{
	int i, x, cap = MIN(argc, probe->ftp_nargs);

	for (i = 0; i < cap; i++) {
		x = probe->ftp_argmap[i];

		                                                   
		if (x < 8) {
			argv[i] = regs64->x[x];
		} else {
			fasttrap_fuword64_noerr(regs64->sp + (x - 8) * sizeof(uint64_t), &argv[i]);
		}
	}

	for (; i < argc; i++) {
		argv[i] = 0;
	}
}

static int
condition_true(int cond, int cpsr)
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

  
                                                     
                                                                              
                                                                 
                                                                                   
                                                                                       
          
  
                                      
                                                                    
  
                                                                                            
                                               
   
static void
fasttrap_pid_probe_thunk_instr64(arm_saved_state_t *state, fasttrap_tracepoint_t *tp, proc_t *p, uthread_t uthread,
    const uint32_t *instructions, uint32_t num_instrs, user_addr_t *pc_out)
{
	uint32_t local_scratch[8];
	user_addr_t pc = get_saved_state_pc(state);
	user_addr_t user_scratch_area;

	assert(num_instrs < 8);

	bcopy(instructions, local_scratch, num_instrs * sizeof(uint32_t));
	local_scratch[num_instrs] = FASTTRAP_ARM64_RET_INSTR;

	uthread->t_dtrace_astpc = uthread->t_dtrace_scrpc = uthread->t_dtrace_scratch->addr;
	user_scratch_area = uthread->t_dtrace_scratch->write_addr;

	if (user_scratch_area == (user_addr_t)0) {
		fasttrap_sigtrap(p, uthread, pc);                                 
		*pc_out = pc;
		return;
	}

	if (uwrite(p, local_scratch, (num_instrs + 1) * sizeof(uint32_t), user_scratch_area) != KERN_SUCCESS) {
		fasttrap_sigtrap(p, uthread, pc);
		*pc_out = pc;
		return;
	}

	                                                                                   
	uthread->t_dtrace_step = 1;

	                                                                                    
	uthread->t_dtrace_ret = (tp->ftt_retids != NULL);
	assert(tp->ftt_type != FASTTRAP_T_ARM64_RET);
	assert(tp->ftt_type != FASTTRAP_T_ARM64_RETAB);

	                                              
	uthread->t_dtrace_pc = pc;

	                                                                        
	uthread->t_dtrace_npc = pc + 4;

	                                       
	uthread->t_dtrace_on = 1;

	                                    
	*pc_out = uthread->t_dtrace_scratch->addr;
}

  
                                                  
   
static int64_t
sign_extend(int64_t input, uint32_t sign_bit_index)
{
	assert(sign_bit_index < 63);
	if (input & (1ULL << sign_bit_index)) {
		                                         
		input |= ((~0ULL) & ~((1ULL << (sign_bit_index + 1)) - 1ULL));
	}

	return input;
}

  
                                                           
   
static uint64_t
get_saved_state64_regno(arm_saved_state64_t *regs64, uint32_t regno, int use_xzr)
{
	                              
	switch (regno) {
	case 29:
		return regs64->fp;
	case 30:
		return regs64->lr;
	case 31:
		         
		if (use_xzr) {
			return 0;
		} else {
			return regs64->sp;
		}
	default:
		return regs64->x[regno];
	}
}

static void
set_saved_state64_regno(arm_saved_state64_t *regs64, uint32_t regno, int use_xzr, register_t value)
{
	                              
	switch (regno) {
	case 29:
		regs64->fp = value;
		break;
	case 30:
		regs64->lr = value;
		break;
	case 31:
		if (!use_xzr) {
			regs64->sp = value;
		}
		break;
	default:
		regs64->x[regno] = value;
		break;
	}
}

  
                                                                     
                                  
   
static uint64_t
extract_address_literal_sign_extended(uint32_t instr, uint32_t base, uint32_t numbits)
{
	uint64_t offset;

	offset = (instr >> base) & ((1 << numbits) - 1);
	offset = sign_extend(offset, numbits - 1);
	offset = offset << 2;

	return offset;
}

static void
do_cbz_cnbz(arm_saved_state64_t *regs64, uint32_t regwidth, uint32_t instr, int is_cbz, user_addr_t *pc_out)
{
	uint32_t regno;
	uint64_t regval;
	uint64_t offset;

	                      
	regno = (instr & 0x1f);
	assert(regno <= 31);
	regval = get_saved_state64_regno(regs64, regno, 1);

	                      
	if (regwidth == 32) {
		regval &= 0xFFFFFFFFULL;
	}

	                    
	offset = extract_address_literal_sign_extended(instr, 5, 19);

	             
	if ((is_cbz && regval == 0) || ((!is_cbz) && regval != 0)) {
		                       
		*pc_out = regs64->pc + offset;
	} else {
		                
		*pc_out = regs64->pc + 4;
	}
}

static void
do_tbz_tbnz(arm_saved_state64_t *regs64, uint32_t instr, int is_tbz, user_addr_t *pc_out)
{
	uint64_t offset, regval;
	uint32_t bit_index, b5, b40, regno, bit_set;

	                    
	offset = extract_address_literal_sign_extended(instr, 5, 14);

	                       
	b5 = (instr >> 31);
	b40 = ((instr >> 19) & 0x1f);
	bit_index = (b5 << 5) | b40;
	assert(bit_index <= 63);

	                      
	regno = (instr & 0x1f);
	assert(regno <= 31);
	regval = get_saved_state64_regno(regs64, regno, 1);

	              
	bit_set = ((regval & (1 << bit_index)) != 0);

	if ((is_tbz && (!bit_set)) || ((!is_tbz) && bit_set)) {
		                                                   
		*pc_out = regs64->pc + offset;
	} else {
		                
		*pc_out = regs64->pc + 4;
	}
}


static void
fasttrap_pid_probe_handle_patched_instr64(arm_saved_state_t *state, fasttrap_tracepoint_t *tp __unused, uthread_t uthread,
    proc_t *p, uint_t is_enabled, int *was_simulated)
{
	int res1, res2;
	arm_saved_state64_t *regs64 = saved_state64(state);
	uint32_t instr = tp->ftt_instr;
	user_addr_t new_pc = 0;

	                                                                                                 
	arm_neon_saved_state64_t *ns64 = &(get_user_neon_regs(uthread->uu_thread)->ns_64);

	                                                     
	if (is_enabled) {
		regs64->x[0] = 1;
		set_saved_state_pc(state, regs64->pc + 4);
		return;
	}

	                                                                             
	if (IS_ARM64_NOP(tp->ftt_instr)) {
		set_saved_state_pc(state, regs64->pc + 4);
		return;
	}


	                                                           
	switch (tp->ftt_type) {
	  
                                      
                           
    
	case FASTTRAP_T_ARM64_STANDARD_FUNCTION_ENTRY:
	{
		                           
		res1 = fasttrap_suword64(regs64->sp - 16, regs64->fp);
		res2 = fasttrap_suword64(regs64->sp - 8, regs64->lr);
		if (res1 != 0 || res2 != 0) {
			fasttrap_sigsegv(p, uthread, regs64->sp - (res1 ? 16 : 8), state);
#ifndef DEBUG
			new_pc = regs64->pc;                            
			break;
#endif
		}

		                        
		regs64->sp -= 16;

		                     
		new_pc = regs64->pc + 4;
		*was_simulated = 1;
		break;
	}

	  
                                                      
                                                                                  
                
                
                  
   
                     
                     
                     
                                    
    
	case FASTTRAP_T_ARM64_LDR_S_PC_REL:
	case FASTTRAP_T_ARM64_LDR_W_PC_REL:
	case FASTTRAP_T_ARM64_LDR_D_PC_REL:
	case FASTTRAP_T_ARM64_LDR_X_PC_REL:
	case FASTTRAP_T_ARM64_LDR_Q_PC_REL:
	case FASTTRAP_T_ARM64_LDRSW_PC_REL:
	{
		uint64_t offset;
		uint32_t valsize, regno;
		user_addr_t address;
		union {
			uint32_t val32;
			uint64_t val64;
			uint128_t val128;
		} value;

		                                      
		offset = extract_address_literal_sign_extended(instr, 5, 19);
		address = regs64->pc + offset;

		                                  
		regno = (instr & 0x1f);
		assert(regno <= 31);

		                                            
		switch (tp->ftt_type) {
		case FASTTRAP_T_ARM64_LDR_S_PC_REL:
		case FASTTRAP_T_ARM64_LDR_W_PC_REL:
		case FASTTRAP_T_ARM64_LDRSW_PC_REL:
			valsize = 4;
			break;
		case FASTTRAP_T_ARM64_LDR_D_PC_REL:
		case FASTTRAP_T_ARM64_LDR_X_PC_REL:
			valsize = 8;
			break;
		case FASTTRAP_T_ARM64_LDR_Q_PC_REL:
			valsize = 16;
			break;
		default:
			panic("Should never get here!");
			valsize = -1;
			break;
		}

		if (copyin(address, &value, valsize) != 0) {
			fasttrap_sigsegv(p, uthread, address, state);
#ifndef DEBUG
			new_pc = regs64->pc;                                                                        
			break;
#endif
		}

		                                    
		switch (tp->ftt_type) {
		case FASTTRAP_T_ARM64_LDR_W_PC_REL:
			set_saved_state64_regno(regs64, regno, 1, value.val32);
			break;
		case FASTTRAP_T_ARM64_LDRSW_PC_REL:
			set_saved_state64_regno(regs64, regno, 1, sign_extend(value.val32, 31));
			break;
		case FASTTRAP_T_ARM64_LDR_X_PC_REL:
			set_saved_state64_regno(regs64, regno, 1, value.val64);
			break;
		case FASTTRAP_T_ARM64_LDR_S_PC_REL:
			ns64->v.s[regno][0] = value.val32;
			break;
		case FASTTRAP_T_ARM64_LDR_D_PC_REL:
			ns64->v.d[regno][0] = value.val64;
			break;
		case FASTTRAP_T_ARM64_LDR_Q_PC_REL:
			ns64->v.q[regno] = value.val128;
			break;
		default:
			panic("Should never get here!");
		}


		                     
		new_pc = regs64->pc + 4;
		*was_simulated = 1;
		break;
	}

	case FASTTRAP_T_ARM64_PRFM:
	{
		                                                                      
		new_pc = regs64->pc + 4;
		*was_simulated = 1;
		break;
	}

	  
                                 
    

	  
                                                                          
                                                     
               
                       
                       
                      
                      
                                    
                                   
   
                
                 
   
          
         
          
    
	case FASTTRAP_T_ARM64_B_COND:
	{
		int cond;

		                            
		cond = (instr & 0xf);

		                            
		if (condition_true(cond, regs64->cpsr)) {
			uint64_t offset;

			                                             
			offset = extract_address_literal_sign_extended(instr, 5, 19);
			new_pc = regs64->pc + offset;
		} else {
			                   
			new_pc = regs64->pc + 4;
		}

		*was_simulated = 1;
		break;
	}

	case FASTTRAP_T_ARM64_CBNZ_W:
	{
		do_cbz_cnbz(regs64, 32, instr, 0, &new_pc);
		*was_simulated = 1;
		break;
	}
	case FASTTRAP_T_ARM64_CBNZ_X:
	{
		do_cbz_cnbz(regs64, 64, instr, 0, &new_pc);
		*was_simulated = 1;
		break;
	}
	case FASTTRAP_T_ARM64_CBZ_W:
	{
		do_cbz_cnbz(regs64, 32, instr, 1, &new_pc);
		*was_simulated = 1;
		break;
	}
	case FASTTRAP_T_ARM64_CBZ_X:
	{
		do_cbz_cnbz(regs64, 64, instr, 1, &new_pc);
		*was_simulated = 1;
		break;
	}

	case FASTTRAP_T_ARM64_TBNZ:
	{
		do_tbz_tbnz(regs64, instr, 0, &new_pc);
		*was_simulated = 1;
		break;
	}
	case FASTTRAP_T_ARM64_TBZ:
	{
		do_tbz_tbnz(regs64, instr, 1, &new_pc);
		*was_simulated = 1;
		break;
	}
	case FASTTRAP_T_ARM64_B:
	case FASTTRAP_T_ARM64_BL:
	{
		uint64_t offset;

		                                     
		offset = extract_address_literal_sign_extended(instr, 0, 26);

		                              
		if (tp->ftt_type == FASTTRAP_T_ARM64_BL) {
			regs64->lr = regs64->pc + 4;
		}

		                                                         
		new_pc = regs64->pc + offset;
		*was_simulated = 1;
		break;
	}

	case FASTTRAP_T_ARM64_BLR:
	case FASTTRAP_T_ARM64_BR:
	{
		uint32_t regno;

		                                       
		regno = ((instr >> 5) & 0x1f);
		assert(regno <= 31);

		                              
		if (tp->ftt_type == FASTTRAP_T_ARM64_BLR) {
			regs64->lr = regs64->pc + 4;
		}

		                              
		new_pc = get_saved_state64_regno(regs64, regno, 1);
		*was_simulated = 1;
		break;
	}

	case FASTTRAP_T_ARM64_RET:
	{
		                      
		unsigned regno = ((instr >> 5) & 0x1f);
		assert(regno <= 31);

		                                            
		new_pc = get_saved_state64_regno(regs64, regno, 1);

		*was_simulated = 1;
		break;
	}
	case FASTTRAP_T_ARM64_RETAB:
	{
		                                            
		new_pc = get_saved_state64_regno(regs64, 30, 1);
#if __has_feature(ptrauth_calls)
		new_pc = (user_addr_t) ptrauth_strip((void *)new_pc, ptrauth_key_return_address);
#endif

		*was_simulated = 1;
		break;
	}
	  
                 
    

	  
                                                  
   
                       
                      
    
	case FASTTRAP_T_ARM64_ADRP:
	case FASTTRAP_T_ARM64_ADR:
	{
		uint64_t immhi, immlo, offset, result;
		uint32_t regno;

		                                  
		regno = (instr & 0x1f);
		assert(regno <= 31);

		                    
		immhi = ((instr & 0x00ffffe0) >> 5);                                              
		immlo = ((instr & 0x60000000) >> 29);                                             

		                                                                                             
		if (tp->ftt_type == FASTTRAP_T_ARM64_ADRP) {
			offset =  (immhi << 14) | (immlo << 12);                                                  
			offset = sign_extend(offset, 32);                                                    
			result = (regs64->pc & ~0xfffULL) + offset;                                                
		} else {
			assert(tp->ftt_type == FASTTRAP_T_ARM64_ADR);
			offset =  (immhi << 2) | immlo;                                                           
			offset = sign_extend(offset, 20);                                        
			result = regs64->pc + offset;                                                              
		}

		                 
		set_saved_state64_regno(regs64, regno, 1, result);

		                     
		new_pc = regs64->pc + 4;
		*was_simulated = 1;
		break;
	}

	  
                              
    

	  
                                      
    
	case FASTTRAP_T_COMMON:
	{
		fasttrap_pid_probe_thunk_instr64(state, tp, p, uthread, &tp->ftt_instr, 1, &new_pc);
		*was_simulated = 0;
		break;
	}
	default:
	{
		panic("An instruction DTrace doesn't expect: %d\n", tp->ftt_type);
		break;
	}
	}

	set_saved_state_pc(state, new_pc);
	return;
}

int
fasttrap_pid_probe(arm_saved_state_t *state)
{
	proc_t *p = current_proc();
	fasttrap_bucket_t *bucket;
	lck_mtx_t *pid_mtx;
	fasttrap_tracepoint_t *tp, tp_local;
	pid_t pid;
	dtrace_icookie_t cookie;
	uint_t is_enabled = 0;
	int was_simulated, retire_tp = 1;

	uint64_t pc = get_saved_state_pc(state);

	assert(is_saved_state64(state));

	uthread_t uthread = (uthread_t) get_bsdthread_info(current_thread());

	  
                                                                   
                                                                      
                                                                   
                                        
    
	if (uthread->t_dtrace_step) {
		ASSERT(uthread->t_dtrace_on);
		fasttrap_sigtrap(p, uthread, (user_addr_t)pc);
		return 0;
	}

	  
                                 
    
	uthread->t_dtrace_ft = 0;
	uthread->t_dtrace_pc = 0;
	uthread->t_dtrace_npc = 0;
	uthread->t_dtrace_scrpc = 0;
	uthread->t_dtrace_astpc = 0;
	uthread->t_dtrace_reg = 0;

	  
                                                                 
                                                                     
                      
    
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

	                              
	if (tp->ftt_ids != NULL) {
		fasttrap_id_t *id;
		uint64_t arg4;

		if (is_saved_state64(state)) {
			arg4 = get_saved_state_reg(state, 4);
		} else {
			return -1;
		}


		                                                   

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
					dtrace_probe(probe->ftp_id,
					    get_saved_state_reg(state, 0),
					    get_saved_state_reg(state, 1),
					    get_saved_state_reg(state, 2),
					    get_saved_state_reg(state, 3),
					    arg4);
					DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_ENTRY);
					dtrace_interrupt_enable(cookie);
				} else if (id->fti_ptype == DTFTP_IS_ENABLED) {
					  
                                        
                                           
                                           
                                      
                                  
        
					is_enabled = 1;
				} else if (probe->ftp_argmap == NULL) {
					dtrace_probe(probe->ftp_id,
					    get_saved_state_reg(state, 0),
					    get_saved_state_reg(state, 1),
					    get_saved_state_reg(state, 2),
					    get_saved_state_reg(state, 3),
					    arg4);
				} else {
					uint64_t t[5];

					fasttrap_usdt_args64(probe, saved_state64(state), 5, t);
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

	  
               
   
                                 
                                                                    
                                                                  
                                                                    
                                                               
    
	fasttrap_pid_probe_handle_patched_instr64(state, tp, uthread, p, is_enabled, &was_simulated);

	  
                                                                      
                                                                     
                                                                    
                
    
	if (tp->ftt_retids != NULL) {
		  
                                                             
                                                        
                                              
                                                             
                                                              
     
		  
                                                                
                                                               
                                                                   
                                                     
     
		if (was_simulated) {
			fasttrap_return_common(p, state, (user_addr_t)pc, (user_addr_t)get_saved_state_pc(state));
		} else {
			ASSERT(uthread->t_dtrace_ret != 0);
			ASSERT(uthread->t_dtrace_pc == pc);
			ASSERT(uthread->t_dtrace_scrpc != 0);
			ASSERT(((user_addr_t)get_saved_state_pc(state)) == uthread->t_dtrace_astpc);
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

	  
                                                               
                                                                
                                                             
                                                           
                                           
    
	set_saved_state_pc(regs, pc);

	fasttrap_return_common(p, regs, pc, npc);

	return 0;
}

uint64_t
fasttrap_pid_getarg(void *arg, dtrace_id_t id, void *parg, int argno,
    int aframes)
{
#pragma unused(arg, id, parg, aframes)
	arm_saved_state_t* regs = find_user_regs(current_thread());

	                                            
	if (argno < 8) {
		return saved_state64(regs)->x[argno];
	}

	                                    
	uint64_t value;
	uint64_t* sp = (uint64_t*) saved_state64(regs)->sp;
	DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
	value = dtrace_fuword64((user_addr_t) (sp + argno - 8));
	DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT | CPU_DTRACE_BADADDR);

	return value;
}

uint64_t
fasttrap_usdt_getarg(void *arg, dtrace_id_t id, void *parg, int argno, int aframes)
{
#pragma unused(arg, id, parg, argno, aframes)
	return 0;
}
