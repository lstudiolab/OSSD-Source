/*
 * Copyright (c) 2011 Apple, Inc. All rights reserved.
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
 
#include <arm/arch.h>
.syntax unified
.code 32
.globl _strnlen

#define addr r0
#define maxl r1
#define temp r2
#define mask r3
#define save ip
#define word lr
#define byte lr
#define indx r0

.macro IfHS_and_WordDoesntContainNUL_SetZ
#if defined _ARM_ARCH_6
                                                                       
                                                                      
                                                                        
                                                                        
       
    uqsub8  temp,   mask,   word
    tsths   temp,           temp
#else
                                                                        
                                                          
  
                          
  
                                                                       
                                                                     
                    
    sub     temp,   word,   mask
    bic     temp,   temp,   word
    tsths   temp,           mask, lsl #7
#endif
.endm

.text
.align 3
.long 0x0                     
.long 0x01010101                                   
_strnlen:
                                                                        
                                                                       
                                                                      
                                                                
    push    {r7,lr}
    mov     r7,     sp
    ldr     mask,   (_strnlen-4)
    add		save,   addr,   #4
    subs    maxl,   maxl,   #4
    blo     L_bytewiseSearch

                                                                        
                                                                         
                                                                        
                    
    and     temp,   addr,   #3
    bic     addr,   addr,   #3
    add     maxl,   maxl,   temp
    lsl     temp,   temp,   #3
    ldr     word,  [addr],  #4
    rsb     temp,   temp,   #32
    orr     word,   word,   mask, lsr temp

    subs    maxl,   maxl,   #4
    IfHS_and_WordDoesntContainNUL_SetZ
    bne     1f

.align 4
0:  ldr     word,  [addr],  #4
    subs    maxl,   maxl,   #4
    IfHS_and_WordDoesntContainNUL_SetZ
    beq     0b

.align 4
                                                                   
                                                                          
                                                                      
                                                                     
                                                                     
                               
1:  tst     temp,           temp
    beq     L_bytewiseSearch

                                                                        
                                                                      
                                                           
    sub     indx,   addr,   save
#if defined _ARM_ARCH_6
                                                                        
                                          
    rev     temp,           temp
    clz     temp,           temp
    add     indx,   indx,   temp, lsr #3
#else
                                                                   
                                                       
    tst     word,           #0x000000ff
    addne   indx,           #1
    tstne   word,           #0x0000ff00
    addne   indx,           #1
    tstne   word,           #0x00ff0000
    addne   indx,           #1
#endif
    pop     {r7,pc}

.align 4
L_bytewiseSearch:
                                                                        
                                                                      
                                                                     
                                                                      
                                                                     
                       
    adds    maxl,   maxl,   #4
    sub     save,   save,   #4
    beq     1f
0:  ldrb    byte,          [addr]
    cmp     byte,           #0
    addhi   addr,           #1
    subshi  maxl,           #1
    bhi     0b
1:  sub     indx,   addr,   save
    pop     {r7,pc}
