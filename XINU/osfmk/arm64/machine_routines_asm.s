/*
 * Copyright (c) 2007-2015 Apple Inc. All rights reserved.
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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <machine/asm.h>
#include <arm64/exception_asm.h>
#include <arm64/machine_machdep.h>
#include <arm64/proc_reg.h>
#include <arm/pmap.h>
#include <pexpert/arm64/board_config.h>
#include <sys/errno.h>
#include "assym.s"


#if defined(HAS_APPLE_PAC)


.macro LOAD_CPU_JOP_KEY	dst, tmp
	mrs		\tmp, TPIDR_EL1
	ldr		\tmp, [\tmp, ACT_CPUDATAP]
	ldr		\dst, [\tmp, CPU_JOP_KEY]
.endmacro

  
                                                         
   
	.align 2
	.globl EXT(ml_enable_user_jop_key)
LEXT(ml_enable_user_jop_key)
#if HAS_PARAVIRTUALIZED_PAC
	mov 	x2, x0
	MOV64	x0, VMAPPLE_PAC_SET_EL0_DIVERSIFIER_AT_EL1
	mov		x1, #1
	hvc		#0
	LOAD_CPU_JOP_KEY x0, x1
	ret
#endif                              

  
                                                                                
   
	.align 2
	.globl EXT(ml_disable_user_jop_key)
LEXT(ml_disable_user_jop_key)
#if HAS_PARAVIRTUALIZED_PAC
	mov 	x2, x1
	MOV64	x0, VMAPPLE_PAC_SET_EL0_DIVERSIFIER_AT_EL1
	mov		x1, #0
	hvc		#0
	ret
#endif                              

#endif                             

#if HAS_BP_RET

  
                        
                                                             
                   
   

	.align 2
	.globl EXT(set_bp_ret)
LEXT(set_bp_ret)
	                      
	adrp		x14, EXT(bp_ret)@page
	add		x14, x14, EXT(bp_ret)@pageoff
	ldr		w14, [x14]

	mrs		x13, CPU_CFG
	and		x13, x13, (~(ARM64_REG_ACC_CFG_bpSlp_mask << ARM64_REG_ACC_CFG_bpSlp_shift))
	and		x14, x14, #(ARM64_REG_ACC_CFG_bpSlp_mask)
	orr		x13, x13, x14, lsl #(ARM64_REG_ACC_CFG_bpSlp_shift)
	msr		CPU_CFG, x13

	ret
#endif              

#if HAS_NEX_PG
	.align 2
	.globl EXT(set_nex_pg)
LEXT(set_nex_pg)
	mrs		x14, MPIDR_EL1
	                                                                           
	and		x14, x14, #(MPIDR_PNE)
	cbz		x14, Lnex_pg_done

	                                                              
	HID_INSERT_BITS	HID13, ARM64_REG_HID13_RstCyc_mask, ARM64_REG_HID13_RstCyc_val, x13
	HID_SET_BITS HID14, ARM64_REG_HID14_NexPwgEn, x13

Lnex_pg_done:
	ret

#endif              

                            
                          
   
	.align	2
	.globl	EXT(get_fpscr)
LEXT(get_fpscr)
#if	__ARM_VFP__
	mrs	x1, FPSR			            
	mov	x4, #(FPSR_MASK & 0xFFFF)
	mov	x5, #(FPSR_MASK & 0xFFFF0000)
	orr	x0, x4, x5
	and	x1, x1, x0			                                           
						           
	mrs	x2, FPCR			            
	mov	x4, #(FPCR_MASK & 0xFFFF)
	mov	x5, #(FPCR_MASK & 0xFFFF0000)
	orr	x0, x4, x5
	and	x2, x2, x0			                                           
						           
	orr	x0, x1, x2			                                        
#else
	mov	x0, #0
#endif
	ret
	.align	2
	.globl	EXT(set_fpscr)
                                  
                                                                
                                                                 
                                                     
   
LEXT(set_fpscr)
#if	__ARM_VFP__
	mov	x4, #(FPSR_MASK & 0xFFFF)
	mov	x5, #(FPSR_MASK & 0xFFFF0000)
	orr	x1, x4, x5
	and	x1, x1, x0			                                          
	mov	x4, #(FPCR_MASK & 0xFFFF)
	mov	x5, #(FPCR_MASK & 0xFFFF0000)
	orr	x2, x4, x5
	and	x2, x2, x0			                                          
	msr	FPSR, x1			             
	msr	FPCR, x2			             
	dsb	ish				                                
#endif
	ret

  
                                                            
                                                     
  
                                                                                   
                                                                         
                                                                                  
                                                                               
                                                                        
                                                                                
                                                                             
                                                                    
  
                                           
   
	.align 2
	.globl EXT(update_mdscr)
LEXT(update_mdscr)
	mov	x4, #0
	mrs	x2, MDSCR_EL1
	bic	x2, x2, x0
	orr	x2, x2, x1
1:
	bic	x2, x2, #0x2000
	msr	MDSCR_EL1, x2
#if defined(CONFIG_KERNEL_INTEGRITY)
	  
                                                 
                                   
    
	ands	x3, x2, #0x2000
	orr	x4, x4, x3
	bne	1b
	cmp	x4, xzr
	b.ne	Lupdate_mdscr_panic
#endif
	ret

Lupdate_mdscr_panic:
	adrp	x0, Lupdate_mdscr_panic_str@page
	add	x0, x0, Lupdate_mdscr_panic_str@pageoff
	b	EXT(panic)
	b	.

Lupdate_mdscr_panic_str:
	.asciz "MDSCR.KDE was set"


  
                                            
   
	.text
	.align 2
	.globl EXT(set_mmu_ttb_alternate)
LEXT(set_mmu_ttb_alternate)
	dsb		sy
#if defined(KERNEL_INTEGRITY_KTRR)
	mov		x1, lr
	bl		EXT(pinst_set_ttbr1)
	mov		lr, x1
#else
#if defined(HAS_VMSA_LOCK)
#if DEBUG || DEVELOPMENT
	mrs		x1, VMSA_LOCK_EL1
	and		x1, x1, #(VMSA_LOCK_TTBR1_EL1)
	cbnz		x1, L_set_locked_reg_panic
#endif                           
#endif                             
	msr		TTBR1_EL1, x0
#endif                                     
	isb		sy
	ret

#if XNU_MONITOR
	.section __PPLTEXT,__text,regular,pure_instructions
#else
	.text
#endif
	.align 2
	.globl EXT(set_mmu_ttb)
LEXT(set_mmu_ttb)
#if __ARM_KERNEL_PROTECT__
	                                 
	orr		x0, x0, #(1 << TTBR_ASID_SHIFT)
#endif                             
	dsb		ish
	msr		TTBR0_EL1, x0
	isb		sy
	ret


#if XNU_MONITOR
	.text
	.align 2
	.globl EXT(ml_get_ppl_cpu_data)
LEXT(ml_get_ppl_cpu_data)
	LOAD_PMAP_CPU_DATA x0, x1, x2
	ret
#endif

  
                            
   
	.text
	.align 2
	.globl EXT(set_aux_control)
LEXT(set_aux_control)
	msr		ACTLR_EL1, x0
	                     
	isb		sy
	ret

#if __ARM_KERNEL_PROTECT__
	.text
	.align 2
	.globl EXT(set_vbar_el1)
LEXT(set_vbar_el1)
#if defined(KERNEL_INTEGRITY_KTRR)
	b		EXT(pinst_set_vbar)
#else
	msr		VBAR_EL1, x0
	ret
#endif
#endif                             

#if defined(HAS_VMSA_LOCK)
	.text
	.align 2
	.globl EXT(vmsa_lock)
LEXT(vmsa_lock)
	isb sy
	mov x1, #(VMSA_LOCK_SCTLR_M_BIT)
#if __ARM_MIXED_PAGE_SIZE__
	mov x0, #(VMSA_LOCK_TTBR1_EL1 | VMSA_LOCK_VBAR_EL1)
#else
	mov x0, #(VMSA_LOCK_TTBR1_EL1 | VMSA_LOCK_TCR_EL1 | VMSA_LOCK_VBAR_EL1)
#endif
	orr x0, x0, x1
	msr VMSA_LOCK_EL1, x0
	isb sy
	ret
#endif                             

  
                                   
   
	.text
	.align 2
	.globl EXT(set_tcr)
LEXT(set_tcr)
#if defined(APPLE_ARM64_ARCH_FAMILY)
#if DEBUG || DEVELOPMENT
	                                         
	eor		x1, x0, x0, lsr #(TCR_T1SZ_SHIFT - TCR_T0SZ_SHIFT)
	and		x1, x1, #(TCR_TSZ_MASK << TCR_T0SZ_SHIFT)
	cbnz	x1, L_set_tcr_panic
#endif                           
#endif                                       
#if defined(KERNEL_INTEGRITY_KTRR)
	mov		x1, lr
	bl		EXT(pinst_set_tcr)
	mov		lr, x1
#else
#if defined(HAS_VMSA_LOCK)
#if DEBUG || DEVELOPMENT
	                      
	mrs 		x1, VMSA_LOCK_EL1
	and		x1, x1, #(VMSA_LOCK_TCR_EL1)
	cbnz		x1, L_set_locked_reg_panic
#endif                           
#endif                             
	msr		TCR_EL1, x0
#endif                                   
	isb		sy
	ret

#if DEBUG || DEVELOPMENT
L_set_tcr_panic:
	PUSH_FRAME
	sub		sp, sp, #16
	str		x0, [sp]
	adr		x0, L_set_tcr_panic_str
	BRANCH_EXTERN panic

L_set_locked_reg_panic:
	PUSH_FRAME
	sub		sp, sp, #16
	str		x0, [sp]
	adr		x0, L_set_locked_reg_panic_str
	BRANCH_EXTERN panic
	b .

L_set_tcr_panic_str:
	.asciz	"set_tcr: t0sz, t1sz not equal (%llx)\n"


L_set_locked_reg_panic_str:
	.asciz	"attempt to set locked register: (%llx)\n"
#endif                           

  
                                                     
   
	.text
	.align 2
	.globl EXT(mmu_kvtop)
LEXT(mmu_kvtop)
	mrs		x2, DAIF									                    
	msr		DAIFSet, #(DAIFSC_IRQF | DAIFSC_FIQF)		              
	at		s1e1r, x0									                          
	isb		sy
	mrs		x1, PAR_EL1									              
	msr		DAIF, x2									                          
	tbnz	x1, #0, L_mmu_kvtop_invalid					                             
	bfm		x1, x0, #0, #11								                  
	and		x0, x1, #0x0000ffffffffffff					                          
	ret
L_mmu_kvtop_invalid:
	mov		x0, #0										                 
	ret

  
                                                   
   
	.text
	.align 2
	.globl EXT(mmu_uvtop)
LEXT(mmu_uvtop)
	lsr		x8, x0, #56									                   
	cbnz	x8, L_mmu_uvtop_invalid						                              
	mrs		x2, DAIF									                    
	msr		DAIFSet, #(DAIFSC_IRQF | DAIFSC_FIQF)		              
	at		s1e0r, x0									                          
	isb		sy
	mrs		x1, PAR_EL1									              
	msr		DAIF, x2									                          
	tbnz	x1, #0, L_mmu_uvtop_invalid					                             
	bfm		x1, x0, #0, #11								                  
	and		x0, x1, #0x0000ffffffffffff					                          
	ret
L_mmu_uvtop_invalid:
	mov		x0, #0										                 
	ret

  
                                                                
   
	.text
	.align 2
	.globl EXT(mmu_kvtop_wpreflight)
LEXT(mmu_kvtop_wpreflight)
	mrs		x2, DAIF									                    
	msr		DAIFSet, #(DAIFSC_IRQF | DAIFSC_FIQF)		              
	at		s1e1w, x0									                          
	mrs		x1, PAR_EL1									              
	msr		DAIF, x2									                          
	tbnz	x1, #0, L_mmu_kvtop_wpreflight_invalid		                             
	bfm		x1, x0, #0, #11								                  
	and		x0, x1, #0x0000ffffffffffff					                         
	ret
L_mmu_kvtop_wpreflight_invalid:
	mov		x0, #0										                 
	ret

  
                       
  
                                                                           
  
                         
                                   
                                           
                                                           
   
.macro SET_RECOVERY_HANDLER	label, tpidr=x16, old_handler=x10, label_in_adr_range=0
	                                                             
	                                                                         
.if \label_in_adr_range==1						                                    
	adr		x17, \label
.else
	adrp	x17, \label@page
	add		x17, x17, \label@pageoff
.endif
#if defined(HAS_APPLE_PAC)
	mrs		x16, TPIDR_EL1
	add		x16, x16, TH_RECOVER
	movk	x16, #PAC_DISCRIMINATOR_RECOVER, lsl 48
	pacia	x17, x16							                                          
#endif

	mrs		\tpidr, TPIDR_EL1					                      
	ldr		\old_handler, [\tpidr, TH_RECOVER]	                                 
	str		x17, [\tpidr, TH_RECOVER]			                                  
.endmacro

  
                         
  
                                                        
  
                                                       
                                                                   
   
.macro CLEAR_RECOVERY_HANDLER	tpidr=x16, old_handler=x10
	str		\old_handler, [\tpidr, TH_RECOVER]	                                        
.endmacro


	.text
	.align 2
copyio_error:
	CLEAR_RECOVERY_HANDLER
	mov		x0, #EFAULT					                         
	POP_FRAME
	ARM64_STACK_EPILOG

  
                                                          
   
	.text
	.align 2
	.globl EXT(_bcopyin)
LEXT(_bcopyin)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	SET_RECOVERY_HANDLER copyio_error
	                                                           
	cmp		x2, #16
	b.lt	2f
	sub		x2, x2, #16
1:
	                        
	ldp		x3, x4, [x0], #16
	stp		x3, x4, [x1], #16
	subs	x2, x2, #16
	b.ge	1b
	                                           
	adds	x2, x2, #16
	b.eq	3f
2:	              
	subs	x2, x2, #1
	ldrb	w3, [x0], #1
	strb	w3, [x1], #1
	b.hi	2b
3:
	CLEAR_RECOVERY_HANDLER
	mov		x0, #0
	POP_FRAME
	ARM64_STACK_EPILOG

  
                                                       
   
	.text
	.align 2
	.globl EXT(_copyin_atomic32)
LEXT(_copyin_atomic32)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	SET_RECOVERY_HANDLER copyio_error
	ldr		w8, [x0]
	str		w8, [x1]
	mov		x0, #0
	CLEAR_RECOVERY_HANDLER
	POP_FRAME
	ARM64_STACK_EPILOG

  
                                                                       
   
	.text
	.align 2
	.globl EXT(_copyin_atomic32_wait_if_equals)
LEXT(_copyin_atomic32_wait_if_equals)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	SET_RECOVERY_HANDLER copyio_error
	ldxr		w8, [x0]
	cmp		w8, w1
	mov		x0, ESTALE
	b.ne		1f
	mov		x0, #0
	wfe
1:
	clrex
	CLEAR_RECOVERY_HANDLER
	POP_FRAME
	ARM64_STACK_EPILOG

  
                                                       
   
	.text
	.align 2
	.globl EXT(_copyin_atomic64)
LEXT(_copyin_atomic64)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	SET_RECOVERY_HANDLER copyio_error
	ldr		x8, [x0]
	str		x8, [x1]
	mov		x0, #0
	CLEAR_RECOVERY_HANDLER
	POP_FRAME
	ARM64_STACK_EPILOG


  
                                                   
   
	.text
	.align 2
	.globl EXT(_copyout_atomic32)
LEXT(_copyout_atomic32)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	SET_RECOVERY_HANDLER copyio_error
	str		w0, [x1]
	mov		x0, #0
	CLEAR_RECOVERY_HANDLER
	POP_FRAME
	ARM64_STACK_EPILOG

  
                                                   
   
	.text
	.align 2
	.globl EXT(_copyout_atomic64)
LEXT(_copyout_atomic64)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	SET_RECOVERY_HANDLER copyio_error
	str		x0, [x1]
	mov		x0, #0
	CLEAR_RECOVERY_HANDLER
	POP_FRAME
	ARM64_STACK_EPILOG


  
                                                           
   
	.text
	.align 2
	.globl EXT(_bcopyout)
LEXT(_bcopyout)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	SET_RECOVERY_HANDLER copyio_error
	                                                           
	cmp		x2, #16
	b.lt	2f
	sub		x2, x2, #16
1:
	                        
	ldp		x3, x4, [x0], #16
	stp		x3, x4, [x1], #16
	subs	x2, x2, #16
	b.ge	1b
	                                           
	adds	x2, x2, #16
	b.eq	3f
2:                
	subs	x2, x2, #1
	ldrb	w3, [x0], #1
	strb	w3, [x1], #1
	b.hi	2b
3:
	CLEAR_RECOVERY_HANDLER
	mov		x0, #0
	POP_FRAME
	ARM64_STACK_EPILOG

  
                   
                                 
                       
                   
                       
   
	.text
	.align 2
	.globl EXT(_bcopyinstr)
LEXT(_bcopyinstr)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	SET_RECOVERY_HANDLER Lcopyinstr_error, label_in_adr_range=1
	mov		x4, #0						                          
Lcopyinstr_loop:
	ldrb	w5, [x0], #1					                                   
	strb	w5, [x1], #1				                                  
	add		x4, x4, #1					                         
	cbz	x5, Lcopyinstr_done	  		                                   
	cmp		x4, x2						                                         
	b.ne	Lcopyinstr_loop
Lcopyinstr_too_long:
	mov		x5, #ENAMETOOLONG			                                                  
Lcopyinstr_done:
	str		x4, [x3]					                                
	mov		x0, x5						                                                         
	b		Lcopyinstr_exit
Lcopyinstr_error:
	mov		x0, #EFAULT					                         
Lcopyinstr_exit:
	CLEAR_RECOVERY_HANDLER
	POP_FRAME
	ARM64_STACK_EPILOG

  
                                                                                  
  
                                                                   
                                                                     
   
                                 
                                              
                                                    
            
                                  
            
                                                            
                            
                                                     
  
   
	.text
	.align 2
	.globl EXT(copyinframe)
LEXT(copyinframe)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	SET_RECOVERY_HANDLER copyio_error
	cbnz	w2, Lcopyinframe64 		                   
	adrp	x5, EXT(gVirtBase)@page                                                                    
	add		x5, x5, EXT(gVirtBase)@pageoff
	ldr		x5, [x5]
	cmp     x5, x0					                                            
	b.hi	Lcopyinframe32			                                          
	mov		w0, #EFAULT				                                                           
	b		Lcopyinframe_done		

Lcopyinframe32:
	ldr		x12, [x0]				               
	str		x12, [x1]
	mov 	w0, #0					          
	b		Lcopyinframe_done

Lcopyinframe64:
	mov		x3, VM_MIN_KERNEL_ADDRESS		                          
	orr		x9, x0, TBI_MASK				                                  
	cmp		x9, x3							                                            
	b.hs	Lcopyinframe_valid
	tst		x0, TBI_MASK					                         
	b.eq	Lcopyinframe_valid
	mov		w0, #EFAULT						                       
	b		Lcopyinframe_done
Lcopyinframe_valid:
	ldp		x12, x13, [x0]			                
	stp		x12, x13, [x1]
	mov 	w0, #0					          

Lcopyinframe_done:
	CLEAR_RECOVERY_HANDLER
	POP_FRAME
	ARM64_STACK_EPILOG


  
                                     
   
	.text
	.align 2
	.globl EXT(arm_debug_read_dscr)
LEXT(arm_debug_read_dscr)
	PANIC_UNIMPLEMENTED

  
                                                          
  
                                                            
                                                      
                                                              
                 
   
       .text
       .align 2
       .globl EXT(arm_debug_set_cp14)
LEXT(arm_debug_set_cp14)
	PANIC_UNIMPLEMENTED

#if defined(APPLE_ARM64_ARCH_FAMILY)
  
                                                
   
	.text
	.align 2
	.globl EXT(arm64_prepare_for_sleep)
LEXT(arm64_prepare_for_sleep)
	PUSH_FRAME

#if defined(APPLETYPHOON)
	                            
	HID_SET_BITS HID2, ARM64_REG_HID2_disMMUmtlbPrefetch, x9
	dsb		sy
	isb		sy
#endif

#if HAS_CLUSTER
	cbnz		x0, 1f                                                                   
	                                             
	mrs		x9, CPU_OVRD
	and		x9, x9, #(~(ARM64_REG_CYC_OVRD_irq_mask | ARM64_REG_CYC_OVRD_fiq_mask))
	mov		x10, #(ARM64_REG_CYC_OVRD_irq_disable | ARM64_REG_CYC_OVRD_fiq_disable)
	orr		x9, x9, x10
	msr		CPU_OVRD, x9
	isb
1:
#endif

	cbz		x0, 1f                                                                        
#if __ARM_GLOBAL_SLEEP_BIT__
	                    
	mrs		x1, ACC_OVRD
	orr		x1, x1, #(ARM64_REG_ACC_OVRD_enDeepSleep)
	and		x1, x1, #(~(ARM64_REG_ACC_OVRD_disL2Flush4AccSlp_mask))
	orr		x1, x1, #(  ARM64_REG_ACC_OVRD_disL2Flush4AccSlp_deepsleep)
	and		x1, x1, #(~(ARM64_REG_ACC_OVRD_ok2PwrDnSRM_mask))
	orr		x1, x1, #(  ARM64_REG_ACC_OVRD_ok2PwrDnSRM_deepsleep)
	and		x1, x1, #(~(ARM64_REG_ACC_OVRD_ok2TrDnLnk_mask))
	orr		x1, x1, #(  ARM64_REG_ACC_OVRD_ok2TrDnLnk_deepsleep)
	and		x1, x1, #(~(ARM64_REG_ACC_OVRD_ok2PwrDnCPM_mask))
	orr		x1, x1, #(  ARM64_REG_ACC_OVRD_ok2PwrDnCPM_deepsleep)
#if HAS_RETENTION_STATE
	orr		x1, x1, #(ARM64_REG_ACC_OVRD_disPioOnWfiCpu)
#endif
	msr		ACC_OVRD, x1

#if defined(APPLEMONSOON)
	                                                                                       
	mrs		x1, EBLK_OVRD
	orr		x1, x1, #(ARM64_REG_ACC_OVRD_enDeepSleep)
	and		x1, x1, #(~(ARM64_REG_ACC_OVRD_disL2Flush4AccSlp_mask))
	orr		x1, x1, #(  ARM64_REG_ACC_OVRD_disL2Flush4AccSlp_deepsleep)
	and		x1, x1, #(~(ARM64_REG_ACC_OVRD_ok2PwrDnSRM_mask))
	orr		x1, x1, #(  ARM64_REG_ACC_OVRD_ok2PwrDnSRM_deepsleep)
	and		x1, x1, #(~(ARM64_REG_ACC_OVRD_ok2TrDnLnk_mask))
	orr		x1, x1, #(  ARM64_REG_ACC_OVRD_ok2TrDnLnk_deepsleep)
	and		x1, x1, #(~(ARM64_REG_ACC_OVRD_ok2PwrDnCPM_mask))
	orr		x1, x1, #(  ARM64_REG_ACC_OVRD_ok2PwrDnCPM_deepsleep)
	msr		EBLK_OVRD, x1

#endif

#else
#if defined(APPLETYPHOON) || defined(APPLETWISTER)
	                    
	mov		x1, ARM64_REG_CYC_CFG_deepSleep
	msr		CPU_CFG, x1
#endif
#endif

1:
	                                                     
	mrs		x9, CPU_OVRD
	orr		x9, x9, #(ARM64_REG_CYC_OVRD_ok2pwrdn_force_down)
#if HAS_RETENTION_STATE
	orr		x9, x9, #(ARM64_REG_CYC_OVRD_disWfiRetn)
#endif
	msr		CPU_OVRD, x9

#if defined(APPLEMONSOON) || defined(APPLEVORTEX)
	ARM64_IS_PCORE x9
	cbz		x9, Lwfi_inst                      

	                                                                
                                                                
                                                         
                                                             
                                                             
                                                                 
                                                              
                                                            
                                                                 
                                                    
#if defined(APPLEVORTEX)
	                                                               
                                                               
                                                
	mrs x9, MIDR_EL1
	EXEC_COREALL_REVLO CPU_VERSION_B0, x9, x10
#endif
	mrs		x9, HID10
	orr		x9, x9, #(ARM64_REG_HID10_DisHwpGups)
	msr		HID10, x9
	isb		sy
	and		x9, x9, #(~(ARM64_REG_HID10_DisHwpGups))
	msr		HID10, x9
	isb		sy
#endif
	EXEC_END

Lwfi_inst:
	dsb		sy
	isb		sy
	wfi
	b		Lwfi_inst

  
                                     
  
   	
	.text
	.align 2
	.globl EXT(arm64_force_wfi_clock_gate)
LEXT(arm64_force_wfi_clock_gate)
	ARM64_STACK_PROLOG
	PUSH_FRAME

	mrs		x0, CPU_OVRD
	orr		x0, x0, #(ARM64_REG_CYC_OVRD_ok2pwrdn_force_up)
	msr		CPU_OVRD, x0
	
	POP_FRAME
	ARM64_STACK_EPILOG


#if HAS_RETENTION_STATE
	.text
	.align 2
	.globl EXT(arm64_retention_wfi)
LEXT(arm64_retention_wfi)
	wfi
	cbz		lr, Lwfi_retention	                                                                            
	ret					                                      
Lwfi_retention:
	mov		x0, #1
	bl		EXT(ClearIdlePop)
	mov		x0, #0 
	bl		EXT(cpu_idle_exit)	                                    
	b		.			                                      
#endif

#if defined(APPLETYPHOON)

	.text
	.align 2
	.globl EXT(typhoon_prepare_for_wfi)

LEXT(typhoon_prepare_for_wfi)
	PUSH_FRAME

	                            
	HID_SET_BITS HID2, ARM64_REG_HID2_disMMUmtlbPrefetch, x0
	dsb		sy
	isb		sy

	POP_FRAME
	ret


	.text
	.align 2
	.globl EXT(typhoon_return_from_wfi)
LEXT(typhoon_return_from_wfi)
	PUSH_FRAME

	                            
	HID_CLEAR_BITS HID2, ARM64_REG_HID2_disMMUmtlbPrefetch, x0
	dsb		sy
	isb		sy 

	POP_FRAME
	ret
#endif

#ifdef  APPLETYPHOON

#define HID0_DEFEATURES_1 0x0000a0c000064010ULL
#define HID1_DEFEATURES_1 0x000000004005bf20ULL
#define HID2_DEFEATURES_1 0x0000000000102074ULL
#define HID3_DEFEATURES_1 0x0000000000400003ULL
#define HID4_DEFEATURES_1 0x83ff00e100000268ULL
#define HID7_DEFEATURES_1 0x000000000000000eULL

#define HID0_DEFEATURES_2 0x0000a1c000020010ULL
#define HID1_DEFEATURES_2 0x000000000005d720ULL
#define HID2_DEFEATURES_2 0x0000000000002074ULL
#define HID3_DEFEATURES_2 0x0000000000400001ULL
#define HID4_DEFEATURES_2 0x8390000200000208ULL
#define HID7_DEFEATURES_2 0x0000000000000000ULL

  
                       
                       
  
.macro LOAD_UINT64 
	movz	$0, #(($1 >> 48) & 0xffff), lsl #48
	movk	$0, #(($1 >> 32) & 0xffff), lsl #32
	movk	$0, #(($1 >> 16) & 0xffff), lsl #16
	movk	$0, #(($1)       & 0xffff)
.endmacro

	.text
	.align 2
	.globl EXT(cpu_defeatures_set)
LEXT(cpu_defeatures_set)
	PUSH_FRAME
	cmp		x0, #2
	b.eq		cpu_defeatures_set_2
	cmp		x0, #1
	b.ne		cpu_defeatures_set_ret
	LOAD_UINT64	x1, HID0_DEFEATURES_1
	mrs		x0, HID0
	orr		x0, x0, x1
	msr		HID0, x0
	LOAD_UINT64	x1, HID1_DEFEATURES_1
	mrs		x0, HID1
	orr		x0, x0, x1
	msr		HID1, x0
	LOAD_UINT64	x1, HID2_DEFEATURES_1
	mrs		x0, HID2
	orr		x0, x0, x1
	msr		HID2, x0
	LOAD_UINT64	x1, HID3_DEFEATURES_1
	mrs		x0, HID3
	orr		x0, x0, x1
	msr		HID3, x0
	LOAD_UINT64	x1, HID4_DEFEATURES_1
	mrs		x0, HID4
	orr		x0, x0, x1
	msr		HID4, x0
	LOAD_UINT64	x1, HID7_DEFEATURES_1
	mrs		x0, HID7
	orr		x0, x0, x1
	msr		HID7, x0
	dsb		sy
	isb		sy 
	b		cpu_defeatures_set_ret
cpu_defeatures_set_2:
	LOAD_UINT64	x1, HID0_DEFEATURES_2
	mrs		x0, HID0
	orr		x0, x0, x1
	msr		HID0, x0
	LOAD_UINT64	x1, HID1_DEFEATURES_2
	mrs		x0, HID1
	orr		x0, x0, x1
	msr		HID1, x0
	LOAD_UINT64	x1, HID2_DEFEATURES_2
	mrs		x0, HID2
	orr		x0, x0, x1
	msr		HID2, x0
	LOAD_UINT64	x1, HID3_DEFEATURES_2
	mrs		x0, HID3
	orr		x0, x0, x1
	msr		HID3, x0
	LOAD_UINT64	x1, HID4_DEFEATURES_2
	mrs		x0, HID4
	orr		x0, x0, x1
	msr		HID4, x0
	LOAD_UINT64	x1, HID7_DEFEATURES_2
	mrs		x0, HID7
	orr		x0, x0, x1
	msr		HID7, x0
	dsb		sy
	isb		sy 
	b		cpu_defeatures_set_ret
cpu_defeatures_set_ret:
	POP_FRAME
	ret
#endif

#else                                        
	.text
	.align 2
	.globl EXT(arm64_prepare_for_sleep)
LEXT(arm64_prepare_for_sleep)
	PUSH_FRAME
Lwfi_inst:
	dsb		sy
	isb		sy
	wfi
	b		Lwfi_inst

  
                                     
                                          
   	
	.text
	.align 2
	.globl EXT(arm64_force_wfi_clock_gate)
LEXT(arm64_force_wfi_clock_gate)
	PUSH_FRAME
	nop
	POP_FRAME

#endif                                       

  
                                                     
  
                                                                            
                                                                             
                                                                          
                                                                                 
                                                                  
   
	.text
	.align 2
	.globl EXT(arm64_replace_bootstack)
LEXT(arm64_replace_bootstack)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	                                  
	ldr		x0, [x0, CPU_EXCEPSTACK_TOP]
	mrs		x4, DAIF					                                                     
	msr		DAIFSet, #(DAIFSC_IRQF | DAIFSC_FIQF | DAIFSC_ASYNCF)		                         
	                                
#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
	mov		x1, lr
	bl		EXT(pinst_spsel_1)
	mov		lr, x1
#else
	msr		SPSel, #1
#endif
	mov		sp, x0
	msr		SPSel, #0
	msr		DAIF, x4					                          
	POP_FRAME
	ARM64_STACK_EPILOG

#ifdef MONITOR
  
                                                                
                                         
  
                                                     
                                                                                              
                                                                                              
                    
   
	.text
	.align 2
	.globl EXT(monitor_call)
LEXT(monitor_call)
	smc 	0x11
	ret
#endif

#ifdef HAS_APPLE_PAC
  
                    
  
                                 
                                                         
   
.macro SIGN_THREAD_STATE
	pacga	x1, x1, x0		                                       
	  
                                                                         
                                           
    
	bic		x2, x2, PSR_CF
	pacga	x1, x2, x1		                                
	pacga	x1, x3, x1		                                
	pacga	x1, x4, x1		                               
	pacga	x1, x5, x1		                                
	str		x1, [x0, $0]
#if DEBUG || DEVELOPMENT
	mrs		x1, DAIF
	tbz		x1, #DAIF_IRQF_SHIFT, Lintr_enabled_panic
#endif                           
.endmacro

  
                     
  
                                         
                                                                    
                                                    
   
.macro CHECK_SIGNED_STATE
	pacga	x1, x1, x0		                                       
	  
                                                                         
                                           
    
	bic		x2, x2, PSR_CF
	pacga	x1, x2, x1		                                
	pacga	x1, x3, x1		                                
	pacga	x1, x4, x1		                               
	pacga	x1, x5, x1		                                
	ldr		x2, [x0, $0]
	cmp		x1, x2
	b.ne	$1
#if DEBUG || DEVELOPMENT
	mrs		x1, DAIF
	tbz		x1, #DAIF_IRQF_SHIFT, Lintr_enabled_panic
#endif                           
.endmacro

   
                                                                
                                                   
                       
   
	.text
	.align 2
	.globl EXT(ml_sign_thread_state)
LEXT(ml_sign_thread_state)
	SIGN_THREAD_STATE SS64_JOPHASH
	ret

   
                                                                            
                                                   
                       
   
	.text
	.align 2
	.globl EXT(ml_sign_kernel_thread_state)
LEXT(ml_sign_kernel_thread_state)
	SIGN_THREAD_STATE SS64_KERNEL_JOPHASH
	ret

   
                                                                 
                                                    
                        
   
	.text
	.align 2
	.globl EXT(ml_check_signed_state)
LEXT(ml_check_signed_state)
	CHECK_SIGNED_STATE SS64_JOPHASH, Lcheck_hash_panic
	ret
Lcheck_hash_panic:
	  
                                                                         
                                                                     
                                                                       
                                                                        
                                                 
    
	ARM64_STACK_PROLOG
	PUSH_FRAME
	mov		x1, x0
	adr		x0, Lcheck_hash_str
	CALL_EXTERN panic_with_thread_kernel_state

   
                                                                             
                                                    
                        
   
	.text
	.align 2
	.globl EXT(ml_check_kernel_signed_state)
LEXT(ml_check_kernel_signed_state)
	CHECK_SIGNED_STATE SS64_KERNEL_JOPHASH, Lcheck_kernel_hash_panic
	ret
Lcheck_kernel_hash_panic:
	ARM64_STACK_PROLOG
	PUSH_FRAME
	adr		x0, Lcheck_hash_str
	CALL_EXTERN panic

Lcheck_hash_str:
	.asciz "JOP Hash Mismatch Detected (PC, CPSR, or LR corruption)"

#if DEBUG || DEVELOPMENT
Lintr_enabled_panic:
	ARM64_STACK_PROLOG
	PUSH_FRAME
	adr		x0, Lintr_enabled_str
	CALL_EXTERN panic
Lintr_enabled_str:
	  
                                                                         
                                                                     
    
	.asciz "Signed thread state manipulated with interrupts enabled"
#endif                           

   
                                                                
  
                                             
   
	.text
	.align 2
	.globl EXT(ml_auth_thread_state_invalid_cpsr)
LEXT(ml_auth_thread_state_invalid_cpsr)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	mov		x1, x0
	adr		x0, Linvalid_cpsr_str
	CALL_EXTERN panic_with_thread_kernel_state

Linvalid_cpsr_str:
	.asciz "Thread state corruption detected (PE mode == 0)"
#endif                    

	.text
	.align 2
	.globl EXT(fill32_dczva)
LEXT(fill32_dczva)
0:
	dc	zva, x0
	add	x0, x0, #64
	subs	x1, x1, #64
	b.hi	0b
	ret

	.text
	.align 2
	.globl EXT(fill32_nt)
LEXT(fill32_nt)
	dup.4s	v0, w2
0:
	stnp	q0, q0, [x0]
	stnp	q0, q0, [x0, #0x20]
	stnp	q0, q0, [x0, #0x40]
	stnp	q0, q0, [x0, #0x60]
	add	x0, x0, #128
	subs	x1, x1, #128
	b.hi	0b
	ret

                         
