/*
 * Copyright (c) 2000-2013 Apple Inc. All rights reserved.
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

  
                                                                                           

                                                                                                             

        
                                                   
                                                   
                                                                            
                                                         

         

                                                                                      

                                                                                                                                          

                                       
                    
                                   
                                                                             
                                                                                       
                                                                              

                                                                                                                                         

                                                                                                                                                   

                        
                    
                 
                                    
                                                                                                                                     
                                                                                                        
                                                             
    
 
                 

                                                                              
                    
  

#define MZV_MAGIC           $17185                                                       

	.text

	.globl _WKdm_decompress_new
_WKdm_decompress_new:

	                                                                

	pushq	%rbp
	movq	%rsp, %rbp
	pushq	%r12
	pushq	%r13
	pushq	%rbx

	subq	$(64+8+16), %rsp

    movl    0(%rdi), %eax                                                   
    cmpl    MZV_MAGIC, %eax                                                                
    jne     L_default_decompressor                                      

                                                                       
                                            
    movq    $0, %rax
1:                                                                 
    movq    $0, 0(%rsi, %rax)
    movq    $0, 8(%rsi, %rax)
    movq    $0, 16(%rsi, %rax)
    movq    $0, 24(%rsi, %rax)
    movq    $0, 32(%rsi, %rax)
    movq    $0, 40(%rsi, %rax)
    movq    $0, 48(%rsi, %rax)
    movq    $0, 56(%rsi, %rax)
    addq    $64, %rax
    cmpq    $4096, %rax
    jne     1b

    movq    $4, %r12                                                                
2:
    movl    0(%rdi, %r12), %eax                        
    movzwq  4(%rdi, %r12), %rdx                         
    movl    %eax, 0(%rsi, %rdx)                                                      
    addq    $6, %r12                                             
    cmpl    %ecx, %r12d                                                      
    jne     2b
    jmp     L_done
                                            

L_default_decompressor:

	movq	%rsi, %r12					           
	movq	%rdx, %r13					              

	                                                                            
                                                                                           
#if 1
	movl	$0, 0(%rsp)
	movl	$0, 4(%rsp)
	movl	$0, 8(%rsp)
	movl	$0, 12(%rsp)
	movl	$0, 16(%rsp)
	movl	$0, 20(%rsp)
	movl	$0, 24(%rsp)
	movl	$0, 28(%rsp)
	movl	$0, 32(%rsp)
	movl	$0, 36(%rsp)
	movl	$0, 40(%rsp)
	movl	$0, 44(%rsp)
	movl	$0, 48(%rsp)
	movl	$0, 52(%rsp)
	movl	$0, 56(%rsp)
	movl	$0, 60(%rsp)
#else
	mov		$0x100000001, %rax
	mov		%rax, (%rsp)
	mov		%rax, 8(%rsp)
	mov		%rax, 16(%rsp)
	mov		%rax, 24(%rsp)
	mov		%rax, 32(%rsp)
	mov		%rax, 40(%rsp)
	mov		%rax, 48(%rsp)
	mov		%rax, 56(%rsp)
#endif

	                                                                                    

	leaq	268(%rdi), %r10				                
	leaq	12(%rdi), %rax				                   
	movq	%r13, %rsi					                
	cmpq	%rax, %r10					                                   
	jbe		1f							                                                              
	movq	%r13, %rcx					            
	xorl	%r8d, %r8d					        
	mov		$(50529027<<32)+50529027, %r9
L_WK_unpack_2bits:
	movl	12(%rdi,%r8, 4), %eax
	movl	12(%rdi,%r8, 4), %edx
	shrl	$2, %eax
	shlq	$32, %rax
	orq		%rdx, %rax
	movq	%rax, %rdx
	shrq	$4, %rax
	andq	%r9, %rdx
	andq	%r9, %rax
	incq	%r8							      
	movq	%rdx, (%rcx)
	movq	%rax, 8(%rcx)
	addq	$16, %rcx					                  
	cmpq	$64, %r8					          
	jne		L_WK_unpack_2bits			                          
1:


	                                                                                    

	mov		4(%rdi), %eax				                       
	leaq	(%rdi,%rax,4), %r9			                
	mov		0(%rdi), %eax				                         
	leaq	(%rdi,%rax,4), %r8			                  
	leaq	1024(%r13), %rbx			                
	cmpq	%r8, %r9					                                   
	jbe		1f							                                                              
	leaq	8(%rbx), %rcx				            

	mov		$(252645135<<32)+252645135, %r11
L_WK_unpack_4bits:
	movl	(%r8), %eax					                 
	movl	%eax, %edx					    
	shlq	$28, %rax
	orq		%rdx, %rax
	addq	$4, %r8						              
	andq	%r11, %rax
	movq	%rax, -8(%rcx)
	addq	$8, %rcx					               
	cmpq	%r8, %r9					                                   
	ja		L_WK_unpack_4bits			                                                     


1:

	                                                                                                   

	movl	8(%rdi), %eax				                           
	leaq	(%rdi,%rax,4), %rdi			                    
	leaq	2048(%r13), %r11			                   
	leaq	4094(%r13), %r13			                     
	sub		%r9, %rdi					                                           
	jle		1f							                                            
	movq	%r11, %rcx					                
L_WK_unpack_3_tenbits:
	movl	(%r9), %eax					                          
	movl	$(1023<<10), %edx
	movl	$(1023<<20), %r8d
	andl	%eax, %edx					          
	andl	%eax, %r8d					          
	andq	$1023, %rax
	shll	$6, %edx
	shlq	$12, %r8
	orl		%edx, %eax
	orq		%r8, %rax
	cmp		%r13, %rcx
	je		2f
	mov		%rax, (%rcx)
	jmp		3f
2:	mov		%ax, (%rcx)
3:
	addq	$4, %r9						              
	addq	$6, %rcx					                     
	sub		$4, %rdi
	jg		L_WK_unpack_3_tenbits		                                               
1:


	#define	next_qpos		%rbx
	#define	hash			%r8
	#define	tags_counter	%edi
	#define	dest_buf		%r12
	#define next_full_patt	%r10	

	leaq	_hashLookupTable_new(%rip), hash	                     
	movl	$1024, tags_counter				               
	jmp		L_next

	.align 4,0x90
L_nonpartital:
	jl		L_ZERO_TAG
	cmpb	$2, -1(%rsi)
	je		L_MISS_TAG

L_EXACT_TAG:
	movzbl	(next_qpos), %eax				                    
	incq	next_qpos						              
	decl	tags_counter					                 
	movl	(%rsp,%rax,4), %eax				                       
	movl	%eax, -4(dest_buf)				                
	je		L_done

L_next:
	incq	%rsi							             
	addq	$4, dest_buf
	cmpb	$1, -1(%rsi)
	jne		L_nonpartital

L_PARTIAL_TAG:
	movzbl	(next_qpos),%edx				                    
	incq	next_qpos						              
	movl	(%rsp,%rdx,4), %eax				                       
	andl	$-1024, %eax					                      
	or		(%r11), %ax						                                            
	addq	$2, %r11						                  
	decl	tags_counter					                 
	movl	%eax, (%rsp,%rdx,4)				                                      
	movl	%eax, -4(dest_buf)				                                
	jg		L_next							                                           

L_done:

	                                                      

	addq	$(64+8+16), %rsp
	popq	%rbx
	popq	%r13
	popq	%r12
	leave
	ret

	.align 4,0x90
L_MISS_TAG:
	movl	(next_full_patt), %edx			                      
	movl	(next_full_patt), %eax			                      
	shrl	$10, %edx						        
	addq	$4, next_full_patt				                   
	movzbl	%dl, %edx						                         
	movl	%eax, -4(dest_buf)				                   
	movzbl	(hash,%rdx),%edx				       
	decl	tags_counter					                 
	movl	%eax, (%rsp,%rdx)				                          
	jg		L_next							                  
	jmp		L_done

	.align 4,0x90
L_ZERO_TAG:
	decl	tags_counter					                 
	movl	$0, -4(dest_buf)					                
	jg		L_next							                  
	jmp		L_done


