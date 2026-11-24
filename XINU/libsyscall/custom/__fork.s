/*
 * Copyright (c) 1999-2010 Apple Inc. All rights reserved.
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
/* Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	libc/ppc/sys/fork.s
 *
 * HISTORY
 * 18-Nov-92  Ben Fathi (benf@next.com)
 *	Created from M88K sources
 *
 * 11-Jan-92  Peter King (king@next.com)
 *	Created from M68K sources
 */
 
  
                                                                       
                                                                        
                                                                      
                       
   

#include "SYS.h"

#if defined(__i386__)

LEAF(___fork, 0)
	subl  $28, %esp                                                                     

	movl 	$ SYS_fork,%eax                          
	UNIX_SYSCALL_TRAP		                     
	jnc	L1			                

	CALL_EXTERN(tramp_cerror)
	movl	$-1,%eax
	addl	$28, %esp                       
	ret
	
L1:
	orl	%edx,%edx	                                   
	jz	L2		                                              
	
	               
	xorl	%eax,%eax	           
	REG_TO_EXTERN(%eax, __current_pid) 
L2:
	addl	$28, %esp                       
	                                             
	ret

#elif defined(__x86_64__)

LEAF(___fork, 0)
	subq  $24, %rsp                                                  

	movl 	$ SYSCALL_CONSTRUCT_UNIX(SYS_fork),%eax                         
	UNIX_SYSCALL_TRAP		                     
	jnc	L1			                

	movq	%rax, %rdi
	CALL_EXTERN(_cerror)
	movq	$-1, %rax
	addq	$24, %rsp                       
	ret
	
L1:
	orl	%edx,%edx	                                   
	jz	L2		                                              
	
	               
	xorq	%rax, %rax
	PICIFY(__current_pid)
	movl	%eax,(%r11)
L2:
	                                             
	addq	$24, %rsp                       
	ret

#elif defined(__arm__)
	
MI_ENTRY_POINT(___fork)
	stmfd	sp!, {r4, r7, lr}
	add	r7, sp, #4

	mov	r1, #1					                
	mov	r12, #SYS_fork
	swi	#SWI_SYSCALL				                   
	bcs	Lbotch					         

	cmp	r1, #0					                               
	beq	Lparent

	               
	MI_GET_ADDRESS(r3, __current_pid)
	mov	r0, #0
	str	r0, [r3]		                            
	ldmfd   sp!, {r4, r7, pc}

Lbotch:
	MI_CALL_EXTERNAL(_cerror)			                     
	mov	r0,#-1					                
	            
Lparent:	
	ldmfd   sp!, {r4, r7, pc}			                 

#elif defined(__arm64__)

#include <mach/arm64/asm.h>
	
MI_ENTRY_POINT(___fork)
	ARM64_STACK_PROLOG
	PUSH_FRAME
	                                                    
	mov		x16, #SYS_fork				               
	svc		#SWI_SYSCALL				                 
	b.cs	Lbotch						                              
	cbz		x1, Lparent					                                           

	        
	MI_GET_ADDRESS(x9, __current_pid)	                                      
	mov		w0, #0	
	str		w0, [x9]					                               
	POP_FRAME							           
	ARM64_STACK_EPILOG

Lbotch:
	MI_CALL_EXTERNAL(_cerror)			               
	mov		w0, #-1						                     
Lparent:
	POP_FRAME							         
	ARM64_STACK_EPILOG

#else
#error Unsupported architecture
#endif
