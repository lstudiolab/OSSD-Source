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
 *  This file implements the following functions for the arm64 architecture.
 *
 *  void bcopy(const void * source,
 *             void * destination,
 *             size_t length);
 *
 *  void *memmove(void * destination,
 *                const void * source,
 *                size_t n);
 *
 *  void *memcpy(void * restrict destination,
 *               const void * restrict source,
 *               size_t n);
 *
 * All copy n successive bytes from source to destination.  Memmove and memcpy
 * return destination, whereas bcopy has no return value.  Copying takes place
 * as if it were through a temporary buffer -- after return destination
 * contains exactly the bytes from source, even if the buffers overlap (this is
 * not required of memcpy by the C standard; its behavior is undefined if the
 * buffers overlap, but we are holding ourselves to the historical behavior of
 * this function on MacOS).
 */

#include "asm.h"

.globl _bcopy
.globl _ovbcopy
.globl _memcpy
.globl _memmove

                                                                              
                                                                              
                                                                               

#define kSmallCopy 64

                                                                              
                                                                              
                                                                               

.text
.align 5
_bcopy:
_ovbcopy:
                                                                           
	mov     x3,      x0
	mov     x0,      x1
	mov     x1,      x3

.align 4
_memcpy:
_memmove:
                                                                            
                                                                            
                                                                            
                                                                           
        
  
                                                          
  
                                                                        
                                                                             
                                                                             
                                                                            
                                                           
	ARM64_STACK_PROLOG
	PUSH_FRAME
	sub     x3,      x0, x1
	cmp     x3,      x2
	b.cc    L_reverse
	mov     x3,      x0                                 
	cmp     x2,      #(kSmallCopy)
	b.cc    L_forwardSmallCopy

                                                                              
                                                                              
                                                                               

                                                                           
                                                                           
                                                                        
                                                                             
                                                                            
                              
	add     x3,      x3, #32
	and     x3,      x3, #-32               
	ldp     x12,x13,[x1]
	ldp     x14,x15,[x1, #16]
	sub     x5,      x3, x0                                                
	add     x1,      x1, x5                        

                                                               
  
                              
                                               
                                                                        
                                        
                                         
                                         
  
                                                                            
                                                                         
                                                                          
         
  
                                                                           
                                                                            
                                
	ldp     x8, x9, [x1]
	ldp     x10,x11,[x1, #16]
	add     x1,      x1, #32
	sub     x2,      x2, x5                   
	stp     x12,x13,[x0]                                
	stp     x14,x15,[x0, #16]                           
	subs    x2,      x2, #64
	b.ls    L_forwardCleanup

L_forwardCopyLoop:
                  
  
                                                               
                                      
                             
                                 
                                  
  
                                                                            
                                                          
	stnp    x8, x9, [x3]
	stnp    x10,x11,[x3, #16]
	add     x3,      x3, #32
	ldnp    x8, x9, [x1]
	ldnp    x10,x11,[x1, #16]
	add     x1,      x1, #32
	subs    x2,      x2, #32
	b.hi    L_forwardCopyLoop

L_forwardCleanup:
                                                                     
                                                                          
                                                                        
                                                                            
                                                                    
                                                                            
                                                          
	add     x1,      x1, x2
	ldp     x12,x13,[x1]
	ldp     x14,x15,[x1, #16]
	stp     x8, x9, [x3]
	stp     x10,x11,[x3, #16]
	add     x3,      x3, x2
	stp     x12,x13,[x3, #32]
	stp     x14,x15,[x3, #48]
	POP_FRAME
	ARM64_STACK_EPILOG

                                                                              
                                                                              
                                                                               

                                                                           
                                                                        
                 
  
                                                
                                           
                             
                                                
0:	ldr     x6,     [x1],#8
	str     x6,     [x3],#8
L_forwardSmallCopy:
	subs    x2,      x2, #8
	b.cs    0b
	adds    x2,      x2, #8
	b.eq    2f
1:	ldrb    w6,     [x1],#1
	strb    w6,     [x3],#1
	subs    x2,      x2, #1
	b.ne    1b
2:	POP_FRAME
	ARM64_STACK_EPILOG

                                                                              
                                                                              
                                                                               

                                                                          
                                                                               
                                                                            
                                                                             
                                          
  
                                                                           
                                                                           
                                                                             
                                                                           
                                                                             
                                                                   

L_reverse:
                                                       
	cbz     x3,      L_return
                                                                       
                                                  
	add     x4,      x0, x2
	add     x1,      x1, x2
	cmp     x2,      #(kSmallCopy)
	b.cc    L_reverseSmallCopy

                                                                              
                                                                              
                                                                               

	ldp     x12,x13,[x1, #-16]
	ldp     x14,x15,[x1, #-32]
	sub     x3,      x4, #1                                               
	and     x3,      x3, #-32                                           
	sub     x5,      x4, x3                                               
	sub     x1,      x1, x5                                     
	sub     x2,      x2, x5
	ldp     x8, x9, [x1, #-16]
	ldp     x10,x11,[x1, #-32]
	stp     x12,x13,[x4, #-16]
	stp     x14,x15,[x4, #-32]
	sub     x1,      x1, #32
	subs    x2,      x2, #64
	b.ls    L_reverseCleanup

L_reverseCopyLoop:
	stnp    x8, x9, [x3, #-16]
	stnp    x10,x11,[x3, #-32]
	sub     x3,      x3, #32
	ldnp    x8, x9, [x1, #-16]
	ldnp    x10,x11,[x1, #-32]
	sub     x1,      x1, #32
	subs    x2,      x2, #32
	b.hi    L_reverseCopyLoop

L_reverseCleanup:
	sub     x1,      x1, x2
	ldp     x12,x13,[x1, #-16]
	ldp     x14,x15,[x1, #-32]
	stp     x8, x9, [x3, #-16]
	stp     x10,x11,[x3, #-32]
	stp     x12,x13,[x0, #16]                                               
	stp     x14,x15,[x0]                                                     
	POP_FRAME                                                    
	ARM64_STACK_EPILOG

                                                                              
                                                                              
                                                                               

0:	ldr     x6,     [x1,#-8]!
	str     x6,     [x4,#-8]!
L_reverseSmallCopy:
	subs    x2,      x2, #8
	b.cs    0b
	adds    x2,      x2, #8
	b.eq    2f
1:	ldrb    w6,     [x1,#-1]!
	strb    w6,     [x4,#-1]!
	subs    x2,      x2, #1
	b.ne    1b
2:	POP_FRAME
	ARM64_STACK_EPILOG


L_return:
	POP_FRAME
	ARM64_STACK_EPILOG
