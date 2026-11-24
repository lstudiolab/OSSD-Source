/*
 * Copyright (c) 2005-2006 Apple Computer, Inc. All rights reserved.
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

#include <string.h>
#include <mach/vm_param.h>
#include <mach/vm_prot.h>
#include <mach/machine.h>
#include <mach/time_value.h>
#include <kern/spl.h>
#include <kern/assert.h>
#include <kern/debug.h>
#include <kern/misc_protos.h>
#include <kern/startup.h>
#include <kern/clock.h>
#include <kern/cpu_data.h>
#include <kern/processor.h>
#include <vm/vm_page.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <i386/cpuid.h>
#include <i386/machine_cpu.h>
#include <i386/mp.h>
#include <i386/machine_routines.h>
#include <i386/pmap.h>
#include <i386/misc_protos.h>
#include <i386/io_map_entries.h>
#include <architecture/i386/pio.h>
#include <i386/cpuid.h>
#include <i386/apic.h>
#include <i386/tsc.h>
#include <i386/hpet.h>
#include <i386/pmCPU.h>
#include <i386/cpu_topology.h>
#include <i386/cpu_threads.h>
#include <pexpert/device_tree.h>

                     
#define kilo (1000ULL)
#define Mega (kilo * kilo)
#define Giga (kilo * Mega)
#define Tera (kilo * Giga)
#define Peta (kilo * Tera)

vm_offset_t hpetArea = 0;
uint32_t hpetAreap = 0;
uint64_t hpetFemto = 0;
uint64_t hpetFreq = 0;
uint64_t hpetCvt = 0;                                          
uint64_t hpetCvtt2n = 0;
uint64_t hpetCvtn2t = 0;
uint64_t tsc2hpet = 0;
uint64_t hpet2tsc = 0;
uint64_t bus2hpet = 0;
uint64_t hpet2bus = 0;

vm_offset_t rcbaArea = 0;
uint32_t rcbaAreap = 0;

static int (*hpet_req)(uint32_t apicid, void *arg, hpetRequest_t *hpet) = NULL;
static void *hpet_arg = NULL;

#if DEBUG
#define DBG(x...)       kprintf("DBG: " x)
#else
#define DBG(x...)
#endif

int
hpet_register_callback(int (*hpet_reqst)(uint32_t apicid,
    void *arg,
    hpetRequest_t *hpet),
    void *arg)
{
	hpet_req = hpet_reqst;
	hpet_arg = arg;
	return 0;
}

  
                                                                
                                                                  
                   
   
int
hpet_request(uint32_t cpu)
{
	hpetRequest_t       hpetReq;
	int                 rc;
	x86_lcpu_t          *lcpu;
	x86_core_t          *core;
	x86_pkg_t           *pkg;
	boolean_t           enabled;

	if (hpet_req == NULL) {
		return -1;
	}

	  
                                                            
                                           
    
	if (cpu >= real_ncpus) {
		enabled = ml_set_interrupts_enabled(FALSE);
		lcpu = cpu_to_lcpu(cpu);
		if (lcpu != NULL) {
			core = lcpu->core;
			pkg  = core->package;

			if (lcpu->primary) {
				pkg->flags |= X86PKG_FL_HAS_HPET;
			}
		}

		ml_set_interrupts_enabled(enabled);
		return 0;
	}

	rc = (*hpet_req)(ml_get_apicid(cpu), hpet_arg, &hpetReq);
	if (rc != 0) {
		return rc;
	}

	enabled = ml_set_interrupts_enabled(FALSE);
	lcpu = cpu_to_lcpu(cpu);
	core = lcpu->core;
	pkg  = core->package;

	  
                                    
    
	core->Hpet = (hpetTimer_t *)((uint8_t *)hpetArea + hpetReq.hpetOffset);
	core->HpetVec = hpetReq.hpetVector;

	  
                     
    
	core->Hpet->Config |= Tn_INT_ENB_CNF;

	  
                          
    
	core->HpetCfg = core->Hpet->Config;
	core->HpetCmp = 0;

	  
                                                     
                                    
    
	if (lcpu->primary) {
		pkg->Hpet = core->Hpet;
		pkg->HpetCfg = core->HpetCfg;
		pkg->HpetCmp = core->HpetCmp;
		pkg->flags |= X86PKG_FL_HAS_HPET;
	}

	ml_set_interrupts_enabled(enabled);

	return 0;
}

  
                     
   
static void
map_rcbaArea(void)
{
	  
                                             
    
	outl(cfgAdr, lpcCfg | (0xF0 & 0xFC));
	rcbaAreap = inl(cfgDat | (0xF0 & 0x03));
	rcbaArea = io_map_spec(rcbaAreap & -4096, PAGE_SIZE * 4, VM_WIMG_IO);
	kprintf("RCBA: vaddr = %lX, paddr = %08X\n", (unsigned long)rcbaArea, rcbaAreap);
}

  
                      
   
void
hpet_init(void)
{
	unsigned int    *xmod;

	map_rcbaArea();

	  
                                       
                                   
    
	xmod = (uint32_t *)(rcbaArea + 0x3404);                        
	uint32_t hptc = *xmod;                                       
	DBG("    current RCBA.HPTC:  %08X\n", *xmod);
	if (!(hptc & hptcAE)) {
		DBG("HPET memory is not enabled, "
		    "enabling and assigning to 0xFED00000 (hope that's ok)\n");
		*xmod = (hptc & ~3) | hptcAE;
	}

	  
                                            
    
	hpetAreap = hpetAddr | ((hptc & 3) << 12);
	hpetArea = io_map_spec(hpetAreap & -4096, PAGE_SIZE * 4, VM_WIMG_IO);
	kprintf("HPET: vaddr = %lX, paddr = %08X\n", (unsigned long)hpetArea, hpetAreap);

	  
                               
                                                                 
                                      
    
	hpetFemto = (uint32_t)(((hpetReg_t *)hpetArea)->GCAP_ID >> 32);
	hpetFreq = (1 * Peta) / hpetFemto;

	  
                                                                    
                                                                
                                                                    
                                                                        
                                                                 
                                
    

	  
                                                                  
                                                                        
                                           
    
	hpetCvtt2n = (uint64_t)hpetFemto << 32;
	hpetCvtt2n = hpetCvtt2n / 1000000ULL;
	hpetCvtn2t = 0xFFFFFFFFFFFFFFFFULL / hpetCvtt2n;
	kprintf("HPET: Frequency = %6d.%04dMHz, "
	    "cvtt2n = %08X.%08X, cvtn2t = %08X.%08X\n",
	    (uint32_t)(hpetFreq / Mega), (uint32_t)(hpetFreq % Mega),
	    (uint32_t)(hpetCvtt2n >> 32), (uint32_t)hpetCvtt2n,
	    (uint32_t)(hpetCvtn2t >> 32), (uint32_t)hpetCvtn2t);


	                   
                                                      
                                                                
    
	hpetCvt = (uint64_t)hpetFemto << 20;
	hpetCvt = hpetCvt / 1000000ULL;

	                                           
	tsc2hpet = tmrCvt(tscFCvtt2n, hpetCvtn2t);
	DBG(" CVT: TSC to HPET = %08X.%08X\n",
	    (uint32_t)(tsc2hpet >> 32), (uint32_t)tsc2hpet);

	                                           
	hpet2tsc = tmrCvt(hpetCvtt2n, tscFCvtn2t);
	DBG(" CVT: HPET to TSC = %08X.%08X\n",
	    (uint32_t)(hpet2tsc >> 32), (uint32_t)hpet2tsc);

	                                           
	bus2hpet = tmrCvt(busFCvtt2n, hpetCvtn2t);
	DBG(" CVT: BUS to HPET = %08X.%08X\n",
	    (uint32_t)(bus2hpet >> 32), (uint32_t)bus2hpet);

	                                           
	hpet2bus = tmrCvt(hpetCvtt2n, busFCvtn2t);
	DBG(" CVT: HPET to BUS = %08X.%08X\n",
	    (uint32_t)(hpet2bus >> 32), (uint32_t)hpet2bus);
}

  
                                                                 
                                                                
                           
   
void
hpet_get_info(hpetInfo_t *info)
{
	info->hpetCvtt2n = hpetCvtt2n;
	info->hpetCvtn2t = hpetCvtn2t;
	info->tsc2hpet   = tsc2hpet;
	info->hpet2tsc   = hpet2tsc;
	info->bus2hpet   = bus2hpet;
	info->hpet2bus   = hpet2bus;
	  
       
                                                          
                                           
    
	info->rcbaArea   = hpetArea;
	info->rcbaAreap  = hpetAreap;
}


  
                                            
                                                
  
                                                  
                                   
   
void
ml_hpet_cfg(uint32_t cpu, uint32_t hpetVect)
{
	uint64_t        *hpetVaddr;
	hpetTimer_t     *hpet;
	x86_lcpu_t      *lcpu;
	x86_core_t      *core;
	x86_pkg_t       *pkg;
	boolean_t       enabled;

	if (cpu > 1) {
		panic("ml_hpet_cfg: invalid cpu = %d\n", cpu);
	}

	lcpu = cpu_to_lcpu(cpu);
	core = lcpu->core;
	pkg  = core->package;

	  
                                                   
    
	if (!lcpu->primary) {
		return;
	}

	enabled = ml_set_interrupts_enabled(FALSE);

	                                                      
	hpetVaddr = (uint64_t *)(((uintptr_t)&(((hpetReg_t *)hpetArea)->TIM1_CONF)) + (cpu << 5));
	hpet = (hpetTimer_t *)hpetVaddr;

	DBG("ml_hpet_cfg: HPET for cpu %d at %p, vector = %d\n",
	    cpu, hpetVaddr, hpetVect);

	                                                                
	core->Hpet = hpet;
	core->HpetVec = hpetVect;

	  
                     
    
	core->Hpet->Config |= Tn_INT_ENB_CNF;

	                            
	core->HpetCfg = core->Hpet->Config;
	core->HpetCmp = 0;

	  
                                                    
                                              
    
	pkg->Hpet = core->Hpet;
	pkg->HpetVec = core->HpetVec;
	pkg->HpetCfg = core->HpetCfg;
	pkg->HpetCmp = core->HpetCmp;
	pkg->flags |= X86PKG_FL_HAS_HPET;

	ml_set_interrupts_enabled(enabled);
}

  
                                      
  
                                                             
                                     
   
int
HPETInterrupt(void)
{
	                                         
	x86_package()->HpetInt++;

	  
                                       
    
	pmHPETInterrupt();

	                                                        
	return 1;
}


static hpetReg_t saved_hpet;

void
hpet_save(void)
{
	hpetReg_t       *from = (hpetReg_t *) hpetArea;
	hpetReg_t       *to = &saved_hpet;

	to->GEN_CONF  = from->GEN_CONF;
	to->TIM0_CONF = from->TIM0_CONF;
	to->TIM0_COMP = from->TIM0_COMP;
	to->TIM1_CONF = from->TIM1_CONF;
	to->TIM1_COMP = from->TIM1_COMP;
	to->TIM2_CONF = from->TIM2_CONF;
	to->TIM2_COMP = from->TIM2_COMP;
	to->MAIN_CNT  = from->MAIN_CNT;
}

void
hpet_restore(void)
{
	hpetReg_t       *from = &saved_hpet;
	hpetReg_t       *to = (hpetReg_t *) hpetArea;

	  
                                       
                                   
    
	uint32_t *hptcp = (uint32_t *)(rcbaArea + 0x3404);
	uint32_t hptc = *hptcp;
	if (!(hptc & hptcAE)) {
		DBG("HPET memory is not enabled, "
		    "enabling and assigning to 0xFED00000 (hope that's ok)\n");
		*hptcp = (hptc & ~3) | hptcAE;
	}

	to->GEN_CONF  = from->GEN_CONF & ~1;

	to->TIM0_CONF = from->TIM0_CONF;
	to->TIM0_COMP = from->TIM0_COMP;
	to->TIM1_CONF = from->TIM1_CONF;
	to->TIM1_COMP = from->TIM1_COMP;
	to->TIM2_CONF = from->TIM2_CONF;
	to->TIM2_COMP = from->TIM2_COMP;
	to->GINTR_STA = -1ULL;
	to->MAIN_CNT  = from->MAIN_CNT;

	to->GEN_CONF = from->GEN_CONF;
}

  
                           
  
   
uint64_t
rdHPET(void)
{
	hpetReg_t               *hpetp = (hpetReg_t *) hpetArea;
	volatile uint32_t       *regp = (uint32_t *) &hpetp->MAIN_CNT;
	uint32_t                high;
	uint32_t                low;

	do {
		high = *(regp + 1);
		low = *regp;
	} while (high != *(regp + 1));

	return (((uint64_t) high) << 32) | low;
}
