/*-
 * Copyright (c) 1999-2020 Apple Inc.
 * Copyright (c) 2006-2007 Robert N. M. Watson
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
 *
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

#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_pcb.h>

#if CONFIG_AUDIT
MALLOC_DEFINE(M_AUDITDATA, "audit_data", "Audit data storage");
MALLOC_DEFINE(M_AUDITPATH, "audit_path", "Audit path storage");
MALLOC_DEFINE(M_AUDITTEXT, "audit_text", "Audit text storage");
KALLOC_HEAP_DEFINE(KHEAP_AUDIT, "Audit", KHEAP_ID_DEFAULT);

  
                                                                         
              
  
                                  
   
int                     audit_enabled;
int                     audit_suspended;

int                     audit_syscalls;
au_class_t              audit_kevent_mask;

  
                                                                           
                                                               
   
au_ctlmode_t audit_ctl_mode;
au_expire_after_t audit_expire_after;

  
                                                                            
                                                                  
   
int                     audit_panic_on_write_fail;
int                     audit_fail_stop;
int                     audit_argv;
int                     audit_arge;

  
                                                            
   
int                     audit_in_failure;

  
                           
   
struct audit_fstat      audit_fstat;

  
                                                 
   
struct au_mask          audit_nae_mask;

  
                                                                       
             
   
struct mtx              audit_mtx;

  
                                                                            
                                                                       
                                                                           
                                                                           
                                                                   
                             
   
struct kaudit_queue     audit_q;
int                     audit_q_len;
int                     audit_pre_q_len;

  
                                                                          
   
struct au_qctrl         audit_qctrl;

  
                                                                            
                                                                      
   
struct cv               audit_worker_cv;

  
                                                                          
         
   
struct cv               audit_drain_cv;

  
                                                                           
                                                                             
                     
   
struct cv               audit_watermark_cv;

  
                                                                           
                                                                            
         
   
static struct cv        audit_fail_cv;

static ZONE_DECLARE(audit_record_zone, "audit_zone",
    sizeof(struct kaudit_record), ZC_NONE);

  
                                                                       
                                                                    
                                                                           
           
   
static struct auditinfo_addr    audit_kinfo;
static struct rwlock            audit_kinfo_lock;

#define KINFO_LOCK_INIT()       rw_init(&audit_kinfo_lock,              \
	                                "audit_kinfo_lock")
#define KINFO_RLOCK()           rw_rlock(&audit_kinfo_lock)
#define KINFO_WLOCK()           rw_wlock(&audit_kinfo_lock)
#define KINFO_RUNLOCK()         rw_runlock(&audit_kinfo_lock)
#define KINFO_WUNLOCK()         rw_wunlock(&audit_kinfo_lock)

void
audit_set_kinfo(struct auditinfo_addr *ak)
{
	KASSERT(ak->ai_termid.at_type == AU_IPv4 ||
	    ak->ai_termid.at_type == AU_IPv6,
	    ("audit_set_kinfo: invalid address type"));

	KINFO_WLOCK();
	bcopy(ak, &audit_kinfo, sizeof(audit_kinfo));
	KINFO_WUNLOCK();
}

void
audit_get_kinfo(struct auditinfo_addr *ak)
{
	KASSERT(audit_kinfo.ai_termid.at_type == AU_IPv4 ||
	    audit_kinfo.ai_termid.at_type == AU_IPv6,
	    ("audit_set_kinfo: invalid address type"));

	KINFO_RLOCK();
	bcopy(&audit_kinfo, ak, sizeof(*ak));
	KINFO_RUNLOCK();
}

  
                                                   
   
static void
audit_record_ctor(proc_t p, struct kaudit_record *ar)
{
	kauth_cred_t cred;

	bzero(ar, sizeof(*ar));
	ar->k_ar.ar_magic = AUDIT_RECORD_MAGIC;
	nanotime(&ar->k_ar.ar_starttime);

	if (PROC_NULL != p) {
		cred = kauth_cred_proc_ref(p);

		  
                                   
     
		cru2x(cred, &ar->k_ar.ar_subj_cred);
		ar->k_ar.ar_subj_ruid = kauth_cred_getruid(cred);
		ar->k_ar.ar_subj_rgid = kauth_cred_getrgid(cred);
		ar->k_ar.ar_subj_egid = kauth_cred_getgid(cred);
		ar->k_ar.ar_subj_pid = p->p_pid;
		ar->k_ar.ar_subj_auid = cred->cr_audit.as_aia_p->ai_auid;
		ar->k_ar.ar_subj_asid = cred->cr_audit.as_aia_p->ai_asid;
		bcopy(&cred->cr_audit.as_mask, &ar->k_ar.ar_subj_amask,
		    sizeof(struct au_mask));
		bcopy(&cred->cr_audit.as_aia_p->ai_termid,
		    &ar->k_ar.ar_subj_term_addr, sizeof(struct au_tid_addr));
		kauth_cred_unref(&cred);
	}
}

static void
audit_record_dtor(struct kaudit_record *ar)
{
	if (ar->k_ar.ar_arg_upath1 != NULL) {
		free(ar->k_ar.ar_arg_upath1, M_AUDITPATH);
	}
	if (ar->k_ar.ar_arg_upath2 != NULL) {
		free(ar->k_ar.ar_arg_upath2, M_AUDITPATH);
	}
	if (ar->k_ar.ar_arg_kpath1 != NULL) {
		free(ar->k_ar.ar_arg_kpath1, M_AUDITPATH);
	}
	if (ar->k_ar.ar_arg_kpath2 != NULL) {
		free(ar->k_ar.ar_arg_kpath2, M_AUDITPATH);
	}
	if (ar->k_ar.ar_arg_text != NULL) {
		free(ar->k_ar.ar_arg_text, M_AUDITTEXT);
	}
	if (ar->k_ar.ar_arg_opaque != NULL) {
		free(ar->k_ar.ar_arg_opaque, M_AUDITDATA);
	}
	if (ar->k_ar.ar_arg_data != NULL) {
		free(ar->k_ar.ar_arg_data, M_AUDITDATA);
	}
	if (ar->k_udata != NULL) {
		free(ar->k_udata, M_AUDITDATA);
	}
	if (ar->k_ar.ar_arg_argv != NULL) {
		free(ar->k_ar.ar_arg_argv, M_AUDITTEXT);
	}
	if (ar->k_ar.ar_arg_envv != NULL) {
		free(ar->k_ar.ar_arg_envv, M_AUDITTEXT);
	}
	audit_identity_info_destruct(&ar->k_ar.ar_arg_identity);
}

  
                                                                   
                                                                            
                                                    
   
void
audit_init(void)
{
	audit_enabled = 0;
	audit_syscalls = 0;
	audit_kevent_mask = 0;
	audit_suspended = 0;
	audit_panic_on_write_fail = 0;
	audit_fail_stop = 0;
	audit_in_failure = 0;
	audit_argv = 0;
	audit_arge = 0;
	audit_ctl_mode = AUDIT_CTLMODE_NORMAL;
	audit_expire_after.age = 0;
	audit_expire_after.size = 0;
	audit_expire_after.op_type = AUDIT_EXPIRE_OP_AND;

	audit_fstat.af_filesz = 0;                                       
	audit_fstat.af_currsz = 0;
	audit_nae_mask.am_success = 0;
	audit_nae_mask.am_failure = 0;

	TAILQ_INIT(&audit_q);
	audit_q_len = 0;
	audit_pre_q_len = 0;
	audit_qctrl.aq_hiwater = AQ_HIWATER;
	audit_qctrl.aq_lowater = AQ_LOWATER;
	audit_qctrl.aq_bufsz = AQ_BUFSZ;
	audit_qctrl.aq_minfree = AU_FS_MINFREE;

	audit_kinfo.ai_termid.at_type = AU_IPv4;
	audit_kinfo.ai_termid.at_addr[0] = INADDR_ANY;

	mtx_init(&audit_mtx, "audit_mtx", NULL, MTX_DEF);
	KINFO_LOCK_INIT();
	cv_init(&audit_worker_cv, "audit_worker_cv");
	cv_init(&audit_drain_cv, "audit_drain_cv");
	cv_init(&audit_watermark_cv, "audit_watermark_cv");
	cv_init(&audit_fail_cv, "audit_fail_cv");

	                                   
	audit_session_init();

	                                         
	kau_init();

	                           

	                                
	(void) audit_pipe_init();

	                                
	audit_worker_init();
}

  
                                                                           
                                                                   
                                                            
   
void
audit_shutdown(void)
{
	audit_rotate_vnode(NULL, NULL);
}

  
                                                    
   
struct kaudit_record *
currecord(void)
{
	return curthread()->uu_ar;
}

  
                                                                         
                                                                          
                      
  
                                                                        
                                                               
   
struct kaudit_record *
audit_new(int event, proc_t p, __unused struct uthread *uthread)
{
	struct kaudit_record *ar;
	int no_record;
	int audit_override;

	  
                                                               
                          
   
                                                                  
                                                                    
                                 
    
	audit_override = (AUE_SESSION_START == event ||
	    AUE_SESSION_UPDATE == event || AUE_SESSION_END == event ||
	    AUE_SESSION_CLOSE == event);

	mtx_lock(&audit_mtx);
	no_record = (audit_suspended || !audit_enabled);
	mtx_unlock(&audit_mtx);
	if (!audit_override && no_record) {
		return NULL;
	}

	  
                                       
                                                      
   
                                                                
                                                                      
                  
    
	ar = zalloc(audit_record_zone);
	if (ar == NULL) {
		return NULL;
	}
	audit_record_ctor(p, ar);
	ar->k_ar.ar_event = event;

#if CONFIG_MACF
	if (PROC_NULL != p) {
		if (audit_mac_new(p, ar) != 0) {
			zfree(audit_record_zone, ar);
			return NULL;
		}
	} else {
		ar->k_ar.ar_mac_records = NULL;
	}
#endif

	mtx_lock(&audit_mtx);
	audit_pre_q_len++;
	mtx_unlock(&audit_mtx);

	return ar;
}

void
audit_free(struct kaudit_record *ar)
{
	audit_record_dtor(ar);
#if CONFIG_MACF
	if (NULL != ar->k_ar.ar_mac_records) {
		audit_mac_free(ar);
	}
#endif
	zfree(audit_record_zone, ar);
}

void
audit_commit(struct kaudit_record *ar, int error, int retval)
{
	au_event_t event;
	au_class_t class;
	au_id_t auid;
	int sorf;
	struct au_mask *aumask;
	int audit_override;

	if (ar == NULL) {
		return;
	}

	  
                                                                   
                                                                    
    
	if (ar->k_ar.ar_subj_auid == AU_DEFAUDITID) {
		aumask = &audit_nae_mask;
	} else {
		aumask = &ar->k_ar.ar_subj_amask;
	}

	if (error) {
		sorf = AU_PRS_FAILURE;
	} else {
		sorf = AU_PRS_SUCCESS;
	}

	switch (ar->k_ar.ar_event) {
	case AUE_OPEN_RWTC:
		  
                                                          
                                                             
                         
     
		ar->k_ar.ar_event = audit_flags_and_error_to_openevent(
			ar->k_ar.ar_arg_fflags, error);
		break;

	case AUE_OPEN_EXTENDED_RWTC:
		  
                                              
                                                                 
                                                  
     
		ar->k_ar.ar_event = audit_flags_and_error_to_openextendedevent(
			ar->k_ar.ar_arg_fflags, error);
		break;

	case AUE_OPENAT_RWTC:
		  
                                       
                                                           
                                                  
     
		ar->k_ar.ar_event = audit_flags_and_error_to_openatevent(
			ar->k_ar.ar_arg_fflags, error);
		break;

	case AUE_OPENBYID_RWT:
		  
                                         
                                                            
                                                  
     
		ar->k_ar.ar_event = audit_flags_and_error_to_openbyidevent(
			ar->k_ar.ar_arg_fflags, error);
		break;

	case AUE_SYSCTL:
		ar->k_ar.ar_event = audit_ctlname_to_sysctlevent(
			ar->k_ar.ar_arg_ctlname, ar->k_ar.ar_valid_arg);
		break;

	case AUE_AUDITON:
		                                                
		ar->k_ar.ar_event = auditon_command_event(ar->k_ar.ar_arg_cmd);
		break;

	case AUE_FCNTL:
		                                                        
		ar->k_ar.ar_event = audit_fcntl_command_event(
			ar->k_ar.ar_arg_cmd, ar->k_ar.ar_arg_fflags, error);
		break;
	}

	auid = ar->k_ar.ar_subj_auid;
	event = ar->k_ar.ar_event;
	class = au_event_class(event);

	  
                                                                  
          
   
                                                                 
                    
    
	audit_override = (AUE_SESSION_START == event ||
	    AUE_SESSION_UPDATE == event || AUE_SESSION_END == event ||
	    AUE_SESSION_CLOSE == event);

	ar->k_ar_commit |= AR_COMMIT_KERNEL;
	if (au_preselect(event, class, aumask, sorf) != 0) {
		ar->k_ar_commit |= AR_PRESELECT_TRAIL;
	}
	if (audit_pipe_preselect(auid, event, class, sorf,
	    ar->k_ar_commit & AR_PRESELECT_TRAIL) != 0) {
		ar->k_ar_commit |= AR_PRESELECT_PIPE;
	}
	if ((ar->k_ar_commit & (AR_PRESELECT_TRAIL | AR_PRESELECT_PIPE |
	    AR_PRESELECT_USER_TRAIL | AR_PRESELECT_USER_PIPE |
	    AR_PRESELECT_FILTER)) == 0) {
		mtx_lock(&audit_mtx);
		audit_pre_q_len--;
		mtx_unlock(&audit_mtx);
		audit_free(ar);
		return;
	}

	ar->k_ar.ar_errno = error;
	ar->k_ar.ar_retval = retval;
	nanotime(&ar->k_ar.ar_endtime);

	  
                                                                 
                                      
    
	mtx_lock(&audit_mtx);
	if (!audit_override && (audit_suspended || !audit_enabled)) {
		audit_pre_q_len--;
		mtx_unlock(&audit_mtx);
		audit_free(ar);
		return;
	}

	  
                                                                
                           
    
	while (audit_q_len >= audit_qctrl.aq_hiwater) {
		cv_wait(&audit_watermark_cv, &audit_mtx);
	}

	TAILQ_INSERT_TAIL(&audit_q, ar, k_q);
	audit_q_len++;
	audit_pre_q_len--;
	cv_signal(&audit_worker_cv);
	mtx_unlock(&audit_mtx);
}

  
                                                                       
                                                                            
                                                                             
                                      
   
void
audit_syscall_enter(unsigned int code, proc_t proc, struct uthread *uthread)
{
	struct au_mask *aumask;
	au_class_t class;
	au_event_t event;
	au_id_t auid;
	kauth_cred_t cred;

	  
                                                                 
                                                                      
                                                                 
                                                                 
                                                                   
                                                             
   
                                                                     
                                                                     
                                                       
    
	if (code >= nsysent) {
		return;
	}
	event = sys_au_event[code];
	if (event == AUE_NULL) {
		return;
	}

	KASSERT(uthread->uu_ar == NULL,
	    ("audit_syscall_enter: uthread->uu_ar != NULL"));

	  
                                                                     
                                         
    
	cred = kauth_cred_proc_ref(proc);
	auid = cred->cr_audit.as_aia_p->ai_auid;
	if (auid == AU_DEFAUDITID) {
		aumask = &audit_nae_mask;
	} else {
		aumask = &cred->cr_audit.as_mask;
	}

	  
                                                                     
                             
    
	class = au_event_class(event);
#if CONFIG_MACF
	  
                                                                     
                                        
    
	if (audit_mac_syscall_enter(code, proc, uthread, cred, event) == 0) {
		goto out;
	}
#endif
	if (au_preselect(event, class, aumask, AU_PRS_BOTH)) {
		  
                                                           
                                                           
                          
    
                                                           
                                                             
                                                        
                                                               
                                                     
                               
     
		if (audit_in_failure &&
		    suser(cred, &proc->p_acflag) != 0) {
			cv_wait(&audit_fail_cv, &audit_mtx);
			panic("audit_failing_stop: thread continued");
		}
		if (uthread->uu_ar == NULL) {
			uthread->uu_ar = audit_new(event, proc, uthread);
		}
	} else if (audit_pipe_preselect(auid, event, class, AU_PRS_BOTH, 0)) {
		if (uthread->uu_ar == NULL) {
			uthread->uu_ar = audit_new(event, proc, uthread);
		}
	}

	  
                                               
   
                                                                         
                                                                         
    
	AUDIT_ARG(identity);

out:
	kauth_cred_unref(&cred);
}

  
                                                                             
                                                                            
                                                                        
  
                                                                        
                                                                   
   
#if CONFIG_MACF
void
audit_syscall_exit(unsigned int code, int error, __unused proc_t proc,
    struct uthread *uthread)
#else
void
audit_syscall_exit(int error, __unsed proc_t proc, struct uthread *uthread)
#endif
{
	int retval;

	  
                                                                    
                                                                    
                                                                   
                                                                   
                                       
    
	if (error) {
		retval = -1;
	} else {
		retval = uthread->uu_rval[0];
	}

#if CONFIG_MACF
	if (audit_mac_syscall_exit(code, uthread, error, retval) != 0) {
		goto out;
	}
#endif
	audit_commit(uthread->uu_ar, error, retval);

out:
	uthread->uu_ar = NULL;
}

  
                                                                                 
                                                                             
                                                                         
                                
   
void
audit_subcall_enter(au_event_t event, proc_t proc, struct uthread *uthread)
{
	struct au_mask *aumask;
	au_class_t class;
	au_id_t auid;
	kauth_cred_t cred;

	  
                                                                     
                                         
    
	cred = kauth_cred_proc_ref(proc);
	auid = cred->cr_audit.as_aia_p->ai_auid;
	if (auid == AU_DEFAUDITID) {
		aumask = &audit_nae_mask;
	} else {
		aumask = &cred->cr_audit.as_mask;
	}

	  
                                                                     
                             
    
	class = au_event_class(event);

	if (au_preselect(event, class, aumask, AU_PRS_BOTH)) {
		  
                                                           
                                                           
                          
    
                                                           
                                                             
                                                        
                                                               
                                                     
                               
     
		if (audit_in_failure &&
		    suser(cred, &proc->p_acflag) != 0) {
			cv_wait(&audit_fail_cv, &audit_mtx);
			panic("audit_failing_stop: thread continued");
		}
		if (uthread->uu_ar == NULL) {
			uthread->uu_ar = audit_new(event, proc, uthread);
		}
	} else if (audit_pipe_preselect(auid, event, class, AU_PRS_BOTH, 0)) {
		if (uthread->uu_ar == NULL) {
			uthread->uu_ar = audit_new(event, proc, uthread);
		}
	}

	kauth_cred_unref(&cred);
}

void
audit_subcall_exit(int error, struct uthread *uthread)
{
	                                                           
	audit_commit(uthread->uu_ar, error, 0             );

	uthread->uu_ar = NULL;
}

  
                                                                         
         
   
void
audit_mach_syscall_enter(unsigned short event)
{
	struct uthread *uthread;
	proc_t proc;
	struct au_mask *aumask;
	kauth_cred_t cred;
	au_class_t class;
	au_id_t auid;

	if (event == AUE_NULL) {
		return;
	}

	uthread = curthread();
	if (uthread == NULL) {
		return;
	}

	proc = current_proc();
	if (proc == NULL) {
		return;
	}

	KASSERT(uthread->uu_ar == NULL,
	    ("audit_mach_syscall_enter: uthread->uu_ar != NULL"));

	cred = kauth_cred_proc_ref(proc);
	auid = cred->cr_audit.as_aia_p->ai_auid;

	  
                                                                     
                                         
    
	if (auid == AU_DEFAUDITID) {
		aumask = &audit_nae_mask;
	} else {
		aumask = &cred->cr_audit.as_mask;
	}

	  
                                                                     
                  
    
	class = au_event_class(event);
	if (au_preselect(event, class, aumask, AU_PRS_BOTH)) {
		uthread->uu_ar = audit_new(event, proc, uthread);
	} else if (audit_pipe_preselect(auid, event, class, AU_PRS_BOTH, 0)) {
		uthread->uu_ar = audit_new(event, proc, uthread);
	} else {
		uthread->uu_ar = NULL;
	}

	kauth_cred_unref(&cred);
}

void
audit_mach_syscall_exit(int retval, struct uthread *uthread)
{
	  
                                                            
                
    
	                                             
	audit_commit(uthread->uu_ar, retval, retval);
	uthread->uu_ar = NULL;
}

  
                                                               
                                                                       
                       
   
int
kau_will_audit(void)
{
	return audit_enabled && currecord() != NULL;
}

#if CONFIG_COREDUMP
void
audit_proc_coredump(proc_t proc, char *path, int errcode)
{
	struct kaudit_record *ar;
	struct au_mask *aumask;
	au_class_t class;
	int ret, sorf;
	char **pathp;
	au_id_t auid;
	kauth_cred_t my_cred;
	struct uthread *uthread;

	ret = 0;

	  
                                                         
    
	my_cred = kauth_cred_proc_ref(proc);
	auid = my_cred->cr_audit.as_aia_p->ai_auid;
	if (auid == AU_DEFAUDITID) {
		aumask = &audit_nae_mask;
	} else {
		aumask = &my_cred->cr_audit.as_mask;
	}
	kauth_cred_unref(&my_cred);
	  
                                                                     
                                                   
    
	if (errcode != 0) {
		sorf = AU_PRS_FAILURE;
	} else {
		sorf = AU_PRS_SUCCESS;
	}
	class = au_event_class(AUE_CORE);
	if (au_preselect(AUE_CORE, class, aumask, sorf) == 0 &&
	    audit_pipe_preselect(auid, AUE_CORE, class, sorf, 0) == 0) {
		return;
	}
	  
                                                                  
                                                                       
                    
    
	uthread = curthread();
	ar = audit_new(AUE_CORE, proc, uthread);
	if (ar == NULL) {
		return;
	}
	if (path != NULL) {
		pathp = &ar->k_ar.ar_arg_upath1;
		*pathp = malloc(MAXPATHLEN, M_AUDITPATH, M_WAITOK);
		if (audit_canon_path(vfs_context_cwd(vfs_context_current()), path,
		    *pathp)) {
			free(*pathp, M_AUDITPATH);
		} else {
			ARG_SET_VALID(ar, ARG_UPATH1);
		}
	}
	ar->k_ar.ar_arg_signum = proc->p_sigacts->ps_sig;
	ARG_SET_VALID(ar, ARG_SIGNUM);
	if (errcode != 0) {
		ret = 1;
	}
	audit_commit(ar, errcode, ret);
}
#endif                      
#endif                   
