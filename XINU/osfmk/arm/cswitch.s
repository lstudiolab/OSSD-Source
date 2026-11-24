/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
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
#include <machine/asm.h>
#include <arm/proc_reg.h>
#include "assym.s"

  
                     
  
                                                                                                    
                      
   
.macro	save_vfp_registers
#if     __ARM_VFP__
	fmrx		r2, fpscr						                           
	str			r2, [r3, VSS_FPSCR]				                                  
	add			r3, r3, #64						                                
#if     (__ARM_VFP__ >= 3)
	vstmia.64	r3!, {d8-d11}
	vstmia.64	r3!, {d12-d15}
#else
	fstmias		r3!, {s16-s31}
#endif                       
#endif                  
.endmacro

  
                     
  
                                                                                                      
                      
   
.macro	load_vfp_registers
#if     __ARM_VFP__
	add			r2, r3, #64						                                
#if     (__ARM_VFP__ >= 3)
	vldmia.64	r2!, {d8-d11}
	vldmia.64	r2!, {d12-d15}
#else
	fldmias		r2!, {s16-s31}
#endif                       
	ldr			r3, [r3, VSS_FPSCR]				                               
	fmxr		fpscr, r3						                    
#endif                  
.endmacro

  
                                                        
  
                                                    
               
   
	.syntax unified
	.text
	.align 2
	.globl	EXT(machine_load_context)

LEXT(machine_load_context)
	mcr		p15, 0, r0, c13, c0, 4				                 
	ldr		r1, [r0, TH_CTH_SELF]
	mrc		p15, 0, r2, c13, c0, 3				                
	and		r2, r2, #3							                     
	orr		r1, r1, r2							   
	mcr		p15, 0, r1, c13, c0, 3				                 
	mov		r1, #0
	mcr		p15, 0, r1, c13, c0, 2				                 
	mov		r7, #0								                      
	ldr		r3, [r0, TH_KSTACKPTR]				                       
	mov		r0, #0								           
	add		r3, r3, SS_R4
	ldmia	r3!, {r4-r14}						                     
	bx		lr									         

  
                                                                 
  
                                                          
                               
                                        
                                                   
   
	.text
	.align	5
	.globl	EXT(Call_continuation)

LEXT(Call_continuation)
	mrc		p15, 0, r9, c13, c0, 4				                
	ldr		sp, [r9, TH_KSTACKPTR]				                    
	mov		r7, #0								                      

	mov		r4,r0								                    
	mov		r5,r1								                         
	mov		r6,r2								                      

    teq     r3, #0
    beq     1f
    mov     r0, #1
    bl _ml_set_interrupts_enabled
1:
    
	mov		r0,r5								                      
	mov		r1,r6								                      
	blx		r4									                         

	mrc		p15, 0, r0, c13, c0, 4				                
	LOAD_ADDR_PC(thread_terminate)
	b		.									            


  
                                        
                           
                   
   
	.text
	.align 5
	.globl	EXT(Switch_context)

LEXT(Switch_context)
	teq		r1, #0								                                  
	bne		switch_threads						                                           
#if     __ARM_VFP__
	mov		r1, r2								                                                   
	ldr		r3, [r0, TH_KSTACKPTR]				                           
	add		r3, r3, SS_KVFP						                                                     
	save_vfp_registers							                                  
	mov		r2, r1								                                      
#endif                  
	ldr		r3, [r0, TH_KSTACKPTR]				                           
	add		r3, r3, SS_R4
	stmia		r3!, {r4-r14}					                                
switch_threads:
	ldr		r3, [r2, ACT_CPUDATAP]
	str		r2, [r3, CPU_ACTIVE_THREAD]
	ldr		r3, [r2, TH_KSTACKPTR]				                       
	mcr		p15, 0, r2, c13, c0, 4				                 
	ldr		r6, [r2, TH_CTH_SELF]
	mrc		p15, 0, r5, c13, c0, 3				                
	and		r5, r5, #3							                     
	orr		r6, r6, r5
	mcr		p15, 0, r6, c13, c0, 3				                 
	mov		r6, #0
	mcr		p15, 0, r6, c13, c0, 2				                 
load_reg:	
	add		r3, r3, SS_R4
	ldmia	r3!, {r4-r14}						                            
#if     __ARM_VFP__
	ldr		r3, [r2, TH_KSTACKPTR]				                       
	add		r3, r3, SS_KVFP						                                                     
	load_vfp_registers							                              
#endif                  
	bx		lr									         

  
                                                                                    
  
   
	.text
	.align 5
	.globl	EXT(Shutdown_context)

LEXT(Shutdown_context)
	mrc		p15, 0, r9, c13, c0, 4				                
#if __ARM_VFP__
	ldr		r3, [r9, TH_KSTACKPTR]				                       
	add		r3, r3, SS_KVFP						                                                         
	save_vfp_registers							                                  
#endif
	ldr		r3, [r9, TH_KSTACKPTR]				                       
	add		r3, r3, SS_R4
	stmia	r3!, {r4-r14}						                                
	cpsid	if									                  

	ldr		r12, [r9, ACT_CPUDATAP]				                  
	ldr		sp, [r12, CPU_ISTACKPTR]			                            
	LOAD_ADDR_PC(cpu_doshutdown)

  
                              
  
   
	.text
	.align 5
	.globl	EXT(Idle_context)

LEXT(Idle_context)

	mrc		p15, 0, r9, c13, c0, 4				                
#if	__ARM_VFP__
	ldr		r3, [r9, TH_KSTACKPTR]				                       
	add		r3, r3, SS_KVFP						                                                         
	save_vfp_registers							                                  
#endif
	ldr		r3, [r9, TH_KSTACKPTR]				                       
	add		r3, r3, SS_R4
	stmia	r3!, {r4-r14}						                                

	ldr		r12, [r9, ACT_CPUDATAP]				                  
	ldr		sp, [r12, CPU_ISTACKPTR]			                            
	LOAD_ADDR_PC(cpu_idle)

  
                              
  
   
	.text
	.align 5
	.globl	EXT(Idle_load_context)

LEXT(Idle_load_context)

	mrc		p15, 0, r12, c13, c0, 4				                
	ldr		r3, [r12, TH_KSTACKPTR]				                       
	add		r3, r3, SS_R4
	ldmia	r3!, {r4-r14}						                            
#if __ARM_VFP__
	ldr		r3, [r9, TH_KSTACKPTR]				                       
	add		r3, r3, SS_KVFP						                                                         
	load_vfp_registers							                              
#endif
	bx		lr									         

  
                                                    
   
	.text
	.align 2
	.globl	EXT(vfp_save)

LEXT(vfp_save)
#if	__ARM_VFP__
	fmrx        r1, fpscr                                                  
	str         r1, [r0, VSS_FPSCR]                                               
#if     (__ARM_VFP__ >= 3)
	vstmia.64   r0!, {d0-d3}                                         
	vstmia.64   r0!, {d4-d7}
	vstmia.64   r0!, {d8-d11}
	vstmia.64   r0!, {d12-d15}
	vstmia.64   r0!, {d16-d19}
	vstmia.64   r0!, {d20-d23}
	vstmia.64   r0!, {d24-d27}
	vstmia.64   r0!, {d28-d31}
#else
	fstmias     r0!, {s0-s31}                                        
#endif
#endif                   
	bx          lr                                       

  
                                                   
  
                                                    
   
	.text
	.align 2
	.globl	EXT(vfp_load)
LEXT(vfp_load)
#if __ARM_VFP__
	                                                    
	mov         r1, r0
#if (__ARM_VFP__ >= 3)
	vldmia.64   r0!, {d0-d3}                                            
	vldmia.64   r0!, {d4-d7}
	vldmia.64   r0!, {d8-d11}
	vldmia.64   r0!, {d12-d15}
	vldmia.64   r0!, {d16-d19}
	vldmia.64   r0!, {d20-d23}
	vldmia.64   r0!, {d24-d27}
	vldmia.64   r0!, {d28-d31}
#else
	fldmias	    r0!, {s0-s31}                                           
#endif                       
	ldr         r1, [r1, VSS_FPSCR]                                                
	fmxr        fpscr, r1                                                          
#endif                  
	bx          lr                                       

#include        "globals_asm.h"

LOAD_ADDR_GEN_DEF(thread_terminate)
LOAD_ADDR_GEN_DEF(cpu_doshutdown)
LOAD_ADDR_GEN_DEF(cpu_idle)

                    

