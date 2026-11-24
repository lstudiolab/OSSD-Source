/*
 * Copyright (c) 2007-2020 Apple Inc. All Rights Reserved.
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
/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: vm_mmap.c 1.6 91/10/21$
 *
 *	@(#)vm_mmap.c	8.10 (Berkeley) 2/19/95
 */
  
                                                                      
                                                                          
                                                                        
               
   

  
                                     
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/resourcevar.h>
#include <sys/vnode_internal.h>
#include <sys/acct.h>
#include <sys/wait.h>
#include <sys/file_internal.h>
#include <sys/vadvise.h>
#include <sys/trace.h>
#include <sys/mman.h>
#include <sys/conf.h>
#include <sys/stat.h>
#include <sys/ubc.h>
#include <sys/ubc_internal.h>
#include <sys/sysproto.h>

#include <sys/syscall.h>
#include <sys/kdebug.h>
#include <sys/bsdtask_info.h>

#include <security/audit/audit.h>
#include <bsm/audit_kevents.h>

#include <mach/mach_types.h>
#include <mach/mach_traps.h>
#include <mach/vm_sync.h>
#include <mach/vm_behavior.h>
#include <mach/vm_inherit.h>
#include <mach/vm_statistics.h>
#include <mach/mach_vm.h>
#include <mach/vm_map.h>
#include <mach/host_priv.h>
#include <mach/sdt.h>
#include <mach-o/loader.h>

#include <machine/machine_routines.h>

#include <kern/cpu_number.h>
#include <kern/host.h>
#include <kern/task.h>
#include <kern/page_decrypt.h>

#include <IOKit/IOReturn.h>

#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_pager.h>
#include <vm/vm_protos.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif
#include <os/overflow.h>

  
                                                                                 
                                                                                
                                                                                
   
static uint32_t
proc_2020_fall_os_sdk(void)
{
	switch (current_proc()->p_platform) {
	case PLATFORM_MACOS:
		return 0x000a1000;                             
	case PLATFORM_IOS:
	case PLATFORM_IOSSIMULATOR:
	case PLATFORM_MACCATALYST:
		return 0x000e0000;                         
	case PLATFORM_BRIDGEOS:
		return 0x00050000;                             
	case PLATFORM_TVOS:
	case PLATFORM_TVOSSIMULATOR:
		return 0x000e0000;                          
	case PLATFORM_WATCHOS:
	case PLATFORM_WATCHOSSIMULATOR:
		return 0x00070000;                            
	default:
		return 0;
	}
}

  
                                                                             
                                                                         
                                               
   
int
mmap(proc_t p, struct mmap_args *uap, user_addr_t *retval)
{
	  
                                                  
    
	struct fileproc *fp;
	struct                  vnode *vp;
	int                     flags;
	int                     prot;
	int                     err = 0;
	vm_map_t                user_map;
	kern_return_t           result;
	vm_map_offset_t         user_addr;
	vm_map_offset_t         sum;
	vm_map_size_t           user_size;
	vm_object_offset_t      pageoff;
	vm_object_offset_t      file_pos;
	int                     alloc_flags = 0;
	vm_tag_t                tag = VM_KERN_MEMORY_NONE;
	vm_map_kernel_flags_t   vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
	boolean_t               docow;
	vm_prot_t               maxprot;
	void                    *handle;
	memory_object_t         pager = MEMORY_OBJECT_NULL;
	memory_object_control_t  control;
	int                     mapanon = 0;
	int                     fpref = 0;
	int error = 0;
	int fd = uap->fd;
	int num_retries = 0;

	  
                                                                                
                                                                              
                                                                                
                                
    

	user_map = current_map();
	user_addr = (vm_map_offset_t)uap->addr;
	user_size = (vm_map_size_t) uap->len;

	AUDIT_ARG(addr, user_addr);
	AUDIT_ARG(len, user_size);
	AUDIT_ARG(fd, uap->fd);

	if (vm_map_range_overflows(user_addr, user_size)) {
		return EINVAL;
	}
	prot = (uap->prot & VM_PROT_ALL);
#if 3777787
	  
                                                                 
                                                                   
                                                                   
                                                                   
    
	if (prot & (VM_PROT_EXECUTE | VM_PROT_WRITE)) {
		prot |= VM_PROT_READ;
	}
#endif                     

	flags = uap->flags;
	vp = NULLVP;

	  
                                                          
                                                                 
                            
    
	if (flags & ~(MAP_SHARED |
	    MAP_PRIVATE |
	    MAP_COPY |
	    MAP_FIXED |
	    MAP_RENAME |
	    MAP_NORESERVE |
	    MAP_RESERVED0080 |                                                                            
	    MAP_NOEXTEND |
	    MAP_HASSEMAPHORE |
	    MAP_NOCACHE |
	    MAP_JIT |
	    MAP_FILE |
	    MAP_ANON |
	    MAP_RESILIENT_CODESIGN |
	    MAP_RESILIENT_MEDIA |
#if XNU_TARGET_OS_OSX
	    MAP_32BIT |
#endif
	    MAP_TRANSLATED_ALLOW_EXECUTE |
	    MAP_UNIX03)) {
		if (proc_sdk(current_proc()) >= proc_2020_fall_os_sdk()) {
			return EINVAL;
		}
	}


	  
                                                              
                                                                     
                        
    
	file_pos = (vm_object_offset_t)uap->pos;


	                                                   
	if (os_add3_overflow(file_pos, user_size, vm_map_page_size(user_map) - 1, &sum)) {
		return EINVAL;
	}

	if (flags & MAP_UNIX03) {
		vm_map_offset_t offset_alignment_mask;

		  
                               
     

		if (vm_map_is_exotic(current_map())) {
			offset_alignment_mask = 0xFFF;
		} else {
			offset_alignment_mask = vm_map_page_mask(current_map());
		}
		if (file_pos & offset_alignment_mask) {
			                                        
			return EINVAL;
		}
		if (!(flags & (MAP_PRIVATE | MAP_SHARED))) {
			                                           
			return EINVAL;
		}
		if (user_size == 0) {
			                                    
			return EINVAL;
		}
	}

	  
                                               
                                       
    
	pageoff = (file_pos & vm_map_page_mask(user_map));
	file_pos -= (vm_object_offset_t)pageoff;


	                                              
	user_size += pageoff;                   
	user_size = vm_map_round_page(user_size,
	    vm_map_page_mask(user_map));                                       


	if (flags & MAP_JIT) {
		if ((flags & MAP_FIXED) ||
		    (flags & MAP_SHARED) ||
		    !(flags & MAP_ANON) ||
		    (flags & MAP_RESILIENT_CODESIGN) ||
		    (flags & MAP_RESILIENT_MEDIA)) {
			return EINVAL;
		}
	}

	if ((flags & MAP_RESILIENT_CODESIGN) ||
	    (flags & MAP_RESILIENT_MEDIA)) {
		if ((flags & MAP_ANON) ||
		    (flags & MAP_JIT)) {
			return EINVAL;
		}
	}
	if (flags & MAP_RESILIENT_CODESIGN) {
		int reject_prot = ((flags & MAP_PRIVATE) ? VM_PROT_EXECUTE : (VM_PROT_WRITE | VM_PROT_EXECUTE));
		if (prot & reject_prot) {
			  
                                                         
                            
      
			return EPERM;
		}
	}
	if (flags & MAP_SHARED) {
		  
                                                             
                                                          
                                      
                                                                
                                                               
                                        
                                                            
                                      
     
		flags &= ~MAP_RESILIENT_MEDIA;
	}
	if (flags & MAP_RESILIENT_MEDIA) {
		if ((flags & MAP_ANON) ||
		    (flags & MAP_SHARED)) {
			return EINVAL;
		}
	}

	  
                                                                    
                                                            
    
	if (flags & MAP_FIXED) {
		  
                                                       
                                                     
                                                   
     
		user_addr -= pageoff;
		if (user_addr & vm_map_page_mask(user_map)) {
			return EINVAL;
		}
	}
#ifdef notyet
	                                                              
	  
                                                           
                                                    
                                                        
   
                                                                
             
    
	else if (addr < vm_map_round_page(p->p_vmspace->vm_daddr + MAXDSIZ,
	    vm_map_page_mask(user_map))) {
		addr = vm_map_round_page(p->p_vmspace->vm_daddr + MAXDSIZ,
		    vm_map_page_mask(user_map));
	}

#endif

	alloc_flags = 0;

	if (flags & MAP_ANON) {
		maxprot = VM_PROT_ALL;
#if CONFIG_MACF
		  
                       
     
		error = mac_proc_check_map_anon(p, user_addr, user_size, prot, flags, &maxprot);
		if (error) {
			return EINVAL;
		}
#endif          

		  
                                                                   
                               
     
		if (fd != -1) {
			  
                                                       
                                       
      
			alloc_flags = fd & (VM_FLAGS_ALIAS_MASK |
			    VM_FLAGS_SUPERPAGE_MASK |
			    VM_FLAGS_PURGABLE |
			    VM_FLAGS_4GB_CHUNK);
			if (alloc_flags != fd) {
				                                         
				return EINVAL;
			}
			VM_GET_FLAGS_ALIAS(alloc_flags, tag);
			alloc_flags &= ~VM_FLAGS_ALIAS_MASK;
		}

		handle = NULL;
		file_pos = 0;
		pageoff = 0;
		mapanon = 1;
	} else {
		struct vnode_attr va;
		vfs_context_t ctx = vfs_context_current();

		if (flags & MAP_JIT) {
			return EINVAL;
		}

		  
                                                               
                                    
     
		err = fp_lookup(p, fd, &fp, 0);
		if (err) {
			return err;
		}
		fpref = 1;
		switch (FILEGLOB_DTYPE(fp->fp_glob)) {
		case DTYPE_PSXSHM:
			uap->addr = (user_addr_t)user_addr;
			uap->len = (user_size_t)user_size;
			uap->prot = prot;
			uap->flags = flags;
			uap->pos = file_pos;
			error = pshm_mmap(p, uap, retval, fp, (off_t)pageoff);
			goto bad;
		case DTYPE_VNODE:
			break;
		default:
			error = EINVAL;
			goto bad;
		}
		vp = (struct vnode *)fp->fp_glob->fg_data;
		error = vnode_getwithref(vp);
		if (error != 0) {
			goto bad;
		}

		if (vp->v_type != VREG && vp->v_type != VCHR) {
			(void)vnode_put(vp);
			error = EINVAL;
			goto bad;
		}

		AUDIT_ARG(vnpath, vp, ARG_VNODE1);

		  
                                                             
     
		if ((vnode_vfsvisflags(vp) & MNT_NOATIME) == 0) {
			VATTR_INIT(&va);
			nanotime(&va.va_access_time);
			VATTR_SET_ACTIVE(&va, va_access_time);
			vnode_setattr(vp, &va, ctx);
		}

		  
                                                                
            
     
		if (vp->v_type == VCHR || vp->v_type == VSTR) {
			(void)vnode_put(vp);
			error = ENODEV;
			goto bad;
		} else {
			  
                                                 
                                                
                                                     
                                                         
                                                  
                                                      
                         
      
			maxprot = VM_PROT_EXECUTE;                                                   
			if (fp->fp_glob->fg_flag & FREAD) {
				maxprot |= VM_PROT_READ;
			} else if (prot & PROT_READ) {
				(void)vnode_put(vp);
				error = EACCES;
				goto bad;
			}
			  
                                                     
                                                         
                                                      
                                                     
                       
      

			if ((flags & MAP_SHARED) != 0) {
				if ((fp->fp_glob->fg_flag & FWRITE) != 0 &&
				      
                                            
                                                
           
				    !vnode_isswap(vp)) {
					  
                              
       
                                                                 
                                                     
        
					error = vnode_authorize(vp, NULL, KAUTH_VNODE_CHECKIMMUTABLE, ctx);

					                                                          
					if ((prot & PROT_WRITE) && (error != 0)) {
						vnode_put(vp);
						goto bad;
					}

					                           
					if (error == 0) {
						maxprot |= VM_PROT_WRITE;
					}
				} else if ((prot & PROT_WRITE) != 0) {
					(void)vnode_put(vp);
					error = EACCES;
					goto bad;
				}
			} else {
				maxprot |= VM_PROT_WRITE;
			}

			handle = (void *)vp;
#if CONFIG_MACF
			error = mac_file_check_mmap(vfs_context_ucred(ctx),
			    fp->fp_glob, prot, flags, file_pos + pageoff,
			    &maxprot);
			if (error) {
				(void)vnode_put(vp);
				goto bad;
			}
#endif          
			  
                                                  
                                           
     
                                                        
                                                      
                                                  
      
			error = VNOP_MMAP_CHECK(vp, (flags & MAP_PRIVATE) ?
			    (prot & ~PROT_WRITE) : prot, ctx);
			if (error) {
				(void)vnode_put(vp);
				goto bad;
			}
		}

		  
                                                    
     
		vmk_flags.vmkf_no_copy_on_read = 1;
	}

	if (user_size == 0) {
		if (!mapanon) {
			(void)vnode_put(vp);
		}
		error = 0;
		goto bad;
	}

	  
                                                        
                                 
    
	user_size = vm_map_round_page(user_size,
	    vm_map_page_mask(user_map));

	if (file_pos & vm_map_page_mask(user_map)) {
		if (!mapanon) {
			(void)vnode_put(vp);
		}
		error = EINVAL;
		goto bad;
	}

	if ((flags & MAP_FIXED) == 0) {
		alloc_flags |= VM_FLAGS_ANYWHERE;
		user_addr = vm_map_round_page(user_addr,
		    vm_map_page_mask(user_map));
	} else {
		if (user_addr != vm_map_trunc_page(user_addr,
		    vm_map_page_mask(user_map))) {
			if (!mapanon) {
				(void)vnode_put(vp);
			}
			error = EINVAL;
			goto bad;
		}
		  
                                                              
                                                       
                                                            
                                                             
                                                               
                                                            
                                                             
                                                              
                         
     
		alloc_flags |= VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE;
	}

	if (flags & MAP_NOCACHE) {
		alloc_flags |= VM_FLAGS_NO_CACHE;
	}

	if (flags & MAP_JIT) {
		vmk_flags.vmkf_map_jit = TRUE;
	}


	if (flags & MAP_RESILIENT_CODESIGN) {
		alloc_flags |= VM_FLAGS_RESILIENT_CODESIGN;
	}
	if (flags & MAP_RESILIENT_MEDIA) {
		alloc_flags |= VM_FLAGS_RESILIENT_MEDIA;
	}

#if XNU_TARGET_OS_OSX
	                                            
	if (flags & MAP_32BIT) {
		vmk_flags.vmkf_32bit_map_va = TRUE;
	}
#endif

	  
                           
    
	if (handle == NULL) {
		control = NULL;
#ifdef notyet
            
#if defined(VM_PROT_READ_IS_EXEC)
		if (prot & VM_PROT_READ) {
			prot |= VM_PROT_EXECUTE;
		}
		if (maxprot & VM_PROT_READ) {
			maxprot |= VM_PROT_EXECUTE;
		}
#endif
#endif

#if 3777787
		if (prot & (VM_PROT_EXECUTE | VM_PROT_WRITE)) {
			prot |= VM_PROT_READ;
		}
		if (maxprot & (VM_PROT_EXECUTE | VM_PROT_WRITE)) {
			maxprot |= VM_PROT_READ;
		}
#endif                     
map_anon_retry:

		result = vm_map_enter_mem_object(user_map,
		    &user_addr, user_size,
		    0, alloc_flags, vmk_flags,
		    tag,
		    IPC_PORT_NULL, 0, FALSE,
		    prot, maxprot,
		    (flags & MAP_SHARED) ?
		    VM_INHERIT_SHARE :
		    VM_INHERIT_DEFAULT);

		                                                            
                                                
                                                     
                                                             
     
		if ((result == KERN_NO_SPACE) && ((flags & MAP_FIXED) == 0) && user_addr && (num_retries++ == 0)) {
			user_addr = vm_map_page_size(user_map);
			goto map_anon_retry;
		}
	} else {
		if (vnode_isswap(vp)) {
			  
                                         
                                       
      
			control = NULL;
			pager = swapfile_pager_setup(vp);
			if (pager != MEMORY_OBJECT_NULL) {
				control = swapfile_pager_control(pager);
			}
		} else {
			control = ubc_getobject(vp, UBC_FLAGS_NONE);
		}

		if (control == NULL) {
			(void)vnode_put(vp);
			error = ENOMEM;
			goto bad;
		}

		  
                      
                                                      
                                                           
                                            
     

		ubc_setthreadcred(vp, p, current_thread());
		docow = FALSE;
		if ((flags & (MAP_ANON | MAP_SHARED)) == 0) {
			docow = TRUE;
		}

#ifdef notyet
            
#if defined(VM_PROT_READ_IS_EXEC)
		if (prot & VM_PROT_READ) {
			prot |= VM_PROT_EXECUTE;
		}
		if (maxprot & VM_PROT_READ) {
			maxprot |= VM_PROT_EXECUTE;
		}
#endif
#endif             

#if 3777787
		if (prot & (VM_PROT_EXECUTE | VM_PROT_WRITE)) {
			prot |= VM_PROT_READ;
		}
		if (maxprot & (VM_PROT_EXECUTE | VM_PROT_WRITE)) {
			maxprot |= VM_PROT_READ;
		}
#endif                     

map_file_retry:
		if (flags & MAP_RESILIENT_CODESIGN) {
			int reject_prot = ((flags & MAP_PRIVATE) ? VM_PROT_EXECUTE : (VM_PROT_WRITE | VM_PROT_EXECUTE));
			if (prot & reject_prot) {
				  
                                              
                                            
                                                   
      
                                                    
                                                  
             
       
				assert(!mapanon);
				vnode_put(vp);
				error = EPERM;
				goto bad;
			}
			                                     
			maxprot &= prot;
		}

		vm_object_offset_t end_pos = 0;
		if (os_add_overflow(user_size, file_pos, &end_pos)) {
			vnode_put(vp);
			error = EINVAL;
			goto bad;
		}

		result = vm_map_enter_mem_object_control(user_map,
		    &user_addr, user_size,
		    0, alloc_flags, vmk_flags,
		    tag,
		    control, file_pos,
		    docow, prot, maxprot,
		    (flags & MAP_SHARED) ?
		    VM_INHERIT_SHARE :
		    VM_INHERIT_DEFAULT);

		                                                              
                                                
                                                     
                                                             
     
		if ((result == KERN_NO_SPACE) && ((flags & MAP_FIXED) == 0) && user_addr && (num_retries++ == 0)) {
			user_addr = vm_map_page_size(user_map);
			goto map_file_retry;
		}
	}

	if (!mapanon) {
		(void)vnode_put(vp);
	}

	switch (result) {
	case KERN_SUCCESS:
		*retval = user_addr + pageoff;
		error = 0;
		break;
	case KERN_INVALID_ADDRESS:
	case KERN_NO_SPACE:
		error =  ENOMEM;
		break;
	case KERN_PROTECTION_FAILURE:
		error =  EACCES;
		break;
	default:
		error =  EINVAL;
		break;
	}
bad:
	if (pager != MEMORY_OBJECT_NULL) {
		  
                                        
                                                
                        
     
		memory_object_deallocate(pager);
	}
	if (fpref) {
		fp_drop(p, fd, fp, 0);
	}

	KERNEL_DEBUG_CONSTANT((BSDDBG_CODE(DBG_BSD_SC_EXTENDED_INFO, SYS_mmap) | DBG_FUNC_NONE), fd, (uint32_t)(*retval), (uint32_t)user_size, error, 0);
#if XNU_TARGET_OS_OSX
	KERNEL_DEBUG_CONSTANT((BSDDBG_CODE(DBG_BSD_SC_EXTENDED_INFO2, SYS_mmap) | DBG_FUNC_NONE), (uint32_t)(*retval >> 32), (uint32_t)(user_size >> 32),
	    (uint32_t)(file_pos >> 32), (uint32_t)file_pos, 0);
#endif                        
	return error;
}

int
msync(__unused proc_t p, struct msync_args *uap, int32_t *retval)
{
	__pthread_testcancel(1);
	return msync_nocancel(p, (struct msync_nocancel_args *)uap, retval);
}

int
msync_nocancel(__unused proc_t p, struct msync_nocancel_args *uap, __unused int32_t *retval)
{
	mach_vm_offset_t addr;
	mach_vm_size_t size;
	int flags;
	vm_map_t user_map;
	int rv;
	vm_sync_t sync_flags = 0;

	user_map = current_map();
	addr = (mach_vm_offset_t) uap->addr;
	size = (mach_vm_size_t) uap->len;
#if XNU_TARGET_OS_OSX
	KERNEL_DEBUG_CONSTANT((BSDDBG_CODE(DBG_BSD_SC_EXTENDED_INFO, SYS_msync) | DBG_FUNC_NONE), (uint32_t)(addr >> 32), (uint32_t)(size >> 32), 0, 0, 0);
#endif                        
	if (mach_vm_range_overflows(addr, size)) {
		return EINVAL;
	}
	if (addr & vm_map_page_mask(user_map)) {
		                                                                
		return EINVAL;
	}
	if (size == 0) {
		  
                                                        
                                                                  
                                                              
                                                               
              
     
		return EINVAL;                            
	}

	flags = uap->flags;
	                                  
	if ((flags & (MS_SYNC | MS_ASYNC)) == (MS_SYNC | MS_ASYNC)) {
		return EINVAL;
	}

	if (flags & MS_KILLPAGES) {
		sync_flags |= VM_SYNC_KILLPAGES;
	}
	if (flags & MS_DEACTIVATE) {
		sync_flags |= VM_SYNC_DEACTIVATE;
	}
	if (flags & MS_INVALIDATE) {
		sync_flags |= VM_SYNC_INVALIDATE;
	}

	if (!(flags & (MS_KILLPAGES | MS_DEACTIVATE))) {
		if (flags & MS_ASYNC) {
			sync_flags |= VM_SYNC_ASYNCHRONOUS;
		} else {
			sync_flags |= VM_SYNC_SYNCHRONOUS;
		}
	}

	sync_flags |= VM_SYNC_CONTIGUOUS;                              

	rv = mach_vm_msync(user_map, addr, size, sync_flags);

	switch (rv) {
	case KERN_SUCCESS:
		break;
	case KERN_INVALID_ADDRESS:                                        
		return ENOMEM;
	case KERN_FAILURE:
		return EIO;
	default:
		return EINVAL;
	}
	return 0;
}


int
munmap(__unused proc_t p, struct munmap_args *uap, __unused int32_t *retval)
{
	mach_vm_offset_t        user_addr;
	mach_vm_size_t          user_size;
	kern_return_t           result;
	vm_map_t                user_map;

	user_map = current_map();
	user_addr = (mach_vm_offset_t) uap->addr;
	user_size = (mach_vm_size_t) uap->len;

	AUDIT_ARG(addr, user_addr);
	AUDIT_ARG(len, user_size);

	if (user_addr & vm_map_page_mask(user_map)) {
		                                                                
		return EINVAL;
	}

	if (mach_vm_range_overflows(user_addr, user_size)) {
		return EINVAL;
	}

	if (user_size == 0) {
		                                         
		return EINVAL;
	}

	result = mach_vm_deallocate(user_map, user_addr, user_size);
	if (result != KERN_SUCCESS) {
		return EINVAL;
	}
	return 0;
}

int
mprotect(__unused proc_t p, struct mprotect_args *uap, __unused int32_t *retval)
{
	vm_prot_t prot;
	mach_vm_offset_t        user_addr;
	mach_vm_size_t  user_size;
	kern_return_t   result;
	vm_map_t        user_map;
#if CONFIG_MACF
	int error;
#endif

	AUDIT_ARG(addr, uap->addr);
	AUDIT_ARG(len, uap->len);
	AUDIT_ARG(value32, uap->prot);

	user_map = current_map();
	user_addr = (mach_vm_offset_t) uap->addr;
	user_size = (mach_vm_size_t) uap->len;
	prot = (vm_prot_t)(uap->prot & (VM_PROT_ALL | VM_PROT_TRUSTED | VM_PROT_STRIP_READ));

	if (mach_vm_range_overflows(user_addr, user_size)) {
		return EINVAL;
	}
	if (user_addr & vm_map_page_mask(user_map)) {
		                                                                
		return EINVAL;
	}

#ifdef notyet
            
#if defined(VM_PROT_READ_IS_EXEC)
	if (prot & VM_PROT_READ) {
		prot |= VM_PROT_EXECUTE;
	}
#endif
#endif             

#if 3936456
	if (prot & (VM_PROT_EXECUTE | VM_PROT_WRITE)) {
		prot |= VM_PROT_READ;
	}
#endif               

#if defined(__arm64__)
	if (prot & VM_PROT_STRIP_READ) {
		prot &= ~(VM_PROT_READ | VM_PROT_STRIP_READ);
	}
#endif

#if CONFIG_MACF
	  
                                                               
                                                                    
                                                                 
                                      
                                                                    
                               
                                                                 
                                      
    
	error = mac_proc_check_mprotect(p, user_addr,
	    user_size, prot);
	if (error) {
		return error;
	}
#endif

	if (prot & VM_PROT_TRUSTED) {
#if CONFIG_DYNAMIC_CODE_SIGNING
		                                            
		                                                                      
                                                   
                                                                    
                                                                         
                                                                      
                                       
		result = vm_map_sign(
			user_map,
			vm_map_trunc_page(user_addr,
			vm_map_page_mask(user_map)),
			vm_map_round_page(user_addr + user_size,
			vm_map_page_mask(user_map)));
		switch (result) {
		case KERN_SUCCESS:
			break;
		case KERN_INVALID_ADDRESS:
			                                                            
			return ENOMEM;
		default:
			return EINVAL;
		}
#else
		return ENOTSUP;
#endif
	}
	prot &= ~VM_PROT_TRUSTED;

	result = mach_vm_protect(user_map, user_addr, user_size,
	    FALSE, prot);
	switch (result) {
	case KERN_SUCCESS:
		return 0;
	case KERN_PROTECTION_FAILURE:
		return EACCES;
	case KERN_INVALID_ADDRESS:
		                                                            
		return ENOMEM;
	}
	return EINVAL;
}


int
minherit(__unused proc_t p, struct minherit_args *uap, __unused int32_t *retval)
{
	mach_vm_offset_t addr;
	mach_vm_size_t size;
	vm_inherit_t inherit;
	vm_map_t        user_map;
	kern_return_t   result;

	AUDIT_ARG(addr, uap->addr);
	AUDIT_ARG(len, uap->len);
	AUDIT_ARG(value32, uap->inherit);

	addr = (mach_vm_offset_t)uap->addr;
	size = (mach_vm_size_t)uap->len;
	inherit = uap->inherit;
	if (mach_vm_range_overflows(addr, size)) {
		return EINVAL;
	}
	user_map = current_map();
	result = mach_vm_inherit(user_map, addr, size,
	    inherit);
	switch (result) {
	case KERN_SUCCESS:
		return 0;
	case KERN_PROTECTION_FAILURE:
		return EACCES;
	}
	return EINVAL;
}

int
madvise(__unused proc_t p, struct madvise_args *uap, __unused int32_t *retval)
{
	vm_map_t user_map;
	mach_vm_offset_t start;
	mach_vm_size_t size;
	vm_behavior_t new_behavior;
	kern_return_t   result;

	  
                                                                   
             
    
	switch (uap->behav) {
	case MADV_RANDOM:
		new_behavior = VM_BEHAVIOR_RANDOM;
		break;
	case MADV_SEQUENTIAL:
		new_behavior = VM_BEHAVIOR_SEQUENTIAL;
		break;
	case MADV_NORMAL:
		new_behavior = VM_BEHAVIOR_DEFAULT;
		break;
	case MADV_WILLNEED:
		new_behavior = VM_BEHAVIOR_WILLNEED;
		break;
	case MADV_DONTNEED:
		new_behavior = VM_BEHAVIOR_DONTNEED;
		break;
	case MADV_FREE:
		new_behavior = VM_BEHAVIOR_FREE;
		break;
	case MADV_ZERO_WIRED_PAGES:
		new_behavior = VM_BEHAVIOR_ZERO_WIRED_PAGES;
		break;
	case MADV_FREE_REUSABLE:
		new_behavior = VM_BEHAVIOR_REUSABLE;
		break;
	case MADV_FREE_REUSE:
		new_behavior = VM_BEHAVIOR_REUSE;
		break;
	case MADV_CAN_REUSE:
		new_behavior = VM_BEHAVIOR_CAN_REUSE;
		break;
	case MADV_PAGEOUT:
#if MACH_ASSERT
		new_behavior = VM_BEHAVIOR_PAGEOUT;
		break;
#else                  
		return ENOTSUP;
#endif                  
	default:
		return EINVAL;
	}

	start = (mach_vm_offset_t) uap->addr;
	size = (mach_vm_size_t) uap->len;
	if (mach_vm_range_overflows(start, size)) {
		return EINVAL;
	}
#if __arm64__
	if (start == 0 &&
	    size != 0 &&
	    (uap->behav == MADV_FREE ||
	    uap->behav == MADV_FREE_REUSABLE)) {
		printf("** FOURK_COMPAT: %d[%s] "
		    "failing madvise(0x%llx,0x%llx,%s)\n",
		    p->p_pid, p->p_comm, start, size,
		    ((uap->behav == MADV_FREE_REUSABLE)
		    ? "MADV_FREE_REUSABLE"
		    : "MADV_FREE"));
		DTRACE_VM3(fourk_compat_madvise,
		    uint64_t, start,
		    uint64_t, size,
		    int, uap->behav);
		return EINVAL;
	}
#endif                

	user_map = current_map();

	result = mach_vm_behavior_set(user_map, start, size, new_behavior);
	switch (result) {
	case KERN_SUCCESS:
		return 0;
	case KERN_INVALID_ADDRESS:
		return EINVAL;
	case KERN_NO_SPACE:
		return ENOMEM;
	}

	return EINVAL;
}

int
mincore(__unused proc_t p, struct mincore_args *uap, __unused int32_t *retval)
{
	mach_vm_offset_t addr = 0, first_addr = 0, end = 0, cur_end = 0;
	vm_map_t map = VM_MAP_NULL;
	user_addr_t vec = 0;
	int error = 0;
	int64_t lastvecindex = 0;
	int mincoreinfo = 0;
	int pqueryinfo = 0;
	uint64_t pqueryinfo_vec_size = 0;
	vm_page_info_basic_t info = NULL;
	mach_msg_type_number_t count = 0;
	char *kernel_vec = NULL;
	uint64_t req_vec_size_pages = 0, cur_vec_size_pages = 0, vecindex = 0;
	kern_return_t kr = KERN_SUCCESS;
	int effective_page_shift, effective_page_size;

	map = current_map();

	  
                                                               
                                                                      
                                                             
                                           
    
	if (vm_map_page_shift(map) < PAGE_SHIFT) {
		effective_page_shift = vm_map_page_shift(map);
		effective_page_size = vm_map_page_size(map);
	} else {
		effective_page_shift = PAGE_SHIFT;
		effective_page_size = PAGE_SIZE;
	}

	  
                                                             
         
    
	first_addr = addr = vm_map_trunc_page(uap->addr,
	    vm_map_page_mask(map));
	end = vm_map_round_page(uap->addr + uap->len,
	    vm_map_page_mask(map));

	if (end < addr) {
		return EINVAL;
	}

	if (end == addr) {
		return 0;
	}

	  
                                                               
                                      
    

	req_vec_size_pages = (end - addr) >> effective_page_shift;
	cur_vec_size_pages = MIN(req_vec_size_pages, (MAX_PAGE_RANGE_QUERY >> effective_page_shift));
	size_t kernel_vec_size = cur_vec_size_pages;

	kernel_vec = kheap_alloc(KHEAP_TEMP, kernel_vec_size, Z_WAITOK | Z_ZERO);

	if (kernel_vec == NULL) {
		return ENOMEM;
	}

	  
                          
    
	vec = uap->vec;

	pqueryinfo_vec_size = cur_vec_size_pages * sizeof(struct vm_page_info_basic);

	info = kheap_alloc(KHEAP_TEMP, pqueryinfo_vec_size, Z_WAITOK);

	if (info == NULL) {
		kheap_free(KHEAP_TEMP, kernel_vec, kernel_vec_size);
		return ENOMEM;
	}

	while (addr < end) {
		cur_end = addr + (cur_vec_size_pages * effective_page_size);

		count =  VM_PAGE_INFO_BASIC_COUNT;
		kr = vm_map_page_range_info_internal(map,
		    addr,
		    cur_end,
		    effective_page_shift,
		    VM_PAGE_INFO_BASIC,
		    (vm_page_info_t) info,
		    &count);

		assert(kr == KERN_SUCCESS);

		  
                                                              
                                                               
                            
     
		lastvecindex = -1;

		for (; addr < cur_end; addr += effective_page_size) {
			pqueryinfo = info[lastvecindex + 1].disposition;

			mincoreinfo = 0;

			if (pqueryinfo & VM_PAGE_QUERY_PAGE_PRESENT) {
				mincoreinfo |= MINCORE_INCORE;
			}
			if (pqueryinfo & VM_PAGE_QUERY_PAGE_REF) {
				mincoreinfo |= MINCORE_REFERENCED;
			}
			if (pqueryinfo & VM_PAGE_QUERY_PAGE_DIRTY) {
				mincoreinfo |= MINCORE_MODIFIED;
			}
			if (pqueryinfo & VM_PAGE_QUERY_PAGE_PAGED_OUT) {
				mincoreinfo |= MINCORE_PAGED_OUT;
			}
			if (pqueryinfo & VM_PAGE_QUERY_PAGE_COPIED) {
				mincoreinfo |= MINCORE_COPIED;
			}
			if ((pqueryinfo & VM_PAGE_QUERY_PAGE_EXTERNAL) == 0) {
				mincoreinfo |= MINCORE_ANONYMOUS;
			}
			  
                                                    
      
			vecindex = (addr - first_addr) >> effective_page_shift;
			kernel_vec[vecindex] = (char)mincoreinfo;
			lastvecindex = vecindex;
		}


		assert(vecindex == (cur_vec_size_pages - 1));

		error = copyout(kernel_vec, vec, cur_vec_size_pages * sizeof(char)                      );

		if (error) {
			break;
		}

		  
                                    
                                                                     
                 
                       
     
		vec += cur_vec_size_pages * sizeof(char);
		req_vec_size_pages = (end - addr) >> effective_page_shift;
		cur_vec_size_pages = MIN(req_vec_size_pages, (MAX_PAGE_RANGE_QUERY >> effective_page_shift));

		first_addr = addr;
	}

	kheap_free(KHEAP_TEMP, info, pqueryinfo_vec_size);
	kheap_free(KHEAP_TEMP, kernel_vec, kernel_vec_size);

	if (error) {
		return EFAULT;
	}

	return 0;
}

int
mlock(__unused proc_t p, struct mlock_args *uap, __unused int32_t *retvalval)
{
	vm_map_t user_map;
	vm_map_offset_t addr;
	vm_map_size_t size, pageoff;
	kern_return_t   result;

	AUDIT_ARG(addr, uap->addr);
	AUDIT_ARG(len, uap->len);

	addr = (vm_map_offset_t) uap->addr;
	size = (vm_map_size_t)uap->len;

	if (vm_map_range_overflows(addr, size)) {
		return EINVAL;
	}

	if (size == 0) {
		return 0;
	}

	user_map = current_map();
	pageoff = (addr & vm_map_page_mask(user_map));
	addr -= pageoff;
	size = vm_map_round_page(size + pageoff, vm_map_page_mask(user_map));

	                                                                          
	result = vm_map_wire_kernel(user_map, addr, addr + size, VM_PROT_NONE, VM_KERN_MEMORY_MLOCK, TRUE);

	if (result == KERN_RESOURCE_SHORTAGE) {
		return EAGAIN;
	} else if (result == KERN_PROTECTION_FAILURE) {
		return EACCES;
	} else if (result != KERN_SUCCESS) {
		return ENOMEM;
	}

	return 0;                         
}

int
munlock(__unused proc_t p, struct munlock_args *uap, __unused int32_t *retval)
{
	mach_vm_offset_t addr;
	mach_vm_size_t size;
	vm_map_t user_map;
	kern_return_t   result;

	AUDIT_ARG(addr, uap->addr);
	AUDIT_ARG(len, uap->len);

	addr = (mach_vm_offset_t) uap->addr;
	size = (mach_vm_size_t)uap->len;
	user_map = current_map();
	if (mach_vm_range_overflows(addr, size)) {
		return EINVAL;
	}
	                                                                      
	result = mach_vm_wire_kernel(host_priv_self(), user_map, addr, size, VM_PROT_NONE, VM_KERN_MEMORY_MLOCK);
	return result == KERN_SUCCESS ? 0 : ENOMEM;
}


int
mlockall(__unused proc_t p, __unused struct mlockall_args *uap, __unused int32_t *retval)
{
	return ENOSYS;
}

int
munlockall(__unused proc_t p, __unused struct munlockall_args *uap, __unused int32_t *retval)
{
	return ENOSYS;
}

#if CONFIG_CODE_DECRYPTION
int
mremap_encrypted(__unused struct proc *p, struct mremap_encrypted_args *uap, __unused int32_t *retval)
{
	mach_vm_offset_t    user_addr;
	mach_vm_size_t      user_size;
	kern_return_t       result;
	vm_map_t    user_map;
	uint32_t    cryptid;
	cpu_type_t  cputype;
	cpu_subtype_t       cpusubtype;
	pager_crypt_info_t  crypt_info;
	const char * cryptname = 0;
	char *vpath;
	int len, ret;
	struct proc_regioninfo_internal pinfo;
	vnode_t vp;
	uintptr_t vnodeaddr;
	uint32_t vid;

	AUDIT_ARG(addr, uap->addr);
	AUDIT_ARG(len, uap->len);

	user_map = current_map();
	user_addr = (mach_vm_offset_t) uap->addr;
	user_size = (mach_vm_size_t) uap->len;

	cryptid = uap->cryptid;
	cputype = uap->cputype;
	cpusubtype = uap->cpusubtype;

	if (mach_vm_range_overflows(user_addr, user_size)) {
		return EINVAL;
	}
	if (user_addr & vm_map_page_mask(user_map)) {
		                                                                
		return EINVAL;
	}

	switch (cryptid) {
	case CRYPTID_NO_ENCRYPTION:
		                                               
		return 0;
	case CRYPTID_APP_ENCRYPTION:
	case CRYPTID_MODEL_ENCRYPTION:
		cryptname = "com.apple.unfree";
		break;
	case 0x10:
		                                                       
                                    
		cryptname = "com.apple.null";
		break;
	default:
		return EINVAL;
	}

	if (NULL == text_crypter_create) {
		return ENOTSUP;
	}

	ret = fill_procregioninfo_onlymappedvnodes( proc_task(p), user_addr, &pinfo, &vnodeaddr, &vid);
	if (ret == 0 || !vnodeaddr) {
		                                                                             
		return EINVAL;
	}

	vp = (vnode_t)vnodeaddr;
	if ((vnode_getwithvid(vp, vid)) == 0) {
		vpath = zalloc(ZV_NAMEI);

		len = MAXPATHLEN;
		ret = vn_getpath(vp, vpath, &len);
		if (ret) {
			zfree(ZV_NAMEI, vpath);
			vnode_put(vp);
			return ret;
		}

		vnode_put(vp);
	} else {
		return EINVAL;
	}

#if 0
	kprintf("%s vpath %s cryptid 0x%08x cputype 0x%08x cpusubtype 0x%08x range 0x%016llx size 0x%016llx\n",
	    __FUNCTION__, vpath, cryptid, cputype, cpusubtype, (uint64_t)user_addr, (uint64_t)user_size);
#endif

	                            
	crypt_file_data_t crypt_data = {
		.filename = vpath,
		.cputype = cputype,
		.cpusubtype = cpusubtype
	};
	result = text_crypter_create(&crypt_info, cryptname, (void*)&crypt_data);
#if VM_MAP_DEBUG_APPLE_PROTECT
	if (vm_map_debug_apple_protect) {
		printf("APPLE_PROTECT: %d[%s] map %p [0x%llx:0x%llx] %s(%s) -> 0x%x\n",
		    p->p_pid, p->p_comm,
		    user_map,
		    (uint64_t) user_addr,
		    (uint64_t) (user_addr + user_size),
		    __FUNCTION__, vpath, result);
	}
#endif                                 
	zfree(ZV_NAMEI, vpath);

	if (result) {
		printf("%s: unable to create decrypter %s, kr=%d\n",
		    __FUNCTION__, cryptname, result);
		if (result == kIOReturnNotPrivileged) {
			                                                 
			return EPERM;
		} else {
			return ENOMEM;
		}
	}

	                                   
	vm_object_offset_t crypto_backing_offset;
	crypto_backing_offset = -1;                                  
	result = vm_map_apple_protected(user_map,
	    user_addr,
	    user_addr + user_size,
	    crypto_backing_offset,
	    &crypt_info,
	    cryptid);
	if (result) {
		printf("%s: mapping failed with %d\n", __FUNCTION__, result);
	}

	if (result) {
		return EPERM;
	}
	return 0;
}
#endif                             
