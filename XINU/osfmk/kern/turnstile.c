/*
 * Copyright (c) 2017-2020 Apple Inc. All rights reserved.
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

#include <kern/turnstile.h>
#include <kern/cpu_data.h>
#include <kern/mach_param.h>
#include <kern/kern_types.h>
#include <kern/assert.h>
#include <kern/kalloc.h>
#include <kern/thread.h>
#include <kern/clock.h>
#include <kern/policy_internal.h>
#include <kern/task.h>
#include <kern/waitq.h>
#include <kern/sched_prim.h>
#include <kern/zalloc.h>
#include <kern/debug.h>
#include <machine/limits.h>
#include <machine/atomic.h>

#include <pexpert/pexpert.h>
#include <os/hash.h>
#include <libkern/section_keywords.h>

static TUNABLE(int, turnstile_max_hop, "turnstile_max_hop", TURNSTILE_MAX_HOP_DEFAULT);
static ZONE_DECLARE(turnstiles_zone, "turnstiles", sizeof(struct turnstile), ZC_NONE);

static struct mpsc_daemon_queue turnstile_deallocate_queue;
#define TURNSTILES_CHUNK (THREAD_CHUNK)

                                                                          
static const turnstile_promote_policy_t turnstile_promote_policy[TURNSTILE_TOTAL_TYPES] = {
	[TURNSTILE_NONE]          = TURNSTILE_PROMOTE_NONE,
	[TURNSTILE_KERNEL_MUTEX]  = TURNSTILE_KERNEL_PROMOTE,
	[TURNSTILE_ULOCK]         = TURNSTILE_USER_PROMOTE,
	[TURNSTILE_PTHREAD_MUTEX] = TURNSTILE_USER_PROMOTE,
	[TURNSTILE_SYNC_IPC]      = TURNSTILE_USER_IPC_PROMOTE,
	[TURNSTILE_WORKLOOPS]     = TURNSTILE_USER_IPC_PROMOTE,
	[TURNSTILE_WORKQS]        = TURNSTILE_USER_IPC_PROMOTE,
	[TURNSTILE_KNOTE]         = TURNSTILE_USER_IPC_PROMOTE,
	[TURNSTILE_SLEEP_INHERITOR] = TURNSTILE_KERNEL_PROMOTE,
};

                                                                            
static const turnstile_hash_lock_policy_t turnstile_hash_lock_policy[TURNSTILE_TOTAL_TYPES] = {
	[TURNSTILE_NONE]          = TURNSTILE_HASH_LOCK_POLICY_NONE,
	[TURNSTILE_KERNEL_MUTEX]  = TURNSTILE_HASH_LOCK_POLICY_NONE,
	[TURNSTILE_ULOCK]         = TURNSTILE_HASH_LOCK_POLICY_NONE,
	[TURNSTILE_PTHREAD_MUTEX] = TURNSTILE_HASH_LOCK_POLICY_NONE,
	[TURNSTILE_SYNC_IPC]      = TURNSTILE_HASH_LOCK_POLICY_NONE,
	[TURNSTILE_WORKLOOPS]     = TURNSTILE_HASH_LOCK_POLICY_NONE,
	[TURNSTILE_WORKQS]        = TURNSTILE_HASH_LOCK_POLICY_NONE,
	[TURNSTILE_KNOTE]         = TURNSTILE_HASH_LOCK_POLICY_NONE,
	[TURNSTILE_SLEEP_INHERITOR] = (TURNSTILE_IRQ_UNSAFE_HASH | TURNSTILE_LOCKED_HASH),
};

os_refgrp_decl(static, turnstile_refgrp, "turnstile", NULL);

#if DEVELOPMENT || DEBUG
static queue_head_t turnstiles_list;

static LCK_GRP_DECLARE(turnstiles_dev_lock_grp, "turnstile_dev_lock");
static LCK_SPIN_DECLARE(global_turnstile_lock, &turnstiles_dev_lock_grp);

#define global_turnstiles_lock() \
	lck_spin_lock_grp(&global_turnstile_lock, &turnstiles_dev_lock_grp)
#define global_turnstiles_lock_try() \
	lck_spin_try_lock_grp(&global_turnstile_lock, &turnstiles_dev_lock_grp)
#define global_turnstiles_unlock() \
	lck_spin_unlock(&global_turnstile_lock)

                                                 
static struct turnstile_stats turnstile_boost_stats[TURNSTILE_MAX_HOP_DEFAULT] = {};
static struct turnstile_stats turnstile_unboost_stats[TURNSTILE_MAX_HOP_DEFAULT] = {};
uint64_t thread_block_on_turnstile_count;
uint64_t thread_block_on_regular_waitq_count;
#endif                           

#ifndef max
#define max(a, b)        (((a) > (b)) ? (a) : (b))
#endif          

                                  
static turnstile_type_t
turnstile_get_type(struct turnstile *turnstile);
static bool
turnstile_is_send_turnstile(struct turnstile *turnstile);
static uint32_t
turnstile_get_gencount(struct turnstile *turnstile);
static void
turnstile_set_type_and_increment_gencount(struct turnstile *turnstile, turnstile_type_t type);
static void
turnstile_init(struct turnstile *turnstile);
static void
turnstile_update_inheritor_workq_priority_chain(struct turnstile *in_turnstile, spl_t s);
static void
turnstile_update_inheritor_thread_priority_chain(struct turnstile **in_turnstile,
    thread_t *out_thread, int total_hop, turnstile_stats_update_flags_t tsu_flags);
static void
turnstile_update_inheritor_turnstile_priority_chain(struct turnstile **in_out_turnstile,
    int total_hop, turnstile_stats_update_flags_t tsu_flags);
static void
thread_update_waiting_turnstile_priority_chain(thread_t *in_thread,
    struct turnstile **out_turnstile, int thread_hop, int total_hop,
    turnstile_stats_update_flags_t tsu_flags);
static boolean_t
turnstile_update_turnstile_promotion_locked(struct turnstile *dst_turnstile,
    struct turnstile *src_turnstile);
static boolean_t
turnstile_update_turnstile_promotion(struct turnstile *dst_turnstile,
    struct turnstile *src_turnstile);
static boolean_t
turnstile_need_turnstile_promotion_update(struct turnstile *dst_turnstile,
    struct turnstile *src_turnstile);
static boolean_t
turnstile_add_turnstile_promotion(struct turnstile *dst_turnstile,
    struct turnstile *src_turnstile);
static boolean_t
turnstile_remove_turnstile_promotion(struct turnstile *dst_turnstile,
    struct turnstile *src_turnstile);
static boolean_t
turnstile_update_thread_promotion_locked(struct turnstile *dst_turnstile,
    thread_t thread);
static boolean_t
turnstile_need_thread_promotion_update(struct turnstile *dst_turnstile,
    thread_t thread);
static boolean_t
thread_add_turnstile_promotion(
	thread_t thread, struct turnstile *turnstile);
static boolean_t
thread_remove_turnstile_promotion(
	thread_t thread, struct turnstile *turnstile);
static boolean_t
thread_needs_turnstile_promotion_update(thread_t thread,
    struct turnstile *turnstile);
static boolean_t
thread_update_turnstile_promotion(
	thread_t thread, struct turnstile *turnstile);
static boolean_t
thread_update_turnstile_promotion_locked(
	thread_t thread, struct turnstile *turnstile);
static boolean_t
workq_add_turnstile_promotion(
	struct workqueue *wq_inheritor, struct turnstile *turnstile);
static turnstile_stats_update_flags_t
thread_get_update_flags_for_turnstile_propagation_stoppage(thread_t thread);
static turnstile_stats_update_flags_t
turnstile_get_update_flags_for_above_UI_pri_change(struct turnstile *turnstile);
static void turnstile_stash_inheritor(turnstile_inheritor_t new_inheritor,
    turnstile_update_flags_t flags);
static int turnstile_compute_thread_push(struct turnstile *turnstile, thread_t thread);

#if DEVELOPMENT || DEBUG
                                                           
struct tstile_test_prim {
	struct turnstile *ttprim_turnstile;
	thread_t ttprim_owner;
	lck_spin_t ttprim_interlock;
	uint32_t tt_prim_waiters;
};

struct tstile_test_prim *test_prim_ts_inline;
struct tstile_test_prim *test_prim_global_htable;
struct tstile_test_prim *test_prim_global_ts_kernel;
struct tstile_test_prim *test_prim_global_ts_kernel_hash;

static void
tstile_test_prim_init(struct tstile_test_prim **test_prim_ptr);
#endif

union turnstile_type_gencount {
	uint32_t value;
	struct {
		uint32_t ts_type:(8 * sizeof(turnstile_type_t)),
		ts_gencount: (8 * (sizeof(uint32_t) - sizeof(turnstile_type_t)));
	};
};

static turnstile_type_t
turnstile_get_type(struct turnstile *turnstile)
{
	union turnstile_type_gencount type_and_gencount;

	type_and_gencount.value = atomic_load_explicit(&turnstile->ts_type_gencount, memory_order_relaxed);
	return (turnstile_type_t) type_and_gencount.ts_type;
}

                                                   
static bool
turnstile_is_send_turnstile(struct turnstile *turnstile)
{
	if (not_in_kdp) {
		panic("turnstile_is_send_turnstile() called outside of kernel debugger context");
	}

	if (turnstile_get_type(turnstile) == TURNSTILE_SYNC_IPC) {
		ipc_port_t port = (ipc_port_t) turnstile->ts_proprietor;

		return port_send_turnstile(port) == turnstile;
	}

	return false;
}

                                                   
static bool
turnstile_is_receive_turnstile(struct turnstile *turnstile)
{
	if (not_in_kdp) {
		panic("turnstile_is_receive_turnstile() called outside of kernel debugger context");
	}

	if (turnstile_get_type(turnstile) == TURNSTILE_SYNC_IPC) {
		ipc_port_t port = (ipc_port_t) turnstile->ts_proprietor;

		return *port_rcv_turnstile_address(port) == turnstile;
	}

	return false;
}

static uint32_t
turnstile_get_gencount(struct turnstile *turnstile)
{
	union turnstile_type_gencount type_and_gencount;

	type_and_gencount.value = atomic_load_explicit(&turnstile->ts_type_gencount, memory_order_relaxed);
	return (uint32_t) type_and_gencount.ts_gencount;
}

static void
turnstile_set_type_and_increment_gencount(struct turnstile *turnstile, turnstile_type_t type)
{
	union turnstile_type_gencount type_and_gencount;

	                                                                                          
	type_and_gencount.value = atomic_load_explicit(&turnstile->ts_type_gencount, memory_order_relaxed);
	type_and_gencount.ts_type = type;
	type_and_gencount.ts_gencount++;
	atomic_store_explicit(&turnstile->ts_type_gencount, type_and_gencount.value, memory_order_relaxed);
}


                                        

  
                                                                                
                                                                      
                                                                           
                                
   
#define TURNSTILE_HTABLE_BUCKETS_DEFAULT   32
#define TURNSTILE_HTABLE_BUCKETS_MAX       1024

SLIST_HEAD(turnstile_hashlist, turnstile);

struct turnstile_htable_bucket {
	lck_spin_t                    ts_ht_bucket_lock;
	struct turnstile_hashlist     ts_ht_bucket_list;
};

SECURITY_READ_ONLY_LATE(static uint32_t) ts_htable_buckets;
                                                                      
SECURITY_READ_ONLY_LATE(static struct turnstile_htable_bucket *)turnstile_htable_irq_safe;
                                                                     
SECURITY_READ_ONLY_LATE(static struct turnstile_htable_bucket *)turnstile_htable;


                                          
LCK_GRP_DECLARE(turnstiles_htable_lock_grp, "turnstiles_htable_locks");

#define turnstile_bucket_lock_init(bucket) \
	lck_spin_init(&bucket->ts_ht_bucket_lock, &turnstiles_htable_lock_grp, LCK_ATTR_NULL)
#define turnstile_bucket_lock(bucket) \
	lck_spin_lock_grp(&bucket->ts_ht_bucket_lock, &turnstiles_htable_lock_grp)
#define turnstile_bucket_unlock(bucket) \
	lck_spin_unlock(&bucket->ts_ht_bucket_lock)

#define kdp_turnstile_bucket_is_locked(bucket) \
	kdp_lck_spin_is_acquired(&bucket->ts_ht_bucket_lock)

  
                                  
  
                                                            
  
        
         
  
           
         
   
static void
turnstiles_hashtable_init(void)
{
	                                                   
	if (PE_parse_boot_argn("ts_htable_buckets", &ts_htable_buckets, sizeof(ts_htable_buckets)) != TRUE) {
		ts_htable_buckets = TURNSTILE_HTABLE_BUCKETS_DEFAULT;
	}

	assert(ts_htable_buckets <= TURNSTILE_HTABLE_BUCKETS_MAX);
	uint32_t ts_htable_size = ts_htable_buckets * sizeof(struct turnstile_htable_bucket);
	turnstile_htable_irq_safe = zalloc_permanent(ts_htable_size, ZALIGN_PTR);
	if (turnstile_htable_irq_safe == NULL) {
		panic("Turnstiles hash table memory allocation failed!");
	}

	turnstile_htable = zalloc_permanent(ts_htable_size, ZALIGN_PTR);
	if (turnstile_htable == NULL) {
		panic("Turnstiles hash table memory allocation failed!");
	}

	                                                  
	for (uint32_t i = 0; i < ts_htable_buckets; i++) {
		struct turnstile_htable_bucket *ts_bucket = &(turnstile_htable_irq_safe[i]);
		turnstile_bucket_lock_init(ts_bucket);
		SLIST_INIT(&ts_bucket->ts_ht_bucket_list);

		ts_bucket = &(turnstile_htable[i]);
		turnstile_bucket_lock_init(ts_bucket);
		SLIST_INIT(&ts_bucket->ts_ht_bucket_list);
	}
}

  
                                 
  
                                                           
                                                            
  
        
                    
  
           
                                               
   
static inline boolean_t
turnstile_freelist_empty(
	struct turnstile *ts)
{
	return SLIST_EMPTY(&ts->ts_free_turnstiles);
}


  
                                  
  
                                                                            
                                                            
  
        
                              
                                           
  
           
         
   
static void
turnstile_freelist_insert(
	struct turnstile *dst_ts,
	struct turnstile *free_ts)
{
	assert(turnstile_get_type(dst_ts) == turnstile_get_type(free_ts));
	assert(dst_ts->ts_proprietor == free_ts->ts_proprietor);
	turnstile_state_add(free_ts, TURNSTILE_STATE_FREELIST);
	SLIST_INSERT_HEAD(&dst_ts->ts_free_turnstiles, free_ts, ts_free_elm);
}

  
                                  
  
                                                                    
                                                            
  
        
                              
  
           
                                        
   
static struct turnstile *
turnstile_freelist_remove(
	struct turnstile *ts)
{
	struct turnstile *ret_turnstile = TURNSTILE_NULL;
	assert(!SLIST_EMPTY(&ts->ts_free_turnstiles));
	ret_turnstile = SLIST_FIRST(&ts->ts_free_turnstiles);
	SLIST_REMOVE_HEAD(&ts->ts_free_turnstiles, ts_free_elm);
	assert(ret_turnstile != TURNSTILE_NULL);
	turnstile_state_remove(ret_turnstile, TURNSTILE_STATE_FREELIST);
	                                                                        
	SLIST_INIT(&ret_turnstile->ts_free_turnstiles);
	return ret_turnstile;
}

  
                       
  
                                                                       
  
        
                                       
  
           
                                                    
   
static inline uint32_t
turnstile_hash(uintptr_t proprietor)
{
	uint32_t hash = os_hash_kernel_pointer((void *)proprietor);
	return hash & (ts_htable_buckets - 1);
}

static inline struct turnstile_htable_bucket *
turnstile_get_bucket(uint32_t index, turnstile_type_t type)
{
	struct turnstile_htable_bucket *ts_bucket;
	int hash_policy = turnstile_hash_lock_policy[type];

	if (hash_policy & TURNSTILE_IRQ_UNSAFE_HASH) {
		ts_bucket = &(turnstile_htable[index]);
	} else {
		ts_bucket = &(turnstile_htable_irq_safe[index]);
	}

	return ts_bucket;
}

  
                                   
  
                                                                       
                                                                         
                                                            
                                                                              
  
        
                                       
                                           
                         
  
                                                                            
   
unsigned
turnstile_hash_bucket_lock(uintptr_t proprietor, uint32_t *index_proprietor, turnstile_type_t type)
{
	struct turnstile_htable_bucket *ts_bucket;
	int hash_policy = turnstile_hash_lock_policy[type];
	bool irq_safe = !(hash_policy & TURNSTILE_IRQ_UNSAFE_HASH);
	spl_t ret = 0;
	uint32_t index;

	  
                                                           
                                         
                                              
    
	if (proprietor) {
		index = turnstile_hash(proprietor);
		*index_proprietor = index;
	} else {
		index = *index_proprietor;
	}

	ts_bucket = turnstile_get_bucket(index, type);

	if (irq_safe) {
		ret = splsched();
	}

	turnstile_bucket_lock(ts_bucket);

	return ret;
}

  
                                     
  
                                                                         
                                                                         
                                                            
                                                                              
  
        
                                       
                                           
                         
                                                           
  
   
void
turnstile_hash_bucket_unlock(uintptr_t proprietor, uint32_t *index_proprietor, turnstile_type_t type, unsigned s)
{
	struct turnstile_htable_bucket *ts_bucket;
	int hash_policy = turnstile_hash_lock_policy[type];
	bool irq_safe = !(hash_policy & TURNSTILE_IRQ_UNSAFE_HASH);
	uint32_t index;

	  
                                                           
                                         
                                              
    
	if (proprietor) {
		index = turnstile_hash(proprietor);
		*index_proprietor = index;
	} else {
		index = *index_proprietor;
	}
	ts_bucket = turnstile_get_bucket(index, type);

	turnstile_bucket_unlock(ts_bucket);
	if (irq_safe) {
		splx(s);
	}
}

  
                                    
  
                                                                         
                                                                                      
                                                                          
                                                                                  
                                                                
  
        
                     
                                      
                                
  
           
                                                        
   
static struct turnstile *
turnstile_htable_lookup_add(
	uintptr_t proprietor,
	struct turnstile *new_turnstile,
	turnstile_type_t type)
{
	uint32_t index = turnstile_hash(proprietor);
	assert(index < ts_htable_buckets);
	struct turnstile_htable_bucket *ts_bucket;
	int hash_policy = turnstile_hash_lock_policy[type];
	bool needs_lock = !(hash_policy & TURNSTILE_LOCKED_HASH);
	bool irq_safe = !(hash_policy & TURNSTILE_IRQ_UNSAFE_HASH);
	spl_t s;

	ts_bucket = turnstile_get_bucket(index, type);

	if (needs_lock) {
		if (irq_safe) {
			s = splsched();
		}
		turnstile_bucket_lock(ts_bucket);
	}

	struct turnstile *ts;

	SLIST_FOREACH(ts, &ts_bucket->ts_ht_bucket_list, ts_htable_link) {
		if (ts->ts_proprietor == proprietor) {
			  
                                                                                           
                               
      
			if (needs_lock) {
				turnstile_bucket_unlock(ts_bucket);
				if (irq_safe) {
					splx(s);
				}
			}
			turnstile_freelist_insert(ts, new_turnstile);
			return ts;
		}
	}

	                                                                           
	SLIST_INSERT_HEAD(&ts_bucket->ts_ht_bucket_list, new_turnstile, ts_htable_link);
	turnstile_state_add(new_turnstile, TURNSTILE_STATE_HASHTABLE);
	if (needs_lock) {
		turnstile_bucket_unlock(ts_bucket);
		if (irq_safe) {
			splx(s);
		}
	}
	                                                                                  
	return TURNSTILE_NULL;
}

  
                                        
  
                                                                         
                                                                                         
                                                                                         
                                              
                                                                                  
                                                                
  
        
                     
                                        
                                
  
           
                                                                     
   
static struct turnstile *
turnstable_htable_lookup_remove(
	uintptr_t proprietor,
	struct turnstile **free_turnstile,
	turnstile_type_t type)
{
	uint32_t index = turnstile_hash(proprietor);
	assert(index < ts_htable_buckets);
	struct turnstile_htable_bucket *ts_bucket;
	struct turnstile *ret_turnstile = TURNSTILE_NULL;
	int hash_policy = turnstile_hash_lock_policy[type];
	bool needs_lock = !(hash_policy & TURNSTILE_LOCKED_HASH);
	bool irq_safe = !(hash_policy & TURNSTILE_IRQ_UNSAFE_HASH);
	spl_t s;

	ts_bucket = turnstile_get_bucket(index, type);

	if (needs_lock) {
		if (irq_safe) {
			s = splsched();
		}
		turnstile_bucket_lock(ts_bucket);
	}

	struct turnstile *ts, **prev_tslink;
	                                                                  
	SLIST_FOREACH_PREVPTR(ts, prev_tslink, &ts_bucket->ts_ht_bucket_list, ts_htable_link) {
		if (ts->ts_proprietor == proprietor) {
			ret_turnstile = ts;
			break;
		}
	}
	assert(ret_turnstile != TURNSTILE_NULL);

	                                                               
	if (turnstile_freelist_empty(ret_turnstile)) {
		                                                                                              
		*prev_tslink = SLIST_NEXT(ret_turnstile, ts_htable_link);
		turnstile_state_remove(ret_turnstile, TURNSTILE_STATE_HASHTABLE);
		if (needs_lock) {
			turnstile_bucket_unlock(ts_bucket);
			if (irq_safe) {
				splx(s);
			}
		}
		*free_turnstile = ret_turnstile;
		return TURNSTILE_NULL;
	} else {
		  
                                                                             
                                                                         
     
		if (needs_lock) {
			turnstile_bucket_unlock(ts_bucket);
			if (irq_safe) {
				splx(s);
			}
		}
		*free_turnstile = turnstile_freelist_remove(ret_turnstile);
		return ret_turnstile;
	}
}

  
                                
  
                                                                         
                                                                                  
                                                                
  
        
                     
                                
  
           
                                               
   
static struct turnstile *
turnstile_htable_lookup(
	uintptr_t proprietor,
	turnstile_type_t type)
{
	uint32_t index = turnstile_hash(proprietor);
	assert(index < ts_htable_buckets);
	bool kdp_ctx = !not_in_kdp;
	struct turnstile_htable_bucket *ts_bucket = turnstile_get_bucket(index, type);
	int hash_policy = turnstile_hash_lock_policy[type];
	bool needs_lock = !(hash_policy & TURNSTILE_LOCKED_HASH);
	bool irq_safe = !(hash_policy & TURNSTILE_IRQ_UNSAFE_HASH);
	spl_t s;

	if (needs_lock) {
		if (irq_safe && !kdp_ctx) {
			s = splsched();
		}

		if (kdp_ctx) {
			if (kdp_turnstile_bucket_is_locked(ts_bucket)) {
				                                                                      
				return TURNSTILE_NULL;
			}
		} else {
			turnstile_bucket_lock(ts_bucket);
		}
	}
	struct turnstile *ts = TURNSTILE_NULL;
	struct turnstile *ret_turnstile = TURNSTILE_NULL;

	SLIST_FOREACH(ts, &ts_bucket->ts_ht_bucket_list, ts_htable_link) {
		if (ts->ts_proprietor == proprietor) {
			                                                         
			ret_turnstile = ts;
			break;
		}
	}

	if (needs_lock && !kdp_ctx) {
		turnstile_bucket_unlock(ts_bucket);
		if (irq_safe) {
			splx(s);
		}
	}

	return ret_turnstile;
}

  
                                          
  
                                                                           
                     
  
                                    
                                                                               
                 
  
                 
   
static void
turnstile_deallocate_queue_invoke(mpsc_queue_chain_t e,
    __assert_only mpsc_daemon_queue_t dq)
{
	struct turnstile *ts;

	ts = mpsc_queue_element(e, struct turnstile, ts_deallocate_link);
	assert(dq == &turnstile_deallocate_queue);
	turnstile_destroy(ts);
}

  
                        
  
                                                
  
              
  
                 
   
void
turnstiles_init(void)
{
	turnstiles_hashtable_init();

	thread_deallocate_daemon_register_queue(&turnstile_deallocate_queue,
	    turnstile_deallocate_queue_invoke);

#if DEVELOPMENT || DEBUG
	queue_init(&turnstiles_list);

	                                         
	tstile_test_prim_init(&test_prim_ts_inline);
	tstile_test_prim_init(&test_prim_global_htable);
	tstile_test_prim_init(&test_prim_global_ts_kernel);
	tstile_test_prim_init(&test_prim_global_ts_kernel_hash);
#endif
}

  
                        
  
                                     
  
              
  
           
                          
   
struct turnstile *
turnstile_alloc(void)
{
	struct turnstile *turnstile = TURNSTILE_NULL;

	turnstile = zalloc(turnstiles_zone);
	turnstile_init(turnstile);

#if DEVELOPMENT || DEBUG
	                                  
	global_turnstiles_lock();
	queue_enter(&turnstiles_list, turnstile,
	    struct turnstile *, ts_global_elm);
	global_turnstiles_unlock();
#endif
	return turnstile;
}

  
                       
  
                                         
  
        
                                  
  
                 
   
static void
turnstile_init(struct turnstile *turnstile)
{
	kern_return_t kret;

	                          
	kret = waitq_init(&turnstile->ts_waitq, SYNC_POLICY_DISABLE_IRQ | SYNC_POLICY_REVERSED |
	    SYNC_POLICY_TURNSTILE);
	assert(kret == KERN_SUCCESS);

	turnstile->ts_inheritor = TURNSTILE_INHERITOR_NULL;
	SLIST_INIT(&turnstile->ts_free_turnstiles);
	os_atomic_init(&turnstile->ts_type_gencount, 0);
	turnstile_set_type_and_increment_gencount(turnstile, TURNSTILE_NONE);
	turnstile_state_init(turnstile, TURNSTILE_STATE_THREAD);
	os_ref_init_count(&turnstile->ts_refcount, &turnstile_refgrp, 1);
	turnstile->ts_proprietor = TURNSTILE_PROPRIETOR_NULL;
	turnstile->ts_priority = 0;
	turnstile->ts_inheritor_flags = TURNSTILE_UPDATE_FLAGS_NONE;
	turnstile->ts_port_ref = 0;
	priority_queue_init(&turnstile->ts_inheritor_queue);

#if DEVELOPMENT || DEBUG
	turnstile->ts_thread = current_thread();
	turnstile->ts_prev_thread = NULL;
#endif
}

  
                            
  
                                                  
  
                  
  
                 
   
void
turnstile_reference(struct turnstile *turnstile)
{
	if (turnstile == TURNSTILE_NULL) {
		return;
	}
	os_ref_retain(&turnstile->ts_refcount);
}

  
                             
  
                                                  
                                                      
  
                  
  
                 
   
void
turnstile_deallocate(struct turnstile *turnstile)
{
	if (turnstile == TURNSTILE_NULL) {
		return;
	}

	if (__improbable(os_ref_release(&turnstile->ts_refcount) == 0)) {
		turnstile_destroy(turnstile);
	}
}

  
                                  
  
                                                                                  
  
                  
  
                 
   
void
turnstile_deallocate_safe(struct turnstile *turnstile)
{
	if (turnstile == TURNSTILE_NULL) {
		return;
	}

	if (__improbable(os_ref_release(&turnstile->ts_refcount) == 0)) {
		mpsc_daemon_enqueue(&turnstile_deallocate_queue,
		    &turnstile->ts_deallocate_link, MPSC_QUEUE_DISABLE_PREEMPTION);
	}
}

  
                          
  
                                          
  
        
                    
  
                 
   
void
turnstile_destroy(struct turnstile *turnstile)
{
	                       
	waitq_deinit(&turnstile->ts_waitq);

	assert(turnstile->ts_inheritor == TURNSTILE_INHERITOR_NULL);
	assert(SLIST_EMPTY(&turnstile->ts_free_turnstiles));
	assert(turnstile->ts_state & TURNSTILE_STATE_THREAD);
#if DEVELOPMENT || DEBUG
	                                       
	global_turnstiles_lock();
	queue_remove(&turnstiles_list, turnstile,
	    struct turnstile *, ts_global_elm);
	global_turnstiles_unlock();
#endif
	zfree(turnstiles_zone, turnstile);
}

  
                          
  
                                                                                             
                                                                                     
                                                                                                                  
                                                                                                                  
                                                      
  
        
                     
                                                         
                                                             
                            
  
           
               
   
struct turnstile *
turnstile_prepare(
	uintptr_t proprietor,
	struct turnstile **tstore,
	struct turnstile *turnstile,
	turnstile_type_t type)
{
	thread_t thread = current_thread();
	struct turnstile *ret_turnstile = TURNSTILE_NULL;
	struct turnstile *thread_turnstile = turnstile;

	                                                         
	if (thread_turnstile == TURNSTILE_NULL) {
		thread_turnstile = thread->turnstile;
		assert(thread_turnstile != TURNSTILE_NULL);
		assert(thread->inheritor == NULL);
		thread->turnstile = TURNSTILE_NULL;
	}

	                                                                
	SLIST_INIT(&thread_turnstile->ts_free_turnstiles);
	turnstile_set_type_and_increment_gencount(thread_turnstile, type);
	thread_turnstile->ts_inheritor = TURNSTILE_INHERITOR_NULL;
	thread_turnstile->ts_proprietor = proprietor;
	turnstile_state_remove(thread_turnstile, TURNSTILE_STATE_THREAD);

	thread_turnstile->ts_priority = 0;
#if DEVELOPMENT || DEBUG
	thread_turnstile->ts_prev_thread = thread_turnstile->ts_thread;
	thread_turnstile->ts_thread = NULL;
#endif

	if (tstore != NULL) {
		  
                                           
                                                                                                         
               
                                                                           
     
		ret_turnstile = *tstore;
		if (*tstore == TURNSTILE_NULL) {
			turnstile_state_add(thread_turnstile, TURNSTILE_STATE_PROPRIETOR);
			*tstore = thread_turnstile;
			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    (TURNSTILE_CODE(TURNSTILE_FREELIST_OPERATIONS, (TURNSTILE_PREPARE))) | DBG_FUNC_NONE,
			    VM_KERNEL_UNSLIDE_OR_PERM(thread_turnstile),
			    VM_KERNEL_UNSLIDE_OR_PERM(proprietor),
			    turnstile_get_type(thread_turnstile), 0, 0);
		} else {
			turnstile_freelist_insert(ret_turnstile, thread_turnstile);
		}
		ret_turnstile = *tstore;
	} else {
		  
                                                                                         
     
		ret_turnstile = turnstile_htable_lookup_add(proprietor, thread_turnstile, type);
		if (ret_turnstile == NULL) {
			ret_turnstile = thread_turnstile;
			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    (TURNSTILE_CODE(TURNSTILE_FREELIST_OPERATIONS, (TURNSTILE_PREPARE))) | DBG_FUNC_NONE,
			    VM_KERNEL_UNSLIDE_OR_PERM(thread_turnstile),
			    VM_KERNEL_UNSLIDE_OR_PERM(proprietor),
			    turnstile_get_type(thread_turnstile), 0, 0);
		}
	}

	return ret_turnstile;
}

  
                           
  
                                                                                           
                                                                                    
  
        
                     
                                                          
                                                                                    
                            
  
           
          
   
void
turnstile_complete(
	uintptr_t proprietor,
	struct turnstile **tstore,
	struct turnstile **out_turnstile,
	turnstile_type_t type)
{
	thread_t thread = current_thread();
	struct turnstile *primitive_turnstile = TURNSTILE_NULL;
	struct turnstile *thread_turnstile = TURNSTILE_NULL;

	assert(thread->inheritor == NULL);

	if (tstore != NULL) {
		  
                                                                            
                                        
     
		assert(*tstore != TURNSTILE_NULL);
		if (turnstile_freelist_empty(*tstore)) {
			                                                              
			thread_turnstile = *tstore;
			*tstore = TURNSTILE_NULL;
			turnstile_state_remove(thread_turnstile, TURNSTILE_STATE_PROPRIETOR);
		} else {
			                                                           
			thread_turnstile = turnstile_freelist_remove(*tstore);
		}
		primitive_turnstile = *tstore;
	} else {
		                                                        
		primitive_turnstile = turnstable_htable_lookup_remove(proprietor, &thread_turnstile, type);
	}
	if (primitive_turnstile == NULL) {
		  
                                                                             
                                                                              
                                                 
     
		if (thread_turnstile->ts_inheritor != TURNSTILE_INHERITOR_NULL) {
			turnstile_update_inheritor(thread_turnstile, TURNSTILE_INHERITOR_NULL,
			    (TURNSTILE_IMMEDIATE_UPDATE | TURNSTILE_INHERITOR_THREAD));
			  
                                                                        
                                           
      
		}
		assert(thread_turnstile->ts_inheritor == TURNSTILE_INHERITOR_NULL);

		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (TURNSTILE_CODE(TURNSTILE_FREELIST_OPERATIONS, (TURNSTILE_COMPLETE))) | DBG_FUNC_NONE,
		    VM_KERNEL_UNSLIDE_OR_PERM(thread_turnstile),
		    VM_KERNEL_UNSLIDE_OR_PERM(proprietor),
		    turnstile_get_type(thread_turnstile), 0, 0);
	} else {
		                                                                                     
		if (turnstile_recompute_priority(primitive_turnstile)) {
			turnstile_reference(primitive_turnstile);
			thread->inheritor = primitive_turnstile;
			thread->inheritor_flags = (TURNSTILE_INHERITOR_TURNSTILE |
			    TURNSTILE_INHERITOR_NEEDS_PRI_UPDATE);
		}
	}

	turnstile_set_type_and_increment_gencount(thread_turnstile, TURNSTILE_NONE);
#if DEVELOPMENT || DEBUG
	thread_turnstile->ts_prev_thread = NULL;
	thread_turnstile->ts_thread = thread;
#endif

	turnstile_state_add(thread_turnstile, TURNSTILE_STATE_THREAD);
	if (out_turnstile == NULL) {
		                                                            
		thread->turnstile = thread_turnstile;
	} else {
		*out_turnstile = thread_turnstile;
	}
	return;
}

  
                                                         
  
                                                                
                        
        
                    
                        
                
  
                               
   
void
turnstile_kernel_update_inheritor_on_wake_locked(
	struct turnstile *turnstile,
	turnstile_inheritor_t new_inheritor,
	turnstile_update_flags_t flags __assert_only)
{
	                                                                      
	__assert_only turnstile_promote_policy_t policy =
	    turnstile_promote_policy[turnstile_get_type(turnstile)];

	assert(flags & TURNSTILE_INHERITOR_THREAD);
	assert(policy == TURNSTILE_KERNEL_PROMOTE || policy == TURNSTILE_USER_PROMOTE);

	turnstile_stash_inheritor((thread_t)new_inheritor, TURNSTILE_INHERITOR_THREAD);
	  
                                                                 
                                                             
                                                               
                                 
    
	turnstile_recompute_priority_locked(turnstile);
	turnstile_update_inheritor_locked(turnstile);
}

  
                                          
  
                                                                   
                                                        
  
        
                    
                                                                            
  
           
                                                                    
   
void
turnstile_update_inheritor_locked(
	struct turnstile *turnstile)
{
	turnstile_inheritor_t old_inheritor = turnstile->ts_inheritor;
	turnstile_update_flags_t old_inheritor_flags = turnstile->ts_inheritor_flags;
	thread_t thread = current_thread();
	boolean_t old_inheritor_needs_update = FALSE;
	boolean_t new_inheritor_needs_update = FALSE;
	turnstile_stats_update_flags_t tsu_flags =
	    turnstile_get_update_flags_for_above_UI_pri_change(turnstile);

	assert(waitq_held(&turnstile->ts_waitq));

	  
                                                     
                                                               
    
	turnstile_inheritor_t new_inheritor = thread->inheritor;
	turnstile_update_flags_t new_inheritor_flags = thread->inheritor_flags;

	switch (turnstile_promote_policy[turnstile_get_type(turnstile)]) {
	case TURNSTILE_USER_PROMOTE:
	case TURNSTILE_USER_IPC_PROMOTE:
		break;
	case TURNSTILE_KERNEL_PROMOTE:
		                                                                        
		if (old_inheritor) {
			assert(old_inheritor_flags & TURNSTILE_INHERITOR_THREAD);
		}

		if (new_inheritor) {
			assert(new_inheritor_flags & TURNSTILE_INHERITOR_THREAD);
		}

		break;
	default:
		panic("turnstile promotion for type %d not yet implemented", turnstile_get_type(turnstile));
	}

	                               
	if (old_inheritor == new_inheritor && old_inheritor == NULL) {
		goto done;
	}

	if (old_inheritor == new_inheritor) {
		if (new_inheritor_flags & TURNSTILE_INHERITOR_THREAD) {
			thread_t thread_inheritor = (thread_t)new_inheritor;

			assert(old_inheritor_flags & TURNSTILE_INHERITOR_THREAD);

			                                                              
			new_inheritor_needs_update = thread_update_turnstile_promotion(
				thread_inheritor, turnstile);
		} else if (new_inheritor_flags & TURNSTILE_INHERITOR_TURNSTILE) {
			struct turnstile *inheritor_turnstile = new_inheritor;

			assert(old_inheritor_flags & TURNSTILE_INHERITOR_TURNSTILE);

			new_inheritor_needs_update = turnstile_update_turnstile_promotion(
				inheritor_turnstile, turnstile);
		} else if (new_inheritor_flags & TURNSTILE_INHERITOR_WORKQ) {
			  
                                                            
                                                             
                                                
      
			turnstile_stats_update(1, TSU_NO_PRI_CHANGE_NEEDED |
			    TSU_TURNSTILE_ARG | TSU_BOOST_ARG, turnstile);
		} else {
			panic("Inheritor flags lost along the way");
		}

		                            
		if (!new_inheritor_needs_update) {
			turnstile_stats_update(1, TSU_PRI_PROPAGATION |
			    TSU_TURNSTILE_ARG | TSU_BOOST_ARG | tsu_flags, turnstile);
		}
		goto done;
	}

	if (old_inheritor != NULL) {
		if (old_inheritor_flags & TURNSTILE_INHERITOR_THREAD) {
			thread_t thread_inheritor = (thread_t)old_inheritor;

			                                                   
			old_inheritor_needs_update = thread_remove_turnstile_promotion(thread_inheritor, turnstile);
		} else if (old_inheritor_flags & TURNSTILE_INHERITOR_TURNSTILE) {
			struct turnstile *old_turnstile = old_inheritor;

			old_inheritor_needs_update = turnstile_remove_turnstile_promotion(
				old_turnstile, turnstile);
		} else if (old_inheritor_flags & TURNSTILE_INHERITOR_WORKQ) {
			  
                                                          
                                                      
      
			turnstile_stats_update(1, TSU_NO_PRI_CHANGE_NEEDED |
			    TSU_TURNSTILE_ARG, turnstile);
		} else {
			panic("Inheritor flags lost along the way");
		}
		                            
		if (!old_inheritor_needs_update) {
			turnstile_stats_update(1, TSU_PRI_PROPAGATION | TSU_TURNSTILE_ARG,
			    turnstile);
		}
	}

	if (new_inheritor != NULL) {
		if (new_inheritor_flags & TURNSTILE_INHERITOR_THREAD) {
			thread_t thread_inheritor = (thread_t)new_inheritor;

			assert(new_inheritor_flags & TURNSTILE_INHERITOR_THREAD);
			                                              
			new_inheritor_needs_update = thread_add_turnstile_promotion(
				thread_inheritor, turnstile);
		} else if (new_inheritor_flags & TURNSTILE_INHERITOR_TURNSTILE) {
			struct turnstile *new_turnstile = new_inheritor;

			new_inheritor_needs_update = turnstile_add_turnstile_promotion(
				new_turnstile, turnstile);
		} else if (new_inheritor_flags & TURNSTILE_INHERITOR_WORKQ) {
			struct workqueue *wq_inheritor = new_inheritor;

			new_inheritor_needs_update = workq_add_turnstile_promotion(
				wq_inheritor, turnstile);
			if (!new_inheritor_needs_update) {
				turnstile_stats_update(1, TSU_NO_PRI_CHANGE_NEEDED |
				    TSU_TURNSTILE_ARG | TSU_BOOST_ARG, turnstile);
			}
		} else {
			panic("Inheritor flags lost along the way");
		}
		                            
		if (!new_inheritor_needs_update) {
			turnstile_stats_update(1, TSU_PRI_PROPAGATION |
			    TSU_TURNSTILE_ARG | TSU_BOOST_ARG | tsu_flags, turnstile);
		}
	}

done:
	if (old_inheritor_needs_update) {
		old_inheritor_flags |= TURNSTILE_INHERITOR_NEEDS_PRI_UPDATE;
	}

	  
                                                                                
                                                                  
    
	if (new_inheritor_needs_update) {
		old_inheritor_flags |= TURNSTILE_NEEDS_PRI_UPDATE;
	}

	turnstile->ts_inheritor = new_inheritor;
	turnstile->ts_inheritor_flags = new_inheritor_flags;
	thread->inheritor = old_inheritor;
	thread->inheritor_flags = old_inheritor_flags;
	return;
}

  
                                  
  
                                                                        
                                                                                 
                                                                
  
        
                    
                
  
           
                                                                   
   
static void
turnstile_stash_inheritor(
	turnstile_inheritor_t new_inheritor,
	turnstile_update_flags_t flags)
{
	thread_t thread = current_thread();

	  
                                                       
                                                        
                                             
    
	assert(thread->inheritor == TURNSTILE_INHERITOR_NULL);
	thread->inheritor = new_inheritor;
	thread->inheritor_flags = TURNSTILE_UPDATE_FLAGS_NONE;
	if (new_inheritor == TURNSTILE_INHERITOR_NULL) {
		                                   
	} else if (flags & TURNSTILE_INHERITOR_THREAD) {
		thread->inheritor_flags |= TURNSTILE_INHERITOR_THREAD;
		thread_reference((thread_t)new_inheritor);
	} else if (flags & TURNSTILE_INHERITOR_TURNSTILE) {
		thread->inheritor_flags |= TURNSTILE_INHERITOR_TURNSTILE;
		turnstile_reference((struct turnstile *)new_inheritor);
	} else if (flags & TURNSTILE_INHERITOR_WORKQ) {
		thread->inheritor_flags |= TURNSTILE_INHERITOR_WORKQ;
		workq_reference((struct workqueue *)new_inheritor);
	} else {
		panic("Missing type in flags (%x) for inheritor (%p)", flags,
		    new_inheritor);
	}
}

  
                                   
  
                                                                   
                                                                            
                                                                
  
        
                    
                    
                                                                                     
  
           
                                                                   
   
void
turnstile_update_inheritor(
	struct turnstile *turnstile,
	turnstile_inheritor_t new_inheritor,
	turnstile_update_flags_t flags)
{
	spl_t spl;

	turnstile_stash_inheritor(new_inheritor, flags);

	                                                              
	if (flags & TURNSTILE_DELAYED_UPDATE) {
		return;
	}

	                              
	spl = splsched();
	waitq_lock(&turnstile->ts_waitq);

	turnstile_update_inheritor_locked(turnstile);

	waitq_unlock(&turnstile->ts_waitq);
	splx(spl);

	return;
}


  
                                               
  
                                                                                   
  
                      
               
  
                                                                                 
                             
  
                            
   
static boolean_t
turnstile_need_thread_promotion_update(
	struct turnstile *dst_turnstile,
	thread_t thread)
{
	int thread_link_priority;
	boolean_t needs_update = FALSE;

	thread_link_priority = priority_queue_entry_sched_pri(
		&dst_turnstile->ts_waitq.waitq_prio_queue,
		&thread->wait_prioq_links);

	int priority = turnstile_compute_thread_push(dst_turnstile, thread);

	needs_update = (thread_link_priority == priority) ? FALSE : TRUE;

	return needs_update;
}

  
                                                  
  
                                                                    
  
                                              
                                              
                         
  
                                                              
   
static boolean_t
turnstile_priority_queue_update_entry_key(struct priority_queue_sched_max *q,
    priority_queue_entry_sched_t elt, priority_queue_key_t pri)
{
	priority_queue_key_t old_key = priority_queue_max_sched_pri(q);

	if (priority_queue_entry_sched_pri(q, elt) < pri) {
		priority_queue_entry_set_sched_pri(q, elt, pri, false);
		if (priority_queue_entry_increased(q, elt)) {
			return old_key != priority_queue_max_sched_pri(q);
		}
	} else if (priority_queue_entry_sched_pri(q, elt) > pri) {
		priority_queue_entry_set_sched_pri(q, elt, pri, false);
		if (priority_queue_entry_decreased(q, elt)) {
			return old_key != priority_queue_max_sched_pri(q);
		}
	}

	return FALSE;
}

  
                                                 
  
                                                                              
                                              
  
                      
               
  
                                                                                  
                                                                                                
  
                                                  
   
static boolean_t
turnstile_update_thread_promotion_locked(
	struct turnstile *dst_turnstile,
	thread_t thread)
{
	int thread_link_priority;

	int priority = turnstile_compute_thread_push(dst_turnstile, thread);

	thread_link_priority = priority_queue_entry_sched_pri(
		&dst_turnstile->ts_waitq.waitq_prio_queue,
		&thread->wait_prioq_links);

	if (priority != thread_link_priority) {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (TURNSTILE_CODE(TURNSTILE_HEAP_OPERATIONS, (THREAD_MOVED_IN_TURNSTILE_WAITQ))) | DBG_FUNC_NONE,
		    VM_KERNEL_UNSLIDE_OR_PERM(dst_turnstile),
		    thread_tid(thread),
		    priority,
		    thread_link_priority, 0);
	}

	if (!turnstile_priority_queue_update_entry_key(
		    &dst_turnstile->ts_waitq.waitq_prio_queue,
		    &thread->wait_prioq_links, (priority_queue_key_t)priority)) {
		return FALSE;
	}

	                                     
	return turnstile_recompute_priority_locked(dst_turnstile);
}

  
                                       
  
                                                                                        
  
               
                  
  
                                                                             
                                                                                           
  
                               
   
static boolean_t
thread_add_turnstile_promotion(
	thread_t thread,
	struct turnstile *turnstile)
{
	boolean_t needs_update = FALSE;

	                             
	thread_lock(thread);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (TURNSTILE_CODE(TURNSTILE_HEAP_OPERATIONS, (TURNSTILE_ADDED_TO_THREAD_HEAP))) | DBG_FUNC_NONE,
	    thread_tid(thread),
	    VM_KERNEL_UNSLIDE_OR_PERM(turnstile),
	    turnstile->ts_priority, 0, 0);

	priority_queue_entry_init(&turnstile->ts_inheritor_links);

	switch (turnstile_promote_policy[turnstile_get_type(turnstile)]) {
	case TURNSTILE_USER_PROMOTE:
	case TURNSTILE_USER_IPC_PROMOTE:

		priority_queue_entry_set_sched_pri(&thread->base_inheritor_queue,
		    &turnstile->ts_inheritor_links, turnstile->ts_priority, false);
		if (priority_queue_insert(&thread->base_inheritor_queue,
		    &turnstile->ts_inheritor_links)) {
			needs_update = thread_recompute_user_promotion_locked(thread);
		}

		break;
	case TURNSTILE_KERNEL_PROMOTE:

		priority_queue_entry_set_sched_pri(&thread->sched_inheritor_queue,
		    &turnstile->ts_inheritor_links, turnstile->ts_priority, false);
		if (priority_queue_insert(&thread->sched_inheritor_queue,
		    &turnstile->ts_inheritor_links)) {
			needs_update = thread_recompute_kernel_promotion_locked(thread);
		}

		break;
	default:
		panic("turnstile promotion for type %d not yet implemented", turnstile_get_type(turnstile));
	}

	                            
	if (!needs_update) {
		turnstile_stats_update(1,
		    thread_get_update_flags_for_turnstile_propagation_stoppage(thread) |
		    TSU_TURNSTILE_ARG | TSU_BOOST_ARG,
		    turnstile);
	}

	thread_unlock(thread);

	return needs_update;
}

  
                                          
  
                                                                                           
  
               
                  
  
                                                                             
                                                                                           
  
                               
   
static boolean_t
thread_remove_turnstile_promotion(
	thread_t thread,
	struct turnstile *turnstile)
{
	boolean_t needs_update = FALSE;

	thread_lock(thread);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (TURNSTILE_CODE(TURNSTILE_HEAP_OPERATIONS, (TURNSTILE_REMOVED_FROM_THREAD_HEAP))) | DBG_FUNC_NONE,
	    thread_tid(thread),
	    VM_KERNEL_UNSLIDE_OR_PERM(turnstile),
	    0, 0, 0);

	                             

	switch (turnstile_promote_policy[turnstile_get_type(turnstile)]) {
	case TURNSTILE_USER_PROMOTE:
	case TURNSTILE_USER_IPC_PROMOTE:
		if (priority_queue_remove(&thread->base_inheritor_queue,
		    &turnstile->ts_inheritor_links)) {
			needs_update = thread_recompute_user_promotion_locked(thread);
		}
		break;
	case TURNSTILE_KERNEL_PROMOTE:
		if (priority_queue_remove(&thread->sched_inheritor_queue,
		    &turnstile->ts_inheritor_links)) {
			needs_update = thread_recompute_kernel_promotion_locked(thread);
		}
		break;
	default:
		panic("turnstile promotion for type %d not yet implemented", turnstile_get_type(turnstile));
	}

	                            
	if (!needs_update) {
		turnstile_stats_update(1,
		    thread_get_update_flags_for_turnstile_propagation_stoppage(thread) | TSU_TURNSTILE_ARG,
		    turnstile);
	}

	thread_unlock(thread);

	return needs_update;
}

  
                                                
  
                                                                                           
  
               
                  
  
                                                                          
                             
  
                               
   
static boolean_t
thread_needs_turnstile_promotion_update(
	thread_t thread __assert_only,
	struct turnstile *turnstile)
{
	boolean_t needs_update = FALSE;
	int turnstile_link_priority = 0;

	switch (turnstile_promote_policy[turnstile_get_type(turnstile)]) {
	case TURNSTILE_USER_PROMOTE:
	case TURNSTILE_USER_IPC_PROMOTE:
		turnstile_link_priority = priority_queue_entry_sched_pri(
			&thread->base_inheritor_queue,
			&turnstile->ts_inheritor_links);
		break;
	case TURNSTILE_KERNEL_PROMOTE:
		turnstile_link_priority = priority_queue_entry_sched_pri(
			&thread->sched_inheritor_queue,
			&turnstile->ts_inheritor_links);
		break;
	default:
		panic("turnstile promotion for type %d not yet implemented", turnstile_get_type(turnstile));
	}

	needs_update = (turnstile_link_priority == turnstile->ts_priority) ? FALSE : TRUE;
	return needs_update;
}

  
                                                 
  
                                                                                                  
  
               
                  
  
                                                                             
                                                                                           
  
                                              
   
static boolean_t
thread_update_turnstile_promotion_locked(
	thread_t thread,
	struct turnstile *turnstile)
{
	boolean_t needs_update = FALSE;
	int turnstile_link_priority = 0;

	switch (turnstile_promote_policy[turnstile_get_type(turnstile)]) {
	case TURNSTILE_USER_PROMOTE:
	case TURNSTILE_USER_IPC_PROMOTE:
		turnstile_link_priority = priority_queue_entry_sched_pri(
			&thread->base_inheritor_queue,
			&turnstile->ts_inheritor_links);

		if (turnstile_priority_queue_update_entry_key(&(thread->base_inheritor_queue),
		    &turnstile->ts_inheritor_links, turnstile->ts_priority)) {
			needs_update = thread_recompute_user_promotion_locked(thread);
		}
		break;
	case TURNSTILE_KERNEL_PROMOTE:
		turnstile_link_priority = priority_queue_entry_sched_pri(
			&thread->sched_inheritor_queue,
			&turnstile->ts_inheritor_links);

		if (turnstile_priority_queue_update_entry_key(&(thread->sched_inheritor_queue),
		    &turnstile->ts_inheritor_links, turnstile->ts_priority)) {
			needs_update = thread_recompute_kernel_promotion_locked(thread);
		}
		break;
	default:
		panic("turnstile promotion for type %d not yet implemented", turnstile_get_type(turnstile));
	}

	if (turnstile->ts_priority != turnstile_link_priority) {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (TURNSTILE_CODE(TURNSTILE_HEAP_OPERATIONS, (TURNSTILE_MOVED_IN_THREAD_HEAP))) | DBG_FUNC_NONE,
		    thread_tid(thread),
		    VM_KERNEL_UNSLIDE_OR_PERM(turnstile),
		    turnstile->ts_priority,
		    turnstile_link_priority, 0);
	}

	return needs_update;
}


  
                                          
  
                                                                                                  
  
               
                  
  
                                                                             
                                                                                           
  
                               
   
static boolean_t
thread_update_turnstile_promotion(
	thread_t thread,
	struct turnstile *turnstile)
{
	                                                                
	boolean_t needs_update = thread_needs_turnstile_promotion_update(thread, turnstile);

	if (!needs_update) {
		turnstile_stats_update(1, TSU_NO_PRI_CHANGE_NEEDED |
		    TSU_TURNSTILE_ARG | TSU_BOOST_ARG, turnstile);
		return needs_update;
	}

	thread_lock(thread);

	                             
	needs_update = thread_update_turnstile_promotion_locked(thread, turnstile);

	                            
	if (!needs_update) {
		turnstile_stats_update(1,
		    thread_get_update_flags_for_turnstile_propagation_stoppage(thread) |
		    TSU_TURNSTILE_ARG | TSU_BOOST_ARG,
		    turnstile);
	}

	thread_unlock(thread);

	return needs_update;
}


  
                                                      
  
                                                                          
  
               
  
                                                               
  
                           
   
int
thread_get_inheritor_turnstile_sched_priority(thread_t thread)
{
	struct turnstile *max_turnstile;

	max_turnstile = priority_queue_max(&thread->sched_inheritor_queue,
	    struct turnstile, ts_inheritor_links);

	if (max_turnstile) {
		return priority_queue_entry_sched_pri(
			&thread->sched_inheritor_queue,
			&max_turnstile->ts_inheritor_links);
	}

	return 0;
}

  
                                                     
  
                                                                         
  
               
  
                                                              
  
                           
   
int
thread_get_inheritor_turnstile_base_priority(thread_t thread)
{
	struct turnstile *max_turnstile;

	max_turnstile = priority_queue_max(&thread->base_inheritor_queue,
	    struct turnstile, ts_inheritor_links);

	if (max_turnstile) {
		return priority_queue_entry_sched_pri(
			&thread->base_inheritor_queue,
			&max_turnstile->ts_inheritor_links);
	}

	return 0;
}


  
                                     
  
                                                                          
  
               
  
                                                               
                                      
  
                            
   
struct turnstile *
thread_get_waiting_turnstile(thread_t thread)
{
	struct turnstile *turnstile = TURNSTILE_NULL;
	struct waitq *waitq = thread->waitq;

	                                       
	if (waitq == NULL) {
		return turnstile;
	}

	if (waitq_is_turnstile_proxy(waitq)) {
		return waitq->waitq_ts;
	}

	                                             
	if (waitq_is_turnstile_queue(waitq)) {
		turnstile = waitq_to_turnstile(waitq);
	}
	return turnstile;
}

  
                                       
  
                                                          
                               
  
             
                              
  
                                                         
                                      
  
                                        
   
struct turnstile *
turnstile_lookup_by_proprietor(uintptr_t proprietor, turnstile_type_t type)
{
	return turnstile_htable_lookup(proprietor, type);
}

  
                                                                   
  
                                                                              
  
               
  
                                                           
                                                                   
                                                
  
                            
   
static turnstile_stats_update_flags_t
thread_get_update_flags_for_turnstile_propagation_stoppage(thread_t thread)
{
	struct waitq *waitq = thread->waitq;

	                                       
	if (waitq == NULL) {
		return TSU_THREAD_RUNNABLE;
	}

	                                                
	if (waitq_is_turnstile_proxy(waitq)) {
		if (waitq->waitq_ts) {
			return TSU_NO_PRI_CHANGE_NEEDED;
		}
		return TSU_NO_TURNSTILE;
	}

	                                             
	if (!waitq_is_turnstile_queue(waitq)) {
		return TSU_NO_TURNSTILE;
	}

	                                                                 
	return TSU_NO_PRI_CHANGE_NEEDED;
}


  
                                                           
  
                                                                              
  
                  
  
                                                                                              
                                      
  
                               
   
static turnstile_stats_update_flags_t
turnstile_get_update_flags_for_above_UI_pri_change(struct turnstile *turnstile)
{
	if (turnstile->ts_priority >
	    (thread_qos_policy_params.qos_pri[THREAD_QOS_USER_INTERACTIVE] + 1) &&
	    turnstile_get_type(turnstile) != TURNSTILE_ULOCK) {
		return TSU_ABOVE_UI_PRI_CHANGE;
	}

	return TSU_FLAGS_NONE;
}


  
                                      
  
                                                                          
                         
  
                  
                  
  
                               
   
static boolean_t
workq_add_turnstile_promotion(
	struct workqueue *wq_inheritor __unused,
	struct turnstile *turnstile)
{
	  
                                                                        
                      
    
	return turnstile->ts_priority > MAXPRI_THROTTLE;
}

  
                                                  
  
                                                                                              
  
                      
                      
  
                                                                             
                             
  
                                   
   
static boolean_t
turnstile_need_turnstile_promotion_update(
	struct turnstile *dst_turnstile __assert_only,
	struct turnstile *src_turnstile)
{
	int src_turnstile_link_priority;
	boolean_t needs_update = FALSE;

	src_turnstile_link_priority = priority_queue_entry_sched_pri(
		&dst_turnstile->ts_inheritor_queue,
		&src_turnstile->ts_inheritor_links);

	needs_update = (src_turnstile_link_priority == src_turnstile->ts_priority) ? FALSE : TRUE;
	return needs_update;
}

  
                                                    
  
                                                                           
                                             
  
                      
                      
  
                                                                                  
                                                                                                
  
                                           
   
static boolean_t
turnstile_update_turnstile_promotion_locked(
	struct turnstile *dst_turnstile,
	struct turnstile *src_turnstile)
{
	int src_turnstile_link_priority;
	src_turnstile_link_priority = priority_queue_entry_sched_pri(
		&dst_turnstile->ts_inheritor_queue,
		&src_turnstile->ts_inheritor_links);

	if (src_turnstile->ts_priority != src_turnstile_link_priority) {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (TURNSTILE_CODE(TURNSTILE_HEAP_OPERATIONS, (TURNSTILE_MOVED_IN_TURNSTILE_HEAP))) | DBG_FUNC_NONE,
		    VM_KERNEL_UNSLIDE_OR_PERM(dst_turnstile),
		    VM_KERNEL_UNSLIDE_OR_PERM(src_turnstile),
		    src_turnstile->ts_priority, src_turnstile_link_priority, 0);
	}

	if (!turnstile_priority_queue_update_entry_key(
		    &dst_turnstile->ts_inheritor_queue, &src_turnstile->ts_inheritor_links,
		    src_turnstile->ts_priority)) {
		return FALSE;
	}

	                                     
	return turnstile_recompute_priority_locked(dst_turnstile);
}

  
                                             
  
                                                                           
                                             
  
                      
                      
  
                                                                                  
                                                                                                
  
                                   
   
static boolean_t
turnstile_update_turnstile_promotion(
	struct turnstile *dst_turnstile,
	struct turnstile *src_turnstile)
{
	                                                                      
	boolean_t needs_update = turnstile_need_turnstile_promotion_update(dst_turnstile, src_turnstile);
	if (!needs_update) {
		turnstile_stats_update(1, TSU_NO_PRI_CHANGE_NEEDED |
		    TSU_TURNSTILE_ARG | TSU_BOOST_ARG,
		    src_turnstile);
		return needs_update;
	}

	                             
	waitq_lock(&dst_turnstile->ts_waitq);
	needs_update = turnstile_update_turnstile_promotion_locked(dst_turnstile, src_turnstile);

	                            
	if (!needs_update) {
		turnstile_stats_update(1,
		    (dst_turnstile->ts_inheritor ? TSU_NO_PRI_CHANGE_NEEDED : TSU_NO_INHERITOR) |
		    TSU_TURNSTILE_ARG | TSU_BOOST_ARG, src_turnstile);
	}
	waitq_unlock(&dst_turnstile->ts_waitq);
	return needs_update;
}

  
                                          
  
                                                                   
                                                    
  
                      
                      
  
                                                                                  
                                                                                                
  
                                   
   
static boolean_t
turnstile_add_turnstile_promotion(
	struct turnstile *dst_turnstile,
	struct turnstile *src_turnstile)
{
	boolean_t needs_update = FALSE;

	                             
	waitq_lock(&dst_turnstile->ts_waitq);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (TURNSTILE_CODE(TURNSTILE_HEAP_OPERATIONS, (TURNSTILE_ADDED_TO_TURNSTILE_HEAP))) | DBG_FUNC_NONE,
	    VM_KERNEL_UNSLIDE_OR_PERM(dst_turnstile),
	    VM_KERNEL_UNSLIDE_OR_PERM(src_turnstile),
	    src_turnstile->ts_priority, 0, 0);

	priority_queue_entry_init(&src_turnstile->ts_inheritor_links);
	priority_queue_entry_set_sched_pri(&dst_turnstile->ts_inheritor_queue,
	    &src_turnstile->ts_inheritor_links, src_turnstile->ts_priority, false);
	if (priority_queue_insert(&dst_turnstile->ts_inheritor_queue,
	    &src_turnstile->ts_inheritor_links)) {
		                                   
		needs_update = turnstile_recompute_priority_locked(dst_turnstile);
	}

	                            
	if (!needs_update) {
		turnstile_stats_update(1,
		    (dst_turnstile->ts_inheritor ? TSU_NO_PRI_CHANGE_NEEDED : TSU_NO_INHERITOR) |
		    TSU_TURNSTILE_ARG | TSU_BOOST_ARG, src_turnstile);
	}

	waitq_unlock(&dst_turnstile->ts_waitq);
	return needs_update;
}

  
                                             
  
                                                                        
                                                    
  
                      
                      
  
                                                                                  
                                                                                                
  
                                   
   
static boolean_t
turnstile_remove_turnstile_promotion(
	struct turnstile *dst_turnstile,
	struct turnstile *src_turnstile)
{
	boolean_t needs_update = FALSE;

	                             
	waitq_lock(&dst_turnstile->ts_waitq);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (TURNSTILE_CODE(TURNSTILE_HEAP_OPERATIONS, (TURNSTILE_REMOVED_FROM_TURNSTILE_HEAP))) | DBG_FUNC_NONE,
	    VM_KERNEL_UNSLIDE_OR_PERM(dst_turnstile),
	    VM_KERNEL_UNSLIDE_OR_PERM(src_turnstile),
	    0, 0, 0);

	if (priority_queue_remove(&dst_turnstile->ts_inheritor_queue,
	    &src_turnstile->ts_inheritor_links)) {
		                                   
		needs_update = turnstile_recompute_priority_locked(dst_turnstile);
	}

	                            
	if (!needs_update) {
		turnstile_stats_update(1,
		    (dst_turnstile->ts_inheritor ? TSU_NO_PRI_CHANGE_NEEDED : TSU_NO_INHERITOR) |
		    TSU_TURNSTILE_ARG, src_turnstile);
	}

	waitq_unlock(&dst_turnstile->ts_waitq);
	return needs_update;
}

  
                                      
  
                                                                  
                          
  
                  
               
  
                       
   
static int
turnstile_compute_thread_push(
	struct turnstile *turnstile,
	thread_t thread)
{
	int priority = 0;
	switch (turnstile_promote_policy[turnstile_get_type(turnstile)]) {
	case TURNSTILE_USER_PROMOTE:
	case TURNSTILE_USER_IPC_PROMOTE:
		priority = thread->base_pri;
		break;
	case TURNSTILE_KERNEL_PROMOTE:
		  
                                        
                                     
    
                                                  
                                               
     
		priority = thread->sched_pri;
		priority = MAX(priority, thread->base_pri);
		priority = MAX(priority, BASEPRI_DEFAULT);
		priority = MIN(priority, MAXPRI_PROMOTE);
		break;
	default:
		panic("turnstile promotion for type %d not yet implemented", turnstile_get_type(turnstile));
	}

	return priority;
}

  
                                                  
  
                                              
  
                     
                      
  
                       
   
void
turnstile_waitq_add_thread_priority_queue(
	struct waitq *wq,
	thread_t thread)
{
	struct turnstile *turnstile = waitq_to_turnstile(wq);
	int priority = turnstile_compute_thread_push(turnstile, thread);

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
	    (TURNSTILE_CODE(TURNSTILE_HEAP_OPERATIONS, (THREAD_ADDED_TO_TURNSTILE_WAITQ))) | DBG_FUNC_NONE,
	    VM_KERNEL_UNSLIDE_OR_PERM(turnstile),
	    thread_tid(thread),
	    priority, 0, 0);
	  
                                                     
                                                        
                                               
                                                          
                                             
                                                         
                                                         
                                                     
               
    
	priority_queue_entry_init(&thread->wait_prioq_links);
	priority_queue_entry_set_sched_pri(&wq->waitq_prio_queue,
	    &thread->wait_prioq_links, priority, false);
	priority_queue_insert(&wq->waitq_prio_queue,
	    &thread->wait_prioq_links);
}

  
                                            
  
                                               
                                                             
                          
  
                  
  
                                                                          
                                                                                            
  
                              
   
boolean_t
turnstile_recompute_priority_locked(
	struct turnstile *turnstile)
{
	int old_priority;
	int new_priority;
	boolean_t needs_priority_update = FALSE;
	thread_t max_thread = THREAD_NULL;
	struct turnstile *max_turnstile;
	int thread_max_pri = 0;
	int turnstile_max_pri = 0;

	switch (turnstile_promote_policy[turnstile_get_type(turnstile)]) {
	case TURNSTILE_USER_PROMOTE:
	case TURNSTILE_USER_IPC_PROMOTE:
	case TURNSTILE_KERNEL_PROMOTE:

		old_priority = turnstile->ts_priority;

		max_thread = priority_queue_max(&turnstile->ts_waitq.waitq_prio_queue,
		    struct thread, wait_prioq_links);

		if (max_thread) {
			thread_max_pri = priority_queue_entry_sched_pri(
				&turnstile->ts_waitq.waitq_prio_queue,
				&max_thread->wait_prioq_links);
		}

		max_turnstile = priority_queue_max(&turnstile->ts_inheritor_queue,
		    struct turnstile, ts_inheritor_links);

		if (max_turnstile) {
			assert(turnstile_promote_policy[turnstile_get_type(turnstile)] != TURNSTILE_KERNEL_PROMOTE);
			turnstile_max_pri = priority_queue_entry_sched_pri(
				&turnstile->ts_inheritor_queue,
				&max_turnstile->ts_inheritor_links);
		}

		new_priority = max(thread_max_pri, turnstile_max_pri);
		turnstile->ts_priority = (uint8_t)new_priority;

		if (old_priority != new_priority) {
			KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
			    (TURNSTILE_CODE(TURNSTILE_PRIORITY_OPERATIONS,
			    (TURNSTILE_PRIORITY_CHANGE))) | DBG_FUNC_NONE,
			    VM_KERNEL_UNSLIDE_OR_PERM(turnstile),
			    new_priority,
			    old_priority,
			    0, 0);
		}
		needs_priority_update = (!(old_priority == new_priority)) &&
		    (turnstile->ts_inheritor != NULL);
		break;

	case TURNSTILE_PROMOTE_NONE:
		                                              
		break;

	default:

		panic("Needs implementation for turnstile_recompute_priority");
		break;
	}
	return needs_priority_update;
}


  
                                     
  
                                               
                                                             
                          
  
                  
  
                                                                          
                                                                                            
   
boolean_t
turnstile_recompute_priority(
	struct turnstile *turnstile)
{
	boolean_t needs_priority_update = FALSE;
	spl_t s = splsched();

	waitq_lock(&turnstile->ts_waitq);

	needs_priority_update = turnstile_recompute_priority_locked(turnstile);

	waitq_unlock(&turnstile->ts_waitq);
	splx(s);
	return needs_priority_update;
}


  
                                                    
  
                                                                          
                                                 
  
                                                                 
                                         
  
                  
  
           
                                     
                                         
   
int
turnstile_workq_proprietor_of_max_turnstile(
	struct turnstile *turnstile,
	uintptr_t *proprietor_out)
{
	struct turnstile *max_turnstile;
	int max_priority = 0;
	uintptr_t proprietor = 0;

	assert(turnstile_get_type(turnstile) == TURNSTILE_WORKQS);

	spl_t s = splsched();

	waitq_lock(&turnstile->ts_waitq);

	max_turnstile = priority_queue_max(&turnstile->ts_inheritor_queue,
	    struct turnstile, ts_inheritor_links);
	if (max_turnstile) {
		max_priority = priority_queue_entry_sched_pri(
			&turnstile->ts_inheritor_queue,
			&max_turnstile->ts_inheritor_links);
		proprietor = max_turnstile->ts_proprietor;
	}

	waitq_unlock(&turnstile->ts_waitq);
	splx(s);

	if (max_priority <= MAXPRI_THROTTLE) {
		max_priority = 0;
		proprietor = 0;
	}
	if (proprietor_out) {
		*proprietor_out = proprietor;
	}
	return max_priority;
}

  
                                       
  
                                                                          
                                                                  
  
                           
  
           
                               
                                                                 
                                                            
                                                    
   
int
turnstile_workloop_pusher_info(
	struct turnstile *turnstile,
	thread_t *thread_out,
	ipc_port_t *port_out,
	struct knote **knote_out)
{
	struct turnstile *max_ts;
	thread_t max_thread;
	int max_thread_pri = 0;
	int max_ts_pri = 0;
	ipc_port_t port;

	assert(turnstile_get_type(turnstile) == TURNSTILE_WORKLOOPS);

	spl_t s = splsched();
	waitq_lock(&turnstile->ts_waitq);

	max_thread = priority_queue_max(&turnstile->ts_waitq.waitq_prio_queue,
	    struct thread, wait_prioq_links);
	if (max_thread) {
		max_thread_pri = priority_queue_entry_sched_pri(
			&turnstile->ts_waitq.waitq_prio_queue,
			&max_thread->wait_prioq_links);
	}

	max_ts = priority_queue_max(&turnstile->ts_inheritor_queue,
	    struct turnstile, ts_inheritor_links);
	if (max_ts) {
		max_ts_pri = priority_queue_entry_sched_pri(
			&turnstile->ts_inheritor_queue,
			&max_ts->ts_inheritor_links);
	}

	  
                                                
   
                               
   
                                                       
   
                                                                      
                     
   
                                                                         
                    
   
                                                                    
                    
   
                                                                        
                    
   
                                                                           
   
                                                                     
                                                                         
                                                     
    
	if (max_thread_pri > max_ts_pri) {
		thread_reference(max_thread);
		*thread_out = max_thread;
		*port_out = NULL;
		*knote_out = NULL;
	} else if (max_ts_pri) {
		switch (turnstile_get_type(max_ts)) {
		case TURNSTILE_KNOTE:
			          
			*thread_out = THREAD_NULL;
			*port_out = IP_NULL;
			*knote_out = (struct knote *)max_ts->ts_proprietor;
			break;

		case TURNSTILE_SYNC_IPC:
			             
			port = (ipc_port_t)max_ts->ts_proprietor;
			ip_reference(port);
			*thread_out = THREAD_NULL;
			*port_out = port;
			*knote_out = NULL;
			break;

		default:
			panic("Unexpected type for turnstile %p", max_ts);
		}
	} else {
		*thread_out = THREAD_NULL;
		*port_out = IP_NULL;
		*knote_out = NULL;
	}

	waitq_unlock(&turnstile->ts_waitq);
	splx(s);

	return max(max_thread_pri, max_ts_pri);
}

  
                              
  
                                                             
  
                             
  
                                                       
   

boolean_t
turnstile_has_waiters(struct turnstile *turnstile)
{
	boolean_t ret;

	spl_t s = splsched();
	waitq_lock(&turnstile->ts_waitq);
	ret = !priority_queue_empty(&turnstile->ts_waitq.waitq_prio_queue);
	waitq_unlock(&turnstile->ts_waitq);
	splx(s);

	return ret;
}

  
                                                  
  
                                                                   
                                                                        
  
                  
                        
  
                 
   
static void
turnstile_update_inheritor_priority_chain(
	turnstile_inheritor_t inheritor,
	turnstile_update_flags_t turnstile_flags)
{
	struct turnstile *turnstile = TURNSTILE_NULL;
	thread_t thread = THREAD_NULL;
	int total_hop = 0, thread_hop = 0;
	spl_t s;
	turnstile_stats_update_flags_t tsu_flags = ((turnstile_flags & TURNSTILE_UPDATE_BOOST) ?
	    TSU_BOOST_ARG : TSU_FLAGS_NONE) | TSU_PRI_PROPAGATION;

	if (inheritor == NULL) {
		return;
	}

	s = splsched();

	if (turnstile_flags & TURNSTILE_INHERITOR_THREAD) {
		thread = inheritor;
		thread_lock(thread);
		thread_recompute_user_promotion_locked(thread);
		thread_recompute_kernel_promotion_locked(thread);
	} else if (turnstile_flags & TURNSTILE_INHERITOR_TURNSTILE) {
		turnstile = inheritor;
		waitq_lock(&turnstile->ts_waitq);
		turnstile_recompute_priority_locked(turnstile);
		tsu_flags |= turnstile_get_update_flags_for_above_UI_pri_change(turnstile);
	} else {
		  
                                                                     
                                                      
     
		assert((turnstile_flags & TURNSTILE_INHERITOR_WORKQ) == 0);
	}

	while (turnstile != TURNSTILE_NULL || thread != THREAD_NULL) {
		if (turnstile != TURNSTILE_NULL) {
			if (turnstile->ts_inheritor == NULL) {
				turnstile_stats_update(total_hop + 1, TSU_NO_INHERITOR |
				    TSU_TURNSTILE_ARG | tsu_flags,
				    turnstile);
				waitq_unlock(&turnstile->ts_waitq);
				turnstile = TURNSTILE_NULL;
				break;
			}
			if (turnstile->ts_inheritor_flags & TURNSTILE_INHERITOR_THREAD) {
				turnstile_update_inheritor_thread_priority_chain(&turnstile, &thread,
				    total_hop, tsu_flags);
			} else if (turnstile->ts_inheritor_flags & TURNSTILE_INHERITOR_TURNSTILE) {
				turnstile_update_inheritor_turnstile_priority_chain(&turnstile,
				    total_hop, tsu_flags);
			} else if (turnstile->ts_inheritor_flags & TURNSTILE_INHERITOR_WORKQ) {
				turnstile_update_inheritor_workq_priority_chain(turnstile, s);
				turnstile_stats_update(total_hop + 1, TSU_NO_PRI_CHANGE_NEEDED | tsu_flags,
				    NULL);
				return;
			} else {
				panic("Inheritor flags not passed in turnstile_update_inheritor");
			}
		} else if (thread != THREAD_NULL) {
			thread_update_waiting_turnstile_priority_chain(&thread, &turnstile,
			    thread_hop, total_hop, tsu_flags);
			thread_hop++;
		}
		total_hop++;
	}

	splx(s);
	return;
}

  
                                            
  
                                                                       
                                                                    
                                                                
                                                                     
                                                                    
  
        
                    
                         
  
                 
   
void
turnstile_update_inheritor_complete(
	struct turnstile *turnstile,
	turnstile_update_complete_flags_t flags __unused)
{
	thread_t thread = current_thread();

	turnstile_update_flags_t inheritor_flags = thread->inheritor_flags;

	turnstile_cleanup();

	                                               
	if (inheritor_flags & TURNSTILE_NEEDS_PRI_UPDATE) {
		turnstile_update_inheritor_priority_chain(turnstile,
		    TURNSTILE_INHERITOR_TURNSTILE | TURNSTILE_UPDATE_BOOST);
	}
}

  
                          
  
                                                        
                          
  
                                                     
  
                 
   
void
turnstile_cleanup(void)
{
	thread_t thread = current_thread();

	                                                      
	turnstile_inheritor_t old_inheritor = thread->inheritor;
	turnstile_update_flags_t inheritor_flags = thread->inheritor_flags;
	thread->inheritor = THREAD_NULL;
	thread->inheritor_flags = TURNSTILE_UPDATE_FLAGS_NONE;

	if (old_inheritor == TURNSTILE_INHERITOR_NULL) {
		                      
		return;
	}

	                                                 
	if (inheritor_flags & TURNSTILE_INHERITOR_NEEDS_PRI_UPDATE) {
		turnstile_update_inheritor_priority_chain(old_inheritor,
		    inheritor_flags);
	}

	                                             
	if (inheritor_flags & TURNSTILE_INHERITOR_THREAD) {
		thread_deallocate_safe(old_inheritor);
	} else if (inheritor_flags & TURNSTILE_INHERITOR_TURNSTILE) {
		turnstile_deallocate_safe((struct turnstile *)old_inheritor);
	} else if (inheritor_flags & TURNSTILE_INHERITOR_WORKQ) {
		workq_deallocate_safe((struct workqueue *)old_inheritor);
	} else {
		panic("Inheritor flags lost along the way");
	}
}

  
                                               
  
                                                           
                                                           
  
                                                   
  
                 
   
void
turnstile_update_thread_priority_chain(thread_t thread)
{
	turnstile_update_inheritor_priority_chain(thread,
	    TURNSTILE_INHERITOR_THREAD | TURNSTILE_UPDATE_BOOST);
}

  
                                                        
  
                                                                      
                                                             
  
                             
                                           
  
                                                                   
                                        
   
static void
turnstile_update_inheritor_workq_priority_chain(struct turnstile *turnstile, spl_t s)
{
	struct workqueue *wq = turnstile->ts_inheritor;
	bool workq_lock_held = workq_is_current_thread_updating_turnstile(wq);

	if (__improbable(turnstile->ts_priority <= MAXPRI_THROTTLE)) {
		waitq_unlock(&turnstile->ts_waitq);
		splx(s);
		return;
	}

	if (!workq_lock_held) {
		workq_reference(wq);
		disable_preemption();
	}
	waitq_unlock(&turnstile->ts_waitq);
	splx(s);

	workq_schedule_creator_turnstile_redrive(wq, workq_lock_held);

	if (!workq_lock_held) {
		enable_preemption();
		workq_deallocate_safe(wq);
	}
}

  
                                                         
  
                                                                       
                         
  
                                           
                                                           
                                                              
                                          
  
                                                                    
                                
  
                                                                          
                                           
   
static void
turnstile_update_inheritor_thread_priority_chain(
	struct turnstile **in_turnstile,
	thread_t *out_thread,
	int total_hop,
	turnstile_stats_update_flags_t tsu_flags)
{
	boolean_t needs_update = FALSE;
	struct turnstile *turnstile = *in_turnstile;
	thread_t thread_inheritor = turnstile->ts_inheritor;
	boolean_t first_update = !total_hop;

	assert(turnstile->ts_inheritor_flags & TURNSTILE_INHERITOR_THREAD);
	*in_turnstile = TURNSTILE_NULL;

	                                                               
	needs_update = thread_needs_turnstile_promotion_update(thread_inheritor, turnstile);
	if (!needs_update && !first_update) {
		turnstile_stats_update(total_hop + 1, TSU_NO_PRI_CHANGE_NEEDED |
		    TSU_TURNSTILE_ARG | tsu_flags, turnstile);
		waitq_unlock(&turnstile->ts_waitq);
		return;
	}

	thread_lock(thread_inheritor);

	                                                              
	needs_update = thread_update_turnstile_promotion_locked(
		thread_inheritor, turnstile);

	  
                                                       
                                                   
                                                       
                                                           
                        
    
	if (!needs_update && !first_update) {
		                                             
		turnstile_stats_update(total_hop + 1,
		    (thread_get_update_flags_for_turnstile_propagation_stoppage(thread_inheritor)) |
		    TSU_TURNSTILE_ARG | tsu_flags,
		    turnstile);
		thread_unlock(thread_inheritor);
		waitq_unlock(&turnstile->ts_waitq);
		return;
	}

	                                                
	waitq_unlock(&turnstile->ts_waitq);
	*out_thread = thread_inheritor;
	return;
}

  
                                                            
  
                                                                          
                         
  
                                               
                                                              
                                          
  
                                                                             
                                
  
                                                                              
                                                                                
                                                                                   
   
static void
turnstile_update_inheritor_turnstile_priority_chain(
	struct turnstile **in_out_turnstile,
	int total_hop,
	turnstile_stats_update_flags_t tsu_flags)
{
	boolean_t needs_update = FALSE;
	struct turnstile *turnstile = *in_out_turnstile;
	struct turnstile *inheritor_turnstile = turnstile->ts_inheritor;
	boolean_t first_update = !total_hop;

	assert(turnstile->ts_inheritor_flags & TURNSTILE_INHERITOR_TURNSTILE);
	*in_out_turnstile = TURNSTILE_NULL;

	                                                                                   
	needs_update = turnstile_need_turnstile_promotion_update(inheritor_turnstile, turnstile);
	if (!needs_update && !first_update) {
		turnstile_stats_update(total_hop + 1, TSU_NO_PRI_CHANGE_NEEDED |
		    TSU_TURNSTILE_ARG | tsu_flags,
		    turnstile);
		waitq_unlock(&turnstile->ts_waitq);
		return;
	}

	waitq_lock(&inheritor_turnstile->ts_waitq);

	needs_update = turnstile_update_turnstile_promotion_locked(
		inheritor_turnstile, turnstile);

	  
                                                          
                                                   
                                                       
                                                           
                        
    
	if (!needs_update && !first_update) {
		                                             
		turnstile_stats_update(total_hop + 1,
		    (inheritor_turnstile->ts_inheritor ? TSU_NO_PRI_CHANGE_NEEDED : TSU_NO_INHERITOR) |
		    TSU_TURNSTILE_ARG | tsu_flags,
		    turnstile);
		waitq_unlock(&inheritor_turnstile->ts_waitq);
		waitq_unlock(&turnstile->ts_waitq);
		return;
	}

	                                                               
	waitq_unlock(&turnstile->ts_waitq);
	*in_out_turnstile = inheritor_turnstile;
	return;
}

  
                                                       
  
                                                          
                                   
  
                                     
                                                                
                                                  
                                      
                                          
  
                                                                                 
  
                                                                           
   
static void
thread_update_waiting_turnstile_priority_chain(
	thread_t *in_thread,
	struct turnstile **out_turnstile,
	int thread_hop,
	int total_hop,
	turnstile_stats_update_flags_t tsu_flags)
{
	boolean_t needs_update = FALSE;
	thread_t thread = *in_thread;
	struct turnstile *waiting_turnstile = TURNSTILE_NULL;
	uint32_t turnstile_gencount;
	boolean_t first_update = !total_hop;

	*in_thread = THREAD_NULL;

	                                            
	waiting_turnstile = thread_get_waiting_turnstile(thread);

	if (waiting_turnstile == TURNSTILE_NULL || thread_hop > turnstile_max_hop) {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (TURNSTILE_CODE(TURNSTILE_HEAP_OPERATIONS,
		    (waiting_turnstile ? TURNSTILE_UPDATE_STOPPED_BY_LIMIT : THREAD_NOT_WAITING_ON_TURNSTILE)
		    )) | DBG_FUNC_NONE,
		    thread_tid(thread),
		    turnstile_max_hop,
		    thread_hop,
		    VM_KERNEL_UNSLIDE_OR_PERM(waiting_turnstile), 0);
		turnstile_stats_update(total_hop + 1, TSU_NO_TURNSTILE |
		    TSU_THREAD_ARG | tsu_flags, thread);
		thread_unlock(thread);
		return;
	}

	                                                               
	needs_update = turnstile_need_thread_promotion_update(waiting_turnstile, thread);
	if (!needs_update && !first_update) {
		turnstile_stats_update(total_hop + 1, TSU_NO_PRI_CHANGE_NEEDED |
		    TSU_THREAD_ARG | tsu_flags, thread);
		thread_unlock(thread);
		return;
	}

	                                                                    
	turnstile_gencount = turnstile_get_gencount(waiting_turnstile);
	turnstile_reference(waiting_turnstile);
	thread_reference(thread);

	                                                         
	thread_unlock(thread);
	waitq_lock(&waiting_turnstile->ts_waitq);
	thread_lock(thread);

	                                                                                 
	if (turnstile_gencount != turnstile_get_gencount(waiting_turnstile) ||
	    waiting_turnstile != thread_get_waiting_turnstile(thread)) {
		turnstile_stats_update(total_hop + 1, TSU_NO_PRI_CHANGE_NEEDED |
		    TSU_THREAD_ARG | tsu_flags, thread);
		                                   
		thread_unlock(thread);
		waitq_unlock(&waiting_turnstile->ts_waitq);
		thread_deallocate_safe(thread);
		turnstile_deallocate_safe(waiting_turnstile);
		return;
	}

	  
                                                                           
                                                                         
                                                                   
    
	thread_deallocate_safe(thread);
	turnstile_deallocate_safe(waiting_turnstile);

	                                                 
	needs_update = turnstile_update_thread_promotion_locked(waiting_turnstile, thread);

	  
                                                       
                                                   
                                                       
                                                           
                        
    
	if (!needs_update && !first_update) {
		turnstile_stats_update(total_hop + 1,
		    (waiting_turnstile->ts_inheritor ? TSU_NO_PRI_CHANGE_NEEDED : TSU_NO_INHERITOR) |
		    TSU_THREAD_ARG | tsu_flags, thread);
		thread_unlock(thread);
		waitq_unlock(&waiting_turnstile->ts_waitq);
		return;
	}

	                                                   
	thread_unlock(thread);
	*out_turnstile = waiting_turnstile;
}

  
                               
  
                                                                  
  
                                                             
                                             
                             
  
                   
   
void
turnstile_stats_update(
	int hop __assert_only,
	turnstile_stats_update_flags_t flags __assert_only,
	turnstile_inheritor_t inheritor __assert_only)
{
#if DEVELOPMENT || DEBUG
	if (flags & TSU_TURNSTILE_BLOCK_COUNT) {
		os_atomic_inc(&thread_block_on_turnstile_count, relaxed);
	}

	if (flags & TSU_REGULAR_WAITQ_BLOCK_COUNT) {
		os_atomic_inc(&thread_block_on_regular_waitq_count, relaxed);
	}

	if (hop > TURNSTILE_MAX_HOP_DEFAULT || hop == 0) {
		return;
	}

	assert(hop >= 0);

	  
                                                 
                                                
                            
                                                   
                                                         
                                                   
    
	if (flags & TSU_TURNSTILE_ARG) {
		struct turnstile *ts = (struct turnstile *)inheritor;
		if (ts->ts_priority == 0) {
			return;
		}

		if (hop == 1 && turnstile_get_type(ts) == TURNSTILE_WORKQS) {
			return;
		}
	} else if (flags & TSU_THREAD_ARG) {
		thread_t thread = (thread_t)inheritor;
		if (thread->user_promotion_basepri == 0) {
			return;
		}
	} else {
		assert(inheritor == NULL);
	}

	struct turnstile_stats *turnstile_stats;
	if (flags & TSU_BOOST_ARG) {
		turnstile_stats = turnstile_boost_stats;
	} else {
		turnstile_stats = turnstile_unboost_stats;
	}

	if (flags & TSU_PRI_PROPAGATION) {
		os_atomic_inc(&turnstile_stats[hop - 1].ts_priority_propagation, relaxed);
	}

	if (flags & TSU_NO_INHERITOR) {
		os_atomic_inc(&turnstile_stats[hop - 1].ts_no_inheritor, relaxed);
	}

	if (flags & TSU_NO_TURNSTILE) {
		os_atomic_inc(&turnstile_stats[hop - 1].ts_no_turnstile, relaxed);
	}

	if (flags & TSU_NO_PRI_CHANGE_NEEDED) {
		os_atomic_inc(&turnstile_stats[hop - 1].ts_no_priority_change_required, relaxed);
	}

	if (flags & TSU_THREAD_RUNNABLE) {
		os_atomic_inc(&turnstile_stats[hop - 1].ts_thread_runnable, relaxed);
	}

	if (flags & TSU_ABOVE_UI_PRI_CHANGE) {
		os_atomic_inc(&turnstile_stats[hop - 1].ts_above_ui_pri_change, relaxed);
	}
#endif
}

static uint64_t
kdp_turnstile_traverse_inheritor_chain(struct turnstile *ts, uint64_t *flags, uint8_t *hops)
{
	uint8_t unknown_hops;

	if (waitq_held(&ts->ts_waitq)) {
		*flags |= STACKSHOT_TURNSTILE_STATUS_LOCKED_WAITQ;
		return 0;
	}

	*hops = *hops + 1;
	unknown_hops = *hops;

	  
                                                                                          
                                                                         
                                                                              
                                                         
    
	if (ts->ts_inheritor_flags & TURNSTILE_INHERITOR_TURNSTILE) {
		uint8_t pre_hops = *hops;
		uint64_t ret = kdp_turnstile_traverse_inheritor_chain(ts->ts_inheritor, flags, hops);
		  
                                                                          
                                                
     
		if (ret != 0 || *flags != STACKSHOT_TURNSTILE_STATUS_UNKNOWN) {
			return ret;
		}
		                                                                                   
		unknown_hops = *hops;
		*hops = pre_hops;
		*flags = 0;
	}

	if (ts->ts_inheritor_flags & TURNSTILE_INHERITOR_THREAD) {
		*flags |= STACKSHOT_TURNSTILE_STATUS_THREAD;
		return (uint64_t) thread_tid(ts->ts_inheritor);
	}

	if (ts->ts_inheritor_flags & TURNSTILE_INHERITOR_WORKQ) {
		*flags |= STACKSHOT_TURNSTILE_STATUS_WORKQUEUE;
		return VM_KERNEL_UNSLIDE_OR_PERM(ts->ts_inheritor);
	}

	  
                                                                          
                               
    
	if (turnstile_is_send_turnstile(ts)) {
		ipc_port_t port = (ipc_port_t)ts->ts_proprietor;

		if (port && ip_active(port)) {
			if (ip_lock_held_kdp(port)) {
				*flags |= STACKSHOT_TURNSTILE_STATUS_HELD_IPLOCK;
				return 0;
			}
			if (port->ip_receiver_name != 0 && port->ip_receiver) {
				ipc_space_t space = (ipc_space_t) port->ip_receiver;
				task_t dest_task = space->is_task;

				if (dest_task != TASK_NULL) {
					*flags |= STACKSHOT_TURNSTILE_STATUS_BLOCKED_ON_TASK;
					return pid_from_task(dest_task);
				}
			}
		}
	}

	if (turnstile_is_receive_turnstile(ts)) {
		ipc_port_t port = (ipc_port_t)ts->ts_proprietor;
		if (port && ip_active(port)) {
			if (ip_lock_held_kdp(port)) {
				*flags |= STACKSHOT_TURNSTILE_STATUS_HELD_IPLOCK;
				return 0;
			}
			if (port->ip_specialreply) {
				                                            
				uint64_t pid_candidate = ipc_special_reply_get_pid_locked(port);

				if (pid_candidate) {
					*flags |= STACKSHOT_TURNSTILE_STATUS_BLOCKED_ON_TASK;
					return pid_candidate;
				}
			}
		}
	}

	*hops = unknown_hops;
	*flags |= STACKSHOT_TURNSTILE_STATUS_UNKNOWN;
	return 0;
}

void
kdp_turnstile_fill_tsinfo(struct turnstile *ts, thread_turnstileinfo_t *tsinfo)
{
	uint64_t final_inheritor;
	uint64_t flags = 0;
	uint8_t hops = 0;

	tsinfo->turnstile_context  = 0;
	tsinfo->number_of_hops     = 0;
	tsinfo->turnstile_priority = 0;

	assert(ts != TURNSTILE_NULL);

	if (waitq_held(&ts->ts_waitq)) {
		tsinfo->turnstile_flags |= STACKSHOT_TURNSTILE_STATUS_LOCKED_WAITQ;
		return;
	}

	final_inheritor = kdp_turnstile_traverse_inheritor_chain(ts, &flags, &hops);

	                                                    
	tsinfo->turnstile_flags = flags;
	tsinfo->number_of_hops = hops;
	tsinfo->turnstile_priority = ts->ts_priority;
	tsinfo->turnstile_context = final_inheritor;
}

#if DEVELOPMENT || DEBUG

int sysctl_io_opaque(void *req, void *pValue, size_t valueSize, int *changed);

  
                                         
  
                                                
  
                                                        
  
                   
   
int
turnstile_get_boost_stats_sysctl(
	void *req)
{
	return sysctl_io_opaque(req, turnstile_boost_stats, sizeof(struct turnstile_stats) * TURNSTILE_MAX_HOP_DEFAULT, NULL);
}

  
                                   
  
                                                
  
                                                        
  
                   
   
int
turnstile_get_unboost_stats_sysctl(
	void *req)
{
	return sysctl_io_opaque(req, turnstile_unboost_stats, sizeof(struct turnstile_stats) * TURNSTILE_MAX_HOP_DEFAULT, NULL);
}

                                               
#define tstile_test_prim_lock_interlock(test_prim) \
	lck_spin_lock(&test_prim->ttprim_interlock)
#define tstile_test_prim_unlock_interlock(test_prim) \
	lck_spin_unlock(&test_prim->ttprim_interlock)

static void
tstile_test_prim_init(struct tstile_test_prim **test_prim_ptr)
{
	struct tstile_test_prim *test_prim = (struct tstile_test_prim *) kalloc(sizeof(struct tstile_test_prim));

	test_prim->ttprim_turnstile = TURNSTILE_NULL;
	test_prim->ttprim_owner = NULL;
	lck_spin_init(&test_prim->ttprim_interlock, &turnstiles_dev_lock_grp, LCK_ATTR_NULL);
	test_prim->tt_prim_waiters = 0;

	*test_prim_ptr = test_prim;
	return;
}

int
tstile_test_prim_lock(int val)
{
	struct tstile_test_prim *test_prim;
	boolean_t use_hashtable;
	turnstile_type_t type;
	wait_interrupt_t wait_type;

	switch (val) {
	case SYSCTL_TURNSTILE_TEST_USER_DEFAULT:
		test_prim = test_prim_ts_inline;
		use_hashtable = FALSE;
		wait_type = THREAD_ABORTSAFE;
		type = TURNSTILE_ULOCK;
		break;
	case SYSCTL_TURNSTILE_TEST_USER_HASHTABLE:
		test_prim = test_prim_global_htable;
		use_hashtable = TRUE;
		wait_type = THREAD_ABORTSAFE;
		type = TURNSTILE_ULOCK;
		break;
	case SYSCTL_TURNSTILE_TEST_KERNEL_DEFAULT:
		test_prim = test_prim_global_ts_kernel;
		use_hashtable = FALSE;
		wait_type = THREAD_UNINT | THREAD_WAIT_NOREPORT_USER;
		type = TURNSTILE_KERNEL_MUTEX;
		break;
	case SYSCTL_TURNSTILE_TEST_KERNEL_HASHTABLE:
		test_prim = test_prim_global_ts_kernel_hash;
		use_hashtable = TRUE;
		wait_type = THREAD_UNINT | THREAD_WAIT_NOREPORT_USER;
		type = TURNSTILE_KERNEL_MUTEX;
		break;

	default:
		return -1;
	}

lock_start:

	                                         
	tstile_test_prim_lock_interlock(test_prim);

	                                    
	if (test_prim->ttprim_owner == NULL && test_prim->tt_prim_waiters == 0) {
		thread_reference(current_thread());
		test_prim->ttprim_owner = current_thread();
		tstile_test_prim_unlock_interlock(test_prim);
		return 0;
	}

	struct turnstile *prim_turnstile = TURNSTILE_NULL;

	                                       
	prim_turnstile = turnstile_prepare((uintptr_t)test_prim,
	    use_hashtable ? NULL : &test_prim->ttprim_turnstile,
	    TURNSTILE_NULL, type);

	assert(prim_turnstile != TURNSTILE_NULL);

	                                    
	if (test_prim->ttprim_owner == NULL) {
		thread_reference(current_thread());
		test_prim->ttprim_owner = current_thread();

		                                
		turnstile_update_inheritor(prim_turnstile,
		    current_thread(),
		    (TURNSTILE_IMMEDIATE_UPDATE | TURNSTILE_INHERITOR_THREAD));

		turnstile_update_inheritor_complete(prim_turnstile, TURNSTILE_INTERLOCK_HELD);

		turnstile_complete((uintptr_t)test_prim,
		    use_hashtable ? NULL : &test_prim->ttprim_turnstile, NULL, type);

		tstile_test_prim_unlock_interlock(test_prim);

		turnstile_cleanup();
		return 0;
	}

	test_prim->tt_prim_waiters++;
	turnstile_update_inheritor(prim_turnstile,
	    test_prim->ttprim_owner,
	    (TURNSTILE_DELAYED_UPDATE | TURNSTILE_INHERITOR_THREAD));

	waitq_assert_wait64(&prim_turnstile->ts_waitq,
	    CAST_EVENT64_T(test_prim), wait_type,
	    TIMEOUT_WAIT_FOREVER);

	                        
	tstile_test_prim_unlock_interlock(test_prim);

	turnstile_update_inheritor_complete(prim_turnstile, TURNSTILE_INTERLOCK_NOT_HELD);

	wait_result_t result;
	result = thread_block(THREAD_CONTINUE_NULL);

	                                                    
	tstile_test_prim_lock_interlock(test_prim);
	test_prim->tt_prim_waiters--;
	turnstile_complete((uintptr_t)test_prim,
	    use_hashtable ? NULL : &test_prim->ttprim_turnstile, NULL, type);

	tstile_test_prim_unlock_interlock(test_prim);

	turnstile_cleanup();

	                                  
	if (result == THREAD_INTERRUPTED) {
		return 1;
	}

	goto lock_start;
}

int
tstile_test_prim_unlock(int val)
{
	struct tstile_test_prim *test_prim;
	boolean_t use_hashtable;
	turnstile_type_t type;

	switch (val) {
	case SYSCTL_TURNSTILE_TEST_USER_DEFAULT:
		test_prim = test_prim_ts_inline;
		use_hashtable = FALSE;
		type = TURNSTILE_ULOCK;
		break;
	case SYSCTL_TURNSTILE_TEST_USER_HASHTABLE:
		test_prim = test_prim_global_htable;
		use_hashtable = TRUE;
		type = TURNSTILE_ULOCK;
		break;
	case SYSCTL_TURNSTILE_TEST_KERNEL_DEFAULT:
		test_prim = test_prim_global_ts_kernel;
		use_hashtable = FALSE;
		type = TURNSTILE_KERNEL_MUTEX;
		break;
	case SYSCTL_TURNSTILE_TEST_KERNEL_HASHTABLE:
		test_prim = test_prim_global_ts_kernel_hash;
		use_hashtable = TRUE;
		type = TURNSTILE_KERNEL_MUTEX;
		break;
	default:
		return -1;
	}

	                                         
	tstile_test_prim_lock_interlock(test_prim);

	if (test_prim->ttprim_owner == NULL) {
		tstile_test_prim_unlock_interlock(test_prim);
		return 1;
	}

	                                    
	if (test_prim->ttprim_owner != NULL && test_prim->tt_prim_waiters == 0) {
		                           
		thread_t old_owner = test_prim->ttprim_owner;
		test_prim->ttprim_owner = NULL;
		tstile_test_prim_unlock_interlock(test_prim);

		thread_deallocate(old_owner);
		return 0;
	}

	struct turnstile *prim_turnstile = TURNSTILE_NULL;

	thread_t old_owner = test_prim->ttprim_owner;
	test_prim->ttprim_owner = NULL;

	                                       
	prim_turnstile = turnstile_prepare((uintptr_t)test_prim,
	    use_hashtable ? NULL : &test_prim->ttprim_turnstile,
	    TURNSTILE_NULL, type);

	assert(prim_turnstile != TURNSTILE_NULL);

	                                
	turnstile_update_inheritor(prim_turnstile,
	    NULL,
	    (TURNSTILE_IMMEDIATE_UPDATE | TURNSTILE_INHERITOR_THREAD));

	waitq_wakeup64_one(&prim_turnstile->ts_waitq,
	    CAST_EVENT64_T(test_prim),
	    THREAD_AWAKENED, WAITQ_ALL_PRIORITIES);

	turnstile_update_inheritor_complete(prim_turnstile, TURNSTILE_INTERLOCK_HELD);

	turnstile_complete((uintptr_t)test_prim,
	    use_hashtable ? NULL : &test_prim->ttprim_turnstile, NULL, type);

	tstile_test_prim_unlock_interlock(test_prim);

	turnstile_cleanup();

	if (old_owner) {
		                                                                                     
		thread_deallocate_safe(old_owner);
	}

	return 0;
}

#endif
