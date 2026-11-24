/*
 * Copyright (c) 2003-2020 Apple Inc. All rights reserved.
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


  
        
                                                                  
                                                                         
                                       
                                                                    
                                                                      
                                
   


  
                                                                     
   

#include <sys/systm.h>
#include <sys/fcntl.h>
#include <sys/file_internal.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/vnode_internal.h>
#include <sys/malloc.h>
#include <sys/mount_internal.h>
#include <sys/param.h>
#include <sys/proc_internal.h>
#include <sys/sysctl.h>
#include <sys/unistd.h>
#include <sys/user.h>

#include <sys/aio_kern.h>
#include <sys/sysproto.h>

#include <machine/limits.h>

#include <mach/mach_types.h>
#include <kern/kern_types.h>
#include <kern/waitq.h>
#include <kern/zalloc.h>
#include <kern/task.h>
#include <kern/sched_prim.h>

#include <vm/vm_map.h>

#include <os/refcnt.h>

#include <sys/kdebug.h>
#define AIO_work_queued                 1
#define AIO_worker_wake                 2
#define AIO_completion_sig              3
#define AIO_completion_cleanup_wait     4
#define AIO_completion_cleanup_wake     5
#define AIO_completion_suspend_wake     6
#define AIO_fsync_delay                 7
#define AIO_cancel                      10
#define AIO_cancel_async_workq          11
#define AIO_cancel_sync_workq           12
#define AIO_cancel_activeq              13
#define AIO_cancel_doneq                14
#define AIO_fsync                       20
#define AIO_read                        30
#define AIO_write                       40
#define AIO_listio                      50
#define AIO_error                       60
#define AIO_error_val                   61
#define AIO_error_activeq               62
#define AIO_error_workq                 63
#define AIO_return                      70
#define AIO_return_val                  71
#define AIO_return_activeq              72
#define AIO_return_workq                73
#define AIO_exec                        80
#define AIO_exit                        90
#define AIO_exit_sleep                  91
#define AIO_close                       100
#define AIO_close_sleep                 101
#define AIO_suspend                     110
#define AIO_suspend_sleep               111
#define AIO_worker_thread               120

__options_decl(aio_entry_flags_t, uint32_t, {
	AIO_READ        = 0x00000001,             
	AIO_WRITE       = 0x00000002,              
	AIO_FSYNC       = 0x00000004,                                 
	AIO_DSYNC       = 0x00000008,                                                      
	AIO_LIO         = 0x00000010,                              
	AIO_LIO_WAIT    = 0x00000020,                                          

	  
                                                        
                            
                                  
   
                                                                     
                                                                     
    
	AIO_CLOSE_WAIT  = 0x00004000,
	AIO_EXIT_WAIT   = 0x00008000,
});

                           
  
              
                                           
  
                                     
  
                                                                           
                                                        
  
                                                                           
                                                       
  
                                                                      
                                                         
  
                                                   
                                                                   
  
                                                                         
                                                                    
                                                                             
                                           
  
                                                                    
  
                                                                     
                                                                        
                                                    
   
struct aio_workq_entry {
	                
	TAILQ_ENTRY(aio_workq_entry)    aio_workq_link;

	               
	TAILQ_ENTRY(aio_workq_entry)    aio_proc_link;                                    
	user_ssize_t                    returnval;                                                  
	errno_t                         errorval;                                                  
	os_refcnt_t                     aio_refcount;
	aio_entry_flags_t               flags;

	int                             lio_pending;                                                   
	struct aio_workq_entry         *lio_leader;                                                 

	                                                   
	struct proc                    *procp;                                                  
	user_addr_t                     uaiocbp;                                              
	struct user_aiocb               aiocb;                                            
	thread_t                        thread;                                              

	                                                                                  
	vm_map_t                        aio_map;                                                  
};

  
                                                                      
                                                                           
                                                                  
                                                                           
                                                                         
                                                                             
                                    
   
typedef struct aio_workq   {
	TAILQ_HEAD(, aio_workq_entry)   aioq_entries;
	lck_spin_t                      aioq_lock;
	struct waitq                    aioq_waitq;
} *aio_workq_t;

#define AIO_NUM_WORK_QUEUES 1
struct aio_anchor_cb {
	os_atomic(int)          aio_total_count;                                  

	                               
	int                     aio_num_workqs;
	struct aio_workq        aio_async_workqs[AIO_NUM_WORK_QUEUES];
};
typedef struct aio_anchor_cb aio_anchor_cb;

  
                                      
                                                                              
                                                                                  
                                                                                    
   
#define AIO_SUSPEND_SLEEP_CHAN  p_aio_activeq
#define AIO_CLEANUP_SLEEP_CHAN  p_aio_total_count

#define ASSERT_AIO_FROM_PROC(aiop, theproc)     \
	if ((aiop)->procp != (theproc)) {       \
	        panic("AIO on a proc list that does not belong to that proc.\n"); \
	}

  
                    
   
static void             aio_proc_lock(proc_t procp);
static void             aio_proc_lock_spin(proc_t procp);
static void             aio_proc_unlock(proc_t procp);
static lck_mtx_t       *aio_proc_mutex(proc_t procp);
static bool             aio_has_active_requests_for_process(proc_t procp);
static bool             aio_proc_has_active_requests_for_file(proc_t procp, int fd);
static boolean_t        is_already_queued(proc_t procp, user_addr_t aiocbp);

static aio_workq_t      aio_entry_workq(aio_workq_entry *entryp);
static void             aio_workq_remove_entry_locked(aio_workq_t queue, aio_workq_entry *entryp);
static void             aio_workq_add_entry_locked(aio_workq_t queue, aio_workq_entry *entryp);
static void             aio_entry_ref(aio_workq_entry *entryp);
static void             aio_entry_unref(aio_workq_entry *entryp);
static bool             aio_entry_try_workq_remove(aio_workq_entry *entryp);
static boolean_t        aio_delay_fsync_request(aio_workq_entry *entryp);
static void             aio_free_request(aio_workq_entry *entryp);

static void             aio_workq_init(aio_workq_t wq);
static void             aio_workq_lock_spin(aio_workq_t wq);
static void             aio_workq_unlock(aio_workq_t wq);
static lck_spin_t      *aio_workq_lock(aio_workq_t wq);

static void             aio_work_thread(void *arg, wait_result_t wr);
static aio_workq_entry *aio_get_some_work(void);

static int              aio_queue_async_request(proc_t procp, user_addr_t aiocbp, aio_entry_flags_t);
static int              aio_validate(proc_t, aio_workq_entry *entryp);

static int              do_aio_cancel_locked(proc_t p, int fd, user_addr_t aiocbp, aio_entry_flags_t);
static void             do_aio_completion_and_unlock(proc_t p, aio_workq_entry *entryp);
static int              do_aio_fsync(aio_workq_entry *entryp);
static int              do_aio_read(aio_workq_entry *entryp);
static int              do_aio_write(aio_workq_entry *entryp);
static void             do_munge_aiocb_user32_to_user(struct user32_aiocb *my_aiocbp, struct user_aiocb *the_user_aiocbp);
static void             do_munge_aiocb_user64_to_user(struct user64_aiocb *my_aiocbp, struct user_aiocb *the_user_aiocbp);
static aio_workq_entry *aio_create_queue_entry(proc_t procp, user_addr_t aiocbp, aio_entry_flags_t);
static int              aio_copy_in_list(proc_t, user_addr_t, user_addr_t *, int);

#define ASSERT_AIO_PROC_LOCK_OWNED(p)   LCK_MTX_ASSERT(aio_proc_mutex(p), LCK_MTX_ASSERT_OWNED)
#define ASSERT_AIO_WORKQ_LOCK_OWNED(q)  LCK_SPIN_ASSERT(aio_workq_lock(q), LCK_ASSERT_OWNED)

  
                       
   

                                  
extern int dofileread(vfs_context_t ctx, struct fileproc *fp,
    user_addr_t bufp, user_size_t nbyte,
    off_t offset, int flags, user_ssize_t *retval);
extern int dofilewrite(vfs_context_t ctx, struct fileproc *fp,
    user_addr_t bufp, user_size_t nbyte, off_t offset,
    int flags, user_ssize_t *retval);

  
                                 
   
extern int aio_max_requests;                                                
extern int aio_max_requests_per_process;                                            
extern int aio_worker_threads;                                                       


  
                        
   
static aio_anchor_cb aio_anchor = {
	.aio_num_workqs = AIO_NUM_WORK_QUEUES,
};
os_refgrp_decl(static, aio_refgrp, "aio", NULL);
static LCK_GRP_DECLARE(aio_proc_lock_grp, "aio_proc");
static LCK_GRP_DECLARE(aio_queue_lock_grp, "aio_queue");
static LCK_MTX_DECLARE(aio_proc_mtx, &aio_proc_lock_grp);

static ZONE_DECLARE(aio_workq_zonep, "aiowq", sizeof(aio_workq_entry),
    ZC_ZFREE_CLEARMEM);

          
static aio_workq_t
aio_entry_workq(__unused aio_workq_entry *entryp)
{
	return &aio_anchor.aio_async_workqs[0];
}

static void
aio_workq_init(aio_workq_t wq)
{
	TAILQ_INIT(&wq->aioq_entries);
	lck_spin_init(&wq->aioq_lock, &aio_queue_lock_grp, LCK_ATTR_NULL);
	waitq_init(&wq->aioq_waitq, SYNC_POLICY_FIFO);
}


  
                                              
   
static void
aio_workq_remove_entry_locked(aio_workq_t queue, aio_workq_entry *entryp)
{
	ASSERT_AIO_WORKQ_LOCK_OWNED(queue);

	if (entryp->aio_workq_link.tqe_prev == NULL) {
		panic("Trying to remove an entry from a work queue, but it is not on a queue\n");
	}

	TAILQ_REMOVE(&queue->aioq_entries, entryp, aio_workq_link);
	entryp->aio_workq_link.tqe_prev = NULL;                     
}

static void
aio_workq_add_entry_locked(aio_workq_t queue, aio_workq_entry *entryp)
{
	ASSERT_AIO_WORKQ_LOCK_OWNED(queue);

	TAILQ_INSERT_TAIL(&queue->aioq_entries, entryp, aio_workq_link);
}

static void
aio_proc_lock(proc_t procp)
{
	lck_mtx_lock(aio_proc_mutex(procp));
}

static void
aio_proc_lock_spin(proc_t procp)
{
	lck_mtx_lock_spin(aio_proc_mutex(procp));
}

static bool
aio_has_any_work(void)
{
	return os_atomic_load(&aio_anchor.aio_total_count, relaxed) != 0;
}

static bool
aio_try_proc_insert_active_locked(proc_t procp, aio_workq_entry *entryp)
{
	int old, new;

	ASSERT_AIO_PROC_LOCK_OWNED(procp);

	if (procp->p_aio_total_count >= aio_max_requests_per_process) {
		return false;
	}

	if (is_already_queued(procp, entryp->uaiocbp)) {
		return false;
	}

	os_atomic_rmw_loop(&aio_anchor.aio_total_count, old, new, relaxed, {
		if (old >= aio_max_requests) {
		        os_atomic_rmw_loop_give_up(return false);
		}
		new = old + 1;
	});

	TAILQ_INSERT_TAIL(&procp->p_aio_activeq, entryp, aio_proc_link);
	procp->p_aio_total_count++;
	return true;
}

static void
aio_proc_move_done_locked(proc_t procp, aio_workq_entry *entryp)
{
	TAILQ_REMOVE(&procp->p_aio_activeq, entryp, aio_proc_link);
	TAILQ_INSERT_TAIL(&procp->p_aio_doneq, entryp, aio_proc_link);
}

static void
aio_proc_remove_done_locked(proc_t procp, aio_workq_entry *entryp)
{
	TAILQ_REMOVE(&procp->p_aio_doneq, entryp, aio_proc_link);
	entryp->aio_proc_link.tqe_prev = NULL;
	if (os_atomic_dec_orig(&aio_anchor.aio_total_count, relaxed) <= 0) {
		panic("Negative total AIO count!\n");
	}
	if (procp->p_aio_total_count-- <= 0) {
		panic("proc %p: p_aio_total_count accounting mismatch", procp);
	}
}

static void
aio_proc_unlock(proc_t procp)
{
	lck_mtx_unlock(aio_proc_mutex(procp));
}

static lck_mtx_t*
aio_proc_mutex(proc_t procp)
{
	return &procp->p_mlock;
}

static void
aio_entry_ref(aio_workq_entry *entryp)
{
	os_ref_retain(&entryp->aio_refcount);
}

static void
aio_entry_unref(aio_workq_entry *entryp)
{
	if (os_ref_release(&entryp->aio_refcount) == 0) {
		aio_free_request(entryp);
	}
}

static bool
aio_entry_try_workq_remove(aio_workq_entry *entryp)
{
	                                                         
	if (entryp->aio_workq_link.tqe_prev != NULL) {
		aio_workq_t queue;

		                                             
		queue = aio_entry_workq(entryp);
		aio_workq_lock_spin(queue);
		if (entryp->aio_workq_link.tqe_prev != NULL) {
			aio_workq_remove_entry_locked(queue, entryp);
			aio_workq_unlock(queue);
			return true;
		} else {
			aio_workq_unlock(queue);
		}
	}

	return false;
}

static void
aio_workq_lock_spin(aio_workq_t wq)
{
	lck_spin_lock(aio_workq_lock(wq));
}

static void
aio_workq_unlock(aio_workq_t wq)
{
	lck_spin_unlock(aio_workq_lock(wq));
}

static lck_spin_t*
aio_workq_lock(aio_workq_t wq)
{
	return &wq->aioq_lock;
}

  
                                                                         
                                                                      
                                                                             
                                                                   
                                          
   
int
aio_cancel(proc_t p, struct aio_cancel_args *uap, int *retval)
{
	struct user_aiocb my_aiocb;
	int               result;

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_cancel) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, 0, 0, 0);

	                                                                     
	if (!aio_has_any_work()) {
		result = 0;
		*retval = AIO_ALLDONE;
		goto ExitRoutine;
	}

	*retval = -1;
	if (uap->aiocbp != USER_ADDR_NULL) {
		if (proc_is64bit(p)) {
			struct user64_aiocb aiocb64;

			result = copyin(uap->aiocbp, &aiocb64, sizeof(aiocb64));
			if (result == 0) {
				do_munge_aiocb_user64_to_user(&aiocb64, &my_aiocb);
			}
		} else {
			struct user32_aiocb aiocb32;

			result = copyin(uap->aiocbp, &aiocb32, sizeof(aiocb32));
			if (result == 0) {
				do_munge_aiocb_user32_to_user(&aiocb32, &my_aiocb);
			}
		}

		if (result != 0) {
			result = EAGAIN;
			goto ExitRoutine;
		}

		                                                            
		                                                              
		                                                               
		                      
		if (uap->fd != my_aiocb.aio_fildes) {
			result = EBADF;
			goto ExitRoutine;
		}
	}

	aio_proc_lock(p);
	result = do_aio_cancel_locked(p, uap->fd, uap->aiocbp, 0);
	ASSERT_AIO_PROC_LOCK_OWNED(p);
	aio_proc_unlock(p);

	if (result != -1) {
		*retval = result;
		result = 0;
		goto ExitRoutine;
	}

	result = EBADF;

ExitRoutine:
	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_cancel) | DBG_FUNC_END,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, result, 0, 0);

	return result;
}


  
                                                                        
                                     
                  
   
__private_extern__ void
_aio_close(proc_t p, int fd)
{
	int error;

	                                                                     
	if (!aio_has_any_work()) {
		return;
	}

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_close) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(p), fd, 0, 0, 0);

	                                                                              
	aio_proc_lock(p);
	error = do_aio_cancel_locked(p, fd, USER_ADDR_NULL, AIO_CLOSE_WAIT);
	ASSERT_AIO_PROC_LOCK_OWNED(p);
	if (error == AIO_NOTCANCELED) {
		  
                                                                             
                                                                              
                                                                              
                                                                         
                                                                             
                                                   
     

		KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_close_sleep) | DBG_FUNC_NONE,
		    VM_KERNEL_ADDRPERM(p), fd, 0, 0, 0);

		while (aio_proc_has_active_requests_for_file(p, fd)) {
			msleep(&p->AIO_CLEANUP_SLEEP_CHAN, aio_proc_mutex(p), PRIBIO, "aio_close", 0);
		}
	}

	aio_proc_unlock(p);

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_close) | DBG_FUNC_END,
	    VM_KERNEL_ADDRPERM(p), fd, 0, 0, 0);
}


  
                                                                   
                                                                     
                                                                        
                       
   
int
aio_error(proc_t p, struct aio_error_args *uap, int *retval)
{
	aio_workq_entry *entryp;
	int              error;

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_error) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, 0, 0, 0);

	                                        
	if (!aio_has_any_work()) {
		return EINVAL;
	}

	aio_proc_lock(p);

	                                                                            
	TAILQ_FOREACH(entryp, &p->p_aio_doneq, aio_proc_link) {
		if (entryp->uaiocbp == uap->aiocbp) {
			ASSERT_AIO_FROM_PROC(entryp, p);

			*retval = entryp->errorval;
			error = 0;

			KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_error_val) | DBG_FUNC_NONE,
			    VM_KERNEL_ADDRPERM(p), uap->aiocbp, *retval, 0, 0);
			goto ExitRoutine;
		}
	}

	                                                               
	TAILQ_FOREACH(entryp, &p->p_aio_activeq, aio_proc_link) {
		if (entryp->uaiocbp == uap->aiocbp) {
			ASSERT_AIO_FROM_PROC(entryp, p);
			*retval = EINPROGRESS;
			error = 0;
			KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_error_activeq) | DBG_FUNC_NONE,
			    VM_KERNEL_ADDRPERM(p), uap->aiocbp, *retval, 0, 0);
			goto ExitRoutine;
		}
	}

	error = EINVAL;

ExitRoutine:
	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_error) | DBG_FUNC_END,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, error, 0, 0);
	aio_proc_unlock(p);

	return error;
}


  
                                                                
                                                                               
                                                                       
                                                                                
                    
   
int
aio_fsync(proc_t p, struct aio_fsync_args *uap, int *retval)
{
	aio_entry_flags_t fsync_kind;
	int error;

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_fsync) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, uap->op, 0, 0);

	*retval = 0;
	                                                                
	if (uap->op == O_SYNC || uap->op == 0) {
		fsync_kind = AIO_FSYNC;
	} else if (uap->op == O_DSYNC) {
		fsync_kind = AIO_DSYNC;
	} else {
		*retval = -1;
		error = EINVAL;
		goto ExitRoutine;
	}

	error = aio_queue_async_request(p, uap->aiocbp, fsync_kind);
	if (error != 0) {
		*retval = -1;
	}

ExitRoutine:
	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_fsync) | DBG_FUNC_END,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, error, 0, 0);

	return error;
}


                                                                        
                                                            
                          
   
int
aio_read(proc_t p, struct aio_read_args *uap, int *retval)
{
	int error;

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_read) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, 0, 0, 0);

	*retval = 0;

	error = aio_queue_async_request(p, uap->aiocbp, AIO_READ);
	if (error != 0) {
		*retval = -1;
	}

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_read) | DBG_FUNC_END,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, error, 0, 0);

	return error;
}


  
                                                                     
                                                                      
                                                                   
                                                                  
                                                                  
   
int
aio_return(proc_t p, struct aio_return_args *uap, user_ssize_t *retval)
{
	aio_workq_entry *entryp;
	int              error = EINVAL;

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_return) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, 0, 0, 0);

	                                           
	if (!aio_has_any_work()) {
		goto ExitRoutine;
	}

	aio_proc_lock(p);
	*retval = 0;

	                                                                            
	TAILQ_FOREACH(entryp, &p->p_aio_doneq, aio_proc_link) {
		ASSERT_AIO_FROM_PROC(entryp, p);
		if (entryp->uaiocbp == uap->aiocbp) {
			                                                           
			aio_proc_remove_done_locked(p, entryp);

			*retval = entryp->returnval;
			error = 0;
			aio_proc_unlock(p);

			aio_entry_unref(entryp);

			KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_return_val) | DBG_FUNC_NONE,
			    VM_KERNEL_ADDRPERM(p), uap->aiocbp, *retval, 0, 0);
			goto ExitRoutine;
		}
	}

	                                                               
	TAILQ_FOREACH(entryp, &p->p_aio_activeq, aio_proc_link) {
		ASSERT_AIO_FROM_PROC(entryp, p);
		if (entryp->uaiocbp == uap->aiocbp) {
			error = EINPROGRESS;
			KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_return_activeq) | DBG_FUNC_NONE,
			    VM_KERNEL_ADDRPERM(p), uap->aiocbp, *retval, 0, 0);
			break;
		}
	}

	aio_proc_unlock(p);

ExitRoutine:
	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_return) | DBG_FUNC_END,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, error, 0, 0);

	return error;
}


  
                                                                       
                                                                       
                                                                       
                                                      
                          
   
__private_extern__ void
_aio_exec(proc_t p)
{
	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_exec) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(p), 0, 0, 0, 0);

	_aio_exit(p);

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_exec) | DBG_FUNC_END,
	    VM_KERNEL_ADDRPERM(p), 0, 0, 0, 0);
}


  
                                                                       
                                                                                 
                                                                       
                                                                               
   
__private_extern__ void
_aio_exit(proc_t p)
{
	TAILQ_HEAD(, aio_workq_entry) tofree = TAILQ_HEAD_INITIALIZER(tofree);
	aio_workq_entry *entryp, *tmp;
	int              error;

	                                                                     
	if (!aio_has_any_work()) {
		return;
	}

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_exit) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(p), 0, 0, 0, 0);

	aio_proc_lock(p);

	  
                                                                      
                               
    
	error = do_aio_cancel_locked(p, -1, USER_ADDR_NULL, AIO_EXIT_WAIT);
	ASSERT_AIO_PROC_LOCK_OWNED(p);
	if (error == AIO_NOTCANCELED) {
		  
                                                                             
                                                                             
                                                                           
                                                                               
                                                                                 
                         
     

		KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_exit_sleep) | DBG_FUNC_NONE,
		    VM_KERNEL_ADDRPERM(p), 0, 0, 0, 0);

		while (aio_has_active_requests_for_process(p)) {
			msleep(&p->AIO_CLEANUP_SLEEP_CHAN, aio_proc_mutex(p), PRIBIO, "aio_exit", 0);
		}
	}

	assert(!aio_has_active_requests_for_process(p));

	                                                    
	TAILQ_FOREACH_SAFE(entryp, &p->p_aio_doneq, aio_proc_link, tmp) {
		ASSERT_AIO_FROM_PROC(entryp, p);

		aio_proc_remove_done_locked(p, entryp);
		TAILQ_INSERT_TAIL(&tofree, entryp, aio_proc_link);
	}

	aio_proc_unlock(p);

	                                                         
	TAILQ_FOREACH_SAFE(entryp, &tofree, aio_proc_link, tmp) {
		entryp->aio_proc_link.tqe_prev = NULL;
		aio_entry_unref(entryp);
	}

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_exit) | DBG_FUNC_END,
	    VM_KERNEL_ADDRPERM(p), 0, 0, 0, 0);
}


static bool
should_cancel(aio_workq_entry *entryp, int fd, user_addr_t aiocbp,
    aio_entry_flags_t reason)
{
	if (reason & AIO_EXIT_WAIT) {
		                           
		return true;
	}
	if (fd != entryp->aiocb.aio_fildes) {
		                                    
		return false;
	}
	  
                                       
                                                 
    
	return aiocbp == USER_ADDR_NULL || entryp->uaiocbp == aiocbp;
}

  
                                                                                   
                                  
                                                                              
                                                                              
                                                                             
          
                                                                          
                                                                       
                                                                            
                         
                                                                          
                                                                        
  
                                                     
   
static int
do_aio_cancel_locked(proc_t p, int fd, user_addr_t aiocbp,
    aio_entry_flags_t reason)
{
	bool multiple_matches = (aiocbp == USER_ADDR_NULL);
	aio_workq_entry *entryp, *tmp;
	int result;

	ASSERT_AIO_PROC_LOCK_OWNED(p);

	                                                       
again:
	result = -1;
	TAILQ_FOREACH_SAFE(entryp, &p->p_aio_activeq, aio_proc_link, tmp) {
		ASSERT_AIO_FROM_PROC(entryp, p);

		if (!should_cancel(entryp, fd, aiocbp, reason)) {
			continue;
		}

		if (reason) {
			                                                   
			entryp->flags |= reason;
			if ((entryp->flags & AIO_EXIT_WAIT) && (entryp->flags & AIO_CLOSE_WAIT)) {
				panic("Close and exit flags set at the same time\n");
			}
		}

		                                                         
		if (aio_entry_try_workq_remove(entryp)) {
			entryp->errorval = ECANCELED;
			entryp->returnval = -1;

			                                                       
			KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_cancel_async_workq) | DBG_FUNC_NONE,
			    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
			    fd, 0, 0);
			do_aio_completion_and_unlock(p, entryp);

			aio_proc_lock(p);

			if (multiple_matches) {
				  
                                                              
                                                                
                  
      
                                                                 
                                                                 
                                                                  
                                
       
				goto again;
			}

			return AIO_CANCELED;
		}

		  
                                                                     
                                           
     
		KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_cancel_activeq) | DBG_FUNC_NONE,
		    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
		    fd, 0, 0);

		result = AIO_NOTCANCELED;
		if (!multiple_matches) {
			return result;
		}
	}

	  
                                                                              
                                                                            
                              
   
                                
    
	if (result == -1) {
		TAILQ_FOREACH(entryp, &p->p_aio_doneq, aio_proc_link) {
			ASSERT_AIO_FROM_PROC(entryp, p);
			if (should_cancel(entryp, fd, aiocbp, reason)) {
				KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_cancel_doneq) | DBG_FUNC_NONE,
				    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
				    fd, 0, 0);

				result = AIO_ALLDONE;
				if (!multiple_matches) {
					return result;
				}
			}
		}
	}

	return result;
}


  
                                                                           
                                                                           
                                                                         
          
                                                                         
                                                                         
              
   
int
aio_suspend(proc_t p, struct aio_suspend_args *uap, int *retval)
{
	__pthread_testcancel(1);
	return aio_suspend_nocancel(p, (struct aio_suspend_nocancel_args *)uap, retval);
}


int
aio_suspend_nocancel(proc_t p, struct aio_suspend_nocancel_args *uap, int *retval)
{
	int                     error;
	int                     i;
	uint64_t                abstime;
	struct user_timespec    ts;
	aio_workq_entry        *entryp;
	user_addr_t            *aiocbpp;
	size_t                  aiocbpp_size;

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_suspend) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(p), uap->nent, 0, 0, 0);

	*retval = -1;
	abstime = 0;
	aiocbpp = NULL;

	if (!aio_has_any_work()) {
		error = EINVAL;
		goto ExitThisRoutine;
	}

	if (uap->nent < 1 || uap->nent > aio_max_requests_per_process ||
	    os_mul_overflow(sizeof(user_addr_t), uap->nent, &aiocbpp_size)) {
		error = EINVAL;
		goto ExitThisRoutine;
	}

	if (uap->timeoutp != USER_ADDR_NULL) {
		if (proc_is64bit(p)) {
			struct user64_timespec temp;
			error = copyin(uap->timeoutp, &temp, sizeof(temp));
			if (error == 0) {
				ts.tv_sec = (user_time_t)temp.tv_sec;
				ts.tv_nsec = (user_long_t)temp.tv_nsec;
			}
		} else {
			struct user32_timespec temp;
			error = copyin(uap->timeoutp, &temp, sizeof(temp));
			if (error == 0) {
				ts.tv_sec = temp.tv_sec;
				ts.tv_nsec = temp.tv_nsec;
			}
		}
		if (error != 0) {
			error = EAGAIN;
			goto ExitThisRoutine;
		}

		if (ts.tv_sec < 0 || ts.tv_nsec < 0 || ts.tv_nsec >= 1000000000) {
			error = EINVAL;
			goto ExitThisRoutine;
		}

		nanoseconds_to_absolutetime((uint64_t)ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec,
		    &abstime);
		clock_absolutetime_interval_to_deadline(abstime, &abstime);
	}

	aiocbpp = kheap_alloc(KHEAP_TEMP, aiocbpp_size, Z_WAITOK);
	if (aiocbpp == NULL || aio_copy_in_list(p, uap->aiocblist, aiocbpp, uap->nent)) {
		error = EAGAIN;
		goto ExitThisRoutine;
	}

	                                                             
check_for_our_aiocbp:
	aio_proc_lock_spin(p);
	for (i = 0; i < uap->nent; i++) {
		user_addr_t     aiocbp;

		                                              
		aiocbp = *(aiocbpp + i);
		if (aiocbp == USER_ADDR_NULL) {
			continue;
		}

		                                                               
		TAILQ_FOREACH(entryp, &p->p_aio_doneq, aio_proc_link) {
			ASSERT_AIO_FROM_PROC(entryp, p);
			if (entryp->uaiocbp == aiocbp) {
				aio_proc_unlock(p);
				*retval = 0;
				error = 0;
				goto ExitThisRoutine;
			}
		}
	}

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_suspend_sleep) | DBG_FUNC_NONE,
	    VM_KERNEL_ADDRPERM(p), uap->nent, 0, 0, 0);

	  
                                                                          
                                                                            
                                                                         
                                                                 
    

	error = msleep1(&p->AIO_SUSPEND_SLEEP_CHAN, aio_proc_mutex(p),
	    PCATCH | PWAIT | PDROP, "aio_suspend", abstime);
	if (error == 0) {
		  
                                                
                                                                         
                                                                              
                                                                          
                                                                     
                                                
     
		goto check_for_our_aiocbp;
	} else if (error == EWOULDBLOCK) {
		                         
		error = EAGAIN;
	} else {
		                         
		error = EINTR;
	}

ExitThisRoutine:
	if (aiocbpp != NULL) {
		kheap_free(KHEAP_TEMP, aiocbpp, aiocbpp_size);
	}

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_suspend) | DBG_FUNC_END,
	    VM_KERNEL_ADDRPERM(p), uap->nent, error, 0, 0);

	return error;
}


                                                                        
                                                            
                          
   

int
aio_write(proc_t p, struct aio_write_args *uap, int *retval __unused)
{
	int error;

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_write) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, 0, 0, 0);

	error = aio_queue_async_request(p, uap->aiocbp, AIO_WRITE);

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_write) | DBG_FUNC_END,
	    VM_KERNEL_ADDRPERM(p), uap->aiocbp, error, 0, 0);

	return error;
}


static int
aio_copy_in_list(proc_t procp, user_addr_t aiocblist, user_addr_t *aiocbpp,
    int nent)
{
	int result;

	                                         
	result = copyin(aiocblist, aiocbpp,
	    proc_is64bit(procp) ? (nent * sizeof(user64_addr_t))
	    : (nent * sizeof(user32_addr_t)));
	if (result) {
		return result;
	}

	  
                                                      
                                                    
                  
    
	if (!proc_is64bit(procp)) {
		                                                  
		user32_addr_t *my_ptrp = ((user32_addr_t *)aiocbpp) + (nent - 1);
		user_addr_t *my_addrp = aiocbpp + (nent - 1);

		for (int i = 0; i < nent; i++, my_ptrp--, my_addrp--) {
			*my_addrp = (user_addr_t) (*my_ptrp);
		}
	}

	return 0;
}


static int
aio_copy_in_sigev(proc_t procp, user_addr_t sigp, struct user_sigevent *sigev)
{
	int     result = 0;

	if (sigp == USER_ADDR_NULL) {
		goto out;
	}

	  
                                                             
                                                                
                                                              
                                                                
                                         
   
                                                          
                                        
    
	if (proc_is64bit(procp)) {
#if __LP64__
		struct user64_sigevent sigevent64;

		result = copyin(sigp, &sigevent64, sizeof(sigevent64));
		if (result == 0) {
			sigev->sigev_notify = sigevent64.sigev_notify;
			sigev->sigev_signo = sigevent64.sigev_signo;
			sigev->sigev_value.size_equivalent.sival_int = sigevent64.sigev_value.size_equivalent.sival_int;
			sigev->sigev_notify_function = sigevent64.sigev_notify_function;
			sigev->sigev_notify_attributes = sigevent64.sigev_notify_attributes;
		}
#else
		panic("64bit process on 32bit kernel is not supported");
#endif
	} else {
		struct user32_sigevent sigevent32;

		result = copyin(sigp, &sigevent32, sizeof(sigevent32));
		if (result == 0) {
			sigev->sigev_notify = sigevent32.sigev_notify;
			sigev->sigev_signo = sigevent32.sigev_signo;
			sigev->sigev_value.size_equivalent.sival_int = sigevent32.sigev_value.sival_int;
			sigev->sigev_notify_function = CAST_USER_ADDR_T(sigevent32.sigev_notify_function);
			sigev->sigev_notify_attributes = CAST_USER_ADDR_T(sigevent32.sigev_notify_attributes);
		}
	}

	if (result != 0) {
		result = EAGAIN;
	}

out:
	return result;
}

  
                                                         
                                                                
                                                                  
                                                                      
                                                       
   
static int
aio_sigev_validate(const struct user_sigevent *sigev)
{
	switch (sigev->sigev_notify) {
	case SIGEV_SIGNAL:
	{
		int signum;

		                                             
		signum = sigev->sigev_signo;
		if (signum <= 0 || signum >= NSIG ||
		    signum == SIGKILL || signum == SIGSTOP) {
			return EINVAL;
		}
	}
	break;

	case SIGEV_NONE:
		break;

	case SIGEV_THREAD:
	                       

	default:
		return EINVAL;
	}

	return 0;
}


  
                              
  
                                                                          
                                                                            
                                                            
  
                                               
                                              
                                   
  
                                             
  
                                                           
  
                                                               
                                                           
                                                
  
                           
   
static bool
aio_try_enqueue_work_locked(proc_t procp, aio_workq_entry *entryp,
    aio_workq_entry *leader)
{
	aio_workq_t queue = aio_entry_workq(entryp);

	ASSERT_AIO_PROC_LOCK_OWNED(procp);

	                     
	if (!aio_try_proc_insert_active_locked(procp, entryp)) {
		return false;
	}

	if (leader) {
		aio_entry_ref(leader);                                               
		leader->lio_pending++;
		entryp->lio_leader = leader;
	}

	                    
	aio_entry_ref(entryp);                                               
	aio_workq_lock_spin(queue);
	aio_workq_add_entry_locked(queue, entryp);
	waitq_wakeup64_one(&queue->aioq_waitq, CAST_EVENT64_T(queue),
	    THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);
	aio_workq_unlock(queue);

	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_AIO, AIO_work_queued) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(procp), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
	    entryp->flags, entryp->aiocb.aio_fildes, 0);
	KERNEL_DEBUG_CONSTANT(BSDDBG_CODE(DBG_BSD_AIO, AIO_work_queued) | DBG_FUNC_END,
	    entryp->aiocb.aio_offset, 0, entryp->aiocb.aio_nbytes, 0, 0);
	return true;
}


  
                                                                       
                                                                   
                        
  
                                                                     
                                                                       
                                   
   
int
lio_listio(proc_t p, struct lio_listio_args *uap, int *retval __unused)
{
	aio_workq_entry         *entries[AIO_LISTIO_MAX] = { };
	user_addr_t              aiocbpp[AIO_LISTIO_MAX];
	struct user_sigevent     aiosigev = { };
	int                      result = 0;
	int                      lio_count = 0;

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_listio) | DBG_FUNC_START,
	    VM_KERNEL_ADDRPERM(p), uap->nent, uap->mode, 0, 0);

	if (!(uap->mode == LIO_NOWAIT || uap->mode == LIO_WAIT)) {
		result = EINVAL;
		goto ExitRoutine;
	}

	if (uap->nent < 1 || uap->nent > AIO_LISTIO_MAX) {
		result = EINVAL;
		goto ExitRoutine;
	}

	  
                                                                   
                                                                     
    
	if (uap->sigp != USER_ADDR_NULL) {
		result = aio_copy_in_sigev(p, uap->sigp, &aiosigev);
		if (result) {
			goto ExitRoutine;
		}
		result = aio_sigev_validate(&aiosigev);
		if (result) {
			goto ExitRoutine;
		}
	}

	if (aio_copy_in_list(p, uap->aiocblist, aiocbpp, uap->nent)) {
		result = EAGAIN;
		goto ExitRoutine;
	}

	  
                              
    
	for (int i = 0; i < uap->nent; i++) {
		aio_workq_entry *entryp;

		                                              
		if (aiocbpp[i] == USER_ADDR_NULL) {
			continue;
		}

		entryp = aio_create_queue_entry(p, aiocbpp[i], AIO_LIO);
		if (entryp == NULL) {
			result = EAGAIN;
			goto ExitRoutine;
		}

		  
                                                     
                    
     
		entries[lio_count++] = entryp;
		if (uap->mode == LIO_NOWAIT) {
			                               
			entryp->aiocb.aio_sigevent = aiosigev;
		}
	}

	if (lio_count == 0) {
		                               
		goto ExitRoutine;
	}

	  
                                                        
   
                                                 
                                                 
    

	aio_workq_entry *leader = entries[0];
	if (uap->mode == LIO_WAIT) {
		aio_entry_ref(leader);                     
	}

	aio_proc_lock_spin(p);

	for (int i = 0; i < lio_count; i++) {
		if (aio_try_enqueue_work_locked(p, entries[i], leader)) {
			entries[i] = NULL;                              
		} else {
			result = EAGAIN;
		}
	}

	if (uap->mode == LIO_WAIT && result == 0) {
		leader->flags |= AIO_LIO_WAIT;

		while (leader->lio_pending) {
			                                                             
			if (msleep(leader, aio_proc_mutex(p),
			    PCATCH | PRIBIO | PSPIN, "lio_listio", 0) != 0) {
				result = EINTR;
				break;
			}
		}

		leader->flags &= ~AIO_LIO_WAIT;
	}

	aio_proc_unlock(p);

	if (uap->mode == LIO_WAIT) {
		aio_entry_unref(leader);
	}

ExitRoutine:
	                                 
	for (int i = 0; i < lio_count; i++) {
		if (entries[i]) {
			aio_entry_unref(entries[i]);
		}
	}

	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_listio) | DBG_FUNC_END,
	    VM_KERNEL_ADDRPERM(p), result, 0, 0, 0);

	return result;
}


  
                                                                 
                                                                     
                               
   
__attribute__((noreturn))
static void
aio_work_thread(void *arg __unused, wait_result_t wr __unused)
{
	aio_workq_entry *entryp;
	int              error;
	vm_map_t         currentmap;
	vm_map_t         oldmap = VM_MAP_NULL;
	task_t           oldaiotask = TASK_NULL;
	struct uthread  *uthreadp = NULL;
	proc_t           p = NULL;

	for (;;) {
		  
                                   
                                    
     
		entryp = aio_get_some_work();
		p = entryp->procp;

		KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_worker_thread) | DBG_FUNC_START,
		    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
		    entryp->flags, 0, 0);

		  
                                                                
                                                            
                                                            
     
		currentmap = get_task_map((current_proc())->task);
		if (currentmap != entryp->aio_map) {
			uthreadp = (struct uthread *) get_bsdthread_info(current_thread());
			oldaiotask = uthreadp->uu_aio_task;
			  
                                                                      
                                                                
                                                                
                                          
      
			uthreadp->uu_aio_task = p->task;
			oldmap = vm_map_switch(entryp->aio_map);
		}

		if ((entryp->flags & AIO_READ) != 0) {
			error = do_aio_read(entryp);
		} else if ((entryp->flags & AIO_WRITE) != 0) {
			error = do_aio_write(entryp);
		} else if ((entryp->flags & (AIO_FSYNC | AIO_DSYNC)) != 0) {
			error = do_aio_fsync(entryp);
		} else {
			error = EINVAL;
		}

		                     
		if (currentmap != entryp->aio_map) {
			vm_map_switch(oldmap);
			uthreadp->uu_aio_task = oldaiotask;
		}

		                         
		vm_map_deallocate(entryp->aio_map);
		entryp->aio_map = VM_MAP_NULL;

		KERNEL_DEBUG(SDDBG_CODE(DBG_BSD_AIO, AIO_worker_thread) | DBG_FUNC_END,
		    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
		    entryp->errorval, entryp->returnval, 0);

		                                                                       
		                               
		aio_proc_lock(p);
		entryp->errorval = error;
		do_aio_completion_and_unlock(p, entryp);
	}
}


  
                                                                                  
                                                                                   
                                                                     
                                         
   
static aio_workq_entry *
aio_get_some_work(void)
{
	aio_workq_entry *entryp = NULL;
	aio_workq_t      queue = NULL;

	                                                                       
	queue = &aio_anchor.aio_async_workqs[0];
	aio_workq_lock_spin(queue);

	  
                        
   
                                                                
                                          
    
	while ((entryp = TAILQ_FIRST(&queue->aioq_entries))) {
		  
                                                                  
                                                        
     

		aio_workq_remove_entry_locked(queue, entryp);

		aio_workq_unlock(queue);

		  
                                                                             
                                                     
     
		if ((entryp->flags & AIO_FSYNC) != 0) {
			  
                                                      
                           
      
			aio_proc_lock_spin(entryp->procp);
			if (aio_delay_fsync_request(entryp)) {
				                                                                       
				KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_fsync_delay) | DBG_FUNC_NONE,
				    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
				    0, 0, 0);

				aio_proc_unlock(entryp->procp);

				aio_workq_lock_spin(queue);
				aio_workq_add_entry_locked(queue, entryp);
				continue;
			}
			aio_proc_unlock(entryp->procp);
		}

		return entryp;
	}

	                                                     
	waitq_assert_wait64(&queue->aioq_waitq, CAST_EVENT64_T(queue), THREAD_UNINT, 0);
	aio_workq_unlock(queue);
	thread_block(aio_work_thread);

	__builtin_unreachable();
}

  
                                                                                     
                                                                                      
                      
   
static boolean_t
aio_delay_fsync_request(aio_workq_entry *entryp)
{
	if (proc_in_teardown(entryp->procp)) {
		  
                                                                         
                                                      
     
		return FALSE;
	}

	if (entryp == TAILQ_FIRST(&entryp->procp->p_aio_activeq)) {
		return FALSE;
	}

	return TRUE;
}

static aio_workq_entry *
aio_create_queue_entry(proc_t procp, user_addr_t aiocbp, aio_entry_flags_t flags)
{
	aio_workq_entry *entryp;

	entryp = zalloc_flags(aio_workq_zonep, Z_WAITOK | Z_ZERO);
	entryp->procp = procp;
	entryp->uaiocbp = aiocbp;
	entryp->flags = flags;
	                                         
	os_ref_init(&entryp->aio_refcount, &aio_refgrp);

	if (proc_is64bit(procp)) {
		struct user64_aiocb aiocb64;

		if (copyin(aiocbp, &aiocb64, sizeof(aiocb64)) != 0) {
			goto error_exit;
		}
		do_munge_aiocb_user64_to_user(&aiocb64, &entryp->aiocb);
	} else {
		struct user32_aiocb aiocb32;

		if (copyin(aiocbp, &aiocb32, sizeof(aiocb32)) != 0) {
			goto error_exit;
		}
		do_munge_aiocb_user32_to_user(&aiocb32, &entryp->aiocb);
	}

	                                                                       
	if (aio_validate(procp, entryp) != 0) {
		goto error_exit;
	}

	                                                                     
	entryp->aio_map = get_task_map(procp->task);
	vm_map_reference(entryp->aio_map);

	                                                                            
	entryp->thread = current_thread();
	thread_reference(entryp->thread);
	return entryp;

error_exit:
	zfree(aio_workq_zonep, entryp);
	return NULL;
}


  
                                                                                
                                                                               
                                                                        
                          
   
static int
aio_queue_async_request(proc_t procp, user_addr_t aiocbp,
    aio_entry_flags_t flags)
{
	aio_workq_entry *entryp;
	int              result;

	entryp = aio_create_queue_entry(procp, aiocbp, flags);
	if (entryp == NULL) {
		result = EAGAIN;
		goto error_noalloc;
	}

	aio_proc_lock_spin(procp);
	if (!aio_try_enqueue_work_locked(procp, entryp, NULL)) {
		result = EAGAIN;
		goto error_exit;
	}
	aio_proc_unlock(procp);
	return 0;

error_exit:
	  
                                                         
                              
    
	aio_proc_unlock(procp);
	aio_free_request(entryp);
error_noalloc:
	return result;
}


  
                                                                   
                                                                   
                                                             
   
static void
aio_free_request(aio_workq_entry *entryp)
{
	if (entryp->aio_proc_link.tqe_prev || entryp->aio_workq_link.tqe_prev) {
		panic("aio_workq_entry %p being freed while still enqueued", entryp);
	}

	                                                
	if (VM_MAP_NULL != entryp->aio_map) {
		vm_map_deallocate(entryp->aio_map);
	}

	                                                               
	if (NULL != entryp->thread) {
		thread_deallocate(entryp->thread);
	}

	zfree(aio_workq_zonep, entryp);
}


  
               
  
                                                           
   
static int
aio_validate(proc_t p, aio_workq_entry *entryp)
{
	struct fileproc *fp;
	int              flag;
	int              result;

	result = 0;

	if ((entryp->flags & AIO_LIO) != 0) {
		if (entryp->aiocb.aio_lio_opcode == LIO_READ) {
			entryp->flags |= AIO_READ;
		} else if (entryp->aiocb.aio_lio_opcode == LIO_WRITE) {
			entryp->flags |= AIO_WRITE;
		} else if (entryp->aiocb.aio_lio_opcode == LIO_NOP) {
			return 0;
		} else {
			return EINVAL;
		}
	}

	flag = FREAD;
	if ((entryp->flags & (AIO_WRITE | AIO_FSYNC | AIO_DSYNC)) != 0) {
		flag = FWRITE;
	}

	if ((entryp->flags & (AIO_READ | AIO_WRITE)) != 0) {
		if (entryp->aiocb.aio_nbytes > INT_MAX ||
		    entryp->aiocb.aio_buf == USER_ADDR_NULL ||
		    entryp->aiocb.aio_offset < 0) {
			return EINVAL;
		}
	}

	result = aio_sigev_validate(&entryp->aiocb.aio_sigevent);
	if (result) {
		return result;
	}

	                                                            
                                            
    
	proc_fdlock(p);

	fp = fp_get_noref_locked(p, entryp->aiocb.aio_fildes);
	if (fp == NULL) {
		result = EBADF;
	} else if ((fp->fp_glob->fg_flag & flag) == 0) {
		                                        
		result = EBADF;
	} else if (FILEGLOB_DTYPE(fp->fp_glob) != DTYPE_VNODE) {
		                        
		result = ESPIPE;
	} else {
		fp->fp_flags |= FP_AIOISSUED;
	}

	proc_fdunlock(p);

	return result;
}

  
                                                             
   
static void
do_aio_completion_and_unlock(proc_t p, aio_workq_entry *entryp)
{
	aio_workq_entry *leader = entryp->lio_leader;
	int              lio_pending = 0;
	bool             do_signal = false;

	ASSERT_AIO_PROC_LOCK_OWNED(p);

	aio_proc_move_done_locked(p, entryp);

	if (leader) {
		lio_pending = --leader->lio_pending;
		if (lio_pending < 0) {
			panic("lio_pending accounting mistake");
		}
		if (lio_pending == 0 && (leader->flags & AIO_LIO_WAIT)) {
			wakeup(leader);
		}
		entryp->lio_leader = NULL;                                  
	}

	  
                                                                   
                                                                       
                                                                           
                                                                      
                                                                     
                                                                    
                                                                     
    
	if (__improbable(entryp->flags & AIO_EXIT_WAIT)) {
		KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_completion_cleanup_wait) | DBG_FUNC_NONE,
		    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
		    0, 0, 0);

		if (!aio_has_active_requests_for_process(p)) {
			  
                                                                         
                                                                     
      
			wakeup_one((caddr_t)&p->AIO_CLEANUP_SLEEP_CHAN);

			KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_completion_cleanup_wake) | DBG_FUNC_NONE,
			    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
			    0, 0, 0);
		}
	} else if (entryp->aiocb.aio_sigevent.sigev_notify == SIGEV_SIGNAL) {
		  
                                                              
                                                     
     
		do_signal = (lio_pending == 0);
	}

	if (__improbable(entryp->flags & AIO_CLOSE_WAIT)) {
		KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_completion_cleanup_wait) | DBG_FUNC_NONE,
		    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
		    0, 0, 0);

		if (!aio_proc_has_active_requests_for_file(p, entryp->aiocb.aio_fildes)) {
			                                                               
			wakeup(&p->AIO_CLEANUP_SLEEP_CHAN);

			KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_completion_cleanup_wake) | DBG_FUNC_NONE,
			    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
			    0, 0, 0);
		}
	}

	aio_proc_unlock(p);

	if (do_signal) {
		KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_completion_sig) | DBG_FUNC_NONE,
		    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp),
		    entryp->aiocb.aio_sigevent.sigev_signo, 0, 0);

		psignal(p, entryp->aiocb.aio_sigevent.sigev_signo);
	}

	  
                                                                                
                                                                                   
                                                                                 
                                                                                
                                               
    
	wakeup(&p->AIO_SUSPEND_SLEEP_CHAN);
	KERNEL_DEBUG(BSDDBG_CODE(DBG_BSD_AIO, AIO_completion_suspend_wake) | DBG_FUNC_NONE,
	    VM_KERNEL_ADDRPERM(p), VM_KERNEL_ADDRPERM(entryp->uaiocbp), 0, 0, 0);

	aio_entry_unref(entryp);                                      
	if (leader) {
		aio_entry_unref(leader);                     
	}
}


  
              
   
static int
do_aio_read(aio_workq_entry *entryp)
{
	struct proc     *p = entryp->procp;
	struct fileproc *fp;
	int error;

	if ((error = fp_lookup(p, entryp->aiocb.aio_fildes, &fp, 0))) {
		return error;
	}

	if (fp->fp_glob->fg_flag & FREAD) {
		struct vfs_context context = {
			.vc_thread = entryp->thread,              
			.vc_ucred = fp->fp_glob->fg_cred,
		};

		error = dofileread(&context, fp,
		    entryp->aiocb.aio_buf,
		    entryp->aiocb.aio_nbytes,
		    entryp->aiocb.aio_offset, FOF_OFFSET,
		    &entryp->returnval);
	} else {
		error = EBADF;
	}

	fp_drop(p, entryp->aiocb.aio_fildes, fp, 0);
	return error;
}


  
               
   
static int
do_aio_write(aio_workq_entry *entryp)
{
	struct proc     *p = entryp->procp;
	struct fileproc *fp;
	int error;

	if ((error = fp_lookup(p, entryp->aiocb.aio_fildes, &fp, 0))) {
		return error;
	}

	if (fp->fp_glob->fg_flag & FWRITE) {
		struct vfs_context context = {
			.vc_thread = entryp->thread,              
			.vc_ucred = fp->fp_glob->fg_cred,
		};
		int flags = FOF_PCRED;

		if ((fp->fp_glob->fg_flag & O_APPEND) == 0) {
			flags |= FOF_OFFSET;
		}

		                                                               
		error = dofilewrite(&context,
		    fp,
		    entryp->aiocb.aio_buf,
		    entryp->aiocb.aio_nbytes,
		    entryp->aiocb.aio_offset,
		    flags,
		    &entryp->returnval);
	} else {
		error = EBADF;
	}

	fp_drop(p, entryp->aiocb.aio_fildes, fp, 0);
	return error;
}


  
                                                                              
                    
   
static bool
aio_has_active_requests_for_process(proc_t procp)
{
	return !TAILQ_EMPTY(&procp->p_aio_activeq);
}

  
                               
   
static bool
aio_proc_has_active_requests_for_file(proc_t procp, int fd)
{
	aio_workq_entry *entryp;

	TAILQ_FOREACH(entryp, &procp->p_aio_activeq, aio_proc_link) {
		if (entryp->aiocb.aio_fildes == fd) {
			return true;
		}
	}

	return false;
}


  
               
   
static int
do_aio_fsync(aio_workq_entry *entryp)
{
	struct proc            *p = entryp->procp;
	struct vnode           *vp;
	struct fileproc        *fp;
	int                     sync_flag;
	int                     error;

	  
                                                                     
   
                                                                   
                                                                   
                                              
   
                                                                     
                                                                    
                               
   
                                                                    
                                               
    
	if (entryp->flags & AIO_FSYNC) {
		sync_flag = MNT_WAIT;
	} else {
		sync_flag = MNT_DWAIT;
	}

	error = fp_get_ftype(p, entryp->aiocb.aio_fildes, DTYPE_VNODE, ENOTSUP, &fp);
	if (error != 0) {
		entryp->returnval = -1;
		return error;
	}
	vp = fp->fp_glob->fg_data;

	if ((error = vnode_getwithref(vp)) == 0) {
		struct vfs_context context = {
			.vc_thread = entryp->thread,              
			.vc_ucred = fp->fp_glob->fg_cred,
		};

		error = VNOP_FSYNC(vp, sync_flag, &context);

		(void)vnode_put(vp);
	} else {
		entryp->returnval = -1;
	}

	fp_drop(p, entryp->aiocb.aio_fildes, fp, 0);
	return error;
}


  
                                                                  
                                                               
                            
  
                                                    
   
static boolean_t
is_already_queued(proc_t procp, user_addr_t aiocbp)
{
	aio_workq_entry *entryp;
	boolean_t        result;

	result = FALSE;

	                                                                            
	TAILQ_FOREACH(entryp, &procp->p_aio_doneq, aio_proc_link) {
		if (aiocbp == entryp->uaiocbp) {
			result = TRUE;
			goto ExitThisRoutine;
		}
	}

	                                                               
	TAILQ_FOREACH(entryp, &procp->p_aio_activeq, aio_proc_link) {
		if (aiocbp == entryp->uaiocbp) {
			result = TRUE;
			goto ExitThisRoutine;
		}
	}

ExitThisRoutine:
	return result;
}


  
                     
   
__private_extern__ void
aio_init(void)
{
	for (int i = 0; i < AIO_NUM_WORK_QUEUES; i++) {
		aio_workq_init(&aio_anchor.aio_async_workqs[i]);
	}

	_aio_create_worker_threads(aio_worker_threads);
}


  
                                   
   
__private_extern__ void
_aio_create_worker_threads(int num)
{
	int i;

	                                                                
	for (i = 0; i < num; i++) {
		thread_t                myThread;

		if (KERN_SUCCESS != kernel_thread_start(aio_work_thread, NULL, &myThread)) {
			printf("%s - failed to create a work thread \n", __FUNCTION__);
		} else {
			thread_deallocate(myThread);
		}
	}
}

  
                                      
   
task_t
get_aiotask(void)
{
	return ((struct uthread *)get_bsdthread_info(current_thread()))->uu_aio_task;
}


  
                                 
                                                                          
                                                                        
                                           
   
static void
do_munge_aiocb_user32_to_user(struct user32_aiocb *my_aiocbp, struct user_aiocb *the_user_aiocbp)
{
	the_user_aiocbp->aio_fildes = my_aiocbp->aio_fildes;
	the_user_aiocbp->aio_offset = my_aiocbp->aio_offset;
	the_user_aiocbp->aio_buf = CAST_USER_ADDR_T(my_aiocbp->aio_buf);
	the_user_aiocbp->aio_nbytes = my_aiocbp->aio_nbytes;
	the_user_aiocbp->aio_reqprio = my_aiocbp->aio_reqprio;
	the_user_aiocbp->aio_lio_opcode = my_aiocbp->aio_lio_opcode;

	                                                                   
	                                                                   
	                                                                       
	                                                        
	                                                                       
	                              
	      
	the_user_aiocbp->aio_sigevent.sigev_notify = my_aiocbp->aio_sigevent.sigev_notify;
	the_user_aiocbp->aio_sigevent.sigev_signo = my_aiocbp->aio_sigevent.sigev_signo;
	the_user_aiocbp->aio_sigevent.sigev_value.size_equivalent.sival_int =
	    my_aiocbp->aio_sigevent.sigev_value.sival_int;
	the_user_aiocbp->aio_sigevent.sigev_notify_function =
	    CAST_USER_ADDR_T(my_aiocbp->aio_sigevent.sigev_notify_function);
	the_user_aiocbp->aio_sigevent.sigev_notify_attributes =
	    CAST_USER_ADDR_T(my_aiocbp->aio_sigevent.sigev_notify_attributes);
}

                                                                    
                                                         
   
#if !__LP64__
__dead2
#endif
static void
do_munge_aiocb_user64_to_user(struct user64_aiocb *my_aiocbp, struct user_aiocb *the_user_aiocbp)
{
#if __LP64__
	the_user_aiocbp->aio_fildes = my_aiocbp->aio_fildes;
	the_user_aiocbp->aio_offset = my_aiocbp->aio_offset;
	the_user_aiocbp->aio_buf = my_aiocbp->aio_buf;
	the_user_aiocbp->aio_nbytes = my_aiocbp->aio_nbytes;
	the_user_aiocbp->aio_reqprio = my_aiocbp->aio_reqprio;
	the_user_aiocbp->aio_lio_opcode = my_aiocbp->aio_lio_opcode;

	the_user_aiocbp->aio_sigevent.sigev_notify = my_aiocbp->aio_sigevent.sigev_notify;
	the_user_aiocbp->aio_sigevent.sigev_signo = my_aiocbp->aio_sigevent.sigev_signo;
	the_user_aiocbp->aio_sigevent.sigev_value.size_equivalent.sival_int =
	    my_aiocbp->aio_sigevent.sigev_value.size_equivalent.sival_int;
	the_user_aiocbp->aio_sigevent.sigev_notify_function =
	    my_aiocbp->aio_sigevent.sigev_notify_function;
	the_user_aiocbp->aio_sigevent.sigev_notify_attributes =
	    my_aiocbp->aio_sigevent.sigev_notify_attributes;
#else
#pragma unused(my_aiocbp, the_user_aiocbp)
	panic("64bit process on 32bit kernel is not supported");
#endif
}
