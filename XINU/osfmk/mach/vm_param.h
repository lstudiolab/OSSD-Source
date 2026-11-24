/*
 * Copyright (c) 2000-2020 Apple Computer, Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
   
  
                        
                                                    
             
  
                                                 
  
   

#ifndef _MACH_VM_PARAM_H_
#define _MACH_VM_PARAM_H_

#include <mach/machine/vm_param.h>

#ifdef  KERNEL

#ifndef ASSEMBLER
#include <mach/vm_types.h>
#endif                 

#include <os/base.h>
#include <os/overflow.h>

  
                                                                 
                                                                     
   

#ifndef ASSEMBLER

#define PAGE_SIZE_64 (unsigned long long)PAGE_SIZE                                          
#define PAGE_MASK_64 (unsigned long long)PAGE_MASK                                        

  
                                                                   
                                                                         
                                                                          
   

#define atop_32(x) ((uint32_t)(x) >> PAGE_SHIFT)
#define ptoa_32(x) ((uint32_t)(x) << PAGE_SHIFT)
#define atop_64(x) ((uint64_t)(x) >> PAGE_SHIFT)
#define ptoa_64(x) ((uint64_t)(x) << PAGE_SHIFT)

#define atop_kernel(x) ((vm_address_t)(x) >> PAGE_SHIFT)
#define ptoa_kernel(x) ((vm_address_t)(x) << PAGE_SHIFT)

  
                                                                      
                                                                 
                              
   

#if 1
#define atop(x) ((vm_address_t)(x) >> PAGE_SHIFT)
#define ptoa(x) ((vm_address_t)(x) << PAGE_SHIFT)
#else
#define atop(x) (0UL = 0)
#define ptoa(x) (0UL = 0)
#endif

  
                                                                 
   
#define mach_vm_round_page(x) (((mach_vm_offset_t)(x) + PAGE_MASK) & ~((signed)PAGE_MASK))
#define mach_vm_trunc_page(x) ((mach_vm_offset_t)(x) & ~((signed)PAGE_MASK))

#define round_page_overflow(in, out) __os_warn_unused(({ \
	        bool __ovr = os_add_overflow(in, (__typeof__(*out))PAGE_MASK, out); \
	        *out &= ~((__typeof__(*out))PAGE_MASK); \
	        __ovr; \
	}))

static inline int OS_WARN_RESULT
mach_vm_round_page_overflow(mach_vm_offset_t in, mach_vm_offset_t *out)
{
	return round_page_overflow(in, out);
}

#define memory_object_round_page(x) (((memory_object_offset_t)(x) + PAGE_MASK) & ~((signed)PAGE_MASK))
#define memory_object_trunc_page(x) ((memory_object_offset_t)(x) & ~((signed)PAGE_MASK))

  
                                                                   
                                
   

#define round_page(x) (((vm_offset_t)(x) + PAGE_MASK) & ~((vm_offset_t)PAGE_MASK))
#define trunc_page(x) ((vm_offset_t)(x) & ~((vm_offset_t)PAGE_MASK))

  
                                                              
                                                                 
                                                                   
                                                                        
                               
  
                                                                     
                                                                  
                                                       
   

#define round_page_32(x) (((uint32_t)(x) + PAGE_MASK) & ~((uint32_t)PAGE_MASK))
#define trunc_page_32(x) ((uint32_t)(x) & ~((uint32_t)PAGE_MASK))
#define round_page_64(x) (((uint64_t)(x) + PAGE_MASK_64) & ~((uint64_t)PAGE_MASK_64))
#define trunc_page_64(x) ((uint64_t)(x) & ~((uint64_t)PAGE_MASK_64))

#define round_page_mask_32(x, mask) (((uint32_t)(x) + (mask)) & ~((uint32_t)(mask)))
#define trunc_page_mask_32(x, mask) ((uint32_t)(x) & ~((uint32_t)(mask)))
#define round_page_mask_64(x, mask) (((uint64_t)(x) + (mask)) & ~((uint64_t)(mask)))
#define trunc_page_mask_64(x, mask) ((uint64_t)(x) & ~((uint64_t)(mask)))

  
                                                                            
                                                                             
                                                                       
                            
  
                                                                            
                                                                            
                                                                              
                               
   

#if 0
#undef atop_32
#undef ptoa_32
#undef round_page_32
#undef trunc_page_32
#undef atop_64
#undef ptoa_64
#undef round_page_64
#undef trunc_page_64

#ifndef __cplusplus

#define atop_32(x) \
    (__builtin_choose_expr (sizeof(x) != sizeof(uint64_t), \
	(*(long *)0), \
	(0UL)) = 0)

#define ptoa_32(x) \
    (__builtin_choose_expr (sizeof(x) != sizeof(uint64_t), \
	(*(long *)0), \
	(0UL)) = 0)

#define round_page_32(x) \
    (__builtin_choose_expr (sizeof(x) != sizeof(uint64_t), \
	(*(long *)0), \
	(0UL)) = 0)

#define trunc_page_32(x) \
    (__builtin_choose_expr (sizeof(x) != sizeof(uint64_t), \
	(*(long *)0), \
	(0UL)) = 0)
#else

#define atop_32(x) (0)
#define ptoa_32(x) (0)
#define round_page_32(x) (0)
#define trunc_page_32(x) (0)

#endif                    

#define atop_64(x) ((uint64_t)((x) + (uint8_t *)0))
#define ptoa_64(x) ((uint64_t)((x) + (uint8_t *)0))
#define round_page_64(x) ((uint64_t)((x) + (uint8_t *)0))
#define trunc_page_64(x) ((uint64_t)((x) + (uint8_t *)0))

#endif

  
                                                              
                          
   

#define page_aligned(x) (((x) & PAGE_MASK) == 0)

extern vm_size_t        mem_size;                                                                           
extern uint64_t         max_mem;                                                               

  
                                                                       
                                                                           
                                                                       
                                           
   
#define ANON_MAX_SIZE   ((1ULL << 32) - PAGE_SIZE)
  
                                           
                                                                    
                                                                         
                             
   
#define ANON_CHUNK_SIZE (128ULL * 1024 * 1024)            

  
                                                       
                                                        
                                                          
                                                                 
                                                                        
   
#define MALLOC_MEDIUM_CHUNK_SIZE (8ULL * 1024 * 1024)           

#ifdef  XNU_KERNEL_PRIVATE

#include <kern/debug.h>

extern uint64_t         mem_actual;                                                                
extern uint64_t         max_mem_actual;                                                         
extern uint64_t         sane_size;                                                                
extern addr64_t         vm_last_addr;                                                              

extern const vm_offset_t        vm_min_kernel_address;
extern const vm_offset_t        vm_max_kernel_address;

extern vm_offset_t              vm_kernel_stext;
extern vm_offset_t              vm_kernel_etext;
extern vm_offset_t              vm_kernel_slid_base;
extern vm_offset_t              vm_kernel_slid_top;
extern vm_offset_t              vm_kernel_slide;
extern vm_offset_t              vm_kernel_addrperm;
extern vm_offset_t              vm_kext_base;
extern vm_offset_t              vm_kext_top;
extern vm_offset_t              vm_kernel_base;
extern vm_offset_t              vm_kernel_top;
extern vm_offset_t              vm_hib_base;

extern vm_offset_t              vm_kernel_builtinkmod_text;
extern vm_offset_t              vm_kernel_builtinkmod_text_end;

#define VM_KERNEL_IS_SLID(_o)                                             \
	(((vm_offset_t)VM_KERNEL_STRIP_PTR(_o) >= vm_kernel_slid_base) && \
	 ((vm_offset_t)VM_KERNEL_STRIP_PTR(_o) <  vm_kernel_slid_top))

#define VM_KERNEL_SLIDE(_u) ((vm_offset_t)(_u) + vm_kernel_slide)

  
                                                                        
                                                                             
                                                                           
                                                                            
                                                                         
                                                                              
                                                               
  
                     
                                                                            
                                                                                  
                                                                              
                                                                               
                              
  
                      
                                                                              
                                                                            
                                                                    
  
                      
                                                                             
                                                                                
                                                                        
  
  
                                                                     
                                                                  
  
                      
                             
                                                                               
                                                                                
                                                                          
  
                                                        
   

__BEGIN_DECLS
#if XNU_KERNEL_PRIVATE
extern vm_offset_t vm_kernel_addrhash(vm_offset_t addr)
__XNU_INTERNAL(vm_kernel_addrhash);
#else
extern vm_offset_t vm_kernel_addrhash(vm_offset_t addr);
#endif
__END_DECLS

#define __DO_UNSLIDE(_v) ((vm_offset_t)VM_KERNEL_STRIP_PTR(_v) - vm_kernel_slide)

#if DEBUG || DEVELOPMENT
#define VM_KERNEL_ADDRHIDE(_v) (VM_KERNEL_IS_SLID(_v) ? __DO_UNSLIDE(_v) : (vm_address_t)VM_KERNEL_STRIP_PTR(_v))
#else
#define VM_KERNEL_ADDRHIDE(_v) (VM_KERNEL_IS_SLID(_v) ? __DO_UNSLIDE(_v) : (vm_address_t)0)
#endif                           

#define VM_KERNEL_ADDRHASH(_v) vm_kernel_addrhash((vm_offset_t)(_v))

#define VM_KERNEL_UNSLIDE_OR_PERM(_v) ({ \
	        VM_KERNEL_IS_SLID(_v) ? __DO_UNSLIDE(_v) : \
	        VM_KERNEL_ADDRESS(_v) ? ((vm_offset_t)VM_KERNEL_STRIP_PTR(_v) + vm_kernel_addrperm) : \
	        (vm_offset_t)VM_KERNEL_STRIP_PTR(_v); \
	})

#define VM_KERNEL_UNSLIDE(_v) ({ \
	        VM_KERNEL_IS_SLID(_v) ? __DO_UNSLIDE(_v) : (vm_offset_t)0; \
	})

#define VM_KERNEL_ADDRPERM(_v) VM_KERNEL_UNSLIDE_OR_PERM(_v)

#undef mach_vm_round_page
#undef round_page
#undef round_page_32
#undef round_page_64

static inline mach_vm_offset_t
mach_vm_round_page(mach_vm_offset_t x)
{
	if (round_page_overflow(x, &x)) {
		panic("overflow detected");
	}
	return x;
}

static inline vm_offset_t
round_page(vm_offset_t x)
{
	if (round_page_overflow(x, &x)) {
		panic("overflow detected");
	}
	return x;
}

static inline mach_vm_offset_t
round_page_64(mach_vm_offset_t x)
{
	if (round_page_overflow(x, &x)) {
		panic("overflow detected");
	}
	return x;
}

static inline uint32_t
round_page_32(uint32_t x)
{
	if (round_page_overflow(x, &x)) {
		panic("overflow detected");
	}
	return x;
}


   
                               
  
         
                                                                                
            
  
              
                                                                          
                                   
  
                                            
                                                             
                                                                       
                                                  
  
                                                                       
                   
  
  
                                                                   
                                                               
  
                                                                       
                                                                               
  
  
                                                            
  
                                                                                
                                                                               
  
                                                  
  
                                                                  
  
                                                                          
                                             
  
                                                                               
                   
  
                                                                              
               
   
typedef struct vm_packing_params {
	vm_offset_t vmpp_base;
	uint8_t     vmpp_bits;
	uint8_t     vmpp_shift;
	bool        vmpp_base_relative;
} vm_packing_params_t;


   
                                     
  
         
                                                                          
   
#define VM_PACKING_IS_BASE_RELATIVE(ns) \
	(ns##_BITS + ns##_SHIFT <= VM_KERNEL_POINTER_SIGNIFICANT_BITS)


   
                           
  
         
                                                                             
                                                                              
                        
   
#define VM_PACKING_PARAMS(ns) \
	(vm_packing_params_t){ \
	    .vmpp_base  = ns##_BASE, \
	    .vmpp_bits  = ns##_BITS, \
	    .vmpp_shift = ns##_SHIFT, \
	    .vmpp_base_relative = VM_PACKING_IS_BASE_RELATIVE(ns), \
	}

   
                            
  
         
                                                         
  
              
                                                                
                          
  
                                            
                                                
                                           
   
static inline vm_offset_t
vm_pack_pointer(vm_offset_t ptr, vm_packing_params_t params)
{
	if (!params.vmpp_base_relative) {
		return ptr >> params.vmpp_shift;
	}
	if (ptr) {
		return (ptr - params.vmpp_base) >> params.vmpp_shift;
	}
	return (vm_offset_t)0;
}
#define VM_PACK_POINTER(ptr, ns) \
	vm_pack_pointer(ptr, VM_PACKING_PARAMS(ns))

   
                              
  
         
                                                      
  
              
                                                                  
                          
  
                                                   
                                                
                                             
   
static inline vm_offset_t
vm_unpack_pointer(vm_offset_t packed, vm_packing_params_t params)
{
	if (!params.vmpp_base_relative) {
		intptr_t addr = (intptr_t)packed;
		addr <<= __WORDSIZE - params.vmpp_bits;
		addr >>= __WORDSIZE - params.vmpp_bits - params.vmpp_shift;
		return (vm_offset_t)addr;
	}
	if (packed) {
		return (packed << params.vmpp_shift) + params.vmpp_base;
	}
	return (vm_offset_t)0;
}
#define VM_UNPACK_POINTER(packed, ns) \
	vm_unpack_pointer(packed, VM_PACKING_PARAMS(ns))

   
                                    
  
         
                                                                 
  
              
                                                                        
                          
  
                                                
                                                     
   
static inline vm_offset_t
vm_packing_max_packable(vm_packing_params_t params)
{
	if (!params.vmpp_base_relative) {
		return VM_MAX_KERNEL_ADDRESS;
	}

	vm_offset_t ptr = params.vmpp_base +
	    (((1ul << params.vmpp_bits) - 1) << params.vmpp_shift);

	return ptr >= params.vmpp_base ? ptr : VM_MAX_KERNEL_ADDRESS;
}
#define VM_PACKING_MAX_PACKABLE(ns) \
	vm_packing_max_packable(VM_PACKING_PARAMS(ns))


__abortlike
extern void
vm_packing_pointer_invalid(vm_offset_t ptr, vm_packing_params_t params);

   
                                       
  
         
                                                                      
              
  
              
                                                                           
                          
  
                                                                           
                                                                      
  
                                                   
                                                
   
static inline void
vm_verify_pointer_packable(vm_offset_t ptr, vm_packing_params_t params)
{
	if (ptr & ((1ul << params.vmpp_shift) - 1)) {
		vm_packing_pointer_invalid(ptr, params);
	}
	if (!params.vmpp_base_relative || ptr == 0) {
		return;
	}
	if (ptr <= params.vmpp_base || ptr > vm_packing_max_packable(params)) {
		vm_packing_pointer_invalid(ptr, params);
	}
}
#define VM_VERIFY_POINTER_PACKABLE(ptr, ns) \
	vm_verify_pointer_packable(ptr, VM_PACKING_PARAMS(ns))

#if DEBUG || DEVELOPMENT
#define VM_ASSERT_POINTER_PACKABLE(ptr, ns) \
    VM_VERIFY_POINTER_PACKABLE(ptr, ns)
#else
#define VM_ASSERT_POINTER_PACKABLE(ptr, ns) ((void)(ptr))
#endif

   
                                    
  
         
                                                                          
                        
  
                                                            
                                                          
                                                         
                                                
   
extern void
vm_packing_verify_range(
	const char         *subsystem,
	vm_offset_t         min_address,
	vm_offset_t         max_address,
	vm_packing_params_t params);

#endif                          

extern vm_size_t        page_size;
extern vm_size_t        page_mask;
extern int              page_shift;

                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
#include <stdint.h>
#ifndef __CAST_DOWN_CHECK
#define __CAST_DOWN_CHECK

#define CAST_DOWN( type, addr ) \
    ( ((type)((uintptr_t) (addr)/(sizeof(type) < sizeof(uintptr_t) ? 0 : 1))) )

#define CAST_DOWN_EXPLICIT( type, addr )  ( ((type)((uintptr_t) (addr))) )

#endif                        

#endif                 

#endif              

#endif                         
