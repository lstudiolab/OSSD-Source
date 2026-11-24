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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

  
                                                                                                
                                                                            
   

                                                    
#define DYLD_ALL_IMAGE_INFOS_ADDRESS_MINIMUM_VERSION    9
#define DYLD_ALL_IMAGE_INFOS_TIMESTAMP_MINIMUM_VERSION  15

#define DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT 8
#define DYLD_PROCESS_INFO_NOTIFY_MAGIC 0x49414E46

                                                
#if __LP64__
typedef struct user64_dyld_uuid_info kernel_uuid_info;
#else
typedef struct user32_dyld_uuid_info kernel_uuid_info;
#endif

struct user32_dyld_image_info {
	user32_addr_t   imageLoadAddress;                                             
	user32_addr_t   imageFilePath;                                                
	user32_ulong_t  imageFileModDate;                                 
};

struct user64_dyld_image_info {
	user64_addr_t   imageLoadAddress;                                             
	user64_addr_t   imageFilePath;                                                
	user64_ulong_t  imageFileModDate;                                 
};

                                                                                
                                                  
typedef uint8_t dyld_bool;

struct user32_dyld_all_image_infos {
	uint32_t                                        version;
	uint32_t                                        infoArrayCount;
	user32_addr_t                           infoArray;
	user32_addr_t                           notification;
	dyld_bool                                       processDetachedFromSharedRegion;
	dyld_bool                                       libSystemInitialized;
	user32_addr_t                           dyldImageLoadAddress;
	user32_addr_t                           jitInfo;
	user32_addr_t                           dyldVersion;
	user32_addr_t                           errorMessage;
	user32_addr_t                           terminationFlags;
	user32_addr_t                           coreSymbolicationShmPage;
	user32_addr_t                           systemOrderFlag;
	user32_size_t uuidArrayCount;                                                             
	user32_addr_t uuidArray;
	user32_addr_t dyldAllImageInfosAddress;

	                                                                                  
	user32_addr_t initialImageCount;
	                                                                                  
	user32_addr_t errorKind;
	user32_addr_t errorClientOfDylibPath;
	user32_addr_t errorTargetDylibPath;
	user32_addr_t errorSymbol;
	                                                                                  
	user32_addr_t sharedCacheSlide;
	                                                                                  
	uint8_t sharedCacheUUID[16];
	                                                                                    
	user32_addr_t   sharedCacheBaseAddress;
	uint64_t        timestamp;
	user32_addr_t   dyldpath;
	mach_port_name_t notifyMachPorts[DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT];
	user32_addr_t   reserved[5];
	                                                                                   
	user32_addr_t compact_dyld_image_info_addr;
	user32_size_t compact_dyld_image_info_size;
};

struct user64_dyld_all_image_infos {
	uint32_t                                        version;
	uint32_t                                        infoArrayCount;
	user64_addr_t                           infoArray;
	user64_addr_t                           notification;
	dyld_bool                                       processDetachedFromSharedRegion;
	dyld_bool                                       libSystemInitialized;
	user64_addr_t                           dyldImageLoadAddress;
	user64_addr_t                           jitInfo;
	user64_addr_t                           dyldVersion;
	user64_addr_t                           errorMessage;
	user64_addr_t                           terminationFlags;
	user64_addr_t                           coreSymbolicationShmPage;
	user64_addr_t                           systemOrderFlag;
	user64_size_t uuidArrayCount;                                                             
	user64_addr_t uuidArray;
	user64_addr_t dyldAllImageInfosAddress;

	                                                                                  
	user64_addr_t initialImageCount;
	                                                                                  
	user64_addr_t errorKind;
	user64_addr_t errorClientOfDylibPath;
	user64_addr_t errorTargetDylibPath;
	user64_addr_t errorSymbol;
	                                                                                  
	user64_addr_t sharedCacheSlide;
	                                                                                  
	uint8_t sharedCacheUUID[16];
	                                                                                 
	user64_addr_t   sharedCacheBaseAddress;
	uint64_t        timestamp;
	user64_addr_t   dyldPath;
	mach_port_name_t notifyMachPorts[DYLD_MAX_PROCESS_INFO_NOTIFY_COUNT];
	user64_addr_t   reserved[9];
	                                                                                   
	user64_addr_t compact_dyld_image_info_addr;
	user64_size_t compact_dyld_image_info_size;
	uint32_t        platform;
	                                                                         
	uint32_t                                aotInfoArrayCount;
	user64_addr_t                           aotInfoArray;
	uint64_t                                aotTimestamp;
};
