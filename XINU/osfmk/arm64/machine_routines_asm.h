/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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

#include <arm64/proc_reg.h>
#include <pexpert/arm64/board_config.h>
#include "assym.s"

#ifndef __ASSEMBLER__
#error "This header should only be used in .s files"
#endif

   
                                                                         
  
                    
                      
                      
                      
                    
  
                                                                                                
  
                            
                            
                            
                            
                            
   
                            
.macro AUTH_THREAD_STATE_IN_X0_COMMON tmp1, tmp2, tmp3, tmp4, tmp5, el0_state_allowed=0, PC_OFF=SS64_PC, CPSR_OFF=SS64_CPSR, X16_OFF=SS64_X16, LR_OFF=SS64_LR, check_func=ml_check_signed_state
	ldr		w2, [x0, \CPSR_OFF]
.if \el0_state_allowed==0
#if __has_feature(ptrauth_calls)
	                                                                                                      
	dmb		ld
#endif
.endif
	ldr		x1, [x0, \PC_OFF]
	ldp		x16, x17, [x0, \X16_OFF]

#if defined(HAS_APPLE_PAC)
	                                     
	mov		\tmp3, x3
	mov		\tmp4, x4
	mov		\tmp5, x5

	  
                                                
                                   
                                     
                        
  
                                                                                           
                                                                       
   
	mov		\tmp1, x1
	mov		\tmp2, x2
	ldr		x3, [x0, \LR_OFF]
	mov		x4, x16
	mov		x5, x17
	bl		EXT(\check_func)
	mov		x1, \tmp1
	mov		x2, \tmp2

.if \el0_state_allowed==0
	and		\tmp2, \tmp2, #PSR64_MODE_MASK
	cbnz		\tmp2, 1f
	bl		EXT(ml_auth_thread_state_invalid_cpsr)
1:
.endif

	                                                                                                         
	mov		lr, x3
	mov		x3, \tmp3
	mov		x4, \tmp4
	mov		x5, \tmp5
#else
	ldr		lr, [x0, \LR_OFF]
#endif                             
.endmacro

.macro  AUTH_THREAD_STATE_IN_X0 tmp1, tmp2, tmp3, tmp4, tmp5, el0_state_allowed=0
	AUTH_THREAD_STATE_IN_X0_COMMON \tmp1, \tmp2, \tmp3, \tmp4, \tmp5, \el0_state_allowed
.endmacro

.macro  AUTH_KERNEL_THREAD_STATE_IN_X0 tmp1, tmp2, tmp3, tmp4, tmp5, el0_state_allowed=0
	AUTH_THREAD_STATE_IN_X0_COMMON \tmp1, \tmp2, \tmp3, \tmp4, \tmp5, \el0_state_allowed, SS64_KERNEL_PC, SS64_KERNEL_CPSR, SS64_KERNEL_X16, SS64_KERNEL_LR, ml_check_kernel_signed_state
.endmacro
                          

                      
