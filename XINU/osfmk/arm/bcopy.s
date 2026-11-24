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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <arm/proc_reg.h>

.syntax unified
.text
.align 2
	
	.globl _ovbcopy
	.globl _memcpy
	.globl _bcopy
	.globl _memmove

_bcopy:		                                                          
_ovbcopy:
	mov		r3, r0
	mov		r0, r1
	mov		r1, r3

_memcpy:		                                                            
_memmove: 	                                                             
	                                                        
	cmp		r2, #0
	cmpne	r0, r1
	bxeq	lr

	                                                            
	stmfd   sp!, { r0, r4, r5, r7, lr }
	add	r7, sp, #12
	
	                                                          
	subhs	r3, r0, r1
	sublo	r3, r1, r0
	cmp		r3, r2			                                                   
	blo		Loverlap

Lnormalforwardcopy:
	                                                 
	mov		r12, r0, lsl #30
	cmp		r12, r1, lsl #30
	bne		Lnonwordaligned_forward

	                                          
	cmp		r2, #64
	blt		Lsmallforwardcopy

	                                            
	tst		r0, #0xf
	bne		Lsimilarlyunaligned

	                                        
	tst		r0, #(1<<4)
	bne		Lunaligned_32

Lmorethan64_aligned:
	                                                 
	stmfd	sp!, { r6, r8, r10, r11 }

	                                                                                
	sub		r2, r2, #64

L64loop:
	                             
	ldmia	r1!, { r3, r4, r5, r6, r8, r10, r11, r12 }
	pld		[r1, #32]
	stmia	r0!, { r3, r4, r5, r6, r8, r10, r11, r12 }
	ldmia	r1!, { r3, r4, r5, r6, r8, r10, r11, r12 }
	subs	r2, r2, #64
	pld		[r1, #32]
	stmia	r0!, { r3, r4, r5, r6, r8, r10, r11, r12 }
	bge		L64loop

	                                                 
	ldmfd	sp!, { r6, r8, r10, r11 }

	                                                                                                
	adds	r2, r2, #64
	beq		Lexit

Llessthan64_aligned:
	                                                      
	cmp		r2, #16
	ldmiage	r1!, { r3, r4, r5, r12 }
	stmiage	r0!, { r3, r4, r5, r12 }
	subsge	r2, r2, #16
	bgt		Llessthan64_aligned
	beq		Lexit
	
Llessthan16_aligned:
	mov		r2, r2, lsl #28
	msr		cpsr_f, r2

	ldmiami	r1!, { r2, r3 }
	ldreq	r4, [r1], #4
	ldrhcs	r5, [r1], #2
	ldrbvs	r12, [r1], #1

	stmiami	r0!, { r2, r3 }
	streq	r4, [r0], #4
	strhcs	r5, [r0], #2
	strbvs	r12, [r0], #1
	b		Lexit

Lsimilarlyunaligned:
	                                                                                        
	mov		r12, r0, lsl #28
	rsb		r12, r12, #0
	msr		cpsr_f, r12

	ldrbvs	r3, [r1], #1
	ldrhcs	r4, [r1], #2
	ldreq	r5, [r1], #4

	strbvs	r3, [r0], #1
	strhcs	r4, [r0], #2
	streq	r5, [r0], #4

	ldmiami	r1!, { r3, r4 }
	stmiami	r0!, { r3, r4 }

	subs	r2, r2, r12, lsr #28
	beq		Lexit

Lunaligned_32:
	                                        
	tst		r0, #(1 << 4)
	ldmiane	r1!, { r3, r4, r5, r12 }
	stmiane	r0!, { r3, r4, r5, r12 }
	subne	r2, r2, #16

	                                                                  
	cmp		r2, #64
	bge		Lmorethan64_aligned
	b		Llessthan64_aligned
	
Lbytewise2:
	                            
	subs	r2, r2, #2

	ldrb	r3, [r1], #1
	ldrbpl	r4, [r1], #1

	strb	r3, [r0], #1
	strbpl	r4, [r0], #1

	bhi		Lbytewise2
	b		Lexit

Lbytewise:
	                                  
	ldrb	r3, [r1], #1
	subs	r2, r2, #1
	strb	r3, [r0], #1
	bne		Lbytewise
	b		Lexit

Lsmallforwardcopy:
	                                                                         
	cmp		r2, #4
	blt		Lbytewise2

	                                      
	tst		r1, #3
Lwordalignloop:
	ldrbne	r3, [r1], #1
	strbne	r3, [r0], #1
	subne	r2, r2, #1
	tstne	r1, #3
	bne		Lwordalignloop

	cmp		r2, #16
	bge		Llessthan64_aligned
	blt		Llessthan16_aligned

Loverlap:
	                                               
	cmp		r0, r1				                   
	bhi		Loverlap_srclower

Loverlap_destlower:
	                                                                                             
	cmp		r3, #64
	bge		Lnormalforwardcopy 	                                                                     

	cmp		r3, #2
	bge		Lbytewise2
	b		Lbytewise

	                                                                              
Loverlap_srclower:
	                              

	                              
	add		r0, r0, r2
	add		r1, r1, r2

	                                                                                                 
	cmp		r2, #64				                                 
	cmpgt	r3, #64				                                       
	blt		Lbytewise_reverse

	                                                          
	mov		r3, r0, lsl #30
	cmp		r3, r1, lsl #30
	bne		Lbytewise_reverse

	                                                                              
	tst		r0, #0xf
	bne		Lunaligned_reverse_similarly

	                                     
	tst		r0, #(1<<4)
	bne		Lunaligned_32_reverse_similarly

	                                                      
Lmorethan64_aligned_reverse:
	                                                 
	stmfd	sp!, { r6, r8, r10, r11 }

	                                                                                
	sub		r2, r2, #64

L64loop_reverse:
	                             
	ldmdb	r1!, { r3, r4, r5, r6, r8, r10, r11, r12 }
#if ARCH_ARMv5 || ARCH_ARMv5e || ARCH_ARMv6
	pld		[r1, #-32]
#endif
	stmdb	r0!, { r3, r4, r5, r6, r8, r10, r11, r12 }	
	ldmdb	r1!, { r3, r4, r5, r6, r8, r10, r11, r12 }	
	subs	r2, r2, #64
	pld		[r1, #-32]
	stmdb	r0!, { r3, r4, r5, r6, r8, r10, r11, r12 }	
	bge		L64loop_reverse

	                                                 
	ldmfd	sp!, { r6, r8, r10, r11 }

	                                                                                                
	adds	r2, r2, #64
	beq		Lexit

Lbytewise_reverse:
	ldrb	r3, [r1, #-1]!
	strb	r3, [r0, #-1]!
	subs	r2, r2, #1
	bne		Lbytewise_reverse
	b		Lexit

Lunaligned_reverse_similarly:
	                                                                                        
	mov		r12, r0, lsl #28
	msr		cpsr_f, r12

	ldrbvs	r3, [r1, #-1]!
	ldrhcs	r4, [r1, #-2]!
	ldreq	r5, [r1, #-4]!

	strbvs	r3, [r0, #-1]!
	strhcs	r4, [r0, #-2]!
	streq	r5, [r0, #-4]!

	ldmdbmi	r1!, { r3, r4 }
	stmdbmi	r0!, { r3, r4 }

	subs	r2, r2, r12, lsr #28
	beq		Lexit

Lunaligned_32_reverse_similarly:
	                                        
	tst		r0, #(1 << 4)
	ldmdbne	r1!, { r3, r4, r5, r12 }
	stmdbne	r0!, { r3, r4, r5, r12 }
	subne	r2, r2, #16

	                                                                  
	cmp		r2, #64
	bge		Lmorethan64_aligned_reverse
	b		Lbytewise_reverse

	                                                              
Lnonwordaligned_forward:
	cmp		r2, #8
	blt		Lbytewise2			                                                        

	                                          
	tst		r1, #3
Lwordalignloop2:
	ldrbne	r3, [r1], #1
	strbne	r3, [r0], #1
	subne	r2, r2, #1
	tstne	r1, #3
	bne		Lwordalignloop2

	                                                   
	and		r3, r0, #3
	cmp		r3, #2
	blt		Lalign1_forward
	beq		Lalign2_forward
	bgt		Lalign3_forward

Lalign1_forward:
	                                             
	mov		r12, r2, lsr #2		                                    
	sub		r0, r0, #1

	                    
	ldrb	r4, [r0]			                 

Lalign1_forward_loop:
	ldr		r3, [r1], #4		            
	orr		r4, r4, r3, lsl #8	                       
	str		r4, [r0], #4		            
	mov		r4, r3, lsr #24		                       
	subs	r12, r12, #1
	bne		Lalign1_forward_loop

	                         
	strb	r4, [r0], #1		                 

	ands	r2, r2, #3
	beq		Lexit
	b		Lbytewise2

Lalign2_forward:
	                                              
	mov		r12, r2, lsr #2		                                    
	sub		r0, r0, #2

	                    
	ldrh	r4, [r0]			                  

Lalign2_forward_loop:
	ldr		r3, [r1], #4		            
	orr		r4, r4, r3, lsl #16	                        
	str		r4, [r0], #4		            
	mov		r4, r3, lsr #16		                        
	subs	r12, r12, #1
	bne		Lalign2_forward_loop

	                         
	strh	r4, [r0], #2		                  

	ands	r2, r2, #3
	beq		Lexit
	b		Lbytewise2

Lalign3_forward:
	                                              
	mov		r12, r2, lsr #2		                                    
	sub		r0, r0, #3

	                    
	ldr		r4, [r0]
	and		r4, r4, #0x00ffffff	                  

Lalign3_forward_loop:
	ldr		r3, [r1], #4		            
	orr		r4, r4, r3, lsl #24	                       
	str		r4, [r0], #4		            
	mov		r4, r3, lsr #8		                       
	subs	r12, r12, #1
	bne		Lalign3_forward_loop

	                         
	strh	r4, [r0], #2		                  
	mov		r4, r4, lsr #16
	strb	r4, [r0], #1		                   

	ands	r2, r2, #3
	beq		Lexit
	b		Lbytewise2

Lexit:
	ldmfd	sp!, { r0, r4, r5, r7, pc }

