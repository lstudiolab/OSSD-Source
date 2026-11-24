/*
 * Copyright (c) 2007-2009 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */

#include <debug.h>
#include <mach_ldebug.h>
#include <mach_kdp.h>

#include <kern/misc_protos.h>
#include <kern/thread.h>
#include <kern/timer_queue.h>
#include <kern/processor.h>
#include <kern/startup.h>
#include <kern/debug.h>
#include <prng/random.h>
#include <machine/machine_routines.h>
#include <machine/commpage.h>
#if HIBERNATION
#include <machine/pal_hibernate.h>
#endif                  
                             
#if __arm64__
#include <pexpert/arm64/apple_arm64_common.h>
#include <pexpert/arm64/boot.h>
#elif __arm__
#include <pexpert/arm/boot.h>
#else
#error Unsupported arch
#endif
#include <pexpert/arm/consistent_debug.h>
#include <pexpert/device_tree.h>
#include <arm/proc_reg.h>
#include <arm/pmap.h>
#include <arm/caches_internal.h>
#include <arm/cpu_internal.h>
#include <arm/cpu_data_internal.h>
#include <arm/cpuid_internal.h>
#include <arm/io_map_entries.h>
#include <arm/misc_protos.h>
#include <arm/machine_cpu.h>
#include <arm/rtclock.h>
#include <vm/vm_map.h>

#include <libkern/kernel_mach_header.h>
#include <libkern/stack_protector.h>
#include <libkern/section_keywords.h>
#include <san/kasan.h>
#include <sys/kdebug.h>

#include <pexpert/pexpert.h>

#include <console/serial_protos.h>

#if CONFIG_TELEMETRY
#include <kern/telemetry.h>
#endif
#if MONOTONIC
#include <kern/monotonic.h>
#endif                

#if HIBERNATION
#include <IOKit/IOPlatformExpert.h>
#endif                  

extern void     patch_low_glo(void);
extern int      serial_init(void);
extern void sleep_token_buffer_init(void);

extern vm_offset_t intstack_top;
#if __arm64__
extern vm_offset_t excepstack_top;
#else
extern vm_offset_t fiqstack_top;
#endif

extern const char version[];
extern const char version_variant[];
extern int      disableConsoleOutput;

int             pc_trace_buf[PC_TRACE_BUF_SIZE] = {0};
int             pc_trace_cnt = PC_TRACE_BUF_SIZE;
int             debug_task;

bool need_wa_rdar_55577508 = false;
SECURITY_READ_ONLY_LATE(bool) static_kernelcache = false;

#if HAS_BP_RET
                                                                                                 
uint32_t bp_ret = 3;
extern void set_bp_ret(void);
#endif

#if INTERRUPT_MASKED_DEBUG
boolean_t interrupt_masked_debug = 1;
                                              
uint64_t interrupt_masked_timeout = 0xd0000;
uint64_t stackshot_interrupt_masked_timeout = 0xf9999;
#endif

  
                                                                 
                                                    
   
#define XCALL_ACK_TIMEOUT_NS ((uint64_t) 6000000000)
uint64_t xcall_ack_timeout_abstime;

#if APPLEVIRTUALPLATFORM
extern uint64_t debug_ack_timeout;
#endif

boot_args const_boot_args __attribute__((section("__DATA, __const")));
boot_args      *BootArgs __attribute__((section("__DATA, __const")));

TUNABLE(uint32_t, arm_diag, "diag", 0);
#ifdef  APPLETYPHOON
static unsigned cpus_defeatures = 0x0;
extern void cpu_defeatures_set(unsigned int);
#endif

#if __arm64__ && __ARM_GLOBAL_SLEEP_BIT__
extern volatile boolean_t arm64_stall_sleep;
#endif

extern boolean_t force_immediate_debug_halt;

#if HAS_APPLE_PAC
SECURITY_READ_ONLY_LATE(boolean_t) diversify_user_jop = TRUE;
#endif

SECURITY_READ_ONLY_LATE(uint64_t) gDramBase;
SECURITY_READ_ONLY_LATE(uint64_t) gDramSize;

SECURITY_READ_ONLY_LATE(bool) serial_console_enabled = false;

  
                     
   
void arm_init(boot_args * args);

#if __arm64__
unsigned int page_shift_user32;                                             

extern void configure_misc_apple_boot_args(void);
extern void configure_misc_apple_regs(void);
#endif                


  
               
   

#define dyldLogFunc(msg, ...)
#include <mach/dyld_kernel_fixups.h>

extern uint32_t __thread_starts_sect_start[] __asm("section$start$__TEXT$__thread_starts");
extern uint32_t __thread_starts_sect_end[]   __asm("section$end$__TEXT$__thread_starts");
#if defined(HAS_APPLE_PAC)
extern void OSRuntimeSignStructors(kernel_mach_header_t * header);
extern void OSRuntimeSignStructorsInFileset(kernel_mach_header_t * header);
#endif                             

extern vm_offset_t vm_kernel_slide;
extern vm_offset_t segLOWESTKC, segHIGHESTKC, segLOWESTROKC, segHIGHESTROKC;
extern vm_offset_t segLOWESTAuxKC, segHIGHESTAuxKC, segLOWESTROAuxKC, segHIGHESTROAuxKC;
extern vm_offset_t segLOWESTRXAuxKC, segHIGHESTRXAuxKC, segHIGHESTNLEAuxKC;

static void
arm_slide_rebase_and_sign_image(void)
{
	kernel_mach_header_t *k_mh, *kc_mh = NULL;
	kernel_segment_command_t *seg;
	uintptr_t slide;

	k_mh = &_mh_execute_header;
	if (kernel_mach_header_is_in_fileset(k_mh)) {
		  
                                                                          
                                                                       
                                                       
     
		seg = (kernel_segment_command_t *)((uintptr_t)k_mh + sizeof(*k_mh));
		assert(seg->cmd == LC_SEGMENT_KERNEL);
		slide = (uintptr_t)k_mh - seg->vmaddr;

		  
                                                                          
                                                                        
                                   
     
		kc_mh = (kernel_mach_header_t*)(VM_KERNEL_LINK_ADDRESS + slide);
		assert(kc_mh->filetype == MH_FILESET);

		  
                         
                                                                
                                                               
                                                        
                                                   
                                      
     
		const void *collection_base_pointers[KCNumKinds] = {[0] = kc_mh, };
		kernel_collection_slide((struct mach_header_64 *)kc_mh, collection_base_pointers);

		PE_set_kc_header(KCKindPrimary, kc_mh, slide);

		  
                                                                     
                                                                   
                                                      
     
		kernel_collection_adjust_mh_addrs((struct mach_header_64 *)kc_mh, slide, false,
		    (uintptr_t *)&segLOWESTKC, (uintptr_t *)&segHIGHESTKC,
		    (uintptr_t *)&segLOWESTROKC, (uintptr_t *)&segHIGHESTROKC,
		    NULL, NULL, NULL);
#if defined(HAS_APPLE_PAC)
		OSRuntimeSignStructorsInFileset(kc_mh);
#endif                             
	} else {
		  
                                                                         
                                        
     
		slide = (uintptr_t)k_mh - VM_KERNEL_LINK_ADDRESS;

		                          
		static_kernelcache = &__thread_starts_sect_end[0] != &__thread_starts_sect_start[0];
		if (static_kernelcache) {
			rebase_threaded_starts( &__thread_starts_sect_start[0],
			    &__thread_starts_sect_end[0],
			    (uintptr_t)k_mh, (uintptr_t)k_mh - slide, slide);
		}
#if defined(HAS_APPLE_PAC)
		OSRuntimeSignStructors(&_mh_execute_header);
#endif                             
	}


	  
                                                                   
                 
    
	vm_kernel_slide = slide;
}

void
arm_auxkc_init(void *mh, void *base)
{
	  
                                                                        
                                                                           
                                                  
    
	uintptr_t slide = (uintptr_t)base;
	kernel_mach_header_t *akc_mh = (kernel_mach_header_t*)mh;

	assert(akc_mh->filetype == MH_FILESET);
	PE_set_kc_header_and_base(KCKindAuxiliary, akc_mh, base, slide);

	                          
	const void *collection_base_pointers[KCNumKinds];
	memcpy(collection_base_pointers, PE_get_kc_base_pointers(), sizeof(collection_base_pointers));
	kernel_collection_slide((struct mach_header_64 *)akc_mh, collection_base_pointers);

	kernel_collection_adjust_mh_addrs((struct mach_header_64 *)akc_mh, slide, false,
	    (uintptr_t *)&segLOWESTAuxKC, (uintptr_t *)&segHIGHESTAuxKC, (uintptr_t *)&segLOWESTROAuxKC,
	    (uintptr_t *)&segHIGHESTROAuxKC, (uintptr_t *)&segLOWESTRXAuxKC, (uintptr_t *)&segHIGHESTRXAuxKC,
	    (uintptr_t *)&segHIGHESTNLEAuxKC);
#if defined(HAS_APPLE_PAC)
	OSRuntimeSignStructorsInFileset(akc_mh);
#endif                             
}

  
                      
                                                               
   

__startup_func
void
arm_init(
	boot_args       *args)
{
	unsigned int    maxmem;
	uint32_t        memsize;
	uint64_t        xmaxmem;
	thread_t        thread;

	arm_slide_rebase_and_sign_image();

	                                                                             
	const_boot_args = *args;
	BootArgs = args = &const_boot_args;

	cpu_data_init(&BootCpuData);
#if defined(HAS_APPLE_PAC)
	                                                                               
	BootCpuData.rop_key = ml_default_rop_pid();
	BootCpuData.jop_key = ml_default_jop_pid();
#endif                             

	PE_init_platform(FALSE, args);                                 

#if __arm64__
	wfe_timeout_configure();

	configure_misc_apple_boot_args();
	configure_misc_apple_regs();


	{
		  
                                            
     
		if (args->memSize > 1ULL * 1024 * 1024 * 1024) {
			  
                                     
                             
      
			PAGE_SHIFT_CONST = PAGE_MAX_SHIFT;
		} else {
			  
                                      
                                    
                                    
      
			PAGE_SHIFT_CONST = ARM_PGSHIFT;
		}

		                                           
		page_shift_user32 = PAGE_MAX_SHIFT;
#ifdef  APPLETYPHOON
		if (PE_parse_boot_argn("cpus_defeatures", &cpus_defeatures, sizeof(cpus_defeatures))) {
			if ((cpus_defeatures & 0xF) != 0) {
				cpu_defeatures_set(cpus_defeatures & 0xF);
			}
		}
#endif
	}
#endif

	ml_parse_cpu_topology();

	master_cpu = ml_get_boot_cpu_number();
	assert(master_cpu >= 0 && master_cpu <= ml_get_max_cpu_number());

	BootCpuData.cpu_number = (unsigned short)master_cpu;
#if     __arm__
	BootCpuData.cpu_exc_vectors = (vm_offset_t)&ExceptionVectorsTable;
#endif
	BootCpuData.intstack_top = (vm_offset_t) &intstack_top;
	BootCpuData.istackptr = BootCpuData.intstack_top;
#if __arm64__
	BootCpuData.excepstack_top = (vm_offset_t) &excepstack_top;
	BootCpuData.excepstackptr = BootCpuData.excepstack_top;
#else
	BootCpuData.fiqstack_top = (vm_offset_t) &fiqstack_top;
	BootCpuData.fiqstackptr = BootCpuData.fiqstack_top;
#endif
	BootCpuData.cpu_console_buf = (void *)NULL;
	CpuDataEntries[master_cpu].cpu_data_vaddr = &BootCpuData;
	CpuDataEntries[master_cpu].cpu_data_paddr = (void *)((uintptr_t)(args->physBase)
	    + ((uintptr_t)&BootCpuData
	    - (uintptr_t)(args->virtBase)));

	thread = thread_bootstrap();
	thread->machine.CpuDatap = &BootCpuData;
	thread->machine.pcpu_data_base = (vm_offset_t)0;
	machine_set_current_thread(thread);

	  
                                                                             
                                                                             
                                                                          
                                                                       
    
	thread->machine.preemption_count = 0;
#if     __arm__ && __ARM_USER_PROTECT__
	{
		unsigned int ttbr0_val, ttbr1_val;
		__asm__ volatile ("mrc p15,0,%0,c2,c0,0\n" : "=r"(ttbr0_val));
		__asm__ volatile ("mrc p15,0,%0,c2,c0,1\n" : "=r"(ttbr1_val));
		thread->machine.uptw_ttb = ttbr0_val;
		thread->machine.kptw_ttb = ttbr1_val;
	}
#endif
	processor_t boot_processor = PERCPU_GET_MASTER(processor);
	boot_processor->kernel_timer = &thread->system_timer;
	boot_processor->thread_timer = &thread->system_timer;

	cpu_bootstrap();

	rtclock_early_init();

	kernel_debug_string_early("kernel_startup_bootstrap");
	kernel_startup_bootstrap();

	  
                                      
    
	timer_call_init();

	cpu_init();

	processor_bootstrap();

	if (PE_parse_boot_argn("maxmem", &maxmem, sizeof(maxmem))) {
		xmaxmem = (uint64_t) maxmem * (1024 * 1024);
	} else if (PE_get_default("hw.memsize", &memsize, sizeof(memsize))) {
		xmaxmem = (uint64_t) memsize;
	} else {
		xmaxmem = 0;
	}

#if INTERRUPT_MASKED_DEBUG
	int wdt_boot_arg = 0;
	                                                                        
	if (PE_parse_boot_argn("no_interrupt_masked_debug", &interrupt_masked_debug,
	    sizeof(interrupt_masked_debug)) || (PE_parse_boot_argn("wdt", &wdt_boot_arg,
	    sizeof(wdt_boot_arg)) && (wdt_boot_arg == -1)) || kern_feature_override(KF_INTERRUPT_MASKED_DEBUG_OVRD)) {
		interrupt_masked_debug = 0;
	}

	PE_parse_boot_argn("interrupt_masked_debug_timeout", &interrupt_masked_timeout, sizeof(interrupt_masked_timeout));

#endif                             

	nanoseconds_to_absolutetime(XCALL_ACK_TIMEOUT_NS, &xcall_ack_timeout_abstime);

#if APPLEVIRTUALPLATFORM
	unsigned int vti;

	if (!PE_parse_boot_argn("vti", &vti, sizeof(vti))) {
		vti = 6;
	}

#define VIRTUAL_TIMEOUT_INFLATE_ABS(_timeout)              \
MACRO_BEGIN                                                \
	_timeout = virtual_timeout_inflate_abs(vti, _timeout); \
MACRO_END

#define VIRTUAL_TIMEOUT_INFLATE_NS(_timeout)              \
MACRO_BEGIN                                                \
	_timeout = virtual_timeout_inflate_ns(vti, _timeout); \
MACRO_END

#if INTERRUPT_MASKED_DEBUG
	VIRTUAL_TIMEOUT_INFLATE_ABS(interrupt_masked_timeout);
	VIRTUAL_TIMEOUT_INFLATE_ABS(stackshot_interrupt_masked_timeout);
#endif                             
	VIRTUAL_TIMEOUT_INFLATE_NS(debug_ack_timeout);
#endif                           

#if HAS_BP_RET
	PE_parse_boot_argn("bpret", &bp_ret, sizeof(bp_ret));
	set_bp_ret();                                                         
#endif

	PE_parse_boot_argn("immediate_NMI", &force_immediate_debug_halt, sizeof(force_immediate_debug_halt));

#if __ARM_PAN_AVAILABLE__
	__builtin_arm_wsr("pan", 1);
#endif                             

	arm_vm_init(xmaxmem, args);

	if (debug_boot_arg) {
		patch_low_glo();
	}

#if __arm64__ && WITH_CLASSIC_S2R
	sleep_token_buffer_init();
#endif

	PE_consistent_debug_inherit();

	  
                                                                                                                                          
                                                                              
                                                                                                                 
    
#if __arm64__
	need_wa_rdar_55577508 = cpuid_get_cpufamily() == CPUFAMILY_ARM_LIGHTNING_THUNDER;
#ifndef RC_HIDE_XNU_FIRESTORM
	need_wa_rdar_55577508 |= (cpuid_get_cpufamily() == CPUFAMILY_ARM_FIRESTORM_ICESTORM && get_arm_cpu_version() == CPU_VERSION_A0);
#endif
#endif

	                                                   
	kernel_startup_initialize_upto(STARTUP_SUB_KPRINTF);
	kprintf("kprintf initialized\n");

	serialmode = 0;
	if (PE_parse_boot_argn("serial", &serialmode, sizeof(serialmode))) {
		                                                  
		kprintf("Serial mode specified: %08X\n", serialmode);
		int force_sync = serialmode & SERIALMODE_SYNCDRAIN;
		if (force_sync || PE_parse_boot_argn("drain_uart_sync", &force_sync, sizeof(force_sync))) {
			if (force_sync) {
				serialmode |= SERIALMODE_SYNCDRAIN;
				kprintf(
					"WARNING: Forcing uart driver to output synchronously."
					"printf()s/IOLogs will impact kernel performance.\n"
					"You are advised to avoid using 'drain_uart_sync' boot-arg.\n");
			}
		}
	}
	if (kern_feature_override(KF_SERIAL_OVRD)) {
		serialmode = 0;
	}

	if (serialmode & SERIALMODE_OUTPUT) {                                                
		serial_console_enabled = true;
		(void)switch_to_serial_console();                              
		disableConsoleOutput = FALSE;                                  
	}
	PE_create_console();

	                          
	PE_init_printf(FALSE);

#if __arm64__
#if DEBUG
	dump_kva_space();
#endif
#endif

	cpu_machine_idle_init(TRUE);

#if     (__ARM_ARCH__ == 7)
	if (arm_diag & 0x8000) {
		set_mmu_control((get_mmu_control()) ^ SCTLR_PREDIC);
	}
#endif

	PE_init_platform(TRUE, &BootCpuData);

#if __arm64__
	ml_map_cpu_pio();
#endif

	cpu_timebase_init(TRUE);
	PE_init_cpu();
	fiq_context_init(TRUE);


#if HIBERNATION
	pal_hib_init();
#endif                  

	  
                                                                                
                                                                          
                
    
	DTEntry chosen;
	unsigned int dt_entry_size;
	unsigned long const *dram_base;
	unsigned long const *dram_size;
	if (SecureDTLookupEntry(NULL, "/chosen", &chosen) != kSuccess) {
		panic("%s: Unable to find 'chosen' DT node", __FUNCTION__);
	}

	if (SecureDTGetProperty(chosen, "dram-base", (void const **)&dram_base, &dt_entry_size) != kSuccess) {
		panic("%s: Unable to find 'dram-base' entry in the 'chosen' DT node", __FUNCTION__);
	}

	if (SecureDTGetProperty(chosen, "dram-size", (void const **)&dram_size, &dt_entry_size) != kSuccess) {
		panic("%s: Unable to find 'dram-size' entry in the 'chosen' DT node", __FUNCTION__);
	}

	gDramBase = *dram_base;
	gDramSize = *dram_size;

	  
                                                       
                                                  
                                                         
                                                          
                                              
    
	__stack_chk_guard = (unsigned long)early_random();
	                                        
                                  
    
	__stack_chk_guard &= ~(0xFFULL << 8);
	machine_startup(args);
}

  
                               
            
                                                                      
   

void
arm_init_cpu(
	cpu_data_t      *cpu_data_ptr)
{
#if __ARM_PAN_AVAILABLE__
	__builtin_arm_wsr("pan", 1);
#endif

#ifdef __arm64__
	configure_misc_apple_regs();
#endif

	cpu_data_ptr->cpu_flags &= ~SleepState;
#if     defined(ARMA7)
	cpu_data_ptr->cpu_CLW_active = 1;
#endif

	machine_set_current_thread(cpu_data_ptr->cpu_active_thread);

#if HIBERNATION
	if ((cpu_data_ptr == &BootCpuData) && (gIOHibernateState == kIOHibernateStateWakingFromHibernate)) {
		                                                                                                      
		extern uint64_t wake_abstime;
		wake_abstime = gIOHibernateCurrentHeader->lastHibAbsTime;

		                                                                                   
		                                                                             
		extern uint64_t hwclock_conttime_offset;
		hwclock_conttime_offset = gIOHibernateCurrentHeader->hwClockOffset;

		                                                                                                     
		                              
		unsigned int expected_preemption_count = (gEnforceQuiesceSafety ? 2 : 1);
		if (cpu_data_ptr->cpu_active_thread->machine.preemption_count != expected_preemption_count) {
			panic("unexpected preemption count %u on boot cpu thread (should be %u)\n",
			    cpu_data_ptr->cpu_active_thread->machine.preemption_count,
			    expected_preemption_count);
		}
		cpu_data_ptr->cpu_active_thread->machine.preemption_count--;
	}
#endif                  

#if __arm64__
	wfe_timeout_init();
	pmap_clear_user_ttb();
	flush_mmu_tlb();
#endif

	cpu_machine_idle_init(FALSE);

	cpu_init();

#if     (__ARM_ARCH__ == 7)
	if (arm_diag & 0x8000) {
		set_mmu_control((get_mmu_control()) ^ SCTLR_PREDIC);
	}
#endif
#ifdef  APPLETYPHOON
	if ((cpus_defeatures & (0xF << 4 * cpu_data_ptr->cpu_number)) != 0) {
		cpu_defeatures_set((cpus_defeatures >> 4 * cpu_data_ptr->cpu_number) & 0xF);
	}
#endif
	                                                             
                                                              
    
	cpu_timebase_init(FALSE);

	if (cpu_data_ptr == &BootCpuData) {
#if __arm64__ && __ARM_GLOBAL_SLEEP_BIT__
		  
                                                      
                             
     
		arm64_stall_sleep = TRUE;
#endif
		serial_init();
		PE_init_platform(TRUE, NULL);
		commpage_update_timebase();
	}
	PE_init_cpu();

	fiq_context_init(TRUE);
	cpu_data_ptr->rtcPop = EndOfAllTime;
	timer_resync_deadlines();

#if DEVELOPMENT || DEBUG
	PE_arm_debug_enable_trace();
#endif


	kprintf("arm_cpu_init(): cpu %d online\n", cpu_data_ptr->cpu_number);

	if (cpu_data_ptr == &BootCpuData) {
		if (kdebug_enable == 0) {
			__kdebug_only uint64_t elapsed = kdebug_wake();
			KDBG(IOKDBG_CODE(DBG_HIBERNATE, 15), mach_absolute_time() - elapsed);
		}

#if CONFIG_TELEMETRY
		bootprofile_wake_from_sleep();
#endif                       
	}
#if MONOTONIC && defined(__arm64__)
	mt_wake_per_core();
#endif                                      

#if defined(KERNEL_INTEGRITY_CTRR)
	if (ctrr_cluster_locked[cpu_data_ptr->cpu_cluster_id] != CTRR_LOCKED) {
		lck_spin_lock(&ctrr_cpu_start_lck);
		ctrr_cluster_locked[cpu_data_ptr->cpu_cluster_id] = CTRR_LOCKED;
		thread_wakeup(&ctrr_cluster_locked[cpu_data_ptr->cpu_cluster_id]);
		lck_spin_unlock(&ctrr_cpu_start_lck);
	}
#endif

	slave_main(NULL);
}

  
                              
                                                                          
   
void __attribute__((noreturn))
arm_init_idle_cpu(
	cpu_data_t      *cpu_data_ptr)
{
#if __ARM_PAN_AVAILABLE__
	__builtin_arm_wsr("pan", 1);
#endif
#if     defined(ARMA7)
	cpu_data_ptr->cpu_CLW_active = 1;
#endif

	machine_set_current_thread(cpu_data_ptr->cpu_active_thread);

#if __arm64__
	wfe_timeout_init();
	pmap_clear_user_ttb();
	flush_mmu_tlb();
	                                    
	__builtin_arm_wsr("DAIFClr", DAIFSC_ASYNCF);
#endif

#if     (__ARM_ARCH__ == 7)
	if (arm_diag & 0x8000) {
		set_mmu_control((get_mmu_control()) ^ SCTLR_PREDIC);
	}
#endif
#ifdef  APPLETYPHOON
	if ((cpus_defeatures & (0xF << 4 * cpu_data_ptr->cpu_number)) != 0) {
		cpu_defeatures_set((cpus_defeatures >> 4 * cpu_data_ptr->cpu_number) & 0xF);
	}
#endif

	fiq_context_init(FALSE);

	cpu_idle_exit(TRUE);
}
