/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
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
  
   

  
               
  
                                                      
             
  
                                                            
   
#ifdef KERNEL_PRIVATE
#ifndef _PMAP_MACHINE_
#define _PMAP_MACHINE_  1

#ifndef ASSEMBLER

#include <mach/kern_return.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/vm_statistics.h>
#include <mach/machine/vm_param.h>
#include <kern/kern_types.h>
#include <kern/thread.h>
#include <kern/simple_lock.h>

#include <i386/mp.h>
#include <i386/proc_reg.h>

#include <i386/pal_routines.h>

  
                                              
   

#define INTEL_PGBYTES           I386_PGBYTES
#define INTEL_PGSHIFT           I386_PGSHIFT
#define intel_btop(x)           i386_btop(x)
#define intel_ptob(x)           i386_ptob(x)
#define intel_round_page(x)     i386_round_page(x)
#define intel_trunc_page(x)     i386_trunc_page(x)

  
                                  
   

#endif                 

#define NPGPTD          4ULL
#define PDESHIFT        21ULL
#define PTEMASK         0x1ffULL
#define PTEINDX         3ULL

#define PTESHIFT        12ULL

#define LOW_4GB_MASK    ((vm_offset_t)0x00000000FFFFFFFFUL)

#define PDESIZE         sizeof(pd_entry_t)                         
#define PTESIZE         sizeof(pt_entry_t)                         

#define INTEL_OFFMASK   (I386_PGBYTES - 1)
#define INTEL_LOFFMASK  (I386_LPGBYTES - 1)
#define PG_FRAME        0x000FFFFFFFFFF000ULL
#define NPTEPG          (PAGE_SIZE/(sizeof (pt_entry_t)))
#define NPTDPG          (PAGE_SIZE/(sizeof (pd_entry_t)))

#define NBPTD           (NPGPTD << PAGE_SHIFT)
#define NPDEPTD         (NBPTD / (sizeof (pd_entry_t)))
#define NPDEPG          (PAGE_SIZE/(sizeof (pd_entry_t)))
#define NBPDE           (1ULL << PDESHIFT)
#define PDEMASK         (NBPDE - 1)

#define PTE_PER_PAGE    512                                            

                                                             
typedef uint64_t        pml4_entry_t;
#define NPML4PG         (PAGE_SIZE/(sizeof (pml4_entry_t)))
#define PML4SHIFT       39
#define PML4PGSHIFT     9
#define NBPML4          (1ULL << PML4SHIFT)
#define PML4MASK        (NBPML4-1)
#define PML4_ENTRY_NULL ((pml4_entry_t *) 0)

typedef uint64_t        pdpt_entry_t;
#define NPDPTPG         (PAGE_SIZE/(sizeof (pdpt_entry_t)))
#define PDPTSHIFT       30
#define PDPTPGSHIFT     9
#define NBPDPT          (1ULL << PDPTSHIFT)
#define PDPTMASK        (NBPDPT-1)
#define PDPT_ENTRY_NULL ((pdpt_entry_t *) 0)

typedef uint64_t        pd_entry_t;
#define NPDPG           (PAGE_SIZE/(sizeof (pd_entry_t)))
#define PDSHIFT         21
#define PDPGSHIFT       9
#define NBPD            (1ULL << PDSHIFT)
#define PDMASK          (NBPD-1)
#define PD_ENTRY_NULL   ((pd_entry_t *) 0)

typedef uint64_t        pt_entry_t;
#define NPTPG           (PAGE_SIZE/(sizeof (pt_entry_t)))
#define PTSHIFT         12
#define PTPGSHIFT       9
#define NBPT            (1ULL << PTSHIFT)
#define PTMASK          (NBPT-1)
#define PT_ENTRY_NULL   ((pt_entry_t *) 0)

typedef uint64_t  pmap_paddr_t;

#if     DEVELOPMENT || DEBUG
#define PMAP_ASSERT 1
extern int pmap_asserts_enabled;
extern int pmap_asserts_traced;
#endif

#if PMAP_ASSERT
#define pmap_assert(ex) (pmap_asserts_enabled ? ((ex) ? (void)0 : Assert(__FILE__, __LINE__, # ex)) : (void)0)

#define pmap_assert2(ex, fmt, args...)                                  \
	do {                                                            \
	        if (__improbable(pmap_asserts_enabled && !(ex))) {      \
	                if (pmap_asserts_traced) {                      \
	                        KERNEL_DEBUG_CONSTANT(0xDEAD1000, __builtin_return_address(0), __LINE__, 0, 0, 0); \
	                        kdebug_enable = 0;                      \
	                } else {                                        \
	                                kprintf("Assertion %s failed (%s:%d, caller %p) " fmt , #ex, __FILE__, __LINE__, __builtin_return_address(0),  ##args); \
	                                panic("Assertion %s failed (%s:%d, caller %p) " fmt , #ex, __FILE__, __LINE__, __builtin_return_address(0),  ##args); \
	                }                                               \
	        }                                                       \
	} while(0)
#else
#define pmap_assert(ex)
#define pmap_assert2(ex, fmt, args...)
#endif

                
#define SUPERPAGE_NBASEPAGES 512

  
                                             
   
static inline void
pmap_store_pte(pt_entry_t *entryp, pt_entry_t value)
{
	  
                                                        
                                                           
    
	*entryp = value;
}

                                                                          
#define NPML4PGS        (1ULL * (PAGE_SIZE/(sizeof (pml4_entry_t))))
#define NPDPTPGS        (NPML4PGS * (PAGE_SIZE/(sizeof (pdpt_entry_t))))
#define NPDEPGS         (NPDPTPGS * (PAGE_SIZE/(sizeof (pd_entry_t))))
#define NPTEPGS         (NPDEPGS * (PAGE_SIZE/(sizeof (pt_entry_t))))

extern int      kernPhysPML4Index;
extern int      kernPhysPML4EntryCount;

#define KERNEL_PML4_INDEX               511
#define KERNEL_KEXTS_INDEX              (KERNEL_PML4_INDEX - 1)                                                
#define KERNEL_PHYSMAP_PML4_INDEX       (kernPhysPML4Index)                                               
#define KERNEL_PHYSMAP_PML4_COUNT       (kernPhysPML4EntryCount)
#define KERNEL_PHYSMAP_PML4_COUNT_MAX   (16 - 2)                                          
                                                                                 
#define KERNEL_KASAN_PML4_LAST          (495)               
#define KERNEL_KASAN_PML4_FIRST         (494)               
#define KERNEL_DBLMAP_PML4_INDEX        (KERNEL_KASAN_PML4_FIRST - 1)
#define KERNEL_PML4_COUNT               1
#define KERNEL_BASE                     (0ULL - (NBPML4 * KERNEL_PML4_COUNT))
#define KERNEL_BASEMENT                 (KERNEL_BASE - NBPML4)                                    

  
                     
   
#define KVADDR(pmi, pdpi, pdi, pti)               \
	 ((vm_offset_t)                   \
	        ((uint64_t) -1    << 47)        | \
	        ((uint64_t)(pmi)  << PML4SHIFT) | \
	        ((uint64_t)(pdpi) << PDPTSHIFT) | \
	        ((uint64_t)(pdi)  << PDESHIFT)  | \
	        ((uint64_t)(pti)  << PTESHIFT))


#ifndef NKPT
#define NKPT            500                                                        
#endif



  
                                                  
   
#define pdptnum(pmap, a) (((vm_offset_t)(a) >> PDPTSHIFT) & PDPTMASK)
#define pdenum(pmap, a) (((vm_offset_t)(a) >> PDESHIFT) & PDEMASK)
#define PMAP_INVALID_PDPTNUM (~0ULL)

#define pdeidx(pmap, a)    (((a) >> PDSHIFT)   & ((1ULL<<(48 - PDSHIFT)) -1))
#define pdptidx(pmap, a)   (((a) >> PDPTSHIFT) & ((1ULL<<(48 - PDPTSHIFT)) -1))
#define pml4idx(pmap, a)   (((a) >> PML4SHIFT) & ((1ULL<<(48 - PML4SHIFT)) -1))


  
                                                        
   
#define pdetova(a)      ((vm_offset_t)(a) << PDESHIFT)

  
                                             
   
#define ptenum(a)       (((vm_offset_t)(a) >> PTESHIFT) & PTEMASK)

  
                                                                
                                 
   

#define INTEL_PTE_VALID         0x00000001ULL

#define INTEL_PTE_WRITE         0x00000002ULL
#define INTEL_PTE_RW            0x00000002ULL

#define INTEL_PTE_USER          0x00000004ULL

#define INTEL_PTE_WTHRU         0x00000008ULL
#define INTEL_PTE_NCACHE        0x00000010ULL

#define INTEL_PTE_REF           0x00000020ULL
#define INTEL_PTE_MOD           0x00000040ULL

#define INTEL_PTE_PS            0x00000080ULL
#define INTEL_PTE_PAT           0x00000080ULL

#define INTEL_PTE_GLOBAL        0x00000100ULL

                                                           
                                                 
                                                             
                                                                      
               
   
#define INTEL_PTE_SWLOCK        (0x1ULL << 52)
#define INTEL_PDPTE_NESTED      (0x1ULL << 53)
#define INTEL_PTE_WIRED         (0x1ULL << 54)
                                                                        
#define INTEL_PTE_COMPRESSED_ALT (1ULL << 61)                                                 
#define INTEL_PTE_COMPRESSED    (1ULL << 62)                                                                                      

#define INTEL_PTE_PFN           PG_FRAME
                                                
#define INTEL_PTE_NX            (1ULL << 63)

#define INTEL_PTE_INVALID       0
                                        
#define INTEL_PTE_RSVD          ((1ULL << 10) | (1ULL << 11))


#define INTEL_PTE_COMPRESSED_MASK (INTEL_PTE_COMPRESSED | \
	                           INTEL_PTE_COMPRESSED_ALT | INTEL_PTE_SWLOCK)
#define PTE_IS_COMPRESSED(x, ptep, pmap, vaddr)                            \
	((((x) & INTEL_PTE_VALID) == 0) &&                                 \
	 ((x) & INTEL_PTE_COMPRESSED) &&                                   \
	 ((!((x) & ~INTEL_PTE_COMPRESSED_MASK)) ||                         \
	  pmap_compressed_pte_corruption_repair((x), &(x), (ptep), (pmap), (vaddr))))

#define pa_to_pte(a)            ((a) & INTEL_PTE_PFN)          
#define pte_to_pa(p)            ((p) & INTEL_PTE_PFN)          
#define pte_increment_pa(p)     ((p) += INTEL_OFFMASK+1)

#define pte_kernel_rw(p)          ((pt_entry_t)(pa_to_pte(p) | INTEL_PTE_VALID|INTEL_PTE_RW))
#define pte_kernel_ro(p)          ((pt_entry_t)(pa_to_pte(p) | INTEL_PTE_VALID))
#define pte_user_rw(p)            ((pt_entry_t)(pa_to_pte(p) | INTEL_PTE_VALID|INTEL_PTE_USER|INTEL_PTE_RW))
#define pte_user_ro(p)            ((pt_entry_t)(pa_to_pte(p) | INTEL_PTE_VALID|INTEL_PTE_USER))

#define PMAP_INVEPT_SINGLE_CONTEXT      1


#define INTEL_EPTP_AD           0x00000040ULL

#define INTEL_EPT_READ          0x00000001ULL
#define INTEL_EPT_WRITE         0x00000002ULL
#define INTEL_EPT_EX            0x00000004ULL
#define INTEL_EPT_IPAT          0x00000040ULL
#define INTEL_EPT_PS            0x00000080ULL
#define INTEL_EPT_REF           0x00000100ULL
#define INTEL_EPT_MOD           0x00000200ULL

#define INTEL_EPT_CACHE_MASK    0x00000038ULL
#define INTEL_EPT_NCACHE        0x00000000ULL
#define INTEL_EPT_WC            0x00000008ULL
#define INTEL_EPT_WTHRU         0x00000020ULL
#define INTEL_EPT_WP            0x00000028ULL
#define INTEL_EPT_WB            0x00000030ULL

  
                                                             
   

static inline pt_entry_t
pte_remove_ex(pt_entry_t pte, boolean_t is_ept)
{
	if (__probable(!is_ept)) {
		return pte | INTEL_PTE_NX;
	}

	return pte & (~INTEL_EPT_EX);
}

static inline pt_entry_t
pte_set_ex(pt_entry_t pte, boolean_t is_ept)
{
	if (__probable(!is_ept)) {
		return pte & (~INTEL_PTE_NX);
	}

	return pte | INTEL_EPT_EX;
}

static inline pt_entry_t
physmap_refmod_to_ept(pt_entry_t physmap_pte)
{
	pt_entry_t ept_pte = 0;

	if (physmap_pte & INTEL_PTE_MOD) {
		ept_pte |= INTEL_EPT_MOD;
	}

	if (physmap_pte & INTEL_PTE_REF) {
		ept_pte |= INTEL_EPT_REF;
	}

	return ept_pte;
}

static inline pt_entry_t
ept_refmod_to_physmap(pt_entry_t ept_pte)
{
	pt_entry_t physmap_pte = 0;

	assert((ept_pte & ~(INTEL_EPT_REF | INTEL_EPT_MOD)) == 0);

	if (ept_pte & INTEL_EPT_REF) {
		physmap_pte |= INTEL_PTE_REF;
	}

	if (ept_pte & INTEL_EPT_MOD) {
		physmap_pte |= INTEL_PTE_MOD;
	}

	return physmap_pte;
}

  
                                                                               
                                                                           
                                         
   
extern boolean_t pmap_ept_support_ad;

#define PTE_VALID_MASK(is_ept)  ((is_ept) ? (INTEL_EPT_READ | INTEL_EPT_WRITE | INTEL_EPT_EX) : INTEL_PTE_VALID)
#define PTE_READ(is_ept)        ((is_ept) ? INTEL_EPT_READ : INTEL_PTE_VALID)
#define PTE_WRITE(is_ept)       ((is_ept) ? INTEL_EPT_WRITE : INTEL_PTE_WRITE)
#define PTE_IS_EXECUTABLE(is_ept, pte)  ((is_ept) ? (((pte) & INTEL_EPT_EX) != 0) : (((pte) & INTEL_PTE_NX) == 0))
#define PTE_PS                  INTEL_PTE_PS
#define PTE_COMPRESSED          INTEL_PTE_COMPRESSED
#define PTE_COMPRESSED_ALT      INTEL_PTE_COMPRESSED_ALT
#define PTE_NCACHE(is_ept)      ((is_ept) ? INTEL_EPT_NCACHE : INTEL_PTE_NCACHE)
#define PTE_WTHRU(is_ept)       ((is_ept) ? INTEL_EPT_WTHRU : INTEL_PTE_WTHRU)
#define PTE_REF(is_ept)         ((is_ept) ? INTEL_EPT_REF : INTEL_PTE_REF)
#define PTE_MOD(is_ept)         ((is_ept) ? INTEL_EPT_MOD : INTEL_PTE_MOD)
#define PTE_WIRED               INTEL_PTE_WIRED


#define PMAP_DEFAULT_CACHE      0
#define PMAP_INHIBIT_CACHE      1
#define PMAP_GUARDED_CACHE      2
#define PMAP_ACTIVATE_CACHE     4
#define PMAP_NO_GUARD_CACHE     8

                                
#define pmap_ledger_debit(p, e, a) ledger_debit((p)->ledger, e, a)
#define pmap_ledger_credit(p, e, a) ledger_credit((p)->ledger, e, a)

#ifndef ASSEMBLER

#include <sys/queue.h>

  
                                                                 
                   
   

extern pt_entry_t       *PTmap;
extern pdpt_entry_t     *IdlePDPT;
extern pml4_entry_t     *IdlePML4;
extern boolean_t        no_shared_cr3;
extern pd_entry_t       *IdlePTD;                                              

extern uint64_t         pmap_pv_hashlist_walks;
extern uint64_t         pmap_pv_hashlist_cnts;
extern uint32_t         pmap_pv_hashlist_max;
extern uint32_t         pmap_kernel_text_ps;

#define ID_MAP_VTOP(x)  ((void *)(((uint64_t)(x)) & LOW_4GB_MASK))

extern  uint64_t physmap_base, physmap_max;

#define NPHYSMAP (MAX(((physmap_max - physmap_base) / GB), 4))

static inline boolean_t
physmap_enclosed(addr64_t a)
{
	return a < (NPHYSMAP * GB);
}

static  inline void *
PHYSMAP_PTOV_check(void *paddr)
{
	uint64_t pvaddr = (uint64_t)paddr + physmap_base;

	if (__improbable(pvaddr >= physmap_max)) {
		panic("PHYSMAP_PTOV bounds exceeded, 0x%qx, 0x%qx, 0x%qx",
		    pvaddr, physmap_base, physmap_max);
	}

	return (void *)pvaddr;
}

#define PHYSMAP_PTOV(x) (PHYSMAP_PTOV_check((void*) (x)))
#define phystokv(x) ((vm_offset_t)(PHYSMAP_PTOV(x)))
#if MACH_KERNEL_PRIVATE
extern uint64_t dblmap_base, dblmap_max, dblmap_dist;

static inline uint64_t
DBLMAP_CHECK(uintptr_t x)
{
	uint64_t dbladdr = (uint64_t)x + dblmap_dist;
	if (__improbable((dbladdr >= dblmap_max) || (dbladdr < dblmap_base))) {
		panic("DBLMAP bounds exceeded, 0x%qx, 0x%qx 0x%qx, 0x%qx",
		    (uint64_t)x, dbladdr, dblmap_base, dblmap_max);
	}
	return dbladdr;
}
#define DBLMAP(x) (DBLMAP_CHECK((uint64_t) x))
extern uint64_t ldt_alias_offset;
static inline uint64_t
LDTALIAS_CHECK(uintptr_t x)
{
	uint64_t dbladdr = (uint64_t)x + ldt_alias_offset;
	if (__improbable((dbladdr >= dblmap_max) || (dbladdr < dblmap_base))) {
		panic("LDTALIAS: bounds exceeded, 0x%qx, 0x%qx 0x%qx, 0x%qx",
		    (uint64_t)x, dbladdr, dblmap_base, dblmap_max);
	}
	return dbladdr;
}
#define LDTALIAS(x) (LDTALIAS_CHECK((uint64_t) x))
#endif

  
                                                                  
                                                         
                                                                           
                                                              
   
#define LOWGLOBAL_ALIAS         (VM_MIN_KERNEL_ADDRESS + 0x2000)

  
                                                                
                                                         
   
#define KERNEL_PMAP_HEAP_RANGE_START VM_MIN_KERNEL_AND_KEXT_ADDRESS

#if MACH_KERNEL_PRIVATE
extern void
pmap_tlbi_range(uint64_t startv, uint64_t endv, bool global, uint16_t pcid);

#include <vm/vm_page.h>

  
                                                       
                                                    
                                          
   

struct pmap {
	lck_rw_t        pmap_rwl __attribute((aligned(64)));
	pmap_paddr_t    pm_cr3 __attribute((aligned(64)));                                      
	pmap_paddr_t    pm_ucr3;                                         
	pml4_entry_t    *pm_pml4;                             
	pml4_entry_t    *pm_upml4;                                   
	pmap_paddr_t    pm_eptp;                  

	task_map_t      pm_task_map;
	boolean_t       pagezero_accessible;
	boolean_t       pm_vm_map_cs_enforced;                             
#define PMAP_PCID_MAX_CPUS      MAX_CPUS                                     
	pcid_t          pmap_pcid_cpus[PMAP_PCID_MAX_CPUS];
	volatile uint8_t pmap_pcid_coherency_vector[PMAP_PCID_MAX_CPUS];
	boolean_t       pm_shared;
	os_refcnt_t     ref_count;
	pdpt_entry_t    *pm_pdpt;                                  
	vm_object_t     pm_obj;                                   
	vm_object_t     pm_obj_pdpt;                          
	vm_object_t     pm_obj_pml4;                          
#if     DEVELOPMENT || DEBUG
	int             nx_enabled;
#endif
	ledger_t        ledger;                                            
	struct pmap_statistics  stats;                      
	uint64_t        corrected_compressed_ptes_count;
#if MACH_ASSERT
	boolean_t       pmap_stats_assert;
	int             pmap_pid;
	char            pmap_procname[17];
#endif                  
};

static inline boolean_t
is_ept_pmap(pmap_t p)
{
	if (__probable(p->pm_cr3 != 0)) {
		assert(p->pm_eptp == 0);
		return FALSE;
	}

	assert(p->pm_eptp != 0);

	return TRUE;
}

void hv_ept_pmap_create(void **ept_pmap, void **eptp);

typedef struct pmap_memory_regions {
	ppnum_t base;                                           
	ppnum_t alloc_up;                                                     
	ppnum_t alloc_down;                                                   
	ppnum_t alloc_frag_up;                                                    
	ppnum_t alloc_frag_down;                                                   
	ppnum_t end;                                           
	uint32_t type;
	uint64_t attribute;
} pmap_memory_region_t;

extern unsigned pmap_memory_region_count;
extern unsigned pmap_memory_region_current;

#define PMAP_MEMORY_REGIONS_SIZE 128

extern pmap_memory_region_t pmap_memory_regions[];
#include <i386/pmap_pcid.h>

static inline void
set_dirbase(pmap_t tpmap, thread_t thread, int my_cpu)
{
	int ccpu = my_cpu;
	uint64_t pcr3 = tpmap->pm_cr3, ucr3 = tpmap->pm_ucr3;
	cpu_datap(ccpu)->cpu_task_cr3 = pcr3;
	cpu_shadowp(ccpu)->cpu_shadowtask_cr3 = pcr3;

	cpu_datap(ccpu)->cpu_ucr3 = ucr3;
	cpu_shadowp(ccpu)->cpu_ucr3 = ucr3;

	cpu_datap(ccpu)->cpu_task_map = cpu_shadowp(ccpu)->cpu_task_map =
	    tpmap->pm_task_map;

	assert((get_preemption_level() > 0) || (ml_get_interrupts_enabled() == FALSE));
	assert(ccpu == cpu_number());
	  
                           
                                                      
                                                                 
    
	boolean_t nopagezero = tpmap->pagezero_accessible;
	boolean_t priorpagezero = cpu_datap(ccpu)->cpu_pagezero_mapped;
	cpu_datap(ccpu)->cpu_pagezero_mapped = nopagezero;

	if (__probable(!no_shared_cr3)) {
		if (__improbable(nopagezero)) {
			boolean_t copyio_active = ((thread->machine.specFlags & CopyIOActive) != 0);
			if (pmap_pcid_ncpus) {
				pmap_pcid_activate(tpmap, ccpu, TRUE, copyio_active);
			} else {
				if (copyio_active) {
					if (get_cr3_base() != tpmap->pm_cr3) {
						set_cr3_raw(tpmap->pm_cr3);
					}
				} else if (get_cr3_base() != cpu_datap(ccpu)->cpu_kernel_cr3) {
					set_cr3_raw(cpu_datap(ccpu)->cpu_kernel_cr3);
				}
			}
		} else if ((get_cr3_base() != tpmap->pm_cr3) || priorpagezero) {
			if (pmap_pcid_ncpus) {
				pmap_pcid_activate(tpmap, ccpu, FALSE, FALSE);
			} else {
				set_cr3_raw(tpmap->pm_cr3);
			}
		}
	} else {
		if (get_cr3_base() != cpu_datap(ccpu)->cpu_kernel_cr3) {
			set_cr3_raw(cpu_datap(ccpu)->cpu_kernel_cr3);
		}
	}
}

  
                                           
   

extern void             pmap_update_interrupt(void);

extern addr64_t(kvtophys)(
	vm_offset_t     addr);

extern kern_return_t    pmap_expand(
	pmap_t          pmap,
	vm_map_offset_t addr,
	unsigned int options);
extern vm_offset_t      pmap_map(
	vm_offset_t     virt,
	vm_map_offset_t start,
	vm_map_offset_t end,
	vm_prot_t       prot,
	unsigned int    flags);

extern vm_offset_t      pmap_map_bd(
	vm_offset_t     virt,
	vm_map_offset_t start,
	vm_map_offset_t end,
	vm_prot_t       prot,
	unsigned int    flags);
extern void             pmap_bootstrap(
	vm_offset_t     load_start,
	boolean_t       IA32e);

extern boolean_t        pmap_valid_page(
	ppnum_t pn);

extern int              pmap_list_resident_pages(
	struct pmap     *pmap,
	vm_offset_t     *listp,
	int             space);
extern void             x86_filter_TLB_coherency_interrupts(boolean_t);

extern void
pmap_mark_range(pmap_t npmap, uint64_t sv, uint64_t nxrosz, boolean_t NX,
    boolean_t ro);

  
                                                                       
   
extern  unsigned        pmap_get_cache_attributes(ppnum_t, boolean_t is_ept);

extern kern_return_t    pmap_map_block_addr(
	pmap_t pmap,
	addr64_t va,
	pmap_paddr_t pa,
	uint32_t size,
	vm_prot_t prot,
	int attr,
	unsigned int flags);
extern kern_return_t    pmap_map_block(
	pmap_t pmap,
	addr64_t va,
	ppnum_t pa,
	uint32_t size,
	vm_prot_t prot,
	int attr,
	unsigned int flags);

extern void invalidate_icache(vm_offset_t addr, unsigned cnt, int phys);
extern void flush_dcache(vm_offset_t addr, unsigned count, int phys);
extern pmap_paddr_t pmap_find_pa(pmap_t map, addr64_t va);
extern ppnum_t pmap_find_phys(pmap_t map, addr64_t va);
extern ppnum_t pmap_find_phys_nofault(pmap_t pmap, addr64_t va);

extern kern_return_t pmap_get_prot(pmap_t pmap, addr64_t va, vm_prot_t *protp);

extern void pmap_cpu_init(void);
extern void pmap_disable_NX(pmap_t pmap);

extern void pmap_pagetable_corruption_msg_log(int (*)(const char * fmt, ...)__printflike(1, 2));

extern void x86_64_protect_data_const(void);

extern uint64_t pmap_commpage_size_min(pmap_t pmap);

  
                    
   


#include <kern/spl.h>


#define PMAP_ACTIVATE_MAP(map, thread, my_cpu)  {                               \
	pmap_t		tpmap;                                  \
                                                                        \
	tpmap = vm_map_pmap(map);                                       \
	set_dirbase(tpmap, thread, my_cpu);                                     \
}

#if   defined(__x86_64__)
#define PMAP_DEACTIVATE_MAP(map, thread, ccpu)                          \
	pmap_assert2((pmap_pcid_ncpus ? (pcid_for_pmap_cpu_tuple(map->pmap, thread, ccpu) == (get_cr3_raw() & 0xFFF)) : TRUE),"PCIDs: 0x%x, active PCID: 0x%x, CR3: 0x%lx, pmap_cr3: 0x%llx, kernel_cr3: 0x%llx, kernel pmap cr3: 0x%llx, CPU active PCID: 0x%x, CPU kernel PCID: 0x%x, specflags: 0x%x, pagezero: 0x%x", pmap_pcid_ncpus, pcid_for_pmap_cpu_tuple(map->pmap, thread, ccpu), get_cr3_raw(), map->pmap->pm_cr3, cpu_datap(ccpu)->cpu_kernel_cr3, kernel_pmap->pm_cr3, cpu_datap(ccpu)->cpu_active_pcid, cpu_datap(ccpu)->cpu_kernel_pcid, thread->machine.specFlags, map->pmap->pagezero_accessible);
#else
#define PMAP_DEACTIVATE_MAP(map, thread)
#endif

#define PMAP_SWITCH_USER(th, new_map, my_cpu) {                         \
	spl_t		spl;                                            \
                                                                        \
	spl = splhigh();                                                \
	PMAP_DEACTIVATE_MAP(th->map, th, my_cpu);                       \
	th->map = new_map;                                              \
	PMAP_ACTIVATE_MAP(th->map, th, my_cpu);                         \
	splx(spl);                                                      \
}

  
                                                                         
                                                                            
                                                                         
                                  
                                                                            
   
#define CPU_CR3_MARK_INACTIVE()                                         \
	current_cpu_datap()->cpu_active_cr3 |= 1

#define CPU_CR3_MARK_ACTIVE()                                           \
	current_cpu_datap()->cpu_active_cr3 &= ~1

#define CPU_CR3_IS_ACTIVE(cpu)                                          \
	((cpu_datap(cpu)->cpu_active_cr3 & 1) == 0)

#define CPU_GET_ACTIVE_CR3(cpu)                                         \
	(cpu_datap(cpu)->cpu_active_cr3 & ~1)

#define CPU_GET_TASK_CR3(cpu)                                           \
	(cpu_datap(cpu)->cpu_task_cr3)

  
                                                         
                                                        
                                                       
                                                     
                  
   
#define MARK_CPU_IDLE(my_cpu)   {                                       \
	assert(ml_get_interrupts_enabled() == FALSE);                   \
	CPU_CR3_MARK_INACTIVE();                                        \
	mfence();                                                                       \
}

#define MARK_CPU_ACTIVE(my_cpu) {                                       \
	assert(ml_get_interrupts_enabled() == FALSE);                   \
	    
                                                          
                                                      
                                                         
                                                        
                                                        
                                                        
                                                     
                                
                                                                 \
	CPU_CR3_MARK_ACTIVE();                                          \
	mfence();                                                       \
	pmap_update_interrupt();                                        \
}

#define PMAP_CONTEXT(pmap, thread)

#define pmap_kernel_va(VA)      \
	((((vm_offset_t) (VA)) >= vm_min_kernel_address) &&     \
	 (((vm_offset_t) (VA)) <= vm_max_kernel_address))


#define pmap_compressed(pmap)           ((pmap)->stats.compressed)
#define pmap_resident_count(pmap)       ((pmap)->stats.resident_count)
#define pmap_resident_max(pmap)         ((pmap)->stats.resident_max)
#define pmap_copy(dst_pmap, src_pmap, dst_addr, len, src_addr)
#define pmap_attribute(pmap, addr, size, attr, value) \
	                                (KERN_INVALID_ADDRESS)
#define pmap_attribute_cache_sync(addr, size, attr, value) \
	                                (KERN_INVALID_ADDRESS)

#define MACHINE_PMAP_IS_EMPTY   1
extern boolean_t pmap_is_empty(pmap_t           pmap,
    vm_map_offset_t  start,
    vm_map_offset_t  end);

#define MACHINE_BOOTSTRAPPTD    1                                         

kern_return_t
    pmap_permissions_verify(pmap_t, vm_map_t, vm_offset_t, vm_offset_t);

#if DEVELOPMENT || DEBUG
extern kern_return_t pmap_test_text_corruption(pmap_paddr_t);
#endif                           

#if MACH_ASSERT
extern int pmap_stats_assert;
#define PMAP_STATS_ASSERTF(args)                \
	MACRO_BEGIN                             \
	if (pmap_stats_assert) assertf args;    \
	MACRO_END
#else                  
#define PMAP_STATS_ASSERTF(args)
#endif                  
#endif                          
#endif                 
#endif                      
#endif                      
