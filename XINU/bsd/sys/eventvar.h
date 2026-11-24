/*
 * Copyright (c) 2003 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1999,2000 Jonathan Lemon <jlemon@FreeBSD.org>
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
 *
 *	$FreeBSD: src/sys/sys/eventvar.h,v 1.1.2.2 2000/07/18 21:49:12 jlemon Exp $
 */

#ifndef _SYS_EVENTVAR_H_
#define _SYS_EVENTVAR_H_

#include <sys/event.h>
#include <sys/select.h>
#include <kern/kern_types.h>
#include <kern/waitq.h>

#if defined(XNU_KERNEL_PRIVATE)

typedef int (*kevent_callback_t)(struct kevent_qos_s *, struct kevent_ctx_s *);

#include <stdint.h>
#include <kern/locks.h>
#include <mach/thread_policy.h>
#include <pthread/workqueue_internal.h>

  
                 
  
                                                                
  
                                            
                                                                  
  
                                       
                                                           
  
                                                                       
                                                                         
                                                                        
                                     
  
                                            
                                                              
  
                                       
                                                       
   

#define KQEXTENT        256                                               

struct knote_lock_ctx {
	struct knote               *knlc_knote;
	thread_t                    knlc_thread;
	uintptr_t                   knlc_waiters;
	LIST_ENTRY(knote_lock_ctx)  knlc_link;
#if DEBUG || DEVELOPMENT
#define KNOTE_LOCK_CTX_UNLOCKED 0
#define KNOTE_LOCK_CTX_LOCKED   1
#define KNOTE_LOCK_CTX_WAITING  2
	int knlc_state;
#endif
};
LIST_HEAD(knote_locks, knote_lock_ctx);

#if DEBUG || DEVELOPMENT
  
                                                                                
                                                                              
                                                                                
   
static inline void
knote_lock_ctx_chk(struct knote_lock_ctx *knlc)
{
	                                                        
	assert(knlc->knlc_state == KNOTE_LOCK_CTX_UNLOCKED);
}
#define KNOTE_LOCK_CTX(n) \
	        struct knote_lock_ctx n __attribute__((cleanup(knote_lock_ctx_chk))); \
	        n.knlc_state = KNOTE_LOCK_CTX_UNLOCKED
#else
#define KNOTE_LOCK_CTX(n) \
	        struct knote_lock_ctx n
#endif


__options_decl(kq_state_t, uint16_t, {
	KQ_SEL            = 0x0001,                                 
	KQ_SLEEP          = 0x0002,                                   
	KQ_PROCWAIT       = 0x0004,                                    
	KQ_KEV32          = 0x0008,                                    
	KQ_KEV64          = 0x0010,                                    
	KQ_KEV_QOS        = 0x0020,                               
	KQ_WORKQ          = 0x0040,                                   
	KQ_WORKLOOP       = 0x0080,                               
	KQ_PROCESSING     = 0x0100,                            
	KQ_DRAIN          = 0x0200,                     
	KQ_WAKEUP         = 0x0400,                                   
	KQ_DYNAMIC        = 0x0800,                                    
	KQ_R2K_ARMED      = 0x1000,                             
	KQ_HAS_TURNSTILE  = 0x2000,                                  
});

  
                                              
  
                                                                   
                                                                       
                                         
   
struct kqueue {
	struct {
		struct waitq_set    kq_wqs;                              
		lck_spin_t          kq_lock;                       
		kq_state_t          kq_state;                          
		union {
			uint16_t    kq_waitq_hook;                              
			uint16_t    kq_level;                                  
		};
		uint32_t            kq_count;                                  
		struct proc        *kq_p;                                        
		struct knote_locks  kq_knlocks;                                 
	};                                                      
	struct kqtailq      kq_queue[0];                                    
};

  
                                                                      
                                         
  
                                                                        
                                         
   
struct kqfile {
	struct kqueue       kqf_kqueue;                             
	struct kqtailq      kqf_queue;                                    
	struct kqtailq      kqf_suppressed;                        
	struct selinfo      kqf_sel;                                       
#define kqf_wqs      kqf_kqueue.kq_wqs
#define kqf_lock     kqf_kqueue.kq_lock
#define kqf_state    kqf_kqueue.kq_state
#define kqf_level    kqf_kqueue.kq_level
#define kqf_count    kqf_kqueue.kq_count
#define kqf_p        kqf_kqueue.kq_p
};

#define QOS_INDEX_KQFILE   0                                                 

  
                                                                 
                                                          
                                                                
                                                                   
                                                               
                                             
   

#if !defined(KQWQ_QOS_MANAGER)
#define KQWQ_QOS_MANAGER (THREAD_QOS_LAST)
#endif

#if !defined(KQWQ_NBUCKETS)
#define KQWQ_NBUCKETS    (KQWQ_QOS_MANAGER + 1)
#endif

  
                                                                    
                                              
  
                                                                              
                                                                                  
                    
   
struct kqworkq {
	struct kqueue       kqwq_kqueue;
	struct kqtailq      kqwq_queue[KQWQ_NBUCKETS];                            
	struct kqtailq      kqwq_suppressed[KQWQ_NBUCKETS];                                  
	workq_threadreq_s   kqwq_request[KQWQ_NBUCKETS];                                 
};

#define kqwq_wqs         kqwq_kqueue.kq_wqs
#define kqwq_lock        kqwq_kqueue.kq_lock
#define kqwq_state       kqwq_kqueue.kq_state
#define kqwq_waitq_hook  kqwq_kqueue.kq_waitq_hook
#define kqwq_count       kqwq_kqueue.kq_count
#define kqwq_p           kqwq_kqueue.kq_p

  
                                                                     
                                                      
                                                                
                                                               
                                                                
                                                                   
                                           
   

  
                                                                    
                                                                
                                                                   
                                                                  
                                                                  
                                                                
                                                                   
                                
   
#define KQWL_BUCKET_STAYACTIVE (THREAD_QOS_LAST)

#if !defined(KQWL_NBUCKETS)
#define KQWL_NBUCKETS    (KQWL_BUCKET_STAYACTIVE + 1)
#endif

  
                                                                       
                                               
  
                                                                               
                                                                              
                                                                       
                                                                                 
                                                                                   
                          
  
                                      
   
struct kqworkloop {
	struct kqueue       kqwl_kqueue;                                       
	struct kqtailq      kqwl_queue[KQWL_NBUCKETS];                         
	struct kqtailq      kqwl_suppressed;                                              
	workq_threadreq_s   kqwl_request;                                           
	lck_spin_t          kqwl_statelock;                                        
	thread_t            kqwl_owner;                                                    
	uint32_t            kqwl_retains;                                        
#define KQWL_STAYACTIVE_FIRED_BIT     (1 << 0)
	uint8_t             kqwl_wakeup_indexes;                                             
	kq_index_t          kqwl_stayactive_qos;                                             
	struct turnstile   *kqwl_turnstile;                                                   
	kqueue_id_t         kqwl_dynamicid;                                     
	uint64_t            kqwl_params;                                             
	LIST_ENTRY(kqworkloop) kqwl_hashlink;                                          
#if CONFIG_WORKLOOP_DEBUG
#define KQWL_HISTORY_COUNT 32
#define KQWL_HISTORY_WRITE_ENTRY(kqwl, ...) ({ \
	        struct kqworkloop *__kqwl = (kqwl); \
	        unsigned int __index = os_atomic_inc_orig(&__kqwl->kqwl_index, relaxed); \
	        __kqwl->kqwl_history[__index % KQWL_HISTORY_COUNT] = \
	                        (struct kqwl_history)__VA_ARGS__; \
	})
	struct kqwl_history {
		thread_t updater;                                      
		thread_t servicer;
		thread_t old_owner;
		thread_t new_owner;

		uint64_t kev_ident;
		int16_t  error;
		uint16_t kev_flags;
		uint32_t kev_fflags;

		uint64_t kev_mask;
		uint64_t kev_value;
		uint64_t in_value;
	} kqwl_history[KQWL_HISTORY_COUNT];
	unsigned int kqwl_index;
#endif                         
};
LIST_HEAD(kqwllist, kqworkloop);

typedef union {
	struct kqueue       *kq;
	struct kqworkq      *kqwq;
	struct kqfile       *kqf;
	struct kqworkloop   *kqwl;
} __attribute__((transparent_union)) kqueue_t;


#define kqwl_wqs         kqwl_kqueue.kq_wqs
#define kqwl_lock        kqwl_kqueue.kq_lock
#define kqwl_state       kqwl_kqueue.kq_state
#define kqwl_waitq_hook  kqwl_kqueue.kq_waitq_hook
#define kqwl_count       kqwl_kqueue.kq_count
#define kqwl_p           kqwl_kqueue.kq_p

#define KQ_WORKLOOP_RETAINS_MAX UINT32_MAX

extern void kqueue_threadreq_unbind(struct proc *p, workq_threadreq_t);

                              
#define KQUEUE_THREADERQ_BIND_NO_INHERITOR_UPDATE 0x1
extern void kqueue_threadreq_bind(struct proc *p, workq_threadreq_t req,
    thread_t thread, unsigned int flags);

struct turnstile *kqueue_threadreq_get_turnstile(workq_threadreq_t kqr);

                               
extern void kqueue_threadreq_bind_prepost(struct proc *p, workq_threadreq_t req,
    struct uthread *uth);

                           
extern void kqueue_threadreq_bind_commit(struct proc *p, thread_t thread);

extern void kqueue_threadreq_cancel(struct proc *p, workq_threadreq_t req);

                                                           
extern workq_threadreq_param_t kqueue_threadreq_workloop_param(workq_threadreq_t req);

extern struct kqueue *kqueue_alloc(struct proc *);
extern void kqueue_dealloc(struct kqueue *);
extern void kqworkq_dealloc(struct kqworkq *kqwq);

extern void knotes_dealloc(struct proc *);
extern void kqworkloops_dealloc(struct proc *);

extern int kevent_register(struct kqueue *, struct kevent_qos_s *,
    struct knote **);
extern int kqueue_scan(struct kqueue *, int flags,
    struct kevent_ctx_s *, kevent_callback_t);
extern int kqueue_stat(struct kqueue *, void *, int, proc_t);

#endif                         

#endif                        
