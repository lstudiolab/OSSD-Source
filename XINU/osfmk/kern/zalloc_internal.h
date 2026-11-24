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
  
   

#ifndef _KERN_ZALLOC_INTERNAL_H_
#define _KERN_ZALLOC_INTERNAL_H_

#include <kern/zalloc.h>
#include <kern/locks.h>
#include <kern/btlog.h>
#include <kern/simple_lock.h>

#include <os/atomic_private.h>
#include <sys/queue.h>

#if KASAN
#include <san/kasan.h>
#include <kern/spl.h>
#endif             

   
                                 
  
            
                                                                             
                     
   

__BEGIN_DECLS

#pragma GCC visibility push(hidden)

#if CONFIG_GZALLOC
typedef struct gzalloc_data {
	uint32_t        gzfc_index;
	vm_offset_t     *gzfc;
} gzalloc_data_t;
#endif

  
                                                              
                                                               
                                                                   
                                                 
  
   

   
                      
  
         
                                                                      
  
               
                                      
                                 
                                                                           
                                                                   
   
typedef struct zone_packed_virtual_address {
	uint32_t packed_address;
} zone_pva_t;

   
                     
  
            
                                               
  
              
                                              
   
struct zone_stats {
	uint64_t            zs_mem_allocated;
	uint64_t            zs_mem_freed;
	uint32_t            zs_poison_seqno;                                          
	uint32_t            zs_alloc_rr;                             
};

STAILQ_HEAD(zone_depot, zone_magazine);

struct zone {
	  
                                    
    

	  
                                         
   
                                                                
                    
    
	struct zone        *z_self;
	zone_stats_t        z_stats;
	const char         *z_name;
	struct zone_view   *z_views;

	struct thread      *z_expander;
	struct zone_cache  *__zpercpu z_pcpu_cache;

	uint16_t            z_chunk_pages;                                           
	uint16_t            z_chunk_elems;                                      
	uint16_t            z_elems_rsv;                                             
	uint16_t            z_elem_size;                                             

	uint64_t
	  
                                            
    
	    z_destroyed        :1,                                 
	    z_async_refilling  :1,                                        
	    z_replenish_wait   :1,                                                  
	    z_expanding_wait   :1,                                        
	    z_expander_vm_priv :1,                                           

	  
                                         
    
	    z_allows_foreign   :1,                               
	    z_destructible     :1,                                 
	    kalloc_heap        :2,                                                  
	    z_noencrypt        :1,                                             
	    z_submap_idx       :2,                              
	    z_va_sequester     :1,                                                    
	    z_free_zeroes      :1,                                                            

	  
                               
    
	    z_percpu           :1,                          
	    z_permanent        :1,                                          
	    z_replenishes      :1,                                                 
	    z_nocaching        :1,                                           
	    collectable        :1,                                   
	    exhaustible        :1,                               
	    expandable         :1,                                   
	    no_callout         :1,

	    _reserved          :26,

	  
                      
    
	    alignment_required :1,                                               
	    gzalloc_tracked    :1,                                       
	    gzalloc_exempt     :1,                                                  
	    kasan_fakestacks   :1,
	    kasan_noquarantine :1,                                           
	    tag_zone_index     :7,
	    tags               :1,
	    tags_inline        :1,
	    zleak_on           :1,                                                 
	    zone_logging       :1;                                          

	  
                        
    

	lck_spin_t          z_lock;
	struct zone_depot   z_recirc;

	  
                                
   
                                                 
                                                                      
    
	uint32_t            z_wired_max;                                             
	uint32_t            z_wired_hwm;                                             
	uint32_t            z_wired_cur;                                             
	uint32_t            z_wired_empty;                                           
	uint32_t            z_va_cur;                                                

	  
                                                                        
   
                                                    
                                                                      
    
	zone_pva_t          z_pageq_empty;                                           
	zone_pva_t          z_pageq_partial;                                         
	zone_pva_t          z_pageq_full;                                            
	zone_pva_t          z_pageq_va;                                              

	  
                   
   
                     
                                                                      
                                                           
   
                     
                                                                          
                         
   
                 
                                                     
   
                 
                                          
   
                      
                                                                   
                                     
   
                     
                                                                          
                                                           
   
                  
                                              
    
#define Z_CONTENTION_WMA_UNIT (1u << 8)
	uint32_t            z_contention_wma;
	uint32_t            z_contention_cur;
	uint32_t            z_recirc_cur;
	uint32_t            z_elems_free_max;
	uint32_t            z_elems_free_wss;
	uint32_t            z_elems_free_min;
	uint32_t            z_elems_free;                                            
	uint32_t            z_elems_avail;                                           

#if CONFIG_ZLEAKS
	uint32_t            zleak_capture;                                                          
#endif
#if CONFIG_GZALLOC
	gzalloc_data_t      gz;
#endif
#if KASAN_ZALLOC
	uint32_t            z_kasan_redzone;
	spl_t               z_kasan_spl;
#endif
#if DEBUG || DEVELOPMENT || CONFIG_ZLEAKS
	                                                                                   
	btlog_t            *zlog_btlog;
#endif
};


__options_decl(zone_security_options_t, uint64_t, {
	  
                                                       
    
	ZSECURITY_OPTIONS_SEQUESTER             = 0x00000001,
	  
                                                                 
                 
    
	ZSECURITY_OPTIONS_SUBMAP_USER_DATA      = 0x00000004,
	  
                                                                   
                           
    
	ZSECURITY_OPTIONS_SEQUESTER_KEXT_KALLOC = 0x00000008,
	  
                                                                   
                                     
    
	ZSECURITY_OPTIONS_STRICT_IOKIT_FREE     = 0x00000010,
});

#define KALLOC_MINALIGN     (1 << KALLOC_LOG2_MINALIGN)
#define KALLOC_DLUT_SIZE    (2048 / KALLOC_MINALIGN)

struct kheap_zones {
	struct kalloc_zone_cfg         *cfg;
	struct kalloc_heap             *views;
	zone_kheap_id_t                 heap_id;
	uint16_t                        max_k_zone;
	uint8_t                         dlut[KALLOC_DLUT_SIZE];                                       
	uint8_t                         k_zindex_start;
	                                                                              
	zone_t                         *k_zone;
};

extern zone_security_options_t zsecurity_options;
extern zone_id_t _Atomic       num_zones;
extern uint32_t                zone_view_count;
extern struct zone             zone_array[];
extern const char * const      kalloc_heap_names[KHEAP_ID_COUNT];
extern bool                    panic_include_zprint;
#if CONFIG_ZLEAKS
extern bool                    panic_include_ztrace;
extern struct ztrace          *top_ztrace;
#endif
extern mach_memory_info_t     *panic_kext_memory_info;
extern vm_size_t               panic_kext_memory_size;
extern unsigned int            zone_map_jetsam_limit;

#define zone_index_foreach(i) \
	for (zone_id_t i = 1, num_zones_##i = os_atomic_load(&num_zones, acquire); \
	    i < num_zones_##i; i++)

#define zone_foreach(z) \
	for (zone_t z = &zone_array[1], \
	    last_zone_##z = &zone_array[os_atomic_load(&num_zones, acquire)]; \
	    z < last_zone_##z; z++)

struct zone_map_range {
	vm_offset_t min_address;
	vm_offset_t max_address;
} __attribute__((aligned(2 * sizeof(vm_offset_t))));

__pure2
static inline vm_offset_t
zone_elem_size(zone_t zone)
{
	return zone->z_elem_size;
}

static inline uint32_t
zone_count_allocated(zone_t zone)
{
	return zone->z_elems_avail - zone->z_elems_free;
}

static inline vm_size_t
zone_scale_for_percpu(zone_t zone, vm_size_t size)
{
	if (zone->z_percpu) {
		size *= zpercpu_count();
	}
	return size;
}

static inline vm_size_t
zone_size_wired(zone_t zone)
{
	  
                                      
                                               
    
	vm_size_t size = ptoa(os_atomic_load(&zone->z_wired_cur, relaxed));
	return zone_scale_for_percpu(zone, size);
}

static inline vm_size_t
zone_size_free(zone_t zone)
{
	return zone_scale_for_percpu(zone,
	           (vm_size_t)zone->z_elem_size * zone->z_elems_free);
}

static inline vm_size_t
zone_size_allocated(zone_t zone)
{
	return zone_scale_for_percpu(zone,
	           (vm_size_t)zone->z_elem_size * zone_count_allocated(zone));
}

static inline vm_size_t
zone_size_wasted(zone_t zone)
{
	return zone_size_wired(zone) - zone_scale_for_percpu(zone,
	           (vm_size_t)zone->z_elem_size * zone->z_elems_avail);
}

  
                                                                                   
                                                                               
                                                                     
   
extern uint64_t get_zones_collectable_bytes(void);

   
                        
  
                      
                                                            
                                         
  
                       
                                                              
                                                                            
  
                        
                                                                        
                                                           
                                                                       
                                     
   
__enum_closed_decl(zone_gc_level_t, uint32_t, {
	ZONE_GC_TRIM,
	ZONE_GC_DRAIN,
	ZONE_GC_JETSAM,
});

   
                    
  
         
                                                                 
  
              
                          
                                                                 
                                                                 
                                     
  
                                                    
   
extern void     zone_gc(zone_gc_level_t level);

extern void     zone_gc_trim(void);
extern void     zone_gc_drain(void);

#define ZONE_WSS_UPDATE_PERIOD  10
   
                                          
  
         
                                                 
  
              
                                                                
                                                                
                                                                   
   
extern void     compute_zone_working_set_size(void *);

                                                    
extern void     get_zone_map_size(uint64_t *current_size, uint64_t *capacity);
extern void     get_largest_zone_info(char *zone_name, size_t zone_name_len, uint64_t *zone_size);

                                              
extern void     zone_bootstrap(void);

   
                                  
  
         
                                                             
                                                               
                                                         
   
__startup_func
extern vm_offset_t zone_foreign_mem_init(
	vm_size_t       size);

   
                                        
  
         
                                                                                
                                                                           
                
   
__startup_func
extern vm_size_t zone_get_foreign_alloc_size(
	const char          *name __unused,
	vm_size_t            elem_size,
	zone_create_flags_t  flags,
	uint16_t             min_pages);

   
                              
  
         
                                                                     
  
                                                     
                                                                
                                                                     
   
__startup_func
extern void     zone_cram_foreign(
	zone_t          zone,
	vm_offset_t     newmem,
	vm_size_t       size);

extern bool     zone_maps_owned(
	vm_address_t    addr,
	vm_size_t       size);

extern void     zone_map_sizes(
	vm_map_size_t  *psize,
	vm_map_size_t  *pfree,
	vm_map_size_t  *plargest_free);

extern bool
zone_map_nearing_exhaustion(void);

#if defined(__LP64__)
#define ZONE_POISON       0xdeadbeefdeadbeef
#else
#define ZONE_POISON       0xdeadbeef
#endif

static inline vm_tag_t
zalloc_flags_get_tag(zalloc_flags_t flags)
{
	return (vm_tag_t)((flags & Z_VM_TAG_MASK) >> Z_VM_TAG_SHIFT);
}

extern void    *zalloc_ext(
	zone_t          zone,
	zone_stats_t    zstats,
	zalloc_flags_t  flags);

extern void     zfree_ext(
	zone_t          zone,
	zone_stats_t    zstats,
	void           *addr);

   
                                     
  
         
                                                                       
  
              
                                                               
   
extern void     zone_replenish_configure(
	zone_t          zone);

extern vm_size_t zone_element_size(
	void           *addr,
	zone_t         *z);

   
                      
  
            
                                                                         
                                                                       
                  
  
              
                                            
                                      
                                 
  
                                                                
                                                     
   
extern bool     zone_owns(
	zone_t          zone,
	void           *addr);

  
                                                                        
                                                                            
                                                       
   
struct ztrace {
	vm_size_t               zt_size;                                                                                                        
	uint32_t                zt_depth;                                                                   
	void*                   zt_stack[MAX_ZTRACE_DEPTH];                                                      
	uint32_t                zt_collisions;                                                                                             
	uint32_t                zt_hit_count;                                                                       
};

#ifndef VM_MAX_TAG_ZONES
#error MAX_TAG_ZONES
#endif
#if VM_MAX_TAG_ZONES

extern uint32_t zone_index_from_tag_index(
	uint32_t        tag_zone_index,
	vm_size_t      *elem_size);

#endif                       

static inline void
zone_lock(zone_t zone)
{
#if KASAN_ZALLOC
	spl_t s = 0;
	if (zone->kasan_fakestacks) {
		s = splsched();
	}
#endif                   
	lck_spin_lock(&zone->z_lock);
#if KASAN_ZALLOC
	zone->z_kasan_spl = s;
#endif                   
}

static inline void
zone_unlock(zone_t zone)
{
#if KASAN_ZALLOC
	spl_t s = zone->z_kasan_spl;
	zone->z_kasan_spl = 0;
#endif                   
	lck_spin_unlock(&zone->z_lock);
#if KASAN_ZALLOC
	if (zone->kasan_fakestacks) {
		splx(s);
	}
#endif                   
}

#if CONFIG_GZALLOC
void gzalloc_init(vm_size_t);
void gzalloc_zone_init(zone_t);
void gzalloc_empty_free_cache(zone_t);
boolean_t gzalloc_enabled(void);

vm_offset_t gzalloc_alloc(zone_t, zone_stats_t zstats, zalloc_flags_t flags);
void gzalloc_free(zone_t, zone_stats_t zstats, void *);
boolean_t gzalloc_element_size(void *, zone_t *, vm_size_t *);
#endif                     

#define MAX_ZONE_NAME   32                                                                    
int track_this_zone(const char *zonename, const char *logname);

#if DEBUG || DEVELOPMENT
extern boolean_t run_zone_test(void);
extern void zone_gc_replenish_test(void);
extern void zone_alloc_replenish_test(void);
extern vm_size_t zone_element_info(void *addr, vm_tag_t * ptag);
extern bool zalloc_disable_copyio_check;
#else
#define zalloc_disable_copyio_check false
#endif                           

#pragma GCC visibility pop

__END_DECLS

#endif                                
