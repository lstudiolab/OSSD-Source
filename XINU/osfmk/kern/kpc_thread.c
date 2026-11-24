/*
 * Copyright (c) 2012 Apple Inc. All rights reserved.
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

#include <mach/mach_types.h>
#include <kern/processor.h>
#include <kern/thread.h>
#include <kern/assert.h>
#include <kern/locks.h>
#include <sys/errno.h>

#include <kperf/kperf.h>
#include <kperf/buffer.h>
#include <kperf/context.h>
#include <kperf/sample.h>
#include <kperf/action.h>
#include <kperf/kperf_kpc.h>
#include <kern/kpc.h>

#if defined (__arm64__)
#include <arm/cpu_data_internal.h>
#elif defined (__arm__)
#include <arm/cpu_data_internal.h>
#endif

                                                       
int kpc_threads_counting = 0;

                                                         
boolean_t kpc_off_cpu_active = FALSE;

                                                          
static uint32_t kpc_thread_classes = 0;
static uint32_t kpc_thread_classes_count = 0;

static LCK_GRP_DECLARE(kpc_thread_lckgrp, "kpc thread");
static LCK_MTX_DECLARE(kpc_thread_lock, &kpc_thread_lckgrp);

uint32_t
kpc_get_thread_counting(void)
{
	uint32_t kpc_thread_classes_tmp;
	int kpc_threads_counting_tmp;

	                                                            
	lck_mtx_lock(&kpc_thread_lock);

	kpc_thread_classes_tmp = kpc_thread_classes;
	kpc_threads_counting_tmp = kpc_threads_counting;

	lck_mtx_unlock(&kpc_thread_lock);

	if (kpc_threads_counting_tmp) {
		return kpc_thread_classes_tmp;
	} else {
		return 0;
	}
}

int
kpc_set_thread_counting(uint32_t classes)
{
	uint32_t count;

	lck_mtx_lock(&kpc_thread_lock);

	count = kpc_get_counter_count(classes);

	if ((classes == 0)
	    || (count == 0)) {
		               
		kpc_threads_counting = FALSE;
	} else {
		                      
		kpc_thread_classes = classes;

		                       
		kpc_thread_classes_count = count;
		assert(kpc_thread_classes_count <= KPC_MAX_COUNTERS);

		                   
		kpc_threads_counting = TRUE;

		                                            
		if (!current_thread()->kpc_buf) {
			current_thread()->kperf_ast |= T_KPC_ALLOC;
			act_set_kperf(current_thread());
		}
	}

	kpc_off_cpu_update();
	lck_mtx_unlock(&kpc_thread_lock);

	return 0;
}

                                                                     
static void
kpc_update_thread_counters( thread_t thread )
{
	uint32_t i;
	uint64_t *tmp = NULL;
	cpu_data_t *cpu = NULL;

	cpu = current_cpu_datap();

	                                                 
	kpc_get_cpu_counters( FALSE, kpc_thread_classes,
	    NULL, cpu->cpu_kpc_buf[1] );

	                                  
	if (thread->kpc_buf) {
		for (i = 0; i < kpc_thread_classes_count; i++) {
			thread->kpc_buf[i] += cpu->cpu_kpc_buf[1][i] - cpu->cpu_kpc_buf[0][i];
		}
	}

	                                        
	if (!current_thread()->kpc_buf) {
		current_thread()->kperf_ast |= T_KPC_ALLOC;
		act_set_kperf(current_thread());
	}

	                                      
	tmp = cpu->cpu_kpc_buf[1];
	cpu->cpu_kpc_buf[1] = cpu->cpu_kpc_buf[0];
	cpu->cpu_kpc_buf[0] = tmp;
}

                                     
int
kpc_get_curthread_counters(uint32_t *inoutcount, uint64_t *buf)
{
	thread_t thread = current_thread();
	boolean_t enabled;

	                         
	if (*inoutcount < kpc_thread_classes_count) {
		return EINVAL;
	}

	                               
	if (!thread->kpc_buf) {
		return EINVAL;
	}

	enabled = ml_set_interrupts_enabled(FALSE);

	                                                     
	kpc_update_thread_counters( current_thread());

	              
	memcpy( buf, thread->kpc_buf,
	    kpc_thread_classes_count * sizeof(*buf));
	*inoutcount = kpc_thread_classes_count;

	ml_set_interrupts_enabled(enabled);

	return 0;
}

void
kpc_off_cpu_update(void)
{
	kpc_off_cpu_active = kpc_threads_counting;
}

void
kpc_off_cpu_internal(thread_t thread)
{
	if (kpc_threads_counting) {
		kpc_update_thread_counters(thread);
	}
}

void
kpc_thread_create(thread_t thread)
{
	                                         
	if (!kpc_threads_counting) {
		return;
	}

	                                      
	thread->kpc_buf = kpc_counterbuf_alloc();
}

void
kpc_thread_destroy(thread_t thread)
{
	uint64_t *buf = NULL;

	                                         
	if (!thread->kpc_buf) {
		return;
	}

	                           
	buf = thread->kpc_buf;
	thread->kpc_buf = NULL;
	kpc_counterbuf_free(buf);
}

void
kpc_thread_ast_handler(thread_t thread)
{
	if (thread->kperf_ast & T_KPC_ALLOC) {
		thread->kpc_buf = kpc_counterbuf_alloc();
	}
}
