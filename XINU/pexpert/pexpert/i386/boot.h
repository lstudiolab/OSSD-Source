/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
#ifndef _PEXPERT_I386_BOOT_H
#define _PEXPERT_I386_BOOT_H

#include <stdint.h>

  
                                                
   

  
                                                         
   
enum {
	kBootDriverTypeInvalid = 0,
	kBootDriverTypeKEXT    = 1,
	kBootDriverTypeMKEXT   = 2
};

enum {
	kEfiReservedMemoryType      = 0,
	kEfiLoaderCode              = 1,
	kEfiLoaderData              = 2,
	kEfiBootServicesCode        = 3,
	kEfiBootServicesData        = 4,
	kEfiRuntimeServicesCode     = 5,
	kEfiRuntimeServicesData     = 6,
	kEfiConventionalMemory      = 7,
	kEfiUnusableMemory          = 8,
	kEfiACPIReclaimMemory       = 9,
	kEfiACPIMemoryNVS           = 10,
	kEfiMemoryMappedIO          = 11,
	kEfiMemoryMappedIOPortSpace = 12,
	kEfiPalCode                 = 13,
	kEfiMaxMemoryType           = 14
};

  
                           
   
typedef struct EfiMemoryRange {
	uint32_t Type;
	uint32_t Pad;
	uint64_t PhysicalStart;
	uint64_t VirtualStart;
	uint64_t NumberOfPages;
	uint64_t Attribute;
} EfiMemoryRange;

#define BOOT_LINE_LENGTH        1024
#define BOOT_STRING_LEN         BOOT_LINE_LENGTH

  
                      
   

struct Boot_VideoV1 {
	uint32_t        v_baseAddr;                                       
	uint32_t        v_display;                                       
	uint32_t        v_rowBytes;                                        
	uint32_t        v_width;                   
	uint32_t        v_height;                   
	uint32_t        v_depth;                         
};
typedef struct Boot_VideoV1     Boot_VideoV1;

struct Boot_Video {
	uint32_t        v_display;                                       
	uint32_t        v_rowBytes;                                        
	uint32_t        v_width;                   
	uint32_t        v_height;                   
	uint32_t        v_depth;                         
	uint8_t         v_rotate;                     
	uint8_t         v_resv_byte[3];               
	uint32_t        v_resv[6];                    
	uint64_t        v_baseAddr;                                       
};
typedef struct Boot_Video       Boot_Video;

                          

#define GRAPHICS_MODE         1
#define FB_TEXT_MODE          2

                                                        
struct boot_icon_element {
	unsigned int    width;
	unsigned int    height;
	int             y_offset_from_center;
	unsigned int    data_size;
	unsigned int    __reserved1[4];
	unsigned char   data[0];
};
typedef struct boot_icon_element boot_icon_element;

                                                                  
                                                       
   
#define kBootArgsRevision               0
#define kBootArgsRevision0              kBootArgsRevision
#define kBootArgsRevision1              1                         
#define kBootArgsVersion                2

                                                                 
#define kBootArgsVersion1               1
#define kBootArgsVersion2               2
#define kBootArgsRevision2_0            0

#define kBootArgsEfiMode32              32
#define kBootArgsEfiMode64              64

                                    
#define kBootArgsFlagRebootOnPanic      (1 << 0)
#define kBootArgsFlagHiDPI              (1 << 1)
#define kBootArgsFlagBlack              (1 << 2)
#define kBootArgsFlagCSRActiveConfig    (1 << 3)
#define kBootArgsFlagCSRConfigMode      (1 << 4)
#define kBootArgsFlagCSRBoot            (1 << 5)
#define kBootArgsFlagBlackBg            (1 << 6)
#define kBootArgsFlagLoginUI            (1 << 7)
#define kBootArgsFlagInstallUI          (1 << 8)
#define kBootArgsFlagRecoveryBoot       (1 << 10)

typedef struct boot_args {
	uint16_t    Revision;                                        
	uint16_t    Version;                                        

	uint8_t     efiMode;                              
	uint8_t     debugMode;                                     
	uint16_t    flags;

	char        CommandLine[BOOT_LINE_LENGTH];                            

	uint32_t    MemoryMap;                                    
	uint32_t    MemoryMapSize;
	uint32_t    MemoryMapDescriptorSize;
	uint32_t    MemoryMapDescriptorVersion;

	Boot_VideoV1 VideoV1;                          

	uint32_t    deviceTreeP;                                                 
	uint32_t    deviceTreeLength;                              

	uint32_t    kaddr;                                                          
	uint32_t    ksize;                                                   

	uint32_t    efiRuntimeServicesPageStart;                                                    
	uint32_t    efiRuntimeServicesPageCount;
	uint64_t    efiRuntimeServicesVirtualPageStart;                                                   

	uint32_t    efiSystemTable;                                                      
	uint32_t    kslide;

	uint32_t    performanceDataStart;                             
	uint32_t    performanceDataSize;

	uint32_t    keyStoreDataStart;                                        
	uint32_t    keyStoreDataSize;
	uint64_t    bootMemStart;
	uint64_t    bootMemSize;
	uint64_t    PhysicalMemorySize;
	uint64_t    FSBFrequency;
	uint64_t    pciConfigSpaceBaseAddress;
	uint32_t    pciConfigSpaceStartBusNumber;
	uint32_t    pciConfigSpaceEndBusNumber;
	uint32_t    csrActiveConfig;
	uint32_t    csrCapabilities;
	uint32_t    boot_SMC_plimit;
	uint16_t    bootProgressMeterStart;
	uint16_t    bootProgressMeterEnd;
	Boot_Video  Video;                             

	uint32_t    apfsDataStart;                                                   
	uint32_t    apfsDataSize;

	                           
	uint64_t    KC_hdrs_vaddr;

	uint64_t    arvRootHashStart;                                                       
	uint64_t    arvRootHashSize;

	uint64_t    arvManifestStart;                                                      
	uint64_t    arvManifestSize;

	uint64_t    bsARVRootHashStart;                                                    
	uint64_t    bsARVRootHashSize;

	uint64_t    bsARVManifestStart;                                                   
	uint64_t    bsARVManifestSize;

	              
	uint32_t    __reserved4[692];
} boot_args;

extern char assert_boot_args_size_is_4096[sizeof(boot_args) == 4096 ? 1 : -1];

#endif                           
