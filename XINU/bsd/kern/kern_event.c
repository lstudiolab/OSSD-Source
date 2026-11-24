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
 *
 */
/*-
 * Copyright (c) 1999,2000,2001 Jonathan Lemon <jlemon@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
  
                                         
   
#include <stdint.h>
#include <machine/atomic.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/malloc.h>
#include <sys/unistd.h>
#include <sys/file_internal.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/queue.h>
#include <sys/event.h>
#include <sys/eventvar.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/syscall.h>                   
#include <sys/sysctl.h>
#include <sys/uio.h>
#include <sys/sysproto.h>
#include <sys/user.h>
#include <sys/vnode_internal.h>
#include <string.h>
#include <sys/proc_info.h>
#include <sys/codesign.h>
#include <sys/pthread_shims.h>
#include <sys/kdebug.h>
#include <os/base.h>
#include <pexpert/pexpert.h>

#include <kern/locks.h>
#include <kern/clock.h>
#include <kern/cpu_data.h>
#include <kern/policy_internal.h>
#include <kern/thread_call.h>
#include <kern/sched_prim.h>
#include <kern/waitq.h>
#include <kern/zalloc.h>
#include <kern/kalloc.h>
#include <kern/assert.h>
#include <kern/ast.h>
#include <kern/thread.h>
#include <kern/kcdata.h>

#include <pthread/priority_private.h>
#include <pthread/workqueue_syscalls.h>
#include <pthread/workqueue_internal.h>
#include <libkern/libkern.h>

#include "net/net_str_id.h"

#include <mach/task.h>
#include <libkern/section_keywords.h>

#if CONFIG_MEMORYSTATUS
#include <sys/kern_memorystatus.h>
#endif

#if DEVELOPMENT || DEBUG
#define KEVENT_PANIC_ON_WORKLOOP_OWNERSHIP_LEAK  (1U << 0)
#define KEVENT_PANIC_ON_NON_ENQUEUED_PROCESS     (1U << 1)
TUNABLE(uint32_t, kevent_debug_flags, "kevent_debug", 0);
#endif

static LCK_GRP_DECLARE(kq_lck_grp, "kqueue");
SECURITY_READ_ONLY_EARLY(vm_packing_params_t) kn_kq_packing_params =
    VM_PACKING_PARAMS(KNOTE_KQ_PACKED);

extern mach_port_name_t ipc_entry_name_mask(mach_port_name_t name);                            
extern int cansignal(struct proc *, kauth_cred_t, struct proc *, int);                          

#define KEV_EVTID(code) BSDDBG_CODE(DBG_BSD_KEVENT, (code))

  
                                                      
                                            
   
#define KM_KQUEUE       KHEAP_DEFAULT

#define KQ_EVENT        NO_EVENT64

static int kqueue_select(struct fileproc *fp, int which, void *wq_link_id,
    vfs_context_t ctx);
static int kqueue_close(struct fileglob *fg, vfs_context_t ctx);
static int kqueue_kqfilter(struct fileproc *fp, struct knote *kn,
    struct kevent_qos_s *kev);
static int kqueue_drain(struct fileproc *fp, vfs_context_t ctx);

static const struct fileops kqueueops = {
	.fo_type     = DTYPE_KQUEUE,
	.fo_read     = fo_no_read,
	.fo_write    = fo_no_write,
	.fo_ioctl    = fo_no_ioctl,
	.fo_select   = kqueue_select,
	.fo_close    = kqueue_close,
	.fo_drain    = kqueue_drain,
	.fo_kqfilter = kqueue_kqfilter,
};

static inline int kevent_modern_copyout(struct kevent_qos_s *, user_addr_t *);
static int kevent_register_wait_prepare(struct knote *kn, struct kevent_qos_s *kev, int result);
static void kevent_register_wait_block(struct turnstile *ts, thread_t handoff_thread,
    thread_continue_t cont, struct _kevent_register *cont_args) __dead2;
static void kevent_register_wait_return(struct _kevent_register *cont_args) __dead2;
static void kevent_register_wait_cleanup(struct knote *kn);

static struct kqtailq *kqueue_get_suppressed_queue(kqueue_t kq, struct knote *kn);
static void kqueue_threadreq_initiate(struct kqueue *kq, workq_threadreq_t, kq_index_t qos, int flags);

static void kqworkq_unbind(proc_t p, workq_threadreq_t);
static thread_qos_t kqworkq_unbind_locked(struct kqworkq *kqwq, workq_threadreq_t, thread_t thread);
static workq_threadreq_t kqworkq_get_request(struct kqworkq *kqwq, kq_index_t qos_index);

static void kqworkloop_unbind(struct kqworkloop *kwql);

enum kqwl_unbind_locked_mode {
	KQWL_OVERRIDE_DROP_IMMEDIATELY,
	KQWL_OVERRIDE_DROP_DELAYED,
};
static void kqworkloop_unbind_locked(struct kqworkloop *kwql, thread_t thread,
    enum kqwl_unbind_locked_mode how);
static void kqworkloop_unbind_delayed_override_drop(thread_t thread);
static kq_index_t kqworkloop_override(struct kqworkloop *kqwl);
static void kqworkloop_set_overcommit(struct kqworkloop *kqwl);
enum {
	KQWL_UTQ_NONE,
	  
                                               
   
                                                             
                                                                             
                                                                             
                                                              
    
	KQWL_UTQ_UPDATE_WAKEUP_QOS,
	KQWL_UTQ_UPDATE_STAYACTIVE_QOS,
	KQWL_UTQ_RECOMPUTE_WAKEUP_QOS,
	KQWL_UTQ_UNBINDING,                        
	KQWL_UTQ_PARKING,
	  
                                                                         
                                                                    
                                                                          
    
	KQWL_UTQ_UPDATE_WAKEUP_OVERRIDE,
	KQWL_UTQ_RESET_WAKEUP_OVERRIDE,
	  
                                                                       
                                                                   
                                                                             
                              
    
	KQWL_UTQ_SET_QOS_INDEX,
	KQWL_UTQ_REDRIVE_EVENTS,
};
static void kqworkloop_update_threads_qos(struct kqworkloop *kqwl, int op, kq_index_t qos);
static int kqworkloop_end_processing(struct kqworkloop *kqwl, int flags, int kevent_flags);

static struct knote *knote_alloc(void);
static void knote_free(struct knote *kn);
static int kq_add_knote(struct kqueue *kq, struct knote *kn,
    struct knote_lock_ctx *knlc, struct proc *p);
static struct knote *kq_find_knote_and_kq_lock(struct kqueue *kq,
    struct kevent_qos_s *kev, bool is_fd, struct proc *p);

static void knote_activate(kqueue_t kqu, struct knote *kn, int result);
static void knote_dequeue(kqueue_t kqu, struct knote *kn);

static void knote_apply_touch(kqueue_t kqu, struct knote *kn,
    struct kevent_qos_s *kev, int result);
static void knote_suppress(kqueue_t kqu, struct knote *kn);
static void knote_unsuppress(kqueue_t kqu, struct knote *kn);
static void knote_drop(kqueue_t kqu, struct knote *kn, struct knote_lock_ctx *knlc);

                                                                        
                            
static void knote_adjust_qos(struct kqueue *kq, struct knote *kn, int result);
static void knote_reset_priority(kqueue_t kqu, struct knote *kn, pthread_priority_t pp);

static ZONE_DECLARE(knote_zone, "knote zone",
    sizeof(struct knote), ZC_CACHING | ZC_ZFREE_CLEARMEM);
static ZONE_DECLARE(kqfile_zone, "kqueue file zone",
    sizeof(struct kqfile), ZC_ZFREE_CLEARMEM);
static ZONE_DECLARE(kqworkq_zone, "kqueue workq zone",
    sizeof(struct kqworkq), ZC_ZFREE_CLEARMEM);
static ZONE_DECLARE(kqworkloop_zone, "kqueue workloop zone",
    sizeof(struct kqworkloop), ZC_CACHING | ZC_ZFREE_CLEARMEM);

#define KN_HASH(val, mask)      (((val) ^ (val >> 8)) & (mask))

static int filt_no_attach(struct knote *kn, struct kevent_qos_s *kev);
static void filt_no_detach(struct knote *kn);
static int filt_bad_event(struct knote *kn, long hint);
static int filt_bad_touch(struct knote *kn, struct kevent_qos_s *kev);
static int filt_bad_process(struct knote *kn, struct kevent_qos_s *kev);

SECURITY_READ_ONLY_EARLY(static struct filterops) bad_filtops = {
	.f_attach  = filt_no_attach,
	.f_detach  = filt_no_detach,
	.f_event   = filt_bad_event,
	.f_touch   = filt_bad_touch,
	.f_process = filt_bad_process,
};

#if CONFIG_MEMORYSTATUS
extern const struct filterops memorystatus_filtops;
#endif                          
extern const struct filterops fs_filtops;
extern const struct filterops sig_filtops;
extern const struct filterops machport_filtops;
extern const struct filterops pipe_nfiltops;
extern const struct filterops pipe_rfiltops;
extern const struct filterops pipe_wfiltops;
extern const struct filterops ptsd_kqops;
extern const struct filterops ptmx_kqops;
extern const struct filterops soread_filtops;
extern const struct filterops sowrite_filtops;
extern const struct filterops sock_filtops;
extern const struct filterops soexcept_filtops;
extern const struct filterops spec_filtops;
extern const struct filterops bpfread_filtops;
extern const struct filterops necp_fd_rfiltops;
extern const struct filterops fsevent_filtops;
extern const struct filterops vnode_filtops;
extern const struct filterops tty_filtops;

const static struct filterops file_filtops;
const static struct filterops kqread_filtops;
const static struct filterops proc_filtops;
const static struct filterops timer_filtops;
const static struct filterops user_filtops;
const static struct filterops workloop_filtops;

  
  
                                              
                  
                                                                                     
                                          
                                                                 
                                                                                        
                                                
                   
                                                                              
                                                    
                                                              
                                                                                      
                                              
   
static_assert(EVFILTID_MAX < UINT8_MAX, "kn_filtid expects this to be true");
static const struct filterops * const sysfilt_ops[EVFILTID_MAX] = {
	                    
	[~EVFILT_READ]                  = &file_filtops,
	[~EVFILT_WRITE]                 = &file_filtops,
	[~EVFILT_AIO]                   = &bad_filtops,
	[~EVFILT_VNODE]                 = &file_filtops,
	[~EVFILT_PROC]                  = &proc_filtops,
	[~EVFILT_SIGNAL]                = &sig_filtops,
	[~EVFILT_TIMER]                 = &timer_filtops,
	[~EVFILT_MACHPORT]              = &machport_filtops,
	[~EVFILT_FS]                    = &fs_filtops,
	[~EVFILT_USER]                  = &user_filtops,
	[~EVFILT_UNUSED_11]             = &bad_filtops,
	[~EVFILT_VM]                    = &bad_filtops,
	[~EVFILT_SOCK]                  = &file_filtops,
#if CONFIG_MEMORYSTATUS
	[~EVFILT_MEMORYSTATUS]          = &memorystatus_filtops,
#else
	[~EVFILT_MEMORYSTATUS]          = &bad_filtops,
#endif
	[~EVFILT_EXCEPT]                = &file_filtops,
	[~EVFILT_WORKLOOP]              = &workloop_filtops,

	                     
	[EVFILTID_KQREAD]               = &kqread_filtops,
	[EVFILTID_PIPE_N]               = &pipe_nfiltops,
	[EVFILTID_PIPE_R]               = &pipe_rfiltops,
	[EVFILTID_PIPE_W]               = &pipe_wfiltops,
	[EVFILTID_PTSD]                 = &ptsd_kqops,
	[EVFILTID_SOREAD]               = &soread_filtops,
	[EVFILTID_SOWRITE]              = &sowrite_filtops,
	[EVFILTID_SCK]                  = &sock_filtops,
	[EVFILTID_SOEXCEPT]             = &soexcept_filtops,
	[EVFILTID_SPEC]                 = &spec_filtops,
	[EVFILTID_BPFREAD]              = &bpfread_filtops,
	[EVFILTID_NECP_FD]              = &necp_fd_rfiltops,
	[EVFILTID_FSEVENT]              = &fsevent_filtops,
	[EVFILTID_VN]                   = &vnode_filtops,
	[EVFILTID_TTY]                  = &tty_filtops,
	[EVFILTID_PTMX]                 = &ptmx_kqops,

	                                                
	[EVFILTID_DETACHED]             = &bad_filtops,
};

                            
void waitq_set__CALLING_PREPOST_HOOK__(waitq_set_prepost_hook_t *kq_hook);

static inline bool
kqr_thread_bound(workq_threadreq_t kqr)
{
	return kqr->tr_state == WORKQ_TR_STATE_BOUND;
}

static inline bool
kqr_thread_requested_pending(workq_threadreq_t kqr)
{
	workq_tr_state_t tr_state = kqr->tr_state;
	return tr_state > WORKQ_TR_STATE_IDLE && tr_state < WORKQ_TR_STATE_BOUND;
}

static inline bool
kqr_thread_requested(workq_threadreq_t kqr)
{
	return kqr->tr_state != WORKQ_TR_STATE_IDLE;
}

static inline thread_t
kqr_thread_fast(workq_threadreq_t kqr)
{
	assert(kqr_thread_bound(kqr));
	return kqr->tr_thread;
}

static inline thread_t
kqr_thread(workq_threadreq_t kqr)
{
	return kqr_thread_bound(kqr) ? kqr->tr_thread : THREAD_NULL;
}

static inline struct kqworkloop *
kqr_kqworkloop(workq_threadreq_t kqr)
{
	if (kqr->tr_flags & WORKQ_TR_FLAG_WORKLOOP) {
		return __container_of(kqr, struct kqworkloop, kqwl_request);
	}
	return NULL;
}

static inline kqueue_t
kqr_kqueue(proc_t p, workq_threadreq_t kqr)
{
	kqueue_t kqu;
	if (kqr->tr_flags & WORKQ_TR_FLAG_WORKLOOP) {
		kqu.kqwl = kqr_kqworkloop(kqr);
	} else {
		kqu.kqwq = p->p_fd->fd_wqkqueue;
		assert(kqr >= kqu.kqwq->kqwq_request &&
		    kqr < kqu.kqwq->kqwq_request + KQWQ_NBUCKETS);
	}
	return kqu;
}

  
                                   
  
                                                                
                                                              
  
                                                              
                                                               
                                                              
                                                              
                                                              
                                                           
  
                                                               
                                                              
                                                               
        
   

static inline void
kqlock(kqueue_t kqu)
{
	lck_spin_lock(&kqu.kq->kq_lock);
}

static inline void
kqlock_held(__assert_only kqueue_t kqu)
{
	LCK_SPIN_ASSERT(&kqu.kq->kq_lock, LCK_ASSERT_OWNED);
}

static inline void
kqunlock(kqueue_t kqu)
{
	lck_spin_unlock(&kqu.kq->kq_lock);
}

static inline void
knhash_lock(struct filedesc *fdp)
{
	lck_mtx_lock(&fdp->fd_knhashlock);
}

static inline void
knhash_unlock(struct filedesc *fdp)
{
	lck_mtx_unlock(&fdp->fd_knhashlock);
}

                                
static inline event_t
knote_lock_wev(struct knote *kn)
{
	return (event_t)(&kn->kn_hook);
}

                                           
static inline event64_t
knote_filt_wev64(struct knote *kn)
{
	                                                        
	return CAST_EVENT64_T(kn);
}

                                          
static inline event64_t
knote_post_wev64(struct knote *kn)
{
	return CAST_EVENT64_T(&kn->kn_kevent);
}

   
                          
  
         
                                       
  
              
                      
                 
                                                              
  
                                                               
   
static inline bool
knote_has_qos(struct knote *kn)
{
	return kn->kn_qos_override > 0 && kn->kn_qos_override < THREAD_QOS_LAST;
}

#pragma mark knote locks

  
                                           
  
                       
                                                           
  
                           
                                                                        
                                                       
  
                           
                                                                          
                                                       
  
                   
                                               
   
enum kqlocking {
	KNOTE_KQ_LOCK_ALWAYS,
	KNOTE_KQ_LOCK_ON_SUCCESS,
	KNOTE_KQ_LOCK_ON_FAILURE,
	KNOTE_KQ_UNLOCK,
};

static struct knote_lock_ctx *
knote_lock_ctx_find(kqueue_t kqu, struct knote *kn)
{
	struct knote_lock_ctx *ctx;
	LIST_FOREACH(ctx, &kqu.kq->kq_knlocks, knlc_link) {
		if (ctx->knlc_knote == kn) {
			return ctx;
		}
	}
	panic("knote lock context not found: %p", kn);
	__builtin_trap();
}

                              
__attribute__((noinline))
static bool __result_use_check
knote_lock_slow(kqueue_t kqu, struct knote *kn,
    struct knote_lock_ctx *knlc, int kqlocking)
{
	struct knote_lock_ctx *owner_lc;
	struct uthread *uth = current_uthread();
	wait_result_t wr;

	kqlock_held(kqu);

	owner_lc = knote_lock_ctx_find(kqu, kn);
#if DEBUG || DEVELOPMENT
	knlc->knlc_state = KNOTE_LOCK_CTX_WAITING;
#endif
	owner_lc->knlc_waiters++;

	  
                                                   
    
	uth->uu_knlock = knlc;

	wr = lck_spin_sleep_with_inheritor(&kqu.kq->kq_lock, LCK_SLEEP_UNLOCK,
	    knote_lock_wev(kn), owner_lc->knlc_thread,
	    THREAD_UNINT | THREAD_WAIT_NOREPORT, TIMEOUT_WAIT_FOREVER);

	if (wr == THREAD_RESTART) {
		  
                                                                        
                                                   
     
		uth->uu_knlock = NULL;
#if DEBUG || DEVELOPMENT
		assert(knlc->knlc_state == KNOTE_LOCK_CTX_WAITING);
		knlc->knlc_state = KNOTE_LOCK_CTX_UNLOCKED;
#endif

		if (kqlocking == KNOTE_KQ_LOCK_ALWAYS ||
		    kqlocking == KNOTE_KQ_LOCK_ON_FAILURE) {
			kqlock(kqu);
		}
		return false;
	} else {
		if (kqlocking == KNOTE_KQ_LOCK_ALWAYS ||
		    kqlocking == KNOTE_KQ_LOCK_ON_SUCCESS) {
			kqlock(kqu);
#if DEBUG || DEVELOPMENT
			  
                                                  
                                                 
      
			assert(knlc->knlc_state == KNOTE_LOCK_CTX_LOCKED);
#endif
		}
		return true;
	}
}

  
                                     
  
                                    
  
                                                                           
   
static bool __result_use_check
knote_lock(kqueue_t kqu, struct knote *kn, struct knote_lock_ctx *knlc,
    enum kqlocking kqlocking)
{
	kqlock_held(kqu);

#if DEBUG || DEVELOPMENT
	assert(knlc->knlc_state == KNOTE_LOCK_CTX_UNLOCKED);
#endif
	knlc->knlc_knote = kn;
	knlc->knlc_thread = current_thread();
	knlc->knlc_waiters = 0;

	if (__improbable(kn->kn_status & KN_LOCKED)) {
		return knote_lock_slow(kqu, kn, knlc, kqlocking);
	}

	  
                                                                  
                                                                   
                                                              
    
	assert((kn->kn_status & KN_DROPPING) == 0);
	LIST_INSERT_HEAD(&kqu.kq->kq_knlocks, knlc, knlc_link);
	kn->kn_status |= KN_LOCKED;
#if DEBUG || DEVELOPMENT
	knlc->knlc_state = KNOTE_LOCK_CTX_LOCKED;
#endif

	if (kqlocking == KNOTE_KQ_UNLOCK ||
	    kqlocking == KNOTE_KQ_LOCK_ON_FAILURE) {
		kqunlock(kqu);
	}
	return true;
}

  
                                                         
  
                                    
  
                                                                  
   
static void
knote_unlock(kqueue_t kqu, struct knote *kn,
    struct knote_lock_ctx *knlc, enum kqlocking kqlocking)
{
	kqlock_held(kqu);

	assert(knlc->knlc_knote == kn);
	assert(kn->kn_status & KN_LOCKED);
#if DEBUG || DEVELOPMENT
	assert(knlc->knlc_state == KNOTE_LOCK_CTX_LOCKED);
#endif

	LIST_REMOVE(knlc, knlc_link);

	if (knlc->knlc_waiters) {
		thread_t thread = THREAD_NULL;

		wakeup_one_with_inheritor(knote_lock_wev(kn), THREAD_AWAKENED,
		    LCK_WAKE_DEFAULT, &thread);

		  
                                                            
                           
    
                                                   
     
		struct uthread *ut = get_bsdthread_info(thread);
		struct knote_lock_ctx *next_owner_lc = ut->uu_knlock;

		assert(next_owner_lc->knlc_knote == kn);
		next_owner_lc->knlc_waiters = knlc->knlc_waiters - 1;
		LIST_INSERT_HEAD(&kqu.kq->kq_knlocks, next_owner_lc, knlc_link);
#if DEBUG || DEVELOPMENT
		next_owner_lc->knlc_state = KNOTE_LOCK_CTX_LOCKED;
#endif
		ut->uu_knlock = NULL;
		thread_deallocate_safe(thread);
	} else {
		kn->kn_status &= ~KN_LOCKED;
	}

	if ((kn->kn_status & KN_MERGE_QOS) && !(kn->kn_status & KN_POSTING)) {
		  
                                                                  
    
                           
     
		kn->kn_status &= ~KN_MERGE_QOS;
	}
	if (kqlocking == KNOTE_KQ_UNLOCK) {
		kqunlock(kqu);
	}
#if DEBUG || DEVELOPMENT
	knlc->knlc_state = KNOTE_LOCK_CTX_UNLOCKED;
#endif
}

  
                                                             
  
                                    
  
                                    
   
static void
knote_unlock_cancel(struct kqueue *kq, struct knote *kn,
    struct knote_lock_ctx *knlc)
{
	kqlock_held(kq);

	assert(knlc->knlc_knote == kn);
	assert(kn->kn_status & KN_LOCKED);
	assert(kn->kn_status & KN_DROPPING);

	LIST_REMOVE(knlc, knlc_link);
	kn->kn_status &= ~KN_LOCKED;
	kqunlock(kq);

	if (knlc->knlc_waiters) {
		wakeup_all_with_inheritor(knote_lock_wev(kn), THREAD_RESTART);
	}
#if DEBUG || DEVELOPMENT
	knlc->knlc_state = KNOTE_LOCK_CTX_UNLOCKED;
#endif
}

  
                                           
  
                                                         
   
static void
knote_post(struct knote *kn, long hint)
{
	struct kqueue *kq = knote_get_kq(kn);
	int dropping, result;

	kqlock(kq);

	if (__improbable(kn->kn_status & (KN_DROPPING | KN_VANISHED))) {
		return kqunlock(kq);
	}

	if (__improbable(kn->kn_status & KN_POSTING)) {
		panic("KNOTE() called concurrently on knote %p", kn);
	}

	kn->kn_status |= KN_POSTING;

	kqunlock(kq);
	result = filter_call(knote_fops(kn), f_event(kn, hint));
	kqlock(kq);

	dropping = (kn->kn_status & KN_DROPPING);

	if (!dropping && (result & FILTER_ACTIVE)) {
		knote_activate(kq, kn, result);
	}

	if ((kn->kn_status & KN_LOCKED) == 0) {
		  
                                                                        
    
                           
     
		kn->kn_status &= ~(KN_POSTING | KN_MERGE_QOS);
	} else {
		kn->kn_status &= ~KN_POSTING;
	}

	if (__improbable(dropping)) {
		waitq_wakeup64_all((struct waitq *)&kq->kq_wqs, knote_post_wev64(kn),
		    THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);
	}

	kqunlock(kq);
}

  
                                                                           
  
                       
                        
   
static void
knote_wait_for_post(struct kqueue *kq, struct knote *kn)
{
	wait_result_t wr = THREAD_NOT_WAITING;

	kqlock_held(kq);

	assert(kn->kn_status & KN_DROPPING);

	if (kn->kn_status & KN_POSTING) {
		wr = waitq_assert_wait64((struct waitq *)&kq->kq_wqs,
		    knote_post_wev64(kn), THREAD_UNINT | THREAD_WAIT_NOREPORT,
		    TIMEOUT_WAIT_FOREVER);
	}
	kqunlock(kq);
	if (wr == THREAD_WAITING) {
		thread_block(THREAD_CONTINUE_NULL);
	}
}

#pragma mark knote helpers for filters

OS_ALWAYS_INLINE
void
knote_set_error(struct knote *kn, int error)
{
	kn->kn_flags |= EV_ERROR;
	kn->kn_sdata = error;
}

OS_ALWAYS_INLINE
int64_t
knote_low_watermark(const struct knote *kn)
{
	return (kn->kn_sfflags & NOTE_LOWAT) ? kn->kn_sdata : 1;
}

   
                                         
  
         
                                                         
  
              
                                                          
                                               
  
                                                             
  
                                                  
   
OS_ALWAYS_INLINE
void
knote_fill_kevent_with_sdata(struct knote *kn, struct kevent_qos_s *kev)
{
#define knote_assert_aliases(name1, offs1, name2) \
	static_assert(offsetof(struct kevent_qos_s, name1) + offs1 == \
	    offsetof(struct kevent_internal_s, name2), \
	        "kevent_qos_s::" #name1 " and kevent_internal_s::" #name2 "need to alias")
	  
                                                     
                                                                   
    
	knote_assert_aliases(ident, 0, kei_ident);
#ifdef __LITTLE_ENDIAN__
	knote_assert_aliases(filter, 0, kei_filter);                        
	knote_assert_aliases(filter, 1, kei_filtid);                        
#else
	knote_assert_aliases(filter, 0, kei_filtid);                        
	knote_assert_aliases(filter, 1, kei_filter);                        
#endif
	knote_assert_aliases(flags, 0, kei_flags);
	knote_assert_aliases(qos, 0, kei_qos);
	knote_assert_aliases(udata, 0, kei_udata);
	knote_assert_aliases(fflags, 0, kei_fflags);
	knote_assert_aliases(xflags, 0, kei_sfflags);                       
	knote_assert_aliases(data, 0, kei_sdata);                           
	knote_assert_aliases(ext, 0, kei_ext);
#undef knote_assert_aliases

	  
                                                                   
                                                          
                                                           
    
	*kev = *(struct kevent_qos_s *)&kn->kn_kevent;
	kev->xflags = 0;
	kev->filter |= 0xff00;
	if (kn->kn_flags & EV_CLEAR) {
		kn->kn_fflags = 0;
	}
}

   
                              
  
         
                                                         
  
              
                                                          
                                                     
  
                                                             
   
OS_ALWAYS_INLINE
void
knote_fill_kevent(struct knote *kn, struct kevent_qos_s *kev, int64_t data)
{
	knote_fill_kevent_with_sdata(kn, kev);
	kev->filter = kn->kn_filter;
	kev->data = data;
}


#pragma mark file_filtops

static int
filt_fileattach(struct knote *kn, struct kevent_qos_s *kev)
{
	return fo_kqfilter(kn->kn_fp, kn, kev);
}

SECURITY_READ_ONLY_EARLY(static struct filterops) file_filtops = {
	.f_isfd = 1,
	.f_attach = filt_fileattach,
};

#pragma mark kqread_filtops

#define f_flag fp_glob->fg_flag
#define f_ops fp_glob->fg_ops
#define f_data fp_glob->fg_data
#define f_lflags fp_glob->fg_lflags

static void
filt_kqdetach(struct knote *kn)
{
	struct kqfile *kqf = (struct kqfile *)kn->kn_fp->f_data;
	struct kqueue *kq = &kqf->kqf_kqueue;

	kqlock(kq);
	KNOTE_DETACH(&kqf->kqf_sel.si_note, kn);
	kqunlock(kq);
}

static int
filt_kqueue(struct knote *kn, __unused long hint)
{
	struct kqueue *kq = (struct kqueue *)kn->kn_fp->f_data;

	return kq->kq_count > 0;
}

static int
filt_kqtouch(struct knote *kn, struct kevent_qos_s *kev)
{
#pragma unused(kev)
	struct kqueue *kq = (struct kqueue *)kn->kn_fp->f_data;
	int res;

	kqlock(kq);
	res = (kq->kq_count > 0);
	kqunlock(kq);

	return res;
}

static int
filt_kqprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	struct kqueue *kq = (struct kqueue *)kn->kn_fp->f_data;
	int res = 0;

	kqlock(kq);
	if (kq->kq_count) {
		knote_fill_kevent(kn, kev, kq->kq_count);
		res = 1;
	}
	kqunlock(kq);

	return res;
}

SECURITY_READ_ONLY_EARLY(static struct filterops) kqread_filtops = {
	.f_isfd = 1,
	.f_detach = filt_kqdetach,
	.f_event = filt_kqueue,
	.f_touch = filt_kqtouch,
	.f_process = filt_kqprocess,
};

#pragma mark proc_filtops

static int
filt_procattach(struct knote *kn, __unused struct kevent_qos_s *kev)
{
	struct proc *p;

	assert(PID_MAX < NOTE_PDATAMASK);

	if ((kn->kn_sfflags & (NOTE_TRACK | NOTE_TRACKERR | NOTE_CHILD)) != 0) {
		knote_set_error(kn, ENOTSUP);
		return 0;
	}

	p = proc_find((int)kn->kn_id);
	if (p == NULL) {
		knote_set_error(kn, ESRCH);
		return 0;
	}

	const uint32_t NoteExitStatusBits = NOTE_EXIT | NOTE_EXITSTATUS;

	if ((kn->kn_sfflags & NoteExitStatusBits) == NoteExitStatusBits) {
		do {
			pid_t selfpid = proc_selfpid();

			if (p->p_ppid == selfpid) {
				break;                    
			}
			if ((p->p_lflag & P_LTRACED) != 0 &&
			    (p->p_oppid == selfpid)) {
				break;                               
			}
			if (cansignal(current_proc(), kauth_cred_get(), p, SIGKILL)) {
				break;                              
			}
			proc_rele(p);
			knote_set_error(kn, EACCES);
			return 0;
		} while (0);
	}

	kn->kn_proc = p;
	kn->kn_flags |= EV_CLEAR;                              
	kn->kn_sdata = 0;                                             

	proc_klist_lock();

	KNOTE_ATTACH(&p->p_klist, kn);

	proc_klist_unlock();

	proc_rele(p);

	  
                                                        
                                 
    
	return 0;
}


  
                                                                    
                                                                  
                                                                
   
static void
filt_procdetach(struct knote *kn)
{
	struct proc *p;

	proc_klist_lock();

	p = kn->kn_proc;
	if (p != PROC_NULL) {
		kn->kn_proc = PROC_NULL;
		KNOTE_DETACH(&p->p_klist, kn);
	}

	proc_klist_unlock();
}

static int
filt_procevent(struct knote *kn, long hint)
{
	u_int event;

	                                        

	  
                                                              
                                                                        
                                   
   
                       
    
	event = (u_int)hint & NOTE_PCTRLMASK;

	  
                                                            
                                                         
                                                        
                                                        
                                                               
                                     
    
	if (event & NOTE_EXIT) {
		if ((kn->kn_proc->p_oppid != 0)
		    && (knote_get_kq(kn)->kq_p->p_pid != kn->kn_proc->p_ppid)) {
			  
                                                                 
      
			return 0;
		}
	}

	  
                                                       
    
	if (kn->kn_sfflags & event) {
		kn->kn_fflags |= event;
	}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	if ((event == NOTE_REAP) || ((event == NOTE_EXIT) && !(kn->kn_sfflags & NOTE_REAP))) {
		kn->kn_flags |= (EV_EOF | EV_ONESHOT);
	}
#pragma clang diagnostic pop


	  
                                                                
                                                           
                                                      
                                                               
    
	if (event == NOTE_EXIT) {
		kn->kn_hook32 = 0;
		if ((kn->kn_sfflags & NOTE_EXITSTATUS) != 0) {
			kn->kn_fflags |= NOTE_EXITSTATUS;
			kn->kn_hook32 |= (hint & NOTE_PDATAMASK);
		}
		if ((kn->kn_sfflags & NOTE_EXIT_DETAIL) != 0) {
			kn->kn_fflags |= NOTE_EXIT_DETAIL;
			if ((kn->kn_proc->p_lflag &
			    P_LTERM_DECRYPTFAIL) != 0) {
				kn->kn_hook32 |= NOTE_EXIT_DECRYPTFAIL;
			}
			if ((kn->kn_proc->p_lflag &
			    P_LTERM_JETSAM) != 0) {
				kn->kn_hook32 |= NOTE_EXIT_MEMORY;
				switch (kn->kn_proc->p_lflag & P_JETSAM_MASK) {
				case P_JETSAM_VMPAGESHORTAGE:
					kn->kn_hook32 |= NOTE_EXIT_MEMORY_VMPAGESHORTAGE;
					break;
				case P_JETSAM_VMTHRASHING:
					kn->kn_hook32 |= NOTE_EXIT_MEMORY_VMTHRASHING;
					break;
				case P_JETSAM_FCTHRASHING:
					kn->kn_hook32 |= NOTE_EXIT_MEMORY_FCTHRASHING;
					break;
				case P_JETSAM_VNODE:
					kn->kn_hook32 |= NOTE_EXIT_MEMORY_VNODE;
					break;
				case P_JETSAM_HIWAT:
					kn->kn_hook32 |= NOTE_EXIT_MEMORY_HIWAT;
					break;
				case P_JETSAM_PID:
					kn->kn_hook32 |= NOTE_EXIT_MEMORY_PID;
					break;
				case P_JETSAM_IDLEEXIT:
					kn->kn_hook32 |= NOTE_EXIT_MEMORY_IDLE;
					break;
				}
			}
			if ((kn->kn_proc->p_csflags &
			    CS_KILLED) != 0) {
				kn->kn_hook32 |= NOTE_EXIT_CSERROR;
			}
		}
	}

	                                                       
	return kn->kn_fflags != 0;
}

static int
filt_proctouch(struct knote *kn, struct kevent_qos_s *kev)
{
	int res;

	proc_klist_lock();

	                                                                            
	kn->kn_sfflags = kev->fflags;

	                                                                        
	  
                                                                       
                        
    
	                                  

	res = (kn->kn_fflags != 0);

	proc_klist_unlock();

	return res;
}

static int
filt_procprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	int res = 0;

	proc_klist_lock();
	if (kn->kn_fflags) {
		knote_fill_kevent(kn, kev, kn->kn_hook32);
		kn->kn_hook32 = 0;
		res = 1;
	}
	proc_klist_unlock();
	return res;
}

SECURITY_READ_ONLY_EARLY(static struct filterops) proc_filtops = {
	.f_attach  = filt_procattach,
	.f_detach  = filt_procdetach,
	.f_event   = filt_procevent,
	.f_touch   = filt_proctouch,
	.f_process = filt_procprocess,
};

#pragma mark timer_filtops

struct filt_timer_params {
	uint64_t deadline;                             
                                                                                              
	uint64_t leeway;                                        
	uint64_t interval;                                                      
};

  
                                                                      
  
                                                              
                                                                  
                                    
                                                    
                                                  
                                                   
  
              
                                                                 
                                                    
  
               
                                 
  
              
                                                            
                                                          
  
                  
                                                                        
                
   
#define TIMER_IDLE       0x0
#define TIMER_ARMED      0x1
#define TIMER_FIRED      0x2
#define TIMER_IMMEDIATE  0x3
#define TIMER_STATE_MASK 0x3
#define TIMER_GEN_INC    0x4

static void
filt_timer_set_params(struct knote *kn, struct filt_timer_params *params)
{
	kn->kn_ext[0] = params->deadline;
	kn->kn_ext[1] = params->leeway;
	kn->kn_sdata  = params->interval;
}

  
                                              
  
                                                                         
  
         
                                                       
                                                           
  
          
                                                              
                                                                    
  
           
                                                    
                                                             
  
                                      
   
static int
filt_timervalidate(const struct kevent_qos_s *kev,
    struct filt_timer_params *params)
{
	  
                                                                      
   
                                                                        
                                    
                                                                          
                                                  
   
                                                                   
                                    
                                                                  
                                                      
                                                                                      
   
                                       
                                                                                            
                                                                      
                                                               
   
                                                               
                                                           
                                                                                       
                                                               
   
                                                       
                                     
                                                                                              
                                                                        
                                                                              
    

	uint64_t multiplier;

	boolean_t use_abstime = FALSE;

	switch (kev->fflags & (NOTE_SECONDS | NOTE_USECONDS | NOTE_NSECONDS | NOTE_MACHTIME)) {
	case NOTE_SECONDS:
		multiplier = NSEC_PER_SEC;
		break;
	case NOTE_USECONDS:
		multiplier = NSEC_PER_USEC;
		break;
	case NOTE_NSECONDS:
		multiplier = 1;
		break;
	case NOTE_MACHTIME:
		multiplier = 0;
		use_abstime = TRUE;
		break;
	case 0:                             
		multiplier = NSEC_PER_SEC / 1000;
		break;
	default:
		return EINVAL;
	}

	                                                          
	if (kev->fflags & NOTE_LEEWAY) {
		uint64_t leeway_abs;

		if (use_abstime) {
			leeway_abs = (uint64_t)kev->ext[1];
		} else {
			uint64_t leeway_ns;
			if (os_mul_overflow((uint64_t)kev->ext[1], multiplier, &leeway_ns)) {
				return ERANGE;
			}

			nanoseconds_to_absolutetime(leeway_ns, &leeway_abs);
		}

		params->leeway = leeway_abs;
	} else {
		params->leeway = 0;
	}

	if (kev->fflags & NOTE_ABSOLUTE) {
		uint64_t deadline_abs;

		if (use_abstime) {
			deadline_abs = (uint64_t)kev->data;
		} else {
			uint64_t calendar_deadline_ns;

			if (os_mul_overflow((uint64_t)kev->data, multiplier, &calendar_deadline_ns)) {
				return ERANGE;
			}

			                                                            

			clock_sec_t seconds;
			clock_nsec_t nanoseconds;

			  
                                                                   
     
                                                               
                                           
     
                                                          
                                                         
      
			clock_get_calendar_nanotime(&seconds, &nanoseconds);

			uint64_t calendar_now_ns = (uint64_t)seconds * NSEC_PER_SEC + nanoseconds;

			                                  
			if (calendar_now_ns < calendar_deadline_ns) {
				uint64_t interval_ns = calendar_deadline_ns - calendar_now_ns;
				uint64_t interval_abs;

				nanoseconds_to_absolutetime(interval_ns, &interval_abs);

				  
                                                             
                                                         
                                                
       

				if (kev->fflags & NOTE_MACH_CONTINUOUS_TIME) {
					clock_continuoustime_interval_to_deadline(interval_abs,
					    &deadline_abs);
				} else {
					clock_absolutetime_interval_to_deadline(interval_abs,
					    &deadline_abs);
				}
			} else {
				deadline_abs = 0;                                 
			}
		}

		params->deadline = deadline_abs;
		params->interval = 0;                                     
	} else if (kev->data < 0) {
		  
                                                     
    
                                                                       
                                                                
    
                                                                   
                                                                     
                                                                                   
    
                                                                                    
     

		params->deadline = 0;                         
		params->interval = 0;                    
	} else {
		uint64_t interval_abs = 0;

		if (use_abstime) {
			interval_abs = (uint64_t)kev->data;
		} else {
			uint64_t interval_ns;
			if (os_mul_overflow((uint64_t)kev->data, multiplier, &interval_ns)) {
				return ERANGE;
			}

			nanoseconds_to_absolutetime(interval_ns, &interval_abs);
		}

		uint64_t deadline = 0;

		if (kev->fflags & NOTE_MACH_CONTINUOUS_TIME) {
			clock_continuoustime_interval_to_deadline(interval_abs, &deadline);
		} else {
			clock_absolutetime_interval_to_deadline(interval_abs, &deadline);
		}

		params->deadline = deadline;
		params->interval = interval_abs;
	}

	return 0;
}

  
                                               
   
static void
filt_timerexpire(void *knx, void *state_on_arm)
{
	struct knote *kn = knx;

	uint32_t state = (uint32_t)(uintptr_t)state_on_arm;
	uint32_t fired_state = state ^ TIMER_ARMED ^ TIMER_FIRED;

	if (os_atomic_cmpxchg(&kn->kn_hook32, state, fired_state, relaxed)) {
		                                              
		                               
		struct kqueue *kq = knote_get_kq(kn);
		kqlock(kq);
		knote_activate(kq, kn, FILTER_ACTIVE);
		kqunlock(kq);
	} else {
		  
                                                                    
                                                             
     
	}
}

  
                                                                    
   
static bool
filt_timer_is_ready(struct knote *kn)
{
	uint64_t now, deadline = kn->kn_ext[0];

	if (deadline == 0) {
		return true;
	}

	if (kn->kn_sfflags & NOTE_MACH_CONTINUOUS_TIME) {
		now = mach_continuous_time();
	} else {
		now = mach_absolute_time();
	}
	return deadline <= now;
}

  
              
  
                                                                     
                                                               
   
static void
filt_timerarm(struct knote *kn)
{
	uint64_t deadline = kn->kn_ext[0];
	uint64_t leeway   = kn->kn_ext[1];
	uint32_t state;

	int filter_flags = kn->kn_sfflags;
	unsigned int timer_flags = 0;

	if (filter_flags & NOTE_CRITICAL) {
		timer_flags |= THREAD_CALL_DELAY_USER_CRITICAL;
	} else if (filter_flags & NOTE_BACKGROUND) {
		timer_flags |= THREAD_CALL_DELAY_USER_BACKGROUND;
	} else {
		timer_flags |= THREAD_CALL_DELAY_USER_NORMAL;
	}

	if (filter_flags & NOTE_LEEWAY) {
		timer_flags |= THREAD_CALL_DELAY_LEEWAY;
	}

	if (filter_flags & NOTE_MACH_CONTINUOUS_TIME) {
		timer_flags |= THREAD_CALL_CONTINUOUS;
	}

	  
                  
   
                                                                          
                                                                           
                                                                
   
                                                                           
                                                                           
                                                                         
                                                                        
   
                                                                           
                                                                         
   
                                                                          
                                   
   
                                                                        
                                     
    
	state = os_atomic_load(&kn->kn_hook32, relaxed);
	state &= ~TIMER_STATE_MASK;
	state += TIMER_GEN_INC + TIMER_ARMED;
	os_atomic_store(&kn->kn_hook32, state, relaxed);

	thread_call_enter_delayed_with_leeway(kn->kn_thcall,
	    (void *)(uintptr_t)state, deadline, leeway, timer_flags);
}

  
                                                                
  
                                                                             
                                                                      
                                                                          
               
   
static void
filt_timerfire_immediate(struct knote *kn)
{
	uint32_t state;

	static_assert(TIMER_IMMEDIATE == TIMER_STATE_MASK,
	    "validate that this atomic or will transition to IMMEDIATE");
	state = os_atomic_or_orig(&kn->kn_hook32, TIMER_IMMEDIATE, relaxed);

	if ((state & TIMER_STATE_MASK) == TIMER_ARMED) {
		thread_call_cancel(kn->kn_thcall);
	}
}

  
                                                                           
   
static int
filt_timerattach(struct knote *kn, struct kevent_qos_s *kev)
{
	thread_call_t callout;
	struct filt_timer_params params;
	int error;

	if ((error = filt_timervalidate(kev, &params)) != 0) {
		knote_set_error(kn, error);
		return 0;
	}

	callout = thread_call_allocate_with_options(filt_timerexpire,
	    (thread_call_param_t)kn, THREAD_CALL_PRIORITY_HIGH,
	    THREAD_CALL_OPTIONS_ONCE);

	if (NULL == callout) {
		knote_set_error(kn, ENOMEM);
		return 0;
	}

	filt_timer_set_params(kn, &params);
	kn->kn_thcall = callout;
	kn->kn_flags |= EV_CLEAR;
	os_atomic_store(&kn->kn_hook32, TIMER_IDLE, relaxed);

	                                      
	if (kn->kn_sfflags & NOTE_ABSOLUTE) {
		kn->kn_flags |= EV_ONESHOT;
	}

	if (filt_timer_is_ready(kn)) {
		os_atomic_store(&kn->kn_hook32, TIMER_IMMEDIATE, relaxed);
		return FILTER_ACTIVE;
	} else {
		filt_timerarm(kn);
		return 0;
	}
}

  
                                                             
   
static void
filt_timerdetach(struct knote *kn)
{
	__assert_only boolean_t freed;

	  
                                                                             
                    
    
	thread_call_cancel_wait(kn->kn_thcall);
	freed = thread_call_free(kn->kn_thcall);
	assert(freed);
}

  
                                                           
  
                                                            
                                                               
                                   
   
static int
filt_timertouch(struct knote *kn, struct kevent_qos_s *kev)
{
	struct filt_timer_params params;
	uint32_t changed_flags = (kn->kn_sfflags ^ kev->fflags);
	int error;

	if (changed_flags & NOTE_ABSOLUTE) {
		kev->flags |= EV_ERROR;
		kev->data = EINVAL;
		return 0;
	}

	if ((error = filt_timervalidate(kev, &params)) != 0) {
		kev->flags |= EV_ERROR;
		kev->data = error;
		return 0;
	}

	                                                     
	filt_timer_set_params(kn, &params);
	kn->kn_sfflags = kev->fflags;

	if (filt_timer_is_ready(kn)) {
		filt_timerfire_immediate(kn);
		return FILTER_ACTIVE | FILTER_UPDATE_REQ_QOS;
	} else {
		filt_timerarm(kn);
		return FILTER_UPDATE_REQ_QOS;
	}
}

  
                                                                   
  
                                                                   
                                                                     
                              
   
static int
filt_timerprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	uint32_t state = os_atomic_load(&kn->kn_hook32, relaxed);

	  
                                                                      
                                                                       
                                                    
   
                                                                           
                                                                        
                                        
    
	switch (state & TIMER_STATE_MASK) {
	case TIMER_IDLE:
	case TIMER_ARMED:
		  
                                                             
                            
     
		return 0;
	}

	os_atomic_store(&kn->kn_hook32, state & ~TIMER_STATE_MASK, relaxed);

	  
                                          
                                                 
   
                                                       
                                                               
                                                                       
                                           
    
	knote_fill_kevent(kn, kev, 1);
	kev->ext[0] = 0;
	                                                         

	if (kn->kn_sdata != 0) {
		  
                                                    
                                               
                     
    
                                               
                                               
     

		uint64_t now;

		if (kn->kn_sfflags & NOTE_MACH_CONTINUOUS_TIME) {
			now = mach_continuous_time();
		} else {
			now = mach_absolute_time();
		}

		uint64_t first_deadline = kn->kn_ext[0];
		uint64_t interval_abs   = kn->kn_sdata;
		uint64_t orig_arm_time  = first_deadline - interval_abs;

		assert(now > orig_arm_time);
		assert(now > first_deadline);

		uint64_t elapsed = now - orig_arm_time;

		uint64_t num_fired = elapsed / interval_abs;

		  
                                                        
                                                             
                                                       
                      
     
		assert(num_fired > 0);

		                                                        
		kev->data = (int64_t)num_fired;

		                                                                        
		if ((kn->kn_flags & EV_ONESHOT) == 0) {
			                                          
			uint64_t new_deadline = first_deadline + num_fired * interval_abs;

			assert(new_deadline > now);

			kn->kn_ext[0] = new_deadline;

			  
                                                             
                                                                 
      
			filt_timerarm(kn);
		}
	}

	return FILTER_ACTIVE;
}

SECURITY_READ_ONLY_EARLY(static struct filterops) timer_filtops = {
	.f_extended_codes = true,
	.f_attach   = filt_timerattach,
	.f_detach   = filt_timerdetach,
	.f_event    = filt_bad_event,
	.f_touch    = filt_timertouch,
	.f_process  = filt_timerprocess,
};

#pragma mark user_filtops

static int
filt_userattach(struct knote *kn, __unused struct kevent_qos_s *kev)
{
	if (kn->kn_sfflags & NOTE_TRIGGER) {
		kn->kn_hook32 = FILTER_ACTIVE;
	} else {
		kn->kn_hook32 = 0;
	}
	return kn->kn_hook32;
}

static int
filt_usertouch(struct knote *kn, struct kevent_qos_s *kev)
{
	uint32_t ffctrl;
	int fflags;

	ffctrl = kev->fflags & NOTE_FFCTRLMASK;
	fflags = kev->fflags & NOTE_FFLAGSMASK;
	switch (ffctrl) {
	case NOTE_FFNOP:
		break;
	case NOTE_FFAND:
		kn->kn_sfflags &= fflags;
		break;
	case NOTE_FFOR:
		kn->kn_sfflags |= fflags;
		break;
	case NOTE_FFCOPY:
		kn->kn_sfflags = fflags;
		break;
	}
	kn->kn_sdata = kev->data;

	if (kev->fflags & NOTE_TRIGGER) {
		kn->kn_hook32 = FILTER_ACTIVE;
	}
	return (int)kn->kn_hook32;
}

static int
filt_userprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	int result = (int)kn->kn_hook32;

	if (result) {
		                                                     
		knote_fill_kevent_with_sdata(kn, kev);
		kev->fflags = kn->kn_sfflags;
		if (kn->kn_flags & EV_CLEAR) {
			                                         
			kn->kn_hook32 = 0;
		}
	}

	return result;
}

SECURITY_READ_ONLY_EARLY(static struct filterops) user_filtops = {
	.f_extended_codes = true,
	.f_attach  = filt_userattach,
	.f_detach  = filt_no_detach,
	.f_event   = filt_bad_event,
	.f_touch   = filt_usertouch,
	.f_process = filt_userprocess,
};

#pragma mark workloop_filtops

#define EPREEMPTDISABLED (-1)

static inline void
filt_wllock(struct kqworkloop *kqwl)
{
	lck_spin_lock(&kqwl->kqwl_statelock);
}

static inline void
filt_wlunlock(struct kqworkloop *kqwl)
{
	lck_spin_unlock(&kqwl->kqwl_statelock);
}

  
                                                                          
  
                                                                 
                              
  
                                                                        
                                                                             
                                                                             
                                   
   
static inline bool
filt_wlturnstile_interlock_is_workq(struct kqworkloop *kqwl)
{
	return kqr_thread_requested_pending(&kqwl->kqwl_request);
}

static void
filt_wlupdate_inheritor(struct kqworkloop *kqwl, struct turnstile *ts,
    turnstile_update_flags_t flags)
{
	turnstile_inheritor_t inheritor = TURNSTILE_INHERITOR_NULL;
	workq_threadreq_t kqr = &kqwl->kqwl_request;

	  
                                                     
                                           
    
	assert(!filt_wlturnstile_interlock_is_workq(kqwl));

	if ((inheritor = kqwl->kqwl_owner)) {
		flags |= TURNSTILE_INHERITOR_THREAD;
	} else if ((inheritor = kqr_thread(kqr))) {
		flags |= TURNSTILE_INHERITOR_THREAD;
	}

	turnstile_update_inheritor(ts, inheritor, flags);
}

#define EVFILT_WORKLOOP_EFAULT_RETRY_COUNT 100
#define FILT_WLATTACH 0
#define FILT_WLTOUCH  1
#define FILT_WLDROP   2

__result_use_check
static int
filt_wlupdate(struct kqworkloop *kqwl, struct knote *kn,
    struct kevent_qos_s *kev, kq_index_t qos_index, int op)
{
	user_addr_t uaddr = CAST_USER_ADDR_T(kev->ext[EV_EXTIDX_WL_ADDR]);
	workq_threadreq_t kqr = &kqwl->kqwl_request;
	thread_t cur_owner, new_owner, extra_thread_ref = THREAD_NULL;
	kq_index_t cur_override = THREAD_QOS_UNSPECIFIED;
	int efault_retry = EVFILT_WORKLOOP_EFAULT_RETRY_COUNT;
	int action = KQWL_UTQ_NONE, error = 0;
	bool wl_inheritor_updated = false, needs_wake = false;
	uint64_t kdata = kev->ext[EV_EXTIDX_WL_VALUE];
	uint64_t mask = kev->ext[EV_EXTIDX_WL_MASK];
	uint64_t udata = 0;
	struct turnstile *ts = TURNSTILE_NULL;

	filt_wllock(kqwl);

again:
	new_owner = cur_owner = kqwl->kqwl_owner;

	  
            
   
                                                                            
                                                     
   
                                                                        
                       
   
                                                                          
                                           
   
                                                          
    
	if (uaddr) {
		  
                                            
                                           
                                   
     
		error = copyin_atomic64(uaddr, &udata);

		  
                                             
                                        
                                            
                                            
     
		switch (error) {
		case 0:
			break;
		case EFAULT:
			if (efault_retry-- > 0) {
				filt_wlunlock(kqwl);
				error = copyin_atomic64(uaddr, &udata);
				filt_wllock(kqwl);
				if (error == 0) {
					goto again;
				}
			}
			OS_FALLTHROUGH;
		default:
			goto out;
		}

		                                 
		kev->ext[EV_EXTIDX_WL_VALUE] = udata;

		if ((udata & mask) != (kdata & mask)) {
			error = ESTALE;
		} else if (kev->fflags & NOTE_WL_DISCOVER_OWNER) {
			  
                                                              
                                                                     
     
                                                             
      
			mach_port_name_t name = (mach_port_name_t)udata & ~0x3;
			if (name != MACH_PORT_NULL) {
				name = ipc_entry_name_mask(name);
				extra_thread_ref = port_name_to_thread(name,
				    PORT_TO_THREAD_IN_CURRENT_TASK);
				if (extra_thread_ref == THREAD_NULL) {
					error = EOWNERDEAD;
					goto out;
				}
				new_owner = extra_thread_ref;
			}
		}
	}

	if ((kev->fflags & NOTE_WL_END_OWNERSHIP) && new_owner == current_thread()) {
		new_owner = THREAD_NULL;
	}

	if (error == 0) {
		if ((kev->fflags & NOTE_WL_THREAD_REQUEST) && (kev->flags & EV_DELETE)) {
			action = KQWL_UTQ_SET_QOS_INDEX;
		} else if (qos_index && kqr->tr_kq_qos_index != qos_index) {
			action = KQWL_UTQ_SET_QOS_INDEX;
		}

		if (op == FILT_WLTOUCH) {
			  
                                                          
                                                                           
                      
      
			kn->kn_sfflags &= ~NOTE_WL_UPDATES_MASK;
			kn->kn_sfflags |= kev->fflags;
			if (kev->fflags & NOTE_WL_SYNC_WAKE) {
				needs_wake = (kn->kn_thread != THREAD_NULL);
			}
		} else if (op == FILT_WLDROP) {
			if ((kn->kn_sfflags & (NOTE_WL_SYNC_WAIT | NOTE_WL_SYNC_WAKE)) ==
			    NOTE_WL_SYNC_WAIT) {
				  
                                                                
                                   
       
				kn->kn_sfflags |= NOTE_WL_SYNC_WAKE;
				needs_wake = (kn->kn_thread != THREAD_NULL);
			}
		}
	}

	  
            
   
                                                                     
    

	if (cur_owner == new_owner && action == KQWL_UTQ_NONE && !needs_wake) {
		goto out;
	}

	kqlock(kqwl);

	                                                          
	if (new_owner == kqr_thread(kqr)) {
		new_owner = THREAD_NULL;
	}

	if (cur_owner != new_owner) {
		kqwl->kqwl_owner = new_owner;
		if (new_owner == extra_thread_ref) {
			                                               
			extra_thread_ref = THREAD_NULL;
		}
		cur_override = kqworkloop_override(kqwl);

		if (new_owner) {
			                                        
			if (cur_override != THREAD_QOS_UNSPECIFIED) {
				thread_add_kevent_override(new_owner, cur_override);
			}
			if (kqr_thread_requested_pending(kqr)) {
				if (action == KQWL_UTQ_NONE) {
					action = KQWL_UTQ_REDRIVE_EVENTS;
				}
			}
		} else {
			if (!kqr_thread_requested(kqr) && kqr->tr_kq_wakeup) {
				if (action == KQWL_UTQ_NONE) {
					action = KQWL_UTQ_REDRIVE_EVENTS;
				}
			}
		}
	}

	if (action != KQWL_UTQ_NONE) {
		kqworkloop_update_threads_qos(kqwl, action, qos_index);
	}

	ts = kqwl->kqwl_turnstile;
	if (cur_owner != new_owner && ts) {
		if (action == KQWL_UTQ_REDRIVE_EVENTS) {
			  
                                                       
                                                           
                                                     
      
			assert(filt_wlturnstile_interlock_is_workq(kqwl));
		} else if (filt_wlturnstile_interlock_is_workq(kqwl)) {
			workq_kern_threadreq_lock(kqwl->kqwl_p);
			workq_kern_threadreq_update_inheritor(kqwl->kqwl_p, kqr, new_owner,
			    ts, TURNSTILE_IMMEDIATE_UPDATE);
			workq_kern_threadreq_unlock(kqwl->kqwl_p);
			if (!filt_wlturnstile_interlock_is_workq(kqwl)) {
				  
                                                    
                                                                  
                                                   
       
				filt_wlupdate_inheritor(kqwl, ts, TURNSTILE_IMMEDIATE_UPDATE);
			}
			wl_inheritor_updated = true;
		} else {
			filt_wlupdate_inheritor(kqwl, ts, TURNSTILE_IMMEDIATE_UPDATE);
			wl_inheritor_updated = true;
		}

		  
                                                                        
                                                     
     
		if (wl_inheritor_updated) {
			turnstile_reference(ts);
		}
	}

	if (needs_wake && ts) {
		waitq_wakeup64_thread(&ts->ts_waitq, knote_filt_wev64(kn),
		    kn->kn_thread, THREAD_AWAKENED);
		if (op == FILT_WLATTACH || op == FILT_WLTOUCH) {
			disable_preemption();
			error = EPREEMPTDISABLED;
		}
	}

	kqunlock(kqwl);

out:
	  
            
   
                                                               
    
	filt_wlunlock(kqwl);

#if CONFIG_WORKLOOP_DEBUG
	KQWL_HISTORY_WRITE_ENTRY(kqwl, {
		.updater = current_thread(),
		.servicer = kqr_thread(kqr),                 
		.old_owner = cur_owner,
		.new_owner = new_owner,

		.kev_ident  = kev->ident,
		.error      = (int16_t)error,
		.kev_flags  = kev->flags,
		.kev_fflags = kev->fflags,

		.kev_mask   = mask,
		.kev_value  = kdata,
		.in_value   = udata,
	});
#endif                         

	if (wl_inheritor_updated) {
		turnstile_update_inheritor_complete(ts, TURNSTILE_INTERLOCK_NOT_HELD);
		turnstile_deallocate_safe(ts);
	}

	if (cur_owner && new_owner != cur_owner) {
		if (cur_override != THREAD_QOS_UNSPECIFIED) {
			thread_drop_kevent_override(cur_owner);
		}
		thread_deallocate_safe(cur_owner);
	}
	if (extra_thread_ref) {
		thread_deallocate_safe(extra_thread_ref);
	}
	return error;
}

  
                                                                                
                                                 
                                                             
                                                           
                                    
   
static inline void
filt_wlremember_last_update(struct knote *kn, struct kevent_qos_s *kev,
    int error)
{
	kn->kn_fflags = kev->fflags;
	kn->kn_sdata = error;
	memcpy(kn->kn_ext, kev->ext, sizeof(kev->ext));
}

static int
filt_wlupdate_sync_ipc(struct kqworkloop *kqwl, struct knote *kn,
    struct kevent_qos_s *kev, int op)
{
	user_addr_t uaddr = (user_addr_t) kev->ext[EV_EXTIDX_WL_ADDR];
	uint64_t kdata = kev->ext[EV_EXTIDX_WL_VALUE];
	uint64_t mask  = kev->ext[EV_EXTIDX_WL_MASK];
	uint64_t udata = 0;
	int efault_retry = EVFILT_WORKLOOP_EFAULT_RETRY_COUNT;
	int error = 0;

	if (op == FILT_WLATTACH) {
		(void)kqueue_alloc_turnstile(&kqwl->kqwl_kqueue);
	} else if (uaddr == 0) {
		return 0;
	}

	filt_wllock(kqwl);

again:

	  
                                                                            
    
	if (uaddr) {
		  
                                            
                                           
                                   
     
		error = copyin_atomic64(uaddr, &udata);

		  
                                             
                                        
                                            
                                            
     
		switch (error) {
		case 0:
			break;
		case EFAULT:
			if (efault_retry-- > 0) {
				filt_wlunlock(kqwl);
				error = copyin_atomic64(uaddr, &udata);
				filt_wllock(kqwl);
				if (error == 0) {
					goto again;
				}
			}
			OS_FALLTHROUGH;
		default:
			goto out;
		}

		kev->ext[EV_EXTIDX_WL_VALUE] = udata;
		kn->kn_ext[EV_EXTIDX_WL_VALUE] = udata;

		if ((udata & mask) != (kdata & mask)) {
			error = ESTALE;
			goto out;
		}
	}

	if (op == FILT_WLATTACH) {
		error = filt_wlattach_sync_ipc(kn);
		if (error == 0) {
			disable_preemption();
			error = EPREEMPTDISABLED;
		}
	}

out:
	filt_wlunlock(kqwl);
	return error;
}

static int
filt_wlattach(struct knote *kn, struct kevent_qos_s *kev)
{
	struct kqueue *kq = knote_get_kq(kn);
	struct kqworkloop *kqwl = (struct kqworkloop *)kq;
	int error = 0, result = 0;
	kq_index_t qos_index = 0;

	if (__improbable((kq->kq_state & KQ_WORKLOOP) == 0)) {
		error = ENOTSUP;
		goto out;
	}

	uint32_t command = (kn->kn_sfflags & NOTE_WL_COMMANDS_MASK);
	switch (command) {
	case NOTE_WL_THREAD_REQUEST:
		if (kn->kn_id != kqwl->kqwl_dynamicid) {
			error = EINVAL;
			goto out;
		}
		qos_index = _pthread_priority_thread_qos(kn->kn_qos);
		if (qos_index == THREAD_QOS_UNSPECIFIED) {
			error = ERANGE;
			goto out;
		}
		if (kqwl->kqwl_request.tr_kq_qos_index) {
			  
                                                                      
                                           
      
			error = EALREADY;
			goto out;
		}
		break;
	case NOTE_WL_SYNC_WAIT:
	case NOTE_WL_SYNC_WAKE:
		if (kn->kn_id == kqwl->kqwl_dynamicid) {
			error = EINVAL;
			goto out;
		}
		if ((kn->kn_flags & EV_DISABLE) == 0) {
			error = EINVAL;
			goto out;
		}
		if (kn->kn_sfflags & NOTE_WL_END_OWNERSHIP) {
			error = EINVAL;
			goto out;
		}
		break;

	case NOTE_WL_SYNC_IPC:
		if ((kn->kn_flags & EV_DISABLE) == 0) {
			error = EINVAL;
			goto out;
		}
		if (kn->kn_sfflags & (NOTE_WL_UPDATE_QOS | NOTE_WL_DISCOVER_OWNER)) {
			error = EINVAL;
			goto out;
		}
		break;
	default:
		error = EINVAL;
		goto out;
	}

	if (command == NOTE_WL_SYNC_IPC) {
		error = filt_wlupdate_sync_ipc(kqwl, kn, kev, FILT_WLATTACH);
	} else {
		error = filt_wlupdate(kqwl, kn, kev, qos_index, FILT_WLATTACH);
	}

	if (error == EPREEMPTDISABLED) {
		error = 0;
		result = FILTER_THREADREQ_NODEFEER;
	}
out:
	if (error) {
		                                                                   
		if (error == ESTALE && (kn->kn_sfflags & NOTE_WL_IGNORE_ESTALE)) {
			error = 0;
		}
		knote_set_error(kn, error);
		return result;
	}
	if (command == NOTE_WL_SYNC_WAIT) {
		return kevent_register_wait_prepare(kn, kev, result);
	}
	                                                                 
	if (command == NOTE_WL_THREAD_REQUEST) {
		  
                                                                     
                                                     
     
		kn->kn_flags |= EV_CLEAR;
		return result | FILTER_ACTIVE;
	}
	return result;
}

static void __dead2
filt_wlwait_continue(void *parameter, wait_result_t wr)
{
	struct _kevent_register *cont_args = parameter;
	struct kqworkloop *kqwl = cont_args->kqwl;

	kqlock(kqwl);
	if (filt_wlturnstile_interlock_is_workq(kqwl)) {
		workq_kern_threadreq_lock(kqwl->kqwl_p);
		turnstile_complete((uintptr_t)kqwl, &kqwl->kqwl_turnstile, NULL, TURNSTILE_WORKLOOPS);
		workq_kern_threadreq_unlock(kqwl->kqwl_p);
	} else {
		turnstile_complete((uintptr_t)kqwl, &kqwl->kqwl_turnstile, NULL, TURNSTILE_WORKLOOPS);
	}
	kqunlock(kqwl);

	turnstile_cleanup();

	if (wr == THREAD_INTERRUPTED) {
		cont_args->kev.flags |= EV_ERROR;
		cont_args->kev.data = EINTR;
	} else if (wr != THREAD_AWAKENED) {
		panic("Unexpected wait result: %d", wr);
	}

	kevent_register_wait_return(cont_args);
}

  
                                                                            
                                                     
   
static void __dead2
filt_wlpost_register_wait(struct uthread *uth, struct knote *kn,
    struct _kevent_register *cont_args)
{
	struct kqworkloop *kqwl = cont_args->kqwl;
	workq_threadreq_t kqr = &kqwl->kqwl_request;
	struct turnstile *ts;
	bool workq_locked = false;

	kqlock_held(kqwl);

	if (filt_wlturnstile_interlock_is_workq(kqwl)) {
		workq_kern_threadreq_lock(kqwl->kqwl_p);
		workq_locked = true;
	}

	ts = turnstile_prepare((uintptr_t)kqwl, &kqwl->kqwl_turnstile,
	    TURNSTILE_NULL, TURNSTILE_WORKLOOPS);

	if (workq_locked) {
		workq_kern_threadreq_update_inheritor(kqwl->kqwl_p,
		    &kqwl->kqwl_request, kqwl->kqwl_owner, ts,
		    TURNSTILE_DELAYED_UPDATE);
		if (!filt_wlturnstile_interlock_is_workq(kqwl)) {
			  
                                                       
                                            
      
			workq_kern_threadreq_unlock(kqwl->kqwl_p);
			workq_locked = false;
		}
	}
	if (!workq_locked) {
		  
                                                                        
                                          
     
		filt_wlupdate_inheritor(kqwl, ts, TURNSTILE_DELAYED_UPDATE);
	}

	thread_set_pending_block_hint(uth->uu_thread, kThreadWaitWorkloopSyncWait);
	waitq_assert_wait64(&ts->ts_waitq, knote_filt_wev64(kn),
	    THREAD_ABORTSAFE, TIMEOUT_WAIT_FOREVER);

	if (workq_locked) {
		workq_kern_threadreq_unlock(kqwl->kqwl_p);
	}

	thread_t thread = kqwl->kqwl_owner ?: kqr_thread(kqr);
	if (thread) {
		thread_reference(thread);
	}

	kevent_register_wait_block(ts, thread, filt_wlwait_continue, cont_args);
}

                                                                                           
void
kdp_workloop_sync_wait_find_owner(__assert_only thread_t thread,
    event64_t event, thread_waitinfo_t *waitinfo)
{
	extern zone_t thread_zone;
	struct knote *kn = (struct knote *)event;

	zone_require(knote_zone, kn);

	assert(kn->kn_thread == thread);

	struct kqueue *kq = knote_get_kq(kn);

	zone_require(kqworkloop_zone, kq);
	assert(kq->kq_state & KQ_WORKLOOP);

	struct kqworkloop *kqwl = (struct kqworkloop *)kq;
	workq_threadreq_t kqr = &kqwl->kqwl_request;

	thread_t kqwl_owner = kqwl->kqwl_owner;

	if (kqwl_owner != THREAD_NULL) {
		zone_require(thread_zone, kqwl_owner);
		waitinfo->owner = thread_tid(kqwl->kqwl_owner);
	} else if (kqr_thread_requested_pending(kqr)) {
		waitinfo->owner = STACKSHOT_WAITOWNER_THREQUESTED;
	} else if (kqr->tr_state >= WORKQ_TR_STATE_BINDING) {
		zone_require(thread_zone, kqr->tr_thread);
		waitinfo->owner = thread_tid(kqr->tr_thread);
	} else {
		waitinfo->owner = 0;
	}

	waitinfo->context = kqwl->kqwl_dynamicid;
}

static void
filt_wldetach(struct knote *kn)
{
	if (kn->kn_sfflags & NOTE_WL_SYNC_IPC) {
		filt_wldetach_sync_ipc(kn);
	} else if (kn->kn_thread) {
		kevent_register_wait_cleanup(kn);
	}
}

static int
filt_wlvalidate_kev_flags(struct knote *kn, struct kevent_qos_s *kev,
    thread_qos_t *qos_index)
{
	uint32_t new_commands = kev->fflags & NOTE_WL_COMMANDS_MASK;
	uint32_t sav_commands = kn->kn_sfflags & NOTE_WL_COMMANDS_MASK;

	if ((kev->fflags & NOTE_WL_DISCOVER_OWNER) && (kev->flags & EV_DELETE)) {
		return EINVAL;
	}
	if (kev->fflags & NOTE_WL_UPDATE_QOS) {
		if (kev->flags & EV_DELETE) {
			return EINVAL;
		}
		if (sav_commands != NOTE_WL_THREAD_REQUEST) {
			return EINVAL;
		}
		if (!(*qos_index = _pthread_priority_thread_qos(kev->qos))) {
			return ERANGE;
		}
	}

	switch (new_commands) {
	case NOTE_WL_THREAD_REQUEST:
		                                                
		if (sav_commands != NOTE_WL_THREAD_REQUEST) {
			return EINVAL;
		}
		break;

	case NOTE_WL_SYNC_WAIT:
		if (kev->fflags & NOTE_WL_END_OWNERSHIP) {
			return EINVAL;
		}
		goto sync_checks;

	case NOTE_WL_SYNC_WAKE:
sync_checks:
		if (!(sav_commands & (NOTE_WL_SYNC_WAIT | NOTE_WL_SYNC_WAKE))) {
			return EINVAL;
		}
		if ((kev->flags & (EV_ENABLE | EV_DELETE)) == EV_ENABLE) {
			return EINVAL;
		}
		break;

	case NOTE_WL_SYNC_IPC:
		if (sav_commands != NOTE_WL_SYNC_IPC) {
			return EINVAL;
		}
		if ((kev->flags & (EV_ENABLE | EV_DELETE)) == EV_ENABLE) {
			return EINVAL;
		}
		break;

	default:
		return EINVAL;
	}
	return 0;
}

static int
filt_wltouch(struct knote *kn, struct kevent_qos_s *kev)
{
	struct kqworkloop *kqwl = (struct kqworkloop *)knote_get_kq(kn);
	thread_qos_t qos_index = THREAD_QOS_UNSPECIFIED;
	int result = 0;

	int error = filt_wlvalidate_kev_flags(kn, kev, &qos_index);
	if (error) {
		goto out;
	}

	uint32_t command = kev->fflags & NOTE_WL_COMMANDS_MASK;
	if (command == NOTE_WL_SYNC_IPC) {
		error = filt_wlupdate_sync_ipc(kqwl, kn, kev, FILT_WLTOUCH);
	} else {
		error = filt_wlupdate(kqwl, kn, kev, qos_index, FILT_WLTOUCH);
		filt_wlremember_last_update(kn, kev, error);
	}
	if (error == EPREEMPTDISABLED) {
		error = 0;
		result = FILTER_THREADREQ_NODEFEER;
	}

out:
	if (error) {
		if (error == ESTALE && (kev->fflags & NOTE_WL_IGNORE_ESTALE)) {
			                                                            
			return result;
		}
		kev->flags |= EV_ERROR;
		kev->data = error;
		return result;
	}
	if (command == NOTE_WL_SYNC_WAIT && !(kn->kn_sfflags & NOTE_WL_SYNC_WAKE)) {
		return kevent_register_wait_prepare(kn, kev, result);
	}
	                                                                
	if (command == NOTE_WL_THREAD_REQUEST) {
		if (kev->fflags & NOTE_WL_UPDATE_QOS) {
			result |= FILTER_UPDATE_REQ_QOS;
		}
		result |= FILTER_ACTIVE;
	}
	return result;
}

static bool
filt_wlallow_drop(struct knote *kn, struct kevent_qos_s *kev)
{
	struct kqworkloop *kqwl = (struct kqworkloop *)knote_get_kq(kn);

	int error = filt_wlvalidate_kev_flags(kn, kev, NULL);
	if (error) {
		goto out;
	}

	uint32_t command = (kev->fflags & NOTE_WL_COMMANDS_MASK);
	if (command == NOTE_WL_SYNC_IPC) {
		error = filt_wlupdate_sync_ipc(kqwl, kn, kev, FILT_WLDROP);
	} else {
		error = filt_wlupdate(kqwl, kn, kev, 0, FILT_WLDROP);
		filt_wlremember_last_update(kn, kev, error);
	}
	assert(error != EPREEMPTDISABLED);

out:
	if (error) {
		if (error == ESTALE && (kev->fflags & NOTE_WL_IGNORE_ESTALE)) {
			return false;
		}
		kev->flags |= EV_ERROR;
		kev->data = error;
		return false;
	}
	return true;
}

static int
filt_wlprocess(struct knote *kn, struct kevent_qos_s *kev)
{
	struct kqworkloop *kqwl = (struct kqworkloop *)knote_get_kq(kn);
	int rc = 0;

	assert(kn->kn_sfflags & NOTE_WL_THREAD_REQUEST);

	kqlock(kqwl);

	if (kqwl->kqwl_owner) {
		  
                                                                      
                                                                     
                                 
    
                                                                 
                                                                     
     
		knote_activate(kqwl, kn, FILTER_ACTIVE);
	} else {
#if DEBUG || DEVELOPMENT
		if (kevent_debug_flags & KEVENT_PANIC_ON_NON_ENQUEUED_PROCESS) {
			  
                                             
      
#define DISPATCH_QUEUE_ENQUEUED 0x1ull
			user_addr_t addr = CAST_USER_ADDR_T(kn->kn_ext[EV_EXTIDX_WL_ADDR]);
			task_t t = current_task();
			uint64_t val;
			if (addr && task_is_active(t) && !task_is_halting(t) &&
			    copyin_atomic64(addr, &val) == 0 &&
			    val && (val & DISPATCH_QUEUE_ENQUEUED) == 0 &&
			    (val >> 48) != 0xdead && (val >> 48) != 0 && (val >> 48) != 0xffff) {
				panic("kevent: workloop %#016llx is not enqueued "
				    "(kn:%p dq_state:%#016llx kev.dq_state:%#016llx)",
				    kn->kn_udata, kn, val, kn->kn_ext[EV_EXTIDX_WL_VALUE]);
			}
		}
#endif
		knote_fill_kevent(kn, kev, 0);
		kev->fflags = kn->kn_sfflags;
		rc |= FILTER_ACTIVE;
	}

	kqunlock(kqwl);

	if (rc & FILTER_ACTIVE) {
		workq_thread_set_max_qos(kqwl->kqwl_p, &kqwl->kqwl_request);
	}
	return rc;
}

SECURITY_READ_ONLY_EARLY(static struct filterops) workloop_filtops = {
	.f_extended_codes = true,
	.f_attach  = filt_wlattach,
	.f_detach  = filt_wldetach,
	.f_event   = filt_bad_event,
	.f_touch   = filt_wltouch,
	.f_process = filt_wlprocess,
	.f_allow_drop = filt_wlallow_drop,
	.f_post_register_wait = filt_wlpost_register_wait,
};

#pragma mark - kqueues allocation and deallocation

   
                                   
  
         
                                                  
  
                           
                                  
  
                                       
                                                  
                                                                          
                                 
   
OS_OPTIONS(kqworkloop_dealloc_flags, unsigned,
    KQWL_DEALLOC_NONE               = 0x0000,
    KQWL_DEALLOC_SKIP_HASH_REMOVE   = 0x0001,
    );

static void
kqworkloop_dealloc(struct kqworkloop *, kqworkloop_dealloc_flags_t, uint32_t);

OS_NOINLINE OS_COLD OS_NORETURN
static void
kqworkloop_retain_panic(struct kqworkloop *kqwl, uint32_t previous)
{
	if (previous == 0) {
		panic("kq(%p) resurrection", kqwl);
	} else {
		panic("kq(%p) retain overflow", kqwl);
	}
}

OS_NOINLINE OS_COLD OS_NORETURN
static void
kqworkloop_release_panic(struct kqworkloop *kqwl)
{
	panic("kq(%p) over-release", kqwl);
}

OS_ALWAYS_INLINE
static inline bool
kqworkloop_try_retain(struct kqworkloop *kqwl)
{
	uint32_t old_ref, new_ref;
	os_atomic_rmw_loop(&kqwl->kqwl_retains, old_ref, new_ref, relaxed, {
		if (__improbable(old_ref == 0)) {
		        os_atomic_rmw_loop_give_up(return false);
		}
		if (__improbable(old_ref >= KQ_WORKLOOP_RETAINS_MAX)) {
		        kqworkloop_retain_panic(kqwl, old_ref);
		}
		new_ref = old_ref + 1;
	});
	return true;
}

OS_ALWAYS_INLINE
static inline void
kqworkloop_retain(struct kqworkloop *kqwl)
{
	uint32_t previous = os_atomic_inc_orig(&kqwl->kqwl_retains, relaxed);
	if (__improbable(previous == 0 || previous >= KQ_WORKLOOP_RETAINS_MAX)) {
		kqworkloop_retain_panic(kqwl, previous);
	}
}

OS_ALWAYS_INLINE
static inline void
kqueue_retain(kqueue_t kqu)
{
	if (kqu.kq->kq_state & KQ_DYNAMIC) {
		kqworkloop_retain(kqu.kqwl);
	}
}

OS_ALWAYS_INLINE
static inline void
kqworkloop_release_live(struct kqworkloop *kqwl)
{
	uint32_t refs = os_atomic_dec_orig(&kqwl->kqwl_retains, relaxed);
	if (__improbable(refs <= 1)) {
		kqworkloop_release_panic(kqwl);
	}
}

OS_ALWAYS_INLINE
static inline void
kqueue_release_live(kqueue_t kqu)
{
	if (kqu.kq->kq_state & KQ_DYNAMIC) {
		kqworkloop_release_live(kqu.kqwl);
	}
}

OS_ALWAYS_INLINE
static inline void
kqworkloop_release(struct kqworkloop *kqwl)
{
	uint32_t refs = os_atomic_dec_orig(&kqwl->kqwl_retains, relaxed);

	if (__improbable(refs <= 1)) {
		kqworkloop_dealloc(kqwl, KQWL_DEALLOC_NONE, refs - 1);
	}
}

OS_ALWAYS_INLINE
static inline void
kqueue_release(kqueue_t kqu)
{
	if (kqu.kq->kq_state & KQ_DYNAMIC) {
		kqworkloop_release(kqu.kqwl);
	}
}

   
                           
  
         
                                               
   
OS_NOINLINE
static void
kqueue_destroy(kqueue_t kqu, zone_t zone)
{
	  
                                                     
                                           
   
                                                                         
                                                                        
            
    
	waitq_set_deinit(&kqu.kq->kq_wqs);
	lck_spin_destroy(&kqu.kq->kq_lock, &kq_lck_grp);

	zfree(zone, kqu.kq);
}

   
                        
  
         
                                             
   
static kqueue_t
kqueue_init(kqueue_t kqu, waitq_set_prepost_hook_t *hook, int policy)
{
	waitq_set_init(&kqu.kq->kq_wqs, policy, NULL, hook);
	lck_spin_init(&kqu.kq->kq_lock, &kq_lck_grp, LCK_ATTR_NULL);
	return kqu;
}

#pragma mark kqfile allocation and deallocation

   
                           
  
         
                                               
  
              
                                                        
                                                        
                                                     
                                                      
                                                       
                                                         
                                             
  
                                                       
                                   
   
void
kqueue_dealloc(struct kqueue *kq)
{
	KNOTE_LOCK_CTX(knlc);
	struct proc *p = kq->kq_p;
	struct filedesc *fdp = p->p_fd;
	struct knote *kn;

	assert(kq && (kq->kq_state & (KQ_WORKLOOP | KQ_WORKQ)) == 0);

	proc_fdlock(p);
	for (int i = 0; i < fdp->fd_knlistsize; i++) {
		kn = SLIST_FIRST(&fdp->fd_knlist[i]);
		while (kn != NULL) {
			if (kq == knote_get_kq(kn)) {
				kqlock(kq);
				proc_fdunlock(p);
				if (knote_lock(kq, kn, &knlc, KNOTE_KQ_LOCK_ON_SUCCESS)) {
					knote_drop(kq, kn, &knlc);
				}
				proc_fdlock(p);
				                                     
				kn = SLIST_FIRST(&fdp->fd_knlist[i]);
				continue;
			}
			kn = SLIST_NEXT(kn, kn_link);
		}
	}

	knhash_lock(fdp);
	proc_fdunlock(p);

	if (fdp->fd_knhashmask != 0) {
		for (int i = 0; i < (int)fdp->fd_knhashmask + 1; i++) {
			kn = SLIST_FIRST(&fdp->fd_knhash[i]);
			while (kn != NULL) {
				if (kq == knote_get_kq(kn)) {
					kqlock(kq);
					knhash_unlock(fdp);
					if (knote_lock(kq, kn, &knlc, KNOTE_KQ_LOCK_ON_SUCCESS)) {
						knote_drop(kq, kn, &knlc);
					}
					knhash_lock(fdp);
					                                     
					kn = SLIST_FIRST(&fdp->fd_knhash[i]);
					continue;
				}
				kn = SLIST_NEXT(kn, kn_link);
			}
		}
	}
	knhash_unlock(fdp);

	kqueue_destroy(kq, kqfile_zone);
}

   
                         
  
         
                     
   
struct kqueue *
kqueue_alloc(struct proc *p)
{
	struct kqfile *kqf;

	  
                                                            
                                                    
                                               
    
	kqf = zalloc_flags(kqfile_zone, Z_WAITOK | Z_ZERO);
	kqf->kqf_p = p;
	TAILQ_INIT_AFTER_BZERO(&kqf->kqf_queue);
	TAILQ_INIT_AFTER_BZERO(&kqf->kqf_suppressed);

	return kqueue_init(kqf, NULL, SYNC_POLICY_FIFO | SYNC_POLICY_PREPOST).kq;
}

   
                            
  
         
                                                         
   
int
kqueue_internal(struct proc *p, fp_allocfn_t fp_zalloc, void *cra, int32_t *retval)
{
	struct kqueue *kq;
	struct fileproc *fp;
	int fd, error;

	error = falloc_withalloc(p, &fp, &fd, vfs_context_current(), fp_zalloc, cra);
	if (error) {
		return error;
	}

	kq = kqueue_alloc(p);
	if (kq == NULL) {
		fp_free(p, fd, fp);
		return ENOMEM;
	}

	fp->f_flag = FREAD | FWRITE;
	fp->f_ops = &kqueueops;
	fp->f_data = kq;
	fp->f_lflags |= FG_CONFINED;

	proc_fdlock(p);
	*fdflags(p, fd) |= UF_EXCLOSE | UF_FORKCLOSE;
	procfdtbl_releasefd(p, fd, NULL);
	fp_drop(p, fd, fp, 1);
	proc_fdunlock(p);

	*retval = fd;
	return error;
}

   
                   
  
         
                      
   
int
kqueue(struct proc *p, __unused struct kqueue_args *uap, int32_t *retval)
{
	return kqueue_internal(p, fileproc_alloc_init, NULL, retval);
}

#pragma mark kqworkq allocation and deallocation

   
                            
  
         
                                  
  
              
                                                                   
                                                                              
                                                                         
   
void
kqworkq_dealloc(struct kqworkq *kqwq)
{
	kqueue_destroy(kqwq, kqworkq_zone);
}

   
                          
  
         
                                
  
              
                                            
                                                                   
   
OS_NOINLINE
static struct kqworkq *
kqworkq_alloc(struct proc *p, unsigned int flags)
{
	struct kqworkq *kqwq, *tmp;

	kqwq = zalloc_flags(kqworkq_zone, Z_WAITOK | Z_ZERO);

	assert((flags & KEVENT_FLAG_LEGACY32) == 0);
	if (flags & KEVENT_FLAG_LEGACY64) {
		kqwq->kqwq_state = KQ_WORKQ | KQ_KEV64;
	} else {
		kqwq->kqwq_state = KQ_WORKQ | KQ_KEV_QOS;
	}
	kqwq->kqwq_p = p;

	for (int i = 0; i < KQWQ_NBUCKETS; i++) {
		TAILQ_INIT_AFTER_BZERO(&kqwq->kqwq_queue[i]);
		TAILQ_INIT_AFTER_BZERO(&kqwq->kqwq_suppressed[i]);
	}
	for (int i = 0; i < KQWQ_NBUCKETS; i++) {
		  
                                                                  
                                                                
                                                               
                                                                 
                                              
    
                                                               
                                                        
                  
     
		kqwq->kqwq_request[i].tr_state = WORKQ_TR_STATE_IDLE;
		kqwq->kqwq_request[i].tr_flags = WORKQ_TR_FLAG_KEVENT;
		if (i != KQWQ_QOS_MANAGER) {
			kqwq->kqwq_request[i].tr_flags |= WORKQ_TR_FLAG_OVERCOMMIT;
		}
		kqwq->kqwq_request[i].tr_kq_qos_index = (kq_index_t)i;
	}

	kqueue_init(kqwq, &kqwq->kqwq_waitq_hook, SYNC_POLICY_FIFO);

	if (!os_atomic_cmpxchgv(&p->p_fd->fd_wqkqueue, NULL, kqwq, &tmp, release)) {
		kqworkq_dealloc(kqwq);
		return tmp;
	}

	return kqwq;
}

#pragma mark kqworkloop allocation and deallocation

#define KQ_HASH(val, mask)  (((val) ^ (val >> 8)) & (mask))
#define CONFIG_KQ_HASHSIZE  CONFIG_KN_HASHSIZE

OS_ALWAYS_INLINE
static inline void
kqhash_lock(struct filedesc *fdp)
{
	lck_mtx_lock_spin_always(&fdp->fd_kqhashlock);
}

OS_ALWAYS_INLINE
static inline void
kqhash_unlock(struct filedesc *fdp)
{
	lck_mtx_unlock(&fdp->fd_kqhashlock);
}

OS_ALWAYS_INLINE
static inline void
kqworkloop_hash_insert_locked(struct filedesc *fdp, kqueue_id_t id,
    struct kqworkloop *kqwl)
{
	struct kqwllist *list = &fdp->fd_kqhash[KQ_HASH(id, fdp->fd_kqhashmask)];
	LIST_INSERT_HEAD(list, kqwl, kqwl_hashlink);
}

OS_ALWAYS_INLINE
static inline struct kqworkloop *
kqworkloop_hash_lookup_locked(struct filedesc *fdp, kqueue_id_t id)
{
	struct kqwllist *list = &fdp->fd_kqhash[KQ_HASH(id, fdp->fd_kqhashmask)];
	struct kqworkloop *kqwl;

	LIST_FOREACH(kqwl, list, kqwl_hashlink) {
		if (kqwl->kqwl_dynamicid == id) {
			return kqwl;
		}
	}
	return NULL;
}

static struct kqworkloop *
kqworkloop_hash_lookup_and_retain(struct filedesc *fdp, kqueue_id_t kq_id)
{
	struct kqworkloop *kqwl = NULL;

	kqhash_lock(fdp);
	if (__probable(fdp->fd_kqhash)) {
		kqwl = kqworkloop_hash_lookup_locked(fdp, kq_id);
		if (kqwl && !kqworkloop_try_retain(kqwl)) {
			kqwl = NULL;
		}
	}
	kqhash_unlock(fdp);
	return kqwl;
}

OS_NOINLINE
static void
kqworkloop_hash_init(struct filedesc *fdp)
{
	struct kqwllist *alloc_hash;
	u_long alloc_mask;

	kqhash_unlock(fdp);
	alloc_hash = hashinit(CONFIG_KQ_HASHSIZE, M_KQUEUE, &alloc_mask);
	kqhash_lock(fdp);

	                            
	if (__probable(fdp->fd_kqhashmask == 0)) {
		fdp->fd_kqhash = alloc_hash;
		fdp->fd_kqhashmask = alloc_mask;
	} else {
		kqhash_unlock(fdp);
		hashdestroy(alloc_hash, M_KQUEUE, alloc_mask);
		kqhash_lock(fdp);
	}
}

   
                               
  
         
                                 
  
              
                                                                        
                                                 
  
                                   
  
               
                                                                       
                       
  
                     
                                                                     
                                                                          
                                                                            
   
static void
kqworkloop_dealloc(struct kqworkloop *kqwl, kqworkloop_dealloc_flags_t flags,
    uint32_t current_ref)
{
	thread_t cur_owner;

	if (__improbable(current_ref > 1)) {
		kqworkloop_release_panic(kqwl);
	}
	assert(kqwl->kqwl_retains == current_ref);

	                                             
	os_atomic_thread_fence(acquire);

	cur_owner = kqwl->kqwl_owner;
	if (cur_owner) {
		if (kqworkloop_override(kqwl) != THREAD_QOS_UNSPECIFIED) {
			thread_drop_kevent_override(cur_owner);
		}
		thread_deallocate(cur_owner);
		kqwl->kqwl_owner = THREAD_NULL;
	}

	if (kqwl->kqwl_state & KQ_HAS_TURNSTILE) {
		struct turnstile *ts;
		turnstile_complete((uintptr_t)kqwl, &kqwl->kqwl_turnstile,
		    &ts, TURNSTILE_WORKLOOPS);
		turnstile_cleanup();
		turnstile_deallocate(ts);
	}

	if ((flags & KQWL_DEALLOC_SKIP_HASH_REMOVE) == 0) {
		struct filedesc *fdp = kqwl->kqwl_p->p_fd;

		kqhash_lock(fdp);
		LIST_REMOVE(kqwl, kqwl_hashlink);
		kqhash_unlock(fdp);
	}

	assert(TAILQ_EMPTY(&kqwl->kqwl_suppressed));
	assert(kqwl->kqwl_owner == THREAD_NULL);
	assert(kqwl->kqwl_turnstile == TURNSTILE_NULL);

	lck_spin_destroy(&kqwl->kqwl_statelock, &kq_lck_grp);
	kqueue_destroy(kqwl, kqworkloop_zone);
}

   
                             
  
         
                               
   
static void
kqworkloop_init(struct kqworkloop *kqwl, proc_t p,
    kqueue_id_t id, workq_threadreq_param_t *trp)
{
	kqwl->kqwl_state     = KQ_WORKLOOP | KQ_DYNAMIC | KQ_KEV_QOS;
	kqwl->kqwl_retains   = 1;                                 
	kqwl->kqwl_dynamicid = id;
	kqwl->kqwl_p         = p;
	if (trp) {
		kqwl->kqwl_params = trp->trp_value;
	}

	workq_tr_flags_t tr_flags = WORKQ_TR_FLAG_WORKLOOP;
	if (trp) {
		if (trp->trp_flags & TRP_PRIORITY) {
			tr_flags |= WORKQ_TR_FLAG_WL_OUTSIDE_QOS;
		}
		if (trp->trp_flags) {
			tr_flags |= WORKQ_TR_FLAG_WL_PARAMS;
		}
	}
	kqwl->kqwl_request.tr_state = WORKQ_TR_STATE_IDLE;
	kqwl->kqwl_request.tr_flags = tr_flags;

	for (int i = 0; i < KQWL_NBUCKETS; i++) {
		TAILQ_INIT_AFTER_BZERO(&kqwl->kqwl_queue[i]);
	}
	TAILQ_INIT_AFTER_BZERO(&kqwl->kqwl_suppressed);

	lck_spin_init(&kqwl->kqwl_statelock, &kq_lck_grp, LCK_ATTR_NULL);

	kqueue_init(kqwl, &kqwl->kqwl_waitq_hook, SYNC_POLICY_FIFO);
}

   
                                     
  
         
                                                                          
  
           
                  
                             
                                                                               
                                                                               
                            
   
static int
kqworkloop_get_or_create(struct proc *p, kqueue_id_t id,
    workq_threadreq_param_t *trp, unsigned int flags, struct kqworkloop **kqwlp)
{
	struct filedesc *fdp = p->p_fd;
	struct kqworkloop *alloc_kqwl = NULL;
	struct kqworkloop *kqwl = NULL;
	int error = 0;

	assert(!trp || (flags & KEVENT_FLAG_DYNAMIC_KQ_MUST_NOT_EXIST));

	if (id == 0 || id == (kqueue_id_t)-1) {
		return EINVAL;
	}

	for (;;) {
		kqhash_lock(fdp);
		if (__improbable(fdp->fd_kqhash == NULL)) {
			kqworkloop_hash_init(fdp);
		}

		kqwl = kqworkloop_hash_lookup_locked(fdp, id);
		if (kqwl) {
			if (__improbable(flags & KEVENT_FLAG_DYNAMIC_KQ_MUST_NOT_EXIST)) {
				  
                                                                 
                                                            
                                                   
       
				error = EEXIST;
				break;
			}

			if (__probable(kqworkloop_try_retain(kqwl))) {
				  
                                      
       
				*kqwlp = kqwl;
				error = 0;
				break;
			}
		}

		if (__improbable(flags & KEVENT_FLAG_DYNAMIC_KQ_MUST_EXIST)) {
			error = ENOENT;
			break;
		}

		  
                                             
    
                                                                         
                     
    
                                                                        
                                               
     
		if (__probable(alloc_kqwl == NULL)) {
			alloc_kqwl = zalloc_flags(kqworkloop_zone, Z_NOWAIT | Z_ZERO);
		}
		if (__probable(alloc_kqwl)) {
			kqworkloop_init(alloc_kqwl, p, id, trp);
			kqworkloop_hash_insert_locked(fdp, id, alloc_kqwl);
			kqhash_unlock(fdp);
			*kqwlp = alloc_kqwl;
			return 0;
		}

		  
                                                            
                                                               
                             
     
		kqhash_unlock(fdp);

		alloc_kqwl = zalloc_flags(kqworkloop_zone, Z_WAITOK | Z_ZERO);
	}

	kqhash_unlock(fdp);

	if (__improbable(alloc_kqwl)) {
		zfree(kqworkloop_zone, alloc_kqwl);
	}

	return error;
}

#pragma mark - knotes

static int
filt_no_attach(struct knote *kn, __unused struct kevent_qos_s *kev)
{
	knote_set_error(kn, ENOTSUP);
	return 0;
}

static void
filt_no_detach(__unused struct knote *kn)
{
}

static int __dead2
filt_bad_event(struct knote *kn, long hint)
{
	panic("%s[%d](%p, %ld)", __func__, kn->kn_filter, kn, hint);
}

static int __dead2
filt_bad_touch(struct knote *kn, struct kevent_qos_s *kev)
{
	panic("%s[%d](%p, %p)", __func__, kn->kn_filter, kn, kev);
}

static int __dead2
filt_bad_process(struct knote *kn, struct kevent_qos_s *kev)
{
	panic("%s[%d](%p, %p)", __func__, kn->kn_filter, kn, kev);
}

  
                                                                   
  
                                 
                           
                            
                                                               
                                                                   
   
void
knotes_dealloc(proc_t p)
{
	struct filedesc *fdp = p->p_fd;
	struct kqueue *kq;
	struct knote *kn;
	struct  klist *kn_hash = NULL;
	u_long kn_hashmask;
	int i;

	                                              
	if (fdp->fd_knlistsize > 0) {
		for (i = 0; i < fdp->fd_knlistsize; i++) {
			while ((kn = SLIST_FIRST(&fdp->fd_knlist[i])) != NULL) {
				kq = knote_get_kq(kn);
				kqlock(kq);
				proc_fdunlock(p);
				knote_drop(kq, kn, NULL);
				proc_fdlock(p);
			}
		}
		                    
		kheap_free(KM_KQUEUE, fdp->fd_knlist,
		    fdp->fd_knlistsize * sizeof(struct klist *));
	}
	fdp->fd_knlistsize = 0;

	knhash_lock(fdp);
	proc_fdunlock(p);

	                                             
	if (fdp->fd_knhashmask != 0) {
		for (i = 0; i <= (int)fdp->fd_knhashmask; i++) {
			while ((kn = SLIST_FIRST(&fdp->fd_knhash[i])) != NULL) {
				kq = knote_get_kq(kn);
				kqlock(kq);
				knhash_unlock(fdp);
				knote_drop(kq, kn, NULL);
				knhash_lock(fdp);
			}
		}
		kn_hash = fdp->fd_knhash;
		kn_hashmask = fdp->fd_knhashmask;
		fdp->fd_knhashmask = 0;
		fdp->fd_knhash = NULL;
	}

	knhash_unlock(fdp);

	if (kn_hash) {
		hashdestroy(kn_hash, M_KQUEUE, kn_hashmask);
	}

	proc_fdlock(p);
}

  
                                                                      
                        
  
                                 
                           
                                                               
                                                                   
   
void
kqworkloops_dealloc(proc_t p)
{
	struct filedesc *fdp = p->p_fd;
	struct kqworkloop *kqwl, *kqwln;
	struct kqwllist tofree;

	if (!(fdp->fd_flags & FD_WORKLOOP)) {
		return;
	}

	kqhash_lock(fdp);

	if (fdp->fd_kqhashmask == 0) {
		kqhash_unlock(fdp);
		return;
	}

	LIST_INIT(&tofree);

	for (size_t i = 0; i <= fdp->fd_kqhashmask; i++) {
		LIST_FOREACH_SAFE(kqwl, &fdp->fd_kqhash[i], kqwl_hashlink, kqwln) {
			  
                                                         
                                                         
                                     
      
			assert(kqwl->kqwl_params);
			LIST_REMOVE(kqwl, kqwl_hashlink);
			LIST_INSERT_HEAD(&tofree, kqwl, kqwl_hashlink);
		}
	}

	kqhash_unlock(fdp);

	LIST_FOREACH_SAFE(kqwl, &tofree, kqwl_hashlink, kqwln) {
		kqworkloop_dealloc(kqwl, KQWL_DEALLOC_SKIP_HASH_REMOVE, 1);
	}
}

static int
kevent_register_validate_priority(struct kqueue *kq, struct knote *kn,
    struct kevent_qos_s *kev)
{
	                                                                     
	if (kev->flags & (EV_DISABLE | EV_DELETE)) {
		return 0;
	}

	if (kq->kq_state & KQ_WORKLOOP) {
		  
                                                                       
                       
    
                                                                  
                                                                         
                                                                     
    
                                                                
     
		if (!_pthread_priority_thread_qos(kn ? kn->kn_qos : kev->qos)) {
			return ERANGE;
		}
	}

	return 0;
}

  
                                               
  
                                                                          
                                               
   
static int
kevent_register_wait_prepare(struct knote *kn, struct kevent_qos_s *kev, int rc)
{
	thread_t thread = current_thread();

	assert(knote_fops(kn)->f_extended_codes);

	if (kn->kn_thread == NULL) {
		thread_reference(thread);
		kn->kn_thread = thread;
	} else if (kn->kn_thread != thread) {
		  
                                                      
                                                
     
		kev->flags |= EV_ERROR;
		kev->data = EXDEV;
		return 0;
	}

	return FILTER_REGISTER_WAIT | rc;
}

  
                                                                             
                                                                    
   
static void
kevent_register_wait_cleanup(struct knote *kn)
{
	thread_t thread = kn->kn_thread;
	kn->kn_thread = NULL;
	thread_deallocate(thread);
}

  
                                                                          
   
static void
kevent_register_wait_block(struct turnstile *ts, thread_t thread,
    thread_continue_t cont, struct _kevent_register *cont_args)
{
	turnstile_update_inheritor_complete(ts, TURNSTILE_INTERLOCK_HELD);
	kqunlock(cont_args->kqwl);
	cont_args->handoff_thread = thread;
	thread_handoff_parameter(thread, cont, cont_args, THREAD_HANDOFF_NONE);
}

  
                                                                            
   
static void
kevent_register_wait_return(struct _kevent_register *cont_args)
{
	struct kqworkloop *kqwl = cont_args->kqwl;
	struct kevent_qos_s *kev = &cont_args->kev;
	int error = 0;

	if (cont_args->handoff_thread) {
		thread_deallocate(cont_args->handoff_thread);
	}

	if (kev->flags & (EV_ERROR | EV_RECEIPT)) {
		if ((kev->flags & EV_ERROR) == 0) {
			kev->flags |= EV_ERROR;
			kev->data = 0;
		}
		error = kevent_modern_copyout(kev, &cont_args->ueventlist);
		if (error == 0) {
			cont_args->eventout++;
		}
	}

	kqworkloop_release(kqwl);
	if (error == 0) {
		*(int32_t *)&current_uthread()->uu_rval = cont_args->eventout;
	}
	unix_syscall_return(error);
}

  
                                                
  
                                                 
                                         
  
                                               
                                              
                                             
  
                             
                                         
   

int
kevent_register(struct kqueue *kq, struct kevent_qos_s *kev,
    struct knote **kn_out)
{
	struct proc *p = kq->kq_p;
	const struct filterops *fops;
	struct knote *kn = NULL;
	int result = 0, error = 0;
	unsigned short kev_flags = kev->flags;
	KNOTE_LOCK_CTX(knlc);

	if (__probable(kev->filter < 0 && kev->filter + EVFILT_SYSCOUNT >= 0)) {
		fops = sysfilt_ops[~kev->filter];                            
	} else {
		error = EINVAL;
		goto out;
	}

	                                                                    
	if (__improbable((kev->flags & EV_VANISHED) &&
	    (kev->flags & (EV_ADD | EV_DISPATCH2)) != (EV_ADD | EV_DISPATCH2))) {
		error = EINVAL;
		goto out;
	}

	                                                      
	if (kev->flags & EV_DELETE) {
		kev->flags &= ~EV_ADD;
	}
	if (kev->flags & EV_DISABLE) {
		kev->flags &= ~EV_ENABLE;
	}

	if (kq->kq_state & KQ_WORKLOOP) {
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWL_REGISTER),
		    ((struct kqworkloop *)kq)->kqwl_dynamicid,
		    kev->udata, kev->flags, kev->filter);
	} else if (kq->kq_state & KQ_WORKQ) {
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWQ_REGISTER),
		    0, kev->udata, kev->flags, kev->filter);
	} else {
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQ_REGISTER),
		    VM_KERNEL_UNSLIDE_OR_PERM(kq),
		    kev->udata, kev->flags, kev->filter);
	}

restart:
	                                                       
	kn = kq_find_knote_and_kq_lock(kq, kev, fops->f_isfd, p);
	error = kevent_register_validate_priority(kq, kn, kev);
	result = 0;
	if (error) {
		goto out;
	}

	if (kn == NULL && (kev->flags & EV_ADD) == 0) {
		  
                                            
     

		if ((kev_flags & EV_ADD) && (kev_flags & EV_DELETE) &&
		    (kq->kq_state & KQ_WORKLOOP)) {
			  
                                                                   
                                                                  
               
      
		} else {
			error = ENOENT;
		}
		goto out;
	} else if (kn == NULL) {
		  
                                                      
     

		struct fileproc *knote_fp = NULL;

		                                             
		if (fops->f_isfd) {
			if ((error = fp_lookup(p, (int)kev->ident, &knote_fp, 0)) != 0) {
				goto out;
			}
		}

		kn = knote_alloc();
		if (kn == NULL) {
			error = ENOMEM;
			if (knote_fp != NULL) {
				fp_drop(p, (int)kev->ident, knote_fp, 0);
			}
			goto out;
		}

		kn->kn_fp = knote_fp;
		kn->kn_is_fd = fops->f_isfd;
		kn->kn_kq_packed = VM_PACK_POINTER((vm_offset_t)kq, KNOTE_KQ_PACKED);
		kn->kn_status = 0;

		                                  
		if (kev->flags & EV_VANISHED) {
			kev->flags &= ~EV_VANISHED;
			kn->kn_status |= KN_REQVANISH;
		}

		                                                             
		if (kev->flags & EV_DISABLE) {
			kn->kn_status |= KN_DISABLED;
		}

		  
                                     
                                     
                                      
                                
    
                                              
                                            
                                                      
     
		kn->kn_kevent  = *(struct kevent_internal_s *)kev;
		kn->kn_sfflags = kev->fflags;
		kn->kn_filtid  = (uint8_t)~kev->filter;
		kn->kn_fflags  = 0;
		knote_reset_priority(kq, kn, kev->qos);

		                                                
		error = kq_add_knote(kq, kn, &knlc, p);
		if (error) {
			knote_free(kn);
			if (knote_fp != NULL) {
				fp_drop(p, (int)kev->ident, knote_fp, 0);
			}

			if (error == ERESTART) {
				goto restart;
			}
			goto out;
		}

		                                             

		  
                                                                          
                                                                      
                                     
     
		result = fops->f_attach(kn, kev);
		if (result && !knote_fops(kn)->f_extended_codes) {
			result = FILTER_ACTIVE;
		}

		kqlock(kq);

		if (result & FILTER_THREADREQ_NODEFEER) {
			enable_preemption();
		}

		if (kn->kn_flags & EV_ERROR) {
			  
                                          
      
			kn->kn_filtid = EVFILTID_DETACHED;
			error = (int)kn->kn_sdata;
			knote_drop(kq, kn, &knlc);
			result = 0;
			goto out;
		}

		  
                                              
    
                                                    
    
                                            
                                                
     
		if ((kn->kn_qos & _PTHREAD_PRIORITY_OVERCOMMIT_FLAG) &&
		    (kq->kq_state & KQ_WORKLOOP)) {
			kqworkloop_set_overcommit((struct kqworkloop *)kq);
		}
	} else if (!knote_lock(kq, kn, &knlc, KNOTE_KQ_LOCK_ON_SUCCESS)) {
		  
                                                              
                                    
     

		goto restart;
	} else if (kev->flags & EV_DELETE) {
		  
                               
    
                                                             
    
                                                                       
                                                                    
                                 
     

		if (knote_fops(kn)->f_allow_drop) {
			bool drop;

			kqunlock(kq);
			drop = knote_fops(kn)->f_allow_drop(kn, kev);
			kqlock(kq);

			if (!drop) {
				goto out_unlock;
			}
		}

		if ((kev->flags & EV_ENABLE) == 0 &&
		    (kn->kn_flags & EV_DISPATCH2) == EV_DISPATCH2 &&
		    (kn->kn_status & KN_DISABLED) != 0) {
			kn->kn_status |= KN_DEFERDELETE;
			error = EINPROGRESS;
			goto out_unlock;
		}

		knote_drop(kq, kn, &knlc);
		goto out;
	} else {
		  
                                      
    
                                                                    
                                                   
    
                                                                    
                                                       
    
                                         
                                                   
     

		if (kn->kn_status & (KN_DEFERDELETE | KN_VANISHED)) {
			if (kev->flags & EV_ENABLE) {
				result = FILTER_ACTIVE;
			}
		} else {
			kqunlock(kq);
			result = filter_call(knote_fops(kn), f_touch(kn, kev));
			kqlock(kq);
			if (result & FILTER_THREADREQ_NODEFEER) {
				enable_preemption();
			}
		}

		if (kev->flags & EV_ERROR) {
			result = 0;
			goto out_unlock;
		}

		if ((kn->kn_flags & EV_UDATA_SPECIFIC) == 0 &&
		    kn->kn_udata != kev->udata) {
			                                                   
			os_atomic_store_wide(&kn->kn_udata, kev->udata, relaxed);
		}
		if ((kev->flags & EV_DISABLE) && !(kn->kn_status & KN_DISABLED)) {
			kn->kn_status |= KN_DISABLED;
			knote_dequeue(kq, kn);
		}
	}

	                             
	knote_apply_touch(kq, kn, kev, result);

out_unlock:
	  
                                                   
                                                    
                                                   
    
	if (result & FILTER_REGISTER_WAIT) {
		knote_unlock(kq, kn, &knlc, KNOTE_KQ_LOCK_ALWAYS);
		*kn_out = kn;
	} else {
		knote_unlock(kq, kn, &knlc, KNOTE_KQ_UNLOCK);
	}

out:
	                                            
	if (error) {
		kev->flags |= EV_ERROR;
		kev->data = error;
	}
	return result;
}

  
                                            
  
                                                     
                                                       
                                                           
  
                                                          
                                                          
                                                           
                                  
  
                                                           
                                                         
                                                        
                                       
  
                                          
                                                       
   
static int
knote_process(struct knote *kn, kevent_ctx_t kectx,
    kevent_callback_t callback)
{
	struct kevent_qos_s kev;
	struct kqueue *kq = knote_get_kq(kn);
	KNOTE_LOCK_CTX(knlc);
	int result = FILTER_ACTIVE;
	int error = 0;
	bool drop = false;

	  
                                
                                                          
    
	assert(kn->kn_status & KN_QUEUED);
	assert(kn->kn_status & (KN_ACTIVE | KN_STAYACTIVE));
	assert(!(kn->kn_status & (KN_DISABLED | KN_SUPPRESSED | KN_DROPPING)));

	if (kq->kq_state & KQ_WORKLOOP) {
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWL_PROCESS),
		    ((struct kqworkloop *)kq)->kqwl_dynamicid,
		    kn->kn_udata, kn->kn_status | (kn->kn_id << 32),
		    kn->kn_filtid);
	} else if (kq->kq_state & KQ_WORKQ) {
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWQ_PROCESS),
		    0, kn->kn_udata, kn->kn_status | (kn->kn_id << 32),
		    kn->kn_filtid);
	} else {
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQ_PROCESS),
		    VM_KERNEL_UNSLIDE_OR_PERM(kq), kn->kn_udata,
		    kn->kn_status | (kn->kn_id << 32), kn->kn_filtid);
	}

	if (!knote_lock(kq, kn, &knlc, KNOTE_KQ_LOCK_ALWAYS)) {
		  
                                               
                                             
     
		return EJUSTRETURN;
	}

	  
                                                                      
                                                         
    
	if (!(kn->kn_status & KN_QUEUED)) {
		knote_unlock(kq, kn, &knlc, KNOTE_KQ_LOCK_ALWAYS);
		return EJUSTRETURN;
	}

	  
                                                               
                                                             
                                                               
                                  
   
                                                                       
                  
    
	knote_suppress(kq, kn);

	if (kn->kn_status & (KN_DEFERDELETE | KN_VANISHED)) {
		uint16_t kev_flags = EV_DISPATCH2 | EV_ONESHOT;
		if (kn->kn_status & KN_DEFERDELETE) {
			kev_flags |= EV_DELETE;
		} else {
			kev_flags |= EV_VANISHED;
		}

		                       
		kev = (struct kevent_qos_s){
			.filter = kn->kn_filter,
			.ident  = kn->kn_id,
			.flags  = kev_flags,
			.udata  = kn->kn_udata,
		};
	} else {
		kqunlock(kq);
		kev = (struct kevent_qos_s) { };
		result = filter_call(knote_fops(kn), f_process(kn, &kev));
		kqlock(kq);
	}

	  
                                                                  
                                                               
                                                                       
                                                                         
                       
                                                        
                                         
                                                                 
                                                                 
                                                                  
                                                                      
                                                                 
    
	if ((result & FILTER_ACTIVE) == 0) {
		if ((kn->kn_status & (KN_ACTIVE | KN_STAYACTIVE)) == 0) {
			  
                                                                     
                    
     
                                                                
                                                               
                                                   
     
                                                          
                                                    
      
			knote_unsuppress(kq, kn);
		}
		knote_unlock(kq, kn, &knlc, KNOTE_KQ_LOCK_ALWAYS);
		return EJUSTRETURN;
	}

	if (result & FILTER_ADJUST_EVENT_QOS_BIT) {
		knote_adjust_qos(kq, kn, result);
	}
	kev.qos = _pthread_priority_combine(kn->kn_qos, kn->kn_qos_override);

	if (kev.flags & EV_ONESHOT) {
		if ((kn->kn_flags & EV_DISPATCH2) == EV_DISPATCH2 &&
		    (kn->kn_status & KN_DEFERDELETE) == 0) {
			                                                        
			kn->kn_status |= KN_DEFERDELETE | KN_DISABLED;
		} else {
			drop = true;
		}
	} else if (kn->kn_flags & EV_DISPATCH) {
		                                 
		kn->kn_status |= KN_DISABLED;
	} else if ((kn->kn_flags & EV_CLEAR) == 0) {
		                                               
		knote_activate(kq, kn, FILTER_ACTIVE);
	}

	  
                                                
                                               
                                     
    
	if (drop) {
		knote_drop(kq, kn, &knlc);
	} else {
		knote_unlock(kq, kn, &knlc, KNOTE_KQ_UNLOCK);
	}

	if (kev.flags & EV_VANISHED) {
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KNOTE_VANISHED),
		    kev.ident, kn->kn_udata, kn->kn_status | (kn->kn_id << 32),
		    kn->kn_filtid);
	}

	error = (callback)(&kev, kectx);
	kqlock(kq);
	return error;
}

  
                                                                        
   
#define KQWQAE_BEGIN_PROCESSING 1
#define KQWQAE_END_PROCESSING   2
#define KQWQAE_UNBIND           3
static int
kqworkq_acknowledge_events(struct kqworkq *kqwq, workq_threadreq_t kqr,
    int kevent_flags, int kqwqae_op)
{
	thread_qos_t old_override = THREAD_QOS_UNSPECIFIED;
	thread_t thread = kqr_thread_fast(kqr);
	struct knote *kn;
	int rc = 0;
	bool unbind;
	struct kqtailq *suppressq = &kqwq->kqwq_suppressed[kqr->tr_kq_qos_index];

	kqlock_held(&kqwq->kqwq_kqueue);

	if (!TAILQ_EMPTY(suppressq)) {
		  
                                                      
                                                      
                                                       
                                                    
              
     
		while ((kn = TAILQ_FIRST(suppressq)) != NULL) {
			assert(kn->kn_status & KN_SUPPRESSED);
			knote_unsuppress(kqwq, kn);
		}
	}

#if DEBUG || DEVELOPMENT
	thread_t self = current_thread();
	struct uthread *ut = get_bsdthread_info(self);

	assert(thread == self);
	assert(ut->uu_kqr_bound == kqr);
#endif                        

	if (kqwqae_op == KQWQAE_UNBIND) {
		unbind = true;
	} else if ((kevent_flags & KEVENT_FLAG_PARKING) == 0) {
		unbind = false;
	} else {
		unbind = !kqr->tr_kq_wakeup;
	}
	if (unbind) {
		old_override = kqworkq_unbind_locked(kqwq, kqr, thread);
		rc = -1;
		  
                                                                             
                                                              
     
		if (kqr->tr_kq_wakeup) {
			kqueue_threadreq_initiate(&kqwq->kqwq_kqueue, kqr,
			    kqr->tr_kq_qos_index, 0);
		}
	}

	if (rc == 0) {
		  
                                                                      
                                                                    
                   
     
		kqr->tr_kq_wakeup = false;
	}

	if (old_override) {
		thread_drop_kevent_override(thread);
	}

	return rc;
}

  
                                                       
                                     
  
                                                      
                                 
   
static int
kqworkq_begin_processing(struct kqworkq *kqwq, workq_threadreq_t kqr,
    int kevent_flags)
{
	int rc = 0;

	KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWQ_PROCESS_BEGIN) | DBG_FUNC_START,
	    0, kqr->tr_kq_qos_index);

	rc = kqworkq_acknowledge_events(kqwq, kqr, kevent_flags,
	    KQWQAE_BEGIN_PROCESSING);

	KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWQ_PROCESS_BEGIN) | DBG_FUNC_END,
	    thread_tid(kqr_thread(kqr)), kqr->tr_kq_wakeup);

	return rc;
}

static thread_qos_t
kqworkloop_acknowledge_events(struct kqworkloop *kqwl)
{
	kq_index_t qos = THREAD_QOS_UNSPECIFIED;
	struct knote *kn, *tmp;

	kqlock_held(kqwl);

	TAILQ_FOREACH_SAFE(kn, &kqwl->kqwl_suppressed, kn_tqe, tmp) {
		  
                                                                        
                                                                       
                                    
     
		if (knote_fops(kn)->f_adjusts_qos && (kn->kn_status & KN_DISABLED) &&
		    (kn->kn_status & (KN_STAYACTIVE | KN_DROPPING)) == 0 &&
		    (kn->kn_flags & (EV_DISPATCH | EV_DISABLE)) == EV_DISPATCH) {
			qos = MAX(qos, kn->kn_qos_override);
			continue;
		}
		knote_unsuppress(kqwl, kn);
	}

	return qos;
}

static int
kqworkloop_begin_processing(struct kqworkloop *kqwl, unsigned int kevent_flags)
{
	workq_threadreq_t kqr = &kqwl->kqwl_request;
	struct kqueue *kq = &kqwl->kqwl_kqueue;
	thread_qos_t qos_override;
	thread_t thread = kqr_thread_fast(kqr);
	int rc = 0, op = KQWL_UTQ_NONE;

	kqlock_held(kq);

	KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWL_PROCESS_BEGIN) | DBG_FUNC_START,
	    kqwl->kqwl_dynamicid, 0, 0);

	                                            
	assert((kq->kq_state & KQ_PROCESSING) == 0);

	kq->kq_state |= KQ_PROCESSING;

	if (!TAILQ_EMPTY(&kqwl->kqwl_suppressed)) {
		op = KQWL_UTQ_RESET_WAKEUP_OVERRIDE;
	}

	if (kevent_flags & KEVENT_FLAG_PARKING) {
		  
                                                                        
            
    
                                                                       
                                                                         
                                                                       
                                                                      
                    
     
		if (kqr->tr_flags & WORKQ_TR_FLAG_OVERCOMMIT) {
			op = KQWL_UTQ_PARKING;
		} else {
			op = KQWL_UTQ_UNBINDING;
		}
	}
	if (op == KQWL_UTQ_NONE) {
		goto done;
	}

	qos_override = kqworkloop_acknowledge_events(kqwl);

	if (op == KQWL_UTQ_UNBINDING) {
		kqworkloop_unbind_locked(kqwl, thread, KQWL_OVERRIDE_DROP_IMMEDIATELY);
		kqworkloop_release_live(kqwl);
	}
	kqworkloop_update_threads_qos(kqwl, op, qos_override);
	if (op == KQWL_UTQ_PARKING) {
		if (!TAILQ_EMPTY(&kqwl->kqwl_queue[KQWL_BUCKET_STAYACTIVE])) {
			  
                                                                      
                                                                 
                        
      
		} else if (!kqr->tr_kq_wakeup || kqwl->kqwl_owner) {
			kqworkloop_unbind_locked(kqwl, thread, KQWL_OVERRIDE_DROP_DELAYED);
			kqworkloop_release_live(kqwl);
			rc = -1;
		}
	} else if (op == KQWL_UTQ_UNBINDING) {
		if (kqr_thread(kqr) == thread) {
			  
                                                                    
                                            
      
		} else {
			rc = -1;
		}
	}

	if (rc == 0) {
		  
                                                                      
                                                                      
     
		kqwl->kqwl_wakeup_indexes &= ~KQWL_STAYACTIVE_FIRED_BIT;
	} else {
		kq->kq_state &= ~KQ_PROCESSING;
	}

	if (rc == -1) {
		kqworkloop_unbind_delayed_override_drop(thread);
	}

done:
	KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWL_PROCESS_BEGIN) | DBG_FUNC_END,
	    kqwl->kqwl_dynamicid, 0, 0);

	return rc;
}

  
                                                       
                                     
                                  
  
                                                      
                                 
             
   
static int
kqfile_begin_processing(struct kqfile *kq)
{
	struct kqtailq *suppressq;

	kqlock_held(kq);

	assert((kq->kqf_state & (KQ_WORKQ | KQ_WORKLOOP)) == 0);
	KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQ_PROCESS_BEGIN) | DBG_FUNC_START,
	    VM_KERNEL_UNSLIDE_OR_PERM(kq), 0);

	                                                    
	for (;;) {
		if (kq->kqf_state & KQ_DRAIN) {
			KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQ_PROCESS_BEGIN) | DBG_FUNC_END,
			    VM_KERNEL_UNSLIDE_OR_PERM(kq), 2);
			return EBADF;
		}

		if ((kq->kqf_state & KQ_PROCESSING) == 0) {
			break;
		}

		                                                   
		kq->kqf_state |= KQ_PROCWAIT;
		suppressq = &kq->kqf_suppressed;
		waitq_assert_wait64((struct waitq *)&kq->kqf_wqs,
		    CAST_EVENT64_T(suppressq), THREAD_UNINT | THREAD_WAIT_NOREPORT,
		    TIMEOUT_WAIT_FOREVER);

		kqunlock(kq);
		thread_block(THREAD_CONTINUE_NULL);
		kqlock(kq);
	}

	                            

	                                                              
	waitq_set_clear_preposts(&kq->kqf_wqs);
	kq->kqf_state &= ~KQ_WAKEUP;

	                               
	if (TAILQ_EMPTY(&kq->kqf_queue)) {
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQ_PROCESS_BEGIN) | DBG_FUNC_END,
		    VM_KERNEL_UNSLIDE_OR_PERM(kq), 1);
		return -1;
	}

	                                
	kq->kqf_state |= KQ_PROCESSING;

	KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQ_PROCESS_BEGIN) | DBG_FUNC_END,
	    VM_KERNEL_UNSLIDE_OR_PERM(kq));

	return 0;
}

  
                                                                              
                                     
  
                                                                          
             
   
static int
kqworkq_end_processing(struct kqworkq *kqwq, workq_threadreq_t kqr,
    int kevent_flags)
{
	if (!TAILQ_EMPTY(&kqwq->kqwq_queue[kqr->tr_kq_qos_index])) {
		                                           
		kqr->tr_kq_wakeup = true;
	}

	if (kevent_flags & KEVENT_FLAG_PARKING) {
		  
                                                                
                                                     
     
		int rc = kqworkq_acknowledge_events(kqwq, kqr, kevent_flags,
		    KQWQAE_END_PROCESSING);
		if (rc == 0) {
			return -1;
		}
	}

	return 0;
}

  
                                                                              
                                     
  
                                                                          
                                                                      
  
                                                                     
                                                                
   
static int
kqworkloop_end_processing(struct kqworkloop *kqwl, int flags, int kevent_flags)
{
	struct kqueue *kq = &kqwl->kqwl_kqueue;
	workq_threadreq_t kqr = &kqwl->kqwl_request;
	thread_qos_t qos_override;
	thread_t thread = kqr_thread_fast(kqr);
	int rc = 0;

	kqlock_held(kq);

	KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWL_PROCESS_END) | DBG_FUNC_START,
	    kqwl->kqwl_dynamicid, 0, 0);

	if (flags & KQ_PROCESSING) {
		assert(kq->kq_state & KQ_PROCESSING);

		  
                                                                         
                                                                     
                                                        
     
		if (!TAILQ_EMPTY(&kqwl->kqwl_queue[KQWL_BUCKET_STAYACTIVE])) {
			kqworkloop_update_threads_qos(kqwl, KQWL_UTQ_UPDATE_WAKEUP_QOS,
			    KQWL_BUCKET_STAYACTIVE);
		}

		  
                                                                        
                          
    
                                                                        
                                                                      
                                                                          
                               
    
                                                                    
                   
     
		if (kevent_flags & KEVENT_FLAG_PARKING) {
			qos_override = kqworkloop_acknowledge_events(kqwl);
		}
	}

	if (kevent_flags & KEVENT_FLAG_PARKING) {
		kqworkloop_update_threads_qos(kqwl, KQWL_UTQ_PARKING, qos_override);
		if (kqr->tr_kq_wakeup && !kqwl->kqwl_owner) {
			  
                                                                       
                                                                       
      
			kqwl->kqwl_wakeup_indexes &= ~KQWL_STAYACTIVE_FIRED_BIT;
			rc = -1;
		} else {
			kqworkloop_unbind_locked(kqwl, thread, KQWL_OVERRIDE_DROP_DELAYED);
			kqworkloop_release_live(kqwl);
			kq->kq_state &= ~flags;
		}
	} else {
		kq->kq_state &= ~flags;
		kq->kq_state |= KQ_R2K_ARMED;
		kqworkloop_update_threads_qos(kqwl, KQWL_UTQ_RECOMPUTE_WAKEUP_QOS, 0);
	}

	if ((kevent_flags & KEVENT_FLAG_PARKING) && rc == 0) {
		kqworkloop_unbind_delayed_override_drop(thread);
	}

	KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWL_PROCESS_END) | DBG_FUNC_END,
	    kqwl->kqwl_dynamicid, 0, 0);

	return rc;
}

  
                                
  
                    
                      
                                    
   
static int
kqfile_end_processing(struct kqfile *kq)
{
	struct kqtailq *suppressq = &kq->kqf_suppressed;
	struct knote *kn;
	int procwait;

	kqlock_held(kq);

	assert((kq->kqf_state & (KQ_WORKQ | KQ_WORKLOOP)) == 0);

	KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQ_PROCESS_END),
	    VM_KERNEL_UNSLIDE_OR_PERM(kq), 0);

	  
                                                     
    
	while ((kn = TAILQ_FIRST(suppressq)) != NULL) {
		assert(kn->kn_status & KN_SUPPRESSED);
		knote_unsuppress(kq, kn);
	}

	procwait = (kq->kqf_state & KQ_PROCWAIT);
	kq->kqf_state &= ~(KQ_PROCESSING | KQ_PROCWAIT);

	if (procwait) {
		                                                         
		waitq_wakeup64_all((struct waitq *)&kq->kqf_wqs,
		    CAST_EVENT64_T(suppressq), THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);
	}

	if (kq->kqf_state & KQ_DRAIN) {
		return EBADF;
	}
	return (kq->kqf_state & KQ_WAKEUP) ? -1 : 0;
}

static int
kqueue_workloop_ctl_internal(proc_t p, uintptr_t cmd, uint64_t __unused options,
    struct kqueue_workloop_params *params, int *retval)
{
	int error = 0;
	struct kqworkloop *kqwl;
	struct filedesc *fdp = p->p_fd;
	workq_threadreq_param_t trp = { };

	switch (cmd) {
	case KQ_WORKLOOP_CREATE:
		if (!params->kqwlp_flags) {
			error = EINVAL;
			break;
		}

		if ((params->kqwlp_flags & KQ_WORKLOOP_CREATE_SCHED_PRI) &&
		    (params->kqwlp_sched_pri < 1 ||
		    params->kqwlp_sched_pri > 63                  )) {
			error = EINVAL;
			break;
		}

		if ((params->kqwlp_flags & KQ_WORKLOOP_CREATE_SCHED_POL) &&
		    invalid_policy(params->kqwlp_sched_pol)) {
			error = EINVAL;
			break;
		}

		if ((params->kqwlp_flags & KQ_WORKLOOP_CREATE_CPU_PERCENT) &&
		    (params->kqwlp_cpu_percent <= 0 ||
		    params->kqwlp_cpu_percent > 100 ||
		    params->kqwlp_cpu_refillms <= 0 ||
		    params->kqwlp_cpu_refillms > 0x00ffffff)) {
			error = EINVAL;
			break;
		}

		if (params->kqwlp_flags & KQ_WORKLOOP_CREATE_SCHED_PRI) {
			trp.trp_flags |= TRP_PRIORITY;
			trp.trp_pri = (uint8_t)params->kqwlp_sched_pri;
		}
		if (params->kqwlp_flags & KQ_WORKLOOP_CREATE_SCHED_POL) {
			trp.trp_flags |= TRP_POLICY;
			trp.trp_pol = (uint8_t)params->kqwlp_sched_pol;
		}
		if (params->kqwlp_flags & KQ_WORKLOOP_CREATE_CPU_PERCENT) {
			trp.trp_flags |= TRP_CPUPERCENT;
			trp.trp_cpupercent = (uint8_t)params->kqwlp_cpu_percent;
			trp.trp_refillms = params->kqwlp_cpu_refillms;
		}

		error = kqworkloop_get_or_create(p, params->kqwlp_id, &trp,
		    KEVENT_FLAG_DYNAMIC_KQUEUE | KEVENT_FLAG_WORKLOOP |
		    KEVENT_FLAG_DYNAMIC_KQ_MUST_NOT_EXIST, &kqwl);
		if (error) {
			break;
		}

		if (!(fdp->fd_flags & FD_WORKLOOP)) {
			                                                      
                                                                  
              
      
			proc_fdlock(p);
			fdp->fd_flags |= FD_WORKLOOP;
			proc_fdunlock(p);
		}
		break;
	case KQ_WORKLOOP_DESTROY:
		error = kqworkloop_get_or_create(p, params->kqwlp_id, NULL,
		    KEVENT_FLAG_DYNAMIC_KQUEUE | KEVENT_FLAG_WORKLOOP |
		    KEVENT_FLAG_DYNAMIC_KQ_MUST_EXIST, &kqwl);
		if (error) {
			break;
		}
		kqlock(kqwl);
		trp.trp_value = kqwl->kqwl_params;
		if (trp.trp_flags && !(trp.trp_flags & TRP_RELEASED)) {
			trp.trp_flags |= TRP_RELEASED;
			kqwl->kqwl_params = trp.trp_value;
			kqworkloop_release_live(kqwl);
		} else {
			error = EINVAL;
		}
		kqunlock(kqwl);
		kqworkloop_release(kqwl);
		break;
	}
	*retval = 0;
	return error;
}

int
kqueue_workloop_ctl(proc_t p, struct kqueue_workloop_ctl_args *uap, int *retval)
{
	struct kqueue_workloop_params params = {
		.kqwlp_id = 0,
	};
	if (uap->sz < sizeof(params.kqwlp_version)) {
		return EINVAL;
	}

	size_t copyin_sz = MIN(sizeof(params), uap->sz);
	int rv = copyin(uap->addr, &params, copyin_sz);
	if (rv) {
		return rv;
	}

	if (params.kqwlp_version != (int)uap->sz) {
		return EINVAL;
	}

	return kqueue_workloop_ctl_internal(p, uap->cmd, uap->options, &params,
	           retval);
}

            
static int
kqueue_select(struct fileproc *fp, int which, void *wq_link_id,
    __unused vfs_context_t ctx)
{
	struct kqfile *kq = (struct kqfile *)fp->f_data;
	struct kqtailq *suppressq = &kq->kqf_suppressed;
	struct kqtailq *queue = &kq->kqf_queue;
	struct knote *kn;
	int retnum = 0;

	if (which != FREAD) {
		return 0;
	}

	kqlock(kq);

	assert((kq->kqf_state & KQ_WORKQ) == 0);

	  
                                                                      
                                                                     
                                                                   
                                                                       
                                                                           
                                                      
    
	if (wq_link_id != NULL) {
		thread_t cur_act = current_thread();
		struct uthread * ut = get_bsdthread_info(cur_act);

		kq->kqf_state |= KQ_SEL;
		waitq_link((struct waitq *)&kq->kqf_wqs, ut->uu_wqset,
		    WAITQ_SHOULD_LOCK, (uint64_t *)wq_link_id);

		                                             
		waitq_link_release(*(uint64_t *)wq_link_id);
		*(uint64_t *)wq_link_id = 0;

		  
                                                             
                                                            
                                                            
                                                
                                                            
                                                                
                       
     
		void *wqptr = &kq->kqf_wqs;
		memcpy(wq_link_id, (void *)&wqptr, sizeof(void *));
	}

	if (kqfile_begin_processing(kq) == -1) {
		kqunlock(kq);
		return 0;
	}

	if (!TAILQ_EMPTY(queue)) {
		  
                                                  
                                                   
                                                    
                                               
                                                       
     
		while ((kn = (struct knote *)TAILQ_FIRST(queue)) != NULL) {
			if (kn->kn_status & KN_ACTIVE) {
				retnum = 1;
				goto out;
			}
			assert(kn->kn_status & KN_STAYACTIVE);
			knote_suppress(kq, kn);
		}

		  
                                                       
                                                         
     
		while ((kn = (struct knote *)TAILQ_FIRST(suppressq)) != NULL) {
			KNOTE_LOCK_CTX(knlc);
			int result = 0;

			                                                    
			if ((kn->kn_status & KN_DROPPING) || !knote_lock(kq, kn, &knlc,
			    KNOTE_KQ_LOCK_ON_FAILURE)) {
				continue;
			}

			result = filter_call(knote_fops(kn), f_peek(kn));

			kqlock(kq);
			knote_unlock(kq, kn, &knlc, KNOTE_KQ_LOCK_ALWAYS);

			                   
			knote_unsuppress(kq, kn);

			                                           
			if (result & FILTER_ACTIVE) {
				retnum = 1;
				goto out;
			}
		}
	}

out:
	kqfile_end_processing(kq);
	kqunlock(kq);
	return retnum;
}

  
                 
   
            
static int
kqueue_close(struct fileglob *fg, __unused vfs_context_t ctx)
{
	struct kqfile *kqf = (struct kqfile *)fg->fg_data;

	assert((kqf->kqf_state & KQ_WORKQ) == 0);
	kqueue_dealloc(&kqf->kqf_kqueue);
	fg->fg_data = NULL;
	return 0;
}

  
                                                       
                                                          
                                                      
   
#define MAX_NESTED_KQ 1000

            
  
                                                                                
                                                                                 
                                  
   
static int
kqueue_kqfilter(struct fileproc *fp, struct knote *kn,
    __unused struct kevent_qos_s *kev)
{
	struct kqfile *kqf = (struct kqfile *)fp->f_data;
	struct kqueue *kq = &kqf->kqf_kqueue;
	struct kqueue *parentkq = knote_get_kq(kn);

	assert((kqf->kqf_state & KQ_WORKQ) == 0);

	if (parentkq == kq || kn->kn_filter != EVFILT_READ) {
		knote_set_error(kn, EINVAL);
		return 0;
	}

	  
                                                          
                                                         
                                                         
                                                              
                                                           
                                                          
                                                            
                                                        
   
                                           
   
                                                                       
                                                    
    

	kqlock(parentkq);

	if ((parentkq->kq_state & (KQ_WORKQ | KQ_WORKLOOP)) == 0) {
		if (parentkq->kq_level > 0 &&
		    parentkq->kq_level < kq->kq_level) {
			kqunlock(parentkq);
			knote_set_error(kn, EINVAL);
			return 0;
		}

		                                    
		uint16_t plevel = (parentkq->kq_level == 0)? 2: parentkq->kq_level;
		if (plevel < kq->kq_level + 1) {
			if (kq->kq_level + 1 > MAX_NESTED_KQ) {
				kqunlock(parentkq);
				knote_set_error(kn, EINVAL);
				return 0;
			}
			plevel = kq->kq_level + 1;
		}

		parentkq->kq_level = plevel;
	}

	kqunlock(parentkq);

	kn->kn_filtid = EVFILTID_KQREAD;
	kqlock(kq);
	KNOTE_ATTACH(&kqf->kqf_sel.si_note, kn);
	                                          
	if (kq->kq_level == 0) {
		kq->kq_level = 1;
	}

	int count = kq->kq_count;
	kqunlock(kq);
	return count > 0;
}

  
                                          
   
            
static int
kqueue_drain(struct fileproc *fp, __unused vfs_context_t ctx)
{
	struct kqfile *kqf = (struct kqfile *)fp->fp_glob->fg_data;

	assert((kqf->kqf_state & KQ_WORKQ) == 0);

	kqlock(kqf);
	kqf->kqf_state |= KQ_DRAIN;

	                             
	if ((kqf->kqf_state & (KQ_SLEEP | KQ_SEL)) != 0) {
		kqf->kqf_state &= ~(KQ_SLEEP | KQ_SEL);
		(void)waitq_wakeup64_all((struct waitq *)&kqf->kqf_wqs,
		    KQ_EVENT,
		    THREAD_RESTART,
		    WAITQ_ALL_PRIORITIES);
	}

	                                                  
	if (kqf->kqf_state & KQ_PROCWAIT) {
		assert(kqf->kqf_state & KQ_PROCESSING);

		kqf->kqf_state &= ~KQ_PROCWAIT;
		(void)waitq_wakeup64_all((struct waitq *)&kqf->kqf_wqs,
		    CAST_EVENT64_T(&kqf->kqf_suppressed),
		    THREAD_RESTART, WAITQ_ALL_PRIORITIES);
	}

	kqunlock(kqf);
	return 0;
}

            
int
kqueue_stat(struct kqueue *kq, void *ub, int isstat64, proc_t p)
{
	assert((kq->kq_state & KQ_WORKQ) == 0);

	kqlock(kq);
	if (isstat64 != 0) {
		struct stat64 *sb64 = (struct stat64 *)ub;

		bzero((void *)sb64, sizeof(*sb64));
		sb64->st_size = kq->kq_count;
		if (kq->kq_state & KQ_KEV_QOS) {
			sb64->st_blksize = sizeof(struct kevent_qos_s);
		} else if (kq->kq_state & KQ_KEV64) {
			sb64->st_blksize = sizeof(struct kevent64_s);
		} else if (IS_64BIT_PROCESS(p)) {
			sb64->st_blksize = sizeof(struct user64_kevent);
		} else {
			sb64->st_blksize = sizeof(struct user32_kevent);
		}
		sb64->st_mode = S_IFIFO;
	} else {
		struct stat *sb = (struct stat *)ub;

		bzero((void *)sb, sizeof(*sb));
		sb->st_size = kq->kq_count;
		if (kq->kq_state & KQ_KEV_QOS) {
			sb->st_blksize = sizeof(struct kevent_qos_s);
		} else if (kq->kq_state & KQ_KEV64) {
			sb->st_blksize = sizeof(struct kevent64_s);
		} else if (IS_64BIT_PROCESS(p)) {
			sb->st_blksize = sizeof(struct user64_kevent);
		} else {
			sb->st_blksize = sizeof(struct user32_kevent);
		}
		sb->st_mode = S_IFIFO;
	}
	kqunlock(kq);
	return 0;
}

static inline bool
kqueue_threadreq_can_use_ast(struct kqueue *kq)
{
	if (current_proc() == kq->kq_p) {
		  
                                                                         
                                                                          
                                                                    
                                                   
    
                                            
    
                                                                    
                                                 
    
                                                         
    
                                                                        
                                                  
     
		switch (current_uthread()->syscall_code) {
		case SYS_kevent_qos:
		case SYS_kevent_id:
		case SYS_workq_kernreturn:
		case SYS_bsdthread_ctl:
			return true;
		}
	}
	return false;
}

  
                                                                                
         
  
                                        
  
                                                           
                                                      
   
static void
kqueue_threadreq_initiate(struct kqueue *kq, workq_threadreq_t kqr,
    kq_index_t qos, int flags)
{
	assert(kqr->tr_kq_wakeup);
	assert(kqr_thread(kqr) == THREAD_NULL);
	assert(!kqr_thread_requested(kqr));
	struct turnstile *ts = TURNSTILE_NULL;

	if (workq_is_exiting(kq->kq_p)) {
		return;
	}

	kqlock_held(kq);

	if (kq->kq_state & KQ_WORKLOOP) {
		__assert_only struct kqworkloop *kqwl = (struct kqworkloop *)kq;

		assert(kqwl->kqwl_owner == THREAD_NULL);
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWL_THREQUEST),
		    kqwl->kqwl_dynamicid, 0, qos, kqr->tr_kq_wakeup);
		ts = kqwl->kqwl_turnstile;
		                                                   
		kqworkloop_retain(kqwl);
	} else {
		assert(kq->kq_state & KQ_WORKQ);
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWQ_THREQUEST),
		    -1, 0, qos, kqr->tr_kq_wakeup);
	}

	  
                                       
                                                                           
                                                                 
    
	if (kqueue_threadreq_can_use_ast(kq)) {
		flags |= WORKQ_THREADREQ_SET_AST_ON_FAILURE;
	}
	if (qos == KQWQ_QOS_MANAGER) {
		qos = WORKQ_THREAD_QOS_MANAGER;
	}
	if (!workq_kern_threadreq_initiate(kq->kq_p, kqr, ts, qos, flags)) {
		  
                                          
                                               
                                              
                                         
     
		kq->kq_state &= ~KQ_R2K_ARMED;
		kqueue_release_live(kq);
	}
}

  
                                                             
  
                                                                      
   
__attribute__((always_inline))
void
kqueue_threadreq_bind_prepost(struct proc *p __unused, workq_threadreq_t kqr,
    struct uthread *ut)
{
	ut->uu_kqr_bound = kqr;
	kqr->tr_thread = ut->uu_thread;
	kqr->tr_state = WORKQ_TR_STATE_BINDING;
}

  
                                                       
  
                                                                         
                                                                             
   
void
kqueue_threadreq_bind_commit(struct proc *p, thread_t thread)
{
	struct uthread *ut = get_bsdthread_info(thread);
	workq_threadreq_t kqr = ut->uu_kqr_bound;
	kqueue_t kqu = kqr_kqueue(p, kqr);

	kqlock(kqu);
	if (kqr->tr_state == WORKQ_TR_STATE_BINDING) {
		kqueue_threadreq_bind(p, kqr, thread, 0);
	}
	kqunlock(kqu);
}

static void
kqueue_threadreq_modify(kqueue_t kqu, workq_threadreq_t kqr, kq_index_t qos,
    workq_kern_threadreq_flags_t flags)
{
	assert(kqr_thread_requested_pending(kqr));

	kqlock_held(kqu);

	if (kqueue_threadreq_can_use_ast(kqu.kq)) {
		flags |= WORKQ_THREADREQ_SET_AST_ON_FAILURE;
	}
	workq_kern_threadreq_modify(kqu.kq->kq_p, kqr, qos, flags);
}

  
                                                              
  
                                                                
                                                                  
                                   
   
void
kqueue_threadreq_bind(struct proc *p, workq_threadreq_t kqr, thread_t thread,
    unsigned int flags)
{
	kqueue_t kqu = kqr_kqueue(p, kqr);
	struct uthread *ut = get_bsdthread_info(thread);

	kqlock_held(kqu);

	assert(ut->uu_kqueue_override == 0);

	if (kqr->tr_state == WORKQ_TR_STATE_BINDING) {
		assert(ut->uu_kqr_bound == kqr);
		assert(kqr->tr_thread == thread);
	} else {
		assert(kqr_thread_requested_pending(kqr));
		assert(kqr->tr_thread == THREAD_NULL);
		assert(ut->uu_kqr_bound == NULL);
		ut->uu_kqr_bound = kqr;
		kqr->tr_thread = thread;
	}

	kqr->tr_state = WORKQ_TR_STATE_BOUND;

	if (kqu.kq->kq_state & KQ_WORKLOOP) {
		struct turnstile *ts = kqu.kqwl->kqwl_turnstile;

		if (__improbable(thread == kqu.kqwl->kqwl_owner)) {
			  
                                                                    
     
                                                                      
                                                                     
                                                                   
                                                   
     
                                                                  
                                                                     
                                                                   
                                                                 
                                                                  
      
			kqu.kqwl->kqwl_owner = THREAD_NULL;
			if (kqworkloop_override(kqu.kqwl)) {
				thread_drop_kevent_override(thread);
			}
		}

		if (ts && (flags & KQUEUE_THREADERQ_BIND_NO_INHERITOR_UPDATE) == 0) {
			  
                                                              
                                           
      
			filt_wlupdate_inheritor(kqu.kqwl, ts, TURNSTILE_IMMEDIATE_UPDATE);
			turnstile_update_inheritor_complete(ts, TURNSTILE_INTERLOCK_HELD);
		}

		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWL_BIND), kqu.kqwl->kqwl_dynamicid,
		    thread_tid(thread), kqr->tr_kq_qos_index,
		    (kqr->tr_kq_override_index << 16) | kqr->tr_kq_wakeup);

		ut->uu_kqueue_override = kqr->tr_kq_override_index;
		if (kqr->tr_kq_override_index) {
			thread_add_servicer_override(thread, kqr->tr_kq_override_index);
		}
	} else {
		assert(kqr->tr_kq_override_index == 0);

		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWQ_BIND), -1,
		    thread_tid(thread), kqr->tr_kq_qos_index,
		    (kqr->tr_kq_override_index << 16) | kqr->tr_kq_wakeup);
	}
}

  
                                                           
  
                                                            
   
void
kqueue_threadreq_cancel(struct proc *p, workq_threadreq_t kqr)
{
	kqueue_release(kqr_kqueue(p, kqr));
}

workq_threadreq_param_t
kqueue_threadreq_workloop_param(workq_threadreq_t kqr)
{
	struct kqworkloop *kqwl;
	workq_threadreq_param_t trp;

	assert(kqr->tr_flags & WORKQ_TR_FLAG_WORKLOOP);
	kqwl = __container_of(kqr, struct kqworkloop, kqwl_request);
	trp.trp_value = kqwl->kqwl_params;
	return trp;
}

  
                                                                 
  
                                                                      
                                        
  
                                          
   
void
kqueue_threadreq_unbind(struct proc *p, workq_threadreq_t kqr)
{
	if (kqr->tr_flags & WORKQ_TR_FLAG_WORKLOOP) {
		kqworkloop_unbind(kqr_kqworkloop(kqr));
	} else {
		kqworkq_unbind(p, kqr);
	}
}

  
                                                              
                                               
  
                                                           
  
                                                           
                                                      
   
static void
kqworkq_wakeup(struct kqworkq *kqwq, kq_index_t qos_index)
{
	workq_threadreq_t kqr = kqworkq_get_request(kqwq, qos_index);

	                                 
	assert(qos_index < KQWQ_NBUCKETS);

	if (!kqr->tr_kq_wakeup) {
		kqr->tr_kq_wakeup = true;
		if (!kqr_thread_requested(kqr)) {
			kqueue_threadreq_initiate(&kqwq->kqwq_kqueue, kqr, qos_index, 0);
		}
	}
}

  
                                                                    
                                                                 
                                                  
   
static kq_index_t
kqworkloop_override(struct kqworkloop *kqwl)
{
	workq_threadreq_t kqr = &kqwl->kqwl_request;
	return MAX(kqr->tr_kq_qos_index, kqr->tr_kq_override_index);
}

static inline void
kqworkloop_request_fire_r2k_notification(struct kqworkloop *kqwl)
{
	workq_threadreq_t kqr = &kqwl->kqwl_request;

	kqlock_held(kqwl);

	if (kqwl->kqwl_state & KQ_R2K_ARMED) {
		kqwl->kqwl_state &= ~KQ_R2K_ARMED;
		act_set_astkevent(kqr_thread_fast(kqr), AST_KEVENT_RETURN_TO_KERNEL);
	}
}

static void
kqworkloop_update_threads_qos(struct kqworkloop *kqwl, int op, kq_index_t qos)
{
	workq_threadreq_t kqr = &kqwl->kqwl_request;
	struct kqueue *kq = &kqwl->kqwl_kqueue;
	kq_index_t old_override = kqworkloop_override(kqwl);
	kq_index_t i;

	kqlock_held(kqwl);

	switch (op) {
	case KQWL_UTQ_UPDATE_WAKEUP_QOS:
		if (qos == KQWL_BUCKET_STAYACTIVE) {
			  
                                                                      
                                                                     
                                                  
     
                                                                       
                                                                       
                                                                   
                                                                       
                                
      
			kqwl->kqwl_wakeup_indexes |= KQWL_STAYACTIVE_FIRED_BIT;
			qos = kqwl->kqwl_stayactive_qos;
			assert(qos);
		}
		if (kqwl->kqwl_wakeup_indexes & (1 << qos)) {
			assert(kqr->tr_kq_wakeup);
			break;
		}

		kqwl->kqwl_wakeup_indexes |= (1 << qos);
		kqr->tr_kq_wakeup = true;
		kqworkloop_request_fire_r2k_notification(kqwl);
		goto recompute;

	case KQWL_UTQ_UPDATE_STAYACTIVE_QOS:
		assert(qos);
		if (kqwl->kqwl_stayactive_qos < qos) {
			kqwl->kqwl_stayactive_qos = qos;
			if (kqwl->kqwl_wakeup_indexes & KQWL_STAYACTIVE_FIRED_BIT) {
				assert(kqr->tr_kq_wakeup);
				kqwl->kqwl_wakeup_indexes |= (1 << qos);
				goto recompute;
			}
		}
		break;

	case KQWL_UTQ_PARKING:
	case KQWL_UTQ_UNBINDING:
		kqr->tr_kq_override_index = qos;
		OS_FALLTHROUGH;
	case KQWL_UTQ_RECOMPUTE_WAKEUP_QOS:
		if (op == KQWL_UTQ_RECOMPUTE_WAKEUP_QOS) {
			assert(qos == THREAD_QOS_UNSPECIFIED);
		}
		i = KQWL_BUCKET_STAYACTIVE;
		if (TAILQ_EMPTY(&kqwl->kqwl_suppressed)) {
			kqr->tr_kq_override_index = THREAD_QOS_UNSPECIFIED;
		}
		if (!TAILQ_EMPTY(&kqwl->kqwl_queue[i]) &&
		    (kqwl->kqwl_wakeup_indexes & KQWL_STAYACTIVE_FIRED_BIT)) {
			  
                                                                     
                                                                       
     
                                                                      
      
			kqwl->kqwl_wakeup_indexes &= KQWL_STAYACTIVE_FIRED_BIT;
			kqwl->kqwl_wakeup_indexes |= (1 << kqwl->kqwl_stayactive_qos);
		} else {
			kqwl->kqwl_wakeup_indexes = 0;
		}
		for (i = THREAD_QOS_UNSPECIFIED + 1; i < KQWL_BUCKET_STAYACTIVE; i++) {
			if (!TAILQ_EMPTY(&kqwl->kqwl_queue[i])) {
				kqwl->kqwl_wakeup_indexes |= (1 << i);
			}
		}
		if (kqwl->kqwl_wakeup_indexes) {
			kqr->tr_kq_wakeup = true;
			kqworkloop_request_fire_r2k_notification(kqwl);
		} else {
			kqr->tr_kq_wakeup = false;
		}
		goto recompute;

	case KQWL_UTQ_RESET_WAKEUP_OVERRIDE:
		kqr->tr_kq_override_index = qos;
		goto recompute;

	case KQWL_UTQ_UPDATE_WAKEUP_OVERRIDE:
recompute:
		  
                                                                         
                                                                        
                                                    
    
                                                                         
                                            
     
		if (kqwl->kqwl_wakeup_indexes > (1 << qos)) {
			qos = (uint8_t)(fls(kqwl->kqwl_wakeup_indexes) - 1);                     
		}
		if (kqr->tr_kq_override_index < qos) {
			kqr->tr_kq_override_index = qos;
		}
		break;

	case KQWL_UTQ_REDRIVE_EVENTS:
		break;

	case KQWL_UTQ_SET_QOS_INDEX:
		kqr->tr_kq_qos_index = qos;
		break;

	default:
		panic("unknown kqwl thread qos update operation: %d", op);
	}

	thread_t kqwl_owner = kqwl->kqwl_owner;
	thread_t servicer = kqr_thread(kqr);
	boolean_t qos_changed = FALSE;
	kq_index_t new_override = kqworkloop_override(kqwl);

	  
                                              
    
	if (kqwl_owner) {
#if 0
		                                                    
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWL_THADJUST),
		    kqwl->kqwl_dynamicid, thread_tid(kqwl_owner), kqr->tr_kq_qos_index,
		    (kqr->tr_kq_override_index << 16) | kqr->tr_kq_wakeup);
#endif
		if (new_override == old_override) {
			                
		} else if (old_override == THREAD_QOS_UNSPECIFIED) {
			thread_add_kevent_override(kqwl_owner, new_override);
		} else if (new_override == THREAD_QOS_UNSPECIFIED) {
			thread_drop_kevent_override(kqwl_owner);
		} else {                                    
			thread_update_kevent_override(kqwl_owner, new_override);
		}
	}

	  
                                   
    
	if (!kqr_thread_requested(kqr)) {
		  
                                    
    
                                                                         
                                                                         
                 
     

		if (kqwl_owner == NULL && kqr->tr_kq_wakeup) {
			int initiate_flags = 0;
			if (op == KQWL_UTQ_UNBINDING) {
				initiate_flags = WORKQ_THREADREQ_ATTEMPT_REBIND;
			}
			kqueue_threadreq_initiate(kq, kqr, new_override, initiate_flags);
		}
	} else if (servicer) {
		  
                       
    
                                        
     
		struct uthread *ut = get_bsdthread_info(servicer);
		if (ut->uu_kqueue_override != new_override) {
			if (ut->uu_kqueue_override == THREAD_QOS_UNSPECIFIED) {
				thread_add_servicer_override(servicer, new_override);
			} else if (new_override == THREAD_QOS_UNSPECIFIED) {
				thread_drop_servicer_override(servicer);
			} else {                                             
				thread_update_servicer_override(servicer, new_override);
			}
			ut->uu_kqueue_override = new_override;
			qos_changed = TRUE;
		}
	} else if (new_override == THREAD_QOS_UNSPECIFIED) {
		  
                                  
    
                                                                       
                                                                        
              
     
	} else if (old_override != new_override) {
		  
                         
    
                                         
     
		kqueue_threadreq_modify(kq, kqr, new_override, WORKQ_THREADREQ_NONE);
		qos_changed = TRUE;
	}

	if (qos_changed) {
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWL_THADJUST), kqwl->kqwl_dynamicid,
		    thread_tid(servicer), kqr->tr_kq_qos_index,
		    (kqr->tr_kq_override_index << 16) | kqr->tr_kq_wakeup);
	}
}

static void
kqworkloop_wakeup(struct kqworkloop *kqwl, kq_index_t qos)
{
	if ((kqwl->kqwl_state & KQ_PROCESSING) &&
	    kqr_thread(&kqwl->kqwl_request) == current_thread()) {
		  
                                                              
                                                     
     
		return;
	}

	kqworkloop_update_threads_qos(kqwl, KQWL_UTQ_UPDATE_WAKEUP_QOS, qos);
}

static struct kqtailq *
kqueue_get_suppressed_queue(kqueue_t kq, struct knote *kn)
{
	if (kq.kq->kq_state & KQ_WORKLOOP) {
		return &kq.kqwl->kqwl_suppressed;
	} else if (kq.kq->kq_state & KQ_WORKQ) {
		return &kq.kqwq->kqwq_suppressed[kn->kn_qos_index];
	} else {
		return &kq.kqf->kqf_suppressed;
	}
}

struct turnstile *
kqueue_alloc_turnstile(kqueue_t kqu)
{
	struct kqworkloop *kqwl = kqu.kqwl;
	kq_state_t kq_state;

	kq_state = os_atomic_load(&kqu.kq->kq_state, dependency);
	if (kq_state & KQ_HAS_TURNSTILE) {
		                                                                      
		return os_atomic_load_with_dependency_on(&kqwl->kqwl_turnstile,
		           (uintptr_t)kq_state);
	}

	if (!(kq_state & KQ_WORKLOOP)) {
		return TURNSTILE_NULL;
	}

	struct turnstile *ts = turnstile_alloc(), *free_ts = TURNSTILE_NULL;
	bool workq_locked = false;

	kqlock(kqu);

	if (filt_wlturnstile_interlock_is_workq(kqwl)) {
		workq_locked = true;
		workq_kern_threadreq_lock(kqwl->kqwl_p);
	}

	if (kqwl->kqwl_state & KQ_HAS_TURNSTILE) {
		free_ts = ts;
		ts = kqwl->kqwl_turnstile;
	} else {
		ts = turnstile_prepare((uintptr_t)kqwl, &kqwl->kqwl_turnstile,
		    ts, TURNSTILE_WORKLOOPS);

		                                                                            
		os_atomic_or(&kqwl->kqwl_state, KQ_HAS_TURNSTILE, release);

		if (filt_wlturnstile_interlock_is_workq(kqwl)) {
			workq_kern_threadreq_update_inheritor(kqwl->kqwl_p,
			    &kqwl->kqwl_request, kqwl->kqwl_owner,
			    ts, TURNSTILE_IMMEDIATE_UPDATE);
			  
                                                          
                                                 
      
		}
		if (!filt_wlturnstile_interlock_is_workq(kqwl)) {
			filt_wlupdate_inheritor(kqwl, ts, TURNSTILE_IMMEDIATE_UPDATE);
		}
	}

	if (workq_locked) {
		workq_kern_threadreq_unlock(kqwl->kqwl_p);
	}

	kqunlock(kqu);

	if (free_ts) {
		turnstile_deallocate(free_ts);
	} else {
		turnstile_update_inheritor_complete(ts, TURNSTILE_INTERLOCK_NOT_HELD);
	}
	return ts;
}

__attribute__((always_inline))
struct turnstile *
kqueue_turnstile(kqueue_t kqu)
{
	kq_state_t kq_state = os_atomic_load(&kqu.kq->kq_state, relaxed);
	if (kq_state & KQ_WORKLOOP) {
		return os_atomic_load(&kqu.kqwl->kqwl_turnstile, relaxed);
	}
	return TURNSTILE_NULL;
}

__attribute__((always_inline))
struct turnstile *
kqueue_threadreq_get_turnstile(workq_threadreq_t kqr)
{
	struct kqworkloop *kqwl = kqr_kqworkloop(kqr);
	if (kqwl) {
		return os_atomic_load(&kqwl->kqwl_turnstile, relaxed);
	}
	return TURNSTILE_NULL;
}

static void
kqworkloop_set_overcommit(struct kqworkloop *kqwl)
{
	workq_threadreq_t kqr = &kqwl->kqwl_request;

	  
                                                          
                                        
    
	if (kqr->tr_flags & WORKQ_TR_FLAG_OVERCOMMIT) {
		return;
	}

	kqlock_held(kqwl);

	if (kqr_thread_requested_pending(kqr)) {
		kqueue_threadreq_modify(kqwl, kqr, kqr->tr_qos,
		    WORKQ_THREADREQ_MAKE_OVERCOMMIT);
	} else {
		kqr->tr_flags |= WORKQ_TR_FLAG_OVERCOMMIT;
	}
}

static void
kqworkq_update_override(struct kqworkq *kqwq, struct knote *kn,
    kq_index_t override_index)
{
	workq_threadreq_t kqr;
	kq_index_t old_override_index;
	kq_index_t queue_index = kn->kn_qos_index;

	if (override_index <= queue_index) {
		return;
	}

	kqr = kqworkq_get_request(kqwq, queue_index);

	kqlock_held(kqwq);

	old_override_index = kqr->tr_kq_override_index;
	if (override_index > MAX(kqr->tr_kq_qos_index, old_override_index)) {
		thread_t servicer = kqr_thread(kqr);
		kqr->tr_kq_override_index = override_index;

		                                                        
		if (servicer) {
			if (old_override_index) {
				thread_update_kevent_override(servicer, override_index);
			} else {
				thread_add_kevent_override(servicer, override_index);
			}
		}
	}
}

static void
kqueue_update_override(kqueue_t kqu, struct knote *kn, thread_qos_t qos)
{
	if (kqu.kq->kq_state & KQ_WORKLOOP) {
		kqworkloop_update_threads_qos(kqu.kqwl, KQWL_UTQ_UPDATE_WAKEUP_OVERRIDE,
		    qos);
	} else {
		kqworkq_update_override(kqu.kqwq, kn, qos);
	}
}

static void
kqworkloop_unbind_locked(struct kqworkloop *kqwl, thread_t thread,
    enum kqwl_unbind_locked_mode how)
{
	struct uthread *ut = get_bsdthread_info(thread);
	workq_threadreq_t kqr = &kqwl->kqwl_request;

	KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWL_UNBIND), kqwl->kqwl_dynamicid,
	    thread_tid(thread), 0, 0);

	kqlock_held(kqwl);

	assert(ut->uu_kqr_bound == kqr);
	ut->uu_kqr_bound = NULL;
	if (how == KQWL_OVERRIDE_DROP_IMMEDIATELY &&
	    ut->uu_kqueue_override != THREAD_QOS_UNSPECIFIED) {
		thread_drop_servicer_override(thread);
		ut->uu_kqueue_override = THREAD_QOS_UNSPECIFIED;
	}

	if (kqwl->kqwl_owner == NULL && kqwl->kqwl_turnstile) {
		turnstile_update_inheritor(kqwl->kqwl_turnstile,
		    TURNSTILE_INHERITOR_NULL, TURNSTILE_IMMEDIATE_UPDATE);
		turnstile_update_inheritor_complete(kqwl->kqwl_turnstile,
		    TURNSTILE_INTERLOCK_HELD);
	}

	kqr->tr_thread = THREAD_NULL;
	kqr->tr_state = WORKQ_TR_STATE_IDLE;
	kqwl->kqwl_state &= ~KQ_R2K_ARMED;
}

static void
kqworkloop_unbind_delayed_override_drop(thread_t thread)
{
	struct uthread *ut = get_bsdthread_info(thread);
	assert(ut->uu_kqr_bound == NULL);
	if (ut->uu_kqueue_override != THREAD_QOS_UNSPECIFIED) {
		thread_drop_servicer_override(thread);
		ut->uu_kqueue_override = THREAD_QOS_UNSPECIFIED;
	}
}

  
                                                                      
  
                                                                    
                                  
                                                    
                                                          
  
                                
   
static void
kqworkloop_unbind(struct kqworkloop *kqwl)
{
	struct kqueue *kq = &kqwl->kqwl_kqueue;
	workq_threadreq_t kqr = &kqwl->kqwl_request;
	thread_t thread = kqr_thread_fast(kqr);
	int op = KQWL_UTQ_PARKING;
	kq_index_t qos_override = THREAD_QOS_UNSPECIFIED;

	assert(thread == current_thread());

	kqlock(kqwl);

	  
                                                                    
                                                                     
                                          
    
	assert((kq->kq_state & KQ_PROCESSING) == 0);
	if (!TAILQ_EMPTY(&kqwl->kqwl_suppressed)) {
		kq->kq_state |= KQ_PROCESSING;
		qos_override = kqworkloop_acknowledge_events(kqwl);
		kq->kq_state &= ~KQ_PROCESSING;
	}

	kqworkloop_unbind_locked(kqwl, thread, KQWL_OVERRIDE_DROP_DELAYED);
	kqworkloop_update_threads_qos(kqwl, op, qos_override);

	kqunlock(kqwl);

	  
                                                                   
                                        
    
	kqworkloop_unbind_delayed_override_drop(thread);

	                                                
	kqworkloop_release(kqwl);
}

static thread_qos_t
kqworkq_unbind_locked(struct kqworkq *kqwq,
    workq_threadreq_t kqr, thread_t thread)
{
	struct uthread *ut = get_bsdthread_info(thread);
	kq_index_t old_override = kqr->tr_kq_override_index;

	KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KQWQ_UNBIND), -1,
	    thread_tid(kqr_thread(kqr)), kqr->tr_kq_qos_index, 0);

	kqlock_held(kqwq);

	assert(ut->uu_kqr_bound == kqr);
	ut->uu_kqr_bound = NULL;
	kqr->tr_thread = THREAD_NULL;
	kqr->tr_state = WORKQ_TR_STATE_IDLE;
	kqr->tr_kq_override_index = THREAD_QOS_UNSPECIFIED;
	kqwq->kqwq_state &= ~KQ_R2K_ARMED;

	return old_override;
}

  
                                                          
  
                                      
                                                               
                                                         
                                                    
                                                          
   
static void
kqworkq_unbind(proc_t p, workq_threadreq_t kqr)
{
	struct kqworkq *kqwq = (struct kqworkq *)p->p_fd->fd_wqkqueue;
	__assert_only int rc;

	kqlock(kqwq);
	rc = kqworkq_acknowledge_events(kqwq, kqr, 0, KQWQAE_UNBIND);
	assert(rc == -1);
	kqunlock(kqwq);
}

workq_threadreq_t
kqworkq_get_request(struct kqworkq *kqwq, kq_index_t qos_index)
{
	assert(qos_index < KQWQ_NBUCKETS);
	return &kqwq->kqwq_request[qos_index];
}

static void
knote_reset_priority(kqueue_t kqu, struct knote *kn, pthread_priority_t pp)
{
	kq_index_t qos = _pthread_priority_thread_qos(pp);

	if (kqu.kq->kq_state & KQ_WORKLOOP) {
		assert((pp & _PTHREAD_PRIORITY_EVENT_MANAGER_FLAG) == 0);
		pp = _pthread_priority_normalize(pp);
	} else if (kqu.kq->kq_state & KQ_WORKQ) {
		if (qos == THREAD_QOS_UNSPECIFIED) {
			                                                 
			qos = KQWQ_QOS_MANAGER;
			pp = _PTHREAD_PRIORITY_EVENT_MANAGER_FLAG;
		} else {
			pp = _pthread_priority_normalize(pp);
		}
	} else {
		pp = _pthread_unspecified_priority();
		qos = THREAD_QOS_UNSPECIFIED;
	}

	kn->kn_qos = (int32_t)pp;

	if ((kn->kn_status & KN_MERGE_QOS) == 0 || qos > kn->kn_qos_override) {
		                                          
		kn->kn_qos_override = qos;
	}

	                                                      
	if (kn->kn_status & KN_SUPPRESSED) {
		kqueue_update_override(kqu, kn, qos);
	} else if (kn->kn_qos_index != qos) {
		knote_dequeue(kqu, kn);
		kn->kn_qos_index = qos;
	}
}

static void
knote_adjust_qos(struct kqueue *kq, struct knote *kn, int result)
{
	thread_qos_t qos_index = (result >> FILTER_ADJUST_EVENT_QOS_SHIFT) & 7;

	kqlock_held(kq);

	assert(result & FILTER_ADJUST_EVENT_QOS_BIT);
	assert(qos_index < THREAD_QOS_LAST);

	  
                                                    
    
	if (__improbable(!knote_fops(kn)->f_adjusts_qos)) {
		panic("filter %d cannot change QoS", kn->kn_filtid);
	} else if (__improbable(!knote_has_qos(kn))) {
		return;
	}

	  
                                                                             
                                                                          
    
	thread_qos_t req_qos = _pthread_priority_thread_qos_fast(kn->kn_qos);
	if (kn->kn_qos & _PTHREAD_PRIORITY_FALLBACK_FLAG) {
		if (qos_index == THREAD_QOS_UNSPECIFIED) {
			qos_index = req_qos;
		}
	} else {
		if (qos_index < req_qos) {
			qos_index = req_qos;
		}
	}
	if ((kn->kn_status & KN_MERGE_QOS) && (qos_index < kn->kn_qos_override)) {
		                                          
		return;
	}

	if ((kn->kn_status & KN_LOCKED) && (kn->kn_status & KN_POSTING)) {
		  
                                                                  
                                                                         
                                                                       
                                
    
                                                                        
                                                                   
                                             
    
                                                          
                                                    
     
		kn->kn_status |= KN_MERGE_QOS;
	}

	  
                                         
    

	if (kn->kn_qos_override == qos_index) {
		return;
	}

	kn->kn_qos_override = qos_index;

	if (kn->kn_status & KN_SUPPRESSED) {
		  
                                                                          
                                                                         
    
                                                                         
                                                                          
                                                                  
     
		kqueue_update_override(kq, kn, qos_index);
	} else if (kn->kn_qos_index != qos_index) {
		knote_dequeue(kq, kn);
		kn->kn_qos_index = qos_index;
	}
}

  
                                                                            
  
                                                                       
   
void
waitq_set__CALLING_PREPOST_HOOK__(waitq_set_prepost_hook_t *kq_hook)
{
	kqueue_t kqu;

	kqu.kq = __container_of(kq_hook, struct kqueue, kq_waitq_hook);
	assert(kqu.kq->kq_state & (KQ_WORKQ | KQ_WORKLOOP));

	kqlock(kqu);

	if (kqu.kq->kq_count > 0) {
		if (kqu.kq->kq_state & KQ_WORKLOOP) {
			kqworkloop_wakeup(kqu.kqwl, KQWL_BUCKET_STAYACTIVE);
		} else {
			kqworkq_wakeup(kqu.kqwq, KQWQ_QOS_MANAGER);
		}
	}

	kqunlock(kqu);
}

void
klist_init(struct klist *list)
{
	SLIST_INIT(list);
}


  
                                             
  
                                                   
                                                   
                                                   
                                     
  
                                               
                          
   
void
knote(struct klist *list, long hint)
{
	struct knote *kn;

	SLIST_FOREACH(kn, list, kn_selnext) {
		knote_post(kn, hint);
	}
}

  
                                                                                 
                                                                                
   
int
knote_attach(struct klist *list, struct knote *kn)
{
	int ret = SLIST_EMPTY(list);
	SLIST_INSERT_HEAD(list, kn, kn_selnext);
	return ret;
}

  
                                                                                   
                                                                                
   
int
knote_detach(struct klist *list, struct knote *kn)
{
	SLIST_REMOVE(list, kn, knote, kn_selnext);
	return SLIST_EMPTY(list);
}

  
                                                       
  
                                                   
                                                     
                                    
  
                                                     
                                                      
                                                      
  
                                                       
                                                 
                                               
   
void
knote_vanish(struct klist *list, bool make_active)
{
	struct knote *kn;
	struct knote *kn_next;

	SLIST_FOREACH_SAFE(kn, list, kn_selnext, kn_next) {
		struct kqueue *kq = knote_get_kq(kn);

		kqlock(kq);
		if (__probable(kn->kn_status & KN_REQVANISH)) {
			  
                                                     
      
			kn->kn_status |= KN_VANISHED;
		} else {
			  
                                                                 
                                                                      
                                       
     
                                                                      
                                                                    
                                                                   
                                                       
      
			kn->kn_flags |= EV_EOF | EV_ONESHOT;
		}
		if (make_active) {
			knote_activate(kq, kn, FILTER_ACTIVE);
		}
		kqunlock(kq);
	}
}

  
                                               
                                       
                                  
  
                                              
                                
   
void
knote_link_waitqset_lazy_alloc(struct knote *kn)
{
	struct kqueue *kq = knote_get_kq(kn);
	waitq_set_lazy_init_link(&kq->kq_wqs);
}

  
                                                   
                           
   
boolean_t
knote_link_waitqset_should_lazy_alloc(struct knote *kn)
{
	struct kqueue *kq = knote_get_kq(kn);
	return waitq_set_should_lazy_init_link(&kq->kq_wqs);
}

  
                                                                          
                                                                               
                                                                            
                                    
  
                                                  
                          
  
                                                                                
                                                                  
   
int
knote_link_waitq(struct knote *kn, struct waitq *wq, uint64_t *reserved_link)
{
	struct kqueue *kq = knote_get_kq(kn);
	kern_return_t kr;

	kr = waitq_link(wq, &kq->kq_wqs, WAITQ_ALREADY_LOCKED, reserved_link);
	if (kr == KERN_SUCCESS) {
		knote_markstayactive(kn);
		return 0;
	} else {
		return EINVAL;
	}
}

  
                                                                          
                                                                  
  
                                                                         
                                                                         
  
                                                           
   
int
knote_unlink_waitq(struct knote *kn, struct waitq *wq)
{
	struct kqueue *kq = knote_get_kq(kn);
	kern_return_t kr;

	kr = waitq_unlink(wq, &kq->kq_wqs);
	knote_clearstayactive(kn);
	return (kr != KERN_SUCCESS) ? EINVAL : 0;
}

  
                                               
  
                                              
                                                        
   
void
knote_fdclose(struct proc *p, int fd)
{
	struct klist *list;
	struct knote *kn;
	KNOTE_LOCK_CTX(knlc);

restart:
	list = &p->p_fd->fd_knlist[fd];
	SLIST_FOREACH(kn, list, kn_link) {
		struct kqueue *kq = knote_get_kq(kn);

		kqlock(kq);

		if (kq->kq_p != p) {
			panic("%s: proc mismatch (kq->kq_p=%p != p=%p)",
			    __func__, kq->kq_p, p);
		}

		  
                                                
                                                 
                             
     
		if (kn->kn_status & KN_VANISHED) {
			kqunlock(kq);
			continue;
		}

		proc_fdunlock(p);
		if (!knote_lock(kq, kn, &knlc, KNOTE_KQ_LOCK_ON_SUCCESS)) {
			                                                     
		} else if (kn->kn_status & KN_REQVANISH) {
			kn->kn_status |= KN_VANISHED;

			kqunlock(kq);
			knote_fops(kn)->f_detach(kn);
			if (kn->kn_is_fd) {
				fp_drop(p, (int)kn->kn_id, kn->kn_fp, 0);
			}
			kn->kn_filtid = EVFILTID_DETACHED;
			kqlock(kq);

			knote_activate(kq, kn, FILTER_ACTIVE);
			knote_unlock(kq, kn, &knlc, KNOTE_KQ_UNLOCK);
		} else {
			knote_drop(kq, kn, &knlc);
		}

		proc_fdlock(p);
		goto restart;
	}
}

  
                                                            
  
                                                         
                                           
  
                                                          
                                                          
                                                           
              
  
                                                   
   
static struct knote *
knote_fdfind(struct kqueue *kq,
    const struct kevent_internal_s *kev,
    bool is_fd,
    struct proc *p)
{
	struct filedesc *fdp = p->p_fd;
	struct klist *list = NULL;
	struct knote *kn = NULL;

	  
                                         
    
	if (is_fd) {
		                                                 
		if (kev->kei_ident < (u_int)fdp->fd_knlistsize) {
			list = &fdp->fd_knlist[kev->kei_ident];
		}
	} else if (fdp->fd_knhashmask != 0) {
		                                 
		list = &fdp->fd_knhash[KN_HASH((u_long)kev->kei_ident, fdp->fd_knhashmask)];
	}

	  
                                              
    
	if (list != NULL) {
		SLIST_FOREACH(kn, list, kn_link) {
			if (kq == knote_get_kq(kn) &&
			    kev->kei_ident == kn->kn_id &&
			    kev->kei_filter == kn->kn_filter) {
				if (kev->kei_flags & EV_UDATA_SPECIFIC) {
					if ((kn->kn_flags & EV_UDATA_SPECIFIC) &&
					    kev->kei_udata == kn->kn_udata) {
						break;                                    
					}
				} else if ((kn->kn_flags & EV_UDATA_SPECIFIC) == 0) {
					break;                                        
				}
			}
		}
	}
	return kn;
}

  
                                                      
                                 
  
                                                            
                                                               
  
                                                         
                                   
  
                                                       
  
                                                             
   
static int
kq_add_knote(struct kqueue *kq, struct knote *kn, struct knote_lock_ctx *knlc,
    struct proc *p)
{
	struct filedesc *fdp = p->p_fd;
	struct klist *list = NULL;
	int ret = 0;
	bool is_fd = kn->kn_is_fd;
	uint64_t nofile = proc_limitgetcur(p, RLIMIT_NOFILE, TRUE);

	if (is_fd) {
		proc_fdlock(p);
	} else {
		knhash_lock(fdp);
	}

	if (knote_fdfind(kq, &kn->kn_kevent, is_fd, p) != NULL) {
		                                                    
		ret = ERESTART;
		goto out_locked;
	}

	                                     
	if (!is_fd) {
		if (fdp->fd_knhashmask == 0) {
			u_long size = 0;

			list = hashinit(CONFIG_KN_HASHSIZE, M_KQUEUE, &size);
			if (list == NULL) {
				ret = ENOMEM;
				goto out_locked;
			}

			fdp->fd_knhash = list;
			fdp->fd_knhashmask = size;
		}

		list = &fdp->fd_knhash[KN_HASH(kn->kn_id, fdp->fd_knhashmask)];
		SLIST_INSERT_HEAD(list, kn, kn_link);
		ret = 0;
		goto out_locked;
	} else {
		                       

		if ((u_int)fdp->fd_knlistsize <= kn->kn_id) {
			u_int size = 0;

			                                                                                                      
			if (kn->kn_id >= (uint64_t) nofile
			    || kn->kn_id >= (uint64_t)maxfilesperproc) {
				ret = EINVAL;
				goto out_locked;
			}
			                                
			size = fdp->fd_knlistsize;
			while (size <= kn->kn_id) {
				size += KQEXTENT;
			}

			if (size >= (UINT_MAX / sizeof(struct klist *))) {
				ret = EINVAL;
				goto out_locked;
			}

			list = kheap_alloc(KM_KQUEUE, size * sizeof(struct klist *),
			    Z_WAITOK);
			if (list == NULL) {
				ret = ENOMEM;
				goto out_locked;
			}

			bcopy((caddr_t)fdp->fd_knlist, (caddr_t)list,
			    fdp->fd_knlistsize * sizeof(struct klist *));
			bzero((caddr_t)list +
			    fdp->fd_knlistsize * sizeof(struct klist *),
			    (size - fdp->fd_knlistsize) * sizeof(struct klist *));
			kheap_free(KM_KQUEUE, fdp->fd_knlist,
			    fdp->fd_knlistsize * sizeof(struct klist *));
			fdp->fd_knlist = list;
			fdp->fd_knlistsize = size;
		}

		list = &fdp->fd_knlist[kn->kn_id];
		SLIST_INSERT_HEAD(list, kn, kn_link);
		ret = 0;
		goto out_locked;
	}

out_locked:
	if (ret == 0) {
		kqlock(kq);
		assert((kn->kn_status & KN_LOCKED) == 0);
		(void)knote_lock(kq, kn, knlc, KNOTE_KQ_UNLOCK);
		kqueue_retain(kq);                      
	}
	if (is_fd) {
		proc_fdunlock(p);
	} else {
		knhash_unlock(fdp);
	}

	return ret;
}

  
                                                                 
  
                                                         
                                                     
  
                                                       
   
static void
kq_remove_knote(struct kqueue *kq, struct knote *kn, struct proc *p,
    struct knote_lock_ctx *knlc)
{
	struct filedesc *fdp = p->p_fd;
	struct klist *list = NULL;
	uint16_t kq_state;
	bool is_fd = kn->kn_is_fd;

	if (is_fd) {
		proc_fdlock(p);
	} else {
		knhash_lock(fdp);
	}

	if (is_fd) {
		assert((u_int)fdp->fd_knlistsize > kn->kn_id);
		list = &fdp->fd_knlist[kn->kn_id];
	} else {
		list = &fdp->fd_knhash[KN_HASH(kn->kn_id, fdp->fd_knhashmask)];
	}
	SLIST_REMOVE(list, kn, knote, kn_link);

	kqlock(kq);
	kq_state = kq->kq_state;
	if (knlc) {
		knote_unlock_cancel(kq, kn, knlc);
	} else {
		kqunlock(kq);
	}
	if (is_fd) {
		proc_fdunlock(p);
	} else {
		knhash_unlock(fdp);
	}

	if (kq_state & KQ_DYNAMIC) {
		kqworkloop_release((struct kqworkloop *)kq);
	}
}

  
                                                                         
                                                                                            
  
                                                     
   

static struct knote *
kq_find_knote_and_kq_lock(struct kqueue *kq, struct kevent_qos_s *kev,
    bool is_fd, struct proc *p)
{
	struct filedesc *fdp = p->p_fd;
	struct knote *kn;

	if (is_fd) {
		proc_fdlock(p);
	} else {
		knhash_lock(fdp);
	}

	  
                            
                                                           
                                                            
                                                
                                     
    
	kn = knote_fdfind(kq, (struct kevent_internal_s *)kev, is_fd, p);

	if (kn) {
		kqlock(kq);
		assert(knote_get_kq(kn) == kq);
	}

	if (is_fd) {
		proc_fdunlock(p);
	} else {
		knhash_unlock(fdp);
	}

	return kn;
}

__attribute__((noinline))
static void
kqfile_wakeup(struct kqfile *kqf, __unused kq_index_t qos)
{
	                                    
	if (kqf->kqf_state & KQ_PROCESSING) {
		kqf->kqf_state |= KQ_WAKEUP;
	}

	                                           
	if (kqf->kqf_state & (KQ_SLEEP | KQ_SEL)) {
		kqf->kqf_state &= ~(KQ_SLEEP | KQ_SEL);
		waitq_wakeup64_all((struct waitq *)&kqf->kqf_wqs, KQ_EVENT,
		    THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);
	}

	                                                   
	KNOTE(&kqf->kqf_sel.si_note, 0);
}

static struct kqtailq *
knote_get_tailq(kqueue_t kqu, struct knote *kn)
{
	kq_index_t qos_index = kn->kn_qos_index;

	if (kqu.kq->kq_state & KQ_WORKLOOP) {
		assert(qos_index < KQWL_NBUCKETS);
	} else if (kqu.kq->kq_state & KQ_WORKQ) {
		assert(qos_index < KQWQ_NBUCKETS);
	} else {
		assert(qos_index == QOS_INDEX_KQFILE);
	}
	static_assert(offsetof(struct kqueue, kq_queue) == sizeof(struct kqueue),
	    "struct kqueue::kq_queue must be exactly at the end");
	return &kqu.kq->kq_queue[qos_index];
}

static void
knote_enqueue(kqueue_t kqu, struct knote *kn, kn_status_t wakeup_mask)
{
	kqlock_held(kqu);

	if ((kn->kn_status & (KN_ACTIVE | KN_STAYACTIVE)) == 0) {
		return;
	}

	if (kn->kn_status & (KN_DISABLED | KN_SUPPRESSED | KN_DROPPING)) {
		return;
	}

	if ((kn->kn_status & KN_QUEUED) == 0) {
		struct kqtailq *queue = knote_get_tailq(kqu, kn);

		TAILQ_INSERT_TAIL(queue, kn, kn_tqe);
		kn->kn_status |= KN_QUEUED;
		kqu.kq->kq_count++;
	} else if ((kn->kn_status & KN_STAYACTIVE) == 0) {
		return;
	}

	if (kn->kn_status & wakeup_mask) {
		if (kqu.kq->kq_state & KQ_WORKLOOP) {
			kqworkloop_wakeup(kqu.kqwl, kn->kn_qos_index);
		} else if (kqu.kq->kq_state & KQ_WORKQ) {
			kqworkq_wakeup(kqu.kqwq, kn->kn_qos_index);
		} else {
			kqfile_wakeup(kqu.kqf, kn->kn_qos_index);
		}
	}
}

__attribute__((always_inline))
static inline void
knote_dequeue(kqueue_t kqu, struct knote *kn)
{
	if (kn->kn_status & KN_QUEUED) {
		struct kqtailq *queue = knote_get_tailq(kqu, kn);

		                                                           
		                                                           
		                       
		kqlock_held(kqu);

		TAILQ_REMOVE(queue, kn, kn_tqe);
		kn->kn_status &= ~KN_QUEUED;
		kqu.kq->kq_count--;
	}
}

                                  
static void
knote_suppress(kqueue_t kqu, struct knote *kn)
{
	struct kqtailq *suppressq;

	kqlock_held(kqu);

	assert((kn->kn_status & KN_SUPPRESSED) == 0);
	assert(kn->kn_status & KN_QUEUED);

	knote_dequeue(kqu, kn);
	                                                       
	kn->kn_status &= ~KN_ACTIVE;
	kn->kn_status |= KN_SUPPRESSED;
	suppressq = kqueue_get_suppressed_queue(kqu, kn);
	TAILQ_INSERT_TAIL(suppressq, kn, kn_tqe);
}

__attribute__((always_inline))
static inline void
knote_unsuppress_noqueue(kqueue_t kqu, struct knote *kn)
{
	struct kqtailq *suppressq;

	kqlock_held(kqu);

	assert(kn->kn_status & KN_SUPPRESSED);

	kn->kn_status &= ~KN_SUPPRESSED;
	suppressq = kqueue_get_suppressed_queue(kqu, kn);
	TAILQ_REMOVE(suppressq, kn, kn_tqe);

	  
                                                     
                                                       
                               
    
	if ((kn->kn_status & KN_ACTIVE) == 0 && knote_has_qos(kn)) {
		kn->kn_qos_override = _pthread_priority_thread_qos_fast(kn->kn_qos);
	}
	kn->kn_qos_index = kn->kn_qos_override;
}

                                  
static void
knote_unsuppress(kqueue_t kqu, struct knote *kn)
{
	if (kn->kn_status & KN_SUPPRESSED) {
		knote_unsuppress_noqueue(kqu, kn);

		                                                            
		knote_enqueue(kqu, kn, KN_ACTIVE);
	}
}

__attribute__((always_inline))
static inline void
knote_mark_active(struct knote *kn)
{
	if ((kn->kn_status & KN_ACTIVE) == 0) {
		KDBG_DEBUG(KEV_EVTID(BSD_KEVENT_KNOTE_ACTIVATE),
		    kn->kn_udata, kn->kn_status | (kn->kn_id << 32),
		    kn->kn_filtid);
	}

	kn->kn_status |= KN_ACTIVE;
}

                                  
static void
knote_activate(kqueue_t kqu, struct knote *kn, int result)
{
	assert(result & FILTER_ACTIVE);
	if (result & FILTER_ADJUST_EVENT_QOS_BIT) {
		                        
		knote_adjust_qos(kqu.kq, kn, result);
	}
	knote_mark_active(kn);
	knote_enqueue(kqu, kn, KN_ACTIVE | KN_STAYACTIVE);
}

  
                                                                     
                                                                  
                                                                  
   
static void
knote_apply_touch(kqueue_t kqu, struct knote *kn, struct kevent_qos_s *kev,
    int result)
{
	kn_status_t wakeup_mask = KN_ACTIVE;

	if ((kev->flags & EV_ENABLE) && (kn->kn_status & KN_DISABLED)) {
		  
                                                                     
                                                                
                                           
     
		wakeup_mask |= KN_STAYACTIVE;
		kn->kn_status &= ~KN_DISABLED;

		  
                                                                      
                                                                        
    
                                                                   
                                                                         
                                                               
                                                             
    
                                                                       
                                                                    
                                                                     
                    
     
		if (__improbable(kn->kn_status & KN_SUPPRESSED)) {
			if ((kqu.kq->kq_state & KQ_PROCESSING) == 0) {
				knote_unsuppress_noqueue(kqu, kn);
			}
		}
	}

	if ((result & FILTER_UPDATE_REQ_QOS) && kev->qos && kev->qos != kn->kn_qos) {
		                        
		knote_reset_priority(kqu, kn, kev->qos);
	}

	  
                                                                   
                                                                      
                                                              
                                
    
	if (result & FILTER_ACTIVE) {
		knote_activate(kqu, kn, result);
	} else {
		knote_enqueue(kqu, kn, wakeup_mask);
	}

	if ((result & FILTER_THREADREQ_NODEFEER) &&
	    act_clear_astkevent(current_thread(), AST_KEVENT_REDRIVE_THREADREQ)) {
		workq_kern_threadreq_redrive(kqu.kq->kq_p, WORKQ_THREADREQ_NONE);
	}
}

  
                                             
  
                                                                   
  
                                                        
  
                                                
                                              
   
static void
knote_drop(struct kqueue *kq, struct knote *kn, struct knote_lock_ctx *knlc)
{
	struct proc *p = kq->kq_p;

	kqlock_held(kq);

	assert((kn->kn_status & KN_DROPPING) == 0);
	if (knlc == NULL) {
		assert((kn->kn_status & KN_LOCKED) == 0);
	}
	kn->kn_status |= KN_DROPPING;

	if (kn->kn_status & KN_SUPPRESSED) {
		knote_unsuppress_noqueue(kq, kn);
	} else {
		knote_dequeue(kq, kn);
	}
	knote_wait_for_post(kq, kn);

	knote_fops(kn)->f_detach(kn);

	                                                    
	kq_remove_knote(kq, kn, p, knlc);
	if (kn->kn_is_fd && ((kn->kn_status & KN_VANISHED) == 0)) {
		fp_drop(p, (int)kn->kn_id, kn->kn_fp, 0);
	}

	knote_free(kn);
}

void
knote_init(void)
{
#if CONFIG_MEMORYSTATUS
	                                           
	memorystatus_kevent_init(&kq_lck_grp, LCK_ATTR_NULL);
#endif
}
SYSINIT(knote, SI_SUB_PSEUDO, SI_ORDER_ANY, knote_init, NULL);

const struct filterops *
knote_fops(struct knote *kn)
{
	return sysfilt_ops[kn->kn_filtid];
}

static struct knote *
knote_alloc(void)
{
	return zalloc_flags(knote_zone, Z_WAITOK | Z_ZERO);
}

static void
knote_free(struct knote *kn)
{
	assert((kn->kn_status & (KN_LOCKED | KN_POSTING)) == 0);
	zfree(knote_zone, kn);
}

#pragma mark - syscalls: kevent, kevent64, kevent_qos, kevent_id

kevent_ctx_t
kevent_get_context(thread_t thread)
{
	uthread_t ut = get_bsdthread_info(thread);
	return &ut->uu_save.uus_kevent;
}

static inline bool
kevent_args_requesting_events(unsigned int flags, int nevents)
{
	return !(flags & KEVENT_FLAG_ERROR_EVENTS) && nevents > 0;
}

static inline int
kevent_adjust_flags_for_proc(proc_t p, int flags)
{
	__builtin_assume(p);
	return flags | (IS_64BIT_PROCESS(p) ? KEVENT_FLAG_PROC64 : 0);
}

   
                              
  
         
                         
  
              
                                        
  
                                                                      
   
OS_NOINLINE
static int
kevent_get_kqfile(struct proc *p, int fd, int flags,
    struct fileproc **fpp, struct kqueue **kqp)
{
	int error = 0;
	struct kqueue *kq;

	error = fp_get_ftype(p, fd, DTYPE_KQUEUE, EBADF, fpp);
	if (__improbable(error)) {
		return error;
	}
	kq = (struct kqueue *)(*fpp)->f_data;

	uint16_t kq_state = os_atomic_load(&kq->kq_state, relaxed);
	if (__improbable((kq_state & (KQ_KEV32 | KQ_KEV64 | KQ_KEV_QOS)) == 0)) {
		kqlock(kq);
		kq_state = kq->kq_state;
		if (!(kq_state & (KQ_KEV32 | KQ_KEV64 | KQ_KEV_QOS))) {
			if (flags & KEVENT_FLAG_LEGACY32) {
				kq_state |= KQ_KEV32;
			} else if (flags & KEVENT_FLAG_LEGACY64) {
				kq_state |= KQ_KEV64;
			} else {
				kq_state |= KQ_KEV_QOS;
			}
			kq->kq_state = kq_state;
		}
		kqunlock(kq);
	}

	  
                                                     
                                          
    
	if (__improbable((bool)(flags & KEVENT_FLAG_LEGACY32) !=
	    (bool)(kq_state & KQ_KEV32))) {
		fp_drop(p, fd, *fpp, 0);
		return EINVAL;
	}

	*kqp = kq;
	return 0;
}

   
                            
  
         
                                               
  
              
                                
   
OS_ALWAYS_INLINE
static int
kevent_get_kqwq(proc_t p, int flags, int nevents, struct kqueue **kqp)
{
	struct kqworkq *kqwq = p->p_fd->fd_wqkqueue;

	if (__improbable(kevent_args_requesting_events(flags, nevents))) {
		return EINVAL;
	}
	if (__improbable(kqwq == NULL)) {
		kqwq = kqworkq_alloc(p, flags);
		if (__improbable(kqwq == NULL)) {
			return ENOMEM;
		}
	}

	*kqp = &kqwq->kqwq_kqueue;
	return 0;
}

#pragma mark kevent copyio

   
                                 
  
         
                                                 
   
static int
kevent_get_data_size(int flags, user_addr_t data_avail, user_addr_t data_out,
    kevent_ctx_t kectx)
{
	if (!data_avail || !data_out) {
		kectx->kec_data_size  = 0;
		kectx->kec_data_resid = 0;
	} else if (flags & KEVENT_FLAG_PROC64) {
		user64_size_t usize = 0;
		int error = copyin((user_addr_t)data_avail, &usize, sizeof(usize));
		if (__improbable(error)) {
			return error;
		}
		kectx->kec_data_resid = kectx->kec_data_size = (user_size_t)usize;
	} else {
		user32_size_t usize = 0;
		int error = copyin((user_addr_t)data_avail, &usize, sizeof(usize));
		if (__improbable(error)) {
			return error;
		}
		kectx->kec_data_avail = data_avail;
		kectx->kec_data_resid = kectx->kec_data_size = (user_size_t)usize;
	}
	kectx->kec_data_out   = data_out;
	kectx->kec_data_avail = data_avail;
	return 0;
}

   
                                 
  
         
                                                                        
   
static int
kevent_put_data_size(unsigned int flags, kevent_ctx_t kectx)
{
	if (kectx->kec_data_resid == kectx->kec_data_size) {
		return 0;
	}
	if (flags & KEVENT_FLAG_KERNEL) {
		*(user_size_t *)(uintptr_t)kectx->kec_data_avail = kectx->kec_data_resid;
		return 0;
	}
	if (flags & KEVENT_FLAG_PROC64) {
		user64_size_t usize = (user64_size_t)kectx->kec_data_resid;
		return copyout(&usize, (user_addr_t)kectx->kec_data_avail, sizeof(usize));
	} else {
		user32_size_t usize = (user32_size_t)kectx->kec_data_resid;
		return copyout(&usize, (user_addr_t)kectx->kec_data_avail, sizeof(usize));
	}
}

   
                                 
  
         
                                                 
   
static int
kevent_legacy_copyin(user_addr_t *addrp, struct kevent_qos_s *kevp, unsigned int flags)
{
	int error;

	assert((flags & (KEVENT_FLAG_LEGACY32 | KEVENT_FLAG_LEGACY64)) != 0);

	if (flags & KEVENT_FLAG_LEGACY64) {
		struct kevent64_s kev64;

		error = copyin(*addrp, (caddr_t)&kev64, sizeof(kev64));
		if (__improbable(error)) {
			return error;
		}
		*addrp += sizeof(kev64);
		*kevp = (struct kevent_qos_s){
			.ident  = kev64.ident,
			.filter = kev64.filter,
			                                                     
			.flags  = kev64.flags & ~EV_SYSFLAGS,
			.udata  = kev64.udata,
			.fflags = kev64.fflags,
			.data   = kev64.data,
			.ext[0] = kev64.ext[0],
			.ext[1] = kev64.ext[1],
		};
	} else if (flags & KEVENT_FLAG_PROC64) {
		struct user64_kevent kev64;

		error = copyin(*addrp, (caddr_t)&kev64, sizeof(kev64));
		if (__improbable(error)) {
			return error;
		}
		*addrp += sizeof(kev64);
		*kevp = (struct kevent_qos_s){
			.ident  = kev64.ident,
			.filter = kev64.filter,
			                                                     
			.flags  = kev64.flags & ~EV_SYSFLAGS,
			.udata  = kev64.udata,
			.fflags = kev64.fflags,
			.data   = kev64.data,
		};
	} else {
		struct user32_kevent kev32;

		error = copyin(*addrp, (caddr_t)&kev32, sizeof(kev32));
		if (__improbable(error)) {
			return error;
		}
		*addrp += sizeof(kev32);
		*kevp = (struct kevent_qos_s){
			.ident  = (uintptr_t)kev32.ident,
			.filter = kev32.filter,
			                                                     
			.flags  = kev32.flags & ~EV_SYSFLAGS,
			.udata  = CAST_USER_ADDR_T(kev32.udata),
			.fflags = kev32.fflags,
			.data   = (intptr_t)kev32.data,
		};
	}

	return 0;
}

   
                                 
  
         
                                                      
   
static int
kevent_modern_copyin(user_addr_t *addrp, struct kevent_qos_s *kevp)
{
	int error = copyin(*addrp, (caddr_t)kevp, sizeof(struct kevent_qos_s));
	if (__probable(!error)) {
		                                                     
		*addrp += sizeof(struct kevent_qos_s);
		kevp->flags &= ~EV_SYSFLAGS;
	}
	return error;
}

   
                                  
  
         
                                                  
   
static int
kevent_legacy_copyout(struct kevent_qos_s *kevp, user_addr_t *addrp, unsigned int flags)
{
	int advance;
	int error;

	assert((flags & (KEVENT_FLAG_LEGACY32 | KEVENT_FLAG_LEGACY64)) != 0);

	  
                                                        
                                                      
                                                       
          
   
                                                        
                                                     
    
	if (flags & KEVENT_FLAG_LEGACY64) {
		struct kevent64_s kev64 = {
			.ident  = kevp->ident,
			.filter = kevp->filter,
			.flags  = kevp->flags,
			.fflags = kevp->fflags,
			.data   = (int64_t)kevp->data,
			.udata  = kevp->udata,
			.ext[0] = kevp->ext[0],
			.ext[1] = kevp->ext[1],
		};
		advance = sizeof(struct kevent64_s);
		error = copyout((caddr_t)&kev64, *addrp, advance);
	} else if (flags & KEVENT_FLAG_PROC64) {
		  
                                                  
                            
     
		uint64_t ident = (kevp->ident == (uintptr_t)-1) ?
		    (uint64_t)-1LL : (uint64_t)kevp->ident;
		struct user64_kevent kev64 = {
			.ident  = ident,
			.filter = kevp->filter,
			.flags  = kevp->flags,
			.fflags = kevp->fflags,
			.data   = (int64_t) kevp->data,
			.udata  = (user_addr_t) kevp->udata,
		};
		advance = sizeof(kev64);
		error = copyout((caddr_t)&kev64, *addrp, advance);
	} else {
		struct user32_kevent kev32 = {
			.ident  = (uint32_t)kevp->ident,
			.filter = kevp->filter,
			.flags  = kevp->flags,
			.fflags = kevp->fflags,
			.data   = (int32_t)kevp->data,
			.udata  = (uint32_t)kevp->udata,
		};
		advance = sizeof(kev32);
		error = copyout((caddr_t)&kev32, *addrp, advance);
	}
	if (__probable(!error)) {
		*addrp += advance;
	}
	return error;
}

   
                                  
  
         
                                                       
   
OS_ALWAYS_INLINE
static inline int
kevent_modern_copyout(struct kevent_qos_s *kevp, user_addr_t *addrp)
{
	int error = copyout((caddr_t)kevp, *addrp, sizeof(struct kevent_qos_s));
	if (__probable(!error)) {
		*addrp += sizeof(struct kevent_qos_s);
	}
	return error;
}

#pragma mark kevent core implementation

   
                                   
  
         
                                     
  
              
                                                            
                          
   
OS_ALWAYS_INLINE
static inline int
kevent_callback_inline(struct kevent_qos_s *kevp, kevent_ctx_t kectx, bool legacy)
{
	int error;

	assert(kectx->kec_process_noutputs < kectx->kec_process_nevents);

	  
                                                                
    
	if (legacy) {
		error = kevent_legacy_copyout(kevp, &kectx->kec_process_eventlist,
		    kectx->kec_process_flags);
	} else {
		error = kevent_modern_copyout(kevp, &kectx->kec_process_eventlist);
	}

	  
                                                      
                                                
    
	if (error == 0 && ++kectx->kec_process_noutputs == kectx->kec_process_nevents) {
		error = EWOULDBLOCK;
	}
	return error;
}

   
                                   
  
         
                                             
  
              
                                                     
   
static int
kevent_modern_callback(struct kevent_qos_s *kevp, kevent_ctx_t kectx)
{
	return kevent_callback_inline(kevp, kectx,            false);
}

   
                                   
  
         
                                             
  
              
                                                
   
static int
kevent_legacy_callback(struct kevent_qos_s *kevp, kevent_ctx_t kectx)
{
	return kevent_callback_inline(kevp, kectx,            true);
}

   
                           
  
         
                                                    
  
              
                                                          
                                             
  
                                                          
  
           
                                    
   
static int
kevent_cleanup(kqueue_t kqu, int flags, int error, kevent_ctx_t kectx)
{
	                                                    
	assert((flags & KEVENT_FLAG_POLL) == 0);

	if (flags & KEVENT_FLAG_WORKLOOP) {
		kqworkloop_release(kqu.kqwl);
	} else if (flags & KEVENT_FLAG_WORKQ) {
		                  
	} else {
		fp_drop(kqu.kqf->kqf_p, kectx->kec_fd, kectx->kec_fp, 0);
	}

	                                    
	if (error == ERESTART) {
		error = EINTR;
	} else if (error == 0) {
		                                                                          
		(void)kevent_put_data_size(flags, kectx);
	}

	if (flags & KEVENT_FLAG_PARKING) {
		thread_t th = current_thread();
		struct uthread *uth = get_bsdthread_info(th);
		if (uth->uu_kqr_bound) {
			thread_unfreeze_base_pri(th);
		}
	}
	return error;
}

   
                           
  
         
                                            
  
              
                                                                           
                                                        
  
                                                                         
                    
  
                                          
                                                       
                                                 
  
                                                                           
  
           
                                                                
                                                                  
                                                                             
                                 
                           
   
static int
kqueue_process(kqueue_t kqu, int flags, kevent_ctx_t kectx,
    kevent_callback_t callback)
{
	workq_threadreq_t kqr = current_uthread()->uu_kqr_bound;
	struct knote *kn;
	int error = 0, rc = 0;
	struct kqtailq *base_queue, *queue;
#if DEBUG || DEVELOPMENT
	int retries = 64;
#endif
	uint16_t kq_type = (kqu.kq->kq_state & (KQ_WORKQ | KQ_WORKLOOP));

	if (kq_type & KQ_WORKQ) {
		rc = kqworkq_begin_processing(kqu.kqwq, kqr, flags);
	} else if (kq_type & KQ_WORKLOOP) {
		rc = kqworkloop_begin_processing(kqu.kqwl, flags);
	} else {
kqfile_retry:
		rc = kqfile_begin_processing(kqu.kqf);
		if (rc == EBADF) {
			return EBADF;
		}
	}

	if (rc == -1) {
		                        
		return 0;
	}

	  
                                                                  
                                                             
                                                                
                                                                     
          
    

process_again:
	if (kq_type & KQ_WORKQ) {
		base_queue = queue = &kqu.kqwq->kqwq_queue[kqr->tr_kq_qos_index];
	} else if (kq_type & KQ_WORKLOOP) {
		base_queue = &kqu.kqwl->kqwl_queue[0];
		queue = &kqu.kqwl->kqwl_queue[KQWL_NBUCKETS - 1];
	} else {
		base_queue = queue = &kqu.kqf->kqf_queue;
	}

	do {
		while ((kn = TAILQ_FIRST(queue)) != NULL) {
			error = knote_process(kn, kectx, callback);
			if (error == EJUSTRETURN) {
				error = 0;
			} else if (__improbable(error)) {
				                                                           
				goto stop_processing;
			}
		}
	} while (queue-- > base_queue);

	if (kectx->kec_process_noutputs) {
		                                               
		error = EWOULDBLOCK;
	}

stop_processing:
	  
                                                                     
                                                    
   
                                                                           
                                                                           
                                                     
   
                                                                       
    
	if (error) {
		flags &= ~KEVENT_FLAG_PARKING;
	}
	if (kq_type & KQ_WORKQ) {
		rc = kqworkq_end_processing(kqu.kqwq, kqr, flags);
	} else if (kq_type & KQ_WORKLOOP) {
		rc = kqworkloop_end_processing(kqu.kqwl, KQ_PROCESSING, flags);
	} else {
		rc = kqfile_end_processing(kqu.kqf);
	}

	if (__probable(error)) {
		return error;
	}

	if (__probable(rc >= 0)) {
		assert(rc == 0 || rc == EBADF);
		return rc;
	}

#if DEBUG || DEVELOPMENT
	if (retries-- == 0) {
		panic("kevent: way too many knote_process retries, kq: %p (0x%04x)",
		    kqu.kq, kqu.kq->kq_state);
	}
#endif
	if (kq_type & (KQ_WORKQ | KQ_WORKLOOP)) {
		assert(flags & KEVENT_FLAG_PARKING);
		goto process_again;
	} else {
		goto kqfile_retry;
	}
}

   
                                 
  
         
                                                                
  
              
                                                                
  
                                                                
                                                          
   
OS_NORETURN OS_NOINLINE
static void
kqueue_scan_continue(void *data, wait_result_t wait_result)
{
	uthread_t ut = current_uthread();
	kevent_ctx_t kectx = &ut->uu_save.uus_kevent;
	int error = 0, flags = kectx->kec_process_flags;
	struct kqueue *kq = data;

	  
                                                                 
                                                     
    
	assert((flags & (KEVENT_FLAG_POLL | KEVENT_FLAG_KERNEL)) == 0);

	switch (wait_result) {
	case THREAD_AWAKENED:
		if (__improbable(flags & (KEVENT_FLAG_LEGACY32 | KEVENT_FLAG_LEGACY64))) {
			error = kqueue_scan(kq, flags, kectx, kevent_legacy_callback);
		} else {
			error = kqueue_scan(kq, flags, kectx, kevent_modern_callback);
		}
		break;
	case THREAD_TIMED_OUT:
		error = 0;
		break;
	case THREAD_INTERRUPTED:
		error = EINTR;
		break;
	case THREAD_RESTART:
		error = EBADF;
		break;
	default:
		panic("%s: - invalid wait_result (%d)", __func__, wait_result);
	}


	error = kevent_cleanup(kq, flags, error, kectx);
	*(int32_t *)&ut->uu_rval = kectx->kec_process_noutputs;
	unix_syscall_return(error);
}

   
                        
  
         
                                                                
  
              
                                            
  
                                                             
                                                                    
                                               
  
                                                                          
                                                                       
  
                                                           
                                                    
  
            
                            
  
               
                                         
  
               
                                  
                                                                      
  
                  
                                                             
                                                                           
   
int
kqueue_scan(struct kqueue *kq, int flags, kevent_ctx_t kectx,
    kevent_callback_t callback)
{
	int error;

	for (;;) {
		kqlock(kq);
		error = kqueue_process(kq, flags, kectx, callback);

		  
                                                      
                                                        
                 
     
		if (__probable(error || (flags & KEVENT_FLAG_IMMEDIATE))) {
			kqunlock(kq);
			return error == EWOULDBLOCK ? 0 : error;
		}

		waitq_assert_wait64_leeway((struct waitq *)&kq->kq_wqs,
		    KQ_EVENT, THREAD_ABORTSAFE, TIMEOUT_URGENCY_USER_NORMAL,
		    kectx->kec_deadline, TIMEOUT_NO_LEEWAY);
		kq->kq_state |= KQ_SLEEP;

		kqunlock(kq);

		if (__probable((flags & (KEVENT_FLAG_POLL | KEVENT_FLAG_KERNEL)) == 0)) {
			thread_block_parameter(kqueue_scan_continue, kq);
			__builtin_unreachable();
		}

		wait_result_t wr = thread_block(THREAD_CONTINUE_NULL);
		switch (wr) {
		case THREAD_AWAKENED:
			break;
		case THREAD_TIMED_OUT:
			return 0;
		case THREAD_INTERRUPTED:
			return EINTR;
		case THREAD_RESTART:
			return EBADF;
		default:
			panic("%s: - bad wait_result (%d)", __func__, wr);
		}
	}
}

   
                            
  
         
                      
  
              
                                                             
                       
  
                                                            
                     
                                          
  
                                                                 
                                                                   
   
OS_ALWAYS_INLINE
static inline int
kevent_internal(kqueue_t kqu,
    user_addr_t changelist, int nchanges,
    user_addr_t ueventlist, int nevents,
    int flags, kevent_ctx_t kectx, int32_t *retval,
    bool legacy)
{
	int error = 0, noutputs = 0, register_rc;

	                                                        
	if (!legacy && (flags & KEVENT_FLAG_WORKLOOP)) {
#if CONFIG_WORKLOOP_DEBUG
		UU_KEVENT_HISTORY_WRITE_ENTRY(current_uthread(), {
			.uu_kqid = kqu.kqwl->kqwl_dynamicid,
			.uu_kq = error ? NULL : kqu.kq,
			.uu_error = error,
			.uu_nchanges = nchanges,
			.uu_nevents = nevents,
			.uu_flags = flags,
		});
#endif                         

		if (flags & KEVENT_FLAG_KERNEL) {
			                               
			error = copyout(&kqu.kqwl->kqwl_dynamicid,
			    ueventlist - sizeof(kqueue_id_t), sizeof(kqueue_id_t));
			kectx->kec_data_resid -= sizeof(kqueue_id_t);
			if (__improbable(error)) {
				goto out;
			}
		}

		if (kevent_args_requesting_events(flags, nevents)) {
			  
                                                                 
                                                                    
                                     
      
			kqlock(kqu);
			kqu.kq->kq_state &= ~KQ_R2K_ARMED;
			kqunlock(kqu);
			flags |= KEVENT_FLAG_NEEDS_END_PROCESSING;
		}
	}

	                                                           
	while (nchanges > 0 && error == 0) {
		struct kevent_qos_s kev;
		struct knote *kn = NULL;

		if (legacy) {
			error = kevent_legacy_copyin(&changelist, &kev, flags);
		} else {
			error = kevent_modern_copyin(&changelist, &kev);
		}
		if (error) {
			break;
		}

		register_rc = kevent_register(kqu.kq, &kev, &kn);
		if (__improbable(!legacy && (register_rc & FILTER_REGISTER_WAIT))) {
			thread_t thread = current_thread();

			kqlock_held(kqu);

			if (act_clear_astkevent(thread, AST_KEVENT_REDRIVE_THREADREQ)) {
				workq_kern_threadreq_redrive(kqu.kq->kq_p, WORKQ_THREADREQ_NONE);
			}

			                                                                  
			                                                                
			                                                               
			                        
			  
			                                                                 
			                                                               
			if (nchanges == 1 && noutputs < nevents &&
			    (flags & KEVENT_FLAG_KERNEL) == 0 &&
			    (flags & KEVENT_FLAG_PARKING) == 0 &&
			    (flags & KEVENT_FLAG_ERROR_EVENTS) &&
			    (flags & KEVENT_FLAG_WORKLOOP)) {
				uthread_t ut = get_bsdthread_info(thread);

				  
                                                            
      
                                         
                                       
       
				ut->uu_save.uus_kevent_register = (struct _kevent_register){
					.kev        = kev,
					.kqwl       = kqu.kqwl,
					.eventout   = noutputs,
					.ueventlist = ueventlist,
				};
				knote_fops(kn)->f_post_register_wait(ut, kn,
				    &ut->uu_save.uus_kevent_register);
				__builtin_unreachable();
			}
			kqunlock(kqu);

			kev.flags |= EV_ERROR;
			kev.data = ENOTSUP;
		} else {
			assert((register_rc & FILTER_REGISTER_WAIT) == 0);
		}

		                                                  
		if (noutputs < nevents && (kev.flags & (EV_ERROR | EV_RECEIPT))) {
			if ((kev.flags & EV_ERROR) == 0) {
				kev.flags |= EV_ERROR;
				kev.data = 0;
			}
			if (legacy) {
				error = kevent_legacy_copyout(&kev, &ueventlist, flags);
			} else {
				error = kevent_modern_copyout(&kev, &ueventlist);
			}
			if (error == 0) {
				noutputs++;
			}
		} else if (kev.flags & EV_ERROR) {
			error = (int)kev.data;
		}
		nchanges--;
	}

	if ((flags & KEVENT_FLAG_ERROR_EVENTS) == 0 &&
	    nevents > 0 && noutputs == 0 && error == 0) {
		kectx->kec_process_flags = flags;
		kectx->kec_process_nevents = nevents;
		kectx->kec_process_noutputs = 0;
		kectx->kec_process_eventlist = ueventlist;

		if (legacy) {
			error = kqueue_scan(kqu.kq, flags, kectx, kevent_legacy_callback);
		} else {
			error = kqueue_scan(kqu.kq, flags, kectx, kevent_modern_callback);
		}

		noutputs = kectx->kec_process_noutputs;
	} else if (!legacy && (flags & KEVENT_FLAG_NEEDS_END_PROCESSING)) {
		  
                                                      
                           
    
                                                       
                                                
     
		kqlock(kqu);
		kqworkloop_end_processing(kqu.kqwl, 0, 0);
		kqunlock(kqu);
	}

	*retval = noutputs;
out:
	return kevent_cleanup(kqu.kq, flags, error, kectx);
}

#pragma mark modern syscalls: kevent_qos, kevent_id, kevent_workq_internal

   
                                   
  
         
                                                                       
  
              
                                                     
   
OS_NOINLINE
static int
kevent_modern_internal(kqueue_t kqu,
    user_addr_t changelist, int nchanges,
    user_addr_t ueventlist, int nevents,
    int flags, kevent_ctx_t kectx, int32_t *retval)
{
	return kevent_internal(kqu.kq, changelist, nchanges,
	           ueventlist, nevents, flags, kectx, retval,            false);
}

   
                      
  
         
                           
   
int
kevent_id(struct proc *p, struct kevent_id_args *uap, int32_t *retval)
{
	int error, flags = uap->flags & KEVENT_FLAG_USER;
	uthread_t uth = current_uthread();
	workq_threadreq_t kqr = uth->uu_kqr_bound;
	kevent_ctx_t kectx = &uth->uu_save.uus_kevent;
	kqueue_t kqu;

	flags = kevent_adjust_flags_for_proc(p, flags);
	flags |= KEVENT_FLAG_DYNAMIC_KQUEUE;

	if (__improbable((flags & (KEVENT_FLAG_WORKQ | KEVENT_FLAG_WORKLOOP)) !=
	    KEVENT_FLAG_WORKLOOP)) {
		return EINVAL;
	}

	error = kevent_get_data_size(flags, uap->data_available, uap->data_out, kectx);
	if (__improbable(error)) {
		return error;
	}

	kectx->kec_deadline = 0;
	kectx->kec_fp       = NULL;
	kectx->kec_fd       = -1;
	                                                                        

	  
                                            
                                                        
    
	kqu.kqwl = kqr ? kqr_kqworkloop(kqr) : NULL;
	if (kqu.kqwl && kqu.kqwl->kqwl_dynamicid == uap->id) {
		if (__improbable(flags & KEVENT_FLAG_DYNAMIC_KQ_MUST_NOT_EXIST)) {
			return EEXIST;
		}
		kqworkloop_retain(kqu.kqwl);
	} else if (__improbable(kevent_args_requesting_events(flags, uap->nevents))) {
		return EXDEV;
	} else {
		error = kqworkloop_get_or_create(p, uap->id, NULL, flags, &kqu.kqwl);
		if (__improbable(error)) {
			return error;
		}
	}

	return kevent_modern_internal(kqu, uap->changelist, uap->nchanges,
	           uap->eventlist, uap->nevents, flags, kectx, retval);
}

    
                                  
  
              
                                                                     
  
                            
                                                                       
                                                               
  
                                                             
                                    
                                   
                                                                               
                
  
                                                                        
                                                   
  
  
                                                                          
  
                                                                    
                                                                    
                                                                    
                                   
                                       
  
                                                                      
                                                   
  
           
                                                                                
                                                                     
  
                                                                     
   
int
kevent_workq_internal(struct proc *p,
    user_addr_t changelist, int nchanges,
    user_addr_t eventlist, int nevents,
    user_addr_t data_out, user_size_t *data_available,
    unsigned int flags, int32_t *retval)
{
	uthread_t uth = current_uthread();
	workq_threadreq_t kqr = uth->uu_kqr_bound;
	kevent_ctx_t kectx = &uth->uu_save.uus_kevent;
	kqueue_t kqu;

	assert(flags == (KEVENT_FLAG_STACK_DATA | KEVENT_FLAG_IMMEDIATE) ||
	    flags == (KEVENT_FLAG_STACK_DATA | KEVENT_FLAG_IMMEDIATE | KEVENT_FLAG_PARKING));

	kectx->kec_data_out   = data_out;
	kectx->kec_data_avail = (uint64_t)data_available;
	kectx->kec_data_size  = *data_available;
	kectx->kec_data_resid = *data_available;
	kectx->kec_deadline   = 0;
	kectx->kec_fp         = NULL;
	kectx->kec_fd         = -1;
	                                                                        

	flags = kevent_adjust_flags_for_proc(p, flags);

	if (kqr->tr_flags & WORKQ_TR_FLAG_WORKLOOP) {
		kqu.kqwl = __container_of(kqr, struct kqworkloop, kqwl_request);
		kqworkloop_retain(kqu.kqwl);

		flags |= KEVENT_FLAG_WORKLOOP | KEVENT_FLAG_DYNAMIC_KQUEUE |
		    KEVENT_FLAG_KERNEL;
	} else {
		kqu.kqwq = p->p_fd->fd_wqkqueue;

		flags |= KEVENT_FLAG_WORKQ | KEVENT_FLAG_KERNEL;
	}

	return kevent_modern_internal(kqu, changelist, nchanges,
	           eventlist, nevents, flags, kectx, retval);
}

   
                       
  
         
                            
   
int
kevent_qos(struct proc *p, struct kevent_qos_args *uap, int32_t *retval)
{
	uthread_t uth = current_uthread();
	kevent_ctx_t kectx = &uth->uu_save.uus_kevent;
	int error, flags = uap->flags & KEVENT_FLAG_USER;
	struct kqueue *kq;

	if (__improbable(flags & KEVENT_ID_FLAG_USER)) {
		return EINVAL;
	}

	flags = kevent_adjust_flags_for_proc(p, flags);

	error = kevent_get_data_size(flags, uap->data_available, uap->data_out, kectx);
	if (__improbable(error)) {
		return error;
	}

	kectx->kec_deadline = 0;
	kectx->kec_fp       = NULL;
	kectx->kec_fd       = uap->fd;
	                                                                        

	                                              
	if (__probable(flags & KEVENT_FLAG_WORKQ)) {
		error = kevent_get_kqwq(p, flags, uap->nevents, &kq);
	} else {
		error = kevent_get_kqfile(p, uap->fd, flags, &kectx->kec_fp, &kq);
	}
	if (__improbable(error)) {
		return error;
	}

	return kevent_modern_internal(kq, uap->changelist, uap->nchanges,
	           uap->eventlist, uap->nevents, flags, kectx, retval);
}

#pragma mark legacy syscalls: kevent, kevent64

   
                                       
  
         
                                                       
  
              
                                                               
                                              
  
                                                        
                             
   
static int
kevent_legacy_get_deadline(int flags, user_addr_t utimeout, uint64_t *deadline)
{
	struct timespec ts;

	if (flags & KEVENT_FLAG_PROC64) {
		struct user64_timespec ts64;
		int error = copyin(utimeout, &ts64, sizeof(ts64));
		if (__improbable(error)) {
			return error;
		}
		ts.tv_sec = (unsigned long)ts64.tv_sec;
		ts.tv_nsec = (long)ts64.tv_nsec;
	} else {
		struct user32_timespec ts32;
		int error = copyin(utimeout, &ts32, sizeof(ts32));
		if (__improbable(error)) {
			return error;
		}
		ts.tv_sec = ts32.tv_sec;
		ts.tv_nsec = ts32.tv_nsec;
	}
	if (!timespec_is_valid(&ts)) {
		return EINVAL;
	}

	clock_absolutetime_interval_to_deadline(tstoabstime(&ts), deadline);
	return 0;
}

   
                                   
  
         
                                                  
   
OS_NOINLINE
static int
kevent_legacy_internal(struct proc *p, struct kevent64_args *uap,
    int32_t *retval, int flags)
{
	uthread_t uth = current_uthread();
	kevent_ctx_t kectx = &uth->uu_save.uus_kevent;
	struct kqueue *kq;
	int error;

	if (__improbable(uap->flags & KEVENT_ID_FLAG_USER)) {
		return EINVAL;
	}

	flags = kevent_adjust_flags_for_proc(p, flags);

	kectx->kec_data_out   = 0;
	kectx->kec_data_avail = 0;
	kectx->kec_data_size  = 0;
	kectx->kec_data_resid = 0;
	kectx->kec_deadline   = 0;
	kectx->kec_fp         = NULL;
	kectx->kec_fd         = uap->fd;
	                                                                        

	                                                                      
	if (__improbable(uap->timeout && !(flags & KEVENT_FLAG_IMMEDIATE))) {
		error = kevent_legacy_get_deadline(flags, uap->timeout,
		    &kectx->kec_deadline);
		if (__improbable(error)) {
			return error;
		}
	}

	                                              
	if (flags & KEVENT_FLAG_WORKQ) {
		error = kevent_get_kqwq(p, flags, uap->nevents, &kq);
	} else {
		error = kevent_get_kqfile(p, uap->fd, flags, &kectx->kec_fp, &kq);
	}
	if (__improbable(error)) {
		return error;
	}

	return kevent_internal(kq, uap->changelist, uap->nchanges,
	           uap->eventlist, uap->nevents, flags, kectx, retval,
	                      true);
}

   
                   
  
         
                               
   
int
kevent(struct proc *p, struct kevent_args *uap, int32_t *retval)
{
	struct kevent64_args args = {
		.fd         = uap->fd,
		.changelist = uap->changelist,
		.nchanges   = uap->nchanges,
		.eventlist  = uap->eventlist,
		.nevents    = uap->nevents,
		.timeout    = uap->timeout,
	};

	return kevent_legacy_internal(p, &args, retval, KEVENT_FLAG_LEGACY32);
}

   
                     
  
         
                                 
   
int
kevent64(struct proc *p, struct kevent64_args *uap, int32_t *retval)
{
	int flags = (uap->flags & KEVENT_FLAG_USER) | KEVENT_FLAG_LEGACY64;
	return kevent_legacy_internal(p, uap, retval, flags);
}

#pragma mark - socket interface

#if SOCKETS
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/kern_event.h>
#include <sys/malloc.h>
#include <sys/sys_domain.h>
#include <sys/syslog.h>

#ifndef ROUNDUP64
#define ROUNDUP64(x) P2ROUNDUP((x), sizeof (u_int64_t))
#endif

#ifndef ADVANCE64
#define ADVANCE64(p, n) (void*)((char *)(p) + ROUNDUP64(n))
#endif

static LCK_GRP_DECLARE(kev_lck_grp, "Kernel Event Protocol");
static LCK_RW_DECLARE(kev_rwlock, &kev_lck_grp);

static int kev_attach(struct socket *so, int proto, struct proc *p);
static int kev_detach(struct socket *so);
static int kev_control(struct socket *so, u_long cmd, caddr_t data,
    struct ifnet *ifp, struct proc *p);
static lck_mtx_t * event_getlock(struct socket *, int);
static int event_lock(struct socket *, int, void *);
static int event_unlock(struct socket *, int, void *);

static int event_sofreelastref(struct socket *);
static void kev_delete(struct kern_event_pcb *);

static struct pr_usrreqs event_usrreqs = {
	.pru_attach =           kev_attach,
	.pru_control =          kev_control,
	.pru_detach =           kev_detach,
	.pru_soreceive =        soreceive,
};

static struct protosw eventsw[] = {
	{
		.pr_type =              SOCK_RAW,
		.pr_protocol =          SYSPROTO_EVENT,
		.pr_flags =             PR_ATOMIC,
		.pr_usrreqs =           &event_usrreqs,
		.pr_lock =              event_lock,
		.pr_unlock =            event_unlock,
		.pr_getlock =           event_getlock,
	}
};

__private_extern__ int kevt_getstat SYSCTL_HANDLER_ARGS;
__private_extern__ int kevt_pcblist SYSCTL_HANDLER_ARGS;

SYSCTL_NODE(_net_systm, OID_AUTO, kevt,
    CTLFLAG_RW | CTLFLAG_LOCKED, 0, "Kernel event family");

struct kevtstat kevtstat;
SYSCTL_PROC(_net_systm_kevt, OID_AUTO, stats,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED, 0, 0,
    kevt_getstat, "S,kevtstat", "");

SYSCTL_PROC(_net_systm_kevt, OID_AUTO, pcblist,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED, 0, 0,
    kevt_pcblist, "S,xkevtpcb", "");

static lck_mtx_t *
event_getlock(struct socket *so, int flags)
{
#pragma unused(flags)
	struct kern_event_pcb *ev_pcb = (struct kern_event_pcb *)so->so_pcb;

	if (so->so_pcb != NULL) {
		if (so->so_usecount < 0) {
			panic("%s: so=%p usecount=%d lrh= %s\n", __func__,
			    so, so->so_usecount, solockhistory_nr(so));
		}
		                
	} else {
		panic("%s: so=%p NULL NO so_pcb %s\n", __func__,
		    so, solockhistory_nr(so));
		                
	}
	return &ev_pcb->evp_mtx;
}

static int
event_lock(struct socket *so, int refcount, void *lr)
{
	void *lr_saved;

	if (lr == NULL) {
		lr_saved = __builtin_return_address(0);
	} else {
		lr_saved = lr;
	}

	if (so->so_pcb != NULL) {
		lck_mtx_lock(&((struct kern_event_pcb *)so->so_pcb)->evp_mtx);
	} else {
		panic("%s: so=%p NO PCB! lr=%p lrh= %s\n", __func__,
		    so, lr_saved, solockhistory_nr(so));
		                
	}

	if (so->so_usecount < 0) {
		panic("%s: so=%p so_pcb=%p lr=%p ref=%d lrh= %s\n", __func__,
		    so, so->so_pcb, lr_saved, so->so_usecount,
		    solockhistory_nr(so));
		                
	}

	if (refcount) {
		so->so_usecount++;
	}

	so->lock_lr[so->next_lock_lr] = lr_saved;
	so->next_lock_lr = (so->next_lock_lr + 1) % SO_LCKDBG_MAX;
	return 0;
}

static int
event_unlock(struct socket *so, int refcount, void *lr)
{
	void *lr_saved;
	lck_mtx_t *mutex_held;

	if (lr == NULL) {
		lr_saved = __builtin_return_address(0);
	} else {
		lr_saved = lr;
	}

	if (refcount) {
		so->so_usecount--;
	}
	if (so->so_usecount < 0) {
		panic("%s: so=%p usecount=%d lrh= %s\n", __func__,
		    so, so->so_usecount, solockhistory_nr(so));
		                
	}
	if (so->so_pcb == NULL) {
		panic("%s: so=%p NO PCB usecount=%d lr=%p lrh= %s\n", __func__,
		    so, so->so_usecount, (void *)lr_saved,
		    solockhistory_nr(so));
		                
	}
	mutex_held = (&((struct kern_event_pcb *)so->so_pcb)->evp_mtx);

	LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);
	so->unlock_lr[so->next_unlock_lr] = lr_saved;
	so->next_unlock_lr = (so->next_unlock_lr + 1) % SO_LCKDBG_MAX;

	if (so->so_usecount == 0) {
		VERIFY(so->so_flags & SOF_PCBCLEARING);
		event_sofreelastref(so);
	} else {
		lck_mtx_unlock(mutex_held);
	}

	return 0;
}

static int
event_sofreelastref(struct socket *so)
{
	struct kern_event_pcb *ev_pcb = (struct kern_event_pcb *)so->so_pcb;

	LCK_MTX_ASSERT(&(ev_pcb->evp_mtx), LCK_MTX_ASSERT_OWNED);

	so->so_pcb = NULL;

	  
                                                                   
                                                                   
                                          
    
	so->so_rcv.sb_flags &= ~SB_UPCALL;
	so->so_snd.sb_flags &= ~SB_UPCALL;
	so->so_event = sonullevent;
	lck_mtx_unlock(&(ev_pcb->evp_mtx));

	LCK_MTX_ASSERT(&(ev_pcb->evp_mtx), LCK_MTX_ASSERT_NOTOWNED);
	lck_rw_lock_exclusive(&kev_rwlock);
	LIST_REMOVE(ev_pcb, evp_link);
	kevtstat.kes_pcbcount--;
	kevtstat.kes_gencnt++;
	lck_rw_done(&kev_rwlock);
	kev_delete(ev_pcb);

	sofreelastref(so, 1);
	return 0;
}

static int event_proto_count = (sizeof(eventsw) / sizeof(struct protosw));

static
struct kern_event_head kern_event_head;

static u_int32_t static_event_id = 0;

static ZONE_DECLARE(ev_pcb_zone, "kerneventpcb",
    sizeof(struct kern_event_pcb), ZC_ZFREE_CLEARMEM);

  
                                                                             
   
void
kern_event_init(struct domain *dp)
{
	struct protosw *pr;
	int i;

	VERIFY(!(dp->dom_flags & DOM_INITIALIZED));
	VERIFY(dp == systemdomain);

	for (i = 0, pr = &eventsw[0]; i < event_proto_count; i++, pr++) {
		net_add_proto(pr, dp, 1);
	}
}

static int
kev_attach(struct socket *so, __unused int proto, __unused struct proc *p)
{
	int error = 0;
	struct kern_event_pcb *ev_pcb;

	error = soreserve(so, KEV_SNDSPACE, KEV_RECVSPACE);
	if (error != 0) {
		return error;
	}

	ev_pcb = zalloc_flags(ev_pcb_zone, Z_WAITOK | Z_ZERO);
	lck_mtx_init(&ev_pcb->evp_mtx, &kev_lck_grp, LCK_ATTR_NULL);

	ev_pcb->evp_socket = so;
	ev_pcb->evp_vendor_code_filter = 0xffffffff;

	so->so_pcb = (caddr_t) ev_pcb;
	lck_rw_lock_exclusive(&kev_rwlock);
	LIST_INSERT_HEAD(&kern_event_head, ev_pcb, evp_link);
	kevtstat.kes_pcbcount++;
	kevtstat.kes_gencnt++;
	lck_rw_done(&kev_rwlock);

	return error;
}

static void
kev_delete(struct kern_event_pcb *ev_pcb)
{
	VERIFY(ev_pcb != NULL);
	lck_mtx_destroy(&ev_pcb->evp_mtx, &kev_lck_grp);
	zfree(ev_pcb_zone, ev_pcb);
}

static int
kev_detach(struct socket *so)
{
	struct kern_event_pcb *ev_pcb = (struct kern_event_pcb *) so->so_pcb;

	if (ev_pcb != NULL) {
		soisdisconnected(so);
		so->so_flags |= SOF_PCBCLEARING;
	}

	return 0;
}

  
                                                      
             
   
errno_t
kev_vendor_code_find(
	const char      *string,
	u_int32_t       *out_vendor_code)
{
	if (strlen(string) >= KEV_VENDOR_CODE_MAX_STR_LEN) {
		return EINVAL;
	}
	return net_str_id_find_internal(string, out_vendor_code,
	           NSI_VENDOR_CODE, 1);
}

errno_t
kev_msg_post(struct kev_msg *event_msg)
{
	mbuf_tag_id_t min_vendor, max_vendor;

	net_str_id_first_last(&min_vendor, &max_vendor, NSI_VENDOR_CODE);

	if (event_msg == NULL) {
		return EINVAL;
	}

	  
                                                                     
        
    
	if (event_msg->vendor_code < min_vendor ||
	    event_msg->vendor_code > max_vendor) {
		os_atomic_inc(&kevtstat.kes_badvendor, relaxed);
		return EINVAL;
	}
	return kev_post_msg(event_msg);
}

int
kev_post_msg(struct kev_msg *event_msg)
{
	struct mbuf *m, *m2;
	struct kern_event_pcb *ev_pcb;
	struct kern_event_msg *ev;
	char *tmp;
	u_int32_t total_size;
	int i;

	                                                                       
	total_size = KEV_MSG_HEADER_SIZE;

	for (i = 0; i < 5; i++) {
		if (event_msg->dv[i].data_length == 0) {
			break;
		}
		total_size += event_msg->dv[i].data_length;
	}

	if (total_size > MLEN) {
		os_atomic_inc(&kevtstat.kes_toobig, relaxed);
		return EMSGSIZE;
	}

	m = m_get(M_WAIT, MT_DATA);
	if (m == 0) {
		os_atomic_inc(&kevtstat.kes_nomem, relaxed);
		return ENOMEM;
	}
	ev = mtod(m, struct kern_event_msg *);
	total_size = KEV_MSG_HEADER_SIZE;

	tmp = (char *) &ev->event_data[0];
	for (i = 0; i < 5; i++) {
		if (event_msg->dv[i].data_length == 0) {
			break;
		}

		total_size += event_msg->dv[i].data_length;
		bcopy(event_msg->dv[i].data_ptr, tmp,
		    event_msg->dv[i].data_length);
		tmp += event_msg->dv[i].data_length;
	}

	ev->id = ++static_event_id;
	ev->total_size   = total_size;
	ev->vendor_code  = event_msg->vendor_code;
	ev->kev_class    = event_msg->kev_class;
	ev->kev_subclass = event_msg->kev_subclass;
	ev->event_code   = event_msg->event_code;

	m->m_len = total_size;
	lck_rw_lock_shared(&kev_rwlock);
	for (ev_pcb = LIST_FIRST(&kern_event_head);
	    ev_pcb;
	    ev_pcb = LIST_NEXT(ev_pcb, evp_link)) {
		lck_mtx_lock(&ev_pcb->evp_mtx);
		if (ev_pcb->evp_socket->so_pcb == NULL) {
			lck_mtx_unlock(&ev_pcb->evp_mtx);
			continue;
		}
		if (ev_pcb->evp_vendor_code_filter != KEV_ANY_VENDOR) {
			if (ev_pcb->evp_vendor_code_filter != ev->vendor_code) {
				lck_mtx_unlock(&ev_pcb->evp_mtx);
				continue;
			}

			if (ev_pcb->evp_class_filter != KEV_ANY_CLASS) {
				if (ev_pcb->evp_class_filter != ev->kev_class) {
					lck_mtx_unlock(&ev_pcb->evp_mtx);
					continue;
				}

				if ((ev_pcb->evp_subclass_filter !=
				    KEV_ANY_SUBCLASS) &&
				    (ev_pcb->evp_subclass_filter !=
				    ev->kev_subclass)) {
					lck_mtx_unlock(&ev_pcb->evp_mtx);
					continue;
				}
			}
		}

		m2 = m_copym(m, 0, m->m_len, M_WAIT);
		if (m2 == 0) {
			os_atomic_inc(&kevtstat.kes_nomem, relaxed);
			m_free(m);
			lck_mtx_unlock(&ev_pcb->evp_mtx);
			lck_rw_done(&kev_rwlock);
			return ENOMEM;
		}
		if (sbappendrecord(&ev_pcb->evp_socket->so_rcv, m2)) {
			  
                                                    
                        
      
			so_inc_recv_data_stat(ev_pcb->evp_socket,
			    1, m->m_len, MBUF_TC_BE);

			sorwakeup(ev_pcb->evp_socket);
			os_atomic_inc(&kevtstat.kes_posted, relaxed);
		} else {
			os_atomic_inc(&kevtstat.kes_fullsock, relaxed);
		}
		lck_mtx_unlock(&ev_pcb->evp_mtx);
	}
	m_free(m);
	lck_rw_done(&kev_rwlock);

	return 0;
}

static int
kev_control(struct socket *so,
    u_long cmd,
    caddr_t data,
    __unused struct ifnet *ifp,
    __unused struct proc *p)
{
	struct kev_request *kev_req = (struct kev_request *) data;
	struct kern_event_pcb  *ev_pcb;
	struct kev_vendor_code *kev_vendor;
	u_int32_t  *id_value = (u_int32_t *) data;

	switch (cmd) {
	case SIOCGKEVID:
		*id_value = static_event_id;
		break;
	case SIOCSKEVFILT:
		ev_pcb = (struct kern_event_pcb *) so->so_pcb;
		ev_pcb->evp_vendor_code_filter = kev_req->vendor_code;
		ev_pcb->evp_class_filter = kev_req->kev_class;
		ev_pcb->evp_subclass_filter  = kev_req->kev_subclass;
		break;
	case SIOCGKEVFILT:
		ev_pcb = (struct kern_event_pcb *) so->so_pcb;
		kev_req->vendor_code = ev_pcb->evp_vendor_code_filter;
		kev_req->kev_class   = ev_pcb->evp_class_filter;
		kev_req->kev_subclass = ev_pcb->evp_subclass_filter;
		break;
	case SIOCGKEVVENDOR:
		kev_vendor = (struct kev_vendor_code *)data;
		                                         
		kev_vendor->vendor_string[KEV_VENDOR_CODE_MAX_STR_LEN - 1] = 0;
		return net_str_id_find_internal(kev_vendor->vendor_string,
		           &kev_vendor->vendor_code, NSI_VENDOR_CODE, 0);
	default:
		return ENOTSUP;
	}

	return 0;
}

int
kevt_getstat SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error = 0;

	lck_rw_lock_shared(&kev_rwlock);

	if (req->newptr != USER_ADDR_NULL) {
		error = EPERM;
		goto done;
	}
	if (req->oldptr == USER_ADDR_NULL) {
		req->oldidx = sizeof(struct kevtstat);
		goto done;
	}

	error = SYSCTL_OUT(req, &kevtstat,
	    MIN(sizeof(struct kevtstat), req->oldlen));
done:
	lck_rw_done(&kev_rwlock);

	return error;
}

__private_extern__ int
kevt_pcblist SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error = 0;
	uint64_t n, i;
	struct xsystmgen xsg;
	void *buf = NULL;
	size_t item_size = ROUNDUP64(sizeof(struct xkevtpcb)) +
	    ROUNDUP64(sizeof(struct xsocket_n)) +
	    2 * ROUNDUP64(sizeof(struct xsockbuf_n)) +
	    ROUNDUP64(sizeof(struct xsockstat_n));
	struct kern_event_pcb  *ev_pcb;

	buf = kheap_alloc(KHEAP_TEMP, item_size, Z_WAITOK | Z_ZERO);
	if (buf == NULL) {
		return ENOMEM;
	}

	lck_rw_lock_shared(&kev_rwlock);

	n = kevtstat.kes_pcbcount;

	if (req->oldptr == USER_ADDR_NULL) {
		req->oldidx = (size_t) ((n + n / 8) * item_size);
		goto done;
	}
	if (req->newptr != USER_ADDR_NULL) {
		error = EPERM;
		goto done;
	}
	bzero(&xsg, sizeof(xsg));
	xsg.xg_len = sizeof(xsg);
	xsg.xg_count = n;
	xsg.xg_gen = kevtstat.kes_gencnt;
	xsg.xg_sogen = so_gencnt;
	error = SYSCTL_OUT(req, &xsg, sizeof(xsg));
	if (error) {
		goto done;
	}
	  
                                  
    
	if (n == 0) {
		goto done;
	}

	i = 0;
	for (i = 0, ev_pcb = LIST_FIRST(&kern_event_head);
	    i < n && ev_pcb != NULL;
	    i++, ev_pcb = LIST_NEXT(ev_pcb, evp_link)) {
		struct xkevtpcb *xk = (struct xkevtpcb *)buf;
		struct xsocket_n *xso = (struct xsocket_n *)
		    ADVANCE64(xk, sizeof(*xk));
		struct xsockbuf_n *xsbrcv = (struct xsockbuf_n *)
		    ADVANCE64(xso, sizeof(*xso));
		struct xsockbuf_n *xsbsnd = (struct xsockbuf_n *)
		    ADVANCE64(xsbrcv, sizeof(*xsbrcv));
		struct xsockstat_n *xsostats = (struct xsockstat_n *)
		    ADVANCE64(xsbsnd, sizeof(*xsbsnd));

		bzero(buf, item_size);

		lck_mtx_lock(&ev_pcb->evp_mtx);

		xk->kep_len = sizeof(struct xkevtpcb);
		xk->kep_kind = XSO_EVT;
		xk->kep_evtpcb = (uint64_t)VM_KERNEL_ADDRPERM(ev_pcb);
		xk->kep_vendor_code_filter = ev_pcb->evp_vendor_code_filter;
		xk->kep_class_filter = ev_pcb->evp_class_filter;
		xk->kep_subclass_filter = ev_pcb->evp_subclass_filter;

		sotoxsocket_n(ev_pcb->evp_socket, xso);
		sbtoxsockbuf_n(ev_pcb->evp_socket ?
		    &ev_pcb->evp_socket->so_rcv : NULL, xsbrcv);
		sbtoxsockbuf_n(ev_pcb->evp_socket ?
		    &ev_pcb->evp_socket->so_snd : NULL, xsbsnd);
		sbtoxsockstat_n(ev_pcb->evp_socket, xsostats);

		lck_mtx_unlock(&ev_pcb->evp_mtx);

		error = SYSCTL_OUT(req, buf, item_size);
	}

	if (error == 0) {
		  
                                                
                                                
                                                  
                                                  
                                 
     
		bzero(&xsg, sizeof(xsg));
		xsg.xg_len = sizeof(xsg);
		xsg.xg_count = n;
		xsg.xg_gen = kevtstat.kes_gencnt;
		xsg.xg_sogen = so_gencnt;
		error = SYSCTL_OUT(req, &xsg, sizeof(xsg));
		if (error) {
			goto done;
		}
	}

done:
	lck_rw_done(&kev_rwlock);

	kheap_free(KHEAP_TEMP, buf, item_size);
	return error;
}

#endif              


int
fill_kqueueinfo(struct kqueue *kq, struct kqueue_info * kinfo)
{
	struct vinfo_stat * st;

	st = &kinfo->kq_stat;

	st->vst_size = kq->kq_count;
	if (kq->kq_state & KQ_KEV_QOS) {
		st->vst_blksize = sizeof(struct kevent_qos_s);
	} else if (kq->kq_state & KQ_KEV64) {
		st->vst_blksize = sizeof(struct kevent64_s);
	} else {
		st->vst_blksize = sizeof(struct kevent);
	}
	st->vst_mode = S_IFIFO;
	st->vst_ino = (kq->kq_state & KQ_DYNAMIC) ?
	    ((struct kqworkloop *)kq)->kqwl_dynamicid : 0;

	                                                                  
#define PROC_KQUEUE_MASK (KQ_SEL|KQ_SLEEP|KQ_KEV32|KQ_KEV64|KQ_KEV_QOS|KQ_WORKQ|KQ_WORKLOOP)
	kinfo->kq_state = kq->kq_state & PROC_KQUEUE_MASK;

	return 0;
}

static int
fill_kqueue_dyninfo(struct kqworkloop *kqwl, struct kqueue_dyninfo *kqdi)
{
	workq_threadreq_t kqr = &kqwl->kqwl_request;
	workq_threadreq_param_t trp = {};
	int err;

	if ((kqwl->kqwl_state & KQ_WORKLOOP) == 0) {
		return EINVAL;
	}

	if ((err = fill_kqueueinfo(&kqwl->kqwl_kqueue, &kqdi->kqdi_info))) {
		return err;
	}

	kqlock(kqwl);

	kqdi->kqdi_servicer = thread_tid(kqr_thread(kqr));
	kqdi->kqdi_owner = thread_tid(kqwl->kqwl_owner);
	kqdi->kqdi_request_state = kqr->tr_state;
	kqdi->kqdi_async_qos = kqr->tr_kq_qos_index;
	kqdi->kqdi_events_qos = kqr->tr_kq_override_index;
	kqdi->kqdi_sync_waiters = 0;
	kqdi->kqdi_sync_waiter_qos = 0;

	trp.trp_value = kqwl->kqwl_params;
	if (trp.trp_flags & TRP_PRIORITY) {
		kqdi->kqdi_pri = trp.trp_pri;
	} else {
		kqdi->kqdi_pri = 0;
	}

	if (trp.trp_flags & TRP_POLICY) {
		kqdi->kqdi_pol = trp.trp_pol;
	} else {
		kqdi->kqdi_pol = 0;
	}

	if (trp.trp_flags & TRP_CPUPERCENT) {
		kqdi->kqdi_cpupercent = trp.trp_cpupercent;
	} else {
		kqdi->kqdi_cpupercent = 0;
	}

	kqunlock(kqwl);

	return 0;
}


void
knote_markstayactive(struct knote *kn)
{
	struct kqueue *kq = knote_get_kq(kn);
	kq_index_t qos;

	kqlock(kq);
	kn->kn_status |= KN_STAYACTIVE;

	  
                                                                      
                                            
    
	assert((kn->kn_status & (KN_QUEUED | KN_SUPPRESSED)) == 0);

	                                                               
	if (kq->kq_state & KQ_WORKLOOP) {
		struct kqworkloop *kqwl = (struct kqworkloop *)kq;

		qos = _pthread_priority_thread_qos(kn->kn_qos);
		assert(qos && qos < THREAD_QOS_LAST);
		kqworkloop_update_threads_qos(kqwl, KQWL_UTQ_UPDATE_STAYACTIVE_QOS, qos);
		qos = KQWL_BUCKET_STAYACTIVE;
	} else if (kq->kq_state & KQ_WORKQ) {
		qos = KQWQ_QOS_MANAGER;
	} else {
		qos = THREAD_QOS_UNSPECIFIED;
	}

	kn->kn_qos_override = qos;
	kn->kn_qos_index = qos;

	knote_activate(kq, kn, FILTER_ACTIVE);
	kqunlock(kq);
}

void
knote_clearstayactive(struct knote *kn)
{
	struct kqueue *kq = knote_get_kq(kn);
	kqlock(kq);
	kn->kn_status &= ~(KN_STAYACTIVE | KN_ACTIVE);
	knote_dequeue(kq, kn);
	kqunlock(kq);
}

static unsigned long
kevent_extinfo_emit(struct kqueue *kq, struct knote *kn, struct kevent_extinfo *buf,
    unsigned long buflen, unsigned long nknotes)
{
	for (; kn; kn = SLIST_NEXT(kn, kn_link)) {
		if (kq == knote_get_kq(kn)) {
			if (nknotes < buflen) {
				struct kevent_extinfo *info = &buf[nknotes];

				kqlock(kq);

				info->kqext_kev         = *(struct kevent_qos_s *)&kn->kn_kevent;
				if (knote_has_qos(kn)) {
					info->kqext_kev.qos =
					    _pthread_priority_thread_qos_fast(kn->kn_qos);
				} else {
					info->kqext_kev.qos = kn->kn_qos_override;
				}
				info->kqext_kev.filter |= 0xff00;                         
				info->kqext_kev.xflags  = 0;                                  
				info->kqext_kev.data    = 0;                                
				info->kqext_sdata       = kn->kn_sdata;
				info->kqext_status      = kn->kn_status;
				info->kqext_sfflags     = kn->kn_sfflags;

				kqunlock(kq);
			}

			                                                                        
			nknotes++;
		}
	}

	return nknotes;
}

int
kevent_copyout_proc_dynkqids(void *proc, user_addr_t ubuf, uint32_t ubufsize,
    int32_t *nkqueues_out)
{
	proc_t p = (proc_t)proc;
	struct filedesc *fdp = p->p_fd;
	unsigned int nkqueues = 0;
	unsigned long ubuflen = ubufsize / sizeof(kqueue_id_t);
	size_t buflen, bufsize;
	kqueue_id_t *kq_ids = NULL;
	int err = 0;

	assert(p != NULL);

	if (ubuf == USER_ADDR_NULL && ubufsize != 0) {
		err = EINVAL;
		goto out;
	}

	buflen = MIN(ubuflen, PROC_PIDDYNKQUEUES_MAX);

	if (ubuflen != 0) {
		if (os_mul_overflow(sizeof(kqueue_id_t), buflen, &bufsize)) {
			err = ERANGE;
			goto out;
		}
		kq_ids = kheap_alloc(KHEAP_TEMP, bufsize, Z_WAITOK | Z_ZERO);
		if (!kq_ids) {
			err = ENOMEM;
			goto out;
		}
	}

	kqhash_lock(fdp);

	if (fdp->fd_kqhashmask > 0) {
		for (uint32_t i = 0; i < fdp->fd_kqhashmask + 1; i++) {
			struct kqworkloop *kqwl;

			LIST_FOREACH(kqwl, &fdp->fd_kqhash[i], kqwl_hashlink) {
				                                                              
				if (nkqueues < buflen) {
					kq_ids[nkqueues] = kqwl->kqwl_dynamicid;
				}
				nkqueues++;
			}
		}
	}

	kqhash_unlock(fdp);

	if (kq_ids) {
		size_t copysize;
		if (os_mul_overflow(sizeof(kqueue_id_t), MIN(buflen, nkqueues), &copysize)) {
			err = ERANGE;
			goto out;
		}

		assert(ubufsize >= copysize);
		err = copyout(kq_ids, ubuf, copysize);
	}

out:
	if (kq_ids) {
		kheap_free(KHEAP_TEMP, kq_ids, bufsize);
	}

	if (!err) {
		*nkqueues_out = (int)min(nkqueues, PROC_PIDDYNKQUEUES_MAX);
	}
	return err;
}

int
kevent_copyout_dynkqinfo(void *proc, kqueue_id_t kq_id, user_addr_t ubuf,
    uint32_t ubufsize, int32_t *size_out)
{
	proc_t p = (proc_t)proc;
	struct kqworkloop *kqwl;
	int err = 0;
	struct kqueue_dyninfo kqdi = { };

	assert(p != NULL);

	if (ubufsize < sizeof(struct kqueue_info)) {
		return ENOBUFS;
	}

	kqwl = kqworkloop_hash_lookup_and_retain(p->p_fd, kq_id);
	if (!kqwl) {
		return ESRCH;
	}

	  
                                                                      
                        
    
	if (ubufsize >= sizeof(struct kqueue_dyninfo)) {
		ubufsize = sizeof(struct kqueue_dyninfo);
		err = fill_kqueue_dyninfo(kqwl, &kqdi);
	} else {
		ubufsize = sizeof(struct kqueue_info);
		err = fill_kqueueinfo(&kqwl->kqwl_kqueue, &kqdi.kqdi_info);
	}
	if (err == 0 && (err = copyout(&kqdi, ubuf, ubufsize)) == 0) {
		*size_out = ubufsize;
	}
	kqworkloop_release(kqwl);
	return err;
}

int
kevent_copyout_dynkqextinfo(void *proc, kqueue_id_t kq_id, user_addr_t ubuf,
    uint32_t ubufsize, int32_t *nknotes_out)
{
	proc_t p = (proc_t)proc;
	struct kqworkloop *kqwl;
	int err;

	kqwl = kqworkloop_hash_lookup_and_retain(p->p_fd, kq_id);
	if (!kqwl) {
		return ESRCH;
	}

	err = pid_kqueue_extinfo(p, &kqwl->kqwl_kqueue, ubuf, ubufsize, nknotes_out);
	kqworkloop_release(kqwl);
	return err;
}

int
pid_kqueue_extinfo(proc_t p, struct kqueue *kq, user_addr_t ubuf,
    uint32_t bufsize, int32_t *retval)
{
	struct knote *kn;
	int i;
	int err = 0;
	struct filedesc *fdp = p->p_fd;
	unsigned long nknotes = 0;
	unsigned long buflen = bufsize / sizeof(struct kevent_extinfo);
	struct kevent_extinfo *kqext = NULL;

	                                                                          
	buflen = MIN(buflen, PROC_PIDFDKQUEUE_KNOTES_MAX);

	kqext = kheap_alloc(KHEAP_TEMP,
	    buflen * sizeof(struct kevent_extinfo), Z_WAITOK | Z_ZERO);
	if (kqext == NULL) {
		err = ENOMEM;
		goto out;
	}

	proc_fdlock(p);
	for (i = 0; i < fdp->fd_knlistsize; i++) {
		kn = SLIST_FIRST(&fdp->fd_knlist[i]);
		nknotes = kevent_extinfo_emit(kq, kn, kqext, buflen, nknotes);
	}
	proc_fdunlock(p);

	if (fdp->fd_knhashmask != 0) {
		for (i = 0; i < (int)fdp->fd_knhashmask + 1; i++) {
			knhash_lock(fdp);
			kn = SLIST_FIRST(&fdp->fd_knhash[i]);
			nknotes = kevent_extinfo_emit(kq, kn, kqext, buflen, nknotes);
			knhash_unlock(fdp);
		}
	}

	assert(bufsize >= sizeof(struct kevent_extinfo) * MIN(buflen, nknotes));
	err = copyout(kqext, ubuf, sizeof(struct kevent_extinfo) * MIN(buflen, nknotes));

out:
	kheap_free(KHEAP_TEMP, kqext, buflen * sizeof(struct kevent_extinfo));

	if (!err) {
		*retval = (int32_t)MIN(nknotes, PROC_PIDFDKQUEUE_KNOTES_MAX);
	}
	return err;
}

static unsigned int
klist_copy_udata(struct klist *list, uint64_t *buf,
    unsigned int buflen, unsigned int nknotes)
{
	struct knote *kn;
	SLIST_FOREACH(kn, list, kn_link) {
		if (nknotes < buflen) {
			  
                                                         
                                                    
      
			buf[nknotes] = os_atomic_load_wide(&kn->kn_udata, relaxed);
		}
		                                                                        
		nknotes++;
	}

	return nknotes;
}

int
kevent_proc_copy_uptrs(void *proc, uint64_t *buf, uint32_t bufsize)
{
	proc_t p = (proc_t)proc;
	struct filedesc *fdp = p->p_fd;
	unsigned int nuptrs = 0;
	unsigned int buflen = bufsize / sizeof(uint64_t);
	struct kqworkloop *kqwl;

	if (buflen > 0) {
		assert(buf != NULL);
	}

	proc_fdlock(p);
	for (int i = 0; i < fdp->fd_knlistsize; i++) {
		nuptrs = klist_copy_udata(&fdp->fd_knlist[i], buf, buflen, nuptrs);
	}
	proc_fdunlock(p);

	knhash_lock(fdp);
	if (fdp->fd_knhashmask != 0) {
		for (size_t i = 0; i < fdp->fd_knhashmask + 1; i++) {
			nuptrs = klist_copy_udata(&fdp->fd_knhash[i], buf, buflen, nuptrs);
		}
	}
	knhash_unlock(fdp);

	kqhash_lock(fdp);
	if (fdp->fd_kqhashmask != 0) {
		for (size_t i = 0; i < fdp->fd_kqhashmask + 1; i++) {
			LIST_FOREACH(kqwl, &fdp->fd_kqhash[i], kqwl_hashlink) {
				if (nuptrs < buflen) {
					buf[nuptrs] = kqwl->kqwl_dynamicid;
				}
				nuptrs++;
			}
		}
	}
	kqhash_unlock(fdp);

	return (int)nuptrs;
}

static void
kevent_set_return_to_kernel_user_tsd(proc_t p, thread_t thread)
{
	uint64_t ast_addr;
	bool proc_is_64bit = !!(p->p_flag & P_LP64);
	size_t user_addr_size = proc_is_64bit ? 8 : 4;
	uint32_t ast_flags32 = 0;
	uint64_t ast_flags64 = 0;
	struct uthread *ut = get_bsdthread_info(thread);

	if (ut->uu_kqr_bound != NULL) {
		ast_flags64 |= R2K_WORKLOOP_PENDING_EVENTS;
	}

	if (ast_flags64 == 0) {
		return;
	}

	if (!(p->p_flag & P_LP64)) {
		ast_flags32 = (uint32_t)ast_flags64;
		assert(ast_flags64 < 0x100000000ull);
	}

	ast_addr = thread_rettokern_addr(thread);
	if (ast_addr == 0) {
		return;
	}

	if (copyout((proc_is_64bit ? (void *)&ast_flags64 : (void *)&ast_flags32),
	    (user_addr_t)ast_addr,
	    user_addr_size) != 0) {
		printf("pid %d (tid:%llu): copyout of return_to_kernel ast flags failed with "
		    "ast_addr = %llu\n", p->p_pid, thread_tid(current_thread()), ast_addr);
	}
}

void
kevent_ast(thread_t thread, uint16_t bits)
{
	proc_t p = current_proc();

	if (bits & AST_KEVENT_REDRIVE_THREADREQ) {
		workq_kern_threadreq_redrive(p, WORKQ_THREADREQ_CAN_CREATE_THREADS);
	}
	if (bits & AST_KEVENT_RETURN_TO_KERNEL) {
		kevent_set_return_to_kernel_user_tsd(p, thread);
	}
}

#if DEVELOPMENT || DEBUG

#define KEVENT_SYSCTL_BOUND_ID 1

static int
kevent_sysctl SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg2)
	uintptr_t type = (uintptr_t)arg1;
	uint64_t bound_id = 0;

	if (type != KEVENT_SYSCTL_BOUND_ID) {
		return EINVAL;
	}

	if (req->newptr) {
		return EINVAL;
	}

	struct uthread *ut = get_bsdthread_info(current_thread());
	if (!ut) {
		return EFAULT;
	}

	workq_threadreq_t kqr = ut->uu_kqr_bound;
	if (kqr) {
		if (kqr->tr_flags & WORKQ_TR_FLAG_WORKLOOP) {
			bound_id = kqr_kqworkloop(kqr)->kqwl_dynamicid;
		} else {
			bound_id = -1;
		}
	}

	return sysctl_io_number(req, bound_id, sizeof(bound_id), NULL, NULL);
}

SYSCTL_NODE(_kern, OID_AUTO, kevent, CTLFLAG_RW | CTLFLAG_LOCKED, 0,
    "kevent information");

SYSCTL_PROC(_kern_kevent, OID_AUTO, bound_id,
    CTLTYPE_QUAD | CTLFLAG_RD | CTLFLAG_LOCKED | CTLFLAG_MASKED,
    (void *)KEVENT_SYSCTL_BOUND_ID,
    sizeof(kqueue_id_t), kevent_sysctl, "Q",
    "get the ID of the bound kqueue");

#endif                           
