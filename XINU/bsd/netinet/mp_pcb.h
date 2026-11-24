/*
 * Copyright (c) 2012-2017 Apple Inc. All rights reserved.
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

#ifndef _NETINET_MP_PCB_H_
#define _NETINET_MP_PCB_H_

#ifdef BSD_KERNEL_PRIVATE
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <kern/locks.h>

                                                      
typedef enum mppcb_state {
	MPPCB_STATE_INUSE       = 1,
	MPPCB_STATE_DEAD        = 2,
} mppcb_state_t;

  
                                   
   
struct mppcb {
	TAILQ_ENTRY(mppcb)      mpp_entry;                            
	decl_lck_mtx_data(, mpp_lock);                            
	struct mppcbinfo        *mpp_pcbinfo;                 
	struct mptses           *mpp_pcbe;                                
	struct socket           *mpp_socket;                                
	uint32_t                mpp_flags;                     
	mppcb_state_t           mpp_state;                     
	int32_t                 mpp_inside;                                                                

#if NECP
	uuid_t necp_client_uuid;
	void (*necp_cb)(void *, int, uint32_t, uint32_t, bool *);
#endif
};

static inline struct mppcb *
mpsotomppcb(struct socket *mp_so)
{
	VERIFY(SOCK_DOM(mp_so) == PF_MULTIPATH);
	return (struct mppcb *)mp_so->so_pcb;
}

                                
#define MPP_ATTACHED            0x001
#define MPP_INSIDE_OUTPUT       0x002                                                           
#define MPP_INSIDE_INPUT        0x004                                                          
#define MPP_INPUT_HANDLE        0x008                                              
#define MPP_WUPCALL             0x010                                                      
#define MPP_SHOULD_WORKLOOP     0x020                                                              
#define MPP_SHOULD_RWAKEUP      0x040                                                  
#define MPP_SHOULD_WWAKEUP      0x080                                                  
#define MPP_CREATE_SUBFLOWS     0x100                                                         

static inline boolean_t
mptcp_should_defer_upcall(struct mppcb *mpp)
{
	return !!(mpp->mpp_flags & (MPP_INSIDE_OUTPUT | MPP_INSIDE_INPUT | MPP_INPUT_HANDLE | MPP_WUPCALL));
}

  
                            
   
struct mppcbinfo {
	TAILQ_ENTRY(mppcbinfo)  mppi_entry;                               
	TAILQ_HEAD(, mppcb)     mppi_pcbs;                        
	uint32_t                mppi_count;                            
	struct zone             *mppi_zone;                            
	uint32_t                mppi_size;                                 
	lck_grp_t               *mppi_lock_grp;               
	lck_attr_t              *mppi_lock_attr;                
	lck_grp_attr_t          *mppi_lock_grp_attr;                    
	decl_lck_mtx_data(, mppi_lock);                              
	uint32_t (*mppi_gc)(struct mppcbinfo *);                             
	uint32_t (*mppi_timer)(struct mppcbinfo *);                 
};

__BEGIN_DECLS
extern void mp_pcbinit(void);
extern void mp_pcbinfo_attach(struct mppcbinfo *);
extern int mp_pcbinfo_detach(struct mppcbinfo *);
extern int mp_pcballoc(struct socket *, struct mppcbinfo *);
extern void mp_pcbdetach(struct socket *);
extern void mptcp_pcbdispose(struct mppcb *);
extern void mp_gc_sched(void);
extern void mptcp_timer_sched(void);
extern void mptcp_handle_deferred_upcalls(struct mppcb *mpp, uint32_t flag);
extern int mp_getsockaddr(struct socket *mp_so, struct sockaddr **nam);
extern int mp_getpeeraddr(struct socket *mp_so, struct sockaddr **nam);
#if NECP
extern int necp_client_register_multipath_cb(pid_t pid, uuid_t client_id, struct mppcb *mpp);
extern void necp_mppcb_dispose(struct mppcb *mpp);
#endif
__END_DECLS

#endif                         
#endif                          
