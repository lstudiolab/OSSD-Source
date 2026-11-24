/*
 * Copyright (c) 2003-2020 Apple Inc. All rights reserved.
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

  
                                                                       
  
                                                                          
                                                          
  
                                                                    
                                                                      
                                                                   
                                                            
                                                          
  
                                                                     
                                                                  
                                                                  
  
                                                     
   

#include <mach/mach_types.h>
#include <mach/machine.h>
#include <mach/vm_map.h>
#include <mach/mach_vm.h>
#include <mach/machine.h>
#include <i386/cpuid.h>
#include <i386/tsc.h>
#include <i386/rtclock_protos.h>
#include <i386/cpu_data.h>
#include <i386/machine_routines.h>
#include <i386/misc_protos.h>
#include <i386/cpuid.h>
#include <machine/cpu_capabilities.h>
#include <machine/commpage.h>
#include <machine/pmap.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <stdatomic.h>

#include <ipc/ipc_port.h>

#include <kern/page_decrypt.h>
#include <kern/processor.h>

#include <sys/kdebug.h>

#if CONFIG_ATM
#include <atm/atm_internal.h>
#endif

                                                           
extern  commpage_descriptor*    commpage_32_routines[];
extern  commpage_descriptor*    commpage_64_routines[];

extern vm_map_t commpage32_map;                                        
extern vm_map_t commpage64_map;                                        
extern vm_map_t commpage_text32_map;                                           
extern vm_map_t commpage_text64_map;                                           


char    *commPagePtr32 = NULL;                                                          
char    *commPagePtr64 = NULL;                                      
char    *commPageTextPtr32 = NULL;                                                      
char    *commPageTextPtr64 = NULL;                                  

uint64_t     _cpu_capabilities = 0;                                    

typedef uint32_t commpage_address_t;

static commpage_address_t       next;                                         

static char    *commPagePtr;                                                                       
static commpage_address_t       commPageBaseOffset;                                                                                        

static  commpage_time_data      *time_data32 = NULL;
static  commpage_time_data      *time_data64 = NULL;
static  new_commpage_timeofday_data_t *gtod_time_data32 = NULL;
static  new_commpage_timeofday_data_t *gtod_time_data64 = NULL;


decl_simple_lock_data(static, commpage_active_cpus_lock);

                                                                    
                                                 
                  
                                   
                                                             
   

static  void*
commpage_allocate(
	vm_map_t        submap,                                                    
	size_t          area_used,                                                                 
	vm_prot_t       uperm)
{
	vm_offset_t     kernel_addr = 0;                                            
	vm_offset_t     zero = 0;
	vm_size_t       size = area_used;                                 
	vm_map_entry_t  entry;
	ipc_port_t      handle;
	kern_return_t   kr;
	vm_map_kernel_flags_t vmk_flags;

	if (submap == NULL) {
		panic("commpage submap is null");
	}

	kr = vm_map_kernel(kernel_map,
	    &kernel_addr,
	    area_used,
	    0,
	    VM_FLAGS_ANYWHERE,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    VM_KERN_MEMORY_OSFMK,
	    NULL,
	    0,
	    FALSE,
	    VM_PROT_ALL,
	    VM_PROT_ALL,
	    VM_INHERIT_NONE);
	if (kr != KERN_SUCCESS) {
		panic("cannot allocate commpage %d", kr);
	}

	if ((kr = vm_map_wire_kernel(kernel_map,
	    kernel_addr,
	    kernel_addr + area_used,
	    VM_PROT_DEFAULT, VM_KERN_MEMORY_OSFMK,
	    FALSE))) {
		panic("cannot wire commpage: %d", kr);
	}

	  
                                                                                          
                                                                                         
                                                                                          
                                  
   
                                                                                 
    
	if (!(kr = vm_map_lookup_entry( kernel_map, vm_map_trunc_page(kernel_addr, VM_MAP_PAGE_MASK(kernel_map)), &entry) || entry->is_sub_map)) {
		panic("cannot find commpage entry %d", kr);
	}
	VME_OBJECT(entry)->copy_strategy = MEMORY_OBJECT_COPY_NONE;

	if ((kr = mach_make_memory_entry( kernel_map,                        
	    &size,                                             
	    kernel_addr,                                                                 
	    uperm,                                                         
	    &handle,                                                                       
	    NULL ))) {                                                                 
		panic("cannot make entry for commpage %d", kr);
	}

	vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
	if (uperm == (VM_PROT_READ | VM_PROT_EXECUTE)) {
		  
                                                            
                                                                
          
     
		vmk_flags.vmkf_map_jit = TRUE;
	}

	kr = vm_map_64_kernel(
		submap,                                              
		&zero,                                                          
		area_used,                     
		0,                             
		VM_FLAGS_FIXED,                                                 
		vmk_flags,
		VM_KERN_MEMORY_NONE,
		handle,                                                         
		0,                                                              
		FALSE,                         
		uperm,                                                        
		uperm,                                   
		VM_INHERIT_SHARE);                    
	if (kr != KERN_SUCCESS) {
		panic("cannot map commpage %d", kr);
	}

	ipc_port_release(handle);
	                                                              
                                                               
                                                    
    
	kr = vm_protect(kernel_map, kernel_addr, area_used, FALSE, VM_PROT_READ | VM_PROT_WRITE);
	assert(kr == KERN_SUCCESS);

	return (void*)(intptr_t)kernel_addr;                                                    
}

                                                      

static void*
commpage_addr_of(
	commpage_address_t     addr_at_runtime )
{
	return (void*) ((uintptr_t)commPagePtr + (addr_at_runtime - commPageBaseOffset));
}

  
                                                                                          
         
   
static void*
commpage_specific_addr_of(char *commPageBase, commpage_address_t addr_at_runtime)
{
	  
                                                                          
                            
    
	return (void*) ((uintptr_t)commPageBase + (addr_at_runtime - _COMM_PAGE32_BASE_ADDRESS));
}

                                                              
                                                
   
static int
commpage_cpus( void )
{
	unsigned int cpus;

	cpus = ml_wait_max_cpus();                                             

	if (cpus == 0) {
		panic("commpage cpus==0");
	}
	if (cpus > 0xFF) {
		cpus = 0xFF;
	}

	return cpus;
}

                                                                            

static void
commpage_init_cpu_capabilities( void )
{
	uint64_t bits;
	int cpus;
	ml_cpu_info_t cpu_info;

	bits = 0;
	ml_cpu_get_info(&cpu_info);

	switch (cpu_info.vector_unit) {
	case 9:
		bits |= kHasAVX1_0;
		OS_FALLTHROUGH;
	case 8:
		bits |= kHasSSE4_2;
		OS_FALLTHROUGH;
	case 7:
		bits |= kHasSSE4_1;
		OS_FALLTHROUGH;
	case 6:
		bits |= kHasSupplementalSSE3;
		OS_FALLTHROUGH;
	case 5:
		bits |= kHasSSE3;
		OS_FALLTHROUGH;
	case 4:
		bits |= kHasSSE2;
		OS_FALLTHROUGH;
	case 3:
		bits |= kHasSSE;
		OS_FALLTHROUGH;
	case 2:
		bits |= kHasMMX;
		OS_FALLTHROUGH;
	default:
		break;
	}
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

	bits |= (cpus << kNumCPUsShift);

	bits |= kFastThreadLocalStorage;                             

#define setif(_bits, _bit, _condition) \
	if (_condition) _bits |= _bit

	setif(bits, kUP, cpus == 1);
	setif(bits, k64Bit, cpu_mode_is64bit());
	setif(bits, kSlow, tscFreq <= SLOW_TSC_THRESHOLD);

	setif(bits, kHasAES, cpuid_features() &
	    CPUID_FEATURE_AES);
	setif(bits, kHasF16C, cpuid_features() &
	    CPUID_FEATURE_F16C);
	setif(bits, kHasRDRAND, cpuid_features() &
	    CPUID_FEATURE_RDRAND);
	setif(bits, kHasFMA, cpuid_features() &
	    CPUID_FEATURE_FMA);

	setif(bits, kHasBMI1, cpuid_leaf7_features() &
	    CPUID_LEAF7_FEATURE_BMI1);
	setif(bits, kHasBMI2, cpuid_leaf7_features() &
	    CPUID_LEAF7_FEATURE_BMI2);
	                                                                        
	if (cpuid_wa_required(CPU_INTEL_TSXFA) & CWA_OFF) {
		setif(bits, kHasRTM, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_RTM);
		setif(bits, kHasHLE, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_HLE);
	}
	setif(bits, kHasAVX2_0, cpuid_leaf7_features() &
	    CPUID_LEAF7_FEATURE_AVX2);
	setif(bits, kHasRDSEED, cpuid_leaf7_features() &
	    CPUID_LEAF7_FEATURE_RDSEED);
	setif(bits, kHasADX, cpuid_leaf7_features() &
	    CPUID_LEAF7_FEATURE_ADX);

#if 0                                              
	setif(bits, kHasMPX, cpuid_leaf7_features() &
	    CPUID_LEAF7_FEATURE_MPX);
	setif(bits, kHasSGX, cpuid_leaf7_features() &
	    CPUID_LEAF7_FEATURE_SGX);
#endif

	if (ml_fpu_avx512_enabled()) {
		setif(bits, kHasAVX512F, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_AVX512F);
		setif(bits, kHasAVX512CD, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_AVX512CD);
		setif(bits, kHasAVX512DQ, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_AVX512DQ);
		setif(bits, kHasAVX512BW, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_AVX512BW);
		setif(bits, kHasAVX512VL, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_AVX512VL);
		setif(bits, kHasAVX512IFMA, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_AVX512IFMA);
		setif(bits, kHasAVX512VBMI, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_AVX512VBMI);
		setif(bits, kHasVAES, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_VAES);
		setif(bits, kHasVPCLMULQDQ, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_VPCLMULQDQ);
		setif(bits, kHasAVX512VNNI, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_AVX512VNNI);
		setif(bits, kHasAVX512BITALG, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_AVX512BITALG);
		setif(bits, kHasAVX512VPOPCNTDQ, cpuid_leaf7_features() &
		    CPUID_LEAF7_FEATURE_AVX512VPCDQ);
	}

	uint64_t misc_enable = rdmsr64(MSR_IA32_MISC_ENABLE);
	setif(bits, kHasENFSTRG, (misc_enable & 1ULL) &&
	    (cpuid_leaf7_features() &
	    CPUID_LEAF7_FEATURE_ERMS));

	_cpu_capabilities = bits;                                                           
}

                                                                          
                                             
   
static void
commpage_mach_approximate_time_init(void)
{
	char *cp = commPagePtr32;
	uint8_t supported;

#ifdef CONFIG_MACH_APPROXIMATE_TIME
	supported = 1;
#else
	supported = 0;
#endif
	if (cp) {
		cp += (_COMM_PAGE_APPROX_TIME_SUPPORTED - _COMM_PAGE32_BASE_ADDRESS);
		*(boolean_t *)cp = supported;
	}

	cp = commPagePtr64;
	if (cp) {
		cp += (_COMM_PAGE_APPROX_TIME_SUPPORTED - _COMM_PAGE32_START_ADDRESS);
		*(boolean_t *)cp = supported;
	}
	commpage_update_mach_approximate_time(0);
}

static void
commpage_mach_continuous_time_init(void)
{
	commpage_update_mach_continuous_time(0);
}

static void
commpage_boottime_init(void)
{
	clock_sec_t secs;
	clock_usec_t microsecs;
	clock_get_boottime_microtime(&secs, &microsecs);
	commpage_update_boottime(secs * USEC_PER_SEC + microsecs);
}

uint64_t
_get_cpu_capabilities(void)
{
	return _cpu_capabilities;
}

                              

static void
commpage_stuff(
	commpage_address_t  address,
	const void  *source,
	int         length  )
{
	void        *dest = commpage_addr_of(address);

	if (address < next) {
		panic("commpage overlap at address 0x%p, 0x%x < 0x%x", dest, address, next);
	}

	bcopy(source, dest, length);

	next = address + length;
}

  
                                                                  
   
static void
commpage_update(commpage_address_t address, const void *source, int length)
{
	void *dest = commpage_specific_addr_of(commPagePtr32, address);
	bcopy(source, dest, length);

	dest = commpage_specific_addr_of(commPagePtr64, address);
	bcopy(source, dest, length);
}

void
commpage_post_ucode_update(void)
{
	commpage_init_cpu_capabilities();
	commpage_update(_COMM_PAGE_CPU_CAPABILITIES64, &_cpu_capabilities, sizeof(_cpu_capabilities));
	commpage_update(_COMM_PAGE_CPU_CAPABILITIES, &_cpu_capabilities, sizeof(uint32_t));
}

                                                               
   
static void
commpage_stuff_routine(
	commpage_descriptor *rd     )
{
	commpage_stuff(rd->commpage_address, rd->code_address, rd->code_length);
}


                                                             
   

static void
commpage_populate_one(
	vm_map_t        submap,                                           
	char **         kernAddressPtr,                                    
	size_t          area_used,                                                         
	commpage_address_t base_offset,                                  
	commpage_time_data** time_data,                                
	new_commpage_timeofday_data_t** gtod_time_data,                                          
	const char*     signature,                                               
	vm_prot_t       uperm)
{
	uint8_t         c1;
	uint16_t        c2;
	uint64_t        c8;
	uint32_t        cfamily;
	short   version = _COMM_PAGE_THIS_VERSION;

	next = 0;
	commPagePtr = (char *)commpage_allocate( submap, (vm_size_t) area_used, uperm );
	*kernAddressPtr = commPagePtr;                                                                       
	commPageBaseOffset = base_offset;

	*time_data = commpage_addr_of( _COMM_PAGE_TIME_DATA_START );
	*gtod_time_data = commpage_addr_of( _COMM_PAGE_NEWTIMEOFDAY_DATA );

	                                                                         
                                                                 
                                                                       
                               
    
	commpage_stuff(_COMM_PAGE_SIGNATURE, signature, (int)MIN(_COMM_PAGE_SIGNATURELEN, strlen(signature)));
	commpage_stuff(_COMM_PAGE_CPU_CAPABILITIES64, &_cpu_capabilities, sizeof(_cpu_capabilities));
	commpage_stuff(_COMM_PAGE_VERSION, &version, sizeof(short));
	commpage_stuff(_COMM_PAGE_CPU_CAPABILITIES, &_cpu_capabilities, sizeof(uint32_t));

	c2 = 32;            
	if (_cpu_capabilities & kCache64) {
		c2 = 64;
	} else if (_cpu_capabilities & kCache128) {
		c2 = 128;
	}
	commpage_stuff(_COMM_PAGE_CACHE_LINESIZE, &c2, 2);

	                                                 
	c1 = machine_info.physical_cpu_max;
	commpage_stuff(_COMM_PAGE_PHYSICAL_CPUS, &c1, 1);
	c1 = machine_info.logical_cpu_max;
	commpage_stuff(_COMM_PAGE_LOGICAL_CPUS, &c1, 1);

	c8 = ml_cpu_cache_size(0);
	commpage_stuff(_COMM_PAGE_MEMORY_SIZE, &c8, 8);

	cfamily = cpuid_info()->cpuid_cpufamily;
	commpage_stuff(_COMM_PAGE_CPUFAMILY, &cfamily, 4);
	c1 = PAGE_SHIFT;
	commpage_stuff(_COMM_PAGE_KERNEL_PAGE_SHIFT, &c1, 1);
	commpage_stuff(_COMM_PAGE_USER_PAGE_SHIFT_64, &c1, 1);

	if (next > _COMM_PAGE_END) {
		panic("commpage overflow: next = 0x%08x, commPagePtr = 0x%p", next, commPagePtr);
	}
}


                                                                         
                                                   
  
                                                                    
                                 
   

void
commpage_populate( void )
{
	commpage_init_cpu_capabilities();

	commpage_populate_one(  commpage32_map,
	    &commPagePtr32,
	    _COMM_PAGE32_AREA_USED,
	    _COMM_PAGE32_BASE_ADDRESS,
	    &time_data32,
	    &gtod_time_data32,
	    _COMM_PAGE32_SIGNATURE_STRING,
	    VM_PROT_READ);
#ifndef __LP64__
	pmap_commpage32_init((vm_offset_t) commPagePtr32, _COMM_PAGE32_BASE_ADDRESS,
	    _COMM_PAGE32_AREA_USED / INTEL_PGBYTES);
#endif
	time_data64 = time_data32;                                                                  
	gtod_time_data64 = gtod_time_data32;

	if (_cpu_capabilities & k64Bit) {
		commpage_populate_one(  commpage64_map,
		    &commPagePtr64,
		    _COMM_PAGE64_AREA_USED,
		    _COMM_PAGE32_START_ADDRESS,                                                                                     
		    &time_data64,
		    &gtod_time_data64,
		    _COMM_PAGE64_SIGNATURE_STRING,
		    VM_PROT_READ);
#ifndef __LP64__
		pmap_commpage64_init((vm_offset_t) commPagePtr64, _COMM_PAGE64_BASE_ADDRESS,
		    _COMM_PAGE64_AREA_USED / INTEL_PGBYTES);
#endif
	}

	simple_lock_init(&commpage_active_cpus_lock, 0);

	commpage_update_active_cpus();
	commpage_mach_approximate_time_init();
	commpage_mach_continuous_time_init();
	commpage_boottime_init();
	rtc_nanotime_init_commpage();
	commpage_update_kdebug_state();
#if CONFIG_ATM
	commpage_update_atm_diagnostic_config(atm_get_diagnostic_config());
#endif
}

                                                            
                                                   
   
void
commpage_text_populate( void )
{
	commpage_descriptor **rd;

	next = 0;
	commPagePtr = (char *) commpage_allocate(commpage_text32_map, (vm_size_t) _COMM_PAGE_TEXT_AREA_USED, VM_PROT_READ | VM_PROT_EXECUTE);
	commPageTextPtr32 = commPagePtr;

	char *cptr = commPagePtr;
	int i = 0;
	for (; i < _COMM_PAGE_TEXT_AREA_USED; i++) {
		cptr[i] = 0xCC;
	}

	commPageBaseOffset = _COMM_PAGE_TEXT_START;
	for (rd = commpage_32_routines; *rd != NULL; rd++) {
		commpage_stuff_routine(*rd);
	}

#ifndef __LP64__
	pmap_commpage32_init((vm_offset_t) commPageTextPtr32, _COMM_PAGE_TEXT_START,
	    _COMM_PAGE_TEXT_AREA_USED / INTEL_PGBYTES);
#endif

	if (_cpu_capabilities & k64Bit) {
		next = 0;
		commPagePtr = (char *) commpage_allocate(commpage_text64_map, (vm_size_t) _COMM_PAGE_TEXT_AREA_USED, VM_PROT_READ | VM_PROT_EXECUTE);
		commPageTextPtr64 = commPagePtr;

		cptr = commPagePtr;
		for (i = 0; i < _COMM_PAGE_TEXT_AREA_USED; i++) {
			cptr[i] = 0xCC;
		}

		for (rd = commpage_64_routines; *rd != NULL; rd++) {
			commpage_stuff_routine(*rd);
		}

#ifndef __LP64__
		pmap_commpage64_init((vm_offset_t) commPageTextPtr64, _COMM_PAGE_TEXT_START,
		    _COMM_PAGE_TEXT_AREA_USED / INTEL_PGBYTES);
#endif
	}

	if (next > _COMM_PAGE_TEXT_END) {
		panic("commpage text overflow: next=0x%08x, commPagePtr=%p", next, commPagePtr);
	}
}

                                        
  
                                                                     
   

void
commpage_set_nanotime(
	uint64_t        tsc_base,
	uint64_t        ns_base,
	uint32_t        scale,
	uint32_t        shift )
{
	commpage_time_data      *p32 = time_data32;
	commpage_time_data      *p64 = time_data64;
	static uint32_t generation = 0;
	uint32_t        next_gen;

	if (p32 == NULL) {                                                      
		return;
	}

	if (generation != p32->nt_generation) {
		panic("nanotime trouble 1");                                 
	}
	if (ns_base < p32->nt_ns_base) {
		panic("nanotime trouble 2");
	}
	if ((shift != 0) && ((_cpu_capabilities & kSlow) == 0)) {
		panic("nanotime trouble 3");
	}

	next_gen = ++generation;
	if (next_gen == 0) {
		next_gen = ++generation;
	}

	p32->nt_generation = 0;                                                            
	p64->nt_generation = 0;

	p32->nt_tsc_base = tsc_base;
	p64->nt_tsc_base = tsc_base;

	p32->nt_ns_base = ns_base;
	p64->nt_ns_base = ns_base;

	p32->nt_scale = scale;
	p64->nt_scale = scale;

	p32->nt_shift = shift;
	p64->nt_shift = shift;

	p32->nt_generation = next_gen;                          
	p64->nt_generation = next_gen;
}

                                                                                 
                                                                                    
                                
  
                                                                      
   

void
commpage_set_timestamp(
	uint64_t        abstime,
	uint64_t        sec,
	uint64_t        frac,
	uint64_t        scale,
	uint64_t        tick_per_sec)
{
	new_commpage_timeofday_data_t   *p32 = gtod_time_data32;
	new_commpage_timeofday_data_t   *p64 = gtod_time_data64;

	p32->TimeStamp_tick = 0x0ULL;
	p64->TimeStamp_tick = 0x0ULL;

	p32->TimeStamp_sec = sec;
	p64->TimeStamp_sec = sec;

	p32->TimeStamp_frac = frac;
	p64->TimeStamp_frac = frac;

	p32->Ticks_scale = scale;
	p64->Ticks_scale = scale;

	p32->Ticks_per_sec = tick_per_sec;
	p64->Ticks_per_sec = tick_per_sec;

	p32->TimeStamp_tick = abstime;
	p64->TimeStamp_tick = abstime;
}

                                                                                                  

void
commpage_set_memory_pressure(
	unsigned int    pressure )
{
	char        *cp;
	uint32_t    *ip;

	cp = commPagePtr32;
	if (cp) {
		cp += (_COMM_PAGE_MEMORY_PRESSURE - _COMM_PAGE32_BASE_ADDRESS);
		ip = (uint32_t*) (void *) cp;
		*ip = (uint32_t) pressure;
	}

	cp = commPagePtr64;
	if (cp) {
		cp += (_COMM_PAGE_MEMORY_PRESSURE - _COMM_PAGE32_START_ADDRESS);
		ip = (uint32_t*) (void *) cp;
		*ip = (uint32_t) pressure;
	}
}

                                                          
void
commpage_update_active_cpus(void)
{
	char        *cp;
	volatile uint8_t    *ip;

	                                                  
	if (!commPagePtr32) {
		return;
	}

	simple_lock(&commpage_active_cpus_lock, LCK_GRP_NULL);

	cp = commPagePtr32;
	cp += (_COMM_PAGE_ACTIVE_CPUS - _COMM_PAGE32_BASE_ADDRESS);
	ip = (volatile uint8_t*) cp;
	*ip = (uint8_t) processor_avail_count_user;

	cp = commPagePtr64;
	if (cp) {
		cp += (_COMM_PAGE_ACTIVE_CPUS - _COMM_PAGE32_START_ADDRESS);
		ip = (volatile uint8_t*) cp;
		*ip = (uint8_t) processor_avail_count_user;
	}

	simple_unlock(&commpage_active_cpus_lock);
}

  
                                                                             
                                                                               
                                 
  
                                                                             
                                                                        
   
void
commpage_update_kdebug_state(void)
{
	volatile uint32_t *saved_data_ptr;
	char *cp;

	cp = commPagePtr32;
	if (cp) {
		cp += (_COMM_PAGE_KDEBUG_ENABLE - _COMM_PAGE32_BASE_ADDRESS);
		saved_data_ptr = (volatile uint32_t *)cp;
		*saved_data_ptr = kdebug_commpage_state();
	}

	cp = commPagePtr64;
	if (cp) {
		cp += (_COMM_PAGE_KDEBUG_ENABLE - _COMM_PAGE32_START_ADDRESS);
		saved_data_ptr = (volatile uint32_t *)cp;
		*saved_data_ptr = kdebug_commpage_state();
	}
}

                                     
void
commpage_update_atm_diagnostic_config(uint32_t diagnostic_config)
{
	volatile uint32_t *saved_data_ptr;
	char *cp;

	cp = commPagePtr32;
	if (cp) {
		cp += (_COMM_PAGE_ATM_DIAGNOSTIC_CONFIG - _COMM_PAGE32_BASE_ADDRESS);
		saved_data_ptr = (volatile uint32_t *)cp;
		*saved_data_ptr = diagnostic_config;
	}

	cp = commPagePtr64;
	if (cp) {
		cp += (_COMM_PAGE_ATM_DIAGNOSTIC_CONFIG - _COMM_PAGE32_START_ADDRESS);
		saved_data_ptr = (volatile uint32_t *)cp;
		*saved_data_ptr = diagnostic_config;
	}
}

  
                                                                  
   
void
commpage_update_dof(boolean_t enabled)
{
#if CONFIG_DTRACE
	char *cp;

	cp = commPagePtr32;
	if (cp) {
		cp += (_COMM_PAGE_DTRACE_DOF_ENABLED - _COMM_PAGE32_BASE_ADDRESS);
		*cp = (enabled ? 1 : 0);
	}

	cp = commPagePtr64;
	if (cp) {
		cp += (_COMM_PAGE_DTRACE_DOF_ENABLED - _COMM_PAGE32_START_ADDRESS);
		*cp = (enabled ? 1 : 0);
	}
#else
	(void)enabled;
#endif
}


  
                                      
   
void
commpage_update_dyld_flags(uint64_t value)
{
	char *cp;

	cp = commPagePtr32;
	if (cp) {
		cp += (_COMM_PAGE_DYLD_FLAGS - _COMM_PAGE32_BASE_ADDRESS);
		*(uint64_t *)cp = value;
	}

	cp = commPagePtr64;
	if (cp) {
		cp += (_COMM_PAGE_DYLD_FLAGS - _COMM_PAGE32_BASE_ADDRESS);
		*(uint64_t *)cp = value;
	}
}


  
                                                                        
   

void
commpage_update_mach_approximate_time(uint64_t abstime)
{
#ifdef CONFIG_MACH_APPROXIMATE_TIME
	uint64_t saved_data;
	char *cp;

	cp = commPagePtr32;
	if (cp) {
		cp += (_COMM_PAGE_APPROX_TIME - _COMM_PAGE32_BASE_ADDRESS);
		saved_data = atomic_load_explicit((_Atomic uint64_t *)(uintptr_t)cp, memory_order_relaxed);
		if (saved_data < abstime) {
			                                                       
                                                          
                                                         
                     
			atomic_compare_exchange_strong_explicit((_Atomic uint64_t *)(uintptr_t)cp,
			    &saved_data, abstime, memory_order_relaxed, memory_order_relaxed);
		}
	}
	cp = commPagePtr64;
	if (cp) {
		cp += (_COMM_PAGE_APPROX_TIME - _COMM_PAGE32_START_ADDRESS);
		saved_data = atomic_load_explicit((_Atomic uint64_t *)(uintptr_t)cp, memory_order_relaxed);
		if (saved_data < abstime) {
			                                                       
                                                          
                                                         
                     
			atomic_compare_exchange_strong_explicit((_Atomic uint64_t *)(uintptr_t)cp,
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
	char *cp;
	cp = commPagePtr32;
	if (cp) {
		cp += (_COMM_PAGE_CONT_TIMEBASE - _COMM_PAGE32_START_ADDRESS);
		*(uint64_t *)cp = sleeptime;
	}

	cp = commPagePtr64;
	if (cp) {
		cp += (_COMM_PAGE_CONT_TIMEBASE - _COMM_PAGE32_START_ADDRESS);
		*(uint64_t *)cp = sleeptime;
	}
}

void
commpage_update_boottime(uint64_t boottime)
{
	char *cp;
	cp = commPagePtr32;
	if (cp) {
		cp += (_COMM_PAGE_BOOTTIME_USEC - _COMM_PAGE32_START_ADDRESS);
		*(uint64_t *)cp = boottime;
	}

	cp = commPagePtr64;
	if (cp) {
		cp += (_COMM_PAGE_BOOTTIME_USEC - _COMM_PAGE32_START_ADDRESS);
		*(uint64_t *)cp = boottime;
	}
}


extern user32_addr_t commpage_text32_location;
extern user64_addr_t commpage_text64_location;

                                                                          

uint32_t
commpage_is_in_pfz32(uint32_t addr32)
{
	if ((addr32 >= (commpage_text32_location + _COMM_TEXT_PFZ_START_OFFSET))
	    && (addr32 < (commpage_text32_location + _COMM_TEXT_PFZ_END_OFFSET))) {
		return 1;
	} else {
		return 0;
	}
}

uint32_t
commpage_is_in_pfz64(addr64_t addr64)
{
	if ((addr64 >= (commpage_text64_location + _COMM_TEXT_PFZ_START_OFFSET))
	    && (addr64 < (commpage_text64_location + _COMM_TEXT_PFZ_END_OFFSET))) {
		return 1;
	} else {
		return 0;
	}
}
