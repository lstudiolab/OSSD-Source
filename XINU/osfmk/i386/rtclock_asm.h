/*
 * Copyright (c) 2004-2012 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/*
 * @APPLE_FREE_COPYRIGHT@
 */
  
                       
                                                                
                      
   

#ifndef _I386_RTCLOCK_H_
#define _I386_RTCLOCK_H_

#include <i386/pal_rtclock_asm.h>

  
                             
                                                                            
                                                       
                                                
                                    
  
                               
   
#define NANOTIME							       \
	movq	%gs:CPU_NANOTIME,%rdi					     ; \
	PAL_RTC_NANOTIME_READ_FAST()

  
                                                                         
   
#define TIMER_UPDATE(treg,reg,offset)					       \
	addq	reg,(offset)+TIMER_ALL(treg)		               

  
                                             
                                     
   
#define TIMER_EVENT(old,new)						       \
	NANOTIME				                             ; \
	movq	%rax,%rsi			                    	     ; \
	movq	%gs:CPU_ACTIVE_THREAD,%rcx	                	     ; \
	subq	(old##_TIMER)+TIMER_TSTAMP(%rcx),%rax	                     ; \
	TIMER_UPDATE(%rcx,%rax,old##_TIMER)	                  	     ; \
	leaq	(new##_TIMER)(%rcx),%rcx	                             ; \
	movq	%rsi,TIMER_TSTAMP(%rcx)		                   	     ; \
	movq	%gs:CPU_PROCESSOR,%rdx		                   	     ; \
	movq	%rcx,THREAD_TIMER(%rdx)		                       	     ; \
	movq	%rsi,%rax			                       	     ; \
	subq	(old##_STATE)+TIMER_TSTAMP(%rdx),%rax	                     ; \
	TIMER_UPDATE(%rdx,%rax,old##_STATE)	                  	     ; \
	leaq	(new##_STATE)(%rdx),%rcx 	                             ; \
	movq	%rcx,CURRENT_STATE(%rdx)	                       	     ; \
	movq	%rsi,TIMER_TSTAMP(%rcx)		                   

  
                                  
                                     
   
#define	TIME_TRAP_UENTRY	TIMER_EVENT(USER,SYSTEM)

  
                                 
                                     
   
#define	TIME_TRAP_UEXIT		TIMER_EVENT(SYSTEM,USER)

  
                                  
                                     
                                       
   
#define	TIME_INT_ENTRY							       \
	NANOTIME				                      	     ; \
	movq	%rax,%gs:CPU_INT_EVENT_TIME	                      	     ; \
	movq	%rax,%rsi			                    	     ; \
	movq	%gs:CPU_PROCESSOR,%rdx		                   	     ; \
	movq 	THREAD_TIMER(%rdx),%rcx		                       	     ; \
	subq	TIMER_TSTAMP(%rcx),%rax		                     	     ; \
	TIMER_UPDATE(%rcx,%rax,0)		                  	     ; \
	movq	KERNEL_TIMER(%rdx),%rcx		                      	     ; \
	movq	%rsi,TIMER_TSTAMP(%rcx)		                   	     ; \
	movq	%rsi,%rax			                       	     ; \
	movq	CURRENT_STATE(%rdx),%rcx	                       	     ; \
	pushq	%rcx				                	     ; \
	subq	TIMER_TSTAMP(%rcx),%rax		                     	     ; \
	TIMER_UPDATE(%rcx,%rax,0)		                  	     ; \
	leaq	IDLE_STATE(%rdx),%rax		                    	     ; \
	cmpq	%rax,%rcx			                     	     ; \
	je	0f				                   	     ; \
	leaq	SYSTEM_STATE(%rdx),%rcx		                      	     ; \
	movq	%rcx,CURRENT_STATE(%rdx)	                       	     ; \
0:	movq	%rsi,TIMER_TSTAMP(%rcx)		                   

  
                                 
                                     
                                            
   
#define	TIME_INT_EXIT							       \
	NANOTIME				                      	     ; \
	movq	%rax,%rsi			                    	     ; \
	movq	%gs:CPU_PROCESSOR,%rdx		                   	     ; \
	movq	KERNEL_TIMER(%rdx),%rcx		                      	     ; \
	subq	TIMER_TSTAMP(%rcx),%rax		                             ; \
	TIMER_UPDATE(%rcx,%rax,0)		                  	     ; \
	movq	THREAD_TIMER(%rdx),%rcx		                       	     ; \
	movq	%rsi,TIMER_TSTAMP(%rcx)		                   	     ; \
	movq	%rsi,%rax			                       	     ; \
	movq	CURRENT_STATE(%rdx),%rcx	                       	     ; \
	subq	TIMER_TSTAMP(%rcx),%rax		                     	     ; \
	TIMER_UPDATE(%rcx,%rax,0)		                  	     ; \
	popq	%rcx				                   	     ; \
	movq	%rcx,CURRENT_STATE(%rdx)	                       	     ; \
	movq	%rsi,TIMER_TSTAMP(%rcx)		                   	     ; \
	movq	$0,%gs:CPU_INT_EVENT_TIME	                                


  
                              
                                                    
                                                      
   
#define TASK_VTIMER_CHECK(task_reg,thread_reg)				       \
	cmpl	$0,TASK_VTIMERS(task_reg)				     ; \
	jz	1f							     ; \
	orl	$(AST_BSD),%gs:CPU_PENDING_AST	                     	     ; \
	lock								     ; \
	orl	$(AST_BSD),TH_AST(thread_reg)	                     	     ; \
1:									     ; \

#endif                       
