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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <arm/proc_reg.h>

#include <arm/asm.h>
	
   
                                                                                             
          
  
                                                                                   
                                                                     
   
	.syntax unified
	.text
	.align 2

  
                                                               
  
                                                                          
                          
   
ENTRY(secure_memset)
                                                 
ENTRY(memset)
	                                        
	mov		r3, r2
	and		r1, r1, #0xff
	orr		r1, r1, r1, lsl #8
	orr		r2, r1, r1, lsl #16
	mov		r1, r3
	b		Lbzeroengine

                                        
ENTRY2(bzero,__bzero)
	                                                  
	mov		r2, #0

Lbzeroengine:
	                                                                                              
	mov		r12, r0

	                                       
	mov		r3, r2

	                        
	cmp		r1, #0
	bxeq	lr

	                                                          
	cmp		r1, #32
	blt		L_bytewise

	                                     
	tst		r12, #0x1f
	bne		L_unaligned

	                                                  
	cmp		r1, #64
	blt		L_lessthan64aligned

	                                         
L_64ormorealigned:

	                                                                               
	stmfd	sp!, { r4-r6, r8, r10-r11 }
	mov		r4, r2
	mov		r5, r2
	mov		r6, r2
	mov		r8, r2
	mov		r10, r2
	mov		r11, r2

	                                                                        
	sub		r1, r1, #64

L_64loop:
	stmia	r12!, { r2-r6, r8, r10-r11 }
	subs	r1, r1, #64
	stmia	r12!, { r2-r6, r8, r10-r11 }
	bge		L_64loop

	                            
	ldmfd	sp!, { r4-r6, r8, r10-r11 }

	                                                                           
	adds	r1, r1, #64
	bxeq	lr

L_lessthan64aligned:
	                                      
	cmp		r1, #16
	stmiage	r12!, { r2-r3 }
	stmiage	r12!, { r2-r3 }
	subsge	r1, r1, #16
	bgt		L_lessthan64aligned
	bxeq	lr

L_lessthan16aligned:
	                         
	mov		r1, r1, lsl #28		                                                                 
	msr		cpsr_f, r1

	stmiami	r12!, { r2-r3 }		                             
	streq	r2, [r12], #4		                             
	strhcs	r2, [r12], #2		                             
	strbvs	r2, [r12], #1		                            
	bx		lr

L_bytewise:
	                                                                	
	subs	r1, r1, #2
	strb	r2, [r12], #1
	strbpl	r2, [r12], #1
	bhi		L_bytewise
	bx		lr

L_unaligned:
	                                                                                 
	mov		r3, r12, lsl #28
	rsb     r3, r3, #0x00000000
	msr		cpsr_f, r3

	strbvs	r2, [r12], #1		                                          
	strhcs	r2, [r12], #2		                                          
	streq	r2, [r12], #4		                                          
	strmi	r2, [r12], #4		                                          
	strmi	r2, [r12], #4

	subs	r1, r1, r3, lsr #28
	bxeq	lr

	                                              
	mov		r3, r2

	                                         
	tst		r12, #(1 << 4)
	stmiane	r12!, { r2-r3 }
	stmiane	r12!, { r2-r3 }
	subsne	r1, r1, #16

	                                                   
	cmp		r1, #64
	bge		L_64ormorealigned
	b		L_lessthan64aligned

