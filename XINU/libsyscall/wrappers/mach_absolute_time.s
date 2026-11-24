/*
 * Copyright (c) 2003-2007 Apple Inc. All rights reserved.
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

#include <sys/appleapiopts.h>
#include <machine/cpu_capabilities.h>

#if defined(__i386__)

                                         
  
                           
  
                                                                                    
  
                                                                                         
  
                                               
  
                                                                                     
                                                                               
  
                                                           
   

	.globl _mach_absolute_time
_mach_absolute_time:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	pushl	%ebx

0:
	movl	_COMM_PAGE_NT_GENERATION,%esi	                                         
	testl	%esi,%esi			                                         
	jz	0b

	lfence
	rdtsc					                          
	lfence

	subl	_COMM_PAGE_NT_TSC_BASE,%eax
	sbbl	_COMM_PAGE_NT_TSC_BASE+4,%edx
    
      
                                                                             
                                                                      
                                                                               
       
	movl    _COMM_PAGE_NT_SHIFT,%ecx
	shldl   %cl,%eax,%edx			                                           
	shll    %cl,%eax			                                                                

	movl	_COMM_PAGE_NT_SCALE,%ecx

	movl	%edx,%ebx
	mull	%ecx
	movl	%ebx,%eax
	movl	%edx,%ebx
	mull	%ecx
	addl	%ebx,%eax
	adcl	$0,%edx

	addl	_COMM_PAGE_NT_NS_BASE,%eax
	adcl	_COMM_PAGE_NT_NS_BASE+4,%edx

	cmpl	_COMM_PAGE_NT_GENERATION,%esi	                                  
	jne	0b				                            

	popl	%ebx
	popl	%esi
	popl	%ebp
	ret

#elif defined(__x86_64__)

  
                                                                              
  
                           
  
                                                                                    
  
                                                                                         
  
                                             
  
                                                                                       
                                                                               
  
                                                           
  
   
	.globl	_mach_absolute_time
_mach_absolute_time:
	pushq	%rbp				                                
	movq	%rsp,%rbp
	movq	$(_COMM_PAGE_TIME_DATA_START),%rsi
1:
	movl	_NT_GENERATION(%rsi),%r8d	                 
	testl	%r8d,%r8d			                                 
	jz      1b				                          
	lfence
	rdtsc					                 
	lfence
	shlq	$32,%rdx			                                            
	orq     %rdx,%rax

      
                                                                             
                                                                                
                                                                             
                                                                              
       
	movl    _NT_SHIFT(%rsi),%ecx
	andl    $0x1F,%ecx			                                           
	subq	_NT_TSC_BASE(%rsi), %rax	                          
	shlq    %cl,%rax			                                      
	movl	_NT_SCALE(%rsi),%ecx
	mulq	%rcx				                                               
	shrdq	$32,%rdx,%rax			                  
	addq	_NT_NS_BASE(%rsi),%rax		                                               
	
	cmpl	_NT_GENERATION(%rsi),%r8d	                                          
	jne     1b
	popq	%rbp
	ret

#elif defined(__arm__)

#include <mach/arm/syscall_sw.h>
#include <mach/arm/traps.h>

  
                                                                                     
                                                                                   
                                                                                   
                                                                     
  
                                                                                   
                                                                               
                                                                                 
                                                                               
                                  
  
                                                                                     
                                                                                    
                                                                                 
                                 
   
	.text
	.align 2
	.globl _mach_absolute_time
_mach_absolute_time:
	movw	ip, #((_COMM_PAGE_TIMEBASE_OFFSET) & 0x0000FFFF)
	movt	ip, #(((_COMM_PAGE_TIMEBASE_OFFSET) >> 16) & 0x0000FFFF)
	ldrb	r0, [ip, #((_COMM_PAGE_USER_TIMEBASE) - (_COMM_PAGE_TIMEBASE_OFFSET))]
	cmp	r0, #USER_TIMEBASE_NONE		                                 
	beq	_mach_absolute_time_kernel	                           
	isb					                                             
						                                                      
	push	{r4, r5, r7, lr}		               
	add	r7, sp, #8
L_mach_absolute_time_user:
	ldr	r4, [ip]			                       
	ldr	r5, [ip, #4]			                        
	mrrc	p15, 0, r3, r1, c14		                           
	mrrc	p15, 0, r0, r3, c14		                          
	mrrc	p15, 0, r3, r2, c14		                           
	cmp	r1, r2				                            
	bne	L_mach_absolute_time_user	                                
	ldr	r2, [ip]			                       
	ldr	r3, [ip, #4]			                        
	eor	r4, r2				                               
	eor	r5, r3
	orrs	r5, r4
	bne	L_mach_absolute_time_user	                             
	adds	r0, r0, r2			                               
	adcs	r1, r1, r3
	pop	{r4, r5, r7, pc}		                

	.text
	.align 2
	.globl _mach_absolute_time_kernel
_mach_absolute_time_kernel:
	mov	r12, #MACH_ARM_TRAP_ABSTIME	                            
	swi	#SWI_SYSCALL
	bx	lr

	.text
	.align 2
	.globl _mach_continuous_time_kernel
_mach_continuous_time_kernel:
	mov	r12, #MACH_ARM_TRAP_CONTTIME	                            
	swi	#SWI_SYSCALL
	bx	lr

#elif defined(__arm64__)

#include <mach/arm/syscall_sw.h>
#include <mach/arm/traps.h>

  
                                                                                     
                                                                                   
                                                                                   
                                                                     
  
                                                                                 
                                                                                   
                                                                                      
                                                               
   
	.text
	.align 2
	.globl _mach_absolute_time
_mach_absolute_time:
	movk	x3, #(((_COMM_PAGE_TIMEBASE_OFFSET) >> 48) & 0x000000000000FFFF), lsl #48
	movk	x3, #(((_COMM_PAGE_TIMEBASE_OFFSET) >> 32) & 0x000000000000FFFF), lsl #32
	movk	x3, #(((_COMM_PAGE_TIMEBASE_OFFSET) >> 16) & 0x000000000000FFFF), lsl #16
	movk	x3, #((_COMM_PAGE_TIMEBASE_OFFSET) & 0x000000000000FFFF)
	ldrb	w2, [x3, #((_COMM_PAGE_USER_TIMEBASE) - (_COMM_PAGE_TIMEBASE_OFFSET))]
	cmp	x2, #USER_TIMEBASE_NONE		                                 
	b.eq	_mach_absolute_time_kernel	                           
	isb					                                             
						                                                      
L_mach_absolute_time_user:
	ldr	x1, [x3]			                  
	mrs	x0, CNTVCT_EL0			                    
	ldr	x2, [x3]			                  
	cmp	x1, x2				                               
	b.ne	L_mach_absolute_time_user	                             
	add	x0, x0, x1			                               
	ret



	.text
	.align 2
	.globl _mach_absolute_time_kernel
_mach_absolute_time_kernel:
	mov	w16, #MACH_ARM_TRAP_ABSTIME	                            
	svc	#SWI_SYSCALL
	ret

	.text
	.align 2
	.globl _mach_continuous_time_kernel
_mach_continuous_time_kernel:
	mov	w16, #MACH_ARM_TRAP_CONTTIME	                            
	svc	#SWI_SYSCALL
	ret

#else
#error Unsupported architecture
#endif
