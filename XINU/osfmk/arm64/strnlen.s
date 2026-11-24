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
 *  size_t strnlen(const char *string, size_t maxlen);
 *
 * The strnlen function returns either strlen(string) or maxlen, whichever
 * is amller, without reading beyond the first maxlen characters of string.
 */

#include <arm64/asm.h>

.globl _strlen
.globl _strnlen

                                                                              
                                                                              
                                                                               

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
L_masks:
.quad 0x0706050403020100, 0x0f0e0d0c0b0a0908
.quad 0x0000000000000000, 0x0000000000000000

                                                                              
                                                                              
                                                                               

_strnlen:
                                                                            
                                                                           
                                                                            
                                                          
	tst       x1,      x1
	b.mi      _strlen
	b.eq      L_maxlenIsZero
	EstablishFrame
                                                                      
	and       x2,      x0, #-16
	ldr       q0,     [x2]
                                                                         
                                                                          
                                                                
	adr       x3,          L_masks
	ldr       q2,     [x3],#16
                                                                            
                                                                       
                                                                             
                                                                             
                                                                         
                                                                            
                                            
	and       x4,      x0, #0xf
	sub       x3,      x3, x4
	ldr       q1,     [x3]
	orr.16b   v0,      v0, v1
                                                                            
                                        
	add       x1,      x1, x4
	b         1f

                                                                             
                                                                             
                                                                          
.align 4
0:	ldr       q0,     [x2, #16]!
1:  uminv.16b b1,      v0
	fmov      w3,      s1
	cbz       w3,      L_foundNUL
	subs      x1,      x1, #16
	b.hi      0b

                                                                            
                             
	sub       x0,      x2, x0
	add       x1,      x1, #16
	add       x0,      x0, x1
	ClearFrameAndReturn

L_maxlenIsZero:
	mov       x0,      #0
	ret                                                              

L_foundNUL:
                                                                          
                                                                        
                        
	eor.16b   v1,      v1, v1
	cmhi.16b  v0,      v0, v1
	orr.16b   v0,      v0, v2
	uminv.16b b1,      v0
	fmov      w3,      s1                                    
	sub       x0,      x2, x0                              
	cmp       x1,      x3                                          
	csel      x1,      x1, x3, cc                              
	add       x0,      x0, x1
	ClearFrameAndReturn

                                                                              
                                                                              
                                                                               

.align 4
_strlen:
	EstablishFrame
                                                                      
	and       x1,      x0, #-16
	ldr       q0,     [x1]
                                                                         
                                                                          
                                                                
	adr       x3,          L_masks
	ldr       q2,     [x3],#16
                                                                            
                                                                       
                                                                             
                                                                             
                                                                         
                                                                            
                                            
	and       x2,      x0, #0xf
	sub       x3,      x3, x2
	ldr       q1,     [x3]
	orr.16b   v0,      v0, v1
	b         1f

                                                     
  
                                          
                                      
                             
  
                                                                             
.align 4
0:	ldr       q0,     [x1, #16]!
1:	uminv.16b b1,      v0
	fmov      w2,      s1                                                      
	cbnz      w2,      0b

                                                                           
                                          
  
                                  
                                                         
                                              
                               
  
                                                                           
                                                  
  
                              
                                            
	eor.16b   v1,      v1, v1
	cmhi.16b  v0,      v0, v1
	orr.16b   v0,      v0, v2
	uminv.16b b1,      v0
	fmov      w2,      s1
	sub       x0,      x1, x0
	add       x0,      x0, x2
	ClearFrameAndReturn
