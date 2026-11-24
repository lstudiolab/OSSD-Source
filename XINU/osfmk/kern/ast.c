/*
 * Copyright (c) 2000-2017 Apple Inc. All rights reserved.
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
 * @OSF_COPYRIGHT@
 */
/*
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988,1987 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

#include <kern/ast.h>
#include <kern/counter.h>
#include <kern/cpu_quiesce.h>
#include <kern/misc_protos.h>
#include <kern/queue.h>
#include <kern/sched_prim.h>
#include <kern/thread.h>
#include <kern/processor.h>
#include <kern/restartable.h>
#include <kern/spl.h>
#include <kern/sfi.h>
#if CONFIG_TELEMETRY
#include <kern/telemetry.h>
#endif
#include <kern/waitq.h>
#include <kern/ledger.h>
#include <kern/machine.h>
#include <kperf/kperf_kpc.h>
#include <mach/policy.h>
#include <security/mac_mach_internal.h>                     
#include <stdatomic.h>

#if CONFIG_ARCADE
#include <kern/arcade.h>
#endif

static void __attribute__((noinline, noreturn, disable_tail_calls))
thread_preempted(__unused void* parameter, __unused wait_result_t result)
{
	  
                                                            
                                     
    
	thread_exception_return();
}

  
                                               
                                                        
                        
   
void
ast_taken_kernel(void)
{
	assert(ml_get_interrupts_enabled() == FALSE);

	thread_t thread = current_thread();

	                                               
	if ((thread->state & TH_IDLE)) {
		ast_off(AST_PREEMPTION);
		return;
	}

	  
                                                        
                                                               
    
	if (ast_peek(AST_URGENT) != AST_URGENT) {
		return;
	}

	  
                                                              
                                                      
    
	if (waitq_wait_possible(thread) == FALSE) {
		                                                            
		ast_consume(AST_URGENT);
		return;
	}

	                                                                           

	ast_t urgent_reason = ast_consume(AST_PREEMPTION);

	assert(urgent_reason & AST_PREEMPT);

	thread_block_reason(THREAD_CONTINUE_NULL, NULL, urgent_reason);

	assert(ml_get_interrupts_enabled() == FALSE);
}

  
                                                   
                                                                   
                                             
   
void
ast_taken_user(void)
{
	assert(ml_get_interrupts_enabled() == FALSE);

	thread_t thread = current_thread();

	                                                                           
	assert(waitq_wait_possible(thread));
	assert((thread->state & TH_IDLE) == 0);

	                                                          

	  
                                                       
                                                   
                                                                     
                                         
    
	if (ast_peek(AST_URGENT) == AST_URGENT) {
		ast_t urgent_reason = ast_consume(AST_PREEMPTION);

		assert(urgent_reason & AST_PREEMPT);

		                                                                           

		thread_block_reason(thread_preempted, NULL, urgent_reason);
		                
	}

	  
                                                                                         
                                                                         
   
                                                            
    
	ast_propagate(thread);

	  
                                                              
                                     
   
                                                        
                                                         
                                                       
                                                
                                                    
   
                                                             
                                                           
                           
   
                                                               
    
	ast_t reasons = ast_consume(AST_PER_THREAD | AST_KPERF | AST_DTRACE);

	ml_set_interrupts_enabled(TRUE);

#if CONFIG_DTRACE
	if (reasons & AST_DTRACE) {
		dtrace_ast();
	}
#endif

#ifdef MACH_BSD
	if (reasons & AST_BSD) {
		thread_ast_clear(thread, AST_BSD);
		bsd_ast(thread);
	}
#endif

#if CONFIG_MACF
	if (reasons & AST_MACF) {
		thread_ast_clear(thread, AST_MACF);
		mac_thread_userret(thread);
	}
#endif

#if CONFIG_ARCADE
	if (reasons & AST_ARCADE) {
		thread_ast_clear(thread, AST_ARCADE);
		arcade_ast(thread);
	}
#endif

	if (reasons & AST_APC) {
		thread_ast_clear(thread, AST_APC);
		thread_apc_ast(thread);
	}

	if (reasons & AST_GUARD) {
		thread_ast_clear(thread, AST_GUARD);
		guard_ast(thread);
	}

	if (reasons & AST_LEDGER) {
		thread_ast_clear(thread, AST_LEDGER);
		ledger_ast(thread);
	}

	if (reasons & AST_KPERF) {
		thread_ast_clear(thread, AST_KPERF);
		kperf_kpc_thread_ast(thread);
	}

	if (reasons & AST_RESET_PCS) {
		thread_ast_clear(thread, AST_RESET_PCS);
		thread_reset_pcs_ast(thread);
	}

	if (reasons & AST_KEVENT) {
		thread_ast_clear(thread, AST_KEVENT);
		uint16_t bits = atomic_exchange(&thread->kevent_ast_bits, 0);
		if (bits) {
			kevent_ast(thread, bits);
		}
	}

#if CONFIG_TELEMETRY
	if (reasons & AST_TELEMETRY_ALL) {
		ast_t telemetry_reasons = reasons & AST_TELEMETRY_ALL;
		thread_ast_clear(thread, AST_TELEMETRY_ALL);
		telemetry_ast(thread, telemetry_reasons);
	}
#endif

	spl_t s = splsched();

#if CONFIG_SCHED_SFI
	  
                                                              
                                             
    
	if (ast_consume(AST_SFI) == AST_SFI) {
		sfi_ast(thread);
	}
#endif

	                                                                           
	assert(waitq_wait_possible(thread));

	  
                                                                            
   
                                                                     
                                          
   
                                                                
                                                      
    
	ast_t preemption_reasons = ast_consume(AST_PREEMPTION);

	if (preemption_reasons & AST_PREEMPT) {
		                                                                                            

		thread_lock(thread);
		preemption_reasons = csw_check(thread, current_processor(), (preemption_reasons & AST_QUANTUM));
		thread_unlock(thread);

#if CONFIG_SCHED_SFI
		                                                
		if (preemption_reasons & AST_SFI) {
			sfi_ast(thread);
		}
#endif

		if (preemption_reasons & AST_PREEMPT) {
			                                                                  
			thread_block_reason(thread_preempted, NULL, preemption_reasons);
			                
		}
	}

	if (ast_consume(AST_UNQUIESCE) == AST_UNQUIESCE) {
		cpu_quiescent_counter_ast();
	}

	cpu_quiescent_counter_assert_ast();

	splx(s);

	  
                                                                      
                             
    
	assert((thread->sched_flags & TH_SFLAG_WAITQ_PROMOTED) == 0);
	assert((thread->sched_flags & TH_SFLAG_RW_PROMOTED) == 0);
	assert((thread->sched_flags & TH_SFLAG_EXEC_PROMOTED) == 0);
	assert((thread->sched_flags & TH_SFLAG_PROMOTED) == 0);
	assert((thread->sched_flags & TH_SFLAG_DEPRESS) == 0);

	assert(thread->kern_promotion_schedpri == 0);
	assert(thread->waiting_for_mutex == NULL);
	assert(thread->rwlock_count == 0);
}

  
                                     
                     
   
void
ast_on(ast_t reasons)
{
	ast_t *pending_ast = ast_pending();

	*pending_ast |= reasons;
}

  
                                       
                     
   
void
ast_off(ast_t reasons)
{
	ast_t *pending_ast = ast_pending();

	*pending_ast &= ~reasons;
}

  
                                                                     
                                
                     
   
ast_t
ast_consume(ast_t reasons)
{
	ast_t *pending_ast = ast_pending();

	reasons &= *pending_ast;
	*pending_ast &= ~reasons;

	return reasons;
}

  
                                                                  
                                                            
                     
   
ast_t
ast_peek(ast_t reasons)
{
	ast_t *pending_ast = ast_pending();

	reasons &= *pending_ast;

	return reasons;
}

  
                                                                         
                     
   
void
ast_context(thread_t thread)
{
	ast_t *pending_ast = ast_pending();

	*pending_ast = ((*pending_ast & ~AST_PER_THREAD) | thread->ast);
}

  
                                                          
                     
   
void
ast_propagate(thread_t thread)
{
	ast_on(thread->ast);
}

void
ast_dtrace_on(void)
{
	ast_on(AST_DTRACE);
}
