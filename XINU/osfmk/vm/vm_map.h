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
  
   

  
                    
                                                    
             
  
                                         
  
                
                     
   

#ifndef _VM_VM_MAP_H_
#define _VM_VM_MAP_H_

#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <mach/boolean.h>
#include <mach/vm_types.h>
#include <mach/vm_prot.h>
#include <mach/vm_inherit.h>
#include <mach/vm_behavior.h>
#include <mach/vm_param.h>
#include <mach/sdt.h>
#include <vm/pmap.h>
#include <os/overflow.h>

#ifdef  KERNEL_PRIVATE

#include <sys/cdefs.h>

#ifdef XNU_KERNEL_PRIVATE
#include <vm/vm_protos.h>
#endif                         

__BEGIN_DECLS

extern void     vm_map_reference(vm_map_t       map);
extern vm_map_t current_map(void);

                                          
extern kern_return_t    vm_map_exec(
	vm_map_t                new_map,
	task_t                  task,
	boolean_t               is64bit,
	void                    *fsroot,
	cpu_type_t              cpu,
	cpu_subtype_t           cpu_subtype,
	boolean_t               reslide);

__END_DECLS

#ifdef  MACH_KERNEL_PRIVATE

#include <mach_assert.h>

#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <kern/locks.h>
#include <kern/zalloc.h>
#include <kern/macro_help.h>

#include <kern/thread.h>
#include <os/refcnt.h>

#define current_map_fast()      (current_thread()->map)
#define current_map()           (current_map_fast())

#include <vm/vm_map_store.h>


  
                 
  
                                                       
                                              
                                                                    
                                                               
                                         
   
typedef struct vm_map_entry     *vm_map_entry_t;
#define VM_MAP_ENTRY_NULL       ((vm_map_entry_t) NULL)


  
                                             
  
               
                                                      
                                                   
   
typedef union vm_map_object {
	vm_object_t             vmo_object;                        
	vm_map_t                vmo_submap;                                 
} vm_map_object_t;

#define named_entry_lock_init(object)   lck_mtx_init(&(object)->Lock, &vm_object_lck_grp, &vm_object_lck_attr)
#define named_entry_lock_destroy(object)        lck_mtx_destroy(&(object)->Lock, &vm_object_lck_grp)
#define named_entry_lock(object)                lck_mtx_lock(&(object)->Lock)
#define named_entry_unlock(object)              lck_mtx_unlock(&(object)->Lock)
#if VM_NAMED_ENTRY_LIST
extern queue_head_t vm_named_entry_list;
#endif                          

  
                                              
  
               
                                                      
  
                  
                                                             
                                                                         
                                                            
                                                         
                                                      
                                                            
                                                           
                                
  
   

struct vm_named_entry {
	decl_lck_mtx_data(, Lock);                                   
	union {
		vm_map_t        map;                                    
		vm_map_copy_t   copy;                              
	} backing;
	vm_object_offset_t      offset;                                 
	vm_object_size_t        size;                               
	vm_object_offset_t      data_offset;                                      
	vm_prot_t               protection;                             
	int                     ref_count;                                
	unsigned int                                                  
	                is_object:1,                                                            
	                internal:1,                                          
	                is_sub_map:1,                              
	                is_copy:1;                                     
#if VM_NAMED_ENTRY_LIST
	queue_chain_t           named_entry_list;
	int                     named_entry_alias;
	mach_port_t             named_entry_port;
#define NAMED_ENTRY_BT_DEPTH 16
	void                    *named_entry_bt[NAMED_ENTRY_BT_DEPTH];
#endif                          
};

  
                                            
  
               
                                           
  
                  
                                                           
                                                         
                                                             
                                                           
                                    
   

struct vm_map_links {
	struct vm_map_entry     *prev;                              
	struct vm_map_entry     *next;                          
	vm_map_offset_t         start;                             
	vm_map_offset_t         end;                             
};

  
                        
                                                                
  
                                                               
                
                                                                   
                                                                        
                                                                          
                                                            
                                            
                                                                      
                                                                        
                                                                   
                                 
  
                          
                               
                                                        
                                      
                                                           
                                   
                                                                    
                                            
       
                                                                     
                                             
   

struct vm_map_entry {
	struct vm_map_links     links;                                      
#define vme_prev                links.prev
#define vme_next                links.next
#define vme_start               links.start
#define vme_end                 links.end

	struct vm_map_store     store;
	union vm_map_object     vme_object;                            
	vm_object_offset_t      vme_offset;                             

	unsigned int
	                is_shared:1,                          
	                is_sub_map:1,                              
	                in_transition:1,                          
	                needs_wakeup:1,                               
	                    behavior:2,                                
	                                             
	                needs_copy:1,                                  

	                        
	                protection:3,                        
	                max_protection:3,                         
	                   inheritance:2,                  
	                use_pmap:1,       
                                                           
                                                    
                                                           
                                                          
                                                            
                                                       
                                    
	                no_cache:1,                                      
	                permanent:1,                                    
	                superpage_size:1,                                       
	                map_aligned:1,                                
	                zero_wired_pages:1,                               
                                                                      
                                                                
	                used_for_jit:1,
	                pmap_cs_associated:1,                            
	                from_reserved_zone:1,                  
                                                                  

	                                                                       
	                iokit_acct:1,
	                vme_resilient_codesign:1,
	                vme_resilient_media:1,
	                vme_atomic:1,                                      
	                vme_no_copy_on_read:1,
	                translated_allow_execute:1,                                      
	__unused:2;

	unsigned short          wired_count;                             
	unsigned short          user_wired_count;                  
#if     DEBUG
#define MAP_ENTRY_CREATION_DEBUG (1)
#define MAP_ENTRY_INSERTION_DEBUG (1)
#endif
#if     MAP_ENTRY_CREATION_DEBUG
	struct vm_map_header    *vme_creation_maphdr;
	uintptr_t               vme_creation_bt[16];
#endif
#if     MAP_ENTRY_INSERTION_DEBUG
	vm_map_offset_t         vme_start_original;
	vm_map_offset_t         vme_end_original;
	uintptr_t               vme_insertion_bt[16];
#endif
};

#define VME_SUBMAP_PTR(entry)                   \
	(&((entry)->vme_object.vmo_submap))
#define VME_SUBMAP(entry)                                       \
	((vm_map_t)((uintptr_t)0 + *VME_SUBMAP_PTR(entry)))
#define VME_OBJECT_PTR(entry)                   \
	(&((entry)->vme_object.vmo_object))
#define VME_OBJECT(entry)                                       \
	((vm_object_t)((uintptr_t)0 + *VME_OBJECT_PTR(entry)))
#define VME_OFFSET(entry)                       \
	((entry)->vme_offset & (vm_object_offset_t)~FOURK_PAGE_MASK)
#define VME_ALIAS_MASK (FOURK_PAGE_MASK)
#define VME_ALIAS(entry)                                        \
	((unsigned int)((entry)->vme_offset & VME_ALIAS_MASK))

static inline void
VME_OBJECT_SET(
	vm_map_entry_t entry,
	vm_object_t object)
{
	entry->vme_object.vmo_object = object;
	if (object != VM_OBJECT_NULL && !object->internal) {
		entry->vme_resilient_media = FALSE;
	}
	entry->vme_resilient_codesign = FALSE;
	entry->used_for_jit = FALSE;
}
static inline void
VME_SUBMAP_SET(
	vm_map_entry_t entry,
	vm_map_t submap)
{
	entry->vme_object.vmo_submap = submap;
}
static inline void
VME_OFFSET_SET(
	vm_map_entry_t entry,
	vm_object_offset_t offset)
{
	unsigned int alias;
	alias = VME_ALIAS(entry);
	assert((offset & FOURK_PAGE_MASK) == 0);
	entry->vme_offset = offset | alias;
}
  
             
                                                                 
                                                                 
                                               
   
static inline void
VME_ALIAS_SET(
	vm_map_entry_t entry,
	int alias)
{
	vm_object_offset_t offset;
	offset = VME_OFFSET(entry);
	entry->vme_offset = offset | ((unsigned int)alias & VME_ALIAS_MASK);
}

static inline void
VME_OBJECT_SHADOW(
	vm_map_entry_t entry,
	vm_object_size_t length)
{
	vm_object_t object;
	vm_object_offset_t offset;

	object = VME_OBJECT(entry);
	offset = VME_OFFSET(entry);
	vm_object_shadow(&object, &offset, length);
	if (object != VME_OBJECT(entry)) {
		VME_OBJECT_SET(entry, object);
		entry->use_pmap = TRUE;
	}
	if (offset != VME_OFFSET(entry)) {
		VME_OFFSET_SET(entry, offset);
	}
}


  
                                                 
                                                                       
   
#define SUPERPAGE_SIZE (PAGE_SIZE*SUPERPAGE_NBASEPAGES)
#define SUPERPAGE_MASK (-SUPERPAGE_SIZE)
#define SUPERPAGE_ROUND_DOWN(a) (a & SUPERPAGE_MASK)
#define SUPERPAGE_ROUND_UP(a) ((a + SUPERPAGE_SIZE-1) & SUPERPAGE_MASK)

  
                                                                    
                                                    
   
#define MAX_WIRE_COUNT          65535



  
                              
  
               
                                          
   


struct vm_map_header {
	struct vm_map_links     links;                                     
	int                     nentries;                              
	boolean_t               entries_pageable;
	                               
#ifdef VM_MAP_STORE_USE_RB
	struct rb_head  rb_head_store;
#endif
	int                     page_shift;                     
};

#define VM_MAP_HDR_PAGE_SHIFT(hdr) ((hdr)->page_shift)
#define VM_MAP_HDR_PAGE_SIZE(hdr) (1 << VM_MAP_HDR_PAGE_SHIFT((hdr)))
#define VM_MAP_HDR_PAGE_MASK(hdr) (VM_MAP_HDR_PAGE_SIZE((hdr)) - 1)

  
                                                 
  
               
                                                
                                                          
                           
  
                  
                                                       
                                          
                                                   
                                                   
                            
   
struct _vm_map {
	lck_rw_t                lock;                         
	struct vm_map_header    hdr;                                  
#define min_offset              hdr.links.start                     
#define max_offset              hdr.links.end                     
	pmap_t                  XNU_PTRAUTH_SIGNED_PTR("_vm_map.pmap") pmap;                             
	vm_map_size_t           size;                             
	vm_map_size_t           user_wire_limit;                                  
	vm_map_size_t           user_wire_size;                                                     
#if XNU_TARGET_OS_OSX
	vm_map_offset_t         vmmap_high_start;
#endif                        

	union {
		  
                                           
                                                             
     
		vm_map_offset_t         vmu1_highest_entry_end;
		  
                         
                                                           
                                                           
                                            
     
		vm_map_offset_t         vmu1_lowest_unnestable_start;
	} vmu1;
#define highest_entry_end       vmu1.vmu1_highest_entry_end
#define lowest_unnestable_start vmu1.vmu1_lowest_unnestable_start
	decl_lck_mtx_data(, s_lock);                                              
	lck_mtx_ext_t           s_lock_ext;
	vm_map_entry_t          hint;                                       
	union {
		struct vm_map_links* vmmap_hole_hint;                                    
		struct vm_map_corpse_footprint_header *vmmap_corpse_footprint;
	} vmmap_u_1;
#define hole_hint vmmap_u_1.vmmap_hole_hint
#define vmmap_corpse_footprint vmmap_u_1.vmmap_corpse_footprint
	union {
		vm_map_entry_t          _first_free;                               
		struct vm_map_links*    _holes;                                              
	} f_s;                                                                                               

#define first_free              f_s._first_free
#define holes_list              f_s._holes

	struct os_refcnt        map_refcnt;                            

	unsigned int
	                wait_for_space:1,                                             
	                wiring_required:1,                               
	                no_zero_fill:1,                                          
	                mapped_in_other_pmaps:1,                                                                     
	                switch_protect:1,                                                           
	                disable_vmentry_reuse:1,                                                                              
	                map_disallow_data_exec:1,                                                                          
	                holelistenabled:1,
	                is_nested_map:1,
	                map_disallow_new_exec:1,                                   
	                jit_entry_exists:1,
	                has_corpse_footprint:1,
	                terminated:1,
	                is_alien:1,                                                                     
	                cs_enforcement:1,                                      
	                cs_debugged:1,                                         
	                reserved_regions:1,                                                                                       
	                single_jit:1,                                        
	               pad:14;
	unsigned int            timestamp;                          
};

#define CAST_TO_VM_MAP_ENTRY(x) ((struct vm_map_entry *)(uintptr_t)(x))
#define vm_map_to_entry(map) CAST_TO_VM_MAP_ENTRY(&(map)->hdr.links)
#define vm_map_first_entry(map) ((map)->hdr.links.next)
#define vm_map_last_entry(map)  ((map)->hdr.links.prev)

  
                                                         
  
               
                                                           
                     
  
              
                                                            
                        
  
                  
                                      
   
typedef struct vm_map_version {
	unsigned int    main_timestamp;
} vm_map_version_t;

  
                                                      
  
               
                                                           
                                                         
               
  
                                                         
              
  
                  
                                                             
                                         
                                                   
                                                    
                                                         
  
                                               
                                                     
                           
  
                                                           
                                                             
                                                                 
  
                                                              
                                                                         
                                                            
                                                                
                                                               
                                                               
                                                 
   

struct vm_map_copy {
	int                     type;
#define VM_MAP_COPY_ENTRY_LIST          1
#define VM_MAP_COPY_OBJECT              2
#define VM_MAP_COPY_KERNEL_BUFFER       3
	vm_object_offset_t      offset;
	vm_map_size_t           size;
	union {
		struct vm_map_header                  hdr;                    
		vm_object_t                           object;             
		void *XNU_PTRAUTH_SIGNED_PTR("vm_map_copy.kdata") kdata;                     
	} c_u;
};


#define cpy_hdr                 c_u.hdr

#define cpy_object              c_u.object
#define cpy_kdata               c_u.kdata

#define VM_MAP_COPY_PAGE_SHIFT(copy) ((copy)->cpy_hdr.page_shift)
#define VM_MAP_COPY_PAGE_SIZE(copy) (1 << VM_MAP_COPY_PAGE_SHIFT((copy)))
#define VM_MAP_COPY_PAGE_MASK(copy) (VM_MAP_COPY_PAGE_SIZE((copy)) - 1)

  
                                            
   

#define vm_map_copy_to_entry(copy) CAST_TO_VM_MAP_ENTRY(&(copy)->cpy_hdr.links)
#define vm_map_copy_first_entry(copy)           \
	        ((copy)->cpy_hdr.links.next)
#define vm_map_copy_last_entry(copy)            \
	        ((copy)->cpy_hdr.links.prev)

extern kern_return_t
vm_map_copy_adjust_to_target(
	vm_map_copy_t           copy_map,
	vm_map_offset_t         offset,
	vm_map_size_t           size,
	vm_map_t                target_map,
	boolean_t               copy,
	vm_map_copy_t           *target_copy_map_p,
	vm_map_offset_t         *overmap_start_p,
	vm_map_offset_t         *overmap_end_p,
	vm_map_offset_t         *trimmed_start_p);

  
                                                 
               
                                                 
                                                             
                             
   

#define vm_map_lock_init(map)                                           \
	((map)->timestamp = 0 ,                                         \
	lck_rw_init(&(map)->lock, &vm_map_lck_grp, &vm_map_lck_rw_attr))

#define vm_map_lock(map)                     \
	MACRO_BEGIN                          \
	DTRACE_VM(vm_map_lock_w);            \
	lck_rw_lock_exclusive(&(map)->lock); \
	MACRO_END

#define vm_map_unlock(map)          \
	MACRO_BEGIN                 \
	DTRACE_VM(vm_map_unlock_w); \
	(map)->timestamp++;         \
	lck_rw_done(&(map)->lock);  \
	MACRO_END

#define vm_map_lock_read(map)             \
	MACRO_BEGIN                       \
	DTRACE_VM(vm_map_lock_r);         \
	lck_rw_lock_shared(&(map)->lock); \
	MACRO_END

#define vm_map_unlock_read(map)     \
	MACRO_BEGIN                 \
	DTRACE_VM(vm_map_unlock_r); \
	lck_rw_done(&(map)->lock);  \
	MACRO_END

#define vm_map_lock_write_to_read(map)                 \
	MACRO_BEGIN                                    \
	DTRACE_VM(vm_map_lock_downgrade);              \
	(map)->timestamp++;                            \
	lck_rw_lock_exclusive_to_shared(&(map)->lock); \
	MACRO_END

__attribute__((always_inline))
int vm_map_lock_read_to_write(vm_map_t map);

__attribute__((always_inline))
boolean_t vm_map_try_lock(vm_map_t map);

__attribute__((always_inline))
boolean_t vm_map_try_lock_read(vm_map_t map);

int vm_self_region_page_shift(vm_map_t target_map);
int vm_self_region_page_shift_safely(vm_map_t target_map);

#if MACH_ASSERT || DEBUG
#define vm_map_lock_assert_held(map) \
	lck_rw_assert(&(map)->lock, LCK_RW_ASSERT_HELD)
#define vm_map_lock_assert_shared(map)  \
	lck_rw_assert(&(map)->lock, LCK_RW_ASSERT_SHARED)
#define vm_map_lock_assert_exclusive(map) \
	lck_rw_assert(&(map)->lock, LCK_RW_ASSERT_EXCLUSIVE)
#define vm_map_lock_assert_notheld(map) \
	lck_rw_assert(&(map)->lock, LCK_RW_ASSERT_NOTHELD)
#else                            
#define vm_map_lock_assert_held(map)
#define vm_map_lock_assert_shared(map)
#define vm_map_lock_assert_exclusive(map)
#define vm_map_lock_assert_notheld(map)
#endif                           

  
                                                
   

                           
extern void             vm_map_init(void);

extern void             vm_kernel_reserved_entry_init(void);

                                                            
                                                
extern kern_return_t vm_map_find_space(
	vm_map_t                map,
	vm_map_address_t        *address,                                        
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	vm_map_entry_t          *o_entry);                                       

                                 
#define VM_MAP_FIND_LAST_FREE              0x01

extern void vm_map_clip_start(
	vm_map_t        map,
	vm_map_entry_t  entry,
	vm_map_offset_t endaddr);
extern void vm_map_clip_end(
	vm_map_t        map,
	vm_map_entry_t  entry,
	vm_map_offset_t endaddr);
extern boolean_t vm_map_entry_should_cow_for_true_share(
	vm_map_entry_t  entry);

                                                                           
extern boolean_t        vm_map_lookup_entry(
	vm_map_t                map,
	vm_map_address_t        address,
	vm_map_entry_t          *entry);                                         

extern void             vm_map_copy_remap(
	vm_map_t                map,
	vm_map_entry_t          where,
	vm_map_copy_t           copy,
	vm_map_offset_t         adjustment,
	vm_prot_t               cur_prot,
	vm_prot_t               max_prot,
	vm_inherit_t            inheritance);

                                                                         
                                                                        
extern kern_return_t    vm_map_lookup_locked(
	vm_map_t                *var_map,                                           
	vm_map_address_t        vaddr,
	vm_prot_t               fault_type,
	int                     object_lock_type,
	vm_map_version_t        *out_version,                                    
	vm_object_t             *object,                                         
	vm_object_offset_t      *offset,                                         
	vm_prot_t               *out_prot,                                       
	boolean_t               *wired,                                          
	vm_object_fault_info_t  fault_info,                                      
	vm_map_t                *real_map,                                       
	bool                    *contended);                                     

                                                                    
extern boolean_t        vm_map_verify(
	vm_map_t                map,
	vm_map_version_t        *version);                                       

extern vm_map_entry_t   vm_map_entry_insert(
	vm_map_t                map,
	vm_map_entry_t          insp_entry,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_map_kernel_flags_t   vmk_flags,
	boolean_t               needs_copy,
	boolean_t               is_shared,
	boolean_t               in_transition,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_behavior_t           behavior,
	vm_inherit_t            inheritance,
	unsigned short          wired_count,
	boolean_t               no_cache,
	boolean_t               permanent,
	boolean_t               no_copy_on_read,
	unsigned int            superpage_size,
	boolean_t               clear_map_aligned,
	boolean_t               is_submap,
	boolean_t               used_for_jit,
	int                     alias,
	boolean_t               translated_allow_execute);


  
                                  
   
#define         vm_map_min(map) ((map)->min_offset)
                          
           

#define         vm_map_max(map) ((map)->max_offset)
                           

#define         vm_map_pmap(map)        ((map)->pmap)
                          
                          

                                         
extern void             vm_map_reference(
	vm_map_t        map);

  
                                                          
                                
   
extern vm_object_t      vm_submap_object;

  
                                                        
   
#define vm_map_entry_wait(map, interruptible)           \
	((map)->timestamp++ ,                           \
	 lck_rw_sleep(&(map)->lock, LCK_SLEEP_EXCLUSIVE|LCK_SLEEP_PROMOTED_PRI, \
	                          (event_t)&(map)->hdr,	interruptible))


#define vm_map_entry_wakeup(map)        \
	thread_wakeup((event_t)(&(map)->hdr))


                          
extern void             vm_map_simplify_entry(
	vm_map_t        map,
	vm_map_entry_t  this_entry);
extern void             vm_map_simplify(
	vm_map_t                map,
	vm_map_offset_t         start);

                                                                        
extern vm_map_copy_t    vm_map_copy_copy(
	vm_map_copy_t           copy);

                                          
extern kern_return_t    vm_map_copyin_object(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	vm_map_copy_t           *copy_result);                                  

extern kern_return_t    vm_map_random_address_for_size(
	vm_map_t        map,
	vm_map_offset_t *address,
	vm_map_size_t   size);

                     
extern kern_return_t    vm_map_enter(
	vm_map_t                map,
	vm_map_offset_t         *address,
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	vm_object_t             object,
	vm_object_offset_t      offset,
	boolean_t               needs_copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance);

#if __arm64__
extern kern_return_t    vm_map_enter_fourk(
	vm_map_t                map,
	vm_map_offset_t         *address,
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	vm_object_t             object,
	vm_object_offset_t      offset,
	boolean_t               needs_copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance);
#endif                

                                                                       
extern  kern_return_t   vm_map_enter_cpm(
	vm_map_t                map,
	vm_map_address_t        *addr,
	vm_map_size_t           size,
	int                     flags);

extern kern_return_t vm_map_remap(
	vm_map_t                target_map,
	vm_map_offset_t         *address,
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	vm_map_t                src_map,
	vm_map_offset_t         memory_address,
	boolean_t               copy,
	vm_prot_t               *cur_protection,
	vm_prot_t               *max_protection,
	vm_inherit_t            inheritance);


  
                                                          
   
extern  kern_return_t   vm_map_write_user(
	vm_map_t                map,
	void                    *src_p,
	vm_map_offset_t         dst_addr,
	vm_size_t               size);

extern  kern_return_t   vm_map_read_user(
	vm_map_t                map,
	vm_map_offset_t         src_addr,
	void                    *dst_p,
	vm_size_t               size);

                                                                     
extern vm_map_t         vm_map_fork(
	ledger_t                ledger,
	vm_map_t                old_map,
	int                     options);
#define VM_MAP_FORK_SHARE_IF_INHERIT_NONE       0x00000001
#define VM_MAP_FORK_PRESERVE_PURGEABLE          0x00000002
#define VM_MAP_FORK_CORPSE_FOOTPRINT            0x00000004

                        
extern kern_return_t    vm_map_inherit(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_inherit_t            new_inheritance);

                                                                 
extern kern_return_t    vm_map_machine_attribute(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_machine_attribute_t  attribute,
	vm_machine_attribute_val_t* value);                                     

extern kern_return_t    vm_map_msync(
	vm_map_t                map,
	vm_map_address_t        address,
	vm_map_size_t           size,
	vm_sync_t               sync_flags);

                         
extern kern_return_t    vm_map_behavior_set(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_behavior_t           new_behavior);

extern kern_return_t vm_map_region(
	vm_map_t                 map,
	vm_map_offset_t         *address,
	vm_map_size_t           *size,
	vm_region_flavor_t       flavor,
	vm_region_info_t         info,
	mach_msg_type_number_t  *count,
	mach_port_t             *object_name);

extern kern_return_t vm_map_region_recurse_64(
	vm_map_t                 map,
	vm_map_offset_t         *address,
	vm_map_size_t           *size,
	natural_t               *nesting_depth,
	vm_region_submap_info_64_t info,
	mach_msg_type_number_t  *count);

extern kern_return_t vm_map_page_query_internal(
	vm_map_t                map,
	vm_map_offset_t         offset,
	int                     *disposition,
	int                     *ref_count);

extern kern_return_t vm_map_query_volatile(
	vm_map_t        map,
	mach_vm_size_t  *volatile_virtual_size_p,
	mach_vm_size_t  *volatile_resident_size_p,
	mach_vm_size_t  *volatile_compressed_size_p,
	mach_vm_size_t  *volatile_pmap_size_p,
	mach_vm_size_t  *volatile_compressed_pmap_size_p);

extern kern_return_t    vm_map_submap(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_map_t                submap,
	vm_map_offset_t         offset,
	boolean_t               use_pmap);

extern void vm_map_submap_pmap_clean(
	vm_map_t        map,
	vm_map_offset_t start,
	vm_map_offset_t end,
	vm_map_t        sub_map,
	vm_map_offset_t offset);

                                            
extern vm_map_t convert_port_entry_to_map(
	ipc_port_t      port);

                                        
extern vm_object_t convert_port_entry_to_object(
	ipc_port_t      port);


extern kern_return_t vm_map_set_cache_attr(
	vm_map_t        map,
	vm_map_offset_t va);


                                                       

#define VM_ABI_32       0x1
#define VM_ABI_64       0x2

extern int override_nx(vm_map_t map, uint32_t user_tag);


extern void vm_map_region_top_walk(
	vm_map_entry_t entry,
	vm_region_top_info_t top);
extern void vm_map_region_walk(
	vm_map_t map,
	vm_map_offset_t va,
	vm_map_entry_t entry,
	vm_object_offset_t offset,
	vm_object_size_t range,
	vm_region_extended_info_t extended,
	boolean_t look_for_pages,
	mach_msg_type_number_t count);



extern void vm_map_copy_footprint_ledgers(
	task_t  old_task,
	task_t  new_task);
extern void vm_map_copy_ledger(
	task_t  old_task,
	task_t  new_task,
	int     ledger_entry);

   
                                                                              
                                        
   
struct vm_reserved_region {
	char            *vmrr_name;
	vm_map_offset_t vmrr_addr;
	vm_map_size_t   vmrr_size;
};

   
                                                                                
                                                                        
                            
   
extern size_t ml_get_vm_reserved_regions(
	bool vm_is64bit,
	struct vm_reserved_region **regions);

#endif                          

__BEGIN_DECLS

                         
extern vm_map_t         vm_map_create(
	pmap_t                  pmap,
	vm_map_offset_t         min_off,
	vm_map_offset_t         max_off,
	boolean_t               pageable);
extern vm_map_t vm_map_create_options(
	pmap_t                  pmap,
	vm_map_offset_t         min_off,
	vm_map_offset_t         max_off,
	int                     options);
#define VM_MAP_CREATE_PAGEABLE          0x00000001
#define VM_MAP_CREATE_CORPSE_FOOTPRINT  0x00000002
#define VM_MAP_CREATE_ALL_OPTIONS (VM_MAP_CREATE_PAGEABLE | \
	                           VM_MAP_CREATE_CORPSE_FOOTPRINT)

extern vm_map_size_t    vm_map_adjusted_size(vm_map_t map);

extern void             vm_map_disable_hole_optimization(vm_map_t map);

                      
extern void             vm_map_destroy(
	vm_map_t                map,
	int                     flags);

                      
extern void             vm_map_deallocate(
	vm_map_t                map);

                      
extern void             vm_map_inspect_deallocate(
	vm_map_inspect_t        map);

                      
extern void             vm_map_read_deallocate(
	vm_map_read_t        map);

extern vm_map_t         vm_map_switch(
	vm_map_t                map);

                       
extern kern_return_t    vm_map_protect(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_prot_t               new_prot,
	boolean_t               set_max);

                      
extern boolean_t vm_map_check_protection(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_prot_t               protection);

extern boolean_t vm_map_cs_enforcement(
	vm_map_t                map);
extern void vm_map_cs_enforcement_set(
	vm_map_t                map,
	boolean_t               val);

extern void vm_map_cs_debugged_set(
	vm_map_t map,
	boolean_t val);

extern kern_return_t vm_map_cs_wx_enable(vm_map_t map);

                        

#ifdef XNU_KERNEL_PRIVATE

extern kern_return_t    vm_map_wire_kernel(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_prot_t               access_type,
	vm_tag_t                tag,
	boolean_t               user_wire);

extern kern_return_t    vm_map_wire_and_extract_kernel(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_prot_t               access_type,
	vm_tag_t                tag,
	boolean_t               user_wire,
	ppnum_t                 *physpage_p);

                            

extern kern_return_t    vm_map_wire_external(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_prot_t               access_type,
	boolean_t               user_wire);

extern kern_return_t    vm_map_wire_and_extract_external(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_prot_t               access_type,
	boolean_t               user_wire,
	ppnum_t                 *physpage_p);

#else                         

extern kern_return_t    vm_map_wire(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	vm_prot_t               access_type,
	boolean_t               user_wire);

extern kern_return_t    vm_map_wire_and_extract(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_prot_t               access_type,
	boolean_t               user_wire,
	ppnum_t                 *physpage_p);

#endif                          

                     
extern kern_return_t    vm_map_unwire(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	boolean_t               user_wire);

#ifdef XNU_KERNEL_PRIVATE

                                        
extern kern_return_t    vm_map_enter_mem_object(
	vm_map_t                map,
	vm_map_offset_t         *address,
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	ipc_port_t              port,
	vm_object_offset_t      offset,
	boolean_t               needs_copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance);

                                        
extern kern_return_t    vm_map_enter_mem_object_prefault(
	vm_map_t                map,
	vm_map_offset_t         *address,
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	ipc_port_t              port,
	vm_object_offset_t      offset,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	upl_page_list_ptr_t     page_list,
	unsigned int            page_list_count);

                                        
extern kern_return_t    vm_map_enter_mem_object_control(
	vm_map_t                map,
	vm_map_offset_t         *address,
	vm_map_size_t           size,
	vm_map_offset_t         mask,
	int                     flags,
	vm_map_kernel_flags_t   vmk_flags,
	vm_tag_t                tag,
	memory_object_control_t control,
	vm_object_offset_t      offset,
	boolean_t               needs_copy,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance);

extern kern_return_t    vm_map_terminate(
	vm_map_t                map);

extern void             vm_map_require(
	vm_map_t                map);

#endif                          

                         
extern kern_return_t    vm_map_remove(
	vm_map_t                map,
	vm_map_offset_t         start,
	vm_map_offset_t         end,
	boolean_t               flags);

                                                        
extern kern_return_t    vm_map_remove_locked(
	vm_map_t        map,
	vm_map_offset_t     start,
	vm_map_offset_t     end,
	boolean_t       flags);

                                     
extern void             vm_map_copy_discard(
	vm_map_copy_t           copy);

                                           
extern kern_return_t    vm_map_copy_overwrite(
	vm_map_t                dst_map,
	vm_map_address_t        dst_addr,
	vm_map_copy_t           copy,
	vm_map_size_t           copy_size,
	boolean_t               interruptible);

#define VM_MAP_COPY_OVERWRITE_OPTIMIZATION_THRESHOLD_PAGES      (3)


                                                                           
extern boolean_t        vm_map_copy_validate_size(
	vm_map_t                dst_map,
	vm_map_copy_t           copy,
	vm_map_size_t           *size);

                             
extern kern_return_t    vm_map_copyout(
	vm_map_t                dst_map,
	vm_map_address_t        *dst_addr,                                       
	vm_map_copy_t           copy);

extern kern_return_t vm_map_copyout_size(
	vm_map_t                dst_map,
	vm_map_address_t        *dst_addr,                                       
	vm_map_copy_t           copy,
	vm_map_size_t           copy_size);

extern kern_return_t    vm_map_copyout_internal(
	vm_map_t                dst_map,
	vm_map_address_t        *dst_addr,               
	vm_map_copy_t           copy,
	vm_map_size_t           copy_size,
	boolean_t               consume_on_success,
	vm_prot_t               cur_protection,
	vm_prot_t               max_protection,
	vm_inherit_t            inheritance);

extern kern_return_t    vm_map_copyin(
	vm_map_t                        src_map,
	vm_map_address_t        src_addr,
	vm_map_size_t           len,
	boolean_t                       src_destroy,
	vm_map_copy_t           *copy_result);                                   

extern kern_return_t    vm_map_copyin_common(
	vm_map_t                src_map,
	vm_map_address_t        src_addr,
	vm_map_size_t           len,
	boolean_t               src_destroy,
	boolean_t               src_volatile,
	vm_map_copy_t           *copy_result,                                    
	boolean_t               use_maxprot);

#define VM_MAP_COPYIN_SRC_DESTROY       0x00000001
#define VM_MAP_COPYIN_USE_MAXPROT       0x00000002
#define VM_MAP_COPYIN_ENTRY_LIST        0x00000004
#define VM_MAP_COPYIN_PRESERVE_PURGEABLE 0x00000008
#define VM_MAP_COPYIN_ALL_FLAGS         0x0000000F
extern kern_return_t    vm_map_copyin_internal(
	vm_map_t                src_map,
	vm_map_address_t        src_addr,
	vm_map_size_t           len,
	int                     flags,
	vm_map_copy_t           *copy_result);                                  

extern kern_return_t    vm_map_copy_extract(
	vm_map_t                src_map,
	vm_map_address_t        src_addr,
	vm_map_size_t           len,
	boolean_t               copy,
	vm_map_copy_t           *copy_result,            
	vm_prot_t               *cur_prot,               
	vm_prot_t               *max_prot,               
	vm_inherit_t            inheritance,
	vm_map_kernel_flags_t   vmk_flags);


extern void             vm_map_disable_NX(
	vm_map_t                map);

extern void             vm_map_disallow_data_exec(
	vm_map_t                map);

extern void             vm_map_set_64bit(
	vm_map_t                map);

extern void             vm_map_set_32bit(
	vm_map_t                map);

extern void             vm_map_set_jumbo(
	vm_map_t                map);

extern void             vm_map_set_jit_entitled(
	vm_map_t                map);

extern void             vm_map_set_max_addr(
	vm_map_t                map, vm_map_offset_t new_max_offset);

extern boolean_t        vm_map_has_hard_pagezero(
	vm_map_t                map,
	vm_map_offset_t         pagezero_size);
extern void             vm_commit_pagezero_status(vm_map_t      tmap);

#ifdef __arm__
static inline boolean_t
vm_map_is_64bit(__unused vm_map_t map)
{
	return 0;
}
#else
extern boolean_t        vm_map_is_64bit(
	vm_map_t                map);
#endif


extern kern_return_t    vm_map_raise_max_offset(
	vm_map_t        map,
	vm_map_offset_t new_max_offset);

extern kern_return_t    vm_map_raise_min_offset(
	vm_map_t        map,
	vm_map_offset_t new_min_offset);
#if XNU_TARGET_OS_OSX
extern void vm_map_set_high_start(
	vm_map_t        map,
	vm_map_offset_t high_start);
#endif                        

extern vm_map_offset_t  vm_compute_max_offset(
	boolean_t               is64);

extern void             vm_map_get_max_aslr_slide_section(
	vm_map_t                map,
	int64_t                 *max_sections,
	int64_t                 *section_size);

extern uint64_t         vm_map_get_max_aslr_slide_pages(
	vm_map_t map);

extern uint64_t         vm_map_get_max_loader_aslr_slide_pages(
	vm_map_t map);

extern void             vm_map_set_user_wire_limit(
	vm_map_t                map,
	vm_size_t               limit);

extern void vm_map_switch_protect(
	vm_map_t                map,
	boolean_t               val);

extern void vm_map_iokit_mapped_region(
	vm_map_t                map,
	vm_size_t               bytes);

extern void vm_map_iokit_unmapped_region(
	vm_map_t                map,
	vm_size_t               bytes);


extern boolean_t first_free_is_valid(vm_map_t);

extern int              vm_map_page_shift(
	vm_map_t                map);

extern vm_map_offset_t  vm_map_page_mask(
	vm_map_t                map);

extern int              vm_map_page_size(
	vm_map_t                map);

extern vm_map_offset_t  vm_map_round_page_mask(
	vm_map_offset_t         offset,
	vm_map_offset_t         mask);

extern vm_map_offset_t  vm_map_trunc_page_mask(
	vm_map_offset_t         offset,
	vm_map_offset_t         mask);

extern boolean_t        vm_map_page_aligned(
	vm_map_offset_t         offset,
	vm_map_offset_t         mask);

static inline int
vm_map_range_overflows(vm_map_offset_t addr, vm_map_size_t size)
{
	vm_map_offset_t sum;
	return os_add_overflow(addr, size, &sum);
}

static inline int
mach_vm_range_overflows(mach_vm_offset_t addr, mach_vm_size_t size)
{
	mach_vm_offset_t sum;
	return os_add_overflow(addr, size, &sum);
}

#ifdef XNU_KERNEL_PRIVATE

#if XNU_TARGET_OS_OSX
extern void vm_map_mark_alien(vm_map_t map);
extern void vm_map_single_jit(vm_map_t map);
#endif                        

extern kern_return_t vm_map_page_info(
	vm_map_t                map,
	vm_map_offset_t         offset,
	vm_page_info_flavor_t   flavor,
	vm_page_info_t          info,
	mach_msg_type_number_t  *count);
extern kern_return_t vm_map_page_range_info_internal(
	vm_map_t                map,
	vm_map_offset_t         start_offset,
	vm_map_offset_t         end_offset,
	int                     effective_page_shift,
	vm_page_info_flavor_t   flavor,
	vm_page_info_t          info,
	mach_msg_type_number_t  *count);
#endif                         


#ifdef  MACH_KERNEL_PRIVATE

  
                                                               
                                                                  
                                                     
                                                                   
                                                                  
                                                                  
                                     
   
#define vm_map_copyin(src_map, src_addr, len, src_destroy, copy_result) \
	        vm_map_copyin_common(src_map, src_addr, len, src_destroy, \
	                                FALSE, copy_result, FALSE)

#define vm_map_copyin_maxprot(src_map, \
	    src_addr, len, src_destroy, copy_result) \
	        vm_map_copyin_common(src_map, src_addr, len, src_destroy, \
	                                FALSE, copy_result, TRUE)


  
                                                                          
   
#define VM_MAP_ROUND_PAGE(x, pgmask) (((vm_map_offset_t)(x) + (pgmask)) & ~((signed)(pgmask)))
#define VM_MAP_TRUNC_PAGE(x, pgmask) ((vm_map_offset_t)(x) & ~((signed)(pgmask)))

  
                                                                 
   
#define VM_MAP_PAGE_SHIFT(map) ((map) ? (map)->hdr.page_shift : PAGE_SHIFT)
#define VM_MAP_PAGE_SIZE(map) (1 << VM_MAP_PAGE_SHIFT((map)))
#define VM_MAP_PAGE_MASK(map) (VM_MAP_PAGE_SIZE((map)) - 1)
#define VM_MAP_PAGE_ALIGNED(x, pgmask) (((x) & (pgmask)) == 0)

static inline bool
VM_MAP_IS_EXOTIC(
	vm_map_t map __unused)
{
#if __arm64__
	if (VM_MAP_PAGE_SHIFT(map) < PAGE_SHIFT ||
	    pmap_is_exotic(map->pmap)) {
		return true;
	}
#endif                
	return false;
}

static inline bool
VM_MAP_IS_ALIEN(
	vm_map_t map __unused)
{
	  
                                                         
                                 
    
#if XNU_TARGET_OS_OSX
	if (map->is_alien) {
		return true;
	}
	return false;
#else                        
	return true;
#endif                        
}

static inline bool
VM_MAP_POLICY_WX_FAIL(
	vm_map_t map __unused)
{
	if (VM_MAP_IS_ALIEN(map)) {
		return false;
	}
	return true;
}

static inline bool
VM_MAP_POLICY_WX_STRIP_X(
	vm_map_t map __unused)
{
	if (VM_MAP_IS_ALIEN(map)) {
		return true;
	}
	return false;
}

static inline bool
VM_MAP_POLICY_ALLOW_MULTIPLE_JIT(
	vm_map_t map __unused)
{
	if (VM_MAP_IS_ALIEN(map) || map->single_jit) {
		return false;
	}
	return true;
}

static inline bool
VM_MAP_POLICY_ALLOW_JIT_RANDOM_ADDRESS(
	vm_map_t map)
{
	return VM_MAP_IS_ALIEN(map);
}

static inline bool
VM_MAP_POLICY_ALLOW_JIT_INHERIT(
	vm_map_t map __unused)
{
	if (VM_MAP_IS_ALIEN(map)) {
		return false;
	}
	return true;
}

static inline bool
VM_MAP_POLICY_ALLOW_JIT_SHARING(
	vm_map_t map __unused)
{
	if (VM_MAP_IS_ALIEN(map)) {
		return false;
	}
	return true;
}

static inline bool
VM_MAP_POLICY_ALLOW_JIT_COPY(
	vm_map_t map __unused)
{
	if (VM_MAP_IS_ALIEN(map)) {
		return false;
	}
	return true;
}

static inline bool
VM_MAP_POLICY_WRITABLE_SHARED_REGION(
	vm_map_t map __unused)
{
#if __x86_64__
	return true;
#else                 
	if (VM_MAP_IS_EXOTIC(map)) {
		return true;
	}
	return false;
#endif                 
}

static inline void
vm_prot_to_wimg(unsigned int prot, unsigned int *wimg)
{
	switch (prot) {
	case MAP_MEM_NOOP:                      break;
	case MAP_MEM_IO:                        *wimg = VM_WIMG_IO; break;
	case MAP_MEM_COPYBACK:                  *wimg = VM_WIMG_USE_DEFAULT; break;
	case MAP_MEM_INNERWBACK:                *wimg = VM_WIMG_INNERWBACK; break;
	case MAP_MEM_POSTED:                    *wimg = VM_WIMG_POSTED; break;
	case MAP_MEM_POSTED_REORDERED:          *wimg = VM_WIMG_POSTED_REORDERED; break;
	case MAP_MEM_POSTED_COMBINED_REORDERED: *wimg = VM_WIMG_POSTED_COMBINED_REORDERED; break;
	case MAP_MEM_WTHRU:                     *wimg = VM_WIMG_WTHRU; break;
	case MAP_MEM_WCOMB:                     *wimg = VM_WIMG_WCOMB; break;
	case MAP_MEM_RT:                        *wimg = VM_WIMG_RT; break;
	default:                                break;
	}
}

#endif                          

#ifdef XNU_KERNEL_PRIVATE
extern kern_return_t vm_map_set_page_shift(vm_map_t map, int pageshift);
extern bool vm_map_is_exotic(vm_map_t map);
extern bool vm_map_is_alien(vm_map_t map);
#endif                         

#define vm_map_round_page(x, pgmask) (((vm_map_offset_t)(x) + (pgmask)) & ~((signed)(pgmask)))
#define vm_map_trunc_page(x, pgmask) ((vm_map_offset_t)(x) & ~((signed)(pgmask)))

  
                                                
   
#define VM_MAP_REMOVE_NO_FLAGS          0x0
#define VM_MAP_REMOVE_KUNWIRE           0x1
#define VM_MAP_REMOVE_INTERRUPTIBLE     0x2
#define VM_MAP_REMOVE_WAIT_FOR_KWIRE    0x4
#define VM_MAP_REMOVE_SAVE_ENTRIES      0x8
#define VM_MAP_REMOVE_NO_PMAP_CLEANUP   0x10
#define VM_MAP_REMOVE_NO_MAP_ALIGN      0x20
#define VM_MAP_REMOVE_NO_UNNESTING      0x40
#define VM_MAP_REMOVE_IMMUTABLE         0x80
#define VM_MAP_REMOVE_GAPS_OK           0x100

                                   

#ifdef XNU_KERNEL_PRIVATE

extern kern_return_t vm_map_get_upl(
	vm_map_t                target_map,
	vm_map_offset_t         map_offset,
	upl_size_t              *size,
	upl_t                   *upl,
	upl_page_info_array_t   page_info,
	unsigned int            *page_infoCnt,
	upl_control_flags_t     *flags,
	vm_tag_t                tag,
	int                     force_data_sync);

#endif                         

extern void
vm_map_sizes(vm_map_t map,
    vm_map_size_t * psize,
    vm_map_size_t * pfree,
    vm_map_size_t * plargest_free);

#if CONFIG_DYNAMIC_CODE_SIGNING
extern kern_return_t vm_map_sign(vm_map_t map,
    vm_map_offset_t start,
    vm_map_offset_t end);
#endif

extern kern_return_t vm_map_partial_reap(
	vm_map_t map,
	unsigned int *reclaimed_resident,
	unsigned int *reclaimed_compressed);


#if DEVELOPMENT || DEBUG

extern int vm_map_disconnect_page_mappings(
	vm_map_t map,
	boolean_t);

extern kern_return_t vm_map_inject_error(vm_map_t map, vm_map_offset_t vaddr);

#endif


#if CONFIG_FREEZE

extern kern_return_t vm_map_freeze(
	task_t       task,
	unsigned int *purgeable_count,
	unsigned int *wired_count,
	unsigned int *clean_count,
	unsigned int *dirty_count,
	unsigned int dirty_budget,
	unsigned int *shared_count,
	int          *freezer_error_code,
	boolean_t    eval_only);

#define FREEZER_ERROR_GENERIC                   (-1)
#define FREEZER_ERROR_EXCESS_SHARED_MEMORY      (-2)
#define FREEZER_ERROR_LOW_PRIVATE_SHARED_RATIO  (-3)
#define FREEZER_ERROR_NO_COMPRESSOR_SPACE       (-4)
#define FREEZER_ERROR_NO_SWAP_SPACE             (-5)

#endif

__END_DECLS

  
                                                                           
                                                                            
                                                                             
            
   
#define VM_OBJECT_ID_FAKE(map, ledger_id) ((uint32_t)(uintptr_t)VM_KERNEL_ADDRPERM((int*)((map)->pmap->ledger)+(ledger_id)))

#endif                      

#endif                     
