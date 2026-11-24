/*
 * Copyright (c) 2011-2019 Apple Computer, Inc. All rights reserved.
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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#ifndef KPERF_H
#define KPERF_H

#include <kern/thread.h>
#include <kern/locks.h>

extern lck_grp_t kperf_lck_grp;

                                          
#define TRIGGER_TYPE_TIMER     (0)
#define TRIGGER_TYPE_PMI       (1)
#define TRIGGER_TYPE_KDEBUG    (2)
#define TRIGGER_TYPE_LAZY_WAIT (3)
#define TRIGGER_TYPE_LAZY_CPU  (3)

uint32_t kperf_get_thread_ast(thread_t thread);
void kperf_set_thread_ast(thread_t thread, uint32_t flags);

  
                                                                         
                                            
   
boolean_t kperf_thread_get_dirty(thread_t thread);
void kperf_thread_set_dirty(thread_t thread, boolean_t dirty);

  
                                                                                      
                                
   
void kperf_setup(void);

  
                                                       
   
extern void kperf_init_early(void);

bool kperf_is_sampling(void);
int kperf_enable_sampling(void);
int kperf_disable_sampling(void);
int kperf_port_to_pid(mach_port_name_t portname);

                                 
struct kperf_sample *kperf_intr_sample_buffer(void);

enum kperf_sampling {
	KPERF_SAMPLING_OFF,
	KPERF_SAMPLING_SHUTDOWN,
	KPERF_SAMPLING_ON,
};

extern enum kperf_sampling kperf_status;

#pragma mark - external callbacks

  
                                      
   
void kperf_init(void);

  
                    
  
                                                                           
                                                                               
   
extern __attribute__((noinline)) void kperf_thread_ast_handler(thread_t thread);

  
                                                       
   
void kperf_on_cpu_update(void);

  
                                                                            
   
static inline void
kperf_on_cpu(thread_t thread, thread_continue_t continuation,
    uintptr_t *starting_fp)
{
	extern boolean_t kperf_on_cpu_active;
	void kperf_on_cpu_internal(thread_t thread, thread_continue_t continuation,
	    uintptr_t *starting_fp);

	if (__improbable(kperf_on_cpu_active)) {
		kperf_on_cpu_internal(thread, continuation, starting_fp);
	}
}

  
                                                                             
   
static inline void
kperf_off_cpu(thread_t thread)
{
	extern unsigned int kperf_lazy_cpu_action;
	void kperf_lazy_off_cpu(thread_t thread);

	if (__improbable(kperf_lazy_cpu_action != 0)) {
		kperf_lazy_off_cpu(thread);
	}
}

  
                                                                         
   
static inline void
kperf_make_runnable(thread_t thread, int interrupt)
{
	extern unsigned int kperf_lazy_cpu_action;
	void kperf_lazy_make_runnable(thread_t thread, bool interrupt);

	if (__improbable(kperf_lazy_cpu_action != 0)) {
		kperf_lazy_make_runnable(thread, interrupt);
	}
}

static inline void
kperf_running_setup(processor_t processor, uint64_t now)
{
	if (kperf_status == KPERF_SAMPLING_ON) {
		extern void kptimer_running_setup(processor_t, uint64_t now);
		kptimer_running_setup(processor, now);
	}
}

  
                                                                       
   
static inline void
kperf_interrupt(void)
{
	extern unsigned int kperf_lazy_cpu_action;
	extern void kperf_lazy_cpu_sample(thread_t thread, unsigned int flags,
	    bool interrupt);

	if (__improbable(kperf_lazy_cpu_action != 0)) {
		kperf_lazy_cpu_sample(NULL, 0, true);
	}
}

  
                                                                          
                                            
   
static inline void
kperf_kdebug_callback(uint32_t debugid, uintptr_t *starting_fp)
{
	extern boolean_t kperf_kdebug_active;
	void kperf_kdebug_handler(uint32_t debugid, uintptr_t *starting_fp);

	if (__improbable(kperf_kdebug_active)) {
		kperf_kdebug_handler(debugid, starting_fp);
	}
}

  
                                                                           
                                                        
   
void kperf_timer_expire(void *param0, void *param1);

  
                                                            
   
extern void kperf_reset(void);

  
                                                      
   
void kperf_kernel_configure(const char *config);

#endif                        
