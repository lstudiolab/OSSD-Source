/*
 * Copyright (c) 2008-2020 Apple Inc. All rights reserved.
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

#include <mach/kern_return.h>
#include <mach/memory_object_control.h>
#include <mach/upl.h>

#include <kern/ipc_kobject.h>
#include <kern/kalloc.h>
#include <kern/queue.h>

#include <vm/memory_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>


  
                              
  
                                                                         
                                                                            
                                                          
                                                                           
                                                                           
                                                               
                                                        
                 
                                                                             
                                                                       
                                                    
              
                                                                              
                                                                             
                                                                            
                                                
                                                                              
                                                                            
                                                                         
                                                                            
                               
  
                                                                           
                                                                            
            
                                                                                
                                                      
                                                                         
                                                                    
   

                          
void swapfile_pager_reference(memory_object_t mem_obj);
void swapfile_pager_deallocate(memory_object_t mem_obj);
kern_return_t swapfile_pager_init(memory_object_t mem_obj,
    memory_object_control_t control,
    memory_object_cluster_size_t pg_size);
kern_return_t swapfile_pager_terminate(memory_object_t mem_obj);
kern_return_t swapfile_pager_data_request(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t length,
    vm_prot_t protection_required,
    memory_object_fault_info_t fault_info);
kern_return_t swapfile_pager_data_return(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t      data_cnt,
    memory_object_offset_t *resid_offset,
    int *io_error,
    boolean_t dirty,
    boolean_t kernel_copy,
    int upl_flags);
kern_return_t swapfile_pager_data_initialize(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_cluster_size_t data_cnt);
kern_return_t swapfile_pager_data_unlock(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_size_t size,
    vm_prot_t desired_access);
kern_return_t swapfile_pager_synchronize(memory_object_t mem_obj,
    memory_object_offset_t offset,
    memory_object_size_t length,
    vm_sync_t sync_flags);
kern_return_t swapfile_pager_map(memory_object_t mem_obj,
    vm_prot_t prot);
kern_return_t swapfile_pager_last_unmap(memory_object_t mem_obj);

  
                                        
                                                                         
   
const struct memory_object_pager_ops swapfile_pager_ops = {
	.memory_object_reference = swapfile_pager_reference,
	.memory_object_deallocate = swapfile_pager_deallocate,
	.memory_object_init = swapfile_pager_init,
	.memory_object_terminate = swapfile_pager_terminate,
	.memory_object_data_request = swapfile_pager_data_request,
	.memory_object_data_return = swapfile_pager_data_return,
	.memory_object_data_initialize = swapfile_pager_data_initialize,
	.memory_object_data_unlock = swapfile_pager_data_unlock,
	.memory_object_synchronize = swapfile_pager_synchronize,
	.memory_object_map = swapfile_pager_map,
	.memory_object_last_unmap = swapfile_pager_last_unmap,
	.memory_object_data_reclaim = NULL,
	.memory_object_backing_object = NULL,
	.memory_object_pager_name = "swapfile pager"
};

  
                                                           
                      
   
typedef struct swapfile_pager {
	                              
	struct memory_object    swp_pgr_hdr;

	                         
	queue_chain_t           pager_queue;                            
#if MEMORY_OBJECT_HAS_REFCOUNT
#define swp_pgr_hdr_ref         swp_pgr_hdr.mo_ref
#else
	os_ref_atomic_t         swp_pgr_hdr_ref;                           
#endif
	bool                    is_ready;                                  
	bool                    is_mapped;                                  
	struct vnode            *swapfile_vnode;                          
} *swapfile_pager_t;
#define SWAPFILE_PAGER_NULL     ((swapfile_pager_t) NULL)

  
                                              
                                                           
   
int swapfile_pager_count = 0;                                 
queue_head_t swapfile_pager_queue = QUEUE_HEAD_INITIALIZER(swapfile_pager_queue);
LCK_GRP_DECLARE(swapfile_pager_lck_grp, "swapfile pager");
LCK_MTX_DECLARE(swapfile_pager_lock, &swapfile_pager_lck_grp);

  
                         
   
int swapfile_pager_count_max = 0;

                         
swapfile_pager_t swapfile_pager_create(struct vnode *vp);
swapfile_pager_t swapfile_pager_lookup(memory_object_t mem_obj);
void swapfile_pager_dequeue(swapfile_pager_t pager);
void swapfile_pager_deallocate_internal(swapfile_pager_t pager,
    boolean_t locked);
void swapfile_pager_terminate_internal(swapfile_pager_t pager);


#if DEBUG
int swapfile_pagerdebug = 0;
#define PAGER_ALL               0xffffffff
#define PAGER_INIT              0x00000001
#define PAGER_PAGEIN            0x00000002

#define PAGER_DEBUG(LEVEL, A)                                           \
	MACRO_BEGIN                                                     \
	if ((swapfile_pagerdebug & LEVEL)==LEVEL) {             \
	        printf A;                                               \
	}                                                               \
	MACRO_END
#else
#define PAGER_DEBUG(LEVEL, A)
#endif


  
                        
  
                                                                         
   
kern_return_t
swapfile_pager_init(
	memory_object_t         mem_obj,
	memory_object_control_t control,
#if !DEBUG
	__unused
#endif
	memory_object_cluster_size_t pg_size)
{
	swapfile_pager_t        pager;
	kern_return_t           kr;
	memory_object_attr_info_data_t  attributes;

	PAGER_DEBUG(PAGER_ALL,
	    ("swapfile_pager_init: %p, %p, %x\n",
	    mem_obj, control, pg_size));

	if (control == MEMORY_OBJECT_CONTROL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	pager = swapfile_pager_lookup(mem_obj);

	memory_object_control_reference(control);

	pager->swp_pgr_hdr.mo_control = control;

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
		panic("swapfile_pager_init: "
		    "memory_object_change_attributes() failed");
	}

	return KERN_SUCCESS;
}

  
                         
  
                                                                     
                                                                         
                                                                        
            
   
kern_return_t
swapfile_pager_data_return(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_cluster_size_t           data_cnt,
	__unused memory_object_offset_t *resid_offset,
	__unused int                    *io_error,
	__unused boolean_t              dirty,
	__unused boolean_t              kernel_copy,
	__unused int                    upl_flags)
{
	panic("swapfile_pager_data_return: should never get called");
	return KERN_FAILURE;
}

kern_return_t
swapfile_pager_data_initialize(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_cluster_size_t           data_cnt)
{
	panic("swapfile_pager_data_initialize: should never get called");
	return KERN_FAILURE;
}

kern_return_t
swapfile_pager_data_unlock(
	__unused memory_object_t        mem_obj,
	__unused memory_object_offset_t offset,
	__unused memory_object_size_t           size,
	__unused vm_prot_t              desired_access)
{
	return KERN_FAILURE;
}

  
                                
  
                                    
   
kern_return_t
swapfile_pager_data_request(
	memory_object_t         mem_obj,
	memory_object_offset_t  offset,
	memory_object_cluster_size_t            length,
#if !DEBUG
	__unused
#endif
	vm_prot_t               protection_required,
	__unused memory_object_fault_info_t mo_fault_info)
{
	swapfile_pager_t        pager;
	memory_object_control_t mo_control;
	upl_t                   upl;
	int                     upl_flags;
	upl_size_t              upl_size;
	upl_page_info_t         *upl_pl = NULL;
	unsigned int            pl_count;
	vm_object_t             dst_object;
	kern_return_t           kr, retval;
	vm_map_offset_t         kernel_mapping;
	vm_offset_t             dst_vaddr;
	char                    *dst_ptr;
	vm_offset_t             cur_offset;
	vm_map_entry_t          map_entry;

	PAGER_DEBUG(PAGER_ALL, ("swapfile_pager_data_request: %p, %llx, %x, %x\n", mem_obj, offset, length, protection_required));

	kernel_mapping = 0;
	upl = NULL;
	upl_pl = NULL;

	pager = swapfile_pager_lookup(mem_obj);
	assert(pager->is_ready);
	assert(os_ref_get_count_raw(&pager->swp_pgr_hdr_ref) > 1);                                

	PAGER_DEBUG(PAGER_PAGEIN, ("swapfile_pager_data_request: %p, %llx, %x, %x, pager %p\n", mem_obj, offset, length, protection_required, pager));

	  
                                                     
    
	mo_control = pager->swp_pgr_hdr.mo_control;

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
	    &upl, NULL, NULL, upl_flags, VM_KERN_MEMORY_OSFMK);
	if (kr != KERN_SUCCESS) {
		retval = kr;
		goto done;
	}
	dst_object = memory_object_control_to_vm_object(mo_control);
	assert(dst_object != VM_OBJECT_NULL);


	  
                                                                  
                                                                 
    
	vm_object_reference(kernel_object);                           
	kr = vm_map_find_space(kernel_map,
	    &kernel_mapping,
	    PAGE_SIZE_64,
	    0,
	    0,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    VM_KERN_MEMORY_NONE,
	    &map_entry);
	if (kr != KERN_SUCCESS) {
		vm_object_deallocate(kernel_object);
		retval = kr;
		goto done;
	}
	VME_OBJECT_SET(map_entry, kernel_object);
	VME_OFFSET_SET(map_entry, kernel_mapping - VM_MIN_KERNEL_ADDRESS);
	vm_map_unlock(kernel_map);
	dst_vaddr = CAST_DOWN(vm_offset_t, kernel_mapping);
	dst_ptr = (char *) dst_vaddr;

	  
                                                      
    
	upl_pl = UPL_GET_INTERNAL_PAGE_LIST(upl);
	pl_count = length / PAGE_SIZE;
	for (cur_offset = 0; cur_offset < length; cur_offset += PAGE_SIZE) {
		ppnum_t dst_pnum;

		if (!upl_page_present(upl_pl, (int)(cur_offset / PAGE_SIZE))) {
			                                          
			continue;
		}

		  
                                                          
                   
                                                         
               
     
		dst_pnum = (ppnum_t)
		    upl_phys_page(upl_pl, (int)(cur_offset / PAGE_SIZE));
		assert(dst_pnum != 0);
		retval = pmap_enter(kernel_pmap,
		    kernel_mapping,
		    dst_pnum,
		    VM_PROT_READ | VM_PROT_WRITE,
		    VM_PROT_NONE,
		    0,
		    TRUE);

		assert(retval == KERN_SUCCESS);

		if (retval != KERN_SUCCESS) {
			goto done;
		}

		memset(dst_ptr, '\0', PAGE_SIZE);
		                                                    
		dst_ptr[PAGE_SIZE - 1] = '\n';

		  
                                                    
                   
     
		pmap_remove(kernel_pmap,
		    (addr64_t) kernel_mapping,
		    (addr64_t) (kernel_mapping + PAGE_SIZE_64));
	}

	retval = KERN_SUCCESS;
done:
	if (upl != NULL) {
		                      

		  
                                                          
                                                            
                                                          
                               
                                       
     
		upl_clear_dirty(upl, TRUE);

		                             
		if (retval != KERN_SUCCESS) {
			upl_abort(upl, 0);
		} else {
			boolean_t empty;
			assertf(page_aligned(upl->u_offset) && page_aligned(upl->u_size),
			    "upl %p offset 0x%llx size 0x%x",
			    upl, upl->u_offset, upl->u_size);
			upl_commit_range(upl, 0, upl->u_size,
			    UPL_COMMIT_CS_VALIDATED,
			    upl_pl, pl_count, &empty);
		}

		                            
		upl_deallocate(upl);
		upl = NULL;
	}
	if (kernel_mapping != 0) {
		                                                              
		kr = vm_map_remove(kernel_map,
		    kernel_mapping,
		    kernel_mapping + PAGE_SIZE_64,
		    VM_MAP_REMOVE_NO_FLAGS);
		assert(kr == KERN_SUCCESS);
		kernel_mapping = 0;
		dst_vaddr = 0;
	}

	return retval;
}

  
                             
  
                                         
                                                                               
                                                     
   
void
swapfile_pager_reference(
	memory_object_t         mem_obj)
{
	swapfile_pager_t        pager;

	pager = swapfile_pager_lookup(mem_obj);

	lck_mtx_lock(&swapfile_pager_lock);
	os_ref_retain_locked_raw(&pager->swp_pgr_hdr_ref, NULL);
	lck_mtx_unlock(&swapfile_pager_lock);
}


  
                          
  
                                           
  
                                              
   
void
swapfile_pager_dequeue(
	swapfile_pager_t pager)
{
	assert(!pager->is_mapped);

	queue_remove(&swapfile_pager_queue,
	    pager,
	    swapfile_pager_t,
	    pager_queue);
	pager->pager_queue.next = NULL;
	pager->pager_queue.prev = NULL;

	swapfile_pager_count--;
}

  
                                     
  
                                                                       
                   
                                                                    
                                                                   
                          
  
                                                          
                                                                         
                                                
   
void
swapfile_pager_terminate_internal(
	swapfile_pager_t pager)
{
	assert(pager->is_ready);
	assert(!pager->is_mapped);

	if (pager->swapfile_vnode != NULL) {
		pager->swapfile_vnode = NULL;
	}

	                                                  
	memory_object_destroy(pager->swp_pgr_hdr.mo_control, 0);
}

  
                                       
  
                                                              
                       
                                                                        
                    
   
void
swapfile_pager_deallocate_internal(
	swapfile_pager_t        pager,
	boolean_t               locked)
{
	os_ref_count_t ref_count;

	if (!locked) {
		lck_mtx_lock(&swapfile_pager_lock);
	}

	                                    
	ref_count = os_ref_release_locked_raw(&pager->swp_pgr_hdr_ref, NULL);

	if (ref_count == 1) {
		  
                                                         
                                                       
                  
     
		swapfile_pager_dequeue(pager);
		                                                     
		lck_mtx_unlock(&swapfile_pager_lock);
		swapfile_pager_terminate_internal(pager);
	} else if (ref_count == 0) {
		  
                                                            
                                                            
                     
     
		lck_mtx_unlock(&swapfile_pager_lock);
		if (pager->swp_pgr_hdr.mo_control != MEMORY_OBJECT_CONTROL_NULL) {
			memory_object_control_deallocate(pager->swp_pgr_hdr.mo_control);
			pager->swp_pgr_hdr.mo_control = MEMORY_OBJECT_CONTROL_NULL;
		}
		kfree(pager, sizeof(*pager));
		pager = SWAPFILE_PAGER_NULL;
	} else {
		                                                          
		lck_mtx_unlock(&swapfile_pager_lock);
	}

	                                            
}

  
                              
  
                                                              
                       
   
void
swapfile_pager_deallocate(
	memory_object_t         mem_obj)
{
	swapfile_pager_t        pager;

	PAGER_DEBUG(PAGER_ALL, ("swapfile_pager_deallocate: %p\n", mem_obj));
	pager = swapfile_pager_lookup(mem_obj);
	swapfile_pager_deallocate_internal(pager, FALSE);
}

  
  
   
kern_return_t
swapfile_pager_terminate(
#if !DEBUG
	__unused
#endif
	memory_object_t mem_obj)
{
	PAGER_DEBUG(PAGER_ALL, ("swapfile_pager_terminate: %p\n", mem_obj));

	return KERN_SUCCESS;
}

  
  
   
kern_return_t
swapfile_pager_synchronize(
	__unused memory_object_t        mem_obbj,
	__unused memory_object_offset_t offset,
	__unused memory_object_size_t   length,
	__unused vm_sync_t              sync_flags)
{
	panic("swapfile_pager_synchronize: memory_object_synchronize no longer supported\n");
	return KERN_FAILURE;
}

  
                       
  
                                                                  
                                                                         
                                                                       
                                                 
   
kern_return_t
swapfile_pager_map(
	memory_object_t         mem_obj,
	__unused vm_prot_t      prot)
{
	swapfile_pager_t        pager;

	PAGER_DEBUG(PAGER_ALL, ("swapfile_pager_map: %p\n", mem_obj));

	pager = swapfile_pager_lookup(mem_obj);

	lck_mtx_lock(&swapfile_pager_lock);
	assert(pager->is_ready);
	assert(os_ref_get_count_raw(&pager->swp_pgr_hdr_ref) > 0);                     
	if (pager->is_mapped == FALSE) {
		  
                                                          
                                                          
                 
     
		pager->is_mapped = TRUE;
		os_ref_retain_locked_raw(&pager->swp_pgr_hdr_ref, NULL);
	}
	lck_mtx_unlock(&swapfile_pager_lock);

	return KERN_SUCCESS;
}

  
                              
  
                                                                             
   
kern_return_t
swapfile_pager_last_unmap(
	memory_object_t         mem_obj)
{
	swapfile_pager_t        pager;

	PAGER_DEBUG(PAGER_ALL,
	    ("swapfile_pager_last_unmap: %p\n", mem_obj));

	pager = swapfile_pager_lookup(mem_obj);

	lck_mtx_lock(&swapfile_pager_lock);
	if (pager->is_mapped) {
		  
                                                          
                                                              
     
		pager->is_mapped = FALSE;
		swapfile_pager_deallocate_internal(pager, TRUE);
		                                                        
	} else {
		lck_mtx_unlock(&swapfile_pager_lock);
	}

	return KERN_SUCCESS;
}


  
  
   
swapfile_pager_t
swapfile_pager_lookup(
	memory_object_t  mem_obj)
{
	swapfile_pager_t        pager;

	assert(mem_obj->mo_pager_ops == &swapfile_pager_ops);
	__IGNORE_WCASTALIGN(pager = (swapfile_pager_t) mem_obj);
	assert(os_ref_get_count_raw(&pager->swp_pgr_hdr_ref) > 0);
	return pager;
}

swapfile_pager_t
swapfile_pager_create(
	struct vnode            *vp)
{
	swapfile_pager_t        pager, pager2;
	memory_object_control_t control;
	kern_return_t           kr;

	pager = (swapfile_pager_t) kalloc(sizeof(*pager));
	if (pager == SWAPFILE_PAGER_NULL) {
		return SWAPFILE_PAGER_NULL;
	}

	  
                                                               
                                                             
                                                               
                                                                 
                                                                 
    
	pager->swp_pgr_hdr.mo_ikot = IKOT_MEMORY_OBJECT;
	pager->swp_pgr_hdr.mo_pager_ops = &swapfile_pager_ops;
	pager->swp_pgr_hdr.mo_control = MEMORY_OBJECT_CONTROL_NULL;

	pager->is_ready = FALSE;                                     
	os_ref_init_raw(&pager->swp_pgr_hdr_ref, NULL);                        
	pager->is_mapped = FALSE;
	pager->swapfile_vnode = vp;

	lck_mtx_lock(&swapfile_pager_lock);
	                                                                  
	queue_iterate(&swapfile_pager_queue,
	    pager2,
	    swapfile_pager_t,
	    pager_queue) {
		if (pager2->swapfile_vnode == vp) {
			break;
		}
	}
	if (!queue_end(&swapfile_pager_queue,
	    (queue_entry_t) pager2)) {
		                                                              
		os_ref_retain_locked_raw(&pager2->swp_pgr_hdr_ref, NULL);
		                                              
		lck_mtx_unlock(&swapfile_pager_lock);
		pager->swapfile_vnode = NULL;
		kfree(pager, sizeof(*pager));
		                                
		pager = pager2;
		                                                   
		return pager;
	}

	                                                       
	queue_enter_first(&swapfile_pager_queue,
	    pager,
	    swapfile_pager_t,
	    pager_queue);
	swapfile_pager_count++;
	if (swapfile_pager_count > swapfile_pager_count_max) {
		swapfile_pager_count_max = swapfile_pager_count;
	}
	lck_mtx_unlock(&swapfile_pager_lock);

	kr = memory_object_create_named((memory_object_t) pager,
	    0,
	    &control);
	assert(kr == KERN_SUCCESS);

	memory_object_mark_trusted(control);

	lck_mtx_lock(&swapfile_pager_lock);
	                                           
	pager->is_ready = TRUE;
	lck_mtx_unlock(&swapfile_pager_lock);

	                                                      
	thread_wakeup(&pager->is_ready);

	return pager;
}

  
                         
  
                                                                 
                                                                       
                                                   
   
memory_object_t
swapfile_pager_setup(
	struct vnode *vp)
{
	swapfile_pager_t        pager;

	lck_mtx_lock(&swapfile_pager_lock);

	queue_iterate(&swapfile_pager_queue,
	    pager,
	    swapfile_pager_t,
	    pager_queue) {
		if (pager->swapfile_vnode == vp) {
			break;
		}
	}
	if (queue_end(&swapfile_pager_queue,
	    (queue_entry_t) pager)) {
		                                               
		pager = SWAPFILE_PAGER_NULL;
	} else {
		                                       
		os_ref_retain_raw(&pager->swp_pgr_hdr_ref, NULL);
	}

	lck_mtx_unlock(&swapfile_pager_lock);

	if (pager == SWAPFILE_PAGER_NULL) {
		pager = swapfile_pager_create(vp);
		if (pager == SWAPFILE_PAGER_NULL) {
			return MEMORY_OBJECT_NULL;
		}
	}

	lck_mtx_lock(&swapfile_pager_lock);
	while (!pager->is_ready) {
		lck_mtx_sleep(&swapfile_pager_lock,
		    LCK_SLEEP_DEFAULT,
		    &pager->is_ready,
		    THREAD_UNINT);
	}
	lck_mtx_unlock(&swapfile_pager_lock);

	return (memory_object_t) pager;
}

memory_object_control_t
swapfile_pager_control(
	memory_object_t mem_obj)
{
	swapfile_pager_t        pager;

	if (mem_obj == MEMORY_OBJECT_NULL ||
	    mem_obj->mo_pager_ops != &swapfile_pager_ops) {
		return MEMORY_OBJECT_CONTROL_NULL;
	}
	pager = swapfile_pager_lookup(mem_obj);
	return pager->swp_pgr_hdr.mo_control;
}
