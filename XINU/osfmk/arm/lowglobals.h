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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
  
                                                
   
#ifndef _LOW_MEMORY_GLOBALS_H_
#define _LOW_MEMORY_GLOBALS_H_

#ifndef __arm__
#error  Wrong architecture - this file is meant for arm
#endif

#define LOWGLO_LAYOUT_MAGIC             0xC0DEC0DE

#pragma pack(4)                                                     
typedef struct lowglo {
	unsigned char lgVerCode[8];                                                  
	uint32_t      lgZero[2];                                              
	uint32_t      lgStext;                                                   
	uint32_t      lgRsv014[2];                                   
	uint32_t      lgVersion;                                                             
	uint32_t      lgRsv020[216];                                 
	uint32_t      lgKmodptr;                                                           
	uint32_t      lgTransOff;                                                                   
	uint32_t      lgRsv388[3];                                   
	uint32_t      lgOSVersion;                                                       
	uint32_t      lgRsv398;                                      
	uint32_t      lgRebootFlag;                                                            
	uint32_t      lgManualPktAddr;                                                         
	uint32_t      lgRsv3A4;                                      
	uint32_t      lgPmapMemQ;                                                        
	uint32_t      lgPmapMemPageOffset;                                                                         
	uint32_t      lgPmapMemChainOffset;                                                                      
	uint32_t      lgStaticAddr;                                                   
	uint32_t      lgStaticSize;                                                
	uint32_t      lgLayoutMajorVersion;                                             
	uint32_t      lgLayoutMagic;                                                                                       
	uint32_t      lgPmapMemStartAddr;                                                       
	uint32_t      lgPmapMemEndAddr;                                                       
	uint32_t      lgPmapMemPagesize;                                      
	uint32_t      lgPmapMemFirstppnum;                                                                           
	uint32_t      lgLayoutMinorVersion;                                             
	uint32_t      lgPageShift;                                                                    
	uint32_t      lgVmFirstPhys;                                                                                  
	uint32_t      lgVmLastPhys;                                                                                  
} lowglo;
#pragma pack()

extern lowglo lowGlo;

void patch_low_glo(void);
void patch_low_glo_static_region(uint32_t address, uint32_t size);
void patch_low_glo_vm_page_info(void *, void *, uint32_t);

#endif                             
