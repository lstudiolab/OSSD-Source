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

#ifndef _ARM64_PAC_ASM_H_
#define _ARM64_PAC_ASM_H_

#ifndef __ASSEMBLER__
#error "This header should only be used in .s files"
#endif

#include <pexpert/arm64/board_config.h>
#include <arm64/proc_reg.h>
#if HAS_PARAVIRTUALIZED_PAC
#include "smccc_asm.h"
#endif
#include "assym.s"

#if defined(HAS_APPLE_PAC)


                            

   
                     
  
                                                                        
                                                                                
                                                                              
  
                                                                                
                                    
                                    
                           
   
.macro REPROGRAM_JOP_KEYS	skip_label, new_jop_key, cpudatap, tmp
	ldr		\tmp, [\cpudatap, CPU_JOP_KEY]
	cmp		\new_jop_key, \tmp
	b.eq	\skip_label
	SET_JOP_KEY_REGISTERS	\new_jop_key, \tmp
	str		\new_jop_key, [\cpudatap, CPU_JOP_KEY]
.endmacro

   
                     
  
                                                                        
                                                                                
                                                                              
  
                                                                                
                                    
                                    
                           
   
.macro REPROGRAM_ROP_KEYS	skip_label, new_rop_key, cpudatap, tmp
	ldr		\tmp, [\cpudatap, CPU_ROP_KEY]
	cmp		\new_rop_key, \tmp
	b.eq	\skip_label
	SET_ROP_KEY_REGISTERS	\new_rop_key, \tmp
	str		\new_rop_key, [\cpudatap, CPU_ROP_KEY]
.endmacro

   
                        
  
                                                                               
                                                                               
          
  
                                    
                           
   
.macro SET_JOP_KEY_REGISTERS	new_jop_key, tmp
#if HAS_PARAVIRTUALIZED_PAC
	SAVE_SMCCC_CLOBBERED_REGISTERS
	  
                                                                            
                                                                         
                                                                         
                                                                    
   
                                                                             
                                                                          
    
	MOV64	x0, VMAPPLE_PAC_SET_EL0_DIVERSIFIER
	mov		x1, \new_jop_key
	hvc		#0
	LOAD_SMCCC_CLOBBERED_REGISTERS
#endif                              
.endmacro

   
                        
  
                                                                               
                                                                               
          
  
                                    
                           
   
.macro SET_ROP_KEY_REGISTERS	new_rop_key, tmp
#if HAS_PARAVIRTUALIZED_PAC
	SAVE_SMCCC_CLOBBERED_REGISTERS
	MOV64	x0, VMAPPLE_PAC_SET_B_KEYS
	mov		x1, \new_rop_key
	hvc		#0
	LOAD_SMCCC_CLOBBERED_REGISTERS
#endif                              
.endmacro

                          

#endif                             

#endif                        

                           
