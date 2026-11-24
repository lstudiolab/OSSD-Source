/*
 * Copyright (c) 2000-2010 Apple Inc. All rights reserved.
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

#include <i386/proc_reg.h>
#include <i386/cpuid.h>
#include <i386/tsc.h>
#include <vm/pmap.h>
#include <vm/vm_map.h>
#include <i386/pmap_internal.h>
#include <i386/pmap_pcid.h>

  
                                                            
                                                                              
                                                  
                                                                     
                                                                       
                                                                  
                                                                     
                
                                                                        
                                         
                                                                              
                                                                  
                                              
                                                                       
                                                                   
                                                               
                                                              
                                                                          
   

uint32_t        pmap_pcid_ncpus;
boolean_t       pmap_pcid_disabled = FALSE;
bool            invpcid_enabled = false;
static uint32_t INP_MAX = 0;
pcid_cdata_t pcid_data[MAX_CPUS] __attribute__((aligned(64)));

void
pmap_pcid_configure(void)
{
	int ccpu = cpu_number();
	uintptr_t cr4 = get_cr4();
	boolean_t pcid_present = FALSE;

	pmap_pcid_log("PCID configure invoked on CPU %d\n", ccpu);
	pmap_assert(ml_get_interrupts_enabled() == FALSE || get_preemption_level() != 0);
	pmap_assert(cpu_mode_is64bit());

	if (PE_parse_boot_argn("-pmap_pcid_disable", &pmap_pcid_disabled, sizeof(pmap_pcid_disabled))) {
		pmap_pcid_log("PMAP: PCID feature disabled\n");
		printf("PMAP: PCID feature disabled, %u\n", pmap_pcid_disabled);
		kprintf("PMAP: PCID feature disabled %u\n", pmap_pcid_disabled);
	}
	                                                 

#if     DEBUG
	if (pmap_pcid_disabled == FALSE) {
		no_shared_cr3 = FALSE;
	} else {
		no_shared_cr3 = TRUE;
	}
#else
	if (no_shared_cr3) {
		pmap_pcid_disabled = TRUE;
	}
#endif
	if (pmap_pcid_disabled || no_shared_cr3) {
		unsigned i;
		                                              
                                           
                           
     
		for (i = 0; i < real_ncpus; i++) {
			if (cpu_datap(i)) {
				cpu_datap(i)->cpu_pmap_pcid_enabled = FALSE;
			}
			pmap_pcid_ncpus = 0;
		}
		cpu_datap(ccpu)->cpu_pmap_pcid_enabled = FALSE;
		return;
	}
	                                                                     
                                                                   
    
	if ((cpuid_features() & CPUID_FEATURE_PCID)) {
		pcid_present = TRUE;
	} else {
		cpu_datap(ccpu)->cpu_pmap_pcid_enabled = FALSE;
		pmap_pcid_log("PMAP: PCID not detected CPU %d\n", ccpu);
		return;
	}
	if ((cr4 & (CR4_PCIDE | CR4_PGE)) == (CR4_PCIDE | CR4_PGE)) {
		cpu_datap(ccpu)->cpu_pmap_pcid_enabled = TRUE;
		pmap_pcid_log("PMAP: PCID already enabled %d\n", ccpu);
		return;
	}
	if (pcid_present == TRUE) {
		if (ccpu == 0) {
			if (cpuid_leaf7_features() & CPUID_LEAF7_FEATURE_INVPCID) {
				invpcid_enabled = true;
			}
		}
#if DEVELOPMENT || DEBUG
		PE_parse_boot_argn("pmap_inp_max", &INP_MAX, sizeof(INP_MAX));
#endif
		pmap_pcid_log("Pre-PCID:CR0: 0x%lx, CR3: 0x%lx, CR4(CPU %d): 0x%lx\n", get_cr0(), get_cr3_raw(), ccpu, cr4);

		if (cpu_number() >= PMAP_PCID_MAX_CPUS) {
			panic("PMAP_PCID_MAX_CPUS %d\n", cpu_number());
		}
		if ((get_cr4() & CR4_PGE) == 0) {
			set_cr4(get_cr4() | CR4_PGE);
			pmap_pcid_log("Toggled PGE ON (CPU: %d\n", ccpu);
		}
		set_cr4(get_cr4() | CR4_PCIDE);
		pmap_pcid_log("Post PCID: CR0: 0x%lx, CR3: 0x%lx, CR4(CPU %d): 0x%lx\n", get_cr0(), get_cr3_raw(), ccpu, get_cr4());
		pmap_tlbi_range(0, ~0ULL, true, 0);
		cpu_datap(ccpu)->cpu_pmap_pcid_enabled = TRUE;

		if (OSIncrementAtomic(&pmap_pcid_ncpus) == machine_info.max_cpus) {
			pmap_pcid_log("All PCIDs enabled: real_ncpus: %d, pmap_pcid_ncpus: %d\n", real_ncpus, pmap_pcid_ncpus);
		}
		cpu_datap(ccpu)->cpu_pmap_pcid_coherentp =
		    cpu_datap(ccpu)->cpu_pmap_pcid_coherentp_kernel =
		    &(kernel_pmap->pmap_pcid_coherency_vector[ccpu]);
		cpu_datap(ccpu)->cpu_pcid_data = &pcid_data[ccpu];
		cpu_datap(ccpu)->cpu_pcid_data->cpu_pcid_refcounts[0] = 1;
	}
}

void
pmap_pcid_initialize(pmap_t p)
{
	unsigned i;
	unsigned nc = sizeof(p->pmap_pcid_cpus) / sizeof(pcid_t);

	pmap_assert(nc >= real_ncpus);
	for (i = 0; i < nc; i++) {
		p->pmap_pcid_cpus[i] = PMAP_PCID_INVALID_PCID;
		                                                        
                
     
	}
}

void
pmap_pcid_initialize_kernel(pmap_t p)
{
	unsigned i;
	unsigned nc = sizeof(p->pmap_pcid_cpus) / sizeof(pcid_t);

	for (i = 0; i < nc; i++) {
		p->pmap_pcid_cpus[i] = 0;
		                                                        
                
     
	}
}

pcid_t
pmap_pcid_allocate_pcid(int ccpu)
{
	int i;
	pcid_ref_t      cur_min = 0xFF;
	uint32_t        cur_min_index = ~1;
	pcid_ref_t      *cpu_pcid_refcounts = &cpu_datap(ccpu)->cpu_pcid_data->cpu_pcid_refcounts[0];
	pcid_ref_t      old_count;

	if ((i = cpu_datap(ccpu)->cpu_pcid_data->cpu_pcid_free_hint) != 0) {
		if (cpu_pcid_refcounts[i] == 0) {
			(void)__sync_fetch_and_add(&cpu_pcid_refcounts[i], 1);
			cpu_datap(ccpu)->cpu_pcid_data->cpu_pcid_free_hint = 0;
			return i;
		}
	}
	                                                                      
                                                   
                                           
                                                      
                                                    
                                                   
                        
    
	for (i = PMAP_PCID_MIN_PCID; i < PMAP_PCID_MAX_PCID; i++) {
		pcid_ref_t cur_refcount = cpu_pcid_refcounts[i];

		pmap_assert(cur_refcount < PMAP_PCID_MAX_REFCOUNT);

		if (cur_refcount == 0) {
			(void)__sync_fetch_and_add(&cpu_pcid_refcounts[i], 1);
			return i;
		} else {
			if (cur_refcount < cur_min) {
				cur_min_index = i;
				cur_min = cur_refcount;
			}
		}
	}
	pmap_assert(cur_min_index > 0 && cur_min_index < PMAP_PCID_MAX_PCID);
	                                                                      
                                               
    

	old_count = __sync_fetch_and_add(&cpu_pcid_refcounts[cur_min_index], 1);
	pmap_assert(old_count < PMAP_PCID_MAX_REFCOUNT);
	return cur_min_index;
}

void
pmap_pcid_deallocate_pcid(int ccpu, pmap_t tpmap)
{
	pcid_t pcid;
	pmap_t lp;
	pcid_ref_t prior_count;

	pcid = tpmap->pmap_pcid_cpus[ccpu];
	pmap_assert(pcid != PMAP_PCID_INVALID_PCID);
	if (pcid == PMAP_PCID_INVALID_PCID) {
		return;
	}

	lp = cpu_datap(ccpu)->cpu_pcid_data->cpu_pcid_last_pmap_dispatched[pcid];
	pmap_assert(pcid > 0 && pcid < PMAP_PCID_MAX_PCID);
	pmap_assert(cpu_datap(ccpu)->cpu_pcid_data->cpu_pcid_refcounts[pcid] >= 1);

	if (lp == tpmap) {
		(void)__sync_bool_compare_and_swap(&cpu_datap(ccpu)->cpu_pcid_data->cpu_pcid_last_pmap_dispatched[pcid], tpmap, PMAP_INVALID);
	}

	if ((prior_count = __sync_fetch_and_sub(&cpu_datap(ccpu)->cpu_pcid_data->cpu_pcid_refcounts[pcid], 1)) == 1) {
		cpu_datap(ccpu)->cpu_pcid_data->cpu_pcid_free_hint = pcid;
	}
	pmap_assert(prior_count <= PMAP_PCID_MAX_REFCOUNT);
}

void
pmap_destroy_pcid_sync(pmap_t p)
{
	int i;
	pmap_assert(ml_get_interrupts_enabled() == FALSE || get_preemption_level() != 0);
	for (i = 0; i < PMAP_PCID_MAX_CPUS; i++) {
		if (p->pmap_pcid_cpus[i] != PMAP_PCID_INVALID_PCID) {
			pmap_pcid_deallocate_pcid(i, p);
		}
	}
}

pcid_t
pcid_for_pmap_cpu_tuple(pmap_t cpmap, thread_t cthread, int ccpu)
{
	pmap_t active_pmap = cpmap;

	if (__improbable(cpmap->pagezero_accessible)) {
		if ((cthread->machine.specFlags & CopyIOActive) == 0) {
			active_pmap = kernel_pmap;
		}
	}

	return active_pmap->pmap_pcid_cpus[ccpu];
}
int npz = 0;

#if PMAP_ASSERT
#define PCID_RECORD_SIZE 128
uint64_t pcid_record_array[PCID_RECORD_SIZE];
#endif
#define PMAP_UPCIDP(p) ((p ? (p + PMAP_PCID_MAX_PCID) : 0) | 1ULL << 63)

void
pmap_pcid_activate(pmap_t tpmap, int ccpu, boolean_t nopagezero, boolean_t copyio)
{
	pcid_t          new_pcid = tpmap->pmap_pcid_cpus[ccpu];
	pmap_t          last_pmap;
	boolean_t       pcid_conflict = FALSE, pending_flush = FALSE;
	pcid_cdata_t    *pcdata = cpu_datap(ccpu)->cpu_pcid_data;

	pmap_assert(cpu_datap(ccpu)->cpu_pmap_pcid_enabled);
	if (__improbable(new_pcid == PMAP_PCID_INVALID_PCID)) {
		new_pcid = tpmap->pmap_pcid_cpus[ccpu] = pmap_pcid_allocate_pcid(ccpu);
	}

	pmap_assert(new_pcid != PMAP_PCID_INVALID_PCID);
#ifdef  PCID_ASSERT
	cpu_datap(ccpu)->cpu_last_pcid = cpu_datap(ccpu)->cpu_active_pcid;
#endif
	cpu_datap(ccpu)->cpu_active_pcid = new_pcid;

	pending_flush = (tpmap->pmap_pcid_coherency_vector[ccpu] != 0);
	if (__probable(pending_flush == FALSE)) {
		last_pmap = pcdata->cpu_pcid_last_pmap_dispatched[new_pcid];
		pcid_conflict = ((last_pmap != NULL) && (tpmap != last_pmap));
	}
	if (__improbable(pending_flush || pcid_conflict)) {
		pmap_pcid_validate_cpu(tpmap, ccpu);
	}
	                                      
	pcdata->cpu_pcid_last_pmap_dispatched[new_pcid] = tpmap;

	pmap_assert(new_pcid < PMAP_PCID_MAX_PCID);
	pmap_assert(((tpmap == kernel_pmap) && new_pcid == 0) ||
	    ((new_pcid != PMAP_PCID_INVALID_PCID) && (new_pcid != 0)));
#if     PMAP_ASSERT
	pcid_record_array[ccpu % PCID_RECORD_SIZE] = tpmap->pm_cr3 | new_pcid | (((uint64_t)(!(pending_flush || pcid_conflict))) << 63);
	pml4_entry_t *pml4 = pmap64_pml4(tpmap, 0ULL);
	                                                      
	if (pml4[KERNEL_PML4_INDEX] != kernel_pmap->pm_pml4[KERNEL_PML4_INDEX]) {
		__asm__ volatile ("int3");
	}
#endif                   

	pmap_paddr_t ncr3 = tpmap->pm_cr3;

	if (__improbable(nopagezero)) {
		pending_flush = TRUE;
		if (copyio == FALSE) {
			new_pcid = kernel_pmap->pmap_pcid_cpus[ccpu];
			ncr3 = kernel_pmap->pm_cr3;
		}
		cpu_datap(ccpu)->cpu_kernel_pcid = kernel_pmap->pmap_pcid_cpus[ccpu];
		npz++;
	}

	set_cr3_composed(ncr3, new_pcid, 1ULL);
	cpu_shadowp(ccpu)->cpu_shadowtask_cr3 = ncr3 | new_pcid | (1ULL << 63);

	bool preserve = !pcid_conflict && !pending_flush;
	if (preserve == true) {
		                                                                
                                                      
                                                      
                                                      
                                                        
                                                         
                                                            
                   
     
		pending_flush = (tpmap->pmap_pcid_coherency_vector[ccpu] != 0);
		if (__improbable(pending_flush != 0)) {
			pmap_pcid_validate_cpu(tpmap, ccpu);
			preserve = false;
		}
	}

	if (preserve == false) {
		bool gtlbi = (invpcid_enabled == false);
		pmap_tlbi_range(0, ~0ULL, gtlbi, new_pcid);
	}

	uint64_t spcid = PMAP_UPCIDP(new_pcid);
	uint64_t scr3 = tpmap->pm_ucr3 | spcid;

	cpu_datap(ccpu)->cpu_ucr3 = scr3;
	cpu_shadowp(ccpu)->cpu_ucr3 = scr3;

	cpu_datap(ccpu)->cpu_pmap_pcid_coherentp = &(tpmap->pmap_pcid_coherency_vector[ccpu]);
#if DEBUG
	cpu_datap(ccpu)->cpu_pcid_last_cr3 = scr3;
	KERNEL_DEBUG_CONSTANT(0x9c1d0000, tpmap, new_pcid, pending_flush, pcid_conflict, 0);
#endif
}

typedef enum {
	INP_ALLG = 2, INP_ASPACE = 1, INP_SINGLE = 0, INP_ALLNG = 3
} invpcid_type_t;
typedef struct __attribute__((packed)) {
	uint64_t ipcid_and_rsvd;
	uint64_t iaddr;
} invpcid_desc_t;

static inline void
invpcid(invpcid_type_t itype, pcid_t ipcid, uint64_t iaddr)
{
	invpcid_desc_t ipcdt;

	ipcdt.ipcid_and_rsvd = ipcid;
	ipcdt.iaddr = iaddr;

	uint64_t iptype = itype;                                      

	__asm__ volatile ("invpcid %0, %1" :: "m" (ipcdt), "r" (iptype) : "memory");
}


void
pmap_tlbi_range(uint64_t startv, uint64_t endv, bool global, uint16_t pcid)
{
	assert(ml_get_interrupts_enabled() == FALSE ||
	    get_preemption_level() != 0);

	if (invpcid_enabled) {
		if (global) {
			invpcid(INP_ALLG, 0, 0ULL);
		} else {
			                                                  
			if ((endv - startv) >= INP_MAX) {
				invpcid(INP_ASPACE, pcid, 0ULL);
				if (pcid) {
					invpcid(INP_ASPACE, (pcid + PMAP_PCID_MAX_PCID), 0ULL);
				}
			} else {
				uint64_t cv = startv;
				for (; cv < endv; cv += PAGE_SIZE) {
					invpcid(INP_SINGLE, pcid, cv);
					if (pcid) {
						invpcid(INP_SINGLE, (pcid + PMAP_PCID_MAX_PCID), cv);
					}
				}
			}
		}
	} else {
		if (pmap_pcid_ncpus) {
			uintptr_t cr4 = get_cr4();
			if (__improbable((cr4 & CR4_PGE) == 0)) {
				set_cr4(cr4 | CR4_PGE);
			} else {
				set_cr4(cr4 & ~CR4_PGE);
				set_cr4(cr4 | CR4_PGE);
			}
		} else {
			set_cr3_raw(get_cr3_raw());
		}
	}
	__c11_atomic_thread_fence(__ATOMIC_SEQ_CST);
}
