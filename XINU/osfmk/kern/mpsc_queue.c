/*
 * Copyright (c) 2018 Apple Inc. All rights reserved.
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

#include <machine/machine_cpu.h>
#include <kern/locks.h>
#include <kern/mpsc_queue.h>
#include <kern/thread.h>

#pragma mark Single Consumer calls

__attribute__((noinline))
static mpsc_queue_chain_t
_mpsc_queue_wait_for_enqueuer(struct mpsc_queue_chain *_Atomic *ptr)
{
	return hw_wait_while_equals((void **)ptr, NULL);
}

void
mpsc_queue_restore_batch(mpsc_queue_head_t q, mpsc_queue_chain_t first,
    mpsc_queue_chain_t last)
{
	mpsc_queue_chain_t head = os_atomic_load(&q->mpqh_head.mpqc_next, relaxed);

	os_atomic_store(&last->mpqc_next, head, relaxed);

	if (head == NULL &&
	    !os_atomic_cmpxchg(&q->mpqh_tail, &q->mpqh_head, last, release)) {
		head = os_atomic_load(&q->mpqh_head.mpqc_next, relaxed);
		if (__improbable(head == NULL)) {
			head = _mpsc_queue_wait_for_enqueuer(&q->mpqh_head.mpqc_next);
		}
		os_atomic_store(&last->mpqc_next, head, relaxed);
	}

	os_atomic_store(&q->mpqh_head.mpqc_next, first, relaxed);
}

mpsc_queue_chain_t
mpsc_queue_dequeue_batch(mpsc_queue_head_t q, mpsc_queue_chain_t *tail_out,
    os_atomic_dependency_t dependency)
{
	mpsc_queue_chain_t head, tail;

	q = os_atomic_inject_dependency(q, dependency);

	tail = os_atomic_load(&q->mpqh_tail, relaxed);
	if (__improbable(tail == &q->mpqh_head)) {
		*tail_out = NULL;
		return NULL;
	}

	head = os_atomic_load(&q->mpqh_head.mpqc_next, relaxed);
	if (__improbable(head == NULL)) {
		head = _mpsc_queue_wait_for_enqueuer(&q->mpqh_head.mpqc_next);
	}
	os_atomic_store(&q->mpqh_head.mpqc_next, NULL, relaxed);
	  
                                                                        
                                                                     
   
                                                                            
                                                                           
                                      
   
                                                                     
                                                                     
                                            
   
                                                                       
                                             
    
	*tail_out = os_atomic_xchg(&q->mpqh_tail, &q->mpqh_head, seq_cst);

	return head;
}

mpsc_queue_chain_t
mpsc_queue_batch_next(mpsc_queue_chain_t cur, mpsc_queue_chain_t tail)
{
	mpsc_queue_chain_t elm = NULL;
	if (cur == tail || cur == NULL) {
		return elm;
	}

	elm = os_atomic_load(&cur->mpqc_next, relaxed);
	if (__improbable(elm == NULL)) {
		elm = _mpsc_queue_wait_for_enqueuer(&cur->mpqc_next);
	}
	return elm;
}

#pragma mark "GCD"-like facilities

static void _mpsc_daemon_queue_drain(mpsc_daemon_queue_t, thread_t);
static void _mpsc_daemon_queue_enqueue(mpsc_daemon_queue_t, mpsc_queue_chain_t);

                         

static void
_mpsc_queue_thread_continue(void *param, wait_result_t wr __unused)
{
	mpsc_daemon_queue_t dq = param;
	mpsc_daemon_queue_kind_t kind = dq->mpd_kind;
	thread_t self = dq->mpd_thread;

	__builtin_assume(self != THREAD_NULL);

	if (kind == MPSC_QUEUE_KIND_THREAD_CRITICAL) {
		self->options |= TH_OPT_SYSTEM_CRITICAL;
	}

	assert(dq->mpd_thread == current_thread());
	_mpsc_daemon_queue_drain(dq, self);

	if (kind == MPSC_QUEUE_KIND_THREAD_CRITICAL) {
		self->options &= ~TH_OPT_SYSTEM_CRITICAL;
	}

	thread_block_parameter(_mpsc_queue_thread_continue, dq);
}

static void
_mpsc_queue_thread_wakeup(mpsc_daemon_queue_t dq)
{
	thread_wakeup_thread((event_t)dq, dq->mpd_thread);
}

static kern_return_t
_mpsc_daemon_queue_init_with_thread(mpsc_daemon_queue_t dq,
    mpsc_daemon_invoke_fn_t invoke, int pri, const char *name,
    mpsc_daemon_queue_kind_t kind)
{
	kern_return_t kr;

	*dq = (struct mpsc_daemon_queue){
		.mpd_kind   = kind,
		.mpd_invoke = invoke,
		.mpd_queue  = MPSC_QUEUE_INITIALIZER(dq->mpd_queue),
		.mpd_chain  = { MPSC_QUEUE_NOTQUEUED_MARKER },
	};

	kr = kernel_thread_create(_mpsc_queue_thread_continue, dq, pri,
	    &dq->mpd_thread);
	if (kr == KERN_SUCCESS) {
		thread_set_thread_name(dq->mpd_thread, name);
		thread_start_in_assert_wait(dq->mpd_thread, (event_t)dq, THREAD_UNINT);
		thread_deallocate(dq->mpd_thread);
	}
	return kr;
}

kern_return_t
mpsc_daemon_queue_init_with_thread(mpsc_daemon_queue_t dq,
    mpsc_daemon_invoke_fn_t invoke, int pri, const char *name)
{
	return _mpsc_daemon_queue_init_with_thread(dq, invoke, pri, name,
	           MPSC_QUEUE_KIND_THREAD);
}

                              

static void
_mpsc_queue_thread_call_drain(thread_call_param_t arg0,
    thread_call_param_t arg1 __unused)
{
	_mpsc_daemon_queue_drain((mpsc_daemon_queue_t)arg0, NULL);
}

static void
_mpsc_queue_thread_call_wakeup(mpsc_daemon_queue_t dq)
{
	thread_call_enter(dq->mpd_call);
}

void
mpsc_daemon_queue_init_with_thread_call(mpsc_daemon_queue_t dq,
    mpsc_daemon_invoke_fn_t invoke, thread_call_priority_t pri)
{
	*dq = (struct mpsc_daemon_queue){
		.mpd_kind   = MPSC_QUEUE_KIND_THREAD_CALL,
		.mpd_invoke = invoke,
		.mpd_queue  = MPSC_QUEUE_INITIALIZER(dq->mpd_queue),
		.mpd_chain  = { MPSC_QUEUE_NOTQUEUED_MARKER },
	};
	dq->mpd_call = thread_call_allocate_with_options(
		_mpsc_queue_thread_call_drain, dq, pri, THREAD_CALL_OPTIONS_ONCE);
}

                   

void
mpsc_daemon_queue_nested_invoke(mpsc_queue_chain_t elm,
    __unused mpsc_daemon_queue_t tq)
{
	mpsc_daemon_queue_t dq;
	dq = mpsc_queue_element(elm, struct mpsc_daemon_queue, mpd_chain);
	_mpsc_daemon_queue_drain(dq, NULL);
}

static void
_mpsc_daemon_queue_nested_wakeup(mpsc_daemon_queue_t dq)
{
	_mpsc_daemon_queue_enqueue(dq->mpd_target, &dq->mpd_chain);
}

void
mpsc_daemon_queue_init_with_target(mpsc_daemon_queue_t dq,
    mpsc_daemon_invoke_fn_t invoke, mpsc_daemon_queue_t target)
{
	*dq = (struct mpsc_daemon_queue){
		.mpd_kind   = MPSC_QUEUE_KIND_NESTED,
		.mpd_invoke = invoke,
		.mpd_target = target,
		.mpd_queue  = MPSC_QUEUE_INITIALIZER(dq->mpd_queue),
		.mpd_chain  = { MPSC_QUEUE_NOTQUEUED_MARKER },
	};
}

                                  

static void
_mpsc_daemon_queue_drain(mpsc_daemon_queue_t dq, thread_t self)
{
	mpsc_daemon_invoke_fn_t invoke = dq->mpd_invoke;
	mpsc_queue_chain_t head, cur, tail;
	mpsc_daemon_queue_state_t st;

again:
	  
                                                        
                                                                      
                 
   
                                                                           
                                                                         
                                                                     
                                                                      
                             
   
                                                                       
                                                                             
                                                                          
    
	st = os_atomic_xor(&dq->mpd_state,
	    MPSC_QUEUE_STATE_DRAINING | MPSC_QUEUE_STATE_WAKEUP, dependency);
	assert(st & MPSC_QUEUE_STATE_DRAINING);
	if (__improbable(st & MPSC_QUEUE_STATE_WAKEUP)) {
		assert(st & MPSC_QUEUE_STATE_CANCELED);
		os_atomic_andnot(&dq->mpd_state, MPSC_QUEUE_STATE_WAKEUP, relaxed);
	}

	os_atomic_dependency_t dep = os_atomic_make_dependency((uintptr_t)st);
	while ((head = mpsc_queue_dequeue_batch(&dq->mpd_queue, &tail, dep))) {
		mpsc_queue_batch_foreach_safe(cur, head, tail) {
			os_atomic_store(&cur->mpqc_next,
			    MPSC_QUEUE_NOTQUEUED_MARKER, relaxed);
			invoke(cur, dq);
		}
	}

	if (self) {
		assert_wait((event_t)dq, THREAD_UNINT);
	}

	  
                                                                      
                                                                          
                                                                        
   
                                                                             
                                        
    
	st = os_atomic_andnot(&dq->mpd_state, MPSC_QUEUE_STATE_DRAINING, relaxed);

	  
                                                 
                                                            
                                
   
                                                           
                                                             
                                                                     
                                
    
	if (__improbable(st & MPSC_QUEUE_STATE_WAKEUP)) {
		if (self) {
			clear_wait(self, THREAD_AWAKENED);
		}
		goto again;
	}

	                                                  

	if (__improbable(st & MPSC_QUEUE_STATE_CANCELED)) {
		thread_wakeup(&dq->mpd_state);
		if (self) {
			clear_wait(self, THREAD_AWAKENED);
			thread_terminate_self();
			__builtin_unreachable();
		}
	}
}

static void
_mpsc_daemon_queue_wakeup(mpsc_daemon_queue_t dq)
{
	switch (dq->mpd_kind) {
	case MPSC_QUEUE_KIND_NESTED:
		_mpsc_daemon_queue_nested_wakeup(dq);
		break;
	case MPSC_QUEUE_KIND_THREAD:
	case MPSC_QUEUE_KIND_THREAD_CRITICAL:
		_mpsc_queue_thread_wakeup(dq);
		break;
	case MPSC_QUEUE_KIND_THREAD_CALL:
		_mpsc_queue_thread_call_wakeup(dq);
		break;
	default:
		panic("mpsc_queue[%p]: invalid kind (%d)", dq, dq->mpd_kind);
	}
}

static void
_mpsc_daemon_queue_enqueue(mpsc_daemon_queue_t dq, mpsc_queue_chain_t elm)
{
	mpsc_daemon_queue_state_t st;

	if (mpsc_queue_append(&dq->mpd_queue, elm)) {
		  
                                                                
     
		st = os_atomic_or_orig(&dq->mpd_state, MPSC_QUEUE_STATE_WAKEUP, release);
		if (__improbable(st & MPSC_QUEUE_STATE_CANCELED)) {
			panic("mpsc_queue[%p]: use after cancelation", dq);
		}

		if ((st & (MPSC_QUEUE_STATE_DRAINING | MPSC_QUEUE_STATE_WAKEUP)) == 0) {
			_mpsc_daemon_queue_wakeup(dq);
		}
	}
}

void
mpsc_daemon_enqueue(mpsc_daemon_queue_t dq, mpsc_queue_chain_t elm,
    mpsc_queue_options_t options)
{
	if (options & MPSC_QUEUE_DISABLE_PREEMPTION) {
		disable_preemption();
	}

	_mpsc_daemon_queue_enqueue(dq, elm);

	if (options & MPSC_QUEUE_DISABLE_PREEMPTION) {
		enable_preemption();
	}
}

void
mpsc_daemon_queue_cancel_and_wait(mpsc_daemon_queue_t dq)
{
	mpsc_daemon_queue_state_t st;

	assert_wait((event_t)&dq->mpd_state, THREAD_UNINT);

	st = os_atomic_or_orig(&dq->mpd_state, MPSC_QUEUE_STATE_CANCELED, relaxed);
	if (__improbable(st & MPSC_QUEUE_STATE_CANCELED)) {
		panic("mpsc_queue[%p]: cancelled twice (%x)", dq, st);
	}

	if (dq->mpd_kind == MPSC_QUEUE_KIND_NESTED && st == 0) {
		clear_wait(current_thread(), THREAD_AWAKENED);
	} else {
		disable_preemption();
		_mpsc_daemon_queue_wakeup(dq);
		enable_preemption();
		thread_block(THREAD_CONTINUE_NULL);
	}

	switch (dq->mpd_kind) {
	case MPSC_QUEUE_KIND_NESTED:
		dq->mpd_target = NULL;
		break;
	case MPSC_QUEUE_KIND_THREAD:
	case MPSC_QUEUE_KIND_THREAD_CRITICAL:
		dq->mpd_thread = NULL;
		break;
	case MPSC_QUEUE_KIND_THREAD_CALL:
		thread_call_cancel_wait(dq->mpd_call);
		thread_call_free(dq->mpd_call);
		dq->mpd_call = NULL;
		break;
	default:
		panic("mpsc_queue[%p]: invalid kind (%d)", dq, dq->mpd_kind);
	}
	dq->mpd_kind = MPSC_QUEUE_KIND_UNKNOWN;
}

#pragma mark deferred deallocation daemon

static struct mpsc_daemon_queue thread_deferred_deallocation_queue;

void
thread_deallocate_daemon_init(void)
{
	kern_return_t kr;

	kr = _mpsc_daemon_queue_init_with_thread(&thread_deferred_deallocation_queue,
	    mpsc_daemon_queue_nested_invoke, MINPRI_KERNEL,
	    "daemon.deferred-deallocation", MPSC_QUEUE_KIND_THREAD_CRITICAL);
	if (kr != KERN_SUCCESS) {
		panic("thread_deallocate_daemon_init: creating daemon failed (%d)", kr);
	}
}

void
thread_deallocate_daemon_register_queue(mpsc_daemon_queue_t dq,
    mpsc_daemon_invoke_fn_t invoke)
{
	mpsc_daemon_queue_init_with_target(dq, invoke,
	    &thread_deferred_deallocation_queue);
}
