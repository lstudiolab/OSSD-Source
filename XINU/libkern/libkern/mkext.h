/*
 * Copyright (c) 2008 Apple Inc. All rights reserved.
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

#ifndef _MKEXT_H_
#define _MKEXT_H_ 1

#include <sys/cdefs.h>
#include <sys/types.h>

#include <mach/machine.h>

                                                                      
                   
 
                                   
                                                                      
#if PRAGMA_MARK
#pragma mark Constants
#endif

#define MKEXT_MAGIC 0x4D4B5854             
#define MKEXT_SIGN  0x4D4F5358             

#define MKEXT_EXTN  ".mkext"

#define MKEXT_VERS_1  (0x01008000)

                                           
#define MKEXT_VERS_2  (0x02002001)
                                           
                                     

#if PRAGMA_MARK
#pragma mark Core Header
#endif
                                                                      
             
 
                                                     
 
                                                                  
                                         
                                                   
                                  
                                                                     
                                                             
                                                                  
                                                                     
                                                                
                                                    
                                                                      

#define MKEXT_HEADER_CORE      \
    uint32_t      magic;       \
    uint32_t      signature;   \
    uint32_t      length;      \
    uint32_t      adler32;     \
    uint32_t      version;     \
    uint32_t      numkexts;    \
    cpu_type_t    cputype;     \
    cpu_subtype_t cpusubtype;

typedef struct mkext_basic_header {
	MKEXT_HEADER_CORE
} mkext_basic_header;

#define MKEXT_HDR_CAST(hdr)        ((mkext_basic_header *)(hdr))

#define MKEXT_SWAP(num)            OSSwapBigToHostInt32((uint32_t)(num))

#define MKEXT_GET_MAGIC(hdr)       (MKEXT_SWAP(MKEXT_HDR_CAST(hdr)->magic))
#define MKEXT_GET_SIGNATURE(hdr)   (MKEXT_SWAP(MKEXT_HDR_CAST(hdr)->signature))
#define MKEXT_GET_LENGTH(hdr)      (MKEXT_SWAP(MKEXT_HDR_CAST(hdr)->length))
#define MKEXT_GET_CHECKSUM(hdr)    (MKEXT_SWAP(MKEXT_HDR_CAST(hdr)->adler32))
#define MKEXT_GET_VERSION(hdr)     (MKEXT_SWAP(MKEXT_HDR_CAST(hdr)->version))
#define MKEXT_GET_COUNT(hdr)       (MKEXT_SWAP(MKEXT_HDR_CAST(hdr)->numkexts))
#define MKEXT_GET_CPUTYPE(hdr)     (MKEXT_SWAP(MKEXT_HDR_CAST(hdr)->cputype))
#define MKEXT_GET_CPUSUBTYPE(hdr)  (MKEXT_SWAP(MKEXT_HDR_CAST(hdr)->cpusubtype))

#if PRAGMA_MARK
#pragma mark Mkext Version 2 Format Definitions
#endif
                                                                      
                                    
 
                                                                    
                                       
 
                                                                   
               
 
                                                                      
                                                                     
                                                                      
                                                                     
         
 
                                                              
                        
 
                                                                 
                       
                                                                
                                                                      
                    
 
                                                                   
                                                                      
                                                             
                        
 
                                                                          
                                                                    
                                                                
                                                            
                                            
                                                                   
                                                            
                                                                 
                                                                  
                                                                  
                                                  
                                                                      
#define kMKEXTInfoDictionariesKey             "_MKEXTInfoDictionaries"

#define kMKEXTBundlePathKey                   "_MKEXTBundlePath"
#define kMKEXTExecutableRelativePathKey       "_MKEXTExecutableRelativePath"
#define kMKEXTExecutableKey                   "_MKEXTExecutable"

#define kMKEXTLoadRequestKey                  "_MKEXTLoadRequest"
#define kMKEXTLoadRequestLoadKey              "Load Kext"
#define kMKEXTLoadRequestStartKey             "Start Kext"
#define kMKEXTLoadRequestAddPersonalitiesKey  "Add Personalities"
#define kMKEXTLoadRequestDisableAutounloadKey "Disable Autounload"

typedef struct mkext2_file_entry {
	uint32_t  compressed_size;                                  
	uint32_t  full_size;                                        
	uint8_t   data[0];                                      
} mkext2_file_entry;

typedef struct mkext2_header {
	MKEXT_HEADER_CORE
	uint32_t plist_offset;
	uint32_t plist_compressed_size;
	uint32_t plist_full_size;
} mkext2_header;

#define MKEXT2_GET_ENTRY_COMPSIZE(ptr)   MKEXT_SWAP((ptr)->compressed_size)
#define MKEXT2_GET_ENTRY_FULLSIZE(ptr)   MKEXT_SWAP((ptr)->full_size)
#define MKEXT2_GET_ENTRY_DATA(ptr)       ((ptr)->data)

#define MKEXT2_HDR_CAST(hdr)             ((mkext2_header *)(hdr))
#define MKEXT2_GET_PLIST(hdr)            MKEXT_SWAP(MKEXT2_HDR_CAST(hdr)->plist_offset)
#define MKEXT2_GET_PLIST_COMPSIZE(hdr)   MKEXT_SWAP(MKEXT2_HDR_CAST(hdr)->plist_compressed_size)
#define MKEXT2_GET_PLIST_FULLSIZE(hdr)   MKEXT_SWAP(MKEXT2_HDR_CAST(hdr)->plist_full_size)

#if PRAGMA_MARK
#pragma mark Mkext Version 1 Format Definitions
#endif
                                                                      
                                    
 
                                                           
 
                                                                 
                                                    
 
                                                        
                                                                      

                                                   
                                                      
typedef struct mkext_file {
	uint32_t offset;               
	uint32_t compsize;             
	uint32_t realsize;             
	uint32_t modifiedsecs;                                  
} mkext_file;

                                                             
typedef struct mkext_kext {
	mkext_file plist;               
	mkext_file module;              
} mkext_kext;

typedef struct mkext_header {
	MKEXT_HEADER_CORE
	mkext_kext kext[1];                  
} mkext_header;

typedef mkext_header mkext1_header;

#define MKEXT1_ENTRY_CAST(ptr)           ((mkext_file *)(ptr))
#define MKEXT1_GET_ENTRY_OFFSET(ptr)     (MKEXT_SWAP(MKEXT1_ENTRY_CAST(ptr)->offset))
#define MKEXT1_GET_ENTRY_COMPSIZE(ptr)   (MKEXT_SWAP(MKEXT1_ENTRY_CAST(ptr)->compsize))
#define MKEXT1_GET_ENTRY_FULLSIZE(ptr)   (MKEXT_SWAP(MKEXT1_ENTRY_CAST(ptr)->realsize))
#define MKEXT1_GET_ENTRY_MODTIME(ptr)    ((time_t)MKEXT_SWAP(MKEXT1_ENTRY_CAST(ptr)->modifiedsecs))
#define MKEXT1_ENTRY_EXISTS(ptr)         (MKEXT1_GET_ENTRY_OFFSET(ptr)   ||  \
	                                  MKEXT1_GET_ENTRY_FULLSIZE(ptr) ||  \
	                                  MKEXT_GET_ENTRY_COMPSIZE(ptr)  ||  \
	                                  MKEXT_GET_ENTRY_COMPSIZE(ptr))

#define MKEXT1_GET_KEXT(hdr, i)          ((mkext_kext *)&(MKEXT1_HDR_CAST(hdr)->kext[(i)]))
#define MKEXT1_GET_KEXT_PLIST(hdr, i)    (MKEXT1_ENTRY_CAST(&(MKEXT1_GET_KEXT((hdr), (i))->plist)))
#define MKEXT1_GET_KEXT_EXEC(hdr, i)     (MKEXT1_ENTRY_CAST(&(MKEXT1_GET_KEXT((hdr), (i))->module)

#define MKEXT1_HDR_CAST(hdr)             ((mkext1_header *)(hdr))

                                                      
   
__BEGIN_DECLS
u_int8_t *
compress_lzss(u_int8_t * dst, u_int32_t dstlen,
    u_int8_t * src, u_int32_t srclen);

int
decompress_lzss(u_int8_t * dst, u_int32_t dstlen,
    u_int8_t * src, u_int32_t srclen);

u_int32_t
mkext_adler32(u_int8_t * src, int32_t length);
__END_DECLS

#endif                
