/*
 * Copyright (c) 2015-2020 Apple Inc. All rights reserved.
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
 * @OSF_FREE_COPYRIGHT@
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

  
                                                                  
                                               
   
                               
                                  

#include <kern/ast.h>
#include <kern/backtrace.h>
#include <kern/kern_types.h>
#include <kern/ltable.h>
#include <kern/mach_param.h>
#include <kern/percpu.h>
#include <kern/queue.h>
#include <kern/sched_prim.h>
#include <kern/simple_lock.h>
#include <kern/spl.h>
#include <kern/waitq.h>
#include <kern/zalloc.h>
#include <kern/policy_internal.h>
#include <kern/turnstile.h>

#include <os/hash.h>
#include <libkern/OSAtomic.h>
#include <mach/sync_policy.h>
#include <vm/vm_kern.h>

#include <sys/kdebug.h>

#if defined(KEEP_WAITQ_LINK_STATS) || defined(KEEP_WAITQ_PREPOST_STATS)
#  if !CONFIG_LTABLE_STATS
#    error "You must configure LTABLE_STATS to use WAITQ_[LINK|PREPOST]_STATS"
#  endif
#  if !CONFIG_WAITQ_STATS
#    error "You must configure WAITQ_STATS to use WAITQ_[LINK|PREPOST]_STATS"
#  endif
#endif

#if CONFIG_WAITQ_DEBUG
#define wqdbg(fmt, ...) \
	printf("WQ[%s]:  " fmt "\n", __func__, ## __VA_ARGS__)
#else
#define wqdbg(fmt, ...) do { } while (0)
#endif

#ifdef WAITQ_VERBOSE_DEBUG
#define wqdbg_v(fmt, ...) \
	printf("WQ[v:%s]:  " fmt "\n", __func__, ## __VA_ARGS__)
#else
#define wqdbg_v(fmt, ...) do { } while (0)
#endif

#define wqinfo(fmt, ...) \
	printf("WQ[%s]: " fmt "\n", __func__,  ## __VA_ARGS__)

#define wqerr(fmt, ...) \
	printf("WQ[%s] ERROR: " fmt "\n", __func__, ## __VA_ARGS__)

  
                               
   
static thread_t waitq_select_one_locked(struct waitq *waitq, event64_t event,
    uint64_t *reserved_preposts,
    int priority, spl_t *spl);

static kern_return_t waitq_select_thread_locked(struct waitq *waitq,
    event64_t event,
    thread_t thread, spl_t *spl);

ZONE_DECLARE(waitq_set_zone, "waitq sets",
    sizeof(struct waitq_set), ZC_NOENCRYPT);

                         
#define WQP_CACHE_MAX   50
struct wqp_cache {
	uint64_t        head;
	unsigned int    avail;
};
static struct wqp_cache PERCPU_DATA(wqp_cache);

#define P2ROUNDUP(x, align) (-(-((uint32_t)(x)) & -(align)))
#define ROUNDDOWN(x, y)  (((x)/(y))*(y))


#if CONFIG_LTABLE_STATS || CONFIG_WAITQ_STATS
static __inline__ void waitq_grab_backtrace(uintptr_t bt[NWAITQ_BTFRAMES], int skip);
#endif

LCK_GRP_DECLARE(waitq_lck_grp, "waitq");

#if __arm64__

#define waitq_lock_to(wq, to) \
	(hw_lock_bit_to(&(wq)->waitq_interlock, LCK_ILOCK, to, &waitq_lck_grp))

#define waitq_lock_unlock(wq) \
	(hw_unlock_bit(&(wq)->waitq_interlock, LCK_ILOCK))

#define waitq_lock_init(wq) \
	(wq->waitq_interlock = 0)

#else

#define waitq_lock_to(wq, to) \
	(hw_lock_to(&(wq)->waitq_interlock, to, &waitq_lck_grp))

#define waitq_lock_unlock(wq) \
	(hw_lock_unlock(&(wq)->waitq_interlock))

#define waitq_lock_init(wq) \
	(hw_lock_init(&(wq)->waitq_interlock))

#endif                 

  
                                                            
                        
   
extern void waitq_set__CALLING_PREPOST_HOOK__(waitq_set_prepost_hook_t *ctx);

#define DEFAULT_MIN_FREE_TABLE_ELEM    100
static uint32_t g_min_free_table_elem;
static uint32_t g_min_free_cache;


                                                                         
  
                                  
  
                                                                            
static struct link_table g_wqlinktable;

enum wq_link_type {
	WQL_ALL     = -1,
	WQL_FREE    = LT_FREE,
	WQL_WQS     = LT_ELEM,
	WQL_LINK    = LT_LINK,
};

struct waitq_link {
	struct lt_elem wqte;

	union {
		                                   
		struct {
			struct waitq_set *wql_set;
			                                      
		} wql_wqs;

		                                    
		struct {
			uint64_t          left_setid;
			uint64_t          right_setid;
		} wql_link;
	};
#ifdef KEEP_WAITQ_LINK_STATS
	thread_t  sl_alloc_th;
	task_t    sl_alloc_task;
	uintptr_t sl_alloc_bt[NWAITQ_BTFRAMES];
	uint64_t  sl_alloc_ts;
	uintptr_t sl_invalidate_bt[NWAITQ_BTFRAMES];
	uint64_t  sl_invalidate_ts;
	uintptr_t sl_mkvalid_bt[NWAITQ_BTFRAMES];
	uint64_t  sl_mkvalid_ts;
	uint64_t  sl_free_ts;
#endif
};
#if !defined(KEEP_WAITQ_LINK_STATS)
static_assert((sizeof(struct waitq_link) & (sizeof(struct waitq_link) - 1)) == 0,
    "waitq_link struct must be a power of two!");
#endif

#define wql_refcnt(link) \
	(lt_bits_refcnt((link)->wqte.lt_bits))

#define wql_type(link) \
	(lt_bits_type((link)->wqte.lt_bits))

#define wql_mkvalid(link) \
	do { \
	        lt_elem_mkvalid(&(link)->wqte); \
	        wql_do_mkvalid_stats(&(link)->wqte); \
	} while (0)

#define wql_is_valid(link) \
	lt_bits_valid((link)->wqte.lt_bits)

#define wql_setid wqte.lt_id

#define WQL_WQS_POISON         ((void *)(0xf00df00d))
#define WQL_LINK_POISON        (0x0bad0badffffffffull)

static void
wql_poison(struct link_table *table, struct lt_elem *elem)
{
	struct waitq_link *link = (struct waitq_link *)elem;
	(void)table;

	switch (wql_type(link)) {
	case WQL_WQS:
		link->wql_wqs.wql_set = WQL_WQS_POISON;
		break;
	case WQL_LINK:
		link->wql_link.left_setid = WQL_LINK_POISON;
		link->wql_link.right_setid = WQL_LINK_POISON;
		break;
	default:
		break;
	}
#ifdef KEEP_WAITQ_LINK_STATS
	memset(link->sl_alloc_bt, 0, sizeof(link->sl_alloc_bt));
	link->sl_alloc_ts = 0;
	memset(link->sl_mkvalid_bt, 0, sizeof(link->sl_mkvalid_bt));
	link->sl_mkvalid_ts = 0;

	link->sl_alloc_th = THREAD_NULL;
	                                                    

	link->sl_free_ts = mach_absolute_time();
#endif
}

#ifdef KEEP_WAITQ_LINK_STATS
static __inline__ void
wql_do_alloc_stats(struct lt_elem *elem)
{
	if (elem) {
		struct waitq_link *link = (struct waitq_link *)elem;
		memset(link->sl_alloc_bt, 0, sizeof(link->sl_alloc_bt));
		waitq_grab_backtrace(link->sl_alloc_bt, 0);
		link->sl_alloc_th = current_thread();
		link->sl_alloc_task = current_task();

		assert(link->sl_alloc_ts == 0);
		link->sl_alloc_ts = mach_absolute_time();

		memset(link->sl_invalidate_bt, 0, sizeof(link->sl_invalidate_bt));
		link->sl_invalidate_ts = 0;
	}
}

static __inline__ void
wql_do_invalidate_stats(struct lt_elem *elem)
{
	struct waitq_link *link = (struct waitq_link *)elem;

	if (!elem) {
		return;
	}

	assert(link->sl_mkvalid_ts > 0);

	memset(link->sl_invalidate_bt, 0, sizeof(link->sl_invalidate_bt));
	link->sl_invalidate_ts = mach_absolute_time();
	waitq_grab_backtrace(link->sl_invalidate_bt, 0);
}

static __inline__ void
wql_do_mkvalid_stats(struct lt_elem *elem)
{
	struct waitq_link *link = (struct waitq_link *)elem;

	if (!elem) {
		return;
	}

	memset(link->sl_mkvalid_bt, 0, sizeof(link->sl_mkvalid_bt));
	link->sl_mkvalid_ts = mach_absolute_time();
	waitq_grab_backtrace(link->sl_mkvalid_bt, 0);
}
#else
#define wql_do_alloc_stats(e)
#define wql_do_invalidate_stats(e)
#define wql_do_mkvalid_stats(e)
#endif                            

static void
wql_init(void)
{
	uint32_t tablesz = 0, max_links = 0;

	if (PE_parse_boot_argn("wql_tsize", &tablesz, sizeof(tablesz)) != TRUE) {
		tablesz = (uint32_t)g_lt_max_tbl_size;
	}

	tablesz = P2ROUNDUP(tablesz, PAGE_SIZE);
	max_links = tablesz / sizeof(struct waitq_link);
	assert(max_links > 0 && tablesz > 0);

	                                      
	if (max_links > (LT_IDX_MAX + 1)) {
		max_links = LT_IDX_MAX + 1;
	}

	wqinfo("init linktable with max:%d elements (%d bytes)",
	    max_links, tablesz);
	ltable_init(&g_wqlinktable, "wqslab.wql", max_links,
	    sizeof(struct waitq_link), wql_poison);
}

static void
wql_ensure_free_space(void)
{
	if (g_wqlinktable.nelem - g_wqlinktable.used_elem < g_min_free_table_elem) {
		  
                                                                
     
		if (g_wqlinktable.used_elem <= g_wqlinktable.nelem) {
			wqdbg_v("Forcing table growth: nelem=%d, used=%d, min_free=%d",
			    g_wqlinktable.nelem, g_wqlinktable.used_elem,
			    g_min_free_table_elem);
			ltable_grow(&g_wqlinktable, g_min_free_table_elem);
		}
	}
}

static struct waitq_link *
wql_alloc_link(int type)
{
	struct lt_elem *elem;

	elem = ltable_alloc_elem(&g_wqlinktable, type, 1, 0);
	wql_do_alloc_stats(elem);
	return (struct waitq_link *)elem;
}

static void
wql_realloc_link(struct waitq_link *link, int type)
{
	ltable_realloc_elem(&g_wqlinktable, &link->wqte, type);
#ifdef KEEP_WAITQ_LINK_STATS
	memset(link->sl_alloc_bt, 0, sizeof(link->sl_alloc_bt));
	link->sl_alloc_ts = 0;
	wql_do_alloc_stats(&link->wqte);

	memset(link->sl_invalidate_bt, 0, sizeof(link->sl_invalidate_bt));
	link->sl_invalidate_ts = 0;
#endif
}

static void
wql_invalidate(struct waitq_link *link)
{
	lt_elem_invalidate(&link->wqte);
	wql_do_invalidate_stats(&link->wqte);
}

static struct waitq_link *
wql_get_link(uint64_t setid)
{
	struct lt_elem *elem;

	elem = ltable_get_elem(&g_wqlinktable, setid);
	return (struct waitq_link *)elem;
}

static void
wql_put_link(struct waitq_link *link)
{
	if (!link) {
		return;
	}
	ltable_put_elem(&g_wqlinktable, (struct lt_elem *)link);
}

static struct waitq_link *
wql_get_reserved(uint64_t setid, int type)
{
	struct lt_elem *elem;

	elem = lt_elem_list_first(&g_wqlinktable, setid);
	if (!elem) {
		return NULL;
	}
	ltable_realloc_elem(&g_wqlinktable, elem, type);
	return (struct waitq_link *)elem;
}


static inline int waitq_maybe_remove_link(struct waitq *waitq,
    uint64_t setid,
    struct waitq_link *parent,
    struct waitq_link *left,
    struct waitq_link *right);

enum {
	LINK_WALK_ONE_LEVEL = 0,
	LINK_WALK_FULL_DAG  = 1,
	LINK_WALK_FULL_DAG_UNLOCKED = 2,
};

typedef int (*wql_callback_func)(struct waitq *waitq, void *ctx,
    struct waitq_link *link);

   
                                                                                        
  
              
                            
                                
                                                       
                                                            
                                                   
  
         
                                                                 
                                                                    
                                                                      
                               
  
                                                                       
                                                                         
                 
  
                                         
                            
                           
                            
                                   
                                  
                                   
                                        
                                     
                                         
                          
                       
                                                    
                                                            
  
                          
                           
                            
                                   
                                  
                                     
  
                                     
                                                                
                                                                          
                                                                 
                                                                   
                                                                      
                                                                  
                                                           
                                             
                                                            
                                              
   
static __attribute__((noinline))
int
walk_waitq_links(int walk_type, struct waitq *waitq,
    uint64_t setid, int link_type,
    void *ctx, wql_callback_func cb)
{
	struct waitq_link *link;
	uint64_t nextid;
	int wqltype;

	link = wql_get_link(setid);

	                  
	if (!link) {
		return WQ_ITERATE_CONTINUE;
	}

	setid = nextid = 0;
	wqltype = wql_type(link);
	if (wqltype == WQL_LINK) {
		setid  = link->wql_link.left_setid;
		nextid = link->wql_link.right_setid;
	}

	  
                                                                
                                                        
                                                      
                                                            
    
	if (link_type == WQL_ALL || link_type == wqltype) {
		                                     
		int ret = cb(waitq, ctx, link);
		if (ret != WQ_ITERATE_CONTINUE) {
			wql_put_link(link);
			return ret;
		}
	}

	if (wqltype == WQL_WQS &&
	    (walk_type == LINK_WALK_FULL_DAG ||
	    walk_type == LINK_WALK_FULL_DAG_UNLOCKED)) {
		  
                                                           
                                                           
                                         
     
		struct waitq_set *wqset = link->wql_wqs.wql_set;
		int ret = WQ_ITERATE_CONTINUE;
		int should_unlock = 0;
		uint64_t wqset_setid = 0;

		if (waitq_set_is_valid(wqset) && walk_type == LINK_WALK_FULL_DAG) {
			assert(!waitq_irq_safe(&wqset->wqset_q));
			waitq_set_lock(wqset);
			should_unlock = 1;
		}

		  
                                                      
                                            
     
		if (wqset->wqset_id != link->wql_setid.id) {
			                                                  
			if (should_unlock) {
				waitq_set_unlock(wqset);
			}
			wql_put_link(link);
			return WQ_ITERATE_CONTINUE;
		}

		wqset_setid = wqset->wqset_q.waitq_set_id;

		if (wqset_setid > 0) {
			ret = walk_waitq_links(walk_type, &wqset->wqset_q,
			    wqset_setid, link_type, ctx, cb);
		}
		if (should_unlock) {
			waitq_set_unlock(wqset);
		}
		if (ret != WQ_ITERATE_CONTINUE) {
			wql_put_link(link);
			return ret;
		}
	}

	wql_put_link(link);

	                                        
	if (setid) {
		int ret = walk_waitq_links(walk_type, waitq, setid, link_type, ctx, cb);
		if (ret != WQ_ITERATE_CONTINUE) {
			return ret;
		}
	}

	                                         
	if (nextid) {
		return walk_waitq_links(walk_type, waitq, nextid, link_type, ctx, cb);
	}

	return WQ_ITERATE_CONTINUE;
}

                                                                         
  
                                    
  
                                                                            
static struct link_table g_prepost_table;

enum wq_prepost_type {
	WQP_FREE  = LT_FREE,
	WQP_WQ    = LT_ELEM,
	WQP_POST  = LT_LINK,
};

struct wq_prepost {
	struct lt_elem wqte;

	union {
		                                  
		struct {
			struct waitq *wqp_wq_ptr;
		} wqp_wq;
		                                    
		struct {
			uint64_t      wqp_next_id;
			uint64_t      wqp_wq_id;
		} wqp_post;
	};
#ifdef KEEP_WAITQ_PREPOST_STATS
	thread_t  wqp_alloc_th;
	task_t    wqp_alloc_task;
	uintptr_t wqp_alloc_bt[NWAITQ_BTFRAMES];
#endif
};
#if !defined(KEEP_WAITQ_PREPOST_STATS)
static_assert((sizeof(struct wq_prepost) & (sizeof(struct wq_prepost) - 1)) == 0,
    "wq_prepost struct must be a power of two!");
#endif

#define wqp_refcnt(wqp) \
	(lt_bits_refcnt((wqp)->wqte.lt_bits))

#define wqp_type(wqp) \
	(lt_bits_type((wqp)->wqte.lt_bits))

#define wqp_set_valid(wqp) \
	lt_elem_mkvalid(&(wqp)->wqte)

#define wqp_is_valid(wqp) \
	lt_bits_valid((wqp)->wqte.lt_bits)

#define wqp_prepostid wqte.lt_id

#define WQP_WQ_POISON              (0x0bad0badffffffffull)
#define WQP_POST_POISON            (0xf00df00df00df00d)

static void
wqp_poison(struct link_table *table, struct lt_elem *elem)
{
	struct wq_prepost *wqp = (struct wq_prepost *)elem;
	(void)table;

	switch (wqp_type(wqp)) {
	case WQP_WQ:
		break;
	case WQP_POST:
		wqp->wqp_post.wqp_next_id = WQP_POST_POISON;
		wqp->wqp_post.wqp_wq_id = WQP_POST_POISON;
		break;
	default:
		break;
	}
}

#ifdef KEEP_WAITQ_PREPOST_STATS
static __inline__ void
wqp_do_alloc_stats(struct lt_elem *elem)
{
	if (!elem) {
		return;
	}

	struct wq_prepost *wqp = (struct wq_prepost *)elem;
	uintptr_t alloc_bt[sizeof(wqp->wqp_alloc_bt)];

	waitq_grab_backtrace(alloc_bt, NWAITQ_BTFRAMES);

	                                                        
	for (;;) {
		memcpy(wqp->wqp_alloc_bt, alloc_bt, sizeof(alloc_bt));
		wqp->wqp_alloc_th = current_thread();
		wqp->wqp_alloc_task = current_task();
		wqp = (struct wq_prepost *)lt_elem_list_next(&g_prepost_table, &wqp->wqte);
		if (!wqp) {
			break;
		}
	}
}
#else
#define wqp_do_alloc_stats(e)
#endif                            

static void
wqp_init(void)
{
	uint32_t tablesz = 0, max_wqp = 0;

	if (PE_parse_boot_argn("wqp_tsize", &tablesz, sizeof(tablesz)) != TRUE) {
		tablesz = (uint32_t)g_lt_max_tbl_size;
	}

	tablesz = P2ROUNDUP(tablesz, PAGE_SIZE);
	max_wqp = tablesz / sizeof(struct wq_prepost);
	assert(max_wqp > 0 && tablesz > 0);

	                                      
	if (max_wqp > (LT_IDX_MAX + 1)) {
		max_wqp = LT_IDX_MAX + 1;
	}

	wqinfo("init prepost table with max:%d elements (%d bytes)",
	    max_wqp, tablesz);
	ltable_init(&g_prepost_table, "wqslab.prepost", max_wqp,
	    sizeof(struct wq_prepost), wqp_poison);
}

  
                            
   
static void
wq_prepost_refill_cpu_cache(uint32_t nalloc)
{
	struct lt_elem *new_head, *old_head;
	struct wqp_cache *cache;

	                                                     
	if (get_preemption_level() != 0) {
		return;
	}

	new_head = ltable_alloc_elem(&g_prepost_table,
	    LT_RESERVED, nalloc, 1);
	if (new_head == NULL) {
		return;
	}

	disable_preemption();
	cache = PERCPU_GET(wqp_cache);

	                                                               
	if (cache->avail >= WQP_CACHE_MAX) {
		lt_elem_list_release(&g_prepost_table, new_head, LT_RESERVED);
		enable_preemption();
		return;
	}

	cache->avail += nalloc;
	if (cache->head == 0 || cache->head == LT_IDX_MAX) {
		cache->head = new_head->lt_id.id;
		goto out;
	}

	old_head = lt_elem_list_first(&g_prepost_table, cache->head);
	(void)lt_elem_list_link(&g_prepost_table, new_head, old_head);
	cache->head = new_head->lt_id.id;

out:
	enable_preemption();
	return;
}

static void
wq_prepost_ensure_free_space(void)
{
	uint32_t free_elem;
	uint32_t min_free;
	struct wqp_cache *cache;

	if (g_min_free_cache == 0) {
		g_min_free_cache = (WQP_CACHE_MAX * ml_wait_max_cpus());
	}

	  
                                                                 
    
	disable_preemption();
	cache = PERCPU_GET(wqp_cache);
	free_elem = cache->avail;
	enable_preemption();

	if (free_elem < (WQP_CACHE_MAX / 3)) {
		wq_prepost_refill_cpu_cache(WQP_CACHE_MAX - free_elem);
	}

	  
                                                                   
    
	free_elem = g_prepost_table.nelem - g_prepost_table.used_elem;
	min_free = g_min_free_table_elem + g_min_free_cache;
	if (free_elem < min_free) {
		  
                                                                
     
		if (g_prepost_table.used_elem <= g_prepost_table.nelem) {
			wqdbg_v("Forcing table growth: nelem=%d, used=%d, min_free=%d+%d",
			    g_prepost_table.nelem, g_prepost_table.used_elem,
			    g_min_free_table_elem, g_min_free_cache);
			ltable_grow(&g_prepost_table, min_free);
		}
	}
}

static struct wq_prepost *
wq_prepost_alloc(int type, int nelem)
{
	struct lt_elem *elem;
	struct wq_prepost *wqp;
	struct wqp_cache *cache;

	if (type != LT_RESERVED) {
		goto do_alloc;
	}
	if (nelem == 0) {
		return NULL;
	}

	  
                                                                   
                                
    
	disable_preemption();
	cache = PERCPU_GET(wqp_cache);
	if (nelem <= (int)cache->avail) {
		struct lt_elem *first, *next = NULL;
		int nalloc = nelem;

		cache->avail -= nelem;

		                            
		first = lt_elem_list_first(&g_prepost_table, cache->head);

		                                                        
		for (elem = first; elem != NULL && nalloc > 0; elem = next) {
			next = lt_elem_list_next(&g_prepost_table, elem);
			if (--nalloc == 0) {
				                                  
				elem->lt_next_idx = LT_IDX_MAX;
				break;
			}
		}
		assert(nalloc == 0);
		if (!next) {
			cache->head = LT_IDX_MAX;
		} else {
			cache->head = next->lt_id.id;
		}
		                                                        
		assert(!(cache->head == LT_IDX_MAX && cache->avail > 0));
		enable_preemption();
		elem = first;
		goto out;
	}
	enable_preemption();

do_alloc:
	                                            
	elem = ltable_alloc_elem(&g_prepost_table, type, nelem, 0);
	if (!elem) {
		return NULL;
	}

out:
	wqp = (struct wq_prepost *)elem;
	wqp_do_alloc_stats(elem);
	return wqp;
}

static void
wq_prepost_invalidate(struct wq_prepost *wqp)
{
	lt_elem_invalidate(&wqp->wqte);
}

static struct wq_prepost *
wq_prepost_get(uint64_t wqp_id)
{
	struct lt_elem *elem;

	elem = ltable_get_elem(&g_prepost_table, wqp_id);
	return (struct wq_prepost *)elem;
}

static void
wq_prepost_put(struct wq_prepost *wqp)
{
	ltable_put_elem(&g_prepost_table, (struct lt_elem *)wqp);
}

static int
wq_prepost_rlink(struct wq_prepost *parent, struct wq_prepost *child)
{
	return lt_elem_list_link(&g_prepost_table, &parent->wqte, &child->wqte);
}

static struct wq_prepost *
wq_prepost_get_rnext(struct wq_prepost *head)
{
	struct lt_elem *elem;
	struct wq_prepost *wqp;
	uint64_t id;

	elem = lt_elem_list_next(&g_prepost_table, &head->wqte);
	if (!elem) {
		return NULL;
	}
	id = elem->lt_id.id;
	elem = ltable_get_elem(&g_prepost_table, id);

	if (!elem) {
		return NULL;
	}
	wqp = (struct wq_prepost *)elem;
	if (elem->lt_id.id != id ||
	    wqp_type(wqp) != WQP_POST ||
	    wqp->wqp_post.wqp_next_id != head->wqp_prepostid.id) {
		ltable_put_elem(&g_prepost_table, elem);
		return NULL;
	}

	return wqp;
}

static void
wq_prepost_reset_rnext(struct wq_prepost *wqp)
{
	(void)lt_elem_list_break(&g_prepost_table, &wqp->wqte);
}


   
                                                
  
              
                  
                                                                     
  
          
                                                              
                                                              
                                                          
   
static int
wq_prepost_remove(struct waitq_set *wqset,
    struct wq_prepost *wqp)
{
	int more_posts = 1;
	uint64_t next_id = wqp->wqp_post.wqp_next_id;
	uint64_t wqp_id = wqp->wqp_prepostid.id;
	struct wq_prepost *prev_wqp, *next_wqp;

	assert(wqp_type(wqp) == WQP_POST);
	assert(wqset->wqset_q.waitq_prepost == 1);

	if (next_id == wqp_id) {
		                                             
		wqset->wqset_prepost_id = 0;
		more_posts = 0;
		goto out;
	}

	prev_wqp = wq_prepost_get_rnext(wqp);
	assert(prev_wqp != NULL);
	assert(prev_wqp->wqp_post.wqp_next_id == wqp_id);
	assert(prev_wqp->wqp_prepostid.id != wqp_id);
	assert(wqp_type(prev_wqp) == WQP_POST);

	if (prev_wqp->wqp_prepostid.id == next_id) {
		  
                                                                
                                                         
     
		wqset->wqset_prepost_id = prev_wqp->wqp_post.wqp_wq_id;
		wq_prepost_invalidate(prev_wqp);
		wq_prepost_put(prev_wqp);
		more_posts = 0;
		goto out;
	}

	                       
	prev_wqp->wqp_post.wqp_next_id = next_id;

	                       
	next_wqp = wq_prepost_get(next_id);
	assert(next_wqp != NULL);
	assert(next_wqp != wqp);
	assert(next_wqp != prev_wqp);
	assert(wqp_type(next_wqp) == WQP_POST);

	wq_prepost_reset_rnext(next_wqp);
	wq_prepost_rlink(next_wqp, prev_wqp);

	                                                         
	if (wqp_id == wqset->wqset_prepost_id) {
		wqset->wqset_prepost_id = next_id;
	}

	wq_prepost_put(prev_wqp);
	wq_prepost_put(next_wqp);

out:
	wq_prepost_reset_rnext(wqp);
	wq_prepost_invalidate(wqp);
	return more_posts;
}

static struct wq_prepost *
wq_prepost_rfirst(uint64_t id)
{
	struct lt_elem *elem;
	elem = lt_elem_list_first(&g_prepost_table, id);
	wqp_do_alloc_stats(elem);
	return (struct wq_prepost *)(void *)elem;
}

static struct wq_prepost *
wq_prepost_rpop(uint64_t *id, int type)
{
	struct lt_elem *elem;
	elem = lt_elem_list_pop(&g_prepost_table, id, type);
	wqp_do_alloc_stats(elem);
	return (struct wq_prepost *)(void *)elem;
}

static void
wq_prepost_release_rlist(struct wq_prepost *wqp)
{
	int nelem = 0;
	struct wqp_cache *cache;
	struct lt_elem *elem;

	if (!wqp) {
		return;
	}

	elem = &wqp->wqte;

	  
                                                                      
                                
    
	disable_preemption();
	cache = PERCPU_GET(wqp_cache);
	if (cache->avail < WQP_CACHE_MAX) {
		struct lt_elem *tmp = NULL;
		if (cache->head != LT_IDX_MAX) {
			tmp = lt_elem_list_first(&g_prepost_table, cache->head);
		}
		nelem = lt_elem_list_link(&g_prepost_table, elem, tmp);
		cache->head = elem->lt_id.id;
		cache->avail += nelem;
		enable_preemption();
		return;
	}
	enable_preemption();

	                                                   
	nelem = lt_elem_list_release(&g_prepost_table, elem, LT_RESERVED);

#if CONFIG_WAITQ_STATS
	g_prepost_table.nreserved_releases += 1;
	OSDecrementAtomic64(&g_prepost_table.nreservations);
#endif
}

typedef int (*wqp_callback_func)(struct waitq_set *wqset,
    void *ctx,
    struct wq_prepost *wqp,
    struct waitq *waitq);

   
                                                                
  
              
                  
  
         
                                                                       
                                                                       
                                                                
   
static int
wq_prepost_foreach_locked(struct waitq_set *wqset,
    void *ctx, wqp_callback_func cb)
{
	int ret = WQ_ITERATE_SUCCESS;
	struct wq_prepost *wqp, *tmp_wqp;

	assert(cb != NULL);

	if (!wqset || !waitq_set_maybe_preposted(wqset)) {
		return WQ_ITERATE_SUCCESS;
	}

restart:
	wqp = wq_prepost_get(wqset->wqset_prepost_id);
	if (!wqp) {
		  
                                                           
                      
     
		wqset->wqset_prepost_id = 0;
		return WQ_ITERATE_SUCCESS;
	}

	if (wqp_type(wqp) == WQP_WQ) {
		uint64_t __assert_only wqp_id = wqp->wqp_prepostid.id;

		ret = cb(wqset, ctx, wqp, wqp->wqp_wq.wqp_wq_ptr);

		switch (ret) {
		case WQ_ITERATE_INVALIDATE_CONTINUE:
			                                                      
			assert(wqp_id == wqset->wqset_prepost_id);
			wqset->wqset_prepost_id = 0;
			OS_FALLTHROUGH;
		case WQ_ITERATE_CONTINUE:
			wq_prepost_put(wqp);
			ret = WQ_ITERATE_SUCCESS;
			break;
		case WQ_ITERATE_RESTART:
			wq_prepost_put(wqp);
			OS_FALLTHROUGH;
		case WQ_ITERATE_DROPPED:
			goto restart;
		default:
			wq_prepost_put(wqp);
			break;
		}
		return ret;
	}

	assert(wqp->wqp_prepostid.id == wqset->wqset_prepost_id);
	assert(wqp_type(wqp) == WQP_POST);

	  
                                                         
                                                           
                  
    
	tmp_wqp = wq_prepost_get_rnext(wqp);
	assert(tmp_wqp != NULL && wqp_type(tmp_wqp) == WQP_POST);

	uint64_t last_id = tmp_wqp->wqp_prepostid.id;
	wq_prepost_put(tmp_wqp);

	ret = WQ_ITERATE_SUCCESS;
	for (;;) {
		uint64_t wqp_id, first_id, next_id;

		wqp_id = wqp->wqp_prepostid.id;
		first_id = wqset->wqset_prepost_id;
		next_id = wqp->wqp_post.wqp_next_id;

		                                                 
		tmp_wqp = wq_prepost_get(wqp->wqp_post.wqp_wq_id);
		if (!tmp_wqp) {
			  
                                               
                                                 
               
      
			if (wq_prepost_remove(wqset, wqp) == 0) {
				wq_prepost_put(wqp);
				goto restart;
			}
			goto next_prepost;
		}
		assert(wqp_type(tmp_wqp) == WQP_WQ);
		  
                                                            
                                                           
                                          
     
		ret = cb(wqset, ctx, wqp, tmp_wqp->wqp_wq.wqp_wq_ptr);
		wq_prepost_put(tmp_wqp);

		switch (ret) {
		case WQ_ITERATE_CONTINUE:
			                        
			break;
		case WQ_ITERATE_INVALIDATE_CONTINUE:
			assert(next_id == wqp->wqp_post.wqp_next_id);
			if (wq_prepost_remove(wqset, wqp) == 0) {
				wq_prepost_put(wqp);
				goto restart;
			}
			goto next_prepost;
		case WQ_ITERATE_RESTART:
			wq_prepost_put(wqp);
			OS_FALLTHROUGH;
		case WQ_ITERATE_DROPPED:
			                                                       
			goto restart;
		default:
			                                                      
			goto finish_prepost_foreach;
		}

		  
                                                        
                                                                
     
		if (!wqp_is_valid(wqp) ||
		    (wqp->wqp_post.wqp_next_id != next_id) ||
		    wqset->wqset_prepost_id != first_id) {
			wq_prepost_put(wqp);
			goto restart;
		}

next_prepost:
		                                          
		if (wqp_id == last_id) {
			break;
		}

		                         
		tmp_wqp = wq_prepost_get(next_id);
		if (!tmp_wqp) {
			  
                                                   
                                                        
                                                     
                              
      
			panic("Invalid WQP_POST member 0x%llx in waitq set "
			    "0x%llx prepost list (first:%llx, "
			    "wqp:%p)",
			    next_id, wqset->wqset_id, first_id, wqp);
		}
		wq_prepost_put(wqp);
		wqp = tmp_wqp;

		assert(wqp_type(wqp) == WQP_POST);
	}

finish_prepost_foreach:
	wq_prepost_put(wqp);
	if (ret == WQ_ITERATE_CONTINUE) {
		ret = WQ_ITERATE_SUCCESS;
	}

	return ret;
}

   
                                                        
  
              
                                                                         
                                          
                                                                       
                            
  
         
                                                                         
                                                                       
                                                                         
                                            
   
static int
wq_prepost_iterate(uint64_t prepost_id,
    void *ctx, wqp_callback_func cb)
{
	int ret;
	struct wq_prepost *wqp;

	if (!prepost_id) {
		return WQ_ITERATE_SUCCESS;
	}

	wqp = wq_prepost_get(prepost_id);
	if (!wqp) {
		return WQ_ITERATE_SUCCESS;
	}

	if (wqp_type(wqp) == WQP_WQ) {
		ret = WQ_ITERATE_SUCCESS;
		if (cb) {
			ret = cb(NULL, ctx, wqp, wqp->wqp_wq.wqp_wq_ptr);
		}

		if (ret != WQ_ITERATE_DROPPED) {
			wq_prepost_put(wqp);
		}
		return ret;
	}

	assert(wqp->wqp_prepostid.id == prepost_id);
	assert(wqp_type(wqp) == WQP_POST);

	                                                          
	uint64_t next_id;

	ret = WQ_ITERATE_CONTINUE;
	do {
		struct wq_prepost *tmp_wqp;
		struct waitq *wq = NULL;

		next_id = wqp->wqp_post.wqp_next_id;

		                                                 
		tmp_wqp = wq_prepost_get(wqp->wqp_post.wqp_wq_id);
		if (tmp_wqp) {
			assert(wqp_type(tmp_wqp) == WQP_WQ);
			wq = tmp_wqp->wqp_wq.wqp_wq_ptr;
		}

		if (cb) {
			ret = cb(NULL, ctx, wqp, wq);
		}
		if (tmp_wqp) {
			wq_prepost_put(tmp_wqp);
		}

		if (ret != WQ_ITERATE_CONTINUE) {
			break;
		}

		tmp_wqp = wq_prepost_get(next_id);
		if (!tmp_wqp) {
			  
                                                         
                              
      
			ret = WQ_ITERATE_ABORTED;
			break;
		}

		wq_prepost_put(wqp);
		wqp = tmp_wqp;

		assert(wqp_type(wqp) == WQP_POST);
	} while (next_id != prepost_id);

	if (ret != WQ_ITERATE_DROPPED) {
		wq_prepost_put(wqp);
	}

	if (ret == WQ_ITERATE_CONTINUE) {
		ret = WQ_ITERATE_SUCCESS;
	}
	return ret;
}


struct _is_posted_ctx {
	struct waitq *posting_wq;
	int did_prepost;
};

static int
wq_is_preposted_on_set_cb(struct waitq_set *wqset, void *ctx,
    struct wq_prepost *wqp, struct waitq *waitq)
{
	struct _is_posted_ctx *pctx = (struct _is_posted_ctx *)ctx;

	(void)wqset;
	(void)wqp;

	  
                                                   
                                                                     
    
	if (pctx->posting_wq == waitq) {
		pctx->did_prepost = 1;
	}

	return WQ_ITERATE_CONTINUE;
}


   
                                                     
  
              
                                       
                                                     
  
              
                                  
  
                                                               
   
static int
wq_is_preposted_on_set(struct waitq *waitq, struct waitq_set *wqset)
{
	int ret;
	struct _is_posted_ctx pctx;

	  
                                                             
                                                   
    
	if (waitq->waitq_prepost_id != 0 &&
	    wqset->wqset_prepost_id == waitq->waitq_prepost_id) {
		return 1;
	}

	                                                      
	pctx.posting_wq = waitq;
	pctx.did_prepost = 0;
	ret = wq_prepost_foreach_locked(wqset, (void *)&pctx,
	    wq_is_preposted_on_set_cb);
	return pctx.did_prepost;
}

static struct wq_prepost *
wq_get_prepost_obj(uint64_t *reserved, int type)
{
	struct wq_prepost *wqp = NULL;
	  
                                                          
                                                                   
                                             
    
	if (reserved && *reserved) {
		wqp = wq_prepost_rpop(reserved, type);
		assert(wqp->wqte.lt_id.idx < g_prepost_table.nelem);
	} else {
		  
                                                       
                                  
     
		wqp = wq_prepost_alloc(type, 1);
	}

	if (wqp == NULL) {
		panic("Couldn't allocate prepost object!");
	}
	return wqp;
}


   
                                   
  
              
                                                      
                                       
                                                                        
                         
  
              
                                  
  
         
                                                               
   
static void
wq_prepost_do_post_locked(struct waitq_set *wqset,
    struct waitq *waitq,
    uint64_t *reserved)
{
	struct wq_prepost *wqp_post, *wqp_head, *wqp_tail;

	assert(waitq_held(waitq) && waitq_held(&wqset->wqset_q));

	  
                                            
                                                         
    
	if (wq_is_preposted_on_set(waitq, wqset)) {
		return;
	}

	assert(waitqs_is_linked(wqset));

	  
                                                                        
                                                                     
                                                       
    
	if (waitq->waitq_prepost_id == 0) {
		struct wq_prepost *wqp;
		wqp = wq_get_prepost_obj(reserved, WQP_WQ);
		wqp->wqp_wq.wqp_wq_ptr = waitq;
		wqp_set_valid(wqp);
		waitq->waitq_prepost_id = wqp->wqp_prepostid.id;
		wq_prepost_put(wqp);
	}

#if CONFIG_LTABLE_STATS
	g_prepost_table.npreposts += 1;
#endif

	wqdbg_v("preposting waitq %p (0x%llx) to set 0x%llx",
	    (void *)VM_KERNEL_UNSLIDE_OR_PERM(waitq),
	    waitq->waitq_prepost_id, wqset->wqset_id);

	if (wqset->wqset_prepost_id == 0) {
		                                      
		wqset->wqset_prepost_id = waitq->waitq_prepost_id;
		return;
	}

	wqp_head = wq_prepost_get(wqset->wqset_prepost_id);
	if (!wqp_head) {
		                                             
		wqset->wqset_prepost_id = waitq->waitq_prepost_id;
		return;
	}

	assert(wqp_head->wqp_prepostid.id == wqset->wqset_prepost_id);

	  
                                                                   
                                                                   
                                                                     
                                                    
    
	wqp_post = wq_get_prepost_obj(reserved, WQP_POST);

	wqp_post->wqp_post.wqp_wq_id = waitq->waitq_prepost_id;
	wqdbg_v("POST 0x%llx :: WQ 0x%llx", wqp_post->wqp_prepostid.id,
	    waitq->waitq_prepost_id);

	if (wqp_type(wqp_head) == WQP_WQ) {
		  
                                                        
                                
     
		uint64_t wqp_id = wqp_head->wqp_prepostid.id;
		wqdbg_v("set 0x%llx previous had 1 WQ prepost (0x%llx): "
		    "replacing with two POST preposts",
		    wqset->wqset_id, wqp_id);

		                            
		wq_prepost_put(wqp_head);

		                                              
		wqp_head = wq_get_prepost_obj(reserved, WQP_POST);

		                                                  
		wqp_head->wqp_post.wqp_wq_id = wqp_id;
		wqdbg_v("POST 0x%llx :: WQ 0x%llx",
		    wqp_head->wqp_prepostid.id, wqp_id);

		                                                          
		wqp_head->wqp_post.wqp_next_id = wqp_post->wqp_prepostid.id;
		                                                
		wq_prepost_rlink(wqp_head, wqp_post);

		  
                                                            
                                                           
                      
     
		wqp_post->wqp_post.wqp_next_id = wqp_head->wqp_prepostid.id;
		wq_prepost_rlink(wqp_post, wqp_head);

		                                                               
		wqp_set_valid(wqp_head);
		wqp_set_valid(wqp_post);
		wqset->wqset_prepost_id = wqp_head->wqp_prepostid.id;

		                             
		wq_prepost_put(wqp_head);
		wq_prepost_put(wqp_post);

		wqdbg_v("set 0x%llx: 0x%llx/0x%llx -> 0x%llx/0x%llx -> 0x%llx",
		    wqset->wqset_id, wqset->wqset_prepost_id,
		    wqp_head->wqp_prepostid.id, wqp_head->wqp_post.wqp_next_id,
		    wqp_post->wqp_prepostid.id,
		    wqp_post->wqp_post.wqp_next_id);
		return;
	}

	assert(wqp_type(wqp_head) == WQP_POST);

	  
                                                      
    
	wqp_tail = wq_prepost_get_rnext(wqp_head);
	assert(wqp_tail != NULL);
	assert(wqp_tail->wqp_post.wqp_next_id == wqset->wqset_prepost_id);

	  
                                 
                                                                 
    
	wq_prepost_reset_rnext(wqp_head);
	wq_prepost_rlink(wqp_head, wqp_post);

	                                                          
	wqp_post->wqp_post.wqp_next_id = wqp_head->wqp_prepostid.id;
	wq_prepost_rlink(wqp_post, wqp_tail);

	                                                                   
	wqp_tail->wqp_post.wqp_next_id = wqp_post->wqp_prepostid.id;

	wqp_set_valid(wqp_post);

	wq_prepost_put(wqp_head);
	wq_prepost_put(wqp_tail);
	wq_prepost_put(wqp_post);

	wqdbg_v("set 0x%llx (wqp:0x%llx) last_prepost:0x%llx, "
	    "new_prepost:0x%llx->0x%llx", wqset->wqset_id,
	    wqset->wqset_prepost_id, wqp_head->wqp_prepostid.id,
	    wqp_post->wqp_prepostid.id, wqp_post->wqp_post.wqp_next_id);

	return;
}


                                                                         
  
                               
  
                                                                            
#if CONFIG_LTABLE_STATS && CONFIG_WAITQ_STATS
static void
wq_table_stats(struct link_table *table, struct wq_table_stats *stats)
{
	stats->version = WAITQ_STATS_VERSION;
	stats->table_elements = table->nelem;
	stats->table_used_elems = table->used_elem;
	stats->table_elem_sz = table->elem_sz;
	stats->table_slabs = table->nslabs;
	stats->table_slab_sz = table->slab_sz;

	stats->table_num_allocs = table->nallocs;
	stats->table_num_preposts = table->npreposts;
	stats->table_num_reservations = table->nreservations;

	stats->table_max_used = table->max_used;
	stats->table_avg_used = table->avg_used;
	stats->table_max_reservations = table->max_reservations;
	stats->table_avg_reservations = table->avg_reservations;
}

void
waitq_link_stats(struct wq_table_stats *stats)
{
	if (!stats) {
		return;
	}
	wq_table_stats(&g_wqlinktable, stats);
}

void
waitq_prepost_stats(struct wq_table_stats *stats)
{
	wq_table_stats(&g_prepost_table, stats);
}
#endif


                                                                         
  
                     
  
                                                                            

static struct waitq g_boot_waitq;
static struct waitq *global_waitqs = &g_boot_waitq;
static uint32_t g_num_waitqs = 1;

  
                                       
   
#define _CAST_TO_EVENT_MASK(event)   ((uintptr_t)(event) & ((1ul << _EVENT_MASK_BITS) - 1ul))

static __inline__ uint32_t
waitq_hash(char *key, size_t length)
{
	uint32_t hash = os_hash_jenkins(key, length);

	hash &= (g_num_waitqs - 1);
	return hash;
}

                                                                    
struct waitq *
_global_eventq(char *event, size_t event_length)
{
	return &global_waitqs[waitq_hash(event, event_length)];
}

                                            
struct waitq *
global_waitq(int index)
{
	return &global_waitqs[index % g_num_waitqs];
}


#if CONFIG_LTABLE_STATS || CONFIG_WAITQ_STATS
                             
const uint32_t g_nwaitq_btframes = NWAITQ_BTFRAMES;

static __inline__ void
waitq_grab_backtrace(uintptr_t bt[NWAITQ_BTFRAMES], int skip)
{
	uintptr_t buf[NWAITQ_BTFRAMES + skip];
	if (skip < 0) {
		skip = 0;
	}
	memset(buf, 0, (NWAITQ_BTFRAMES + skip) * sizeof(uintptr_t));
	backtrace(buf, g_nwaitq_btframes + skip, NULL);
	memcpy(&bt[0], &buf[skip], NWAITQ_BTFRAMES * sizeof(uintptr_t));
}
#else               
#define waitq_grab_backtrace(...)
#endif

#if CONFIG_WAITQ_STATS

struct wq_stats g_boot_stats;
struct wq_stats *g_waitq_stats = &g_boot_stats;

static __inline__ struct wq_stats *
waitq_global_stats(struct waitq *waitq)
{
	struct wq_stats *wqs;
	uint32_t idx;

	if (!waitq_is_global(waitq)) {
		return NULL;
	}

	idx = (uint32_t)(((uintptr_t)waitq - (uintptr_t)global_waitqs) / sizeof(*waitq));
	assert(idx < g_num_waitqs);
	wqs = &g_waitq_stats[idx];
	return wqs;
}

static __inline__ void
waitq_stats_count_wait(struct waitq *waitq)
{
	struct wq_stats *wqs = waitq_global_stats(waitq);
	if (wqs != NULL) {
		wqs->waits++;
		waitq_grab_backtrace(wqs->last_wait, 2);
	}
}

static __inline__ void
waitq_stats_count_wakeup(struct waitq *waitq)
{
	struct wq_stats *wqs = waitq_global_stats(waitq);
	if (wqs != NULL) {
		wqs->wakeups++;
		waitq_grab_backtrace(wqs->last_wakeup, 2);
	}
}

static __inline__ void
waitq_stats_count_clear_wakeup(struct waitq *waitq)
{
	struct wq_stats *wqs = waitq_global_stats(waitq);
	if (wqs != NULL) {
		wqs->wakeups++;
		wqs->clears++;
		waitq_grab_backtrace(wqs->last_wakeup, 2);
	}
}

static __inline__ void
waitq_stats_count_fail(struct waitq *waitq)
{
	struct wq_stats *wqs = waitq_global_stats(waitq);
	if (wqs != NULL) {
		wqs->failed_wakeups++;
		waitq_grab_backtrace(wqs->last_failed_wakeup, 2);
	}
}
#else                          
#define waitq_stats_count_wait(q)         do { } while (0)
#define waitq_stats_count_wakeup(q)       do { } while (0)
#define waitq_stats_count_clear_wakeup(q) do { } while (0)
#define waitq_stats_count_fail(q)         do { } while (0)
#endif

int
waitq_is_valid(struct waitq *waitq)
{
	return (waitq != NULL) && waitq->waitq_isvalid;
}

int
waitq_set_is_valid(struct waitq_set *wqset)
{
	return (wqset != NULL) && wqset->wqset_q.waitq_isvalid && waitqs_is_set(wqset);
}

int
waitq_is_global(struct waitq *waitq)
{
	if (waitq >= global_waitqs && waitq < global_waitqs + g_num_waitqs) {
		return 1;
	}
	return 0;
}

int
waitq_irq_safe(struct waitq *waitq)
{
	                                                            
	return waitq->waitq_irq;
}

static inline bool
waitq_empty(struct waitq *wq)
{
	if (waitq_is_turnstile_queue(wq)) {
		return priority_queue_empty(&wq->waitq_prio_queue);
	} else if (waitq_is_turnstile_proxy(wq)) {
		struct turnstile *ts = wq->waitq_ts;
		return ts == TURNSTILE_NULL ||
		       priority_queue_empty(&ts->ts_waitq.waitq_prio_queue);
	} else {
		return queue_empty(&wq->waitq_queue);
	}
}

static struct waitq *
waitq_get_safeq(struct waitq *waitq)
{
	                                
	if (waitq_is_turnstile_proxy(waitq)) {
		struct turnstile *ts = waitq->waitq_ts;
		return ts ? &ts->ts_waitq : NULL;
	}
	return global_eventq(waitq);
}

static uint32_t
waitq_hash_size(void)
{
	uint32_t hsize, queues;

	if (PE_parse_boot_argn("wqsize", &hsize, sizeof(hsize))) {
		return hsize;
	}

	queues = thread_max / 5;
	hsize = P2ROUNDUP(queues * sizeof(struct waitq), PAGE_SIZE);

	return hsize;
}

  
                                                       
                                                         
   
static_assert(MAXPRI <= UINT8_MAX);

static inline void
waitq_thread_insert(struct waitq *wq,
    thread_t thread, boolean_t fifo)
{
	if (waitq_is_turnstile_queue(wq)) {
		turnstile_stats_update(0, TSU_TURNSTILE_BLOCK_COUNT, NULL);
		turnstile_waitq_add_thread_priority_queue(wq, thread);
	} else {
		turnstile_stats_update(0, TSU_REGULAR_WAITQ_BLOCK_COUNT, NULL);
		if (fifo) {
			enqueue_tail(&wq->waitq_queue, &thread->wait_links);
		} else {
			enqueue_head(&wq->waitq_queue, &thread->wait_links);
		}
	}
}

static inline void
waitq_thread_remove(struct waitq *wq,
    thread_t thread)
{
	if (waitq_is_turnstile_queue(wq)) {
		KERNEL_DEBUG_CONSTANT_IST(KDEBUG_TRACE,
		    (TURNSTILE_CODE(TURNSTILE_HEAP_OPERATIONS, (THREAD_REMOVED_FROM_TURNSTILE_WAITQ))) | DBG_FUNC_NONE,
		    VM_KERNEL_UNSLIDE_OR_PERM(waitq_to_turnstile(wq)),
		    thread_tid(thread),
		    0, 0, 0);
		priority_queue_remove(&wq->waitq_prio_queue, &thread->wait_prioq_links);
	} else {
		remqueue(&(thread->wait_links));
	}
}

void
waitq_bootstrap(void)
{
	kern_return_t kret;
	uint32_t whsize, qsz, tmp32;

	g_min_free_table_elem = DEFAULT_MIN_FREE_TABLE_ELEM;
	if (PE_parse_boot_argn("wqt_min_free", &tmp32, sizeof(tmp32)) == TRUE) {
		g_min_free_table_elem = tmp32;
	}
	wqdbg("Minimum free table elements: %d", tmp32);

	  
                                                               
                            
    
	whsize = waitq_hash_size();

	                                                    
	qsz = sizeof(struct waitq);
	whsize = ROUNDDOWN(whsize, qsz);
	g_num_waitqs = whsize / qsz;

	  
                                                                
                                         
    
	for (uint32_t i = 0; i < 31; i++) {
		uint32_t bit = (1 << i);
		if ((g_num_waitqs & bit) == g_num_waitqs) {
			break;
		}
		g_num_waitqs &= ~bit;
	}
	assert(g_num_waitqs > 0);

	                                                   
	whsize = P2ROUNDUP(g_num_waitqs * qsz, PAGE_SIZE);

	wqdbg("allocating %d global queues  (%d bytes)", g_num_waitqs, whsize);
	kret = kernel_memory_allocate(kernel_map, (vm_offset_t *)&global_waitqs,
	    whsize, 0, KMA_KOBJECT | KMA_NOPAGEWAIT, VM_KERN_MEMORY_WAITQ);
	if (kret != KERN_SUCCESS || global_waitqs == NULL) {
		panic("kernel_memory_allocate() failed to alloc global_waitqs"
		    ", error: %d, whsize: 0x%x", kret, whsize);
	}

#if CONFIG_WAITQ_STATS
	whsize = P2ROUNDUP(g_num_waitqs * sizeof(struct wq_stats), PAGE_SIZE);
	kret = kernel_memory_allocate(kernel_map, (vm_offset_t *)&g_waitq_stats,
	    whsize, 0, KMA_KOBJECT | KMA_NOPAGEWAIT, VM_KERN_MEMORY_WAITQ);
	if (kret != KERN_SUCCESS || global_waitqs == NULL) {
		panic("kernel_memory_allocate() failed to alloc g_waitq_stats"
		    ", error: %d, whsize: 0x%x", kret, whsize);
	}
	memset(g_waitq_stats, 0, whsize);
#endif

	for (uint32_t i = 0; i < g_num_waitqs; i++) {
		waitq_init(&global_waitqs[i], SYNC_POLICY_FIFO | SYNC_POLICY_DISABLE_IRQ);
	}

	                                            
	wql_init();

	                                               
	wqp_init();
}


                                                                         
  
                            
  
                                                                            

  
                                                             
                                                                   
                                                                  
                                              
   
                                                    
#if defined(__i386__) || defined(__x86_64__)
#define hwLockTimeOut LockTimeOutTSC
#else
#define hwLockTimeOut LockTimeOut
#endif

void
waitq_lock(struct waitq *wq)
{
	if (__improbable(waitq_lock_to(wq,
	    hwLockTimeOut * 2) == 0)) {
		boolean_t wql_acquired = FALSE;

		while (machine_timeout_suspended()) {
			mp_enable_preemption();
			wql_acquired = waitq_lock_to(wq,
			    hwLockTimeOut * 2);
			if (wql_acquired) {
				break;
			}
		}
		if (wql_acquired == FALSE) {
			panic("waitq deadlock - waitq=%p, cpu=%d\n",
			    wq, cpu_number());
		}
	}
#if defined(__x86_64__)
	pltrace(FALSE);
#endif
	assert(waitq_held(wq));
}

void
waitq_unlock(struct waitq *wq)
{
	assert(waitq_held(wq));
#if defined(__x86_64__)
	pltrace(TRUE);
#endif
	waitq_lock_unlock(wq);
}


   
                                       
  
              
                     
   
static inline void
thread_clear_waitq_state(thread_t thread)
{
	thread->waitq = NULL;
	thread->wait_event = NO_EVENT64;
	thread->at_safe_point = FALSE;
}


typedef thread_t (*waitq_select_cb)(void *ctx, struct waitq *waitq,
    int is_global, thread_t thread);

struct waitq_select_args {
	                      
	struct waitq    *posted_waitq;
	struct waitq    *waitq;
	event64_t        event;
	waitq_select_cb  select_cb;
	void            *select_ctx;
	int             priority;

	uint64_t        *reserved_preposts;

	                       
	queue_t       threadq;
	int           max_threads;
	int          *nthreads;
	spl_t        *spl;
};

static void do_waitq_select_n_locked(struct waitq_select_args *args);

   
                                                                     
  
              
                              
                                     
  
         
                                                           
                                                                 
                                                            
                                                           
                                           
   
static int
waitq_select_walk_cb(struct waitq *waitq, void *ctx,
    struct waitq_link *link)
{
	int ret = WQ_ITERATE_CONTINUE;
	struct waitq_select_args args = *((struct waitq_select_args *)ctx);
	struct waitq_set *wqset;

	(void)waitq;
	assert(wql_type(link) == WQL_WQS);

	wqset = link->wql_wqs.wql_set;
	args.waitq = &wqset->wqset_q;

	assert(!waitq_irq_safe(waitq));
	assert(!waitq_irq_safe(&wqset->wqset_q));

	waitq_set_lock(wqset);
	  
                                                       
                                  
    
	if (wqset->wqset_id != link->wql_setid.id) {
		goto out_unlock;
	}

	assert(waitqs_is_linked(wqset));

	  
                                                    
                                                             
    
	do_waitq_select_n_locked(&args);

	if (*args.nthreads > 0 || (args.threadq && !queue_empty(args.threadq))) {
		                                                                
		if (args.max_threads > 0 && *args.nthreads >= args.max_threads) {
			                                 
			ret = WQ_ITERATE_FOUND;
		}
	} else if (args.event == NO_EVENT64) {
		  
                                                   
                                                            
                                                            
    
                                                          
                                                            
                                                             
                                                               
     
		if (waitq_set_can_prepost(wqset)) {
			wq_prepost_do_post_locked(
				wqset, waitq, args.reserved_preposts);
		} else if (waitq_set_has_prepost_hook(wqset)) {
			waitq_set_prepost_hook_t *hook = wqset->wqset_prepost_hook;

			  
                                           
                                                     
                                                      
                                 
     
                                                         
                                                
                                
     
                                                       
                                                         
     
                                                         
                                                 
      

			disable_preemption();
			os_atomic_add(hook, (uint16_t)1, relaxed);
			waitq_set_unlock(wqset);

			waitq_set__CALLING_PREPOST_HOOK__(hook);

			                                                              
			os_atomic_add(hook, (uint16_t)-1, relaxed);
			enable_preemption();
			return ret;
		}
	}

out_unlock:
	waitq_set_unlock(wqset);
	return ret;
}

   
                                                                    
  
              
                                                 
  
         
                                                                     
                                                                      
                                                                              
  
                                                                    
                                                                   
                                                                     
   
static thread_t
waitq_queue_iterate_locked(struct waitq *safeq, struct waitq *waitq,
    spl_t spl, struct waitq_select_args *args,
    uint32_t *remaining_eventmask)
{
	int max_threads = args->max_threads;
	int *nthreads = args->nthreads;
	thread_t thread = THREAD_NULL;
	thread_t first_thread = THREAD_NULL;

	qe_foreach_element_safe(thread, &safeq->waitq_queue, wait_links) {
		thread_t t = THREAD_NULL;
		assert_thread_magic(thread);

		  
                                                                    
                                                                     
                                                      
                          
     
		if (thread->waitq == waitq && thread->wait_event == args->event) {
			t = thread;
			if (first_thread == THREAD_NULL) {
				first_thread = thread;
			}

			                                                     
			if (args->select_cb) {
				t = args->select_cb(args->select_ctx, waitq,
				    waitq_is_global(waitq), thread);
			}
			if (t != THREAD_NULL) {
				*nthreads += 1;
				if (args->threadq) {
					                                              
					if (*nthreads == 1) {
						*(args->spl) = (safeq != waitq) ? spl : splsched();
					}
					thread_lock(t);
					thread_clear_waitq_state(t);
					re_queue_tail(args->threadq, &t->wait_links);
				}
				                                      
				if (*nthreads >= max_threads && max_threads > 0) {
					break;
				}
			}
		}
		                                                
		if (t == THREAD_NULL) {
			*remaining_eventmask |= (thread->waitq != safeq) ?
			    _CAST_TO_EVENT_MASK(thread->waitq) : _CAST_TO_EVENT_MASK(thread->wait_event);
		}
	}

	return first_thread;
}

   
                                                                     
  
              
                                                 
  
         
                                                                             
  
                                                                             
                                                                             
                                                                             
                                                                            
                                                                                
  
                                                                             
                                                                                
                                                                             
                                                                              
                                                                      
                                                                                 
                                                     
   
static thread_t
waitq_prioq_iterate_locked(struct waitq *safeq, struct waitq *waitq,
    spl_t spl, struct waitq_select_args *args,
    uint32_t *remaining_eventmask)
{
	int max_threads = args->max_threads;
	int *nthreads = args->nthreads;
	thread_t first_thread = THREAD_NULL;
	thread_t thread = THREAD_NULL;

	  
                                                       
                                                                        
                                                                               
                                                                    
                                                                          
                                                                              
                                                                                 
   
                                                                           
                                                                        
                                                                    
    
	*remaining_eventmask = safeq->waitq_eventmask;
	boolean_t remove_op = !!(args->threadq);

	while ((max_threads <= 0) || (*nthreads < max_threads)) {
		if (priority_queue_empty(&(safeq->waitq_prio_queue))) {
			*remaining_eventmask = 0;
			break;
		}

		if (remove_op) {
			thread = priority_queue_remove_max(&safeq->waitq_prio_queue,
			    struct thread, wait_prioq_links);
		} else {
			                                                                       
			assert(max_threads == 1);
			thread = priority_queue_max(&safeq->waitq_prio_queue,
			    struct thread, wait_prioq_links);
		}
		  
                                                                       
                                               
     
		assert((thread->waitq == waitq) && (thread->wait_event == args->event));

		if (args->select_cb) {
			  
                                                                        
                                                                        
                                                    
      
			thread_t __assert_only ret_thread = args->select_cb(args->select_ctx, waitq,
			    waitq_is_global(waitq), thread);
			if (!remove_op) {
				                                                                            
				assert(ret_thread == THREAD_NULL);
			} else {
				  
                                                                                
                                                                           
                                                                           
                                                                     
                                          
       
				assert(ret_thread == thread);
			}
		}

		if (first_thread == THREAD_NULL) {
			first_thread = thread;
			  
                                                                
                                                 
      
			if (args->priority == WAITQ_PROMOTE_ON_WAKE && first_thread != THREAD_NULL && waitq_is_turnstile_queue(safeq)) {
				turnstile_kernel_update_inheritor_on_wake_locked(waitq_to_turnstile(safeq), (turnstile_inheritor_t)first_thread, TURNSTILE_INHERITOR_THREAD);
			}
		}

		                                             
		if (!remove_op) {
			break;
		}

		                                              
		*nthreads += 1;
		if (*nthreads == 1) {
			*(args->spl) = (safeq != waitq) ? spl : splsched();
		}
		thread_lock(thread);
		thread_clear_waitq_state(thread);
		enqueue_tail(args->threadq, &(thread->wait_links));
	}

	return first_thread;
}

   
                                                                              
  
              
                                                 
  
         
                                                                     
                                                                     
                                                                     
                                                   
  
                                                                    
                                                                   
                                                                     
   
static void
do_waitq_select_n_locked(struct waitq_select_args *args)
{
	struct waitq *waitq = args->waitq;
	int max_threads = args->max_threads;
	thread_t first_thread = THREAD_NULL;
	struct waitq *safeq;
	uint32_t remaining_eventmask = 0;
	uint32_t eventmask;
	int *nthreads = args->nthreads;
	spl_t spl = 0;

	assert(max_threads != 0);

	if (!waitq_irq_safe(waitq)) {
		                                                                  
		eventmask = _CAST_TO_EVENT_MASK(waitq);
		safeq = waitq_get_safeq(waitq);
		if (safeq == NULL) {
			  
                                                       
                                                              
                                
      
			goto handle_waitq_set;
		}

		if (*nthreads == 0) {
			spl = splsched();
		}
		waitq_lock(safeq);
	} else {
		eventmask = _CAST_TO_EVENT_MASK(args->event);
		safeq = waitq;
	}

	  
                                                                    
                                                                    
                                                                      
    
	if (!waitq_is_global(safeq) ||
	    (safeq->waitq_eventmask & eventmask) == eventmask) {
		if (waitq_is_turnstile_queue(safeq)) {
			first_thread = waitq_prioq_iterate_locked(safeq, waitq,
			    spl, args,
			    &remaining_eventmask);
		} else {
			first_thread = waitq_queue_iterate_locked(safeq, waitq,
			    spl, args,
			    &remaining_eventmask);
		}

		  
                                                           
                                                                    
                 
    
                                                                      
                                                                    
                                          
     
		if (waitq_is_global(safeq)) {
			if (waitq_empty(safeq)) {
				safeq->waitq_eventmask = 0;
			} else if (max_threads < 0 || *nthreads < max_threads) {
				safeq->waitq_eventmask = remaining_eventmask;
			}
		}
	}

	  
                                                                       
                                                                    
                                                                   
    
	if (*nthreads == 0 && first_thread != THREAD_NULL && args->threadq) {
		                                                 
		++(*nthreads);
		*(args->spl) = (safeq != waitq) ? spl : splsched();

		thread_lock(first_thread);
		thread_clear_waitq_state(first_thread);
		waitq_thread_remove(safeq, first_thread);
		enqueue_tail(args->threadq, &(first_thread->wait_links));

		                                                       
		if (waitq_is_global(safeq) && waitq_empty(safeq)) {
			safeq->waitq_eventmask = 0;
		}
	}

	                                                  
	if (safeq != waitq) {
		waitq_unlock(safeq);
		if (*nthreads == 0) {
			splx(spl);
		}
	}

	if (max_threads > 0 && *nthreads >= max_threads) {
		return;
	}

handle_waitq_set:
	  
                                        
                                   
    
	if (!waitq->waitq_set_id) {
		return;
	}

	                                                             
	struct waitq_link *link = wql_get_link(waitq->waitq_set_id);
	if (!link) {
		                                                                      
		waitq->waitq_set_id = 0;
		return;
	}

	wql_put_link(link);

	  
                                                                     
                                                                         
                              
   
                                                                    
                                                                    
    
	(void)walk_waitq_links(LINK_WALK_ONE_LEVEL, waitq, waitq->waitq_set_id,
	    WQL_WQS, (void *)args, waitq_select_walk_cb);
}

   
                                                     
  
              
                  
  
           
                                                                  
                                       
  
         
                                                                        
                                                                
                                          
  
                                                                      
                                                                  
                             
   
static __inline__ int
waitq_select_n_locked(struct waitq *waitq,
    event64_t event,
    waitq_select_cb select_cb,
    void *select_ctx,
    uint64_t *reserved_preposts,
    queue_t threadq,
    int max_threads, spl_t *spl,
    int priority)
{
	int nthreads = 0;

	struct waitq_select_args args = {
		.posted_waitq = waitq,
		.waitq = waitq,
		.event = event,
		.select_cb = select_cb,
		.select_ctx = select_ctx,
		.priority = priority,
		.reserved_preposts = reserved_preposts,
		.threadq = threadq,
		.max_threads = max_threads,
		.nthreads = &nthreads,
		.spl = spl,
	};

	do_waitq_select_n_locked(&args);
	return nthreads;
}

   
                                                                
  
              
                    
  
           
                                                                  
                                                                  
                         
   
static thread_t
waitq_select_one_locked(struct waitq *waitq, event64_t event,
    uint64_t *reserved_preposts,
    int priority, spl_t *spl)
{
	int nthreads;
	queue_head_t threadq;

	queue_init(&threadq);

	nthreads = waitq_select_n_locked(waitq, event, NULL, NULL,
	    reserved_preposts, &threadq, 1, spl, priority);

	                                                       
	if (!queue_empty(&threadq)) {
		thread_t t;
		queue_entry_t qe = dequeue_head(&threadq);
		t = qe_element(qe, struct thread, wait_links);
		assert(queue_empty(&threadq));                              
		                                                   
		return t;
	}

	return THREAD_NULL;
}

struct select_thread_ctx {
	thread_t      thread;
	event64_t     event;
	spl_t        *spl;
};

   
                                                                        
  
              
                          
                          
  
         
                                                                         
                                                                       
                                                                        
                                                                         
               
   
static int
waitq_select_thread_cb(struct waitq *waitq, void *ctx,
    struct waitq_link *link)
{
	struct select_thread_ctx *stctx = (struct select_thread_ctx *)ctx;
	struct waitq_set *wqset;
	struct waitq *wqsetq;
	struct waitq *safeq;
	spl_t s;

	(void)waitq;

	thread_t thread = stctx->thread;
	event64_t event = stctx->event;

	if (wql_type(link) != WQL_WQS) {
		return WQ_ITERATE_CONTINUE;
	}

	wqset = link->wql_wqs.wql_set;
	wqsetq = &wqset->wqset_q;

	assert(!waitq_irq_safe(waitq));
	assert(!waitq_irq_safe(wqsetq));

	waitq_set_lock(wqset);

	s = splsched();

	                                                                           
	safeq = waitq_get_safeq(wqsetq);
	waitq_lock(safeq);

	thread_lock(thread);

	if ((thread->waitq == wqsetq) && (thread->wait_event == event)) {
		waitq_thread_remove(wqsetq, thread);
		if (waitq_empty(safeq)) {
			safeq->waitq_eventmask = 0;
		}
		thread_clear_waitq_state(thread);
		waitq_unlock(safeq);
		waitq_set_unlock(wqset);
		  
                         
                                             
     
		*(stctx->spl) = s;
		return WQ_ITERATE_FOUND;
	}

	thread_unlock(thread);
	waitq_set_unlock(wqset);
	waitq_unlock(safeq);
	splx(s);

	return WQ_ITERATE_CONTINUE;
}

   
                                                                             
                                                             
  
              
                    
                       
   
static kern_return_t
waitq_select_thread_locked(struct waitq *waitq,
    event64_t event,
    thread_t thread, spl_t *spl)
{
	struct waitq *safeq;
	struct waitq_link *link;
	struct select_thread_ctx ctx;
	kern_return_t kr;
	spl_t s;

	                                                                           
	if (!waitq_irq_safe(waitq)) {
		safeq = waitq_get_safeq(waitq);
		if (safeq == NULL) {
			  
                                                       
                                                              
                                
      
			goto handle_waitq_set;
		}

		s = splsched();
		waitq_lock(safeq);
	} else {
		s = splsched();
		safeq = waitq;
	}

	thread_lock(thread);

	if ((thread->waitq == waitq) && (thread->wait_event == event)) {
		waitq_thread_remove(safeq, thread);
		if (waitq_empty(safeq)) {
			safeq->waitq_eventmask = 0;
		}
		thread_clear_waitq_state(thread);
		*spl = s;
		                         
		return KERN_SUCCESS;
	}

	thread_unlock(thread);

	if (safeq != waitq) {
		waitq_unlock(safeq);
	}

	splx(s);

handle_waitq_set:
	if (!waitq->waitq_set_id) {
		return KERN_NOT_WAITING;
	}

	                                                             
	link = wql_get_link(waitq->waitq_set_id);
	if (!link) {
		                                                                
		waitq->waitq_set_id = 0;
		return KERN_NOT_WAITING;
	}

	  
                                                          
                                                        
                                                       
                                                                   
    
	ctx.thread = thread;
	ctx.event = event;
	ctx.spl = spl;
	kr = walk_waitq_links(LINK_WALK_FULL_DAG, waitq, waitq->waitq_set_id,
	    WQL_WQS, (void *)&ctx, waitq_select_thread_cb);

	wql_put_link(link);

	                                       
	if (kr == WQ_ITERATE_FOUND) {
		return KERN_SUCCESS;
	}

	return KERN_NOT_WAITING;
}

static int
prepost_exists_cb(struct waitq_set __unused *wqset,
    void __unused *ctx,
    struct wq_prepost __unused *wqp,
    struct waitq __unused *waitq)
{
	                                                                        
	return WQ_ITERATE_FOUND;
}

   
                                                                
  
              
                    
   
wait_result_t
waitq_assert_wait64_locked(struct waitq *waitq,
    event64_t wait_event,
    wait_interrupt_t interruptible,
    wait_timeout_urgency_t urgency,
    uint64_t deadline,
    uint64_t leeway,
    thread_t thread)
{
	wait_result_t wait_result;
	int realtime = 0;
	struct waitq *safeq;
	uintptr_t eventmask;
	spl_t s;


	  
                                                            
                                 
    
	assert(!thread->started || thread == current_thread());

	if (thread->waitq != NULL) {
		panic("thread already waiting on %p", thread->waitq);
	}

	if (waitq_is_set(waitq)) {
		struct waitq_set *wqset = (struct waitq_set *)waitq;
		  
                                                           
                                      
     
		if (wait_event == NO_EVENT64 && waitq_set_maybe_preposted(wqset)) {
			int ret;
			  
                                                         
                                                        
                                         
      
			ret = wq_prepost_foreach_locked(wqset, NULL,
			    prepost_exists_cb);
			if (ret == WQ_ITERATE_FOUND) {
				s = splsched();
				thread_lock(thread);
				thread->wait_result = THREAD_AWAKENED;
				thread_unlock(thread);
				splx(s);
				return THREAD_AWAKENED;
			}
		}
	}

	s = splsched();

	  
                                                                   
                                                           
    
	if (!waitq_irq_safe(waitq)) {
		safeq = waitq_get_safeq(waitq);
		if (__improbable(safeq == NULL)) {
			panic("Trying to assert_wait on a turnstile proxy "
			    "that hasn't been donated one (waitq: %p)", waitq);
		}
		eventmask = _CAST_TO_EVENT_MASK(waitq);
		waitq_lock(safeq);
	} else {
		safeq = waitq;
		eventmask = _CAST_TO_EVENT_MASK(wait_event);
	}

	                                                                
	thread_lock(thread);

	  
                                                            
                                                         
                                                       
                                                                 
                                                       
                                     
    
	if (thread->sched_pri >= BASEPRI_REALTIME) {
		realtime = 1;
	}

	  
                                                                       
                                                                  
                                                                      
                                 
    
	wait_result = thread_mark_wait_locked(thread, interruptible);
	                                      
	if (wait_result == THREAD_WAITING) {
		if (!safeq->waitq_fifo
		    || (thread->options & TH_OPT_VMPRIV) || realtime) {
			waitq_thread_insert(safeq, thread, false);
		} else {
			waitq_thread_insert(safeq, thread, true);
		}

		                                                                       
		thread->wait_event = wait_event;
		thread->waitq = waitq;

		if (deadline != 0) {
			boolean_t act;

			act = timer_call_enter_with_leeway(&thread->wait_timer,
			    NULL,
			    deadline, leeway,
			    urgency, FALSE);
			if (!act) {
				thread->wait_timer_active++;
			}
			thread->wait_timer_is_set = TRUE;
		}

		if (waitq_is_global(safeq)) {
			safeq->waitq_eventmask |= eventmask;
		}

		waitq_stats_count_wait(waitq);
	}

	                       
	thread_unlock(thread);

	                                                                                  
	if (waitq_is_turnstile_queue(safeq) && wait_result == THREAD_WAITING) {
		turnstile_recompute_priority_locked(waitq_to_turnstile(safeq));
		turnstile_update_inheritor_locked(waitq_to_turnstile(safeq));
	}

	                                           
	if (safeq != waitq) {
		waitq_unlock(safeq);
	}

	splx(s);

	return wait_result;
}

   
                                                             
  
              
                     
  
         
                                                            
                                                 
                                                                       
   
int
waitq_pull_thread_locked(struct waitq *waitq, thread_t thread)
{
	struct waitq *safeq;

	assert_thread_magic(thread);
	assert(thread->waitq == waitq);

	                                                             
	if (!waitq_irq_safe(waitq)) {
		safeq = waitq_get_safeq(waitq);
		if (__improbable(safeq == NULL)) {
			panic("Trying to clear_wait on a turnstile proxy "
			    "that hasn't been donated one (waitq: %p)", waitq);
		}
	} else {
		safeq = waitq;
	}

	                                                                
	if (!waitq_lock_try(safeq)) {
		return 0;
	}

	waitq_thread_remove(safeq, thread);
	thread_clear_waitq_state(thread);
	waitq_stats_count_clear_wakeup(waitq);

	                                                                    
	if (waitq_is_global(safeq) && waitq_empty(safeq)) {
		safeq->waitq_eventmask = 0;
		                                                                        
	}

	waitq_unlock(safeq);

	return 1;
}


static __inline__
void
maybe_adjust_thread_pri(thread_t   thread,
    int        priority,
    __kdebug_only struct waitq *waitq)
{
	  
                                                                  
                                                                      
                                                                  
                                                               
                                                               
   
                                 
                                                  
                                        
                                 
    
	if (priority == WAITQ_PROMOTE_PRIORITY) {
		uintptr_t trace_waitq = 0;
		if (__improbable(kdebug_enable)) {
			trace_waitq = VM_KERNEL_UNSLIDE_OR_PERM(waitq);
		}

		sched_thread_promote_reason(thread, TH_SFLAG_WAITQ_PROMOTED, trace_waitq);
	}
}

  
                                                                  
                               
  
                                                                                     
   
void
waitq_clear_promotion_locked(struct waitq *waitq, thread_t thread)
{
	spl_t s;

	assert(waitq_held(waitq));
	assert(thread != THREAD_NULL);
	assert(thread == current_thread());

	                                                                                   
	if ((thread->sched_flags & TH_SFLAG_WAITQ_PROMOTED) != TH_SFLAG_WAITQ_PROMOTED) {
		return;
	}

	if (!waitq_irq_safe(waitq)) {
		s = splsched();
	}
	thread_lock(thread);

	sched_thread_unpromote_reason(thread, TH_SFLAG_WAITQ_PROMOTED, 0);

	thread_unlock(thread);
	if (!waitq_irq_safe(waitq)) {
		splx(s);
	}
}

   
                                                         
  
              
                    
  
         
                                                   
                                                       
  
                                                                     
                                                                        
                                                     
   
kern_return_t
waitq_wakeup64_all_locked(struct waitq *waitq,
    event64_t wake_event,
    wait_result_t result,
    uint64_t *reserved_preposts,
    int priority,
    waitq_lock_state_t lock_state)
{
	kern_return_t ret;
	thread_t thread;
	spl_t th_spl;
	int nthreads;
	queue_head_t wakeup_queue;

	assert(waitq_held(waitq));
	queue_init(&wakeup_queue);

	nthreads = waitq_select_n_locked(waitq, wake_event, NULL, NULL,
	    reserved_preposts,
	    &wakeup_queue, -1, &th_spl, priority);

	                             
	ret = KERN_NOT_WAITING;

#if CONFIG_WAITQ_STATS
	qe_foreach_element(thread, &wakeup_queue, wait_links)
	waitq_stats_count_wakeup(waitq);
#endif
	if (lock_state == WAITQ_UNLOCK) {
		waitq_unlock(waitq);
	}

	qe_foreach_element_safe(thread, &wakeup_queue, wait_links) {
		assert_thread_magic(thread);
		remqueue(&thread->wait_links);
		maybe_adjust_thread_pri(thread, priority, waitq);
		ret = thread_go(thread, result, WQ_OPTION_NONE);
		assert(ret == KERN_SUCCESS);
		thread_unlock(thread);
	}
	if (nthreads > 0) {
		splx(th_spl);
	} else {
		waitq_stats_count_fail(waitq);
	}

	return ret;
}

   
                                                        
  
              
                    
  
         
                                                    
   
kern_return_t
waitq_wakeup64_one_locked(struct waitq *waitq,
    event64_t wake_event,
    wait_result_t result,
    uint64_t *reserved_preposts,
    int priority,
    waitq_lock_state_t lock_state,
    waitq_options_t option)
{
	thread_t thread;
	spl_t th_spl;

	assert(waitq_held(waitq));

	thread = waitq_select_one_locked(waitq, wake_event,
	    reserved_preposts,
	    priority, &th_spl);

	if (thread != THREAD_NULL) {
		waitq_stats_count_wakeup(waitq);
	} else {
		waitq_stats_count_fail(waitq);
	}

	if (lock_state == WAITQ_UNLOCK) {
		waitq_unlock(waitq);
	}

	if (thread != THREAD_NULL) {
		maybe_adjust_thread_pri(thread, priority, waitq);
		kern_return_t ret = thread_go(thread, result, option);
		assert(ret == KERN_SUCCESS);
		thread_unlock(thread);
		splx(th_spl);
		return ret;
	}

	return KERN_NOT_WAITING;
}

   
                                                        
  
              
                    
  
           
                             
                                                    
                                                       
                                         
   
thread_t
waitq_wakeup64_identify_locked(struct waitq     *waitq,
    event64_t        wake_event,
    wait_result_t    result,
    spl_t            *spl,
    uint64_t         *reserved_preposts,
    int              priority,
    waitq_lock_state_t lock_state)
{
	thread_t thread;

	assert(waitq_held(waitq));

	thread = waitq_select_one_locked(waitq, wake_event,
	    reserved_preposts,
	    priority, spl);

	if (thread != THREAD_NULL) {
		waitq_stats_count_wakeup(waitq);
	} else {
		waitq_stats_count_fail(waitq);
	}

	if (lock_state == WAITQ_UNLOCK) {
		waitq_unlock(waitq);
	}

	if (thread != THREAD_NULL) {
		kern_return_t __assert_only ret;
		ret = thread_go(thread, result, WQ_OPTION_NONE);
		assert(ret == KERN_SUCCESS);
	}

	return thread;                                                      
}

   
                                                                        
  
              
                    
                       
  
         
                                                   
  
                                                                        
                                                                       
                                                     
   
kern_return_t
waitq_wakeup64_thread_locked(struct waitq *waitq,
    event64_t wake_event,
    thread_t thread,
    wait_result_t result,
    waitq_lock_state_t lock_state)
{
	kern_return_t ret;
	spl_t th_spl;

	assert(waitq_held(waitq));
	assert_thread_magic(thread);

	  
                                                             
                                 
    
	ret = waitq_select_thread_locked(waitq, wake_event, thread, &th_spl);

	if (ret == KERN_SUCCESS) {
		waitq_stats_count_wakeup(waitq);
	} else {
		waitq_stats_count_fail(waitq);
	}

	if (lock_state == WAITQ_UNLOCK) {
		waitq_unlock(waitq);
	}

	if (ret != KERN_SUCCESS) {
		return KERN_NOT_WAITING;
	}

	ret = thread_go(thread, result, WQ_OPTION_NONE);
	assert(ret == KERN_SUCCESS);
	thread_unlock(thread);
	splx(th_spl);

	return ret;
}



                                                                         
  
                
  
                                                                            

   
                            
   
kern_return_t
waitq_init(struct waitq *waitq, int policy)
{
	assert(waitq != NULL);

	                                
	if ((policy & SYNC_POLICY_FIXED_PRIORITY) != 0) {
		return KERN_INVALID_ARGUMENT;
	}

	waitq->waitq_fifo = ((policy & SYNC_POLICY_REVERSED) == 0);
	waitq->waitq_irq = !!(policy & SYNC_POLICY_DISABLE_IRQ);
	waitq->waitq_prepost = 0;
	if (policy & SYNC_POLICY_TURNSTILE_PROXY) {
		waitq->waitq_type = WQT_TSPROXY;
	} else {
		waitq->waitq_type = WQT_QUEUE;
	}
	waitq->waitq_turnstile = !!(policy & SYNC_POLICY_TURNSTILE);
	waitq->waitq_eventmask = 0;

	waitq->waitq_set_id = 0;
	waitq->waitq_prepost_id = 0;

	waitq_lock_init(waitq);
	if (waitq_is_turnstile_queue(waitq)) {
		                                                      
		priority_queue_init(&waitq->waitq_prio_queue);
		assert(waitq->waitq_fifo == 0);
	} else if (policy & SYNC_POLICY_TURNSTILE_PROXY) {
		waitq->waitq_ts = TURNSTILE_NULL;
		waitq->waitq_tspriv = NULL;
	} else {
		queue_init(&waitq->waitq_queue);
	}

	waitq->waitq_isvalid = 1;
	return KERN_SUCCESS;
}

struct wq_unlink_ctx {
	struct waitq *unlink_wq;
	struct waitq_set *unlink_wqset;
};

static int waitq_unlink_prepost_cb(struct waitq_set __unused *wqset, void *ctx,
    struct wq_prepost *wqp, struct waitq *waitq);

   
                                                           
  
              
                                
                                                                       
                                                                       
              
   
static int
waitq_unlink_all_cb(struct waitq *waitq, void *ctx,
    struct waitq_link *link)
{
	(void)waitq;
	(void)ctx;
	if (wql_type(link) == WQL_LINK && wql_is_valid(link)) {
		wql_invalidate(link);
	}

	if (wql_type(link) == WQL_WQS) {
		struct waitq_set *wqset;
		struct wq_unlink_ctx ulctx;

		  
                                                              
                                                                
                                                               
                                     
     
		if (waitq->waitq_prepost_id == 0) {
			goto out;
		}

		wqset = link->wql_wqs.wql_set;
		assert(wqset != NULL);
		assert(!waitq_irq_safe(&wqset->wqset_q));

		waitq_set_lock(wqset);

		if (!waitq_set_is_valid(wqset)) {
			                                  
			goto out_unlock;
		}
		if (!waitq_set_maybe_preposted(wqset)) {
			goto out_unlock;
		}

		ulctx.unlink_wq = waitq;
		ulctx.unlink_wqset = wqset;
		(void)wq_prepost_iterate(wqset->wqset_prepost_id, &ulctx,
		    waitq_unlink_prepost_cb);
out_unlock:
		waitq_set_unlock(wqset);
	}

out:
	return WQ_ITERATE_CONTINUE;
}


   
                                                                   
   
void
waitq_deinit(struct waitq *waitq)
{
	spl_t s;

	assert(waitq);
	if (!waitq_is_valid(waitq)) {
		return;
	}

	if (!waitq_is_queue(waitq) && !waitq_is_turnstile_proxy(waitq)) {
		return;
	}

	if (waitq_irq_safe(waitq)) {
		s = splsched();
	}
	waitq_lock(waitq);

	if (waitq_valid(waitq)) {
		waitq->waitq_isvalid = 0;
		if (!waitq_irq_safe(waitq)) {
			waitq_unlink_all_unlock(waitq);
			                                              
			goto out;
		}
	}

	waitq_unlock(waitq);
	if (waitq_irq_safe(waitq)) {
		splx(s);
	}

out:
#if MACH_ASSERT
	if (waitq_is_turnstile_queue(waitq)) {
		assert(priority_queue_empty(&waitq->waitq_prio_queue));
	} else if (waitq_is_turnstile_proxy(waitq)) {
		assert(waitq->waitq_ts == TURNSTILE_NULL);
	} else {
		assert(queue_empty(&waitq->waitq_queue));
	}
#else
	(void)0;
#endif               
}

void
waitq_invalidate_locked(struct waitq *waitq)
{
	assert(waitq_held(waitq));
	assert(waitq_is_valid(waitq));
	waitq->waitq_isvalid = 0;
}

   
                                         
  
              
                                                                         
   
static int
wqset_clear_prepost_chain_cb(struct waitq_set __unused *wqset,
    void __unused *ctx,
    struct wq_prepost *wqp,
    struct waitq __unused *waitq)
{
	if (wqp_type(wqp) == WQP_POST) {
		wq_prepost_invalidate(wqp);
	}
	return WQ_ITERATE_CONTINUE;
}


   
                                             
  
              
            
  
           
                                            
                                              
                           
  
                  
   
struct waitq_set *
waitq_set_alloc(int policy, waitq_set_prepost_hook_t *prepost_hook)
{
	struct waitq_set *wqset;

	wqset = (struct waitq_set *)zalloc(waitq_set_zone);
	if (!wqset) {
		panic("Can't allocate a new waitq set from zone %p", waitq_set_zone);
	}

	kern_return_t ret;
	ret = waitq_set_init(wqset, policy, NULL, prepost_hook);
	if (ret != KERN_SUCCESS) {
		zfree(waitq_set_zone, wqset);
		wqset = NULL;
	}

	return wqset;
}

   
                                
  
                                         
                                          
                                              
   
kern_return_t
waitq_set_init(struct waitq_set *wqset,
    int policy, uint64_t *reserved_link,
    waitq_set_prepost_hook_t *prepost_hook)
{
	struct waitq_link *link;
	kern_return_t ret;

	memset(wqset, 0, sizeof(*wqset));

	ret = waitq_init(&wqset->wqset_q, policy);
	if (ret != KERN_SUCCESS) {
		return ret;
	}

	wqset->wqset_q.waitq_type = WQT_SET;
	if (policy & SYNC_POLICY_PREPOST) {
		wqset->wqset_q.waitq_prepost = 1;
		wqset->wqset_prepost_id = 0;
		assert(prepost_hook == NULL);
	} else {
		wqset->wqset_q.waitq_prepost = 0;
		wqset->wqset_prepost_hook = prepost_hook;
	}

	if (reserved_link && *reserved_link != 0) {
		link = wql_get_reserved(*reserved_link, WQL_WQS);

		if (!link) {
			panic("Can't allocate link object for waitq set: %p", wqset);
		}

		                                           
		*reserved_link = 0;

		link->wql_wqs.wql_set = wqset;
		wql_mkvalid(link);

		wqset->wqset_id = link->wql_setid.id;
		wql_put_link(link);
	} else {
		  
                                                             
     
		wqset->wqset_id = WQSET_NOT_LINKED;
	}

	return KERN_SUCCESS;
}

#if DEVELOPMENT || DEBUG

int
sysctl_helper_waitq_set_nelem(void)
{
	return ltable_nelem(&g_wqlinktable);
}

#endif

   
                               
  
              
            
                                      
  
   
void
waitq_set_lazy_init_link(struct waitq_set *wqset)
{
	struct waitq_link *link;

	assert(get_preemption_level() == 0 && waitq_wait_possible(current_thread()));

	waitq_set_lock(wqset);
	if (!waitq_set_should_lazy_init_link(wqset)) {
		waitq_set_unlock(wqset);
		return;
	}

	assert(wqset->wqset_id == WQSET_NOT_LINKED);
	waitq_set_unlock(wqset);

	link = wql_alloc_link(WQL_WQS);
	if (!link) {
		panic("Can't allocate link object for waitq set: %p", wqset);
	}

	link->wql_wqs.wql_set = wqset;

	waitq_set_lock(wqset);
	if (waitq_set_should_lazy_init_link(wqset)) {
		wql_mkvalid(link);
		wqset->wqset_id = link->wql_setid.id;
	}

	assert(wqset->wqset_id != 0);
	assert(wqset->wqset_id != WQSET_NOT_LINKED);

	waitq_set_unlock(wqset);

	wql_put_link(link);

	return;
}

   
                                            
  
   
boolean_t
waitq_set_should_lazy_init_link(struct waitq_set *wqset)
{
	if (waitqs_is_linked(wqset) || wqset->wqset_id == 0) {
		return FALSE;
	}
	return TRUE;
}

   
                                                                
  
              
            
        
                                                      
                                                                    
   
void
waitq_set_deinit(struct waitq_set *wqset)
{
	struct waitq_link *link = NULL;
	uint64_t set_id, prepost_id;

	if (!waitqs_is_set(wqset)) {
		panic("trying to de-initialize an invalid wqset @%p", wqset);
	}

	assert(!waitq_irq_safe(&wqset->wqset_q));

	waitq_set_lock(wqset);

	if (waitq_set_has_prepost_hook(wqset)) {
		waitq_set_prepost_hook_t *hook = wqset->wqset_prepost_hook;
		  
                                              
                                                             
                                                  
     
		while (os_atomic_load(hook, relaxed) != 0) {
			waitq_set_unlock(wqset);
			do {
				delay(1);
			} while (os_atomic_load(hook, relaxed) != 0);
			waitq_set_lock(wqset);
		}
	}

	set_id = wqset->wqset_id;

	if (waitqs_is_linked(wqset) || set_id == 0) {
		                                
		link = wql_get_link(set_id);
		if (link) {
			wql_invalidate(link);
		}
		                                
		if (!link || wqset->wqset_id != set_id || set_id != link->wql_setid.id) {
			if (link) {
				wql_put_link(link);
			}
			waitq_set_unlock(wqset);
			return;
		}

		                                                          
		assert(link != NULL && wql_type(link) == WQL_WQS);

		wqset->wqset_id = 0;
	}

	  
                                                                     
                                                                      
                                                                     
                                                                 
                                                        
    
	prepost_id = 0;
	if (wqset->wqset_q.waitq_prepost && wqset->wqset_prepost_id) {
		assert(link != NULL);
		prepost_id = wqset->wqset_prepost_id;
	}
	                                             
	wqset->wqset_prepost_id = 0;

	wqset->wqset_q.waitq_fifo = 0;
	wqset->wqset_q.waitq_prepost = 0;
	wqset->wqset_q.waitq_isvalid = 0;

	                                                            
	wqset->wqset_q.waitq_eventmask = 0;

	waitq_unlink_all_unlock(&wqset->wqset_q);
	                                                       


	if (link) {
		  
                                                                   
                                                                        
                                                                       
                                                                     
                                                    
     
		while (wql_refcnt(link) > 1) {
			delay(1);
		}
		wql_put_link(link);
	}

	                                                 
	                                             
	if (prepost_id) {
		(void)wq_prepost_iterate(prepost_id, NULL,
		    wqset_clear_prepost_chain_cb);
	}
}

   
                                                       
  
              
            
   
kern_return_t
waitq_set_free(struct waitq_set *wqset)
{
	waitq_set_deinit(wqset);

	memset(wqset, 0, sizeof(*wqset));
	zfree(waitq_set_zone, wqset);

	return KERN_SUCCESS;
}

#if DEVELOPMENT || DEBUG
#if CONFIG_WAITQ_DEBUG
   
                               
   
uint64_t
wqset_id(struct waitq_set *wqset)
{
	if (!wqset) {
		return 0;
	}

	assert(waitqs_is_set(wqset));

	if (!waitqs_is_linked(wqset)) {
		waitq_set_lazy_init_link(wqset);
	}

	return wqset->wqset_id;
}

   
                                                            
   
struct waitq *
wqset_waitq(struct waitq_set *wqset)
{
	if (!wqset) {
		return NULL;
	}

	assert(waitqs_is_set(wqset));

	return &wqset->wqset_q;
}
#endif                         
#endif                           


   
                                              
  
              
                 
                                                          
                                                                
  
        
                                                               
                                                                 
                                                  
   
int
waitq_clear_prepost_locked(struct waitq *waitq)
{
	struct wq_prepost *wqp;
	int dropped_lock = 0;

	assert(!waitq_irq_safe(waitq));

	if (waitq->waitq_prepost_id == 0) {
		return 0;
	}

	wqp = wq_prepost_get(waitq->waitq_prepost_id);
	waitq->waitq_prepost_id = 0;
	if (wqp) {
		uint64_t wqp_id = wqp->wqp_prepostid.id;
		wqdbg_v("invalidate prepost 0x%llx (refcnt:%d)",
		    wqp->wqp_prepostid.id, wqp_refcnt(wqp));
		wq_prepost_invalidate(wqp);
		while (wqp_refcnt(wqp) > 1) {
			  
                                                         
                                                     
                                                       
                                                       
                                                     
                                                   
                                                      
                                                         
                                                         
                                                    
                                                        
                                                         
      
			disable_preemption();

			waitq_unlock(waitq);
			dropped_lock = 1;
			  
                                                      
                                    
      
			delay(1);

			waitq_lock(waitq);

			enable_preemption();
		}
		if (wqp_refcnt(wqp) > 0 && wqp->wqp_prepostid.id == wqp_id) {
			wq_prepost_put(wqp);
		}
	}

	return dropped_lock;
}

   
                                              
  
              
                        
                                       
   
void
waitq_clear_prepost(struct waitq *waitq)
{
	assert(waitq_valid(waitq));
	assert(!waitq_irq_safe(waitq));

	waitq_lock(waitq);
	                                                         
	(void)waitq_clear_prepost_locked(waitq);
	waitq_unlock(waitq);
}

   
                                                                 
  
              
                      
   
uint64_t
waitq_get_prepost_id(struct waitq *waitq)
{
	struct wq_prepost *wqp;
	uint64_t wqp_id = 0;

	if (!waitq_valid(waitq)) {
		return 0;
	}

	assert(!waitq_irq_safe(waitq));

	waitq_lock(waitq);

	if (!waitq_valid(waitq)) {
		goto out_unlock;
	}

	if (waitq->waitq_prepost_id) {
		wqp_id = waitq->waitq_prepost_id;
		goto out_unlock;
	}

	                                                             
	waitq_unlock(waitq);

	wqp = wq_prepost_alloc(WQP_WQ, 1);
	if (!wqp) {
		return 0;
	}

	                               
	waitq_lock(waitq);

	if (!waitq_valid(waitq)) {
		wq_prepost_put(wqp);
		wqp_id = 0;
		goto out_unlock;
	}

	if (waitq->waitq_prepost_id) {
		                                  
		wq_prepost_put(wqp);
		wqp_id = waitq->waitq_prepost_id;
		goto out_unlock;
	}

	wqp->wqp_wq.wqp_wq_ptr = waitq;

	wqp_set_valid(wqp);
	wqp_id = wqp->wqp_prepostid.id;
	waitq->waitq_prepost_id = wqp_id;

	wq_prepost_put(wqp);

out_unlock:
	waitq_unlock(waitq);

	return wqp_id;
}


static int
waitq_inset_cb(struct waitq *waitq, void *ctx, struct waitq_link *link)
{
	uint64_t setid = *(uint64_t *)ctx;
	int wqltype = wql_type(link);
	(void)waitq;
	if (wqltype == WQL_WQS && link->wql_setid.id == setid) {
		wqdbg_v("  waitq already in set 0x%llx", setid);
		return WQ_ITERATE_FOUND;
	} else if (wqltype == WQL_LINK) {
		  
                                                              
                                             
                        
     
		wqdbg_v("  waitq already in set 0x%llx (WQL_LINK)", setid);
		if (link->wql_link.left_setid == setid ||
		    link->wql_link.right_setid == setid) {
			return WQ_ITERATE_FOUND;
		}
	}

	return WQ_ITERATE_CONTINUE;
}

   
                                              
  
              
                                            
                                                             
   
boolean_t
waitq_member(struct waitq *waitq, struct waitq_set *wqset)
{
	kern_return_t kr = WQ_ITERATE_SUCCESS;
	uint64_t setid;

	if (!waitq_valid(waitq)) {
		panic("Invalid waitq: %p", waitq);
	}
	assert(!waitq_irq_safe(waitq));

	if (!waitqs_is_set(wqset)) {
		return FALSE;
	}

	waitq_lock(waitq);

	if (!waitqs_is_linked(wqset)) {
		goto out_unlock;
	}

	setid = wqset->wqset_id;

	                                                      
	if (waitq->waitq_set_id == setid) {
		waitq_unlock(waitq);
		return TRUE;
	}

	                                                          
	kr = walk_waitq_links(LINK_WALK_ONE_LEVEL, waitq, waitq->waitq_set_id,
	    WQL_ALL, (void *)&setid, waitq_inset_cb);

out_unlock:
	waitq_unlock(waitq);
	return kr == WQ_ITERATE_FOUND;
}

   
                                                                
   
boolean_t
waitq_in_set(struct waitq *waitq)
{
	struct waitq_link *link;
	boolean_t inset = FALSE;

	if (waitq_irq_safe(waitq)) {
		return FALSE;
	}

	waitq_lock(waitq);

	if (!waitq->waitq_set_id) {
		goto out_unlock;
	}

	link = wql_get_link(waitq->waitq_set_id);
	if (link) {
		                                                        
		inset = TRUE;
		wql_put_link(link);
	} else {
		                                             
		waitq->waitq_set_id = 0;
	}

out_unlock:
	waitq_unlock(waitq);
	return inset;
}


   
                                                          
  
              
                        
                                                 
   
uint64_t
waitq_link_reserve(struct waitq *waitq)
{
	struct waitq_link *link;
	uint64_t reserved_id = 0;

	assert(get_preemption_level() == 0 && waitq_wait_possible(current_thread()));

	  
                                                             
                                           
    
	wql_ensure_free_space();

	(void)waitq;
	link = wql_alloc_link(LT_RESERVED);
	if (!link) {
		return 0;
	}

	reserved_id = link->wql_setid.id;

	return reserved_id;
}

   
                                               
   
void
waitq_link_release(uint64_t id)
{
	struct waitq_link *link;

	if (id == 0) {
		return;
	}

	link = wql_get_reserved(id, WQL_LINK);
	if (!link) {
		return;
	}

	  
                                                      
                                                        
                                                            
    
	wql_put_link(link);
#if CONFIG_LTABLE_STATS
	g_wqlinktable.nreserved_releases += 1;
#endif
}

   
                                                                           
  
              
                    
                                                      
   
static kern_return_t
waitq_link_internal(struct waitq *waitq,
    uint64_t setid, struct waitq_link *link)
{
	struct waitq_link *qlink;
	kern_return_t kr;

	assert(waitq_held(waitq));
	assert(setid != 0);
	assert(setid != WQSET_NOT_LINKED);

	  
                                                              
                                                              
          
    
	if (!waitq->waitq_set_id) {
		waitq->waitq_set_id = setid;
		return KERN_SUCCESS;
	}

	qlink = wql_get_link(waitq->waitq_set_id);
	if (!qlink) {
		  
                                                       
                                                            
     
		waitq->waitq_set_id = setid;
		return KERN_SUCCESS;
	}
	wql_put_link(qlink);

	  
                                                     
   
                           
    
	kr = walk_waitq_links(LINK_WALK_ONE_LEVEL, waitq, waitq->waitq_set_id,
	    WQL_ALL, (void *)&setid, waitq_inset_cb);
	if (kr == WQ_ITERATE_FOUND) {
		return KERN_ALREADY_IN_SET;
	}

	  
                                                            
                                                           
                                                            
                                                                      
                                                                    
                                                                 
                                
    
	link->wql_link.left_setid = setid;
	link->wql_link.right_setid = waitq->waitq_set_id;
	wql_mkvalid(link);

	waitq->waitq_set_id = link->wql_setid.id;

	return KERN_SUCCESS;
}

   
                          
  
              
                                                                        
                                     
  
                                                                        
                                           
  
                                                                    
  
         
                                                                  
                                                                             
                                                                                 
                                                                                
             
   
kern_return_t
waitq_link(struct waitq *waitq, struct waitq_set *wqset,
    waitq_lock_state_t lock_state, uint64_t *reserved_link)
{
	kern_return_t kr;
	struct waitq_link *link;
	int should_lock = (lock_state == WAITQ_SHOULD_LOCK);

	if (!waitq_valid(waitq) || waitq_irq_safe(waitq)) {
		panic("Invalid waitq: %p", waitq);
	}

	if (!waitqs_is_set(wqset)) {
		return KERN_INVALID_ARGUMENT;
	}

	if (!reserved_link || *reserved_link == 0) {
		if (!waitqs_is_linked(wqset)) {
			waitq_set_lazy_init_link(wqset);
		}
	}

	wqdbg_v("Link waitq %p to wqset 0x%llx",
	    (void *)VM_KERNEL_UNSLIDE_OR_PERM(waitq), wqset->wqset_id);

	  
                                                                 
                                                  
   
                                                               
                                                                
                                                                 
               
    
	if (reserved_link && *reserved_link != 0) {
		link = wql_get_reserved(*reserved_link, WQL_LINK);
		                                           
		*reserved_link = 0;
	} else {
		link = wql_alloc_link(WQL_LINK);
	}
	if (!link) {
		return KERN_NO_SPACE;
	}

	if (should_lock) {
		waitq_lock(waitq);
	}

	kr = waitq_link_internal(waitq, wqset->wqset_id, link);

	if (should_lock) {
		waitq_unlock(waitq);
	}

	wql_put_link(link);

	return kr;
}

   
                                                              
                                                                  
  
              
                                                       
                    
  
         
                                                                       
                                                                
             
   
static inline int
waitq_maybe_remove_link(struct waitq *waitq,
    uint64_t setid,
    struct waitq_link *parent,
    struct waitq_link *left,
    struct waitq_link *right)
{
	uint64_t *wq_setid = &waitq->waitq_set_id;

	  
                            
   
               
                                                                        
                                 
   
                        
                    
                     
                      
                                  
   
                                                                     
                                                                    
                                                              
                                                                         
                                                                  
                                                                       
    
	if (*wq_setid == parent->wql_setid.id) {
		if (!left && !right) {
			                                 
			wql_invalidate(parent);
			wqdbg_v("S1, L+R");
			*wq_setid = 0;
			return WQ_ITERATE_INVALID;
		} else if (!left || left->wql_setid.id == setid) {
			  
                                                       
                                                   
                              
      
			*wq_setid = right ? right->wql_setid.id : 0;
			wql_invalidate(parent);
			wqdbg_v("S1, L");
			return left ? WQ_ITERATE_UNLINKED : WQ_ITERATE_INVALID;
		} else if (!right || right->wql_setid.id == setid) {
			  
                                                           
                                                   
                              
      
			*wq_setid = left ? left->wql_setid.id : 0;
			wql_invalidate(parent);
			wqdbg_v("S1, R");
			return right ? WQ_ITERATE_UNLINKED : WQ_ITERATE_INVALID;
		}
	}

	  
                                                           
                              
    
	assert(left || right);                                                

	  
               
                                                                        
                                          
   
                          
                         
                            
                            
                             
                                    
                                   
                                    
                                         
                            
   
                                                                  
                                                                       
                                                                    
                                                                       
                                                                     
                            
    
	if (left && wql_type(left) == WQL_LINK) {
		uint64_t Ll, Lr;
		struct waitq_link *linkLl, *linkLr;
		assert(left->wql_setid.id != setid);
		Ll = left->wql_link.left_setid;
		Lr = left->wql_link.right_setid;
		linkLl = wql_get_link(Ll);
		linkLr = wql_get_link(Lr);
		if (!linkLl && !linkLr) {
			  
                                                    
                                                         
      
			wql_invalidate(left);
			wqdbg_v("S2, Ll+Lr");
			return WQ_ITERATE_INVALID;
		} else if (!linkLl || Ll == setid) {
			                                                               
			parent->wql_link.left_setid = Lr;
			wql_invalidate(left);
			wql_put_link(linkLl);
			wql_put_link(linkLr);
			wqdbg_v("S2, Ll");
			return linkLl ? WQ_ITERATE_UNLINKED : WQ_ITERATE_INVALID;
		} else if (!linkLr || Lr == setid) {
			                                                               
			parent->wql_link.left_setid = Ll;
			wql_invalidate(left);
			wql_put_link(linkLr);
			wql_put_link(linkLl);
			wqdbg_v("S2, Lr");
			return linkLr ? WQ_ITERATE_UNLINKED : WQ_ITERATE_INVALID;
		}
		wql_put_link(linkLl);
		wql_put_link(linkLr);
	}

	if (right && wql_type(right) == WQL_LINK) {
		uint64_t Rl, Rr;
		struct waitq_link *linkRl, *linkRr;
		assert(right->wql_setid.id != setid);
		Rl = right->wql_link.left_setid;
		Rr = right->wql_link.right_setid;
		linkRl = wql_get_link(Rl);
		linkRr = wql_get_link(Rr);
		if (!linkRl && !linkRr) {
			  
                                                     
                                                          
      
			wql_invalidate(right);
			wqdbg_v("S2, Rl+Rr");
			return WQ_ITERATE_INVALID;
		} else if (!linkRl || Rl == setid) {
			                                                               
			parent->wql_link.right_setid = Rr;
			wql_invalidate(right);
			wql_put_link(linkRl);
			wql_put_link(linkRr);
			wqdbg_v("S2, Rl");
			return linkRl ? WQ_ITERATE_UNLINKED : WQ_ITERATE_INVALID;
		} else if (!linkRr || Rr == setid) {
			                                                               
			parent->wql_link.right_setid = Rl;
			wql_invalidate(right);
			wql_put_link(linkRl);
			wql_put_link(linkRr);
			wqdbg_v("S2, Rr");
			return linkRr ? WQ_ITERATE_UNLINKED : WQ_ITERATE_INVALID;
		}
		wql_put_link(linkRl);
		wql_put_link(linkRr);
	}

	return WQ_ITERATE_CONTINUE;
}

   
                                                                  
  
              
                               
                    
  
         
                                                               
                               
   
static int
waitq_unlink_cb(struct waitq *waitq, void *ctx,
    struct waitq_link *link)
{
	uint64_t setid = *((uint64_t *)ctx);
	struct waitq_link *right, *left;
	int ret = 0;

	if (wql_type(link) != WQL_LINK) {
		return WQ_ITERATE_CONTINUE;
	}

	do {
		left  = wql_get_link(link->wql_link.left_setid);
		right = wql_get_link(link->wql_link.right_setid);

		ret = waitq_maybe_remove_link(waitq, setid, link, left, right);

		wql_put_link(left);
		wql_put_link(right);

		if (!wql_is_valid(link)) {
			return WQ_ITERATE_INVALID;
		}
		                                                             
	} while (ret == WQ_ITERATE_INVALID);

	return ret;
}


   
                                                      
  
              
                                                              
                      
                                
   
static int
waitq_unlink_prepost_cb(struct waitq_set __unused *wqset, void *ctx,
    struct wq_prepost *wqp, struct waitq *waitq)
{
	struct wq_unlink_ctx *ulctx = (struct wq_unlink_ctx *)ctx;

	if (waitq != ulctx->unlink_wq) {
		return WQ_ITERATE_CONTINUE;
	}

	if (wqp_type(wqp) == WQP_WQ &&
	    wqp->wqp_prepostid.id == ulctx->unlink_wqset->wqset_prepost_id) {
		                                                     
		wqdbg_v("unlink wqp (WQ) 0x%llx", wqp->wqp_prepostid.id);
		ulctx->unlink_wqset->wqset_prepost_id = 0;
		return WQ_ITERATE_BREAK;
	}

	assert(wqp_type(wqp) == WQP_POST);

	  
                                                                     
                                                                    
                                                                  
                                                                       
                                                                    
    
	wq_prepost_remove(ulctx->unlink_wqset, wqp);
	return WQ_ITERATE_BREAK;
}

   
                              
  
              
                    
                          
                                                                      
                                   
   
static kern_return_t
waitq_unlink_locked(struct waitq *waitq,
    struct waitq_set *wqset)
{
	uint64_t setid;
	kern_return_t kr;

	assert(!waitq_irq_safe(waitq));

	if (waitq->waitq_set_id == 0) {
		  
          
                                                              
                                                              
                                     
     
		if (waitq->waitq_prepost_id != 0) {
			(void)waitq_clear_prepost_locked(waitq);
		}
		return KERN_NOT_IN_SET;
	}

	if (!waitqs_is_linked(wqset)) {
		  
                                              
                                              
     
		return KERN_NOT_IN_SET;
	}

	setid = wqset->wqset_id;

	if (waitq->waitq_set_id == setid) {
		waitq->waitq_set_id = 0;
		  
                                                              
                                                          
                                                           
                                                         
     
		(void)waitq_clear_prepost_locked(waitq);
		return KERN_SUCCESS;
	}

	  
                                                            
                                                      
                                            
   
                                                                      
                                                                     
                                   
   
                                                                         
                                                                   
                                                                      
                                                                    
                     
    
	kr = walk_waitq_links(LINK_WALK_ONE_LEVEL, waitq, waitq->waitq_set_id,
	    WQL_LINK, (void *)&setid, waitq_unlink_cb);

	if (kr == WQ_ITERATE_UNLINKED) {
		struct wq_unlink_ctx ulctx;

		kr = KERN_SUCCESS;                                     

		                                                         
		if (!wqset->wqset_q.waitq_prepost) {
			goto out;
		}

		assert(!waitq_irq_safe(&wqset->wqset_q));

		waitq_set_lock(wqset);
		  
                                                
                                                               
                                        
     
		ulctx.unlink_wq = waitq;
		ulctx.unlink_wqset = wqset;
		(void)wq_prepost_iterate(wqset->wqset_prepost_id, (void *)&ulctx,
		    waitq_unlink_prepost_cb);
		waitq_set_unlock(wqset);
	} else {
		kr = KERN_NOT_IN_SET;                                           
	}

out:
	return kr;
}

   
                              
  
              
                                        
                                       
                                     
                                   
   
kern_return_t
waitq_unlink(struct waitq *waitq, struct waitq_set *wqset)
{
	kern_return_t kr = KERN_SUCCESS;

	assert(waitqs_is_set(wqset));

	  
                                                                     
                                
    
	if (!waitq_valid(waitq)) {
		return KERN_INVALID_ARGUMENT;
	}

	wqdbg_v("unlink waitq %p from set 0x%llx",
	    (void *)VM_KERNEL_UNSLIDE_OR_PERM(waitq), wqset->wqset_id);

	assert(!waitq_irq_safe(waitq));

	waitq_lock(waitq);

	kr = waitq_unlink_locked(waitq, wqset);

	waitq_unlock(waitq);
	return kr;
}

   
                                                                             
  
              
                      
                                 
   
void
waitq_unlink_by_prepost_id(uint64_t wqp_id, struct waitq_set *wqset)
{
	struct wq_prepost *wqp;

	disable_preemption();
	wqp = wq_prepost_get(wqp_id);
	if (wqp) {
		struct waitq *wq;

		wq = wqp->wqp_wq.wqp_wq_ptr;

		  
                                                                
                                                                
                                                               
                                                             
                             
     
		assert(!waitq_irq_safe(wq));

		waitq_lock(wq);
		wq_prepost_put(wqp);

		if (!waitq_valid(wq)) {
			                                           
			waitq_unlock(wq);
			enable_preemption();
			return;
		}

		                                                
		waitq_unlink_locked(wq, wqset);

		waitq_unlock(wq);
	}
	enable_preemption();
	return;
}


   
                                               
  
              
                                 
  
           
                                     
                  
   
struct waitq *
waitq_lock_by_prepost_id(uint64_t wqp_id)
{
	struct waitq *wq = NULL;
	struct wq_prepost *wqp;

	disable_preemption();
	wqp = wq_prepost_get(wqp_id);
	if (wqp) {
		wq = wqp->wqp_wq.wqp_wq_ptr;

		assert(!waitq_irq_safe(wq));

		waitq_lock(wq);
		wq_prepost_put(wqp);

		if (!waitq_valid(wq)) {
			                                           
			waitq_unlock(wq);
			enable_preemption();
			return NULL;
		}
	}
	enable_preemption();
	return wq;
}


   
                                                   
  
              
                             
                                  
  
         
                                                     
   
kern_return_t
waitq_unlink_all_unlock(struct waitq *waitq)
{
	uint64_t old_set_id = 0;
	wqdbg_v("unlink waitq %p from all sets",
	    (void *)VM_KERNEL_UNSLIDE_OR_PERM(waitq));
	assert(!waitq_irq_safe(waitq));

	                                   
	if (waitq->waitq_set_id == 0) {
		waitq_unlock(waitq);
		return KERN_SUCCESS;
	}

	old_set_id = waitq->waitq_set_id;
	waitq->waitq_set_id = 0;

	  
                                                
                                                                      
                                                                   
                                                            
    
	(void)waitq_clear_prepost_locked(waitq);

	waitq_unlock(waitq);

	if (old_set_id) {
		  
                                                             
                                                               
                                                            
     
		(void)walk_waitq_links(LINK_WALK_ONE_LEVEL, waitq, old_set_id,
		    WQL_LINK, NULL, waitq_unlink_all_cb);
	}

	return KERN_SUCCESS;
}

   
                                                   
  
              
                        
                                       
                    
                                                            
   
kern_return_t
waitq_unlink_all(struct waitq *waitq)
{
	kern_return_t kr = KERN_SUCCESS;

	if (!waitq_valid(waitq)) {
		panic("Invalid waitq: %p", waitq);
	}

	assert(!waitq_irq_safe(waitq));
	waitq_lock(waitq);
	if (!waitq_valid(waitq)) {
		waitq_unlock(waitq);
		return KERN_SUCCESS;
	}

	kr = waitq_unlink_all_unlock(waitq);
	                                              

	return kr;
}


   
                                 
  
              
                             
                                                  
  
        
                                                           
   
kern_return_t
waitq_set_unlink_all_unlock(struct waitq_set *wqset)
{
	struct waitq_link *link;
	uint64_t prepost_id;

	wqdbg_v("unlink all queues from set 0x%llx", wqset->wqset_id);

	  
                                                                     
                                                                      
    

	if (waitqs_is_linked(wqset)) {
		                                                   
		link = wql_get_link(wqset->wqset_id);

		                                                            
		if (!link) {
			waitq_set_unlock(wqset);
			return KERN_SUCCESS;
		}

		wql_invalidate(link);

		                                                    
		wql_realloc_link(link, WQL_WQS);
		link->wql_wqs.wql_set = wqset;

		wqset->wqset_id = link->wql_setid.id;
		wql_mkvalid(link);
		wql_put_link(link);
	}

	                                             
	prepost_id = 0;
	if (wqset->wqset_q.waitq_prepost && wqset->wqset_prepost_id) {
		prepost_id = wqset->wqset_prepost_id;
	}
	                                             
	wqset->wqset_prepost_id = 0;

	  
                                                                  
                                                                  
                                                      
   
                                          
                                                                
    
	waitq_unlink_all_unlock(&wqset->wqset_q);
	                             

	                                                 
	if (prepost_id) {
		(void)wq_prepost_iterate(prepost_id, NULL,
		    wqset_clear_prepost_chain_cb);
	}

	return KERN_SUCCESS;
}

   
                                 
  
              
                        
                                                           
   
kern_return_t
waitq_set_unlink_all(struct waitq_set *wqset)
{
	assert(waitqs_is_set(wqset));
	assert(!waitq_irq_safe(&wqset->wqset_q));

	waitq_set_lock(wqset);
	return waitq_set_unlink_all_unlock(wqset);
	                                                           
}

static int
waitq_prepost_reserve_cb(struct waitq *waitq, void *ctx,
    struct waitq_link *link)
{
	uint32_t *num = (uint32_t *)ctx;
	(void)waitq;

	  
                                                               
                                                              
           
    
	if (wql_type(link) == WQL_WQS) {
		  
                                                           
               
     
		if (waitq_set_can_prepost(link->wql_wqs.wql_set)) {
			*num += 2;
		}
	}
	return WQ_ITERATE_CONTINUE;
}

static int
waitq_alloc_prepost_reservation(int nalloc, struct waitq *waitq,
    int *did_unlock, struct wq_prepost **wqp)
{
	struct wq_prepost *tmp;
	struct wqp_cache *cache;

	*did_unlock = 0;

	  
                                                                      
                                                              
                                                                       
            
    
	if (waitq) {
		disable_preemption();
		cache = PERCPU_GET(wqp_cache);
		if (nalloc <= (int)cache->avail) {
			goto do_alloc;
		}
		enable_preemption();

		                                                
		*did_unlock = 1;
		waitq_unlock(waitq);
	}

do_alloc:
	tmp = wq_prepost_alloc(LT_RESERVED, nalloc);
	if (!tmp) {
		panic("Couldn't reserve %d preposts for waitq @%p (wqp@%p)",
		    nalloc, waitq, *wqp);
	}
	if (*wqp) {
		                        
		int __assert_only rc;
		rc = wq_prepost_rlink(tmp, *wqp);
		assert(rc == nalloc);
	}
	*wqp = tmp;

	  
                                                                      
                                                                   
                                                                   
                    
    
	if (get_preemption_level() == 0) {
		wq_prepost_ensure_free_space();
	}

	if (waitq) {
		if (*did_unlock == 0) {
			                                                        
			enable_preemption();
		} else {
			                                  
			waitq_lock(waitq);
		}
	}

	return nalloc;
}

static int
waitq_count_prepost_reservation(struct waitq *waitq, int extra, int keep_locked)
{
	int npreposts = 0;

	  
                                                                       
                                                              
                                                                      
                                                                
    
	if (waitq->waitq_set_id == 0) {
		npreposts = 3;
	} else {
		                                                
		if (waitq->waitq_prepost_id == 0) {
			npreposts = 3;
		}

		  
                                                              
                                                            
                                                                
                                                               
                                                            
                                                                 
                                                              
                                                             
                                                        
                                                                
                                                        
     
		(void)walk_waitq_links(LINK_WALK_FULL_DAG_UNLOCKED,
		    waitq, waitq->waitq_set_id,
		    WQL_WQS, (void *)&npreposts,
		    waitq_prepost_reserve_cb);
	}

	if (extra > 0) {
		npreposts += extra;
	}

	if (npreposts == 0 && !keep_locked) {
		  
                                                          
                                                                
                                                      
                                                            
                                                             
                                                             
                          
     
		npreposts = 3;
	}

	return npreposts;
}


   
                                           
  
              
                        
  
           
                 
  
                                                                  
                                         
  
         
                                                                                  
                                         
  
                                                                  
                                                                 
                                                     
                                                                     
                                          
   
uint64_t
waitq_prepost_reserve(struct waitq *waitq, int extra,
    waitq_lock_state_t lock_state)
{
	uint64_t reserved = 0;
	uint64_t prev_setid = 0, prev_prepostid = 0;
	struct wq_prepost *wqp = NULL;
	int nalloc = 0, npreposts = 0;
	int keep_locked = (lock_state == WAITQ_KEEP_LOCKED);
	int unlocked = 0;

	wqdbg_v("Attempting to reserve prepost linkages for waitq %p (extra:%d)",
	    (void *)VM_KERNEL_UNSLIDE_OR_PERM(waitq), extra);

	if (waitq == NULL && extra > 0) {
		  
                                      
                                                             
                                                              
                                                      
     
		nalloc = waitq_alloc_prepost_reservation(extra + 2, NULL,
		    &unlocked, &wqp);
		assert(nalloc == extra + 2);
		return wqp->wqp_prepostid.id;
	}

	assert(lock_state == WAITQ_KEEP_LOCKED || lock_state == WAITQ_UNLOCK);

	assert(!waitq_irq_safe(waitq));

	waitq_lock(waitq);

	                                              
	prev_setid = waitq->waitq_set_id;
	prev_prepostid = waitq->waitq_prepost_id;

	  
                                                         
                                                      
             
    
	if (prev_setid == 0 && keep_locked) {
		goto out;
	}

	npreposts = waitq_count_prepost_reservation(waitq, extra, keep_locked);

	                           
	if (npreposts == 0) {
		if (keep_locked) {
			goto out;
		}
		goto out_unlock;
	}

try_alloc:
	                                             
	nalloc = waitq_alloc_prepost_reservation(npreposts, waitq,
	    &unlocked, &wqp);

	if (!unlocked) {
		                                                
		if (keep_locked) {
			goto out;
		}
		goto out_unlock;
	}

	  
                                                                   
                                                                    
                                                                    
                                                                    
                                                                     
                                         
    

	  
                                                                      
                                                                    
               
    
	if ((waitq->waitq_set_id == 0) ||
	    (waitq->waitq_set_id == prev_setid &&
	    waitq->waitq_prepost_id == prev_prepostid)) {
		if (keep_locked) {
			goto out;
		}
		goto out_unlock;
	}

	npreposts = waitq_count_prepost_reservation(waitq, extra, keep_locked);

	if (npreposts > nalloc) {
		prev_setid = waitq->waitq_set_id;
		prev_prepostid = waitq->waitq_prepost_id;
		npreposts = npreposts - nalloc;                             
		goto try_alloc;
	}

	if (keep_locked) {
		goto out;
	}

out_unlock:
	waitq_unlock(waitq);
out:
	if (wqp) {
		reserved = wqp->wqp_prepostid.id;
	}

	return reserved;
}

   
                                                                          
  
              
                                                            
   
void
waitq_prepost_release_reserve(uint64_t id)
{
	struct wq_prepost *wqp;

	wqdbg_v("releasing reserved preposts starting at: 0x%llx", id);

	wqp = wq_prepost_rfirst(id);
	if (!wqp) {
		return;
	}

	wq_prepost_release_rlist(wqp);
}


   
                                  
  
              
                        
   
void
waitq_set_clear_preposts(struct waitq_set *wqset)
{
	uint64_t prepost_id;
	spl_t spl;

	assert(waitqs_is_set(wqset));

	if (!wqset->wqset_q.waitq_prepost || !wqset->wqset_prepost_id) {
		return;
	}

	wqdbg_v("Clearing all preposted queues on waitq_set: 0x%llx",
	    wqset->wqset_id);

	if (waitq_irq_safe(&wqset->wqset_q)) {
		spl = splsched();
	}
	waitq_set_lock(wqset);
	prepost_id = wqset->wqset_prepost_id;
	wqset->wqset_prepost_id = 0;
	waitq_set_unlock(wqset);
	if (waitq_irq_safe(&wqset->wqset_q)) {
		splx(spl);
	}

	                                                 
	if (prepost_id) {
		(void)wq_prepost_iterate(prepost_id, NULL,
		    wqset_clear_prepost_chain_cb);
	}
}


                                                                         
  
                                                   
  
                                                                            

struct wq_it_ctx {
	void *input;
	void *ctx;
	waitq_iterator_t it;
};

static int
waitq_iterate_sets_cb(struct waitq *waitq, void *ctx,
    struct waitq_link *link)
{
	struct wq_it_ctx *wctx = (struct wq_it_ctx *)(ctx);
	struct waitq_set *wqset;
	int ret;

	(void)waitq;
	assert(!waitq_irq_safe(waitq));
	assert(wql_type(link) == WQL_WQS);

	  
                                                         
                                  
    
	wqset = link->wql_wqs.wql_set;
	assert(wqset != NULL);
	assert(!waitq_irq_safe(&wqset->wqset_q));
	waitq_set_lock(wqset);

	ret = wctx->it(wctx->ctx, (struct waitq *)wctx->input, wqset);

	waitq_set_unlock(wqset);
	return ret;
}

   
                                                                   
  
              
                                        
                                     
   
static int
wqset_iterate_prepost_cb(struct waitq_set *wqset, void *ctx,
    struct wq_prepost *wqp, struct waitq *waitq)
{
	struct wq_it_ctx *wctx = (struct wq_it_ctx *)(ctx);
	uint64_t wqp_id;
	int ret;

	(void)wqp;

	  
                                                                        
                                                                       
                                                                       
                                                                  
                                                                     
                                                                      
                                                                   
                                                   
    
	assert(!waitq_irq_safe(waitq));

	if (waitq_lock_try(waitq)) {
		goto call_iterator;
	}

	if (!wqp_is_valid(wqp)) {
		return WQ_ITERATE_RESTART;
	}

	                                                                    
                                                                    
                                                                 
                                                                       
                                        
    
	wqp_id = wqp->wqp_prepostid.id;
	wq_prepost_put(wqp);
	waitq_set_unlock(wqset);
	wqdbg_v("dropped set:%p lock waiting for wqp:%p (0x%llx -> wq:%p)",
	    wqset, wqp, wqp->wqp_prepostid.id, waitq);
	delay(1);
	waitq_set_lock(wqset);
	wqp = wq_prepost_get(wqp_id);
	if (!wqp) {
		                                              
		return WQ_ITERATE_DROPPED;
	}

	  
         
                                                         
                                                            
                                                                
                                                                 
                                       
    
	return WQ_ITERATE_RESTART;

call_iterator:
	if (!wqp_is_valid(wqp)) {
		ret = WQ_ITERATE_RESTART;
		goto out_unlock;
	}

	                                
	ret = wctx->it(wctx->ctx, waitq, wqset);

	if (ret == WQ_ITERATE_BREAK_KEEP_LOCKED) {
		ret = WQ_ITERATE_BREAK;
		goto out;
	}

out_unlock:
	waitq_unlock(waitq);
out:
	return ret;
}

   
                                                                  
  
              
                         
   
int
waitq_iterate_sets(struct waitq *waitq, void *ctx, waitq_iterator_t it)
{
	int ret;
	struct wq_it_ctx wctx = {
		.input = (void *)waitq,
		.ctx = ctx,
		.it = it,
	};
	if (!it || !waitq) {
		return KERN_INVALID_ARGUMENT;
	}

	ret = walk_waitq_links(LINK_WALK_ONE_LEVEL, waitq, waitq->waitq_set_id,
	    WQL_WQS, (void *)&wctx, waitq_iterate_sets_cb);
	if (ret == WQ_ITERATE_CONTINUE) {
		ret = WQ_ITERATE_SUCCESS;
	}
	return ret;
}

   
                                                
  
              
                         
   
int
waitq_set_iterate_preposts(struct waitq_set *wqset,
    void *ctx, waitq_iterator_t it)
{
	struct wq_it_ctx wctx = {
		.input = (void *)wqset,
		.ctx = ctx,
		.it = it,
	};
	if (!it || !wqset) {
		return WQ_ITERATE_INVALID;
	}

	assert(waitq_held(&wqset->wqset_q));

	return wq_prepost_foreach_locked(wqset, (void *)&wctx,
	           wqset_iterate_prepost_cb);
}


                                                                         
  
                    
  
                                                                            


   
                                                                
  
              
                        
   
wait_result_t
waitq_assert_wait64(struct waitq *waitq,
    event64_t wait_event,
    wait_interrupt_t interruptible,
    uint64_t deadline)
{
	thread_t thread = current_thread();
	wait_result_t ret;
	spl_t s;

	if (!waitq_valid(waitq)) {
		panic("Invalid waitq: %p", waitq);
	}

	if (waitq_irq_safe(waitq)) {
		s = splsched();
	}

	waitq_lock(waitq);
	ret = waitq_assert_wait64_locked(waitq, wait_event, interruptible,
	    TIMEOUT_URGENCY_SYS_NORMAL,
	    deadline, TIMEOUT_NO_LEEWAY, thread);
	waitq_unlock(waitq);

	if (waitq_irq_safe(waitq)) {
		splx(s);
	}

	return ret;
}

   
                                                                
  
              
                        
                                                                       
   
wait_result_t
waitq_assert_wait64_leeway(struct waitq *waitq,
    event64_t wait_event,
    wait_interrupt_t interruptible,
    wait_timeout_urgency_t urgency,
    uint64_t deadline,
    uint64_t leeway)
{
	wait_result_t ret;
	thread_t thread = current_thread();
	spl_t s;

	if (!waitq_valid(waitq)) {
		panic("Invalid waitq: %p", waitq);
	}

	if (waitq_irq_safe(waitq)) {
		s = splsched();
	}

	waitq_lock(waitq);
	ret = waitq_assert_wait64_locked(waitq, wait_event, interruptible,
	    urgency, deadline, leeway, thread);
	waitq_unlock(waitq);

	if (waitq_irq_safe(waitq)) {
		splx(s);
	}

	return ret;
}

   
                                                                       
  
              
                        
                                                                             
                                       
  
         
                                                                   
   
kern_return_t
waitq_wakeup64_one(struct waitq *waitq, event64_t wake_event,
    wait_result_t result, int priority)
{
	kern_return_t kr;
	uint64_t reserved_preposts = 0;
	spl_t spl;

	if (!waitq_valid(waitq)) {
		panic("Invalid waitq: %p", waitq);
	}

	if (!waitq_irq_safe(waitq)) {
		                                                       
		reserved_preposts = waitq_prepost_reserve(waitq, 0, WAITQ_KEEP_LOCKED);
	} else {
		spl = splsched();
		waitq_lock(waitq);
	}

	                                 
	kr = waitq_wakeup64_one_locked(waitq, wake_event, result,
	    &reserved_preposts, priority, WAITQ_UNLOCK, WQ_OPTION_NONE);

	if (waitq_irq_safe(waitq)) {
		splx(spl);
	}

	                                                                      
	waitq_prepost_release_reserve(reserved_preposts);

	return kr;
}

   
                                                                     
  
              
                        
                                                                             
                                       
  
         
                                                                   
   
kern_return_t
waitq_wakeup64_all(struct waitq *waitq,
    event64_t wake_event,
    wait_result_t result,
    int priority)
{
	kern_return_t ret;
	uint64_t reserved_preposts = 0;
	spl_t s;

	if (!waitq_valid(waitq)) {
		panic("Invalid waitq: %p", waitq);
	}

	if (!waitq_irq_safe(waitq)) {
		                                                   
		reserved_preposts = waitq_prepost_reserve(waitq, 0,
		    WAITQ_KEEP_LOCKED);
	} else {
		s = splsched();
		waitq_lock(waitq);
	}

	ret = waitq_wakeup64_all_locked(waitq, wake_event, result,
	    &reserved_preposts, priority,
	    WAITQ_UNLOCK);

	if (waitq_irq_safe(waitq)) {
		splx(s);
	}

	waitq_prepost_release_reserve(reserved_preposts);

	return ret;
}

   
                                                                        
  
              
                        
  
         
                                                   
   
kern_return_t
waitq_wakeup64_thread(struct waitq *waitq,
    event64_t wake_event,
    thread_t thread,
    wait_result_t result)
{
	kern_return_t ret;
	spl_t s, th_spl;

	if (!waitq_valid(waitq)) {
		panic("Invalid waitq: %p", waitq);
	}

	if (waitq_irq_safe(waitq)) {
		s = splsched();
	}
	waitq_lock(waitq);

	ret = waitq_select_thread_locked(waitq, wake_event, thread, &th_spl);
	                                         

	waitq_unlock(waitq);

	if (ret == KERN_SUCCESS) {
		ret = thread_go(thread, result, WQ_OPTION_NONE);
		assert(ret == KERN_SUCCESS);
		thread_unlock(thread);
		splx(th_spl);
		waitq_stats_count_wakeup(waitq);
	} else {
		ret = KERN_NOT_WAITING;
		waitq_stats_count_fail(waitq);
	}

	if (waitq_irq_safe(waitq)) {
		splx(s);
	}

	return ret;
}

   
                                                                       
                                        
                                               
  
              
                        
                                                                             
                                       
  
         
                                                                   
   
thread_t
waitq_wakeup64_identify(struct waitq    *waitq,
    event64_t       wake_event,
    wait_result_t   result,
    int             priority)
{
	uint64_t reserved_preposts = 0;
	spl_t thread_spl = 0;
	thread_t thread;
	spl_t spl;

	if (!waitq_valid(waitq)) {
		panic("Invalid waitq: %p", waitq);
	}

	if (!waitq_irq_safe(waitq)) {
		                                                   
		reserved_preposts = waitq_prepost_reserve(waitq, 0, WAITQ_KEEP_LOCKED);
	} else {
		spl = splsched();
		waitq_lock(waitq);
	}

	thread = waitq_wakeup64_identify_locked(waitq, wake_event, result,
	    &thread_spl, &reserved_preposts,
	    priority, WAITQ_UNLOCK);
	                                         

	if (thread != THREAD_NULL) {
		thread_reference(thread);
		thread_unlock(thread);
		splx(thread_spl);
	}

	if (waitq_irq_safe(waitq)) {
		splx(spl);
	}

	                                                                      
	waitq_prepost_release_reserve(reserved_preposts);

	                                                     
	return thread;
}
