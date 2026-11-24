/*
 * Copyright (c) 2019 Apple Computer, Inc. All rights reserved.
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
   
                                                                         
                                                                 
                                                                                
   
#include <darwintest.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>
#include <mach/thread_status.h>
#include <sys/sysctl.h>
#include <inttypes.h>

#include "exc_helpers.h"

T_GLOBAL_META(T_META_RUN_CONCURRENTLY(true));

                                                                                   
#define FPCR_DIV_EXC 0x200
#define FPCR_INIT (0x0)

                                                                 
static volatile bool mach_exc_caught = false;

#ifdef __arm64__
static size_t
exc_arithmetic_handler(
	__unused mach_port_t task,
	__unused mach_port_t thread,
	exception_type_t type,
	mach_exception_data_t codes_64)
{
	                                                                             
	T_ASSERT_EQ(type, EXC_ARITHMETIC, "Caught an EXC_ARITHMETIC exception");

	                                                                        
	T_ASSERT_EQ(codes_64[0], (mach_exception_data_type_t)EXC_ARM_FP_DZ, "The subcode is EXC_ARM_FP_DZ (floating point divide-by-zero)");

	mach_exc_caught = true;
	return 4;
}
#endif

#define KERNEL_BOOTARGS_MAX_SIZE 1024
static char kernel_bootargs[KERNEL_BOOTARGS_MAX_SIZE];

T_DECL(armv8_fp_exception,
    "Test that ARMv8 floating point exceptions generate Mach exceptions, verify default FPCR value.")
{
#ifndef __arm64__
	T_SKIP("Running on non-arm64 target, skipping...");
#else
	mach_port_t exc_port = MACH_PORT_NULL;
	size_t kernel_bootargs_len;

	uint64_t fpcr = __builtin_arm_rsr64("FPCR");

	if (fpcr != FPCR_INIT) {
		T_FAIL("The floating point control register has a non-default value" "%" PRIx64, fpcr);
	}

	                                                                             
	uint64_t fpcr_divexc = fpcr | FPCR_DIV_EXC;
	__builtin_arm_wsr64("FPCR", fpcr_divexc);
#define DSB_ISH 0xb
	__builtin_arm_dsb(DSB_ISH);

	                                                                               
	if (__builtin_arm_rsr64("FPCR") != fpcr_divexc) {
		T_SKIP("Running on a device that doesn't support floating point exceptions, skipping...");
	}

	                                                    
	kernel_bootargs_len = sizeof(kernel_bootargs);
	kern_return_t kr = sysctlbyname("kern.bootargs", kernel_bootargs, &kernel_bootargs_len, NULL, 0);
	if (kr != 0) {
		T_SKIP("Could not get kernel bootargs, skipping...");
	}

	if (NULL == strstr(kernel_bootargs, "-fp_exceptions")) {
		T_SKIP("Floating-point exceptions are disabled, skipping...");
	}

	                                                                  
	exc_port = create_exception_port(EXC_MASK_ARITHMETIC);
	                                          
	run_exception_handler(exc_port, exc_arithmetic_handler);

	   
                                                                                 
   
                                                                               
                                                                             
                                                                     
    
	float a = 6.5f;
	float b = 0.0f;
	__asm volatile ("fdiv %s0, %s1, %s2" : "=w" (a) : "w" (a), "w" (b));

	if (mach_exc_caught) {
		T_PASS("The expected floating point divide-by-zero exception was caught!");
	} else {
		T_FAIL("The floating point divide-by-zero exception was not captured :(");
	}
#endif                
}
