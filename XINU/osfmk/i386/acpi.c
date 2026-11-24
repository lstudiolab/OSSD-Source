/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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

#include <i386/pmap.h>
#include <i386/proc_reg.h>
#include <i386/mp_desc.h>
#include <i386/misc_protos.h>
#include <i386/mp.h>
#include <i386/cpu_data.h>
#if CONFIG_MTRR
#include <i386/mtrr.h>
#endif
#if HYPERVISOR
#include <kern/hv_support.h>
#endif
#if CONFIG_VMX
#include <i386/vmx/vmx_cpu.h>
#endif
#include <i386/ucode.h>
#include <i386/acpi.h>
#include <i386/fpu.h>
#include <i386/lapic.h>
#include <i386/mp.h>
#include <i386/mp_desc.h>
#include <i386/serial_io.h>
#if CONFIG_MCA
#include <i386/machine_check.h>
#endif
#include <i386/pmCPU.h>

#include <i386/tsc.h>

#define UINT64 uint64_t
#define UINT32 uint32_t
#define UINT16 uint16_t
#define UINT8 uint8_t
#define RSDP_VERSION_ACPI10     0
#define RSDP_VERSION_ACPI20     2
#include <acpi/Acpi.h>
#include <acpi/Acpi_v1.h>
#include <pexpert/i386/efi.h>

#include <kern/cpu_data.h>
#include <kern/machine.h>
#include <kern/timer_queue.h>
#include <console/serial_protos.h>
#include <machine/pal_routines.h>
#include <vm/vm_page.h>

#if HIBERNATION
#include <IOKit/IOHibernatePrivate.h>
#endif
#include <IOKit/IOPlatformExpert.h>
#include <sys/kdebug.h>

#if MONOTONIC
#include <kern/monotonic.h>
#endif                

#if CONFIG_SLEEP
extern void     acpi_sleep_cpu(acpi_sleep_callback, void * refcon);
extern void     acpi_wake_prot(void);
#endif
extern kern_return_t IOCPURunPlatformQuiesceActions(void);
extern kern_return_t IOCPURunPlatformActiveActions(void);
extern kern_return_t IOCPURunPlatformHaltRestartActions(uint32_t message);

extern void     fpinit(void);

#if DEVELOPMENT || DEBUG
#define DBG(x...) kprintf(x)
#else
#define DBG(x...)
#endif

vm_offset_t
acpi_install_wake_handler(void)
{
#if CONFIG_SLEEP
	install_real_mode_bootstrap(acpi_wake_prot);
	return REAL_MODE_BOOTSTRAP_OFFSET;
#else
	return 0;
#endif
}

#if CONFIG_SLEEP

unsigned int            save_kdebug_enable = 0;
static uint64_t         acpi_sleep_abstime;
static uint64_t         acpi_idle_abstime;
static uint64_t         acpi_wake_abstime, acpi_wake_postrebase_abstime;
boolean_t               deep_idle_rebase = TRUE;

#if HIBERNATION
struct acpi_hibernate_callback_data {
	acpi_sleep_callback func;
	void *refcon;
};
typedef struct acpi_hibernate_callback_data acpi_hibernate_callback_data_t;

static void
acpi_hibernate(void *refcon)
{
	uint32_t mode;

	acpi_hibernate_callback_data_t *data =
	    (acpi_hibernate_callback_data_t *)refcon;

	if (current_cpu_datap()->cpu_hibernate) {
		mode = hibernate_write_image();

		if (mode == kIOHibernatePostWriteHalt) {
			      
			HIBLOG("power off\n");
			IOCPURunPlatformHaltRestartActions(kPEHaltCPU);
			if (PE_halt_restart) {
				(*PE_halt_restart)(kPEHaltCPU);
			}
		} else if (mode == kIOHibernatePostWriteRestart) {
			          
			HIBLOG("restart\n");
			IOCPURunPlatformHaltRestartActions(kPERestartCPU);
			if (PE_halt_restart) {
				(*PE_halt_restart)(kPERestartCPU);
			}
		} else {
			        
			HIBLOG("sleep\n");

			                                                                 
			cpu_datap(0)->cpu_hibernate = 0;
		}
	}

#if CONFIG_VMX
	vmx_suspend();
#endif
	kdebug_enable = 0;

	IOCPURunPlatformQuiesceActions();

	acpi_sleep_abstime = mach_absolute_time();

	(data->func)(data->refcon);

	                            
}
#endif                  
#endif                   

extern void                     slave_pstart(void);

void
acpi_sleep_kernel(acpi_sleep_callback func, void *refcon)
{
#if HIBERNATION
	acpi_hibernate_callback_data_t data;
#endif
	boolean_t did_hibernate;
	cpu_data_t *cdp = current_cpu_datap();
	unsigned int    cpu;
	kern_return_t   rc;
	unsigned int    my_cpu;
	uint64_t        start;
	uint64_t        elapsed = 0;
	uint64_t        elapsed_trace_start = 0;

	my_cpu = cpu_number();
	kprintf("acpi_sleep_kernel hib=%d, cpu=%d\n", cdp->cpu_hibernate,
	    my_cpu);

	                                           
	for (cpu = 0; cpu < real_ncpus; cpu += 1) {
		if (cpu == my_cpu) {
			continue;
		}
		rc = pmCPUExitHaltToOff(cpu);
		if (rc != KERN_SUCCESS) {
			panic("Error %d trying to transition CPU %d to OFF",
			    rc, cpu);
		}
	}

	                                                            
	lapic_shutdown(true);

#if HIBERNATION
	data.func = func;
	data.refcon = refcon;
#endif

#if MONOTONIC
	mt_cpu_down(cdp);
#endif                

	                                       
	pmTimerSave();

#if HYPERVISOR
	                                                  
	hv_suspend();
#endif

	  
                                                             
    
	clear_ts();

	KDBG(IOKDBG_CODE(DBG_HIBERNATE, 0) | DBG_FUNC_START);

	save_kdebug_enable = kdebug_enable;
	kdebug_enable = 0;

	acpi_sleep_abstime = mach_absolute_time();

#if CONFIG_SLEEP
	  
                                             
                                               
                       
    
	uint64_t old_cr3 = x86_64_pre_sleep();
#if HIBERNATION
	acpi_sleep_cpu(acpi_hibernate, &data);
#else
#if CONFIG_VMX
	vmx_suspend();
#endif
	acpi_sleep_cpu(func, refcon);
#endif

	acpi_wake_abstime = mach_absolute_time();
	                                                        
                          
    
	rtc_nanotime_init(acpi_sleep_abstime);
	acpi_wake_postrebase_abstime = start = mach_absolute_time();
	assert(start >= acpi_sleep_abstime);

	x86_64_post_sleep(old_cr3);

#endif                   

	                                    
                                                                
                                            
    

	if (FALSE == disable_serial_output) {
		pal_serial_init();
	}

#if HIBERNATION
	if (current_cpu_datap()->cpu_hibernate) {
		did_hibernate = TRUE;
	} else
#endif
	{
		did_hibernate = FALSE;
	}

	                            
	cpu_syscall_init(current_cpu_datap());

#if CONFIG_MCA
	                                      
	mca_cpu_init();
#endif

#if CONFIG_MTRR
	                           
	mtrr_update_cpu();
#endif

	                                                    
	ucode_update_wake_and_apply_cpu_was();

#if CONFIG_MTRR
	                                                  
	pat_init();
#endif

#if CONFIG_VMX
	  
                   
    
	vmx_resume(did_hibernate);
#endif

	  
                                                         
                   
    
	pmMarkAllCPUsOff();


	                                                                 
	if (lapic_probe()) {
		lapic_configure(true);
	}

#if KASAN
	  
                                                                           
                                  
    
	kasan_unpoison_curstack(true);
#endif

	elapsed += mach_absolute_time() - start;

	rtc_decrementer_configure();
	kdebug_enable = save_kdebug_enable;

	if (kdebug_enable == 0) {
		elapsed_trace_start += kdebug_wake();
	}
	start = mach_absolute_time();

	                              
	init_fpu();
	clear_ts();


#if HYPERVISOR
	                                                   
	hv_resume();
#endif

	IOCPURunPlatformActiveActions();

	KDBG(IOKDBG_CODE(DBG_HIBERNATE, 0) | DBG_FUNC_END, start, elapsed,
	    elapsed_trace_start, acpi_wake_abstime);

	                                             
	pmCPUMarkRunning(current_cpu_datap());

	                                          
	pmTimerRestore();

	                              
	rtc_timer_start();

#if MONOTONIC
	mt_cpu_up(cdp);
#endif                

#if HIBERNATION
	kprintf("ret from acpi_sleep_cpu hib=%d\n", did_hibernate);
#endif                  

#if CONFIG_SLEEP
	                                                           
                                                           
                                                
	install_real_mode_bootstrap(slave_pstart);
#endif                   
}

void
ml_hibernate_active_pre(void)
{
#if HIBERNATION
	hibernate_rebuild_vm_structs();
#endif                  
}

void
ml_hibernate_active_post(void)
{
#if HIBERNATION
	if (current_cpu_datap()->cpu_hibernate) {
		KDBG(IOKDBG_CODE(DBG_HIBERNATE, 2) | DBG_FUNC_START);
		hibernate_machine_init();
		KDBG(IOKDBG_CODE(DBG_HIBERNATE, 2) | DBG_FUNC_END);
		current_cpu_datap()->cpu_hibernate = 0;
	}
#endif                  
}

  
                                                                             
                                                                            
                                                                                
  
                                                                               
                                                                              
                                                                         
                                                                      
                                                                      
                         
   
void
acpi_idle_kernel(acpi_sleep_callback func, void *refcon)
{
	boolean_t       istate = ml_get_interrupts_enabled();

	kprintf("acpi_idle_kernel, cpu=%d, interrupts %s\n",
	    cpu_number(), istate ? "enabled" : "disabled");

	assert(cpu_number() == master_cpu);

#if MONOTONIC
	mt_cpu_down(cpu_datap(0));
#endif                

	                                 
	setPop(0);
	while (lapic_is_interrupting(LAPIC_TIMER_VECTOR)
#if MONOTONIC
	    || lapic_is_interrupting(LAPIC_VECTOR(PERFCNT))
#endif                
	    ) {
		(void) ml_set_interrupts_enabled(TRUE);
		setPop(0);
		ml_set_interrupts_enabled(FALSE);
	}

	if (current_cpu_datap()->cpu_hibernate) {
		                                                                 
		hibernate_write_image();
		cpu_datap(0)->cpu_hibernate = 0;
	}

	  
                                                               
                                                 
    
	IOCPURunPlatformQuiesceActions();

	func(refcon);

	acpi_idle_abstime = mach_absolute_time();

	KERNEL_DEBUG_CONSTANT(
		MACHDBG_CODE(DBG_MACH_SCHED, MACH_DEEP_IDLE) | DBG_FUNC_START,
		acpi_idle_abstime, deep_idle_rebase, 0, 0, 0);

	  
                                   
                                                         
    
	if (deep_idle_rebase) {
		save_kdebug_enable = kdebug_enable;
		kdebug_enable = 0;
	}

	  
                                                           
                                                            
                                
    
	pmCPUHalt(PM_HALT_SLEEP);

	  
                                                             
                                                                   
                                                                  
    
	acpi_wake_abstime = mach_absolute_time();
	if (deep_idle_rebase) {
		rtc_sleep_wakeup(acpi_idle_abstime);
		kdebug_enable = save_kdebug_enable;
	}
	acpi_wake_postrebase_abstime = mach_absolute_time();
	assert(mach_absolute_time() >= acpi_idle_abstime);

	KERNEL_DEBUG_CONSTANT(
		MACHDBG_CODE(DBG_MACH_SCHED, MACH_DEEP_IDLE) | DBG_FUNC_END,
		acpi_wake_abstime, acpi_wake_abstime - acpi_idle_abstime, 0, 0, 0);

#if MONOTONIC
	mt_cpu_up(cpu_datap(0));
#endif                

	                                                                      
	if (kdebug_enable == 0) {
		kdebug_wake();
	}

	IOCPURunPlatformActiveActions();

	                              
	rtc_timer_start();
}

extern char real_mode_bootstrap_end[];
extern char real_mode_bootstrap_base[];

void
install_real_mode_bootstrap(void *prot_entry)
{
	  
                                                                                     
                                                                 
                                              
                                                                 
                                                          
    
	bcopy_phys(kvtophys((vm_offset_t) real_mode_bootstrap_base),
	    (addr64_t) REAL_MODE_BOOTSTRAP_OFFSET,
	    real_mode_bootstrap_end - real_mode_bootstrap_base);

	  
                                                             
                         
    
	ml_phys_write_word(
		PROT_MODE_START + REAL_MODE_BOOTSTRAP_OFFSET,
		(unsigned int)kvtophys((vm_offset_t)prot_entry));

	                  
	__asm__("wbinvd");
}

boolean_t
ml_recent_wake(void)
{
	uint64_t ctime = mach_absolute_time();
	assert(ctime > acpi_wake_postrebase_abstime);
	return (ctime - acpi_wake_postrebase_abstime) < 5 * NSEC_PER_SEC;
}

static uint8_t
cksum8(uint8_t *ptr, uint32_t size)
{
	uint8_t sum = 0;
	uint32_t i;

	for (i = 0; i < size; i++) {
		sum += ptr[i];
	}

	return sum;
}

  
                                                                                  
                                                                                
                                                                                 
            
   
#define SEARCH_FOR_ACPI_TABLE(sdtp, signature, entry_type) \
{                                                                                               \
	uint32_t i, pointer_count;                                                              \
                                                                                                \
	                                                                                        \
	pointer_count = ((sdtp)->Length - sizeof(ACPI_TABLE_HEADER)) / sizeof(entry_type);      \
                                                                                                \
	for (i = 0; i < pointer_count; i++) {                                                   \
	        ACPI_TABLE_HEADER *next_table =                                                 \
	                (ACPI_TABLE_HEADER *)PHYSMAP_PTOV(                                      \
	                        (uintptr_t)(sdtp)->TableOffsetEntry[i]);                        \
	        if (strncmp(&next_table->Signature[0], (signature), 4) == 0) {                  \
	    
                                                              
               
                                                                                         \
	                if (cksum8((uint8_t *)next_table, next_table->Length) == 0) {           \
	                        return next_table;                                              \
	                } else {                                                                \
	                        DBG("Invalid checksum for table [%s]@0x%lx!\n", (signature),    \
	                            (unsigned long)(sdtp)->TableOffsetEntry[i]);                \
	                        return NULL;                                                    \
	                }                                                                       \
	        }                                                                               \
	}                                                                                       \
                                                                                                \
	return NULL;                                                                            \
}

static ACPI_TABLE_HEADER *
acpi_find_table_via_xsdt(XSDT_DESCRIPTOR *xsdtp, const char *signature)
{
	SEARCH_FOR_ACPI_TABLE(xsdtp, signature, UINT64);
}

static ACPI_TABLE_HEADER *
acpi_find_table_via_rsdt(RSDT_DESCRIPTOR *rsdtp, const char *signature)
{
	SEARCH_FOR_ACPI_TABLE(rsdtp, signature, UINT32);
}

  
                                                                       
                                                                         
   
static ACPI_TABLE_HEADER *
acpi_find_table(uintptr_t rsdp_physaddr, const char *signature)
{
	static RSDP_DESCRIPTOR *rsdp = NULL;
	static XSDT_DESCRIPTOR *xsdtp = NULL;
	static RSDT_DESCRIPTOR *rsdtp = NULL;

	if (signature == NULL) {
		DBG("Invalid NULL signature passed to acpi_find_table\n");
		return NULL;
	}

	  
                                                                        
    
	if (__improbable(rsdp == NULL || (rsdtp == NULL && xsdtp == NULL))) {
		rsdp = PHYSMAP_PTOV(rsdp_physaddr);

		                           
		if (__improbable(strncmp((void *)rsdp, "RSD PTR ", 8) != 0)) {
			DBG("RSDP signature mismatch: Aborting acpi_find_table\n");
			rsdp = NULL;
			return NULL;
		}

		                          
		if (__improbable(cksum8((uint8_t *)rsdp, sizeof(RSDP_DESCRIPTOR)) != 0)) {
			DBG("RSDP@0x%lx signature mismatch: Aborting acpi_find_table\n",
			    (unsigned long)rsdp_physaddr);
			rsdp = NULL;
			return NULL;
		}

		                                                                               
		if (__improbable(rsdp->Revision >= RSDP_VERSION_ACPI20 && rsdp->XsdtPhysicalAddress == 0ULL)) {
			DBG("RSDP XSDT Physical Address is 0!: Aborting acpi_find_table\n");
			rsdp = NULL;
			return NULL;
		} else if (__probable(rsdp->Revision >= RSDP_VERSION_ACPI20)) {
			                                           
			rsdtp = NULL;
			xsdtp = PHYSMAP_PTOV(rsdp->XsdtPhysicalAddress);
			if (cksum8((uint8_t *)xsdtp, xsdtp->Length) != 0) {
				DBG("ERROR: XSDT@0x%lx checksum is non-zero; not using this XSDT\n",
				    (unsigned long)rsdp->XsdtPhysicalAddress);
				xsdtp = NULL;
				return NULL;
			}
		} else if (__improbable(rsdp->Revision == RSDP_VERSION_ACPI10 && rsdp->RsdtPhysicalAddress == 0)) {
			DBG("RSDP RSDT Physical Address is 0!: Aborting acpi_find_table\n");
			rsdp = NULL;
			return NULL;
		} else if (__improbable(rsdp->Revision == RSDP_VERSION_ACPI10)) {
			                                           
			xsdtp = NULL;
			rsdtp = PHYSMAP_PTOV((uintptr_t)rsdp->RsdtPhysicalAddress);
			if (cksum8((uint8_t *)rsdtp, rsdtp->Length) != 0) {
				DBG("ERROR: RSDT@0x%lx checksum is non-zero; not using this RSDT\n",
				    (unsigned long)rsdp->RsdtPhysicalAddress);
				rsdtp = NULL;
				return NULL;
			}
		} else {
			DBG("Unrecognized RSDP Revision (0x%x): Aborting acpi_find_table\n",
			    rsdp->Revision);
			rsdp = NULL;
			return NULL;
		}
	}

	assert(xsdtp != NULL || rsdtp != NULL);

	if (__probable(xsdtp != NULL)) {
		return acpi_find_table_via_xsdt(xsdtp, signature);
	} else if (rsdtp != NULL) {
		return acpi_find_table_via_rsdt(rsdtp, signature);
	}

	return NULL;
}

  
                                                                      
                                               
   
uint32_t
acpi_count_enabled_logical_processors(void)
{
	MULTIPLE_APIC_TABLE *madtp;
	void *end_ptr;
	APIC_HEADER *next_apic_entryp;
	uint32_t enabled_cpu_count = 0;
	uint64_t rsdp_physaddr;

	rsdp_physaddr = efi_get_rsdp_physaddr();
	if (__improbable(rsdp_physaddr == 0)) {
		DBG("acpi_count_enabled_logical_processors: Could not get RSDP physaddr from EFI.\n");
		return 0;
	}

	madtp = (MULTIPLE_APIC_TABLE *)acpi_find_table(rsdp_physaddr, ACPI_SIG_MADT);

	if (__improbable(madtp == NULL)) {
		DBG("acpi_count_enabled_logical_processors: Could not find the MADT.\n");
		return 0;
	}

	end_ptr = (void *)((uintptr_t)madtp + madtp->Length);
	next_apic_entryp = (APIC_HEADER *)((uintptr_t)madtp + sizeof(MULTIPLE_APIC_TABLE));

	while ((void *)next_apic_entryp < end_ptr) {
		switch (next_apic_entryp->Type) {
		case APIC_PROCESSOR:
		{
			MADT_PROCESSOR_APIC *madt_procp = (MADT_PROCESSOR_APIC *)next_apic_entryp;
			if (madt_procp->ProcessorEnabled) {
				enabled_cpu_count++;
			}

			break;
		}

		default:
			DBG("Ignoring MADT entry type 0x%x length 0x%x\n", next_apic_entryp->Type,
			    next_apic_entryp->Length);
			break;
		}

		next_apic_entryp = (APIC_HEADER *)((uintptr_t)next_apic_entryp + next_apic_entryp->Length);
	}

	return enabled_cpu_count;
}
