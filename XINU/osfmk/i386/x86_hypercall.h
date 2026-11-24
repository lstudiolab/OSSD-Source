/*
 * Copyright (c) 2020 Apple Inc. All rights reserved.
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

#ifndef _I386_X86_HYPERCALL_H_
#define _I386_X86_HYPERCALL_H_

#if DEBUG || DEVELOPMENT


  
                                           
  
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
                                                           
  
                                                                                
                                                                          
                                                                    
   

typedef struct hvg_hcall_output_regs {
	uint64_t   rax;
	uint64_t   rdi;
	uint64_t   rsi;
	uint64_t   rdx;
	uint64_t   rcx;
	uint64_t   r8;
	uint64_t   r9;
} hvg_hcall_output_regs_t;

  
                                                                               
                                                                             
                                                                           
                                                      
  
                                                                            
                                                                         
                                                                             
   

#define HVG_HCALL_CODE(code) ('A' << 24 | (code & 0xFFFFFF))


  
                                                                      
                                    
   

#define HVG_HCALL_RETURN(rax) {\
	__asm__ __volatile__ goto (\
	                           "jnc 2f  \n\t" \
	                           "jmp %l0 \n\t" \
	                           "2:      \n\t" \
	                          :                 \
	                          :                 \
	                          :                  \
	                          : error);\
	return HVG_HCALL_SUCCESS;\
error:\
	return (hvg_hcall_return_t)rax;\
}

static inline hvg_hcall_return_t
hvg_hypercall6(uint64_t code, uint64_t rdi, uint64_t rsi, uint64_t rdx, uint64_t rcx, uint64_t r8, uint64_t r9,
    hvg_hcall_output_regs_t *output)
{
	__asm__ __volatile__ ("movq %12, %%r8  \n\t"
                          "movq %13, %%r9  \n\t"
                          "vmcall          \n\t"
                          "movq %%r8, %5   \n\t"
                          "movq %%r9, %6   \n\t"
                        : "=a" (output->rax),                             
                          "=D" (output->rdi),                             
                          "=S" (output->rsi),                             
                          "=d" (output->rdx),                             
                          "=c" (output->rcx),                             
                          "=r" (output->r8),                              
                          "=r" (output->r9)                               
                        : "a"  (HVG_HCALL_CODE(code)),                    
                          "D"  (rdi),                                     
                          "S"  (rsi),                                     
                          "d"  (rdx),                                     
                          "c"  (rcx),                                     
                          "r"  (r8),                                      
                          "r"  (r9)                                       
                        : "memory", "r8", "r9");
	HVG_HCALL_RETURN(output->rax);
}

static inline hvg_hcall_return_t
hvg_hypercall0(const uint64_t code,
    hvg_hcall_output_regs_t *output)
{
	return hvg_hypercall6(code, 0, 0, 0, 0, 0, 0, output);
}

static inline hvg_hcall_return_t
hvg_hypercall1(const uint64_t code,
    const uint64_t rdi,
    hvg_hcall_output_regs_t *output)
{
	return hvg_hypercall6(code, rdi, 0, 0, 0, 0, 0, output);
}

static inline hvg_hcall_return_t
hvg_hypercall2(const uint64_t code,
    const uint64_t rdi, const uint64_t rsi,
    hvg_hcall_output_regs_t *output)
{
	return hvg_hypercall6(code, rdi, rsi, 0, 0, 0, 0, output);
}

static inline hvg_hcall_return_t
hvg_hypercall3(const uint64_t code,
    const uint64_t rdi, const uint64_t rsi, const uint64_t rdx,
    hvg_hcall_output_regs_t *output)
{
	return hvg_hypercall6(code, rdi, rsi, rdx, 0, 0, 0, output);
}

static inline hvg_hcall_return_t
hvg_hypercall4(const uint64_t code,
    const uint64_t rdi, const uint64_t rsi, const uint64_t rdx, const uint64_t rcx,
    hvg_hcall_output_regs_t *output)
{
	return hvg_hypercall6(code, rdi, rsi, rdx, rcx, 0, 0, output);
}

static inline hvg_hcall_return_t
hvg_hypercall5(const uint64_t code,
    const uint64_t rdi, const uint64_t rsi, const uint64_t rdx, const uint64_t rcx, const uint64_t r8,
    hvg_hcall_output_regs_t *output)
{
	return hvg_hypercall6(code, rdi, rsi, rdx, rcx, r8, 0, output);
}

#endif                           

#endif                             
