/*
 * Copyright (c) 1999-2011 Apple Inc. All rights reserved.
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
/* Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	SYS.h
 *
 *	Definition of the user side of the UNIX system call interface
 *	for M98K.
 *
 *	Errors are flagged by the location of the trap return (ie., which
 *	instruction is executed upon rfi):
 *
 *		SC PC + 4:	Error (typically branch to cerror())
 *		SC PC + 8:	Success
 *
 * HISTORY
 * 18-Nov-92	Ben Fathi (benf@next.com)
 *	Ported to m98k.
 *
 *  9-Jan-92	Peter King (king@next.com)
 *	Created.
 */

#include <sys/syscall.h>

#if defined(__i386__)

#include <architecture/i386/asm_help.h>
#include <mach/i386/syscall_sw.h>

  
                                                                              
                                                                                
                                                                  
   

#define UNIX_SYSCALL_SYSENTER		call __sysenter_trap
#define UNIX_SYSCALL(name, nargs)			\
	.globl	tramp_cerror				;\
LEAF(_##name, 0)					;\
	movl	$ SYS_##name, %eax			;\
	UNIX_SYSCALL_SYSENTER				;\
	jnb	2f					;\
	BRANCH_EXTERN(tramp_cerror)  			;\
2:

#define UNIX_SYSCALL_INT(name, nargs)			\
	.globl	tramp_cerror				;\
LEAF(_##name, 0)					;\
	movl	$ SYS_##name, %eax			;\
	UNIX_SYSCALL_TRAP				;\
	jnb	2f					;\
	BRANCH_EXTERN(tramp_cerror)  			;\
2:

#if defined(__SYSCALL_32BIT_ARG_BYTES) && ((__SYSCALL_32BIT_ARG_BYTES >= 4) && (__SYSCALL_32BIT_ARG_BYTES <= 20))
#define UNIX_SYSCALL_NONAME(name, nargs, cerror)			\
	movl	$(SYS_##name | (__SYSCALL_32BIT_ARG_BYTES << I386_SYSCALL_ARG_BYTES_SHIFT)), %eax		;\
	UNIX_SYSCALL_SYSENTER					;\
	jnb	2f						;\
	BRANCH_EXTERN(tramp_##cerror)				;\
2:
#else                                            
#define UNIX_SYSCALL_NONAME(name, nargs, cerror)	\
	movl	$ SYS_##name, %eax			;\
	UNIX_SYSCALL_SYSENTER				;\
	jnb	2f					;\
	BRANCH_EXTERN(tramp_##cerror)			;\
2:
#endif

#define UNIX_SYSCALL_INT_NONAME(name, nargs)		\
	.globl	tramp_cerror_nocancel			;\
	movl	$ SYS_##name, %eax			;\
	UNIX_SYSCALL_TRAP				;\
	jnb	2f					;\
	BRANCH_EXTERN(tramp_cerror_nocancel) 		;\
2:

#define PSEUDO(pseudo, name, nargs, cerror)	\
LEAF(pseudo, 0)					;\
	UNIX_SYSCALL_NONAME(name, nargs, cerror)

#define PSEUDO_INT(pseudo, name, nargs)		\
LEAF(pseudo, 0)					;\
	UNIX_SYSCALL_INT_NONAME(name, nargs)

#define __SYSCALL2(pseudo, name, nargs, cerror)			\
	PSEUDO(pseudo, name, nargs, cerror)			;\
	ret

#define __SYSCALL(pseudo, name, nargs)			\
	PSEUDO(pseudo, name, nargs, cerror)		;\
	ret

#define __SYSCALL_INT(pseudo, name, nargs)		\
	PSEUDO_INT(pseudo, name, nargs)			;\
	ret

#elif defined(__x86_64__)

#include <architecture/i386/asm_help.h>
#include <mach/i386/syscall_sw.h>

#define UNIX_SYSCALL_SYSCALL	\
	movq	%rcx, %r10		;\
	syscall

#define UNIX_SYSCALL(name, nargs)						 \
	.globl	cerror								;\
LEAF(_##name, 0)								;\
	movl	$ SYSCALL_CONSTRUCT_UNIX(SYS_##name), %eax			;\
	UNIX_SYSCALL_SYSCALL							;\
	jnb		2f							;\
	movq	%rax, %rdi							;\
	BRANCH_EXTERN(_cerror)							;\
2:

#define UNIX_SYSCALL_NONAME(name, nargs, cerror)		 \
	.globl	cerror								;\
	movl	$ SYSCALL_CONSTRUCT_UNIX(SYS_##name), %eax			;\
	UNIX_SYSCALL_SYSCALL							;\
	jnb		2f							;\
	movq	%rax, %rdi							;\
	BRANCH_EXTERN(_##cerror)						;\
2:

#define PSEUDO(pseudo, name, nargs, cerror)			\
LEAF(pseudo, 0)					;\
	UNIX_SYSCALL_NONAME(name, nargs, cerror)

#define __SYSCALL2(pseudo, name, nargs, cerror) \
	PSEUDO(pseudo, name, nargs, cerror)			;\
	ret

#define __SYSCALL(pseudo, name, nargs)			\
	PSEUDO(pseudo, name, nargs, cerror)			;\
	ret

#elif defined(__arm__)

#include <architecture/arm/asm_help.h>
#include <mach/arm/syscall_sw.h>

  
                             
  
           
              
                  
                                                                         
   

  
          
   

  
                                                            
   

#if defined(__DYNAMIC__)
#define MI_GET_ADDRESS(reg,var)  \
	ldr	reg, 4f					;\
3:	ldr	reg, [pc, reg]				;\
	b	5f					;\
4:	.long	6f - (3b + 8)				;\
5:							;\
	.non_lazy_symbol_pointer			;\
6:							;\
	.indirect_symbol var				;\
	.long 0						;\
	.text						;\
	.align 2
#else
#define MI_GET_ADDRESS(reg,var)  \
	ldr	reg, 3f	;\
	b	4f	;\
3:	.long var	;\
4:
#endif

#if defined(__DYNAMIC__)
#define MI_BRANCH_EXTERNAL(var)				\
	.globl	var								;\
	MI_GET_ADDRESS(ip, var)				;\
 	bx	ip
#else
#define MI_BRANCH_EXTERNAL(var)				;\
	.globl	var								;\
 	b	var
#endif

#if defined(__DYNAMIC__)
#define MI_CALL_EXTERNAL(var)    \
	.globl	var				;\
	MI_GET_ADDRESS(ip,var)	;\
	blx	ip
#else
#define MI_CALL_EXTERNAL(var)				\
	.globl	var								;\
 	bl	var
#endif

#define MI_ENTRY_POINT(name)				\
	.text									;\
	.align 2	;\
	.globl  name							;\
name:

                                               
#define DO_SYSCALL(num)		\
	.if (((num) & 0xff) == (num)) 	       				;\
	mov		r12, #(num)		       			;\
	.elseif (((num) & 0x3fc) == (num))				;\
	mov		r12, #(num)					;\
	.else								;\
	mov		r12, #((num) & 0xffffff00)	                                     ;\
	orr		r12, r12, #((num) & 0xff)	                  ;\
	.endif								;\
	swi		#SWI_SYSCALL

                                   
#define	SYSCALL_0to4(name, cerror)			\
	MI_ENTRY_POINT(_##name)					;\
	DO_SYSCALL(SYS_##name)					;\
	bxcc	lr								                                          ; \
1:	MI_BRANCH_EXTERNAL(_##cerror)

                                                                                
#define	SYSCALL_5(name, cerror)				\
	MI_ENTRY_POINT(_##name)					;\
	mov		ip, sp							                                 ; \
	stmfd	sp!, { r4-r5 }					                 ;\
	ldr		r4, [ip]						                   ; \
	DO_SYSCALL(SYS_##name)					;\
	ldmfd	sp!, { r4-r5 }					                    ; \
	bxcc	lr								                                          ; \
1:	MI_BRANCH_EXTERNAL(_##cerror)

                                                                    
#define SYSCALL_6to12(name, save_regs, arg_regs, cerror) \
	MI_ENTRY_POINT(_##name)					;\
	mov		ip, sp							                                 ; \
	stmfd	sp!, { save_regs }				                        ;\
	ldmia	ip, { arg_regs }				                    ; \
	DO_SYSCALL(SYS_##name)					;\
	ldmfd	sp!, { save_regs }				                                ; \
	bxcc	lr								                                          ; \
1:	MI_BRANCH_EXTERNAL(_##cerror)

#define COMMA ,

#if __BIGGEST_ALIGNMENT__ > 4

                                                                        
                                                                              
              

                                             

#define SYSCALL_0(name)						SYSCALL_0to4(name)
#define SYSCALL_1(name)						SYSCALL_0to4(name)
#define SYSCALL_2(name)						SYSCALL_0to4(name)
#define SYSCALL_3(name)						SYSCALL_0to4(name)
#define SYSCALL_4(name)						SYSCALL_6to12(name, r4-r5, r4-r5)
#undef SYSCALL_5
#define SYSCALL_5(name)						SYSCALL_6to12(name, r4-r5, r4-r5)
#define SYSCALL_6(name)						SYSCALL_6to12(name, r4-r6 COMMA r8, r4-r6 COMMA r8)
#define SYSCALL_7(name)						SYSCALL_6to12(name, r4-r6 COMMA r8, r4-r6 COMMA r8)
#define SYSCALL_8(name)						SYSCALL_6to12(name, r4-r6 COMMA r8, r4-r6 COMMA r8)
#define SYSCALL_12(name)					SYSCALL_6to12(name, r4-r6 COMMA r8, r4-r6 COMMA r8)

#else                                                            

#define SYSCALL_0(name)						SYSCALL_0to4(name)
#define SYSCALL_1(name)						SYSCALL_0to4(name)
#define SYSCALL_2(name)						SYSCALL_0to4(name)
#define SYSCALL_3(name)						SYSCALL_0to4(name)
#define SYSCALL_4(name)						SYSCALL_0to4(name)
                              
#define SYSCALL_6(name)						SYSCALL_6to12(name, r4-r5, r4-r5)
#define SYSCALL_7(name)						SYSCALL_6to12(name, r4-r6 COMMA r8, r4-r6)
#define SYSCALL_8(name)						SYSCALL_6to12(name, r4-r6 COMMA r8, r4-r6)                   
#define SYSCALL_12(name)					SYSCALL_6to12(name, r4-r6 COMMA r8, r4-r6)                        

#endif                             

                                                                           
#ifndef __SYSCALL_32BIT_ARG_BYTES
#define SYSCALL(name, nargs, cerror)		SYSCALL_##nargs(name, cerror)
#define SYSCALL_NONAME(name, nargs, cerror)	SYSCALL_NONAME_##nargs(name, cerror)
#else
#if __SYSCALL_32BIT_ARG_BYTES < 20
#define SYSCALL(name, nargs, cerror)		SYSCALL_0to4(name, cerror)
#define SYSCALL_NONAME(name, nargs, cerror)	SYSCALL_NONAME_0to4(name, cerror)
#elif __SYSCALL_32BIT_ARG_BYTES == 20
#define SYSCALL(name, nargs, cerror)		SYSCALL_5(name, cerror)
#define SYSCALL_NONAME(name, nargs, cerror)	SYSCALL_NONAME_5(name, cerror)
#elif __SYSCALL_32BIT_ARG_BYTES == 24
#define SYSCALL(name, nargs, cerror)		SYSCALL_6(name, cerror)
#define SYSCALL_NONAME(name, nargs, cerror)	SYSCALL_NONAME_6(name, cerror)
#elif __SYSCALL_32BIT_ARG_BYTES == 28 
#define SYSCALL(name, nargs, cerror)		SYSCALL_7(name, cerror)
#define SYSCALL_NONAME(name, nargs, cerror)	SYSCALL_NONAME_7(name, cerror)
#elif __SYSCALL_32BIT_ARG_BYTES == 32 
#define SYSCALL(name, nargs, cerror)		SYSCALL_8(name, cerror)
#define SYSCALL_NONAME(name, nargs, cerror)	SYSCALL_NONAME_8(name, cerror)
#elif __SYSCALL_32BIT_ARG_BYTES == 36 
#define SYSCALL(name, nargs, cerror)		SYSCALL_8(name, cerror)
#define SYSCALL_NONAME(name, nargs, cerror)	SYSCALL_NONAME_8(name, cerror)
#elif __SYSCALL_32BIT_ARG_BYTES == 40 
#define SYSCALL(name, nargs, cerror)		SYSCALL_8(name, cerror)
#define SYSCALL_NONAME(name, nargs, cerror)	SYSCALL_NONAME_8(name, cerror)
#elif __SYSCALL_32BIT_ARG_BYTES == 44 
#define SYSCALL(name, nargs, cerror)		SYSCALL_8(name, cerror)
#define SYSCALL_NONAME(name, nargs, cerror)	SYSCALL_NONAME_8(name, cerror)
#elif __SYSCALL_32BIT_ARG_BYTES == 48 
#define SYSCALL(name, nargs, cerror)		SYSCALL_12(name, cerror)
#define SYSCALL_NONAME(name, nargs, cerror)	SYSCALL_NONAME_12(name, cerror)
#endif
#endif

#define	SYSCALL_NONAME_0to4(name, cerror)	\
	DO_SYSCALL(SYS_##name)					;\
	bcc		1f								                                              ; \
	MI_BRANCH_EXTERNAL(_##cerror)			                  ; \
1:

#define	SYSCALL_NONAME_5(name, cerror)		\
	mov		ip, sp 							                                 ; \
	stmfd	sp!, { r4-r5 }					                 ;\
	ldr		r4, [ip]						                   ; \
	DO_SYSCALL(SYS_##name)					;\
	ldmfd	sp!, { r4-r5 }					                    ; \
	bcc		1f								                                              ; \
	MI_BRANCH_EXTERNAL(_##cerror)			                  ; \
1:

#define	SYSCALL_NONAME_6to12(name, save_regs, arg_regs, cerror)	\
	mov		ip, sp 							                                 ; \
	stmfd	sp!, { save_regs }				                       ;\
	ldmia	ip, { arg_regs }				                     ; \
	DO_SYSCALL(SYS_##name)					;\
	ldmfd	sp!, { save_regs }				                                ; \
	bcc		1f								                                              ; \
	MI_BRANCH_EXTERNAL(_##cerror)			                  ; \
1:


#if __BIGGEST_ALIGNMENT__ > 4

                                                                        
                                                                              
                                                                          
                                                             

#define SYSCALL_NONAME_0(name, cerror)				SYSCALL_NONAME_0to4(name, cerror)
#define SYSCALL_NONAME_1(name, cerror)				SYSCALL_NONAME_0to4(name, cerror)
#define SYSCALL_NONAME_2(name, cerror)				SYSCALL_NONAME_0to4(name, cerror)
#define SYSCALL_NONAME_3(name, cerror)				SYSCALL_NONAME_0to4(name, cerror)
#define SYSCALL_NONAME_4(name, cerror)				SYSCALL_NONAME_6to12(name, r4-r5, r4-r5, cerror)
#undef SYSCALL_NONAME_5
#define SYSCALL_NONAME_5(name, cerror)				SYSCALL_NONAME_6to12(name, r4-r5, r4-r5, cerror)
#define SYSCALL_NONAME_6(name, cerror)				SYSCALL_NONAME_6to12(name, r4-r6 COMMA r8, r4-r6 COMMA r8, cerror)
#define SYSCALL_NONAME_7(name, cerror)				SYSCALL_NONAME_6to12(name, r4-r6 COMMA r8, r4-r6 COMMA r8, cerror)
#define SYSCALL_NONAME_8(name, cerror)				SYSCALL_NONAME_6to12(name, r4-r6 COMMA r8, r4-r6 COMMA r8, cerror)
#define SYSCALL_NONAME_12(name, cerror)				SYSCALL_NONAME_6to12(name, r4-r6 COMMA r8, r4-r6 COMMA r8, cerror)

#else                                                            

#define SYSCALL_NONAME_0(name, cerror)				SYSCALL_NONAME_0to4(name, cerror)
#define SYSCALL_NONAME_1(name, cerror)				SYSCALL_NONAME_0to4(name, cerror)
#define SYSCALL_NONAME_2(name, cerror)				SYSCALL_NONAME_0to4(name, cerror)
#define SYSCALL_NONAME_3(name, cerror)				SYSCALL_NONAME_0to4(name, cerror)
#define SYSCALL_NONAME_4(name, cerror)				SYSCALL_NONAME_0to4(name, cerror)
                                     
#define SYSCALL_NONAME_6(name, cerror)				SYSCALL_NONAME_6to12(name, r4-r5, r4-r5, cerror)
#define SYSCALL_NONAME_7(name, cerror)				SYSCALL_NONAME_6to12(name, r4-r6 COMMA r8, r4-r6, cerror)
#define SYSCALL_NONAME_8(name, cerror)				SYSCALL_NONAME_6to12(name, r4-r6 COMMA r8, r4-r6, cerror)
#define SYSCALL_NONAME_12(name, cerror)				SYSCALL_NONAME_6to12(name, r4-r6 COMMA r8, r4-r6, cerror)

#endif                             

#define	PSEUDO(pseudo, name, nargs, cerror)			\
	.globl pseudo						;\
	.text									;\
	.align  2								;\
pseudo:									;\
	SYSCALL_NONAME(name, nargs, cerror)

#define __SYSCALL2(pseudo, name, nargs, cerror)		\
	PSEUDO(pseudo, name, nargs, cerror)				;\
	bx lr

#define __SYSCALL(pseudo, name, nargs)				\
	PSEUDO(pseudo, name, nargs, cerror)				;\
	bx lr

#elif defined(__arm64__)

#include <mach/arm/syscall_sw.h>
#include <mach/arm/vm_param.h>
#include <mach/arm64/asm.h>

#if defined(__arm64__) && !defined(__LP64__)
#define ZERO_EXTEND(argnum) uxtw  x ## argnum, w ## argnum
#else
#define ZERO_EXTEND(argnum)
#endif

#if defined(__arm64__) && !defined(__LP64__)
#define SIGN_EXTEND(argnum) sxtw  x ## argnum, w ## argnum
#else
#define SIGN_EXTEND(argnum)
#endif

  
                               
  
      
   

#define DO_SYSCALL(num, cerror)                 \
	mov   x16, #(num)                     %%\
	svc   #SWI_SYSCALL                    %%\
	b.cc  2f                              %%\
	ARM64_STACK_PROLOG                    %%\
	PUSH_FRAME                            %%\
	bl    _##cerror                       %%\
	POP_FRAME                             %%\
	ARM64_STACK_EPILOG                    %%\
2:

#define MI_GET_ADDRESS(reg,var)  \
   adrp	reg, var@page      %%\
   add  reg, reg, var@pageoff   %%

#define MI_CALL_EXTERNAL(sym)	\
   .globl sym                %% \
   bl sym                  	

#define	SYSCALL_NONAME(name, nargs, cerror)						\
  DO_SYSCALL(SYS_##name, cerror)					%%	\
1:

#define MI_ENTRY_POINT(name)				\
  .text					%% \
  .align 2	            %% \
  .globl  name			%%	\
name:

#define	PSEUDO(pseudo, name, nargs, cerror)			\
  .text									%% \
  .align  2								%% \
  .globl pseudo						%%		\
  pseudo:									%% \
	SYSCALL_NONAME(name, nargs, cerror)

#define __SYSCALL(pseudo, name, nargs)		\
  PSEUDO(pseudo, name, nargs, cerror)		%%	\
  ret

#define __SYSCALL2(pseudo, name, nargs, cerror)		\
  PSEUDO(pseudo, name, nargs, cerror)		%% \
  ret

#else
#error Unsupported architecture
#endif

