/*
 * Copyright (c) 2010-2012 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/*
 * @APPLE_FREE_COPYRIGHT@
 */
  
                              
                                                
   

#ifndef _PAL_RTCLOCK_ASM_NATIVE_H_
#define _PAL_RTCLOCK_ASM_NATIVE_H_

  
                                                                          
  
  
                                                                              
                                                                               
                                                                   
  
                           
  
                                                                                    
  
                                                                                         
  
                                              
  
                                                                                       
                                                                               
  
                                                           
  
                                       
                        
   
#define PAL_RTC_NANOTIME_READ_FAST()					  \
0:	movl	RNT_GENERATION(%rdi),%esi				; \
	test        %esi,%esi		                    		; \
        jz        0b			                  		; \
	lfence								; \
	rdtsc								; \
	shlq	$32,%rdx						; \
	movl    RNT_SHIFT(%rdi),%ecx					; \
	orq	%rdx,%rax			                 	; \
	subq	RNT_TSC_BASE(%rdi),%rax		                    	; \
	shlq    %cl,%rax						; \
	movl	RNT_SCALE(%rdi),%ecx					; \
	mulq	%rcx				                   	; \
	shrdq	$32,%rdx,%rax			                      	; \
	addq	RNT_NS_BASE(%rdi),%rax		                 	; \
	cmpl	RNT_GENERATION(%rdi),%esi	                        ; \
	jne	0b

#define PAL_RTC_NANOTIME_READ_NOBARRIER()					  \
0:	movl	RNT_GENERATION(%rdi),%esi				; \
	test        %esi,%esi		                    		; \
        jz        0b			                  		; \
	rdtsc								; \
	shlq	$32,%rdx						; \
	movl    RNT_SHIFT(%rdi),%ecx					; \
	orq	%rdx,%rax			                 	; \
	subq	RNT_TSC_BASE(%rdi),%rax		                    	; \
	shlq    %cl,%rax						; \
	movl	RNT_SCALE(%rdi),%ecx					; \
	mulq	%rcx				                   	; \
	shrdq	$32,%rdx,%rax			                      	; \
	addq	RNT_NS_BASE(%rdi),%rax		                 	; \
	cmpl	RNT_GENERATION(%rdi),%esi	                        ; \
	jne	0b

#endif                                 
