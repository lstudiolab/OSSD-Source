/*
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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

  
                                                
   
#ifndef _LOW_MEMORY_GLOBALS_H_
#define _LOW_MEMORY_GLOBALS_H_

#include <mach/mach_types.h>
#include <mach/vm_types.h>
#include <mach/machine/vm_types.h>
#include <mach/vm_prot.h>

#ifndef __x86_64__
#error  Wrong architecture - this file is meant for x86_64
#endif

  
                                                                                  
                               
   

#pragma pack(8)                                                             
typedef struct lowglo {
	unsigned char   lgVerCode[8];                                                            
	uint64_t        lgZero;                                                           
	uint64_t        lgStext;                                                             
	uint64_t        lgRsv018;                                                
	uint64_t        lgRsv020;                                                
	uint64_t        lgRsv028;                                                
	uint64_t        lgVersion;                                                                       
	uint64_t        lgRsv038[280];                                           
	uint64_t        lgKmodptr;                                                                     
	uint64_t        lgTransOff;                                                                             
	uint64_t        lgReadIO;                                                                             
	uint64_t        lgDevSlot1;                                                       
	uint64_t        lgDevSlot2;                                                       
	uint64_t        lgOSVersion;                                                                 
	uint64_t        lgRebootFlag;                                                                      
	uint64_t        lgManualPktAddr;                                                                   
	uint64_t        lgKdpJtagCoredumpAddr;                                                                   

	uint64_t        lgRsv940[216];                                                            
} lowglo;
#pragma pack()
extern lowglo lowGlo;
#endif                             
