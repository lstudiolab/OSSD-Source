/*
 * Copyright (c) 2017 Apple Inc. All rights reserved.
 *
 * Disassemblers for ARM (arm), Thumb (thumb16), and Thumb2 (thumb32).
 *
 * Each disassembly begins with a call to dtrace_decode_arm or dtrace_decode_thumb. The thumb
 * decoder will then call dtrace_decode_thumb16 or dtrace_decode_thumb32 as appropriate.
 *
 * The respective disassembly functions are all of the form {arm,thumb16,thumb32}_type. They
 * follow the ordering and breakdown in the ARMv7 Architecture Reference Manual.
 */

#include  <sys/fasttrap_isa.h>

#define BITS(x, n, mask) (((x) >> (n)) & (mask))

static uint32_t
thumb32_instword_to_arm(uint16_t hw1, uint16_t hw2)
{
	return (hw1 << 16) | hw2;
}

int dtrace_decode_arm(uint32_t instr);
int dtrace_decode_thumb(uint32_t instr);

  
                                                    
   

static
int
vfp_struct_loadstore(uint32_t instr)
{
	if (ARM_RM(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
vfp_64transfer(uint32_t instr)
{
	                                          
	if (ARM_RD(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
vfp_transfer(uint32_t instr)
{
	                                        
	if (ARM_RD(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
vfp_loadstore(uint32_t instr)
{
	int opcode = BITS(instr, 20, 0x1F);

	                     
	if ((opcode & 0x13) == 0x11 && ARM_RN(instr) == REG_PC) {
		return FASTTRAP_T_VLDR_PC_IMMED;
	}

	                                        
	if (ARM_RN(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

  
              
   

static
int
arm_unconditional_misc(uint32_t instr)
{
	int op = BITS(instr, 20, 0x7F);

	if ((op & 0x60) == 0x20) {
		                                                
		return FASTTRAP_T_COMMON;
	}

	if ((op & 0x71) == 0x40) {
		return vfp_struct_loadstore(instr);
	}

	return FASTTRAP_T_INV;
}

static
int
arm_unconditional(uint32_t instr)
{
	if (BITS(instr, 27, 0x1) == 0) {
		return arm_unconditional_misc(instr);
	}

	                                                          

	                                                                                

	return FASTTRAP_T_INV;
}

static
int
arm_syscall_coproc(uint32_t instr)
{
	                                                                      

	int op1 = BITS(instr, 20, 0x3F), coproc = BITS(instr, 8, 0xF), op = BITS(instr, 4, 0x1);

	if ((op1 & 0x3E) == 0 || (op1 & 0x30) == 0x30) {
		                      
		return FASTTRAP_T_INV;
	}

	if ((coproc & 0xE) == 0xA) {
		                     

		if ((op1 & 0x20) == 0 && (op1 & 0x3A) != 0) {
			return vfp_loadstore(instr);
		}

		if ((op1 & 0x3E) == 0x04) {
			return vfp_64transfer(instr);
		}

		if ((op1 & 0x30) == 0x20) {
			                                                                              
			if (op == 0) {
				                                                
				return FASTTRAP_T_COMMON;
			} else {
				return vfp_transfer(instr);
			}
		}
	}

	return FASTTRAP_T_INV;
}

static
int
arm_branch_link_blockdata(uint32_t instr)
{
	int branch = BITS(instr, 25, 0x1), link = BITS(instr, 24, 0x1), op = BITS(instr, 20, 0x1F), uses_pc = BITS(instr, 15, 0x1), uses_lr = BITS(instr, 14, 0x1);

	if (branch == 1) {
		if (link == 0) {
			return FASTTRAP_T_B_COND;
		}
		return FASTTRAP_T_INV;
	} else {
		                                                                                        
		if (op == 0x0B && ARM_RN(instr) == REG_SP && uses_pc == 1) {
			return FASTTRAP_T_LDM_PC;
		}

		                                                                                           
		if (op == 0x12 && ARM_RN(instr) == REG_SP && uses_lr == 1) {
			return FASTTRAP_T_STM_LR;
		}

		if (ARM_RN(instr) != REG_PC && uses_pc == 0) {
			return FASTTRAP_T_COMMON;
		}
	}

	return FASTTRAP_T_INV;
}

static
int
arm_signed_multiplies(uint32_t instr)
{
	int op1 = BITS(instr, 20, 0x7), op2 = BITS(instr, 5, 0x7);

	                                                                
	if ((op1 == 0x4 && (op2 & 0x4) == 0) || (op1 == 0x5 && (op2 & 0x6) == 0x6)) {
		if (ARM_RD(instr) == REG_PC) {
			return FASTTRAP_T_INV;
		}
	}

	if (ARM_RM(instr) != REG_PC && ARM_RS(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
arm_pack_unpack_sat_reversal(uint32_t instr)
{
	int op1 = BITS(instr, 20, 0x7), op2 = BITS(instr, 5, 0x7);

	                                              
	if ((op1 == 0 && (op2 & 0x1) == 0) || (op1 == 0 && op2 == 0x5)) {
		if (ARM_RN(instr) == REG_PC) {
			return FASTTRAP_T_INV;
		}
	}

	if (ARM_RM(instr) != REG_PC && ARM_RD(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
arm_parallel_addsub_unsigned(uint32_t instr)
{
	if (ARM_RM(instr) != REG_PC && ARM_RD(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
arm_parallel_addsub_signed(uint32_t instr)
{
	if (ARM_RM(instr) != REG_PC && ARM_RD(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
arm_media(uint32_t instr)
{
	int op1 = BITS(instr, 20, 0x1F), op2 = BITS(instr, 5, 0x7);

	if ((op1 & 0x1C) == 0) {
		return arm_parallel_addsub_signed(instr);
	}

	if ((op1 & 0x1C) == 0x04) {
		return arm_parallel_addsub_unsigned(instr);
	}

	if ((op1 & 0x18) == 0x08) {
		return arm_pack_unpack_sat_reversal(instr);
	}

	if ((op1 & 0x18) == 0x10) {
		return arm_signed_multiplies(instr);
	}

	if (op1 == 0x1F && op2 == 0x7) {
		                           
		return FASTTRAP_T_INV;
	}

	if (op1 == 0x18 && op2 == 0) {
		                  
		                                                                                   
                                               

		if (ARM_RM(instr) != REG_PC && ARM_RS(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}

		return FASTTRAP_T_INV;
	}

	if ((op1 & 0x1E) == 0x1C && (op2 & 0x3) == 0) {
		             
		if (ARM_RD(instr) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}

		return FASTTRAP_T_INV;
	}

	if (((op1 & 0x1E) == 0x1A || (op1 & 0x1E) == 0x1E) && ((op2 & 0x3) == 0x2)) {
		               
		if (ARM_RM(instr) != REG_PC && ARM_RD(instr) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}

		return FASTTRAP_T_INV;
	}

	return FASTTRAP_T_INV;
}

static
int
arm_loadstore_wordbyte(uint32_t instr)
{
	                                                                                 
	int R = BITS(instr, 25, 0x1), L = BITS(instr, 20, 0x1);

	if (R == 1) {
		                               
		if (ARM_RM(instr) != REG_PC && ARM_RD(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	} else {
		                                                                    
		if (L == 1 && ARM_RN(instr) == REG_PC && ARM_RD(instr) != REG_PC) {
			return FASTTRAP_T_LDR_PC_IMMED;
		}

		if (ARM_RD(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	}

	return FASTTRAP_T_INV;
}

static
int
arm_saturating(uint32_t instr)
{
	if (ARM_RM(instr) != REG_PC && ARM_RD(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
arm_misc(uint32_t instr)
{
	int op = BITS(instr, 21, 0x3), __unused op1 = BITS(instr, 16, 0xF), op2 = BITS(instr, 4, 0x7);

	if (op2 == 1 && op == 1) {
		return FASTTRAP_T_BX_REG;
	}

	                                                                                              
                                                                                               
                                                                           
    

	if (op2 == 0x5) {
		return arm_saturating(instr);
	}

	return FASTTRAP_T_INV;
}

static
int
arm_msr_hints(__unused uint32_t instr)
{
	                                               

	return FASTTRAP_T_INV;
}

static
int
arm_sync_primitive(__unused uint32_t instr)
{
	                                                                                          
	                              

	return FASTTRAP_T_INV;
}

static
int
arm_extra_loadstore_unpriv(uint32_t instr)
{
	int op = BITS(instr, 20, 0x1), __unused op2 = BITS(instr, 5, 0x3), immed = BITS(instr, 22, 0x1);

	if (op == 0 && (op2 & 0x2) == 0x2) {
		                                
		return FASTTRAP_T_INV;
	}

	if (immed == 1) {
		if (ARM_RD(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	} else {
		if (ARM_RM(instr) != REG_PC && ARM_RD(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	}

	return FASTTRAP_T_INV;
}

static
int
arm_extra_loadstore(uint32_t instr)
{
	int op1 = BITS(instr, 20, 0x1F);

	                                                                                     

	if ((op1 & 0x4) == 0) {
		                         
		if (ARM_RM(instr) != REG_PC && ARM_RD(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	} else {
		                          
		if (ARM_RD(instr) != REG_PC && ARM_RN(instr) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	}

	return FASTTRAP_T_INV;
}

static
int
arm_halfword_multiply(uint32_t instr)
{
	                                                                                              
                                                            
    

	if (ARM_RN(instr) != REG_PC && ARM_RD(instr) != REG_PC && ARM_RS(instr) != REG_PC && ARM_RM(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
arm_multiply(uint32_t instr)
{
	                                                                                              
                                                            
    

	if (ARM_RN(instr) != REG_PC && ARM_RD(instr) != REG_PC && ARM_RS(instr) != REG_PC && ARM_RM(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
arm_dataproc_immed(uint32_t instr)
{
	                                                                                                                   
	if (ARM_RN(instr) != REG_PC && ARM_RD(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
arm_dataproc_regshift(uint32_t instr)
{
	                                                                                                                        
	if (ARM_RN(instr) != REG_PC && ARM_RD(instr) != REG_PC && ARM_RS(instr) != REG_PC && ARM_RM(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
arm_dataproc_reg(uint32_t instr)
{
	int op1 = BITS(instr, 20, 0x1F), op2 = BITS(instr, 7, 0x1F), op3 = BITS(instr, 5, 0x3);

	if (op1 == 0x11 || op1 == 0x13 || op1 == 0x15 || op1 == 0x17) {
		                                                                       
		if (ARM_RN(instr) != REG_PC && ARM_RM(instr) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}

		return FASTTRAP_T_INV;
	}

	                                                                                           
                                                                                           
                                       
    
	if (op1 == 0x1A && op2 == 0 && op3 == 0 && ARM_RD(instr) == REG_PC) {
		return FASTTRAP_T_MOV_PC_REG;
	}

	                                                                                                           
	if (ARM_RN(instr) != REG_PC && ARM_RD(instr) != REG_PC && ARM_RM(instr) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
arm_dataproc_misc(uint32_t instr)
{
	int op = BITS(instr, 25, 0x1), op1 = BITS(instr, 20, 0x1F), op2 = BITS(instr, 4, 0xF);

	if (op == 0) {
		if ((op1 & 0x19) != 0x10 && (op2 & 0x1) == 0) {
			return arm_dataproc_reg(instr);
		}

		if ((op1 & 0x19) != 0x10 && (op2 & 0x9) == 0x1) {
			return arm_dataproc_regshift(instr);
		}

		if ((op1 & 0x19) == 0x10 && (op2 & 0x8) == 0) {
			return arm_misc(instr);
		}

		if ((op1 & 0x19) == 0x19 && (op2 & 0x9) == 0x8) {
			return arm_halfword_multiply(instr);
		}

		if ((op1 & 0x10) == 0 && op2 == 0x9) {
			return arm_multiply(instr);
		}

		if ((op1 & 0x10) == 0x10 && op2 == 0x9) {
			return arm_sync_primitive(instr);
		}

		if ((op1 & 0x12) != 0x02 && (op2 == 0xB || (op2 & 0xD) == 0xD)) {
			return arm_extra_loadstore(instr);
		}

		if ((op1 & 0x12) == 0x02 && (op2 == 0xB || (op2 & 0xD) == 0xD)) {
			return arm_extra_loadstore_unpriv(instr);
		}
	} else {
		if ((op1 & 0x19) != 0x10) {
			return arm_dataproc_immed(instr);
		}

		if (op1 == 0x10) {
			                                                       
			if (ARM_RD(instr) != REG_PC) {
				return FASTTRAP_T_COMMON;
			}

			return FASTTRAP_T_INV;
		}

		if (op1 == 0x14) {
			                                                              
			if (ARM_RD(instr) != REG_PC) {
				return FASTTRAP_T_COMMON;
			}

			return FASTTRAP_T_INV;
		}

		if ((op1 & 0x1B) == 0x12) {
			return arm_msr_hints(instr);
		}
	}

	return FASTTRAP_T_INV;
}

int
dtrace_decode_arm(uint32_t instr)
{
	int cond = BITS(instr, 28, 0xF), op1 = BITS(instr, 25, 0x7), op = BITS(instr, 4, 0x1);

	if (cond == 0xF) {
		return arm_unconditional(instr);
	}

	if ((op1 & 0x6) == 0) {
		return arm_dataproc_misc(instr);
	}

	if (op1 == 0x2) {
		return arm_loadstore_wordbyte(instr);
	}

	if (op1 == 0x3 && op == 0) {
		return arm_loadstore_wordbyte(instr);
	}

	if (op1 == 0x3 && op == 1) {
		return arm_media(instr);
	}

	if ((op1 & 0x6) == 0x4) {
		return arm_branch_link_blockdata(instr);
	}

	if ((op1 & 0x6) == 0x6) {
		return arm_syscall_coproc(instr);
	}

	return FASTTRAP_T_INV;
}


  
                       
   

static
int
thumb16_cond_supervisor(uint16_t instr)
{
	int opcode = BITS(instr, 8, 0xF);

	if ((opcode & 0xE) != 0xE) {
		return FASTTRAP_T_B_COND;
	}

	return FASTTRAP_T_INV;
}

static
int
thumb16_misc(uint16_t instr)
{
	int opcode = BITS(instr, 5, 0x7F);

	if ((opcode & 0x70) == 0x30 || (opcode & 0x70) == 0x70) {
		                                                             
		return FASTTRAP_T_INV;
	} else if ((opcode & 0x78) == 0x28) {
		                                                                                         
		return FASTTRAP_T_PUSH_LR;
	} else if ((opcode & 0x78) == 0x68) {
		return FASTTRAP_T_POP_PC;
	} else if ((opcode & 0x28) == 0x08) {
		return FASTTRAP_T_CB_N_Z;
	}

	                                                                         
	return FASTTRAP_T_COMMON;
}

static
int
thumb16_loadstore_single(__unused uint16_t instr)
{
	                                                   
	return FASTTRAP_T_COMMON;
}

static
int
thumb16_data_special_and_branch(uint16_t instr)
{
	int opcode = BITS(instr, 6, 0xF);

	if (opcode == 0x4) {
		                   
		return FASTTRAP_T_INV;
	} else if ((opcode & 0xC) == 0xC) {
		               
		                            
		if ((opcode & 0x2) == 0) {
			return FASTTRAP_T_BX_REG;
		}
		return FASTTRAP_T_INV;
	} else {
		                                                                    
		if ((opcode & 0xC) == 0x8 && THUMB16_HRD(instr) == REG_PC) {
			return FASTTRAP_T_CPY_PC;
		}

		if (THUMB16_HRM(instr) != REG_PC && THUMB16_HRD(instr) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	}

	return FASTTRAP_T_INV;
}

static
int
thumb16_data_proc(__unused uint16_t instr)
{
	                                             
	return FASTTRAP_T_COMMON;
}

static
int
thumb16_shift_addsub_move_compare(__unused uint16_t instr)
{
	                                             
	return FASTTRAP_T_COMMON;
}

static
int
dtrace_decode_thumb16(uint16_t instr)
{
	int opcode = BITS(instr, 10, 0x3F);

	if ((opcode & 0x30) == 0) {
		return thumb16_shift_addsub_move_compare(instr);
	}

	if (opcode == 0x10) {
		return thumb16_data_proc(instr);
	}

	if (opcode == 0x11) {
		return thumb16_data_special_and_branch(instr);
	}

	if ((opcode & 0x3E) == 0x12) {
		                   
		return FASTTRAP_T_LDR_PC_IMMED;
	}

	if ((opcode & 0x3C) == 0x14 || (opcode & 0x38) == 0x18 || (opcode & 0x38) == 0x20) {
		return thumb16_loadstore_single(instr);
	}

	if ((opcode & 0x3E) == 0x28) {
		                      
		return FASTTRAP_T_INV;
	}

	if ((opcode & 0x3E) == 0x2A) {
		                             
		return FASTTRAP_T_COMMON;
	}

	if ((opcode & 0x3C) == 0x2C) {
		return thumb16_misc(instr);
	}

	if ((opcode & 0x3E) == 0x30) {
		                                       
		return FASTTRAP_T_COMMON;
	}

	if ((opcode & 0x3E) == 0x32) {
		                                       
		return FASTTRAP_T_COMMON;
	}

	if ((opcode & 0x3C) == 0x34) {
		return thumb16_cond_supervisor(instr);
	}

	if ((opcode & 0x3E) == 0x38) {
		                     
		return FASTTRAP_T_B_UNCOND;
	}

	return FASTTRAP_T_INV;
}

  
                       
   

static
int
thumb32_coproc(uint16_t instr1, uint16_t instr2)
{
	                                                                      

	int op1 = BITS(instr1, 4, 0x3F), coproc = BITS(instr2, 8, 0xF), op = BITS(instr2, 4, 0x1);

	if ((op1 & 0x3E) == 0) {
		               
		return FASTTRAP_T_INV;
	}

	if ((coproc & 0xE) == 0xA || (op1 & 0x30) == 0x30) {
		                     
		uint32_t instr = thumb32_instword_to_arm(instr1, instr2);

		if ((op1 & 0x30) == 0x30) {
			                                                
			return FASTTRAP_T_COMMON;
		}

		if ((op1 & 0x3A) == 0x02 || (op1 & 0x38) == 0x08 || (op1 & 0x30) == 0x10) {
			return vfp_loadstore(instr);
		}

		if ((op1 & 0x3E) == 0x04) {
			return vfp_64transfer(instr);
		}

		if ((op1 & 0x30) == 0x20) {
			                                                                              
			if (op == 0) {
				                                                
				return FASTTRAP_T_COMMON;
			} else {
				return vfp_transfer(instr);
			}
		}
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_longmultiply(uint16_t instr1, uint16_t instr2)
{
	int op1 = BITS(instr1, 4, 0x7), op2 = BITS(instr2, 4, 0xF);

	if ((op1 == 1 && op2 == 0xF) || (op1 == 0x3 && op2 == 0xF)) {
		                                
		if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RD(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	} else {
		                               
		if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RD(instr1, instr2) != REG_PC &&
		    THUMB32_RT(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_multiply(uint16_t instr1, uint16_t instr2)
{
	int op1 = BITS(instr1, 4, 0x7), op2 = BITS(instr2, 4, 0x3);

	if ((op1 == 0 && op2 == 1) || (op1 == 0x6 && (op2 & 0x2) == 0)) {
		if (THUMB32_RT(instr1, instr2) == REG_PC) {
			return FASTTRAP_T_INV;
		}
	}

	if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RD(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_misc(uint16_t instr1, uint16_t instr2)
{
	if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RD(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_parallel_addsub_unsigned(uint16_t instr1, uint16_t instr2)
{
	if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RD(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_parallel_addsub_signed(uint16_t instr1, uint16_t instr2)
{
	if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RD(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_dataproc_reg(uint16_t instr1, uint16_t instr2)
{
	int op1 = BITS(instr1, 4, 0xF), op2 = BITS(instr2, 4, 0xF);

	if (((0 <= op1) && (op1 <= 5)) && (op2 & 0x8) == 0x8) {
		if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RD(instr1, instr2) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	}

	if ((op1 & 0x8) == 0 && op2 == 0) {
		if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RD(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	}

	if ((op1 & 0x8) == 0x8 && (op2 & 0xC) == 0) {
		return thumb32_parallel_addsub_signed(instr1, instr2);
	}

	if ((op1 & 0x8) == 0x8 && (op2 & 0xC) == 0x4) {
		return thumb32_parallel_addsub_unsigned(instr1, instr2);
	}

	if ((op1 & 0xC) == 0x8 && (op2 & 0xC) == 0x8) {
		return thumb32_misc(instr1, instr2);
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_dataproc_regshift(uint16_t instr1, uint16_t instr2)
{
	int op = BITS(instr1, 5, 0xF), S = BITS(instr1, 4, 0x1);

	if (op == 0 || op == 0x4 || op == 0x8 || op == 0xD) {
		                                                                                                  
		if (S == 1) {
			if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
				return FASTTRAP_T_COMMON;
			}
		} else {
			if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RD(instr1, instr2) != REG_PC &&
			    THUMB32_RN(instr1, instr2) != REG_PC) {
				return FASTTRAP_T_COMMON;
			}
		}
	} else if (op == 0x2 || op == 0x3) {
		                                                                                                         
		if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RD(instr1, instr2) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	} else {
		                                       
		if (THUMB32_RM(instr1, instr2) != REG_PC && THUMB32_RD(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_store_single(uint16_t instr1, uint16_t instr2)
{
	int op1 = BITS(instr1, 5, 0x7), op2 = BITS(instr2, 6, 0x3F);

	                                          
	if ((op1 == 0 || op1 == 1 || op1 == 2) && (op2 & 0x20) == 0) {
		                            
		if (THUMB32_RM(instr1, instr2) == REG_PC) {
			return FASTTRAP_T_INV;
		}
	}

	if (THUMB32_RT(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_loadbyte_memhint(uint16_t instr1, uint16_t instr2)
{
	int op1 = BITS(instr1, 7, 0x3), __unused op2 = BITS(instr2, 6, 0x3F);

	                                          
	if ((op1 == 0 || op1 == 0x2) && THUMB32_RM(instr1, instr2) == REG_PC) {
		return FASTTRAP_T_INV;
	}

	if (THUMB32_RT(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_loadhalfword_memhint(uint16_t instr1, uint16_t instr2)
{
	int op1 = BITS(instr1, 7, 0x3), op2 = BITS(instr2, 6, 0x3F);

	                                          
	if (op1 == 0 && op2 == 0 && THUMB32_RM(inst1, instr2) == REG_PC) {
		return FASTTRAP_T_INV;
	}

	if (THUMB32_RT(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_loadword(uint16_t instr1, uint16_t instr2)
{
	int op1 = BITS(instr1, 7, 0x3), op2 = BITS(instr2, 6, 0x3F);

	if ((op1 & 0x2) == 0 && THUMB32_RN(instr1, instr2) == REG_PC && THUMB32_RT(instr1, instr2) != REG_PC) {
		return FASTTRAP_T_LDR_PC_IMMED;
	}

	if (op1 == 0 && op2 == 0) {
		                                           
		if (THUMB32_RM(instr1, instr2) == REG_PC) {
			return FASTTRAP_T_INV;
		}
	}

	if (THUMB32_RT(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_loadstore_double_exclusive_table(__unused uint16_t instr1, __unused uint16_t instr2)
{
	                                   

	return FASTTRAP_T_INV;
}

static
int
thumb32_loadstore_multiple(uint16_t instr1, uint16_t instr2)
{
	int op = BITS(instr1, 7, 0x3), L = BITS(instr1, 4, 0x1), uses_pc = BITS(instr2, 15, 0x1), uses_lr = BITS(instr2, 14, 0x1);

	if (op == 0 || op == 0x3) {
		                                       
		return FASTTRAP_T_INV;
	}

	                                                                                                            
	if (op == 0x1 && L == 1 && THUMB32_RN(instr1, instr2) == REG_SP && uses_pc == 1) {
		return FASTTRAP_T_LDM_PC;
	}

	                                                                                                                
	if (op == 0x2 && L == 0 && THUMB32_RN(instr1, instr2) == REG_SP && uses_lr == 1) {
		return FASTTRAP_T_STM_LR;
	}

	if (THUMB32_RN(instr1, instr2) != REG_PC && uses_pc == 0) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_misc_control(__unused uint16_t instr1, __unused uint16_t instr2)
{
	                                                       
	return FASTTRAP_T_INV;
}

static
int
thumb32_cps_hints(__unused uint16_t instr1, __unused uint16_t instr2)
{
	                
	return FASTTRAP_T_INV;
}

static
int
thumb32_b_misc_control(uint16_t instr1, uint16_t instr2)
{
	int op = BITS(instr1, 4, 0x7F), op1 = BITS(instr2, 12, 0x7), __unused op2 = BITS(instr2, 8, 0xF);

	if ((op1 & 0x5) == 0) {
		if ((op & 0x38) != 0x38) {
			return FASTTRAP_T_B_COND;
		}

		if (op == 0x3A) {
			return thumb32_cps_hints(instr1, instr2);
		}

		if (op == 0x3B) {
			return thumb32_misc_control(instr1, instr2);
		}
	}

	if ((op1 & 0x5) == 1) {
		return FASTTRAP_T_B_UNCOND;
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_dataproc_plain_immed(uint16_t instr1, uint16_t instr2)
{
	int op = BITS(instr1, 4, 0x1F);

	if (op == 0x04 || op == 0x0C || op == 0x16) {
		                         
		                       
		if (THUMB32_RD(instr1, instr2) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	} else {
		if (THUMB32_RD(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	}

	return FASTTRAP_T_INV;
}

static
int
thumb32_dataproc_mod_immed(uint16_t instr1, uint16_t instr2)
{
	int op = BITS(instr1, 5, 0xF), S = BITS(instr1, 4, 0x1);

	if (op == 0x2 || op == 0x3) {
		                                                               
		if (THUMB32_RD(instr1, instr2) != REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	}

	if (op == 0 || op == 0x4 || op == 0x8 || op == 0xD) {
		                                                                           
		if (S && THUMB32_RD(instr1, instr2) == REG_PC) {
			return FASTTRAP_T_COMMON;
		}
	}

	if (THUMB32_RD(instr1, instr2) != REG_PC && THUMB32_RN(instr1, instr2) != REG_PC) {
		return FASTTRAP_T_COMMON;
	}

	return FASTTRAP_T_INV;
}

static
int
dtrace_decode_thumb32(uint16_t instr1, uint16_t instr2)
{
	int op1 = BITS(instr1, 11, 0x3), op2 = BITS(instr1, 4, 0x7F), op = BITS(instr2, 15, 0x1);

	if (op1 == 0x1) {
		if ((op2 & 0x64) == 0) {
			return thumb32_loadstore_multiple(instr1, instr2);
		}

		if ((op2 & 0x64) == 0x04) {
			return thumb32_loadstore_double_exclusive_table(instr1, instr2);
		}

		if ((op2 & 0x60) == 0x20) {
			return thumb32_dataproc_regshift(instr1, instr2);
		}

		if ((op2 & 0x40) == 0x40) {
			return thumb32_coproc(instr1, instr2);
		}
	}

	if (op1 == 0x2) {
		if ((op2 & 0x20) == 0 && op == 0) {
			return thumb32_dataproc_mod_immed(instr1, instr2);
		}

		if ((op2 & 0x20) == 0x20 && op == 0) {
			return thumb32_dataproc_plain_immed(instr1, instr2);
		}

		if (op == 1) {
			return thumb32_b_misc_control(instr1, instr2);
		}
	}

	if (op1 == 0x3) {
		if ((op2 & 0x71) == 0) {
			return thumb32_store_single(instr1, instr2);
		}

		if ((op2 & 0x71) == 0x10) {
			return vfp_struct_loadstore(thumb32_instword_to_arm(instr1, instr2));
		}

		if ((op2 & 0x67) == 0x01) {
			return thumb32_loadbyte_memhint(instr1, instr2);
		}

		if ((op2 & 0x67) == 0x03) {
			return thumb32_loadhalfword_memhint(instr1, instr2);
		}

		if ((op2 & 0x67) == 0x05) {
			return thumb32_loadword(instr1, instr2);
		}

		if ((op2 & 0x67) == 0x07) {
			                           
			return FASTTRAP_T_INV;
		}

		if ((op2 & 0x70) == 0x20) {
			return thumb32_dataproc_reg(instr1, instr2);
		}

		if ((op2 & 0x78) == 0x30) {
			return thumb32_multiply(instr1, instr2);
		}

		if ((op2 & 0x78) == 0x38) {
			return thumb32_longmultiply(instr1, instr2);
		}

		if ((op2 & 0x40) == 0x40) {
			return thumb32_coproc(instr1, instr2);
		}
	}

	return FASTTRAP_T_INV;
}

int
dtrace_decode_thumb(uint32_t instr)
{
	uint16_t* pInstr = (uint16_t*) &instr;
	uint16_t hw1 = pInstr[0], hw2 = pInstr[1];

	int size = BITS(hw1, 11, 0x1F);

	if (size == 0x1D || size == 0x1E || size == 0x1F) {
		return dtrace_decode_thumb32(hw1, hw2);
	} else {
		return dtrace_decode_thumb16(hw1);
	}
}
