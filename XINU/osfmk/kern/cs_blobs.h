/*
 * Copyright (c) 2017 Apple Computer, Inc. All rights reserved.
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

#ifndef _KERN_CODESIGN_H_
#define _KERN_CODESIGN_H_

#include <stdint.h>

                                          
#define CS_VALID                    0x00000001                         
#define CS_ADHOC                    0x00000002                     
#define CS_GET_TASK_ALLOW           0x00000004                                      
#define CS_INSTALLER                0x00000008                                 

#define CS_FORCED_LV                0x00000010                                                             
#define CS_INVALID_ALLOWED          0x00000020                                                                  

#define CS_HARD                     0x00000100                                
#define CS_KILL                     0x00000200                                          
#define CS_CHECK_EXPIRATION         0x00000400                                 
#define CS_RESTRICT                 0x00000800                                     

#define CS_ENFORCEMENT              0x00001000                           
#define CS_REQUIRE_LV               0x00002000                                  
#define CS_ENTITLEMENTS_VALIDATED   0x00004000                                                      
#define CS_NVRAM_UNRESTRICTED       0x00008000                                                                               

#define CS_RUNTIME                  0x00010000                                       
#define CS_LINKER_SIGNED            0x00020000                                          

#define CS_ALLOWED_MACHO            (CS_ADHOC | CS_HARD | CS_KILL | CS_CHECK_EXPIRATION | \
	                             CS_RESTRICT | CS_ENFORCEMENT | CS_REQUIRE_LV | CS_RUNTIME | CS_LINKER_SIGNED)

#define CS_EXEC_SET_HARD            0x00100000                                          
#define CS_EXEC_SET_KILL            0x00200000                                          
#define CS_EXEC_SET_ENFORCEMENT     0x00400000                                                 
#define CS_EXEC_INHERIT_SIP         0x00800000                                               

#define CS_KILLED                   0x01000000                                           
#define CS_DYLD_PLATFORM            0x02000000                                                   
#define CS_PLATFORM_BINARY          0x04000000                                 
#define CS_PLATFORM_PATH            0x08000000                                                      

#define CS_DEBUGGED                 0x10000000                                                                                                  
#define CS_SIGNED                   0x20000000                                                       
#define CS_DEV_CODE                 0x40000000                                                                                                              
#define CS_DATAVAULT_CONTROLLER     0x80000000                                             

#define CS_ENTITLEMENT_FLAGS        (CS_GET_TASK_ALLOW | CS_INSTALLER | CS_DATAVAULT_CONTROLLER | CS_NVRAM_UNRESTRICTED)

                              

#define CS_EXECSEG_MAIN_BINARY          0x1                                                         
#define CS_EXECSEG_ALLOW_UNSIGNED       0x10                                                      
#define CS_EXECSEG_DEBUGGER             0x20                                         
#define CS_EXECSEG_JIT                  0x40                             
#define CS_EXECSEG_SKIP_LV              0x80                                                   
#define CS_EXECSEG_CAN_LOAD_CDHASH      0x100                                               
#define CS_EXECSEG_CAN_EXEC_CDHASH      0x200                                           

  
                                     
   
enum {
	CSMAGIC_REQUIREMENT = 0xfade0c00,                                            
	CSMAGIC_REQUIREMENTS = 0xfade0c01,                                                               
	CSMAGIC_CODEDIRECTORY = 0xfade0c02,                                     
	CSMAGIC_EMBEDDED_SIGNATURE = 0xfade0cc0,                                      
	CSMAGIC_EMBEDDED_SIGNATURE_OLD = 0xfade0b02,             
	CSMAGIC_EMBEDDED_ENTITLEMENTS = 0xfade7171,                                
	CSMAGIC_DETACHED_SIGNATURE = 0xfade0cc1,                                                   
	CSMAGIC_BLOBWRAPPER = 0xfade0b01,                                              

	CS_SUPPORTSSCATTER = 0x20100,
	CS_SUPPORTSTEAMID = 0x20200,
	CS_SUPPORTSCODELIMIT64 = 0x20300,
	CS_SUPPORTSEXECSEG = 0x20400,
	CS_SUPPORTSRUNTIME = 0x20500,
	CS_SUPPORTSLINKAGE = 0x20600,

	CSSLOT_CODEDIRECTORY = 0,                                                                 
	CSSLOT_INFOSLOT = 1,
	CSSLOT_REQUIREMENTS = 2,
	CSSLOT_RESOURCEDIR = 3,
	CSSLOT_APPLICATION = 4,
	CSSLOT_ENTITLEMENTS = 5,

	CSSLOT_ALTERNATE_CODEDIRECTORIES = 0x1000,                                            
	CSSLOT_ALTERNATE_CODEDIRECTORY_MAX = 5,                                               
	CSSLOT_ALTERNATE_CODEDIRECTORY_LIMIT = CSSLOT_ALTERNATE_CODEDIRECTORIES + CSSLOT_ALTERNATE_CODEDIRECTORY_MAX,                        

	CSSLOT_SIGNATURESLOT = 0x10000,                                    
	CSSLOT_IDENTIFICATIONSLOT = 0x10001,
	CSSLOT_TICKETSLOT = 0x10002,

	CSTYPE_INDEX_REQUIREMENTS = 0x00000002,                               
	CSTYPE_INDEX_ENTITLEMENTS = 0x00000005,                               

	CS_HASHTYPE_SHA1 = 1,
	CS_HASHTYPE_SHA256 = 2,
	CS_HASHTYPE_SHA256_TRUNCATED = 3,
	CS_HASHTYPE_SHA384 = 4,

	CS_SHA1_LEN = 20,
	CS_SHA256_LEN = 32,
	CS_SHA256_TRUNCATED_LEN = 20,

	CS_CDHASH_LEN = 20,                                                                                       
	CS_HASH_MAX_SIZE = 48,                                         

  
                                                                  
                                                                             
   
	CS_SIGNER_TYPE_UNKNOWN = 0,
	CS_SIGNER_TYPE_LEGACYVPN = 5,
	CS_SIGNER_TYPE_MAC_APP_STORE = 6,

	CS_SUPPL_SIGNER_TYPE_UNKNOWN = 0,
	CS_SUPPL_SIGNER_TYPE_TRUSTCACHE = 7,
	CS_SUPPL_SIGNER_TYPE_LOCAL = 8,
};

#define KERNEL_HAVE_CS_CODEDIRECTORY 1
#define KERNEL_CS_CODEDIRECTORY_HAVE_PLATFORM 1

  
                             
   
typedef struct __CodeDirectory {
	uint32_t magic;                                                                           
	uint32_t length;                                                                        
	uint32_t version;                                                          
	uint32_t flags;                                                           
	uint32_t hashOffset;                                                                   
	uint32_t identOffset;                                                    
	uint32_t nSpecialSlots;                                                   
	uint32_t nCodeSlots;                                                              
	uint32_t codeLimit;                                                                      
	uint8_t hashSize;                                                               
	uint8_t hashType;                                                                         
	uint8_t platform;                                                                                     
	uint8_t pageSize;                                                                            
	uint32_t spare2;                                                           

	char end_earliest[0];

	                     
	uint32_t scatterOffset;                                                        
	char end_withScatter[0];

	                     
	uint32_t teamOffset;                                                            
	char end_withTeam[0];

	                     
	uint32_t spare3;                                                           
	uint64_t codeLimit64;                                                                     
	char end_withCodeLimit64[0];

	                     
	uint64_t execSegBase;                                                     
	uint64_t execSegLimit;                                                   
	uint64_t execSegFlags;                                                
	char end_withExecSeg[0];
	                     
	uint32_t runtime;
	uint32_t preEncryptOffset;
	char end_withPreEncryptOffset[0];

	                     
	uint8_t linkageHashType;
	uint8_t linkageTruncated;
	uint16_t spare4;
	uint32_t linkageOffset;
	uint32_t linkageSize;
	char end_withLinkage[0];


	                                                                   
} CS_CodeDirectory
__attribute__ ((aligned(1)));

  
                                               
   

typedef struct __BlobIndex {
	uint32_t type;                                                     
	uint32_t offset;                                                     
} CS_BlobIndex
__attribute__ ((aligned(1)));

typedef struct __SC_SuperBlob {
	uint32_t magic;                                                   
	uint32_t length;                                                               
	uint32_t count;                                                                        
	CS_BlobIndex index[];                                        
	                                                                               
} CS_SuperBlob
__attribute__ ((aligned(1)));

#define KERNEL_HAVE_CS_GENERICBLOB 1
typedef struct __SC_GenericBlob {
	uint32_t magic;                                                   
	uint32_t length;                                                          
	char data[];
} CS_GenericBlob
__attribute__ ((aligned(1)));

typedef struct __SC_Scatter {
	uint32_t count;                                                                             
	uint32_t base;                                                      
	uint64_t targetOffset;                                     
	uint64_t spare;                                            
} SC_Scatter
__attribute__ ((aligned(1)));


#endif                       
