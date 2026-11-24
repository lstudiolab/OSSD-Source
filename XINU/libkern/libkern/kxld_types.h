/*
 * Copyright (c) 2007-2012 Apple Inc. All rights reserved.
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
#ifndef _KXLD_TYPES_H
#define _KXLD_TYPES_H

#include <stdarg.h>
#include <stdint.h>
#include <mach/boolean.h>                   
#include <mach/kern_return.h>

                                                                                
        
                                                                                

                                      
#if (!KERNEL || !__LP64__)
    #define KXLD_USER_OR_ILP32 1
#endif

                                      
#if (!KERNEL || __LP64__)
    #define KXLD_USER_OR_LP64 1
#endif

                                    
#if (!KERNEL || __i386__)
    #define KXLD_USER_OR_I386 1
#endif

                                      
#if (!KERNEL || __x86_64__)
    #define KXLD_USER_OR_X86_64 1
#endif

                                   
#if (!KERNEL || __arm__)
    #define KXLD_USER_OR_ARM 1
#endif

                                     
#if (!KERNEL || __arm64__)
    #define KXLD_USER_OR_ARM64 1
#endif

                                                                            
#if (!KERNEL || __i386__)
    #define KXLD_USER_OR_COMMON 1
#endif

                                                                             
    #define KXLD_USER_OR_STRICT_PATCHING 1

                                                                   
#if (!KERNEL || __i386__)
    #define KXLD_USER_OR_OBJECT 1
#endif

                                                                        
#define KXLD_USER_OR_BUNDLE 1

                                                                                
                                                       
   
#define KXLD_USER_OR_GOT 0

                                                                 
#if (!KERNEL)
    #define KXLD_PIC_KEXTS 1
                           
    #define SPLIT_KEXTS_DEBUG 0
#endif

                                                                                
       
                                                                                

                                                                            
                    
   
typedef struct kxld_context KXLDContext;

                                                                             
                                                                            
                                                                
                                                                           
   
#if KERNEL && !__LP64__
typedef uint32_t kxld_addr_t;
typedef uint32_t kxld_size_t;
#else
typedef uint64_t kxld_addr_t;
typedef uint64_t kxld_size_t;
#endif                          

typedef struct splitKextLinkInfo {
	u_char *        kextExecutable;                     
	size_t          kextSize;                                
	u_char *        linkedKext;                   
	size_t          linkedKextSize;                      
	uint64_t        vmaddr_TEXT;                                    
	uint64_t        vmaddr_TEXT_EXEC;                                     
	uint64_t        vmaddr_DATA;                                    
	uint64_t        vmaddr_DATA_CONST;                                      
	uint64_t        vmaddr_LINKEDIT;                                    
	uint64_t        vmaddr_LLVM_COV;                                    
	uint32_t        kaslr_offsets_count;                                 
	uint32_t *      kaslr_offsets;                                   
} splitKextLinkInfo;

                                       
enum kxld_flags {
	kKxldFlagDefault = 0x0,
	kKXLDFlagIncludeRelocs = 0x01
};
typedef enum kxld_flags KXLDFlags;

                                       
enum kxld_allocate_flags {
	kKxldAllocateDefault = 0x0,
	kKxldAllocateWritable = 0x1,                                                  
};
typedef enum kxld_allocate_flags KXLDAllocateFlags;

                                                                               
                                                                          
                                                                               
           
   
typedef kxld_addr_t (*KXLDAllocateCallback)(size_t size,
    KXLDAllocateFlags *flags, void *user_data);

                                    
typedef enum kxld_log_subsystem {
	kKxldLogLinking = 0x0,
	kKxldLogPatching = 0x01
} KXLDLogSubsystem;

typedef enum kxld_log_level {
	kKxldLogExplicit = 0x0,
	kKxldLogErr = 0x1,
	kKxldLogWarn = 0x2,
	kKxldLogBasic = 0x3,
	kKxldLogDetail = 0x4,
	kKxldLogDebug = 0x5
} KXLDLogLevel;

                                                                       
                                                                         
                                                                      
                                                       
  
                                                                       
                                                                     
                              
   
typedef struct kxld_dependency {
	u_char      * kext;
	u_long        kext_size;
	char        * kext_name;
	u_char      * interface;
	u_long        interface_size;
	char        * interface_name;
	boolean_t     is_direct_dependency;
} KXLDDependency;

typedef void (*KXLDLoggingCallback) (KXLDLogSubsystem sys, KXLDLogLevel level,
    const char *format, va_list ap, void *user_data);

#endif                    
