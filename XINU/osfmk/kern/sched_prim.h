/*
 * Copyright (c) 2000-2012 Apple Inc. All rights reserved.
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
  
   
  
                     
                      
  
                                        
  
   

#ifndef _KERN_SCHED_PRIM_H_
#define _KERN_SCHED_PRIM_H_

#include <sys/cdefs.h>
#include <mach/boolean.h>
#include <mach/machine/vm_types.h>
#include <mach/kern_return.h>
#include <kern/clock.h>
#include <kern/kern_types.h>
#include <kern/percpu.h>
#include <kern/thread.h>
#include <kern/block_hint.h>

extern int              thread_get_current_cpuid(void);

#ifdef  MACH_KERNEL_PRIVATE

#include <kern/sched_urgency.h>
#include <kern/thread_group.h>
#include <kern/waitq.h>

                    
extern void             sched_init(void);

extern void             sched_startup(void);

extern void             sched_timebase_init(void);

extern void             pset_rt_init(processor_set_t pset);

extern void             sched_rtlocal_init(processor_set_t pset);

extern rt_queue_t       sched_rtlocal_runq(processor_set_t pset);

extern void             sched_rtlocal_queue_shutdown(processor_t processor);

extern int64_t          sched_rtlocal_runq_count_sum(void);

extern void             sched_check_spill(processor_set_t pset, thread_t thread);

extern bool             sched_thread_should_yield(processor_t processor, thread_t thread);

extern bool             sched_steal_thread_DISABLED(processor_set_t pset);
extern bool             sched_steal_thread_enabled(processor_set_t pset);

                                                                           
extern boolean_t        thread_stop(
	thread_t        thread,
	boolean_t       until_not_runnable);

                                     
extern void                     thread_unstop(
	thread_t        thread);

                                       
extern void                     thread_wait(
	thread_t        thread,
	boolean_t       until_not_runnable);

                               
extern boolean_t        thread_unblock(
	thread_t                thread,
	wait_result_t   wresult);

                                 
extern kern_return_t    thread_go(
	thread_t                thread,
	wait_result_t   wresult,
	waitq_options_t option);

                                        
extern boolean_t
thread_allowed_for_handoff(
	thread_t         thread);

                                      
extern void                     thread_dispatch(
	thread_t                old_thread,
	thread_t                new_thread);

                                            
extern int                      thread_run(
	thread_t                        self,
	thread_continue_t       continuation,
	void                            *parameter,
	thread_t                        new_thread);

                                  
extern __dead2 void     thread_continue(thread_t old_thread);

                         
extern __dead2 void     call_continuation(
	thread_continue_t       continuation,
	void                    *parameter,
	wait_result_t           wresult,
	boolean_t               enable_interrupts);

  
                                            
                       
   
__options_decl(set_sched_pri_options_t, uint32_t, {
	SETPRI_DEFAULT  = 0x0,
	SETPRI_LAZY     = 0x1,                                               
});

                                        
extern void set_sched_pri(
	thread_t      thread,
	int16_t       priority,
	set_sched_pri_options_t options);

                                               
extern void             sched_set_thread_base_priority(
	thread_t                thread,
	int                             priority);

                                                        
extern void             sched_set_kernel_thread_priority(
	thread_t                thread,
	int                             priority);


                                           
extern void             sched_set_thread_mode(thread_t thread,
    sched_mode_t mode);
                                    
extern void             sched_thread_mode_demote(thread_t thread,
    uint32_t reason);
                                       
extern void             sched_thread_mode_undemote(thread_t thread,
    uint32_t reason);

extern void sched_thread_promote_reason(thread_t thread, uint32_t reason, uintptr_t trace_obj);
extern void sched_thread_unpromote_reason(thread_t thread, uint32_t reason, uintptr_t trace_obj);

                                                         
void thread_recompute_priority(thread_t thread);

                                                              
extern void thread_recompute_sched_pri(
	thread_t thread,
	set_sched_pri_options_t options);

                                 
extern void             sched_init_thread(void);

                                                
extern boolean_t                can_update_priority(
	thread_t                thread);

extern void             update_priority(
	thread_t                thread);

extern void             lightweight_update_priority(
	thread_t                thread);

extern void             sched_default_quantum_expire(thread_t thread);

                                        
extern void             idle_thread(
	void*           parameter,
	wait_result_t   result);

extern kern_return_t    idle_thread_create(
	processor_t             processor);

                                      
extern void     thread_syscall_return(
	kern_return_t   ret);

                    
extern wait_result_t    thread_block_reason(
	thread_continue_t       continuation,
	void                            *parameter,
	ast_t                           reason);

__options_decl(sched_options_t, uint32_t, {
	SCHED_NONE      = 0x0,
	SCHED_TAILQ     = 0x1,
	SCHED_HEADQ     = 0x2,
	SCHED_PREEMPT   = 0x4,
	SCHED_REBALANCE = 0x8,
});

                                     
extern void             thread_setrun(
	thread_t        thread,
	sched_options_t options);

extern processor_set_t  task_choose_pset(
	task_t                  task);

                                                       
extern processor_t              thread_bind(
	processor_t             processor);

extern bool pset_has_stealable_threads(
	processor_set_t         pset);

extern processor_set_t choose_starting_pset(
	pset_node_t  node,
	thread_t     thread,
	processor_t *processor_hint);

extern pset_node_t sched_choose_node(
	thread_t     thread);

                                               
extern processor_t      choose_processor(
	processor_set_t                pset,
	processor_t                    processor,
	thread_t                       thread);

extern void sched_SMT_balance(
	processor_t processor,
	processor_set_t pset);

extern void thread_quantum_init(
	thread_t thread);

extern void             run_queue_init(
	run_queue_t             runq);

extern thread_t run_queue_dequeue(
	run_queue_t           runq,
	sched_options_t       options);

extern boolean_t        run_queue_enqueue(
	run_queue_t           runq,
	thread_t              thread,
	sched_options_t       options);

extern void     run_queue_remove(
	run_queue_t            runq,
	thread_t                       thread);

extern thread_t run_queue_peek(
	run_queue_t            runq);

struct sched_update_scan_context {
	uint64_t        earliest_bg_make_runnable_time;
	uint64_t        earliest_normal_make_runnable_time;
	uint64_t        earliest_rt_make_runnable_time;
	uint64_t        sched_tick_last_abstime;
};
typedef struct sched_update_scan_context *sched_update_scan_context_t;

extern void             sched_rtlocal_runq_scan(sched_update_scan_context_t scan_context);

extern void sched_pset_made_schedulable(
	processor_t processor,
	processor_set_t pset,
	boolean_t drop_lock);

  
                                                                
                                                              
                                                                      
   
typedef enum {
	SCHED_IPI_EVENT_BOUND_THR   = 0x1,
	SCHED_IPI_EVENT_PREEMPT     = 0x2,
	SCHED_IPI_EVENT_SMT_REBAL   = 0x3,
	SCHED_IPI_EVENT_SPILL       = 0x4,
	SCHED_IPI_EVENT_REBALANCE   = 0x5,
} sched_ipi_event_t;


                                                            
typedef enum {
	SCHED_IPI_NONE              = 0x0,
	SCHED_IPI_IMMEDIATE         = 0x1,
	SCHED_IPI_IDLE              = 0x2,
	SCHED_IPI_DEFERRED          = 0x3,
} sched_ipi_type_t;

                                                         
                                                                          
                                                             
                                                                             
                                                                      
                                                                             
                                                                              
   
extern sched_ipi_type_t sched_ipi_action(processor_t dst, thread_t thread,
    boolean_t dst_idle, sched_ipi_event_t event);
extern void sched_ipi_perform(processor_t dst, sched_ipi_type_t ipi);

                                                                            
extern sched_ipi_type_t sched_ipi_policy(processor_t dst, thread_t thread,
    boolean_t dst_idle, sched_ipi_event_t event);

                                                                                              
extern sched_ipi_type_t sched_ipi_deferred_policy(processor_set_t pset,
    processor_t dst, sched_ipi_event_t event);

#if defined(CONFIG_SCHED_TIMESHARE_CORE)

extern boolean_t        thread_update_add_thread(thread_t thread);
extern void             thread_update_process_threads(void);
extern boolean_t        runq_scan(run_queue_t runq, sched_update_scan_context_t scan_context);

#if CONFIG_SCHED_CLUTCH
extern boolean_t        sched_clutch_timeshare_scan(queue_t thread_queue, uint16_t count, sched_update_scan_context_t scan_context);
#endif                          

extern void sched_timeshare_init(void);
extern void sched_timeshare_timebase_init(void);
extern void sched_timeshare_maintenance_continue(void);

extern boolean_t priority_is_urgent(int priority);
extern uint32_t sched_timeshare_initial_quantum_size(thread_t thread);

extern int sched_compute_timeshare_priority(thread_t thread);

#endif                                  

                                      
extern boolean_t        thread_run_queue_remove(thread_t thread);
thread_t thread_run_queue_remove_for_handoff(thread_t thread);

                                                           
extern void thread_run_queue_reinsert(thread_t thread, sched_options_t options);

extern void             thread_timer_expire(
	void                    *thread,
	void                    *p1);

extern bool thread_is_eager_preempt(thread_t thread);

extern boolean_t sched_generic_direct_dispatch_to_idle_processors;

                                                    
__private_extern__ wait_interrupt_t thread_interrupt_level(
	wait_interrupt_t interruptible);

__private_extern__ wait_result_t thread_mark_wait_locked(
	thread_t                 thread,
	wait_interrupt_t interruptible);

                                                    
__private_extern__ kern_return_t clear_wait_internal(
	thread_t                thread,
	wait_result_t   result);

struct sched_statistics {
	uint32_t        csw_count;
	uint32_t        preempt_count;
	uint32_t        preempted_rt_count;
	uint32_t        preempted_by_rt_count;
	uint32_t        rt_sched_count;
	uint32_t        interrupt_count;
	uint32_t        ipi_count;
	uint32_t        timer_pop_count;
	uint32_t        idle_transitions;
	uint32_t        quantum_timer_expirations;
};
PERCPU_DECL(struct sched_statistics, sched_stats);
extern bool             sched_stats_active;

extern void sched_stats_handle_csw(
	processor_t processor,
	int reasons,
	int selfpri,
	int otherpri);

extern void sched_stats_handle_runq_change(
	struct runq_stats *stats,
	int old_count);

#define SCHED_STATS_INC(field)                                                  \
MACRO_BEGIN                                                                     \
	if (__improbable(sched_stats_active)) {                                 \
	        PERCPU_GET(sched_stats)->field++;                               \
	}                                                                       \
MACRO_END

#if DEBUG

#define SCHED_STATS_CSW(processor, reasons, selfpri, otherpri)                  \
MACRO_BEGIN                                                                     \
	if (__improbable(sched_stats_active)) {                                 \
	        sched_stats_handle_csw((processor),                             \
	            (reasons), (selfpri), (otherpri));                          \
	}                                                                       \
MACRO_END


#define SCHED_STATS_RUNQ_CHANGE(stats, old_count)                               \
MACRO_BEGIN                                                                     \
	if (__improbable(sched_stats_active)) {                                 \
	        sched_stats_handle_runq_change((stats), (old_count));           \
	}                                                                       \
MACRO_END

#else            

#define SCHED_STATS_CSW(processor, reasons, selfpri, otherpri) do { }while(0)
#define SCHED_STATS_RUNQ_CHANGE(stats, old_count) do { }while(0)

#endif            

extern uint32_t sched_debug_flags;
#define SCHED_DEBUG_FLAG_PLATFORM_TRACEPOINTS           0x00000001
#define SCHED_DEBUG_FLAG_CHOOSE_PROCESSOR_TRACEPOINTS   0x00000002

#define SCHED_DEBUG_PLATFORM_KERNEL_DEBUG_CONSTANT(...)                         \
MACRO_BEGIN                                                                     \
	if (__improbable(sched_debug_flags &                                    \
	    SCHED_DEBUG_FLAG_PLATFORM_TRACEPOINTS)) {                           \
	        KERNEL_DEBUG_CONSTANT(__VA_ARGS__);                             \
	}                                                                       \
MACRO_END

#define SCHED_DEBUG_CHOOSE_PROCESSOR_KERNEL_DEBUG_CONSTANT(...)                 \
MACRO_BEGIN                                                                     \
	if (__improbable(sched_debug_flags &                                    \
	    SCHED_DEBUG_FLAG_CHOOSE_PROCESSOR_TRACEPOINTS)) {                   \
	        KERNEL_DEBUG_CONSTANT(__VA_ARGS__);                             \
	}                                                                       \
MACRO_END

                                                                               
extern void     active_rt_threads(
	boolean_t       active);

                                                      
extern perfcontrol_class_t thread_get_perfcontrol_class(
	thread_t        thread);

                                                                     
extern uint32_t sched_qos_max_parallelism(int qos, uint64_t options);

#endif                          

__BEGIN_DECLS

#ifdef  XNU_KERNEL_PRIVATE

extern void thread_bind_cluster_type(thread_t, char cluster_type, bool soft_bind);

extern int sched_get_rt_n_backup_processors(void);
extern void sched_set_rt_n_backup_processors(int n);

                                                          
extern void     sys_override_cpu_throttle(boolean_t enable_override);

  
                                                                            
   

extern void                     thread_vm_bind_group_add(void);

                                             
extern kern_return_t clear_wait(
	thread_t                thread,
	wait_result_t   result);

                          
extern void             thread_bootstrap_return(void) __attribute__((noreturn));

                                                   
extern void             thread_exception_return(void) __dead2;

#define SCHED_STRING_MAX_LENGTH (48)
                                                        
extern char sched_string[SCHED_STRING_MAX_LENGTH];

__options_decl(thread_handoff_option_t, uint32_t, {
	THREAD_HANDOFF_NONE          = 0,
	THREAD_HANDOFF_SETRUN_NEEDED = 0x1,
});

                                      
thread_t thread_prepare_for_handoff(thread_t thread, thread_handoff_option_t option);

                                                             
extern wait_result_t thread_handoff_deallocate(thread_t thread, thread_handoff_option_t option);

__attribute__((nonnull(1, 2)))
extern void thread_handoff_parameter(thread_t thread,
    thread_continue_t continuation, void *parameter, thread_handoff_option_t) __dead2;

extern struct waitq     *assert_wait_queue(event_t event);

extern kern_return_t thread_wakeup_one_with_pri(event_t event, int priority);

extern thread_t thread_wakeup_identify(event_t event, int priority);

#endif                          

#ifdef KERNEL_PRIVATE
                                                                                   
extern void             thread_set_pending_block_hint(
	thread_t                        thread,
	block_hint_t                    block_hint);

#define QOS_PARALLELISM_COUNT_LOGICAL   0x1
#define QOS_PARALLELISM_REALTIME        0x2
extern uint32_t qos_max_parallelism(int qos, uint64_t options);
#endif                     

#if XNU_KERNEL_PRIVATE
extern void             thread_yield_with_continuation(
	thread_continue_t       continuation,
	void                            *parameter) __dead2;
#endif

                    
extern wait_result_t    thread_block(
	thread_continue_t       continuation);

extern wait_result_t    thread_block_parameter(
	thread_continue_t       continuation,
	void                            *parameter);

                                                    
extern wait_result_t    assert_wait(
	event_t                         event,
	wait_interrupt_t        interruptible);

                                                           
extern wait_result_t    assert_wait_timeout(
	event_t                         event,
	wait_interrupt_t        interruptible,
	uint32_t                        interval,
	uint32_t                        scale_factor);

                                                                                
extern wait_result_t    assert_wait_timeout_with_leeway(
	event_t                         event,
	wait_interrupt_t        interruptible,
	wait_timeout_urgency_t  urgency,
	uint32_t                        interval,
	uint32_t                        leeway,
	uint32_t                        scale_factor);

extern wait_result_t    assert_wait_deadline(
	event_t                         event,
	wait_interrupt_t        interruptible,
	uint64_t                        deadline);

                                                                                  
extern wait_result_t    assert_wait_deadline_with_leeway(
	event_t                         event,
	wait_interrupt_t        interruptible,
	wait_timeout_urgency_t  urgency,
	uint64_t                        deadline,
	uint64_t                        leeway);

                                                               
extern kern_return_t    thread_wakeup_prim(
	event_t                         event,
	boolean_t                       one_thread,
	wait_result_t                   result);

#define thread_wakeup(x)                                        \
	                thread_wakeup_prim((x), FALSE, THREAD_AWAKENED)
#define thread_wakeup_with_result(x, z)         \
	                thread_wakeup_prim((x), FALSE, (z))
#define thread_wakeup_one(x)                            \
	                thread_wakeup_prim((x), TRUE, THREAD_AWAKENED)

                                                                
extern kern_return_t thread_wakeup_thread(event_t event, thread_t thread);

extern boolean_t preemption_enabled(void);

#ifdef MACH_KERNEL_PRIVATE

  
                                                            
                                                              
                                                            
                            
   

#if   !defined(CONFIG_SCHED_TRADITIONAL) && !defined(CONFIG_SCHED_PROTO) && !defined(CONFIG_SCHED_GRRR) && !defined(CONFIG_SCHED_MULTIQ) && !defined(CONFIG_SCHED_CLUTCH) && !defined(CONFIG_SCHED_EDGE)
#error Enable at least one scheduler algorithm in osfmk/conf/MASTER.XXX
#endif

#if __AMP__

#if CONFIG_SCHED_EDGE
extern const struct sched_dispatch_table sched_edge_dispatch;
#define SCHED(f) (sched_edge_dispatch.f)
#else                        
extern const struct sched_dispatch_table sched_amp_dispatch;
#define SCHED(f) (sched_amp_dispatch.f)
#endif                        

#else              

#if CONFIG_SCHED_CLUTCH
extern const struct sched_dispatch_table sched_clutch_dispatch;
#define SCHED(f) (sched_clutch_dispatch.f)
#else                          
extern const struct sched_dispatch_table sched_dualq_dispatch;
#define SCHED(f) (sched_dualq_dispatch.f)
#endif                          

#endif              

struct sched_dispatch_table {
	const char *sched_name;
	void    (*init)(void);                                                 
	void    (*timebase_init)(void);                                                
	void    (*processor_init)(processor_t processor);                                         
	void    (*pset_init)(processor_set_t pset);                                           

	void    (*maintenance_continuation)(void);                                     

	  
                                                                       
                                        
    
	thread_t        (*choose_thread)(
		processor_t           processor,
		int                           priority,
		ast_t reason);

	                                                               
	bool    (*steal_thread_enabled)(processor_set_t pset);

	  
                                                                        
               
    
	thread_t        (*steal_thread)(
		processor_set_t         pset);

	  
                                                                   
    
	int (*compute_timeshare_priority)(thread_t thread);

	  
                                              
    
	pset_node_t (*choose_node)(
		thread_t                      thread);

	  
                                                                        
    
	processor_t     (*choose_processor)(
		processor_set_t                pset,
		processor_t                    processor,
		thread_t                       thread);
	  
                                                                       
            
    
	boolean_t (*processor_enqueue)(
		processor_t                    processor,
		thread_t                       thread,
		sched_options_t                options);

	                                                                
	void (*processor_queue_shutdown)(
		processor_t                    processor);

	                                                                
	boolean_t       (*processor_queue_remove)(
		processor_t             processor,
		thread_t                thread);

	  
                                                                        
                                                           
                                            
    
	boolean_t       (*processor_queue_empty)(processor_t            processor);

	  
                                                                    
                                  
    
	boolean_t       (*priority_is_urgent)(int priority);

	  
                                                                 
                                                              
    
	ast_t           (*processor_csw_check)(processor_t processor);

	  
                                                             
                                                                    
                    
    
	boolean_t       (*processor_queue_has_priority)(processor_t             processor,
	    int                             priority,
	    boolean_t               gte);

	                                                         
	uint32_t        (*initial_quantum_size)(thread_t thread);

	                                     
	sched_mode_t    (*initial_thread_sched_mode)(task_t parent_task);

	  
                                                                   
                                                                 
                        
    
	boolean_t       (*can_update_priority)(thread_t thread);

	  
                                                              
                                                                         
    
	void            (*update_priority)(thread_t thread);

	                                                            
	void            (*lightweight_update_priority)(thread_t thread);

	                                                                    
	void            (*quantum_expire)(thread_t thread);

	  
                                                           
                                                                
    
	int                     (*processor_runq_count)(processor_t     processor);

	                                                              
	uint64_t    (*processor_runq_stats_count_sum)(processor_t   processor);

	boolean_t       (*processor_bound_count)(processor_t processor);

	void            (*thread_update_scan)(sched_update_scan_context_t scan_context);

	                                 
	boolean_t   multiple_psets_enabled;
	                               
	boolean_t   sched_groups_enabled;

	                              
	boolean_t   avoid_processor_enabled;

	                                                                      
	bool    (*thread_avoid_processor)(processor_t processor, thread_t thread);

	  
                                                               
                                                                                 
                                                           
    
	void    (*processor_balance)(processor_t processor, processor_set_t pset);
	rt_queue_t      (*rt_runq)(processor_set_t pset);
	void    (*rt_init)(processor_set_t pset);
	void    (*rt_queue_shutdown)(processor_t processor);
	void    (*rt_runq_scan)(sched_update_scan_context_t scan_context);
	int64_t (*rt_runq_count_sum)(void);

	uint32_t (*qos_max_parallelism)(int qos, uint64_t options);
	void    (*check_spill)(processor_set_t pset, thread_t thread);
	sched_ipi_type_t (*ipi_policy)(processor_t dst, thread_t thread, boolean_t dst_idle, sched_ipi_event_t event);
	bool    (*thread_should_yield)(processor_t processor, thread_t thread);

	                                  
	uint32_t (*run_count_incr)(thread_t thread);
	uint32_t (*run_count_decr)(thread_t thread);

	                                                      
	void (*update_thread_bucket)(thread_t thread);

	                                                                         
	void (*pset_made_schedulable)(processor_t processor, processor_set_t pset, boolean_t drop_lock);
#if CONFIG_THREAD_GROUPS
	                                                                                     
	void (*thread_group_recommendation_change)(struct thread_group *tg, cluster_type_t new_recommendation);
#endif
};

#if defined(CONFIG_SCHED_TRADITIONAL)
extern const struct sched_dispatch_table sched_traditional_dispatch;
extern const struct sched_dispatch_table sched_traditional_with_pset_runqueue_dispatch;
#endif

#if defined(CONFIG_SCHED_MULTIQ)
extern const struct sched_dispatch_table sched_multiq_dispatch;
extern const struct sched_dispatch_table sched_dualq_dispatch;
#if __AMP__
extern const struct sched_dispatch_table sched_amp_dispatch;
#endif
#endif

#if defined(CONFIG_SCHED_PROTO)
extern const struct sched_dispatch_table sched_proto_dispatch;
#endif

#if defined(CONFIG_SCHED_GRRR)
extern const struct sched_dispatch_table sched_grrr_dispatch;
#endif

#if defined(CONFIG_SCHED_CLUTCH)
extern const struct sched_dispatch_table sched_clutch_dispatch;
#endif

#if defined(CONFIG_SCHED_EDGE)
extern const struct sched_dispatch_table sched_edge_dispatch;
#endif


#endif                           

__END_DECLS

#endif                           
