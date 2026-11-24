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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
  
                      
   

  
                                                   
   

#ifndef _MACH_ARM_VM_PARAM_H_
#define _MACH_ARM_VM_PARAM_H_

#if defined(KERNEL_PRIVATE) && __ARM_16K_PG__
#include <arm64/proc_reg.h>
#endif

#if !defined (KERNEL) && !defined (__ASSEMBLER__)
#include <mach/vm_page_size.h>
#endif

#define BYTE_SIZE       8                              

#if defined (KERNEL)

#ifndef __ASSEMBLER__

#ifdef  __arm__
#define PAGE_SHIFT_CONST        12
#elif defined(__arm64__)
extern int PAGE_SHIFT_CONST;
#else
#error Unsupported arch
#endif

#if defined(KERNEL_PRIVATE) && __ARM_16K_PG__
#define PAGE_SHIFT              ARM_PGSHIFT
#else
#define PAGE_SHIFT              PAGE_SHIFT_CONST
#endif
#define PAGE_SIZE               (1 << PAGE_SHIFT)
#define PAGE_MASK               (PAGE_SIZE-1)

#define VM_PAGE_SIZE            PAGE_SIZE

#define machine_ptob(x)         ((x) << PAGE_SHIFT)

  
                                                              
                                                                
   
#define TEST_PAGE_SIZE_16K      ((PAGE_SHIFT_CONST == 14))
#define TEST_PAGE_SIZE_4K       ((PAGE_SHIFT_CONST == 12))

#endif                      

#else

#define PAGE_SHIFT                      vm_page_shift
#define PAGE_SIZE                       vm_page_size
#define PAGE_MASK                       vm_page_mask

#define VM_PAGE_SIZE            vm_page_size

#define machine_ptob(x)         ((x) << PAGE_SHIFT)

#endif

#define PAGE_MAX_SHIFT          14
#define PAGE_MAX_SIZE           (1 << PAGE_MAX_SHIFT)
#define PAGE_MAX_MASK           (PAGE_MAX_SIZE-1)

#define PAGE_MIN_SHIFT          12
#define PAGE_MIN_SIZE           (1 << PAGE_MIN_SHIFT)
#define PAGE_MIN_MASK           (PAGE_MIN_SIZE-1)

#define VM_MAX_PAGE_ADDRESS     MACH_VM_MAX_ADDRESS

#ifndef __ASSEMBLER__

#ifdef  MACH_KERNEL_PRIVATE

#define VM32_SUPPORT            1
#define VM32_MIN_ADDRESS        ((vm32_offset_t) 0)
#define VM32_MAX_ADDRESS        ((vm32_offset_t) (VM_MAX_ADDRESS & 0xFFFFFFFF))

  
                       
  
                                                                       
                                                               
                                                        
  
                                                                             
                                                                             
  
                                                                         
                                                                             
   

#if defined (__arm__)

#define KALLOC_MINSIZE          8                                    
#define KALLOC_LOG2_MINALIGN    3                                   

#elif defined(__arm64__)

#define KALLOC_MINSIZE          16                                   
#define KALLOC_LOG2_MINALIGN    4                                   

#else
#error Unsupported arch
#endif

#if defined (__arm__)
                                                                
#define ZONE_MAP_MAX (1024 * 1024 * 1536)                              
#elif defined(__arm64__)
  
                                            
   
#define ZONE_MAP_MAX (31ULL << 30)                             
#else
#error Unsupported arch
#endif

#endif

#if defined (__arm__)

#define VM_MIN_ADDRESS          ((vm_address_t) 0x00000000)
#define VM_MAX_ADDRESS          ((vm_address_t) 0x80000000)

                        
#define MACH_VM_MIN_ADDRESS     ((mach_vm_offset_t) 0)
#define MACH_VM_MAX_ADDRESS     ((mach_vm_offset_t) VM_MAX_ADDRESS)

#elif defined (__arm64__)

#define VM_MIN_ADDRESS          ((vm_address_t) 0x0000000000000000ULL)
#define VM_MAX_ADDRESS          ((vm_address_t) 0x0000000080000000ULL)

                        
#define MACH_VM_MIN_ADDRESS_RAW 0x0ULL
#if defined(PLATFORM_MacOSX) || defined(PLATFORM_DriverKit)
#define MACH_VM_MAX_ADDRESS_RAW 0x00007FFFFE000000ULL
#else
#define MACH_VM_MAX_ADDRESS_RAW 0x0000000FC0000000ULL
#endif

#define MACH_VM_MIN_ADDRESS     ((mach_vm_offset_t) MACH_VM_MIN_ADDRESS_RAW)
#define MACH_VM_MAX_ADDRESS     ((mach_vm_offset_t) MACH_VM_MAX_ADDRESS_RAW)

#define MACH_VM_MIN_GPU_CARVEOUT_ADDRESS_RAW 0x0000001000000000ULL
#define MACH_VM_MAX_GPU_CARVEOUT_ADDRESS_RAW 0x0000007000000000ULL
#define MACH_VM_MIN_GPU_CARVEOUT_ADDRESS     ((mach_vm_offset_t) MACH_VM_MIN_GPU_CARVEOUT_ADDRESS_RAW)
#define MACH_VM_MAX_GPU_CARVEOUT_ADDRESS     ((mach_vm_offset_t) MACH_VM_MAX_GPU_CARVEOUT_ADDRESS_RAW)

#else                         
#error architecture not supported
#endif

#define VM_MAP_MIN_ADDRESS      VM_MIN_ADDRESS
#define VM_MAP_MAX_ADDRESS      VM_MAX_ADDRESS

#ifdef  KERNEL

#if defined (__arm__)
#define VM_KERNEL_POINTER_SIGNIFICANT_BITS  31
#define VM_MIN_KERNEL_ADDRESS   ((vm_address_t) 0x80000000)
#define VM_MAX_KERNEL_ADDRESS   ((vm_address_t) 0xFFFEFFFF)
#define VM_HIGH_KERNEL_WINDOW   ((vm_address_t) 0xFFFE0000)
#elif defined (__arm64__)
  
                                                                  
                                       
   
#define TiB(x) ((0ULL + (x)) << 40)
#define GiB(x) ((0ULL + (x)) << 30)

#if XNU_KERNEL_PRIVATE
#if defined(ARM_LARGE_MEMORY)
  
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
   
#define VM_KERNEL_POINTER_SIGNIFICANT_BITS  41

                                 
#define VM_MIN_KERNEL_ADDRESS   ((vm_address_t) (0ULL - TiB(2)))

                                                                          
#define VM_MAX_KERNEL_ADDRESS   ((vm_address_t) (VM_MIN_KERNEL_ADDRESS + GiB(64) + GiB(512) - 1))
#else                    
  
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                       
                                                                             
                                                                       
                                                                       
                                                                             
                                                                       
   
#define VM_KERNEL_POINTER_SIGNIFICANT_BITS  37
#define VM_MIN_KERNEL_ADDRESS   ((vm_address_t) 0xffffffe000000000ULL)
#define VM_MAX_KERNEL_ADDRESS   ((vm_address_t) 0xfffffffbffffffffULL)
#endif                    

#else                       
                                                           
#define VM_MIN_KERNEL_ADDRESS   ((vm_address_t) (0ULL - TiB(2)))
#define VM_MAX_KERNEL_ADDRESS   ((vm_address_t) 0xfffffffbffffffffULL)
#endif                      
#else
#error architecture not supported
#endif

#define VM_MIN_KERNEL_AND_KEXT_ADDRESS  VM_MIN_KERNEL_ADDRESS

#if __has_feature(ptrauth_calls)
#include <ptrauth.h>
#define VM_KERNEL_STRIP_PTR(_v) (ptrauth_strip((void *)(uintptr_t)(_v), ptrauth_key_asia))
#else                     
#define VM_KERNEL_STRIP_PTR(_v) (_v)
#endif                    

#define VM_KERNEL_ADDRESS(_va)  \
	((((vm_address_t)VM_KERNEL_STRIP_PTR(_va)) >= VM_MIN_KERNEL_ADDRESS) && \
	 (((vm_address_t)VM_KERNEL_STRIP_PTR(_va)) <= VM_MAX_KERNEL_ADDRESS))

#ifdef  MACH_KERNEL_PRIVATE
  
                                                                  
   
extern unsigned long            gVirtBase, gPhysBase, gPhysSize;

#define isphysmem(a)            (((vm_address_t)(a) - gPhysBase) < gPhysSize)
#define physmap_enclosed(a)     isphysmem(a)

  
                                                                               
                                                                                
        
   
#include <stdint.h>
extern uint64_t                 gDramBase, gDramSize;
#define is_dram_addr(addr)      (((uint64_t)(addr) - gDramBase) < gDramSize)

#if KASAN
                                                                               
                   
# define KERNEL_STACK_SIZE      (4*4*4096)
#elif DEBUG
   
                                                                                
                      
   
# define KERNEL_STACK_SIZE      (2*4*4096)
#else
  
                                                                    
                                                                       
                                                                
   
#ifndef KERNEL_STACK_MULTIPLIER
#define KERNEL_STACK_MULTIPLIER (1)
#endif                              
# define KERNEL_STACK_SIZE      (4*4096*KERNEL_STACK_MULTIPLIER)
#endif

#define INTSTACK_SIZE           (4*4096)

#ifdef __arm64__
#define EXCEPSTACK_SIZE         (4*4096)
#else
#define FIQSTACK_SIZE           (4096)
#endif

#if defined (__arm__)
#define HIGH_EXC_VECTORS        ((vm_address_t) 0xFFFF0000)
#endif

  
                                                              
                                                           
                                                          
                                      
   
#if defined (__arm__)
#define VM_KERNEL_LINK_ADDRESS  ((vm_address_t) 0x80000000)
#elif defined (__arm64__)
                                                                                
#else
#error architecture not supported
#endif

#endif                           
#endif              

#endif                      

#define SWI_SYSCALL     0x80

#endif                             
