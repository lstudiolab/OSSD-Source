/*
 * Copyright (c) 2000-2016 Apple Inc. All rights reserved.
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
/*
 * @OSF_FREE_COPYRIGHT@
 */
/*
 * Copyright (c) 1993 The University of Utah and
 * the Center for Software Science (CSS).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 *	Author:	Bryan Ford, University of Utah CSS
 *
 *	Thread management routines
 */

#include <mach/mach_types.h>
#include <mach/kern_return.h>
#include <mach/thread_act_server.h>

#include <kern/kern_types.h>
#include <kern/ast.h>
#include <kern/mach_param.h>
#include <kern/zalloc.h>
#include <kern/extmod_statistics.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <kern/sched_prim.h>
#include <kern/misc_protos.h>
#include <kern/assert.h>
#include <kern/exception.h>
#include <kern/ipc_mig.h>
#include <kern/ipc_tt.h>
#include <kern/machine.h>
#include <kern/spl.h>
#include <kern/syscall_subr.h>
#include <kern/sync_lock.h>
#include <kern/processor.h>
#include <kern/timer.h>
#include <kern/affinity.h>
#include <kern/host.h>

#include <stdatomic.h>

#include <security/mac_mach_internal.h>

static void act_abort(thread_t thread);

static void thread_suspended(void *arg, wait_result_t result);
static void thread_set_apc_ast(thread_t thread);
static void thread_set_apc_ast_locked(thread_t thread);

  
                                                
                                              
   
void
thread_start(
	thread_t                        thread)
{
	clear_wait(thread, THREAD_AWAKENED);
	thread->started = TRUE;
}

  
                                               
                                               
                                                  
                                         
  
                                              
  
                                          
                                               
              
  
                                                 
                                                
   
void
thread_start_in_assert_wait(
	thread_t                        thread,
	event_t             event,
	wait_interrupt_t    interruptible)
{
	struct waitq *waitq = assert_wait_queue(event);
	wait_result_t wait_result;
	spl_t spl;

	spl = splsched();
	waitq_lock(waitq);

	                                                                       
	thread_lock(thread);
	assert(!thread->started);
	assert((thread->state & (TH_WAIT | TH_UNINT)) == (TH_WAIT | TH_UNINT));
	thread->state &= ~(TH_WAIT | TH_UNINT);
	thread_unlock(thread);

	                                       
	wait_result = waitq_assert_wait64_locked(waitq, CAST_EVENT64_T(event),
	    interruptible,
	    TIMEOUT_URGENCY_SYS_NORMAL,
	    TIMEOUT_WAIT_FOREVER,
	    TIMEOUT_NO_LEEWAY,
	    thread);
	assert(wait_result == THREAD_WAITING);

	                                                            
	thread_lock(thread);
	thread->started = TRUE;
	thread_unlock(thread);

	waitq_unlock(waitq);
	splx(spl);
}

  
                                          
                                             
   
kern_return_t
thread_terminate_internal(
	thread_t                        thread,
	thread_terminate_options_t      options)
{
	kern_return_t           result = KERN_SUCCESS;
	boolean_t               test_pin_bit = false;

	thread_mtx_lock(thread);

	if (thread->active) {
		thread->active = FALSE;

		act_abort(thread);

		if (thread->started) {
			clear_wait(thread, THREAD_INTERRUPTED);
		} else {
			thread_start(thread);
		}
		                                                                        
		test_pin_bit = (options == TH_TERMINATE_OPTION_UNPIN) ? true : false;
	} else {
		result = KERN_TERMINATED;
	}

	if (thread->affinity_set != NULL) {
		thread_affinity_terminate(thread);
	}

	  
                                                                              
                                                                                   
                                        
    
	ipc_thread_port_unpin(thread->ith_self, test_pin_bit);

	thread_mtx_unlock(thread);

	if (thread != current_thread() && result == KERN_SUCCESS) {
		thread_wait(thread, FALSE);
	}

	return result;
}

  
                      
   
kern_return_t
thread_terminate(
	thread_t                thread)
{
	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	                                                                      
	if (thread->task == kernel_task && thread != current_thread()) {
		return KERN_FAILURE;
	}

	kern_return_t result = thread_terminate_internal(thread, TH_TERMINATE_OPTION_NONE);

	  
                                                                            
                                                                           
                                                                        
    
	if (thread->task == kernel_task) {
		assert(thread->active == FALSE);
		thread_ast_clear(thread, AST_APC);
		thread_apc_ast(thread);

		panic("thread_terminate");
		                
	}

	return result;
}

kern_return_t
thread_terminate_pinned(
	thread_t                thread)
{
	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	assert(thread->task != kernel_task);

	kern_return_t result = thread_terminate_internal(thread, TH_TERMINATE_OPTION_UNPIN);
	return result;
}

  
                                             
                                                                 
                          
  
                                 
   
void
thread_hold(thread_t thread)
{
	if (thread->suspend_count++ == 0) {
		thread_set_apc_ast(thread);
		assert(thread->suspend_parked == FALSE);
	}
}

  
                                                      
                                     
  
                                                                        
                                                               
  
                                 
   
void
thread_release(thread_t thread)
{
	assertf(thread->suspend_count > 0, "thread %p over-resumed", thread);

	                                    
	if (thread->suspend_count == 0) {
		return;
	}

	if (--thread->suspend_count == 0) {
		if (!thread->started) {
			thread_start(thread);
		} else if (thread->suspend_parked) {
			thread->suspend_parked = FALSE;
			thread_wakeup_thread(&thread->suspend_count, thread);
		}
	}
}

kern_return_t
thread_suspend(thread_t thread)
{
	kern_return_t result = KERN_SUCCESS;

	if (thread == THREAD_NULL || thread->task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(thread);

	if (thread->active) {
		if (thread->user_stop_count++ == 0) {
			thread_hold(thread);
		}
	} else {
		result = KERN_TERMINATED;
	}

	thread_mtx_unlock(thread);

	if (thread != current_thread() && result == KERN_SUCCESS) {
		thread_wait(thread, FALSE);
	}

	return result;
}

kern_return_t
thread_resume(thread_t thread)
{
	kern_return_t result = KERN_SUCCESS;

	if (thread == THREAD_NULL || thread->task == kernel_task) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(thread);

	if (thread->active) {
		if (thread->user_stop_count > 0) {
			if (--thread->user_stop_count == 0) {
				thread_release(thread);
			}
		} else {
			result = KERN_FAILURE;
		}
	} else {
		result = KERN_TERMINATED;
	}

	thread_mtx_unlock(thread);

	return result;
}

  
                                  
  
                                                         
   
kern_return_t
thread_depress_abort_from_user(thread_t thread)
{
	kern_return_t result;

	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(thread);

	if (thread->active) {
		result = thread_depress_abort(thread);
	} else {
		result = KERN_TERMINATED;
	}

	thread_mtx_unlock(thread);

	return result;
}


  
                                                           
                                
  
                                 
   
static void
act_abort(
	thread_t        thread)
{
	spl_t           s = splsched();

	thread_lock(thread);

	if (!(thread->sched_flags & TH_SFLAG_ABORT)) {
		thread->sched_flags |= TH_SFLAG_ABORT;
		thread_set_apc_ast_locked(thread);
		thread_depress_abort_locked(thread);
	} else {
		thread->sched_flags &= ~TH_SFLAG_ABORTSAFELY;
	}

	thread_unlock(thread);
	splx(s);
}

kern_return_t
thread_abort(
	thread_t        thread)
{
	kern_return_t   result = KERN_SUCCESS;

	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(thread);

	if (thread->active) {
		act_abort(thread);
		clear_wait(thread, THREAD_INTERRUPTED);
	} else {
		result = KERN_TERMINATED;
	}

	thread_mtx_unlock(thread);

	return result;
}

kern_return_t
thread_abort_safely(
	thread_t                thread)
{
	kern_return_t   result = KERN_SUCCESS;

	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(thread);

	if (thread->active) {
		spl_t           s = splsched();

		thread_lock(thread);
		if (!thread->at_safe_point ||
		    clear_wait_internal(thread, THREAD_INTERRUPTED) != KERN_SUCCESS) {
			if (!(thread->sched_flags & TH_SFLAG_ABORT)) {
				thread->sched_flags |= TH_SFLAG_ABORTED_MASK;
				thread_set_apc_ast_locked(thread);
				thread_depress_abort_locked(thread);
			}
		}
		thread_unlock(thread);
		splx(s);
	} else {
		result = KERN_TERMINATED;
	}

	thread_mtx_unlock(thread);

	return result;
}

                                      
#include <mach/thread_info.h>
#include <mach/thread_special_ports.h>
#include <ipc/ipc_port.h>

kern_return_t
thread_info(
	thread_t                        thread,
	thread_flavor_t                 flavor,
	thread_info_t                   thread_info_out,
	mach_msg_type_number_t  *thread_info_count)
{
	kern_return_t                   result;

	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(thread);

	if (thread->active || thread->inspection) {
		result = thread_info_internal(
			thread, flavor, thread_info_out, thread_info_count);
	} else {
		result = KERN_TERMINATED;
	}

	thread_mtx_unlock(thread);

	return result;
}

static inline kern_return_t
thread_get_state_internal(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  state,                                            
	mach_msg_type_number_t  *state_count,             
	boolean_t                               to_user)
{
	kern_return_t           result = KERN_SUCCESS;

	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(thread);

	if (thread->active) {
		if (thread != current_thread()) {
			thread_hold(thread);

			thread_mtx_unlock(thread);

			if (thread_stop(thread, FALSE)) {
				thread_mtx_lock(thread);
				result = machine_thread_get_state(
					thread, flavor, state, state_count);
				thread_unstop(thread);
			} else {
				thread_mtx_lock(thread);
				result = KERN_ABORTED;
			}

			thread_release(thread);
		} else {
			result = machine_thread_get_state(
				thread, flavor, state, state_count);
		}
	} else if (thread->inspection) {
		result = machine_thread_get_state(
			thread, flavor, state, state_count);
	} else {
		result = KERN_TERMINATED;
	}

	if (to_user && result == KERN_SUCCESS) {
		result = machine_thread_state_convert_to_user(thread, flavor, state,
		    state_count);
	}

	thread_mtx_unlock(thread);

	return result;
}

                                                                                       

kern_return_t
thread_get_state(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  *state_count);

kern_return_t
thread_get_state(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  state,                                            
	mach_msg_type_number_t  *state_count)             
{
	return thread_get_state_internal(thread, flavor, state, state_count, FALSE);
}

kern_return_t
thread_get_state_to_user(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  state,                                            
	mach_msg_type_number_t  *state_count)             
{
	return thread_get_state_internal(thread, flavor, state, state_count, TRUE);
}

  
                                                                
                             
   
static inline kern_return_t
thread_set_state_internal(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  state_count,
	boolean_t                               from_user)
{
	kern_return_t           result = KERN_SUCCESS;

	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(thread);

	if (thread->active) {
		if (from_user) {
			result = machine_thread_state_convert_from_user(thread, flavor,
			    state, state_count);
			if (result != KERN_SUCCESS) {
				goto out;
			}
		}
		if (thread != current_thread()) {
			thread_hold(thread);

			thread_mtx_unlock(thread);

			if (thread_stop(thread, TRUE)) {
				thread_mtx_lock(thread);
				result = machine_thread_set_state(
					thread, flavor, state, state_count);
				thread_unstop(thread);
			} else {
				thread_mtx_lock(thread);
				result = KERN_ABORTED;
			}

			thread_release(thread);
		} else {
			result = machine_thread_set_state(
				thread, flavor, state, state_count);
		}
	} else {
		result = KERN_TERMINATED;
	}

	if ((result == KERN_SUCCESS) && from_user) {
		extmod_statistics_incr_thread_set_state(thread);
	}

out:
	thread_mtx_unlock(thread);

	return result;
}

                                                                                         
kern_return_t
thread_set_state(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  state_count);

kern_return_t
thread_set_state(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  state_count)
{
	return thread_set_state_internal(thread, flavor, state, state_count, FALSE);
}

kern_return_t
thread_set_state_from_user(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  state_count)
{
	return thread_set_state_internal(thread, flavor, state, state_count, TRUE);
}

kern_return_t
thread_convert_thread_state(
	thread_t                thread,
	int                     direction,
	thread_state_flavor_t   flavor,
	thread_state_t          in_state,                                   
	mach_msg_type_number_t  in_state_count,
	thread_state_t          out_state,                                   
	mach_msg_type_number_t  *out_state_count)             
{
	kern_return_t kr;
	thread_t to_thread = THREAD_NULL;
	thread_t from_thread = THREAD_NULL;
	mach_msg_type_number_t state_count = in_state_count;

	if (direction != THREAD_CONVERT_THREAD_STATE_TO_SELF &&
	    direction != THREAD_CONVERT_THREAD_STATE_FROM_SELF) {
		return KERN_INVALID_ARGUMENT;
	}

	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (state_count > *out_state_count) {
		return KERN_INSUFFICIENT_BUFFER_SIZE;
	}

	if (direction == THREAD_CONVERT_THREAD_STATE_FROM_SELF) {
		to_thread = thread;
		from_thread = current_thread();
	} else {
		to_thread = current_thread();
		from_thread = thread;
	}

	                                                                    
	kr = machine_thread_state_convert_from_user(from_thread, flavor,
	    in_state, state_count);

	                                                                           
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	                                                               
	kr = machine_thread_state_convert_to_user(to_thread, flavor,
	    in_state, &state_count);

	if (kr == KERN_SUCCESS) {
		if (state_count <= *out_state_count) {
			memcpy(out_state, in_state, state_count * sizeof(uint32_t));
			*out_state_count = state_count;
		} else {
			kr = KERN_INSUFFICIENT_BUFFER_SIZE;
		}
	}

	return kr;
}

  
                                                              
   

                                                                    
                                         
   
kern_return_t
thread_state_initialize(
	thread_t                thread)
{
	kern_return_t           result = KERN_SUCCESS;

	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(thread);

	if (thread->active) {
		if (thread != current_thread()) {
			thread_hold(thread);

			thread_mtx_unlock(thread);

			if (thread_stop(thread, TRUE)) {
				thread_mtx_lock(thread);
				result = machine_thread_state_initialize( thread );
				thread_unstop(thread);
			} else {
				thread_mtx_lock(thread);
				result = KERN_ABORTED;
			}

			thread_release(thread);
		} else {
			result = machine_thread_state_initialize( thread );
		}
	} else {
		result = KERN_TERMINATED;
	}

	thread_mtx_unlock(thread);

	return result;
}

kern_return_t
thread_dup(
	thread_t        target)
{
	thread_t                        self = current_thread();
	kern_return_t           result = KERN_SUCCESS;

	if (target == THREAD_NULL || target == self) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(target);

	if (target->active) {
		thread_hold(target);

		thread_mtx_unlock(target);

		if (thread_stop(target, TRUE)) {
			thread_mtx_lock(target);
			result = machine_thread_dup(self, target, FALSE);

			if (self->affinity_set != AFFINITY_SET_NULL) {
				thread_affinity_dup(self, target);
			}
			thread_unstop(target);
		} else {
			thread_mtx_lock(target);
			result = KERN_ABORTED;
		}

		thread_release(target);
	} else {
		result = KERN_TERMINATED;
	}

	thread_mtx_unlock(target);

	return result;
}


kern_return_t
thread_dup2(
	thread_t        source,
	thread_t        target)
{
	kern_return_t           result = KERN_SUCCESS;
	uint32_t                active = 0;

	if (source == THREAD_NULL || target == THREAD_NULL || target == source) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(source);
	active = source->active;
	thread_mtx_unlock(source);

	if (!active) {
		return KERN_TERMINATED;
	}

	thread_mtx_lock(target);

	if (target->active || target->inspection) {
		thread_hold(target);

		thread_mtx_unlock(target);

		if (thread_stop(target, TRUE)) {
			thread_mtx_lock(target);
			result = machine_thread_dup(source, target, TRUE);
			if (source->affinity_set != AFFINITY_SET_NULL) {
				thread_affinity_dup(source, target);
			}
			thread_unstop(target);
		} else {
			thread_mtx_lock(target);
			result = KERN_ABORTED;
		}

		thread_release(target);
	} else {
		result = KERN_TERMINATED;
	}

	thread_mtx_unlock(target);

	return result;
}

  
                    
  
                                          
                                                
   
kern_return_t
thread_setstatus(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  tstate,
	mach_msg_type_number_t  count)
{
	return thread_set_state(thread, flavor, tstate, count);
}

kern_return_t
thread_setstatus_from_user(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  tstate,
	mach_msg_type_number_t  count)
{
	return thread_set_state_from_user(thread, flavor, tstate, count);
}

  
                    
  
                                          
   
kern_return_t
thread_getstatus(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  tstate,
	mach_msg_type_number_t  *count)
{
	return thread_get_state(thread, flavor, tstate, count);
}

kern_return_t
thread_getstatus_to_user(
	thread_t                thread,
	int                                             flavor,
	thread_state_t                  tstate,
	mach_msg_type_number_t  *count)
{
	return thread_get_state_to_user(thread, flavor, tstate, count);
}

  
                                                        
                                                  
   
kern_return_t
thread_set_tsd_base(
	thread_t                        thread,
	mach_vm_offset_t        tsd_base)
{
	kern_return_t           result = KERN_SUCCESS;

	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(thread);

	if (thread->active) {
		if (thread != current_thread()) {
			thread_hold(thread);

			thread_mtx_unlock(thread);

			if (thread_stop(thread, TRUE)) {
				thread_mtx_lock(thread);
				result = machine_thread_set_tsd_base(thread, tsd_base);
				thread_unstop(thread);
			} else {
				thread_mtx_lock(thread);
				result = KERN_ABORTED;
			}

			thread_release(thread);
		} else {
			result = machine_thread_set_tsd_base(thread, tsd_base);
		}
	} else {
		result = KERN_TERMINATED;
	}

	thread_mtx_unlock(thread);

	return result;
}

  
                      
  
                                                            
                                                    
  
                                     
   
static void
thread_set_apc_ast(thread_t thread)
{
	spl_t s = splsched();

	thread_lock(thread);
	thread_set_apc_ast_locked(thread);
	thread_unlock(thread);

	splx(s);
}

  
                             
  
                                                       
  
                                                         
   
static void
thread_set_apc_ast_locked(thread_t thread)
{
	thread_ast_set(thread, AST_APC);

	if (thread == current_thread()) {
		ast_propagate(thread);
	} else {
		processor_t processor = thread->last_processor;

		if (processor != PROCESSOR_NULL &&
		    processor->state == PROCESSOR_RUNNING &&
		    processor->active_thread == thread) {
			cause_ast_check(processor);
		}
	}
}

  
                                                             
  
   

  
                   
  
                                                         
                                                                
                                      
   
__attribute__((noreturn))
static void
thread_suspended(__unused void *parameter, wait_result_t result)
{
	thread_t thread = current_thread();

	thread_mtx_lock(thread);

	if (result == THREAD_INTERRUPTED) {
		thread->suspend_parked = FALSE;
	} else {
		assert(thread->suspend_parked == FALSE);
	}

	if (thread->suspend_count > 0) {
		thread_set_apc_ast(thread);
	}

	thread_mtx_unlock(thread);

	thread_exception_return();
	              
}

  
                                                                                 
                                                                     
   
void
thread_apc_ast(thread_t thread)
{
	thread_mtx_lock(thread);

	assert(thread->suspend_parked == FALSE);

	spl_t s = splsched();
	thread_lock(thread);

	                                             
	assert((thread->sched_flags & TH_SFLAG_DEPRESS) == 0);

	thread->sched_flags &= ~TH_SFLAG_ABORTED_MASK;
	thread_unlock(thread);
	splx(s);

	if (!thread->active) {
		                                                        
		thread_mtx_unlock(thread);

		thread_terminate_self();
		              
	}

	                                                                         
	if (thread->suspend_count > 0) {
		thread->suspend_parked = TRUE;
		assert_wait(&thread->suspend_count,
		    THREAD_ABORTSAFE | THREAD_WAIT_NOREPORT_USER);
		thread_mtx_unlock(thread);

		thread_block(thread_suspended);
		              
	}

	thread_mtx_unlock(thread);
}


                                        
kern_return_t
act_set_state(
	thread_t                                thread,
	int                                             flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  count);

kern_return_t
act_set_state(
	thread_t                                thread,
	int                                             flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  count)
{
	if (thread == current_thread()) {
		return KERN_INVALID_ARGUMENT;
	}

	return thread_set_state(thread, flavor, state, count);
}

kern_return_t
act_set_state_from_user(
	thread_t                                thread,
	int                                             flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  count)
{
	if (thread == current_thread()) {
		return KERN_INVALID_ARGUMENT;
	}

	return thread_set_state_from_user(thread, flavor, state, count);
}

                                        
kern_return_t
act_get_state(
	thread_t                                thread,
	int                                             flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  *count);

kern_return_t
act_get_state(
	thread_t                                thread,
	int                                             flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  *count)
{
	if (thread == current_thread()) {
		return KERN_INVALID_ARGUMENT;
	}

	return thread_get_state(thread, flavor, state, count);
}

kern_return_t
act_get_state_to_user(
	thread_t                                thread,
	int                                             flavor,
	thread_state_t                  state,
	mach_msg_type_number_t  *count)
{
	if (thread == current_thread()) {
		return KERN_INVALID_ARGUMENT;
	}

	return thread_get_state_to_user(thread, flavor, state, count);
}

static void
act_set_ast(
	thread_t thread,
	ast_t ast)
{
	spl_t s = splsched();

	if (thread == current_thread()) {
		thread_ast_set(thread, ast);
		ast_propagate(thread);
	} else {
		processor_t processor;

		thread_lock(thread);
		thread_ast_set(thread, ast);
		processor = thread->last_processor;
		if (processor != PROCESSOR_NULL &&
		    processor->state == PROCESSOR_RUNNING &&
		    processor->active_thread == thread) {
			cause_ast_check(processor);
		}
		thread_unlock(thread);
	}

	splx(s);
}

  
                                                 
                                     
  
                                                        
                                                         
                                           
   
static void
act_set_ast_async(thread_t  thread,
    ast_t     ast)
{
	thread_ast_set(thread, ast);

	if (thread == current_thread()) {
		spl_t s = splsched();
		ast_propagate(thread);
		splx(s);
	}
}

void
act_set_astbsd(
	thread_t        thread)
{
	act_set_ast( thread, AST_BSD );
}

void
act_set_astkevent(thread_t thread, uint16_t bits)
{
	os_atomic_or(&thread->kevent_ast_bits, bits, relaxed);

	                                              
	act_set_ast_async(thread, AST_KEVENT);
}

uint16_t
act_clear_astkevent(thread_t thread, uint16_t bits)
{
	  
                                                          
                                  
    
	uint16_t cur = os_atomic_load(&thread->kevent_ast_bits, relaxed);
	if (cur & bits) {
		cur = os_atomic_andnot_orig(&thread->kevent_ast_bits, bits, relaxed);
	}
	return cur & bits;
}

void
act_set_ast_reset_pcs(thread_t thread)
{
	act_set_ast(thread, AST_RESET_PCS);
}

void
act_set_kperf(
	thread_t        thread)
{
	                  
	if (thread != current_thread()) {
		if (!ml_get_interrupts_enabled()) {
			panic("unsafe act_set_kperf operation");
		}
	}

	act_set_ast( thread, AST_KPERF );
}

#if CONFIG_MACF
void
act_set_astmacf(
	thread_t        thread)
{
	act_set_ast( thread, AST_MACF);
}
#endif

void
act_set_astledger(thread_t thread)
{
	act_set_ast(thread, AST_LEDGER);
}

  
                                                          
                                                        
                                      
  
                                                            
                               
   
void
act_set_astledger_async(thread_t thread)
{
	act_set_ast_async(thread, AST_LEDGER);
}

void
act_set_io_telemetry_ast(thread_t thread)
{
	act_set_ast(thread, AST_TELEMETRY_IO);
}
