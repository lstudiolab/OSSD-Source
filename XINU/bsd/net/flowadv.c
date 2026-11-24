/*
 * Copyright (c) 2012-2020 Apple Inc. All rights reserved.
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

  
                                     
  
                                                                         
                                                                           
                                                                         
                                                                            
                                                                            
                                 
  
                                                                  
                                                                       
                                                                       
                                                         
  
                                                                      
                                                                   
                                                                      
                                                                     
                                                                     
                                                                        
                                                                     
                           
  
                                                                      
                                                                        
                                                                       
                                                                    
                                                                         
                                                                        
                                                                       
                                                   
  
                                                                             
                                                                          
                                                                            
                                                                            
                                                                          
                                                     
  
                                                                            
                                                                            
                                                                          
                                                                           
                                                   
  
                                                                          
                                                                            
                                                                            
                                                                            
                                                                   
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mcache.h>
#include <sys/mbuf.h>
#include <sys/proc_internal.h>
#include <sys/socketvar.h>

#include <kern/assert.h>
#include <kern/thread.h>
#include <kern/locks.h>
#include <kern/zalloc.h>

#include <netinet/in_pcb.h>
#include <net/flowadv.h>

                                            
static lck_grp_t        *fadv_lock_grp;
static lck_grp_attr_t   *fadv_lock_grp_attr;
decl_lck_mtx_data(static, fadv_lock);

                            
static STAILQ_HEAD(fadv_head, flowadv_fcentry) fadv_list;
static thread_t fadv_thread = THREAD_NULL;
static uint32_t fadv_active;

static unsigned int fadv_size;                                               
static struct mcache *fadv_cache;                                               

#define FADV_CACHE_NAME  "flowadv"                              

static int flowadv_thread_cont(int);
static void flowadv_thread_func(void *, wait_result_t);

void
flowadv_init(void)
{
	STAILQ_INIT(&fadv_list);

	                                                  
	fadv_lock_grp_attr = lck_grp_attr_alloc_init();
	fadv_lock_grp = lck_grp_alloc_init("fadv_lock", fadv_lock_grp_attr);
	lck_mtx_init(&fadv_lock, fadv_lock_grp, NULL);

	fadv_size = sizeof(struct flowadv_fcentry);
	fadv_cache = mcache_create(FADV_CACHE_NAME, fadv_size,
	    sizeof(uint64_t), 0, MCR_SLEEP);

	if (kernel_thread_start(flowadv_thread_func, NULL, &fadv_thread) !=
	    KERN_SUCCESS) {
		panic("%s: couldn't create flow event advisory thread",
		    __func__);
		                
	}
	thread_deallocate(fadv_thread);
}

struct flowadv_fcentry *
flowadv_alloc_entry(int how)
{
	struct flowadv_fcentry *fce;

	if ((fce = mcache_alloc(fadv_cache, (how == M_WAITOK) ?
	    MCR_SLEEP : MCR_NOSLEEP)) != NULL) {
		bzero(fce, fadv_size);
	}

	return fce;
}

void
flowadv_free_entry(struct flowadv_fcentry *fce)
{
	mcache_free(fadv_cache, fce);
}

void
flowadv_add(struct flowadv_fclist *fcl)
{
	if (STAILQ_EMPTY(fcl)) {
		return;
	}

	lck_mtx_lock_spin(&fadv_lock);

	STAILQ_CONCAT(&fadv_list, fcl);
	VERIFY(!STAILQ_EMPTY(&fadv_list));

	if (!fadv_active && fadv_thread != THREAD_NULL) {
		wakeup_one((caddr_t)&fadv_list);
	}

	lck_mtx_unlock(&fadv_lock);
}

void
flowadv_add_entry(struct flowadv_fcentry *fce)
{
	lck_mtx_lock_spin(&fadv_lock);
	STAILQ_INSERT_HEAD(&fadv_list, fce, fce_link);
	VERIFY(!STAILQ_EMPTY(&fadv_list));

	if (!fadv_active && fadv_thread != THREAD_NULL) {
		wakeup_one((caddr_t)&fadv_list);
	}

	lck_mtx_unlock(&fadv_lock);
}

static int
flowadv_thread_cont(int err)
{
#pragma unused(err)
	for (;;) {
		LCK_MTX_ASSERT(&fadv_lock, LCK_MTX_ASSERT_OWNED);
		while (STAILQ_EMPTY(&fadv_list)) {
			VERIFY(!fadv_active);
			(void) msleep0(&fadv_list, &fadv_lock, (PSOCK | PSPIN),
			    "flowadv_cont", 0, flowadv_thread_cont);
			                
		}

		fadv_active = 1;
		for (;;) {
			struct flowadv_fcentry *fce;

			VERIFY(!STAILQ_EMPTY(&fadv_list));
			fce = STAILQ_FIRST(&fadv_list);
			STAILQ_REMOVE(&fadv_list, fce,
			    flowadv_fcentry, fce_link);
			STAILQ_NEXT(fce, fce_link) = NULL;

			lck_mtx_unlock(&fadv_lock);
			switch (fce->fce_flowsrc_type) {
			case FLOWSRC_INPCB:
				inp_flowadv(fce->fce_flowid);
				break;

			case FLOWSRC_IFNET:
				ifnet_flowadv(fce->fce_flowid);
				break;


			case FLOWSRC_PF:
			default:
				break;
			}
			flowadv_free_entry(fce);
			lck_mtx_lock_spin(&fadv_lock);

			                                               
			if (STAILQ_EMPTY(&fadv_list)) {
				break;
			}
		}
		fadv_active = 0;
	}
}

__dead2
static void
flowadv_thread_func(void *v, wait_result_t w)
{
#pragma unused(v, w)
	lck_mtx_lock(&fadv_lock);
	(void) msleep0(&fadv_list, &fadv_lock, (PSOCK | PSPIN),
	    "flowadv", 0, flowadv_thread_cont);
	  
                                                            
                                  
    
	lck_mtx_unlock(&fadv_lock);
	VERIFY(0);
}

void
flowadv_reap_caches(boolean_t purge)
{
	mcache_reap_now(fadv_cache, purge);
}
