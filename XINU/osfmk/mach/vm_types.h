/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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
 *
 */
#ifndef _MACH_VM_TYPES_H_
#define _MACH_VM_TYPES_H_

#include <mach/port.h>
#include <mach/machine/vm_types.h>

#include <stdint.h>

typedef vm_offset_t             pointer_t;
typedef vm_offset_t             vm_address_t;

  
                                                             
                                                                   
                                                             
                                                              
   
typedef uint64_t addr64_t;                                           

  
                                                            
                                                           
                                                         
                                                        
                                                         
                                                               
                                                                 
                                                             
   
typedef uint32_t        reg64_t;

  
                                                              
                                                            
                                                         
   
typedef uint32_t ppnum_t;                                         
#define PPNUM_MAX UINT32_MAX


#ifdef  KERNEL_PRIVATE

#include <sys/cdefs.h>

#ifndef MACH_KERNEL_PRIVATE
  
                                                      
                                                        
                                                        
            
   
__BEGIN_DECLS

struct pmap;
struct _vm_map;
struct vm_object;

__END_DECLS

#endif                           

typedef struct pmap             *pmap_t;
typedef struct _vm_map          *vm_map_t, *vm_map_read_t, *vm_map_inspect_t;
typedef struct vm_object        *vm_object_t;
typedef struct vm_object_fault_info     *vm_object_fault_info_t;

#define PMAP_NULL               ((pmap_t) NULL)
#define VM_OBJECT_NULL  ((vm_object_t) NULL)

#else                       

typedef mach_port_t             vm_map_t, vm_map_read_t, vm_map_inspect_t;

#endif                      

#ifdef KERNEL
#define VM_MAP_NULL             ((vm_map_t) NULL)
#define VM_MAP_INSPECT_NULL     ((vm_map_inspect_t) NULL)
#define VM_MAP_READ_NULL        ((vm_map_read_t) NULL)
#else
#define VM_MAP_NULL             ((vm_map_t) 0)
#define VM_MAP_INSPECT_NULL     ((vm_map_inspect_t) 0)
#define VM_MAP_READ_NULL        ((vm_map_read_t) 0)
#endif

  
                                          
   

typedef uint64_t                vm_object_offset_t;
typedef uint64_t                vm_object_size_t;


#ifdef XNU_KERNEL_PRIVATE

#define VM_TAG_ACTIVE_UPDATE    1

typedef uint16_t vm_tag_t;

#define VM_TAG_NAME_LEN_MAX     0x7F
#define VM_TAG_NAME_LEN_SHIFT   0
#define VM_TAG_BT               0x0080
#define VM_TAG_UNLOAD           0x0100
#define VM_TAG_KMOD             0x0200

#if DEBUG || DEVELOPMENT
#if __LP64__
#define VM_MAX_TAG_ZONES        84
#else
#define VM_MAX_TAG_ZONES        31
#endif
#else
#define VM_MAX_TAG_ZONES        0
#endif

#if VM_MAX_TAG_ZONES
                         
#define VM_MAX_TAG_VALUE        1536
#else
#define VM_MAX_TAG_VALUE        256
#endif


#define ARRAY_COUNT(a)  (sizeof((a)) / sizeof((a)[0]))

struct vm_allocation_total {
	vm_tag_t tag;
	uint64_t total;
};

struct vm_allocation_zone_total {
	vm_size_t vazt_total;
	vm_size_t vazt_peak;
};
typedef struct vm_allocation_zone_total vm_allocation_zone_total_t;

struct vm_allocation_site {
	uint64_t  total;
#if DEBUG || DEVELOPMENT
	uint64_t  peak;
#endif                           
	uint64_t  mapped;
	int16_t   refcount;
	vm_tag_t  tag;
	uint16_t  flags;
	uint16_t  subtotalscount;
	struct vm_allocation_total subtotals[0];
	                                                                         
};
typedef struct vm_allocation_site vm_allocation_site_t;

#define VM_ALLOC_SITE_STATIC(iflags, itag)                                          \
	static vm_allocation_site_t site __attribute__((section("__DATA, __data"))) \
	 = { .refcount = 2, .tag = (itag), .flags = (iflags) };

extern int vmrtf_extract(uint64_t, boolean_t, unsigned long, void *, unsigned long *);
extern unsigned int vmrtfaultinfo_bufsz(void);

#endif                         

#ifdef  KERNEL_PRIVATE

#ifndef MACH_KERNEL_PRIVATE

__BEGIN_DECLS

struct upl;
struct vm_map_copy;
struct vm_named_entry;

__END_DECLS

#endif                           

typedef struct upl              *upl_t;
typedef struct vm_map_copy      *vm_map_copy_t;
typedef struct vm_named_entry   *vm_named_entry_t;

#define VM_MAP_COPY_NULL        ((vm_map_copy_t) NULL)

#else                       

typedef mach_port_t             upl_t;
typedef mach_port_t             vm_named_entry_t;

#endif                      

#ifdef KERNEL
#define UPL_NULL                ((upl_t) NULL)
#define VM_NAMED_ENTRY_NULL     ((vm_named_entry_t) NULL)
#else
#define UPL_NULL                ((upl_t) 0)
#define VM_NAMED_ENTRY_NULL     ((vm_named_entry_t) 0)
#endif

#ifdef PRIVATE
typedef struct {
	uint64_t rtfabstime;                                          
	uint64_t rtfduration;                          
	uint64_t rtfaddr;                 
	uint64_t rtfpc;                                                           
	uint64_t rtftid;             
	uint64_t rtfupid;                      
	uint64_t rtftype;              
} vm_rtfault_record_t;
#endif
#endif                         
