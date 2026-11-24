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
 *	@(#)ip_var.h	8.2 (Berkeley) 1/9/95
 */
  
                                                                      
                                                                          
                                                                        
               
   

#ifndef _NETINET_IP_VAR_H_
#define _NETINET_IP_VAR_H_
#include <sys/appleapiopts.h>

  
                                                            
   
struct ipovly {
	u_char  ih_x1[9];                             
	u_char  ih_pr;                                
	u_short ih_len;                                      
	struct  in_addr ih_src;                                      
	struct  in_addr ih_dst;                                           
};

#ifdef BSD_KERNEL_PRIVATE
  
                                                
                                                            
                                                            
                                        
   
struct ipq {
	TAILQ_ENTRY(ipq) ipq_list;                                  
	struct mbuf *ipq_frags;                                         
	u_char  ipq_ttl;                                              
	u_char  ipq_p;                                                 
	u_short ipq_id;                                                 
	struct  in_addr ipq_src, ipq_dst;
	u_int32_t       ipq_nfrags;                                 
	uint32_t ipq_csum_flags;                            
	uint32_t ipq_csum;                                          
};

  
                                               
                                                      
                                                         
               
   
#endif                         
#define MAX_IPOPTLEN    40
#ifdef BSD_KERNEL_PRIVATE
struct ipoption {
	struct  in_addr ipopt_dst;                                          
	char    ipopt_list[MAX_IPOPTLEN];                           
};

  
                                              
                                                            
   
struct ip_moptions {
	decl_lck_mtx_data(, imo_lock);
	uint32_t imo_refcnt;                           
	uint32_t imo_debug;                                      
	struct  ifnet *imo_multicast_ifp;                                  
	u_char  imo_multicast_ttl;                                       
	u_char  imo_multicast_loop;                                      
	u_short imo_num_memberships;                                     
	u_short imo_max_memberships;                                     
	struct  in_multi **imo_membership;                             
	struct  in_mfilter *imo_mfilters;                           
	u_int32_t imo_multicast_vif;                                     
	struct  in_addr imo_multicast_addr;                                   
	void (*imo_trace)                                                 
	(struct ip_moptions *, int);
};

#define IMO_LOCK_ASSERT_HELD(_imo)                                      \
	LCK_MTX_ASSERT(&(_imo)->imo_lock, LCK_MTX_ASSERT_OWNED)

#define IMO_LOCK_ASSERT_NOTHELD(_imo)                                   \
	LCK_MTX_ASSERT(&(_imo)->imo_lock, LCK_MTX_ASSERT_NOTOWNED)

#define IMO_LOCK(_imo)                                                  \
	lck_mtx_lock(&(_imo)->imo_lock)

#define IMO_LOCK_SPIN(_imo)                                             \
	lck_mtx_lock_spin(&(_imo)->imo_lock)

#define IMO_CONVERT_LOCK(_imo) do {                                     \
	IMO_LOCK_ASSERT_HELD(_imo);                                     \
	lck_mtx_convert_spin(&(_imo)->imo_lock);                        \
} while (0)

#define IMO_UNLOCK(_imo)                                                \
	lck_mtx_unlock(&(_imo)->imo_lock)

#define IMO_ADDREF(_imo)                                                \
	imo_addref(_imo, 0)

#define IMO_ADDREF_LOCKED(_imo)                                         \
	imo_addref(_imo, 1)

#define IMO_REMREF(_imo)                                                \
	imo_remref(_imo)

                                     
struct ip_fwd_tag {
	struct sockaddr_in *next_hop;                 
};
#endif                         

struct  ipstat {
	u_int32_t ips_total;                                        
	u_int32_t ips_badsum;                             
	u_int32_t ips_tooshort;                               
	u_int32_t ips_toosmall;                              
	u_int32_t ips_badhlen;                                            
	u_int32_t ips_badlen;                                             
	u_int32_t ips_fragments;                                
	u_int32_t ips_fragdropped;                                              
	u_int32_t ips_fragtimeout;                               
	u_int32_t ips_forward;                                 
	u_int32_t ips_fastforward;                                  
	u_int32_t ips_cantforward;                                             
	u_int32_t ips_redirectsent;                                        
	u_int32_t ips_noproto;                                               
	u_int32_t ips_delivered;                                                
	u_int32_t ips_localout;                                              
	u_int32_t ips_odropped;                                               
	u_int32_t ips_reassembled;                                        
	u_int32_t ips_fragmented;                                              
	u_int32_t ips_ofragments;                                     
	u_int32_t ips_cantfrag;                                                
	u_int32_t ips_badoptions;                                       
	u_int32_t ips_noroute;                                                 
	u_int32_t ips_badvers;                               
	u_int32_t ips_rawout;                                               
	u_int32_t ips_toolong;                                              
	u_int32_t ips_notmember;                                              
	u_int32_t ips_nogif;                                    
	u_int32_t ips_badaddr;                                         
	u_int32_t ips_pktdropcntrl;                                             
	u_int32_t ips_rcv_swcsum;                                              
	u_int32_t ips_rcv_swcsum_bytes;                                      
	u_int32_t ips_snd_swcsum;                                               
	u_int32_t ips_snd_swcsum_bytes;                                       
	u_int32_t ips_adj;                                                  
	u_int32_t ips_adj_hwcsum_clr;                                     
	u_int32_t ips_rxc_collisions;                               
	u_int32_t ips_rxc_chained;                     
	u_int32_t ips_rxc_notchain;                               
	u_int32_t ips_rxc_chainsz_gt2;                                    
	u_int32_t ips_rxc_chainsz_gt4;                                    
	u_int32_t ips_rxc_notlist;                                          
	u_int32_t ips_raw_sappend_fail;                         
	u_int32_t ips_necp_policy_drop;                               
	u_int32_t ips_rcv_if_weak_match;                                                                            
	u_int32_t ips_rcv_if_no_match;                                                                      
};

struct ip_linklocal_stat {
	u_int32_t       iplls_in_total;
	u_int32_t       iplls_in_badttl;
	u_int32_t       iplls_out_total;
	u_int32_t       iplls_out_badttl;
};

#ifdef KERNEL_PRIVATE
                                          
struct ip_out_args;
struct ip_moptions;
#endif                     

#ifdef BSD_KERNEL_PRIVATE
                                                 
#define IP_FORWARDING   0x1                                           
#define IP_RAWOUTPUT    0x2                                       
#define IP_NOIPSEC      0x4                                      
#define IP_ROUTETOIF    SO_DONTROUTE                                        
#define IP_ALLOWBROADCAST SO_BROADCAST                                        
#define IP_OUTARGS      0x100                                          

#define IP_HDR_ALIGNED_P(_ip)   ((((uintptr_t)(_ip)) & ((uintptr_t)3)) == 0)
#define IP_OFF_IS_ATOMIC(_ip_off) ((_ip_off & (IP_DF | IP_MF | IP_OFFMASK)) == IP_DF)

  
                                                                          
                                                                          
                                           
   
#if defined(__i386__) || defined(__x86_64__)
#define IP_HDR_STRICT_ALIGNMENT_CHECK(_ip) do { } while (0)
#else                               
#define IP_HDR_STRICT_ALIGNMENT_CHECK(_ip) do {                         \
	if (!IP_HDR_ALIGNED_P(_ip)) {                                   \
	        panic_plain("\n%s: Unaligned IP header %p\n",           \
	            __func__, _ip);                                     \
	}                                                               \
} while (0)
#endif                               

struct ip;
struct inpcb;
struct route;
struct sockopt;

#include <kern/zalloc.h>
#include <net/flowadv.h>

  
                                                                
  
                                                                        
                                                                
   
struct ip_out_args {
	unsigned int    ipoa_boundif;                                
	struct flowadv  ipoa_flowadv;                           
	u_int32_t       ipoa_flags;                                         
#define IPOAF_SELECT_SRCIF      0x00000001                                   
#define IPOAF_BOUND_IF          0x00000002                                  
#define IPOAF_BOUND_SRCADDR     0x00000004                                
#define IPOAF_NO_CELLULAR       0x00000010                             
#define IPOAF_NO_EXPENSIVE      0x00000020                              
#define IPOAF_AWDL_UNRESTRICTED 0x00000040                      
                                                               
#define IPOAF_QOSMARKING_ALLOWED        0x00000080                                               
#define IPOAF_NO_CONSTRAINED    0x00000100                                 
#define IPOAF_REDO_QOSMARKING_POLICY    0x00000200                                          
	u_int32_t       ipoa_retflags;                                       
#define IPOARF_IFDENIED 0x00000001                                      
	int             ipoa_sotc;                                                   
	int             ipoa_netsvctype;                           
	int32_t         qos_marking_gencount;
};

extern struct ipstat ipstat;
extern int ip_use_randomid;
extern u_short ip_id;                                               
extern int ip_defttl;                                       
extern int ipforwarding;                                   
extern int rfc6864;
extern struct protosw *ip_protox[];
extern struct pr_usrreqs rip_usrreqs;

extern void ip_moptions_init(void);
extern struct ip_moptions *ip_allocmoptions(zalloc_flags_t);
extern int inp_getmoptions(struct inpcb *, struct sockopt *);
extern int inp_setmoptions(struct inpcb *, struct sockopt *);
extern void imo_addref(struct ip_moptions *, int);
extern void imo_remref(struct ip_moptions *);

struct protosw;
struct domain;

extern int ip_checkrouteralert(struct mbuf *);
extern int ip_ctloutput(struct socket *, struct sockopt *sopt);
extern void ip_drain(void);
extern void ip_init(struct protosw *, struct domain *);
extern int ip_output(struct mbuf *, struct mbuf *, struct route *, int,
    struct ip_moptions *, struct ip_out_args *);
extern int ip_output_list(struct mbuf *, int, struct mbuf *, struct route *,
    int, struct ip_moptions *, struct ip_out_args *);
extern void ip_output_checksum(struct ifnet *, struct mbuf *, int, int,
    uint32_t *);
extern struct in_ifaddr *ip_rtaddr(struct in_addr);
extern int ip_savecontrol(struct inpcb *, struct mbuf **, struct ip *,
    struct mbuf *);
extern struct mbuf *ip_srcroute(void);
extern void  ip_stripoptions(struct mbuf *);
extern void ip_initid(void);
extern u_int16_t ip_randomid(void);
extern int ip_fragment(struct mbuf *, struct ifnet *, uint32_t, int);

extern void ip_setsrcifaddr_info(struct mbuf *, uint32_t, struct in_ifaddr *);
extern void ip_setdstifaddr_info(struct mbuf *, uint32_t, struct in_ifaddr *);
extern int ip_getsrcifaddr_info(struct mbuf *, uint32_t *, uint32_t *);
extern int ip_getdstifaddr_info(struct mbuf *, uint32_t *, uint32_t *);

extern int rip_ctloutput(struct socket *, struct sockopt *);
extern void rip_ctlinput(int, struct sockaddr *, void *, struct ifnet *);
extern void rip_init(struct protosw *, struct domain *);
extern void rip_input(struct mbuf *, int);
extern int rip_output(struct mbuf *, struct socket *, u_int32_t, struct mbuf *);
extern int rip_unlock(struct socket *, int, void *);
extern int rip_send(struct socket *, int, struct mbuf *, struct sockaddr *,
    struct mbuf *, struct proc *);

extern void tcp_in_cksum_stats(u_int32_t);
extern void tcp_out_cksum_stats(u_int32_t);

extern void udp_in_cksum_stats(u_int32_t);
extern void udp_out_cksum_stats(u_int32_t);

extern void tcp_in6_cksum_stats(u_int32_t);
extern void tcp_out6_cksum_stats(u_int32_t);

extern void udp_in6_cksum_stats(u_int32_t);
extern void udp_out6_cksum_stats(u_int32_t);
#endif                         
#ifdef KERNEL_PRIVATE
                  
extern int ip_gre_output(struct mbuf *);
typedef struct mbuf *(*gre_input_func_t)(struct mbuf *, int, int);
extern int ip_gre_register_input(gre_input_func_t);
#endif                     
#endif                          
