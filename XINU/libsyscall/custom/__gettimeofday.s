/*
 * Copyright (c) 1999-2016 Apple Inc. All rights reserved.
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

#include "SYS.h"

  
                                                                             
                                                                               
                       
  
                                                                            
                                                                            
        
  
                                                                            
                                                                          
               
   

.private_extern ___gettimeofday_with_mach

#if defined(__i386__)

LABEL(___gettimeofday)
    pushl	$0
    pushl	12(%esp)
    pushl	12(%esp)
    calll	___gettimeofday_with_mach
    addl	$12, %esp
    ret

LABEL(___gettimeofday_with_mach)
    UNIX_SYSCALL_INT_NONAME(gettimeofday,0)
      
                                
                                                                                
                                                                           
       
    cmp     $0, %eax
    je      2f
    mov     4(%esp),%ecx
    mov     %eax,(%ecx)
    mov     %edx,4(%ecx)
    xor     %eax,%eax
2:
    ret

#elif defined(__x86_64__)

__SYSCALL(___gettimeofday_with_mach, gettimeofday, 3)

LABEL(___gettimeofday)
    movq   $0x0, %rdx                           

    UNIX_SYSCALL_NONAME(gettimeofday,0,cerror_nocancel)
      
                                
                                                                                
                                                                           
       
    cmp     $0, %rax
    je      2f
    movq    %rax, (%rdi)
    movl    %edx, 8(%rdi)
    xorl    %eax, %eax
2:
    ret

#elif defined(__arm__)

__SYSCALL2(___gettimeofday_with_mach, gettimeofday, 3, cerror_nocancel)

.text
.align  2
.globl ___gettimeofday
___gettimeofday:
    mov    r2, #0x0
    SYSCALL_NONAME(gettimeofday, 3, cerror_nocancel)
    bx lr

#elif defined(__arm64__)

__SYSCALL2(___gettimeofday_with_mach, gettimeofday, 3, cerror_nocancel)

.text
.align  2
.globl ___gettimeofday
___gettimeofday:
    movz   x2, #0x0
    SYSCALL_NONAME(gettimeofday, 3, cerror_nocancel)
    ret

#else
#error Unsupported architecture
#endif
