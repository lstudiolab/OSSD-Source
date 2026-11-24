/*
 * Copyright (c) 2000-2010 Apple Inc. All rights reserved.
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
#include <i386/rtclock_asm.h>
#include <i386/proc_reg.h>
#include <i386/eflags.h>
       
#include <i386/postcode.h>
#include <i386/apic.h>
#include <i386/vmx/vmx_asm.h>
#include <assym.s>

  
                         
  
                          
  
  
ENTRY(ml_get_timebase)

	lfence
	rdtsc
	lfence
        shlq	$32,%rdx 
        orq	%rdx,%rax
			
	ret

  
                                         
  
                                                       
                                                   
  
                                                         
                                                            
                                              
  
                                                               
                                                             
                                                                
                                               
  
                            
  
                                                                       
                                                                  
                                  
  
                                          
                                 
  
   
ENTRY(tmrCvt)
	cmpq	$1,%rsi				                              
	je	1f
	movq	%rdi,%rax
	mulq	%rsi				                         
	shrdq   $32,%rdx,%rax			                      
	ret
1:
	mov	%rdi,%rax
	ret

   
                             
                                           
                                   
   
ENTRY(_rtc_nanotime_adjust)
	movl	RNT_GENERATION(%rsi),%eax	                            
	movl	$0,RNT_GENERATION(%rsi)		                                
	addq	%rdi,RNT_TSC_BASE(%rsi)

	incl	%eax				                     
	jnz	1f
	incl	%eax				                             
1:	movl	%eax,RNT_GENERATION(%rsi)	                       

	ret

  
                                                     
  
                                                                             
                                                                         
                                                                   
  
                                                                              
                                                                               
                                                                   
  
                           
  
                                                                                    
  
                                                                                         
  
                                              
  
                                                                                       
                                                                               
  
                                                           
  
                                                                               
                                   
  
                                                                      
  
                                                                            
                                            
  
                                                                            
                                                                            
                                                                              
                                                                             
                                                                          
                                                                              
                          
  
                                
                                    
  
   
ENTRY(_rtc_nanotime_read)

	PAL_RTC_NANOTIME_READ_FAST()

	ret
    
  
                                           
                                                   
                                                  
  
                                                                            
                                                                                  
                                                                              
                                                                            
   

ENTRY(_rtc_tsc_to_nanoseconds)
	movq    %rdi,%rax			                                          
	movl    RNT_SHIFT(%rsi),%ecx
	movl    RNT_SCALE(%rsi),%edx
	shlq    %cl,%rax			                       
	mulq    %rdx				                                 
	shrdq   $32,%rdx,%rax			                      
	ret
    

  
                                                                 
  
                                                          
                              
                                        
                                                   
   

Entry(call_continuation)

	movq    %rdi, %r12                    
    movq    %rsi, %r13                          
    movq    %rdx, %r14                   

	movq	%gs:CPU_KERNEL_STACK,%rsp	                   
	xorq	%rbp,%rbp			                        

    test    %ecx, %ecx
    jz 1f
    mov     $1, %edi
    call   _ml_set_interrupts_enabled
1:

	movq	%r12,%rcx			                  
	movq	%r13,%rdi			                        
	movq	%r14,%rsi			                 

	call	*%rcx				                       
	movq	%gs:CPU_ACTIVE_THREAD,%rdi
	call	EXT(thread_terminate)


Entry(x86_init_wrapper)
	xor	%rbp, %rbp
	movq	%rsi, %rsp
	callq	*%rdi

#if CONFIG_VMX

  
                                 
                           
   
Entry(__vmxon)
	FRAME
	push	%rdi
	
	mov	$(VMX_FAIL_INVALID), %ecx
	mov	$(VMX_FAIL_VALID), %edx
	mov	$(VMX_SUCCEED), %eax
	vmxon	(%rsp)
	cmovcl 	%ecx, %eax	                    
	cmovzl	%edx, %eax	                    

	pop	%rdi
	EMARF
	ret

  
                                  
                      
   
Entry(__vmxoff)
	FRAME
	
	mov	$(VMX_FAIL_INVALID), %ecx
	mov	$(VMX_FAIL_VALID), %edx
	mov	$(VMX_SUCCEED), %eax
	vmxoff
	cmovcl 	%ecx, %eax	                    
	cmovzl	%edx, %eax	                    

	EMARF
	ret

#endif                 

  
                           
                                  
                                 
                                  
                              
   
Entry(do_mfence)
	mfence
	ret
