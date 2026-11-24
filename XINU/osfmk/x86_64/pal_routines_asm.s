/*
 * Copyright (c) 2009 Apple Inc. All rights reserved.
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
 
#include <i386/asm.h>
#include <i386/asm64.h>
       
#include <assym.s>

  
                                               
                                            
   
#define COPY_STACK(src, count, tmpindex) \
	mov	$0, tmpindex	                                 
1: \
	mov	0(src,tmpindex,1), %rax	                                              
	mov	%rax, 0(%rsp,tmpindex,1)                       
	add	$8, tmpindex		                            
	cmp	count, tmpindex		                                        
	jne 1b
	
  
     
                                              
                                                              
                                                 
                                                       

                                                   
                                                       
                                                           
                                                        
              
  
ENTRY(_pal_efi_call_in_64bit_mode_asm)
	FRAME

	                                 
	push	%rbx
	push	%r12
	push	%r13
	push	%r14
	push	%r15

	                                             
	push	%rsi
	push	%rcx

	sub	$8, %rsp	                               
				                     
				                              
				                     
	sub	%rcx, %rsp	                                  

	COPY_STACK(%rdx, %rcx, %r8)

	                                      
	mov	0(%rsi),  %rcx
	mov	8(%rsi),  %rdx
	mov	16(%rsi), %r8
	mov	24(%rsi), %r9
	mov	32(%rsi), %rax

					                          
	call	*%rdi			                      

	mov	-48(%rbp), %rsi		                            
	mov	%rax, 32(%rsi)		                   

	mov	-56(%rbp), %rcx	                          
	add	%rcx, %rsp	                            
	add	$8, %rsp	                           

	pop	%rcx
	pop	%rsi
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%rbx

	EMARF
	ret

  
     
                                              
                                                              
                                                 
                                                       

  
ENTRY(_pal_efi_call_in_32bit_mode_asm)
	FRAME

	                                 
	push	%rbx
	push	%r12
	push	%r13
	push	%r14
	push	%r15

	                                             
	push	%rsi
	push	%rcx

	push	%rbp	                                             
				                     
				                              
				                     
	sub	%rcx, %rsp	                                  

	COPY_STACK(%rdx, %rcx, %r8)

	  
                                                  
                                                        
                                                   
    
	ENTER_COMPAT_MODE()

	call	*%edi			                      

	ENTER_64BIT_MODE()

	mov	-48(%rbp), %rsi		                            
	mov	%rax, 32(%rsi)		                   

	mov	-56(%rbp), %rcx	                          
	add	%rcx, %rsp	                            
	pop	%rbp		                                       
				                                              
				                                             
	pop	%rcx
	pop	%rsi
	pop	%r15
	pop	%r14
	pop	%r13
	pop	%r12
	pop	%rbx

	EMARF
	ret



  
                                
                                 
                                  
                                   
                                   
                                  
   
ENTRY(_pal_rtc_nanotime_store)
	movl	RNT_GENERATION(%r8),%eax	                            
	movl	$0,RNT_GENERATION(%r8)		                                
	movq	%rdi,RNT_TSC_BASE(%r8)
	movq	%rsi,RNT_NS_BASE(%r8)
	movl	%edx,RNT_SCALE(%r8)
	movl	%ecx,RNT_SHIFT(%r8)

	incl	%eax				                     
	jnz	1f
	incl	%eax				                             
1:	movl	%eax,RNT_GENERATION(%r8)	                       

	ret

