/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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

#include <mach/mach_types.h>
#include <mach/task.h>

#include <kern/ast.h>
#include <kern/kalloc.h>
#include <kern/kern_types.h>
#include <kern/mach_param.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/processor.h>
#include <kern/queue.h>
#include <kern/restartable.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/waitq.h>

#include <os/hash.h>
#include <os/refcnt.h>

   
                                 
  
         
                                                          
  
              
                                                              
                                              
                                        
  
                                                                     
                                                          
  
                                                                        
                                                                   
                                         
  
                                                                      
                                                                       
                     
   

typedef int (*cmpfunc_t)(const void *a, const void *b);
extern void qsort(void *a, size_t n, size_t es, cmpfunc_t cmp);

struct restartable_ranges {
	queue_chain_t            rr_link;
	os_refcnt_t              rr_ref;
	uint32_t                 rr_count;
	uint32_t                 rr_hash;
	task_restartable_range_t rr_ranges[];
};

#if DEBUG || DEVELOPMENT
#define RR_HASH_SIZE   256
#else
                                                                               
#define RR_HASH_SIZE    16
#endif

static queue_head_t rr_hash[RR_HASH_SIZE];
LCK_GRP_DECLARE(rr_lock_grp, "restartable ranges");
LCK_SPIN_DECLARE(rr_spinlock, &rr_lock_grp);

#define rr_lock()   lck_spin_lock_grp(&rr_spinlock, &rr_lock_grp)
#define rr_unlock() lck_spin_unlock(&rr_spinlock);

#pragma mark internals

   
                        
  
         
                                
   
static int
_ranges_cmp(const void *_r1, const void *_r2)
{
	const task_restartable_range_t *r1 = _r1;
	const task_restartable_range_t *r2 = _r2;

	if (r1->location != r2->location) {
		return r1->location < r2->location ? -1 : 1;
	}
	if (r1->length == r2->length) {
		return 0;
	}
	return r1->length < r2->length ? -1 : 1;
}

   
                             
  
         
                                                               
  
              
                                     
  
                   
                     
                                                                   
  
           
                                            
                                              
   
static kern_return_t
_ranges_validate(task_t task, task_restartable_range_t *ranges, uint32_t count)
{
	qsort(ranges, count, sizeof(task_restartable_range_t), _ranges_cmp);
	uint64_t limit = task_has_64Bit_data(task) ? UINT64_MAX : UINT32_MAX;
	uint64_t end, recovery;

	if (count == 0) {
		return KERN_INVALID_ARGUMENT;
	}

	for (size_t i = 0; i < count; i++) {
		if (ranges[i].length > TASK_RESTARTABLE_OFFSET_MAX ||
		    ranges[i].recovery_offs > TASK_RESTARTABLE_OFFSET_MAX) {
			return KERN_INVALID_ARGUMENT;
		}
		if (ranges[i].flags) {
			return KERN_INVALID_ARGUMENT;
		}
		if (os_add_overflow(ranges[i].location, ranges[i].length, &end)) {
			return KERN_INVALID_ARGUMENT;
		}
		if (os_add_overflow(ranges[i].location, ranges[i].recovery_offs, &recovery)) {
			return KERN_INVALID_ARGUMENT;
		}
		if (ranges[i].location > limit || end > limit || recovery > limit) {
			return KERN_INVALID_ARGUMENT;
		}
		if (i + 1 < count && end > ranges[i + 1].location) {
			return KERN_INVALID_ARGUMENT;
		}
	}

	return KERN_SUCCESS;
}

   
                           
  
         
                                                                         
  
           
                         
                                
   
__attribute__((always_inline))
static mach_vm_address_t
_ranges_lookup(struct restartable_ranges *rr, mach_vm_address_t pc)
{
	task_restartable_range_t *ranges = rr->rr_ranges;
	uint32_t l = 0, r = rr->rr_count;

	if (pc <= ranges[0].location) {
		return 0;
	}
	if (pc >= ranges[r - 1].location + ranges[r - 1].length) {
		return 0;
	}

	while (l < r) {
		uint32_t i = (r + l) / 2;
		mach_vm_address_t location = ranges[i].location;

		if (pc <= location) {
			                                                          
			r = i;
		} else if (location + ranges[i].length <= pc) {
			                                                               
			l = i + 1;
		} else {
			                                                             
			return location + ranges[i].recovery_offs;
		}
	}

	return 0;
}

   
                                        
  
         
                                                              
   
__attribute__((noinline))
static void
_restartable_ranges_dispose(struct restartable_ranges *rr, bool hash_remove)
{
	if (hash_remove) {
		rr_lock();
		remqueue(&rr->rr_link);
		rr_unlock();
	}
	kfree(rr, sizeof(*rr) + rr->rr_count * sizeof(task_restartable_range_t));
}

   
                                       
  
         
                                            
   
static bool
_restartable_ranges_equals(
	const struct restartable_ranges *rr1,
	const struct restartable_ranges *rr2)
{
	size_t rr1_size = rr1->rr_count * sizeof(task_restartable_range_t);
	return rr1->rr_hash == rr2->rr_hash &&
	       rr1->rr_count == rr2->rr_count &&
	       memcmp(rr1->rr_ranges, rr2->rr_ranges, rr1_size) == 0;
}

   
                                       
  
         
                                                
  
           
                 
                                                                    
                                                           
   
static kern_return_t
_restartable_ranges_create(task_t task, task_restartable_range_t *ranges,
    uint32_t count, struct restartable_ranges **rr_storage)
{
	struct restartable_ranges *rr, *rr_found, *rr_base;
	queue_head_t *head;
	uint32_t base_count, total_count;
	size_t base_size, size;
	kern_return_t kr;

	rr_base = *rr_storage;
	base_count = rr_base ? rr_base->rr_count : 0;
	base_size = sizeof(task_restartable_range_t) * base_count;
	size = sizeof(task_restartable_range_t) * count;

	if (os_add_overflow(base_count, count, &total_count)) {
		return KERN_INVALID_ARGUMENT;
	}
	if (total_count > 1024) {
		return KERN_RESOURCE_SHORTAGE;
	}

	rr = kalloc(sizeof(*rr) + base_size + size);
	if (rr == NULL) {
		return KERN_RESOURCE_SHORTAGE;
	}

	queue_chain_init(rr->rr_link);
	os_ref_init(&rr->rr_ref, NULL);
	rr->rr_count = total_count;
	if (base_size) {
		memcpy(rr->rr_ranges, rr_base->rr_ranges, base_size);
	}
	memcpy(rr->rr_ranges + base_count, ranges, size);
	kr = _ranges_validate(task, rr->rr_ranges, total_count);
	if (kr) {
		_restartable_ranges_dispose(rr, false);
		return kr;
	}
	rr->rr_hash = os_hash_jenkins(rr->rr_ranges,
	    rr->rr_count * sizeof(task_restartable_range_t));

	head = &rr_hash[rr->rr_hash % RR_HASH_SIZE];

	rr_lock();
	queue_iterate(head, rr_found, struct restartable_ranges *, rr_link) {
		if (_restartable_ranges_equals(rr, rr_found) &&
		os_ref_retain_try(&rr_found->rr_ref)) {
			goto found;
		}
	}

	enqueue_tail(head, &rr->rr_link);
	rr_found = rr;

found:
	if (rr_base && os_ref_release_relaxed(&rr_base->rr_ref) == 0) {
		remqueue(&rr_base->rr_link);
	} else {
		rr_base = NULL;
	}
	rr_unlock();

	*rr_storage = rr_found;

	if (rr_found != rr) {
		_restartable_ranges_dispose(rr, false);
	}
	if (rr_base) {
		_restartable_ranges_dispose(rr_base, false);
	}
	return KERN_SUCCESS;
}

#pragma mark extern interfaces

void
restartable_ranges_release(struct restartable_ranges *rr)
{
	if (os_ref_release_relaxed(&rr->rr_ref) == 0) {
		_restartable_ranges_dispose(rr, true);
	}
}

void
thread_reset_pcs_ast(thread_t thread)
{
	task_t task = thread->task;
	struct restartable_ranges *rr;
	mach_vm_address_t pc;

	  
                                                                        
                                                                           
    
	rr = task->restartable_ranges;
	if (rr) {
		                                                                     
		os_atomic_thread_fence(acquire);

		pc = _ranges_lookup(rr, machine_thread_pc(thread));

		if (pc) {
			machine_thread_reset_pc(thread, pc);
		}
	}
}

void
restartable_init(void)
{
	for (size_t i = 0; i < RR_HASH_SIZE; i++) {
		queue_head_init(rr_hash[i]);
	}
}

#pragma mark MiG interfaces

kern_return_t
task_restartable_ranges_register(
	task_t                    task,
	task_restartable_range_t *ranges,
	mach_msg_type_number_t    count)
{
	kern_return_t kr;
	thread_t th;

	if (task != current_task()) {
		return KERN_FAILURE;
	}


	kr = _ranges_validate(task, ranges, count);

	if (kr == KERN_SUCCESS) {
		task_lock(task);

		queue_iterate(&task->threads, th, thread_t, task_threads) {
			if (th != current_thread()) {
				kr = KERN_NOT_SUPPORTED;
				break;
			}
		}
#if !DEBUG && !DEVELOPMENT
		  
                                                                        
                     
    
                                                                 
                 
     
		if (task->restartable_ranges) {
			kr = KERN_NOT_SUPPORTED;
		}
#endif
		if (kr == KERN_SUCCESS) {
			kr = _restartable_ranges_create(task, ranges, count,
			    &task->restartable_ranges);
		}
		task_unlock(task);
	}

	return kr;
}

kern_return_t
task_restartable_ranges_synchronize(task_t task)
{
	thread_t thread;

	if (task != current_task()) {
		return KERN_FAILURE;
	}

	                                                      
	os_atomic_thread_fence(release);

	task_lock(task);

	if (task->restartable_ranges) {
		queue_iterate(&task->threads, thread, thread_t, task_threads) {
			if (thread != current_thread()) {
				thread_mtx_lock(thread);
				act_set_ast_reset_pcs(thread);
				thread_mtx_unlock(thread);
			}
		}
	}

	task_unlock(task);

	return KERN_SUCCESS;
}
