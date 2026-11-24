/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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

.code64
.globl _compat_mode_trampoline
_compat_mode_trampoline:
	  
                                                                       
                        
                                       
                                         
                                      
    
	movq	%rsp, %rax
	movq	%rsi, %rsp
	pushq	%rax		                               
	leaq	1f(%rip), %rax
	movq	%rdx, %r9
	xorq	%rdx, %rdx
	movw	%cs, %dx
	shlq	$32, %rdx
	orq	%rdx, %rax
	movq	%r9, %rdx
	  
                                                                  
                                                              
                                          
    
	pushq	%rbp
	pushq	%rbx
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15

	pushq	%r8		                                                 
	pushq	%rcx		                                                       
	pushq	%rdx		                             
	pushq	%rax		                                                     

	ljmpq	*(%rdi)
1:
	  
                                                      
                                              
    
	addq	$0x18, %rsp

	                                    
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbx
	popq	%rbp

	popq	%rsp
	retq


.code32
.globl _code_32
.align 12
_code_32:
	  
                                                                                  
   
                          
                             
                            
                                                              
                                                                                        
                                                                    
                                                                                  
                                            
    
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	call	1f
1:
	popl	%ebx		                                               
	subl	$8, %esp

	movl	0x1c(%ebp), %eax

	                                       
	movl	%eax, -8(%ebp)
	movl	8(%ebp), %eax	                                           
	movl	%eax, -4(%ebp)

	pushl	$0	                         
	pushl	0x18(%ebp)	                                       
	pushl	0x14(%ebp)	                                      

	  
                                                                        
                                                                        
                                                                        
                                            
    
	leal	2f - 1b, %eax
	addl	%ebx, %eax

	pushl	$0
	pushl	%cs
	pushl	$0
	pushl	%eax

	  
                                                                   
                                                                   
                             
    
	ljmpl	*-8(%ebp)	                                          
2:
	  
                                                                   
    
	addl	$12, %esp

	  
                                                                   
                                                                
                                                               
                                                    
    
	xorl	%eax, %eax
	div	%eax

.globl _first_invalid_opcode
_first_invalid_opcode:
	  
                                                                     
          
    
	leal	3f - 1b, %edx
	addl	%ebx, %edx		                                        
	pushl	%ecx
	movl	%esp, %ecx		                                   
	sysenter
3:
	popl	%ecx

	  
                                                        
    
	int $0x80
	int $0x81
.globl _last_invalid_opcode
_last_invalid_opcode:
	int $0x82

	  
                                                              
                                                                   
                                                                
          
    
	movl	0xc(%ebp), %eax
	incl	%eax
	addl	$8, %esp
	popl	%ebx
	popl	%ebp
	lret

.code64

.globl _thunk64
_thunk64:
	  
                                                         
                                                        
                                                                                   
                                                             
   
    
.globl _thunk64_movabs
_thunk64_movabs:
	movabs	$0xdeadbeeffeedface, %rax
	jmpq	*%rax


.globl _compat_mode_trampoline_len
_compat_mode_trampoline_len:
	.long   (. - _compat_mode_trampoline)


.globl _long_mode_trampoline
_long_mode_trampoline:
	  
                                                               
   
                                                   
                                                  
                                                 
                                                  
                                             
                                                          
                                                           
                                                           
                                                            
                                                          
                                                           
                                                           
                                                            
   
                                                         
                                  
    
	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%rdi
	pushq	%rsi
	pushq	%rcx
	movl	0x20(%rbp), %eax

	testl	%eax, %eax
	jz	5f

	movq	0x24(%rbp), %rdi
	decl	%eax

2:
	testl	%eax, %eax
	jz	5f

	movq	0x2c(%rbp), %rsi
	decl	%eax

3:
	testl	%eax, %eax
	jz	5f

	movq	0x34(%rbp), %rdx
	decl	%eax

4:
	testl	%eax, %eax
	jnz	1f			                                                         

	movq	0x3c(%rbp), %rcx

5:	                             
	                                                                                             	

	pushq	%rbx
	movq	%rsp, %rbx
	subq	$0x10, %rsp
	andq	$0xffffffffffffffe0, %rsp

	callq	*0x18(%rbp)
	movq	%rbx, %rsp
	popq	%rbx
1:
	popq	%rcx
	popq	%rsi
	popq	%rdi
	popq	%rbp
	lretq
