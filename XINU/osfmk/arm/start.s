/*
 * Copyright (c) 2007-2014 Apple Inc. All rights reserved.
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

#include <arm/asm.h>
#include <arm/proc_reg.h>
#include <mach_kdp.h>
#include "assym.s"

	.text
	.align 12

	.align 2
	.globl EXT(resume_idle_cpu)
LEXT(resume_idle_cpu)
	                                  
	                                  
	LOAD_ADDR(lr, arm_init_idle_cpu)
	b		L_start_cpu_0

	.globl EXT(start_cpu)
LEXT(start_cpu)
	                                  
	                                  
	LOAD_ADDR(lr, arm_init_cpu)
	b		L_start_cpu_0

L_start_cpu_0:
	cpsid	if									                  

	                                              
	mcr		p15, 0, r11, c7, c5, 0				                        
	isb											                   
	mrc		p15, 0, r11, c1, c0, 0				                            
	orr		r11, r11, #(SCTLR_ICACHE | SCTLR_PREDIC)	                               
	mcr		p15, 0, r11, c1, c0, 0				                  
	dsb											                                  
	isb											                   

	                                                            
	ldr		r8, [r0, BA_PHYS_BASE]				                          
	ldr		r9, [r0, BA_VIRT_BASE]				                          
	ldr		r10, [r0, BA_MEM_SIZE]				                          

	                                                     
	ldr		r4, [r0, BA_TOP_OF_KERNEL_DATA]		                             
	orr		r5, r4, #(TTBR_SETUP & 0x00FF)		                              
	orr		r5, r5, #(TTBR_SETUP & 0xFF00)		                              
	mcr		p15, 0, r5, c2, c0, 0				                                           
	mcr		p15, 0, r5, c2, c0, 1				                                   
	mov		r5, #TTBCR_N_SETUP					                                     
	mcr		p15, 0, r5, c2, c0, 2				                                         
	ldr		r2, [r1, CPU_NUMBER_GS]				                 
	mcr		p15, 0, r2, c13, c0, 3				                 
	ldr		sp, [r1, CPU_INTSTACK_TOP]			                          
	sub		sp, sp, SS_SIZE						                    
	sub		r0, r1, r8							                             
	add		r0, r0, r9
	b		join_start

	.align 2
	.globl EXT(_start)
LEXT(_start)
	                                
	                 
	mov		r1, #0
	LOAD_ADDR(lr, arm_init)
	cpsid	if									                  

	                                              
	mcr		p15, 0, r11, c7, c5, 0				                        
	isb											                   
	mrc		p15, 0, r11, c1, c0, 0				                            
	orr		r11, r11, #(SCTLR_ICACHE | SCTLR_PREDIC)	                               
	mcr		p15, 0, r11, c1, c0, 0				                  
	dsb											                                  
	isb											                   

	                                                              
	ldr		r8, [r0, BA_PHYS_BASE]				                          
	ldr		r9, [r0, BA_VIRT_BASE]				                          
	ldr		r10, [r0, BA_MEM_SIZE]				                          

#define LOAD_PHYS_ADDR(reg, label) \
	LOAD_ADDR(reg, label); \
	sub		reg, reg, r9; \
	add		reg, reg, r8

	                                                                       
	LOAD_ADDR(r4, fleh_reset)
	LOAD_PHYS_ADDR(r5, ExceptionVectorsTable)
	str		r4, [r5]
	LOAD_ADDR(r4, fleh_undef)
	add		r5, #4
	str		r4, [r5]
	LOAD_ADDR(r4, fleh_swi)
	add		r5, #4
	str		r4, [r5]
	LOAD_ADDR(r4, fleh_prefabt)
	add		r5, #4
	str		r4, [r5]
	LOAD_ADDR(r4, fleh_dataabt)
	add		r5, #4
	str		r4, [r5]
	LOAD_ADDR(r4, fleh_addrexc)
	add		r5, #4
	str		r4, [r5]
	LOAD_ADDR(r4, fleh_irq)
	add		r5, #4
	str		r4, [r5]
	LOAD_ADDR(r4, fleh_decirq)
	add		r5, #4
	str		r4, [r5]

	                                                                                    
	                                                                                 
	                                                                              
	LOAD_ADDR(r4, arm_init_tramp)
	adr		r5, arm_init_tramp_addr
	str		r4, [r5]

#undef LOAD_PHYS_ADDR

	                                                     
	ldr		r4, [r0, BA_TOP_OF_KERNEL_DATA]		                             
	orr		r5, r4, #(TTBR_SETUP & 0x00FF)		                              
	orr		r5, r5, #(TTBR_SETUP & 0xFF00)		                              
	mcr		p15, 0, r5, c2, c0, 0				                                           
	mcr		p15, 0, r5, c2, c0, 1				                                   
	mov		r5, #TTBCR_N_SETUP					                                     
	mcr		p15, 0, r5, c2, c0, 2				                                         
		
	                                                                      
	                                                               
	                                                                             
	                                                                      
	mov		r5, r4								                     
	mov		r11, #ARM_TTE_TYPE_FAULT			                         
	mov		r2, PGBYTES >> 2					                      
	add		r2, r2, r2, LSL #2					                                             
	mov		r2, r2, LSL #1						                        
invalidate_tte:
	str		r11, [r5]							                        
	add		r5, r5, #4							                        
	subs	r2, r2, #1							                  
	bne		invalidate_tte

	                                      
	mov		r6, #ARM_TTE_TYPE_BLOCK				                            
	mov		r7, #(ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DEFAULT) & 0xFF)
	orr		r7, r7, #(ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DEFAULT) & 0xFF00)
	orr		r7, r7, #(ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_DEFAULT) & 0xF0000)
	orr		r6, r6, r7							                           
	mov		r7, #ARM_TTE_BLOCK_AP(AP_RWNA)		                                
	orr		r7, r7, #(ARM_TTE_BLOCK_AP(AP_RWNA) & 0xFF00)
	orr		r7, r7, #(ARM_TTE_BLOCK_AP(AP_RWNA) & 0xF0000)
	orr		r6, r6, r7							                       

	orr		r6, r6, #ARM_TTE_BLOCK_AF			                         
	orr		r6, r6, #ARM_TTE_BLOCK_SH			                   

	                                                                    
	lsr		r7, pc, #ARM_TT_L1_SHIFT			                                
	add		r5, r4, r7, LSL #2					                                   
	lsl		r7, r7, #ARM_TT_L1_SHIFT			                                  
	orr		r11, r7, r6							                       
	str		r11, [r5]							            

	                                                                                 
	mov		r7, r8								                          
	add		r5, r4, r9, LSR #ARM_TT_L1_SHIFT-2	                               
	mov		r3, #ARM_TT_L1_SIZE					                   
	
mapveqp:
	cmp		r3, r10								                                             
	bgt		mapveqpL2							                                    

	orr		r11, r7, r6							                       
	str		r11, [r5], #4						                             
	add		r7, r7, #ARM_TT_L1_SIZE				                         
	subs	r10, r10, #ARM_TT_L1_SIZE			                    
	bne		mapveqp
	b		doneveqp							                                     
	
mapveqpL2:
	                                                                           
	
	                     
	add		r6, r4, PGBYTES * 8					                
	mov 		r11, r6
	
	orr		r6, #ARM_TTE_TYPE_TABLE				               
	
	str		r6, [r5]							                          
	
	                         
	mov 		r5, r11
	
	                      
	mov		r2, #ARM_PTE_TYPE					                   
	orr		r2, r2, #(ARM_PTE_ATTRINDX(CACHE_ATTRINDX_DEFAULT) & 0xff)	                           
	orr		r2, r2, #(ARM_PTE_ATTRINDX(CACHE_ATTRINDX_DEFAULT) & 0xff00)
	orr		r2, r2, #(ARM_PTE_AP(AP_RWNA) & 0xff)	                           
	orr		r2, r2, #(ARM_PTE_AP(AP_RWNA) & 0xff00)
	orr		r2, r2, #ARM_PTE_AF					              
	orr		r2, r2, #ARM_PTE_SH					                    
	
storepte:
	orr		r11, r7, r2							                       
	str		r11, [r5], #4						                             
	add		r7, r7,  PGBYTES					                         
	subs	r10, r10, PGBYTES					                    
	bne		storepte

doneveqp:
	                                                                                   
	mov		r5, #0xff000000						                                        
	orr		r5, r5, #0x00ff0000					                                        
	mov		r5, r5, LSR #ARM_TT_L1_SHIFT		                             
	add		r5, r4, r5, LSL #2					                         

	add		r6, r4, PGBYTES * 9					                                                     
	add		r6, r6, #0xc00						                             
	mov		r7, #(ARM_TTE_TABLE_MASK & 0xFFFF) 	                                  
	movt		r7, #(ARM_TTE_TABLE_MASK >> 16)		                                   
	and		r11, r6, r7							             
	orr		r11, r11, #ARM_TTE_TYPE_TABLE		                                 
	str		r11, [r5]							                                 

	                      
	mov		r2, #ARM_PTE_TYPE					           
	orr		r2, r2, #(ARM_PTE_ATTRINDX(CACHE_ATTRINDX_DEFAULT) & 0x00ff)	                      
	orr		r2, r2, #(ARM_PTE_ATTRINDX(CACHE_ATTRINDX_DEFAULT) & 0xff00)
	orr		r2, r2, #(ARM_PTE_AP(AP_RWNA) & 0x00ff)	                       
	orr		r2, r2, #(ARM_PTE_AP(AP_RWNA) & 0xff00)
	orr		r2, r2, #ARM_PTE_AF					              
	orr		r2, r2, #ARM_PTE_SH					                    

	                                                                
	mov		r5, #0xff000000						                           
	orr		r5, r5, #0x00ff0000					                           
	mov		r7, #(ARM_TT_L2_INDEX_MASK & 0xFFFF)                                     
	movt	r7, #(ARM_TT_L2_INDEX_MASK >> 16)	                                     
	and		r5, r5, r7 							                          
	mov		r5, r5, LSR #ARM_TT_L2_SHIFT		                       
	add		r5, r6, r5, LSL #2					                         

	LOAD_ADDR(r11, ExceptionVectorsBase)		                              
	sub		r11, r11, r9						                              
	add		r11, r11, r8

	mov		r7, #(ARM_PTE_PAGE_MASK & 0xFFFF) 	                                 
	movt	r7, #(ARM_PTE_PAGE_MASK >> 16)		                                  
	and		r11, r11, r7						                                 
	orr		r11, r11, r2						                    
	str		r11, [r5]							                              

	                   
	mov		r11, #0
cleanflushway:
cleanflushline:		
	mcr		p15, 0, r11, c7, c14, 2				                                     
	add		r11, r11, #1 << MMU_I7SET			                       
	tst		r11, #1 << (MMU_NSET + MMU_I7SET)	                     
	beq		cleanflushline
	bic		r11, r11, #1 << (MMU_NSET + MMU_I7SET)                      
	adds	r11, r11, #1 << MMU_I7WAY			                 
	bcc		cleanflushway				 		        

#if	__ARM_L2CACHE__
	                      
	mov		r11, #2
invall2flushway:
invall2flushline:		
	mcr		p15, 0, r11, c7, c14, 2				                                     
	add		r11, r11, #1 << L2_I7SET			                       
	tst		r11, #1 << (L2_NSET + L2_I7SET)		                     
	beq		invall2flushline
	bic		r11, r11, #1 << (L2_NSET + L2_I7SET)                      
	adds	r11, r11, #1 << L2_I7WAY			                 
	bcc		invall2flushway				 		        

#endif

	mov		r11, #0
	mcr		p15, 0, r11, c13, c0, 3				                 
	LOAD_ADDR(sp, intstack_top)					                          
	sub		sp, sp, SS_SIZE						                    
	sub		r0, r0, r8							                             
	add		r0, r0, r9

join_start:
	                             
	                                                    
	                                                    
	                                                    

	                             

#ifdef  ARMA7
#if	 __ARMA7_SMP__
	mrc		p15, 0, r11, c1, c0, 1
	orr		r11, r11, #(1<<6)						      
	mcr		p15, 0, r11, c1, c0, 1
	isb
#endif
#endif

	mrs		r11, cpsr							           
	bic		r11, #0x100							                     
	msr		cpsr_x, r11							              

	mov		r11, #0
	mcr		p15, 0, r11, c8, c7, 0				                             
	mcr		p15, 0, r11, c7, c5, 0				                        

	           
	mov		r11, #(ARM_DAC_SETUP & 0xFFFF) 		                             
	movt	r11, #(ARM_DAC_SETUP >> 16)			                              
	mcr		p15, 0, r11, c3, c0, 0				                        

	           
	mov		r11, #(PRRR_SETUP & 0xFFFF) 		                          
	movt	r11, #(PRRR_SETUP >> 16)			                           
	mcr		p15, 0, r11, c10,c2,0				                         

	           
	mov		r11, #(NMRR_SETUP & 0xFFFF)			                          
	movt	r11, #(NMRR_SETUP >> 16)			                           
	mcr		p15, 0, r11, c10,c2,1				                         

	            
	mrc		p15, 0, r11, c1, c0, 0				                       

	bic		r11, r11, #SCTLR_ALIGN				                                 
	mov		r7, #(SCTLR_AFE|SCTLR_TRE)			                         
	orr		r7, r7, #(SCTLR_HIGHVEC | SCTLR_ICACHE | SCTLR_PREDIC)
	orr		r7, r7, #(SCTLR_DCACHE | SCTLR_ENABLE)
#if  (__ARM_ENABLE_SWAP__ == 1)
	orr		r7, r7, #SCTLR_SW					                  
#endif
	orr		r11, r11, r7						                             
	mcr		p15, 0, r11, c1, c0, 0				                  

	dsb											                                  
	isb											                   

#if __ARM_VFP__
	                                   
	mrc		p15, 0, r2, c1, c0, 2				                                    
	mov		r3, #15								      
	orr		r2, r2, r3, LSL #20					                   
	mcr		p15, 0, r2, c1, c0, 2				                                     
	isb
#endif	                 
		
	                                              
	cmp		r1, #0								                             
	beq		join_start_1						                
	ldr		r7, arm_init_tramp_addr				                          
	bx		r7									                                       

	                                                                  
	                                                                     
	                                                             
arm_init_tramp_addr:
	.long	0
	.globl EXT(arm_init_tramp)
LEXT(arm_init_tramp)
	mrc		p15, 0, r5, c2, c0, 0				                                   
	add		r5, r5, PGBYTES * 4 				                                                     
	mcr		p15, 0, r5, c2, c0, 0				                                           
	mcr		p15, 0, r5, c2, c0, 1				                                   
	isb
	mov		r5, #0
	mcr		p15, 0, r5, c8, c7, 0				                        
	dsb											                                  
	isb											                   

join_start_1:
#if __ARM_VFP__
	                                               
	                                                 
	                                                  
	fmrx	r2, fpexc							            
	orr		r2, #FPEXC_EN						                     
	fmxr	fpexc, r2							            
	mov		r2, #FPSCR_DEFAULT					                    
	fmxr	fpscr, r2							            
#endif	                 

	mov		r7, #0								                    
	bx		lr

LOAD_ADDR_GEN_DEF(arm_init)
LOAD_ADDR_GEN_DEF(arm_init_cpu)
LOAD_ADDR_GEN_DEF(arm_init_idle_cpu)
LOAD_ADDR_GEN_DEF(arm_init_tramp)
LOAD_ADDR_GEN_DEF(fleh_reset)
LOAD_ADDR_GEN_DEF(ExceptionVectorsTable)
LOAD_ADDR_GEN_DEF(fleh_undef)
LOAD_ADDR_GEN_DEF(fleh_swi)
LOAD_ADDR_GEN_DEF(fleh_prefabt)
LOAD_ADDR_GEN_DEF(fleh_dataabt)
LOAD_ADDR_GEN_DEF(fleh_addrexc)
LOAD_ADDR_GEN_DEF(fleh_irq)
LOAD_ADDR_GEN_DEF(fleh_decirq)

#include "globals_asm.h"

                    
