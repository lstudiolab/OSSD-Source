/*
 * Copyright (c) 2007-2020 Apple Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

  
                                    
  
                                                        
  
   
  
                 
                 
  
                                                                          
                                                         
             
               
                   
                                                   
  
                                                                             
                                                                          
                                                                                 
                                                                          
                                                                        
                                                                               
          
  
                                                                                
                                                                        
                                                                             
                                                                            
                                                   
  
                                                                                
                                                                       
                                                                          
                                                                            
                                                                            
  
                                                                              
                                                                                  
                                                                      
                                      
  
                                                                         
                                                 
  
                                                                           
                                                                     
  
                                                                                
                                                                      
                                                                          
                                                   
   

  
            
  
                                                                          
                                                                        
                                                                        
                                                                       
                                          
  
                                                         
  
                                                                     
                                                                    
  
                                                                     
                                                   
  
                                                                        
                                                                      
                                                                           
                                      
   

#include <debug.h>

#include <kern/ipc_tt.h>
#include <kern/kalloc.h>
#include <kern/thread_call.h>

#include <mach/mach_vm.h>

#include <vm/vm_map.h>
#include <vm/vm_shared_region.h>

#include <vm/vm_protos.h>

#include <machine/commpage.h>
#include <machine/cpu_capabilities.h>
#include <sys/random.h>

#if defined (__arm__) || defined(__arm64__)
#include <arm/cpu_data_internal.h>
#include <arm/misc_protos.h>
#endif

  
                                                
                                     
   
#define PROCESS_SHARED_CACHE_LAYOUT 0x00

#if __has_feature(ptrauth_calls)
#include <ptrauth.h>
#endif                                   

                                                             
int shared_region_version = 3;

                                                        
int shared_region_trace_level = SHARED_REGION_TRACE_ERROR_LVL;

                                                                               
int shared_region_persistence = 0;                         


                                                                
TUNABLE_WRITEABLE(int, shared_region_destroy_delay, "vm_shared_region_destroy_delay", 120);

  
                                                                                    
                                                                                     
                                                   
  
                                          
   
struct vm_shared_region *primary_system_shared_region = NULL;

#if XNU_TARGET_OS_OSX
  
                                                          
                                                               
                                  
   
boolean_t shared_region_completed_slide = FALSE;
#endif                        

                                                              
static LCK_GRP_DECLARE(vm_shared_region_lck_grp, "vm shared region");
static LCK_MTX_DECLARE(vm_shared_region_lock, &vm_shared_region_lck_grp);

#define vm_shared_region_lock() lck_mtx_lock(&vm_shared_region_lock)
#define vm_shared_region_unlock() lck_mtx_unlock(&vm_shared_region_lock)
#define vm_shared_region_sleep(event, interruptible)                    \
	lck_mtx_sleep(&vm_shared_region_lock,                           \
	              LCK_SLEEP_DEFAULT,                                \
	              (event_t) (event),                                \
	              (interruptible))

                                                                          
queue_head_t    vm_shared_region_queue = QUEUE_HEAD_INITIALIZER(vm_shared_region_queue);
int             vm_shared_region_count = 0;
int             vm_shared_region_peak = 0;

  
                                                                           
                       
   
#if __has_feature(ptrauth_calls)
int                             vm_shared_region_reslide_count = 0;
#endif                                   

static void vm_shared_region_reference_locked(vm_shared_region_t shared_region);
static vm_shared_region_t vm_shared_region_create(
	void                    *root_dir,
	cpu_type_t              cputype,
	cpu_subtype_t           cpu_subtype,
	boolean_t               is_64bit,
	boolean_t               reslide);
static void vm_shared_region_destroy(vm_shared_region_t shared_region);

static kern_return_t vm_shared_region_slide_sanity_check(vm_shared_region_slide_info_entry_t entry, mach_vm_size_t size);
static void vm_shared_region_timeout(thread_call_param_t param0,
    thread_call_param_t param1);
static kern_return_t vm_shared_region_slide_mapping(
	vm_shared_region_t sr,
	user_addr_t        slide_info_addr,
	mach_vm_size_t     slide_info_size,
	mach_vm_offset_t   start,
	mach_vm_size_t     size,
	mach_vm_offset_t   slid_mapping,
	uint32_t           slide,
	memory_object_control_t,
	vm_prot_t          prot);              

static int __commpage_setup = 0;
#if XNU_TARGET_OS_OSX
static int __system_power_source = 1;                                     
static void post_sys_powersource_internal(int i, int internal);
#endif                        

extern u_int32_t random(void);

  
                                                                 
                                                               
                                                                  
             
   
vm_shared_region_t
vm_shared_region_get(
	task_t          task)
{
	vm_shared_region_t      shared_region;

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: -> get(%p)\n",
		(void *)VM_KERNEL_ADDRPERM(task)));

	task_lock(task);
	vm_shared_region_lock();
	shared_region = task->shared_region;
	if (shared_region) {
		assert(shared_region->sr_ref_count > 0);
		vm_shared_region_reference_locked(shared_region);
	}
	vm_shared_region_unlock();
	task_unlock(task);

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: get(%p) <- %p\n",
		(void *)VM_KERNEL_ADDRPERM(task),
		(void *)VM_KERNEL_ADDRPERM(shared_region)));

	return shared_region;
}

vm_map_t
vm_shared_region_vm_map(
	vm_shared_region_t      shared_region)
{
	ipc_port_t              sr_handle;
	vm_named_entry_t        sr_mem_entry;
	vm_map_t                sr_map;

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: -> vm_map(%p)\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region)));
	assert(shared_region->sr_ref_count > 0);

	sr_handle = shared_region->sr_mem_entry;
	sr_mem_entry = (vm_named_entry_t) ip_get_kobject(sr_handle);
	sr_map = sr_mem_entry->backing.map;
	assert(sr_mem_entry->is_sub_map);

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: vm_map(%p) <- %p\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region),
		(void *)VM_KERNEL_ADDRPERM(sr_map)));
	return sr_map;
}

  
                                                
                                                                      
                 
                                                                             
                                                                       
   
void
vm_shared_region_set(
	task_t                  task,
	vm_shared_region_t      new_shared_region)
{
	vm_shared_region_t      old_shared_region;

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: -> set(%p, %p)\n",
		(void *)VM_KERNEL_ADDRPERM(task),
		(void *)VM_KERNEL_ADDRPERM(new_shared_region)));

	task_lock(task);
	vm_shared_region_lock();

	old_shared_region = task->shared_region;
	if (new_shared_region) {
		assert(new_shared_region->sr_ref_count > 0);
	}

	task->shared_region = new_shared_region;

	vm_shared_region_unlock();
	task_unlock(task);

	if (old_shared_region) {
		assert(old_shared_region->sr_ref_count > 0);
		vm_shared_region_deallocate(old_shared_region);
	}

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: set(%p) <- old=%p new=%p\n",
		(void *)VM_KERNEL_ADDRPERM(task),
		(void *)VM_KERNEL_ADDRPERM(old_shared_region),
		(void *)VM_KERNEL_ADDRPERM(new_shared_region)));
}

  
                                                           
                                              
                                                                      
                                                                          
                                                                  
   
vm_shared_region_t
vm_shared_region_lookup(
	void            *root_dir,
	cpu_type_t      cputype,
	cpu_subtype_t   cpu_subtype,
	boolean_t       is_64bit,
	boolean_t       reslide)
{
	vm_shared_region_t      shared_region;
	vm_shared_region_t      new_shared_region;

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: -> lookup(root=%p,cpu=<%d,%d>,64bit=%d,reslide=%d)\n",
		(void *)VM_KERNEL_ADDRPERM(root_dir),
		cputype, cpu_subtype, is_64bit, reslide));

	shared_region = NULL;
	new_shared_region = NULL;

	vm_shared_region_lock();
	for (;;) {
		queue_iterate(&vm_shared_region_queue,
		    shared_region,
		    vm_shared_region_t,
		    sr_q) {
			assert(shared_region->sr_ref_count > 0);
			if (shared_region->sr_cpu_type == cputype &&
#if !__has_feature(ptrauth_calls)                                                                   
			    shared_region->sr_cpu_subtype == cpu_subtype &&
#endif                                    
			    shared_region->sr_root_dir == root_dir &&
			    shared_region->sr_64bit == is_64bit &&
#if __has_feature(ptrauth_calls)
			    shared_region->sr_reslide == reslide &&
#endif                                   
			    !shared_region->sr_stale) {
				                     
				vm_shared_region_reference_locked(shared_region);
				goto done;
			}
		}
		if (new_shared_region == NULL) {
			                                
			vm_shared_region_unlock();
			new_shared_region = vm_shared_region_create(root_dir,
			    cputype,
			    cpu_subtype,
			    is_64bit,
			    reslide);
			                                                 
			vm_shared_region_lock();
			continue;
		}
		                                     
		shared_region = new_shared_region;
		new_shared_region = NULL;
		queue_enter(&vm_shared_region_queue,
		    shared_region,
		    vm_shared_region_t,
		    sr_q);
		vm_shared_region_count++;
		if (vm_shared_region_count > vm_shared_region_peak) {
			vm_shared_region_peak = vm_shared_region_count;
		}
		break;
	}

done:
	vm_shared_region_unlock();

	if (new_shared_region) {
		  
                                                            
                                                            
     
		assert(new_shared_region->sr_ref_count == 1);
		new_shared_region->sr_ref_count--;
		vm_shared_region_destroy(new_shared_region);
		new_shared_region = NULL;
	}

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: lookup(root=%p,cpu=<%d,%d>,64bit=%d,reslide=%d) <- %p\n",
		(void *)VM_KERNEL_ADDRPERM(root_dir),
		cputype, cpu_subtype, is_64bit, reslide,
		(void *)VM_KERNEL_ADDRPERM(shared_region)));

	assert(shared_region->sr_ref_count > 0);
	return shared_region;
}

  
                                              
                                                                  
   
static void
vm_shared_region_reference_locked(
	vm_shared_region_t      shared_region)
{
	LCK_MTX_ASSERT(&vm_shared_region_lock, LCK_MTX_ASSERT_OWNED);

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: -> reference_locked(%p)\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region)));
	assert(shared_region->sr_ref_count > 0);
	shared_region->sr_ref_count++;
	assert(shared_region->sr_ref_count != 0);

	if (shared_region->sr_timer_call != NULL) {
		boolean_t cancelled;

		                                         
		cancelled = thread_call_cancel(shared_region->sr_timer_call);
		if (cancelled) {
			thread_call_free(shared_region->sr_timer_call);
			shared_region->sr_timer_call = NULL;
			                                                       
			shared_region->sr_ref_count--;
		} else {
			                                                       
		}
	}

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: reference_locked(%p) <- %d\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region),
		shared_region->sr_ref_count));
}

  
                                       
   
void
vm_shared_region_reference(vm_shared_region_t shared_region)
{
	SHARED_REGION_TRACE_DEBUG(
		("shared_region: -> reference(%p)\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region)));

	vm_shared_region_lock();
	vm_shared_region_reference_locked(shared_region);
	vm_shared_region_unlock();

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: reference(%p) <- %d\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region),
		shared_region->sr_ref_count));
}

  
                                            
                                              
   
void
vm_shared_region_deallocate(
	vm_shared_region_t      shared_region)
{
	SHARED_REGION_TRACE_DEBUG(
		("shared_region: -> deallocate(%p)\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region)));

	vm_shared_region_lock();

	assert(shared_region->sr_ref_count > 0);

	if (shared_region->sr_root_dir == NULL) {
		  
                                                         
                                                                
            
                                      
    
                                                             
                                                                     
     
		if (shared_region_persistence &&
		    !shared_region->sr_persists) {
			                              
			shared_region->sr_ref_count++;
			shared_region->sr_persists = TRUE;
		} else if (!shared_region_persistence &&
		    shared_region->sr_persists) {
			                                        
			assert(shared_region->sr_ref_count > 1);
			shared_region->sr_ref_count--;
			shared_region->sr_persists = FALSE;
		}
	}

	assert(shared_region->sr_ref_count > 0);
	shared_region->sr_ref_count--;
	SHARED_REGION_TRACE_DEBUG(
		("shared_region: deallocate(%p): ref now %d\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region),
		shared_region->sr_ref_count));

	if (shared_region->sr_ref_count == 0) {
		uint64_t deadline;

		  
                                                                
                                                          
     
		if (shared_region->sr_timer_call == NULL &&
		    shared_region_destroy_delay != 0 &&
		    !shared_region->sr_stale) {
			                                      
			assert(!shared_region->sr_mapping_in_progress);
			shared_region->sr_ref_count++;

			                      
			shared_region->sr_timer_call = thread_call_allocate(
				(thread_call_func_t) vm_shared_region_timeout,
				(thread_call_param_t) shared_region);

			                        
			clock_interval_to_deadline(shared_region_destroy_delay,
			    NSEC_PER_SEC,
			    &deadline);
			thread_call_enter_delayed(shared_region->sr_timer_call,
			    deadline);

			SHARED_REGION_TRACE_DEBUG(
				("shared_region: deallocate(%p): armed timer\n",
				(void *)VM_KERNEL_ADDRPERM(shared_region)));

			vm_shared_region_unlock();
		} else {
			                                                 

			                                                        
			if (primary_system_shared_region == shared_region) {
				primary_system_shared_region = NULL;
			}

			  
                                                        
           
      
			queue_remove(&vm_shared_region_queue,
			    shared_region,
			    vm_shared_region_t,
			    sr_q);
			vm_shared_region_count--;
			vm_shared_region_unlock();

			                        
			vm_shared_region_destroy(shared_region);
			shared_region = NULL;
		}
	} else {
		vm_shared_region_unlock();
	}

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: deallocate(%p) <-\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region)));
}

void
vm_shared_region_timeout(
	thread_call_param_t     param0,
	__unused thread_call_param_t    param1)
{
	vm_shared_region_t      shared_region;

	shared_region = (vm_shared_region_t) param0;

	vm_shared_region_deallocate(shared_region);
}


  
                                                            
   
static vm_shared_region_t
vm_shared_region_create(
	void                    *root_dir,
	cpu_type_t              cputype,
	cpu_subtype_t           cpu_subtype,
	boolean_t               is_64bit,
#if !__has_feature(ptrauth_calls)
	__unused
#endif                                   
	boolean_t               reslide)
{
	kern_return_t           kr;
	vm_named_entry_t        mem_entry;
	ipc_port_t              mem_entry_port;
	vm_shared_region_t      shared_region;
	vm_map_t                sub_map;
	mach_vm_offset_t        base_address, pmap_nesting_start;
	mach_vm_size_t          size, pmap_nesting_size;

	SHARED_REGION_TRACE_INFO(
		("shared_region: -> create(root=%p,cpu=<%d,%d>,64bit=%d,reslide=%d)\n",
		(void *)VM_KERNEL_ADDRPERM(root_dir),
		cputype, cpu_subtype, is_64bit, reslide));

	base_address = 0;
	size = 0;
	mem_entry = NULL;
	mem_entry_port = IPC_PORT_NULL;
	sub_map = VM_MAP_NULL;

	                                             
	shared_region = kalloc(sizeof(*shared_region));
	if (shared_region == NULL) {
		SHARED_REGION_TRACE_ERROR(
			("shared_region: create: couldn't allocate\n"));
		goto done;
	}

	                                                                 
	if (is_64bit) {
		switch (cputype) {
#if defined(__arm64__)
		case CPU_TYPE_ARM64:
			base_address = SHARED_REGION_BASE_ARM64;
			size = SHARED_REGION_SIZE_ARM64;
			pmap_nesting_start = SHARED_REGION_NESTING_BASE_ARM64;
			pmap_nesting_size = SHARED_REGION_NESTING_SIZE_ARM64;
			break;
#elif !defined(__arm__)
		case CPU_TYPE_I386:
			base_address = SHARED_REGION_BASE_X86_64;
			size = SHARED_REGION_SIZE_X86_64;
			pmap_nesting_start = SHARED_REGION_NESTING_BASE_X86_64;
			pmap_nesting_size = SHARED_REGION_NESTING_SIZE_X86_64;
			break;
		case CPU_TYPE_POWERPC:
			base_address = SHARED_REGION_BASE_PPC64;
			size = SHARED_REGION_SIZE_PPC64;
			pmap_nesting_start = SHARED_REGION_NESTING_BASE_PPC64;
			pmap_nesting_size = SHARED_REGION_NESTING_SIZE_PPC64;
			break;
#endif
		default:
			SHARED_REGION_TRACE_ERROR(
				("shared_region: create: unknown cpu type %d\n",
				cputype));
			kfree(shared_region, sizeof(*shared_region));
			shared_region = NULL;
			goto done;
		}
	} else {
		switch (cputype) {
#if defined(__arm__) || defined(__arm64__)
		case CPU_TYPE_ARM:
			base_address = SHARED_REGION_BASE_ARM;
			size = SHARED_REGION_SIZE_ARM;
			pmap_nesting_start = SHARED_REGION_NESTING_BASE_ARM;
			pmap_nesting_size = SHARED_REGION_NESTING_SIZE_ARM;
			break;
#else
		case CPU_TYPE_I386:
			base_address = SHARED_REGION_BASE_I386;
			size = SHARED_REGION_SIZE_I386;
			pmap_nesting_start = SHARED_REGION_NESTING_BASE_I386;
			pmap_nesting_size = SHARED_REGION_NESTING_SIZE_I386;
			break;
		case CPU_TYPE_POWERPC:
			base_address = SHARED_REGION_BASE_PPC;
			size = SHARED_REGION_SIZE_PPC;
			pmap_nesting_start = SHARED_REGION_NESTING_BASE_PPC;
			pmap_nesting_size = SHARED_REGION_NESTING_SIZE_PPC;
			break;
#endif
		default:
			SHARED_REGION_TRACE_ERROR(
				("shared_region: create: unknown cpu type %d\n",
				cputype));
			kfree(shared_region, sizeof(*shared_region));
			shared_region = NULL;
			goto done;
		}
	}

	                                                            
	kr = mach_memory_entry_allocate(&mem_entry, &mem_entry_port);
	if (kr != KERN_SUCCESS) {
		kfree(shared_region, sizeof(*shared_region));
		shared_region = NULL;
		SHARED_REGION_TRACE_ERROR(
			("shared_region: create: "
			"couldn't allocate mem_entry\n"));
		goto done;
	}

#if     defined(__arm__) || defined(__arm64__)
	{
		struct pmap *pmap_nested;
		int pmap_flags = 0;
		pmap_flags |= is_64bit ? PMAP_CREATE_64BIT : 0;


		pmap_nested = pmap_create_options(NULL, 0, pmap_flags);
		if (pmap_nested != PMAP_NULL) {
			pmap_set_nested(pmap_nested);
			sub_map = vm_map_create(pmap_nested, 0, (vm_map_offset_t)size, TRUE);
#if defined(__arm64__)
			if (is_64bit ||
			    page_shift_user32 == SIXTEENK_PAGE_SHIFT) {
				                                              
				vm_map_set_page_shift(sub_map,
				    SIXTEENK_PAGE_SHIFT);
			}

#elif (__ARM_ARCH_7K__ >= 2)
			                                                           
			vm_map_set_page_shift(sub_map, SIXTEENK_PAGE_SHIFT);
#endif                
		} else {
			sub_map = VM_MAP_NULL;
		}
	}
#else
	                                      
	sub_map = vm_map_create(pmap_create_options(NULL, 0, is_64bit), 0, size, TRUE);
#endif
	if (sub_map == VM_MAP_NULL) {
		ipc_port_release_send(mem_entry_port);
		kfree(shared_region, sizeof(*shared_region));
		shared_region = NULL;
		SHARED_REGION_TRACE_ERROR(("shared_region: create: couldn't allocate map\n"));
		goto done;
	}

	                                                       
	vm_map_cs_enforcement_set(sub_map, true);
	assert(vm_map_cs_enforcement(sub_map));
	assert(pmap_get_vm_map_cs_enforced(vm_map_pmap(sub_map)));

	assert(!sub_map->disable_vmentry_reuse);
	sub_map->is_nested_map = TRUE;

	                                                   
	mem_entry->is_sub_map = TRUE;
	mem_entry->backing.map = sub_map;
	mem_entry->size = size;
	mem_entry->protection = VM_PROT_ALL;

	                                                      
	shared_region->sr_mem_entry = mem_entry_port;

	                                                          
	shared_region->sr_base_address = base_address;
	shared_region->sr_size = size;
	shared_region->sr_pmap_nesting_start = pmap_nesting_start;
	shared_region->sr_pmap_nesting_size = pmap_nesting_size;
	shared_region->sr_cpu_type = cputype;
	shared_region->sr_cpu_subtype = cpu_subtype;
	shared_region->sr_64bit = (uint8_t)is_64bit;
	shared_region->sr_root_dir = root_dir;

	queue_init(&shared_region->sr_q);
	shared_region->sr_mapping_in_progress = FALSE;
	shared_region->sr_slide_in_progress = FALSE;
	shared_region->sr_persists = FALSE;
	shared_region->sr_stale = FALSE;
	shared_region->sr_timer_call = NULL;
	shared_region->sr_first_mapping = (mach_vm_offset_t) -1;

	                                     
	shared_region->sr_ref_count = 1;

	shared_region->sr_slide = 0;                   

	                                        
	memset(&shared_region->sr_uuid, '\0', sizeof(shared_region->sr_uuid));
	shared_region->sr_uuid_copied = FALSE;
	shared_region->sr_images_count = 0;
	shared_region->sr_images = NULL;
#if __has_feature(ptrauth_calls)
	shared_region->sr_reslide = reslide;
	shared_region->sr_num_auth_section = 0;
	for (uint_t i = 0; i < NUM_SR_AUTH_SECTIONS; ++i) {
		shared_region->sr_auth_section[i] = NULL;
	}
	shared_region->sr_num_auth_section = 0;
#endif                                   

done:
	if (shared_region) {
		SHARED_REGION_TRACE_INFO(
			("shared_region: create(root=%p,cpu=<%d,%d>,64bit=%d,reslide=%d"
			"base=0x%llx,size=0x%llx) <- "
			"%p mem=(%p,%p) map=%p pmap=%p\n",
			(void *)VM_KERNEL_ADDRPERM(root_dir),
			cputype, cpu_subtype, is_64bit, reslide,
			(long long)base_address,
			(long long)size,
			(void *)VM_KERNEL_ADDRPERM(shared_region),
			(void *)VM_KERNEL_ADDRPERM(mem_entry_port),
			(void *)VM_KERNEL_ADDRPERM(mem_entry),
			(void *)VM_KERNEL_ADDRPERM(sub_map),
			(void *)VM_KERNEL_ADDRPERM(sub_map->pmap)));
	} else {
		SHARED_REGION_TRACE_INFO(
			("shared_region: create(root=%p,cpu=<%d,%d>,64bit=%d,"
			"base=0x%llx,size=0x%llx) <- NULL",
			(void *)VM_KERNEL_ADDRPERM(root_dir),
			cputype, cpu_subtype, is_64bit,
			(long long)base_address,
			(long long)size));
	}
	return shared_region;
}

  
                                      
                                                                        
   
static void
vm_shared_region_destroy(
	vm_shared_region_t      shared_region)
{
	vm_named_entry_t        mem_entry;
	vm_map_t                map;

	SHARED_REGION_TRACE_INFO(
		("shared_region: -> destroy(%p) (root=%p,cpu=<%d,%d>,64bit=%d)\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region),
		(void *)VM_KERNEL_ADDRPERM(shared_region->sr_root_dir),
		shared_region->sr_cpu_type,
		shared_region->sr_cpu_subtype,
		shared_region->sr_64bit));

	assert(shared_region->sr_ref_count == 0);
	assert(!shared_region->sr_persists);

	mem_entry = (vm_named_entry_t) ip_get_kobject(shared_region->sr_mem_entry);
	assert(mem_entry->is_sub_map);
	assert(!mem_entry->internal);
	assert(!mem_entry->is_copy);
	map = mem_entry->backing.map;

	  
                                                             
                                                                    
                                                                
                                                                     
                                                                        
                  
    
	if (map->pmap) {
		pmap_remove(map->pmap,
		    (vm_map_offset_t)shared_region->sr_base_address,
		    (vm_map_offset_t)(shared_region->sr_base_address + shared_region->sr_size));
	}

	  
                                                          
                                                                         
                                                                
                                                                     
                                     
    
	mach_memory_entry_port_release(shared_region->sr_mem_entry);
	mem_entry = NULL;
	shared_region->sr_mem_entry = IPC_PORT_NULL;

	if (shared_region->sr_timer_call) {
		thread_call_free(shared_region->sr_timer_call);
	}

#if __has_feature(ptrauth_calls)
	  
                                                              
    
	for (uint_t i = 0; i < shared_region->sr_num_auth_section; ++i) {
		vm_shared_region_slide_info_t si = shared_region->sr_auth_section[i];
		if (si != NULL) {
			vm_object_deallocate(si->si_slide_object);
			kheap_free(KHEAP_DATA_BUFFERS, si->si_slide_info_entry, si->si_slide_info_size);
			kfree(si, sizeof *si);
			shared_region->sr_auth_section[i] = NULL;
		}
	}
	shared_region->sr_num_auth_section = 0;
#endif                                   

	                                            
	kfree(shared_region, sizeof(*shared_region));

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: destroy(%p) <-\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region)));
	shared_region = NULL;
}

  
                                                                        
                                                                   
   
kern_return_t
vm_shared_region_start_address(
	vm_shared_region_t      shared_region,
	mach_vm_offset_t        *start_address,
	task_t                  task)
{
	kern_return_t           kr;
	mach_vm_offset_t        sr_base_address;
	mach_vm_offset_t        sr_first_mapping;

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: -> start_address(%p)\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region)));

	vm_shared_region_lock();

	  
                                                         
                                                         
                                           
    
	while (shared_region->sr_mapping_in_progress) {
		                          
		vm_shared_region_sleep(&shared_region->sr_mapping_in_progress,
		    THREAD_UNINT);
	}
	assert(!shared_region->sr_mapping_in_progress);
	assert(shared_region->sr_ref_count > 0);

	sr_base_address = shared_region->sr_base_address;
	sr_first_mapping = shared_region->sr_first_mapping;

	if (sr_first_mapping == (mach_vm_offset_t) -1) {
		                            
		kr = KERN_INVALID_ADDRESS;
	} else {
		kr = KERN_SUCCESS;
		*start_address = sr_base_address + sr_first_mapping;
	}


	uint32_t slide = shared_region->sr_slide;

	vm_shared_region_unlock();

	  
                                                                          
                                                                               
    
	if (kr == KERN_SUCCESS && task != NULL && task->task_shared_region_slide == -1) {
		uint_t sc_header_uuid_offset = offsetof(struct _dyld_cache_header, uuid);
		if (copyin((user_addr_t)(*start_address + sc_header_uuid_offset),
		    (char *)&task->task_shared_region_uuid,
		    sizeof(task->task_shared_region_uuid)) == 0) {
			task->task_shared_region_slide = slide;
		}
	}

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: start_address(%p) <- 0x%llx\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region),
		(long long)shared_region->sr_base_address));

	return kr;
}

  
                                                                    
                                            
   
static kern_return_t
find_mapping_to_slide(vm_map_t map, vm_map_address_t addr, vm_map_entry_t entry)
{
	vm_map_entry_t found;

	                                                 
	vm_map_lock_read(map);
	if (!vm_map_lookup_entry(map, addr, &found)) {
		                      
		vm_map_unlock(map);
		return KERN_INVALID_ARGUMENT;
	}

	*entry = *found;
	                                                          
	vm_object_reference(VME_OBJECT(found));
	vm_map_unlock_read(map);
	return KERN_SUCCESS;
}

#if __has_feature(ptrauth_calls)

  
                                                            
   
static boolean_t
task_sign_pointers(task_t task)
{
	if (task->map &&
	    task->map->pmap &&
	    !task->map->pmap->disable_jop) {
		return TRUE;
	}
	return FALSE;
}

  
                                                                      
                                        
  
                                                                    
                                                                         
                                                               
   
__attribute__((noinline))
kern_return_t
vm_shared_region_auth_remap(vm_shared_region_t sr)
{
	memory_object_t               sr_pager = MEMORY_OBJECT_NULL;
	task_t                        task = current_task();
	vm_shared_region_slide_info_t si;
	uint_t                        i;
	vm_object_t                   object;
	vm_map_t                      sr_map;
	struct vm_map_entry           tmp_entry_store = {0};
	vm_map_entry_t                tmp_entry = NULL;
	int                           vm_flags;
	vm_map_kernel_flags_t         vmk_flags;
	vm_map_offset_t               map_addr;
	kern_return_t                 kr = KERN_SUCCESS;
	boolean_t                     use_ptr_auth = task_sign_pointers(task);

	  
                                                                               
    
	vm_shared_region_lock();
	while (sr->sr_mapping_in_progress) {
		                          
		vm_shared_region_sleep(&sr->sr_mapping_in_progress, THREAD_UNINT);
	}
	assert(!sr->sr_mapping_in_progress);
	assert(sr->sr_ref_count > 0);

	                                  
	if (task->shared_region_auth_remapped) {
		vm_shared_region_unlock();
		return KERN_SUCCESS;
	}

	                                                                       
	sr->sr_mapping_in_progress = TRUE;
	vm_shared_region_unlock();

	  
                                                                         
    
	for (i = 0; i < sr->sr_num_auth_section; ++i) {
		si = sr->sr_auth_section[i];
		assert(si != NULL);
		assert(si->si_ptrauth);

		  
                                              
                                                            
                                                              
     
		object = si->si_slide_object;
		sr_pager = shared_region_pager_match(object, si->si_start, si,
		    use_ptr_auth ? task->jop_pid : 0);
		if (sr_pager == MEMORY_OBJECT_NULL) {
			kr = KERN_FAILURE;
			goto done;
		}

		  
                                                         
     
		if (use_ptr_auth) {
			shared_region_pager_match_task_key(sr_pager, task);
		}

		sr_map = vm_shared_region_vm_map(sr);
		tmp_entry = NULL;

		kr = find_mapping_to_slide(sr_map, si->si_slid_address - sr->sr_base_address, &tmp_entry_store);
		if (kr != KERN_SUCCESS) {
			goto done;
		}
		tmp_entry = &tmp_entry_store;

		  
                                                              
     
		if (tmp_entry->vme_end - tmp_entry->vme_start != si->si_end - si->si_start) {
			kr = KERN_FAILURE;
			goto done;
		}

		  
                                                                     
     
		vm_flags = VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE;
		vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
		vmk_flags.vmkf_overwrite_immutable = TRUE;
		map_addr = si->si_slid_address;
		kr = vm_map_enter_mem_object(task->map,
		    &map_addr,
		    si->si_end - si->si_start,
		    (mach_vm_offset_t) 0,
		    vm_flags,
		    vmk_flags,
		    VM_KERN_MEMORY_NONE,
		    (ipc_port_t)(uintptr_t) sr_pager,
		    0,
		    TRUE,
		    tmp_entry->protection,
		    tmp_entry->max_protection,
		    tmp_entry->inheritance);
		memory_object_deallocate(sr_pager);
		sr_pager = MEMORY_OBJECT_NULL;
		if (kr != KERN_SUCCESS) {
			goto done;
		}
		assertf(map_addr == si->si_slid_address,
		    "map_addr=0x%llx si_slid_address=0x%llx tmp_entry=%p\n",
		    (uint64_t)map_addr,
		    (uint64_t)si->si_slid_address,
		    tmp_entry);

		                                                         
		vm_object_deallocate(VME_OBJECT(tmp_entry));
		tmp_entry = NULL;
	}

done:
	if (tmp_entry) {
		                                                         
		vm_object_deallocate(VME_OBJECT(tmp_entry));
		tmp_entry = NULL;
	}

	  
                                                                                       
    
	if (sr_pager != MEMORY_OBJECT_NULL) {
		memory_object_deallocate(sr_pager);
	}

	  
                                                          
    
	vm_shared_region_lock();
	task->shared_region_auth_remapped = TRUE;
	sr->sr_mapping_in_progress = FALSE;
	thread_wakeup((event_t)&sr->sr_mapping_in_progress);
	vm_shared_region_unlock();
	return kr;
}
#endif                                   

void
vm_shared_region_undo_mappings(
	vm_map_t                 sr_map,
	mach_vm_offset_t         sr_base_address,
	struct _sr_file_mappings *srf_mappings,
	struct _sr_file_mappings *srf_mappings_current,
	unsigned int             srf_current_mappings_count)
{
	unsigned int             j = 0;
	vm_shared_region_t       shared_region = NULL;
	boolean_t                reset_shared_region_state = FALSE;
	struct _sr_file_mappings *srfmp;
	unsigned int             mappings_count;
	struct shared_file_mapping_slide_np *mappings;

	shared_region = vm_shared_region_get(current_task());
	if (shared_region == NULL) {
		printf("Failed to undo mappings because of NULL shared region.\n");
		return;
	}

	if (sr_map == NULL) {
		ipc_port_t              sr_handle;
		vm_named_entry_t        sr_mem_entry;

		vm_shared_region_lock();
		assert(shared_region->sr_ref_count > 0);

		while (shared_region->sr_mapping_in_progress) {
			                          
			vm_shared_region_sleep(&shared_region->sr_mapping_in_progress,
			    THREAD_UNINT);
		}
		assert(!shared_region->sr_mapping_in_progress);
		assert(shared_region->sr_ref_count > 0);
		                                                         
		shared_region->sr_mapping_in_progress = TRUE;

		vm_shared_region_unlock();

		reset_shared_region_state = TRUE;

		                                                            
		sr_handle = shared_region->sr_mem_entry;
		sr_mem_entry = (vm_named_entry_t) ip_get_kobject(sr_handle);
		sr_map = sr_mem_entry->backing.map;
		sr_base_address = shared_region->sr_base_address;
	}
	  
                                               
    
	for (srfmp = &srf_mappings[0];
	    srfmp <= srf_mappings_current;
	    srfmp++) {
		mappings = srfmp->mappings;
		mappings_count = srfmp->mappings_count;
		if (srfmp == srf_mappings_current) {
			mappings_count = srf_current_mappings_count;
		}

		for (j = 0; j < mappings_count; j++) {
			kern_return_t kr2;

			if (mappings[j].sms_size == 0) {
				  
                               
                                   
       
				continue;
			}
			SHARED_REGION_TRACE_INFO(
				("shared_region: mapping[%d]: "
				"address:0x%016llx "
				"size:0x%016llx "
				"offset:0x%016llx "
				"maxprot:0x%x prot:0x%x: "
				"undoing...\n",
				j,
				(long long)mappings[j].sms_address,
				(long long)mappings[j].sms_size,
				(long long)mappings[j].sms_file_offset,
				mappings[j].sms_max_prot,
				mappings[j].sms_init_prot));
			kr2 = mach_vm_deallocate(
				sr_map,
				(mappings[j].sms_address -
				sr_base_address),
				mappings[j].sms_size);
			assert(kr2 == KERN_SUCCESS);
		}
	}

	if (reset_shared_region_state) {
		vm_shared_region_lock();
		assert(shared_region->sr_ref_count > 0);
		assert(shared_region->sr_mapping_in_progress);
		                                              
		shared_region->sr_mapping_in_progress = FALSE;
		thread_wakeup((event_t) &shared_region->sr_mapping_in_progress);
		vm_shared_region_unlock();
		reset_shared_region_state = FALSE;
	}

	vm_shared_region_deallocate(shared_region);
}

  
                                                                           
                                                                                
                                                                    
   
#define VMSR_NUM_SLIDES 4

  
                                                          
                               
   
__attribute__((noinline))
static kern_return_t
vm_shared_region_map_file_setup(
	vm_shared_region_t              shared_region,
	int                             sr_file_mappings_count,
	struct _sr_file_mappings        *sr_file_mappings,
	unsigned int                    *mappings_to_slide_cnt,
	struct shared_file_mapping_slide_np **mappings_to_slide,
	mach_vm_offset_t                *slid_mappings,
	memory_object_control_t         *slid_file_controls,
	mach_vm_offset_t                *first_mapping,
	mach_vm_offset_t                *file_first_mappings,
	mach_vm_offset_t                *sfm_min_address,
	mach_vm_offset_t                *sfm_max_address,
	vm_map_t                        *sr_map_ptr,
	vm_map_offset_t                 *lowest_unnestable_addr_ptr)
{
	kern_return_t           kr = KERN_SUCCESS;
	memory_object_control_t file_control;
	vm_object_t             file_object;
	ipc_port_t              sr_handle;
	vm_named_entry_t        sr_mem_entry;
	vm_map_t                sr_map;
	mach_vm_offset_t        sr_base_address;
	unsigned int            i = 0;
	mach_port_t             map_port;
	vm_map_offset_t         target_address;
	vm_object_t             object;
	vm_object_size_t        obj_size;
	vm_map_offset_t         lowest_unnestable_addr = 0;
	vm_map_kernel_flags_t   vmk_flags;
	mach_vm_offset_t        sfm_end;
	uint32_t                mappings_count;
	struct shared_file_mapping_slide_np *mappings;
	struct _sr_file_mappings *srfmp;
	unsigned int            current_file_index = 0;

	vm_shared_region_lock();
	assert(shared_region->sr_ref_count > 0);

	  
                                                             
                                                             
                        
    
	while (shared_region->sr_mapping_in_progress) {
		                          
		vm_shared_region_sleep(&shared_region->sr_mapping_in_progress,
		    THREAD_UNINT);
	}
	assert(!shared_region->sr_mapping_in_progress);
	assert(shared_region->sr_ref_count > 0);
	                                                         
	shared_region->sr_mapping_in_progress = TRUE;

	vm_shared_region_unlock();

	                                                            
	sr_handle = shared_region->sr_mem_entry;
	sr_mem_entry = (vm_named_entry_t) ip_get_kobject(sr_handle);
	sr_map = sr_mem_entry->backing.map;
	sr_base_address = shared_region->sr_base_address;

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: -> map(%p)\n",
		(void *)VM_KERNEL_ADDRPERM(shared_region)));

	mappings_count = 0;
	mappings = NULL;
	srfmp = NULL;

	                                        
	for (srfmp = &sr_file_mappings[0];
	    srfmp < &sr_file_mappings[sr_file_mappings_count];
	    srfmp++) {
		mappings_count = srfmp->mappings_count;
		mappings = srfmp->mappings;
		file_control = srfmp->file_control;

		if (mappings_count == 0) {
			                         
			continue;
		}

		  
                                                                            
                                                                            
                                                                           
                          
     
		if (shared_region->sr_slide == 0 && srfmp->slide != 0) {
			shared_region->sr_slide = srfmp->slide;
		} else if (shared_region->sr_slide != 0 &&
		    srfmp->slide != 0 &&
		    shared_region->sr_slide != srfmp->slide) {
			SHARED_REGION_TRACE_ERROR(
				("shared_region: more than 1 non-zero slide value amount "
				"slide 1:0x%x slide 2:0x%x\n ",
				shared_region->sr_slide, srfmp->slide));
			kr = KERN_INVALID_ARGUMENT;
			break;
		}

#if __arm64__
		if ((shared_region->sr_64bit ||
		    page_shift_user32 == SIXTEENK_PAGE_SHIFT) &&
		    ((srfmp->slide & SIXTEENK_PAGE_MASK) != 0)) {
			printf("FOURK_COMPAT: %s: rejecting mis-aligned slide 0x%x\n",
			    __FUNCTION__, srfmp->slide);
			kr = KERN_INVALID_ARGUMENT;
			break;
		}
#endif                

		                                                             
		file_object = memory_object_control_to_vm_object(file_control);
		assert(file_object);

		                                          
		for (i = 0; i < mappings_count; i++) {
			SHARED_REGION_TRACE_INFO(
				("shared_region: mapping[%d]: "
				"address:0x%016llx size:0x%016llx offset:0x%016llx "
				"maxprot:0x%x prot:0x%x\n",
				i,
				(long long)mappings[i].sms_address,
				(long long)mappings[i].sms_size,
				(long long)mappings[i].sms_file_offset,
				mappings[i].sms_max_prot,
				mappings[i].sms_init_prot));

			if (mappings[i].sms_address < *sfm_min_address) {
				*sfm_min_address = mappings[i].sms_address;
			}

			if (os_add_overflow(mappings[i].sms_address,
			    mappings[i].sms_size,
			    &sfm_end) ||
			    (vm_map_round_page(sfm_end, VM_MAP_PAGE_MASK(sr_map)) <
			    mappings[i].sms_address)) {
				              
				kr = KERN_INVALID_ARGUMENT;
				break;
			}
			if (sfm_end > *sfm_max_address) {
				*sfm_max_address = sfm_end;
			}

			if (mappings[i].sms_init_prot & VM_PROT_ZF) {
				                        
				map_port = MACH_PORT_NULL;
			} else {
				                        
				__IGNORE_WCASTALIGN(map_port = (ipc_port_t) file_object->pager);
			}

			  
                                           
      
			if (mappings[i].sms_max_prot & VM_PROT_SLIDE) {
				if (*mappings_to_slide_cnt == VMSR_NUM_SLIDES) {
					SHARED_REGION_TRACE_INFO(
						("shared_region: mapping[%d]: "
						"address:0x%016llx size:0x%016llx "
						"offset:0x%016llx "
						"maxprot:0x%x prot:0x%x "
						"too many mappings to slide...\n",
						i,
						(long long)mappings[i].sms_address,
						(long long)mappings[i].sms_size,
						(long long)mappings[i].sms_file_offset,
						mappings[i].sms_max_prot,
						mappings[i].sms_init_prot));
				} else {
					mappings_to_slide[*mappings_to_slide_cnt] = &mappings[i];
					*mappings_to_slide_cnt += 1;
				}
			}

			                                                             
			target_address = (vm_map_offset_t)(mappings[i].sms_address - sr_base_address);

			vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
			vmk_flags.vmkf_already = TRUE;
			                                         
			vmk_flags.vmkf_no_copy_on_read = 1;


			                                                        
			if (map_port == MACH_PORT_NULL) {
				  
                                                               
                                                                                     
       
				obj_size = vm_map_round_page(mappings[i].sms_size, VM_MAP_PAGE_MASK(sr_map));
				object = vm_object_allocate(obj_size);
				if (object == VM_OBJECT_NULL) {
					kr = KERN_RESOURCE_SHORTAGE;
				} else {
					kr = vm_map_enter(
						sr_map,
						&target_address,
						vm_map_round_page(mappings[i].sms_size,
						VM_MAP_PAGE_MASK(sr_map)),
						0,
						VM_FLAGS_FIXED,
						vmk_flags,
						VM_KERN_MEMORY_NONE,
						object,
						0,
						TRUE,
						mappings[i].sms_init_prot & VM_PROT_ALL,
						mappings[i].sms_max_prot & VM_PROT_ALL,
						VM_INHERIT_DEFAULT);
				}
			} else {
				object = VM_OBJECT_NULL;                               
				kr = vm_map_enter_mem_object(
					sr_map,
					&target_address,
					vm_map_round_page(mappings[i].sms_size,
					VM_MAP_PAGE_MASK(sr_map)),
					0,
					VM_FLAGS_FIXED,
					vmk_flags,
					VM_KERN_MEMORY_NONE,
					map_port,
					mappings[i].sms_file_offset,
					TRUE,
					mappings[i].sms_init_prot & VM_PROT_ALL,
					mappings[i].sms_max_prot & VM_PROT_ALL,
					VM_INHERIT_DEFAULT);
			}

			if (kr == KERN_SUCCESS) {
				  
                                                    
                                     
                                                        
                                          
       
				assert(current_file_index < VMSR_NUM_SLIDES);
				if (file_first_mappings[current_file_index] == (mach_vm_offset_t) -1) {
					file_first_mappings[current_file_index] = target_address;
				}

				if (*mappings_to_slide_cnt > 0 &&
				    mappings_to_slide[*mappings_to_slide_cnt - 1] == &mappings[i]) {
					slid_mappings[*mappings_to_slide_cnt - 1] = target_address;
					slid_file_controls[*mappings_to_slide_cnt - 1] = file_control;
				}

				  
                                                 
                                                     
                                               
       
				if ((mappings[i].sms_init_prot & VM_PROT_WRITE) &&
				    sr_map->is_nested_map &&
				    (lowest_unnestable_addr == 0 ||
				    (target_address < lowest_unnestable_addr))) {
					lowest_unnestable_addr = target_address;
				}
			} else {
				if (map_port == MACH_PORT_NULL) {
					  
                                                
                          
        
					vm_object_deallocate(object);
					object = VM_OBJECT_NULL;
				}
				if (kr == KERN_MEMORY_PRESENT) {
					  
                                             
                    
        
					SHARED_REGION_TRACE_INFO(
						("shared_region: mapping[%d]: "
						"address:0x%016llx size:0x%016llx "
						"offset:0x%016llx "
						"maxprot:0x%x prot:0x%x "
						"already mapped...\n",
						i,
						(long long)mappings[i].sms_address,
						(long long)mappings[i].sms_size,
						(long long)mappings[i].sms_file_offset,
						mappings[i].sms_max_prot,
						mappings[i].sms_init_prot));
					  
                                                   
                                               
                                                    
        
					mappings[i].sms_size = 0;
					kr = KERN_SUCCESS;
				} else {
					break;
				}
			}
		}

		if (kr != KERN_SUCCESS) {
			break;
		}

		++current_file_index;
	}

	if (file_first_mappings[0] != (mach_vm_offset_t)-1) {
		*first_mapping = file_first_mappings[0];
	}


	if (kr != KERN_SUCCESS) {
		                                                      
		assert(i < mappings_count);
		SHARED_REGION_TRACE_ERROR(
			("shared_region: mapping[%d]: "
			"address:0x%016llx size:0x%016llx "
			"offset:0x%016llx "
			"maxprot:0x%x prot:0x%x failed 0x%x\n",
			i,
			(long long)mappings[i].sms_address,
			(long long)mappings[i].sms_size,
			(long long)mappings[i].sms_file_offset,
			mappings[i].sms_max_prot,
			mappings[i].sms_init_prot,
			kr));

		  
                                                         
                                                               
                                                                           
                                                                   
     
		assert(sr_map != NULL);
		                                    
		vm_shared_region_undo_mappings(sr_map, sr_base_address, sr_file_mappings, srfmp, i);
		return kr;
	}

	*lowest_unnestable_addr_ptr = lowest_unnestable_addr;
	*sr_map_ptr = sr_map;
	return KERN_SUCCESS;
}

                          
__attribute__((noinline))
static void
vm_shared_region_map_file_final(
	vm_shared_region_t shared_region,
	vm_map_t           sr_map,
	mach_vm_offset_t   sfm_min_address,
	mach_vm_offset_t   sfm_max_address,
	mach_vm_offset_t   *file_first_mappings);

  
                                                          
                                                                    
                                                                   
  
                                                                     
                                             
                                                                            
                                                                      
   
__attribute__((noinline))
kern_return_t
vm_shared_region_map_file(
	vm_shared_region_t       shared_region,
	int                      sr_file_mappings_count,
	struct _sr_file_mappings *sr_file_mappings)
{
	kern_return_t           kr = KERN_SUCCESS;
	unsigned int            i;
	unsigned int            mappings_to_slide_cnt = 0;
	struct shared_file_mapping_slide_np *mappings_to_slide[VMSR_NUM_SLIDES] = {};
	mach_vm_offset_t        slid_mappings[VMSR_NUM_SLIDES];
	memory_object_control_t slid_file_controls[VMSR_NUM_SLIDES];
	mach_vm_offset_t        first_mapping = (mach_vm_offset_t)-1;
	mach_vm_offset_t        sfm_min_address = (mach_vm_offset_t)-1;
	mach_vm_offset_t        sfm_max_address = 0;
	vm_map_t                sr_map = NULL;
	vm_map_offset_t         lowest_unnestable_addr = 0;
	mach_vm_offset_t        file_first_mappings[VMSR_NUM_SLIDES];
	for (i = 0; i < VMSR_NUM_SLIDES; ++i) {
		file_first_mappings[i] = (mach_vm_offset_t) -1;
	}

	kr = vm_shared_region_map_file_setup(shared_region, sr_file_mappings_count, sr_file_mappings,
	    &mappings_to_slide_cnt, &mappings_to_slide[0], slid_mappings, slid_file_controls,
	    &first_mapping, &file_first_mappings[0],
	    &sfm_min_address, &sfm_max_address, &sr_map, &lowest_unnestable_addr);
	if (kr != KERN_SUCCESS) {
		vm_shared_region_lock();
		goto done;
	}

	  
                                                                      
                                                                  
                                       
    
	for (i = 0; i < mappings_to_slide_cnt; ++i) {
		kr = vm_shared_region_slide(shared_region->sr_slide,
		    mappings_to_slide[i]->sms_file_offset,
		    mappings_to_slide[i]->sms_size,
		    mappings_to_slide[i]->sms_slide_start,
		    mappings_to_slide[i]->sms_slide_size,
		    slid_mappings[i],
		    slid_file_controls[i],
		    mappings_to_slide[i]->sms_max_prot);
		if (kr != KERN_SUCCESS) {
			SHARED_REGION_TRACE_ERROR(
				("shared_region: region_slide("
				"slide:0x%x start:0x%016llx "
				"size:0x%016llx) failed 0x%x\n",
				shared_region->sr_slide,
				(long long)mappings_to_slide[i]->sms_slide_start,
				(long long)mappings_to_slide[i]->sms_slide_size,
				kr));
			vm_shared_region_lock();
			goto done;
		}
	}

	assert(kr == KERN_SUCCESS);

	                                                
	lowest_unnestable_addr &= ~(pmap_shared_region_size_min(sr_map->pmap) - 1);
	if (lowest_unnestable_addr != sr_map->lowest_unnestable_start) {
		vm_map_lock(sr_map);
		sr_map->lowest_unnestable_start = lowest_unnestable_addr;
		vm_map_unlock(sr_map);
	}

	vm_shared_region_lock();
	assert(shared_region->sr_ref_count > 0);
	assert(shared_region->sr_mapping_in_progress);

	                                                                       
	if (shared_region->sr_first_mapping == (mach_vm_offset_t) -1) {
		shared_region->sr_first_mapping = first_mapping;
	}

	vm_shared_region_map_file_final(shared_region, sr_map, sfm_min_address, sfm_max_address,
	    &file_first_mappings[0]);

done:
	  
                                             
                                
    
	shared_region->sr_mapping_in_progress = FALSE;
	thread_wakeup((event_t) &shared_region->sr_mapping_in_progress);
	vm_shared_region_unlock();

#if __has_feature(ptrauth_calls)
	if (kr == KERN_SUCCESS) {
		  
                                                                       
                                                                
     
		kr = vm_shared_region_auth_remap(shared_region);
	}
#endif                                   

	                                                               
	task_t task;
	if (kr == KERN_SUCCESS && (task = current_task())->task_shared_region_slide == -1) {
		mach_vm_offset_t start_address;
		(void)vm_shared_region_start_address(shared_region, &start_address, task);
	}

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: map(%p) <- 0x%x \n",
		(void *)VM_KERNEL_ADDRPERM(shared_region), kr));
	return kr;
}

  
                                             
                                                            
   
__attribute__((noinline))
static void
vm_shared_region_map_file_final(
	vm_shared_region_t        shared_region,
	vm_map_t                  sr_map,
	mach_vm_offset_t          sfm_min_address,
	mach_vm_offset_t          sfm_max_address,
	__unused mach_vm_offset_t *file_first_mappings)
{
	struct _dyld_cache_header sr_cache_header;
	int                       error;
	size_t                    image_array_length;
	struct _dyld_cache_image_text_info *sr_image_layout;
	boolean_t                 locally_built = FALSE;


	  
                                                                  
                                                                     
                                                                   
                                                  
    
	if (!shared_region->sr_uuid_copied) {
		error = copyin((user_addr_t)(shared_region->sr_base_address + shared_region->sr_first_mapping),
		    (char *)&sr_cache_header,
		    sizeof(sr_cache_header));
		if (error == 0) {
			memcpy(&shared_region->sr_uuid, &sr_cache_header.uuid, sizeof(shared_region->sr_uuid));
			shared_region->sr_uuid_copied = TRUE;
			locally_built = sr_cache_header.locallyBuiltCache;
		} else {
#if DEVELOPMENT || DEBUG
			panic("shared_region: copyin shared_cache_header(sr_base_addr:0x%016llx sr_first_mapping:0x%016llx "
			    "offset:0 size:0x%016llx) failed with %d\n",
			    (long long)shared_region->sr_base_address,
			    (long long)shared_region->sr_first_mapping,
			    (long long)sizeof(sr_cache_header),
			    error);
#endif                           
			shared_region->sr_uuid_copied = FALSE;
		}
	}

	  
                                                                                                   
                                                                                                           
                                                                                                          
                                                                                                       
                                
   
                                                                                                         
    
	bool is_init_task = (task_pid(current_task()) == 1);
	if (shared_region->sr_uuid_copied && is_init_task) {
		                                                                                        
		if (locally_built) {
			KDBG((MACHDBG_CODE(DBG_MACH_SHAREDREGION, PROCESS_SHARED_CACHE_LAYOUT)) | DBG_FUNC_START);
			image_array_length = (size_t)(sr_cache_header.imagesTextCount * sizeof(struct _dyld_cache_image_text_info));
			sr_image_layout = kheap_alloc(KHEAP_DATA_BUFFERS, image_array_length, Z_WAITOK);
			error = copyin((user_addr_t)(shared_region->sr_base_address + shared_region->sr_first_mapping +
			    sr_cache_header.imagesTextOffset), (char *)sr_image_layout, image_array_length);
			if (error == 0) {
				if (sr_cache_header.imagesTextCount >= UINT32_MAX) {
					panic("shared_region: sr_cache_header.imagesTextCount >= UINT32_MAX");
				}
				shared_region->sr_images = kalloc((vm_size_t)(sr_cache_header.imagesTextCount * sizeof(struct dyld_uuid_info_64)));
				for (size_t index = 0; index < sr_cache_header.imagesTextCount; index++) {
					memcpy((char *)&shared_region->sr_images[index].imageUUID, (char *)&sr_image_layout[index].uuid,
					    sizeof(shared_region->sr_images[index].imageUUID));
					shared_region->sr_images[index].imageLoadAddress = sr_image_layout[index].loadAddress;
				}

				shared_region->sr_images_count = (uint32_t) sr_cache_header.imagesTextCount;
			} else {
#if DEVELOPMENT || DEBUG
				panic("shared_region: copyin shared_cache_layout(sr_base_addr:0x%016llx sr_first_mapping:0x%016llx "
				    "offset:0x%016llx size:0x%016llx) failed with %d\n",
				    (long long)shared_region->sr_base_address,
				    (long long)shared_region->sr_first_mapping,
				    (long long)sr_cache_header.imagesTextOffset,
				    (long long)image_array_length,
				    error);
#endif                           
			}
			KDBG((MACHDBG_CODE(DBG_MACH_SHAREDREGION, PROCESS_SHARED_CACHE_LAYOUT)) | DBG_FUNC_END, shared_region->sr_images_count);
			kheap_free(KHEAP_DATA_BUFFERS, sr_image_layout, image_array_length);
			sr_image_layout = NULL;
		}
		primary_system_shared_region = shared_region;
	}

	  
                                                             
                                                             
                   
    
	if (VM_MAP_PAGE_SHIFT(current_map()) == VM_MAP_PAGE_SHIFT(sr_map)) {
		pmap_trim(current_map()->pmap, sr_map->pmap, sfm_min_address, sfm_max_address - sfm_min_address);
	}
}

  
                                                                 
                                                               
                                                                  
             
  
                                                          
   
vm_shared_region_t
vm_shared_region_trim_and_get(task_t task)
{
	vm_shared_region_t shared_region;
	ipc_port_t sr_handle;
	vm_named_entry_t sr_mem_entry;
	vm_map_t sr_map;

	                                        
	shared_region = vm_shared_region_get(task);
	if (shared_region == NULL) {
		return NULL;
	}

	sr_handle = shared_region->sr_mem_entry;
	sr_mem_entry = (vm_named_entry_t) ip_get_kobject(sr_handle);
	sr_map = sr_mem_entry->backing.map;

	                                
	if (VM_MAP_PAGE_SHIFT(task->map) == VM_MAP_PAGE_SHIFT(sr_map)) {
		pmap_trim(task->map->pmap, sr_map->pmap, 0, 0);
	}

	return shared_region;
}

  
                                                             
                                                                          
                                                                         
                                                               
   
kern_return_t
vm_shared_region_enter(
	struct _vm_map          *map,
	struct task             *task,
	boolean_t               is_64bit,
	void                    *fsroot,
	cpu_type_t              cpu,
	cpu_subtype_t           cpu_subtype,
	boolean_t               reslide)
{
	kern_return_t           kr;
	vm_shared_region_t      shared_region;
	vm_map_offset_t         sr_address, sr_offset, target_address;
	vm_map_size_t           sr_size, mapping_size;
	vm_map_offset_t         sr_pmap_nesting_start;
	vm_map_size_t           sr_pmap_nesting_size;
	ipc_port_t              sr_handle;
	vm_prot_t               cur_prot, max_prot;

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: -> "
		"enter(map=%p,task=%p,root=%p,cpu=<%d,%d>,64bit=%d)\n",
		(void *)VM_KERNEL_ADDRPERM(map),
		(void *)VM_KERNEL_ADDRPERM(task),
		(void *)VM_KERNEL_ADDRPERM(fsroot),
		cpu, cpu_subtype, is_64bit));

	                                                                      
	shared_region = vm_shared_region_lookup(fsroot, cpu, cpu_subtype, is_64bit, reslide);
	if (shared_region == NULL) {
		                              
		SHARED_REGION_TRACE_ERROR(
			("shared_region: -> "
			"enter(map=%p,task=%p,root=%p,cpu=<%d,%d>,64bit=%d,reslide=%d): "
			"lookup failed !\n",
			(void *)VM_KERNEL_ADDRPERM(map),
			(void *)VM_KERNEL_ADDRPERM(task),
			(void *)VM_KERNEL_ADDRPERM(fsroot),
			cpu, cpu_subtype, is_64bit, reslide));
		                                                
		return KERN_FAILURE;
	}

	kr = KERN_SUCCESS;
	                                                       
	sr_address = (vm_map_offset_t)shared_region->sr_base_address;
	sr_size = (vm_map_size_t)shared_region->sr_size;
	sr_handle = shared_region->sr_mem_entry;
	sr_pmap_nesting_start = (vm_map_offset_t)shared_region->sr_pmap_nesting_start;
	sr_pmap_nesting_size = (vm_map_size_t)shared_region->sr_pmap_nesting_size;

	cur_prot = VM_PROT_READ;
	if (VM_MAP_POLICY_WRITABLE_SHARED_REGION(map)) {
		  
                             
                                                      
                                                     
                    
     
		max_prot = VM_PROT_ALL;
	} else {
		max_prot = VM_PROT_READ;
	}

	  
                                                                        
    
	sr_offset = 0;

	if (sr_pmap_nesting_start > sr_address) {
		                                                       
		target_address = sr_address;
		mapping_size = sr_pmap_nesting_start - sr_address;
		kr = vm_map_enter_mem_object(
			map,
			&target_address,
			mapping_size,
			0,
			VM_FLAGS_FIXED,
			VM_MAP_KERNEL_FLAGS_NONE,
			VM_KERN_MEMORY_NONE,
			sr_handle,
			sr_offset,
			TRUE,
			cur_prot,
			max_prot,
			VM_INHERIT_SHARE);
		if (kr != KERN_SUCCESS) {
			SHARED_REGION_TRACE_ERROR(
				("shared_region: enter(%p,%p,%p,%d,%d,%d): "
				"vm_map_enter(0x%llx,0x%llx,%p) error 0x%x\n",
				(void *)VM_KERNEL_ADDRPERM(map),
				(void *)VM_KERNEL_ADDRPERM(task),
				(void *)VM_KERNEL_ADDRPERM(fsroot),
				cpu, cpu_subtype, is_64bit,
				(long long)target_address,
				(long long)mapping_size,
				(void *)VM_KERNEL_ADDRPERM(sr_handle), kr));
			goto done;
		}
		SHARED_REGION_TRACE_DEBUG(
			("shared_region: enter(%p,%p,%p,%d,%d,%d): "
			"vm_map_enter(0x%llx,0x%llx,%p) error 0x%x\n",
			(void *)VM_KERNEL_ADDRPERM(map),
			(void *)VM_KERNEL_ADDRPERM(task),
			(void *)VM_KERNEL_ADDRPERM(fsroot),
			cpu, cpu_subtype, is_64bit,
			(long long)target_address, (long long)mapping_size,
			(void *)VM_KERNEL_ADDRPERM(sr_handle), kr));
		sr_offset += mapping_size;
		sr_size -= mapping_size;
	}
	  
                                                                    
                                          
                                                                   
    
	for (;
	    sr_pmap_nesting_size > 0;
	    sr_offset += mapping_size,
	    sr_size -= mapping_size,
	    sr_pmap_nesting_size -= mapping_size) {
		vm_map_kernel_flags_t vmk_flags;

		target_address = sr_address + sr_offset;
		mapping_size = sr_pmap_nesting_size;
		if (mapping_size > pmap_nesting_size_max(map->pmap)) {
			mapping_size = (vm_map_offset_t) pmap_nesting_size_max(map->pmap);
		}
		vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
		vmk_flags.vmkf_nested_pmap = TRUE;
		kr = vm_map_enter_mem_object(
			map,
			&target_address,
			mapping_size,
			0,
			VM_FLAGS_FIXED,
			vmk_flags,
			VM_MEMORY_SHARED_PMAP,
			sr_handle,
			sr_offset,
			TRUE,
			cur_prot,
			max_prot,
			VM_INHERIT_SHARE);
		if (kr != KERN_SUCCESS) {
			SHARED_REGION_TRACE_ERROR(
				("shared_region: enter(%p,%p,%p,%d,%d,%d): "
				"vm_map_enter(0x%llx,0x%llx,%p) error 0x%x\n",
				(void *)VM_KERNEL_ADDRPERM(map),
				(void *)VM_KERNEL_ADDRPERM(task),
				(void *)VM_KERNEL_ADDRPERM(fsroot),
				cpu, cpu_subtype, is_64bit,
				(long long)target_address,
				(long long)mapping_size,
				(void *)VM_KERNEL_ADDRPERM(sr_handle), kr));
			goto done;
		}
		SHARED_REGION_TRACE_DEBUG(
			("shared_region: enter(%p,%p,%p,%d,%d,%d): "
			"nested vm_map_enter(0x%llx,0x%llx,%p) error 0x%x\n",
			(void *)VM_KERNEL_ADDRPERM(map),
			(void *)VM_KERNEL_ADDRPERM(task),
			(void *)VM_KERNEL_ADDRPERM(fsroot),
			cpu, cpu_subtype, is_64bit,
			(long long)target_address, (long long)mapping_size,
			(void *)VM_KERNEL_ADDRPERM(sr_handle), kr));
	}
	if (sr_size > 0) {
		                                                             
		target_address = sr_address + sr_offset;
		mapping_size = sr_size;
		kr = vm_map_enter_mem_object(
			map,
			&target_address,
			mapping_size,
			0,
			VM_FLAGS_FIXED,
			VM_MAP_KERNEL_FLAGS_NONE,
			VM_KERN_MEMORY_NONE,
			sr_handle,
			sr_offset,
			TRUE,
			cur_prot,
			max_prot,
			VM_INHERIT_SHARE);
		if (kr != KERN_SUCCESS) {
			SHARED_REGION_TRACE_ERROR(
				("shared_region: enter(%p,%p,%p,%d,%d,%d): "
				"vm_map_enter(0x%llx,0x%llx,%p) error 0x%x\n",
				(void *)VM_KERNEL_ADDRPERM(map),
				(void *)VM_KERNEL_ADDRPERM(task),
				(void *)VM_KERNEL_ADDRPERM(fsroot),
				cpu, cpu_subtype, is_64bit,
				(long long)target_address,
				(long long)mapping_size,
				(void *)VM_KERNEL_ADDRPERM(sr_handle), kr));
			goto done;
		}
		SHARED_REGION_TRACE_DEBUG(
			("shared_region: enter(%p,%p,%p,%d,%d,%d): "
			"vm_map_enter(0x%llx,0x%llx,%p) error 0x%x\n",
			(void *)VM_KERNEL_ADDRPERM(map),
			(void *)VM_KERNEL_ADDRPERM(task),
			(void *)VM_KERNEL_ADDRPERM(fsroot),
			cpu, cpu_subtype, is_64bit,
			(long long)target_address, (long long)mapping_size,
			(void *)VM_KERNEL_ADDRPERM(sr_handle), kr));
		sr_offset += mapping_size;
		sr_size -= mapping_size;
	}
	assert(sr_size == 0);

done:
	if (kr == KERN_SUCCESS) {
		                                         
		vm_shared_region_set(task, shared_region);
	} else {
		                                                 
		vm_shared_region_deallocate(shared_region);
		vm_shared_region_set(task, NULL);
	}

	SHARED_REGION_TRACE_DEBUG(
		("shared_region: enter(%p,%p,%p,%d,%d,%d) <- 0x%x\n",
		(void *)VM_KERNEL_ADDRPERM(map),
		(void *)VM_KERNEL_ADDRPERM(task),
		(void *)VM_KERNEL_ADDRPERM(fsroot),
		cpu, cpu_subtype, is_64bit,
		kr));
	return kr;
}

#define SANE_SLIDE_INFO_SIZE            (2560*1024)                             
struct vm_shared_region_slide_info      slide_info;

kern_return_t
vm_shared_region_sliding_valid(uint32_t slide)
{
	kern_return_t kr = KERN_SUCCESS;
	vm_shared_region_t sr = vm_shared_region_get(current_task());

	                                
	if (sr == NULL) {
		return kr;
	}

	if (sr->sr_slide != 0 && slide != 0) {
		if (slide == sr->sr_slide) {
			  
                                    
                                      
                              
                                      
                                      
                              
      
			kr = KERN_INVALID_ARGUMENT;
		} else {
			printf("Mismatched shared region slide\n");
			kr = KERN_FAILURE;
		}
	}
	vm_shared_region_deallocate(sr);
	return kr;
}

  
                                                                                   
                                                                                 
                                                                                 
                                                                                
                    
   
kern_return_t
vm_shared_region_slide_mapping(
	vm_shared_region_t      sr,
	user_addr_t             slide_info_addr,
	mach_vm_size_t          slide_info_size,
	mach_vm_offset_t        start,
	mach_vm_size_t          size,
	mach_vm_offset_t        slid_mapping,
	uint32_t                slide,
	memory_object_control_t sr_file_control,
	vm_prot_t               prot)
{
	kern_return_t           kr;
	vm_object_t             object = VM_OBJECT_NULL;
	vm_shared_region_slide_info_t si = NULL;
	vm_map_entry_t          tmp_entry = VM_MAP_ENTRY_NULL;
	struct vm_map_entry     tmp_entry_store;
	memory_object_t         sr_pager = MEMORY_OBJECT_NULL;
	vm_map_t                sr_map;
	int                     vm_flags;
	vm_map_kernel_flags_t   vmk_flags;
	vm_map_offset_t         map_addr;
	void                    *slide_info_entry = NULL;
	int                     error;

	assert(sr->sr_slide_in_progress);

	if (sr_file_control == MEMORY_OBJECT_CONTROL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	  
                                                  
    
	if (slide_info_size < MIN_SLIDE_INFO_SIZE) {
		printf("Slide_info_size too small: %lx\n", (uintptr_t)slide_info_size);
		return KERN_FAILURE;
	}
	if (slide_info_size > SANE_SLIDE_INFO_SIZE) {
		printf("Slide_info_size too large: %lx\n", (uintptr_t)slide_info_size);
		return KERN_FAILURE;
	}

	slide_info_entry = kheap_alloc(KHEAP_DATA_BUFFERS, (vm_size_t)slide_info_size, Z_WAITOK);
	if (slide_info_entry == NULL) {
		return KERN_RESOURCE_SHORTAGE;
	}
	error = copyin(slide_info_addr, slide_info_entry, (size_t)slide_info_size);
	if (error) {
		printf("copyin of slide_info failed\n");
		kr = KERN_INVALID_ADDRESS;
		goto done;
	}

	if ((kr = vm_shared_region_slide_sanity_check(slide_info_entry, slide_info_size)) != KERN_SUCCESS) {
		printf("Sanity Check failed for slide_info\n");
		goto done;
	}

	  
                                                       
                                                            
                                  
    
	object = memory_object_control_to_vm_object(sr_file_control);
	if (object == VM_OBJECT_NULL || object->internal) {
		object = VM_OBJECT_NULL;
		kr = KERN_INVALID_ADDRESS;
		goto done;
	}

	si = kalloc(sizeof(*si));
	if (si == NULL) {
		kr = KERN_RESOURCE_SHORTAGE;
		goto done;
	}
	vm_object_lock(object);

	vm_object_reference_locked(object);                               
	object->object_is_shared_cache = TRUE;
	vm_object_unlock(object);

	si->si_slide_info_entry = slide_info_entry;
	si->si_slide_info_size = slide_info_size;

	assert(slid_mapping != (mach_vm_offset_t) -1);
	si->si_slid_address = slid_mapping + sr->sr_base_address;
	si->si_slide_object = object;
	si->si_start = start;
	si->si_end = si->si_start + size;
	si->si_slide = slide;
#if __has_feature(ptrauth_calls)
	  
                                                                
                                                                 
                                       
    
	if (sr->sr_cpu_type == CPU_TYPE_ARM64 &&
	    sr->sr_cpu_subtype == CPU_SUBTYPE_ARM64E &&
	    !(prot & VM_PROT_NOAUTH)) {
		if (sr->sr_num_auth_section == NUM_SR_AUTH_SECTIONS) {
			printf("Too many auth/private sections for shared region!!\n");
			kr = KERN_INVALID_ARGUMENT;
			goto done;
		}
		si->si_ptrauth = TRUE;
		sr->sr_auth_section[sr->sr_num_auth_section++] = si;
		  
                                                         
                                                             
                                         
     
		si->si_shared_region = sr;
		kr = KERN_SUCCESS;
		goto done;
	}
	si->si_shared_region = NULL;
	si->si_ptrauth = FALSE;
#else                                   
	(void)prot;                                 
#endif                                   

	  
                                                            
    
	sr_map = vm_shared_region_vm_map(sr);
	kr = find_mapping_to_slide(sr_map, (vm_map_address_t)slid_mapping, &tmp_entry_store);
	if (kr != KERN_SUCCESS) {
		goto done;
	}
	tmp_entry = &tmp_entry_store;

	  
                                                      
    
	assert(VME_OFFSET(tmp_entry) == start);
	assert(tmp_entry->vme_end - tmp_entry->vme_start == size);

	                                            
	sr_pager = shared_region_pager_setup(VME_OBJECT(tmp_entry), VME_OFFSET(tmp_entry), si, 0);
	if (sr_pager == MEMORY_OBJECT_NULL) {
		kr = KERN_RESOURCE_SHORTAGE;
		goto done;
	}

	                                                                       
	vm_flags = VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE;
	vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
	vmk_flags.vmkf_overwrite_immutable = TRUE;
	map_addr = tmp_entry->vme_start;
	kr = vm_map_enter_mem_object(sr_map,
	    &map_addr,
	    (tmp_entry->vme_end - tmp_entry->vme_start),
	    (mach_vm_offset_t) 0,
	    vm_flags,
	    vmk_flags,
	    VM_KERN_MEMORY_NONE,
	    (ipc_port_t)(uintptr_t) sr_pager,
	    0,
	    TRUE,
	    tmp_entry->protection,
	    tmp_entry->max_protection,
	    tmp_entry->inheritance);
	assertf(kr == KERN_SUCCESS, "kr = 0x%x\n", kr);
	assertf(map_addr == tmp_entry->vme_start,
	    "map_addr=0x%llx vme_start=0x%llx tmp_entry=%p\n",
	    (uint64_t)map_addr,
	    (uint64_t) tmp_entry->vme_start,
	    tmp_entry);

	              
	kr = KERN_SUCCESS;

done:
	if (sr_pager != NULL) {
		  
                                                                            
                                                                                   
     
		memory_object_deallocate(sr_pager);
		sr_pager = MEMORY_OBJECT_NULL;
	}
	if (tmp_entry != NULL) {
		                                                
		vm_object_deallocate(VME_OBJECT(tmp_entry));
		tmp_entry = VM_MAP_ENTRY_NULL;
	}

	if (kr != KERN_SUCCESS) {
		             
		if (si != NULL) {
			if (si->si_slide_object) {
				vm_object_deallocate(si->si_slide_object);
				si->si_slide_object = VM_OBJECT_NULL;
			}
			kfree(si, sizeof(*si));
			si = NULL;
		}
		if (slide_info_entry != NULL) {
			kheap_free(KHEAP_DATA_BUFFERS, slide_info_entry, (vm_size_t)slide_info_size);
			slide_info_entry = NULL;
		}
	}
	return kr;
}

static kern_return_t
vm_shared_region_slide_sanity_check_v2(
	vm_shared_region_slide_info_entry_v2_t s_info,
	mach_vm_size_t slide_info_size)
{
	if (slide_info_size < sizeof(struct vm_shared_region_slide_info_entry_v2)) {
		printf("%s bad slide_info_size: %lx\n", __func__, (uintptr_t)slide_info_size);
		return KERN_FAILURE;
	}
	if (s_info->page_size != PAGE_SIZE_FOR_SR_SLIDE) {
		return KERN_FAILURE;
	}

	                                                                                  

	uint32_t page_starts_count = s_info->page_starts_count;
	uint32_t page_extras_count = s_info->page_extras_count;
	mach_vm_size_t num_trailing_entries = page_starts_count + page_extras_count;
	if (num_trailing_entries < page_starts_count) {
		return KERN_FAILURE;
	}

	                                                                                    
	mach_vm_size_t trailing_size = num_trailing_entries << 1;
	if (trailing_size >> 1 != num_trailing_entries) {
		return KERN_FAILURE;
	}

	mach_vm_size_t required_size = sizeof(*s_info) + trailing_size;
	if (required_size < sizeof(*s_info)) {
		return KERN_FAILURE;
	}

	if (required_size > slide_info_size) {
		return KERN_FAILURE;
	}

	return KERN_SUCCESS;
}

static kern_return_t
vm_shared_region_slide_sanity_check_v3(
	vm_shared_region_slide_info_entry_v3_t s_info,
	mach_vm_size_t slide_info_size)
{
	if (slide_info_size < sizeof(struct vm_shared_region_slide_info_entry_v3)) {
		printf("%s bad slide_info_size: %lx\n", __func__, (uintptr_t)slide_info_size);
		return KERN_FAILURE;
	}
	if (s_info->page_size != PAGE_SIZE_FOR_SR_SLIDE) {
		printf("vm_shared_region_slide_sanity_check_v3: s_info->page_size != PAGE_SIZE_FOR_SR_SL 0x%llx != 0x%llx\n", (uint64_t)s_info->page_size, (uint64_t)PAGE_SIZE_FOR_SR_SLIDE);
		return KERN_FAILURE;
	}

	uint32_t page_starts_count = s_info->page_starts_count;
	mach_vm_size_t num_trailing_entries = page_starts_count;
	mach_vm_size_t trailing_size = num_trailing_entries << 1;
	mach_vm_size_t required_size = sizeof(*s_info) + trailing_size;
	if (required_size < sizeof(*s_info)) {
		printf("vm_shared_region_slide_sanity_check_v3: required_size != sizeof(*s_info) 0x%llx != 0x%llx\n", (uint64_t)required_size, (uint64_t)sizeof(*s_info));
		return KERN_FAILURE;
	}

	if (required_size > slide_info_size) {
		printf("vm_shared_region_slide_sanity_check_v3: required_size != slide_info_size 0x%llx != 0x%llx\n", (uint64_t)required_size, (uint64_t)slide_info_size);
		return KERN_FAILURE;
	}

	return KERN_SUCCESS;
}

static kern_return_t
vm_shared_region_slide_sanity_check_v4(
	vm_shared_region_slide_info_entry_v4_t s_info,
	mach_vm_size_t slide_info_size)
{
	if (slide_info_size < sizeof(struct vm_shared_region_slide_info_entry_v4)) {
		printf("%s bad slide_info_size: %lx\n", __func__, (uintptr_t)slide_info_size);
		return KERN_FAILURE;
	}
	if (s_info->page_size != PAGE_SIZE_FOR_SR_SLIDE) {
		return KERN_FAILURE;
	}

	                                                                                  

	uint32_t page_starts_count = s_info->page_starts_count;
	uint32_t page_extras_count = s_info->page_extras_count;
	mach_vm_size_t num_trailing_entries = page_starts_count + page_extras_count;
	if (num_trailing_entries < page_starts_count) {
		return KERN_FAILURE;
	}

	                                                                                    
	mach_vm_size_t trailing_size = num_trailing_entries << 1;
	if (trailing_size >> 1 != num_trailing_entries) {
		return KERN_FAILURE;
	}

	mach_vm_size_t required_size = sizeof(*s_info) + trailing_size;
	if (required_size < sizeof(*s_info)) {
		return KERN_FAILURE;
	}

	if (required_size > slide_info_size) {
		return KERN_FAILURE;
	}

	return KERN_SUCCESS;
}


static kern_return_t
vm_shared_region_slide_sanity_check(
	vm_shared_region_slide_info_entry_t s_info,
	mach_vm_size_t s_info_size)
{
	kern_return_t kr;

	switch (s_info->version) {
	case 2:
		kr = vm_shared_region_slide_sanity_check_v2(&s_info->v2, s_info_size);
		break;
	case 3:
		kr = vm_shared_region_slide_sanity_check_v3(&s_info->v3, s_info_size);
		break;
	case 4:
		kr = vm_shared_region_slide_sanity_check_v4(&s_info->v4, s_info_size);
		break;
	default:
		kr = KERN_FAILURE;
	}
	return kr;
}

static kern_return_t
rebase_chain_32(
	uint8_t *page_content,
	uint16_t start_offset,
	uint32_t slide_amount,
	vm_shared_region_slide_info_entry_v2_t s_info)
{
	const uint32_t last_page_offset = PAGE_SIZE_FOR_SR_SLIDE - sizeof(uint32_t);

	const uint32_t delta_mask = (uint32_t)(s_info->delta_mask);
	const uint32_t value_mask = ~delta_mask;
	const uint32_t value_add = (uint32_t)(s_info->value_add);
	const uint32_t delta_shift = __builtin_ctzll(delta_mask) - 2;

	uint32_t page_offset = start_offset;
	uint32_t delta = 1;

	while (delta != 0 && page_offset <= last_page_offset) {
		uint8_t *loc;
		uint32_t value;

		loc = page_content + page_offset;
		memcpy(&value, loc, sizeof(value));
		delta = (value & delta_mask) >> delta_shift;
		value &= value_mask;

		if (value != 0) {
			value += value_add;
			value += slide_amount;
		}
		memcpy(loc, &value, sizeof(value));
		page_offset += delta;
	}

	                                                                                  
	if (page_offset > last_page_offset) {
		return KERN_FAILURE;
	}
	return KERN_SUCCESS;
}

static kern_return_t
rebase_chain_64(
	uint8_t *page_content,
	uint16_t start_offset,
	uint32_t slide_amount,
	vm_shared_region_slide_info_entry_v2_t s_info)
{
	const uint32_t last_page_offset = PAGE_SIZE_FOR_SR_SLIDE - sizeof(uint64_t);

	const uint64_t delta_mask = s_info->delta_mask;
	const uint64_t value_mask = ~delta_mask;
	const uint64_t value_add = s_info->value_add;
	const uint64_t delta_shift = __builtin_ctzll(delta_mask) - 2;

	uint32_t page_offset = start_offset;
	uint32_t delta = 1;

	while (delta != 0 && page_offset <= last_page_offset) {
		uint8_t *loc;
		uint64_t value;

		loc = page_content + page_offset;
		memcpy(&value, loc, sizeof(value));
		delta = (uint32_t)((value & delta_mask) >> delta_shift);
		value &= value_mask;

		if (value != 0) {
			value += value_add;
			value += slide_amount;
		}
		memcpy(loc, &value, sizeof(value));
		page_offset += delta;
	}

	if (page_offset + sizeof(uint32_t) == PAGE_SIZE_FOR_SR_SLIDE) {
		                                                                       
                                                                            
                                                
    
                                                                        
                                      

		uint8_t *loc = page_content + page_offset;
		uint32_t value;

		memcpy(&value, loc, sizeof(value));
		value += slide_amount;
		memcpy(loc, &value, sizeof(value));
	} else if (page_offset > last_page_offset) {
		return KERN_FAILURE;
	}

	return KERN_SUCCESS;
}

static kern_return_t
rebase_chain(
	boolean_t is_64,
	uint32_t pageIndex,
	uint8_t *page_content,
	uint16_t start_offset,
	uint32_t slide_amount,
	vm_shared_region_slide_info_entry_v2_t s_info)
{
	kern_return_t kr;
	if (is_64) {
		kr = rebase_chain_64(page_content, start_offset, slide_amount, s_info);
	} else {
		kr = rebase_chain_32(page_content, start_offset, slide_amount, s_info);
	}

	if (kr != KERN_SUCCESS) {
		printf("vm_shared_region_slide_page() offset overflow: pageIndex=%u, start_offset=%u, slide_amount=%u\n",
		    pageIndex, start_offset, slide_amount);
	}
	return kr;
}

static kern_return_t
vm_shared_region_slide_page_v2(vm_shared_region_slide_info_t si, vm_offset_t vaddr, uint32_t pageIndex)
{
	vm_shared_region_slide_info_entry_v2_t s_info = &si->si_slide_info_entry->v2;
	const uint32_t slide_amount = si->si_slide;

	                                                                              
                       
	const boolean_t is_64 = (s_info->delta_mask >> 32) != 0;

	const uint16_t *page_starts = (uint16_t *)((uintptr_t)s_info + s_info->page_starts_offset);
	const uint16_t *page_extras = (uint16_t *)((uintptr_t)s_info + s_info->page_extras_offset);

	uint8_t *page_content = (uint8_t *)vaddr;
	uint16_t page_entry;

	if (pageIndex >= s_info->page_starts_count) {
		printf("vm_shared_region_slide_page() did not find page start in slide info: pageIndex=%u, count=%u\n",
		    pageIndex, s_info->page_starts_count);
		return KERN_FAILURE;
	}
	page_entry = page_starts[pageIndex];

	if (page_entry == DYLD_CACHE_SLIDE_PAGE_ATTR_NO_REBASE) {
		return KERN_SUCCESS;
	}

	if (page_entry & DYLD_CACHE_SLIDE_PAGE_ATTR_EXTRA) {
		uint16_t chain_index = page_entry & DYLD_CACHE_SLIDE_PAGE_VALUE;
		uint16_t info;

		do {
			uint16_t page_start_offset;
			kern_return_t kr;

			if (chain_index >= s_info->page_extras_count) {
				printf("vm_shared_region_slide_page() out-of-bounds extras index: index=%u, count=%u\n",
				    chain_index, s_info->page_extras_count);
				return KERN_FAILURE;
			}
			info = page_extras[chain_index];
			page_start_offset = (uint16_t)((info & DYLD_CACHE_SLIDE_PAGE_VALUE) << DYLD_CACHE_SLIDE_PAGE_OFFSET_SHIFT);

			kr = rebase_chain(is_64, pageIndex, page_content, page_start_offset, slide_amount, s_info);
			if (kr != KERN_SUCCESS) {
				return KERN_FAILURE;
			}

			chain_index++;
		} while (!(info & DYLD_CACHE_SLIDE_PAGE_ATTR_END));
	} else {
		const uint16_t page_start_offset = (uint16_t)(page_entry << DYLD_CACHE_SLIDE_PAGE_OFFSET_SHIFT);
		kern_return_t kr;

		kr = rebase_chain(is_64, pageIndex, page_content, page_start_offset, slide_amount, s_info);
		if (kr != KERN_SUCCESS) {
			return KERN_FAILURE;
		}
	}

	return KERN_SUCCESS;
}


static kern_return_t
vm_shared_region_slide_page_v3(
	vm_shared_region_slide_info_t si,
	vm_offset_t vaddr,
	__unused mach_vm_offset_t uservaddr,
	uint32_t pageIndex,
#if !__has_feature(ptrauth_calls)
	__unused
#endif                                    
	uint64_t jop_key)
{
	vm_shared_region_slide_info_entry_v3_t s_info = &si->si_slide_info_entry->v3;
	const uint32_t slide_amount = si->si_slide;

	uint8_t *page_content = (uint8_t *)vaddr;
	uint16_t page_entry;

	if (pageIndex >= s_info->page_starts_count) {
		printf("vm_shared_region_slide_page() did not find page start in slide info: pageIndex=%u, count=%u\n",
		    pageIndex, s_info->page_starts_count);
		return KERN_FAILURE;
	}
	page_entry = s_info->page_starts[pageIndex];

	if (page_entry == DYLD_CACHE_SLIDE_V3_PAGE_ATTR_NO_REBASE) {
		return KERN_SUCCESS;
	}

	uint8_t* rebaseLocation = page_content;
	uint64_t delta = page_entry;
	do {
		rebaseLocation += delta;
		uint64_t value;
		memcpy(&value, rebaseLocation, sizeof(value));
		delta = ((value & 0x3FF8000000000000) >> 51) * sizeof(uint64_t);

		                        
		    
		                               
		                                      
		                            
		                                   
		    
		    
		                                       
		                           
		                                     
		                         
		                         
		                                      
		                        
		                                   
		    

		bool isBind = (value & (1ULL << 62)) == 1;
		if (isBind) {
			return KERN_FAILURE;
		}

#if __has_feature(ptrauth_calls)
		uint16_t diversity_data = (uint16_t)(value >> 32);
		bool hasAddressDiversity = (value & (1ULL << 48)) != 0;
		ptrauth_key key = (ptrauth_key)((value >> 49) & 0x3);
#endif                                   
		bool isAuthenticated = (value & (1ULL << 63)) != 0;

		if (isAuthenticated) {
			                                                                                      
			value = (value & 0xFFFFFFFF) + slide_amount;
			                                         
			const uint64_t value_add = s_info->value_add;
			value += value_add;

#if __has_feature(ptrauth_calls)
			uint64_t discriminator = diversity_data;
			if (hasAddressDiversity) {
				                                                                                                  
				uintptr_t pageOffset = rebaseLocation - page_content;
				discriminator = __builtin_ptrauth_blend_discriminator((void*)(((uintptr_t)uservaddr) + pageOffset), discriminator);
			}

			if (jop_key != 0 && si->si_ptrauth && !arm_user_jop_disabled()) {
				  
                                                                                   
                                                
       
				value = (uintptr_t)pmap_sign_user_ptr((void *)value, key, discriminator, jop_key);
			}
#endif                                   
		} else {
			                                                                                      
			                                                          
			                                                               
			                                                  
			uint64_t top8Bits = value & 0x0007F80000000000ULL;
			uint64_t bottom43Bits = value & 0x000007FFFFFFFFFFULL;
			uint64_t targetValue = (top8Bits << 13) | bottom43Bits;
			value = targetValue + slide_amount;
		}

		memcpy(rebaseLocation, &value, sizeof(value));
	} while (delta != 0);

	return KERN_SUCCESS;
}

static kern_return_t
rebase_chainv4(
	uint8_t *page_content,
	uint16_t start_offset,
	uint32_t slide_amount,
	vm_shared_region_slide_info_entry_v4_t s_info)
{
	const uint32_t last_page_offset = PAGE_SIZE_FOR_SR_SLIDE - sizeof(uint32_t);

	const uint32_t delta_mask = (uint32_t)(s_info->delta_mask);
	const uint32_t value_mask = ~delta_mask;
	const uint32_t value_add = (uint32_t)(s_info->value_add);
	const uint32_t delta_shift = __builtin_ctzll(delta_mask) - 2;

	uint32_t page_offset = start_offset;
	uint32_t delta = 1;

	while (delta != 0 && page_offset <= last_page_offset) {
		uint8_t *loc;
		uint32_t value;

		loc = page_content + page_offset;
		memcpy(&value, loc, sizeof(value));
		delta = (value & delta_mask) >> delta_shift;
		value &= value_mask;

		if ((value & 0xFFFF8000) == 0) {
			                                        
		} else if ((value & 0x3FFF8000) == 0x3FFF8000) {
			                             
			value |= 0xC0000000;
		} else {
			                              
			value += value_add;
			value += slide_amount;
		}
		memcpy(loc, &value, sizeof(value));
		page_offset += delta;
	}

	                                                                                  
	if (page_offset > last_page_offset) {
		return KERN_FAILURE;
	}
	return KERN_SUCCESS;
}

static kern_return_t
vm_shared_region_slide_page_v4(vm_shared_region_slide_info_t si, vm_offset_t vaddr, uint32_t pageIndex)
{
	vm_shared_region_slide_info_entry_v4_t s_info = &si->si_slide_info_entry->v4;
	const uint32_t slide_amount = si->si_slide;

	const uint16_t *page_starts = (uint16_t *)((uintptr_t)s_info + s_info->page_starts_offset);
	const uint16_t *page_extras = (uint16_t *)((uintptr_t)s_info + s_info->page_extras_offset);

	uint8_t *page_content = (uint8_t *)vaddr;
	uint16_t page_entry;

	if (pageIndex >= s_info->page_starts_count) {
		printf("vm_shared_region_slide_page() did not find page start in slide info: pageIndex=%u, count=%u\n",
		    pageIndex, s_info->page_starts_count);
		return KERN_FAILURE;
	}
	page_entry = page_starts[pageIndex];

	if (page_entry == DYLD_CACHE_SLIDE4_PAGE_NO_REBASE) {
		return KERN_SUCCESS;
	}

	if (page_entry & DYLD_CACHE_SLIDE4_PAGE_USE_EXTRA) {
		uint16_t chain_index = page_entry & DYLD_CACHE_SLIDE4_PAGE_INDEX;
		uint16_t info;

		do {
			uint16_t page_start_offset;
			kern_return_t kr;

			if (chain_index >= s_info->page_extras_count) {
				printf("vm_shared_region_slide_page() out-of-bounds extras index: index=%u, count=%u\n",
				    chain_index, s_info->page_extras_count);
				return KERN_FAILURE;
			}
			info = page_extras[chain_index];
			page_start_offset = (uint16_t)((info & DYLD_CACHE_SLIDE4_PAGE_INDEX) << DYLD_CACHE_SLIDE_PAGE_OFFSET_SHIFT);

			kr = rebase_chainv4(page_content, page_start_offset, slide_amount, s_info);
			if (kr != KERN_SUCCESS) {
				return KERN_FAILURE;
			}

			chain_index++;
		} while (!(info & DYLD_CACHE_SLIDE4_PAGE_EXTRA_END));
	} else {
		const uint16_t page_start_offset = (uint16_t)(page_entry << DYLD_CACHE_SLIDE_PAGE_OFFSET_SHIFT);
		kern_return_t kr;

		kr = rebase_chainv4(page_content, page_start_offset, slide_amount, s_info);
		if (kr != KERN_SUCCESS) {
			return KERN_FAILURE;
		}
	}

	return KERN_SUCCESS;
}



kern_return_t
vm_shared_region_slide_page(
	vm_shared_region_slide_info_t si,
	vm_offset_t vaddr,
	mach_vm_offset_t uservaddr,
	uint32_t pageIndex,
	uint64_t jop_key)
{
	switch (si->si_slide_info_entry->version) {
	case 2:
		return vm_shared_region_slide_page_v2(si, vaddr, pageIndex);
	case 3:
		return vm_shared_region_slide_page_v3(si, vaddr, uservaddr, pageIndex, jop_key);
	case 4:
		return vm_shared_region_slide_page_v4(si, vaddr, pageIndex);
	default:
		return KERN_FAILURE;
	}
}

                                                                                
                                                                                
                                                                                

SECURITY_READ_ONLY_LATE(ipc_port_t) commpage32_handle = IPC_PORT_NULL;
SECURITY_READ_ONLY_LATE(ipc_port_t) commpage64_handle = IPC_PORT_NULL;
SECURITY_READ_ONLY_LATE(vm_named_entry_t) commpage32_entry = NULL;
SECURITY_READ_ONLY_LATE(vm_named_entry_t) commpage64_entry = NULL;
SECURITY_READ_ONLY_LATE(vm_map_t) commpage32_map = VM_MAP_NULL;
SECURITY_READ_ONLY_LATE(vm_map_t) commpage64_map = VM_MAP_NULL;

SECURITY_READ_ONLY_LATE(ipc_port_t) commpage_text32_handle = IPC_PORT_NULL;
SECURITY_READ_ONLY_LATE(ipc_port_t) commpage_text64_handle = IPC_PORT_NULL;
SECURITY_READ_ONLY_LATE(vm_named_entry_t) commpage_text32_entry = NULL;
SECURITY_READ_ONLY_LATE(vm_named_entry_t) commpage_text64_entry = NULL;
SECURITY_READ_ONLY_LATE(vm_map_t) commpage_text32_map = VM_MAP_NULL;
SECURITY_READ_ONLY_LATE(vm_map_t) commpage_text64_map = VM_MAP_NULL;

SECURITY_READ_ONLY_LATE(user32_addr_t) commpage_text32_location = 0;
SECURITY_READ_ONLY_LATE(user64_addr_t) commpage_text64_location = 0;

#if defined(__i386__) || defined(__x86_64__)
  
                                                              
   
static void
_vm_commpage_init(
	ipc_port_t      *handlep,
	vm_map_size_t   size)
{
	kern_return_t           kr;
	vm_named_entry_t        mem_entry;
	vm_map_t                new_map;

	SHARED_REGION_TRACE_DEBUG(
		("commpage: -> _init(0x%llx)\n",
		(long long)size));

	kr = mach_memory_entry_allocate(&mem_entry,
	    handlep);
	if (kr != KERN_SUCCESS) {
		panic("_vm_commpage_init: could not allocate mem_entry");
	}
	new_map = vm_map_create(pmap_create_options(NULL, 0, 0), 0, size, PMAP_CREATE_64BIT);
	if (new_map == VM_MAP_NULL) {
		panic("_vm_commpage_init: could not allocate VM map");
	}
	mem_entry->backing.map = new_map;
	mem_entry->internal = TRUE;
	mem_entry->is_sub_map = TRUE;
	mem_entry->offset = 0;
	mem_entry->protection = VM_PROT_ALL;
	mem_entry->size = size;

	SHARED_REGION_TRACE_DEBUG(
		("commpage: _init(0x%llx) <- %p\n",
		(long long)size, (void *)VM_KERNEL_ADDRPERM(*handlep)));
}
#endif


  
                                              
   
void
vm_commpage_text_init(void)
{
	SHARED_REGION_TRACE_DEBUG(
		("commpage text: ->init()\n"));
#if defined(__i386__) || defined(__x86_64__)
	                                      
	unsigned int offset = (random() % _PFZ32_SLIDE_RANGE) << PAGE_SHIFT;                                  
	_vm_commpage_init(&commpage_text32_handle, _COMM_PAGE_TEXT_AREA_LENGTH);
	commpage_text32_entry = (vm_named_entry_t) ip_get_kobject(commpage_text32_handle);
	commpage_text32_map = commpage_text32_entry->backing.map;
	commpage_text32_location = (user32_addr_t) (_COMM_PAGE32_TEXT_START + offset);
	                                       
	                                 
	offset = (random() % _PFZ64_SLIDE_RANGE) << PAGE_SHIFT;                                         
	_vm_commpage_init(&commpage_text64_handle, _COMM_PAGE_TEXT_AREA_LENGTH);
	commpage_text64_entry = (vm_named_entry_t) ip_get_kobject(commpage_text64_handle);
	commpage_text64_map = commpage_text64_entry->backing.map;
	commpage_text64_location = (user64_addr_t) (_COMM_PAGE64_TEXT_START + offset);
#endif

	commpage_text_populate();

	                                   
	SHARED_REGION_TRACE_DEBUG(
		("commpage text: init() <-\n"));
}

  
                                          
   
void
vm_commpage_init(void)
{
	SHARED_REGION_TRACE_DEBUG(
		("commpage: -> init()\n"));

#if defined(__i386__) || defined(__x86_64__)
	                                 
	_vm_commpage_init(&commpage32_handle, _COMM_PAGE32_AREA_LENGTH);
	commpage32_entry = (vm_named_entry_t) ip_get_kobject(commpage32_handle);
	commpage32_map = commpage32_entry->backing.map;

	                                       
	                                 
	_vm_commpage_init(&commpage64_handle, _COMM_PAGE64_AREA_LENGTH);
	commpage64_entry = (vm_named_entry_t) ip_get_kobject(commpage64_handle);
	commpage64_map = commpage64_entry->backing.map;

#endif                             

	                                                       
	commpage_populate();
	__commpage_setup = 1;
#if XNU_TARGET_OS_OSX
	if (__system_power_source == 0) {
		post_sys_powersource_internal(0, 1);
	}
#endif                        

	SHARED_REGION_TRACE_DEBUG(
		("commpage: init() <-\n"));
}

  
                                                                 
                                                   
   
kern_return_t
vm_commpage_enter(
	vm_map_t        map,
	task_t          task,
	boolean_t       is64bit)
{
#if     defined(__arm__)
#pragma unused(is64bit)
	(void)task;
	(void)map;
	return KERN_SUCCESS;
#elif   defined(__arm64__)
#pragma unused(is64bit)
	(void)task;
	(void)map;
	pmap_insert_sharedpage(vm_map_pmap(map));
	return KERN_SUCCESS;
#else
	ipc_port_t              commpage_handle, commpage_text_handle;
	vm_map_offset_t         commpage_address, objc_address, commpage_text_address;
	vm_map_size_t           commpage_size, objc_size, commpage_text_size;
	int                     vm_flags;
	vm_map_kernel_flags_t   vmk_flags;
	kern_return_t           kr;

	SHARED_REGION_TRACE_DEBUG(
		("commpage: -> enter(%p,%p)\n",
		(void *)VM_KERNEL_ADDRPERM(map),
		(void *)VM_KERNEL_ADDRPERM(task)));

	commpage_text_size = _COMM_PAGE_TEXT_AREA_LENGTH;
	                                                                       
	vm_flags = VM_FLAGS_FIXED;
	vmk_flags = VM_MAP_KERNEL_FLAGS_NONE;
	vmk_flags.vmkf_beyond_max = TRUE;

	                                                    
	assert(!(is64bit ^ vm_map_is_64bit(map)));
	if (is64bit) {
		commpage_handle = commpage64_handle;
		commpage_address = (vm_map_offset_t) _COMM_PAGE64_BASE_ADDRESS;
		commpage_size = _COMM_PAGE64_AREA_LENGTH;
		objc_size = _COMM_PAGE64_OBJC_SIZE;
		objc_address = _COMM_PAGE64_OBJC_BASE;
		commpage_text_handle = commpage_text64_handle;
		commpage_text_address = (vm_map_offset_t) commpage_text64_location;
	} else {
		commpage_handle = commpage32_handle;
		commpage_address =
		    (vm_map_offset_t)(unsigned) _COMM_PAGE32_BASE_ADDRESS;
		commpage_size = _COMM_PAGE32_AREA_LENGTH;
		objc_size = _COMM_PAGE32_OBJC_SIZE;
		objc_address = _COMM_PAGE32_OBJC_BASE;
		commpage_text_handle = commpage_text32_handle;
		commpage_text_address = (vm_map_offset_t) commpage_text32_location;
	}

	vm_tag_t tag = VM_KERN_MEMORY_NONE;
	if ((commpage_address & (pmap_commpage_size_min(map->pmap) - 1)) == 0 &&
	    (commpage_size & (pmap_commpage_size_min(map->pmap) - 1)) == 0) {
		                                                                
		tag = VM_MEMORY_SHARED_PMAP;
		vmk_flags.vmkf_nested_pmap = TRUE;
	}
	                                                   
	assert(commpage_handle != IPC_PORT_NULL);
	kr = vm_map_enter_mem_object(
		map,
		&commpage_address,
		commpage_size,
		0,
		vm_flags,
		vmk_flags,
		tag,
		commpage_handle,
		0,
		FALSE,
		VM_PROT_READ,
		VM_PROT_READ,
		VM_INHERIT_SHARE);
	if (kr != KERN_SUCCESS) {
		SHARED_REGION_TRACE_ERROR(
			("commpage: enter(%p,0x%llx,0x%llx) "
			"commpage %p mapping failed 0x%x\n",
			(void *)VM_KERNEL_ADDRPERM(map),
			(long long)commpage_address,
			(long long)commpage_size,
			(void *)VM_KERNEL_ADDRPERM(commpage_handle), kr));
	}

	                                                        
	assert(commpage_text_handle != IPC_PORT_NULL);
	kr = vm_map_enter_mem_object(
		map,
		&commpage_text_address,
		commpage_text_size,
		0,
		vm_flags,
		vmk_flags,
		tag,
		commpage_text_handle,
		0,
		FALSE,
		VM_PROT_READ | VM_PROT_EXECUTE,
		VM_PROT_READ | VM_PROT_EXECUTE,
		VM_INHERIT_SHARE);
	if (kr != KERN_SUCCESS) {
		SHARED_REGION_TRACE_ERROR(
			("commpage text: enter(%p,0x%llx,0x%llx) "
			"commpage text %p mapping failed 0x%x\n",
			(void *)VM_KERNEL_ADDRPERM(map),
			(long long)commpage_text_address,
			(long long)commpage_text_size,
			(void *)VM_KERNEL_ADDRPERM(commpage_text_handle), kr));
	}

	  
                                                                     
                                      
    
	if (objc_size != 0) {
		kr = vm_map_enter_mem_object(
			map,
			&objc_address,
			objc_size,
			0,
			VM_FLAGS_FIXED,
			vmk_flags,
			tag,
			IPC_PORT_NULL,
			0,
			FALSE,
			VM_PROT_ALL,
			VM_PROT_ALL,
			VM_INHERIT_DEFAULT);
		if (kr != KERN_SUCCESS) {
			SHARED_REGION_TRACE_ERROR(
				("commpage: enter(%p,0x%llx,0x%llx) "
				"objc mapping failed 0x%x\n",
				(void *)VM_KERNEL_ADDRPERM(map),
				(long long)objc_address,
				(long long)objc_size, kr));
		}
	}

	SHARED_REGION_TRACE_DEBUG(
		("commpage: enter(%p,%p) <- 0x%x\n",
		(void *)VM_KERNEL_ADDRPERM(map),
		(void *)VM_KERNEL_ADDRPERM(task), kr));
	return kr;
#endif
}

int
vm_shared_region_slide(
	uint32_t slide,
	mach_vm_offset_t        entry_start_address,
	mach_vm_size_t          entry_size,
	mach_vm_offset_t        slide_start,
	mach_vm_size_t          slide_size,
	mach_vm_offset_t        slid_mapping,
	memory_object_control_t sr_file_control,
	vm_prot_t               prot)
{
	vm_shared_region_t      sr;
	kern_return_t           error;

	SHARED_REGION_TRACE_DEBUG(
		("vm_shared_region_slide: -> slide %#x, entry_start %#llx, entry_size %#llx, slide_start %#llx, slide_size %#llx\n",
		slide, entry_start_address, entry_size, slide_start, slide_size));

	sr = vm_shared_region_get(current_task());
	if (sr == NULL) {
		printf("%s: no shared region?\n", __FUNCTION__);
		SHARED_REGION_TRACE_DEBUG(
			("vm_shared_region_slide: <- %d (no shared region)\n",
			KERN_FAILURE));
		return KERN_FAILURE;
	}

	  
                                   
    
	vm_shared_region_lock();
	while (sr->sr_slide_in_progress) {
		vm_shared_region_sleep(&sr->sr_slide_in_progress, THREAD_UNINT);
	}

	sr->sr_slide_in_progress = TRUE;
	vm_shared_region_unlock();

	error = vm_shared_region_slide_mapping(sr,
	    (user_addr_t)slide_start,
	    slide_size,
	    entry_start_address,
	    entry_size,
	    slid_mapping,
	    slide,
	    sr_file_control,
	    prot);
	if (error) {
		printf("slide_info initialization failed with kr=%d\n", error);
	}

	vm_shared_region_lock();

	assert(sr->sr_slide_in_progress);
	sr->sr_slide_in_progress = FALSE;
	thread_wakeup(&sr->sr_slide_in_progress);

#if XNU_TARGET_OS_OSX
	if (error == KERN_SUCCESS) {
		shared_region_completed_slide = TRUE;
	}
#endif                        
	vm_shared_region_unlock();

	vm_shared_region_deallocate(sr);

	SHARED_REGION_TRACE_DEBUG(
		("vm_shared_region_slide: <- %d\n",
		error));

	return error;
}

  
                                                    
                                                           
                                                       
                                                      
                  
   
void
vm_shared_region_pivot(void)
{
	vm_shared_region_t      shared_region = NULL;

	vm_shared_region_lock();

	queue_iterate(&vm_shared_region_queue, shared_region, vm_shared_region_t, sr_q) {
		assert(shared_region->sr_ref_count > 0);
		shared_region->sr_stale = TRUE;
		if (shared_region->sr_timer_call) {
			  
                                                   
                                                   
                                                   
                                                    
                                                             
                                                      
                                                        
                                                       
                                                          
         
      
			uint64_t deadline;
			assert(shared_region->sr_ref_count == 1);

			  
                                                             
                                                                             
                                                                    
                                                                        
                                  
      
			vm_shared_region_reference_locked(shared_region);

			                                                                    
			shared_region->sr_timer_call = thread_call_allocate(
				(thread_call_func_t) vm_shared_region_timeout,
				(thread_call_param_t) shared_region);

			                        
			clock_interval_to_deadline(10,                      
			    NSEC_PER_MSEC,
			    &deadline);
			thread_call_enter_delayed(shared_region->sr_timer_call,
			    deadline);

			SHARED_REGION_TRACE_DEBUG(
				("shared_region: pivot(%p): armed timer\n",
				(void *)VM_KERNEL_ADDRPERM(shared_region)));
		}
	}

	vm_shared_region_unlock();
}

  
                                                                       
                                                                      
   
void
vm_shared_region_reslide_stale(void)
{
#if __has_feature(ptrauth_calls)
	vm_shared_region_t      shared_region = NULL;

	vm_shared_region_lock();

	queue_iterate(&vm_shared_region_queue, shared_region, vm_shared_region_t, sr_q) {
		assert(shared_region->sr_ref_count > 0);
		if (!shared_region->sr_stale && shared_region->sr_reslide) {
			shared_region->sr_stale = TRUE;
			vm_shared_region_reslide_count++;
		}
	}

	vm_shared_region_unlock();
#endif                                   
}

  
                                                             
   
bool
vm_shared_region_is_reslide(__unused struct task *task)
{
	bool is_reslide = FALSE;
#if !XNU_TARGET_OS_OSX && __has_feature(ptrauth_calls)
	vm_shared_region_t sr = vm_shared_region_get(task);

	if (sr != NULL) {
		is_reslide = sr->sr_reslide;
		vm_shared_region_deallocate(sr);
	}
#endif                                                         
	return is_reslide;
}

  
                                                                                           
                                                   
                                              
   
void
#if XNU_TARGET_OS_OSX
post_sys_powersource(int i)
#else                        
post_sys_powersource(__unused int i)
#endif                        
{
#if XNU_TARGET_OS_OSX
	post_sys_powersource_internal(i, 0);
#endif                        
}


#if XNU_TARGET_OS_OSX
static void
post_sys_powersource_internal(int i, int internal)
{
	if (internal == 0) {
		__system_power_source = i;
	}
}
#endif                        

void *
vm_shared_region_root_dir(
	struct vm_shared_region *sr)
{
	void *vnode;

	vm_shared_region_lock();
	vnode = sr->sr_root_dir;
	vm_shared_region_unlock();
	return vnode;
}
