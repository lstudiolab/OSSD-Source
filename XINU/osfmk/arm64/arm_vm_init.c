/*
 * Copyright (c) 2007-2011 Apple Inc. All rights reserved.
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

#include <mach_debug.h>
#include <mach_kdp.h>
#include <debug.h>

#include <kern/assert.h>
#include <kern/misc_protos.h>
#include <kern/monotonic.h>
#include <mach/vm_types.h>
#include <mach/vm_param.h>
#include <vm/vm_kern.h>
#include <vm/vm_page.h>
#include <vm/pmap.h>

#include <machine/atomic.h>
#include <arm64/proc_reg.h>
#include <arm64/lowglobals.h>
#include <arm/cpu_data_internal.h>
#include <arm/misc_protos.h>
#include <pexpert/arm64/boot.h>
#include <pexpert/device_tree.h>

#include <libkern/kernel_mach_header.h>
#include <libkern/section_keywords.h>

#include <san/kasan.h>

#if __ARM_KERNEL_PROTECT__
  
                                                                               
                                                                              
                                                                                
   
static_assert(KERNEL_PMAP_HEAP_RANGE_START > ARM_KERNEL_PROTECT_EXCEPTION_START);

                                                                  
static_assert((KERNEL_PMAP_HEAP_RANGE_START & ~ARM_TT_ROOT_OFFMASK) > ARM_KERNEL_PROTECT_EXCEPTION_START);

  
                                                                                
                         
   
static_assert((((~ARM_KERNEL_PROTECT_EXCEPTION_START) + 1) * 2ULL) <= (ARM_TT_ROOT_SIZE + ARM_TT_ROOT_INDEX_MASK));
#endif                             

#define ARM_DYNAMIC_TABLE_XN (ARM_TTE_TABLE_PXN | ARM_TTE_TABLE_XN)

#if KASAN
extern vm_offset_t shadow_pbase;
extern vm_offset_t shadow_ptop;
extern vm_offset_t physmap_vbase;
extern vm_offset_t physmap_vtop;
#endif

  
                                                                        
                                                                           
                             
   
SECURITY_READ_ONLY_LATE(uint8_t) bootstrap_pagetables[BOOTSTRAP_TABLE_SIZE] __attribute__((aligned(ARM_PGBYTES)));

  
                          
   
extern void *last_kernel_symbol;

extern void arm64_replace_bootstack(cpu_data_t*);
extern void PE_slide_devicetree(vm_offset_t);

  
                   
   
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kernel_base;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kernel_top;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kext_base;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kext_top;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kernel_stext;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kernel_etext;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kernel_slide;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kernel_slid_base;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kernel_slid_top;

SECURITY_READ_ONLY_LATE(vm_offset_t) vm_prelink_stext;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_prelink_etext;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_prelink_sdata;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_prelink_edata;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_prelink_sinfo;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_prelink_einfo;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_slinkedit;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_elinkedit;

SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kernel_builtinkmod_text;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kernel_builtinkmod_text_end;

SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kernelcache_base;
SECURITY_READ_ONLY_LATE(vm_offset_t) vm_kernelcache_top;

                                   
SECURITY_READ_ONLY_LATE(unsigned long) gVirtBase;
SECURITY_READ_ONLY_LATE(unsigned long) gPhysBase;
SECURITY_READ_ONLY_LATE(unsigned long) gPhysSize;
SECURITY_READ_ONLY_LATE(unsigned long) gT0Sz = T0SZ_BOOT;
SECURITY_READ_ONLY_LATE(unsigned long) gT1Sz = T1SZ_BOOT;

                                                                             
                                                                         
                                
                     
                                    
                                  
                                  
                                 
                                  
                                      
                                                   
                                    
                                           
   

                                     
  
                                                                             
                            
             
  
                 
         
               
              
        
         
         
                                  
             
                 
  
             
  
                                                       
                   
                  
         
               
              
        
                                                      
         
                          
             
                                          
                 
                 
  
   

vm_offset_t mem_size;                                                                      
                                                                                              
                                                                                     
uint64_t    mem_actual;                                                                 
                                                                                        
                                                                   
uint64_t    max_mem;                                                                          
                                                                 
uint64_t    max_mem_actual;                                                                 
                                                                                       
uint64_t    sane_size;                                                              
                                                                    
                                                 
addr64_t    vm_last_addr = VM_MAX_KERNEL_ADDRESS;                  
                                                                          
                                                                        

SECURITY_READ_ONLY_LATE(vm_offset_t)              segEXTRADATA;
SECURITY_READ_ONLY_LATE(unsigned long)            segSizeEXTRADATA;

SECURITY_READ_ONLY_LATE(vm_offset_t)          segLOWESTTEXT;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segLOWEST;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segLOWESTRO;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segHIGHESTRO;

                                                             
SECURITY_READ_ONLY_LATE(vm_offset_t)          segLOWESTKC;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segHIGHESTKC;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segLOWESTROKC;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segHIGHESTROKC;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segLOWESTAuxKC;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segHIGHESTAuxKC;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segLOWESTROAuxKC;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segHIGHESTROAuxKC;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segLOWESTRXAuxKC;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segHIGHESTRXAuxKC;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segHIGHESTNLEAuxKC;

SECURITY_READ_ONLY_LATE(static vm_offset_t)   segTEXTB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizeTEXT;

#if XNU_MONITOR
SECURITY_READ_ONLY_LATE(vm_offset_t)          segPPLTEXTB;
SECURITY_READ_ONLY_LATE(unsigned long)        segSizePPLTEXT;

SECURITY_READ_ONLY_LATE(vm_offset_t)          segPPLTRAMPB;
SECURITY_READ_ONLY_LATE(unsigned long)        segSizePPLTRAMP;

SECURITY_READ_ONLY_LATE(vm_offset_t)          segPPLDATACONSTB;
SECURITY_READ_ONLY_LATE(unsigned long)        segSizePPLDATACONST;
SECURITY_READ_ONLY_LATE(void *)               pmap_stacks_start = NULL;
SECURITY_READ_ONLY_LATE(void *)               pmap_stacks_end = NULL;
#endif

SECURITY_READ_ONLY_LATE(static vm_offset_t)   segDATACONSTB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizeDATACONST;

SECURITY_READ_ONLY_LATE(vm_offset_t)   segTEXTEXECB;
SECURITY_READ_ONLY_LATE(unsigned long) segSizeTEXTEXEC;

SECURITY_READ_ONLY_LATE(static vm_offset_t)   segDATAB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizeDATA;

#if XNU_MONITOR
SECURITY_READ_ONLY_LATE(vm_offset_t)          segPPLDATAB;
SECURITY_READ_ONLY_LATE(unsigned long)        segSizePPLDATA;
#endif

SECURITY_READ_ONLY_LATE(vm_offset_t)          segBOOTDATAB;
SECURITY_READ_ONLY_LATE(unsigned long)        segSizeBOOTDATA;
extern vm_offset_t                            intstack_low_guard;
extern vm_offset_t                            intstack_high_guard;
extern vm_offset_t                            excepstack_high_guard;

SECURITY_READ_ONLY_LATE(vm_offset_t)          segLINKB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizeLINK;

SECURITY_READ_ONLY_LATE(static vm_offset_t)   segKLDB;
SECURITY_READ_ONLY_LATE(unsigned long)        segSizeKLD;
SECURITY_READ_ONLY_LATE(static vm_offset_t)   segKLDDATAB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizeKLDDATA;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segLASTB;
SECURITY_READ_ONLY_LATE(unsigned long)        segSizeLAST;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segLASTDATACONSTB;
SECURITY_READ_ONLY_LATE(unsigned long)        segSizeLASTDATACONST;

SECURITY_READ_ONLY_LATE(vm_offset_t)          sectHIBTEXTB;
SECURITY_READ_ONLY_LATE(unsigned long)        sectSizeHIBTEXT;
SECURITY_READ_ONLY_LATE(vm_offset_t)          segHIBDATAB;
SECURITY_READ_ONLY_LATE(unsigned long)        segSizeHIBDATA;
SECURITY_READ_ONLY_LATE(vm_offset_t)          sectHIBDATACONSTB;
SECURITY_READ_ONLY_LATE(unsigned long)        sectSizeHIBDATACONST;

SECURITY_READ_ONLY_LATE(vm_offset_t)          segPRELINKTEXTB;
SECURITY_READ_ONLY_LATE(unsigned long)        segSizePRELINKTEXT;

SECURITY_READ_ONLY_LATE(static vm_offset_t)   segPLKTEXTEXECB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizePLKTEXTEXEC;

SECURITY_READ_ONLY_LATE(static vm_offset_t)   segPLKDATACONSTB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizePLKDATACONST;

SECURITY_READ_ONLY_LATE(static vm_offset_t)   segPRELINKDATAB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizePRELINKDATA;

SECURITY_READ_ONLY_LATE(static vm_offset_t)   segPLKLLVMCOVB = 0;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizePLKLLVMCOV = 0;

SECURITY_READ_ONLY_LATE(static vm_offset_t)   segPLKLINKEDITB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizePLKLINKEDIT;

SECURITY_READ_ONLY_LATE(static vm_offset_t)   segPRELINKINFOB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizePRELINKINFO;

                                                                    
SECURITY_READ_ONLY_LATE(vm_offset_t)          segKCTEXTEXECB;
SECURITY_READ_ONLY_LATE(unsigned long)        segSizeKCTEXTEXEC;
SECURITY_READ_ONLY_LATE(static vm_offset_t)   segKCDATACONSTB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizeKCDATACONST;
SECURITY_READ_ONLY_LATE(static vm_offset_t)   segKCDATAB;
SECURITY_READ_ONLY_LATE(static unsigned long) segSizeKCDATA;

SECURITY_READ_ONLY_LATE(static boolean_t) use_contiguous_hint = TRUE;

SECURITY_READ_ONLY_LATE(int) PAGE_SHIFT_CONST;

SECURITY_READ_ONLY_LATE(vm_offset_t) end_kern;
SECURITY_READ_ONLY_LATE(vm_offset_t) etext;
SECURITY_READ_ONLY_LATE(vm_offset_t) sdata;
SECURITY_READ_ONLY_LATE(vm_offset_t) edata;

SECURITY_READ_ONLY_LATE(static vm_offset_t) auxkc_mh, auxkc_base, auxkc_right_above;

vm_offset_t alloc_ptpage(boolean_t map_static);
SECURITY_READ_ONLY_LATE(vm_offset_t) ropage_next;

  
                                                          
                                                                      
                                 
  
              
                                               
                                        
                                         
                                        
   
SECURITY_READ_ONLY_LATE(vm_offset_t)     first_avail;
SECURITY_READ_ONLY_LATE(vm_offset_t)     static_memory_end;
SECURITY_READ_ONLY_LATE(pmap_paddr_t)    avail_start;
SECURITY_READ_ONLY_LATE(pmap_paddr_t)    avail_end;
SECURITY_READ_ONLY_LATE(pmap_paddr_t)    real_avail_end;
SECURITY_READ_ONLY_LATE(unsigned long)   real_phys_size;
SECURITY_READ_ONLY_LATE(vm_map_address_t) physmap_base = (vm_map_address_t)0;
SECURITY_READ_ONLY_LATE(vm_map_address_t) physmap_end = (vm_map_address_t)0;

#if __ARM_KERNEL_PROTECT__
extern void ExceptionVectorsBase;
extern void ExceptionVectorsEnd;
#endif                             

typedef struct {
	pmap_paddr_t pa;
	vm_map_address_t va;
	vm_size_t len;
} ptov_table_entry;

#define PTOV_TABLE_SIZE 8
SECURITY_READ_ONLY_LATE(static ptov_table_entry)        ptov_table[PTOV_TABLE_SIZE];
SECURITY_READ_ONLY_LATE(static boolean_t)               kva_active = FALSE;


vm_map_address_t
phystokv(pmap_paddr_t pa)
{
	for (size_t i = 0; (i < PTOV_TABLE_SIZE) && (ptov_table[i].len != 0); i++) {
		if ((pa >= ptov_table[i].pa) && (pa < (ptov_table[i].pa + ptov_table[i].len))) {
			return pa - ptov_table[i].pa + ptov_table[i].va;
		}
	}
	assertf((pa - gPhysBase) < real_phys_size, "%s: illegal PA: 0x%llx", __func__, (uint64_t)pa);
	return pa - gPhysBase + gVirtBase;
}

vm_map_address_t
phystokv_range(pmap_paddr_t pa, vm_size_t *max_len)
{
	vm_size_t len;
	for (size_t i = 0; (i < PTOV_TABLE_SIZE) && (ptov_table[i].len != 0); i++) {
		if ((pa >= ptov_table[i].pa) && (pa < (ptov_table[i].pa + ptov_table[i].len))) {
			len = ptov_table[i].len - (pa - ptov_table[i].pa);
			if (*max_len > len) {
				*max_len = len;
			}
			return pa - ptov_table[i].pa + ptov_table[i].va;
		}
	}
	len = PAGE_SIZE - (pa & PAGE_MASK);
	if (*max_len > len) {
		*max_len = len;
	}
	assertf((pa - gPhysBase) < real_phys_size, "%s: illegal PA: 0x%llx", __func__, (uint64_t)pa);
	return pa - gPhysBase + gVirtBase;
}

vm_offset_t
ml_static_vtop(vm_offset_t va)
{
	for (size_t i = 0; (i < PTOV_TABLE_SIZE) && (ptov_table[i].len != 0); i++) {
		if ((va >= ptov_table[i].va) && (va < (ptov_table[i].va + ptov_table[i].len))) {
			return va - ptov_table[i].va + ptov_table[i].pa;
		}
	}
	assertf(((vm_address_t)(va) - gVirtBase) < gPhysSize, "%s: illegal VA: %p", __func__, (void*)va);
	return (vm_address_t)(va) - gVirtBase + gPhysBase;
}

  
                                                                                
        
   
static vm_offset_t
round_up_pte_hint_address(vm_offset_t address)
{
	vm_offset_t hint_size = ARM_PTE_SIZE << ARM_PTE_HINT_ENTRIES_SHIFT;
	return (address + (hint_size - 1)) & ~(hint_size - 1);
}

                                                                            
  
                                                   
  
                                                                                            
                                                                                                   
  
                                                                                
   

vm_offset_t
alloc_ptpage(boolean_t map_static)
{
	vm_offset_t vaddr;

#if !(defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR))
	map_static = FALSE;
#endif

	if (!ropage_next) {
		ropage_next = (vm_offset_t)&ropagetable_begin;
	}

	if (map_static) {
		assert(ropage_next < (vm_offset_t)&ropagetable_end);

		vaddr = ropage_next;
		ropage_next += ARM_PGBYTES;

		return vaddr;
	} else {
		vaddr = phystokv(avail_start);
		avail_start += ARM_PGBYTES;

		return vaddr;
	}
}

#if DEBUG

void dump_kva_l2(vm_offset_t tt_base, tt_entry_t *tt, int indent, uint64_t *rosz_out, uint64_t *rwsz_out);

void
dump_kva_l2(vm_offset_t tt_base, tt_entry_t *tt, int indent, uint64_t *rosz_out, uint64_t *rwsz_out)
{
	unsigned int i;
	boolean_t cur_ro, prev_ro = 0;
	int start_entry = -1;
	tt_entry_t cur, prev = 0;
	pmap_paddr_t robegin = kvtophys((vm_offset_t)&ropagetable_begin);
	pmap_paddr_t roend = kvtophys((vm_offset_t)&ropagetable_end);
	boolean_t tt_static = kvtophys((vm_offset_t)tt) >= robegin &&
	    kvtophys((vm_offset_t)tt) < roend;

	for (i = 0; i < TTE_PGENTRIES; i++) {
		int tte_type = tt[i] & ARM_TTE_TYPE_MASK;
		cur = tt[i] & ARM_TTE_TABLE_MASK;

		if (tt_static) {
			                                                                      
                                                                 
			cur_ro = (tte_type == ARM_TTE_TYPE_BLOCK) || (cur >= robegin && cur < roend);
		} else {
			cur_ro = 0;
		}

		if ((cur == 0 && prev != 0) || (cur_ro != prev_ro && prev != 0)) {                
			uintptr_t start, end, sz;

			start = (uintptr_t)start_entry << ARM_TT_L2_SHIFT;
			start += tt_base;
			end = ((uintptr_t)i << ARM_TT_L2_SHIFT) - 1;
			end += tt_base;

			sz = end - start + 1;
			printf("%*s0x%08x_%08x-0x%08x_%08x %s (%luMB)\n",
			    indent * 4, "",
			    (uint32_t)(start >> 32), (uint32_t)start,
			    (uint32_t)(end >> 32), (uint32_t)end,
			    prev_ro ? "Static " : "Dynamic",
			    (sz >> 20));

			if (prev_ro) {
				*rosz_out += sz;
			} else {
				*rwsz_out += sz;
			}
		}

		if ((prev == 0 && cur != 0) || cur_ro != prev_ro) {                          
			start_entry = i;
		}

		prev = cur;
		prev_ro = cur_ro;
	}
}

void
dump_kva_space()
{
	uint64_t tot_rosz = 0, tot_rwsz = 0;
	int ro_ptpages, rw_ptpages;
	pmap_paddr_t robegin = kvtophys((vm_offset_t)&ropagetable_begin);
	pmap_paddr_t roend = kvtophys((vm_offset_t)&ropagetable_end);
	boolean_t root_static = kvtophys((vm_offset_t)cpu_tte) >= robegin &&
	    kvtophys((vm_offset_t)cpu_tte) < roend;
	uint64_t kva_base = ~((1ULL << (64 - T1SZ_BOOT)) - 1);

	printf("Root page table: %s\n", root_static ? "Static" : "Dynamic");

	for (unsigned int i = 0; i < TTE_PGENTRIES; i++) {
		pmap_paddr_t cur;
		boolean_t cur_ro;
		uintptr_t start, end;
		uint64_t rosz = 0, rwsz = 0;

		if ((cpu_tte[i] & ARM_TTE_VALID) == 0) {
			continue;
		}

		cur = cpu_tte[i] & ARM_TTE_TABLE_MASK;
		start = (uint64_t)i << ARM_TT_L1_SHIFT;
		start = start + kva_base;
		end = start + (ARM_TT_L1_SIZE - 1);
		cur_ro = cur >= robegin && cur < roend;

		printf("0x%08x_%08x-0x%08x_%08x %s\n",
		    (uint32_t)(start >> 32), (uint32_t)start,
		    (uint32_t)(end >> 32), (uint32_t)end,
		    cur_ro ? "Static " : "Dynamic");

		dump_kva_l2(start, (tt_entry_t*)phystokv(cur), 1, &rosz, &rwsz);
		tot_rosz += rosz;
		tot_rwsz += rwsz;
	}

	printf("L2 Address space mapped: Static %lluMB Dynamic %lluMB Total %lluMB\n",
	    tot_rosz >> 20,
	    tot_rwsz >> 20,
	    (tot_rosz >> 20) + (tot_rwsz >> 20));

	ro_ptpages = (int)((ropage_next - (vm_offset_t)&ropagetable_begin) >> ARM_PGSHIFT);
	rw_ptpages = (int)(lowGlo.lgStaticSize  >> ARM_PGSHIFT);
	printf("Pages used: static %d dynamic %d\n", ro_ptpages, rw_ptpages);
}

#endif            

#if __ARM_KERNEL_PROTECT__ || XNU_MONITOR
  
              
                                                                                
                                      
                                                         
  
                                                                                
                                                     
   
static void
arm_vm_map(tt_entry_t * root_ttp, vm_offset_t vaddr, pt_entry_t pte)
{
	vm_offset_t ptpage = 0;
	tt_entry_t * ttp = root_ttp;

	tt_entry_t * l1_ttep = NULL;
	tt_entry_t l1_tte = 0;

	tt_entry_t * l2_ttep = NULL;
	tt_entry_t l2_tte = 0;
	pt_entry_t * ptep = NULL;
	pt_entry_t cpte = 0;

	  
                                                                    
                                                              
    
	l1_ttep = ttp + ((vaddr & ARM_TT_L1_INDEX_MASK) >> ARM_TT_L1_SHIFT);
	l1_tte = *l1_ttep;

	if (l1_tte == ARM_TTE_EMPTY) {
		ptpage = alloc_ptpage(TRUE);
		bzero((void *)ptpage, ARM_PGBYTES);
		l1_tte = kvtophys(ptpage);
		l1_tte &= ARM_TTE_TABLE_MASK;
		l1_tte |= ARM_TTE_VALID | ARM_TTE_TYPE_TABLE;
		*l1_ttep = l1_tte;
		ptpage = 0;
	}

	ttp = (tt_entry_t *)phystokv(l1_tte & ARM_TTE_TABLE_MASK);

	l2_ttep = ttp + ((vaddr & ARM_TT_L2_INDEX_MASK) >> ARM_TT_L2_SHIFT);
	l2_tte = *l2_ttep;

	if (l2_tte == ARM_TTE_EMPTY) {
		ptpage = alloc_ptpage(TRUE);
		bzero((void *)ptpage, ARM_PGBYTES);
		l2_tte = kvtophys(ptpage);
		l2_tte &= ARM_TTE_TABLE_MASK;
		l2_tte |= ARM_TTE_VALID | ARM_TTE_TYPE_TABLE;
		*l2_ttep = l2_tte;
		ptpage = 0;
	}

	ttp = (tt_entry_t *)phystokv(l2_tte & ARM_TTE_TABLE_MASK);

	ptep = ttp + ((vaddr & ARM_TT_L3_INDEX_MASK) >> ARM_TT_L3_SHIFT);
	cpte = *ptep;

	  
                                                                   
            
    
	if (cpte != ARM_PTE_EMPTY) {
		panic("%s: cpte=%#llx is not empty, "
		    "vaddr=%#lx, pte=%#llx",
		    __FUNCTION__, cpte,
		    vaddr, pte);
	}

	*ptep = pte;
}

#endif                                       

#if __ARM_KERNEL_PROTECT__

  
                         
                                      
                                                         
  
                                                                   
   
static void
arm_vm_kernel_el0_map(vm_offset_t vaddr, pt_entry_t pte)
{
	                                                                  
	vm_offset_t kernel_pmap_vaddr = vaddr - ((ARM_TT_ROOT_INDEX_MASK + ARM_TT_ROOT_SIZE) / 2ULL);
	arm_vm_map(cpu_tte, kernel_pmap_vaddr, pte);
}

  
                         
                                      
                                                         
  
                                                                   
   
static void
arm_vm_kernel_el1_map(vm_offset_t vaddr, pt_entry_t pte)
{
	arm_vm_map(cpu_tte, vaddr, pte);
}

  
                     
                                      
  
                                                                               
                                                                       
                                                                             
                                                                               
   
static pt_entry_t
arm_vm_kernel_pte(vm_offset_t vaddr)
{
	tt_entry_t * ttp = cpu_tte;
	tt_entry_t * ttep = NULL;
	tt_entry_t tte = 0;
	pt_entry_t * ptep = NULL;
	pt_entry_t pte = 0;

	ttep = ttp + ((vaddr & ARM_TT_L1_INDEX_MASK) >> ARM_TT_L1_SHIFT);
	tte = *ttep;

	assert(tte & ARM_TTE_VALID);

	if ((tte & ARM_TTE_TYPE_MASK) == ARM_TTE_TYPE_BLOCK) {
		                                                               
		pte = (pt_entry_t)(tte & ~ARM_TTE_TYPE_MASK);
		pte |= ARM_PTE_TYPE_VALID;
		pte |= vaddr & ((ARM_TT_L1_SIZE - 1) & ARM_PTE_PAGE_MASK);
		pte &= ~ARM_PTE_HINT_MASK;
		return pte;
	}

	ttp = (tt_entry_t *)phystokv(tte & ARM_TTE_TABLE_MASK);
	ttep = ttp + ((vaddr & ARM_TT_L2_INDEX_MASK) >> ARM_TT_L2_SHIFT);
	tte = *ttep;

	assert(tte & ARM_TTE_VALID);

	if ((tte & ARM_TTE_TYPE_MASK) == ARM_TTE_TYPE_BLOCK) {
		                                                               
		pte = (pt_entry_t)(tte & ~ARM_TTE_TYPE_MASK);
		pte |= ARM_PTE_TYPE_VALID;
		pte |= vaddr & ((ARM_TT_L2_SIZE - 1) & ARM_PTE_PAGE_MASK);
		pte &= ~ARM_PTE_HINT_MASK;
		return pte;
	}

	ttp = (tt_entry_t *)phystokv(tte & ARM_TTE_TABLE_MASK);

	ptep = ttp + ((vaddr & ARM_TT_L3_INDEX_MASK) >> ARM_TT_L3_SHIFT);
	pte = *ptep;
	pte &= ~ARM_PTE_HINT_MASK;
	return pte;
}

  
                                      
                                                                             
                
  
                                                                         
                                                                         
                                     
   
static void
arm_vm_prepare_kernel_el0_mappings(bool alloc_only)
{
	pt_entry_t pte = 0;
	vm_offset_t start = ((vm_offset_t)&ExceptionVectorsBase) & ~PAGE_MASK;
	vm_offset_t end = (((vm_offset_t)&ExceptionVectorsEnd) + PAGE_MASK) & ~PAGE_MASK;
	vm_offset_t cur = 0;
	vm_offset_t cur_fixed = 0;

	                                                                       
	for (cur = start, cur_fixed = ARM_KERNEL_PROTECT_EXCEPTION_START; cur < end; cur += ARM_PGBYTES, cur_fixed += ARM_PGBYTES) {
		  
                                                                  
                                                                  
                                                              
                      
     
		if (!alloc_only) {
			pte = arm_vm_kernel_pte(cur);
		}

		arm_vm_kernel_el1_map(cur_fixed, pte);
		arm_vm_kernel_el0_map(cur_fixed, pte);
	}

	__builtin_arm_dmb(DMB_ISH);
	__builtin_arm_isb(ISB_SY);

	if (!alloc_only) {
		  
                                                                
                                              
     
		set_vbar_el1(ARM_KERNEL_PROTECT_EXCEPTION_START);
		__builtin_arm_isb(ISB_SY);
	}
}

  
                                       
  
                                                                       
   
static void
arm_vm_populate_kernel_el0_mappings(void)
{
	arm_vm_prepare_kernel_el0_mappings(FALSE);
}

  
                                     
  
                                                                            
            
   
static void
arm_vm_expand_kernel_el0_mappings(void)
{
	arm_vm_prepare_kernel_el0_mappings(TRUE);
}
#endif                             

#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
extern void bootstrap_instructions;

  
                                                                          
                                                                           
                                                                       
                                                                          
                 
   
static void
arm_replace_identity_map(void)
{
	vm_offset_t addr;
	pmap_paddr_t paddr;

	pmap_paddr_t l1_ptp_phys = 0;
	tt_entry_t *l1_ptp_virt = NULL;
	tt_entry_t *tte1 = NULL;
	pmap_paddr_t l2_ptp_phys = 0;
	tt_entry_t *l2_ptp_virt = NULL;
	tt_entry_t *tte2 = NULL;
	pmap_paddr_t l3_ptp_phys = 0;
	pt_entry_t *l3_ptp_virt = NULL;
	pt_entry_t *ptep = NULL;

	addr = ((vm_offset_t)&bootstrap_instructions) & ~ARM_PGMASK;
	paddr = kvtophys(addr);

	  
                                                                    
    
	l1_ptp_phys = kvtophys((vm_offset_t)&bootstrap_pagetables);
	l1_ptp_virt = (tt_entry_t *)phystokv(l1_ptp_phys);
	tte1 = &l1_ptp_virt[L1_TABLE_INDEX(paddr)];

	l2_ptp_virt = L2_TABLE_VA(tte1);
	l2_ptp_phys = (*tte1) & ARM_TTE_TABLE_MASK;
	tte2 = &l2_ptp_virt[L2_TABLE_INDEX(paddr)];

	l3_ptp_virt = (pt_entry_t *)alloc_ptpage(TRUE);
	l3_ptp_phys = kvtophys((vm_offset_t)l3_ptp_virt);
	ptep = &l3_ptp_virt[L3_TABLE_INDEX(paddr)];

	  
                                                                       
                                       
    

	bzero(l1_ptp_virt, ARM_PGBYTES);
	*tte1 = ARM_TTE_BOOT_TABLE | (l2_ptp_phys & ARM_TTE_TABLE_MASK);

	bzero(l2_ptp_virt, ARM_PGBYTES);
	*tte2 = ARM_TTE_BOOT_TABLE | (l3_ptp_phys & ARM_TTE_TABLE_MASK);

	*ptep = (paddr & ARM_PTE_MASK) |
	    ARM_PTE_TYPE_VALID |
	    ARM_PTE_SH(SH_OUTER_MEMORY) |
	    ARM_PTE_ATTRINDX(CACHE_ATTRINDX_WRITEBACK) |
	    ARM_PTE_AF |
	    ARM_PTE_AP(AP_RONA) |
	    ARM_PTE_NX;
}
#endif                                                                       

tt_entry_t *arm_kva_to_tte(vm_offset_t);

tt_entry_t *
arm_kva_to_tte(vm_offset_t va)
{
	tt_entry_t *tte1, *tte2;
	tte1 = cpu_tte + L1_TABLE_INDEX(va);
	tte2 = L2_TABLE_VA(tte1) + L2_TABLE_INDEX(va);

	return tte2;
}

#if XNU_MONITOR

static inline pt_entry_t *
arm_kva_to_pte(vm_offset_t va)
{
	tt_entry_t *tte2 = arm_kva_to_tte(va);
	return L3_TABLE_VA(tte2) + L3_TABLE_INDEX(va);
}

#endif

#define ARM64_GRANULE_ALLOW_BLOCK (1 << 0)
#define ARM64_GRANULE_ALLOW_HINT (1 << 1)

  
                                                                                
                                                                               
                                                                                      
                                                                                       
                                                                                     
                                                          
  
                                                                    
                                        
   

static void
arm_vm_page_granular_helper(vm_offset_t start, vm_offset_t _end, vm_offset_t va, pmap_paddr_t pa_offset,
    int pte_prot_APX, int pte_prot_XN, unsigned granule,
    pt_entry_t **deferred_pte, pt_entry_t *deferred_ptmp)
{
	if (va & ARM_TT_L2_OFFMASK) {                                                          
		tt_entry_t *tte2;
		tt_entry_t tmplate;
		pmap_paddr_t pa;
		pt_entry_t *ppte, *recursive_pte = NULL, ptmp, recursive_ptmp = 0;
		addr64_t ppte_phys;
		unsigned i;

		va &= ~ARM_TT_L2_OFFMASK;
		pa = va - gVirtBase + gPhysBase - pa_offset;

		if (pa >= real_avail_end) {
			return;
		}

		tte2 = arm_kva_to_tte(va);

		assert(_end >= va);
		tmplate = *tte2;

		if (ARM_TTE_TYPE_TABLE == (tmplate & ARM_TTE_TYPE_MASK)) {
			                                      
			ppte = (pt_entry_t *)phystokv((tmplate & ARM_TTE_TABLE_MASK));
		} else {
			                                                     

			                                                   
			                      
			assert(!kva_active);

			ppte = (pt_entry_t*)alloc_ptpage(pa_offset == 0);
			bzero(ppte, ARM_PGBYTES);
			ppte_phys = kvtophys((vm_offset_t)ppte);

			*tte2 = pa_to_tte(ppte_phys) | ARM_TTE_TYPE_TABLE | ARM_TTE_VALID;
		}

		vm_offset_t len = _end - va;
		if ((pa + len) > real_avail_end) {
			_end -= (pa + len - real_avail_end);
		}
		assert((start - gVirtBase + gPhysBase - pa_offset) >= gPhysBase);

		                                                                     
                                                                           
                                            
		vm_offset_t rounded_end = round_page(_end);
		                                                               
		for (i = 0; i <= (ARM_TT_L3_INDEX_MASK >> ARM_TT_L3_SHIFT); i++) {
			if ((start <= va) && (va < rounded_end)) {
				ptmp = pa | ARM_PTE_AF | ARM_PTE_SH(SH_OUTER_MEMORY) | ARM_PTE_TYPE;
				ptmp = ptmp | ARM_PTE_ATTRINDX(CACHE_ATTRINDX_DEFAULT);
				ptmp = ptmp | ARM_PTE_AP(pte_prot_APX);
				ptmp = ptmp | ARM_PTE_NX;
#if __ARM_KERNEL_PROTECT__
				ptmp = ptmp | ARM_PTE_NG;
#endif                             

				if (pte_prot_XN) {
					ptmp = ptmp | ARM_PTE_PNX;
				}

				  
                                                                       
                                                                                  
                                                
       
				if ((va >= round_up_pte_hint_address(start)) && (round_up_pte_hint_address(va + 1) <= _end) &&
				    (granule & ARM64_GRANULE_ALLOW_HINT) && use_contiguous_hint) {
					assert((va & ((1 << ARM_PTE_HINT_ADDR_SHIFT) - 1)) == ((pa & ((1 << ARM_PTE_HINT_ADDR_SHIFT) - 1))));
					ptmp |= ARM_PTE_HINT;
					                                                                       
                                                                                                  
                                                         
					assert(!kva_active || (ppte[i] == ARM_PTE_TYPE_FAULT));
				}
				  
                                                                                        
                                                                                             
                                                                                              
                                                                                          
                                          
       
				assert(!kva_active || (ppte[i] == ARM_PTE_TYPE_FAULT) || ((ppte[i] & ARM_PTE_HINT) == (ptmp & ARM_PTE_HINT)));

				  
                                                                                                 
                                                                                              
                                                                                                  
                                                                                                 
                                                                                                
                                                                                                
                                                                                        
       
				if (kva_active && ((pt_entry_t*)(phystokv(pa)) == ppte)) {
					assert(recursive_pte == NULL);
					assert(granule & ARM64_GRANULE_ALLOW_BLOCK);
					recursive_pte = &ppte[i];
					recursive_ptmp = ptmp;
				} else if ((deferred_pte != NULL) && (&ppte[i] == &recursive_pte[1])) {
					assert(*deferred_pte == NULL);
					assert(deferred_ptmp != NULL);
					*deferred_pte = &ppte[i];
					*deferred_ptmp = ptmp;
				} else {
					ppte[i] = ptmp;
				}
			}

			va += ARM_PGBYTES;
			pa += ARM_PGBYTES;
		}
		if (recursive_pte != NULL) {
			*recursive_pte = recursive_ptmp;
		}
	}
}

  
                                                                                     
                                                                                
                                                                                    
                                                                                  
   
static void
arm_vm_page_granular_prot(vm_offset_t start, unsigned long size, pmap_paddr_t pa_offset,
    int tte_prot_XN, int pte_prot_APX, int pte_prot_XN,
    unsigned granule)
{
	pt_entry_t *deferred_pte = NULL, deferred_ptmp = 0;
	vm_offset_t _end = start + size;
	vm_offset_t align_start = (start + ARM_TT_L2_OFFMASK) & ~ARM_TT_L2_OFFMASK;

	if (size == 0x0UL) {
		return;
	}

	if (align_start > _end) {
		arm_vm_page_granular_helper(start, _end, start, pa_offset, pte_prot_APX, pte_prot_XN, granule, NULL, NULL);
		return;
	}

	arm_vm_page_granular_helper(start, align_start, start, pa_offset, pte_prot_APX, pte_prot_XN, granule, &deferred_pte, &deferred_ptmp);

	while ((_end - align_start) >= ARM_TT_L2_SIZE) {
		if (!(granule & ARM64_GRANULE_ALLOW_BLOCK)) {
			arm_vm_page_granular_helper(align_start, align_start + ARM_TT_L2_SIZE, align_start + 1, pa_offset,
			    pte_prot_APX, pte_prot_XN, granule, NULL, NULL);
		} else {
			pmap_paddr_t pa = align_start - gVirtBase + gPhysBase - pa_offset;
			assert((pa & ARM_TT_L2_OFFMASK) == 0);
			tt_entry_t *tte2;
			tt_entry_t tmplate;

			tte2 = arm_kva_to_tte(align_start);

			if ((pa >= gPhysBase) && (pa < real_avail_end)) {
				tmplate = (pa & ARM_TTE_BLOCK_L2_MASK) | ARM_TTE_TYPE_BLOCK
				    | ARM_TTE_VALID | ARM_TTE_BLOCK_AF | ARM_TTE_BLOCK_NX
				    | ARM_TTE_BLOCK_AP(pte_prot_APX) | ARM_TTE_BLOCK_SH(SH_OUTER_MEMORY)
				    | ARM_TTE_BLOCK_ATTRINDX(CACHE_ATTRINDX_WRITEBACK);

#if __ARM_KERNEL_PROTECT__
				tmplate = tmplate | ARM_TTE_BLOCK_NG;
#endif                             
				if (tte_prot_XN) {
					tmplate = tmplate | ARM_TTE_BLOCK_PNX;
				}

				*tte2 = tmplate;
			}
		}
		align_start += ARM_TT_L2_SIZE;
	}

	if (align_start < _end) {
		arm_vm_page_granular_helper(align_start, _end, _end, pa_offset, pte_prot_APX, pte_prot_XN, granule, &deferred_pte, &deferred_ptmp);
	}

	if (deferred_pte != NULL) {
		*deferred_pte = deferred_ptmp;
	}
}

static inline void
arm_vm_page_granular_RNX(vm_offset_t start, unsigned long size, unsigned granule)
{
	arm_vm_page_granular_prot(start, size, 0, 1, AP_RONA, 1, granule);
}

static inline void
arm_vm_page_granular_ROX(vm_offset_t start, unsigned long size, unsigned granule)
{
	arm_vm_page_granular_prot(start, size, 0, 0, AP_RONA, 0, granule);
}

static inline void
arm_vm_page_granular_RWNX(vm_offset_t start, unsigned long size, unsigned granule)
{
	arm_vm_page_granular_prot(start, size, 0, 1, AP_RWNA, 1, granule);
}

                                                             
typedef struct MemoryMapFileInfo {
	vm_offset_t paddr;
	size_t length;
} MemoryMapFileInfo;

                                                   
static bool
arm_vm_auxkc_init(void)
{
	if (auxkc_mh == 0 || auxkc_base == 0) {
		return false;             
	}

	                                                           
	arm_auxkc_init((void*)auxkc_mh, (void*)auxkc_base);

	if (segLOWESTAuxKC != segLOWEST) {
		panic("segLOWESTAuxKC (%p) not equal to segLOWEST (%p). auxkc_mh: %p, auxkc_base: %p",
		    (void*)segLOWESTAuxKC, (void*)segLOWEST,
		    (void*)auxkc_mh, (void*)auxkc_base);
	}

	  
                                                                                   
                                                                              
                                                                              
                                      
    
	assert(segHIGHESTROAuxKC == segHIGHESTNLEAuxKC);
	assert(segHIGHESTAuxKC >= segHIGHESTROAuxKC);
	if (segHIGHESTAuxKC > segHIGHESTROAuxKC) {
		segHIGHESTROAuxKC = segHIGHESTAuxKC;
	}

	  
                                                                                                
                                                                  
    
	assert(segHIGHESTROAuxKC == auxkc_right_above);
	assert(segHIGHESTRXAuxKC <= segHIGHESTROAuxKC);
	assert(segLOWESTRXAuxKC <= segHIGHESTRXAuxKC);
	assert(segLOWESTROAuxKC <= segLOWESTRXAuxKC);
	assert(segLOWESTAuxKC <= segLOWESTROAuxKC);

	if (segHIGHESTRXAuxKC < segLOWEST) {
		arm_vm_page_granular_RNX(segHIGHESTRXAuxKC, segLOWEST - segHIGHESTRXAuxKC, 0);
	}
	if (segLOWESTRXAuxKC < segHIGHESTRXAuxKC) {
		arm_vm_page_granular_ROX(segLOWESTRXAuxKC, segHIGHESTRXAuxKC - segLOWESTRXAuxKC, 0);                                              
	}
	if (segLOWESTROAuxKC < segLOWESTRXAuxKC) {
		arm_vm_page_granular_RNX(segLOWESTROAuxKC, segLOWESTRXAuxKC - segLOWESTROAuxKC, 0);
	}
	if (segLOWESTAuxKC < segLOWESTROAuxKC) {
		arm_vm_page_granular_RWNX(segLOWESTAuxKC, segLOWESTROAuxKC - segLOWESTAuxKC, 0);
	}

	return true;
}

void
arm_vm_prot_init(__unused boot_args * args)
{
	segLOWESTTEXT = UINT64_MAX;
	if (segSizePRELINKTEXT && (segPRELINKTEXTB < segLOWESTTEXT)) {
		segLOWESTTEXT = segPRELINKTEXTB;
	}
	assert(segSizeTEXT);
	if (segTEXTB < segLOWESTTEXT) {
		segLOWESTTEXT = segTEXTB;
	}
	assert(segLOWESTTEXT < UINT64_MAX);

	segEXTRADATA = segLOWESTTEXT;
	segSizeEXTRADATA = 0;

	segLOWEST = segLOWESTTEXT;
	segLOWESTRO = segLOWESTTEXT;

	if (segLOWESTKC && segLOWESTKC < segLOWEST) {
		  
                                                                                                
                                                                                    
     
		arm_vm_page_granular_RNX(segLOWESTKC, segLOWEST - segLOWESTKC, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
		segLOWEST = segLOWESTKC;
		if (segLOWESTROKC && segLOWESTROKC < segLOWESTRO) {
			segLOWESTRO = segLOWESTROKC;
		}
		if (segHIGHESTROKC && segHIGHESTROKC > segHIGHESTRO) {
			segHIGHESTRO = segHIGHESTROKC;
		}
	}

	DTEntry memory_map;
	MemoryMapFileInfo const *trustCacheRange;
	unsigned int trustCacheRangeSize;
	int err;

	if (SecureDTIsLockedDown()) {
		segEXTRADATA = (vm_offset_t)PE_state.deviceTreeHead;
		segSizeEXTRADATA = PE_state.deviceTreeSize;
	}

	err = SecureDTLookupEntry(NULL, "chosen/memory-map", &memory_map);
	assert(err == kSuccess);

	err = SecureDTGetProperty(memory_map, "TrustCache", (void const **)&trustCacheRange, &trustCacheRangeSize);
	if (err == kSuccess) {
		assert(trustCacheRangeSize == sizeof(MemoryMapFileInfo));

		if (segSizeEXTRADATA == 0) {
			segEXTRADATA = phystokv(trustCacheRange->paddr);
			segSizeEXTRADATA = trustCacheRange->length;
		} else {
			segSizeEXTRADATA += trustCacheRange->length;
		}
	}

	if (segSizeEXTRADATA != 0) {
		if (segEXTRADATA <= segLOWEST) {
			segLOWEST = segEXTRADATA;
			if (segEXTRADATA <= segLOWESTRO) {
				segLOWESTRO = segEXTRADATA;
			}
		}
#if !(DEBUG || DEVELOPMENT)


		else {
			panic("EXTRADATA is in an unexpected place: %#lx > %#lx", segEXTRADATA, segLOWEST);
		}
#endif                              

		arm_vm_page_granular_RNX(segEXTRADATA, segSizeEXTRADATA, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
	}

	const MemoryMapFileInfo *auxKC_range, *auxKC_header_range;
	unsigned int auxKC_range_size, auxKC_header_range_size;

	err = SecureDTGetProperty(memory_map, "AuxKC", (const void**)&auxKC_range,
	    &auxKC_range_size);
	if (err != kSuccess) {
		goto noAuxKC;
	}
	assert(auxKC_range_size == sizeof(MemoryMapFileInfo));
	err = SecureDTGetProperty(memory_map, "AuxKC-mach_header",
	    (const void**)&auxKC_header_range, &auxKC_header_range_size);
	if (err != kSuccess) {
		goto noAuxKC;
	}
	assert(auxKC_header_range_size == sizeof(MemoryMapFileInfo));

	auxkc_mh = phystokv(auxKC_header_range->paddr);
	auxkc_base = phystokv(auxKC_range->paddr);
	if (!auxkc_mh || !auxkc_base) {
		goto noAuxKC;
	}

	if (auxkc_base < segLOWEST) {
		auxkc_right_above = segLOWEST;
		segLOWEST = auxkc_base;
	} else {
		panic("auxkc_base (%p) not below segLOWEST (%p)", (void*)auxkc_base, (void*)segLOWEST);
	}

	                                                                  
                                                                  
            
    
	arm_vm_page_granular_RWNX(auxkc_base, auxKC_range->length, 0);

noAuxKC:
	                                                  
	arm_vm_page_granular_RWNX(segPRELINKTEXTB, segSizePRELINKTEXT, ARM64_GRANULE_ALLOW_BLOCK);                                              

	                                                                 
	arm_vm_page_granular_RWNX(segPLKDATACONSTB, segSizePLKDATACONST, ARM64_GRANULE_ALLOW_BLOCK);                                              

	                                                              
	arm_vm_page_granular_ROX(segPLKTEXTEXECB, segSizePLKTEXTEXEC, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);                                              

	                                                                                   
                                                                                                            
    
	if (!segSizePLKDATACONST && !segSizePLKTEXTEXEC) {
		if (segSizePRELINKTEXT) {
			arm_vm_page_granular_RWNX(segPRELINKTEXTB + segSizePRELINKTEXT, segTEXTB - (segPRELINKTEXTB + segSizePRELINKTEXT),
			    ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
		}
	} else {
		  
                                                                              
                                                                           
            
     
		if ((segPLKDATACONSTB + segSizePLKDATACONST) < segTEXTB) {
			arm_vm_page_granular_RWNX(segPLKDATACONSTB + segSizePLKDATACONST, segTEXTB - (segPLKDATACONSTB + segSizePLKDATACONST),
			    ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
		}
	}

	  
                                                                                 
                                                                                   
                                                   
   
                                                                                                  
    
	arm_vm_page_granular_RNX(segTEXTB, segSizeTEXT, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);

	                                        
                                                                                                   
                             
    
	arm_vm_page_granular_RWNX(segDATACONSTB, segSizeDATACONST, ARM64_GRANULE_ALLOW_BLOCK);

	arm_vm_page_granular_ROX(segTEXTEXECB, segSizeTEXTEXEC, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);

#if XNU_MONITOR
	arm_vm_page_granular_ROX(segPPLTEXTB, segSizePPLTEXT, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
	arm_vm_page_granular_ROX(segPPLTRAMPB, segSizePPLTRAMP, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
	arm_vm_page_granular_RNX(segPPLDATACONSTB, segSizePPLDATACONST, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
#endif

	                                   
	arm_vm_page_granular_RWNX(segDATAB, segSizeDATA, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
#if XNU_MONITOR
	arm_vm_page_granular_RWNX(segPPLDATAB, segSizePPLDATA, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
#endif

	arm_vm_page_granular_RWNX(segHIBDATAB, segSizeHIBDATA, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);

	arm_vm_page_granular_RWNX(segBOOTDATAB, segSizeBOOTDATA, 0);
	arm_vm_page_granular_RNX((vm_offset_t)&intstack_low_guard, PAGE_MAX_SIZE, 0);
	arm_vm_page_granular_RNX((vm_offset_t)&intstack_high_guard, PAGE_MAX_SIZE, 0);
	arm_vm_page_granular_RNX((vm_offset_t)&excepstack_high_guard, PAGE_MAX_SIZE, 0);

	arm_vm_page_granular_ROX(segKLDB, segSizeKLD, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
	arm_vm_page_granular_RNX(segKLDDATAB, segSizeKLDDATA, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
	arm_vm_page_granular_RWNX(segLINKB, segSizeLINK, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);
	arm_vm_page_granular_RWNX(segPLKLINKEDITB, segSizePLKLINKEDIT, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);                                   
	arm_vm_page_granular_ROX(segLASTB, segSizeLAST, ARM64_GRANULE_ALLOW_BLOCK);                                                  
	if (segLASTDATACONSTB) {
		arm_vm_page_granular_RWNX(segLASTDATACONSTB, segSizeLASTDATACONST, ARM64_GRANULE_ALLOW_BLOCK);                                                            
	}
	arm_vm_page_granular_RWNX(segPRELINKDATAB, segSizePRELINKDATA, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);                                      

	if (segSizePLKLLVMCOV > 0) {
		arm_vm_page_granular_RWNX(segPLKLLVMCOVB, segSizePLKLLVMCOV, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);                           
	}
	arm_vm_page_granular_RWNX(segPRELINKINFOB, segSizePRELINKINFO, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);                            

	                                           
	vm_kernelcache_base = segLOWEST;
	vm_kernelcache_top = end_kern;
}

  
                       
                        
                       
   
typedef int (*cmpfunc_t)(const void *a, const void *b);

extern void
qsort(void *a, size_t n, size_t es, cmpfunc_t cmp);

static int
cmp_ptov_entries(const void *a, const void *b)
{
	const ptov_table_entry *entry_a = a;
	const ptov_table_entry *entry_b = b;
	                                             
	if (entry_a->len < entry_b->len) {
		return 1;
	} else if (entry_a->len > entry_b->len) {
		return -1;
	} else {
		return 0;
	}
}

SECURITY_READ_ONLY_LATE(static unsigned int) ptov_index = 0;

#define ROUND_L1(addr) (((addr) + ARM_TT_L1_OFFMASK) & ~(ARM_TT_L1_OFFMASK))
#define ROUND_TWIG(addr) (((addr) + ARM_TT_TWIG_OFFMASK) & ~(ARM_TT_TWIG_OFFMASK))

static void
arm_vm_physmap_slide(ptov_table_entry *temp_ptov_table, vm_map_address_t orig_va, vm_size_t len, int pte_prot_APX, unsigned granule)
{
	pmap_paddr_t pa_offset;

	assert(ptov_index < PTOV_TABLE_SIZE);
	assert((orig_va & ARM_PGMASK) == 0);
	temp_ptov_table[ptov_index].pa = orig_va - gVirtBase + gPhysBase;
	if (ptov_index == 0) {
		temp_ptov_table[ptov_index].va = physmap_base;
	} else {
		temp_ptov_table[ptov_index].va = temp_ptov_table[ptov_index - 1].va + temp_ptov_table[ptov_index - 1].len;
	}
	if (granule & ARM64_GRANULE_ALLOW_BLOCK) {
		vm_map_address_t orig_offset = temp_ptov_table[ptov_index].pa & ARM_TT_TWIG_OFFMASK;
		vm_map_address_t new_offset = temp_ptov_table[ptov_index].va & ARM_TT_TWIG_OFFMASK;
		if (new_offset < orig_offset) {
			temp_ptov_table[ptov_index].va += (orig_offset - new_offset);
		} else if (new_offset > orig_offset) {
			temp_ptov_table[ptov_index].va = ROUND_TWIG(temp_ptov_table[ptov_index].va) + orig_offset;
		}
	}
	assert((temp_ptov_table[ptov_index].va & ARM_PGMASK) == 0);
	temp_ptov_table[ptov_index].len = round_page(len);
	pa_offset = temp_ptov_table[ptov_index].va - orig_va;
	arm_vm_page_granular_prot(temp_ptov_table[ptov_index].va, temp_ptov_table[ptov_index].len, pa_offset, 1, pte_prot_APX, 1, granule);
	++ptov_index;
}

#if XNU_MONITOR

SECURITY_READ_ONLY_LATE(static boolean_t) keep_linkedit = FALSE;

static void
arm_vm_physmap_init(boot_args *args)
{
	ptov_table_entry temp_ptov_table[PTOV_TABLE_SIZE];
	bzero(temp_ptov_table, sizeof(temp_ptov_table));

	                                                                                        
	                                                                                             
	                                                                                                 
	                                     

	                                                                
	arm_vm_physmap_slide(temp_ptov_table, gVirtBase, segLOWEST - gVirtBase, AP_RWNA, 0);

	                          
#if !defined(KERNEL_INTEGRITY_KTRR) && !defined(KERNEL_INTEGRITY_CTRR)
	                                          
	arm_vm_physmap_slide(temp_ptov_table, segKLDB, segSizeKLD, AP_RONA, 0);
#endif
	arm_vm_physmap_slide(temp_ptov_table, segKLDDATAB, segSizeKLDDATA, AP_RONA, 0);

	                  
	arm_vm_physmap_slide(temp_ptov_table, segBOOTDATAB, segSizeBOOTDATA, AP_RONA, 0);

#if KASAN_DYNAMIC_BLACKLIST
	                                                                                          
                                                                                                       
	keep_linkedit = TRUE;
#else
	PE_parse_boot_argn("keepsyms", &keep_linkedit, sizeof(keep_linkedit));
	if (kernel_mach_header_is_in_fileset(&_mh_execute_header)) {
		keep_linkedit = TRUE;
	}
#endif
	if (!keep_linkedit) {
		                  
		arm_vm_physmap_slide(temp_ptov_table, segLINKB, segSizeLINK, AP_RWNA, 0);

		                            
		arm_vm_physmap_slide(temp_ptov_table, segPLKLINKEDITB, segSizePLKLINKEDIT, AP_RWNA, 0);
	}

	                          
	arm_vm_physmap_slide(temp_ptov_table, segPRELINKINFOB, segSizePRELINKINFO, AP_RWNA, 0);

	                                                       
	arm_vm_physmap_slide(temp_ptov_table, end_kern, (args->topOfKernelData - gPhysBase + gVirtBase) - end_kern, AP_RWNA, 0);
	if (!SecureDTIsLockedDown()) {
		PE_slide_devicetree(temp_ptov_table[ptov_index - 1].va - end_kern);
	}

	                               
	arm_vm_physmap_slide(temp_ptov_table, (args->topOfKernelData - gPhysBase + gVirtBase),
	    real_avail_end - args->topOfKernelData, AP_RWNA, 0);

	assert((temp_ptov_table[ptov_index - 1].va + temp_ptov_table[ptov_index - 1].len) <= physmap_end);

	                                                                                                      
	                                                                                  
	qsort(temp_ptov_table, PTOV_TABLE_SIZE, sizeof(temp_ptov_table[0]), cmp_ptov_entries);

	memcpy(ptov_table, temp_ptov_table, sizeof(ptov_table));
}

#else

static void
arm_vm_physmap_init(boot_args *args)
{
	ptov_table_entry temp_ptov_table[PTOV_TABLE_SIZE];
	bzero(temp_ptov_table, sizeof(temp_ptov_table));

	                                                                                      
	arm_vm_physmap_slide(temp_ptov_table, gVirtBase, segLOWEST - gVirtBase, AP_RWNA,
	    ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);

	arm_vm_page_granular_RWNX(end_kern, phystokv(args->topOfKernelData) - end_kern,
	    ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);                                                                        

	arm_vm_physmap_slide(temp_ptov_table, (args->topOfKernelData - gPhysBase + gVirtBase),
	    real_avail_end - args->topOfKernelData, AP_RWNA, ARM64_GRANULE_ALLOW_BLOCK | ARM64_GRANULE_ALLOW_HINT);                   

	assert((temp_ptov_table[ptov_index - 1].va + temp_ptov_table[ptov_index - 1].len) <= physmap_end);

	                                                                                                      
	                                                                                  
	qsort(temp_ptov_table, PTOV_TABLE_SIZE, sizeof(temp_ptov_table[0]), cmp_ptov_entries);

	memcpy(ptov_table, temp_ptov_table, sizeof(ptov_table));
}

#endif               

void
arm_vm_prot_finalize(boot_args * args __unused)
{
	  
                                                                              
                                                                            
                               
   
                                                                            
              
    

	  
                                                                                            
                                                                                     
                                                                                          
                                                        
    

	ml_static_mfree(phystokv(gPhysBase), segLOWEST - gVirtBase);

	  
                                                                        
                                                                   
    
	if (!segSizePLKDATACONST && !segSizePLKTEXTEXEC && segSizePRELINKTEXT) {
		                                                                                                           
		ml_static_mfree(segPRELINKTEXTB + segSizePRELINKTEXT, segTEXTB - (segPRELINKTEXTB + segSizePRELINKTEXT));
	}

	                                                         
	arm_vm_page_granular_RNX(segPRELINKTEXTB, segSizePRELINKTEXT, ARM64_GRANULE_ALLOW_BLOCK);
	arm_vm_page_granular_RNX(segPLKDATACONSTB, segSizePLKDATACONST, ARM64_GRANULE_ALLOW_BLOCK);

	cpu_stack_alloc(&BootCpuData);
	arm64_replace_bootstack(&BootCpuData);
	ml_static_mfree(phystokv(segBOOTDATAB - gVirtBase + gPhysBase), segSizeBOOTDATA);

#if __ARM_KERNEL_PROTECT__
	arm_vm_populate_kernel_el0_mappings();
#endif                             

#if XNU_MONITOR
#if !defined(KERNEL_INTEGRITY_KTRR) && !defined(KERNEL_INTEGRITY_CTRR)
	                                          
	for (vm_offset_t va = segKLDB; va < (segKLDB + segSizeKLD); va += ARM_PGBYTES) {
		pt_entry_t *pte = arm_kva_to_pte(va);
		*pte = ARM_PTE_EMPTY;
	}
#endif
	for (vm_offset_t va = segKLDDATAB; va < (segKLDDATAB + segSizeKLDDATA); va += ARM_PGBYTES) {
		pt_entry_t *pte = arm_kva_to_pte(va);
		*pte = ARM_PTE_EMPTY;
	}
	                                                                                         
	for (vm_offset_t va = segBOOTDATAB; va < (segBOOTDATAB + segSizeBOOTDATA); va += ARM_PGBYTES) {
		pt_entry_t *pte = arm_kva_to_pte(va);
		*pte = ARM_PTE_EMPTY;
	}
	                                                                                           
                                                 
	for (vm_offset_t va = segPRELINKINFOB; va < (segPRELINKINFOB + segSizePRELINKINFO); va += ARM_PGBYTES) {
		pt_entry_t *pte = arm_kva_to_pte(va);
		*pte = ARM_PTE_EMPTY;
	}
	if (!keep_linkedit) {
		for (vm_offset_t va = segLINKB; va < (segLINKB + segSizeLINK); va += ARM_PGBYTES) {
			pt_entry_t *pte = arm_kva_to_pte(va);
			*pte = ARM_PTE_EMPTY;
		}
		for (vm_offset_t va = segPLKLINKEDITB; va < (segPLKLINKEDITB + segSizePLKLINKEDIT); va += ARM_PGBYTES) {
			pt_entry_t *pte = arm_kva_to_pte(va);
			*pte = ARM_PTE_EMPTY;
		}
	}
#endif                  

#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
	  
                                                  
    
	arm_vm_page_granular_RNX(segLASTB, segSizeLAST, ARM64_GRANULE_ALLOW_BLOCK);

	                                                    
	if (segLASTDATACONSTB) {
		arm_vm_page_granular_RNX(segLASTDATACONSTB, segSizeLASTDATACONST, ARM64_GRANULE_ALLOW_BLOCK);
	}

	  
                                                
    
	arm_vm_page_granular_RNX(segKLDB, segSizeKLD, ARM64_GRANULE_ALLOW_BLOCK);

	  
                                                                                       
                                                                               
                              
    
#endif

	arm_vm_page_granular_RNX(segDATACONSTB, segSizeDATACONST, ARM64_GRANULE_ALLOW_BLOCK);

	__builtin_arm_dsb(DSB_ISH);
	flush_mmu_tlb();
}

#define TBI_USER 0x1
#define TBI_KERNEL 0x2

  
                                                                           
                                                                      
                                                         
   
static void
set_tbi(void)
{
#if !__ARM_KERNEL_PROTECT__
	uint64_t old_tcr, new_tcr;

	old_tcr = new_tcr = get_tcr();
	new_tcr |= TCR_TBI0_TOPBYTE_IGNORED;

	if (old_tcr != new_tcr) {
		set_tcr(new_tcr);
		sysreg_restore.tcr_el1 = new_tcr;
	}
#endif                              
}

  
                                                                                                   
  
                                                                                                       
                                                                                                          
                                                                                                    
  
                                                                                                          
                                                                                  
  
              
  
                                                        
                               
                       
                                                                                       
                                                                                            
  
   

static void
init_ptpages(tt_entry_t *tt, vm_map_address_t start, vm_map_address_t end, bool static_map, uint64_t table_attrs)
{
	tt_entry_t *l1_tte;
	vm_offset_t ptpage_vaddr;

	l1_tte = tt + ((start & ARM_TT_L1_INDEX_MASK) >> ARM_TT_L1_SHIFT);

	while (start < end) {
		if (*l1_tte == ARM_TTE_EMPTY) {
			                                                  
			ptpage_vaddr = alloc_ptpage(static_map);
			*l1_tte = (kvtophys(ptpage_vaddr) & ARM_TTE_TABLE_MASK) | ARM_TTE_TYPE_TABLE | ARM_TTE_VALID | table_attrs;
			bzero((void *)ptpage_vaddr, ARM_PGBYTES);
		}

		if ((start + ARM_TT_L1_SIZE) < start) {
			                                                                   
			break;
		}

		start += ARM_TT_L1_SIZE;
		l1_tte++;
	}
}

#define ARM64_PHYSMAP_SLIDE_RANGE (1ULL << 30)        
#define ARM64_PHYSMAP_SLIDE_MASK  (ARM64_PHYSMAP_SLIDE_RANGE - 1)

void
arm_vm_init(uint64_t memory_size, boot_args * args)
{
	vm_map_address_t va_l1, va_l1_end;
	tt_entry_t       *cpu_l1_tte;
	vm_map_address_t va_l2, va_l2_end;
	tt_entry_t       *cpu_l2_tte;
	pmap_paddr_t     boot_ttep;
	tt_entry_t       *boot_tte;
	uint64_t         mem_segments;
	vm_offset_t      ptpage_vaddr;
	vm_map_address_t dynamic_memory_begin;

	  
                                                                           
    
	gVirtBase = args->virtBase;
	gPhysBase = args->physBase;
#if KASAN
	real_phys_size = args->memSize + (shadow_ptop - shadow_pbase);
#else
	real_phys_size = args->memSize;
#endif
	  
                                                                        
                                                                         
                                                                         
                                                                        
                                                                       
                                                                             
                                                                             
    
	gPhysSize = mem_size = ((gPhysBase + args->memSize) & ~PAGE_MASK) - gPhysBase;

	mem_actual = args->memSizeActual ? args->memSizeActual : mem_size;

	if ((memory_size != 0) && (mem_size > memory_size)) {
		mem_size = memory_size;
		max_mem_actual = memory_size;
	} else {
		max_mem_actual = mem_actual;
	}
	if (mem_size >= ((VM_MAX_KERNEL_ADDRESS - VM_MIN_KERNEL_ADDRESS) / 2)) {
		panic("Unsupported memory configuration %lx\n", mem_size);
	}

#if defined(ARM_LARGE_MEMORY)
	unsigned long physmap_l1_entries = ((real_phys_size + ARM64_PHYSMAP_SLIDE_RANGE) >> ARM_TT_L1_SHIFT) + 1;
	physmap_base = VM_MIN_KERNEL_ADDRESS - (physmap_l1_entries << ARM_TT_L1_SHIFT);
#else
	physmap_base = phystokv(args->topOfKernelData);
#endif

	                                                                                       
	uint64_t physmap_slide = early_random() & ARM64_PHYSMAP_SLIDE_MASK & ~((uint64_t)PAGE_MASK);
	assert(physmap_slide < ARM64_PHYSMAP_SLIDE_RANGE);

	physmap_base += physmap_slide;

#if XNU_MONITOR
	physmap_base = ROUND_TWIG(physmap_base);
#if defined(ARM_LARGE_MEMORY)
	static_memory_end = phystokv(args->topOfKernelData);
#else
	static_memory_end = physmap_base + mem_size;
#endif                    
	physmap_end = physmap_base + real_phys_size;
#else
	static_memory_end = physmap_base + mem_size + (PTOV_TABLE_SIZE * ARM_TT_TWIG_SIZE);                                           
	physmap_end = physmap_base + real_phys_size + (PTOV_TABLE_SIZE * ARM_TT_TWIG_SIZE);
#endif

#if KASAN && !defined(ARM_LARGE_MEMORY)
	                                                
	dynamic_memory_begin = static_memory_end + (shadow_ptop - shadow_pbase);
#else
	dynamic_memory_begin = static_memory_end;
#endif
#if XNU_MONITOR
	pmap_stacks_start = (void*)dynamic_memory_begin;
	dynamic_memory_begin += PPL_STACK_REGION_SIZE;
	pmap_stacks_end = (void*)dynamic_memory_begin;
#endif
	if (dynamic_memory_begin > VM_MAX_KERNEL_ADDRESS) {
		panic("Unsupported memory configuration %lx\n", mem_size);
	}

	boot_tte = (tt_entry_t *)&bootstrap_pagetables;
	boot_ttep = kvtophys((vm_offset_t)boot_tte);

#if DEVELOPMENT || DEBUG
	                                                                           
                                                         
	size_t bytes_mapped = dynamic_memory_begin - gVirtBase;
	size_t l1_entries = 1 + ((bytes_mapped + ARM_TT_L1_SIZE - 1) / ARM_TT_L1_SIZE);
	                                                        
	size_t pages_used = 2 * (l1_entries + 1);
	if (pages_used > BOOTSTRAP_TABLE_SIZE) {
		panic("BOOTSTRAP_TABLE_SIZE too small for memory config\n");
	}
#endif

	  
                                                
                                        
    

	  
                                                        
                                                                  
                                                                                
                                                                      
    
	avail_start = args->topOfKernelData;

#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
	arm_replace_identity_map();
#endif

	                                 
	invalid_tte = (tt_entry_t *)alloc_ptpage(TRUE);
	invalid_ttep = kvtophys((vm_offset_t)invalid_tte);
	bzero(invalid_tte, ARM_PGBYTES);

	  
                                 
    
	cpu_tte = (tt_entry_t *)alloc_ptpage(TRUE);
	cpu_ttep = kvtophys((vm_offset_t)cpu_tte);
	bzero(cpu_tte, ARM_PGBYTES);
	avail_end = gPhysBase + mem_size;
	assert(!(avail_end & PAGE_MASK));

#if KASAN
	real_avail_end = gPhysBase + real_phys_size;
#else
	real_avail_end = avail_end;
#endif

	  
                                           
                                                            
                                                    
   
                                                                 
    
	init_ptpages(cpu_tte, gVirtBase, dynamic_memory_begin, TRUE, 0);

#if defined(ARM_LARGE_MEMORY)
	  
                                    
                                                                           
                                                  
    
	init_ptpages(cpu_tte, physmap_base, ROUND_L1(physmap_end), TRUE, ARM_DYNAMIC_TABLE_XN);
#endif


#if __ARM_KERNEL_PROTECT__
	                                                             
	arm_vm_expand_kernel_el0_mappings();
#endif                             

	  
                                                                         
    
	segPRELINKTEXTB  = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__PRELINK_TEXT", &segSizePRELINKTEXT);
	segPLKDATACONSTB = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__PLK_DATA_CONST", &segSizePLKDATACONST);
	segPLKTEXTEXECB  = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__PLK_TEXT_EXEC", &segSizePLKTEXTEXEC);
	segTEXTB         = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__TEXT", &segSizeTEXT);
	segDATACONSTB    = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__DATA_CONST", &segSizeDATACONST);
	segTEXTEXECB     = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__TEXT_EXEC", &segSizeTEXTEXEC);
#if XNU_MONITOR
	segPPLTEXTB      = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__PPLTEXT", &segSizePPLTEXT);
	segPPLTRAMPB     = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__PPLTRAMP", &segSizePPLTRAMP);
	segPPLDATACONSTB = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__PPLDATA_CONST", &segSizePPLDATACONST);
#endif
	segDATAB         = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__DATA", &segSizeDATA);
#if XNU_MONITOR
	segPPLDATAB      = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__PPLDATA", &segSizePPLDATA);
#endif

	segBOOTDATAB     = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__BOOTDATA", &segSizeBOOTDATA);
	segLINKB         = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__LINKEDIT", &segSizeLINK);
	segKLDB          = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__KLD", &segSizeKLD);
	segKLDDATAB      = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__KLDDATA", &segSizeKLDDATA);
	segPRELINKDATAB  = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__PRELINK_DATA", &segSizePRELINKDATA);
	segPRELINKINFOB  = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__PRELINK_INFO", &segSizePRELINKINFO);
	segPLKLLVMCOVB   = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__PLK_LLVM_COV", &segSizePLKLLVMCOV);
	segPLKLINKEDITB  = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__PLK_LINKEDIT", &segSizePLKLINKEDIT);
	segLASTB         = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__LAST", &segSizeLAST);
	segLASTDATACONSTB = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__LASTDATA_CONST", &segSizeLASTDATACONST);

	sectHIBTEXTB     = (vm_offset_t) getsectdatafromheader(&_mh_execute_header, "__TEXT_EXEC", "__hib_text", &sectSizeHIBTEXT);
	sectHIBDATACONSTB = (vm_offset_t) getsectdatafromheader(&_mh_execute_header, "__DATA_CONST", "__hib_const", &sectSizeHIBDATACONST);
	segHIBDATAB      = (vm_offset_t) getsegdatafromheader(&_mh_execute_header, "__HIBDATA", &segSizeHIBDATA);

	if (kernel_mach_header_is_in_fileset(&_mh_execute_header)) {
		kernel_mach_header_t *kc_mh = PE_get_kc_header(KCKindPrimary);

		                                                                                           
		segKCTEXTEXECB = (vm_offset_t) getsegdatafromheader(kc_mh,             "__TEXT_EXEC", &segSizeKCTEXTEXEC);
		assert(segPLKTEXTEXECB && !segSizePLKTEXTEXEC);                                                             

		assert(segLASTB);                                                                                                   
		                                                                                                                 

		assert(segKCTEXTEXECB <= segLASTB);                                                                            
		assert(segKCTEXTEXECB + segSizeKCTEXTEXEC >= segLASTB + segSizeLAST);
		segPLKTEXTEXECB = segLASTB + segSizeLAST;
		segSizePLKTEXTEXEC = segSizeKCTEXTEXEC - (segPLKTEXTEXECB - segKCTEXTEXECB);

		                                                                                                       
		segKCDATACONSTB = (vm_offset_t) getsegdatafromheader(kc_mh,            "__DATA_CONST", &segSizeKCDATACONST);
		assert(segPLKDATACONSTB && !segSizePLKDATACONST);                                                            
		assert(segLASTDATACONSTB && segSizeLASTDATACONST);                                                               
		assert(segKCDATACONSTB <= segLASTDATACONSTB);                                                                             
		assert(segKCDATACONSTB + segSizeKCDATACONST >= segLASTDATACONSTB + segSizeLASTDATACONST);
		segPLKDATACONSTB = segLASTDATACONSTB + segSizeLASTDATACONST;
		segSizePLKDATACONST = segSizeKCDATACONST - (segPLKDATACONSTB - segKCDATACONSTB);

		                                                                                                   
		segKCDATAB      = (vm_offset_t) getsegdatafromheader(kc_mh,            "__DATA", &segSizeKCDATA);
		assert(segPRELINKDATAB && !segSizePRELINKDATA);                                                            
		assert(segKCDATAB <= segPRELINKDATAB);                                                                            
		assert(segKCDATAB + segSizeKCDATA >= segPRELINKDATAB + segSizePRELINKDATA);
		segSizePRELINKDATA = segSizeKCDATA - (segPRELINKDATAB - segKCDATAB);

		                                                                                                  
		assert(segPRELINKTEXTB && !segSizePRELINKTEXT);                                                            
		segPRELINKTEXTB = (vm_offset_t) getsegdatafromheader(kc_mh,            "__PRELINK_TEXT", &segSizePRELINKTEXT);
		assert(segPRELINKINFOB && !segSizePRELINKINFO);                                                            
		segPRELINKINFOB = (vm_offset_t) getsegdatafromheader(kc_mh,            "__PRELINK_INFO", &segSizePRELINKINFO);
		segLINKB        = (vm_offset_t) getsegdatafromheader(kc_mh,            "__LINKEDIT", &segSizeLINK);
	}

	(void) PE_parse_boot_argn("use_contiguous_hint", &use_contiguous_hint, sizeof(use_contiguous_hint));
	assert(segSizePRELINKTEXT < 0x03000000);               

	                                                                            
	if (segSizePLKDATACONST || segSizePLKTEXTEXEC) {
		assert(segSizePLKDATACONST && segSizePLKTEXTEXEC);
	}

	etext = (vm_offset_t) segTEXTB + segSizeTEXT;
	sdata = (vm_offset_t) segDATAB;
	edata = (vm_offset_t) segDATAB + segSizeDATA;
	end_kern = round_page(segHIGHESTKC ? segHIGHESTKC : getlastaddr());                             

	vm_set_page_size();

	vm_kernel_base = segTEXTB;
	vm_kernel_top = (vm_offset_t) &last_kernel_symbol;
	vm_kext_base = segPRELINKTEXTB;
	vm_kext_top = vm_kext_base + segSizePRELINKTEXT;

	vm_prelink_stext = segPRELINKTEXTB;
	if (!segSizePLKTEXTEXEC && !segSizePLKDATACONST) {
		vm_prelink_etext = segPRELINKTEXTB + segSizePRELINKTEXT;
	} else {
		vm_prelink_etext = segPRELINKTEXTB + segSizePRELINKTEXT + segSizePLKDATACONST + segSizePLKTEXTEXEC;
	}
	vm_prelink_sinfo = segPRELINKINFOB;
	vm_prelink_einfo = segPRELINKINFOB + segSizePRELINKINFO;
	vm_slinkedit = segLINKB;
	vm_elinkedit = segLINKB + segSizeLINK;

	vm_prelink_sdata = segPRELINKDATAB;
	vm_prelink_edata = segPRELINKDATAB + segSizePRELINKDATA;

	arm_vm_prot_init(args);

	vm_page_kernelcache_count = (unsigned int) (atop_64(end_kern - segLOWEST));

	  
                                                   
                               
                                     
    
	va_l1 = va_l2 = LOW_GLOBAL_BASE_ADDRESS;
	cpu_l1_tte = cpu_tte + ((va_l1 & ARM_TT_L1_INDEX_MASK) >> ARM_TT_L1_SHIFT);
	cpu_l2_tte = ((tt_entry_t *) phystokv(((*cpu_l1_tte) & ARM_TTE_TABLE_MASK))) + ((va_l2 & ARM_TT_L2_INDEX_MASK) >> ARM_TT_L2_SHIFT);
	ptpage_vaddr = alloc_ptpage(TRUE);
	*cpu_l2_tte = (kvtophys(ptpage_vaddr) & ARM_TTE_TABLE_MASK) | ARM_TTE_TYPE_TABLE | ARM_TTE_VALID | ARM_TTE_TABLE_PXN | ARM_TTE_TABLE_XN;
	bzero((void *)ptpage_vaddr, ARM_PGBYTES);

	  
                                    
                               
                                                  
    
#if defined(ARM_LARGE_MEMORY)
	  
                                                                                           
                                                                                                   
    
	init_ptpages(cpu_tte, dynamic_memory_begin, ROUND_L1(dynamic_memory_begin), FALSE, ARM_DYNAMIC_TABLE_XN);
#else
	  
                                                            
                                                                                                          
    
	init_ptpages(cpu_tte, dynamic_memory_begin, VM_MAX_KERNEL_ADDRESS, TRUE, ARM_DYNAMIC_TABLE_XN);
#endif

#if KASAN
	                                      
	physmap_vbase = physmap_base;
	physmap_vtop = physmap_end;
	kasan_init();
#endif            

#if MONOTONIC
	mt_early_init();
#endif                

	set_tbi();

	arm_vm_physmap_init(args);
	set_mmu_ttb_alternate(cpu_ttep & TTBR_BADDR_MASK);

	ml_enable_monitor();

	set_mmu_ttb(invalid_ttep & TTBR_BADDR_MASK);

	flush_mmu_tlb();
#if defined(HAS_VMSA_LOCK)
	vmsa_lock();
#endif
	kva_active = TRUE;
	                                                                                    
	cpu_tte = (tt_entry_t*)(phystokv(cpu_ttep));
	invalid_tte = (tt_entry_t*)(phystokv(invalid_ttep));

	                                                                


	                                                                  
                                                             
                                                            
   
                                                                
                                  
    

	if (arm_vm_auxkc_init()) {
		if (segLOWESTROAuxKC < segLOWESTRO) {
			segLOWESTRO = segLOWESTROAuxKC;
		}
		if (segHIGHESTROAuxKC > segHIGHESTRO) {
			segHIGHESTRO = segHIGHESTROAuxKC;
		}
		if (segLOWESTRXAuxKC < segLOWESTTEXT) {
			segLOWESTTEXT = segLOWESTRXAuxKC;
		}
		assert(segLOWEST == segLOWESTAuxKC);

		                                                    
		flush_mmu_tlb();
	}

	sane_size = mem_size - (avail_start - gPhysBase);
	max_mem = mem_size;
	vm_kernel_slid_base = segLOWESTTEXT;
	vm_kernel_slid_top = vm_prelink_einfo;
	                                                                          
	vm_kernel_stext = segTEXTB;

	if (kernel_mach_header_is_in_fileset(&_mh_execute_header)) {
		                                                 
		assert(segTEXTEXECB == segTEXTB + segSizeTEXT);
		vm_kernel_etext = segTEXTB + segSizeTEXT + segSizeTEXTEXEC;
	} else {
		assert(segDATACONSTB == segTEXTB + segSizeTEXT);
		assert(segTEXTEXECB == segDATACONSTB + segSizeDATACONST);
		vm_kernel_etext = segTEXTB + segSizeTEXT + segSizeDATACONST + segSizeTEXTEXEC;
	}

	dynamic_memory_begin = ROUND_TWIG(dynamic_memory_begin);
#if defined(KERNEL_INTEGRITY_CTRR) && defined(CONFIG_XNUPOST)
	                                                                                       
	{
		extern vm_offset_t ctrr_test_page;
		tt_entry_t *new_tte;

		ctrr_test_page = dynamic_memory_begin;
		dynamic_memory_begin += ARM_TT_L2_SIZE;
		cpu_l1_tte = cpu_tte + ((ctrr_test_page & ARM_TT_L1_INDEX_MASK) >> ARM_TT_L1_SHIFT);
		assert((*cpu_l1_tte) & ARM_TTE_VALID);
		cpu_l2_tte = ((tt_entry_t *) phystokv(((*cpu_l1_tte) & ARM_TTE_TABLE_MASK))) + ((ctrr_test_page & ARM_TT_L2_INDEX_MASK) >> ARM_TT_L2_SHIFT);
		assert((*cpu_l2_tte) == ARM_TTE_EMPTY);
		new_tte = (tt_entry_t *)alloc_ptpage(FALSE);
		bzero(new_tte, ARM_PGBYTES);
		*cpu_l2_tte = (kvtophys((vm_offset_t)new_tte) & ARM_TTE_TABLE_MASK) | ARM_TTE_TYPE_TABLE | ARM_TTE_VALID;
	}
#endif                                                                
#if XNU_MONITOR
	for (vm_offset_t cur = (vm_offset_t)pmap_stacks_start; cur < (vm_offset_t)pmap_stacks_end; cur += ARM_PGBYTES) {
		arm_vm_map(cpu_tte, cur, ARM_PTE_EMPTY);
	}
#endif
	pmap_bootstrap(dynamic_memory_begin);

	disable_preemption();

	  
                                    
                               
                                                           
    

	mem_segments = (mem_size + 0x0FFFFFFF) >> 28;

	va_l1 = dynamic_memory_begin;
	va_l1_end = va_l1 + ((2 + (mem_segments * 10)) << 20);
	va_l1_end += round_page(args->Video.v_height * args->Video.v_rowBytes);
	va_l1_end = (va_l1_end + 0x00000000007FFFFFULL) & 0xFFFFFFFFFF800000ULL;

	cpu_l1_tte = cpu_tte + ((va_l1 & ARM_TT_L1_INDEX_MASK) >> ARM_TT_L1_SHIFT);

	while (va_l1 < va_l1_end) {
		va_l2 = va_l1;

		if (((va_l1 & ~ARM_TT_L1_OFFMASK) + ARM_TT_L1_SIZE) < va_l1) {
			                                                                   
			va_l2_end = va_l1_end;
		} else {
			va_l2_end = MIN((va_l1 & ~ARM_TT_L1_OFFMASK) + ARM_TT_L1_SIZE, va_l1_end);
		}

		cpu_l2_tte = ((tt_entry_t *) phystokv(((*cpu_l1_tte) & ARM_TTE_TABLE_MASK))) + ((va_l2 & ARM_TT_L2_INDEX_MASK) >> ARM_TT_L2_SHIFT);

		while (va_l2 < va_l2_end) {
			pt_entry_t *    ptp;
			pmap_paddr_t    ptp_phys;

			                                                  
			ptp = (pt_entry_t *) alloc_ptpage(FALSE);
			ptp_phys = (pmap_paddr_t)kvtophys((vm_offset_t)ptp);

			bzero(ptp, ARM_PGBYTES);
			pmap_init_pte_page(kernel_pmap, ptp, va_l2, 3, TRUE);

			*cpu_l2_tte = (pa_to_tte(ptp_phys)) | ARM_TTE_TYPE_TABLE | ARM_TTE_VALID | ARM_DYNAMIC_TABLE_XN;

			va_l2 += ARM_TT_L2_SIZE;
			cpu_l2_tte++;
		}

		va_l1 = va_l2_end;
		cpu_l1_tte++;
	}

#if defined(KERNEL_INTEGRITY_KTRR) || defined(KERNEL_INTEGRITY_CTRR)
	  
                                                                   
                                                                    
                                 
    
#if defined(ARM_LARGE_MEMORY)
	init_ptpages(cpu_tte, KERNEL_PMAP_HEAP_RANGE_START & ~ARM_TT_L1_OFFMASK, VM_MAX_KERNEL_ADDRESS, FALSE, ARM_DYNAMIC_TABLE_XN);
#else                             
	va_l1 = VM_MIN_KERNEL_ADDRESS & ~ARM_TT_L1_OFFMASK;
	init_ptpages(cpu_tte, VM_MIN_KERNEL_ADDRESS & ~ARM_TT_L1_OFFMASK, VM_MAX_KERNEL_ADDRESS, FALSE, ARM_DYNAMIC_TABLE_XN);
#endif                             
#endif                                                                    

	  
                                    
                               
                                                                                                  
    
	va_l1 = (VM_MAX_KERNEL_ADDRESS & CPUWINDOWS_BASE_MASK) - PE_EARLY_BOOT_VA;
	va_l1_end = VM_MAX_KERNEL_ADDRESS;

	cpu_l1_tte = cpu_tte + ((va_l1 & ARM_TT_L1_INDEX_MASK) >> ARM_TT_L1_SHIFT);

	while (va_l1 < va_l1_end) {
		va_l2 = va_l1;

		if (((va_l1 & ~ARM_TT_L1_OFFMASK) + ARM_TT_L1_SIZE) < va_l1) {
			                                                                   
			va_l2_end = va_l1_end;
		} else {
			va_l2_end = MIN((va_l1 & ~ARM_TT_L1_OFFMASK) + ARM_TT_L1_SIZE, va_l1_end);
		}

		cpu_l2_tte = ((tt_entry_t *) phystokv(((*cpu_l1_tte) & ARM_TTE_TABLE_MASK))) + ((va_l2 & ARM_TT_L2_INDEX_MASK) >> ARM_TT_L2_SHIFT);

		while (va_l2 < va_l2_end) {
			pt_entry_t *    ptp;
			pmap_paddr_t    ptp_phys;

			                                                  
			ptp = (pt_entry_t *) alloc_ptpage(FALSE);
			ptp_phys = (pmap_paddr_t)kvtophys((vm_offset_t)ptp);

			bzero(ptp, ARM_PGBYTES);
			pmap_init_pte_page(kernel_pmap, ptp, va_l2, 3, TRUE);

			*cpu_l2_tte = (pa_to_tte(ptp_phys)) | ARM_TTE_TYPE_TABLE | ARM_TTE_VALID | ARM_DYNAMIC_TABLE_XN;

			va_l2 += ARM_TT_L2_SIZE;
			cpu_l2_tte++;
		}

		va_l1 = va_l2_end;
		cpu_l1_tte++;
	}


	  
                                                         
                                           
    
	avail_start = (avail_start + PAGE_MASK) & ~PAGE_MASK;

#if XNU_MONITOR
	pmap_static_allocations_done();
#endif
	first_avail = avail_start;
	patch_low_glo_static_region(args->topOfKernelData, avail_start - args->topOfKernelData);
	enable_preemption();
}
