/*
 * Copyright (c) 2000-2007 Apple Inc. All rights reserved.
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
  
                                                                      
                                                                          
                                                                        
               
   

#ifndef    _MACH_KMOD_H_
#define    _MACH_KMOD_H_

#include <mach/kern_return.h>
#include <mach/mach_types.h>

#include <sys/cdefs.h>

__BEGIN_DECLS

#if PRAGMA_MARK
#pragma mark Basic macros & typedefs
#endif
                                                                        
                         
                                                                        
#define KMOD_MAX_NAME    64

#define KMOD_RETURN_SUCCESS    KERN_SUCCESS
#define KMOD_RETURN_FAILURE    KERN_FAILURE

typedef int kmod_t;

struct  kmod_info;
typedef kern_return_t kmod_start_func_t(struct kmod_info * ki, void * data);
typedef kern_return_t kmod_stop_func_t(struct kmod_info * ki, void * data);

#if PRAGMA_MARK
#pragma mark Structure definitions
#endif
                                                                        
                       
 
                                         
                                                                        
#pragma pack(push, 4)

                                                 
typedef struct kmod_reference {
	struct kmod_reference * next;
	struct kmod_info      * info;
} kmod_reference_t;

                                                                        
                                                            
                             
                                                                        

                                                                   
                                                                    
                                                              
   
typedef struct kmod_info {
	struct kmod_info  * next;
	int32_t             info_version;                                   
	uint32_t            id;
	char                name[KMOD_MAX_NAME];
	char                version[KMOD_MAX_NAME];
	int32_t             reference_count;                             
	kmod_reference_t  * reference_list;                                
	vm_address_t        address;                               
	vm_size_t           size;                            
	vm_size_t           hdr_size;                              
	kmod_start_func_t * start;
	kmod_stop_func_t  * stop;
} kmod_info_t;

                                                              
   
typedef struct kmod_info_32_v1 {
	uint32_t            next_addr;
	int32_t             info_version;
	uint32_t            id;
	uint8_t             name[KMOD_MAX_NAME];
	uint8_t             version[KMOD_MAX_NAME];
	int32_t             reference_count;
	uint32_t            reference_list_addr;
	uint32_t            address;
	uint32_t            size;
	uint32_t            hdr_size;
	uint32_t            start_addr;
	uint32_t            stop_addr;
} kmod_info_32_v1_t;

                                                              
   
typedef struct kmod_info_64_v1 {
	uint64_t            next_addr;
	int32_t             info_version;
	uint32_t            id;
	uint8_t             name[KMOD_MAX_NAME];
	uint8_t             version[KMOD_MAX_NAME];
	int32_t             reference_count;
	uint64_t            reference_list_addr;
	uint64_t            address;
	uint64_t            size;
	uint64_t            hdr_size;
	uint64_t            start_addr;
	uint64_t            stop_addr;
} kmod_info_64_v1_t;

#pragma pack(pop)

#if PRAGMA_MARK
#pragma mark Kmod structure declaration macros
#endif
                                                                        
                                   
                                                                        
#define KMOD_INFO_NAME       kmod_info
#define KMOD_INFO_VERSION    1

#define KMOD_DECL(name, version)                                  \
    static kmod_start_func_t name ## _module_start;               \
    static kmod_stop_func_t  name ## _module_stop;                \
    kmod_info_t KMOD_INFO_NAME = { 0, KMOD_INFO_VERSION, -1U,      \
	               { #name }, { version }, -1, 0, 0, 0, 0,    \
	                   name ## _module_start,                 \
	                   name ## _module_stop };

#define KMOD_EXPLICIT_DECL(name, version, start, stop)            \
    kmod_info_t KMOD_INFO_NAME = { 0, KMOD_INFO_VERSION, -1U,      \
	               { #name }, { version }, -1, 0, 0, 0, 0,    \
	                   start, stop };

#if PRAGMA_MARK
#pragma mark Kernel private declarations
#endif
                                                                        
                              
                                                                        
#ifdef    KERNEL_PRIVATE

                                                  
extern void kmod_panic_dump(vm_offset_t * addr, unsigned int dump_cnt);

#if CONFIG_DTRACE
  
                                                                 
                                                             
                                                  
   
#define KMOD_DTRACE_FORCE_INIT  0x01
#define KMOD_DTRACE_STATIC_KEXT 0x02
#define KMOD_DTRACE_NO_KERNEL_SYMS 0x04
#endif                    

#endif                        


#if PRAGMA_MARK
#pragma mark Obsolete kmod stuff
#endif
                                                                        
                                                                       
                                                                        
typedef void * kmod_args_t;
typedef int kmod_control_flavor_t;
typedef kmod_info_t * kmod_info_array_t;

__END_DECLS

#endif                       
