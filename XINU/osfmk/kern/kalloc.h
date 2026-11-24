/*
 * Copyright (c) 2000-2020 Apple Computer, Inc. All rights reserved.
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

#ifdef  KERNEL_PRIVATE

#ifndef _KERN_KALLOC_H_
#define _KERN_KALLOC_H_

#include <mach/machine/vm_types.h>
#include <mach/boolean.h>
#include <mach/vm_types.h>
#include <kern/zalloc.h>

__BEGIN_DECLS

#if XNU_KERNEL_PRIVATE

   
                         
  
            
                                                         
                           
   
typedef struct kalloc_heap {
	struct kheap_zones *kh_zones;
	zone_stats_t        kh_stats;
	const char         *kh_name;
	struct kalloc_heap *kh_next;
	zone_kheap_id_t     kh_heap_id;
} *kalloc_heap_t;

   
                             
  
            
                                                       
  
              
                                                                    
                                                              
   
#define KALLOC_HEAP_DECLARE(var) \
	extern struct kalloc_heap var[1]

   
                   
  
         
                                                                             
                                               
  
              
                                                                          
                           
  
                                                                        
                                                        
   
#define KHEAP_ANY  ((struct kalloc_heap *)NULL)

   
                            
  
         
                                           
  
              
                                                                              
                           
  
                                                                             
                                                                            
  
                                                                              
                                                                              
                                                             
  
                                                                         
              
                       
                                      
                       
   
KALLOC_HEAP_DECLARE(KHEAP_DATA_BUFFERS);

   
                    
  
         
                                                  
  
              
                                                                         
                                                 
   
KALLOC_HEAP_DECLARE(KHEAP_KEXT);

   
                       
  
         
                                               
  
              
                                                                              
                                                                             
   
KALLOC_HEAP_DECLARE(KHEAP_DEFAULT);

   
                    
  
         
                                                                        
            
  
              
                                                                                
                                                                       
                                                                           
  
                                                                   
   
KALLOC_HEAP_DECLARE(KHEAP_TEMP);

   
                            
  
            
                                                          
  
              
                                                                   
                                                                     
                                                                    
  
                                                                            
                                                                        
                             
  
                                                   
                                                          
                                                 
   
#define KALLOC_HEAP_DEFINE(var, name, heap_id) \
	SECURITY_READ_ONLY_LATE(struct kalloc_heap) var[1] = { { \
	    .kh_name = name, \
	    .kh_heap_id = heap_id, \
	} }; \
	STARTUP_ARG(ZALLOC, STARTUP_RANK_LAST, kheap_startup_init, var)

#define kalloc(size) \
	kheap_alloc(KHEAP_DEFAULT, size, Z_WAITOK)

#define kalloc_flags(size, flags) \
	kheap_alloc(KHEAP_DEFAULT, size, flags)

#define kalloc_tag(size, itag) \
	kheap_alloc_tag(KHEAP_DEFAULT, size, Z_WAITOK, itag)

#define kalloc_tag_bt(size, itag) \
	kheap_alloc_tag_bt(KHEAP_DEFAULT, size, Z_WAITOK, itag)

#define krealloc(elem, old_size, new_size, flags) \
	kheap_realloc(KHEAP_DEFAULT, elem, old_size, new_size, flags)

  
                                                                     
       
   
#define kheap_alloc(kalloc_heap, size, flags)                           \
	({ VM_ALLOC_SITE_STATIC(0, 0);                                  \
	kalloc_ext(kalloc_heap, size, flags, &site).addr; })

#define kheap_alloc_tag(kalloc_heap, size, flags, itag)                 \
	({ VM_ALLOC_SITE_STATIC(0, (itag));                             \
	kalloc_ext(kalloc_heap, size, flags, &site).addr; })

#define kheap_alloc_tag_bt(kalloc_heap, size, flags, itag)              \
	({ VM_ALLOC_SITE_STATIC(VM_TAG_BT, (itag));                     \
	kalloc_ext(kalloc_heap, size, flags, &site).addr; })

#define kheap_realloc(kalloc_heap, elem, old_size, new_size, flags)     \
	({ VM_ALLOC_SITE_STATIC(0, 0);                                  \
	krealloc_ext(kalloc_heap, elem, old_size, new_size, flags, &site).addr; })

extern void
kfree(
	void         *data,
	vm_size_t     size);

extern void
kheap_free(
	kalloc_heap_t heap,
	void         *data,
	vm_size_t     size);

__abortlike
extern void
kheap_temp_leak_panic(thread_t self);

#else                         

extern void *kalloc(vm_size_t size) __attribute__((alloc_size(1)));

extern void  kfree(void *data, vm_size_t size);

#endif                          
#pragma mark implementation details
#if XNU_KERNEL_PRIVATE
#pragma GCC visibility push(hidden)

                                        
KALLOC_HEAP_DECLARE(KERN_OS_MALLOC);

extern void kheap_startup_init(
	kalloc_heap_t heap);


  
                                                                         
                                                                  
                                         
   
struct kalloc_result {
	void     *addr;
	vm_size_t size;
};

extern struct kalloc_result
kalloc_ext(
	kalloc_heap_t           kheap,
	vm_size_t               size,
	zalloc_flags_t          flags,
	vm_allocation_site_t   *site);

extern struct kalloc_result
krealloc_ext(
	kalloc_heap_t           kheap,
	void                   *addr,
	vm_size_t               old_size,
	vm_size_t               new_size,
	zalloc_flags_t          flags,
	vm_allocation_site_t   *site);

extern struct kalloc_result
kheap_realloc_addr(
	kalloc_heap_t           kheap,
	void                   *addr,
	vm_size_t               new_size,
	zalloc_flags_t          flags,
	vm_allocation_site_t   *site);


                                                                             

static inline void *
kallocp_ext(
	kalloc_heap_t           kheap,
	vm_size_t              *size,
	zalloc_flags_t          flags,
	vm_allocation_site_t   *site)
{
	struct kalloc_result kar = kalloc_ext(kheap, *size, flags, site);
	*size = kar.size;
	return kar.addr;
}

#define kallocp(sizep)                                  \
	({ VM_ALLOC_SITE_STATIC(0, 0);                  \
	kallocp_ext(KHEAP_DEFAULT, sizep, Z_WAITOK, &site); })

#define kallocp_tag(sizep, itag)                        \
	({ VM_ALLOC_SITE_STATIC(0, (itag));             \
	kallocp_ext(KHEAP_DEFAULT, sizep, Z_WAITOK, &site); })

#define kallocp_tag_bt(sizep, itag)                     \
	({ VM_ALLOC_SITE_STATIC(VM_TAG_BT, (itag));     \
	kallocp_ext(KHEAP_DEFAULT, sizep, Z_WAITOK, &site); })

extern vm_size_t
kalloc_size(
	void                 *addr);

extern void
kheap_free_addr(
	kalloc_heap_t         heap,
	void                 *addr);

extern vm_size_t
kalloc_bucket_size(
	vm_size_t             size);

  
                                           
  
                                                                           
                                                                               
   
#define kfree(elem, size) ({ \
	_Static_assert(sizeof(elem) == sizeof(void *), "elem isn't pointer sized"); \
	__auto_type __kfree_eptr = &(elem); \
	__auto_type __kfree_elem = *__kfree_eptr; \
	__auto_type __kfree_size = (size); \
	*__kfree_eptr = (__typeof__(__kfree_elem))NULL; \
	(kfree)((void *)__kfree_elem, __kfree_size); \
})

#define kheap_free(heap, elem, size) ({ \
	_Static_assert(sizeof(elem) == sizeof(void *), "elem isn't pointer sized"); \
	__auto_type __kfree_heap = (heap); \
	__auto_type __kfree_eptr = &(elem); \
	__auto_type __kfree_elem = *__kfree_eptr; \
	__auto_type __kfree_size = (size); \
	*__kfree_eptr = (__typeof__(__kfree_elem))NULL; \
	(kheap_free)(__kfree_heap, (void *)__kfree_elem, __kfree_size); \
})

#define kheap_free_addr(heap, elem) ({ \
	_Static_assert(sizeof(elem) == sizeof(void *), "elem isn't pointer sized"); \
	__auto_type __kfree_heap = (heap); \
	__auto_type __kfree_eptr = &(elem); \
	__auto_type __kfree_elem = *__kfree_eptr; \
	*__kfree_eptr = (__typeof__(__kfree_elem))NULL; \
	(kheap_free_addr)(__kfree_heap, (void *)__kfree_elem); \
})

extern zone_t
kalloc_heap_zone_for_size(
	kalloc_heap_t             heap,
	vm_size_t           size);

extern vm_size_t kalloc_max_prerounded;
extern vm_size_t kalloc_large_total;

extern void
kern_os_kfree(
	void *addr,
	vm_size_t size);

#pragma GCC visibility pop
#endif                          

extern void
kern_os_zfree(
	zone_t zone,
	void *addr,
	vm_size_t size);

__END_DECLS

#endif                       

#endif                      
