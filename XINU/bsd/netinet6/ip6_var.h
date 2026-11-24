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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1993
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
 *	@(#)ip_var.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _NETINET6_IP6_VAR_H_
#define _NETINET6_IP6_VAR_H_
#include <sys/appleapiopts.h>

#ifdef BSD_KERNEL_PRIVATE
#include <kern/zalloc.h>
#include <net/ethernet.h>

struct ip6asfrag;
  
                                                 
                                                            
   
struct  ip6q {
	struct ip6asfrag *ip6q_down;
	struct ip6asfrag *ip6q_up;
	u_int32_t       ip6q_ident;
	u_int8_t        ip6q_nxt;
	u_int8_t        ip6q_ecn;
	u_int8_t        ip6q_ttl;
	struct in6_addr ip6q_src, ip6q_dst;
	struct ip6q     *ip6q_next;
	struct ip6q     *ip6q_prev;
	int             ip6q_unfrglen;                                  
	int             ip6q_nfrag;                         
	uint32_t        ip6q_csum_flags;                     
	uint32_t        ip6q_csum;                                  
	uint32_t        ip6q_flags;
#define IP6QF_DIRTY    0x00000001
};

struct  ip6_moptions {
	decl_lck_mtx_data(, im6o_lock);
	uint32_t im6o_refcnt;                          
	uint32_t im6o_debug;                                     
	struct  ifnet *im6o_multicast_ifp;                                  
	u_char  im6o_multicast_hlim;                                          
	u_char  im6o_multicast_loop;                                     
	u_short im6o_num_memberships;                                    
	u_short im6o_max_memberships;                                    
	struct  in6_multi **im6o_membership;                           
	struct  in6_mfilter *im6o_mfilters;                         
	void (*im6o_trace)                                                
	(struct ip6_moptions *, int);
};

#define IM6O_LOCK_ASSERT_HELD(_im6o)                                    \
	LCK_MTX_ASSERT(&(_im6o)->im6o_lock, LCK_MTX_ASSERT_OWNED)

#define IM6O_LOCK_ASSERT_NOTHELD(_im6o)                                 \
	LCK_MTX_ASSERT(&(_im6o)->im6o_lock, LCK_MTX_ASSERT_NOTOWNED)

#define IM6O_LOCK(_im6o)                                                \
	lck_mtx_lock(&(_im6o)->im6o_lock)

#define IM6O_LOCK_SPIN(_im6o)                                           \
	lck_mtx_lock_spin(&(_im6o)->im6o_lock)

#define IM6O_CONVERT_LOCK(_im6o) do {                                   \
	IM6O_LOCK_ASSERT_HELD(_im6o);                                   \
	lck_mtx_convert_spin(&(_im6o)->im6o_lock);                      \
} while (0)

#define IM6O_UNLOCK(_im6o)                                              \
	lck_mtx_unlock(&(_im6o)->im6o_lock)

#define IM6O_ADDREF(_im6o)                                              \
	im6o_addref(_im6o, 0)

#define IM6O_ADDREF_LOCKED(_im6o)                                       \
	im6o_addref(_im6o, 1)

#define IM6O_REMREF(_im6o)                                              \
	im6o_remref(_im6o)

struct ip6_exthdrs {
	struct mbuf *ip6e_ip6;
	struct mbuf *ip6e_hbh;
	struct mbuf *ip6e_dest1;
	struct mbuf *ip6e_rthdr;
	struct mbuf *ip6e_dest2;
	boolean_t merged;
};

  
                                       
   

                                 
struct  ip6po_rhinfo {
	struct  ip6_rthdr *ip6po_rhi_rthdr;                     
	struct  route_in6 ip6po_rhi_route;                           
};
#define ip6po_rthdr     ip6po_rhinfo.ip6po_rhi_rthdr
#define ip6po_route     ip6po_rhinfo.ip6po_rhi_route

                          
struct  ip6po_nhinfo {
	struct  sockaddr *ip6po_nhi_nexthop;
	struct  route_in6 ip6po_nhi_route;                           
};
#define ip6po_nexthop   ip6po_nhinfo.ip6po_nhi_nexthop
#define ip6po_nextroute ip6po_nhinfo.ip6po_nhi_route

struct  ip6_pktopts {
	struct  mbuf *ip6po_m;                                        
	int     ip6po_hlim;                                        

	                                     
	struct  in6_pktinfo *ip6po_pktinfo;

	                                  
	struct  ip6po_nhinfo ip6po_nhinfo;

	struct  ip6_hbh *ip6po_hbh;                                

	                                                          
	struct  ip6_dest *ip6po_dest1;

	                                  
	struct  ip6po_rhinfo ip6po_rhinfo;

	                                                         
	struct  ip6_dest *ip6po_dest2;

	int     ip6po_tclass;                      

	int     ip6po_minmtu;                                         
#define IP6PO_MINMTU_MCASTONLY  -1                                             
#define IP6PO_MINMTU_DISABLE     0                               
#define IP6PO_MINMTU_ALL         1                             

	                                                                 
	int     ip6po_prefer_tempaddr;

#define IP6PO_TEMPADDR_SYSTEM   -1                                
#define IP6PO_TEMPADDR_NOTPREFER 0                                   
#define IP6PO_TEMPADDR_PREFER    1                               

	int ip6po_flags;
#if 0                                                                       
#define IP6PO_REACHCONF 0x01                                                
#define IP6PO_MINMTU    0x02                                            
#endif
#define IP6PO_DONTFRAG          0x04                                          
#define IP6PO_USECOA            0x08                             
};

  
                                       
   
#endif                         

#define IP6S_SRCRULE_COUNT 16
#include <netinet6/scope6_var.h>

struct  ip6stat {
	u_quad_t ip6s_total;                                        
	u_quad_t ip6s_tooshort;                               
	u_quad_t ip6s_toosmall;                              
	u_quad_t ip6s_fragments;                                
	u_quad_t ip6s_fragdropped;                                             
	u_quad_t ip6s_fragtimeout;                               
	u_quad_t ip6s_fragoverflow;                                        
	u_quad_t ip6s_forward;                                 
	u_quad_t ip6s_cantforward;                                             
	u_quad_t ip6s_redirectsent;                                        
	u_quad_t ip6s_delivered;                                                
	u_quad_t ip6s_localout;                                              
	u_quad_t ip6s_odropped;                                               
	u_quad_t ip6s_reassembled;                                        
	u_quad_t ip6s_atmfrag_rcvd;                                    
	u_quad_t ip6s_fragmented;                                              
	u_quad_t ip6s_ofragments;                                     
	u_quad_t ip6s_cantfrag;                                                
	u_quad_t ip6s_badoptions;                                       
	u_quad_t ip6s_noroute;                                                 
	u_quad_t ip6s_badvers;                                
	u_quad_t ip6s_rawout;                                               
	u_quad_t ip6s_badscope;                          
	u_quad_t ip6s_notmember;                                             
	u_quad_t ip6s_nxthist[256];                              
	u_quad_t ip6s_m1;                             
	u_quad_t ip6s_m2m[32];                                
	u_quad_t ip6s_mext1;                              
	u_quad_t ip6s_mext2m;                                     
	u_quad_t ip6s_exthdrtoolong;                                    
	u_quad_t ip6s_nogif;                                    
	u_quad_t ip6s_toomanyhdr;                                              

	  
                                                              
              
    
	                                                  
	u_quad_t ip6s_sources_none;
	                                                                   
	u_quad_t ip6s_sources_sameif[SCOPE6_ID_MAX];
	                                                                     
	u_quad_t ip6s_sources_otherif[SCOPE6_ID_MAX];
	  
                                                           
                                   
    
	u_quad_t ip6s_sources_samescope[SCOPE6_ID_MAX];
	  
                                                              
                                   
    
	u_quad_t ip6s_sources_otherscope[SCOPE6_ID_MAX];
	                                                         
	u_quad_t ip6s_sources_deprecated[SCOPE6_ID_MAX];

	u_quad_t ip6s_forward_cachehit;
	u_quad_t ip6s_forward_cachemiss;

	                                                                    
	u_quad_t ip6s_sources_rule[IP6S_SRCRULE_COUNT];

	                                                                          
	u_quad_t ip6s_sources_skip_expensive_secondary_if;

	                                            
	u_quad_t ip6s_pktdropcntrl;

	                                     
	u_quad_t ip6s_adj;
	                                              
	u_quad_t ip6s_adj_hwcsum_clr;

	                                            
	u_quad_t ip6s_dad_collide;

	                        
	u_quad_t ip6s_dad_loopcount;

	                              
	u_quad_t ip6s_necp_policy_drop;

	                  
	u_quad_t ip6s_clat464_in_tooshort_drop;
	u_quad_t ip6s_clat464_in_nov6addr_drop;
	u_quad_t ip6s_clat464_in_nov4addr_drop;
	u_quad_t ip6s_clat464_in_v4synthfail_drop;
	u_quad_t ip6s_clat464_in_64transfail_drop;
	u_quad_t ip6s_clat464_in_64proto_transfail_drop;
	u_quad_t ip6s_clat464_in_64frag_transfail_drop;
	u_quad_t ip6s_clat464_in_invalpbuf_drop;
	u_quad_t ip6s_clat464_in_success;
	u_quad_t ip6s_clat464_in_drop;
	u_quad_t ip6s_clat464_in_v4_drop;

	u_quad_t ip6s_clat464_out_nov6addr_drop;
	u_quad_t ip6s_clat464_out_v6synthfail_drop;
	u_quad_t ip6s_clat464_out_46transfail_drop;
	u_quad_t ip6s_clat464_out_46proto_transfail_drop;
	u_quad_t ip6s_clat464_out_46frag_transfail_drop;
	u_quad_t ip6s_clat464_out_invalpbuf_drop;
	u_quad_t ip6s_clat464_out_success;
	u_quad_t ip6s_clat464_out_drop;

	u_quad_t ip6s_clat464_v6addr_conffail;
	u_quad_t ip6s_clat464_plat64_pfx_setfail;
	u_quad_t ip6s_clat464_plat64_pfx_getfail;

	u_quad_t ip6s_overlap_frag_drop;

	u_quad_t ip6s_rcv_if_weak_match;
	u_quad_t ip6s_rcv_if_no_match;
};

enum ip6s_sources_rule_index {
	IP6S_SRCRULE_0, IP6S_SRCRULE_1, IP6S_SRCRULE_2, IP6S_SRCRULE_3, IP6S_SRCRULE_4,
	IP6S_SRCRULE_5, IP6S_SRCRULE_6, IP6S_SRCRULE_7,
	IP6S_SRCRULE_7x, IP6S_SRCRULE_8
};

#ifdef BSD_KERNEL_PRIVATE
  
                            
  
                                                                    
                                                                         
                                    
   
struct ip6aux {
	u_int32_t ip6a_flags;
#define IP6A_HASEEN     0x01                                

#ifdef notyet
#define IP6A_SWAP       0x02                                                
#define IP6A_BRUID      0x04                                                  
#define IP6A_RTALERTSEEN 0x08                                

	                 
	struct in6_addr ip6a_careof;                                     
	struct in6_addr ip6a_home;                                    
	u_int16_t       ip6a_bruid;                               

	             
	u_int16_t ip6a_rtalert;                                   
#endif             

	                                                             
	u_char ip6a_ehsrc[ETHER_ADDR_LEN];
};

                                                  
#define IPV6_UNSPECSRC          0x01                                        
#define IPV6_FORWARDING         0x02                                    
#define IPV6_MINMTU             0x04                                            
#define IPV6_FLAG_NOSRCIFSEL    0x80                                        
#define IPV6_OUTARGS            0x100                                  

#ifdef BSD_KERNEL_PRIVATE
#define IP6_HDR_ALIGNED_P(_ip6) ((((uintptr_t)(_ip6)) & ((uintptr_t)3)) == 0)

  
                                                                          
                                                                          
                                           
   
#if defined(__i386__) || defined(__x86_64__)
#define IP6_HDR_STRICT_ALIGNMENT_CHECK(_ip6) do { } while (0)
#else                               
#define IP6_HDR_STRICT_ALIGNMENT_CHECK(_ip6) do {                       \
	if (!IP_HDR_ALIGNED_P(_ip6)) {                                  \
	        panic_plain("\n%s: Unaligned IPv6 header %p\n",         \
	            __func__, _ip6);                                    \
	}                                                               \
} while (0)
#endif                               
#endif                         

#include <net/flowadv.h>

  
                                                                   
   
struct ip6_out_args {
	unsigned int    ip6oa_boundif;                                
	struct flowadv  ip6oa_flowadv;                          
	u_int32_t       ip6oa_flags;                                  
#define IP6OAF_SELECT_SRCIF     0x00000001                                   
#define IP6OAF_BOUND_IF         0x00000002                                  
#define IP6OAF_BOUND_SRCADDR    0x00000004                                
#define IP6OAF_NO_CELLULAR      0x00000010                             
#define IP6OAF_NO_EXPENSIVE     0x00000020                               
#define IP6OAF_AWDL_UNRESTRICTED 0x00000040                          
#define IP6OAF_QOSMARKING_ALLOWED 0x00000080                                             
#define IP6OAF_INTCOPROC_ALLOWED 0x00000100                                               
#define IP6OAF_NO_LOW_POWER     0x00000200                          
#define IP6OAF_NO_CONSTRAINED   0x00000400                                 
#define IP6OAF_SKIP_PF          0x00000800                   
#define IP6OAF_DONT_FRAG        0x00001000                          
#define IP6OAF_REDO_QOSMARKING_POLICY   0x00002000                                          
	u_int32_t       ip6oa_retflags;                                       
#define IP6OARF_IFDENIED        0x00000001                                      
	int             ip6oa_sotc;                                                          
	int             ip6oa_netsvctype;
	int32_t         qos_marking_gencount;
};

extern struct ip6stat ip6stat;                  
extern int ip6_defhlim;                                
extern int ip6_defmcasthlim;                                     
extern int ip6_forwarding;                          
extern int ip6_gif_hlim;                                            
extern int ip6_use_deprecated;                                       
extern int ip6_rr_prune;                                       
                                                              
extern int ip6_mcast_pmtu;                                                
#define ip6_mapped_addr_on      (!ip6_v6only)
extern int ip6_v6only;

extern int ip6_neighborgcthresh;                                        
extern int ip6_maxifprefixes;                                              
extern int ip6_maxifdefrouters;                                        
extern int ip6_maxdynroutes;                                              
extern int ip6_sendredirects;                                           
extern int ip6_accept_rtadv;                    
extern int ip6_log_interval;
extern uint64_t ip6_log_time;
extern int ip6_hdrnestlimit;                                               
extern int ip6_dad_count;                                      

                                                
extern int ip6_only_allow_rfc4193_prefix;

extern int ip6_auto_flowlabel;
extern int ip6_auto_linklocal;

extern int ip6_anonportmin;                                         
extern int ip6_anonportmax;                                         
extern int ip6_lowportmin;                                         
extern int ip6_lowportmax;                                         

extern int ip6_use_tempaddr;                                          
extern int ip6_ula_use_tempaddr;                                             

                                                                           
extern int ip6_prefer_tempaddr;

                                                            
extern int ip6_use_defzone;

                                                                 
extern int ip6_cga_conflict_retries;
#define IPV6_CGA_CONFLICT_RETRIES_DEFAULT 3
#define IPV6_CGA_CONFLICT_RETRIES_MAX     10

extern struct pr_usrreqs rip6_usrreqs;
extern struct pr_usrreqs icmp6_dgram_usrreqs;

struct sockopt;
struct inpcb;
struct ip6_hdr;
struct in6_ifaddr;
struct ip6protosw;
struct domain;

extern int icmp6_ctloutput(struct socket *, struct sockopt *);
extern int icmp6_dgram_ctloutput(struct socket *, struct sockopt *);
extern int icmp6_dgram_send(struct socket *, int, struct mbuf *,
    struct sockaddr *, struct mbuf *, struct proc *);
extern int icmp6_dgram_attach(struct socket *, int, struct proc *);

extern void ip6_init(struct ip6protosw *, struct domain *);
extern void ip6_input(struct mbuf *);
extern void ip6_setsrcifaddr_info(struct mbuf *, uint32_t, struct in6_ifaddr *);
extern void ip6_setdstifaddr_info(struct mbuf *, uint32_t, struct in6_ifaddr *);
extern int ip6_getsrcifaddr_info(struct mbuf *, uint32_t *, uint32_t *);
extern int ip6_getdstifaddr_info(struct mbuf *, uint32_t *, uint32_t *);
extern void ip6_freepcbopts(struct ip6_pktopts *);
extern int ip6_unknown_opt(u_int8_t *, struct mbuf *, size_t);
extern char *ip6_get_prevhdr(struct mbuf *, int);
extern int ip6_nexthdr(struct mbuf *, int, int, int *);
extern int ip6_lasthdr(struct mbuf *, int, int, int *);
extern boolean_t ip6_pkt_has_ulp(struct mbuf *m);

extern void ip6_moptions_init(void);
extern struct ip6_moptions *ip6_allocmoptions(zalloc_flags_t);
extern void im6o_addref(struct ip6_moptions *, int);
extern void im6o_remref(struct ip6_moptions *);

extern struct ip6aux *ip6_addaux(struct mbuf *);
extern struct ip6aux *ip6_findaux(struct mbuf *);
extern void ip6_delaux(struct mbuf *);

extern int ip6_process_hopopts(struct mbuf *, u_int8_t *, int, u_int32_t *,
    u_int32_t *);
extern struct mbuf **ip6_savecontrol_v4(struct inpcb *, struct mbuf *,
    struct mbuf **, int *);
extern int ip6_savecontrol(struct inpcb *, struct mbuf *, struct mbuf **);
extern struct mbuf *ip6_forward(struct mbuf *, struct route_in6 *, int);
extern void ip6_notify_pmtu(struct inpcb *, struct sockaddr_in6 *, u_int32_t *);
extern void ip6_mloopback(struct ifnet *, struct ifnet *, struct mbuf *,
    struct sockaddr_in6 *, uint32_t, int32_t);
extern int ip6_output(struct mbuf *, struct ip6_pktopts *, struct route_in6 *,
    int, struct ip6_moptions *, struct ifnet **, struct ip6_out_args *);
extern int ip6_output_list(struct mbuf *, int, struct ip6_pktopts *,
    struct route_in6 *, int, struct ip6_moptions *, struct ifnet **,
    struct ip6_out_args *);
extern int ip6_ctloutput(struct socket *, struct sockopt *);
extern int ip6_raw_ctloutput(struct socket *, struct sockopt *);
extern void ip6_initpktopts(struct ip6_pktopts *);
extern int ip6_setpktoptions(struct mbuf *, struct ip6_pktopts *, int, int);
extern void ip6_clearpktopts(struct ip6_pktopts *, int);
extern struct ip6_pktopts *ip6_copypktopts(struct ip6_pktopts *, int);
extern int ip6_optlen(struct inpcb *);
extern void ip6_drain(void);
extern int ip6_do_fragmentation(struct mbuf **, uint32_t, struct ifnet *, uint32_t,
    struct ip6_hdr *, uint8_t *, uint32_t, int, uint32_t);

extern int route6_input(struct mbuf **, int *, int);

extern void frag6_init(void);
extern int frag6_input(struct mbuf **, int *, int);
extern void frag6_drain(void);

extern int rip6_input(struct mbuf **, int *, int);
extern void rip6_ctlinput(int, struct sockaddr *, void *, struct ifnet *);
extern int rip6_ctloutput(struct socket *so, struct sockopt *sopt);
extern int rip6_output(struct mbuf *, struct socket *, struct sockaddr_in6 *,
    struct mbuf *, int);

extern int dest6_input(struct mbuf **, int *, int);
  
                                      
   
#define IPV6_SRCSEL_HINT_PREFER_TMPADDR         0x00000001

extern struct ifaddr * in6_selectsrc_core_ifa(struct sockaddr_in6 *, struct ifnet *, int);
extern struct in6_addr * in6_selectsrc_core(struct sockaddr_in6 *,
    uint32_t, struct ifnet *, int,
    struct in6_addr *, struct ifnet **, int *, struct ifaddr **);
extern struct in6_addr *in6_selectsrc(struct sockaddr_in6 *,
    struct ip6_pktopts *, struct inpcb *, struct route_in6 *,
    struct ifnet **, struct in6_addr *, unsigned int, int *);
extern struct in6_addrpolicy *in6_addrsel_lookup_policy(struct sockaddr_in6 *);
extern int in6_selectroute(struct sockaddr_in6 *, struct sockaddr_in6 *,
    struct ip6_pktopts *, struct ip6_moptions *, struct in6_ifaddr **,
    struct route_in6 *, struct ifnet **, struct rtentry **, int,
    struct ip6_out_args *);
extern int ip6_setpktopts(struct mbuf *control, struct ip6_pktopts *opt,
    struct ip6_pktopts *stickyopt, int uproto);
extern u_int32_t ip6_randomid(void);
extern u_int32_t ip6_randomflowlabel(void);
#endif                         
#endif                            
