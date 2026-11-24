/*-
 * Copyright (c) 1999-2016 Apple Inc.
 * Copyright (c) 2006-2008 Robert N. M. Watson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/namei.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/ucred.h>
#include <sys/uio.h>
#include <sys/unistd.h>
#include <sys/file_internal.h>
#include <sys/vnode_internal.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/malloc.h>
#include <sys/un.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/vfs_context.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>

#include <bsm/audit.h>
#include <bsm/audit_internal.h>
#include <bsm/audit_kevents.h>

#include <security/audit/audit.h>
#include <security/audit/audit_bsd.h>
#include <security/audit/audit_private.h>

#include <mach/host_priv.h>
#include <mach/host_special_ports.h>
#include <mach/audit_triggers_server.h>

#include <kern/host.h>
#include <kern/zalloc.h>
#include <kern/sched_prim.h>
#include <kern/task.h>

#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>

  
                                                  
   
static thread_t audit_thread;

  
                                                                        
                                                                            
                                                                           
                                                                           
                                                                       
                                                                       
                   
   
static int                      audit_file_rotate_wait;
static struct slck              audit_worker_sl;
static struct vfs_context       audit_ctx;
static struct vnode             *audit_vp;

#define AUDIT_WORKER_SX_INIT()          slck_init(&audit_worker_sl,     \
	                                    "audit_worker_sl")
#define AUDIT_WORKER_SX_XLOCK()         slck_lock(&audit_worker_sl)
#define AUDIT_WORKER_SX_XUNLOCK()       slck_unlock(&audit_worker_sl)
#define AUDIT_WORKER_SX_ASSERT()        slck_assert(&audit_worker_sl, SL_OWNED)
#define AUDIT_WORKER_SX_DESTROY()       slck_destroy(&audit_worker_sl)

  
                                                                        
                                 
   
static int                      audit_q_draining;

  
                                                                               
                              
   
static struct kaudit_record     audit_drain_kar = {
	.k_ar = {
		.ar_event = AUE_NULL,
	},
	.k_ar_commit = AR_DRAIN_QUEUE,
};

  
                                                                          
                                                                             
                                
  
                                                                           
                                                              
   
static void
audit_record_write(struct vnode *vp, struct vfs_context *ctx, void *data,
    size_t len)
{
	static struct timeval last_lowspace_trigger;
	static struct timeval last_fail;
	static int cur_lowspace_trigger;
	struct vfsstatfs *mnt_stat;
	int error;
	static int cur_fail;
	uint64_t temp;
	off_t file_size;

	AUDIT_WORKER_SX_ASSERT();                                    

	if (vp == NULL) {
		return;
	}

	if (vnode_getwithref(vp)) {
		return             ;
	}

	mnt_stat = &vp->v_mount->mnt_vfsstat;

	  
                                                                     
                                                                     
                                                                   
    
	error = vfs_update_vfsstat(vp->v_mount, ctx, VFS_KERNEL_EVENT);
	if (error) {
		goto fail;
	}
	error = vnode_size(vp, &file_size, ctx);
	if (error) {
		goto fail;
	}
	audit_fstat.af_currsz = (u_quad_t)file_size;

	  
                                                  
   
                                                                   
                                                                 
                                                     
   
                                                                      
                                                            
   
                                                                      
                                                                 
   
                                                                      
                                                                    
                                                              
                                                                 
   
                                                                     
                                                                   
         
   
                                                                   
                                                                 
                                          
    
	if (mnt_stat->f_bfree < AUDIT_HARD_LIMIT_FREE_BLOCKS) {
		error = ENOSPC;
		goto fail_enospc;
	}

	  
                                                                    
                                                                      
                         
    
	if (audit_qctrl.aq_minfree != 0) {
		temp = mnt_stat->f_blocks / (100 / audit_qctrl.aq_minfree);
		if (mnt_stat->f_bfree < temp &&
		    ppsratecheck(&last_lowspace_trigger,
		    &cur_lowspace_trigger, 1)) {
			(void)audit_send_trigger(
				AUDIT_TRIGGER_LOW_SPACE);
		}
	}

	  
                                                                    
                                                                   
                                                                
    
	if ((audit_fstat.af_filesz != 0) && (audit_file_rotate_wait == 0) &&
	    ((u_quad_t)file_size >= audit_fstat.af_filesz)) {
		AUDIT_WORKER_SX_ASSERT();

		audit_file_rotate_wait = 1;
		(void)audit_send_trigger(AUDIT_TRIGGER_ROTATE_KERNEL);
	}

	  
                                                                  
                                                                      
                                                                    
                                                                      
                                                                    
                                                                 
                                                                     
                                                                   
                                        
    
	if (audit_fail_stop) {
		if ((unsigned long)((audit_q_len + audit_pre_q_len + 1) *
		    MAX_AUDIT_RECORD_SIZE) / mnt_stat->f_bsize >=
		    (unsigned long)(mnt_stat->f_bfree)) {
			if (ppsratecheck(&last_fail, &cur_fail, 1)) {
				printf("audit_record_write: free space "
				    "below size of audit queue, failing "
				    "stop\n");
			}
			audit_in_failure = 1;
		} else if (audit_in_failure) {
			  
                                                      
                                                        
                       
      
		}
	}

	error = vn_rdwr(UIO_WRITE, vp, data, len, (off_t)0, UIO_SYSSPACE,
	    IO_APPEND | IO_UNIT, vfs_context_ucred(ctx), NULL,
	    vfs_context_proc(ctx));
	if (error == ENOSPC) {
		goto fail_enospc;
	} else if (error) {
		goto fail;
	}

	  
                                                                     
                                                                      
                                                                      
        
   
                                                                      
                                
    
	if (audit_in_failure) {
		if (audit_q_len == 0 && audit_pre_q_len == 0) {
			(void)VNOP_FSYNC(vp, MNT_WAIT, ctx);
			panic("Audit store overflow; record queue drained.");
		}
	}

	vnode_put(vp);
	return;

fail_enospc:
	  
                                                                    
                                                                    
                                                      
    
	if (audit_fail_stop) {
		(void)VNOP_FSYNC(vp, MNT_WAIT, ctx);
		panic("Audit log space exhausted and fail-stop set.");
	}
	(void)audit_send_trigger(AUDIT_TRIGGER_NO_SPACE);
	audit_suspended = 1;

	                 
fail:
	  
                                                                 
                                                     
    
	if (audit_panic_on_write_fail) {
		(void)VNOP_FSYNC(vp, MNT_WAIT, ctx);
		panic("audit_worker: write error %d\n", error);
	} else if (ppsratecheck(&last_fail, &cur_fail, 1)) {
		printf("audit_worker: write error %d\n", error);
	}
	vnode_put(vp);
}

  
                                                                          
                                                                           
                                                                     
                                                                       
                                    
   
static void
audit_worker_process_record(struct kaudit_record *ar)
{
	struct au_record *bsm;
	au_class_t class;
	au_event_t event;
	au_id_t auid;
	int error, sorf;
	int trail_locked;

	  
                                                                   
                                                                     
                                        
    
	if (((ar->k_ar_commit & AR_COMMIT_USER) &&
	    (ar->k_ar_commit & AR_PRESELECT_USER_TRAIL)) ||
	    (ar->k_ar_commit & AR_PRESELECT_TRAIL)) {
		AUDIT_WORKER_SX_XLOCK();
		trail_locked = 1;
	} else {
		trail_locked = 0;
	}

	  
                                                                     
                                
    
	if ((ar->k_ar_commit & AR_COMMIT_USER) &&
	    (ar->k_ar_commit & AR_PRESELECT_USER_TRAIL)) {
		AUDIT_WORKER_SX_ASSERT();
		audit_record_write(audit_vp, &audit_ctx, ar->k_udata,
		    ar->k_ulen);
	}

	if ((ar->k_ar_commit & AR_COMMIT_USER) &&
	    (ar->k_ar_commit & AR_PRESELECT_USER_PIPE)) {
		audit_pipe_submit_user(ar->k_udata, ar->k_ulen);
	}

	if (!(ar->k_ar_commit & AR_COMMIT_KERNEL) ||
	    ((ar->k_ar_commit & AR_PRESELECT_PIPE) == 0 &&
	    (ar->k_ar_commit & AR_PRESELECT_TRAIL) == 0 &&
	    (ar->k_ar_commit & AR_PRESELECT_FILTER) == 0)) {
		goto out;
	}

	auid = ar->k_ar.ar_subj_auid;
	event = ar->k_ar.ar_event;
	class = au_event_class(event);
	if (ar->k_ar.ar_errno == 0) {
		sorf = AU_PRS_SUCCESS;
	} else {
		sorf = AU_PRS_FAILURE;
	}

	error = kaudit_to_bsm(ar, &bsm);
	switch (error) {
	case BSM_NOAUDIT:
		goto out;

	case BSM_FAILURE:
		printf("audit_worker_process_record: BSM_FAILURE\n");
		goto out;

	case BSM_SUCCESS:
		break;

	default:
		panic("kaudit_to_bsm returned %d", error);
	}

	if (ar->k_ar_commit & AR_PRESELECT_TRAIL) {
		AUDIT_WORKER_SX_ASSERT();
		audit_record_write(audit_vp, &audit_ctx, bsm->data, bsm->len);
	}

	if (ar->k_ar_commit & AR_PRESELECT_PIPE) {
		audit_pipe_submit(auid, event, class, sorf,
		    ar->k_ar_commit & AR_PRESELECT_TRAIL, bsm->data,
		    bsm->len);
	}

	if (ar->k_ar_commit & AR_PRESELECT_FILTER) {
		  
                                                             
                           
     
		audit_sdev_submit(auid, ar->k_ar.ar_subj_asid, bsm->data,
		    bsm->len);
	}

	kau_free(bsm);
out:
	if (trail_locked) {
		AUDIT_WORKER_SX_XUNLOCK();
	}
}

  
                                                                       
                                                                            
                                                                           
                                
  
                                                                           
                                              
   
__attribute__((noreturn))
static void
audit_worker(void)
{
	struct kaudit_queue ar_worklist;
	struct kaudit_record *ar;
	int lowater_signal;

	if (audit_ctx.vc_thread == NULL) {
		audit_ctx.vc_thread = current_thread();
	}

	TAILQ_INIT(&ar_worklist);
	mtx_lock(&audit_mtx);
	while (1) {
		mtx_assert(&audit_mtx, MA_OWNED);

		  
                       
     
		while (TAILQ_EMPTY(&audit_q)) {
			cv_wait_continuation(&audit_worker_cv, &audit_mtx,
			    (thread_continue_t)audit_worker);
		}

		  
                                                           
                                                           
                                                          
                                                           
                                 
     
		lowater_signal = 0;
		while ((ar = TAILQ_FIRST(&audit_q))) {
			TAILQ_REMOVE(&audit_q, ar, k_q);
			audit_q_len--;
			if (audit_q_len == audit_qctrl.aq_lowater) {
				lowater_signal++;
			}
			TAILQ_INSERT_TAIL(&ar_worklist, ar, k_q);
		}
		if (lowater_signal) {
			cv_broadcast(&audit_watermark_cv);
		}

		mtx_unlock(&audit_mtx);
		while ((ar = TAILQ_FIRST(&ar_worklist))) {
			TAILQ_REMOVE(&ar_worklist, ar, k_q);
			if (ar->k_ar_commit & AR_DRAIN_QUEUE) {
				audit_q_draining = 0;
				cv_broadcast(&audit_drain_cv);
			} else {
				audit_worker_process_record(ar);
				audit_free(ar);
			}
		}
		mtx_lock(&audit_mtx);
	}
}

  
                                                                            
                                                                       
                                                                          
                                                                           
                                                                        
                                                                             
                                          
   
void
audit_rotate_vnode(kauth_cred_t cred, struct vnode *vp)
{
	kauth_cred_t old_audit_cred;
	struct vnode *old_audit_vp;

	KASSERT((cred != NULL && vp != NULL) || (cred == NULL && vp == NULL),
	    ("audit_rotate_vnode: cred %p vp %p", cred, vp));


	mtx_lock(&audit_mtx);
	if (audit_enabled && (NULL == vp)) {
		                                                         

		  
                                                            
                                                            
     
		audit_enabled = 0;

		  
                                                                
                                                             
                                                          
                              
     
		audit_q_draining = 1;
		while (audit_q_len >= audit_qctrl.aq_hiwater) {
			cv_wait(&audit_watermark_cv, &audit_mtx);
		}
		TAILQ_INSERT_TAIL(&audit_q, &audit_drain_kar, k_q);
		audit_q_len++;
		cv_signal(&audit_worker_cv);
	}

	                                                                    
	while (audit_q_draining) {
		cv_wait(&audit_drain_cv, &audit_mtx);
	}
	mtx_unlock(&audit_mtx);


	  
                                                                    
                                                
    
	AUDIT_WORKER_SX_XLOCK();
	old_audit_cred = audit_ctx.vc_ucred;
	old_audit_vp = audit_vp;
	audit_ctx.vc_ucred = cred;
	audit_vp = vp;
	audit_file_rotate_wait = 0;
	audit_enabled = (audit_vp != NULL);
	AUDIT_WORKER_SX_XUNLOCK();

	  
                                                         
    
	if (old_audit_vp != NULL) {
		if (vnode_get(old_audit_vp) == 0) {
			vn_close(old_audit_vp, AUDIT_CLOSE_FLAGS,
			    vfs_context_kernel());
			vnode_put(old_audit_vp);
		} else {
			printf("audit_rotate_vnode: Couldn't close "
			    "audit file.\n");
		}
		kauth_cred_unref(&old_audit_cred);
	}
}

void
audit_worker_init(void)
{
	AUDIT_WORKER_SX_INIT();
	kernel_thread_start((thread_continue_t)audit_worker, NULL,
	    &audit_thread);
	if (audit_thread == THREAD_NULL) {
		panic("audit_worker_init: Couldn't create audit_worker thread");
	}
}
