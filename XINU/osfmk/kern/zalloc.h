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
  
   
  
                 
                               
              
  
   

#ifdef  KERNEL_PRIVATE

#ifndef _KERN_ZALLOC_H_
#define _KERN_ZALLOC_H_

#include <mach/machine/vm_types.h>
#include <mach_debug/zone_info.h>
#include <kern/kern_types.h>
#include <sys/cdefs.h>

#if XNU_KERNEL_PRIVATE && !defined(ZALLOC_ALLOW_DEPRECATED)
#define __zalloc_deprecated(msg)       __deprecated_msg(msg)
#else
#define __zalloc_deprecated(msg)
#endif

__BEGIN_DECLS

   
                     
  
            
                          
   
typedef uint16_t zone_id_t;

   
                            
  
            
                                         
  
              
                                                               
                                           
   
__options_decl(zone_create_flags_t, uint64_t, {
	                                                 
	ZC_NONE                 = 0x00000000,

	                                                    
	ZC_SEQUESTER            = 0x00000001,
	                                                           
	ZC_NOSEQUESTER          = 0x00000002,

	                                                
	ZC_CACHING              = 0x00000010,
	                                                 
	ZC_NOCACHING            = 0x00000020,


	                                  
	ZC_PERCPU               = 0x01000000,

	                                                               
	ZC_ZFREE_CLEARMEM       = 0x02000000,

	                                                
	ZC_NOGC                 = 0x04000000,

	                                                  
	ZC_NOENCRYPT            = 0x08000000,

	                                              
	ZC_ALIGNMENT_REQUIRED   = 0x10000000,

	                                                     
	ZC_NOGZALLOC            = 0x20000000,

	                                                           
	ZC_NOCALLOUT            = 0x40000000,

	                                                      
	ZC_DESTRUCTIBLE         = 0x80000000,

#ifdef XNU_KERNEL_PRIVATE

	                                        
	ZC_KALLOC_HEAP          = 0x0800000000000000,

	                                                  
	ZC_ALLOW_FOREIGN        = 0x1000000000000000,

	                                                   
	ZC_DATA_BUFFERS         = 0x2000000000000000,

	                                             
	ZC_KASAN_NOQUARANTINE   = 0x4000000000000000,

	                                           
	ZC_KASAN_NOREDZONE      = 0x8000000000000000,
#endif
});

   
                      
  
            
                                                               
  
              
                                                                
                        
   
union zone_or_view {
	struct zone_view *zov_view;
	struct zone      *zov_zone;
#ifdef __cplusplus
	inline zone_or_view(struct zone_view *zv) : zov_view(zv) {
	}
	inline zone_or_view(struct zone *z) : zov_zone(z) {
	}
#endif
};
#ifdef __cplusplus
typedef union zone_or_view zone_or_view_t;
#else
typedef union zone_or_view zone_or_view_t __attribute__((transparent_union));
#endif

   
                        
  
            
                                                
  
              
                                                                                
  
                                                  
                                                                       
                                                              
  
                                                                
   
extern zone_t   zone_create(
	const char             *name,
	vm_size_t               size,
	zone_create_flags_t     flags);

   
                     
  
            
                                                    
  
              
                                                                          
                                                       
  
                                            
   
extern void     zdestroy(
	zone_t          zone);

   
                         
  
            
                                                                
  
              
                                                                         
                                   
  
                                               
                                      
                                 
  
                                                                
                                                     
   
extern void     zone_require(
	zone_t          zone,
	void           *addr);

   
                       
  
         
                                                                     
  
              
                                                                        
                                                                             
                                 
  
                                                            
                               
  
                  
                                                               
                                                          
  
                  
                                                                    
  
                      
                                                                     
                                                                  
                         
  
                
                                                                         
  
                  
                                                                           
                                                                         
  
                                                                               
                                
  
                       
  
                       
                                                                        
                                                           
       
   
__options_decl(zalloc_flags_t, uint32_t, {
	                                                                         
	Z_WAITOK        = 0x0000,
	Z_NOWAIT        = 0x0001,
	Z_NOPAGEWAIT    = 0x0002,
	Z_ZERO          = 0x0004,

	Z_NOFAIL        = 0x8000,
#if XNU_KERNEL_PRIVATE
	                                                  
	Z_VM_TAG_MASK   = 0xffff0000,

#define Z_VM_TAG_SHIFT        16
#define Z_VM_TAG(tag)         ((zalloc_flags_t)(tag) << Z_VM_TAG_SHIFT)
#endif
});

   
                   
  
            
                                              
  
              
                                                                          
  
                                                              
  
                                                     
   
extern void    *zalloc(
	zone_or_view_t  zone_or_view);

   
                           
  
            
                                                                
  
              
                                                                 
                                        
  
                                                              
  
                                                     
   
extern void    *zalloc_noblock(
	zone_or_view_t  zone_or_view);

   
                           
  
            
                                                          
  
                                                              
                                                          
  
                                                     
   
extern void    *zalloc_flags(
	zone_or_view_t  zone_or_view,
	zalloc_flags_t  flags);

   
                  
  
            
                                              
  
              
                                                                   
                             
  
                                                                     
                                           
   
extern void     zfree(
	zone_or_view_t  zone_or_view,
	void            *elem);

                     

__zalloc_deprecated("use zone_create()")
extern zone_t   zinit(
	vm_size_t       size,                                       
	vm_size_t       maxmem,                                    
	vm_size_t       alloc,                               
	const char      *name);                                  

#ifdef XNU_KERNEL_PRIVATE
#pragma mark - XNU only interfaces
#include <kern/startup.h>
#include <kern/cpu_number.h>

#pragma GCC visibility push(hidden)

#pragma mark XNU only: zalloc (extended)

#define ZALIGN_NONE             (sizeof(uint8_t)  - 1)
#define ZALIGN_16               (sizeof(uint16_t) - 1)
#define ZALIGN_32               (sizeof(uint32_t) - 1)
#define ZALIGN_PTR              (sizeof(void *)   - 1)
#define ZALIGN_64               (sizeof(uint64_t) - 1)
#define ZALIGN(t)               (_Alignof(t)      - 1)


   
                               
  
            
                                                        
  
              
                                                            
                                                              
                                                            
  
                                                                         
                                                                  
  
                                             
   
extern void    *zalloc_permanent(
	vm_size_t       size,
	vm_offset_t     align_mask);

   
                                    
  
            
                                                                            
  
              
                                                            
  
                                        
  
                                             
   
#define zalloc_permanent_type(type_t) \
	((type_t *)zalloc_permanent(sizeof(type_t), ZALIGN(type_t)))

   
                                     
  
            
                                                     
   
extern void
zalloc_first_proc_made(void);

#pragma mark XNU only: per-cpu allocations

   
                   
  
            
                                                                          
                             
   
#define __zpercpu

   
                           
  
            
                                                                    
  
                                                                              
                                                                               
  
                                                                          
   
#define zpercpu_get_cpu(ptr, cpu) \
	__zpcpu_cast(ptr, __zpcpu_demangle(ptr) + ptoa((unsigned)cpu))

   
                       
  
            
                                                                     
  
                                                                              
  
                                                                        
   
#define zpercpu_get(ptr) \
	zpercpu_get_cpu(ptr, cpu_number())

   
                           
  
            
                                          
  
                                                 
                                                                              
   
#define zpercpu_foreach(it, ptr) \
	for (typeof(ptr) it = zpercpu_get_cpu(ptr, 0), \
	    __end_##it = zpercpu_get_cpu(ptr, zpercpu_count()); \
	    it < __end_##it; it = __zpcpu_next(it))

   
                               
  
            
                                                  
  
                                                         
   
#define zpercpu_foreach_cpu(cpu) \
	for (unsigned cpu = 0; cpu < zpercpu_count(); cpu++)

   
                            
  
            
                                            
  
              
                                                                       
                                            
  
                                                              
                                                          
  
                                                     
   
extern void    *zalloc_percpu(
	zone_or_view_t  zone_or_view,
	zalloc_flags_t  flags);

   
                           
  
            
                                                                 
  
                                                                     
                                           
   
extern void     zfree_percpu(
	zone_or_view_t  zone_or_view,
	void           *addr);

   
                                      
  
            
                                                                       
  
              
                                                            
  
                                                                         
                                                                  
  
                                             
   
extern void    *zalloc_percpu_permanent(
	vm_size_t       size,
	vm_offset_t     align_mask);

   
                                           
  
            
                                                                                 
                                   
  
              
                                                            
  
                                        
  
                                             
   
#define zalloc_percpu_permanent_type(type_t) \
	((type_t *)zalloc_percpu_permanent(sizeof(type_t), ZALIGN(type_t)))

#pragma mark XNU only: zone views

   
                        
  
         
                                      
  
              
                                                                     
  
                       
                                                        
  
                          
                                                  
  
                               
                                                       
  
                       
                                               
   
__enum_decl(zone_kheap_id_t, uint32_t, {
	KHEAP_ID_NONE,
	KHEAP_ID_DEFAULT,
	KHEAP_ID_DATA_BUFFERS,
	KHEAP_ID_KEXT,

#define KHEAP_ID_COUNT (KHEAP_ID_KEXT + 1)
});

   
                        
  
            
                                                                       
                    
   
typedef struct zone_stats *__zpercpu zone_stats_t;

   
                       
  
            
                                            
  
              
                                                                             
                                                        
  
                                                                   
                                                                       
                                  
  
                                                                           
                                                                 
   
typedef struct zone_view *zone_view_t;
struct zone_view {
	zone_t          zv_zone;
	zone_stats_t    zv_stats;
	const char     *zv_name;
	zone_view_t     zv_next;
};

   
                           
  
            
                                                   
  
                                                   
   
#define ZONE_VIEW_DECLARE(var) \
	extern struct zone_view var[1]

   
                          
  
            
                                                   
  
              
                                                   
                                                      
                                         
  
                                                                     
                                                                        
                             
  
                                                   
                                                          
                                                                        
                                                                        
   
#define ZONE_VIEW_DEFINE(var, name, heap_or_zone, size) \
	SECURITY_READ_ONLY_LATE(struct zone_view) var[1] = { { \
	    .zv_name = name, \
	} }; \
	static __startup_data struct zone_view_startup_spec \
	__startup_zone_view_spec_ ## var = { var, { heap_or_zone }, size }; \
	STARTUP_ARG(ZALLOC, STARTUP_RANK_LAST, zone_view_startup_init, \
	    &__startup_zone_view_spec_ ## var)


#pragma mark XNU only: zone creation (extended)

   
                           
  
            
                                                                     
  
              
                                      
  
                                                                               
                                                   
  
                                                                             
                    
   
__enum_decl(zone_reserved_id_t, zone_id_t, {
	ZONE_ID__ZERO,

	ZONE_ID_PERMANENT,
	ZONE_ID_PERCPU_PERMANENT,

	ZONE_ID_IPC_PORT,
	ZONE_ID_IPC_PORT_SET,
	ZONE_ID_IPC_VOUCHERS,
	ZONE_ID_TASK,
	ZONE_ID_PROC,
	ZONE_ID_VM_MAP_COPY,
	ZONE_ID_PMAP,
	ZONE_ID_VM_MAP,

	ZONE_ID__FIRST_DYNAMIC,
});

   
                     
                                                                             
           
   
#define ZONE_ID_ANY ((zone_id_t)-1)

    
                      
  
                                          
                                                       
   
const char     *zone_name(
	zone_t                  zone);

    
                           
  
                                          
                                                                         
   
const char     *zone_heap_name(
	zone_t                  zone);

    
                        
  
                                          
                                                                   
   
__pure2
extern vm_map_t zone_submap(
	zone_t                  zone);

   
                            
  
            
                                                
  
              
                                                   
  
                                                  
                                                                       
                                                              
                                                                        
  
                                                                           
                                                              
                                                                 
                                               
                                                
                                             
  
                                                                
   
extern zone_t   zone_create_ext(
	const char             *name,
	vm_size_t               size,
	zone_create_flags_t     flags,
	zone_id_t               desired_zid,
	void                  (^extra_setup)(zone_t));

   
                      
  
            
                                                                          
              
  
                                                            
                                             
                                                                       
                                                              
   
#define ZONE_DECLARE(var, name, size, flags) \
	SECURITY_READ_ONLY_LATE(zone_t) var; \
	static_assert(((flags) & ZC_DESTRUCTIBLE) == 0); \
	static __startup_data struct zone_create_startup_spec \
	__startup_zone_spec_ ## var = { &var, name, size, flags, \
	    ZONE_ID_ANY, NULL }; \
	STARTUP_ARG(ZALLOC, STARTUP_RANK_MIDDLE, zone_create_startup, \
	    &__startup_zone_spec_ ## var)

   
                   
  
            
                                                                           
              
  
                                                               
                                             
                                                                       
                                                              
                                                                        
                                                                           
                                                    
   
#define ZONE_INIT(var, name, size, flags, desired_zid, extra_setup) \
	__ZONE_INIT(__LINE__, var, name, size, flags, desired_zid, extra_setup)

   
                            
  
            
                                                                                
  
              
                                                                         
                                   
  
                                                
                                                               
                                                                  
  
                                                                 
  
                                                                   
                                                           
                                                     
   
extern void     zone_id_require(
	zone_id_t               zone_id,
	vm_size_t               elem_size,
	void                   *addr);

   
                                          
  
            
                                                                                
  
              
                                                                           
                  
   
extern void     zone_id_require_allow_foreign(
	zone_id_t               zone_id,
	vm_size_t               elem_size,
	void                   *addr);

  
                      
  
                                    
                                                                
                      
  
                                  
                                                                
                                                                       
  
                       
                                    
  
                            
                                                                                
                 
   
#if defined(__LP64__)
#define Z_SUBMAP_IDX_VA_RESTRICTED  0
#else
#define Z_SUBMAP_IDX_VA_RESERVE     0
#endif
#define Z_SUBMAP_IDX_GENERAL        1
#define Z_SUBMAP_IDX_BAG_OF_BYTES   2
#define Z_SUBMAP_IDX_COUNT          3

                                                                             
extern void     zone_set_submap_idx(
	zone_t          zone,
	unsigned int    submap_idx);

                                                                                     
extern void     zone_set_noexpand(
	zone_t          zone,
	vm_size_t       max_elements);

                                                                               
extern void     zone_set_exhaustible(
	zone_t          zone,
	vm_size_t       max_elements);

   
                                
  
         
                                                                        
            
  
              
                                                                             
                     
  
                                         
                                                                  
   
extern void     zone_fill_initially(
	zone_t          zone,
	vm_size_t       nelems);

#pragma mark XNU only: misc & implementation details

  
                                          
  
                                                                          
                                                                               
   
#define zfree(zone, elem) ({ \
	_Static_assert(sizeof(elem) == sizeof(void *), "elem isn't pointer sized"); \
	__auto_type __zfree_zone = (zone); \
	__auto_type __zfree_eptr = &(elem); \
	__auto_type __zfree_elem = *__zfree_eptr; \
	*__zfree_eptr = (__typeof__(__zfree_elem))NULL; \
	(zfree)(__zfree_zone, (void *)__zfree_elem); \
})

struct zone_create_startup_spec {
	zone_t                 *z_var;
	const char             *z_name;
	vm_size_t               z_size;
	zone_create_flags_t     z_flags;
	zone_id_t               z_zid;
	void                  (^z_setup)(zone_t);
};

extern void     zone_create_startup(
	struct zone_create_startup_spec *spec);

#define __ZONE_INIT1(ns, var, name, size, flags, zid, setup) \
	static __startup_data struct zone_create_startup_spec \
	__startup_zone_spec_ ## ns = { var, name, size, flags, zid, setup }; \
	STARTUP_ARG(ZALLOC, STARTUP_RANK_MIDDLE, zone_create_startup, \
	    &__startup_zone_spec_ ## ns)

#define __ZONE_INIT(ns, var, name, size, flags, zid, setup) \
	__ZONE_INIT1(ns, var, name, size, flags, zid, setup) \

struct zone_view_startup_spec {
	zone_view_t         zv_view;
	union {
		zone_kheap_id_t zv_heapid;
		zone_t          zv_zone;
	};
	vm_size_t           zv_size;
};

extern void zone_view_startup_init(
	struct zone_view_startup_spec *spec);


#if DEBUG || DEVELOPMENT
#  if __LP64__
#    define ZPCPU_MANGLE_BIT    (1ul << 63)
#  else                
#    define ZPCPU_MANGLE_BIT    (1ul << 31)
#  endif                
#else                              
#  define ZPCPU_MANGLE_BIT      0ul
#endif                              

#define __zpcpu_mangle(ptr)     (__zpcpu_addr(ptr) & ~ZPCPU_MANGLE_BIT)
#define __zpcpu_demangle(ptr)   (__zpcpu_addr(ptr) | ZPCPU_MANGLE_BIT)
#define __zpcpu_addr(e)         ((vm_address_t)(e))
#define __zpcpu_cast(ptr, e)    ((typeof(ptr))(e))
#define __zpcpu_next(ptr)       __zpcpu_cast(ptr, __zpcpu_addr(ptr) + PAGE_SIZE)

   
                                   
  
              
                                                                                
                                                                                
                                                                             
                                                                        
  
                                                                               
                                                                              
                                                             
  
                                                                              
   
#define __zpcpu_mangle_for_boot(ptr)  ({ \
	assert(startup_phase < STARTUP_SUB_ZALLOC); \
	__zpcpu_cast(ptr, __zpcpu_mangle(__zpcpu_addr(ptr) - ptoa(cpu_number()))); \
})

extern unsigned zpercpu_count(void) __pure2;


                                                                         
extern uintptr_t hash_mix(uintptr_t);
extern uint32_t hashbacktrace(uintptr_t *, uint32_t, uint32_t);
extern uint32_t hashaddr(uintptr_t, uint32_t);

#if CONFIG_ZLEAKS
                                          

extern kern_return_t zleak_activate(void);
extern vm_size_t zleak_max_zonemap_size;
extern vm_size_t zleak_global_tracking_threshold;
extern vm_size_t zleak_per_zone_tracking_threshold;

extern int get_zleak_state(void);

#endif                    
#if DEBUG || DEVELOPMENT

extern boolean_t run_zone_test(void);
extern void zone_gc_replenish_test(void);
extern void zone_alloc_replenish_test(void);

#endif                           

#pragma GCC visibility pop
#endif                         

__END_DECLS

#endif                       

#endif                      
