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
/* Copyright (c) 1995, 1997 Apple Computer, Inc. All Rights Reserved */
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
 *	@(#)kern_resource.c	8.5 (Berkeley) 1/21/94
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/sysctl.h>
#include <sys/kernel.h>
#include <sys/file_internal.h>
#include <sys/resourcevar.h>
#include <sys/malloc.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/mount_internal.h>
#include <sys/sysproto.h>

#include <security/audit/audit.h>

#include <machine/vmparam.h>

#include <mach/mach_types.h>
#include <mach/time_value.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <mach/vm_map.h>
#include <mach/mach_vm.h>
#include <mach/thread_act.h>                                
#include <kern/thread.h>
#include <kern/policy_internal.h>

#include <kern/task.h>
#include <kern/clock.h>                                              
#include <netinet/in.h>                                   
#include <sys/socketvar.h>                             
#if NECP
#include <net/necp.h>
#endif           

#include <vm/vm_map.h>

#include <kern/assert.h>
#include <sys/resource.h>
#include <sys/priv.h>
#include <IOKit/IOBSD.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

int     donice(struct proc *curp, struct proc *chgp, int n);
int     dosetrlimit(struct proc *p, u_int which, struct rlimit *limp);
int     uthread_get_background_state(uthread_t);
static void do_background_socket(struct proc *p, thread_t thread);
static int do_background_thread(thread_t thread, int priority);
static int do_background_proc(struct proc *curp, struct proc *targetp, int priority);
static int set_gpudeny_proc(struct proc *curp, struct proc *targetp, int priority);
static int proc_set_darwin_role(proc_t curp, proc_t targetp, int priority);
static int proc_get_darwin_role(proc_t curp, proc_t targetp, int *priority);
static int get_background_proc(struct proc *curp, struct proc *targetp, int *priority);
int proc_pid_rusage(int pid, int flavor, user_addr_t buf, int32_t *retval);
void gather_rusage_info(proc_t p, rusage_info_current *ru, int flavor);
int fill_task_rusage(task_t task, rusage_info_current *ri);
void fill_task_billed_usage(task_t task, rusage_info_current *ri);
int fill_task_io_rusage(task_t task, rusage_info_current *ri);
int fill_task_qos_rusage(task_t task, rusage_info_current *ri);
uint64_t get_task_logical_writes(task_t task, boolean_t external);
void fill_task_monotonic_rusage(task_t task, rusage_info_current *ri);

int proc_get_rusage(proc_t p, int flavor, user_addr_t buffer, __unused int is_zombie);

rlim_t maxdmap = MAXDSIZ;                
rlim_t maxsmap = MAXSSIZ - PAGE_MAX_SIZE;                

                                
os_refgrp_decl(, rlimit_refgrp, "plimit_refcnt", NULL);

ZONE_DECLARE(plimit_zone, "plimit", sizeof(struct plimit), ZC_NOENCRYPT);

  
                                                                 
                                  
  
                                                  
   
__private_extern__ int maxfilesperproc = OPEN_MAX;                                             

SYSCTL_INT(_kern, KERN_MAXPROCPERUID, maxprocperuid, CTLFLAG_RW | CTLFLAG_LOCKED,
    &maxprocperuid, 0, "Maximum processes allowed per userid" );

SYSCTL_INT(_kern, KERN_MAXFILESPERPROC, maxfilesperproc, CTLFLAG_RW | CTLFLAG_LOCKED,
    &maxfilesperproc, 0, "Maximum files allowed open per process" );

                                                                 
struct puser_nice_args {
	proc_t curp;
	int     prio;
	id_t    who;
	int *   foundp;
	int *   errorp;
};
static int puser_donice_callback(proc_t p, void * arg);


                                                                 
struct ppgrp_nice_args {
	proc_t curp;
	int     prio;
	int *   foundp;
	int *   errorp;
};
static int ppgrp_donice_callback(proc_t p, void * arg);

  
                                    
   
int
getpriority(struct proc *curp, struct getpriority_args *uap, int32_t *retval)
{
	struct proc *p;
	int low = PRIO_MAX + 1;
	kauth_cred_t my_cred;
	int refheld = 0;
	int error = 0;

	                                                          
	if (uap->who > 0x7fffffff) {
		return EINVAL;
	}

	switch (uap->which) {
	case PRIO_PROCESS:
		if (uap->who == 0) {
			p = curp;
			low = p->p_nice;
		} else {
			p = proc_find(uap->who);
			if (p == 0) {
				break;
			}
			low = p->p_nice;
			proc_rele(p);
		}
		break;

	case PRIO_PGRP: {
		struct pgrp *pg = PGRP_NULL;

		if (uap->who == 0) {
			                             
			pg = proc_pgrp(curp);
		} else if ((pg = pgfind(uap->who)) == PGRP_NULL) {
			break;
		}
		                                                  
		pgrp_lock(pg);
		PGMEMBERS_FOREACH(pg, p) {
			if (p->p_nice < low) {
				low = p->p_nice;
			}
		}
		pgrp_unlock(pg);
		pg_rele(pg);
		break;
	}

	case PRIO_USER:
		if (uap->who == 0) {
			uap->who = kauth_cred_getuid(kauth_cred_get());
		}

		proc_list_lock();

		for (p = allproc.lh_first; p != 0; p = p->p_list.le_next) {
			my_cred = kauth_cred_proc_ref(p);
			if (kauth_cred_getuid(my_cred) == uap->who &&
			    p->p_nice < low) {
				low = p->p_nice;
			}
			kauth_cred_unref(&my_cred);
		}

		proc_list_unlock();

		break;

	case PRIO_DARWIN_THREAD:
		                                                  
		if (uap->who != 0) {
			return EINVAL;
		}

		low = proc_get_thread_policy(current_thread(), TASK_POLICY_INTERNAL, TASK_POLICY_DARWIN_BG);

		break;

	case PRIO_DARWIN_PROCESS:
		if (uap->who == 0) {
			p = curp;
		} else {
			p = proc_find(uap->who);
			if (p == PROC_NULL) {
				break;
			}
			refheld = 1;
		}

		error = get_background_proc(curp, p, &low);

		if (refheld) {
			proc_rele(p);
		}
		if (error) {
			return error;
		}
		break;

	case PRIO_DARWIN_ROLE:
		if (uap->who == 0) {
			p = curp;
		} else {
			p = proc_find(uap->who);
			if (p == PROC_NULL) {
				break;
			}
			refheld = 1;
		}

		error = proc_get_darwin_role(curp, p, &low);

		if (refheld) {
			proc_rele(p);
		}
		if (error) {
			return error;
		}
		break;

	default:
		return EINVAL;
	}
	if (low == PRIO_MAX + 1) {
		return ESRCH;
	}
	*retval = low;
	return 0;
}

                                                             
static int
puser_donice_callback(proc_t p, void * arg)
{
	int error, n;
	struct puser_nice_args * pun = (struct puser_nice_args *)arg;
	kauth_cred_t my_cred;

	my_cred = kauth_cred_proc_ref(p);
	if (kauth_cred_getuid(my_cred) == pun->who) {
		error = donice(pun->curp, p, pun->prio);
		if (pun->errorp != NULL) {
			*pun->errorp = error;
		}
		if (pun->foundp != NULL) {
			n = *pun->foundp;
			*pun->foundp = n + 1;
		}
	}
	kauth_cred_unref(&my_cred);

	return PROC_RETURNED;
}

                                                             
static int
ppgrp_donice_callback(proc_t p, void * arg)
{
	int error;
	struct ppgrp_nice_args * pun = (struct ppgrp_nice_args *)arg;
	int n;

	error = donice(pun->curp, p, pun->prio);
	if (pun->errorp != NULL) {
		*pun->errorp = error;
	}
	if (pun->foundp != NULL) {
		n = *pun->foundp;
		*pun->foundp = n + 1;
	}

	return PROC_RETURNED;
}

  
                       
          
         
               
                
   
              
int
setpriority(struct proc *curp, struct setpriority_args *uap, int32_t *retval)
{
	struct proc *p;
	int found = 0, error = 0;
	int refheld = 0;

	AUDIT_ARG(cmd, uap->which);
	AUDIT_ARG(owner, uap->who, 0);
	AUDIT_ARG(value32, uap->prio);

	                                                          
	if (uap->who > 0x7fffffff) {
		return EINVAL;
	}

	switch (uap->which) {
	case PRIO_PROCESS:
		if (uap->who == 0) {
			p = curp;
		} else {
			p = proc_find(uap->who);
			if (p == 0) {
				break;
			}
			refheld = 1;
		}
		error = donice(curp, p, uap->prio);
		found++;
		if (refheld != 0) {
			proc_rele(p);
		}
		break;

	case PRIO_PGRP: {
		struct pgrp *pg = PGRP_NULL;
		struct ppgrp_nice_args ppgrp;

		if (uap->who == 0) {
			pg = proc_pgrp(curp);
		} else if ((pg = pgfind(uap->who)) == PGRP_NULL) {
			break;
		}

		ppgrp.curp = curp;
		ppgrp.prio = uap->prio;
		ppgrp.foundp = &found;
		ppgrp.errorp = &error;

		                                                       
		pgrp_iterate(pg, PGRP_DROPREF, ppgrp_donice_callback, (void *)&ppgrp, NULL, NULL);

		break;
	}

	case PRIO_USER: {
		struct puser_nice_args punice;

		if (uap->who == 0) {
			uap->who = kauth_cred_getuid(kauth_cred_get());
		}

		punice.curp = curp;
		punice.prio = uap->prio;
		punice.who = uap->who;
		punice.foundp = &found;
		error = 0;
		punice.errorp = &error;
		proc_iterate(PROC_ALLPROCLIST, puser_donice_callback, (void *)&punice, NULL, NULL);

		break;
	}

	case PRIO_DARWIN_THREAD: {
		                                                  
		if (uap->who != 0) {
			return EINVAL;
		}

		error = do_background_thread(current_thread(), uap->prio);
		found++;
		break;
	}

	case PRIO_DARWIN_PROCESS: {
		if (uap->who == 0) {
			p = curp;
		} else {
			p = proc_find(uap->who);
			if (p == 0) {
				break;
			}
			refheld = 1;
		}

		error = do_background_proc(curp, p, uap->prio);

		found++;
		if (refheld != 0) {
			proc_rele(p);
		}
		break;
	}

	case PRIO_DARWIN_GPU: {
		if (uap->who == 0) {
			return EINVAL;
		}

		p = proc_find(uap->who);
		if (p == PROC_NULL) {
			break;
		}

		error = set_gpudeny_proc(curp, p, uap->prio);

		found++;
		proc_rele(p);
		break;
	}

	case PRIO_DARWIN_ROLE: {
		if (uap->who == 0) {
			p = curp;
		} else {
			p = proc_find(uap->who);
			if (p == PROC_NULL) {
				break;
			}
			refheld = 1;
		}

		error = proc_set_darwin_role(curp, p, uap->prio);

		found++;
		if (refheld != 0) {
			proc_rele(p);
		}
		break;
	}

	default:
		return EINVAL;
	}
	if (found == 0) {
		return ESRCH;
	}
	if (error == EIDRM) {
		*retval = -2;
		error = 0;
	}
	return error;
}


  
                       
         
          
                           
   
int
donice(struct proc *curp, struct proc *chgp, int n)
{
	int error = 0;
	kauth_cred_t ucred;
	kauth_cred_t my_cred;

	ucred = kauth_cred_proc_ref(curp);
	my_cred = kauth_cred_proc_ref(chgp);

	if (suser(ucred, NULL) && kauth_cred_getruid(ucred) &&
	    kauth_cred_getuid(ucred) != kauth_cred_getuid(my_cred) &&
	    kauth_cred_getruid(ucred) != kauth_cred_getuid(my_cred)) {
		error = EPERM;
		goto out;
	}
	if (n > PRIO_MAX) {
		n = PRIO_MAX;
	}
	if (n < PRIO_MIN) {
		n = PRIO_MIN;
	}
	if (n < chgp->p_nice && suser(ucred, &curp->p_acflag)) {
		error = EACCES;
		goto out;
	}
#if CONFIG_MACF
	error = mac_proc_check_sched(curp, chgp);
	if (error) {
		goto out;
	}
#endif
	proc_lock(chgp);
	chgp->p_nice = (char)n;
	proc_unlock(chgp);
	(void)resetpriority(chgp);
out:
	kauth_cred_unref(&ucred);
	kauth_cred_unref(&my_cred);
	return error;
}

static int
set_gpudeny_proc(struct proc *curp, struct proc *targetp, int priority)
{
	int error = 0;
	kauth_cred_t ucred;
	kauth_cred_t target_cred;

	ucred = kauth_cred_get();
	target_cred = kauth_cred_proc_ref(targetp);

	                                            

	if (!kauth_cred_issuser(ucred) && kauth_cred_getruid(ucred) &&
	    kauth_cred_getuid(ucred) != kauth_cred_getuid(target_cred) &&
	    kauth_cred_getruid(ucred) != kauth_cred_getuid(target_cred)) {
		error = EPERM;
		goto out;
	}

	if (curp == targetp) {
		error = EPERM;
		goto out;
	}

#if CONFIG_MACF
	error = mac_proc_check_sched(curp, targetp);
	if (error) {
		goto out;
	}
#endif

	switch (priority) {
	case PRIO_DARWIN_GPU_DENY:
		task_set_gpu_denied(proc_task(targetp), TRUE);
		break;
	case PRIO_DARWIN_GPU_ALLOW:
		task_set_gpu_denied(proc_task(targetp), FALSE);
		break;
	default:
		error = EINVAL;
		goto out;
	}

out:
	kauth_cred_unref(&target_cred);
	return error;
}

static int
proc_set_darwin_role(proc_t curp, proc_t targetp, int priority)
{
	int error = 0;
	uint32_t flagsp = 0;

	kauth_cred_t ucred, target_cred;

	ucred = kauth_cred_get();
	target_cred = kauth_cred_proc_ref(targetp);

	if (!kauth_cred_issuser(ucred) && kauth_cred_getruid(ucred) &&
	    kauth_cred_getuid(ucred) != kauth_cred_getuid(target_cred) &&
	    kauth_cred_getruid(ucred) != kauth_cred_getuid(target_cred)) {
		if (priv_check_cred(ucred, PRIV_SETPRIORITY_DARWIN_ROLE, 0) != 0) {
			error = EPERM;
			goto out;
		}
	}

	if (curp != targetp) {
#if CONFIG_MACF
		if ((error = mac_proc_check_sched(curp, targetp))) {
			goto out;
		}
#endif
	}

	proc_get_darwinbgstate(proc_task(targetp), &flagsp);
	if ((flagsp & PROC_FLAG_APPLICATION) != PROC_FLAG_APPLICATION) {
		error = ENOTSUP;
		goto out;
	}

	task_role_t role = TASK_UNSPECIFIED;

	if ((error = proc_darwin_role_to_task_role(priority, &role))) {
		goto out;
	}

	proc_set_task_policy(proc_task(targetp), TASK_POLICY_ATTRIBUTE,
	    TASK_POLICY_ROLE, role);

out:
	kauth_cred_unref(&target_cred);
	return error;
}

static int
proc_get_darwin_role(proc_t curp, proc_t targetp, int *priority)
{
	int error = 0;
	int role = 0;

	kauth_cred_t ucred, target_cred;

	ucred = kauth_cred_get();
	target_cred = kauth_cred_proc_ref(targetp);

	if (!kauth_cred_issuser(ucred) && kauth_cred_getruid(ucred) &&
	    kauth_cred_getuid(ucred) != kauth_cred_getuid(target_cred) &&
	    kauth_cred_getruid(ucred) != kauth_cred_getuid(target_cred)) {
		error = EPERM;
		goto out;
	}

	if (curp != targetp) {
#if CONFIG_MACF
		if ((error = mac_proc_check_sched(curp, targetp))) {
			goto out;
		}
#endif
	}

	role = proc_get_task_policy(proc_task(targetp), TASK_POLICY_ATTRIBUTE, TASK_POLICY_ROLE);

	*priority = proc_task_role_to_darwin_role(role);

out:
	kauth_cred_unref(&target_cred);
	return error;
}


static int
get_background_proc(struct proc *curp, struct proc *targetp, int *priority)
{
	int external = 0;
	int error = 0;
	kauth_cred_t ucred, target_cred;

	ucred = kauth_cred_get();
	target_cred = kauth_cred_proc_ref(targetp);

	if (!kauth_cred_issuser(ucred) && kauth_cred_getruid(ucred) &&
	    kauth_cred_getuid(ucred) != kauth_cred_getuid(target_cred) &&
	    kauth_cred_getruid(ucred) != kauth_cred_getuid(target_cred)) {
		error = EPERM;
		goto out;
	}

	external = (curp == targetp) ? TASK_POLICY_INTERNAL : TASK_POLICY_EXTERNAL;

	*priority = proc_get_task_policy(current_task(), external, TASK_POLICY_DARWIN_BG);

out:
	kauth_cred_unref(&target_cred);
	return error;
}

static int
do_background_proc(struct proc *curp, struct proc *targetp, int priority)
{
#if !CONFIG_MACF
#pragma unused(curp)
#endif
	int error = 0;
	kauth_cred_t ucred;
	kauth_cred_t target_cred;
	int external;
	int enable;

	ucred = kauth_cred_get();
	target_cred = kauth_cred_proc_ref(targetp);

	if (!kauth_cred_issuser(ucred) && kauth_cred_getruid(ucred) &&
	    kauth_cred_getuid(ucred) != kauth_cred_getuid(target_cred) &&
	    kauth_cred_getruid(ucred) != kauth_cred_getuid(target_cred)) {
		error = EPERM;
		goto out;
	}

#if CONFIG_MACF
	error = mac_proc_check_sched(curp, targetp);
	if (error) {
		goto out;
	}
#endif

	external = (curp == targetp) ? TASK_POLICY_INTERNAL : TASK_POLICY_EXTERNAL;

	switch (priority) {
	case PRIO_DARWIN_BG:
		enable = TASK_POLICY_ENABLE;
		break;
	case PRIO_DARWIN_NONUI:
		                               
		goto out;
	default:
		                                   
		enable = TASK_POLICY_DISABLE;
		break;
	}

	proc_set_task_policy(proc_task(targetp), external, TASK_POLICY_DARWIN_BG, enable);

out:
	kauth_cred_unref(&target_cred);
	return error;
}

static void
do_background_socket(struct proc *p, thread_t thread)
{
#if SOCKETS
	struct fileproc *fp;
	int              background = false;
#if NECP
	int              update_necp = false;
#endif           

	proc_fdlock(p);

	if (thread != THREAD_NULL) {
		background = proc_get_effective_thread_policy(thread, TASK_POLICY_ALL_SOCKETS_BG);
	} else {
		background = proc_get_effective_task_policy(proc_task(p), TASK_POLICY_ALL_SOCKETS_BG);
	}

	if (background) {
		  
                                                          
                                                           
                                                
     
		if (thread == THREAD_NULL) {
			fdt_foreach(fp, p) {
				if (FILEGLOB_DTYPE(fp->fp_glob) == DTYPE_SOCKET) {
					struct socket *sockp = (struct socket *)fp->fp_glob->fg_data;
					socket_set_traffic_mgt_flags(sockp, TRAFFIC_MGT_SO_BACKGROUND);
					sockp->so_background_thread = NULL;
				}
#if NECP
				else if (FILEGLOB_DTYPE(fp->fp_glob) == DTYPE_NETPOLICY) {
					if (necp_set_client_as_background(p, fp, background)) {
						update_necp = true;
					}
				}
#endif           
			}
		}
	} else {
		                                  
                                                                  
                                                              
                                                          
     
		fdt_foreach(fp, p) {
			struct socket *sockp;

			if (FILEGLOB_DTYPE(fp->fp_glob) == DTYPE_SOCKET) {
				sockp = (struct socket *)fp->fp_glob->fg_data;
				                                                 
				if ((thread) && (sockp->so_background_thread != thread)) {
					continue;
				}
				socket_clear_traffic_mgt_flags(sockp, TRAFFIC_MGT_SO_BACKGROUND);
				sockp->so_background_thread = NULL;
			}
#if NECP
			else if (FILEGLOB_DTYPE(fp->fp_glob) == DTYPE_NETPOLICY) {
				if (necp_set_client_as_background(p, fp, background)) {
					update_necp = true;
				}
			}
#endif           
		}
	}

	proc_fdunlock(p);

#if NECP
	if (update_necp) {
		necp_update_all_clients();
	}
#endif           
#else
#pragma unused(p, thread)
#endif
}


  
                       
  
                             
  
                                               
                                                                          
                                           
   
static int
do_background_thread(thread_t thread, int priority)
{
	struct uthread *ut;
	int enable, external;
	int rv = 0;

	ut = get_bsdthread_info(thread);

	                                                       
	if ((ut->uu_flag & UT_VFORK) != 0) {
		return EPERM;
	}

	                                                    
	if (thread_is_static_param(thread)) {
		return EPERM;
	}

	                                                                       
	if (thread_has_qos_policy(thread)) {
		thread_remove_qos_policy(thread);
		rv = EIDRM;
	}

	                                                                        
	enable   = (priority == PRIO_DARWIN_BG) ? TASK_POLICY_ENABLE   : TASK_POLICY_DISABLE;
	external = (current_thread() == thread) ? TASK_POLICY_INTERNAL : TASK_POLICY_EXTERNAL;

	proc_set_thread_policy(thread, external, TASK_POLICY_DARWIN_BG, enable);

	return rv;
}


  
                       
                
               
   
              
int
setrlimit(struct proc *p, struct setrlimit_args *uap, __unused int32_t *retval)
{
	struct rlimit alim;
	int error;

	if ((error = copyin(uap->rlp, (caddr_t)&alim,
	    sizeof(struct rlimit)))) {
		return error;
	}

	return dosetrlimit(p, uap->which, &alim);
}

  
                       
          
              
  
                                                                   
                                                              
                                     
   
int
dosetrlimit(struct proc *p, u_int which, struct rlimit *newrlim)
{
	struct rlimit        rlim;
	int                  error;
	kern_return_t        kr;
	int                  posix = (which & _RLIMIT_POSIX_FLAG) ? 1 : 0;

	                                      
	which &= ~_RLIMIT_POSIX_FLAG;

	                      
	if (which >= RLIM_NLIMITS) {
		return EINVAL;
	}

	  
                                                                         
                                                                      
                                                                    
                                  
    
	proc_lock(p);
	rlim = p->p_limit->pl_rlimit[which];
	proc_unlock(p);

	error = 0;
	                                                               
	if (newrlim->rlim_cur > newrlim->rlim_max) {
		error = EINVAL;
	}
	  
                                                           
                                                                                                 
    
	else if (newrlim->rlim_cur > rlim.rlim_max || newrlim->rlim_max > rlim.rlim_max) {
		                                                            
		error = suser(kauth_cred_get(), &p->p_acflag);
	}

	if (error) {
		                                                
		return error;
	}

	                                                                           
	proc_lock(p);
	proc_limitblock(p);
	proc_unlock(p);

	                                                                                            
	switch (which) {
	case RLIMIT_CPU:
		if (newrlim->rlim_cur == RLIM_INFINITY) {
			task_vtimer_clear(p->task, TASK_VTIMER_RLIM);
			timerclear(&p->p_rlim_cpu);
		} else {
			task_absolutetime_info_data_t   tinfo;
			mach_msg_type_number_t          count;
			struct timeval                  ttv, tv;
			clock_sec_t                     tv_sec;
			clock_usec_t                    tv_usec;

			count = TASK_ABSOLUTETIME_INFO_COUNT;
			task_info(p->task, TASK_ABSOLUTETIME_INFO, (task_info_t)&tinfo, &count);
			absolutetime_to_microtime(tinfo.total_user + tinfo.total_system, &tv_sec, &tv_usec);
			ttv.tv_sec = tv_sec;
			ttv.tv_usec = tv_usec;

			tv.tv_sec = (newrlim->rlim_cur > __INT_MAX__ ? __INT_MAX__ : (__darwin_time_t)newrlim->rlim_cur);
			tv.tv_usec = 0;
			timersub(&tv, &ttv, &p->p_rlim_cpu);

			timerclear(&tv);
			if (timercmp(&p->p_rlim_cpu, &tv, >)) {
				task_vtimer_set(p->task, TASK_VTIMER_RLIM);
			} else {
				task_vtimer_clear(p->task, TASK_VTIMER_RLIM);

				timerclear(&p->p_rlim_cpu);

				psignal(p, SIGXCPU);
			}
		}
		break;

	case RLIMIT_DATA:
		if (newrlim->rlim_cur > maxdmap) {
			newrlim->rlim_cur = maxdmap;
		}
		if (newrlim->rlim_max > maxdmap) {
			newrlim->rlim_max = maxdmap;
		}
		break;

	case RLIMIT_STACK:
		if (p->p_lflag & P_LCUSTOM_STACK) {
			                                                                         
			error = EINVAL;
			goto out;
		}

		                                                     
		if (newrlim->rlim_cur > maxsmap ||
		    newrlim->rlim_max > maxsmap) {
			if (posix) {
				error = EINVAL;
				goto out;
			} else {
				  
                                                        
                                                         
                               
       
				if (newrlim->rlim_cur > maxsmap) {
					newrlim->rlim_cur = maxsmap;
				}
				if (newrlim->rlim_max > maxsmap) {
					newrlim->rlim_max = maxsmap;
				}
			}
		}

		  
                                                         
                                                          
                                                              
     
		if (newrlim->rlim_cur > rlim.rlim_cur) {
			mach_vm_offset_t addr;
			mach_vm_size_t size;

			                
			size = round_page_64(newrlim->rlim_cur);
			size -= round_page_64(rlim.rlim_cur);

			addr = (mach_vm_offset_t)(p->user_stack - round_page_64(newrlim->rlim_cur));
			kr = mach_vm_protect(current_map(), addr, size, FALSE, VM_PROT_DEFAULT);
			if (kr != KERN_SUCCESS) {
				error =  EINVAL;
				goto out;
			}
		} else if (newrlim->rlim_cur < rlim.rlim_cur) {
			mach_vm_offset_t addr;
			mach_vm_size_t size;
			uint64_t cur_sp;

			                  

			  
                                                
                               
                                               
      
			cur_sp = thread_adjuserstack(current_thread(), 0);
			if (cur_sp <= p->user_stack &&
			    cur_sp > (p->user_stack - round_page_64(rlim.rlim_cur))) {
				                                    
				if (cur_sp <= (p->user_stack - round_page_64(newrlim->rlim_cur))) {
					  
                                                          
                         
        
					error =  EINVAL;
					goto out;
				}
			} else {
				                                   
				error =  EINVAL;
				goto out;
			}

			size = round_page_64(rlim.rlim_cur);
			size -= round_page_64(rlim.rlim_cur);

			addr = (mach_vm_offset_t)(p->user_stack - round_page_64(rlim.rlim_cur));

			kr = mach_vm_protect(current_map(), addr, size, FALSE, VM_PROT_NONE);
			if (kr != KERN_SUCCESS) {
				error =  EINVAL;
				goto out;
			}
		} else {
			                   
		}
		break;

	case RLIMIT_NOFILE:
		  
                                                                                                             
                                                                                                   
     
		break;

	case RLIMIT_NPROC:
		  
                                                      
                                                   
                                                  
     
		if (kauth_cred_issuser(kauth_cred_get())) {
			if (newrlim->rlim_cur > (rlim_t)maxproc) {
				newrlim->rlim_cur = maxproc;
			}
			if (newrlim->rlim_max > (rlim_t)maxproc) {
				newrlim->rlim_max = maxproc;
			}
		} else {
			if (newrlim->rlim_cur > (rlim_t)maxprocperuid) {
				newrlim->rlim_cur = maxprocperuid;
			}
			if (newrlim->rlim_max > (rlim_t)maxprocperuid) {
				newrlim->rlim_max = maxprocperuid;
			}
		}
		break;

	case RLIMIT_MEMLOCK:
		  
                                                      
     
		newrlim->rlim_cur = (vm_size_t)newrlim->rlim_cur;
		vm_map_set_user_wire_limit(current_map(), (vm_size_t)newrlim->rlim_cur);
		break;
	}                

	                                                                     
	error = 0;

out:

	if (error == 0) {
		  
                                                                         
                                                            
     
		proc_lock(p);
		proc_limitupdate(p, newrlim, (uint8_t)which);
		proc_limitunblock(p);
		proc_unlock(p);
	} else {
		  
                                                                             
                                      
     
		proc_lock(p);
		proc_limitunblock(p);
		proc_unlock(p);
	}

	return error;
}

              
int
getrlimit(struct proc *p, struct getrlimit_args *uap, __unused int32_t *retval)
{
	struct rlimit lim = {};

	  
                                                                  
                    
    
	uap->which &= ~_RLIMIT_POSIX_FLAG;

	if (uap->which >= RLIM_NLIMITS) {
		return EINVAL;
	}
	proc_limitget(p, uap->which, &lim);
	return copyout((caddr_t)&lim,
	           uap->rlp, sizeof(struct rlimit));
}

  
                                                                       
                                    
   
                                        
void
calcru(struct proc *p, struct timeval *up, struct timeval *sp, struct timeval *ip)
{
	task_t                  task;

	timerclear(up);
	timerclear(sp);
	if (ip != NULL) {
		timerclear(ip);
	}

	task = p->task;
	if (task) {
		mach_task_basic_info_data_t tinfo;
		task_thread_times_info_data_t ttimesinfo;
		task_events_info_data_t teventsinfo;
		mach_msg_type_number_t task_info_count, task_ttimes_count;
		mach_msg_type_number_t task_events_count;
		struct timeval ut, st;

		task_info_count = MACH_TASK_BASIC_INFO_COUNT;
		task_info(task, MACH_TASK_BASIC_INFO,
		    (task_info_t)&tinfo, &task_info_count);
		ut.tv_sec = tinfo.user_time.seconds;
		ut.tv_usec = tinfo.user_time.microseconds;
		st.tv_sec = tinfo.system_time.seconds;
		st.tv_usec = tinfo.system_time.microseconds;
		timeradd(&ut, up, up);
		timeradd(&st, sp, sp);

		task_ttimes_count = TASK_THREAD_TIMES_INFO_COUNT;
		task_info(task, TASK_THREAD_TIMES_INFO,
		    (task_info_t)&ttimesinfo, &task_ttimes_count);

		ut.tv_sec = ttimesinfo.user_time.seconds;
		ut.tv_usec = ttimesinfo.user_time.microseconds;
		st.tv_sec = ttimesinfo.system_time.seconds;
		st.tv_usec = ttimesinfo.system_time.microseconds;
		timeradd(&ut, up, up);
		timeradd(&st, sp, sp);

		task_events_count = TASK_EVENTS_INFO_COUNT;
		task_info(task, TASK_EVENTS_INFO,
		    (task_info_t)&teventsinfo, &task_events_count);

		  
                                                   
                                   
     
		p->p_stats->p_ru.ru_minflt = (teventsinfo.faults -
		    teventsinfo.pageins);
		p->p_stats->p_ru.ru_majflt = teventsinfo.pageins;
		p->p_stats->p_ru.ru_nivcsw = (teventsinfo.csw -
		    p->p_stats->p_ru.ru_nvcsw);
		if (p->p_stats->p_ru.ru_nivcsw < 0) {
			p->p_stats->p_ru.ru_nivcsw = 0;
		}

		p->p_stats->p_ru.ru_maxrss = (long)tinfo.resident_size_max;
	}
}

__private_extern__ void munge_user64_rusage(struct rusage *a_rusage_p, struct user64_rusage *a_user_rusage_p);
__private_extern__ void munge_user32_rusage(struct rusage *a_rusage_p, struct user32_rusage *a_user_rusage_p);

              
int
getrusage(struct proc *p, struct getrusage_args *uap, __unused int32_t *retval)
{
	struct rusage *rup, rubuf;
	struct user64_rusage rubuf64 = {};
	struct user32_rusage rubuf32 = {};
	size_t retsize = sizeof(rubuf);                                       
	caddr_t retbuf = (caddr_t)&rubuf;                                     
	struct timeval utime;
	struct timeval stime;


	switch (uap->who) {
	case RUSAGE_SELF:
		calcru(p, &utime, &stime, NULL);
		proc_lock(p);
		rup = &p->p_stats->p_ru;
		rup->ru_utime = utime;
		rup->ru_stime = stime;

		rubuf = *rup;
		proc_unlock(p);

		break;

	case RUSAGE_CHILDREN:
		proc_lock(p);
		rup = &p->p_stats->p_cru;
		rubuf = *rup;
		proc_unlock(p);
		break;

	default:
		return EINVAL;
	}
	if (IS_64BIT_PROCESS(p)) {
		retsize = sizeof(rubuf64);
		retbuf = (caddr_t)&rubuf64;
		munge_user64_rusage(&rubuf, &rubuf64);
	} else {
		retsize = sizeof(rubuf32);
		retbuf = (caddr_t)&rubuf32;
		munge_user32_rusage(&rubuf, &rubuf32);
	}

	return copyout(retbuf, uap->rusage, retsize);
}

void
ruadd(struct rusage *ru, struct rusage *ru2)
{
	long *ip, *ip2;
	long i;

	timeradd(&ru->ru_utime, &ru2->ru_utime, &ru->ru_utime);
	timeradd(&ru->ru_stime, &ru2->ru_stime, &ru->ru_stime);
	if (ru->ru_maxrss < ru2->ru_maxrss) {
		ru->ru_maxrss = ru2->ru_maxrss;
	}
	ip = &ru->ru_first; ip2 = &ru2->ru_first;
	for (i = &ru->ru_last - &ru->ru_first; i >= 0; i--) {
		*ip++ += *ip2++;
	}
}

  
                                           
  
                                                                       
                          
  
                                                                        
   
void
update_rusage_info_child(struct rusage_info_child *ri, rusage_info_current *ri_current)
{
	ri->ri_child_user_time += (ri_current->ri_user_time +
	    ri_current->ri_child_user_time);
	ri->ri_child_system_time += (ri_current->ri_system_time +
	    ri_current->ri_child_system_time);
	ri->ri_child_pkg_idle_wkups += (ri_current->ri_pkg_idle_wkups +
	    ri_current->ri_child_pkg_idle_wkups);
	ri->ri_child_interrupt_wkups += (ri_current->ri_interrupt_wkups +
	    ri_current->ri_child_interrupt_wkups);
	ri->ri_child_pageins += (ri_current->ri_pageins +
	    ri_current->ri_child_pageins);
	ri->ri_child_elapsed_abstime += ((ri_current->ri_proc_exit_abstime -
	    ri_current->ri_proc_start_abstime) + ri_current->ri_child_elapsed_abstime);
}

  
                                              
   
rlim_t
proc_limitgetcur(proc_t p, int which, boolean_t to_lock_proc)
{
	rlim_t rlim_cur;

	assert(p);
	assert(which < RLIM_NLIMITS);

	  
                                                                            
    
	if (to_lock_proc) {
		lck_mtx_assert(&p->p_mlock, LCK_MTX_ASSERT_NOTOWNED);
		proc_lock(p);
	}

	rlim_cur = p->p_limit->pl_rlimit[which].rlim_cur;

	if (to_lock_proc) {
		proc_unlock(p);
	}

	return rlim_cur;
}

  
                                                                         
                                                                           
                                    
   
void
proc_limitsetcur_internal(proc_t p, int which, rlim_t value)
{
	struct rlimit rlim;

	assert(p);
	assertf(which == RLIMIT_FSIZE, "%s only supports RLIMIT_FSIZE\n", __FUNCTION__);


	proc_lock(p);

	                                                               
	proc_limitblock(p);

	                                            
	rlim = p->p_limit->pl_rlimit[which];
	rlim.rlim_cur = value;

	  
                                                                                    
                                                                              
    
	proc_limitupdate(p, &rlim, (uint8_t)which);

	                                                    
	proc_limitunblock(p);

	proc_unlock(p);
}

void
proc_limitget(proc_t p, int which, struct rlimit * limp)
{
	assert(p);
	assert(limp);
	assert(which < RLIM_NLIMITS);

	                                                                                 
	proc_lock(p);

	limp->rlim_cur = p->p_limit->pl_rlimit[which].rlim_cur;
	limp->rlim_max = p->p_limit->pl_rlimit[which].rlim_max;

	proc_unlock(p);
}

void
proc_limitfork(proc_t parent, proc_t child)
{
	assert(parent && child);

	proc_lock(parent);

	                                         
	child->p_limit = parent->p_limit;

	                                           
	os_ref_retain(&parent->p_limit->pl_refcnt);

	proc_unlock(parent);
}

void
proc_limitdrop(proc_t p)
{
	struct plimit *free_plim = NULL;
	os_ref_count_t refcnt;

	proc_lock(p);

	                                                         
	refcnt = os_ref_release(&p->p_limit->pl_refcnt);
	if (refcnt == 0) {
		free_plim = p->p_limit;
	}

	p->p_limit = NULL;
	proc_unlock(p);

	                                                       
	if (free_plim != NULL) {
		zfree(plimit_zone, free_plim);
	}
}

  
                                                                 
                                                   
                                                              
                       
   
void
proc_limitblock(proc_t p)
{
	lck_mtx_assert(&p->p_mlock, LCK_MTX_ASSERT_OWNED);

	while (p->p_lflag & P_LLIMCHANGE) {
		p->p_lflag |= P_LLIMWAIT;
		msleep(&p->p_limit, &p->p_mlock, 0, "proc_limitblock", NULL);
	}
	p->p_lflag |= P_LLIMCHANGE;
}

  
                                                              
                       
   
void
proc_limitunblock(proc_t p)
{
	lck_mtx_assert(&p->p_mlock, LCK_MTX_ASSERT_OWNED);

	p->p_lflag &= ~P_LLIMCHANGE;
	if (p->p_lflag & P_LLIMWAIT) {
		p->p_lflag &= ~P_LLIMWAIT;
		wakeup(&p->p_limit);
	}
}

  
                                                                          
  
                                                                      
                                                                            
                                                                             
                               
  
                                                                           
                                                          
  
                                                                           
                                                                               
                                                               
   
void
proc_limitupdate(proc_t p, struct rlimit *rlim, uint8_t which)
{
	struct plimit  *copy_plim;
	struct plimit  *free_plim;
	os_ref_count_t refcnt;

	assert(p && p->p_limit);
	assert(rlim);
	assert(which < RLIM_NLIMITS);
	lck_mtx_assert(&p->p_mlock, LCK_MTX_ASSERT_OWNED);

	  
                                                                            
                                                                  
    
	refcnt = os_ref_get_count(&p->p_limit->pl_refcnt);
	if (refcnt == 1) {
		p->p_limit->pl_rlimit[which] = *rlim;
		return;
	}

	  
                                                                                  
                                                                                   
                                                                 
   
                                                            
    
	proc_unlock(p);
	copy_plim = zalloc(plimit_zone);

	                              
	proc_lock(p);
	bcopy(p->p_limit->pl_rlimit, copy_plim->pl_rlimit, sizeof(struct rlimit) * RLIM_NLIMITS);

	  
                                                                                      
                                                                                     
                                                                                      
                                                                                           
                                                                          
    
	free_plim = NULL;
	refcnt = os_ref_release(&p->p_limit->pl_refcnt);
	if (refcnt == 0) {
		free_plim = p->p_limit;
	}
	                                           
	os_ref_init_count(&copy_plim->pl_refcnt, &rlimit_refgrp, 1);

	                             
	copy_plim->pl_rlimit[which] = *rlim;

	                                                                     
	p->p_limit = copy_plim;
	proc_unlock(p);

	if (free_plim != NULL) {
		zfree(plimit_zone, free_plim);
	}

	                                      
	proc_lock(p);
}

static int
iopolicysys_disk(struct proc *p, int cmd, int scope, int policy, struct _iopol_param_t *iop_param);
static int
iopolicysys_vfs_hfs_case_sensitivity(struct proc *p, int cmd, int scope, int policy, struct _iopol_param_t *iop_param);
static int
iopolicysys_vfs_atime_updates(struct proc *p, int cmd, int scope, int policy, struct _iopol_param_t *iop_param);
static int
iopolicysys_vfs_materialize_dataless_files(struct proc *p, int cmd, int scope, int policy, struct _iopol_param_t *iop_param);
static int
iopolicysys_vfs_statfs_no_data_volume(struct proc *p, int cmd, int scope, int policy, struct _iopol_param_t *iop_param);
static int
iopolicysys_vfs_trigger_resolve(struct proc *p, int cmd, int scope, int policy, struct _iopol_param_t *iop_param);
static int
iopolicysys_vfs_ignore_content_protection(struct proc *p, int cmd, int scope, int policy, struct _iopol_param_t *iop_param);
static int
iopolicysys_vfs_ignore_node_permissions(struct proc *p, int cmd, int scope, int policy, struct _iopol_param_t *ipo_param);
static int
iopolicysys_vfs_skip_mtime_update(struct proc *p, int cmd, int scope, int policy, struct _iopol_param_t *iop_param);

  
              
  
                                                                                                              
  
                                    
                                      
  
                        
                                                         
  
   
int
iopolicysys(struct proc *p, struct iopolicysys_args *uap, int32_t *retval)
{
	int     error = 0;
	struct _iopol_param_t iop_param;

	if ((error = copyin(uap->arg, &iop_param, sizeof(iop_param))) != 0) {
		goto out;
	}

	switch (iop_param.iop_iotype) {
	case IOPOL_TYPE_DISK:
		error = iopolicysys_disk(p, uap->cmd, iop_param.iop_scope, iop_param.iop_policy, &iop_param);
		if (error == EIDRM) {
			*retval = -2;
			error = 0;
		}
		if (error) {
			goto out;
		}
		break;
	case IOPOL_TYPE_VFS_HFS_CASE_SENSITIVITY:
		error = iopolicysys_vfs_hfs_case_sensitivity(p, uap->cmd, iop_param.iop_scope, iop_param.iop_policy, &iop_param);
		if (error) {
			goto out;
		}
		break;
	case IOPOL_TYPE_VFS_ATIME_UPDATES:
		error = iopolicysys_vfs_atime_updates(p, uap->cmd, iop_param.iop_scope, iop_param.iop_policy, &iop_param);
		if (error) {
			goto out;
		}
		break;
	case IOPOL_TYPE_VFS_MATERIALIZE_DATALESS_FILES:
		error = iopolicysys_vfs_materialize_dataless_files(p, uap->cmd, iop_param.iop_scope, iop_param.iop_policy, &iop_param);
		if (error) {
			goto out;
		}
		break;
	case IOPOL_TYPE_VFS_STATFS_NO_DATA_VOLUME:
		error = iopolicysys_vfs_statfs_no_data_volume(p, uap->cmd, iop_param.iop_scope, iop_param.iop_policy, &iop_param);
		if (error) {
			goto out;
		}
		break;
	case IOPOL_TYPE_VFS_TRIGGER_RESOLVE:
		error = iopolicysys_vfs_trigger_resolve(p, uap->cmd, iop_param.iop_scope, iop_param.iop_policy, &iop_param);
		if (error) {
			goto out;
		}
		break;
	case IOPOL_TYPE_VFS_IGNORE_CONTENT_PROTECTION:
		error = iopolicysys_vfs_ignore_content_protection(p, uap->cmd, iop_param.iop_scope, iop_param.iop_policy, &iop_param);
		if (error) {
			goto out;
		}
		break;
	case IOPOL_TYPE_VFS_IGNORE_PERMISSIONS:
		error = iopolicysys_vfs_ignore_node_permissions(p, uap->cmd, iop_param.iop_scope, iop_param.iop_policy, &iop_param);
		if (error) {
			goto out;
		}
		break;
	case IOPOL_TYPE_VFS_SKIP_MTIME_UPDATE:
		error = iopolicysys_vfs_skip_mtime_update(p, uap->cmd, iop_param.iop_scope, iop_param.iop_policy, &iop_param);
		if (error) {
			goto out;
		}
		break;
	default:
		error = EINVAL;
		goto out;
	}

	                                                                                                  
	if (uap->cmd == IOPOL_CMD_GET) {
		error = copyout((caddr_t)&iop_param, uap->arg, sizeof(iop_param));
		if (error) {
			goto out;
		}
	}

out:
	return error;
}

static int
iopolicysys_disk(struct proc *p __unused, int cmd, int scope, int policy, struct _iopol_param_t *iop_param)
{
	int                     error = 0;
	thread_t        thread;
	int                     policy_flavor;

	                    
	switch (scope) {
	case IOPOL_SCOPE_PROCESS:
		thread = THREAD_NULL;
		policy_flavor = TASK_POLICY_IOPOL;
		break;

	case IOPOL_SCOPE_THREAD:
		thread = current_thread();
		policy_flavor = TASK_POLICY_IOPOL;

		                                                                                     
		if (cmd == IOPOL_CMD_SET && thread_has_qos_policy(thread)) {
			switch (policy) {
			case IOPOL_DEFAULT:
			case IOPOL_PASSIVE:
				break;
			case IOPOL_UTILITY:
			case IOPOL_THROTTLE:
			case IOPOL_IMPORTANT:
			case IOPOL_STANDARD:
				if (!thread_is_static_param(thread)) {
					thread_remove_qos_policy(thread);
					  
                                                                                
                                                
        
					error = EIDRM;
					break;
				}
				OS_FALLTHROUGH;
			default:
				error = EINVAL;
				goto out;
			}
		}
		break;

	case IOPOL_SCOPE_DARWIN_BG:
#if !defined(XNU_TARGET_OS_OSX)
		                                                                                     
		error = ENOTSUP;
		goto out;
#else                                  
		thread = THREAD_NULL;
		policy_flavor = TASK_POLICY_DARWIN_BG_IOPOL;
		break;
#endif                                  

	default:
		error = EINVAL;
		goto out;
	}

	                     
	if (cmd == IOPOL_CMD_SET) {
		switch (policy) {
		case IOPOL_DEFAULT:
			if (scope == IOPOL_SCOPE_DARWIN_BG) {
				                                                      
				policy = IOPOL_UTILITY;
			} else {
				policy = IOPOL_IMPORTANT;
			}
			break;
		case IOPOL_UTILITY:
		                  
		case IOPOL_THROTTLE:
			                         
			break;
		case IOPOL_IMPORTANT:
		                  
		case IOPOL_STANDARD:
		                  
		case IOPOL_PASSIVE:
			if (scope == IOPOL_SCOPE_DARWIN_BG) {
				                                     
				error = EINVAL;
				goto out;
			} else {
				                         
			}
			break;
		default:
			error = EINVAL;
			goto out;
		}
	}

	                     
	switch (cmd) {
	case IOPOL_CMD_SET:
		if (thread != THREAD_NULL) {
			proc_set_thread_policy(thread, TASK_POLICY_INTERNAL, policy_flavor, policy);
		} else {
			proc_set_task_policy(current_task(), TASK_POLICY_INTERNAL, policy_flavor, policy);
		}
		break;
	case IOPOL_CMD_GET:
		if (thread != THREAD_NULL) {
			policy = proc_get_thread_policy(thread, TASK_POLICY_INTERNAL, policy_flavor);
		} else {
			policy = proc_get_task_policy(current_task(), TASK_POLICY_INTERNAL, policy_flavor);
		}
		iop_param->iop_policy = policy;
		break;
	default:
		error = EINVAL;                              
		break;
	}

out:
	return error;
}

static int
iopolicysys_vfs_hfs_case_sensitivity(struct proc *p, int cmd, int scope, int policy, struct _iopol_param_t *iop_param)
{
	int                     error = 0;

	                    
	switch (scope) {
	case IOPOL_SCOPE_PROCESS:
		                     
		break;
	default:
		error = EINVAL;
		goto out;
	}

	                     
	if (cmd == IOPOL_CMD_SET) {
		switch (policy) {
		case IOPOL_VFS_HFS_CASE_SENSITIVITY_DEFAULT:
		                  
		case IOPOL_VFS_HFS_CASE_SENSITIVITY_FORCE_CASE_SENSITIVE:
			                           
			break;
		default:
			error = EINVAL;
			goto out;
		}
	}

	                     
	switch (cmd) {
	case IOPOL_CMD_SET:
		if (0 == kauth_cred_issuser(kauth_cred_get())) {
			                                                                                    
			boolean_t entitled = FALSE;
			entitled = IOTaskHasEntitlement(current_task(), "com.apple.private.iopol.case_sensitivity");
			if (!entitled) {
				error = EPERM;
				goto out;
			}
		}

		switch (policy) {
		case IOPOL_VFS_HFS_CASE_SENSITIVITY_DEFAULT:
			OSBitAndAtomic16(~((uint32_t)P_VFS_IOPOLICY_FORCE_HFS_CASE_SENSITIVITY), &p->p_vfs_iopolicy);
			break;
		case IOPOL_VFS_HFS_CASE_SENSITIVITY_FORCE_CASE_SENSITIVE:
			OSBitOrAtomic16((uint32_t)P_VFS_IOPOLICY_FORCE_HFS_CASE_SENSITIVITY, &p->p_vfs_iopolicy);
			break;
		default:
			error = EINVAL;
			goto out;
		}

		break;
	case IOPOL_CMD_GET:
		iop_param->iop_policy = (p->p_vfs_iopolicy & P_VFS_IOPOLICY_FORCE_HFS_CASE_SENSITIVITY)
		    ? IOPOL_VFS_HFS_CASE_SENSITIVITY_FORCE_CASE_SENSITIVE
		    : IOPOL_VFS_HFS_CASE_SENSITIVITY_DEFAULT;
		break;
	default:
		error = EINVAL;                              
		break;
	}

out:
	return error;
}

static inline int
get_thread_atime_policy(struct uthread *ut)
{
	return (ut->uu_flag & UT_ATIME_UPDATE) ? IOPOL_ATIME_UPDATES_OFF : IOPOL_ATIME_UPDATES_DEFAULT;
}

static inline void
set_thread_atime_policy(struct uthread *ut, int policy)
{
	if (policy == IOPOL_ATIME_UPDATES_OFF) {
		ut->uu_flag |= UT_ATIME_UPDATE;
	} else {
		ut->uu_flag &= ~UT_ATIME_UPDATE;
	}
}

static inline void
set_task_atime_policy(struct proc *p, int policy)
{
	if (policy == IOPOL_ATIME_UPDATES_OFF) {
		OSBitOrAtomic16((uint16_t)P_VFS_IOPOLICY_ATIME_UPDATES, &p->p_vfs_iopolicy);
	} else {
		OSBitAndAtomic16(~((uint16_t)P_VFS_IOPOLICY_ATIME_UPDATES), &p->p_vfs_iopolicy);
	}
}

static inline int
get_task_atime_policy(struct proc *p)
{
	return (p->p_vfs_iopolicy & P_VFS_IOPOLICY_ATIME_UPDATES) ? IOPOL_ATIME_UPDATES_OFF : IOPOL_ATIME_UPDATES_DEFAULT;
}

static int
iopolicysys_vfs_atime_updates(struct proc *p __unused, int cmd, int scope, int policy, struct _iopol_param_t *iop_param)
{
	int                     error = 0;
	thread_t                thread;

	                    
	switch (scope) {
	case IOPOL_SCOPE_THREAD:
		thread = current_thread();
		break;
	case IOPOL_SCOPE_PROCESS:
		thread = THREAD_NULL;
		break;
	default:
		error = EINVAL;
		goto out;
	}

	                     
	if (cmd == IOPOL_CMD_SET) {
		switch (policy) {
		case IOPOL_ATIME_UPDATES_DEFAULT:
		case IOPOL_ATIME_UPDATES_OFF:
			break;
		default:
			error = EINVAL;
			goto out;
		}
	}

	                     
	switch (cmd) {
	case IOPOL_CMD_SET:
		if (thread != THREAD_NULL) {
			set_thread_atime_policy(get_bsdthread_info(thread), policy);
		} else {
			set_task_atime_policy(p, policy);
		}
		break;
	case IOPOL_CMD_GET:
		if (thread != THREAD_NULL) {
			policy = get_thread_atime_policy(get_bsdthread_info(thread));
		} else {
			policy = get_task_atime_policy(p);
		}
		iop_param->iop_policy = policy;
		break;
	default:
		error = EINVAL;                              
		break;
	}

out:
	return error;
}

static inline int
get_thread_materialize_policy(struct uthread *ut)
{
	if (ut->uu_flag & UT_NSPACE_NODATALESSFAULTS) {
		return IOPOL_MATERIALIZE_DATALESS_FILES_OFF;
	} else if (ut->uu_flag & UT_NSPACE_FORCEDATALESSFAULTS) {
		return IOPOL_MATERIALIZE_DATALESS_FILES_ON;
	}
	                                                            
	return IOPOL_MATERIALIZE_DATALESS_FILES_DEFAULT;
}

static inline void
set_thread_materialize_policy(struct uthread *ut, int policy)
{
	if (policy == IOPOL_MATERIALIZE_DATALESS_FILES_OFF) {
		ut->uu_flag &= ~UT_NSPACE_FORCEDATALESSFAULTS;
		ut->uu_flag |= UT_NSPACE_NODATALESSFAULTS;
	} else if (policy == IOPOL_MATERIALIZE_DATALESS_FILES_ON) {
		ut->uu_flag &= ~UT_NSPACE_NODATALESSFAULTS;
		ut->uu_flag |= UT_NSPACE_FORCEDATALESSFAULTS;
	} else {
		ut->uu_flag &= ~(UT_NSPACE_NODATALESSFAULTS | UT_NSPACE_FORCEDATALESSFAULTS);
	}
}

static inline void
set_proc_materialize_policy(struct proc *p, int policy)
{
	if (policy == IOPOL_MATERIALIZE_DATALESS_FILES_DEFAULT) {
		  
                                                   
                                                      
           
     
		policy = IOPOL_MATERIALIZE_DATALESS_FILES_OFF;
	}
	if (policy == IOPOL_MATERIALIZE_DATALESS_FILES_ON) {
		OSBitOrAtomic16((uint16_t)P_VFS_IOPOLICY_MATERIALIZE_DATALESS_FILES, &p->p_vfs_iopolicy);
	} else {
		OSBitAndAtomic16(~((uint16_t)P_VFS_IOPOLICY_MATERIALIZE_DATALESS_FILES), &p->p_vfs_iopolicy);
	}
}

static int
get_proc_materialize_policy(struct proc *p)
{
	return (p->p_vfs_iopolicy & P_VFS_IOPOLICY_MATERIALIZE_DATALESS_FILES) ? IOPOL_MATERIALIZE_DATALESS_FILES_ON : IOPOL_MATERIALIZE_DATALESS_FILES_OFF;
}

static int
iopolicysys_vfs_materialize_dataless_files(struct proc *p __unused, int cmd, int scope, int policy, struct _iopol_param_t *iop_param)
{
	int                     error = 0;
	thread_t                thread;

	                    
	switch (scope) {
	case IOPOL_SCOPE_THREAD:
		thread = current_thread();
		break;
	case IOPOL_SCOPE_PROCESS:
		thread = THREAD_NULL;
		break;
	default:
		error = EINVAL;
		goto out;
	}

	                     
	if (cmd == IOPOL_CMD_SET) {
		switch (policy) {
		case IOPOL_MATERIALIZE_DATALESS_FILES_DEFAULT:
		case IOPOL_MATERIALIZE_DATALESS_FILES_OFF:
		case IOPOL_MATERIALIZE_DATALESS_FILES_ON:
			break;
		default:
			error = EINVAL;
			goto out;
		}
	}

	                     
	switch (cmd) {
	case IOPOL_CMD_SET:
		if (thread != THREAD_NULL) {
			set_thread_materialize_policy(get_bsdthread_info(thread), policy);
		} else {
			set_proc_materialize_policy(p, policy);
		}
		break;
	case IOPOL_CMD_GET:
		if (thread != THREAD_NULL) {
			policy = get_thread_materialize_policy(get_bsdthread_info(thread));
		} else {
			policy = get_proc_materialize_policy(p);
		}
		iop_param->iop_policy = policy;
		break;
	default:
		error = EINVAL;                              
		break;
	}

out:
	return error;
}

static int
iopolicysys_vfs_statfs_no_data_volume(struct proc *p __unused, int cmd,
    int scope, int policy, struct _iopol_param_t *iop_param)
{
	int error = 0;

	                    
	switch (scope) {
	case IOPOL_SCOPE_PROCESS:
		                     
		break;
	default:
		error = EINVAL;
		goto out;
	}

	                     
	if (cmd == IOPOL_CMD_SET) {
		switch (policy) {
		case IOPOL_VFS_STATFS_NO_DATA_VOLUME_DEFAULT:
		                  
		case IOPOL_VFS_STATFS_FORCE_NO_DATA_VOLUME:
			                           
			break;
		default:
			error = EINVAL;
			goto out;
		}
	}

	                     
	switch (cmd) {
	case IOPOL_CMD_SET:
		if (0 == kauth_cred_issuser(kauth_cred_get())) {
			                                                                                    
			boolean_t entitled = FALSE;
			entitled = IOTaskHasEntitlement(current_task(), "com.apple.private.iopol.case_sensitivity");
			if (!entitled) {
				error = EPERM;
				goto out;
			}
		}

		switch (policy) {
		case IOPOL_VFS_STATFS_NO_DATA_VOLUME_DEFAULT:
			OSBitAndAtomic16(~((uint32_t)P_VFS_IOPOLICY_STATFS_NO_DATA_VOLUME), &p->p_vfs_iopolicy);
			break;
		case IOPOL_VFS_STATFS_FORCE_NO_DATA_VOLUME:
			OSBitOrAtomic16((uint32_t)P_VFS_IOPOLICY_STATFS_NO_DATA_VOLUME, &p->p_vfs_iopolicy);
			break;
		default:
			error = EINVAL;
			goto out;
		}

		break;
	case IOPOL_CMD_GET:
		iop_param->iop_policy = (p->p_vfs_iopolicy & P_VFS_IOPOLICY_STATFS_NO_DATA_VOLUME)
		    ? IOPOL_VFS_STATFS_FORCE_NO_DATA_VOLUME
		    : IOPOL_VFS_STATFS_NO_DATA_VOLUME_DEFAULT;
		break;
	default:
		error = EINVAL;                              
		break;
	}

out:
	return error;
}

static int
iopolicysys_vfs_trigger_resolve(struct proc *p __unused, int cmd,
    int scope, int policy, struct _iopol_param_t *iop_param)
{
	int error = 0;

	                    
	switch (scope) {
	case IOPOL_SCOPE_PROCESS:
		                     
		break;
	default:
		error = EINVAL;
		goto out;
	}

	                     
	if (cmd == IOPOL_CMD_SET) {
		switch (policy) {
		case IOPOL_VFS_TRIGGER_RESOLVE_DEFAULT:
		                  
		case IOPOL_VFS_TRIGGER_RESOLVE_OFF:
			                           
			break;
		default:
			error = EINVAL;
			goto out;
		}
	}

	                     
	switch (cmd) {
	case IOPOL_CMD_SET:
		switch (policy) {
		case IOPOL_VFS_TRIGGER_RESOLVE_DEFAULT:
			OSBitAndAtomic16(~((uint32_t)P_VFS_IOPOLICY_TRIGGER_RESOLVE_DISABLE), &p->p_vfs_iopolicy);
			break;
		case IOPOL_VFS_TRIGGER_RESOLVE_OFF:
			OSBitOrAtomic16((uint32_t)P_VFS_IOPOLICY_TRIGGER_RESOLVE_DISABLE, &p->p_vfs_iopolicy);
			break;
		default:
			error = EINVAL;
			goto out;
		}

		break;
	case IOPOL_CMD_GET:
		iop_param->iop_policy = (p->p_vfs_iopolicy & P_VFS_IOPOLICY_TRIGGER_RESOLVE_DISABLE)
		    ? IOPOL_VFS_TRIGGER_RESOLVE_OFF
		    : IOPOL_VFS_TRIGGER_RESOLVE_DEFAULT;
		break;
	default:
		error = EINVAL;                              
		break;
	}

out:
	return error;
}

static int
iopolicysys_vfs_ignore_content_protection(struct proc *p, int cmd, int scope,
    int policy, struct _iopol_param_t *iop_param)
{
	int error = 0;

	                    
	switch (scope) {
	case IOPOL_SCOPE_PROCESS:
		                     
		break;
	default:
		error = EINVAL;
		goto out;
	}

	                     
	if (cmd == IOPOL_CMD_SET) {
		switch (policy) {
		case IOPOL_VFS_CONTENT_PROTECTION_DEFAULT:
			OS_FALLTHROUGH;
		case IOPOL_VFS_CONTENT_PROTECTION_IGNORE:
			                           
			break;
		default:
			error = EINVAL;
			goto out;
		}
	}

	                     
	switch (cmd) {
	case IOPOL_CMD_SET:
		if (0 == kauth_cred_issuser(kauth_cred_get())) {
			                                                                                    
			boolean_t entitled = FALSE;
			entitled = IOTaskHasEntitlement(current_task(), "com.apple.private.iopol.case_sensitivity");
			if (!entitled) {
				error = EPERM;
				goto out;
			}
		}

		switch (policy) {
		case IOPOL_VFS_CONTENT_PROTECTION_DEFAULT:
			os_atomic_andnot(&p->p_vfs_iopolicy, P_VFS_IOPOLICY_IGNORE_CONTENT_PROTECTION, relaxed);
			break;
		case IOPOL_VFS_CONTENT_PROTECTION_IGNORE:
			os_atomic_or(&p->p_vfs_iopolicy, P_VFS_IOPOLICY_IGNORE_CONTENT_PROTECTION, relaxed);
			break;
		default:
			error = EINVAL;
			goto out;
		}

		break;
	case IOPOL_CMD_GET:
		iop_param->iop_policy = (os_atomic_load(&p->p_vfs_iopolicy, relaxed) & P_VFS_IOPOLICY_IGNORE_CONTENT_PROTECTION)
		    ? IOPOL_VFS_CONTENT_PROTECTION_IGNORE
		    : IOPOL_VFS_CONTENT_PROTECTION_DEFAULT;
		break;
	default:
		error = EINVAL;                              
		break;
	}

out:
	return error;
}

#define AUTHORIZED_ACCESS_ENTITLEMENT \
	"com.apple.private.vfs.authorized-access"
int
iopolicysys_vfs_ignore_node_permissions(struct proc *p, int cmd, int scope,
    int policy, __unused struct _iopol_param_t *iop_param)
{
	int error = EINVAL;

	switch (scope) {
	case IOPOL_SCOPE_PROCESS:
		break;
	default:
		goto out;
	}

	switch (cmd) {
	case IOPOL_CMD_GET:
		policy = os_atomic_load(&p->p_vfs_iopolicy, relaxed) & P_VFS_IOPOLICY_IGNORE_NODE_PERMISSIONS ?
		    IOPOL_VFS_IGNORE_PERMISSIONS_ON : IOPOL_VFS_IGNORE_PERMISSIONS_OFF;
		iop_param->iop_policy = policy;
		goto out_ok;
	case IOPOL_CMD_SET:
		                                     
		break;
	default:
		goto out;
	}

	if (!IOTaskHasEntitlement(current_task(), AUTHORIZED_ACCESS_ENTITLEMENT)) {
		error = EPERM;
		goto out;
	}

	switch (policy) {
	case IOPOL_VFS_IGNORE_PERMISSIONS_OFF:
		os_atomic_andnot(&p->p_vfs_iopolicy, P_VFS_IOPOLICY_IGNORE_NODE_PERMISSIONS, relaxed);
		break;
	case IOPOL_VFS_IGNORE_PERMISSIONS_ON:
		os_atomic_or(&p->p_vfs_iopolicy, P_VFS_IOPOLICY_IGNORE_NODE_PERMISSIONS, relaxed);
		break;
	default:
		break;
	}

out_ok:
	error = 0;
out:
	return error;
}

#define SKIP_MTIME_UPDATE_ENTITLEMENT \
	"com.apple.private.vfs.skip-mtime-updates"
int
iopolicysys_vfs_skip_mtime_update(struct proc *p, int cmd, int scope,
    int policy, __unused struct _iopol_param_t *iop_param)
{
	int error = EINVAL;

	switch (scope) {
	case IOPOL_SCOPE_PROCESS:
		break;
	default:
		goto out;
	}

	switch (cmd) {
	case IOPOL_CMD_GET:
		policy = os_atomic_load(&p->p_vfs_iopolicy, relaxed) & P_VFS_IOPOLICY_SKIP_MTIME_UPDATE ?
		    IOPOL_VFS_SKIP_MTIME_UPDATE_ON : IOPOL_VFS_SKIP_MTIME_UPDATE_OFF;
		iop_param->iop_policy = policy;
		goto out_ok;
	case IOPOL_CMD_SET:
		break;
	default:
		break;
	}

	if (!IOTaskHasEntitlement(current_task(), SKIP_MTIME_UPDATE_ENTITLEMENT)) {
		error = EPERM;
		goto out;
	}

	switch (policy) {
	case IOPOL_VFS_SKIP_MTIME_UPDATE_OFF:
		os_atomic_andnot(&p->p_vfs_iopolicy, P_VFS_IOPOLICY_SKIP_MTIME_UPDATE, relaxed);
		break;
	case IOPOL_VFS_SKIP_MTIME_UPDATE_ON:
		os_atomic_or(&p->p_vfs_iopolicy, P_VFS_IOPOLICY_SKIP_MTIME_UPDATE, relaxed);
		break;
	default:
		break;
	}

out_ok:
	error = 0;
out:
	return error;
}
                                                               
void
proc_apply_task_networkbg(void * bsd_info, thread_t thread)
{
	assert(bsd_info != PROC_NULL);

	pid_t pid = proc_pid((proc_t)bsd_info);

	proc_t p = proc_find(pid);

	if (p != PROC_NULL) {
		assert(p == (proc_t)bsd_info);

		do_background_socket(p, thread);
		proc_rele(p);
	}
}

void
gather_rusage_info(proc_t p, rusage_info_current *ru, int flavor)
{
	struct rusage_info_child *ri_child;

	assert(p->p_stats != NULL);
	memset(ru, 0, sizeof(*ru));
	switch (flavor) {
	case RUSAGE_INFO_V5:
#if !XNU_TARGET_OS_OSX && __has_feature(ptrauth_calls)
		if (vm_shared_region_is_reslide(p->task)) {
			ru->ri_flags |= RU_PROC_RUNS_RESLIDE;
		}
#endif                                                         
		OS_FALLTHROUGH;
	case RUSAGE_INFO_V4:
		ru->ri_logical_writes = get_task_logical_writes(p->task, FALSE);
		ru->ri_lifetime_max_phys_footprint = get_task_phys_footprint_lifetime_max(p->task);
#if CONFIG_LEDGER_INTERVAL_MAX
		ru->ri_interval_max_phys_footprint = get_task_phys_footprint_interval_max(p->task, FALSE);
#endif
		fill_task_monotonic_rusage(p->task, ru);
		OS_FALLTHROUGH;

	case RUSAGE_INFO_V3:
		fill_task_qos_rusage(p->task, ru);
		fill_task_billed_usage(p->task, ru);
		OS_FALLTHROUGH;

	case RUSAGE_INFO_V2:
		fill_task_io_rusage(p->task, ru);
		OS_FALLTHROUGH;

	case RUSAGE_INFO_V1:
		  
                                                                   
     
		proc_lock(p);

		ri_child = &(p->p_stats->ri_child);
		ru->ri_child_user_time = ri_child->ri_child_user_time;
		ru->ri_child_system_time = ri_child->ri_child_system_time;
		ru->ri_child_pkg_idle_wkups = ri_child->ri_child_pkg_idle_wkups;
		ru->ri_child_interrupt_wkups = ri_child->ri_child_interrupt_wkups;
		ru->ri_child_pageins = ri_child->ri_child_pageins;
		ru->ri_child_elapsed_abstime = ri_child->ri_child_elapsed_abstime;

		proc_unlock(p);
		OS_FALLTHROUGH;

	case RUSAGE_INFO_V0:
		proc_getexecutableuuid(p, (unsigned char *)&ru->ri_uuid, sizeof(ru->ri_uuid));
		fill_task_rusage(p->task, ru);
		ru->ri_proc_start_abstime = p->p_stats->ps_start;
	}
}

int
proc_get_rusage(proc_t p, int flavor, user_addr_t buffer, __unused int is_zombie)
{
	rusage_info_current ri_current = {};

	int error = 0;
	size_t size = 0;

	switch (flavor) {
	case RUSAGE_INFO_V0:
		size = sizeof(struct rusage_info_v0);
		break;

	case RUSAGE_INFO_V1:
		size = sizeof(struct rusage_info_v1);
		break;

	case RUSAGE_INFO_V2:
		size = sizeof(struct rusage_info_v2);
		break;

	case RUSAGE_INFO_V3:
		size = sizeof(struct rusage_info_v3);
		break;

	case RUSAGE_INFO_V4:
		size = sizeof(struct rusage_info_v4);
		break;

	case RUSAGE_INFO_V5:
		size = sizeof(struct rusage_info_v5);
		break;
	default:
		return EINVAL;
	}

	if (size == 0) {
		return EINVAL;
	}

	  
                                                                   
                                                          
    
	if (p->p_ru == NULL) {
		gather_rusage_info(p, &ri_current, flavor);
		ri_current.ri_proc_exit_abstime = 0;
		error = copyout(&ri_current, buffer, size);
	} else {
		ri_current = p->p_ru->ri;
		error = copyout(&p->p_ru->ri, buffer, size);
	}

	return error;
}

static int
mach_to_bsd_rv(int mach_rv)
{
	int bsd_rv = 0;

	switch (mach_rv) {
	case KERN_SUCCESS:
		bsd_rv = 0;
		break;
	case KERN_INVALID_ARGUMENT:
		bsd_rv = EINVAL;
		break;
	default:
		panic("unknown error %#x", mach_rv);
	}

	return bsd_rv;
}

  
                          
  
                                 
  
                             
                               
                              
                                
   
int
proc_rlimit_control(__unused struct proc *p, struct proc_rlimit_control_args *uap, __unused int32_t *retval)
{
	proc_t  targetp;
	int     error = 0;
	struct  proc_rlimit_control_wakeupmon wakeupmon_args;
	uint32_t cpumon_flags;
	uint32_t cpulimits_flags;
	kauth_cred_t my_cred, target_cred;
#if CONFIG_LEDGER_INTERVAL_MAX
	uint32_t footprint_interval_flags;
	uint64_t interval_max_footprint;
#endif                                 

	                                                                                                     
	if (uap->pid == -1) {
		targetp = proc_self();
	} else {
		targetp = proc_find(uap->pid);
	}

	                                                        
	if (targetp == PROC_NULL) {
		return ESRCH;
	}

	my_cred = kauth_cred_get();
	target_cred = kauth_cred_proc_ref(targetp);

	if (!kauth_cred_issuser(my_cred) && kauth_cred_getruid(my_cred) &&
	    kauth_cred_getuid(my_cred) != kauth_cred_getuid(target_cred) &&
	    kauth_cred_getruid(my_cred) != kauth_cred_getuid(target_cred)) {
		proc_rele(targetp);
		kauth_cred_unref(&target_cred);
		return EACCES;
	}

	switch (uap->flavor) {
	case RLIMIT_WAKEUPS_MONITOR:
		if ((error = copyin(uap->arg, &wakeupmon_args, sizeof(wakeupmon_args))) != 0) {
			break;
		}
		if ((error = mach_to_bsd_rv(task_wakeups_monitor_ctl(targetp->task, &wakeupmon_args.wm_flags,
		    &wakeupmon_args.wm_rate))) != 0) {
			break;
		}
		error = copyout(&wakeupmon_args, uap->arg, sizeof(wakeupmon_args));
		break;
	case RLIMIT_CPU_USAGE_MONITOR:
		cpumon_flags = (uint32_t)uap->arg;                                                     
		error = mach_to_bsd_rv(task_cpu_usage_monitor_ctl(targetp->task, &cpumon_flags));
		break;
	case RLIMIT_THREAD_CPULIMITS:
		cpulimits_flags = (uint32_t)uap->arg;                                                            

		if (uap->pid != -1) {
			error = EINVAL;
			break;
		}

		uint8_t percent = 0;
		uint32_t ms_refill = 0;
		uint64_t ns_refill;

		percent = (uint8_t)(cpulimits_flags & 0xffU);                                                               
		ms_refill = (cpulimits_flags >> 8) & 0xffffff;                                                      
		if (percent >= 100) {
			error = EINVAL;
			break;
		}

		ns_refill = ((uint64_t)ms_refill) * NSEC_PER_MSEC;

		error = mach_to_bsd_rv(thread_set_cpulimit(THREAD_CPULIMIT_BLOCK, percent, ns_refill));
		break;

#if CONFIG_LEDGER_INTERVAL_MAX
	case RLIMIT_FOOTPRINT_INTERVAL:
		footprint_interval_flags = (uint32_t)uap->arg;                                                     
		  
                                                        
     
		if ((footprint_interval_flags & FOOTPRINT_INTERVAL_RESET) == 0) {
			error = EINVAL;
			break;
		}
		interval_max_footprint = get_task_phys_footprint_interval_max(targetp->task, TRUE);
		break;
#endif                                 
	default:
		error = EINVAL;
		break;
	}

	proc_rele(targetp);
	kauth_cred_unref(&target_cred);

	  
                                                                     
    
	return error;
}

  
                                                                                           
   
int
thread_selfusage(struct proc *p __unused, struct thread_selfusage_args *uap __unused, uint64_t *retval)
{
	uint64_t runtime;

	runtime = thread_get_runtime_self();
	*retval = runtime;

	return 0;
}

#if !MONOTONIC
int
thread_selfcounts(__unused struct proc *p, __unused struct thread_selfcounts_args *uap, __unused int *ret_out)
{
	return ENOTSUP;
}
#endif                 
