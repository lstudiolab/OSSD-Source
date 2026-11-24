/*
 * Copyright (c) 2007-2013 Apple Inc. All rights reserved.
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
#include <arm/proc_reg.h>
#include <arm64/asm.h>
#include <arm64/proc_reg.h>
#include <pexpert/arm64/board_config.h>
#include <mach_assert.h>
#include <machine/asm.h>
#include "assym.s"
#include <arm64/tunables/tunables.s>
#include <arm64/exception_asm.h>

#if __ARM_KERNEL_PROTECT__
#include <arm/pmap.h>
#endif                             



.macro MSR_VBAR_EL1_X0
#if defined(KERNEL_INTEGRITY_KTRR)
	mov	x1, lr
	bl		EXT(pinst_set_vbar)
	mov	lr, x1
#else
	msr		VBAR_EL1, x0
#endif
.endmacro

.macro MSR_TCR_EL1_X1
#if defined(KERNEL_INTEGRITY_KTRR)
	mov		x0, x1
	mov		x1, lr
	bl		EXT(pinst_set_tcr)
	mov		lr, x1
#else
	msr		TCR_EL1, x1
#endif
.endmacro

.macro MSR_TTBR1_EL1_X0
#if defined(KERNEL_INTEGRITY_KTRR)
	mov		x1, lr
	bl		EXT(pinst_set_ttbr1)
	mov		lr, x1
#else
	msr		TTBR1_EL1, x0
#endif
.endmacro

.macro MSR_SCTLR_EL1_X0
#if defined(KERNEL_INTEGRITY_KTRR)
	mov		x1, lr

	                               
	bl		EXT(pinst_spsel_1)

	bl		EXT(pinst_set_sctlr)
	msr		SPSel, #0									              
	mov		lr, x1
#else
	msr		SCTLR_EL1, x0
#endif                                     
.endmacro

  
                                                                               
                                                                              
                                                               
  
             
                                   
                          
                         
   
	.text
	.align 12
	.globl EXT(LowResetVectorBase)
LEXT(LowResetVectorBase)
	  
                                                                     
                                                                 
                                                              
                                                               
                                                                 
                                 
    
	b		EXT(reset_vector)

	                                                                               
	.align	9
	b		.
	.align	7
	b		.
	.align	7
	b		.
	.align	7
	b		.

	.align	7
	.globl EXT(reset_vector)
LEXT(reset_vector)
	                                             
	                                
	msr		OSLAR_EL1, xzr
	msr		DAIFSet, #(DAIFSC_ALL)				                         

#if !(defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR))
	                                                   
	adrp    x0, EXT(LowExceptionVectorBase)@page
	add     x0, x0, EXT(LowExceptionVectorBase)@pageoff
	msr     VBAR_EL1, x0
#endif



	                         
	adrp	x19, EXT(ResetHandlerData)@page			                                        
	add		x19, x19, EXT(ResetHandlerData)@pageoff
	mrs		x15, MPIDR_EL1						                               
#if HAS_CLUSTER
	and		x0, x15, #0xFFFF					                                                   
#else
	and		x0, x15, #0xFF						                                          
#endif
	ldr		x1, [x19, CPU_DATA_ENTRIES]			                             
	add		x3, x1, MAX_CPUS * 16				                                                     
Lcheck_cpu_data_entry:
	ldr		x21, [x1, CPU_DATA_PADDR]			                                 
	cbz		x21, Lnext_cpu_data_entry
	ldr		w2, [x21, CPU_PHYS_ID]				                       
	cmp		x0, x2						                                                   
	b.eq	Lfound_cpu_data_entry				                  
Lnext_cpu_data_entry:
	add		x1, x1, #16					                                       
	cmp		x1, x3
	b.eq	Lskip_cpu_reset_handler				            
	b		Lcheck_cpu_data_entry	       
Lfound_cpu_data_entry:
	adrp	x20, EXT(const_boot_args)@page
	add		x20, x20, EXT(const_boot_args)@pageoff
	ldr		x0, [x21, CPU_RESET_HANDLER]		                         
	cbz		x0, Lskip_cpu_reset_handler

	                                                                
	adrp	x2, EXT(resume_idle_cpu)@page
	add		x2, x2, EXT(resume_idle_cpu)@pageoff
	cmp		x0, x2
	beq		1f
	adrp	x2, EXT(start_cpu)@page
	add		x2, x2, EXT(start_cpu)@pageoff
	cmp		x0, x2
	bne		Lskip_cpu_reset_handler
1:

#if HAS_BP_RET
	bl		EXT(set_bp_ret)
#endif

#if __ARM_KERNEL_PROTECT__ && defined(KERNEL_INTEGRITY_KTRR)
	  
                                                                
                        
    
	ldr		x13, [x21, CPU_ACTIVE_THREAD]
	msr		TPIDR_EL1, x13
#endif                             

	blr		x0
Lskip_cpu_reset_handler:
	b		.									                                         

	.align 3
	.global EXT(LowResetVectorEnd)
LEXT(LowResetVectorEnd)
	.global	EXT(SleepToken)
#if WITH_CLASSIC_S2R
LEXT(SleepToken)
	.space	(stSize_NUM),0
#endif

	.section __DATA_CONST,__const
	.align	3
	.globl  EXT(ResetHandlerData)
LEXT(ResetHandlerData)
	.space  (rhdSize_NUM),0		                   
	.text


  
                                                                             
                                                                           
                                                                  
   
	.align	3
	.globl EXT(_start)
LEXT(_start)
	b	EXT(start_first_cpu)


  
                                                                          
                                                                              
                                                                         
                                    
   
	.align 12, 0
	.global	EXT(LowExceptionVectorBase)
LEXT(LowExceptionVectorBase)
	              
	b		.
	.align	7
	b		.
	.align	7
	b		.
	.align	7
	b		.
	             
	.align	7
	b		.
	.align	7
	b		.
	.align	7
	b		.
	.align	7
	b		.
	            
	.align	7
	b		.
	.align	7
	b		.
	.align	7
	b		.
	.align	7
	b		.
	            
	.align	7
	b		.
	.align	7
	b		.
	.align	7
	b		.
	.align	7
	b		.
	.align 12, 0

#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
  
                                                                         
                                
   
.align ARM_PGSHIFT
.globl EXT(bootstrap_instructions)
LEXT(bootstrap_instructions)

#endif                                                                       
	.align 2
	.globl EXT(resume_idle_cpu)
LEXT(resume_idle_cpu)
	adrp	lr, EXT(arm_init_idle_cpu)@page
	add		lr, lr, EXT(arm_init_idle_cpu)@pageoff
	b		start_cpu

	.align 2
	.globl EXT(start_cpu)
LEXT(start_cpu)
	adrp	lr, EXT(arm_init_cpu)@page
	add		lr, lr, EXT(arm_init_cpu)@pageoff
	b		start_cpu

	.align 2
start_cpu:
#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
	                                                               
	                                                             
	adrp	x0, EXT(LowExceptionVectorBase)@page
	add		x0, x0, EXT(LowExceptionVectorBase)@pageoff
	MSR_VBAR_EL1_X0
#endif                                                                       

	                                   
	                                   

	                                                      
	ldr		x22, [x20, BA_VIRT_BASE]			                           
	ldr		x23, [x20, BA_PHYS_BASE]			                           
	ldr		x24, [x20, BA_MEM_SIZE]				                               
	adrp	x25, EXT(bootstrap_pagetables)@page	                                   
	ldr		x26, [x20, BA_BOOT_FLAGS]			                            


	                                      
	ldr		x0, [x21, CPU_NUMBER_GS]
	msr		TPIDRRO_EL0, x0

	                                  
	ldr		x0, [x21, CPU_EXCEPSTACK_TOP]


	                                
#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
	mov		x1, lr
	bl		EXT(pinst_spsel_1)
	mov		lr, x1
#else
	msr		SPSel, #1
#endif
	mov		sp, x0

	                                  
	ldr		x0, [x21, CPU_INTSTACK_TOP]
	msr		SPSel, #0
	mov		sp, x0

	                    
	add		lr, lr, x22
	sub		lr, lr, x23

	b		common_start

  
                        
  
                                                                            
                                       
                           
                            
                            
                            
                            
                            
   
.macro create_l1_table_entry
	and		$3,	$0, #(ARM_TT_L1_INDEX_MASK)
	lsr		$3, $3, #(ARM_TT_L1_SHIFT)			                                     
	lsl		$3, $3, #(TTE_SHIFT)				                                    
	add		$3, $1, $3							                       
	mov		$4, #(ARM_TTE_BOOT_TABLE)			                              
	and		$5, $2, #(ARM_TTE_TABLE_MASK)		                               
	orr		$5, $4, $5 							                                  
	str		$5, [$3]							                          
.endmacro

  
                          
  
                                                                               
                              
                           
                            
                            
                             
                            
                            
                            
                            
   
.macro create_l2_block_entries
	and		$4,	$0, #(ARM_TT_L2_INDEX_MASK)
	lsr		$4, $4, #(ARM_TTE_BLOCK_L2_SHIFT)	                                        
	lsl		$4, $4, #(TTE_SHIFT)				                                    
	add		$4, $2, $4							                       
	mov		$5, #(ARM_TTE_BOOT_BLOCK)			                              
	and		$6, $1, #(ARM_TTE_BLOCK_L2_MASK)	                                    
	orr		$6, $5, $6
	mov		$5, $3
	mov		$7, #(ARM_TT_L2_SIZE)
1:
	str		$6, [$4], #(1 << TTE_SHIFT)			                                      
	add		$6, $6, $7							                               
	subs	$5, $5, #1							                                  
	b.ne	1b
.endmacro

  
                                
                                 
                                   
                           
                             
                                            
                  
                  
                  
                  
   
.macro create_bootstrap_mapping
	                                         
	and	$5, $0, #(ARM_TT_L2_INDEX_MASK)
	lsr	$5, $5, #(ARM_TT_L2_SHIFT)
	mov	$6, #(TTE_PGENTRIES)
	sub	$5, $6, $5

	                          
3:	add	$4, $4, PGBYTES

	                                                                                        
	create_l1_table_entry	$0, $3, $4, $6, $7, $8

	                                                                       
                                               
	cmp	$2, $5
	csel	$5, $2, $5, lt

	                                                                                                     
	create_l2_block_entries	$0, $1, $4, $5, $6, $7, $8, $9

	                                                             
	subs	$2, $2, $5
	beq	2f

	                                                 
	add 	$0, $0, $5, lsl #(ARM_TT_L2_SHIFT)
	add 	$1, $1, $5, lsl #(ARM_TT_L2_SHIFT)

	mov	$5, #(TTE_PGENTRIES)                                                    
	b	3b
2:
.endmacro

  
                   
                                               
                   
   
	.align 2
	.globl EXT(start_first_cpu)
LEXT(start_first_cpu)

	                                
	msr		OSLAR_EL1, xzr
	msr		DAIFSet, #(DAIFSC_ALL)				                         

	mov		x20, x0
	mov		x21, #0

	                                                   
	adrp	x0, EXT(LowExceptionVectorBase)@page
	add		x0, x0, EXT(LowExceptionVectorBase)@pageoff
	MSR_VBAR_EL1_X0


	                                                      
	ldr		x22, [x20, BA_VIRT_BASE]			                           
	ldr		x23, [x20, BA_PHYS_BASE]			                           
	ldr		x24, [x20, BA_MEM_SIZE]				                               
	adrp	x25, EXT(bootstrap_pagetables)@page	                                   
	ldr		x26, [x20, BA_BOOT_FLAGS]			                            

	                                                                                             
	                                                                                      
	                                                                
	msr		TPIDRRO_EL0, x21

	                                 
	adrp	x0, EXT(excepstack_top)@page		                              
	add		x0, x0, EXT(excepstack_top)@pageoff
	add		x0, x0, x22							                 
	sub		x0, x0, x23

	                                
#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
	bl		EXT(pinst_spsel_1)
#else
	msr		SPSel, #1
#endif

	mov		sp, x0

	                                 
	adrp	x0, EXT(intstack_top)@page			                        
	add		x0, x0, EXT(intstack_top)@pageoff
	add		x0, x0, x22							                 
	sub		x0, x0, x23
	msr		SPSel, #0							                                
	mov		sp, x0

	                                                        
	adrp	lr, EXT(arm_init)@page
	add		lr, lr, EXT(arm_init)@pageoff
	add		lr, lr, x22							                 
	sub		lr, lr, x23

	  
                                                                          
                                                                           
                                                   
                         
                         
                         
                         
    

	                                                      
	mov		x0, #(ARM_TTE_EMPTY)				                              
	mov		x1, x25								                              
	mov		x2, #(TTE_PGENTRIES)				                                  
	lsl		x2, x2, #2							                                        

Linvalidate_bootstrap:							       
	str		x0, [x1], #(1 << TTE_SHIFT)			                           
	subs	x2, x2, #1							              
	b.ne	Linvalidate_bootstrap				                         

	  
                                                                          
                                                                        
                                                                      
                            
   
                                                                          
                                                                         
                                                                          
                                                                            
                                     
   
                                                                        
                                                                          
                
    
	adrp	x0, EXT(_mh_execute_header)@page	                                
	add		x0, x0, EXT(_mh_execute_header)@pageoff
	ldr		w1, [x0, #0x18]						                          
	tbz		w1, #0x1f, Lkernelcache_base_found	                                                         
	ldr		w1, [x0, #0x20]						                                                               
	cmp		w1, #0x19							                        
	bne		.
	ldr		x1, [x0, #0x38]						                            
	sub		x1, x0, x1							                
	MOV64	x0, VM_KERNEL_LINK_ADDRESS
	add		x0, x0, x1							                                      

Lkernelcache_base_found:
	  
                                                                      
                                       
                             
                              
                              
    
	sub		x18, x0, x23
	sub		x24, x24, x18
	add		x22, x22, x18
	add		x23, x23, x18

	  
                            
                             
                            
                             
    
	mov		x4, x0
	mov		x14, x22
	mov		x15, x23

	  
                      
                    
                    
                                                                        
                                                                         
                                                                          
                                                                          
                                  
    
	mov		x1, x25
	add		x3, x1, PGBYTES
	mov		x2, x3

	  
                                   
                                               
    
	lsr		x5,  x24, #(ARM_TT_L2_SHIFT)
	                                                                         
	create_bootstrap_mapping x0,  x4,  x5, x1, x2, x6, x10, x11, x12, x13

	                                     
	lsr		x5,  x24, #(ARM_TT_L2_SHIFT)
	create_bootstrap_mapping x14, x15, x5, x3, x2, x9, x10, x11, x12, x13

	                             
	dsb		ish


	b		common_start

  
                                  
  
                 
                        
                                                        
                            
                             
                                     
                              
                        
  
                                        
                                        
                           
   
common_start:

#if HAS_NEX_PG
	mov x19, lr
	bl		EXT(set_nex_pg)
	mov lr, x19
#endif

	                                        
	adrp	x0,     EXT(sysreg_restore)@page		                                                            
	add		x0, x0, EXT(sysreg_restore)@pageoff
	ldr		x1, [x0, SR_RESTORE_TCR_EL1]
	MSR_TCR_EL1_X1

	                                           
                                     
                                              
    
#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
	                                                                     
                  
    
#endif
	and		x0, x25, #(TTBR_BADDR_MASK)
	mov		x19, lr
	bl		EXT(set_mmu_ttb)
	mov		lr, x19
	add		x0, x25, PGBYTES
	and		x0, x0, #(TTBR_BADDR_MASK)
	MSR_TTBR1_EL1_X0

	                                                               
	mov		x0, xzr
	mov		x1, #(MAIR_WRITEBACK << MAIR_ATTR_SHIFT(CACHE_ATTRINDX_WRITEBACK))
	orr		x0, x0, x1
	mov		x1, #(MAIR_INNERWRITEBACK << MAIR_ATTR_SHIFT(CACHE_ATTRINDX_INNERWRITEBACK))
	orr		x0, x0, x1
	mov		x1, #(MAIR_DISABLE << MAIR_ATTR_SHIFT(CACHE_ATTRINDX_DISABLE))
	orr		x0, x0, x1
	mov		x1, #(MAIR_WRITETHRU << MAIR_ATTR_SHIFT(CACHE_ATTRINDX_WRITETHRU))
	orr		x0, x0, x1
	mov		x1, #(MAIR_WRITECOMB << MAIR_ATTR_SHIFT(CACHE_ATTRINDX_WRITECOMB))
	orr		x0, x0, x1
	mov		x1, #(MAIR_POSTED << MAIR_ATTR_SHIFT(CACHE_ATTRINDX_POSTED))
	orr		x0, x0, x1
	mov		x1, #(MAIR_POSTED_REORDERED << MAIR_ATTR_SHIFT(CACHE_ATTRINDX_POSTED_REORDERED))
	orr		x0, x0, x1
	mov		x1, #(MAIR_POSTED_COMBINED_REORDERED << MAIR_ATTR_SHIFT(CACHE_ATTRINDX_POSTED_COMBINED_REORDERED))
	orr		x0, x0, x1
	msr		MAIR_EL1, x0
	isb
	tlbi	vmalle1
	dsb		ish

#if defined(APPLEHURRICANE)
	                                                                                        
	                                         
	HID_INSERT_BITS	HID5, ARM64_REG_HID5_CrdEdbSnpRsvd_mask, ARM64_REG_HID5_CrdEdbSnpRsvd_VALUE, x12
#endif

#if defined(BCM2837)
	                                                                    
	mrs		x15, MPIDR_EL1						                               
	and		x15, x15, #0xFF						                                          
	mov		x0, #0x4000
	lsl		x0, x0, #16
	add		x0, x0, #0x0040						                                               
	add		x0, x0, x15, lsl #2
	mov		w1, #0xF0 						                                 
	str		w1, [x0]
	isb		sy
#endif

#ifndef __ARM_IC_NOALIAS_ICACHE__
	                                                                          
                                    
    
	tlbi	vmalle1
	ic		iallu
#endif

	                                                             
                                                         
                                                     
    

	cbz		x21, 1f
	adrp	x0, EXT(cpu_ttep)@page
	add		x0, x0, EXT(cpu_ttep)@pageoff
	ldr		x0, [x0]
	MSR_TTBR1_EL1_X0
1:

	                               
#if __ARM_KERNEL_PROTECT__
	                                                                        
                                                                       
                                                              
    
	MOV64	x0, ARM_KERNEL_PROTECT_EXCEPTION_START

	cbnz		x21, 1f
#endif                             
	adrp	x0, EXT(ExceptionVectorsBase)@page			                                      
	add		x0, x0, EXT(ExceptionVectorsBase)@pageoff
	add		x0, x0, x22									                                          
	sub		x0, x0, x23
1:
	MSR_VBAR_EL1_X0

1:
#ifdef HAS_APPLE_PAC
#if HAS_PARAVIRTUALIZED_PAC
	mov		x0, #VMAPPLE_PAC_SET_INITIAL_STATE
	hvc		#0
#endif                              

	                                  
	MOV64	x0, SCTLR_EL1_DEFAULT
	orr		x0, x0, #(SCTLR_PACIB_ENABLED)                

	MOV64	x1, SCTLR_JOP_KEYS_ENABLED
	orr 	x0, x0, x1
#else                     

	                        
	MOV64	x0, SCTLR_EL1_DEFAULT
#endif                    
	MSR_SCTLR_EL1_X0
	isb		sy

	MOV64	x1, SCTLR_EL1_DEFAULT
#if HAS_APPLE_PAC
	orr		x1, x1, #(SCTLR_PACIB_ENABLED)
	MOV64	x2, SCTLR_JOP_KEYS_ENABLED
	orr		x1, x1, x2
#endif                    
	cmp		x0, x1
	bne		.

#if (!CONFIG_KERNEL_INTEGRITY || (CONFIG_KERNEL_INTEGRITY && !defined(KERNEL_INTEGRITY_WT)))
	             
   
                                                                            
                              
    

	              
	mov		x0, #(CPACR_FPEN_ENABLE)
	msr		CPACR_EL1, x0
#endif

	                       
	msr		TPIDR_EL1, xzr						                      


#if defined(APPLE_ARM64_ARCH_FAMILY)
	                                                         
#if !APPLEVIRTUALPLATFORM
	ARM64_IS_PCORE x15
	ARM64_READ_EP_SPR x15, x12, EHID4, HID4
	orr		x12, x12, ARM64_REG_HID4_DisDcMVAOps
	orr		x12, x12, ARM64_REG_HID4_DisDcSWL2Ops
	ARM64_WRITE_EP_SPR x15, x12, EHID4, HID4
#endif                          
#endif                            

	                                             
	mrs x12, MIDR_EL1

	APPLY_TUNABLES x12, x13



#if HAS_CLUSTER
	                                                                  
	mrs		x9, CPU_OVRD
	and		x9, x9, #(~(ARM64_REG_CYC_OVRD_irq_mask | ARM64_REG_CYC_OVRD_fiq_mask))
	msr		CPU_OVRD, x9
#endif

	                                                                                      
	cbnz	x21, Ltrampoline

	                                    
	add		x0, x20, x22
	sub		x0, x0, x23

#if KASAN
	mov	x20, x0
	mov	x21, lr

	                
	                               
	mrs	x1, TTBR1_EL1
	bl	EXT(kasan_bootstrap)

	mov	x0, x20
	mov	lr, x21
#endif

	                       
	ret

Ltrampoline:
	                            
	adrp	x0, arm_init_tramp@page
	add		x0, x0, arm_init_tramp@pageoff
	add		x0, x0, x22
	sub		x0, x0, x23

	                           
	br		x0

  
                         
                               
   
	.text
	.align 2
arm_init_tramp:
	                                                                      
                                                               
   
                        
               
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
               
                        
    


	mov		x19, lr
#if defined(HAS_VMSA_LOCK)
	bl		EXT(vmsa_lock)
#endif
	                                                      
	mov		x0, x21
	bl		EXT(phystokv)

	mov		lr, x19

	                          
	ret

                          

                    
