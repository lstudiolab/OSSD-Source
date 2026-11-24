/*
 * Copyright (c) 2012-2013, 2015 Apple Inc. All rights reserved.
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


  
                   
                   
  
                                                                                                  
                                                                                                    
                                                                                                    
                                                             
  
                          
                          
                                                                                                     
                                                                                           
  
                                  
                                  
                                                                                                   
                                                                                    
                                                                                                     
                                                                                                     
                                                                                                   
                                                                     
  
                                                                        
                                                                                                   
                                                                                                       
                                                                                                   
                                                                                                     
                                                                                                  
                                                                  
                                                                                               
                                                              
                                                                                                   
                                                                         
                                                                                                
                                                                         
                                                                                 
                                                                                          
                                         
                                                                                                    
                                                     
                                                                                              
                                                                                          
                                                        
                                                                                             
                                                                                               
                                         
                                                                                              
                                                                                             
                                                                                              
                                
  
  
                     
                     
                                                                                              
                                                                                                
                      
                                                                                        
                                                                         
                                                                                         
  
                                    
                                    
                                                                                                      
                           
                                                                                                   
                                                                                                      
                 
                                                                                                      
                                                             
  
                       
                       
                                                                                                   
                                                                                                
                                                    
                                                                                            
                               
  
                         
                         
                                                                                                
                                                                                              
                                     
  
   


#include <stdatomic.h>
#include <kern/assert.h>
#include <mach/mach_types.h>
#include <mach/boolean.h>
#include <mach/vm_param.h>
#include <kern/kern_types.h>
#include <kern/mach_param.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <corpses/task_corpse.h>
#include <kern/kalloc.h>
#include <kern/kern_cdata.h>
#include <mach/mach_vm.h>
#include <kern/exc_guard.h>
#include <os/log.h>

#if CONFIG_MACF
#include <security/mac_mach_internal.h>
#endif

  
                      
   
#include <mach/task_server.h>

union corpse_creation_gate {
	struct {
		uint16_t user_faults;
		uint16_t corpses;
	};
	uint32_t value;
};

static _Atomic uint32_t inflight_corpses;
unsigned long  total_corpses_created = 0;
boolean_t corpse_enabled_config = TRUE;

                                                                 
boolean_t corpse_threshold_system_limit = FALSE;

                                                        
int exc_via_corpse_forking = 1;

                                                                          
int corpse_for_fatal_memkill = 1;

#ifdef  __arm__
static inline int
IS_64BIT_PROCESS(__unused void *p)
{
	return 0;
}
#else
extern int IS_64BIT_PROCESS(void *);
#endif              
extern void gather_populate_corpse_crashinfo(void *p, task_t task,
    mach_exception_data_type_t code, mach_exception_data_type_t subcode,
    uint64_t *udata_buffer, int num_udata, void *reason);
extern void *proc_find(int pid);
extern int proc_rele(void *p);


void
corpses_init()
{
	char temp_buf[20];
	int exc_corpse_forking;
	int fatal_memkill;
	if (PE_parse_boot_argn("-no_corpses", temp_buf, sizeof(temp_buf))) {
		corpse_enabled_config = FALSE;
	}
	if (PE_parse_boot_argn("exc_via_corpse_forking", &exc_corpse_forking, sizeof(exc_corpse_forking))) {
		exc_via_corpse_forking = exc_corpse_forking;
	}
	if (PE_parse_boot_argn("corpse_for_fatal_memkill", &fatal_memkill, sizeof(fatal_memkill))) {
		corpse_for_fatal_memkill = fatal_memkill;
	}
#if DEBUG || DEVELOPMENT
	if (PE_parse_boot_argn("-corpse_threshold_system_limit", &corpse_threshold_system_limit, sizeof(corpse_threshold_system_limit))) {
		corpse_threshold_system_limit = TRUE;
	}
#endif                           
}

  
                           
                               
   
boolean_t
corpses_enabled()
{
	return corpse_enabled_config;
}

unsigned long
total_corpses_count(void)
{
	union corpse_creation_gate gate;

	gate.value = atomic_load_explicit(&inflight_corpses, memory_order_relaxed);
	return gate.corpses;
}

extern char *proc_best_name(struct proc *);
extern int proc_pid(struct proc *);

  
                                    
                                             
                                                                    
   
static kern_return_t
task_crashinfo_get_ref(corpse_flags_t kcd_u_flags)
{
	union corpse_creation_gate oldgate, newgate;
	struct proc *p = (void *)current_proc();

	assert(kcd_u_flags & CORPSE_CRASHINFO_HAS_REF);

	oldgate.value = atomic_load_explicit(&inflight_corpses, memory_order_relaxed);
	for (;;) {
		newgate = oldgate;
		if (kcd_u_flags & CORPSE_CRASHINFO_USER_FAULT) {
			if (newgate.user_faults++ >= TOTAL_USER_FAULTS_ALLOWED) {
				os_log(OS_LOG_DEFAULT, "%s[%d] Corpse failure, too many faults %d\n",
				    proc_best_name(p), proc_pid(p), newgate.user_faults);
				return KERN_RESOURCE_SHORTAGE;
			}
		}
		if (newgate.corpses++ >= TOTAL_CORPSES_ALLOWED) {
			os_log(OS_LOG_DEFAULT, "%s[%d] Corpse failure, too many %d\n",
			    proc_best_name(p), proc_pid(p), newgate.corpses);
			return KERN_RESOURCE_SHORTAGE;
		}

		                                    
		if (atomic_compare_exchange_strong_explicit(&inflight_corpses,
		    &oldgate.value, newgate.value, memory_order_relaxed,
		    memory_order_relaxed)) {
			os_log(OS_LOG_DEFAULT, "%s[%d] Corpse allowed %d of %d\n",
			    proc_best_name(p), proc_pid(p), newgate.corpses, TOTAL_CORPSES_ALLOWED);
			return KERN_SUCCESS;
		}
	}
}

  
                                      
                                                   
   
static kern_return_t
task_crashinfo_release_ref(corpse_flags_t kcd_u_flags)
{
	union corpse_creation_gate oldgate, newgate;

	assert(kcd_u_flags & CORPSE_CRASHINFO_HAS_REF);

	oldgate.value = atomic_load_explicit(&inflight_corpses, memory_order_relaxed);
	for (;;) {
		newgate = oldgate;
		if (kcd_u_flags & CORPSE_CRASHINFO_USER_FAULT) {
			if (newgate.user_faults-- == 0) {
				panic("corpse in flight count over-release");
			}
		}
		if (newgate.corpses-- == 0) {
			panic("corpse in flight count over-release");
		}
		                                    
		if (atomic_compare_exchange_strong_explicit(&inflight_corpses,
		    &oldgate.value, newgate.value, memory_order_relaxed,
		    memory_order_relaxed)) {
			os_log(OS_LOG_DEFAULT, "Corpse released, count at %d\n", newgate.corpses);
			return KERN_SUCCESS;
		}
	}
}


kcdata_descriptor_t
task_crashinfo_alloc_init(mach_vm_address_t crash_data_p, unsigned size,
    corpse_flags_t kc_u_flags, unsigned kc_flags)
{
	kcdata_descriptor_t kcdata;

	if (kc_u_flags & CORPSE_CRASHINFO_HAS_REF) {
		if (KERN_SUCCESS != task_crashinfo_get_ref(kc_u_flags)) {
			return NULL;
		}
	}

	kcdata = kcdata_memory_alloc_init(crash_data_p, TASK_CRASHINFO_BEGIN, size,
	    kc_flags);
	if (kcdata) {
		kcdata->kcd_user_flags = kc_u_flags;
	} else if (kc_u_flags & CORPSE_CRASHINFO_HAS_REF) {
		task_crashinfo_release_ref(kc_u_flags);
	}
	return kcdata;
}


  
                                                         
   
kern_return_t
task_crashinfo_destroy(kcdata_descriptor_t data)
{
	if (!data) {
		return KERN_INVALID_ARGUMENT;
	}
	if (data->kcd_user_flags & CORPSE_CRASHINFO_HAS_REF) {
		task_crashinfo_release_ref(data->kcd_user_flags);
	}
	return kcdata_memory_destroy(data);
}

  
                               
                                                   
                                             
                                                      
   
kcdata_descriptor_t
task_get_corpseinfo(task_t task)
{
	kcdata_descriptor_t retval = NULL;
	if (task != NULL) {
		retval = task->corpse_info;
	}
	return retval;
}

  
                                        
                                                      
                 
   
void
task_add_to_corpse_task_list(task_t corpse_task)
{
	lck_mtx_lock(&tasks_corpse_lock);
	queue_enter(&corpse_tasks, corpse_task, task_t, corpse_tasks);
	lck_mtx_unlock(&tasks_corpse_lock);
}

  
                                             
                                                          
                 
   
void
task_remove_from_corpse_task_list(task_t corpse_task)
{
	lck_mtx_lock(&tasks_corpse_lock);
	queue_remove(&corpse_tasks, corpse_task, task_t, corpse_tasks);
	lck_mtx_unlock(&tasks_corpse_lock);
}

  
                                  
                
                 
   
void
task_purge_all_corpses(void)
{
	task_t task;

	printf("Purging corpses......\n\n");

	lck_mtx_lock(&tasks_corpse_lock);
	                                                                    
	queue_iterate(&corpse_tasks, task, task_t, corpse_tasks) {
		vm_map_remove(task->map,
		    task->map->min_offset,
		    task->map->max_offset,
		      
                       
                       
                                    
                                  
         
		    (VM_MAP_REMOVE_NO_UNNESTING |
		    VM_MAP_REMOVE_IMMUTABLE |
		    VM_MAP_REMOVE_GAPS_OK));
	}

	lck_mtx_unlock(&tasks_corpse_lock);
}

  
                                
                                       
                                                          
                                    
                                    
                                                  
                                                                                  
   
kern_return_t
task_generate_corpse(
	task_t task,
	ipc_port_t *corpse_task_port)
{
	task_t new_task;
	kern_return_t kr;
	thread_t thread, th_iter;
	ipc_port_t corpse_port;
	ipc_port_t old_notify;

	if (task == kernel_task || task == TASK_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	task_lock(task);
	if (task_is_a_corpse_fork(task)) {
		task_unlock(task);
		return KERN_INVALID_ARGUMENT;
	}
	task_unlock(task);

	                                                                                 
	kr = task_generate_corpse_internal(task, &new_task, &thread, 0, 0, 0, NULL);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	if (thread != THREAD_NULL) {
		thread_deallocate(thread);
	}

	                                                       
	task_lock(new_task);
	task_wait_till_threads_terminate_locked(new_task);

	                                                   
	queue_iterate(&new_task->threads, th_iter, thread_t, task_threads)
	{
		                                                       
		if (th_iter->corpse_dup == FALSE) {
			ipc_thread_reset(th_iter);
		}
	}
	task_unlock(new_task);

	                                                                       
	corpse_port = convert_task_to_port(new_task);
	assert(IP_NULL != corpse_port);

	ip_lock(corpse_port);
	require_ip_active(corpse_port);
	ipc_port_nsrequest(corpse_port, corpse_port->ip_mscount, ipc_port_make_sonce_locked(corpse_port), &old_notify);
	                   

	assert(IP_NULL == old_notify);
	*corpse_task_port = corpse_port;
	return KERN_SUCCESS;
}

  
                                              
                                                          
                                            
                                               
                                                        
  
                                    
                                    
                                                              
                                                  
                                                                                  
   
kern_return_t
task_enqueue_exception_with_corpse(
	task_t task,
	exception_type_t etype,
	mach_exception_data_t code,
	mach_msg_type_number_t codeCnt,
	void *reason)
{
	task_t new_task = TASK_NULL;
	thread_t thread = THREAD_NULL;
	kern_return_t kr;

	if (codeCnt < 2) {
		return KERN_INVALID_ARGUMENT;
	}

	                                                                                 
	kr = task_generate_corpse_internal(task, &new_task, &thread,
	    etype, code[0], code[1], reason);
	if (kr == KERN_SUCCESS) {
		if (thread == THREAD_NULL) {
			return KERN_FAILURE;
		}
		assert(new_task != TASK_NULL);
		assert(etype == EXC_RESOURCE || etype == EXC_GUARD);
		thread_exception_enqueue(new_task, thread, etype);
	}
	return kr;
}

  
                                         
                                       
                                                     
                                                                       
                                                 
                                                                 
                                                                       
                                    
                                    
                                                  
                                                                                  
   
kern_return_t
task_generate_corpse_internal(
	task_t task,
	task_t *corpse_task,
	thread_t *exc_thread,
	exception_type_t etype,
	mach_exception_data_type_t code,
	mach_exception_data_type_t subcode,
	void *reason)
{
	task_t new_task = TASK_NULL;
	thread_t thread = THREAD_NULL;
	thread_t thread_next = THREAD_NULL;
	kern_return_t kr;
	struct proc *p = NULL;
	int is_64bit_addr;
	int is_64bit_data;
	int t_flags;
	uint64_t *udata_buffer = NULL;
	int size = 0;
	int num_udata = 0;
	corpse_flags_t kc_u_flags = CORPSE_CRASHINFO_HAS_REF;

#if CONFIG_MACF
	struct label *label = NULL;
#endif

	if (!corpses_enabled()) {
		return KERN_NOT_SUPPORTED;
	}

	if (etype == EXC_GUARD && EXC_GUARD_DECODE_GUARD_TYPE(code) == GUARD_TYPE_USER) {
		kc_u_flags |= CORPSE_CRASHINFO_USER_FAULT;
	}

	kr = task_crashinfo_get_ref(kc_u_flags);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	                                                                 
	p = proc_find(task_pid(task));
	if (p == NULL) {
		kr = KERN_INVALID_TASK;
		goto error_task_generate_corpse;
	}

	is_64bit_addr = IS_64BIT_PROCESS(p);
	is_64bit_data = (task == TASK_NULL) ? is_64bit_addr : task_get_64bit_data(task);
	t_flags = TF_CORPSE_FORK |
	    TF_PENDING_CORPSE |
	    TF_CORPSE |
	    (is_64bit_addr ? TF_64B_ADDR : TF_NONE) |
	    (is_64bit_data ? TF_64B_DATA : TF_NONE);

#if CONFIG_MACF
	                                                           
	label = mac_exc_create_label_for_proc(p);
#endif

	                              
	kr = task_create_internal(task,
	    NULL,
	    TRUE,
	    is_64bit_addr,
	    is_64bit_data,
	    t_flags,
	    TPF_NONE,
	    TWF_NONE,
	    &new_task);
	if (kr != KERN_SUCCESS) {
		goto error_task_generate_corpse;
	}

	                                                                
	kr = task_duplicate_map_and_threads(task, p, new_task, &thread,
	    &udata_buffer, &size, &num_udata);
	if (kr != KERN_SUCCESS) {
		goto error_task_generate_corpse;
	}

	kr = task_collect_crash_info(new_task,
#if CONFIG_MACF
	    label,
#endif
	    TRUE);
	if (kr != KERN_SUCCESS) {
		goto error_task_generate_corpse;
	}

	                                                
	assert(new_task->corpse_info->kcd_user_flags == 0);
	new_task->corpse_info->kcd_user_flags = kc_u_flags;
	kc_u_flags = 0;

	kr = task_start_halt(new_task);
	if (kr != KERN_SUCCESS) {
		goto error_task_generate_corpse;
	}

	                             
	ipc_space_terminate(new_task->itk_space);

	                                                                                  
	gather_populate_corpse_crashinfo(p, new_task,
	    code, subcode, udata_buffer, num_udata, reason);

	                                       
	task_add_to_corpse_task_list(new_task);

	*corpse_task = new_task;
	*exc_thread = thread;

error_task_generate_corpse:
#if CONFIG_MACF
	if (label) {
		mac_exc_free_label(label);
	}
#endif

	                                
	if (p != NULL) {
		proc_rele(p);
	}

	if (kr != KERN_SUCCESS) {
		if (thread != THREAD_NULL) {
			thread_deallocate(thread);
		}
		if (new_task != TASK_NULL) {
			task_lock(new_task);
			                                                  
			queue_iterate(&new_task->threads, thread_next, thread_t, task_threads)
			{
				thread_terminate_internal(thread_next, TH_TERMINATE_OPTION_NONE);
			}
			                                                       
			task_wait_till_threads_terminate_locked(new_task);
			task_unlock(new_task);

			task_clear_corpse(new_task);
			task_terminate_internal(new_task);
			task_deallocate(new_task);
		}
		if (kc_u_flags) {
			task_crashinfo_release_ref(kc_u_flags);
		}
	}
	                                                                       
	if (udata_buffer != NULL) {
		kheap_free(KHEAP_DATA_BUFFERS, udata_buffer, size);
	}

	return kr;
}

  
                                
                                                             
                                                
                                                             
                                                          
                                    
                                    
                                                       
                                                  
   
kern_return_t
task_map_corpse_info(
	task_t task,
	task_t corpse_task,
	vm_address_t *kcd_addr_begin,
	uint32_t *kcd_size)
{
	kern_return_t kr;
	mach_vm_address_t kcd_addr_begin_64;
	mach_vm_size_t size_64;

	kr = task_map_corpse_info_64(task, corpse_task, &kcd_addr_begin_64, &size_64);
	if (kr != KERN_SUCCESS) {
		return kr;
	}

	*kcd_addr_begin = (vm_address_t)kcd_addr_begin_64;
	*kcd_size = (uint32_t) size_64;
	return KERN_SUCCESS;
}

  
                                   
                                                             
                                                
                                                                                        
                                                                                   
                                    
                                    
                                                       
   
kern_return_t
task_map_corpse_info_64(
	task_t task,
	task_t corpse_task,
	mach_vm_address_t *kcd_addr_begin,
	mach_vm_size_t *kcd_size)
{
	kern_return_t kr;
	mach_vm_offset_t crash_data_ptr = 0;
	const mach_vm_size_t size = CORPSEINFO_ALLOCATION_SIZE;
	void *corpse_info_kernel = NULL;

	if (task == TASK_NULL || task_is_a_corpse_fork(task)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (corpse_task == TASK_NULL || !task_is_a_corpse(corpse_task) ||
	    kcdata_memory_get_begin_addr(corpse_task->corpse_info) == NULL) {
		return KERN_INVALID_ARGUMENT;
	}
	corpse_info_kernel = kcdata_memory_get_begin_addr(corpse_task->corpse_info);
	kr = mach_vm_allocate_kernel(task->map, &crash_data_ptr, size,
	    VM_FLAGS_ANYWHERE, VM_MEMORY_CORPSEINFO);
	if (kr != KERN_SUCCESS) {
		return kr;
	}
	copyout(corpse_info_kernel, (user_addr_t)crash_data_ptr, (size_t)size);
	*kcd_addr_begin = crash_data_ptr;
	*kcd_size = size;

	return KERN_SUCCESS;
}

uint64_t
task_corpse_get_crashed_thread_id(task_t corpse_task)
{
	return corpse_task->crashed_thread_id;
}
