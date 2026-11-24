/*
 * Copyright (c) 2010, 2011 Apple, Inc. All rights reserved.
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
 
.text
.syntax unified
.code 32
.globl _strncmp
                                                         
  
                                                                           
                                                                         
                                                                  

#define ESTABLISH_FRAME        \
	push   {r4-r7,lr}         ;\
	add     r7,     sp, #12   ;\
	push   {r8,r10}
#define CLEAR_FRAME            \
	pop    {r8,r10}            ;\
	pop    {r4-r7,lr}

.align 3
.long 0, 0x01010101
_strncmp:
                                                                 
	cmp     r2,         #16
	blo     L_byteCompareLoop
                                                                              
                                                           
0:	ldrb    r3,    [r0],#1
	ldrb    ip,    [r1],#1
	sub     r2,         #1
	cmp     r3,         #1
	cmphs   r3,         ip
	bne     L_earlyReturn
                                                                              
                                                                         
                                                  
	tst     r0,         #3
	bne     0b
	
                                                                           
                                                                             
	ESTABLISH_FRAME
	ldr     lr,    (_strncmp-4)

                                                                          
                                                                             
                                        
  
                                                        
                           
                           
                                           
                                           
                                           
                               
                                            
	and     r10,    r1, #3
	bic     r1,     r1, #3
	mov     r10,        r10, lsl #3
	rsb     r6,     r10,#32
	
                                                                            
                            
	sub     r2,     r2, r6, lsr #3
	
                                                                               
                                                                             
                                                                          
                                              
	ldr     r8,    [r1],#4
	orr     r8,     r8, lr, lsr r6
	sub     r3,     r8, lr
	bic     r3,     r3, r8
	tst     r3,         lr, lsl #7
	mov     r5,         r8, lsr r10
	bne     L_unwindLoopPreload
	
.align 3
L_wordCompareLoop:
                                                                       
	subs    r2,     r2, #4
	blo     L_nIsLessThanFour
                                                                            
                                                                              
                                                                              
                            
                                                                           
	ldr     r8,    [r1],#4
#if defined _ARM_ARCH_6
    uqsub8  r3,     lr, r8
    tst     r3,         r3
    ldr     ip,    [r0],#4
#else
	sub     r3,     r8, lr
	bic     r3,     r3, r8
	ldr     ip,    [r0],#4
	tst     r3,         lr, lsl #7
#endif
	orr     r4,     r5, r8, lsl r6
	cmpeq   ip,         r4
	mov     r5,         r8, lsr r10
	beq     L_wordCompareLoop

                                                                            
                                                                        
	sub     r0,     r0, #4
	sub     r1,     r1, #4
L_nIsLessThanFour:
	add     r2,     r2, #4
L_unwindLoopPreload:
	sub     r1,     r1, r6, lsr #3
	add     r2,     r2, r6, lsr #3
	CLEAR_FRAME
	
L_byteCompareLoop:
                                                                              
                                            
	subs    r2,     r2, #1
	movlo   r0,         #0
	bxlo    lr
                                                                              
                                                           
	ldrb    r3,    [r0],#1
	ldrb    ip,    [r1],#1
	cmp     r3,         #1
	cmpcs   r3,         ip
	beq     L_byteCompareLoop
	
L_earlyReturn:
                                                                 
	sub     r0,     r3, ip
	bx      lr
