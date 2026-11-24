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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 *
 * This file implements the following function for the arm64 architecture:
 *
 *  int strncmp(const char *s1, const char *s2, size_t n);
 *
 * Returns 0 if the two strings are equal up to the first n bytes or to the
 * end of the string, whichever comes first.  Otherwise, returns the difference
 * of the first mismatched characters interpreted as uint8_t.
 */

#include <arm64/asm.h>

.globl _strncmp

                                                                              
                                                                              
                                                                               

.macro EstablishFrame
	ARM64_STACK_PROLOG
	stp       fp, lr, [sp, #-16]!
	mov       fp,      sp
.endm

.macro ClearFrameAndReturn
	ldp       fp, lr, [sp], #16
	ARM64_STACK_EPILOG
.endm

#include "../mach/arm/vm_param.h"
#define kVectorSize 16

                                                                              
                                                                              
                                                                               

.text
.align 5
L_mask:
.quad 0x0706050403020100, 0x0f0e0d0c0b0a0908

                                                                              
                                                                              
                                                                               

_strncmp:
	EstablishFrame
	eor       x3,      x3, x3
	cbz       x2,      L_scalarDone
                                                            
0:	tst       x0,      #(kVectorSize-1)
	b.eq      L_s1aligned
	ldrb      w4,     [x0],#1                        
	ldrb      w5,     [x1],#1                        
	subs      x3,      x4, x5                         
	ccmp      w4,  #0, #4, eq                         
	b.eq      L_scalarDone                             
	subs      x2,      x2, #1                     
	b.ne      0b                                           

                                                                          
                                                    
L_scalarDone:
	mov       x0,      x3
	ClearFrameAndReturn

L_s1aligned:
                                                                              
                                                                              
                                                                       
	tst       x1,      #(kVectorSize-1)
	b.eq      L_naiveVector

                                                                              
                                                                              
                                                                               

                                                                         
                                                                         
                                                                              
                                                                             
                                                                           
                                                      
	neg       x7,      x1
	ands      x7,      x7, #(PAGE_MIN_SIZE-kVectorSize)
	b.eq      2f

.align 4
                                                                          
                                                                          
                                                                             
                                       
0:	cmp       x2,      x7
	b.ls      L_naiveVector
	sub       x6,      x2, x7
                                                                             
                                              
1:	ldr       q0,     [x0],#(kVectorSize)
	ldr       q1,     [x1],#(kVectorSize)
	cmeq.16b  v1,      v0, v1
	and.16b   v0,      v0, v1                                            
	uminv.16b b1,      v0
	fmov      w3,      s1                                              
	cbz       w3,      L_vectorDone
	subs      x7,      x7, #(kVectorSize)
	b.ne      1b
                              
	mov       x2,      x6
                                                                              
                                                                             
                             
2:	ldrb      w4,     [x0],#1                        
	ldrb      w5,     [x1],#1                        
	subs      x3,      x4, x5                         
	ccmp      w4,  #0, #4, eq                         
	b.eq      L_scalarDone                             
	subs      x2,      x2, #1                     
	b.eq      L_scalarDone                          
	tst       x0,      #(kVectorSize-1)
	b.ne      2b
                                                                            
                                                                            
                                                    
	mov       x7,      #(PAGE_MIN_SIZE-kVectorSize)
	b         0b

                                                                              
                                                                              
                                                                               

.align 4
L_naiveVector:
	ldr       q0,     [x0],#(kVectorSize)
	ldr       q1,     [x1],#(kVectorSize)
	cmeq.16b  v1,      v0, v1
	and.16b   v0,      v0, v1                                            
	uminv.16b b1,      v0
	fmov      w3,      s1                                              
	cbz       w3,      L_vectorDone
	subs      x2,      x2, #16
	b.hi      L_naiveVector

L_readNBytes:
	eor       x0,      x0, x0
	ClearFrameAndReturn

L_vectorDone:
                                                                       
                     
	eor.16b   v1,      v1, v1
	cmhi.16b  v0,      v0, v1                                  
	ldr       q1,      L_mask
	orr.16b   v0,      v0, v1                                                    
	uminv.16b b1,      v0
	fmov      w3,      s1
                                                                        
                                                                   
	cmp       x3,      x2
	b.cs      L_readNBytes
	sub       x3,      x3, #(kVectorSize)
	ldrb      w4,     [x0, x3]
	ldrb      w5,     [x1, x3]
	sub       x0,      x4, x5
	ClearFrameAndReturn
