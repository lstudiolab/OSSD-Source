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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <machine/asm.h>
#include <arm/proc_reg.h>
#include <arm/pmap.h>
#include <sys/errno.h>
#include "assym.s"

	.align	2
	.globl	EXT(machine_set_current_thread)
LEXT(machine_set_current_thread)
	ldr		r1, [r0, ACT_CPUDATAP]
	str		r0, [r1, CPU_ACTIVE_THREAD]
	mcr		p15, 0, r0, c13, c0, 4				                 
	ldr		r1, [r0, TH_CTH_SELF]
	mrc		p15, 0, r2, c13, c0, 3				                
	and		r2, r2, #3							                     
	orr		r1, r1, r2							  
	mcr		p15, 0, r1, c13, c0, 3				                 
	mov		r1, #0
	mcr		p15, 0, r1, c13, c0, 2				                 
	bx		lr

  
                           
   
	.text
	.align 2
	.globl EXT(machine_idle)
LEXT(machine_idle)
	cpsid	if									                  
	mov		ip, lr
	bl		EXT(Idle_context)
	mov		lr, ip
	cpsie	if									                 
	bx		lr

  
                                         
                                                        
   
	.text
	.align 2
	.globl EXT(cpu_idle_wfi)
LEXT(cpu_idle_wfi)
	mov		r1, #32
	mov		r2, #1200
	cmp		r0, #0
	beq		3f
	mov		r1, #1
	b		2f
	.align 5
1:
	add		r0, r0, #1
	mov		r1, r2
2:

  
                                                                                       
                                       
   

#if	(__ARM_ARCH__ >= 7)
	dsb
	.globl EXT(wfi_inst)
LEXT(wfi_inst)
	wfi
#else
	mcr		p15, 0, r0, c7, c10, 4
	.globl EXT(wfi_inst)
LEXT(wfi_inst)
	mcr		p15, 0, r0, c7, c0, 4
#endif
3:
	subs		r1, r1, #1
	bne		3b
	nop
	nop
	nop
	nop
	nop
	cmp		r0, #0
	beq		1b
	bx lr

	.align	2
	.globl	EXT(timer_grab)
LEXT(timer_grab)
0:
	ldr		r2, [r0, TIMER_HIGH]
	ldr		r3, [r0, TIMER_LOW]
	dmb		ish									          
	ldr		r1, [r0, TIMER_HIGHCHK]
	cmp		r1, r2
	bne		0b
	mov		r0, r3
	bx		lr

	.align	2
	.globl	EXT(timer_advance_internal_32)
LEXT(timer_advance_internal_32)
	str		r1, [r0, TIMER_HIGHCHK]
	dmb		ish									          
	str		r2, [r0, TIMER_LOW]
	dmb		ish									          
	str		r1, [r0, TIMER_HIGH]
	bx		lr

	.align	2
	.globl	EXT(get_vfp_enabled)
LEXT(get_vfp_enabled)
#if	__ARM_VFP__
	fmrx	r0, fpexc
	and		r1, r0, #FPEXC_EN					                                   
	mov		r0, r1, LSR #FPEXC_EN_BIT			                                     
#else
	mov		r0, #0								               
#endif
	bx		lr

                                                                                   
	.align	2
	.globl	EXT(enable_kernel_vfp_context)
LEXT(enable_kernel_vfp_context)
	bx              lr

                            
                                                    
   
	.align	2
	.globl	EXT(get_fpscr)
LEXT(get_fpscr)
#if	__ARM_VFP__
	fmrx	r0, fpscr
#endif
	bx	lr
	.align	2
	.globl	EXT(set_fpscr)
                                  
                           
   
LEXT(set_fpscr)
#if	__ARM_VFP__
	fmxr	fpscr, r0
#else
	mov	r0, #0
#endif
	bx	lr

  
                             
   
	.text
	.align 2
        .globl EXT(OSSynchronizeIO)
LEXT(OSSynchronizeIO)
	.align          2
	dsb
	bx		lr

.macro SYNC_TLB_FLUSH
	dsb	ish
	isb
.endmacro

  
                      
  
                                                      
   
	.text
	.align 2
	.globl EXT(sync_tlb_flush)
LEXT(sync_tlb_flush)
	SYNC_TLB_FLUSH
	bx	lr

.macro FLUSH_MMU_TLB
	mov     r0, #0
	mcr     p15, 0, r0, c8, c3, 0				                                         
.endmacro

  
                                 
  
                                             
   
	.text
	.align 2
	.globl EXT(flush_mmu_tlb_async)
LEXT(flush_mmu_tlb_async)
	FLUSH_MMU_TLB
	bx	lr

  
                           
  
                  
   
	.text
	.align 2
	.globl EXT(flush_mmu_tlb)
LEXT(flush_mmu_tlb)
	FLUSH_MMU_TLB
	SYNC_TLB_FLUSH
	bx	lr

.macro FLUSH_CORE_TLB
	mov     r0, #0
	mcr     p15, 0, r0, c8, c7, 0				                        
.endmacro

  
  
                                  
  
                                                     
   
	.text
	.align 2
	.globl EXT(flush_core_tlb_async)
LEXT(flush_core_tlb_async)
	FLUSH_CORE_TLB
	bx	lr

  
                            
  
                          
   
	.text
	.align 2
	.globl EXT(flush_core_tlb)
LEXT(flush_core_tlb)
	FLUSH_CORE_TLB
	SYNC_TLB_FLUSH
	bx	lr

.macro FLUSH_MMU_TLB_ENTRY
	mcr     p15, 0, r0, c8, c3, 1				                                       
.endmacro
  
                                           
  
                                              
   
	.text
	.align 2
	.globl EXT(flush_mmu_tlb_entry_async)
LEXT(flush_mmu_tlb_entry_async)
	FLUSH_MMU_TLB_ENTRY
	bx	lr

  
                                     
  
                   
   
	.text
	.align 2
	.globl EXT(flush_mmu_tlb_entry)
LEXT(flush_mmu_tlb_entry)
	FLUSH_MMU_TLB_ENTRY
	SYNC_TLB_FLUSH
	bx	lr

.macro FLUSH_MMU_TLB_ENTRIES
1:
	mcr     p15, 0, r0, c8, c3, 1				                                        
	add	r0, r0, ARM_PGBYTES				                             
	cmp	r0, r1						                                        
	blt	1b
.endmacro

  
                                                       
  
                                                                  
   
	.text
	.align 2
	.globl EXT(flush_mmu_tlb_entries_async)
LEXT(flush_mmu_tlb_entries_async)
	FLUSH_MMU_TLB_ENTRIES
	bx	lr

  
                                                 
  
                                       
   
	.text
	.align 2
	.globl EXT(flush_mmu_tlb_entries)
LEXT(flush_mmu_tlb_entries)
	FLUSH_MMU_TLB_ENTRIES
	SYNC_TLB_FLUSH
	bx	lr


.macro FLUSH_MMU_TLB_MVA_ENTRIES
	mcr     p15, 0, r0, c8, c3, 3				                                                
.endmacro

  
                                                 
  
                                                        
   
	.text
	.align 2
	.globl EXT(flush_mmu_tlb_mva_entries_async)
LEXT(flush_mmu_tlb_mva_entries_async)
	FLUSH_MMU_TLB_MVA_ENTRIES
	bx	lr

  
                                                 
  
                             
   
	.text
	.align 2
	.globl EXT(flush_mmu_tlb_mva_entries)
LEXT(flush_mmu_tlb_mva_entries)
	FLUSH_MMU_TLB_MVA_ENTRIES
	SYNC_TLB_FLUSH
	bx	lr

.macro FLUSH_MMU_TLB_ASID
	mcr     p15, 0, r0, c8, c3, 2				                                                 
.endmacro

  
                                          
  
                                                         
   
	.text
	.align 2
	.globl EXT(flush_mmu_tlb_asid_async)
LEXT(flush_mmu_tlb_asid_async)
	FLUSH_MMU_TLB_ASID
	bx	lr

  
                                    
  
                              
   
	.text
	.align 2
	.globl EXT(flush_mmu_tlb_asid)
LEXT(flush_mmu_tlb_asid)
	FLUSH_MMU_TLB_ASID
	SYNC_TLB_FLUSH
	bx	lr

.macro FLUSH_CORE_TLB_ASID
	mcr     p15, 0, r0, c8, c7, 2				                                 
.endmacro

  
                                           
  
                                                                    
   
	.text
	.align 2
	.globl EXT(flush_core_tlb_asid_async)
LEXT(flush_core_tlb_asid_async)
	FLUSH_CORE_TLB_ASID
	bx	lr

  
                                     
  
                                         
   
	.text
	.align 2
	.globl EXT(flush_core_tlb_asid)
LEXT(flush_core_tlb_asid)
	FLUSH_CORE_TLB_ASID
	SYNC_TLB_FLUSH
	bx	lr

  
                                  
   
	.text
	.align 2
	.globl EXT(set_mmu_ttb)
LEXT(set_mmu_ttb)
	orr		r0, r0, #(TTBR_SETUP & 0xFF)		                              
	orr		r0, r0, #(TTBR_SETUP & 0xFF00)		                              
	mcr		p15, 0, r0, c2, c0, 0				                                  
	dsb		ish
	isb
	bx		lr

  
                                            
   
	.text
	.align 2
	.globl EXT(set_mmu_ttb_alternate)
LEXT(set_mmu_ttb_alternate)
	orr		r0, r0, #(TTBR_SETUP & 0xFF)		                              
	orr		r0, r0, #(TTBR_SETUP & 0xFF00)		                              
	mcr		p15, 0, r0, c2, c0, 1				                                  
	dsb		ish
	isb
	bx		lr

  
                                  
   
	.text
	.align 2
	.globl EXT(get_mmu_ttb)
LEXT(get_mmu_ttb)
	mrc		p15, 0, r0, c2, c0, 0				                          
	bx		lr

  
                            
   
	.text
	.align 2
	.globl EXT(get_aux_control)
LEXT(get_aux_control)
	mrc		p15, 0, r0, c1, c0, 1				                           
	bx		lr									                        

  
                            
   
	.text
	.align 2
	.globl EXT(set_aux_control)
LEXT(set_aux_control)
	mcr		p15, 0, r0, c1, c0, 1				                               
	isb
	bx		lr


  
                            
   
	.text
	.align 2
	.globl EXT(get_mmu_control)
LEXT(get_mmu_control)
	mrc		p15, 0, r0, c1, c0, 0				                           
	bx		lr									                        

  
                            
   
	.text
	.align 2
	.globl EXT(set_mmu_control)
LEXT(set_mmu_control)
	mcr		p15, 0, r0, c1, c0, 0				                               
	isb
	bx		lr

  
                                                     
   
	.text
	.align 2
	.globl EXT(mmu_kvtop)
LEXT(mmu_kvtop)
	mrs		r3, cpsr							            
	cpsid	if									                  
	mov		r1, r0
	mcr		p15, 0, r1, c7, c8, 0				                
	isb
	mrc		p15, 0, r0, c7, c4, 0				           
	ands	r2, r0, #0x1						                          
	bne		mmu_kvtophys_fail
	ands	r2, r0, #0x2						                     
	mvnne	r2, #0xFF000000
	moveq	r2, #0x000000FF
	orreq	r2, r2, #0x00000F00
	bics	r0, r0, r2							                   
	beq		mmu_kvtophys_fail
	and		r1, r1, r2
	orr		r0, r0, r1
	b		mmu_kvtophys_ret
mmu_kvtophys_fail:
	mov		r0, #0
mmu_kvtophys_ret:
	msr		cpsr, r3							               
	bx		lr

  
                                                   
   
	.text
	.align 2
	.globl EXT(mmu_uvtop)
LEXT(mmu_uvtop)
	mrs		r3, cpsr							            
	cpsid	if									                  
	mov		r1, r0
	mcr		p15, 0, r1, c7, c8, 2				                
	isb
	mrc		p15, 0, r0, c7, c4, 0				           
	ands	r2, r0, #0x1						                          
	bne		mmu_uvtophys_fail
	ands	r2, r0, #0x2						                     
	mvnne	r2, #0xFF000000
	moveq	r2, #0x000000FF
	orreq	r2, r2, #0x00000F00
	bics	r0, r0, r2							                   
	beq		mmu_uvtophys_fail
	and		r1, r1, r2
	orr		r0, r0, r1
	b		mmu_uvtophys_ret
mmu_uvtophys_fail:
	mov		r0, #0
mmu_uvtophys_ret:
	msr		cpsr, r3							               
	bx		lr

  
                                                                
   
	.text
	.align 2
	.globl EXT(mmu_kvtop_wpreflight)
LEXT(mmu_kvtop_wpreflight)
	mrs		r3, cpsr							            
	cpsid	if									                  
	mov		r1, r0
	mcr		p15, 0, r1, c7, c8, 1				                
	isb
	mrc		p15, 0, r0, c7, c4, 0				           
	ands	r2, r0, #0x1						                          
	bne		mmu_kvtophys_wpreflight_fail
	ands	r2, r0, #0x2						                     
	mvnne	r2, #0xFF000000
	moveq	r2, #0x000000FF
	orreq	r2, r2, #0x00000F00
	bics	r0, r0, r2							                   
	beq		mmu_kvtophys_wpreflight_fail		                                                             
	and		r1, r1, r2
	orr		r0, r0, r1
	b		mmu_kvtophys_wpreflight_ret
mmu_kvtophys_wpreflight_fail:
	mov		r0, #0
mmu_kvtophys_wpreflight_ret:
	msr		cpsr, r3							               
	bx		lr

  
                           
   
  
                           
   
	.text
	.align 2
	.globl EXT(set_context_id)
LEXT(set_context_id)
	mcr		p15, 0, r0, c13, c0, 1
	isb
	bx		lr

  
                                                                      
                                                                          
   
.macro COPYIO_VALIDATE
	                                               
	push		{r0-r$1, r7, lr}
	add		r7, sp, #(($1 + 1) * 4)
	blx		EXT($0_validate)
	cmp		r0, #0
	addne           sp, #(($1 + 1) * 4)
	popne		{r7, pc}
	pop		{r0-r$1, r7, lr}
.endmacro


#define	COPYIO_SET_RECOVER()						\
	                          					;\
	stmfd		sp!, { r4, r5, r6 }				;\
	adr		r3, copyio_error				;\
	mrc		p15, 0, r12, c13, c0, 4				;\
	ldr		r4, [r12, TH_RECOVER]				;\
	str		r3, [r12, TH_RECOVER]

#define COPYIO_TRY_KERNEL()								\
	                                                                     		;\
	mrc		p15, 0, r12, c13, c0, 4			                   
	ldr		r3, [r12, ACT_MAP]						;\
	ldr		r3, [r3, MAP_PMAP]						;\
	LOAD_ADDR(ip, kernel_pmap_store)						;\
	cmp		r3, ip								;\
	beq		copyio_kern_body

#if __ARM_USER_PROTECT__
#define	COPYIO_MAP_USER()					\
	                                                          
                                                   	;\
	mrs		r5, cpsr				;\
	cpsid		if					;\
	ldr		r3, [r12, ACT_UPTW_TTB]			;\
	mcr		p15, 0, r3, c2, c0, 0			;\
	msr		cpsr, r5				;\
	ldr		r3, [r12, ACT_ASID]			;\
	mcr		p15, 0, r3, c13, c0, 1			;\
	isb
#else
#define	COPYIO_MAP_USER()
#endif

#define COPYIO_HEADER()							;\
	                       						;\
	cmp		r2, #0						;\
	moveq		r0, #0						;\
	bxeq		lr
	
.macro COPYIO_BODY
	                                                         
	cmp			r2, #16
	blt			L$0_bytewise
	                                                      
	orr			r3, r0, r1
	tst			r3, #3
	bne			L$0_bytewise
L$0_wordwise:
	sub			r2, r2, #16
L$0_wordwise_loop:
	                        
	ldmia		r0!, { r3, r5, r6, r12 }
	stmia		r1!, { r3, r5, r6, r12 }
	subs		r2, r2, #16
	bge			L$0_wordwise_loop
	                                           
	adds		r2, r2, #16
	beq			L$0_noerror
L$0_bytewise:
	                            
	subs		r2, r2, #2
	ldrb		r3, [r0], #1
	ldrbpl		r12, [r0], #1
	strb		r3, [r1], #1
	strbpl		r12, [r1], #1
	bhi			L$0_bytewise
L$0_noerror:
	mov			r0, #0
.endmacro

#if __ARM_USER_PROTECT__
#define	COPYIO_UNMAP_USER()					\
	mrc		p15, 0, r12, c13, c0, 4				;\
	ldr		r3, [r12, ACT_KPTW_TTB]				;\
	mcr		p15, 0, r3, c2, c0, 0				;\
	mov		r3, #0						;\
	mcr		p15, 0, r3, c13, c0, 1				;\
	isb
#else
#define	COPYIO_UNMAP_USER()					\
	mrc		p15, 0, r12, c13, c0, 4
#endif

#define	COPYIO_RESTORE_RECOVER()					\
	                                  			;\
	str		r4, [r12, TH_RECOVER]			;\
	ldmfd		sp!, { r4, r5, r6 }

  
                 
                                 
                       
                   
                       
   
	.text
	.align 2
	.globl EXT(copyinstr)
LEXT(copyinstr)
	cmp		r2, #0
	moveq		r0, #ENAMETOOLONG
	moveq		r12, #0
	streq		r12, [r3]
	bxeq		lr
	COPYIO_VALIDATE copyin_user, 3
	stmfd	sp!, { r4, r5, r6 }
	
	mov		r6, r3
	adr     	r3, copyinstr_error			                          
	mrc		p15, 0, r12, c13, c0, 4			                
	ldr		r4, [r12, TH_RECOVER]				;\
	str		r3, [r12, TH_RECOVER]
	COPYIO_MAP_USER()
	mov		r12, #0							                                
copyinstr_loop:
	ldrb		r3, [r0], #1					                                     
	strb		r3, [r1], #1					                                           
	add		r12, r12, #1
	cmp		r3, #0
	beq		copyinstr_done
	cmp		r12, r2							                           
	bne		copyinstr_loop
  
                                                                      
  
copyinstr_too_long:
	mov		r3, #ENAMETOOLONG
copyinstr_done:
  
                                                                            
                                                                              
                                                                                
                                    
  
	str		r12, [r6]						                            
	mov		r0, r3							                            
copyinstr_exit:
	COPYIO_UNMAP_USER()
	str		r4, [r12, TH_RECOVER]
	ldmfd	sp!, { r4, r5, r6 }
	bx		lr

copyinstr_error:
	                             
	mov		r0, #EFAULT
	b		copyinstr_exit

  
                                                                               
   
	.text
	.align 2
	.globl EXT(copyin)
LEXT(copyin)
	COPYIO_HEADER()
	COPYIO_VALIDATE copyin, 2
	COPYIO_TRY_KERNEL()
	COPYIO_SET_RECOVER()
	COPYIO_MAP_USER()
	COPYIO_BODY copyin
	COPYIO_UNMAP_USER()
	COPYIO_RESTORE_RECOVER()
	bx	lr

  
                                                                                 
   
	.text
	.align 2
	.globl EXT(copyout)
LEXT(copyout)
	COPYIO_HEADER()
	COPYIO_VALIDATE copyout, 2
	COPYIO_TRY_KERNEL()
	COPYIO_SET_RECOVER()
	COPYIO_MAP_USER()
	COPYIO_BODY copyout
	COPYIO_UNMAP_USER()
	COPYIO_RESTORE_RECOVER()
	bx		lr


  
                                                                           
                   
                     
   
	.text
	.align 2
	.globl EXT(copyin_atomic32)
LEXT(copyin_atomic32)
	tst		r0, #3			                                 
	bne		2f

	mov		r2, #4
	COPYIO_VALIDATE copyin_user, 1
	COPYIO_SET_RECOVER()
	COPYIO_MAP_USER()

	ldr		r2, [r0]		                      
	str		r2, [r1]		                       
	mov		r0, #0			          

	COPYIO_UNMAP_USER()
	COPYIO_RESTORE_RECOVER()
	bx		lr
2:	                    
	mov		r0, #EINVAL
	bx		lr

  
                                                                       
                   
               
   
	.text
	.align 2
	.globl EXT(copyin_atomic32_wait_if_equals)
LEXT(copyin_atomic32_wait_if_equals)
	tst		r0, #3			                                 
	bne		2f

	mov		r2, r0
	mov		r3, #4
	COPYIO_VALIDATE copyio_user, 1		                                      
	COPYIO_SET_RECOVER()
	COPYIO_MAP_USER()

	ldrex		r2, [r0]
	cmp		r2, r1
	movne		r0, ESTALE
	bne		1f
	mov		r0, #0
	wfe
1:
	clrex

	COPYIO_UNMAP_USER()
	COPYIO_RESTORE_RECOVER()
	bx		lr
2:	                    
	mov		r0, #EINVAL
	bx		lr

  
                                                                           
                   
                     
   
	.text
	.align 2
	.globl EXT(copyin_atomic64)
LEXT(copyin_atomic64)
	tst		r0, #7			                                 
	bne		2f

	mov		r2, #8
	COPYIO_VALIDATE copyin_user, 1
	COPYIO_SET_RECOVER()
	COPYIO_MAP_USER()

1:	                         
	ldrexd		r2, r3, [r0]		                             
	strexd		r5, r2, r3, [r0]	                                                                         
	cmp		r5, #0
	bne		1b
	stm		r1, {r2, r3}		                       
	mov		r0, #0			          

	COPYIO_UNMAP_USER()
	COPYIO_RESTORE_RECOVER()
	bx		lr
2:	                    
	mov		r0, #EINVAL
	bx		lr


copyio_error:
	mov		r0, #EFAULT
	COPYIO_UNMAP_USER()
	str		r4, [r12, TH_RECOVER]
	ldmfd		sp!, { r4, r5, r6 }
	bx		lr


  
                                                               
               
                   
   
	.text
	.align 2
	.globl EXT(copyout_atomic32)
LEXT(copyout_atomic32)
	tst		r1, #3			                                 
	bne		2f

	mov		r2, r1
	mov		r3, #4
	COPYIO_VALIDATE copyio_user, 1		                                      
	COPYIO_SET_RECOVER()
	COPYIO_MAP_USER()

	str		r0, [r1]		                     
	mov		r0, #0			          

	COPYIO_UNMAP_USER()
	COPYIO_RESTORE_RECOVER()
	bx		lr
2:	                     
	mov		r0, #EINVAL
	bx		lr


  
                                                               
                   
                   
   
	.text
	.align 2
	.globl EXT(copyout_atomic64)
LEXT(copyout_atomic64)
	tst		r2, #7			                                 
	bne		2f

	mov		r3, #8
	COPYIO_VALIDATE copyio_user, 2		                                      
	COPYIO_SET_RECOVER()
	COPYIO_MAP_USER()

1:	                         
	ldrexd		r4, r5, [r2]
	strexd		r3, r0, r1, [r2]	                                       
	cmp		r3, #0
	bne		1b

	mov		r0, #0			          

	COPYIO_UNMAP_USER()
	COPYIO_RESTORE_RECOVER()
	bx		lr
2:	                     
	mov		r0, #EINVAL
	bx		lr


  
                                                                                    
   
	.text
	.align 2
	.globl EXT(copyin_kern)
LEXT(copyin_kern)
	COPYIO_HEADER()
	b		copyio_kern_body

  
                                                                                      
   
	.text
	.align 2
	.globl EXT(copyout_kern)
LEXT(copyout_kern)
	COPYIO_HEADER()
	b		copyio_kern_body

copyio_kern_body:
	stmfd	sp!, { r5, r6 }
	COPYIO_BODY copyio_kernel
	ldmfd	sp!, { r5, r6 }
	bx		lr
		
  
                                                                    
  
                                                               
                                
   
	.text
	.align 2
	.globl EXT(copyinframe)
LEXT(copyinframe)
	COPYIO_SET_RECOVER()
	COPYIO_MAP_USER()
	ldmia		r0, {r2, r3}
	stmia		r1, {r2, r3}
	b		Lcopyin_noerror

   
                                     
   
	.text
	.align 2
	.globl EXT(arm_debug_read_dscr)
LEXT(arm_debug_read_dscr)
#if __ARM_DEBUG__ >= 6
	mrc		p14, 0, r0, c0, c1
#else
	mov		r0, #0
#endif
	bx		lr

  
                                                          
  
                                                            
                                                      
                                                              
                 
   
       .text
       .align 2
       .globl EXT(arm_debug_set_cp14)
LEXT(arm_debug_set_cp14)
#if __ARM_DEBUG__ >= 6
	mrc		p15, 0, r1, c13, c0, 4					                
	ldr		r2, [r1, ACT_CPUDATAP]					                  
	str   	r0, [r2, CPU_USER_DEBUG]				                         

	                           
	movw    ip, #0xCE55
	movt    ip, #0xC5AC
	mcr     p14, 0, ip, c1, c0, 4

	                                                              
	mrc     p14, 0, ip, c0, c1, 0
	orr     ip, ip, #0x8000    	                 
#if __ARM_DEBUG__ >= 7
	mcr     p14, 0, ip, c0, c2, 2
#else
	mcr	    p14, 0, ip, c0, c1, 0
#endif
	                                             
	mov     r1, #0
	mcr     p14, 0, r1, c0, c0, 5          
	mcr     p14, 0, r1, c0, c1, 5          
	mcr     p14, 0, r1, c0, c2, 5          
	mcr     p14, 0, r1, c0, c3, 5          
	mcr     p14, 0, r1, c0, c4, 5          
	mcr     p14, 0, r1, c0, c5, 5          
	mcr     p14, 0, r1, c0, c0, 7          
	mcr     p14, 0, r1, c0, c1, 7          
	                                                            
	cmp     r0, #0
	biceq   ip, ip, #0x8000		                 
#if __ARM_DEBUG__ >= 7
	mcreq   p14, 0, ip, c0, c2, 2
#else
	mcreq   p14, 0, ip, c0, c1, 0
#endif
	bxeq    lr
	ldmia   r0!, {r1, r2, r3, ip}
	mcr     p14, 0, r1, c0, c0, 4          
	mcr     p14, 0, r2, c0, c1, 4          
	mcr     p14, 0, r3, c0, c2, 4          
	mcr     p14, 0, ip, c0, c3, 4          
	ldmia   r0!, {r1, r2}
	mcr     p14, 0, r1, c0, c4, 4          
	mcr     p14, 0, r2, c0, c5, 4          
	add     r0, r0, #40                                 
	ldmia   r0!, {r1, r2, r3, ip}
	mcr     p14, 0, r1, c0, c0, 5          
	mcr     p14, 0, r2, c0, c1, 5          
	mcr     p14, 0, r3, c0, c2, 5          
	mcr     p14, 0, ip, c0, c3, 5          
	ldmia   r0!, {r1, r2}
	mcr     p14, 0, r1, c0, c4, 5          
	mcr     p14, 0, r2, c0, c5, 5          
	add     r0, r0, #40                                 
	ldmia   r0!, {r1, r2}
	mcr     p14, 0, r1, c0, c0, 6          
	mcr     p14, 0, r2, c0, c1, 6          
	add     r0, r0, #56                                 
	ldmia   r0!, {r1, r2}
	mcr     p14, 0, r1, c0, c0, 7          
	mcr     p14, 0, r2, c0, c1, 7          
	
	                         
	mov     ip, #0
	mcr     p14, 0, ip, c1, c0, 4
#endif
	bx      lr
	
  
                                              
   
	.text
	.align 2
	.globl EXT(fiq_context_init)
LEXT(fiq_context_init)
	mrs		r3, cpsr									                    
	cmp		r0, #0										                  
	bicne	r3, r3, #PSR_FIQF							                          
	mrc		p15, 0, r12, c13, c0, 4						                
	ldr		r2, [r12, ACT_CPUDATAP]						                       

#if __ARM_TIME__
	                                                     
                                                       
                                                     
                                                    
                                                     
                                                       
                                                   
                                                        
          
    

	mov		r0, #1										               
	mcr		p15, 0, r0, c14, c3, 1						                    

	                                                          
                                                      
                                                          
                                                           
                                                           
                                                           
                                                       
    
	mcr		p15, 0, r0, c14, c1, 0						                                    

#else                     
	msr		cpsr_c, #(PSR_FIQ_MODE|PSR_FIQF|PSR_IRQF)	                                           
	mov		r8, r2										                               
	ldr		r9, [r2, CPU_GET_FIQ_HANDLER]				                            
	ldr		r10, [r2, CPU_TBD_HARDWARE_ADDR]			                            
	ldr		r11, [r2, CPU_TBD_HARDWARE_VAL]				                          
#endif                   

	msr		cpsr_c, r3									                     
	bx		lr

  
                                   
   
	.text
	.align 2
	.globl EXT(reenable_async_aborts)
LEXT(reenable_async_aborts)
	cpsie 	a											                         
	bx		lr

  
                                             
   
	.text
	.align 2
	.globl EXT(ml_get_speculative_timebase)
LEXT(ml_get_speculative_timebase)
1:
	mrrc	p15, 0, r3, r1, c14							                                          
	mrrc	p15, 0, r0, r3, c14							                                         
	mrrc	p15, 0, r3, r2, c14							                                          
	cmp		r1, r2
	bne		1b											                                           

	mrc		p15, 0, r12, c13, c0, 4						                
	ldr		r3, [r12, ACT_CPUDATAP]						                       
	ldr		r2, [r3, CPU_BASE_TIMEBASE_LOW]				                       
	adds	r0, r0, r2									             
	ldr		r2, [r3, CPU_BASE_TIMEBASE_HIGH]			                     
	adc		r1, r1, r2									  
	bx		lr											         

  
                                 
   
	.text
	.align 2
	.globl EXT(ml_get_timebase)
LEXT(ml_get_timebase)
	isb													                                                             
	b	EXT(ml_get_speculative_timebase)

  
                                         
   
	.text
	.align 2
	.globl EXT(ml_get_timebase_entropy)
LEXT(ml_get_timebase_entropy)
	b	EXT(ml_get_speculative_timebase)

  
                                    
   
	.text
	.align 2
	.globl EXT(ml_get_decrementer)
LEXT(ml_get_decrementer)
	mrc		p15, 0, r12, c13, c0, 4						                
	ldr		r3, [r12, ACT_CPUDATAP]						                       
	ldr		r2, [r3, CPU_GET_DECREMENTER_FUNC]			                           
	cmp		r2, #0
	bxne	r2											                          
#if __ARM_TIME__
	mrc		p15, 0, r0, c14, c3, 0						                                   
#else
	ldr		r0, [r3, CPU_DECREMENTER]					                          
#endif
	bx		lr											         


  
                                              
   
	.text
	.align 2
	.globl EXT(ml_set_decrementer)
LEXT(ml_set_decrementer)
	mrc		p15, 0, r12, c13, c0, 4						                
	ldr		r3, [r12, ACT_CPUDATAP]						                       
	ldr		r2, [r3, CPU_SET_DECREMENTER_FUNC]			                           
	cmp		r2, #0
	bxne	r2											                          
#if __ARM_TIME__
	str		r0, [r3, CPU_DECREMENTER]					                         
	mcr		p15, 0, r0, c14, c3, 0						                                    
#else
	mrs		r2, cpsr									                    
	msr		cpsr_c, #(PSR_FIQ_MODE|PSR_FIQF|PSR_IRQF)	                                            
	mov		r12, r0										                    
	str		r12, [r8, CPU_DECREMENTER]					            
	msr		cpsr_c, r2									                     
#endif
	bx		lr


  
                                            
   
	.text
	.align 2
	.globl EXT(ml_get_interrupts_enabled)
LEXT(ml_get_interrupts_enabled)
	mrs	r2, cpsr
	mov		r0, #1
	bic		r0, r0, r2, lsr #PSR_IRQFb
	bx		lr

  
                                                     
  
   

#if defined(ARM_BOARD_CLASS_T8002)
	.text
	.align 2
	.globl EXT(fleh_fiq_t8002)
LEXT(fleh_fiq_t8002)
	mov		r13, #kAICTmrIntStat
	str		r11, [r10, r13]						                                  
	mvn		r13, #0
	str		r13, [r8, CPU_DECREMENTER]
	b		EXT(fleh_dec)

	.text
	.align 2
	.globl EXT(t8002_get_decrementer)
LEXT(t8002_get_decrementer)
	ldr		ip, [r3, CPU_TBD_HARDWARE_ADDR]					                           
	mov		r0, #kAICTmrCnt
	add		ip, ip, r0
	ldr		r0, [ip]										                      
	bx		lr

	.text
	.align 2
	.globl EXT(t8002_set_decrementer)
LEXT(t8002_set_decrementer)
	str		r0, [r3, CPU_DECREMENTER]					                         
	ldr		ip, [r3, CPU_TBD_HARDWARE_ADDR]				                           
	mov		r5, #kAICTmrCnt
	str		r0, [ip, r5]						                            
	bx		lr
#endif                                     

LOAD_ADDR_GEN_DEF(kernel_pmap_store)

#include        "globals_asm.h"

                    
