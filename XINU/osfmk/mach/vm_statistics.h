/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
/*
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
  
   
  
                             
                                                                 
  
                                       
  
   

#ifndef _MACH_VM_STATISTICS_H_
#define _MACH_VM_STATISTICS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <mach/machine/vm_types.h>
#include <mach/machine/kern_return.h>

  
                
  
           
                              
                                                           
                                   
  
                                                                           
                         
   

struct vm_statistics {
	natural_t       free_count;                                  
	natural_t       active_count;                                  
	natural_t       inactive_count;                                  
	natural_t       wire_count;                                        
	natural_t       zero_fill_count;                                  
	natural_t       reactivations;                                      
	natural_t       pageins;                                  
	natural_t       pageouts;                                  
	natural_t       faults;                                  
	natural_t       cow_faults;                                      
	natural_t       lookups;                                          
	natural_t       hits;                                          

	                    
	natural_t       purgeable_count;                                  
	natural_t       purges;                                        

	                    
	  
                                                                    
                                                                 
                                                               
                                                
    
	natural_t       speculative_count;                                  
};

                               
typedef struct vm_statistics    *vm_statistics_t;
typedef struct vm_statistics    vm_statistics_data_t;

  
                  
  
           
                              
                                                           
                                   
          
                                           
                                                 
                                       
                                                        
                                              
  
   

struct vm_statistics64 {
	natural_t       free_count;                                  
	natural_t       active_count;                                  
	natural_t       inactive_count;                                  
	natural_t       wire_count;                                        
	uint64_t        zero_fill_count;                                  
	uint64_t        reactivations;                                      
	uint64_t        pageins;                                  
	uint64_t        pageouts;                                  
	uint64_t        faults;                                  
	uint64_t        cow_faults;                                      
	uint64_t        lookups;                                          
	uint64_t        hits;                                          
	uint64_t        purges;                                        
	natural_t       purgeable_count;                                  
	  
                                                                    
                                                                 
                                                               
                                                
    
	natural_t       speculative_count;                                  

	                    
	uint64_t        decompressions;                                      
	uint64_t        compressions;                                      
	uint64_t        swapins;                                                                      
	uint64_t        swapouts;                                                                      
	natural_t       compressor_page_count;                                                                               
	natural_t       throttled_count;                                  
	natural_t       external_page_count;                                                    
	natural_t       internal_page_count;                                       
	uint64_t        total_uncompressed_pages_in_compressor;                                                            
} __attribute__((aligned(8)));

typedef struct vm_statistics64  *vm_statistics64_t;
typedef struct vm_statistics64  vm_statistics64_data_t;

kern_return_t vm_stats(void *info, unsigned int *count);

  
                                   
  
                                                                                         
                                                                                     
   
#define VM_STATISTICS_TRUNCATE_TO_32_BIT(value) ((uint32_t)(((value) > UINT32_MAX ) ? UINT32_MAX : (value)))

  
                       
  
                                                    
                  
  
           
                              
   

struct vm_extmod_statistics {
	int64_t task_for_pid_count;                                                             
	int64_t task_for_pid_caller_count;                                                    
	int64_t thread_creation_count;                                            
	int64_t thread_creation_caller_count;                                     
	int64_t thread_set_state_count;                                               
	int64_t thread_set_state_caller_count;                                        
} __attribute__((aligned(8)));

typedef struct vm_extmod_statistics *vm_extmod_statistics_t;
typedef struct vm_extmod_statistics vm_extmod_statistics_data_t;

typedef struct vm_purgeable_stat {
	uint64_t        count;
	uint64_t        size;
}vm_purgeable_stat_t;

struct vm_purgeable_info {
	vm_purgeable_stat_t fifo_data[8];
	vm_purgeable_stat_t obsolete_data;
	vm_purgeable_stat_t lifo_data[8];
};

typedef struct vm_purgeable_info        *vm_purgeable_info_t;

                                             

#define VM_PAGE_QUERY_PAGE_PRESENT      0x1
#define VM_PAGE_QUERY_PAGE_FICTITIOUS   0x2
#define VM_PAGE_QUERY_PAGE_REF          0x4
#define VM_PAGE_QUERY_PAGE_DIRTY        0x8
#define VM_PAGE_QUERY_PAGE_PAGED_OUT    0x10
#define VM_PAGE_QUERY_PAGE_COPIED       0x20
#define VM_PAGE_QUERY_PAGE_SPECULATIVE  0x40
#define VM_PAGE_QUERY_PAGE_EXTERNAL     0x80
#define VM_PAGE_QUERY_PAGE_CS_VALIDATED 0x100
#define VM_PAGE_QUERY_PAGE_CS_TAINTED   0x200
#define VM_PAGE_QUERY_PAGE_CS_NX        0x400
#define VM_PAGE_QUERY_PAGE_REUSABLE     0x800

#ifdef  MACH_KERNEL_PRIVATE

  
                                                       
                                                         
                                                       
                              
   

struct pmap_statistics {
	integer_t       resident_count;                               
	integer_t       resident_max;                                 
	integer_t       wired_count;                          

	integer_t       device;
	integer_t       device_peak;
	integer_t       internal;
	integer_t       internal_peak;
	integer_t       external;
	integer_t       external_peak;
	integer_t       reusable;
	integer_t       reusable_peak;
	uint64_t        compressed __attribute__((aligned(8)));
	uint64_t        compressed_peak __attribute__((aligned(8)));
	uint64_t        compressed_lifetime __attribute__((aligned(8)));
};

typedef struct pmap_statistics  *pmap_statistics_t;

#define PMAP_STATS_PEAK(field)                  \
	MACRO_BEGIN                             \
	if (field > field##_peak) {             \
	        field##_peak = field;           \
	}                                       \
	MACRO_END

#endif                           

  
                       
  
                 
                                                 
                                                                        
  
                    
                                                                     
  
                    
                                                      
  
                     
                                                              
  
                         
                                                            
                                                                          
                                                                          
                                                                        
  
                     
                                                                 
                                                   
  
                    
                                                                   
                                                                   
                                                               
   

#define VM_FLAGS_FIXED          0x0000
#define VM_FLAGS_ANYWHERE       0x0001
#define VM_FLAGS_PURGABLE       0x0002
#define VM_FLAGS_4GB_CHUNK      0x0004
#define VM_FLAGS_RANDOM_ADDR    0x0008
#define VM_FLAGS_NO_CACHE       0x0010
#define VM_FLAGS_RESILIENT_CODESIGN     0x0020
#define VM_FLAGS_RESILIENT_MEDIA        0x0040
#define VM_FLAGS_PERMANENT      0x0080
#define VM_FLAGS_OVERWRITE      0x4000                                          
  
                          
                                                                    
                                                        
   
#define VM_FLAGS_SUPERPAGE_MASK 0x70000                                     
#define VM_FLAGS_RETURN_DATA_ADDR       0x100000                                                              
#define VM_FLAGS_RETURN_4K_DATA_ADDR    0x800000                                               
#define VM_FLAGS_ALIAS_MASK     0xFF000000
#define VM_GET_FLAGS_ALIAS(flags, alias)                        \
	        (alias) = ((flags) & VM_FLAGS_ALIAS_MASK) >> 24
#if !XNU_KERNEL_PRIVATE
#define VM_SET_FLAGS_ALIAS(flags, alias)                        \
	        (flags) = (((flags) & ~VM_FLAGS_ALIAS_MASK) |   \
	        (((alias) & ~VM_FLAGS_ALIAS_MASK) << 24))
#endif                          

                                                        
#define VM_FLAGS_USER_ALLOCATE  (VM_FLAGS_FIXED |               \
	                         VM_FLAGS_ANYWHERE |            \
	                         VM_FLAGS_PURGABLE |            \
	                         VM_FLAGS_4GB_CHUNK |           \
	                         VM_FLAGS_RANDOM_ADDR |         \
	                         VM_FLAGS_NO_CACHE |            \
	                         VM_FLAGS_PERMANENT |           \
	                         VM_FLAGS_OVERWRITE |           \
	                         VM_FLAGS_SUPERPAGE_MASK |      \
	                         VM_FLAGS_ALIAS_MASK)
#define VM_FLAGS_USER_MAP       (VM_FLAGS_USER_ALLOCATE |       \
	                         VM_FLAGS_RETURN_4K_DATA_ADDR | \
	                         VM_FLAGS_RETURN_DATA_ADDR)
#define VM_FLAGS_USER_REMAP     (VM_FLAGS_FIXED |               \
	                         VM_FLAGS_ANYWHERE |            \
	                         VM_FLAGS_RANDOM_ADDR |         \
	                         VM_FLAGS_OVERWRITE|            \
	                         VM_FLAGS_RETURN_DATA_ADDR |    \
	                         VM_FLAGS_RESILIENT_CODESIGN |  \
	                         VM_FLAGS_RESILIENT_MEDIA)

#define VM_FLAGS_SUPERPAGE_SHIFT 16
#define SUPERPAGE_NONE                  0                                             
#define SUPERPAGE_SIZE_ANY              1
#define VM_FLAGS_SUPERPAGE_NONE     (SUPERPAGE_NONE     << VM_FLAGS_SUPERPAGE_SHIFT)
#define VM_FLAGS_SUPERPAGE_SIZE_ANY (SUPERPAGE_SIZE_ANY << VM_FLAGS_SUPERPAGE_SHIFT)
#if defined(__x86_64__) || !defined(KERNEL)
#define SUPERPAGE_SIZE_2MB              2
#define VM_FLAGS_SUPERPAGE_SIZE_2MB (SUPERPAGE_SIZE_2MB<<VM_FLAGS_SUPERPAGE_SHIFT)
#endif

  
                                            
   
#define GUARD_TYPE_VIRT_MEMORY  0x5

                                              
enum virtual_memory_guard_exception_codes {
	kGUARD_EXC_DEALLOC_GAP  = 1u << 0
};

#ifdef KERNEL_PRIVATE
typedef struct {
	unsigned int
	    vmkf_atomic_entry:1,
	    vmkf_permanent:1,
	    vmkf_guard_after:1,
	    vmkf_guard_before:1,
	    vmkf_submap:1,
	    vmkf_already:1,
	    vmkf_beyond_max:1,
	    vmkf_no_pmap_check:1,
	    vmkf_map_jit:1,
	    vmkf_iokit_acct:1,
	    vmkf_keep_map_locked:1,
	    vmkf_fourk:1,
	    vmkf_overwrite_immutable:1,
	    vmkf_remap_prot_copy:1,
	    vmkf_cs_enforcement_override:1,
	    vmkf_cs_enforcement:1,
	    vmkf_nested_pmap:1,
	    vmkf_no_copy_on_read:1,
	    vmkf_32bit_map_va:1,
	    vmkf_copy_single_object:1,
	    vmkf_copy_pageable:1,
	    vmkf_copy_same_map:1,
	    vmkf_translated_allow_execute:1,
	    __vmkf_unused:9;
} vm_map_kernel_flags_t;
#define VM_MAP_KERNEL_FLAGS_NONE (vm_map_kernel_flags_t) {              \
	.vmkf_atomic_entry = 0,                                         \
	.vmkf_permanent = 0,                                            \
	.vmkf_guard_after = 0,                                          \
	.vmkf_guard_before = 0,                                         \
	.vmkf_submap = 0,                                               \
	.vmkf_already = 0,                                              \
	.vmkf_beyond_max = 0,                                           \
	.vmkf_no_pmap_check = 0,                                        \
	.vmkf_map_jit = 0,                                              \
	.vmkf_iokit_acct = 0,                                           \
	.vmkf_keep_map_locked = 0,                                                          \
	.vmkf_fourk = 0,                                                \
	.vmkf_overwrite_immutable = 0,                                         \
	.vmkf_remap_prot_copy = 0,                                      \
	.vmkf_cs_enforcement_override = 0,                               \
	.vmkf_cs_enforcement = 0,                                       \
	.vmkf_nested_pmap = 0,                                          \
	.vmkf_no_copy_on_read = 0,                                      \
	.vmkf_32bit_map_va = 0,                                         \
	.vmkf_copy_single_object = 0,                                    \
	.vmkf_copy_pageable = 0,                                         \
	.vmkf_copy_same_map = 0,                                            \
	.vmkf_translated_allow_execute = 0,                                             \
	.__vmkf_unused = 0                                              \
}

typedef struct {
	unsigned int
	    vmnekf_ledger_tag:3,
	    vmnekf_ledger_no_footprint:1,
	    __vmnekf_unused:28;
} vm_named_entry_kernel_flags_t;
#define VM_NAMED_ENTRY_KERNEL_FLAGS_NONE (vm_named_entry_kernel_flags_t) {    \
	.vmnekf_ledger_tag = 0,                                                \
	.vmnekf_ledger_no_footprint = 0,                                       \
	.__vmnekf_unused = 0                                                   \
}

#endif                     

                                 
#define __VM_LEDGER_ACCOUNTING_POSTMARK 2019032600

                      
#define VM_LEDGER_TAG_NONE      0x00000000
#define VM_LEDGER_TAG_DEFAULT   0x00000001
#define VM_LEDGER_TAG_NETWORK   0x00000002
#define VM_LEDGER_TAG_MEDIA     0x00000003
#define VM_LEDGER_TAG_GRAPHICS  0x00000004
#define VM_LEDGER_TAG_NEURAL    0x00000005
#define VM_LEDGER_TAG_MAX       0x00000005
                      
#define VM_LEDGER_FLAG_NO_FOOTPRINT     0x00000001
#define VM_LEDGER_FLAGS (VM_LEDGER_FLAG_NO_FOOTPRINT)


#define VM_MEMORY_MALLOC 1
#define VM_MEMORY_MALLOC_SMALL 2
#define VM_MEMORY_MALLOC_LARGE 3
#define VM_MEMORY_MALLOC_HUGE 4
#define VM_MEMORY_SBRK 5                                      
#define VM_MEMORY_REALLOC 6
#define VM_MEMORY_MALLOC_TINY 7
#define VM_MEMORY_MALLOC_LARGE_REUSABLE 8
#define VM_MEMORY_MALLOC_LARGE_REUSED 9

#define VM_MEMORY_ANALYSIS_TOOL 10

#define VM_MEMORY_MALLOC_NANO 11
#define VM_MEMORY_MALLOC_MEDIUM 12
#define VM_MEMORY_MALLOC_PGUARD 13

#define VM_MEMORY_MACH_MSG 20
#define VM_MEMORY_IOKIT 21
#define VM_MEMORY_STACK  30
#define VM_MEMORY_GUARD  31
#define VM_MEMORY_SHARED_PMAP 32
                               
#define VM_MEMORY_DYLIB 33
#define VM_MEMORY_OBJC_DISPATCHERS 34

                                                                           
#define VM_MEMORY_UNSHARED_PMAP 35


                                                                        
                                                      
#define VM_MEMORY_APPKIT 40
#define VM_MEMORY_FOUNDATION 41
#define VM_MEMORY_COREGRAPHICS 42
#define VM_MEMORY_CORESERVICES 43
#define VM_MEMORY_CARBON VM_MEMORY_CORESERVICES
#define VM_MEMORY_JAVA 44
#define VM_MEMORY_COREDATA 45
#define VM_MEMORY_COREDATA_OBJECTIDS 46
#define VM_MEMORY_ATS 50
#define VM_MEMORY_LAYERKIT 51
#define VM_MEMORY_CGIMAGE 52
#define VM_MEMORY_TCMALLOC 53

                                                                   
#define VM_MEMORY_COREGRAPHICS_DATA     54

                                  
#define VM_MEMORY_COREGRAPHICS_SHARED   55

                                                                     
#define VM_MEMORY_COREGRAPHICS_FRAMEBUFFERS     56

                                                                              
#define VM_MEMORY_COREGRAPHICS_BACKINGSTORES    57

                      
#define VM_MEMORY_COREGRAPHICS_XALLOC 58

                                                                      
#define VM_MEMORY_COREGRAPHICS_MISC VM_MEMORY_COREGRAPHICS

                                                       
#define VM_MEMORY_DYLD 60
                                     
#define VM_MEMORY_DYLD_MALLOC 61

                                
#define VM_MEMORY_SQLITE 62

                          
#define VM_MEMORY_JAVASCRIPT_CORE 63
#define VM_MEMORY_WEBASSEMBLY VM_MEMORY_JAVASCRIPT_CORE
                                  
#define VM_MEMORY_JAVASCRIPT_JIT_EXECUTABLE_ALLOCATOR 64
#define VM_MEMORY_JAVASCRIPT_JIT_REGISTER_FILE 65

                               
#define VM_MEMORY_GLSL  66

                                           
#define VM_MEMORY_OPENCL    67

                                               
#define VM_MEMORY_COREIMAGE 68

                                                    
#define VM_MEMORY_WEBCORE_PURGEABLE_BUFFERS 69

                    
#define VM_MEMORY_IMAGEIO       70

                        
#define VM_MEMORY_COREPROFILE   71

                                      
#define VM_MEMORY_ASSETSD       72

                                    
#define VM_MEMORY_OS_ALLOC_ONCE 73

                                    
#define VM_MEMORY_LIBDISPATCH 74

                                               
#define VM_MEMORY_ACCELERATE 75

                             
#define VM_MEMORY_COREUI 76

                       
#define VM_MEMORY_COREUIFILE 77

                       
#define VM_MEMORY_GENEALOGY 78

                                   
#define VM_MEMORY_RAWCAMERA 79

                                  
#define VM_MEMORY_CORPSEINFO 80

                                        
#define VM_MEMORY_ASL 81

                   
#define VM_MEMORY_SWIFT_RUNTIME 82

                    
#define VM_MEMORY_SWIFT_METADATA 83

               
#define VM_MEMORY_DHMM 84


                                            
#define VM_MEMORY_SCENEKIT 86

                                            
#define VM_MEMORY_SKYWALK 87

#define VM_MEMORY_IOSURFACE 88

#define VM_MEMORY_LIBNETWORK 89

#define VM_MEMORY_AUDIO 90

#define VM_MEMORY_VIDEOBITSTREAM 91

                                   
#define VM_MEMORY_CM_XPC 92

#define VM_MEMORY_CM_RPC 93

#define VM_MEMORY_CM_MEMORYPOOL 94

#define VM_MEMORY_CM_READCACHE 95

#define VM_MEMORY_CM_CRABS 96

                                                
#define VM_MEMORY_QUICKLOOK_THUMBNAILS 97

                                            
#define VM_MEMORY_ACCOUNTS 98

                                                     
#define VM_MEMORY_SANITIZER 99

                                                                                              
#define VM_MEMORY_IOACCELERATOR 100

                                                                           
#define VM_MEMORY_CM_REGWARP 101

                                                                        
#define VM_MEMORY_EAR_DECODER 102

                              
#define VM_MEMORY_COREUI_CACHED_IMAGE_DATA 103

                                 
#define VM_MEMORY_ROSETTA 230
#define VM_MEMORY_ROSETTA_THREAD_CONTEXT 231
#define VM_MEMORY_ROSETTA_INDIRECT_BRANCH_MAP 232
#define VM_MEMORY_ROSETTA_RETURN_STACK 233
#define VM_MEMORY_ROSETTA_EXECUTABLE_HEAP 234
#define VM_MEMORY_ROSETTA_USER_LDT 235
#define VM_MEMORY_ROSETTA_ARENA 236
#define VM_MEMORY_ROSETTA_10 239

                                     
#define VM_MEMORY_APPLICATION_SPECIFIC_1 240
#define VM_MEMORY_APPLICATION_SPECIFIC_16 255

#if !XNU_KERNEL_PRIVATE
#define VM_MAKE_TAG(tag) ((tag) << 24)
#endif                         


#if KERNEL_PRIVATE

                     
                                                 

#define VM_KERN_MEMORY_NONE             0

#define VM_KERN_MEMORY_OSFMK            1
#define VM_KERN_MEMORY_BSD              2
#define VM_KERN_MEMORY_IOKIT            3
#define VM_KERN_MEMORY_LIBKERN          4
#define VM_KERN_MEMORY_OSKEXT           5
#define VM_KERN_MEMORY_KEXT             6
#define VM_KERN_MEMORY_IPC              7
#define VM_KERN_MEMORY_STACK            8
#define VM_KERN_MEMORY_CPU              9
#define VM_KERN_MEMORY_PMAP             10
#define VM_KERN_MEMORY_PTE              11
#define VM_KERN_MEMORY_ZONE             12
#define VM_KERN_MEMORY_KALLOC           13
#define VM_KERN_MEMORY_COMPRESSOR       14
#define VM_KERN_MEMORY_COMPRESSED_DATA  15
#define VM_KERN_MEMORY_PHANTOM_CACHE    16
#define VM_KERN_MEMORY_WAITQ            17
#define VM_KERN_MEMORY_DIAG             18
#define VM_KERN_MEMORY_LOG              19
#define VM_KERN_MEMORY_FILE             20
#define VM_KERN_MEMORY_MBUF             21
#define VM_KERN_MEMORY_UBC              22
#define VM_KERN_MEMORY_SECURITY         23
#define VM_KERN_MEMORY_MLOCK            24
#define VM_KERN_MEMORY_REASON           25
#define VM_KERN_MEMORY_SKYWALK          26
#define VM_KERN_MEMORY_LTABLE           27
#define VM_KERN_MEMORY_HV               28
#define VM_KERN_MEMORY_RETIRED          29

#define VM_KERN_MEMORY_FIRST_DYNAMIC    30
                  
#define VM_KERN_MEMORY_ANY              255
#define VM_KERN_MEMORY_COUNT            256

                         

                         
#define VM_KERN_SITE_TYPE               0x000000FF
#define VM_KERN_SITE_TAG                0x00000000
#define VM_KERN_SITE_KMOD               0x00000001
#define VM_KERN_SITE_KERNEL             0x00000002
#define VM_KERN_SITE_COUNTER            0x00000003
#define VM_KERN_SITE_WIRED              0x00000100                              
#define VM_KERN_SITE_HIDE               0x00000200                     
#define VM_KERN_SITE_NAMED              0x00000400
#define VM_KERN_SITE_ZONE               0x00000800
#define VM_KERN_SITE_ZONE_VIEW          0x00001000

#define VM_KERN_COUNT_MANAGED           0
#define VM_KERN_COUNT_RESERVED          1
#define VM_KERN_COUNT_WIRED             2
#define VM_KERN_COUNT_WIRED_MANAGED     3
#define VM_KERN_COUNT_STOLEN            4
#define VM_KERN_COUNT_LOPAGE            5
#define VM_KERN_COUNT_MAP_KERNEL        6
#define VM_KERN_COUNT_MAP_ZONE          7
#define VM_KERN_COUNT_MAP_KALLOC        8

#define VM_KERN_COUNT_WIRED_BOOT    9

#define VM_KERN_COUNT_BOOT_STOLEN       10

                                                                        
#define VM_KERN_COUNT_WIRED_STATIC_KERNELCACHE 11

#define VM_KERN_COUNTER_COUNT           12

#endif                     

#ifdef __cplusplus
}
#endif

#endif                              
