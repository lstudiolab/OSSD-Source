/*
 * Copyright (c) 2007 Apple Inc. All rights reserved.
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
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
 * @APPLE_FREE_COPYRIGHT@
 */
  
                                 
                                          
   
#include <libkern/section_keywords.h>
#include <mach/mach_types.h>
#include <mach/machine.h>
#include <mach/vm_map.h>
#include <machine/cpu_capabilities.h>
#include <machine/commpage.h>
#include <machine/config.h>
#include <machine/pmap.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_protos.h>
#include <ipc/ipc_port.h>
#include <arm/cpuid.h>                                               
#include <arm/rtclock.h>
#include <libkern/OSAtomic.h>
#include <stdatomic.h>
#include <kern/remote_time.h>
#include <machine/machine_remote_time.h>
#include <machine/machine_routines.h>

#include <sys/kdebug.h>

#if CONFIG_ATM
#include <atm/atm_internal.h>
#endif

static int commpage_cpus( void );


static void commpage_init_cpu_capabilities( void );

SECURITY_READ_ONLY_LATE(vm_address_t)   commPagePtr = 0;
SECURITY_READ_ONLY_LATE(vm_address_t)   sharedpage_rw_addr = 0;
SECURITY_READ_ONLY_LATE(uint64_t)       _cpu_capabilities = 0;
SECURITY_READ_ONLY_LATE(vm_address_t)   sharedpage_rw_text_addr = 0;

extern user64_addr_t commpage_text64_location;
extern user32_addr_t commpage_text32_location;

                                     
extern int      gARMv81Atomics;
extern int      gARMv8Crc32;
extern int      gARMv82FHM;
extern int      gARMv82SHA512;
extern int      gARMv82SHA3;

void
commpage_populate(void)
{
	uint16_t        c2;
	int cpufamily;

	                                        
	vm_map_address_t kernel_data_addr, kernel_text_addr, user_text_addr;
	pmap_create_sharedpages(&kernel_data_addr, &kernel_text_addr, &user_text_addr);

	sharedpage_rw_addr = kernel_data_addr;
	sharedpage_rw_text_addr = kernel_text_addr;
	commPagePtr = (vm_address_t) _COMM_PAGE_BASE_ADDRESS;

#if __arm64__
	commpage_text64_location = user_text_addr;
	bcopy(_COMM_PAGE64_SIGNATURE_STRING, (void *)(_COMM_PAGE_SIGNATURE + _COMM_PAGE_RW_OFFSET),
	    MIN(_COMM_PAGE_SIGNATURELEN, strlen(_COMM_PAGE64_SIGNATURE_STRING)));
#else
	commpage_text32_location = user_text_addr;
	bcopy(_COMM_PAGE32_SIGNATURE_STRING, (void *)(_COMM_PAGE_SIGNATURE + _COMM_PAGE_RW_OFFSET),
	    MIN(_COMM_PAGE_SIGNATURELEN, strlen(_COMM_PAGE32_SIGNATURE_STRING)));
#endif

	*((uint16_t*)(_COMM_PAGE_VERSION + _COMM_PAGE_RW_OFFSET)) = (uint16_t) _COMM_PAGE_THIS_VERSION;

	commpage_init_cpu_capabilities();
	commpage_set_timestamp(0, 0, 0, 0, 0);

	if (_cpu_capabilities & kCache32) {
		c2 = 32;
	} else if (_cpu_capabilities & kCache64) {
		c2 = 64;
	} else if (_cpu_capabilities & kCache128) {
		c2 = 128;
	} else {
		c2 = 0;
	}

	*((uint16_t*)(_COMM_PAGE_CACHE_LINESIZE + _COMM_PAGE_RW_OFFSET)) = c2;

	commpage_update_active_cpus();
	cpufamily = cpuid_get_cpufamily();

	*((uint8_t*)(_COMM_PAGE_PHYSICAL_CPUS + _COMM_PAGE_RW_OFFSET)) = (uint8_t) machine_info.physical_cpu_max;
	*((uint8_t*)(_COMM_PAGE_LOGICAL_CPUS + _COMM_PAGE_RW_OFFSET)) = (uint8_t) machine_info.logical_cpu_max;
	*((uint64_t*)(_COMM_PAGE_MEMORY_SIZE + _COMM_PAGE_RW_OFFSET)) = machine_info.max_mem;
	*((uint32_t*)(_COMM_PAGE_CPUFAMILY + _COMM_PAGE_RW_OFFSET)) = (uint32_t)cpufamily;
	*((uint32_t*)(_COMM_PAGE_DEV_FIRM + _COMM_PAGE_RW_OFFSET)) = (uint32_t)PE_i_can_has_debugger(NULL);
	*((uint8_t*)(_COMM_PAGE_USER_TIMEBASE + _COMM_PAGE_RW_OFFSET)) = user_timebase_type();
	*((uint8_t*)(_COMM_PAGE_CONT_HWCLOCK + _COMM_PAGE_RW_OFFSET)) = (uint8_t)user_cont_hwclock_allowed();
	*((uint8_t*)(_COMM_PAGE_KERNEL_PAGE_SHIFT + _COMM_PAGE_RW_OFFSET)) = (uint8_t) page_shift;

#if __arm64__
	*((uint8_t*)(_COMM_PAGE_USER_PAGE_SHIFT_32 + _COMM_PAGE_RW_OFFSET)) = (uint8_t) page_shift_user32;
	*((uint8_t*)(_COMM_PAGE_USER_PAGE_SHIFT_64 + _COMM_PAGE_RW_OFFSET)) = (uint8_t) SIXTEENK_PAGE_SHIFT;
#elif (__ARM_ARCH_7K__ >= 2)
	                                                           
	*((uint8_t*)(_COMM_PAGE_USER_PAGE_SHIFT_32 + _COMM_PAGE_RW_OFFSET)) = (uint8_t) SIXTEENK_PAGE_SHIFT;
	*((uint8_t*)(_COMM_PAGE_USER_PAGE_SHIFT_64 + _COMM_PAGE_RW_OFFSET)) = (uint8_t) SIXTEENK_PAGE_SHIFT;
#else                
	*((uint8_t*)(_COMM_PAGE_USER_PAGE_SHIFT_32 + _COMM_PAGE_RW_OFFSET)) = (uint8_t) PAGE_SHIFT;
	*((uint8_t*)(_COMM_PAGE_USER_PAGE_SHIFT_64 + _COMM_PAGE_RW_OFFSET)) = (uint8_t) PAGE_SHIFT;
#endif                

	commpage_update_timebase();
	commpage_update_mach_continuous_time(0);

	clock_sec_t secs;
	clock_usec_t microsecs;
	clock_get_boottime_microtime(&secs, &microsecs);
	commpage_update_boottime(secs * USEC_PER_SEC + microsecs);

	  
                                                             
                                                                     
    
	*((uint64_t *)(_COMM_PAGE_APPROX_TIME + _COMM_PAGE_RW_OFFSET)) = 0;
#ifdef CONFIG_MACH_APPROXIMATE_TIME
	*((uint8_t *)(_COMM_PAGE_APPROX_TIME_SUPPORTED + _COMM_PAGE_RW_OFFSET)) = 1;
#else
	*((uint8_t *)(_COMM_PAGE_APPROX_TIME_SUPPORTED + _COMM_PAGE_RW_OFFSET)) = 0;
#endif

	commpage_update_kdebug_state();

#if CONFIG_ATM
	commpage_update_atm_diagnostic_config(atm_get_diagnostic_config());
#endif


	*((uint64_t*)(_COMM_PAGE_REMOTETIME_PARAMS + _COMM_PAGE_RW_OFFSET)) = BT_RESET_SENTINEL_TS;
}

#define COMMPAGE_TEXT_SEGMENT "__TEXT_EXEC"
#define COMMPAGE_TEXT_SECTION "__commpage_text"

                                                                              
                                                                                  
extern char commpage_text_start[]
__SECTION_START_SYM(COMMPAGE_TEXT_SEGMENT, COMMPAGE_TEXT_SECTION);

                                                                            
                                                                                
extern char commpage_text_end[]
__SECTION_END_SYM(COMMPAGE_TEXT_SEGMENT, COMMPAGE_TEXT_SECTION);

                                                                                           
               
extern char commpage_text_preemptible_functions;

#if CONFIG_ARM_PFZ
static size_t size_of_pfz = 0;
#endif

                                     
#define BRK_666_OPCODE 0xD4205340

void
commpage_text_populate(void)
{
#if CONFIG_ARM_PFZ
	size_t size_of_commpage_text = commpage_text_end - commpage_text_start;
	if (size_of_commpage_text == 0) {
		panic("ARM comm page text section %s,%s missing", COMMPAGE_TEXT_SEGMENT, COMMPAGE_TEXT_SECTION);
	}
	assert(size_of_commpage_text <= PAGE_SIZE);
	assert(size_of_commpage_text > 0);

	                                                                 
	size_of_pfz = &commpage_text_preemptible_functions - commpage_text_start;

	                                                               
	memcpy((void *) _COMM_PAGE64_TEXT_START_ADDRESS, (void *) commpage_text_start, size_of_commpage_text);

	                                                                           
	                   
	memset((char *) _COMM_PAGE64_TEXT_START_ADDRESS + size_of_commpage_text, BRK_666_OPCODE,
	    PAGE_SIZE - size_of_commpage_text);
#endif
}

uint32_t
commpage_is_in_pfz64(addr64_t addr64)
{
#if CONFIG_ARM_PFZ
	if ((addr64 >= commpage_text64_location) &&
	    (addr64 < (commpage_text64_location + size_of_pfz))) {
		return 1;
	} else {
		return 0;
	}
#else
#pragma unused (addr64)
	return 0;
#endif
}


void
commpage_set_timestamp(
	uint64_t        tbr,
	uint64_t        secs,
	uint64_t        frac,
	uint64_t        scale,
	uint64_t        tick_per_sec)
{
	new_commpage_timeofday_data_t *commpage_timeofday_datap;

	if (commPagePtr == 0) {
		return;
	}

	commpage_timeofday_datap =  (new_commpage_timeofday_data_t *)(_COMM_PAGE_NEWTIMEOFDAY_DATA + _COMM_PAGE_RW_OFFSET);

	commpage_timeofday_datap->TimeStamp_tick = 0x0ULL;

#if     (__ARM_ARCH__ >= 7)
	__asm__ volatile ("dmb ish");
#endif
	commpage_timeofday_datap->TimeStamp_sec = secs;
	commpage_timeofday_datap->TimeStamp_frac = frac;
	commpage_timeofday_datap->Ticks_scale = scale;
	commpage_timeofday_datap->Ticks_per_sec = tick_per_sec;

#if     (__ARM_ARCH__ >= 7)
	__asm__ volatile ("dmb ish");
#endif
	commpage_timeofday_datap->TimeStamp_tick = tbr;

}

  
                                                                                              
   

void
commpage_set_memory_pressure(
	unsigned int    pressure )
{
	if (commPagePtr == 0) {
		return;
	}
	*((uint32_t *)(_COMM_PAGE_MEMORY_PRESSURE + _COMM_PAGE_RW_OFFSET)) = pressure;
}

  
                                           
   
static int
commpage_cpus( void )
{
	int cpus;

	cpus = machine_info.max_cpus;

	if (cpus == 0) {
		panic("commpage cpus==0");
	}
	if (cpus > 0xFF) {
		cpus = 0xFF;
	}

	return cpus;
}

uint64_t
_get_cpu_capabilities(void)
{
	return _cpu_capabilities;
}

vm_address_t
_get_commpage_priv_address(void)
{
	return sharedpage_rw_addr;
}

vm_address_t
_get_commpage_text_priv_address(void)
{
	return sharedpage_rw_text_addr;
}

  
                                      
   
static void
commpage_init_cpu_capabilities( void )
{
	uint64_t bits;
	int cpus;
	ml_cpu_info_t cpu_info;

	bits = 0;
	ml_cpu_get_info(&cpu_info);

	switch (cpu_info.cache_line_size) {
	case 128:
		bits |= kCache128;
		break;
	case 64:
		bits |= kCache64;
		break;
	case 32:
		bits |= kCache32;
		break;
	default:
		break;
	}
	cpus = commpage_cpus();

	if (cpus == 1) {
		bits |= kUP;
	}

	bits |= (cpus << kNumCPUsShift);

	bits |= kFastThreadLocalStorage;                           

#if     __ARM_VFP__
	bits |= kHasVfp;
	arm_mvfp_info_t *mvfp_info = arm_mvfp_info();
	if (mvfp_info->neon) {
		bits |= kHasNeon;
	}
	if (mvfp_info->neon_hpfp) {
		bits |= kHasNeonHPFP;
	}
	if (mvfp_info->neon_fp16) {
		bits |= kHasNeonFP16;
	}
#endif
#if defined(__arm64__)
	bits |= kHasFMA;
#endif
#if     __ARM_ENABLE_WFE_
	bits |= kHasEvent;
#endif
#if __ARM_V8_CRYPTO_EXTENSIONS__
	bits |= kHasARMv8Crypto;
#endif
#ifdef __arm64__
	uint64_t isar0 = __builtin_arm_rsr64("ID_AA64ISAR0_EL1");
	if ((isar0 & ID_AA64ISAR0_EL1_ATOMIC_MASK) == ID_AA64ISAR0_EL1_ATOMIC_8_1) {
		bits |= kHasARMv81Atomics;
		gARMv81Atomics = 1;
	}
	if ((isar0 & ID_AA64ISAR0_EL1_CRC32_MASK) == ID_AA64ISAR0_EL1_CRC32_EN) {
		bits |= kHasARMv8Crc32;
		gARMv8Crc32 = 1;
	}
	if ((isar0 & ID_AA64ISAR0_EL1_FHM_MASK) >= ID_AA64ISAR0_EL1_FHM_8_2) {
		bits |= kHasARMv82FHM;
		gARMv82FHM = 1;
	}

	if ((isar0 & ID_AA64ISAR0_EL1_SHA2_MASK) > ID_AA64ISAR0_EL1_SHA2_EN) {
		bits |= kHasARMv82SHA512;
		gARMv82SHA512 = 1;
	}
	if ((isar0 & ID_AA64ISAR0_EL1_SHA3_MASK) >= ID_AA64ISAR0_EL1_SHA3_EN) {
		bits |= kHasARMv82SHA3;
		gARMv82SHA3 = 1;
	}

#endif




	_cpu_capabilities = bits;

	*((uint32_t *)(_COMM_PAGE_CPU_CAPABILITIES + _COMM_PAGE_RW_OFFSET)) = (uint32_t)_cpu_capabilities;
	*((uint64_t *)(_COMM_PAGE_CPU_CAPABILITIES64 + _COMM_PAGE_RW_OFFSET)) = _cpu_capabilities;
}

  
                                                       
   
void
commpage_update_active_cpus(void)
{
	if (!commPagePtr) {
		return;
	}
	*((uint8_t *)(_COMM_PAGE_ACTIVE_CPUS + _COMM_PAGE_RW_OFFSET)) = (uint8_t)processor_avail_count;

}

  
                                                                                           
   
void
commpage_update_timebase(void)
{
	if (commPagePtr) {
		*((uint64_t*)(_COMM_PAGE_TIMEBASE_OFFSET + _COMM_PAGE_RW_OFFSET)) = rtclock_base_abstime;
	}
}

  
                                                                             
                                                                               
                                 
  
                                                                             
                                                                        
   
void
commpage_update_kdebug_state(void)
{
	if (commPagePtr) {
		*((volatile uint32_t*)(_COMM_PAGE_KDEBUG_ENABLE + _COMM_PAGE_RW_OFFSET)) = kdebug_commpage_state();
	}
}

                                     
void
commpage_update_atm_diagnostic_config(uint32_t diagnostic_config)
{
	if (commPagePtr) {
		*((volatile uint32_t*)(_COMM_PAGE_ATM_DIAGNOSTIC_CONFIG + _COMM_PAGE_RW_OFFSET)) = diagnostic_config;
	}
}

  
                                                                
                                                               
                                                      
   
void
commpage_update_multiuser_config(uint32_t multiuser_config)
{
	if (commPagePtr) {
		*((volatile uint32_t *)(_COMM_PAGE_MULTIUSER_CONFIG + _COMM_PAGE_RW_OFFSET)) = multiuser_config;
	}
}

  
                               
                                           
   

void
commpage_update_mach_approximate_time(uint64_t abstime)
{
#ifdef CONFIG_MACH_APPROXIMATE_TIME
	uintptr_t approx_time_base = (uintptr_t)(_COMM_PAGE_APPROX_TIME + _COMM_PAGE_RW_OFFSET);
	uint64_t saved_data;

	if (commPagePtr) {
		saved_data = atomic_load_explicit((_Atomic uint64_t *)approx_time_base,
		    memory_order_relaxed);
		if (saved_data < abstime) {
			                                                       
                                                          
                                                         
                     
			atomic_compare_exchange_strong_explicit((_Atomic uint64_t *)approx_time_base,
			    &saved_data, abstime, memory_order_relaxed, memory_order_relaxed);
		}
	}
#else
#pragma unused (abstime)
#endif
}

  
                                                         
                                           
   
void
commpage_update_mach_continuous_time(uint64_t sleeptime)
{
	if (commPagePtr) {
#ifdef __arm64__
		*((uint64_t *)(_COMM_PAGE_CONT_TIMEBASE + _COMM_PAGE_RW_OFFSET)) = sleeptime;
#else
		uint64_t *c_time_base = (uint64_t *)(_COMM_PAGE_CONT_TIMEBASE + _COMM_PAGE_RW_OFFSET);
		uint64_t old;
		do {
			old = *c_time_base;
		} while (!OSCompareAndSwap64(old, sleeptime, c_time_base));
#endif                
	}
}

void
commpage_update_mach_continuous_time_hw_offset(uint64_t offset)
{
	*((uint64_t *)(_COMM_PAGE_CONT_HW_TIMEBASE + _COMM_PAGE_RW_OFFSET)) = offset;
}

  
                                                
   
void
commpage_update_boottime(uint64_t value)
{
	if (commPagePtr) {
#ifdef __arm64__
		*((uint64_t *)(_COMM_PAGE_BOOTTIME_USEC + _COMM_PAGE_RW_OFFSET)) = value;
#else
		uint64_t *cp = (uint64_t *)(_COMM_PAGE_BOOTTIME_USEC + _COMM_PAGE_RW_OFFSET);
		uint64_t old_value;
		do {
			old_value = *cp;
		} while (!OSCompareAndSwap64(old_value, value, cp));
#endif                
	}
}

  
                                            
                                              
   
void
commpage_set_remotetime_params(double rate, uint64_t base_local_ts, uint64_t base_remote_ts)
{
	if (commPagePtr) {
#ifdef __arm64__
		struct bt_params *paramsp = (struct bt_params *)(_COMM_PAGE_REMOTETIME_PARAMS + _COMM_PAGE_RW_OFFSET);
		paramsp->base_local_ts = 0;
		__asm__ volatile ("dmb ish" ::: "memory");
		paramsp->rate = rate;
		paramsp->base_remote_ts = base_remote_ts;
		__asm__ volatile ("dmb ish" ::: "memory");
		paramsp->base_local_ts = base_local_ts;                                       
#else
		(void)rate;
		(void)base_local_ts;
		(void)base_remote_ts;
#endif                
	}
}


  
                                                                         
                                                                   
                                                                            
                      
  
                                                                            
                                                                        
                                                            
   
uint64_t
commpage_increment_cpu_quiescent_counter(void)
{
	if (!commPagePtr) {
		return 0;
	}

	uint64_t old_gen;

	_Atomic uint64_t *sched_gen = (_Atomic uint64_t *)(_COMM_PAGE_CPU_QUIESCENT_COUNTER +
	    _COMM_PAGE_RW_OFFSET);
	  
                                                                        
                                                                      
            
   
                                                                         
                                                             
    
#if __LP64__
	old_gen = os_atomic_load(sched_gen, relaxed);
	os_atomic_store(sched_gen, old_gen + 1, relaxed);
#else
	old_gen = atomic_fetch_add_explicit(sched_gen, 1, memory_order_relaxed);
#endif
	return old_gen;
}

  
                                                                  
   
void
commpage_update_dof(boolean_t enabled)
{
#if CONFIG_DTRACE
	*((uint8_t*)(_COMM_PAGE_DTRACE_DOF_ENABLED + _COMM_PAGE_RW_OFFSET)) = (enabled ? 1 : 0);
#else
	(void)enabled;
#endif
}

  
                                      
   
void
commpage_update_dyld_flags(uint64_t value)
{
	*((uint64_t*)(_COMM_PAGE_DYLD_FLAGS + _COMM_PAGE_RW_OFFSET)) = value;

}
