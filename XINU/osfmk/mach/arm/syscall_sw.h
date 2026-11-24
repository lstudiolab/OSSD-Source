/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
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
  
                        
   

#ifndef	_MACH_ARM_SYSCALL_SW_H_
#define _MACH_ARM_SYSCALL_SW_H_

#if defined(__arm__)

#include <mach/machine/vm_param.h>
#include <architecture/arm/asm_help.h>

                                             
#define _kernel_trap_0to4(trap_name, trap_number) \
	mov		r12, # ## trap_number			                          ; \
	swi		#SWI_SYSCALL					; \
	bx		lr								             ;

#define _kernel_trap_5(trap_name, trap_number) \
	mov		ip, sp							                           ; \
	stmfd	sp!, { r4-r5 }					                                           ; \
	ldr		r4, [ ip ]						                 ; \
	mov		r12, # ## trap_number			                          ; \
	swi		#SWI_SYSCALL					; \
	ldmfd	sp!, { r4-r5 }					                    ;\
	bx		lr								             ;

#define _kernel_trap_6to9(trap_name, trap_number, save_regs, arg_regs) \
	mov		ip, sp							                           ; \
	stmfd	sp!, { save_regs }				                       ; \
	ldmia	ip, { arg_regs }				                                       ;\
	mov		r12, # ## trap_number			                          ; \
	swi		#SWI_SYSCALL					; \
	ldmfd	sp!, { save_regs }	 			                                ;\
	bx		lr								             ;

#define COMMA ,

                                                                        
                                                                              
                                                                           
                        
#if __BIGGEST_ALIGNMENT__ > 4

#define _kernel_trap_0(trap_name, trap_number) _kernel_trap_0to4(trap_name, trap_number)
#define _kernel_trap_1(trap_name, trap_number) _kernel_trap_0to4(trap_name, trap_number)
#define _kernel_trap_2(trap_name, trap_number) _kernel_trap_0to4(trap_name, trap_number)
#define _kernel_trap_3(trap_name, trap_number) _kernel_trap_0to4(trap_name, trap_number)
#define _kernel_trap_4(trap_name, trap_number) _kernel_trap_6to9(trap_name, trap_number, r4-r5, r4-r5)
#undef _kernel_trap_5
#define _kernel_trap_5(trap_name, trap_number) _kernel_trap_6to9(trap_name, trap_number, r4-r5, r4-r5)
#define _kernel_trap_6(trap_name, trap_number) _kernel_trap_6to9(trap_name, trap_number, r4-r6 COMMA r8, r4-r6 COMMA r8)
#define _kernel_trap_7(trap_name, trap_number) _kernel_trap_6to9(trap_name, trap_number, r4-r6 COMMA r8, r4-r6 COMMA r8)
#define _kernel_trap_8(trap_name, trap_number) _kernel_trap_6to9(trap_name, trap_number, r4-r6 COMMA r8, r4-r6 COMMA r8)
#define _kernel_trap_9(trap_name, trap_number) _kernel_trap_6to9(trap_name, trap_number, r4-r6 COMMA r8, r4-r6 COMMA r8)
#else                                

#define _kernel_trap_0(trap_name, trap_number) _kernel_trap_0to4(trap_name, trap_number)
#define _kernel_trap_1(trap_name, trap_number) _kernel_trap_0to4(trap_name, trap_number)
#define _kernel_trap_2(trap_name, trap_number) _kernel_trap_0to4(trap_name, trap_number)
#define _kernel_trap_3(trap_name, trap_number) _kernel_trap_0to4(trap_name, trap_number)
#define _kernel_trap_4(trap_name, trap_number) _kernel_trap_0to4(trap_name, trap_number)
                                  
#define _kernel_trap_6(trap_name, trap_number) _kernel_trap_6to9(trap_name, trap_number, r4-r5, r4-r5)
                                                                                                     
#define _kernel_trap_7(trap_name, trap_number) _kernel_trap_6to9(trap_name, trap_number, r4-r6 COMMA r8, r4-r6)
#define _kernel_trap_8(trap_name, trap_number) _kernel_trap_6to9(trap_name, trap_number, r4-r6 COMMA r8, r4-r6 COMMA r8)
                                                                                                          
#define _kernel_trap_9(trap_name, trap_number) _kernel_trap_8(trap_name, trap_number)

#endif                             


                                                                    
#define kernel_trap(trap_name, trap_number, num_args) \
	LEAF(_##trap_name, 0)	\
	_kernel_trap_##num_args(trap_name, trap_number)

#elif defined(__arm64__)

#include <mach/machine/vm_param.h>

#define kernel_trap(trap_name, trap_number, num_args) \
.globl _##trap_name                                           %% \
.text                                                         %% \
.align  2                                                     %% \
_##trap_name:                                                 %% \
    mov x16, #(trap_number)                                   %% \
    svc #SWI_SYSCALL                                          %% \
    ret

#else
#error Unsupported architecture
#endif

#endif	                             
