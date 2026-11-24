/*
 * Copyright (c) 2019-2020 Apple Inc. All rights reserved.
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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_LICENSE_HEADER_END@
 */

#include <kern/sched_prim.h>
#include <kern/ledger.h>
#include <kern/policy_internal.h>

#include <libkern/OSDebug.h>

#include <mach/mach_types.h>

#include <machine/limits.h>

#include <vm/vm_compressor_pager.h>
#include <vm/vm_kern.h>                                         
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>
#include <vm/vm_purgeable_internal.h>

#include <sys/kdebug.h>

  
                                                 
  
                                                                          
                                                                          
                          
  
                                                                       
  
                                                                             
                                                                        
                                                        
   

extern vm_pressure_level_t memorystatus_vm_pressure_level;

struct token {
	token_cnt_t     count;
	token_idx_t     prev;
	token_idx_t     next;
};

struct token    *tokens;
token_idx_t     token_q_max_cnt = 0;
vm_size_t       token_q_cur_size = 0;

token_idx_t     token_free_idx = 0;                                     
token_idx_t     token_init_idx = 1;                                        
int32_t         token_new_pagecount = 0;                                   
                                                                               

int             available_for_purge = 0;                                   
                                                                            
                                                                 
                                                                               
                                                   

static int token_q_allocating = 0;                                         
                                                               

struct purgeable_q purgeable_queues[PURGEABLE_Q_TYPE_MAX];
queue_head_t purgeable_nonvolatile_queue;
int purgeable_nonvolatile_count;

decl_lck_mtx_data(, vm_purgeable_queue_lock);

static token_idx_t vm_purgeable_token_remove_first(purgeable_q_t queue);

static void vm_purgeable_stats_helper(vm_purgeable_stat_t *stat, purgeable_q_t queue, int group, task_t target_task);


#if MACH_ASSERT
static void
vm_purgeable_token_check_queue(purgeable_q_t queue)
{
	int             token_cnt = 0, page_cnt = 0;
	token_idx_t     token = queue->token_q_head;
	token_idx_t     unripe = 0;
	int             our_inactive_count;


#if DEVELOPMENT
	static int lightweight_check = 0;

	  
                                                                                         
                                                                           
                                                                
    
	if (lightweight_check++ < (100 + queue->debug_count_tokens / 512)) {
		return;
	}

	lightweight_check = 0;
#endif

	while (token) {
		if (tokens[token].count != 0) {
			assert(queue->token_q_unripe);
			if (unripe == 0) {
				assert(token == queue->token_q_unripe);
				unripe = token;
			}
			page_cnt += tokens[token].count;
		}
		if (tokens[token].next == 0) {
			assert(queue->token_q_tail == token);
		}

		token_cnt++;
		token = tokens[token].next;
	}

	if (unripe) {
		assert(queue->token_q_unripe == unripe);
	}
	assert(token_cnt == queue->debug_count_tokens);

	                                                  
	if (queue->type != PURGEABLE_Q_TYPE_OBSOLETE) {
		our_inactive_count = page_cnt + queue->new_pages + token_new_pagecount;
		assert(our_inactive_count >= 0);
		assert((uint32_t) our_inactive_count == vm_page_inactive_count - vm_page_cleaned_count);
	}
}
#endif

  
                                                         
                               
   
kern_return_t
vm_purgeable_token_add(purgeable_q_t queue)
{
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);

	               
	token_idx_t     token;
	enum purgeable_q_type i;

find_available_token:

	if (token_free_idx) {                                                        
		token = token_free_idx;
		token_free_idx = tokens[token_free_idx].next;
	} else if (token_init_idx < token_q_max_cnt) {                             
		token = token_init_idx;
		token_init_idx++;
	} else {                                                                  
		                                                               
		while (token_q_allocating) {
			wait_result_t res = lck_mtx_sleep(&vm_page_queue_lock,
			    LCK_SLEEP_DEFAULT,
			    (event_t)&token_q_allocating,
			    THREAD_UNINT);
			if (res != THREAD_AWAKENED) {
				return KERN_ABORTED;
			}
		}
		;

		                                             
		if (token_init_idx < token_q_max_cnt) {
			goto find_available_token;
		}

		                                     
		token_q_allocating = 1;

		                                             
		vm_page_unlock_queues();

		struct token *new_loc;
		vm_size_t alloc_size = token_q_cur_size + PAGE_SIZE;
		kern_return_t result;

		if (alloc_size / sizeof(struct token) > TOKEN_COUNT_MAX) {
			result = KERN_RESOURCE_SHORTAGE;
		} else {
			if (token_q_cur_size) {
				result = kmem_realloc(kernel_map,
				    (vm_offset_t) tokens,
				    token_q_cur_size,
				    (vm_offset_t *) &new_loc,
				    alloc_size, VM_KERN_MEMORY_OSFMK);
			} else {
				result = kmem_alloc(kernel_map,
				    (vm_offset_t *) &new_loc,
				    alloc_size, VM_KERN_MEMORY_OSFMK);
			}
		}

		vm_page_lock_queues();

		if (result) {
			                             
			token_q_allocating = 0;
			thread_wakeup((event_t)&token_q_allocating);
			return result;
		}

		                                                               
                         
		struct token *old_tokens = tokens;
		tokens = new_loc;
		vm_size_t old_token_q_cur_size = token_q_cur_size;
		token_q_cur_size = alloc_size;
		token_q_max_cnt = (token_idx_t) (token_q_cur_size /
		    sizeof(struct token));
		assert(token_init_idx < token_q_max_cnt);                                          

		if (old_token_q_cur_size) {                               
			vm_page_unlock_queues();
			                                                               
			kmem_free(kernel_map, (vm_offset_t)old_tokens, old_token_q_cur_size);
			vm_page_lock_queues();
		}

		                             
		token_q_allocating = 0;
		thread_wakeup((event_t)&token_q_allocating);

		goto find_available_token;
	}

	assert(token);

	  
                                                                    
            
    
	for (i = PURGEABLE_Q_TYPE_FIFO; i < PURGEABLE_Q_TYPE_MAX; i++) {
		int64_t pages = purgeable_queues[i].new_pages += token_new_pagecount;
		assert(pages >= 0);
		assert(pages <= TOKEN_COUNT_MAX);
		purgeable_queues[i].new_pages = (int32_t) pages;
		assert(purgeable_queues[i].new_pages == pages);
	}
	token_new_pagecount = 0;

	                             
	if (queue->type != PURGEABLE_Q_TYPE_OBSOLETE) {
		tokens[token].count = queue->new_pages;
	} else {
		tokens[token].count = 0;                                 
                                                        
	}
	queue->new_pages = 0;

	                                     
	tokens[token].next = 0;
	if (queue->token_q_tail == 0) {
		assert(queue->token_q_head == 0 && queue->token_q_unripe == 0);
		queue->token_q_head = token;
		tokens[token].prev = 0;
	} else {
		tokens[queue->token_q_tail].next = token;
		tokens[token].prev = queue->token_q_tail;
	}
	if (queue->token_q_unripe == 0) {                                 
                                                            
		if (tokens[token].count > 0) {
			queue->token_q_unripe = token;                          
		} else {
			available_for_purge++;                        
                                                         
		}
	}
	queue->token_q_tail = token;

#if MACH_ASSERT
	queue->debug_count_tokens++;
	                                                                      
	vm_purgeable_token_check_queue(&purgeable_queues[PURGEABLE_Q_TYPE_FIFO]);
	vm_purgeable_token_check_queue(&purgeable_queues[PURGEABLE_Q_TYPE_LIFO]);

	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, TOKEN_ADD)),
	    queue->type,
	    tokens[token].count,                                             
                                                                   
	    queue->debug_count_tokens,
	    0,
	    0);
#endif

	return KERN_SUCCESS;
}

  
                                                                           
                           
                               
   
static token_idx_t
vm_purgeable_token_remove_first(purgeable_q_t queue)
{
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);

	token_idx_t     token;
	token = queue->token_q_head;

	assert(token);

	if (token) {
		assert(queue->token_q_tail);
		if (queue->token_q_head == queue->token_q_unripe) {
			                                                
			queue->token_q_unripe = tokens[token].next;
		} else {
			                                                 
			available_for_purge--;
			assert(available_for_purge >= 0);
		}

		if (queue->token_q_tail == queue->token_q_head) {
			assert(tokens[token].next == 0);
		}

		queue->token_q_head = tokens[token].next;
		if (queue->token_q_head) {
			tokens[queue->token_q_head].count += tokens[token].count;
			tokens[queue->token_q_head].prev = 0;
		} else {
			                                            
			  
                                                    
                   
      
			queue->new_pages += tokens[token].count;
			                                  
			queue->token_q_tail = 0;
		}

#if MACH_ASSERT
		queue->debug_count_tokens--;
		vm_purgeable_token_check_queue(queue);

		KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, TOKEN_DELETE)),
		    queue->type,
		    tokens[queue->token_q_head].count,                                             
                                                                                   
		    token_new_pagecount,                                                
                                                                  
		    available_for_purge,
		    0);
#endif
	}
	return token;
}

static token_idx_t
vm_purgeable_token_remove_last(purgeable_q_t queue)
{
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);

	token_idx_t     token;
	token = queue->token_q_tail;

	assert(token);

	if (token) {
		assert(queue->token_q_head);

		if (queue->token_q_tail == queue->token_q_head) {
			assert(tokens[token].next == 0);
		}

		if (queue->token_q_unripe == 0) {
			                                                 
			available_for_purge--;
			assert(available_for_purge >= 0);
		} else if (queue->token_q_unripe == token) {
			                                          
			queue->token_q_unripe = 0;
		}

		if (token == queue->token_q_head) {
			                                        
			queue->token_q_head = 0;
			queue->token_q_tail = 0;
		} else {
			token_idx_t new_tail;

			new_tail = tokens[token].prev;

			assert(new_tail);
			assert(tokens[new_tail].next == token);

			queue->token_q_tail = new_tail;
			tokens[new_tail].next = 0;
		}

		queue->new_pages += tokens[token].count;

#if MACH_ASSERT
		queue->debug_count_tokens--;
		vm_purgeable_token_check_queue(queue);

		KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, TOKEN_DELETE)),
		    queue->type,
		    tokens[queue->token_q_head].count,                                             
                                                                                   
		    token_new_pagecount,                                                
                                                                  
		    available_for_purge,
		    0);
#endif
	}
	return token;
}

  
                                                              
                               
   
void
vm_purgeable_token_delete_first(purgeable_q_t queue)
{
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);
	token_idx_t     token = vm_purgeable_token_remove_first(queue);

	if (token) {
		                                       
		tokens[token].next = token_free_idx;
		tokens[token].prev = 0;
		token_free_idx = token;
	}
}

void
vm_purgeable_token_delete_last(purgeable_q_t queue)
{
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);
	token_idx_t     token = vm_purgeable_token_remove_last(queue);

	if (token) {
		                                       
		tokens[token].next = token_free_idx;
		tokens[token].prev = 0;
		token_free_idx = token;
	}
}


                                  
void
vm_purgeable_q_advance_all()
{
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);

	                                                                    
                                                                          
	int i;
	if (token_new_pagecount > (TOKEN_NEW_PAGECOUNT_MAX >> 1)) {                                                 
		for (i = PURGEABLE_Q_TYPE_FIFO; i < PURGEABLE_Q_TYPE_MAX; i++) {
			int64_t pages = purgeable_queues[i].new_pages += token_new_pagecount;
			assert(pages >= 0);
			assert(pages <= TOKEN_COUNT_MAX);
			purgeable_queues[i].new_pages = (int32_t) pages;
			assert(purgeable_queues[i].new_pages == pages);
		}
		token_new_pagecount = 0;
	}

	  
                                                                         
                                                                           
                                                                              
                                                                            
                                                                           
            
   
                                                                 
          
    
	for (i = PURGEABLE_Q_TYPE_FIFO; i < PURGEABLE_Q_TYPE_MAX; i++) {
		purgeable_q_t queue = &purgeable_queues[i];
		uint32_t num_pages = 1;

		                                                             
		while (queue->token_q_unripe) {
			if (tokens[queue->token_q_unripe].count && num_pages) {
				tokens[queue->token_q_unripe].count -= 1;
				num_pages -= 1;
			}

			if (tokens[queue->token_q_unripe].count == 0) {
				queue->token_q_unripe = tokens[queue->token_q_unripe].next;
				available_for_purge++;
				KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, TOKEN_RIPEN)),
				    queue->type,
				    tokens[queue->token_q_head].count,                                             
                                                                                     
				    0,
				    available_for_purge,
				    0);
				continue;                                         
                                         
			}
			if (num_pages == 0) {
				break;                                              
                            
			}
		}

		  
                                                              
                                                                     
                                                                     
                                                        
     
		if (!queue->token_q_unripe) {
			queue->new_pages -= num_pages;
			assert((int32_t) token_new_pagecount + queue->new_pages >= 0);
		}
#if MACH_ASSERT
		vm_purgeable_token_check_queue(queue);
#endif
	}
}

  
                                                                           
                                                         
                             
                                        
                                  
                                                
                                           
                                                                           
                                                                 
                             
                               
   
static void
vm_purgeable_token_remove_ripe(purgeable_q_t queue)
{
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);
	assert(queue->token_q_head && tokens[queue->token_q_head].count == 0);
	                                                    
	token_idx_t     new_head = tokens[queue->token_q_head].next;
	tokens[queue->token_q_head].next = token_free_idx;
	tokens[queue->token_q_head].prev = 0;
	token_free_idx = queue->token_q_head;
	queue->token_q_head = new_head;
	tokens[new_head].prev = 0;
	if (new_head == 0) {
		queue->token_q_tail = 0;
	}

#if MACH_ASSERT
	queue->debug_count_tokens--;
	vm_purgeable_token_check_queue(queue);
#endif

	available_for_purge--;
	assert(available_for_purge >= 0);
}

  
                                                                           
                                                                           
                       
                               
   
static void
vm_purgeable_token_choose_and_delete_ripe(purgeable_q_t queue, purgeable_q_t queue2)
{
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);
	assert(queue->token_q_head);

	if (tokens[queue->token_q_head].count == 0) {
		                                          
		vm_purgeable_token_remove_ripe(queue);
	} else {
		assert(queue2);
		  
                                                           
                          
     
		vm_purgeable_token_remove_ripe(queue2);
		                          
		token_idx_t     token;
		token_cnt_t     count;

		                              
		assert(queue->token_q_unripe == queue->token_q_head);                          
                                                                                 
		token = vm_purgeable_token_remove_first(queue);
		assert(token);

		count = tokens[token].count;

		                       
		                                

		token_idx_t token_to_insert_before = queue2->token_q_head, token_to_insert_after;

		while (token_to_insert_before != 0 && count > tokens[token_to_insert_before].count) {
			count -= tokens[token_to_insert_before].count;
			token_to_insert_before = tokens[token_to_insert_before].next;
		}

		                                                 

		                                                              
		if ((token_to_insert_before == queue2->token_q_unripe) || (queue2->token_q_unripe == 0)) {
			queue2->token_q_unripe = token;                                        
		}
		  
                  
                                                         
                                            
     

		tokens[token].count = count;

		if (token_to_insert_before != 0) {
			token_to_insert_after = tokens[token_to_insert_before].prev;

			tokens[token].next = token_to_insert_before;
			tokens[token_to_insert_before].prev = token;

			assert(tokens[token_to_insert_before].count >= count);
			tokens[token_to_insert_before].count -= count;
		} else {
			                                                                              
			token_to_insert_after = queue2->token_q_tail;

			tokens[token].next = 0;
			queue2->token_q_tail = token;

			assert(queue2->new_pages >= (int32_t) count);
			queue2->new_pages -= count;
		}

		if (token_to_insert_after != 0) {
			tokens[token].prev = token_to_insert_after;
			tokens[token_to_insert_after].next = token;
		} else {
			                            
			tokens[token].prev = 0;
			queue2->token_q_head = token;
		}

#if MACH_ASSERT
		queue2->debug_count_tokens++;
		vm_purgeable_token_check_queue(queue2);
#endif
	}
}

                                                               
                                       
static vm_object_t
vm_purgeable_object_find_and_lock(
	purgeable_q_t   queue,
	int             group,
	boolean_t       pick_ripe)
{
	vm_object_t     object, best_object;
	int             object_task_importance;
	int             best_object_task_importance;
	int             best_object_skipped;
	int             num_objects_skipped;
	int             try_lock_failed = 0;
	int             try_lock_succeeded = 0;
	task_t          owner;

	best_object = VM_OBJECT_NULL;
	best_object_task_importance = INT_MAX;

	LCK_MTX_ASSERT(&vm_purgeable_queue_lock, LCK_MTX_ASSERT_OWNED);
	  
                                                                     
                                                                   
                                
    

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (MACHDBG_CODE(DBG_MACH_VM, OBJECT_PURGE_LOOP) | DBG_FUNC_START),
	    pick_ripe,
	    group,
	    VM_KERNEL_UNSLIDE_OR_PERM(queue),
	    0,
	    0);

	num_objects_skipped = 0;
	for (object = (vm_object_t) queue_first(&queue->objq[group]);
	    !queue_end(&queue->objq[group], (queue_entry_t) object);
	    object = (vm_object_t) queue_next(&object->objq),
	    num_objects_skipped++) {
		  
                                                               
                                                                             
                                                                             
                                                           
     
		if ((num_objects_skipped >= PURGEABLE_LOOP_MAX) && (best_object != NULL)) {
			break;
		}

		if (pick_ripe &&
		    !object->purgeable_when_ripe) {
			                                             
			continue;
		}

		object_task_importance = 0;

		  
                                                            
                                                   
                                                                  
     
		owner = object->vo_owner;
		if (owner != NULL && owner != VM_OBJECT_OWNER_DISOWNED) {
#if !XNU_TARGET_OS_OSX
#if CONFIG_JETSAM
			object_task_importance = proc_get_memstat_priority((struct proc *)get_bsdtask_info(owner), TRUE);
#endif                    
#else                         
			object_task_importance = task_importance_estimate(owner);
#endif                         
		}

		if (object_task_importance < best_object_task_importance) {
			if (vm_object_lock_try(object)) {
				try_lock_succeeded++;
				if (best_object != VM_OBJECT_NULL) {
					                                       
					vm_object_unlock(best_object);
				}
				best_object = object;
				best_object_task_importance = object_task_importance;
				best_object_skipped = num_objects_skipped;
				if (best_object_task_importance == 0) {
					                                        
					break;
				}
			} else {
				try_lock_failed++;
			}
		}
	}

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (MACHDBG_CODE(DBG_MACH_VM, OBJECT_PURGE_LOOP) | DBG_FUNC_END),
	    num_objects_skipped,                                           
	    try_lock_failed,
	    try_lock_succeeded,
	    VM_KERNEL_UNSLIDE_OR_PERM(best_object),
	    ((best_object == NULL) ? 0 : best_object->resident_page_count));

	object = best_object;

	if (object == VM_OBJECT_NULL) {
		return VM_OBJECT_NULL;
	}

	                                                      
                                                                                 

	vm_object_lock_assert_exclusive(object);

	queue_remove(&queue->objq[group], object,
	    vm_object_t, objq);
	object->objq.next = NULL;
	object->objq.prev = NULL;
	object->purgeable_queue_type = PURGEABLE_Q_TYPE_MAX;
	object->purgeable_queue_group = 0;
	                                                      
	vm_purgeable_volatile_owner_update(VM_OBJECT_OWNER(object), -1);

#if DEBUG
	object->vo_purgeable_volatilizer = NULL;
#endif            

	                                        
	queue_enter(&purgeable_nonvolatile_queue, object,
	    vm_object_t, objq);
	assert(purgeable_nonvolatile_count >= 0);
	purgeable_nonvolatile_count++;
	assert(purgeable_nonvolatile_count > 0);
	                                                         
	vm_purgeable_nonvolatile_owner_update(VM_OBJECT_OWNER(object), +1);

#if MACH_ASSERT
	queue->debug_count_objects--;
#endif
	return object;
}

                                         
void
vm_purgeable_object_purge_all(void)
{
	enum purgeable_q_type i;
	int             group;
	vm_object_t     object;
	unsigned int    purged_count;
	uint32_t        collisions;

	purged_count = 0;
	collisions = 0;

restart:
	lck_mtx_lock(&vm_purgeable_queue_lock);
	                              
	for (i = PURGEABLE_Q_TYPE_OBSOLETE; i < PURGEABLE_Q_TYPE_MAX; i++) {
		purgeable_q_t   queue;

		queue = &purgeable_queues[i];

		  
                                                          
                                                              
                                                           
                                                    
     
		for (group = 0; group < NUM_VOLATILE_GROUPS; group++) {
			while (!queue_empty(&queue->objq[group])) {
				object = vm_purgeable_object_find_and_lock(queue, group, FALSE);
				if (object == VM_OBJECT_NULL) {
					lck_mtx_unlock(&vm_purgeable_queue_lock);
					mutex_pause(collisions++);
					goto restart;
				}

				lck_mtx_unlock(&vm_purgeable_queue_lock);

				                                               
                                          
				if (object->purgeable_when_ripe) {
					vm_page_lock_queues();
					vm_purgeable_token_remove_first(queue);
					vm_page_unlock_queues();
				}

				(void) vm_object_purge(object, 0);
				assert(object->purgable == VM_PURGABLE_EMPTY);
				                                       

				vm_object_unlock(object);
				purged_count++;
				goto restart;
			}
			assert(queue->debug_count_objects >= 0);
		}
	}
	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, OBJECT_PURGE_ALL)),
	    purged_count,                                            
	    0,
	    available_for_purge,
	    0,
	    0);
	lck_mtx_unlock(&vm_purgeable_queue_lock);
	return;
}

boolean_t
vm_purgeable_object_purge_one_unlocked(
	int     force_purge_below_group)
{
	boolean_t       retval;

	vm_page_lock_queues();
	retval = vm_purgeable_object_purge_one(force_purge_below_group, 0);
	vm_page_unlock_queues();

	return retval;
}

boolean_t
vm_purgeable_object_purge_one(
	int     force_purge_below_group,
	int     flags)
{
	enum purgeable_q_type i;
	int             group;
	vm_object_t     object = 0;
	purgeable_q_t   queue, queue2;
	boolean_t       forced_purge;
	unsigned int    resident_page_count;


	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, OBJECT_PURGE)) | DBG_FUNC_START,
	    force_purge_below_group, flags, 0, 0, 0);

	                                                                       
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);
	lck_mtx_lock(&vm_purgeable_queue_lock);

	                              
	for (i = PURGEABLE_Q_TYPE_OBSOLETE; i < PURGEABLE_Q_TYPE_MAX; i++) {
		queue = &purgeable_queues[i];

		if (force_purge_below_group == 0) {
			  
                                                      
                                         
      
			if (!queue->token_q_head) {
				                                            
				continue;
			}

			if (tokens[queue->token_q_head].count != 0) {
				                               
				continue;
			}
		}

		  
                                                              
                                                              
                                                           
                                                    
     
		for (group = 0; group < NUM_VOLATILE_GROUPS; group++) {
			if (!queue->token_q_head ||
			    tokens[queue->token_q_head].count != 0) {
				                                 

				if (group >= force_purge_below_group) {
					                                   
					break;
				}

				  
                                            
                                      
       
				if (!queue_empty(&queue->objq[group]) &&
				    (object = vm_purgeable_object_find_and_lock(queue, group, FALSE))) {
					lck_mtx_unlock(&vm_purgeable_queue_lock);
					if (object->purgeable_when_ripe) {
						vm_purgeable_token_delete_first(queue);
					}
					forced_purge = TRUE;
					goto purge_now;
				}

				                                                
				continue;
			}
			if (!queue_empty(&queue->objq[group]) &&
			    (object = vm_purgeable_object_find_and_lock(queue, group, TRUE))) {
				lck_mtx_unlock(&vm_purgeable_queue_lock);
				if (object->purgeable_when_ripe) {
					vm_purgeable_token_choose_and_delete_ripe(queue, 0);
				}
				forced_purge = FALSE;
				goto purge_now;
			}
			if (i != PURGEABLE_Q_TYPE_OBSOLETE) {
				                                                         
                            
				queue2 = &purgeable_queues[i != PURGEABLE_Q_TYPE_FIFO ?
				    PURGEABLE_Q_TYPE_FIFO :
				    PURGEABLE_Q_TYPE_LIFO];

				if (!queue_empty(&queue2->objq[group]) &&
				    (object = vm_purgeable_object_find_and_lock(queue2, group, TRUE))) {
					lck_mtx_unlock(&vm_purgeable_queue_lock);
					if (object->purgeable_when_ripe) {
						vm_purgeable_token_choose_and_delete_ripe(queue2, queue);
					}
					forced_purge = FALSE;
					goto purge_now;
				}
			}
			assert(queue->debug_count_objects >= 0);
		}
	}
	  
                                                                     
                                                                     
                                        
    
	lck_mtx_unlock(&vm_purgeable_queue_lock);

	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, OBJECT_PURGE)) | DBG_FUNC_END,
	    0, 0, available_for_purge, 0, 0);

	return FALSE;

purge_now:

	assert(object);
	vm_page_unlock_queues();                                            
                                                                                                                                                                                                                                                                                                                
	resident_page_count = object->resident_page_count;
	(void) vm_object_purge(object, flags);
	assert(object->purgable == VM_PURGABLE_EMPTY);
	                                       
	vm_object_unlock(object);
	vm_page_lock_queues();

	vm_pageout_vminfo.vm_pageout_pages_purged += resident_page_count;

	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, OBJECT_PURGE)) | DBG_FUNC_END,
	    VM_KERNEL_UNSLIDE_OR_PERM(object),                                             
	    resident_page_count,
	    available_for_purge,
	    0,
	    0);

	return TRUE;
}

                                  
void
vm_purgeable_object_add(vm_object_t object, purgeable_q_t queue, int group)
{
	vm_object_lock_assert_exclusive(object);
	lck_mtx_lock(&vm_purgeable_queue_lock);

	assert(object->objq.next != NULL);
	assert(object->objq.prev != NULL);
	queue_remove(&purgeable_nonvolatile_queue, object,
	    vm_object_t, objq);
	object->objq.next = NULL;
	object->objq.prev = NULL;
	assert(purgeable_nonvolatile_count > 0);
	purgeable_nonvolatile_count--;
	assert(purgeable_nonvolatile_count >= 0);
	                                                         
	vm_purgeable_nonvolatile_owner_update(VM_OBJECT_OWNER(object), -1);

	if (queue->type == PURGEABLE_Q_TYPE_OBSOLETE) {
		group = 0;
	}

	if (queue->type != PURGEABLE_Q_TYPE_LIFO) {                             
                                                           
		queue_enter(&queue->objq[group], object, vm_object_t, objq);                     
	} else {
		queue_enter_first(&queue->objq[group], object, vm_object_t, objq);                        
	}
	                                                      
	vm_purgeable_volatile_owner_update(VM_OBJECT_OWNER(object), +1);

	object->purgeable_queue_type = queue->type;
	object->purgeable_queue_group = group;

#if DEBUG
	assert(object->vo_purgeable_volatilizer == NULL);
	object->vo_purgeable_volatilizer = current_task();
	OSBacktrace(&object->purgeable_volatilizer_bt[0],
	    ARRAY_COUNT(object->purgeable_volatilizer_bt));
#endif            

#if MACH_ASSERT
	queue->debug_count_objects++;
	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, OBJECT_ADD)),
	    0,
	    tokens[queue->token_q_head].count,
	    queue->type,
	    group,
	    0);
#endif

	lck_mtx_unlock(&vm_purgeable_queue_lock);
}

                                                             
                                  
purgeable_q_t
vm_purgeable_object_remove(vm_object_t object)
{
	int group;
	enum purgeable_q_type type;
	purgeable_q_t queue;

	vm_object_lock_assert_exclusive(object);

	type = object->purgeable_queue_type;
	group = object->purgeable_queue_group;

	if (type == PURGEABLE_Q_TYPE_MAX) {
		if (object->objq.prev || object->objq.next) {
			panic("unmarked object on purgeable q");
		}

		return NULL;
	} else if (!(object->objq.prev && object->objq.next)) {
		panic("marked object not on purgeable q");
	}

	lck_mtx_lock(&vm_purgeable_queue_lock);

	queue = &purgeable_queues[type];

	queue_remove(&queue->objq[group], object, vm_object_t, objq);
	object->objq.next = NULL;
	object->objq.prev = NULL;
	                                                      
	vm_purgeable_volatile_owner_update(VM_OBJECT_OWNER(object), -1);
#if DEBUG
	object->vo_purgeable_volatilizer = NULL;
#endif            
	                                        
	if (object->alive && !object->terminating) {
		queue_enter(&purgeable_nonvolatile_queue, object,
		    vm_object_t, objq);
		assert(purgeable_nonvolatile_count >= 0);
		purgeable_nonvolatile_count++;
		assert(purgeable_nonvolatile_count > 0);
		                                                         
		vm_purgeable_nonvolatile_owner_update(VM_OBJECT_OWNER(object), +1);
	}

#if MACH_ASSERT
	queue->debug_count_objects--;
	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, OBJECT_REMOVE)),
	    0,
	    tokens[queue->token_q_head].count,
	    queue->type,
	    group,
	    0);
#endif

	lck_mtx_unlock(&vm_purgeable_queue_lock);

	object->purgeable_queue_type = PURGEABLE_Q_TYPE_MAX;
	object->purgeable_queue_group = 0;

	vm_object_lock_assert_exclusive(object);

	return &purgeable_queues[type];
}

void
vm_purgeable_stats_helper(vm_purgeable_stat_t *stat, purgeable_q_t queue, int group, task_t target_task)
{
	LCK_MTX_ASSERT(&vm_purgeable_queue_lock, LCK_MTX_ASSERT_OWNED);

	stat->count = stat->size = 0;
	vm_object_t     object;
	for (object = (vm_object_t) queue_first(&queue->objq[group]);
	    !queue_end(&queue->objq[group], (queue_entry_t) object);
	    object = (vm_object_t) queue_next(&object->objq)) {
		if (!target_task || VM_OBJECT_OWNER(object) == target_task) {
			stat->count++;
			stat->size += (object->resident_page_count * PAGE_SIZE);
		}
	}
	return;
}

void
vm_purgeable_stats(vm_purgeable_info_t info, task_t target_task)
{
	purgeable_q_t   queue;
	int             group;

	lck_mtx_lock(&vm_purgeable_queue_lock);

	                        
	queue = &purgeable_queues[PURGEABLE_Q_TYPE_FIFO];
	for (group = 0; group < NUM_VOLATILE_GROUPS; group++) {
		vm_purgeable_stats_helper(&(info->fifo_data[group]), queue, group, target_task);
	}

	                        
	queue = &purgeable_queues[PURGEABLE_Q_TYPE_LIFO];
	for (group = 0; group < NUM_VOLATILE_GROUPS; group++) {
		vm_purgeable_stats_helper(&(info->lifo_data[group]), queue, group, target_task);
	}

	                            
	queue = &purgeable_queues[PURGEABLE_Q_TYPE_OBSOLETE];
	vm_purgeable_stats_helper(&(info->obsolete_data), queue, 0, target_task);

	lck_mtx_unlock(&vm_purgeable_queue_lock);
	return;
}

#if DEVELOPMENT || DEBUG
static void
vm_purgeable_account_volatile_queue(
	purgeable_q_t queue,
	int group,
	task_t task,
	pvm_account_info_t acnt_info)
{
	vm_object_t object;
	uint64_t compressed_count;

	for (object = (vm_object_t) queue_first(&queue->objq[group]);
	    !queue_end(&queue->objq[group], (queue_entry_t) object);
	    object = (vm_object_t) queue_next(&object->objq)) {
		if (VM_OBJECT_OWNER(object) == task) {
			compressed_count = vm_compressor_pager_get_count(object->pager);
			acnt_info->pvm_volatile_compressed_count += compressed_count;
			acnt_info->pvm_volatile_count += (object->resident_page_count - object->wired_page_count);
			acnt_info->pvm_nonvolatile_count += object->wired_page_count;
		}
	}
}

  
                                                             
                                                  
   
kern_return_t
vm_purgeable_account(
	task_t                  task,
	pvm_account_info_t      acnt_info)
{
	queue_head_t    *nonvolatile_q;
	vm_object_t     object;
	int             group;
	int             state;
	uint64_t        compressed_count;
	purgeable_q_t   volatile_q;


	if ((task == NULL) || (acnt_info == NULL)) {
		return KERN_INVALID_ARGUMENT;
	}

	acnt_info->pvm_volatile_count = 0;
	acnt_info->pvm_volatile_compressed_count = 0;
	acnt_info->pvm_nonvolatile_count = 0;
	acnt_info->pvm_nonvolatile_compressed_count = 0;

	lck_mtx_lock(&vm_purgeable_queue_lock);

	nonvolatile_q = &purgeable_nonvolatile_queue;
	for (object = (vm_object_t) queue_first(nonvolatile_q);
	    !queue_end(nonvolatile_q, (queue_entry_t) object);
	    object = (vm_object_t) queue_next(&object->objq)) {
		if (VM_OBJECT_OWNER(object) == task) {
			state = object->purgable;
			compressed_count =  vm_compressor_pager_get_count(object->pager);
			if (state == VM_PURGABLE_EMPTY) {
				acnt_info->pvm_volatile_count += (object->resident_page_count - object->wired_page_count);
				acnt_info->pvm_volatile_compressed_count += compressed_count;
			} else {
				acnt_info->pvm_nonvolatile_count += (object->resident_page_count - object->wired_page_count);
				acnt_info->pvm_nonvolatile_compressed_count += compressed_count;
			}
			acnt_info->pvm_nonvolatile_count += object->wired_page_count;
		}
	}

	volatile_q = &purgeable_queues[PURGEABLE_Q_TYPE_OBSOLETE];
	vm_purgeable_account_volatile_queue(volatile_q, 0, task, acnt_info);

	volatile_q = &purgeable_queues[PURGEABLE_Q_TYPE_FIFO];
	for (group = 0; group < NUM_VOLATILE_GROUPS; group++) {
		vm_purgeable_account_volatile_queue(volatile_q, group, task, acnt_info);
	}

	volatile_q = &purgeable_queues[PURGEABLE_Q_TYPE_LIFO];
	for (group = 0; group < NUM_VOLATILE_GROUPS; group++) {
		vm_purgeable_account_volatile_queue(volatile_q, group, task, acnt_info);
	}
	lck_mtx_unlock(&vm_purgeable_queue_lock);

	acnt_info->pvm_volatile_count = (acnt_info->pvm_volatile_count * PAGE_SIZE);
	acnt_info->pvm_volatile_compressed_count = (acnt_info->pvm_volatile_compressed_count * PAGE_SIZE);
	acnt_info->pvm_nonvolatile_count = (acnt_info->pvm_nonvolatile_count * PAGE_SIZE);
	acnt_info->pvm_nonvolatile_compressed_count = (acnt_info->pvm_nonvolatile_compressed_count * PAGE_SIZE);

	return KERN_SUCCESS;
}
#endif                           

static uint64_t
vm_purgeable_queue_purge_task_owned(
	purgeable_q_t   queue,
	int             group,
	task_t          task)
{
	vm_object_t     object = VM_OBJECT_NULL;
	int             collisions = 0;
	uint64_t        num_pages_purged = 0;

	num_pages_purged = 0;
	collisions = 0;

look_again:
	lck_mtx_lock(&vm_purgeable_queue_lock);

	for (object = (vm_object_t) queue_first(&queue->objq[group]);
	    !queue_end(&queue->objq[group], (queue_entry_t) object);
	    object = (vm_object_t) queue_next(&object->objq)) {
		if (object->vo_owner != task) {
			continue;
		}

		                                      
		if (!vm_object_lock_try(object)) {
			lck_mtx_unlock(&vm_purgeable_queue_lock);
			mutex_pause(collisions++);
			goto look_again;
		}
		              

		collisions = 0;

		                                        
		queue_remove(&queue->objq[group], object,
		    vm_object_t, objq);
		object->objq.next = NULL;
		object->objq.prev = NULL;
		object->purgeable_queue_type = PURGEABLE_Q_TYPE_MAX;
		object->purgeable_queue_group = 0;
		                                                      
		assert(object->vo_owner == task);
		vm_purgeable_volatile_owner_update(task, -1);

#if DEBUG
		object->vo_purgeable_volatilizer = NULL;
#endif            
		queue_enter(&purgeable_nonvolatile_queue, object,
		    vm_object_t, objq);
		assert(purgeable_nonvolatile_count >= 0);
		purgeable_nonvolatile_count++;
		assert(purgeable_nonvolatile_count > 0);
		                                                         
		assert(object->vo_owner == task);
		vm_purgeable_nonvolatile_owner_update(task, +1);

		                             
		lck_mtx_unlock(&vm_purgeable_queue_lock);

		if (object->purgeable_when_ripe) {
			                    
			vm_page_lock_queues();
			vm_purgeable_token_remove_first(queue);
			vm_page_unlock_queues();
		}

		                      
		num_pages_purged += vm_object_purge(object, 0);

		assert(object->purgable == VM_PURGABLE_EMPTY);
		                                        
		vm_object_unlock(object);

		                                                     
		goto look_again;
	}

	lck_mtx_unlock(&vm_purgeable_queue_lock);

	return num_pages_purged;
}

uint64_t
vm_purgeable_purge_task_owned(
	task_t  task)
{
	purgeable_q_t   queue = NULL;
	int             group = 0;
	uint64_t        num_pages_purged = 0;

	num_pages_purged = 0;

	queue = &purgeable_queues[PURGEABLE_Q_TYPE_OBSOLETE];
	num_pages_purged += vm_purgeable_queue_purge_task_owned(queue,
	    0,
	    task);

	queue = &purgeable_queues[PURGEABLE_Q_TYPE_FIFO];
	for (group = 0; group < NUM_VOLATILE_GROUPS; group++) {
		num_pages_purged += vm_purgeable_queue_purge_task_owned(queue,
		    group,
		    task);
	}

	queue = &purgeable_queues[PURGEABLE_Q_TYPE_LIFO];
	for (group = 0; group < NUM_VOLATILE_GROUPS; group++) {
		num_pages_purged += vm_purgeable_queue_purge_task_owned(queue,
		    group,
		    task);
	}

	return num_pages_purged;
}

void
vm_purgeable_nonvolatile_enqueue(
	vm_object_t     object,
	task_t          owner)
{
	int ledger_flags;
	kern_return_t kr;

	vm_object_lock_assert_exclusive(object);

	assert(object->purgable == VM_PURGABLE_NONVOLATILE);
	assert(object->vo_owner == NULL);

	lck_mtx_lock(&vm_purgeable_queue_lock);

	if (owner != NULL &&
	    owner->task_objects_disowning) {
		                                                              
		owner = VM_OBJECT_OWNER_DISOWNED;
	}
	if (owner == NULL) {
		owner = kernel_task;
	}
#if DEBUG
	OSBacktrace(&object->purgeable_owner_bt[0],
	    ARRAY_COUNT(object->purgeable_owner_bt));
	object->vo_purgeable_volatilizer = NULL;
#endif            

	ledger_flags = 0;
	if (object->vo_no_footprint) {
		ledger_flags |= VM_LEDGER_FLAG_NO_FOOTPRINT;
	}
	kr = vm_object_ownership_change(object,
	    object->vo_ledger_tag,                                                
	    owner,
	    ledger_flags,
	    FALSE);                                                   
	assert(kr == KERN_SUCCESS);

	assert(object->objq.next == NULL);
	assert(object->objq.prev == NULL);

	queue_enter(&purgeable_nonvolatile_queue, object,
	    vm_object_t, objq);
	assert(purgeable_nonvolatile_count >= 0);
	purgeable_nonvolatile_count++;
	assert(purgeable_nonvolatile_count > 0);
	lck_mtx_unlock(&vm_purgeable_queue_lock);

	vm_object_lock_assert_exclusive(object);
}

void
vm_purgeable_nonvolatile_dequeue(
	vm_object_t     object)
{
	task_t  owner;
	kern_return_t kr;

	vm_object_lock_assert_exclusive(object);

	owner = VM_OBJECT_OWNER(object);
#if DEBUG
	assert(object->vo_purgeable_volatilizer == NULL);
#endif            
	if (owner != NULL) {
		  
                                                 
                     
     
		                                      
		assert(VM_OBJECT_OWNER(object) != kernel_task);
		kr = vm_object_ownership_change(
			object,
			object->vo_ledger_tag,                 
			VM_OBJECT_OWNER_DISOWNED,                
			0,                   
			FALSE);                                  
		assert(kr == KERN_SUCCESS);
		assert(object->vo_owner == VM_OBJECT_OWNER_DISOWNED);
	}

	lck_mtx_lock(&vm_purgeable_queue_lock);
	assert(object->objq.next != NULL);
	assert(object->objq.prev != NULL);
	queue_remove(&purgeable_nonvolatile_queue, object,
	    vm_object_t, objq);
	object->objq.next = NULL;
	object->objq.prev = NULL;
	assert(purgeable_nonvolatile_count > 0);
	purgeable_nonvolatile_count--;
	assert(purgeable_nonvolatile_count >= 0);
	lck_mtx_unlock(&vm_purgeable_queue_lock);

	vm_object_lock_assert_exclusive(object);
}

void
vm_purgeable_accounting(
	vm_object_t     object,
	vm_purgable_t   old_state)
{
	task_t          owner;
	int             resident_page_count;
	int             wired_page_count;
	int             compressed_page_count;
	int             ledger_idx_volatile;
	int             ledger_idx_nonvolatile;
	int             ledger_idx_volatile_compressed;
	int             ledger_idx_nonvolatile_compressed;
	boolean_t       do_footprint;

	vm_object_lock_assert_exclusive(object);
	assert(object->purgable != VM_PURGABLE_DENY);

	owner = VM_OBJECT_OWNER(object);
	if (owner == NULL ||
	    object->purgable == VM_PURGABLE_DENY) {
		return;
	}

	vm_object_ledger_tag_ledgers(object,
	    &ledger_idx_volatile,
	    &ledger_idx_nonvolatile,
	    &ledger_idx_volatile_compressed,
	    &ledger_idx_nonvolatile_compressed,
	    &do_footprint);

	resident_page_count = object->resident_page_count;
	wired_page_count = object->wired_page_count;
	if (VM_CONFIG_COMPRESSOR_IS_PRESENT &&
	    object->pager != NULL) {
		compressed_page_count =
		    vm_compressor_pager_get_count(object->pager);
	} else {
		compressed_page_count = 0;
	}

	if (old_state == VM_PURGABLE_VOLATILE ||
	    old_state == VM_PURGABLE_EMPTY) {
		                                   
		ledger_debit(owner->ledger,
		    ledger_idx_volatile,
		    ptoa_64(resident_page_count - wired_page_count));
		                                              
		ledger_debit(owner->ledger,
		    ledger_idx_volatile_compressed,
		    ptoa_64(compressed_page_count));

		                                       
		ledger_credit(owner->ledger,
		    ledger_idx_nonvolatile,
		    ptoa_64(resident_page_count - wired_page_count));
		                                                  
		ledger_credit(owner->ledger,
		    ledger_idx_nonvolatile_compressed,
		    ptoa_64(compressed_page_count));
		if (do_footprint) {
			                    
			ledger_credit(owner->ledger,
			    task_ledgers.phys_footprint,
			    ptoa_64(resident_page_count
			    + compressed_page_count
			    - wired_page_count));
		}
	} else if (old_state == VM_PURGABLE_NONVOLATILE) {
		                                       
		ledger_debit(owner->ledger,
		    ledger_idx_nonvolatile,
		    ptoa_64(resident_page_count - wired_page_count));
		                                                  
		ledger_debit(owner->ledger,
		    ledger_idx_nonvolatile_compressed,
		    ptoa_64(compressed_page_count));
		if (do_footprint) {
			                    
			ledger_debit(owner->ledger,
			    task_ledgers.phys_footprint,
			    ptoa_64(resident_page_count
			    + compressed_page_count
			    - wired_page_count));
		}

		                                   
		ledger_credit(owner->ledger,
		    ledger_idx_volatile,
		    ptoa_64(resident_page_count - wired_page_count));
		                                              
		ledger_credit(owner->ledger,
		    ledger_idx_volatile_compressed,
		    ptoa_64(compressed_page_count));
	} else {
		panic("vm_purgeable_accounting(%p): "
		    "unexpected old_state=%d\n",
		    object, old_state);
	}

	vm_object_lock_assert_exclusive(object);
}

void
vm_purgeable_nonvolatile_owner_update(
	task_t  owner,
	int     delta)
{
	if (owner == NULL || delta == 0) {
		return;
	}

	if (delta > 0) {
		assert(owner->task_nonvolatile_objects >= 0);
		OSAddAtomic(delta, &owner->task_nonvolatile_objects);
		assert(owner->task_nonvolatile_objects > 0);
	} else {
		assert(owner->task_nonvolatile_objects > delta);
		OSAddAtomic(delta, &owner->task_nonvolatile_objects);
		assert(owner->task_nonvolatile_objects >= 0);
	}
}

void
vm_purgeable_volatile_owner_update(
	task_t  owner,
	int     delta)
{
	if (owner == NULL || delta == 0) {
		return;
	}

	if (delta > 0) {
		assert(owner->task_volatile_objects >= 0);
		OSAddAtomic(delta, &owner->task_volatile_objects);
		assert(owner->task_volatile_objects > 0);
	} else {
		assert(owner->task_volatile_objects > delta);
		OSAddAtomic(delta, &owner->task_volatile_objects);
		assert(owner->task_volatile_objects >= 0);
	}
}

void
vm_object_owner_compressed_update(
	vm_object_t     object,
	int             delta)
{
	task_t          owner;
	int             ledger_idx_volatile;
	int             ledger_idx_nonvolatile;
	int             ledger_idx_volatile_compressed;
	int             ledger_idx_nonvolatile_compressed;
	boolean_t       do_footprint;

	vm_object_lock_assert_exclusive(object);

	owner = VM_OBJECT_OWNER(object);

	if (delta == 0 ||
	    !object->internal ||
	    (object->purgable == VM_PURGABLE_DENY &&
	    !object->vo_ledger_tag) ||
	    owner == NULL) {
		                                                                     
		return;
	}

	vm_object_ledger_tag_ledgers(object,
	    &ledger_idx_volatile,
	    &ledger_idx_nonvolatile,
	    &ledger_idx_volatile_compressed,
	    &ledger_idx_nonvolatile_compressed,
	    &do_footprint);
	switch (object->purgable) {
	case VM_PURGABLE_DENY:
		                                          
		assert(object->vo_ledger_tag != VM_LEDGER_TAG_NONE);
		OS_FALLTHROUGH;
	case VM_PURGABLE_NONVOLATILE:
		if (delta > 0) {
			ledger_credit(owner->ledger,
			    ledger_idx_nonvolatile_compressed,
			    ptoa_64(delta));
			if (do_footprint) {
				ledger_credit(owner->ledger,
				    task_ledgers.phys_footprint,
				    ptoa_64(delta));
			}
		} else {
			ledger_debit(owner->ledger,
			    ledger_idx_nonvolatile_compressed,
			    ptoa_64(-delta));
			if (do_footprint) {
				ledger_debit(owner->ledger,
				    task_ledgers.phys_footprint,
				    ptoa_64(-delta));
			}
		}
		break;
	case VM_PURGABLE_VOLATILE:
	case VM_PURGABLE_EMPTY:
		if (delta > 0) {
			ledger_credit(owner->ledger,
			    ledger_idx_volatile_compressed,
			    ptoa_64(delta));
		} else {
			ledger_debit(owner->ledger,
			    ledger_idx_volatile_compressed,
			    ptoa_64(-delta));
		}
		break;
	default:
		panic("vm_purgeable_compressed_update(): "
		    "unexpected purgable %d for object %p\n",
		    object->purgable, object);
	}
}
