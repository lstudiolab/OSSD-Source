/*
 * Copyright (c) 2007-2011 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <machine/asm.h>
#include <arm/proc_reg.h>
#include <pexpert/arm/board_config.h>
#include <mach/arm/traps.h>
#include <mach/exception_types.h>
#include <mach_kdp.h>
#include <mach_assert.h>
#include <config_dtrace.h>
#include "assym.s"
#include "dwarf_unwind.h"

#define TRACE_SYSCALL 0

  
                                             
                                            
                                              
   
	.syntax unified
	.text
	.align 12
	.globl EXT(ExceptionLowVectorsBase)

LEXT(ExceptionLowVectorsBase)	
	adr	pc, Lreset_low_vector
	b	.	        
	b	.	      
	b	.	                 
	b	.	             
	b	.	                    
	b	.	      
	b	.	          
LEXT(ResetPrivateData)
	.space  (480),0		                   
	                                  
Lreset_low_vector:
	adr		r4, EXT(ResetHandlerData)
	ldr		r0, [r4, ASSIST_RESET_HANDLER]
	movs	r0, r0
	blxne	r0
	adr		r4, EXT(ResetHandlerData)
	ldr		r1, [r4, CPU_DATA_ENTRIES]
	ldr		r1, [r1, CPU_DATA_PADDR]
	ldr		r5, [r1, CPU_RESET_ASSIST]
	movs	r5, r5
	blxne	r5
	adr		r4, EXT(ResetHandlerData)
	ldr		r0, [r4, BOOT_ARGS]
	ldr		r1, [r4, CPU_DATA_ENTRIES]
#if	defined(ARMA7)
	                                                          
	mrc		p15, 0, r6, c0, c0, 5				             
	and		r6, r6, #0xFF						                           
#else
#error missing Who Am I implementation
#endif
	                                         
                   
                                              
        
	lsl		r6, r6, #4							                          
	add		r1, r1, r6							                              
	ldr		r1, [r1, CPU_DATA_PADDR]
	ldr		r5, [r1, CPU_RESET_HANDLER]
	movs	r5, r5
	blxne	r5									                              
	b		.									                   
	.globl  EXT(ResetHandlerData)
LEXT(ResetHandlerData)
	.space  (rhdSize_NUM),0		                   


        .globl EXT(ExceptionLowVectorsEnd)
LEXT(ExceptionLowVectorsEnd)	

	.text
	.align 12
	.globl EXT(ExceptionVectorsBase)

LEXT(ExceptionVectorsBase)	

	adr	pc, Lexc_reset_vector
	adr	pc, Lexc_undefined_inst_vector
	adr	pc, Lexc_swi_vector
	adr	pc, Lexc_prefetch_abort_vector
	adr	pc, Lexc_data_abort_vector
	adr	pc, Lexc_address_exception_vector
	adr	pc, Lexc_irq_vector
#if __ARM_TIME__
	adr	pc, Lexc_decirq_vector
#else                     
	mov	pc, r9
#endif                   

Lexc_reset_vector:
	b	.
	.long	0x0
	.long	0x0
	.long	0x0
Lexc_undefined_inst_vector:
	mrc		p15, 0, sp, c13, c0, 4				                
	ldr		sp, [sp, ACT_CPUDATAP]				                       
	ldr		sp, [sp, CPU_EXC_VECTORS]			                             
	ldr		pc, [sp, #4]						                              
Lexc_swi_vector:
	mrc		p15, 0, sp, c13, c0, 4				                
	ldr		sp, [sp, ACT_CPUDATAP]				                       
	ldr		sp, [sp, CPU_EXC_VECTORS]			                             
	ldr		pc, [sp, #8]						                              
Lexc_prefetch_abort_vector:
	mrc		p15, 0, sp, c13, c0, 4				                
	ldr		sp, [sp, ACT_CPUDATAP]				                       
	ldr		sp, [sp, CPU_EXC_VECTORS]			                             
	ldr		pc, [sp, #0xC]						                              
Lexc_data_abort_vector:
	mrc		p15, 0, sp, c13, c0, 4				                
	ldr		sp, [sp, ACT_CPUDATAP]				                       
	ldr		sp, [sp, CPU_EXC_VECTORS]			                             
	ldr		pc, [sp, #0x10]						                              
Lexc_address_exception_vector:
	mrc		p15, 0, sp, c13, c0, 4				                
	ldr		sp, [sp, ACT_CPUDATAP]				                       
	ldr		sp, [sp, CPU_EXC_VECTORS]			                             
	ldr		pc, [sp, #0x14]						                              
Lexc_irq_vector:
	mrc		p15, 0, sp, c13, c0, 4				                
	ldr		sp, [sp, ACT_CPUDATAP]				                       
	ldr		sp, [sp, CPU_EXC_VECTORS]			                             
	ldr		pc, [sp, #0x18]						                              
#if __ARM_TIME__
Lexc_decirq_vector:
	mrc		p15, 0, sp, c13, c0, 4				                
	ldr		sp, [sp, ACT_CPUDATAP]				                       
	ldr		sp, [sp, CPU_EXC_VECTORS]			                             
	ldr		pc, [sp, #0x1C]						                              
#else                     
	.long	0x0
	.long	0x0
	.long	0x0
	.long	0x0
#endif                   

	.fill   984, 4, 0						                                

    .globl EXT(ExceptionVectorsEnd)
LEXT(ExceptionVectorsEnd)	


  
                                                                          
                                                                                
   
	.globl EXT(ExceptionVectorsTable)
LEXT(ExceptionVectorsTable)	
Lreset_vector:
	.long	0x0
Lundefined_inst_vector:
	.long	0x0
Lswi_vector:
	.long	0x0
Lprefetch_abort_vector:
	.long	0x0
Ldata_abort_vector:
	.long	0x0
Laddress_exception_vector:
	.long	0x0
Lirq_vector:
	.long	0x0
Ldecirq_vector:
	.long	0x0


  
                                 
   
	.text
	.align 2
	.globl EXT(fleh_reset)
LEXT(fleh_reset)
	b		.									               

  
                                                           
   
	.text
	.align 2
	.globl EXT(fleh_undef)

  
                                                                                     
                                                   
                                                             
   
.macro ALIGN_STACK
  
                                                        
   
#if __BIGGEST_ALIGNMENT__ > 4
	and		$0, sp, #0x0F						                  
	cmp		$0, #4							                                     
	addlt		$0, $0, #0x10						                                              
	mov		$1, sp							                 
	sub		sp, sp, $0						              
	str		$1, [sp]						                             
#endif
.endmacro

  
                                                                                 
   
.macro UNALIGN_STACK
#if __BIGGEST_ALIGNMENT__ > 4
	ldr		sp, [sp]
#endif
.endmacro

  
                                                                    
                                                                   
   
.macro VERIFY_EXCEPTION_MODE
	mrs		sp, cpsr 							            
	and		sp, sp, #PSR_MODE_MASK					                       
	cmp		sp, $0							                       
	movne		r0, sp
	bne		EXT(ExceptionVectorPanic)
.endmacro

  
                                                                      
                                                                     
                                                                   
            
                                                                        
    
.macro IF_USERMODE_EXCEPTION
	mrs		sp, spsr
	and		sp, sp, #PSR_MODE_MASK						                   
	cmp		sp, #PSR_USER_MODE
	beq		$0_from_user
	cmp		sp, #PSR_IRQ_MODE
	beq		$0_from_irq
	cmp		sp, #PSR_FIQ_MODE
	beq		$0_from_fiq
	bne		$0_from_svc
$0_from_user:
.endmacro

  
                                                                
                                                              
                                                           
                                                                    
                                                                        
   
.macro ELSE_IF_KERNELMODE_EXCEPTION
$0_from_irq:
	cpsid		i, #PSR_IRQ_MODE
	b		$0_from_kernel
$0_from_fiq:
	cpsid		i, #PSR_FIQ_MODE
	b		$0_from_kernel
$0_from_svc:
	cpsid		i, #PSR_SVC_MODE
$0_from_kernel:
.endmacro

LEXT(fleh_undef)
VERIFY_EXCEPTION_MODE PSR_UND_MODE
	mrs		sp, spsr							                              
	tst		sp, #PSR_TF							               
	subeq		lr, lr, #4
	subne		lr, lr, #2
IF_USERMODE_EXCEPTION undef
	mrc		p15, 0, sp, c13, c0, 4				                
	add		sp, sp, ACT_PCBDATA				                                 

	stmia	sp, {r0-r12, sp, lr}^				                           
	mov		r7, #0								                         
	nop

	mov		r0, sp								                                 
												                

	str		lr, [sp, SS_PC]						                             

	mrs		r4, spsr
	str		r4, [sp, SS_CPSR]					                      

	cpsid i, #PSR_SVC_MODE
	mrs		r3, cpsr 							            
	msr		spsr_cxsf, r3                                                 
	mrc		p15, 0, r9, c13, c0, 4				                
	ldr		sp, [r9, TH_KSTACKPTR]				                    
#if __ARM_USER_PROTECT__
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r3, c2, c0, 0				            
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif

	mvn		r0, #0
	str		r0, [r9, TH_IOTIER_OVERRIDE]			                                                                    

#if	!CONFIG_SKIP_PRECISE_USER_KERNEL_TIME
	bl		EXT(timer_state_event_user_to_kernel)
	mrc		p15, 0, r9, c13, c0, 4				                
#endif

#if __ARM_VFP__
	add		r0, r9, ACT_UVFP				                                            
	bl		EXT(vfp_save)					                                         
	mov		r3, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r3					                          
	add		r1, r9, ACT_UVFP				                                       
	add		r0, r9, ACT_PCBDATA				                                     
#else
	mov		r1, #0                              		                                    
	add		r0, r9, ACT_PCBDATA					                                 
#endif

	bl		EXT(sleh_undef)						                            
												                               
	b		load_and_go_user

ELSE_IF_KERNELMODE_EXCEPTION undef
	  
                                                                      
                   
    
#if CONFIG_DTRACE
	                                                                                                                  
	                                                                                                   

	                                                                                                                 
	                             
	stmfd sp!, { r7, lr }
#endif

	sub		sp, sp, EXC_CTX_SIZE						                              

	stmia	sp, {r0-r12}						                                
	str		lr, [sp, SS_LR]
	
#if CONFIG_DTRACE
	add		r7, sp, EXC_CTX_SIZE						                     
#endif

	mrs		r4, lr_und
	str		r4, [sp, SS_PC]						                
	mrs		r4, spsr_und
	str		r4, [sp, SS_CPSR]	

	mov		ip, sp

  
                     
                     
                           
   


#if CONFIG_DTRACE
	ldr		r0, [ip, SS_PC]						                                      
#endif

	add		ip, ip, EXC_CTX_SIZE						                                 
#if CONFIG_DTRACE
	str		r0, [ip, #4]
	add		ip, ip, #8
#endif
	str		ip, [sp, SS_SP]						                               
#if CONFIG_DTRACE
	sub		ip, ip, #8
#endif
	sub		ip, ip, EXC_CTX_SIZE

#if __ARM_VFP__
	mrc		p15, 0, r9, c13, c0, 4				                
	add		r0, sp, SS_SIZE					                        
	bic		r0, #(VSS_ALIGN_NUM - 1)			                                        
	add		r0, VSS_ALIGN					                               
	mov		r5, r0						                                          
	bl		EXT(vfp_save)					                                          
	mov		r1, r5						                                   
	mov		r4, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r4					                          
#else
	mov     r1, #0                                                                    
#endif
#if __ARM_USER_PROTECT__
	mrc		p15, 0, r10, c2, c0, 0				            
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	cmp		r3, r10
	beq		1f
	mcr		p15, 0, r3, c2, c0, 0				            
1:
	mrc		p15, 0, r11, c13, c0, 1				                  
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif
	mov		r0, sp								           

	ALIGN_STACK r2, r3
	bl		EXT(sleh_undef)						                            
	UNALIGN_STACK

#if __ARM_USER_PROTECT__
	mrc		p15, 0, r9, c13, c0, 4                              
	ldr		r0, [r9, ACT_KPTW_TTB]                                
	cmp		r10, r0
	beq		1f
	ldr		r10, [r9, ACT_UPTW_TTB]                               
	cmp		r10, r0
	beq		1f
	mcr		p15, 0, r10, c2, c0, 0                          
	ldr		r11, [r9, ACT_ASID]                                    
1:
	mcr		p15, 0, r11, c13, c0, 1                              
	isb
#endif
	b		load_and_go_sys


  
                                                       
  
                                                                            
                                                                
  
                                       
                                                                   
  
   
	.text
	.align 5
	.globl EXT(fleh_swi)

LEXT(fleh_swi)
	cpsid	i, #PSR_ABT_MODE
	mov		sp, ip								          
	cpsid	i, #PSR_SVC_MODE
	mrs		ip, spsr							                          
	tst		ip, #0x0f
	cpsid	i, #PSR_ABT_MODE
	mov		ip, sp								             
	cpsid	i, #PSR_SVC_MODE
	beq		swi_from_user

                                                            
swi_from_kernel:
	sub     sp, sp, EXC_CTX_SIZE
	stmia	sp, {r0-r12}
	add		r0, sp, EXC_CTX_SIZE

	str		r0, [sp, SS_SP]						                          
	str		lr, [sp, SS_LR]                                               

	ALIGN_STACK r0, r1
	adr		r0, L_kernel_swi_panic_str			                                  
	blx		EXT(panic)
	b		.

swi_from_user:
	mrc		p15, 0, sp, c13, c0, 4				                
	add		sp, sp, ACT_PCBDATA					               


	                                                   
                                                                               
                                                                                                        
	cmp		r12, #MACH_ARM_TRAP_ABSTIME
	beq		fleh_swi_trap_tb
	stmia	sp, {r0-r12, sp, lr}^				                           
	mov		r7, #0								                         
	nop
	mov		r8, sp								                                
	add		sp, sp, SS_PC
	srsia sp, 	#PSR_SVC_MODE
	mrs		r3, cpsr 							            
	msr		spsr_cxsf, r3                                                 
	sub		r9, sp, ACT_PCBDATA_PC

	ldr		sp, [r9, TH_KSTACKPTR]				                    
	mov		r11, r12							                                                   

#if __ARM_VFP__
	add		r0, r9, ACT_UVFP				                                            
	bl		EXT(vfp_save)					                                         
	mov		r4, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r4					                          
#endif
#if __ARM_USER_PROTECT__
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r3, c2, c0, 0				            
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif

	mvn		r0, #0
	str		r0, [r9, TH_IOTIER_OVERRIDE]			                                                                  

#if	!CONFIG_SKIP_PRECISE_USER_KERNEL_TIME
	bl		EXT(timer_state_event_user_to_kernel)
	mrc		p15, 0, r9, c13, c0, 4				                
	add		r8, r9, ACT_PCBDATA				                                 
#endif
	ldr		r10, [r9, ACT_TASK]				                        

	                       
	cpsie	i							             

	cmp		r11, #MACH_ARM_TRAP_CONTTIME			                                         
	beq		fleh_swi_trap_mct

	cmp		r11, #0x80000000
	beq		fleh_swi_trap
fleh_swi_trap_ret:

#if TRACE_SYSCALL
	                       
	mov		r0, r8
	bl		EXT(syscall_trace)
#endif

	bl		EXT(mach_kauth_cred_uthread_update)
	mrc		p15, 0, r9, c13, c0, 4				                          
	                   
	rsbs	r5, r11, #0							                                          
	ble		fleh_swi_unix						                                                       

fleh_swi_mach:
	                                                                      
                                      
	ldr		r2, [r9, TH_MACH_SYSCALLS]
	add		r2, r2, #1
	str		r2, [r9, TH_MACH_SYSCALLS]

	LOAD_ADDR(r1, mach_trap_table)				                       
#if MACH_TRAP_TABLE_ENTRY_SIZE_NUM == 12
	add		r11, r5, r5, lsl #1					              
	add		r6, r1, r11, lsl #2					                            
#elif MACH_TRAP_TABLE_ENTRY_SIZE_NUM == 16
	add		r6, r1, r5, lsl #4					                            
#elif MACH_TRAP_TABLE_ENTRY_SIZE_NUM == 20
	add		r11, r5, r5, lsl #2					              
	add		r6, r1, r11, lsl #2					                            
#else
#error mach_trap_t size unhandled (see MACH_TRAP_TABLE_ENTRY_SIZE)!
#endif

#ifndef	NO_KDEBUG
	LOAD_ADDR(r4, kdebug_enable)
	ldr		r4, [r4]
	movs	r4, r4
	movne	r0, r8								                                                    
	movne	r1, r5								                            
	COND_EXTERN_BLNE(mach_syscall_trace)
#endif
	adr		lr,	fleh_swi_exit					                                                          
	cmp		r5, MACH_TRAP_TABLE_COUNT			                             
	bge		fleh_swi_mach_error

   
                                                                     
                                                                
                                                                   
                                 
   
#if __BIGGEST_ALIGNMENT__ > 4
	sub		sp, #0x40						                                                 
	                                            				                                                        
	ldr		r11, [r6, #8]						                                                         
	teq		r11, #0							                            
	moveq		r0, #0
	movne		r0, r8							                                                    
	movne		r1, sp							                                   
	blxne		r11							                                              
	adr		lr,	fleh_swi_exit					                                                          
	teq		r0, #0
	bne		fleh_swi_mach_error					                                              
#endif

	ldr		r1, [r6, #4]						                          

	LOAD_ADDR(r2, kern_invalid)					                                                 
	teq		r1, r2
	beq		fleh_swi_mach_error

#if __BIGGEST_ALIGNMENT__ > 4
	mov		r0, sp								                           
	bx		r1									                           
#else
	mov		r0, r8								                                                    
	bx		r1									                           
#endif

fleh_swi_exit64:
	str		r1, [r8, #4]						                       
fleh_swi_exit:
	str		r0, [r8]							                        
#ifndef	NO_KDEBUG
	movs	r4, r4
	movne	r1, r5
	COND_EXTERN_BLNE(mach_syscall_trace_exit)
#endif
#if TRACE_SYSCALL
	bl		EXT(syscall_trace_exit)
#endif

	mov		r0, #1
	bl		EXT(throttle_lowpri_io)				                         

	bl		EXT(thread_exception_return)
	b		.

fleh_swi_mach_error:
	mov		r0, #EXC_SYSCALL
	sub		r1, sp, #4
	mov		r2, #1
	bl		EXT(exception_triage)
	b		.

	.align	5
fleh_swi_unix:
	ldr		r1, [r9, TH_UNIX_SYSCALLS]
	mov		r0, r8								                             
	add		r1, r1, #1
	str		r1, [r9, TH_UNIX_SYSCALLS]
	mov		r1, r9								                         
	ldr		r2, [r9, TH_UTHREAD]				                          
	ldr		r3, [r10, TASK_BSD_INFO]			                       
	bl		EXT(unix_syscall)
	b		.

fleh_swi_trap:
	ldmia		r8, {r0-r3}
	cmp		r3, #3
	addls	pc, pc, r3, LSL#2
	b		fleh_swi_trap_ret
	b		icache_invalidate_trap
	b		dcache_flush_trap
	b		thread_set_cthread_trap
	b		thread_get_cthread_trap

icache_invalidate_trap:
	add		r3, r0, r1
	cmp		r3, VM_MAX_ADDRESS
	subhi	r3, r3, #1<<MMU_CLINE
	bhi		cache_trap_error
	adr		r11, cache_trap_jmp	
	ldr		r6,  [r9, TH_RECOVER]				                                 
	str		r11, [r9, TH_RECOVER] 
#if __ARM_USER_PROTECT__
	ldr     r5, [r9, ACT_UPTW_TTB]				                  
	mcr		p15, 0, r5, c2, c0, 0				            
	ldr     r5, [r9, ACT_ASID]					                   
	mcr		p15, 0, r5, c13, c0, 1				                 
	dsb		ish
	isb
#endif
	bl		EXT(InvalidatePoU_IcacheRegion)
	mrc		p15, 0, r9, c13, c0, 4				                          
#if __ARM_USER_PROTECT__
	ldr		r4, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r4, c2, c0, 0				            
	mov		r4, #0								                   
	mcr		p15, 0, r4, c13, c0, 1				                 
	isb
#endif
	str		r6, [r9, TH_RECOVER]
	bl		EXT(thread_exception_return)
	b		.

dcache_flush_trap:
	add		r3, r0, r1
	cmp		r3, VM_MAX_ADDRESS
	subhi	r3, r3, #1<<MMU_CLINE
	bhi		cache_trap_error
	adr		r11, cache_trap_jmp	
	ldr		r4,  [r9, TH_RECOVER]				                                 
	str		r11, [r9, TH_RECOVER] 
#if __ARM_USER_PROTECT__
	ldr     r6, [r9, ACT_UPTW_TTB]                                
	mcr		p15, 0, r6, c2, c0, 0				            
	ldr     r5, [r9, ACT_ASID]					                   
	mcr		p15, 0, r5, c13, c0, 1				                 
	isb
#endif
	bl		EXT(flush_dcache_syscall)
	mrc		p15, 0, r9, c13, c0, 4				                          
#if __ARM_USER_PROTECT__
	ldr		r5, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r5, c2, c0, 0				            
	mov		r5, #0								                   
	mcr		p15, 0, r5, c13, c0, 1				                 
	isb
#endif
	str		r4, [r9, TH_RECOVER]
	bl		EXT(thread_exception_return)
	b		.

thread_set_cthread_trap:
	bl		EXT(thread_set_cthread_self)
	bl		EXT(thread_exception_return)
	b		.

thread_get_cthread_trap:
	bl		EXT(thread_get_cthread_self)
	mrc		p15, 0, r9, c13, c0, 4				                          
	add		r1, r9, ACT_PCBDATA					               
	str		r0, [r1, SS_R0]						                   
	bl		EXT(thread_exception_return)
	b		.

cache_trap_jmp:
#if __ARM_USER_PROTECT__
	mrc		p15, 0, r9, c13, c0, 4				                          
	ldr		r5, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r5, c2, c0, 0				            
	mov		r5, #0								                   
	mcr		p15, 0, r5, c13, c0, 1				                 
	isb
#endif
	mrc		p15, 0, r3, c6, c0 					                     
cache_trap_error:
	mrc		p15, 0, r9, c13, c0, 4				                          
	add		r0, r9, ACT_PCBDATA					               
	ldr		r1, [r0, SS_PC]						                                   
	sub		r1, r1, #4							                       
	str		r1, [r0, SS_PC]						                         
	str		r3, [r0, SS_VADDR]					                
	mov		r0, #EXC_BAD_ACCESS
	mov		r2, KERN_INVALID_ADDRESS
	sub		sp, sp, #8
	mov		r1, sp
	str		r2, [sp]
	str		r3, [sp, #4]
	ALIGN_STACK r2, r3
	mov		r2, #2
	bl		EXT(exception_triage)
	b		.

fleh_swi_trap_mct:
	bl 		EXT(mach_continuous_time)
	mrc		p15, 0, r9, c13, c0, 4				                
	add		r9, r9, ACT_PCBDATA_R0				                          
	stmia		r9, {r0, r1}					                          
	bl		EXT(thread_exception_return)
	b		.

fleh_swi_trap_tb:
	str		lr, [sp, SS_PC]
	bl		EXT(ml_get_timebase)				                                    
	ldr		lr, [sp, SS_PC]
	nop
	movs	pc, lr								                 

	.align  2
L_kernel_swi_panic_str:
	.asciz  "fleh_swi: took SWI from kernel mode\n"
	.align	2

  
                                                       
   
	.text
	.align 2
	.globl EXT(fleh_prefabt)
	
LEXT(fleh_prefabt)
VERIFY_EXCEPTION_MODE PSR_ABT_MODE
	sub		lr, lr, #4

IF_USERMODE_EXCEPTION prefabt
	mrc		p15, 0, sp, c13, c0, 4				                
	add		sp, sp, ACT_PCBDATA					               

	stmia   sp, {r0-r12, sp, lr}^				                           
	mov		r7, #0								                         
	nop
	mov     r0, sp								                                 
												               
	str		lr, [sp, SS_PC]						                                   
	mrc		p15, 0, r1, c6, c0, 2 				            
	str		r1, [sp, SS_VADDR]					                           

	mrc		p15, 0, r5, c5, c0, 1 				                    
	str		r5, [sp, SS_STATUS]					                                    

	mrs     r4, spsr
	str     r4, [sp, SS_CPSR]					                      

	cpsid	i, #PSR_SVC_MODE
	mrs		r3, cpsr 							            
	msr		spsr_cxsf, r3                                                 
	mrc		p15, 0, r9, c13, c0, 4				                
	ldr		sp, [r9, TH_KSTACKPTR]				                    

#if __ARM_VFP__
	add		r0, r9, ACT_UVFP				                                            
	bl		EXT(vfp_save)					                                         
	mov		r3, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r3					                          
#endif
#if __ARM_USER_PROTECT__
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r3, c2, c0, 0				            
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif

	mvn		r0, #0
	str		r0, [r9, TH_IOTIER_OVERRIDE]			                                                                    

#if	!CONFIG_SKIP_PRECISE_USER_KERNEL_TIME
	bl		EXT(timer_state_event_user_to_kernel)
	mrc		p15, 0, r9, c13, c0, 4				                
#endif

	add		r0, r9, ACT_PCBDATA					                                 
	mov		r1, T_PREFETCH_ABT					                  
	bl		EXT(sleh_abort)						                            
												                             
	b		load_and_go_user

ELSE_IF_KERNELMODE_EXCEPTION prefabt
	
UNWIND_PROLOGUE
	
	  
                                                                       
                          
                           
                           
                           
                          
   
                   
    
	sub     sp, sp, EXC_CTX_SIZE
	stmia	sp, {r0-r12}
	add		r0, sp, EXC_CTX_SIZE

	str		r0, [sp, SS_SP]						                          
	str		lr, [sp, SS_LR]                                               

	mrc		p15, 0, r9, c13, c0, 4				                

#if __ARM_VFP__
	add		r0, sp, SS_SIZE					                        
	bic		r0, #(VSS_ALIGN_NUM - 1)			                                        
	add		r0, VSS_ALIGN					                               
	bl		EXT(vfp_save)					                                           
	mov		r4, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r4					                          
#endif
#if __ARM_USER_PROTECT__
	mrc		p15, 0, r10, c2, c0, 0				            
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	cmp		r3, r10
	beq		1f
	mcr		p15, 0, r3, c2, c0, 0				            
1:
	mrc		p15, 0, r11, c13, c0, 1				                  
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif

	mrs		r4, lr_abt
	str		r4, [sp, SS_PC]					          

	mrc		p15, 0, r5, c6, c0, 2 				            
	str		r5, [sp, SS_VADDR]					                           
	mrc		p15, 0, r5, c5, c0, 1 				                                  
	str		r5, [sp, SS_STATUS]					                                    

	mrs		r4, spsr_abt
	str		r4, [sp, SS_CPSR]	

	mov		r0, sp
	ALIGN_STACK r1, r2
	mov		r1, T_PREFETCH_ABT					                  


UNWIND_DIRECTIVES	
	
	bl		EXT(sleh_abort) 					                            
	UNALIGN_STACK

	mrc		p15, 0, r9, c13, c0, 4				                
#if __ARM_USER_PROTECT__
	ldr		r0, [r9, ACT_KPTW_TTB]                                
	cmp		r10, r0
	beq		1f
	ldr		r10, [r9, ACT_UPTW_TTB]                               
	cmp		r10, r0
	beq		1f
	mcr		p15, 0, r10, c2, c0, 0                          
	ldr		r11, [r9, ACT_ASID]                                    
1:
	mcr		p15, 0, r11, c13, c0, 1                              
	isb
#endif

	b		load_and_go_sys

UNWIND_EPILOGUE

  
                                               
   
	.text
	.align 2
	.globl EXT(fleh_dataabt)
	
LEXT(fleh_dataabt)
VERIFY_EXCEPTION_MODE PSR_ABT_MODE
	sub		lr, lr, #8
IF_USERMODE_EXCEPTION dataabt
	mrc		p15, 0, sp, c13, c0, 4				                
	add		sp, sp, ACT_PCBDATA					               

	stmia	sp, {r0-r12, sp, lr}^				                           
	mov		r7, #0								                         
	nop
		
	mov		r0, sp								                                 
												               

	str		lr, [sp, SS_PC]						                             

	mrs		r4, spsr
	str		r4, [sp, SS_CPSR]					                      

	mrc		p15, 0, r5, c5, c0 					                    
	mrc		p15, 0, r6, c6, c0 					                     
	str		r5, [sp, SS_STATUS]					                                    
	str		r6, [sp, SS_VADDR]					                            

	cpsid	i, #PSR_SVC_MODE
	mrs		r3, cpsr 							            
	msr		spsr_cxsf, r3                                                 
	mrc		p15, 0, r9, c13, c0, 4				                
	ldr		sp, [r9, TH_KSTACKPTR]				                    

#if __ARM_VFP__
	add		r0, r9, ACT_UVFP				                                            
	bl		EXT(vfp_save)					                                         
	mov		r3, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r3					                          
#endif
#if __ARM_USER_PROTECT__
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r3, c2, c0, 0				            
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif

	mvn		r0, #0
	str		r0, [r9, TH_IOTIER_OVERRIDE]			                                                                    

#if	!CONFIG_SKIP_PRECISE_USER_KERNEL_TIME
	bl		EXT(timer_state_event_user_to_kernel)
	mrc		p15, 0, r9, c13, c0, 4				                
#endif

	add		r0, r9, ACT_PCBDATA					                                 
	mov     r1, T_DATA_ABT						                  
	bl		EXT(sleh_abort)						                            
												                       
	b		load_and_go_user

ELSE_IF_KERNELMODE_EXCEPTION dataabt

UNWIND_PROLOGUE	
	
	  
                                                                       
                          
                           
                           
                           
                          
   
                   
    
	sub     sp, sp, EXC_CTX_SIZE
	stmia	sp, {r0-r12}
	add		r0, sp, EXC_CTX_SIZE

	str		r0, [sp, SS_SP]						                          
	str		lr, [sp, SS_LR]                                               

	mrc		p15, 0, r9, c13, c0, 4				                

#if __ARM_VFP__
	add		r0, sp, SS_SIZE					                        
	bic		r0, #(VSS_ALIGN_NUM - 1)			                                        
	add		r0, VSS_ALIGN					                               
	bl		EXT(vfp_save)					                                           
	mov		r4, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r4					                          
#endif

	mrs		r4, lr_abt
	str		r4, [sp, SS_PC]
	mrs		r4, spsr_abt
	str		r4, [sp, SS_CPSR]	

#if __ARM_USER_PROTECT__
	mrc		p15, 0, r10, c2, c0, 0				            
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	cmp		r3, r10
	beq		1f
	mcr		p15, 0, r3, c2, c0, 0				            
1:
	mrc		p15, 0, r11, c13, c0, 1				                  
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif
	mrc		p15, 0, r5, c5, c0					                    
	mrc		p15, 0, r6, c6, c0					                     
	str		r5, [sp, SS_STATUS]					                                    
	str		r6, [sp, SS_VADDR]					                            

	mov		r0, sp								           
	ALIGN_STACK r1, r2
	mov		r1, T_DATA_ABT						                  

UNWIND_DIRECTIVES
	
	bl		EXT(sleh_abort)						                            
	UNALIGN_STACK

	mrc		p15, 0, r9, c13, c0, 4				                
#if __ARM_USER_PROTECT__
	ldr		r0, [r9, ACT_KPTW_TTB]                                
	cmp		r10, r0
	beq		1f
	ldr		r10, [r9, ACT_UPTW_TTB]                               
	cmp		r10, r0
	beq		1f
	mcr		p15, 0, r10, c2, c0, 0                          
	ldr		r11, [r9, ACT_ASID]                                    
1:
	mcr		p15, 0, r11, c13, c0, 1                              
	isb
#endif

load_and_go_sys:	
	mrc		p15, 0, r9, c13, c0, 4				                

	ldr		r4, [sp, SS_CPSR]					                  
	tst		r4, #PSR_IRQF						               
	bne		lags1								                         

	cpsid	i									              
	ldr		r2, [r9, ACT_PREEMPT_CNT]                                   
	movs	r2, r2								               
	ldr		r8, [r9, ACT_CPUDATAP]				                  
	bne		lags1								                           
	ldr		r5, [r8, CPU_PENDING_AST]			           
	ands	r5, r5, AST_URGENT					                               
	beq		lags1								                    
#if __ARM_USER_PROTECT__
	mrc		p15, 0, r10, c2, c0, 0				            
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	cmp		r3, r10
	beq		1f
	mcr		p15, 0, r3, c2, c0, 0				            
1:
	mrc		p15, 0, r11, c13, c0, 1				                  
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif
	ldr		lr, [sp, SS_LR]							                            
	stmfd		sp!, {r7, lr}							                    

	ALIGN_STACK r2, r3
	bl		EXT(ast_taken_kernel)				                    
	UNALIGN_STACK

	ldmfd		sp!, {r7, lr}							                     
	mrc		p15, 0, r9, c13, c0, 4				                          
	ldr		r8, [r9, ACT_CPUDATAP]				                  
#if __ARM_USER_PROTECT__
	ldr		r0, [r9, ACT_KPTW_TTB]                                
	cmp		r10, r0
	beq		1f
	ldr		r10, [r9, ACT_UPTW_TTB]                               
	cmp		r10, r0
	beq		1f
	mcr		p15, 0, r10, c2, c0, 0                          
	ldr		r11, [r9, ACT_ASID]                                    
1:
	mcr		p15, 0, r11, c13, c0, 1                              
	isb
#endif
lags1:
	ldr		lr, [sp, SS_LR]

	mov		ip, sp                                                                        
	ldr		sp, [ip, SS_SP]                                             

	cpsid	if, #PSR_ABT_MODE

	mov		sp, ip

	ldr		r4, [sp, SS_CPSR]
	msr		spsr_cxsf, r4						               

	clrex										                             
#if	__ARM_ENABLE_WFE_
	sev
#endif

#if __ARM_VFP__
	add		r0, sp, SS_SIZE					                        
	bic		r0, #(VSS_ALIGN_NUM - 1)			                                        
	add		r0, VSS_ALIGN					                               
	bl		EXT(vfp_load)					                                             
#endif

	ldr		lr, [sp, SS_PC]						             

	ldmia	sp, {r0-r12}						                          

	movs	pc, lr								                                
	
UNWIND_EPILOGUE
	
  
                                                      
                
   
	.text
	.align 2
	.globl EXT(fleh_addrexc)

LEXT(fleh_addrexc)	
	b	.
	

  
                                        
                     
                                                               
                                     
   
                                   
                                  
   

	.text
	.align 2
	.globl EXT(fleh_irq)

LEXT(fleh_irq)
	sub		lr, lr, #4
	
	cpsie	a									                         
	
	mrs		sp, spsr
	tst		sp, #0x0f							                        
	bne		fleh_irq_kernel

fleh_irq_user:
	mrc		p15, 0, sp, c13, c0, 4				                
	add		sp, sp, ACT_PCBDATA					               
	stmia	sp, {r0-r12, sp, lr}^
	mov		r7, #0								                         
	nop
	str		lr, [sp, SS_PC]
	mrs		r4, spsr
	str		r4, [sp, SS_CPSR]
	mov		r5, sp								                      
	mrc		p15, 0, r9, c13, c0, 4				                
	ldr		r6, [r9, ACT_CPUDATAP]				                  
	ldr		sp,	[r6, CPU_ISTACKPTR]				                      
	cpsid	i, #PSR_SVC_MODE
	ldr		sp, [r9, TH_KSTACKPTR]				                   
	cpsid	i, #PSR_IRQ_MODE

#if __ARM_VFP__
	add		r0, r9, ACT_UVFP				                                            
	bl		EXT(vfp_save)					                                         
	mov		r4, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r4					                          
#endif
#if __ARM_USER_PROTECT__
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r3, c2, c0, 0				            
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif
#if	!CONFIG_SKIP_PRECISE_USER_KERNEL_TIME
	bl		EXT(timer_state_event_user_to_kernel)
	mrc		p15, 0, r9, c13, c0, 4				                
#endif
#if CONFIG_TELEMETRY
	LOAD_ADDR(r2, telemetry_needs_record)		                                               
	mov		r0, #1
	ldr		r2, [r2]
	movs	r2, r2
	beq		1f
	mov		r1, #0					                     
	bl		EXT(telemetry_mark_curthread)		                                       
	mrc		p15, 0, r9, c13, c0, 4				                                                  
1:
#endif

	b		fleh_irq_handler

fleh_irq_kernel:
	cpsid	i, #PSR_SVC_MODE

	sub     sp, sp, EXC_CTX_SIZE
	stmia	sp, {r0-r12}
	add		r0, sp, EXC_CTX_SIZE

	str		r0, [sp, SS_SP]						                          
	str		lr, [sp, SS_LR]                                               

	mrc		p15, 0, r9, c13, c0, 4				                

#if __ARM_VFP__
	add		r0, sp, SS_SIZE					                        
	bic		r0, #(VSS_ALIGN_NUM - 1)			                                        
	add		r0, VSS_ALIGN					                               
	bl		EXT(vfp_save)					                                           
	mov		r4, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r4					                          
#endif
#if __ARM_USER_PROTECT__
	mrc		p15, 0, r10, c2, c0, 0				            
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r3, c2, c0, 0				            
	mrc		p15, 0, r11, c13, c0, 1				                 
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif
	mov		r5, sp								                      

	cpsid	i, #PSR_IRQ_MODE

	str		lr, [r5, SS_PC]                                                
	mrs		r4, spsr
	str		r4, [r5, SS_CPSR]                                                           

	ldr		sp, [r9, ACT_CPUDATAP]				                  
	ldr		sp,	[sp, CPU_ISTACKPTR]				                      

#if CONFIG_TELEMETRY
	LOAD_ADDR(r2, telemetry_needs_record)		                                               
	mov		r0, #0
	ldr		r2, [r2]
	movs	r2, r2
	beq		1f
	mov		r1, #0					                     
	bl		EXT(telemetry_mark_curthread)		                                       
	mrc		p15, 0, r9, c13, c0, 4				                                                  
1:
#endif

fleh_irq_handler:
	ldr		r2, [r9, ACT_PREEMPT_CNT]                                   
	add		r2, r2, #1							                  
	str		r2, [r9, ACT_PREEMPT_CNT]			                          
#ifndef	NO_KDEBUG
	LOAD_ADDR(r8, kdebug_enable)
	ldr		r8, [r8]
	movs	r8, r8
	movne	r0, r5
	COND_EXTERN_BLNE(interrupt_trace)
#endif
	bl	    EXT(interrupt_stats)                                              
	mrc		p15, 0, r9, c13, c0, 4				                          
	ldr		r4, [r9, ACT_CPUDATAP]				                  
	str		r5, [r4, CPU_INT_STATE] 			                                 
	ldr		r3, [r4, CPU_STAT_IRQ]				                
	add		r3, r3, #1					                  
	str		r3, [r4, CPU_STAT_IRQ]				                    
	ldr		r3, [r4, CPU_STAT_IRQ_WAKE]			                          
	add		r3, r3, #1					                  
	str		r3, [r4, CPU_STAT_IRQ_WAKE]			                             
	ldr		r0, [r4, INTERRUPT_TARGET]
	ldr		r1, [r4, INTERRUPT_REFCON]
	ldr		r2, [r4, INTERRUPT_NUB]
	ldr		r3, [r4, INTERRUPT_SOURCE]
	ldr		r5, [r4, INTERRUPT_HANDLER]			                                       
	blx		r5
#ifndef	NO_KDEBUG
	movs	r8, r8
	COND_EXTERN_BLNE(interrupt_trace_exit)
#endif
	bl		EXT(entropy_collect)
return_from_irq:
	mrc		p15, 0, r9, c13, c0, 4				                          
	mov		r5, #0
	ldr		r4, [r9, ACT_CPUDATAP]				                  
	str		r5, [r4, CPU_INT_STATE]				                      
	ldr		r2, [r9, ACT_PREEMPT_CNT]                                   
#if MACH_ASSERT
	cmp		r2, #0								                        
	bgt		1f
	push	{r7, lr}
	mov		r7, sp
	adr		r0, L_preemption_count_zero_str
	blx		EXT(panic)
	b		.
1:
#endif
	sub		r2, r2, #1							                  
	str		r2, [r9, ACT_PREEMPT_CNT]			                          

	mrs		r0, spsr							                              

	cpsid	i, #PSR_SVC_MODE

	tst		r0, #0x0f							                                     
	ldreq   sp, [r9, TH_KSTACKPTR]                                                          
	beq     load_and_go_user                                    

#if __ARM_USER_PROTECT__
	ldr		r0, [r9, ACT_KPTW_TTB]                                
	cmp		r10, r0
	beq		1f
	ldr		r10, [r9, ACT_UPTW_TTB]                               
	cmp		r10, r0
	beq		1f
	mcr		p15, 0, r10, c2, c0, 0                          
	ldr		r11, [r9, ACT_ASID]                                    
1:
	mcr		p15, 0, r11, c13, c0, 1                              
	isb
#endif
	b       load_and_go_sys

	.align 2
L_preemption_count_zero_str:
	.ascii	"locore.s: preemption count is zero \000"
	.align 2
  
                                        
                     
                                                               
                                     
   
                                   
                                  
   

	.text
	.align 2
	.globl EXT(fleh_decirq)

LEXT(fleh_decirq)
	sub		lr, lr, #4
	
	cpsie		af								                             
	
	mrs		sp, spsr
	tst		sp, #0x0f							                        
	bne		fleh_decirq_kernel

fleh_decirq_user:
	mrc		p15, 0, sp, c13, c0, 4				                
	add		sp, sp, ACT_PCBDATA					               
	stmia	sp, {r0-r12, sp, lr}^
	mov		r7, #0								                         
	nop
	str		lr, [sp, SS_PC]
	mrs		r4, spsr
	str		r4, [sp, SS_CPSR]
	mov		r5, sp								                      
	mrc		p15, 0, r9, c13, c0, 4				                
	ldr		r6, [r9, ACT_CPUDATAP]				                  
	ldr		sp,	[r6, CPU_ISTACKPTR]				                      
	cpsid	i, #PSR_SVC_MODE
	ldr		sp, [r9, TH_KSTACKPTR]				                   
	cpsid	i, #PSR_IRQ_MODE

#if __ARM_VFP__
	add		r0, r9, ACT_UVFP				                                            
	bl		EXT(vfp_save)					                                         
	mov		r4, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r4					                          
#endif
#if __ARM_USER_PROTECT__
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r3, c2, c0, 0				            
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif
#if	!CONFIG_SKIP_PRECISE_USER_KERNEL_TIME
	bl		EXT(timer_state_event_user_to_kernel)
	mrc		p15, 0, r9, c13, c0, 4				                
#endif
#if CONFIG_TELEMETRY
	LOAD_ADDR(r2, telemetry_needs_record)		                                               
	mov		r0, #1
	ldr		r2, [r2]
	movs	r2, r2
	beq		1f
	mov 		r1, #0					                     
	bl		EXT(telemetry_mark_curthread)		                                       
	mrc		p15, 0, r9, c13, c0, 4				                                                  
1:
#endif

	b		fleh_decirq_handler

fleh_decirq_kernel:
	cpsid	i, #PSR_SVC_MODE

	sub     sp, sp, EXC_CTX_SIZE
	stmia	sp, {r0-r12}
	add		r0, sp, EXC_CTX_SIZE

	str		r0, [sp, SS_SP]						                          
	str		lr, [sp, SS_LR]                                               

	mrc		p15, 0, r9, c13, c0, 4				                

#if __ARM_VFP__
	add		r0, sp, SS_SIZE					                        
	bic		r0, #(VSS_ALIGN_NUM - 1)			                                        
	add		r0, VSS_ALIGN					                               
	bl		EXT(vfp_save)					                                           
	mov		r4, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r4					                          
#endif
#if __ARM_USER_PROTECT__
	mrc		p15, 0, r10, c2, c0, 0				            
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r3, c2, c0, 0				            
	mrc		p15, 0, r11, c13, c0, 1				                 
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif
	mov		r5, sp								                      

	cpsid	i, #PSR_IRQ_MODE

	str		lr, [r5, SS_PC]                                                
	mrs		r4, spsr
	str		r4, [r5, SS_CPSR]                                                           

	ldr		sp, [r9, ACT_CPUDATAP]				                  
	ldr		sp,	[sp, CPU_ISTACKPTR]				                      

#if CONFIG_TELEMETRY
	LOAD_ADDR(r2, telemetry_needs_record)		                                               
	mov		r0, #0
	ldr		r2, [r2]
	movs	r2, r2
	beq		1f
	mov		r1, #0					                     
	bl		EXT(telemetry_mark_curthread)		                                       
	mrc		p15, 0, r9, c13, c0, 4				                                                  
1:
#endif

fleh_decirq_handler:
	ldr		r2, [r9, ACT_PREEMPT_CNT]                                   
	add		r2, r2, #1							                  
	str		r2, [r9, ACT_PREEMPT_CNT]			                          
	ldr		r2, [r9, ACT_CPUDATAP]				                  
	str		r5, [r2, CPU_INT_STATE]				                                 
	ldr		r3, [r2, CPU_STAT_IRQ]				                
	add		r3, r3, #1							                  
	str		r3, [r2, CPU_STAT_IRQ]				                   
	ldr		r3, [r2, CPU_STAT_IRQ_WAKE]			                          
	add		r3, r3, #1					                  
	str		r3, [r2, CPU_STAT_IRQ_WAKE]			                             
#ifndef NO_KDEBUG
	LOAD_ADDR(r4, kdebug_enable)
	ldr		r4, [r4]
	movs	r4, r4
	movne	r0, r5								                     
	COND_EXTERN_BLNE(interrupt_trace)
#endif
	bl		EXT(interrupt_stats)                                              
	mov		r0, #0
	bl		EXT(rtclock_intr)					                                      
#ifndef NO_KDEBUG
	movs	r4, r4
	COND_EXTERN_BLNE(interrupt_trace_exit)
#endif

	b		return_from_irq


  
                                        
                     
                                                               
                                     
   
                                   
                                  
  
                                                                  
                                                              
                                                                               
                                                       
   
	.text
	.align 2
	.globl EXT(fleh_fiq_generic)
	
LEXT(fleh_fiq_generic)
	str		r11, [r10]							                       

	ldr		r13, [r8, CPU_TIMEBASE_LOW]			           
	adds	r13, r13, #1						                
	str		r13, [r8, CPU_TIMEBASE_LOW]			            
	ldreq	r13, [r8, CPU_TIMEBASE_HIGH]		           
	addeq	r13, r13, #1						                
	streq	r13, [r8, CPU_TIMEBASE_HIGH]		            
	subs	r12, r12, #1						                 
	str		r12, [r8, CPU_DECREMENTER]			            
	subspl	pc, lr, #4							                        
	b		EXT(fleh_dec)

	.text
	.align	2
	.globl	EXT(fleh_dec)
LEXT(fleh_dec)
	mrs		sp, spsr							               
	sub		lr, lr, #4
	tst		sp, #0x0f							                        
	bne		2f

	               
	mrc		p15, 0, sp, c13, c0, 4				                
	add		sp, sp, ACT_PCBDATA					               
	
	stmia	sp, {r0-r12, sp, lr}^
	mov		r7, #0								                         
	nop
	str		lr, [sp, SS_PC]
	
	mrs		r4, spsr
	str		r4, [sp, SS_CPSR]
	mov		r5, sp
	sub		sp, sp, ACT_PCBDATA					               
	ldr		sp, [sp, ACT_CPUDATAP]				                  
	ldr		sp,	[sp, CPU_ISTACKPTR]				                      
	mov		r6, sp
	cpsid	i, #PSR_SVC_MODE
	mrc		p15, 0, r9, c13, c0, 4				                
	ldr		sp, [r9, TH_KSTACKPTR]				                   

#if __ARM_VFP__
	add		r0, r9, ACT_UVFP				                                            
	bl		EXT(vfp_save)					                                         
	mov		r4, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r4					                          
#endif
#if __ARM_USER_PROTECT__
	mrc		p15, 0, r10, c2, c0, 0				            
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r3, c2, c0, 0				            
	mrc		p15, 0, r11, c13, c0, 1				                 
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif
	mov		r0, #1								                                        
	b		4f

2:
	                 
	tst		sp, #PSR_IRQF						                      
	bne		3f									                               

	cpsid   if, #PSR_SVC_MODE

	sub     sp, sp, EXC_CTX_SIZE
	stmia	sp, {r0-r12}
	add		r0, sp, EXC_CTX_SIZE

	str		r0, [sp, SS_SP]						                          
	str		lr, [sp, SS_LR]                                               

	mrc		p15, 0, r9, c13, c0, 4				                

#if __ARM_VFP__
	add		r0, sp, SS_SIZE					                        
	bic		r0, #(VSS_ALIGN_NUM - 1)			                                        
	add		r0, VSS_ALIGN					                               
	bl		EXT(vfp_save)					                                           
	mov		r4, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r4					                          
#endif
#if __ARM_USER_PROTECT__
	mrc		p15, 0, r10, c2, c0, 0				            
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r3, c2, c0, 0				            
	mrc		p15, 0, r11, c13, c0, 1				                 
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif
	mov		r5, sp								                      

	cpsid   if, #PSR_FIQ_MODE

	mrc     p15, 0, r1, c13, c0, 4                              

	str		lr, [r5, SS_PC]                                                
	mrs		r4, spsr
	str		r4, [r5, SS_CPSR]                                                           

	ldr		r6, [r1, ACT_CPUDATAP]				                  
	ldr		r6,	[r6, CPU_ISTACKPTR]				                      

	mov		r0, #0								                                          
	b       4f

3:
	                     
	mrc		p15, 0, sp, c13, c0, 4				                
	ldr		sp, [sp, ACT_CPUDATAP]				                  
	ldr		sp,	[sp, CPU_FIQSTACKPTR]			                
	sub		sp, sp, EXC_CTX_SIZE
	stmia		sp, {r0-r12}
	str		lr, [sp, SS_PC]
	mrs		r4, spsr
	str		r4, [sp, SS_CPSR]
	mrc		p15, 0, r9, c13, c0, 4				                

#if __ARM_VFP__
	add		r0, sp, SS_SIZE					                        
	bic		r0, #(VSS_ALIGN_NUM - 1)			                                        
	add		r0, VSS_ALIGN					                               
	bl		EXT(vfp_save)					                                           
	mov		r4, #FPSCR_DEFAULT				                                     
	fmxr		fpscr, r4					                          
#endif
#if __ARM_USER_PROTECT__
	mrc		p15, 0, r10, c2, c0, 0				            
	ldr		r3, [r9, ACT_KPTW_TTB]				                  
	mcr		p15, 0, r3, c2, c0, 0				            
	mrc		p15, 0, r11, c13, c0, 1				                 
	mov		r3, #0								                   
	mcr		p15, 0, r3, c13, c0, 1				                 
	isb
#endif

	ALIGN_STACK r0, r1
	mov		r0, r8								                           
	mov		r1, SIGPdec							                             
	mov		r2, #0
	mov		r3, #0
	bl		EXT(cpu_signal)						                  
	UNALIGN_STACK

	mrc		p15, 0, r9, c13, c0, 4				                

#if __ARM_VFP__
	add		r0, sp, SS_SIZE					                        
	bic		r0, #(VSS_ALIGN_NUM - 1)			                                        
	add		r0, VSS_ALIGN					                               
	bl		EXT(vfp_load)					                                             
#endif

	clrex										                             
#if	__ARM_ENABLE_WFE_
	sev
#endif
#if __ARM_USER_PROTECT__
	mcr		p15, 0, r10, c2, c0, 0				            
	mcr		p15, 0, r11, c13, c0, 1				                 
	isb
#endif
	ldr		lr, [sp, SS_PC]
	ldmia	sp, {r0-r12}						                          
	movs	pc, lr								                  

4:
	cpsid	i, #PSR_IRQ_MODE
	cpsie	f
	mov		sp, r6								                            
	ALIGN_STACK r2, r3
	msr		spsr_cxsf, r4						                   
	ldr		r2, [r9, ACT_PREEMPT_CNT]                                   
	add		r2, r2, #1							                  
	str		r2, [r9, ACT_PREEMPT_CNT]			                          
	ldr		r4, [r9, ACT_CPUDATAP]				                  
	str		r5, [r4, CPU_INT_STATE] 
	ldr		r3, [r4, CPU_STAT_IRQ]				                
	add		r3, r3, #1							                  
	str		r3, [r4, CPU_STAT_IRQ]				                   
	ldr		r3, [r4, CPU_STAT_IRQ_WAKE]			                          
	add		r3, r3, #1					                  
	str		r3, [r4, CPU_STAT_IRQ_WAKE]			                             
#if	!CONFIG_SKIP_PRECISE_USER_KERNEL_TIME
	movs	r0, r0
	beq		5f
	mov	r8, r0							                                      
	bl		EXT(timer_state_event_user_to_kernel)
	mov	r0, r8							                                
	mrc		p15, 0, r9, c13, c0, 4				                
5:
#endif
#if CONFIG_TELEMETRY
	LOAD_ADDR(r4, telemetry_needs_record)		                                               
	ldr		r4, [r4]
	movs		r4, r4
	beq		6f
	mov		r1, #0					                     
	bl		EXT(telemetry_mark_curthread)		                                       
	mrc		p15, 0, r9, c13, c0, 4				                                                  
6:
#endif

#ifndef NO_KDEBUG
	LOAD_ADDR(r4, kdebug_enable)
	ldr     r4, [r4]
	movs    r4, r4
	ldrne	r1, [r9, ACT_CPUDATAP]				                  
	ldrne	r0, [r1, CPU_INT_STATE]
	COND_EXTERN_BLNE(interrupt_trace)
#endif
	bl		EXT(interrupt_stats)                                              
	mov		r0, #0
	bl		EXT(rtclock_intr)					                                      
#ifndef NO_KDEBUG
	movs	r4, r4
	COND_EXTERN_BLNE(interrupt_trace_exit)
#endif
	UNALIGN_STACK

	b       return_from_irq

  
                                               
  
   
	.text
	.align 2
	.globl EXT(thread_syscall_return)

LEXT(thread_syscall_return)
	mrc		p15, 0, r9, c13, c0, 4				                
	add		r1, r9, ACT_PCBDATA					               
	str		r0, [r1, SS_R0]						                   
#ifndef	NO_KDEBUG
	LOAD_ADDR(r4, kdebug_enable)
	ldr		r4, [r4]
	movs	r4, r4
	beq		load_and_go_user
	ldr		r12, [r1, SS_R12]					                      
	rsbs	r1, r12, #0							                                          
	COND_EXTERN_BLGT(mach_syscall_trace_exit)
#endif
	b		load_and_go_user

  
                                     
                                     
  
   
	.text
	.globl EXT(thread_exception_return)
	.globl EXT(thread_bootstrap_return)

LEXT(thread_bootstrap_return)
#if CONFIG_DTRACE
	bl EXT(dtrace_thread_bootstrap)
#endif
	                

LEXT(thread_exception_return)

load_and_go_user:	
  
                                                    
   
	cpsid	i									              
	mrc		p15, 0, r9, c13, c0, 4				                

	mvn		r0, #0
	str		r0, [r9, TH_IOTIER_OVERRIDE]			                                                        
	
	ldr		r8, [r9, ACT_CPUDATAP]				                  
	ldr		r5, [r8, CPU_PENDING_AST]			           
	cmp		r5, #0								                       
	beq		return_to_user_now					                    

	bl		EXT(ast_taken_user)					                                                             

	mrc		p15, 0, r9, c13, c0, 4				                          
	b	load_and_go_user						            

return_to_user_now:	

#if MACH_ASSERT
  
                                                                             
   
	ldr		r1, [r9, ACT_PREEMPT_CNT]			                        
	cmp		r1, #0						       
	bne		L_lagu_preempt_panic				                    

  
                                                                             
   
	ldr		r2, [r9, TH_RWLOCK_CNT]				                     
	cmp		r2, #0						       
	bne		L_lagu_rwlock_cnt_panic				                    
#endif

  
                                                                                         
   
	ldr		r1, [r9, TH_TMP_ALLOC_CNT]			                        
	cmp		r1, #0						       
	bne		L_lagu_temp_alloc_cnt_panic			                    

#if	!CONFIG_SKIP_PRECISE_USER_KERNEL_TIME
	bl		EXT(timer_state_event_kernel_to_user)
	mrc		p15, 0, r9, c13, c0, 4				                
	ldr		r8, [r9, ACT_CPUDATAP]				                       
#endif	                                           
#if __ARM_DEBUG__ >= 6
	ldr		r0, [r9, ACT_DEBUGDATA]
	ldr		r6, [r8, CPU_USER_DEBUG]
	cmp		r0, r6								                                             
	beq		1f
	bl		EXT(arm_debug_set)					                            
	mrc		p15, 0, r9, c13, c0, 4				                
1:
#endif
#if __ARM_VFP__
	add		r0, r9, ACT_UVFP				                                            
	bl		EXT(vfp_load)					                                           
#endif
	add		r0, r9, ACT_PCBDATA					               
	ldr		r4, [r0, SS_CPSR]					                 
	and		r3, r4, #PSR_MODE_MASK				                        
	cmp		r3, #PSR_USER_MODE					                  
	movne	r0, r3
	bne		EXT(ExceptionVectorPanic)

	msr		spsr_cxsf, r4						                               
	mov		sp, r0								               

	clrex										                             
#if	__ARM_ENABLE_WFE_
	sev
#endif
#if __ARM_USER_PROTECT__
	ldr     r3, [r9, ACT_UPTW_TTB]                                
	mcr		p15, 0, r3, c2, c0, 0				            
	ldr		r2, [r9, ACT_ASID]					                   
	mcr		p15, 0, r2, c13, c0, 1
	isb
#endif
	ldr		lr, [sp, SS_PC]						                       
	ldmia	sp, {r0-r12, sp, lr}^				                                        
	nop											                   
	movs	pc, lr								                 

  
                      
                       
   
L_lagu_temp_alloc_cnt_panic:
	mov		r0, r9						                  
	blx		EXT(kheap_temp_leak_panic)			                 

#if MACH_ASSERT
  
                               
                       
   
L_lagu_preempt_panic:
	adr		r0, L_lagu_preempt_panic_str			                           
	blx		EXT(panic)					                 

  
                   
                       
   
L_lagu_rwlock_cnt_panic:
	adr		r0, L_lagu_rwlock_cnt_panic_str			                           
	mov		r1, r9						                                   
	blx		EXT(panic)					                 

	.align  2
L_lagu_preempt_panic_str:
	.asciz  "load_and_go_user: preemption_level %d"
	.align  2

	.align  2
L_lagu_rwlock_cnt_panic_str:
	.asciz  "load_and_go_user: RW lock count not 0 on thread %p (%u)"
	.align  2
#endif                  

	.align  2
L_evimpanic_str:
	.ascii  "Exception Vector: Illegal Mode: 0x%08X\n\000"
	.align  2

	.text
	.align 2
	.globl EXT(ExceptionVectorPanic)

LEXT(ExceptionVectorPanic)
	cpsid i, #PSR_SVC_MODE
	ALIGN_STACK r1, r2
	mov		r1, r0
	adr		r0, L_evimpanic_str
	blx		EXT(panic)
	b		.

#include	"globals_asm.h"

LOAD_ADDR_GEN_DEF(mach_trap_table)
LOAD_ADDR_GEN_DEF(kern_invalid)

                    
