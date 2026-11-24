/*
 * Copyright (c) 2008 Apple Computer, Inc. All rights reserved.
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

#include <sys/appleapiopts.h>
#include <machine/cpu_capabilities.h>
#include <machine/commpage.h>
#include <mach/i386/syscall_sw.h>


                             
  
                                                                                     
                                                                              
                                                                                     
                                                              
  
                                                                                    
                                                                                
                                                                                      
                                                                                  
                                                                       
                                                                                  
  
                                                                               
                                                                               
                                                  
  
                                                                                    
                                                                                        
                                                                         
                                                                              
   

                                                        
fifo_queue_dummy_symbol:	

  
                            
                                                          
                                                         
                               
                     
  
                                                                               
   


                                                                       
                                                 
                        
                        
                   
                                 
   
COMMPAGE_FUNCTION_START(preempt, 32, 4)
	popl	%edx		                     
	movl	%esp,%ecx	                      
	movl	$(-58),%eax	                   
	xorl	%ebx,%ebx	                                  
	sysenter
COMMPAGE_DESCRIPTOR(preempt,_COMM_PAGE_PREEMPT)


                                                                
                                                                    
                  
                                     
                                                         
                 
   
COMMPAGE_FUNCTION_START(backoff, 32, 4)
	testl	%ebx,%ebx	                                  
	jz	1f		     
	xorl	%ebx,%ebx	                  
	pushl	%edx		                                        
	pushl	%ecx
	COMMPAGE_CALL(_COMM_PAGE_PREEMPT,_COMM_PAGE_BACKOFF,backoff)
	popl	%ecx
	popl	%edx
1:
	pause			                       
	cmpl	$0,8(%edi)	                     
	jnz	1b		                      
	ret			                                   
COMMPAGE_DESCRIPTOR(backoff,_COMM_PAGE_BACKOFF)


                                                
                                     
                                   
                                          
                                                                    
   
 
COMMPAGE_FUNCTION_START(pfz_enqueue, 32, 4)
	movl	    $0,(%edx,%esi)                                     
1:
	xorl	    %eax, %eax
	orl	    $-1, %ecx
	lock
	cmpxchgl    %ecx, 8(%edi)                              
	jz	    2f		             
	
	pause
	xorl	    %eax, %eax
	lock
	cmpxchgl    %ecx, 8(%edi)                                       
	jz	    2f		             
	
	pause
	xorl	    %eax, %eax
	lock
	cmpxchgl    %ecx, 8(%edi)                                       
	jz	    2f		             
	
	COMMPAGE_CALL(_COMM_PAGE_BACKOFF,_COMM_PAGE_PFZ_ENQUEUE,pfz_enqueue)
	jmp	    1b		                        
2:
	movl	    4(%edi),%eax                                   
	testl	    %eax,%eax	              
	jnz	    3f		         
	movl	    %esi,(%edi)	                                       
	jmp	    4f
3:
	movl	    %esi,(%edx,%eax)                                  
4:
	movl	    %esi,4(%edi)                                    
	movl	    $0,8(%edi)	                      
	COMMPAGE_JMP(_COMM_PAGE_RET,_COMM_PAGE_PFZ_ENQUEUE,pfz_enqueue)
COMMPAGE_DESCRIPTOR(pfz_enqueue,_COMM_PAGE_PFZ_ENQUEUE)


                                                
                                     
                                          
                                                                    
  
                                            
   
 
COMMPAGE_FUNCTION_START(pfz_dequeue, 32, 4)
1:
	xorl	    %eax, %eax
	orl	    $-1, %ecx
	lock
	cmpxchgl    %ecx, 8(%edi)                              
	jz	    2f		             
	
	pause
	xorl	    %eax, %eax
	lock
	cmpxchgl    %ecx, 8(%edi)                                       
	jz	    2f		             
	
	pause
	xorl	    %eax, %eax
	lock
	cmpxchgl    %ecx, 8(%edi)                                       
	jz	    2f		             
	
	COMMPAGE_CALL(_COMM_PAGE_BACKOFF,_COMM_PAGE_PFZ_DEQUEUE,pfz_dequeue)
	jmp	    1b		                        
2:
	movl	    (%edi),%eax	                                    
	testl	    %eax,%eax	              
	jz	    4f		          
	movl	    (%edx,%eax),%esi                              
	testl	    %esi,%esi	                              
	jnz	    3f		          
	movl	    %esi,4(%edi)                                   
3:
	movl	    %esi,(%edi)	                                     
4:
	movl	    $0,8(%edi)	                      
	COMMPAGE_JMP(_COMM_PAGE_RET,_COMM_PAGE_PFZ_DEQUEUE,pfz_dequeue)
COMMPAGE_DESCRIPTOR(pfz_dequeue,_COMM_PAGE_PFZ_DEQUEUE)


COMMPAGE_FUNCTION_START(ret, 32, 4)
	ret
COMMPAGE_DESCRIPTOR(ret,_COMM_PAGE_RET)



                                                                             


  
                            
                                                          
                                                         
                               
                     
  
                                                                               
   


                                                                       
                                                              
  
                                                                     
   
COMMPAGE_FUNCTION_START(preempt_64, 64, 4)
	pushq	%rax
	pushq	%rcx
	pushq	%r11
	movl	$(SYSCALL_CONSTRUCT_MACH(58)),%eax	                   
	xorl	%ebx,%ebx
	syscall
	popq	%r11
	popq	%rcx
	popq	%rax
	ret
COMMPAGE_DESCRIPTOR(preempt_64,_COMM_PAGE_PREEMPT)


                                                                
                                                                    
                  
                                     
                                                         
              
   
COMMPAGE_FUNCTION_START(backoff_64, 64, 4)
	testl	%ebx,%ebx	                                  
	jz	1f		     
	COMMPAGE_CALL(_COMM_PAGE_PREEMPT,_COMM_PAGE_BACKOFF,backoff_64)
1:
	pause			                       
	cmpl	$0,16(%rdi)	                     
	jnz	1b		                      
	ret			                                   
COMMPAGE_DESCRIPTOR(backoff_64,_COMM_PAGE_BACKOFF)


                                                
                                     
                                       
                                          
                                                                    
   
 
COMMPAGE_FUNCTION_START(pfz_enqueue_64, 64, 4)
	movq	    $0,(%rdx,%rsi)                                     
1:
	xorl	    %eax, %eax
	orl	    $-1, %ecx
	lock
	cmpxchgl    %ecx,16(%rdi)                              
	jz	    2f		             
	
	pause
	xorl	    %eax, %eax
	lock
	cmpxchgl    %ecx,16(%rdi)                                       
	jz	    2f		             
	
	pause
	xorl	    %eax, %eax
	lock
	cmpxchgl    %ecx,16(%rdi)                                       
	jz	    2f		             
	
	COMMPAGE_CALL(_COMM_PAGE_BACKOFF,_COMM_PAGE_PFZ_ENQUEUE,pfz_enqueue_64)
	jmp	    1b		                        
2:
	movq	    8(%rdi),%rax                                   
	testq	    %rax,%rax	              
	jnz	    3f		         
	movq	    %rsi,(%rdi)	                                       
	jmp	    4f
3:
	movq	    %rsi,(%rdx,%rax)                                  
4:
	movq	    %rsi,8(%rdi)                                    
	movl	    $0,16(%rdi)	                      
	COMMPAGE_JMP(_COMM_PAGE_RET,_COMM_PAGE_PFZ_ENQUEUE,pfz_enqueue_64)
COMMPAGE_DESCRIPTOR(pfz_enqueue_64,_COMM_PAGE_PFZ_ENQUEUE)



                                                
                                     
                                          
                                                                    
  
                                            
   
 
COMMPAGE_FUNCTION_START(pfz_dequeue_64, 64, 4)
1:
	xorl	    %eax, %eax
	orl	    $-1, %ecx
	lock
	cmpxchgl    %ecx,16(%rdi)                              
	jz	    2f		             
	
	pause
	xorl	    %eax, %eax
	lock
	cmpxchgl    %ecx,16(%rdi)                                       
	jz	    2f		             
	
	pause
	xorl	    %eax, %eax
	lock
	cmpxchgl    %ecx,16(%rdi)                                       
	jz	    2f		             
	
	COMMPAGE_CALL(_COMM_PAGE_BACKOFF,_COMM_PAGE_PFZ_DEQUEUE,pfz_dequeue_64)
	jmp	    1b		                        
2:
	movq	    (%rdi),%rax	                                    
	testq	    %rax,%rax	              
	jz	    4f		          
	movq	    (%rdx,%rax),%rsi                              
	testq	    %rsi,%rsi	                              
	jnz	    3f		          
	movq	    %rsi,8(%rdi)                                        
3:
	movq	    %rsi,(%rdi)	                                     
4:
	movl	    $0,16(%rdi)	                      
	COMMPAGE_JMP(_COMM_PAGE_RET,_COMM_PAGE_PFZ_DEQUEUE,pfz_dequeue_64)
COMMPAGE_DESCRIPTOR(pfz_dequeue_64,_COMM_PAGE_PFZ_DEQUEUE)

COMMPAGE_FUNCTION_START(ret_64, 64, 4)
	ret
COMMPAGE_DESCRIPTOR(ret_64,_COMM_PAGE_RET)
