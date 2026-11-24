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
  
   
  
                    
                                                    
             
  
                                            
   

#ifndef _VM_VM_OBJECT_H_
#define _VM_VM_OBJECT_H_

#include <debug.h>
#include <mach_assert.h>
#include <mach_pagemap.h>

#include <mach/kern_return.h>
#include <mach/boolean.h>
#include <mach/memory_object_types.h>
#include <mach/port.h>
#include <mach/vm_prot.h>
#include <mach/vm_param.h>
#include <mach/machine/vm_types.h>
#include <kern/queue.h>
#include <kern/locks.h>
#include <kern/assert.h>
#include <kern/misc_protos.h>
#include <kern/macro_help.h>
#include <ipc/ipc_types.h>
#include <vm/pmap.h>

#include <vm/vm_external.h>

#include <vm/vm_options.h>
#include <vm/vm_page.h>

#if VM_OBJECT_TRACKING
#include <libkern/OSDebug.h>
#include <kern/btlog.h>
extern void vm_object_tracking_init(void);
extern boolean_t vm_object_tracking_inited;
extern btlog_t *vm_object_tracking_btlog;
#define VM_OBJECT_TRACKING_NUM_RECORDS  50000
#define VM_OBJECT_TRACKING_BTDEPTH 7
#define VM_OBJECT_TRACKING_OP_CREATED   1
#define VM_OBJECT_TRACKING_OP_MODIFIED  2
#define VM_OBJECT_TRACKING_OP_TRUESHARE 3
#endif                         

struct vm_page;

  
                 
  
                                      
                                                         
   

struct vm_object_fault_info {
	int             interruptible;
	uint32_t        user_tag;
	vm_size_t       cluster_size;
	vm_behavior_t   behavior;
	vm_object_offset_t lo_offset;
	vm_object_offset_t hi_offset;
	unsigned int
	                no_cache:1,
	                stealth:1,
	                io_sync:1,
	                cs_bypass:1,
	                pmap_cs_associated:1,
	                mark_zf_absent:1,
	                batch_pmap_op:1,
	                resilient_media:1,
	                no_copy_on_read:1,
	    __vm_object_fault_info_unused_bits:23;
	int             pmap_options;
};


#define vo_size                         vo_un1.vou_size
#define vo_cache_pages_to_scan          vo_un1.vou_cache_pages_to_scan
#define vo_shadow_offset                vo_un2.vou_shadow_offset
#define vo_cache_ts                     vo_un2.vou_cache_ts
#define vo_owner                        vo_un2.vou_owner

struct vm_object {
	  
                                                             
                                                             
                                                                
                                                           
                                                               
                                                              
                                                           
                                                           
                                                         
                                                             
                                          
    
	vm_page_queue_head_t    memq;                                                
	lck_rw_t                Lock;                                

#if DEVELOPMENT || DEBUG
	thread_t                Lock_owner;
#endif
	union {
		vm_object_size_t  vou_size;                                               
		int               vou_cache_pages_to_scan;                                      
                                                                             
                                                     
	} vo_un1;

	struct vm_page          *memq_hint;
	int                     ref_count;                                
	unsigned int            resident_page_count;
	                              
	unsigned int            wired_page_count;                         
                                                                                                  
	unsigned int            reusable_page_count;

	struct vm_object        *copy;                                       
                                                                       
                                                                       
                                                                 
                                                                    
                                                      
                                            
	struct vm_object        *shadow;                       
	memory_object_t         pager;                                 

	union {
		vm_object_offset_t vou_shadow_offset;                           
		clock_sec_t     vou_cache_ts;                               
                                                     
                                     
		task_t          vou_owner;                                   
                                                                
                                                              
                                     
	} vo_un2;

	vm_object_offset_t      paging_offset;                                 
	memory_object_control_t pager_control;                             

	memory_object_copy_strategy_t
	    copy_strategy;                                                   

#if __LP64__
	  
                                                                     
                                                                   
                                                                      
                                                                    
                                       
    
	unsigned int            paging_in_progress:16,
	    __object1_unused_bits:16;
	unsigned int            activity_in_progress;
#else               
	  
                                                                        
                                                                      
                                                                 
                                
    
	unsigned short          paging_in_progress;
	unsigned short          activity_in_progress;
#endif               
	                              
                                
                               
                              
                              
              
    

	unsigned int
	                      all_wanted:11,                               
                                                                     
                                                                   
                                                    
	                pager_created:1,                                     
	                pager_initialized:1,                                  
	                pager_ready:1,                                         

	                pager_trusted:1,                                    
                                                                        
                                                                        
                                                                 
                                            
	                can_persist:1,                                         
                                                                       
                                                                       
                                                                      
                                                            
                                            
	                internal:1,                                          
                                                                     
                                                                  
                                            
	                private:1,                                           
                                                                      
	                pageout:1,                                         
                                                                       
                                                                    
	                alive:1,                                        

	                purgable:2,                                    
                                                         
                                            
	                purgeable_only_by_kernel:1,
	                purgeable_when_ripe:1,                                  
                                                                
                                                   
	                shadowed:1,                                   
	                true_share:1,
	                        
                          
                       
                
	                terminating:1,
	                          
                            
                         
                          
                         
                        
                      
    
	                named:1,                                          
                                                                 
                                                                      
                                                              
                                                                       
                                                                     
                                                    
                                            
	                shadow_severed:1,
	                          
                           
                           
                             
                          
                     
    
	                phys_contiguous:1,
	                      
                         
                        
                           
                            
                           
                
    
	                nophyscache:1,
	                     
                           
                         
        
    
	                _object5_unused_bits:1;

	queue_chain_t           cached_list;                               
                                                                       
                                                                       
                                                 
                                            
	  
                                                       
                                                
    
	vm_object_offset_t      last_alloc;                                 
	vm_offset_t             cow_hint;                                     
	                                                                      
	int                     sequential;                                 

	uint32_t                pages_created;
	uint32_t                pages_used;
	                                    
	unsigned        int
	    wimg_bits:8,                                             
	    code_signed:1,                                               
                                                                         
                                                      
	    transposed:1,                                                       
	    mapping_in_progress:1,                                       
	    phantom_isssd:1,
	    volatile_empty:1,
	    volatile_fault:1,
	    all_reusable:1,
	    blocked_access:1,
	    set_cache_attr:1,
	    object_is_shared_cache:1,
	    purgeable_queue_type:2,
	    purgeable_queue_group:3,
	    io_tracking:1,
	    no_tag_update:1,                  
#if CONFIG_SECLUDED_MEMORY
	    eligible_for_secluded:1,
	    can_grab_secluded:1,
#else                             
	__object3_unused_bits:2,
#endif                             
#if VM_OBJECT_ACCESS_TRACKING
	    access_tracking:1,
#else                                
	__unused_access_tracking:1,
#endif                                
	vo_ledger_tag:3,
	    vo_no_footprint:1;

#if VM_OBJECT_ACCESS_TRACKING
	uint32_t        access_tracking_reads;
	uint32_t        access_tracking_writes;
#endif                                

	uint8_t                 scan_collisions;
	uint8_t                 __object4_unused_bits[1];
	vm_tag_t                wire_tag;

#if CONFIG_PHANTOM_CACHE
	uint32_t                phantom_object_id;
#endif
#if CONFIG_IOSCHED || UPL_DEBUG
	queue_head_t            uplq;                                         
#endif

#ifdef  VM_PIP_DEBUG
  
                                                       
                                                          
   
#define VM_PIP_DEBUG_STACK_FRAMES       25                                     
#define VM_PIP_DEBUG_MAX_REFS           10                                      
	struct __pip_backtrace {
		void *pip_retaddr[VM_PIP_DEBUG_STACK_FRAMES];
	} pip_holders[VM_PIP_DEBUG_MAX_REFS];
#endif                     

	queue_chain_t           objq;                                                             
	queue_chain_t           task_objq;                                                     

#if !VM_TAG_ACTIVE_UPDATE
	queue_chain_t           wired_objq;
#endif                            

#if DEBUG
	void *purgeable_owner_bt[16];
	task_t vo_purgeable_volatilizer;                            
	void *purgeable_volatilizer_bt[16];
#endif            
};

#define VM_OBJECT_PURGEABLE_FAULT_ERROR(object)                         \
	((object)->volatile_fault &&                                    \
	 ((object)->purgable == VM_PURGABLE_VOLATILE ||                 \
	  (object)->purgable == VM_PURGABLE_EMPTY))

#if VM_OBJECT_ACCESS_TRACKING
extern uint64_t vm_object_access_tracking_reads;
extern uint64_t vm_object_access_tracking_writes;
extern void vm_object_access_tracking(vm_object_t object,
    int *access_tracking,
    uint32_t *access_tracking_reads,
    uint32_t *acess_tracking_writes);
#endif                                

extern
vm_object_t     kernel_object;                                        

extern
vm_object_t     compressor_object;                                        

extern
vm_object_t     retired_pages_object;                                                  

extern
unsigned int    vm_object_absent_max;                                    
                                                                        

# define        VM_MSYNC_INITIALIZED                    0
# define        VM_MSYNC_SYNCHRONIZING                  1
# define        VM_MSYNC_DONE                           2


extern lck_grp_t                vm_map_lck_grp;
extern lck_attr_t               vm_map_lck_attr;

#ifndef VM_TAG_ACTIVE_UPDATE
#error VM_TAG_ACTIVE_UPDATE
#endif

#if VM_TAG_ACTIVE_UPDATE
#define VM_OBJECT_WIRED_ENQUEUE(object) panic("VM_OBJECT_WIRED_ENQUEUE")
#define VM_OBJECT_WIRED_DEQUEUE(object) panic("VM_OBJECT_WIRED_DEQUEUE")
#else                           
#define VM_OBJECT_WIRED_ENQUEUE(object)                                 \
	MACRO_BEGIN                                                     \
	lck_spin_lock_grp(&vm_objects_wired_lock, &vm_page_lck_grp_bucket);                          \
	assert(!(object)->wired_objq.next);                             \
	assert(!(object)->wired_objq.prev);                             \
	queue_enter(&vm_objects_wired, (object),                        \
	            vm_object_t, wired_objq);                           \
	lck_spin_unlock(&vm_objects_wired_lock);                        \
	MACRO_END
#define VM_OBJECT_WIRED_DEQUEUE(object)                                 \
	MACRO_BEGIN                                                     \
	if ((object)->wired_objq.next) {                                \
	        lck_spin_lock_grp(&vm_objects_wired_lock, &vm_page_lck_grp_bucket);                  \
	        queue_remove(&vm_objects_wired, (object),               \
	                     vm_object_t, wired_objq);                  \
	        lck_spin_unlock(&vm_objects_wired_lock);                \
	}                                                               \
	MACRO_END
#endif                           

#define VM_OBJECT_WIRED(object, tag)                                    \
    MACRO_BEGIN                                                         \
    assert(VM_KERN_MEMORY_NONE != (tag));                               \
    assert(VM_KERN_MEMORY_NONE == (object)->wire_tag);                  \
    (object)->wire_tag = (tag);                                         \
    if (!VM_TAG_ACTIVE_UPDATE) {                                        \
	VM_OBJECT_WIRED_ENQUEUE((object));                              \
    }                                                                   \
    MACRO_END

#define VM_OBJECT_UNWIRED(object)                                                       \
    MACRO_BEGIN                                                                         \
    if (!VM_TAG_ACTIVE_UPDATE) {                                                        \
	    VM_OBJECT_WIRED_DEQUEUE((object));                                          \
    }                                                                                   \
    if (VM_KERN_MEMORY_NONE != (object)->wire_tag) {                                    \
	vm_tag_update_size((object)->wire_tag, -ptoa_64((object)->wired_page_count));   \
	(object)->wire_tag = VM_KERN_MEMORY_NONE;                                       \
    }                                                                                   \
    MACRO_END

                                         
#define VM_OBJECT_WIRED_PAGE_UPDATE_START(object)                                       \
    MACRO_BEGIN                                                                         \
    {                                                                                   \
	int64_t __wireddelta = 0; vm_tag_t __waswired = (object)->wire_tag;

#define VM_OBJECT_WIRED_PAGE_UPDATE_END(object, tag)                                    \
	if (__wireddelta) {                                                             \
	    boolean_t __overflow __assert_only =                                        \
	    os_add_overflow((object)->wired_page_count, __wireddelta,                   \
	                    &(object)->wired_page_count);                               \
	    assert(!__overflow);                                                        \
	    if (!(object)->pageout && !(object)->no_tag_update) {                       \
	        if (__wireddelta > 0) {                                                 \
	            assert (VM_KERN_MEMORY_NONE != (tag));                              \
	            if (VM_KERN_MEMORY_NONE == __waswired) {                            \
	                VM_OBJECT_WIRED((object), (tag));                               \
	            }                                                                   \
	            vm_tag_update_size((object)->wire_tag, ptoa_64(__wireddelta));      \
	        } else if (VM_KERN_MEMORY_NONE != __waswired) {                         \
	            assert (VM_KERN_MEMORY_NONE != (object)->wire_tag);                 \
	            vm_tag_update_size((object)->wire_tag, ptoa_64(__wireddelta));      \
	            if (!(object)->wired_page_count) {                                  \
	                VM_OBJECT_UNWIRED((object));                                    \
	            }                                                                   \
	        }                                                                       \
	    }                                                                           \
	}                                                                               \
    }                                                                                   \
    MACRO_END

#define VM_OBJECT_WIRED_PAGE_COUNT(object, delta)               \
    __wireddelta += delta; \

#define VM_OBJECT_WIRED_PAGE_ADD(object, m)                     \
    if (!(m)->vmp_private && !(m)->vmp_fictitious) __wireddelta++;

#define VM_OBJECT_WIRED_PAGE_REMOVE(object, m)                  \
    if (!(m)->vmp_private && !(m)->vmp_fictitious) __wireddelta--;



#define OBJECT_LOCK_SHARED      0
#define OBJECT_LOCK_EXCLUSIVE   1

extern lck_grp_t        vm_object_lck_grp;
extern lck_attr_t       vm_object_lck_attr;
extern lck_attr_t       kernel_object_lck_attr;
extern lck_attr_t       compressor_object_lck_attr;

extern vm_object_t      vm_pageout_scan_wants_object;

extern void             vm_object_lock(vm_object_t);
extern bool             vm_object_lock_check_contended(vm_object_t);
extern boolean_t        vm_object_lock_try(vm_object_t);
extern boolean_t        _vm_object_lock_try(vm_object_t);
extern boolean_t        vm_object_lock_avoid(vm_object_t);
extern void             vm_object_lock_shared(vm_object_t);
extern boolean_t        vm_object_lock_yield_shared(vm_object_t);
extern boolean_t        vm_object_lock_try_shared(vm_object_t);
extern void             vm_object_unlock(vm_object_t);
extern boolean_t        vm_object_lock_upgrade(vm_object_t);

  
                        
   

#define vm_object_lock_init(object)                                     \
	lck_rw_init(&(object)->Lock, &vm_object_lck_grp,                \
	            (((object) == kernel_object ||                      \
	              (object) == vm_submap_object) ?                   \
	             &kernel_object_lck_attr :                          \
	             (((object) == compressor_object) ?                 \
	             &compressor_object_lck_attr :                      \
	              &vm_object_lck_attr)))
#define vm_object_lock_destroy(object)  lck_rw_destroy(&(object)->Lock, &vm_object_lck_grp)

#define vm_object_lock_try_scan(object) _vm_object_lock_try(object)

  
                                                                     
                                                                          
                   
   
#if MACH_ASSERT || DEBUG
#define vm_object_lock_assert_held(object) \
	lck_rw_assert(&(object)->Lock, LCK_RW_ASSERT_HELD)
#define vm_object_lock_assert_shared(object) \
	lck_rw_assert(&(object)->Lock, LCK_RW_ASSERT_SHARED)
#define vm_object_lock_assert_exclusive(object) \
	lck_rw_assert(&(object)->Lock, LCK_RW_ASSERT_EXCLUSIVE)
#define vm_object_lock_assert_notheld(object) \
	lck_rw_assert(&(object)->Lock, LCK_RW_ASSERT_NOTHELD)
#else                            
#define vm_object_lock_assert_held(object)
#define vm_object_lock_assert_shared(object)
#define vm_object_lock_assert_exclusive(object)
#define vm_object_lock_assert_notheld(object)
#endif                           


  
                                                 
   

__private_extern__ void         vm_object_bootstrap(void);

__private_extern__ void         vm_object_reaper_init(void);

__private_extern__ vm_object_t  vm_object_allocate(vm_object_size_t size);

__private_extern__ void    _vm_object_allocate(vm_object_size_t size,
    vm_object_t object);

#define vm_object_reference_locked(object)              \
	MACRO_BEGIN                                     \
	vm_object_t RLObject = (object);                \
	vm_object_lock_assert_exclusive(object);        \
	assert((RLObject)->ref_count > 0);              \
	(RLObject)->ref_count++;                        \
	assert((RLObject)->ref_count > 1);              \
	MACRO_END


#define vm_object_reference_shared(object)              \
	MACRO_BEGIN                                     \
	vm_object_t RLObject = (object);                \
	vm_object_lock_assert_shared(object);           \
	assert((RLObject)->ref_count > 0);              \
	OSAddAtomic(1, &(RLObject)->ref_count);         \
	assert((RLObject)->ref_count > 0);              \
	MACRO_END


__private_extern__ void         vm_object_reference(
	vm_object_t     object);

#if     !MACH_ASSERT

#define vm_object_reference(object)                     \
MACRO_BEGIN                                             \
	vm_object_t RObject = (object);                 \
	if (RObject) {                                  \
	        vm_object_lock_shared(RObject);         \
	        vm_object_reference_shared(RObject);    \
	        vm_object_unlock(RObject);              \
	}                                               \
MACRO_END

#endif                   

__private_extern__ void         vm_object_deallocate(
	vm_object_t     object);

__private_extern__ kern_return_t vm_object_release_name(
	vm_object_t     object,
	int             flags);

__private_extern__ void         vm_object_pmap_protect(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	pmap_t                  pmap,
	vm_map_size_t           pmap_page_size,
	vm_map_offset_t         pmap_start,
	vm_prot_t               prot);

__private_extern__ void         vm_object_pmap_protect_options(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	pmap_t                  pmap,
	vm_map_size_t           pmap_page_size,
	vm_map_offset_t         pmap_start,
	vm_prot_t               prot,
	int                     options);

__private_extern__ void         vm_object_page_remove(
	vm_object_t             object,
	vm_object_offset_t      start,
	vm_object_offset_t      end);

__private_extern__ void         vm_object_deactivate_pages(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	boolean_t               kill_page,
	boolean_t               reusable_page,
	struct pmap             *pmap,
                                               
	vm_map_offset_t         pmap_offset);

__private_extern__ void vm_object_reuse_pages(
	vm_object_t             object,
	vm_object_offset_t      start_offset,
	vm_object_offset_t      end_offset,
	boolean_t               allow_partial_reuse);

__private_extern__ uint64_t     vm_object_purge(
	vm_object_t              object,
	int                      flags);

__private_extern__ kern_return_t vm_object_purgable_control(
	vm_object_t     object,
	vm_purgable_t   control,
	int             *state);

__private_extern__ kern_return_t vm_object_get_page_counts(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	unsigned int            *resident_page_count,
	unsigned int            *dirty_page_count);

__private_extern__ boolean_t    vm_object_coalesce(
	vm_object_t             prev_object,
	vm_object_t             next_object,
	vm_object_offset_t      prev_offset,
	vm_object_offset_t      next_offset,
	vm_object_size_t        prev_size,
	vm_object_size_t        next_size);

__private_extern__ boolean_t    vm_object_shadow(
	vm_object_t             *object,
	vm_object_offset_t      *offset,
	vm_object_size_t        length);

__private_extern__ void         vm_object_collapse(
	vm_object_t             object,
	vm_object_offset_t      offset,
	boolean_t               can_bypass);

__private_extern__ boolean_t    vm_object_copy_quickly(
	vm_object_t             *_object,
	vm_object_offset_t      src_offset,
	vm_object_size_t        size,
	boolean_t               *_src_needs_copy,
	boolean_t               *_dst_needs_copy);

__private_extern__ kern_return_t        vm_object_copy_strategically(
	vm_object_t             src_object,
	vm_object_offset_t      src_offset,
	vm_object_size_t        size,
	vm_object_t             *dst_object,
	vm_object_offset_t      *dst_offset,
	boolean_t               *dst_needs_copy);

__private_extern__ kern_return_t        vm_object_copy_slowly(
	vm_object_t             src_object,
	vm_object_offset_t      src_offset,
	vm_object_size_t        size,
	boolean_t               interruptible,
	vm_object_t             *_result_object);

__private_extern__ vm_object_t  vm_object_copy_delayed(
	vm_object_t             src_object,
	vm_object_offset_t      src_offset,
	vm_object_size_t        size,
	boolean_t               src_object_shared);



__private_extern__ kern_return_t        vm_object_destroy(
	vm_object_t     object,
	kern_return_t   reason);

__private_extern__ void         vm_object_pager_create(
	vm_object_t     object);

__private_extern__ void         vm_object_compressor_pager_create(
	vm_object_t     object);

__private_extern__ void         vm_object_page_map(
	vm_object_t     object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	vm_object_offset_t      (*map_fn)
	(void *, vm_object_offset_t),
	void            *map_fn_data);

__private_extern__ kern_return_t vm_object_upl_request(
	vm_object_t             object,
	vm_object_offset_t      offset,
	upl_size_t              size,
	upl_t                   *upl,
	upl_page_info_t         *page_info,
	unsigned int            *count,
	upl_control_flags_t     flags,
	vm_tag_t            tag);

__private_extern__ kern_return_t vm_object_transpose(
	vm_object_t             object1,
	vm_object_t             object2,
	vm_object_size_t        transpose_size);

__private_extern__ boolean_t vm_object_sync(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	boolean_t               should_flush,
	boolean_t               should_return,
	boolean_t               should_iosync);

__private_extern__ kern_return_t vm_object_update(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	vm_object_offset_t      *error_offset,
	int                     *io_errno,
	memory_object_return_t  should_return,
	int                     flags,
	vm_prot_t               prot);

__private_extern__ kern_return_t vm_object_lock_request(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	memory_object_return_t  should_return,
	int                     flags,
	vm_prot_t               prot);



__private_extern__ vm_object_t  vm_object_memory_object_associate(
	memory_object_t         pager,
	vm_object_t             object,
	vm_object_size_t        size,
	boolean_t               check_named);


__private_extern__ void vm_object_cluster_size(
	vm_object_t             object,
	vm_object_offset_t      *start,
	vm_size_t               *length,
	vm_object_fault_info_t  fault_info,
	uint32_t                *io_streaming);

__private_extern__ kern_return_t vm_object_populate_with_private(
	vm_object_t             object,
	vm_object_offset_t      offset,
	ppnum_t                 phys_page,
	vm_size_t               size);

__private_extern__ void vm_object_change_wimg_mode(
	vm_object_t             object,
	unsigned int            wimg_mode);

extern kern_return_t adjust_vm_object_cache(
	vm_size_t oval,
	vm_size_t nval);

extern kern_return_t vm_object_page_op(
	vm_object_t             object,
	vm_object_offset_t      offset,
	int                     ops,
	ppnum_t                 *phys_entry,
	int                     *flags);

extern kern_return_t vm_object_range_op(
	vm_object_t             object,
	vm_object_offset_t      offset_beg,
	vm_object_offset_t      offset_end,
	int                     ops,
	uint32_t                *range);


__private_extern__ void         vm_object_reap_pages(
	vm_object_t object,
	int     reap_type);
#define REAP_REAP       0
#define REAP_TERMINATE  1
#define REAP_PURGEABLE  2
#define REAP_DATA_FLUSH 3

#if CONFIG_FREEZE

__private_extern__ uint32_t
vm_object_compressed_freezer_pageout(
	vm_object_t     object, uint32_t dirty_budget);

__private_extern__ void
vm_object_compressed_freezer_done(
	void);

#endif                    

__private_extern__ void
vm_object_pageout(
	vm_object_t     object);

#if CONFIG_IOSCHED
struct io_reprioritize_req {
	uint64_t        blkno;
	uint32_t        len;
	int             priority;
	struct vnode    *devvp;
	queue_chain_t   io_reprioritize_list;
};
typedef struct io_reprioritize_req *io_reprioritize_req_t;

extern void vm_io_reprioritize_init(void);
#endif

  
                         
   

#define VM_OBJECT_EVENT_INITIALIZED             0
#define VM_OBJECT_EVENT_PAGER_READY             1
#define VM_OBJECT_EVENT_PAGING_IN_PROGRESS      2
#define VM_OBJECT_EVENT_MAPPING_IN_PROGRESS     3
#define VM_OBJECT_EVENT_LOCK_IN_PROGRESS        4
#define VM_OBJECT_EVENT_UNCACHING               5
#define VM_OBJECT_EVENT_COPY_CALL               6
#define VM_OBJECT_EVENT_CACHING                 7
#define VM_OBJECT_EVENT_UNBLOCKED               8
#define VM_OBJECT_EVENT_PAGING_ONLY_IN_PROGRESS 9

#define VM_OBJECT_EVENT_MAX 10                                        

static __inline__ wait_result_t
vm_object_assert_wait(
	vm_object_t             object,
	int                     event,
	wait_interrupt_t        interruptible)
{
	wait_result_t wr;

	vm_object_lock_assert_exclusive(object);
	assert(event >= 0 && event <= VM_OBJECT_EVENT_MAX);

	object->all_wanted |= 1 << event;
	wr = assert_wait((event_t)((vm_offset_t)object + (vm_offset_t)event),
	    interruptible);
	return wr;
}

static __inline__ wait_result_t
vm_object_wait(
	vm_object_t             object,
	int                     event,
	wait_interrupt_t        interruptible)
{
	wait_result_t wr;

	vm_object_assert_wait(object, event, interruptible);
	vm_object_unlock(object);
	wr = thread_block(THREAD_CONTINUE_NULL);
	return wr;
}

static __inline__ wait_result_t
thread_sleep_vm_object(
	vm_object_t             object,
	event_t                 event,
	wait_interrupt_t        interruptible)
{
	wait_result_t wr;

#if DEVELOPMENT || DEBUG
	if (object->Lock_owner != current_thread()) {
		panic("thread_sleep_vm_object: now owner - %p\n", object);
	}
	object->Lock_owner = 0;
#endif
	wr = lck_rw_sleep(&object->Lock,
	    LCK_SLEEP_PROMOTED_PRI,
	    event,
	    interruptible);
#if DEVELOPMENT || DEBUG
	object->Lock_owner = current_thread();
#endif
	return wr;
}

static __inline__ wait_result_t
vm_object_sleep(
	vm_object_t             object,
	int                     event,
	wait_interrupt_t        interruptible)
{
	wait_result_t wr;

	vm_object_lock_assert_exclusive(object);
	assert(event >= 0 && event <= VM_OBJECT_EVENT_MAX);

	object->all_wanted |= 1 << event;
	wr = thread_sleep_vm_object(object,
	    (event_t)((vm_offset_t)object + (vm_offset_t)event),
	    interruptible);
	return wr;
}

static __inline__ void
vm_object_wakeup(
	vm_object_t             object,
	int                     event)
{
	vm_object_lock_assert_exclusive(object);
	assert(event >= 0 && event <= VM_OBJECT_EVENT_MAX);

	if (object->all_wanted & (1 << event)) {
		thread_wakeup((event_t)((vm_offset_t)object + (vm_offset_t)event));
	}
	object->all_wanted &= ~(1 << event);
}

static __inline__ void
vm_object_set_wanted(
	vm_object_t             object,
	int                     event)
{
	vm_object_lock_assert_exclusive(object);
	assert(event >= 0 && event <= VM_OBJECT_EVENT_MAX);

	object->all_wanted |= (1 << event);
}

static __inline__ int
vm_object_wanted(
	vm_object_t             object,
	int                     event)
{
	vm_object_lock_assert_held(object);
	assert(event >= 0 && event <= VM_OBJECT_EVENT_MAX);

	return object->all_wanted & (1 << event);
}

  
                                 
   
#ifdef VM_PIP_DEBUG
#include <libkern/OSDebug.h>
#define VM_PIP_DEBUG_BEGIN(object)                                      \
	MACRO_BEGIN                                                     \
	int pip = ((object)->paging_in_progress +                       \
	           (object)->activity_in_progress);                     \
	if (pip < VM_PIP_DEBUG_MAX_REFS) {                              \
	        (void) OSBacktrace(&(object)->pip_holders[pip].pip_retaddr[0], \
	                           VM_PIP_DEBUG_STACK_FRAMES);          \
	}                                                               \
	MACRO_END
#else                     
#define VM_PIP_DEBUG_BEGIN(object)
#endif                    

#define         vm_object_activity_begin(object)                        \
	MACRO_BEGIN                                                     \
	vm_object_lock_assert_exclusive((object));                      \
	VM_PIP_DEBUG_BEGIN((object));                                   \
	(object)->activity_in_progress++;                               \
	if ((object)->activity_in_progress == 0) {                      \
	        panic("vm_object_activity_begin(%p): overflow\n", (object));\
	}                                                               \
	MACRO_END

#define         vm_object_activity_end(object)                          \
	MACRO_BEGIN                                                     \
	vm_object_lock_assert_exclusive((object));                      \
	if ((object)->activity_in_progress == 0) {                      \
	        panic("vm_object_activity_end(%p): underflow\n", (object));\
	}                                                               \
	(object)->activity_in_progress--;                               \
	if ((object)->paging_in_progress == 0 &&                        \
	    (object)->activity_in_progress == 0)                        \
	        vm_object_wakeup((object),                              \
	                         VM_OBJECT_EVENT_PAGING_IN_PROGRESS);   \
	MACRO_END

#define         vm_object_paging_begin(object)                          \
	MACRO_BEGIN                                                     \
	vm_object_lock_assert_exclusive((object));                      \
	VM_PIP_DEBUG_BEGIN((object));                                   \
	(object)->paging_in_progress++;                                 \
	if ((object)->paging_in_progress == 0) {                        \
	        panic("vm_object_paging_begin(%p): overflow\n", (object));\
	}                                                               \
	MACRO_END

#define         vm_object_paging_end(object)                            \
	MACRO_BEGIN                                                     \
	vm_object_lock_assert_exclusive((object));                      \
	if ((object)->paging_in_progress == 0) {                        \
	        panic("vm_object_paging_end(%p): underflow\n", (object));\
	}                                                               \
	(object)->paging_in_progress--;                                 \
	if ((object)->paging_in_progress == 0) {                        \
	        vm_object_wakeup((object),                              \
	                         VM_OBJECT_EVENT_PAGING_ONLY_IN_PROGRESS); \
	        if ((object)->activity_in_progress == 0)                \
	                vm_object_wakeup((object),                      \
	                                 VM_OBJECT_EVENT_PAGING_IN_PROGRESS); \
	}                                                               \
	MACRO_END

#define         vm_object_paging_wait(object, interruptible)            \
	MACRO_BEGIN                                                     \
	vm_object_lock_assert_exclusive((object));                      \
	while ((object)->paging_in_progress != 0 ||                     \
	       (object)->activity_in_progress != 0) {                   \
	        wait_result_t  _wr;                                     \
                                                                        \
	        _wr = vm_object_sleep((object),                         \
	                        VM_OBJECT_EVENT_PAGING_IN_PROGRESS,     \
	                        (interruptible));                       \
                                                                        \
	                                                         \
	                                                \
	}                                                               \
	MACRO_END

#define vm_object_paging_only_wait(object, interruptible)               \
	MACRO_BEGIN                                                     \
	vm_object_lock_assert_exclusive((object));                      \
	while ((object)->paging_in_progress != 0) {                     \
	        wait_result_t  _wr;                                     \
                                                                        \
	        _wr = vm_object_sleep((object),                         \
	                        VM_OBJECT_EVENT_PAGING_ONLY_IN_PROGRESS,\
	                        (interruptible));                       \
                                                                        \
	                                                         \
	                                                \
	}                                                               \
	MACRO_END


#define vm_object_mapping_begin(object)                                 \
	MACRO_BEGIN                                                     \
	vm_object_lock_assert_exclusive((object));                      \
	assert(! (object)->mapping_in_progress);                        \
	(object)->mapping_in_progress = TRUE;                           \
	MACRO_END

#define vm_object_mapping_end(object)                                   \
	MACRO_BEGIN                                                     \
	vm_object_lock_assert_exclusive((object));                      \
	assert((object)->mapping_in_progress);                          \
	(object)->mapping_in_progress = FALSE;                          \
	vm_object_wakeup((object),                                      \
	                 VM_OBJECT_EVENT_MAPPING_IN_PROGRESS);          \
	MACRO_END

#define vm_object_mapping_wait(object, interruptible)                   \
	MACRO_BEGIN                                                     \
	vm_object_lock_assert_exclusive((object));                      \
	while ((object)->mapping_in_progress) {                         \
	        wait_result_t	_wr;                                    \
                                                                        \
	        _wr = vm_object_sleep((object),                         \
	                              VM_OBJECT_EVENT_MAPPING_IN_PROGRESS, \
	                              (interruptible));                 \
	                                                         \
	                                                \
	}                                                               \
	assert(!(object)->mapping_in_progress);                         \
	MACRO_END



#define vm_object_round_page(x) (((vm_object_offset_t)(x) + PAGE_MASK) & ~((signed)PAGE_MASK))
#define vm_object_trunc_page(x) ((vm_object_offset_t)(x) & ~((signed)PAGE_MASK))

extern void     vm_object_cache_add(vm_object_t);
extern void     vm_object_cache_remove(vm_object_t);
extern int      vm_object_cache_evict(int, int);

#define VM_OBJECT_OWNER_DISOWNED ((task_t) -1)
#define VM_OBJECT_OWNER(object)                                         \
	((((object)->purgable == VM_PURGABLE_DENY &&                    \
	   (object)->vo_ledger_tag == 0) ||                             \
	  (object)->vo_owner == TASK_NULL)                              \
	 ? TASK_NULL                                                    \
	 : (((object)->vo_owner == VM_OBJECT_OWNER_DISOWNED)            \
	    ? kernel_task                                               \
	    : (object)->vo_owner))                                      \

extern void     vm_object_ledger_tag_ledgers(
	vm_object_t object,
	int *ledger_idx_volatile,
	int *ledger_idx_nonvolatile,
	int *ledger_idx_volatile_compressed,
	int *ledger_idx_nonvolatile_compressed,
	boolean_t *do_footprint);
extern kern_return_t vm_object_ownership_change(
	vm_object_t object,
	int new_ledger_tag,
	task_t new_owner,
	int new_ledger_flags,
	boolean_t task_objq_locked);

                                                                            
                                               
                                        
#define VM_OBJECT_ID(o) ((uint32_t)(uintptr_t)VM_KERNEL_ADDRPERM((o)))

#endif                        
