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
  
   
  
                       
                                                    
  
                                
   

#include <debug.h>
#include <mach_pagemap.h>

#include <mach/mach_types.h>
#include <mach/memory_object.h>
#include <mach/memory_object_default.h>
#include <mach/memory_object_control_server.h>
#include <mach/vm_param.h>

#include <mach/sdt.h>

#include <ipc/ipc_types.h>
#include <ipc/ipc_port.h>

#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/queue.h>
#include <kern/kalloc.h>
#include <kern/zalloc.h>
#include <kern/host.h>
#include <kern/host_statistics.h>
#include <kern/processor.h>
#include <kern/misc_protos.h>
#include <kern/policy_internal.h>

#include <vm/memory_object.h>
#include <vm/vm_compressor_pager.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>
#include <vm/vm_purgeable_internal.h>

#include <vm/vm_compressor.h>

#if CONFIG_PHANTOM_CACHE
#include <vm/vm_phantom_cache.h>
#endif

#if VM_OBJECT_ACCESS_TRACKING
uint64_t vm_object_access_tracking_reads = 0;
uint64_t vm_object_access_tracking_writes = 0;
#endif                                

boolean_t vm_object_collapse_compressor_allowed = TRUE;

struct vm_counters vm_counters;

#if DEVELOPMENT || DEBUG
extern struct memory_object_pager_ops shared_region_pager_ops;
extern unsigned int shared_region_pagers_resident_count;
extern unsigned int shared_region_pagers_resident_peak;
#endif                           

#if VM_OBJECT_TRACKING
boolean_t vm_object_tracking_inited = FALSE;
btlog_t *vm_object_tracking_btlog;

void
vm_object_tracking_init(void)
{
	int vm_object_tracking;

	vm_object_tracking = 1;
	PE_parse_boot_argn("vm_object_tracking", &vm_object_tracking,
	    sizeof(vm_object_tracking));

	if (vm_object_tracking) {
		vm_object_tracking_btlog = btlog_create(
			VM_OBJECT_TRACKING_NUM_RECORDS,
			VM_OBJECT_TRACKING_BTDEPTH,
			TRUE                                              );
		assert(vm_object_tracking_btlog);
		vm_object_tracking_inited = TRUE;
	}
}
#endif                         

  
                                                  
                                                     
                                                   
  
                                                      
                
  
                                                        
                                                      
                                                                 
        
  
                                                       
                                                  
                                                       
  
                                                  
                                                         
                                                       
  
                                                      
                                                            
                                                   
                                                       
                                                       
                                                      
                                                          
                                                        
          
  
                                                    
                                                         
                                                    
                                                      
                                                 
                                                    
                                                   
                                                           
  
                                                     
                                                      
                                                       
                                                       
                                                    
                                             
                                                       
                                                        
                                                         
  
                                                       
                                                      
                                                   
                                                     
                                                    
                                                          
                                                      
                                                              
                                                     
                                                        
                                                  
                                 
  
                                                   
                                                    
                                                       
                                                        
                                                         
             
  
                             
   

                                                  
static kern_return_t    vm_object_terminate(
	vm_object_t     object);

static kern_return_t    vm_object_copy_call(
	vm_object_t             src_object,
	vm_object_offset_t      src_offset,
	vm_object_size_t        size,
	vm_object_t             *_result_object);

static void             vm_object_do_collapse(
	vm_object_t     object,
	vm_object_t     backing_object);

static void             vm_object_do_bypass(
	vm_object_t     object,
	vm_object_t     backing_object);

static void             vm_object_release_pager(
	memory_object_t pager);

SECURITY_READ_ONLY_LATE(zone_t) vm_object_zone;                            

  
                                                           
                                                                  
   
static struct vm_object                 kernel_object_store VM_PAGE_PACKED_ALIGNED;
SECURITY_READ_ONLY_LATE(vm_object_t)    kernel_object = &kernel_object_store;

static struct vm_object                 compressor_object_store VM_PAGE_PACKED_ALIGNED;
SECURITY_READ_ONLY_LATE(vm_object_t)    compressor_object = &compressor_object_store;

  
                                                                             
                                                                           
                                                             
   
static struct vm_object                 retired_pages_object_store VM_PAGE_PACKED_ALIGNED;
SECURITY_READ_ONLY_LATE(vm_object_t)    retired_pages_object = &retired_pages_object_store;

  
                                                               
                                                             
                                                             
                                            
   
static struct vm_object                 vm_submap_object_store VM_PAGE_PACKED_ALIGNED;
SECURITY_READ_ONLY_LATE(vm_object_t)    vm_submap_object = &vm_submap_object_store;


  
                                              
                                       
  
                                                
                                                  
                               
   
static const struct vm_object vm_object_template = {
	.memq.prev = 0,
	.memq.next = 0,
	  
                                                             
                                                               
                           
    
#if DEVELOPMENT || DEBUG
	.Lock_owner = 0,
#endif
	.vo_size = 0,
	.memq_hint = VM_PAGE_NULL,
	.ref_count = 1,
	.resident_page_count = 0,
	.wired_page_count = 0,
	.reusable_page_count = 0,
	.copy = VM_OBJECT_NULL,
	.shadow = VM_OBJECT_NULL,
	.vo_shadow_offset = (vm_object_offset_t) 0,
	.pager = MEMORY_OBJECT_NULL,
	.paging_offset = 0,
	.pager_control = MEMORY_OBJECT_CONTROL_NULL,
	.copy_strategy = MEMORY_OBJECT_COPY_SYMMETRIC,
	.paging_in_progress = 0,
#if __LP64__
	.__object1_unused_bits = 0,
#endif               
	.activity_in_progress = 0,

	                     
	.all_wanted = 0,                     
	.pager_created = FALSE,
	.pager_initialized = FALSE,
	.pager_ready = FALSE,
	.pager_trusted = FALSE,
	.can_persist = FALSE,
	.internal = TRUE,
	.private = FALSE,
	.pageout = FALSE,
	.alive = TRUE,
	.purgable = VM_PURGABLE_DENY,
	.purgeable_when_ripe = FALSE,
	.purgeable_only_by_kernel = FALSE,
	.shadowed = FALSE,
	.true_share = FALSE,
	.terminating = FALSE,
	.named = FALSE,
	.shadow_severed = FALSE,
	.phys_contiguous = FALSE,
	.nophyscache = FALSE,
	                   

	.cached_list.prev = NULL,
	.cached_list.next = NULL,

	.last_alloc = (vm_object_offset_t) 0,
	.sequential = (vm_object_offset_t) 0,
	.pages_created = 0,
	.pages_used = 0,
	.scan_collisions = 0,
#if CONFIG_PHANTOM_CACHE
	.phantom_object_id = 0,
#endif
	.cow_hint = ~(vm_offset_t)0,

	                     
	.wimg_bits = VM_WIMG_USE_DEFAULT,
	.set_cache_attr = FALSE,
	.object_is_shared_cache = FALSE,
	.code_signed = FALSE,
	.transposed = FALSE,
	.mapping_in_progress = FALSE,
	.phantom_isssd = FALSE,
	.volatile_empty = FALSE,
	.volatile_fault = FALSE,
	.all_reusable = FALSE,
	.blocked_access = FALSE,
	.vo_ledger_tag = VM_LEDGER_TAG_NONE,
	.vo_no_footprint = FALSE,
#if CONFIG_IOSCHED || UPL_DEBUG
	.uplq.prev = NULL,
	.uplq.next = NULL,
#endif                
#ifdef VM_PIP_DEBUG
	.pip_holders = {0},
#endif                   

	.objq.next = NULL,
	.objq.prev = NULL,
	.task_objq.next = NULL,
	.task_objq.prev = NULL,

	.purgeable_queue_type = PURGEABLE_Q_TYPE_MAX,
	.purgeable_queue_group = 0,

	.wire_tag = VM_KERN_MEMORY_NONE,
#if !VM_TAG_ACTIVE_UPDATE
	.wired_objq.next = NULL,
	.wired_objq.prev = NULL,
#endif                             

	.io_tracking = FALSE,

#if CONFIG_SECLUDED_MEMORY
	.eligible_for_secluded = FALSE,
	.can_grab_secluded = FALSE,
#else                             
	.__object3_unused_bits = 0,
#endif                             

#if VM_OBJECT_ACCESS_TRACKING
	.access_tracking = FALSE,
	.access_tracking_reads = 0,
	.access_tracking_writes = 0,
#endif                                

#if DEBUG
	.purgeable_owner_bt = {0},
	.vo_purgeable_volatilizer = NULL,
	.purgeable_volatilizer_bt = {0},
#endif            
};

LCK_GRP_DECLARE(vm_object_lck_grp, "vm_object");
LCK_GRP_DECLARE(vm_object_cache_lck_grp, "vm_object_cache");
LCK_ATTR_DECLARE(vm_object_lck_attr, 0, 0);
LCK_ATTR_DECLARE(kernel_object_lck_attr, 0, LCK_ATTR_DEBUG);
LCK_ATTR_DECLARE(compressor_object_lck_attr, 0, LCK_ATTR_DEBUG);

unsigned int vm_page_purged_wired = 0;
unsigned int vm_page_purged_busy = 0;
unsigned int vm_page_purged_others = 0;

static queue_head_t     vm_object_cached_list;
static uint32_t         vm_object_cache_pages_freed = 0;
static uint32_t         vm_object_cache_pages_moved = 0;
static uint32_t         vm_object_cache_pages_skipped = 0;
static uint32_t         vm_object_cache_adds = 0;
static uint32_t         vm_object_cached_count = 0;
static LCK_MTX_EARLY_DECLARE_ATTR(vm_object_cached_lock_data,
    &vm_object_cache_lck_grp, &vm_object_lck_attr);

static uint32_t         vm_object_page_grab_failed = 0;
static uint32_t         vm_object_page_grab_skipped = 0;
static uint32_t         vm_object_page_grab_returned = 0;
static uint32_t         vm_object_page_grab_pmapped = 0;
static uint32_t         vm_object_page_grab_reactivations = 0;

#define vm_object_cache_lock_spin()             \
	        lck_mtx_lock_spin(&vm_object_cached_lock_data)
#define vm_object_cache_unlock()        \
	        lck_mtx_unlock(&vm_object_cached_lock_data)

static void     vm_object_cache_remove_locked(vm_object_t);


static void vm_object_reap(vm_object_t object);
static void vm_object_reap_async(vm_object_t object);
static void vm_object_reaper_thread(void);

static LCK_MTX_EARLY_DECLARE_ATTR(vm_object_reaper_lock_data,
    &vm_object_lck_grp, &vm_object_lck_attr);

static queue_head_t vm_object_reaper_queue;                                           
unsigned int vm_object_reap_count = 0;
unsigned int vm_object_reap_count_async = 0;

#define vm_object_reaper_lock()         \
	        lck_mtx_lock(&vm_object_reaper_lock_data)
#define vm_object_reaper_lock_spin()            \
	        lck_mtx_lock_spin(&vm_object_reaper_lock_data)
#define vm_object_reaper_unlock()       \
	        lck_mtx_unlock(&vm_object_reaper_lock_data)

#if CONFIG_IOSCHED
                                        
queue_head_t io_reprioritize_list = QUEUE_HEAD_INITIALIZER(io_reprioritize_list);

LCK_SPIN_DECLARE_ATTR(io_reprioritize_list_lock,
    &vm_object_lck_grp, &vm_object_lck_attr);

#define IO_REPRIORITIZE_LIST_LOCK()     \
	        lck_spin_lock_grp(&io_reprioritize_list_lock, &vm_object_lck_grp)
#define IO_REPRIORITIZE_LIST_UNLOCK()   \
	        lck_spin_unlock(&io_reprioritize_list_lock)

#define MAX_IO_REPRIORITIZE_REQS        8192
ZONE_DECLARE(io_reprioritize_req_zone, "io_reprioritize_req",
    sizeof(struct io_reprioritize_req), ZC_NOGC);

                                  
int io_reprioritize_wakeup = 0;
static void io_reprioritize_thread(void *param __unused, wait_result_t wr __unused);

#define IO_REPRIO_THREAD_WAKEUP()       thread_wakeup((event_t)&io_reprioritize_wakeup)
#define IO_REPRIO_THREAD_CONTINUATION()                                 \
{                                                               \
	assert_wait(&io_reprioritize_wakeup, THREAD_UNINT);     \
	thread_block(io_reprioritize_thread);                   \
}

void vm_page_request_reprioritize(vm_object_t, uint64_t, uint32_t, int);
void vm_page_handle_prio_inversion(vm_object_t, vm_page_t);
void vm_decmp_upl_reprioritize(upl_t, int);
#endif

#if 0
#undef KERNEL_DEBUG
#define KERNEL_DEBUG KERNEL_DEBUG_CONSTANT
#endif


  
                      
  
                                            
   

__private_extern__ void
_vm_object_allocate(
	vm_object_size_t        size,
	vm_object_t             object)
{
	*object = vm_object_template;
	vm_page_queue_init(&object->memq);
#if UPL_DEBUG || CONFIG_IOSCHED
	queue_init(&object->uplq);
#endif
	vm_object_lock_init(object);
	object->vo_size = vm_object_round_page(size);

#if VM_OBJECT_TRACKING_OP_CREATED
	if (vm_object_tracking_inited) {
		void    *bt[VM_OBJECT_TRACKING_BTDEPTH];
		int     numsaved = 0;

		numsaved = OSBacktrace(bt, VM_OBJECT_TRACKING_BTDEPTH);
		btlog_add_entry(vm_object_tracking_btlog,
		    object,
		    VM_OBJECT_TRACKING_OP_CREATED,
		    bt,
		    numsaved);
	}
#endif                                    
}

__private_extern__ vm_object_t
vm_object_allocate(
	vm_object_size_t        size)
{
	vm_object_t object;

	object = (vm_object_t) zalloc(vm_object_zone);

                                                   

	if (object != VM_OBJECT_NULL) {
		_vm_object_allocate(size, object);
	}

	return object;
}

TUNABLE(bool, workaround_41447923, "workaround_41447923", false);

  
                       
  
                                    
   
__startup_func
void
vm_object_bootstrap(void)
{
	vm_size_t       vm_object_size;

	assert(sizeof(mo_ipc_object_bits_t) == sizeof(ipc_object_bits_t));

	vm_object_size = (sizeof(struct vm_object) + (VM_PAGE_PACKED_PTR_ALIGNMENT - 1)) &
	    ~(VM_PAGE_PACKED_PTR_ALIGNMENT - 1);

	vm_object_zone = zone_create_ext("vm objects", vm_object_size,
	    ZC_NOENCRYPT | ZC_ALIGNMENT_REQUIRED,
	    ZONE_ID_ANY, ^(zone_t z){
#if defined(__LP64__)
		zone_set_submap_idx(z, Z_SUBMAP_IDX_VA_RESTRICTED);
#else
		(void)z;
#endif
	});

	queue_init(&vm_object_cached_list);

	queue_init(&vm_object_reaper_queue);

	  
                                  
    

	  
                                                                            
                                                                          
    
	_vm_object_allocate(VM_MAX_KERNEL_ADDRESS + 1, kernel_object);
	_vm_object_allocate(VM_MAX_KERNEL_ADDRESS + 1, compressor_object);
	kernel_object->copy_strategy = MEMORY_OBJECT_COPY_NONE;
	compressor_object->copy_strategy = MEMORY_OBJECT_COPY_NONE;
	kernel_object->no_tag_update = TRUE;

	  
                                        
    
	_vm_object_allocate(VM_MAX_KERNEL_ADDRESS + 1, retired_pages_object);
	retired_pages_object->copy_strategy = MEMORY_OBJECT_COPY_NONE;

	  
                                                            
                                                              
    

	_vm_object_allocate(VM_MAX_KERNEL_ADDRESS + 1, vm_submap_object);
	vm_submap_object->copy_strategy = MEMORY_OBJECT_COPY_NONE;

	  
                                                               
                                                              
                    
    
	vm_object_reference(vm_submap_object);
}

#if CONFIG_IOSCHED
void
vm_io_reprioritize_init(void)
{
	kern_return_t   result;
	thread_t        thread = THREAD_NULL;

	result = kernel_thread_start_priority(io_reprioritize_thread, NULL, 95                    , &thread);
	if (result == KERN_SUCCESS) {
		thread_set_thread_name(thread, "VM_io_reprioritize_thread");
		thread_deallocate(thread);
	} else {
		panic("Could not create io_reprioritize_thread");
	}
}
#endif

void
vm_object_reaper_init(void)
{
	kern_return_t   kr;
	thread_t        thread;

	kr = kernel_thread_start_priority(
		(thread_continue_t) vm_object_reaper_thread,
		NULL,
		BASEPRI_VM,
		&thread);
	if (kr != KERN_SUCCESS) {
		panic("failed to launch vm_object_reaper_thread kr=0x%x", kr);
	}
	thread_set_thread_name(thread, "VM_object_reaper_thread");
	thread_deallocate(thread);
}


  
                        
  
                                               
                                             
                                                      
                                                
                       
  
                           
   
unsigned long vm_object_deallocate_shared_successes = 0;
unsigned long vm_object_deallocate_shared_failures = 0;
unsigned long vm_object_deallocate_shared_swap_failures = 0;

__private_extern__ void
vm_object_deallocate(
	vm_object_t     object)
{
	vm_object_t     shadow = VM_OBJECT_NULL;

                                                                                          
                                                   

	if (object == VM_OBJECT_NULL) {
		return;
	}

	if (object == kernel_object || object == compressor_object || object == retired_pages_object) {
		vm_object_lock_shared(object);

		OSAddAtomic(-1, &object->ref_count);

		if (object->ref_count == 0) {
			if (object == kernel_object) {
				panic("vm_object_deallocate: losing kernel_object\n");
			} else if (object == retired_pages_object) {
				panic("vm_object_deallocate: losing retired_pages_object\n");
			} else {
				panic("vm_object_deallocate: losing compressor_object\n");
			}
		}
		vm_object_unlock(object);
		return;
	}

	if (object->ref_count == 2 &&
	    object->named) {
		  
                                                      
                      
                                                   
     
	} else if (object->ref_count == 2 &&
	    object->internal &&
	    object->shadow != VM_OBJECT_NULL) {
		  
                                                       
                                                 
                                                        
            
     
	} else if (object->ref_count >= 2) {
		UInt32          original_ref_count;
		volatile UInt32 *ref_count_p;
		Boolean         atomic_swap;

		  
                                                    
                                                               
                                                       
                                                           
                                                 
                                                             
                                   
     
		vm_object_lock_shared(object);
		ref_count_p = (volatile UInt32 *) &object->ref_count;
		original_ref_count = object->ref_count;
		  
                                                  
                              
     
		if (original_ref_count == 2 &&
		    object->named) {
			                                                 
			atomic_swap = FALSE;
		} else if (original_ref_count == 2 &&
		    object->internal &&
		    object->shadow != VM_OBJECT_NULL) {
			                                                     
			atomic_swap = FALSE;
		} else if (original_ref_count < 2) {
			                                                      
			atomic_swap = FALSE;
		} else {
			                                               
			atomic_swap = OSCompareAndSwap(
				original_ref_count,
				original_ref_count - 1,
				(UInt32 *) &object->ref_count);
			if (atomic_swap == FALSE) {
				vm_object_deallocate_shared_swap_failures++;
				                                   
			}
		}

		vm_object_unlock(object);

		if (atomic_swap) {
			  
                                        
      
			vm_object_deallocate_shared_successes++;
			return;
		}

		  
                                                   
                                                       
                          
     
		vm_object_deallocate_shared_failures++;
	}

	while (object != VM_OBJECT_NULL) {
		vm_object_lock(object);

		assert(object->ref_count > 0);

		  
                                                  
                                                  
                                                   
     
		if ((object->ref_count == 2) && (object->named)) {
			memory_object_t pager = object->pager;

			                                        
			                                  

			if (pager != MEMORY_OBJECT_NULL) {
				vm_object_mapping_wait(object, THREAD_UNINT);
				vm_object_mapping_begin(object);
				vm_object_unlock(object);

				memory_object_last_unmap(pager);

				vm_object_lock(object);
				vm_object_mapping_end(object);
			}
			assert(object->ref_count > 0);
		}

		  
                                            
                                             
                           
                                              
                                                   
     

		                                                     
		                                                     
		                       

		if ((object->ref_count > 1) || object->terminating) {
			vm_object_lock_assert_exclusive(object);
			object->ref_count--;

			if (object->ref_count == 1 &&
			    object->shadow != VM_OBJECT_NULL) {
				  
                                              
                                                
                                                  
                                                  
                            
                                                
                                                 
                                  
                                                   
                                                 
             
                                                   
                                                 
                         
       
				vm_object_collapse(object, 0, FALSE);
			}
			vm_object_unlock(object);
			return;
		}

		  
                                       
                                             
     

		if (object->pager_created && !object->pager_initialized) {
			assert(!object->can_persist);
			vm_object_assert_wait(object,
			    VM_OBJECT_EVENT_INITIALIZED,
			    THREAD_UNINT);
			vm_object_unlock(object);

			thread_block(THREAD_CONTINUE_NULL);
			continue;
		}

		  
                                               
                                              
                                                 
                                                  
                                                  
                        
     
		shadow = object->pageout?VM_OBJECT_NULL:object->shadow;

		if (vm_object_terminate(object) != KERN_SUCCESS) {
			return;
		}
		if (shadow != VM_OBJECT_NULL) {
			object = shadow;
			continue;
		}
		return;
	}
}



vm_page_t
vm_object_page_grab(
	vm_object_t     object)
{
	vm_page_t       p, next_p;
	int             p_limit = 0;
	int             p_skipped = 0;

	vm_object_lock_assert_exclusive(object);

	next_p = (vm_page_t)vm_page_queue_first(&object->memq);
	p_limit = MIN(50, object->resident_page_count);

	while (!vm_page_queue_end(&object->memq, (vm_page_queue_entry_t)next_p) && --p_limit > 0) {
		p = next_p;
		next_p = (vm_page_t)vm_page_queue_next(&next_p->vmp_listq);

		if (VM_PAGE_WIRED(p) || p->vmp_busy || p->vmp_cleaning || p->vmp_laundry || p->vmp_fictitious) {
			goto move_page_in_obj;
		}

		if (p->vmp_pmapped || p->vmp_dirty || p->vmp_precious) {
			vm_page_lockspin_queues();

			if (p->vmp_pmapped) {
				int refmod_state;

				vm_object_page_grab_pmapped++;

				if (p->vmp_reference == FALSE || p->vmp_dirty == FALSE) {
					refmod_state = pmap_get_refmod(VM_PAGE_GET_PHYS_PAGE(p));

					if (refmod_state & VM_MEM_REFERENCED) {
						p->vmp_reference = TRUE;
					}
					if (refmod_state & VM_MEM_MODIFIED) {
						SET_PAGE_DIRTY(p, FALSE);
					}
				}
				if (p->vmp_dirty == FALSE && p->vmp_precious == FALSE) {
					refmod_state = pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(p));

					if (refmod_state & VM_MEM_REFERENCED) {
						p->vmp_reference = TRUE;
					}
					if (refmod_state & VM_MEM_MODIFIED) {
						SET_PAGE_DIRTY(p, FALSE);
					}

					if (p->vmp_dirty == FALSE) {
						goto take_page;
					}
				}
			}
			if ((p->vmp_q_state != VM_PAGE_ON_ACTIVE_Q) && p->vmp_reference == TRUE) {
				vm_page_activate(p);

				counter_inc(&vm_statistics_reactivations);
				vm_object_page_grab_reactivations++;
			}
			vm_page_unlock_queues();
move_page_in_obj:
			vm_page_queue_remove(&object->memq, p, vmp_listq);
			vm_page_queue_enter(&object->memq, p, vmp_listq);

			p_skipped++;
			continue;
		}
		vm_page_lockspin_queues();
take_page:
		vm_page_free_prepare_queues(p);
		vm_object_page_grab_returned++;
		vm_object_page_grab_skipped += p_skipped;

		vm_page_unlock_queues();

		vm_page_free_prepare_object(p, TRUE);

		return p;
	}
	vm_object_page_grab_skipped += p_skipped;
	vm_object_page_grab_failed++;

	return NULL;
}



#define EVICT_PREPARE_LIMIT     64
#define EVICT_AGE               10

static  clock_sec_t     vm_object_cache_aging_ts = 0;

static void
vm_object_cache_remove_locked(
	vm_object_t     object)
{
	assert(object->purgable == VM_PURGABLE_DENY);

	queue_remove(&vm_object_cached_list, object, vm_object_t, cached_list);
	object->cached_list.next = NULL;
	object->cached_list.prev = NULL;

	vm_object_cached_count--;
}

void
vm_object_cache_remove(
	vm_object_t     object)
{
	vm_object_cache_lock_spin();

	if (object->cached_list.next &&
	    object->cached_list.prev) {
		vm_object_cache_remove_locked(object);
	}

	vm_object_cache_unlock();
}

void
vm_object_cache_add(
	vm_object_t     object)
{
	clock_sec_t sec;
	clock_nsec_t nsec;

	assert(object->purgable == VM_PURGABLE_DENY);

	if (object->resident_page_count == 0) {
		return;
	}
	clock_get_system_nanotime(&sec, &nsec);

	vm_object_cache_lock_spin();

	if (object->cached_list.next == NULL &&
	    object->cached_list.prev == NULL) {
		queue_enter(&vm_object_cached_list, object, vm_object_t, cached_list);
		object->vo_cache_ts = sec + EVICT_AGE;
		object->vo_cache_pages_to_scan = object->resident_page_count;

		vm_object_cached_count++;
		vm_object_cache_adds++;
	}
	vm_object_cache_unlock();
}

int
vm_object_cache_evict(
	int     num_to_evict,
	int     max_objects_to_examine)
{
	vm_object_t     object = VM_OBJECT_NULL;
	vm_object_t     next_obj = VM_OBJECT_NULL;
	vm_page_t       local_free_q = VM_PAGE_NULL;
	vm_page_t       p;
	vm_page_t       next_p;
	int             object_cnt = 0;
	vm_page_t       ep_array[EVICT_PREPARE_LIMIT];
	int             ep_count;
	int             ep_limit;
	int             ep_index;
	int             ep_freed = 0;
	int             ep_moved = 0;
	uint32_t        ep_skipped = 0;
	clock_sec_t     sec;
	clock_nsec_t    nsec;

	KERNEL_DEBUG(0x13001ec | DBG_FUNC_START, 0, 0, 0, 0, 0);
	  
                                              
                                
    
	if (queue_empty(&vm_object_cached_list)) {
		KERNEL_DEBUG(0x13001ec | DBG_FUNC_END, 0, 0, 0, 0, 0);
		return 0;
	}
	clock_get_system_nanotime(&sec, &nsec);

	  
                                               
                         
    
	if (sec < vm_object_cache_aging_ts) {
		KERNEL_DEBUG(0x13001ec | DBG_FUNC_END, 0, 0, 0, 0, 0);
		return 0;
	}
	  
                                     
                                         
    
	vm_page_unlock_queues();

	vm_object_cache_lock_spin();

	for (;;) {
		next_obj = (vm_object_t)queue_first(&vm_object_cached_list);

		while (!queue_end(&vm_object_cached_list, (queue_entry_t)next_obj) && object_cnt++ < max_objects_to_examine) {
			object = next_obj;
			next_obj = (vm_object_t)queue_next(&next_obj->cached_list);

			assert(object->purgable == VM_PURGABLE_DENY);

			if (sec < object->vo_cache_ts) {
				KERNEL_DEBUG(0x130020c, object, object->resident_page_count, object->vo_cache_ts, sec, 0);

				vm_object_cache_aging_ts = object->vo_cache_ts;
				object = VM_OBJECT_NULL;
				break;
			}
			if (!vm_object_lock_try_scan(object)) {
				  
                                                    
                                                              
                                            
       
				KERNEL_DEBUG(0x13001f8, object, object->resident_page_count, 0, 0, 0);

				object = VM_OBJECT_NULL;
				continue;
			}
			if (vm_page_queue_empty(&object->memq) || object->vo_cache_pages_to_scan == 0) {
				  
                                                            
                                                            
                                          
       
				KERNEL_DEBUG(0x13001fc, object, object->resident_page_count, ep_freed, ep_moved, 0);

				vm_object_cache_remove_locked(object);
				vm_object_unlock(object);
				object = VM_OBJECT_NULL;
				continue;
			}
			  
                                           
                              
      
			break;
		}
		vm_object_cache_unlock();

		if (object == VM_OBJECT_NULL) {
			break;
		}

		  
                                       
                       
     
		next_p = (vm_page_t)vm_page_queue_first(&object->memq);

		  
                                                                 
                                  
                                                                   
                                                                   
                                                                       
                                
     
		if ((ep_limit = num_to_evict - (ep_freed + ep_moved)) > EVICT_PREPARE_LIMIT) {
			ep_limit = EVICT_PREPARE_LIMIT;
		}
		ep_count = 0;

		while (!vm_page_queue_end(&object->memq, (vm_page_queue_entry_t)next_p) && object->vo_cache_pages_to_scan && ep_count < ep_limit) {
			p = next_p;
			next_p = (vm_page_t)vm_page_queue_next(&next_p->vmp_listq);

			object->vo_cache_pages_to_scan--;

			if (VM_PAGE_WIRED(p) || p->vmp_busy || p->vmp_cleaning || p->vmp_laundry) {
				vm_page_queue_remove(&object->memq, p, vmp_listq);
				vm_page_queue_enter(&object->memq, p, vmp_listq);

				ep_skipped++;
				continue;
			}
			if (p->vmp_wpmapped || p->vmp_dirty || p->vmp_precious) {
				vm_page_queue_remove(&object->memq, p, vmp_listq);
				vm_page_queue_enter(&object->memq, p, vmp_listq);

				pmap_clear_reference(VM_PAGE_GET_PHYS_PAGE(p));
			}
			ep_array[ep_count++] = p;
		}
		KERNEL_DEBUG(0x13001f4 | DBG_FUNC_START, object, object->resident_page_count, ep_freed, ep_moved, 0);

		vm_page_lockspin_queues();

		for (ep_index = 0; ep_index < ep_count; ep_index++) {
			p = ep_array[ep_index];

			if (p->vmp_wpmapped || p->vmp_dirty || p->vmp_precious) {
				p->vmp_reference = FALSE;
				p->vmp_no_cache = FALSE;

				  
                                                               
                                                                 
       
				vm_page_queues_remove(p, FALSE);
				vm_page_enqueue_inactive(p, TRUE);

				ep_moved++;
			} else {
#if CONFIG_PHANTOM_CACHE
				vm_phantom_cache_add_ghost(p);
#endif
				vm_page_free_prepare_queues(p);

				assert(p->vmp_pageq.next == 0 && p->vmp_pageq.prev == 0);
				  
                                                    
                         
       
				p->vmp_snext = local_free_q;
				local_free_q = p;

				ep_freed++;
			}
		}
		vm_page_unlock_queues();

		KERNEL_DEBUG(0x13001f4 | DBG_FUNC_END, object, object->resident_page_count, ep_freed, ep_moved, 0);

		if (local_free_q) {
			vm_page_free_list(local_free_q, TRUE);
			local_free_q = VM_PAGE_NULL;
		}
		if (object->vo_cache_pages_to_scan == 0) {
			KERNEL_DEBUG(0x1300208, object, object->resident_page_count, ep_freed, ep_moved, 0);

			vm_object_cache_remove(object);

			KERNEL_DEBUG(0x13001fc, object, object->resident_page_count, ep_freed, ep_moved, 0);
		}
		  
                          
     
		vm_object_unlock(object);
		object = VM_OBJECT_NULL;

		  
                                                
     
		if ((ep_freed + ep_moved) >= num_to_evict) {
			  
                                      
                              
      
			break;
		}
		vm_object_cache_lock_spin();
	}
	  
                                                 
              
    
	vm_page_lock_queues();

	vm_object_cache_pages_freed += ep_freed;
	vm_object_cache_pages_moved += ep_moved;
	vm_object_cache_pages_skipped += ep_skipped;

	KERNEL_DEBUG(0x13001ec | DBG_FUNC_END, ep_freed, 0, 0, 0, 0);
	return ep_freed;
}

  
                               
           
                                                   
                     
                                          
                                                   
  
                                              
  
                                                       
                                                    
                                                       
                                                              
                                                  
                               
   
static kern_return_t
vm_object_terminate(
	vm_object_t     object)
{
	vm_object_t     shadow_object;

	vm_object_lock_assert_exclusive(object);

	if (!object->pageout && (!object->internal && object->can_persist) &&
	    (object->pager != NULL || object->shadow_severed)) {
		  
                                                         
                                                         
                                                            
     
		object->pager_trusted = FALSE;

		vm_object_reap_pages(object, REAP_TERMINATE);
	}
	  
                                                       
    
	if (object->terminating) {
		vm_object_lock_assert_exclusive(object);
		object->ref_count--;
		assert(object->ref_count > 0);
		vm_object_unlock(object);
		return KERN_FAILURE;
	}

	  
                                                            
                
    
	if (object->ref_count != 1) {
		vm_object_lock_assert_exclusive(object);
		object->ref_count--;
		assert(object->ref_count > 0);
		vm_object_unlock(object);
		return KERN_FAILURE;
	}

	  
                                        
    

	object->terminating = TRUE;
	object->alive = FALSE;

	if (!object->internal &&
	    object->cached_list.next &&
	    object->cached_list.prev) {
		vm_object_cache_remove(object);
	}

	  
                                                            
                                                             
                  
    
	if (((shadow_object = object->shadow) != VM_OBJECT_NULL) &&
	    !(object->pageout)) {
		vm_object_lock(shadow_object);
		if (shadow_object->copy == object) {
			shadow_object->copy = VM_OBJECT_NULL;
		}
		vm_object_unlock(shadow_object);
	}

	if (object->paging_in_progress != 0 ||
	    object->activity_in_progress != 0) {
		  
                                                       
                                                       
                                                            
                                                            
                                                            
                                                                
                                                             
                                                           
                                                            
                                                      
                                                                
              
    
                                                                
                                                         
                                                 
    
                                                         
                                                
     
		vm_object_reap_async(object);
		vm_object_unlock(object);
		  
                                                       
                                                             
                                                 
                                                         
                                         
     
		return KERN_FAILURE;
	}
	  
                                      
    
	vm_object_reap(object);
	object = VM_OBJECT_NULL;

	  
                                                    
   
                                                           
                                                           
                    
                                                          
                                                          
                             
    
	return KERN_SUCCESS;
}


  
                    
  
                                                                 
                                                          
  
                                          
                                                                            
   

void
vm_object_reap(
	vm_object_t object)
{
	memory_object_t         pager;

	vm_object_lock_assert_exclusive(object);
	assert(object->paging_in_progress == 0);
	assert(object->activity_in_progress == 0);

	vm_object_reap_count++;

	  
                                                                 
                                                                
                                                             
    
	if (object->internal &&
	    (object->purgable != VM_PURGABLE_DENY ||
	    object->vo_ledger_tag)) {
		int ledger_flags;
		kern_return_t kr;

		ledger_flags = 0;
		if (object->vo_no_footprint) {
			ledger_flags |= VM_LEDGER_FLAG_NO_FOOTPRINT;
		}
		assert(!object->alive);
		assert(object->terminating);
		kr = vm_object_ownership_change(object,
		    object->vo_ledger_tag,                  
		    NULL,                                  
		    ledger_flags,
		    FALSE);                                            
		assert(kr == KERN_SUCCESS);
		assert(object->vo_owner == NULL);
	}

#if DEVELOPMENT || DEBUG
	if (object->object_is_shared_cache &&
	    object->pager != NULL &&
	    object->pager->mo_pager_ops == &shared_region_pager_ops) {
		OSAddAtomic(-object->resident_page_count, &shared_region_pagers_resident_count);
	}
#endif                           

	pager = object->pager;
	object->pager = MEMORY_OBJECT_NULL;

	if (pager != MEMORY_OBJECT_NULL) {
		memory_object_control_disable(&object->pager_control);
	}

	object->ref_count--;
	assert(object->ref_count == 0);

	  
                                          
    
	if (object->internal) {
		assert(VM_OBJECT_OWNER(object) == TASK_NULL);

		VM_OBJECT_UNWIRED(object);

		if (object->purgable == VM_PURGABLE_DENY) {
			                                  
		} else if (object->purgable == VM_PURGABLE_VOLATILE) {
			purgeable_q_t queue;

			queue = vm_purgeable_object_remove(object);
			assert(queue);

			if (object->purgeable_when_ripe) {
				  
                                     
                                      
       
				vm_page_lock_queues();
				vm_purgeable_token_delete_first(queue);

				assert(queue->debug_count_objects >= 0);
				vm_page_unlock_queues();
			}

			  
                                                       
                                                   
                                                         
                             
      
			unsigned int delta;
			assert(object->resident_page_count >=
			    object->wired_page_count);
			delta = (object->resident_page_count -
			    object->wired_page_count);
			if (delta != 0) {
				assert(vm_page_purgeable_count >= delta);
				OSAddAtomic(-delta,
				    (SInt32 *)&vm_page_purgeable_count);
			}
			if (object->wired_page_count != 0) {
				assert(vm_page_purgeable_wired_count >=
				    object->wired_page_count);
				OSAddAtomic(-object->wired_page_count,
				    (SInt32 *)&vm_page_purgeable_wired_count);
			}
			object->purgable = VM_PURGABLE_EMPTY;
		} else if (object->purgable == VM_PURGABLE_NONVOLATILE ||
		    object->purgable == VM_PURGABLE_EMPTY) {
			                                   
			vm_purgeable_nonvolatile_dequeue(object);
		} else {
			panic("object %p in unexpected purgeable state 0x%x\n",
			    object, object->purgable);
		}
		if (object->transposed &&
		    object->cached_list.next != NULL &&
		    object->cached_list.prev == NULL) {
			  
                                              
                                                  
      
		} else {
			assert(object->cached_list.next == NULL);
		}
		assert(object->cached_list.prev == NULL);
	}

	if (object->pageout) {
		  
                                       
                
                         
     
		assert(object->shadow != VM_OBJECT_NULL);

		vm_pageout_object_terminate(object);
	} else if (object->resident_page_count) {
		  
                                       
                
     
		vm_object_reap_pages(object, REAP_REAP);
	}
	assert(vm_page_queue_empty(&object->memq));
	assert(object->paging_in_progress == 0);
	assert(object->activity_in_progress == 0);
	assert(object->ref_count == 0);

	  
                                                 
                                                  
                                     
    
	if (pager != MEMORY_OBJECT_NULL) {
		vm_object_unlock(object);
		vm_object_release_pager(pager);
		vm_object_lock(object);
	}

	                                            
	object->terminating = FALSE;
	vm_object_paging_begin(object);
	vm_object_paging_end(object);
	vm_object_unlock(object);

	object->shadow = VM_OBJECT_NULL;

#if VM_OBJECT_TRACKING
	if (vm_object_tracking_inited) {
		btlog_remove_entries_for_element(vm_object_tracking_btlog,
		    object);
	}
#endif                         

	vm_object_lock_destroy(object);
	  
                                  
    
	zfree(vm_object_zone, object);
	object = VM_OBJECT_NULL;
}


unsigned int vm_max_batch = 256;

#define V_O_R_MAX_BATCH 128

#define BATCH_LIMIT(max)        (vm_max_batch >= max ? max : vm_max_batch)


#define VM_OBJ_REAP_FREELIST(_local_free_q, do_disconnect)              \
	MACRO_BEGIN                                                     \
	if (_local_free_q) {                                            \
	        if (do_disconnect) {                                    \
	                vm_page_t m;                                    \
	                for (m = _local_free_q;                         \
	                     m != VM_PAGE_NULL;                         \
	                     m = m->vmp_snext) {                        \
	                        if (m->vmp_pmapped) {                   \
	                                pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m)); \
	                        }                                       \
	                }                                               \
	        }                                                       \
	        vm_page_free_list(_local_free_q, TRUE);                 \
	        _local_free_q = VM_PAGE_NULL;                           \
	}                                                               \
	MACRO_END


void
vm_object_reap_pages(
	vm_object_t     object,
	int             reap_type)
{
	vm_page_t       p;
	vm_page_t       next;
	vm_page_t       local_free_q = VM_PAGE_NULL;
	int             loop_count;
	boolean_t       disconnect_on_release;
	pmap_flush_context      pmap_flush_context_storage;

	if (reap_type == REAP_DATA_FLUSH) {
		  
                                                      
                                    
     
		disconnect_on_release = TRUE;
	} else {
		  
                                                         
                                                         
                                                    
                                                            
                      
     
		disconnect_on_release = FALSE;
	}

restart_after_sleep:
	if (vm_page_queue_empty(&object->memq)) {
		return;
	}
	loop_count = BATCH_LIMIT(V_O_R_MAX_BATCH);

	if (reap_type == REAP_PURGEABLE) {
		pmap_flush_context_init(&pmap_flush_context_storage);
	}

	vm_page_lock_queues();

	next = (vm_page_t)vm_page_queue_first(&object->memq);

	while (!vm_page_queue_end(&object->memq, (vm_page_queue_entry_t)next)) {
		p = next;
		next = (vm_page_t)vm_page_queue_next(&next->vmp_listq);

		if (--loop_count == 0) {
			vm_page_unlock_queues();

			if (local_free_q) {
				if (reap_type == REAP_PURGEABLE) {
					pmap_flush(&pmap_flush_context_storage);
					pmap_flush_context_init(&pmap_flush_context_storage);
				}
				  
                                         
                                       
                                           
       
				VM_OBJ_REAP_FREELIST(local_free_q,
				    disconnect_on_release);
			} else {
				mutex_pause(0);
			}

			loop_count = BATCH_LIMIT(V_O_R_MAX_BATCH);

			vm_page_lock_queues();
		}
		if (reap_type == REAP_DATA_FLUSH || reap_type == REAP_TERMINATE) {
			if (p->vmp_busy || p->vmp_cleaning) {
				vm_page_unlock_queues();
				  
                                      
       
				VM_OBJ_REAP_FREELIST(local_free_q,
				    disconnect_on_release);

				PAGE_SLEEP(object, p, THREAD_UNINT);

				goto restart_after_sleep;
			}
			if (p->vmp_laundry) {
				vm_pageout_steal_laundry(p, TRUE);
			}
		}
		switch (reap_type) {
		case REAP_DATA_FLUSH:
			if (VM_PAGE_WIRED(p)) {
				  
                                               
                                                   
                                                  
                                                    
       
				continue;
			}
			break;

		case REAP_PURGEABLE:
			if (VM_PAGE_WIRED(p)) {
				  
                               
       
				vm_page_purged_wired++;
				continue;
			}
			if (p->vmp_laundry && !p->vmp_busy && !p->vmp_cleaning) {
				vm_pageout_steal_laundry(p, TRUE);
			}

			if (p->vmp_cleaning || p->vmp_laundry || p->vmp_absent) {
				  
                                
                            
       
				vm_page_purged_others++;
				continue;
			}
			if (p->vmp_busy) {
				  
                                              
                                                               
                                      
                               
       
				if (VM_PAGE_PAGEABLE(p)) {
					vm_page_deactivate(p);
				}
				vm_page_purged_busy++;
				continue;
			}

			assert(VM_PAGE_OBJECT(p) != kernel_object);

			  
                                 
      
			if (p->vmp_pmapped == TRUE) {
				  
                     
       
				pmap_disconnect_options(VM_PAGE_GET_PHYS_PAGE(p), PMAP_OPTIONS_NOFLUSH | PMAP_OPTIONS_NOREFMOD, (void *)&pmap_flush_context_storage);
			}
			vm_page_purged_count++;

			break;

		case REAP_TERMINATE:
			if (p->vmp_absent || p->vmp_private) {
				  
                                           
                                           
                                         
                                          
                            
       
				break;
			}
			if (p->vmp_fictitious) {
				assert(VM_PAGE_GET_PHYS_PAGE(p) == vm_page_guard_addr);
				break;
			}
			if (!p->vmp_dirty && p->vmp_wpmapped) {
				p->vmp_dirty = pmap_is_modified(VM_PAGE_GET_PHYS_PAGE(p));
			}

			if ((p->vmp_dirty || p->vmp_precious) && !p->vmp_error && object->alive) {
				assert(!object->internal);

				p->vmp_free_when_done = TRUE;

				if (!p->vmp_laundry) {
					vm_page_queues_remove(p, TRUE);
					  
                                        
                              
        
					vm_pageout_cluster(p);
				}
				vm_page_unlock_queues();
				  
                                      
       
				VM_OBJ_REAP_FREELIST(local_free_q,
				    disconnect_on_release);

				vm_object_paging_wait(object, THREAD_UNINT);

				goto restart_after_sleep;
			}
			break;

		case REAP_REAP:
			break;
		}
		vm_page_free_prepare_queues(p);
		assert(p->vmp_pageq.next == 0 && p->vmp_pageq.prev == 0);
		  
                                                  
                       
     
		p->vmp_snext = local_free_q;
		local_free_q = p;
	}
	vm_page_unlock_queues();

	  
                                      
    
	if (reap_type == REAP_PURGEABLE) {
		pmap_flush(&pmap_flush_context_storage);
	}

	VM_OBJ_REAP_FREELIST(local_free_q,
	    disconnect_on_release);
}


void
vm_object_reap_async(
	vm_object_t     object)
{
	vm_object_lock_assert_exclusive(object);

	vm_object_reaper_lock_spin();

	vm_object_reap_count_async++;

	                              
	queue_enter(&vm_object_reaper_queue, object,
	    vm_object_t, cached_list);

	vm_object_reaper_unlock();

	                                       
	thread_wakeup((event_t) &vm_object_reaper_queue);
}


void
vm_object_reaper_thread(void)
{
	vm_object_t     object, shadow_object;

	vm_object_reaper_lock_spin();

	while (!queue_empty(&vm_object_reaper_queue)) {
		queue_remove_first(&vm_object_reaper_queue,
		    object,
		    vm_object_t,
		    cached_list);

		vm_object_reaper_unlock();
		vm_object_lock(object);

		assert(object->terminating);
		assert(!object->alive);

		  
                                                        
                                                         
                                                             
                                                           
                                                          
            
     
		while (object->paging_in_progress != 0 ||
		    object->activity_in_progress != 0) {
			vm_object_wait(object,
			    VM_OBJECT_EVENT_PAGING_IN_PROGRESS,
			    THREAD_UNINT);
			vm_object_lock(object);
		}

		shadow_object =
		    object->pageout ? VM_OBJECT_NULL : object->shadow;

		vm_object_reap(object);
		                                                     
		object = VM_OBJECT_NULL;

		if (shadow_object != VM_OBJECT_NULL) {
			  
                                                
                        
      
			vm_object_deallocate(shadow_object);
			shadow_object = VM_OBJECT_NULL;
		}
		vm_object_reaper_lock_spin();
	}

	                           
	assert_wait((event_t) &vm_object_reaper_queue, THREAD_UNINT);

	vm_object_reaper_unlock();

	thread_block((thread_continue_t) vm_object_reaper_thread);
	              
}

  
                                   
                                                     
                                      
   
static void
vm_object_release_pager(
	memory_object_t pager)
{
	  
                        
    

	(void) memory_object_terminate(pager);

	  
                               
    
	memory_object_deallocate(pager);
}

  
                             
           
                                      
                                                 
                     
   
kern_return_t
vm_object_destroy(
	vm_object_t             object,
	__unused kern_return_t          reason)
{
	memory_object_t         old_pager;

	if (object == VM_OBJECT_NULL) {
		return KERN_SUCCESS;
	}

	  
                                             
   
                                                     
                                                  
                                                           
                      
    

	vm_object_lock(object);
	object->can_persist = FALSE;
	object->named = FALSE;
	object->alive = FALSE;

#if DEVELOPMENT || DEBUG
	if (object->object_is_shared_cache &&
	    object->pager != NULL &&
	    object->pager->mo_pager_ops == &shared_region_pager_ops) {
		OSAddAtomic(-object->resident_page_count, &shared_region_pagers_resident_count);
	}
#endif                           

	old_pager = object->pager;
	object->pager = MEMORY_OBJECT_NULL;
	if (old_pager != MEMORY_OBJECT_NULL) {
		memory_object_control_disable(&object->pager_control);
	}

	  
                                                   
                                                       
    

	vm_object_paging_wait(object, THREAD_UNINT);
	vm_object_unlock(object);

	  
                             
    
	if (old_pager != MEMORY_OBJECT_NULL) {
		vm_object_release_pager(old_pager);

		  
                                                            
                                                           
                                                           
                                                             
                 
     
		vm_object_deallocate(object);
	}
	return KERN_SUCCESS;
}

  
                                                                                             
                                                                                       
                                                                                       
                                                                                       
                                                                                        
                                                                                       
                     
  
                                                                                      
                                                                                      
                                                                                    
           
  
                                                                                         
                                                                                        
                                                                                        
                                                                                       
                                                                                       
   

#define PAGES_IN_A_CHUNK        64                                                 
                                                                                   
                                                                                   
                                                                     

#define CHUNK_SIZE      (PAGES_IN_A_CHUNK * PAGE_SIZE_64)                                     

typedef uint64_t        chunk_state_t;

  
                                                                                    
                                                                                         
                                                                                           
                                                                                        
                
   

#define CHUNK_INIT(c, len)                                              \
	MACRO_BEGIN                                                     \
	uint64_t p;                                                     \
                                                                        \
	(c) = 0xffffffffffffffffLL;                                     \
                                                                        \
	for (p = (len) / PAGE_SIZE_64; p < PAGES_IN_A_CHUNK; p++)       \
	        MARK_PAGE_HANDLED(c, p);                                \
	MACRO_END


  
                                                                     
   

#define CHUNK_NOT_COMPLETE(c)   ((c) != 0)

  
                                                                                
                                                              
   

#define PAGE_ALREADY_HANDLED(c, p)      (((c) & (1ULL << (p))) == 0)

  
                                                                       
   

#define MARK_PAGE_HANDLED(c, p) \
MACRO_BEGIN \
	(c) = (c) & ~(1ULL << (p)); \
MACRO_END


  
                                                                             
                                         
   

static boolean_t
page_is_paged_out(
	vm_object_t             object,
	vm_object_offset_t      offset)
{
	if (object->internal &&
	    object->alive &&
	    !object->terminating &&
	    object->pager_ready) {
		if (VM_COMPRESSOR_PAGER_STATE_GET(object, offset)
		    == VM_EXTERNAL_STATE_EXISTS) {
			return TRUE;
		}
	}
	return FALSE;
}



  
                     
  
                                                               
                                                                       
                                                                   
                                 
   
#if DEVELOPMENT || DEBUG
int madvise_free_debug = 1;
#else            
int madvise_free_debug = 0;
#endif            

__options_decl(deactivate_flags_t, uint32_t, {
	DEACTIVATE_KILL         = 0x1,
	DEACTIVATE_REUSABLE     = 0x2,
	DEACTIVATE_ALL_REUSABLE = 0x4,
	DEACTIVATE_CLEAR_REFMOD = 0x8
});

  
                                                                                                 
                                                                                                      
                                                       
   

static void
deactivate_pages_in_object(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	deactivate_flags_t      flags,
	chunk_state_t           *chunk_state,
	pmap_flush_context      *pfc,
	struct pmap             *pmap,
	vm_map_offset_t         pmap_offset)
{
	vm_page_t       m;
	int             p;
	struct  vm_page_delayed_work    dw_array;
	struct  vm_page_delayed_work    *dwp, *dwp_start;
	bool            dwp_finish_ctx = TRUE;
	int             dw_count;
	int             dw_limit;
	unsigned int    reusable = 0;

	  
                                                                                                     
                                                                                                     
                                                                                                        
                               
    

	dwp_start = dwp = NULL;
	dw_count = 0;
	dw_limit = DELAYED_WORK_LIMIT(DEFAULT_DELAYED_WORK_LIMIT);
	dwp_start = vm_page_delayed_work_get_ctx();
	if (dwp_start == NULL) {
		dwp_start = &dw_array;
		dw_limit = 1;
		dwp_finish_ctx = FALSE;
	}

	dwp = dwp_start;

	for (p = 0; size && CHUNK_NOT_COMPLETE(*chunk_state); p++, size -= PAGE_SIZE_64, offset += PAGE_SIZE_64, pmap_offset += PAGE_SIZE_64) {
		  
                                                                                           
                                                      
     

		if (PAGE_ALREADY_HANDLED(*chunk_state, p)) {
			continue;
		}

		  
                                                                                           
                                                            
     

		if ((m = vm_page_lookup(object, offset)) != VM_PAGE_NULL) {
			  
                                                                                       
                                                                                       
                                                
      

			MARK_PAGE_HANDLED(*chunk_state, p);

			if ((!VM_PAGE_WIRED(m)) && (!m->vmp_private) && (!m->vmp_gobbled) && (!m->vmp_busy) &&
			    (!m->vmp_laundry) && (!m->vmp_cleaning) && !(m->vmp_free_when_done)) {
				int     clear_refmod_mask;
				int     pmap_options;
				dwp->dw_mask = 0;

				pmap_options = 0;
				clear_refmod_mask = VM_MEM_REFERENCED;
				dwp->dw_mask |= DW_clear_reference;

				if ((flags & DEACTIVATE_KILL) && (object->internal)) {
					if (madvise_free_debug) {
						  
                               
                             
                                  
         
						pmap_zero_page(VM_PAGE_GET_PHYS_PAGE(m));
					}
					m->vmp_precious = FALSE;
					m->vmp_dirty = FALSE;

					clear_refmod_mask |= VM_MEM_MODIFIED;
					if (m->vmp_q_state == VM_PAGE_ON_THROTTLED_Q) {
						  
                                   
                                  
                                   
                                   
                 
         
						dwp->dw_mask |= DW_move_page;
					}

					VM_COMPRESSOR_PAGER_STATE_CLR(object, offset);

					if ((flags & DEACTIVATE_REUSABLE) && !m->vmp_reusable) {
						assert(!(flags & DEACTIVATE_ALL_REUSABLE));
						assert(!object->all_reusable);
						m->vmp_reusable = TRUE;
						object->reusable_page_count++;
						assert(object->resident_page_count >= object->reusable_page_count);
						reusable++;
						  
                                   
                                   
                                 
         
						pmap_options |= PMAP_OPTIONS_SET_REUSABLE;
					}
				}
				if (flags & DEACTIVATE_CLEAR_REFMOD) {
					  
                                                           
                                     
        
					pmap_options |= PMAP_OPTIONS_NOFLUSH;
					pmap_clear_refmod_options(VM_PAGE_GET_PHYS_PAGE(m),
					    clear_refmod_mask,
					    pmap_options,
					    (void *)pfc);
				}

				if ((m->vmp_q_state != VM_PAGE_ON_THROTTLED_Q) &&
				    !(flags & (DEACTIVATE_REUSABLE | DEACTIVATE_ALL_REUSABLE))) {
					dwp->dw_mask |= DW_move_page;
				}

				if (dwp->dw_mask) {
					VM_PAGE_ADD_DELAYED_WORK(dwp, m,
					    dw_count);
				}

				if (dw_count >= dw_limit) {
					if (reusable) {
						OSAddAtomic(reusable,
						    &vm_page_stats_reusable.reusable_count);
						vm_page_stats_reusable.reusable += reusable;
						reusable = 0;
					}
					vm_page_do_delayed_work(object, VM_KERN_MEMORY_NONE, dwp_start, dw_count);

					dwp = dwp_start;
					dw_count = 0;
				}
			}
		} else {
			  
                                                                         
                                                                           
                                 
      

			if (page_is_paged_out(object, offset)) {
				MARK_PAGE_HANDLED(*chunk_state, p);

				  
                                                                                 
                                                                                    
       

				if ((flags & DEACTIVATE_KILL) && (object->internal)) {
					VM_COMPRESSOR_PAGER_STATE_CLR(object, offset);

					if (pmap != PMAP_NULL) {
						  
                                 
                                
                                    
                                
                           
         
						pmap_remove_options(
							pmap,
							pmap_offset,
							(pmap_offset +
							PAGE_SIZE),
							PMAP_OPTIONS_REMOVE);
					}
				}
			}
		}
	}

	if (reusable) {
		OSAddAtomic(reusable, &vm_page_stats_reusable.reusable_count);
		vm_page_stats_reusable.reusable += reusable;
		reusable = 0;
	}

	if (dw_count) {
		vm_page_do_delayed_work(object, VM_KERN_MEMORY_NONE, dwp_start, dw_count);
		dwp = dwp_start;
		dw_count = 0;
	}

	if (dwp_start && dwp_finish_ctx) {
		vm_page_delayed_work_finish_ctx(dwp_start);
		dwp_start = dwp = NULL;
	}
}


  
                                                                                     
                                                                                      
                                                                                          
                                                                                         
                                                                                         
                            
                                                                                                      
   

static vm_object_size_t
deactivate_a_chunk(
	vm_object_t             orig_object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	deactivate_flags_t      flags,
	pmap_flush_context      *pfc,
	struct pmap             *pmap,
	vm_map_offset_t         pmap_offset)
{
	vm_object_t             object;
	vm_object_t             tmp_object;
	vm_object_size_t        length;
	chunk_state_t           chunk_state;


	  
                                                                           
                                        
    

	length = MIN(size, CHUNK_SIZE);

	  
                                                                                 
                                                                                     
                                                                                   
    

	CHUNK_INIT(chunk_state, length);
	object = orig_object;

	  
                                                                                  
                                                                                       
                                                                       
    

	while (object && CHUNK_NOT_COMPLETE(chunk_state)) {
		vm_object_paging_begin(object);

		deactivate_pages_in_object(object, offset, length, flags, &chunk_state, pfc, pmap, pmap_offset);

		vm_object_paging_end(object);

		  
                                                                         
                                                                           
                                                                             
     

		tmp_object = object->shadow;

		if (tmp_object) {
			assert(!(flags & DEACTIVATE_KILL) || (flags & DEACTIVATE_CLEAR_REFMOD));
			flags &= ~(DEACTIVATE_KILL | DEACTIVATE_REUSABLE | DEACTIVATE_ALL_REUSABLE);
			offset += object->vo_shadow_offset;
			vm_object_lock(tmp_object);
		}

		if (object != orig_object) {
			vm_object_unlock(object);
		}

		object = tmp_object;
	}

	if (object && object != orig_object) {
		vm_object_unlock(object);
	}

	return length;
}



  
                                                                                              
                                                                                             
               
   

__private_extern__ void
vm_object_deactivate_pages(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	boolean_t               kill_page,
	boolean_t               reusable_page,
	struct pmap             *pmap,
	vm_map_offset_t         pmap_offset)
{
	vm_object_size_t        length;
	boolean_t               all_reusable;
	pmap_flush_context      pmap_flush_context_storage;
	unsigned int pmap_clear_refmod_mask = VM_MEM_REFERENCED;
	unsigned int pmap_clear_refmod_options = 0;
	deactivate_flags_t flags = DEACTIVATE_CLEAR_REFMOD;
	bool refmod_cleared = false;
	if (kill_page) {
		flags |= DEACTIVATE_KILL;
	}
	if (reusable_page) {
		flags |= DEACTIVATE_REUSABLE;
	}

	  
                                                                              
                                                                              
                                                                                 
                                                                   
    


	all_reusable = FALSE;
#if 11
	  
                                                           
                                                              
                                                   
   
                                                            
                                                                  
                                                                      
         
    
	if (size == 0) {
		return;
	}
#else
	if (reusable_page &&
	    object->internal &&
	    object->vo_size != 0 &&
	    object->vo_size == size &&
	    object->reusable_page_count == 0) {
		all_reusable = TRUE;
		reusable_page = FALSE;
		flags |= DEACTIVATE_ALL_REUSABLE;
	}
#endif

	if ((reusable_page || all_reusable) && object->all_reusable) {
		                                                             
                            
		return;
	}


	pmap_flush_context_init(&pmap_flush_context_storage);

	  
                                                                                 
                                                                         
                                                                                      
                  
                                                         
    
	if (size > PAGE_SIZE && (!kill_page || !object->shadow)) {
		if (kill_page && object->internal) {
			pmap_clear_refmod_mask |= VM_MEM_MODIFIED;
		}
		if (reusable_page) {
			pmap_clear_refmod_options |= PMAP_OPTIONS_SET_REUSABLE;
		}

		refmod_cleared = pmap_clear_refmod_range_options(pmap, pmap_offset, pmap_offset + size, pmap_clear_refmod_mask, pmap_clear_refmod_options);
		if (refmod_cleared) {
			                                                                                          
			flags &= ~DEACTIVATE_CLEAR_REFMOD;
		}
	}

	while (size) {
		length = deactivate_a_chunk(object, offset, size, flags,
		    &pmap_flush_context_storage, pmap, pmap_offset);

		size -= length;
		offset += length;
		pmap_offset += length;
	}
	pmap_flush(&pmap_flush_context_storage);

	if (all_reusable) {
		if (!object->all_reusable) {
			unsigned int reusable;

			object->all_reusable = TRUE;
			assert(object->reusable_page_count == 0);
			                         
			reusable = object->resident_page_count;
			OSAddAtomic(reusable,
			    &vm_page_stats_reusable.reusable_count);
			vm_page_stats_reusable.reusable += reusable;
			vm_page_stats_reusable.all_reusable_calls++;
		}
	} else if (reusable_page) {
		vm_page_stats_reusable.partial_reusable_calls++;
	}
}

void
vm_object_reuse_pages(
	vm_object_t             object,
	vm_object_offset_t      start_offset,
	vm_object_offset_t      end_offset,
	boolean_t               allow_partial_reuse)
{
	vm_object_offset_t      cur_offset;
	vm_page_t               m;
	unsigned int            reused, reusable;

#define VM_OBJECT_REUSE_PAGE(object, m, reused)                         \
	MACRO_BEGIN                                                     \
	        if ((m) != VM_PAGE_NULL &&                              \
	            (m)->vmp_reusable) {                                \
	                assert((object)->reusable_page_count <=         \
	                       (object)->resident_page_count);          \
	                assert((object)->reusable_page_count > 0);      \
	                (object)->reusable_page_count--;                \
	                (m)->vmp_reusable = FALSE;                      \
	                (reused)++;                                     \
	    
                                           
                                                
                                             
           
                                                                 \
	                pmap_clear_refmod_options(VM_PAGE_GET_PHYS_PAGE((m)), \
	                                          0,                    \
	                                          (PMAP_OPTIONS_CLEAR_REUSABLE \
	                                           | PMAP_OPTIONS_NOFLUSH), \
	                                          NULL);                \
	        }                                                       \
	MACRO_END

	reused = 0;
	reusable = 0;

	vm_object_lock_assert_exclusive(object);

	if (object->all_reusable) {
		panic("object %p all_reusable: can't update pmap stats\n",
		    object);
		assert(object->reusable_page_count == 0);
		object->all_reusable = FALSE;
		if (end_offset - start_offset == object->vo_size ||
		    !allow_partial_reuse) {
			vm_page_stats_reusable.all_reuse_calls++;
			reused = object->resident_page_count;
		} else {
			vm_page_stats_reusable.partial_reuse_calls++;
			vm_page_queue_iterate(&object->memq, m, vmp_listq) {
				if (m->vmp_offset < start_offset ||
				    m->vmp_offset >= end_offset) {
					m->vmp_reusable = TRUE;
					object->reusable_page_count++;
					assert(object->resident_page_count >= object->reusable_page_count);
					continue;
				} else {
					assert(!m->vmp_reusable);
					reused++;
				}
			}
		}
	} else if (object->resident_page_count >
	    ((end_offset - start_offset) >> PAGE_SHIFT)) {
		vm_page_stats_reusable.partial_reuse_calls++;
		for (cur_offset = start_offset;
		    cur_offset < end_offset;
		    cur_offset += PAGE_SIZE_64) {
			if (object->reusable_page_count == 0) {
				break;
			}
			m = vm_page_lookup(object, cur_offset);
			VM_OBJECT_REUSE_PAGE(object, m, reused);
		}
	} else {
		vm_page_stats_reusable.partial_reuse_calls++;
		vm_page_queue_iterate(&object->memq, m, vmp_listq) {
			if (object->reusable_page_count == 0) {
				break;
			}
			if (m->vmp_offset < start_offset ||
			    m->vmp_offset >= end_offset) {
				continue;
			}
			VM_OBJECT_REUSE_PAGE(object, m, reused);
		}
	}

	                         
	OSAddAtomic(reusable - reused, &vm_page_stats_reusable.reusable_count);
	vm_page_stats_reusable.reused += reused;
	vm_page_stats_reusable.reusable += reusable;
}

  
                                  
  
           
                                           
                                        
  
                                            
                                           
                                              
                          
  
                                              
                                             
                                                   
  
                                                     
                
  
                                                              
                                                           
                     
   

__private_extern__ void
vm_object_pmap_protect(
	vm_object_t                     object,
	vm_object_offset_t              offset,
	vm_object_size_t                size,
	pmap_t                          pmap,
	vm_map_size_t                   pmap_page_size,
	vm_map_offset_t                 pmap_start,
	vm_prot_t                       prot)
{
	vm_object_pmap_protect_options(object, offset, size, pmap,
	    pmap_page_size,
	    pmap_start, prot, 0);
}

__private_extern__ void
vm_object_pmap_protect_options(
	vm_object_t                     object,
	vm_object_offset_t              offset,
	vm_object_size_t                size,
	pmap_t                          pmap,
	vm_map_size_t                   pmap_page_size,
	vm_map_offset_t                 pmap_start,
	vm_prot_t                       prot,
	int                             options)
{
	pmap_flush_context      pmap_flush_context_storage;
	boolean_t               delayed_pmap_flush = FALSE;
	vm_object_offset_t      offset_in_object;
	vm_object_size_t        size_in_object;

	if (object == VM_OBJECT_NULL) {
		return;
	}
	if (pmap_page_size > PAGE_SIZE) {
		                                 
		pmap_page_size = PAGE_SIZE;
	}
	  
                                                                  
                                        
    
	size_in_object = vm_object_round_page(offset + size) - vm_object_trunc_page(offset);
	offset_in_object = vm_object_trunc_page(offset);
	  
                                                                    
                                                                   
                                 
    
	assertf(!(offset & (pmap_page_size - 1)) && !(size & (pmap_page_size - 1)),
	    "offset 0x%llx size 0x%llx pmap_page_size 0x%llx",
	    offset, size, (uint64_t)pmap_page_size);

	vm_object_lock(object);

	if (object->phys_contiguous) {
		if (pmap != NULL) {
			vm_object_unlock(object);
			pmap_protect_options(pmap,
			    pmap_start,
			    pmap_start + size,
			    prot,
			    options & ~PMAP_OPTIONS_NOFLUSH,
			    NULL);
		} else {
			vm_object_offset_t phys_start, phys_end, phys_addr;

			phys_start = object->vo_shadow_offset + offset_in_object;
			phys_end = phys_start + size_in_object;
			assert(phys_start <= phys_end);
			assert(phys_end <= object->vo_shadow_offset + object->vo_size);
			vm_object_unlock(object);

			pmap_flush_context_init(&pmap_flush_context_storage);
			delayed_pmap_flush = FALSE;

			for (phys_addr = phys_start;
			    phys_addr < phys_end;
			    phys_addr += PAGE_SIZE_64) {
				pmap_page_protect_options(
					(ppnum_t) (phys_addr >> PAGE_SHIFT),
					prot,
					options | PMAP_OPTIONS_NOFLUSH,
					(void *)&pmap_flush_context_storage);
				delayed_pmap_flush = TRUE;
			}
			if (delayed_pmap_flush == TRUE) {
				pmap_flush(&pmap_flush_context_storage);
			}
		}
		return;
	}

	assert(object->internal);

	while (TRUE) {
		if (ptoa_64(object->resident_page_count) > size_in_object / 2 && pmap != PMAP_NULL) {
			vm_object_unlock(object);
			if (pmap_page_size < PAGE_SIZE) {
				DEBUG4K_PMAP("pmap %p start 0x%llx end 0x%llx prot 0x%x: pmap_protect()\n", pmap, (uint64_t)pmap_start, pmap_start + size, prot);
			}
			pmap_protect_options(pmap, pmap_start, pmap_start + size, prot,
			    options & ~PMAP_OPTIONS_NOFLUSH, NULL);
			return;
		}

		if (pmap_page_size < PAGE_SIZE) {
			DEBUG4K_PMAP("pmap %p start 0x%llx end 0x%llx prot 0x%x: offset 0x%llx size 0x%llx object %p offset 0x%llx size 0x%llx\n", pmap, (uint64_t)pmap_start, pmap_start + size, prot, offset, size, object, offset_in_object, size_in_object);
		}

		pmap_flush_context_init(&pmap_flush_context_storage);
		delayed_pmap_flush = FALSE;

		  
                                                          
                                                            
                                        
     
		if (ptoa_64(object->resident_page_count / 4) < size_in_object) {
			vm_page_t               p;
			vm_object_offset_t      end;

			end = offset_in_object + size_in_object;

			vm_page_queue_iterate(&object->memq, p, vmp_listq) {
				if (!p->vmp_fictitious && (offset_in_object <= p->vmp_offset) && (p->vmp_offset < end)) {
					vm_map_offset_t start;

					  
                                                              
                                                               
        
					start = pmap_start + p->vmp_offset - offset;

					if (pmap != PMAP_NULL) {
						vm_map_offset_t curr;
						for (curr = start;
						    curr < start + PAGE_SIZE_64;
						    curr += pmap_page_size) {
							if (curr < pmap_start) {
								continue;
							}
							if (curr >= pmap_start + size) {
								break;
							}
							pmap_protect_options(
								pmap,
								curr,
								curr + pmap_page_size,
								prot,
								options | PMAP_OPTIONS_NOFLUSH,
								&pmap_flush_context_storage);
						}
					} else {
						pmap_page_protect_options(
							VM_PAGE_GET_PHYS_PAGE(p),
							prot,
							options | PMAP_OPTIONS_NOFLUSH,
							&pmap_flush_context_storage);
					}
					delayed_pmap_flush = TRUE;
				}
			}
		} else {
			vm_page_t               p;
			vm_object_offset_t      end;
			vm_object_offset_t      target_off;

			end = offset_in_object + size_in_object;

			for (target_off = offset_in_object;
			    target_off < end; target_off += PAGE_SIZE) {
				p = vm_page_lookup(object, target_off);

				if (p != VM_PAGE_NULL) {
					vm_object_offset_t start;

					  
                                                              
                                                               
        
					start = pmap_start + (p->vmp_offset - offset);

					if (pmap != PMAP_NULL) {
						vm_map_offset_t curr;
						for (curr = start;
						    curr < start + PAGE_SIZE;
						    curr += pmap_page_size) {
							if (curr < pmap_start) {
								continue;
							}
							if (curr >= pmap_start + size) {
								break;
							}
							pmap_protect_options(
								pmap,
								curr,
								curr + pmap_page_size,
								prot,
								options | PMAP_OPTIONS_NOFLUSH,
								&pmap_flush_context_storage);
						}
					} else {
						pmap_page_protect_options(
							VM_PAGE_GET_PHYS_PAGE(p),
							prot,
							options | PMAP_OPTIONS_NOFLUSH,
							&pmap_flush_context_storage);
					}
					delayed_pmap_flush = TRUE;
				}
			}
		}
		if (delayed_pmap_flush == TRUE) {
			pmap_flush(&pmap_flush_context_storage);
		}

		if (prot == VM_PROT_NONE) {
			  
                                               
                                   
      
			vm_object_t     next_object;

			next_object = object->shadow;
			if (next_object != VM_OBJECT_NULL) {
				offset_in_object += object->vo_shadow_offset;
				offset += object->vo_shadow_offset;
				vm_object_lock(next_object);
				vm_object_unlock(object);
				object = next_object;
			} else {
				  
                                  
       
				break;
			}
		} else {
			  
                                              
                                          
      
			break;
		}
	}

	vm_object_unlock(object);
}

uint32_t vm_page_busy_absent_skipped = 0;

  
                                 
  
               
                                          
                                       
                                        
                                        
                               
  
                     
                                               
                                                     
                                       
  
           
                                                          
                                                       
                                                        
                                                        
                                                       
                                          
  
                                                
                                                     
                                                     
                                                 
                                                   
                   
   
__private_extern__ kern_return_t
vm_object_copy_slowly(
	vm_object_t             src_object,
	vm_object_offset_t      src_offset,
	vm_object_size_t        size,
	boolean_t               interruptible,
	vm_object_t             *_result_object)                 
{
	vm_object_t             new_object;
	vm_object_offset_t      new_offset;

	struct vm_object_fault_info fault_info = {};

	if (size == 0) {
		vm_object_unlock(src_object);
		*_result_object = VM_OBJECT_NULL;
		return KERN_INVALID_ARGUMENT;
	}

	  
                                                           
    

	vm_object_reference_locked(src_object);
	vm_object_unlock(src_object);

	  
                                                 
                
                                                 
                                     
                                                  
                                                     
    

	size = vm_object_round_page(src_offset + size) - vm_object_trunc_page(src_offset);
	src_offset = vm_object_trunc_page(src_offset);
	new_object = vm_object_allocate(size);
	new_offset = 0;

	assert(size == trunc_page_64(size));                                  

	fault_info.interruptible = interruptible;
	fault_info.behavior  = VM_BEHAVIOR_SEQUENTIAL;
	fault_info.lo_offset = src_offset;
	fault_info.hi_offset = src_offset + size;
	fault_info.stealth = TRUE;

	for (;
	    size != 0;
	    src_offset += PAGE_SIZE_64,
	    new_offset += PAGE_SIZE_64, size -= PAGE_SIZE_64
	    ) {
		vm_page_t       new_page;
		vm_fault_return_t result;

		vm_object_lock(new_object);

		while ((new_page = vm_page_alloc(new_object, new_offset))
		    == VM_PAGE_NULL) {
			vm_object_unlock(new_object);

			if (!vm_page_wait(interruptible)) {
				vm_object_deallocate(new_object);
				vm_object_deallocate(src_object);
				*_result_object = VM_OBJECT_NULL;
				return MACH_SEND_INTERRUPTED;
			}
			vm_object_lock(new_object);
		}
		vm_object_unlock(new_object);

		do {
			vm_prot_t       prot = VM_PROT_READ;
			vm_page_t       _result_page;
			vm_page_t       top_page;
			vm_page_t       result_page;
			kern_return_t   error_code;
			vm_object_t     result_page_object;


			vm_object_lock(src_object);

			if (src_object->internal &&
			    src_object->shadow == VM_OBJECT_NULL &&
			    (src_object->pager == NULL ||
			    (VM_COMPRESSOR_PAGER_STATE_GET(src_object,
			    src_offset) ==
			    VM_EXTERNAL_STATE_ABSENT))) {
				boolean_t can_skip_page;

				_result_page = vm_page_lookup(src_object,
				    src_offset);
				if (_result_page == VM_PAGE_NULL) {
					  
                                         
                                        
                                          
                               
                                            
                                           
                                           
                  
        
					can_skip_page = TRUE;
				} else if (workaround_41447923 &&
				    src_object->pager == NULL &&
				    _result_page != VM_PAGE_NULL &&
				    _result_page->vmp_busy &&
				    _result_page->vmp_absent &&
				    src_object->purgable == VM_PURGABLE_DENY &&
				    !src_object->blocked_access) {
					  
                                        
                                         
                                       
                                        
                                  
                                            
                           
                                          
                                           
                                             
                  
        
					assert(src_object->internal);
					assert(src_object->shadow == NULL);
					assert(src_object->pager == NULL);
					can_skip_page = TRUE;
					vm_page_busy_absent_skipped++;
				} else {
					can_skip_page = FALSE;
				}
				if (can_skip_page) {
					vm_object_unlock(src_object);
					                                   
					vm_object_lock(new_object);
					VM_PAGE_FREE(new_page);
					new_page = VM_PAGE_NULL;
					vm_object_unlock(new_object);
					                                            
					result = VM_FAULT_SUCCESS;
					break;
				}
			}

			vm_object_paging_begin(src_object);

			                                  
			upl_size_t cluster_size;
			if (os_convert_overflow(size, &cluster_size)) {
				cluster_size = 0 - (upl_size_t)PAGE_SIZE;
			}
			fault_info.cluster_size = cluster_size;

			_result_page = VM_PAGE_NULL;
			result = vm_fault_page(src_object, src_offset,
			    VM_PROT_READ, FALSE,
			    FALSE,                             
			    &prot, &_result_page, &top_page,
			    (int *)0,
			    &error_code, FALSE, FALSE, &fault_info);

			switch (result) {
			case VM_FAULT_SUCCESS:
				result_page = _result_page;
				result_page_object = VM_PAGE_OBJECT(result_page);

				  
                                       
      
                       
                                
                                 
                   
       

				vm_page_copy(result_page, new_page);
				vm_object_unlock(result_page_object);

				  
                                      
                                           
       
				vm_object_lock(new_object);
				SET_PAGE_DIRTY(new_page, FALSE);
				PAGE_WAKEUP_DONE(new_page);
				vm_object_unlock(new_object);

				vm_object_lock(result_page_object);
				PAGE_WAKEUP_DONE(result_page);

				vm_page_lockspin_queues();
				if ((result_page->vmp_q_state == VM_PAGE_ON_SPECULATIVE_Q) ||
				    (result_page->vmp_q_state == VM_PAGE_NOT_ON_Q)) {
					vm_page_activate(result_page);
				}
				vm_page_activate(new_page);
				vm_page_unlock_queues();

				  
                                    
                                          
       

				vm_fault_cleanup(result_page_object,
				    top_page);

				break;

			case VM_FAULT_RETRY:
				break;

			case VM_FAULT_MEMORY_SHORTAGE:
				if (vm_page_wait(interruptible)) {
					break;
				}
				OS_FALLTHROUGH;

			case VM_FAULT_INTERRUPTED:
				vm_object_lock(new_object);
				VM_PAGE_FREE(new_page);
				vm_object_unlock(new_object);

				vm_object_deallocate(new_object);
				vm_object_deallocate(src_object);
				*_result_object = VM_OBJECT_NULL;
				return MACH_SEND_INTERRUPTED;

			case VM_FAULT_SUCCESS_NO_VM_PAGE:
				                                  
				vm_object_paging_end(src_object);
				vm_object_unlock(src_object);
				OS_FALLTHROUGH;
			case VM_FAULT_MEMORY_ERROR:
				  
                       
                                     
               
                                    
                                  
       

				vm_object_lock(new_object);
				VM_PAGE_FREE(new_page);
				vm_object_unlock(new_object);

				vm_object_deallocate(new_object);
				vm_object_deallocate(src_object);
				*_result_object = VM_OBJECT_NULL;
				return error_code ? error_code:
				       KERN_MEMORY_ERROR;

			default:
				panic("vm_object_copy_slowly: unexpected error"
				    " 0x%x from vm_fault_page()\n", result);
			}
		} while (result != VM_FAULT_SUCCESS);
	}

	  
                                                    
    
	vm_object_deallocate(src_object);
	*_result_object = new_object;
	return KERN_SUCCESS;
}

  
                                  
  
           
                                                  
                                                    
                              
  
           
                                                      
                                                   
             
  
                     
                                                    
   

            
__private_extern__ boolean_t
vm_object_copy_quickly(
	vm_object_t             *_object,                          
	__unused vm_object_offset_t     offset,         
	__unused vm_object_size_t       size,           
	boolean_t               *_src_needs_copy,                
	boolean_t               *_dst_needs_copy)                
{
	vm_object_t     object = *_object;
	memory_object_copy_strategy_t copy_strategy;

	if (object == VM_OBJECT_NULL) {
		*_src_needs_copy = FALSE;
		*_dst_needs_copy = FALSE;
		return TRUE;
	}

	vm_object_lock(object);

	copy_strategy = object->copy_strategy;

	switch (copy_strategy) {
	case MEMORY_OBJECT_COPY_SYMMETRIC:

		  
                             
                                          
                                   
     

		vm_object_reference_locked(object);
		object->shadowed = TRUE;
		vm_object_unlock(object);

		  
                                          
                                         
                              
     

		*_src_needs_copy = TRUE;
		*_dst_needs_copy = TRUE;

		break;

	case MEMORY_OBJECT_COPY_DELAY:
		vm_object_unlock(object);
		return FALSE;

	default:
		vm_object_unlock(object);
		return FALSE;
	}
	return TRUE;
}

static int copy_call_count = 0;
static int copy_call_sleep_count = 0;
static int copy_call_restart_count = 0;

  
                                          
  
               
                                                  
                                
  
                     
                                                  
                               
  
           
                                                        
                                                   
                                                        
                                                          
                 
   
static kern_return_t
vm_object_copy_call(
	vm_object_t             src_object,
	vm_object_offset_t      src_offset,
	vm_object_size_t        size,
	vm_object_t             *_result_object)                 
{
	kern_return_t   kr;
	vm_object_t     copy;
	boolean_t       check_ready = FALSE;
	uint32_t        try_failed_count = 0;

	  
                                                     
   
       
                                                    
                      
   
        
                                                   
                                                    
                                                    
                                                         
    
	copy_call_count++;
	while (vm_object_wanted(src_object, VM_OBJECT_EVENT_COPY_CALL)) {
		vm_object_sleep(src_object, VM_OBJECT_EVENT_COPY_CALL,
		    THREAD_UNINT);
		copy_call_restart_count++;
	}

	  
                                                           
                                                            
                                                            
                               
    

	vm_object_set_wanted(src_object, VM_OBJECT_EVENT_COPY_CALL);
	vm_object_unlock(src_object);

	  
                                                     
                                              
                                                  
                                              
                                                     
                                  
    

	kr = KERN_FAILURE;                                                 
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	  
                                
    
	vm_object_lock(src_object);
	while (vm_object_wanted(src_object, VM_OBJECT_EVENT_COPY_CALL)) {
		vm_object_sleep(src_object, VM_OBJECT_EVENT_COPY_CALL,
		    THREAD_UNINT);
		copy_call_sleep_count++;
	}
Retry:
	assert(src_object->copy != VM_OBJECT_NULL);
	copy = src_object->copy;
	if (!vm_object_lock_try(copy)) {
		vm_object_unlock(src_object);

		try_failed_count++;
		mutex_pause(try_failed_count);                  

		vm_object_lock(src_object);
		goto Retry;
	}
	if (copy->vo_size < src_offset + size) {
		assertf(page_aligned(src_offset + size),
		    "object %p size 0x%llx",
		    copy, (uint64_t)(src_offset + size));
		copy->vo_size = src_offset + size;
	}

	if (!copy->pager_ready) {
		check_ready = TRUE;
	}

	  
                    
    
	*_result_object = copy;
	vm_object_unlock(copy);
	vm_object_unlock(src_object);

	                                    
	if (check_ready == TRUE) {
		vm_object_lock(copy);
		while (!copy->pager_ready) {
			vm_object_sleep(copy, VM_OBJECT_EVENT_PAGER_READY, THREAD_UNINT);
		}
		vm_object_unlock(copy);
	}

	return KERN_SUCCESS;
}

static int copy_delayed_lock_collisions = 0;
static int copy_delayed_max_collisions = 0;
static int copy_delayed_lock_contention = 0;
static int copy_delayed_protect_iterate = 0;

  
                                             
  
               
                                                   
                                           
  
                     
                                                                
                                                             
  
                                                          
                                     
   
__private_extern__ vm_object_t
vm_object_copy_delayed(
	vm_object_t             src_object,
	vm_object_offset_t      src_offset,
	vm_object_size_t        size,
	boolean_t               src_object_shared)
{
	vm_object_t             new_copy = VM_OBJECT_NULL;
	vm_object_t             old_copy;
	vm_page_t               p;
	vm_object_size_t        copy_size = src_offset + size;
	pmap_flush_context      pmap_flush_context_storage;
	boolean_t               delayed_pmap_flush = FALSE;


	int collisions = 0;
	  
                                                                 
                                                                  
            
   
                                                    
                                                        
                                                         
                                
                                                         
                                                        
                                                          
                                                   
                                                             
                                                         
                      
                                                          
                                                           
                                                      
                                                           
                                              
   
                                                                   
                                                                 
                                                                  
             
   
                                                           
                                                                 
                                                                  
                                                              
                                                                   
                                                    
   
                                                                 
                                                                  
                                                                  
    

	copy_size = vm_object_round_page(copy_size);
Retry:

	  
                                
    
	if (!src_object->true_share &&
	    (src_object->paging_in_progress != 0 ||
	    src_object->activity_in_progress != 0)) {
		if (src_object_shared == TRUE) {
			vm_object_unlock(src_object);
			vm_object_lock(src_object);
			src_object_shared = FALSE;
			goto Retry;
		}
		vm_object_paging_wait(src_object, THREAD_UNINT);
	}
	  
                                                     
                   
    

	old_copy = src_object->copy;
	if (old_copy != VM_OBJECT_NULL) {
		int lock_granted;

		  
                                        
     
		if (src_object_shared == TRUE) {
			lock_granted = vm_object_lock_try_shared(old_copy);
		} else {
			lock_granted = vm_object_lock_try(old_copy);
		}

		if (!lock_granted) {
			vm_object_unlock(src_object);

			if (collisions++ == 0) {
				copy_delayed_lock_contention++;
			}
			mutex_pause(collisions);

			                      
			copy_delayed_lock_collisions++;

			if (collisions > copy_delayed_max_collisions) {
				copy_delayed_max_collisions = collisions;
			}

			if (src_object_shared == TRUE) {
				vm_object_lock_shared(src_object);
			} else {
				vm_object_lock(src_object);
			}

			goto Retry;
		}

		  
                                              
                   
     

		if (old_copy->resident_page_count == 0 &&
		    !old_copy->pager_created) {
			  
                               
     
                                 
                                 
                               
              
      

			if (old_copy->vo_size < copy_size) {
				if (src_object_shared == TRUE) {
					vm_object_unlock(old_copy);
					vm_object_unlock(src_object);

					vm_object_lock(src_object);
					src_object_shared = FALSE;
					goto Retry;
				}
				  
                                                    
                                                     
                                                      
                                                     
                                 
       
				copy_delayed_protect_iterate++;

				pmap_flush_context_init(&pmap_flush_context_storage);
				delayed_pmap_flush = FALSE;

				vm_page_queue_iterate(&src_object->memq, p, vmp_listq) {
					if (!p->vmp_fictitious &&
					    p->vmp_offset >= old_copy->vo_size &&
					    p->vmp_offset < copy_size) {
						if (VM_PAGE_WIRED(p)) {
							vm_object_unlock(old_copy);
							vm_object_unlock(src_object);

							if (new_copy != VM_OBJECT_NULL) {
								vm_object_unlock(new_copy);
								vm_object_deallocate(new_copy);
							}
							if (delayed_pmap_flush == TRUE) {
								pmap_flush(&pmap_flush_context_storage);
							}

							return VM_OBJECT_NULL;
						} else {
							pmap_page_protect_options(VM_PAGE_GET_PHYS_PAGE(p), (VM_PROT_ALL & ~VM_PROT_WRITE),
							    PMAP_OPTIONS_NOFLUSH, (void *)&pmap_flush_context_storage);
							delayed_pmap_flush = TRUE;
						}
					}
				}
				if (delayed_pmap_flush == TRUE) {
					pmap_flush(&pmap_flush_context_storage);
				}

				assertf(page_aligned(copy_size),
				    "object %p size 0x%llx",
				    old_copy, (uint64_t)copy_size);
				old_copy->vo_size = copy_size;
			}
			if (src_object_shared == TRUE) {
				vm_object_reference_shared(old_copy);
			} else {
				vm_object_reference_locked(old_copy);
			}
			vm_object_unlock(old_copy);
			vm_object_unlock(src_object);

			if (new_copy != VM_OBJECT_NULL) {
				vm_object_unlock(new_copy);
				vm_object_deallocate(new_copy);
			}
			return old_copy;
		}



		  
                                                       
                                                        
                                        
     
		if (old_copy->vo_size > copy_size) {
			copy_size = old_copy->vo_size;
		}

		if (new_copy == VM_OBJECT_NULL) {
			vm_object_unlock(old_copy);
			vm_object_unlock(src_object);
			new_copy = vm_object_allocate(copy_size);
			vm_object_lock(src_object);
			vm_object_lock(new_copy);

			src_object_shared = FALSE;
			goto Retry;
		}
		assertf(page_aligned(copy_size),
		    "object %p size 0x%llx",
		    new_copy, (uint64_t)copy_size);
		new_copy->vo_size = copy_size;

		  
                                                   
                                                 
                                                 
                                             
     

		assert((old_copy->shadow == src_object) &&
		    (old_copy->vo_shadow_offset == (vm_object_offset_t) 0));
	} else if (new_copy == VM_OBJECT_NULL) {
		vm_object_unlock(src_object);
		new_copy = vm_object_allocate(copy_size);
		vm_object_lock(src_object);
		vm_object_lock(new_copy);

		src_object_shared = FALSE;
		goto Retry;
	}

	  
                                                              
                                                               
                                                            
                                                      
   
                                                               
                                                              
                                                                 
                                          
    
	copy_delayed_protect_iterate++;

	pmap_flush_context_init(&pmap_flush_context_storage);
	delayed_pmap_flush = FALSE;

	vm_page_queue_iterate(&src_object->memq, p, vmp_listq) {
		if (!p->vmp_fictitious && p->vmp_offset < copy_size) {
			if (VM_PAGE_WIRED(p)) {
				if (old_copy) {
					vm_object_unlock(old_copy);
				}
				vm_object_unlock(src_object);
				vm_object_unlock(new_copy);
				vm_object_deallocate(new_copy);

				if (delayed_pmap_flush == TRUE) {
					pmap_flush(&pmap_flush_context_storage);
				}

				return VM_OBJECT_NULL;
			} else {
				pmap_page_protect_options(VM_PAGE_GET_PHYS_PAGE(p), (VM_PROT_ALL & ~VM_PROT_WRITE),
				    PMAP_OPTIONS_NOFLUSH, (void *)&pmap_flush_context_storage);
				delayed_pmap_flush = TRUE;
			}
		}
	}
	if (delayed_pmap_flush == TRUE) {
		pmap_flush(&pmap_flush_context_storage);
	}

	if (old_copy != VM_OBJECT_NULL) {
		  
                                                 
                                                    
            
     

		                               
		vm_object_lock_assert_exclusive(src_object);
		src_object->ref_count--;
		assert(src_object->ref_count > 0);
		vm_object_lock_assert_exclusive(old_copy);
		old_copy->shadow = new_copy;
		vm_object_lock_assert_exclusive(new_copy);
		assert(new_copy->ref_count > 0);
		new_copy->ref_count++;                                         

		vm_object_unlock(old_copy);                             
	}

	  
                                              
    
	vm_object_lock_assert_exclusive(new_copy);
	new_copy->shadow = src_object;
	new_copy->vo_shadow_offset = 0;
	new_copy->shadowed = TRUE;                                      

	vm_object_lock_assert_exclusive(src_object);
	vm_object_reference_locked(src_object);
	src_object->copy = new_copy;
	vm_object_unlock(src_object);
	vm_object_unlock(new_copy);

	return new_copy;
}

  
                                        
  
           
                                                   
                                                 
                           
   
__private_extern__ kern_return_t
vm_object_copy_strategically(
	vm_object_t             src_object,
	vm_object_offset_t      src_offset,
	vm_object_size_t        size,
	vm_object_t             *dst_object,             
	vm_object_offset_t      *dst_offset,             
	boolean_t               *dst_needs_copy)          
{
	boolean_t       result;
	boolean_t       interruptible = THREAD_ABORTSAFE;          
	boolean_t       object_lock_shared = FALSE;
	memory_object_copy_strategy_t copy_strategy;

	assert(src_object != VM_OBJECT_NULL);

	copy_strategy = src_object->copy_strategy;

	if (copy_strategy == MEMORY_OBJECT_COPY_DELAY) {
		vm_object_lock_shared(src_object);
		object_lock_shared = TRUE;
	} else {
		vm_object_lock(src_object);
	}

	  
                                                         
                                                  
    

	while (!src_object->internal && !src_object->pager_ready) {
		wait_result_t wait_result;

		if (object_lock_shared == TRUE) {
			vm_object_unlock(src_object);
			vm_object_lock(src_object);
			object_lock_shared = FALSE;
			continue;
		}
		wait_result = vm_object_sleep(  src_object,
		    VM_OBJECT_EVENT_PAGER_READY,
		    interruptible);
		if (wait_result != THREAD_AWAKENED) {
			vm_object_unlock(src_object);
			*dst_object = VM_OBJECT_NULL;
			*dst_offset = 0;
			*dst_needs_copy = FALSE;
			return MACH_SEND_INTERRUPTED;
		}
	}

	  
                                      
    

	switch (copy_strategy) {
	case MEMORY_OBJECT_COPY_DELAY:
		*dst_object = vm_object_copy_delayed(src_object,
		    src_offset, size, object_lock_shared);
		if (*dst_object != VM_OBJECT_NULL) {
			*dst_offset = src_offset;
			*dst_needs_copy = TRUE;
			result = KERN_SUCCESS;
			break;
		}
		vm_object_lock(src_object);
		OS_FALLTHROUGH;                                              

	case MEMORY_OBJECT_COPY_NONE:
		result = vm_object_copy_slowly(src_object, src_offset, size,
		    interruptible, dst_object);
		if (result == KERN_SUCCESS) {
			*dst_offset = src_offset - vm_object_trunc_page(src_offset);
			*dst_needs_copy = FALSE;
		}
		break;

	case MEMORY_OBJECT_COPY_CALL:
		result = vm_object_copy_call(src_object, src_offset, size,
		    dst_object);
		if (result == KERN_SUCCESS) {
			*dst_offset = src_offset;
			*dst_needs_copy = TRUE;
		}
		break;

	case MEMORY_OBJECT_COPY_SYMMETRIC:
		vm_object_unlock(src_object);
		result = KERN_MEMORY_RESTART_COPY;
		break;

	default:
		panic("copy_strategically: bad strategy");
		result = KERN_INVALID_ARGUMENT;
	}
	return result;
}

  
                    
  
                                             
                                               
                                   
  
                                             
                                         
   
boolean_t vm_object_shadow_check = TRUE;

__private_extern__ boolean_t
vm_object_shadow(
	vm_object_t             *object,                    
	vm_object_offset_t      *offset,                    
	vm_object_size_t        length)
{
	vm_object_t     source;
	vm_object_t     result;

	source = *object;
	assert(source != VM_OBJECT_NULL);
	if (source == VM_OBJECT_NULL) {
		return FALSE;
	}

#if 0
	  
            
                                                                        
                                                                 
                                                                      
                                                                 
                                                                      
           
                                                                    
                                                                      
                                                                     
                                                 
    
	assert(source->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC);
#endif

	  
                                         
   
                                                          
                                                         
                                                        
                                                     
                                                        
                   
    

	assert(source->copy_strategy != MEMORY_OBJECT_COPY_NONE);                                                       

#if 00
	  
                                                                      
                                       
                                                                    
                                                                
                                                    
    
	if (vm_object_shadow_check &&
	    source->vo_size == length &&
	    source->ref_count == 1) {
		  
                                     
                                    
                                      
                                     
                                     
                                      
                                     
                     
     
		vm_object_lock(source);
		if (source->vo_size == length &&
		    source->ref_count == 1 &&
		    (source->shadow == VM_OBJECT_NULL ||
		    source->shadow->copy == VM_OBJECT_NULL)) {
			source->shadowed = FALSE;
			vm_object_unlock(source);
			return FALSE;
		}
		                                                      
		vm_object_unlock(source);
	}
#endif         

	  
                                                            
                                      
                                                            
                                                              
                                                           
    
	vm_object_offset_t offset_adjustment;
	offset_adjustment = *offset - vm_object_trunc_page(*offset);
	length = vm_object_round_page(length + offset_adjustment);
	*offset = vm_object_trunc_page(*offset);

	  
                                               
    

	if ((result = vm_object_allocate(length)) == VM_OBJECT_NULL) {
		panic("vm_object_shadow: no object for shadowing");
	}

	  
                                                    
                                                        
                                                       
                                                          
          
    
	result->shadow = source;

	  
                                            
                                              
    

	result->vo_shadow_offset = *offset;
	assertf(page_aligned(result->vo_shadow_offset),
	    "result %p shadow offset 0x%llx",
	    result, result->vo_shadow_offset);

	  
                         
    

	*offset = 0;
	if (offset_adjustment) {
		  
                                                      
                       
     
		DEBUG4K_COPY("adjusting offset @ %p from 0x%llx to 0x%llx for object %p length: 0x%llx\n", offset, *offset, *offset + offset_adjustment, result, length);
		*offset += offset_adjustment;
	}
	*object = result;
	return TRUE;
}

  
                                                    
                                                      
  
                                                             
                                                                          
                       
  
                                                
                                                  
                                             
                                                    
                      
  
                  
                                    
                                            
                                           
                                   
  
                                                           
                                                                 
                                                         
                                                               
  
                                                                
                                                              
                                                                    
                                                                  
                                                               
                                                                
                                                               
                                                              
                              
  
                                                                
                                            
  
                                                         
                      
                        
                                               
                                               
                                              
                                                   
          
                          
                                              
                                               
                                                
                                               
                                                   
  
                                                            
                                                             
                                                            
                                                             
                                         
  
                                                           
                                                          
                                                           
                                                          
                                                        
            
  
                                                                     
                                                                 
                                                                
                                                              
                                                                        
                                                                      
                                                
   


  
                                             
           
                                             
                                               
                         
   
vm_object_t
vm_object_memory_object_associate(
	memory_object_t         pager,
	vm_object_t             object,
	vm_object_size_t        size,
	boolean_t               named)
{
	memory_object_control_t control;

	assert(pager != MEMORY_OBJECT_NULL);

	if (object != VM_OBJECT_NULL) {
		assert(object->internal);
		assert(object->pager_created);
		assert(!object->pager_initialized);
		assert(!object->pager_ready);
		assert(object->pager_trusted);
	} else {
		object = vm_object_allocate(size);
		assert(object != VM_OBJECT_NULL);
		object->internal = FALSE;
		object->pager_trusted = FALSE;
		                                                       
		object->copy_strategy = MEMORY_OBJECT_COPY_INVALID;
	}

	  
                          
    

	control = memory_object_control_allocate(object);
	assert(control != MEMORY_OBJECT_CONTROL_NULL);

	vm_object_lock(object);

	assert(!object->pager_ready);
	assert(!object->pager_initialized);
	assert(object->pager == NULL);
	assert(object->pager_control == NULL);

	  
                                     
    

	memory_object_reference(pager);
	object->pager_created = TRUE;
	object->pager = pager;
	object->pager_control = control;
	object->pager_ready = FALSE;

	vm_object_unlock(object);

	  
                                      
    

	(void) memory_object_init(pager,
	    object->pager_control,
	    PAGE_SIZE);

	vm_object_lock(object);
	if (named) {
		object->named = TRUE;
	}
	if (object->internal) {
		object->pager_ready = TRUE;
		vm_object_wakeup(object, VM_OBJECT_EVENT_PAGER_READY);
	}

	object->pager_initialized = TRUE;
	vm_object_wakeup(object, VM_OBJECT_EVENT_INITIALIZED);

	vm_object_unlock(object);

	return object;
}

  
                                             
           
                                                  
                     
                                           
                                        
               
                                       
                                                     
                                         
                                      
   

void
vm_object_compressor_pager_create(
	vm_object_t     object)
{
	memory_object_t         pager;
	vm_object_t             pager_object = VM_OBJECT_NULL;

	assert(object != kernel_object);

	  
                                                                 
    

	vm_object_paging_begin(object);
	if (object->pager_created) {
		  
                                    
                                                   
     
		while (!object->pager_initialized) {
			vm_object_sleep(object,
			    VM_OBJECT_EVENT_INITIALIZED,
			    THREAD_UNINT);
		}
		vm_object_paging_end(object);
		return;
	}

	if ((uint32_t) (object->vo_size / PAGE_SIZE) !=
	    (object->vo_size / PAGE_SIZE)) {
#if DEVELOPMENT || DEBUG
		printf("vm_object_compressor_pager_create(%p): "
		    "object size 0x%llx >= 0x%llx\n",
		    object,
		    (uint64_t) object->vo_size,
		    0x0FFFFFFFFULL * PAGE_SIZE);
#endif                           
		vm_object_paging_end(object);
		return;
	}

	  
                                                   
                                                
    

	object->pager_created = TRUE;
	object->pager_trusted = TRUE;
	object->paging_offset = 0;

	vm_object_unlock(object);

	  
                                                                   
   
                                                          
                                                                
                                   
    
	{
		                                  
		assert((uint32_t) (object->vo_size / PAGE_SIZE) ==
		    (object->vo_size / PAGE_SIZE));
		(void) compressor_memory_object_create(
			(memory_object_size_t) object->vo_size,
			&pager);
		if (pager == NULL) {
			panic("vm_object_compressor_pager_create(): "
			    "no pager for object %p size 0x%llx\n",
			    object, (uint64_t) object->vo_size);
		}
	}

	  
                               
                                     
                                                  
    

	pager_object = vm_object_memory_object_associate(pager,
	    object,
	    object->vo_size,
	    FALSE);
	if (pager_object != object) {
		panic("vm_object_compressor_pager_create: mismatch (pager: %p, pager_object: %p, orig_object: %p, orig_object size: 0x%llx)\n", pager, pager_object, object, (uint64_t) object->vo_size);
	}

	  
                                      
    
	memory_object_deallocate(pager);

	vm_object_lock(object);

	  
                                
    
	vm_object_paging_end(object);
}

  
                                             
  
                                             
                                                      
   
static long     object_collapses = 0;
static long     object_bypasses  = 0;

static boolean_t        vm_object_collapse_allowed = TRUE;
static boolean_t        vm_object_bypass_allowed = TRUE;

void vm_object_do_collapse_compressor(vm_object_t object,
    vm_object_t backing_object);
void
vm_object_do_collapse_compressor(
	vm_object_t object,
	vm_object_t backing_object)
{
	vm_object_offset_t new_offset, backing_offset;
	vm_object_size_t size;

	vm_counters.do_collapse_compressor++;

	vm_object_lock_assert_exclusive(object);
	vm_object_lock_assert_exclusive(backing_object);

	size = object->vo_size;

	  
                                                 
                  
    

	for (backing_offset = object->vo_shadow_offset;
	    backing_offset < object->vo_shadow_offset + object->vo_size;
	    backing_offset += PAGE_SIZE) {
		memory_object_offset_t backing_pager_offset;

		                                                           
		backing_pager_offset = (backing_offset +
		    backing_object->paging_offset);
		backing_pager_offset = vm_compressor_pager_next_compressed(
			backing_object->pager,
			backing_pager_offset);
		if (backing_pager_offset == (memory_object_offset_t) -1) {
			                              
			break;
		}
		backing_offset = (backing_pager_offset -
		    backing_object->paging_offset);

		new_offset = backing_offset - object->vo_shadow_offset;

		if (new_offset >= object->vo_size) {
			                                              
			break;
		}

		if ((vm_page_lookup(object, new_offset) != VM_PAGE_NULL) ||
		    (vm_compressor_pager_state_get(object->pager,
		    (new_offset +
		    object->paging_offset)) ==
		    VM_EXTERNAL_STATE_EXISTS)) {
			  
                                                     
                 
                                                          
                                              
                     
      
			continue;
		}

		  
                                                          
                                      
     
		vm_counters.do_collapse_compressor_pages++;
		vm_compressor_pager_transfer(
			                  
			object->pager,
			(new_offset + object->paging_offset),
			             
			backing_object->pager,
			(backing_offset + backing_object->paging_offset));
	}
}

  
                                 
           
                                                  
                                                  
                                                  
              
                                                   
                        
  
   
static void
vm_object_do_collapse(
	vm_object_t object,
	vm_object_t backing_object)
{
	vm_page_t p, pp;
	vm_object_offset_t new_offset, backing_offset;
	vm_object_size_t size;

	vm_object_lock_assert_exclusive(object);
	vm_object_lock_assert_exclusive(backing_object);

	assert(object->purgable == VM_PURGABLE_DENY);
	assert(backing_object->purgable == VM_PURGABLE_DENY);

	backing_offset = object->vo_shadow_offset;
	size = object->vo_size;

	  
                                                
                                                  
                                              
                           
    

	while (!vm_page_queue_empty(&backing_object->memq)) {
		p = (vm_page_t) vm_page_queue_first(&backing_object->memq);

		new_offset = (p->vmp_offset - backing_offset);

		assert(!p->vmp_busy || p->vmp_absent);

		  
                                         
                                        
                   
    
                                   
     

		if (p->vmp_offset < backing_offset || new_offset >= size) {
			VM_PAGE_FREE(p);
		} else {
			pp = vm_page_lookup(object, new_offset);
			if (pp == VM_PAGE_NULL) {
				if (VM_COMPRESSOR_PAGER_STATE_GET(object,
				    new_offset)
				    == VM_EXTERNAL_STATE_EXISTS) {
					  
                                   
                             
                              
                      
        
					VM_PAGE_FREE(p);
				} else {
					  
                               
                                      
                
        
					vm_page_rename(p, object, new_offset);
				}
			} else {
				assert(!pp->vmp_absent);

				  
                                     
                                      
            
       
				VM_PAGE_FREE(p);
			}
		}
	}

	if (vm_object_collapse_compressor_allowed &&
	    object->pager != MEMORY_OBJECT_NULL &&
	    backing_object->pager != MEMORY_OBJECT_NULL) {
		                                                         
		vm_object_do_collapse_compressor(object, backing_object);
	} else if (backing_object->pager != MEMORY_OBJECT_NULL) {
		assert((!object->pager_created &&
		    (object->pager == MEMORY_OBJECT_NULL)) ||
		    (!backing_object->pager_created &&
		    (backing_object->pager == MEMORY_OBJECT_NULL)));
		  
                                                  
    
                                                  
                                             
                    
     

		assert(!object->paging_in_progress);
		assert(!object->activity_in_progress);
		assert(!object->pager_created);
		assert(object->pager == NULL);
		object->pager = backing_object->pager;

		object->pager_created = backing_object->pager_created;
		object->pager_control = backing_object->pager_control;
		object->pager_ready = backing_object->pager_ready;
		object->pager_initialized = backing_object->pager_initialized;
		object->paging_offset =
		    backing_object->paging_offset + backing_offset;
		if (object->pager_control != MEMORY_OBJECT_CONTROL_NULL) {
			memory_object_control_collapse(&object->pager_control,
			    object);
		}
		                                                             
		backing_object->pager_created = FALSE;
		backing_object->pager_control = NULL;
		backing_object->pager_ready = FALSE;
		backing_object->paging_offset = 0;
		backing_object->pager = NULL;
	}
	  
                                                   
                                                     
                                                      
    

	assert(!object->phys_contiguous);
	assert(!backing_object->phys_contiguous);
	object->shadow = backing_object->shadow;
	if (object->shadow) {
		assertf(page_aligned(object->vo_shadow_offset),
		    "object %p shadow_offset 0x%llx",
		    object, object->vo_shadow_offset);
		assertf(page_aligned(backing_object->vo_shadow_offset),
		    "backing_object %p shadow_offset 0x%llx",
		    backing_object, backing_object->vo_shadow_offset);
		object->vo_shadow_offset += backing_object->vo_shadow_offset;
		                                                  
		backing_object->shadow = VM_OBJECT_NULL;
		backing_object->vo_shadow_offset = 0;
	} else {
		                                              
		object->vo_shadow_offset = 0;
	}
	assert((object->shadow == VM_OBJECT_NULL) ||
	    (object->shadow->copy != backing_object));

	  
                           
   
                                             
                                                   
                                              
    
	object_collapses++;

	assert(backing_object->ref_count == 1);
	assert(backing_object->resident_page_count == 0);
	assert(backing_object->paging_in_progress == 0);
	assert(backing_object->activity_in_progress == 0);
	assert(backing_object->shadow == VM_OBJECT_NULL);
	assert(backing_object->vo_shadow_offset == 0);

	if (backing_object->pager != MEMORY_OBJECT_NULL) {
		                                                            
		vm_counters.do_collapse_terminate++;
		if (vm_object_terminate(backing_object) != KERN_SUCCESS) {
			vm_counters.do_collapse_terminate_failure++;
		}
		return;
	}

	assert(backing_object->pager == NULL);

	backing_object->alive = FALSE;
	vm_object_unlock(backing_object);

#if VM_OBJECT_TRACKING
	if (vm_object_tracking_inited) {
		btlog_remove_entries_for_element(vm_object_tracking_btlog,
		    backing_object);
	}
#endif                         

	vm_object_lock_destroy(backing_object);

	zfree(vm_object_zone, backing_object);
}

static void
vm_object_do_bypass(
	vm_object_t object,
	vm_object_t backing_object)
{
	  
                                          
                 
    

	vm_object_lock_assert_exclusive(object);
	vm_object_lock_assert_exclusive(backing_object);

	vm_object_reference(backing_object->shadow);

	assert(!object->phys_contiguous);
	assert(!backing_object->phys_contiguous);
	object->shadow = backing_object->shadow;
	if (object->shadow) {
		assertf(page_aligned(object->vo_shadow_offset),
		    "object %p shadow_offset 0x%llx",
		    object, object->vo_shadow_offset);
		assertf(page_aligned(backing_object->vo_shadow_offset),
		    "backing_object %p shadow_offset 0x%llx",
		    backing_object, backing_object->vo_shadow_offset);
		object->vo_shadow_offset += backing_object->vo_shadow_offset;
	} else {
		                                              
		object->vo_shadow_offset = 0;
	}

	  
                                                
                                
    
	if (backing_object->copy == object) {
		backing_object->copy = VM_OBJECT_NULL;
	}

	  
                                               
                  
                                          
                                             
                         
                                       
   
                                          
                                             
                                          
                                                
                                               
                           
       
                                          
                            
   
                                              
                                            
                                              
                              
        
    
	if (backing_object->ref_count > 2 ||
	    (!backing_object->named && backing_object->ref_count > 1)) {
		vm_object_lock_assert_exclusive(backing_object);
		backing_object->ref_count--;
		vm_object_unlock(backing_object);
	} else {
		  
                                         
                        
     

		  
                                                        
                                                           
                                                         
                                                            
                                                             
                                                                
                                                              
              
     
		vm_object_activity_begin(object);
		vm_object_unlock(object);

		vm_object_unlock(backing_object);
		vm_object_deallocate(backing_object);

		  
                                             
                                            
                                       
                     
     

		vm_object_lock(object);
		vm_object_activity_end(object);
	}

	object_bypasses++;
}


  
                      
  
                                                                 
                                                            
                                                              
  
                                                                      
  
   
static unsigned long vm_object_collapse_calls = 0;
static unsigned long vm_object_collapse_objects = 0;
static unsigned long vm_object_collapse_do_collapse = 0;
static unsigned long vm_object_collapse_do_bypass = 0;

__private_extern__ void
vm_object_collapse(
	vm_object_t                             object,
	vm_object_offset_t                      hint_offset,
	boolean_t                               can_bypass)
{
	vm_object_t                             backing_object;
	unsigned int                            rcount;
	unsigned int                            size;
	vm_object_t                             original_object;
	int                                     object_lock_type;
	int                                     backing_object_lock_type;

	vm_object_collapse_calls++;

	assertf(page_aligned(hint_offset), "hint_offset 0x%llx", hint_offset);

	if (!vm_object_collapse_allowed &&
	    !(can_bypass && vm_object_bypass_allowed)) {
		return;
	}

	if (object == VM_OBJECT_NULL) {
		return;
	}

	original_object = object;

	  
                                                        
                                                                        
                                                                    
                                                               
    
	object_lock_type = OBJECT_LOCK_EXCLUSIVE;
	backing_object_lock_type = OBJECT_LOCK_SHARED;

retry:
	object = original_object;
	vm_object_lock_assert_exclusive(object);

	while (TRUE) {
		vm_object_collapse_objects++;
		  
                                                    
                        
     

		  
                                   
     

		backing_object = object->shadow;
		if (backing_object == VM_OBJECT_NULL) {
			if (object != original_object) {
				vm_object_unlock(object);
			}
			return;
		}
		if (backing_object_lock_type == OBJECT_LOCK_SHARED) {
			vm_object_lock_shared(backing_object);
		} else {
			vm_object_lock(backing_object);
		}

		  
                                         
                         
     
		if (object->paging_in_progress != 0 ||
		    object->activity_in_progress != 0) {
			                                                   
			if (object != original_object) {
				vm_object_unlock(object);
			}
			object = backing_object;
			object_lock_type = backing_object_lock_type;
			continue;
		}

		  
        
                                          
                                            
                                
                                     
    
     

		if (!backing_object->internal ||
		    backing_object->paging_in_progress != 0 ||
		    backing_object->activity_in_progress != 0) {
			                                                   
			if (object != original_object) {
				vm_object_unlock(object);
			}
			object = backing_object;
			object_lock_type = backing_object_lock_type;
			continue;
		}

		  
                                                    
                                                 
                                                        
            
                                                     
                                                 
     
		if (object->purgable != VM_PURGABLE_DENY ||
		    backing_object->purgable != VM_PURGABLE_DENY) {
			panic("vm_object_collapse() attempting to collapse "
			    "purgeable object: %p(%d) %p(%d)\n",
			    object, object->purgable,
			    backing_object, backing_object->purgable);
			                                                   
			if (object != original_object) {
				vm_object_unlock(object);
			}
			object = backing_object;
			object_lock_type = backing_object_lock_type;
			continue;
		}

		  
                                               
                                                    
                                                 
                                                  
                                                  
                                                        
                                                       
                   
     
		if (backing_object->shadow != VM_OBJECT_NULL &&
		    backing_object->shadow->copy == backing_object) {
			                                                   
			if (object != original_object) {
				vm_object_unlock(object);
			}
			object = backing_object;
			object_lock_type = backing_object_lock_type;
			continue;
		}

		  
                                                  
                                                   
                                                   
           
    
                                                     
                                                   
            
    
                                                     
                                        
     
		if (backing_object->ref_count == 1 &&
		    (vm_object_collapse_compressor_allowed ||
		    !object->pager_created
		    || (!backing_object->pager_created)
		    ) && vm_object_collapse_allowed) {
			  
                                                   
      
			if (backing_object_lock_type != OBJECT_LOCK_EXCLUSIVE) {
				  
                                              
                                                 
                                                 
                                                  
                                             
                                             
                                                   
                       
                                             
                                                  
                                              
       
				vm_object_unlock(backing_object);
				if (object != original_object) {
					vm_object_unlock(object);
				}
				object_lock_type = OBJECT_LOCK_EXCLUSIVE;
				backing_object_lock_type = OBJECT_LOCK_EXCLUSIVE;
				goto retry;
			}

			  
                                          
                                             
                         
      

			vm_object_do_collapse(object, backing_object);
			vm_object_collapse_do_collapse++;
			continue;
		}

		  
                                                   
                                             
     

		if (!(can_bypass && vm_object_bypass_allowed)) {
			                                                   
			if (object != original_object) {
				vm_object_unlock(object);
			}
			object = backing_object;
			object_lock_type = backing_object_lock_type;
			continue;
		}


		  
                                                      
                                                        
                                        
     
		size = (unsigned int)atop(object->vo_size);
		rcount = object->resident_page_count;

		if (rcount != size) {
			vm_object_offset_t      offset;
			vm_object_offset_t      backing_offset;
			unsigned int            backing_rcount;

			  
                                                       
                                                     
                        
      
			if (backing_object->pager_created) {
				                                                   
				if (object != original_object) {
					vm_object_unlock(object);
				}
				object = backing_object;
				object_lock_type = backing_object_lock_type;
				continue;
			}

			  
                                               
                                                     
                        
      
			if (object->pager_created) {
				                                                   
				if (object != original_object) {
					vm_object_unlock(object);
				}
				object = backing_object;
				object_lock_type = backing_object_lock_type;
				continue;
			}

			backing_offset = object->vo_shadow_offset;
			backing_rcount = backing_object->resident_page_count;

			if ((int)backing_rcount - (int)(atop(backing_object->vo_size) - size) > (int)rcount) {
				  
                                                                   
                                                                
                                                     
                                                   
       
				if (object != original_object) {
					vm_object_unlock(object);
				}
				object = backing_object;
				object_lock_type = backing_object_lock_type;
				continue;
			}

			  
                                                   
                                               
                                                
                                               
            
     
                                               
                                              
              
     
      

#define EXISTS_IN_OBJECT(obj, off, rc)                  \
	((VM_COMPRESSOR_PAGER_STATE_GET((obj), (off))   \
	  == VM_EXTERNAL_STATE_EXISTS) ||               \
	 ((rc) && vm_page_lookup((obj), (off)) != VM_PAGE_NULL && (rc)--))

			  
                                   
                                                       
      
			if (object->cow_hint != ~(vm_offset_t)0) {
				hint_offset = (vm_object_offset_t)object->cow_hint;
			} else {
				hint_offset = (hint_offset > 8 * PAGE_SIZE_64) ?
				    (hint_offset - 8 * PAGE_SIZE_64) : 0;
			}

			if (EXISTS_IN_OBJECT(backing_object, hint_offset +
			    backing_offset, backing_rcount) &&
			    !EXISTS_IN_OBJECT(object, hint_offset, rcount)) {
				                                  
				object->cow_hint = (vm_offset_t) hint_offset;             
				                                                   
				if (object != original_object) {
					vm_object_unlock(object);
				}
				object = backing_object;
				object_lock_type = backing_object_lock_type;
				continue;
			}

			  
                                                    
                                                 
                                                    
                                                     
     
                                                         
                                                      
                                                     
                                                         
                                                       
                                                    
                                               
      
			if (backing_rcount && backing_rcount < (size / 8)) {
				unsigned int rc = rcount;
				vm_page_t p;

				backing_rcount = backing_object->resident_page_count;
				p = (vm_page_t)vm_page_queue_first(&backing_object->memq);
				do {
					offset = (p->vmp_offset - backing_offset);

					if (offset < object->vo_size &&
					    offset != hint_offset &&
					    !EXISTS_IN_OBJECT(object, offset, rc)) {
						                        
						object->cow_hint = (vm_offset_t) offset;             

						break;
					}
					p = (vm_page_t) vm_page_queue_next(&p->vmp_listq);
				} while (--backing_rcount);
				if (backing_rcount != 0) {
					                                                   
					if (object != original_object) {
						vm_object_unlock(object);
					}
					object = backing_object;
					object_lock_type = backing_object_lock_type;
					continue;
				}
			}

			  
                                                       
                                                     
      
			if (backing_rcount) {
				offset = hint_offset;

				while ((offset =
				    (offset + PAGE_SIZE_64 < object->vo_size) ?
				    (offset + PAGE_SIZE_64) : 0) != hint_offset) {
					if (EXISTS_IN_OBJECT(backing_object, offset +
					    backing_offset, backing_rcount) &&
					    !EXISTS_IN_OBJECT(object, offset, rcount)) {
						                        
						object->cow_hint = (vm_offset_t) offset;             
						break;
					}
				}
				if (offset != hint_offset) {
					                                                   
					if (object != original_object) {
						vm_object_unlock(object);
					}
					object = backing_object;
					object_lock_type = backing_object_lock_type;
					continue;
				}
			}
		}

		  
                                                   
     
		if (backing_object_lock_type != OBJECT_LOCK_EXCLUSIVE) {
			vm_object_unlock(backing_object);
			if (object != original_object) {
				vm_object_unlock(object);
			}
			object_lock_type = OBJECT_LOCK_EXCLUSIVE;
			backing_object_lock_type = OBJECT_LOCK_EXCLUSIVE;
			goto retry;
		}

		                                                               
		object->cow_hint = (vm_offset_t)0;

		  
                                                
                                             
                                           
     

		vm_object_do_bypass(object, backing_object);
		vm_object_collapse_do_bypass++;

		  
                                                     
     

		continue;
	}

	                 
	  
                                     
                                   
      
    
}

  
                                             
           
                                               
                                                 
  
                     
                              
                                                        
                                     
   
unsigned int vm_object_page_remove_lookup = 0;
unsigned int vm_object_page_remove_iterate = 0;

__private_extern__ void
vm_object_page_remove(
	vm_object_t             object,
	vm_object_offset_t      start,
	vm_object_offset_t      end)
{
	vm_page_t       p, next;

	  
                                               
                                                
                                              
    

	if (atop_64(end - start) < (unsigned)object->resident_page_count / 16) {
		vm_object_page_remove_lookup++;

		for (; start < end; start += PAGE_SIZE_64) {
			p = vm_page_lookup(object, start);
			if (p != VM_PAGE_NULL) {
				assert(!p->vmp_cleaning && !p->vmp_laundry);
				if (!p->vmp_fictitious && p->vmp_pmapped) {
					pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(p));
				}
				VM_PAGE_FREE(p);
			}
		}
	} else {
		vm_object_page_remove_iterate++;

		p = (vm_page_t) vm_page_queue_first(&object->memq);
		while (!vm_page_queue_end(&object->memq, (vm_page_queue_entry_t) p)) {
			next = (vm_page_t) vm_page_queue_next(&p->vmp_listq);
			if ((start <= p->vmp_offset) && (p->vmp_offset < end)) {
				assert(!p->vmp_cleaning && !p->vmp_laundry);
				if (!p->vmp_fictitious && p->vmp_pmapped) {
					pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(p));
				}
				VM_PAGE_FREE(p);
			}
			p = next;
		}
	}
}


  
                              
                                                       
                                            
  
                                         
  
                                                                
                                               
  
              
                                        
                                       
                                           
                                       
  
                                              
                                              
  
              
                                                             
                                              
   
static int vm_object_coalesce_count = 0;

__private_extern__ boolean_t
vm_object_coalesce(
	vm_object_t                     prev_object,
	vm_object_t                     next_object,
	vm_object_offset_t              prev_offset,
	__unused vm_object_offset_t next_offset,
	vm_object_size_t                prev_size,
	vm_object_size_t                next_size)
{
	vm_object_size_t        newsize;

#ifdef  lint
	next_offset++;
#endif            

	if (next_object != VM_OBJECT_NULL) {
		return FALSE;
	}

	if (prev_object == VM_OBJECT_NULL) {
		return TRUE;
	}

	vm_object_lock(prev_object);

	  
                                    
    
	vm_object_collapse(prev_object, prev_offset, TRUE);

	  
                                         
                                     
                             
               
                            
                          
                  
                                                     
    

	if ((prev_object->ref_count > 1) ||
	    prev_object->pager_created ||
	    (prev_object->shadow != VM_OBJECT_NULL) ||
	    (prev_object->copy != VM_OBJECT_NULL) ||
	    (prev_object->true_share != FALSE) ||
	    (prev_object->purgable != VM_PURGABLE_DENY) ||
	    (prev_object->paging_in_progress != 0) ||
	    (prev_object->activity_in_progress != 0)) {
		vm_object_unlock(prev_object);
		return FALSE;
	}

	vm_object_coalesce_count++;

	  
                                                         
                            
    
	vm_object_page_remove(prev_object,
	    prev_offset + prev_size,
	    prev_offset + prev_size + next_size);

	  
                                   
    
	newsize = prev_offset + prev_size + next_size;
	if (newsize > prev_object->vo_size) {
		assertf(page_aligned(newsize),
		    "object %p size 0x%llx",
		    prev_object, (uint64_t)newsize);
		prev_object->vo_size = newsize;
	}

	vm_object_unlock(prev_object);
	return TRUE;
}

kern_return_t
vm_object_populate_with_private(
	vm_object_t             object,
	vm_object_offset_t      offset,
	ppnum_t                 phys_page,
	vm_size_t               size)
{
	ppnum_t                 base_page;
	vm_object_offset_t      base_offset;


	if (!object->private) {
		return KERN_FAILURE;
	}

	base_page = phys_page;

	vm_object_lock(object);

	if (!object->phys_contiguous) {
		vm_page_t       m;

		if ((base_offset = trunc_page_64(offset)) != offset) {
			vm_object_unlock(object);
			return KERN_FAILURE;
		}
		base_offset += object->paging_offset;

		while (size) {
			m = vm_page_lookup(object, base_offset);

			if (m != VM_PAGE_NULL) {
				if (m->vmp_fictitious) {
					if (VM_PAGE_GET_PHYS_PAGE(m) != vm_page_guard_addr) {
						vm_page_lockspin_queues();
						m->vmp_private = TRUE;
						vm_page_unlock_queues();

						m->vmp_fictitious = FALSE;
						VM_PAGE_SET_PHYS_PAGE(m, base_page);
					}
				} else if (VM_PAGE_GET_PHYS_PAGE(m) != base_page) {
					if (!m->vmp_private) {
						  
                                                     
         
						panic("vm_object_populate_with_private - %p not private", m);
					}
					if (m->vmp_pmapped) {
						  
                                       
         
						pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m));
					}
					VM_PAGE_SET_PHYS_PAGE(m, base_page);
				}
			} else {
				m = vm_page_grab_fictitious(TRUE);

				  
                                                         
                                                        
       
				m->vmp_private = TRUE;
				m->vmp_fictitious = FALSE;
				VM_PAGE_SET_PHYS_PAGE(m, base_page);
				m->vmp_unusual = TRUE;
				m->vmp_busy = FALSE;

				vm_page_insert(m, object, base_offset);
			}
			base_page++;                                                                                                      
			base_offset += PAGE_SIZE;
			size -= PAGE_SIZE;
		}
	} else {
		                                                      
		                                                         
		                          

		            

		                                                  
		                                           
		object->vo_shadow_offset = (vm_object_offset_t)phys_page << PAGE_SHIFT;
		assertf(page_aligned(size),
		    "object %p size 0x%llx",
		    object, (uint64_t)size);
		object->vo_size = size;
	}
	vm_object_unlock(object);

	return KERN_SUCCESS;
}


kern_return_t
memory_object_create_named(
	memory_object_t pager,
	memory_object_offset_t  size,
	memory_object_control_t         *control)
{
	vm_object_t             object;

	*control = MEMORY_OBJECT_CONTROL_NULL;
	if (pager == MEMORY_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	object = vm_object_memory_object_associate(pager,
	    VM_OBJECT_NULL,
	    size,
	    TRUE);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_OBJECT;
	}

	                                          
	if (object != VM_OBJECT_NULL) {
		vm_object_lock(object);
		object->named = TRUE;
		while (!object->pager_ready) {
			vm_object_sleep(object,
			    VM_OBJECT_EVENT_PAGER_READY,
			    THREAD_UNINT);
		}
		*control = object->pager_control;
		vm_object_unlock(object);
	}
	return KERN_SUCCESS;
}


  
                                                        
           
                                                         
                                                          
                                                             
                            
           
                                                               
                                                                 
                                                     
   
kern_return_t
memory_object_recover_named(
	memory_object_control_t control,
	boolean_t               wait_on_terminating)
{
	vm_object_t             object;

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}
restart:
	vm_object_lock(object);

	if (object->terminating && wait_on_terminating) {
		vm_object_wait(object,
		    VM_OBJECT_EVENT_PAGING_IN_PROGRESS,
		    THREAD_UNINT);
		goto restart;
	}

	if (!object->alive) {
		vm_object_unlock(object);
		return KERN_FAILURE;
	}

	if (object->named == TRUE) {
		vm_object_unlock(object);
		return KERN_SUCCESS;
	}
	object->named = TRUE;
	vm_object_lock_assert_exclusive(object);
	object->ref_count++;
	while (!object->pager_ready) {
		vm_object_sleep(object,
		    VM_OBJECT_EVENT_PAGER_READY,
		    THREAD_UNINT);
	}
	vm_object_unlock(object);
	return KERN_SUCCESS;
}


  
                          
  
                                                                    
                                                                   
                                                           
  
                                                                 
                                                                        
                  
                                                                   
                                                                      
                                                            
                                                                    
                                                        
   

__private_extern__ kern_return_t
vm_object_release_name(
	vm_object_t     object,
	int             flags)
{
	vm_object_t     shadow;
	boolean_t       original_object = TRUE;

	while (object != VM_OBJECT_NULL) {
		vm_object_lock(object);

		assert(object->alive);
		if (original_object) {
			assert(object->named);
		}
		assert(object->ref_count > 0);

		  
                                              
                                      
     

		if (object->pager_created && !object->pager_initialized) {
			assert(!object->can_persist);
			vm_object_assert_wait(object,
			    VM_OBJECT_EVENT_INITIALIZED,
			    THREAD_UNINT);
			vm_object_unlock(object);
			thread_block(THREAD_CONTINUE_NULL);
			continue;
		}

		if (((object->ref_count > 1)
		    && (flags & MEMORY_OBJECT_TERMINATE_IDLE))
		    || (object->terminating)) {
			vm_object_unlock(object);
			return KERN_FAILURE;
		} else {
			if (flags & MEMORY_OBJECT_RELEASE_NO_OP) {
				vm_object_unlock(object);
				return KERN_SUCCESS;
			}
		}

		if ((flags & MEMORY_OBJECT_RESPECT_CACHE) &&
		    (object->ref_count == 1)) {
			if (original_object) {
				object->named = FALSE;
			}
			vm_object_unlock(object);
			                                                   
			                                                
			vm_object_deallocate(object);
			return KERN_SUCCESS;
		}
		shadow = object->pageout?VM_OBJECT_NULL:object->shadow;

		if (object->ref_count == 1) {
			if (vm_object_terminate(object) != KERN_SUCCESS) {
				if (original_object) {
					return KERN_FAILURE;
				} else {
					return KERN_SUCCESS;
				}
			}
			if (shadow != VM_OBJECT_NULL) {
				original_object = FALSE;
				object = shadow;
				continue;
			}
			return KERN_SUCCESS;
		} else {
			vm_object_lock_assert_exclusive(object);
			object->ref_count--;
			assert(object->ref_count > 0);
			if (original_object) {
				object->named = FALSE;
			}
			vm_object_unlock(object);
			return KERN_SUCCESS;
		}
	}
	              
	assert(0);
	return KERN_FAILURE;
}


__private_extern__ kern_return_t
vm_object_lock_request(
	vm_object_t                     object,
	vm_object_offset_t              offset,
	vm_object_size_t                size,
	memory_object_return_t          should_return,
	int                             flags,
	vm_prot_t                       prot)
{
	__unused boolean_t      should_flush;

	should_flush = flags & MEMORY_OBJECT_DATA_FLUSH;

	  
                              
    
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if ((prot & ~VM_PROT_ALL) != 0 && prot != VM_PROT_NO_CHANGE) {
		return KERN_INVALID_ARGUMENT;
	}

	  
              
                                                                       
                                                          
    
	size = vm_object_round_page(offset + size) - vm_object_trunc_page(offset);
	offset = vm_object_trunc_page(offset);

	  
                                                      
                                                            
    
	vm_object_lock(object);
	vm_object_paging_begin(object);

	(void)vm_object_update(object,
	    offset, size, NULL, NULL, should_return, flags, prot);

	vm_object_paging_end(object);
	vm_object_unlock(object);

	return KERN_SUCCESS;
}

  
                                                                             
                                                                             
                                                                           
                                                                          
                                                                           
                                                                          
                                             
  
                                                    
                                            
   
uint64_t
vm_object_purge(vm_object_t object, int flags)
{
	unsigned int    object_page_count = 0, pgcount = 0;
	uint64_t        total_purged_pgcount = 0;
	boolean_t       skipped_object = FALSE;

	vm_object_lock_assert_exclusive(object);

	if (object->purgable == VM_PURGABLE_DENY) {
		return 0;
	}

	assert(object->copy == VM_OBJECT_NULL);
	assert(object->copy_strategy == MEMORY_OBJECT_COPY_NONE);

	  
                                                                   
                                                                 
                                                                  
                       
   
                                                                    
                                                                     
                        
                                                          
                                                                  
                                                                  
                                                            
                                  
    
	if (object->purgable == VM_PURGABLE_VOLATILE) {
		unsigned int delta;
		assert(object->resident_page_count >=
		    object->wired_page_count);
		delta = (object->resident_page_count -
		    object->wired_page_count);
		if (delta != 0) {
			assert(vm_page_purgeable_count >=
			    delta);
			OSAddAtomic(-delta,
			    (SInt32 *)&vm_page_purgeable_count);
		}
		if (object->wired_page_count != 0) {
			assert(vm_page_purgeable_wired_count >=
			    object->wired_page_count);
			OSAddAtomic(-object->wired_page_count,
			    (SInt32 *)&vm_page_purgeable_wired_count);
		}
		object->purgable = VM_PURGABLE_EMPTY;
	}
	assert(object->purgable == VM_PURGABLE_EMPTY);

	object_page_count = object->resident_page_count;

	vm_object_reap_pages(object, REAP_PURGEABLE);

	if (object->resident_page_count >= object_page_count) {
		total_purged_pgcount = 0;
	} else {
		total_purged_pgcount = object_page_count - object->resident_page_count;
	}

	if (object->pager != NULL) {
		assert(VM_CONFIG_COMPRESSOR_IS_PRESENT);

		if (object->activity_in_progress == 0 &&
		    object->paging_in_progress == 0) {
			  
                                                  
                           
     
                                                          
                                                        
                                                         
                                       
      
			pgcount = vm_compressor_pager_get_count(object->pager);
			if (pgcount) {
				pgcount = vm_compressor_pager_reap_pages(object->pager, flags);
				vm_compressor_pager_count(object->pager,
				    -pgcount,
				    FALSE,                                   
				    object);
				vm_object_owner_compressed_update(object,
				    -pgcount);
			}
			if (!(flags & C_DONT_BLOCK)) {
				assert(vm_compressor_pager_get_count(object->pager)
				    == 0);
			}
		} else {
			  
                                                      
                                                   
                                                      
                                                   
              
     
                                                       
                                                     
                                                  
                                                  
                                 
     
                                                  
                                               
                                
      
			skipped_object = TRUE;
		}
	}

	vm_object_lock_assert_exclusive(object);

	total_purged_pgcount += pgcount;

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (MACHDBG_CODE(DBG_MACH_VM, OBJECT_PURGE_ONE)),
	    VM_KERNEL_UNSLIDE_OR_PERM(object),                                      
	    object_page_count,
	    total_purged_pgcount,
	    skipped_object,
	    0);

	return total_purged_pgcount;
}


  
                                                                                 
                                                                            
                                                                           
                                                                              
                                                                         
                                                                             
                                                                             
                                                                           
                            
  
                                                                            
                                                                         
                                                                      
                                                                            
                                                                            
                                                                             
                                                                             
                                                                             
                                                                         
                                                                           
                                                                          
                                                      
  
                                                                             
                                                                              
                                                                            
                                                                             
                                                            
                                                                             
                                                                        
                                                                           
                                                                      
                                                                               
                                                                             
                       
  
                                                                             
                                                                              
                                                                            
                                                         
  
                                                                            
                                                                           
                                                                             
                                                                          
                                                                           
            
  
                                                                   
                                                                          
                                                                            
                                                                            
                                             
  
                                                                    
                                                                             
                                                                        
                                                                    
                                                                         
                                                                       
                   
  
                                                                            
                                                                 
                                                           
                                                                          
                                                                         
                                                                            
                                                                           
                  
   
  
                             
   
kern_return_t
vm_object_purgable_control(
	vm_object_t     object,
	vm_purgable_t   control,
	int             *state)
{
	int             old_state;
	int             new_state;

	if (object == VM_OBJECT_NULL) {
		  
                                                             
     
		return KERN_INVALID_ARGUMENT;
	}

	vm_object_lock_assert_exclusive(object);

	  
                                              
    
	old_state = object->purgable;
	if (old_state == VM_PURGABLE_DENY) {
		return KERN_INVALID_ARGUMENT;
	}

	                                                               
	assert(object->copy == VM_OBJECT_NULL);
	assert(object->copy_strategy == MEMORY_OBJECT_COPY_NONE);

	  
                                  
    
	if (control == VM_PURGABLE_GET_STATE) {
		*state = old_state;
		return KERN_SUCCESS;
	}

	if (control == VM_PURGABLE_SET_STATE &&
	    object->purgeable_only_by_kernel) {
		return KERN_PROTECTION_FAILURE;
	}

	if (control != VM_PURGABLE_SET_STATE &&
	    control != VM_PURGABLE_SET_STATE_FROM_KERNEL) {
		return KERN_INVALID_ARGUMENT;
	}

	if ((*state) & VM_PURGABLE_DEBUG_EMPTY) {
		object->volatile_empty = TRUE;
	}
	if ((*state) & VM_PURGABLE_DEBUG_FAULT) {
		object->volatile_fault = TRUE;
	}

	new_state = *state & VM_PURGABLE_STATE_MASK;
	if (new_state == VM_PURGABLE_VOLATILE) {
		if (old_state == VM_PURGABLE_EMPTY) {
			                                         
			new_state = VM_PURGABLE_EMPTY;
		}
		if (object->volatile_empty) {
			                                          
			new_state = VM_PURGABLE_EMPTY;
		}
	}

	switch (new_state) {
	case VM_PURGABLE_DENY:
		  
                                                             
                 
     
		return KERN_INVALID_ARGUMENT;
	case VM_PURGABLE_NONVOLATILE:
		object->purgable = new_state;

		if (old_state == VM_PURGABLE_VOLATILE) {
			unsigned int delta;

			assert(object->resident_page_count >=
			    object->wired_page_count);
			delta = (object->resident_page_count -
			    object->wired_page_count);

			assert(vm_page_purgeable_count >= delta);

			if (delta != 0) {
				OSAddAtomic(-delta,
				    (SInt32 *)&vm_page_purgeable_count);
			}
			if (object->wired_page_count != 0) {
				assert(vm_page_purgeable_wired_count >=
				    object->wired_page_count);
				OSAddAtomic(-object->wired_page_count,
				    (SInt32 *)&vm_page_purgeable_wired_count);
			}

			vm_page_lock_queues();

			                                 
			assert(object->objq.next != NULL &&
			    object->objq.prev != NULL);
			purgeable_q_t queue;

			  
                                                
                           
      
			queue = vm_purgeable_object_remove(object);
			assert(queue);

			if (object->purgeable_when_ripe) {
				vm_purgeable_token_delete_last(queue);
			}
			assert(queue->debug_count_objects >= 0);

			vm_page_unlock_queues();
		}
		if (old_state == VM_PURGABLE_VOLATILE ||
		    old_state == VM_PURGABLE_EMPTY) {
			  
                                                      
                           
      
			vm_purgeable_accounting(object, VM_PURGABLE_VOLATILE);
		}

		break;

	case VM_PURGABLE_VOLATILE:
		if (object->volatile_fault) {
			vm_page_t       p;
			int             refmod;

			vm_page_queue_iterate(&object->memq, p, vmp_listq) {
				if (p->vmp_busy ||
				    VM_PAGE_WIRED(p) ||
				    p->vmp_fictitious) {
					continue;
				}
				refmod = pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(p));
				if ((refmod & VM_MEM_MODIFIED) &&
				    !p->vmp_dirty) {
					SET_PAGE_DIRTY(p, FALSE);
				}
			}
		}

		assert(old_state != VM_PURGABLE_EMPTY);

		purgeable_q_t queue;

		                            
		if ((*state & VM_PURGABLE_ORDERING_MASK) == VM_PURGABLE_ORDERING_OBSOLETE) {
			queue = &purgeable_queues[PURGEABLE_Q_TYPE_OBSOLETE];
		} else {
			if ((*state & VM_PURGABLE_BEHAVIOR_MASK) == VM_PURGABLE_BEHAVIOR_FIFO) {
				queue = &purgeable_queues[PURGEABLE_Q_TYPE_FIFO];
			} else {
				queue = &purgeable_queues[PURGEABLE_Q_TYPE_LIFO];
			}
		}

		if (old_state == VM_PURGABLE_NONVOLATILE ||
		    old_state == VM_PURGABLE_EMPTY) {
			unsigned int delta;

			if ((*state & VM_PURGABLE_NO_AGING_MASK) ==
			    VM_PURGABLE_NO_AGING) {
				object->purgeable_when_ripe = FALSE;
			} else {
				object->purgeable_when_ripe = TRUE;
			}

			if (object->purgeable_when_ripe) {
				kern_return_t result;

				                                       
				vm_page_lock_queues();

				result = vm_purgeable_token_add(queue);
				if (result != KERN_SUCCESS) {
					vm_page_unlock_queues();
					return result;
				}
				vm_page_unlock_queues();
			}

			assert(object->resident_page_count >=
			    object->wired_page_count);
			delta = (object->resident_page_count -
			    object->wired_page_count);

			if (delta != 0) {
				OSAddAtomic(delta,
				    &vm_page_purgeable_count);
			}
			if (object->wired_page_count != 0) {
				OSAddAtomic(object->wired_page_count,
				    &vm_page_purgeable_wired_count);
			}

			object->purgable = new_state;

			                                              
			assert(object->objq.next != NULL);
			assert(object->objq.prev != NULL);
		} else if (old_state == VM_PURGABLE_VOLATILE) {
			purgeable_q_t   old_queue;
			boolean_t       purgeable_when_ripe;

			  
                                                                       
                                                                               
                                                                               
                                                                                      
                                                                          
                                                                                            
      
			assert(object->objq.next != NULL && object->objq.prev != NULL);                                  

			old_queue = vm_purgeable_object_remove(object);
			assert(old_queue);

			if ((*state & VM_PURGABLE_NO_AGING_MASK) ==
			    VM_PURGABLE_NO_AGING) {
				purgeable_when_ripe = FALSE;
			} else {
				purgeable_when_ripe = TRUE;
			}

			if (old_queue != queue ||
			    (purgeable_when_ripe !=
			    object->purgeable_when_ripe)) {
				kern_return_t result;

				                                         
				vm_page_lock_queues();
				if (object->purgeable_when_ripe) {
					vm_purgeable_token_delete_last(old_queue);
				}
				object->purgeable_when_ripe = purgeable_when_ripe;
				if (object->purgeable_when_ripe) {
					result = vm_purgeable_token_add(queue);
					assert(result == KERN_SUCCESS);                                                           
				}
				vm_page_unlock_queues();
			}
		}
		;
		vm_purgeable_object_add(object, queue, (*state & VM_VOLATILE_GROUP_MASK) >> VM_VOLATILE_GROUP_SHIFT );
		if (old_state == VM_PURGABLE_NONVOLATILE) {
			vm_purgeable_accounting(object,
			    VM_PURGABLE_NONVOLATILE);
		}

		assert(queue->debug_count_objects >= 0);

		break;


	case VM_PURGABLE_EMPTY:
		if (object->volatile_fault) {
			vm_page_t       p;
			int             refmod;

			vm_page_queue_iterate(&object->memq, p, vmp_listq) {
				if (p->vmp_busy ||
				    VM_PAGE_WIRED(p) ||
				    p->vmp_fictitious) {
					continue;
				}
				refmod = pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(p));
				if ((refmod & VM_MEM_MODIFIED) &&
				    !p->vmp_dirty) {
					SET_PAGE_DIRTY(p, FALSE);
				}
			}
		}

		if (old_state == VM_PURGABLE_VOLATILE) {
			purgeable_q_t old_queue;

			                                 
			assert(object->objq.next != NULL &&
			    object->objq.prev != NULL);

			old_queue = vm_purgeable_object_remove(object);
			assert(old_queue);
			if (object->purgeable_when_ripe) {
				vm_page_lock_queues();
				vm_purgeable_token_delete_first(old_queue);
				vm_page_unlock_queues();
			}
		}

		if (old_state == VM_PURGABLE_NONVOLATILE) {
			  
                                                      
                                                    
                 
      
			vm_purgeable_accounting(object,
			    VM_PURGABLE_NONVOLATILE);
			  
                                                       
                                                   
                                       
                                
      
			object->purgable = VM_PURGABLE_EMPTY;
		}

		(void) vm_object_purge(object, 0);
		assert(object->purgable == VM_PURGABLE_EMPTY);

		break;
	}

	*state = old_state;

	vm_object_lock_assert_exclusive(object);

	return KERN_SUCCESS;
}

kern_return_t
vm_object_get_page_counts(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	unsigned int            *resident_page_count,
	unsigned int            *dirty_page_count)
{
	kern_return_t           kr = KERN_SUCCESS;
	boolean_t               count_dirty_pages = FALSE;
	vm_page_t               p = VM_PAGE_NULL;
	unsigned int            local_resident_count = 0;
	unsigned int            local_dirty_count = 0;
	vm_object_offset_t      cur_offset = 0;
	vm_object_offset_t      end_offset = 0;

	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}


	cur_offset = offset;

	end_offset = offset + size;

	vm_object_lock_assert_exclusive(object);

	if (dirty_page_count != NULL) {
		count_dirty_pages = TRUE;
	}

	if (resident_page_count != NULL && count_dirty_pages == FALSE) {
		  
                    
                                                 
                                                           
     
		if (offset == 0 && (object->vo_size == size)) {
			*resident_page_count = object->resident_page_count;
			goto out;
		}
	}

	if (object->resident_page_count <= (size >> PAGE_SHIFT)) {
		vm_page_queue_iterate(&object->memq, p, vmp_listq) {
			if (p->vmp_offset >= cur_offset && p->vmp_offset < end_offset) {
				local_resident_count++;

				if (count_dirty_pages) {
					if (p->vmp_dirty || (p->vmp_wpmapped && pmap_is_modified(VM_PAGE_GET_PHYS_PAGE(p)))) {
						local_dirty_count++;
					}
				}
			}
		}
	} else {
		for (cur_offset = offset; cur_offset < end_offset; cur_offset += PAGE_SIZE_64) {
			p = vm_page_lookup(object, cur_offset);

			if (p != VM_PAGE_NULL) {
				local_resident_count++;

				if (count_dirty_pages) {
					if (p->vmp_dirty || (p->vmp_wpmapped && pmap_is_modified(VM_PAGE_GET_PHYS_PAGE(p)))) {
						local_dirty_count++;
					}
				}
			}
		}
	}

	if (resident_page_count != NULL) {
		*resident_page_count = local_resident_count;
	}

	if (dirty_page_count != NULL) {
		*dirty_page_count = local_dirty_count;
	}

out:
	return kr;
}


  
                       
  
                                              
   
#ifdef vm_object_reference
#undef vm_object_reference
#endif
__private_extern__ void
vm_object_reference(
	vm_object_t     object)
{
	if (object == VM_OBJECT_NULL) {
		return;
	}

	vm_object_lock(object);
	assert(object->ref_count > 0);
	vm_object_reference_locked(object);
	vm_object_unlock(object);
}

  
                      
  
                                                                   
                       
                                                                            
                                                        
  
                                               
   
unsigned int vm_object_transpose_count = 0;
kern_return_t
vm_object_transpose(
	vm_object_t             object1,
	vm_object_t             object2,
	vm_object_size_t        transpose_size)
{
	vm_object_t             tmp_object;
	kern_return_t           retval;
	boolean_t               object1_locked, object2_locked;
	vm_page_t               page;
	vm_object_offset_t      page_offset;

	tmp_object = VM_OBJECT_NULL;
	object1_locked = FALSE; object2_locked = FALSE;

	if (object1 == object2 ||
	    object1 == VM_OBJECT_NULL ||
	    object2 == VM_OBJECT_NULL) {
		  
                                              
                                                
     
		retval = KERN_INVALID_VALUE;
		goto done;
	}

	  
                                                        
                                                      
                    
    
	if (object1 > object2) {
		tmp_object = object1;
		object1 = object2;
		object2 = tmp_object;
	}

	  
                                                             
                                     
    
	tmp_object = vm_object_allocate(transpose_size);
	vm_object_lock(tmp_object);
	tmp_object->can_persist = FALSE;


	  
                                      
    
	vm_object_lock(object1);
	object1_locked = TRUE;
	if (!object1->alive || object1->terminating ||
	    object1->copy || object1->shadow || object1->shadowed ||
	    object1->purgable != VM_PURGABLE_DENY) {
		  
                                                     
     
		retval = KERN_INVALID_VALUE;
		goto done;
	}
	  
                                                           
                                                              
                                                                       
                                                                    
                                                                    
                                                    
   
                                                                   
                                                                   
                                                                   
                                                                      
    
	vm_object_paging_only_wait(object1, THREAD_UNINT);

	  
                                       
    
	vm_object_lock(object2);
	object2_locked = TRUE;
	if (!object2->alive || object2->terminating ||
	    object2->copy || object2->shadow || object2->shadowed ||
	    object2->purgable != VM_PURGABLE_DENY) {
		retval = KERN_INVALID_VALUE;
		goto done;
	}
	vm_object_paging_only_wait(object2, THREAD_UNINT);


	if (object1->vo_size != object2->vo_size ||
	    object1->vo_size != transpose_size) {
		  
                                                        
                                                         
                                             
                                                        
                                                       
                 
     
		retval = KERN_INVALID_VALUE;
		goto done;
	}


	  
                                          
                                                                
    
	if (object1->phys_contiguous || vm_page_queue_empty(&object1->memq)) {
		  
                                             
                                                    
                            
     
		while (!vm_page_queue_empty(&object2->memq)) {
			page = (vm_page_t) vm_page_queue_first(&object2->memq);
			vm_page_rename(page, object1, page->vmp_offset);
		}
		assert(vm_page_queue_empty(&object2->memq));
	} else if (object2->phys_contiguous || vm_page_queue_empty(&object2->memq)) {
		  
                                             
                                                    
                            
     
		while (!vm_page_queue_empty(&object1->memq)) {
			page = (vm_page_t) vm_page_queue_first(&object1->memq);
			vm_page_rename(page, object2, page->vmp_offset);
		}
		assert(vm_page_queue_empty(&object1->memq));
	} else {
		                                            
		while (!vm_page_queue_empty(&object1->memq)) {
			page = (vm_page_t) vm_page_queue_first(&object1->memq);
			page_offset = page->vmp_offset;
			vm_page_remove(page, TRUE);
			page->vmp_offset = page_offset;
			vm_page_queue_enter(&tmp_object->memq, page, vmp_listq);
		}
		assert(vm_page_queue_empty(&object1->memq));
		                                         
		while (!vm_page_queue_empty(&object2->memq)) {
			page = (vm_page_t) vm_page_queue_first(&object2->memq);
			vm_page_rename(page, object1, page->vmp_offset);
		}
		assert(vm_page_queue_empty(&object2->memq));
		                                            
		while (!vm_page_queue_empty(&tmp_object->memq)) {
			page = (vm_page_t) vm_page_queue_first(&tmp_object->memq);
			vm_page_queue_remove(&tmp_object->memq, page, vmp_listq);
			vm_page_insert(page, object2, page->vmp_offset);
		}
		assert(vm_page_queue_empty(&tmp_object->memq));
	}

#define __TRANSPOSE_FIELD(field)                                \
MACRO_BEGIN                                                     \
	tmp_object->field = object1->field;                     \
	object1->field = object2->field;                        \
	object2->field = tmp_object->field;                     \
MACRO_END

	                                                  
	                                
	assert(object1->vo_size == object2->vo_size);
	                                                          
	                                                       
	assert(object1->ref_count >= 1);
	assert(object2->ref_count >= 1);
	                                                                    
	                                                                 
#if !VM_TAG_ACTIVE_UPDATE
	                                                               
#endif                             
	                                                                    
	                               
	assert(!object1->copy);
	assert(!object2->copy);
	                                 
	assert(!object1->shadow);
	assert(!object2->shadow);
	__TRANSPOSE_FIELD(vo_shadow_offset);                                      
	__TRANSPOSE_FIELD(pager);
	__TRANSPOSE_FIELD(paging_offset);
	__TRANSPOSE_FIELD(pager_control);
	                                                                
	if (object1->pager_control != MEMORY_OBJECT_CONTROL_NULL) {
		memory_object_control_collapse(&object1->pager_control,
		    object1);
	}
	if (object2->pager_control != MEMORY_OBJECT_CONTROL_NULL) {
		memory_object_control_collapse(&object2->pager_control,
		    object2);
	}
	__TRANSPOSE_FIELD(copy_strategy);
	                                                                
	assert(!object1->paging_in_progress);
	assert(!object2->paging_in_progress);
	assert(object1->activity_in_progress);
	assert(object2->activity_in_progress);
	                                                        
	__TRANSPOSE_FIELD(pager_created);
	__TRANSPOSE_FIELD(pager_initialized);
	__TRANSPOSE_FIELD(pager_ready);
	__TRANSPOSE_FIELD(pager_trusted);
	__TRANSPOSE_FIELD(can_persist);
	__TRANSPOSE_FIELD(internal);
	__TRANSPOSE_FIELD(private);
	__TRANSPOSE_FIELD(pageout);
	                           
	assert(object1->alive);
	assert(object2->alive);
	                                         
	assert(object1->purgable == VM_PURGABLE_DENY);
	assert(object2->purgable == VM_PURGABLE_DENY);
	                                                          
	__TRANSPOSE_FIELD(purgeable_when_ripe);
	__TRANSPOSE_FIELD(true_share);
	                                     
	assert(!object1->terminating);
	assert(!object2->terminating);
	                                          
	if (object1->named && !object2->named) {
		assert(object1->ref_count >= 2);
		assert(object2->ref_count >= 1);
		object1->ref_count--;
		object2->ref_count++;
	} else if (!object1->named && object2->named) {
		assert(object1->ref_count >= 1);
		assert(object2->ref_count >= 2);
		object1->ref_count++;
		object2->ref_count--;
	}
	__TRANSPOSE_FIELD(named);
	                                                            
	__TRANSPOSE_FIELD(phys_contiguous);
	__TRANSPOSE_FIELD(nophyscache);
	                                                    
	object1->cached_list.next = (queue_entry_t) object2;
	object2->cached_list.next = (queue_entry_t) object1;
	                                       
	assert(object1->cached_list.prev == NULL);
	assert(object2->cached_list.prev == NULL);
	__TRANSPOSE_FIELD(last_alloc);
	__TRANSPOSE_FIELD(sequential);
	__TRANSPOSE_FIELD(pages_created);
	__TRANSPOSE_FIELD(pages_used);
	__TRANSPOSE_FIELD(scan_collisions);
	__TRANSPOSE_FIELD(cow_hint);
	__TRANSPOSE_FIELD(wimg_bits);
	__TRANSPOSE_FIELD(set_cache_attr);
	__TRANSPOSE_FIELD(code_signed);
	object1->transposed = TRUE;
	object2->transposed = TRUE;
	__TRANSPOSE_FIELD(mapping_in_progress);
	__TRANSPOSE_FIELD(volatile_empty);
	__TRANSPOSE_FIELD(volatile_fault);
	__TRANSPOSE_FIELD(all_reusable);
	assert(object1->blocked_access);
	assert(object2->blocked_access);
	__TRANSPOSE_FIELD(set_cache_attr);
	assert(!object1->object_is_shared_cache);
	assert(!object2->object_is_shared_cache);
	                                                           
	assert(!object1->io_tracking);
	assert(!object2->io_tracking);
#if VM_OBJECT_ACCESS_TRACKING
	assert(!object1->access_tracking);
	assert(!object2->access_tracking);
#endif                                
	__TRANSPOSE_FIELD(no_tag_update);
#if CONFIG_SECLUDED_MEMORY
	assert(!object1->eligible_for_secluded);
	assert(!object2->eligible_for_secluded);
	assert(!object1->can_grab_secluded);
	assert(!object2->can_grab_secluded);
#else                             
	assert(object1->__object3_unused_bits == 0);
	assert(object2->__object3_unused_bits == 0);
#endif                             
#if UPL_DEBUG
	                                                                        
#endif
	assert((object1->purgable == VM_PURGABLE_DENY) || (object1->objq.next == NULL));
	assert((object1->purgable == VM_PURGABLE_DENY) || (object1->objq.prev == NULL));
	assert((object2->purgable == VM_PURGABLE_DENY) || (object2->objq.next == NULL));
	assert((object2->purgable == VM_PURGABLE_DENY) || (object2->objq.prev == NULL));

#undef __TRANSPOSE_FIELD

	retval = KERN_SUCCESS;

done:
	  
            
    
	if (tmp_object != VM_OBJECT_NULL) {
		vm_object_unlock(tmp_object);
		  
                                                
                               
     
		_vm_object_allocate(transpose_size, tmp_object);
		vm_object_deallocate(tmp_object);
		tmp_object = VM_OBJECT_NULL;
	}

	if (object1_locked) {
		vm_object_unlock(object1);
		object1_locked = FALSE;
	}
	if (object2_locked) {
		vm_object_unlock(object2);
		object2_locked = FALSE;
	}

	vm_object_transpose_count++;

	return retval;
}


  
                              
  
                                                                 
  
                                            
                                                 
                                                  
                                         
  
                                                         
  
   
extern int speculative_reads_disabled;

  
                                                                                     
                                                                                                 
                                                                                  
                                                                                   
                                  
   
#if !XNU_TARGET_OS_OSX
unsigned int preheat_max_bytes = (1024 * 512);
#else                         
unsigned int preheat_max_bytes = MAX_UPL_TRANSFER_BYTES;
#endif                         
unsigned int preheat_min_bytes = (1024 * 32);


__private_extern__ void
vm_object_cluster_size(vm_object_t object, vm_object_offset_t *start,
    vm_size_t *length, vm_object_fault_info_t fault_info, uint32_t *io_streaming)
{
	vm_size_t               pre_heat_size;
	vm_size_t               tail_size;
	vm_size_t               head_size;
	vm_size_t               max_length;
	vm_size_t               cluster_size;
	vm_object_offset_t      object_size;
	vm_object_offset_t      orig_start;
	vm_object_offset_t      target_start;
	vm_object_offset_t      offset;
	vm_behavior_t           behavior;
	boolean_t               look_behind = TRUE;
	boolean_t               look_ahead  = TRUE;
	boolean_t               isSSD = FALSE;
	uint32_t                throttle_limit;
	int                     sequential_run;
	int                     sequential_behavior = VM_BEHAVIOR_SEQUENTIAL;
	vm_size_t               max_ph_size;
	vm_size_t               min_ph_size;

	assert( !(*length & PAGE_MASK));
	assert( !(*start & PAGE_MASK_64));

	  
                                             
    
	max_length = *length;
	  
                                                  
                                                
                
    
	*length = PAGE_SIZE;
	*io_streaming = 0;

	if (speculative_reads_disabled || fault_info == NULL) {
		  
                                         
     
		return;
	}
	orig_start = *start;
	target_start = orig_start;
	cluster_size = round_page(fault_info->cluster_size);
	behavior = fault_info->behavior;

	vm_object_lock(object);

	if (object->pager == MEMORY_OBJECT_NULL) {
		goto out;                                                              
	}
	vnode_pager_get_isSSD(object->pager, &isSSD);

	min_ph_size = round_page(preheat_min_bytes);
	max_ph_size = round_page(preheat_max_bytes);

#if XNU_TARGET_OS_OSX
	if (isSSD) {
		min_ph_size /= 2;
		max_ph_size /= 8;

		if (min_ph_size & PAGE_MASK_64) {
			min_ph_size = trunc_page(min_ph_size);
		}

		if (max_ph_size & PAGE_MASK_64) {
			max_ph_size = trunc_page(max_ph_size);
		}
	}
#endif                        

	if (min_ph_size < PAGE_SIZE) {
		min_ph_size = PAGE_SIZE;
	}

	if (max_ph_size < PAGE_SIZE) {
		max_ph_size = PAGE_SIZE;
	} else if (max_ph_size > MAX_UPL_TRANSFER_BYTES) {
		max_ph_size = MAX_UPL_TRANSFER_BYTES;
	}

	if (max_length > max_ph_size) {
		max_length = max_ph_size;
	}

	if (max_length <= PAGE_SIZE) {
		goto out;
	}

	if (object->internal) {
		object_size = object->vo_size;
	} else {
		vnode_pager_get_object_size(object->pager, &object_size);
	}

	object_size = round_page_64(object_size);

	if (orig_start >= object_size) {
		  
                                     
                                     
                    
     
		goto out;
	}
	if (object->pages_used > object->pages_created) {
		  
                                          
             
     
		object->pages_used = object->pages_created = 0;
	}
	if ((sequential_run = object->sequential)) {
		if (sequential_run < 0) {
			sequential_behavior = VM_BEHAVIOR_RSEQNTL;
			sequential_run = 0 - sequential_run;
		} else {
			sequential_behavior = VM_BEHAVIOR_SEQUENTIAL;
		}
	}
	switch (behavior) {
	default:
		behavior = VM_BEHAVIOR_DEFAULT;
		OS_FALLTHROUGH;

	case VM_BEHAVIOR_DEFAULT:
		if (object->internal && fault_info->user_tag == VM_MEMORY_STACK) {
			goto out;
		}

		if (sequential_run >= (3 * PAGE_SIZE)) {
			pre_heat_size = sequential_run + PAGE_SIZE;

			if (sequential_behavior == VM_BEHAVIOR_SEQUENTIAL) {
				look_behind = FALSE;
			} else {
				look_ahead = FALSE;
			}

			*io_streaming = 1;
		} else {
			if (object->pages_created < (20 * (min_ph_size >> PAGE_SHIFT))) {
				  
                     
       
				pre_heat_size = min_ph_size;
			} else {
				  
                                                                  
                                                              
                                                         
                                              
       
				pre_heat_size = (max_length * (uint64_t)object->pages_used) / object->pages_created;

				if (pre_heat_size < min_ph_size) {
					pre_heat_size = min_ph_size;
				} else {
					pre_heat_size = round_page(pre_heat_size);
				}
			}
		}
		break;

	case VM_BEHAVIOR_RANDOM:
		if ((pre_heat_size = cluster_size) <= PAGE_SIZE) {
			goto out;
		}
		break;

	case VM_BEHAVIOR_SEQUENTIAL:
		if ((pre_heat_size = cluster_size) == 0) {
			pre_heat_size = sequential_run + PAGE_SIZE;
		}
		look_behind = FALSE;
		*io_streaming = 1;

		break;

	case VM_BEHAVIOR_RSEQNTL:
		if ((pre_heat_size = cluster_size) == 0) {
			pre_heat_size = sequential_run + PAGE_SIZE;
		}
		look_ahead = FALSE;
		*io_streaming = 1;

		break;
	}
	throttle_limit = (uint32_t) max_length;
	assert(throttle_limit == max_length);

	if (vnode_pager_get_throttle_io_limit(object->pager, &throttle_limit) == KERN_SUCCESS) {
		if (max_length > throttle_limit) {
			max_length = throttle_limit;
		}
	}
	if (pre_heat_size > max_length) {
		pre_heat_size = max_length;
	}

	if (behavior == VM_BEHAVIOR_DEFAULT && (pre_heat_size > min_ph_size)) {
		unsigned int consider_free = vm_page_free_count + vm_page_cleaned_count;

		if (consider_free < vm_page_throttle_limit) {
			pre_heat_size = trunc_page(pre_heat_size / 16);
		} else if (consider_free < vm_page_free_target) {
			pre_heat_size = trunc_page(pre_heat_size / 4);
		}

		if (pre_heat_size < min_ph_size) {
			pre_heat_size = min_ph_size;
		}
	}
	if (look_ahead == TRUE) {
		if (look_behind == TRUE) {
			  
                                                  
                                                     
                                                   
                                                            
                                                        
                                                                
                                        
      
			head_size = trunc_page(pre_heat_size / 2);

			if (target_start > head_size) {
				target_start -= head_size;
			} else {
				target_start = 0;
			}

			  
                                                                  
                                                                  
                                                                    
                                               
      
		}
		if ((target_start + pre_heat_size) > object_size) {
			pre_heat_size = (vm_size_t)(round_page_64(object_size - target_start));
		}
		  
                                                                          
                                                                                
                       
     
		assert((vm_size_t)(orig_start - target_start) == (orig_start - target_start));
		tail_size = pre_heat_size - (vm_size_t)(orig_start - target_start) - PAGE_SIZE;
	} else {
		if (pre_heat_size > target_start) {
			  
                                                      
                                                        
                                                
      
			pre_heat_size = (vm_size_t) target_start;
		}
		tail_size = 0;
	}
	assert( !(target_start & PAGE_MASK_64));
	assert( !(pre_heat_size & PAGE_MASK_64));

	if (pre_heat_size <= PAGE_SIZE) {
		goto out;
	}

	if (look_behind == TRUE) {
		  
                                                 
                                                
                                                 
                               
     
		head_size = pre_heat_size - tail_size - PAGE_SIZE;

		for (offset = orig_start - PAGE_SIZE_64; head_size; offset -= PAGE_SIZE_64, head_size -= PAGE_SIZE) {
			  
                                        
      
			if (offset < fault_info->lo_offset) {
				break;
			}
			  
                                                           
                                                                         
      
			if (VM_COMPRESSOR_PAGER_STATE_GET(object, offset) == VM_EXTERNAL_STATE_ABSENT) {
				break;
			}
			if (vm_page_lookup(object, offset) != VM_PAGE_NULL) {
				  
                                  
       
				break;
			}
			*start = offset;
			*length += PAGE_SIZE;
		}
	}
	if (look_ahead == TRUE) {
		for (offset = orig_start + PAGE_SIZE_64; tail_size; offset += PAGE_SIZE_64, tail_size -= PAGE_SIZE) {
			  
                                         
      
			if (offset >= fault_info->hi_offset) {
				break;
			}
			assert(offset < object_size);

			  
                                                           
                                                                         
      
			if (VM_COMPRESSOR_PAGER_STATE_GET(object, offset) == VM_EXTERNAL_STATE_ABSENT) {
				break;
			}
			if (vm_page_lookup(object, offset) != VM_PAGE_NULL) {
				  
                                  
       
				break;
			}
			*length += PAGE_SIZE;
		}
	}
out:
	if (*length > max_length) {
		*length = max_length;
	}

	vm_object_unlock(object);

	DTRACE_VM1(clustersize, vm_size_t, *length);
}


  
                                                                         
                                                                        
   

kern_return_t
vm_object_page_op(
	vm_object_t             object,
	vm_object_offset_t      offset,
	int                     ops,
	ppnum_t                 *phys_entry,
	int                     *flags)
{
	vm_page_t               dst_page;

	vm_object_lock(object);

	if (ops & UPL_POP_PHYSICAL) {
		if (object->phys_contiguous) {
			if (phys_entry) {
				*phys_entry = (ppnum_t)
				    (object->vo_shadow_offset >> PAGE_SHIFT);
			}
			vm_object_unlock(object);
			return KERN_SUCCESS;
		} else {
			vm_object_unlock(object);
			return KERN_INVALID_OBJECT;
		}
	}
	if (object->phys_contiguous) {
		vm_object_unlock(object);
		return KERN_INVALID_OBJECT;
	}

	while (TRUE) {
		if ((dst_page = vm_page_lookup(object, offset)) == VM_PAGE_NULL) {
			vm_object_unlock(object);
			return KERN_FAILURE;
		}

		                                     
		if ((dst_page->vmp_busy || dst_page->vmp_cleaning) &&
		    (((ops & UPL_POP_SET) &&
		    (ops & UPL_POP_BUSY)) || (ops & UPL_POP_DUMP))) {
			                                                    
			                  
			PAGE_SLEEP(object, dst_page, THREAD_UNINT);
			continue;
		}

		if (ops & UPL_POP_DUMP) {
			if (dst_page->vmp_pmapped == TRUE) {
				pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(dst_page));
			}

			VM_PAGE_FREE(dst_page);
			break;
		}

		if (flags) {
			*flags = 0;

			                                                     
			                    

			if (dst_page->vmp_dirty) {
				*flags |= UPL_POP_DIRTY;
			}
			if (dst_page->vmp_free_when_done) {
				*flags |= UPL_POP_PAGEOUT;
			}
			if (dst_page->vmp_precious) {
				*flags |= UPL_POP_PRECIOUS;
			}
			if (dst_page->vmp_absent) {
				*flags |= UPL_POP_ABSENT;
			}
			if (dst_page->vmp_busy) {
				*flags |= UPL_POP_BUSY;
			}
		}

		                                                               
		                                               
		if (ops & UPL_POP_SET) {
			                                                  
			                                                  
			                                                  
			                                                  
			                                                    
			                                                    
			               
			assert(dst_page->vmp_busy || (ops & UPL_POP_BUSY));
			if (ops & UPL_POP_DIRTY) {
				SET_PAGE_DIRTY(dst_page, FALSE);
			}
			if (ops & UPL_POP_PAGEOUT) {
				dst_page->vmp_free_when_done = TRUE;
			}
			if (ops & UPL_POP_PRECIOUS) {
				dst_page->vmp_precious = TRUE;
			}
			if (ops & UPL_POP_ABSENT) {
				dst_page->vmp_absent = TRUE;
			}
			if (ops & UPL_POP_BUSY) {
				dst_page->vmp_busy = TRUE;
			}
		}

		if (ops & UPL_POP_CLR) {
			assert(dst_page->vmp_busy);
			if (ops & UPL_POP_DIRTY) {
				dst_page->vmp_dirty = FALSE;
			}
			if (ops & UPL_POP_PAGEOUT) {
				dst_page->vmp_free_when_done = FALSE;
			}
			if (ops & UPL_POP_PRECIOUS) {
				dst_page->vmp_precious = FALSE;
			}
			if (ops & UPL_POP_ABSENT) {
				dst_page->vmp_absent = FALSE;
			}
			if (ops & UPL_POP_BUSY) {
				dst_page->vmp_busy = FALSE;
				PAGE_WAKEUP(dst_page);
			}
		}
		if (phys_entry) {
			  
                                                
                                    
      
			assert(dst_page->vmp_busy);
			*phys_entry = VM_PAGE_GET_PHYS_PAGE(dst_page);
		}

		break;
	}

	vm_object_unlock(object);
	return KERN_SUCCESS;
}

  
                                                         
                                                                    
                                                                            
                                                                          
                                                                           
                                                                             
                                                                                
   

kern_return_t
vm_object_range_op(
	vm_object_t             object,
	vm_object_offset_t      offset_beg,
	vm_object_offset_t      offset_end,
	int                     ops,
	uint32_t                *range)
{
	vm_object_offset_t      offset;
	vm_page_t               dst_page;

	if (offset_end - offset_beg > (uint32_t) -1) {
		                                                  
		return KERN_INVALID_ARGUMENT;
	}
	if (object->resident_page_count == 0) {
		if (range) {
			if (ops & UPL_ROP_PRESENT) {
				*range = 0;
			} else {
				*range = (uint32_t) (offset_end - offset_beg);
				assert(*range == (offset_end - offset_beg));
			}
		}
		return KERN_SUCCESS;
	}
	vm_object_lock(object);

	if (object->phys_contiguous) {
		vm_object_unlock(object);
		return KERN_INVALID_OBJECT;
	}

	offset = offset_beg & ~PAGE_MASK_64;

	while (offset < offset_end) {
		dst_page = vm_page_lookup(object, offset);
		if (dst_page != VM_PAGE_NULL) {
			if (ops & UPL_ROP_DUMP) {
				if (dst_page->vmp_busy || dst_page->vmp_cleaning) {
					  
                                        
                                  
        
					PAGE_SLEEP(object, dst_page, THREAD_UNINT);
					  
                                             
                                             
                                                  
                     
        
					continue;
				}
				if (dst_page->vmp_laundry) {
					vm_pageout_steal_laundry(dst_page, FALSE);
				}

				if (dst_page->vmp_pmapped == TRUE) {
					pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(dst_page));
				}

				VM_PAGE_FREE(dst_page);
			} else if ((ops & UPL_ROP_ABSENT)
			    && (!dst_page->vmp_absent || dst_page->vmp_busy)) {
				break;
			}
		} else if (ops & UPL_ROP_PRESENT) {
			break;
		}

		offset += PAGE_SIZE;
	}
	vm_object_unlock(object);

	if (range) {
		if (offset > offset_end) {
			offset = offset_end;
		}
		if (offset > offset_beg) {
			*range = (uint32_t) (offset - offset_beg);
			assert(*range == (offset - offset_beg));
		} else {
			*range = 0;
		}
	}
	return KERN_SUCCESS;
}

  
                                                                                        
                                                                                             
                                                                                                    
   
kern_return_t
pager_map_to_phys_contiguous(
	memory_object_control_t object,
	memory_object_offset_t  offset,
	addr64_t                base_vaddr,
	vm_size_t               size)
{
	ppnum_t page_num;
	boolean_t clobbered_private;
	kern_return_t retval;
	vm_object_t pager_object;

	page_num = pmap_find_phys(kernel_pmap, base_vaddr);

	if (!page_num) {
		retval = KERN_FAILURE;
		goto out;
	}

	pager_object = memory_object_control_to_vm_object(object);

	if (!pager_object) {
		retval = KERN_FAILURE;
		goto out;
	}

	clobbered_private = pager_object->private;
	if (pager_object->private != TRUE) {
		vm_object_lock(pager_object);
		pager_object->private = TRUE;
		vm_object_unlock(pager_object);
	}
	retval = vm_object_populate_with_private(pager_object, offset, page_num, size);

	if (retval != KERN_SUCCESS) {
		if (pager_object->private != clobbered_private) {
			vm_object_lock(pager_object);
			pager_object->private = clobbered_private;
			vm_object_unlock(pager_object);
		}
	}

out:
	return retval;
}

uint32_t scan_object_collision = 0;

void
vm_object_lock(vm_object_t object)
{
	if (object == vm_pageout_scan_wants_object) {
		scan_object_collision++;
		mutex_pause(2);
	}
	DTRACE_VM(vm_object_lock_w);
	lck_rw_lock_exclusive(&object->Lock);
#if DEVELOPMENT || DEBUG
	object->Lock_owner = current_thread();
#endif
}

boolean_t
vm_object_lock_avoid(vm_object_t object)
{
	if (object == vm_pageout_scan_wants_object) {
		scan_object_collision++;
		return TRUE;
	}
	return FALSE;
}

boolean_t
_vm_object_lock_try(vm_object_t object)
{
	boolean_t       retval;

	retval = lck_rw_try_lock_exclusive(&object->Lock);
#if DEVELOPMENT || DEBUG
	if (retval == TRUE) {
		DTRACE_VM(vm_object_lock_w);
		object->Lock_owner = current_thread();
	}
#endif
	return retval;
}

boolean_t
vm_object_lock_try(vm_object_t object)
{
	  
                                                        
    
	if (vm_object_lock_avoid(object) && ml_get_interrupts_enabled() && get_preemption_level() == 0) {
		mutex_pause(2);
	}
	return _vm_object_lock_try(object);
}

  
                             
  
                                                          
                      
   
bool
vm_object_lock_check_contended(vm_object_t object)
{
	bool contended;
	if (object == vm_pageout_scan_wants_object) {
		scan_object_collision++;
		mutex_pause(2);
	}
	DTRACE_VM(vm_object_lock_w);
	contended = lck_rw_lock_exclusive_check_contended(&object->Lock);
#if DEVELOPMENT || DEBUG
	object->Lock_owner = current_thread();
#endif
	return contended;
}

void
vm_object_lock_shared(vm_object_t object)
{
	if (vm_object_lock_avoid(object)) {
		mutex_pause(2);
	}
	DTRACE_VM(vm_object_lock_r);
	lck_rw_lock_shared(&object->Lock);
}

boolean_t
vm_object_lock_yield_shared(vm_object_t object)
{
	boolean_t retval = FALSE, force_yield = FALSE;;

	vm_object_lock_assert_shared(object);

	force_yield = vm_object_lock_avoid(object);

	retval = lck_rw_lock_yield_shared(&object->Lock, force_yield);
	if (retval) {
		DTRACE_VM(vm_object_lock_yield);
	}

	return retval;
}

boolean_t
vm_object_lock_try_shared(vm_object_t object)
{
	boolean_t retval;

	if (vm_object_lock_avoid(object)) {
		mutex_pause(2);
	}
	retval = lck_rw_try_lock_shared(&object->Lock);
	if (retval) {
		DTRACE_VM(vm_object_lock_r);
	}
	return retval;
}

boolean_t
vm_object_lock_upgrade(vm_object_t object)
{
	boolean_t       retval;

	retval = lck_rw_lock_shared_to_exclusive(&object->Lock);
#if DEVELOPMENT || DEBUG
	if (retval == TRUE) {
		DTRACE_VM(vm_object_lock_w);
		object->Lock_owner = current_thread();
	}
#endif
	return retval;
}

void
vm_object_unlock(vm_object_t object)
{
#if DEVELOPMENT || DEBUG
	if (object->Lock_owner) {
		if (object->Lock_owner != current_thread()) {
			panic("vm_object_unlock: not owner - %p\n", object);
		}
		object->Lock_owner = 0;
		DTRACE_VM(vm_object_unlock);
	}
#endif
	lck_rw_done(&object->Lock);
}


unsigned int vm_object_change_wimg_mode_count = 0;

  
                            
   
void
vm_object_change_wimg_mode(vm_object_t object, unsigned int wimg_mode)
{
	vm_page_t p;

	vm_object_lock_assert_exclusive(object);

	vm_object_paging_wait(object, THREAD_UNINT);

	vm_page_queue_iterate(&object->memq, p, vmp_listq) {
		if (!p->vmp_fictitious) {
			pmap_set_cache_attributes(VM_PAGE_GET_PHYS_PAGE(p), wimg_mode);
		}
	}
	if (wimg_mode == VM_WIMG_USE_DEFAULT) {
		object->set_cache_attr = FALSE;
	} else {
		object->set_cache_attr = TRUE;
	}

	object->wimg_bits = wimg_mode;

	vm_object_change_wimg_mode_count++;
}

#if CONFIG_FREEZE

extern struct freezer_context   freezer_context_global;

  
                                                   
                                                     
                                                   
                                                 
                                             
   

extern AbsoluteTime c_freezer_last_yield_ts;

#define MAX_FREE_BATCH  32
#define FREEZER_DUTY_CYCLE_ON_MS        5
#define FREEZER_DUTY_CYCLE_OFF_MS       5

static int c_freezer_should_yield(void);


static int
c_freezer_should_yield()
{
	AbsoluteTime    cur_time;
	uint64_t        nsecs;

	assert(c_freezer_last_yield_ts);
	clock_get_uptime(&cur_time);

	SUB_ABSOLUTETIME(&cur_time, &c_freezer_last_yield_ts);
	absolutetime_to_nanoseconds(cur_time, &nsecs);

	if (nsecs > 1000 * 1000 * FREEZER_DUTY_CYCLE_ON_MS) {
		return 1;
	}
	return 0;
}


void
vm_object_compressed_freezer_done()
{
	vm_compressor_finished_filling( &(freezer_context_global.freezer_ctx_chead));
}


uint32_t
vm_object_compressed_freezer_pageout(
	vm_object_t object, uint32_t dirty_budget)
{
	vm_page_t                       p;
	vm_page_t                       local_freeq = NULL;
	int                             local_freed = 0;
	kern_return_t                   retval = KERN_SUCCESS;
	int                             obj_resident_page_count_snapshot = 0;
	uint32_t                        paged_out_count = 0;

	assert(object != VM_OBJECT_NULL);
	assert(object->internal);

	vm_object_lock(object);

	if (!object->pager_initialized || object->pager == MEMORY_OBJECT_NULL) {
		if (!object->pager_initialized) {
			vm_object_collapse(object, (vm_object_offset_t) 0, TRUE);

			if (!object->pager_initialized) {
				vm_object_compressor_pager_create(object);
			}
		}

		if (!object->pager_initialized || object->pager == MEMORY_OBJECT_NULL) {
			vm_object_unlock(object);
			return paged_out_count;
		}
	}

	  
                                                            
                                                         
                                                                      
                                                                
   
                                                      
                                                     
                                                   
    
	if (object->paging_in_progress || object->activity_in_progress) {
		vm_object_unlock(object);
		return paged_out_count;
	}

	if (VM_CONFIG_FREEZER_SWAP_IS_ACTIVE) {
		vm_object_offset_t      curr_offset = 0;

		  
                                                 
                                                   
                                                              
     
		while (curr_offset < object->vo_size) {
			curr_offset = vm_compressor_pager_next_compressed(object->pager, curr_offset);

			if (curr_offset == (vm_object_offset_t) -1) {
				break;
			}

			retval = vm_compressor_pager_relocate(object->pager, curr_offset, &(freezer_context_global.freezer_ctx_chead));

			if (retval != KERN_SUCCESS) {
				break;
			}

			curr_offset += PAGE_SIZE_64;
		}
	}

	  
                                                                              
                                                                              
                                                                               
                                                                               
                                
   
                                                                                 
                                                                          
                                                                                
                                                        
    

	obj_resident_page_count_snapshot = object->resident_page_count;

	vm_object_activity_begin(object);

	while ((obj_resident_page_count_snapshot--) && !vm_page_queue_empty(&object->memq) && paged_out_count < dirty_budget) {
		p = (vm_page_t)vm_page_queue_first(&object->memq);

		KERNEL_DEBUG(0xe0430004 | DBG_FUNC_START, object, local_freed, 0, 0, 0);

		vm_page_lockspin_queues();

		if (p->vmp_cleaning || p->vmp_fictitious || p->vmp_busy || p->vmp_absent || p->vmp_unusual || p->vmp_error || VM_PAGE_WIRED(p)) {
			vm_page_unlock_queues();

			KERNEL_DEBUG(0xe0430004 | DBG_FUNC_END, object, local_freed, 1, 0, 0);

			vm_page_queue_remove(&object->memq, p, vmp_listq);
			vm_page_queue_enter(&object->memq, p, vmp_listq);

			continue;
		}

		if (p->vmp_pmapped == TRUE) {
			int refmod_state, pmap_flags;

			if (p->vmp_dirty || p->vmp_precious) {
				pmap_flags = PMAP_OPTIONS_COMPRESSOR;
			} else {
				pmap_flags = PMAP_OPTIONS_COMPRESSOR_IFF_MODIFIED;
			}

			refmod_state = pmap_disconnect_options(VM_PAGE_GET_PHYS_PAGE(p), pmap_flags, NULL);
			if (refmod_state & VM_MEM_MODIFIED) {
				SET_PAGE_DIRTY(p, FALSE);
			}
		}

		if (p->vmp_dirty == FALSE && p->vmp_precious == FALSE) {
			  
                                  
      
			vm_page_unlock_queues();
			VM_PAGE_FREE(p);

			KERNEL_DEBUG(0xe0430004 | DBG_FUNC_END, object, local_freed, 2, 0, 0);
			continue;
		}

		if (p->vmp_laundry) {
			vm_pageout_steal_laundry(p, TRUE);
		}

		vm_page_queues_remove(p, TRUE);

		vm_page_unlock_queues();


		  
                                                                      
                                                                            
                                                           
     

		vm_page_queue_remove(&object->memq, p, vmp_listq);
		vm_page_queue_enter(&object->memq, p, vmp_listq);

		  
                                                                                 
    
                                                                                       
     
		p->vmp_busy = TRUE;

		vm_object_activity_begin(object);

		vm_object_unlock(object);

		if (vm_pageout_compress_page(&(freezer_context_global.freezer_ctx_chead),
		    (freezer_context_global.freezer_ctx_compressor_scratch_buf),
		    p) == KERN_SUCCESS) {
			  
                                                                          
      
			p->vmp_snext = local_freeq;
			local_freeq = p;
			local_freed++;
			paged_out_count++;

			if (local_freed >= MAX_FREE_BATCH) {
				OSAddAtomic64(local_freed, &vm_pageout_vminfo.vm_pageout_compressions);

				vm_page_free_list(local_freeq, TRUE);

				local_freeq = NULL;
				local_freed = 0;
			}
			freezer_context_global.freezer_ctx_uncompressed_pages++;
		}
		KERNEL_DEBUG(0xe0430004 | DBG_FUNC_END, object, local_freed, 0, 0, 0);

		if (local_freed == 0 && c_freezer_should_yield()) {
			thread_yield_internal(FREEZER_DUTY_CYCLE_OFF_MS);
			clock_get_uptime(&c_freezer_last_yield_ts);
		}

		vm_object_lock(object);
	}

	if (local_freeq) {
		OSAddAtomic64(local_freed, &vm_pageout_vminfo.vm_pageout_compressions);

		vm_page_free_list(local_freeq, TRUE);

		local_freeq = NULL;
		local_freed = 0;
	}

	vm_object_activity_end(object);

	vm_object_unlock(object);

	if (c_freezer_should_yield()) {
		thread_yield_internal(FREEZER_DUTY_CYCLE_OFF_MS);
		clock_get_uptime(&c_freezer_last_yield_ts);
	}
	return paged_out_count;
}

#endif                    


void
vm_object_pageout(
	vm_object_t object)
{
	vm_page_t                       p, next;
	struct  vm_pageout_queue        *iq;

	if (!VM_CONFIG_COMPRESSOR_IS_PRESENT) {
		return;
	}

	iq = &vm_pageout_queue_internal;

	assert(object != VM_OBJECT_NULL );

	vm_object_lock(object);

	if (!object->internal ||
	    object->terminating ||
	    !object->alive) {
		vm_object_unlock(object);
		return;
	}

	if (!object->pager_initialized || object->pager == MEMORY_OBJECT_NULL) {
		if (!object->pager_initialized) {
			vm_object_collapse(object, (vm_object_offset_t) 0, TRUE);

			if (!object->pager_initialized) {
				vm_object_compressor_pager_create(object);
			}
		}

		if (!object->pager_initialized || object->pager == MEMORY_OBJECT_NULL) {
			vm_object_unlock(object);
			return;
		}
	}

ReScan:
	next = (vm_page_t)vm_page_queue_first(&object->memq);

	while (!vm_page_queue_end(&object->memq, (vm_page_queue_entry_t)next)) {
		p = next;
		next = (vm_page_t)vm_page_queue_next(&next->vmp_listq);

		assert(p->vmp_q_state != VM_PAGE_ON_FREE_Q);

		if ((p->vmp_q_state == VM_PAGE_ON_THROTTLED_Q) ||
		    p->vmp_cleaning ||
		    p->vmp_laundry ||
		    p->vmp_busy ||
		    p->vmp_absent ||
		    p->vmp_error ||
		    p->vmp_fictitious ||
		    VM_PAGE_WIRED(p)) {
			  
                                                        
      
			continue;
		}
		if (vm_compressor_low_on_space()) {
			break;
		}

		                                

		vm_page_lockspin_queues();

		if (VM_PAGE_Q_THROTTLED(iq)) {
			iq->pgo_draining = TRUE;

			assert_wait((event_t) (&iq->pgo_laundry + 1),
			    THREAD_INTERRUPTIBLE);
			vm_page_unlock_queues();
			vm_object_unlock(object);

			thread_block(THREAD_CONTINUE_NULL);

			vm_object_lock(object);
			goto ReScan;
		}

		assert(!p->vmp_fictitious);
		assert(!p->vmp_busy);
		assert(!p->vmp_absent);
		assert(!p->vmp_unusual);
		assert(!p->vmp_error);
		assert(!VM_PAGE_WIRED(p));
		assert(!p->vmp_cleaning);

		if (p->vmp_pmapped == TRUE) {
			int refmod_state;
			int pmap_options;

			  
                                            
                                                
      
			pmap_options =
			    PMAP_OPTIONS_COMPRESSOR_IFF_MODIFIED;
			if (p->vmp_dirty || p->vmp_precious) {
				  
                                          
                                     
                       
       
				pmap_options = PMAP_OPTIONS_COMPRESSOR;
			}
			refmod_state = pmap_disconnect_options(VM_PAGE_GET_PHYS_PAGE(p),
			    pmap_options,
			    NULL);
			if (refmod_state & VM_MEM_MODIFIED) {
				SET_PAGE_DIRTY(p, FALSE);
			}
		}

		if (!p->vmp_dirty && !p->vmp_precious) {
			vm_page_unlock_queues();
			VM_PAGE_FREE(p);
			continue;
		}
		vm_page_queues_remove(p, TRUE);

		vm_pageout_cluster(p);

		vm_page_unlock_queues();
	}
	vm_object_unlock(object);
}


#if CONFIG_IOSCHED
void
vm_page_request_reprioritize(vm_object_t o, uint64_t blkno, uint32_t len, int prio)
{
	io_reprioritize_req_t   req;
	struct vnode            *devvp = NULL;

	if (vnode_pager_get_object_devvp(o->pager, (uintptr_t *)&devvp) != KERN_SUCCESS) {
		return;
	}

	  
                                                
                                                                         
                                                                     
    
	req = (io_reprioritize_req_t)zalloc_noblock(io_reprioritize_req_zone);
	if (req == NULL) {
		return;
	}
	req->blkno = blkno;
	req->len = len;
	req->priority = prio;
	req->devvp = devvp;

	                                                   
	IO_REPRIORITIZE_LIST_LOCK();
	queue_enter(&io_reprioritize_list, req, io_reprioritize_req_t, io_reprioritize_list);
	IO_REPRIORITIZE_LIST_UNLOCK();

	                                
	IO_REPRIO_THREAD_WAKEUP();

	return;
}

void
vm_decmp_upl_reprioritize(upl_t upl, int prio)
{
	int offset;
	vm_object_t object;
	io_reprioritize_req_t   req;
	struct vnode            *devvp = NULL;
	uint64_t                blkno;
	uint32_t                len;
	upl_t                   io_upl;
	uint64_t                *io_upl_reprio_info;
	int                     io_upl_size;

	if ((upl->flags & UPL_TRACKED_BY_OBJECT) == 0 || (upl->flags & UPL_EXPEDITE_SUPPORTED) == 0) {
		return;
	}

	  
                                                                                   
                                                                                  
                                                                                         
                                                              
    


	                                                                                          
	if (!upl_try_lock(upl)) {
		return;
	}

	if (upl->decmp_io_upl == NULL) {
		                                                                                
		upl_unlock(upl);
		return;
	}

	io_upl = upl->decmp_io_upl;
	assert((io_upl->flags & UPL_DECMP_REAL_IO) != 0);
	assertf(page_aligned(io_upl->u_offset) && page_aligned(io_upl->u_size),
	    "upl %p offset 0x%llx size 0x%x\n",
	    io_upl, io_upl->u_offset, io_upl->u_size);
	io_upl_size = io_upl->u_size;
	upl_unlock(upl);

	                                
	io_upl_reprio_info = (uint64_t *)kheap_alloc(KHEAP_TEMP,
	    sizeof(uint64_t) * atop(io_upl_size), Z_WAITOK);
	if (io_upl_reprio_info == NULL) {
		return;
	}

	                                                                               
	if (!upl_try_lock(upl)) {
		goto out;
	}

	if (upl->decmp_io_upl == NULL || upl->decmp_io_upl != io_upl) {
		                                                                                
		upl_unlock(upl);
		goto out;
	}
	memcpy(io_upl_reprio_info, io_upl->upl_reprio_info,
	    sizeof(uint64_t) * atop(io_upl_size));

	                                    
	if (io_upl->flags & UPL_SHADOWED) {
		object = io_upl->map_object->shadow;
	} else {
		object = io_upl->map_object;
	}

	                                           
	if (!object || !object->pager ||
	    vnode_pager_get_object_devvp(object->pager, (uintptr_t *)&devvp) != KERN_SUCCESS) {
		upl_unlock(upl);
		goto out;
	}

	upl_unlock(upl);

	                                                               

	offset = 0;
	while (offset < io_upl_size) {
		blkno   = io_upl_reprio_info[atop(offset)] & UPL_REPRIO_INFO_MASK;
		len     = (io_upl_reprio_info[atop(offset)] >> UPL_REPRIO_INFO_SHIFT) & UPL_REPRIO_INFO_MASK;

		  
                                                                     
                                                                       
                                    
     

		if (blkno != 0 && len != 0) {
			                                                 
			req = (io_reprioritize_req_t)zalloc(io_reprioritize_req_zone);
			assert(req != NULL);
			req->blkno = blkno;
			req->len = len;
			req->priority = prio;
			req->devvp = devvp;

			                                                   
			IO_REPRIORITIZE_LIST_LOCK();
			queue_enter(&io_reprioritize_list, req, io_reprioritize_req_t, io_reprioritize_list);
			IO_REPRIORITIZE_LIST_UNLOCK();

			offset += len;
		} else {
			offset += PAGE_SIZE;
		}
	}

	                                
	IO_REPRIO_THREAD_WAKEUP();

out:
	kheap_free(KHEAP_TEMP, io_upl_reprio_info,
	    sizeof(uint64_t) * atop(io_upl_size));
}

void
vm_page_handle_prio_inversion(vm_object_t o, vm_page_t m)
{
	upl_t upl;
	upl_page_info_t *pl;
	unsigned int i, num_pages;
	int cur_tier;

	cur_tier = proc_get_effective_thread_policy(current_thread(), TASK_POLICY_IO);

	  
                                                                 
                                       
    
	queue_iterate(&o->uplq, upl, upl_t, uplq) {
		if (((upl->flags & UPL_EXPEDITE_SUPPORTED) == 0) || upl->upl_priority <= cur_tier) {
			continue;
		}
		pl = UPL_GET_INTERNAL_PAGE_LIST(upl);
		assertf(page_aligned(upl->u_offset) && page_aligned(upl->u_size),
		    "upl %p offset 0x%llx size 0x%x\n",
		    upl, upl->u_offset, upl->u_size);
		num_pages = (upl->u_size / PAGE_SIZE);

		  
                                                                         
                                            
     
		for (i = 0; i < num_pages; i++) {
			if (UPL_PAGE_PRESENT(pl, i) && VM_PAGE_GET_PHYS_PAGE(m) == pl[i].phys_addr) {
				if ((upl->flags & UPL_DECMP_REQ) && upl->decmp_io_upl) {
					KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_PAGE_EXPEDITE)) | DBG_FUNC_NONE, VM_KERNEL_UNSLIDE_OR_PERM(upl->upl_creator), VM_KERNEL_UNSLIDE_OR_PERM(m),
					    VM_KERNEL_UNSLIDE_OR_PERM(upl), upl->upl_priority, 0);
					vm_decmp_upl_reprioritize(upl, cur_tier);
					break;
				}
				KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_PAGE_EXPEDITE)) | DBG_FUNC_NONE, VM_KERNEL_UNSLIDE_OR_PERM(upl->upl_creator), VM_KERNEL_UNSLIDE_OR_PERM(m),
				    upl->upl_reprio_info[i], upl->upl_priority, 0);
				if (UPL_REPRIO_INFO_BLKNO(upl, i) != 0 && UPL_REPRIO_INFO_LEN(upl, i) != 0) {
					vm_page_request_reprioritize(o, UPL_REPRIO_INFO_BLKNO(upl, i), UPL_REPRIO_INFO_LEN(upl, i), cur_tier);
				}
				break;
			}
		}
		                                
		if (i != num_pages) {
			break;
		}
	}

	return;
}

wait_result_t
vm_page_sleep(vm_object_t o, vm_page_t m, int interruptible)
{
	wait_result_t ret;

	KERNEL_DEBUG((MACHDBG_CODE(DBG_MACH_VM, VM_PAGE_SLEEP)) | DBG_FUNC_START, o, m, 0, 0, 0);

	if (o->io_tracking && ((m->vmp_busy == TRUE) || (m->vmp_cleaning == TRUE) || VM_PAGE_WIRED(m))) {
		  
                                                                                      
     
		vm_page_handle_prio_inversion(o, m);
	}
	m->vmp_wanted = TRUE;
	ret = thread_sleep_vm_object(o, m, interruptible);
	KERNEL_DEBUG((MACHDBG_CODE(DBG_MACH_VM, VM_PAGE_SLEEP)) | DBG_FUNC_END, o, m, 0, 0, 0);
	return ret;
}

static void
io_reprioritize_thread(void *param __unused, wait_result_t wr __unused)
{
	io_reprioritize_req_t   req = NULL;

	while (1) {
		IO_REPRIORITIZE_LIST_LOCK();
		if (queue_empty(&io_reprioritize_list)) {
			IO_REPRIORITIZE_LIST_UNLOCK();
			break;
		}

		queue_remove_first(&io_reprioritize_list, req, io_reprioritize_req_t, io_reprioritize_list);
		IO_REPRIORITIZE_LIST_UNLOCK();

		vnode_pager_issue_reprioritize_io(req->devvp, req->blkno, req->len, req->priority);
		zfree(io_reprioritize_req_zone, req);
	}

	IO_REPRIO_THREAD_CONTINUATION();
}
#endif

#if VM_OBJECT_ACCESS_TRACKING
void
vm_object_access_tracking(
	vm_object_t     object,
	int             *access_tracking_p,
	uint32_t        *access_tracking_reads_p,
	uint32_t        *access_tracking_writes_p)
{
	int     access_tracking;

	access_tracking = !!*access_tracking_p;

	vm_object_lock(object);
	*access_tracking_p = object->access_tracking;
	if (access_tracking_reads_p) {
		*access_tracking_reads_p = object->access_tracking_reads;
	}
	if (access_tracking_writes_p) {
		*access_tracking_writes_p = object->access_tracking_writes;
	}
	object->access_tracking = access_tracking;
	object->access_tracking_reads = 0;
	object->access_tracking_writes = 0;
	vm_object_unlock(object);

	if (access_tracking) {
		vm_object_pmap_protect_options(object,
		    0,
		    object->vo_size,
		    PMAP_NULL,
		    PAGE_SIZE,
		    0,
		    VM_PROT_NONE,
		    0);
	}
}
#endif                                

void
vm_object_ledger_tag_ledgers(
	vm_object_t     object,
	int             *ledger_idx_volatile,
	int             *ledger_idx_nonvolatile,
	int             *ledger_idx_volatile_compressed,
	int             *ledger_idx_nonvolatile_compressed,
	boolean_t       *do_footprint)
{
	assert(object->shadow == VM_OBJECT_NULL);

	*do_footprint = !object->vo_no_footprint;

	switch (object->vo_ledger_tag) {
	case VM_LEDGER_TAG_NONE:
		  
                              
                                               
     
		*do_footprint = TRUE;
		assert(object->purgable != VM_PURGABLE_DENY);
		*ledger_idx_volatile = task_ledgers.purgeable_volatile;
		*ledger_idx_nonvolatile = task_ledgers.purgeable_nonvolatile;
		*ledger_idx_volatile_compressed = task_ledgers.purgeable_volatile_compressed;
		*ledger_idx_nonvolatile_compressed = task_ledgers.purgeable_nonvolatile_compressed;
		break;
	case VM_LEDGER_TAG_DEFAULT:
		  
                             
                                                             
                       
     
		*ledger_idx_volatile = task_ledgers.tagged_nofootprint;
		*ledger_idx_volatile_compressed = task_ledgers.tagged_nofootprint_compressed;
		if (*do_footprint) {
			*ledger_idx_nonvolatile = task_ledgers.tagged_footprint;
			*ledger_idx_nonvolatile_compressed = task_ledgers.tagged_footprint_compressed;
		} else {
			*ledger_idx_nonvolatile = task_ledgers.tagged_nofootprint;
			*ledger_idx_nonvolatile_compressed = task_ledgers.tagged_nofootprint_compressed;
		}
		break;
	case VM_LEDGER_TAG_NETWORK:
		  
                             
                               
     
		*do_footprint = FALSE;
		*ledger_idx_volatile = task_ledgers.network_volatile;
		*ledger_idx_volatile_compressed = task_ledgers.network_volatile_compressed;
		*ledger_idx_nonvolatile = task_ledgers.network_nonvolatile;
		*ledger_idx_nonvolatile_compressed = task_ledgers.network_nonvolatile_compressed;
		break;
	case VM_LEDGER_TAG_MEDIA:
		  
                           
                                                             
                       
     
		*ledger_idx_volatile = task_ledgers.media_nofootprint;
		*ledger_idx_volatile_compressed = task_ledgers.media_nofootprint_compressed;
		if (*do_footprint) {
			*ledger_idx_nonvolatile = task_ledgers.media_footprint;
			*ledger_idx_nonvolatile_compressed = task_ledgers.media_footprint_compressed;
		} else {
			*ledger_idx_nonvolatile = task_ledgers.media_nofootprint;
			*ledger_idx_nonvolatile_compressed = task_ledgers.media_nofootprint_compressed;
		}
		break;
	case VM_LEDGER_TAG_GRAPHICS:
		  
                              
                                                             
                       
     
		*ledger_idx_volatile = task_ledgers.graphics_nofootprint;
		*ledger_idx_volatile_compressed = task_ledgers.graphics_nofootprint_compressed;
		if (*do_footprint) {
			*ledger_idx_nonvolatile = task_ledgers.graphics_footprint;
			*ledger_idx_nonvolatile_compressed = task_ledgers.graphics_footprint_compressed;
		} else {
			*ledger_idx_nonvolatile = task_ledgers.graphics_nofootprint;
			*ledger_idx_nonvolatile_compressed = task_ledgers.graphics_nofootprint_compressed;
		}
		break;
	case VM_LEDGER_TAG_NEURAL:
		  
                            
                                                             
                       
     
		*ledger_idx_volatile = task_ledgers.neural_nofootprint;
		*ledger_idx_volatile_compressed = task_ledgers.neural_nofootprint_compressed;
		if (*do_footprint) {
			*ledger_idx_nonvolatile = task_ledgers.neural_footprint;
			*ledger_idx_nonvolatile_compressed = task_ledgers.neural_footprint_compressed;
		} else {
			*ledger_idx_nonvolatile = task_ledgers.neural_nofootprint;
			*ledger_idx_nonvolatile_compressed = task_ledgers.neural_nofootprint_compressed;
		}
		break;
	default:
		panic("%s: object %p has unsupported ledger_tag %d\n",
		    __FUNCTION__, object, object->vo_ledger_tag);
	}
}

kern_return_t
vm_object_ownership_change(
	vm_object_t     object,
	int             new_ledger_tag,
	task_t          new_owner,
	int             new_ledger_flags,
	boolean_t       old_task_objq_locked)
{
	int             old_ledger_tag;
	task_t          old_owner;
	int             resident_count, wired_count;
	unsigned int    compressed_count;
	int             ledger_idx_volatile;
	int             ledger_idx_nonvolatile;
	int             ledger_idx_volatile_compressed;
	int             ledger_idx_nonvolatile_compressed;
	int             ledger_idx;
	int             ledger_idx_compressed;
	boolean_t       do_footprint, old_no_footprint, new_no_footprint;
	boolean_t       new_task_objq_locked;

	vm_object_lock_assert_exclusive(object);

	if (!object->internal) {
		return KERN_INVALID_ARGUMENT;
	}
	if (new_ledger_tag == VM_LEDGER_TAG_NONE &&
	    object->purgable == VM_PURGABLE_DENY) {
		                                                                
		return KERN_INVALID_ARGUMENT;
	}
	if (new_ledger_tag < 0 ||
	    new_ledger_tag > VM_LEDGER_TAG_MAX) {
		return KERN_INVALID_ARGUMENT;
	}
	if (new_ledger_flags & ~VM_LEDGER_FLAGS) {
		return KERN_INVALID_ARGUMENT;
	}
	if (object->vo_ledger_tag == VM_LEDGER_TAG_NONE &&
	    object->purgable == VM_PURGABLE_DENY) {
		  
                                                           
                                                       
                                                        
                                                             
     
		if (object->resident_page_count != 0 ||
		    object->pager != NULL ||
		    object->pager_created ||
		    object->ref_count != 1 ||
		    object->vo_owner != TASK_NULL ||
		    object->copy_strategy != MEMORY_OBJECT_COPY_NONE ||
		    new_owner == TASK_NULL) {
			return KERN_FAILURE;
		}
	}

	if (new_ledger_flags & VM_LEDGER_FLAG_NO_FOOTPRINT) {
		new_no_footprint = TRUE;
	} else {
		new_no_footprint = FALSE;
	}
#if __arm64__
	if (!new_no_footprint &&
	    object->purgable != VM_PURGABLE_DENY &&
	    new_owner != TASK_NULL &&
	    new_owner != VM_OBJECT_OWNER_DISOWNED &&
	    new_owner->task_legacy_footprint) {
		  
                                                             
                                                             
                                                          
                                                               
            
     
		new_no_footprint = TRUE;
	}
#endif                
	assert(object->copy_strategy == MEMORY_OBJECT_COPY_NONE);
	assert(object->shadow == VM_OBJECT_NULL);
	assert(object->copy == VM_OBJECT_NULL);

	old_ledger_tag = object->vo_ledger_tag;
	old_no_footprint = object->vo_no_footprint;
	old_owner = VM_OBJECT_OWNER(object);

	DTRACE_VM8(object_ownership_change,
	    vm_object_t, object,
	    task_t, old_owner,
	    int, old_ledger_tag,
	    int, old_no_footprint,
	    task_t, new_owner,
	    int, new_ledger_tag,
	    int, new_no_footprint,
	    int, VM_OBJECT_ID(object));

	assert(object->internal);
	resident_count = object->resident_page_count - object->wired_page_count;
	wired_count = object->wired_page_count;
	compressed_count = vm_compressor_pager_get_count(object->pager);

	  
                                                         
    
	if (old_owner != TASK_NULL &&
	    ((old_owner != new_owner)                              
	    ||                                                  
	    (old_no_footprint != new_no_footprint)                         
	    ||                                                  
	    old_ledger_tag != new_ledger_tag)) {                     
		  
                                                     
     
		vm_object_ledger_tag_ledgers(object,
		    &ledger_idx_volatile,
		    &ledger_idx_nonvolatile,
		    &ledger_idx_volatile_compressed,
		    &ledger_idx_nonvolatile_compressed,
		    &do_footprint);
		if (object->purgable == VM_PURGABLE_VOLATILE ||
		    object->purgable == VM_PURGABLE_EMPTY) {
			ledger_idx = ledger_idx_volatile;
			ledger_idx_compressed = ledger_idx_volatile_compressed;
		} else {
			ledger_idx = ledger_idx_nonvolatile;
			ledger_idx_compressed = ledger_idx_nonvolatile_compressed;
		}
		if (resident_count) {
			  
                                                        
                               
      
			ledger_debit(old_owner->ledger,
			    ledger_idx,
			    ptoa_64(resident_count));
			                                  
			if (do_footprint &&
			    object->purgable != VM_PURGABLE_VOLATILE &&
			    object->purgable != VM_PURGABLE_EMPTY) {
				ledger_debit(old_owner->ledger,
				    task_ledgers.phys_footprint,
				    ptoa_64(resident_count));
			}
		}
		if (wired_count) {
			                                        
			ledger_debit(old_owner->ledger,
			    ledger_idx_nonvolatile,
			    ptoa_64(wired_count));
			if (do_footprint) {
				ledger_debit(old_owner->ledger,
				    task_ledgers.phys_footprint,
				    ptoa_64(wired_count));
			}
		}
		if (compressed_count) {
			  
                                                
                                        
      
			ledger_debit(old_owner->ledger,
			    ledger_idx_compressed,
			    ptoa_64(compressed_count));
			if (do_footprint &&
			    object->purgable != VM_PURGABLE_VOLATILE &&
			    object->purgable != VM_PURGABLE_EMPTY) {
				ledger_debit(old_owner->ledger,
				    task_ledgers.phys_footprint,
				    ptoa_64(compressed_count));
			}
		}
		if (old_owner != new_owner) {
			                                                          
			DTRACE_VM2(object_owner_remove,
			    vm_object_t, object,
			    task_t, old_owner);
			if (!old_task_objq_locked) {
				task_objq_lock(old_owner);
			}
			old_owner->task_owned_objects--;
			queue_remove(&old_owner->task_objq, object,
			    vm_object_t, task_objq);
			switch (object->purgable) {
			case VM_PURGABLE_NONVOLATILE:
			case VM_PURGABLE_EMPTY:
				vm_purgeable_nonvolatile_owner_update(old_owner,
				    -1);
				break;
			case VM_PURGABLE_VOLATILE:
				vm_purgeable_volatile_owner_update(old_owner,
				    -1);
				break;
			default:
				break;
			}
			if (!old_task_objq_locked) {
				task_objq_unlock(old_owner);
			}
		}
	}

	  
                                          
    

	new_task_objq_locked = FALSE;
	if (new_owner != old_owner &&
	    new_owner != TASK_NULL &&
	    new_owner != VM_OBJECT_OWNER_DISOWNED) {
		  
                                                                 
                                                       
                            
    
                                                             
                                   
     
		if (new_owner->task_objects_disowning) {
			new_owner = VM_OBJECT_OWNER_DISOWNED;
		} else {
			task_objq_lock(new_owner);
			                                           
			if (new_owner->task_objects_disowning) {
				new_owner = VM_OBJECT_OWNER_DISOWNED;
				task_objq_unlock(new_owner);
			} else {
				new_task_objq_locked = TRUE;
			}
		}
	}

	object->vo_ledger_tag = new_ledger_tag;
	object->vo_owner = new_owner;
	object->vo_no_footprint = new_no_footprint;

	if (new_owner == VM_OBJECT_OWNER_DISOWNED) {
		  
                                                             
                                                         
                                                           
                
     
		assert(old_owner != kernel_task);
		new_owner = kernel_task;
		assert(!new_task_objq_locked);
		task_objq_lock(new_owner);
		new_task_objq_locked = TRUE;
	}

	  
                                                         
    
	if (new_owner != TASK_NULL &&
	    ((new_owner != old_owner)                              
	    ||                                                  
	    (new_no_footprint != old_no_footprint)                             
	    ||                                                  
	    new_ledger_tag != old_ledger_tag)) {                     
		  
                                                
     
		vm_object_ledger_tag_ledgers(object,
		    &ledger_idx_volatile,
		    &ledger_idx_nonvolatile,
		    &ledger_idx_volatile_compressed,
		    &ledger_idx_nonvolatile_compressed,
		    &do_footprint);
		if (object->purgable == VM_PURGABLE_VOLATILE ||
		    object->purgable == VM_PURGABLE_EMPTY) {
			ledger_idx = ledger_idx_volatile;
			ledger_idx_compressed = ledger_idx_volatile_compressed;
		} else {
			ledger_idx = ledger_idx_nonvolatile;
			ledger_idx_compressed = ledger_idx_nonvolatile_compressed;
		}
		if (resident_count) {
			  
                                                        
                               
      
			ledger_credit(new_owner->ledger,
			    ledger_idx,
			    ptoa_64(resident_count));
			                                  
			if (do_footprint &&
			    object->purgable != VM_PURGABLE_VOLATILE &&
			    object->purgable != VM_PURGABLE_EMPTY) {
				ledger_credit(new_owner->ledger,
				    task_ledgers.phys_footprint,
				    ptoa_64(resident_count));
			}
		}
		if (wired_count) {
			                                        
			ledger_credit(new_owner->ledger,
			    ledger_idx_nonvolatile,
			    ptoa_64(wired_count));
			if (do_footprint) {
				ledger_credit(new_owner->ledger,
				    task_ledgers.phys_footprint,
				    ptoa_64(wired_count));
			}
		}
		if (compressed_count) {
			  
                                                     
                       
      
			ledger_credit(new_owner->ledger,
			    ledger_idx_compressed,
			    ptoa_64(compressed_count));
			if (do_footprint &&
			    object->purgable != VM_PURGABLE_VOLATILE &&
			    object->purgable != VM_PURGABLE_EMPTY) {
				ledger_credit(new_owner->ledger,
				    task_ledgers.phys_footprint,
				    ptoa_64(compressed_count));
			}
		}
		if (new_owner != old_owner) {
			                                                     
			DTRACE_VM2(object_owner_add,
			    vm_object_t, object,
			    task_t, new_owner);
			assert(new_task_objq_locked);
			new_owner->task_owned_objects++;
			queue_enter(&new_owner->task_objq, object,
			    vm_object_t, task_objq);
			switch (object->purgable) {
			case VM_PURGABLE_NONVOLATILE:
			case VM_PURGABLE_EMPTY:
				vm_purgeable_nonvolatile_owner_update(new_owner,
				    +1);
				break;
			case VM_PURGABLE_VOLATILE:
				vm_purgeable_volatile_owner_update(new_owner,
				    +1);
				break;
			default:
				break;
			}
		}
	}

	if (new_task_objq_locked) {
		task_objq_unlock(new_owner);
	}

	return KERN_SUCCESS;
}

void
vm_owned_objects_disown(
	task_t  task)
{
	vm_object_t     next_object;
	vm_object_t     object;
	int             collisions;
	kern_return_t   kr;

	if (task == NULL) {
		return;
	}

	collisions = 0;

again:
	if (task->task_objects_disowned) {
		                                                 
		assert(task->task_volatile_objects == 0);
		assert(task->task_nonvolatile_objects == 0);
		assert(task->task_owned_objects == 0);
		return;
	}

	task_objq_lock(task);

	task->task_objects_disowning = TRUE;

	for (object = (vm_object_t) queue_first(&task->task_objq);
	    !queue_end(&task->task_objq, (queue_entry_t) object);
	    object = next_object) {
		if (task->task_nonvolatile_objects == 0 &&
		    task->task_volatile_objects == 0 &&
		    task->task_owned_objects == 0) {
			                                     
			break;
		}

		next_object = (vm_object_t) queue_next(&object->task_objq);

#if DEBUG
		assert(object->vo_purgeable_volatilizer == NULL);
#endif            
		assert(object->vo_owner == task);
		if (!vm_object_lock_try(object)) {
			task_objq_unlock(task);
			mutex_pause(collisions++);
			goto again;
		}
		                                      
		assert(VM_OBJECT_OWNER(object) != kernel_task);
		kr = vm_object_ownership_change(
			object,
			object->vo_ledger_tag,                
			VM_OBJECT_OWNER_DISOWNED,                
			0,                       
			TRUE);                                   
		assert(kr == KERN_SUCCESS);
		assert(object->vo_owner == VM_OBJECT_OWNER_DISOWNED);
		vm_object_unlock(object);
	}

	if (__improbable(task->task_owned_objects != 0)) {
		panic("%s(%p): volatile=%d nonvolatile=%d owned=%d q=%p q_first=%p q_last=%p",
		    __FUNCTION__,
		    task,
		    task->task_volatile_objects,
		    task->task_nonvolatile_objects,
		    task->task_owned_objects,
		    &task->task_objq,
		    queue_first(&task->task_objq),
		    queue_last(&task->task_objq));
	}

	                                                      
	assert(task->task_volatile_objects == 0);
	assert(task->task_nonvolatile_objects == 0);
	assert(task->task_owned_objects == 0);
	assert(task->task_objects_disowning);

	                                               
	task->task_objects_disowned = TRUE;

	task_objq_unlock(task);
}
