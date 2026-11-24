/*
 * Copyright (c) 2000-2015 Apple Inc. All rights reserved.
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

#include <mach/mach_types.h>
#include <mach/thread_act_server.h>

#include <kern/kern_types.h>
#include <kern/processor.h>
#include <kern/thread.h>
#include <kern/affinity.h>
#include <mach/task_policy.h>
#include <kern/sfi.h>
#include <kern/policy_internal.h>
#include <sys/errno.h>
#include <sys/ulock.h>

#include <mach/machine/sdt.h>

#ifdef MACH_BSD
extern int      proc_selfpid(void);
extern char *   proc_name_address(void *p);
extern void     rethrottle_thread(void * uthread);
#endif               

#define QOS_EXTRACT(q)        ((q) & 0xff)

uint32_t qos_override_mode;
#define QOS_OVERRIDE_MODE_OVERHANG_PEAK 0
#define QOS_OVERRIDE_MODE_IGNORE_OVERRIDE 1
#define QOS_OVERRIDE_MODE_FINE_GRAINED_OVERRIDE 2
#define QOS_OVERRIDE_MODE_FINE_GRAINED_OVERRIDE_BUT_SINGLE_MUTEX_OVERRIDE 3

extern zone_t thread_qos_override_zone;

static void
proc_thread_qos_remove_override_internal(thread_t thread, user_addr_t resource, int resource_type, boolean_t reset);

  
                                                                                                
                                              
   
const qos_policy_params_t thread_qos_policy_params = {
	  
                                                                
                                                                             
                         
    
	.qos_pri[THREAD_QOS_UNSPECIFIED]                = 0,                    
	.qos_pri[THREAD_QOS_USER_INTERACTIVE]           = BASEPRI_BACKGROUND,              
	.qos_pri[THREAD_QOS_USER_INITIATED]             = BASEPRI_USER_INITIATED,
	.qos_pri[THREAD_QOS_LEGACY]                     = BASEPRI_DEFAULT,
	.qos_pri[THREAD_QOS_UTILITY]                    = BASEPRI_UTILITY,
	.qos_pri[THREAD_QOS_BACKGROUND]                 = MAXPRI_THROTTLE,
	.qos_pri[THREAD_QOS_MAINTENANCE]                = MAXPRI_THROTTLE,

	  
                                                                             
                       
    
	.qos_iotier[THREAD_QOS_UNSPECIFIED]             = THROTTLE_LEVEL_TIER0,
	.qos_iotier[THREAD_QOS_USER_INTERACTIVE]        = THROTTLE_LEVEL_TIER0,
	.qos_iotier[THREAD_QOS_USER_INITIATED]          = THROTTLE_LEVEL_TIER0,
	.qos_iotier[THREAD_QOS_LEGACY]                  = THROTTLE_LEVEL_TIER0,
	.qos_iotier[THREAD_QOS_UTILITY]                 = THROTTLE_LEVEL_TIER1,
	.qos_iotier[THREAD_QOS_BACKGROUND]              = THROTTLE_LEVEL_TIER2,                                       
	.qos_iotier[THREAD_QOS_MAINTENANCE]             = THROTTLE_LEVEL_TIER3,

	  
                                                 
                                                 
    

	.qos_through_qos[THREAD_QOS_UNSPECIFIED]        = QOS_EXTRACT(THROUGHPUT_QOS_TIER_UNSPECIFIED),
	.qos_through_qos[THREAD_QOS_USER_INTERACTIVE]   = QOS_EXTRACT(THROUGHPUT_QOS_TIER_0),
	.qos_through_qos[THREAD_QOS_USER_INITIATED]     = QOS_EXTRACT(THROUGHPUT_QOS_TIER_1),
	.qos_through_qos[THREAD_QOS_LEGACY]             = QOS_EXTRACT(THROUGHPUT_QOS_TIER_1),
	.qos_through_qos[THREAD_QOS_UTILITY]            = QOS_EXTRACT(THROUGHPUT_QOS_TIER_2),
	.qos_through_qos[THREAD_QOS_BACKGROUND]         = QOS_EXTRACT(THROUGHPUT_QOS_TIER_5),
	.qos_through_qos[THREAD_QOS_MAINTENANCE]        = QOS_EXTRACT(THROUGHPUT_QOS_TIER_5),

	.qos_latency_qos[THREAD_QOS_UNSPECIFIED]        = QOS_EXTRACT(LATENCY_QOS_TIER_UNSPECIFIED),
	.qos_latency_qos[THREAD_QOS_USER_INTERACTIVE]   = QOS_EXTRACT(LATENCY_QOS_TIER_0),
	.qos_latency_qos[THREAD_QOS_USER_INITIATED]     = QOS_EXTRACT(LATENCY_QOS_TIER_1),
	.qos_latency_qos[THREAD_QOS_LEGACY]             = QOS_EXTRACT(LATENCY_QOS_TIER_1),
	.qos_latency_qos[THREAD_QOS_UTILITY]            = QOS_EXTRACT(LATENCY_QOS_TIER_3),
	.qos_latency_qos[THREAD_QOS_BACKGROUND]         = QOS_EXTRACT(LATENCY_QOS_TIER_3),
	.qos_latency_qos[THREAD_QOS_MAINTENANCE]        = QOS_EXTRACT(LATENCY_QOS_TIER_3),
};

static void
thread_set_user_sched_mode_and_recompute_pri(thread_t thread, sched_mode_t mode);

static int
thread_qos_scaled_relative_priority(int qos, int qos_relprio);

static void
proc_get_thread_policy_bitfield(thread_t thread, thread_policy_state_t info);

static void
proc_set_thread_policy_locked(thread_t thread, int category, int flavor, int value, int value2, task_pend_token_t pend_token);

static void
proc_set_thread_policy_spinlocked(thread_t thread, int category, int flavor, int value, int value2, task_pend_token_t pend_token);

static void
thread_set_requested_policy_spinlocked(thread_t thread, int category, int flavor, int value, int value2, task_pend_token_t pend_token);

static int
thread_get_requested_policy_spinlocked(thread_t thread, int category, int flavor, int* value2);

static int
proc_get_thread_policy_locked(thread_t thread, int category, int flavor, int* value2);

static void
thread_policy_update_spinlocked(thread_t thread, bool recompute_priority, task_pend_token_t pend_token);

static void
thread_policy_update_internal_spinlocked(thread_t thread, bool recompute_priority, task_pend_token_t pend_token);

void
thread_policy_init(void)
{
	if (PE_parse_boot_argn("qos_override_mode", &qos_override_mode, sizeof(qos_override_mode))) {
		printf("QOS override mode: 0x%08x\n", qos_override_mode);
	} else {
		qos_override_mode = QOS_OVERRIDE_MODE_FINE_GRAINED_OVERRIDE_BUT_SINGLE_MUTEX_OVERRIDE;
	}
}

boolean_t
thread_has_qos_policy(thread_t thread)
{
	return (proc_get_thread_policy(thread, TASK_POLICY_ATTRIBUTE, TASK_POLICY_QOS) != THREAD_QOS_UNSPECIFIED) ? TRUE : FALSE;
}


static void
thread_remove_qos_policy_locked(thread_t thread,
    task_pend_token_t pend_token)
{
	__unused int prev_qos = thread->requested_policy.thrp_qos;

	DTRACE_PROC2(qos__remove, thread_t, thread, int, prev_qos);

	proc_set_thread_policy_locked(thread, TASK_POLICY_ATTRIBUTE, TASK_POLICY_QOS_AND_RELPRIO,
	    THREAD_QOS_UNSPECIFIED, 0, pend_token);
}

kern_return_t
thread_remove_qos_policy(thread_t thread)
{
	struct task_pend_token pend_token = {};

	thread_mtx_lock(thread);
	if (!thread->active) {
		thread_mtx_unlock(thread);
		return KERN_TERMINATED;
	}

	thread_remove_qos_policy_locked(thread, &pend_token);

	thread_mtx_unlock(thread);

	thread_policy_update_complete_unlocked(thread, &pend_token);

	return KERN_SUCCESS;
}


boolean_t
thread_is_static_param(thread_t thread)
{
	if (thread->static_param) {
		DTRACE_PROC1(qos__legacy__denied, thread_t, thread);
		return TRUE;
	}
	return FALSE;
}

  
                                                               
                                                                 
          
   
static int
thread_qos_scaled_relative_priority(int qos, int qos_relprio)
{
	int next_lower_qos;

	                                                         
	if (qos_relprio == 0) {
		return 0;
	}

	switch (qos) {
	case THREAD_QOS_USER_INTERACTIVE:
		next_lower_qos = THREAD_QOS_USER_INITIATED;
		break;
	case THREAD_QOS_USER_INITIATED:
		next_lower_qos = THREAD_QOS_LEGACY;
		break;
	case THREAD_QOS_LEGACY:
		next_lower_qos = THREAD_QOS_UTILITY;
		break;
	case THREAD_QOS_UTILITY:
		next_lower_qos = THREAD_QOS_BACKGROUND;
		break;
	case THREAD_QOS_MAINTENANCE:
	case THREAD_QOS_BACKGROUND:
		next_lower_qos = 0;
		break;
	default:
		panic("Unrecognized QoS %d", qos);
		return 0;
	}

	int prio_range_max = thread_qos_policy_params.qos_pri[qos];
	int prio_range_min = next_lower_qos ? thread_qos_policy_params.qos_pri[next_lower_qos] : 0;

	  
                                                                                  
                                                                               
                                                                                   
                                                                                        
              
    
	int scaled_relprio = -(((prio_range_max - prio_range_min) * (-qos_relprio)) >> 4);

	return scaled_relprio;
}

  
                                                  
                                           
   
static TUNABLE(bool, allow_qos_policy_set, "-qos-policy-allow", false);

kern_return_t
thread_policy_set(
	thread_t                                thread,
	thread_policy_flavor_t  flavor,
	thread_policy_t                 policy_info,
	mach_msg_type_number_t  count)
{
	thread_qos_policy_data_t req_qos;
	kern_return_t kr;

	req_qos.qos_tier = THREAD_QOS_UNSPECIFIED;

	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (!allow_qos_policy_set) {
		if (thread_is_static_param(thread)) {
			return KERN_POLICY_STATIC;
		}

		if (flavor == THREAD_QOS_POLICY) {
			return KERN_INVALID_ARGUMENT;
		}
	}

	                                                                                       
	if (thread->requested_policy.thrp_qos != THREAD_QOS_UNSPECIFIED) {
		                                                                             
		req_qos.qos_tier = thread->requested_policy.thrp_qos;
		req_qos.tier_importance = thread->requested_policy.thrp_qos_relprio;

		kr = thread_remove_qos_policy(thread);
		if (kr != KERN_SUCCESS) {
			return kr;
		}
	}

	kr = thread_policy_set_internal(thread, flavor, policy_info, count);

	                                                                      
	if (req_qos.qos_tier != THREAD_QOS_UNSPECIFIED) {
		if (kr != KERN_SUCCESS) {
			                                                        
			(void)thread_policy_set_internal(thread, THREAD_QOS_POLICY, (thread_policy_t)&req_qos, THREAD_QOS_POLICY_COUNT);
		}
	}

	return kr;
}

kern_return_t
thread_policy_set_internal(
	thread_t                     thread,
	thread_policy_flavor_t       flavor,
	thread_policy_t              policy_info,
	mach_msg_type_number_t       count)
{
	kern_return_t result = KERN_SUCCESS;
	struct task_pend_token pend_token = {};

	thread_mtx_lock(thread);
	if (!thread->active) {
		thread_mtx_unlock(thread);

		return KERN_TERMINATED;
	}

	switch (flavor) {
	case THREAD_EXTENDED_POLICY:
	{
		boolean_t timeshare = TRUE;

		if (count >= THREAD_EXTENDED_POLICY_COUNT) {
			thread_extended_policy_t info;

			info = (thread_extended_policy_t)policy_info;
			timeshare = info->timeshare;
		}

		sched_mode_t mode = (timeshare == TRUE) ? TH_MODE_TIMESHARE : TH_MODE_FIXED;

		spl_t s = splsched();
		thread_lock(thread);

		thread_set_user_sched_mode_and_recompute_pri(thread, mode);

		thread_unlock(thread);
		splx(s);

		pend_token.tpt_update_thread_sfi = 1;

		break;
	}

	case THREAD_TIME_CONSTRAINT_POLICY:
	{
		thread_time_constraint_policy_t info;

		if (count < THREAD_TIME_CONSTRAINT_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		info = (thread_time_constraint_policy_t)policy_info;


		if (info->constraint < info->computation ||
		    info->computation > max_rt_quantum ||
		    info->computation < min_rt_quantum) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		spl_t s = splsched();
		thread_lock(thread);

		thread->realtime.period         = info->period;
		thread->realtime.computation    = info->computation;
		thread->realtime.constraint     = info->constraint;
		thread->realtime.preemptible    = info->preemptible;

		thread_set_user_sched_mode_and_recompute_pri(thread, TH_MODE_REALTIME);

		thread_unlock(thread);
		splx(s);

		pend_token.tpt_update_thread_sfi = 1;

		break;
	}

	case THREAD_PRECEDENCE_POLICY:
	{
		thread_precedence_policy_t info;

		if (count < THREAD_PRECEDENCE_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}
		info = (thread_precedence_policy_t)policy_info;

		spl_t s = splsched();
		thread_lock(thread);

		thread->importance = info->importance;

		thread_recompute_priority(thread);

		thread_unlock(thread);
		splx(s);

		break;
	}

	case THREAD_AFFINITY_POLICY:
	{
		thread_affinity_policy_t info;

		if (!thread_affinity_is_supported()) {
			result = KERN_NOT_SUPPORTED;
			break;
		}
		if (count < THREAD_AFFINITY_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		info = (thread_affinity_policy_t) policy_info;
		  
                                     
                                                         
                                                        
                                               
     
		thread_mtx_unlock(thread);
		return thread_affinity_set(thread, info->affinity_tag);
	}

#if !defined(XNU_TARGET_OS_OSX)
	case THREAD_BACKGROUND_POLICY:
	{
		thread_background_policy_t info;

		if (count < THREAD_BACKGROUND_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		if (thread->task != current_task()) {
			result = KERN_PROTECTION_FAILURE;
			break;
		}

		info = (thread_background_policy_t) policy_info;

		int enable;

		if (info->priority == THREAD_BACKGROUND_POLICY_DARWIN_BG) {
			enable = TASK_POLICY_ENABLE;
		} else {
			enable = TASK_POLICY_DISABLE;
		}

		int category = (current_thread() == thread) ? TASK_POLICY_INTERNAL : TASK_POLICY_EXTERNAL;

		proc_set_thread_policy_locked(thread, category, TASK_POLICY_DARWIN_BG, enable, 0, &pend_token);

		break;
	}
#endif                                  

	case THREAD_THROUGHPUT_QOS_POLICY:
	{
		thread_throughput_qos_policy_t info = (thread_throughput_qos_policy_t) policy_info;
		thread_throughput_qos_t tqos;

		if (count < THREAD_THROUGHPUT_QOS_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		if ((result = qos_throughput_policy_validate(info->thread_throughput_qos_tier)) != KERN_SUCCESS) {
			break;
		}

		tqos = qos_extract(info->thread_throughput_qos_tier);

		proc_set_thread_policy_locked(thread, TASK_POLICY_ATTRIBUTE,
		    TASK_POLICY_THROUGH_QOS, tqos, 0, &pend_token);

		break;
	}

	case THREAD_LATENCY_QOS_POLICY:
	{
		thread_latency_qos_policy_t info = (thread_latency_qos_policy_t) policy_info;
		thread_latency_qos_t lqos;

		if (count < THREAD_LATENCY_QOS_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		if ((result = qos_latency_policy_validate(info->thread_latency_qos_tier)) != KERN_SUCCESS) {
			break;
		}

		lqos = qos_extract(info->thread_latency_qos_tier);

		proc_set_thread_policy_locked(thread, TASK_POLICY_ATTRIBUTE,
		    TASK_POLICY_LATENCY_QOS, lqos, 0, &pend_token);

		break;
	}

	case THREAD_QOS_POLICY:
	{
		thread_qos_policy_t info = (thread_qos_policy_t)policy_info;

		if (count < THREAD_QOS_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		if (info->qos_tier < 0 || info->qos_tier >= THREAD_QOS_LAST) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		if (info->tier_importance > 0 || info->tier_importance < THREAD_QOS_MIN_TIER_IMPORTANCE) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		if (info->qos_tier == THREAD_QOS_UNSPECIFIED && info->tier_importance != 0) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		proc_set_thread_policy_locked(thread, TASK_POLICY_ATTRIBUTE, TASK_POLICY_QOS_AND_RELPRIO,
		    info->qos_tier, -info->tier_importance, &pend_token);

		break;
	}

	default:
		result = KERN_INVALID_ARGUMENT;
		break;
	}

	thread_mtx_unlock(thread);

	thread_policy_update_complete_unlocked(thread, &pend_token);

	return result;
}

  
                                                                                  
                                        
   
static sched_mode_t
convert_policy_to_sched_mode(integer_t policy)
{
	switch (policy) {
	case POLICY_TIMESHARE:
		return TH_MODE_TIMESHARE;
	case POLICY_RR:
	case POLICY_FIFO:
		return TH_MODE_FIXED;
	default:
		panic("unexpected sched policy: %d", policy);
		return TH_MODE_NONE;
	}
}

  
                                             
                                              
   
static kern_return_t
thread_set_mode_and_absolute_pri_internal(thread_t              thread,
    sched_mode_t          mode,
    integer_t             priority,
    task_pend_token_t     pend_token)
{
	kern_return_t kr = KERN_SUCCESS;

	spl_t s = splsched();
	thread_lock(thread);

	                                                                 
	if ((thread->sched_mode == TH_MODE_REALTIME) ||
	    (thread->saved_mode == TH_MODE_REALTIME)) {
		kr = KERN_FAILURE;
		goto unlock;
	}

	if (thread->policy_reset) {
		kr = KERN_SUCCESS;
		goto unlock;
	}

	sched_mode_t old_mode = thread->sched_mode;

	  
                                                           
                                               
   
                                                   
    

	if (priority >= thread->max_priority) {
		priority = thread->max_priority - thread->task_priority;
	} else if (priority >= MINPRI_KERNEL) {
		priority -=  MINPRI_KERNEL;
	} else if (priority >= MINPRI_RESERVED) {
		priority -=  MINPRI_RESERVED;
	} else {
		priority -= BASEPRI_DEFAULT;
	}

	priority += thread->task_priority;

	if (priority > thread->max_priority) {
		priority = thread->max_priority;
	} else if (priority < MINPRI) {
		priority = MINPRI;
	}

	thread->importance = priority - thread->task_priority;

	thread_set_user_sched_mode_and_recompute_pri(thread, mode);

	if (mode != old_mode) {
		pend_token->tpt_update_thread_sfi = 1;
	}

unlock:
	thread_unlock(thread);
	splx(s);

	return kr;
}

void
thread_freeze_base_pri(thread_t thread)
{
	assert(thread == current_thread());

	spl_t s = splsched();
	thread_lock(thread);

	assert((thread->sched_flags & TH_SFLAG_BASE_PRI_FROZEN) == 0);
	thread->sched_flags |= TH_SFLAG_BASE_PRI_FROZEN;

	thread_unlock(thread);
	splx(s);
}

bool
thread_unfreeze_base_pri(thread_t thread)
{
	assert(thread == current_thread());
	integer_t base_pri;
	ast_t ast = 0;

	spl_t s = splsched();
	thread_lock(thread);

	assert(thread->sched_flags & TH_SFLAG_BASE_PRI_FROZEN);
	thread->sched_flags &= ~TH_SFLAG_BASE_PRI_FROZEN;

	base_pri = thread->req_base_pri;
	if (base_pri != thread->base_pri) {
		  
                                                        
                                                 
     
		sched_set_thread_base_priority(thread, base_pri);
		ast = ast_peek(AST_PREEMPT);
	}

	thread_unlock(thread);
	splx(s);

	return ast != 0;
}

uint8_t
thread_workq_pri_for_qos(thread_qos_t qos)
{
	assert(qos < THREAD_QOS_LAST);
	return (uint8_t)thread_qos_policy_params.qos_pri[qos];
}

thread_qos_t
thread_workq_qos_for_pri(int priority)
{
	thread_qos_t qos;
	if (priority > thread_qos_policy_params.qos_pri[THREAD_QOS_USER_INTERACTIVE]) {
		                                                        
		                                       
		return THREAD_QOS_UNSPECIFIED;
	}
	for (qos = THREAD_QOS_USER_INTERACTIVE; qos > THREAD_QOS_MAINTENANCE; qos--) {
		                                                        
		if (thread_qos_policy_params.qos_pri[qos - 1] < priority) {
			return qos;
		}
	}
	return THREAD_QOS_MAINTENANCE;
}

  
                                           
  
                                                       
                                    
                                
   
void
thread_reset_workq_qos(thread_t thread, uint32_t qos)
{
	struct task_pend_token pend_token = {};

	assert(qos < THREAD_QOS_LAST);

	spl_t s = splsched();
	thread_lock(thread);

	proc_set_thread_policy_spinlocked(thread, TASK_POLICY_ATTRIBUTE,
	    TASK_POLICY_QOS_AND_RELPRIO, qos, 0, &pend_token);
	proc_set_thread_policy_spinlocked(thread, TASK_POLICY_ATTRIBUTE,
	    TASK_POLICY_QOS_WORKQ_OVERRIDE, THREAD_QOS_UNSPECIFIED, 0,
	    &pend_token);

	assert(pend_token.tpt_update_sockets == 0);

	thread_unlock(thread);
	splx(s);

	thread_policy_update_complete_unlocked(thread, &pend_token);
}

  
                                           
  
                                                       
                                    
                            
   
void
thread_set_workq_override(thread_t thread, uint32_t qos)
{
	struct task_pend_token pend_token = {};

	assert(qos < THREAD_QOS_LAST);

	spl_t s = splsched();
	thread_lock(thread);

	proc_set_thread_policy_spinlocked(thread, TASK_POLICY_ATTRIBUTE,
	    TASK_POLICY_QOS_WORKQ_OVERRIDE, qos, 0, &pend_token);

	assert(pend_token.tpt_update_sockets == 0);

	thread_unlock(thread);
	splx(s);

	thread_policy_update_complete_unlocked(thread, &pend_token);
}

  
                                           
  
                                                       
                                    
                                
   
void
thread_set_workq_pri(thread_t  thread,
    thread_qos_t qos,
    integer_t priority,
    integer_t policy)
{
	struct task_pend_token pend_token = {};
	sched_mode_t mode = convert_policy_to_sched_mode(policy);

	assert(qos < THREAD_QOS_LAST);
	assert(thread->static_param);

	if (!thread->static_param || !thread->active) {
		return;
	}

	spl_t s = splsched();
	thread_lock(thread);

	proc_set_thread_policy_spinlocked(thread, TASK_POLICY_ATTRIBUTE,
	    TASK_POLICY_QOS_AND_RELPRIO, qos, 0, &pend_token);
	proc_set_thread_policy_spinlocked(thread, TASK_POLICY_ATTRIBUTE,
	    TASK_POLICY_QOS_WORKQ_OVERRIDE, THREAD_QOS_UNSPECIFIED,
	    0, &pend_token);

	thread_unlock(thread);
	splx(s);

	                                             

	__assert_only kern_return_t kr;
	kr = thread_set_mode_and_absolute_pri_internal(thread, mode, priority,
	    &pend_token);
	assert(kr == KERN_SUCCESS);

	if (pend_token.tpt_update_thread_sfi) {
		sfi_reevaluate(thread);
	}
}

  
                                    
  
                                                                       
                                                  
  
                              
   
kern_return_t
thread_set_mode_and_absolute_pri(thread_t   thread,
    integer_t  policy,
    integer_t  priority)
{
	kern_return_t kr = KERN_SUCCESS;
	struct task_pend_token pend_token = {};

	sched_mode_t mode = convert_policy_to_sched_mode(policy);

	thread_mtx_lock(thread);

	if (!thread->active) {
		kr = KERN_TERMINATED;
		goto unlock;
	}

	if (thread_is_static_param(thread)) {
		kr = KERN_POLICY_STATIC;
		goto unlock;
	}

	                                                              
	if (thread->requested_policy.thrp_qos != THREAD_QOS_UNSPECIFIED) {
		thread_remove_qos_policy_locked(thread, &pend_token);
	}

	kr = thread_set_mode_and_absolute_pri_internal(thread, mode, priority, &pend_token);

unlock:
	thread_mtx_unlock(thread);

	thread_policy_update_complete_unlocked(thread, &pend_token);

	return kr;
}

  
                                                         
                                             
  
                                                                           
                                      
                                                                                           
   
static void
thread_set_user_sched_mode_and_recompute_pri(thread_t thread, sched_mode_t mode)
{
	if (thread->policy_reset) {
		return;
	}

	boolean_t removed = thread_run_queue_remove(thread);

	  
                                                                         
                                                             
                               
    
	if (thread->sched_flags & TH_SFLAG_DEMOTED_MASK) {
		thread->saved_mode = mode;
	} else {
		sched_set_thread_mode(thread, mode);
	}

	thread_recompute_priority(thread);

	if (removed) {
		thread_run_queue_reinsert(thread, SCHED_TAILQ);
	}
}

                                                
static void
thread_update_qos_cpu_time_locked(thread_t thread)
{
	task_t task = thread->task;
	uint64_t timer_sum, timer_delta;

	  
                                                    
                                                                                    
                                                              
   
                                                                            
                                                                    
                                                                             
   
                                                        
    

	timer_sum = timer_grab(&thread->user_timer);
	timer_sum += timer_grab(&thread->system_timer);
	timer_delta = timer_sum - thread->vtimer_qos_save;

	thread->vtimer_qos_save = timer_sum;

	uint64_t* task_counter = NULL;

	                                                                                                              
	switch (thread->effective_policy.thep_qos) {
	case THREAD_QOS_UNSPECIFIED:        task_counter = &task->cpu_time_eqos_stats.cpu_time_qos_default; break;
	case THREAD_QOS_MAINTENANCE:        task_counter = &task->cpu_time_eqos_stats.cpu_time_qos_maintenance; break;
	case THREAD_QOS_BACKGROUND:         task_counter = &task->cpu_time_eqos_stats.cpu_time_qos_background; break;
	case THREAD_QOS_UTILITY:            task_counter = &task->cpu_time_eqos_stats.cpu_time_qos_utility; break;
	case THREAD_QOS_LEGACY:             task_counter = &task->cpu_time_eqos_stats.cpu_time_qos_legacy; break;
	case THREAD_QOS_USER_INITIATED:     task_counter = &task->cpu_time_eqos_stats.cpu_time_qos_user_initiated; break;
	case THREAD_QOS_USER_INTERACTIVE:   task_counter = &task->cpu_time_eqos_stats.cpu_time_qos_user_interactive; break;
	default:
		panic("unknown effective QoS: %d", thread->effective_policy.thep_qos);
	}

	OSAddAtomic64(timer_delta, task_counter);

	                                                                                      
	switch (thread->requested_policy.thrp_qos) {
	case THREAD_QOS_UNSPECIFIED:        task_counter = &task->cpu_time_rqos_stats.cpu_time_qos_default; break;
	case THREAD_QOS_MAINTENANCE:        task_counter = &task->cpu_time_rqos_stats.cpu_time_qos_maintenance; break;
	case THREAD_QOS_BACKGROUND:         task_counter = &task->cpu_time_rqos_stats.cpu_time_qos_background; break;
	case THREAD_QOS_UTILITY:            task_counter = &task->cpu_time_rqos_stats.cpu_time_qos_utility; break;
	case THREAD_QOS_LEGACY:             task_counter = &task->cpu_time_rqos_stats.cpu_time_qos_legacy; break;
	case THREAD_QOS_USER_INITIATED:     task_counter = &task->cpu_time_rqos_stats.cpu_time_qos_user_initiated; break;
	case THREAD_QOS_USER_INTERACTIVE:   task_counter = &task->cpu_time_rqos_stats.cpu_time_qos_user_interactive; break;
	default:
		panic("unknown requested QoS: %d", thread->requested_policy.thrp_qos);
	}

	OSAddAtomic64(timer_delta, task_counter);
}

  
                                   
                     
   
void
thread_update_qos_cpu_time(thread_t thread)
{
	thread_mtx_lock(thread);

	spl_t s = splsched();
	thread_lock(thread);

	thread_update_qos_cpu_time_locked(thread);

	thread_unlock(thread);
	splx(s);

	thread_mtx_unlock(thread);
}

  
                                                                           
  
                                                 
   
extern thread_t vm_pageout_scan_thread;
extern boolean_t vps_dynamic_priority_enabled;

void
thread_recompute_priority(
	thread_t                thread)
{
	integer_t               priority;

	if (thread->policy_reset) {
		return;
	}

	if (thread->sched_mode == TH_MODE_REALTIME) {
		sched_set_thread_base_priority(thread, BASEPRI_RTQUEUES);
		return;
	} else if (thread->effective_policy.thep_qos != THREAD_QOS_UNSPECIFIED) {
		int qos = thread->effective_policy.thep_qos;
		int qos_ui_is_urgent = thread->effective_policy.thep_qos_ui_is_urgent;
		int qos_relprio = -(thread->effective_policy.thep_qos_relprio);                                     
		int qos_scaled_relprio;

		assert(qos >= 0 && qos < THREAD_QOS_LAST);
		assert(qos_relprio <= 0 && qos_relprio >= THREAD_QOS_MIN_TIER_IMPORTANCE);

		priority = thread_qos_policy_params.qos_pri[qos];
		qos_scaled_relprio = thread_qos_scaled_relative_priority(qos, qos_relprio);

		if (qos == THREAD_QOS_USER_INTERACTIVE && qos_ui_is_urgent == 1) {
			                                                    
			qos_scaled_relprio += 1;
		}

		                                           

		priority += qos_scaled_relprio;
	} else {
		if (thread->importance > MAXPRI) {
			priority = MAXPRI;
		} else if (thread->importance < -MAXPRI) {
			priority = -MAXPRI;
		} else {
			priority = thread->importance;
		}

		priority += thread->task_priority;
	}

	priority = MAX(priority, thread->user_promotion_basepri);

	  
                                                             
                                                                  
                                                            
                                          
                                            
                                                                
                                 
    
	if (priority > thread->max_priority) {
		if (thread->effective_policy.thep_promote_above_task) {
			priority = MAX(thread->max_priority, thread->user_promotion_basepri);
		} else {
			priority = thread->max_priority;
		}
	} else if (priority < MINPRI) {
		priority = MINPRI;
	}

	if (thread->saved_mode == TH_MODE_REALTIME &&
	    thread->sched_flags & TH_SFLAG_FAILSAFE) {
		priority = DEPRESSPRI;
	}

	if (thread->effective_policy.thep_terminated == TRUE) {
		  
                                                             
                                                     
                                                             
                                                          
                                 
     
		if (priority < thread->task_priority) {
			priority = thread->task_priority;
		}
		if (priority < BASEPRI_DEFAULT) {
			priority = BASEPRI_DEFAULT;
		}
	}

#if !defined(XNU_TARGET_OS_OSX)
	                                                               
	if (priority < MAXPRI_THROTTLE) {
		priority = MAXPRI_THROTTLE;
	}
#endif                                  

	sched_set_thread_base_priority(thread, priority);
}

                                                                          
void
thread_policy_update_tasklocked(
	thread_t           thread,
	integer_t          priority,
	integer_t          max_priority,
	task_pend_token_t  pend_token)
{
	thread_mtx_lock(thread);

	if (!thread->active || thread->policy_reset) {
		thread_mtx_unlock(thread);
		return;
	}

	spl_t s = splsched();
	thread_lock(thread);

	__unused
	integer_t old_max_priority = thread->max_priority;

	assert(priority >= INT16_MIN && priority <= INT16_MAX);
	thread->task_priority = (int16_t)priority;

	assert(max_priority >= INT16_MIN && max_priority <= INT16_MAX);
	thread->max_priority = (int16_t)max_priority;

	  
                                                                    
                                                      
   
                                                                                  
                                         
    
	if ((max_priority <= MAXPRI_THROTTLE) && (old_max_priority > MAXPRI_THROTTLE)) {
		sched_thread_mode_demote(thread, TH_SFLAG_THROTTLED);
	} else if ((max_priority > MAXPRI_THROTTLE) && (old_max_priority <= MAXPRI_THROTTLE)) {
		sched_thread_mode_undemote(thread, TH_SFLAG_THROTTLED);
	}

	thread_policy_update_spinlocked(thread, true, pend_token);

	thread_unlock(thread);
	splx(s);

	thread_mtx_unlock(thread);
}

  
                                                               
                                  
  
                                                                       
   
void
thread_policy_reset(
	thread_t                thread)
{
	spl_t           s;

	assert(thread == current_thread());

	s = splsched();
	thread_lock(thread);

	if (thread->sched_flags & TH_SFLAG_FAILSAFE) {
		sched_thread_mode_undemote(thread, TH_SFLAG_FAILSAFE);
	}

	if (thread->sched_flags & TH_SFLAG_THROTTLED) {
		sched_thread_mode_undemote(thread, TH_SFLAG_THROTTLED);
	}

	                                                             
	assert(!(thread->sched_flags & TH_SFLAG_DEMOTED_MASK));
	assert(!(thread->sched_flags & TH_SFLAG_THROTTLED));
	assert(!(thread->sched_flags & TH_SFLAG_DEPRESSED_MASK));

	                                                         
	sched_mode_t newmode = SCHED(initial_thread_sched_mode)(thread->task);

	sched_set_thread_mode(thread, newmode);

	thread->importance = 0;

	                                                             
	thread->policy_reset = 1;

	sched_set_thread_base_priority(thread, thread->task_priority);

	thread_unlock(thread);
	splx(s);
}

kern_return_t
thread_policy_get(
	thread_t                                thread,
	thread_policy_flavor_t  flavor,
	thread_policy_t                 policy_info,
	mach_msg_type_number_t  *count,
	boolean_t                               *get_default)
{
	kern_return_t                   result = KERN_SUCCESS;

	if (thread == THREAD_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	thread_mtx_lock(thread);
	if (!thread->active) {
		thread_mtx_unlock(thread);

		return KERN_TERMINATED;
	}

	switch (flavor) {
	case THREAD_EXTENDED_POLICY:
	{
		boolean_t               timeshare = TRUE;

		if (!(*get_default)) {
			spl_t s = splsched();
			thread_lock(thread);

			if ((thread->sched_mode != TH_MODE_REALTIME) &&
			    (thread->saved_mode != TH_MODE_REALTIME)) {
				if (!(thread->sched_flags & TH_SFLAG_DEMOTED_MASK)) {
					timeshare = (thread->sched_mode == TH_MODE_TIMESHARE) != 0;
				} else {
					timeshare = (thread->saved_mode == TH_MODE_TIMESHARE) != 0;
				}
			} else {
				*get_default = TRUE;
			}

			thread_unlock(thread);
			splx(s);
		}

		if (*count >= THREAD_EXTENDED_POLICY_COUNT) {
			thread_extended_policy_t        info;

			info = (thread_extended_policy_t)policy_info;
			info->timeshare = timeshare;
		}

		break;
	}

	case THREAD_TIME_CONSTRAINT_POLICY:
	{
		thread_time_constraint_policy_t         info;

		if (*count < THREAD_TIME_CONSTRAINT_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		info = (thread_time_constraint_policy_t)policy_info;

		if (!(*get_default)) {
			spl_t s = splsched();
			thread_lock(thread);

			if ((thread->sched_mode == TH_MODE_REALTIME) ||
			    (thread->saved_mode == TH_MODE_REALTIME)) {
				info->period = thread->realtime.period;
				info->computation = thread->realtime.computation;
				info->constraint = thread->realtime.constraint;
				info->preemptible = thread->realtime.preemptible;
			} else {
				*get_default = TRUE;
			}

			thread_unlock(thread);
			splx(s);
		}

		if (*get_default) {
			info->period = 0;
			info->computation = default_timeshare_computation;
			info->constraint = default_timeshare_constraint;
			info->preemptible = TRUE;
		}


		break;
	}

	case THREAD_PRECEDENCE_POLICY:
	{
		thread_precedence_policy_t              info;

		if (*count < THREAD_PRECEDENCE_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		info = (thread_precedence_policy_t)policy_info;

		if (!(*get_default)) {
			spl_t s = splsched();
			thread_lock(thread);

			info->importance = thread->importance;

			thread_unlock(thread);
			splx(s);
		} else {
			info->importance = 0;
		}

		break;
	}

	case THREAD_AFFINITY_POLICY:
	{
		thread_affinity_policy_t                info;

		if (!thread_affinity_is_supported()) {
			result = KERN_NOT_SUPPORTED;
			break;
		}
		if (*count < THREAD_AFFINITY_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		info = (thread_affinity_policy_t)policy_info;

		if (!(*get_default)) {
			info->affinity_tag = thread_affinity_get(thread);
		} else {
			info->affinity_tag = THREAD_AFFINITY_TAG_NULL;
		}

		break;
	}

	case THREAD_POLICY_STATE:
	{
		thread_policy_state_t           info;

		if (*count < THREAD_POLICY_STATE_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		                                 
		if (current_task()->sec_token.val[0] != 0) {
			result = KERN_PROTECTION_FAILURE;
			break;
		}

		info = (thread_policy_state_t)(void*)policy_info;

		if (!(*get_default)) {
			info->flags = 0;

			spl_t s = splsched();
			thread_lock(thread);

			info->flags |= (thread->static_param ? THREAD_POLICY_STATE_FLAG_STATIC_PARAM : 0);

			info->thps_requested_policy = *(uint64_t*)(void*)(&thread->requested_policy);
			info->thps_effective_policy = *(uint64_t*)(void*)(&thread->effective_policy);

			info->thps_user_promotions          = 0;
			info->thps_user_promotion_basepri   = thread->user_promotion_basepri;
			info->thps_ipc_overrides            = thread->kevent_overrides;

			proc_get_thread_policy_bitfield(thread, info);

			thread_unlock(thread);
			splx(s);
		} else {
			info->requested = 0;
			info->effective = 0;
			info->pending = 0;
		}

		break;
	}

	case THREAD_LATENCY_QOS_POLICY:
	{
		thread_latency_qos_policy_t info = (thread_latency_qos_policy_t) policy_info;
		thread_latency_qos_t plqos;

		if (*count < THREAD_LATENCY_QOS_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		if (*get_default) {
			plqos = 0;
		} else {
			plqos = proc_get_thread_policy_locked(thread, TASK_POLICY_ATTRIBUTE, TASK_POLICY_LATENCY_QOS, NULL);
		}

		info->thread_latency_qos_tier = qos_latency_policy_package(plqos);
	}
	break;

	case THREAD_THROUGHPUT_QOS_POLICY:
	{
		thread_throughput_qos_policy_t info = (thread_throughput_qos_policy_t) policy_info;
		thread_throughput_qos_t ptqos;

		if (*count < THREAD_THROUGHPUT_QOS_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		if (*get_default) {
			ptqos = 0;
		} else {
			ptqos = proc_get_thread_policy_locked(thread, TASK_POLICY_ATTRIBUTE, TASK_POLICY_THROUGH_QOS, NULL);
		}

		info->thread_throughput_qos_tier = qos_throughput_policy_package(ptqos);
	}
	break;

	case THREAD_QOS_POLICY:
	{
		thread_qos_policy_t info = (thread_qos_policy_t)policy_info;

		if (*count < THREAD_QOS_POLICY_COUNT) {
			result = KERN_INVALID_ARGUMENT;
			break;
		}

		if (!(*get_default)) {
			int relprio_value = 0;
			info->qos_tier = proc_get_thread_policy_locked(thread, TASK_POLICY_ATTRIBUTE,
			    TASK_POLICY_QOS_AND_RELPRIO, &relprio_value);

			info->tier_importance = -relprio_value;
		} else {
			info->qos_tier = THREAD_QOS_UNSPECIFIED;
			info->tier_importance = 0;
		}

		break;
	}

	default:
		result = KERN_INVALID_ARGUMENT;
		break;
	}

	thread_mtx_unlock(thread);

	return result;
}

void
thread_policy_create(thread_t thread)
{
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (IMPORTANCE_CODE(IMP_UPDATE, (IMP_UPDATE_TASK_CREATE | TASK_POLICY_THREAD))) | DBG_FUNC_START,
	    thread_tid(thread), theffective_0(thread),
	    theffective_1(thread), thread->base_pri, 0);

	                                        
	struct task_pend_token pend_token = {};

	thread_policy_update_internal_spinlocked(thread, true, &pend_token);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (IMPORTANCE_CODE(IMP_UPDATE, (IMP_UPDATE_TASK_CREATE | TASK_POLICY_THREAD))) | DBG_FUNC_END,
	    thread_tid(thread), theffective_0(thread),
	    theffective_1(thread), thread->base_pri, 0);
}

static void
thread_policy_update_spinlocked(thread_t thread, bool recompute_priority, task_pend_token_t pend_token)
{
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (IMPORTANCE_CODE(IMP_UPDATE, TASK_POLICY_THREAD) | DBG_FUNC_START),
	    thread_tid(thread), theffective_0(thread),
	    theffective_1(thread), thread->base_pri, 0);

	thread_policy_update_internal_spinlocked(thread, recompute_priority, pend_token);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (IMPORTANCE_CODE(IMP_UPDATE, TASK_POLICY_THREAD)) | DBG_FUNC_END,
	    thread_tid(thread), theffective_0(thread),
	    theffective_1(thread), thread->base_pri, 0);
}



  
                                                    
  
                                                           
                                                     
  
                                                   
  
                                                                                     
   
static void
thread_policy_update_internal_spinlocked(thread_t thread, bool recompute_priority,
    task_pend_token_t pend_token)
{
	  
           
                                                     
    

	struct thread_requested_policy requested = thread->requested_policy;
	struct task_effective_policy task_effective = thread->task->effective_policy;

	  
           
                                                                                  
           
                                                     
    

	struct thread_effective_policy next = {};

	next.thep_qos_ui_is_urgent = task_effective.tep_qos_ui_is_urgent;

	uint32_t next_qos = requested.thrp_qos;

	if (requested.thrp_qos != THREAD_QOS_UNSPECIFIED) {
		next_qos = MAX(requested.thrp_qos_override, next_qos);
		next_qos = MAX(requested.thrp_qos_promote, next_qos);
		next_qos = MAX(requested.thrp_qos_kevent_override, next_qos);
		next_qos = MAX(requested.thrp_qos_wlsvc_override, next_qos);
		next_qos = MAX(requested.thrp_qos_workq_override, next_qos);
	}

	if (task_effective.tep_darwinbg && task_effective.tep_adaptive_bg &&
	    requested.thrp_qos_promote > THREAD_QOS_BACKGROUND) {
		  
                                                                    
                                                                 
                                            
     
		next.thep_promote_above_task = true;
		next_qos = requested.thrp_qos_promote;
	}

	next.thep_qos = next_qos;

	                                                                                     
	if (task_effective.tep_qos_clamp != THREAD_QOS_UNSPECIFIED) {
		if (next.thep_qos != THREAD_QOS_UNSPECIFIED) {
			next.thep_qos = MIN(task_effective.tep_qos_clamp, next.thep_qos);
		} else {
			next.thep_qos = task_effective.tep_qos_clamp;
		}
	}

	  
                                                                           
                                                                                    
    
	next.thep_qos_promote = next.thep_qos;

	                                                                   
	                                                                                 
	if (task_effective.tep_qos_ceiling != THREAD_QOS_UNSPECIFIED &&
	    next.thep_qos != THREAD_QOS_UNSPECIFIED) {
		next.thep_qos = MIN(task_effective.tep_qos_ceiling, next.thep_qos);
	}

	  
                                                                               
                                                                        
                                                                                   
                                                                               
                                                                       
                            
    
	if ((requested.thrp_qos != THREAD_QOS_UNSPECIFIED) &&
	    (requested.thrp_qos == next.thep_qos) &&
	    (requested.thrp_qos_override == THREAD_QOS_UNSPECIFIED)) {
		next.thep_qos_relprio = requested.thrp_qos_relprio;
	} else {
		next.thep_qos_relprio = 0;
	}

	                         
	bool wants_darwinbg        = false;
	bool wants_all_sockets_bg  = false;                                             

	if (task_effective.tep_darwinbg && !next.thep_promote_above_task) {
		wants_darwinbg = true;
	}

	  
                                                                  
                                              
                                                      
                         
    
	if (requested.thrp_int_darwinbg || requested.thrp_ext_darwinbg) {
		wants_all_sockets_bg = wants_darwinbg = true;
	}

	if (requested.thrp_pidbind_bg) {
		wants_all_sockets_bg = wants_darwinbg = true;
	}

	if (next.thep_qos == THREAD_QOS_BACKGROUND ||
	    next.thep_qos == THREAD_QOS_MAINTENANCE) {
		wants_darwinbg = true;
	}

	                                         

	if (wants_darwinbg) {
		next.thep_darwinbg = 1;
	}

	if (next.thep_darwinbg || task_effective.tep_new_sockets_bg) {
		next.thep_new_sockets_bg = 1;
	}

	                                                      
	if (wants_all_sockets_bg) {
		next.thep_all_sockets_bg = 1;
	}

	                                                
	if (next.thep_darwinbg &&
	    (next.thep_qos > THREAD_QOS_BACKGROUND || next.thep_qos == THREAD_QOS_UNSPECIFIED)) {
		next.thep_qos = THREAD_QOS_BACKGROUND;
		next.thep_qos_relprio = 0;
	}

	                         

	int iopol = THROTTLE_LEVEL_TIER0;

	                                    
	if (next.thep_darwinbg) {
		iopol = MAX(iopol, task_effective.tep_bg_iotier);
	}

	if (!next.thep_promote_above_task) {
		iopol = MAX(iopol, task_effective.tep_io_tier);
	}

	                                                            
	iopol = MAX(iopol, thread_qos_policy_params.qos_iotier[next.thep_qos]);

	iopol = MAX(iopol, requested.thrp_int_iotier);
	iopol = MAX(iopol, requested.thrp_ext_iotier);

	next.thep_io_tier = iopol;

	  
                                                                        
                                                                             
                                       
    

	int next_qos_iotier = thread_qos_policy_params.qos_iotier[next.thep_qos];
	int req_qos_iotier = thread_qos_policy_params.qos_iotier[requested.thrp_qos];
	bool qos_io_override_active = (next_qos_iotier < req_qos_iotier);

	                                 
	if (requested.thrp_ext_iopassive ||
	    requested.thrp_int_iopassive ||
	    qos_io_override_active ||
	    task_effective.tep_io_passive) {
		next.thep_io_passive = 1;
	}

	                         
	uint32_t latency_qos = requested.thrp_latency_qos;

	if (!next.thep_promote_above_task) {
		latency_qos = MAX(latency_qos, task_effective.tep_latency_qos);
	}

	latency_qos = MAX(latency_qos, thread_qos_policy_params.qos_latency_qos[next.thep_qos]);

	next.thep_latency_qos = latency_qos;

	                              
	uint32_t through_qos = requested.thrp_through_qos;

	if (!next.thep_promote_above_task) {
		through_qos = MAX(through_qos, task_effective.tep_through_qos);
	}

	through_qos = MAX(through_qos, thread_qos_policy_params.qos_through_qos[next.thep_qos]);

	next.thep_through_qos = through_qos;

	if (task_effective.tep_terminated || requested.thrp_terminated) {
		                                                                         
		next.thep_terminated    = 1;
		next.thep_darwinbg      = 0;
		next.thep_io_tier       = THROTTLE_LEVEL_TIER0;
		next.thep_qos           = THREAD_QOS_UNSPECIFIED;
		next.thep_latency_qos   = LATENCY_QOS_TIER_UNSPECIFIED;
		next.thep_through_qos   = THROUGHPUT_QOS_TIER_UNSPECIFIED;
	}

	  
           
                                       
    

	struct thread_effective_policy prev = thread->effective_policy;

	thread_update_qos_cpu_time_locked(thread);

	                                                                            
	thread->effective_policy = next;

	  
           
                                                                  
    

	if (prev.thep_all_sockets_bg != next.thep_all_sockets_bg) {
		pend_token->tpt_update_sockets = 1;
	}

	                                                                      
	if (prev.thep_io_tier != next.thep_io_tier) {
		pend_token->tpt_update_throttle = 1;
	}

	  
                                                                 
                                   
    
	if (prev.thep_qos != next.thep_qos ||
	    prev.thep_darwinbg != next.thep_darwinbg) {
		pend_token->tpt_update_thread_sfi = 1;
	}

	integer_t old_base_pri = thread->base_pri;

	  
           
                                                                  
    

	                                                                        
	if (prev.thep_qos != next.thep_qos ||
	    prev.thep_qos_relprio != next.thep_qos_relprio ||
	    prev.thep_qos_ui_is_urgent != next.thep_qos_ui_is_urgent ||
	    prev.thep_promote_above_task != next.thep_promote_above_task ||
	    prev.thep_terminated != next.thep_terminated ||
	    pend_token->tpt_force_recompute_pri == 1 ||
	    recompute_priority) {
		thread_recompute_priority(thread);
	}

	  
                                                                                 
    
	if (pend_token->tpt_update_turnstile &&
	    ((old_base_pri == thread->base_pri) ||
	    !thread_get_waiting_turnstile(thread))) {
		  
                                                   
                                                    
                                
     
		pend_token->tpt_update_turnstile = 0;
	}
}


  
                                                                     
                                                                 
                                  
                                                            
                                    
   
void
proc_set_thread_policy_with_tid(task_t     task,
    uint64_t   tid,
    int        category,
    int        flavor,
    int        value)
{
	                                                    
	thread_t thread = task_findtid(task, tid);

	if (thread == THREAD_NULL) {
		return;
	}

	proc_set_thread_policy(thread, category, flavor, value);

	thread_deallocate(thread);
}

  
                                                  
                                                                        
                                  
   
void
proc_set_thread_policy(thread_t   thread,
    int        category,
    int        flavor,
    int        value)
{
	struct task_pend_token pend_token = {};

	thread_mtx_lock(thread);

	proc_set_thread_policy_locked(thread, category, flavor, value, 0, &pend_token);

	thread_mtx_unlock(thread);

	thread_policy_update_complete_unlocked(thread, &pend_token);
}

  
                                                                 
                                                                            
                                                         
                                                                                     
  
                                         
  
                                                                           
                                      
   
void
thread_policy_update_complete_unlocked(thread_t thread, task_pend_token_t pend_token)
{
#ifdef MACH_BSD
	if (pend_token->tpt_update_sockets) {
		proc_apply_task_networkbg(thread->task->bsd_info, thread);
	}
#endif               

	if (pend_token->tpt_update_throttle) {
		rethrottle_thread(thread->uthread);
	}

	if (pend_token->tpt_update_thread_sfi) {
		sfi_reevaluate(thread);
	}

	if (pend_token->tpt_update_turnstile) {
		turnstile_update_thread_priority_chain(thread);
	}
}

  
                               
                             
   
static void
proc_set_thread_policy_locked(thread_t          thread,
    int               category,
    int               flavor,
    int               value,
    int               value2,
    task_pend_token_t pend_token)
{
	spl_t s = splsched();
	thread_lock(thread);

	proc_set_thread_policy_spinlocked(thread, category, flavor, value, value2, pend_token);

	thread_unlock(thread);
	splx(s);
}

  
                               
                          
   
static void
proc_set_thread_policy_spinlocked(thread_t          thread,
    int               category,
    int               flavor,
    int               value,
    int               value2,
    task_pend_token_t pend_token)
{
	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (IMPORTANCE_CODE(flavor, (category | TASK_POLICY_THREAD))) | DBG_FUNC_START,
	    thread_tid(thread), threquested_0(thread),
	    threquested_1(thread), value, 0);

	thread_set_requested_policy_spinlocked(thread, category, flavor, value, value2, pend_token);

	thread_policy_update_spinlocked(thread, false, pend_token);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (IMPORTANCE_CODE(flavor, (category | TASK_POLICY_THREAD))) | DBG_FUNC_END,
	    thread_tid(thread), threquested_0(thread),
	    threquested_1(thread), tpending(pend_token), 0);
}

  
                                                                     
   
static void
thread_set_requested_policy_spinlocked(thread_t     thread,
    int               category,
    int               flavor,
    int               value,
    int               value2,
    task_pend_token_t pend_token)
{
	int tier, passive;

	struct thread_requested_policy requested = thread->requested_policy;

	switch (flavor) {
	                                                      

	case TASK_POLICY_DARWIN_BG:
		if (category == TASK_POLICY_EXTERNAL) {
			requested.thrp_ext_darwinbg = value;
		} else {
			requested.thrp_int_darwinbg = value;
		}
		break;

	case TASK_POLICY_IOPOL:
		proc_iopol_to_tier(value, &tier, &passive);
		if (category == TASK_POLICY_EXTERNAL) {
			requested.thrp_ext_iotier  = tier;
			requested.thrp_ext_iopassive = passive;
		} else {
			requested.thrp_int_iotier  = tier;
			requested.thrp_int_iopassive = passive;
		}
		break;

	case TASK_POLICY_IO:
		if (category == TASK_POLICY_EXTERNAL) {
			requested.thrp_ext_iotier = value;
		} else {
			requested.thrp_int_iotier = value;
		}
		break;

	case TASK_POLICY_PASSIVE_IO:
		if (category == TASK_POLICY_EXTERNAL) {
			requested.thrp_ext_iopassive = value;
		} else {
			requested.thrp_int_iopassive = value;
		}
		break;

	                                      

	case TASK_POLICY_PIDBIND_BG:
		assert(category == TASK_POLICY_ATTRIBUTE);
		requested.thrp_pidbind_bg = value;
		break;

	case TASK_POLICY_LATENCY_QOS:
		assert(category == TASK_POLICY_ATTRIBUTE);
		requested.thrp_latency_qos = value;
		break;

	case TASK_POLICY_THROUGH_QOS:
		assert(category == TASK_POLICY_ATTRIBUTE);
		requested.thrp_through_qos = value;
		break;

	case TASK_POLICY_QOS_OVERRIDE:
		assert(category == TASK_POLICY_ATTRIBUTE);
		requested.thrp_qos_override = value;
		pend_token->tpt_update_turnstile = 1;
		break;

	case TASK_POLICY_QOS_AND_RELPRIO:
		assert(category == TASK_POLICY_ATTRIBUTE);
		requested.thrp_qos = value;
		requested.thrp_qos_relprio = value2;
		pend_token->tpt_update_turnstile = 1;
		DTRACE_BOOST3(qos_set, uint64_t, thread->thread_id, int, requested.thrp_qos, int, requested.thrp_qos_relprio);
		break;

	case TASK_POLICY_QOS_WORKQ_OVERRIDE:
		assert(category == TASK_POLICY_ATTRIBUTE);
		requested.thrp_qos_workq_override = value;
		pend_token->tpt_update_turnstile = 1;
		break;

	case TASK_POLICY_QOS_PROMOTE:
		assert(category == TASK_POLICY_ATTRIBUTE);
		requested.thrp_qos_promote = value;
		break;

	case TASK_POLICY_QOS_KEVENT_OVERRIDE:
		assert(category == TASK_POLICY_ATTRIBUTE);
		requested.thrp_qos_kevent_override = value;
		pend_token->tpt_update_turnstile = 1;
		break;

	case TASK_POLICY_QOS_SERVICER_OVERRIDE:
		assert(category == TASK_POLICY_ATTRIBUTE);
		requested.thrp_qos_wlsvc_override = value;
		pend_token->tpt_update_turnstile = 1;
		break;

	case TASK_POLICY_TERMINATED:
		assert(category == TASK_POLICY_ATTRIBUTE);
		requested.thrp_terminated = value;
		break;

	default:
		panic("unknown task policy: %d %d %d", category, flavor, value);
		break;
	}

	thread->requested_policy = requested;
}

  
                                                        
                                  
   
int
proc_get_thread_policy(thread_t   thread,
    int        category,
    int        flavor)
{
	int value = 0;
	thread_mtx_lock(thread);
	value = proc_get_thread_policy_locked(thread, category, flavor, NULL);
	thread_mtx_unlock(thread);
	return value;
}

static int
proc_get_thread_policy_locked(thread_t   thread,
    int        category,
    int        flavor,
    int*       value2)
{
	int value = 0;

	spl_t s = splsched();
	thread_lock(thread);

	value = thread_get_requested_policy_spinlocked(thread, category, flavor, value2);

	thread_unlock(thread);
	splx(s);

	return value;
}

  
                                                        
   
static int
thread_get_requested_policy_spinlocked(thread_t thread,
    int      category,
    int      flavor,
    int*     value2)
{
	int value = 0;

	struct thread_requested_policy requested = thread->requested_policy;

	switch (flavor) {
	case TASK_POLICY_DARWIN_BG:
		if (category == TASK_POLICY_EXTERNAL) {
			value = requested.thrp_ext_darwinbg;
		} else {
			value = requested.thrp_int_darwinbg;
		}
		break;
	case TASK_POLICY_IOPOL:
		if (category == TASK_POLICY_EXTERNAL) {
			value = proc_tier_to_iopol(requested.thrp_ext_iotier,
			    requested.thrp_ext_iopassive);
		} else {
			value = proc_tier_to_iopol(requested.thrp_int_iotier,
			    requested.thrp_int_iopassive);
		}
		break;
	case TASK_POLICY_IO:
		if (category == TASK_POLICY_EXTERNAL) {
			value = requested.thrp_ext_iotier;
		} else {
			value = requested.thrp_int_iotier;
		}
		break;
	case TASK_POLICY_PASSIVE_IO:
		if (category == TASK_POLICY_EXTERNAL) {
			value = requested.thrp_ext_iopassive;
		} else {
			value = requested.thrp_int_iopassive;
		}
		break;
	case TASK_POLICY_QOS:
		assert(category == TASK_POLICY_ATTRIBUTE);
		value = requested.thrp_qos;
		break;
	case TASK_POLICY_QOS_OVERRIDE:
		assert(category == TASK_POLICY_ATTRIBUTE);
		value = requested.thrp_qos_override;
		break;
	case TASK_POLICY_LATENCY_QOS:
		assert(category == TASK_POLICY_ATTRIBUTE);
		value = requested.thrp_latency_qos;
		break;
	case TASK_POLICY_THROUGH_QOS:
		assert(category == TASK_POLICY_ATTRIBUTE);
		value = requested.thrp_through_qos;
		break;
	case TASK_POLICY_QOS_WORKQ_OVERRIDE:
		assert(category == TASK_POLICY_ATTRIBUTE);
		value = requested.thrp_qos_workq_override;
		break;
	case TASK_POLICY_QOS_AND_RELPRIO:
		assert(category == TASK_POLICY_ATTRIBUTE);
		assert(value2 != NULL);
		value = requested.thrp_qos;
		*value2 = requested.thrp_qos_relprio;
		break;
	case TASK_POLICY_QOS_PROMOTE:
		assert(category == TASK_POLICY_ATTRIBUTE);
		value = requested.thrp_qos_promote;
		break;
	case TASK_POLICY_QOS_KEVENT_OVERRIDE:
		assert(category == TASK_POLICY_ATTRIBUTE);
		value = requested.thrp_qos_kevent_override;
		break;
	case TASK_POLICY_QOS_SERVICER_OVERRIDE:
		assert(category == TASK_POLICY_ATTRIBUTE);
		value = requested.thrp_qos_wlsvc_override;
		break;
	case TASK_POLICY_TERMINATED:
		assert(category == TASK_POLICY_ATTRIBUTE);
		value = requested.thrp_terminated;
		break;

	default:
		panic("unknown policy_flavor %d", flavor);
		break;
	}

	return value;
}

  
                                                                                                
  
                                                             
                                                                                        
                                                                          
                                           
  
                                                                                                         
                                                                                       
                                                                     
   
int
proc_get_effective_thread_policy(thread_t thread,
    int      flavor)
{
	int value = 0;

	switch (flavor) {
	case TASK_POLICY_DARWIN_BG:
		  
                                                         
                                                  
                                                 
                                                     
     

		value = thread->effective_policy.thep_darwinbg ? 1 : 0;
		break;
	case TASK_POLICY_IO:
		  
                                                                                         
                                    
     
		value = thread->effective_policy.thep_io_tier;
		if (thread->iotier_override != THROTTLE_LEVEL_NONE) {
			value = MIN(value, thread->iotier_override);
		}
		break;
	case TASK_POLICY_PASSIVE_IO:
		  
                                                                                  
                                                                          
                                                  
    
                                                                      
                                                                              
                                        
     
		value = thread->effective_policy.thep_io_passive ? 1 : 0;
		if (thread->iotier_override != THROTTLE_LEVEL_NONE &&
		    thread->iotier_override < thread->effective_policy.thep_io_tier) {
			value = 1;
		}
		break;
	case TASK_POLICY_ALL_SOCKETS_BG:
		  
                                                           
                                          
                                                     
                                                                                  
                                                 
     
		value = (thread->effective_policy.thep_all_sockets_bg ||
		    thread->task->effective_policy.tep_all_sockets_bg) ? 1 : 0;
		break;
	case TASK_POLICY_NEW_SOCKETS_BG:
		  
                                                                                    
                                                     
     
		value = thread->effective_policy.thep_new_sockets_bg ? 1 : 0;
		break;
	case TASK_POLICY_LATENCY_QOS:
		  
                                                                        
                                     
     
		value = thread->effective_policy.thep_latency_qos;
		break;
	case TASK_POLICY_THROUGH_QOS:
		  
                                                                     
                                             
    
                                        
     
		value = thread->effective_policy.thep_through_qos;
		break;
	case TASK_POLICY_QOS:
		  
                                                                      
    
                              
     
		value = thread->effective_policy.thep_qos;
		break;
	default:
		panic("unknown thread policy flavor %d", flavor);
		break;
	}

	return value;
}


  
                                                             
                                                                 
   
static void
proc_get_thread_policy_bitfield(thread_t thread, thread_policy_state_t info)
{
	uint64_t bits = 0;
	struct thread_requested_policy requested = thread->requested_policy;

	bits |= (requested.thrp_int_darwinbg    ? POLICY_REQ_INT_DARWIN_BG  : 0);
	bits |= (requested.thrp_ext_darwinbg    ? POLICY_REQ_EXT_DARWIN_BG  : 0);
	bits |= (requested.thrp_int_iotier      ? (((uint64_t)requested.thrp_int_iotier) << POLICY_REQ_INT_IO_TIER_SHIFT) : 0);
	bits |= (requested.thrp_ext_iotier      ? (((uint64_t)requested.thrp_ext_iotier) << POLICY_REQ_EXT_IO_TIER_SHIFT) : 0);
	bits |= (requested.thrp_int_iopassive   ? POLICY_REQ_INT_PASSIVE_IO : 0);
	bits |= (requested.thrp_ext_iopassive   ? POLICY_REQ_EXT_PASSIVE_IO : 0);

	bits |= (requested.thrp_qos             ? (((uint64_t)requested.thrp_qos) << POLICY_REQ_TH_QOS_SHIFT) : 0);
	bits |= (requested.thrp_qos_override    ? (((uint64_t)requested.thrp_qos_override) << POLICY_REQ_TH_QOS_OVER_SHIFT)   : 0);

	bits |= (requested.thrp_pidbind_bg      ? POLICY_REQ_PIDBIND_BG     : 0);

	bits |= (requested.thrp_latency_qos     ? (((uint64_t)requested.thrp_latency_qos) << POLICY_REQ_BASE_LATENCY_QOS_SHIFT) : 0);
	bits |= (requested.thrp_through_qos     ? (((uint64_t)requested.thrp_through_qos) << POLICY_REQ_BASE_THROUGH_QOS_SHIFT) : 0);

	info->requested = (integer_t) bits;
	bits = 0;

	struct thread_effective_policy effective = thread->effective_policy;

	bits |= (effective.thep_darwinbg        ? POLICY_EFF_DARWIN_BG      : 0);

	bits |= (effective.thep_io_tier         ? (((uint64_t)effective.thep_io_tier) << POLICY_EFF_IO_TIER_SHIFT) : 0);
	bits |= (effective.thep_io_passive      ? POLICY_EFF_IO_PASSIVE     : 0);
	bits |= (effective.thep_all_sockets_bg  ? POLICY_EFF_ALL_SOCKETS_BG : 0);
	bits |= (effective.thep_new_sockets_bg  ? POLICY_EFF_NEW_SOCKETS_BG : 0);

	bits |= (effective.thep_qos             ? (((uint64_t)effective.thep_qos) << POLICY_EFF_TH_QOS_SHIFT) : 0);

	bits |= (effective.thep_latency_qos     ? (((uint64_t)effective.thep_latency_qos) << POLICY_EFF_LATENCY_QOS_SHIFT) : 0);
	bits |= (effective.thep_through_qos     ? (((uint64_t)effective.thep_through_qos) << POLICY_EFF_THROUGH_QOS_SHIFT) : 0);

	info->effective = (integer_t)bits;
	bits = 0;

	info->pending = 0;
}

  
                                                      
                                                                     
                                                                                 
  
                                                      
                                                         
                                                           
  
   

uintptr_t
threquested_0(thread_t thread)
{
	static_assert(sizeof(struct thread_requested_policy) == sizeof(uint64_t), "size invariant violated");

	uintptr_t* raw = (uintptr_t*)(void*)&thread->requested_policy;

	return raw[0];
}

uintptr_t
threquested_1(thread_t thread)
{
#if defined __LP64__
	return *(uintptr_t*)&thread->task->requested_policy;
#else
	uintptr_t* raw = (uintptr_t*)(void*)&thread->requested_policy;
	return raw[1];
#endif
}

uintptr_t
theffective_0(thread_t thread)
{
	static_assert(sizeof(struct thread_effective_policy) == sizeof(uint64_t), "size invariant violated");

	uintptr_t* raw = (uintptr_t*)(void*)&thread->effective_policy;
	return raw[0];
}

uintptr_t
theffective_1(thread_t thread)
{
#if defined __LP64__
	return *(uintptr_t*)&thread->task->effective_policy;
#else
	uintptr_t* raw = (uintptr_t*)(void*)&thread->effective_policy;
	return raw[1];
#endif
}


  
                                                          
                                                           
                                                    
                                    
  
                                                         
                                            
                                                         
  
                                                         
                                                              
                                                         
   

void
set_thread_iotier_override(thread_t thread, int policy)
{
	int current_override;

	                                                            
	do {
		current_override = thread->iotier_override;

		if (current_override != THROTTLE_LEVEL_NONE) {
			policy = MIN(current_override, policy);
		}

		if (current_override == policy) {
			                         
			return;
		}
	} while (!OSCompareAndSwap(current_override, policy, &thread->iotier_override));

	  
                                                
                                               
                
    
	rethrottle_thread(thread->uthread);
}

  
                                                                                         
                                                                                       
                                                                                          
                                                                                       
                                                                                      
                                                                                   
                            
   

  
                                                                                           
                                                                                  
                                                                       
                                                                    
   
static void
canonicalize_resource_and_type(user_addr_t *resource, int *resource_type)
{
	if (qos_override_mode == QOS_OVERRIDE_MODE_OVERHANG_PEAK || qos_override_mode == QOS_OVERRIDE_MODE_IGNORE_OVERRIDE) {
		                                                 
		*resource = USER_ADDR_NULL;
		*resource_type = THREAD_QOS_OVERRIDE_TYPE_UNKNOWN;
	} else if (qos_override_mode == QOS_OVERRIDE_MODE_FINE_GRAINED_OVERRIDE) {
		                  
	} else if (qos_override_mode == QOS_OVERRIDE_MODE_FINE_GRAINED_OVERRIDE_BUT_SINGLE_MUTEX_OVERRIDE) {
		                                                                       
		if (*resource_type == THREAD_QOS_OVERRIDE_TYPE_PTHREAD_MUTEX) {
			*resource = USER_ADDR_NULL;
		}
	}
}

                                                                                               
static struct thread_qos_override *
find_qos_override(thread_t thread,
    user_addr_t resource,
    int resource_type)
{
	struct thread_qos_override *override;

	override = thread->overrides;
	while (override) {
		if (override->override_resource == resource &&
		    override->override_resource_type == resource_type) {
			return override;
		}

		override = override->override_next;
	}

	return NULL;
}

static void
find_and_decrement_qos_override(thread_t       thread,
    user_addr_t    resource,
    int            resource_type,
    boolean_t      reset,
    struct thread_qos_override **free_override_list)
{
	struct thread_qos_override *override, *override_prev;

	override_prev = NULL;
	override = thread->overrides;
	while (override) {
		struct thread_qos_override *override_next = override->override_next;

		if ((THREAD_QOS_OVERRIDE_RESOURCE_WILDCARD == resource || override->override_resource == resource) &&
		    (THREAD_QOS_OVERRIDE_TYPE_WILDCARD == resource_type || override->override_resource_type == resource_type)) {
			if (reset) {
				override->override_contended_resource_count = 0;
			} else {
				override->override_contended_resource_count--;
			}

			if (override->override_contended_resource_count == 0) {
				if (override_prev == NULL) {
					thread->overrides = override_next;
				} else {
					override_prev->override_next = override_next;
				}

				                                      
				override->override_next = *free_override_list;
				*free_override_list = override;
			} else {
				override_prev = override;
			}

			if (THREAD_QOS_OVERRIDE_RESOURCE_WILDCARD != resource) {
				return;
			}
		} else {
			override_prev = override;
		}

		override = override_next;
	}
}

                                                                                               
static int
calculate_requested_qos_override(thread_t thread)
{
	if (qos_override_mode == QOS_OVERRIDE_MODE_IGNORE_OVERRIDE) {
		return THREAD_QOS_UNSPECIFIED;
	}

	                                                  
	struct thread_qos_override *override;
	int qos_override = THREAD_QOS_UNSPECIFIED;

	override = thread->overrides;
	while (override) {
		qos_override = MAX(qos_override, override->override_qos);
		override = override->override_next;
	}

	return qos_override;
}

  
           
                 
                                            
   
static int
proc_thread_qos_add_override_internal(thread_t         thread,
    int              override_qos,
    boolean_t        first_override_for_resource,
    user_addr_t      resource,
    int              resource_type)
{
	struct task_pend_token pend_token = {};
	int rc = 0;

	thread_mtx_lock(thread);

	KERNEL_DEBUG_CONSTANT((IMPORTANCE_CODE(IMP_USYNCH_QOS_OVERRIDE, IMP_USYNCH_ADD_OVERRIDE)) | DBG_FUNC_START,
	    thread_tid(thread), override_qos, first_override_for_resource ? 1 : 0, 0, 0);

	DTRACE_BOOST5(qos_add_override_pre, uint64_t, thread_tid(thread),
	    uint64_t, thread->requested_policy.thrp_qos,
	    uint64_t, thread->effective_policy.thep_qos,
	    int, override_qos, boolean_t, first_override_for_resource);

	struct thread_qos_override *override;
	struct thread_qos_override *override_new = NULL;
	int new_qos_override, prev_qos_override;
	int new_effective_qos;

	canonicalize_resource_and_type(&resource, &resource_type);

	override = find_qos_override(thread, resource, resource_type);
	if (first_override_for_resource && !override) {
		                                                             
                                                               
     
		thread_mtx_unlock(thread);
		override_new = zalloc(thread_qos_override_zone);
		thread_mtx_lock(thread);
		override = find_qos_override(thread, resource, resource_type);
	}
	if (first_override_for_resource && override) {
		                                                                      
		override->override_contended_resource_count++;
	} else if (!override && override_new) {
		override = override_new;
		override_new = NULL;
		override->override_next = thread->overrides;
		                                                
		override->override_contended_resource_count = 1;
		override->override_resource = resource;
		override->override_resource_type = (int16_t)resource_type;
		override->override_qos = THREAD_QOS_UNSPECIFIED;
		thread->overrides = override;
	}

	if (override) {
		if (override->override_qos == THREAD_QOS_UNSPECIFIED) {
			override->override_qos = (int16_t)override_qos;
		} else {
			override->override_qos = MAX(override->override_qos, (int16_t)override_qos);
		}
	}

	                                                                       
                      
    
	new_qos_override = calculate_requested_qos_override(thread);

	prev_qos_override = proc_get_thread_policy_locked(thread,
	    TASK_POLICY_ATTRIBUTE, TASK_POLICY_QOS_OVERRIDE, NULL);

	if (new_qos_override != prev_qos_override) {
		proc_set_thread_policy_locked(thread, TASK_POLICY_ATTRIBUTE,
		    TASK_POLICY_QOS_OVERRIDE,
		    new_qos_override, 0, &pend_token);
	}

	new_effective_qos = proc_get_effective_thread_policy(thread, TASK_POLICY_QOS);

	thread_mtx_unlock(thread);

	thread_policy_update_complete_unlocked(thread, &pend_token);

	if (override_new) {
		zfree(thread_qos_override_zone, override_new);
	}

	DTRACE_BOOST4(qos_add_override_post, int, prev_qos_override,
	    int, new_qos_override, int, new_effective_qos, int, rc);

	KERNEL_DEBUG_CONSTANT((IMPORTANCE_CODE(IMP_USYNCH_QOS_OVERRIDE, IMP_USYNCH_ADD_OVERRIDE)) | DBG_FUNC_END,
	    new_qos_override, resource, resource_type, 0, 0);

	return rc;
}

int
proc_thread_qos_add_override(task_t           task,
    thread_t         thread,
    uint64_t         tid,
    int              override_qos,
    boolean_t        first_override_for_resource,
    user_addr_t      resource,
    int              resource_type)
{
	boolean_t has_thread_reference = FALSE;
	int rc = 0;

	if (thread == THREAD_NULL) {
		thread = task_findtid(task, tid);
		                               

		if (thread == THREAD_NULL) {
			KERNEL_DEBUG_CONSTANT((IMPORTANCE_CODE(IMP_USYNCH_QOS_OVERRIDE, IMP_USYNCH_ADD_OVERRIDE)) | DBG_FUNC_NONE,
			    tid, 0, 0xdead, 0, 0);
			return ESRCH;
		}
		has_thread_reference = TRUE;
	} else {
		assert(thread->task == task);
	}
	rc = proc_thread_qos_add_override_internal(thread, override_qos,
	    first_override_for_resource, resource, resource_type);
	if (has_thread_reference) {
		thread_deallocate(thread);
	}

	return rc;
}

static void
proc_thread_qos_remove_override_internal(thread_t       thread,
    user_addr_t    resource,
    int            resource_type,
    boolean_t      reset)
{
	struct task_pend_token pend_token = {};

	struct thread_qos_override *deferred_free_override_list = NULL;
	int new_qos_override, prev_qos_override, new_effective_qos;

	thread_mtx_lock(thread);

	canonicalize_resource_and_type(&resource, &resource_type);

	find_and_decrement_qos_override(thread, resource, resource_type, reset, &deferred_free_override_list);

	KERNEL_DEBUG_CONSTANT((IMPORTANCE_CODE(IMP_USYNCH_QOS_OVERRIDE, IMP_USYNCH_REMOVE_OVERRIDE)) | DBG_FUNC_START,
	    thread_tid(thread), resource, reset, 0, 0);

	DTRACE_BOOST3(qos_remove_override_pre, uint64_t, thread_tid(thread),
	    uint64_t, thread->requested_policy.thrp_qos,
	    uint64_t, thread->effective_policy.thep_qos);

	                                                                                             
	new_qos_override = calculate_requested_qos_override(thread);

	spl_t s = splsched();
	thread_lock(thread);

	  
                                                                                                   
                                                                                                           
                                                                                 
    
	prev_qos_override = thread_get_requested_policy_spinlocked(thread, TASK_POLICY_ATTRIBUTE, TASK_POLICY_QOS_OVERRIDE, NULL);

	if (new_qos_override != prev_qos_override) {
		proc_set_thread_policy_spinlocked(thread, TASK_POLICY_ATTRIBUTE, TASK_POLICY_QOS_OVERRIDE, new_qos_override, 0, &pend_token);
	}

	new_effective_qos = proc_get_effective_thread_policy(thread, TASK_POLICY_QOS);

	thread_unlock(thread);
	splx(s);

	thread_mtx_unlock(thread);

	thread_policy_update_complete_unlocked(thread, &pend_token);

	while (deferred_free_override_list) {
		struct thread_qos_override *override_next = deferred_free_override_list->override_next;

		zfree(thread_qos_override_zone, deferred_free_override_list);
		deferred_free_override_list = override_next;
	}

	DTRACE_BOOST3(qos_remove_override_post, int, prev_qos_override,
	    int, new_qos_override, int, new_effective_qos);

	KERNEL_DEBUG_CONSTANT((IMPORTANCE_CODE(IMP_USYNCH_QOS_OVERRIDE, IMP_USYNCH_REMOVE_OVERRIDE)) | DBG_FUNC_END,
	    thread_tid(thread), 0, 0, 0, 0);
}

int
proc_thread_qos_remove_override(task_t      task,
    thread_t    thread,
    uint64_t    tid,
    user_addr_t resource,
    int         resource_type)
{
	boolean_t has_thread_reference = FALSE;

	if (thread == THREAD_NULL) {
		thread = task_findtid(task, tid);
		                               

		if (thread == THREAD_NULL) {
			KERNEL_DEBUG_CONSTANT((IMPORTANCE_CODE(IMP_USYNCH_QOS_OVERRIDE, IMP_USYNCH_REMOVE_OVERRIDE)) | DBG_FUNC_NONE,
			    tid, 0, 0xdead, 0, 0);
			return ESRCH;
		}
		has_thread_reference = TRUE;
	} else {
		assert(task == thread->task);
	}

	proc_thread_qos_remove_override_internal(thread, resource, resource_type, FALSE);

	if (has_thread_reference) {
		thread_deallocate(thread);
	}

	return 0;
}

                                          
void
proc_thread_qos_deallocate(thread_t thread)
{
	                                                  
	assert(thread->kevent_overrides == 0);
	assert(thread->requested_policy.thrp_qos_kevent_override == THREAD_QOS_UNSPECIFIED);
	assert(thread->requested_policy.thrp_qos_wlsvc_override == THREAD_QOS_UNSPECIFIED);

	  
                                             
    
	struct thread_qos_override *override;

	thread_mtx_lock(thread);
	override = thread->overrides;
	thread->overrides = NULL;
	thread->requested_policy.thrp_qos_override = THREAD_QOS_UNSPECIFIED;
	                                                                                           
	thread_mtx_unlock(thread);

	while (override) {
		struct thread_qos_override *override_next = override->override_next;

		zfree(thread_qos_override_zone, override);
		override = override_next;
	}
}

  
                                     
   
void
task_set_main_thread_qos(task_t task, thread_t thread)
{
	struct task_pend_token pend_token = {};

	assert(thread->task == task);

	thread_mtx_lock(thread);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (IMPORTANCE_CODE(IMP_MAIN_THREAD_QOS, 0)) | DBG_FUNC_START,
	    thread_tid(thread), threquested_0(thread), threquested_1(thread),
	    thread->requested_policy.thrp_qos, 0);

	thread_qos_t primordial_qos = task_compute_main_thread_qos(task);

	proc_set_thread_policy_locked(thread, TASK_POLICY_ATTRIBUTE, TASK_POLICY_QOS_AND_RELPRIO,
	    primordial_qos, 0, &pend_token);

	thread_mtx_unlock(thread);

	thread_policy_update_complete_unlocked(thread, &pend_token);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (IMPORTANCE_CODE(IMP_MAIN_THREAD_QOS, 0)) | DBG_FUNC_END,
	    thread_tid(thread), threquested_0(thread), threquested_1(thread),
	    primordial_qos, 0);
}

  
                       
  
                                                              
                                                           
   
thread_qos_t
task_get_default_manager_qos(task_t task)
{
	thread_qos_t primordial_qos = task_compute_main_thread_qos(task);

	if (primordial_qos == THREAD_QOS_LEGACY) {
		primordial_qos = THREAD_QOS_USER_INITIATED;
	}

	return primordial_qos;
}

  
                                                      
                
  
                                  
   
boolean_t
thread_recompute_kernel_promotion_locked(thread_t thread)
{
	boolean_t needs_update = FALSE;
	uint8_t kern_promotion_schedpri = (uint8_t)thread_get_inheritor_turnstile_sched_priority(thread);

	  
                                                                       
                                                                                       
                                                                                              
                                              
   
                                                                                        
                                    
                                                                           
                                                                                    
    
	assert(kern_promotion_schedpri <= MAXPRI_PROMOTE);

	if (kern_promotion_schedpri != thread->kern_promotion_schedpri) {
		KDBG(MACHDBG_CODE(
			    DBG_MACH_SCHED, MACH_TURNSTILE_KERNEL_CHANGE) | DBG_FUNC_NONE,
		    thread_tid(thread),
		    kern_promotion_schedpri,
		    thread->kern_promotion_schedpri);

		needs_update = TRUE;
		thread->kern_promotion_schedpri = kern_promotion_schedpri;
		thread_recompute_sched_pri(thread, SETPRI_DEFAULT);
	}

	return needs_update;
}

  
                                                    
                
  
                                                     
                    
   
boolean_t
thread_recompute_user_promotion_locked(thread_t thread)
{
	boolean_t needs_update = FALSE;
	struct task_pend_token pend_token = {};
	uint8_t user_promotion_basepri = MIN((uint8_t)thread_get_inheritor_turnstile_base_priority(thread), MAXPRI_USER);
	int old_base_pri = thread->base_pri;
	thread_qos_t qos_promotion;

	                                         
	if (thread->user_promotion_basepri == user_promotion_basepri) {
		return needs_update;
	} else {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (TURNSTILE_CODE(TURNSTILE_PRIORITY_OPERATIONS, (THREAD_USER_PROMOTION_CHANGE))) | DBG_FUNC_NONE,
		    thread_tid(thread),
		    user_promotion_basepri,
		    thread->user_promotion_basepri,
		    0, 0);
		KDBG(MACHDBG_CODE(
			    DBG_MACH_SCHED, MACH_TURNSTILE_USER_CHANGE) | DBG_FUNC_NONE,
		    thread_tid(thread),
		    user_promotion_basepri,
		    thread->user_promotion_basepri);
	}

	                                        
	thread->user_promotion_basepri = user_promotion_basepri;
	pend_token.tpt_force_recompute_pri = 1;

	if (user_promotion_basepri <= MAXPRI_THROTTLE) {
		qos_promotion = THREAD_QOS_UNSPECIFIED;
	} else {
		qos_promotion = thread_user_promotion_qos_for_pri(user_promotion_basepri);
	}

	proc_set_thread_policy_spinlocked(thread, TASK_POLICY_ATTRIBUTE,
	    TASK_POLICY_QOS_PROMOTE, qos_promotion, 0, &pend_token);

	if (thread_get_waiting_turnstile(thread) &&
	    thread->base_pri != old_base_pri) {
		needs_update = TRUE;
	}

	thread_unlock(thread);

	thread_policy_update_complete_unlocked(thread, &pend_token);

	thread_lock(thread);

	return needs_update;
}

  
                                                                              
                                                         
   
thread_qos_t
thread_user_promotion_qos_for_pri(int priority)
{
	thread_qos_t qos;
	for (qos = THREAD_QOS_USER_INTERACTIVE; qos > THREAD_QOS_MAINTENANCE; qos--) {
		if (thread_qos_policy_params.qos_pri[qos] <= priority) {
			return qos;
		}
	}
	return THREAD_QOS_MAINTENANCE;
}

  
                                       
                                
  
                                                       
                                                                                 
  
                                            
                                                                                   
                                                                         
   
static void
thread_kevent_override(thread_t    thread,
    uint32_t    qos_override,
    boolean_t   is_new_override)
{
	struct task_pend_token pend_token = {};
	boolean_t needs_update;

	spl_t s = splsched();
	thread_lock(thread);

	uint32_t old_override = thread->requested_policy.thrp_qos_kevent_override;

	assert(qos_override > THREAD_QOS_UNSPECIFIED);
	assert(qos_override < THREAD_QOS_LAST);

	if (is_new_override) {
		if (thread->kevent_overrides++ == 0) {
			                                                    
			assert(old_override == THREAD_QOS_UNSPECIFIED);
		} else {
			                                                                 
			assert(old_override > THREAD_QOS_UNSPECIFIED);
		}
	} else {
		                                                                           
		assert(thread->kevent_overrides > 0);
		assert(old_override > THREAD_QOS_UNSPECIFIED);
	}

	  
                                                                      
                                                  
    
	if (thread->kevent_overrides == 1) {
		needs_update = qos_override != old_override;
	} else {
		needs_update = qos_override > old_override;
	}

	if (needs_update) {
		proc_set_thread_policy_spinlocked(thread, TASK_POLICY_ATTRIBUTE,
		    TASK_POLICY_QOS_KEVENT_OVERRIDE,
		    qos_override, 0, &pend_token);
		assert(pend_token.tpt_update_sockets == 0);
	}

	thread_unlock(thread);
	splx(s);

	thread_policy_update_complete_unlocked(thread, &pend_token);
}

void
thread_add_kevent_override(thread_t thread, uint32_t qos_override)
{
	thread_kevent_override(thread, qos_override, TRUE);
}

void
thread_update_kevent_override(thread_t thread, uint32_t qos_override)
{
	thread_kevent_override(thread, qos_override, FALSE);
}

void
thread_drop_kevent_override(thread_t thread)
{
	struct task_pend_token pend_token = {};

	spl_t s = splsched();
	thread_lock(thread);

	assert(thread->kevent_overrides > 0);

	if (--thread->kevent_overrides == 0) {
		  
                                                              
                                           
     

		proc_set_thread_policy_spinlocked(thread, TASK_POLICY_ATTRIBUTE,
		    TASK_POLICY_QOS_KEVENT_OVERRIDE, THREAD_QOS_UNSPECIFIED,
		    0, &pend_token);
	}

	thread_unlock(thread);
	splx(s);

	thread_policy_update_complete_unlocked(thread, &pend_token);
}

  
                                                  
                                
  
                                                       
                                                                                 
  
                                            
                                                                                   
                                                                         
   
static void
thread_servicer_override(thread_t    thread,
    uint32_t    qos_override,
    boolean_t   is_new_override)
{
	struct task_pend_token pend_token = {};

	spl_t s = splsched();
	thread_lock(thread);

	if (is_new_override) {
		assert(!thread->requested_policy.thrp_qos_wlsvc_override);
	} else {
		assert(thread->requested_policy.thrp_qos_wlsvc_override);
	}

	proc_set_thread_policy_spinlocked(thread, TASK_POLICY_ATTRIBUTE,
	    TASK_POLICY_QOS_SERVICER_OVERRIDE,
	    qos_override, 0, &pend_token);

	thread_unlock(thread);
	splx(s);

	assert(pend_token.tpt_update_sockets == 0);
	thread_policy_update_complete_unlocked(thread, &pend_token);
}

void
thread_add_servicer_override(thread_t thread, uint32_t qos_override)
{
	assert(qos_override > THREAD_QOS_UNSPECIFIED);
	assert(qos_override < THREAD_QOS_LAST);

	thread_servicer_override(thread, qos_override, TRUE);
}

void
thread_update_servicer_override(thread_t thread, uint32_t qos_override)
{
	assert(qos_override > THREAD_QOS_UNSPECIFIED);
	assert(qos_override < THREAD_QOS_LAST);

	thread_servicer_override(thread, qos_override, FALSE);
}

void
thread_drop_servicer_override(thread_t thread)
{
	thread_servicer_override(thread, THREAD_QOS_UNSPECIFIED, FALSE);
}


                                                                             
thread_qos_t
thread_get_requested_qos(thread_t thread, int *relpri)
{
	int relprio_value = 0;
	thread_qos_t qos;

	qos = (thread_qos_t)proc_get_thread_policy_locked(thread, TASK_POLICY_ATTRIBUTE,
	    TASK_POLICY_QOS_AND_RELPRIO, &relprio_value);
	if (relpri) {
		*relpri = -relprio_value;
	}
	return qos;
}

  
                                                 
                                               
                                                    
                                           
  
                                                                                         
   
void
thread_set_exec_promotion(thread_t thread)
{
	spl_t s = splsched();
	thread_lock(thread);

	sched_thread_promote_reason(thread, TH_SFLAG_EXEC_PROMOTED, 0);

	thread_unlock(thread);
	splx(s);
}

  
                                           
                                                            
   
void
thread_clear_exec_promotion(thread_t thread)
{
	spl_t s = splsched();
	thread_lock(thread);

	sched_thread_unpromote_reason(thread, TH_SFLAG_EXEC_PROMOTED, 0);

	thread_unlock(thread);
	splx(s);
}
