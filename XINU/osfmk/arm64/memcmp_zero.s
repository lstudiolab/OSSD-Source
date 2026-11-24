/*
 * Copyright (c) 2012 Apple Computer, Inc. All rights reserved.
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
 *
 * This file implements the following function for the arm64 architecture:
 *
 *  int memcmp_zero_ptr_aligned(const void *s, size_t n);
 *
 * The memcmp_zero_ptr_aligned function checks string s of n bytes contains all zeros.
 * Address and size of the string s must be pointer-aligned (8-byte for arm64).
 * Return 0 if true, 1 otherwise. Also return 0 if n is 0.
 */

                                 
#ifdef __arm64__

#include "asm.h"

.globl _memcmp_zero_ptr_aligned

                                                                              
                                                                              
                                                                               

.macro EstablishFrame
	ARM64_STACK_PROLOG
	stp       fp, lr, [sp, #-16]!
	mov       fp,      sp
.endm

.macro ClearFrameAndReturn
	ldp       fp, lr, [sp], #16
	ARM64_STACK_EPILOG
.endm

                                                                              
                                                                              
                                                                               

.text
.align 5

                                                                              
                                                                               
                                                                               

_memcmp_zero_ptr_aligned:

                                                                                   
                                                                            
                                                                          
                                          

	EstablishFrame
	cmp         x1,     #64
	b.lo        L_sizeIsSmall

                                                                  
                                                                    
                                                                        
                                                               
	mov         x2,     x0                                   
	add         x0,     x0, #64
	and         x0,     x0, #-64                   
	ldp         q4, q5, [x2]
	ldp         q6, q7, [x2, #32]
	sub         x2,     x0, x2                                                
	sub         x1,     x1, x2                      
	subs        x1,     x1, #64                         
	b.ls        L_cleanup

L_loop:
	ldp         q0, q1, [x0]
	ldp         q2, q3, [x0, #32]
	orr.16b     v4,     v4, v0                                       
	orr.16b     v5,     v5, v1
	orr.16b     v6,     v6, v2
	orr.16b     v7,     v7, v3
	add         x0,     x0, #64                       
	subs        x1,     x1, #64                         
	b.hi        L_loop

L_cleanup:
                                                             
                                                                            
                                                                               
                                      
	add         x0,     x0, x1
	ldp         q0, q1, [x0]
	ldp         q2, q3, [x0, #32]
	orr.16b     v4,     v4, v0                                       
	orr.16b     v5,     v5, v1
	orr.16b     v6,     v6, v2
	orr.16b     v7,     v7, v3

	orr.16b     v4,     v4, v5                              
	orr.16b     v6,     v6, v7
	orr.16b     v4,     v4, v6                             
	umaxv.16b   b0,     v4                                 
	umov        w0,     v0.b[0]                                
	tst         w0,     w0
	cset        x0,     ne                                          
	ClearFrameAndReturn

L_sizeIsSmall:
	cbz     x1,     L_sizeIsZero                                    

	mov     x3,     #0
0:	ldr     x2,    [x0],#8
	orr     x3,     x3, x2                                       
	subs    x1,     x1, #8                      
	b.hi    0b

	tst     x3,     x3
	cset    x0,     ne                                              
	ClearFrameAndReturn

L_sizeIsZero:
	mov     x0,     #0
	ClearFrameAndReturn

#endif             
