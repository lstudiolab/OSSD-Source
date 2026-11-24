/*
 * Copyright (c) 2014-2020 Apple Inc. All rights reserved.
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

#include <sys/errno.h>

#include <mach/mach_types.h>
#include <mach/mach_traps.h>
#include <mach/host_priv.h>
#include <mach/kern_return.h>
#include <mach/memory_object_control.h>
#include <mach/memory_object_types.h>
#include <mach/port.h>
#include <mach/policy.h>
#include <mach/upl.h>
#include <mach/thread_act.h>
#include <mach/mach_vm.h>

#include <kern/host.h>
#include <kern/kalloc.h>
#include <kern/page_decrypt.h>
#include <kern/queue.h>
#include <kern/thread.h>
#include <kern/ipc_kobject.h>

#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>

#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_pageout.h>
#include <vm/memory_object.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>
#include <vm/vm_kern.h>


  
                  
  
                                                                      
                                                                            
  
                                                                            
                                                                      
                                                                
                                                                         
                                                                       
               
  
                                                                               
                                                                        
                                
  
   

                          
void fourk_pager_reference(memory_object_t mem_obj);
void fourk_pager_deallocate(memory_object_t mem_obj);
kern_return_t fourk_pager_init(memory_object_t mem_obj,
    memory_object_control_t control,
    memory_object_cluster_size_t pg_size);
kern_return_t fourk_pager_terminate(memory_object_t mem_obj);
kern_return_t fourk_pager_data_request(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t length,
    vm_prot_t protection_required,
    memory_object_fault_info_t fault_info);
kern_return_t fourk_pager_data_return(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t    data_cnt,
    memory_object_offset_t *resid_offset,
    int *io_error,
    boolean_t dirty,
    boolean_t kernel_copy,
    int upl_flags);
kern_return_t fourk_pager_data_initialize(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t data_cnt);
kern_return_t fourk_pager_data_unlock(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_size_t size,
    vm_prot_t desired_access);
kern_return_t fourk_pager_synchronize(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_size_t length,
    vm_sync_t sync_flags);
kern_return_t fourk_pager_map(memory_object_t mem_obj,
    vm_prot_t prot);
kern_return_t fourk_pager_last_unmap(memory_object_t mem_obj);

  
                                        
                                                                         
   
const struct memory_object_pager_ops fourk_pager_ops = {
	.memory_object_reference = fourk_pager_reference,
	.memory_object_deallocate = fourk_pager_deallocate,
	.memory_object_init = fourk_pager_init,
	.memory_object_terminate = fourk_pager_terminate,
	.memory_object_data_request = fourk_pager_data_request,
	.memory_object_data_return = fourk_pager_data_return,
	.memory_object_data_initialize = fourk_pager_data_initialize,
	.memory_object_data_unlock = fourk_pager_data_unlock,
	.memory_object_synchronize = fourk_pager_synchronize,
	.memory_object_map = fourk_pager_map,
	.memory_object_last_unmap = fourk_pager_last_unmap,
	.memory_object_data_reclaim = NULL,
	.memory_object_backing_object = NULL,
	.memory_object_pager_name = "fourk_pager"
};

  
                                                        
                
   
#define FOURK_PAGER_SLOTS 4                   
typedef struct fourk_pager_backing {
	vm_object_t             backing_object;
	vm_object_offset_t      backing_offset;
} *fourk_pager_backing_t;
typedef struct fourk_pager {
	                              
	struct memory_object fourk_pgr_hdr;

	                         
	queue_chain_t           pager_queue;                            
#if MEMORY_OBJECT_HAS_REFCOUNT
#define fourk_pgr_hdr_ref       fourk_pgr_hdr.mo_ref
#else
	os_ref_atomic_t         fourk_pgr_hdr_ref;
#endif
	bool    is_ready;                                  
	bool    is_mapped;                                    
	struct fourk_pager_backing slots[FOURK_PAGER_SLOTS];                    
                                                                     
} *fourk_pager_t;
#define FOURK_PAGER_NULL        ((fourk_pager_t) NULL)

  
                                              
                                                        
   
int fourk_pager_count = 0;                                    
int fourk_pager_count_mapped = 0;                                      
queue_head_t fourk_pager_queue = QUEUE_HEAD_INITIALIZER(fourk_pager_queue);
LCK_GRP_DECLARE(fourk_pager_lck_grp, "4K-pager");
LCK_MTX_DECLARE(fourk_pager_lock, &fourk_pager_lck_grp);

  
                                                                  
   
int fourk_pager_cache_limit = 0;

  
                         
   
int fourk_pager_count_max = 0;
int fourk_pager_count_unmapped_max = 0;
int fourk_pager_num_trim_max = 0;
int fourk_pager_num_trim_total = 0;

                         
fourk_pager_t fourk_pager_lookup(memory_object_t mem_obj);
void fourk_pager_dequeue(fourk_pager_t pager);
void fourk_pager_deallocate_internal(fourk_pager_t pager,
    boolean_t locked);
void fourk_pager_terminate_internal(fourk_pager_t pager);
void fourk_pager_trim(void);


#if DEBUG
int fourk_pagerdebug = 0;
#define PAGER_ALL               0xffffffff
#define PAGER_INIT              0x00000001
#define PAGER_PAGEIN            0x00000002

#define PAGER_DEBUG(LEVEL, A)                                           \
	MACRO_BEGIN                                                     \
	if ((fourk_pagerdebug & LEVEL)==LEVEL) {                \
	        printf A;                                               \
	}                                                               \
	MACRO_END
#else
#define PAGER_DEBUG(LEVEL, A)
#endif


  
                     
  
                                                                         
   
kern_return_t
fourk_pager_init(
	memory_object_t         mem_obj,
	memory_object_control_t control,
#if !DEBUG
	__unused
#endif
	memory_object_cluster_size_t pg_size)
{
	fourk_pager_t   pager;
	kern_return_t           kr;
	memory_object_attr_info_data_t  attributes;

	PAGER_DEBUG(PAGER_ALL,
	    ("fourk_pager_init: %p, %p, %x\n",
	    mem_obj, control, pg_size));

	if (control == MEMORY_OBJECT_CONTROL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	pager = fourk_pager_lookup(mem_obj);

	memory_object_control_reference(control);

	pager->fourk_pgr_hdr.mo_control = control;

	attributes.copy_strategy = MEMORY_OBJECT_COPY_DELAY;
	                                                                   
	attributes.cluster_size = (1 << (PAGE_SHIFT));
	attributes.may_cache_object = FALSE;
	attributes.temporary = TRUE;

	kr = memory_object_change_attributes(
		control,
		MEMORY_OBJECT_ATTRIBUTE_INFO,
		(memory_object_info_t) &attributes,
		MEMORY_OBJECT_ATTR_INFO_COUNT);
	if (kr != KERN_SUCCESS) {
		panic("fourk_pager_init: "
		    "memory_object_change_attributes() failed");
	}

#if CONFIG_SECLUDED_MEMORY
	if (secluded_for_filecache) {
		memory_object_mark_eligible_for_secluded(control, TRUE);
	}
#endif                             

	return KERN_SUCCESS;
}

  
                            
  
                                                                     
                                                                         
                                                                        
            
   
kern_return_t
fourk_pager_data_return(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_cluster_size_t           data_cnt,
	__unused memory_object_offset_t *resid_offset,
	__unused int                    *io_error,
	__unused boolean_t              dirty,
	__unused boolean_t              kernel_copy,
	__unused int                    upl_flags)
{
	panic("fourk_pager_data_return: should never get called");
	return KERN_FAILURE;
}

kern_return_t
fourk_pager_data_initialize(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_cluster_size_t           data_cnt)
{
	panic("fourk_pager_data_initialize: should never get called");
	return KERN_FAILURE;
}

kern_return_t
fourk_pager_data_unlock(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_size_t           size,
	__unused vm_prot_t              desired_access)
{
	return KERN_FAILURE;
}

  
                          
  
                                         
                                                                               
                                                     
   
void
fourk_pager_reference(
	memory_object_t         mem_obj)
{
	fourk_pager_t   pager;

	pager = fourk_pager_lookup(mem_obj);

	lck_mtx_lock(&fourk_pager_lock);
	os_ref_retain_locked_raw(&pager->fourk_pgr_hdr_ref, NULL);
	lck_mtx_unlock(&fourk_pager_lock);
}


  
                       
  
                                           
  
                                           
   
void
fourk_pager_dequeue(
	fourk_pager_t pager)
{
	assert(!pager->is_mapped);

	queue_remove(&fourk_pager_queue,
	    pager,
	    fourk_pager_t,
	    pager_queue);
	pager->pager_queue.next = NULL;
	pager->pager_queue.prev = NULL;

	fourk_pager_count--;
}

  
                                  
  
                                                                       
                   
                                                                    
                                                                
                          
  
                                                       
                                                                         
                                                
   
void
fourk_pager_terminate_internal(
	fourk_pager_t pager)
{
	int i;

	assert(pager->is_ready);
	assert(!pager->is_mapped);

	for (i = 0; i < FOURK_PAGER_SLOTS; i++) {
		if (pager->slots[i].backing_object != VM_OBJECT_NULL &&
		    pager->slots[i].backing_object != (vm_object_t) -1) {
			vm_object_deallocate(pager->slots[i].backing_object);
			pager->slots[i].backing_object = (vm_object_t) -1;
			pager->slots[i].backing_offset = (vm_object_offset_t) -1;
		}
	}

	                                                  
	memory_object_destroy(pager->fourk_pgr_hdr.mo_control, 0);
}

  
                                    
  
                                                              
                       
                                                                     
                    
   
void
fourk_pager_deallocate_internal(
	fourk_pager_t   pager,
	boolean_t               locked)
{
	boolean_t       needs_trimming;
	int             count_unmapped;
	os_ref_count_t  ref_count;

	if (!locked) {
		lck_mtx_lock(&fourk_pager_lock);
	}

	count_unmapped = (fourk_pager_count -
	    fourk_pager_count_mapped);
	if (count_unmapped > fourk_pager_cache_limit) {
		                                                  
		needs_trimming = TRUE;
	} else {
		needs_trimming = FALSE;
	}

	                                    
	ref_count = os_ref_release_locked_raw(&pager->fourk_pgr_hdr_ref, NULL);

	if (ref_count == 1) {
		  
                                                         
                                                       
                  
     
		fourk_pager_dequeue(pager);
		                                                     
		lck_mtx_unlock(&fourk_pager_lock);
		fourk_pager_terminate_internal(pager);
	} else if (ref_count == 0) {
		  
                                                            
                                                            
                     
     
		lck_mtx_unlock(&fourk_pager_lock);
		if (pager->fourk_pgr_hdr.mo_control != MEMORY_OBJECT_CONTROL_NULL) {
			memory_object_control_deallocate(pager->fourk_pgr_hdr.mo_control);
			pager->fourk_pgr_hdr.mo_control = MEMORY_OBJECT_CONTROL_NULL;
		}
		kfree(pager, sizeof(*pager));
		pager = FOURK_PAGER_NULL;
	} else {
		                                                          
		lck_mtx_unlock(&fourk_pager_lock);
	}

	if (needs_trimming) {
		fourk_pager_trim();
	}
	                                            
}

  
                           
  
                                                              
                       
   
void
fourk_pager_deallocate(
	memory_object_t         mem_obj)
{
	fourk_pager_t   pager;

	PAGER_DEBUG(PAGER_ALL, ("fourk_pager_deallocate: %p\n", mem_obj));
	pager = fourk_pager_lookup(mem_obj);
	fourk_pager_deallocate_internal(pager, FALSE);
}

  
  
   
kern_return_t
fourk_pager_terminate(
#if !DEBUG
	__unused
#endif
	memory_object_t mem_obj)
{
	PAGER_DEBUG(PAGER_ALL, ("fourk_pager_terminate: %p\n", mem_obj));

	return KERN_SUCCESS;
}

  
  
   
kern_return_t
fourk_pager_synchronize(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_size_t   length,
	__unused vm_sync_t              sync_flags)
{
	panic("fourk_pager_synchronize: memory_object_synchronize no longer supported\n");
	return KERN_FAILURE;
}

  
                    
  
                                                                  
                                                                         
                                                                       
                                                 
   
kern_return_t
fourk_pager_map(
	memory_object_t         mem_obj,
	__unused vm_prot_t      prot)
{
	fourk_pager_t   pager;

	PAGER_DEBUG(PAGER_ALL, ("fourk_pager_map: %p\n", mem_obj));

	pager = fourk_pager_lookup(mem_obj);

	lck_mtx_lock(&fourk_pager_lock);
	assert(pager->is_ready);
	assert(os_ref_get_count_raw(&pager->fourk_pgr_hdr_ref) > 0);                     
	if (pager->is_mapped == FALSE) {
		  
                                                          
                                                          
                 
     
		pager->is_mapped = TRUE;
		os_ref_retain_locked_raw(&pager->fourk_pgr_hdr_ref, NULL);
		fourk_pager_count_mapped++;
	}
	lck_mtx_unlock(&fourk_pager_lock);

	return KERN_SUCCESS;
}

  
                           
  
                                                                             
   
kern_return_t
fourk_pager_last_unmap(
	memory_object_t         mem_obj)
{
	fourk_pager_t   pager;
	int                     count_unmapped;

	PAGER_DEBUG(PAGER_ALL,
	    ("fourk_pager_last_unmap: %p\n", mem_obj));

	pager = fourk_pager_lookup(mem_obj);

	lck_mtx_lock(&fourk_pager_lock);
	if (pager->is_mapped) {
		  
                                                          
                                                              
     
		fourk_pager_count_mapped--;
		count_unmapped = (fourk_pager_count -
		    fourk_pager_count_mapped);
		if (count_unmapped > fourk_pager_count_unmapped_max) {
			fourk_pager_count_unmapped_max = count_unmapped;
		}
		pager->is_mapped = FALSE;
		fourk_pager_deallocate_internal(pager, TRUE);
		                                                        
	} else {
		lck_mtx_unlock(&fourk_pager_lock);
	}

	return KERN_SUCCESS;
}


  
  
   
fourk_pager_t
fourk_pager_lookup(
	memory_object_t  mem_obj)
{
	fourk_pager_t   pager;

	assert(mem_obj->mo_pager_ops == &fourk_pager_ops);
	pager = (fourk_pager_t) mem_obj;
	assert(os_ref_get_count_raw(&pager->fourk_pgr_hdr_ref) > 0);
	return pager;
}

void
fourk_pager_trim(void)
{
	fourk_pager_t   pager, prev_pager;
	queue_head_t            trim_queue;
	int                     num_trim;
	int                     count_unmapped;

	lck_mtx_lock(&fourk_pager_lock);

	  
                                                           
                                                           
    
	queue_init(&trim_queue);
	num_trim = 0;

	for (pager = (fourk_pager_t)
	    queue_last(&fourk_pager_queue);
	    !queue_end(&fourk_pager_queue,
	    (queue_entry_t) pager);
	    pager = prev_pager) {
		                                    
		prev_pager = (fourk_pager_t)
		    queue_prev(&pager->pager_queue);

		if (os_ref_get_count_raw(&pager->fourk_pgr_hdr_ref) == 2 &&
		    pager->is_ready &&
		    !pager->is_mapped) {
			                               
			num_trim++;
			                                              
			fourk_pager_dequeue(pager);
			                                      
			queue_enter_first(&trim_queue,
			    pager,
			    fourk_pager_t,
			    pager_queue);

			count_unmapped = (fourk_pager_count -
			    fourk_pager_count_mapped);
			if (count_unmapped <= fourk_pager_cache_limit) {
				                                   
				break;
			}
		}
	}
	if (num_trim > fourk_pager_num_trim_max) {
		fourk_pager_num_trim_max = num_trim;
	}
	fourk_pager_num_trim_total += num_trim;

	lck_mtx_unlock(&fourk_pager_lock);

	                                  
	while (!queue_empty(&trim_queue)) {
		queue_remove_first(&trim_queue,
		    pager,
		    fourk_pager_t,
		    pager_queue);
		pager->pager_queue.next = NULL;
		pager->pager_queue.prev = NULL;
		assert(os_ref_get_count_raw(&pager->fourk_pgr_hdr_ref) == 2);
		  
                                                          
                                                           
                 
     
		(void)os_ref_release_locked_raw(&pager->fourk_pgr_hdr_ref, NULL);
		fourk_pager_terminate_internal(pager);
	}
}






vm_object_t
fourk_pager_to_vm_object(
	memory_object_t mem_obj)
{
	fourk_pager_t   pager;
	vm_object_t     object;

	pager = fourk_pager_lookup(mem_obj);
	if (pager == NULL) {
		return VM_OBJECT_NULL;
	}

	assert(os_ref_get_count_raw(&pager->fourk_pgr_hdr_ref) > 0);
	assert(pager->fourk_pgr_hdr.mo_control != MEMORY_OBJECT_CONTROL_NULL);
	object = memory_object_control_to_vm_object(pager->fourk_pgr_hdr.mo_control);
	assert(object != VM_OBJECT_NULL);
	return object;
}

memory_object_t
fourk_pager_create(void)
{
	fourk_pager_t           pager;
	memory_object_control_t control;
	kern_return_t           kr;
	int                     i;

#if 00
	if (PAGE_SIZE_64 == FOURK_PAGE_SIZE) {
		panic("fourk_pager_create: page size is 4K !?");
	}
#endif

	pager = (fourk_pager_t) kalloc(sizeof(*pager));
	if (pager == FOURK_PAGER_NULL) {
		return MEMORY_OBJECT_NULL;
	}
	bzero(pager, sizeof(*pager));

	  
                                                               
                                                             
                                                               
                                                                
                                                                 
    
	pager->fourk_pgr_hdr.mo_ikot = IKOT_MEMORY_OBJECT;
	pager->fourk_pgr_hdr.mo_pager_ops = &fourk_pager_ops;
	pager->fourk_pgr_hdr.mo_control = MEMORY_OBJECT_CONTROL_NULL;

	os_ref_init_count_raw(&pager->fourk_pgr_hdr_ref, NULL, 2);                                  
	pager->is_ready = FALSE;                                      
	pager->is_mapped = FALSE;

	for (i = 0; i < FOURK_PAGER_SLOTS; i++) {
		pager->slots[i].backing_object = (vm_object_t) -1;
		pager->slots[i].backing_offset = (vm_object_offset_t) -1;
	}

	lck_mtx_lock(&fourk_pager_lock);

	                                                       
	queue_enter_first(&fourk_pager_queue,
	    pager,
	    fourk_pager_t,
	    pager_queue);
	fourk_pager_count++;
	if (fourk_pager_count > fourk_pager_count_max) {
		fourk_pager_count_max = fourk_pager_count;
	}
	lck_mtx_unlock(&fourk_pager_lock);

	kr = memory_object_create_named((memory_object_t) pager,
	    0,
	    &control);
	assert(kr == KERN_SUCCESS);

	memory_object_mark_trusted(control);

	lck_mtx_lock(&fourk_pager_lock);
	                                           
	pager->is_ready = TRUE;
	lck_mtx_unlock(&fourk_pager_lock);

	                                                      
	thread_wakeup(&pager->is_ready);

	return (memory_object_t) pager;
}

  
                             
  
                                    
   
int fourk_pager_data_request_debug = 0;
kern_return_t
fourk_pager_data_request(
	memory_object_t         mem_obj,
	memory_object_offset_t  offset,
	memory_object_cluster_size_t            length,
#if !DEBUG
	__unused
#endif
	vm_prot_t               protection_required,
	memory_object_fault_info_t mo_fault_info)
{
	fourk_pager_t           pager;
	memory_object_control_t mo_control;
	upl_t                   upl;
	int                     upl_flags;
	upl_size_t              upl_size;
	upl_page_info_t         *upl_pl;
	unsigned int            pl_count;
	vm_object_t             dst_object;
	kern_return_t           kr, retval;
	vm_map_offset_t         kernel_mapping;
	vm_offset_t             src_vaddr, dst_vaddr;
	vm_offset_t             cur_offset;
	int                     sub_page;
	int                     sub_page_idx, sub_page_cnt;

	pager = fourk_pager_lookup(mem_obj);
	assert(pager->is_ready);
	assert(os_ref_get_count_raw(&pager->fourk_pgr_hdr_ref) > 1);                                

	PAGER_DEBUG(PAGER_PAGEIN, ("fourk_pager_data_request: %p, %llx, %x, %x, pager %p\n", mem_obj, offset, length, protection_required, pager));

	retval = KERN_SUCCESS;
	kernel_mapping = 0;

	offset = memory_object_trunc_page(offset);

	  
                                                     
    
	mo_control = pager->fourk_pgr_hdr.mo_control;

	upl_size = length;
	upl_flags =
	    UPL_RET_ONLY_ABSENT |
	    UPL_SET_LITE |
	    UPL_NO_SYNC |
	    UPL_CLEAN_IN_PLACE |                                      
	    UPL_SET_INTERNAL;
	pl_count = 0;
	kr = memory_object_upl_request(mo_control,
	    offset, upl_size,
	    &upl, NULL, NULL, upl_flags, VM_KERN_MEMORY_NONE);
	if (kr != KERN_SUCCESS) {
		retval = kr;
		goto done;
	}
	dst_object = memory_object_control_to_vm_object(mo_control);
	assert(dst_object != VM_OBJECT_NULL);

#if __x86_64__ || __arm__ || __arm64__
	                                               
#else                                         
	  
                                                                  
                                                                 
                 
    
	vm_map_entry_t          map_entry;

	vm_object_reference(kernel_object);                           
	kr = vm_map_find_space(kernel_map,
	    &kernel_mapping,
	    2 * PAGE_SIZE_64,
	    0,
	    0,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    &map_entry);
	if (kr != KERN_SUCCESS) {
		vm_object_deallocate(kernel_object);
		retval = kr;
		goto done;
	}
	map_entry->object.vm_object = kernel_object;
	map_entry->offset = kernel_mapping;
	vm_map_unlock(kernel_map);
	src_vaddr = CAST_DOWN(vm_offset_t, kernel_mapping);
	dst_vaddr = CAST_DOWN(vm_offset_t, kernel_mapping + PAGE_SIZE_64);
#endif                                         

	  
                                                      
    
	upl_pl = UPL_GET_INTERNAL_PAGE_LIST(upl);
	pl_count = length / PAGE_SIZE;
	for (cur_offset = 0;
	    retval == KERN_SUCCESS && cur_offset < length;
	    cur_offset += PAGE_SIZE) {
		ppnum_t dst_pnum;
		int num_subpg_signed, num_subpg_validated;
		int num_subpg_tainted, num_subpg_nx;

		if (!upl_page_present(upl_pl, (int)(cur_offset / PAGE_SIZE))) {
			                                          
			continue;
		}

		  
                                                          
                   
                                                         
               
     
		dst_pnum = (ppnum_t)
		    upl_phys_page(upl_pl, (int)(cur_offset / PAGE_SIZE));
		assert(dst_pnum != 0);
		dst_vaddr = (vm_map_offset_t)
		    phystokv((pmap_paddr_t)dst_pnum << PAGE_SHIFT);

		                                                             
		if (PAGE_SHIFT == FOURK_PAGE_SHIFT &&
		    page_shift_user32 == SIXTEENK_PAGE_SHIFT) {
			  
                                                 
                     
      
			assert(PAGE_SHIFT == FOURK_PAGE_SHIFT);
			assert(page_shift_user32 == SIXTEENK_PAGE_SHIFT);
			sub_page_idx = ((offset & SIXTEENK_PAGE_MASK) /
			    PAGE_SIZE);
			  
                                             
      
			sub_page_cnt = 1;
		} else {
			  
                                                        
                                       
      
			assert(PAGE_SHIFT == SIXTEENK_PAGE_SHIFT);
			sub_page_idx = 0;
			sub_page_cnt = FOURK_PAGER_SLOTS;
		}

		num_subpg_signed = 0;
		num_subpg_validated = 0;
		num_subpg_tainted = 0;
		num_subpg_nx = 0;

		                                                             
		for (sub_page = sub_page_idx;
		    sub_page < sub_page_idx + sub_page_cnt;
		    sub_page++) {
			vm_object_t             src_object;
			memory_object_offset_t  src_offset;
			vm_offset_t             offset_in_src_page;
			kern_return_t           error_code;
			vm_object_t             src_page_object;
			vm_page_t               src_page;
			vm_page_t               top_page;
			vm_prot_t               prot;
			int                     interruptible;
			struct vm_object_fault_info     fault_info;
			boolean_t       subpg_validated;
			unsigned        subpg_tainted;


			if (offset < SIXTEENK_PAGE_SIZE) {
				  
                                          
                                        
                            
       
				src_object =
				    pager->slots[sub_page].backing_object;
				src_offset =
				    pager->slots[sub_page].backing_offset;
			} else {
				fourk_pager_backing_t slot;

				  
                                              
                                             
                                
       
				slot = &pager->slots[FOURK_PAGER_SLOTS - 1];
				src_object = slot->backing_object;
				src_offset = slot->backing_offset;
				src_offset += FOURK_PAGE_SIZE;
				src_offset +=
				    (vm_map_trunc_page(offset,
				    SIXTEENK_PAGE_MASK)
				    - SIXTEENK_PAGE_SIZE);
				src_offset += sub_page * FOURK_PAGE_SIZE;
			}
			offset_in_src_page = src_offset & PAGE_MASK_64;
			src_offset = vm_object_trunc_page(src_offset);

			if (src_object == VM_OBJECT_NULL ||
			    src_object == (vm_object_t) -1) {
				               
				bzero((char *)(dst_vaddr +
				    ((sub_page - sub_page_idx)
				    * FOURK_PAGE_SIZE)),
				    FOURK_PAGE_SIZE);
				if (fourk_pager_data_request_debug) {
					printf("fourk_pager_data_request"
					    "(%p,0x%llx+0x%lx+0x%04x): "
					    "ZERO\n",
					    pager,
					    offset,
					    cur_offset,
					    ((sub_page - sub_page_idx)
					    * FOURK_PAGE_SIZE));
				}
				continue;
			}

			                                              
retry_src_fault:
			src_page = VM_PAGE_NULL;
			top_page = VM_PAGE_NULL;
			fault_info = *((struct vm_object_fault_info *)
			    (uintptr_t)mo_fault_info);
			fault_info.stealth = TRUE;
			fault_info.io_sync = FALSE;
			fault_info.mark_zf_absent = FALSE;
			fault_info.batch_pmap_op = FALSE;
			interruptible = fault_info.interruptible;
			prot = VM_PROT_READ;
			error_code = 0;

			vm_object_lock(src_object);
			vm_object_paging_begin(src_object);
			kr = vm_fault_page(src_object,
			    src_offset,
			    VM_PROT_READ,
			    FALSE,
			    FALSE,                                            
			    &prot,
			    &src_page,
			    &top_page,
			    NULL,
			    &error_code,
			    FALSE,
			    FALSE,
			    &fault_info);
			switch (kr) {
			case VM_FAULT_SUCCESS:
				break;
			case VM_FAULT_RETRY:
				goto retry_src_fault;
			case VM_FAULT_MEMORY_SHORTAGE:
				if (vm_page_wait(interruptible)) {
					goto retry_src_fault;
				}
				OS_FALLTHROUGH;
			case VM_FAULT_INTERRUPTED:
				retval = MACH_SEND_INTERRUPTED;
				goto src_fault_done;
			case VM_FAULT_SUCCESS_NO_VM_PAGE:
				                                  
				vm_object_paging_end(src_object);
				vm_object_unlock(src_object);
				OS_FALLTHROUGH;
			case VM_FAULT_MEMORY_ERROR:
				                            
				if (error_code) {
					retval = error_code;
				} else {
					retval = KERN_MEMORY_ERROR;
				}
				goto src_fault_done;
			default:
				panic("fourk_pager_data_request: "
				    "vm_fault_page() unexpected error 0x%x\n",
				    kr);
			}
			assert(src_page != VM_PAGE_NULL);
			assert(src_page->vmp_busy);

			src_page_object = VM_PAGE_OBJECT(src_page);

			if ((!VM_PAGE_PAGEABLE(src_page)) &&
			    !VM_PAGE_WIRED(src_page)) {
				vm_page_lockspin_queues();
				if ((!VM_PAGE_PAGEABLE(src_page)) &&
				    !VM_PAGE_WIRED(src_page)) {
					vm_page_deactivate(src_page);
				}
				vm_page_unlock_queues();
			}

			src_vaddr = (vm_map_offset_t)
			    phystokv((pmap_paddr_t)VM_PAGE_GET_PHYS_PAGE(src_page)
			        << PAGE_SHIFT);

			  
                                       
                         
      
			subpg_validated = FALSE;
			subpg_tainted = 0;
			if (src_page_object->code_signed) {
				vm_page_validate_cs_mapped_chunk(
					src_page,
					(const void *) src_vaddr,
					offset_in_src_page,
					FOURK_PAGE_SIZE,
					&subpg_validated,
					&subpg_tainted);
				num_subpg_signed++;
				if (subpg_validated) {
					num_subpg_validated++;
				}
				if (subpg_tainted & CS_VALIDATE_TAINTED) {
					num_subpg_tainted++;
				}
				if (subpg_tainted & CS_VALIDATE_NX) {
					                                    
					if (sub_page_cnt > 1) {
						  
                                 
                                    
                                  
                                     
                              
                            
         
					} else {
						num_subpg_nx++;
					}
				}
			}

			  
                                                  
                                                        
      
			bcopy((const char *)(src_vaddr + offset_in_src_page),
			    (char *)(dst_vaddr +
			    ((sub_page - sub_page_idx) *
			    FOURK_PAGE_SIZE)),
			    FOURK_PAGE_SIZE);
			if (fourk_pager_data_request_debug) {
				printf("fourk_data_request"
				    "(%p,0x%llx+0x%lx+0x%04x): "
				    "backed by [%p:0x%llx]: "
				    "[0x%016llx 0x%016llx] "
				    "code_signed=%d "
				    "cs_valid=%d cs_tainted=%d cs_nx=%d\n",
				    pager,
				    offset, cur_offset,
				    (sub_page - sub_page_idx) * FOURK_PAGE_SIZE,
				    src_page_object,
				    src_page->vmp_offset + offset_in_src_page,
				    *(uint64_t *)(dst_vaddr +
				    ((sub_page - sub_page_idx) *
				    FOURK_PAGE_SIZE)),
				    *(uint64_t *)(dst_vaddr +
				    ((sub_page - sub_page_idx) *
				    FOURK_PAGE_SIZE) +
				    8),
				    src_page_object->code_signed,
				    subpg_validated,
				    !!(subpg_tainted & CS_VALIDATE_TAINTED),
				    !!(subpg_tainted & CS_VALIDATE_NX));
			}

#if __x86_64__ || __arm__ || __arm64__
			                                                   
			src_vaddr = 0;
#else                                         
			  
                                                
                    
      
			pmap_remove(kernel_pmap,
			    (addr64_t) src_vaddr,
			    (addr64_t) src_vaddr + PAGE_SIZE_64);
#endif                                         

src_fault_done:
			  
                                            
      
			if (src_page) {
				assert(VM_PAGE_OBJECT(src_page) == src_page_object);

				PAGE_WAKEUP_DONE(src_page);
				src_page = VM_PAGE_NULL;
				vm_object_paging_end(src_page_object);
				vm_object_unlock(src_page_object);
				if (top_page) {
					vm_object_t     top_object;

					top_object = VM_PAGE_OBJECT(top_page);
					vm_object_lock(top_object);
					VM_PAGE_FREE(top_page);
					top_page = VM_PAGE_NULL;
					vm_object_paging_end(top_object);
					vm_object_unlock(top_object);
				}
			}
		}
		if (num_subpg_signed > 0) {
			                                                   
			if (num_subpg_tainted > 0) {
				                                              
				UPL_SET_CS_TAINTED(upl_pl,
				    cur_offset / PAGE_SIZE,
				    VMP_CS_ALL_TRUE);
				                                             
				UPL_SET_CS_VALIDATED(upl_pl,
				    cur_offset / PAGE_SIZE,
				    VMP_CS_ALL_TRUE);
			} else if (num_subpg_validated == num_subpg_signed) {
				  
                                              
                                               
                            
       
				UPL_SET_CS_VALIDATED(upl_pl,
				    cur_offset / PAGE_SIZE,
				    VMP_CS_ALL_TRUE);
			}
			if (num_subpg_nx > 0) {
				UPL_SET_CS_NX(upl_pl,
				    cur_offset / PAGE_SIZE,
				    VMP_CS_ALL_TRUE);
			}
		}
	}

done:
	if (upl != NULL) {
		                      

		  
                                                          
                                                            
                                                          
                               
                                       
     
		upl_clear_dirty(upl, TRUE);

		                             
		if (retval != KERN_SUCCESS) {
			upl_abort(upl, 0);
			if (retval == KERN_ABORTED) {
				wait_result_t   wait_result;

				  
                                               
                                               
                                               
                                              
                                                
                                                
                                           
       
				retval = KERN_SUCCESS;
				  
                                             
                                             
                                          
       
				wait_result = assert_wait_timeout(
					(event_t) fourk_pager_data_request,
					THREAD_UNINT,
					10000,            
					NSEC_PER_USEC);
				assert(wait_result == THREAD_WAITING);
				wait_result = thread_block(THREAD_CONTINUE_NULL);
				assert(wait_result == THREAD_TIMED_OUT);
			}
		} else {
			boolean_t empty;
			assertf(page_aligned(upl->u_offset) && page_aligned(upl->u_size),
			    "upl %p offset 0x%llx size 0x%x",
			    upl, upl->u_offset, upl->u_size);
			upl_commit_range(upl, 0, upl->u_size,
			    UPL_COMMIT_CS_VALIDATED | UPL_COMMIT_WRITTEN_BY_KERNEL,
			    upl_pl, pl_count, &empty);
		}

		                            
		upl_deallocate(upl);
		upl = NULL;
	}
	if (kernel_mapping != 0) {
		                                                              
		kr = vm_map_remove(kernel_map,
		    kernel_mapping,
		    kernel_mapping + (2 * PAGE_SIZE_64),
		    VM_MAP_REMOVE_NO_FLAGS);
		assert(kr == KERN_SUCCESS);
		kernel_mapping = 0;
		src_vaddr = 0;
		dst_vaddr = 0;
	}

	return retval;
}



kern_return_t
fourk_pager_populate(
	memory_object_t         mem_obj,
	boolean_t               overwrite,
	int                     index,
	vm_object_t             new_backing_object,
	vm_object_offset_t      new_backing_offset,
	vm_object_t             *old_backing_object,
	vm_object_offset_t      *old_backing_offset)
{
	fourk_pager_t   pager;

	pager = fourk_pager_lookup(mem_obj);
	if (pager == NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	assert(os_ref_get_count_raw(&pager->fourk_pgr_hdr_ref) > 0);
	assert(pager->fourk_pgr_hdr.mo_control != MEMORY_OBJECT_CONTROL_NULL);

	if (index < 0 || index > FOURK_PAGER_SLOTS) {
		return KERN_INVALID_ARGUMENT;
	}

	if (!overwrite &&
	    (pager->slots[index].backing_object != (vm_object_t) -1 ||
	    pager->slots[index].backing_offset != (vm_object_offset_t) -1)) {
		return KERN_INVALID_ADDRESS;
	}

	*old_backing_object = pager->slots[index].backing_object;
	*old_backing_offset = pager->slots[index].backing_offset;

	pager->slots[index].backing_object = new_backing_object;
	pager->slots[index].backing_offset = new_backing_offset;

	return KERN_SUCCESS;
}
