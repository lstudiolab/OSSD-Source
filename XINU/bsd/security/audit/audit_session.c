/*-
 * Copyright (c) 2008-2009 Apple Inc.
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

#include <stdarg.h>

#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/kauth.h>
#include <sys/conf.h>
#include <sys/poll.h>
#include <sys/priv.h>
#include <sys/queue.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/systm.h>
#include <sys/ucred.h>
#include <sys/user.h>

#include <miscfs/devfs/devfs.h>

#include <libkern/OSAtomic.h>

#include <bsm/audit.h>
#include <bsm/audit_internal.h>
#include <bsm/audit_kevents.h>

#include <security/audit/audit.h>
#include <security/audit/audit_bsd.h>
#include <security/audit/audit_ioctl.h>
#include <security/audit/audit_private.h>

#include <vm/vm_protos.h>
#include <mach/mach_port.h>
#include <kern/audit_sessionport.h>

#include <libkern/OSDebug.h>

  
                                                                             
                                                                         
                               
   
struct au_sentry {
	auditinfo_addr_t        se_auinfo;                                      
#define se_asid         se_auinfo.ai_asid
#define se_auid         se_auinfo.ai_auid
#define se_mask         se_auinfo.ai_mask
#define se_termid       se_auinfo.ai_termid
#define se_flags        se_auinfo.ai_flags

	long                    se_refcnt;                            
	long                    se_procnt;                                 
	ipc_port_t              se_port;                           
	LIST_ENTRY(au_sentry)   se_link;                                       
};
typedef struct au_sentry au_sentry_t;

#define AU_SENTRY_PTR(aia_p)    ((au_sentry_t *)(aia_p))

  
                                                        
   

static au_sentry_t audit_default_se = {
	.se_auinfo = {
		.ai_auid = AU_DEFAUDITID,
		.ai_asid = AU_DEFAUDITSID,
		.ai_termid = { .at_type = AU_IPv4, },
	},
	.se_refcnt = 1,
	.se_procnt = 1,
};

struct auditinfo_addr * const audit_default_aia_p = &audit_default_se.se_auinfo;

                                    
#define IPC_OBJECT_COPYIN_FLAGS_ALLOW_IMMOVABLE_SEND 0x1
kern_return_t ipc_object_copyin(ipc_space_t, mach_port_name_t,
    mach_msg_type_name_t, ipc_port_t *, mach_port_context_t, mach_msg_guard_flags_t *, uint32_t);
void ipc_port_release_send(ipc_port_t);

#if CONFIG_AUDIT


  
                                            
   
#define HASH_TABLE_SIZE         97
#define HASH_ASID(asid)         (audit_session_hash(asid) % HASH_TABLE_SIZE)

static struct rwlock    se_entry_lck;                                           

LIST_HEAD(au_sentry_head, au_sentry);
static struct au_sentry_head *au_sentry_bucket = NULL;

#define AU_HISTORY_LOGGING 0
#if AU_HISTORY_LOGGING
typedef enum au_history_event {
	AU_HISTORY_EVENT_UNKNOWN = 0,
	AU_HISTORY_EVENT_REF     = 1,
	AU_HISTORY_EVENT_UNREF   = 2,
	AU_HISTORY_EVENT_BIRTH   = 3,
	AU_HISTORY_EVENT_DEATH   = 4,
	AU_HISTORY_EVENT_FIND    = 5
} au_history_event_t;

#define AU_HISTORY_MAX_STACK_DEPTH 8

struct au_history {
	struct au_sentry        *ptr;
	struct au_sentry         se;
	void                    *stack[AU_HISTORY_MAX_STACK_DEPTH];
	unsigned int             stack_depth;
	au_history_event_t       event;
};

static struct au_history *au_history;
static size_t             au_history_size = 65536;
static unsigned int       au_history_index;

static inline unsigned int
au_history_entries(void)
{
	if (au_history_index >= au_history_size) {
		return au_history_size;
	} else {
		return au_history_index;
	}
}

static inline void
au_history_record(au_sentry_t *se, au_history_event_t event)
{
	struct au_history *p;
	unsigned int i;

	i = OSAddAtomic(1, &au_history_index);
	p = &au_history[i % au_history_size];

	bzero(p, sizeof(*p));
	p->event = event;
	bcopy(se, &p->se, sizeof(p->se));
	p->stack_depth = OSBacktrace(&p->stack[0], AU_HISTORY_MAX_STACK_DEPTH);
	p->ptr = se;
}
#else
#define au_history_record(se, event) do {} while (0)
#endif

MALLOC_DEFINE(M_AU_SESSION, "audit_session", "Audit session data");

static void     audit_ref_session(au_sentry_t *se);
static void     audit_unref_session(au_sentry_t *se);

static void     audit_session_event(int event, auditinfo_addr_t *aia_p);

  
                        
   

static MALLOC_DEFINE(M_AUDIT_SDEV, "audit_sdev", "Audit sdevs");
static MALLOC_DEFINE(M_AUDIT_SDEV_ENTRY, "audit_sdevent",
    "Audit sdev entries and buffers");

  
                                        
   
#define AUDIT_SDEV_QLIMIT_DEFAULT       128
#define AUDIT_SDEV_QLIMIT_MIN           1
#define AUDIT_SDEV_QLIMIT_MAX           1024

  
                   
   
struct  audit_sdev_entry {
	void                            *ase_record;
	u_int                            ase_record_len;
	TAILQ_ENTRY(audit_sdev_entry)    ase_queue;
};

  
                            
   

struct audit_sdev {
	int             asdev_open;

#define AUDIT_SDEV_ASYNC        0x00000001
#define AUDIT_SDEV_NBIO         0x00000002

#define AUDIT_SDEV_ALLSESSIONS  0x00010000
	u_int           asdev_flags;

	struct selinfo  asdev_selinfo;
	pid_t           asdev_sigio;

	au_id_t         asdev_auid;
	au_asid_t       asdev_asid;

	                                                    
	struct mtx      asdev_mtx;

	  
                                                            
                                                             
                                                             
                                                           
    
	struct slck     asdev_sx;

	  
                                                                
           
    
	struct cv       asdev_cv;

	                                              
	u_int           asdev_qlen;
	u_int           asdev_qlimit;

	                                                     
	u_int           asdev_qbyteslen;

	  
                                                            
                                                              
                         
    
	u_int           asdev_qoffset;

	  
                                  
    
	u_int64_t       asdev_inserts;                      
	u_int64_t       asdev_reads;                       
	u_int64_t       asdev_drops;                          

	  
                                                        
                                                          
                                                              
                                                              
                                     
    
	TAILQ_HEAD(, audit_sdev_entry)  asdev_queue;

	                       
	TAILQ_ENTRY(audit_sdev)         asdev_list;
};

#define AUDIT_SDEV_LOCK(asdev)          mtx_lock(&(asdev)->asdev_mtx)
#define AUDIT_SDEV_LOCK_ASSERT(asdev)   mtx_assert(&(asdev)->asdev_mtx, \
	                                    MA_OWNED)
#define AUDIT_SDEV_LOCK_DESTROY(asdev)  mtx_destroy(&(asdev)->asdev_mtx)
#define AUDIT_SDEV_LOCK_INIT(asdev)     mtx_init(&(asdev)->asdev_mtx, \
	                                    "audit_sdev_mtx", NULL, MTX_DEF)
#define AUDIT_SDEV_UNLOCK(asdev)        mtx_unlock(&(asdev)->asdev_mtx)
#define AUDIT_SDEV_MTX(asdev)           (&(asdev)->asdev_mtx)

#define AUDIT_SDEV_SX_LOCK_DESTROY(asd) slck_destroy(&(asd)->asdev_sx)
#define AUDIT_SDEV_SX_LOCK_INIT(asd)    slck_init(&(asd)->asdev_sx, \
	                                    "audit_sdev_sx")
#define AUDIT_SDEV_SX_XLOCK_ASSERT(asd) slck_assert(&(asd)->asdev_sx, \
	                                    SA_XLOCKED)
#define AUDIT_SDEV_SX_XLOCK_SIG(asd)    slck_lock_sig(&(asd)->asdev_sx)
#define AUDIT_SDEV_SX_XUNLOCK(asd)      slck_unlock(&(asd)->asdev_sx)

  
                                   
   
#define AUDIT_SDEV_NAME         "auditsessions"
#define MAX_AUDIT_SDEVS         32

static int audit_sdev_major;
static void *devnode;

  
                                                                   
                                                                    
                                                                   
                                              
   
static TAILQ_HEAD(, audit_sdev) audit_sdev_list;
static struct rwlock            audit_sdev_lock;

#define AUDIT_SDEV_LIST_LOCK_INIT()     rw_init(&audit_sdev_lock, \
	                                    "audit_sdev_list_lock")
#define AUDIT_SDEV_LIST_RLOCK()         rw_rlock(&audit_sdev_lock)
#define AUDIT_SDEV_LIST_RUNLOCK()       rw_runlock(&audit_sdev_lock)
#define AUDIT_SDEV_LIST_WLOCK()         rw_wlock(&audit_sdev_lock)
#define AUDIT_SDEV_LIST_WLOCK_ASSERT()  rw_assert(&audit_sdev_lock, \
	                                    RA_WLOCKED)
#define AUDIT_SDEV_LIST_WUNLOCK()       rw_wunlock(&audit_sdev_lock)

  
                                                                            
                                                                    
  
                                                    
   
static struct audit_sdev        *audit_sdev_dtab[MAX_AUDIT_SDEVS];

  
                                         
   
static open_close_fcn_t         audit_sdev_open;
static open_close_fcn_t         audit_sdev_close;
static read_write_fcn_t         audit_sdev_read;
static ioctl_fcn_t              audit_sdev_ioctl;
static select_fcn_t             audit_sdev_poll;

static const struct cdevsw audit_sdev_cdevsw = {
	.d_open      =          audit_sdev_open,
	.d_close     =          audit_sdev_close,
	.d_read      =          audit_sdev_read,
	.d_write     =          eno_rdwrt,
	.d_ioctl     =          audit_sdev_ioctl,
	.d_stop      =          eno_stop,
	.d_reset     =          eno_reset,
	.d_ttys      =          NULL,
	.d_select    =          audit_sdev_poll,
	.d_mmap      =          eno_mmap,
	.d_strategy  =          eno_strat,
	.d_type      =          0
};

  
                                    
   
static int              audit_sdev_count;                                     
static u_int64_t        audit_sdev_ever;                                   
static u_int64_t        audit_sdev_records;                              
static u_int64_t        audit_sdev_drops;                                      

static int audit_sdev_init(void);

#define AUDIT_SENTRY_RWLOCK_INIT()      rw_init(&se_entry_lck, \
	                                    "se_entry_lck")
#define AUDIT_SENTRY_RLOCK()            rw_rlock(&se_entry_lck)
#define AUDIT_SENTRY_WLOCK()            rw_wlock(&se_entry_lck)
#define AUDIT_SENTRY_RWLOCK_ASSERT()    rw_assert(&se_entry_lck, RA_LOCKED)
#define AUDIT_SENTRY_RUNLOCK()          rw_runlock(&se_entry_lck)
#define AUDIT_SENTRY_WUNLOCK()          rw_wunlock(&se_entry_lck)

                                                           
static uint64_t audit_session_superuser_set_sflags_mask;
static uint64_t audit_session_superuser_clear_sflags_mask;
static uint64_t audit_session_member_set_sflags_mask;
static uint64_t audit_session_member_clear_sflags_mask;
SYSCTL_NODE(, OID_AUTO, audit, CTLFLAG_RW | CTLFLAG_LOCKED, 0, "Audit controls");
SYSCTL_NODE(_audit, OID_AUTO, session, CTLFLAG_RW | CTLFLAG_LOCKED, 0, "Audit sessions");
SYSCTL_QUAD(_audit_session, OID_AUTO, superuser_set_sflags_mask, CTLFLAG_RW | CTLFLAG_LOCKED,
    &audit_session_superuser_set_sflags_mask,
    "Audit session flags settable by superuser");
SYSCTL_QUAD(_audit_session, OID_AUTO, superuser_clear_sflags_mask, CTLFLAG_RW | CTLFLAG_LOCKED,
    &audit_session_superuser_clear_sflags_mask,
    "Audit session flags clearable by superuser");
SYSCTL_QUAD(_audit_session, OID_AUTO, member_set_sflags_mask, CTLFLAG_RW | CTLFLAG_LOCKED,
    &audit_session_member_set_sflags_mask,
    "Audit session flags settable by a session member");
SYSCTL_QUAD(_audit_session, OID_AUTO, member_clear_sflags_mask, CTLFLAG_RW | CTLFLAG_LOCKED,
    &audit_session_member_clear_sflags_mask,
    "Audit session flags clearable by a session member");

extern int set_security_token_task_internal(proc_t p, void *task);

#define AUDIT_SESSION_DEBUG     0
#if     AUDIT_SESSION_DEBUG
  
                                                                            
                                                                          
  
                                                                            
                         
   
#include <kern/kalloc.h>

  
                                                          
   
struct au_sentry_debug {
	auditinfo_addr_t        se_auinfo;
	int64_t                 se_refcnt;                            
	int64_t                 se_procnt;                         
	int64_t                 se_ptcnt;                            
                                                          
};
typedef struct au_sentry_debug au_sentry_debug_t;

static int audit_sysctl_session_debug(struct sysctl_oid *oidp, void *arg1,
    int arg2, struct sysctl_req *req);

SYSCTL_PROC(_kern, OID_AUTO, audit_session_debug, CTLFLAG_RD | CTLFLAG_LOCKED,
    NULL, 0, audit_sysctl_session_debug, "S,audit_session_debug",
    "Current session debug info for auditing.");

  
                                                                            
                                                                         
                                                                            
                      
   
static int
audit_session_debug_callout(__unused proc_t p, __unused void *arg)
{
	return PROC_RETURNED_DONE;
}

static int
audit_session_debug_filterfn(proc_t p, void *st)
{
	kauth_cred_t cred = kauth_cred_get();
	auditinfo_addr_t *aia_p = cred->cr_audit.as_aia_p;
	au_sentry_debug_t *sed_tab = (au_sentry_debug_t *) st;
	au_sentry_debug_t  *sdtp;
	au_sentry_t *se;

	if (IS_VALID_SESSION(aia_p)) {
		sdtp = &sed_tab[0];
		do {
			if (aia_p->ai_asid == sdtp->se_asid) {
				sdtp->se_ptcnt++;

				                           
				se = AU_SENTRY_PTR(aia_p);
				if (se->se_refcnt != sdtp->se_refcnt) {
					sdtp->se_refcnt =
					    (int64_t)se->se_refcnt;
				}
				if (se->se_procnt != sdtp->se_procnt) {
					sdtp->se_procnt =
					    (int64_t)se->se_procnt;
				}
				break;
			}
			sdtp++;
		} while (sdtp->se_asid != 0 && sdtp->se_auid != 0);
	} else {
		                                   
		sed_tab->se_ptcnt++;
	}

	return 0;
}

  
                                                            
  
   
static int
audit_sysctl_session_debug(__unused struct sysctl_oid *oidp,
    __unused void *arg1, __unused int arg2, struct sysctl_req *req)
{
	au_sentry_t *se;
	au_sentry_debug_t *sed_tab, *next_sed;
	int i, entry_cnt = 0;
	size_t sz;
	int err = 0;

	  
                                   
    
	if (req->newptr != USER_ADDR_NULL) {
		return EPERM;
	}

	  
                                                            
    
	AUDIT_SENTRY_RLOCK();
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		LIST_FOREACH(se, &au_sentry_bucket[i], se_link)
		if (se != NULL) {
			entry_cnt++;
		}
	}

	entry_cnt++;                                     
	  
                                                                 
                                                                  
                                  
    
	if (req->oldptr == USER_ADDR_NULL) {
		req->oldidx = (entry_cnt + 3) * sizeof(au_sentry_debug_t);
		AUDIT_SENTRY_RUNLOCK();
		return 0;
	}

	  
                             
    
	if (req->oldlen < (entry_cnt * sizeof(au_sentry_debug_t))) {
		AUDIT_SENTRY_RUNLOCK();
		return ENOMEM;
	}
	  
                                                                    
                                                                      
    
	sed_tab = (au_sentry_debug_t *)kheap_alloc(KHEAP_TEMP,
	    entry_cnt * sizeof(au_sentry_debug_t), Z_NOWAIT | Z_ZERO);
	if (sed_tab == NULL) {
		AUDIT_SENTRY_RUNLOCK();
		return ENOMEM;
	}

	  
                                                                 
    
	sz = 0;
	next_sed = sed_tab;
	                                                                
	bcopy(audit_default_aia_p, &next_sed->se_auinfo, sizeof(au_sentry_t));
	next_sed->se_refcnt = (int64_t)audit_default_se.se_refcnt;
	next_sed->se_procnt = (int64_t)audit_default_se.se_procnt;
	next_sed++;
	sz += sizeof(au_sentry_debug_t);
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		LIST_FOREACH(se, &au_sentry_bucket[i], se_link) {
			if (se != NULL) {
				next_sed->se_auinfo = se->se_auinfo;
				next_sed->se_refcnt = (int64_t)se->se_refcnt;
				next_sed->se_procnt = (int64_t)se->se_procnt;
				next_sed++;
				sz += sizeof(au_sentry_debug_t);
			}
		}
	}
	AUDIT_SENTRY_RUNLOCK();

	                                       
	proc_iterate(PROC_ALLPROCLIST | PROC_ZOMBPROCLIST,
	    audit_session_debug_callout, NULL,
	    audit_session_debug_filterfn, (void *)&sed_tab[0]);


	req->oldlen = sz;
	err = SYSCTL_OUT(req, sed_tab, sz);
	kheap_free(KHEAP_TEMP, sed_tab, entry_cnt * sizeof(au_sentry_debug_t));

	return err;
}

#endif                          

  
                                                                              
                                                                  
   
static void
audit_session_event(int event, auditinfo_addr_t *aia_p)
{
	struct kaudit_record *ar;

	KASSERT(AUE_SESSION_START == event || AUE_SESSION_UPDATE == event ||
	    AUE_SESSION_END == event || AUE_SESSION_CLOSE == event,
	    ("audit_session_event: invalid event: %d", event));

	if (NULL == aia_p) {
		return;
	}

	  
                                                                   
                                                           
                                    
    
	ar = audit_new(event, PROC_NULL,                NULL);
	if (NULL == ar) {
		return;
	}

	  
                                                                   
                                                              
    
	ar->k_ar_commit |= AR_PRESELECT_FILTER;

	  
                                                                
                                                                        
                
    
	ar->k_ar.ar_subj_ruid = 0;
	ar->k_ar.ar_subj_rgid = 0;
	ar->k_ar.ar_subj_egid = 0;
	ar->k_ar.ar_subj_pid = 0;
	ar->k_ar.ar_subj_auid = aia_p->ai_auid;
	ar->k_ar.ar_subj_asid = aia_p->ai_asid;
	bcopy(&aia_p->ai_termid, &ar->k_ar.ar_subj_term_addr,
	    sizeof(struct au_tid_addr));

	                                        
	ar->k_ar.ar_arg_amask.am_success = aia_p->ai_mask.am_success;
	ar->k_ar.ar_arg_amask.am_failure = aia_p->ai_mask.am_failure;
	ARG_SET_VALID(ar, ARG_AMASK);

	                                                
	ar->k_ar.ar_arg_value64 = aia_p->ai_flags;
	ARG_SET_VALID(ar, ARG_VALUE64);


	                                     
	audit_commit(ar, 0, 0);
}

  
                                                       
   
static inline uint32_t
audit_session_hash(au_asid_t asid)
{
	uint32_t a = (uint32_t) asid;

	a = (a - (a << 6)) ^ (a >> 17);
	a = (a - (a << 9)) ^ (a << 4);
	a = (a - (a << 3)) ^ (a << 10);
	a = a ^ (a >> 15);

	return a;
}

  
                                                                              
                                                                        
             
   
static au_sentry_t *
audit_session_find(au_asid_t asid)
{
	uint32_t         hkey;
	au_sentry_t     *found_se;

	AUDIT_SENTRY_RWLOCK_ASSERT();

	hkey = HASH_ASID(asid);

	LIST_FOREACH(found_se, &au_sentry_bucket[hkey], se_link)
	if (found_se->se_asid == asid) {
		au_history_record(found_se, AU_HISTORY_EVENT_FIND);
		audit_ref_session(found_se);
		return found_se;
	}
	return NULL;
}

  
                                                            
   
static void
audit_session_remove(au_sentry_t *se)
{
	uint32_t         hkey;
	au_sentry_t     *found_se, *tmp_se;

	au_history_record(se, AU_HISTORY_EVENT_DEATH);
	KASSERT(se->se_refcnt == 0, ("audit_session_remove: ref count != 0"));
	KASSERT(se != &audit_default_se,
	    ("audit_session_remove: removing default session"));

	hkey = HASH_ASID(se->se_asid);

	AUDIT_SENTRY_WLOCK();
	  
                                                                    
    
	if (se->se_refcnt != 0) {
		AUDIT_SENTRY_WUNLOCK();
		return;
	}

	audit_session_portdestroy(&se->se_port);
	LIST_FOREACH_SAFE(found_se, &au_sentry_bucket[hkey], se_link, tmp_se) {
		if (found_se == se) {
			  
                                                       
                    
      
			audit_session_event(AUE_SESSION_CLOSE,
			    &found_se->se_auinfo);

			LIST_REMOVE(found_se, se_link);
			AUDIT_SENTRY_WUNLOCK();
			free(found_se, M_AU_SESSION);

			return;
		}
	}
	AUDIT_SENTRY_WUNLOCK();
}

  
                                                              
   
static void
audit_ref_session(au_sentry_t *se)
{
	long old_val;

	if (se == NULL || se == &audit_default_se) {
		return;
	}

	au_history_record(se, AU_HISTORY_EVENT_REF);

	old_val = OSAddAtomicLong(1, &se->se_refcnt);
	KASSERT(old_val < 100000,
	    ("audit_ref_session: Too many references on session."));
}

  
                                                                           
   
static void
audit_unref_session(au_sentry_t *se)
{
	long old_val;

	if (se == NULL || se == &audit_default_se) {
		return;
	}

	au_history_record(se, AU_HISTORY_EVENT_UNREF);

	old_val = OSAddAtomicLong(-1, &se->se_refcnt);
	if (old_val == 1) {
		audit_session_remove(se);
	}
	KASSERT(old_val > 0,
	    ("audit_unref_session: Too few references on session."));
}

  
                                              
   
static void
audit_inc_procount(au_sentry_t *se)
{
	long old_val;

	if (se == NULL || se == &audit_default_se) {
		return;
	}

	old_val = OSAddAtomicLong(1, &se->se_procnt);
	KASSERT(old_val <= PID_MAX,
	    ("audit_inc_procount: proc count > PID_MAX"));
}

  
                                                                        
                       
   
static void
audit_dec_procount(au_sentry_t *se)
{
	long old_val;

	if (se == NULL || se == &audit_default_se) {
		return;
	}

	old_val = OSAddAtomicLong(-1, &se->se_procnt);
	  
                                                                  
                                   
    
	if (old_val == 1) {
		audit_session_event(AUE_SESSION_END, &se->se_auinfo);
	}
	KASSERT(old_val >= 1,
	    ("audit_dec_procount: proc count < 0"));
}

  
                                                                     
           
                                                                               
                                 
   
static int
audit_update_sentry(au_sentry_t *se, auditinfo_addr_t *new_aia)
{
	auditinfo_addr_t *aia = &se->se_auinfo;
	int update;

	KASSERT(new_aia != audit_default_aia_p,
	    ("audit_update_sentry: Trying to update the default aia."));

	update = (aia->ai_auid != new_aia->ai_auid ||
	    bcmp(&aia->ai_termid, &new_aia->ai_termid,
	    sizeof(new_aia->ai_termid)) ||
	    aia->ai_flags != new_aia->ai_flags);

	if (update) {
		bcopy(new_aia, aia, sizeof(*aia));
	}

	return update;
}

  
                                                                               
                                             
   
static uint32_t
audit_session_nextid(void)
{
	static uint32_t next_asid = ASSIGNED_ASID_MIN;

	AUDIT_SENTRY_RWLOCK_ASSERT();

	if (next_asid > ASSIGNED_ASID_MAX) {
		next_asid = ASSIGNED_ASID_MIN;
	}

	return next_asid++;
}

  
                                                                            
                                                                            
                                                                            
                                                
   
static auditinfo_addr_t *
audit_session_new(auditinfo_addr_t *new_aia_p, auditinfo_addr_t *old_aia_p)
{
	au_asid_t new_asid;
	au_sentry_t *se = NULL;
	au_sentry_t *found_se = NULL;
	auditinfo_addr_t *aia = NULL;

	KASSERT(new_aia_p != NULL, ("audit_session_new: new_aia_p == NULL"));

	new_asid = new_aia_p->ai_asid;

	  
                                                                    
    
	se = malloc(sizeof(au_sentry_t), M_AU_SESSION, M_WAITOK | M_ZERO);

	  
                                          
    
	AUDIT_SENTRY_WLOCK();
	if (new_asid == AU_ASSIGN_ASID) {
		do {
			new_asid = (au_asid_t)audit_session_nextid();
			found_se = audit_session_find(new_asid);

			  
                                                         
                              
      
			if (found_se != NULL) {
				audit_unref_session(found_se);
			} else {
				break;
			}
		} while (1);
	} else {
		  
                                                         
                                                          
     
		if ((found_se = audit_session_find(new_asid)) != NULL) {
			int updated;

			updated = audit_update_sentry(found_se, new_aia_p);

			AUDIT_SENTRY_WUNLOCK();
			free(se, M_AU_SESSION);

			                                                      
			if (new_aia_p != old_aia_p) {
				audit_inc_procount(found_se);
			}

			  
                                                 
                                                 
      
			if (updated) {
				audit_session_event(AUE_SESSION_UPDATE,
				    &found_se->se_auinfo);
			}

			return &found_se->se_auinfo;
		}
	}

	  
                                                                      
                                                       
    
	se->se_refcnt = se->se_procnt = 1;

	  
                                                                       
                                          
    
	se->se_port = IPC_PORT_NULL;
	aia = &se->se_auinfo;
	aia->ai_asid = new_asid;
	aia->ai_auid = new_aia_p->ai_auid;
	bzero(&new_aia_p->ai_mask, sizeof(new_aia_p->ai_mask));
	bcopy(&new_aia_p->ai_termid, &aia->ai_termid, sizeof(aia->ai_termid));
	aia->ai_flags = new_aia_p->ai_flags;

	  
                             
    
	LIST_INSERT_HEAD(&au_sentry_bucket[HASH_ASID(new_asid)], se, se_link);
	AUDIT_SENTRY_WUNLOCK();

	  
                                                                  
    
	audit_session_event(AUE_SESSION_START, aia);
	au_history_record(se, AU_HISTORY_EVENT_BIRTH);
	return aia;
}

  
                                                                            
                                                      
   
int
audit_session_lookup(au_asid_t asid, auditinfo_addr_t *ret_aia)
{
	au_sentry_t *se = NULL;

	if ((uint32_t)asid > ASSIGNED_ASID_MAX) {
		return -1;
	}
	AUDIT_SENTRY_RLOCK();
	if ((se = audit_session_find(asid)) == NULL) {
		AUDIT_SENTRY_RUNLOCK();
		return 1;
	}
	                                                                        
	AUDIT_SENTRY_RUNLOCK();
	if (ret_aia != NULL) {
		bcopy(&se->se_auinfo, ret_aia, sizeof(*ret_aia));
	}
	audit_unref_session(se);

	return 0;
}

void
audit_session_aiaref(auditinfo_addr_t *aia_p)
{
	audit_ref_session(AU_SENTRY_PTR(aia_p));
}

  
                                        
   
void
audit_session_ref(kauth_cred_t cred)
{
	auditinfo_addr_t *aia_p;

	KASSERT(IS_VALID_CRED(cred),
	    ("audit_session_ref: Invalid kauth_cred."));

	aia_p = cred->cr_audit.as_aia_p;
	audit_session_aiaref(aia_p);
}

void
audit_session_aiaunref(auditinfo_addr_t *aia_p)
{
	audit_unref_session(AU_SENTRY_PTR(aia_p));
}

  
                                           
   
void
audit_session_unref(kauth_cred_t cred)
{
	auditinfo_addr_t *aia_p;

	KASSERT(IS_VALID_CRED(cred),
	    ("audit_session_unref: Invalid kauth_cred."));

	aia_p = cred->cr_audit.as_aia_p;
	audit_session_aiaunref(aia_p);
}

  
                                                                              
                                    
   
void
audit_session_procnew(proc_t p)
{
	kauth_cred_t cred = p->p_ucred;
	auditinfo_addr_t *aia_p;

	KASSERT(IS_VALID_CRED(cred),
	    ("audit_session_procnew: Invalid kauth_cred."));

	aia_p = cred->cr_audit.as_aia_p;

	audit_inc_procount(AU_SENTRY_PTR(aia_p));
}

  
                                                                              
                           
   
void
audit_session_procexit(proc_t p)
{
	kauth_cred_t cred = p->p_ucred;
	auditinfo_addr_t *aia_p;

	KASSERT(IS_VALID_CRED(cred),
	    ("audit_session_procexit: Invalid kauth_cred."));

	aia_p = cred->cr_audit.as_aia_p;

	audit_dec_procount(AU_SENTRY_PTR(aia_p));
}

  
                               
   
void
audit_session_init(void)
{
	int i;

	KASSERT((ASSIGNED_ASID_MAX - ASSIGNED_ASID_MIN) > PID_MAX,
	    ("audit_session_init: ASSIGNED_ASID_MAX is not large enough."));

	AUDIT_SENTRY_RWLOCK_INIT();

	au_sentry_bucket = malloc( sizeof(struct au_sentry) *
	    HASH_TABLE_SIZE, M_AU_SESSION, M_WAITOK | M_ZERO);

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		LIST_INIT(&au_sentry_bucket[i]);
	}

	(void)audit_sdev_init();
#if AU_HISTORY_LOGGING
	au_history = malloc(sizeof(struct au_history) * au_history_size,
	    M_AU_SESSION, M_WAITOK | M_ZERO);
#endif
}

static int
audit_session_update_check(kauth_cred_t cred, auditinfo_addr_t *old,
    auditinfo_addr_t *new)
{
	uint64_t n;

	                                                                      
	if (old->ai_auid != AU_DEFAUDITID && old->ai_auid != new->ai_auid) {
		return EINVAL;
	}

	                                                                    
	if ((old->ai_termid.at_type != AU_IPv4 ||
	    old->ai_termid.at_port != 0 ||
	    old->ai_termid.at_addr[0] != 0) &&
	    (old->ai_termid.at_port != new->ai_termid.at_port ||
	    old->ai_termid.at_type != new->ai_termid.at_type ||
	    0 != bcmp(&old->ai_termid.at_addr, &new->ai_termid.at_addr,
	    sizeof(old->ai_termid.at_addr)))) {
		return EINVAL;
	}

	                                             
                                     
    
	n = ~old->ai_flags & new->ai_flags;
	if (0 != n &&
	    !((n == (audit_session_superuser_set_sflags_mask & n) &&
	    kauth_cred_issuser(cred)) ||
	    (n == (audit_session_member_set_sflags_mask & n) &&
	    old->ai_asid == new->ai_asid))) {
		return EINVAL;
	}

	                                                 
                                       
    
	n = ~new->ai_flags & old->ai_flags;
	if (0 != n &&
	    !((n == (audit_session_superuser_clear_sflags_mask & n) &&
	    kauth_cred_issuser(cred)) ||
	    (n == (audit_session_member_clear_sflags_mask & n) &&
	    old->ai_asid == new->ai_asid))) {
		return EINVAL;
	}

	                                  
	return 0;
}

  
                                                                               
   
int
audit_session_setaia(proc_t p, auditinfo_addr_t *new_aia_p)
{
	kauth_cred_t my_cred, my_new_cred;
	struct au_session  as;
	struct au_session  tmp_as;
	auditinfo_addr_t caia, *old_aia_p;
	int ret;

	  
                                                               
                     
    
	if (audit_session_lookup(new_aia_p->ai_asid, &caia) == 0) {
		my_cred = kauth_cred_proc_ref(p);
		ret = audit_session_update_check(my_cred, &caia, new_aia_p);
		kauth_cred_unref(&my_cred);
		if (ret) {
			return ret;
		}
	}

	my_cred = kauth_cred_proc_ref(p);
	bcopy(&new_aia_p->ai_mask, &as.as_mask, sizeof(as.as_mask));
	old_aia_p = my_cred->cr_audit.as_aia_p;
	                                                         
	as.as_aia_p = audit_session_new(new_aia_p, old_aia_p);

	                                                                  
	if (old_aia_p != new_aia_p) {
		audit_dec_procount(AU_SENTRY_PTR(old_aia_p));
	}


	  
                                                                    
                                                                        
                                                                        
                                                                      
                                                                     
    
	for (;;) {
		  
                                                              
                                                              
                                                         
                                                              
                                                      
     
		bcopy(&as, &tmp_as, sizeof(tmp_as));
		my_new_cred = kauth_cred_setauditinfo(my_cred, &tmp_as);

		if (my_cred != my_new_cred) {
			proc_ucred_lock(p);
			                                                       
                                                         
                                                        
                              
      
			if (p->p_ucred != my_cred) {
				proc_ucred_unlock(p);
				audit_session_unref(my_new_cred);
				kauth_cred_unref(&my_new_cred);
				               
				my_cred = kauth_cred_proc_ref(p);
				continue;
			}
			p->p_ucred = my_new_cred;
			                         
			PROC_UPDATE_CREDS_ONPROC(p);
			proc_ucred_unlock(p);
		}
		  
                                                    
     
		kauth_cred_unref(&my_cred);
		break;
	}

	                                                            
	audit_unref_session(AU_SENTRY_PTR(as.as_aia_p));

	                                                             
	set_security_token(p);

	return 0;
}

  
                                    
  
                                                                 
  
                                                       
  
                                                         
                                                                   
  
                    
                                                                    
                                                                     
                                                                     
   
int
audit_session_self(proc_t p, __unused struct audit_session_self_args *uap,
    mach_port_name_t *ret_port)
{
	ipc_port_t sendport = IPC_PORT_NULL;
	kauth_cred_t cred = NULL;
	auditinfo_addr_t *aia_p;
	au_sentry_t *se;
	int err = 0;

	cred = kauth_cred_proc_ref(p);
	if (!IS_VALID_CRED(cred)) {
		err = ESRCH;
		goto done;
	}

	aia_p = cred->cr_audit.as_aia_p;
	if (!IS_VALID_SESSION(aia_p)) {
		                                     
		err = EINVAL;
		goto done;
	}

	se = AU_SENTRY_PTR(aia_p);

	  
                                                                       
                        
    
	if (se->se_port == IPC_PORT_NULL) {
		bcopy(&cred->cr_audit.as_mask, &se->se_mask,
		    sizeof(se->se_mask));
	}

	  
                                                                    
                                 
    
	sendport = audit_session_mksend(aia_p, &se->se_port);
	*ret_port = ipc_port_copyout_send(sendport, get_task_ipcspace(p->task));

done:
	if (cred != NULL) {
		kauth_cred_unref(&cred);
	}
	if (err != 0) {
		*ret_port = MACH_PORT_NULL;
	}
	return err;
}

  
                                    
  
                                                                  
  
                                                       
                                                                         
                                                                            
                                            
                                                                         
  
                      
                                                                    
                                                                
                                                                    
                                                                     
                                                                      
                                                       
                                                                     
   
int
audit_session_port(proc_t p, struct audit_session_port_args *uap,
    __unused int *retval)
{
	ipc_port_t sendport = IPC_PORT_NULL;
	mach_port_name_t portname = MACH_PORT_NULL;
	kauth_cred_t cred = NULL;
	auditinfo_addr_t *aia_p = NULL;
	au_sentry_t *se = NULL;
	int err = 0;

	                                                        
                                                  
    
	if (uap->asid != -1 && (uint32_t)uap->asid > ASSIGNED_ASID_MAX) {
		err = EINVAL;
		goto done;
	}
	cred = kauth_cred_proc_ref(p);
	if (!IS_VALID_CRED(cred)) {
		err = ESRCH;
		goto done;
	}
	aia_p = cred->cr_audit.as_aia_p;

	                                                        
                                                         
                                                            
    
	if (uap->asid == (au_asid_t)-1 ||
	    uap->asid == aia_p->ai_asid) {
		if (!IS_VALID_SESSION(aia_p)) {
			                                     
			err = EINVAL;
			goto done;
		}

		                                                    
                 
     
		se = AU_SENTRY_PTR(aia_p);
		audit_ref_session(se);
	} else {
		  
                                                    
                          
     
		err = priv_check_cred(cred, PRIV_AUDIT_SESSION_PORT, 0);
		if (err != 0) {
			goto done;
		}
		AUDIT_SENTRY_RLOCK();
		se = audit_session_find(uap->asid);
		AUDIT_SENTRY_RUNLOCK();
		if (NULL == se) {
			err = EINVAL;
			goto done;
		}
		aia_p = &se->se_auinfo;
	}

	  
                                                                       
                        
    
	if (se->se_port == IPC_PORT_NULL) {
		bcopy(&cred->cr_audit.as_mask, &se->se_mask,
		    sizeof(se->se_mask));
	}

	  
                                                                     
                                                                      
                                                                        
    
	sendport = audit_session_mksend(aia_p, &se->se_port);
	portname = ipc_port_copyout_send(sendport, get_task_ipcspace(p->task));
	if (!MACH_PORT_VALID(portname)) {
		err = EINVAL;
		goto done;
	}
	err = copyout(&portname, uap->portnamep, sizeof(mach_port_name_t));
done:
	if (cred != NULL) {
		kauth_cred_unref(&cred);
	}
	if (NULL != se) {
		audit_unref_session(se);
	}
	if (MACH_PORT_VALID(portname) && 0 != err) {
		(void)mach_port_deallocate(get_task_ipcspace(p->task),
		    portname);
	}

	return err;
}

static int
audit_session_join_internal(proc_t p, task_t task, ipc_port_t port, au_asid_t *new_asid)
{
	auditinfo_addr_t *new_aia_p, *old_aia_p;
	kauth_cred_t my_cred = NULL;
	au_asid_t old_asid;
	int err = 0;

	*new_asid = AU_DEFAUDITSID;

	if ((new_aia_p = audit_session_porttoaia(port)) == NULL) {
		err = EINVAL;
		goto done;
	}

	proc_ucred_lock(p);
	kauth_cred_ref(p->p_ucred);
	my_cred = p->p_ucred;
	if (!IS_VALID_CRED(my_cred)) {
		kauth_cred_unref(&my_cred);
		proc_ucred_unlock(p);
		err = ESRCH;
		goto done;
	}
	old_aia_p = my_cred->cr_audit.as_aia_p;
	old_asid = old_aia_p->ai_asid;
	*new_asid = new_aia_p->ai_asid;

	  
                                                 
    
	if (*new_asid != old_asid) {
		kauth_cred_t my_new_cred;
		struct au_session new_as;

		bcopy(&new_aia_p->ai_mask, &new_as.as_mask,
		    sizeof(new_as.as_mask));
		new_as.as_aia_p = new_aia_p;

		my_new_cred = kauth_cred_setauditinfo(my_cred, &new_as);
		p->p_ucred = my_new_cred;
		PROC_UPDATE_CREDS_ONPROC(p);

		                                             
		audit_inc_procount(AU_SENTRY_PTR(new_aia_p));

		proc_ucred_unlock(p);

		                                                             
		set_security_token_task_internal(p, task);

		                                                        
		audit_dec_procount(AU_SENTRY_PTR(old_aia_p));
	} else {
		proc_ucred_unlock(p);
	}
	kauth_cred_unref(&my_cred);

done:
	if (port != IPC_PORT_NULL) {
		ipc_port_release_send(port);
	}

	return err;
}

  
                          
  
                                                                         
  
                      
                                               
                                                    
   
int
audit_session_spawnjoin(proc_t p, task_t task, ipc_port_t port)
{
	au_asid_t new_asid;

	return audit_session_join_internal(p, task, port, &new_asid);
}

  
                                    
  
                                                                  
  
                                               
                                            
  
                                                      
                                                     
                                                 
                      
  
                    
                                               
                                                    
   
int
audit_session_join(proc_t p, struct audit_session_join_args *uap,
    au_asid_t *ret_asid)
{
	ipc_port_t port = IPC_PORT_NULL;
	mach_port_name_t send = uap->port;
	int err = 0;


	if (ipc_object_copyin(get_task_ipcspace(p->task), send,
	    MACH_MSG_TYPE_COPY_SEND, &port, 0, NULL, IPC_OBJECT_COPYIN_FLAGS_ALLOW_IMMOVABLE_SEND) != KERN_SUCCESS) {
		*ret_asid = AU_DEFAUDITSID;
		err = EINVAL;
	} else {
		err = audit_session_join_internal(p, p->task, port, ret_asid);
	}

	return err;
}

  
                        
   

  
                            
   
static void
audit_sdev_entry_free(struct audit_sdev_entry *ase)
{
	free(ase->ase_record, M_AUDIT_SDEV_ENTRY);
	free(ase, M_AUDIT_SDEV_ENTRY);
}

  
                                                                        
                                                                       
                          
   
static void
audit_sdev_append(struct audit_sdev *asdev, void *record, u_int record_len)
{
	struct audit_sdev_entry *ase;

	AUDIT_SDEV_LOCK_ASSERT(asdev);

	if (asdev->asdev_qlen >= asdev->asdev_qlimit) {
		asdev->asdev_drops++;
		audit_sdev_drops++;
		return;
	}

	ase = malloc(sizeof(*ase), M_AUDIT_SDEV_ENTRY, M_NOWAIT | M_ZERO);
	if (NULL == ase) {
		asdev->asdev_drops++;
		audit_sdev_drops++;
		return;
	}

	ase->ase_record = malloc(record_len, M_AUDIT_SDEV_ENTRY, M_NOWAIT);
	if (NULL == ase->ase_record) {
		free(ase, M_AUDIT_SDEV_ENTRY);
		asdev->asdev_drops++;
		audit_sdev_drops++;
		return;
	}

	bcopy(record, ase->ase_record, record_len);
	ase->ase_record_len = record_len;

	TAILQ_INSERT_TAIL(&asdev->asdev_queue, ase, ase_queue);
	asdev->asdev_inserts++;
	asdev->asdev_qlen++;
	asdev->asdev_qbyteslen += ase->ase_record_len;
	selwakeup(&asdev->asdev_selinfo);
	if (asdev->asdev_flags & AUDIT_SDEV_ASYNC) {
		pgsigio(asdev->asdev_sigio, SIGIO);
	}

	cv_broadcast(&asdev->asdev_cv);
}

  
                                                                   
   
void
audit_sdev_submit(__unused au_id_t auid, __unused au_asid_t asid, void *record,
    u_int record_len)
{
	struct audit_sdev *asdev;

	  
                                                                     
        
    
	if (NULL == TAILQ_FIRST(&audit_sdev_list)) {
		return;
	}

	AUDIT_SDEV_LIST_RLOCK();
	TAILQ_FOREACH(asdev, &audit_sdev_list, asdev_list) {
		AUDIT_SDEV_LOCK(asdev);

		  
                                                                  
                                                             
                             
     
		if ((                                        
			    asid == asdev->asdev_asid) ||
		    (asdev->asdev_flags & AUDIT_SDEV_ALLSESSIONS) != 0) {
			audit_sdev_append(asdev, record, record_len);
		}
		AUDIT_SDEV_UNLOCK(asdev);
	}
	AUDIT_SDEV_LIST_RUNLOCK();

	                         
	audit_sdev_records++;
}

  
                                                                          
                               
   
static struct audit_sdev *
audit_sdev_alloc(void)
{
	struct audit_sdev *asdev;

	AUDIT_SDEV_LIST_WLOCK_ASSERT();

	asdev = malloc(sizeof(*asdev), M_AUDIT_SDEV, M_WAITOK | M_ZERO);
	if (NULL == asdev) {
		return NULL;
	}

	asdev->asdev_qlimit = AUDIT_SDEV_QLIMIT_DEFAULT;
	TAILQ_INIT(&asdev->asdev_queue);
	AUDIT_SDEV_LOCK_INIT(asdev);
	AUDIT_SDEV_SX_LOCK_INIT(asdev);
	cv_init(&asdev->asdev_cv, "audit_sdev_cv");

	  
                                                    
    
	TAILQ_INSERT_HEAD(&audit_sdev_list, asdev, asdev_list);
	audit_sdev_count++;
	audit_sdev_ever++;

	return asdev;
}

  
                                                        
   
static void
audit_sdev_flush(struct audit_sdev *asdev)
{
	struct audit_sdev_entry *ase;

	AUDIT_SDEV_LOCK_ASSERT(asdev);

	while ((ase = TAILQ_FIRST(&asdev->asdev_queue)) != NULL) {
		TAILQ_REMOVE(&asdev->asdev_queue, ase, ase_queue);
		asdev->asdev_qbyteslen -= ase->ase_record_len;
		audit_sdev_entry_free(ase);
		asdev->asdev_qlen--;
	}
	asdev->asdev_qoffset = 0;

	KASSERT(0 == asdev->asdev_qlen, ("audit_sdev_flush: asdev_qlen"));
	KASSERT(0 == asdev->asdev_qbyteslen,
	    ("audit_sdev_flush: asdev_qbyteslen"));
}

  
                      
   
static void
audit_sdev_free(struct audit_sdev *asdev)
{
	AUDIT_SDEV_LIST_WLOCK_ASSERT();
	AUDIT_SDEV_LOCK_ASSERT(asdev);

	                                 
	audit_sdev_flush(asdev);
	cv_destroy(&asdev->asdev_cv);
	AUDIT_SDEV_SX_LOCK_DESTROY(asdev);
	AUDIT_SDEV_UNLOCK(asdev);
	AUDIT_SDEV_LOCK_DESTROY(asdev);

	TAILQ_REMOVE(&audit_sdev_list, asdev, asdev_list);
	free(asdev, M_AUDIT_SDEV);
	audit_sdev_count--;
}

  
                                                                             
                         
   
static int
audit_sdev_get_aia(proc_t p, struct auditinfo_addr *aia_p)
{
	int error;
	kauth_cred_t scred;

	scred = kauth_cred_proc_ref(p);
	error = suser(scred, &p->p_acflag);

	if (NULL != aia_p) {
		bcopy(scred->cr_audit.as_aia_p, aia_p, sizeof(*aia_p));
	}
	kauth_cred_unref(&scred);

	return error;
}

  
                                 
   
static int
audit_sdev_open(dev_t dev, __unused int flags, __unused int devtype, proc_t p)
{
	struct audit_sdev *asdev;
	struct auditinfo_addr aia;
	int u;

	u = minor(dev);
	if (u < 0 || u >= MAX_AUDIT_SDEVS) {
		return ENXIO;
	}

	(void) audit_sdev_get_aia(p, &aia);

	AUDIT_SDEV_LIST_WLOCK();
	asdev = audit_sdev_dtab[u];
	if (NULL == asdev) {
		asdev = audit_sdev_alloc();
		if (NULL == asdev) {
			AUDIT_SDEV_LIST_WUNLOCK();
			return ENOMEM;
		}
		audit_sdev_dtab[u] = asdev;
	} else {
		KASSERT(asdev->asdev_open, ("audit_sdev_open: Already open"));
		AUDIT_SDEV_LIST_WUNLOCK();
		return EBUSY;
	}
	asdev->asdev_open = 1;
	asdev->asdev_auid = aia.ai_auid;
	asdev->asdev_asid = aia.ai_asid;
	asdev->asdev_flags = 0;

	AUDIT_SDEV_LIST_WUNLOCK();

	return 0;
}

  
                                  
   
static int
audit_sdev_close(dev_t dev, __unused int flags, __unused int devtype,
    __unused proc_t p)
{
	struct audit_sdev *asdev;
	int u;

	u = minor(dev);
	asdev = audit_sdev_dtab[u];

	KASSERT(asdev != NULL, ("audit_sdev_close: asdev == NULL"));
	KASSERT(asdev->asdev_open, ("audit_sdev_close: !asdev_open"));

	AUDIT_SDEV_LIST_WLOCK();
	AUDIT_SDEV_LOCK(asdev);
	asdev->asdev_open = 0;
	audit_sdev_free(asdev);                                               
	audit_sdev_dtab[u] = NULL;
	AUDIT_SDEV_LIST_WUNLOCK();

	return 0;
}

  
                                  
   
static int
audit_sdev_ioctl(dev_t dev, u_long cmd, caddr_t data,
    __unused int flag, proc_t p)
{
	struct audit_sdev *asdev;
	int error;

	asdev = audit_sdev_dtab[minor(dev)];
	KASSERT(asdev != NULL, ("audit_sdev_ioctl: asdev == NULL"));

	error = 0;

	switch (cmd) {
	case FIONBIO:
		AUDIT_SDEV_LOCK(asdev);
		if (*(int *)data) {
			asdev->asdev_flags |= AUDIT_SDEV_NBIO;
		} else {
			asdev->asdev_flags &= ~AUDIT_SDEV_NBIO;
		}
		AUDIT_SDEV_UNLOCK(asdev);
		break;

	case FIONREAD:
		AUDIT_SDEV_LOCK(asdev);
		*(int *)data = asdev->asdev_qbyteslen - asdev->asdev_qoffset;
		AUDIT_SDEV_UNLOCK(asdev);
		break;

	case AUDITSDEV_GET_QLEN:
		*(u_int *)data = asdev->asdev_qlen;
		break;

	case AUDITSDEV_GET_QLIMIT:
		*(u_int *)data = asdev->asdev_qlimit;
		break;

	case AUDITSDEV_SET_QLIMIT:
		if (*(u_int *)data >= AUDIT_SDEV_QLIMIT_MIN ||
		    *(u_int *)data <= AUDIT_SDEV_QLIMIT_MAX) {
			asdev->asdev_qlimit = *(u_int *)data;
		} else {
			error = EINVAL;
		}
		break;

	case AUDITSDEV_GET_QLIMIT_MIN:
		*(u_int *)data = AUDIT_SDEV_QLIMIT_MIN;
		break;

	case AUDITSDEV_GET_QLIMIT_MAX:
		*(u_int *)data = AUDIT_SDEV_QLIMIT_MAX;
		break;

	case AUDITSDEV_FLUSH:
		if (AUDIT_SDEV_SX_XLOCK_SIG(asdev) != 0) {
			return EINTR;
		}
		AUDIT_SDEV_LOCK(asdev);
		audit_sdev_flush(asdev);
		AUDIT_SDEV_UNLOCK(asdev);
		AUDIT_SDEV_SX_XUNLOCK(asdev);
		break;

	case AUDITSDEV_GET_MAXDATA:
		*(u_int *)data = MAXAUDITDATA;
		break;

	                                          
	case AUDITSDEV_GET_INSERTS:
		*(u_int *)data = asdev->asdev_inserts;
		break;

	case AUDITSDEV_GET_READS:
		*(u_int *)data = asdev->asdev_reads;
		break;

	case AUDITSDEV_GET_DROPS:
		*(u_int *)data = asdev->asdev_drops;
		break;

	case AUDITSDEV_GET_ALLSESSIONS:
		error = audit_sdev_get_aia(p, NULL);
		if (error) {
			break;
		}
		*(u_int *)data = (asdev->asdev_flags & AUDIT_SDEV_ALLSESSIONS) ?
		    1 : 0;
		break;

	case AUDITSDEV_SET_ALLSESSIONS:
		error = audit_sdev_get_aia(p, NULL);
		if (error) {
			break;
		}

		AUDIT_SDEV_LOCK(asdev);
		if (*(int *)data) {
			asdev->asdev_flags |= AUDIT_SDEV_ALLSESSIONS;
		} else {
			asdev->asdev_flags &= ~AUDIT_SDEV_ALLSESSIONS;
		}
		AUDIT_SDEV_UNLOCK(asdev);
		break;

	default:
		error = ENOTTY;
	}

	return error;
}

  
                                 
   
static int
audit_sdev_read(dev_t dev, struct uio *uio, __unused int flag)
{
	struct audit_sdev_entry *ase;
	struct audit_sdev *asdev;
	u_int toread;
	int error;

	asdev = audit_sdev_dtab[minor(dev)];
	KASSERT(NULL != asdev, ("audit_sdev_read: asdev == NULL"));

	  
                                                                   
                                                      
    
	if (0 != AUDIT_SDEV_SX_XLOCK_SIG(asdev)) {
		return EINTR;
	}
	AUDIT_SDEV_LOCK(asdev);
	while (TAILQ_EMPTY(&asdev->asdev_queue)) {
		if (asdev->asdev_flags & AUDIT_SDEV_NBIO) {
			AUDIT_SDEV_UNLOCK(asdev);
			AUDIT_SDEV_SX_XUNLOCK(asdev);
			return EAGAIN;
		}
		error = cv_wait_sig(&asdev->asdev_cv, AUDIT_SDEV_MTX(asdev));
		if (error) {
			AUDIT_SDEV_UNLOCK(asdev);
			AUDIT_SDEV_SX_XUNLOCK(asdev);
			return error;
		}
	}

	  
                                                                     
                                                                     
                                                         
   
                                                               
    
	asdev->asdev_reads++;
	while ((ase = TAILQ_FIRST(&asdev->asdev_queue)) != NULL &&
	    uio_resid(uio) > 0) {
		AUDIT_SDEV_LOCK_ASSERT(asdev);

		KASSERT(ase->ase_record_len > asdev->asdev_qoffset,
		    ("audit_sdev_read: record_len > qoffset (1)"));
		toread = MIN((int)(ase->ase_record_len - asdev->asdev_qoffset),
		    uio_resid(uio));
		AUDIT_SDEV_UNLOCK(asdev);
		error = uiomove((char *) ase->ase_record + asdev->asdev_qoffset,
		    toread, uio);
		if (error) {
			AUDIT_SDEV_SX_XUNLOCK(asdev);
			return error;
		}

		  
                                                              
                                                     
     
		AUDIT_SDEV_LOCK(asdev);
		KASSERT(TAILQ_FIRST(&asdev->asdev_queue) == ase,
		    ("audit_sdev_read: queue out of sync after uiomove"));
		asdev->asdev_qoffset += toread;
		KASSERT(ase->ase_record_len >= asdev->asdev_qoffset,
		    ("audit_sdev_read: record_len >= qoffset (2)"));
		if (asdev->asdev_qoffset == ase->ase_record_len) {
			TAILQ_REMOVE(&asdev->asdev_queue, ase, ase_queue);
			asdev->asdev_qbyteslen -= ase->ase_record_len;
			audit_sdev_entry_free(ase);
			asdev->asdev_qlen--;
			asdev->asdev_qoffset = 0;
		}
	}
	AUDIT_SDEV_UNLOCK(asdev);
	AUDIT_SDEV_SX_XUNLOCK(asdev);
	return 0;
}

  
                                    
   
static int
audit_sdev_poll(dev_t dev, int events, void *wql, struct proc *p)
{
	struct audit_sdev *asdev;
	int revents;

	revents = 0;
	asdev = audit_sdev_dtab[minor(dev)];
	KASSERT(NULL != asdev, ("audit_sdev_poll: asdev == NULL"));

	if (events & (POLLIN | POLLRDNORM)) {
		AUDIT_SDEV_LOCK(asdev);
		if (NULL != TAILQ_FIRST(&asdev->asdev_queue)) {
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			selrecord(p, &asdev->asdev_selinfo, wql);
		}
		AUDIT_SDEV_UNLOCK(asdev);
	}
	return revents;
}

  
                                                                        
                                    
   
static int
audit_sdev_clone(__unused dev_t dev, int action)
{
	int i;

	if (DEVFS_CLONE_ALLOC == action) {
		for (i = 0; i < MAX_AUDIT_SDEVS; i++) {
			if (NULL == audit_sdev_dtab[i]) {
				return i;
			}
		}

		  
                                                        
                                                       
                                                
     
		return 0;
	}

	return -1;
}

static int
audit_sdev_init(void)
{
	dev_t dev;

	TAILQ_INIT(&audit_sdev_list);
	AUDIT_SDEV_LIST_LOCK_INIT();

	audit_sdev_major = cdevsw_add(-1, &audit_sdev_cdevsw);
	if (audit_sdev_major < 0) {
		return KERN_FAILURE;
	}

	dev = makedev(audit_sdev_major, 0);
	devnode = devfs_make_node_clone(dev, DEVFS_CHAR, UID_ROOT, GID_WHEEL,
	    0644, audit_sdev_clone, AUDIT_SDEV_NAME, 0);

	if (NULL == devnode) {
		return KERN_FAILURE;
	}

	return KERN_SUCCESS;
}

        
              
                             
     
  
                               
                                                                    
  
                               
     
   

#else

int
audit_session_self(proc_t p, struct audit_session_self_args *uap,
    mach_port_name_t *ret_port)
{
#pragma unused(p, uap, ret_port)

	return ENOSYS;
}

int
audit_session_join(proc_t p, struct audit_session_join_args *uap,
    au_asid_t *ret_asid)
{
#pragma unused(p, uap, ret_asid)

	return ENOSYS;
}

int
audit_session_port(proc_t p, struct audit_session_port_args *uap, int *retval)
{
#pragma unused(p, uap, retval)

	return ENOSYS;
}

#endif                   
