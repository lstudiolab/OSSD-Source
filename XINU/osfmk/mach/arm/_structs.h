/*
 * Copyright (c) 2004-2007 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
#ifndef _MACH_ARM__STRUCTS_H_
#define _MACH_ARM__STRUCTS_H_

#include <sys/cdefs.h>                      
#include <machine/types.h>                 

#if __DARWIN_UNIX03
#define _STRUCT_ARM_EXCEPTION_STATE struct __darwin_arm_exception_state
_STRUCT_ARM_EXCEPTION_STATE
{
	__uint32_t __exception;                                    
	__uint32_t __fsr;                         
	__uint32_t __far;                                  
};
#else                       
#define _STRUCT_ARM_EXCEPTION_STATE struct arm_exception_state
_STRUCT_ARM_EXCEPTION_STATE
{
	__uint32_t exception;                                      
	__uint32_t fsr;                           
	__uint32_t far;                                    
};
#endif                      

#if __DARWIN_UNIX03
#define _STRUCT_ARM_EXCEPTION_STATE64 struct __darwin_arm_exception_state64
_STRUCT_ARM_EXCEPTION_STATE64
{
	__uint64_t __far;                                  
	__uint32_t __esr;                               
	__uint32_t __exception;                                    
};
#else                       
#define _STRUCT_ARM_EXCEPTION_STATE64 struct arm_exception_state64
_STRUCT_ARM_EXCEPTION_STATE64
{
	__uint64_t far;                                    
	__uint32_t esr;                                 
	__uint32_t exception;                                      
};
#endif                      

#if __DARWIN_UNIX03
#define _STRUCT_ARM_THREAD_STATE struct __darwin_arm_thread_state
_STRUCT_ARM_THREAD_STATE
{
	__uint32_t __r[13];                                      
	__uint32_t __sp;                           
	__uint32_t __lr;                           
	__uint32_t __pc;                             
	__uint32_t __cpsr;                                       
};
#else                       
#define _STRUCT_ARM_THREAD_STATE struct arm_thread_state
_STRUCT_ARM_THREAD_STATE
{
	__uint32_t r[13];                                        
	__uint32_t sp;                             
	__uint32_t lr;                             
	__uint32_t pc;                               
	__uint32_t cpsr;                                         
};
#endif                      

#if defined(KERNEL)

#define __DARWIN_OPAQUE_ARM_THREAD_STATE64 0
#define __DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH 0x1
#define __DARWIN_ARM_THREAD_STATE64_FLAGS_IB_SIGNED_LR 0x2

#define _STRUCT_ARM_THREAD_STATE64      struct arm_thread_state64
_STRUCT_ARM_THREAD_STATE64
{
	__uint64_t    x[29];                                          
	__uint64_t    fp;                                      
	__uint64_t    lr;                                      
	__uint64_t    sp;                                      
	__uint64_t    pc;                                    
	__uint32_t    cpsr;                                                  
	__uint32_t    flags;                                           
};

#else                      

  
                                                                           
                                                                            
                                                                           
                                          
   
#if defined(__arm64__) && defined(__LP64__)

#if __has_feature(ptrauth_calls)
#define __DARWIN_OPAQUE_ARM_THREAD_STATE64 1
#define __DARWIN_PTRAUTH_ARM_THREAD_STATE64 1
#endif                                   

#ifndef __DARWIN_OPAQUE_ARM_THREAD_STATE64
#define __DARWIN_OPAQUE_ARM_THREAD_STATE64 0
#endif

#else                                              

#undef __DARWIN_OPAQUE_ARM_THREAD_STATE64
#define __DARWIN_OPAQUE_ARM_THREAD_STATE64 0

#endif                                              

#if __DARWIN_UNIX03
#define _STRUCT_ARM_THREAD_STATE64 struct __darwin_arm_thread_state64
#if __DARWIN_OPAQUE_ARM_THREAD_STATE64
_STRUCT_ARM_THREAD_STATE64
{
	__uint64_t __x[29];                                           
	void*      __opaque_fp;                        
	void*      __opaque_lr;                        
	void*      __opaque_sp;                        
	void*      __opaque_pc;                      
	__uint32_t __cpsr;                                           
	__uint32_t __opaque_flags;                                        
};
#else                                         
_STRUCT_ARM_THREAD_STATE64
{
	__uint64_t __x[29];                                       
	__uint64_t __fp;                           
	__uint64_t __lr;                           
	__uint64_t __sp;                           
	__uint64_t __pc;                         
	__uint32_t __cpsr;                                       
	__uint32_t __pad;                                               
};
#endif                                         
#else                       
#define _STRUCT_ARM_THREAD_STATE64 struct arm_thread_state64
#if __DARWIN_OPAQUE_ARM_THREAD_STATE64
_STRUCT_ARM_THREAD_STATE64
{
	__uint64_t x[29];                                             
	void*      __opaque_fp;                        
	void*      __opaque_lr;                        
	void*      __opaque_sp;                        
	void*      __opaque_pc;                      
	__uint32_t cpsr;                                             
	__uint32_t __opaque_flags;                                        
};
#else                                         
_STRUCT_ARM_THREAD_STATE64
{
	__uint64_t x[29];                                       
	__uint64_t fp;                           
	__uint64_t lr;                           
	__uint64_t sp;                           
	__uint64_t pc;                         
	__uint32_t cpsr;                                       
	__uint32_t __pad;                                             
};
#endif                                         
#endif                      

#if __DARWIN_C_LEVEL >= __DARWIN_C_FULL && defined(__arm64__)

                                                             

#if __has_feature(ptrauth_calls) && defined(__LP64__)
#include <ptrauth.h>

#if !__DARWIN_OPAQUE_ARM_THREAD_STATE64 || !__DARWIN_PTRAUTH_ARM_THREAD_STATE64
#error "Invalid configuration"
#endif

#define __DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH 0x1
#define __DARWIN_ARM_THREAD_STATE64_FLAGS_IB_SIGNED_LR 0x2

                                                                     
#define __darwin_arm_thread_state64_get_pc(ts) \
	__extension__ ({ const _STRUCT_ARM_THREAD_STATE64 *__tsp = &(ts); \
	(uintptr_t)(__tsp->__opaque_pc && !(__tsp->__opaque_flags &       \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH) ?                   \
	ptrauth_auth_data(__tsp->__opaque_pc,                             \
	ptrauth_key_process_independent_code,                             \
	ptrauth_string_discriminator("pc")) : __tsp->__opaque_pc); })
                                                                            
                                                                            
                                                                               
#define __darwin_arm_thread_state64_get_pc_fptr(ts) \
	__extension__ ({ const _STRUCT_ARM_THREAD_STATE64 *__tsp = &(ts); \
	(__tsp->__opaque_pc && !(__tsp->__opaque_flags &                  \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH) ?                   \
	ptrauth_auth_function(__tsp->__opaque_pc,                         \
	ptrauth_key_process_independent_code,                             \
	ptrauth_string_discriminator("pc")) : NULL); })
                                                                
#define __darwin_arm_thread_state64_set_pc_fptr(ts, fptr) \
	__extension__ ({ _STRUCT_ARM_THREAD_STATE64 *__tsp = &(ts);   \
	__typeof__(fptr) __f = (fptr); __tsp->__opaque_pc =           \
	(__f ? (!(__tsp->__opaque_flags &                             \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH) ?               \
	ptrauth_auth_and_resign(__f, ptrauth_key_function_pointer, 0, \
	ptrauth_key_process_independent_code,                         \
	ptrauth_string_discriminator("pc")) : ptrauth_auth_data(__f,  \
	ptrauth_key_function_pointer, 0)) : __f); })
                                                                     
#define __darwin_arm_thread_state64_get_lr(ts) \
	__extension__ ({ const _STRUCT_ARM_THREAD_STATE64 *__tsp = &(ts); \
	(uintptr_t)(__tsp->__opaque_lr && !(__tsp->__opaque_flags & (     \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH |                    \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_IB_SIGNED_LR)) ?                \
	ptrauth_auth_data(__tsp->__opaque_lr,                             \
	ptrauth_key_process_independent_code,                             \
	ptrauth_string_discriminator("lr")) : __tsp->__opaque_lr); })
                                                                            
                                                                            
                                                                               
#define __darwin_arm_thread_state64_get_lr_fptr(ts) \
	__extension__ ({ const _STRUCT_ARM_THREAD_STATE64 *__tsp = &(ts); \
	(__tsp->__opaque_lr && !(__tsp->__opaque_flags & (                \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH |                    \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_IB_SIGNED_LR)) ?                \
	ptrauth_auth_function(__tsp->__opaque_lr,                         \
	ptrauth_key_process_independent_code,                             \
	ptrauth_string_discriminator("lr")) : NULL); })
                                                                
#define __darwin_arm_thread_state64_set_lr_fptr(ts, fptr) \
	__extension__ ({ _STRUCT_ARM_THREAD_STATE64 *__tsp = &(ts);            \
	__typeof__(fptr) __f = (fptr); __tsp->__opaque_lr =                    \
	(__f ? (!(__tsp->__opaque_flags &                                      \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH) ? (__tsp->__opaque_flags \
	&= ~__DARWIN_ARM_THREAD_STATE64_FLAGS_IB_SIGNED_LR ,                   \
	ptrauth_auth_and_resign(__f, ptrauth_key_function_pointer, 0,          \
	ptrauth_key_process_independent_code,                                  \
	ptrauth_string_discriminator("lr"))) : ptrauth_auth_data(__f,          \
	ptrauth_key_function_pointer, 0)) : __f); })
                                                                     
#define __darwin_arm_thread_state64_get_sp(ts) \
	__extension__ ({ const _STRUCT_ARM_THREAD_STATE64 *__tsp = &(ts); \
	(uintptr_t)(__tsp->__opaque_sp && !(__tsp->__opaque_flags &       \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH) ?                   \
	ptrauth_auth_data(__tsp->__opaque_sp,                             \
	ptrauth_key_process_independent_data,                             \
	ptrauth_string_discriminator("sp")) : __tsp->__opaque_sp); })
                                                                  
#define __darwin_arm_thread_state64_set_sp(ts, ptr) \
	__extension__ ({ _STRUCT_ARM_THREAD_STATE64 *__tsp = &(ts); \
	void *__p = (void*)(uintptr_t)(ptr); __tsp->__opaque_sp =   \
	(__p && !(__tsp->__opaque_flags &                           \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH) ?             \
	ptrauth_sign_unauthenticated(__p,                           \
	ptrauth_key_process_independent_data,                       \
	ptrauth_string_discriminator("sp")) : __p); })
                                                                     
#define __darwin_arm_thread_state64_get_fp(ts) \
	__extension__ ({ const _STRUCT_ARM_THREAD_STATE64 *__tsp = &(ts); \
	(uintptr_t)(__tsp->__opaque_fp && !(__tsp->__opaque_flags &       \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH) ?                   \
	ptrauth_auth_data(__tsp->__opaque_fp,                             \
	ptrauth_key_process_independent_data,                             \
	ptrauth_string_discriminator("fp")) : __tsp->__opaque_fp); })
                                                                  
#define __darwin_arm_thread_state64_set_fp(ts, ptr) \
	__extension__ ({ _STRUCT_ARM_THREAD_STATE64 *__tsp = &(ts); \
	void *__p = (void*)(uintptr_t)(ptr); __tsp->__opaque_fp =   \
	(__p && !(__tsp->__opaque_flags &                           \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH) ?             \
	ptrauth_sign_unauthenticated(__p,                           \
	ptrauth_key_process_independent_data,                       \
	ptrauth_string_discriminator("fp")) : __p); })

                                                                              
#define __darwin_arm_thread_state64_ptrauth_strip(ts) \
	__extension__ ({ _STRUCT_ARM_THREAD_STATE64 *__tsp = &(ts);               \
	__tsp->__opaque_pc = ((__tsp->__opaque_flags &                            \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH) ? __tsp->__opaque_pc :      \
	ptrauth_strip(__tsp->__opaque_pc, ptrauth_key_process_independent_code)); \
	__tsp->__opaque_lr = ((__tsp->__opaque_flags &                            \
	(__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH |                           \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_IB_SIGNED_LR)) ? __tsp->__opaque_lr :   \
	ptrauth_strip(__tsp->__opaque_lr, ptrauth_key_process_independent_code)); \
	__tsp->__opaque_sp = ((__tsp->__opaque_flags &                            \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH) ? __tsp->__opaque_sp :      \
	ptrauth_strip(__tsp->__opaque_sp, ptrauth_key_process_independent_data)); \
	__tsp->__opaque_fp = ((__tsp->__opaque_flags &                            \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH) ? __tsp->__opaque_fp :      \
	ptrauth_strip(__tsp->__opaque_fp, ptrauth_key_process_independent_data)); \
	__tsp->__opaque_flags |=                                                  \
	__DARWIN_ARM_THREAD_STATE64_FLAGS_NO_PTRAUTH; })

#else                                                        

#if __DARWIN_OPAQUE_ARM_THREAD_STATE64

#ifndef __LP64__
#error "Invalid configuration"
#endif

                                                                     
#define __darwin_arm_thread_state64_get_pc(ts) \
	((uintptr_t)((ts).__opaque_pc))
                                                                   
#define __darwin_arm_thread_state64_get_pc_fptr(ts) \
	((ts).__opaque_pc)
                                                                
#define __darwin_arm_thread_state64_set_pc_fptr(ts, fptr) \
	((ts).__opaque_pc = (fptr))
                                                                     
#define __darwin_arm_thread_state64_get_lr(ts) \
	((uintptr_t)((ts).__opaque_lr))
                                                                   
#define __darwin_arm_thread_state64_get_lr_fptr(ts) \
	((ts).__opaque_lr)
                                                                
#define __darwin_arm_thread_state64_set_lr_fptr(ts, fptr) \
	((ts).__opaque_lr = (fptr))
                                                                     
#define __darwin_arm_thread_state64_get_sp(ts) \
	((uintptr_t)((ts).__opaque_sp))
                                                                  
#define __darwin_arm_thread_state64_set_sp(ts, ptr) \
	((ts).__opaque_sp = (void*)(uintptr_t)(ptr))
                                                                     
#define __darwin_arm_thread_state64_get_fp(ts) \
	((uintptr_t)((ts).__opaque_fp))
                                                                  
#define __darwin_arm_thread_state64_set_fp(ts, ptr) \
	((ts).__opaque_fp = (void*)(uintptr_t)(ptr))
                                                                              
#define __darwin_arm_thread_state64_ptrauth_strip(ts) \
	(void)(ts)

#else                                         
#if __DARWIN_UNIX03

                                                                     
#define __darwin_arm_thread_state64_get_pc(ts) \
	((ts).__pc)
                                                                   
#define __darwin_arm_thread_state64_get_pc_fptr(ts) \
	((void*)(uintptr_t)((ts).__pc))
                                                                
#define __darwin_arm_thread_state64_set_pc_fptr(ts, fptr) \
	((ts).__pc = (uintptr_t)(fptr))
                                                                     
#define __darwin_arm_thread_state64_get_lr(ts) \
	((ts).__lr)
                                                                   
#define __darwin_arm_thread_state64_get_lr_fptr(ts) \
	((void*)(uintptr_t)((ts).__lr))
                                                                
#define __darwin_arm_thread_state64_set_lr_fptr(ts, fptr) \
	((ts).__lr = (uintptr_t)(fptr))
                                                                     
#define __darwin_arm_thread_state64_get_sp(ts) \
	((ts).__sp)
                                                                  
#define __darwin_arm_thread_state64_set_sp(ts, ptr) \
	((ts).__sp = (uintptr_t)(ptr))
                                                                     
#define __darwin_arm_thread_state64_get_fp(ts) \
	((ts).__fp)
                                                                  
#define __darwin_arm_thread_state64_set_fp(ts, ptr) \
	((ts).__fp = (uintptr_t)(ptr))
                                                                              
#define __darwin_arm_thread_state64_ptrauth_strip(ts) \
	(void)(ts)

#else                      

                                                                     
#define __darwin_arm_thread_state64_get_pc(ts) \
	((ts).pc)
                                                                   
#define __darwin_arm_thread_state64_get_pc_fptr(ts) \
	((void*)(uintptr_t)((ts).pc))
                                                                
#define __darwin_arm_thread_state64_set_pc_fptr(ts, fptr) \
	((ts).pc = (uintptr_t)(fptr))
                                                                     
#define __darwin_arm_thread_state64_get_lr(ts) \
	((ts).lr)
                                                                   
#define __darwin_arm_thread_state64_get_lr_fptr(ts) \
	((void*)(uintptr_t)((ts).lr))
                                                                
#define __darwin_arm_thread_state64_set_lr_fptr(ts, fptr) \
	((ts).lr = (uintptr_t)(fptr))
                                                                     
#define __darwin_arm_thread_state64_get_sp(ts) \
	((ts).sp)
                                                                  
#define __darwin_arm_thread_state64_set_sp(ts, ptr) \
	((ts).sp = (uintptr_t)(ptr))
                                                                     
#define __darwin_arm_thread_state64_get_fp(ts) \
	((ts).fp)
                                                                  
#define __darwin_arm_thread_state64_set_fp(ts, ptr) \
	((ts).fp = (uintptr_t)(ptr))
                                                                              
#define __darwin_arm_thread_state64_ptrauth_strip(ts) \
	(void)(ts)

#endif                      
#endif                                         

#endif                                                        
#endif                                                                
#endif                       

#if __DARWIN_UNIX03
#define _STRUCT_ARM_VFP_STATE struct __darwin_arm_vfp_state
_STRUCT_ARM_VFP_STATE
{
	__uint32_t __r[64];
	__uint32_t __fpscr;
};
#else                       
#define _STRUCT_ARM_VFP_STATE struct arm_vfp_state
_STRUCT_ARM_VFP_STATE
{
	__uint32_t r[64];
	__uint32_t fpscr;
};
#endif                      

#if __DARWIN_UNIX03
#define _STRUCT_ARM_NEON_STATE64 struct __darwin_arm_neon_state64
#define _STRUCT_ARM_NEON_STATE   struct __darwin_arm_neon_state

#if defined(__arm64__)
_STRUCT_ARM_NEON_STATE64
{
	__uint128_t __v[32];
	__uint32_t  __fpsr;
	__uint32_t  __fpcr;
};

_STRUCT_ARM_NEON_STATE
{
	__uint128_t __v[16];
	__uint32_t  __fpsr;
	__uint32_t  __fpcr;
};
#elif defined(__arm__)
  
                                                         
   
_STRUCT_ARM_NEON_STATE64
{
	char opaque[(32 * 16) + (2 * sizeof(__uint32_t))];
} __attribute__((aligned(16)));

_STRUCT_ARM_NEON_STATE
{
	char opaque[(16 * 16) + (2 * sizeof(__uint32_t))];
} __attribute__((aligned(16)));

#else
#error Unknown architecture.
#endif

#else                       
#define _STRUCT_ARM_NEON_STATE64 struct arm_neon_state64
#define _STRUCT_ARM_NEON_STATE struct arm_neon_state

#if defined(__arm64__)
_STRUCT_ARM_NEON_STATE64
{
	__uint128_t q[32];
	uint32_t    fpsr;
	uint32_t    fpcr;
};

_STRUCT_ARM_NEON_STATE
{
	__uint128_t q[16];
	uint32_t    fpsr;
	uint32_t    fpcr;
};
#elif defined(__arm__)
  
                                                         
   
_STRUCT_ARM_NEON_STATE64
{
	char opaque[(32 * 16) + (2 * sizeof(__uint32_t))];
} __attribute__((aligned(16)));

_STRUCT_ARM_NEON_STATE
{
	char opaque[(16 * 16) + (2 * sizeof(__uint32_t))];
} __attribute__((aligned(16)));

#else
#error Unknown architecture.
#endif

#endif                      


#define _STRUCT_ARM_PAGEIN_STATE struct __arm_pagein_state
_STRUCT_ARM_PAGEIN_STATE
{
	int __pagein_error;
};

  
              
   
#if defined(__arm__)
                                               

#if __DARWIN_UNIX03
#define _STRUCT_ARM_DEBUG_STATE struct __darwin_arm_debug_state
_STRUCT_ARM_DEBUG_STATE
{
	__uint32_t __bvr[16];
	__uint32_t __bcr[16];
	__uint32_t __wvr[16];
	__uint32_t __wcr[16];
};
#else                       
#define _STRUCT_ARM_DEBUG_STATE struct arm_debug_state
_STRUCT_ARM_DEBUG_STATE
{
	__uint32_t bvr[16];
	__uint32_t bcr[16];
	__uint32_t wvr[16];
	__uint32_t wcr[16];
};
#endif                      

#elif defined(__arm64__)

                                                             

#if __DARWIN_UNIX03
#define _STRUCT_ARM_LEGACY_DEBUG_STATE struct __arm_legacy_debug_state
_STRUCT_ARM_LEGACY_DEBUG_STATE
{
	__uint32_t __bvr[16];
	__uint32_t __bcr[16];
	__uint32_t __wvr[16];
	__uint32_t __wcr[16];
};
#else                      
#define _STRUCT_ARM_LEGACY_DEBUG_STATE struct arm_legacy_debug_state
_STRUCT_ARM_LEGACY_DEBUG_STATE
{
	__uint32_t bvr[16];
	__uint32_t bcr[16];
	__uint32_t wvr[16];
	__uint32_t wcr[16];
};
#endif                      
#else
#error unknown architecture
#endif

#if __DARWIN_UNIX03
#define _STRUCT_ARM_DEBUG_STATE32 struct __darwin_arm_debug_state32
_STRUCT_ARM_DEBUG_STATE32
{
	__uint32_t __bvr[16];
	__uint32_t __bcr[16];
	__uint32_t __wvr[16];
	__uint32_t __wcr[16];
	__uint64_t __mdscr_el1;                                         
};

#define _STRUCT_ARM_DEBUG_STATE64 struct __darwin_arm_debug_state64
_STRUCT_ARM_DEBUG_STATE64
{
	__uint64_t __bvr[16];
	__uint64_t __bcr[16];
	__uint64_t __wvr[16];
	__uint64_t __wcr[16];
	__uint64_t __mdscr_el1;                                         
};
#else                       
#define _STRUCT_ARM_DEBUG_STATE32 struct arm_debug_state32
_STRUCT_ARM_DEBUG_STATE32
{
	__uint32_t bvr[16];
	__uint32_t bcr[16];
	__uint32_t wvr[16];
	__uint32_t wcr[16];
	__uint64_t mdscr_el1;                                         
};

#define _STRUCT_ARM_DEBUG_STATE64 struct arm_debug_state64
_STRUCT_ARM_DEBUG_STATE64
{
	__uint64_t bvr[16];
	__uint64_t bcr[16];
	__uint64_t wvr[16];
	__uint64_t wcr[16];
	__uint64_t mdscr_el1;                                         
};
#endif                      

#if __DARWIN_UNIX03
#define _STRUCT_ARM_CPMU_STATE64 struct __darwin_arm_cpmu_state64
_STRUCT_ARM_CPMU_STATE64
{
	__uint64_t __ctrs[16];
};
#else                      
#define _STRUCT_ARM_CPMU_STATE64 struct arm_cpmu_state64
_STRUCT_ARM_CPMU_STATE64
{
	__uint64_t ctrs[16];
};
#endif                       

#endif                            
