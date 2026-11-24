/*
 * Copyright (c) 2000-2020 Apple Inc. All rights reserved.
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
  
   
  
                        
                                                    
             
  
                                  
   

#include <stdint.h>
#include <ptrauth.h>

#include <debug.h>
#include <mach_pagemap.h>
#include <mach_cluster_stats.h>

#include <mach/mach_types.h>
#include <mach/memory_object.h>
#include <mach/memory_object_default.h>
#include <mach/memory_object_control_server.h>
#include <mach/mach_host_server.h>
#include <mach/upl.h>
#include <mach/vm_map.h>
#include <mach/vm_param.h>
#include <mach/vm_statistics.h>
#include <mach/sdt.h>

#include <kern/kern_types.h>
#include <kern/counter.h>
#include <kern/host_statistics.h>
#include <kern/machine.h>
#include <kern/misc_protos.h>
#include <kern/sched.h>
#include <kern/thread.h>
#include <kern/kalloc.h>
#include <kern/zalloc_internal.h>
#include <kern/policy_internal.h>
#include <kern/thread_group.h>

#include <machine/vm_tuning.h>
#include <machine/commpage.h>

#include <vm/pmap.h>
#include <vm/vm_compressor_pager.h>
#include <vm/vm_fault.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_page.h>
#include <vm/vm_pageout.h>
#include <vm/vm_protos.h>                   
#include <vm/memory_object.h>
#include <vm/vm_purgeable_internal.h>
#include <vm/vm_shared_region.h>
#include <vm/vm_compressor.h>

#include <san/kasan.h>

#if CONFIG_PHANTOM_CACHE
#include <vm/vm_phantom_cache.h>
#endif

#if UPL_DEBUG
#include <libkern/OSDebug.h>
#endif

extern int cs_debug;

extern void mbuf_drain(boolean_t);

#if VM_PRESSURE_EVENTS
#if CONFIG_JETSAM
extern unsigned int memorystatus_available_pages;
extern unsigned int memorystatus_available_pages_pressure;
extern unsigned int memorystatus_available_pages_critical;
#else                    
extern uint64_t memorystatus_available_pages;
extern uint64_t memorystatus_available_pages_pressure;
extern uint64_t memorystatus_available_pages_critical;
#endif                    

extern unsigned int memorystatus_frozen_count;
extern unsigned int memorystatus_suspended_count;
extern vm_pressure_level_t memorystatus_vm_pressure_level;

extern lck_mtx_t memorystatus_jetsam_fg_band_lock;
extern uint32_t memorystatus_jetsam_fg_band_waiters;

void vm_pressure_response(void);
extern void consider_vm_pressure_events(void);

#define MEMORYSTATUS_SUSPENDED_THRESHOLD  4
#endif                         

thread_t  vm_pageout_scan_thread = THREAD_NULL;
boolean_t vps_dynamic_priority_enabled = FALSE;

#ifndef VM_PAGEOUT_BURST_INACTIVE_THROTTLE                                                                             
#if !XNU_TARGET_OS_OSX
#define VM_PAGEOUT_BURST_INACTIVE_THROTTLE 1024
#else                         
#define VM_PAGEOUT_BURST_INACTIVE_THROTTLE 4096
#endif                         
#endif

#ifndef VM_PAGEOUT_DEADLOCK_RELIEF
#define VM_PAGEOUT_DEADLOCK_RELIEF 100                                                 
#endif

#ifndef VM_PAGE_LAUNDRY_MAX
#define VM_PAGE_LAUNDRY_MAX     128UL                                                  
#endif                              

#ifndef VM_PAGEOUT_BURST_WAIT
#define VM_PAGEOUT_BURST_WAIT   1                         
#endif                             

#ifndef VM_PAGEOUT_EMPTY_WAIT
#define VM_PAGEOUT_EMPTY_WAIT   50                        
#endif                             

#ifndef VM_PAGEOUT_DEADLOCK_WAIT
#define VM_PAGEOUT_DEADLOCK_WAIT 100                      
#endif                                

#ifndef VM_PAGEOUT_IDLE_WAIT
#define VM_PAGEOUT_IDLE_WAIT    10                        
#endif                            

#ifndef VM_PAGEOUT_SWAP_WAIT
#define VM_PAGEOUT_SWAP_WAIT    10                        
#endif                            


#ifndef VM_PAGE_SPECULATIVE_TARGET
#define VM_PAGE_SPECULATIVE_TARGET(total) ((total) * 1 / (100 / vm_pageout_state.vm_page_speculative_percentage))
#endif                                 


  
                                                               
                                                              
                                                             
                                                    
                                                                
  
                                                            
                                                           
                                          
   

#ifndef VM_PAGE_INACTIVE_TARGET
#define VM_PAGE_INACTIVE_TARGET(avail)  ((avail) * 1 / 2)
#endif                               

  
                                                         
                                                                 
   

#ifndef VM_PAGE_FREE_TARGET
#if !XNU_TARGET_OS_OSX
#define VM_PAGE_FREE_TARGET(free)       (15 + (free) / 100)
#else                         
#define VM_PAGE_FREE_TARGET(free)       (15 + (free) / 80)
#endif                         
#endif                           


  
                                                                   
                                
   

#ifndef VM_PAGE_FREE_MIN
#if !XNU_TARGET_OS_OSX
#define VM_PAGE_FREE_MIN(free)          (10 + (free) / 200)
#else                         
#define VM_PAGE_FREE_MIN(free)          (10 + (free) / 100)
#endif                         
#endif                        

#if !XNU_TARGET_OS_OSX
#define VM_PAGE_FREE_RESERVED_LIMIT     100
#define VM_PAGE_FREE_MIN_LIMIT          1500
#define VM_PAGE_FREE_TARGET_LIMIT       2000
#else                         
#define VM_PAGE_FREE_RESERVED_LIMIT     1700
#define VM_PAGE_FREE_MIN_LIMIT          3500
#define VM_PAGE_FREE_TARGET_LIMIT       4000
#endif                         

  
                                                             
                                                               
                                                             
                                                             
                                                        
   

#ifndef VM_PAGE_FREE_RESERVED
#define VM_PAGE_FREE_RESERVED(n)        \
	((unsigned) (6 * VM_PAGE_LAUNDRY_MAX) + (n))
#endif                             

  
                                                         
                                                                
                                                           
                                                               
                                                               
                                              
   
#define VM_PAGE_REACTIVATE_LIMIT_MAX 20000

#ifndef VM_PAGE_REACTIVATE_LIMIT
#if !XNU_TARGET_OS_OSX
#define VM_PAGE_REACTIVATE_LIMIT(avail) (VM_PAGE_INACTIVE_TARGET(avail) / 2)
#else                         
#define VM_PAGE_REACTIVATE_LIMIT(avail) (MAX((avail) * 1 / 20,VM_PAGE_REACTIVATE_LIMIT_MAX))
#endif                         
#endif                                
#define VM_PAGEOUT_INACTIVE_FORCE_RECLAIM       1000

extern boolean_t hibernate_cleaning_in_progress;

  
                                              
   
struct cq {
	struct vm_pageout_queue *q;
	void                    *current_chead;
	char                    *scratch_buf;
	int                     id;
};

struct cq ciq[MAX_COMPRESSOR_THREAD_COUNT];


#if VM_PRESSURE_EVENTS
void vm_pressure_thread(void);

boolean_t VM_PRESSURE_NORMAL_TO_WARNING(void);
boolean_t VM_PRESSURE_WARNING_TO_CRITICAL(void);

boolean_t VM_PRESSURE_WARNING_TO_NORMAL(void);
boolean_t VM_PRESSURE_CRITICAL_TO_WARNING(void);
#endif

void vm_pageout_garbage_collect(int);
static void vm_pageout_iothread_external(void);
static void vm_pageout_iothread_internal(struct cq *cq);
static void vm_pageout_adjust_eq_iothrottle(struct vm_pageout_queue *, boolean_t);

extern void vm_pageout_continue(void);
extern void vm_pageout_scan(void);

boolean_t vm_pageout_running = FALSE;

uint32_t vm_page_upl_tainted = 0;
uint32_t vm_page_iopl_tainted = 0;

#if XNU_TARGET_OS_OSX
static boolean_t vm_pageout_waiter  = FALSE;
#endif                        


#if DEVELOPMENT || DEBUG
struct vm_pageout_debug vm_pageout_debug;
#endif
struct vm_pageout_vminfo vm_pageout_vminfo;
struct vm_pageout_state  vm_pageout_state;
struct vm_config         vm_config;

struct  vm_pageout_queue vm_pageout_queue_internal VM_PAGE_PACKED_ALIGNED;
struct  vm_pageout_queue vm_pageout_queue_external VM_PAGE_PACKED_ALIGNED;

int         vm_upl_wait_for_pages = 0;
vm_object_t vm_pageout_scan_wants_object = VM_OBJECT_NULL;

boolean_t(*volatile consider_buffer_cache_collect)(int) = NULL;

int     vm_debug_events = 0;

LCK_GRP_DECLARE(vm_pageout_lck_grp, "vm_pageout");

#if CONFIG_MEMORYSTATUS
extern boolean_t memorystatus_kill_on_VM_page_shortage(boolean_t async);

uint32_t vm_pageout_memorystatus_fb_factor_nr = 5;
uint32_t vm_pageout_memorystatus_fb_factor_dr = 2;

#endif

#if __AMP__
int vm_compressor_ebound = 1;
int vm_pgo_pbound = 0;
extern void thread_bind_cluster_type(thread_t, char, bool);
#endif              


  
                                       
           
                                                      
                             
  
                     
                                                           
   
void
vm_pageout_object_terminate(
	vm_object_t     object)
{
	vm_object_t     shadow_object;

	  
                                                                   
                                                                   
                                         
    

	assert(object->pageout);
	shadow_object = object->shadow;
	vm_object_lock(shadow_object);

	while (!vm_page_queue_empty(&object->memq)) {
		vm_page_t               p, m;
		vm_object_offset_t      offset;

		p = (vm_page_t) vm_page_queue_first(&object->memq);

		assert(p->vmp_private);
		assert(p->vmp_free_when_done);
		p->vmp_free_when_done = FALSE;
		assert(!p->vmp_cleaning);
		assert(!p->vmp_laundry);

		offset = p->vmp_offset;
		VM_PAGE_FREE(p);
		p = VM_PAGE_NULL;

		m = vm_page_lookup(shadow_object,
		    offset + object->vo_shadow_offset);

		if (m == VM_PAGE_NULL) {
			continue;
		}

		assert((m->vmp_dirty) || (m->vmp_precious) ||
		    (m->vmp_busy && m->vmp_cleaning));

		  
                                       
                                                     
     
		vm_page_lock_queues();
		if (m->vmp_q_state == VM_PAGE_ON_PAGEOUT_Q) {
			vm_pageout_throttle_up(m);
		}

		  
                                                                
                                                                
                                                                 
                                                               
                                                             
                                              
     
		if (m->vmp_free_when_done) {
			assert(m->vmp_busy);
			assert(m->vmp_q_state == VM_PAGE_IS_WIRED);
			assert(m->vmp_wire_count == 1);
			m->vmp_cleaning = FALSE;
			m->vmp_free_when_done = FALSE;
			  
                                                        
                                                          
                                                         
              
     
                                                          
                                                      
                                           
      
			if (pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m)) & VM_MEM_MODIFIED) {
				SET_PAGE_DIRTY(m, FALSE);
			} else {
				m->vmp_dirty = FALSE;
			}

			if (m->vmp_dirty) {
				vm_page_unwire(m, TRUE);                         
				counter_inc(&vm_statistics_reactivations);
				PAGE_WAKEUP_DONE(m);
			} else {
				vm_page_free(m);                         
			}
			vm_page_unlock_queues();
			continue;
		}
		  
                                                             
                                     
                                                            
                             
     
		if ((m->vmp_q_state == VM_PAGE_NOT_ON_Q) && !m->vmp_private) {
			if (m->vmp_reference) {
				vm_page_activate(m);
			} else {
				vm_page_deactivate(m);
			}
		}
		if (m->vmp_overwriting) {
			  
                                                         
      
			if (m->vmp_busy) {
				  
                                      
                                                  
                                                        
                                                    
                                                      
                                                      
                                                  
       
				pmap_clear_modify(VM_PAGE_GET_PHYS_PAGE(m));

				m->vmp_busy = FALSE;
				m->vmp_absent = FALSE;
			} else {
				  
                                                                
                                              
                                      
       
				assert(VM_PAGE_WIRED(m));
				vm_page_unwire(m, TRUE);                         
			}
			m->vmp_overwriting = FALSE;
		} else {
			m->vmp_dirty = FALSE;
		}
		m->vmp_cleaning = FALSE;

		  
                                                              
     
		PAGE_WAKEUP(m);
		vm_page_unlock_queues();
	}
	  
                                                                        
    
	vm_object_activity_end(shadow_object);
	vm_object_unlock(shadow_object);

	assert(object->ref_count == 0);
	assert(object->paging_in_progress == 0);
	assert(object->activity_in_progress == 0);
	assert(object->resident_page_count == 0);
	return;
}

  
                              
  
                                                                
                                               
                                              
  
                                             
                   
  
   
static void
vm_pageclean_setup(
	vm_page_t               m,
	vm_page_t               new_m,
	vm_object_t             new_object,
	vm_object_offset_t      new_offset)
{
	assert(!m->vmp_busy);
#if 0
	assert(!m->vmp_cleaning);
#endif

	pmap_clear_modify(VM_PAGE_GET_PHYS_PAGE(m));

	  
                                            
    
	m->vmp_cleaning = TRUE;
	SET_PAGE_DIRTY(m, FALSE);
	m->vmp_precious = FALSE;

	  
                                                      
                  
    
	assert(new_m->vmp_fictitious);
	assert(VM_PAGE_GET_PHYS_PAGE(new_m) == vm_page_fictitious_addr);
	new_m->vmp_fictitious = FALSE;
	new_m->vmp_private = TRUE;
	new_m->vmp_free_when_done = TRUE;
	VM_PAGE_SET_PHYS_PAGE(new_m, VM_PAGE_GET_PHYS_PAGE(m));

	vm_page_lockspin_queues();
	vm_page_wire(new_m, VM_KERN_MEMORY_NONE, TRUE);
	vm_page_unlock_queues();

	vm_page_insert_wired(new_m, new_object, new_offset, VM_KERN_MEMORY_NONE);
	assert(!new_m->vmp_wanted);
	new_m->vmp_busy = FALSE;
}

  
                                      
           
                                                  
                                                               
                                                          
                     
  
                                                          
  
                     
                                                           
                                                  
                                                           
  
                  
                                              
   
void
vm_pageout_initialize_page(
	vm_page_t       m)
{
	vm_object_t             object;
	vm_object_offset_t      paging_offset;
	memory_object_t         pager;

	assert(VM_CONFIG_COMPRESSOR_IS_PRESENT);

	object = VM_PAGE_OBJECT(m);

	assert(m->vmp_busy);
	assert(object->internal);

	  
                                                 
    
	assert(!m->vmp_absent);
	assert(!m->vmp_error);
	assert(m->vmp_dirty);

	  
                                                             
    
	paging_offset = m->vmp_offset + object->paging_offset;

	if (m->vmp_absent || m->vmp_error || m->vmp_restart || (!m->vmp_dirty && !m->vmp_precious)) {
		panic("reservation without pageout?");           

		VM_PAGE_FREE(m);
		vm_object_unlock(object);

		return;
	}

	  
                                                                   
                                                                     
                                                            
    

	pager = object->pager;

	if (pager == MEMORY_OBJECT_NULL) {
		panic("missing pager for copy object");

		VM_PAGE_FREE(m);
		return;
	}

	  
                                                         
    
	pmap_clear_modify(VM_PAGE_GET_PHYS_PAGE(m));
	SET_PAGE_DIRTY(m, FALSE);

	  
                                                  
    
	vm_object_paging_begin(object);
	vm_object_unlock(object);

	  
                                
                                                          
                                                       
                                                       
                                                        
                               
    
	memory_object_data_initialize(pager, paging_offset, PAGE_SIZE);

	vm_object_lock(object);
	vm_object_paging_end(object);
}


  
                      
  
                                                        
                                                             
                         
  
                                                       
                                                               
                                                                 
                                              
  
                                             
   
#if DEVELOPMENT || DEBUG
vmct_stats_t vmct_stats;

int32_t vmct_active = 0;
uint64_t vm_compressor_epoch_start = 0;
uint64_t vm_compressor_epoch_stop = 0;

typedef enum vmct_state_t {
	VMCT_IDLE,
	VMCT_AWAKENED,
	VMCT_ACTIVE,
} vmct_state_t;
vmct_state_t vmct_state[MAX_COMPRESSOR_THREAD_COUNT];
#endif


void
vm_pageout_cluster(vm_page_t m)
{
	vm_object_t     object = VM_PAGE_OBJECT(m);
	struct          vm_pageout_queue *q;

	VM_PAGE_CHECK(m);
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);
	vm_object_lock_assert_exclusive(object);

	  
                                                    
    
	assert((m->vmp_dirty || m->vmp_precious) && (!VM_PAGE_WIRED(m)));
	assert(!m->vmp_cleaning && !m->vmp_laundry);
	assert(m->vmp_q_state == VM_PAGE_NOT_ON_Q);

	  
                                                   
    
	vm_object_activity_begin(object);

	if (object->internal == TRUE) {
		assert(VM_CONFIG_COMPRESSOR_IS_PRESENT);

		m->vmp_busy = TRUE;

		q = &vm_pageout_queue_internal;
	} else {
		q = &vm_pageout_queue_external;
	}

	  
                                                
    
	m->vmp_laundry = TRUE;
	q->pgo_laundry++;

	m->vmp_q_state = VM_PAGE_ON_PAGEOUT_Q;
	vm_page_queue_enter(&q->pgo_pending, m, vmp_pageq);

	if (q->pgo_idle == TRUE) {
		q->pgo_idle = FALSE;
		thread_wakeup((event_t) &q->pgo_pending);
	}
	VM_PAGE_CHECK(m);
}


  
                                                              
                                                                
                                                       
  
                                         
   
void
vm_pageout_throttle_up(
	vm_page_t       m)
{
	struct vm_pageout_queue *q;
	vm_object_t      m_object;

	m_object = VM_PAGE_OBJECT(m);

	assert(m_object != VM_OBJECT_NULL);
	assert(m_object != kernel_object);

	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);
	vm_object_lock_assert_exclusive(m_object);

	if (m_object->internal == TRUE) {
		q = &vm_pageout_queue_internal;
	} else {
		q = &vm_pageout_queue_external;
	}

	if (m->vmp_q_state == VM_PAGE_ON_PAGEOUT_Q) {
		vm_page_queue_remove(&q->pgo_pending, m, vmp_pageq);
		m->vmp_q_state = VM_PAGE_NOT_ON_Q;

		VM_PAGE_ZERO_PAGEQ_ENTRY(m);

		vm_object_activity_end(m_object);

		VM_PAGEOUT_DEBUG(vm_page_steal_pageout_page, 1);
	}
	if (m->vmp_laundry == TRUE) {
		m->vmp_laundry = FALSE;
		q->pgo_laundry--;

		if (q->pgo_throttled == TRUE) {
			q->pgo_throttled = FALSE;
			thread_wakeup((event_t) &q->pgo_laundry);
		}
		if (q->pgo_draining == TRUE && q->pgo_laundry == 0) {
			q->pgo_draining = FALSE;
			thread_wakeup((event_t) (&q->pgo_laundry + 1));
		}
		VM_PAGEOUT_DEBUG(vm_pageout_throttle_up_count, 1);
	}
}


static void
vm_pageout_throttle_up_batch(
	struct vm_pageout_queue *q,
	int             batch_cnt)
{
	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);

	VM_PAGEOUT_DEBUG(vm_pageout_throttle_up_count, batch_cnt);

	q->pgo_laundry -= batch_cnt;

	if (q->pgo_throttled == TRUE) {
		q->pgo_throttled = FALSE;
		thread_wakeup((event_t) &q->pgo_laundry);
	}
	if (q->pgo_draining == TRUE && q->pgo_laundry == 0) {
		q->pgo_draining = FALSE;
		thread_wakeup((event_t) (&q->pgo_laundry + 1));
	}
}



  
                                 
  
                                                                        
                                                                          
  
                                                                           
                                                                            
                                                        
  
                                                                             
                                                                          
                              
                                                                            
                                                                             
                  
   
#if DEVELOPMENT || DEBUG
#define VM_PAGEOUT_STAT_SIZE    (30 * 8) + 1
#else
#define VM_PAGEOUT_STAT_SIZE    (1 * 8) + 1
#endif
struct vm_pageout_stat {
	unsigned long vm_page_active_count;
	unsigned long vm_page_speculative_count;
	unsigned long vm_page_inactive_count;
	unsigned long vm_page_anonymous_count;

	unsigned long vm_page_free_count;
	unsigned long vm_page_wire_count;
	unsigned long vm_page_compressor_count;

	unsigned long vm_page_pages_compressed;
	unsigned long vm_page_pageable_internal_count;
	unsigned long vm_page_pageable_external_count;
	unsigned long vm_page_xpmapped_external_count;

	unsigned int pages_grabbed;
	unsigned int pages_freed;

	unsigned int pages_compressed;
	unsigned int pages_grabbed_by_compressor;
	unsigned int failed_compressions;

	unsigned int pages_evicted;
	unsigned int pages_purged;

	unsigned int considered;
	unsigned int considered_bq_internal;
	unsigned int considered_bq_external;

	unsigned int skipped_external;
	unsigned int filecache_min_reactivations;

	unsigned int freed_speculative;
	unsigned int freed_cleaned;
	unsigned int freed_internal;
	unsigned int freed_external;

	unsigned int cleaned_dirty_external;
	unsigned int cleaned_dirty_internal;

	unsigned int inactive_referenced;
	unsigned int inactive_nolock;
	unsigned int reactivation_limit_exceeded;
	unsigned int forced_inactive_reclaim;

	unsigned int throttled_internal_q;
	unsigned int throttled_external_q;

	unsigned int phantom_ghosts_found;
	unsigned int phantom_ghosts_added;
} vm_pageout_stats[VM_PAGEOUT_STAT_SIZE] = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, };

unsigned int vm_pageout_stat_now = 0;

#define VM_PAGEOUT_STAT_BEFORE(i) \
	(((i) == 0) ? VM_PAGEOUT_STAT_SIZE - 1 : (i) - 1)
#define VM_PAGEOUT_STAT_AFTER(i) \
	(((i) == VM_PAGEOUT_STAT_SIZE - 1) ? 0 : (i) + 1)

#if VM_PAGE_BUCKETS_CHECK
int vm_page_buckets_check_interval = 80;                             
#endif                            


void
record_memory_pressure(void);
void
record_memory_pressure(void)
{
	unsigned int vm_pageout_next;

#if VM_PAGE_BUCKETS_CHECK
	                                                                  
	static int counter = 0;
	if ((++counter % vm_page_buckets_check_interval) == 0) {
		vm_page_buckets_check();
	}
#endif                            

	vm_pageout_state.vm_memory_pressure =
	    vm_pageout_stats[VM_PAGEOUT_STAT_BEFORE(vm_pageout_stat_now)].freed_speculative +
	    vm_pageout_stats[VM_PAGEOUT_STAT_BEFORE(vm_pageout_stat_now)].freed_cleaned +
	    vm_pageout_stats[VM_PAGEOUT_STAT_BEFORE(vm_pageout_stat_now)].freed_internal +
	    vm_pageout_stats[VM_PAGEOUT_STAT_BEFORE(vm_pageout_stat_now)].freed_external;

	commpage_set_memory_pressure((unsigned int)vm_pageout_state.vm_memory_pressure );

	                        
	vm_pageout_next = VM_PAGEOUT_STAT_AFTER(vm_pageout_stat_now);

	bzero(&vm_pageout_stats[vm_pageout_next], sizeof(struct vm_pageout_stat));

	vm_pageout_stat_now = vm_pageout_next;
}


  
            
                                                           
                                                                  
                                                                    
                              
   
unsigned int
mach_vm_ctl_page_free_wanted(void)
{
	unsigned int page_free_target, page_free_count, page_free_wanted;

	page_free_target = vm_page_free_target;
	page_free_count = vm_page_free_count;
	if (page_free_target > page_free_count) {
		page_free_wanted = page_free_target - page_free_count;
	} else {
		page_free_wanted = 0;
	}

	return page_free_wanted;
}


  
             
                                                                     
                                                                     
                                                                    
                     
   

kern_return_t
mach_vm_pressure_monitor(
	boolean_t       wait_for_pressure,
	unsigned int    nsecs_monitored,
	unsigned int    *pages_reclaimed_p,
	unsigned int    *pages_wanted_p)
{
	wait_result_t   wr;
	unsigned int    vm_pageout_then, vm_pageout_now;
	unsigned int    pages_reclaimed;
	unsigned int    units_of_monitor;

	units_of_monitor = 8 * nsecs_monitored;
	  
                                                                   
                                                                    
                                                                   
                                 
    

	if (wait_for_pressure) {
		                                        
		while (vm_page_free_count >= vm_page_free_target) {
			wr = assert_wait((event_t) &vm_page_free_wanted,
			    THREAD_INTERRUPTIBLE);
			if (wr == THREAD_WAITING) {
				wr = thread_block(THREAD_CONTINUE_NULL);
			}
			if (wr == THREAD_INTERRUPTED) {
				return KERN_ABORTED;
			}
			if (wr == THREAD_AWAKENED) {
				  
                                             
                                              
                                             
                              
       
				break;
			}
		}
	}

	                                                             
	if (pages_wanted_p != NULL) {
		*pages_wanted_p = mach_vm_ctl_page_free_wanted();
	}

	if (pages_reclaimed_p == NULL) {
		return KERN_SUCCESS;
	}

	                                                                     
	vm_pageout_now = vm_pageout_stat_now;
	pages_reclaimed = 0;
	for (vm_pageout_then =
	    VM_PAGEOUT_STAT_BEFORE(vm_pageout_now);
	    vm_pageout_then != vm_pageout_now &&
	    units_of_monitor-- != 0;
	    vm_pageout_then =
	    VM_PAGEOUT_STAT_BEFORE(vm_pageout_then)) {
		pages_reclaimed += vm_pageout_stats[vm_pageout_then].freed_speculative;
		pages_reclaimed += vm_pageout_stats[vm_pageout_then].freed_cleaned;
		pages_reclaimed += vm_pageout_stats[vm_pageout_then].freed_internal;
		pages_reclaimed += vm_pageout_stats[vm_pageout_then].freed_external;
	}
	*pages_reclaimed_p = pages_reclaimed;

	return KERN_SUCCESS;
}



#if DEVELOPMENT || DEBUG

static void
vm_pageout_disconnect_all_pages_in_queue(vm_page_queue_head_t *, int);

  
                                                
                                         
   
boolean_t       vm_pageout_disconnect_all_pages_active = FALSE;


void
vm_pageout_disconnect_all_pages()
{
	vm_page_lock_queues();

	if (vm_pageout_disconnect_all_pages_active == TRUE) {
		vm_page_unlock_queues();
		return;
	}
	vm_pageout_disconnect_all_pages_active = TRUE;
	vm_page_unlock_queues();

	vm_pageout_disconnect_all_pages_in_queue(&vm_page_queue_throttled, vm_page_throttled_count);
	vm_pageout_disconnect_all_pages_in_queue(&vm_page_queue_anonymous, vm_page_anonymous_count);
	vm_pageout_disconnect_all_pages_in_queue(&vm_page_queue_active, vm_page_active_count);

	vm_pageout_disconnect_all_pages_active = FALSE;
}


void
vm_pageout_disconnect_all_pages_in_queue(vm_page_queue_head_t *q, int qcount)
{
	vm_page_t       m;
	vm_object_t     t_object = NULL;
	vm_object_t     l_object = NULL;
	vm_object_t     m_object = NULL;
	int             delayed_unlock = 0;
	int             try_failed_count = 0;
	int             disconnected_count = 0;
	int             paused_count = 0;
	int             object_locked_count = 0;

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_DISCONNECT_ALL_PAGE_MAPPINGS)) | DBG_FUNC_START,
	    q, qcount, 0, 0, 0);

	vm_page_lock_queues();

	while (qcount && !vm_page_queue_empty(q)) {
		LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);

		m = (vm_page_t) vm_page_queue_first(q);
		m_object = VM_PAGE_OBJECT(m);

		  
                                             
                                         
                         
     
		if (m_object != l_object) {
			  
                                                  
                                                 
                                              
      
			if (l_object != NULL) {
				vm_object_unlock(l_object);
				l_object = NULL;
			}
			if (m_object != t_object) {
				try_failed_count = 0;
			}

			  
                                                    
                                                       
                                                     
                                                       
            
      
			if (!vm_object_lock_try_scan(m_object)) {
				if (try_failed_count > 20) {
					goto reenter_pg_on_q;
				}
				vm_page_unlock_queues();
				mutex_pause(try_failed_count++);
				vm_page_lock_queues();
				delayed_unlock = 0;

				paused_count++;

				t_object = m_object;
				continue;
			}
			object_locked_count++;

			l_object = m_object;
		}
		if (!m_object->alive || m->vmp_cleaning || m->vmp_laundry || m->vmp_busy || m->vmp_absent || m->vmp_error || m->vmp_free_when_done) {
			  
                                          
      
			goto reenter_pg_on_q;
		}
		if (m->vmp_pmapped == TRUE) {
			pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m));

			disconnected_count++;
		}
reenter_pg_on_q:
		vm_page_queue_remove(q, m, vmp_pageq);
		vm_page_queue_enter(q, m, vmp_pageq);

		qcount--;
		try_failed_count = 0;

		if (delayed_unlock++ > 128) {
			if (l_object != NULL) {
				vm_object_unlock(l_object);
				l_object = NULL;
			}
			lck_mtx_yield(&vm_page_queue_lock);
			delayed_unlock = 0;
		}
	}
	if (l_object != NULL) {
		vm_object_unlock(l_object);
		l_object = NULL;
	}
	vm_page_unlock_queues();

	KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE, (MACHDBG_CODE(DBG_MACH_WORKINGSET, VM_DISCONNECT_ALL_PAGE_MAPPINGS)) | DBG_FUNC_END,
	    q, disconnected_count, object_locked_count, paused_count, 0);
}

#endif


static void
vm_pageout_page_queue(vm_page_queue_head_t *, int);

  
                                                
                                         
   
boolean_t       vm_pageout_anonymous_pages_active = FALSE;


void
vm_pageout_anonymous_pages()
{
	if (VM_CONFIG_COMPRESSOR_IS_PRESENT) {
		vm_page_lock_queues();

		if (vm_pageout_anonymous_pages_active == TRUE) {
			vm_page_unlock_queues();
			return;
		}
		vm_pageout_anonymous_pages_active = TRUE;
		vm_page_unlock_queues();

		vm_pageout_page_queue(&vm_page_queue_throttled, vm_page_throttled_count);
		vm_pageout_page_queue(&vm_page_queue_anonymous, vm_page_anonymous_count);
		vm_pageout_page_queue(&vm_page_queue_active, vm_page_active_count);

		if (VM_CONFIG_SWAP_IS_PRESENT) {
			vm_consider_swapping();
		}

		vm_page_lock_queues();
		vm_pageout_anonymous_pages_active = FALSE;
		vm_page_unlock_queues();
	}
}


void
vm_pageout_page_queue(vm_page_queue_head_t *q, int qcount)
{
	vm_page_t       m;
	vm_object_t     t_object = NULL;
	vm_object_t     l_object = NULL;
	vm_object_t     m_object = NULL;
	int             delayed_unlock = 0;
	int             try_failed_count = 0;
	int             refmod_state;
	int             pmap_options;
	struct          vm_pageout_queue *iq;
	ppnum_t         phys_page;


	iq = &vm_pageout_queue_internal;

	vm_page_lock_queues();

	while (qcount && !vm_page_queue_empty(q)) {
		LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);

		if (VM_PAGE_Q_THROTTLED(iq)) {
			if (l_object != NULL) {
				vm_object_unlock(l_object);
				l_object = NULL;
			}
			iq->pgo_draining = TRUE;

			assert_wait((event_t) (&iq->pgo_laundry + 1), THREAD_INTERRUPTIBLE);
			vm_page_unlock_queues();

			thread_block(THREAD_CONTINUE_NULL);

			vm_page_lock_queues();
			delayed_unlock = 0;
			continue;
		}
		m = (vm_page_t) vm_page_queue_first(q);
		m_object = VM_PAGE_OBJECT(m);

		  
                                             
                                         
                         
     
		if (m_object != l_object) {
			if (!m_object->internal) {
				goto reenter_pg_on_q;
			}

			  
                                                  
                                                 
                                              
      
			if (l_object != NULL) {
				vm_object_unlock(l_object);
				l_object = NULL;
			}
			if (m_object != t_object) {
				try_failed_count = 0;
			}

			  
                                                    
                                                       
                                                     
                                                       
            
      
			if (!vm_object_lock_try_scan(m_object)) {
				if (try_failed_count > 20) {
					goto reenter_pg_on_q;
				}
				vm_page_unlock_queues();
				mutex_pause(try_failed_count++);
				vm_page_lock_queues();
				delayed_unlock = 0;

				t_object = m_object;
				continue;
			}
			l_object = m_object;
		}
		if (!m_object->alive || m->vmp_cleaning || m->vmp_laundry || m->vmp_busy || m->vmp_absent || m->vmp_error || m->vmp_free_when_done) {
			  
                               
                                          
      
			goto reenter_pg_on_q;
		}
		phys_page = VM_PAGE_GET_PHYS_PAGE(m);

		if (m->vmp_reference == FALSE && m->vmp_pmapped == TRUE) {
			refmod_state = pmap_get_refmod(phys_page);

			if (refmod_state & VM_MEM_REFERENCED) {
				m->vmp_reference = TRUE;
			}
			if (refmod_state & VM_MEM_MODIFIED) {
				SET_PAGE_DIRTY(m, FALSE);
			}
		}
		if (m->vmp_reference == TRUE) {
			m->vmp_reference = FALSE;
			pmap_clear_refmod_options(phys_page, VM_MEM_REFERENCED, PMAP_OPTIONS_NOFLUSH, (void *)NULL);
			goto reenter_pg_on_q;
		}
		if (m->vmp_pmapped == TRUE) {
			if (m->vmp_dirty || m->vmp_precious) {
				pmap_options = PMAP_OPTIONS_COMPRESSOR;
			} else {
				pmap_options = PMAP_OPTIONS_COMPRESSOR_IFF_MODIFIED;
			}
			refmod_state = pmap_disconnect_options(phys_page, pmap_options, NULL);
			if (refmod_state & VM_MEM_MODIFIED) {
				SET_PAGE_DIRTY(m, FALSE);
			}
		}

		if (!m->vmp_dirty && !m->vmp_precious) {
			vm_page_unlock_queues();
			VM_PAGE_FREE(m);
			vm_page_lock_queues();
			delayed_unlock = 0;

			goto next_pg;
		}
		if (!m_object->pager_initialized || m_object->pager == MEMORY_OBJECT_NULL) {
			if (!m_object->pager_initialized) {
				vm_page_unlock_queues();

				vm_object_collapse(m_object, (vm_object_offset_t) 0, TRUE);

				if (!m_object->pager_initialized) {
					vm_object_compressor_pager_create(m_object);
				}

				vm_page_lock_queues();
				delayed_unlock = 0;
			}
			if (!m_object->pager_initialized || m_object->pager == MEMORY_OBJECT_NULL) {
				goto reenter_pg_on_q;
			}
			  
                                                                 
                                                   
      
			continue;
		}
		  
                                                          
                                                          
                                         
     
		vm_page_queues_remove(m, TRUE);

		LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);

		vm_pageout_cluster(m);

		goto next_pg;

reenter_pg_on_q:
		vm_page_queue_remove(q, m, vmp_pageq);
		vm_page_queue_enter(q, m, vmp_pageq);
next_pg:
		qcount--;
		try_failed_count = 0;

		if (delayed_unlock++ > 128) {
			if (l_object != NULL) {
				vm_object_unlock(l_object);
				l_object = NULL;
			}
			lck_mtx_yield(&vm_page_queue_lock);
			delayed_unlock = 0;
		}
	}
	if (l_object != NULL) {
		vm_object_unlock(l_object);
		l_object = NULL;
	}
	vm_page_unlock_queues();
}



  
                                                              
   
extern void vm_pageout_io_throttle(void);

#define VM_PAGEOUT_SCAN_HANDLE_REUSABLE_PAGE(m, obj)                    \
	MACRO_BEGIN                                                     \
	    
                                                    
                                                    
                      
                                                      
                                                  
                                                    
                                                                 \
	assert(VM_PAGE_OBJECT((m)) == (obj));                           \
	if ((m)->vmp_reusable ||                                        \
	    (obj)->all_reusable) {                                      \
	        vm_object_reuse_pages((obj),                            \
	                              (m)->vmp_offset,                  \
	                              (m)->vmp_offset + PAGE_SIZE_64,   \
	                              FALSE);                           \
	}                                                               \
	MACRO_END


#define VM_PAGEOUT_DELAYED_UNLOCK_LIMIT         64
#define VM_PAGEOUT_DELAYED_UNLOCK_LIMIT_MAX     1024

#define FCS_IDLE                0
#define FCS_DELAYED             1
#define FCS_DEADLOCK_DETECTED   2

struct flow_control {
	int             state;
	mach_timespec_t ts;
};


#if CONFIG_BACKGROUND_QUEUE
uint64_t vm_pageout_rejected_bq_internal = 0;
uint64_t vm_pageout_rejected_bq_external = 0;
uint64_t vm_pageout_skipped_bq_internal = 0;
#endif

#define ANONS_GRABBED_LIMIT     2


#if 0
static void vm_pageout_delayed_unlock(int *, int *, vm_page_t *);
#endif
static void vm_pageout_prepare_to_block(vm_object_t *, int *, vm_page_t *, int *, int);

#define VM_PAGEOUT_PB_NO_ACTION                         0
#define VM_PAGEOUT_PB_CONSIDER_WAKING_COMPACTOR_SWAPPER 1
#define VM_PAGEOUT_PB_THREAD_YIELD                      2


#if 0
static void
vm_pageout_delayed_unlock(int *delayed_unlock, int *local_freed, vm_page_t *local_freeq)
{
	if (*local_freeq) {
		vm_page_unlock_queues();

		VM_DEBUG_CONSTANT_EVENT(
			vm_pageout_freelist, VM_PAGEOUT_FREELIST, DBG_FUNC_START,
			vm_page_free_count, 0, 0, 1);

		vm_page_free_list(*local_freeq, TRUE);

		VM_DEBUG_CONSTANT_EVENT(vm_pageout_freelist, VM_PAGEOUT_FREELIST, DBG_FUNC_END,
		    vm_page_free_count, *local_freed, 0, 1);

		*local_freeq = NULL;
		*local_freed = 0;

		vm_page_lock_queues();
	} else {
		lck_mtx_yield(&vm_page_queue_lock);
	}
	*delayed_unlock = 1;
}
#endif


static void
vm_pageout_prepare_to_block(vm_object_t *object, int *delayed_unlock,
    vm_page_t *local_freeq, int *local_freed, int action)
{
	vm_page_unlock_queues();

	if (*object != NULL) {
		vm_object_unlock(*object);
		*object = NULL;
	}
	if (*local_freeq) {
		vm_page_free_list(*local_freeq, TRUE);

		*local_freeq = NULL;
		*local_freed = 0;
	}
	*delayed_unlock = 1;

	switch (action) {
	case VM_PAGEOUT_PB_CONSIDER_WAKING_COMPACTOR_SWAPPER:
		vm_consider_waking_compactor_swapper();
		break;
	case VM_PAGEOUT_PB_THREAD_YIELD:
		thread_yield_internal(1);
		break;
	case VM_PAGEOUT_PB_NO_ACTION:
	default:
		break;
	}
	vm_page_lock_queues();
}


static struct vm_pageout_vminfo last;

uint64_t last_vm_page_pages_grabbed = 0;

extern  uint32_t c_segment_pages_compressed;

extern uint64_t shared_region_pager_reclaimed;
extern struct memory_object_pager_ops shared_region_pager_ops;

void
update_vm_info(void)
{
	unsigned long tmp;
	uint64_t tmp64;

	vm_pageout_stats[vm_pageout_stat_now].vm_page_active_count = vm_page_active_count;
	vm_pageout_stats[vm_pageout_stat_now].vm_page_speculative_count = vm_page_speculative_count;
	vm_pageout_stats[vm_pageout_stat_now].vm_page_inactive_count = vm_page_inactive_count;
	vm_pageout_stats[vm_pageout_stat_now].vm_page_anonymous_count = vm_page_anonymous_count;

	vm_pageout_stats[vm_pageout_stat_now].vm_page_free_count = vm_page_free_count;
	vm_pageout_stats[vm_pageout_stat_now].vm_page_wire_count = vm_page_wire_count;
	vm_pageout_stats[vm_pageout_stat_now].vm_page_compressor_count = VM_PAGE_COMPRESSOR_COUNT;

	vm_pageout_stats[vm_pageout_stat_now].vm_page_pages_compressed = c_segment_pages_compressed;
	vm_pageout_stats[vm_pageout_stat_now].vm_page_pageable_internal_count = vm_page_pageable_internal_count;
	vm_pageout_stats[vm_pageout_stat_now].vm_page_pageable_external_count = vm_page_pageable_external_count;
	vm_pageout_stats[vm_pageout_stat_now].vm_page_xpmapped_external_count = vm_page_xpmapped_external_count;


	tmp = vm_pageout_vminfo.vm_pageout_considered_page;
	vm_pageout_stats[vm_pageout_stat_now].considered = (unsigned int)(tmp - last.vm_pageout_considered_page);
	last.vm_pageout_considered_page = tmp;

	tmp64 = vm_pageout_vminfo.vm_pageout_compressions;
	vm_pageout_stats[vm_pageout_stat_now].pages_compressed = (unsigned int)(tmp64 - last.vm_pageout_compressions);
	last.vm_pageout_compressions = tmp64;

	tmp = vm_pageout_vminfo.vm_compressor_failed;
	vm_pageout_stats[vm_pageout_stat_now].failed_compressions = (unsigned int)(tmp - last.vm_compressor_failed);
	last.vm_compressor_failed = tmp;

	tmp64 = vm_pageout_vminfo.vm_compressor_pages_grabbed;
	vm_pageout_stats[vm_pageout_stat_now].pages_grabbed_by_compressor = (unsigned int)(tmp64 - last.vm_compressor_pages_grabbed);
	last.vm_compressor_pages_grabbed = tmp64;

	tmp = vm_pageout_vminfo.vm_phantom_cache_found_ghost;
	vm_pageout_stats[vm_pageout_stat_now].phantom_ghosts_found = (unsigned int)(tmp - last.vm_phantom_cache_found_ghost);
	last.vm_phantom_cache_found_ghost = tmp;

	tmp = vm_pageout_vminfo.vm_phantom_cache_added_ghost;
	vm_pageout_stats[vm_pageout_stat_now].phantom_ghosts_added = (unsigned int)(tmp - last.vm_phantom_cache_added_ghost);
	last.vm_phantom_cache_added_ghost = tmp;

	tmp64 = counter_load(&vm_page_grab_count);
	vm_pageout_stats[vm_pageout_stat_now].pages_grabbed = (unsigned int)(tmp64 - last_vm_page_pages_grabbed);
	last_vm_page_pages_grabbed = tmp64;

	tmp = vm_pageout_vminfo.vm_page_pages_freed;
	vm_pageout_stats[vm_pageout_stat_now].pages_freed = (unsigned int)(tmp - last.vm_page_pages_freed);
	last.vm_page_pages_freed = tmp;


	if (vm_pageout_stats[vm_pageout_stat_now].considered) {
		tmp = vm_pageout_vminfo.vm_pageout_pages_evicted;
		vm_pageout_stats[vm_pageout_stat_now].pages_evicted = (unsigned int)(tmp - last.vm_pageout_pages_evicted);
		last.vm_pageout_pages_evicted = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_pages_purged;
		vm_pageout_stats[vm_pageout_stat_now].pages_purged = (unsigned int)(tmp - last.vm_pageout_pages_purged);
		last.vm_pageout_pages_purged = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_freed_speculative;
		vm_pageout_stats[vm_pageout_stat_now].freed_speculative = (unsigned int)(tmp - last.vm_pageout_freed_speculative);
		last.vm_pageout_freed_speculative = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_freed_external;
		vm_pageout_stats[vm_pageout_stat_now].freed_external = (unsigned int)(tmp - last.vm_pageout_freed_external);
		last.vm_pageout_freed_external = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_inactive_referenced;
		vm_pageout_stats[vm_pageout_stat_now].inactive_referenced = (unsigned int)(tmp - last.vm_pageout_inactive_referenced);
		last.vm_pageout_inactive_referenced = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_scan_inactive_throttled_external;
		vm_pageout_stats[vm_pageout_stat_now].throttled_external_q = (unsigned int)(tmp - last.vm_pageout_scan_inactive_throttled_external);
		last.vm_pageout_scan_inactive_throttled_external = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_inactive_dirty_external;
		vm_pageout_stats[vm_pageout_stat_now].cleaned_dirty_external = (unsigned int)(tmp - last.vm_pageout_inactive_dirty_external);
		last.vm_pageout_inactive_dirty_external = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_freed_cleaned;
		vm_pageout_stats[vm_pageout_stat_now].freed_cleaned = (unsigned int)(tmp - last.vm_pageout_freed_cleaned);
		last.vm_pageout_freed_cleaned = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_inactive_nolock;
		vm_pageout_stats[vm_pageout_stat_now].inactive_nolock = (unsigned int)(tmp - last.vm_pageout_inactive_nolock);
		last.vm_pageout_inactive_nolock = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_scan_inactive_throttled_internal;
		vm_pageout_stats[vm_pageout_stat_now].throttled_internal_q = (unsigned int)(tmp - last.vm_pageout_scan_inactive_throttled_internal);
		last.vm_pageout_scan_inactive_throttled_internal = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_skipped_external;
		vm_pageout_stats[vm_pageout_stat_now].skipped_external = (unsigned int)(tmp - last.vm_pageout_skipped_external);
		last.vm_pageout_skipped_external = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_reactivation_limit_exceeded;
		vm_pageout_stats[vm_pageout_stat_now].reactivation_limit_exceeded = (unsigned int)(tmp - last.vm_pageout_reactivation_limit_exceeded);
		last.vm_pageout_reactivation_limit_exceeded = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_inactive_force_reclaim;
		vm_pageout_stats[vm_pageout_stat_now].forced_inactive_reclaim = (unsigned int)(tmp - last.vm_pageout_inactive_force_reclaim);
		last.vm_pageout_inactive_force_reclaim = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_freed_internal;
		vm_pageout_stats[vm_pageout_stat_now].freed_internal = (unsigned int)(tmp - last.vm_pageout_freed_internal);
		last.vm_pageout_freed_internal = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_considered_bq_internal;
		vm_pageout_stats[vm_pageout_stat_now].considered_bq_internal = (unsigned int)(tmp - last.vm_pageout_considered_bq_internal);
		last.vm_pageout_considered_bq_internal = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_considered_bq_external;
		vm_pageout_stats[vm_pageout_stat_now].considered_bq_external = (unsigned int)(tmp - last.vm_pageout_considered_bq_external);
		last.vm_pageout_considered_bq_external = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_filecache_min_reactivated;
		vm_pageout_stats[vm_pageout_stat_now].filecache_min_reactivations = (unsigned int)(tmp - last.vm_pageout_filecache_min_reactivated);
		last.vm_pageout_filecache_min_reactivated = tmp;

		tmp = vm_pageout_vminfo.vm_pageout_inactive_dirty_internal;
		vm_pageout_stats[vm_pageout_stat_now].cleaned_dirty_internal = (unsigned int)(tmp - last.vm_pageout_inactive_dirty_internal);
		last.vm_pageout_inactive_dirty_internal = tmp;
	}

	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_INFO1)) | DBG_FUNC_NONE,
	    vm_pageout_stats[vm_pageout_stat_now].vm_page_active_count,
	    vm_pageout_stats[vm_pageout_stat_now].vm_page_speculative_count,
	    vm_pageout_stats[vm_pageout_stat_now].vm_page_inactive_count,
	    vm_pageout_stats[vm_pageout_stat_now].vm_page_anonymous_count,
	    0);

	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_INFO2)) | DBG_FUNC_NONE,
	    vm_pageout_stats[vm_pageout_stat_now].vm_page_free_count,
	    vm_pageout_stats[vm_pageout_stat_now].vm_page_wire_count,
	    vm_pageout_stats[vm_pageout_stat_now].vm_page_compressor_count,
	    0,
	    0);

	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_INFO3)) | DBG_FUNC_NONE,
	    vm_pageout_stats[vm_pageout_stat_now].vm_page_pages_compressed,
	    vm_pageout_stats[vm_pageout_stat_now].vm_page_pageable_internal_count,
	    vm_pageout_stats[vm_pageout_stat_now].vm_page_pageable_external_count,
	    vm_pageout_stats[vm_pageout_stat_now].vm_page_xpmapped_external_count,
	    0);

	if (vm_pageout_stats[vm_pageout_stat_now].considered ||
	    vm_pageout_stats[vm_pageout_stat_now].pages_compressed ||
	    vm_pageout_stats[vm_pageout_stat_now].failed_compressions) {
		KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_INFO4)) | DBG_FUNC_NONE,
		    vm_pageout_stats[vm_pageout_stat_now].considered,
		    vm_pageout_stats[vm_pageout_stat_now].freed_speculative,
		    vm_pageout_stats[vm_pageout_stat_now].freed_external,
		    vm_pageout_stats[vm_pageout_stat_now].inactive_referenced,
		    0);

		KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_INFO5)) | DBG_FUNC_NONE,
		    vm_pageout_stats[vm_pageout_stat_now].throttled_external_q,
		    vm_pageout_stats[vm_pageout_stat_now].cleaned_dirty_external,
		    vm_pageout_stats[vm_pageout_stat_now].freed_cleaned,
		    vm_pageout_stats[vm_pageout_stat_now].inactive_nolock,
		    0);

		KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_INFO6)) | DBG_FUNC_NONE,
		    vm_pageout_stats[vm_pageout_stat_now].throttled_internal_q,
		    vm_pageout_stats[vm_pageout_stat_now].pages_compressed,
		    vm_pageout_stats[vm_pageout_stat_now].pages_grabbed_by_compressor,
		    vm_pageout_stats[vm_pageout_stat_now].skipped_external,
		    0);

		KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_INFO7)) | DBG_FUNC_NONE,
		    vm_pageout_stats[vm_pageout_stat_now].reactivation_limit_exceeded,
		    vm_pageout_stats[vm_pageout_stat_now].forced_inactive_reclaim,
		    vm_pageout_stats[vm_pageout_stat_now].failed_compressions,
		    vm_pageout_stats[vm_pageout_stat_now].freed_internal,
		    0);

		KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_INFO8)) | DBG_FUNC_NONE,
		    vm_pageout_stats[vm_pageout_stat_now].considered_bq_internal,
		    vm_pageout_stats[vm_pageout_stat_now].considered_bq_external,
		    vm_pageout_stats[vm_pageout_stat_now].filecache_min_reactivations,
		    vm_pageout_stats[vm_pageout_stat_now].cleaned_dirty_internal,
		    0);
	}
	KERNEL_DEBUG_CONSTANT((MACHDBG_CODE(DBG_MACH_VM, VM_INFO9)) | DBG_FUNC_NONE,
	    vm_pageout_stats[vm_pageout_stat_now].pages_grabbed,
	    vm_pageout_stats[vm_pageout_stat_now].pages_freed,
	    vm_pageout_stats[vm_pageout_stat_now].phantom_ghosts_found,
	    vm_pageout_stats[vm_pageout_stat_now].phantom_ghosts_added,
	    0);

	record_memory_pressure();
}

extern boolean_t hibernation_vmqueues_inspection;

  
                                                        
                         
  
                                                                
                                                                                 
                                                                            
   

#define VM_PAGEOUT_SCAN_PROCEED                 (0)
#define VM_PAGEOUT_SCAN_DONE_RETURN             (1)
#define VM_PAGEOUT_SCAN_NEXT_ITERATION          (2)

  
                                                        
                                                          
                                      
   
static void
vps_deal_with_secluded_page_overflow(vm_page_t *local_freeq, int *local_freed)
{
#if CONFIG_SECLUDED_MEMORY
	  
                                  
    
	if (vm_page_secluded_count > vm_page_secluded_target) {
		vm_page_t secluded_page;

		  
                                  
                                                     
                                         
     
		assert((vm_page_secluded_count_free +
		    vm_page_secluded_count_inuse) ==
		    vm_page_secluded_count);
		secluded_page = (vm_page_t)vm_page_queue_first(&vm_page_queue_secluded);
		assert(secluded_page->vmp_q_state == VM_PAGE_ON_SECLUDED_Q);

		vm_page_queues_remove(secluded_page, FALSE);
		assert(!secluded_page->vmp_fictitious);
		assert(!VM_PAGE_WIRED(secluded_page));

		if (secluded_page->vmp_object == 0) {
			                            
			assert(secluded_page->vmp_busy);
			secluded_page->vmp_snext = *local_freeq;
			*local_freeq = secluded_page;
			*local_freed += 1;
		} else {
			                                      
			vm_page_enqueue_active(secluded_page, FALSE);
			secluded_page = VM_PAGE_NULL;
		}
	}
#else                             

#pragma unused(local_freeq)
#pragma unused(local_freed)

	return;

#endif                             
}

  
                                                        
                                                         
   
static void
vps_init_page_targets(void)
{
	  
                                                              
    
	vm_page_anonymous_min = vm_page_inactive_target / 20;

	if (vm_pageout_state.vm_page_speculative_percentage > 50) {
		vm_pageout_state.vm_page_speculative_percentage = 50;
	} else if (vm_pageout_state.vm_page_speculative_percentage <= 0) {
		vm_pageout_state.vm_page_speculative_percentage = 1;
	}

	vm_pageout_state.vm_page_speculative_target = VM_PAGE_SPECULATIVE_TARGET(vm_page_active_count +
	    vm_page_inactive_count);
}

  
                                                        
                                                         
                                                                     
   
static int
vps_purge_object()
{
	int             force_purge;

	assert(available_for_purge >= 0);
	force_purge = 0;                       

#if VM_PRESSURE_EVENTS
	vm_pressure_level_t pressure_level;

	pressure_level = memorystatus_vm_pressure_level;

	if (pressure_level > kVMPressureNormal) {
		if (pressure_level >= kVMPressureCritical) {
			force_purge = vm_pageout_state.memorystatus_purge_on_critical;
		} else if (pressure_level >= kVMPressureUrgent) {
			force_purge = vm_pageout_state.memorystatus_purge_on_urgent;
		} else if (pressure_level >= kVMPressureWarning) {
			force_purge = vm_pageout_state.memorystatus_purge_on_warning;
		}
	}
#endif                         

	if (available_for_purge || force_purge) {
		memoryshot(VM_PAGEOUT_PURGEONE, DBG_FUNC_START);

		VM_DEBUG_EVENT(vm_pageout_purgeone, VM_PAGEOUT_PURGEONE, DBG_FUNC_START, vm_page_free_count, 0, 0, 0);
		if (vm_purgeable_object_purge_one(force_purge, C_DONT_BLOCK)) {
			VM_PAGEOUT_DEBUG(vm_pageout_purged_objects, 1);
			VM_DEBUG_EVENT(vm_pageout_purgeone, VM_PAGEOUT_PURGEONE, DBG_FUNC_END, vm_page_free_count, 0, 0, 0);
			memoryshot(VM_PAGEOUT_PURGEONE, DBG_FUNC_END);

			return VM_PAGEOUT_SCAN_NEXT_ITERATION;
		}
		VM_DEBUG_EVENT(vm_pageout_purgeone, VM_PAGEOUT_PURGEONE, DBG_FUNC_END, 0, 0, 0, -1);
		memoryshot(VM_PAGEOUT_PURGEONE, DBG_FUNC_END);
	}

	return VM_PAGEOUT_SCAN_PROCEED;
}

  
                                                        
                                                          
                
   
static int
vps_age_speculative_queue(boolean_t force_speculative_aging)
{
#define DELAY_SPECULATIVE_AGE   1000

	  
                                            
                                           
                        
    
	boolean_t                       can_steal = FALSE;
	int                             num_scanned_queues;
	static int                      delay_speculative_age = 0;                                                                      
	mach_timespec_t                 ts;
	struct vm_speculative_age_q     *aq;
	struct vm_speculative_age_q     *sq;

	sq = &vm_page_queue_speculative[VM_PAGE_SPECULATIVE_AGED_Q];

	aq = &vm_page_queue_speculative[speculative_steal_index];

	num_scanned_queues = 0;
	while (vm_page_queue_empty(&aq->age_q) &&
	    num_scanned_queues++ != VM_PAGE_MAX_SPECULATIVE_AGE_Q) {
		speculative_steal_index++;

		if (speculative_steal_index > VM_PAGE_MAX_SPECULATIVE_AGE_Q) {
			speculative_steal_index = VM_PAGE_MIN_SPECULATIVE_AGE_Q;
		}

		aq = &vm_page_queue_speculative[speculative_steal_index];
	}

	if (num_scanned_queues == VM_PAGE_MAX_SPECULATIVE_AGE_Q + 1) {
		  
                                          
                                       
                                   
                                        
     
		if (!vm_page_queue_empty(&sq->age_q)) {
			return VM_PAGEOUT_SCAN_NEXT_ITERATION;
		}
#if DEVELOPMENT || DEBUG
		panic("vm_pageout_scan: vm_page_speculative_count=%d but queues are empty", vm_page_speculative_count);
#endif
		                 
		vm_page_speculative_count = 0;
		                      
		return VM_PAGEOUT_SCAN_NEXT_ITERATION;
	}

	if (vm_page_speculative_count > vm_pageout_state.vm_page_speculative_target || force_speculative_aging == TRUE) {
		can_steal = TRUE;
	} else {
		if (!delay_speculative_age) {
			mach_timespec_t ts_fully_aged;

			ts_fully_aged.tv_sec = (VM_PAGE_MAX_SPECULATIVE_AGE_Q * vm_pageout_state.vm_page_speculative_q_age_ms) / 1000;
			ts_fully_aged.tv_nsec = ((VM_PAGE_MAX_SPECULATIVE_AGE_Q * vm_pageout_state.vm_page_speculative_q_age_ms) % 1000)
			    * 1000 * NSEC_PER_USEC;

			ADD_MACH_TIMESPEC(&ts_fully_aged, &aq->age_ts);

			clock_sec_t sec;
			clock_nsec_t nsec;
			clock_get_system_nanotime(&sec, &nsec);
			ts.tv_sec = (unsigned int) sec;
			ts.tv_nsec = nsec;

			if (CMP_MACH_TIMESPEC(&ts, &ts_fully_aged) >= 0) {
				can_steal = TRUE;
			} else {
				delay_speculative_age++;
			}
		} else {
			delay_speculative_age++;
			if (delay_speculative_age == DELAY_SPECULATIVE_AGE) {
				delay_speculative_age = 0;
			}
		}
	}
	if (can_steal == TRUE) {
		vm_page_speculate_ageit(aq);
	}

	return VM_PAGEOUT_SCAN_PROCEED;
}

  
                                                        
                                               
   
static int inline
vps_object_cache_evict(vm_object_t *object_to_unlock)
{
	static int                      cache_evict_throttle = 0;
	struct vm_speculative_age_q     *sq;

	sq = &vm_page_queue_speculative[VM_PAGE_SPECULATIVE_AGED_Q];

	if (vm_page_queue_empty(&sq->age_q) && cache_evict_throttle == 0) {
		int     pages_evicted;

		if (*object_to_unlock != NULL) {
			vm_object_unlock(*object_to_unlock);
			*object_to_unlock = NULL;
		}
		KERNEL_DEBUG_CONSTANT(0x13001ec | DBG_FUNC_START, 0, 0, 0, 0, 0);

		pages_evicted = vm_object_cache_evict(100, 10);

		KERNEL_DEBUG_CONSTANT(0x13001ec | DBG_FUNC_END, pages_evicted, 0, 0, 0, 0);

		if (pages_evicted) {
			vm_pageout_vminfo.vm_pageout_pages_evicted += pages_evicted;

			VM_DEBUG_EVENT(vm_pageout_cache_evict, VM_PAGEOUT_CACHE_EVICT, DBG_FUNC_NONE,
			    vm_page_free_count, pages_evicted, vm_pageout_vminfo.vm_pageout_pages_evicted, 0);
			memoryshot(VM_PAGEOUT_CACHE_EVICT, DBG_FUNC_NONE);

			  
                                    
                                            
                                          
      
			return VM_PAGEOUT_SCAN_NEXT_ITERATION;
		} else {
			cache_evict_throttle = 1000;
		}
	}
	if (cache_evict_throttle) {
		cache_evict_throttle--;
	}

	return VM_PAGEOUT_SCAN_PROCEED;
}


  
                                                        
                                                               
                              
   
static void
vps_calculate_filecache_min(void)
{
	int divisor = vm_pageout_state.vm_page_filecache_min_divisor;

#if CONFIG_JETSAM
	  
                                                                  
                                                               
                                    
   
                                                                  
                                                                  
                                                             
                                                            
                                    
    
	if (vm_compressor_low_on_space() || divisor == 0) {
		vm_pageout_state.vm_page_filecache_min = 0;
	} else {
		vm_pageout_state.vm_page_filecache_min =
		    ((AVAILABLE_NON_COMPRESSED_MEMORY) * 10) / divisor;
	}
#else
	if (vm_compressor_out_of_space() || divisor == 0) {
		vm_pageout_state.vm_page_filecache_min = 0;
	} else {
		  
                                                                        
     
		vm_pageout_state.vm_page_filecache_min =
		    ((AVAILABLE_NON_COMPRESSED_MEMORY) * 10) / divisor;
	}
#endif
	if (vm_page_free_count < (vm_page_free_reserved / 4)) {
		vm_pageout_state.vm_page_filecache_min = 0;
	}
}

  
                                                        
                                                               
                         
   
static void
vps_flow_control_reset_deadlock_timer(struct flow_control *flow_control)
{
	mach_timespec_t ts;
	clock_sec_t sec;
	clock_nsec_t nsec;

	ts.tv_sec = vm_pageout_state.vm_pageout_deadlock_wait / 1000;
	ts.tv_nsec = (vm_pageout_state.vm_pageout_deadlock_wait % 1000) * 1000 * NSEC_PER_USEC;
	clock_get_system_nanotime(&sec, &nsec);
	flow_control->ts.tv_sec = (unsigned int) sec;
	flow_control->ts.tv_nsec = nsec;
	ADD_MACH_TIMESPEC(&flow_control->ts, &ts);

	flow_control->state = FCS_DELAYED;

	vm_pageout_vminfo.vm_pageout_scan_inactive_throttled_internal++;
}

  
                                                        
                                                        
                                                
                                                                 
   
static int
vps_flow_control(struct flow_control *flow_control, int *anons_grabbed, vm_object_t *object, int *delayed_unlock,
    vm_page_t *local_freeq, int *local_freed, int *vm_pageout_deadlock_target, unsigned int inactive_burst_count)
{
	boolean_t       exceeded_burst_throttle = FALSE;
	unsigned int    msecs = 0;
	uint32_t        inactive_external_count;
	mach_timespec_t ts;
	struct  vm_pageout_queue *iq;
	struct  vm_pageout_queue *eq;
	struct  vm_speculative_age_q *sq;

	iq = &vm_pageout_queue_internal;
	eq = &vm_pageout_queue_external;
	sq = &vm_page_queue_speculative[VM_PAGE_SPECULATIVE_AGED_Q];

	  
                               
                                         
                                                                     
                                                                         
                                                   
    
	if (vm_page_queue_empty(&vm_page_queue_inactive) &&
	    vm_page_queue_empty(&vm_page_queue_anonymous) &&
	    vm_page_queue_empty(&vm_page_queue_cleaned) &&
	    vm_page_queue_empty(&sq->age_q)) {
		VM_PAGEOUT_DEBUG(vm_pageout_scan_empty_throttle, 1);
		msecs = vm_pageout_state.vm_pageout_empty_wait;
	} else if (inactive_burst_count >=
	    MIN(vm_pageout_state.vm_pageout_burst_inactive_throttle,
	    (vm_page_inactive_count +
	    vm_page_speculative_count))) {
		VM_PAGEOUT_DEBUG(vm_pageout_scan_burst_throttle, 1);
		msecs = vm_pageout_state.vm_pageout_burst_wait;

		exceeded_burst_throttle = TRUE;
	} else if (VM_PAGE_Q_THROTTLED(iq) &&
	    VM_DYNAMIC_PAGING_ENABLED()) {
		clock_sec_t sec;
		clock_nsec_t nsec;

		switch (flow_control->state) {
		case FCS_IDLE:
			if ((vm_page_free_count + *local_freed) < vm_page_free_target &&
			    vm_pageout_state.vm_restricted_to_single_processor == FALSE) {
				  
                                                                       
                                                                            
                                                                  
       
				inactive_external_count = vm_page_inactive_count - vm_page_anonymous_count;

				if (vm_page_pageable_external_count > vm_pageout_state.vm_page_filecache_min &&
				    (inactive_external_count >= VM_PAGE_INACTIVE_TARGET(vm_page_pageable_external_count))) {
					*anons_grabbed = ANONS_GRABBED_LIMIT;
					VM_PAGEOUT_DEBUG(vm_pageout_scan_throttle_deferred, 1);
					return VM_PAGEOUT_SCAN_PROCEED;
				}
			}

			vps_flow_control_reset_deadlock_timer(flow_control);
			msecs = vm_pageout_state.vm_pageout_deadlock_wait;

			break;

		case FCS_DELAYED:
			clock_get_system_nanotime(&sec, &nsec);
			ts.tv_sec = (unsigned int) sec;
			ts.tv_nsec = nsec;

			if (CMP_MACH_TIMESPEC(&ts, &flow_control->ts) >= 0) {
				  
                                                              
                           
                                                               
                                                                  
                                                                      
                                                                     
                                                                   
                                                                       
                                                                        
                                                                
                                                                   
                                                                   
                                                                
                             
       

				*vm_pageout_deadlock_target = vm_pageout_state.vm_pageout_deadlock_relief +
				    vm_page_free_wanted + vm_page_free_wanted_privileged;
				VM_PAGEOUT_DEBUG(vm_pageout_scan_deadlock_detected, 1);
				flow_control->state = FCS_DEADLOCK_DETECTED;
				thread_wakeup((event_t) &vm_pageout_garbage_collect);
				return VM_PAGEOUT_SCAN_PROCEED;
			}
			  
                                    
                                                      
                                                     
                                                
      
			msecs = vm_pageout_state.vm_pageout_idle_wait;
			break;

		case FCS_DEADLOCK_DETECTED:
			if (*vm_pageout_deadlock_target) {
				return VM_PAGEOUT_SCAN_PROCEED;
			}

			vps_flow_control_reset_deadlock_timer(flow_control);
			msecs = vm_pageout_state.vm_pageout_deadlock_wait;

			break;
		}
	} else {
		  
                        
     
		return VM_PAGEOUT_SCAN_PROCEED;
	}

	vm_pageout_scan_wants_object = VM_OBJECT_NULL;

	vm_pageout_prepare_to_block(object, delayed_unlock, local_freeq, local_freed,
	    VM_PAGEOUT_PB_CONSIDER_WAKING_COMPACTOR_SWAPPER);

	if (vm_page_free_count >= vm_page_free_target) {
		  
                       
                                                      
                                  
                                               
                                              
    
                                             
                                             
    
                                                        
                                                           
                                                      
                                                        
                                                          
             
    
                                                       
                                                      
                                                           
                                                       
                                                        
                                                    
                                           
     
		vm_pageout_adjust_eq_iothrottle(eq, TRUE);
	}
	lck_mtx_lock(&vm_page_queue_free_lock);

	if (vm_page_free_count >= vm_page_free_target &&
	    (vm_page_free_wanted == 0) && (vm_page_free_wanted_privileged == 0)) {
		return VM_PAGEOUT_SCAN_DONE_RETURN;
	}
	lck_mtx_unlock(&vm_page_queue_free_lock);

	if ((vm_page_free_count + vm_page_cleaned_count) < vm_page_free_target) {
		  
                                                   
                                                   
                                             
                                              
                                             
                                           
                          
    
                                                   
                                                
                                               
     
		vm_pageout_adjust_eq_iothrottle(eq, FALSE);
	}
	if (vm_page_cleaned_count > 0 && exceeded_burst_throttle == FALSE) {
		  
                                                   
                                                  
                                                
                                
                                                   
                                                 
                           
     
		flow_control->state = FCS_IDLE;
		return VM_PAGEOUT_SCAN_PROCEED;
	}
	if (flow_control->state == FCS_DELAYED && !VM_PAGE_Q_THROTTLED(iq)) {
		flow_control->state = FCS_IDLE;
		return VM_PAGEOUT_SCAN_PROCEED;
	}

	VM_CHECK_MEMORYSTATUS;

	if (flow_control->state != FCS_IDLE) {
		VM_PAGEOUT_DEBUG(vm_pageout_scan_throttle, 1);
	}

	iq->pgo_throttled = TRUE;
	assert_wait_timeout((event_t) &iq->pgo_laundry, THREAD_INTERRUPTIBLE, msecs, 1000 * NSEC_PER_USEC);

	vm_page_unlock_queues();

	assert(vm_pageout_scan_wants_object == VM_OBJECT_NULL);

	VM_DEBUG_EVENT(vm_pageout_thread_block, VM_PAGEOUT_THREAD_BLOCK, DBG_FUNC_START,
	    iq->pgo_laundry, iq->pgo_maxlaundry, msecs, 0);
	memoryshot(VM_PAGEOUT_THREAD_BLOCK, DBG_FUNC_START);

	thread_block(THREAD_CONTINUE_NULL);

	VM_DEBUG_EVENT(vm_pageout_thread_block, VM_PAGEOUT_THREAD_BLOCK, DBG_FUNC_END,
	    iq->pgo_laundry, iq->pgo_maxlaundry, msecs, 0);
	memoryshot(VM_PAGEOUT_THREAD_BLOCK, DBG_FUNC_END);

	vm_page_lock_queues();

	iq->pgo_throttled = FALSE;

	vps_init_page_targets();

	return VM_PAGEOUT_SCAN_NEXT_ITERATION;
}

  
                                                        
                                                          
             
   
static int
vps_choose_victim_page(vm_page_t *victim_page, int *anons_grabbed, boolean_t *grab_anonymous, boolean_t force_anonymous,
    boolean_t *is_page_from_bg_q, unsigned int *reactivated_this_call)
{
	vm_page_t                       m = NULL;
	vm_object_t                     m_object = VM_OBJECT_NULL;
	uint32_t                        inactive_external_count;
	struct vm_speculative_age_q     *sq;
	struct vm_pageout_queue         *iq;
	int                             retval = VM_PAGEOUT_SCAN_PROCEED;

	sq = &vm_page_queue_speculative[VM_PAGE_SPECULATIVE_AGED_Q];
	iq = &vm_pageout_queue_internal;

	*is_page_from_bg_q = FALSE;

	m = NULL;
	m_object = VM_OBJECT_NULL;

	if (VM_DYNAMIC_PAGING_ENABLED()) {
		assert(vm_page_throttled_count == 0);
		assert(vm_page_queue_empty(&vm_page_queue_throttled));
	}

	  
                                        
                                                                    
                                                                            
    
	if (!vm_page_queue_empty(&vm_page_queue_cleaned)) {
		m = (vm_page_t) vm_page_queue_first(&vm_page_queue_cleaned);

		assert(m->vmp_q_state == VM_PAGE_ON_INACTIVE_CLEANED_Q);

		goto found_page;
	}

	  
                                                                    
                                                               
    
	if (!vm_page_queue_empty(&sq->age_q)) {
		m = (vm_page_t) vm_page_queue_first(&sq->age_q);

		assert(m->vmp_q_state == VM_PAGE_ON_SPECULATIVE_Q);

		if (!m->vmp_dirty || force_anonymous == FALSE) {
			goto found_page;
		} else {
			m = NULL;
		}
	}

#if CONFIG_BACKGROUND_QUEUE
	if (vm_page_background_mode != VM_PAGE_BG_DISABLED && (vm_page_background_count > vm_page_background_target)) {
		vm_object_t     bg_m_object = NULL;

		m = (vm_page_t) vm_page_queue_first(&vm_page_queue_background);

		bg_m_object = VM_PAGE_OBJECT(m);

		if (!VM_PAGE_PAGEABLE(m)) {
			  
                                          
                                           
                                          
                                       
                                           
                                          
                                         
           
      
		} else if (force_anonymous == FALSE || bg_m_object->internal) {
			if (bg_m_object->internal &&
			    (VM_PAGE_Q_THROTTLED(iq) ||
			    vm_compressor_out_of_space() == TRUE ||
			    vm_page_free_count < (vm_page_free_reserved / 4))) {
				vm_pageout_skipped_bq_internal++;
			} else {
				*is_page_from_bg_q = TRUE;

				if (bg_m_object->internal) {
					vm_pageout_vminfo.vm_pageout_considered_bq_internal++;
				} else {
					vm_pageout_vminfo.vm_pageout_considered_bq_external++;
				}
				goto found_page;
			}
		}
	}
#endif                              

	inactive_external_count = vm_page_inactive_count - vm_page_anonymous_count;

	if ((vm_page_pageable_external_count < vm_pageout_state.vm_page_filecache_min || force_anonymous == TRUE) ||
	    (inactive_external_count < VM_PAGE_INACTIVE_TARGET(vm_page_pageable_external_count))) {
		*grab_anonymous = TRUE;
		*anons_grabbed = 0;

		vm_pageout_vminfo.vm_pageout_skipped_external++;
		goto want_anonymous;
	}
	*grab_anonymous = (vm_page_anonymous_count > vm_page_anonymous_min);

#if CONFIG_JETSAM
	                                          
                                            
                                      
                                                     
                    
                                                       
                                                     
                                              
    

	struct vm_pageout_queue *eq = &vm_pageout_queue_external;

	if (*grab_anonymous == TRUE && !VM_PAGE_Q_THROTTLED(eq)) {
		if (vm_page_pageable_external_count >
		    vm_pageout_state.vm_page_filecache_min) {
			if ((vm_page_pageable_external_count *
			    vm_pageout_memorystatus_fb_factor_dr) >
			    (memorystatus_available_pages_critical *
			    vm_pageout_memorystatus_fb_factor_nr)) {
				*grab_anonymous = FALSE;

				VM_PAGEOUT_DEBUG(vm_grab_anon_overrides, 1);
			}
		}
		if (*grab_anonymous) {
			VM_PAGEOUT_DEBUG(vm_grab_anon_nops, 1);
		}
	}
#endif                    

want_anonymous:
	if (*grab_anonymous == FALSE || *anons_grabbed >= ANONS_GRABBED_LIMIT || vm_page_queue_empty(&vm_page_queue_anonymous)) {
		if (!vm_page_queue_empty(&vm_page_queue_inactive)) {
			m = (vm_page_t) vm_page_queue_first(&vm_page_queue_inactive);

			assert(m->vmp_q_state == VM_PAGE_ON_INACTIVE_EXTERNAL_Q);
			*anons_grabbed = 0;

			if (vm_page_pageable_external_count < vm_pageout_state.vm_page_filecache_min) {
				if (!vm_page_queue_empty(&vm_page_queue_anonymous)) {
					if ((++(*reactivated_this_call) % 100)) {
						vm_pageout_vminfo.vm_pageout_filecache_min_reactivated++;

						vm_page_activate(m);
						counter_inc(&vm_statistics_reactivations);
#if CONFIG_BACKGROUND_QUEUE
#if DEVELOPMENT || DEBUG
						if (*is_page_from_bg_q == TRUE) {
							if (m_object->internal) {
								vm_pageout_rejected_bq_internal++;
							} else {
								vm_pageout_rejected_bq_external++;
							}
						}
#endif                           
#endif                              
						vm_pageout_state.vm_pageout_inactive_used++;

						m = NULL;
						retval = VM_PAGEOUT_SCAN_NEXT_ITERATION;

						goto found_page;
					}

					  
                                                
                                                
                               
        
				}
			}
			goto found_page;
		}
	}
	if (!vm_page_queue_empty(&vm_page_queue_anonymous)) {
		m = (vm_page_t) vm_page_queue_first(&vm_page_queue_anonymous);

		assert(m->vmp_q_state == VM_PAGE_ON_INACTIVE_INTERNAL_Q);
		*anons_grabbed += 1;

		goto found_page;
	}

	m = NULL;

found_page:
	*victim_page = m;

	return retval;
}

  
                                                        
                                                       
                                          
   
static void
vps_requeue_page(vm_page_t m, int page_prev_q_state, __unused boolean_t page_from_bg_q)
{
	if (page_prev_q_state == VM_PAGE_ON_SPECULATIVE_Q) {
		vm_page_enqueue_inactive(m, FALSE);
	} else {
		vm_page_activate(m);
	}

#if CONFIG_BACKGROUND_QUEUE
#if DEVELOPMENT || DEBUG
	vm_object_t m_object = VM_PAGE_OBJECT(m);

	if (page_from_bg_q == TRUE) {
		if (m_object->internal) {
			vm_pageout_rejected_bq_internal++;
		} else {
			vm_pageout_rejected_bq_external++;
		}
	}
#endif                           
#endif                              
}

  
                                                        
                                                             
                                                                 
   
static int
vps_switch_object(vm_page_t m, vm_object_t m_object, vm_object_t *object, int page_prev_q_state, boolean_t avoid_anon_pages, boolean_t page_from_bg_q)
{
	struct vm_speculative_age_q *sq;

	sq = &vm_page_queue_speculative[VM_PAGE_SPECULATIVE_AGED_Q];

	  
                                                
                                               
                                            
    
	if (*object != NULL) {
		vm_object_unlock(*object);
		*object = NULL;
	}
	  
                                                  
                                                     
                                                   
                                                     
                                                    
                                                       
                                                         
                                                               
    
	if (!vm_object_lock_try_scan(m_object)) {
		vm_page_t m_want = NULL;

		vm_pageout_vminfo.vm_pageout_inactive_nolock++;

		if (page_prev_q_state == VM_PAGE_ON_INACTIVE_CLEANED_Q) {
			VM_PAGEOUT_DEBUG(vm_pageout_cleaned_nolock, 1);
		}

		pmap_clear_reference(VM_PAGE_GET_PHYS_PAGE(m));

		m->vmp_reference = FALSE;

		if (!m_object->object_is_shared_cache) {
			  
                                                               
                                                                
              
                                                                        
                                                                  
                                                                      
                                                                            
                                                                          
                                                                          
                                    
      
			m_object->scan_collisions = 1;
		}
		if (!vm_page_queue_empty(&vm_page_queue_cleaned)) {
			m_want = (vm_page_t) vm_page_queue_first(&vm_page_queue_cleaned);
		} else if (!vm_page_queue_empty(&sq->age_q)) {
			m_want = (vm_page_t) vm_page_queue_first(&sq->age_q);
		} else if ((avoid_anon_pages || vm_page_queue_empty(&vm_page_queue_anonymous)) &&
		    !vm_page_queue_empty(&vm_page_queue_inactive)) {
			m_want = (vm_page_t) vm_page_queue_first(&vm_page_queue_inactive);
		} else if (!vm_page_queue_empty(&vm_page_queue_anonymous)) {
			m_want = (vm_page_t) vm_page_queue_first(&vm_page_queue_anonymous);
		}

		  
                                                            
                                                         
                    
     
		if (m_want) {
			vm_pageout_scan_wants_object = VM_PAGE_OBJECT(m_want);
		}

		vps_requeue_page(m, page_prev_q_state, page_from_bg_q);

		return VM_PAGEOUT_SCAN_NEXT_ITERATION;
	} else {
		*object = m_object;
		vm_pageout_scan_wants_object = VM_OBJECT_NULL;
	}

	return VM_PAGEOUT_SCAN_PROCEED;
}

  
                                                        
                                                           
                                                              
                                                               
                                                              
                                          
   
static void
vps_deal_with_throttled_queues(vm_page_t m, vm_object_t *object, uint32_t *vm_pageout_inactive_external_forced_reactivate_limit,
    int *delayed_unlock, boolean_t *force_anonymous, __unused boolean_t is_page_from_bg_q)
{
	struct  vm_pageout_queue *eq;
	vm_object_t cur_object = VM_OBJECT_NULL;

	cur_object = *object;

	eq = &vm_pageout_queue_external;

	if (cur_object->internal == FALSE) {
		  
                                                                 
                                                                                                                  
                                                                                          
                                                                     
    
                                                   
                                                
                                                                            
                                        
    
                                                                                     
                                                                                        
                                                                                      
    
                                                                           
                                                                                    
                                                                                        
                                                                                         
                 
    
                                                                                    
                                                                             
                                 
     
		vm_pageout_vminfo.vm_pageout_scan_inactive_throttled_external++;

		vm_page_check_pageable_safe(m);
		assert(m->vmp_q_state == VM_PAGE_NOT_ON_Q);
		vm_page_queue_enter(&vm_page_queue_active, m, vmp_pageq);
		m->vmp_q_state = VM_PAGE_ON_ACTIVE_Q;
		vm_page_active_count++;
		vm_page_pageable_external_count++;

		vm_pageout_adjust_eq_iothrottle(eq, FALSE);

#if CONFIG_MEMORYSTATUS && CONFIG_JETSAM

#pragma unused(force_anonymous)

		*vm_pageout_inactive_external_forced_reactivate_limit -= 1;

		if (*vm_pageout_inactive_external_forced_reactivate_limit <= 0) {
			*vm_pageout_inactive_external_forced_reactivate_limit = vm_page_active_count + vm_page_inactive_count;
			  
                                                         
      

			assert(cur_object);
			vm_object_unlock(cur_object);

			cur_object = VM_OBJECT_NULL;

			  
                                                                            
                                               
      
			*object = VM_OBJECT_NULL;

			vm_page_unlock_queues();

			VM_DEBUG_CONSTANT_EVENT(vm_pageout_jetsam, VM_PAGEOUT_JETSAM, DBG_FUNC_START,
			    vm_page_active_count, vm_page_inactive_count, vm_page_free_count, vm_page_free_count);

			                                                                                                              
			if (memorystatus_kill_on_VM_page_shortage(FALSE) == TRUE) {
				VM_PAGEOUT_DEBUG(vm_pageout_inactive_external_forced_jetsam_count, 1);
			}

			VM_DEBUG_CONSTANT_EVENT(vm_pageout_jetsam, VM_PAGEOUT_JETSAM, DBG_FUNC_END,
			    vm_page_active_count, vm_page_inactive_count, vm_page_free_count, vm_page_free_count);

			vm_page_lock_queues();
			*delayed_unlock = 1;
		}
#else                                           

#pragma unused(vm_pageout_inactive_external_forced_reactivate_limit)
#pragma unused(delayed_unlock)

		*force_anonymous = TRUE;
#endif                                           
	} else {
		vm_page_activate(m);
		counter_inc(&vm_statistics_reactivations);

#if CONFIG_BACKGROUND_QUEUE
#if DEVELOPMENT || DEBUG
		if (is_page_from_bg_q == TRUE) {
			if (cur_object->internal) {
				vm_pageout_rejected_bq_internal++;
			} else {
				vm_pageout_rejected_bq_external++;
			}
		}
#endif                           
#endif                              

		vm_pageout_state.vm_pageout_inactive_used++;
	}
}


void
vm_page_balance_inactive(int max_to_move)
{
	vm_page_t m;

	LCK_MTX_ASSERT(&vm_page_queue_lock, LCK_MTX_ASSERT_OWNED);

	if (hibernation_vmqueues_inspection || hibernate_cleaning_in_progress) {
		  
                                                   
                                                   
                                                     
                                     
    
                                                    
     
		return;
	}
	vm_page_inactive_target = VM_PAGE_INACTIVE_TARGET(vm_page_active_count +
	    vm_page_inactive_count +
	    vm_page_speculative_count);

	while (max_to_move-- && (vm_page_inactive_count + vm_page_speculative_count) < vm_page_inactive_target) {
		VM_PAGEOUT_DEBUG(vm_pageout_balanced, 1);

		m = (vm_page_t) vm_page_queue_first(&vm_page_queue_active);

		assert(m->vmp_q_state == VM_PAGE_ON_ACTIVE_Q);
		assert(!m->vmp_laundry);
		assert(VM_PAGE_OBJECT(m) != kernel_object);
		assert(VM_PAGE_GET_PHYS_PAGE(m) != vm_page_guard_addr);

		DTRACE_VM2(scan, int, 1, (uint64_t *), NULL);

		  
                                                                                                 
    
                                                                                           
                                                                                               
                                                                                                    
                                                                                                    
                                                                                               
                                                                                                 
                                           
     
		if (m->vmp_pmapped == TRUE) {
			pmap_clear_refmod_options(VM_PAGE_GET_PHYS_PAGE(m), VM_MEM_REFERENCED, PMAP_OPTIONS_NOFLUSH, (void *)NULL);
		}

		  
                                      
                                            
                                                             
     
		vm_page_deactivate_internal(m, FALSE);
	}
}


  
                                                              
                                                                      
                                     
   
void
vm_pageout_scan(void)
{
	unsigned int loop_count = 0;
	unsigned int inactive_burst_count = 0;
	unsigned int reactivated_this_call;
	unsigned int reactivate_limit;
	vm_page_t   local_freeq = NULL;
	int         local_freed = 0;
	int         delayed_unlock;
	int         delayed_unlock_limit = 0;
	int         refmod_state = 0;
	int     vm_pageout_deadlock_target = 0;
	struct  vm_pageout_queue *iq;
	struct  vm_pageout_queue *eq;
	struct  vm_speculative_age_q *sq;
	struct  flow_control    flow_control = { .state = 0, .ts = { .tv_sec = 0, .tv_nsec = 0 } };
	boolean_t inactive_throttled = FALSE;
	vm_object_t     object = NULL;
	uint32_t        inactive_reclaim_run;
	boolean_t       grab_anonymous = FALSE;
	boolean_t       force_anonymous = FALSE;
	boolean_t       force_speculative_aging = FALSE;
	int             anons_grabbed = 0;
	int             page_prev_q_state = 0;
	boolean_t       page_from_bg_q = FALSE;
	uint32_t        vm_pageout_inactive_external_forced_reactivate_limit = 0;
	vm_object_t     m_object = VM_OBJECT_NULL;
	int             retval = 0;
	boolean_t       lock_yield_check = FALSE;


	VM_DEBUG_CONSTANT_EVENT(vm_pageout_scan, VM_PAGEOUT_SCAN, DBG_FUNC_START,
	    vm_pageout_vminfo.vm_pageout_freed_speculative,
	    vm_pageout_state.vm_pageout_inactive_clean,
	    vm_pageout_vminfo.vm_pageout_inactive_dirty_internal,
	    vm_pageout_vminfo.vm_pageout_inactive_dirty_external);

	flow_control.state = FCS_IDLE;
	iq = &vm_pageout_queue_internal;
	eq = &vm_pageout_queue_external;
	sq = &vm_page_queue_speculative[VM_PAGE_SPECULATIVE_AGED_Q];

	                                                                
	uint64_t pmap_wired_pages_freed = pmap_release_pages_fast();

	vm_page_lock_queues();

	vm_page_wire_count -= pmap_wired_pages_freed;

	delayed_unlock = 1;

	  
                                                                
                                  
    
	reactivated_this_call = 0;
	reactivate_limit = VM_PAGE_REACTIVATE_LIMIT(vm_page_active_count +
	    vm_page_inactive_count);
	inactive_reclaim_run = 0;

	vm_pageout_inactive_external_forced_reactivate_limit = vm_page_active_count + vm_page_inactive_count;

	  
                                                               
                                                             
                                                               
                                                                
                                                              
                                                                 
                                                              
                                                          
    

	vps_init_page_targets();
	assert(object == NULL);
	assert(delayed_unlock != 0);

	for (;;) {
		vm_page_t m;

		DTRACE_VM2(rev, int, 1, (uint64_t *), NULL);

		if (lock_yield_check) {
			lock_yield_check = FALSE;

			if (delayed_unlock++ > delayed_unlock_limit) {
				int freed = local_freed;

				vm_pageout_prepare_to_block(&object, &delayed_unlock, &local_freeq, &local_freed,
				    VM_PAGEOUT_PB_CONSIDER_WAKING_COMPACTOR_SWAPPER);
				if (freed == 0) {
					lck_mtx_yield(&vm_page_queue_lock);
				}
			} else if (vm_pageout_scan_wants_object) {
				vm_page_unlock_queues();
				mutex_pause(0);
				vm_page_lock_queues();
			}
		}

		if (vm_upl_wait_for_pages < 0) {
			vm_upl_wait_for_pages = 0;
		}

		delayed_unlock_limit = VM_PAGEOUT_DELAYED_UNLOCK_LIMIT + vm_upl_wait_for_pages;

		if (delayed_unlock_limit > VM_PAGEOUT_DELAYED_UNLOCK_LIMIT_MAX) {
			delayed_unlock_limit = VM_PAGEOUT_DELAYED_UNLOCK_LIMIT_MAX;
		}

		vps_deal_with_secluded_page_overflow(&local_freeq, &local_freed);

		assert(delayed_unlock);

		  
                         
     
		vm_page_balance_inactive(1);


		                                                                       
                                                                      
                                                                 
                          
                                                                         

		if (vm_page_free_count + local_freed >= vm_page_free_target) {
			vm_pageout_scan_wants_object = VM_OBJECT_NULL;

			vm_pageout_prepare_to_block(&object, &delayed_unlock, &local_freeq, &local_freed,
			    VM_PAGEOUT_PB_CONSIDER_WAKING_COMPACTOR_SWAPPER);
			  
                                                   
                                                
                                                     
                                                         
                                                           
              
      
			vm_pageout_adjust_eq_iothrottle(eq, TRUE);

			lck_mtx_lock(&vm_page_queue_free_lock);

			if ((vm_page_free_count >= vm_page_free_target) &&
			    (vm_page_free_wanted == 0) && (vm_page_free_wanted_privileged == 0)) {
				  
                                          
                                          
       
return_from_scan:
				assert(vm_pageout_scan_wants_object == VM_OBJECT_NULL);

				VM_DEBUG_CONSTANT_EVENT(vm_pageout_scan, VM_PAGEOUT_SCAN, DBG_FUNC_NONE,
				    vm_pageout_state.vm_pageout_inactive,
				    vm_pageout_state.vm_pageout_inactive_used, 0, 0);
				VM_DEBUG_CONSTANT_EVENT(vm_pageout_scan, VM_PAGEOUT_SCAN, DBG_FUNC_END,
				    vm_pageout_vminfo.vm_pageout_freed_speculative,
				    vm_pageout_state.vm_pageout_inactive_clean,
				    vm_pageout_vminfo.vm_pageout_inactive_dirty_internal,
				    vm_pageout_vminfo.vm_pageout_inactive_dirty_external);

				return;
			}
			lck_mtx_unlock(&vm_page_queue_free_lock);
		}

		  
                                                           
                                                          
                                                                
                                                            
                              
     
		retval = vps_purge_object();

		if (retval == VM_PAGEOUT_SCAN_NEXT_ITERATION) {
			  
             
      
			if (object != NULL) {
				vm_object_unlock(object);
				object = NULL;
			}

			lock_yield_check = FALSE;
			continue;
		}

		  
                                                                    
                                                                    
          
    
                                                                     
                                                                         
    
                                                                      
                                                                 
                                                    
     
		if (vm_page_queue_empty(&sq->age_q) && vm_page_speculative_count) {
			retval = vps_age_speculative_queue(force_speculative_aging);

			if (retval == VM_PAGEOUT_SCAN_NEXT_ITERATION) {
				lock_yield_check = FALSE;
				continue;
			}
		}
		force_speculative_aging = FALSE;

		  
                                                             
    
                                                         
                                                                 
                                                              
                             
     
		retval = vps_object_cache_evict(&object);

		if (retval == VM_PAGEOUT_SCAN_NEXT_ITERATION) {
			lock_yield_check = FALSE;
			continue;
		}


		  
                                                          
                   
     
		vps_calculate_filecache_min();

		  
                                                                      
                                                                                
     
		retval = vps_flow_control(&flow_control, &anons_grabbed, &object,
		    &delayed_unlock, &local_freeq, &local_freed,
		    &vm_pageout_deadlock_target, inactive_burst_count);

		if (retval == VM_PAGEOUT_SCAN_NEXT_ITERATION) {
			if (loop_count >= vm_page_inactive_count) {
				loop_count = 0;
			}

			inactive_burst_count = 0;

			assert(object == NULL);
			assert(delayed_unlock != 0);

			lock_yield_check = FALSE;
			continue;
		} else if (retval == VM_PAGEOUT_SCAN_DONE_RETURN) {
			goto return_from_scan;
		}

		flow_control.state = FCS_IDLE;

		vm_pageout_inactive_external_forced_reactivate_limit = MIN((vm_page_active_count + vm_page_inactive_count),
		    vm_pageout_inactive_external_forced_reactivate_limit);
		loop_count++;
		inactive_burst_count++;
		vm_pageout_state.vm_pageout_inactive++;

		  
                     
     

		m = NULL;
		retval = vps_choose_victim_page(&m, &anons_grabbed, &grab_anonymous, force_anonymous, &page_from_bg_q, &reactivated_this_call);

		if (m == NULL) {
			if (retval == VM_PAGEOUT_SCAN_NEXT_ITERATION) {
				inactive_burst_count = 0;

				if (page_prev_q_state == VM_PAGE_ON_INACTIVE_CLEANED_Q) {
					VM_PAGEOUT_DEBUG(vm_pageout_cleaned_reactivated, 1);
				}

				lock_yield_check = TRUE;
				continue;
			}

			  
                                                   
                                                             
                                                             
                                                         
                                                          
                                                     
      
			force_anonymous = FALSE;
			VM_PAGEOUT_DEBUG(vm_pageout_no_victim, 1);

			if (!vm_page_queue_empty(&sq->age_q)) {
				lock_yield_check = TRUE;
				continue;
			}

			if (vm_page_speculative_count) {
				force_speculative_aging = TRUE;
				lock_yield_check = TRUE;
				continue;
			}
			panic("vm_pageout: no victim");

			                
		}

		assert(VM_PAGE_PAGEABLE(m));
		m_object = VM_PAGE_OBJECT(m);
		force_anonymous = FALSE;

		page_prev_q_state = m->vmp_q_state;
		  
                                                    
                                                   
                                  
     
		vm_page_queues_remove(m, TRUE);

		assert(!m->vmp_laundry);
		assert(!m->vmp_private);
		assert(!m->vmp_fictitious);
		assert(m_object != kernel_object);
		assert(VM_PAGE_GET_PHYS_PAGE(m) != vm_page_guard_addr);

		vm_pageout_vminfo.vm_pageout_considered_page++;

		DTRACE_VM2(scan, int, 1, (uint64_t *), NULL);

		  
                                             
                                         
                         
     
		if (m_object != object) {
			boolean_t avoid_anon_pages = (grab_anonymous == FALSE || anons_grabbed >= ANONS_GRABBED_LIMIT);

			  
                                                                  
                                                                              
                                
      
			retval = vps_switch_object(m, m_object, &object, page_prev_q_state, avoid_anon_pages, page_from_bg_q);

			if (retval == VM_PAGEOUT_SCAN_NEXT_ITERATION) {
				lock_yield_check = TRUE;
				continue;
			}
		}
		assert(m_object == object);
		assert(VM_PAGE_OBJECT(m) == m_object);

		if (m->vmp_busy) {
			  
                                                 
                                          
     
      
			VM_PAGEOUT_DEBUG(vm_pageout_inactive_busy, 1);

			if (page_prev_q_state == VM_PAGE_ON_INACTIVE_CLEANED_Q) {
				VM_PAGEOUT_DEBUG(vm_pageout_cleaned_busy, 1);
			}

			vps_requeue_page(m, page_prev_q_state, page_from_bg_q);

			lock_yield_check = TRUE;
			continue;
		}

		  
                                                     
                                           
                                         
                                                       
                                    
    
                                                     
                                          
                                              
                                    
                                         
                                    
                                        
    
                                                    
                                       
                                                  
                                        
     
		if (m->vmp_free_when_done || m->vmp_cleaning) {
			lock_yield_check = TRUE;
			continue;
		}


		  
                                                               
                                                            
                                                           
                                                               
               
     
		if (m->vmp_absent || m->vmp_error || !object->alive) {
			if (m->vmp_absent) {
				VM_PAGEOUT_DEBUG(vm_pageout_inactive_absent, 1);
			} else if (!object->alive) {
				VM_PAGEOUT_DEBUG(vm_pageout_inactive_notalive, 1);
			} else {
				VM_PAGEOUT_DEBUG(vm_pageout_inactive_error, 1);
			}
reclaim_page:
			if (vm_pageout_deadlock_target) {
				VM_PAGEOUT_DEBUG(vm_pageout_scan_inactive_throttle_success, 1);
				vm_pageout_deadlock_target--;
			}

			DTRACE_VM2(dfree, int, 1, (uint64_t *), NULL);

			if (object->internal) {
				DTRACE_VM2(anonfree, int, 1, (uint64_t *), NULL);
			} else {
				DTRACE_VM2(fsfree, int, 1, (uint64_t *), NULL);
			}
			assert(!m->vmp_cleaning);
			assert(!m->vmp_laundry);

			if (!object->internal &&
			    object->pager != NULL &&
			    object->pager->mo_pager_ops == &shared_region_pager_ops) {
				shared_region_pager_reclaimed++;
			}

			m->vmp_busy = TRUE;

			  
                                                      
                                                          
                                                     
                                         
      
			if (m->vmp_tabled) {
				vm_page_remove(m, TRUE);
			}

			assert(m->vmp_pageq.next == 0 && m->vmp_pageq.prev == 0);
			m->vmp_snext = local_freeq;
			local_freeq = m;
			local_freed++;

			if (page_prev_q_state == VM_PAGE_ON_SPECULATIVE_Q) {
				vm_pageout_vminfo.vm_pageout_freed_speculative++;
			} else if (page_prev_q_state == VM_PAGE_ON_INACTIVE_CLEANED_Q) {
				vm_pageout_vminfo.vm_pageout_freed_cleaned++;
			} else if (page_prev_q_state == VM_PAGE_ON_INACTIVE_INTERNAL_Q) {
				vm_pageout_vminfo.vm_pageout_freed_internal++;
			} else {
				vm_pageout_vminfo.vm_pageout_freed_external++;
			}

			inactive_burst_count = 0;

			lock_yield_check = TRUE;
			continue;
		}
		if (object->copy == VM_OBJECT_NULL) {
			  
                                                     
                                                          
                              
                                                           
                                                   
      
			if (object->purgable == VM_PURGABLE_EMPTY) {
				if (m->vmp_pmapped == TRUE) {
					                    
					refmod_state = pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m));
					if (refmod_state & VM_MEM_MODIFIED) {
						SET_PAGE_DIRTY(m, FALSE);
					}
				}
				if (m->vmp_dirty || m->vmp_precious) {
					                                               
					vm_page_purged_count++;
				}
				goto reclaim_page;
			}

			if (VM_CONFIG_COMPRESSOR_IS_ACTIVE) {
				  
                                          
                                                
                                                   
                                                   
                                                  
                                               
       
			} else if (object->purgable == VM_PURGABLE_VOLATILE) {
				  
                                                   
                      
       

				                                                 
				assert(!VM_PAGE_WIRED(m));

				                            
				reactivated_this_call++;

				if (page_prev_q_state == VM_PAGE_ON_INACTIVE_CLEANED_Q) {
					VM_PAGEOUT_DEBUG(vm_pageout_cleaned_volatile_reactivated, 1);
				}

				goto reactivate_page;
			}
		}
		  
                                    
                                                  
                                               
                                           
     
		refmod_state = -1;

		if (m->vmp_reference == FALSE && m->vmp_pmapped == TRUE) {
			refmod_state = pmap_get_refmod(VM_PAGE_GET_PHYS_PAGE(m));

			if (refmod_state & VM_MEM_REFERENCED) {
				m->vmp_reference = TRUE;
			}
			if (refmod_state & VM_MEM_MODIFIED) {
				SET_PAGE_DIRTY(m, FALSE);
			}
		}

		if (m->vmp_reference || m->vmp_dirty) {
			                                       
			VM_PAGEOUT_SCAN_HANDLE_REUSABLE_PAGE(m, m_object);
		}

		if (vm_pageout_state.vm_page_xpmapped_min_divisor == 0) {
			vm_pageout_state.vm_page_xpmapped_min = 0;
		} else {
			vm_pageout_state.vm_page_xpmapped_min = (vm_page_external_count * 10) / vm_pageout_state.vm_page_xpmapped_min_divisor;
		}

		if (!m->vmp_no_cache &&
		    page_from_bg_q == FALSE &&
		    (m->vmp_reference || (m->vmp_xpmapped && !object->internal &&
		    (vm_page_xpmapped_external_count < vm_pageout_state.vm_page_xpmapped_min)))) {
			  
                                                  
                                                
                                                    
                                                   
                                               
                    
      
			if (++reactivated_this_call >= reactivate_limit) {
				vm_pageout_vminfo.vm_pageout_reactivation_limit_exceeded++;
			} else if (++inactive_reclaim_run >= VM_PAGEOUT_INACTIVE_FORCE_RECLAIM) {
				vm_pageout_vminfo.vm_pageout_inactive_force_reclaim++;
			} else {
				uint32_t isinuse;

				if (page_prev_q_state == VM_PAGE_ON_INACTIVE_CLEANED_Q) {
					VM_PAGEOUT_DEBUG(vm_pageout_cleaned_reference_reactivated, 1);
				}

				vm_pageout_vminfo.vm_pageout_inactive_referenced++;
reactivate_page:
				if (!object->internal && object->pager != MEMORY_OBJECT_NULL &&
				    vnode_pager_get_isinuse(object->pager, &isinuse) == KERN_SUCCESS && !isinuse) {
					  
                                                
                                            
        
					vm_page_deactivate(m);
					VM_PAGEOUT_DEBUG(vm_pageout_inactive_deactivated, 1);
				} else {
					  
                                                               
        
					vm_page_activate(m);
					counter_inc(&vm_statistics_reactivations);
					inactive_burst_count = 0;
				}
#if CONFIG_BACKGROUND_QUEUE
#if DEVELOPMENT || DEBUG
				if (page_from_bg_q == TRUE) {
					if (m_object->internal) {
						vm_pageout_rejected_bq_internal++;
					} else {
						vm_pageout_rejected_bq_external++;
					}
				}
#endif                           
#endif                              

				if (page_prev_q_state == VM_PAGE_ON_INACTIVE_CLEANED_Q) {
					VM_PAGEOUT_DEBUG(vm_pageout_cleaned_reactivated, 1);
				}
				vm_pageout_state.vm_pageout_inactive_used++;

				lock_yield_check = TRUE;
				continue;
			}
			  
                                               
                                                 
                    
      
			if ((refmod_state == -1) && !m->vmp_dirty && m->vmp_pmapped) {
				refmod_state = pmap_get_refmod(VM_PAGE_GET_PHYS_PAGE(m));
				if (refmod_state & VM_MEM_MODIFIED) {
					SET_PAGE_DIRTY(m, FALSE);
				}
			}
		}

		  
                                           
    
                                               
                                               
                                                 
                                              
                                            
                                       
                                      
    
                                         
                                          
                                                        
                                     
     

		inactive_throttled = FALSE;

		if (m->vmp_dirty || m->vmp_precious) {
			if (object->internal) {
				if (VM_PAGE_Q_THROTTLED(iq)) {
					inactive_throttled = TRUE;
				}
			} else if (VM_PAGE_Q_THROTTLED(eq)) {
				inactive_throttled = TRUE;
			}
		}
throttle_inactive:
		if (!VM_DYNAMIC_PAGING_ENABLED() &&
		    object->internal && m->vmp_dirty &&
		    (object->purgable == VM_PURGABLE_DENY ||
		    object->purgable == VM_PURGABLE_NONVOLATILE ||
		    object->purgable == VM_PURGABLE_VOLATILE)) {
			vm_page_check_pageable_safe(m);
			assert(m->vmp_q_state == VM_PAGE_NOT_ON_Q);
			vm_page_queue_enter(&vm_page_queue_throttled, m, vmp_pageq);
			m->vmp_q_state = VM_PAGE_ON_THROTTLED_Q;
			vm_page_throttled_count++;

			VM_PAGEOUT_DEBUG(vm_pageout_scan_reclaimed_throttled, 1);

			inactive_burst_count = 0;

			lock_yield_check = TRUE;
			continue;
		}
		if (inactive_throttled == TRUE) {
			vps_deal_with_throttled_queues(m, &object, &vm_pageout_inactive_external_forced_reactivate_limit,
			    &delayed_unlock, &force_anonymous, page_from_bg_q);

			inactive_burst_count = 0;

			if (page_prev_q_state == VM_PAGE_ON_INACTIVE_CLEANED_Q) {
				VM_PAGEOUT_DEBUG(vm_pageout_cleaned_reactivated, 1);
			}

			lock_yield_check = TRUE;
			continue;
		}

		  
                                          
                                         
                                          
    
                                               
                                                               
                                              
                                                    
                                                      
    
                                                         
                                                              
                                                                         
                                 
     
		if (m->vmp_pmapped == TRUE) {
			int pmap_options;

			  
                                                        
                               
                                       
                                                     
                                                           
                                                        
                       
                                                          
                                 
      
			if (!VM_CONFIG_COMPRESSOR_IS_ACTIVE ||
			    object->internal == FALSE) {
				pmap_options = 0;
			} else if (m->vmp_dirty || m->vmp_precious) {
				  
                                           
                                           
                         
                                             
                       
       
				pmap_options = PMAP_OPTIONS_COMPRESSOR;
			} else {
				  
                                            
                                                 
                                           
                                              
                                               
                
       
				pmap_options =
				    PMAP_OPTIONS_COMPRESSOR_IFF_MODIFIED;
			}
			refmod_state = pmap_disconnect_options(VM_PAGE_GET_PHYS_PAGE(m),
			    pmap_options,
			    NULL);
			if (refmod_state & VM_MEM_MODIFIED) {
				SET_PAGE_DIRTY(m, FALSE);
			}
		}

		  
                                                      
                                     
     
		inactive_reclaim_run = 0;

		  
                                                          
     
		if (!m->vmp_dirty && !m->vmp_precious) {
			vm_pageout_state.vm_pageout_inactive_clean++;

			  
                                                                  
      
#if CONFIG_PHANTOM_CACHE
			if (!object->internal) {
				vm_phantom_cache_add_ghost(m);
			}
#endif
			goto reclaim_page;
		}

		  
                                                        
                                                              
                                                      
                                                    
     
		if (object->internal) {
			if (VM_PAGE_Q_THROTTLED(iq)) {
				inactive_throttled = TRUE;
			}
		} else if (VM_PAGE_Q_THROTTLED(eq)) {
			inactive_throttled = TRUE;
		}

		if (inactive_throttled == TRUE) {
			goto throttle_inactive;
		}

#if VM_PRESSURE_EVENTS
#if CONFIG_JETSAM

		  
                                           
                                                
                                                       
                                                    
     

#else                    

		vm_pressure_response();

#endif                    
#endif                         

		if (page_prev_q_state == VM_PAGE_ON_SPECULATIVE_Q) {
			VM_PAGEOUT_DEBUG(vm_pageout_speculative_dirty, 1);
		}

		if (object->internal) {
			vm_pageout_vminfo.vm_pageout_inactive_dirty_internal++;
		} else {
			vm_pageout_vminfo.vm_pageout_inactive_dirty_external++;
		}

		  
                                                
                                                                  
                                                                     
                                       
     
		vm_pageout_cluster(m);
		inactive_burst_count = 0;

		  
                                     
     
	}
}


void
vm_page_free_reserve(
	int pages)
{
	int             free_after_reserve;

	if (VM_CONFIG_COMPRESSOR_IS_PRESENT) {
		if ((vm_page_free_reserved + pages + COMPRESSOR_FREE_RESERVED_LIMIT) >= (VM_PAGE_FREE_RESERVED_LIMIT + COMPRESSOR_FREE_RESERVED_LIMIT)) {
			vm_page_free_reserved = VM_PAGE_FREE_RESERVED_LIMIT + COMPRESSOR_FREE_RESERVED_LIMIT;
		} else {
			vm_page_free_reserved += (pages + COMPRESSOR_FREE_RESERVED_LIMIT);
		}
	} else {
		if ((vm_page_free_reserved + pages) >= VM_PAGE_FREE_RESERVED_LIMIT) {
			vm_page_free_reserved = VM_PAGE_FREE_RESERVED_LIMIT;
		} else {
			vm_page_free_reserved += pages;
		}
	}
	free_after_reserve = vm_pageout_state.vm_page_free_count_init - vm_page_free_reserved;

	vm_page_free_min = vm_page_free_reserved +
	    VM_PAGE_FREE_MIN(free_after_reserve);

	if (vm_page_free_min > VM_PAGE_FREE_MIN_LIMIT) {
		vm_page_free_min = VM_PAGE_FREE_MIN_LIMIT;
	}

	vm_page_free_target = vm_page_free_reserved +
	    VM_PAGE_FREE_TARGET(free_after_reserve);

	if (vm_page_free_target > VM_PAGE_FREE_TARGET_LIMIT) {
		vm_page_free_target = VM_PAGE_FREE_TARGET_LIMIT;
	}

	if (vm_page_free_target < vm_page_free_min + 5) {
		vm_page_free_target = vm_page_free_min + 5;
	}

	vm_page_throttle_limit = vm_page_free_target - (vm_page_free_target / 2);
}

  
                                               
   

void
vm_pageout_continue(void)
{
	DTRACE_VM2(pgrrun, int, 1, (uint64_t *), NULL);
	VM_PAGEOUT_DEBUG(vm_pageout_scan_event_counter, 1);

	lck_mtx_lock(&vm_page_queue_free_lock);
	vm_pageout_running = TRUE;
	lck_mtx_unlock(&vm_page_queue_free_lock);

	vm_pageout_scan();
	  
                                            
                                             
    
	assert(vm_page_free_wanted == 0);
	assert(vm_page_free_wanted_privileged == 0);
	assert_wait((event_t) &vm_page_free_wanted, THREAD_UNINT);

	vm_pageout_running = FALSE;
#if XNU_TARGET_OS_OSX
	if (vm_pageout_waiter) {
		vm_pageout_waiter = FALSE;
		thread_wakeup((event_t)&vm_pageout_waiter);
	}
#endif                        

	lck_mtx_unlock(&vm_page_queue_free_lock);
	vm_page_unlock_queues();

	thread_block((thread_continue_t)vm_pageout_continue);
	              
}

#if XNU_TARGET_OS_OSX
kern_return_t
vm_pageout_wait(uint64_t deadline)
{
	kern_return_t kr;

	lck_mtx_lock(&vm_page_queue_free_lock);
	for (kr = KERN_SUCCESS; vm_pageout_running && (KERN_SUCCESS == kr);) {
		vm_pageout_waiter = TRUE;
		if (THREAD_AWAKENED != lck_mtx_sleep_deadline(
			    &vm_page_queue_free_lock, LCK_SLEEP_DEFAULT,
			    (event_t) &vm_pageout_waiter, THREAD_UNINT, deadline)) {
			kr = KERN_OPERATION_TIMED_OUT;
		}
	}
	lck_mtx_unlock(&vm_page_queue_free_lock);

	return kr;
}
#endif                        


static void
vm_pageout_iothread_external_continue(struct vm_pageout_queue *q)
{
	vm_page_t       m = NULL;
	vm_object_t     object;
	vm_object_offset_t offset;
	memory_object_t pager;

	                                                                  
                                                                     
             
    
	if (vm_pageout_state.vm_pageout_internal_iothread != THREAD_NULL) {
		current_thread()->options &= ~TH_OPT_VMPRIV;
	}

	vm_page_lockspin_queues();

	while (!vm_page_queue_empty(&q->pgo_pending)) {
		q->pgo_busy = TRUE;
		vm_page_queue_remove_first(&q->pgo_pending, m, vmp_pageq);

		assert(m->vmp_q_state == VM_PAGE_ON_PAGEOUT_Q);
		VM_PAGE_CHECK(m);
		  
                                                  
                                                   
                                                   
                                                         
                                                        
                                                                
                                                       
     
		object = VM_PAGE_OBJECT(m);
		offset = m->vmp_offset;

		m->vmp_q_state = VM_PAGE_NOT_ON_Q;
		VM_PAGE_ZERO_PAGEQ_ENTRY(m);

		vm_page_unlock_queues();

		vm_object_lock(object);

		m = vm_page_lookup(object, offset);

		if (m == NULL || m->vmp_busy || m->vmp_cleaning ||
		    !m->vmp_laundry || (m->vmp_q_state != VM_PAGE_NOT_ON_Q)) {
			  
                                                     
                                                    
                                             
                                                
                                                      
                                                        
                                                       
      
			vm_object_activity_end(object);
			vm_object_unlock(object);

			vm_page_lockspin_queues();
			continue;
		}
		pager = object->pager;

		if (pager == MEMORY_OBJECT_NULL) {
			  
                                             
                                                     
                                             
      
			if (m->vmp_free_when_done) {
				  
                                               
                                           
                                                 
       
				VM_PAGE_FREE(m);
			} else {
				vm_page_lockspin_queues();

				vm_pageout_throttle_up(m);
				vm_page_activate(m);

				vm_page_unlock_queues();

				  
                               
       
			}
			vm_object_activity_end(object);
			vm_object_unlock(object);

			vm_page_lockspin_queues();
			continue;
		}
#if 0
		  
                                      
                                     
     
		VM_PAGE_CHECK(m);
#endif
		  
                                                    
                                                    
                                                     
                                                  
                                        
     
		vm_object_activity_end(object);
		vm_object_paging_begin(object);
		vm_object_unlock(object);

		  
                                
                                         
     
		memory_object_data_return(pager,
		    m->vmp_offset + object->paging_offset,
		    PAGE_SIZE,
		    NULL,
		    NULL,
		    FALSE,
		    FALSE,
		    0);

		vm_object_lock(object);
		vm_object_paging_end(object);
		vm_object_unlock(object);

		vm_pageout_io_throttle();

		vm_page_lockspin_queues();
	}
	q->pgo_busy = FALSE;
	q->pgo_idle = TRUE;

	assert_wait((event_t) &q->pgo_pending, THREAD_UNINT);
	vm_page_unlock_queues();

	thread_block_parameter((thread_continue_t)vm_pageout_iothread_external_continue, (void *) q);
	              
}


#define         MAX_FREE_BATCH          32
uint32_t vm_compressor_time_thread;                                            
                                                   
                                       


void
vm_pageout_iothread_internal_continue(struct cq *);
void
vm_pageout_iothread_internal_continue(struct cq *cq)
{
	struct vm_pageout_queue *q;
	vm_page_t       m = NULL;
	boolean_t       pgo_draining;
	vm_page_t   local_q;
	int         local_cnt;
	vm_page_t   local_freeq = NULL;
	int         local_freed = 0;
	int         local_batch_size;
#if DEVELOPMENT || DEBUG
	int       ncomps = 0;
	boolean_t marked_active = FALSE;
#endif
	KERNEL_DEBUG(0xe040000c | DBG_FUNC_END, 0, 0, 0, 0, 0);

	q = cq->q;
#if __AMP__
	if (vm_compressor_ebound && (vm_pageout_state.vm_compressor_thread_count > 1)) {
		local_batch_size = (q->pgo_maxlaundry >> 3);
		local_batch_size = MAX(local_batch_size, 16);
	} else {
		local_batch_size = q->pgo_maxlaundry / (vm_pageout_state.vm_compressor_thread_count * 2);
	}
#else
	local_batch_size = q->pgo_maxlaundry / (vm_pageout_state.vm_compressor_thread_count * 2);
#endif

#if RECORD_THE_COMPRESSED_DATA
	if (q->pgo_laundry) {
		c_compressed_record_init();
	}
#endif
	while (TRUE) {
		int     pages_left_on_q = 0;

		local_cnt = 0;
		local_q = NULL;

		KERNEL_DEBUG(0xe0400014 | DBG_FUNC_START, 0, 0, 0, 0, 0);

		vm_page_lock_queues();
#if DEVELOPMENT || DEBUG
		if (marked_active == FALSE) {
			vmct_active++;
			vmct_state[cq->id] = VMCT_ACTIVE;
			marked_active = TRUE;
			if (vmct_active == 1) {
				vm_compressor_epoch_start = mach_absolute_time();
			}
		}
#endif
		KERNEL_DEBUG(0xe0400014 | DBG_FUNC_END, 0, 0, 0, 0, 0);

		KERNEL_DEBUG(0xe0400018 | DBG_FUNC_START, q->pgo_laundry, 0, 0, 0, 0);

		while (!vm_page_queue_empty(&q->pgo_pending) && local_cnt < local_batch_size) {
			vm_page_queue_remove_first(&q->pgo_pending, m, vmp_pageq);
			assert(m->vmp_q_state == VM_PAGE_ON_PAGEOUT_Q);
			VM_PAGE_CHECK(m);

			m->vmp_q_state = VM_PAGE_NOT_ON_Q;
			VM_PAGE_ZERO_PAGEQ_ENTRY(m);
			m->vmp_laundry = FALSE;

			m->vmp_snext = local_q;
			local_q = m;
			local_cnt++;
		}
		if (local_q == NULL) {
			break;
		}

		q->pgo_busy = TRUE;

		if ((pgo_draining = q->pgo_draining) == FALSE) {
			vm_pageout_throttle_up_batch(q, local_cnt);
			pages_left_on_q = q->pgo_laundry;
		} else {
			pages_left_on_q = q->pgo_laundry - local_cnt;
		}

		vm_page_unlock_queues();

#if !RECORD_THE_COMPRESSED_DATA
		if (pages_left_on_q >= local_batch_size && cq->id < (vm_pageout_state.vm_compressor_thread_count - 1)) {
			thread_wakeup((event_t) ((uintptr_t)&q->pgo_pending + cq->id + 1));
		}
#endif
		KERNEL_DEBUG(0xe0400018 | DBG_FUNC_END, q->pgo_laundry, 0, 0, 0, 0);

		while (local_q) {
			KERNEL_DEBUG(0xe0400024 | DBG_FUNC_START, local_cnt, 0, 0, 0, 0);

			m = local_q;
			local_q = m->vmp_snext;
			m->vmp_snext = NULL;

			if (vm_pageout_compress_page(&cq->current_chead, cq->scratch_buf, m) == KERN_SUCCESS) {
#if DEVELOPMENT || DEBUG
				ncomps++;
#endif
				KERNEL_DEBUG(0xe0400024 | DBG_FUNC_END, local_cnt, 0, 0, 0, 0);

				m->vmp_snext = local_freeq;
				local_freeq = m;
				local_freed++;

				if (local_freed >= MAX_FREE_BATCH) {
					OSAddAtomic64(local_freed, &vm_pageout_vminfo.vm_pageout_compressions);

					vm_page_free_list(local_freeq, TRUE);

					local_freeq = NULL;
					local_freed = 0;
				}
			}
#if !CONFIG_JETSAM
			while (vm_page_free_count < COMPRESSOR_FREE_RESERVED_LIMIT) {
				kern_return_t   wait_result;
				int             need_wakeup = 0;

				if (local_freeq) {
					OSAddAtomic64(local_freed, &vm_pageout_vminfo.vm_pageout_compressions);

					vm_page_free_list(local_freeq, TRUE);
					local_freeq = NULL;
					local_freed = 0;

					continue;
				}
				lck_mtx_lock_spin(&vm_page_queue_free_lock);

				if (vm_page_free_count < COMPRESSOR_FREE_RESERVED_LIMIT) {
					if (vm_page_free_wanted_privileged++ == 0) {
						need_wakeup = 1;
					}
					wait_result = assert_wait((event_t)&vm_page_free_wanted_privileged, THREAD_UNINT);

					lck_mtx_unlock(&vm_page_queue_free_lock);

					if (need_wakeup) {
						thread_wakeup((event_t)&vm_page_free_wanted);
					}

					if (wait_result == THREAD_WAITING) {
						thread_block(THREAD_CONTINUE_NULL);
					}
				} else {
					lck_mtx_unlock(&vm_page_queue_free_lock);
				}
			}
#endif
		}
		if (local_freeq) {
			OSAddAtomic64(local_freed, &vm_pageout_vminfo.vm_pageout_compressions);

			vm_page_free_list(local_freeq, TRUE);
			local_freeq = NULL;
			local_freed = 0;
		}
		if (pgo_draining == TRUE) {
			vm_page_lockspin_queues();
			vm_pageout_throttle_up_batch(q, local_cnt);
			vm_page_unlock_queues();
		}
	}
	KERNEL_DEBUG(0xe040000c | DBG_FUNC_START, 0, 0, 0, 0, 0);

	  
                                         
    
	q->pgo_busy = FALSE;
	q->pgo_idle = TRUE;

	assert_wait((event_t) ((uintptr_t)&q->pgo_pending + cq->id), THREAD_UNINT);
#if DEVELOPMENT || DEBUG
	if (marked_active == TRUE) {
		vmct_active--;
		vmct_state[cq->id] = VMCT_IDLE;

		if (vmct_active == 0) {
			vm_compressor_epoch_stop = mach_absolute_time();
			assertf(vm_compressor_epoch_stop >= vm_compressor_epoch_start,
			    "Compressor epoch non-monotonic: 0x%llx -> 0x%llx",
			    vm_compressor_epoch_start, vm_compressor_epoch_stop);
			                                                     
                                        
      
			vmct_stats.vmct_cthreads_total += vm_compressor_epoch_stop - vm_compressor_epoch_start;
		}
	}
#endif
	vm_page_unlock_queues();
#if DEVELOPMENT || DEBUG
	if (__improbable(vm_compressor_time_thread)) {
		vmct_stats.vmct_runtimes[cq->id] = thread_get_runtime_self();
		vmct_stats.vmct_pages[cq->id] += ncomps;
		vmct_stats.vmct_iterations[cq->id]++;
		if (ncomps > vmct_stats.vmct_maxpages[cq->id]) {
			vmct_stats.vmct_maxpages[cq->id] = ncomps;
		}
		if (ncomps < vmct_stats.vmct_minpages[cq->id]) {
			vmct_stats.vmct_minpages[cq->id] = ncomps;
		}
	}
#endif

	KERNEL_DEBUG(0xe0400018 | DBG_FUNC_END, 0, 0, 0, 0, 0);

	thread_block_parameter((thread_continue_t)vm_pageout_iothread_internal_continue, (void *) cq);
	              
}


kern_return_t
vm_pageout_compress_page(void **current_chead, char *scratch_buf, vm_page_t m)
{
	vm_object_t     object;
	memory_object_t pager;
	int             compressed_count_delta;
	kern_return_t   retval;

	object = VM_PAGE_OBJECT(m);

	assert(!m->vmp_free_when_done);
	assert(!m->vmp_laundry);

	pager = object->pager;

	if (!object->pager_initialized || pager == MEMORY_OBJECT_NULL) {
		KERNEL_DEBUG(0xe0400010 | DBG_FUNC_START, object, pager, 0, 0, 0);

		vm_object_lock(object);

		  
                                                      
                                              
     

		if (!object->pager_initialized) {
			vm_object_collapse(object, (vm_object_offset_t) 0, TRUE);
		}
		if (!object->pager_initialized) {
			vm_object_compressor_pager_create(object);
		}

		pager = object->pager;

		if (!object->pager_initialized || pager == MEMORY_OBJECT_NULL) {
			  
                                    
                                      
                          
     
                                       
                       
      
			PAGE_WAKEUP_DONE(m);

			vm_page_lockspin_queues();
			vm_page_activate(m);
			VM_PAGEOUT_DEBUG(vm_pageout_dirty_no_pager, 1);
			vm_page_unlock_queues();

			  
                              
      
			vm_object_activity_end(object);
			vm_object_unlock(object);

			return KERN_FAILURE;
		}
		vm_object_unlock(object);

		KERNEL_DEBUG(0xe0400010 | DBG_FUNC_END, object, pager, 0, 0, 0);
	}
	assert(object->pager_initialized && pager != MEMORY_OBJECT_NULL);
	assert(object->activity_in_progress > 0);

	retval = vm_compressor_pager_put(
		pager,
		m->vmp_offset + object->paging_offset,
		VM_PAGE_GET_PHYS_PAGE(m),
		current_chead,
		scratch_buf,
		&compressed_count_delta);

	vm_object_lock(object);

	assert(object->activity_in_progress > 0);
	assert(VM_PAGE_OBJECT(m) == object);
	assert( !VM_PAGE_WIRED(m));

	vm_compressor_pager_count(pager,
	    compressed_count_delta,
	    FALSE,                                        
	    object);

	if (retval == KERN_SUCCESS) {
		  
                                            
                                         
                                        
                                                 
                               
     
		if ((object->purgable != VM_PURGABLE_DENY ||
		    object->vo_ledger_tag) &&
		    object->vo_owner != NULL) {
			                                               
			vm_object_owner_compressed_update(object,
			    +1);
		}
		counter_inc(&vm_statistics_compressions);

		if (m->vmp_tabled) {
			vm_page_remove(m, TRUE);
		}
	} else {
		PAGE_WAKEUP_DONE(m);

		vm_page_lockspin_queues();

		vm_page_activate(m);
		vm_pageout_vminfo.vm_compressor_failed++;

		vm_page_unlock_queues();
	}
	vm_object_activity_end(object);
	vm_object_unlock(object);

	return retval;
}


static void
vm_pageout_adjust_eq_iothrottle(struct vm_pageout_queue *eq, boolean_t req_lowpriority)
{
	uint32_t        policy;

	if (hibernate_cleaning_in_progress == TRUE) {
		req_lowpriority = FALSE;
	}

	if (eq->pgo_inited == TRUE && eq->pgo_lowpriority != req_lowpriority) {
		vm_page_unlock_queues();

		if (req_lowpriority == TRUE) {
			policy = THROTTLE_LEVEL_PAGEOUT_THROTTLED;
			DTRACE_VM(laundrythrottle);
		} else {
			policy = THROTTLE_LEVEL_PAGEOUT_UNTHROTTLED;
			DTRACE_VM(laundryunthrottle);
		}
		proc_set_thread_policy_with_tid(kernel_task, eq->pgo_tid,
		    TASK_POLICY_EXTERNAL, TASK_POLICY_IO, policy);

		vm_page_lock_queues();
		eq->pgo_lowpriority = req_lowpriority;
	}
}


static void
vm_pageout_iothread_external(void)
{
	thread_t        self = current_thread();

	self->options |= TH_OPT_VMPRIV;

	DTRACE_VM2(laundrythrottle, int, 1, (uint64_t *), NULL);

	proc_set_thread_policy(self, TASK_POLICY_EXTERNAL,
	    TASK_POLICY_IO, THROTTLE_LEVEL_PAGEOUT_THROTTLED);

	vm_page_lock_queues();

	vm_pageout_queue_external.pgo_tid = self->thread_id;
	vm_pageout_queue_external.pgo_lowpriority = TRUE;
	vm_pageout_queue_external.pgo_inited = TRUE;

	vm_page_unlock_queues();

	vm_pageout_iothread_external_continue(&vm_pageout_queue_external);

	              
}


static void
vm_pageout_iothread_internal(struct cq *cq)
{
	thread_t        self = current_thread();

	self->options |= TH_OPT_VMPRIV;

	vm_page_lock_queues();

	vm_pageout_queue_internal.pgo_tid = self->thread_id;
	vm_pageout_queue_internal.pgo_lowpriority = TRUE;
	vm_pageout_queue_internal.pgo_inited = TRUE;

	vm_page_unlock_queues();

	if (vm_pageout_state.vm_restricted_to_single_processor == TRUE) {
		thread_vm_bind_group_add();
	}

#if CONFIG_THREAD_GROUPS
	thread_group_vm_add();
#endif                           

#if __AMP__
	if (vm_compressor_ebound) {
		  
                                                                      
                                         
     
		thread_bind_cluster_type(self, 'E', true);
	}
#endif              

	thread_set_thread_name(current_thread(), "VM_compressor");
#if DEVELOPMENT || DEBUG
	vmct_stats.vmct_minpages[cq->id] = INT32_MAX;
#endif
	vm_pageout_iothread_internal_continue(cq);

	              
}

kern_return_t
vm_set_buffer_cleanup_callout(boolean_t (*func)(int))
{
	if (OSCompareAndSwapPtr(NULL, ptrauth_nop_cast(void *, func), (void * volatile *) &consider_buffer_cache_collect)) {
		return KERN_SUCCESS;
	} else {
		return KERN_FAILURE;                  
	}
}

extern boolean_t        memorystatus_manual_testing_on;
extern unsigned int     memorystatus_level;


#if VM_PRESSURE_EVENTS

boolean_t vm_pressure_events_enabled = FALSE;

void
vm_pressure_response(void)
{
	vm_pressure_level_t     old_level = kVMPressureNormal;
	int                     new_level = -1;
	unsigned int            total_pages;
	uint64_t                available_memory = 0;

	if (vm_pressure_events_enabled == FALSE) {
		return;
	}

#if !XNU_TARGET_OS_OSX

	available_memory = (uint64_t) memorystatus_available_pages;

#else                         

	available_memory = (uint64_t) AVAILABLE_NON_COMPRESSED_MEMORY;
	memorystatus_available_pages = (uint64_t) AVAILABLE_NON_COMPRESSED_MEMORY;

#endif                         

	total_pages = (unsigned int) atop_64(max_mem);
#if CONFIG_SECLUDED_MEMORY
	total_pages -= vm_page_secluded_count;
#endif                             
	memorystatus_level = (unsigned int) ((available_memory * 100) / total_pages);

	if (memorystatus_manual_testing_on) {
		return;
	}

	old_level = memorystatus_vm_pressure_level;

	switch (memorystatus_vm_pressure_level) {
	case kVMPressureNormal:
	{
		if (VM_PRESSURE_WARNING_TO_CRITICAL()) {
			new_level = kVMPressureCritical;
		} else if (VM_PRESSURE_NORMAL_TO_WARNING()) {
			new_level = kVMPressureWarning;
		}
		break;
	}

	case kVMPressureWarning:
	case kVMPressureUrgent:
	{
		if (VM_PRESSURE_WARNING_TO_NORMAL()) {
			new_level = kVMPressureNormal;
		} else if (VM_PRESSURE_WARNING_TO_CRITICAL()) {
			new_level = kVMPressureCritical;
		}
		break;
	}

	case kVMPressureCritical:
	{
		if (VM_PRESSURE_WARNING_TO_NORMAL()) {
			new_level = kVMPressureNormal;
		} else if (VM_PRESSURE_CRITICAL_TO_WARNING()) {
			new_level = kVMPressureWarning;
		}
		break;
	}

	default:
		return;
	}

	if (new_level != -1) {
		memorystatus_vm_pressure_level = (vm_pressure_level_t) new_level;

		if (new_level != (int) old_level) {
			VM_DEBUG_CONSTANT_EVENT(vm_pressure_level_change, VM_PRESSURE_LEVEL_CHANGE, DBG_FUNC_NONE,
			    new_level, old_level, 0, 0);
		}

		if ((memorystatus_vm_pressure_level != kVMPressureNormal) || (old_level != memorystatus_vm_pressure_level)) {
			if (vm_pageout_state.vm_pressure_thread_running == FALSE) {
				thread_wakeup(&vm_pressure_thread);
			}

			if (old_level != memorystatus_vm_pressure_level) {
				thread_wakeup(&vm_pageout_state.vm_pressure_changed);
			}
		}
	}
}
#endif                         

  
                                                                                 
                                                         
   
kern_return_t
mach_vm_pressure_level_monitor(__unused boolean_t wait_for_pressure, __unused unsigned int *pressure_level)
{
#if !VM_PRESSURE_EVENTS

	return KERN_FAILURE;

#else                         

	wait_result_t       wr = 0;
	vm_pressure_level_t old_level = memorystatus_vm_pressure_level;

	if (pressure_level == NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if (*pressure_level == kVMPressureJetsam) {
		if (!wait_for_pressure) {
			return KERN_INVALID_ARGUMENT;
		}

		lck_mtx_lock(&memorystatus_jetsam_fg_band_lock);
		wr = assert_wait((event_t)&memorystatus_jetsam_fg_band_waiters,
		    THREAD_INTERRUPTIBLE);
		if (wr == THREAD_WAITING) {
			++memorystatus_jetsam_fg_band_waiters;
			lck_mtx_unlock(&memorystatus_jetsam_fg_band_lock);
			wr = thread_block(THREAD_CONTINUE_NULL);
		} else {
			lck_mtx_unlock(&memorystatus_jetsam_fg_band_lock);
		}
		if (wr != THREAD_AWAKENED) {
			return KERN_ABORTED;
		}
		*pressure_level = kVMPressureJetsam;
		return KERN_SUCCESS;
	}

	if (wait_for_pressure == TRUE) {
		while (old_level == *pressure_level) {
			wr = assert_wait((event_t) &vm_pageout_state.vm_pressure_changed,
			    THREAD_INTERRUPTIBLE);
			if (wr == THREAD_WAITING) {
				wr = thread_block(THREAD_CONTINUE_NULL);
			}
			if (wr == THREAD_INTERRUPTED) {
				return KERN_ABORTED;
			}

			if (wr == THREAD_AWAKENED) {
				old_level = memorystatus_vm_pressure_level;
			}
		}
	}

	*pressure_level = old_level;
	return KERN_SUCCESS;
#endif                         
}

#if VM_PRESSURE_EVENTS
void
vm_pressure_thread(void)
{
	static boolean_t thread_initialized = FALSE;

	if (thread_initialized == TRUE) {
		vm_pageout_state.vm_pressure_thread_running = TRUE;
		consider_vm_pressure_events();
		vm_pageout_state.vm_pressure_thread_running = FALSE;
	}

	thread_set_thread_name(current_thread(), "VM_pressure");
	thread_initialized = TRUE;
	assert_wait((event_t) &vm_pressure_thread, THREAD_UNINT);
	thread_block((thread_continue_t)vm_pressure_thread);
}
#endif                         


  
                                                
   
void
compute_pageout_gc_throttle(__unused void *arg)
{
	if (vm_pageout_vminfo.vm_pageout_considered_page != vm_pageout_state.vm_pageout_considered_page_last) {
		vm_pageout_state.vm_pageout_considered_page_last = vm_pageout_vminfo.vm_pageout_considered_page;

		thread_wakeup((event_t) &vm_pageout_garbage_collect);
	}
}

  
                                                                              
                                                                                
                                                                              
                                      
  
                                                                             
  
                                                                               
                                                                             
                                                                              
                                                                              
                                                                               
                                                                     
  
                                                                             
                                                                           
                                                                             
        
   

void
vm_pageout_garbage_collect(int collect)
{
	if (collect) {
		if (zone_map_nearing_exhaustion()) {
			  
                                                                     
     
                                                        
                                                                      
                                                                     
                                                                    
                                                                   
                                                                 
                            
     
                                                                
                                                                     
                                                                      
                                                                      
               
      
			zone_gc(ZONE_GC_JETSAM);
		} else {
			                                                                 
			boolean_t buf_large_zfree = FALSE;
			boolean_t first_try = TRUE;

			stack_collect();

			consider_machine_collect();
			mbuf_drain(FALSE);

			do {
				if (consider_buffer_cache_collect != NULL) {
					buf_large_zfree = (*consider_buffer_cache_collect)(0);
				}
				if (first_try == TRUE || buf_large_zfree == TRUE) {
					  
                                                            
                                     
        
					zone_gc(ZONE_GC_TRIM);
				}
				first_try = FALSE;
			} while (buf_large_zfree == TRUE && vm_page_free_count < vm_page_free_target);

			consider_machine_adjust();
		}
	}

	assert_wait((event_t) &vm_pageout_garbage_collect, THREAD_UNINT);

	thread_block_parameter((thread_continue_t) vm_pageout_garbage_collect, (void *)1);
	              
}


#if VM_PAGE_BUCKETS_CHECK
#if VM_PAGE_FAKE_BUCKETS
extern vm_map_offset_t vm_page_fake_buckets_start, vm_page_fake_buckets_end;
#endif                           
#endif                            



void
vm_set_restrictions(unsigned int num_cpus)
{
	int vm_restricted_to_single_processor = 0;

	if (PE_parse_boot_argn("vm_restricted_to_single_processor", &vm_restricted_to_single_processor, sizeof(vm_restricted_to_single_processor))) {
		kprintf("Overriding vm_restricted_to_single_processor to %d\n", vm_restricted_to_single_processor);
		vm_pageout_state.vm_restricted_to_single_processor = (vm_restricted_to_single_processor ? TRUE : FALSE);
	} else {
		assert(num_cpus > 0);

		if (num_cpus <= 3) {
			  
                                                        
                                                               
                                                                         
                                                                         
                                                                             
                                       
                                                                  
                                                                      
                                    
     
                                                                          
                                                                      
                                                                         
                                   
      
			vm_pageout_state.vm_restricted_to_single_processor = TRUE;
		} else {
			vm_pageout_state.vm_restricted_to_single_processor = FALSE;
		}
	}
}

void
vm_pageout(void)
{
	thread_t        self = current_thread();
	thread_t        thread;
	kern_return_t   result;
	spl_t           s;

	  
                          
    
	s = splsched();

	vm_pageout_scan_thread = self;

#if CONFIG_VPS_DYNAMIC_PRIO

	int             vps_dynprio_bootarg = 0;

	if (PE_parse_boot_argn("vps_dynamic_priority_enabled", &vps_dynprio_bootarg, sizeof(vps_dynprio_bootarg))) {
		vps_dynamic_priority_enabled = (vps_dynprio_bootarg ? TRUE : FALSE);
		kprintf("Overriding vps_dynamic_priority_enabled to %d\n", vps_dynamic_priority_enabled);
	} else {
		if (vm_pageout_state.vm_restricted_to_single_processor == TRUE) {
			vps_dynamic_priority_enabled = TRUE;
		} else {
			vps_dynamic_priority_enabled = FALSE;
		}
	}

	if (vps_dynamic_priority_enabled) {
		sched_set_kernel_thread_priority(self, MAXPRI_THROTTLE);
		thread_set_eager_preempt(self);
	} else {
		sched_set_kernel_thread_priority(self, BASEPRI_VM);
	}

#else                              

	vps_dynamic_priority_enabled = FALSE;
	sched_set_kernel_thread_priority(self, BASEPRI_VM);

#endif                              

	thread_lock(self);
	self->options |= TH_OPT_VMPRIV;
	thread_unlock(self);

	if (!self->reserved_stack) {
		self->reserved_stack = self->kernel_stack;
	}

	if (vm_pageout_state.vm_restricted_to_single_processor == TRUE &&
	    vps_dynamic_priority_enabled == FALSE) {
		thread_vm_bind_group_add();
	}


#if CONFIG_THREAD_GROUPS
	thread_group_vm_add();
#endif                           

#if __AMP__
	PE_parse_boot_argn("vmpgo_pcluster", &vm_pgo_pbound, sizeof(vm_pgo_pbound));
	if (vm_pgo_pbound) {
		  
                                                                   
                                         
     
		thread_bind_cluster_type(self, 'P', true);
	}
#endif              

	splx(s);

	thread_set_thread_name(current_thread(), "VM_pageout_scan");

	  
                                      
    

	vm_pageout_state.vm_pressure_thread_running = FALSE;
	vm_pageout_state.vm_pressure_changed = FALSE;
	vm_pageout_state.memorystatus_purge_on_warning = 2;
	vm_pageout_state.memorystatus_purge_on_urgent = 5;
	vm_pageout_state.memorystatus_purge_on_critical = 8;
	vm_pageout_state.vm_page_speculative_q_age_ms = VM_PAGE_SPECULATIVE_Q_AGE_MS;
	vm_pageout_state.vm_page_speculative_percentage = 5;
	vm_pageout_state.vm_page_speculative_target = 0;

	vm_pageout_state.vm_pageout_external_iothread = THREAD_NULL;
	vm_pageout_state.vm_pageout_internal_iothread = THREAD_NULL;

	vm_pageout_state.vm_pageout_swap_wait = 0;
	vm_pageout_state.vm_pageout_idle_wait = 0;
	vm_pageout_state.vm_pageout_empty_wait = 0;
	vm_pageout_state.vm_pageout_burst_wait = 0;
	vm_pageout_state.vm_pageout_deadlock_wait = 0;
	vm_pageout_state.vm_pageout_deadlock_relief = 0;
	vm_pageout_state.vm_pageout_burst_inactive_throttle = 0;

	vm_pageout_state.vm_pageout_inactive = 0;
	vm_pageout_state.vm_pageout_inactive_used = 0;
	vm_pageout_state.vm_pageout_inactive_clean = 0;

	vm_pageout_state.vm_memory_pressure = 0;
	vm_pageout_state.vm_page_filecache_min = 0;
#if CONFIG_JETSAM
	vm_pageout_state.vm_page_filecache_min_divisor = 70;
	vm_pageout_state.vm_page_xpmapped_min_divisor = 40;
#else
	vm_pageout_state.vm_page_filecache_min_divisor = 27;
	vm_pageout_state.vm_page_xpmapped_min_divisor = 36;
#endif
	vm_pageout_state.vm_page_free_count_init = vm_page_free_count;

	vm_pageout_state.vm_pageout_considered_page_last = 0;

	if (vm_pageout_state.vm_pageout_swap_wait == 0) {
		vm_pageout_state.vm_pageout_swap_wait = VM_PAGEOUT_SWAP_WAIT;
	}

	if (vm_pageout_state.vm_pageout_idle_wait == 0) {
		vm_pageout_state.vm_pageout_idle_wait = VM_PAGEOUT_IDLE_WAIT;
	}

	if (vm_pageout_state.vm_pageout_burst_wait == 0) {
		vm_pageout_state.vm_pageout_burst_wait = VM_PAGEOUT_BURST_WAIT;
	}

	if (vm_pageout_state.vm_pageout_empty_wait == 0) {
		vm_pageout_state.vm_pageout_empty_wait = VM_PAGEOUT_EMPTY_WAIT;
	}

	if (vm_pageout_state.vm_pageout_deadlock_wait == 0) {
		vm_pageout_state.vm_pageout_deadlock_wait = VM_PAGEOUT_DEADLOCK_WAIT;
	}

	if (vm_pageout_state.vm_pageout_deadlock_relief == 0) {
		vm_pageout_state.vm_pageout_deadlock_relief = VM_PAGEOUT_DEADLOCK_RELIEF;
	}

	if (vm_pageout_state.vm_pageout_burst_inactive_throttle == 0) {
		vm_pageout_state.vm_pageout_burst_inactive_throttle = VM_PAGEOUT_BURST_INACTIVE_THROTTLE;
	}
	  
                                                     
                                                     
                                                           
                                                           
                                                  
    
	if (vm_page_free_reserved < VM_PAGE_FREE_RESERVED(processor_count)) {
		vm_page_free_reserve((VM_PAGE_FREE_RESERVED(processor_count)) - vm_page_free_reserved);
	} else {
		vm_page_free_reserve(0);
	}


	vm_page_queue_init(&vm_pageout_queue_external.pgo_pending);
	vm_pageout_queue_external.pgo_maxlaundry = VM_PAGE_LAUNDRY_MAX;
	vm_pageout_queue_external.pgo_laundry = 0;
	vm_pageout_queue_external.pgo_idle = FALSE;
	vm_pageout_queue_external.pgo_busy = FALSE;
	vm_pageout_queue_external.pgo_throttled = FALSE;
	vm_pageout_queue_external.pgo_draining = FALSE;
	vm_pageout_queue_external.pgo_lowpriority = FALSE;
	vm_pageout_queue_external.pgo_tid = -1;
	vm_pageout_queue_external.pgo_inited = FALSE;

	vm_page_queue_init(&vm_pageout_queue_internal.pgo_pending);
	vm_pageout_queue_internal.pgo_maxlaundry = 0;
	vm_pageout_queue_internal.pgo_laundry = 0;
	vm_pageout_queue_internal.pgo_idle = FALSE;
	vm_pageout_queue_internal.pgo_busy = FALSE;
	vm_pageout_queue_internal.pgo_throttled = FALSE;
	vm_pageout_queue_internal.pgo_draining = FALSE;
	vm_pageout_queue_internal.pgo_lowpriority = FALSE;
	vm_pageout_queue_internal.pgo_tid = -1;
	vm_pageout_queue_internal.pgo_inited = FALSE;

	                                                                              
	                                                                  

	result = kernel_thread_start_priority((thread_continue_t)vm_pageout_iothread_external, NULL,
	    BASEPRI_VM,
	    &vm_pageout_state.vm_pageout_external_iothread);
	if (result != KERN_SUCCESS) {
		panic("vm_pageout_iothread_external: create failed");
	}
	thread_set_thread_name(vm_pageout_state.vm_pageout_external_iothread, "VM_pageout_external_iothread");
	thread_deallocate(vm_pageout_state.vm_pageout_external_iothread);

	result = kernel_thread_create((thread_continue_t)vm_pageout_garbage_collect, NULL,
	    BASEPRI_DEFAULT,
	    &thread);
	if (result != KERN_SUCCESS) {
		panic("vm_pageout_garbage_collect: create failed");
	}
	thread_set_thread_name(thread, "VM_pageout_garbage_collect");
	if (thread->reserved_stack == 0) {
		assert(thread->kernel_stack);
		thread->reserved_stack = thread->kernel_stack;
	}

	thread_mtx_lock(thread);
	thread_start(thread);
	thread_mtx_unlock(thread);

	thread_deallocate(thread);

#if VM_PRESSURE_EVENTS
	result = kernel_thread_start_priority((thread_continue_t)vm_pressure_thread, NULL,
	    BASEPRI_DEFAULT,
	    &thread);

	if (result != KERN_SUCCESS) {
		panic("vm_pressure_thread: create failed");
	}

	thread_deallocate(thread);
#endif

	vm_object_reaper_init();


	bzero(&vm_config, sizeof(vm_config));

	switch (vm_compressor_mode) {
	case VM_PAGER_DEFAULT:
		printf("mapping deprecated VM_PAGER_DEFAULT to VM_PAGER_COMPRESSOR_WITH_SWAP\n");
		OS_FALLTHROUGH;

	case VM_PAGER_COMPRESSOR_WITH_SWAP:
		vm_config.compressor_is_present = TRUE;
		vm_config.swap_is_present = TRUE;
		vm_config.compressor_is_active = TRUE;
		vm_config.swap_is_active = TRUE;
		break;

	case VM_PAGER_COMPRESSOR_NO_SWAP:
		vm_config.compressor_is_present = TRUE;
		vm_config.swap_is_present = TRUE;
		vm_config.compressor_is_active = TRUE;
		break;

	case VM_PAGER_FREEZER_DEFAULT:
		printf("mapping deprecated VM_PAGER_FREEZER_DEFAULT to VM_PAGER_FREEZER_COMPRESSOR_NO_SWAP\n");
		OS_FALLTHROUGH;

	case VM_PAGER_FREEZER_COMPRESSOR_NO_SWAP:
		vm_config.compressor_is_present = TRUE;
		vm_config.swap_is_present = TRUE;
		break;

	case VM_PAGER_COMPRESSOR_NO_SWAP_PLUS_FREEZER_COMPRESSOR_WITH_SWAP:
		vm_config.compressor_is_present = TRUE;
		vm_config.swap_is_present = TRUE;
		vm_config.compressor_is_active = TRUE;
		vm_config.freezer_swap_is_active = TRUE;
		break;

	case VM_PAGER_NOT_CONFIGURED:
		break;

	default:
		printf("unknown compressor mode - %x\n", vm_compressor_mode);
		break;
	}
	if (VM_CONFIG_COMPRESSOR_IS_PRESENT) {
		vm_compressor_pager_init();
	}

#if VM_PRESSURE_EVENTS
	vm_pressure_events_enabled = TRUE;
#endif                         

#if CONFIG_PHANTOM_CACHE
	vm_phantom_cache_init();
#endif
#if VM_PAGE_BUCKETS_CHECK
#if VM_PAGE_FAKE_BUCKETS
	printf("**** DEBUG: protecting fake buckets [0x%llx:0x%llx]\n",
	    (uint64_t) vm_page_fake_buckets_start,
	    (uint64_t) vm_page_fake_buckets_end);
	pmap_protect(kernel_pmap,
	    vm_page_fake_buckets_start,
	    vm_page_fake_buckets_end,
	    VM_PROT_READ);
                                                           
#endif                           
#endif                            

#if VM_OBJECT_TRACKING
	vm_object_tracking_init();
#endif                         

	vm_pageout_continue();

	  
                   
   
                                                                                  
                                                                                   
                                                                                   
                                                                                    
                                                                                    
                                                                                    
                                                                                    
                                                                                      
                                                                                        
    

	DTRACE_VM2(execfree, int, 1, (uint64_t *), NULL);
	DTRACE_VM2(execpgin, int, 1, (uint64_t *), NULL);
	DTRACE_VM2(execpgout, int, 1, (uint64_t *), NULL);
	DTRACE_VM2(pgswapin, int, 1, (uint64_t *), NULL);
	DTRACE_VM2(pgswapout, int, 1, (uint64_t *), NULL);
	DTRACE_VM2(swapin, int, 1, (uint64_t *), NULL);
	DTRACE_VM2(swapout, int, 1, (uint64_t *), NULL);
	              
}



kern_return_t
vm_pageout_internal_start(void)
{
	kern_return_t   result;
	host_basic_info_data_t hinfo;
	vm_offset_t     buf, bufsize;

	assert(VM_CONFIG_COMPRESSOR_IS_PRESENT);

	mach_msg_type_number_t count = HOST_BASIC_INFO_COUNT;
#define BSD_HOST 1
	host_info((host_t)BSD_HOST, HOST_BASIC_INFO, (host_info_t)&hinfo, &count);

	assert(hinfo.max_cpus > 0);

#if !XNU_TARGET_OS_OSX
	vm_pageout_state.vm_compressor_thread_count = 1;
#else                         
	if (hinfo.max_cpus > 4) {
		vm_pageout_state.vm_compressor_thread_count = 2;
	} else {
		vm_pageout_state.vm_compressor_thread_count = 1;
	}
#endif                         
	PE_parse_boot_argn("vmcomp_threads", &vm_pageout_state.vm_compressor_thread_count,
	    sizeof(vm_pageout_state.vm_compressor_thread_count));

#if     __AMP__
	PE_parse_boot_argn("vmcomp_ecluster", &vm_compressor_ebound, sizeof(vm_compressor_ebound));
	if (vm_compressor_ebound) {
		vm_pageout_state.vm_compressor_thread_count = 2;
	}
#endif
	if (vm_pageout_state.vm_compressor_thread_count >= hinfo.max_cpus) {
		vm_pageout_state.vm_compressor_thread_count = hinfo.max_cpus - 1;
	}
	if (vm_pageout_state.vm_compressor_thread_count <= 0) {
		vm_pageout_state.vm_compressor_thread_count = 1;
	} else if (vm_pageout_state.vm_compressor_thread_count > MAX_COMPRESSOR_THREAD_COUNT) {
		vm_pageout_state.vm_compressor_thread_count = MAX_COMPRESSOR_THREAD_COUNT;
	}

	vm_pageout_queue_internal.pgo_maxlaundry =
	    (vm_pageout_state.vm_compressor_thread_count * 4) * VM_PAGE_LAUNDRY_MAX;

	PE_parse_boot_argn("vmpgoi_maxlaundry",
	    &vm_pageout_queue_internal.pgo_maxlaundry,
	    sizeof(vm_pageout_queue_internal.pgo_maxlaundry));

	bufsize = COMPRESSOR_SCRATCH_BUF_SIZE;
	if (kernel_memory_allocate(kernel_map, &buf,
	    bufsize * vm_pageout_state.vm_compressor_thread_count,
	    0, KMA_KOBJECT | KMA_PERMANENT, VM_KERN_MEMORY_COMPRESSOR)) {
		panic("vm_pageout_internal_start: Unable to allocate %zd bytes",
		    (size_t)(bufsize * vm_pageout_state.vm_compressor_thread_count));
	}

	for (int i = 0; i < vm_pageout_state.vm_compressor_thread_count; i++) {
		ciq[i].id = i;
		ciq[i].q = &vm_pageout_queue_internal;
		ciq[i].current_chead = NULL;
		ciq[i].scratch_buf = (char *)(buf + i * bufsize);

		result = kernel_thread_start_priority((thread_continue_t)vm_pageout_iothread_internal,
		    (void *)&ciq[i], BASEPRI_VM,
		    &vm_pageout_state.vm_pageout_internal_iothread);

		if (result == KERN_SUCCESS) {
			thread_deallocate(vm_pageout_state.vm_pageout_internal_iothread);
		} else {
			break;
		}
	}
	return result;
}

#if CONFIG_IOSCHED
  
                                                                                    
                                                                                        
                                                                                              
                                                                                               
                                                                                       
                                                                                          
                                                                                             
                                                                                            
                                               
   


static void
upl_set_decmp_info(upl_t upl, upl_t src_upl)
{
	assert((src_upl->flags & UPL_DECMP_REQ) != 0);

	upl_lock(src_upl);
	if (src_upl->decmp_io_upl) {
		  
                                                                    
                                                                      
                                                                         
               
     
		upl_unlock(src_upl);
		return;
	}
	src_upl->decmp_io_upl = (void *)upl;
	src_upl->ref_count++;

	upl->flags |= UPL_DECMP_REAL_IO;
	upl->decmp_io_upl = (void *)src_upl;
	upl_unlock(src_upl);
}
#endif                     

#if UPL_DEBUG
int     upl_debug_enabled = 1;
#else
int     upl_debug_enabled = 0;
#endif

static upl_t
upl_create(int type, int flags, upl_size_t size)
{
	upl_t   upl;
	vm_size_t       page_field_size = 0;
	int     upl_flags = 0;
	vm_size_t       upl_size  = sizeof(struct upl);

	assert(page_aligned(size));

	size = round_page_32(size);

	if (type & UPL_CREATE_LITE) {
		page_field_size = (atop(size) + 7) >> 3;
		page_field_size = (page_field_size + 3) & 0xFFFFFFFC;

		upl_flags |= UPL_LITE;
	}
	if (type & UPL_CREATE_INTERNAL) {
		upl_size += sizeof(struct upl_page_info) * atop(size);

		upl_flags |= UPL_INTERNAL;
	}
	upl = (upl_t)kalloc(upl_size + page_field_size);

	if (page_field_size) {
		bzero((char *)upl + upl_size, page_field_size);
	}

	upl->flags = upl_flags | flags;
	upl->kaddr = (vm_offset_t)0;
	upl->u_offset = 0;
	upl->u_size = 0;
	upl->map_object = NULL;
	upl->ref_count = 1;
	upl->ext_ref_count = 0;
	upl->highest_page = 0;
	upl_lock_init(upl);
	upl->vector_upl = NULL;
	upl->associated_upl = NULL;
	upl->upl_iodone = NULL;
#if CONFIG_IOSCHED
	if (type & UPL_CREATE_IO_TRACKING) {
		upl->upl_priority = proc_get_effective_thread_policy(current_thread(), TASK_POLICY_IO);
	}

	upl->upl_reprio_info = 0;
	upl->decmp_io_upl = 0;
	if ((type & UPL_CREATE_INTERNAL) && (type & UPL_CREATE_EXPEDITE_SUP)) {
		                                            
		thread_t        curthread = current_thread();
		upl->upl_reprio_info = (uint64_t *)kalloc(sizeof(uint64_t) * atop(size));
		bzero(upl->upl_reprio_info, (sizeof(uint64_t) * atop(size)));
		upl->flags |= UPL_EXPEDITE_SUPPORTED;
		if (curthread->decmp_upl != NULL) {
			upl_set_decmp_info(upl, curthread->decmp_upl);
		}
	}
#endif
#if CONFIG_IOSCHED || UPL_DEBUG
	if ((type & UPL_CREATE_IO_TRACKING) || upl_debug_enabled) {
		upl->upl_creator = current_thread();
		upl->uplq.next = 0;
		upl->uplq.prev = 0;
		upl->flags |= UPL_TRACKED_BY_OBJECT;
	}
#endif

#if UPL_DEBUG
	upl->ubc_alias1 = 0;
	upl->ubc_alias2 = 0;

	upl->upl_state = 0;
	upl->upl_commit_index = 0;
	bzero(&upl->upl_commit_records[0], sizeof(upl->upl_commit_records));

	(void) OSBacktrace(&upl->upl_create_retaddr[0], UPL_DEBUG_STACK_FRAMES);
#endif                

	return upl;
}

static void
upl_destroy(upl_t upl)
{
	int     page_field_size;                                  
	int     size;

                                                                                                                                           

	if (upl->ext_ref_count) {
		panic("upl(%p) ext_ref_count", upl);
	}

#if CONFIG_IOSCHED
	if ((upl->flags & UPL_DECMP_REAL_IO) && upl->decmp_io_upl) {
		upl_t src_upl;
		src_upl = upl->decmp_io_upl;
		assert((src_upl->flags & UPL_DECMP_REQ) != 0);
		upl_lock(src_upl);
		src_upl->decmp_io_upl = NULL;
		upl_unlock(src_upl);
		upl_deallocate(src_upl);
	}
#endif                     

#if CONFIG_IOSCHED || UPL_DEBUG
	if (((upl->flags & UPL_TRACKED_BY_OBJECT) || upl_debug_enabled) &&
	    !(upl->flags & UPL_VECTOR)) {
		vm_object_t     object;

		if (upl->flags & UPL_SHADOWED) {
			object = upl->map_object->shadow;
		} else {
			object = upl->map_object;
		}

		vm_object_lock(object);
		queue_remove(&object->uplq, upl, upl_t, uplq);
		vm_object_activity_end(object);
		vm_object_collapse(object, 0, TRUE);
		vm_object_unlock(object);
	}
#endif
	  
                                                 
                                    
    
	if (upl->flags & UPL_SHADOWED) {
		vm_object_deallocate(upl->map_object);
	}

	if (upl->flags & UPL_DEVICE_MEMORY) {
		size = PAGE_SIZE;
	} else {
		size = upl_adjusted_size(upl, PAGE_MASK);
	}
	page_field_size = 0;

	if (upl->flags & UPL_LITE) {
		page_field_size = ((size / PAGE_SIZE) + 7) >> 3;
		page_field_size = (page_field_size + 3) & 0xFFFFFFFC;
	}
	upl_lock_destroy(upl);
	upl->vector_upl = (vector_upl_t) 0xfeedbeef;

#if CONFIG_IOSCHED
	if (upl->flags & UPL_EXPEDITE_SUPPORTED) {
		kfree(upl->upl_reprio_info, sizeof(uint64_t) * (size / PAGE_SIZE));
	}
#endif

	if (upl->flags & UPL_INTERNAL) {
		kfree(upl,
		    sizeof(struct upl) +
		    (sizeof(struct upl_page_info) * (size / PAGE_SIZE))
		    + page_field_size);
	} else {
		kfree(upl, sizeof(struct upl) + page_field_size);
	}
}

void
upl_deallocate(upl_t upl)
{
	upl_lock(upl);

	if (--upl->ref_count == 0) {
		if (vector_upl_is_valid(upl)) {
			vector_upl_deallocate(upl);
		}
		upl_unlock(upl);

		if (upl->upl_iodone) {
			upl_callout_iodone(upl);
		}

		upl_destroy(upl);
	} else {
		upl_unlock(upl);
	}
}

#if CONFIG_IOSCHED
void
upl_mark_decmp(upl_t upl)
{
	if (upl->flags & UPL_TRACKED_BY_OBJECT) {
		upl->flags |= UPL_DECMP_REQ;
		upl->upl_creator->decmp_upl = (void *)upl;
	}
}

void
upl_unmark_decmp(upl_t upl)
{
	if (upl && (upl->flags & UPL_DECMP_REQ)) {
		upl->upl_creator->decmp_upl = NULL;
	}
}

#endif                     

#define VM_PAGE_Q_BACKING_UP(q)         \
	((q)->pgo_laundry >= (((q)->pgo_maxlaundry * 8) / 10))

boolean_t must_throttle_writes(void);

boolean_t
must_throttle_writes()
{
	if (VM_PAGE_Q_BACKING_UP(&vm_pageout_queue_external) &&
	    vm_page_pageable_external_count > (AVAILABLE_NON_COMPRESSED_MEMORY * 6) / 10) {
		return TRUE;
	}

	return FALSE;
}

#define MIN_DELAYED_WORK_CTX_ALLOCATED  (16)
#define MAX_DELAYED_WORK_CTX_ALLOCATED  (512)

int vm_page_delayed_work_ctx_needed = 0;
SECURITY_READ_ONLY_LATE(zone_t) dw_ctx_zone;

void
vm_page_delayed_work_init_ctx(void)
{
	size_t elem_size = sizeof(struct vm_page_delayed_work_ctx);

	dw_ctx_zone = zone_create_ext("delayed-work-ctx", elem_size,
	    ZC_NOGC, ZONE_ID_ANY, ^(zone_t z) {
		zone_set_exhaustible(z, MAX_DELAYED_WORK_CTX_ALLOCATED);
	});

	zone_fill_initially(dw_ctx_zone, MIN_DELAYED_WORK_CTX_ALLOCATED);
}

struct vm_page_delayed_work*
vm_page_delayed_work_get_ctx(void)
{
	struct vm_page_delayed_work_ctx * dw_ctx = NULL;

	dw_ctx = (struct vm_page_delayed_work_ctx*) zalloc_noblock(dw_ctx_zone);

	if (dw_ctx) {
		dw_ctx->delayed_owner = current_thread();
	} else {
		vm_page_delayed_work_ctx_needed++;
	}
	return dw_ctx ? dw_ctx->dwp : NULL;
}

void
vm_page_delayed_work_finish_ctx(struct vm_page_delayed_work* dwp)
{
	struct  vm_page_delayed_work_ctx *ldw_ctx;

	ldw_ctx = (struct vm_page_delayed_work_ctx *)dwp;
	ldw_ctx->delayed_owner = NULL;

	zfree(dw_ctx_zone, ldw_ctx);
}

  
                                 
           
                                                     
                                                     
                                                           
                                                     
                                  
                                                           
                                         
                                                                  
                                                             
                                                          
                                                           
                                                         
                                                  
               
                                        
                                              
                                                  
                                                 
                                                    
                                                  
                                                     
                                                   
                                                           
                  
                                                
                                                   
                                               
                                                   
                                            
  
                                                                   
                                                                  
                                                                
                                                            
                                                                
                                                               
                                                             
                                                 
  
   

__private_extern__ kern_return_t
vm_object_upl_request(
	vm_object_t             object,
	vm_object_offset_t      offset,
	upl_size_t              size,
	upl_t                   *upl_ptr,
	upl_page_info_array_t   user_page_list,
	unsigned int            *page_list_count,
	upl_control_flags_t     cntrl_flags,
	vm_tag_t                tag)
{
	vm_page_t               dst_page = VM_PAGE_NULL;
	vm_object_offset_t      dst_offset;
	upl_size_t              xfer_size;
	unsigned int            size_in_pages;
	boolean_t               dirty;
	boolean_t               hw_dirty;
	upl_t                   upl = NULL;
	unsigned int            entry;
	vm_page_t               alias_page = NULL;
	int                     refmod_state = 0;
	wpl_array_t             lite_list = NULL;
	vm_object_t             last_copy_object;
	struct  vm_page_delayed_work    dw_array;
	struct  vm_page_delayed_work    *dwp, *dwp_start;
	bool                    dwp_finish_ctx = TRUE;
	int                     dw_count;
	int                     dw_limit;
	int                     io_tracking_flag = 0;
	int                     grab_options;
	int                     page_grab_count = 0;
	ppnum_t                 phys_page;
	pmap_flush_context      pmap_flush_context_storage;
	boolean_t               pmap_flushes_delayed = FALSE;
#if DEVELOPMENT || DEBUG
	task_t                  task = current_task();
#endif                           

	dwp_start = dwp = NULL;

	if (cntrl_flags & ~UPL_VALID_FLAGS) {
		  
                                      
                             
     
		return KERN_INVALID_VALUE;
	}
	if ((!object->internal) && (object->paging_offset != 0)) {
		panic("vm_object_upl_request: external object with non-zero paging offset\n");
	}
	if (object->phys_contiguous) {
		panic("vm_object_upl_request: contiguous object specified\n");
	}

	assertf(page_aligned(offset) && page_aligned(size),
	    "offset 0x%llx size 0x%x",
	    offset, size);

	VM_DEBUG_CONSTANT_EVENT(vm_object_upl_request, VM_UPL_REQUEST, DBG_FUNC_START, size, cntrl_flags, 0, 0);

	dw_count = 0;
	dw_limit = DELAYED_WORK_LIMIT(DEFAULT_DELAYED_WORK_LIMIT);
	dwp_start = vm_page_delayed_work_get_ctx();
	if (dwp_start == NULL) {
		dwp_start = &dw_array;
		dw_limit = 1;
		dwp_finish_ctx = FALSE;
	}

	dwp = dwp_start;

	if (size > MAX_UPL_SIZE_BYTES) {
		size = MAX_UPL_SIZE_BYTES;
	}

	if ((cntrl_flags & UPL_SET_INTERNAL) && page_list_count != NULL) {
		*page_list_count = MAX_UPL_SIZE_BYTES >> PAGE_SHIFT;
	}

#if CONFIG_IOSCHED || UPL_DEBUG
	if (object->io_tracking || upl_debug_enabled) {
		io_tracking_flag |= UPL_CREATE_IO_TRACKING;
	}
#endif
#if CONFIG_IOSCHED
	if (object->io_tracking) {
		io_tracking_flag |= UPL_CREATE_EXPEDITE_SUP;
	}
#endif

	if (cntrl_flags & UPL_SET_INTERNAL) {
		if (cntrl_flags & UPL_SET_LITE) {
			upl = upl_create(UPL_CREATE_INTERNAL | UPL_CREATE_LITE | io_tracking_flag, 0, size);

			user_page_list = (upl_page_info_t *) (((uintptr_t)upl) + sizeof(struct upl));
			lite_list = (wpl_array_t)
			    (((uintptr_t)user_page_list) +
			    ((size / PAGE_SIZE) * sizeof(upl_page_info_t)));
			if (size == 0) {
				user_page_list = NULL;
				lite_list = NULL;
			}
		} else {
			upl = upl_create(UPL_CREATE_INTERNAL | io_tracking_flag, 0, size);

			user_page_list = (upl_page_info_t *) (((uintptr_t)upl) + sizeof(struct upl));
			if (size == 0) {
				user_page_list = NULL;
			}
		}
	} else {
		if (cntrl_flags & UPL_SET_LITE) {
			upl = upl_create(UPL_CREATE_EXTERNAL | UPL_CREATE_LITE | io_tracking_flag, 0, size);

			lite_list = (wpl_array_t) (((uintptr_t)upl) + sizeof(struct upl));
			if (size == 0) {
				lite_list = NULL;
			}
		} else {
			upl = upl_create(UPL_CREATE_EXTERNAL | io_tracking_flag, 0, size);
		}
	}
	*upl_ptr = upl;

	if (user_page_list) {
		user_page_list[0].device = FALSE;
	}

	if (cntrl_flags & UPL_SET_LITE) {
		upl->map_object = object;
	} else {
		upl->map_object = vm_object_allocate(size);
		  
                                                       
                                           
     
		upl->map_object->shadow = object;
		upl->map_object->pageout = TRUE;
		upl->map_object->can_persist = FALSE;
		upl->map_object->copy_strategy = MEMORY_OBJECT_COPY_NONE;
		upl->map_object->vo_shadow_offset = offset;
		upl->map_object->wimg_bits = object->wimg_bits;
		assertf(page_aligned(upl->map_object->vo_shadow_offset),
		    "object %p shadow_offset 0x%llx",
		    upl->map_object, upl->map_object->vo_shadow_offset);

		alias_page = vm_page_grab_fictitious(TRUE);

		upl->flags |= UPL_SHADOWED;
	}
	if (cntrl_flags & UPL_FOR_PAGEOUT) {
		upl->flags |= UPL_PAGEOUT;
	}

	vm_object_lock(object);
	vm_object_activity_begin(object);

	grab_options = 0;
#if CONFIG_SECLUDED_MEMORY
	if (object->can_grab_secluded) {
		grab_options |= VM_PAGE_GRAB_SECLUDED;
	}
#endif                             

	  
                                                                   
    
	upl->u_size = size;
	upl->u_offset = offset + object->paging_offset;

#if CONFIG_IOSCHED || UPL_DEBUG
	if (object->io_tracking || upl_debug_enabled) {
		vm_object_activity_begin(object);
		queue_enter(&object->uplq, upl, upl_t, uplq);
	}
#endif
	if ((cntrl_flags & UPL_WILL_MODIFY) && object->copy != VM_OBJECT_NULL) {
		  
                                    
    
                                            
                                             
                                               
                                                
                            
     
		vm_object_update(object,
		    offset,
		    size,
		    NULL,
		    NULL,
		    FALSE,                                 
		    MEMORY_OBJECT_COPY_SYNC,
		    VM_PROT_NO_CHANGE);

		VM_PAGEOUT_DEBUG(upl_cow, 1);
		VM_PAGEOUT_DEBUG(upl_cow_pages, (size >> PAGE_SHIFT));
	}
	  
                                                   
    
	last_copy_object = object->copy;
	entry = 0;

	xfer_size = size;
	dst_offset = offset;
	size_in_pages = size / PAGE_SIZE;

	if (vm_page_free_count > (vm_page_free_target + size_in_pages) ||
	    object->resident_page_count < ((MAX_UPL_SIZE_BYTES * 2) >> PAGE_SHIFT)) {
		object->scan_collisions = 0;
	}

	if ((cntrl_flags & UPL_WILL_MODIFY) && must_throttle_writes() == TRUE) {
		boolean_t       isSSD = FALSE;

#if !XNU_TARGET_OS_OSX
		isSSD = TRUE;
#else                         
		vnode_pager_get_isSSD(object->pager, &isSSD);
#endif                         
		vm_object_unlock(object);

		OSAddAtomic(size_in_pages, &vm_upl_wait_for_pages);

		if (isSSD == TRUE) {
			delay(1000 * size_in_pages);
		} else {
			delay(5000 * size_in_pages);
		}
		OSAddAtomic(-size_in_pages, &vm_upl_wait_for_pages);

		vm_object_lock(object);
	}

	while (xfer_size) {
		dwp->dw_mask = 0;

		if ((alias_page == NULL) && !(cntrl_flags & UPL_SET_LITE)) {
			vm_object_unlock(object);
			alias_page = vm_page_grab_fictitious(TRUE);
			vm_object_lock(object);
		}
		if (cntrl_flags & UPL_COPYOUT_FROM) {
			upl->flags |= UPL_PAGE_SYNC_DONE;

			if (((dst_page = vm_page_lookup(object, dst_offset)) == VM_PAGE_NULL) ||
			    dst_page->vmp_fictitious ||
			    dst_page->vmp_absent ||
			    dst_page->vmp_error ||
			    dst_page->vmp_cleaning ||
			    (VM_PAGE_WIRED(dst_page))) {
				if (user_page_list) {
					user_page_list[entry].phys_addr = 0;
				}

				goto try_next_page;
			}
			phys_page = VM_PAGE_GET_PHYS_PAGE(dst_page);

			  
                           
                                                   
                                                      
                                                      
                                                         
      
			if (dst_page->vmp_pmapped) {
				refmod_state = pmap_get_refmod(phys_page);
			} else {
				refmod_state = 0;
			}

			if ((refmod_state & VM_MEM_REFERENCED) && VM_PAGE_INACTIVE(dst_page)) {
				  
                                                 
                                                   
                                                 
                                          
       
				dwp->dw_mask |= DW_vm_page_activate;
			}
			if (cntrl_flags & UPL_RET_ONLY_DIRTY) {
				  
                                                       
       
				if (dst_page->vmp_laundry || !(cntrl_flags & UPL_FOR_PAGEOUT)) {
					  
                                                           
                                                         
                                                                
                                                              
                                               
        
					if (dst_page->vmp_dirty || dst_page->vmp_precious || (refmod_state & VM_MEM_MODIFIED)) {
						goto check_busy;
					}
					goto dont_return;
				}
				  
                                                            
                                                            
                                                           
                                             
       
				if ((hibernate_cleaning_in_progress == TRUE ||
				    (!((refmod_state & VM_MEM_REFERENCED) || dst_page->vmp_reference) ||
				    (dst_page->vmp_q_state == VM_PAGE_ON_THROTTLED_Q))) &&
				    ((refmod_state & VM_MEM_MODIFIED) || dst_page->vmp_dirty || dst_page->vmp_precious)) {
					goto check_busy;
				}
dont_return:
				  
                                            
                                        
       
				if (dst_page->vmp_laundry == TRUE) {
					  
                                                                        
                                                                
                                                          
                                     
        
					vm_page_lockspin_queues();

					vm_pageout_steal_laundry(dst_page, TRUE);
					vm_page_activate(dst_page);

					vm_page_unlock_queues();
				}
				if (user_page_list) {
					user_page_list[entry].phys_addr = 0;
				}

				goto try_next_page;
			}
check_busy:
			if (dst_page->vmp_busy) {
				if (cntrl_flags & UPL_NOBLOCK) {
					if (user_page_list) {
						user_page_list[entry].phys_addr = 0;
					}
					dwp->dw_mask = 0;

					goto try_next_page;
				}
				  
                                       
                                   
       
				PAGE_SLEEP(object, dst_page, THREAD_UNINT);

				continue;
			}
			if (dst_page->vmp_q_state == VM_PAGE_ON_PAGEOUT_Q) {
				vm_page_lockspin_queues();

				if (dst_page->vmp_q_state == VM_PAGE_ON_PAGEOUT_Q) {
					  
                                                       
                                                  
                                                  
                                                    
                     
        
					vm_pageout_throttle_up(dst_page);
				}
				vm_page_unlock_queues();
			}
			hw_dirty = refmod_state & VM_MEM_MODIFIED;
			dirty = hw_dirty ? TRUE : dst_page->vmp_dirty;

			if (phys_page > upl->highest_page) {
				upl->highest_page = phys_page;
			}

			assert(!pmap_is_noencrypt(phys_page));

			if (cntrl_flags & UPL_SET_LITE) {
				unsigned int    pg_num;

				pg_num = (unsigned int) ((dst_offset - offset) / PAGE_SIZE);
				assert(pg_num == (dst_offset - offset) / PAGE_SIZE);
				lite_list[pg_num >> 5] |= 1U << (pg_num & 31);

				if (hw_dirty) {
					if (pmap_flushes_delayed == FALSE) {
						pmap_flush_context_init(&pmap_flush_context_storage);
						pmap_flushes_delayed = TRUE;
					}
					pmap_clear_refmod_options(phys_page,
					    VM_MEM_MODIFIED,
					    PMAP_OPTIONS_NOFLUSH | PMAP_OPTIONS_CLEAR_WRITE,
					    &pmap_flush_context_storage);
				}

				  
                                     
                
       
				dst_page->vmp_cleaning = TRUE;
				dst_page->vmp_precious = FALSE;
			} else {
				  
                                      
                                      
                 
       
				vm_object_lock(upl->map_object);
				vm_pageclean_setup(dst_page, alias_page, upl->map_object, size - xfer_size);
				vm_object_unlock(upl->map_object);

				alias_page->vmp_absent = FALSE;
				alias_page = NULL;
			}
			if (dirty) {
				SET_PAGE_DIRTY(dst_page, FALSE);
			} else {
				dst_page->vmp_dirty = FALSE;
			}

			if (!dirty) {
				dst_page->vmp_precious = TRUE;
			}

			if (!(cntrl_flags & UPL_CLEAN_IN_PLACE)) {
				if (!VM_PAGE_WIRED(dst_page)) {
					dst_page->vmp_free_when_done = TRUE;
				}
			}
		} else {
			if ((cntrl_flags & UPL_WILL_MODIFY) && object->copy != last_copy_object) {
				  
                                      
      
                                           
                                           
                                          
                                              
                                               
                                               
                                          
                                                  
                                                 
                                              
                                             
                                                 
                                                 
                                                 
                                                 
                                                  
                                                  
       
				if (object->copy != VM_OBJECT_NULL) {
					vm_object_update(
						object,
						dst_offset,                    
						xfer_size,                     
						NULL,
						NULL,
						FALSE,                        
						MEMORY_OBJECT_COPY_SYNC,
						VM_PROT_NO_CHANGE);

					VM_PAGEOUT_DEBUG(upl_cow_again, 1);
					VM_PAGEOUT_DEBUG(upl_cow_again_pages, (xfer_size >> PAGE_SHIFT));
				}
				  
                                              
       
				last_copy_object = object->copy;
			}
			dst_page = vm_page_lookup(object, dst_offset);

			if (dst_page != VM_PAGE_NULL) {
				if ((cntrl_flags & UPL_RET_ONLY_ABSENT)) {
					  
                                                    
        
					if (user_page_list) {
						user_page_list[entry].phys_addr = 0;
					}

					goto try_next_page;
				}
				if (dst_page->vmp_fictitious) {
					panic("need corner case for fictitious page");
				}

				if (dst_page->vmp_busy || dst_page->vmp_cleaning) {
					  
                                        
                                    
        
					PAGE_SLEEP(object, dst_page, THREAD_UNINT);

					continue;
				}
				if (dst_page->vmp_laundry) {
					vm_pageout_steal_laundry(dst_page, FALSE);
				}
			} else {
				if (object->private) {
					  
                                         
                                      
                                     
                                     
                                  
                                   
                       
        
					if (user_page_list) {
						user_page_list[entry].phys_addr = 0;
					}

					goto try_next_page;
				}
				if (object->scan_collisions) {
					  
                                                  
                                                    
                                                           
                                                         
                                                            
                                                         
                                                       
                                                    
                    
        
					dst_page = vm_object_page_grab(object);

					if (dst_page != VM_PAGE_NULL) {
						vm_page_release(dst_page,
						    FALSE);
					}

					dst_page = vm_object_page_grab(object);
				}
				if (dst_page == VM_PAGE_NULL) {
					  
                               
        
					dst_page = vm_page_grab_options(grab_options);
					if (dst_page != VM_PAGE_NULL) {
						page_grab_count++;
					}
				}
				if (dst_page == VM_PAGE_NULL) {
					if ((cntrl_flags & (UPL_RET_ONLY_ABSENT | UPL_NOBLOCK)) == (UPL_RET_ONLY_ABSENT | UPL_NOBLOCK)) {
						  
                                                                            
                                                             
                                                  
         
						if (user_page_list) {
							user_page_list[entry].phys_addr = 0;
						}

						goto try_next_page;
					}
					  
                                  
                                   
                 
        
					vm_object_unlock(object);

					OSAddAtomic(size_in_pages, &vm_upl_wait_for_pages);

					VM_DEBUG_EVENT(vm_upl_page_wait, VM_UPL_PAGE_WAIT, DBG_FUNC_START, vm_upl_wait_for_pages, 0, 0, 0);

					VM_PAGE_WAIT();
					OSAddAtomic(-size_in_pages, &vm_upl_wait_for_pages);

					VM_DEBUG_EVENT(vm_upl_page_wait, VM_UPL_PAGE_WAIT, DBG_FUNC_END, vm_upl_wait_for_pages, 0, 0, 0);

					vm_object_lock(object);

					continue;
				}
				vm_page_insert(dst_page, object, dst_offset);

				dst_page->vmp_absent = TRUE;
				dst_page->vmp_busy = FALSE;

				if (cntrl_flags & UPL_RET_ONLY_ABSENT) {
					  
                                             
                                          
                                       
                                                
                                               
                        
        
					dst_page->vmp_clustered = TRUE;

					if (!(cntrl_flags & UPL_FILE_IO)) {
						counter_inc(&vm_statistics_pageins);
					}
				}
			}
			phys_page = VM_PAGE_GET_PHYS_PAGE(dst_page);

			dst_page->vmp_overwriting = TRUE;

			if (dst_page->vmp_pmapped) {
				if (!(cntrl_flags & UPL_FILE_IO)) {
					  
                                       
                                       
        
					refmod_state = pmap_disconnect(phys_page);
				} else {
					refmod_state = pmap_get_refmod(phys_page);
				}
			} else {
				refmod_state = 0;
			}

			hw_dirty = refmod_state & VM_MEM_MODIFIED;
			dirty = hw_dirty ? TRUE : dst_page->vmp_dirty;

			if (cntrl_flags & UPL_SET_LITE) {
				unsigned int    pg_num;

				pg_num = (unsigned int) ((dst_offset - offset) / PAGE_SIZE);
				assert(pg_num == (dst_offset - offset) / PAGE_SIZE);
				lite_list[pg_num >> 5] |= 1U << (pg_num & 31);

				if (hw_dirty) {
					pmap_clear_modify(phys_page);
				}

				  
                                     
                
       
				dst_page->vmp_cleaning = TRUE;
				dst_page->vmp_precious = FALSE;
			} else {
				  
                                      
                                      
                 
       
				vm_object_lock(upl->map_object);
				vm_pageclean_setup(dst_page, alias_page, upl->map_object, size - xfer_size);
				vm_object_unlock(upl->map_object);

				alias_page->vmp_absent = FALSE;
				alias_page = NULL;
			}

			if (cntrl_flags & UPL_REQUEST_SET_DIRTY) {
				upl->flags &= ~UPL_CLEAR_DIRTY;
				upl->flags |= UPL_SET_DIRTY;
				dirty = TRUE;
				  
                                                         
                                                         
                                                          
                                 
       
				if (object->code_signed && dst_page->vmp_cs_tainted != VMP_CS_ALL_TRUE) {
					dst_page->vmp_cs_tainted = VMP_CS_ALL_TRUE;
					vm_page_upl_tainted++;
					if (dst_page->vmp_pmapped) {
						refmod_state = pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(dst_page));
						if (refmod_state & VM_MEM_REFERENCED) {
							dst_page->vmp_reference = TRUE;
						}
					}
				}
			} else if (cntrl_flags & UPL_CLEAN_IN_PLACE) {
				  
                                      
                                       
                                      
                                        
                                   
                                         
       
				upl->flags |= UPL_CLEAR_DIRTY;
			}
			dst_page->vmp_dirty = dirty;

			if (!dirty) {
				dst_page->vmp_precious = TRUE;
			}

			if (!VM_PAGE_WIRED(dst_page)) {
				  
                                           
                            
       
				dst_page->vmp_busy = TRUE;
			} else {
				dwp->dw_mask |= DW_vm_page_wire;
			}

			  
                                                         
                                                  
      
			dst_page->vmp_restart = FALSE;
			if (!dst_page->vmp_absent && !(cntrl_flags & UPL_WILL_MODIFY)) {
				  
                                 
       
				dwp->dw_mask |= DW_set_reference;
			}
			if (cntrl_flags & UPL_PRECIOUS) {
				if (object->internal) {
					SET_PAGE_DIRTY(dst_page, FALSE);
					dst_page->vmp_precious = FALSE;
				} else {
					dst_page->vmp_precious = TRUE;
				}
			} else {
				dst_page->vmp_precious = FALSE;
			}
		}
		if (dst_page->vmp_busy) {
			upl->flags |= UPL_HAS_BUSY;
		}

		if (phys_page > upl->highest_page) {
			upl->highest_page = phys_page;
		}
		assert(!pmap_is_noencrypt(phys_page));
		if (user_page_list) {
			user_page_list[entry].phys_addr = phys_page;
			user_page_list[entry].free_when_done    = dst_page->vmp_free_when_done;
			user_page_list[entry].absent    = dst_page->vmp_absent;
			user_page_list[entry].dirty     = dst_page->vmp_dirty;
			user_page_list[entry].precious  = dst_page->vmp_precious;
			user_page_list[entry].device    = FALSE;
			user_page_list[entry].needed    = FALSE;
			if (dst_page->vmp_clustered == TRUE) {
				user_page_list[entry].speculative = (dst_page->vmp_q_state == VM_PAGE_ON_SPECULATIVE_Q) ? TRUE : FALSE;
			} else {
				user_page_list[entry].speculative = FALSE;
			}
			user_page_list[entry].cs_validated = dst_page->vmp_cs_validated;
			user_page_list[entry].cs_tainted = dst_page->vmp_cs_tainted;
			user_page_list[entry].cs_nx = dst_page->vmp_cs_nx;
			user_page_list[entry].mark      = FALSE;
		}
		  
                                        
                                               
                                         
                                              
                                             
     
		if (!(cntrl_flags & UPL_RET_ONLY_ABSENT)) {
			  
                                                 
                                            
     
      
			if (dst_page->vmp_clustered) {
				VM_PAGE_CONSUME_CLUSTERED(dst_page);
			}
		}
try_next_page:
		if (dwp->dw_mask) {
			if (dwp->dw_mask & DW_vm_page_activate) {
				counter_inc(&vm_statistics_reactivations);
			}

			VM_PAGE_ADD_DELAYED_WORK(dwp, dst_page, dw_count);

			if (dw_count >= dw_limit) {
				vm_page_do_delayed_work(object, tag, dwp_start, dw_count);

				dwp = dwp_start;
				dw_count = 0;
			}
		}
		entry++;
		dst_offset += PAGE_SIZE_64;
		xfer_size -= PAGE_SIZE;
	}
	if (dw_count) {
		vm_page_do_delayed_work(object, tag, dwp_start, dw_count);
		dwp = dwp_start;
		dw_count = 0;
	}

	if (alias_page != NULL) {
		VM_PAGE_FREE(alias_page);
	}
	if (pmap_flushes_delayed == TRUE) {
		pmap_flush(&pmap_flush_context_storage);
	}

	if (page_list_count != NULL) {
		if (upl->flags & UPL_INTERNAL) {
			*page_list_count = 0;
		} else if (*page_list_count > entry) {
			*page_list_count = entry;
		}
	}
#if UPL_DEBUG
	upl->upl_state = 1;
#endif
	vm_object_unlock(object);

	VM_DEBUG_CONSTANT_EVENT(vm_object_upl_request, VM_UPL_REQUEST, DBG_FUNC_END, page_grab_count, 0, 0, 0);
#if DEVELOPMENT || DEBUG
	if (task != NULL) {
		ledger_credit(task->ledger, task_ledgers.pages_grabbed_upl, page_grab_count);
	}
#endif                           

	if (dwp_start && dwp_finish_ctx) {
		vm_page_delayed_work_finish_ctx(dwp_start);
		dwp_start = dwp = NULL;
	}

	return KERN_SUCCESS;
}

  
                                       
           
                                                    
                                                      
                                                     
                                                           
                                                       
                          
   

__private_extern__ kern_return_t
vm_object_super_upl_request(
	vm_object_t object,
	vm_object_offset_t      offset,
	upl_size_t              size,
	upl_size_t              super_cluster,
	upl_t                   *upl,
	upl_page_info_t         *user_page_list,
	unsigned int            *page_list_count,
	upl_control_flags_t     cntrl_flags,
	vm_tag_t                tag)
{
	if (object->paging_offset > offset || ((cntrl_flags & UPL_VECTOR) == UPL_VECTOR)) {
		return KERN_FAILURE;
	}

	assert(object->paging_in_progress);
	offset = offset - object->paging_offset;

	if (super_cluster > size) {
		vm_object_offset_t      base_offset;
		upl_size_t              super_size;
		vm_object_size_t        super_size_64;

		base_offset = (offset & ~((vm_object_offset_t) super_cluster - 1));
		super_size = (offset + size) > (base_offset + super_cluster) ? super_cluster << 1 : super_cluster;
		super_size_64 = ((base_offset + super_size) > object->vo_size) ? (object->vo_size - base_offset) : super_size;
		super_size = (upl_size_t) super_size_64;
		assert(super_size == super_size_64);

		if (offset > (base_offset + super_size)) {
			panic("vm_object_super_upl_request: Missed target pageout"
			    " %#llx,%#llx, %#x, %#x, %#x, %#llx\n",
			    offset, base_offset, super_size, super_cluster,
			    size, object->paging_offset);
		}
		  
                                                       
                                                      
                
     
		if ((offset + size) > (base_offset + super_size)) {
			super_size_64 = (offset + size) - base_offset;
			super_size = (upl_size_t) super_size_64;
			assert(super_size == super_size_64);
		}

		offset = base_offset;
		size = super_size;
	}
	return vm_object_upl_request(object, offset, size, upl, user_page_list, page_list_count, cntrl_flags, tag);
}

int cs_executable_create_upl = 0;
extern int proc_selfpid(void);
extern char *proc_name_address(void *p);

kern_return_t
vm_map_create_upl(
	vm_map_t                map,
	vm_map_address_t        offset,
	upl_size_t              *upl_size,
	upl_t                   *upl,
	upl_page_info_array_t   page_list,
	unsigned int            *count,
	upl_control_flags_t     *flags,
	vm_tag_t                tag)
{
	vm_map_entry_t          entry;
	upl_control_flags_t     caller_flags;
	int                     force_data_sync;
	int                     sync_cow_data;
	vm_object_t             local_object;
	vm_map_offset_t         local_offset;
	vm_map_offset_t         local_start;
	kern_return_t           ret;
	vm_map_address_t        original_offset;
	vm_map_size_t           original_size, adjusted_size;
	vm_map_offset_t         local_entry_start;
	vm_object_offset_t      local_entry_offset;
	vm_object_offset_t      offset_in_mapped_page;
	boolean_t               release_map = FALSE;

start_with_map:

	original_offset = offset;
	original_size = *upl_size;
	adjusted_size = original_size;

	caller_flags = *flags;

	if (caller_flags & ~UPL_VALID_FLAGS) {
		  
                                      
                             
     
		ret = KERN_INVALID_VALUE;
		goto done;
	}
	force_data_sync = (caller_flags & UPL_FORCE_DATA_SYNC);
	sync_cow_data = !(caller_flags & UPL_COPYOUT_FROM);

	if (upl == NULL) {
		ret = KERN_INVALID_ARGUMENT;
		goto done;
	}

REDISCOVER_ENTRY:
	vm_map_lock_read(map);

	if (!vm_map_lookup_entry(map, offset, &entry)) {
		vm_map_unlock_read(map);
		ret = KERN_FAILURE;
		goto done;
	}

	local_entry_start = entry->vme_start;
	local_entry_offset = VME_OFFSET(entry);

	if (VM_MAP_PAGE_SHIFT(map) < PAGE_SHIFT) {
		DEBUG4K_UPL("map %p (%d) offset 0x%llx size 0x%x flags 0x%llx\n", map, VM_MAP_PAGE_SHIFT(map), (uint64_t)offset, *upl_size, *flags);
	}

	if (entry->vme_end - original_offset < adjusted_size) {
		adjusted_size = entry->vme_end - original_offset;
		assert(adjusted_size > 0);
		*upl_size = (upl_size_t) adjusted_size;
		assert(*upl_size == adjusted_size);
	}

	if (caller_flags & UPL_QUERY_OBJECT_TYPE) {
		*flags = 0;

		if (!entry->is_sub_map &&
		    VME_OBJECT(entry) != VM_OBJECT_NULL) {
			if (VME_OBJECT(entry)->private) {
				*flags = UPL_DEV_MEMORY;
			}

			if (VME_OBJECT(entry)->phys_contiguous) {
				*flags |= UPL_PHYS_CONTIG;
			}
		}
		vm_map_unlock_read(map);
		ret = KERN_SUCCESS;
		goto done;
	}

	offset_in_mapped_page = 0;
	if (VM_MAP_PAGE_SIZE(map) < PAGE_SIZE) {
		offset = vm_map_trunc_page(original_offset, VM_MAP_PAGE_MASK(map));
		*upl_size = (upl_size_t)
		    (vm_map_round_page(original_offset + adjusted_size,
		    VM_MAP_PAGE_MASK(map))
		    - offset);

		offset_in_mapped_page = original_offset - offset;
		assert(offset_in_mapped_page < VM_MAP_PAGE_SIZE(map));

		DEBUG4K_UPL("map %p (%d) offset 0x%llx size 0x%llx flags 0x%llx -> offset 0x%llx adjusted_size 0x%llx *upl_size 0x%x offset_in_mapped_page 0x%llx\n", map, VM_MAP_PAGE_SHIFT(map), (uint64_t)original_offset, (uint64_t)original_size, *flags, (uint64_t)offset, (uint64_t)adjusted_size, *upl_size, offset_in_mapped_page);
	}

	if (VME_OBJECT(entry) == VM_OBJECT_NULL ||
	    !VME_OBJECT(entry)->phys_contiguous) {
		if (*upl_size > MAX_UPL_SIZE_BYTES) {
			*upl_size = MAX_UPL_SIZE_BYTES;
		}
	}

	  
                                       
    
	if (VME_OBJECT(entry) == VM_OBJECT_NULL) {
		if (vm_map_lock_read_to_write(map)) {
			goto REDISCOVER_ENTRY;
		}

		VME_OBJECT_SET(entry,
		    vm_object_allocate((vm_size_t)
		    vm_object_round_page((entry->vme_end - entry->vme_start))));
		VME_OFFSET_SET(entry, 0);
		assert(entry->use_pmap);

		vm_map_lock_write_to_read(map);
	}

	if (!(caller_flags & UPL_COPYOUT_FROM) &&
	    !entry->is_sub_map &&
	    !(entry->protection & VM_PROT_WRITE)) {
		vm_map_unlock_read(map);
		ret = KERN_PROTECTION_FAILURE;
		goto done;
	}

#if !XNU_TARGET_OS_OSX
	if (map->pmap != kernel_pmap &&
	    (caller_flags & UPL_COPYOUT_FROM) &&
	    (entry->protection & VM_PROT_EXECUTE) &&
	    !(entry->protection & VM_PROT_WRITE)) {
		vm_offset_t     kaddr;
		vm_size_t       ksize;

		  
                                                    
                                       
                                                            
                                                     
                                                      
                                                          
                                            
                                                             
                                                            
                                           
     

		vm_map_unlock_read(map);
		entry = VM_MAP_ENTRY_NULL;
		                            
		ksize = round_page(*upl_size);
		kaddr = 0;
		ret = kmem_alloc_pageable(kernel_map,
		    &kaddr,
		    ksize,
		    tag);
		if (ret == KERN_SUCCESS) {
			                          
			ret = copyinmap(map, offset, (void *)kaddr, *upl_size);
		}
		if (ret == KERN_SUCCESS) {
			if (ksize > *upl_size) {
				                                               
				memset((void *)(kaddr + *upl_size),
				    0,
				    ksize - *upl_size);
			}
			                                           
			vm_object_offset_t      offset_in_object;
			vm_object_offset_t      offset_in_object_page;

			offset_in_object = offset - local_entry_start + local_entry_offset;
			offset_in_object_page = offset_in_object - vm_object_trunc_page(offset_in_object);
			assert(offset_in_object_page < PAGE_SIZE);
			assert(offset_in_object_page + offset_in_mapped_page < PAGE_SIZE);
			*upl_size -= offset_in_object_page + offset_in_mapped_page;
			ret = vm_map_create_upl(kernel_map,
			    (vm_map_address_t)(kaddr + offset_in_object_page + offset_in_mapped_page),
			    upl_size, upl, page_list, count, flags, tag);
		}
		if (kaddr != 0) {
			                            
			kmem_free(kernel_map, kaddr, ksize);
			kaddr = 0;
			ksize = 0;
		}
#if DEVELOPMENT || DEBUG
		DTRACE_VM4(create_upl_from_executable,
		    vm_map_t, map,
		    vm_map_address_t, offset,
		    upl_size_t, *upl_size,
		    kern_return_t, ret);
#endif                           
		goto done;
	}
#endif                         

	local_object = VME_OBJECT(entry);
	assert(local_object != VM_OBJECT_NULL);

	if (!entry->is_sub_map &&
	    !entry->needs_copy &&
	    *upl_size != 0 &&
	    local_object->vo_size > *upl_size &&                  
	    entry->wired_count == 0 &&                                        
	    (map->pmap != kernel_pmap) &&                   
	    (vm_map_entry_should_cow_for_true_share(entry)             
	    ||
	    (             
		    local_object->internal &&
		    (local_object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) &&
		    local_object->ref_count > 1))) {
		vm_prot_t       prot;

		  
            
                                                         
                                                         
    
            
                                                   
                                                       
                                                     
                                                   
                                               
                                                   
                                                   
                                                     
     

		if (vm_map_lock_read_to_write(map)) {
			goto REDISCOVER_ENTRY;
		}
		vm_map_lock_assert_exclusive(map);
		assert(VME_OBJECT(entry) == local_object);

		vm_map_clip_start(map,
		    entry,
		    vm_map_trunc_page(offset,
		    VM_MAP_PAGE_MASK(map)));
		vm_map_clip_end(map,
		    entry,
		    vm_map_round_page(offset + *upl_size,
		    VM_MAP_PAGE_MASK(map)));
		if ((entry->vme_end - offset) < *upl_size) {
			*upl_size = (upl_size_t) (entry->vme_end - offset);
			assert(*upl_size == entry->vme_end - offset);
		}

		prot = entry->protection & ~VM_PROT_WRITE;
		if (override_nx(map, VME_ALIAS(entry)) && prot) {
			prot |= VM_PROT_EXECUTE;
		}
		vm_object_pmap_protect(local_object,
		    VME_OFFSET(entry),
		    entry->vme_end - entry->vme_start,
		    ((entry->is_shared ||
		    map->mapped_in_other_pmaps)
		    ? PMAP_NULL
		    : map->pmap),
		    VM_MAP_PAGE_SIZE(map),
		    entry->vme_start,
		    prot);

		assert(entry->wired_count == 0);

		  
                                                               
                                                            
                                                          
     
		vm_object_lock(local_object);
		if (local_object->true_share) {
			                                                      
			assert(local_object->copy_strategy !=
			    MEMORY_OBJECT_COPY_SYMMETRIC);
		} else {
			                                                 
			assert(local_object->copy_strategy ==
			    MEMORY_OBJECT_COPY_SYMMETRIC);
			entry->needs_copy = TRUE;
		}
		vm_object_unlock(local_object);

		vm_map_lock_write_to_read(map);
	}

	if (entry->needs_copy) {
		  
                                           
              
     
		vm_map_t                local_map;
		vm_object_t             object;
		vm_object_offset_t      new_offset;
		vm_prot_t               prot;
		boolean_t               wired;
		vm_map_version_t        version;
		vm_map_t                real_map;
		vm_prot_t               fault_type;

		local_map = map;

		if (caller_flags & UPL_COPYOUT_FROM) {
			fault_type = VM_PROT_READ | VM_PROT_COPY;
			vm_counters.create_upl_extra_cow++;
			vm_counters.create_upl_extra_cow_pages +=
			    (entry->vme_end - entry->vme_start) / PAGE_SIZE;
		} else {
			fault_type = VM_PROT_WRITE;
		}
		if (vm_map_lookup_locked(&local_map,
		    offset, fault_type,
		    OBJECT_LOCK_EXCLUSIVE,
		    &version, &object,
		    &new_offset, &prot, &wired,
		    NULL,
		    &real_map, NULL) != KERN_SUCCESS) {
			if (fault_type == VM_PROT_WRITE) {
				vm_counters.create_upl_lookup_failure_write++;
			} else {
				vm_counters.create_upl_lookup_failure_copy++;
			}
			vm_map_unlock_read(local_map);
			ret = KERN_FAILURE;
			goto done;
		}
		if (real_map != local_map) {
			vm_map_unlock(real_map);
		}
		vm_map_unlock_read(local_map);

		vm_object_unlock(object);

		goto REDISCOVER_ENTRY;
	}

	if (entry->is_sub_map) {
		vm_map_t        submap;

		submap = VME_SUBMAP(entry);
		local_start = entry->vme_start;
		local_offset = (vm_map_offset_t)VME_OFFSET(entry);

		vm_map_reference(submap);
		vm_map_unlock_read(map);

		DEBUG4K_UPL("map %p offset 0x%llx (0x%llx) size 0x%x (adjusted 0x%llx original 0x%llx) offset_in_mapped_page 0x%llx submap %p\n", map, (uint64_t)offset, (uint64_t)original_offset, *upl_size, (uint64_t)adjusted_size, (uint64_t)original_size, offset_in_mapped_page, submap);
		offset += offset_in_mapped_page;
		*upl_size -= offset_in_mapped_page;

		if (release_map) {
			vm_map_deallocate(map);
		}
		map = submap;
		release_map = TRUE;
		offset = local_offset + (offset - local_start);
		goto start_with_map;
	}

	if (sync_cow_data &&
	    (VME_OBJECT(entry)->shadow ||
	    VME_OBJECT(entry)->copy)) {
		local_object = VME_OBJECT(entry);
		local_start = entry->vme_start;
		local_offset = (vm_map_offset_t)VME_OFFSET(entry);

		vm_object_reference(local_object);
		vm_map_unlock_read(map);

		if (local_object->shadow && local_object->copy) {
			vm_object_lock_request(local_object->shadow,
			    ((vm_object_offset_t)
			    ((offset - local_start) +
			    local_offset) +
			    local_object->vo_shadow_offset),
			    *upl_size, FALSE,
			    MEMORY_OBJECT_DATA_SYNC,
			    VM_PROT_NO_CHANGE);
		}
		sync_cow_data = FALSE;
		vm_object_deallocate(local_object);

		goto REDISCOVER_ENTRY;
	}
	if (force_data_sync) {
		local_object = VME_OBJECT(entry);
		local_start = entry->vme_start;
		local_offset = (vm_map_offset_t)VME_OFFSET(entry);

		vm_object_reference(local_object);
		vm_map_unlock_read(map);

		vm_object_lock_request(local_object,
		    ((vm_object_offset_t)
		    ((offset - local_start) +
		    local_offset)),
		    (vm_object_size_t)*upl_size,
		    FALSE,
		    MEMORY_OBJECT_DATA_SYNC,
		    VM_PROT_NO_CHANGE);

		force_data_sync = FALSE;
		vm_object_deallocate(local_object);

		goto REDISCOVER_ENTRY;
	}
	if (VME_OBJECT(entry)->private) {
		*flags = UPL_DEV_MEMORY;
	} else {
		*flags = 0;
	}

	if (VME_OBJECT(entry)->phys_contiguous) {
		*flags |= UPL_PHYS_CONTIG;
	}

	local_object = VME_OBJECT(entry);
	local_offset = (vm_map_offset_t)VME_OFFSET(entry);
	local_start = entry->vme_start;

	  
                                                    
                                                
                                                      
                                           
    
	if (entry->protection & VM_PROT_EXECUTE) {
#if MACH_ASSERT
		printf("pid %d[%s] create_upl out of executable range from "
		    "0x%llx to 0x%llx: side effects may include "
		    "code-signing violations later on\n",
		    proc_selfpid(),
		    (current_task()->bsd_info
		    ? proc_name_address(current_task()->bsd_info)
		    : "?"),
		    (uint64_t) entry->vme_start,
		    (uint64_t) entry->vme_end);
#endif                  
		DTRACE_VM2(cs_executable_create_upl,
		    uint64_t, (uint64_t)entry->vme_start,
		    uint64_t, (uint64_t)entry->vme_end);
		cs_executable_create_upl++;
	}

	vm_object_lock(local_object);

	  
                                                                 
                                                                        
                                                                       
                                                                   
                               
    
	if (local_object->true_share) {
		assert(local_object->copy_strategy !=
		    MEMORY_OBJECT_COPY_SYMMETRIC);
	} else if (local_object != kernel_object &&
	    local_object != compressor_object &&
	    !local_object->phys_contiguous) {
#if VM_OBJECT_TRACKING_OP_TRUESHARE
		if (!local_object->true_share &&
		    vm_object_tracking_inited) {
			void *bt[VM_OBJECT_TRACKING_BTDEPTH];
			int num = 0;
			num = OSBacktrace(bt,
			    VM_OBJECT_TRACKING_BTDEPTH);
			btlog_add_entry(vm_object_tracking_btlog,
			    local_object,
			    VM_OBJECT_TRACKING_OP_TRUESHARE,
			    bt,
			    num);
		}
#endif                                      
		local_object->true_share = TRUE;
		if (local_object->copy_strategy ==
		    MEMORY_OBJECT_COPY_SYMMETRIC) {
			local_object->copy_strategy = MEMORY_OBJECT_COPY_DELAY;
		}
	}

	vm_object_reference_locked(local_object);
	vm_object_unlock(local_object);

	vm_map_unlock_read(map);

	offset += offset_in_mapped_page;
	assert(*upl_size > offset_in_mapped_page);
	*upl_size -= offset_in_mapped_page;

	ret = vm_object_iopl_request(local_object,
	    ((vm_object_offset_t)
	    ((offset - local_start) + local_offset)),
	    *upl_size,
	    upl,
	    page_list,
	    count,
	    caller_flags,
	    tag);
	vm_object_deallocate(local_object);

done:
	if (release_map) {
		vm_map_deallocate(map);
	}

	return ret;
}

  
                                                 
  
                                                        
                      
   
kern_return_t
vm_map_enter_upl(
	vm_map_t                map,
	upl_t                   upl,
	vm_map_offset_t         *dst_addr)
{
	vm_map_size_t           size;
	vm_object_offset_t      offset;
	vm_map_offset_t         addr;
	vm_page_t               m;
	kern_return_t           kr;
	int                     isVectorUPL = 0, curr_upl = 0;
	upl_t                   vector_upl = NULL;
	vm_offset_t             vector_upl_dst_addr = 0;
	vm_map_t                vector_upl_submap = NULL;
	upl_offset_t            subupl_offset = 0;
	upl_size_t              subupl_size = 0;

	if (upl == UPL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	DEBUG4K_UPL("map %p upl %p flags 0x%x object %p offset 0x%llx size 0x%x \n", map, upl, upl->flags, upl->map_object, upl->u_offset, upl->u_size);
	assert(map == kernel_map);

	if ((isVectorUPL = vector_upl_is_valid(upl))) {
		int mapped = 0, valid_upls = 0;
		vector_upl = upl;

		upl_lock(vector_upl);
		for (curr_upl = 0; curr_upl < MAX_VECTOR_UPL_ELEMENTS; curr_upl++) {
			upl =  vector_upl_subupl_byindex(vector_upl, curr_upl );
			if (upl == NULL) {
				continue;
			}
			valid_upls++;
			if (UPL_PAGE_LIST_MAPPED & upl->flags) {
				mapped++;
			}
		}

		if (mapped) {
			if (mapped != valid_upls) {
				panic("Only %d of the %d sub-upls within the Vector UPL are alread mapped\n", mapped, valid_upls);
			} else {
				upl_unlock(vector_upl);
				return KERN_FAILURE;
			}
		}

		if (VM_MAP_PAGE_MASK(map) < PAGE_MASK) {
			panic("TODO4K: vector UPL not implemented");
		}

		kr = kmem_suballoc(map, &vector_upl_dst_addr,
		    vector_upl->u_size,
		    FALSE,
		    VM_FLAGS_ANYWHERE, VM_MAP_KERNEL_FLAGS_NONE, VM_KERN_MEMORY_NONE,
		    &vector_upl_submap);
		if (kr != KERN_SUCCESS) {
			panic("Vector UPL submap allocation failed\n");
		}
		map = vector_upl_submap;
		vector_upl_set_submap(vector_upl, vector_upl_submap, vector_upl_dst_addr);
		curr_upl = 0;
	} else {
		upl_lock(upl);
	}

process_upl_to_enter:
	if (isVectorUPL) {
		if (curr_upl == MAX_VECTOR_UPL_ELEMENTS) {
			*dst_addr = vector_upl_dst_addr;
			upl_unlock(vector_upl);
			return KERN_SUCCESS;
		}
		upl =  vector_upl_subupl_byindex(vector_upl, curr_upl++ );
		if (upl == NULL) {
			goto process_upl_to_enter;
		}

		vector_upl_get_iostate(vector_upl, upl, &subupl_offset, &subupl_size);
		*dst_addr = (vm_map_offset_t)(vector_upl_dst_addr + (vm_map_offset_t)subupl_offset);
	} else {
		  
                                   
     
		if (UPL_PAGE_LIST_MAPPED & upl->flags) {
			upl_unlock(upl);
			return KERN_FAILURE;
		}
	}

	size = upl_adjusted_size(upl, VM_MAP_PAGE_MASK(map));

	if ((!(upl->flags & UPL_SHADOWED)) &&
	    ((upl->flags & UPL_HAS_BUSY) ||
	    !((upl->flags & (UPL_DEVICE_MEMORY | UPL_IO_WIRE)) || (upl->map_object->phys_contiguous)))) {
		vm_object_t             object;
		vm_page_t               alias_page;
		vm_object_offset_t      new_offset;
		unsigned int            pg_num;
		wpl_array_t             lite_list;

		if (upl->flags & UPL_INTERNAL) {
			lite_list = (wpl_array_t)
			    ((((uintptr_t)upl) + sizeof(struct upl))
			    + ((size / PAGE_SIZE) * sizeof(upl_page_info_t)));
		} else {
			lite_list = (wpl_array_t)(((uintptr_t)upl) + sizeof(struct upl));
		}
		object = upl->map_object;
		upl->map_object = vm_object_allocate(vm_object_round_page(size));

		vm_object_lock(upl->map_object);

		upl->map_object->shadow = object;
		upl->map_object->pageout = TRUE;
		upl->map_object->can_persist = FALSE;
		upl->map_object->copy_strategy = MEMORY_OBJECT_COPY_NONE;
		upl->map_object->vo_shadow_offset = upl_adjusted_offset(upl, PAGE_MASK) - object->paging_offset;
		assertf(page_aligned(upl->map_object->vo_shadow_offset),
		    "object %p shadow_offset 0x%llx",
		    upl->map_object,
		    (uint64_t)upl->map_object->vo_shadow_offset);
		upl->map_object->wimg_bits = object->wimg_bits;
		assertf(page_aligned(upl->map_object->vo_shadow_offset),
		    "object %p shadow_offset 0x%llx",
		    upl->map_object, upl->map_object->vo_shadow_offset);
		offset = upl->map_object->vo_shadow_offset;
		new_offset = 0;
		size = upl_adjusted_size(upl, VM_MAP_PAGE_MASK(map));

		upl->flags |= UPL_SHADOWED;

		while (size) {
			pg_num = (unsigned int) (new_offset / PAGE_SIZE);
			assert(pg_num == new_offset / PAGE_SIZE);

			if (lite_list[pg_num >> 5] & (1U << (pg_num & 31))) {
				alias_page = vm_page_grab_fictitious(TRUE);

				vm_object_lock(object);

				m = vm_page_lookup(object, offset);
				if (m == VM_PAGE_NULL) {
					panic("vm_upl_map: page missing\n");
				}

				  
                                               
                               
       
				assert(alias_page->vmp_fictitious);
				alias_page->vmp_fictitious = FALSE;
				alias_page->vmp_private = TRUE;
				alias_page->vmp_free_when_done = TRUE;
				  
                                           
                                        
                                             
                        
       
				VM_PAGE_SET_PHYS_PAGE(alias_page, VM_PAGE_GET_PHYS_PAGE(m));

				vm_object_unlock(object);

				vm_page_lockspin_queues();
				vm_page_wire(alias_page, VM_KERN_MEMORY_NONE, TRUE);
				vm_page_unlock_queues();

				vm_page_insert_wired(alias_page, upl->map_object, new_offset, VM_KERN_MEMORY_NONE);

				assert(!alias_page->vmp_wanted);
				alias_page->vmp_busy = FALSE;
				alias_page->vmp_absent = FALSE;
			}
			size -= PAGE_SIZE;
			offset += PAGE_SIZE_64;
			new_offset += PAGE_SIZE_64;
		}
		vm_object_unlock(upl->map_object);
	}
	if (upl->flags & UPL_SHADOWED) {
		offset = 0;
	} else {
		offset = upl_adjusted_offset(upl, VM_MAP_PAGE_MASK(map)) - upl->map_object->paging_offset;
	}

	size = upl_adjusted_size(upl, VM_MAP_PAGE_MASK(map));

	vm_object_reference(upl->map_object);

	if (!isVectorUPL) {
		*dst_addr = 0;
		  
                         
     
		kr = vm_map_enter(map, dst_addr, (vm_map_size_t)size, (vm_map_offset_t) 0,
		    VM_FLAGS_ANYWHERE, VM_MAP_KERNEL_FLAGS_NONE, VM_KERN_MEMORY_OSFMK,
		    upl->map_object, offset, FALSE,
		    VM_PROT_DEFAULT, VM_PROT_ALL, VM_INHERIT_DEFAULT);

		if (kr != KERN_SUCCESS) {
			vm_object_deallocate(upl->map_object);
			upl_unlock(upl);
			return kr;
		}
	} else {
		kr = vm_map_enter(map, dst_addr, (vm_map_size_t)size, (vm_map_offset_t) 0,
		    VM_FLAGS_FIXED, VM_MAP_KERNEL_FLAGS_NONE, VM_KERN_MEMORY_OSFMK,
		    upl->map_object, offset, FALSE,
		    VM_PROT_DEFAULT, VM_PROT_ALL, VM_INHERIT_DEFAULT);
		if (kr) {
			panic("vm_map_enter failed for a Vector UPL\n");
		}
	}
	vm_object_lock(upl->map_object);

	for (addr = *dst_addr; size > 0; size -= PAGE_SIZE, addr += PAGE_SIZE) {
		m = vm_page_lookup(upl->map_object, offset);

		if (m) {
			m->vmp_pmapped = TRUE;

			                                                    
                                                          
                                           
			                             
			assert(map->pmap == kernel_pmap);

			PMAP_ENTER(map->pmap, addr, m, VM_PROT_DEFAULT, VM_PROT_NONE, 0, TRUE, kr);

			assert(kr == KERN_SUCCESS);
#if KASAN
			kasan_notify_address(addr, PAGE_SIZE_64);
#endif
		}
		offset += PAGE_SIZE_64;
	}
	vm_object_unlock(upl->map_object);

	  
                                    
    
	upl->ref_count++;
	upl->flags |= UPL_PAGE_LIST_MAPPED;
	upl->kaddr = (vm_offset_t) *dst_addr;
	assert(upl->kaddr == *dst_addr);

	if (isVectorUPL) {
		goto process_upl_to_enter;
	}

	if (!isVectorUPL) {
		vm_map_offset_t addr_adjustment;

		addr_adjustment = (vm_map_offset_t)(upl->u_offset - upl_adjusted_offset(upl, VM_MAP_PAGE_MASK(map)));
		if (addr_adjustment) {
			assert(VM_MAP_PAGE_MASK(map) != PAGE_MASK);
			DEBUG4K_UPL("dst_addr 0x%llx (+ 0x%llx) -> 0x%llx\n", (uint64_t)*dst_addr, (uint64_t)addr_adjustment, (uint64_t)(*dst_addr + addr_adjustment));
			*dst_addr += addr_adjustment;
		}
	}

	upl_unlock(upl);

	return KERN_SUCCESS;
}

  
                                                          
  
                                                      
                                                           
                                                      
                                                        
                                                      
            
   
kern_return_t
vm_map_remove_upl(
	vm_map_t        map,
	upl_t           upl)
{
	vm_address_t    addr;
	upl_size_t      size;
	int             isVectorUPL = 0, curr_upl = 0;
	upl_t           vector_upl = NULL;

	if (upl == UPL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if ((isVectorUPL = vector_upl_is_valid(upl))) {
		int     unmapped = 0, valid_upls = 0;
		vector_upl = upl;
		upl_lock(vector_upl);
		for (curr_upl = 0; curr_upl < MAX_VECTOR_UPL_ELEMENTS; curr_upl++) {
			upl =  vector_upl_subupl_byindex(vector_upl, curr_upl );
			if (upl == NULL) {
				continue;
			}
			valid_upls++;
			if (!(UPL_PAGE_LIST_MAPPED & upl->flags)) {
				unmapped++;
			}
		}

		if (unmapped) {
			if (unmapped != valid_upls) {
				panic("%d of the %d sub-upls within the Vector UPL is/are not mapped\n", unmapped, valid_upls);
			} else {
				upl_unlock(vector_upl);
				return KERN_FAILURE;
			}
		}
		curr_upl = 0;
	} else {
		upl_lock(upl);
	}

process_upl_to_remove:
	if (isVectorUPL) {
		if (curr_upl == MAX_VECTOR_UPL_ELEMENTS) {
			vm_map_t v_upl_submap;
			vm_offset_t v_upl_submap_dst_addr;
			vector_upl_get_submap(vector_upl, &v_upl_submap, &v_upl_submap_dst_addr);

			vm_map_remove(map, v_upl_submap_dst_addr,
			    v_upl_submap_dst_addr + vector_upl->u_size,
			    VM_MAP_REMOVE_NO_FLAGS);
			vm_map_deallocate(v_upl_submap);
			upl_unlock(vector_upl);
			return KERN_SUCCESS;
		}

		upl =  vector_upl_subupl_byindex(vector_upl, curr_upl++ );
		if (upl == NULL) {
			goto process_upl_to_remove;
		}
	}

	if (upl->flags & UPL_PAGE_LIST_MAPPED) {
		addr = upl->kaddr;
		size = upl_adjusted_size(upl, VM_MAP_PAGE_MASK(map));

		assert(upl->ref_count > 1);
		upl->ref_count--;                                         

		upl->flags &= ~UPL_PAGE_LIST_MAPPED;
		upl->kaddr = (vm_offset_t) 0;

		if (!isVectorUPL) {
			upl_unlock(upl);

			vm_map_remove(
				map,
				vm_map_trunc_page(addr,
				VM_MAP_PAGE_MASK(map)),
				vm_map_round_page(addr + size,
				VM_MAP_PAGE_MASK(map)),
				VM_MAP_REMOVE_NO_FLAGS);
			return KERN_SUCCESS;
		} else {
			  
                                                          
                                                         
                                             
      
			goto process_upl_to_remove;
		}
	}
	upl_unlock(upl);

	return KERN_FAILURE;
}


kern_return_t
upl_commit_range(
	upl_t                   upl,
	upl_offset_t            offset,
	upl_size_t              size,
	int                     flags,
	upl_page_info_t         *page_list,
	mach_msg_type_number_t  count,
	boolean_t               *empty)
{
	upl_size_t              xfer_size, subupl_size;
	vm_object_t             shadow_object;
	vm_object_t             object;
	vm_object_t             m_object;
	vm_object_offset_t      target_offset;
	upl_offset_t            subupl_offset = offset;
	int                     entry;
	wpl_array_t             lite_list;
	int                     occupied;
	int                     clear_refmod = 0;
	int                     pgpgout_count = 0;
	struct  vm_page_delayed_work    dw_array;
	struct  vm_page_delayed_work    *dwp, *dwp_start;
	bool                    dwp_finish_ctx = TRUE;
	int                     dw_count;
	int                     dw_limit;
	int                     isVectorUPL = 0;
	upl_t                   vector_upl = NULL;
	boolean_t               should_be_throttled = FALSE;

	vm_page_t               nxt_page = VM_PAGE_NULL;
	int                     fast_path_possible = 0;
	int                     fast_path_full_commit = 0;
	int                     throttle_page = 0;
	int                     unwired_count = 0;
	int                     local_queue_count = 0;
	vm_page_t               first_local, last_local;
	vm_object_offset_t      obj_start, obj_end, obj_offset;
	kern_return_t           kr = KERN_SUCCESS;

                                                                                                                                                                                                                         

	dwp_start = dwp = NULL;

	subupl_size = size;
	*empty = FALSE;

	if (upl == UPL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	dw_count = 0;
	dw_limit = DELAYED_WORK_LIMIT(DEFAULT_DELAYED_WORK_LIMIT);
	dwp_start = vm_page_delayed_work_get_ctx();
	if (dwp_start == NULL) {
		dwp_start = &dw_array;
		dw_limit = 1;
		dwp_finish_ctx = FALSE;
	}

	dwp = dwp_start;

	if (count == 0) {
		page_list = NULL;
	}

	if ((isVectorUPL = vector_upl_is_valid(upl))) {
		vector_upl = upl;
		upl_lock(vector_upl);
	} else {
		upl_lock(upl);
	}

process_upl_to_commit:

	if (isVectorUPL) {
		size = subupl_size;
		offset = subupl_offset;
		if (size == 0) {
			upl_unlock(vector_upl);
			kr = KERN_SUCCESS;
			goto done;
		}
		upl =  vector_upl_subupl_byoffset(vector_upl, &offset, &size);
		if (upl == NULL) {
			upl_unlock(vector_upl);
			kr = KERN_FAILURE;
			goto done;
		}
		page_list = UPL_GET_INTERNAL_PAGE_LIST_SIMPLE(upl);
		subupl_size -= size;
		subupl_offset += size;
	}

#if UPL_DEBUG
	if (upl->upl_commit_index < UPL_DEBUG_COMMIT_RECORDS) {
		(void) OSBacktrace(&upl->upl_commit_records[upl->upl_commit_index].c_retaddr[0], UPL_DEBUG_STACK_FRAMES);

		upl->upl_commit_records[upl->upl_commit_index].c_beg = offset;
		upl->upl_commit_records[upl->upl_commit_index].c_end = (offset + size);

		upl->upl_commit_index++;
	}
#endif
	if (upl->flags & UPL_DEVICE_MEMORY) {
		xfer_size = 0;
	} else if ((offset + size) <= upl_adjusted_size(upl, PAGE_MASK)) {
		xfer_size = size;
	} else {
		if (!isVectorUPL) {
			upl_unlock(upl);
		} else {
			upl_unlock(vector_upl);
		}
		DEBUG4K_ERROR("upl %p (u_offset 0x%llx u_size 0x%x) offset 0x%x size 0x%x\n", upl, upl->u_offset, upl->u_size, offset, size);
		kr = KERN_FAILURE;
		goto done;
	}
	if (upl->flags & UPL_SET_DIRTY) {
		flags |= UPL_COMMIT_SET_DIRTY;
	}
	if (upl->flags & UPL_CLEAR_DIRTY) {
		flags |= UPL_COMMIT_CLEAR_DIRTY;
	}

	if (upl->flags & UPL_INTERNAL) {
		lite_list = (wpl_array_t) ((((uintptr_t)upl) + sizeof(struct upl))
		    + ((upl_adjusted_size(upl, PAGE_MASK) / PAGE_SIZE) * sizeof(upl_page_info_t)));
	} else {
		lite_list = (wpl_array_t) (((uintptr_t)upl) + sizeof(struct upl));
	}

	object = upl->map_object;

	if (upl->flags & UPL_SHADOWED) {
		vm_object_lock(object);
		shadow_object = object->shadow;
	} else {
		shadow_object = object;
	}
	entry = offset / PAGE_SIZE;
	target_offset = (vm_object_offset_t)offset;

	if (upl->flags & UPL_KERNEL_OBJECT) {
		vm_object_lock_shared(shadow_object);
	} else {
		vm_object_lock(shadow_object);
	}

	VM_OBJECT_WIRED_PAGE_UPDATE_START(shadow_object);

	if (upl->flags & UPL_ACCESS_BLOCKED) {
		assert(shadow_object->blocked_access);
		shadow_object->blocked_access = FALSE;
		vm_object_wakeup(object, VM_OBJECT_EVENT_UNBLOCKED);
	}

	if (shadow_object->code_signed) {
		  
                  
                                                           
                                                        
                                                            
             
     
		flags &= ~UPL_COMMIT_CS_VALIDATED;
	}
	if (!page_list) {
		  
                                                      
     
		flags &= ~UPL_COMMIT_CS_VALIDATED;
	}
	if (!VM_DYNAMIC_PAGING_ENABLED() && shadow_object->internal) {
		should_be_throttled = TRUE;
	}

	if ((upl->flags & UPL_IO_WIRE) &&
	    !(flags & UPL_COMMIT_FREE_ABSENT) &&
	    !isVectorUPL &&
	    shadow_object->purgable != VM_PURGABLE_VOLATILE &&
	    shadow_object->purgable != VM_PURGABLE_EMPTY) {
		if (!vm_page_queue_empty(&shadow_object->memq)) {
			if (size == shadow_object->vo_size) {
				nxt_page = (vm_page_t)vm_page_queue_first(&shadow_object->memq);
				fast_path_full_commit = 1;
			}
			fast_path_possible = 1;

			if (!VM_DYNAMIC_PAGING_ENABLED() && shadow_object->internal &&
			    (shadow_object->purgable == VM_PURGABLE_DENY ||
			    shadow_object->purgable == VM_PURGABLE_NONVOLATILE ||
			    shadow_object->purgable == VM_PURGABLE_VOLATILE)) {
				throttle_page = 1;
			}
		}
	}
	first_local = VM_PAGE_NULL;
	last_local = VM_PAGE_NULL;

	obj_start = target_offset + upl->u_offset - shadow_object->paging_offset;
	obj_end = obj_start + xfer_size;
	obj_start = vm_object_trunc_page(obj_start);
	obj_end = vm_object_round_page(obj_end);
	for (obj_offset = obj_start;
	    obj_offset < obj_end;
	    obj_offset += PAGE_SIZE) {
		vm_page_t       t, m;

		dwp->dw_mask = 0;
		clear_refmod = 0;

		m = VM_PAGE_NULL;

		if (upl->flags & UPL_LITE) {
			unsigned int    pg_num;

			if (nxt_page != VM_PAGE_NULL) {
				m = nxt_page;
				nxt_page = (vm_page_t)vm_page_queue_next(&nxt_page->vmp_listq);
				target_offset = m->vmp_offset;
			}
			pg_num = (unsigned int) (target_offset / PAGE_SIZE);
			assert(pg_num == target_offset / PAGE_SIZE);

			if (lite_list[pg_num >> 5] & (1U << (pg_num & 31))) {
				lite_list[pg_num >> 5] &= ~(1U << (pg_num & 31));

				if (!(upl->flags & UPL_KERNEL_OBJECT) && m == VM_PAGE_NULL) {
					m = vm_page_lookup(shadow_object, obj_offset);
				}
			} else {
				m = NULL;
			}
		}
		if (upl->flags & UPL_SHADOWED) {
			if ((t = vm_page_lookup(object, target_offset)) != VM_PAGE_NULL) {
				t->vmp_free_when_done = FALSE;

				VM_PAGE_FREE(t);

				if (!(upl->flags & UPL_KERNEL_OBJECT) && m == VM_PAGE_NULL) {
					m = vm_page_lookup(shadow_object, target_offset + object->vo_shadow_offset);
				}
			}
		}
		if (m == VM_PAGE_NULL) {
			goto commit_next_page;
		}

		m_object = VM_PAGE_OBJECT(m);

		if (m->vmp_q_state == VM_PAGE_USED_BY_COMPRESSOR) {
			assert(m->vmp_busy);

			dwp->dw_mask |= (DW_clear_busy | DW_PAGE_WAKEUP);
			goto commit_next_page;
		}

		if (flags & UPL_COMMIT_CS_VALIDATED) {
			  
                   
                                            
                                       
      
			m->vmp_cs_validated |= page_list[entry].cs_validated;
			m->vmp_cs_tainted |= page_list[entry].cs_tainted;
			m->vmp_cs_nx |= page_list[entry].cs_nx;
		}
		if (flags & UPL_COMMIT_WRITTEN_BY_KERNEL) {
			m->vmp_written_by_kernel = TRUE;
		}

		if (upl->flags & UPL_IO_WIRE) {
			if (page_list) {
				page_list[entry].phys_addr = 0;
			}

			if (flags & UPL_COMMIT_SET_DIRTY) {
				SET_PAGE_DIRTY(m, FALSE);
			} else if (flags & UPL_COMMIT_CLEAR_DIRTY) {
				m->vmp_dirty = FALSE;

				if (!(flags & UPL_COMMIT_CS_VALIDATED) &&
				    m->vmp_cs_validated &&
				    m->vmp_cs_tainted != VMP_CS_ALL_TRUE) {
					  
                     
                                    
                                     
                             
                     
        
					m->vmp_cs_validated = VMP_CS_ALL_FALSE;

					VM_PAGEOUT_DEBUG(vm_cs_validated_resets, 1);

					pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m));
				}
				clear_refmod |= VM_MEM_MODIFIED;
			}
			if (upl->flags & UPL_ACCESS_BLOCKED) {
				  
                                                  
                                                  
                     
       
				dwp->dw_mask |= (DW_clear_busy | DW_PAGE_WAKEUP);
			}
			if (fast_path_possible) {
				assert(m_object->purgable != VM_PURGABLE_EMPTY);
				assert(m_object->purgable != VM_PURGABLE_VOLATILE);
				if (m->vmp_absent) {
					assert(m->vmp_q_state == VM_PAGE_NOT_ON_Q);
					assert(m->vmp_wire_count == 0);
					assert(m->vmp_busy);

					m->vmp_absent = FALSE;
					dwp->dw_mask |= (DW_clear_busy | DW_PAGE_WAKEUP);
				} else {
					if (m->vmp_wire_count == 0) {
						panic("wire_count == 0, m = %p, obj = %p\n", m, shadow_object);
					}
					assert(m->vmp_q_state == VM_PAGE_IS_WIRED);

					  
                                          
                                             
                      
        
					assert(m->vmp_wire_count > 0);
					m->vmp_wire_count--;

					if (m->vmp_wire_count == 0) {
						m->vmp_q_state = VM_PAGE_NOT_ON_Q;
						unwired_count++;
					}
				}
				if (m->vmp_wire_count == 0) {
					assert(m->vmp_pageq.next == 0 && m->vmp_pageq.prev == 0);

					if (last_local == VM_PAGE_NULL) {
						assert(first_local == VM_PAGE_NULL);

						last_local = m;
						first_local = m;
					} else {
						assert(first_local != VM_PAGE_NULL);

						m->vmp_pageq.next = VM_PAGE_CONVERT_TO_QUEUE_ENTRY(first_local);
						first_local->vmp_pageq.prev = VM_PAGE_CONVERT_TO_QUEUE_ENTRY(m);
						first_local = m;
					}
					local_queue_count++;

					if (throttle_page) {
						m->vmp_q_state = VM_PAGE_ON_THROTTLED_Q;
					} else {
						if (flags & UPL_COMMIT_INACTIVATE) {
							if (shadow_object->internal) {
								m->vmp_q_state = VM_PAGE_ON_INACTIVE_INTERNAL_Q;
							} else {
								m->vmp_q_state = VM_PAGE_ON_INACTIVE_EXTERNAL_Q;
							}
						} else {
							m->vmp_q_state = VM_PAGE_ON_ACTIVE_Q;
						}
					}
				}
			} else {
				if (flags & UPL_COMMIT_INACTIVATE) {
					dwp->dw_mask |= DW_vm_page_deactivate_internal;
					clear_refmod |= VM_MEM_REFERENCED;
				}
				if (m->vmp_absent) {
					if (flags & UPL_COMMIT_FREE_ABSENT) {
						dwp->dw_mask |= DW_vm_page_free;
					} else {
						m->vmp_absent = FALSE;
						dwp->dw_mask |= (DW_clear_busy | DW_PAGE_WAKEUP);

						if (!(dwp->dw_mask & DW_vm_page_deactivate_internal)) {
							dwp->dw_mask |= DW_vm_page_activate;
						}
					}
				} else {
					dwp->dw_mask |= DW_vm_page_unwire;
				}
			}
			goto commit_next_page;
		}
		assert(m->vmp_q_state != VM_PAGE_USED_BY_COMPRESSOR);

		if (page_list) {
			page_list[entry].phys_addr = 0;
		}

		  
                                    
                                    
                                        
                                          
                    
     
		if (flags & UPL_COMMIT_CLEAR_DIRTY) {
			m->vmp_dirty = FALSE;

			clear_refmod |= VM_MEM_MODIFIED;
		}
		if (m->vmp_laundry) {
			dwp->dw_mask |= DW_vm_pageout_throttle_up;
		}

		if (VM_PAGE_WIRED(m)) {
			m->vmp_free_when_done = FALSE;
		}

		if (!(flags & UPL_COMMIT_CS_VALIDATED) &&
		    m->vmp_cs_validated &&
		    m->vmp_cs_tainted != VMP_CS_ALL_TRUE) {
			  
                   
                                  
                                   
                           
                   
      
			m->vmp_cs_validated = VMP_CS_ALL_FALSE;

			VM_PAGEOUT_DEBUG(vm_cs_validated_resets, 1);

			pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m));
		}
		if (m->vmp_overwriting) {
			  
                                                         
      
			if (m->vmp_busy) {
#if CONFIG_PHANTOM_CACHE
				if (m->vmp_absent && !m_object->internal) {
					dwp->dw_mask |= DW_vm_phantom_cache_update;
				}
#endif
				m->vmp_absent = FALSE;

				dwp->dw_mask |= DW_clear_busy;
			} else {
				  
                                                        
                                              
                                      
       
				assert(VM_PAGE_WIRED(m));

				dwp->dw_mask |= DW_vm_page_unwire;                  
			}
			m->vmp_overwriting = FALSE;
		}
		m->vmp_cleaning = FALSE;

		if (m->vmp_free_when_done) {
			  
                                                      
                                                     
                         
     
                                                    
                                                    
                                                         
                                                      
             
     
                                          
      
			assert(!m_object->internal);

			m->vmp_free_when_done = FALSE;

			if ((flags & UPL_COMMIT_SET_DIRTY) ||
			    (m->vmp_pmapped && (pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m)) & VM_MEM_MODIFIED))) {
				  
                                           
                                         
                                        
                                         
       
				SET_PAGE_DIRTY(m, FALSE);

				dwp->dw_mask |= DW_vm_page_activate | DW_PAGE_WAKEUP;

				if (upl->flags & UPL_PAGEOUT) {
					counter_inc(&vm_statistics_reactivations);
					DTRACE_VM2(pgrec, int, 1, (uint64_t *), NULL);
				}
			} else {
				  
                                         
                                         
       
				if (m_object->internal) {
					DTRACE_VM2(anonpgout, int, 1, (uint64_t *), NULL);
				} else {
					DTRACE_VM2(fspgout, int, 1, (uint64_t *), NULL);
				}
				m->vmp_dirty = FALSE;
				m->vmp_busy = TRUE;

				dwp->dw_mask |= DW_vm_page_free;
			}
			goto commit_next_page;
		}
		  
                                                 
                                          
                                              
                                               
                     
     
		if ((upl->flags & UPL_PAGE_SYNC_DONE) || (flags & UPL_COMMIT_CLEAR_PRECIOUS)) {
			m->vmp_precious = FALSE;
		}

		if (flags & UPL_COMMIT_SET_DIRTY) {
			SET_PAGE_DIRTY(m, FALSE);
		} else {
			m->vmp_dirty = FALSE;
		}

		                                                                          
		if (hibernate_cleaning_in_progress == FALSE && !m->vmp_dirty && (upl->flags & UPL_PAGEOUT)) {
			pgpgout_count++;

			counter_inc(&vm_statistics_pageouts);
			DTRACE_VM2(pgout, int, 1, (uint64_t *), NULL);

			dwp->dw_mask |= DW_enqueue_cleaned;
		} else if (should_be_throttled == TRUE && (m->vmp_q_state == VM_PAGE_NOT_ON_Q)) {
			  
                                                
                                                      
                                                             
                                            
      
			SET_PAGE_DIRTY(m, FALSE);
			dwp->dw_mask |= DW_vm_page_activate;
		} else {
			if ((flags & UPL_COMMIT_INACTIVATE) && !m->vmp_clustered && (m->vmp_q_state != VM_PAGE_ON_SPECULATIVE_Q)) {
				dwp->dw_mask |= DW_vm_page_deactivate_internal;
				clear_refmod |= VM_MEM_REFERENCED;
			} else if (!VM_PAGE_PAGEABLE(m)) {
				if (m->vmp_clustered || (flags & UPL_COMMIT_SPECULATE)) {
					dwp->dw_mask |= DW_vm_page_speculate;
				} else if (m->vmp_reference) {
					dwp->dw_mask |= DW_vm_page_activate;
				} else {
					dwp->dw_mask |= DW_vm_page_deactivate_internal;
					clear_refmod |= VM_MEM_REFERENCED;
				}
			}
		}
		if (upl->flags & UPL_ACCESS_BLOCKED) {
			  
                                                 
                                                 
                         
      
			dwp->dw_mask |= DW_clear_busy;
		}
		  
                                                              
     
		dwp->dw_mask |= DW_PAGE_WAKEUP;

commit_next_page:
		if (clear_refmod) {
			pmap_clear_refmod(VM_PAGE_GET_PHYS_PAGE(m), clear_refmod);
		}

		target_offset += PAGE_SIZE_64;
		xfer_size -= PAGE_SIZE;
		entry++;

		if (dwp->dw_mask) {
			if (dwp->dw_mask & ~(DW_clear_busy | DW_PAGE_WAKEUP)) {
				VM_PAGE_ADD_DELAYED_WORK(dwp, m, dw_count);

				if (dw_count >= dw_limit) {
					vm_page_do_delayed_work(shadow_object, VM_KERN_MEMORY_NONE, dwp_start, dw_count);

					dwp = dwp_start;
					dw_count = 0;
				}
			} else {
				if (dwp->dw_mask & DW_clear_busy) {
					m->vmp_busy = FALSE;
				}

				if (dwp->dw_mask & DW_PAGE_WAKEUP) {
					PAGE_WAKEUP(m);
				}
			}
		}
	}
	if (dw_count) {
		vm_page_do_delayed_work(shadow_object, VM_KERN_MEMORY_NONE, dwp_start, dw_count);
		dwp = dwp_start;
		dw_count = 0;
	}

	if (fast_path_possible) {
		assert(shadow_object->purgable != VM_PURGABLE_VOLATILE);
		assert(shadow_object->purgable != VM_PURGABLE_EMPTY);

		if (local_queue_count || unwired_count) {
			if (local_queue_count) {
				vm_page_t       first_target;
				vm_page_queue_head_t    *target_queue;

				if (throttle_page) {
					target_queue = &vm_page_queue_throttled;
				} else {
					if (flags & UPL_COMMIT_INACTIVATE) {
						if (shadow_object->internal) {
							target_queue = &vm_page_queue_anonymous;
						} else {
							target_queue = &vm_page_queue_inactive;
						}
					} else {
						target_queue = &vm_page_queue_active;
					}
				}
				  
                                                                    
       
				vm_page_lockspin_queues();

				first_target = (vm_page_t) vm_page_queue_first(target_queue);

				if (vm_page_queue_empty(target_queue)) {
					target_queue->prev = VM_PAGE_CONVERT_TO_QUEUE_ENTRY(last_local);
				} else {
					first_target->vmp_pageq.prev = VM_PAGE_CONVERT_TO_QUEUE_ENTRY(last_local);
				}

				target_queue->next = VM_PAGE_CONVERT_TO_QUEUE_ENTRY(first_local);
				first_local->vmp_pageq.prev = VM_PAGE_CONVERT_TO_QUEUE_ENTRY(target_queue);
				last_local->vmp_pageq.next = VM_PAGE_CONVERT_TO_QUEUE_ENTRY(first_target);

				  
                                     
       
				if (throttle_page) {
					vm_page_throttled_count += local_queue_count;
				} else {
					if (flags & UPL_COMMIT_INACTIVATE) {
						if (shadow_object->internal) {
							vm_page_anonymous_count += local_queue_count;
						}
						vm_page_inactive_count += local_queue_count;

						token_new_pagecount += local_queue_count;
					} else {
						vm_page_active_count += local_queue_count;
					}

					if (shadow_object->internal) {
						vm_page_pageable_internal_count += local_queue_count;
					} else {
						vm_page_pageable_external_count += local_queue_count;
					}
				}
			} else {
				vm_page_lockspin_queues();
			}
			if (unwired_count) {
				vm_page_wire_count -= unwired_count;
				VM_CHECK_MEMORYSTATUS;
			}
			vm_page_unlock_queues();

			VM_OBJECT_WIRED_PAGE_COUNT(shadow_object, -unwired_count);
		}
	}
	occupied = 1;

	if (upl->flags & UPL_DEVICE_MEMORY) {
		occupied = 0;
	} else if (upl->flags & UPL_LITE) {
		int     pg_num;
		int     i;

		occupied = 0;

		if (!fast_path_full_commit) {
			pg_num = upl_adjusted_size(upl, PAGE_MASK) / PAGE_SIZE;
			pg_num = (pg_num + 31) >> 5;

			for (i = 0; i < pg_num; i++) {
				if (lite_list[i] != 0) {
					occupied = 1;
					break;
				}
			}
		}
	} else {
		if (vm_page_queue_empty(&upl->map_object->memq)) {
			occupied = 0;
		}
	}
	if (occupied == 0) {
		  
                                                       
                                                         
                                                      
                                                             
                                                             
                       
     
		if ((upl->flags & UPL_COMMIT_NOTIFY_EMPTY) || isVectorUPL) {
			*empty = TRUE;
		}

		if (object == shadow_object && !(upl->flags & UPL_KERNEL_OBJECT)) {
			  
                                 
                                             
                                            
                         
      
			vm_object_activity_end(shadow_object);
			vm_object_collapse(shadow_object, 0, TRUE);
		} else {
			  
                                         
                                                   
                              
      
		}
	}
	VM_OBJECT_WIRED_PAGE_UPDATE_END(shadow_object, shadow_object->wire_tag);
	vm_object_unlock(shadow_object);
	if (object != shadow_object) {
		vm_object_unlock(object);
	}

	if (!isVectorUPL) {
		upl_unlock(upl);
	} else {
		  
                                                     
                                                      
                                                        
                                                           
                                                       
                                                          
                                                    
     
		if (*empty == TRUE) {
			*empty = vector_upl_set_subupl(vector_upl, upl, 0);
			upl_deallocate(upl);
		}
		goto process_upl_to_commit;
	}
	if (pgpgout_count) {
		DTRACE_VM2(pgpgout, int, pgpgout_count, (uint64_t *), NULL);
	}

	kr = KERN_SUCCESS;
done:
	if (dwp_start && dwp_finish_ctx) {
		vm_page_delayed_work_finish_ctx(dwp_start);
		dwp_start = dwp = NULL;
	}

	return kr;
}

kern_return_t
upl_abort_range(
	upl_t                   upl,
	upl_offset_t            offset,
	upl_size_t              size,
	int                     error,
	boolean_t               *empty)
{
	upl_page_info_t         *user_page_list = NULL;
	upl_size_t              xfer_size, subupl_size;
	vm_object_t             shadow_object;
	vm_object_t             object;
	vm_object_offset_t      target_offset;
	upl_offset_t            subupl_offset = offset;
	int                     entry;
	wpl_array_t             lite_list;
	int                     occupied;
	struct  vm_page_delayed_work    dw_array;
	struct  vm_page_delayed_work    *dwp, *dwp_start;
	bool                    dwp_finish_ctx = TRUE;
	int                     dw_count;
	int                     dw_limit;
	int                     isVectorUPL = 0;
	upl_t                   vector_upl = NULL;
	vm_object_offset_t      obj_start, obj_end, obj_offset;
	kern_return_t           kr = KERN_SUCCESS;

                                                                                                                                                                                                                         

	dwp_start = dwp = NULL;

	subupl_size = size;
	*empty = FALSE;

	if (upl == UPL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	if ((upl->flags & UPL_IO_WIRE) && !(error & UPL_ABORT_DUMP_PAGES)) {
		return upl_commit_range(upl, offset, size, UPL_COMMIT_FREE_ABSENT, NULL, 0, empty);
	}

	dw_count = 0;
	dw_limit = DELAYED_WORK_LIMIT(DEFAULT_DELAYED_WORK_LIMIT);
	dwp_start = vm_page_delayed_work_get_ctx();
	if (dwp_start == NULL) {
		dwp_start = &dw_array;
		dw_limit = 1;
		dwp_finish_ctx = FALSE;
	}

	dwp = dwp_start;

	if ((isVectorUPL = vector_upl_is_valid(upl))) {
		vector_upl = upl;
		upl_lock(vector_upl);
	} else {
		upl_lock(upl);
	}

process_upl_to_abort:
	if (isVectorUPL) {
		size = subupl_size;
		offset = subupl_offset;
		if (size == 0) {
			upl_unlock(vector_upl);
			kr = KERN_SUCCESS;
			goto done;
		}
		upl =  vector_upl_subupl_byoffset(vector_upl, &offset, &size);
		if (upl == NULL) {
			upl_unlock(vector_upl);
			kr = KERN_FAILURE;
			goto done;
		}
		subupl_size -= size;
		subupl_offset += size;
	}

	*empty = FALSE;

#if UPL_DEBUG
	if (upl->upl_commit_index < UPL_DEBUG_COMMIT_RECORDS) {
		(void) OSBacktrace(&upl->upl_commit_records[upl->upl_commit_index].c_retaddr[0], UPL_DEBUG_STACK_FRAMES);

		upl->upl_commit_records[upl->upl_commit_index].c_beg = offset;
		upl->upl_commit_records[upl->upl_commit_index].c_end = (offset + size);
		upl->upl_commit_records[upl->upl_commit_index].c_aborted = 1;

		upl->upl_commit_index++;
	}
#endif
	if (upl->flags & UPL_DEVICE_MEMORY) {
		xfer_size = 0;
	} else if ((offset + size) <= upl_adjusted_size(upl, PAGE_MASK)) {
		xfer_size = size;
	} else {
		if (!isVectorUPL) {
			upl_unlock(upl);
		} else {
			upl_unlock(vector_upl);
		}
		DEBUG4K_ERROR("upl %p (u_offset 0x%llx u_size 0x%x) offset 0x%x size 0x%x\n", upl, upl->u_offset, upl->u_size, offset, size);
		kr = KERN_FAILURE;
		goto done;
	}
	if (upl->flags & UPL_INTERNAL) {
		lite_list = (wpl_array_t)
		    ((((uintptr_t)upl) + sizeof(struct upl))
		    + ((upl_adjusted_size(upl, PAGE_MASK) / PAGE_SIZE) * sizeof(upl_page_info_t)));

		user_page_list = (upl_page_info_t *) (((uintptr_t)upl) + sizeof(struct upl));
	} else {
		lite_list = (wpl_array_t)
		    (((uintptr_t)upl) + sizeof(struct upl));
	}
	object = upl->map_object;

	if (upl->flags & UPL_SHADOWED) {
		vm_object_lock(object);
		shadow_object = object->shadow;
	} else {
		shadow_object = object;
	}

	entry = offset / PAGE_SIZE;
	target_offset = (vm_object_offset_t)offset;

	if (upl->flags & UPL_KERNEL_OBJECT) {
		vm_object_lock_shared(shadow_object);
	} else {
		vm_object_lock(shadow_object);
	}

	if (upl->flags & UPL_ACCESS_BLOCKED) {
		assert(shadow_object->blocked_access);
		shadow_object->blocked_access = FALSE;
		vm_object_wakeup(object, VM_OBJECT_EVENT_UNBLOCKED);
	}

	if ((error & UPL_ABORT_DUMP_PAGES) && (upl->flags & UPL_KERNEL_OBJECT)) {
		panic("upl_abort_range: kernel_object being DUMPED");
	}

	obj_start = target_offset + upl->u_offset - shadow_object->paging_offset;
	obj_end = obj_start + xfer_size;
	obj_start = vm_object_trunc_page(obj_start);
	obj_end = vm_object_round_page(obj_end);
	for (obj_offset = obj_start;
	    obj_offset < obj_end;
	    obj_offset += PAGE_SIZE) {
		vm_page_t       t, m;
		unsigned int    pg_num;
		boolean_t       needed;

		pg_num = (unsigned int) (target_offset / PAGE_SIZE);
		assert(pg_num == target_offset / PAGE_SIZE);

		needed = FALSE;

		if (user_page_list) {
			needed = user_page_list[pg_num].needed;
		}

		dwp->dw_mask = 0;
		m = VM_PAGE_NULL;

		if (upl->flags & UPL_LITE) {
			if (lite_list[pg_num >> 5] & (1U << (pg_num & 31))) {
				lite_list[pg_num >> 5] &= ~(1U << (pg_num & 31));

				if (!(upl->flags & UPL_KERNEL_OBJECT)) {
					m = vm_page_lookup(shadow_object, obj_offset);
				}
			}
		}
		if (upl->flags & UPL_SHADOWED) {
			if ((t = vm_page_lookup(object, target_offset)) != VM_PAGE_NULL) {
				t->vmp_free_when_done = FALSE;

				VM_PAGE_FREE(t);

				if (m == VM_PAGE_NULL) {
					m = vm_page_lookup(shadow_object, target_offset + object->vo_shadow_offset);
				}
			}
		}
		if ((upl->flags & UPL_KERNEL_OBJECT)) {
			goto abort_next_page;
		}

		if (m != VM_PAGE_NULL) {
			assert(m->vmp_q_state != VM_PAGE_USED_BY_COMPRESSOR);

			if (m->vmp_absent) {
				boolean_t must_free = TRUE;

				  
                           
                                            
                                           
       
				if (error & UPL_ABORT_RESTART) {
					m->vmp_restart = TRUE;
					m->vmp_absent = FALSE;
					m->vmp_unusual = TRUE;
					must_free = FALSE;
				} else if (error & UPL_ABORT_UNAVAILABLE) {
					m->vmp_restart = FALSE;
					m->vmp_unusual = TRUE;
					must_free = FALSE;
				} else if (error & UPL_ABORT_ERROR) {
					m->vmp_restart = FALSE;
					m->vmp_absent = FALSE;
					m->vmp_error = TRUE;
					m->vmp_unusual = TRUE;
					must_free = FALSE;
				}
				if (m->vmp_clustered && needed == FALSE) {
					  
                                             
                                          
                                         
                                         
                                            
                      
                                
        
					must_free = TRUE;
				}
				m->vmp_cleaning = FALSE;

				if (m->vmp_overwriting && !m->vmp_busy) {
					  
                                   
                                     
                                    
                                 
                               
                                         
                                        
                                             
                                            
        
					dwp->dw_mask |= DW_vm_page_unwire;
				}
				m->vmp_overwriting = FALSE;

				dwp->dw_mask |= (DW_clear_busy | DW_PAGE_WAKEUP);

				if (must_free == TRUE) {
					dwp->dw_mask |= DW_vm_page_free;
				} else {
					dwp->dw_mask |= DW_vm_page_activate;
				}
			} else {
				  
                                         
       
				if (m->vmp_laundry) {
					dwp->dw_mask |= DW_vm_pageout_throttle_up;
				}

				if (upl->flags & UPL_ACCESS_BLOCKED) {
					  
                                                   
                                                   
                      
        
					dwp->dw_mask |= DW_clear_busy;
				}
				if (m->vmp_overwriting) {
					if (m->vmp_busy) {
						dwp->dw_mask |= DW_clear_busy;
					} else {
						  
                                         
                                   
                                     
                                     
                                         
                                              
                                             
         
						dwp->dw_mask |= DW_vm_page_unwire;
					}
					m->vmp_overwriting = FALSE;
				}
				m->vmp_free_when_done = FALSE;
				m->vmp_cleaning = FALSE;

				if (error & UPL_ABORT_DUMP_PAGES) {
					pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(m));

					dwp->dw_mask |= DW_vm_page_free;
				} else {
					if (!(dwp->dw_mask & DW_vm_page_unwire)) {
						if (error & UPL_ABORT_REFERENCE) {
							  
                                      
                                    
                                   
                                               
          
							dwp->dw_mask |= DW_vm_page_lru;
						} else if (!VM_PAGE_PAGEABLE(m)) {
							dwp->dw_mask |= DW_vm_page_deactivate_internal;
						}
					}
					dwp->dw_mask |= DW_PAGE_WAKEUP;
				}
			}
		}
abort_next_page:
		target_offset += PAGE_SIZE_64;
		xfer_size -= PAGE_SIZE;
		entry++;

		if (dwp->dw_mask) {
			if (dwp->dw_mask & ~(DW_clear_busy | DW_PAGE_WAKEUP)) {
				VM_PAGE_ADD_DELAYED_WORK(dwp, m, dw_count);

				if (dw_count >= dw_limit) {
					vm_page_do_delayed_work(shadow_object, VM_KERN_MEMORY_NONE, dwp_start, dw_count);

					dwp = dwp_start;
					dw_count = 0;
				}
			} else {
				if (dwp->dw_mask & DW_clear_busy) {
					m->vmp_busy = FALSE;
				}

				if (dwp->dw_mask & DW_PAGE_WAKEUP) {
					PAGE_WAKEUP(m);
				}
			}
		}
	}
	if (dw_count) {
		vm_page_do_delayed_work(shadow_object, VM_KERN_MEMORY_NONE, dwp_start, dw_count);
		dwp = dwp_start;
		dw_count = 0;
	}

	occupied = 1;

	if (upl->flags & UPL_DEVICE_MEMORY) {
		occupied = 0;
	} else if (upl->flags & UPL_LITE) {
		int     pg_num;
		int     i;

		pg_num = upl_adjusted_size(upl, PAGE_MASK) / PAGE_SIZE;
		pg_num = (pg_num + 31) >> 5;
		occupied = 0;

		for (i = 0; i < pg_num; i++) {
			if (lite_list[i] != 0) {
				occupied = 1;
				break;
			}
		}
	} else {
		if (vm_page_queue_empty(&upl->map_object->memq)) {
			occupied = 0;
		}
	}
	if (occupied == 0) {
		  
                                                       
                                                         
                                                      
                                                             
                                                         
                           
     
		if ((upl->flags & UPL_COMMIT_NOTIFY_EMPTY) || isVectorUPL) {
			*empty = TRUE;
		}

		if (object == shadow_object && !(upl->flags & UPL_KERNEL_OBJECT)) {
			  
                                 
                                             
                                            
                         
      
			vm_object_activity_end(shadow_object);
			vm_object_collapse(shadow_object, 0, TRUE);
		} else {
			  
                                         
                                                   
                              
      
		}
	}
	vm_object_unlock(shadow_object);
	if (object != shadow_object) {
		vm_object_unlock(object);
	}

	if (!isVectorUPL) {
		upl_unlock(upl);
	} else {
		  
                                                     
                                                      
                                                        
                                                           
                                                       
                                                        
                                                    
     
		if (*empty == TRUE) {
			*empty = vector_upl_set_subupl(vector_upl, upl, 0);
			upl_deallocate(upl);
		}
		goto process_upl_to_abort;
	}

	kr = KERN_SUCCESS;

done:
	if (dwp_start && dwp_finish_ctx) {
		vm_page_delayed_work_finish_ctx(dwp_start);
		dwp_start = dwp = NULL;
	}

	return kr;
}


kern_return_t
upl_abort(
	upl_t   upl,
	int     error)
{
	boolean_t       empty;

	if (upl == UPL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return upl_abort_range(upl, 0, upl->u_size, error, &empty);
}


                                        
kern_return_t
upl_commit(
	upl_t                   upl,
	upl_page_info_t         *page_list,
	mach_msg_type_number_t  count)
{
	boolean_t       empty;

	if (upl == UPL_NULL) {
		return KERN_INVALID_ARGUMENT;
	}

	return upl_commit_range(upl, 0, upl->u_size, 0,
	           page_list, count, &empty);
}


void
iopl_valid_data(
	upl_t    upl,
	vm_tag_t tag)
{
	vm_object_t     object;
	vm_offset_t     offset;
	vm_page_t       m, nxt_page = VM_PAGE_NULL;
	upl_size_t      size;
	int             wired_count = 0;

	if (upl == NULL) {
		panic("iopl_valid_data: NULL upl");
	}
	if (vector_upl_is_valid(upl)) {
		panic("iopl_valid_data: vector upl");
	}
	if ((upl->flags & (UPL_DEVICE_MEMORY | UPL_SHADOWED | UPL_ACCESS_BLOCKED | UPL_IO_WIRE | UPL_INTERNAL)) != UPL_IO_WIRE) {
		panic("iopl_valid_data: unsupported upl, flags = %x", upl->flags);
	}

	object = upl->map_object;

	if (object == kernel_object || object == compressor_object) {
		panic("iopl_valid_data: object == kernel or compressor");
	}

	if (object->purgable == VM_PURGABLE_VOLATILE ||
	    object->purgable == VM_PURGABLE_EMPTY) {
		panic("iopl_valid_data: object %p purgable %d",
		    object, object->purgable);
	}

	size = upl_adjusted_size(upl, PAGE_MASK);

	vm_object_lock(object);
	VM_OBJECT_WIRED_PAGE_UPDATE_START(object);

	if (object->vo_size == size && object->resident_page_count == (size / PAGE_SIZE)) {
		nxt_page = (vm_page_t)vm_page_queue_first(&object->memq);
	} else {
		offset = (vm_offset_t)(upl_adjusted_offset(upl, PAGE_MASK) - object->paging_offset);
	}

	while (size) {
		if (nxt_page != VM_PAGE_NULL) {
			m = nxt_page;
			nxt_page = (vm_page_t)vm_page_queue_next(&nxt_page->vmp_listq);
		} else {
			m = vm_page_lookup(object, offset);
			offset += PAGE_SIZE;

			if (m == VM_PAGE_NULL) {
				panic("iopl_valid_data: missing expected page at offset %lx", (long)offset);
			}
		}
		if (m->vmp_busy) {
			if (!m->vmp_absent) {
				panic("iopl_valid_data: busy page w/o absent");
			}

			if (m->vmp_pageq.next || m->vmp_pageq.prev) {
				panic("iopl_valid_data: busy+absent page on page queue");
			}
			if (m->vmp_reusable) {
				panic("iopl_valid_data: %p is reusable", m);
			}

			m->vmp_absent = FALSE;
			m->vmp_dirty = TRUE;
			assert(m->vmp_q_state == VM_PAGE_NOT_ON_Q);
			assert(m->vmp_wire_count == 0);
			m->vmp_wire_count++;
			assert(m->vmp_wire_count);
			if (m->vmp_wire_count == 1) {
				m->vmp_q_state = VM_PAGE_IS_WIRED;
				wired_count++;
			} else {
				panic("iopl_valid_data: %p already wired\n", m);
			}

			PAGE_WAKEUP_DONE(m);
		}
		size -= PAGE_SIZE;
	}
	if (wired_count) {
		VM_OBJECT_WIRED_PAGE_COUNT(object, wired_count);
		assert(object->resident_page_count >= object->wired_page_count);

		                                                             
		assert(object->purgable != VM_PURGABLE_VOLATILE);
		assert(object->purgable != VM_PURGABLE_EMPTY);

		vm_page_lockspin_queues();
		vm_page_wire_count += wired_count;
		vm_page_unlock_queues();
	}
	VM_OBJECT_WIRED_PAGE_UPDATE_END(object, tag);
	vm_object_unlock(object);
}


void
vm_object_set_pmap_cache_attr(
	vm_object_t             object,
	upl_page_info_array_t   user_page_list,
	unsigned int            num_pages,
	boolean_t               batch_pmap_op)
{
	unsigned int    cache_attr = 0;

	cache_attr = object->wimg_bits & VM_WIMG_MASK;
	assert(user_page_list);
	if (cache_attr != VM_WIMG_USE_DEFAULT) {
		PMAP_BATCH_SET_CACHE_ATTR(object, user_page_list, cache_attr, num_pages, batch_pmap_op);
	}
}


boolean_t       vm_object_iopl_wire_full(vm_object_t, upl_t, upl_page_info_array_t, wpl_array_t, upl_control_flags_t, vm_tag_t);
kern_return_t   vm_object_iopl_wire_empty(vm_object_t, upl_t, upl_page_info_array_t, wpl_array_t, upl_control_flags_t, vm_tag_t, vm_object_offset_t *, int, int*);



boolean_t
vm_object_iopl_wire_full(vm_object_t object, upl_t upl, upl_page_info_array_t user_page_list,
    wpl_array_t lite_list, upl_control_flags_t cntrl_flags, vm_tag_t tag)
{
	vm_page_t       dst_page;
	unsigned int    entry;
	int             page_count;
	int             delayed_unlock = 0;
	boolean_t       retval = TRUE;
	ppnum_t         phys_page;

	vm_object_lock_assert_exclusive(object);
	assert(object->purgable != VM_PURGABLE_VOLATILE);
	assert(object->purgable != VM_PURGABLE_EMPTY);
	assert(object->pager == NULL);
	assert(object->copy == NULL);
	assert(object->shadow == NULL);

	page_count = object->resident_page_count;
	dst_page = (vm_page_t)vm_page_queue_first(&object->memq);

	vm_page_lock_queues();

	while (page_count--) {
		if (dst_page->vmp_busy ||
		    dst_page->vmp_fictitious ||
		    dst_page->vmp_absent ||
		    dst_page->vmp_error ||
		    dst_page->vmp_cleaning ||
		    dst_page->vmp_restart ||
		    dst_page->vmp_laundry) {
			retval = FALSE;
			goto done;
		}
		if ((cntrl_flags & UPL_REQUEST_FORCE_COHERENCY) && dst_page->vmp_written_by_kernel == TRUE) {
			retval = FALSE;
			goto done;
		}
		dst_page->vmp_reference = TRUE;

		vm_page_wire(dst_page, tag, FALSE);

		if (!(cntrl_flags & UPL_COPYOUT_FROM)) {
			SET_PAGE_DIRTY(dst_page, FALSE);
		}
		entry = (unsigned int)(dst_page->vmp_offset / PAGE_SIZE);
		assert(entry >= 0 && entry < object->resident_page_count);
		lite_list[entry >> 5] |= 1U << (entry & 31);

		phys_page = VM_PAGE_GET_PHYS_PAGE(dst_page);

		if (phys_page > upl->highest_page) {
			upl->highest_page = phys_page;
		}

		if (user_page_list) {
			user_page_list[entry].phys_addr = phys_page;
			user_page_list[entry].absent    = dst_page->vmp_absent;
			user_page_list[entry].dirty     = dst_page->vmp_dirty;
			user_page_list[entry].free_when_done   = dst_page->vmp_free_when_done;
			user_page_list[entry].precious  = dst_page->vmp_precious;
			user_page_list[entry].device    = FALSE;
			user_page_list[entry].speculative = FALSE;
			user_page_list[entry].cs_validated = FALSE;
			user_page_list[entry].cs_tainted = FALSE;
			user_page_list[entry].cs_nx     = FALSE;
			user_page_list[entry].needed    = FALSE;
			user_page_list[entry].mark      = FALSE;
		}
		if (delayed_unlock++ > 256) {
			delayed_unlock = 0;
			lck_mtx_yield(&vm_page_queue_lock);

			VM_CHECK_MEMORYSTATUS;
		}
		dst_page = (vm_page_t)vm_page_queue_next(&dst_page->vmp_listq);
	}
done:
	vm_page_unlock_queues();

	VM_CHECK_MEMORYSTATUS;

	return retval;
}


kern_return_t
vm_object_iopl_wire_empty(vm_object_t object, upl_t upl, upl_page_info_array_t user_page_list,
    wpl_array_t lite_list, upl_control_flags_t cntrl_flags, vm_tag_t tag, vm_object_offset_t *dst_offset,
    int page_count, int* page_grab_count)
{
	vm_page_t       dst_page;
	boolean_t       no_zero_fill = FALSE;
	int             interruptible;
	int             pages_wired = 0;
	int             pages_inserted = 0;
	int             entry = 0;
	uint64_t        delayed_ledger_update = 0;
	kern_return_t   ret = KERN_SUCCESS;
	int             grab_options;
	ppnum_t         phys_page;

	vm_object_lock_assert_exclusive(object);
	assert(object->purgable != VM_PURGABLE_VOLATILE);
	assert(object->purgable != VM_PURGABLE_EMPTY);
	assert(object->pager == NULL);
	assert(object->copy == NULL);
	assert(object->shadow == NULL);

	if (cntrl_flags & UPL_SET_INTERRUPTIBLE) {
		interruptible = THREAD_ABORTSAFE;
	} else {
		interruptible = THREAD_UNINT;
	}

	if (cntrl_flags & (UPL_NOZEROFILL | UPL_NOZEROFILLIO)) {
		no_zero_fill = TRUE;
	}

	grab_options = 0;
#if CONFIG_SECLUDED_MEMORY
	if (object->can_grab_secluded) {
		grab_options |= VM_PAGE_GRAB_SECLUDED;
	}
#endif                             

	while (page_count--) {
		while ((dst_page = vm_page_grab_options(grab_options))
		    == VM_PAGE_NULL) {
			OSAddAtomic(page_count, &vm_upl_wait_for_pages);

			VM_DEBUG_EVENT(vm_iopl_page_wait, VM_IOPL_PAGE_WAIT, DBG_FUNC_START, vm_upl_wait_for_pages, 0, 0, 0);

			if (vm_page_wait(interruptible) == FALSE) {
				  
                       
       
				OSAddAtomic(-page_count, &vm_upl_wait_for_pages);

				VM_DEBUG_EVENT(vm_iopl_page_wait, VM_IOPL_PAGE_WAIT, DBG_FUNC_END, vm_upl_wait_for_pages, 0, 0, -1);

				ret = MACH_SEND_INTERRUPTED;
				goto done;
			}
			OSAddAtomic(-page_count, &vm_upl_wait_for_pages);

			VM_DEBUG_EVENT(vm_iopl_page_wait, VM_IOPL_PAGE_WAIT, DBG_FUNC_END, vm_upl_wait_for_pages, 0, 0, 0);
		}
		if (no_zero_fill == FALSE) {
			vm_page_zero_fill(dst_page);
		} else {
			dst_page->vmp_absent = TRUE;
		}

		dst_page->vmp_reference = TRUE;

		if (!(cntrl_flags & UPL_COPYOUT_FROM)) {
			SET_PAGE_DIRTY(dst_page, FALSE);
		}
		if (dst_page->vmp_absent == FALSE) {
			assert(dst_page->vmp_q_state == VM_PAGE_NOT_ON_Q);
			assert(dst_page->vmp_wire_count == 0);
			dst_page->vmp_wire_count++;
			dst_page->vmp_q_state = VM_PAGE_IS_WIRED;
			assert(dst_page->vmp_wire_count);
			pages_wired++;
			PAGE_WAKEUP_DONE(dst_page);
		}
		pages_inserted++;

		vm_page_insert_internal(dst_page, object, *dst_offset, tag, FALSE, TRUE, TRUE, TRUE, &delayed_ledger_update);

		lite_list[entry >> 5] |= 1U << (entry & 31);

		phys_page = VM_PAGE_GET_PHYS_PAGE(dst_page);

		if (phys_page > upl->highest_page) {
			upl->highest_page = phys_page;
		}

		if (user_page_list) {
			user_page_list[entry].phys_addr = phys_page;
			user_page_list[entry].absent    = dst_page->vmp_absent;
			user_page_list[entry].dirty     = dst_page->vmp_dirty;
			user_page_list[entry].free_when_done    = FALSE;
			user_page_list[entry].precious  = FALSE;
			user_page_list[entry].device    = FALSE;
			user_page_list[entry].speculative = FALSE;
			user_page_list[entry].cs_validated = FALSE;
			user_page_list[entry].cs_tainted = FALSE;
			user_page_list[entry].cs_nx     = FALSE;
			user_page_list[entry].needed    = FALSE;
			user_page_list[entry].mark      = FALSE;
		}
		entry++;
		*dst_offset += PAGE_SIZE_64;
	}
done:
	if (pages_wired) {
		vm_page_lockspin_queues();
		vm_page_wire_count += pages_wired;
		vm_page_unlock_queues();
	}
	if (pages_inserted) {
		if (object->internal) {
			OSAddAtomic(pages_inserted, &vm_page_internal_count);
		} else {
			OSAddAtomic(pages_inserted, &vm_page_external_count);
		}
	}
	if (delayed_ledger_update) {
		task_t          owner;
		int             ledger_idx_volatile;
		int             ledger_idx_nonvolatile;
		int             ledger_idx_volatile_compressed;
		int             ledger_idx_nonvolatile_compressed;
		boolean_t       do_footprint;

		owner = VM_OBJECT_OWNER(object);
		assert(owner);

		vm_object_ledger_tag_ledgers(object,
		    &ledger_idx_volatile,
		    &ledger_idx_nonvolatile,
		    &ledger_idx_volatile_compressed,
		    &ledger_idx_nonvolatile_compressed,
		    &do_footprint);

		                             
		ledger_credit(owner->ledger,
		    ledger_idx_nonvolatile,
		    delayed_ledger_update);
		if (do_footprint) {
			                    
			ledger_credit(owner->ledger,
			    task_ledgers.phys_footprint,
			    delayed_ledger_update);
		}
	}

	assert(page_grab_count);
	*page_grab_count = pages_inserted;

	return ret;
}



kern_return_t
vm_object_iopl_request(
	vm_object_t             object,
	vm_object_offset_t      offset,
	upl_size_t              size,
	upl_t                   *upl_ptr,
	upl_page_info_array_t   user_page_list,
	unsigned int            *page_list_count,
	upl_control_flags_t     cntrl_flags,
	vm_tag_t                tag)
{
	vm_page_t               dst_page;
	vm_object_offset_t      dst_offset;
	upl_size_t              xfer_size;
	upl_t                   upl = NULL;
	unsigned int            entry;
	wpl_array_t             lite_list = NULL;
	int                     no_zero_fill = FALSE;
	unsigned int            size_in_pages;
	int                     page_grab_count = 0;
	u_int32_t               psize;
	kern_return_t           ret;
	vm_prot_t               prot;
	struct vm_object_fault_info fault_info = {};
	struct  vm_page_delayed_work    dw_array;
	struct  vm_page_delayed_work    *dwp, *dwp_start;
	bool                    dwp_finish_ctx = TRUE;
	int                     dw_count;
	int                     dw_limit;
	int                     dw_index;
	boolean_t               caller_lookup;
	int                     io_tracking_flag = 0;
	int                     interruptible;
	ppnum_t                 phys_page;

	boolean_t               set_cache_attr_needed = FALSE;
	boolean_t               free_wired_pages = FALSE;
	boolean_t               fast_path_empty_req = FALSE;
	boolean_t               fast_path_full_req = FALSE;

#if DEVELOPMENT || DEBUG
	task_t                  task = current_task();
#endif                           

	dwp_start = dwp = NULL;

	vm_object_offset_t original_offset = offset;
	upl_size_t original_size = size;

                                                                                                                                  

	size = (upl_size_t)(vm_object_round_page(offset + size) - vm_object_trunc_page(offset));
	offset = vm_object_trunc_page(offset);
	if (size != original_size || offset != original_offset) {
		DEBUG4K_IOKIT("flags 0x%llx object %p offset 0x%llx size 0x%x -> offset 0x%llx size 0x%x\n", cntrl_flags, object, original_offset, original_size, offset, size);
	}

	if (cntrl_flags & ~UPL_VALID_FLAGS) {
		  
                                      
                             
     
		return KERN_INVALID_VALUE;
	}
	if (vm_lopage_needed == FALSE) {
		cntrl_flags &= ~UPL_NEED_32BIT_ADDR;
	}

	if (cntrl_flags & UPL_NEED_32BIT_ADDR) {
		if ((cntrl_flags & (UPL_SET_IO_WIRE | UPL_SET_LITE)) != (UPL_SET_IO_WIRE | UPL_SET_LITE)) {
			return KERN_INVALID_VALUE;
		}

		if (object->phys_contiguous) {
			if ((offset + object->vo_shadow_offset) >= (vm_object_offset_t)max_valid_dma_address) {
				return KERN_INVALID_ADDRESS;
			}

			if (((offset + object->vo_shadow_offset) + size) >= (vm_object_offset_t)max_valid_dma_address) {
				return KERN_INVALID_ADDRESS;
			}
		}
	}
	if (cntrl_flags & (UPL_NOZEROFILL | UPL_NOZEROFILLIO)) {
		no_zero_fill = TRUE;
	}

	if (cntrl_flags & UPL_COPYOUT_FROM) {
		prot = VM_PROT_READ;
	} else {
		prot = VM_PROT_READ | VM_PROT_WRITE;
	}

	if ((!object->internal) && (object->paging_offset != 0)) {
		panic("vm_object_iopl_request: external object with non-zero paging offset\n");
	}


	VM_DEBUG_CONSTANT_EVENT(vm_object_iopl_request, VM_IOPL_REQUEST, DBG_FUNC_START, size, cntrl_flags, prot, 0);

#if CONFIG_IOSCHED || UPL_DEBUG
	if ((object->io_tracking && object != kernel_object) || upl_debug_enabled) {
		io_tracking_flag |= UPL_CREATE_IO_TRACKING;
	}
#endif

#if CONFIG_IOSCHED
	if (object->io_tracking) {
		                                                                                                     
		if (object != kernel_object) {
			io_tracking_flag |= UPL_CREATE_EXPEDITE_SUP;
		}
	}
#endif

	if (object->phys_contiguous) {
		psize = PAGE_SIZE;
	} else {
		psize = size;

		dw_count = 0;
		dw_limit = DELAYED_WORK_LIMIT(DEFAULT_DELAYED_WORK_LIMIT);
		dwp_start = vm_page_delayed_work_get_ctx();
		if (dwp_start == NULL) {
			dwp_start = &dw_array;
			dw_limit = 1;
			dwp_finish_ctx = FALSE;
		}

		dwp = dwp_start;
	}

	if (cntrl_flags & UPL_SET_INTERNAL) {
		upl = upl_create(UPL_CREATE_INTERNAL | UPL_CREATE_LITE | io_tracking_flag, UPL_IO_WIRE, psize);

		user_page_list = (upl_page_info_t *) (((uintptr_t)upl) + sizeof(struct upl));
		lite_list = (wpl_array_t) (((uintptr_t)user_page_list) +
		    ((psize / PAGE_SIZE) * sizeof(upl_page_info_t)));
		if (size == 0) {
			user_page_list = NULL;
			lite_list = NULL;
		}
	} else {
		upl = upl_create(UPL_CREATE_LITE | io_tracking_flag, UPL_IO_WIRE, psize);

		lite_list = (wpl_array_t) (((uintptr_t)upl) + sizeof(struct upl));
		if (size == 0) {
			lite_list = NULL;
		}
	}
	if (user_page_list) {
		user_page_list[0].device = FALSE;
	}
	*upl_ptr = upl;

	if (cntrl_flags & UPL_NOZEROFILLIO) {
		DTRACE_VM4(upl_nozerofillio,
		    vm_object_t, object,
		    vm_object_offset_t, offset,
		    upl_size_t, size,
		    upl_t, upl);
	}

	upl->map_object = object;
	upl->u_offset = original_offset;
	upl->u_size = original_size;

	size_in_pages = size / PAGE_SIZE;

	if (object == kernel_object &&
	    !(cntrl_flags & (UPL_NEED_32BIT_ADDR | UPL_BLOCK_ACCESS))) {
		upl->flags |= UPL_KERNEL_OBJECT;
#if UPL_DEBUG
		vm_object_lock(object);
#else
		vm_object_lock_shared(object);
#endif
	} else {
		vm_object_lock(object);
		vm_object_activity_begin(object);
	}
	  
                                                      
    
	upl->u_offset = original_offset + object->paging_offset;

	if (cntrl_flags & UPL_BLOCK_ACCESS) {
		  
                                                            
                                                     
     
		upl->flags |= UPL_ACCESS_BLOCKED;
	}

#if CONFIG_IOSCHED || UPL_DEBUG
	if ((upl->flags & UPL_TRACKED_BY_OBJECT) || upl_debug_enabled) {
		vm_object_activity_begin(object);
		queue_enter(&object->uplq, upl, upl_t, uplq);
	}
#endif

	if (object->phys_contiguous) {
		if (upl->flags & UPL_ACCESS_BLOCKED) {
			assert(!object->blocked_access);
			object->blocked_access = TRUE;
		}

		vm_object_unlock(object);

		  
                                                
                                   
     
		upl->flags |= UPL_DEVICE_MEMORY;

		upl->highest_page = (ppnum_t) ((offset + object->vo_shadow_offset + size - 1) >> PAGE_SHIFT);

		if (user_page_list) {
			user_page_list[0].phys_addr = (ppnum_t) ((offset + object->vo_shadow_offset) >> PAGE_SHIFT);
			user_page_list[0].device = TRUE;
		}
		if (page_list_count != NULL) {
			if (upl->flags & UPL_INTERNAL) {
				*page_list_count = 0;
			} else {
				*page_list_count = 1;
			}
		}

		VM_DEBUG_CONSTANT_EVENT(vm_object_iopl_request, VM_IOPL_REQUEST, DBG_FUNC_END, page_grab_count, KERN_SUCCESS, 0, 0);
#if DEVELOPMENT || DEBUG
		if (task != NULL) {
			ledger_credit(task->ledger, task_ledgers.pages_grabbed_iopl, page_grab_count);
		}
#endif                           
		return KERN_SUCCESS;
	}
	if (object != kernel_object && object != compressor_object) {
		  
                                                  
     
#if VM_OBJECT_TRACKING_OP_TRUESHARE
		if (!object->true_share &&
		    vm_object_tracking_inited) {
			void *bt[VM_OBJECT_TRACKING_BTDEPTH];
			int num = 0;

			num = OSBacktrace(bt,
			    VM_OBJECT_TRACKING_BTDEPTH);
			btlog_add_entry(vm_object_tracking_btlog,
			    object,
			    VM_OBJECT_TRACKING_OP_TRUESHARE,
			    bt,
			    num);
		}
#endif                                      

		vm_object_lock_assert_exclusive(object);
		object->true_share = TRUE;

		if (object->copy_strategy == MEMORY_OBJECT_COPY_SYMMETRIC) {
			object->copy_strategy = MEMORY_OBJECT_COPY_DELAY;
		}
	}

	if (!(cntrl_flags & UPL_COPYOUT_FROM) &&
	    object->copy != VM_OBJECT_NULL) {
		  
                                    
    
                                            
                                             
                                               
                                                
                            
    
                                                     
                                                       
                                                         
                                                             
                                                           
                                                        
                          
     
		vm_object_update(object,
		    offset,
		    size,
		    NULL,
		    NULL,
		    FALSE,                                 
		    MEMORY_OBJECT_COPY_SYNC,
		    VM_PROT_NO_CHANGE);
		VM_PAGEOUT_DEBUG(iopl_cow, 1);
		VM_PAGEOUT_DEBUG(iopl_cow_pages, (size >> PAGE_SHIFT));
	}
	if (!(cntrl_flags & (UPL_NEED_32BIT_ADDR | UPL_BLOCK_ACCESS)) &&
	    object->purgable != VM_PURGABLE_VOLATILE &&
	    object->purgable != VM_PURGABLE_EMPTY &&
	    object->copy == NULL &&
	    size == object->vo_size &&
	    offset == 0 &&
	    object->shadow == NULL &&
	    object->pager == NULL) {
		if (object->resident_page_count == size_in_pages) {
			assert(object != compressor_object);
			assert(object != kernel_object);
			fast_path_full_req = TRUE;
		} else if (object->resident_page_count == 0) {
			assert(object != compressor_object);
			assert(object != kernel_object);
			fast_path_empty_req = TRUE;
			set_cache_attr_needed = TRUE;
		}
	}

	if (cntrl_flags & UPL_SET_INTERRUPTIBLE) {
		interruptible = THREAD_ABORTSAFE;
	} else {
		interruptible = THREAD_UNINT;
	}

	entry = 0;

	xfer_size = size;
	dst_offset = offset;

	if (fast_path_full_req) {
		if (vm_object_iopl_wire_full(object, upl, user_page_list, lite_list, cntrl_flags, tag) == TRUE) {
			goto finish;
		}
		  
                                                                         
                                                   
     
	} else if (fast_path_empty_req) {
		if (cntrl_flags & UPL_REQUEST_NO_FAULT) {
			ret = KERN_MEMORY_ERROR;
			goto return_err;
		}
		ret = vm_object_iopl_wire_empty(object, upl, user_page_list, lite_list, cntrl_flags, tag, &dst_offset, size_in_pages, &page_grab_count);

		if (ret) {
			free_wired_pages = TRUE;
			goto return_err;
		}
		goto finish;
	}

	fault_info.behavior = VM_BEHAVIOR_SEQUENTIAL;
	fault_info.lo_offset = offset;
	fault_info.hi_offset = offset + xfer_size;
	fault_info.mark_zf_absent = TRUE;
	fault_info.interruptible = interruptible;
	fault_info.batch_pmap_op = TRUE;

	while (xfer_size) {
		vm_fault_return_t       result;

		dwp->dw_mask = 0;

		if (fast_path_full_req) {
			  
                                                      
                                                   
                                                    
                                                        
                                                        
                                                      
      
			if (lite_list[entry >> 5] & (1 << (entry & 31))) {
				goto skip_page;
			}
		}
		dst_page = vm_page_lookup(object, dst_offset);

		if (dst_page == VM_PAGE_NULL ||
		    dst_page->vmp_busy ||
		    dst_page->vmp_error ||
		    dst_page->vmp_restart ||
		    dst_page->vmp_absent ||
		    dst_page->vmp_fictitious) {
			if (object == kernel_object) {
				panic("vm_object_iopl_request: missing/bad page in kernel object\n");
			}
			if (object == compressor_object) {
				panic("vm_object_iopl_request: missing/bad page in compressor object\n");
			}

			if (cntrl_flags & UPL_REQUEST_NO_FAULT) {
				ret = KERN_MEMORY_ERROR;
				goto return_err;
			}
			set_cache_attr_needed = TRUE;

			  
                                                             
                                                     
                                                         
                             
      
			caller_lookup = TRUE;

			do {
				vm_page_t       top_page;
				kern_return_t   error_code;

				fault_info.cluster_size = xfer_size;

				vm_object_paging_begin(object);

				result = vm_fault_page(object, dst_offset,
				    prot | VM_PROT_WRITE, FALSE,
				    caller_lookup,
				    &prot, &dst_page, &top_page,
				    (int *)0,
				    &error_code, no_zero_fill,
				    FALSE, &fault_info);

				                                                 
				caller_lookup = FALSE;

				switch (result) {
				case VM_FAULT_SUCCESS:
					page_grab_count++;

					if (!dst_page->vmp_absent) {
						PAGE_WAKEUP_DONE(dst_page);
					} else {
						  
                                              
                                             
                                                
        
                                         
                                           
                                           
                                            
                                              
                                               
         
					}
					  
                                     
                                           
        
					if (top_page != VM_PAGE_NULL) {
						vm_object_t local_object;

						local_object = VM_PAGE_OBJECT(top_page);

						  
                                    
         
						if (top_page->vmp_object != dst_page->vmp_object) {
							vm_object_lock(local_object);
							VM_PAGE_FREE(top_page);
							vm_object_paging_end(local_object);
							vm_object_unlock(local_object);
						} else {
							VM_PAGE_FREE(top_page);
							vm_object_paging_end(local_object);
						}
					}
					vm_object_paging_end(object);
					break;

				case VM_FAULT_RETRY:
					vm_object_lock(object);
					break;

				case VM_FAULT_MEMORY_SHORTAGE:
					OSAddAtomic((size_in_pages - entry), &vm_upl_wait_for_pages);

					VM_DEBUG_EVENT(vm_iopl_page_wait, VM_IOPL_PAGE_WAIT, DBG_FUNC_START, vm_upl_wait_for_pages, 0, 0, 0);

					if (vm_page_wait(interruptible)) {
						OSAddAtomic(-(size_in_pages - entry), &vm_upl_wait_for_pages);

						VM_DEBUG_EVENT(vm_iopl_page_wait, VM_IOPL_PAGE_WAIT, DBG_FUNC_END, vm_upl_wait_for_pages, 0, 0, 0);
						vm_object_lock(object);

						break;
					}
					OSAddAtomic(-(size_in_pages - entry), &vm_upl_wait_for_pages);

					VM_DEBUG_EVENT(vm_iopl_page_wait, VM_IOPL_PAGE_WAIT, DBG_FUNC_END, vm_upl_wait_for_pages, 0, 0, -1);

					OS_FALLTHROUGH;

				case VM_FAULT_INTERRUPTED:
					error_code = MACH_SEND_INTERRUPTED;
					OS_FALLTHROUGH;
				case VM_FAULT_MEMORY_ERROR:
memory_error:
					ret = (error_code ? error_code: KERN_MEMORY_ERROR);

					vm_object_lock(object);
					goto return_err;

				case VM_FAULT_SUCCESS_NO_VM_PAGE:
					                               
					vm_object_paging_end(object);
					vm_object_unlock(object);
					goto memory_error;

				default:
					panic("vm_object_iopl_request: unexpected error"
					    " 0x%x from vm_fault_page()\n", result);
				}
			} while (result != VM_FAULT_SUCCESS);
		}
		phys_page = VM_PAGE_GET_PHYS_PAGE(dst_page);

		if (upl->flags & UPL_KERNEL_OBJECT) {
			goto record_phys_addr;
		}

		if (dst_page->vmp_q_state == VM_PAGE_USED_BY_COMPRESSOR) {
			dst_page->vmp_busy = TRUE;
			goto record_phys_addr;
		}

		if (dst_page->vmp_cleaning) {
			  
                                                  
                                                           
                                                          
                                                         
                                                     
                            
                                                       
                     
      
			VM_PAGEOUT_DEBUG(vm_object_iopl_request_sleep_for_cleaning, 1);
			PAGE_SLEEP(object, dst_page, THREAD_UNINT);
			continue;
		}
		if (dst_page->vmp_laundry) {
			vm_pageout_steal_laundry(dst_page, FALSE);
		}

		if ((cntrl_flags & UPL_NEED_32BIT_ADDR) &&
		    phys_page >= (max_valid_dma_address >> PAGE_SHIFT)) {
			vm_page_t       low_page;
			int             refmod;

			  
                                                  
                                                      
                                                    
                                                           
                                                          
                                                               
      
			if (VM_PAGE_WIRED(dst_page)) {
				ret = KERN_PROTECTION_FAILURE;
				goto return_err;
			}
			low_page = vm_page_grablo();

			if (low_page == VM_PAGE_NULL) {
				ret = KERN_RESOURCE_SHORTAGE;
				goto return_err;
			}
			  
                                                   
                                                
                                                   
                                                    
                                             
      
			if (dst_page->vmp_pmapped) {
				refmod = pmap_disconnect(phys_page);
			} else {
				refmod = 0;
			}

			if (!dst_page->vmp_absent) {
				vm_page_copy(dst_page, low_page);
			}

			low_page->vmp_reference = dst_page->vmp_reference;
			low_page->vmp_dirty     = dst_page->vmp_dirty;
			low_page->vmp_absent    = dst_page->vmp_absent;

			if (refmod & VM_MEM_REFERENCED) {
				low_page->vmp_reference = TRUE;
			}
			if (refmod & VM_MEM_MODIFIED) {
				SET_PAGE_DIRTY(low_page, FALSE);
			}

			vm_page_replace(low_page, object, dst_offset);

			dst_page = low_page;
			  
                                             
                                              
                                                       
      
			if (!dst_page->vmp_absent) {
				dst_page->vmp_busy = FALSE;
			}

			phys_page = VM_PAGE_GET_PHYS_PAGE(dst_page);
		}
		if (!dst_page->vmp_busy) {
			dwp->dw_mask |= DW_vm_page_wire;
		}

		if (cntrl_flags & UPL_BLOCK_ACCESS) {
			  
                                                         
                                            
                                   
                                                     
      
			assert(!dst_page->vmp_fictitious);
			dst_page->vmp_busy = TRUE;
		}
		  
                               
                                                     
     
		dwp->dw_mask |= DW_set_reference;

		if (!(cntrl_flags & UPL_COPYOUT_FROM)) {
			SET_PAGE_DIRTY(dst_page, TRUE);
			  
                                                        
                                                        
                                                         
                                
      
			if (object->code_signed && dst_page->vmp_cs_tainted != VMP_CS_ALL_TRUE) {
				dst_page->vmp_cs_tainted = VMP_CS_ALL_TRUE;
				vm_page_iopl_tainted++;
				if (dst_page->vmp_pmapped) {
					int refmod = pmap_disconnect(VM_PAGE_GET_PHYS_PAGE(dst_page));
					if (refmod & VM_MEM_REFERENCED) {
						dst_page->vmp_reference = TRUE;
					}
				}
			}
		}
		if ((cntrl_flags & UPL_REQUEST_FORCE_COHERENCY) && dst_page->vmp_written_by_kernel == TRUE) {
			pmap_sync_page_attributes_phys(phys_page);
			dst_page->vmp_written_by_kernel = FALSE;
		}

record_phys_addr:
		if (dst_page->vmp_busy) {
			upl->flags |= UPL_HAS_BUSY;
		}

		lite_list[entry >> 5] |= 1U << (entry & 31);

		if (phys_page > upl->highest_page) {
			upl->highest_page = phys_page;
		}

		if (user_page_list) {
			user_page_list[entry].phys_addr = phys_page;
			user_page_list[entry].free_when_done    = dst_page->vmp_free_when_done;
			user_page_list[entry].absent    = dst_page->vmp_absent;
			user_page_list[entry].dirty     = dst_page->vmp_dirty;
			user_page_list[entry].precious  = dst_page->vmp_precious;
			user_page_list[entry].device    = FALSE;
			user_page_list[entry].needed    = FALSE;
			if (dst_page->vmp_clustered == TRUE) {
				user_page_list[entry].speculative = (dst_page->vmp_q_state == VM_PAGE_ON_SPECULATIVE_Q) ? TRUE : FALSE;
			} else {
				user_page_list[entry].speculative = FALSE;
			}
			user_page_list[entry].cs_validated = dst_page->vmp_cs_validated;
			user_page_list[entry].cs_tainted = dst_page->vmp_cs_tainted;
			user_page_list[entry].cs_nx = dst_page->vmp_cs_nx;
			user_page_list[entry].mark      = FALSE;
		}
		if (object != kernel_object && object != compressor_object) {
			  
                                                 
                                            
     
      
			if (dst_page->vmp_clustered) {
				VM_PAGE_CONSUME_CLUSTERED(dst_page);
			}
		}
skip_page:
		entry++;
		dst_offset += PAGE_SIZE_64;
		xfer_size -= PAGE_SIZE;

		if (dwp->dw_mask) {
			VM_PAGE_ADD_DELAYED_WORK(dwp, dst_page, dw_count);

			if (dw_count >= dw_limit) {
				vm_page_do_delayed_work(object, tag, dwp_start, dw_count);

				dwp = dwp_start;
				dw_count = 0;
			}
		}
	}
	assert(entry == size_in_pages);

	if (dw_count) {
		vm_page_do_delayed_work(object, tag, dwp_start, dw_count);
		dwp = dwp_start;
		dw_count = 0;
	}
finish:
	if (user_page_list && set_cache_attr_needed == TRUE) {
		vm_object_set_pmap_cache_attr(object, user_page_list, size_in_pages, TRUE);
	}

	if (page_list_count != NULL) {
		if (upl->flags & UPL_INTERNAL) {
			*page_list_count = 0;
		} else if (*page_list_count > size_in_pages) {
			*page_list_count = size_in_pages;
		}
	}
	vm_object_unlock(object);

	if (cntrl_flags & UPL_BLOCK_ACCESS) {
		  
                                                     
                            
                                                         
                                                    
     
		vm_object_pmap_protect(object, offset, (vm_object_size_t)size,
		    PMAP_NULL,
		    PAGE_SIZE,
		    0, VM_PROT_NONE);
		assert(!object->blocked_access);
		object->blocked_access = TRUE;
	}

	VM_DEBUG_CONSTANT_EVENT(vm_object_iopl_request, VM_IOPL_REQUEST, DBG_FUNC_END, page_grab_count, KERN_SUCCESS, 0, 0);
#if DEVELOPMENT || DEBUG
	if (task != NULL) {
		ledger_credit(task->ledger, task_ledgers.pages_grabbed_iopl, page_grab_count);
	}
#endif                           

	if (dwp_start && dwp_finish_ctx) {
		vm_page_delayed_work_finish_ctx(dwp_start);
		dwp_start = dwp = NULL;
	}

	return KERN_SUCCESS;

return_err:
	dw_index = 0;

	for (; offset < dst_offset; offset += PAGE_SIZE) {
		boolean_t need_unwire;

		dst_page = vm_page_lookup(object, offset);

		if (dst_page == VM_PAGE_NULL) {
			panic("vm_object_iopl_request: Wired page missing. \n");
		}

		  
                                                       
                                                      
                                                  
                                                        
                                                      
                                                      
                                                       
                        
     
		need_unwire = TRUE;

		if (dw_count) {
			if ((dwp_start)[dw_index].dw_m == dst_page) {
				  
                                      
                                        
                                
       
				need_unwire = FALSE;

				dw_index++;
				dw_count--;
			}
		}
		vm_page_lock_queues();

		if (dst_page->vmp_absent || free_wired_pages == TRUE) {
			vm_page_free(dst_page);

			need_unwire = FALSE;
		} else {
			if (need_unwire == TRUE) {
				vm_page_unwire(dst_page, TRUE);
			}

			PAGE_WAKEUP_DONE(dst_page);
		}
		vm_page_unlock_queues();

		if (need_unwire == TRUE) {
			counter_inc(&vm_statistics_reactivations);
		}
	}
#if UPL_DEBUG
	upl->upl_state = 2;
#endif
	if (!(upl->flags & UPL_KERNEL_OBJECT)) {
		vm_object_activity_end(object);
		vm_object_collapse(object, 0, TRUE);
	}
	vm_object_unlock(object);
	upl_destroy(upl);

	VM_DEBUG_CONSTANT_EVENT(vm_object_iopl_request, VM_IOPL_REQUEST, DBG_FUNC_END, page_grab_count, ret, 0, 0);
#if DEVELOPMENT || DEBUG
	if (task != NULL) {
		ledger_credit(task->ledger, task_ledgers.pages_grabbed_iopl, page_grab_count);
	}
#endif                           

	if (dwp_start && dwp_finish_ctx) {
		vm_page_delayed_work_finish_ctx(dwp_start);
		dwp_start = dwp = NULL;
	}
	return ret;
}

kern_return_t
upl_transpose(
	upl_t           upl1,
	upl_t           upl2)
{
	kern_return_t           retval;
	boolean_t               upls_locked;
	vm_object_t             object1, object2;

	if (upl1 == UPL_NULL || upl2 == UPL_NULL || upl1 == upl2 || ((upl1->flags & UPL_VECTOR) == UPL_VECTOR) || ((upl2->flags & UPL_VECTOR) == UPL_VECTOR)) {
		return KERN_INVALID_ARGUMENT;
	}

	upls_locked = FALSE;

	  
                                                     
                                                             
    
	if (upl1 < upl2) {
		upl_lock(upl1);
		upl_lock(upl2);
	} else {
		upl_lock(upl2);
		upl_lock(upl1);
	}
	upls_locked = TRUE;                                            

	object1 = upl1->map_object;
	object2 = upl2->map_object;

	if (upl1->u_offset != 0 || upl2->u_offset != 0 ||
	    upl1->u_size != upl2->u_size) {
		  
                                                 
                                                             
                                                                
                     
     
		retval = KERN_INVALID_VALUE;
		goto done;
	}

	  
                                           
    
	retval = vm_object_transpose(object1, object2,
	    upl_adjusted_size(upl1, PAGE_MASK));

	if (retval == KERN_SUCCESS) {
		  
                                                           
                                                       
     
#if CONFIG_IOSCHED || UPL_DEBUG
		if ((upl1->flags & UPL_TRACKED_BY_OBJECT) || (upl2->flags & UPL_TRACKED_BY_OBJECT)) {
			vm_object_lock(object1);
			vm_object_lock(object2);
		}
		if ((upl1->flags & UPL_TRACKED_BY_OBJECT) || upl_debug_enabled) {
			queue_remove(&object1->uplq, upl1, upl_t, uplq);
		}
		if ((upl2->flags & UPL_TRACKED_BY_OBJECT) || upl_debug_enabled) {
			queue_remove(&object2->uplq, upl2, upl_t, uplq);
		}
#endif
		upl1->map_object = object2;
		upl2->map_object = object1;

#if CONFIG_IOSCHED || UPL_DEBUG
		if ((upl1->flags & UPL_TRACKED_BY_OBJECT) || upl_debug_enabled) {
			queue_enter(&object2->uplq, upl1, upl_t, uplq);
		}
		if ((upl2->flags & UPL_TRACKED_BY_OBJECT) || upl_debug_enabled) {
			queue_enter(&object1->uplq, upl2, upl_t, uplq);
		}
		if ((upl1->flags & UPL_TRACKED_BY_OBJECT) || (upl2->flags & UPL_TRACKED_BY_OBJECT)) {
			vm_object_unlock(object2);
			vm_object_unlock(object1);
		}
#endif
	}

done:
	  
            
    
	if (upls_locked) {
		upl_unlock(upl1);
		upl_unlock(upl2);
		upls_locked = FALSE;
	}

	return retval;
}

void
upl_range_needed(
	upl_t           upl,
	int             index,
	int             count)
{
	upl_page_info_t *user_page_list;
	int             size_in_pages;

	if (!(upl->flags & UPL_INTERNAL) || count <= 0) {
		return;
	}

	size_in_pages = upl_adjusted_size(upl, PAGE_MASK) / PAGE_SIZE;

	user_page_list = (upl_page_info_t *) (((uintptr_t)upl) + sizeof(struct upl));

	while (count-- && index < size_in_pages) {
		user_page_list[index++].needed = TRUE;
	}
}


  
                                                            
                                                              
                                                            
                                                              
                                                               
                                                       
                   
   
SIMPLE_LOCK_DECLARE(vm_paging_lock, 0);
#define VM_PAGING_NUM_PAGES     64
vm_map_offset_t vm_paging_base_address = 0;
boolean_t       vm_paging_page_inuse[VM_PAGING_NUM_PAGES] = { FALSE, };
int             vm_paging_max_index = 0;
int             vm_paging_page_waiter = 0;
int             vm_paging_page_waiter_total = 0;

unsigned long   vm_paging_no_kernel_page = 0;
unsigned long   vm_paging_objects_mapped = 0;
unsigned long   vm_paging_pages_mapped = 0;
unsigned long   vm_paging_objects_mapped_slow = 0;
unsigned long   vm_paging_pages_mapped_slow = 0;

__startup_func
void
vm_paging_map_init(void)
{
	kern_return_t   kr;
	vm_map_offset_t page_map_offset;
	vm_map_entry_t  map_entry;

	assert(vm_paging_base_address == 0);

	  
                                               
                      
    
	page_map_offset = 0;
	kr = vm_map_find_space(kernel_map,
	    &page_map_offset,
	    VM_PAGING_NUM_PAGES * PAGE_SIZE,
	    0,
	    0,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    VM_KERN_MEMORY_NONE,
	    &map_entry);
	if (kr != KERN_SUCCESS) {
		panic("vm_paging_map_init: kernel_map full\n");
	}
	VME_OBJECT_SET(map_entry, kernel_object);
	VME_OFFSET_SET(map_entry, page_map_offset);
	map_entry->protection = VM_PROT_NONE;
	map_entry->max_protection = VM_PROT_NONE;
	map_entry->permanent = TRUE;
	vm_object_reference(kernel_object);
	vm_map_unlock(kernel_map);

	assert(vm_paging_base_address == 0);
	vm_paging_base_address = page_map_offset;
}

  
                        
                                                 
                                                      
                                         
           
                                                    
                                                     
                                                  
                                            
                                                          
                                                    
                           
   
kern_return_t
vm_paging_map_object(
	vm_page_t               page,
	vm_object_t             object,
	vm_object_offset_t      offset,
	vm_prot_t               protection,
	boolean_t               can_unlock_object,
	vm_map_size_t           *size,                      
	vm_map_offset_t         *address,                
	boolean_t               *need_unmap)             
{
	kern_return_t           kr;
	vm_map_offset_t         page_map_offset;
	vm_map_size_t           map_size;
	vm_object_offset_t      object_offset;
	int                     i;

	if (page != VM_PAGE_NULL && *size == PAGE_SIZE) {
		                                                              
		*address = (vm_map_offset_t)
		    phystokv((pmap_paddr_t)VM_PAGE_GET_PHYS_PAGE(page) << PAGE_SHIFT);
		*need_unmap = FALSE;
		return KERN_SUCCESS;

		assert(page->vmp_busy);
		  
                                                          
                                                           
                             
     
		simple_lock(&vm_paging_lock, &vm_pageout_lck_grp);

		  
                                                     
                                 
     
		page_map_offset = 0;
		for (;;) {
			for (i = 0; i < VM_PAGING_NUM_PAGES; i++) {
				if (vm_paging_page_inuse[i] == FALSE) {
					page_map_offset =
					    vm_paging_base_address +
					    (i * PAGE_SIZE);
					break;
				}
			}
			if (page_map_offset != 0) {
				                                     
				break;
			}

			if (can_unlock_object) {
				  
                                                
                                      
       
				break;
			}
			  
                                                 
                                                   
      
			vm_paging_page_waiter_total++;
			vm_paging_page_waiter++;
			kr = assert_wait((event_t)&vm_paging_page_waiter, THREAD_UNINT);
			if (kr == THREAD_WAITING) {
				simple_unlock(&vm_paging_lock);
				kr = thread_block(THREAD_CONTINUE_NULL);
				simple_lock(&vm_paging_lock, &vm_pageout_lck_grp);
			}
			vm_paging_page_waiter--;
			                       
		}

		if (page_map_offset != 0) {
			  
                                        
                                                    
      
			if (i > vm_paging_max_index) {
				vm_paging_max_index = i;
			}
			vm_paging_page_inuse[i] = TRUE;
			simple_unlock(&vm_paging_lock);

			page->vmp_pmapped = TRUE;

			  
                                                   
                                                   
                                                
                                      
      
			PMAP_ENTER(kernel_pmap,
			    page_map_offset,
			    page,
			    protection,
			    VM_PROT_NONE,
			    0,
			    TRUE,
			    kr);
			assert(kr == KERN_SUCCESS);
			vm_paging_objects_mapped++;
			vm_paging_pages_mapped++;
			*address = page_map_offset;
			*need_unmap = TRUE;

#if KASAN
			kasan_notify_address(page_map_offset, PAGE_SIZE);
#endif

			                                         
			return KERN_SUCCESS;
		}

		  
                                               
                                                
                              
     
		vm_paging_no_kernel_page++;
		simple_unlock(&vm_paging_lock);
	}

	if (!can_unlock_object) {
		*address = 0;
		*size = 0;
		*need_unmap = FALSE;
		return KERN_NOT_SUPPORTED;
	}

	object_offset = vm_object_trunc_page(offset);
	map_size = vm_map_round_page(*size,
	    VM_MAP_PAGE_MASK(kernel_map));

	  
                                                
                     
    

	vm_object_reference_locked(object);                            
	vm_object_unlock(object);

	kr = vm_map_enter(kernel_map,
	    address,
	    map_size,
	    0,
	    VM_FLAGS_ANYWHERE,
	    VM_MAP_KERNEL_FLAGS_NONE,
	    VM_KERN_MEMORY_NONE,
	    object,
	    object_offset,
	    FALSE,
	    protection,
	    VM_PROT_ALL,
	    VM_INHERIT_NONE);
	if (kr != KERN_SUCCESS) {
		*address = 0;
		*size = 0;
		*need_unmap = FALSE;
		vm_object_deallocate(object);                          
		vm_object_lock(object);
		return kr;
	}

	*size = map_size;

	  
                                                 
    
	vm_object_lock(object);
	  
                                                          
                                                         
                                                       
                                                 
    

	for (page_map_offset = 0;
	    map_size != 0;
	    map_size -= PAGE_SIZE_64, page_map_offset += PAGE_SIZE_64) {
		page = vm_page_lookup(object, offset + page_map_offset);
		if (page == VM_PAGE_NULL) {
			printf("vm_paging_map_object: no page !?");
			vm_object_unlock(object);
			kr = vm_map_remove(kernel_map, *address, *size,
			    VM_MAP_REMOVE_NO_FLAGS);
			assert(kr == KERN_SUCCESS);
			*address = 0;
			*size = 0;
			*need_unmap = FALSE;
			vm_object_lock(object);
			return KERN_MEMORY_ERROR;
		}
		page->vmp_pmapped = TRUE;

		                                                        
		PMAP_ENTER(kernel_pmap,
		    *address + page_map_offset,
		    page,
		    protection,
		    VM_PROT_NONE,
		    0,
		    TRUE,
		    kr);
		assert(kr == KERN_SUCCESS);
#if KASAN
		kasan_notify_address(*address + page_map_offset, PAGE_SIZE);
#endif
	}

	vm_paging_objects_mapped_slow++;
	vm_paging_pages_mapped_slow += (unsigned long) (map_size / PAGE_SIZE_64);

	*need_unmap = TRUE;

	return KERN_SUCCESS;
}

  
                          
                                                     
                              
           
                                                    
                                       
   
void
vm_paging_unmap_object(
	vm_object_t     object,
	vm_map_offset_t start,
	vm_map_offset_t end)
{
	kern_return_t   kr;
	int             i;

	if ((vm_paging_base_address == 0) ||
	    (start < vm_paging_base_address) ||
	    (end > (vm_paging_base_address
	    + (VM_PAGING_NUM_PAGES * PAGE_SIZE)))) {
		  
                                            
                                            
                    
     
		if (object != VM_OBJECT_NULL) {
			vm_object_unlock(object);
		}
		kr = vm_map_remove(kernel_map, start, end,
		    VM_MAP_REMOVE_NO_FLAGS);
		if (object != VM_OBJECT_NULL) {
			vm_object_lock(object);
		}
		assert(kr == KERN_SUCCESS);
	} else {
		  
                                              
                                                 
                   
     
		assert(end - start == PAGE_SIZE);
		i = (int) ((start - vm_paging_base_address) >> PAGE_SHIFT);
		assert(i >= 0 && i < VM_PAGING_NUM_PAGES);

		                           
		pmap_remove(kernel_pmap, start, end);

		simple_lock(&vm_paging_lock, &vm_pageout_lck_grp);
		vm_paging_page_inuse[i] = FALSE;
		if (vm_paging_page_waiter) {
			thread_wakeup(&vm_paging_page_waiter);
		}
		simple_unlock(&vm_paging_lock);
	}
}


  
                                  
   
void
vm_pageout_steal_laundry(vm_page_t page, boolean_t queues_locked)
{
	if (!queues_locked) {
		vm_page_lockspin_queues();
	}

	page->vmp_free_when_done = FALSE;
	  
                                     
                                 
                                
                                             
   
                                                      
    
	vm_pageout_throttle_up(page);

	if (!queues_locked) {
		vm_page_unlock_queues();
	}
}

upl_t
vector_upl_create(vm_offset_t upl_offset)
{
	int     vector_upl_size  = sizeof(struct _vector_upl);
	int i = 0;
	upl_t   upl;
	vector_upl_t vector_upl = (vector_upl_t)kalloc(vector_upl_size);

	upl = upl_create(0, UPL_VECTOR, 0);
	upl->vector_upl = vector_upl;
	upl->u_offset = upl_offset;
	vector_upl->size = 0;
	vector_upl->offset = upl_offset;
	vector_upl->invalid_upls = 0;
	vector_upl->num_upls = 0;
	vector_upl->pagelist = NULL;

	for (i = 0; i < MAX_VECTOR_UPL_ELEMENTS; i++) {
		vector_upl->upl_iostates[i].size = 0;
		vector_upl->upl_iostates[i].offset = 0;
	}
	return upl;
}

void
vector_upl_deallocate(upl_t upl)
{
	if (upl) {
		vector_upl_t vector_upl = upl->vector_upl;
		if (vector_upl) {
			if (vector_upl->invalid_upls != vector_upl->num_upls) {
				panic("Deallocating non-empty Vectored UPL\n");
			}
			kfree(vector_upl->pagelist, (sizeof(struct upl_page_info) * (vector_upl->size / PAGE_SIZE)));
			vector_upl->invalid_upls = 0;
			vector_upl->num_upls = 0;
			vector_upl->pagelist = NULL;
			vector_upl->size = 0;
			vector_upl->offset = 0;
			kfree(vector_upl, sizeof(struct _vector_upl));
			vector_upl = (vector_upl_t)0xfeedfeed;
		} else {
			panic("vector_upl_deallocate was passed a non-vectored upl\n");
		}
	} else {
		panic("vector_upl_deallocate was passed a NULL upl\n");
	}
}

boolean_t
vector_upl_is_valid(upl_t upl)
{
	if (upl && ((upl->flags & UPL_VECTOR) == UPL_VECTOR)) {
		vector_upl_t vector_upl = upl->vector_upl;
		if (vector_upl == NULL || vector_upl == (vector_upl_t)0xfeedfeed || vector_upl == (vector_upl_t)0xfeedbeef) {
			return FALSE;
		} else {
			return TRUE;
		}
	}
	return FALSE;
}

boolean_t
vector_upl_set_subupl(upl_t upl, upl_t subupl, uint32_t io_size)
{
	if (vector_upl_is_valid(upl)) {
		vector_upl_t vector_upl = upl->vector_upl;

		if (vector_upl) {
			if (subupl) {
				if (io_size) {
					if (io_size < PAGE_SIZE) {
						io_size = PAGE_SIZE;
					}
					subupl->vector_upl = (void*)vector_upl;
					vector_upl->upl_elems[vector_upl->num_upls++] = subupl;
					vector_upl->size += io_size;
					upl->u_size += io_size;
				} else {
					uint32_t i = 0, invalid_upls = 0;
					for (i = 0; i < vector_upl->num_upls; i++) {
						if (vector_upl->upl_elems[i] == subupl) {
							break;
						}
					}
					if (i == vector_upl->num_upls) {
						panic("Trying to remove sub-upl when none exists");
					}

					vector_upl->upl_elems[i] = NULL;
					invalid_upls = os_atomic_inc(&(vector_upl)->invalid_upls,
					    relaxed);
					if (invalid_upls == vector_upl->num_upls) {
						return TRUE;
					} else {
						return FALSE;
					}
				}
			} else {
				panic("vector_upl_set_subupl was passed a NULL upl element\n");
			}
		} else {
			panic("vector_upl_set_subupl was passed a non-vectored upl\n");
		}
	} else {
		panic("vector_upl_set_subupl was passed a NULL upl\n");
	}

	return FALSE;
}

void
vector_upl_set_pagelist(upl_t upl)
{
	if (vector_upl_is_valid(upl)) {
		uint32_t i = 0;
		vector_upl_t vector_upl = upl->vector_upl;

		if (vector_upl) {
			vm_offset_t pagelist_size = 0, cur_upl_pagelist_size = 0;

			vector_upl->pagelist = (upl_page_info_array_t)kalloc(sizeof(struct upl_page_info) * (vector_upl->size / PAGE_SIZE));

			for (i = 0; i < vector_upl->num_upls; i++) {
				cur_upl_pagelist_size = sizeof(struct upl_page_info) * upl_adjusted_size(vector_upl->upl_elems[i], PAGE_MASK) / PAGE_SIZE;
				bcopy(UPL_GET_INTERNAL_PAGE_LIST_SIMPLE(vector_upl->upl_elems[i]), (char*)vector_upl->pagelist + pagelist_size, cur_upl_pagelist_size);
				pagelist_size += cur_upl_pagelist_size;
				if (vector_upl->upl_elems[i]->highest_page > upl->highest_page) {
					upl->highest_page = vector_upl->upl_elems[i]->highest_page;
				}
			}
			assert( pagelist_size == (sizeof(struct upl_page_info) * (vector_upl->size / PAGE_SIZE)));
		} else {
			panic("vector_upl_set_pagelist was passed a non-vectored upl\n");
		}
	} else {
		panic("vector_upl_set_pagelist was passed a NULL upl\n");
	}
}

upl_t
vector_upl_subupl_byindex(upl_t upl, uint32_t index)
{
	if (vector_upl_is_valid(upl)) {
		vector_upl_t vector_upl = upl->vector_upl;
		if (vector_upl) {
			if (index < vector_upl->num_upls) {
				return vector_upl->upl_elems[index];
			}
		} else {
			panic("vector_upl_subupl_byindex was passed a non-vectored upl\n");
		}
	}
	return NULL;
}

upl_t
vector_upl_subupl_byoffset(upl_t upl, upl_offset_t *upl_offset, upl_size_t *upl_size)
{
	if (vector_upl_is_valid(upl)) {
		uint32_t i = 0;
		vector_upl_t vector_upl = upl->vector_upl;

		if (vector_upl) {
			upl_t subupl = NULL;
			vector_upl_iostates_t subupl_state;

			for (i = 0; i < vector_upl->num_upls; i++) {
				subupl = vector_upl->upl_elems[i];
				subupl_state = vector_upl->upl_iostates[i];
				if (*upl_offset <= (subupl_state.offset + subupl_state.size - 1)) {
					                                                             
                                                                  
                           
        
					if (subupl == NULL) {
						return NULL;
					}
					if ((subupl_state.offset + subupl_state.size) < (*upl_offset + *upl_size)) {
						*upl_size = (subupl_state.offset + subupl_state.size) - *upl_offset;
						if (*upl_size > subupl_state.size) {
							*upl_size = subupl_state.size;
						}
					}
					if (*upl_offset >= subupl_state.offset) {
						*upl_offset -= subupl_state.offset;
					} else if (i) {
						panic("Vector UPL offset miscalculation\n");
					}
					return subupl;
				}
			}
		} else {
			panic("vector_upl_subupl_byoffset was passed a non-vectored UPL\n");
		}
	}
	return NULL;
}

void
vector_upl_get_submap(upl_t upl, vm_map_t *v_upl_submap, vm_offset_t *submap_dst_addr)
{
	*v_upl_submap = NULL;

	if (vector_upl_is_valid(upl)) {
		vector_upl_t vector_upl = upl->vector_upl;
		if (vector_upl) {
			*v_upl_submap = vector_upl->submap;
			*submap_dst_addr = vector_upl->submap_dst_addr;
		} else {
			panic("vector_upl_get_submap was passed a non-vectored UPL\n");
		}
	} else {
		panic("vector_upl_get_submap was passed a null UPL\n");
	}
}

void
vector_upl_set_submap(upl_t upl, vm_map_t submap, vm_offset_t submap_dst_addr)
{
	if (vector_upl_is_valid(upl)) {
		vector_upl_t vector_upl = upl->vector_upl;
		if (vector_upl) {
			vector_upl->submap = submap;
			vector_upl->submap_dst_addr = submap_dst_addr;
		} else {
			panic("vector_upl_get_submap was passed a non-vectored UPL\n");
		}
	} else {
		panic("vector_upl_get_submap was passed a NULL UPL\n");
	}
}

void
vector_upl_set_iostate(upl_t upl, upl_t subupl, upl_offset_t offset, upl_size_t size)
{
	if (vector_upl_is_valid(upl)) {
		uint32_t i = 0;
		vector_upl_t vector_upl = upl->vector_upl;

		if (vector_upl) {
			for (i = 0; i < vector_upl->num_upls; i++) {
				if (vector_upl->upl_elems[i] == subupl) {
					break;
				}
			}

			if (i == vector_upl->num_upls) {
				panic("setting sub-upl iostate when none exists");
			}

			vector_upl->upl_iostates[i].offset = offset;
			if (size < PAGE_SIZE) {
				size = PAGE_SIZE;
			}
			vector_upl->upl_iostates[i].size = size;
		} else {
			panic("vector_upl_set_iostate was passed a non-vectored UPL\n");
		}
	} else {
		panic("vector_upl_set_iostate was passed a NULL UPL\n");
	}
}

void
vector_upl_get_iostate(upl_t upl, upl_t subupl, upl_offset_t *offset, upl_size_t *size)
{
	if (vector_upl_is_valid(upl)) {
		uint32_t i = 0;
		vector_upl_t vector_upl = upl->vector_upl;

		if (vector_upl) {
			for (i = 0; i < vector_upl->num_upls; i++) {
				if (vector_upl->upl_elems[i] == subupl) {
					break;
				}
			}

			if (i == vector_upl->num_upls) {
				panic("getting sub-upl iostate when none exists");
			}

			*offset = vector_upl->upl_iostates[i].offset;
			*size = vector_upl->upl_iostates[i].size;
		} else {
			panic("vector_upl_get_iostate was passed a non-vectored UPL\n");
		}
	} else {
		panic("vector_upl_get_iostate was passed a NULL UPL\n");
	}
}

void
vector_upl_get_iostate_byindex(upl_t upl, uint32_t index, upl_offset_t *offset, upl_size_t *size)
{
	if (vector_upl_is_valid(upl)) {
		vector_upl_t vector_upl = upl->vector_upl;
		if (vector_upl) {
			if (index < vector_upl->num_upls) {
				*offset = vector_upl->upl_iostates[index].offset;
				*size = vector_upl->upl_iostates[index].size;
			} else {
				*offset = *size = 0;
			}
		} else {
			panic("vector_upl_get_iostate_byindex was passed a non-vectored UPL\n");
		}
	} else {
		panic("vector_upl_get_iostate_byindex was passed a NULL UPL\n");
	}
}

upl_page_info_t *
upl_get_internal_vectorupl_pagelist(upl_t upl)
{
	return ((vector_upl_t)(upl->vector_upl))->pagelist;
}

void *
upl_get_internal_vectorupl(upl_t upl)
{
	return upl->vector_upl;
}

vm_size_t
upl_get_internal_pagelist_offset(void)
{
	return sizeof(struct upl);
}

void
upl_clear_dirty(
	upl_t           upl,
	boolean_t       value)
{
	if (value) {
		upl->flags |= UPL_CLEAR_DIRTY;
	} else {
		upl->flags &= ~UPL_CLEAR_DIRTY;
	}
}

void
upl_set_referenced(
	upl_t           upl,
	boolean_t       value)
{
	upl_lock(upl);
	if (value) {
		upl->ext_ref_count++;
	} else {
		if (!upl->ext_ref_count) {
			panic("upl_set_referenced not %p\n", upl);
		}
		upl->ext_ref_count--;
	}
	upl_unlock(upl);
}

#if CONFIG_IOSCHED
void
upl_set_blkno(
	upl_t           upl,
	vm_offset_t     upl_offset,
	int             io_size,
	int64_t         blkno)
{
	int i, j;
	if ((upl->flags & UPL_EXPEDITE_SUPPORTED) == 0) {
		return;
	}

	assert(upl->upl_reprio_info != 0);
	for (i = (int)(upl_offset / PAGE_SIZE), j = 0; j < io_size; i++, j += PAGE_SIZE) {
		UPL_SET_REPRIO_INFO(upl, i, blkno, io_size);
	}
}
#endif

void inline
memoryshot(unsigned int event, unsigned int control)
{
	if (vm_debug_events) {
		KERNEL_DEBUG_CONSTANT1((MACHDBG_CODE(DBG_MACH_VM_PRESSURE, event)) | control,
		    vm_page_active_count, vm_page_inactive_count,
		    vm_page_free_count, vm_page_speculative_count,
		    vm_page_throttled_count);
	} else {
		(void) event;
		(void) control;
	}
}

#ifdef MACH_BSD

boolean_t
upl_device_page(upl_page_info_t *upl)
{
	return UPL_DEVICE_PAGE(upl);
}
boolean_t
upl_page_present(upl_page_info_t *upl, int index)
{
	return UPL_PAGE_PRESENT(upl, index);
}
boolean_t
upl_speculative_page(upl_page_info_t *upl, int index)
{
	return UPL_SPECULATIVE_PAGE(upl, index);
}
boolean_t
upl_dirty_page(upl_page_info_t *upl, int index)
{
	return UPL_DIRTY_PAGE(upl, index);
}
boolean_t
upl_valid_page(upl_page_info_t *upl, int index)
{
	return UPL_VALID_PAGE(upl, index);
}
ppnum_t
upl_phys_page(upl_page_info_t *upl, int index)
{
	return UPL_PHYS_PAGE(upl, index);
}

void
upl_page_set_mark(upl_page_info_t *upl, int index, boolean_t v)
{
	upl[index].mark = v;
}

boolean_t
upl_page_get_mark(upl_page_info_t *upl, int index)
{
	return upl[index].mark;
}

void
vm_countdirtypages(void)
{
	vm_page_t m;
	int dpages;
	int pgopages;
	int precpages;


	dpages = 0;
	pgopages = 0;
	precpages = 0;

	vm_page_lock_queues();
	m = (vm_page_t) vm_page_queue_first(&vm_page_queue_inactive);
	do {
		if (m == (vm_page_t)0) {
			break;
		}

		if (m->vmp_dirty) {
			dpages++;
		}
		if (m->vmp_free_when_done) {
			pgopages++;
		}
		if (m->vmp_precious) {
			precpages++;
		}

		assert(VM_PAGE_OBJECT(m) != kernel_object);
		m = (vm_page_t) vm_page_queue_next(&m->vmp_pageq);
		if (m == (vm_page_t)0) {
			break;
		}
	} while (!vm_page_queue_end(&vm_page_queue_inactive, (vm_page_queue_entry_t) m));
	vm_page_unlock_queues();

	vm_page_lock_queues();
	m = (vm_page_t) vm_page_queue_first(&vm_page_queue_throttled);
	do {
		if (m == (vm_page_t)0) {
			break;
		}

		dpages++;
		assert(m->vmp_dirty);
		assert(!m->vmp_free_when_done);
		assert(VM_PAGE_OBJECT(m) != kernel_object);
		m = (vm_page_t) vm_page_queue_next(&m->vmp_pageq);
		if (m == (vm_page_t)0) {
			break;
		}
	} while (!vm_page_queue_end(&vm_page_queue_throttled, (vm_page_queue_entry_t) m));
	vm_page_unlock_queues();

	vm_page_lock_queues();
	m = (vm_page_t) vm_page_queue_first(&vm_page_queue_anonymous);
	do {
		if (m == (vm_page_t)0) {
			break;
		}

		if (m->vmp_dirty) {
			dpages++;
		}
		if (m->vmp_free_when_done) {
			pgopages++;
		}
		if (m->vmp_precious) {
			precpages++;
		}

		assert(VM_PAGE_OBJECT(m) != kernel_object);
		m = (vm_page_t) vm_page_queue_next(&m->vmp_pageq);
		if (m == (vm_page_t)0) {
			break;
		}
	} while (!vm_page_queue_end(&vm_page_queue_anonymous, (vm_page_queue_entry_t) m));
	vm_page_unlock_queues();

	printf("IN Q: %d : %d : %d\n", dpages, pgopages, precpages);

	dpages = 0;
	pgopages = 0;
	precpages = 0;

	vm_page_lock_queues();
	m = (vm_page_t) vm_page_queue_first(&vm_page_queue_active);

	do {
		if (m == (vm_page_t)0) {
			break;
		}
		if (m->vmp_dirty) {
			dpages++;
		}
		if (m->vmp_free_when_done) {
			pgopages++;
		}
		if (m->vmp_precious) {
			precpages++;
		}

		assert(VM_PAGE_OBJECT(m) != kernel_object);
		m = (vm_page_t) vm_page_queue_next(&m->vmp_pageq);
		if (m == (vm_page_t)0) {
			break;
		}
	} while (!vm_page_queue_end(&vm_page_queue_active, (vm_page_queue_entry_t) m));
	vm_page_unlock_queues();

	printf("AC Q: %d : %d : %d\n", dpages, pgopages, precpages);
}
#endif               


#if CONFIG_IOSCHED
int
upl_get_cached_tier(upl_t  upl)
{
	assert(upl);
	if (upl->flags & UPL_TRACKED_BY_OBJECT) {
		return upl->upl_priority;
	}
	return -1;
}
#endif                     


void
upl_callout_iodone(upl_t upl)
{
	struct upl_io_completion *upl_ctx = upl->upl_iodone;

	if (upl_ctx) {
		void    (*iodone_func)(void *, int) = upl_ctx->io_done;

		assert(upl_ctx->io_done);

		(*iodone_func)(upl_ctx->io_context, upl_ctx->io_error);
	}
}

void
upl_set_iodone(upl_t upl, void *upl_iodone)
{
	upl->upl_iodone = (struct upl_io_completion *)upl_iodone;
}

void
upl_set_iodone_error(upl_t upl, int error)
{
	struct upl_io_completion *upl_ctx = upl->upl_iodone;

	if (upl_ctx) {
		upl_ctx->io_error = error;
	}
}


ppnum_t
upl_get_highest_page(
	upl_t                      upl)
{
	return upl->highest_page;
}

upl_size_t
upl_get_size(
	upl_t                      upl)
{
	return upl_adjusted_size(upl, PAGE_MASK);
}

upl_size_t
upl_adjusted_size(
	upl_t upl,
	vm_map_offset_t pgmask)
{
	vm_object_offset_t start_offset, end_offset;

	start_offset = trunc_page_mask_64(upl->u_offset, pgmask);
	end_offset = round_page_mask_64(upl->u_offset + upl->u_size, pgmask);

	return (upl_size_t)(end_offset - start_offset);
}

vm_object_offset_t
upl_adjusted_offset(
	upl_t upl,
	vm_map_offset_t pgmask)
{
	return trunc_page_mask_64(upl->u_offset, pgmask);
}

vm_object_offset_t
upl_get_data_offset(
	upl_t upl)
{
	return upl->u_offset - upl_adjusted_offset(upl, PAGE_MASK);
}

upl_t
upl_associated_upl(upl_t upl)
{
	return upl->associated_upl;
}

void
upl_set_associated_upl(upl_t upl, upl_t associated_upl)
{
	upl->associated_upl = associated_upl;
}

struct vnode *
upl_lookup_vnode(upl_t upl)
{
	if (!upl->map_object->internal) {
		return vnode_pager_lookup_vnode(upl->map_object->pager);
	} else {
		return NULL;
	}
}

#if UPL_DEBUG
kern_return_t
upl_ubc_alias_set(upl_t upl, uintptr_t alias1, uintptr_t alias2)
{
	upl->ubc_alias1 = alias1;
	upl->ubc_alias2 = alias2;
	return KERN_SUCCESS;
}
int
upl_ubc_alias_get(upl_t upl, uintptr_t * al, uintptr_t * al2)
{
	if (al) {
		*al = upl->ubc_alias1;
	}
	if (al2) {
		*al2 = upl->ubc_alias2;
	}
	return KERN_SUCCESS;
}
#endif                

#if VM_PRESSURE_EVENTS
  
                     
   
extern boolean_t vm_compressor_low_on_space(void);

boolean_t
VM_PRESSURE_NORMAL_TO_WARNING(void)
{
	if (!VM_CONFIG_COMPRESSOR_IS_ACTIVE) {
		                                         
		if (memorystatus_available_pages < memorystatus_available_pages_pressure) {
			                                 
			if (memorystatus_frozen_count == 0) {
				                                               
				if (memorystatus_suspended_count < MEMORYSTATUS_SUSPENDED_THRESHOLD) {
					return TRUE;
				}
			}
		}
		return FALSE;
	} else {
		return (AVAILABLE_NON_COMPRESSED_MEMORY < VM_PAGE_COMPRESSOR_COMPACT_THRESHOLD) ? 1 : 0;
	}
}

boolean_t
VM_PRESSURE_WARNING_TO_CRITICAL(void)
{
	if (!VM_CONFIG_COMPRESSOR_IS_ACTIVE) {
		                                         
		if (memorystatus_available_pages < memorystatus_available_pages_critical) {
			return TRUE;
		}
		return FALSE;
	} else {
		return vm_compressor_low_on_space() || (AVAILABLE_NON_COMPRESSED_MEMORY < ((12 * VM_PAGE_COMPRESSOR_SWAP_UNTHROTTLE_THRESHOLD) / 10)) ? 1 : 0;
	}
}

  
                       
   
boolean_t
VM_PRESSURE_WARNING_TO_NORMAL(void)
{
	if (!VM_CONFIG_COMPRESSOR_IS_ACTIVE) {
		                                         
		unsigned int target_threshold = (unsigned int) (memorystatus_available_pages_pressure + ((15 * memorystatus_available_pages_pressure) / 100));
		if (memorystatus_available_pages > target_threshold) {
			return TRUE;
		}
		return FALSE;
	} else {
		return (AVAILABLE_NON_COMPRESSED_MEMORY > ((12 * VM_PAGE_COMPRESSOR_COMPACT_THRESHOLD) / 10)) ? 1 : 0;
	}
}

boolean_t
VM_PRESSURE_CRITICAL_TO_WARNING(void)
{
	if (!VM_CONFIG_COMPRESSOR_IS_ACTIVE) {
		                                         
		unsigned int target_threshold = (unsigned int)(memorystatus_available_pages_critical + ((15 * memorystatus_available_pages_critical) / 100));
		if (memorystatus_available_pages > target_threshold) {
			return TRUE;
		}
		return FALSE;
	} else {
		return (AVAILABLE_NON_COMPRESSED_MEMORY > ((14 * VM_PAGE_COMPRESSOR_SWAP_UNTHROTTLE_THRESHOLD) / 10)) ? 1 : 0;
	}
}
#endif                         
