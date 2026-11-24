/*
 * Copyright (c) 2003 Apple Computer, Inc. All rights reserved.
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
 */

#define __NO_UNDERSCORES__
#include <i386/asm.h>
#include <assym.s>

Entry(mcount)
        pushq   %rbp            	                       
        movq    %rsp,%rbp
        pushq	%rax			            
        pushf				                       
        cli				                     

	  
	                                
	                                                                     
	  
        mov		%gs,%ax
        test	%ax,%ax
	jz	1f

        movl	%gs:CPU_RUNNING,%eax
        testl	%eax,%eax
	jz	1f

	  
	                                                     
	                                                                 
	  
        movl	%gs:CPU_MCOUNT_OFF,%eax
        testl	%eax,%eax		                     
        jnz	1f

        incl	%gs:CPU_MCOUNT_OFF	                     

        movq    (%rbp),%rax     	                                   
		pushq	%rdi
		pushq	%rsi
		pushq	%rdx
		pushq	%rcx
		pushq	%r8
		pushq	%r9
        movq    8(%rax),%rdi    	                                   
        movq    8(%rbp),%rsi         	                                     

        call	_mcount			                    

		popq	%r9
		popq	%r8
		popq	%rcx
		popq	%rdx
		popq	%rsi
		popq	%rdi

        decl	%gs:CPU_MCOUNT_OFF	                          
1:
        popf				                          
        popq	%rax
        movq    %rbp,%rsp       	                           
        popq    %rbp
        ret
