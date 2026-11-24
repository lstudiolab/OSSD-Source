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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/fasttrap_isa.h>
#include <sys/fasttrap_impl.h>
#include <sys/dtrace.h>
#include <sys/dtrace_impl.h>
extern dtrace_id_t dtrace_probeid_error;

#include "fasttrap_regset.h"

#include <sys/dtrace_ptss.h>
#include <kern/debug.h>

#include <machine/pal_routines.h>

                                                                       
#define proc_t struct proc                                                   

  
                                    
                                    
  
                                                                          
                                                                           
                                                                            
                                                                              
                                               
  
                                                                          
                                                                          
                                                                        
                                                                       
                                                                             
                                                                           
                                                                               
  
                                                                          
                                                                
                                                                            
                                                                            
                                                                    
                                                                    
                                                                  
                                                                     
   

#define	FASTTRAP_MODRM_MOD(modrm)	(((modrm) >> 6) & 0x3)
#define	FASTTRAP_MODRM_REG(modrm)	(((modrm) >> 3) & 0x7)
#define	FASTTRAP_MODRM_RM(modrm)	((modrm) & 0x7)
#define	FASTTRAP_MODRM(mod, reg, rm)	(((mod) << 6) | ((reg) << 3) | (rm))

#define	FASTTRAP_SIB_SCALE(sib)		(((sib) >> 6) & 0x3)
#define	FASTTRAP_SIB_INDEX(sib)		(((sib) >> 3) & 0x7)
#define	FASTTRAP_SIB_BASE(sib)		((sib) & 0x7)

#define	FASTTRAP_REX_W(rex)		(((rex) >> 3) & 1)
#define	FASTTRAP_REX_R(rex)		(((rex) >> 2) & 1)
#define	FASTTRAP_REX_X(rex)		(((rex) >> 1) & 1)
#define	FASTTRAP_REX_B(rex)		((rex) & 1)
#define	FASTTRAP_REX(w, r, x, b)	\
	(0x40 | ((w) << 3) | ((r) << 2) | ((x) << 1) | (b))

  
                        
   
#define	FASTTRAP_PUSHL_EBP	0x55

#define	FASTTRAP_JO		0x70
#define	FASTTRAP_JNO		0x71
#define	FASTTRAP_JB		0x72
#define	FASTTRAP_JAE		0x73
#define	FASTTRAP_JE		0x74
#define	FASTTRAP_JNE		0x75
#define	FASTTRAP_JBE		0x76
#define	FASTTRAP_JA		0x77
#define	FASTTRAP_JS		0x78
#define	FASTTRAP_JNS		0x79
#define	FASTTRAP_JP		0x7a
#define	FASTTRAP_JNP		0x7b
#define	FASTTRAP_JL		0x7c
#define	FASTTRAP_JGE		0x7d
#define	FASTTRAP_JLE		0x7e
#define	FASTTRAP_JG		0x7f

#define	FASTTRAP_NOP		0x90

#define	FASTTRAP_MOV_EAX	0xb8
#define	FASTTRAP_MOV_ECX	0xb9

#define	FASTTRAP_RET16		0xc2
#define	FASTTRAP_RET		0xc3

#define	FASTTRAP_LOOPNZ		0xe0
#define	FASTTRAP_LOOPZ		0xe1
#define	FASTTRAP_LOOP		0xe2
#define	FASTTRAP_JCXZ		0xe3

#define	FASTTRAP_CALL		0xe8
#define	FASTTRAP_JMP32		0xe9
#define	FASTTRAP_JMP8		0xeb

#define	FASTTRAP_INT3		0xcc
#define	FASTTRAP_INT		0xcd
#define	T_DTRACE_RET		0x7f

#define	FASTTRAP_2_BYTE_OP	0x0f
#define	FASTTRAP_GROUP5_OP	0xff

  
                                        
   
#define	FASTTRAP_0F_JO		0x80
#define	FASTTRAP_0F_JNO		0x81
#define	FASTTRAP_0F_JB		0x82
#define	FASTTRAP_0F_JAE		0x83
#define	FASTTRAP_0F_JE		0x84
#define	FASTTRAP_0F_JNE		0x85
#define	FASTTRAP_0F_JBE		0x86
#define	FASTTRAP_0F_JA		0x87
#define	FASTTRAP_0F_JS		0x88
#define	FASTTRAP_0F_JNS		0x89
#define	FASTTRAP_0F_JP		0x8a
#define	FASTTRAP_0F_JNP		0x8b
#define	FASTTRAP_0F_JL		0x8c
#define	FASTTRAP_0F_JGE		0x8d
#define	FASTTRAP_0F_JLE		0x8e
#define	FASTTRAP_0F_JG		0x8f

#define	FASTTRAP_EFLAGS_OF	0x800
#define	FASTTRAP_EFLAGS_DF	0x400
#define	FASTTRAP_EFLAGS_SF	0x080
#define	FASTTRAP_EFLAGS_ZF	0x040
#define	FASTTRAP_EFLAGS_AF	0x010
#define	FASTTRAP_EFLAGS_PF	0x004
#define	FASTTRAP_EFLAGS_CF	0x001

  
                        
   
#define	FASTTRAP_PREFIX_OPERAND	0x66
#define	FASTTRAP_PREFIX_ADDRESS	0x67
#define	FASTTRAP_PREFIX_CS	0x2E
#define	FASTTRAP_PREFIX_DS	0x3E
#define	FASTTRAP_PREFIX_ES	0x26
#define	FASTTRAP_PREFIX_FS	0x64
#define	FASTTRAP_PREFIX_GS	0x65
#define	FASTTRAP_PREFIX_SS	0x36
#define	FASTTRAP_PREFIX_LOCK	0xF0
#define	FASTTRAP_PREFIX_REP	0xF3
#define	FASTTRAP_PREFIX_REPNE	0xF2

#define	FASTTRAP_NOREG	0xff

  
                                                                            
                                           
   

  
                                                                                
                                                                                    
                                                                                     
  
                 
                 
                 
                 
                  
                 
                 
                 
  
                                                                             
   
static const uint8_t regmap[16] = {
	REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
	REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15,
};

static user_addr_t fasttrap_getreg(x86_saved_state_t *, uint_t);

static uint64_t
fasttrap_anarg(x86_saved_state_t *regs, int function_entry, int argno)
{
	uint64_t value;
	int shift = function_entry ? 1 : 0;

	x86_saved_state64_t *regs64;
	x86_saved_state32_t *regs32;
	unsigned int p_model;

        if (is_saved_state64(regs)) {
                regs64 = saved_state64(regs);
		regs32 = NULL;
		p_model = DATAMODEL_LP64;
        } else {
		regs64 = NULL;
                regs32 = saved_state32(regs);
		p_model = DATAMODEL_ILP32;
        }

	if (p_model == DATAMODEL_LP64) {
		user_addr_t stack;

		  
                                                          
               
     
		if (argno < 6)
			return ((&regs64->rdi)[argno]);

		stack = regs64->isf.rsp + sizeof(uint64_t) * (argno - 6 + shift);
		DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
		value = dtrace_fuword64(stack);
		DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT | CPU_DTRACE_BADADDR);
	} else {
		uint32_t *stack = (uint32_t *)(uintptr_t)(regs32->uesp);
		DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
		value = dtrace_fuword32((user_addr_t)(unsigned long)&stack[argno + shift]);
		DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT | CPU_DTRACE_BADADDR);
	}

	return (value);
}

            
int
fasttrap_tracepoint_init(proc_t *p, fasttrap_tracepoint_t *tp, user_addr_t pc,
    fasttrap_probe_type_t type)
{
#pragma unused(type)
	uint8_t instr[FASTTRAP_MAX_INSTR_SIZE + 10];
	size_t len = FASTTRAP_MAX_INSTR_SIZE;
	size_t first = MIN(len, PAGE_SIZE - (pc & PAGE_MASK));
	uint_t start = 0;
	size_t size;
	int rmindex;
	uint8_t seg, rex = 0;
	unsigned int p_model = (p->p_flag & P_LP64) ? DATAMODEL_LP64 : DATAMODEL_ILP32;

	  
                                                                  
                                                          
                                                                  
                                                                   
                                                                   
                                                                     
    
	  
                                                                          
    
	if (uread(p, &instr[0], first, pc) != 0)
		return (-1);
	if (len > first &&
	    uread(p, &instr[first], len - first, pc + first) != 0) {
		bzero(&instr[first], len - first);
		len = first;
	}

	  
                                                                   
    
	if ((size = dtrace_instr_size_isa(instr, p_model, &rmindex)) <= 0)
		return (-1);

	  
                                                       
    
	ASSERT(-1 <= rmindex && rmindex < (int)size);

	  
                                                                  
                                                                     
                                                                   
                        
    
	if (size > len)
		return (-1);

	tp->ftt_size = (uint8_t)size;
	tp->ftt_segment = FASTTRAP_SEG_NONE;

	  
                                                                
                    
    
	for (;;) {
		seg = 0;
		switch (instr[start]) {
		case FASTTRAP_PREFIX_SS:
			seg++;
			OS_FALLTHROUGH;
		case FASTTRAP_PREFIX_GS:
			seg++;
			OS_FALLTHROUGH;
		case FASTTRAP_PREFIX_FS:
			seg++;
			OS_FALLTHROUGH;
		case FASTTRAP_PREFIX_ES:
			seg++;
			OS_FALLTHROUGH;
		case FASTTRAP_PREFIX_DS:
			seg++;
			OS_FALLTHROUGH;
		case FASTTRAP_PREFIX_CS:
			seg++;
			OS_FALLTHROUGH;
		case FASTTRAP_PREFIX_OPERAND:
		case FASTTRAP_PREFIX_ADDRESS:
		case FASTTRAP_PREFIX_LOCK:
		case FASTTRAP_PREFIX_REP:
		case FASTTRAP_PREFIX_REPNE:
			if (seg != 0) {
				  
                                                 
                                              
                           
       
				if (tp->ftt_segment != FASTTRAP_SEG_NONE)
					return (-1);

				tp->ftt_segment = seg;
			}
			start++;
			continue;
		}
		break;
	}

	  
                                                
    
	if (p_model == DATAMODEL_LP64 && (instr[start] & 0xf0) == 0x40)
		rex = instr[start++];

	  
                                                                     
                                 
    
	bcopy(instr, tp->ftt_instr, FASTTRAP_MAX_INSTR_SIZE);

	tp->ftt_type = FASTTRAP_T_COMMON;
	if (instr[start] == FASTTRAP_2_BYTE_OP) {
		switch (instr[start + 1]) {
		case FASTTRAP_0F_JO:
		case FASTTRAP_0F_JNO:
		case FASTTRAP_0F_JB:
		case FASTTRAP_0F_JAE:
		case FASTTRAP_0F_JE:
		case FASTTRAP_0F_JNE:
		case FASTTRAP_0F_JBE:
		case FASTTRAP_0F_JA:
		case FASTTRAP_0F_JS:
		case FASTTRAP_0F_JNS:
		case FASTTRAP_0F_JP:
		case FASTTRAP_0F_JNP:
		case FASTTRAP_0F_JL:
		case FASTTRAP_0F_JGE:
		case FASTTRAP_0F_JLE:
		case FASTTRAP_0F_JG:
			tp->ftt_type = FASTTRAP_T_JCC;
			tp->ftt_code = (instr[start + 1] & 0x0f) | FASTTRAP_JO;
			tp->ftt_dest = pc + tp->ftt_size +
			                            
			    *(int32_t *)&instr[start + 2];
			break;
		}
	} else if (instr[start] == FASTTRAP_GROUP5_OP) {
		uint_t mod = FASTTRAP_MODRM_MOD(instr[start + 1]);
		uint_t reg = FASTTRAP_MODRM_REG(instr[start + 1]);
		uint_t rm = FASTTRAP_MODRM_RM(instr[start + 1]);

		if (reg == 2 || reg == 4) {
			uint_t i, sz;

			if (reg == 2)
				tp->ftt_type = FASTTRAP_T_CALL;
			else
				tp->ftt_type = FASTTRAP_T_JMP;

			if (mod == 3)
				tp->ftt_code = 2;
			else
				tp->ftt_code = 1;

			ASSERT(p_model == DATAMODEL_LP64 || rex == 0);

			  
                                                     
                                              
                                 
      
			if (mod != 3 && rm == 4) {
				uint8_t sib = instr[start + 2];
				uint_t index = FASTTRAP_SIB_INDEX(sib);
				uint_t base = FASTTRAP_SIB_BASE(sib);

				tp->ftt_scale = FASTTRAP_SIB_SCALE(sib);

				tp->ftt_index = (index == 4) ?
				    FASTTRAP_NOREG :
				    regmap[index | (FASTTRAP_REX_X(rex) << 3)];
				tp->ftt_base = (mod == 0 && base == 5) ?
				    FASTTRAP_NOREG :
				    regmap[base | (FASTTRAP_REX_B(rex) << 3)];

				i = 3;
				sz = mod == 1 ? 1 : 4;
			} else {
				  
                                            
                                                  
                                                  
                                        
       
				if (mod == 0 && rm == 5) {
					if (p_model == DATAMODEL_LP64)
						tp->ftt_base = REG_RIP;
					else
						tp->ftt_base = FASTTRAP_NOREG;
					sz = 4;
				} else  {
					uint8_t base = rm |
					    (FASTTRAP_REX_B(rex) << 3);

					tp->ftt_base = regmap[base];
					sz = mod == 1 ? 1 : mod == 2 ? 4 : 0;
				}
				tp->ftt_index = FASTTRAP_NOREG;
				i = 2;
			}

			if (sz == 1) {
				tp->ftt_dest = *(int8_t *)&instr[start + i];
			} else if (sz == 4) {
				                        
				tp->ftt_dest = *(int32_t *)&instr[start + i];
			} else {
				tp->ftt_dest = 0;
			}
		}
	} else {
		switch (instr[start]) {
		case FASTTRAP_RET:
			tp->ftt_type = FASTTRAP_T_RET;
			break;

		case FASTTRAP_RET16:
			tp->ftt_type = FASTTRAP_T_RET16;
			                        
			tp->ftt_dest = *(uint16_t *)&instr[start + 1];
			break;

		case FASTTRAP_JO:
		case FASTTRAP_JNO:
		case FASTTRAP_JB:
		case FASTTRAP_JAE:
		case FASTTRAP_JE:
		case FASTTRAP_JNE:
		case FASTTRAP_JBE:
		case FASTTRAP_JA:
		case FASTTRAP_JS:
		case FASTTRAP_JNS:
		case FASTTRAP_JP:
		case FASTTRAP_JNP:
		case FASTTRAP_JL:
		case FASTTRAP_JGE:
		case FASTTRAP_JLE:
		case FASTTRAP_JG:
			tp->ftt_type = FASTTRAP_T_JCC;
			tp->ftt_code = instr[start];
			tp->ftt_dest = pc + tp->ftt_size +
			    (int8_t)instr[start + 1];
			break;

		case FASTTRAP_LOOPNZ:
		case FASTTRAP_LOOPZ:
		case FASTTRAP_LOOP:
			tp->ftt_type = FASTTRAP_T_LOOP;
			tp->ftt_code = instr[start];
			tp->ftt_dest = pc + tp->ftt_size +
			    (int8_t)instr[start + 1];
			break;

		case FASTTRAP_JCXZ:
			tp->ftt_type = FASTTRAP_T_JCXZ;
			tp->ftt_dest = pc + tp->ftt_size +
			    (int8_t)instr[start + 1];
			break;

		case FASTTRAP_CALL:
			tp->ftt_type = FASTTRAP_T_CALL;
			tp->ftt_dest = pc + tp->ftt_size +
			                            
			    *(int32_t *)&instr[start + 1];
			tp->ftt_code = 0;
			break;

		case FASTTRAP_JMP32:
			tp->ftt_type = FASTTRAP_T_JMP;
			tp->ftt_dest = pc + tp->ftt_size +
				                        
			    *(int32_t *)&instr[start + 1];
			break;
		case FASTTRAP_JMP8:
			tp->ftt_type = FASTTRAP_T_JMP;
			tp->ftt_dest = pc + tp->ftt_size +
			    (int8_t)instr[start + 1];
			break;

		case FASTTRAP_PUSHL_EBP:
			if (start == 0)
				tp->ftt_type = FASTTRAP_T_PUSHL_EBP;
			break;

		case FASTTRAP_NOP:
			ASSERT(p_model == DATAMODEL_LP64 || rex == 0);

			  
                                                         
                                                           
                                                        
                                                 
      
			if (FASTTRAP_REX_B(rex) == 0)
				tp->ftt_type = FASTTRAP_T_NOP;
			break;

		case FASTTRAP_INT3:
			  
                                                         
                                              
      
			ASSERT(instr[start] == FASTTRAP_INSTR);
			return (-1);

		case FASTTRAP_INT:
			  
                                                    
                                                      
                                                       
                                               
                                                       
                                               
                                                        
                                                        
      
			return (-1);
		}
	}

	if (p_model == DATAMODEL_LP64 && tp->ftt_type == FASTTRAP_T_COMMON) {
		  
                                                               
                                                               
                                                     
                                                             
                                                
                                                           
                                     
     
		if (rmindex != -1) {
			uint_t mod = FASTTRAP_MODRM_MOD(instr[rmindex]);
			uint_t reg = FASTTRAP_MODRM_REG(instr[rmindex]);
			uint_t rm = FASTTRAP_MODRM_RM(instr[rmindex]);

			ASSERT(rmindex > (int)start);

			if (mod == 0 && rm == 5) {
				  
                                        
                                                
                                              
                                                
                                                 
                                            
                                             
       
				if (reg != 0) {
					tp->ftt_ripmode = FASTTRAP_RIP_1 |
					    (FASTTRAP_RIP_X *
					    FASTTRAP_REX_B(rex));
					rm = 0;
				} else {
					tp->ftt_ripmode = FASTTRAP_RIP_2 |
					    (FASTTRAP_RIP_X *
					    FASTTRAP_REX_B(rex));
					rm = 1;
				}

				tp->ftt_modrm = tp->ftt_instr[rmindex];
				tp->ftt_instr[rmindex] =
				    FASTTRAP_MODRM(2, reg, rm);
			}
		}
	}

	return (0);
}

int
fasttrap_tracepoint_install(proc_t *p, fasttrap_tracepoint_t *tp)
{
	fasttrap_instr_t instr = FASTTRAP_INSTR;

	if (uwrite(p, &instr, 1, tp->ftt_pc) != 0)
		return (-1);

	tp->ftt_installed = 1;

	return (0);
}

int
fasttrap_tracepoint_remove(proc_t *p, fasttrap_tracepoint_t *tp)
{
	uint8_t instr;

	  
                                                            
                
    
	if (uread(p, &instr, 1, tp->ftt_pc) != 0)
		goto end;
	if (instr != FASTTRAP_INSTR)
		goto end;
	if (uwrite(p, &tp->ftt_instr[0], 1, tp->ftt_pc) != 0)
		return (-1);
end:
	tp->ftt_installed = 0;

	return (0);
}

static void
fasttrap_return_common(x86_saved_state_t *regs, user_addr_t pc, pid_t pid,
    user_addr_t new_pc)
{
	x86_saved_state64_t *regs64;
	x86_saved_state32_t *regs32;
	unsigned int p_model;
	int retire_tp = 1;

	dtrace_icookie_t cookie;

        if (is_saved_state64(regs)) {
                regs64 = saved_state64(regs);
		regs32 = NULL;
		p_model = DATAMODEL_LP64;
        } else {
		regs64 = NULL;
                regs32 = saved_state32(regs);
		p_model = DATAMODEL_ILP32;
        }

	fasttrap_tracepoint_t *tp;
	fasttrap_bucket_t *bucket;
	fasttrap_id_t *id;
	lck_mtx_t *pid_mtx;

	pid_mtx = &cpu_core[CPU->cpu_id].cpuc_pid_lock;
	lck_mtx_lock(pid_mtx);
	bucket = &fasttrap_tpoints.fth_table[FASTTRAP_TPOINTS_INDEX(pid, pc)];

	for (tp = bucket->ftb_data; tp != NULL; tp = tp->ftt_next) {
		if (pid == tp->ftt_pid && pc == tp->ftt_pc &&
		    tp->ftt_proc->ftpc_acount != 0)
			break;
	}

	  
                                                                
                                                                   
                                                             
    
	if (tp == NULL) {
		lck_mtx_unlock(pid_mtx);
		return;
	}

	for (id = tp->ftt_retids; id != NULL; id = id->fti_next) {
		fasttrap_probe_t *probe = id->fti_probe;
		  
                                                            
                                                               
                              
     
		if (tp->ftt_type != FASTTRAP_T_RET &&
		    tp->ftt_type != FASTTRAP_T_RET16 &&
		    new_pc - probe->ftp_faddr < probe->ftp_fsize)
			continue;

		if (probe->ftp_prov->ftp_provider_type == DTFTP_PROVIDER_ONESHOT) {
			if (os_atomic_xchg(&probe->ftp_triggered, 1, relaxed)) {
				                       
				continue;
			}
		}
		  
                                                  
                                             
               
     
		else {
			retire_tp = 0;
		}
		  
                                                           
                                                            
                                                            
     
		cookie = dtrace_interrupt_disable();
		cpu_core[CPU->cpu_id].cpuc_missing_tos = pc;
		if (ISSET(current_proc()->p_lflag, P_LNOATTACH)) {
			dtrace_probe(dtrace_probeid_error, 0            , probe->ftp_id,
				     1          , -1             , DTRACEFLT_UPRIV);
		} else if (p_model == DATAMODEL_LP64) {
			dtrace_probe(probe->ftp_id,
				     pc - id->fti_probe->ftp_faddr,
				     regs64->rax, regs64->rdx, 0, 0);
		} else {
			dtrace_probe(probe->ftp_id,
				     pc - id->fti_probe->ftp_faddr,
				     regs32->eax, regs32->edx, 0, 0);
		}
		                     
		cpu_core[CPU->cpu_id].cpuc_missing_tos = 0;
		dtrace_interrupt_enable(cookie);
	}

	lck_mtx_unlock(pid_mtx);
}

static void
fasttrap_sigsegv(proc_t *p, uthread_t t, user_addr_t addr)
{
	proc_lock(p);

	                                       
	t->uu_code = addr;
	t->uu_siglist |= sigmask(SIGSEGV);

	  
                                                                    
                                                          
                                                                
    
	t->uu_exception = KERN_INVALID_ADDRESS;		             
	t->uu_subcode = 0;	             

	proc_unlock(p);

	                  
	signal_setast(t->uu_context.vc_thread);
}

static void
fasttrap_usdt_args64(fasttrap_probe_t *probe, x86_saved_state64_t *regs64, int argc,
    uint64_t *argv)
{
	int i, x, cap = MIN(argc, probe->ftp_nargs);
	user_addr_t stack = (user_addr_t)regs64->isf.rsp;

	for (i = 0; i < cap; i++) {
		x = probe->ftp_argmap[i];

		if (x < 6) {
			                                              
			argv[i] = (&regs64->rdi)[x];
		} else {
			fasttrap_fuword64_noerr(stack + (x * sizeof(uint64_t)), &argv[i]);
		}
	}

	for (; i < argc; i++) {
		argv[i] = 0;
	}
}

static void
fasttrap_usdt_args32(fasttrap_probe_t *probe, x86_saved_state32_t *regs32, int argc,
    uint32_t *argv)
{
	int i, x, cap = MIN(argc, probe->ftp_nargs);
	uint32_t *stack = (uint32_t *)(uintptr_t)(regs32->uesp);

	for (i = 0; i < cap; i++) {
		x = probe->ftp_argmap[i];

		fasttrap_fuword32_noerr((user_addr_t)(unsigned long)&stack[x], &argv[i]);
	}

	for (; i < argc; i++) {
		argv[i] = 0;
	}
}

  
         
   
static int
fasttrap_do_seg(fasttrap_tracepoint_t *tp, x86_saved_state_t *rp, user_addr_t *addr)          
{
#pragma unused(tp, rp, addr)
	printf("fasttrap_do_seg() called while unimplemented.\n");
#if 0
	proc_t *p = curproc;
	user_desc_t *desc;
	uint16_t sel, ndx, type;
	uintptr_t limit;

	switch (tp->ftt_segment) {
	case FASTTRAP_SEG_CS:
		sel = rp->r_cs;
		break;
	case FASTTRAP_SEG_DS:
		sel = rp->r_ds;
		break;
	case FASTTRAP_SEG_ES:
		sel = rp->r_es;
		break;
	case FASTTRAP_SEG_FS:
		sel = rp->r_fs;
		break;
	case FASTTRAP_SEG_GS:
		sel = rp->r_gs;
		break;
	case FASTTRAP_SEG_SS:
		sel = rp->r_ss;
		break;
	}

	  
                                                                  
                                           
    
	if (!SELISUPL(sel))
		return (-1);

	ndx = SELTOIDX(sel);

	  
                                                                 
                     
    
	if (SELISLDT(sel)) {
		if (ndx > p->p_ldtlimit)
			return (-1);

		desc = p->p_ldt + ndx;

	} else {
		if (ndx >= NGDT)
			return (-1);

		desc = cpu_get_gdt() + ndx;
	}

	  
                                                                
                      
    
	if (desc->usd_dpl != SEL_UPL || desc->usd_p != 1)
		return (-1);

	type = desc->usd_type;

	  
                                                                  
                                   
    
	if ((type & 0x10) != 0x10)
		return (-1);

	limit = USEGD_GETLIMIT(desc) * (desc->usd_gran ? PAGESIZE : 1);

	if (tp->ftt_segment == FASTTRAP_SEG_CS) {
		  
                                                         
     
		if ((type & 0xa) != 0xa)
			return (-1);

		if (*addr > limit)
			return (-1);
	} else {
		  
                                     
     
		if ((type & 0x8) != 0)
			return (-1);

		  
                                                                
                                                               
                                                           
                                                           
                             
     
		if ((type & 0x4) == 0) {
			if (*addr > limit)
				return (-1);
		} else if (desc->usd_def32) {
			if (*addr < limit + 1 || 0xffff < *addr)
				return (-1);
		} else {
			if (*addr < limit + 1 || 0xffffffff < *addr)
				return (-1);
		}
	}

	*addr += USEGD_GETBASE(desc);
#endif        
	return (0);
}

  
                                                                                       
                                                                                      
                                                       
  
           
  
                                                                          
  
   
static int
fasttrap_pid_probe32(x86_saved_state_t *regs)
{
	ASSERT(is_saved_state32(regs));

	x86_saved_state32_t *regs32  = saved_state32(regs);
	user_addr_t pc = regs32->eip - 1;
	proc_t *p = current_proc();
	user_addr_t new_pc = 0;
	fasttrap_bucket_t *bucket;
	lck_mtx_t *pid_mtx;
	fasttrap_tracepoint_t *tp, tp_local;
	pid_t pid;
	dtrace_icookie_t cookie;
	uint_t is_enabled = 0, retire_tp = 1;

	uthread_t uthread = (uthread_t)get_bsdthread_info(current_thread());

	  
                                                                   
                                                                      
                                                                   
                                        
    
	if (uthread->t_dtrace_step) {
		ASSERT(uthread->t_dtrace_on);
		fasttrap_sigtrap(p, uthread, pc);
		return (0);
	}

	  
                                 
    
	uthread->t_dtrace_ft = 0;
	uthread->t_dtrace_pc = 0;
	uthread->t_dtrace_npc = 0;
	uthread->t_dtrace_scrpc = 0;
	uthread->t_dtrace_astpc = 0;

	  
                                                                 
                                                                     
                      
    
	if (p->p_lflag & P_LINVFORK) {
		proc_list_lock();
		while (p->p_lflag & P_LINVFORK)
			p = p->p_pptr;
		proc_list_unlock();
	}

	pid = p->p_pid;
	pid_mtx = &cpu_core[CPU->cpu_id].cpuc_pid_lock;
	lck_mtx_lock(pid_mtx);
	bucket = &fasttrap_tpoints.fth_table[FASTTRAP_TPOINTS_INDEX(pid, pc)];

	  
                                                    
    
	for (tp = bucket->ftb_data; tp != NULL; tp = tp->ftt_next) {
		if (pid == tp->ftt_pid && pc == tp->ftt_pc &&
		    tp->ftt_proc->ftpc_acount != 0)
			break;
	}

	  
                                                                      
                                                                 
                                                                
    
	if (tp == NULL) {
		lck_mtx_unlock(pid_mtx);
		return (-1);
	}

	  
                                                                    
                                              
    
	regs32->eip = pc;

	if (tp->ftt_ids != NULL) {
		fasttrap_id_t *id;

		uint32_t s0, s1, s2, s3, s4, s5;
		uint32_t *stack = (uint32_t *)(uintptr_t)(regs32->uesp);

		  
                                                    
                                                      
                                               
                                                
                               
     
		fasttrap_fuword32_noerr((user_addr_t)(unsigned long)&stack[0], &s0);
		fasttrap_fuword32_noerr((user_addr_t)(unsigned long)&stack[1], &s1);
		fasttrap_fuword32_noerr((user_addr_t)(unsigned long)&stack[2], &s2);
		fasttrap_fuword32_noerr((user_addr_t)(unsigned long)&stack[3], &s3);
		fasttrap_fuword32_noerr((user_addr_t)(unsigned long)&stack[4], &s4);
		fasttrap_fuword32_noerr((user_addr_t)(unsigned long)&stack[5], &s5);

		for (id = tp->ftt_ids; id != NULL; id = id->fti_next) {
			fasttrap_probe_t *probe = id->fti_probe;

			if (ISSET(current_proc()->p_lflag, P_LNOATTACH)) {
				dtrace_probe(dtrace_probeid_error, 0            , probe->ftp_id,
					     1          , -1             , DTRACEFLT_UPRIV);
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
					dtrace_probe(probe->ftp_id, s1, s2,
						     s3, s4, s5);
					DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_ENTRY);
					dtrace_interrupt_enable(cookie);
				} else if (id->fti_ptype == DTFTP_IS_ENABLED) {
					  
                                        
                                           
                                           
                                      
                                  
        
					is_enabled = 1;
				} else if (probe->ftp_argmap == NULL) {
					dtrace_probe(probe->ftp_id, s0, s1,
						     s2, s3, s4);
				} else {
					uint32_t t[5];

					fasttrap_usdt_args32(probe, regs32,
							     sizeof (t) / sizeof (t[0]), t);

					dtrace_probe(probe->ftp_id, t[0], t[1],
						     t[2], t[3], t[4]);
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

	  
                                                                      
                                                                     
                                          
    
	regs32->eip = pc + tp->ftt_size;

	  
                                                                  
                                                                 
                                                                   
                                                                  
                                                                     
                                                                      
                                                                 
                                                                   
                                            
    
	if (is_enabled) {
		regs32->eax = 1;
		new_pc = regs32->eip;
		goto done;
	}

	  
                                                                  
                                                                
                                                                   
         
    
	switch (tp->ftt_type) {
		case FASTTRAP_T_RET:
		case FASTTRAP_T_RET16:
		{
			user_addr_t dst;
			user_addr_t addr;
			int ret;

			  
                                                               
                                                              
                                             
      
			uint32_t dst32;
			ret = fasttrap_fuword32((user_addr_t)regs32->uesp, &dst32);
			dst = dst32;
			addr = regs32->uesp + sizeof (uint32_t);

			if (ret == -1) {
				fasttrap_sigsegv(p, uthread, (user_addr_t)regs32->uesp);
				new_pc = pc;
				break;
			}

			if (tp->ftt_type == FASTTRAP_T_RET16)
				addr += tp->ftt_dest;

			regs32->uesp = addr;
			new_pc = dst;
			break;
		}

		case FASTTRAP_T_JCC:
		{
			uint_t taken;

			switch (tp->ftt_code) {
				case FASTTRAP_JO:
					taken = (regs32->efl & FASTTRAP_EFLAGS_OF) != 0;
					break;
				case FASTTRAP_JNO:
					taken = (regs32->efl & FASTTRAP_EFLAGS_OF) == 0;
					break;
				case FASTTRAP_JB:
					taken = (regs32->efl & FASTTRAP_EFLAGS_CF) != 0;
					break;
				case FASTTRAP_JAE:
					taken = (regs32->efl & FASTTRAP_EFLAGS_CF) == 0;
					break;
				case FASTTRAP_JE:
					taken = (regs32->efl & FASTTRAP_EFLAGS_ZF) != 0;
					break;
				case FASTTRAP_JNE:
					taken = (regs32->efl & FASTTRAP_EFLAGS_ZF) == 0;
					break;
				case FASTTRAP_JBE:
					taken = (regs32->efl & FASTTRAP_EFLAGS_CF) != 0 ||
						(regs32->efl & FASTTRAP_EFLAGS_ZF) != 0;
					break;
				case FASTTRAP_JA:
					taken = (regs32->efl & FASTTRAP_EFLAGS_CF) == 0 &&
						(regs32->efl & FASTTRAP_EFLAGS_ZF) == 0;
					break;
				case FASTTRAP_JS:
					taken = (regs32->efl & FASTTRAP_EFLAGS_SF) != 0;
					break;
				case FASTTRAP_JNS:
					taken = (regs32->efl & FASTTRAP_EFLAGS_SF) == 0;
					break;
				case FASTTRAP_JP:
					taken = (regs32->efl & FASTTRAP_EFLAGS_PF) != 0;
					break;
				case FASTTRAP_JNP:
					taken = (regs32->efl & FASTTRAP_EFLAGS_PF) == 0;
					break;
				case FASTTRAP_JL:
					taken = ((regs32->efl & FASTTRAP_EFLAGS_SF) == 0) !=
						((regs32->efl & FASTTRAP_EFLAGS_OF) == 0);
					break;
				case FASTTRAP_JGE:
					taken = ((regs32->efl & FASTTRAP_EFLAGS_SF) == 0) ==
						((regs32->efl & FASTTRAP_EFLAGS_OF) == 0);
					break;
				case FASTTRAP_JLE:
					taken = (regs32->efl & FASTTRAP_EFLAGS_ZF) != 0 ||
						((regs32->efl & FASTTRAP_EFLAGS_SF) == 0) !=
						((regs32->efl & FASTTRAP_EFLAGS_OF) == 0);
					break;
				case FASTTRAP_JG:
					taken = (regs32->efl & FASTTRAP_EFLAGS_ZF) == 0 &&
						((regs32->efl & FASTTRAP_EFLAGS_SF) == 0) ==
						((regs32->efl & FASTTRAP_EFLAGS_OF) == 0);
					break;
				default:
					taken = FALSE;
			}

			if (taken)
				new_pc = tp->ftt_dest;
			else
				new_pc = pc + tp->ftt_size;
			break;
		}

		case FASTTRAP_T_LOOP:
		{
			uint_t taken;
			greg_t cx = regs32->ecx--;

			switch (tp->ftt_code) {
				case FASTTRAP_LOOPNZ:
					taken = (regs32->efl & FASTTRAP_EFLAGS_ZF) == 0 &&
						cx != 0;
					break;
				case FASTTRAP_LOOPZ:
					taken = (regs32->efl & FASTTRAP_EFLAGS_ZF) != 0 &&
						cx != 0;
					break;
				case FASTTRAP_LOOP:
					taken = (cx != 0);
					break;
				default:
					taken = FALSE;
			}

			if (taken)
				new_pc = tp->ftt_dest;
			else
				new_pc = pc + tp->ftt_size;
			break;
		}

		case FASTTRAP_T_JCXZ:
		{
			greg_t cx = regs32->ecx;

			if (cx == 0)
				new_pc = tp->ftt_dest;
			else
				new_pc = pc + tp->ftt_size;
			break;
		}

		case FASTTRAP_T_PUSHL_EBP:
		{
			user_addr_t addr = regs32->uesp - sizeof (uint32_t);
			int ret = fasttrap_suword32(addr, (uint32_t)regs32->ebp);

			if (ret == -1) {
				fasttrap_sigsegv(p, uthread, addr);
				new_pc = pc;
				break;
			}

			regs32->uesp = addr;
			new_pc = pc + tp->ftt_size;
			break;
		}

		case FASTTRAP_T_NOP:
			new_pc = pc + tp->ftt_size;
			break;

		case FASTTRAP_T_JMP:
		case FASTTRAP_T_CALL:
			if (tp->ftt_code == 0) {
				new_pc = tp->ftt_dest;
			} else {
				user_addr_t              addr = tp->ftt_dest;

				if (tp->ftt_base != FASTTRAP_NOREG)
					addr += fasttrap_getreg(regs, tp->ftt_base);
				if (tp->ftt_index != FASTTRAP_NOREG)
					addr += fasttrap_getreg(regs, tp->ftt_index) <<
						tp->ftt_scale;

				if (tp->ftt_code == 1) {
					  
                                            
                                                    
                                                    
                                
        
					if (tp->ftt_segment != FASTTRAP_SEG_NONE &&
					    fasttrap_do_seg(tp, regs, &addr) != 0) {
						fasttrap_sigsegv(p, uthread, addr);
						new_pc = pc;
						break;
					}

					uint32_t value32;
					addr = (user_addr_t)(uint32_t)addr;
					if (fasttrap_fuword32(addr, &value32) == -1) {
						fasttrap_sigsegv(p, uthread, addr);
						new_pc = pc;
						break;
					}
					new_pc = value32;
				} else {
					new_pc = addr;
				}
			}

			  
                                                               
                                                                
                                                                
                                      
      
			if (tp->ftt_type == FASTTRAP_T_CALL) {
				user_addr_t addr = regs32->uesp - sizeof (uint32_t);
				int ret = fasttrap_suword32(addr, (uint32_t)(pc + tp->ftt_size));

				if (ret == -1) {
					fasttrap_sigsegv(p, uthread, addr);
					new_pc = pc;
					break;
				}

				regs32->uesp = addr;
			}
			break;

		case FASTTRAP_T_COMMON:
		{
			user_addr_t addr, write_addr;
			uint8_t scratch[2 * FASTTRAP_MAX_INSTR_SIZE + 7];
			uint_t i = 0;

			  
                                 
                                 
     
                                                              
                                                      
     
                         
                                    
                                      
                                   
                                     
                             
               
               
     
                         
                                    
                                      
                         
                                
                                      
                              
                
                
     
                                                               
                                                          
                                                              
                                                              
                                                                 
     
                                                                
                                                             
                                                           
                                                               
                   
      

			addr = uthread->t_dtrace_scratch->addr;
			write_addr = uthread->t_dtrace_scratch->write_addr;

			if (addr == 0LL || write_addr == 0LL) {
				fasttrap_sigtrap(p, uthread, pc);                                 
				new_pc = pc;
				break;
			}

			ASSERT(tp->ftt_size < FASTTRAP_MAX_INSTR_SIZE);

			uthread->t_dtrace_scrpc = addr;
			bcopy(tp->ftt_instr, &scratch[i], tp->ftt_size);
			i += tp->ftt_size;

			  
                                                       
                                                     
      
			scratch[i++] = FASTTRAP_JMP32;
			                        
			*(uint32_t *)&scratch[i] = pc - addr - 5;
			i += sizeof (uint32_t);

			uthread->t_dtrace_astpc = addr + i;
			bcopy(tp->ftt_instr, &scratch[i], tp->ftt_size);
			i += tp->ftt_size;
			scratch[i++] = FASTTRAP_INT;
			scratch[i++] = T_DTRACE_RET;

			ASSERT(i <= sizeof (scratch));

			if (fasttrap_copyout(scratch, write_addr, i)) {
				fasttrap_sigtrap(p, uthread, pc);
				new_pc = pc;
				break;
			}

			if (tp->ftt_retids != NULL) {
				uthread->t_dtrace_step = 1;
				uthread->t_dtrace_ret = 1;
				new_pc = uthread->t_dtrace_astpc;
			} else {
				new_pc = uthread->t_dtrace_scrpc;
			}

			uthread->t_dtrace_pc = pc;
			uthread->t_dtrace_npc = pc + tp->ftt_size;
			uthread->t_dtrace_on = 1;
			break;
		}

		default:
			panic("fasttrap: mishandled an instruction");
	}

done:
	  
               
   
                                                                    
                                                                  
                                                                    
                                                               
    
	regs32->eip = new_pc;

	  
                                                                      
                                                                     
                                                                    
                
    
	if (tp->ftt_retids != NULL) {
		  
                                                             
                                                        
                                              
                                                             
                                                              
     
		if (tp->ftt_type != FASTTRAP_T_COMMON) {
			fasttrap_return_common(regs, pc, pid, new_pc);
		} else {
			ASSERT(uthread->t_dtrace_ret != 0);
			ASSERT(uthread->t_dtrace_pc == pc);
			ASSERT(uthread->t_dtrace_scrpc != 0);
			ASSERT(new_pc == uthread->t_dtrace_astpc);
		}
	}

	return (0);
}

  
                                                                                       
                                                                                      
                                                       
  
           
  
                                                                          
  
   
static int
fasttrap_pid_probe64(x86_saved_state_t *regs)
{
	ASSERT(is_saved_state64(regs));

	x86_saved_state64_t *regs64 = saved_state64(regs);
	user_addr_t pc = regs64->isf.rip - 1;
	proc_t *p = current_proc();
	user_addr_t new_pc = 0;
	fasttrap_bucket_t *bucket;
	lck_mtx_t *pid_mtx;
	fasttrap_tracepoint_t *tp, tp_local;
	pid_t pid;
	dtrace_icookie_t cookie;
	uint_t is_enabled = 0;
	int retire_tp = 1;

	uthread_t uthread = (uthread_t)get_bsdthread_info(current_thread());

	  
                                                                   
                                                                      
                                                                   
                                        
    
	if (uthread->t_dtrace_step) {
		ASSERT(uthread->t_dtrace_on);
		fasttrap_sigtrap(p, uthread, pc);
		return (0);
	}

	  
                                 
    
	uthread->t_dtrace_ft = 0;
	uthread->t_dtrace_pc = 0;
	uthread->t_dtrace_npc = 0;
	uthread->t_dtrace_scrpc = 0;
	uthread->t_dtrace_astpc = 0;
	uthread->t_dtrace_regv = 0;

	  
                                                                 
                                                                     
                      
    
	if (p->p_lflag & P_LINVFORK) {
		proc_list_lock();
		while (p->p_lflag & P_LINVFORK)
			p = p->p_pptr;
		proc_list_unlock();
	}

	pid = p->p_pid;
	pid_mtx = &cpu_core[CPU->cpu_id].cpuc_pid_lock;
	lck_mtx_lock(pid_mtx);
	bucket = &fasttrap_tpoints.fth_table[FASTTRAP_TPOINTS_INDEX(pid, pc)];

	  
                                                    
    
	for (tp = bucket->ftb_data; tp != NULL; tp = tp->ftt_next) {
		if (pid == tp->ftt_pid && pc == tp->ftt_pc &&
		    tp->ftt_proc->ftpc_acount != 0)
			break;
	}

	  
                                                                      
                                                                 
                                                                
    
	if (tp == NULL) {
		lck_mtx_unlock(pid_mtx);
		return (-1);
	}

	  
                                                                    
                                              
    
	regs64->isf.rip = pc;

	if (tp->ftt_ids != NULL) {
		fasttrap_id_t *id;

		for (id = tp->ftt_ids; id != NULL; id = id->fti_next) {
			fasttrap_probe_t *probe = id->fti_probe;

			if (probe->ftp_prov->ftp_provider_type == DTFTP_PROVIDER_ONESHOT) {
				if (os_atomic_xchg(&probe->ftp_triggered, 1, relaxed)) {
					                       
					continue;
				}
			}
			  
                                               
                                              
                
      
			else {
				retire_tp = 0;
			}
			if (ISSET(current_proc()->p_lflag, P_LNOATTACH)) {
				dtrace_probe(dtrace_probeid_error, 0            , probe->ftp_id,
					     1          , -1             , DTRACEFLT_UPRIV);
			} else if (id->fti_ptype == DTFTP_ENTRY) {
				  
                                     
                                      
                    
       
				cookie = dtrace_interrupt_disable();
				DTRACE_CPUFLAG_SET(CPU_DTRACE_ENTRY);
				dtrace_probe(probe->ftp_id, regs64->rdi,
					     regs64->rsi, regs64->rdx, regs64->rcx,
					     regs64->r8);
				DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_ENTRY);
				dtrace_interrupt_enable(cookie);
			} else if (id->fti_ptype == DTFTP_IS_ENABLED) {
				  
                                       
                                          
                                          
                                     
                                 
       
				is_enabled = 1;
			} else if (probe->ftp_argmap == NULL) {
				dtrace_probe(probe->ftp_id, regs64->rdi,
					     regs64->rsi, regs64->rdx, regs64->rcx,
					     regs64->r8);
			} else {
				uint64_t t[5];

				fasttrap_usdt_args64(probe, regs64,
						     sizeof (t) / sizeof (t[0]), t);

				dtrace_probe(probe->ftp_id, t[0], t[1],
					     t[2], t[3], t[4]);
			}

		}
		if (retire_tp) {
			fasttrap_tracepoint_retire(p, tp);
		}
	}

	  
                                                                 
                                                                
                                                                    
    
	tp_local = *tp;
	lck_mtx_unlock(pid_mtx);
	tp = &tp_local;

	  
                                                                      
                                                                     
                                          
    
	regs64->isf.rip = pc + tp->ftt_size;

	  
                                                                  
                                                                 
                                                                   
                                                                  
                                                                     
                                                                      
                                                                 
                                                                   
                                            
    
	if (is_enabled) {
		regs64->rax = 1;
		new_pc = regs64->isf.rip;
		goto done;
	}

	  
                                                                  
                                                                
                                                                   
         
    
	switch (tp->ftt_type) {
		case FASTTRAP_T_RET:
		case FASTTRAP_T_RET16:
		{
			user_addr_t dst;
			user_addr_t addr;
			int ret;

			  
                                                               
                                                              
                                             
      
			ret = fasttrap_fuword64((user_addr_t)regs64->isf.rsp, &dst);
			addr = regs64->isf.rsp + sizeof (uint64_t);

			if (ret == -1) {
				fasttrap_sigsegv(p, uthread, (user_addr_t)regs64->isf.rsp);
				new_pc = pc;
				break;
			}

			if (tp->ftt_type == FASTTRAP_T_RET16)
				addr += tp->ftt_dest;

			regs64->isf.rsp = addr;
			new_pc = dst;
			break;
		}

		case FASTTRAP_T_JCC:
		{
			uint_t taken;

			switch (tp->ftt_code) {
				case FASTTRAP_JO:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_OF) != 0;
					break;
				case FASTTRAP_JNO:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_OF) == 0;
					break;
				case FASTTRAP_JB:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_CF) != 0;
					break;
				case FASTTRAP_JAE:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_CF) == 0;
					break;
				case FASTTRAP_JE:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_ZF) != 0;
					break;
				case FASTTRAP_JNE:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_ZF) == 0;
					break;
				case FASTTRAP_JBE:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_CF) != 0 ||
						(regs64->isf.rflags & FASTTRAP_EFLAGS_ZF) != 0;
					break;
				case FASTTRAP_JA:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_CF) == 0 &&
						(regs64->isf.rflags & FASTTRAP_EFLAGS_ZF) == 0;
					break;
				case FASTTRAP_JS:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_SF) != 0;
					break;
				case FASTTRAP_JNS:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_SF) == 0;
					break;
				case FASTTRAP_JP:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_PF) != 0;
					break;
				case FASTTRAP_JNP:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_PF) == 0;
					break;
				case FASTTRAP_JL:
					taken = ((regs64->isf.rflags & FASTTRAP_EFLAGS_SF) == 0) !=
						((regs64->isf.rflags & FASTTRAP_EFLAGS_OF) == 0);
					break;
				case FASTTRAP_JGE:
					taken = ((regs64->isf.rflags & FASTTRAP_EFLAGS_SF) == 0) ==
						((regs64->isf.rflags & FASTTRAP_EFLAGS_OF) == 0);
					break;
				case FASTTRAP_JLE:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_ZF) != 0 ||
						((regs64->isf.rflags & FASTTRAP_EFLAGS_SF) == 0) !=
						((regs64->isf.rflags & FASTTRAP_EFLAGS_OF) == 0);
					break;
				case FASTTRAP_JG:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_ZF) == 0 &&
						((regs64->isf.rflags & FASTTRAP_EFLAGS_SF) == 0) ==
						((regs64->isf.rflags & FASTTRAP_EFLAGS_OF) == 0);
					break;
				default:
					taken = FALSE;
			}

			if (taken)
				new_pc = tp->ftt_dest;
			else
				new_pc = pc + tp->ftt_size;
			break;
		}

		case FASTTRAP_T_LOOP:
		{
			uint_t taken;
			uint64_t cx = regs64->rcx--;

			switch (tp->ftt_code) {
				case FASTTRAP_LOOPNZ:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_ZF) == 0 &&
						cx != 0;
					break;
				case FASTTRAP_LOOPZ:
					taken = (regs64->isf.rflags & FASTTRAP_EFLAGS_ZF) != 0 &&
						cx != 0;
					break;
				case FASTTRAP_LOOP:
					taken = (cx != 0);
					break;
				default:
					taken = FALSE;
			}

			if (taken)
				new_pc = tp->ftt_dest;
			else
				new_pc = pc + tp->ftt_size;
			break;
		}

		case FASTTRAP_T_JCXZ:
		{
			uint64_t cx = regs64->rcx;

			if (cx == 0)
				new_pc = tp->ftt_dest;
			else
				new_pc = pc + tp->ftt_size;
			break;
		}

		case FASTTRAP_T_PUSHL_EBP:
		{
			user_addr_t addr = regs64->isf.rsp - sizeof (uint64_t);
			int ret = fasttrap_suword64(addr, (uint64_t)regs64->rbp);

			if (ret == -1) {
				fasttrap_sigsegv(p, uthread, addr);
				new_pc = pc;
				break;
			}

			regs64->isf.rsp = addr;
			new_pc = pc + tp->ftt_size;
			break;
		}

		case FASTTRAP_T_NOP:
			new_pc = pc + tp->ftt_size;
			break;

		case FASTTRAP_T_JMP:
		case FASTTRAP_T_CALL:
			if (tp->ftt_code == 0) {
				new_pc = tp->ftt_dest;
			} else {
				user_addr_t value, addr = tp->ftt_dest;

				if (tp->ftt_base != FASTTRAP_NOREG)
					addr += fasttrap_getreg(regs, tp->ftt_base);
				if (tp->ftt_index != FASTTRAP_NOREG)
					addr += fasttrap_getreg(regs, tp->ftt_index) <<
						tp->ftt_scale;

				if (tp->ftt_code == 1) {
					  
                                            
                                                    
                                                    
                                
        
					if (tp->ftt_segment != FASTTRAP_SEG_NONE &&
					    fasttrap_do_seg(tp, regs, &addr) != 0) {
						fasttrap_sigsegv(p, uthread, addr);
						new_pc = pc;
						break;
					}

					if (fasttrap_fuword64(addr, &value) == -1) {
						fasttrap_sigsegv(p, uthread, addr);
						new_pc = pc;
						break;
					}
					new_pc = value;
				} else {
					new_pc = addr;
				}
			}

			  
                                                               
                                                                
                                                                
                                      
      
			if (tp->ftt_type == FASTTRAP_T_CALL) {
				user_addr_t addr = regs64->isf.rsp - sizeof (uint64_t);
				int ret = fasttrap_suword64(addr, pc + tp->ftt_size);

				if (ret == -1) {
					fasttrap_sigsegv(p, uthread, addr);
					new_pc = pc;
					break;
				}

				regs64->isf.rsp = addr;
			}
			break;

		case FASTTRAP_T_COMMON:
		{
			user_addr_t addr, write_addr;
			uint8_t scratch[2 * FASTTRAP_MAX_INSTR_SIZE + 22];
			uint_t i = 0;

			  
                                 
                                 
     
                                                              
                                                      
     
                         
                                    
                                      
                                   
                                     
                             
               
               
     
                         
                                    
                                      
                         
                                
                                      
                              
                
                
     
                                                               
                                                          
                                                              
                                                              
                                                                 
     
                                                                
                                                             
                                                           
                                                               
                   
     
                              
                              
     
                                                                
                                                             
                                                                
                                                             
                                                               
                                                             
                                                            
                                                           
                                                        
                                                           
                                                             
                                     
     
                                                         
                                                             
                                                     
                                                              
                                                          
                                                               
                                                             
                                                              
                                                            
                                                          
                                                               
                                                          
                               
     
                                  
                                    
                                      
                                     
                         
                                
                                       
                              
                
               
     
                                                            
                                                              
      

			addr = uthread->t_dtrace_scratch->addr;
			write_addr = uthread->t_dtrace_scratch->write_addr;

			if (addr == 0LL || write_addr == 0LL) {
				fasttrap_sigtrap(p, uthread, pc);                                 
				new_pc = pc;
				break;
			}

			ASSERT(tp->ftt_size < FASTTRAP_MAX_INSTR_SIZE);

			uthread->t_dtrace_scrpc = addr;
			bcopy(tp->ftt_instr, &scratch[i], tp->ftt_size);
			i += tp->ftt_size;

			if (tp->ftt_ripmode != 0) {
				uint64_t* reg;

				ASSERT(tp->ftt_ripmode &
				       (FASTTRAP_RIP_1 | FASTTRAP_RIP_2));

				  
                                                         
                                               
                                                        
                                                   
                                                         
                                                        
                          
       
				if (tp->ftt_ripmode & FASTTRAP_RIP_X)
					scratch[i++] = FASTTRAP_REX(1, 0, 0, 1);
				else
					scratch[i++] = FASTTRAP_REX(1, 0, 0, 0);

				if (tp->ftt_ripmode & FASTTRAP_RIP_1)
					scratch[i++] = FASTTRAP_MOV_EAX;
				else
					scratch[i++] = FASTTRAP_MOV_ECX;

				switch (tp->ftt_ripmode) {
					case FASTTRAP_RIP_1:
						reg = &regs64->rax;
						uthread->t_dtrace_reg = REG_RAX;
						break;
					case FASTTRAP_RIP_2:
						reg = &regs64->rcx;
						uthread->t_dtrace_reg = REG_RCX;
						break;
					case FASTTRAP_RIP_1 | FASTTRAP_RIP_X:
						reg = &regs64->r8;
						uthread->t_dtrace_reg = REG_R8;
						break;
					case FASTTRAP_RIP_2 | FASTTRAP_RIP_X:
						reg = &regs64->r9;
						uthread->t_dtrace_reg = REG_R9;
						break;
					default:
						reg = NULL;
						panic("unhandled ripmode in fasttrap_pid_probe64");
				}

				                        
				*(uint64_t *)&scratch[i] = *reg;
				uthread->t_dtrace_regv = *reg;
				*reg = pc + tp->ftt_size;
				i += sizeof (uint64_t);
			}

			  
                                                        
                                                               
                                                              
                                                         
                                            
      
			scratch[i++] = FASTTRAP_GROUP5_OP;
			scratch[i++] = FASTTRAP_MODRM(0, 4, 5);
			                        
			*(uint32_t *)&scratch[i] = 0;
			i += sizeof (uint32_t);
			                        
			*(uint64_t *)&scratch[i] = pc + tp->ftt_size;
			i += sizeof (uint64_t);

			uthread->t_dtrace_astpc = addr + i;
			bcopy(tp->ftt_instr, &scratch[i], tp->ftt_size);
			i += tp->ftt_size;
			scratch[i++] = FASTTRAP_INT;
			scratch[i++] = T_DTRACE_RET;

			ASSERT(i <= sizeof (scratch));

			if (fasttrap_copyout(scratch, write_addr, i)) {
				fasttrap_sigtrap(p, uthread, pc);
				new_pc = pc;
				break;
			}

			if (tp->ftt_retids != NULL) {
				uthread->t_dtrace_step = 1;
				uthread->t_dtrace_ret = 1;
				new_pc = uthread->t_dtrace_astpc;
			} else {
				new_pc = uthread->t_dtrace_scrpc;
			}

			uthread->t_dtrace_pc = pc;
			uthread->t_dtrace_npc = pc + tp->ftt_size;
			uthread->t_dtrace_on = 1;
			break;
		}

		default:
			panic("fasttrap: mishandled an instruction");
	}

done:
	  
               
   
                                                                    
                                                                  
                                                                    
                                                               
    
	regs64->isf.rip = new_pc;


	  
                                                                      
                                                                     
                                                                    
                
    
	if (tp->ftt_retids != NULL) {
		  
                                                             
                                                        
                                              
                                                             
                                                              
     
		if (tp->ftt_type != FASTTRAP_T_COMMON) {
			fasttrap_return_common(regs, pc, pid, new_pc);
		} else {
			ASSERT(uthread->t_dtrace_ret != 0);
			ASSERT(uthread->t_dtrace_pc == pc);
			ASSERT(uthread->t_dtrace_scrpc != 0);
			ASSERT(new_pc == uthread->t_dtrace_astpc);
		}
	}

	return (0);
}

int
fasttrap_pid_probe(x86_saved_state_t *regs)
{
        if (is_saved_state64(regs))
		return fasttrap_pid_probe64(regs);

	return fasttrap_pid_probe32(regs);
}

int
fasttrap_return_probe(x86_saved_state_t *regs)
{
	x86_saved_state64_t *regs64;
	x86_saved_state32_t *regs32;
	unsigned int p_model;

        if (is_saved_state64(regs)) {
                regs64 = saved_state64(regs);
		regs32 = NULL;
		p_model = DATAMODEL_LP64;
        } else {
		regs64 = NULL;
                regs32 = saved_state32(regs);
		p_model = DATAMODEL_ILP32;
        }

	proc_t *p = current_proc();
	uthread_t uthread = (uthread_t)get_bsdthread_info(current_thread());
	user_addr_t pc = uthread->t_dtrace_pc;
	user_addr_t npc = uthread->t_dtrace_npc;

	uthread->t_dtrace_pc = 0;
	uthread->t_dtrace_npc = 0;
	uthread->t_dtrace_scrpc = 0;
	uthread->t_dtrace_astpc = 0;

	  
                                                                 
                                                                     
                      
    
	proc_list_lock();
	while (p->p_lflag & P_LINVFORK)
		p = p->p_pptr;
	proc_list_unlock();

	  
                                                               
                                                                
                                                             
                                                           
                                           
    
	if (p_model == DATAMODEL_LP64)
		regs64->isf.rip = pc;
	else
		regs32->eip = pc;

	fasttrap_return_common(regs, pc, p->p_pid, npc);

	return (0);
}

uint64_t
fasttrap_pid_getarg(void *arg, dtrace_id_t id, void *parg, int argno,
    int aframes)
{
	pal_register_cache_state(current_thread(), VALID);
#pragma unused(arg, id, parg, aframes)
	return (fasttrap_anarg((x86_saved_state_t *)find_user_regs(current_thread()), 1, argno));
}

uint64_t
fasttrap_usdt_getarg(void *arg, dtrace_id_t id, void *parg, int argno,
    int aframes)
{
	pal_register_cache_state(current_thread(), VALID);
#pragma unused(arg, id, parg, aframes)
	return (fasttrap_anarg((x86_saved_state_t *)find_user_regs(current_thread()), 0, argno));
}

  
                                                                       
                                   
   
static user_addr_t
fasttrap_getreg(x86_saved_state_t *regs, uint_t reg)
{
	if (is_saved_state64(regs)) {
		x86_saved_state64_t *regs64 = saved_state64(regs);

		switch (reg) {
			case REG_RAX:		return regs64->rax;
			case REG_RCX:		return regs64->rcx;
			case REG_RDX:		return regs64->rdx;
			case REG_RBX:		return regs64->rbx;
			case REG_RSP:		return regs64->isf.rsp;
			case REG_RBP:		return regs64->rbp;
			case REG_RSI:		return regs64->rsi;
			case REG_RDI:		return regs64->rdi;
			case REG_R8:		return regs64->r8;
			case REG_R9:		return regs64->r9;
			case REG_R10:		return regs64->r10;
			case REG_R11:		return regs64->r11;
			case REG_R12:		return regs64->r12;
			case REG_R13:		return regs64->r13;
			case REG_R14:		return regs64->r14;
			case REG_R15:		return regs64->r15;
			case REG_TRAPNO:	return regs64->isf.trapno;
			case REG_ERR:		return regs64->isf.err;
			case REG_RIP:		return regs64->isf.rip;
			case REG_CS:		return regs64->isf.cs;
			case REG_RFL:		return regs64->isf.rflags;
			case REG_SS:		return regs64->isf.ss;
			case REG_FS:		return regs64->fs;
			case REG_GS:		return regs64->gs;
			case REG_ES:
			case REG_DS:
			case REG_FSBASE:
			case REG_GSBASE:
				                                                                                     
				panic("dtrace: unimplemented x86_64 getreg()");
		}

		panic("dtrace: unhandled x86_64 getreg() constant");
	} else {
		x86_saved_state32_t *regs32 = saved_state32(regs);

		switch (reg) {
			case REG_RAX:		return regs32->eax;
			case REG_RCX:		return regs32->ecx;
			case REG_RDX:		return regs32->edx;
			case REG_RBX:		return regs32->ebx;
			case REG_RSP:		return regs32->uesp;
			case REG_RBP:		return regs32->ebp;
			case REG_RSI:		return regs32->esi;
			case REG_RDI:		return regs32->edi;
		}

		panic("dtrace: unhandled i386 getreg() constant");
	}

	return 0;
}
