/*
 * Copyright (c) 1999-2016 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
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
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved
 *
 * HISTORY
 *  20-Apr-92    Bruce Martin (bmartin@next.com)
 *      Created from M68K sources.
 */

  
                                 
  
                
                                   
                   
             
                                         
                                             
  
   

#include <architecture/i386/asm_help.h>

#define JB_RBX        0
#define JB_RBP        8
#define JB_RSP        16
#define JB_R12        24
#define JB_R13        32
#define JB_R14        40
#define JB_R15        48
#define JB_RIP        56
#define JB_RFLAGS     64
#define JB_MXCSR      72
#define JB_FPCONTROL  76
#define JB_MASK       80

LEAF(__setjmp, 0)
                                                  

                               
        movq    %rbx, JB_RBX(%rdi)
        movq    %rbp, JB_RBP(%rdi)
        movq    %r12, JB_R12(%rdi)
        movq    %r13, JB_R13(%rdi)
        movq    %r14, JB_R14(%rdi)
        movq    %r15, JB_R15(%rdi)

                                                       
        movq    (%rsp), %rax
        movq    %rax, JB_RIP(%rdi)
                                                        
        leaq    8(%rsp), %rax
        movq    %rax, JB_RSP(%rdi)

                               
        fnstcw  JB_FPCONTROL(%rdi)

                     
        stmxcsr JB_MXCSR(%rdi)

                   
        xorl    %eax, %eax
        ret


LEAF(__longjmp, 0)
        fninit                                                    
                                                  
                                   
        movl    %esi, %eax
        testl   %esi, %esi
        jnz     1f
        incl    %eax

                            
1:
        movq    JB_RBX(%rdi), %rbx
        movq    JB_RBP(%rdi), %rbp
        movq    JB_RSP(%rdi), %rsp
        movq    JB_R12(%rdi), %r12
        movq    JB_R13(%rdi), %r13
        movq    JB_R14(%rdi), %r14
        movq    JB_R15(%rdi), %r15

                                  
        fldcw   JB_FPCONTROL(%rdi)

                        
        ldmxcsr JB_MXCSR(%rdi)


                                
        cld

        jmp             *JB_RIP(%rdi)

