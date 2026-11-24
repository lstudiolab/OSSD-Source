/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
 */
/*
 *	Copyright (C) 1990,  NeXT, Inc.
 *
 *	File:	next/kern_machdep.c
 *	Author:	John Seamons
 *
 *	Machine-specific kernel routines.
 */

#include        <sys/types.h>
#include        <mach/machine.h>
#include        <kern/cpu_number.h>
#include        <libkern/libkern.h>
#include        <machine/exec.h>
#include        <pexpert/arm64/board_config.h>

#if __arm64__
static cpu_subtype_t cpu_subtype32(void);
#endif                

#if __arm64__
  
                                                                          
                                                           
   
static cpu_subtype_t
cpu_subtype32()
{
	switch (cpu_subtype()) {
	case CPU_SUBTYPE_ARM64_V8:
		return CPU_SUBTYPE_ARM_V8;
	default:
		return 0;
	}
}

static int
grade_arm64e_binary(cpu_subtype_t execfeatures)
{
#if XNU_TARGET_OS_IOS
	  
                                                                      
                                     
    
	if ((execfeatures & CPU_SUBTYPE_PTRAUTH_ABI) == 0) {
#if DEBUG || DEVELOPMENT
		printf("%s: arm64e prerelease ABI cannot be used with this kernel\n", __func__);
#endif                           
		return 0;
	}
#endif                        

	                                                     
	if (CPU_SUBTYPE_ARM64_PTR_AUTH_VERSION(execfeatures) ==
	    CPU_SUBTYPE_ARM64_PTR_AUTH_CURRENT_VERSION) {
		return 12;
	}

	                                                                                     
	return 11;
}
#endif                

                                                                       
                         
 
                                                          
                                                        
                                                        
                  
                                                                       
int
grade_binary(cpu_type_t exectype, cpu_subtype_t execsubtype, cpu_subtype_t execfeatures __unused, bool allow_simulator_binary __unused)
{
#if __arm64__
	cpu_subtype_t hostsubtype =
	    (exectype & CPU_ARCH_ABI64) ? cpu_subtype() : cpu_subtype32();
#else
	cpu_subtype_t hostsubtype = cpu_subtype();
#endif                

	switch (exectype) {
#if __arm64__
	case CPU_TYPE_ARM64:
		switch (hostsubtype) {
		case CPU_SUBTYPE_ARM64_V8:
			switch (execsubtype) {
			case CPU_SUBTYPE_ARM64_V8:
				return 10;
			case CPU_SUBTYPE_ARM64_ALL:
				return 9;
			}
			break;

		case CPU_SUBTYPE_ARM64E:
			switch (execsubtype) {
			case CPU_SUBTYPE_ARM64E:
				return grade_arm64e_binary(execfeatures);
			case CPU_SUBTYPE_ARM64_V8:
				return 10;
			case CPU_SUBTYPE_ARM64_ALL:
				return 9;
			}
		}                           
		break;

#else                

	case CPU_TYPE_ARM:
		switch (hostsubtype) {
		  
                                                                        
     
		case CPU_SUBTYPE_ARM_V8:
			switch (execsubtype) {
			case CPU_SUBTYPE_ARM_V8:
				return 7;
			}
			goto v7s;

			  
                                                                        
                                                
      
v7s:
		case CPU_SUBTYPE_ARM_V7S:
			switch (execsubtype) {
			case CPU_SUBTYPE_ARM_V7S:
				return 6;
			}
			goto v7;

		  
                                                        
     
		case CPU_SUBTYPE_ARM_V7K:
			switch (execsubtype) {
			case CPU_SUBTYPE_ARM_V7K:
				return 6;
			}
			break;

		  
                                                                  
                                                         
     
		case CPU_SUBTYPE_ARM_V7F:
			switch (execsubtype) {
			case CPU_SUBTYPE_ARM_V7F:
				return 6;
			}
			goto v7;

v7:
		case CPU_SUBTYPE_ARM_V7:
			switch (execsubtype) {
			case CPU_SUBTYPE_ARM_V7:
				return 5;
			}
		                  

		case CPU_SUBTYPE_ARM_V6:
			switch (execsubtype) {
			case CPU_SUBTYPE_ARM_V6:
				return 4;
			}
		                  

		case CPU_SUBTYPE_ARM_V5TEJ:
			switch (execsubtype) {
			case CPU_SUBTYPE_ARM_V5TEJ:
				return 3;
			}
		               

		case CPU_SUBTYPE_ARM_V4T:
			switch (execsubtype) {
			case CPU_SUBTYPE_ARM_V4T:
				return 2;
			case CPU_SUBTYPE_ARM_ALL:
				return 1;
			}
			break;

		case CPU_SUBTYPE_ARM_XSCALE:
			switch (execsubtype) {
			case CPU_SUBTYPE_ARM_XSCALE:
				return 4;
			case CPU_SUBTYPE_ARM_V5TEJ:
				return 3;
			case CPU_SUBTYPE_ARM_V4T:
				return 2;
			case CPU_SUBTYPE_ARM_ALL:
				return 1;
			}
			break;
		}
#endif                
	}

	return 0;
}
