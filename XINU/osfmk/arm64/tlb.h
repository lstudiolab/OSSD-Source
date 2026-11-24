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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#pragma once

#include <arm64/proc_reg.h>
#include <machine/atomic.h>

#define tlbi_addr(x) ((((x) >> 12) & TLBI_ADDR_MASK) << TLBI_ADDR_SHIFT)
#define tlbi_asid(x) (((uintptr_t)(x) & TLBI_ASID_MASK) << TLBI_ASID_SHIFT)

#if __ARM_KERNEL_PROTECT__
  
                                                                   
  
                                                                               
                                                              
  
                                                                               
                                                                              
                                                                              
                                               
   
#endif                             

static inline void
sync_tlb_flush(void)
{
	__builtin_arm_dsb(DSB_ISH);
	__builtin_arm_isb(ISB_SY);
}

static inline void
sync_tlb_flush_local(void)
{
	__builtin_arm_dsb(DSB_NSH);
	__builtin_arm_isb(ISB_SY);
}

                                             
static inline void
flush_mmu_tlb_async(void)
{
	asm volatile ("tlbi vmalle1is");
}

static inline void
flush_mmu_tlb(void)
{
	flush_mmu_tlb_async();
	sync_tlb_flush();
}

                                                    
static inline void
flush_core_tlb_async(void)
{
	asm volatile ("tlbi vmalle1");
}

static inline void
flush_core_tlb(void)
{
	flush_core_tlb_async();
	sync_tlb_flush();
}

                                                                                        
                                          
static inline void
flush_mmu_tlb_allentries_async(uint64_t start, uint64_t end, uint64_t pmap_page_size)
{
#if __ARM_16K_PG__
	if (pmap_page_size == 16384) {
		start = start & ~0x3ULL;

		  
                                                                    
                                                                   
                                                                     
                                                                   
                                                                  
                                                                  
                                                                     
                                                                     
                      
    
                                                                     
                                                                  
           
     
		end = (end + 0x3ULL) & ~0x3ULL;
	}
#endif                  
	for (; start < end; start += (pmap_page_size / 4096)) {
		asm volatile ("tlbi vaae1is, %0" : : "r"(start));
	}
}

static inline void
flush_mmu_tlb_allentries(uint64_t start, uint64_t end, uint64_t pmap_page_size)
{
	flush_mmu_tlb_allentries_async(start, end, pmap_page_size);
	sync_tlb_flush();
}

                                                                           
                                                   
static inline void
flush_mmu_tlb_entry_async(uint64_t val)
{
#if __ARM_KERNEL_PROTECT__
	uint64_t asid = val >> TLBI_ASID_SHIFT;
	if (asid == 0) {
		asm volatile ("tlbi vaae1is, %0" : : "r"(val));
		return;
	}
	val = val & ~(1ULL << TLBI_ASID_SHIFT);
	asm volatile ("tlbi vae1is, %0" : : "r"(val));
	val = val | (1ULL << TLBI_ASID_SHIFT);
#endif                             
	asm volatile ("tlbi vae1is, %0" : : "r"(val));
}

static inline void
flush_mmu_tlb_entry(uint64_t val)
{
	flush_mmu_tlb_entry_async(val);
	sync_tlb_flush();
}

                                                                                   
                                                                                                      
                                                         
static inline void
flush_mmu_tlb_entries_async(uint64_t start, uint64_t end, uint64_t pmap_page_size)
{
#if __ARM_16K_PG__
	if (pmap_page_size == 16384) {
		start = start & ~0x3ULL;

		  
                                                                    
                                                                   
                                                                     
                                                                   
                                                                  
                                                                  
                                                                     
                                                                     
                      
    
                                                                     
                                                                  
           
     
		end = (end + 0x3ULL) & ~0x3ULL;
	}
#endif                  
#if __ARM_KERNEL_PROTECT__
	uint64_t asid = start >> TLBI_ASID_SHIFT;
	  
                                                                     
                                                      
    
	if (asid == 0) {
		for (; start < end; start += (pmap_page_size / 4096)) {
			asm volatile ("tlbi vaae1is, %0" : : "r"(start));
		}
		return;
	}
	start = start | (1ULL << TLBI_ASID_SHIFT);
	end = end | (1ULL << TLBI_ASID_SHIFT);
	for (; start < end; start += (pmap_page_size / 4096)) {
		start = start & ~(1ULL << TLBI_ASID_SHIFT);
		asm volatile ("tlbi vae1is, %0" : : "r"(start));
		start = start | (1ULL << TLBI_ASID_SHIFT);
		asm volatile ("tlbi vae1is, %0" : : "r"(start));
	}
#else
	for (; start < end; start += (pmap_page_size / 4096)) {
		asm volatile ("tlbi vae1is, %0" : : "r"(start));
	}
#endif                             
}

static inline void
flush_mmu_tlb_entries(uint64_t start, uint64_t end, uint64_t pmap_page_size)
{
	flush_mmu_tlb_entries_async(start, end, pmap_page_size);
	sync_tlb_flush();
}

                                                                         
                                           
                                
static inline void
flush_mmu_tlb_asid_async(uint64_t val)
{
#if __ARM_KERNEL_PROTECT__
	  
                                                                     
                                                      
    
	uint64_t asid = val >> TLBI_ASID_SHIFT;
	if (asid == 0) {
		asm volatile ("tlbi vmalle1is");
		return;
	}
	val = val & ~(1ULL << TLBI_ASID_SHIFT);
	asm volatile ("tlbi aside1is, %0" : : "r"(val));
	val = val | (1ULL << TLBI_ASID_SHIFT);
#endif                             
	asm volatile ("tlbi aside1is, %0" : : "r"(val));
}

static inline void
flush_mmu_tlb_asid(uint64_t val)
{
	flush_mmu_tlb_asid_async(val);
	sync_tlb_flush();
}

                                                                             
                                           
                                
static inline void
flush_core_tlb_asid_async(uint64_t val)
{
#if __ARM_KERNEL_PROTECT__
	  
                                                                     
                                                      
    
	uint64_t asid = val >> TLBI_ASID_SHIFT;
	if (asid == 0) {
		asm volatile ("tlbi vmalle1");
		return;
	}
	val = val & ~(1ULL << TLBI_ASID_SHIFT);
	asm volatile ("tlbi aside1, %0" : : "r"(val));
	val = val | (1ULL << TLBI_ASID_SHIFT);
#endif                             
	asm volatile ("tlbi aside1, %0" : : "r"(val));
}

static inline void
flush_core_tlb_asid(uint64_t val)
{
	flush_core_tlb_asid_async(val);
	sync_tlb_flush();
}

#if __ARM_RANGE_TLBI__
#if __ARM_KERNEL_PROTECT__
	#error __ARM_RANGE_TLBI__ + __ARM_KERNEL_PROTECT__ is not currently supported
#endif

#define ARM64_TLB_RANGE_PAGES (1ULL << 21)
#define rtlbi_addr(x, shift) (((x) >> (shift)) & RTLBI_ADDR_MASK)
#define rtlbi_scale(x) ((uint64_t)(x) << RTLBI_SCALE_SHIFT)
#define rtlbi_num(x) ((uint64_t)(x) << RTLBI_NUM_SHIFT)

   
                                                                             
                                            
   
static inline uint64_t
generate_rtlbi_param(ppnum_t npages, uint32_t asid, vm_offset_t va, uint64_t pmap_page_shift)
{
	   
                                                                                                        
                                                                                
    
	unsigned order = (unsigned)(sizeof(npages) * 8) - (unsigned)__builtin_clz(npages - 1) - 1;
	unsigned scale = ((order ? order : 1) - 1) / 5;
	unsigned granule = 1 << ((5 * scale) + 1);
	unsigned num = (((npages + granule - 1) & ~(granule - 1)) / granule) - 1;
	return tlbi_asid(asid) | RTLBI_TG(pmap_page_shift) | rtlbi_scale(scale) | rtlbi_num(num) | rtlbi_addr(va, pmap_page_shift);
}

                                                                                        
                                                                      
                                                                     
static inline void
flush_mmu_tlb_range_async(uint64_t val)
{
	asm volatile ("tlbi rvae1is, %0" : : "r"(val));
}

static inline void
flush_mmu_tlb_range(uint64_t val)
{
	flush_mmu_tlb_range_async(val);
	sync_tlb_flush();
}

                                                                                           
                                                                      
                                                                        
static inline void
flush_mmu_tlb_allrange_async(uint64_t val)
{
	asm volatile ("tlbi rvaae1is, %0" : : "r"(val));
}

static inline void
flush_mmu_tlb_allrange(uint64_t val)
{
	flush_mmu_tlb_allrange_async(val);
	sync_tlb_flush();
}

                                                                                                             
                                                                      
                                                                        
static inline void
flush_core_tlb_allrange_async(uint64_t val)
{
	asm volatile ("tlbi rvaae1, %0" : : "r"(val));
}

static inline void
flush_core_tlb_allrange(uint64_t val)
{
	flush_core_tlb_allrange_async(val);
	sync_tlb_flush();
}

#endif                      


