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
  
   
  
                           
                              
  
                                                          
   

  
                          
   

#include <mach/std_types.h>                        
#include <mach/mach_types.h>

#include <mach/mig.h>
#include <mach/kern_return.h>
#include <mach/memory_object.h>
#include <mach/memory_object_default.h>
#include <mach/memory_object_control_server.h>
#include <mach/host_priv_server.h>
#include <mach/boolean.h>
#include <mach/vm_prot.h>
#include <mach/message.h>

  
                               
   
#include <string.h>                               

#include <kern/host.h>
#include <kern/thread.h>                                  
#include <kern/ipc_mig.h>
#include <kern/misc_protos.h>

#include <vm/vm_object.h>
#include <vm/vm_fault.h>
#include <vm/memory_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/pmap.h>                                       
#include <vm/vm_kern.h>                                      
#include <vm/vm_map.h>                                   
#include <vm/vm_purgeable_internal.h>                                        
#include <vm/vm_shared_region.h>

#include <vm/vm_external.h>

#include <vm/vm_protos.h>

memory_object_default_t memory_manager_default = MEMORY_OBJECT_DEFAULT_NULL;
LCK_MTX_EARLY_DECLARE(memory_manager_default_lock, &vm_object_lck_grp);


  
                                            
  
               
                                                        
                                                             
  
                                                              
  
                                                          
                                                        
                                        
  
                                                          
                                                     
                              
   

#define memory_object_should_return_page(m, should_return) \
    (should_return != MEMORY_OBJECT_RETURN_NONE && \
     (((m)->vmp_dirty || ((m)->vmp_dirty = pmap_is_modified(VM_PAGE_GET_PHYS_PAGE(m)))) || \
      ((m)->vmp_precious && (should_return) == MEMORY_OBJECT_RETURN_ALL) || \
      (should_return) == MEMORY_OBJECT_RETURN_ANYTHING))

typedef int     memory_object_lock_result_t;

#define MEMORY_OBJECT_LOCK_RESULT_DONE                  0
#define MEMORY_OBJECT_LOCK_RESULT_MUST_BLOCK            1
#define MEMORY_OBJECT_LOCK_RESULT_MUST_RETURN           2
#define MEMORY_OBJECT_LOCK_RESULT_MUST_FREE             3

memory_object_lock_result_t memory_object_lock_page(
	vm_page_t               m,
	memory_object_return_t  should_return,
	boolean_t               should_flush,
	vm_prot_t               prot);

  
                                   
  
               
                                                  
                                       
                                                 
                     
  
                                            
                                             
               
   
memory_object_lock_result_t
memory_object_lock_page(
	vm_page_t               m,
	memory_object_return_t  should_return,
	boolean_t               should_flush,
	vm_prot_t               prot)
{
	if (m->vmp_busy || m->vmp_cleaning) {
		return MEMORY_OBJECT_LOCK_RESULT_MUST_BLOCK;
	}

	if (m->vmp_laundry) {
		vm_pageout_steal_laundry(m, FALSE);
	}

	  
                                                
                           
    
	if (m->vmp_absent || m->vmp_error || m->vmp_restart) {
		if (m->vmp_error && should_flush && !VM_PAGE_WIRED(m)) {
			  
                                      
                                 
                             
      
			return MEMORY_OBJECT_LOCK_RESULT_MUST_FREE;
		}
		return MEMORY_OBJECT_LOCK_RESULT_DONE;
	}
	assert(!m->vmp_fictitious);

	if (VM_PAGE_WIRED(m)) {
		  
                                                                  
                                                                 
     
		if (memory_object_should_return_page(m, should_return)) {
			return MEMORY_OBJECT_LOCK_RESULT_MUST_RETURN;
		}

		return MEMORY_OBJECT_LOCK_RESULT_DONE;
	}

	if (should_flush) {
		  
                                                       
                                                       
                                                         
                                  
     
		if (pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m)) & VM_MEM_MODIFIED) {
			SET_PAGE_DIRTY(m, FALSE);
		}
	} else {
		  
                                                
                                                 
                                                     
     
		if (prot != VM_PROT_NO_CHANGE) {
			pmap_page_protect(VM_PAGE_GET_PHYS_PAGE(m), VM_PROT_ALL & ~prot);
		}
	}
	  
                                            
    
	if (memory_object_should_return_page(m, should_return)) {
		  
                                                   
                                                    
                                                 
                                                   
                                                  
                                                
                                       
    
                       
                                                
     
		return MEMORY_OBJECT_LOCK_RESULT_MUST_RETURN;
	}

	  
                               
    
	if (should_flush) {
		return MEMORY_OBJECT_LOCK_RESULT_MUST_FREE;
	}

	  
                                                   
                                                     
                                                   
                                                
   
                                               
                                             
                                         
                                                
                                            
   
                                                                                  
                                                       
    

	return MEMORY_OBJECT_LOCK_RESULT_DONE;
}



  
                                                       
  
               
                                                     
                                                     
                                               
                                              
                                    
                                                       
                                                  
                                                                      
                                           
                                                 
                      
                                                    
                                                      
                                                  
               
  
                                                       
                                                          
                                                  
   

kern_return_t
memory_object_lock_request(
	memory_object_control_t         control,
	memory_object_offset_t          offset,
	memory_object_size_t            size,
	memory_object_offset_t  *       resid_offset,
	int                     *       io_errno,
	memory_object_return_t          should_return,
	int                             flags,
	vm_prot_t                       prot)
{
	vm_object_t     object;

	  
                              
    
	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if ((prot & ~VM_PROT_ALL) != 0 && prot != VM_PROT_NO_CHANGE) {
		return KERN_INVALID_ARGUMENT;
	}

	size = round_page_64(size);

	  
                                                      
                                                            
    
	vm_object_lock(object);
	vm_object_paging_begin(object);

	if (flags & MEMORY_OBJECT_DATA_FLUSH_ALL) {
		if ((should_return != MEMORY_OBJECT_RETURN_NONE) || offset || object->copy) {
			flags &= ~MEMORY_OBJECT_DATA_FLUSH_ALL;
			flags |= MEMORY_OBJECT_DATA_FLUSH;
		}
	}
	offset -= object->paging_offset;

	if (flags & MEMORY_OBJECT_DATA_FLUSH_ALL) {
		vm_object_reap_pages(object, REAP_DATA_FLUSH);
	} else {
		(void)vm_object_update(object, offset, size, resid_offset,
		    io_errno, should_return, flags, prot);
	}

	vm_object_paging_end(object);
	vm_object_unlock(object);

	return KERN_SUCCESS;
}

  
                                           
  
                                                                    
                                                                   
                                                                 
                             
                                                                 
                                                                        
                  
                                                                   
                                                                      
                                                            
                                                                    
                                                        
   

kern_return_t
memory_object_release_name(
	memory_object_control_t control,
	int                             flags)
{
	vm_object_t     object;

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return vm_object_release_name(object, flags);
}



  
                                                  
           
                                          
                                                 
                     
   
kern_return_t
memory_object_destroy(
	memory_object_control_t control,
	kern_return_t           reason)
{
	vm_object_t             object;

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return vm_object_destroy(object, reason);
}

  
                          
  
                                                         
                                                          
                                                         
                  
  
                                                             
                                                             
                                                           
                                                           
           
  
                                                              
                                                            
                                                            
                                     
  
           
                                                
                   
   

boolean_t
vm_object_sync(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	boolean_t               should_flush,
	boolean_t               should_return,
	boolean_t               should_iosync)
{
	boolean_t       rv;
	int             flags;

	  
                                                      
                                                    
                    
    
	vm_object_lock(object);
	vm_object_paging_begin(object);

	if (should_flush) {
		flags = MEMORY_OBJECT_DATA_FLUSH;
		  
                                                            
                                           
                                                             
                                                             
                                            
     
		flags |= MEMORY_OBJECT_DATA_NO_CHANGE;
	} else {
		flags = 0;
	}

	if (should_iosync) {
		flags |= MEMORY_OBJECT_IO_SYNC;
	}

	rv = vm_object_update(object, offset, (vm_object_size_t)size, NULL, NULL,
	    (should_return) ?
	    MEMORY_OBJECT_RETURN_ALL :
	    MEMORY_OBJECT_RETURN_NONE,
	    flags,
	    VM_PROT_NO_CHANGE);


	vm_object_paging_end(object);
	vm_object_unlock(object);
	return rv;
}



#define LIST_REQ_PAGEOUT_PAGES(object, data_cnt, po, ro, ioerr, iosync)    \
MACRO_BEGIN                                                             \
                                                                        \
	int			upl_flags;                              \
	memory_object_t		pager;                                  \
                                                                        \
	if ((pager = (object)->pager) != MEMORY_OBJECT_NULL) {          \
	        vm_object_paging_begin(object);                         \
	        vm_object_unlock(object);                               \
                                                                        \
	        if (iosync)                                             \
	                upl_flags = UPL_MSYNC | UPL_IOSYNC;             \
	        else                                                    \
	                upl_flags = UPL_MSYNC;                          \
                                                                        \
	        (void) memory_object_data_return(pager,                 \
	                po,                                             \
	                (memory_object_cluster_size_t)data_cnt,         \
	                ro,                                             \
	                ioerr,                                          \
	                FALSE,                                          \
	                FALSE,                                          \
	                upl_flags);                                     \
                                                                        \
	        vm_object_lock(object);                                 \
	        vm_object_paging_end(object);                           \
	}                                                               \
MACRO_END

extern struct vnode *
vnode_pager_lookup_vnode(memory_object_t);

static int
vm_object_update_extent(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_offset_t      offset_end,
	vm_object_offset_t      *offset_resid,
	int                     *io_errno,
	boolean_t               should_flush,
	memory_object_return_t  should_return,
	boolean_t               should_iosync,
	vm_prot_t               prot)
{
	vm_page_t       m;
	int             retval = 0;
	vm_object_offset_t      paging_offset = 0;
	vm_object_offset_t      next_offset = offset;
	memory_object_lock_result_t     page_lock_result;
	memory_object_cluster_size_t    data_cnt = 0;
	struct  vm_page_delayed_work    dw_array;
	struct  vm_page_delayed_work    *dwp, *dwp_start;
	bool            dwp_finish_ctx = TRUE;
	int             dw_count;
	int             dw_limit;
	int             dirty_count;

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

	dirty_count = 0;

	for (;
	    offset < offset_end && object->resident_page_count;
	    offset += PAGE_SIZE_64) {
		  
                                                                    
                                           
     
		if (data_cnt) {
			if ((data_cnt >= MAX_UPL_TRANSFER_BYTES) || (next_offset != offset)) {
				if (dw_count) {
					vm_page_do_delayed_work(object, VM_KERN_MEMORY_NONE, dwp_start, dw_count);
					dwp = dwp_start;
					dw_count = 0;
				}
				LIST_REQ_PAGEOUT_PAGES(object, data_cnt,
				    paging_offset, offset_resid, io_errno, should_iosync);
				data_cnt = 0;
			}
		}
		while ((m = vm_page_lookup(object, offset)) != VM_PAGE_NULL) {
			dwp->dw_mask = 0;

			page_lock_result = memory_object_lock_page(m, should_return, should_flush, prot);

			if (data_cnt && page_lock_result != MEMORY_OBJECT_LOCK_RESULT_MUST_RETURN) {
				  
                                            
       
				if (dw_count) {
					vm_page_do_delayed_work(object, VM_KERN_MEMORY_NONE, dwp_start, dw_count);
					dwp = dwp_start;
					dw_count = 0;
				}
				LIST_REQ_PAGEOUT_PAGES(object, data_cnt,
				    paging_offset, offset_resid, io_errno, should_iosync);
				  
                                                                  
                                                                    
                         
       
				data_cnt = 0;
				continue;
			}

			switch (page_lock_result) {
			case MEMORY_OBJECT_LOCK_RESULT_DONE:
				break;

			case MEMORY_OBJECT_LOCK_RESULT_MUST_FREE:
				if (m->vmp_dirty == TRUE) {
					dirty_count++;
				}
				dwp->dw_mask |= DW_vm_page_free;
				break;

			case MEMORY_OBJECT_LOCK_RESULT_MUST_BLOCK:
				PAGE_SLEEP(object, m, THREAD_UNINT);
				continue;

			case MEMORY_OBJECT_LOCK_RESULT_MUST_RETURN:
				if (data_cnt == 0) {
					paging_offset = offset;
				}

				data_cnt += PAGE_SIZE;
				next_offset = offset + PAGE_SIZE_64;

				  
                                           
                                      
                             
       
				if (!VM_PAGE_WIRED(m)) {
					if (should_flush) {
						  
                                           
         
						m->vmp_free_when_done = TRUE;
					}
					  
                                                         
                                                  
                                                      
       
               
                                                 
        
				}
				retval = 1;
				break;
			}
			if (dwp->dw_mask) {
				VM_PAGE_ADD_DELAYED_WORK(dwp, m, dw_count);

				if (dw_count >= dw_limit) {
					vm_page_do_delayed_work(object, VM_KERN_MEMORY_NONE, dwp_start, dw_count);
					dwp = dwp_start;
					dw_count = 0;
				}
			}
			break;
		}
	}

	if (object->pager) {
		task_update_logical_writes(current_task(), (dirty_count * PAGE_SIZE), TASK_WRITE_INVALIDATED, vnode_pager_lookup_vnode(object->pager));
	}
	  
                                                    
                                         
    
	if (dw_count) {
		vm_page_do_delayed_work(object, VM_KERN_MEMORY_NONE, dwp_start, dw_count);
	}

	if (data_cnt) {
		LIST_REQ_PAGEOUT_PAGES(object, data_cnt,
		    paging_offset, offset_resid, io_errno, should_iosync);
	}

	if (dwp_start && dwp_finish_ctx) {
		vm_page_delayed_work_finish_ctx(dwp_start);
		dwp_start = dwp = NULL;
	}

	return retval;
}



  
                            
               
                                                      
  
                                                   
   
kern_return_t
vm_object_update(
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_object_size_t        size,
	vm_object_offset_t      *resid_offset,
	int                     *io_errno,
	memory_object_return_t  should_return,
	int                     flags,
	vm_prot_t               protection)
{
	vm_object_t             copy_object = VM_OBJECT_NULL;
	boolean_t               data_returned = FALSE;
	boolean_t               update_cow;
	boolean_t               should_flush = (flags & MEMORY_OBJECT_DATA_FLUSH) ? TRUE : FALSE;
	boolean_t               should_iosync = (flags & MEMORY_OBJECT_IO_SYNC) ? TRUE : FALSE;
	vm_fault_return_t       result;
	int                     num_of_extents;
	int                     n;
#define MAX_EXTENTS     8
#define EXTENT_SIZE     (1024 * 1024 * 256)
#define RESIDENT_LIMIT  (1024 * 32)
	struct extent {
		vm_object_offset_t e_base;
		vm_object_offset_t e_min;
		vm_object_offset_t e_max;
	} extents[MAX_EXTENTS];

	  
                                                    
                                          
   
                                                      
                                                     
                                                           
                                                      
                                                     
    

	  
                                                                 
                                                         
                                                
    

	update_cow = ((flags & MEMORY_OBJECT_DATA_FLUSH)
	    && (!(flags & MEMORY_OBJECT_DATA_NO_CHANGE) &&
	    !(flags & MEMORY_OBJECT_DATA_PURGE)))
	    || (flags & MEMORY_OBJECT_COPY_SYNC);

	if (update_cow || (flags & (MEMORY_OBJECT_DATA_PURGE | MEMORY_OBJECT_DATA_SYNC))) {
		int collisions = 0;

		while ((copy_object = object->copy) != VM_OBJECT_NULL) {
			  
                                                         
                                                          
                                                            
                                                               
      
			if (vm_object_lock_try(copy_object)) {
				  
                                              
                                                    
                                                       
                                                        
                                                            
                                                     
                                                                
       
				vm_object_unlock(object);
				vm_object_reference_locked(copy_object);
				break;
			}
			vm_object_unlock(object);

			collisions++;
			mutex_pause(collisions);

			vm_object_lock(object);
		}
	}
	if ((copy_object != VM_OBJECT_NULL && update_cow) || (flags & MEMORY_OBJECT_DATA_SYNC)) {
		vm_object_offset_t      i;
		vm_object_size_t        copy_size;
		vm_object_offset_t      copy_offset;
		vm_prot_t               prot;
		vm_page_t               page;
		vm_page_t               top_page;
		kern_return_t           error = 0;
		struct vm_object_fault_info fault_info = {};

		if (copy_object != VM_OBJECT_NULL) {
			  
                                                      
      
			copy_offset = (offset >= copy_object->vo_shadow_offset) ?
			    (offset - copy_object->vo_shadow_offset) : 0;

			if (copy_offset > copy_object->vo_size) {
				copy_offset = copy_object->vo_size;
			}

			  
                                             
      
			if (offset >= copy_object->vo_shadow_offset) {
				copy_size = size;
			} else if (size >= copy_object->vo_shadow_offset - offset) {
				copy_size = (size - (copy_object->vo_shadow_offset - offset));
			} else {
				copy_size = 0;
			}

			if (copy_offset + copy_size > copy_object->vo_size) {
				if (copy_object->vo_size >= copy_offset) {
					copy_size = copy_object->vo_size - copy_offset;
				} else {
					copy_size = 0;
				}
			}
			copy_size += copy_offset;
		} else {
			copy_object = object;

			copy_size   = offset + size;
			copy_offset = offset;
		}
		fault_info.interruptible = THREAD_UNINT;
		fault_info.behavior  = VM_BEHAVIOR_SEQUENTIAL;
		fault_info.lo_offset = copy_offset;
		fault_info.hi_offset = copy_size;
		fault_info.stealth = TRUE;
		assert(fault_info.cs_bypass == FALSE);
		assert(fault_info.pmap_cs_associated == FALSE);

		vm_object_paging_begin(copy_object);

		for (i = copy_offset; i < copy_size; i += PAGE_SIZE) {
RETRY_COW_OF_LOCK_REQUEST:
			fault_info.cluster_size = (vm_size_t) (copy_size - i);
			assert(fault_info.cluster_size == copy_size - i);

			prot =  VM_PROT_WRITE | VM_PROT_READ;
			page = VM_PAGE_NULL;
			result = vm_fault_page(copy_object, i,
			    VM_PROT_WRITE | VM_PROT_READ,
			    FALSE,
			    FALSE,                                            
			    &prot,
			    &page,
			    &top_page,
			    (int *)0,
			    &error,
			    FALSE,
			    FALSE, &fault_info);

			switch (result) {
			case VM_FAULT_SUCCESS:
				if (top_page) {
					vm_fault_cleanup(
						VM_PAGE_OBJECT(page), top_page);
					vm_object_lock(copy_object);
					vm_object_paging_begin(copy_object);
				}
				if ((!VM_PAGE_NON_SPECULATIVE_PAGEABLE(page))) {
					vm_page_lockspin_queues();

					if ((!VM_PAGE_NON_SPECULATIVE_PAGEABLE(page))) {
						vm_page_deactivate(page);
					}
					vm_page_unlock_queues();
				}
				PAGE_WAKEUP_DONE(page);
				break;
			case VM_FAULT_RETRY:
				prot =  VM_PROT_WRITE | VM_PROT_READ;
				vm_object_lock(copy_object);
				vm_object_paging_begin(copy_object);
				goto RETRY_COW_OF_LOCK_REQUEST;
			case VM_FAULT_INTERRUPTED:
				prot =  VM_PROT_WRITE | VM_PROT_READ;
				vm_object_lock(copy_object);
				vm_object_paging_begin(copy_object);
				goto RETRY_COW_OF_LOCK_REQUEST;
			case VM_FAULT_MEMORY_SHORTAGE:
				VM_PAGE_WAIT();
				prot =  VM_PROT_WRITE | VM_PROT_READ;
				vm_object_lock(copy_object);
				vm_object_paging_begin(copy_object);
				goto RETRY_COW_OF_LOCK_REQUEST;
			case VM_FAULT_SUCCESS_NO_VM_PAGE:
				                                  
				vm_object_paging_end(copy_object);
				vm_object_unlock(copy_object);
				OS_FALLTHROUGH;
			case VM_FAULT_MEMORY_ERROR:
				if (object != copy_object) {
					vm_object_deallocate(copy_object);
				}
				vm_object_lock(object);
				goto BYPASS_COW_COPYIN;
			default:
				panic("vm_object_update: unexpected error 0x%x"
				    " from vm_fault_page()\n", result);
			}
		}
		vm_object_paging_end(copy_object);
	}
	if ((flags & (MEMORY_OBJECT_DATA_SYNC | MEMORY_OBJECT_COPY_SYNC))) {
		if (copy_object != VM_OBJECT_NULL && copy_object != object) {
			vm_object_unlock(copy_object);
			vm_object_deallocate(copy_object);
			vm_object_lock(object);
		}
		return KERN_SUCCESS;
	}
	if (copy_object != VM_OBJECT_NULL && copy_object != object) {
		if ((flags & MEMORY_OBJECT_DATA_PURGE)) {
			vm_object_lock_assert_exclusive(copy_object);
			copy_object->shadow_severed = TRUE;
			copy_object->shadowed = FALSE;
			copy_object->shadow = NULL;
			  
                                                             
      
			vm_object_deallocate(object);
		}
		vm_object_unlock(copy_object);
		vm_object_deallocate(copy_object);
		vm_object_lock(object);
	}
BYPASS_COW_COPYIN:

	  
                                                       
                                                     
                                                     
                                                              
                                                            
                                                         
                                                              
                                                          
                                                            
   
                                                              
                                                           
                                                              
                                                                  
                                                                   
                                                                    
                                                                       
                                                                   
                                                                    
                                                                
                                
    

	if ((object->resident_page_count < RESIDENT_LIMIT) &&
	    (atop_64(size) > (unsigned)(object->resident_page_count / (8 * MAX_EXTENTS)))) {
		vm_page_t               next;
		vm_object_offset_t      start;
		vm_object_offset_t      end;
		vm_object_size_t        e_mask;
		vm_page_t               m;

		start = offset;
		end   = offset + size;
		num_of_extents = 0;
		e_mask = ~((vm_object_size_t)(EXTENT_SIZE - 1));

		m = (vm_page_t) vm_page_queue_first(&object->memq);

		while (!vm_page_queue_end(&object->memq, (vm_page_queue_entry_t) m)) {
			next = (vm_page_t) vm_page_queue_next(&m->vmp_listq);

			if ((m->vmp_offset >= start) && (m->vmp_offset < end)) {
				  
                                         
                                          
       
				for (n = 0; n < num_of_extents; n++) {
					if ((m->vmp_offset & e_mask) == extents[n].e_base) {
						  
                                             
                                            
                                            
         
						if (m->vmp_offset < extents[n].e_min) {
							extents[n].e_min = m->vmp_offset;
						} else if ((m->vmp_offset + (PAGE_SIZE - 1)) > extents[n].e_max) {
							extents[n].e_max = m->vmp_offset + (PAGE_SIZE - 1);
						}
						break;
					}
				}
				if (n == num_of_extents) {
					  
                                                       
                 
        
					if (n < MAX_EXTENTS) {
						  
                               
                            
         
						extents[n].e_base = m->vmp_offset & e_mask;
						extents[n].e_min  = m->vmp_offset;
						extents[n].e_max  = m->vmp_offset + (PAGE_SIZE - 1);

						num_of_extents++;
					} else {
						  
                                          
                                           
                                        
                                                    
                                                
                                                
                                               
         
						for (n = 1; n < num_of_extents; n++) {
							if (extents[n].e_min < extents[0].e_min) {
								extents[0].e_min = extents[n].e_min;
							}
							if (extents[n].e_max > extents[0].e_max) {
								extents[0].e_max = extents[n].e_max;
							}
						}
						  
                                                     
                                             
                                       
         
						extents[0].e_base = 0;
						e_mask = 0;
						num_of_extents = 1;

						  
                                           
                                              
                                    
         
						continue;
					}
				}
			}
			m = next;
		}
	} else {
		extents[0].e_min = offset;
		extents[0].e_max = offset + (size - 1);

		num_of_extents = 1;
	}
	for (n = 0; n < num_of_extents; n++) {
		if (vm_object_update_extent(object, extents[n].e_min, extents[n].e_max, resid_offset, io_errno,
		    should_flush, should_return, should_iosync, protection)) {
			data_returned = TRUE;
		}
	}
	return data_returned;
}


static kern_return_t
vm_object_set_attributes_common(
	vm_object_t     object,
	boolean_t       may_cache,
	memory_object_copy_strategy_t copy_strategy)
{
	boolean_t       object_became_ready;

	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	  
                                       
    

	switch (copy_strategy) {
	case MEMORY_OBJECT_COPY_NONE:
	case MEMORY_OBJECT_COPY_DELAY:
		break;
	default:
		return KERN_INVALID_ARGUMENT;
	}

	if (may_cache) {
		may_cache = TRUE;
	}

	vm_object_lock(object);

	  
                       
    
	assert(!object->internal);
	object_became_ready = !object->pager_ready;
	object->copy_strategy = copy_strategy;
	object->can_persist = may_cache;

	  
                                                  
                       
    

	if (object_became_ready) {
		object->pager_ready = TRUE;
		vm_object_wakeup(object, VM_OBJECT_EVENT_PAGER_READY);
	}

	vm_object_unlock(object);

	return KERN_SUCCESS;
}


kern_return_t
memory_object_synchronize_completed(
	__unused    memory_object_control_t control,
	__unused    memory_object_offset_t  offset,
	__unused    memory_object_size_t    length)
{
	panic("memory_object_synchronize_completed no longer supported\n");
	return KERN_FAILURE;
}


  
                                               
  
                                                                 
                                                                 
                                                             
                                
   
kern_return_t
memory_object_change_attributes(
	memory_object_control_t         control,
	memory_object_flavor_t          flavor,
	memory_object_info_t            attributes,
	mach_msg_type_number_t          count)
{
	vm_object_t                     object;
	kern_return_t                   result = KERN_SUCCESS;
	boolean_t                       may_cache;
	boolean_t                       invalidate;
	memory_object_copy_strategy_t   copy_strategy;

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	vm_object_lock(object);

	may_cache = object->can_persist;
	copy_strategy = object->copy_strategy;
#if notyet
	invalidate = object->invalidate;
#endif
	vm_object_unlock(object);

	switch (flavor) {
	case OLD_MEMORY_OBJECT_BEHAVIOR_INFO:
	{
		old_memory_object_behave_info_t     behave;

		if (count != OLD_MEMORY_OBJECT_BEHAVE_INFO_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		behave = (old_memory_object_behave_info_t) attributes;

		invalidate = behave->invalidate;
		copy_strategy = behave->copy_strategy;

		break;
	}

	case MEMORY_OBJECT_BEHAVIOR_INFO:
	{
		memory_object_behave_info_t     behave;

		if (count != MEMORY_OBJECT_BEHAVE_INFO_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		behave = (memory_object_behave_info_t) attributes;

		invalidate = behave->invalidate;
		copy_strategy = behave->copy_strategy;
		break;
	}

	case MEMORY_OBJECT_PERFORMANCE_INFO:
	{
		memory_object_perf_info_t       perf;

		if (count != MEMORY_OBJECT_PERF_INFO_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		perf = (memory_object_perf_info_t) attributes;

		may_cache = perf->may_cache;

		break;
	}

	case OLD_MEMORY_OBJECT_ATTRIBUTE_INFO:
	{
		old_memory_object_attr_info_t   attr;

		if (count != OLD_MEMORY_OBJECT_ATTR_INFO_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		attr = (old_memory_object_attr_info_t) attributes;

		may_cache = attr->may_cache;
		copy_strategy = attr->copy_strategy;

		break;
	}

	case MEMORY_OBJECT_ATTRIBUTE_INFO:
	{
		memory_object_attr_info_t       attr;

		if (count != MEMORY_OBJECT_ATTR_INFO_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		attr = (memory_object_attr_info_t) attributes;

		copy_strategy = attr->copy_strategy;
		may_cache = attr->may_cache_object;

		break;
	}

	default:
		result = KERN_INVALID_ARGUMENT;
		break;
	}

	if (result != KERN_SUCCESS) {
		return result;
	}

	if (copy_strategy == MEMORY_OBJECT_COPY_TEMPORARY) {
		copy_strategy = MEMORY_OBJECT_COPY_DELAY;
	}

	  
                                                            
                              
    
	return vm_object_set_attributes_common(object,
	           may_cache,
	           copy_strategy);
}

kern_return_t
memory_object_get_attributes(
	memory_object_control_t control,
	memory_object_flavor_t  flavor,
	memory_object_info_t    attributes,                               
	mach_msg_type_number_t  *count)                     
{
	kern_return_t           ret = KERN_SUCCESS;
	vm_object_t             object;

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	vm_object_lock(object);

	switch (flavor) {
	case OLD_MEMORY_OBJECT_BEHAVIOR_INFO:
	{
		old_memory_object_behave_info_t behave;

		if (*count < OLD_MEMORY_OBJECT_BEHAVE_INFO_COUNT) {
			ret = KERN_INVALID_ARGUMENT;
			break;
		}

		behave = (old_memory_object_behave_info_t) attributes;
		behave->copy_strategy = object->copy_strategy;
		behave->temporary = FALSE;
#if notyet                                                                 
		behave->invalidate = object->invalidate;
#else
		behave->invalidate = FALSE;
#endif

		*count = OLD_MEMORY_OBJECT_BEHAVE_INFO_COUNT;
		break;
	}

	case MEMORY_OBJECT_BEHAVIOR_INFO:
	{
		memory_object_behave_info_t     behave;

		if (*count < MEMORY_OBJECT_BEHAVE_INFO_COUNT) {
			ret = KERN_INVALID_ARGUMENT;
			break;
		}

		behave = (memory_object_behave_info_t) attributes;
		behave->copy_strategy = object->copy_strategy;
		behave->temporary = FALSE;
#if notyet                                                                 
		behave->invalidate = object->invalidate;
#else
		behave->invalidate = FALSE;
#endif
		behave->advisory_pageout = FALSE;
		behave->silent_overwrite = FALSE;
		*count = MEMORY_OBJECT_BEHAVE_INFO_COUNT;
		break;
	}

	case MEMORY_OBJECT_PERFORMANCE_INFO:
	{
		memory_object_perf_info_t       perf;

		if (*count < MEMORY_OBJECT_PERF_INFO_COUNT) {
			ret = KERN_INVALID_ARGUMENT;
			break;
		}

		perf = (memory_object_perf_info_t) attributes;
		perf->cluster_size = PAGE_SIZE;
		perf->may_cache = object->can_persist;

		*count = MEMORY_OBJECT_PERF_INFO_COUNT;
		break;
	}

	case OLD_MEMORY_OBJECT_ATTRIBUTE_INFO:
	{
		old_memory_object_attr_info_t       attr;

		if (*count < OLD_MEMORY_OBJECT_ATTR_INFO_COUNT) {
			ret = KERN_INVALID_ARGUMENT;
			break;
		}

		attr = (old_memory_object_attr_info_t) attributes;
		attr->may_cache = object->can_persist;
		attr->copy_strategy = object->copy_strategy;

		*count = OLD_MEMORY_OBJECT_ATTR_INFO_COUNT;
		break;
	}

	case MEMORY_OBJECT_ATTRIBUTE_INFO:
	{
		memory_object_attr_info_t       attr;

		if (*count < MEMORY_OBJECT_ATTR_INFO_COUNT) {
			ret = KERN_INVALID_ARGUMENT;
			break;
		}

		attr = (memory_object_attr_info_t) attributes;
		attr->copy_strategy = object->copy_strategy;
		attr->cluster_size = PAGE_SIZE;
		attr->may_cache_object = object->can_persist;
		attr->temporary = FALSE;

		*count = MEMORY_OBJECT_ATTR_INFO_COUNT;
		break;
	}

	default:
		ret = KERN_INVALID_ARGUMENT;
		break;
	}

	vm_object_unlock(object);

	return ret;
}


kern_return_t
memory_object_iopl_request(
	ipc_port_t              port,
	memory_object_offset_t  offset,
	upl_size_t              *upl_size,
	upl_t                   *upl_ptr,
	upl_page_info_array_t   user_page_list,
	unsigned int            *page_list_count,
	upl_control_flags_t     *flags,
	vm_tag_t                tag)
{
	vm_object_t             object;
	kern_return_t           ret;
	upl_control_flags_t     caller_flags;

	caller_flags = *flags;

	if (caller_flags & ~UPL_VALID_FLAGS) {
		  
                                      
                             
     
		return KERN_INVALID_VALUE;
	}

	if (ip_kotype(port) == IKOT_NAMED_ENTRY) {
		vm_named_entry_t        named_entry;

		named_entry = (vm_named_entry_t) ip_get_kobject(port);
		                                                     
		if (*upl_size == 0) {
			if (offset >= named_entry->size) {
				return KERN_INVALID_RIGHT;
			}
			*upl_size = (upl_size_t)(named_entry->size - offset);
			if (*upl_size != named_entry->size - offset) {
				return KERN_INVALID_ARGUMENT;
			}
		}
		if (caller_flags & UPL_COPYOUT_FROM) {
			if ((named_entry->protection & VM_PROT_READ)
			    != VM_PROT_READ) {
				return KERN_INVALID_RIGHT;
			}
		} else {
			if ((named_entry->protection &
			    (VM_PROT_READ | VM_PROT_WRITE))
			    != (VM_PROT_READ | VM_PROT_WRITE)) {
				return KERN_INVALID_RIGHT;
			}
		}
		if (named_entry->size < (offset + *upl_size)) {
			return KERN_INVALID_ARGUMENT;
		}

		                                                       
		                                                           
		offset = offset + named_entry->offset;
		offset += named_entry->data_offset;

		if (named_entry->is_sub_map ||
		    named_entry->is_copy) {
			return KERN_INVALID_ARGUMENT;
		}
		if (!named_entry->is_object) {
			return KERN_INVALID_ARGUMENT;
		}

		named_entry_lock(named_entry);

		object = vm_named_entry_to_vm_object(named_entry);
		assert(object != VM_OBJECT_NULL);
		vm_object_reference(object);
		named_entry_unlock(named_entry);
	} else if (ip_kotype(port) == IKOT_MEM_OBJ_CONTROL) {
		panic("unexpected IKOT_MEM_OBJ_CONTROL: %p", port);
	} else {
		return KERN_INVALID_ARGUMENT;
	}
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (!object->private) {
		if (object->phys_contiguous) {
			*flags = UPL_PHYS_CONTIG;
		} else {
			*flags = 0;
		}
	} else {
		*flags = UPL_DEV_MEMORY | UPL_PHYS_CONTIG;
	}

	ret = vm_object_iopl_request(object,
	    offset,
	    *upl_size,
	    upl_ptr,
	    user_page_list,
	    page_list_count,
	    caller_flags,
	    tag);
	vm_object_deallocate(object);
	return ret;
}

  
                                                 
           
                                                     
                                                     
                                                           
  
   

kern_return_t
memory_object_upl_request(
	memory_object_control_t control,
	memory_object_offset_t  offset,
	upl_size_t              size,
	upl_t                   *upl_ptr,
	upl_page_info_array_t   user_page_list,
	unsigned int            *page_list_count,
	int                     cntrl_flags,
	int                     tag)
{
	vm_object_t             object;
	vm_tag_t                vmtag = (vm_tag_t)tag;
	assert(vmtag == tag);

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_TERMINATED;
	}

	return vm_object_upl_request(object,
	           offset,
	           size,
	           upl_ptr,
	           user_page_list,
	           page_list_count,
	           (upl_control_flags_t)(unsigned int) cntrl_flags,
	           vmtag);
}

  
                                                       
           
                                                    
                                                      
                                                     
                                                           
                                                       
                          
   

kern_return_t
memory_object_super_upl_request(
	memory_object_control_t control,
	memory_object_offset_t  offset,
	upl_size_t              size,
	upl_size_t              super_cluster,
	upl_t                   *upl,
	upl_page_info_t         *user_page_list,
	unsigned int            *page_list_count,
	int                     cntrl_flags,
	int                     tag)
{
	vm_object_t             object;
	vm_tag_t                vmtag = (vm_tag_t)tag;
	assert(vmtag == tag);

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return vm_object_super_upl_request(object,
	           offset,
	           size,
	           super_cluster,
	           upl,
	           user_page_list,
	           page_list_count,
	           (upl_control_flags_t)(unsigned int) cntrl_flags,
	           vmtag);
}

kern_return_t
memory_object_cluster_size(
	memory_object_control_t control,
	memory_object_offset_t  *start,
	vm_size_t               *length,
	uint32_t                *io_streaming,
	memory_object_fault_info_t mo_fault_info)
{
	vm_object_t             object;
	vm_object_fault_info_t  fault_info;

	object = memory_object_control_to_vm_object(control);

	if (object == VM_OBJECT_NULL || object->paging_offset > *start) {
		return KERN_INVALID_ARGUMENT;
	}

	*start -= object->paging_offset;

	fault_info = (vm_object_fault_info_t)(uintptr_t) mo_fault_info;
	vm_object_cluster_size(object,
	    (vm_object_offset_t *)start,
	    length,
	    fault_info,
	    io_streaming);

	*start += object->paging_offset;

	return KERN_SUCCESS;
}


  
                                                   
           
                                                               
         
  
                                                          
   
kern_return_t
host_default_memory_manager(
	host_priv_t             host_priv,
	memory_object_default_t *default_manager,
	__unused memory_object_cluster_size_t cluster_size)
{
	memory_object_default_t current_manager;
	memory_object_default_t new_manager;
	memory_object_default_t returned_manager;
	kern_return_t result = KERN_SUCCESS;

	if (host_priv == HOST_PRIV_NULL) {
		return KERN_INVALID_HOST;
	}

	new_manager = *default_manager;
	lck_mtx_lock(&memory_manager_default_lock);
	current_manager = memory_manager_default;
	returned_manager = MEMORY_OBJECT_DEFAULT_NULL;

	if (new_manager == MEMORY_OBJECT_DEFAULT_NULL) {
		  
                                
     
		returned_manager = current_manager;
		memory_object_default_reference(returned_manager);
	} else {
		  
                                               
     
		extern task_t kernel_task;
		if (current_task() != kernel_task) {
			result = KERN_NO_ACCESS;
			goto out;
		}

		  
                                                 
                                   
     
		if (current_manager == MEMORY_OBJECT_DEFAULT_NULL) {
			result = vm_pageout_internal_start();
			if (result != KERN_SUCCESS) {
				goto out;
			}
		}

		  
                                
                                            
                                              
                                               
         
     
		returned_manager = current_manager;
		memory_manager_default = new_manager;
		memory_object_default_reference(new_manager);

		  
                                               
                                             
     

		thread_wakeup((event_t) &memory_manager_default);

		  
                                                           
                                                              
               
     
		if (current_manager == MEMORY_OBJECT_DEFAULT_NULL) {
			vm_page_reactivate_all_throttled();
		}
	}
out:
	lck_mtx_unlock(&memory_manager_default_lock);

	*default_manager = returned_manager;
	return result;
}

  
                                            
           
                                              
                                                 
                                   
   

__private_extern__ memory_object_default_t
memory_manager_default_reference(void)
{
	memory_object_default_t current_manager;

	lck_mtx_lock(&memory_manager_default_lock);
	current_manager = memory_manager_default;
	while (current_manager == MEMORY_OBJECT_DEFAULT_NULL) {
		wait_result_t res;

		res = lck_mtx_sleep(&memory_manager_default_lock,
		    LCK_SLEEP_DEFAULT,
		    (event_t) &memory_manager_default,
		    THREAD_UNINT);
		assert(res == THREAD_AWAKENED);
		current_manager = memory_manager_default;
	}
	memory_object_default_reference(current_manager);
	lck_mtx_unlock(&memory_manager_default_lock);

	return current_manager;
}

  
                                        
  
           
                                                       
                                                       
                                           
  
                                                        
                            
  
   
__private_extern__ kern_return_t
memory_manager_default_check(void)
{
	memory_object_default_t current;

	lck_mtx_lock(&memory_manager_default_lock);
	current = memory_manager_default;
	if (current == MEMORY_OBJECT_DEFAULT_NULL) {
		static boolean_t logged;                              
		boolean_t       complain = !logged;
		logged = TRUE;
		lck_mtx_unlock(&memory_manager_default_lock);
		if (complain) {
			printf("Warning: No default memory manager\n");
		}
		return KERN_FAILURE;
	} else {
		lck_mtx_unlock(&memory_manager_default_lock);
		return KERN_SUCCESS;
	}
}

                                                                            
                                                                        

kern_return_t
memory_object_page_op(
	memory_object_control_t control,
	memory_object_offset_t  offset,
	int                     ops,
	ppnum_t                 *phys_entry,
	int                     *flags)
{
	vm_object_t             object;

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return vm_object_page_op(object, offset, ops, phys_entry, flags);
}

  
                                                             
                                                                        
                                                                            
                                                                          
                                                                           
                                                                             
                                                                                
   

kern_return_t
memory_object_range_op(
	memory_object_control_t control,
	memory_object_offset_t  offset_beg,
	memory_object_offset_t  offset_end,
	int                     ops,
	int                     *range)
{
	vm_object_t             object;

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return vm_object_range_op(object,
	           offset_beg,
	           offset_end,
	           ops,
	           (uint32_t *) range);
}


void
memory_object_mark_used(
	memory_object_control_t control)
{
	vm_object_t             object;

	if (control == NULL) {
		return;
	}

	object = memory_object_control_to_vm_object(control);

	if (object != VM_OBJECT_NULL) {
		vm_object_cache_remove(object);
	}
}


void
memory_object_mark_unused(
	memory_object_control_t control,
	__unused boolean_t      rage)
{
	vm_object_t             object;

	if (control == NULL) {
		return;
	}

	object = memory_object_control_to_vm_object(control);

	if (object != VM_OBJECT_NULL) {
		vm_object_cache_add(object);
	}
}

void
memory_object_mark_io_tracking(
	memory_object_control_t control)
{
	vm_object_t             object;

	if (control == NULL) {
		return;
	}
	object = memory_object_control_to_vm_object(control);

	if (object != VM_OBJECT_NULL) {
		vm_object_lock(object);
		object->io_tracking = TRUE;
		vm_object_unlock(object);
	}
}

void
memory_object_mark_trusted(
	memory_object_control_t control)
{
	vm_object_t             object;

	if (control == NULL) {
		return;
	}
	object = memory_object_control_to_vm_object(control);

	if (object != VM_OBJECT_NULL) {
		vm_object_lock(object);
		object->pager_trusted = TRUE;
		vm_object_unlock(object);
	}
}

#if CONFIG_SECLUDED_MEMORY
void
memory_object_mark_eligible_for_secluded(
	memory_object_control_t control,
	boolean_t               eligible_for_secluded)
{
	vm_object_t             object;

	if (control == NULL) {
		return;
	}
	object = memory_object_control_to_vm_object(control);

	if (object == VM_OBJECT_NULL) {
		return;
	}

	vm_object_lock(object);
	if (eligible_for_secluded &&
	    secluded_for_filecache &&                      
	    !object->eligible_for_secluded) {
		object->eligible_for_secluded = TRUE;
		vm_page_secluded.eligible_for_secluded += object->resident_page_count;
	} else if (!eligible_for_secluded &&
	    object->eligible_for_secluded) {
		object->eligible_for_secluded = FALSE;
		vm_page_secluded.eligible_for_secluded -= object->resident_page_count;
		if (object->resident_page_count) {
			                                                     
			                                                                                                          
		}
	}
	vm_object_unlock(object);
}
#endif                             

kern_return_t
memory_object_pages_resident(
	memory_object_control_t control,
	boolean_t                       *       has_pages_resident)
{
	vm_object_t             object;

	*has_pages_resident = FALSE;

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (object->resident_page_count) {
		*has_pages_resident = TRUE;
	}

	return KERN_SUCCESS;
}

kern_return_t
memory_object_signed(
	memory_object_control_t control,
	boolean_t               is_signed)
{
	vm_object_t     object;

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	vm_object_lock(object);
	object->code_signed = is_signed;
	vm_object_unlock(object);

	return KERN_SUCCESS;
}

boolean_t
memory_object_is_signed(
	memory_object_control_t control)
{
	boolean_t       is_signed;
	vm_object_t     object;

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return FALSE;
	}

	vm_object_lock_shared(object);
	is_signed = object->code_signed;
	vm_object_unlock(object);

	return is_signed;
}

boolean_t
memory_object_is_shared_cache(
	memory_object_control_t control)
{
	vm_object_t     object = VM_OBJECT_NULL;

	object = memory_object_control_to_vm_object(control);
	if (object == VM_OBJECT_NULL) {
		return FALSE;
	}

	return object->object_is_shared_cache;
}

__private_extern__ memory_object_control_t
memory_object_control_allocate(
	vm_object_t             object)
{
	return object;
}

__private_extern__ void
memory_object_control_collapse(
	memory_object_control_t *control,
	vm_object_t             object)
{
	*control = object;
}

__private_extern__ vm_object_t
memory_object_control_to_vm_object(
	memory_object_control_t control)
{
	return control;
}

__private_extern__ vm_object_t
memory_object_to_vm_object(
	memory_object_t mem_obj)
{
	memory_object_control_t mo_control;

	if (mem_obj == MEMORY_OBJECT_NULL) {
		return VM_OBJECT_NULL;
	}
	mo_control = mem_obj->mo_control;
	if (mo_control == NULL) {
		return VM_OBJECT_NULL;
	}
	return memory_object_control_to_vm_object(mo_control);
}

memory_object_control_t
convert_port_to_mo_control(
	__unused mach_port_t    port)
{
	return MEMORY_OBJECT_CONTROL_NULL;
}


mach_port_t
convert_mo_control_to_port(
	__unused memory_object_control_t        control)
{
	return MACH_PORT_NULL;
}

void
memory_object_control_reference(
	__unused memory_object_control_t        control)
{
	return;
}

  
                                                          
                                                            
                                    
   
void
memory_object_control_deallocate(
	__unused memory_object_control_t control)
{
}

void
memory_object_control_disable(
	memory_object_control_t *control)
{
	assert(*control != VM_OBJECT_NULL);
	*control = VM_OBJECT_NULL;
}

void
memory_object_default_reference(
	memory_object_default_t dmm)
{
	ipc_port_make_send(dmm);
}

void
memory_object_default_deallocate(
	memory_object_default_t dmm)
{
	ipc_port_release_send(dmm);
}

memory_object_t
convert_port_to_memory_object(
	__unused mach_port_t    port)
{
	return MEMORY_OBJECT_NULL;
}


mach_port_t
convert_memory_object_to_port(
	__unused memory_object_t        object)
{
	return MACH_PORT_NULL;
}


                                     
void
memory_object_reference(
	memory_object_t memory_object)
{
	(memory_object->mo_pager_ops->memory_object_reference)(
		memory_object);
}

                                      
void
memory_object_deallocate(
	memory_object_t memory_object)
{
	(memory_object->mo_pager_ops->memory_object_deallocate)(
		memory_object);
}


                                
kern_return_t
memory_object_init
(
	memory_object_t memory_object,
	memory_object_control_t memory_control,
	memory_object_cluster_size_t memory_object_page_size
)
{
	return (memory_object->mo_pager_ops->memory_object_init)(
		memory_object,
		memory_control,
		memory_object_page_size);
}

                                     
kern_return_t
memory_object_terminate
(
	memory_object_t memory_object
)
{
	return (memory_object->mo_pager_ops->memory_object_terminate)(
		memory_object);
}

                                        
kern_return_t
memory_object_data_request
(
	memory_object_t memory_object,
	memory_object_offset_t offset,
	memory_object_cluster_size_t length,
	vm_prot_t desired_access,
	memory_object_fault_info_t fault_info
)
{
	return (memory_object->mo_pager_ops->memory_object_data_request)(
		memory_object,
		offset,
		length,
		desired_access,
		fault_info);
}

                                       
kern_return_t
memory_object_data_return
(
	memory_object_t memory_object,
	memory_object_offset_t offset,
	memory_object_cluster_size_t size,
	memory_object_offset_t *resid_offset,
	int     *io_error,
	boolean_t dirty,
	boolean_t kernel_copy,
	int     upl_flags
)
{
	return (memory_object->mo_pager_ops->memory_object_data_return)(
		memory_object,
		offset,
		size,
		resid_offset,
		io_error,
		dirty,
		kernel_copy,
		upl_flags);
}

                                           
kern_return_t
memory_object_data_initialize
(
	memory_object_t memory_object,
	memory_object_offset_t offset,
	memory_object_cluster_size_t size
)
{
	return (memory_object->mo_pager_ops->memory_object_data_initialize)(
		memory_object,
		offset,
		size);
}

                                       
kern_return_t
memory_object_data_unlock
(
	memory_object_t memory_object,
	memory_object_offset_t offset,
	memory_object_size_t size,
	vm_prot_t desired_access
)
{
	return (memory_object->mo_pager_ops->memory_object_data_unlock)(
		memory_object,
		offset,
		size,
		desired_access);
}

                                       
kern_return_t
memory_object_synchronize
(
	memory_object_t memory_object,
	memory_object_offset_t offset,
	memory_object_size_t size,
	vm_sync_t sync_flags
)
{
	panic("memory_object_syncrhonize no longer supported\n");

	return (memory_object->mo_pager_ops->memory_object_synchronize)(
		memory_object,
		offset,
		size,
		sync_flags);
}


  
                                                                           
                                                                   
                                                                            
                                                                 
                                                      
                                                                           
                                                        
  
                                                                                
                                                                           
                                                                           
  
                                                                        
                                                                       
             
   

                               
kern_return_t
memory_object_map
(
	memory_object_t memory_object,
	vm_prot_t prot
)
{
	return (memory_object->mo_pager_ops->memory_object_map)(
		memory_object,
		prot);
}

                                      
kern_return_t
memory_object_last_unmap
(
	memory_object_t memory_object
)
{
	return (memory_object->mo_pager_ops->memory_object_last_unmap)(
		memory_object);
}

                                        
kern_return_t
memory_object_data_reclaim
(
	memory_object_t memory_object,
	boolean_t       reclaim_backing_store
)
{
	if (memory_object->mo_pager_ops->memory_object_data_reclaim == NULL) {
		return KERN_NOT_SUPPORTED;
	}
	return (memory_object->mo_pager_ops->memory_object_data_reclaim)(
		memory_object,
		reclaim_backing_store);
}

boolean_t
memory_object_backing_object
(
	memory_object_t memory_object,
	memory_object_offset_t offset,
	vm_object_t *backing_object,
	vm_object_offset_t *backing_offset)
{
	if (memory_object->mo_pager_ops->memory_object_backing_object == NULL) {
		return FALSE;
	}
	return (memory_object->mo_pager_ops->memory_object_backing_object)(
		memory_object,
		offset,
		backing_object,
		backing_offset);
}

upl_t
convert_port_to_upl(
	ipc_port_t      port)
{
	upl_t upl;

	ip_lock(port);
	if (!ip_active(port) || (ip_kotype(port) != IKOT_UPL)) {
		ip_unlock(port);
		return (upl_t)NULL;
	}
	upl = (upl_t) ip_get_kobject(port);
	ip_unlock(port);
	upl_lock(upl);
	upl->ref_count += 1;
	upl_unlock(upl);
	return upl;
}

mach_port_t
convert_upl_to_port(
	__unused upl_t          upl)
{
	return MACH_PORT_NULL;
}

__private_extern__ void
upl_no_senders(
	__unused ipc_port_t                             port,
	__unused mach_port_mscount_t    mscount)
{
	return;
}
