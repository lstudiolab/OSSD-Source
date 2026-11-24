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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved.
 *
 *	File:	bsd/kern/kern_core.c
 *
 *	This file contains machine independent code for performing core dumps.
 *
 */
#if CONFIG_COREDUMP

#include <mach/vm_param.h>
#include <mach/thread_status.h>
#include <sys/content_protection.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/namei.h>
#include <sys/vnode_internal.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/acct.h>
#include <sys/file_internal.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/stat.h>

#include <mach-o/loader.h>
#include <mach/vm_region.h>
#include <mach/vm_statistics.h>

#include <vm/vm_kern.h>
#include <vm/vm_protos.h>           
#include <vm/vm_map.h>                             
#include <mach/mach_vm.h>                                     
#include <mach/task.h>                              
#include <kern/task.h>                                  

#include <security/audit/audit.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif                  

typedef struct {
	int     flavor;                                                 
	mach_msg_type_number_t  count;                                    
} mythread_state_flavor_t;

#if defined (__i386__) || defined (__x86_64__)
mythread_state_flavor_t thread_flavor_array[] = {
	{x86_THREAD_STATE, x86_THREAD_STATE_COUNT},
	{x86_FLOAT_STATE, x86_FLOAT_STATE_COUNT},
	{x86_EXCEPTION_STATE, x86_EXCEPTION_STATE_COUNT},
};
int mynum_flavors = 3;
#elif defined (__arm__)
mythread_state_flavor_t thread_flavor_array[] = {
	{ARM_THREAD_STATE, ARM_THREAD_STATE_COUNT},
	{ARM_VFP_STATE, ARM_VFP_STATE_COUNT},
	{ARM_EXCEPTION_STATE, ARM_EXCEPTION_STATE_COUNT}
};
int mynum_flavors = 3;

#elif defined (__arm64__)
mythread_state_flavor_t thread_flavor_array[] = {
	{ARM_THREAD_STATE64, ARM_THREAD_STATE64_COUNT},
	                     
	{ARM_EXCEPTION_STATE64, ARM_EXCEPTION_STATE64_COUNT}
};
int mynum_flavors = 2;
#else
#error architecture not supported
#endif


typedef struct {
	vm_offset_t header;
	size_t hoffset;
	mythread_state_flavor_t *flavors;
	size_t tstate_size;
	size_t flavor_count;
} tir_t;

extern int freespace_mb(vnode_t vp);

                                       
kern_return_t thread_getstatus(thread_t act, int flavor,
    thread_state_t tstate, mach_msg_type_number_t *count);
void task_act_iterate_wth_args(task_t, void (*)(thread_t, void *), void *);

#ifdef SECURE_KERNEL
__XNU_PRIVATE_EXTERN int do_coredump = 0;                                      
#else
__XNU_PRIVATE_EXTERN int do_coredump = 1;                                
#endif
__XNU_PRIVATE_EXTERN int sugid_coredump = 0;                                      


                                                                           
                                                    

static cpu_type_t
process_cpu_type(proc_t core_proc)
{
	cpu_type_t what_we_think;
#if defined (__i386__) || defined (__x86_64__)
	if (IS_64BIT_PROCESS(core_proc)) {
		what_we_think = CPU_TYPE_X86_64;
	} else {
		what_we_think = CPU_TYPE_I386;
	}
#elif defined (__arm__) || defined(__arm64__)
	if (IS_64BIT_PROCESS(core_proc)) {
		what_we_think = CPU_TYPE_ARM64;
	} else {
		what_we_think = CPU_TYPE_ARM;
	}
#endif

	return what_we_think;
}

static cpu_type_t
process_cpu_subtype(proc_t core_proc)
{
	cpu_type_t what_we_think;
#if defined (__i386__) || defined (__x86_64__)
	if (IS_64BIT_PROCESS(core_proc)) {
		what_we_think = CPU_SUBTYPE_X86_64_ALL;
	} else {
		what_we_think = CPU_SUBTYPE_I386_ALL;
	}
#elif defined (__arm__) || defined(__arm64__)
	if (IS_64BIT_PROCESS(core_proc)) {
		what_we_think = CPU_SUBTYPE_ARM64_ALL;
	} else {
		what_we_think = CPU_SUBTYPE_ARM_ALL;
	}
#endif
	return what_we_think;
}

static void
collectth_state(thread_t th_act, void *tirp)
{
	vm_offset_t     header;
	size_t  hoffset, i;
	mythread_state_flavor_t *flavors;
	struct thread_command   *tc;
	tir_t *t = (tir_t *)tirp;

	  
                                     
    
	header = t->header;
	hoffset = t->hoffset;
	flavors = t->flavors;

	tc = (struct thread_command *) (header + hoffset);
	tc->cmd = LC_THREAD;
	tc->cmdsize = (uint32_t)(sizeof(struct thread_command)
	    + t->tstate_size);
	hoffset += sizeof(struct thread_command);
	  
                                                
                                                
                        
    
	for (i = 0; i < t->flavor_count; i++) {
		*(mythread_state_flavor_t *)(header + hoffset) =
		    flavors[i];
		hoffset += sizeof(mythread_state_flavor_t);
		thread_getstatus(th_act, flavors[i].flavor,
		    (thread_state_t)(header + hoffset),
		    &flavors[i].count);
		hoffset += flavors[i].count * sizeof(int);
	}

	t->hoffset = hoffset;
}

  
           
  
                                                                      
             
  
                                                   
                                                     
                                         
                                                             
  
                        
                       
  
                                                                          
                                                              
            
   
#define MAX_TSTATE_FLAVORS      10
int
coredump(proc_t core_proc, uint32_t reserve_mb, int coredump_flags)
{
                                                                 
	vfs_context_t ctx = vfs_context_current();
	vm_map_t        map = current_map();
	task_t          task = current_task();
                     
	kauth_cred_t cred = vfs_context_ucred(ctx);
	int error = 0;
	struct vnode_attr va;
	size_t          thread_count, segment_count;
	size_t          command_size, header_size, tstate_size;
	size_t          hoffset;
	off_t           foffset;
	mach_vm_offset_t vmoffset;
	vm_offset_t     header;
	mach_vm_size_t  vmsize;
	vm_prot_t       prot;
	vm_prot_t       maxprot;
	vm_inherit_t    inherit;
	int             error1 = 0;
	char            stack_name[MAXCOMLEN + 6];
	char            *alloced_name = NULL;
	char            *name = NULL;
	mythread_state_flavor_t flavors[MAX_TSTATE_FLAVORS];
	vm_size_t       mapsize;
	size_t          i;
	uint32_t nesting_depth = 0;
	kern_return_t   kret;
	struct vm_region_submap_info_64 vbr;
	mach_msg_type_number_t vbrcount = 0;
	tir_t tir1;
	struct vnode * vp;
	struct mach_header      *mh = NULL;                             
	struct mach_header_64   *mh64 = NULL;                           
	int             is_64 = 0;
	size_t          mach_header_sz = sizeof(struct mach_header);
	size_t          segment_command_sz = sizeof(struct segment_command);

	if (current_proc() != core_proc) {
		panic("coredump() called against proc that is not current_proc: %p", core_proc);
	}

	if (do_coredump == 0 ||                                 
	    ((sugid_coredump == 0) &&                                       
	    ((kauth_cred_getsvuid(cred) != kauth_cred_getruid(cred)) ||
	    (kauth_cred_getsvgid(cred) != kauth_cred_getrgid(cred))))) {
		error = EFAULT;
		goto out2;
	}

#if CONFIG_MACF
	error = mac_proc_check_dump_core(core_proc);
	if (error != 0) {
		goto out2;
	}
#endif

	if (IS_64BIT_PROCESS(core_proc)) {
		is_64 = 1;
		mach_header_sz = sizeof(struct mach_header_64);
		segment_command_sz = sizeof(struct segment_command_64);
	}

	mapsize = get_vmmap_size(map);

	if (((coredump_flags & COREDUMP_IGNORE_ULIMIT) == 0) &&
	    (mapsize >= proc_limitgetcur(core_proc, RLIMIT_CORE, FALSE))) {
		error = EFAULT;
		goto out2;
	}

	(void) task_suspend_internal(task);

	alloced_name = zalloc_flags(ZV_NAMEI, Z_NOWAIT | Z_ZERO);

	                                                        
	                                                                  
	if (alloced_name == NULL ||
	    proc_core_name(core_proc->p_comm, kauth_cred_getuid(cred),
	    core_proc->p_pid, alloced_name, MAXPATHLEN)) {
		snprintf(stack_name, sizeof(stack_name),
		    "/cores/core.%d", core_proc->p_pid);
		name = stack_name;
	} else {
		name = alloced_name;
	}

	if ((error = vnode_open(name, (O_CREAT | FWRITE | O_NOFOLLOW), S_IRUSR, VNODE_LOOKUP_NOFOLLOW, &vp, ctx))) {
		goto out2;
	}

	VATTR_INIT(&va);
	VATTR_WANTED(&va, va_nlink);
	                                                          
	if (vp->v_type != VREG ||
	    vnode_getattr(vp, &va, ctx) || va.va_nlink != 1) {
		error = EFAULT;
		goto out;
	}

	VATTR_INIT(&va);                                                                         
	VATTR_SET(&va, va_data_size, 0);
	if (core_proc == initproc) {
		VATTR_SET(&va, va_dataprotect_class, PROTECTION_CLASS_D);
	}
	vnode_setattr(vp, &va, ctx);
	core_proc->p_acflag |= ACORE;

	if ((reserve_mb > 0) &&
	    ((freespace_mb(vp) - (mapsize >> 20)) < reserve_mb)) {
		error = ENOSPC;
		goto out;
	}

	  
                                                  
                                                  
                                         
    

	thread_count = get_task_numacts(task);
	segment_count = get_vmmap_entries(map);          
	tir1.flavor_count = sizeof(thread_flavor_array) / sizeof(mythread_state_flavor_t);
	bcopy(thread_flavor_array, flavors, sizeof(thread_flavor_array));
	tstate_size = 0;
	for (i = 0; i < tir1.flavor_count; i++) {
		tstate_size += sizeof(mythread_state_flavor_t) +
		    (flavors[i].count * sizeof(int));
	}

	{
		size_t lhs;
		size_t rhs;

		                                              
		if (os_mul_overflow(segment_count, segment_command_sz, &lhs)) {
			error = ENOMEM;
			goto out;
		}

		                                                                        
		if (os_add_and_mul_overflow(tstate_size, sizeof(struct thread_command), thread_count, &rhs)) {
			error = ENOMEM;
			goto out;
		}

		                              
		if (os_add_overflow(lhs, rhs, &command_size)) {
			error = ENOMEM;
			goto out;
		}
	}

	if (os_add_overflow(command_size, mach_header_sz, &header_size)) {
		error = ENOMEM;
		goto out;
	}

	if (kmem_alloc(kernel_map, &header, (vm_size_t)header_size, VM_KERN_MEMORY_DIAG) != KERN_SUCCESS) {
		error = ENOMEM;
		goto out;
	}

	  
                         
    
	if (is_64) {
		mh64 = (struct mach_header_64 *)header;
		mh64->magic = MH_MAGIC_64;
		mh64->cputype = process_cpu_type(core_proc);
		mh64->cpusubtype = process_cpu_subtype(core_proc);
		mh64->filetype = MH_CORE;
		mh64->ncmds = (uint32_t)(segment_count + thread_count);
		mh64->sizeofcmds = (uint32_t)command_size;
		mh64->reserved = 0;                                   
	} else {
		mh = (struct mach_header *)header;
		mh->magic = MH_MAGIC;
		mh->cputype = process_cpu_type(core_proc);
		mh->cpusubtype = process_cpu_subtype(core_proc);
		mh->filetype = MH_CORE;
		mh->ncmds = (uint32_t)(segment_count + thread_count);
		mh->sizeofcmds = (uint32_t)command_size;
	}

	hoffset = mach_header_sz;                               
	foffset = round_page(header_size);                            
	vmoffset = MACH_VM_MIN_ADDRESS;                             

	  
                                                          
                     
    
	while (segment_count > 0) {
		struct segment_command          *sc;
		struct segment_command_64       *sc64;

		  
                                            
     

		while (1) {
			vbrcount = VM_REGION_SUBMAP_INFO_COUNT_64;
			if ((kret = mach_vm_region_recurse(map,
			    &vmoffset, &vmsize, &nesting_depth,
			    (vm_region_recurse_info_t)&vbr,
			    &vbrcount)) != KERN_SUCCESS) {
				break;
			}
			  
                                                       
                                                    
                                                      
                                            
      
			if (!(is_64) &&
			    (vmoffset + vmsize > VM_MAX_ADDRESS)) {
				kret = KERN_INVALID_ADDRESS;
				break;
			}
			if (vbr.is_submap) {
				nesting_depth++;
				continue;
			} else {
				break;
			}
		}
		if (kret != KERN_SUCCESS) {
			break;
		}

		prot = vbr.protection;
		maxprot = vbr.max_protection;
		inherit = vbr.inheritance;
		  
                                       
     
		if (is_64) {
			sc64 = (struct segment_command_64 *)(header + hoffset);
			sc64->cmd = LC_SEGMENT_64;
			sc64->cmdsize = sizeof(struct segment_command_64);
			                                          
			sc64->segname[0] = 0;
			sc64->vmaddr = vmoffset;
			sc64->vmsize = vmsize;
			sc64->fileoff = foffset;
			sc64->filesize = vmsize;
			sc64->maxprot = maxprot;
			sc64->initprot = prot;
			sc64->nsects = 0;
			sc64->flags = 0;
		} else {
			sc = (struct segment_command *) (header + hoffset);
			sc->cmd = LC_SEGMENT;
			sc->cmdsize = sizeof(struct segment_command);
			                                          
			sc->segname[0] = 0;
			sc->vmaddr = CAST_DOWN_EXPLICIT(uint32_t, vmoffset);
			sc->vmsize = CAST_DOWN_EXPLICIT(uint32_t, vmsize);
			sc->fileoff = CAST_DOWN_EXPLICIT(uint32_t, foffset);                          
			sc->filesize = CAST_DOWN_EXPLICIT(uint32_t, vmsize);                          
			sc->maxprot = maxprot;
			sc->initprot = prot;
			sc->nsects = 0;
			sc->flags = 0;
		}

		  
                                                   
                                 
     
		if ((prot & VM_PROT_READ) == 0) {
			mach_vm_protect(map, vmoffset, vmsize, FALSE,
			    prot | VM_PROT_READ);
		}
		  
                                                
                                                
                        
     
		if ((maxprot & VM_PROT_READ) == VM_PROT_READ
		    && vbr.user_tag != VM_MEMORY_IOKIT
		    && coredumpok(map, vmoffset)) {
			error = vn_rdwr_64(UIO_WRITE, vp, vmoffset, vmsize, foffset,
			    (IS_64BIT_PROCESS(core_proc) ? UIO_USERSPACE64 : UIO_USERSPACE32),
			    IO_NOCACHE | IO_NODELOCKED | IO_UNIT, cred, (int64_t *) 0, core_proc);
		}

		hoffset += segment_command_sz;
		foffset += vmsize;
		vmoffset += vmsize;
		segment_count--;
	}

	  
                                                               
                                                                   
                                                                
                                                                 
                                                                
    
	if (is_64) {
		mh64->ncmds -= segment_count;
		mh64->sizeofcmds -= segment_count * segment_command_sz;
	} else {
		mh->ncmds -= segment_count;
		mh->sizeofcmds -= segment_count * segment_command_sz;
	}

	tir1.header = header;
	tir1.hoffset = hoffset;
	tir1.flavors = flavors;
	tir1.tstate_size = tstate_size;
	task_act_iterate_wth_args(task, collectth_state, &tir1);

	  
                                                     
                                             
    
	error = vn_rdwr(UIO_WRITE, vp, (caddr_t)header, (int)MIN(header_size, INT_MAX), (off_t)0,
	    UIO_SYSSPACE, IO_NOCACHE | IO_NODELOCKED | IO_UNIT, cred, (int *) 0, core_proc);
	kmem_free(kernel_map, header, header_size);

	if ((coredump_flags & COREDUMP_FULLFSYNC) && error == 0) {
		error = VNOP_IOCTL(vp, F_FULLFSYNC, (caddr_t)NULL, 0, ctx);
	}
out:
	error1 = vnode_close(vp, FWRITE, ctx);
out2:
#if CONFIG_AUDIT
	audit_proc_coredump(core_proc, name, error);
#endif
	if (alloced_name != NULL) {
		zfree(ZV_NAMEI, alloced_name);
	}
	if (error == 0) {
		error = error1;
	}

	return error;
}

#else                      

                                                                        

#error assertion failed: this section is not compiled

#endif                      
