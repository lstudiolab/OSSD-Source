/*
 * Copyright (c) 2006-2012 Apple Inc. All rights reserved.
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

#include <pexpert/pexpert.h>
#include <i386/cpuid.h>
#include <i386/cpu_data.h>
#include <i386/mp.h>
#include <i386/proc_reg.h>
#include <i386/vmx.h>
#include <i386/vmx/vmx_asm.h>
#include <i386/vmx/vmx_shims.h>
#include <i386/vmx/vmx_cpu.h>
#include <mach/mach_host.h>                                  

#define VMX_KPRINTF(x...)                         

int vmx_use_count = 0;
boolean_t vmx_exclusive = FALSE;

static LCK_GRP_DECLARE(vmx_lck_grp, "vmx");
static LCK_MTX_DECLARE(vmx_lck_mtx, &vmx_lck_grp);

                                                                                
                     
                                                  
                                                                                
static inline boolean_t
vmx_is_available(void)
{
	return 0 != (cpuid_features() & CPUID_FEATURE_VMX);
}

                                                                                
                     
                                                     
                                                                                
static inline boolean_t
vmxon_is_enabled(void)
{
	return vmx_is_available() &&
	       (rdmsr64(MSR_IA32_FEATURE_CONTROL) & MSR_IA32_FEATCTL_VMXON);
}

#if MACH_ASSERT
                                                                                
                     
                                                     
                                                                                
static inline boolean_t
vmx_is_cr0_valid(vmx_specs_t *specs)
{
	uintptr_t cr0 = get_cr0();
	return 0 == ((~cr0 & specs->cr0_fixed_0) | (cr0 & ~specs->cr0_fixed_1));
}

                                                                                
                     
                                                     
                                                                                
static inline boolean_t
vmx_is_cr4_valid(vmx_specs_t *specs)
{
	uintptr_t cr4 = get_cr4();
	return 0 == ((~cr4 & specs->cr4_fixed_0) | (cr4 & ~specs->cr4_fixed_1));
}

#endif

static void
vmx_enable(void)
{
	uint64_t msr_image;

	if (!vmx_is_available()) {
		return;
	}

	  
                                                               
                                                                
    
	msr_image = rdmsr64(MSR_IA32_FEATURE_CONTROL);
	if (0 == ((msr_image & MSR_IA32_FEATCTL_LOCK))) {
		wrmsr64(MSR_IA32_FEATURE_CONTROL,
		    (msr_image |
		    MSR_IA32_FEATCTL_VMXON |
		    MSR_IA32_FEATCTL_LOCK));
	}

	set_cr4(get_cr4() | CR4_VMXE);
}

                                                                                
                  
                                                           
                                                                             
                                                                         
                                                       
                                                                                
void
vmx_cpu_init()
{
	vmx_specs_t *specs = &current_cpu_datap()->cpu_vmx.specs;

	vmx_enable();

	VMX_KPRINTF("[%d]vmx_cpu_init() initialized: %d\n",
	    cpu_number(), specs->initialized);

	                                                                        
	if (specs->initialized) {
		return;
	} else {
		specs->initialized = TRUE;
	}

	                                                
	specs->vmx_present = vmx_is_available() && vmxon_is_enabled();
	VMX_KPRINTF("[%d]vmx_cpu_init() vmx_present: %d\n",
	    cpu_number(), specs->vmx_present);
	if (!specs->vmx_present) {
		return;
	}

#define rdmsr_mask(msr, mask) (uint32_t)(rdmsr64(msr) & (mask))
	specs->vmcs_id = rdmsr_mask(MSR_IA32_VMX_BASIC, VMX_VCR_VMCS_REV_ID);

	                                  
	specs->cr0_fixed_0 = rdmsr_mask(MSR_IA32_VMX_CR0_FIXED0, 0xFFFFFFFF);
	specs->cr0_fixed_1 = rdmsr_mask(MSR_IA32_VMX_CR0_FIXED1, 0xFFFFFFFF);

	                                  
	specs->cr4_fixed_0 = rdmsr_mask(MSR_IA32_VMX_CR4_FIXED0, 0xFFFFFFFF);
	specs->cr4_fixed_1 = rdmsr_mask(MSR_IA32_VMX_CR4_FIXED1, 0xFFFFFFFF);
}

                                                                                
           
                                             
                                                                                
static void
vmx_on(void *arg __unused)
{
	vmx_cpu_t *cpu = &current_cpu_datap()->cpu_vmx;
	addr64_t vmxon_region_paddr;
	int result;

	VMX_KPRINTF("[%d]vmx_on() entry state: %d\n",
	    cpu_number(), cpu->specs.vmx_on);

	assert(cpu->specs.vmx_present);

	if (NULL == cpu->vmxon_region) {
		panic("vmx_on: VMXON region not allocated");
	}
	vmxon_region_paddr = vmx_paddr(cpu->vmxon_region);

	  
                         
    
	if (FALSE == cpu->specs.vmx_on) {
		assert(vmx_is_cr0_valid(&cpu->specs));
		assert(vmx_is_cr4_valid(&cpu->specs));

		result = __vmxon(vmxon_region_paddr);

		if (result != VMX_SUCCEED) {
			panic("vmx_on: unexpected return %d from __vmxon()", result);
		}

		cpu->specs.vmx_on = TRUE;
	}
	VMX_KPRINTF("[%d]vmx_on() return state: %d\n",
	    cpu_number(), cpu->specs.vmx_on);
}

                                                                                
            
                                             
                                                                                
static void
vmx_off(void *arg __unused)
{
	vmx_cpu_t *cpu = &current_cpu_datap()->cpu_vmx;
	int result;

	VMX_KPRINTF("[%d]vmx_off() entry state: %d\n",
	    cpu_number(), cpu->specs.vmx_on);

	if (TRUE == cpu->specs.vmx_on) {
		                                              
		result = __vmxoff();

		if (result != VMX_SUCCEED) {
			panic("vmx_off: unexpected return %d from __vmxoff()", result);
		}

		cpu->specs.vmx_on = FALSE;
	}

	VMX_KPRINTF("[%d]vmx_off() return state: %d\n",
	    cpu_number(), cpu->specs.vmx_on);
}

                                                                                
                               
                                                            
                                                                                
static void
vmx_allocate_vmxon_regions(void)
{
	unsigned int i;

	for (i = 0; i < real_ncpus; i++) {
		vmx_cpu_t *cpu = &cpu_datap(i)->cpu_vmx;

		                                                                        
		cpu->vmxon_region = vmx_pcalloc();
		if (NULL == cpu->vmxon_region) {
			panic("vmx_allocate_vmxon_regions: unable to allocate VMXON region");
		}
		*(uint32_t*)(cpu->vmxon_region) = cpu->specs.vmcs_id;
	}
}

                                                                                
                           
                                        
                                                                                
static void
vmx_free_vmxon_regions(void)
{
	unsigned int i;

	for (i = 0; i < real_ncpus; i++) {
		vmx_cpu_t *cpu = &cpu_datap(i)->cpu_vmx;

		vmx_pfree(cpu->vmxon_region);
		cpu->vmxon_region = NULL;
	}
}

                                                                                
                           
                                                        
                                                                                
static boolean_t
vmx_globally_available(void)
{
	unsigned int i;
	unsigned int ncpus = ml_wait_max_cpus();
	boolean_t available = TRUE;

	for (i = 0; i < ncpus; i++) {
		vmx_cpu_t *cpu = &cpu_datap(i)->cpu_vmx;

		if (!cpu->specs.vmx_present) {
			available = FALSE;
		}
	}
	VMX_KPRINTF("VMX available: %d\n", available);
	return available;
}


                                                                                
                
                                         
                                                                                
int
host_vmxon(boolean_t exclusive)
{
	int error;

	assert(0 == get_preemption_level());

	if (!vmx_globally_available()) {
		return VMX_UNSUPPORTED;
	}

	lck_mtx_lock(&vmx_lck_mtx);

	if (vmx_exclusive || (exclusive && vmx_use_count)) {
		error = VMX_INUSE;
	} else {
		if (0 == vmx_use_count) {
			vmx_allocate_vmxon_regions();
			vmx_exclusive = exclusive;
			vmx_use_count = 1;
			mp_cpus_call(CPUMASK_ALL, ASYNC, vmx_on, NULL);
		} else {
			vmx_use_count++;
		}

		VMX_KPRINTF("VMX use count: %d\n", vmx_use_count);
		error = VMX_OK;
	}

	lck_mtx_unlock(&vmx_lck_mtx);

	return error;
}

                                                                                
                 
                                          
                                                                                
void
host_vmxoff()
{
	assert(0 == get_preemption_level());

	lck_mtx_lock(&vmx_lck_mtx);

	if (1 == vmx_use_count) {
		vmx_exclusive = FALSE;
		vmx_use_count = 0;
		mp_cpus_call(CPUMASK_ALL, ASYNC, vmx_off, NULL);
		vmx_free_vmxon_regions();
	} else {
		vmx_use_count--;
	}

	lck_mtx_unlock(&vmx_lck_mtx);

	VMX_KPRINTF("VMX use count: %d\n", vmx_use_count);
}

                                                                                
                
                                                       
                                       
                                                                                
void
vmx_suspend()
{
	VMX_KPRINTF("vmx_suspend\n");

	if (vmx_use_count) {
		vmx_off(NULL);
	}
}

                                                                                
                
                                                                         
                                                                                
void
vmx_resume(boolean_t is_wake_from_hibernate)
{
	VMX_KPRINTF("vmx_resume\n");

	vmx_enable();

	if (vmx_use_count == 0) {
		return;
	}

	  
                                                
                                                             
                                                           
                                                   
    
	if (is_wake_from_hibernate) {
		vmx_cpu_t *cpu = &current_cpu_datap()->cpu_vmx;
		cpu->specs.vmx_on = FALSE;
	}

	vmx_on(NULL);
}

                                                                                
                   
                                                                            
                                                                                
boolean_t
vmx_hv_support()
{
	if (!vmx_is_available()) {
		return FALSE;
	}

#define CHK(msr, shift, mask) if (!VMX_CAP(msr, shift, mask)) return FALSE;

	                                                                          
                            
	CHK(MSR_IA32_VMX_BASIC, 0, VMX_BASIC_TRUE_CTLS)
	CHK(MSR_IA32_VMX_TRUE_PROCBASED_CTLS, 32, VMX_TRUE_PROCBASED_SECONDARY_CTLS)

	                                                               
	CHK(MSR_IA32_VMX_PROCBASED_CTLS2, 32, VMX_PROCBASED_CTLS2_EPT)
	CHK(MSR_IA32_VMX_PROCBASED_CTLS2, 32, VMX_PROCBASED_CTLS2_UNRESTRICTED)

	return TRUE;
}
