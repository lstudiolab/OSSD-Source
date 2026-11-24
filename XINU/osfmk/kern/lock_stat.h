/*
 * Copyright (c) 2018 Apple Computer, Inc. All rights reserved.
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
#ifndef _KERN_LOCKSTAT_H
#define _KERN_LOCKSTAT_H
#include <machine/locks.h>
#include <machine/atomic.h>
#include <kern/lock_group.h>

  
                                                                            
                                                                         
                                                                            
                         
  
  
                                                                 
                                                                
                                                              
   
#undef LOG_FIRST_MISS_ALONE

  
                                                                    
                                                           
                      
   
#define ATOMIC_STAT_UPDATES 1

  
                                    
  
   

enum lockstat_probe_id {
	               
	LS_LCK_SPIN_LOCK_ACQUIRE,
	LS_LCK_SPIN_LOCK_SPIN,
	LS_LCK_SPIN_UNLOCK_RELEASE,

	  
                                                          
                                      
    
	LS_LCK_MTX_LOCK_ACQUIRE,
	LS_LCK_MTX_LOCK_BLOCK,
	LS_LCK_MTX_LOCK_SPIN,
	LS_LCK_MTX_LOCK_ILK_SPIN,
	LS_LCK_MTX_TRY_LOCK_ACQUIRE,
	LS_LCK_MTX_TRY_SPIN_LOCK_ACQUIRE,
	LS_LCK_MTX_UNLOCK_RELEASE,
	LS_LCK_MTX_LOCK_SPIN_ACQUIRE,

	  
                                               
    
	LS_LCK_MTX_EXT_LOCK_ACQUIRE,
	LS_LCK_MTX_EXT_LOCK_BLOCK,
	LS_LCK_MTX_EXT_LOCK_SPIN,
	LS_LCK_MTX_EXT_LOCK_ILK_SPIN,
	LS_LCK_MTX_EXT_UNLOCK_RELEASE,

	  
                                                                
                                                         
    
	LS_LCK_RW_LOCK_SHARED_ACQUIRE,
	LS_LCK_RW_LOCK_SHARED_BLOCK,
	LS_LCK_RW_LOCK_SHARED_SPIN,

	LS_LCK_RW_LOCK_EXCL_ACQUIRE,
	LS_LCK_RW_LOCK_EXCL_BLOCK,
	LS_LCK_RW_LOCK_EXCL_SPIN,

	LS_LCK_RW_DONE_RELEASE,

	LS_LCK_RW_TRY_LOCK_SHARED_ACQUIRE,
	LS_LCK_RW_TRY_LOCK_SHARED_SPIN,

	LS_LCK_RW_TRY_LOCK_EXCL_ACQUIRE,
	LS_LCK_RW_TRY_LOCK_EXCL_ILK_SPIN,

	LS_LCK_RW_LOCK_SHARED_TO_EXCL_UPGRADE,
	LS_LCK_RW_LOCK_SHARED_TO_EXCL_SPIN,
	LS_LCK_RW_LOCK_SHARED_TO_EXCL_BLOCK,

	LS_LCK_RW_LOCK_EXCL_TO_SHARED_DOWNGRADE,
	LS_LCK_RW_LOCK_EXCL_TO_SHARED_ILK_SPIN,

	                 
	LS_LCK_TICKET_LOCK_ACQUIRE,
	LS_LCK_TICKET_LOCK_RELEASE,
	LS_LCK_TICKET_LOCK_SPIN,

	LS_NPROBES
};

#if CONFIG_DTRACE
extern uint32_t lockstat_probemap[LS_NPROBES];
extern void dtrace_probe(uint32_t, uint64_t, uint64_t,
    uint64_t, uint64_t, uint64_t);
  
                                    
   
#define LOCKSTAT_RECORD4(probe, lp, arg0, arg1, arg2, arg3)             \
	{                                                                   \
	        uint32_t id;                                                \
	        if (__improbable(id = lockstat_probemap[(probe)])) {        \
	                dtrace_probe(id, (uintptr_t)(lp), (arg0),           \
	                    (arg1), (arg2), (arg3));                        \
	        }                                                           \
	}
#define LOCKSTAT_RECORD_(probe, lp, arg0, arg1, arg2, arg3, ...) LOCKSTAT_RECORD4(probe, lp, arg0, arg1, arg2, arg3)
#define LOCKSTAT_RECORD__(probe, lp, arg0, arg1, arg2, arg3, ...) LOCKSTAT_RECORD_(probe, lp, arg0, arg1, arg2, arg3)
#define LOCKSTAT_RECORD(probe, lp, ...) LOCKSTAT_RECORD__(probe, lp, ##__VA_ARGS__, 0, 0, 0, 0)
#else
#define LOCKSTAT_RECORD()
#endif                    

  
                                             
                       
   
extern uint64_t dtrace_spin_threshold;

#if CONFIG_DTRACE
void lockprof_invoke(lck_grp_t*, lck_grp_stat_t*, uint64_t);
#endif                    

static inline void
lck_grp_stat_enable(lck_grp_stat_t *stat)
{
	stat->lgs_enablings++;
}

static inline void
lck_grp_stat_disable(lck_grp_stat_t *stat)
{
	stat->lgs_enablings--;
}

#if MACH_KERNEL_PRIVATE
static inline void
lck_grp_inc_stats(lck_grp_t *grp, lck_grp_stat_t *stat)
{
#pragma unused(grp)
	if (__improbable(stat->lgs_enablings)) {
#if ATOMIC_STAT_UPDATES
		uint64_t val = os_atomic_inc_orig(&stat->lgs_count, relaxed);
#else
		uint64_t val = stat->lgs_count++;
#endif                          
#if CONFIG_DTRACE && LOCK_STATS
		if (__improbable(stat->lgs_limit && (val % (stat->lgs_limit)) == 0)) {
			lockprof_invoke(grp, stat, val);
		}
#else
#pragma unused(val)
#endif                                  
	}
}

#if LOCK_STATS
static inline void
lck_grp_inc_time_stats(lck_grp_t *grp, lck_grp_stat_t *stat, uint64_t time)
{
	if (__improbable(stat->lgs_enablings)) {
		uint64_t val = os_atomic_add_orig(&stat->lgs_count, time, relaxed);
#if CONFIG_DTRACE
		if (__improbable(stat->lgs_limit)) {
			while (__improbable(time > stat->lgs_limit)) {
				time -= stat->lgs_limit;
				lockprof_invoke(grp, stat, val);
			}
			if (__improbable(((val % stat->lgs_limit) + time) > stat->lgs_limit)) {
				lockprof_invoke(grp, stat, val);
			}
		}
#else
#pragma unused(val)
#endif                    
	}
}

#endif                 

static inline void
lck_grp_spin_update_held(void *lock LCK_GRP_ARG(lck_grp_t *grp))
{
#pragma unused(lock)
#if CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_SPIN_LOCK_ACQUIRE, lock, (uintptr_t)LCK_GRP_PROBEARG(grp));
#endif
#if LOCK_STATS
	if (!grp) {
		return;
	}
	lck_grp_stat_t *stat = &grp->lck_grp_stats.lgss_spin_held;
	lck_grp_inc_stats(grp, stat);
#endif                 
}

static inline void
lck_grp_spin_update_miss(void *lock LCK_GRP_ARG(lck_grp_t *grp))
{
#pragma unused(lock)
#if LOCK_STATS
	if (!grp) {
		return;
	}
	lck_grp_stat_t *stat = &grp->lck_grp_stats.lgss_spin_miss;
	lck_grp_inc_stats(grp, stat);
#endif                 
}

static inline void
lck_grp_spin_update_spin(void *lock LCK_GRP_ARG(lck_grp_t *grp), uint64_t time)
{
#pragma unused(lock, time)
#if CONFIG_DTRACE
	if (time > dtrace_spin_threshold) {
		LOCKSTAT_RECORD(LS_LCK_SPIN_LOCK_SPIN, lock, time LCK_GRP_ARG((uintptr_t)grp));
	}
#endif                    
#if LOCK_STATS
	if (!grp) {
		return;
	}
	lck_grp_stat_t *stat = &grp->lck_grp_stats.lgss_spin_spin;
	lck_grp_inc_time_stats(grp, stat, time);
#endif                 
}

static inline boolean_t
lck_grp_spin_spin_enabled(void *lock LCK_GRP_ARG(lck_grp_t *grp))
{
#pragma unused(lock)
	boolean_t enabled = FALSE;
#if CONFIG_DTRACE
	enabled |= lockstat_probemap[LS_LCK_SPIN_LOCK_SPIN] != 0;
#endif                    
#if LOCK_STATS
	enabled |= (grp && grp->lck_grp_stats.lgss_spin_spin.lgs_enablings);
#endif                 
	return enabled;
}

static inline void
lck_grp_ticket_update_held(void *lock LCK_GRP_ARG(lck_grp_t *grp))
{
#pragma unused(lock)
#if CONFIG_DTRACE
	LOCKSTAT_RECORD(LS_LCK_TICKET_LOCK_ACQUIRE, lock, (uintptr_t)LCK_GRP_PROBEARG(grp));
#endif
#if LOCK_STATS
	if (!grp) {
		return;
	}
	lck_grp_stat_t *stat = &grp->lck_grp_stats.lgss_ticket_held;
	lck_grp_inc_stats(grp, stat);
#endif                 
}

static inline void
lck_grp_ticket_update_miss(void *lock LCK_GRP_ARG(lck_grp_t *grp))
{
#pragma unused(lock)
#if LOCK_STATS
	if (!grp) {
		return;
	}
	lck_grp_stat_t *stat = &grp->lck_grp_stats.lgss_ticket_miss;
	lck_grp_inc_stats(grp, stat);
#endif                 
}

static inline boolean_t
lck_grp_ticket_spin_enabled(void *lock LCK_GRP_ARG(lck_grp_t *grp))
{
#pragma unused(lock)
	boolean_t enabled = FALSE;
#if CONFIG_DTRACE
	enabled |= lockstat_probemap[LS_LCK_TICKET_LOCK_SPIN] != 0;
#endif                    
#if LOCK_STATS
	enabled |= (grp && grp->lck_grp_stats.lgss_ticket_spin.lgs_enablings);
#endif                 
	return enabled;
}

static inline void
lck_grp_ticket_update_spin(void *lock LCK_GRP_ARG(lck_grp_t *grp), uint64_t time)
{
#pragma unused(lock, time)
#if CONFIG_DTRACE
	if (time > dtrace_spin_threshold) {
		LOCKSTAT_RECORD(LS_LCK_TICKET_LOCK_SPIN, lock, time LCK_GRP_ARG((uintptr_t)grp));
	}
#endif                    
#if LOCK_STATS
	if (!grp) {
		return;
	}
	lck_grp_stat_t *stat = &grp->lck_grp_stats.lgss_ticket_spin;
	lck_grp_inc_time_stats(grp, stat, time);
#endif                 
}


static void inline
lck_grp_mtx_update_miss(
	struct _lck_mtx_ext_ *lock,
	int *first_miss)
{
#pragma unused(first_miss)
#if LOG_FIRST_MISS_ALONE
	if ((*first_miss & 1) == 0) {
#endif                           
	lck_grp_t *grp = lock->lck_mtx_grp;
	lck_grp_stat_t *stat = &grp->lck_grp_stats.lgss_mtx_miss;
	lck_grp_inc_stats(grp, stat);

#if LOG_FIRST_MISS_ALONE
	*first_miss |= 1;
}
#endif                           
}

static void inline
lck_grp_mtx_update_direct_wait(
	struct _lck_mtx_ext_ *lock)
{
	lck_grp_t *grp = lock->lck_mtx_grp;
	lck_grp_stat_t *stat = &grp->lck_grp_stats.lgss_mtx_direct_wait;
	lck_grp_inc_stats(grp, stat);
}

static void inline
lck_grp_mtx_update_wait(
	struct _lck_mtx_ext_ *lock,
	int *first_miss)
{
#pragma unused(first_miss)
#if LOG_FIRST_MISS_ALONE
	if ((*first_miss & 2) == 0) {
#endif                           
	lck_grp_t *grp = lock->lck_mtx_grp;
	lck_grp_stat_t *stat = &grp->lck_grp_stats.lgss_mtx_wait;
	lck_grp_inc_stats(grp, stat);
#if LOG_FIRST_MISS_ALONE
	*first_miss |= 2;
}
#endif                           
}

static void inline
lck_grp_mtx_update_held(
	struct _lck_mtx_ext_ *lock)
{
	lck_grp_t *grp = lock->lck_mtx_grp;
	lck_grp_stat_t *stat = &grp->lck_grp_stats.lgss_mtx_held;
	lck_grp_inc_stats(grp, stat);
}

#endif                          
#endif                       
