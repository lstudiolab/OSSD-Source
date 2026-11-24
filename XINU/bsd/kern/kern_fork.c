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
/*
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
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
 *	@(#)kern_fork.c	8.8 (Berkeley) 2/14/95
 */
  
                                                                         
                                                                          
                                                                        
               
   
  
                                                                      
                                                                          
                                                                        
               
   

#include <kern/assert.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/user.h>
#include <sys/reason.h>
#include <sys/resourcevar.h>
#include <sys/vnode_internal.h>
#include <sys/file_internal.h>
#include <sys/acct.h>
#include <sys/codesign.h>
#include <sys/sysproto.h>
#if CONFIG_PERSONAS
#include <sys/persona.h>
#endif
#include <sys/doc_tombstone.h>
#if CONFIG_DTRACE
                                                             
extern void (*dtrace_proc_waitfor_exec_ptr)(proc_t);
extern void dtrace_proc_fork(proc_t, proc_t, int);

  
                                                                            
                                                      
   
static void (*dtrace_proc_waitfor_hook)(proc_t) = NULL;

#include <sys/dtrace_ptss.h>
#endif

#include <security/audit/audit.h>

#include <mach/mach_types.h>
#include <kern/coalition.h>
#include <kern/kern_types.h>
#include <kern/kalloc.h>
#include <kern/mach_param.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/thread_call.h>
#include <kern/zalloc.h>

#include <os/log.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#include <security/mac_mach_internal.h>
#endif

#include <vm/vm_map.h>
#include <vm/vm_protos.h>
#include <vm/vm_shared_region.h>

#include <sys/shm_internal.h>                      
#include <mach/task.h>                                   
#include <mach/thread_act.h>                             

#include <sys/sdt.h>

#if CONFIG_MEMORYSTATUS
#include <sys/kern_memorystatus.h>
#endif

                                                               
void thread_set_parent(thread_t parent, int pid);
extern void act_thread_catt(void *ctx);
void thread_set_child(thread_t child, int pid);
void *act_thread_csave(void);
extern boolean_t task_is_exec_copy(task_t);
int nextpidversion = 0;


thread_t cloneproc(task_t, coalition_t *, proc_t, int, int);
proc_t forkproc(proc_t);
void forkproc_free(proc_t);
thread_t fork_create_child(task_t parent_task,
    coalition_t *parent_coalitions,
    proc_t child,
    int inherit_memory,
    int is_64bit_addr,
    int is_64bit_data,
    int in_exec);
void proc_vfork_begin(proc_t parent_proc);
void proc_vfork_end(proc_t parent_proc);

static LCK_GRP_DECLARE(rethrottle_lock_grp, "rethrottle");
static ZONE_DECLARE(uthread_zone, "uthreads",
    sizeof(struct uthread), ZC_ZFREE_CLEARMEM);

SECURITY_READ_ONLY_LATE(zone_t) proc_zone;
ZONE_INIT(&proc_zone, "proc", sizeof(struct proc), ZC_ZFREE_CLEARMEM,
    ZONE_ID_PROC, NULL);

ZONE_DECLARE(proc_stats_zone, "pstats",
    sizeof(struct pstats), ZC_NOENCRYPT | ZC_ZFREE_CLEARMEM);

ZONE_DECLARE(proc_sigacts_zone, "sigacts",
    sizeof(struct sigacts), ZC_NOENCRYPT);

#define DOFORK  0x1                             
#define DOVFORK 0x2                              

  
                   
  
                                          
  
                                                            
  
                  
  
                                                               
                                                             
                                                           
                                                          
                                                    
   
void
proc_vfork_begin(proc_t parent_proc)
{
	proc_lock(parent_proc);
	parent_proc->p_lflag  |= P_LVFORK;
	parent_proc->p_vforkcnt++;
	proc_unlock(parent_proc);
}

  
                 
  
                                         
  
                                                       
  
                  
  
                                                                
                                         
   
void
proc_vfork_end(proc_t parent_proc)
{
	proc_lock(parent_proc);
	parent_proc->p_vforkcnt--;
	if (parent_proc->p_vforkcnt < 0) {
		panic("vfork cnt is -ve");
	}
	if (parent_proc->p_vforkcnt == 0) {
		parent_proc->p_lflag  &= ~P_LVFORK;
	}
	proc_unlock(parent_proc);
}


  
        
  
                                 
  
                                    
  
                                 
                                         
                               
  
                                                 
                                          
                                           
  
                                                                     
                                                                
                                                              
                                                              
                                                
  
                                                                 
                                                                  
                                                             
                                                               
                                                                
                                                                 
                                                                 
            
  
                         
  
          
  
                                                 
                                                 
                                                 
                                                 
                                                 
                                            
                                                 
                                                 
                                                 
                                                                    
                                                 
                                                 
               
               
                 
  
         
  
                                                             
                                                             
                                                             
                                                             
                                                             
                                                        
                                                             
                                                             
                                                             
                                                             
                                                             
                                                             
                                   
                              
                                     
                                     
                                                        
                                     
                                     
   
int
vfork(proc_t parent_proc, __unused struct vfork_args *uap, int32_t *retval)
{
	thread_t child_thread;
	int err;

	if ((err = fork1(parent_proc, &child_thread, PROC_CREATE_VFORK, NULL)) != 0) {
		retval[1] = 0;
	} else {
		uthread_t ut = get_bsdthread_info(current_thread());
		proc_t child_proc = ut->uu_proc;

		retval[0] = child_proc->p_pid;
		retval[1] = 1;                                                

		  
                                                             
                                                              
                                                            
     
		proc_signalend(child_proc, 0);
		proc_transend(child_proc, 0);

		proc_knote(parent_proc, NOTE_FORK | child_proc->p_pid);
		DTRACE_PROC1(create, proc_t, child_proc);
		ut->uu_flag &= ~UT_VFORKING;
	}

	return err;
}


  
        
  
                                                                           
                                                  
  
                                                               
                                                     
                                         
              
                                           
                                                   
                                            
                                           
                                          
                                   
                  
  
                                        
  
                                                         
                                          
                                             
                                
                                                          
                                            
                                            
                                            
                                              
                                    
                                                          
                                          
                                              
                                              
                                              
                                              
                                       
               
  
                                                                
                                                               
                                                              
                                                                 
                                                                
                                                             
                                  
   
int
fork1(proc_t parent_proc, thread_t *child_threadp, int kind, coalition_t *coalitions)
{
	thread_t parent_thread = (thread_t)current_thread();
	uthread_t parent_uthread = (uthread_t)get_bsdthread_info(parent_thread);
	proc_t child_proc = NULL;                                           
	thread_t child_thread = NULL;
	uid_t uid;
	size_t count;
	int err = 0;
	int spawn = 0;
	rlim_t rlimit_nproc_cur;

	  
                                                                   
                                                                     
                                                                
                                                                  
                                    
    
	uid = kauth_getruid();
	proc_list_lock();
	if ((nprocs >= maxproc - 1 && uid != 0) || nprocs >= maxproc) {
#if (DEVELOPMENT || DEBUG) && !defined(XNU_TARGET_OS_OSX)
		  
                                                                  
                                                                  
            
     
		panic("The process table is full; parent pid=%d", parent_proc->p_pid);
#endif
		proc_list_unlock();
		tablefull("proc");
		return EAGAIN;
	}
	proc_list_unlock();

	  
                                                                   
                                                                
                                             
                                                                    
    
	count = chgproccnt(uid, 1);
	rlimit_nproc_cur = proc_limitgetcur(parent_proc, RLIMIT_NPROC, TRUE);
	if (uid != 0 &&
	    (rlim_t)count > rlimit_nproc_cur) {
#if (DEVELOPMENT || DEBUG) && !defined(XNU_TARGET_OS_OSX)
		  
                                                                  
                                                                  
                                                                 
          
     
		panic("The per-user process limit has been hit; parent pid=%d, uid=%d", parent_proc->p_pid, uid);
#endif
		err = EAGAIN;
		goto bad;
	}

#if CONFIG_MACF
	  
                                                               
                                                
    
	err = mac_proc_check_fork(parent_proc);
	if (err != 0) {
		goto bad;
	}
#endif

	switch (kind) {
	case PROC_CREATE_VFORK:
		  
                                                       
                                                         
                                                       
                                                      
                                                         
                                                     
                                                       
                                                     
                                                     
                                                       
               
    
                                                        
     
		if (parent_uthread->uu_flag & UT_VFORK) {
			printf("fork1 called within vfork by %s\n", parent_proc->p_comm);
			err = EINVAL;
			goto bad;
		}

		  
                                                           
                                                         
                                                             
                                                         
     
		proc_vfork_begin(parent_proc);

		                                                           
		if ((child_proc = forkproc(parent_proc)) == NULL) {
			                                    
			proc_vfork_end(parent_proc);
			err = ENOMEM;
			goto bad;
		}

                                                        
#if CONFIG_MACF
		  
                                                          
                                                     
                                                      
                                                            
                                                                
     
		mac_cred_label_associate_fork(child_proc->p_ucred, child_proc);
#endif

		  
                                                            
    
                                                      
                                                            
                                                          
                                                            
                                                         
                                                         
          
    
                                                              
     
		set_security_token(child_proc);

		AUDIT_ARG(pid, child_proc->p_pid);

                                                      

		  
                                                  
    
                                                                 
                                                            
                                 
     
		child_proc->task = parent_proc->task;

		child_proc->p_lflag  |= P_LINVFORK;
		child_proc->p_vforkact = parent_thread;
		child_proc->p_stat = SRUN;

		  
                                                         
                                                             
           
    
                                                            
                                                                 
    
                                                            
                                                            
                                                            
                          
     
		parent_uthread->uu_flag |= UT_VFORK | UT_VFORKING;
		parent_uthread->uu_proc = child_proc;
		parent_uthread->uu_userstate = (void *)act_thread_csave();
		parent_uthread->uu_vforkmask = parent_uthread->uu_sigmask;

		                                          
		if (parent_uthread->uu_flag & UT_SETUID) {
			parent_uthread->uu_flag |= UT_WASSETUID;
			parent_uthread->uu_flag &= ~UT_SETUID;
		}

		                                   
		                                                           
		thread_set_child(parent_thread, child_proc->p_pid);

		child_proc->p_acflag = AFORK;                               

		  
                                                     
                                                    
                                                       
     
		child_proc->p_lflag |= P_LPPWAIT;
		pinsertchild(parent_proc, child_proc);                   

		break;

	case PROC_CREATE_SPAWN:
		  
                                                            
                                                          
                                                          
               
     
		spawn = 1;

		OS_FALLTHROUGH;

	case PROC_CREATE_FORK:
		  
                                                              
                                                           
                                                               
                                                                
     
		if ((child_thread = cloneproc(parent_proc->task,
		    spawn ? coalitions : NULL,
		    parent_proc,
		    spawn ? FALSE : TRUE,
		    FALSE)) == NULL) {
			                             
			err = EAGAIN;
			goto bad;
		}

		                                                                     
		if (!spawn) {
			thread_dup(child_thread);
		}

		                                            
		child_proc = (proc_t)(get_bsdtask_info(get_threadtask(child_thread)));

                                                        
#if CONFIG_MACF
		  
                                                          
                                                     
                                                      
                                                            
                                                                
     
		mac_cred_label_associate_fork(child_proc->p_ucred, child_proc);
#endif

		  
                                                            
    
                                                      
                                                            
                                                          
                                                            
                                                         
                                                         
          
    
                                                              
     
		set_security_token(child_proc);

		AUDIT_ARG(pid, child_proc->p_pid);

                                                      

		  
                                                                
                                                   
    
                                                               
                                                         
                                                         
                                                            
     
		thread_set_child(child_thread, child_proc->p_pid);

		child_proc->p_acflag = AFORK;                               

#if CONFIG_DTRACE
		dtrace_proc_fork(parent_proc, child_proc, spawn);
#endif                     
		if (!spawn) {
			  
                                                            
                                                                        
      
			task_bank_init(get_threadtask(child_thread));
		}

		break;

	default:
		panic("fork1 called with unknown kind %d", kind);
		break;
	}


	                                             
	*child_threadp = child_thread;

bad:
	  
                                                                    
                                                                  
                              
    
	if (err) {
		(void)chgproccnt(uid, -1);
	}

	return err;
}


  
               
  
                                                                         
                                                              
                                       
  
                                                          
                        
  
                                        
                                           
                                            
  
                
  
                                                                       
                          
   
void
vfork_return(proc_t child_proc, int32_t *retval, int rval)
{
	task_t parent_task = get_threadtask(child_proc->p_vforkact);
	proc_t parent_proc = get_bsdtask_info(parent_task);
	thread_t th = current_thread();
	uthread_t uth = get_bsdthread_info(th);

	act_thread_catt(uth->uu_userstate);

	                                                
	proc_vfork_end(parent_proc);

	                                             
	uth->uu_userstate = 0;
	uth->uu_flag &= ~UT_VFORK;
	                                 
	if (uth->uu_flag & UT_WASSETUID) {
		uth->uu_flag |= UT_SETUID;
		uth->uu_flag &= ~UT_WASSETUID;
	}
	uth->uu_proc = 0;
	uth->uu_sigmask = uth->uu_vforkmask;

	proc_lock(child_proc);
	child_proc->p_lflag &= ~P_LINVFORK;
	child_proc->p_vforkact = 0;
	proc_unlock(child_proc);

	thread_set_parent(th, rval);

	if (retval) {
		retval[0] = rval;
		retval[1] = 0;                                   
	}
}


  
                    
  
                                                                         
           
  
                                       
                                                
                              
                                                         
                                     
                                                        
                                                  
                                                             
                                                                              
                                                                  
                                             
  
                                                                       
                                                                
                                                             
                                                              
  
                                                                 
                                                                     
                                                            
                                                         
                                                                  
                   
  
                                                                
                                                                   
                                                 
   
thread_t
fork_create_child(task_t parent_task,
    coalition_t *parent_coalitions,
    proc_t child_proc,
    int inherit_memory,
    int is_64bit_addr,
    int is_64bit_data,
    int in_exec)
{
	thread_t        child_thread = NULL;
	task_t          child_task;
	kern_return_t   result;

	                                             
	result = task_create_internal(parent_task,
	    parent_coalitions,
	    inherit_memory,
	    is_64bit_addr,
	    is_64bit_data,
	    TF_NONE,
	    in_exec ? TPF_EXEC_COPY : TPF_NONE,                                                                  
	    (TRW_LRETURNWAIT | TRW_LRETURNWAITER),                                                                               
	    &child_task);
	if (result != KERN_SUCCESS) {
		printf("%s: task_create_internal failed.  Code: %d\n",
		    __func__, result);
		goto bad;
	}

	if (!in_exec) {
		  
                                                               
                                                                                     
     
		child_proc->task = child_task;
	}

	                                          
	set_bsdtask_info(child_task, child_proc);

	                                           
	if (timerisset(&child_proc->p_rlim_cpu)) {
		task_vtimer_set(child_task, TASK_VTIMER_RLIM);
	}

	  
                                                                  
                         
    
	if (child_proc->p_nice != 0) {
		resetpriority(child_proc);
	}

	  
                                                                            
                                                                                
    
	result = thread_create_waiting(child_task,
	    (thread_continue_t)task_wait_to_return,
	    task_get_return_wait_event(child_task),
	    TH_CREATE_WAITING_OPTION_PINNED | TH_CREATE_WAITING_OPTION_IMMOVABLE,
	    &child_thread);

	if (result != KERN_SUCCESS) {
		printf("%s: thread_create failed. Code: %d\n",
		    __func__, result);
		task_deallocate(child_task);
		child_task = NULL;
	}

	  
                                                     
    
	thread_set_tag(child_thread, THREAD_TAG_MAINTHREAD);

bad:
	thread_yield_internal(1);

	return child_thread;
}


  
       
  
                                 
  
                                              
                        
                         
  
                       
                                            
  
                                                                     
                                                             
                                                               
                                                              
                                                               
                          
  
                                                           
                                                         
                                                                
                                                              
                
  
                                                                
                                                            
  
                                                                       
                                                              
                                                                
                                                               
                                                  
   
int
fork(proc_t parent_proc, __unused struct fork_args *uap, int32_t *retval)
{
	thread_t child_thread;
	int err;

	retval[1] = 0;                                                 

	if ((err = fork1(parent_proc, &child_thread, PROC_CREATE_FORK, NULL)) == 0) {
		task_t child_task;
		proc_t child_proc;

		                          
		child_proc = (proc_t)get_bsdthreadtask_info(child_thread);
		retval[0] = child_proc->p_pid;

		  
                                                             
                                                              
                                                            
     
		proc_signalend(child_proc, 0);
		proc_transend(child_proc, 0);

		                                
		proc_knote(parent_proc, NOTE_FORK | child_proc->p_pid);
		DTRACE_PROC1(create, proc_t, child_proc);

#if CONFIG_DTRACE
		if ((dtrace_proc_waitfor_hook = dtrace_proc_waitfor_exec_ptr) != NULL) {
			(*dtrace_proc_waitfor_hook)(child_proc);
		}
#endif

		                           
		task_clear_return_wait(get_threadtask(child_thread), TCRW_CLEAR_ALL_WAIT);

		                                                          
		if ((child_task = (task_t)get_threadtask(child_thread)) != NULL) {
			task_deallocate(child_task);
		}
		thread_deallocate(child_thread);
	}

	return err;
}


  
            
  
                                                              
  
                                                            
                                        
                              
                                         
                                    
                                    
                                                
                                                   
                                         
                                          
                       
                                                        
                                           
  
                                               
                                
  
                                                                   
                                                              
                                                          
                            
  
                                                              
                                                                 
                                                            
                                                   
  
                                                                
                                                              
                                                             
                                                               
                                          
   
thread_t
cloneproc(task_t parent_task, coalition_t *parent_coalitions, proc_t parent_proc, int inherit_memory, int memstat_internal)
{
#if !CONFIG_MEMORYSTATUS
#pragma unused(memstat_internal)
#endif
	task_t child_task;
	proc_t child_proc;
	thread_t child_thread = NULL;

	if ((child_proc = forkproc(parent_proc)) == NULL) {
		                                    
		goto bad;
	}

	  
                                                                         
                                                                         
                                                                     
                                           
   
                                                                             
                                                                            
                                        
    
	const int parent_64bit_addr = parent_proc->p_flag & P_LP64;
	const int parent_64bit_data = (parent_task == TASK_NULL) ? parent_64bit_addr : task_get_64bit_data(parent_task);

	child_thread = fork_create_child(parent_task,
	    parent_coalitions,
	    child_proc,
	    inherit_memory,
	    parent_64bit_addr,
	    parent_64bit_data,
	    FALSE);

	if (child_thread == NULL) {
		  
                                                             
                                                 
     
		forkproc_free(child_proc);
		goto bad;
	}

	child_task = get_threadtask(child_thread);
	if (parent_64bit_addr) {
		OSBitOrAtomic(P_LP64, (UInt32 *)&child_proc->p_flag);
	} else {
		OSBitAndAtomic(~((uint32_t)P_LP64), (UInt32 *)&child_proc->p_flag);
	}

#if CONFIG_MEMORYSTATUS
	if (memstat_internal) {
		proc_list_lock();
		child_proc->p_memstat_state |= P_MEMSTAT_INTERNAL;
		proc_list_unlock();
	}
#endif

	                        
	pinsertchild(parent_proc, child_proc);

	  
                                        
    
	child_proc->p_stat = SRUN;
bad:
	return child_thread;
}


  
                                                                           
                                                                       
                                      
  
                                                         
  
                  
  
                                                                        
                         
  
                                                             
   
void
forkproc_free(proc_t p)
{
#if CONFIG_PERSONAS
	persona_proc_drop(p);
#endif                      

#if PSYNCH
	pth_proc_hashdelete(p);
#endif             

	                                                      
	proc_signalend(p, 0);
	proc_transend(p, 0);

	  
                                                                
                                                                
                    
    
	proc_limitdrop(p);

#if SYSV_SHM
	                                                                     
	if (p->vm_shm) {
		  
                                                            
    
                                         
     
		shmexec(p);
	}
#endif

	                                                      
	fdfree(p);

	  
                                                      
                                                                
                                                                     
    
	if (p->p_textvp) {
		vnode_rele(p->p_textvp);
		p->p_textvp = NULL;
	}

	                                         
	AUDIT_SESSION_PROCEXIT(p);

	lck_mtx_destroy(&p->p_mlock, &proc_mlock_grp);
	lck_mtx_destroy(&p->p_fdmlock, &proc_fdmlock_grp);
	lck_mtx_destroy(&p->p_ucred_mlock, &proc_ucred_mlock_grp);
#if CONFIG_DTRACE
	lck_mtx_destroy(&p->p_dtrace_sprlock, &proc_lck_grp);
#endif
	lck_spin_destroy(&p->p_slock, &proc_slock_grp);
	lck_rw_destroy(&p->p_dirs_lock, &proc_dirslock_grp);

	                                      
	kauth_cred_t tmp_ucred = p->p_ucred;
	kauth_cred_unref(&tmp_ucred);
	p->p_ucred = tmp_ucred;

	proc_list_lock();
	                                                    
	nprocs--;

	                                 
	LIST_REMOVE(p, p_hash);

	proc_list_unlock();

	thread_call_free(p->p_rcall);

	                           
	zfree(proc_sigacts_zone, p->p_sigacts);
	p->p_sigacts = NULL;
	zfree(proc_stats_zone, p->p_stats);
	p->p_stats = NULL;
	if (p->p_subsystem_root_path) {
		zfree(ZV_NAMEI, p->p_subsystem_root_path);
	}

	proc_checkdeadrefs(p);
	zfree(proc_zone, p);
}


  
           
  
                                                                      
              
  
                                              
  
                                             
                                           
  
                                                                
                                                               
                                                                
   
proc_t
forkproc(proc_t parent_proc)
{
	proc_t child_proc;                           
	static int nextpid = 0, pidwrap = 0;
	static uint64_t nextuniqueid = 0;
	int error = 0;
	struct session *sessp;
	uthread_t parent_uthread = (uthread_t)get_bsdthread_info(current_thread());
	rlim_t rlimit_cpu_cur;

	child_proc = zalloc_flags(proc_zone, Z_WAITOK | Z_ZERO);
	child_proc->p_stats = zalloc_flags(proc_stats_zone, Z_WAITOK | Z_ZERO);
	child_proc->p_sigacts = zalloc_flags(proc_sigacts_zone, Z_WAITOK);

	                                                   
	child_proc->p_rcall = thread_call_allocate((thread_call_func_t)realitexpire, child_proc);
	if (child_proc->p_rcall == NULL) {
		zfree(proc_sigacts_zone, child_proc->p_sigacts);
		zfree(proc_stats_zone, child_proc->p_stats);
		zfree(proc_zone, child_proc);
		child_proc = NULL;
		goto bad;
	}


	  
                       
    

	proc_list_lock();

	nextpid++;
retry:
	  
                                                   
                                                       
                                            
    
	if (nextpid >= PID_MAX) {
		nextpid = 100;
		pidwrap = 1;
	}
	if (pidwrap != 0) {
		                                                                 
		if (pfind_locked(nextpid) != PROC_NULL) {
			nextpid++;
			goto retry;
		}

		if (pgfind_internal(nextpid) != PGRP_NULL) {
			nextpid++;
			goto retry;
		}
		if (session_find_internal(nextpid) != SESSION_NULL) {
			nextpid++;
			goto retry;
		}
	}
	nprocs++;
	child_proc->p_pid = nextpid;
	child_proc->p_idversion = OSIncrementAtomic(&nextpidversion);
	                                                                      
	child_proc->p_uniqueid = ++nextuniqueid;
#if 1
	if (child_proc->p_pid != 0) {
		if (pfind_locked(child_proc->p_pid) != PROC_NULL) {
			panic("proc in the list already\n");
		}
	}
#endif
	                        
	child_proc->p_listflag |= (P_LIST_INHASH | P_LIST_INCREATE);
	LIST_INSERT_HEAD(PIDHASH(child_proc->p_pid), child_proc, p_hash);
	proc_list_unlock();

	if (child_proc->p_uniqueid == startup_serial_num_procs) {
		  
                                                             
                                             
     
		startup_serial_logging_active = false;
	}

	  
                                                                    
                      
    
	child_proc->p_stat = SIDL;
	child_proc->p_pgrpid = PGRPID_DEAD;

	  
                                                                   
                                                                 
                             
    
	__nochk_bcopy(&parent_proc->p_startcopy, &child_proc->p_startcopy,
	    (unsigned) ((caddr_t)&child_proc->p_endcopy - (caddr_t)&child_proc->p_startcopy));

#if defined(HAS_APPLE_PAC)
	  
                                                                               
                                     
    
	if (parent_proc->p_textvp) {
		child_proc->p_textvp = parent_proc->p_textvp;
	}

	if (parent_proc->p_pgrp) {
		child_proc->p_pgrp = parent_proc->p_pgrp;
	}
#endif                             

	child_proc->p_sessionid = parent_proc->p_sessionid;

	  
                                             
                                       
                                                
                                                            
    
#if CONFIG_DELAY_IDLE_SLEEP
	child_proc->p_flag = (parent_proc->p_flag & (P_LP64 | P_TRANSLATED | P_DISABLE_ASLR | P_DELAYIDLESLEEP | P_SUGID | P_AFFINITY));
#else                              
	child_proc->p_flag = (parent_proc->p_flag & (P_LP64 | P_TRANSLATED | P_DISABLE_ASLR | P_SUGID));
#endif                              

	child_proc->p_vfs_iopolicy = (parent_proc->p_vfs_iopolicy & (P_VFS_IOPOLICY_VALID_MASK));

	child_proc->p_responsible_pid = parent_proc->p_responsible_pid;

	  
                                                                 
                                                  
    
	child_proc->p_ucred = kauth_cred_get_with_ref();
	                         
	PROC_UPDATE_CREDS_ONPROC(child_proc);
	                                     
	AUDIT_SESSION_PROCNEW(child_proc);

	lck_mtx_init(&child_proc->p_mlock, &proc_mlock_grp, &proc_lck_attr);
	lck_mtx_init(&child_proc->p_fdmlock, &proc_fdmlock_grp, &proc_lck_attr);
	lck_mtx_init(&child_proc->p_ucred_mlock, &proc_ucred_mlock_grp, &proc_lck_attr);
#if CONFIG_DTRACE
	lck_mtx_init(&child_proc->p_dtrace_sprlock, &proc_lck_grp, &proc_lck_attr);
#endif
	lck_spin_init(&child_proc->p_slock, &proc_slock_grp, &proc_lck_attr);
	lck_rw_init(&child_proc->p_dirs_lock, &proc_dirslock_grp, &proc_lck_attr);

	klist_init(&child_proc->p_klist);

	if (child_proc->p_textvp != NULLVP) {
		                                       
		                                              
		if ((error = vnode_getwithref(child_proc->p_textvp)) == 0) {
			error = vnode_ref(child_proc->p_textvp);
			vnode_put(child_proc->p_textvp);
		}

		if (error != 0) {
			child_proc->p_textvp = NULLVP;
		}
	}

	  
                                                                 
                                                                   
                                                                
            
   
                                             
    
	child_proc->p_fd = fdcopy(parent_proc, parent_uthread->uu_cdir);

#if SYSV_SHM
	if (parent_proc->vm_shm) {
		                                         
		(void)shmfork(parent_proc, child_proc);
	}
#endif

	  
                                      
    
	proc_limitfork(parent_proc, child_proc);

	rlimit_cpu_cur = proc_limitgetcur(child_proc, RLIMIT_CPU, TRUE);
	if (rlimit_cpu_cur != RLIM_INFINITY) {
		child_proc->p_rlim_cpu.tv_sec = (rlimit_cpu_cur > __INT_MAX__) ? __INT_MAX__ : rlimit_cpu_cur;
	}

	                                                       
	                                                                 
	microtime_with_abstime(&child_proc->p_start, &child_proc->p_stats->ps_start);

	if (parent_proc->p_sigacts != NULL) {
		(void)memcpy(child_proc->p_sigacts,
		    parent_proc->p_sigacts, sizeof *child_proc->p_sigacts);
	} else {
		(void)memset(child_proc->p_sigacts, 0, sizeof *child_proc->p_sigacts);
	}

	sessp = proc_session(parent_proc);
	if (sessp->s_ttyvp != NULL && parent_proc->p_flag & P_CONTROLT) {
		OSBitOrAtomic(P_CONTROLT, &child_proc->p_flag);
	}
	session_rele(sessp);

	  
                                           
                                                             
                                                                  
    
	proc_signalstart(child_proc, 0);
	proc_transstart(child_proc, 0, 0);

	child_proc->p_pcaction = 0;

	TAILQ_INIT(&child_proc->p_uthlist);
	TAILQ_INIT(&child_proc->p_aio_activeq);
	TAILQ_INIT(&child_proc->p_aio_doneq);

	                                            
	child_proc->p_csflags = (parent_proc->p_csflags & ~CS_KILLED);

	  
                               
   
                                                                   
                                                             
                                                            
                                                             
                  
   
                                                                     
    
	child_proc->p_wqthread = parent_proc->p_wqthread;
	child_proc->p_threadstart = parent_proc->p_threadstart;
	child_proc->p_pthsize = parent_proc->p_pthsize;
	if ((parent_proc->p_lflag & P_LREGISTER) != 0) {
		child_proc->p_lflag |= P_LREGISTER;
	}
	child_proc->p_dispatchqueue_offset = parent_proc->p_dispatchqueue_offset;
	child_proc->p_dispatchqueue_serialno_offset = parent_proc->p_dispatchqueue_serialno_offset;
	child_proc->p_dispatchqueue_label_offset = parent_proc->p_dispatchqueue_label_offset;
	child_proc->p_return_to_kernel_offset = parent_proc->p_return_to_kernel_offset;
	child_proc->p_mach_thread_self_offset = parent_proc->p_mach_thread_self_offset;
	child_proc->p_pth_tsd_offset = parent_proc->p_pth_tsd_offset;
#if PSYNCH
	pth_proc_hashinit(child_proc);
#endif             

#if CONFIG_PERSONAS
	child_proc->p_persona = NULL;
	error = persona_proc_inherit(child_proc, parent_proc);
	if (error != 0) {
		printf("forkproc: persona_proc_inherit failed (persona %d being destroyed?)\n", persona_get_uid(parent_proc->p_persona));
		forkproc_free(child_proc);
		child_proc = NULL;
		goto bad;
	}
#endif

#if CONFIG_MEMORYSTATUS
	                       
	child_proc->p_memstat_state = 0;
	child_proc->p_memstat_effectivepriority = JETSAM_PRIORITY_DEFAULT;
	child_proc->p_memstat_requestedpriority = JETSAM_PRIORITY_DEFAULT;
	child_proc->p_memstat_assertionpriority = 0;
	child_proc->p_memstat_userdata          = 0;
	child_proc->p_memstat_idle_start        = 0;
	child_proc->p_memstat_idle_delta        = 0;
	child_proc->p_memstat_memlimit          = 0;
	child_proc->p_memstat_memlimit_active   = 0;
	child_proc->p_memstat_memlimit_inactive = 0;
	child_proc->p_memstat_relaunch_flags    = P_MEMSTAT_RELAUNCH_UNKNOWN;
#if CONFIG_FREEZE
	child_proc->p_memstat_freeze_sharedanon_pages = 0;
#endif
	child_proc->p_memstat_dirty = 0;
	child_proc->p_memstat_idledeadline = 0;
#endif                          

	if (parent_proc->p_subsystem_root_path) {
		size_t parent_length = strlen(parent_proc->p_subsystem_root_path) + 1;
		assert(parent_length <= MAXPATHLEN);
		child_proc->p_subsystem_root_path = zalloc_flags(ZV_NAMEI,
		    Z_WAITOK | Z_ZERO);
		memcpy(child_proc->p_subsystem_root_path, parent_proc->p_subsystem_root_path, parent_length);
	}

bad:
	return child_proc;
}

void
proc_lock(proc_t p)
{
	LCK_MTX_ASSERT(&proc_list_mlock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(&p->p_mlock);
}

void
proc_unlock(proc_t p)
{
	lck_mtx_unlock(&p->p_mlock);
}

void
proc_spinlock(proc_t p)
{
	lck_spin_lock_grp(&p->p_slock, &proc_slock_grp);
}

void
proc_spinunlock(proc_t p)
{
	lck_spin_unlock(&p->p_slock);
}

void
proc_list_lock(void)
{
	lck_mtx_lock(&proc_list_mlock);
}

void
proc_list_unlock(void)
{
	lck_mtx_unlock(&proc_list_mlock);
}

void
proc_ucred_lock(proc_t p)
{
	lck_mtx_lock(&p->p_ucred_mlock);
}

void
proc_ucred_unlock(proc_t p)
{
	lck_mtx_unlock(&p->p_ucred_mlock);
}

void *
uthread_alloc(task_t task, thread_t thread, int noinherit)
{
	proc_t p;
	uthread_t uth;
	uthread_t uth_parent;
	void *ut;

	ut = zalloc_flags(uthread_zone, Z_WAITOK | Z_ZERO);

	p = (proc_t) get_bsdtask_info(task);
	uth = (uthread_t)ut;
	uth->uu_thread = thread;

	lck_spin_init(&uth->uu_rethrottle_lock, &rethrottle_lock_grp,
	    LCK_ATTR_NULL);

	  
                                                                
                         
   
                                                               
                                                                  
                                                                  
    
	uth_parent = (uthread_t)get_bsdthread_info(current_thread());
	if ((noinherit == 0) && task == current_task() &&
	    uth_parent != NULL &&
	    IS_VALID_CRED(uth_parent->uu_ucred)) {
		  
                                                               
                                                              
               
     
		kauth_cred_ref(uth_parent->uu_ucred);
		uth->uu_ucred = uth_parent->uu_ucred;
		                                                               
		if (uth_parent->uu_flag & UT_SETUID) {
			uth->uu_flag |= UT_SETUID;
		}
	} else {
		                                                              
		if ((task != kernel_task) && (p != PROC_NULL)) {
			uth->uu_ucred = kauth_cred_proc_ref(p);
		} else {
			uth->uu_ucred = NOCRED;
		}
	}


	if ((task != kernel_task) && p) {
		proc_lock(p);
		if (noinherit != 0) {
			                                          
			uth->uu_sigmask = ~workq_threadmask;
		} else if (uth_parent) {
			if (uth_parent->uu_flag & UT_SAS_OLDMASK) {
				uth->uu_sigmask = uth_parent->uu_oldmask;
			} else {
				uth->uu_sigmask = uth_parent->uu_sigmask;
			}
		}
		uth->uu_context.vc_thread = thread;
		  
                                                               
                                          
     
		if (!task_is_exec_copy(task)) {
			TAILQ_INSERT_TAIL(&p->p_uthlist, uth, uu_list);
		}
		proc_unlock(p);

#if CONFIG_DTRACE
		if (p->p_dtrace_ptss_pages != NULL && !task_is_exec_copy(task)) {
			uth->t_dtrace_scratch = dtrace_ptss_claim_entry(p);
		}
#endif
	}

	return ut;
}

  
                                                                                    
                                                                                            
   
void
uthread_cleanup_name(void *uthread)
{
	uthread_t uth = (uthread_t)uthread;

	  
                     
                                               
                                                
                                                   
                                                   
                                                    
            
    
	if (uth->pth_name != NULL) {
		void *pth_name = uth->pth_name;
		uth->pth_name = NULL;
		kfree(pth_name, MAXTHREADNAMESIZE);
	}
	return;
}

  
                                                                           
                                                 
   
void
uthread_cleanup(task_t task, void *uthread, void * bsd_info)
{
	uthread_t uth = (uthread_t)uthread;
	proc_t p = (proc_t)bsd_info;

#if PROC_REF_DEBUG
	if (__improbable(uthread_get_proc_refcount(uthread) != 0)) {
		panic("uthread_cleanup called for uthread %p with uu_proc_refcount != 0", uthread);
	}
#endif

	if (uth->uu_lowpri_window || uth->uu_throttle_info) {
		  
                                              
                                                          
                                                        
                                                         
                                                                     
    
                                                                   
                               
     
		throttle_lowpri_io(0);
	}
	  
                                                          
                                                           
                                                     
                              
    
	assert(uth->uu_ar == NULL);

	if (uth->uu_kqr_bound) {
		kqueue_threadreq_unbind(p, uth->uu_kqr_bound);
	}

	if (uth->uu_select.nbytes) {
		select_cleanup_uthread(&uth->uu_select);
	}

	if (uth->uu_cdir) {
		vnode_rele(uth->uu_cdir);
		uth->uu_cdir = NULLVP;
	}

	if (uth->uu_wqset) {
		if (waitq_set_is_valid(uth->uu_wqset)) {
			waitq_set_deinit(uth->uu_wqset);
		}
		kheap_free(KHEAP_DEFAULT, uth->uu_wqset, uth->uu_wqstate_sz);
		uth->uu_wqset = NULL;
		uth->uu_wqstate_sz = 0;
	}

	os_reason_free(uth->uu_exit_reason);

	if ((task != kernel_task) && p) {
		if (((uth->uu_flag & UT_VFORK) == UT_VFORK) && (uth->uu_proc != PROC_NULL)) {
			vfork_exit_internal(uth->uu_proc, 0, 1);
		}
		  
                                                
                                                           
                                                         
                                                          
                                           
     
		if (get_bsdtask_info(task) == p && !task_is_exec_copy(task)) {
			proc_lock(p);

			TAILQ_REMOVE(&p->p_uthlist, uth, uu_list);
			p->p_siglist |= (uth->uu_siglist & execmask & (~p->p_sigignore | sigcantmask));
			proc_unlock(p);
		}
#if CONFIG_DTRACE
		struct dtrace_ptss_page_entry *tmpptr = uth->t_dtrace_scratch;
		uth->t_dtrace_scratch = NULL;
		if (tmpptr != NULL && !task_is_exec_copy(task)) {
			dtrace_ptss_release_entry(p, tmpptr);
		}
#endif
	}
}

                                                            
void
uthread_cred_free(void *uthread)
{
	uthread_t uth = (uthread_t)uthread;

	                                 
	if (IS_VALID_CRED(uth->uu_ucred)) {
		kauth_cred_t oldcred = uth->uu_ucred;
		uth->uu_ucred = NOCRED;
		kauth_cred_unref(&oldcred);
	}
}

                                                                       
void
uthread_zone_free(void *uthread)
{
	uthread_t uth = (uthread_t)uthread;

	if (uth->t_tombstone) {
		kfree(uth->t_tombstone, sizeof(struct doc_tombstone));
		uth->t_tombstone = NULL;
	}

	lck_spin_destroy(&uth->uu_rethrottle_lock, &rethrottle_lock_grp);

	uthread_cleanup_name(uthread);
	                                 
	zfree(uthread_zone, uthread);
}
