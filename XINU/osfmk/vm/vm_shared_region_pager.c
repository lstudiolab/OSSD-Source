/*
 * Copyright (c) 2018-2020 Apple Inc. All rights reserved.
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
#include <kern/queue.h>
#include <kern/thread.h>
#include <kern/ipc_kobject.h>

#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>

#include <vm/memory_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>
#include <vm/vm_shared_region.h>

#if __has_feature(ptrauth_calls)
#include <ptrauth.h>
extern boolean_t diversify_user_jop;
#endif                                   

  
                             
  
                                                                             
                                                                    
                         
  
                                                                            
                                                                         
                                                                             
  
                                                                          
                                                                            
                                                                           
  
                                                                          
                                                                       
   

                          
void shared_region_pager_reference(memory_object_t mem_obj);
void shared_region_pager_deallocate(memory_object_t mem_obj);
kern_return_t shared_region_pager_init(memory_object_t mem_obj,
    memory_object_control_t control,
    memory_object_cluster_size_t pg_size);
kern_return_t shared_region_pager_terminate(memory_object_t mem_obj);
kern_return_t shared_region_pager_data_request(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t length,
    vm_prot_t protection_required,
    memory_object_fault_info_t fault_info);
kern_return_t shared_region_pager_data_return(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t      data_cnt,
    memory_object_offset_t *resid_offset,
    int *io_error,
    boolean_t dirty,
    boolean_t kernel_copy,
    int upl_flags);
kern_return_t shared_region_pager_data_initialize(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t data_cnt);
kern_return_t shared_region_pager_data_unlock(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_size_t size,
    vm_prot_t desired_access);
kern_return_t shared_region_pager_synchronize(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_size_t length,
    vm_sync_t sync_flags);
kern_return_t shared_region_pager_map(memory_object_t mem_obj,
    vm_prot_t prot);
kern_return_t shared_region_pager_last_unmap(memory_object_t mem_obj);
boolean_t shared_region_pager_backing_object(
	memory_object_t mem_obj,
	memory_object_offset_t mem_obj_offset,
	vm_object_t *backing_object,
	vm_object_offset_t *backing_offset);

  
                                        
                                                                         
   
const struct memory_object_pager_ops shared_region_pager_ops = {
	.memory_object_reference = shared_region_pager_reference,
	.memory_object_deallocate = shared_region_pager_deallocate,
	.memory_object_init = shared_region_pager_init,
	.memory_object_terminate = shared_region_pager_terminate,
	.memory_object_data_request = shared_region_pager_data_request,
	.memory_object_data_return = shared_region_pager_data_return,
	.memory_object_data_initialize = shared_region_pager_data_initialize,
	.memory_object_data_unlock = shared_region_pager_data_unlock,
	.memory_object_synchronize = shared_region_pager_synchronize,
	.memory_object_map = shared_region_pager_map,
	.memory_object_last_unmap = shared_region_pager_last_unmap,
	.memory_object_data_reclaim = NULL,
	.memory_object_backing_object = shared_region_pager_backing_object,
	.memory_object_pager_name = "shared_region"
};

#if __has_feature(ptrauth_calls)
  
                                                                   
                          
   
typedef struct shared_region_jop_key_map {
	queue_chain_t  srk_queue;
	char           *srk_shared_region_id;
	uint64_t       srk_jop_key;
	os_refcnt_t    srk_ref_count;                                                               
} *shared_region_jop_key_map_t;

os_refgrp_decl(static, srk_refgrp, "shared region key ref cnts", NULL);

  
                                                             
   
int shared_region_key_count = 0;                                                          
queue_head_t shared_region_jop_key_queue = QUEUE_HEAD_INITIALIZER(shared_region_jop_key_queue);
LCK_GRP_DECLARE(shared_region_jop_key_lck_grp, "shared_region_jop_key");
LCK_MTX_DECLARE(shared_region_jop_key_lock, &shared_region_jop_key_lck_grp);

  
                                                              
   
uint64_t
shared_region_find_key(char *shared_region_id)
{
	shared_region_jop_key_map_t region;
	uint64_t key;

	lck_mtx_lock(&shared_region_jop_key_lock);
	queue_iterate(&shared_region_jop_key_queue, region, shared_region_jop_key_map_t, srk_queue) {
		if (strcmp(region->srk_shared_region_id, shared_region_id) == 0) {
			goto found;
		}
	}
	panic("shared_region_find_key() no key for region '%s'", shared_region_id);

found:
	key = region->srk_jop_key;
	lck_mtx_unlock(&shared_region_jop_key_lock);
	return key;
}

  
                                                                     
                                                             
                                                   
   
void
shared_region_key_alloc(char *shared_region_id, bool inherit, uint64_t inherited_key)
{
	shared_region_jop_key_map_t region;
	shared_region_jop_key_map_t new = NULL;

	assert(shared_region_id != NULL);
again:
	lck_mtx_lock(&shared_region_jop_key_lock);
	queue_iterate(&shared_region_jop_key_queue, region, shared_region_jop_key_map_t, srk_queue) {
		if (strcmp(region->srk_shared_region_id, shared_region_id) == 0) {
			os_ref_retain_locked(&region->srk_ref_count);
			goto done;
		}
	}

	  
                                                                            
    
	if (new == NULL) {
		lck_mtx_unlock(&shared_region_jop_key_lock);
		new = kalloc(sizeof *new);
		uint_t len = strlen(shared_region_id) + 1;
		new->srk_shared_region_id = kheap_alloc(KHEAP_DATA_BUFFERS, len, Z_WAITOK);
		strlcpy(new->srk_shared_region_id, shared_region_id, len);
		os_ref_init(&new->srk_ref_count, &srk_refgrp);

		if (diversify_user_jop && inherit) {
			new->srk_jop_key = inherited_key;
		} else if (diversify_user_jop && strlen(shared_region_id) > 0) {
			new->srk_jop_key = generate_jop_key();
		} else {
			new->srk_jop_key = ml_default_jop_pid();
		}

		goto again;
	}

	  
                                 
    
	++shared_region_key_count;
	queue_enter_first(&shared_region_jop_key_queue, new, shared_region_jop_key_map_t, srk_queue);
	region = new;
	new = NULL;

done:
	if (inherit && inherited_key != region->srk_jop_key) {
		panic("shared_region_key_alloc() inherited key mismatch");
	}
	lck_mtx_unlock(&shared_region_jop_key_lock);

	  
                             
    
	if (new != NULL) {
		kheap_free(KHEAP_DATA_BUFFERS, new->srk_shared_region_id, strlen(new->srk_shared_region_id) + 1);
		kfree(new, sizeof *new);
	}
}

  
                                                 
   
extern void
shared_region_key_dealloc(char *shared_region_id)
{
	shared_region_jop_key_map_t region;

	assert(shared_region_id != NULL);
	lck_mtx_lock(&shared_region_jop_key_lock);
	queue_iterate(&shared_region_jop_key_queue, region, shared_region_jop_key_map_t, srk_queue) {
		if (strcmp(region->srk_shared_region_id, shared_region_id) == 0) {
			goto done;
		}
	}
	panic("shared_region_key_dealloc() Shared region ID '%s' not found", shared_region_id);

done:
	if (os_ref_release_locked(&region->srk_ref_count) == 0) {
		queue_remove(&shared_region_jop_key_queue, region, shared_region_jop_key_map_t, srk_queue);
		--shared_region_key_count;
	} else {
		region = NULL;
	}
	lck_mtx_unlock(&shared_region_jop_key_lock);

	if (region != NULL) {
		kheap_free(KHEAP_DATA_BUFFERS, region->srk_shared_region_id, strlen(region->srk_shared_region_id) + 1);
		kfree(region, sizeof *region);
	}
}
#endif                                   

  
                                                                
                           
   
typedef struct shared_region_pager {
	struct memory_object    srp_header;                                        

	                         
	queue_chain_t           srp_queue;                                  
#if MEMORY_OBJECT_HAS_REFCOUNT
#define srp_ref_count           srp_header.mo_ref
#else
	os_ref_atomic_t         srp_ref_count;                       
#endif
	bool                    srp_is_mapped;                               
	bool                    srp_is_ready;                                 
	vm_object_t             srp_backing_object;                                 
	vm_object_offset_t      srp_backing_offset;
	vm_shared_region_slide_info_t srp_slide_info;
#if __has_feature(ptrauth_calls)
	uint64_t                srp_jop_key;                                    
#endif                                   
} *shared_region_pager_t;
#define SHARED_REGION_PAGER_NULL        ((shared_region_pager_t) NULL)

  
                                              
                                                                
   
int shared_region_pager_count = 0;                                    
int shared_region_pager_count_mapped = 0;                                      
queue_head_t shared_region_pager_queue = QUEUE_HEAD_INITIALIZER(shared_region_pager_queue);
LCK_GRP_DECLARE(shared_region_pager_lck_grp, "shared_region_pager");
LCK_MTX_DECLARE(shared_region_pager_lock, &shared_region_pager_lck_grp);

  
                                                                  
   
int shared_region_pager_cache_limit = 0;

  
                         
   
int shared_region_pager_count_max = 0;
int shared_region_pager_count_unmapped_max = 0;
int shared_region_pager_num_trim_max = 0;
int shared_region_pager_num_trim_total = 0;

uint64_t shared_region_pager_copied = 0;
uint64_t shared_region_pager_slid = 0;
uint64_t shared_region_pager_slid_error = 0;
uint64_t shared_region_pager_reclaimed = 0;

                         
shared_region_pager_t shared_region_pager_lookup(memory_object_t mem_obj);
void shared_region_pager_dequeue(shared_region_pager_t pager);
void shared_region_pager_deallocate_internal(shared_region_pager_t pager,
    boolean_t locked);
void shared_region_pager_terminate_internal(shared_region_pager_t pager);
void shared_region_pager_trim(void);


#if DEBUG
int shared_region_pagerdebug = 0;
#define PAGER_ALL               0xffffffff
#define PAGER_INIT              0x00000001
#define PAGER_PAGEIN            0x00000002

#define PAGER_DEBUG(LEVEL, A)                                           \
	MACRO_BEGIN                                                     \
	if ((shared_region_pagerdebug & (LEVEL)) == (LEVEL)) {          \
	        printf A;                                               \
	}                                                               \
	MACRO_END
#else
#define PAGER_DEBUG(LEVEL, A)
#endif

  
                             
  
                                                                         
   
kern_return_t
shared_region_pager_init(
	memory_object_t         mem_obj,
	memory_object_control_t control,
#if !DEBUG
	__unused
#endif
	memory_object_cluster_size_t pg_size)
{
	shared_region_pager_t   pager;
	kern_return_t           kr;
	memory_object_attr_info_data_t  attributes;

	PAGER_DEBUG(PAGER_ALL,
	    ("shared_region_pager_init: %p, %p, %x\n",
	    mem_obj, control, pg_size));

	if (control == MEMORY_OBJECT_CONTROL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	pager = shared_region_pager_lookup(mem_obj);

	memory_object_control_reference(control);

	pager->srp_header.mo_control = control;

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
		panic("shared_region_pager_init: "
		    "memory_object_change_attributes() failed");
	}

#if CONFIG_SECLUDED_MEMORY
	if (secluded_for_filecache) {
#if 00
		  
                                                   
                                                           
                                                              
                            
     
		memory_object_mark_eligible_for_secluded(control, TRUE);
#endif         
	}
#endif                             

	return KERN_SUCCESS;
}

  
                              
  
                                                                     
                                                                         
                                                                        
            
   
kern_return_t
shared_region_pager_data_return(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_cluster_size_t           data_cnt,
	__unused memory_object_offset_t *resid_offset,
	__unused int                    *io_error,
	__unused boolean_t              dirty,
	__unused boolean_t              kernel_copy,
	__unused int                    upl_flags)
{
	panic("shared_region_pager_data_return: should never get called");
	return KERN_FAILURE;
}

kern_return_t
shared_region_pager_data_initialize(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_cluster_size_t           data_cnt)
{
	panic("shared_region_pager_data_initialize: should never get called");
	return KERN_FAILURE;
}

kern_return_t
shared_region_pager_data_unlock(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_size_t           size,
	__unused vm_prot_t              desired_access)
{
	return KERN_FAILURE;
}

  
                                     
  
                                    
   
int shared_region_pager_data_request_debug = 0;
kern_return_t
shared_region_pager_data_request(
	memory_object_t         mem_obj,
	memory_object_offset_t  offset,
	memory_object_cluster_size_t            length,
#if !DEBUG
	__unused
#endif
	vm_prot_t               protection_required,
	memory_object_fault_info_t mo_fault_info)
{
	shared_region_pager_t   pager;
	memory_object_control_t mo_control;
	upl_t                   upl;
	int                     upl_flags;
	upl_size_t              upl_size;
	upl_page_info_t         *upl_pl;
	unsigned int            pl_count;
	vm_object_t             src_top_object, src_page_object, dst_object;
	kern_return_t           kr, retval;
	vm_offset_t             src_vaddr, dst_vaddr;
	vm_offset_t             cur_offset;
	vm_offset_t             offset_in_page;
	kern_return_t           error_code;
	vm_prot_t               prot;
	vm_page_t               src_page, top_page;
	int                     interruptible;
	struct vm_object_fault_info     fault_info;
	mach_vm_offset_t        slide_start_address;

	PAGER_DEBUG(PAGER_ALL, ("shared_region_pager_data_request: %p, %llx, %x, %x\n", mem_obj, offset, length, protection_required));

	retval = KERN_SUCCESS;
	src_top_object = VM_OBJECT_NULL;
	src_page_object = VM_OBJECT_NULL;
	upl = NULL;
	upl_pl = NULL;
	fault_info = *((struct vm_object_fault_info *)(uintptr_t)mo_fault_info);
	fault_info.stealth = TRUE;
	fault_info.io_sync = FALSE;
	fault_info.mark_zf_absent = FALSE;
	fault_info.batch_pmap_op = FALSE;
	interruptible = fault_info.interruptible;

	pager = shared_region_pager_lookup(mem_obj);
	assert(pager->srp_is_ready);
	assert(os_ref_get_count_raw(&pager->srp_ref_count) > 1);                     
	assert(pager->srp_is_mapped);                      

	PAGER_DEBUG(PAGER_PAGEIN, ("shared_region_pager_data_request: %p, %llx, %x, %x, pager %p\n", mem_obj, offset, length, protection_required, pager));

	  
                                                     
    
	mo_control = pager->srp_header.mo_control;

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
	    &upl, NULL, NULL, upl_flags, VM_KERN_MEMORY_SECURITY);
	if (kr != KERN_SUCCESS) {
		retval = kr;
		goto done;
	}
	dst_object = memory_object_control_to_vm_object(mo_control);
	assert(dst_object != VM_OBJECT_NULL);

	  
                                                                    
                                                                 
                     
    
	src_top_object = pager->srp_backing_object;
	assert(src_top_object != VM_OBJECT_NULL);
	vm_object_reference(src_top_object);                                   

	slide_start_address = pager->srp_slide_info->si_slid_address;

	fault_info.lo_offset += pager->srp_backing_offset;
	fault_info.hi_offset += pager->srp_backing_offset;

	  
                                                      
    
	upl_pl = UPL_GET_INTERNAL_PAGE_LIST(upl);
	pl_count = length / PAGE_SIZE;
	for (cur_offset = 0;
	    retval == KERN_SUCCESS && cur_offset < length;
	    cur_offset += PAGE_SIZE) {
		ppnum_t dst_pnum;

		if (!upl_page_present(upl_pl, (int)(cur_offset / PAGE_SIZE))) {
			                                          
			continue;
		}

		  
                                                            
                           
                                                       
     
retry_src_fault:
		vm_object_lock(src_top_object);
		vm_object_paging_begin(src_top_object);
		error_code = 0;
		prot = VM_PROT_READ;
		src_page = VM_PAGE_NULL;
		kr = vm_fault_page(src_top_object,
		    pager->srp_backing_offset + offset + cur_offset,
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
			goto done;
		case VM_FAULT_SUCCESS_NO_VM_PAGE:
			                                  
			vm_object_paging_end(src_top_object);
			vm_object_unlock(src_top_object);
			OS_FALLTHROUGH;
		case VM_FAULT_MEMORY_ERROR:
			                             
			if (error_code) {
				retval = error_code;
			} else {
				retval = KERN_MEMORY_ERROR;
			}
			goto done;
		default:
			panic("shared_region_pager_data_request: "
			    "vm_fault_page() unexpected error 0x%x\n",
			    kr);
		}
		assert(src_page != VM_PAGE_NULL);
		assert(src_page->vmp_busy);

		if (src_page->vmp_q_state != VM_PAGE_ON_SPECULATIVE_Q) {
			vm_page_lockspin_queues();
			if (src_page->vmp_q_state != VM_PAGE_ON_SPECULATIVE_Q) {
				vm_page_speculate(src_page, FALSE);
			}
			vm_page_unlock_queues();
		}

		  
                                     
                                    
     
		dst_pnum = (ppnum_t)
		    upl_phys_page(upl_pl, (int)(cur_offset / PAGE_SIZE));
		assert(dst_pnum != 0);

		src_vaddr = (vm_map_offset_t)
		    phystokv((pmap_paddr_t)VM_PAGE_GET_PHYS_PAGE(src_page)
		        << PAGE_SHIFT);
		dst_vaddr = (vm_map_offset_t)
		    phystokv((pmap_paddr_t)dst_pnum << PAGE_SHIFT);
		src_page_object = VM_PAGE_OBJECT(src_page);

		  
                                  
     
		if (src_page_object->code_signed) {
			vm_page_validate_cs_mapped(
				src_page, PAGE_SIZE, 0,
				(const void *) src_vaddr);
		}
		  
                                                          
     
		UPL_SET_CS_VALIDATED(upl_pl, cur_offset / PAGE_SIZE,
		    src_page->vmp_cs_validated);
		UPL_SET_CS_TAINTED(upl_pl, cur_offset / PAGE_SIZE,
		    src_page->vmp_cs_tainted);
		UPL_SET_CS_NX(upl_pl, cur_offset / PAGE_SIZE,
		    src_page->vmp_cs_nx);

		  
                                                           
                                                           
                        
                                               
                                                               
                               
     
		assert(src_page->vmp_busy);
		assert(src_page_object->paging_in_progress > 0);
		vm_object_unlock(src_page_object);

		  
                                                     
                               
     
		for (offset_in_page = 0;
		    offset_in_page < PAGE_SIZE;
		    offset_in_page += PAGE_SIZE_FOR_SR_SLIDE) {
			vm_object_offset_t chunk_offset;
			vm_object_offset_t offset_in_backing_object;
			vm_object_offset_t offset_in_sliding_range;

			chunk_offset = offset + cur_offset + offset_in_page;

			bcopy((const char *)(src_vaddr +
			    offset_in_page),
			    (char *)(dst_vaddr + offset_in_page),
			    PAGE_SIZE_FOR_SR_SLIDE);

			offset_in_backing_object = (chunk_offset +
			    pager->srp_backing_offset);
			if ((offset_in_backing_object < pager->srp_slide_info->si_start) ||
			    (offset_in_backing_object >= pager->srp_slide_info->si_end)) {
				                                             
				shared_region_pager_copied++;
				continue;
			}

			offset_in_sliding_range = offset_in_backing_object - pager->srp_slide_info->si_start;
			kr = vm_shared_region_slide_page(pager->srp_slide_info,
			    dst_vaddr + offset_in_page,
			    (mach_vm_offset_t) (offset_in_sliding_range + slide_start_address),
			    (uint32_t) (offset_in_sliding_range / PAGE_SIZE_FOR_SR_SLIDE),
#if __has_feature(ptrauth_calls)
			    pager->srp_slide_info->si_ptrauth ? pager->srp_jop_key : 0
#else                                   
			    0
#endif                                   
			    );
			if (shared_region_pager_data_request_debug) {
				printf("shared_region_data_request"
				    "(%p,0x%llx+0x%llx+0x%04llx): 0x%llx "
				    "in sliding range [0x%llx:0x%llx]: "
				    "SLIDE offset 0x%llx="
				    "(0x%llx+0x%llx+0x%llx+0x%04llx)"
				    "[0x%016llx 0x%016llx] "
				    "code_signed=%d "
				    "cs_validated=%d "
				    "cs_tainted=%d "
				    "cs_nx=%d "
				    "kr=0x%x\n",
				    pager,
				    offset,
				    (uint64_t) cur_offset,
				    (uint64_t) offset_in_page,
				    chunk_offset,
				    pager->srp_slide_info->si_start,
				    pager->srp_slide_info->si_end,
				    (pager->srp_backing_offset +
				    offset +
				    cur_offset +
				    offset_in_page),
				    pager->srp_backing_offset,
				    offset,
				    (uint64_t) cur_offset,
				    (uint64_t) offset_in_page,
				    *(uint64_t *)(dst_vaddr + offset_in_page),
				    *(uint64_t *)(dst_vaddr + offset_in_page + 8),
				    src_page_object->code_signed,
				    src_page->vmp_cs_validated,
				    src_page->vmp_cs_tainted,
				    src_page->vmp_cs_nx,
				    kr);
			}
			if (kr != KERN_SUCCESS) {
				shared_region_pager_slid_error++;
				break;
			}
			shared_region_pager_slid++;
		}

		assert(VM_PAGE_OBJECT(src_page) == src_page_object);
		assert(src_page->vmp_busy);
		assert(src_page_object->paging_in_progress > 0);
		vm_object_lock(src_page_object);

		  
                                                              
     
		PAGE_WAKEUP_DONE(src_page);
		src_page = VM_PAGE_NULL;
		vm_object_paging_end(src_page_object);
		vm_object_unlock(src_page_object);

		if (top_page != VM_PAGE_NULL) {
			assert(VM_PAGE_OBJECT(top_page) == src_top_object);
			vm_object_lock(src_top_object);
			VM_PAGE_FREE(top_page);
			vm_object_paging_end(src_top_object);
			vm_object_unlock(src_top_object);
		}
	}

done:
	if (upl != NULL) {
		                      

		  
                                                          
                                                            
                                                          
                               
                                       
     
		upl_clear_dirty(upl, TRUE);

		                             
		if (retval != KERN_SUCCESS) {
			upl_abort(upl, 0);
		} else {
			boolean_t empty;
			assertf(page_aligned(upl->u_offset) && page_aligned(upl->u_size),
			    "upl %p offset 0x%llx size 0x%x\n",
			    upl, upl->u_offset, upl->u_size);
			upl_commit_range(upl, 0, upl->u_size,
			    UPL_COMMIT_CS_VALIDATED | UPL_COMMIT_WRITTEN_BY_KERNEL,
			    upl_pl, pl_count, &empty);
		}

		                            
		upl_deallocate(upl);
		upl = NULL;
	}
	if (src_top_object != VM_OBJECT_NULL) {
		vm_object_deallocate(src_top_object);
	}
	return retval;
}

  
                                  
  
                                         
                                                                               
                                                     
   
void
shared_region_pager_reference(
	memory_object_t         mem_obj)
{
	shared_region_pager_t   pager;

	pager = shared_region_pager_lookup(mem_obj);

	lck_mtx_lock(&shared_region_pager_lock);
	os_ref_retain_locked_raw(&pager->srp_ref_count, NULL);
	lck_mtx_unlock(&shared_region_pager_lock);
}


  
                               
  
                                           
  
                                                   
   
void
shared_region_pager_dequeue(
	shared_region_pager_t pager)
{
	assert(!pager->srp_is_mapped);

	queue_remove(&shared_region_pager_queue,
	    pager,
	    shared_region_pager_t,
	    srp_queue);
	pager->srp_queue.next = NULL;
	pager->srp_queue.prev = NULL;

	shared_region_pager_count--;
}

  
                                          
  
                                                                       
                   
                                                                    
                                                                        
                          
  
                                                               
                                                                         
                                                
   
void
shared_region_pager_terminate_internal(
	shared_region_pager_t pager)
{
	assert(pager->srp_is_ready);
	assert(!pager->srp_is_mapped);
	assert(os_ref_get_count_raw(&pager->srp_ref_count) == 1);

	if (pager->srp_backing_object != VM_OBJECT_NULL) {
		vm_object_deallocate(pager->srp_backing_object);
		pager->srp_backing_object = VM_OBJECT_NULL;
	}
	                                                  
	memory_object_destroy(pager->srp_header.mo_control, 0);
}

  
                                            
  
                                                                                   
                                                                              
                    
   
void
shared_region_pager_deallocate_internal(
	shared_region_pager_t   pager,
	boolean_t               locked)
{
	boolean_t       needs_trimming;
	int             count_unmapped;
	os_ref_count_t  ref_count;

	if (!locked) {
		lck_mtx_lock(&shared_region_pager_lock);
	}

	                                                    
	count_unmapped = shared_region_pager_count - shared_region_pager_count_mapped;
	needs_trimming = (count_unmapped > shared_region_pager_cache_limit);

	                                    
	ref_count = os_ref_release_locked_raw(&pager->srp_ref_count, NULL);

	if (ref_count == 1) {
		  
                                                         
                                                       
                  
     
		shared_region_pager_dequeue(pager);
		                                                     
		lck_mtx_unlock(&shared_region_pager_lock);
		shared_region_pager_terminate_internal(pager);
	} else if (ref_count == 0) {
		  
                                                            
                                                            
                     
     
		lck_mtx_unlock(&shared_region_pager_lock);

		vm_shared_region_slide_info_t si = pager->srp_slide_info;
#if __has_feature(ptrauth_calls)
		  
                                                                 
                                                                
     
		if (si != NULL) {
			if (si->si_shared_region != NULL) {
				assert(si->si_ptrauth);
				vm_shared_region_deallocate(si->si_shared_region);
				pager->srp_slide_info = NULL;
				si = NULL;
			}
		}
#endif                                   
		if (si != NULL) {
			vm_object_deallocate(si->si_slide_object);
			                               
			kheap_free(KHEAP_DATA_BUFFERS, si->si_slide_info_entry, si->si_slide_info_size);
			kfree(si, sizeof *si);
			pager->srp_slide_info = NULL;
		}

		if (pager->srp_header.mo_control != MEMORY_OBJECT_CONTROL_NULL) {
			memory_object_control_deallocate(pager->srp_header.mo_control);
			pager->srp_header.mo_control = MEMORY_OBJECT_CONTROL_NULL;
		}
		kfree(pager, sizeof(*pager));
		pager = SHARED_REGION_PAGER_NULL;
	} else {
		                                                          
		lck_mtx_unlock(&shared_region_pager_lock);
	}

	if (needs_trimming) {
		shared_region_pager_trim();
	}
	                                            
}

  
                                   
  
                                                              
                       
   
void
shared_region_pager_deallocate(
	memory_object_t         mem_obj)
{
	shared_region_pager_t   pager;

	PAGER_DEBUG(PAGER_ALL, ("shared_region_pager_deallocate: %p\n", mem_obj));
	pager = shared_region_pager_lookup(mem_obj);
	shared_region_pager_deallocate_internal(pager, FALSE);
}

  
  
   
kern_return_t
shared_region_pager_terminate(
#if !DEBUG
	__unused
#endif
	memory_object_t mem_obj)
{
	PAGER_DEBUG(PAGER_ALL, ("shared_region_pager_terminate: %p\n", mem_obj));

	return KERN_SUCCESS;
}

  
  
   
kern_return_t
shared_region_pager_synchronize(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_size_t   length,
	__unused vm_sync_t              sync_flags)
{
	panic("shared_region_pager_synchronize: memory_object_synchronize no longer supported\n");
	return KERN_FAILURE;
}

  
                            
  
                                                                  
                                                                         
                                                                          
                           
   
kern_return_t
shared_region_pager_map(
	memory_object_t         mem_obj,
	__unused vm_prot_t      prot)
{
	shared_region_pager_t   pager;

	PAGER_DEBUG(PAGER_ALL, ("shared_region_pager_map: %p\n", mem_obj));

	pager = shared_region_pager_lookup(mem_obj);

	lck_mtx_lock(&shared_region_pager_lock);
	assert(pager->srp_is_ready);
	assert(os_ref_get_count_raw(&pager->srp_ref_count) > 0);                     
	if (!pager->srp_is_mapped) {
		pager->srp_is_mapped = TRUE;
		os_ref_retain_locked_raw(&pager->srp_ref_count, NULL);
		shared_region_pager_count_mapped++;
	}
	lck_mtx_unlock(&shared_region_pager_lock);

	return KERN_SUCCESS;
}

  
                                   
  
                                                                             
   
kern_return_t
shared_region_pager_last_unmap(
	memory_object_t         mem_obj)
{
	shared_region_pager_t   pager;
	int                     count_unmapped;

	PAGER_DEBUG(PAGER_ALL,
	    ("shared_region_pager_last_unmap: %p\n", mem_obj));

	pager = shared_region_pager_lookup(mem_obj);

	lck_mtx_lock(&shared_region_pager_lock);
	if (pager->srp_is_mapped) {
		  
                                                          
                                                              
     
		shared_region_pager_count_mapped--;
		count_unmapped = (shared_region_pager_count - shared_region_pager_count_mapped);
		if (count_unmapped > shared_region_pager_count_unmapped_max) {
			shared_region_pager_count_unmapped_max = count_unmapped;
		}
		pager->srp_is_mapped = FALSE;
		shared_region_pager_deallocate_internal(pager, TRUE);
		                                                        
	} else {
		lck_mtx_unlock(&shared_region_pager_lock);
	}

	return KERN_SUCCESS;
}

boolean_t
shared_region_pager_backing_object(
	memory_object_t         mem_obj,
	memory_object_offset_t  offset,
	vm_object_t             *backing_object,
	vm_object_offset_t      *backing_offset)
{
	shared_region_pager_t   pager;

	PAGER_DEBUG(PAGER_ALL,
	    ("shared_region_pager_backing_object: %p\n", mem_obj));

	pager = shared_region_pager_lookup(mem_obj);

	*backing_object = pager->srp_backing_object;
	*backing_offset = pager->srp_backing_offset + offset;

	return TRUE;
}


  
  
   
shared_region_pager_t
shared_region_pager_lookup(
	memory_object_t  mem_obj)
{
	shared_region_pager_t   pager;

	assert(mem_obj->mo_pager_ops == &shared_region_pager_ops);
	pager = (shared_region_pager_t)(uintptr_t) mem_obj;
	assert(os_ref_get_count_raw(&pager->srp_ref_count) > 0);
	return pager;
}

  
                                                          
                           
   
static shared_region_pager_t
shared_region_pager_create(
	vm_object_t             backing_object,
	vm_object_offset_t      backing_offset,
	struct vm_shared_region_slide_info *slide_info,
#if !__has_feature(ptrauth_calls)
	__unused
#endif                                    
	uint64_t                jop_key)
{
	shared_region_pager_t   pager;
	memory_object_control_t control;
	kern_return_t           kr;
	vm_object_t             object;

	pager = (shared_region_pager_t) kalloc(sizeof(*pager));
	if (pager == SHARED_REGION_PAGER_NULL) {
		return SHARED_REGION_PAGER_NULL;
	}

	  
                                                               
                                                             
                                                               
                                                                
                                                                 
    
	pager->srp_header.mo_ikot = IKOT_MEMORY_OBJECT;
	pager->srp_header.mo_pager_ops = &shared_region_pager_ops;
	pager->srp_header.mo_control = MEMORY_OBJECT_CONTROL_NULL;

	pager->srp_is_ready = FALSE;                                     
	                                                            
	os_ref_init_count_raw(&pager->srp_ref_count, NULL, 2);
	pager->srp_is_mapped = FALSE;
	pager->srp_backing_object = backing_object;
	pager->srp_backing_offset = backing_offset;
	pager->srp_slide_info = slide_info;
#if __has_feature(ptrauth_calls)
	pager->srp_jop_key = jop_key;
	  
                                                       
                                                          
    
	if (slide_info->si_shared_region) {
		assert(slide_info->si_ptrauth);
		vm_shared_region_reference(slide_info->si_shared_region);
	}
#endif                                   

	vm_object_reference(backing_object);

	lck_mtx_lock(&shared_region_pager_lock);
	                                                       
	queue_enter_first(&shared_region_pager_queue,
	    pager,
	    shared_region_pager_t,
	    srp_queue);
	shared_region_pager_count++;
	if (shared_region_pager_count > shared_region_pager_count_max) {
		shared_region_pager_count_max = shared_region_pager_count;
	}
	lck_mtx_unlock(&shared_region_pager_lock);

	kr = memory_object_create_named((memory_object_t) pager,
	    0,
	    &control);
	assert(kr == KERN_SUCCESS);

	memory_object_mark_trusted(control);

	lck_mtx_lock(&shared_region_pager_lock);
	                                           
	pager->srp_is_ready = TRUE;
	object = memory_object_to_vm_object((memory_object_t) pager);
	assert(object);
	  
                                                                              
                                                                              
                                               
    
	object->object_is_shared_cache = TRUE;
	lck_mtx_unlock(&shared_region_pager_lock);

	                                                      
	thread_wakeup(&pager->srp_is_ready);

	return pager;
}

  
                              
  
                                                                 
                              
   
memory_object_t
shared_region_pager_setup(
	vm_object_t             backing_object,
	vm_object_offset_t      backing_offset,
	struct vm_shared_region_slide_info *slide_info,
	uint64_t                jop_key)
{
	shared_region_pager_t   pager;

	                      
	pager = shared_region_pager_create(backing_object,
	    backing_offset, slide_info, jop_key);
	if (pager == SHARED_REGION_PAGER_NULL) {
		                                  
		return MEMORY_OBJECT_NULL;
	}

	lck_mtx_lock(&shared_region_pager_lock);
	while (!pager->srp_is_ready) {
		lck_mtx_sleep(&shared_region_pager_lock,
		    LCK_SLEEP_DEFAULT,
		    &pager->srp_is_ready,
		    THREAD_UNINT);
	}
	lck_mtx_unlock(&shared_region_pager_lock);

	return (memory_object_t) pager;
}

#if __has_feature(ptrauth_calls)
  
                              
  
                                                                 
                              
   
memory_object_t
shared_region_pager_match(
	vm_object_t                   backing_object,
	vm_object_offset_t            backing_offset,
	vm_shared_region_slide_info_t slide_info,
	uint64_t                      jop_key)
{
	shared_region_pager_t         pager;
	vm_shared_region_slide_info_t si;

	lck_mtx_lock(&shared_region_pager_lock);
	queue_iterate(&shared_region_pager_queue, pager, shared_region_pager_t, srp_queue) {
		if (pager->srp_backing_object != backing_object->copy) {
			continue;
		}
		if (pager->srp_backing_offset != backing_offset) {
			continue;
		}
		si = pager->srp_slide_info;

		                                                                                 
		if (!si->si_ptrauth) {
			continue;
		}
		if (pager->srp_jop_key != jop_key) {
			continue;
		}
		if (si->si_slide != slide_info->si_slide) {
			continue;
		}
		if (si->si_start != slide_info->si_start) {
			continue;
		}
		if (si->si_end != slide_info->si_end) {
			continue;
		}
		if (si->si_slide_object != slide_info->si_slide_object) {
			continue;
		}
		if (si->si_slide_info_size != slide_info->si_slide_info_size) {
			continue;
		}
		if (memcmp(si->si_slide_info_entry, slide_info->si_slide_info_entry, si->si_slide_info_size) != 0) {
			continue;
		}
		                                            
		os_ref_retain_locked_raw(&pager->srp_ref_count, NULL);
		lck_mtx_unlock(&shared_region_pager_lock);
		return (memory_object_t)pager;
	}

	  
                                                       
   
                                                                                            
                                                                                          
                          
    
	lck_mtx_unlock(&shared_region_pager_lock);
	return shared_region_pager_setup(backing_object->copy, backing_offset, slide_info, jop_key);
}

void
shared_region_pager_match_task_key(memory_object_t memobj, __unused task_t task)
{
	__unused shared_region_pager_t  pager = (shared_region_pager_t)memobj;

	assert(pager->srp_jop_key == task->jop_pid);
}
#endif                                   

void
shared_region_pager_trim(void)
{
	shared_region_pager_t   pager, prev_pager;
	queue_head_t            trim_queue;
	int                     num_trim;
	int                     count_unmapped;

	lck_mtx_lock(&shared_region_pager_lock);

	  
                                                           
                                                           
    
	queue_init(&trim_queue);
	num_trim = 0;

	for (pager = (shared_region_pager_t)queue_last(&shared_region_pager_queue);
	    !queue_end(&shared_region_pager_queue, (queue_entry_t) pager);
	    pager = prev_pager) {
		                                    
		prev_pager = (shared_region_pager_t)queue_prev(&pager->srp_queue);

		if (os_ref_get_count_raw(&pager->srp_ref_count) == 2 &&
		    pager->srp_is_ready &&
		    !pager->srp_is_mapped) {
			                               
			num_trim++;
			                                              
			shared_region_pager_dequeue(pager);
			                                      
			queue_enter_first(&trim_queue,
			    pager,
			    shared_region_pager_t,
			    srp_queue);

			                                       
			count_unmapped = (shared_region_pager_count - shared_region_pager_count_mapped);
			if (count_unmapped <= shared_region_pager_cache_limit) {
				break;
			}
		}
	}
	if (num_trim > shared_region_pager_num_trim_max) {
		shared_region_pager_num_trim_max = num_trim;
	}
	shared_region_pager_num_trim_total += num_trim;

	lck_mtx_unlock(&shared_region_pager_lock);

	                                  
	while (!queue_empty(&trim_queue)) {
		queue_remove_first(&trim_queue,
		    pager,
		    shared_region_pager_t,
		    srp_queue);
		pager->srp_queue.next = NULL;
		pager->srp_queue.prev = NULL;
		assert(os_ref_get_count_raw(&pager->srp_ref_count) == 2);
		  
                                                          
                                                           
                 
     
		(void)os_ref_release_locked_raw(&pager->srp_ref_count, NULL);
		shared_region_pager_terminate_internal(pager);
	}
}
