/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
/*
 * Copyright (c) 1985, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)in_var.h	8.2 (Berkeley) 1/9/95
 */

#ifndef _NETINET_IN_VAR_H_
#define _NETINET_IN_VAR_H_
#include <sys/appleapiopts.h>

#include <sys/queue.h>
#include <sys/kern_event.h>
#include <net/net_kev.h>

#ifdef BSD_KERNEL_PRIVATE
#include <net/route.h>

  
                                                                
                                                          
                                                              
                                               
   
struct in_ifaddr {
	struct ifaddr ia_ifa;                                          
#define ia_ifp          ia_ifa.ifa_ifp
#define ia_flags        ia_ifa.ifa_flags
	                                       
	u_int32_t ia_net;                                                
	u_int32_t ia_netmask;                                 
	u_int32_t ia_subnet;                                              
	u_int32_t ia_subnetmask;                                 
	struct in_addr ia_netbroadcast;                                  
	TAILQ_ENTRY(in_ifaddr) ia_link;                       
	struct sockaddr_in ia_addr;                                           
	struct sockaddr_in ia_dstaddr;                                        
#define ia_broadaddr    ia_dstaddr
	struct sockaddr_in ia_sockmask;                                        
	TAILQ_ENTRY(in_ifaddr) ia_hash;                        
};

#define ifatoia(ifa)    ((struct in_ifaddr *)(void *)(ifa))
#endif                         

struct in_aliasreq {
	char ifra_name[IFNAMSIZ];                                
	struct sockaddr_in ifra_addr;
	struct sockaddr_in ifra_broadaddr;
#define ifra_dstaddr    ifra_broadaddr
	struct sockaddr_in ifra_mask;
};

  
                          
   
struct kev_in_data {
	struct net_event_data link_data;
	struct in_addr ia_addr;                                
	u_int32_t ia_net;                                                
	u_int32_t ia_netmask;                                 
	u_int32_t ia_subnet;                                              
	u_int32_t ia_subnetmask;                                 
	struct in_addr ia_netbroadcast;                                  
	struct in_addr ia_dstaddr;
};

struct kev_in_collision {
	struct net_event_data link_data;                                     
	struct in_addr ia_ipaddr;                                   
	u_char hw_len;                                                  
	u_char hw_addr[0];                                                    
};

struct kev_in_arpfailure {
	struct net_event_data link_data;                                   
};

struct kev_in_arpalive {
	struct net_event_data link_data;                                  
};

#ifdef PRIVATE
  
                                           
                                                        
   
struct kev_socket_event_data {
	union sockaddr_in_4_6 kev_sockname;
	union sockaddr_in_4_6 kev_peername;
};

struct kev_socket_closed {
	struct kev_socket_event_data ev_data;
};
#endif              

#ifdef __APPLE_API_PRIVATE
struct kev_in_portinuse {
	u_int16_t port;                                                    
	u_int32_t req_pid;                              
	u_int32_t reserved[2];
};
#endif                          

#ifdef BSD_KERNEL_PRIVATE
extern void socket_post_kev_msg(uint32_t, struct kev_socket_event_data *,
    uint32_t);
extern void socket_post_kev_msg_closed(struct socket *);

#include <net/if.h>
#include <net/if_var.h>
#include <net/if_llatbl.h>
#include <kern/locks.h>
#include <sys/tree.h>
  
                                            
                                                 
   
#define IA_SIN(ia)    (&(((struct in_ifaddr *)(ia))->ia_addr))
#define IA_DSTSIN(ia) (&(((struct in_ifaddr *)(ia))->ia_dstaddr))

#define IN_LNAOF(in, ifa) \
	((ntohl((in).s_addr) & ~((struct in_ifaddr *)(ifa)->ia_subnetmask))

  
                                 
   
extern TAILQ_HEAD(in_ifaddrhead, in_ifaddr) in_ifaddrhead;
extern TAILQ_HEAD(in_ifaddrhashhead, in_ifaddr) * in_ifaddrhashtbl;
extern lck_rw_t *in_ifaddr_rwlock;

#define INADDR_HASH(x)  (&in_ifaddrhashtbl[inaddr_hashval(x)])

extern  u_char  inetctlerrmap[];

  
                                                                         
                       
   
#define INADDR_TO_IFP(addr, ifp)                                        \
	                                                                \
	                                                                \
{                                                                       \
	struct in_ifaddr *ia;                                           \
                                                                        \
	lck_rw_lock_shared(in_ifaddr_rwlock);                           \
	TAILQ_FOREACH(ia, INADDR_HASH((addr).s_addr), ia_hash) {        \
	        IFA_LOCK_SPIN(&ia->ia_ifa);                             \
	        if (IA_SIN(ia)->sin_addr.s_addr == (addr).s_addr) {     \
	                IFA_UNLOCK(&ia->ia_ifa);                        \
	                break;                                          \
	        }                                                       \
	        IFA_UNLOCK(&ia->ia_ifa);                                \
	}                                                               \
	(ifp) = (ia == NULL) ? NULL : ia->ia_ifp;                       \
	lck_rw_done(in_ifaddr_rwlock);                                  \
}

  
                                                                             
                                                                             
                 
   
#define IFP_TO_IA(ifp, ia)                                              \
	                                                                \
	                                                                \
{                                                                       \
	lck_rw_lock_shared(in_ifaddr_rwlock);                           \
	for ((ia) = TAILQ_FIRST(&in_ifaddrhead);                        \
	    (ia) != NULL && (ia)->ia_ifp != (ifp);                      \
	    (ia) = TAILQ_NEXT((ia), ia_link))                           \
	        continue;                                               \
	if ((ia) != NULL)                                               \
	        IFA_ADDREF(&(ia)->ia_ifa);                              \
	lck_rw_done(in_ifaddr_rwlock);                                  \
}

  
                                                                           
                                                        
   

  
                                       
   
struct router_info {
	struct ifnet *rti_ifp;
	int    rti_type;                                                        
	int    rti_time;                                              
	SLIST_ENTRY(router_info) rti_list;
};

  
                                          
   
struct ip_msource {
	RB_ENTRY(ip_msource)    ims_link;                          
	in_addr_t               ims_haddr;                           
	struct ims_st {
		uint16_t        ex;                                         
		uint16_t        in;                                         
	}                       ims_st[2];                           
	uint8_t                 ims_stp;                           
};

  
                                         
   
struct in_msource {
	RB_ENTRY(ip_msource)    ims_link;                          
	in_addr_t               ims_haddr;                           
	uint8_t                 imsl_st[2];                                 
};

RB_HEAD(ip_msource_tree, ip_msource);                                      

RB_PROTOTYPE_SC_PREV(__private_extern__, ip_msource_tree, ip_msource,
    ims_link, ip_msource_cmp);

  
                                                    
   
struct in_mfilter {
	struct ip_msource_tree  imf_sources;                            
	u_long                  imf_nsrc;                             
	uint8_t                 imf_st[2];                               
};

struct igmp_ifinfo;

  
                         
  
                                                                    
                                                           
  
                                                                           
                                                                     
                                                          
                                                                   
                            
  
                                                                         
                                                                         
                                                                      
                                             
  
                                                                        
                                       
  
                                                                 
                                                               
   
struct in_multi {
	decl_lck_mtx_data(, inm_lock);
	u_int32_t inm_refcount;                              
	u_int32_t inm_reqcnt;                                               
	u_int32_t inm_debug;                                     
	LIST_ENTRY(in_multi) inm_link;                        
	struct  in_addr inm_addr;                                              
	struct  ifnet *inm_ifp;                                    
	struct  ifmultiaddr *inm_ifma;                                   
	u_int   inm_timer;                                                     
	u_int   inm_state;                                            
	void *inm_rti;                                            

	                                   
	struct igmp_ifinfo      *inm_igi;                      
	SLIST_ENTRY(in_multi)    inm_dtle;                                     
	SLIST_ENTRY(in_multi)    inm_nrele;                                 
	u_int32_t                inm_nrelecnt;                              
	struct ip_msource_tree   inm_srcs;                           
	u_long                   inm_nsrc;                             

	struct ifqueue           inm_scq;                          
                                                                   
	struct timeval           inm_lastgsrtv;                               
	uint16_t                 inm_sctimer;                           
	uint16_t                 inm_scrv;                                     

	  
                                                                 
                                                       
                                       
                                                                     
                                                         
    
	struct inm_st {
		uint16_t        iss_fmode;                            
		uint16_t        iss_asm;                                
		uint16_t        iss_ex;                                     
		uint16_t        iss_in;                                     
		uint16_t        iss_rec;                                   
	}                       inm_st[2];                           

	void (*inm_trace)                                                 
	(struct in_multi *, int);
};

#define INM_LOCK_ASSERT_HELD(_inm)                                      \
	LCK_MTX_ASSERT(&(_inm)->inm_lock, LCK_MTX_ASSERT_OWNED)

#define INM_LOCK_ASSERT_NOTHELD(_inm)                                   \
	LCK_MTX_ASSERT(&(_inm)->inm_lock, LCK_MTX_ASSERT_NOTOWNED)

#define INM_LOCK(_inm)                                                  \
	lck_mtx_lock(&(_inm)->inm_lock)

#define INM_LOCK_SPIN(_inm)                                             \
	lck_mtx_lock_spin(&(_inm)->inm_lock)

#define INM_CONVERT_LOCK(_inm) do {                                     \
	INM_LOCK_ASSERT_HELD(_inm);                                     \
	lck_mtx_convert_spin(&(_inm)->inm_lock);                        \
} while (0)

#define INM_UNLOCK(_inm)                                                \
	lck_mtx_unlock(&(_inm)->inm_lock)

#define INM_ADDREF(_inm)                                                \
	inm_addref(_inm, 0)

#define INM_ADDREF_LOCKED(_inm)                                         \
	inm_addref(_inm, 1)

#define INM_REMREF(_inm)                                                \
	inm_remref(_inm, 0)

#define INM_REMREF_LOCKED(_inm)                                         \
	inm_remref(_inm, 1)

#ifdef SYSCTL_DECL
SYSCTL_DECL(_net_inet_ip);
SYSCTL_DECL(_net_inet_raw);
#endif

extern LIST_HEAD(in_multihead, in_multi) in_multihead;

  
                                                                            
                               
   
struct in_multistep {
	struct in_multi *i_inm;
};

  
                                                                            
                                                                            
  
                                                                        
                                                                    
                                                                      
                                                                      
                                                                       
                                                                        
                                            
  
                                              
   
#define IN_LOOKUP_MULTI(addr, ifp, inm)                                 \
	                                                                \
	                                                                \
	                                                                \
do {                                                                    \
	struct in_multistep _step;                                      \
	IN_FIRST_MULTI(_step, inm);                                     \
	while ((inm) != NULL) {                                         \
	        INM_LOCK_SPIN(inm);                                     \
	        if ((inm)->inm_ifp == (ifp) &&                          \
	            (inm)->inm_addr.s_addr == (addr)->s_addr) {         \
	                INM_ADDREF_LOCKED(inm);                         \
	                INM_UNLOCK(inm);                                \
	                break;                                          \
	        }                                                       \
	        INM_UNLOCK(inm);                                        \
	        IN_NEXT_MULTI(_step, inm);                              \
	}                                                               \
} while (0)

  
                                                                    
                                                                      
                                                                         
                                                                        
                            
  
                                              
   
#define IN_NEXT_MULTI(step, inm)                                        \
	                                                                \
	                                                                \
do {                                                                    \
	in_multihead_lock_assert(LCK_RW_ASSERT_HELD);                   \
	if (((inm) = (step).i_inm) != NULL)                             \
	        (step).i_inm = LIST_NEXT((step).i_inm, inm_link);       \
} while (0)

#define IN_FIRST_MULTI(step, inm)                                       \
	                                                                \
	                                                                \
do {                                                                    \
	in_multihead_lock_assert(LCK_RW_ASSERT_HELD);                   \
	(step).i_inm = LIST_FIRST(&in_multihead);                       \
	IN_NEXT_MULTI((step), (inm));                                   \
} while (0)

extern lck_mtx_t *inet_domain_mutex;
extern struct domain *inetdomain;

struct ip_moptions;
struct inpcb;

  
                                        
   
#define MCAST_PASS              0                 
#define MCAST_NOTGMEMBER        1                                            
#define MCAST_NOTSMEMBER        2                                      
#define MCAST_MUTED             3                         

  
                                 
   
struct in_ifextra {
	uint32_t                netsig_len;
	u_int8_t                netsig[IFNET_SIGNATURELEN];
	struct lltable          *ii_llt;                       
};
#define IN_IFEXTRA(_ifp)        ((struct in_ifextra *)(_ifp->if_inetdata))
#define LLTABLE(ifp)            ((IN_IFEXTRA(ifp) == NULL) ? NULL : IN_IFEXTRA(ifp)->ii_llt)

extern u_int32_t ipv4_ll_arp_aware;

extern void in_ifaddr_init(void);
extern int imo_multi_filter(const struct ip_moptions *,
    const struct ifnet *, const struct sockaddr_in *,
    const struct sockaddr_in *);
extern int imo_clone(struct inpcb *, struct inpcb *);
extern void inm_commit(struct in_multi *);
extern void inm_clear_recorded(struct in_multi *);
extern void inm_print(const struct in_multi *);
extern int inm_record_source(struct in_multi *inm, const in_addr_t);
extern void inm_release(struct in_multi *);
extern void in_multi_init(void);
extern struct in_multi *in_addmulti(struct in_addr *, struct ifnet *);
extern void in_delmulti(struct in_multi *);
extern int in_leavegroup(struct in_multi *, struct in_mfilter *);
extern int in_multi_detach(struct in_multi *);
extern void inm_addref(struct in_multi *, int);
extern void inm_remref(struct in_multi *, int);
extern void inm_purge(struct in_multi *);
extern uint8_t ims_get_mode(const struct in_multi *,
    const struct ip_msource *, uint8_t);
extern int in_control(struct socket *, u_long, caddr_t, struct ifnet *,
    struct proc *);
extern int in_inithead(void **, int);
extern void in_rtqdrain(void);
extern struct radix_node *in_validate(struct radix_node *);
extern void ip_input(struct mbuf *);
extern void ip_input_process_list(struct mbuf *);
extern int in_ifadown(struct ifaddr *ifa, int);
extern void in_ifscrub(struct ifnet *, struct in_ifaddr *, int);
extern u_int32_t inaddr_hashval(u_int32_t);
extern void in_purgeaddrs(struct ifnet *);
extern void gre_input(struct mbuf *, int);
extern void imf_leave(struct in_mfilter *);
extern void imf_purge(struct in_mfilter *);
extern int inp_join_group(struct inpcb *, struct sockopt *);
extern int inp_leave_group(struct inpcb *, struct sockopt *);
extern void in_multihead_lock_exclusive(void);
extern void in_multihead_lock_shared(void);
extern void in_multihead_lock_assert(int);
extern void in_multihead_lock_done(void);
#endif                         
                 
#include <netinet6/in6_var.h>
#endif                         
