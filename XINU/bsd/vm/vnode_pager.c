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
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
  
                      
  
                                                
                                    
  
   

#include <mach/boolean.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/vnode_internal.h>
#include <sys/namei.h>
#include <sys/mount_internal.h>                                      
#include <sys/ubc_internal.h>
#include <sys/lock.h>
#include <sys/disk.h>                                

#include <mach/mach_types.h>
#include <mach/memory_object_types.h>
#include <mach/vm_map.h>
#include <mach/mach_vm.h>
#include <mach/upl.h>
#include <mach/sdt.h>

#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <kern/zalloc.h>
#include <libkern/libkern.h>

#include <vm/vnode_pager.h>
#include <vm/vm_pageout.h>

#include <kern/assert.h>
#include <sys/kdebug.h>
#include <nfs/nfs_conf.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>

#include <vm/vm_protos.h>

#include <vfs/vfs_disk_conditioner.h>

void
vnode_pager_throttle(void)
{
	struct uthread *ut;

	ut = get_bsdthread_info(current_thread());

	if (ut->uu_lowpri_window) {
		throttle_lowpri_io(1);
	}
}

boolean_t
vnode_pager_isSSD(vnode_t vp)
{
	return disk_conditioner_mount_is_ssd(vp->v_mount);
}

#if CONFIG_IOSCHED
void
vnode_pager_issue_reprioritize_io(struct vnode *devvp, uint64_t blkno, uint32_t len, int priority)
{
	u_int32_t blocksize = 0;
	dk_extent_t extent;
	dk_set_tier_t set_tier;
	int error = 0;

	error = VNOP_IOCTL(devvp, DKIOCGETBLOCKSIZE, (caddr_t)&blocksize, 0, vfs_context_kernel());
	if (error) {
		return;
	}

	memset(&extent, 0, sizeof(dk_extent_t));
	memset(&set_tier, 0, sizeof(dk_set_tier_t));

	extent.offset = blkno * (u_int64_t) blocksize;
	extent.length = len;

	set_tier.extents = &extent;
	set_tier.extentsCount = 1;
	set_tier.tier = (uint8_t)priority;

	error = VNOP_IOCTL(devvp, DKIOCSETTIER, (caddr_t)&set_tier, 0, vfs_context_kernel());
	return;
}
#endif

void
vnode_pager_was_dirtied(
	struct vnode            *vp,
	vm_object_offset_t      s_offset,
	vm_object_offset_t      e_offset)
{
	cluster_update_state(vp, s_offset, e_offset, TRUE);
}

uint32_t
vnode_pager_isinuse(struct vnode *vp)
{
	if (vp->v_usecount > vp->v_kusecount) {
		return 1;
	}
	return 0;
}

uint32_t
vnode_pager_return_throttle_io_limit(struct vnode *vp, uint32_t *limit)
{
	return cluster_throttle_io_limit(vp, limit);
}

vm_object_offset_t
vnode_pager_get_filesize(struct vnode *vp)
{
	return (vm_object_offset_t) ubc_getsize(vp);
}

extern int safe_getpath(struct vnode *dvp, char *leafname, char *path, int _len, int *truncated_path);

kern_return_t
vnode_pager_get_name(
	struct vnode    *vp,
	char            *pathname,
	vm_size_t       pathname_len,
	char            *filename,
	vm_size_t       filename_len,
	boolean_t       *truncated_path_p)
{
	*truncated_path_p = FALSE;
	if (pathname != NULL) {
		                       
		safe_getpath(vp, NULL,
		    pathname, (int) pathname_len,
		    truncated_path_p);
	}
	if ((pathname == NULL || *truncated_path_p) &&
	    filename != NULL) {
		                       
		const char *name;

		name = vnode_getname_printable(vp);
		strlcpy(filename, name, (size_t) filename_len);
		vnode_putname_printable(name);
	}
	return KERN_SUCCESS;
}

kern_return_t
vnode_pager_get_mtime(
	struct vnode    *vp,
	struct timespec *current_mtime,
	struct timespec *cs_mtime)
{
	vnode_mtime(vp, current_mtime, vfs_context_current());
	if (cs_mtime != NULL) {
		ubc_get_cs_mtime(vp, cs_mtime);
	}
	return KERN_SUCCESS;
}

kern_return_t
vnode_pager_get_cs_blobs(
	struct vnode    *vp,
	void            **blobs)
{
	*blobs = ubc_get_cs_blobs(vp);
	return KERN_SUCCESS;
}

  
              
                                                                                           
                                                                    
  
                                                                                                 
                                                                       
                                                                                                        
                              
   
u_int32_t
vnode_trim(
	struct vnode *vp,
	off_t offset,
	size_t length)
{
	daddr64_t io_blockno;                                                               
	size_t io_bytecount;                                                                   
	size_t trimmed = 0;
	off_t current_offset = offset;
	size_t remaining_length = length;
	int error = 0;
	u_int32_t blocksize = 0;
	struct vnode *devvp;
	dk_extent_t extent;
	dk_unmap_t unmap;


	                                     
	devvp = vp->v_mount->mnt_devvp;

	                                                 
	error  = VNOP_IOCTL(devvp, DKIOCGETBLOCKSIZE, (caddr_t)&blocksize, 0, vfs_context_kernel());
	if (error) {
		goto trim_exit;
	}

	  
                                                                            
                                                                                  
                                                
    
	while (trimmed < length) {
		  
                                                                                    
                                                                                          
                                                                                          
                                                             
     
		error = VNOP_BLOCKMAP(vp, current_offset, remaining_length,
		    &io_blockno, &io_bytecount, NULL, VNODE_READ | VNODE_BLOCKMAP_NO_TRACK, NULL);

		if (error) {
			goto trim_exit;
		}
		  
                                                                         
                                                                             
     
		memset(&extent, 0, sizeof(dk_extent_t));
		memset(&unmap, 0, sizeof(dk_unmap_t));
		extent.offset = (uint64_t) io_blockno * (u_int64_t) blocksize;
		extent.length = io_bytecount;
		unmap.extents = &extent;
		unmap.extentsCount = 1;
		error = VNOP_IOCTL(devvp, DKIOCUNMAP, (caddr_t)&unmap, 0, vfs_context_kernel());

		if (error) {
			goto trim_exit;
		}
		remaining_length = remaining_length - io_bytecount;
		trimmed = trimmed + io_bytecount;
		current_offset = current_offset + io_bytecount;
	}
trim_exit:

	return error;
}

pager_return_t
vnode_pageout(struct vnode *vp,
    upl_t                   upl,
    upl_offset_t            upl_offset,
    vm_object_offset_t      f_offset,
    upl_size_t              size,
    int                     flags,
    int                     *errorp)
{
	int             result = PAGER_SUCCESS;
	int             error = 0;
	int             error_ret = 0;
	daddr64_t blkno;
	int isize;
	int pg_index;
	int base_index;
	upl_offset_t offset;
	upl_page_info_t *pl;
	vfs_context_t ctx = vfs_context_current();                         

	isize = (int)size;

	  
                                                               
                                                             
                                                                 
                                      
   
                                                                
                                                               
                                                               
                                                                 
                                                                 
                                                                 
                     
    
	error = vnode_getalways_from_pager(vp);
	if (error != 0) {
		                       
		panic("vnode_getalways returned %d for vp %p", error, vp);
	}

	if (isize <= 0) {
		result    = PAGER_ERROR;
		error_ret = EINVAL;
		goto out;
	}

	if (UBCINFOEXISTS(vp) == 0) {
		result    = PAGER_ERROR;
		error_ret = EINVAL;

		if (upl && !(flags & UPL_NOCOMMIT)) {
			ubc_upl_abort_range(upl, upl_offset, size, UPL_ABORT_FREE_ON_EMPTY);
		}
		goto out;
	}
	if (!(flags & UPL_VNODE_PAGER)) {
		  
                                              
                                              
                                               
     
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (MACHDBG_CODE(DBG_MACH_VM, 1)) | DBG_FUNC_START,
		    size, 1, 0, 0, 0);

		if ((error_ret = VNOP_PAGEOUT(vp, upl, upl_offset, (off_t)f_offset,
		    (size_t)size, flags, ctx))) {
			result = PAGER_ERROR;
		}

		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (MACHDBG_CODE(DBG_MACH_VM, 1)) | DBG_FUNC_END,
		    size, 1, 0, 0, 0);

		goto out;
	}
	if (upl == NULL) {
		int                     request_flags;

		if (vp->v_mount->mnt_vtable->vfc_vfsflags & VFC_VFSVNOP_PAGEOUTV2) {
			  
                                                                    
                                                                               
                                                                           
                                                                                 
                                                                                 
      
			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    (MACHDBG_CODE(DBG_MACH_VM, 1)) | DBG_FUNC_START,
			    size, (int)f_offset, 0, 0, 0);

			if ((error_ret = VNOP_PAGEOUT(vp, NULL, upl_offset, (off_t)f_offset,
			    size, flags, ctx))) {
				result = PAGER_ERROR;
			}
			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    (MACHDBG_CODE(DBG_MACH_VM, 1)) | DBG_FUNC_END,
			    size, 0, 0, 0, 0);

			goto out;
		}
		if (flags & UPL_MSYNC) {
			request_flags = UPL_UBC_MSYNC | UPL_RET_ONLY_DIRTY;
		} else {
			request_flags = UPL_UBC_PAGEOUT | UPL_RET_ONLY_DIRTY;
		}

		if (ubc_create_upl_kernel(vp, f_offset, size, &upl, &pl, request_flags, VM_KERN_MEMORY_FILE) != KERN_SUCCESS) {
			result    = PAGER_ERROR;
			error_ret = EINVAL;
			goto out;
		}
		upl_offset = 0;
	} else {
		pl = ubc_upl_pageinfo(upl);
	}

	  
                                                  
                                               
                                                 
                       
    
	base_index = upl_offset / PAGE_SIZE;

	for (pg_index = (upl_offset + isize) / PAGE_SIZE; pg_index > base_index;) {
		if (upl_page_present(pl, --pg_index)) {
			break;
		}
		if (pg_index == base_index) {
			  
                                        
                                   
      
			if (!(flags & UPL_NOCOMMIT)) {
				ubc_upl_abort_range(upl, upl_offset, isize, UPL_ABORT_FREE_ON_EMPTY);
			}

			goto out;
		}
	}
	isize = ((pg_index + 1) - base_index) * PAGE_SIZE;

	  
                                                 
                                              
                                                 
                                                 
                              
    

	if (ubc_getsize(vp) == 0) {
		  
                                                   
                                                     
                                                    
                  
     
		for (offset = upl_offset; isize; isize -= PAGE_SIZE, offset += PAGE_SIZE) {
#if CONFIG_NFS_CLIENT
			if (vp->v_tag == VT_NFS) {
				                                          
				error = nfs_buf_page_inval(vp, (off_t)f_offset);
			} else
#endif                        
			{
				blkno = ubc_offtoblk(vp, (off_t)f_offset);
				error = buf_invalblkno(vp, blkno, 0);
			}
			if (error) {
				if (!(flags & UPL_NOCOMMIT)) {
					ubc_upl_abort_range(upl, offset, PAGE_SIZE, UPL_ABORT_FREE_ON_EMPTY);
				}
				if (error_ret == 0) {
					error_ret = error;
				}
				result = PAGER_ERROR;
			} else if (!(flags & UPL_NOCOMMIT)) {
				ubc_upl_commit_range(upl, offset, PAGE_SIZE, UPL_COMMIT_FREE_ON_EMPTY);
			}
			f_offset += PAGE_SIZE;
		}
		goto out;
	}

	offset = upl_offset;
	pg_index = base_index;

	while (isize) {
		int  xsize;
		int  num_of_pages;

		if (!upl_page_present(pl, pg_index)) {
			  
                                                   
                                        
                         
      
			f_offset += PAGE_SIZE;
			offset   += PAGE_SIZE;
			isize    -= PAGE_SIZE;
			pg_index++;

			continue;
		}
		if (!upl_dirty_page(pl, pg_index)) {
			  
                                                     
                                                     
                                                      
                                                 
                                    
                                                         
                                                                 
                                             
      
#if CONFIG_NFS_CLIENT
			if (vp->v_tag == VT_NFS) {
				                                          
				error = nfs_buf_page_inval(vp, (off_t)f_offset);
			} else
#endif                        
			{
				blkno = ubc_offtoblk(vp, (off_t)f_offset);
				error = buf_invalblkno(vp, blkno, 0);
			}
			if (error) {
				if (!(flags & UPL_NOCOMMIT)) {
					ubc_upl_abort_range(upl, offset, PAGE_SIZE, UPL_ABORT_FREE_ON_EMPTY);
				}
				if (error_ret == 0) {
					error_ret = error;
				}
				result = PAGER_ERROR;
			} else if (!(flags & UPL_NOCOMMIT)) {
				ubc_upl_commit_range(upl, offset, PAGE_SIZE, UPL_COMMIT_FREE_ON_EMPTY);
			}
			f_offset += PAGE_SIZE;
			offset   += PAGE_SIZE;
			isize    -= PAGE_SIZE;
			pg_index++;

			continue;
		}
		num_of_pages = 1;
		xsize = isize - PAGE_SIZE;

		while (xsize) {
			if (!upl_dirty_page(pl, pg_index + num_of_pages)) {
				break;
			}
			num_of_pages++;
			xsize -= PAGE_SIZE;
		}
		xsize = num_of_pages * PAGE_SIZE;

		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (MACHDBG_CODE(DBG_MACH_VM, 1)) | DBG_FUNC_START,
		    xsize, (int)f_offset, 0, 0, 0);

		if ((error = VNOP_PAGEOUT(vp, upl, offset, (off_t)f_offset,
		    xsize, flags, ctx))) {
			if (error_ret == 0) {
				error_ret = error;
			}
			result = PAGER_ERROR;
		}
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (MACHDBG_CODE(DBG_MACH_VM, 1)) | DBG_FUNC_END,
		    xsize, 0, 0, 0, 0);

		f_offset += xsize;
		offset   += xsize;
		isize    -= xsize;
		pg_index += num_of_pages;
	}
out:
	vnode_put_from_pager(vp);

	if (errorp) {
		*errorp = error_ret;
	}

	return result;
}


pager_return_t
vnode_pagein(
	struct vnode            *vp,
	upl_t                   upl,
	upl_offset_t            upl_offset,
	vm_object_offset_t      f_offset,
	upl_size_t              size,
	int                     flags,
	int                     *errorp)
{
	upl_page_info_t *pl;
	int             result = PAGER_SUCCESS;
	int             error = 0;
	int             pages_in_upl;
	int             start_pg;
	int             last_pg;
	int             first_pg;
	int             xsize;
	int             must_commit = 1;
	int             ignore_valid_page_check = 0;

	if (flags & UPL_NOCOMMIT) {
		must_commit = 0;
	}

	if (flags & UPL_IGNORE_VALID_PAGE_CHECK) {
		ignore_valid_page_check = 1;
	}

	  
                                                               
                                                             
                                                                 
                                      
   
                                                            
                                                                      
                                                                     
                                                                 
                                                                  
                                             
    
	error = vnode_getalways_from_pager(vp);
	if (error != 0) {
		                       
		panic("vnode_getalways returned %d for vp %p", error, vp);
	}

	if (UBCINFOEXISTS(vp) == 0) {
		result = PAGER_ERROR;
		error  = PAGER_ERROR;

		if (upl && must_commit) {
			ubc_upl_abort_range(upl, upl_offset, size, UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_ERROR);
		}

		goto out;
	}
	if (upl == (upl_t)NULL) {
		flags &= ~UPL_NOCOMMIT;

		if (size > MAX_UPL_SIZE_BYTES) {
			result = PAGER_ERROR;
			error  = PAGER_ERROR;
			goto out;
		}
		if (vp->v_mount->mnt_vtable->vfc_vfsflags & VFC_VFSVNOP_PAGEINV2) {
			  
                                                                   
                                                                              
                                                                           
                                                                                 
                                                                                 
                                                                                        
                                                                                      
                                                                                   
                                                                                       
                          
      
			if ((error = VNOP_PAGEIN(vp, NULL, upl_offset, (off_t)f_offset,
			    size, flags, vfs_context_current()))) {
				set_thread_pagein_error(current_thread(), error);
				result = PAGER_ERROR;
				error  = PAGER_ERROR;
			}
			goto out;
		}
		ubc_create_upl_kernel(vp, f_offset, size, &upl, &pl, UPL_UBC_PAGEIN | UPL_RET_ONLY_ABSENT, VM_KERN_MEMORY_FILE);

		if (upl == (upl_t)NULL) {
			result =  PAGER_ABSENT;
			error = PAGER_ABSENT;
			goto out;
		}
		ubc_upl_range_needed(upl, upl_offset / PAGE_SIZE, 1);

		upl_offset = 0;
		first_pg = 0;

		  
                                              
                                              
                                                
     
		must_commit = 1;
	} else {
		pl = ubc_upl_pageinfo(upl);
		first_pg = upl_offset / PAGE_SIZE;
	}
	pages_in_upl = size / PAGE_SIZE;
	DTRACE_VM2(pgpgin, int, pages_in_upl, (uint64_t *), NULL);

	  
                                                                 
                                                             
       
    
	for (last_pg = pages_in_upl - 1; last_pg >= first_pg; last_pg--) {
		if (upl_page_present(pl, last_pg)) {
			break;
		}
		if (last_pg == first_pg) {
			  
                                     
      
			if (must_commit) {
				ubc_upl_abort_range(upl, upl_offset, size, UPL_ABORT_FREE_ON_EMPTY);
			}
			goto out;
		}
	}
	pages_in_upl = last_pg + 1;
	last_pg = first_pg;

	while (last_pg < pages_in_upl) {
		  
                               
     
		for (; last_pg < pages_in_upl; last_pg++) {
			if (upl_page_present(pl, last_pg)) {
				break;
			}
		}

		if (ignore_valid_page_check == 1) {
			start_pg = last_pg;
		} else {
			  
                                                                     
      
			for (start_pg = last_pg; last_pg < pages_in_upl; last_pg++) {
				if (!upl_valid_page(pl, last_pg)) {
					break;
				}
			}
		}

		if (last_pg > start_pg) {
			  
                                        
                                        
                                            
                     
      
			xsize = (last_pg - start_pg) * PAGE_SIZE;

			if (must_commit) {
				ubc_upl_abort_range(upl, start_pg * PAGE_SIZE, xsize, UPL_ABORT_FREE_ON_EMPTY);
			}
		}
		if (last_pg == pages_in_upl) {
			  
                                               
                                           
                               
      
			break;
		}

		if (!upl_page_present(pl, last_pg)) {
			  
                                     
                                     
                                                 
      
			last_pg++;
			continue;
		}
		  
                                                         
                                                                    
                                                              
                 
     
		for (start_pg = last_pg; last_pg < pages_in_upl; last_pg++) {
			if ((!ignore_valid_page_check && upl_valid_page(pl, last_pg)) || !upl_page_present(pl, last_pg)) {
				break;
			}
		}
		if (last_pg > start_pg) {
			int xoff;
			xsize = (last_pg - start_pg) * PAGE_SIZE;
			xoff  = start_pg * PAGE_SIZE;

			if ((error = VNOP_PAGEIN(vp, upl, (upl_offset_t) xoff,
			    (off_t)f_offset + xoff,
			    xsize, flags, vfs_context_current()))) {
				  
                                                                             
      
                                                                                   
                                                                 
      
                                                                                             
      
                                                                         
       
				if (must_commit) {
					if (error == EAGAIN) {
						ubc_upl_abort_range(upl, (upl_offset_t) xoff, xsize, UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_RESTART);
					}
					if (error == EPERM) {
						ubc_upl_abort_range(upl, (upl_offset_t) xoff, xsize, UPL_ABORT_FREE_ON_EMPTY | UPL_ABORT_ERROR);
					}
				}
				set_thread_pagein_error(current_thread(), error);
				result = PAGER_ERROR;
				error  = PAGER_ERROR;
			}
		}
	}
out:
	vnode_put_from_pager(vp);

	if (errorp) {
		*errorp = result;
	}

	return error;
}

void *
upl_get_internal_page_list(upl_t upl)
{
	return UPL_GET_INTERNAL_PAGE_LIST(upl);
}
