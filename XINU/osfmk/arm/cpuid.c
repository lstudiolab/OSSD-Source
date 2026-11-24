/*
 * Copyright (c) 2007-2016 Apple Inc. All rights reserved.
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

#include <pexpert/pexpert.h>
#include <arm/cpuid.h>
#include <arm/cpuid_internal.h>
#include <vm/vm_page.h>
#include "proc_reg.h"

#include <libkern/section_keywords.h>

                                   
                                 

typedef struct {
	uint32_t
	    Ctype1:3,          
	    Ctype2:3,          
	    Ctype3:3,          
	    Ctypes:15,                        
	    LoC:3,                                 
	    LoU:3,                                   
	    RAZ:2;                           
} arm_cache_clidr_t;

typedef union {
	arm_cache_clidr_t bits;
	uint32_t          value;
} arm_cache_clidr_info_t;


typedef struct {
	uint32_t
	    LineSize:3,                                          
	    Assoc:10,                                    
	    NumSets:15,                                      
	    c_type:4;                         
} arm_cache_ccsidr_t;


typedef union {
	arm_cache_ccsidr_t bits;
	uint32_t           value;
} arm_cache_ccsidr_info_t;

             

static SECURITY_READ_ONLY_LATE(arm_cpu_info_t) cpuid_cpu_info;
static SECURITY_READ_ONLY_LATE(cache_info_t) cpuid_cache_info;

          

__private_extern__
void
do_cpuid(void)
{
	cpuid_cpu_info.value = machine_read_midr();
#if (__ARM_ARCH__ == 8)

#if defined(HAS_APPLE_PAC)
	cpuid_cpu_info.arm_info.arm_arch = CPU_ARCH_ARMv8E;
#else                             
	cpuid_cpu_info.arm_info.arm_arch = CPU_ARCH_ARMv8;
#endif                             

#elif (__ARM_ARCH__ == 7)
#ifdef __ARM_SUB_ARCH__
	cpuid_cpu_info.arm_info.arm_arch = __ARM_SUB_ARCH__;
#else                       
	cpuid_cpu_info.arm_info.arm_arch = CPU_ARCH_ARMv7;
#endif                       
#else                                                 
	                                                              
	if (cpuid_cpu_info.arm_info.arm_arch == CPU_ARCH_EXTENDED) {
		arm_isa_feat1_reg isa = machine_read_isa_feat1();

		  
                                                        
                                                               
     
		if (isa.field.sign_zero_ext_support == 0x2) {
			cpuid_cpu_info.arm_info.arm_arch = CPU_ARCH_ARMv6;
		}
	}
#endif                                                 
}

arm_cpu_info_t *
cpuid_info(void)
{
	return &cpuid_cpu_info;
}

int
cpuid_get_cpufamily(void)
{
	int cpufamily = 0;

	switch (cpuid_info()->arm_info.arm_implementor) {
	case CPU_VID_ARM:
		switch (cpuid_info()->arm_info.arm_part) {
		case CPU_PART_CORTEXA9:
			cpufamily = CPUFAMILY_ARM_14;
			break;
		case CPU_PART_CORTEXA8:
			cpufamily = CPUFAMILY_ARM_13;
			break;
		case CPU_PART_CORTEXA7:
			cpufamily = CPUFAMILY_ARM_15;
			break;
		case CPU_PART_1136JFS:
		case CPU_PART_1176JZFS:
			cpufamily = CPUFAMILY_ARM_11;
			break;
		case CPU_PART_926EJS:
		case CPU_PART_920T:
			cpufamily = CPUFAMILY_ARM_9;
			break;
		default:
			cpufamily = CPUFAMILY_UNKNOWN;
			break;
		}
		break;

	case CPU_VID_INTEL:
		cpufamily = CPUFAMILY_ARM_XSCALE;
		break;

	case CPU_VID_APPLE:
		switch (cpuid_info()->arm_info.arm_part) {
		case CPU_PART_TYPHOON:
		case CPU_PART_TYPHOON_CAPRI:
			cpufamily = CPUFAMILY_ARM_TYPHOON;
			break;
		case CPU_PART_TWISTER:
		case CPU_PART_TWISTER_ELBA_MALTA:
			cpufamily = CPUFAMILY_ARM_TWISTER;
			break;
		case CPU_PART_HURRICANE:
		case CPU_PART_HURRICANE_MYST:
			cpufamily = CPUFAMILY_ARM_HURRICANE;
			break;
		case CPU_PART_MONSOON:
		case CPU_PART_MISTRAL:
			cpufamily = CPUFAMILY_ARM_MONSOON_MISTRAL;
			break;
		case CPU_PART_VORTEX:
		case CPU_PART_TEMPEST:
		case CPU_PART_TEMPEST_M9:
		case CPU_PART_VORTEX_ARUBA:
		case CPU_PART_TEMPEST_ARUBA:
			cpufamily = CPUFAMILY_ARM_VORTEX_TEMPEST;
			break;
		case CPU_PART_LIGHTNING:
		case CPU_PART_THUNDER:
#ifndef RC_HIDE_XNU_FIRESTORM
		case CPU_PART_THUNDER_M10:
#endif
			cpufamily = CPUFAMILY_ARM_LIGHTNING_THUNDER;
			break;
#ifndef RC_HIDE_XNU_FIRESTORM
		case CPU_PART_FIRESTORM:
		case CPU_PART_ICESTORM:
		case CPU_PART_FIRESTORM_TONGA:
		case CPU_PART_ICESTORM_TONGA:
			cpufamily = CPUFAMILY_ARM_FIRESTORM_ICESTORM;
			break;
#endif
		default:
			cpufamily = CPUFAMILY_UNKNOWN;
			break;
		}
		break;

	default:
		cpufamily = CPUFAMILY_UNKNOWN;
		break;
	}

	return cpufamily;
}

int
cpuid_get_cpusubfamily(void)
{
	int cpusubfamily = CPUSUBFAMILY_UNKNOWN;

	if (cpuid_info()->arm_info.arm_implementor != CPU_VID_APPLE) {
		return cpusubfamily;
	}

	switch (cpuid_info()->arm_info.arm_part) {
	case CPU_PART_TYPHOON:
	case CPU_PART_TWISTER:
	case CPU_PART_HURRICANE:
	case CPU_PART_MONSOON:
	case CPU_PART_MISTRAL:
	case CPU_PART_VORTEX:
	case CPU_PART_TEMPEST:
	case CPU_PART_LIGHTNING:
	case CPU_PART_THUNDER:
#ifndef RC_HIDE_XNU_FIRESTORM
	case CPU_PART_FIRESTORM:
	case CPU_PART_ICESTORM:
#endif
		cpusubfamily = CPUSUBFAMILY_ARM_HP;
		break;
	case CPU_PART_TYPHOON_CAPRI:
	case CPU_PART_TWISTER_ELBA_MALTA:
	case CPU_PART_HURRICANE_MYST:
	case CPU_PART_VORTEX_ARUBA:
	case CPU_PART_TEMPEST_ARUBA:
#ifndef RC_HIDE_XNU_FIRESTORM
	case CPU_PART_FIRESTORM_TONGA:
	case CPU_PART_ICESTORM_TONGA:
#endif
		cpusubfamily = CPUSUBFAMILY_ARM_HG;
		break;
	case CPU_PART_TEMPEST_M9:
#ifndef RC_HIDE_XNU_FIRESTORM
	case CPU_PART_THUNDER_M10:
#endif
		cpusubfamily = CPUSUBFAMILY_ARM_M;
		break;
	default:
		cpusubfamily = CPUFAMILY_UNKNOWN;
		break;
	}

	return cpusubfamily;
}

void
do_debugid(void)
{
	machine_do_debugid();
}

arm_debug_info_t *
arm_debug_info(void)
{
	return machine_arm_debug_info();
}

void
do_mvfpid(void)
{
	return machine_do_mvfpid();
}

arm_mvfp_info_t
*
arm_mvfp_info(void)
{
	return machine_arm_mvfp_info();
}

void
do_cacheid(void)
{
	arm_cache_clidr_info_t arm_cache_clidr_info;
	arm_cache_ccsidr_info_t arm_cache_ccsidr_info;

	arm_cache_clidr_info.value = machine_read_clidr();


	                                  

	machine_write_csselr(CSSELR_L1, CSSELR_DATA_UNIFIED);
	arm_cache_ccsidr_info.value = machine_read_ccsidr();

	cpuid_cache_info.c_unified = (arm_cache_clidr_info.bits.Ctype1 == 0x4) ? 1 : 0;

	switch (arm_cache_ccsidr_info.bits.c_type) {
	case 0x1:
		cpuid_cache_info.c_type = CACHE_WRITE_ALLOCATION;
		break;
	case 0x2:
		cpuid_cache_info.c_type = CACHE_READ_ALLOCATION;
		break;
	case 0x4:
		cpuid_cache_info.c_type = CACHE_WRITE_BACK;
		break;
	case 0x8:
		cpuid_cache_info.c_type = CACHE_WRITE_THROUGH;
		break;
	default:
		cpuid_cache_info.c_type = CACHE_UNKNOWN;
	}

	cpuid_cache_info.c_linesz = 4 * (1 << (arm_cache_ccsidr_info.bits.LineSize + 2));
	cpuid_cache_info.c_assoc = (arm_cache_ccsidr_info.bits.Assoc + 1);

	                  
	cpuid_cache_info.c_isize = (arm_cache_ccsidr_info.bits.NumSets + 1) * cpuid_cache_info.c_linesz * cpuid_cache_info.c_assoc;

	                  
	cpuid_cache_info.c_dsize = (arm_cache_ccsidr_info.bits.NumSets + 1) * cpuid_cache_info.c_linesz * cpuid_cache_info.c_assoc;


	if ((arm_cache_clidr_info.bits.Ctype3 == 0x4) ||
	    (arm_cache_clidr_info.bits.Ctype2 == 0x4) || (arm_cache_clidr_info.bits.Ctype2 == 0x2)) {
		if (arm_cache_clidr_info.bits.Ctype3 == 0x4) {
			                                                         
                                                                            
			machine_write_csselr(CSSELR_L3, CSSELR_DATA_UNIFIED);
		} else {
			                          
			machine_write_csselr(CSSELR_L2, CSSELR_DATA_UNIFIED);
		}
		arm_cache_ccsidr_info.value = machine_read_ccsidr();

		cpuid_cache_info.c_linesz = 4 * (1 << (arm_cache_ccsidr_info.bits.LineSize + 2));
		cpuid_cache_info.c_assoc = (arm_cache_ccsidr_info.bits.Assoc + 1);
		cpuid_cache_info.c_l2size = (arm_cache_ccsidr_info.bits.NumSets + 1) * cpuid_cache_info.c_linesz * cpuid_cache_info.c_assoc;
		cpuid_cache_info.c_inner_cache_size = cpuid_cache_info.c_dsize;
		cpuid_cache_info.c_bulksize_op = cpuid_cache_info.c_l2size;

		                                                             
                                                              
                                                                
                                                             
                                                             
                                                             
                                                              
                                                              
                                                               
                                                                 
    
                                                               
                            
     
		vm_cache_geometry_colors = ((arm_cache_ccsidr_info.bits.NumSets + 1) * cpuid_cache_info.c_linesz) / PAGE_SIZE;
		kprintf(" vm_cache_geometry_colors: %d\n", vm_cache_geometry_colors);
	} else {
		cpuid_cache_info.c_l2size = 0;

		cpuid_cache_info.c_inner_cache_size = cpuid_cache_info.c_dsize;
		cpuid_cache_info.c_bulksize_op = cpuid_cache_info.c_dsize;
	}

	if (cpuid_cache_info.c_unified == 0) {
		machine_write_csselr(CSSELR_L1, CSSELR_INSTR);
		arm_cache_ccsidr_info.value = machine_read_ccsidr();
		uint32_t c_linesz = 4 * (1 << (arm_cache_ccsidr_info.bits.LineSize + 2));
		uint32_t c_assoc = (arm_cache_ccsidr_info.bits.Assoc + 1);
		                  
		cpuid_cache_info.c_isize = (arm_cache_ccsidr_info.bits.NumSets + 1) * c_linesz * c_assoc;
	}

	kprintf("%s() - %u bytes %s cache (I:%u D:%u (%s)), %u-way assoc, %u bytes/line\n",
	    __FUNCTION__,
	    cpuid_cache_info.c_dsize + cpuid_cache_info.c_isize,
	    ((cpuid_cache_info.c_type == CACHE_WRITE_BACK) ? "WB" :
	    (cpuid_cache_info.c_type == CACHE_WRITE_THROUGH ? "WT" : "Unknown")),
	    cpuid_cache_info.c_isize,
	    cpuid_cache_info.c_dsize,
	    (cpuid_cache_info.c_unified) ? "unified" : "separate",
	    cpuid_cache_info.c_assoc,
	    cpuid_cache_info.c_linesz);
}

cache_info_t   *
cache_info(void)
{
	return &cpuid_cache_info;
}
