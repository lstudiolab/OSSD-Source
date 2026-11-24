/*
 * Copyright (c) 2017 Apple Inc. All rights reserved.
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
  
                                                
   
#ifndef _LOW_MEMORY_GLOBALS_H_
#define _LOW_MEMORY_GLOBALS_H_

#include <mach/mach_types.h>
#include <mach/vm_types.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>

#ifndef __arm64__
#error  Wrong architecture - this file is meant for arm64
#endif

#define LOWGLO_LAYOUT_MAGIC             0xC0DEC0DE

  
                                                                          
                           
   

#pragma pack(8)                                                     
typedef struct lowglo {
	unsigned char lgVerCode[8];                                                             
	uint64_t      lgZero;                                                     
	uint64_t      lgStext;                                                              
	uint64_t      lgVersion;                                                                        
	uint64_t      lgOSVersion;                                                                  
	uint64_t      lgKmodptr;                                                                      
	uint64_t      lgTransOff;                                                                              
	uint64_t      lgRebootFlag;                                                                       
	uint64_t      lgManualPktAddr;                                                                    
	uint64_t      lgAltDebugger;                                                                                           
	uint64_t      lgPmapMemQ;                                                                   
	uint64_t      lgPmapMemPageOffset;                                                                                                 
	uint64_t      lgPmapMemChainOffset;                                                                                 
	uint64_t      lgStaticAddr;                                                              
	uint64_t      lgStaticSize;                                                           
	uint64_t      lgLayoutMajorVersion;                                                        
	uint64_t      lgLayoutMagic;                                                                                                  
	uint64_t      lgPmapMemStartAddr;                                                                  
	uint64_t      lgPmapMemEndAddr;                                                                  
	uint64_t      lgPmapMemPagesize;                                                 
	uint64_t      lgPmapMemFromArrayMask;                                                                        
	uint64_t      lgPmapMemFirstppnum;                                                                                      
	uint64_t      lgPmapMemPackedShift;                                                        
	uint64_t      lgPmapMemPackedBaseAddr;                                                                              
	uint64_t      lgLayoutMinorVersion;                                                        
	uint64_t      lgPageShift;                                                                               
	uint64_t      lgVmFirstPhys;                                                                                             
	uint64_t      lgVmLastPhys;                                                                                             
	uint64_t      lgPhysMapBase;                                                                                              
	uint64_t      lgPhysMapEnd;                                                                                              
	uint64_t      lgPmapIoRangePtr;                                                                                                              
	uint64_t      lgNumPmapIoRanges;                                                                                                             
} lowglo;
#pragma pack()

extern lowglo lowGlo;

void patch_low_glo(void);
void patch_low_glo_static_region(uint64_t address, uint64_t size);
void patch_low_glo_vm_page_info(void *, void *, uint32_t);

#endif                             
