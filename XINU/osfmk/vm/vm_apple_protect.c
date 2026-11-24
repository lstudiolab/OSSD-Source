/*
 * Copyright (c) 2006-2020 Apple Inc. All rights reserved.
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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
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
#include <os/refcnt.h>

#include <ipc/ipc_port.h>
#include <ipc/ipc_space.h>

#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_pageout.h>
#include <vm/memory_object.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>
#include <vm/vm_kern.h>

  
                             
  
                                                                       
                                                                        
  
                                                                            
                                                                          
                                                            
  
                                                                           
                                                                            
                                                                           
  
                                                                      
                                                                        
           
   

                          
void apple_protect_pager_reference(memory_object_t mem_obj);
void apple_protect_pager_deallocate(memory_object_t mem_obj);
kern_return_t apple_protect_pager_init(memory_object_t mem_obj,
    memory_object_control_t control,
    memory_object_cluster_size_t pg_size);
kern_return_t apple_protect_pager_terminate(memory_object_t mem_obj);
kern_return_t apple_protect_pager_data_request(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t length,
    vm_prot_t protection_required,
    memory_object_fault_info_t fault_info);
kern_return_t apple_protect_pager_data_return(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t      data_cnt,
    memory_object_offset_t *resid_offset,
    int *io_error,
    boolean_t dirty,
    boolean_t kernel_copy,
    int upl_flags);
kern_return_t apple_protect_pager_data_initialize(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t data_cnt);
kern_return_t apple_protect_pager_data_unlock(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_size_t size,
    vm_prot_t desired_access);
kern_return_t apple_protect_pager_synchronize(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_size_t length,
    vm_sync_t sync_flags);
kern_return_t apple_protect_pager_map(memory_object_t mem_obj,
    vm_prot_t prot);
kern_return_t apple_protect_pager_last_unmap(memory_object_t mem_obj);
boolean_t apple_protect_pager_backing_object(
	memory_object_t mem_obj,
	memory_object_offset_t mem_obj_offset,
	vm_object_t *backing_object,
	vm_object_offset_t *backing_offset);

#define CRYPT_INFO_DEBUG 0
void crypt_info_reference(struct pager_crypt_info *crypt_info);
void crypt_info_deallocate(struct pager_crypt_info *crypt_info);

  
                                        
                                                                         
   
const struct memory_object_pager_ops apple_protect_pager_ops = {
	.memory_object_reference = apple_protect_pager_reference,
	.memory_object_deallocate = apple_protect_pager_deallocate,
	.memory_object_init = apple_protect_pager_init,
	.memory_object_terminate = apple_protect_pager_terminate,
	.memory_object_data_request = apple_protect_pager_data_request,
	.memory_object_data_return = apple_protect_pager_data_return,
	.memory_object_data_initialize = apple_protect_pager_data_initialize,
	.memory_object_data_unlock = apple_protect_pager_data_unlock,
	.memory_object_synchronize = apple_protect_pager_synchronize,
	.memory_object_map = apple_protect_pager_map,
	.memory_object_last_unmap = apple_protect_pager_last_unmap,
	.memory_object_data_reclaim = NULL,
	.memory_object_backing_object = apple_protect_pager_backing_object,
	.memory_object_pager_name = "apple_protect"
};

  
                                                                
                           
   
typedef struct apple_protect_pager {
	                              
	struct memory_object    ap_pgr_hdr;

	                         
	queue_chain_t           pager_queue;                            
#if MEMORY_OBJECT_HAS_REFCOUNT
#define ap_pgr_hdr_ref          ap_pgr_hdr.mo_ref
#else
	os_ref_atomic_t         ap_pgr_hdr_ref;                           
#endif
	bool                    is_ready;                                  
	bool                    is_mapped;                                    
	bool                    is_cached;                                  
	vm_object_t             backing_object;                               
	vm_object_offset_t      backing_offset;
	vm_object_offset_t      crypto_backing_offset;                 
	vm_object_offset_t      crypto_start;
	vm_object_offset_t      crypto_end;
	struct pager_crypt_info *crypt_info;
} *apple_protect_pager_t;
#define APPLE_PROTECT_PAGER_NULL        ((apple_protect_pager_t) NULL)

  
                                              
                                                                
   
unsigned int apple_protect_pager_count = 0;                              
unsigned int apple_protect_pager_count_mapped = 0;                                
queue_head_t apple_protect_pager_queue = QUEUE_HEAD_INITIALIZER(apple_protect_pager_queue);
LCK_GRP_DECLARE(apple_protect_pager_lck_grp, "apple_protect");
LCK_MTX_DECLARE(apple_protect_pager_lock, &apple_protect_pager_lck_grp);

  
                                                                  
   
unsigned int apple_protect_pager_cache_limit = 20;

  
                         
   
unsigned int apple_protect_pager_count_max = 0;
unsigned int apple_protect_pager_count_unmapped_max = 0;
unsigned int apple_protect_pager_num_trim_max = 0;
unsigned int apple_protect_pager_num_trim_total = 0;



                         
apple_protect_pager_t apple_protect_pager_create(
	vm_object_t backing_object,
	vm_object_offset_t backing_offset,
	vm_object_offset_t crypto_backing_offset,
	struct pager_crypt_info *crypt_info,
	vm_object_offset_t crypto_start,
	vm_object_offset_t crypto_end,
	boolean_t cache_pager);
apple_protect_pager_t apple_protect_pager_lookup(memory_object_t mem_obj);
void apple_protect_pager_dequeue(apple_protect_pager_t pager);
void apple_protect_pager_deallocate_internal(apple_protect_pager_t pager,
    boolean_t locked);
void apple_protect_pager_terminate_internal(apple_protect_pager_t pager);
void apple_protect_pager_trim(void);


#if DEBUG
int apple_protect_pagerdebug = 0;
#define PAGER_ALL               0xffffffff
#define PAGER_INIT              0x00000001
#define PAGER_PAGEIN            0x00000002

#define PAGER_DEBUG(LEVEL, A)                                           \
	MACRO_BEGIN                                                     \
	if ((apple_protect_pagerdebug & LEVEL)==LEVEL) {                \
	        printf A;                                               \
	}                                                               \
	MACRO_END
#else
#define PAGER_DEBUG(LEVEL, A)
#endif

  
                             
  
                                                                         
   
kern_return_t
apple_protect_pager_init(
	memory_object_t         mem_obj,
	memory_object_control_t control,
#if !DEBUG
	__unused
#endif
	memory_object_cluster_size_t pg_size)
{
	apple_protect_pager_t   pager;
	kern_return_t           kr;
	memory_object_attr_info_data_t  attributes;

	PAGER_DEBUG(PAGER_ALL,
	    ("apple_protect_pager_init: %p, %p, %x\n",
	    mem_obj, control, pg_size));

	if (control == MEMORY_OBJECT_CONTROL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	pager = apple_protect_pager_lookup(mem_obj);

	memory_object_control_reference(control);

	pager->ap_pgr_hdr.mo_control = control;

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
		panic("apple_protect_pager_init: "
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
apple_protect_pager_data_return(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_cluster_size_t           data_cnt,
	__unused memory_object_offset_t *resid_offset,
	__unused int                    *io_error,
	__unused boolean_t              dirty,
	__unused boolean_t              kernel_copy,
	__unused int                    upl_flags)
{
	panic("apple_protect_pager_data_return: should never get called");
	return KERN_FAILURE;
}

kern_return_t
apple_protect_pager_data_initialize(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_cluster_size_t           data_cnt)
{
	panic("apple_protect_pager_data_initialize: should never get called");
	return KERN_FAILURE;
}

kern_return_t
apple_protect_pager_data_unlock(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_size_t           size,
	__unused vm_prot_t              desired_access)
{
	return KERN_FAILURE;
}

  
                                     
  
                                    
   
int apple_protect_pager_data_request_debug = 0;
kern_return_t
apple_protect_pager_data_request(
	memory_object_t         mem_obj,
	memory_object_offset_t  offset,
	memory_object_cluster_size_t            length,
#if !DEBUG
	__unused
#endif
	vm_prot_t               protection_required,
	memory_object_fault_info_t mo_fault_info)
{
	apple_protect_pager_t   pager;
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
	int                     ret;

	PAGER_DEBUG(PAGER_ALL, ("apple_protect_pager_data_request: %p, %llx, %x, %x\n", mem_obj, offset, length, protection_required));

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

	pager = apple_protect_pager_lookup(mem_obj);
	assert(pager->is_ready);
	assert(os_ref_get_count_raw(&pager->ap_pgr_hdr_ref) > 1);                                

	PAGER_DEBUG(PAGER_PAGEIN, ("apple_protect_pager_data_request: %p, %llx, %x, %x, pager %p\n", mem_obj, offset, length, protection_required, pager));

	fault_info.lo_offset += pager->backing_offset;
	fault_info.hi_offset += pager->backing_offset;

	  
                                                     
    
	mo_control = pager->ap_pgr_hdr.mo_control;

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

	  
                                                                     
                                                              
                     
    
	src_top_object = pager->backing_object;
	assert(src_top_object != VM_OBJECT_NULL);
	vm_object_reference(src_top_object);                                   

	  
                                                      
    
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
		    pager->backing_offset + offset + cur_offset,
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
			panic("apple_protect_pager_data_request: "
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
		    offset_in_page += 4096) {
			if (offset + cur_offset + offset_in_page <
			    pager->crypto_start ||
			    offset + cur_offset + offset_in_page >=
			    pager->crypto_end) {
				                              
				bcopy((const char *)(src_vaddr +
				    offset_in_page),
				    (char *)(dst_vaddr + offset_in_page),
				    4096);

				if (apple_protect_pager_data_request_debug) {
					printf("apple_protect_data_request"
					    "(%p,0x%llx+0x%llx+0x%04llx): "
					    "out of crypto range "
					    "[0x%llx:0x%llx]: "
					    "COPY [0x%016llx 0x%016llx] "
					    "code_signed=%d "
					    "cs_validated=%d "
					    "cs_tainted=%d "
					    "cs_nx=%d\n",
					    pager,
					    offset,
					    (uint64_t) cur_offset,
					    (uint64_t) offset_in_page,
					    pager->crypto_start,
					    pager->crypto_end,
					    *(uint64_t *)(dst_vaddr +
					    offset_in_page),
					    *(uint64_t *)(dst_vaddr +
					    offset_in_page + 8),
					    src_page_object->code_signed,
					    src_page->vmp_cs_validated,
					    src_page->vmp_cs_tainted,
					    src_page->vmp_cs_nx);
				}
				ret = 0;
				continue;
			}
			ret = pager->crypt_info->page_decrypt(
				(const void *)(src_vaddr + offset_in_page),
				(void *)(dst_vaddr + offset_in_page),
				((pager->crypto_backing_offset -
				pager->crypto_start) +              
				offset +
				cur_offset +
				offset_in_page),
				pager->crypt_info->crypt_ops);

			if (apple_protect_pager_data_request_debug) {
				printf("apple_protect_data_request"
				    "(%p,0x%llx+0x%llx+0x%04llx): "
				    "in crypto range [0x%llx:0x%llx]: "
				    "DECRYPT offset 0x%llx="
				    "(0x%llx-0x%llx+0x%llx+0x%llx+0x%04llx)"
				    "[0x%016llx 0x%016llx] "
				    "code_signed=%d "
				    "cs_validated=%d "
				    "cs_tainted=%d "
				    "cs_nx=%d "
				    "ret=0x%x\n",
				    pager,
				    offset,
				    (uint64_t) cur_offset,
				    (uint64_t) offset_in_page,
				    pager->crypto_start, pager->crypto_end,
				    ((pager->crypto_backing_offset -
				    pager->crypto_start) +
				    offset +
				    cur_offset +
				    offset_in_page),
				    pager->crypto_backing_offset,
				    pager->crypto_start,
				    offset,
				    (uint64_t) cur_offset,
				    (uint64_t) offset_in_page,
				    *(uint64_t *)(dst_vaddr + offset_in_page),
				    *(uint64_t *)(dst_vaddr + offset_in_page + 8),
				    src_page_object->code_signed,
				    src_page->vmp_cs_validated,
				    src_page->vmp_cs_tainted,
				    src_page->vmp_cs_nx,
				    ret);
			}
			if (ret) {
				break;
			}
		}
		if (ret) {
			  
                                          
      
			retval = KERN_ABORTED;
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
			if (retval == KERN_ABORTED) {
				wait_result_t   wait_result;

				  
                                               
                                               
                                               
                                              
                                                
                                                
                                           
       
				retval = KERN_SUCCESS;
				  
                                             
                                             
                                          
       
				wait_result = assert_wait_timeout(
					(event_t) apple_protect_pager_data_request,
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
	if (src_top_object != VM_OBJECT_NULL) {
		vm_object_deallocate(src_top_object);
	}
	return retval;
}

  
                                  
  
                                         
                                                                               
                                                     
   
void
apple_protect_pager_reference(
	memory_object_t         mem_obj)
{
	apple_protect_pager_t   pager;

	pager = apple_protect_pager_lookup(mem_obj);

	lck_mtx_lock(&apple_protect_pager_lock);
	os_ref_retain_locked_raw(&pager->ap_pgr_hdr_ref, NULL);
	lck_mtx_unlock(&apple_protect_pager_lock);
}


  
                               
  
                                           
  
                                                   
   
void
apple_protect_pager_dequeue(
	apple_protect_pager_t pager)
{
	assert(!pager->is_mapped);

	queue_remove(&apple_protect_pager_queue,
	    pager,
	    apple_protect_pager_t,
	    pager_queue);
	pager->pager_queue.next = NULL;
	pager->pager_queue.prev = NULL;

	apple_protect_pager_count--;
}

  
                                          
  
                                                                       
                   
                                                                    
                                                                        
                          
  
                                                               
                                                                         
                                                
   
void
apple_protect_pager_terminate_internal(
	apple_protect_pager_t pager)
{
	assert(pager->is_ready);
	assert(!pager->is_mapped);

	if (pager->backing_object != VM_OBJECT_NULL) {
		vm_object_deallocate(pager->backing_object);
		pager->backing_object = VM_OBJECT_NULL;
	}

	                                                  
#if CRYPT_INFO_DEBUG
	printf("CRYPT_INFO %s: deallocate %p ref %d\n",
	    __FUNCTION__,
	    pager->crypt_info,
	    pager->crypt_info->crypt_refcnt);
#endif                       
	crypt_info_deallocate(pager->crypt_info);
	pager->crypt_info = NULL;

	                                                  
	memory_object_destroy(pager->ap_pgr_hdr.mo_control, 0);
}

  
                                            
  
                                                              
                       
                                                                             
                    
   
void
apple_protect_pager_deallocate_internal(
	apple_protect_pager_t   pager,
	boolean_t               locked)
{
	boolean_t       needs_trimming;
	unsigned int    count_unmapped;
	os_ref_count_t  ref_count;

	if (!locked) {
		lck_mtx_lock(&apple_protect_pager_lock);
	}

	count_unmapped = (apple_protect_pager_count -
	    apple_protect_pager_count_mapped);
	if (count_unmapped > apple_protect_pager_cache_limit) {
		                                                  
		needs_trimming = TRUE;
	} else {
		needs_trimming = FALSE;
	}

	                                    
	ref_count = os_ref_release_locked_raw(&pager->ap_pgr_hdr_ref, NULL);

	if (ref_count == 1) {
		  
                                                         
                                                       
                  
     
		apple_protect_pager_dequeue(pager);
		                                                     
		lck_mtx_unlock(&apple_protect_pager_lock);
		apple_protect_pager_terminate_internal(pager);
	} else if (ref_count == 0) {
		  
                                                            
                                                            
                     
     
		lck_mtx_unlock(&apple_protect_pager_lock);
		if (pager->ap_pgr_hdr.mo_control != MEMORY_OBJECT_CONTROL_NULL) {
			memory_object_control_deallocate(pager->ap_pgr_hdr.mo_control);
			pager->ap_pgr_hdr.mo_control = MEMORY_OBJECT_CONTROL_NULL;
		}
		kfree(pager, sizeof(*pager));
		pager = APPLE_PROTECT_PAGER_NULL;
	} else {
		                                                          
		lck_mtx_unlock(&apple_protect_pager_lock);
	}

	if (needs_trimming) {
		apple_protect_pager_trim();
	}
	                                            
}

  
                                   
  
                                                              
                       
   
void
apple_protect_pager_deallocate(
	memory_object_t         mem_obj)
{
	apple_protect_pager_t   pager;

	PAGER_DEBUG(PAGER_ALL, ("apple_protect_pager_deallocate: %p\n", mem_obj));
	pager = apple_protect_pager_lookup(mem_obj);
	apple_protect_pager_deallocate_internal(pager, FALSE);
}

  
  
   
kern_return_t
apple_protect_pager_terminate(
#if !DEBUG
	__unused
#endif
	memory_object_t mem_obj)
{
	PAGER_DEBUG(PAGER_ALL, ("apple_protect_pager_terminate: %p\n", mem_obj));

	return KERN_SUCCESS;
}

  
  
   
kern_return_t
apple_protect_pager_synchronize(
	__unused memory_object_t                mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_size_t           length,
	__unused vm_sync_t              sync_flags)
{
	panic("apple_protect_pager_synchronize: memory_object_synchronize no longer supported\n");
	return KERN_FAILURE;
}

  
                            
  
                                                                  
                                                                         
                                                                       
                                                 
   
kern_return_t
apple_protect_pager_map(
	memory_object_t         mem_obj,
	__unused vm_prot_t      prot)
{
	apple_protect_pager_t   pager;

	PAGER_DEBUG(PAGER_ALL, ("apple_protect_pager_map: %p\n", mem_obj));

	pager = apple_protect_pager_lookup(mem_obj);

	lck_mtx_lock(&apple_protect_pager_lock);
	assert(pager->is_ready);
	assert(os_ref_get_count_raw(&pager->ap_pgr_hdr_ref) > 0);                     
	if (pager->is_mapped == FALSE) {
		  
                                                          
                                                          
                 
     
		pager->is_mapped = TRUE;
		os_ref_retain_locked_raw(&pager->ap_pgr_hdr_ref, NULL);
		apple_protect_pager_count_mapped++;
	}
	lck_mtx_unlock(&apple_protect_pager_lock);

	return KERN_SUCCESS;
}

  
                                   
  
                                                                             
   
kern_return_t
apple_protect_pager_last_unmap(
	memory_object_t         mem_obj)
{
	apple_protect_pager_t   pager;
	unsigned int            count_unmapped;

	PAGER_DEBUG(PAGER_ALL,
	    ("apple_protect_pager_last_unmap: %p\n", mem_obj));

	pager = apple_protect_pager_lookup(mem_obj);

	lck_mtx_lock(&apple_protect_pager_lock);
	if (pager->is_mapped) {
		  
                                                          
                                                              
     
		apple_protect_pager_count_mapped--;
		count_unmapped = (apple_protect_pager_count -
		    apple_protect_pager_count_mapped);
		if (count_unmapped > apple_protect_pager_count_unmapped_max) {
			apple_protect_pager_count_unmapped_max = count_unmapped;
		}
		pager->is_mapped = FALSE;
		apple_protect_pager_deallocate_internal(pager, TRUE);
		                                                        
	} else {
		lck_mtx_unlock(&apple_protect_pager_lock);
	}

	return KERN_SUCCESS;
}

boolean_t
apple_protect_pager_backing_object(
	memory_object_t mem_obj,
	memory_object_offset_t offset,
	vm_object_t *backing_object,
	vm_object_offset_t *backing_offset)
{
	apple_protect_pager_t   pager;

	PAGER_DEBUG(PAGER_ALL,
	    ("apple_protect_pager_backing_object: %p\n", mem_obj));

	pager = apple_protect_pager_lookup(mem_obj);

	*backing_object = pager->backing_object;
	*backing_offset = pager->backing_offset + offset;

	return TRUE;
}

  
  
   
apple_protect_pager_t
apple_protect_pager_lookup(
	memory_object_t  mem_obj)
{
	apple_protect_pager_t   pager;

	assert(mem_obj->mo_pager_ops == &apple_protect_pager_ops);
	pager = (apple_protect_pager_t)(uintptr_t) mem_obj;
	assert(os_ref_get_count_raw(&pager->ap_pgr_hdr_ref) > 0);
	return pager;
}

apple_protect_pager_t
apple_protect_pager_create(
	vm_object_t             backing_object,
	vm_object_offset_t      backing_offset,
	vm_object_offset_t      crypto_backing_offset,
	struct pager_crypt_info *crypt_info,
	vm_object_offset_t      crypto_start,
	vm_object_offset_t      crypto_end,
	boolean_t               cache_pager)
{
	apple_protect_pager_t   pager, pager2;
	memory_object_control_t control;
	kern_return_t           kr;
	struct pager_crypt_info *old_crypt_info;

	pager = (apple_protect_pager_t) kalloc(sizeof(*pager));
	if (pager == APPLE_PROTECT_PAGER_NULL) {
		return APPLE_PROTECT_PAGER_NULL;
	}

	  
                                                               
                                                             
                                                               
                                                                
                                                                 
    
	pager->ap_pgr_hdr.mo_ikot = IKOT_MEMORY_OBJECT;
	pager->ap_pgr_hdr.mo_pager_ops = &apple_protect_pager_ops;
	pager->ap_pgr_hdr.mo_control = MEMORY_OBJECT_CONTROL_NULL;

	pager->is_ready = FALSE;                                     
	                                  
	os_ref_init_count_raw(&pager->ap_pgr_hdr_ref, NULL, 1);
	pager->is_mapped = FALSE;
	if (cache_pager) {
		                                   
		os_ref_retain_locked_raw(&pager->ap_pgr_hdr_ref, NULL);
		pager->is_cached = true;
	} else {
		pager->is_cached = false;
	}
	pager->backing_object = backing_object;
	pager->backing_offset = backing_offset;
	pager->crypto_backing_offset = crypto_backing_offset;
	pager->crypto_start = crypto_start;
	pager->crypto_end = crypto_end;
	pager->crypt_info = crypt_info;                          

#if CRYPT_INFO_DEBUG
	printf("CRYPT_INFO %s: crypt_info %p [%p,%p,%p,%d]\n",
	    __FUNCTION__,
	    crypt_info,
	    crypt_info->page_decrypt,
	    crypt_info->crypt_end,
	    crypt_info->crypt_ops,
	    crypt_info->crypt_refcnt);
#endif                       

	vm_object_reference(backing_object);

	old_crypt_info = NULL;

	lck_mtx_lock(&apple_protect_pager_lock);
	                                                                  
	queue_iterate(&apple_protect_pager_queue,
	    pager2,
	    apple_protect_pager_t,
	    pager_queue) {
		if ((pager2->crypt_info->page_decrypt !=
		    crypt_info->page_decrypt) ||
		    (pager2->crypt_info->crypt_end !=
		    crypt_info->crypt_end) ||
		    (pager2->crypt_info->crypt_ops !=
		    crypt_info->crypt_ops)) {
			                                                  
			continue;
		}

		                                      
		if (old_crypt_info) {
			                                             
			assert(old_crypt_info == pager2->crypt_info);
		} else if (pager2->crypt_info != crypt_info) {
			                                           
#if CRYPT_INFO_DEBUG
			printf("CRYPT_INFO %s: reference %p ref %d "
			    "(create match)\n",
			    __FUNCTION__,
			    pager2->crypt_info,
			    pager2->crypt_info->crypt_refcnt);
#endif                       
			old_crypt_info = pager2->crypt_info;
			crypt_info_reference(old_crypt_info);
			pager->crypt_info = old_crypt_info;
		}

		if (pager2->backing_object == backing_object &&
		    pager2->backing_offset == backing_offset &&
		    pager2->crypto_backing_offset == crypto_backing_offset &&
		    pager2->crypto_start == crypto_start &&
		    pager2->crypto_end == crypto_end) {
			                                
			break;
		}
	}
	if (!queue_end(&apple_protect_pager_queue,
	    (queue_entry_t) pager2)) {
		                                              
		lck_mtx_unlock(&apple_protect_pager_lock);
		vm_object_deallocate(pager->backing_object);
		pager->backing_object = VM_OBJECT_NULL;
#if CRYPT_INFO_DEBUG
		printf("CRYPT_INFO %s: %p ref %d (create pager match)\n",
		    __FUNCTION__,
		    pager->crypt_info,
		    pager->crypt_info->crypt_refcnt);
#endif                       
		crypt_info_deallocate(pager->crypt_info);
		pager->crypt_info = NULL;
		kfree(pager, sizeof(*pager));
		                                
		pager = pager2;
		                                                   
		return pager;
	}

	                                                       
	queue_enter_first(&apple_protect_pager_queue,
	    pager,
	    apple_protect_pager_t,
	    pager_queue);
	apple_protect_pager_count++;
	if (apple_protect_pager_count > apple_protect_pager_count_max) {
		apple_protect_pager_count_max = apple_protect_pager_count;
	}
	lck_mtx_unlock(&apple_protect_pager_lock);

	kr = memory_object_create_named((memory_object_t) pager,
	    0,
	    &control);
	assert(kr == KERN_SUCCESS);

	memory_object_mark_trusted(control);

	lck_mtx_lock(&apple_protect_pager_lock);
	                                           
	pager->is_ready = TRUE;
	lck_mtx_unlock(&apple_protect_pager_lock);

	                                                      
	thread_wakeup(&pager->is_ready);

	if (old_crypt_info != NULL &&
	    old_crypt_info != crypt_info) {
		                                                               
#if CRYPT_INFO_DEBUG
		printf("CRYPT_INFO %s: deallocate %p ref %d "
		    "(create used old)\n",
		    __FUNCTION__,
		    crypt_info,
		    crypt_info->crypt_refcnt);
#endif                       
		crypt_info_deallocate(crypt_info);
		crypt_info = NULL;
	}

	return pager;
}

  
                              
  
                                                                 
                                                                       
                                                   
   
memory_object_t
apple_protect_pager_setup(
	vm_object_t             backing_object,
	vm_object_offset_t      backing_offset,
	vm_object_offset_t      crypto_backing_offset,
	struct pager_crypt_info *crypt_info,
	vm_object_offset_t      crypto_start,
	vm_object_offset_t      crypto_end,
	boolean_t               cache_pager)
{
	apple_protect_pager_t   pager;
	struct pager_crypt_info *old_crypt_info, *new_crypt_info;

#if CRYPT_INFO_DEBUG
	printf("CRYPT_INFO %s: crypt_info=%p [%p,%p,%p,%d]\n",
	    __FUNCTION__,
	    crypt_info,
	    crypt_info->page_decrypt,
	    crypt_info->crypt_end,
	    crypt_info->crypt_ops,
	    crypt_info->crypt_refcnt);
#endif                       

	old_crypt_info = NULL;

	lck_mtx_lock(&apple_protect_pager_lock);

	queue_iterate(&apple_protect_pager_queue,
	    pager,
	    apple_protect_pager_t,
	    pager_queue) {
		if ((pager->crypt_info->page_decrypt !=
		    crypt_info->page_decrypt) ||
		    (pager->crypt_info->crypt_end !=
		    crypt_info->crypt_end) ||
		    (pager->crypt_info->crypt_ops !=
		    crypt_info->crypt_ops)) {
			                                           
			continue;
		}
		                                      
		if (old_crypt_info) {
			                                             
			assert(old_crypt_info == pager->crypt_info);
		} else {
			                                           
			old_crypt_info = pager->crypt_info;
#if CRYPT_INFO_DEBUG
			printf("CRYPT_INFO %s: "
			    "switching crypt_info from %p [%p,%p,%p,%d] "
			    "to %p [%p,%p,%p,%d] from pager %p\n",
			    __FUNCTION__,
			    crypt_info,
			    crypt_info->page_decrypt,
			    crypt_info->crypt_end,
			    crypt_info->crypt_ops,
			    crypt_info->crypt_refcnt,
			    old_crypt_info,
			    old_crypt_info->page_decrypt,
			    old_crypt_info->crypt_end,
			    old_crypt_info->crypt_ops,
			    old_crypt_info->crypt_refcnt,
			    pager);
			printf("CRYPT_INFO %s: %p ref %d (setup match)\n",
			    __FUNCTION__,
			    pager->crypt_info,
			    pager->crypt_info->crypt_refcnt);
#endif                       
			crypt_info_reference(pager->crypt_info);
		}

		if (pager->backing_object == backing_object &&
		    pager->backing_offset == backing_offset &&
		    pager->crypto_backing_offset == crypto_backing_offset &&
		    pager->crypto_start == crypto_start &&
		    pager->crypto_end == crypto_end) {
			                                 
			assert(old_crypt_info == pager->crypt_info);
			assert(old_crypt_info->crypt_refcnt > 1);
#if CRYPT_INFO_DEBUG
			printf("CRYPT_INFO %s: "
			    "pager match with %p crypt_info %p\n",
			    __FUNCTION__,
			    pager,
			    pager->crypt_info);
			printf("CRYPT_INFO %s: deallocate %p ref %d "
			    "(pager match)\n",
			    __FUNCTION__,
			    old_crypt_info,
			    old_crypt_info->crypt_refcnt);
#endif                       
			                                                      
			crypt_info_deallocate(old_crypt_info);
			assert(old_crypt_info->crypt_refcnt > 0);
			                                                 
			os_ref_retain_locked_raw(&pager->ap_pgr_hdr_ref, NULL);
			break;
		}
	}
	if (queue_end(&apple_protect_pager_queue,
	    (queue_entry_t) pager)) {
		lck_mtx_unlock(&apple_protect_pager_lock);
		                                               
		pager = APPLE_PROTECT_PAGER_NULL;
		if (old_crypt_info) {
			                                           
			new_crypt_info = old_crypt_info;
#if CRYPT_INFO_DEBUG
			printf("CRYPT_INFO %s: "
			    "will use old_crypt_info %p for new pager\n",
			    __FUNCTION__,
			    old_crypt_info);
#endif                       
		} else {
			                                             
			new_crypt_info = kalloc(sizeof(*new_crypt_info));
			*new_crypt_info = *crypt_info;
			new_crypt_info->crypt_refcnt = 1;
#if CRYPT_INFO_DEBUG
			printf("CRYPT_INFO %s: "
			    "will use new_crypt_info %p for new pager\n",
			    __FUNCTION__,
			    new_crypt_info);
#endif                       
		}
		if (new_crypt_info == NULL) {
			                                                 
		} else {
			                      
			pager = apple_protect_pager_create(
				backing_object,
				backing_offset,
				crypto_backing_offset,
				new_crypt_info,
				crypto_start,
				crypto_end,
				cache_pager);
		}
		if (pager == APPLE_PROTECT_PAGER_NULL) {
			                                  
			if (new_crypt_info == old_crypt_info) {
				                                               
#if CRYPT_INFO_DEBUG
				printf("CRYPT_INFO %s: deallocate %p ref %d "
				    "(create fail old_crypt_info)\n",
				    __FUNCTION__,
				    old_crypt_info,
				    old_crypt_info->crypt_refcnt);
#endif                       
				crypt_info_deallocate(old_crypt_info);
				old_crypt_info = NULL;
			} else {
				                                   
				assert(new_crypt_info->crypt_refcnt == 1);
#if CRYPT_INFO_DEBUG
				printf("CRYPT_INFO %s: deallocate %p ref %d "
				    "(create fail new_crypt_info)\n",
				    __FUNCTION__,
				    new_crypt_info,
				    new_crypt_info->crypt_refcnt);
#endif                       
				crypt_info_deallocate(new_crypt_info);
				new_crypt_info = NULL;
			}
			return MEMORY_OBJECT_NULL;
		}
		lck_mtx_lock(&apple_protect_pager_lock);
	} else {
		assert(old_crypt_info == pager->crypt_info);
	}

	while (!pager->is_ready) {
		lck_mtx_sleep(&apple_protect_pager_lock,
		    LCK_SLEEP_DEFAULT,
		    &pager->is_ready,
		    THREAD_UNINT);
	}
	lck_mtx_unlock(&apple_protect_pager_lock);

	return (memory_object_t) pager;
}

void
apple_protect_pager_trim(void)
{
	apple_protect_pager_t   pager, prev_pager;
	queue_head_t            trim_queue;
	unsigned int            num_trim;
	unsigned int            count_unmapped;

	lck_mtx_lock(&apple_protect_pager_lock);

	  
                                                           
                                                           
    
	queue_init(&trim_queue);
	num_trim = 0;

	for (pager = (apple_protect_pager_t)
	    queue_last(&apple_protect_pager_queue);
	    !queue_end(&apple_protect_pager_queue,
	    (queue_entry_t) pager);
	    pager = prev_pager) {
		                                    
		prev_pager = (apple_protect_pager_t)
		    queue_prev(&pager->pager_queue);

		if (pager->is_cached &&
		    os_ref_get_count_raw(&pager->ap_pgr_hdr_ref) == 2 &&
		    pager->is_ready &&
		    !pager->is_mapped) {
			                               
			num_trim++;
			                                              
			apple_protect_pager_dequeue(pager);
			                                      
			queue_enter_first(&trim_queue,
			    pager,
			    apple_protect_pager_t,
			    pager_queue);

			count_unmapped = (apple_protect_pager_count -
			    apple_protect_pager_count_mapped);
			if (count_unmapped <= apple_protect_pager_cache_limit) {
				                                   
				break;
			}
		}
	}
	if (num_trim > apple_protect_pager_num_trim_max) {
		apple_protect_pager_num_trim_max = num_trim;
	}
	apple_protect_pager_num_trim_total += num_trim;

	lck_mtx_unlock(&apple_protect_pager_lock);

	                                  
	while (!queue_empty(&trim_queue)) {
		queue_remove_first(&trim_queue,
		    pager,
		    apple_protect_pager_t,
		    pager_queue);
		assert(pager->is_cached);
		pager->is_cached = false;
		pager->pager_queue.next = NULL;
		pager->pager_queue.prev = NULL;
		  
                                                          
                                                           
                 
     
		os_ref_count_t __assert_only count;
		count = os_ref_release_locked_raw(&pager->ap_pgr_hdr_ref, NULL);
		assert(count == 1);
		apple_protect_pager_terminate_internal(pager);
	}
}


void
crypt_info_reference(
	struct pager_crypt_info *crypt_info)
{
	assert(crypt_info->crypt_refcnt != 0);
#if CRYPT_INFO_DEBUG
	printf("CRYPT_INFO %s: %p ref %d -> %d\n",
	    __FUNCTION__,
	    crypt_info,
	    crypt_info->crypt_refcnt,
	    crypt_info->crypt_refcnt + 1);
#endif                       
	OSAddAtomic(+1, &crypt_info->crypt_refcnt);
}

void
crypt_info_deallocate(
	struct pager_crypt_info *crypt_info)
{
#if CRYPT_INFO_DEBUG
	printf("CRYPT_INFO %s: %p ref %d -> %d\n",
	    __FUNCTION__,
	    crypt_info,
	    crypt_info->crypt_refcnt,
	    crypt_info->crypt_refcnt - 1);
#endif                       
	OSAddAtomic(-1, &crypt_info->crypt_refcnt);
	if (crypt_info->crypt_refcnt == 0) {
		                                      
		if (crypt_info->crypt_end) {
			crypt_info->crypt_end(crypt_info->crypt_ops);
			crypt_info->crypt_end = NULL;
		}
#if CRYPT_INFO_DEBUG
		printf("CRYPT_INFO %s: freeing %p\n",
		    __FUNCTION__,
		    crypt_info);
#endif                       
		kfree(crypt_info, sizeof(*crypt_info));
		crypt_info = NULL;
	}
}
