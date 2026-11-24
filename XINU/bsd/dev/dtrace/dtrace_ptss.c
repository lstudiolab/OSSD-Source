/*
 * Copyright (c) 2000-2006 Apple Computer, Inc. All rights reserved.
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

#include <sys/types.h>
#include <sys/proc.h>
#include <sys/proc_internal.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/dtrace_ptss.h>

#include <mach/vm_map.h>
#include <mach/vm_param.h>
#include <mach/mach_vm.h>

#include <kern/task.h>

#include <vm/vm_map.h>

  
                                                
  
                                                               
                                                           
   
struct dtrace_ptss_page_entry*
dtrace_ptss_claim_entry_locked(struct proc* p)
{
	LCK_MTX_ASSERT(&p->p_dtrace_sprlock, LCK_MTX_ASSERT_OWNED);

	struct dtrace_ptss_page_entry* entry = NULL;

	while (TRUE) {
		struct dtrace_ptss_page_entry* temp = p->p_dtrace_ptss_free_list;

		if (temp == NULL) {
			                                                                                         
			struct dtrace_ptss_page* page = dtrace_ptss_allocate_page(p);

			                                   
			if (page == NULL) {
				return NULL;
			}

			                                
			page->next = p->p_dtrace_ptss_pages;
			p->p_dtrace_ptss_pages = page;

			                                      
			do {
				page->entries[DTRACE_PTSS_ENTRIES_PER_PAGE - 1].next = p->p_dtrace_ptss_free_list;
			} while (!OSCompareAndSwapPtr((void *)page->entries[DTRACE_PTSS_ENTRIES_PER_PAGE - 1].next,
			    (void *)&page->entries[0],
			    (void * volatile *)&p->p_dtrace_ptss_free_list));

			                                                    
			continue;
		}

		             
		if (!OSCompareAndSwapPtr((void *)temp, (void *)temp->next, (void * volatile *)&p->p_dtrace_ptss_free_list)) {
			continue;
		}

		                              
		entry = temp;

		break;
	}

	return entry;
}

  
                                                                
   
struct dtrace_ptss_page_entry*
dtrace_ptss_claim_entry(struct proc* p)
{
	                                
	LCK_MTX_ASSERT(&p->p_dtrace_sprlock, LCK_MTX_ASSERT_NOTOWNED);
	LCK_MTX_ASSERT(&p->p_mlock, LCK_MTX_ASSERT_NOTOWNED);

	struct dtrace_ptss_page_entry* entry = NULL;

	while (TRUE) {
		struct dtrace_ptss_page_entry* temp = p->p_dtrace_ptss_free_list;

		if (temp == NULL) {
			lck_mtx_lock(&p->p_dtrace_sprlock);
			temp = dtrace_ptss_claim_entry_locked(p);
			lck_mtx_unlock(&p->p_dtrace_sprlock);
			return temp;
		}

		             
		if (!OSCompareAndSwapPtr((void *)temp, (void *)temp->next, (void * volatile *)&p->p_dtrace_ptss_free_list)) {
			continue;
		}

		                              
		entry = temp;

		break;
	}

	return entry;
}

  
                                                                
  
                                                                  
                                                                      
                                                     
   
void
dtrace_ptss_release_entry(struct proc* p, struct dtrace_ptss_page_entry* e)
{
	if (p && p->p_dtrace_ptss_pages && e) {
		do {
			e->next = p->p_dtrace_ptss_free_list;
		} while (!OSCompareAndSwapPtr((void *)e->next, (void *)e, (void * volatile *)&p->p_dtrace_ptss_free_list));
	}
}

  
                                                                            
  
                                                                            
                                                                             
                                  
  
                                                                            
   
struct dtrace_ptss_page*
dtrace_ptss_allocate_page(struct proc* p)
{
	                                
	struct dtrace_ptss_page* ptss_page = _MALLOC(sizeof(struct dtrace_ptss_page), M_TEMP, M_ZERO | M_WAITOK);
	if (ptss_page == NULL) {
		return NULL;
	}

	                                                                              
	task_t task = p->task;
	vm_map_t map = get_task_map_reference(task);
	if (map == NULL) {
		goto err;
	}

	mach_vm_size_t size = PAGE_MAX_SIZE;
	mach_vm_offset_t addr = 0;
	mach_vm_offset_t write_addr = 0;
	  
                                                                            
                                                                               
    
	vm_prot_t cur_protection = VM_PROT_READ | VM_PROT_EXECUTE;
	vm_prot_t max_protection = VM_PROT_READ | VM_PROT_EXECUTE | VM_PROT_WRITE;

	kern_return_t kr = mach_vm_map_kernel(map, &addr, size, 0, VM_FLAGS_ANYWHERE, VM_MAP_KERNEL_FLAGS_NONE, VM_KERN_MEMORY_NONE, IPC_PORT_NULL, 0, FALSE, cur_protection, max_protection, VM_INHERIT_DEFAULT);
	if (kr != KERN_SUCCESS) {
		goto err;
	}
	  
                                                                  
                   
    
	kr = mach_vm_remap_kernel(map, &write_addr, size, 0, VM_FLAGS_ANYWHERE, VM_KERN_MEMORY_NONE, map, addr, FALSE, &cur_protection, &max_protection, VM_INHERIT_DEFAULT);
	if (kr != KERN_SUCCESS || !(max_protection & VM_PROT_WRITE)) {
		goto err;
	}

	kr = mach_vm_protect(map, (mach_vm_offset_t)write_addr, (mach_vm_size_t)size, 0, VM_PROT_READ | VM_PROT_WRITE);
	if (kr != KERN_SUCCESS) {
		goto err;
	}

	                          
	int i;
	for (i = 0; i < DTRACE_PTSS_ENTRIES_PER_PAGE; i++) {
		ptss_page->entries[i].addr = addr + (i * DTRACE_PTSS_SCRATCH_SPACE_PER_THREAD);
		ptss_page->entries[i].write_addr = write_addr + (i * DTRACE_PTSS_SCRATCH_SPACE_PER_THREAD);
		ptss_page->entries[i].next = &ptss_page->entries[i + 1];
	}

	                                      
	ptss_page->entries[DTRACE_PTSS_ENTRIES_PER_PAGE - 1].next = NULL;

	vm_map_deallocate(map);

	return ptss_page;

err:
	_FREE(ptss_page, M_TEMP);

	if (map) {
		vm_map_deallocate(map);
	}

	return NULL;
}

  
                                                                              
  
                                                           
  
                                           
   
void
dtrace_ptss_free_page(struct proc* p, struct dtrace_ptss_page* ptss_page)
{
	                                                  
	task_t task = p->task;
	vm_map_t map = get_task_map_reference(task);

	mach_vm_address_t addr = ptss_page->entries[0].addr;
	mach_vm_size_t size = PAGE_SIZE;                                                                        

	                                                     
	mach_vm_deallocate(map, addr, size);

	mach_vm_address_t write_addr = ptss_page->entries[0].write_addr;
	mach_vm_deallocate(map, write_addr, size);

	vm_map_deallocate(map);
}

  
                                                         
                                                 
   
void
dtrace_ptss_enable(struct proc* p)
{
	LCK_MTX_ASSERT(&p->p_dtrace_sprlock, LCK_MTX_ASSERT_OWNED);
	LCK_MTX_ASSERT(&p->p_mlock, LCK_MTX_ASSERT_OWNED);

	struct uthread* uth;
	  
                                                                   
                                                            
                                                          
    
	TAILQ_FOREACH(uth, &p->p_uthlist, uu_list) {
		uth->t_dtrace_scratch = dtrace_ptss_claim_entry_locked(p);
	}
}

  
                                    
  
                                                            
   
void
dtrace_ptss_exec_exit(struct proc* p)
{
	  
                                                         
                                         
    
	LCK_MTX_ASSERT(&p->p_dtrace_sprlock, LCK_MTX_ASSERT_OWNED);
	LCK_MTX_ASSERT(&p->p_mlock, LCK_MTX_ASSERT_NOTOWNED);

	p->p_dtrace_ptss_free_list = NULL;

	struct dtrace_ptss_page* temp = p->p_dtrace_ptss_pages;
	p->p_dtrace_ptss_pages = NULL;

	while (temp != NULL) {
		struct dtrace_ptss_page* next = temp->next;

		                                                                
		                                                                       
		                                 

		                                             
		_FREE(temp, M_TEMP);
		temp = next;
	}
}

  
                                                              
  
                                                                   
                                                                               
                                     
  
                                                                           
   
void
dtrace_ptss_fork(struct proc* parent, struct proc* child)
{
	                                                                       
	                                              
	                                                  

	  
                                                                  
                                                                    
                                                               
                                                                  
                                                         
    
	LCK_MTX_ASSERT(&parent->p_dtrace_sprlock, LCK_MTX_ASSERT_OWNED);
	LCK_MTX_ASSERT(&parent->p_mlock, LCK_MTX_ASSERT_NOTOWNED);
	LCK_MTX_ASSERT(&child->p_dtrace_sprlock, LCK_MTX_ASSERT_OWNED);
	LCK_MTX_ASSERT(&child->p_mlock, LCK_MTX_ASSERT_NOTOWNED);

	                              
	struct dtrace_ptss_page* temp = parent->p_dtrace_ptss_pages;

	while (temp != NULL) {
		                                  
		dtrace_ptss_free_page(child, temp);

		                                                          
		temp = temp->next;
	}
}
