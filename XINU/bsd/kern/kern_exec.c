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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */

/*-
 * Copyright (c) 1982, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
 *	from: @(#)kern_exec.c	8.1 (Berkeley) 6/10/93
 */
  
                                                                      
                                                                          
                                                                        
               
   
#include <machine/reg.h>
#include <machine/cpu_capabilities.h>

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/user.h>
#include <sys/socketvar.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/mount_internal.h>
#include <sys/vnode_internal.h>
#include <sys/file_internal.h>
#include <sys/stat.h>
#include <sys/uio_internal.h>
#include <sys/acct.h>
#include <sys/exec.h>
#include <sys/kdebug.h>
#include <sys/signal.h>
#include <sys/aio_kern.h>
#include <sys/sysproto.h>
#include <sys/sysctl.h>
#include <sys/persona.h>
#include <sys/reason.h>
#if SYSV_SHM
#include <sys/shm_internal.h>                          
#endif
#include <sys/ubc_internal.h>                          
#include <sys/spawn.h>
#include <sys/spawn_internal.h>
#include <sys/process_policy.h>
#include <sys/codesign.h>
#include <sys/random.h>
#include <crypto/sha1.h>

#include <libkern/libkern.h>
#include <libkern/crypto/sha2.h>
#include <security/audit/audit.h>

#include <ipc/ipc_types.h>

#include <mach/mach_param.h>
#include <mach/mach_types.h>
#include <mach/port.h>
#include <mach/task.h>
#include <mach/task_access.h>
#include <mach/thread_act.h>
#include <mach/vm_map.h>
#include <mach/mach_vm.h>
#include <mach/vm_param.h>

#include <kern/sched_prim.h>                      
#include <kern/affinity.h>
#include <kern/assert.h>
#include <kern/task.h>
#include <kern/coalition.h>
#include <kern/policy_internal.h>
#include <kern/kalloc.h>

#include <os/log.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#include <security/mac_mach_internal.h>
#endif

#if CONFIG_AUDIT
#include <bsm/audit_kevents.h>
#endif

#if CONFIG_ARCADE
#include <kern/arcade.h>
#endif

#include <vm/vm_map.h>
#include <vm/vm_kern.h>
#include <vm/vm_protos.h>
#include <vm/vm_kern.h>
#include <vm/vm_fault.h>
#include <vm/vm_pageout.h>
#include <vm/pmap.h>

#include <kdp/kdp_dyld.h>

#include <machine/machine_routines.h>
#include <machine/pal_routines.h>

#include <pexpert/pexpert.h>

#if CONFIG_MEMORYSTATUS
#include <sys/kern_memorystatus.h>
#endif

#include <IOKit/IOBSD.h>
#include <IOKit/IOPlatformExpert.h>

extern boolean_t vm_darkwake_mode;

extern int bootarg_execfailurereports;                 
boolean_t unentitled_ios_sim_launch = FALSE;

#if __has_feature(ptrauth_calls) && defined(XNU_TARGET_OS_OSX)
static TUNABLE(bool, bootarg_arm64e_preview_abi, "-arm64e_preview_abi", false);
#endif                                                                 

#if CONFIG_DTRACE
                                                             
extern void dtrace_proc_exec(proc_t);
extern void (*dtrace_proc_waitfor_exec_ptr)(proc_t);

  
                                                                            
                                                      
   
static void (*dtrace_proc_waitfor_hook)(proc_t) = NULL;

#include <sys/dtrace_ptss.h>
#endif

#if __has_feature(ptrauth_calls)
static int vm_shared_region_per_team_id = 1;
static int vm_shared_region_by_entitlement = 1;

                                                                          
#if DEVELOPMENT || DEBUG || XNU_TARGET_OS_IOS
static int vm_shared_region_reslide_aslr = 1;
#else                                                
static int vm_shared_region_reslide_aslr = 0;
#endif                                                
  
                                                                             
                                            
  
                                               
                                                         
   
int vm_shared_region_reslide_restrict = 1;

#if DEVELOPMENT || DEBUG
SYSCTL_INT(_vm, OID_AUTO, vm_shared_region_per_team_id, CTLFLAG_RW, &vm_shared_region_per_team_id, 0, "");
SYSCTL_INT(_vm, OID_AUTO, vm_shared_region_by_entitlement, CTLFLAG_RW, &vm_shared_region_by_entitlement, 0, "");
SYSCTL_INT(_vm, OID_AUTO, vm_shared_region_reslide_restrict, CTLFLAG_RW, &vm_shared_region_reslide_restrict, 0, "");
SYSCTL_INT(_vm, OID_AUTO, vm_shared_region_reslide_aslr, CTLFLAG_RW, &vm_shared_region_reslide_aslr, 0, "");
#endif

#endif                                   

                                                    
thread_t fork_create_child(task_t parent_task,
    coalition_t *parent_coalition,
    proc_t child_proc,
    int inherit_memory,
    int is_64bit_addr,
    int is_64bit_data,
    int in_exec);
void vfork_exit(proc_t p, int rv);
extern void proc_apply_task_networkbg_internal(proc_t, thread_t);
extern void task_set_did_exec_flag(task_t task);
extern void task_clear_exec_copy_flag(task_t task);
proc_t proc_exec_switch_task(proc_t p, task_t old_task, task_t new_task, thread_t new_thread, void **inherit);
boolean_t task_is_active(task_t);
boolean_t thread_is_active(thread_t thread);
void thread_copy_resource_info(thread_t dst_thread, thread_t src_thread);
void *ipc_importance_exec_switch_task(task_t old_task, task_t new_task);
extern void ipc_importance_release(void *elem);
extern boolean_t task_has_watchports(task_t task);
extern void task_set_no_smt(task_t task);
#if defined(HAS_APPLE_PAC)
char *task_get_vm_shared_region_id_and_jop_pid(task_t task, uint64_t *jop_pid);
#endif
task_t convert_port_to_task(ipc_port_t port);

  
                                                                     
   
#define IPC_OBJECT_COPYIN_FLAGS_ALLOW_IMMOVABLE_SEND 0x1
void            ipc_task_reset(
	task_t          task);
void            ipc_thread_reset(
	thread_t        thread);
kern_return_t ipc_object_copyin(
	ipc_space_t             space,
	mach_port_name_t        name,
	mach_msg_type_name_t    msgt_name,
	ipc_object_t            *objectp,
	mach_port_context_t     context,
	mach_msg_guard_flags_t  *guard_flags,
	uint32_t                kmsg_flags);
void ipc_port_release_send(ipc_port_t);

#if DEVELOPMENT || DEBUG
void task_importance_update_owner_info(task_t);
#endif

extern struct savearea *get_user_regs(thread_t);

__attribute__((noinline)) int __EXEC_WAITING_ON_TASKGATED_CODE_SIGNATURE_UPCALL__(mach_port_t task_access_port, int32_t new_pid);

#include <kern/thread.h>
#include <kern/task.h>
#include <kern/ast.h>
#include <kern/mach_loader.h>
#include <kern/mach_fat.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <machine/vmparam.h>
#include <sys/imgact.h>

#include <sys/sdt.h>


  
                                                                
                                                       
                             
   
#define EAI_ITERLIMIT           3

  
                             
   
#define IS_WHITESPACE(ch) ((ch == ' ') || (ch == '\t'))
#define IS_EOL(ch) ((ch == '#') || (ch == '\n'))

extern vm_map_t bsd_pageable_map;
extern const struct fileops vnops;
extern int nextpidversion;


#define USER_ADDR_ALIGN(addr, val) \
	( ( (user_addr_t)(addr) + (val) - 1) \
	        & ~((val) - 1) )

  
                             
   
#define SPAWN_SUBSYSTEM_ROOT_ENTITLEMENT "com.apple.private.spawn-subsystem-root"

                                
static int platform_exec_logging = 0;

SYSCTL_DECL(_security_mac);

SYSCTL_INT(_security_mac, OID_AUTO, platform_exec_logging, CTLFLAG_RW, &platform_exec_logging, 0,
    "log cdhashes for all platform binary executions");

static os_log_t peLog = OS_LOG_DEFAULT;

struct exec_port_actions {
	uint32_t portwatch_count;
	uint32_t registered_count;
	ipc_port_t *portwatch_array;
	ipc_port_t *registered_array;
};

struct image_params;                 
static int exec_activate_image(struct image_params *imgp);
static int exec_copyout_strings(struct image_params *imgp, user_addr_t *stackp);
static int load_return_to_errno(load_return_t lrtn);
static int execargs_alloc(struct image_params *imgp);
static int execargs_free(struct image_params *imgp);
static int exec_check_permissions(struct image_params *imgp);
static int exec_extract_strings(struct image_params *imgp);
static int exec_add_apple_strings(struct image_params *imgp, const load_result_t *load_result);
static int exec_handle_sugid(struct image_params *imgp);
static int sugid_scripts = 0;
SYSCTL_INT(_kern, OID_AUTO, sugid_scripts, CTLFLAG_RW | CTLFLAG_LOCKED, &sugid_scripts, 0, "");
static kern_return_t create_unix_stack(vm_map_t map, load_result_t* load_result, proc_t p);
static int copyoutptr(user_addr_t ua, user_addr_t ptr, int ptr_size);
static void exec_resettextvp(proc_t, struct image_params *);
static int check_for_signature(proc_t, struct image_params *);
static void exec_prefault_data(proc_t, struct image_params *, load_result_t *);
static errno_t exec_handle_port_actions(struct image_params *imgp,
    struct exec_port_actions *port_actions);
static errno_t exec_handle_spawnattr_policy(proc_t p, thread_t thread, int psa_apptype, uint64_t psa_qos_clamp,
    task_role_t psa_darwin_role, struct exec_port_actions *port_actions);
static void exec_port_actions_destroy(struct exec_port_actions *port_actions);

  
                       
  
                                                     
  
                                                           
                                               
                                          
                                                    
  
                       
                                       
  
                    
                                                           
                                                             
                                                             
   
__attribute__((noinline))
static int
exec_add_user_string(struct image_params *imgp, user_addr_t str, int seg, boolean_t is_ncargs)
{
	int error = 0;

	do {
		size_t len = 0;
		int space;

		if (is_ncargs) {
			space = imgp->ip_argspace;                                             
		} else {
			space = imgp->ip_strspace;
		}

		if (space <= 0) {
			error = E2BIG;
			break;
		}

		if (!UIO_SEG_IS_USER_SPACE(seg)) {
			char *kstr = CAST_DOWN(char *, str);               
			error = copystr(kstr, imgp->ip_strendp, space, &len);
		} else {
			error = copyinstr(str, imgp->ip_strendp, space, &len);
		}

		imgp->ip_strendp += len;
		imgp->ip_strspace -= len;
		if (is_ncargs) {
			imgp->ip_argspace -= len;
		}
	} while (error == ENAMETOOLONG);

	return error;
}

  
                                                                   
                                                                  
   
#define EXECUTABLE_KEY "executable_path="

  
                 
  
                                                                        
                                                       
  
                                                                          
                                                                         
                                                                          
                                                                           
  
                                                           
                                         
                                        
  
                           
                         
                                   
                                              
  
                    
                                  
                                                      
                                                    
                                                  
                                        
  
                                                                   
                                                            
                                                             
                                                                
                                                              
                          
   
static int
exec_save_path(struct image_params *imgp, user_addr_t path, int seg, const char **excpath)
{
	int error;
	size_t len;
	char *kpath;

	                                                                         
	            
	memset(imgp->ip_strings, '\0', strlen(EXECUTABLE_KEY) + MAXPATHLEN);

	len = MIN(MAXPATHLEN, imgp->ip_strspace);

	switch (seg) {
	case UIO_USERSPACE32:
	case UIO_USERSPACE64:                             
		error = copyinstr(path, imgp->ip_strings + strlen(EXECUTABLE_KEY), len, &len);
		break;
	case UIO_SYSSPACE:
		kpath = CAST_DOWN(char *, path);           
		error = copystr(kpath, imgp->ip_strings + strlen(EXECUTABLE_KEY), len, &len);
		break;
	default:
		error = EFAULT;
		break;
	}

	if (!error) {
		bcopy(EXECUTABLE_KEY, imgp->ip_strings, strlen(EXECUTABLE_KEY));
		len += strlen(EXECUTABLE_KEY);

		imgp->ip_strendp += len;
		imgp->ip_strspace -= len;

		if (excpath) {
			*excpath = imgp->ip_strings + strlen(EXECUTABLE_KEY);
		}
	}

	return error;
}

  
                       
  
                                                                
                                                             
  
                                                           
  
                           
  
                    
                                  
                                                      
                                                    
                                                  
  
   
static int
exec_reset_save_path(struct image_params *imgp)
{
	imgp->ip_strendp = imgp->ip_strings;
	imgp->ip_argspace = NCARGS;
	imgp->ip_strspace = (NCARGS + PAGE_SIZE);

	return 0;
}

  
                    
  
                                                                     
                                                                     
                                                                 
                                                                 
                                                                
                                                                   
                                                                     
                                                               
  
                                                          
  
                                                  
                                       
                                           
  
                                                                            
                                                                 
   
static int
exec_shell_imgact(struct image_params *imgp)
{
	char *vdata = imgp->ip_vdata;
	char *ihp;
	char *line_startp, *line_endp;
	char *interp;

	  
                                                               
                                                     
    
	if (vdata[0] != '#' ||
	    vdata[1] != '!' ||
	    (imgp->ip_flags & IMGPF_INTERPRET) != 0) {
		return -1;
	}

	if (imgp->ip_origcputype != 0) {
		                                                                    
		return -1;
	}

	imgp->ip_flags |= IMGPF_INTERPRET;
	imgp->ip_interp_sugid_fd = -1;
	imgp->ip_interp_buffer[0] = '\0';

	                                                                    
                         
                                        
    
	if (sugid_scripts == 0) {
		imgp->ip_origvattr->va_mode &= ~(VSUID | VSGID);
	}

	                                                    
	for (ihp = &vdata[2]; ihp < &vdata[IMG_SHSIZE]; ihp++) {
		if (IS_EOL(*ihp)) {
			                                      
			return ENOEXEC;
		} else if (IS_WHITESPACE(*ihp)) {
			                                                     
		} else {
			                                
			break;
		}
	}

	if (ihp == &vdata[IMG_SHSIZE]) {
		                                          
		return ENOEXEC;
	}

	line_startp = ihp;

	                                                        
	for (; ihp < &vdata[IMG_SHSIZE]; ihp++) {
		if (IS_EOL(*ihp)) {
			            
			break;
		} else {
			                                       
		}
	}

	if (ihp == &vdata[IMG_SHSIZE]) {
		                                                       
		return ENOEXEC;
	}

	                                                     
	while (IS_EOL(*ihp) || IS_WHITESPACE(*ihp)) {
		ihp--;
	}

	                                                                            
	line_endp = ihp + 1;

	  
                                               
   
                                                    
                                                        
    

	                               
	interp = imgp->ip_interp_buffer;
	for (ihp = line_startp; (ihp < line_endp) && !IS_WHITESPACE(*ihp); ihp++) {
		*interp++ = *ihp;
	}
	*interp = '\0';

	exec_reset_save_path(imgp);
	exec_save_path(imgp, CAST_USER_ADDR_T(imgp->ip_interp_buffer),
	    UIO_SYSSPACE, NULL);

	                                                                         
	interp = imgp->ip_interp_buffer;
	for (ihp = line_startp; (ihp < line_endp); ihp++) {
		*interp++ = *ihp;
	}
	*interp = '\0';

#if CONFIG_SETUID
	  
                                                               
                                                            
                                                          
    
	if (imgp->ip_origvattr->va_mode & (VSUID | VSGID)) {
		proc_t p;
		struct fileproc *fp;
		int fd;
		int error;

		p = vfs_context_proc(imgp->ip_vfs_context);
		error = falloc(p, &fp, &fd, imgp->ip_vfs_context);
		if (error) {
			return error;
		}

		fp->fp_glob->fg_flag = FREAD;
		fp->fp_glob->fg_ops = &vnops;
		fp->fp_glob->fg_data = (caddr_t)imgp->ip_vp;

		proc_fdlock(p);
		procfdtbl_releasefd(p, fd, NULL);
		fp_drop(p, fd, fp, 1);
		proc_fdunlock(p);
		vnode_ref(imgp->ip_vp);

		imgp->ip_interp_sugid_fd = fd;
	}
#endif                    

	return -3;
}



  
                  
  
                                                                       
                                                                      
                                                                    
                                                                    
                                               
  
                                                          
  
                                                
                                             
                              
  
                                                         
  
                                                                
                                                             
                          
  
                                                                        
                                                              
                                                               
                                                           
   
static int
exec_fat_imgact(struct image_params *imgp)
{
	proc_t p = vfs_context_proc(imgp->ip_vfs_context);
	kauth_cred_t cred = kauth_cred_proc_ref(p);
	struct fat_header *fat_header = (struct fat_header *)imgp->ip_vdata;
	struct _posix_spawnattr *psa = NULL;
	struct fat_arch fat_arch;
	int resid, error;
	load_return_t lret;

	if (imgp->ip_origcputype != 0) {
		                                                                        
		error = -1;                  
		goto bad;
	}

	                                 
	if (OSSwapBigToHostInt32(fat_header->magic) != FAT_MAGIC) {
		error = -1;                  
		goto bad;
	}

	                                                                     
	lret = fatfile_validate_fatarches((vm_offset_t)fat_header, PAGE_SIZE);
	if (lret != LOAD_SUCCESS) {
		error = load_return_to_errno(lret);
		goto bad;
	}

	                                                         
	psa = (struct _posix_spawnattr *) imgp->ip_px_sa;
	if (psa != NULL && psa->psa_binprefs[0] != 0) {
		uint32_t pr = 0;

		                                                               
		for (pr = 0; pr < NBINPREFS; pr++) {
			cpu_type_t pref = psa->psa_binprefs[pr];
			cpu_type_t subpref = psa->psa_subcpuprefs[pr];

			if (pref == 0) {
				                                       
				error = EBADARCH;
				goto bad;
			}

			if (pref == CPU_TYPE_ANY) {
				                                     
				goto regular_grading;
			}

			lret = fatfile_getbestarch_for_cputype(pref,
			    subpref,
			    (vm_offset_t)fat_header,
			    PAGE_SIZE,
			    imgp,
			    &fat_arch);
			if (lret == LOAD_SUCCESS) {
				goto use_arch;
			}
		}

		                                                 
		error = EBADEXEC;
		goto bad;
	}

regular_grading:
	                                                         
	lret = fatfile_getbestarch((vm_offset_t)fat_header,
	    PAGE_SIZE,
	    imgp,
	    &fat_arch,
	    (p->p_flag & P_AFFINITY) != 0);
	if (lret != LOAD_SUCCESS) {
		error = load_return_to_errno(lret);
		goto bad;
	}

use_arch:
	                                            
	error = vn_rdwr(UIO_READ, imgp->ip_vp, imgp->ip_vdata,
	    PAGE_SIZE, fat_arch.offset,
	    UIO_SYSSPACE, (IO_UNIT | IO_NODELOCKED),
	    cred, &resid, p);
	if (error) {
		goto bad;
	}

	if (resid) {
		memset(imgp->ip_vdata + (PAGE_SIZE - resid), 0x0, resid);
	}

	                                                                  
	error = -2;
	imgp->ip_arch_offset = (user_size_t)fat_arch.offset;
	imgp->ip_arch_size = (user_size_t)fat_arch.size;
	imgp->ip_origcputype = fat_arch.cputype;
	imgp->ip_origcpusubtype = fat_arch.cpusubtype;

bad:
	kauth_cred_unref(&cred);
	return error;
}

static int
activate_exec_state(task_t task, proc_t p, thread_t thread, load_result_t *result)
{
	int ret;

	task_set_dyld_info(task, MACH_VM_MIN_ADDRESS, 0);
	task_set_64bit(task, result->is_64bit_addr, result->is_64bit_data);
	if (result->is_64bit_addr) {
		OSBitOrAtomic(P_LP64, &p->p_flag);
	} else {
		OSBitAndAtomic(~((uint32_t)P_LP64), &p->p_flag);
	}
	task_set_mach_header_address(task, result->mach_header);

	ret = thread_state_initialize(thread);
	if (ret != KERN_SUCCESS) {
		return ret;
	}

	if (result->threadstate) {
		uint32_t *ts = result->threadstate;
		uint32_t total_size = (uint32_t)result->threadstate_sz;

		while (total_size > 0) {
			uint32_t flavor = *ts++;
			uint32_t size = *ts++;

			ret = thread_setstatus(thread, flavor, (thread_state_t)ts, size);
			if (ret) {
				return ret;
			}
			ts += size;
			total_size -= (size + 2) * sizeof(uint32_t);
		}
	}

	thread_setentrypoint(thread, result->entry_point);

	return KERN_SUCCESS;
}


  
                                                         
   
static void
set_proc_name(struct image_params *imgp, proc_t p)
{
	int p_name_len = sizeof(p->p_name) - 1;

	if (imgp->ip_ndp->ni_cnd.cn_namelen > p_name_len) {
		imgp->ip_ndp->ni_cnd.cn_namelen = p_name_len;
	}

	bcopy((caddr_t)imgp->ip_ndp->ni_cnd.cn_nameptr, (caddr_t)p->p_name,
	    (unsigned)imgp->ip_ndp->ni_cnd.cn_namelen);
	p->p_name[imgp->ip_ndp->ni_cnd.cn_namelen] = '\0';

	if (imgp->ip_ndp->ni_cnd.cn_namelen > MAXCOMLEN) {
		imgp->ip_ndp->ni_cnd.cn_namelen = MAXCOMLEN;
	}

	bcopy((caddr_t)imgp->ip_ndp->ni_cnd.cn_nameptr, (caddr_t)p->p_comm,
	    (unsigned)imgp->ip_ndp->ni_cnd.cn_namelen);
	p->p_comm[imgp->ip_ndp->ni_cnd.cn_namelen] = '\0';
}

#if __has_feature(ptrauth_calls)
   
                                                                       
  
                                                                             
                                                                          
                                                                         
          
  
                                    
                                                                              
                    
   
static inline const char *
get_teamid_for_shared_region(struct image_params *imgp)
{
	assert(imgp->ip_vp != NULL);

	const char *ret = csvnode_get_teamid(imgp->ip_vp, imgp->ip_arch_offset);
	if (ret) {
		return ret;
	}

	struct cs_blob *blob = csvnode_get_blob(imgp->ip_vp, imgp->ip_arch_offset);
	if (csblob_get_platform_binary(blob)) {
		return NULL;
	} else {
		static const char *NO_TEAM_ID = "";
		return NO_TEAM_ID;
	}
}

   
                                                                                   
  
                                             
                                                                              
   
static inline bool
arm64_cpusubtype_uses_ptrauth(cpu_subtype_t cpusubtype)
{
	return (cpusubtype & ~CPU_SUBTYPE_MASK) == CPU_SUBTYPE_ARM64E &&
	       CPU_SUBTYPE_ARM64_PTR_AUTH_VERSION(cpusubtype) == CPU_SUBTYPE_ARM64_PTR_AUTH_CURRENT_VERSION;
}

#endif                                   

   
                                                             
                
  
                                                              
                                    
                                          
                                        
                                              
   
boolean_t
binary_match(cpu_type_t mask, cpu_type_t req_cpu,
    cpu_subtype_t req_subcpu, cpu_type_t test_cpu,
    cpu_subtype_t test_subcpu)
{
	if ((test_cpu & ~mask) != (req_cpu & ~mask)) {
		return FALSE;
	}

	test_subcpu &= ~CPU_SUBTYPE_MASK;
	req_subcpu  &= ~CPU_SUBTYPE_MASK;

	if (test_subcpu != req_subcpu && req_subcpu != (CPU_SUBTYPE_ANY & ~CPU_SUBTYPE_MASK)) {
		return FALSE;
	}

	return TRUE;
}


  
                   
  
                                           
  
                                                          
  
                                                
                                             
                              
                                                     
                   
                                                
                                    
  
                                                             
  
                                                                
                                                             
                          
  
                                                    
   
static int
exec_mach_imgact(struct image_params *imgp)
{
	struct mach_header *mach_header = (struct mach_header *)imgp->ip_vdata;
	proc_t                  p = vfs_context_proc(imgp->ip_vfs_context);
	int                     error = 0;
	task_t                  task;
	task_t                  new_task = NULL;                          
	thread_t                thread;
	struct uthread          *uthread;
	vm_map_t old_map = VM_MAP_NULL;
	vm_map_t map = VM_MAP_NULL;
	load_return_t           lret;
	load_result_t           load_result = {};
	struct _posix_spawnattr *psa = NULL;
	int                     spawn = (imgp->ip_flags & IMGPF_SPAWN);
	int                     vfexec = (imgp->ip_flags & IMGPF_VFORK_EXEC);
	int                     exec = (imgp->ip_flags & IMGPF_EXEC);
	os_reason_t             exec_failure_reason = OS_REASON_NULL;
	boolean_t               reslide = FALSE;

	  
                                                                    
                                                                
                                                               
                                               
    
	if ((mach_header->magic == MH_CIGAM) ||
	    (mach_header->magic == MH_CIGAM_64)) {
		error = EBADARCH;
		goto bad;
	}

	if ((mach_header->magic != MH_MAGIC) &&
	    (mach_header->magic != MH_MAGIC_64)) {
		error = -1;
		goto bad;
	}

	if (mach_header->filetype != MH_EXECUTE) {
		error = -1;
		goto bad;
	}

	if (imgp->ip_origcputype != 0) {
		                                                            
		if (imgp->ip_origcputype != mach_header->cputype ||
		    imgp->ip_origcpusubtype != mach_header->cpusubtype) {
			error = EBADARCH;
			goto bad;
		}
	} else {
		imgp->ip_origcputype = mach_header->cputype;
		imgp->ip_origcpusubtype = mach_header->cpusubtype;
	}

	task = current_task();
	thread = current_thread();
	uthread = get_bsdthread_info(thread);

	if ((mach_header->cputype & CPU_ARCH_ABI64) == CPU_ARCH_ABI64) {
		imgp->ip_flags |= IMGPF_IS_64BIT_ADDR | IMGPF_IS_64BIT_DATA;
	}


	                                                         
	psa = (struct _posix_spawnattr *) imgp->ip_px_sa;
	if (psa != NULL && psa->psa_binprefs[0] != 0) {
		int pr = 0;
		for (pr = 0; pr < NBINPREFS; pr++) {
			cpu_type_t pref = psa->psa_binprefs[pr];
			cpu_subtype_t subpref = psa->psa_subcpuprefs[pr];

			if (pref == 0) {
				                                       
				error = EBADARCH;
				goto bad;
			}

			if (pref == CPU_TYPE_ANY) {
				                             
				goto grade;
			}

			if (binary_match(CPU_ARCH_MASK, pref, subpref,
			    imgp->ip_origcputype, imgp->ip_origcpusubtype)) {
				goto grade;
			}
		}
		error = EBADARCH;
		goto bad;
	}
grade:
	if (!grade_binary(imgp->ip_origcputype, imgp->ip_origcpusubtype & ~CPU_SUBTYPE_MASK,
	    imgp->ip_origcpusubtype & CPU_SUBTYPE_MASK, TRUE)) {
		error = EBADARCH;
		goto bad;
	}

	if (validate_potential_simulator_binary(imgp->ip_origcputype, imgp,
	    imgp->ip_arch_offset, imgp->ip_arch_size) != LOAD_SUCCESS) {
#if __x86_64__
		const char *excpath;
		error = exec_save_path(imgp, imgp->ip_user_fname, imgp->ip_seg, &excpath);
		os_log_error(OS_LOG_DEFAULT, "Unsupported 32-bit executable: \"%s\"", (error) ? imgp->ip_vp->v_name : excpath);
#endif
		error = EBADARCH;
		goto bad;
	}

#if defined(HAS_APPLE_PAC)
	assert(mach_header->cputype == CPU_TYPE_ARM64
	    );

	if ((mach_header->cputype == CPU_TYPE_ARM64 &&
	    arm64_cpusubtype_uses_ptrauth(mach_header->cpusubtype))
	    ) {
		imgp->ip_flags &= ~IMGPF_NOJOP;
	} else {
		imgp->ip_flags |= IMGPF_NOJOP;
	}
#endif

	                                                        
	error = exec_extract_strings(imgp);
	if (error) {
		goto bad;
	}

	AUDIT_ARG(argv, imgp->ip_startargv, imgp->ip_argc,
	    imgp->ip_endargv - imgp->ip_startargv);
	AUDIT_ARG(envv, imgp->ip_endargv, imgp->ip_envc,
	    imgp->ip_endenvv - imgp->ip_endargv);

	  
                                                                    
                                                               
                                                                    
                                                                   
                      
    
	if (vfexec) {
		imgp->ip_new_thread = fork_create_child(task,
		    NULL,
		    p,
		    FALSE,
		    (imgp->ip_flags & IMGPF_IS_64BIT_ADDR),
		    (imgp->ip_flags & IMGPF_IS_64BIT_DATA),
		    FALSE);
		                                                                    
		if (imgp->ip_new_thread == NULL) {
			error = ENOMEM;
			goto bad;
		}
	}


	                                               
	thread = imgp->ip_new_thread;
	uthread = get_bsdthread_info(thread);
	task = new_task = get_threadtask(thread);

	  
                         
   
                                                                  
                                                                   
                                                                      
                                                                      
                                                                   
                                                                 
                                               
    

	  
                                                               
    
	lret = load_machfile(imgp, mach_header, thread, &map, &load_result);
	if (lret != LOAD_SUCCESS) {
		error = load_return_to_errno(lret);

		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
		    p->p_pid, OS_REASON_EXEC, EXEC_EXIT_REASON_BAD_MACHO, 0, 0);
		if (lret == LOAD_BADMACHO_UPX) {
			set_proc_name(imgp, p);
			exec_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_UPX);
			exec_failure_reason->osr_flags |= OS_REASON_FLAG_GENERATE_CRASH_REPORT;
		} else {
			exec_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_BAD_MACHO);

			if (bootarg_execfailurereports) {
				set_proc_name(imgp, p);
				exec_failure_reason->osr_flags |= OS_REASON_FLAG_GENERATE_CRASH_REPORT;
			}
		}

		exec_failure_reason->osr_flags |= OS_REASON_FLAG_CONSISTENT_FAILURE;

		goto badtoolate;
	}

	proc_lock(p);
	{
		p->p_cputype = imgp->ip_origcputype;
		p->p_cpusubtype = imgp->ip_origcpusubtype;
	}
	p->p_platform = load_result.ip_platform;
	p->p_min_sdk = load_result.lr_min_sdk;
	p->p_sdk = load_result.lr_sdk;
	vm_map_set_user_wire_limit(map, (vm_size_t)proc_limitgetcur(p, RLIMIT_MEMLOCK, FALSE));
#if XNU_TARGET_OS_OSX
	if (p->p_platform == PLATFORM_IOS) {
		assert(vm_map_is_alien(map));
	} else {
		assert(!vm_map_is_alien(map));
	}
#endif                        
	proc_unlock(p);

	  
                                                                     
                           
    
	if (load_result.csflags & CS_VALID) {
		imgp->ip_csflags |= load_result.csflags &
		    (CS_VALID | CS_SIGNED | CS_DEV_CODE | CS_LINKER_SIGNED |
		    CS_HARD | CS_KILL | CS_RESTRICT | CS_ENFORCEMENT | CS_REQUIRE_LV |
		    CS_FORCED_LV | CS_ENTITLEMENTS_VALIDATED | CS_DYLD_PLATFORM | CS_RUNTIME |
		    CS_ENTITLEMENT_FLAGS |
		    CS_EXEC_SET_HARD | CS_EXEC_SET_KILL | CS_EXEC_SET_ENFORCEMENT);
	} else {
		imgp->ip_csflags &= ~CS_VALID;
	}

	if (p->p_csflags & CS_EXEC_SET_HARD) {
		imgp->ip_csflags |= CS_HARD;
	}
	if (p->p_csflags & CS_EXEC_SET_KILL) {
		imgp->ip_csflags |= CS_KILL;
	}
	if (p->p_csflags & CS_EXEC_SET_ENFORCEMENT) {
		imgp->ip_csflags |= CS_ENFORCEMENT;
	}
	if (p->p_csflags & CS_EXEC_INHERIT_SIP) {
		if (p->p_csflags & CS_INSTALLER) {
			imgp->ip_csflags |= CS_INSTALLER;
		}
		if (p->p_csflags & CS_DATAVAULT_CONTROLLER) {
			imgp->ip_csflags |= CS_DATAVAULT_CONTROLLER;
		}
		if (p->p_csflags & CS_NVRAM_UNRESTRICTED) {
			imgp->ip_csflags |= CS_NVRAM_UNRESTRICTED;
		}
	}

#if __has_feature(ptrauth_calls) && defined(XNU_TARGET_OS_OSX)
	  
                                                                        
                                                                       
                                   
    
	if ((imgp->ip_origcpusubtype & ~CPU_SUBTYPE_MASK) == CPU_SUBTYPE_ARM64E &&
	    CPU_SUBTYPE_ARM64_PTR_AUTH_VERSION(imgp->ip_origcpusubtype) == 0 &&
	    !load_result.platform_binary &&
	    !bootarg_arm64e_preview_abi) {
		static bool logged_once = false;
		set_proc_name(imgp, p);

		printf("%s: not running binary \"%s\" built against preview arm64e ABI\n", __func__, p->p_name);
		if (!os_atomic_xchg(&logged_once, true, relaxed)) {
			printf("%s: (to allow this, add \"-arm64e_preview_abi\" to boot-args)\n", __func__);
		}

		exec_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_BAD_MACHO);
		if (bootarg_execfailurereports) {
			exec_failure_reason->osr_flags |= OS_REASON_FLAG_GENERATE_CRASH_REPORT;
			exec_failure_reason->osr_flags |= OS_REASON_FLAG_CONSISTENT_FAILURE;
		}
		goto badtoolate;
	}

	if ((imgp->ip_origcpusubtype & ~CPU_SUBTYPE_MASK) != CPU_SUBTYPE_ARM64E &&
	    imgp->ip_origcputype == CPU_TYPE_ARM64 &&
	    load_result.platform_binary &&
	    (imgp->ip_flags & IMGPF_DRIVER) != 0) {
		set_proc_name(imgp, p);
		printf("%s: disallowing arm64 platform driverkit binary \"%s\", should be arm64e\n", __func__, p->p_name);
		exec_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_BAD_MACHO);
		if (bootarg_execfailurereports) {
			exec_failure_reason->osr_flags |= OS_REASON_FLAG_GENERATE_CRASH_REPORT;
			exec_failure_reason->osr_flags |= OS_REASON_FLAG_CONSISTENT_FAILURE;
		}
		goto badtoolate;
	}
#endif                                                                 

	  
                                                      
   
                                                              
                                                                 
                                                                
                                                                   
                                                                  
                        
    
	int cpu_subtype = 0;                                                  
#if __has_feature(ptrauth_calls)
	char *shared_region_id = NULL;
	size_t len;
	char *base;
	const char *cbase;
#define TEAM_ID_PREFIX "T-"
#define ENTITLE_PREFIX "E-"
#define SR_PREFIX_LEN  2
#define SR_ENTITLEMENT "com.apple.pac.shared_region_id"

	if (cpu_type() == CPU_TYPE_ARM64 &&
	    arm64_cpusubtype_uses_ptrauth(p->p_cpusubtype) &&
	    (imgp->ip_flags & IMGPF_NOJOP) == 0) {
		assertf(p->p_cputype == CPU_TYPE_ARM64,
		    "p %p cpu_type() 0x%x p->p_cputype 0x%x p->p_cpusubtype 0x%x",
		    p, cpu_type(), p->p_cputype, p->p_cpusubtype);

		  
                                                              
                                        
     
		cpu_subtype = p->p_cpusubtype & ~CPU_SUBTYPE_MASK;

		  
                                                                
                                                   
     
		if (imgp->ip_inherited_shared_region_id) {
			len = strlen(imgp->ip_inherited_shared_region_id);
			shared_region_id = kheap_alloc(KHEAP_DATA_BUFFERS,
			    len + 1, Z_WAITOK);
			memcpy(shared_region_id, imgp->ip_inherited_shared_region_id, len + 1);
		} else if ((cbase = get_teamid_for_shared_region(imgp)) != NULL) {
			len = strlen(cbase);
			if (vm_shared_region_per_team_id) {
				shared_region_id = kheap_alloc(KHEAP_DATA_BUFFERS,
				    len + SR_PREFIX_LEN + 1, Z_WAITOK);
				memcpy(shared_region_id, TEAM_ID_PREFIX, SR_PREFIX_LEN);
				memcpy(shared_region_id + SR_PREFIX_LEN, cbase, len + 1);
			}
		} else if ((base = IOVnodeGetEntitlement(imgp->ip_vp,
		    (int64_t)imgp->ip_arch_offset, SR_ENTITLEMENT)) != NULL) {
			len = strlen(base);
			if (vm_shared_region_by_entitlement) {
				shared_region_id = kheap_alloc(KHEAP_DATA_BUFFERS,
				    len + SR_PREFIX_LEN + 1, Z_WAITOK);
				memcpy(shared_region_id, ENTITLE_PREFIX, SR_PREFIX_LEN);
				memcpy(shared_region_id + SR_PREFIX_LEN, base, len + 1);
			}
			                                         
			kheap_free(KHEAP_DATA_BUFFERS, base, len + 1);
		}
	}

	if (imgp->ip_flags & IMGPF_RESLIDE) {
		reslide = TRUE;
	}

	                                            
	if (shared_region_id == NULL) {
		shared_region_id = kheap_alloc(KHEAP_DATA_BUFFERS, 1, Z_WAITOK);
		*shared_region_id = 0;
	}

	                                                                           
	shared_region_key_alloc(shared_region_id,
	    imgp->ip_inherited_shared_region_id != NULL, imgp->ip_inherited_jop_pid);
	task_set_shared_region_id(task, shared_region_id);
	shared_region_id = NULL;
#endif                                   

	int cputype = cpu_type();
	vm_map_exec(map, task, load_result.is_64bit_addr, (void *)p->p_fd->fd_rdir, cputype, cpu_subtype, reslide);

#if XNU_TARGET_OS_OSX
#define SINGLE_JIT_ENTITLEMENT "com.apple.security.cs.single-jit"

	if (IOTaskHasEntitlement(task, SINGLE_JIT_ENTITLEMENT)) {
		vm_map_single_jit(map);
	}
#endif                        

	  
                                                       
    
	fdexec(p, psa != NULL ? psa->psa_flags : 0, exec);

	  
                        
    
	error = exec_handle_sugid(imgp);
	if (error) {
		vm_map_deallocate(map);

		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
		    p->p_pid, OS_REASON_EXEC, EXEC_EXIT_REASON_SUGID_FAILURE, 0, 0);

		exec_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_SUGID_FAILURE);
		if (bootarg_execfailurereports) {
			set_proc_name(imgp, p);
			exec_failure_reason->osr_flags |= OS_REASON_FLAG_GENERATE_CRASH_REPORT;
		}

		goto badtoolate;
	}

	  
                      
   
                                                                
                                                                
                                                                
                                        
   
                                                                 
                                                            
                  
    
	old_map = swap_task_map(task, thread, map);
	vm_map_deallocate(old_map);
	old_map = NULL;

	lret = activate_exec_state(task, p, thread, &load_result);
	if (lret != KERN_SUCCESS) {
		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
		    p->p_pid, OS_REASON_EXEC, EXEC_EXIT_REASON_ACTV_THREADSTATE, 0, 0);

		exec_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_ACTV_THREADSTATE);
		if (bootarg_execfailurereports) {
			set_proc_name(imgp, p);
			exec_failure_reason->osr_flags |= OS_REASON_FLAG_GENERATE_CRASH_REPORT;
		}

		goto badtoolate;
	}

	  
                                             
    
	if (imgp->ip_new_thread == NULL) {
		thread_set_mach_voucher(current_thread(), IPC_VOUCHER_NULL);
	}

	                                                                       
	if (!vfexec && !spawn && (p->p_lflag & P_LTRACED)) {
		psignal(p, SIGTRAP);
	}

	if (load_result.unixproc &&
	    create_unix_stack(get_task_map(task),
	    &load_result,
	    p) != KERN_SUCCESS) {
		error = load_return_to_errno(LOAD_NOSPACE);

		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
		    p->p_pid, OS_REASON_EXEC, EXEC_EXIT_REASON_STACK_ALLOC, 0, 0);

		exec_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_STACK_ALLOC);
		if (bootarg_execfailurereports) {
			set_proc_name(imgp, p);
			exec_failure_reason->osr_flags |= OS_REASON_FLAG_GENERATE_CRASH_REPORT;
		}

		goto badtoolate;
	}

	error = exec_add_apple_strings(imgp, &load_result);
	if (error) {
		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
		    p->p_pid, OS_REASON_EXEC, EXEC_EXIT_REASON_APPLE_STRING_INIT, 0, 0);

		exec_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_APPLE_STRING_INIT);
		if (bootarg_execfailurereports) {
			set_proc_name(imgp, p);
			exec_failure_reason->osr_flags |= OS_REASON_FLAG_GENERATE_CRASH_REPORT;
		}
		goto badtoolate;
	}

	                                                     
	old_map = vm_map_switch(get_task_map(task));

	if (load_result.unixproc) {
		user_addr_t     ap;

		  
                                                           
           
     
		ap = p->user_stack;
		error = exec_copyout_strings(imgp, &ap);
		if (error) {
			vm_map_switch(old_map);

			KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
			    p->p_pid, OS_REASON_EXEC, EXEC_EXIT_REASON_COPYOUT_STRINGS, 0, 0);

			exec_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_COPYOUT_STRINGS);
			if (bootarg_execfailurereports) {
				set_proc_name(imgp, p);
				exec_failure_reason->osr_flags |= OS_REASON_FLAG_GENERATE_CRASH_REPORT;
			}
			goto badtoolate;
		}
		                   
		thread_setuserstack(thread, ap);
	}

	if (load_result.dynlinker || load_result.is_cambria) {
		user_addr_t        ap;
		int                     new_ptr_size = (imgp->ip_flags & IMGPF_IS_64BIT_ADDR) ? 8 : 4;

		                      
		ap = thread_adjuserstack(thread, -new_ptr_size);
		error = copyoutptr(load_result.mach_header, ap, new_ptr_size);

		if (error) {
			vm_map_switch(old_map);

			KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
			    p->p_pid, OS_REASON_EXEC, EXEC_EXIT_REASON_COPYOUT_DYNLINKER, 0, 0);

			exec_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_COPYOUT_DYNLINKER);
			if (bootarg_execfailurereports) {
				set_proc_name(imgp, p);
				exec_failure_reason->osr_flags |= OS_REASON_FLAG_GENERATE_CRASH_REPORT;
			}
			goto badtoolate;
		}
		task_set_dyld_info(task, load_result.all_image_info_addr,
		    load_result.all_image_info_size);
	}


	                                                
	exec_prefault_data(p, imgp, &load_result);

	vm_map_switch(old_map);

	  
                       
    
	execsigs(p, thread);

	  
                                                                             
                               
    
	_aio_exec( p );

#if SYSV_SHM
	                                           
	if (!vfexec && p->vm_shm) {
		shmexec(p);
	}
#endif
#if SYSV_SEM
	                             
	semexit(p);
#endif

	  
                                      
    
	p->p_acflag &= ~AFORK;

	set_proc_name(imgp, p);

#if CONFIG_SECLUDED_MEMORY
	if (secluded_for_apps &&
	    load_result.platform_binary) {
		if (strncmp(p->p_name,
		    "Camera",
		    sizeof(p->p_name)) == 0) {
			task_set_could_use_secluded_mem(task, TRUE);
		} else {
			task_set_could_use_secluded_mem(task, FALSE);
		}
		if (strncmp(p->p_name,
		    "mediaserverd",
		    sizeof(p->p_name)) == 0) {
			task_set_could_also_use_secluded_mem(task, TRUE);
		}
	}
#endif                             

#if __arm64__
	if (load_result.legacy_footprint) {
		task_set_legacy_footprint(task);
	}
#endif                

	pal_dbg_set_task_name(task);

	  
                                                                        
                                                                          
                                                                       
                             
    
	if (load_result.platform_binary) {
		if (cs_debug) {
			printf("setting platform binary on task: pid = %d\n", p->p_pid);
		}

		  
                                                                     
                             
     
		task_set_platform_binary(task, TRUE);
	} else {
		if (cs_debug) {
			printf("clearing platform binary on task: pid = %d\n", p->p_pid);
		}

		task_set_platform_binary(task, FALSE);
	}

#if DEVELOPMENT || DEBUG
	  
                                                          
    
	task_importance_update_owner_info(task);
#endif

	memcpy(&p->p_uuid[0], &load_result.uuid[0], sizeof(p->p_uuid));

#if CONFIG_DTRACE
	dtrace_proc_exec(p);
#endif

	if (kdebug_enable) {
		long args[4] = {};

		uintptr_t fsid = 0, fileid = 0;
		if (imgp->ip_vattr) {
			uint64_t fsid64 = vnode_get_va_fsid(imgp->ip_vattr);
			fsid   = (uintptr_t)fsid64;
			fileid = (uintptr_t)imgp->ip_vattr->va_fileid;
			                                                              
			if (fsid != fsid64 || fileid != imgp->ip_vattr->va_fileid) {
				fsid = fileid = 0;
			}
		}
		KERNEL_DEBUG_CONSTANT_IST1(TRACE_DATA_EXEC, p->p_pid, fsid, fileid, 0,
		    (uintptr_t)thread_tid(thread));

		  
                                     
     
		kdbg_trace_string(p, &args[0], &args[1], &args[2], &args[3]);
		KERNEL_DEBUG_CONSTANT_IST1(TRACE_STRING_EXEC, args[0], args[1],
		    args[2], args[3], (uintptr_t)thread_tid(thread));
	}


	  
                                                            
                           
    
	if (imgp->ip_px_sa != NULL) {
		psa = (struct _posix_spawnattr *) imgp->ip_px_sa;
		if (psa->psa_flags & POSIX_SPAWN_START_SUSPENDED) {
			proc_lock(p);
			p->p_stat = SSTOP;
			proc_unlock(p);
			(void) task_suspend_internal(task);
		}
	}

	  
                                                                     
                                             
    
	OSBitOrAtomic(P_EXEC, &p->p_flag);
	proc_resetregister(p);
	if (p->p_pptr && (p->p_lflag & P_LPPWAIT)) {
		proc_lock(p);
		p->p_lflag &= ~P_LPPWAIT;
		proc_unlock(p);
		wakeup((caddr_t)p->p_pptr);
	}

	  
                                                                
                                                         
    
	if (vfexec && (p->p_lflag & P_LTRACED)) {
		psignal_vfork(p, new_task, thread, SIGTRAP);
	}

	goto done;

badtoolate:
	                                                           
	if (!spawn) {
		if (vfexec) {
			assert(exec_failure_reason != OS_REASON_NULL);
			psignal_vfork_with_reason(p, new_task, thread, SIGKILL, exec_failure_reason);
			exec_failure_reason = OS_REASON_NULL;
		} else {
			assert(exec_failure_reason != OS_REASON_NULL);
			psignal_with_reason(p, SIGKILL, exec_failure_reason);
			exec_failure_reason = OS_REASON_NULL;

			if (exec) {
				                                  
				task_terminate_internal(task);
			}
		}

		                                                                                
		error = 0;
	} else {
		os_reason_free(exec_failure_reason);
		exec_failure_reason = OS_REASON_NULL;
	}

done:
	if (load_result.threadstate) {
		kfree(load_result.threadstate, load_result.threadstate_sz);
		load_result.threadstate = NULL;
	}

bad:
	                                                                
	assert(exec_failure_reason == OS_REASON_NULL);
	return error;
}




  
                                                                         
                                                                         
                            
  
                                                 
   
struct execsw {
	int(*const ex_imgact)(struct image_params *);
	const char *ex_name;
}const execsw[] = {
	{ exec_mach_imgact, "Mach-o Binary" },
	{ exec_fat_imgact, "Fat Binary" },
	{ exec_shell_imgact, "Interpreter Script" },
	{ NULL, NULL}
};


  
                      
  
                                                                            
                                                                
                                                                       
                            
  
                                                          
  
                       
                                               
                                          
                                           
                                             
                                          
                                     
                                                
                                                  
                                                              
                                                                       
                             
            
                                              
                                                    
                                         
   
static int
exec_activate_image(struct image_params *imgp)
{
	struct nameidata *ndp = NULL;
	const char *excpath;
	int error;
	int resid;
	int once = 1;                                              
	int i;
	int itercount = 0;
	proc_t p = vfs_context_proc(imgp->ip_vfs_context);

	error = execargs_alloc(imgp);
	if (error) {
		goto bad_notrans;
	}

	error = exec_save_path(imgp, imgp->ip_user_fname, imgp->ip_seg, &excpath);
	if (error) {
		goto bad_notrans;
	}

	                                                         
	DTRACE_PROC1(exec, uintptr_t, excpath);

	ndp = kheap_alloc(KHEAP_TEMP, sizeof(*ndp), Z_WAITOK | Z_ZERO);
	if (ndp == NULL) {
		error = ENOMEM;
		goto bad_notrans;
	}

	NDINIT(ndp, LOOKUP, OP_LOOKUP, FOLLOW | LOCKLEAF | AUDITVNPATH1,
	    UIO_SYSSPACE, CAST_USER_ADDR_T(excpath), imgp->ip_vfs_context);

again:
	error = namei(ndp);
	if (error) {
		goto bad_notrans;
	}
	imgp->ip_ndp = ndp;                                                     
	imgp->ip_vp = ndp->ni_vp;                                                      

	  
                                                                  
                                                                    
                                                                
                                                                    
             
    
	proc_lock(p);
	if (p->p_lflag & P_LEXIT) {
		error = EDEADLK;
		proc_unlock(p);
		goto bad_notrans;
	}
	error = proc_transstart(p, 1, 0);
	proc_unlock(p);
	if (error) {
		goto bad_notrans;
	}

	error = exec_check_permissions(imgp);
	if (error) {
		goto bad;
	}

	                                                                       
	if (once) {
		once = 0;
		*imgp->ip_origvattr = *imgp->ip_vattr;
	}

	error = vn_rdwr(UIO_READ, imgp->ip_vp, imgp->ip_vdata, PAGE_SIZE, 0,
	    UIO_SYSSPACE, IO_NODELOCKED,
	    vfs_context_ucred(imgp->ip_vfs_context),
	    &resid, vfs_context_proc(imgp->ip_vfs_context));
	if (error) {
		goto bad;
	}

	if (resid) {
		memset(imgp->ip_vdata + (PAGE_SIZE - resid), 0x0, resid);
	}

encapsulated_binary:
	                                                                   
	if (++itercount > EAI_ITERLIMIT) {
		error = EBADEXEC;
		goto bad;
	}
	error = -1;
	for (i = 0; error == -1 && execsw[i].ex_imgact != NULL; i++) {
		error = (*execsw[i].ex_imgact)(imgp);

		switch (error) {
		                                    
		case -2:                                                                              
			goto encapsulated_binary;

		case -3:                                 
#if CONFIG_MACF
			  
                                                    
                                                   
                                       
      
			if (imgp->ip_scriptlabelp) {
				mac_vnode_label_free(imgp->ip_scriptlabelp);
			}
			imgp->ip_scriptlabelp = mac_vnode_label_alloc();
			if (imgp->ip_scriptlabelp == NULL) {
				error = ENOMEM;
				break;
			}
			mac_vnode_label_copy(imgp->ip_vp->v_label,
			    imgp->ip_scriptlabelp);

			  
                                                   
      
			if (imgp->ip_scriptvp) {
				vnode_put(imgp->ip_scriptvp);
				imgp->ip_scriptvp = NULLVP;
			}
			if (vnode_getwithref(imgp->ip_vp) == 0) {
				imgp->ip_scriptvp = imgp->ip_vp;
			}
#endif

			nameidone(ndp);

			vnode_put(imgp->ip_vp);
			imgp->ip_vp = NULL;                      
			imgp->ip_ndp = NULL;                        

			                                                                   
			NDINIT(ndp, LOOKUP, OP_LOOKUP, FOLLOW | LOCKLEAF,
			    UIO_SYSSPACE, CAST_USER_ADDR_T(excpath), imgp->ip_vfs_context);

			proc_transend(p, 0);
			goto again;

		default:
			break;
		}
	}

	if (error == 0) {
		if (imgp->ip_flags & IMGPF_INTERPRET && ndp->ni_vp) {
			AUDIT_ARG(vnpath, ndp->ni_vp, ARG_VNODE2);
		}

		  
                                                      
                                                  
     
		if (kauth_authorize_fileop_has_listeners()) {
			kauth_authorize_fileop(vfs_context_ucred(imgp->ip_vfs_context),
			    KAUTH_FILEOP_EXEC,
			    (uintptr_t)ndp->ni_vp, 0);
		}
	}
bad:
	proc_transend(p, 0);

bad_notrans:
	if (imgp->ip_strings) {
		execargs_free(imgp);
	}
	if (imgp->ip_ndp) {
		nameidone(imgp->ip_ndp);
	}
	kheap_free(KHEAP_TEMP, ndp, sizeof(*ndp));

	return error;
}

  
                                 
  
                                                                       
  
                                                                     
  
                                               
                                               
   
static errno_t
exec_validate_spawnattr_policy(int psa_apptype)
{
	if ((psa_apptype & POSIX_SPAWN_PROC_TYPE_MASK) != 0) {
		int proctype = psa_apptype & POSIX_SPAWN_PROC_TYPE_MASK;
		if (proctype == POSIX_SPAWN_PROC_TYPE_DRIVER) {
			if (!IOTaskHasEntitlement(current_task(), POSIX_SPAWN_ENTITLEMENT_DRIVER)) {
				return EPERM;
			}
		}
	}

	return 0;
}

  
                               
  
                                                                                                     
  
                                                                      
                                                                     
  
                                               
   
static errno_t
exec_handle_spawnattr_policy(proc_t p, thread_t thread, int psa_apptype, uint64_t psa_qos_clamp,
    task_role_t psa_darwin_role, struct exec_port_actions *port_actions)
{
	int apptype     = TASK_APPTYPE_NONE;
	int qos_clamp   = THREAD_QOS_UNSPECIFIED;
	task_role_t role = TASK_UNSPECIFIED;

	if ((psa_apptype & POSIX_SPAWN_PROC_TYPE_MASK) != 0) {
		int proctype = psa_apptype & POSIX_SPAWN_PROC_TYPE_MASK;

		switch (proctype) {
		case POSIX_SPAWN_PROC_TYPE_DAEMON_INTERACTIVE:
			apptype = TASK_APPTYPE_DAEMON_INTERACTIVE;
			break;
		case POSIX_SPAWN_PROC_TYPE_DAEMON_STANDARD:
			apptype = TASK_APPTYPE_DAEMON_STANDARD;
			break;
		case POSIX_SPAWN_PROC_TYPE_DAEMON_ADAPTIVE:
			apptype = TASK_APPTYPE_DAEMON_ADAPTIVE;
			break;
		case POSIX_SPAWN_PROC_TYPE_DAEMON_BACKGROUND:
			apptype = TASK_APPTYPE_DAEMON_BACKGROUND;
			break;
		case POSIX_SPAWN_PROC_TYPE_APP_DEFAULT:
			apptype = TASK_APPTYPE_APP_DEFAULT;
			break;
		case POSIX_SPAWN_PROC_TYPE_DRIVER:
			apptype = TASK_APPTYPE_DRIVER;
			break;
		default:
			apptype = TASK_APPTYPE_NONE;
			                                                        
			break;
		}
	}

	if (psa_qos_clamp != POSIX_SPAWN_PROC_CLAMP_NONE) {
		switch (psa_qos_clamp) {
		case POSIX_SPAWN_PROC_CLAMP_UTILITY:
			qos_clamp = THREAD_QOS_UTILITY;
			break;
		case POSIX_SPAWN_PROC_CLAMP_BACKGROUND:
			qos_clamp = THREAD_QOS_BACKGROUND;
			break;
		case POSIX_SPAWN_PROC_CLAMP_MAINTENANCE:
			qos_clamp = THREAD_QOS_MAINTENANCE;
			break;
		default:
			qos_clamp = THREAD_QOS_UNSPECIFIED;
			                                                        
			break;
		}
	}

	if (psa_darwin_role != PRIO_DARWIN_ROLE_DEFAULT) {
		proc_darwin_role_to_task_role(psa_darwin_role, &role);
	}

	if (apptype != TASK_APPTYPE_NONE ||
	    qos_clamp != THREAD_QOS_UNSPECIFIED ||
	    role != TASK_UNSPECIFIED ||
	    port_actions->portwatch_count) {
		proc_set_task_spawnpolicy(p->task, thread, apptype, qos_clamp, role,
		    port_actions->portwatch_array, port_actions->portwatch_count);
	}

	if (port_actions->registered_count) {
		if (mach_ports_register(p->task, port_actions->registered_array,
		    port_actions->registered_count)) {
			return EINVAL;
		}
		                                              
		port_actions->registered_array = NULL;
		port_actions->registered_count = 0;
	}

	return 0;
}

static void
exec_port_actions_destroy(struct exec_port_actions *port_actions)
{
	if (port_actions->portwatch_array) {
		for (uint32_t i = 0; i < port_actions->portwatch_count; i++) {
			ipc_port_t port = NULL;
			if ((port = port_actions->portwatch_array[i]) != NULL) {
				ipc_port_release_send(port);
			}
		}
		kfree(port_actions->portwatch_array,
		    port_actions->portwatch_count * sizeof(ipc_port_t *));
	}

	if (port_actions->registered_array) {
		for (uint32_t i = 0; i < port_actions->registered_count; i++) {
			ipc_port_t port = NULL;
			if ((port = port_actions->registered_array[i]) != NULL) {
				ipc_port_release_send(port);
			}
		}
		kfree(port_actions->registered_array,
		    port_actions->registered_count * sizeof(ipc_port_t *));
	}
}

  
                           
  
                                                              
                                                                       
                                                                    
  
                                                          
  
                       
                                
                                                               
   
static errno_t
exec_handle_port_actions(struct image_params *imgp,
    struct exec_port_actions *actions)
{
	_posix_spawn_port_actions_t pacts = imgp->ip_px_spa;
#if CONFIG_AUDIT
	proc_t p = vfs_context_proc(imgp->ip_vfs_context);
#endif
	_ps_port_action_t *act = NULL;
	task_t task = get_threadtask(imgp->ip_new_thread);
	ipc_port_t port = NULL;
	errno_t ret = 0;
	int i, portwatch_i = 0, registered_i = 0;
	kern_return_t kr;
	boolean_t task_has_watchport_boost = task_has_watchports(current_task());
	boolean_t in_exec = (imgp->ip_flags & IMGPF_EXEC);
	int ptrauth_task_port_count = 0;
	boolean_t suid_cred_specified = FALSE;

	for (i = 0; i < pacts->pspa_count; i++) {
		act = &pacts->pspa_actions[i];

		switch (act->port_type) {
		case PSPA_SPECIAL:
		case PSPA_EXCEPTION:
#if CONFIG_AUDIT
		case PSPA_AU_SESSION:
#endif
			break;
		case PSPA_IMP_WATCHPORTS:
			if (++actions->portwatch_count > TASK_MAX_WATCHPORT_COUNT) {
				ret = EINVAL;
				goto done;
			}
			break;
		case PSPA_REGISTERED_PORTS:
			if (++actions->registered_count > TASK_PORT_REGISTER_MAX) {
				ret = EINVAL;
				goto done;
			}
			break;

		case PSPA_PTRAUTH_TASK_PORT:
			if (++ptrauth_task_port_count > 1) {
				ret = EINVAL;
				goto done;
			}
			break;

		case PSPA_SUID_CRED:
			                                                     
			if (suid_cred_specified) {
				ret = EINVAL;
				goto done;
			}
			suid_cred_specified = TRUE;
			break;

		default:
			ret = EINVAL;
			goto done;
		}
	}

	if (actions->portwatch_count) {
		if (in_exec && task_has_watchport_boost) {
			ret = EINVAL;
			goto done;
		}
		actions->portwatch_array =
		    kalloc(sizeof(ipc_port_t *) * actions->portwatch_count);
		if (actions->portwatch_array == NULL) {
			ret = ENOMEM;
			goto done;
		}
		bzero(actions->portwatch_array,
		    sizeof(ipc_port_t *) * actions->portwatch_count);
	}

	if (actions->registered_count) {
		actions->registered_array =
		    kalloc(sizeof(ipc_port_t *) * actions->registered_count);
		if (actions->registered_array == NULL) {
			ret = ENOMEM;
			goto done;
		}
		bzero(actions->registered_array,
		    sizeof(ipc_port_t *) * actions->registered_count);
	}

	for (i = 0; i < pacts->pspa_count; i++) {
		act = &pacts->pspa_actions[i];

		if (MACH_PORT_VALID(act->new_port)) {
			kr = ipc_object_copyin(get_task_ipcspace(current_task()),
			    act->new_port, MACH_MSG_TYPE_COPY_SEND,
			    (ipc_object_t *) &port, 0, NULL, IPC_OBJECT_COPYIN_FLAGS_ALLOW_IMMOVABLE_SEND);

			if (kr != KERN_SUCCESS) {
				ret = EINVAL;
				goto done;
			}
		} else {
			                       
			port = CAST_MACH_NAME_TO_PORT(act->new_port);
		}

		switch (act->port_type) {
		case PSPA_SPECIAL:
			kr = task_set_special_port(task, act->which, port);

			if (kr != KERN_SUCCESS) {
				ret = EINVAL;
			}
			break;

		case PSPA_EXCEPTION:
			kr = task_set_exception_ports(task, act->mask, port,
			    act->behavior, act->flavor);
			if (kr != KERN_SUCCESS) {
				ret = EINVAL;
			}
			break;
#if CONFIG_AUDIT
		case PSPA_AU_SESSION:
			ret = audit_session_spawnjoin(p, task, port);
			if (ret) {
				                                                                                   
				goto done;
			}

			break;
#endif
		case PSPA_IMP_WATCHPORTS:
			if (actions->portwatch_array) {
				                                       
				actions->portwatch_array[portwatch_i++] = port;
			} else {
				ipc_port_release_send(port);
			}
			break;
		case PSPA_REGISTERED_PORTS:
			                                       
			actions->registered_array[registered_i++] = port;
			break;

		case PSPA_PTRAUTH_TASK_PORT:
#if defined(HAS_APPLE_PAC)
			{
				task_t ptr_auth_task = convert_port_to_task(port);

				if (ptr_auth_task == TASK_NULL) {
					ret = EINVAL;
					break;
				}

				imgp->ip_inherited_shared_region_id =
				    task_get_vm_shared_region_id_and_jop_pid(ptr_auth_task,
				    &imgp->ip_inherited_jop_pid);

				                                                          
				task_deallocate(ptr_auth_task);
			}
#endif                    

			                                               
			ipc_port_release_send(port);
			break;

		case PSPA_SUID_CRED:
			imgp->ip_sc_port = port;
			break;

		default:
			ret = EINVAL;
			break;
		}

		if (ret) {
			                                              
			ipc_port_release_send(port);
			break;
		}
	}

done:
	if (0 != ret) {
		DTRACE_PROC1(spawn__port__failure, mach_port_name_t, act->new_port);
	}
	return ret;
}

  
                           
  
                                                                          
                                                               
                        
  
                                                          
  
                       
       
  
                                                                        
                                                             
                                                                
                                                               
                                  
   
static int
exec_handle_file_actions(struct image_params *imgp, short psa_flags)
{
	int error = 0;
	int action;
	proc_t p = vfs_context_proc(imgp->ip_vfs_context);
	_posix_spawn_file_actions_t px_sfap = imgp->ip_px_sfa;
	int ival[2];                                                
#if CONFIG_AUDIT
	struct uthread *uthread = get_bsdthread_info(current_thread());
#endif

	for (action = 0; action < px_sfap->psfa_act_count; action++) {
		_psfa_action_t *psfa = &px_sfap->psfa_act_acts[action];

		switch (psfa->psfaa_type) {
		case PSFA_OPEN: {
			  
                                                       
                                                       
                                                        
                                                         
                                                    
                                
      
			char *bufp = NULL;
			struct vnode_attr *vap;
			struct nameidata *ndp;
			int mode = psfa->psfaa_openargs.psfao_mode;
			int origfd;

			bufp = kheap_alloc(KHEAP_TEMP,
			    sizeof(*vap) + sizeof(*ndp), Z_WAITOK | Z_ZERO);
			if (bufp == NULL) {
				error = ENOMEM;
				break;
			}

			vap = (struct vnode_attr *) bufp;
			ndp = (struct nameidata *) (bufp + sizeof(*vap));

			VATTR_INIT(vap);
			                                                 
			mode = ((mode & ~p->p_fd->fd_cmask) & ALLPERMS) & ~S_ISTXT;
			VATTR_SET(vap, va_mode, mode & ACCESSPERMS);

			AUDIT_SUBCALL_ENTER(OPEN, p, uthread);

			NDINIT(ndp, LOOKUP, OP_OPEN, FOLLOW | AUDITVNPATH1, UIO_SYSSPACE,
			    CAST_USER_ADDR_T(psfa->psfaa_openargs.psfao_path),
			    imgp->ip_vfs_context);

			error = open1(imgp->ip_vfs_context,
			    ndp,
			    psfa->psfaa_openargs.psfao_oflag,
			    vap,
			    fileproc_alloc_init, NULL,
			    &origfd);

			kheap_free(KHEAP_TEMP, bufp, sizeof(*vap) + sizeof(*ndp));

			AUDIT_SUBCALL_EXIT(uthread, error);

			  
                                                    
                                                        
                                                   
                                                      
      
			if (error || origfd == psfa->psfaa_filedes) {
				break;
			}

			  
                                                      
                                                        
                          
      
			AUDIT_SUBCALL_ENTER(DUP2, p, uthread);
			error = dup2(p, origfd, psfa->psfaa_filedes, ival);
			AUDIT_SUBCALL_EXIT(uthread, error);
			if (error) {
				break;
			}

			  
                                     
      
			AUDIT_SUBCALL_ENTER(CLOSE, p, uthread);
			error = close_nocancel(p, origfd);
			AUDIT_SUBCALL_EXIT(uthread, error);
		}
		break;

		case PSFA_DUP2: {
			AUDIT_SUBCALL_ENTER(DUP2, p, uthread);
			error = dup2(p, psfa->psfaa_filedes,
			    psfa->psfaa_dup2args.psfad_newfiledes, ival);
			AUDIT_SUBCALL_EXIT(uthread, error);
		}
		break;

		case PSFA_FILEPORT_DUP2: {
			ipc_port_t port;
			kern_return_t kr;
			int origfd;

			if (!MACH_PORT_VALID(psfa->psfaa_fileport)) {
				error = EINVAL;
				break;
			}

			kr = ipc_object_copyin(get_task_ipcspace(current_task()),
			    psfa->psfaa_fileport, MACH_MSG_TYPE_COPY_SEND,
			    (ipc_object_t *) &port, 0, NULL, IPC_OBJECT_COPYIN_FLAGS_ALLOW_IMMOVABLE_SEND);

			if (kr != KERN_SUCCESS) {
				error = EINVAL;
				break;
			}

			error = fileport_makefd(p, port, 0, &origfd);

			if (IPC_PORT_NULL != port) {
				ipc_port_release_send(port);
			}

			if (error || origfd == psfa->psfaa_dup2args.psfad_newfiledes) {
				break;
			}

			AUDIT_SUBCALL_ENTER(DUP2, p, uthread);
			error = dup2(p, origfd,
			    psfa->psfaa_dup2args.psfad_newfiledes, ival);
			AUDIT_SUBCALL_EXIT(uthread, error);
			if (error) {
				break;
			}

			AUDIT_SUBCALL_ENTER(CLOSE, p, uthread);
			error = close_nocancel(p, origfd);
			AUDIT_SUBCALL_EXIT(uthread, error);
		}
		break;

		case PSFA_CLOSE: {
			AUDIT_SUBCALL_ENTER(CLOSE, p, uthread);
			error = close_nocancel(p, psfa->psfaa_filedes);
			AUDIT_SUBCALL_EXIT(uthread, error);
		}
		break;

		case PSFA_INHERIT: {
			struct fileproc *fp;

			  
                                                
                                                
     
                                               
                        
      

			proc_fdlock(p);
			if ((fp = fp_get_noref_locked(p, psfa->psfaa_filedes)) == NULL) {
				error = EBADF;
			} else if (fp_isguarded(fp, 0)) {
				error = fp_guard_exception(p, psfa->psfaa_filedes,
				    fp, kGUARD_EXC_NOCLOEXEC);
			} else {
				p->p_fd->fd_ofileflags[psfa->psfaa_filedes] &= ~UF_EXCLOSE;
				error = 0;
			}
			proc_fdunlock(p);
		}
		break;

		case PSFA_CHDIR: {
			  
                                                        
                                                       
                                                       
                                                          
                                                    
                                
      
			struct nameidata *nd;
			nd = kheap_alloc(KHEAP_TEMP, sizeof(*nd), Z_WAITOK | Z_ZERO);
			if (nd == NULL) {
				error = ENOMEM;
				break;
			}

			AUDIT_SUBCALL_ENTER(CHDIR, p, uthread);
			NDINIT(nd, LOOKUP, OP_CHDIR, FOLLOW | AUDITVNPATH1, UIO_SYSSPACE,
			    CAST_USER_ADDR_T(psfa->psfaa_chdirargs.psfac_path),
			    imgp->ip_vfs_context);

			error = chdir_internal(p, imgp->ip_vfs_context, nd, 0);
			kheap_free(KHEAP_TEMP, nd, sizeof(*nd));
			AUDIT_SUBCALL_EXIT(uthread, error);
		}
		break;

		case PSFA_FCHDIR: {
			struct fchdir_args fchdira;

			fchdira.fd = psfa->psfaa_filedes;

			AUDIT_SUBCALL_ENTER(FCHDIR, p, uthread);
			error = fchdir(p, &fchdira, ival);
			AUDIT_SUBCALL_EXIT(uthread, error);
		}
		break;

		default:
			error = EINVAL;
			break;
		}

		                                                               

		if (error) {
			if (PSFA_OPEN == psfa->psfaa_type) {
				DTRACE_PROC1(spawn__open__failure, uintptr_t,
				    psfa->psfaa_openargs.psfao_path);
			} else {
				DTRACE_PROC1(spawn__fd__failure, int, psfa->psfaa_filedes);
			}
			break;
		}
	}

	if (error != 0 || (psa_flags & POSIX_SPAWN_CLOEXEC_DEFAULT) == 0) {
		return error;
	}

	  
                                                         
                                                         
                                                                    
                                                              
                                                              
   
                                                     
    
	proc_fdlock(p);
	for (action = 0; action < px_sfap->psfa_act_count; action++) {
		_psfa_action_t *psfa = &px_sfap->psfa_act_acts[action];
		int fd = psfa->psfaa_filedes;

		switch (psfa->psfaa_type) {
		case PSFA_DUP2:
		case PSFA_FILEPORT_DUP2:
			fd = psfa->psfaa_dup2args.psfad_newfiledes;
			OS_FALLTHROUGH;
		case PSFA_OPEN:
		case PSFA_INHERIT:
			*fdflags(p, fd) |= UF_INHERIT;
			break;

		case PSFA_CLOSE:
		case PSFA_CHDIR:
		case PSFA_FCHDIR:
			  
                                                                 
                                                              
                                                                  
                                                                  
                 
      
			break;
		}
	}
	proc_fdunlock(p);

	return 0;
}

#if CONFIG_MACF
  
                                  
   
void *
exec_spawnattr_getmacpolicyinfo(const void *macextensions, const char *policyname, size_t *lenp)
{
	const struct _posix_spawn_mac_policy_extensions *psmx = macextensions;
	int i;

	if (psmx == NULL) {
		return NULL;
	}

	for (i = 0; i < psmx->psmx_count; i++) {
		const _ps_mac_policy_extension_t *extension = &psmx->psmx_extensions[i];
		if (strncmp(extension->policyname, policyname, sizeof(extension->policyname)) == 0) {
			if (lenp != NULL) {
				*lenp = (size_t)extension->datalen;
			}
			return extension->datap;
		}
	}

	if (lenp != NULL) {
		*lenp = 0;
	}
	return NULL;
}

static void
spawn_free_macpolicyinfo(const struct user__posix_spawn_args_desc *px_args,
    _posix_spawn_mac_policy_extensions_t psmx, int count)
{
	if (psmx == NULL) {
		return;
	}
	for (int i = 0; i < count; i++) {
		_ps_mac_policy_extension_t *ext = &psmx->psmx_extensions[i];
		kheap_free(KHEAP_TEMP, ext->datap, (vm_size_t) ext->datalen);
	}
	kheap_free(KHEAP_TEMP, psmx, px_args->mac_extensions_size);
}

static int
spawn_copyin_macpolicyinfo(const struct user__posix_spawn_args_desc *px_args,
    _posix_spawn_mac_policy_extensions_t *psmxp)
{
	_posix_spawn_mac_policy_extensions_t psmx = NULL;
	int error = 0;
	int copycnt = 0;

	*psmxp = NULL;

	if (px_args->mac_extensions_size < PS_MAC_EXTENSIONS_SIZE(1) ||
	    px_args->mac_extensions_size > PAGE_SIZE) {
		error = EINVAL;
		goto bad;
	}

	psmx = kheap_alloc(KHEAP_TEMP, px_args->mac_extensions_size, Z_WAITOK);
	if (psmx == NULL) {
		error = ENOMEM;
		goto bad;
	}

	error = copyin(px_args->mac_extensions, psmx, px_args->mac_extensions_size);
	if (error) {
		goto bad;
	}

	size_t extsize = PS_MAC_EXTENSIONS_SIZE(psmx->psmx_count);
	if (extsize == 0 || extsize > px_args->mac_extensions_size) {
		error = EINVAL;
		goto bad;
	}

	for (int i = 0; i < psmx->psmx_count; i++) {
		_ps_mac_policy_extension_t *extension = &psmx->psmx_extensions[i];
		if (extension->datalen == 0 || extension->datalen > PAGE_SIZE) {
			error = EINVAL;
			goto bad;
		}
	}

	for (copycnt = 0; copycnt < psmx->psmx_count; copycnt++) {
		_ps_mac_policy_extension_t *extension = &psmx->psmx_extensions[copycnt];
		void *data = NULL;

#if !__LP64__
		if (extension->data > UINT32_MAX) {
			goto bad;
		}
#endif
		data = kheap_alloc(KHEAP_TEMP, (vm_size_t) extension->datalen, Z_WAITOK);
		if (data == NULL) {
			error = ENOMEM;
			goto bad;
		}
		error = copyin((user_addr_t)extension->data, data, (size_t)extension->datalen);
		if (error) {
			kheap_free(KHEAP_TEMP, data, (vm_size_t) extension->datalen);
			error = ENOMEM;
			goto bad;
		}
		extension->datap = data;
	}

	*psmxp = psmx;
	return 0;

bad:
	spawn_free_macpolicyinfo(px_args, psmx, copycnt);
	return error;
}
#endif                  

#if CONFIG_COALITIONS
static inline void
spawn_coalitions_release_all(coalition_t coal[COALITION_NUM_TYPES])
{
	for (int c = 0; c < COALITION_NUM_TYPES; c++) {
		if (coal[c]) {
			coalition_remove_active(coal[c]);
			coalition_release(coal[c]);
		}
	}
}
#endif

#if CONFIG_PERSONAS
static int
spawn_validate_persona(struct _posix_spawn_persona_info *px_persona)
{
	int error = 0;
	struct persona *persona = NULL;
	int verify = px_persona->pspi_flags & POSIX_SPAWN_PERSONA_FLAGS_VERIFY;

	if (!IOTaskHasEntitlement(current_task(), PERSONA_MGMT_ENTITLEMENT)) {
		return EPERM;
	}

	if (px_persona->pspi_flags & POSIX_SPAWN_PERSONA_GROUPS) {
		if (px_persona->pspi_ngroups > NGROUPS_MAX) {
			return EINVAL;
		}
	}

	persona = persona_lookup(px_persona->pspi_id);
	if (!persona) {
		error = ESRCH;
		goto out;
	}

	if (verify) {
		if (px_persona->pspi_flags & POSIX_SPAWN_PERSONA_UID) {
			if (px_persona->pspi_uid != persona_get_uid(persona)) {
				error = EINVAL;
				goto out;
			}
		}
		if (px_persona->pspi_flags & POSIX_SPAWN_PERSONA_GID) {
			if (px_persona->pspi_gid != persona_get_gid(persona)) {
				error = EINVAL;
				goto out;
			}
		}
		if (px_persona->pspi_flags & POSIX_SPAWN_PERSONA_GROUPS) {
			size_t ngroups = 0;
			gid_t groups[NGROUPS_MAX];

			if (persona_get_groups(persona, &ngroups, groups,
			    px_persona->pspi_ngroups) != 0) {
				error = EINVAL;
				goto out;
			}
			if (ngroups != px_persona->pspi_ngroups) {
				error = EINVAL;
				goto out;
			}
			while (ngroups--) {
				if (px_persona->pspi_groups[ngroups] != groups[ngroups]) {
					error = EINVAL;
					goto out;
				}
			}
			if (px_persona->pspi_gmuid != persona_get_gmuid(persona)) {
				error = EINVAL;
				goto out;
			}
		}
	}

out:
	if (persona) {
		persona_put(persona);
	}

	return error;
}

static int
spawn_persona_adopt(proc_t p, struct _posix_spawn_persona_info *px_persona)
{
	int ret;
	kauth_cred_t cred;
	struct persona *persona = NULL;
	int override = !!(px_persona->pspi_flags & POSIX_SPAWN_PERSONA_FLAGS_OVERRIDE);

	if (!override) {
		return persona_proc_adopt_id(p, px_persona->pspi_id, NULL);
	}

	  
                                                                    
                                            
    
	persona = persona_lookup(px_persona->pspi_id);
	if (!persona) {
		return ESRCH;
	}

	cred = persona_get_cred(persona);
	if (!cred) {
		ret = EINVAL;
		goto out;
	}

	if (px_persona->pspi_flags & POSIX_SPAWN_PERSONA_UID) {
		cred = kauth_cred_setresuid(cred,
		    px_persona->pspi_uid,
		    px_persona->pspi_uid,
		    px_persona->pspi_uid,
		    KAUTH_UID_NONE);
	}

	if (px_persona->pspi_flags & POSIX_SPAWN_PERSONA_GID) {
		cred = kauth_cred_setresgid(cred,
		    px_persona->pspi_gid,
		    px_persona->pspi_gid,
		    px_persona->pspi_gid);
	}

	if (px_persona->pspi_flags & POSIX_SPAWN_PERSONA_GROUPS) {
		cred = kauth_cred_setgroups(cred,
		    px_persona->pspi_groups,
		    px_persona->pspi_ngroups,
		    px_persona->pspi_gmuid);
	}

	ret = persona_proc_adopt(p, persona, cred);

out:
	persona_put(persona);
	return ret;
}
#endif

#if __arm64__
extern int legacy_footprint_entitlement_mode;
static inline void
proc_legacy_footprint_entitled(proc_t p, task_t task)
{
#pragma unused(p)
	boolean_t legacy_footprint_entitled;

	switch (legacy_footprint_entitlement_mode) {
	case LEGACY_FOOTPRINT_ENTITLEMENT_IGNORE:
		                                
		break;
	case LEGACY_FOOTPRINT_ENTITLEMENT_IOS11_ACCT:
		                                                    
		legacy_footprint_entitled = IOTaskHasEntitlement(task,
		    "com.apple.private.memory.legacy_footprint");
		if (legacy_footprint_entitled) {
			task_set_legacy_footprint(task);
		}
		break;
	case LEGACY_FOOTPRINT_ENTITLEMENT_LIMIT_INCREASE:
		                                                       
		legacy_footprint_entitled = IOTaskHasEntitlement(task,
		    "com.apple.private.memory.legacy_footprint");
		if (legacy_footprint_entitled) {
			task_set_extra_footprint_limit(task);
		}
		break;
	default:
		break;
	}
}

static inline void
proc_ios13extended_footprint_entitled(proc_t p, task_t task)
{
#pragma unused(p)
	boolean_t ios13extended_footprint_entitled;

	                                                       
	ios13extended_footprint_entitled = IOTaskHasEntitlement(task,
	    "com.apple.developer.memory.ios13extended_footprint");
	if (ios13extended_footprint_entitled) {
		task_set_ios13extended_footprint_limit(task);
	}
}
static inline void
proc_increased_memory_limit_entitled(proc_t p, task_t task)
{
	static const char kIncreasedMemoryLimitEntitlement[] = "com.apple.developer.kernel.increased-memory-limit";
	bool entitled = false;

	entitled = IOTaskHasEntitlement(task, kIncreasedMemoryLimitEntitlement);
	if (entitled) {
		memorystatus_act_on_entitled_task_limit(p);
	}
}

  
                                                                 
                                                               
   
static inline void
proc_footprint_entitlement_hacks(proc_t p, task_t task)
{
	proc_legacy_footprint_entitled(p, task);
	proc_ios13extended_footprint_entitled(p, task);
	proc_increased_memory_limit_entitled(p, task);
}
#endif                

#if CONFIG_MACF
  
                                                                       
   
static inline void
proc_apply_jit_and_jumbo_va_policies(proc_t p, task_t task)
{
	bool jit_entitled;
	jit_entitled = (mac_proc_check_map_anon(p, 0, 0, 0, MAP_JIT, NULL) == 0);
	if (jit_entitled || (IOTaskHasEntitlement(task,
	    "com.apple.developer.kernel.extended-virtual-addressing"))) {
		vm_map_set_jumbo(get_task_map(task));
		if (jit_entitled) {
			vm_map_set_jit_entitled(get_task_map(task));

		}
	}
}
#endif                  

  
                                                                    
  
                                                             
   
static void
apply_kauth_cred_update(proc_t p,
    kauth_cred_t (^update)(kauth_cred_t orig_cred))
{
	kauth_cred_t my_cred, my_new_cred;

	my_cred = kauth_cred_proc_ref(p);
	for (;;) {
		my_new_cred = update(my_cred);
		if (my_cred == my_new_cred) {
			kauth_cred_unref(&my_new_cred);
			break;
		}

		                             
		proc_ucred_lock(p);

		if (p->p_ucred == my_cred) {
			                                                
			p->p_ucred = my_new_cred;
			PROC_UPDATE_CREDS_ONPROC(p);
			proc_ucred_unlock(p);

			                               
			kauth_cred_unref(&my_cred);
			break;
		}

		                                 
		my_cred = p->p_ucred;
		kauth_cred_ref(my_cred);
		proc_ucred_unlock(p);

		kauth_cred_unref(&my_new_cred);
	}
}

static int
spawn_posix_cred_adopt(proc_t p,
    struct _posix_spawn_posix_cred_info *px_pcred_info)
{
	int error = 0;

	if (px_pcred_info->pspci_flags & POSIX_SPAWN_POSIX_CRED_GID) {
		struct setgid_args args = {
			.gid = px_pcred_info->pspci_gid,
		};
		error = setgid(p, &args, NULL);
		if (error) {
			return error;
		}
	}

	if (px_pcred_info->pspci_flags & POSIX_SPAWN_POSIX_CRED_GROUPS) {
		error = setgroups_internal(p,
		    px_pcred_info->pspci_ngroups,
		    px_pcred_info->pspci_groups,
		    px_pcred_info->pspci_gmuid);
		if (error) {
			return error;
		}
	}

	if (px_pcred_info->pspci_flags & POSIX_SPAWN_POSIX_CRED_UID) {
		struct setuid_args args = {
			.uid = px_pcred_info->pspci_uid,
		};
		error = setuid(p, &args, NULL);
		if (error) {
			return error;
		}
	}
	return 0;
}

  
              
  
                                                   
                                 
                            
                               
  
                       
                             
                           
                                          
                                              
                                               
                                                 
                                              
                                         
                                                     
                                                           
                                                                    
                                                    
                                                                 
                          
                       
  
                                                            
                            
                                     
   
int
posix_spawn(proc_t ap, struct posix_spawn_args *uap, int32_t *retval)
{
	proc_t p = ap;                                               
	user_addr_t pid = uap->pid;
	int ival[2];                                            
	char *bufp = NULL;
	char *subsystem_root_path = NULL;
	struct image_params *imgp;
	struct vnode_attr *vap;
	struct vnode_attr *origvap;
	struct uthread  *uthread = 0;                                          
	int error, sig;
	int is_64 = IS_64BIT_PROCESS(p);
	struct vfs_context context;
	struct user__posix_spawn_args_desc px_args;
	struct _posix_spawnattr px_sa;
	_posix_spawn_file_actions_t px_sfap = NULL;
	_posix_spawn_port_actions_t px_spap = NULL;
	struct __kern_sigaction vec;
	boolean_t spawn_no_exec = FALSE;
	boolean_t proc_transit_set = TRUE;
	boolean_t exec_done = FALSE;
	struct exec_port_actions port_actions = { };
	vm_size_t px_sa_offset = offsetof(struct _posix_spawnattr, psa_ports);
	task_t old_task = current_task();
	task_t new_task = NULL;
	boolean_t should_release_proc_ref = FALSE;
	void *inherit = NULL;
#if CONFIG_PERSONAS
	struct _posix_spawn_persona_info *px_persona = NULL;
#endif
	struct _posix_spawn_posix_cred_info *px_pcred_info = NULL;

	  
                                                                      
                              
    
	bufp = kheap_alloc(KHEAP_TEMP,
	    sizeof(*imgp) + sizeof(*vap) + sizeof(*origvap), Z_WAITOK | Z_ZERO);
	imgp = (struct image_params *) bufp;
	if (bufp == NULL) {
		error = ENOMEM;
		goto bad;
	}
	vap = (struct vnode_attr *) (bufp + sizeof(*imgp));
	origvap = (struct vnode_attr *) (bufp + sizeof(*imgp) + sizeof(*vap));

	                                                              
	imgp->ip_user_fname = uap->path;
	imgp->ip_user_argv = uap->argv;
	imgp->ip_user_envv = uap->envp;
	imgp->ip_vattr = vap;
	imgp->ip_origvattr = origvap;
	imgp->ip_vfs_context = &context;
	imgp->ip_flags = (is_64 ? IMGPF_WAS_64BIT_ADDR : IMGPF_NONE);
	imgp->ip_seg = (is_64 ? UIO_USERSPACE64 : UIO_USERSPACE32);
	imgp->ip_mac_return = 0;
	imgp->ip_px_persona = NULL;
	imgp->ip_px_pcred_info = NULL;
	imgp->ip_cs_error = OS_REASON_NULL;
	imgp->ip_simulator_binary = IMGPF_SB_DEFAULT;
	imgp->ip_subsystem_root_path = NULL;
	imgp->ip_inherited_shared_region_id = NULL;
	imgp->ip_inherited_jop_pid = 0;

	if (uap->adesc != USER_ADDR_NULL) {
		if (is_64) {
			error = copyin(uap->adesc, &px_args, sizeof(px_args));
		} else {
			struct user32__posix_spawn_args_desc px_args32;

			error = copyin(uap->adesc, &px_args32, sizeof(px_args32));

			  
                                                       
                                                      
      
			px_args.attr_size = px_args32.attr_size;
			px_args.attrp = CAST_USER_ADDR_T(px_args32.attrp);
			px_args.file_actions_size = px_args32.file_actions_size;
			px_args.file_actions = CAST_USER_ADDR_T(px_args32.file_actions);
			px_args.port_actions_size = px_args32.port_actions_size;
			px_args.port_actions = CAST_USER_ADDR_T(px_args32.port_actions);
			px_args.mac_extensions_size = px_args32.mac_extensions_size;
			px_args.mac_extensions = CAST_USER_ADDR_T(px_args32.mac_extensions);
			px_args.coal_info_size = px_args32.coal_info_size;
			px_args.coal_info = CAST_USER_ADDR_T(px_args32.coal_info);
			px_args.persona_info_size = px_args32.persona_info_size;
			px_args.persona_info = CAST_USER_ADDR_T(px_args32.persona_info);
			px_args.posix_cred_info_size = px_args32.posix_cred_info_size;
			px_args.posix_cred_info = CAST_USER_ADDR_T(px_args32.posix_cred_info);
			px_args.subsystem_root_path_size = px_args32.subsystem_root_path_size;
			px_args.subsystem_root_path = CAST_USER_ADDR_T(px_args32.subsystem_root_path);
		}
		if (error) {
			goto bad;
		}

		if (px_args.attr_size != 0) {
			  
                                                  
                                              
                                                      
      

			if ((error = copyin(px_args.attrp, &px_sa, px_sa_offset)) != 0) {
				goto bad;
			}

			bzero((void *)((unsigned long) &px_sa + px_sa_offset), sizeof(px_sa) - px_sa_offset );

			imgp->ip_px_sa = &px_sa;
		}
		if (px_args.file_actions_size != 0) {
			                                                        
			rlim_t maxfa = (p->p_limit ? MIN(proc_limitgetcur(p, RLIMIT_NOFILE, TRUE), maxfilesperproc) : NOFILE);
			size_t maxfa_size = PSF_ACTIONS_SIZE(maxfa);
			if (px_args.file_actions_size < PSF_ACTIONS_SIZE(1) ||
			    maxfa_size == 0 || px_args.file_actions_size > maxfa_size) {
				error = EINVAL;
				goto bad;
			}

			px_sfap = kheap_alloc(KHEAP_TEMP,
			    px_args.file_actions_size, Z_WAITOK);
			if (px_sfap == NULL) {
				error = ENOMEM;
				goto bad;
			}
			imgp->ip_px_sfa = px_sfap;

			if ((error = copyin(px_args.file_actions, px_sfap,
			    px_args.file_actions_size)) != 0) {
				goto bad;
			}

			                                                          
			size_t psfsize = PSF_ACTIONS_SIZE(px_sfap->psfa_act_count);
			if (psfsize == 0 || psfsize != px_args.file_actions_size) {
				error = EINVAL;
				goto bad;
			}
		}
		if (px_args.port_actions_size != 0) {
			                                            
			if (px_args.port_actions_size < PS_PORT_ACTIONS_SIZE(1) ||
			    px_args.port_actions_size > PAGE_SIZE) {
				error = EINVAL;
				goto bad;
			}

			px_spap = kheap_alloc(KHEAP_TEMP,
			    px_args.port_actions_size, Z_WAITOK);
			if (px_spap == NULL) {
				error = ENOMEM;
				goto bad;
			}
			imgp->ip_px_spa = px_spap;

			if ((error = copyin(px_args.port_actions, px_spap,
			    px_args.port_actions_size)) != 0) {
				goto bad;
			}

			                                                          
			size_t pasize = PS_PORT_ACTIONS_SIZE(px_spap->pspa_count);
			if (pasize == 0 || pasize != px_args.port_actions_size) {
				error = EINVAL;
				goto bad;
			}
		}
#if CONFIG_PERSONAS
		                              
		if (px_args.persona_info_size != 0 && px_args.persona_info != 0) {
			                                                          
			if (px_args.persona_info_size != sizeof(*px_persona)) {
				error = ERANGE;
				goto bad;
			}

			px_persona = kheap_alloc(KHEAP_TEMP,
			    px_args.persona_info_size, Z_WAITOK);
			if (px_persona == NULL) {
				error = ENOMEM;
				goto bad;
			}
			imgp->ip_px_persona = px_persona;

			if ((error = copyin(px_args.persona_info, px_persona,
			    px_args.persona_info_size)) != 0) {
				goto bad;
			}
			if ((error = spawn_validate_persona(px_persona)) != 0) {
				goto bad;
			}
		}
#endif
		                                 
		if (px_args.posix_cred_info_size != 0 && px_args.posix_cred_info != 0) {
			                                                          
			if (px_args.posix_cred_info_size != sizeof(*px_pcred_info)) {
				error = ERANGE;
				goto bad;
			}

			if (!kauth_cred_issuser(kauth_cred_get())) {
				error = EPERM;
				goto bad;
			}

			px_pcred_info = kheap_alloc(KHEAP_TEMP,
			    px_args.posix_cred_info_size, Z_WAITOK);
			if (px_pcred_info == NULL) {
				error = ENOMEM;
				goto bad;
			}
			imgp->ip_px_pcred_info = px_pcred_info;

			if ((error = copyin(px_args.posix_cred_info, px_pcred_info,
			    px_args.posix_cred_info_size)) != 0) {
				goto bad;
			}

			if (px_pcred_info->pspci_flags & POSIX_SPAWN_POSIX_CRED_GROUPS) {
				if (px_pcred_info->pspci_ngroups > NGROUPS_MAX) {
					error = EINVAL;
					goto bad;
				}
			}
		}
#if CONFIG_MACF
		if (px_args.mac_extensions_size != 0) {
			if ((error = spawn_copyin_macpolicyinfo(&px_args, (_posix_spawn_mac_policy_extensions_t *)&imgp->ip_px_smpx)) != 0) {
				goto bad;
			}
		}
#endif                  
		if ((px_args.subsystem_root_path_size > 0) && (px_args.subsystem_root_path_size <= MAXPATHLEN)) {
			  
                                                
                  
      
			if (IOTaskHasEntitlement(old_task, SPAWN_SUBSYSTEM_ROOT_ENTITLEMENT)) {
				  
                                                  
                                  
       
				subsystem_root_path = zalloc_flags(ZV_NAMEI, Z_WAITOK | Z_ZERO);

				if (subsystem_root_path == NULL) {
					error = ENOMEM;
					goto bad;
				}

				if ((error = copyin(px_args.subsystem_root_path, subsystem_root_path, px_args.subsystem_root_path_size))) {
					goto bad;
				}

				              
				subsystem_root_path[px_args.subsystem_root_path_size - 1] = 0;
			}
		}
	}

	                           
	uthread = get_bsdthread_info(current_thread());

	  
                                                                
                                                                 
                        
    
	if (uthread->uu_flag & UT_VFORK) {
		error = EINVAL;
		goto bad;
	}

	if (imgp->ip_px_sa != NULL) {
		struct _posix_spawnattr *psa = (struct _posix_spawnattr *) imgp->ip_px_sa;
		if ((psa->psa_options & PSA_OPTION_PLUGIN_HOST_DISABLE_A_KEYS) == PSA_OPTION_PLUGIN_HOST_DISABLE_A_KEYS) {
			imgp->ip_flags |= IMGPF_PLUGIN_HOST_DISABLE_A_KEYS;
		}

		if ((error = exec_validate_spawnattr_policy(psa->psa_apptype)) != 0) {
			goto bad;
		}
	}

	  
                                                                  
                                                                
                                                                  
                                                                
                 
    
	if (imgp->ip_px_sa == NULL || !(px_sa.psa_flags & POSIX_SPAWN_SETEXEC)) {
		                                                        
		coalition_t coal[COALITION_NUM_TYPES] = { COALITION_NULL };
#if CONFIG_COALITIONS
		int i, ncoals;
		kern_return_t kr = KERN_SUCCESS;
		struct _posix_spawn_coalition_info coal_info;
		int coal_role[COALITION_NUM_TYPES];

		if (imgp->ip_px_sa == NULL || !px_args.coal_info) {
			goto do_fork1;
		}

		memset(&coal_info, 0, sizeof(coal_info));

		if (px_args.coal_info_size > sizeof(coal_info)) {
			px_args.coal_info_size = sizeof(coal_info);
		}
		error = copyin(px_args.coal_info,
		    &coal_info, px_args.coal_info_size);
		if (error != 0) {
			goto bad;
		}

		ncoals = 0;
		for (i = 0; i < COALITION_NUM_TYPES; i++) {
			uint64_t cid = coal_info.psci_info[i].psci_id;
			if (cid != 0) {
				  
                                           
                                              
                                           
       
				if (!task_is_in_privileged_coalition(p->task, i) &&
				    !IOTaskHasEntitlement(p->task, COALITION_SPAWN_ENTITLEMENT)) {
					coal_dbg("ERROR: %d not in privilegd "
					    "coalition of type %d",
					    p->p_pid, i);
					spawn_coalitions_release_all(coal);
					error = EPERM;
					goto bad;
				}

				coal_dbg("searching for coalition id:%llu", cid);
				  
                                             
                                                  
            
       
				coal[i] = coalition_find_and_activate_by_id(cid);
				if (coal[i] == COALITION_NULL) {
					coal_dbg("could not find coalition id:%llu "
					    "(perhaps it has been terminated or reaped)", cid);
					  
                                        
                               
        
					spawn_coalitions_release_all(coal);
					error = ESRCH;
					goto bad;
				}
				if (coalition_type(coal[i]) != i) {
					coal_dbg("coalition with id:%lld is not of type:%d"
					    " (it's type:%d)", cid, i, coalition_type(coal[i]));
					error = ESRCH;
					goto bad;
				}
				coal_role[i] = coal_info.psci_info[i].psci_role;
				ncoals++;
			}
		}
		if (ncoals < COALITION_NUM_TYPES) {
			  
                                                         
                                                         
                                                        
                                                         
                                                       
                                                      
                                                        
      
			if (coal[COALITION_TYPE_RESOURCE] == COALITION_NULL) {
				spawn_coalitions_release_all(coal);
				error = EINVAL;
				goto bad;
			}
		}
do_fork1:
#endif                        

		  
                                               
                                    
     
		error = fork1(p, &imgp->ip_new_thread, PROC_CREATE_SPAWN, coal);
		                                         

		if (error == 0) {
			new_task = get_threadtask(imgp->ip_new_thread);
		}
#if CONFIG_COALITIONS
		                                                            
		if (error == 0) {
			kr = coalitions_set_roles(coal, new_task, coal_role);
			if (kr != KERN_SUCCESS) {
				error = EINVAL;
			}
			if (kdebug_debugid_enabled(MACHDBG_CODE(DBG_MACH_COALITION,
			    MACH_COALITION_ADOPT))) {
				for (i = 0; i < COALITION_NUM_TYPES; i++) {
					if (coal[i] != COALITION_NULL) {
						  
                                    
                                      
         
						KDBG_RELEASE(MACHDBG_CODE(
							    DBG_MACH_COALITION,
							    MACH_COALITION_ADOPT),
						    coalition_id(coal[i]),
						    get_task_uniqueid(new_task));
					}
				}
			}
		}

		                                                                  
		spawn_coalitions_release_all(coal);
#endif                        
		if (error != 0) {
			goto bad;
		}
		imgp->ip_flags |= IMGPF_SPAWN;                      
		spawn_no_exec = TRUE;                                    
	} else {
		  
                                                  
                                                              
                                                               
    
                                                        
                                          
                                          
                                          
       
                                                      
    
                                                                
                                   
    
                                                                                 
         
         
                            
                       
                       
    
                                                                              
                                                               
     
		imgp->ip_new_thread = fork_create_child(old_task,
		    NULL,
		    p,
		    FALSE,
		    p->p_flag & P_LP64,
		    task_get_64bit_data(old_task),
		    TRUE);
		                                                       
		if (imgp->ip_new_thread == NULL) {
			error = ENOMEM;
			goto bad;
		}

		new_task = get_threadtask(imgp->ip_new_thread);
		imgp->ip_flags |= IMGPF_EXEC;
	}

	if (spawn_no_exec) {
		p = (proc_t)get_bsdthreadtask_info(imgp->ip_new_thread);

		  
                                                      
                                                            
                   
     
		DTRACE_PROC1(create, proc_t, p);
	}
	assert(p != NULL);

	if (subsystem_root_path) {
		                                                   
		char * old_subsystem_root_path = p->p_subsystem_root_path;
		p->p_subsystem_root_path = subsystem_root_path;
		subsystem_root_path = old_subsystem_root_path;
	}

	                                                                
	imgp->ip_subsystem_root_path = p->p_subsystem_root_path;

	context.vc_thread = imgp->ip_new_thread;
	context.vc_ucred = p->p_ucred;                                        

	  
                                                                  
                                                                   
                                                                     
                                                         
    

	                             
	if (imgp->ip_px_sfa != NULL) {
		  
                                         
                                              
     
#if CONFIG_AUDIT
		  
                                                         
                                                          
     
		struct kaudit_record *save_uu_ar = uthread->uu_ar;
		uthread->uu_ar = NULL;
#endif
		error = exec_handle_file_actions(imgp,
		    imgp->ip_px_sa != NULL ? px_sa.psa_flags : 0);
#if CONFIG_AUDIT
		                                               
		uthread->uu_ar = save_uu_ar;
#endif
		if (error != 0) {
			goto bad;
		}
	}

	                             
	if (imgp->ip_px_spa != NULL) {
#if CONFIG_AUDIT
		  
                                                            
                                                     
     
		struct kaudit_record *save_uu_ar = uthread->uu_ar;
		uthread->uu_ar = NULL;
#endif
		error = exec_handle_port_actions(imgp, &port_actions);
#if CONFIG_AUDIT
		                                               
		uthread->uu_ar = save_uu_ar;
#endif
		if (error != 0) {
			goto bad;
		}
	}

	                     
	if (imgp->ip_px_sa != NULL) {
		  
                                                         
                                                         
                                                        
                                                        
                                                        
                                                         
                      
    
                                                       
                                                           
                          
     
		if (px_sa.psa_flags & POSIX_SPAWN_RESETIDS) {
			apply_kauth_cred_update(p, ^kauth_cred_t (kauth_cred_t my_cred){
				return kauth_cred_setuidgid(my_cred,
				kauth_cred_getruid(my_cred),
				kauth_cred_getrgid(my_cred));
			});
		}

		if (imgp->ip_px_pcred_info) {
			if (!spawn_no_exec) {
				error = ENOTSUP;
				goto bad;
			}

			error = spawn_posix_cred_adopt(p, imgp->ip_px_pcred_info);
			if (error != 0) {
				goto bad;
			}
		}

#if CONFIG_PERSONAS
		if (imgp->ip_px_persona != NULL) {
			if (!spawn_no_exec) {
				error = ENOTSUP;
				goto bad;
			}

			  
                                                             
                                                                  
                                                                
                                                                
                                                                 
                     
      
			error = spawn_persona_adopt(p, imgp->ip_px_persona);
			if (error != 0) {
				goto bad;
			}
		}
#endif                      
#if !SECURE_KERNEL
		  
                                          
    
                                                     
                                                         
                                                       
                                                     
                                                         
                                          
     
		if (px_sa.psa_flags & _POSIX_SPAWN_DISABLE_ASLR) {
			OSBitOrAtomic(P_DISABLE_ASLR, &p->p_flag);
		}
#endif                     

		                                       
		if (px_sa.psa_flags & _POSIX_SPAWN_HIGH_BITS_ASLR) {
			imgp->ip_flags |= IMGPF_HIGH_BITS_ASLR;
		}

#if !SECURE_KERNEL
		  
                                                                        
                                                                         
     
		if (px_sa.psa_flags & _POSIX_SPAWN_ALLOW_DATA_EXEC) {
			imgp->ip_flags |= IMGPF_ALLOW_DATA_EXEC;
		}
#endif                     

#if     __has_feature(ptrauth_calls)
		if (vm_shared_region_reslide_aslr && is_64 && (px_sa.psa_flags & _POSIX_SPAWN_RESLIDE)) {
			imgp->ip_flags |= IMGPF_RESLIDE;
		}
#endif                                   

		if ((px_sa.psa_apptype & POSIX_SPAWN_PROC_TYPE_MASK) ==
		    POSIX_SPAWN_PROC_TYPE_DRIVER) {
			imgp->ip_flags |= IMGPF_DRIVER;
		}
	}

	  
                                                                    
                                                             
                                                         
    
	if (p->p_flag & P_DISABLE_ASLR) {
		imgp->ip_flags |= IMGPF_DISABLE_ASLR;
	}

	  
                                                                          
                                                                
   
                                                       
    
	if (spawn_no_exec) {
		proc_transend(p, 0);
		proc_transit_set = 0;
	}

#if MAC_SPAWN            
	if (uap->mac_p != USER_ADDR_NULL) {
		error = mac_execve_enter(uap->mac_p, imgp);
		if (error) {
			goto bad;
		}
	}
#endif

	  
                      
    
	error = exec_activate_image(imgp);
#if defined(HAS_APPLE_PAC)
	ml_task_set_jop_pid_from_shared_region(new_task);
	ml_task_set_disable_user_jop(new_task, imgp->ip_flags & IMGPF_NOJOP ? TRUE : FALSE);
	ml_thread_set_disable_user_jop(imgp->ip_new_thread, imgp->ip_flags & IMGPF_NOJOP ? TRUE : FALSE);
	ml_thread_set_jop_pid(imgp->ip_new_thread, new_task);
#endif

	if (error == 0 && !spawn_no_exec) {
		p = proc_exec_switch_task(p, old_task, new_task, imgp->ip_new_thread, &inherit);
		                       
		should_release_proc_ref = TRUE;
	}

	if (error == 0) {
		                                
		exec_done = TRUE;
	} else if (error == -1) {
		                                         
		error = ENOEXEC;
	}

	if (!error && imgp->ip_px_sa != NULL) {
		thread_t child_thread = imgp->ip_new_thread;
		uthread_t child_uthread = get_bsdthread_info(child_thread);

		  
                                                                       
                                                                         
                                                                
     
		if (px_sa.psa_flags & POSIX_SPAWN_SETPGROUP) {
			struct setpgid_args spga;
			spga.pid = p->p_pid;
			spga.pgid = px_sa.psa_pgroup;
			  
                                                    
                                             
      
			if ((error = setpgid(p, &spga, ival)) != 0) {
				goto bad;
			}
		}

		if (px_sa.psa_flags & POSIX_SPAWN_SETSID) {
			error = setsid_internal(p);
			if (error != 0) {
				goto bad;
			}
		}

		  
                                                                   
                                                                  
                                                                  
                                                                
                                                                    
                                                                   
                                        
     

		  
                                                               
                                                             
                      
     
		if (px_sa.psa_flags & POSIX_SPAWN_SETSIGMASK) {
			child_uthread->uu_sigmask = (px_sa.psa_sigmask & ~sigcantmask);
		}
		  
                                                           
                                                        
                                                            
                                                                 
                
     
		if (px_sa.psa_flags & POSIX_SPAWN_SETSIGDEF) {
			vec.sa_handler = SIG_DFL;
			vec.sa_tramp = 0;
			vec.sa_mask = 0;
			vec.sa_flags = 0;
			for (sig = 1; sig < NSIG; sig++) {
				if (px_sa.psa_sigdefault & (1 << (sig - 1))) {
					error = setsigvec(p, child_thread, sig, &vec, spawn_no_exec);
				}
			}
		}

		  
                                                                                               
                                                                                                 
           
    
                                                                                   
     
		if ((px_sa.psa_cpumonitor_percent != 0) && (px_sa.psa_cpumonitor_percent < UINT8_MAX)) {
			  
                                                                                    
                                                                              
                                                                         
      
			error = proc_set_task_ruse_cpu(p->task,
			    TASK_POLICY_RESOURCE_ATTRIBUTE_NOTIFY_EXC,
			    (uint8_t)px_sa.psa_cpumonitor_percent,
			    px_sa.psa_cpumonitor_interval * NSEC_PER_SEC,
			    0, TRUE);
		}


		if (px_pcred_info &&
		    (px_pcred_info->pspci_flags & POSIX_SPAWN_POSIX_CRED_LOGIN)) {
			  
                                           
      
			setlogin_internal(p, px_pcred_info->pspci_login);
		}
	}

bad:

	if (error == 0) {
		                                          
#if CONFIG_DELAY_IDLE_SLEEP
		if ((p->p_flag & P_DELAYIDLESLEEP) == P_DELAYIDLESLEEP) {
			OSBitAndAtomic(~((uint32_t)P_DELAYIDLESLEEP), &p->p_flag);
		}
#endif                              
		                                                           
		if (imgp->ip_px_sa != NULL) {
			switch (px_sa.psa_pcontrol) {
			case POSIX_SPAWN_PCONTROL_THROTTLE:
				p->p_pcaction = P_PCTHROTTLE;
				break;
			case POSIX_SPAWN_PCONTROL_SUSPEND:
				p->p_pcaction = P_PCSUSP;
				break;
			case POSIX_SPAWN_PCONTROL_KILL:
				p->p_pcaction = P_PCKILL;
				break;
			case POSIX_SPAWN_PCONTROL_NONE:
			default:
				p->p_pcaction = 0;
				break;
			}
			;
		}
		exec_resettextvp(p, imgp);

#if CONFIG_MEMORYSTATUS
		                                                 
		if (px_sa.psa_apptype == POSIX_SPAWN_PROC_TYPE_DRIVER) {
			px_sa.psa_priority = JETSAM_PRIORITY_DRIVER_APPLE;
		}

		                            
		if (imgp->ip_px_sa != NULL && (px_sa.psa_jetsam_flags & POSIX_SPAWN_JETSAM_SET)) {
			  
                                                                                       
                                                                                       
     
                                                                                    
                                                                                 
                                                                                        
      

			if (px_sa.psa_jetsam_flags & POSIX_SPAWN_JETSAM_HIWATER_BACKGROUND) {
				memorystatus_update(p, px_sa.psa_priority, 0, FALSE,                         
				    (px_sa.psa_jetsam_flags & POSIX_SPAWN_JETSAM_USE_EFFECTIVE_PRIORITY),
				    TRUE,
				    -1, TRUE,
				    px_sa.psa_memlimit_inactive, FALSE);
			} else {
				memorystatus_update(p, px_sa.psa_priority, 0, FALSE,                         
				    (px_sa.psa_jetsam_flags & POSIX_SPAWN_JETSAM_USE_EFFECTIVE_PRIORITY),
				    TRUE,
				    px_sa.psa_memlimit_active,
				    (px_sa.psa_jetsam_flags & POSIX_SPAWN_JETSAM_MEMLIMIT_ACTIVE_FATAL),
				    px_sa.psa_memlimit_inactive,
				    (px_sa.psa_jetsam_flags & POSIX_SPAWN_JETSAM_MEMLIMIT_INACTIVE_FATAL));
			}
		}

		                                   
		if (imgp->ip_px_sa != NULL && (px_sa.psa_jetsam_flags & POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_MASK)) {
			  
                                                                                               
                                                                                              
                                                            
      
			int relaunch_flags = P_MEMSTAT_RELAUNCH_UNKNOWN;
			switch (px_sa.psa_jetsam_flags & POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_MASK) {
			case POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_LOW:
				relaunch_flags = P_MEMSTAT_RELAUNCH_LOW;
				break;
			case POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_MED:
				relaunch_flags = P_MEMSTAT_RELAUNCH_MED;
				break;
			case POSIX_SPAWN_JETSAM_RELAUNCH_BEHAVIOR_HIGH:
				relaunch_flags = P_MEMSTAT_RELAUNCH_HIGH;
				break;
			default:
				break;
			}
			memorystatus_relaunch_flags_update(p, relaunch_flags);
		}

#endif                          
		if (imgp->ip_px_sa != NULL && px_sa.psa_thread_limit > 0) {
			task_set_thread_limit(new_task, (uint16_t)px_sa.psa_thread_limit);
		}

		                                                       
		if (px_sa.psa_apptype == POSIX_SPAWN_PROC_TYPE_DRIVER) {
			uint32_t      flags = WAKEMON_DISABLE;
			task_wakeups_monitor_ctl(new_task, &flags, NULL);
		}
	}

	  
                                                                 
                                                                  
                                                                   
                                                                 
                                                                
                                                     
    
	if (spawn_no_exec) {
		if (proc_transit_set) {
			proc_transend(p, 0);
		}

		  
                                                             
                                                              
                                                            
     
		proc_signalend(p, 0);
	}

	if (error == 0) {
		  
                                                                    
                                                                             
                                                                        
                                                                       
     
		error = proc_transstart(p, 0, 0);

		if (error == 0) {
			task_bank_init(new_task);
			proc_transend(p, 0);
		}

#if __arm64__
		proc_footprint_entitlement_hacks(p, new_task);
#endif                

#if __has_feature(ptrauth_calls)
		task_set_pac_exception_fatal_flag(new_task);
#endif                                   
	}

	                                                          
	if (error == 0 && !spawn_no_exec) {
		proc_inherit_task_role(new_task, old_task);
	}

#if CONFIG_ARCADE
	if (error == 0) {
		  
                                                                
                                               
     
		arcade_prepare(new_task, imgp->ip_new_thread);
	}
#endif                    

	                                                                       
	if (imgp && imgp->ip_new_thread) {
		task_clear_return_wait(get_threadtask(imgp->ip_new_thread), TCRW_CLEAR_INITIAL_WAIT);
	}

	  
                                                                                        
                                                 
                                                                         
                                                                         
                                                                                     
   
                                                         
    
	if (error == 0 && imgp->ip_px_sa != NULL) {
		struct _posix_spawnattr *psa = (struct _posix_spawnattr *) imgp->ip_px_sa;

		error = exec_handle_spawnattr_policy(p, imgp->ip_new_thread, psa->psa_apptype, psa->psa_qos_clamp,
		    psa->psa_darwin_role, &port_actions);
	}

	                                                                   
	if (error == 0 && !spawn_no_exec) {
		task_transfer_turnstile_watchports(old_task, new_task, imgp->ip_new_thread);
	}

	  
                                        
    
	if (error == 0 && imgp->ip_px_sa != NULL) {
		struct _posix_spawnattr *psa = (struct _posix_spawnattr *) imgp->ip_px_sa;

		if (psa->psa_max_addr) {
			vm_map_set_max_addr(get_task_map(new_task), (vm_map_offset_t)psa->psa_max_addr);
		}
	}

	if (error == 0 && imgp->ip_px_sa != NULL) {
		struct _posix_spawnattr *psa = (struct _posix_spawnattr *) imgp->ip_px_sa;

		if (psa->psa_no_smt) {
			task_set_no_smt(new_task);
		}
		if (psa->psa_tecs) {
			task_set_tecs(new_task);
		}
	}

	if (error == 0) {
		                               
		thread_t main_thread = imgp->ip_new_thread;
		task_set_main_thread_qos(new_task, main_thread);

#if CONFIG_MACF
		proc_apply_jit_and_jumbo_va_policies(p, new_task);
#endif                  
	}

	  
                                                                
                                                                     
    
	if (imgp->ip_px_spa != NULL) {
		exec_port_actions_destroy(&port_actions);
	}

	  
                                                                      
                                                                    
                                                                    
         
    
	if (error == 0) {
		error = check_for_signature(p, imgp);

		  
                                                                 
                                                         
     
		if (imgp != NULL && spawn_no_exec && (p->p_lflag & P_LTRACED)) {
			psignal_vfork(p, p->task, imgp->ip_new_thread, SIGTRAP);
		}

		if (error == 0 && !spawn_no_exec) {
			KDBG(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXEC),
			    p->p_pid);
		}
	}

	if (spawn_no_exec) {
		                                  
		proc_knote(p->p_pptr, NOTE_FORK | p->p_pid);
	}

	                                                                                  
	if (!error && ((p->p_lflag & P_LTERM_DECRYPTFAIL) == 0)) {
		proc_knote(p, NOTE_EXEC);
	}

	if (imgp != NULL) {
		if (imgp->ip_vp) {
			vnode_put(imgp->ip_vp);
		}
		if (imgp->ip_scriptvp) {
			vnode_put(imgp->ip_scriptvp);
		}
		if (imgp->ip_strings) {
			execargs_free(imgp);
		}
		kheap_free(KHEAP_TEMP, imgp->ip_px_sfa,
		    px_args.file_actions_size);
		kheap_free(KHEAP_TEMP, imgp->ip_px_spa,
		    px_args.port_actions_size);
#if CONFIG_PERSONAS
		kheap_free(KHEAP_TEMP, imgp->ip_px_persona,
		    px_args.persona_info_size);
#endif
		kheap_free(KHEAP_TEMP, imgp->ip_px_pcred_info,
		    px_args.posix_cred_info_size);

		if (subsystem_root_path != NULL) {
			zfree(ZV_NAMEI, subsystem_root_path);
		}
#if CONFIG_MACF
		_posix_spawn_mac_policy_extensions_t psmx = imgp->ip_px_smpx;
		if (psmx) {
			spawn_free_macpolicyinfo(&px_args,
			    psmx, psmx->psmx_count);
		}
		if (imgp->ip_execlabelp) {
			mac_cred_label_free(imgp->ip_execlabelp);
		}
		if (imgp->ip_scriptlabelp) {
			mac_vnode_label_free(imgp->ip_scriptlabelp);
		}
		if (imgp->ip_cs_error != OS_REASON_NULL) {
			os_reason_free(imgp->ip_cs_error);
			imgp->ip_cs_error = OS_REASON_NULL;
		}
		if (imgp->ip_inherited_shared_region_id != NULL) {
			kheap_free(KHEAP_DATA_BUFFERS, imgp->ip_inherited_shared_region_id,
			    strlen(imgp->ip_inherited_shared_region_id) + 1);
			imgp->ip_inherited_shared_region_id = NULL;
		}
#endif
		if (imgp->ip_sc_port != NULL) {
			ipc_port_release_send(imgp->ip_sc_port);
			imgp->ip_sc_port = NULL;
		}
	}

#if CONFIG_DTRACE
	if (spawn_no_exec) {
		  
                                                     
                                               
                                                        
                                                      
                                                           
                                                          
                                                         
                                            
     
		if (error) {
			DTRACE_PROC1(spawn__failure, int, error);
		} else {
			DTRACE_PROC(spawn__success);
			  
                                            
                                              
                                              
                                            
                                               
     
                                                
                                                  
                                                
                 
      
			dtrace_thread_didexec(imgp->ip_new_thread);
		}
	} else {
		if (error) {
			DTRACE_PROC1(exec__failure, int, error);
		} else {
			dtrace_thread_didexec(imgp->ip_new_thread);
		}
	}

	if ((dtrace_proc_waitfor_hook = dtrace_proc_waitfor_exec_ptr) != NULL) {
		(*dtrace_proc_waitfor_hook)(p);
	}
#endif

#if CONFIG_AUDIT
	if (!error && AUDIT_ENABLED() && p) {
		                                                           
		uint8_t *cdhash = cs_get_cdhash(p);
		if (cdhash) {
			AUDIT_ARG(data, cdhash, sizeof(uint8_t), CS_CDHASH_LEN);
		}
	}
#endif

	  
                                                
    
	if (task_did_exec(old_task)) {
		set_bsdtask_info(old_task, NULL);
	}

	                                                                   
	if (new_task != NULL && task_is_exec_copy(new_task)) {
		set_bsdtask_info(new_task, NULL);
		task_terminate_internal(new_task);
	}

	                                              
	if (imgp != NULL && spawn_no_exec) {
		  
                                             
     
		if (pid != USER_ADDR_NULL) {
			_Static_assert(sizeof(p->p_pid) == 4, "posix_spawn() assumes a 32-bit pid_t");
			bool aligned = (pid & 3) == 0;
			if (aligned) {
				(void)copyout_atomic32(p->p_pid, pid);
			} else {
				(void)suword(pid, p->p_pid);
			}
		}
		retval[0] = error;

		  
                                                           
                                                         
     
		if (error) {
			proc_list_lock();
			p->p_listflag |= P_LIST_DEADPARENT;
			proc_list_unlock();
			proc_lock(p);
			                                                
			if (p->p_stat != SZOMB && p->exit_thread == NULL) {
				p->exit_thread = current_thread();
				proc_unlock(p);
				exit1(p, 1, (int *)NULL);
			} else {
				                                              
				proc_unlock(p);
			}
		}
	}

	  
                                                                       
                                                                           
                                         
    
	if (task_did_exec(old_task)) {
		                                                                   
		task_terminate_internal(old_task);
	}

	                                                                
	if (imgp != NULL && imgp->ip_new_thread) {
		                            
		task_clear_return_wait(get_threadtask(imgp->ip_new_thread), TCRW_CLEAR_FINAL_WAIT);
		thread_deallocate(imgp->ip_new_thread);
		imgp->ip_new_thread = NULL;
	}

	                                                         
	if (new_task) {
		task_deallocate(new_task);
		new_task = NULL;
	}

	if (should_release_proc_ref) {
		proc_rele(p);
	}

	kheap_free(KHEAP_TEMP, bufp,
	    sizeof(*imgp) + sizeof(*vap) + sizeof(*origvap));

	if (inherit != NULL) {
		ipc_importance_release(inherit);
	}

	return error;
}

  
                        
  
                        
                              
                             
                                  
                                          
  
                 
  
                                                          
                                                        
                                                            
                                                                
                                                                      
                                                             
             
  
                                                                
                                                                    
                                                              
   
proc_t
proc_exec_switch_task(proc_t p, task_t old_task, task_t new_task, thread_t new_thread,
    void **inherit)
{
	int error = 0;
	boolean_t task_active;
	boolean_t proc_active;
	boolean_t thread_active;
	thread_t old_thread = current_thread();

	  
                                                
                                                      
                                                     
                                                
                                                      
                                                       
                                
    
	thread_set_exec_promotion(old_thread);
	p = proc_refdrain_with_refwait(p, TRUE);
	                                           

	assert(get_threadtask(new_thread) == new_task);
	task_active = task_is_active(new_task);

	                                                    
	proc_lock(p);
	proc_active = !(p->p_lflag & P_LEXIT);

	                                                               
	thread_active = thread_is_active(old_thread);

	  
                                                                        
                                                        
    
	if (proc_active && task_active && thread_active) {
		error = proc_transstart(p, 1, 0);
		if (error == 0) {
			uthread_t new_uthread = get_bsdthread_info(new_thread);
			uthread_t old_uthread = get_bsdthread_info(current_thread());

			  
                                                                     
                                                   
      
			p->task = new_task;

			                                                 
			p->p_dispatchqueue_offset = 0;
			p->p_dispatchqueue_serialno_offset = 0;
			p->p_dispatchqueue_label_offset = 0;
			p->p_return_to_kernel_offset = 0;

			                                                                       
			act_set_astbsd(new_thread);
			new_uthread->uu_siglist = old_uthread->uu_siglist;
			new_uthread->uu_sigwait = old_uthread->uu_sigwait;
			new_uthread->uu_sigmask = old_uthread->uu_sigmask;
			new_uthread->uu_oldmask = old_uthread->uu_oldmask;
			new_uthread->uu_vforkmask = old_uthread->uu_vforkmask;
			new_uthread->uu_exit_reason = old_uthread->uu_exit_reason;
#if CONFIG_DTRACE
			new_uthread->t_dtrace_sig = old_uthread->t_dtrace_sig;
			new_uthread->t_dtrace_stop = old_uthread->t_dtrace_stop;
			new_uthread->t_dtrace_resumepid = old_uthread->t_dtrace_resumepid;
			assert(new_uthread->t_dtrace_scratch == NULL);
			new_uthread->t_dtrace_scratch = old_uthread->t_dtrace_scratch;

			old_uthread->t_dtrace_sig = 0;
			old_uthread->t_dtrace_stop = 0;
			old_uthread->t_dtrace_resumepid = 0;
			old_uthread->t_dtrace_scratch = NULL;
#endif
			                                       
			thread_copy_resource_info(new_thread, current_thread());

			                                                          
			old_uthread->uu_exit_reason = NULL;
			old_uthread->uu_siglist = 0;

			                                                                
			TAILQ_INSERT_TAIL(&p->p_uthlist, new_uthread, uu_list);
			TAILQ_REMOVE(&p->p_uthlist, old_uthread, uu_list);

			task_set_did_exec_flag(old_task);
			task_clear_exec_copy_flag(new_task);

			task_copy_fields_for_exec(new_task, old_task);

			                                               
			task_transfer_mach_filter_bits(new_task, old_task);

			  
                                                                
                                                              
                                                                    
                                                                   
                                                                 
                                                                    
                  
      
			*inherit = ipc_importance_exec_switch_task(old_task, new_task);

			proc_transend(p, 1);
		}
	}

	proc_unlock(p);
	proc_refwake(p);
	thread_clear_exec_promotion(old_thread);

	if (error != 0 || !task_active || !proc_active || !thread_active) {
		task_terminate_internal(new_task);
	}

	return p;
}

  
         
  
                                            
                            
                               
  
                       
                                        
                                         
                                         
                                           
                                        
                                   
                                              
                                                     
                                                              
                   
  
                                                                      
   
              
int
execve(proc_t p, struct execve_args *uap, int32_t *retval)
{
	struct __mac_execve_args muap;
	int err;

	memoryshot(VM_EXECVE, DBG_FUNC_NONE);

	muap.fname = uap->fname;
	muap.argp = uap->argp;
	muap.envp = uap->envp;
	muap.mac_p = USER_ADDR_NULL;
	err = __mac_execve(p, &muap, retval);

	return err;
}

  
               
  
                                            
                            
                               
                                            
  
                       
                             
                           
                                          
                                              
                                               
                                                 
                                              
                                         
                                                     
                                                           
                                                                    
                                                                 
                          
                       
  
                                                                      
   
int
__mac_execve(proc_t p, struct __mac_execve_args *uap, int32_t *retval)
{
	char *bufp = NULL;
	struct image_params *imgp;
	struct vnode_attr *vap;
	struct vnode_attr *origvap;
	int error;
	int is_64 = IS_64BIT_PROCESS(p);
	struct vfs_context context;
	struct uthread  *uthread;
	task_t old_task = current_task();
	task_t new_task = NULL;
	boolean_t should_release_proc_ref = FALSE;
	boolean_t exec_done = FALSE;
	boolean_t in_vfexec = FALSE;
	void *inherit = NULL;

	context.vc_thread = current_thread();
	context.vc_ucred = kauth_cred_proc_ref(p);                                            

	                                                                     
                            
    
	bufp = kheap_alloc(KHEAP_TEMP,
	    sizeof(*imgp) + sizeof(*vap) + sizeof(*origvap), Z_WAITOK | Z_ZERO);
	imgp = (struct image_params *) bufp;
	if (bufp == NULL) {
		error = ENOMEM;
		goto exit_with_error;
	}
	vap = (struct vnode_attr *) (bufp + sizeof(*imgp));
	origvap = (struct vnode_attr *) (bufp + sizeof(*imgp) + sizeof(*vap));

	                                                              
	imgp->ip_user_fname = uap->fname;
	imgp->ip_user_argv = uap->argp;
	imgp->ip_user_envv = uap->envp;
	imgp->ip_vattr = vap;
	imgp->ip_origvattr = origvap;
	imgp->ip_vfs_context = &context;
	imgp->ip_flags = (is_64 ? IMGPF_WAS_64BIT_ADDR : IMGPF_NONE) | ((p->p_flag & P_DISABLE_ASLR) ? IMGPF_DISABLE_ASLR : IMGPF_NONE);
	imgp->ip_seg = (is_64 ? UIO_USERSPACE64 : UIO_USERSPACE32);
	imgp->ip_mac_return = 0;
	imgp->ip_cs_error = OS_REASON_NULL;
	imgp->ip_simulator_binary = IMGPF_SB_DEFAULT;
	imgp->ip_subsystem_root_path = NULL;

#if CONFIG_MACF
	if (uap->mac_p != USER_ADDR_NULL) {
		error = mac_execve_enter(uap->mac_p, imgp);
		if (error) {
			kauth_cred_unref(&context.vc_ucred);
			goto exit_with_error;
		}
	}
#endif
	uthread = get_bsdthread_info(current_thread());
	if (uthread->uu_flag & UT_VFORK) {
		imgp->ip_flags |= IMGPF_VFORK_EXEC;
		in_vfexec = TRUE;
	} else {
		imgp->ip_flags |= IMGPF_EXEC;

		  
                                                  
                                                              
                                                               
    
                                                        
                                          
                                          
                                          
       
                                                      
    
                                                                
                                   
    
                                                                                 
         
         
                            
                       
                       
    
                                                                              
                                                               
     
		imgp->ip_new_thread = fork_create_child(old_task,
		    NULL,
		    p,
		    FALSE,
		    p->p_flag & P_LP64,
		    task_get_64bit_data(old_task),
		    TRUE);
		                                                       
		if (imgp->ip_new_thread == NULL) {
			error = ENOMEM;
			goto exit_with_error;
		}

		new_task = get_threadtask(imgp->ip_new_thread);
		context.vc_thread = imgp->ip_new_thread;
	}

	imgp->ip_subsystem_root_path = p->p_subsystem_root_path;

	error = exec_activate_image(imgp);
	                                                  

	if (imgp->ip_new_thread != NULL) {
		  
                                                            
                
     
		new_task = get_threadtask(imgp->ip_new_thread);
#if defined(HAS_APPLE_PAC)
		ml_task_set_disable_user_jop(new_task, imgp->ip_flags & IMGPF_NOJOP ? TRUE : FALSE);
		ml_thread_set_disable_user_jop(imgp->ip_new_thread, imgp->ip_flags & IMGPF_NOJOP ? TRUE : FALSE);
#endif
	}

	if (!error && !in_vfexec) {
		p = proc_exec_switch_task(p, old_task, new_task, imgp->ip_new_thread, &inherit);
		                       
		should_release_proc_ref = TRUE;
	}

	kauth_cred_unref(&context.vc_ucred);

	                                         
	if (error == -1) {
		error = ENOEXEC;
	}

	if (!error) {
		exec_done = TRUE;
		assert(imgp->ip_new_thread != NULL);

		exec_resettextvp(p, imgp);
		error = check_for_signature(p, imgp);
	}

#if defined(HAS_APPLE_PAC)
	if (imgp->ip_new_thread && !error) {
		ml_task_set_jop_pid_from_shared_region(new_task);
		ml_thread_set_jop_pid(imgp->ip_new_thread, new_task);
	}
#endif                             

	                                                                                  
	if (exec_done && ((p->p_lflag & P_LTERM_DECRYPTFAIL) == 0)) {
		proc_knote(p, NOTE_EXEC);
	}

	if (imgp->ip_vp != NULLVP) {
		vnode_put(imgp->ip_vp);
	}
	if (imgp->ip_scriptvp != NULLVP) {
		vnode_put(imgp->ip_scriptvp);
	}
	if (imgp->ip_strings) {
		execargs_free(imgp);
	}
#if CONFIG_MACF
	if (imgp->ip_execlabelp) {
		mac_cred_label_free(imgp->ip_execlabelp);
	}
	if (imgp->ip_scriptlabelp) {
		mac_vnode_label_free(imgp->ip_scriptlabelp);
	}
#endif
	if (imgp->ip_cs_error != OS_REASON_NULL) {
		os_reason_free(imgp->ip_cs_error);
		imgp->ip_cs_error = OS_REASON_NULL;
	}

	if (!error) {
		  
                                                                    
                                                                             
                                                                        
                                                                       
     
		error = proc_transstart(p, 0, 0);
	}

	if (!error) {
		task_bank_init(new_task);
		proc_transend(p, 0);

#if __arm64__
		proc_footprint_entitlement_hacks(p, new_task);
#endif                

		                                      
		thread_affinity_exec(current_thread());

		                                                          
		if (!in_vfexec) {
			proc_inherit_task_role(new_task, old_task);
		}

		thread_t main_thread = imgp->ip_new_thread;

		task_set_main_thread_qos(new_task, main_thread);

#if __has_feature(ptrauth_calls)
		task_set_pac_exception_fatal_flag(new_task);
#endif                                   

#if CONFIG_ARCADE
		  
                                                                
                                               
     
		arcade_prepare(new_task, imgp->ip_new_thread);
#endif                    

#if CONFIG_MACF
		proc_apply_jit_and_jumbo_va_policies(p, new_task);
#endif                  

		if (vm_darkwake_mode == TRUE) {
			  
                                                    
                                                     
                                                            
      
			task_set_darkwake_mode(new_task, vm_darkwake_mode);
		}

#if CONFIG_DTRACE
		dtrace_thread_didexec(imgp->ip_new_thread);

		if ((dtrace_proc_waitfor_hook = dtrace_proc_waitfor_exec_ptr) != NULL) {
			(*dtrace_proc_waitfor_hook)(p);
		}
#endif

#if CONFIG_AUDIT
		if (!error && AUDIT_ENABLED() && p) {
			                                                           
			uint8_t *cdhash = cs_get_cdhash(p);
			if (cdhash) {
				AUDIT_ARG(data, cdhash, sizeof(uint8_t), CS_CDHASH_LEN);
			}
		}
#endif

		if (in_vfexec) {
			vfork_return(p, retval, p->p_pid);
		}
	} else {
		DTRACE_PROC1(exec__failure, int, error);
	}

exit_with_error:

	  
                                                
    
	if (task_did_exec(old_task)) {
		set_bsdtask_info(old_task, NULL);
	}

	                                                                   
	if (new_task != NULL && task_is_exec_copy(new_task)) {
		set_bsdtask_info(new_task, NULL);
		task_terminate_internal(new_task);
	}

	if (imgp != NULL) {
		                                                                  
		if (imgp->ip_new_thread) {
			task_clear_return_wait(get_threadtask(imgp->ip_new_thread), TCRW_CLEAR_INITIAL_WAIT);
		}

		                                              
		if (!error && !in_vfexec) {
			task_transfer_turnstile_watchports(old_task,
			    new_task, imgp->ip_new_thread);
		}
		  
                                                                        
                                                                            
                                          
     
		if (task_did_exec(old_task)) {
			                                                                   
			task_terminate_internal(old_task);
		}

		                                                          
		if (imgp->ip_new_thread) {
			                                 
			task_clear_return_wait(get_threadtask(imgp->ip_new_thread), TCRW_CLEAR_FINAL_WAIT);
			thread_deallocate(imgp->ip_new_thread);
			imgp->ip_new_thread = NULL;
		}
	}

	                                                   
	if (new_task) {
		task_deallocate(new_task);
		new_task = NULL;
	}

	if (should_release_proc_ref) {
		proc_rele(p);
	}

	kheap_free(KHEAP_TEMP, bufp,
	    sizeof(*imgp) + sizeof(*vap) + sizeof(*origvap));

	if (inherit != NULL) {
		ipc_importance_release(inherit);
	}

	return error;
}


  
            
  
                                                                            
                                                   
  
                                         
                                               
                                                 
  
                       
                        
  
                    
                       
   
static int
copyinptr(user_addr_t froma, user_addr_t *toptr, int ptr_size)
{
	int error;

	if (ptr_size == 4) {
		                                            
		unsigned int i = 0;

		error = copyin(froma, &i, 4);
		*toptr = CAST_USER_ADDR_T(i);             
	} else {
		error = copyin(froma, toptr, 8);
	}
	return error;
}


  
             
  
                                                                        
                                                        
  
                                                 
                                             
                                              
  
                       
                     
  
   
static int
copyoutptr(user_addr_t ua, user_addr_t ptr, int ptr_size)
{
	int error;

	if (ptr_size == 4) {
		                                            
		unsigned int i = CAST_DOWN_EXPLICIT(unsigned int, ua);             

		error = copyout(&i, ptr, 4);
	} else {
		error = copyout(&ua, ptr, 8);
	}
	return error;
}


  
                       
  
                                                                          
                                   
  
                                                              
                                                  
  
                       
                      
  
                    
                                         
  
                                                                   
                                                            
                                                            
                                                    
  
                                                             
                              
  
                             
  
                                        
                       
                       
                       
                       
                       
                       
                       
                       
                       
                       
                       
                       
                       
                
                
                       
                       
                       
                       
                       
                       
                       
                       
                       
                       
                
                
                       
                       
                       
                       
                       
                       
                       
                
                
                       
                       
                       
                       
                       
  
                                                                            
                                                                             
                                                                            
                                                      
   
__attribute__((noinline))
static int
exec_copyout_strings(struct image_params *imgp, user_addr_t *stackp)
{
	proc_t p = vfs_context_proc(imgp->ip_vfs_context);
	int     ptr_size = (imgp->ip_flags & IMGPF_IS_64BIT_ADDR) ? 8 : 4;
	int     ptr_area_size;
	void *ptr_buffer_start, *ptr_buffer;
	size_t string_size;

	user_addr_t     string_area;                         
	user_addr_t     ptr_area;                                    
	user_addr_t argc_area;            
	user_addr_t     stack;
	int error;

	unsigned i;
	struct copyout_desc {
		char    *start_string;
		int             count;
#if CONFIG_DTRACE
		user_addr_t     *dtrace_cookie;
#endif
		boolean_t       null_term;
	} descriptors[] = {
		{
			.start_string = imgp->ip_startargv,
			.count = imgp->ip_argc,
#if CONFIG_DTRACE
			.dtrace_cookie = &p->p_dtrace_argv,
#endif
			.null_term = TRUE
		},
		{
			.start_string = imgp->ip_endargv,
			.count = imgp->ip_envc,
#if CONFIG_DTRACE
			.dtrace_cookie = &p->p_dtrace_envp,
#endif
			.null_term = TRUE
		},
		{
			.start_string = imgp->ip_strings,
			.count = 1,
#if CONFIG_DTRACE
			.dtrace_cookie = NULL,
#endif
			.null_term = FALSE
		},
		{
			.start_string = imgp->ip_endenvv,
			.count = imgp->ip_applec - 1,                              
#if CONFIG_DTRACE
			.dtrace_cookie = NULL,
#endif
			.null_term = TRUE
		}
	};

	stack = *stackp;

	  
                                                
                                      
    
	if (imgp->ip_strspace % ptr_size != 0) {
		error = EINVAL;
		goto bad;
	}

	                                                                
	string_size = imgp->ip_strendp - imgp->ip_strings;
	stack -= string_size;
	string_area = stack;

	  
                                                   
                                                                  
    
	ptr_area_size = (imgp->ip_argc + imgp->ip_envc + imgp->ip_applec + 3) * ptr_size;
	stack -= ptr_area_size;
	ptr_area = stack;

	                                                               
                                                              
                                            
    
	ptr_buffer_start = ptr_buffer = (void *)imgp->ip_strendp;

	  
                                            
    
	stack -= ptr_size;
	argc_area = stack;

	  
                                                                   
                                                                       
    
	proc_lock(p);
	p->p_argc = imgp->ip_argc;
	p->p_argslen = (int)(*stackp - string_area);
	proc_unlock(p);

	                                                            
	*stackp = stack;

	  
                                     
    
	error = copyout(imgp->ip_strings, string_area,
	    string_size);
	if (error) {
		goto bad;
	}

	for (i = 0; i < sizeof(descriptors) / sizeof(descriptors[0]); i++) {
		char *cur_string = descriptors[i].start_string;
		int j;

#if CONFIG_DTRACE
		if (descriptors[i].dtrace_cookie) {
			proc_lock(p);
			*descriptors[i].dtrace_cookie = ptr_area + ((uintptr_t)ptr_buffer - (uintptr_t)ptr_buffer_start);                         
			proc_unlock(p);
		}
#endif                    

		  
                                                                              
                           
     
		for (j = 0; j < descriptors[i].count; j++) {
			user_addr_t cur_address = string_area + (cur_string - imgp->ip_strings);

			                                                                              
			if (ptr_size == 8) {
				*(uint64_t *)ptr_buffer = (uint64_t)cur_address;
			} else {
				*(uint32_t *)ptr_buffer = (uint32_t)cur_address;
			}

			ptr_buffer = (void *)((uintptr_t)ptr_buffer + ptr_size);
			cur_string += strlen(cur_string) + 1;                                                  
		}

		if (descriptors[i].null_term) {
			if (ptr_size == 8) {
				*(uint64_t *)ptr_buffer = 0ULL;
			} else {
				*(uint32_t *)ptr_buffer = 0;
			}

			ptr_buffer = (void *)((uintptr_t)ptr_buffer + ptr_size);
		}
	}

	  
                                            
    
	error = copyout(ptr_buffer_start, ptr_area,
	    ptr_area_size);
	if (error) {
		goto bad;
	}

	                                             
	error = copyoutptr((user_addr_t)imgp->ip_argc, argc_area, ptr_size);
	if (error) {
		goto bad;
	}

bad:
	return error;
}


  
                       
  
                                                                        
                                                                      
                                                                  
                                                                 
                                                                     
                                                                   
                                                                
                                    
  
                                                              
  
                       
                       
  
                    
                                                
                                                          
                                                    
                                                                    
  
  
                                                                     
                                     
   
__attribute__((noinline))
static int
exec_extract_strings(struct image_params *imgp)
{
	int error = 0;
	int     ptr_size = (imgp->ip_flags & IMGPF_WAS_64BIT_ADDR) ? 8 : 4;
	int new_ptr_size = (imgp->ip_flags & IMGPF_IS_64BIT_ADDR) ? 8 : 4;
	user_addr_t     argv = imgp->ip_user_argv;
	user_addr_t     envv = imgp->ip_user_envv;

	  
                                                                      
                                                                      
                                             
    
	while (imgp->ip_strspace % new_ptr_size != 0) {
		*imgp->ip_strendp++ = '\0';
		imgp->ip_strspace--;
		                                                              
	}

	  
                                                                 
    
	imgp->ip_startargv = imgp->ip_strendp;
	imgp->ip_argc = 0;

	if ((imgp->ip_flags & IMGPF_INTERPRET) != 0) {
		user_addr_t     arg;
		char *argstart, *ch;

		                                                                          
		argstart = imgp->ip_interp_buffer;
		while (argstart) {
			ch = argstart;
			while (*ch && !IS_WHITESPACE(*ch)) {
				ch++;
			}

			if (*ch == '\0') {
				                                             
				error = exec_add_user_string(imgp, CAST_USER_ADDR_T(argstart), UIO_SYSSPACE, TRUE);
				argstart = NULL;
			} else {
				                   
				*ch = '\0';
				error = exec_add_user_string(imgp, CAST_USER_ADDR_T(argstart), UIO_SYSSPACE, TRUE);

				  
                                                                                 
                     
       
				argstart = ch + 1;
				while (IS_WHITESPACE(*argstart)) {
					argstart++;
				}
			}

			                                                                                
			if (error) {
				goto bad;
			}
			if (imgp->ip_argspace < new_ptr_size) {
				error = E2BIG;
				goto bad;
			}
			imgp->ip_argspace -= new_ptr_size;                           
			imgp->ip_argc++;
		}

		if (argv != 0LL) {
			  
                                                                  
                                                    
                                                                   
                                       
      
			error = copyinptr(argv, &arg, ptr_size);
			if (error) {
				goto bad;
			}
			if (arg != 0LL) {
				argv += ptr_size;                            
			}
		}

		if (imgp->ip_interp_sugid_fd != -1) {
			char temp[19];                                   
			snprintf(temp, sizeof(temp), "/dev/fd/%d", imgp->ip_interp_sugid_fd);
			error = exec_add_user_string(imgp, CAST_USER_ADDR_T(temp), UIO_SYSSPACE, TRUE);
		} else {
			error = exec_add_user_string(imgp, imgp->ip_user_fname, imgp->ip_seg, TRUE);
		}

		if (error) {
			goto bad;
		}
		if (imgp->ip_argspace < new_ptr_size) {
			error = E2BIG;
			goto bad;
		}
		imgp->ip_argspace -= new_ptr_size;                           
		imgp->ip_argc++;
	}

	while (argv != 0LL) {
		user_addr_t     arg;

		error = copyinptr(argv, &arg, ptr_size);
		if (error) {
			goto bad;
		}

		if (arg == 0LL) {
			break;
		}

		argv += ptr_size;

		  
                      
     
		error = exec_add_user_string(imgp, arg, imgp->ip_seg, TRUE);
		if (error) {
			goto bad;
		}
		if (imgp->ip_argspace < new_ptr_size) {
			error = E2BIG;
			goto bad;
		}
		imgp->ip_argspace -= new_ptr_size;                           
		imgp->ip_argc++;
	}

	                                           
	if (imgp->ip_argspace < new_ptr_size) {
		error = E2BIG;
		goto bad;
	}
	imgp->ip_argspace -= new_ptr_size;

	                                              
	imgp->ip_endargv = imgp->ip_strendp;
	imgp->ip_envc = 0;

	                              
	while (envv != 0LL) {
		user_addr_t     env;

		error = copyinptr(envv, &env, ptr_size);
		if (error) {
			goto bad;
		}

		envv += ptr_size;
		if (env == 0LL) {
			break;
		}
		  
                      
     
		error = exec_add_user_string(imgp, env, imgp->ip_seg, TRUE);
		if (error) {
			goto bad;
		}
		if (imgp->ip_argspace < new_ptr_size) {
			error = E2BIG;
			goto bad;
		}
		imgp->ip_argspace -= new_ptr_size;                           
		imgp->ip_envc++;
	}

	                                           
	if (imgp->ip_argspace < new_ptr_size) {
		error = E2BIG;
		goto bad;
	}
	imgp->ip_argspace -= new_ptr_size;

	                                                   
	while (imgp->ip_strspace % new_ptr_size != 0) {
		if (imgp->ip_argspace < 1) {
			error = E2BIG;
			goto bad;
		}
		*imgp->ip_strendp++ = '\0';
		imgp->ip_strspace--;
		imgp->ip_argspace--;
	}

	                                                 
	imgp->ip_endenvv = imgp->ip_strendp;

	  
                                                   
                         
    

bad:
	return error;
}

  
                                                                            
                                                                           
                                                                             
                                 
   
#define GUARD_VALUES 1
#define GUARD_KEY "stack_guard="

  
                                                           
   
#define ENTROPY_VALUES 2
#define ENTROPY_KEY "malloc_entropy="

  
                                                                               
   
#define PTR_MUNGE_VALUES 1
#define PTR_MUNGE_KEY "ptr_munge="

  
                                            
   
#define NANO_ENGAGE_KEY "MallocNanoZone=1"
  
                                                 
   
#define LIBMALLOC_EXPERIMENT_FACTORS_KEY "MallocExperiment="

#define PFZ_KEY "pfz="
extern user32_addr_t commpage_text32_location;
extern user64_addr_t commpage_text64_location;

extern uuid_string_t bootsessionuuid_string;

#define MAIN_STACK_VALUES 4
#define MAIN_STACK_KEY "main_stack="

#define FSID_KEY "executable_file="
#define DYLD_FSID_KEY "dyld_file="
#define CDHASH_KEY "executable_cdhash="
#define DYLD_FLAGS_KEY "dyld_flags="
#define SUBSYSTEM_ROOT_PATH_KEY "subsystem_root_path="
#define APP_BOOT_SESSION_KEY "executable_boothash="
#if __has_feature(ptrauth_calls)
#define PTRAUTH_DISABLED_FLAG "ptrauth_disabled=1"
#define DYLD_ARM64E_ABI_KEY "arm64e_abi="
#endif                                   
#define MAIN_TH_PORT_KEY "th_port="

#define FSID_MAX_STRING "0x1234567890abcdef,0x1234567890abcdef"

#define HEX_STR_LEN 18                                         
#define HEX_STR_LEN32 10                                 

#if XNU_TARGET_OS_OSX && _POSIX_SPAWN_FORCE_4K_PAGES && PMAP_CREATE_FORCE_4K_PAGES
#define VM_FORCE_4K_PAGES_KEY "vm_force_4k_pages=1"
#endif                                                                                     

static int
exec_add_entropy_key(struct image_params *imgp,
    const char *key,
    int values,
    boolean_t embedNUL)
{
	const int limit = 8;
	uint64_t entropy[limit];
	char str[strlen(key) + (HEX_STR_LEN + 1) * limit + 1];
	if (values > limit) {
		values = limit;
	}

	read_random(entropy, sizeof(entropy[0]) * values);

	if (embedNUL) {
		entropy[0] &= ~(0xffull << 8);
	}

	int len = scnprintf(str, sizeof(str), "%s0x%llx", key, entropy[0]);
	size_t remaining = sizeof(str) - len;
	for (int i = 1; i < values && remaining > 0; ++i) {
		size_t start = sizeof(str) - remaining;
		len = scnprintf(&str[start], remaining, ",0x%llx", entropy[i]);
		remaining -= len;
	}

	return exec_add_user_string(imgp, CAST_USER_ADDR_T(str), UIO_SYSSPACE, FALSE);
}

  
                                                     
   
#if (DEVELOPMENT || DEBUG)
extern uint64_t dyld_flags;
#endif

#if __has_feature(ptrauth_calls)
static inline bool
is_arm64e_running_as_arm64(const struct image_params *imgp)
{
	return (imgp->ip_origcpusubtype & ~CPU_SUBTYPE_MASK) == CPU_SUBTYPE_ARM64E &&
	       (imgp->ip_flags & IMGPF_NOJOP);
}
#endif                                   

_Atomic uint64_t libmalloc_experiment_factors = 0;

static int
exec_add_apple_strings(struct image_params *imgp,
    const load_result_t *load_result)
{
	int error;
	int img_ptr_size = (imgp->ip_flags & IMGPF_IS_64BIT_ADDR) ? 8 : 4;
	thread_t new_thread;
	ipc_port_t sright;
	uint64_t local_experiment_factors = 0;

	                                            
	imgp->ip_applec = 1;

	                           
	{
		char pfz_string[strlen(PFZ_KEY) + HEX_STR_LEN + 1];

		if (img_ptr_size == 8) {
			__assert_only size_t ret = snprintf(pfz_string, sizeof(pfz_string), PFZ_KEY "0x%llx", commpage_text64_location);
			assert(ret < sizeof(pfz_string));
		} else {
			snprintf(pfz_string, sizeof(pfz_string), PFZ_KEY "0x%x", commpage_text32_location);
		}
		error = exec_add_user_string(imgp, CAST_USER_ADDR_T(pfz_string), UIO_SYSSPACE, FALSE);
		if (error) {
			printf("Failed to add the pfz string with error %d\n", error);
			goto bad;
		}
		imgp->ip_applec++;
	}

	                                    
	if (imgp->ip_px_sa) {
		int proc_flags = (((struct _posix_spawnattr *) imgp->ip_px_sa)->psa_flags);

		if ((proc_flags & _POSIX_SPAWN_NANO_ALLOCATOR) == _POSIX_SPAWN_NANO_ALLOCATOR) {
			const char *nano_string = NANO_ENGAGE_KEY;
			error = exec_add_user_string(imgp, CAST_USER_ADDR_T(nano_string), UIO_SYSSPACE, FALSE);
			if (error) {
				goto bad;
			}
			imgp->ip_applec++;
		}
	}

	  
                                                              
                                   
   
                                                                    
                                                                     
    
	error = exec_add_entropy_key(imgp, GUARD_KEY, GUARD_VALUES, TRUE);
	if (error) {
		goto bad;
	}
	imgp->ip_applec++;

	  
                                               
    
	error = exec_add_entropy_key(imgp, ENTROPY_KEY, ENTROPY_VALUES, FALSE);
	if (error) {
		goto bad;
	}
	imgp->ip_applec++;

	  
                                                                          
                
    
	error = exec_add_entropy_key(imgp, PTR_MUNGE_KEY, PTR_MUNGE_VALUES, FALSE);
	if (error) {
		goto bad;
	}
	imgp->ip_applec++;

	  
                                                                          
                                            
   
                                                                   
                                                         
    
	if (load_result->unixproc) {
		char stack_string[strlen(MAIN_STACK_KEY) + (HEX_STR_LEN + 1) * MAIN_STACK_VALUES + 1];
		snprintf(stack_string, sizeof(stack_string),
		    MAIN_STACK_KEY "0x%llx,0x%llx,0x%llx,0x%llx",
		    (uint64_t)load_result->user_stack,
		    (uint64_t)load_result->user_stack_size,
		    (uint64_t)load_result->user_stack_alloc,
		    (uint64_t)load_result->user_stack_alloc_size);
		error = exec_add_user_string(imgp, CAST_USER_ADDR_T(stack_string), UIO_SYSSPACE, FALSE);
		if (error) {
			goto bad;
		}
		imgp->ip_applec++;
	}

	if (imgp->ip_vattr) {
		uint64_t fsid    = vnode_get_va_fsid(imgp->ip_vattr);
		uint64_t fsobjid = imgp->ip_vattr->va_fileid;

		char fsid_string[strlen(FSID_KEY) + strlen(FSID_MAX_STRING) + 1];
		snprintf(fsid_string, sizeof(fsid_string),
		    FSID_KEY "0x%llx,0x%llx", fsid, fsobjid);
		error = exec_add_user_string(imgp, CAST_USER_ADDR_T(fsid_string), UIO_SYSSPACE, FALSE);
		if (error) {
			goto bad;
		}
		imgp->ip_applec++;
	}

	if (imgp->ip_dyld_fsid || imgp->ip_dyld_fsobjid) {
		char fsid_string[strlen(DYLD_FSID_KEY) + strlen(FSID_MAX_STRING) + 1];
		snprintf(fsid_string, sizeof(fsid_string),
		    DYLD_FSID_KEY "0x%llx,0x%llx", imgp->ip_dyld_fsid, imgp->ip_dyld_fsobjid);
		error = exec_add_user_string(imgp, CAST_USER_ADDR_T(fsid_string), UIO_SYSSPACE, FALSE);
		if (error) {
			goto bad;
		}
		imgp->ip_applec++;
	}

	uint8_t cdhash[SHA1_RESULTLEN];
	int cdhash_errror = ubc_cs_getcdhash(imgp->ip_vp, imgp->ip_arch_offset, cdhash);
	if (cdhash_errror == 0) {
		char hash_string[strlen(CDHASH_KEY) + 2 * SHA1_RESULTLEN + 1];
		strncpy(hash_string, CDHASH_KEY, sizeof(hash_string));
		char *p = hash_string + sizeof(CDHASH_KEY) - 1;
		for (int i = 0; i < SHA1_RESULTLEN; i++) {
			snprintf(p, 3, "%02x", (int) cdhash[i]);
			p += 2;
		}
		error = exec_add_user_string(imgp, CAST_USER_ADDR_T(hash_string), UIO_SYSSPACE, FALSE);
		if (error) {
			goto bad;
		}
		imgp->ip_applec++;

		                                                 
		uint8_t sha_digest[SHA256_DIGEST_LENGTH];
		SHA256_CTX sha_ctx;
		SHA256_Init(&sha_ctx);
		SHA256_Update(&sha_ctx, bootsessionuuid_string, sizeof(bootsessionuuid_string));
		SHA256_Update(&sha_ctx, cdhash, sizeof(cdhash));
		SHA256_Final(sha_digest, &sha_ctx);
		char app_boot_string[strlen(APP_BOOT_SESSION_KEY) + 2 * SHA1_RESULTLEN + 1];
		strncpy(app_boot_string, APP_BOOT_SESSION_KEY, sizeof(app_boot_string));
		char *s = app_boot_string + sizeof(APP_BOOT_SESSION_KEY) - 1;
		for (int i = 0; i < SHA1_RESULTLEN; i++) {
			snprintf(s, 3, "%02x", (int) sha_digest[i]);
			s += 2;
		}
		error = exec_add_user_string(imgp, CAST_USER_ADDR_T(app_boot_string), UIO_SYSSPACE, FALSE);
		if (error) {
			goto bad;
		}
		imgp->ip_applec++;
	}
#if (DEVELOPMENT || DEBUG)
	if (dyld_flags) {
		char dyld_flags_string[strlen(DYLD_FLAGS_KEY) + HEX_STR_LEN + 1];
		snprintf(dyld_flags_string, sizeof(dyld_flags_string), DYLD_FLAGS_KEY "0x%llx", dyld_flags);
		error = exec_add_user_string(imgp, CAST_USER_ADDR_T(dyld_flags_string), UIO_SYSSPACE, FALSE);
		if (error) {
			goto bad;
		}
		imgp->ip_applec++;
	}
#endif
	if (imgp->ip_subsystem_root_path) {
		size_t buffer_len = MAXPATHLEN + strlen(SUBSYSTEM_ROOT_PATH_KEY);
		char subsystem_root_path_string[buffer_len];
		int required_len = snprintf(subsystem_root_path_string, buffer_len, SUBSYSTEM_ROOT_PATH_KEY "%s", imgp->ip_subsystem_root_path);

		if (((size_t)required_len >= buffer_len) || (required_len < 0)) {
			error = ENAMETOOLONG;
			goto bad;
		}

		error = exec_add_user_string(imgp, CAST_USER_ADDR_T(subsystem_root_path_string), UIO_SYSSPACE, FALSE);
		if (error) {
			goto bad;
		}

		imgp->ip_applec++;
	}
#if __has_feature(ptrauth_calls)
	if (is_arm64e_running_as_arm64(imgp)) {
		error = exec_add_user_string(imgp, CAST_USER_ADDR_T(PTRAUTH_DISABLED_FLAG), UIO_SYSSPACE, FALSE);
		if (error) {
			goto bad;
		}

		imgp->ip_applec++;
	}
#endif                                   


#if __has_feature(ptrauth_calls) && defined(XNU_TARGET_OS_OSX)
	{
		char dyld_abi_string[strlen(DYLD_ARM64E_ABI_KEY) + 8];
		strlcpy(dyld_abi_string, DYLD_ARM64E_ABI_KEY, sizeof(dyld_abi_string));
		bool allowAll = bootarg_arm64e_preview_abi;
		strlcat(dyld_abi_string, (allowAll ? "all" : "os"), sizeof(dyld_abi_string));
		error = exec_add_user_string(imgp, CAST_USER_ADDR_T(dyld_abi_string), UIO_SYSSPACE, FALSE);
		if (error) {
			goto bad;
		}

		imgp->ip_applec++;
	}
#endif
	  
                                  
                                                                                           
                                                                                         
                       
    
	if ((new_thread = imgp->ip_new_thread) != THREAD_NULL) {
		thread_reference(new_thread);
		sright = convert_thread_to_port_pinned(new_thread);
		task_t new_task = get_threadtask(new_thread);
		mach_port_name_t name = ipc_port_copyout_send(sright, get_task_ipcspace(new_task));
		char port_name_hex_str[strlen(MAIN_TH_PORT_KEY) + HEX_STR_LEN32 + 1];
		snprintf(port_name_hex_str, sizeof(port_name_hex_str), MAIN_TH_PORT_KEY "0x%x", name);

		error = exec_add_user_string(imgp, CAST_USER_ADDR_T(port_name_hex_str), UIO_SYSSPACE, FALSE);
		if (error) {
			goto bad;
		}
		imgp->ip_applec++;
	}

#if XNU_TARGET_OS_OSX && _POSIX_SPAWN_FORCE_4K_PAGES && PMAP_CREATE_FORCE_4K_PAGES
	if (imgp->ip_px_sa != NULL) {
		struct _posix_spawnattr* psa = (struct _posix_spawnattr *) imgp->ip_px_sa;
		if (psa->psa_flags & _POSIX_SPAWN_FORCE_4K_PAGES) {
			const char *vm_force_4k_string = VM_FORCE_4K_PAGES_KEY;
			error = exec_add_user_string(imgp, CAST_USER_ADDR_T(vm_force_4k_string), UIO_SYSSPACE, FALSE);
			if (error) {
				goto bad;
			}
			imgp->ip_applec++;
		}
	}
#endif                                                                                     

	                                            
	local_experiment_factors = os_atomic_load_wide(&libmalloc_experiment_factors, relaxed);
	if (__improbable(local_experiment_factors != 0)) {
		char libmalloc_experiment_factors_string[strlen(LIBMALLOC_EXPERIMENT_FACTORS_KEY) + HEX_STR_LEN + 1];

		snprintf(
			libmalloc_experiment_factors_string,
			sizeof(libmalloc_experiment_factors_string),
			LIBMALLOC_EXPERIMENT_FACTORS_KEY "0x%llx",
			local_experiment_factors);
		error = exec_add_user_string(
			imgp,
			CAST_USER_ADDR_T(libmalloc_experiment_factors_string),
			UIO_SYSSPACE,
			FALSE);
		if (error) {
			printf("Failed to add the libmalloc experiment factors string with error %d\n", error);
			goto bad;
		}
		imgp->ip_applec++;
	}

	                                                
	while (imgp->ip_strspace % img_ptr_size != 0) {
		*imgp->ip_strendp++ = '\0';
		imgp->ip_strspace--;
	}

bad:
	return error;
}

  
                         
  
                                                                           
                                                            
                                                 
  
                                                              
  
                       
                              
                                          
                                                   
                    
                      
   
static int
exec_check_permissions(struct image_params *imgp)
{
	struct vnode *vp = imgp->ip_vp;
	struct vnode_attr *vap = imgp->ip_vattr;
	proc_t p = vfs_context_proc(imgp->ip_vfs_context);
	int error;
	kauth_action_t action;

	                                           
	if (!vnode_isreg(vp)) {
		return EACCES;
	}

	                                                                      
	VATTR_INIT(vap);
	VATTR_WANTED(vap, va_uid);
	VATTR_WANTED(vap, va_gid);
	VATTR_WANTED(vap, va_mode);
	VATTR_WANTED(vap, va_fsid);
	VATTR_WANTED(vap, va_fsid64);
	VATTR_WANTED(vap, va_fileid);
	VATTR_WANTED(vap, va_data_size);
	if ((error = vnode_getattr(vp, vap, imgp->ip_vfs_context)) != 0) {
		return error;
	}

	  
                                                               
                                                               
                              
    
	if (!vfs_authopaque(vnode_mount(vp)) && ((vap->va_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0)) {
		return EACCES;
	}

	                                
	if (vap->va_data_size == 0) {
		return ENOEXEC;
	}

	imgp->ip_arch_offset = (user_size_t)0;
#if __LP64__
	imgp->ip_arch_size = vap->va_data_size;
#else
	if (vap->va_data_size > UINT32_MAX) {
		return ENOEXEC;
	}
	imgp->ip_arch_size = (user_size_t)vap->va_data_size;
#endif

	                                                              
	if ((vp->v_mount->mnt_flag & MNT_NOSUID) || (p->p_lflag & P_LTRACED)) {
		vap->va_mode &= ~(VSUID | VSGID);
	}

	  
                                                                      
                                     
    
	if (vap->va_mode & (VSUID | VSGID)) {
		imgp->ip_flags &= ~(IMGPF_ALLOW_DATA_EXEC | IMGPF_DISABLE_ASLR);
	}

#if CONFIG_MACF
	error = mac_vnode_check_exec(imgp->ip_vfs_context, vp, imgp);
	if (error) {
		return error;
	}
#endif

	                                  
	action = KAUTH_VNODE_EXECUTE;
	                                         
	if (p->p_lflag & P_LTRACED) {
		action |= KAUTH_VNODE_READ_DATA;
	}
	if ((error = vnode_authorize(vp, NULL, action, imgp->ip_vfs_context)) != 0) {
		return error;
	}

#if 0
	                                                        
	vnode_lock(vp);
	if (vp->v_writecount) {
		panic("going to return ETXTBSY %x", vp);
		vnode_unlock(vp);
		return ETXTBSY;
	}
	vnode_unlock(vp);
#endif

	                                                                  

	return error;
}


  
                    
  
                                                                           
                                                                    
  
                                                                            
                                                                        
                                                    
  
                                                                             
                                                 
  
                                                                          
                                                                  
  
                                                              
  
                                        
  
                    
                                                      
                                    
                                                     
                                         
   
__attribute__((noinline))
static int
exec_handle_sugid(struct image_params *imgp)
{
	proc_t                  p = vfs_context_proc(imgp->ip_vfs_context);
	kauth_cred_t            cred = vfs_context_ucred(imgp->ip_vfs_context);
	int                     i;
	int                     leave_sugid_clear = 0;
	int                     mac_reset_ipc = 0;
	int                     error = 0;
	task_t                  task = NULL;
#if CONFIG_MACF
	int                     mac_transition, disjoint_cred = 0;
	int             label_update_return = 0;

	  
                                                                       
                        
   
                                                             
                                                      
                                                          
                                                      
    
	mac_transition = mac_cred_check_label_update_execve(
		imgp->ip_vfs_context,
		imgp->ip_vp,
		imgp->ip_arch_offset,
		imgp->ip_scriptvp,
		imgp->ip_scriptlabelp,
		imgp->ip_execlabelp,
		p,
		imgp->ip_px_smpx);
#endif

	OSBitAndAtomic(~((uint32_t)P_SUGID), &p->p_flag);

	  
                                                                   
                                                                   
                                                                     
                                                               
                                                                    
             
   
                                                        
                                                       
                                                      
                                                         
                 
    
	if (((imgp->ip_origvattr->va_mode & VSUID) != 0 &&
	    kauth_cred_getuid(cred) != imgp->ip_origvattr->va_uid) ||
	    ((imgp->ip_origvattr->va_mode & VSGID) != 0 &&
	    ((kauth_cred_ismember_gid(cred, imgp->ip_origvattr->va_gid, &leave_sugid_clear) || !leave_sugid_clear) ||
	    (kauth_cred_getgid(cred) != imgp->ip_origvattr->va_gid))) ||
	    (imgp->ip_sc_port != NULL)) {
#if CONFIG_MACF
                                                          
handle_mac_transition:
#endif

#if CONFIG_SETUID
		  
                                                            
                 
    
                                                        
                                                   
                                                
                                                  
                                                
                                                
                                                 
                                             
                                                
                                                
                             
    
                                                       
                                                           
                          
     

		if (imgp->ip_sc_port != NULL) {
			extern int suid_cred_verify(ipc_port_t, vnode_t, uint32_t *);
			int ret = -1;
			uid_t uid = UINT32_MAX;

			  
                                                       
                                                       
                    
      
			struct vnode *vp = imgp->ip_scriptvp != NULL ? imgp->ip_scriptvp : imgp->ip_vp;

			ret = suid_cred_verify(imgp->ip_sc_port, vp, &uid);
			if (ret == 0) {
				apply_kauth_cred_update(p, ^kauth_cred_t (kauth_cred_t my_cred) {
					return kauth_cred_setresuid(my_cred,
					KAUTH_UID_NONE,
					uid,
					uid,
					KAUTH_UID_NONE);
				});
			} else {
				error = EPERM;
			}
		}

		if (imgp->ip_origvattr->va_mode & VSUID) {
			apply_kauth_cred_update(p, ^kauth_cred_t (kauth_cred_t my_cred) {
				return kauth_cred_setresuid(my_cred,
				KAUTH_UID_NONE,
				imgp->ip_origvattr->va_uid,
				imgp->ip_origvattr->va_uid,
				KAUTH_UID_NONE);
			});
		}

		if (imgp->ip_origvattr->va_mode & VSGID) {
			apply_kauth_cred_update(p, ^kauth_cred_t (kauth_cred_t my_cred) {
				return kauth_cred_setresgid(my_cred,
				KAUTH_GID_NONE,
				imgp->ip_origvattr->va_gid,
				imgp->ip_origvattr->va_gid);
			});
		}
#endif                    

#if CONFIG_MACF
		  
                                                                 
                                                            
                                                        
                                     
     
		if (mac_transition) {
			  
                                                 
                                          
                
      
			thread_t thread = current_thread();
			thread_enable_send_importance(thread, TRUE);
			kauth_proc_label_update_execve(p,
			    imgp->ip_vfs_context,
			    imgp->ip_vp,
			    imgp->ip_arch_offset,
			    imgp->ip_scriptvp,
			    imgp->ip_scriptlabelp,
			    imgp->ip_execlabelp,
			    &imgp->ip_csflags,
			    imgp->ip_px_smpx,
			    &disjoint_cred,                                                       
			    &label_update_return);
			thread_enable_send_importance(thread, FALSE);

			if (disjoint_cred) {
				  
                                              
                                                
                                          
                                             
                                                 
                                                  
                  
       
				leave_sugid_clear = 0;
			}

			imgp->ip_mac_return = label_update_return;
		}

		mac_reset_ipc = mac_proc_check_inherit_ipc_ports(p, p->p_textvp, p->p_textoff, imgp->ip_vp, imgp->ip_arch_offset, imgp->ip_scriptvp);

#endif                   

		  
                                                           
                                                            
                                                           
                                                           
                                                             
                                                            
                                              
     
		if (mac_reset_ipc || !leave_sugid_clear) {
			  
                                                
                                                   
                                                    
                        
      
			ipc_task_reset((imgp->ip_new_thread != NULL) ?
			    get_threadtask(imgp->ip_new_thread) : p->task);
			ipc_thread_reset((imgp->ip_new_thread != NULL) ?
			    imgp->ip_new_thread : current_thread());
		}

		if (!leave_sugid_clear) {
			  
                                 
      
			OSBitOrAtomic(P_SUGID, &p->p_flag);

			  
                                             
                                                      
                                                      
                                                      
                                                         
              
      
			for (i = 0; i < 3; i++) {
				if (fp_get_noref_locked(p, i) != NULL) {
					continue;
				}

				  
                                  
      
                     
                                                       
                
                                                       
       

				struct fileproc *fp;
				int indx;
				int flag;
				struct nameidata *ndp = NULL;

				if (i == 0) {
					flag = FREAD;
				} else {
					flag = FWRITE;
				}

				if ((error = falloc(p,
				    &fp, &indx, imgp->ip_vfs_context)) != 0) {
					continue;
				}

				ndp = kheap_alloc(KHEAP_TEMP,
				    sizeof(*ndp), Z_WAITOK | Z_ZERO);
				if (ndp == NULL) {
					fp_free(p, indx, fp);
					error = ENOMEM;
					break;
				}

				NDINIT(ndp, LOOKUP, OP_OPEN, FOLLOW, UIO_SYSSPACE,
				    CAST_USER_ADDR_T("/dev/null"),
				    imgp->ip_vfs_context);

				if ((error = vn_open(ndp, flag, 0)) != 0) {
					fp_free(p, indx, fp);
					kheap_free(KHEAP_TEMP, ndp, sizeof(*ndp));
					break;
				}

				struct fileglob *fg = fp->fp_glob;

				fg->fg_flag = flag;
				fg->fg_ops = &vnops;
				fg->fg_data = ndp->ni_vp;

				vnode_put(ndp->ni_vp);

				proc_fdlock(p);
				procfdtbl_releasefd(p, indx, NULL);
				fp_drop(p, indx, fp, 1);
				proc_fdunlock(p);

				kheap_free(KHEAP_TEMP, ndp, sizeof(*ndp));
			}
		}
	}
#if CONFIG_MACF
	else {
		  
                                                             
                                                              
                                                            
                                                            
                                
     
		if (mac_transition) {
			leave_sugid_clear = 1;
			goto handle_mac_transition;
		}
	}

#endif                   

	  
                                                                    
                                                 
   
                                                      
                                                          
                         
    
	apply_kauth_cred_update(p, ^kauth_cred_t (kauth_cred_t my_cred) {
		return kauth_cred_setsvuidgid(my_cred,
		kauth_cred_getuid(my_cred),
		kauth_cred_getgid(my_cred));
	});

	                                                                     
	p->p_idversion = OSIncrementAtomic(&nextpidversion);

	if (imgp->ip_new_thread != NULL) {
		task = get_threadtask(imgp->ip_new_thread);
	} else {
		task = p->task;
	}
	set_security_token_task_internal(p, task);

	return error;
}


  
                    
  
                                                                          
                                                              
                                                             
                                                                
                                                               
                                                        
  
                                          
                                                      
                                                        
  
                                                    
                                    
   
__attribute__((noinline))
static kern_return_t
create_unix_stack(vm_map_t map, load_result_t* load_result,
    proc_t p)
{
	mach_vm_size_t          size, prot_size;
	mach_vm_offset_t        addr, prot_addr;
	kern_return_t           kr;

	mach_vm_address_t       user_stack = load_result->user_stack;

	proc_lock(p);
	p->user_stack = (uintptr_t)user_stack;
	if (load_result->custom_stack) {
		p->p_lflag |= P_LCUSTOM_STACK;
	}
	proc_unlock(p);
	if (vm_map_page_shift(map) < (int)PAGE_SHIFT) {
		DEBUG4K_LOAD("map %p user_stack 0x%llx custom %d user_stack_alloc_size 0x%llx\n", map, user_stack, load_result->custom_stack, load_result->user_stack_alloc_size);
	}

	if (load_result->user_stack_alloc_size > 0) {
		  
                                                        
                                                    
                                                          
                                                               
                                             
     
		if (mach_vm_round_page_overflow(load_result->user_stack_alloc_size, &size)) {
			return KERN_INVALID_ARGUMENT;
		}
		addr = vm_map_trunc_page(load_result->user_stack - size,
		    vm_map_page_mask(map));
		kr = mach_vm_allocate_kernel(map, &addr, size,
		    VM_FLAGS_FIXED, VM_MEMORY_STACK);
		if (kr != KERN_SUCCESS) {
			                                                   
			addr = 0;
			kr = mach_vm_allocate_kernel(map, &addr, size,
			    VM_FLAGS_ANYWHERE, VM_MEMORY_STACK);
			if (kr != KERN_SUCCESS) {
				return kr;
			}

			user_stack = addr + size;
			load_result->user_stack = (user_addr_t)user_stack;

			proc_lock(p);
			p->user_stack = (uintptr_t)user_stack;
			proc_unlock(p);
		}

		load_result->user_stack_alloc = (user_addr_t)addr;

		  
                                                         
                                 
     
		if (load_result->user_stack_size == 0) {
			load_result->user_stack_size = proc_limitgetcur(p, RLIMIT_STACK, TRUE);
			prot_size = vm_map_trunc_page(size - load_result->user_stack_size, vm_map_page_mask(map));
		} else {
			prot_size = PAGE_SIZE;
		}

		prot_addr = addr;
		kr = mach_vm_protect(map,
		    prot_addr,
		    prot_size,
		    FALSE,
		    VM_PROT_NONE);
		if (kr != KERN_SUCCESS) {
			(void)mach_vm_deallocate(map, addr, size);
			return kr;
		}
	}

	return KERN_SUCCESS;
}

#include <sys/reboot.h>

  
                            
  
                                                                              
  
                                                     
                         
                                          
                               
  
                                 
                                                                 
  
                                                                 
                                                             
                                                               
                          
  
                                                                 
                                                               
                                
   
static int
load_init_program_at_path(proc_t p, user_addr_t scratch_addr, const char* path)
{
	int retval[2];
	int error;
	struct execve_args init_exec_args;
	user_addr_t argv0 = USER_ADDR_NULL, argv1 = USER_ADDR_NULL;

	  
                                      
    
	assert(p);
	assert(scratch_addr);
	assert(path);

	  
                          
    
	size_t path_length = strlen(path) + 1;
	argv0 = scratch_addr;
	error = copyout(path, argv0, path_length);
	if (error) {
		return error;
	}

	scratch_addr = USER_ADDR_ALIGN(scratch_addr + path_length, sizeof(user_addr_t));

	  
                                                 
                                                         
    
	if (boothowto & RB_SINGLE) {
		const char *init_args = "-s";
		size_t init_args_length = strlen(init_args) + 1;

		argv1 = scratch_addr;
		error = copyout(init_args, argv1, init_args_length);
		if (error) {
			return error;
		}

		scratch_addr = USER_ADDR_ALIGN(scratch_addr + init_args_length, sizeof(user_addr_t));
	}

	if (proc_is64bit(p)) {
		user64_addr_t argv64bit[3] = {};

		argv64bit[0] = argv0;
		argv64bit[1] = argv1;
		argv64bit[2] = USER_ADDR_NULL;

		error = copyout(argv64bit, scratch_addr, sizeof(argv64bit));
		if (error) {
			return error;
		}
	} else {
		user32_addr_t argv32bit[3] = {};

		argv32bit[0] = (user32_addr_t)argv0;
		argv32bit[1] = (user32_addr_t)argv1;
		argv32bit[2] = USER_ADDR_NULL;

		error = copyout(argv32bit, scratch_addr, sizeof(argv32bit));
		if (error) {
			return error;
		}
	}

	  
                                                  
    
	init_exec_args.fname = argv0;
	init_exec_args.argp = scratch_addr;
	init_exec_args.envp = USER_ADDR_NULL;

	  
                                                 
    
	set_security_token(p);

	return execve(p, &init_exec_args, retval);
}

static const char * init_programs[] = {
#if DEBUG
	"/usr/appleinternal/sbin/launchd.debug",
#endif
#if DEVELOPMENT || DEBUG
	"/usr/appleinternal/sbin/launchd.development",
#endif
	"/sbin/launchd",
};

  
                    
  
                                                                              
  
                                                     
                         
  
                  
  
                                                                 
                                                             
                                                               
                          
  
                                                             
                                                                
                                                                   
                                            
  
                                      
  
                                  
                                                                                     
                                                           
                                                   
                                                        
                         
   
void
load_init_program(proc_t p)
{
	uint32_t i;
	int error;
	vm_map_t map = current_map();
	mach_vm_offset_t scratch_addr = 0;
	mach_vm_size_t map_page_size = vm_map_page_size(map);

	(void) mach_vm_allocate_kernel(map, &scratch_addr, map_page_size, VM_FLAGS_ANYWHERE, VM_KERN_MEMORY_NONE);
#if CONFIG_MEMORYSTATUS
	(void) memorystatus_init_at_boot_snapshot();
#endif                          

#if __has_feature(ptrauth_calls)
	PE_parse_boot_argn("vm_shared_region_per_team_id", &vm_shared_region_per_team_id, sizeof(vm_shared_region_per_team_id));
	PE_parse_boot_argn("vm_shared_region_by_entitlement", &vm_shared_region_by_entitlement, sizeof(vm_shared_region_by_entitlement));
	PE_parse_boot_argn("vm_shared_region_reslide_aslr", &vm_shared_region_reslide_aslr, sizeof(vm_shared_region_reslide_aslr));
	PE_parse_boot_argn("vm_shared_region_reslide_restrict", &vm_shared_region_reslide_restrict, sizeof(vm_shared_region_reslide_restrict));
#endif                                   

#if DEBUG || DEVELOPMENT
#if XNU_TARGET_OS_OSX
	PE_parse_boot_argn("unentitled_ios_sim_launch", &unentitled_ios_sim_launch, sizeof(unentitled_ios_sim_launch));
#endif                        

	                                     
	char launchd_suffix[64];
	if (PE_parse_boot_argn("launchdsuffix", launchd_suffix, sizeof(launchd_suffix))) {
		char launchd_path[128];
		boolean_t is_release_suffix = ((launchd_suffix[0] == 0) ||
		    (strcmp(launchd_suffix, "release") == 0));

		if (is_release_suffix) {
			printf("load_init_program: attempting to load /sbin/launchd\n");
			error = load_init_program_at_path(p, (user_addr_t)scratch_addr, "/sbin/launchd");
			if (!error) {
				return;
			}

			panic("Process 1 exec of launchd.release failed, errno %d", error);
		} else {
			strlcpy(launchd_path, "/usr/appleinternal/sbin/launchd.", sizeof(launchd_path));
			strlcat(launchd_path, launchd_suffix, sizeof(launchd_path));

			printf("load_init_program: attempting to load %s\n", launchd_path);
			error = load_init_program_at_path(p, (user_addr_t)scratch_addr, launchd_path);
			if (!error) {
				return;
			} else if (error != ENOENT) {
				printf("load_init_program: failed loading %s: errno %d\n", launchd_path, error);
			}
		}
	}
#endif

	error = ENOENT;
	for (i = 0; i < sizeof(init_programs) / sizeof(init_programs[0]); i++) {
		printf("load_init_program: attempting to load %s\n", init_programs[i]);
		error = load_init_program_at_path(p, (user_addr_t)scratch_addr, init_programs[i]);
		if (!error) {
			return;
		} else if (error != ENOENT) {
			printf("load_init_program: failed loading %s: errno %d\n", init_programs[i], error);
		}
	}

	panic("Process 1 exec of %s failed, errno %d", ((i == 0) ? "<null>" : init_programs[i - 1]), error);
}

  
                       
  
                                                                            
  
                                       
  
                                    
               
                              
                                   
                                          
                                            
                          
                                                
                       
                               
                                               
   
static int
load_return_to_errno(load_return_t lrtn)
{
	switch (lrtn) {
	case LOAD_SUCCESS:
		return 0;
	case LOAD_BADARCH:
		return EBADARCH;
	case LOAD_BADMACHO:
	case LOAD_BADMACHO_UPX:
		return EBADMACHO;
	case LOAD_SHLIB:
		return ESHLIBVERS;
	case LOAD_NOSPACE:
	case LOAD_RESOURCE:
		return ENOMEM;
	case LOAD_PROTECT:
		return EACCES;
	case LOAD_ENOENT:
		return ENOENT;
	case LOAD_IOERROR:
		return EIO;
	case LOAD_DECRYPTFAIL:
		return EAUTH;
	case LOAD_FAILURE:
	default:
		return EBADEXEC;
	}
}

#include <mach/mach_types.h>
#include <mach/vm_prot.h>
#include <mach/semaphore.h>
#include <mach/sync_policy.h>
#include <kern/clock.h>
#include <mach/kern_return.h>

  
                 
  
                                                                          
                                                               
                                
  
                                                              
  
                       
                             
                              
                                
                             
  
                                                                      
                                                               
                                                                 
                                                             
  
                                                             
                                                             
                                                             
                                                            
                                        
  
                                                                   
                                                                  
                                                                  
                                                                
                                 
   


static int execargs_waiters = 0;
static LCK_MTX_DECLARE_ATTR(execargs_cache_lock, &proc_lck_grp, &proc_lck_attr);

static void
execargs_lock_lock(void)
{
	lck_mtx_lock_spin(&execargs_cache_lock);
}

static void
execargs_lock_unlock(void)
{
	lck_mtx_unlock(&execargs_cache_lock);
}

static wait_result_t
execargs_lock_sleep(void)
{
	return lck_mtx_sleep(&execargs_cache_lock, LCK_SLEEP_DEFAULT, &execargs_free_count, THREAD_INTERRUPTIBLE);
}

static kern_return_t
execargs_purgeable_allocate(char **execarg_address)
{
	kern_return_t kr = vm_allocate_kernel(bsd_pageable_map, (vm_offset_t *)execarg_address, BSD_PAGEABLE_SIZE_PER_EXEC, VM_FLAGS_ANYWHERE | VM_FLAGS_PURGABLE, VM_KERN_MEMORY_NONE);
	assert(kr == KERN_SUCCESS);
	return kr;
}

static kern_return_t
execargs_purgeable_reference(void *execarg_address)
{
	int state = VM_PURGABLE_NONVOLATILE;
	kern_return_t kr = vm_purgable_control(bsd_pageable_map, (vm_offset_t) execarg_address, VM_PURGABLE_SET_STATE, &state);

	assert(kr == KERN_SUCCESS);
	return kr;
}

static kern_return_t
execargs_purgeable_volatilize(void *execarg_address)
{
	int state = VM_PURGABLE_VOLATILE | VM_PURGABLE_ORDERING_OBSOLETE;
	kern_return_t kr;
	kr = vm_purgable_control(bsd_pageable_map, (vm_offset_t) execarg_address, VM_PURGABLE_SET_STATE, &state);

	assert(kr == KERN_SUCCESS);

	return kr;
}

static void
execargs_wakeup_waiters(void)
{
	thread_wakeup(&execargs_free_count);
}

static int
execargs_alloc(struct image_params *imgp)
{
	kern_return_t kret;
	wait_result_t res;
	int i, cache_index = -1;

	execargs_lock_lock();

	while (execargs_free_count == 0) {
		execargs_waiters++;
		res = execargs_lock_sleep();
		execargs_waiters--;
		if (res != THREAD_AWAKENED) {
			execargs_lock_unlock();
			return EINTR;
		}
	}

	execargs_free_count--;

	for (i = 0; i < execargs_cache_size; i++) {
		vm_offset_t element = execargs_cache[i];
		if (element) {
			cache_index = i;
			imgp->ip_strings = (char *)(execargs_cache[i]);
			execargs_cache[i] = 0;
			break;
		}
	}

	assert(execargs_free_count >= 0);

	execargs_lock_unlock();

	if (cache_index == -1) {
		kret = execargs_purgeable_allocate(&imgp->ip_strings);
	} else {
		kret = execargs_purgeable_reference(imgp->ip_strings);
	}

	assert(kret == KERN_SUCCESS);
	if (kret != KERN_SUCCESS) {
		return ENOMEM;
	}

	                                            
	imgp->ip_vdata = imgp->ip_strings + (NCARGS + PAGE_SIZE);
	imgp->ip_strendp = imgp->ip_strings;
	imgp->ip_argspace = NCARGS;
	imgp->ip_strspace = (NCARGS + PAGE_SIZE);

	return 0;
}

  
                
  
                                                                             
                                                                   
                     
  
                                                              
  
                       
                             
                                
   
static int
execargs_free(struct image_params *imgp)
{
	kern_return_t kret;
	int i;
	boolean_t needs_wakeup = FALSE;

	kret = execargs_purgeable_volatilize(imgp->ip_strings);

	execargs_lock_lock();
	execargs_free_count++;

	for (i = 0; i < execargs_cache_size; i++) {
		vm_offset_t element = execargs_cache[i];
		if (element == 0) {
			execargs_cache[i] = (vm_offset_t) imgp->ip_strings;
			imgp->ip_strings = NULL;
			break;
		}
	}

	assert(imgp->ip_strings == NULL);

	if (execargs_waiters > 0) {
		needs_wakeup = TRUE;
	}

	execargs_lock_unlock();

	if (needs_wakeup == TRUE) {
		execargs_wakeup_waiters();
	}

	return kret == KERN_SUCCESS ? 0 : EINVAL;
}

static void
exec_resettextvp(proc_t p, struct image_params *imgp)
{
	vnode_t vp;
	off_t offset;
	vnode_t tvp  = p->p_textvp;
	int ret;

	vp = imgp->ip_vp;
	offset = imgp->ip_arch_offset;

	if (vp == NULLVP) {
		panic("exec_resettextvp: expected valid vp");
	}

	ret = vnode_ref(vp);
	proc_lock(p);
	if (ret == 0) {
		p->p_textvp = vp;
		p->p_textoff = offset;
	} else {
		p->p_textvp = NULLVP;                         
		p->p_textoff = 0;
	}
	proc_unlock(p);

	if (tvp != NULLVP) {
		if (vnode_getwithref(tvp) == 0) {
			vnode_rele(tvp);
			vnode_put(tvp);
		}
	}
}

                                                           
static const size_t CS_CDHASH_STRING_SIZE = CS_CDHASH_LEN * 2 + 1;

static void
cdhash_to_string(char str[CS_CDHASH_STRING_SIZE], uint8_t const * const cdhash)
{
	static char const nibble[] = "0123456789abcdef";

	                                                              
                   
                                                                         
                                                                
    
	for (int i = 0; i < CS_CDHASH_LEN; ++i) {
		str[i * 2] = nibble[(cdhash[i] & 0xf0) >> 4];
		str[i * 2 + 1] = nibble[cdhash[i] & 0x0f];
	}
	str[CS_CDHASH_STRING_SIZE - 1] = 0;
}

  
                                                      
  
                                                                        
                                                                      
                                                  
   
__attribute__((noinline)) int
__EXEC_WAITING_ON_TASKGATED_CODE_SIGNATURE_UPCALL__(mach_port_t task_access_port, int32_t new_pid)
{
	return find_code_signature(task_access_port, new_pid);
}

static int
check_for_signature(proc_t p, struct image_params *imgp)
{
	mach_port_t port = IPC_PORT_NULL;
	kern_return_t kr = KERN_FAILURE;
	int error = EACCES;
	boolean_t unexpected_failure = FALSE;
	struct cs_blob *csb;
	boolean_t require_success = FALSE;
	int spawn = (imgp->ip_flags & IMGPF_SPAWN);
	int vfexec = (imgp->ip_flags & IMGPF_VFORK_EXEC);
	os_reason_t signature_failure_reason = OS_REASON_NULL;

	  
                                                  
                                                   
          
    
	proc_lock(p);
	p->p_csflags = imgp->ip_csflags;
	proc_unlock(p);

	                                            
	if (p->p_csflags & (CS_HARD | CS_KILL)) {
		vm_map_switch_protect(get_task_map(p->task), TRUE);
	}
	                                          
	if (p->p_csflags & CS_ENFORCEMENT) {
		vm_map_cs_enforcement_set(get_task_map(p->task), TRUE);
	} else {
		vm_map_cs_enforcement_set(get_task_map(p->task), FALSE);
	}

	  
                                                
                                                       
                                                 
    
	if (imgp->ip_mac_return != 0) {
		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
		    p->p_pid, OS_REASON_EXEC, EXEC_EXIT_REASON_SECURITY_POLICY, 0, 0);
		signature_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_SECURITY_POLICY);
		error = imgp->ip_mac_return;
		unexpected_failure = TRUE;
		goto done;
	}

	if (imgp->ip_cs_error != OS_REASON_NULL) {
		signature_failure_reason = imgp->ip_cs_error;
		imgp->ip_cs_error = OS_REASON_NULL;
		error = EACCES;
		goto done;
	}

#if XNU_TARGET_OS_OSX
	                                                                            
	if (proc_platform(p) == PLATFORM_IOS) {
		struct _posix_spawnattr *psa = (struct _posix_spawnattr *) imgp->ip_px_sa;
		if (psa == NULL || psa->psa_platform == 0) {
			boolean_t no_sandbox_entitled = FALSE;
#if DEBUG || DEVELOPMENT
			  
                                                     
                                                                       
                          
      
			if (unentitled_ios_sim_launch) {
				no_sandbox_entitled = TRUE;
			} else {
				no_sandbox_entitled = IOVnodeHasEntitlement(imgp->ip_vp,
				    (int64_t)imgp->ip_arch_offset, "com.apple.private.security.no-sandbox");
			}
#endif                           
			if (!no_sandbox_entitled) {
				signature_failure_reason = os_reason_create(OS_REASON_EXEC,
				    EXEC_EXIT_REASON_WRONG_PLATFORM);
				error = EACCES;
				goto done;
			}
			printf("Allowing spawn of iOS binary %s since it has "
			    "com.apple.private.security.no-sandbox entitlement or unentitled_ios_sim_launch "
			    "boot-arg set to true\n", p->p_name);
		} else if (psa->psa_platform != PLATFORM_IOS) {
			                                                  
			signature_failure_reason = os_reason_create(OS_REASON_EXEC,
			    EXEC_EXIT_REASON_WRONG_PLATFORM);
			error = EACCES;
			goto done;
		} else {
			printf("Allowing spawn of iOS binary %s since correct platform was passed in spawn\n",
			    p->p_name);
		}
	}
#endif                        

	                                                                            
                                            
	if (imgp->ip_csflags & CS_SIGNED) {
		error = 0;
		goto done;
	}

	                                                                                  
                                                                                 
                                                        

	kr = task_get_task_access_port(p->task, &port);
	if (KERN_SUCCESS != kr || !IPC_PORT_VALID(port)) {
		error = 0;
		if (require_success) {
			KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
			    p->p_pid, OS_REASON_CODESIGNING, CODESIGNING_EXIT_REASON_TASK_ACCESS_PORT, 0, 0);
			signature_failure_reason = os_reason_create(OS_REASON_CODESIGNING, CODESIGNING_EXIT_REASON_TASK_ACCESS_PORT);
			error = EACCES;
		}
		goto done;
	}

	  
                                                               
                                                                 
                                                             
                                                               
                                                    
    

	kr = __EXEC_WAITING_ON_TASKGATED_CODE_SIGNATURE_UPCALL__(port, p->p_pid);
	switch (kr) {
	case KERN_SUCCESS:
		error = 0;
		break;
	case KERN_FAILURE:
		error = EACCES;

		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
		    p->p_pid, OS_REASON_CODESIGNING, CODESIGNING_EXIT_REASON_TASKGATED_INVALID_SIG, 0, 0);
		signature_failure_reason = os_reason_create(OS_REASON_CODESIGNING, CODESIGNING_EXIT_REASON_TASKGATED_INVALID_SIG);
		goto done;
	default:
		error = EACCES;

		KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_PROC, BSD_PROC_EXITREASON_CREATE) | DBG_FUNC_NONE,
		    p->p_pid, OS_REASON_EXEC, EXEC_EXIT_REASON_TASKGATED_OTHER, 0, 0);
		signature_failure_reason = os_reason_create(OS_REASON_EXEC, EXEC_EXIT_REASON_TASKGATED_OTHER);
		unexpected_failure = TRUE;
		goto done;
	}

	                                                     
	if (p->p_textvp != NULLVP) {
		csb = ubc_cs_blob_get(p->p_textvp, -1, -1, p->p_textoff);

		if (csb != NULL) {
			                                                                               
                                                          
                                                                                 
                                                          
			if (
				                                                                       
				(ubc_cs_generation_check(p->p_textvp) == 0 ||
				ubc_cs_blob_revalidate(p->p_textvp, csb, imgp, 0, proc_platform(p)) == 0) &&
				                                             
				(csb->csb_flags & CS_ALLOWED_MACHO) == CS_ADHOC &&
				                                                                      
				csblob_find_blob_bytes((const uint8_t *)csb->csb_mem_kaddr, csb->csb_mem_size,
				CSSLOT_SIGNATURESLOT,
				CSMAGIC_BLOBWRAPPER) == NULL &&
				                                                                                   
				csb->csb_platform_binary == 0 &&
				                                                  
				csb->csb_entitlements_blob == NULL) {
				proc_lock(p);
				p->p_csflags |= CS_SIGNED | CS_VALID;
				proc_unlock(p);
			} else {
				uint8_t cdhash[CS_CDHASH_LEN];
				char cdhash_string[CS_CDHASH_STRING_SIZE];
				proc_getcdhash(p, cdhash);
				cdhash_to_string(cdhash_string, cdhash);
				printf("ignoring detached code signature on '%s' with cdhash '%s' "
				    "because it is invalid, or not a simple adhoc signature.\n",
				    p->p_name, cdhash_string);
			}
		}
	}

done:
	if (0 == error) {
		                                                      
                                                        
                                              
		if (platform_exec_logging != 0 && csproc_get_platform_binary(p)) {
			uint8_t cdhash[CS_CDHASH_LEN];
			char cdhash_string[CS_CDHASH_STRING_SIZE];
			proc_getcdhash(p, cdhash);
			cdhash_to_string(cdhash_string, cdhash);

			os_log(peLog, "CS Platform Exec Logging: Executing platform signed binary "
			    "'%s' with cdhash %s\n", p->p_name, cdhash_string);
		}
	} else {
		if (!unexpected_failure) {
			p->p_csflags |= CS_KILLED;
		}
		                                    
		if (vfexec || spawn) {
			assert(signature_failure_reason != OS_REASON_NULL);
			psignal_vfork_with_reason(p, p->task, imgp->ip_new_thread,
			    SIGKILL, signature_failure_reason);
			signature_failure_reason = OS_REASON_NULL;
			error = 0;
		} else {
			assert(signature_failure_reason != OS_REASON_NULL);
			psignal_with_reason(p, SIGKILL, signature_failure_reason);
			signature_failure_reason = OS_REASON_NULL;
		}
	}

	if (port != IPC_PORT_NULL) {
		ipc_port_release_send(port);
	}

	                                                                
	assert(signature_failure_reason == OS_REASON_NULL);
	return error;
}

  
                                                            
                                                              
                                                              
                                                               
                                                            
                                                        
                                                         
                                  
   
#ifndef PREVENT_CALLER_STACK_USE
#define PREVENT_CALLER_STACK_USE __attribute__((noinline))
#endif
static void PREVENT_CALLER_STACK_USE
exec_prefault_data(proc_t p __unused, struct image_params *imgp, load_result_t *load_result)
{
	int ret;
	size_t expected_all_image_infos_size;
	kern_return_t kr;

	  
                                            
    
	if (vm_map_page_shift(current_map()) < (int)PAGE_SHIFT) {
		DEBUG4K_LOAD("entry_point 0x%llx\n", (uint64_t)load_result->entry_point);
	}
	kr = vm_fault(current_map(),
	    vm_map_trunc_page(load_result->entry_point,
	    vm_map_page_mask(current_map())),
	    VM_PROT_READ | VM_PROT_EXECUTE,
	    FALSE, VM_KERN_MEMORY_NONE,
	    THREAD_UNINT, NULL, 0);
	if (kr != KERN_SUCCESS) {
		DEBUG4K_ERROR("map %p va 0x%llx -> 0x%x\n", current_map(), (uint64_t)vm_map_trunc_page(load_result->entry_point, vm_map_page_mask(current_map())), kr);
	}

	if (imgp->ip_flags & IMGPF_IS_64BIT_ADDR) {
		expected_all_image_infos_size = sizeof(struct user64_dyld_all_image_infos);
	} else {
		expected_all_image_infos_size = sizeof(struct user32_dyld_all_image_infos);
	}

	                                                              
	if (load_result->dynlinker &&
	    load_result->all_image_info_addr &&
	    load_result->all_image_info_size >= expected_all_image_infos_size) {
		union {
			struct user64_dyld_all_image_infos      infos64;
			struct user32_dyld_all_image_infos      infos32;
		} all_image_infos;

		  
                                                               
                       
     
		if (vm_map_page_shift(current_map()) < (int)PAGE_SHIFT) {
			DEBUG4K_LOAD("all_image_info_addr 0x%llx\n", load_result->all_image_info_addr);
		}
		kr = vm_fault(current_map(),
		    vm_map_trunc_page(load_result->all_image_info_addr,
		    vm_map_page_mask(current_map())),
		    VM_PROT_READ | VM_PROT_WRITE,
		    FALSE, VM_KERN_MEMORY_NONE,
		    THREAD_UNINT, NULL, 0);
		if (kr != KERN_SUCCESS) {
                                                                                                                                                                                 
		}
		if ((load_result->all_image_info_addr & PAGE_MASK) + expected_all_image_infos_size > PAGE_SIZE) {
			                                      
			kr = vm_fault(current_map(),
			    vm_map_trunc_page(load_result->all_image_info_addr + expected_all_image_infos_size - 1,
			    vm_map_page_mask(current_map())),
			    VM_PROT_READ | VM_PROT_WRITE,
			    FALSE, VM_KERN_MEMORY_NONE,
			    THREAD_UNINT, NULL, 0);
			if (kr != KERN_SUCCESS) {
                                                                                                                                                                                                                     
			}
		}

		if (vm_map_page_shift(current_map()) < (int)PAGE_SHIFT) {
			DEBUG4K_LOAD("copyin(0x%llx, 0x%lx)\n", load_result->all_image_info_addr, expected_all_image_infos_size);
		}
		ret = copyin((user_addr_t)load_result->all_image_info_addr,
		    &all_image_infos,
		    expected_all_image_infos_size);
		if (ret == 0 && all_image_infos.infos32.version >= DYLD_ALL_IMAGE_INFOS_ADDRESS_MINIMUM_VERSION) {
			user_addr_t notification_address;
			user_addr_t dyld_image_address;
			user_addr_t dyld_version_address;
			user_addr_t dyld_all_image_infos_address;
			user_addr_t dyld_slide_amount;

			if (imgp->ip_flags & IMGPF_IS_64BIT_ADDR) {
				notification_address = (user_addr_t)all_image_infos.infos64.notification;
				dyld_image_address = (user_addr_t)all_image_infos.infos64.dyldImageLoadAddress;
				dyld_version_address = (user_addr_t)all_image_infos.infos64.dyldVersion;
				dyld_all_image_infos_address = (user_addr_t)all_image_infos.infos64.dyldAllImageInfosAddress;
			} else {
				notification_address = all_image_infos.infos32.notification;
				dyld_image_address = all_image_infos.infos32.dyldImageLoadAddress;
				dyld_version_address = all_image_infos.infos32.dyldVersion;
				dyld_all_image_infos_address = all_image_infos.infos32.dyldAllImageInfosAddress;
			}

			  
                                                               
                                                                       
                                                                
                                                                   
                                                                    
                                                                     
                                                                    
                                                                        
                                                                     
                                                                    
      

			dyld_slide_amount = (user_addr_t)load_result->all_image_info_addr - dyld_all_image_infos_address;

#if 0
			kprintf("exec_prefault: 0x%016llx 0x%08x 0x%016llx 0x%016llx 0x%016llx 0x%016llx\n",
			    (uint64_t)load_result->all_image_info_addr,
			    all_image_infos.infos32.version,
			    (uint64_t)notification_address,
			    (uint64_t)dyld_image_address,
			    (uint64_t)dyld_version_address,
			    (uint64_t)dyld_all_image_infos_address);
#endif

			if (vm_map_page_shift(current_map()) < (int)PAGE_SHIFT) {
				DEBUG4K_LOAD("notification_address 0x%llx dyld_slide_amount 0x%llx\n", (uint64_t)notification_address, (uint64_t)dyld_slide_amount);
			}
			kr = vm_fault(current_map(),
			    vm_map_trunc_page(notification_address + dyld_slide_amount,
			    vm_map_page_mask(current_map())),
			    VM_PROT_READ | VM_PROT_EXECUTE,
			    FALSE, VM_KERN_MEMORY_NONE,
			    THREAD_UNINT, NULL, 0);
			if (kr != KERN_SUCCESS) {
                                                                                                                                                                                          
			}
			if (vm_map_page_shift(current_map()) < (int)PAGE_SHIFT) {
				DEBUG4K_LOAD("dyld_image_address 0x%llx dyld_slide_amount 0x%llx\n", (uint64_t)dyld_image_address, (uint64_t)dyld_slide_amount);
			}
			kr = vm_fault(current_map(),
			    vm_map_trunc_page(dyld_image_address + dyld_slide_amount,
			    vm_map_page_mask(current_map())),
			    VM_PROT_READ | VM_PROT_EXECUTE,
			    FALSE, VM_KERN_MEMORY_NONE,
			    THREAD_UNINT, NULL, 0);
			if (kr != KERN_SUCCESS) {
                                                                                                                                                                                        
			}
			if (vm_map_page_shift(current_map()) < (int)PAGE_SHIFT) {
				DEBUG4K_LOAD("dyld_version_address 0x%llx dyld_slide_amount 0x%llx\n", (uint64_t)dyld_version_address, (uint64_t)dyld_slide_amount);
			}
			kr = vm_fault(current_map(),
			    vm_map_trunc_page(dyld_version_address + dyld_slide_amount,
			    vm_map_page_mask(current_map())),
			    VM_PROT_READ,
			    FALSE, VM_KERN_MEMORY_NONE,
			    THREAD_UNINT, NULL, 0);
			if (kr != KERN_SUCCESS) {
                                                                                                                                                                                          
			}
			if (vm_map_page_shift(current_map()) < (int)PAGE_SHIFT) {
				DEBUG4K_LOAD("dyld_all_image_infos_address 0x%llx dyld_slide_amount 0x%llx\n", (uint64_t)dyld_version_address, (uint64_t)dyld_slide_amount);
			}
			kr = vm_fault(current_map(),
			    vm_map_trunc_page(dyld_all_image_infos_address + dyld_slide_amount,
			    vm_map_page_mask(current_map())),
			    VM_PROT_READ | VM_PROT_WRITE,
			    FALSE, VM_KERN_MEMORY_NONE,
			    THREAD_UNINT, NULL, 0);
			if (kr != KERN_SUCCESS) {
                                                                                                                                                                                                  
			}
		}
	}
}

static int
sysctl_libmalloc_experiments SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg2, req)
	int changed;
	errno_t error;
	uint64_t value = os_atomic_load_wide(&libmalloc_experiment_factors, relaxed);

	error = sysctl_io_number(req, value, sizeof(value), &value, &changed);
	if (error) {
		return error;
	}

	if (changed) {
		os_atomic_store_wide(&libmalloc_experiment_factors, value, relaxed);
	}

	return 0;
}

EXPERIMENT_FACTOR_PROC(_kern, libmalloc_experiments, CTLTYPE_QUAD | CTLFLAG_RW, 0, 0, &sysctl_libmalloc_experiments, "A", "");

