/*
 * Copyright (c) 2018 Apple Inc. All rights reserved.
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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <arm64/pac_asm.h>
#include <pexpert/arm64/board_config.h>
#include "assym.s"

#if XNU_MONITOR
                                                                   
#define PPL_EXIT_DISPATCH   0                                                
#define PPL_EXIT_PANIC_CALL 1                                
#define PPL_EXIT_BAD_CALL   2                              
#define PPL_EXIT_EXCEPTION  3                                 


#define KERNEL_MODE_ELR      ELR_GL11
#define KERNEL_MODE_FAR      FAR_GL11
#define KERNEL_MODE_ESR      ESR_GL11
#define KERNEL_MODE_SPSR     SPSR_GL11
#define KERNEL_MODE_ASPSR    ASPSR_GL11
#define KERNEL_MODE_VBAR     VBAR_GL11
#define KERNEL_MODE_TPIDR    TPIDR_GL11

#define GUARDED_MODE_ELR     ELR_EL1
#define GUARDED_MODE_FAR     FAR_EL1
#define GUARDED_MODE_ESR     ESR_EL1
#define GUARDED_MODE_SPSR    SPSR_EL1
#define GUARDED_MODE_ASPSR   ASPSR_EL1
#define GUARDED_MODE_VBAR    VBAR_EL1
#define GUARDED_MODE_TPIDR   TPIDR_EL1

  
                     
  
                                                              
                                                                    
                            
                            
  
   
.macro LOAD_PMAP_CPU_DATA
	                     
	mrs		$0, MPIDR_EL1
	ubfx	$1, $0, MPIDR_AFF1_SHIFT, MPIDR_AFF1_WIDTH
	adrp	$2, EXT(cluster_offsets)@page
	add		$2, $2, EXT(cluster_offsets)@pageoff
	ldr		$1, [$2, $1, lsl #3]

	and		$0, $0, MPIDR_AFF0_MASK
	add		$0, $0, $1

	                                 
	adrp	$1, EXT(pmap_cpu_data_array)@page
	add		$1, $1, EXT(pmap_cpu_data_array)@pageoff

	  
                                                                         
                                                            
                                                
    
	cmp		$0, MAX_CPUS
	b.hs	.

	mov		$2, PMAP_CPU_DATA_ARRAY_ENTRY_SIZE
	                               
	madd	$0, $0, $2, $1
.endmacro

  
                    
  
                                                      
                                                                    
                            
                            
  
   
.macro GET_PMAP_CPU_DATA
	LOAD_PMAP_CPU_DATA $0, $1, $2
.endmacro

#endif                  

  
                           
  
                                                                     
                              
                             
                             
   
.macro INIT_SAVED_STATE_FLAVORS
	mov		$1, ARM_SAVED_STATE64                                                                      
	mov		$2, ARM_SAVED_STATE64_COUNT
	stp		$1, $2, [$0, SS_FLAVOR]
	mov		$1, ARM_NEON_SAVED_STATE64                                                                
	str		$1, [$0, NS_FLAVOR]
	mov		$1, ARM_NEON_SAVED_STATE64_COUNT
	str		$1, [$0, NS_COUNT]
.endmacro

  
                  
  
                                                                              
             
  
                                                                               
                                                                             
  
                                         
                                  
   
#define KERNEL_MODE 0
#define HIBERNATE_MODE 1

.macro SPILL_REGISTERS	mode
	stp		x2, x3, [x0, SS64_X2]                                                         
	stp		x4, x5, [x0, SS64_X4]
	stp		x6, x7, [x0, SS64_X6]
	stp		x8, x9, [x0, SS64_X8]
	stp		x10, x11, [x0, SS64_X10]
	stp		x12, x13, [x0, SS64_X12]
	stp		x14, x15, [x0, SS64_X14]
	stp		x16, x17, [x0, SS64_X16]
	stp		x18, x19, [x0, SS64_X18]
	stp		x20, x21, [x0, SS64_X20]
	stp		x22, x23, [x0, SS64_X22]
	stp		x24, x25, [x0, SS64_X24]
	stp		x26, x27, [x0, SS64_X26]
	stp		x28, fp,  [x0, SS64_X28]
	str		lr, [x0, SS64_LR]

	                                 

	stp		q0, q1, [x0, NS64_Q0]
	stp		q2, q3, [x0, NS64_Q2]
	stp		q4, q5, [x0, NS64_Q4]
	stp		q6, q7, [x0, NS64_Q6]
	stp		q8, q9, [x0, NS64_Q8]
	stp		q10, q11, [x0, NS64_Q10]
	stp		q12, q13, [x0, NS64_Q12]
	stp		q14, q15, [x0, NS64_Q14]
	stp		q16, q17, [x0, NS64_Q16]
	stp		q18, q19, [x0, NS64_Q18]
	stp		q20, q21, [x0, NS64_Q20]
	stp		q22, q23, [x0, NS64_Q22]
	stp		q24, q25, [x0, NS64_Q24]
	stp		q26, q27, [x0, NS64_Q26]
	stp		q28, q29, [x0, NS64_Q28]
	stp		q30, q31, [x0, NS64_Q30]

	mrs		x22, ELR_EL1                                                                                   
	mrs		x23, SPSR_EL1                                                                                
	mrs		x24, FPSR
	mrs		x25, FPCR

#if defined(HAS_APPLE_PAC)
	.if \mode != HIBERNATE_MODE

	                                            
	mov		x21, x1
	mov		x20, lr

	  
                                 
   
                                 
                              
                                
                              
                               
                               
    
	mov		x1, x22
	mov		w2, w23
	mov		x3, x20
	mov		x4, x16
	mov		x5, x17
	bl		_ml_sign_thread_state
	mov		lr, x20
	mov		x1, x21
	.endif
#endif                             

	str		x22, [x0, SS64_PC]                                                                 
	str		w23, [x0, SS64_CPSR]                                                       
	str		w24, [x0, NS64_FPSR]
	str		w25, [x0, NS64_FPCR]

	mrs		x20, FAR_EL1
	mrs		x21, ESR_EL1

	str		x20, [x0, SS64_FAR]
	str		w21, [x0, SS64_ESR]
.endmacro

.macro DEADLOOP
	b	.
.endmacro

                                         
.macro SWITCH_TO_KERN_STACK
	ldr		x1, [x1, TH_KSTACKPTR]	                                         
	mov		sp, x1			                                            
.endmacro

                                         
.macro SWITCH_TO_INT_STACK
	mrs		x1, TPIDR_EL1
	ldr		x1, [x1, ACT_CPUDATAP]
	ldr		x1, [x1, CPU_ISTACKPTR]
	mov		sp, x1			                                               
.endmacro

  
                
  
                                                                                
                                                               
   
.macro REENABLE_DAIF
	                 
	tst		$0, #(DAIF_IRQF | DAIF_FIQF | DAIF_ASYNCF)
	b.eq		3f

	                
	tst		$0, #(DAIF_IRQF | DAIF_FIQF)
	b.eq		2f

	               
	tst		$0, #(DAIF_ASYNCF)
	b.eq		1f

	                     
	b		4f

	               
1:
	msr		DAIFClr, #(DAIFSC_ASYNCF)
	b		4f

	                
2:
	msr		DAIFClr, #(DAIFSC_IRQF | DAIFSC_FIQF)
	b		4f

	                 
3:
	msr		DAIFClr, #(DAIFSC_IRQF | DAIFSC_FIQF | DAIFSC_ASYNCF)

	           
4:
.endmacro

