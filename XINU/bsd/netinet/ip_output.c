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
 * Copyright (c) 1982, 1986, 1988, 1990, 1993
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
 *	@(#)ip_output.c	8.3 (Berkeley) 1/21/94
 */
  
                                                                      
                                                                          
                                                                        
               
   

#define _IP_VHL

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <kern/locks.h>
#include <sys/sysctl.h>
#include <sys/mcache.h>
#include <sys/kdebug.h>

#include <machine/endian.h>
#include <pexpert/pexpert.h>
#include <mach/sdt.h>

#include <libkern/OSAtomic.h>
#include <libkern/OSByteOrder.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/ntstat.h>
#include <net/net_osdep.h>
#include <net/dlil.h>
#include <net/net_perf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/kpi_ipfilter_var.h>
#include <netinet/in_tclass.h>
#include <netinet/udp.h>

#include <netinet6/nd6.h>

#define DBG_LAYER_BEG           NETDBG_CODE(DBG_NETIP, 1)
#define DBG_LAYER_END           NETDBG_CODE(DBG_NETIP, 3)
#define DBG_FNC_IP_OUTPUT       NETDBG_CODE(DBG_NETIP, (1 << 8) | 1)
#define DBG_FNC_IPSEC4_OUTPUT   NETDBG_CODE(DBG_NETIP, (2 << 8) | 1)

#if IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#if IPSEC_DEBUG
#include <netkey/key_debug.h>
#else
#define KEYDEBUG(lev, arg)
#endif
#endif            

#if NECP
#include <net/necp.h>
#endif           


#if DUMMYNET
#include <netinet/ip_dummynet.h>
#endif

#if PF
#include <net/pfvar.h>
#endif         


u_short ip_id;

static int sysctl_reset_ip_output_stats SYSCTL_HANDLER_ARGS;
static int sysctl_ip_output_measure_bins SYSCTL_HANDLER_ARGS;
static int sysctl_ip_output_getperf SYSCTL_HANDLER_ARGS;
static void ip_out_cksum_stats(int, u_int32_t);
static struct mbuf *ip_insertoptions(struct mbuf *, struct mbuf *, int *);
static int ip_optcopy(struct ip *, struct ip *);
static int ip_pcbopts(int, struct mbuf **, struct mbuf *);
static void imo_trace(struct ip_moptions *, int);
static void ip_mloopback(struct ifnet *, struct ifnet *, struct mbuf *,
    struct sockaddr_in *, int);
static struct ifaddr *in_selectsrcif(struct ip *, struct route *, unsigned int);

extern struct ip_linklocal_stat ip_linklocal_stat;

                            
#if IPSEC
extern int ipsec_bypass;
#endif

static int ip_maxchainsent = 0;
SYSCTL_INT(_net_inet_ip, OID_AUTO, maxchainsent,
    CTLFLAG_RW | CTLFLAG_LOCKED, &ip_maxchainsent, 0,
    "use dlil_output_list");
#if DEBUG
static int forge_ce = 0;
SYSCTL_INT(_net_inet_ip, OID_AUTO, forge_ce,
    CTLFLAG_RW | CTLFLAG_LOCKED, &forge_ce, 0,
    "Forge ECN CE");
#endif            

static int ip_select_srcif_debug = 0;
SYSCTL_INT(_net_inet_ip, OID_AUTO, select_srcif_debug,
    CTLFLAG_RW | CTLFLAG_LOCKED, &ip_select_srcif_debug, 0,
    "log source interface selection debug info");

static int ip_output_measure = 0;
SYSCTL_PROC(_net_inet_ip, OID_AUTO, output_perf,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &ip_output_measure, 0, sysctl_reset_ip_output_stats, "I",
    "Do time measurement");

static uint64_t ip_output_measure_bins = 0;
SYSCTL_PROC(_net_inet_ip, OID_AUTO, output_perf_bins,
    CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED, &ip_output_measure_bins, 0,
    sysctl_ip_output_measure_bins, "I",
    "bins for chaining performance data histogram");

static net_perf_t net_perf;
SYSCTL_PROC(_net_inet_ip, OID_AUTO, output_perf_data,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED,
    0, 0, sysctl_ip_output_getperf, "S,net_perf",
    "IP output performance data (struct net_perf, net/net_perf.h)");

__private_extern__ int rfc6864 = 1;
SYSCTL_INT(_net_inet_ip, OID_AUTO, rfc6864, CTLFLAG_RW | CTLFLAG_LOCKED,
    &rfc6864, 0, "updated ip id field behavior");

#define IMO_TRACE_HIST_SIZE     32                                 

             
__private_extern__ unsigned int imo_trace_hist_size = IMO_TRACE_HIST_SIZE;

struct ip_moptions_dbg {
	struct ip_moptions      imo;                                     
	u_int16_t               imo_refhold_cnt;                             
	u_int16_t               imo_refrele_cnt;                             
	  
                           
    
	ctrace_t                imo_alloc;
	ctrace_t                imo_free;
	  
                                                        
    
	ctrace_t                imo_refhold[IMO_TRACE_HIST_SIZE];
	ctrace_t                imo_refrele[IMO_TRACE_HIST_SIZE];
};

#if DEBUG
static unsigned int imo_debug = 1;                               
#else
static unsigned int imo_debug;                                    
#endif             
static struct zone *imo_zone;                                     
#define IMO_ZONE_NAME           "ip_moptions"                  

  
                                                                
                                                     
                                                      
                                               
   
int
ip_output(struct mbuf *m0, struct mbuf *opt, struct route *ro, int flags,
    struct ip_moptions *imo, struct ip_out_args *ipoa)
{
	return ip_output_list(m0, 0, opt, ro, flags, imo, ipoa);
}

  
                                                                
                                                     
                                                      
                                               
  
                                                                          
                                                                      
                                 
  
                                                                         
                                             
   
int
ip_output_list(struct mbuf *m0, int packetchain, struct mbuf *opt,
    struct route *ro, int flags, struct ip_moptions *imo,
    struct ip_out_args *ipoa)
{
	struct ip *ip;
	struct ifnet *ifp = NULL;                                 
	struct mbuf *m = m0, *prevnxt = NULL, **mppn = &prevnxt;
	int hlen = sizeof(struct ip);
	int len = 0, error = 0;
	struct sockaddr_in *dst = NULL;
	struct in_ifaddr *ia = NULL, *src_ia = NULL;
	struct in_addr pkt_dst;
	struct ipf_pktopts *ippo = NULL;
	ipfilter_t inject_filter_ref = NULL;
	struct mbuf *packetlist;
	uint32_t sw_csum, pktcnt = 0, scnt = 0, bytecnt = 0;
	uint32_t packets_processed = 0;
	unsigned int ifscope = IFSCOPE_NONE;
	struct flowadv *adv = NULL;
	struct timeval start_tv;
#if IPSEC
	struct socket *so = NULL;
	struct secpolicy *sp = NULL;
#endif            
#if NECP
	necp_kernel_policy_result necp_result = 0;
	necp_kernel_policy_result_parameter necp_result_parameter;
	necp_kernel_policy_id necp_matched_policy_id = 0;
#endif           
#if DUMMYNET
	struct m_tag *tag;
	struct ip_out_args saved_ipoa;
	struct sockaddr_in dst_buf;
#endif               
	struct {
#if IPSEC
		struct ipsec_output_state ipsec_state;
#endif            
#if NECP
		struct route necp_route;
#endif           
#if DUMMYNET
		struct ip_fw_args args;
		struct route saved_route;
#endif               
		struct ipf_pktopts ipf_pktopts;
	} ipobz;
#define ipsec_state     ipobz.ipsec_state
#define necp_route      ipobz.necp_route
#define args            ipobz.args
#define sro_fwd         ipobz.sro_fwd
#define saved_route     ipobz.saved_route
#define ipf_pktopts     ipobz.ipf_pktopts
	union {
		struct {
			boolean_t select_srcif : 1;                   
			boolean_t srcbound : 1;                       
			boolean_t nocell : 1;                         
			boolean_t isbroadcast : 1;
			boolean_t didfilter : 1;
			boolean_t noexpensive : 1;                    
			boolean_t noconstrained : 1;                    
			boolean_t awdl_unrestricted : 1;                      
		};
		uint32_t raw;
	} ipobf = { .raw = 0 };

	int interface_mtu = 0;

  
                                                      
                                                                   
   
#define IP_CHECK_RESTRICTIONS(_ifp, _ipobf)                             \
	(((_ipobf).nocell && IFNET_IS_CELLULAR(_ifp)) ||                \
	 ((_ipobf).noexpensive && IFNET_IS_EXPENSIVE(_ifp)) ||          \
	 ((_ipobf).noconstrained && IFNET_IS_CONSTRAINED(_ifp)) ||      \
	  (IFNET_IS_INTCOPROC(_ifp)) ||                                 \
	 (!(_ipobf).awdl_unrestricted && IFNET_IS_AWDL_RESTRICTED(_ifp)))

	if (ip_output_measure) {
		net_perf_start_time(&net_perf, &start_tv);
	}
	KERNEL_DEBUG(DBG_FNC_IP_OUTPUT | DBG_FUNC_START, 0, 0, 0, 0, 0);

	VERIFY(m0->m_flags & M_PKTHDR);
	packetlist = m0;

	                                                                    
	bzero(&ipobz, sizeof(ipobz));
	ippo = &ipf_pktopts;

#if DUMMYNET
	if (SLIST_EMPTY(&m0->m_pkthdr.tags)) {
		goto ipfw_tags_done;
	}

	                                                 
	if ((tag = m_tag_locate(m0, KERNEL_MODULE_TAG_ID,
	    KERNEL_TAG_TYPE_DUMMYNET, NULL)) != NULL) {
		struct dn_pkt_tag       *dn_tag;

		dn_tag = (struct dn_pkt_tag *)(tag + 1);
		args.fwa_pf_rule = dn_tag->dn_pf_rule;
		opt = NULL;
		saved_route = dn_tag->dn_ro;
		ro = &saved_route;

		imo = NULL;
		bcopy(&dn_tag->dn_dst, &dst_buf, sizeof(dst_buf));
		dst = &dst_buf;
		ifp = dn_tag->dn_ifp;
		flags = dn_tag->dn_flags;
		if ((dn_tag->dn_flags & IP_OUTARGS)) {
			saved_ipoa = dn_tag->dn_ipoa;
			ipoa = &saved_ipoa;
		}

		m_tag_delete(m0, tag);
	}
ipfw_tags_done:
#endif               

	m = m0;
	m->m_pkthdr.pkt_flags &= ~(PKTF_LOOP | PKTF_IFAINFO);

#if IPSEC
	if (ipsec_bypass == 0 && !(flags & IP_NOIPSEC)) {
		                                                              
		if ((flags & IP_OUTARGS) && (ipoa != NULL) &&
		    (ipoa->ipoa_flags & IPOAF_BOUND_IF) &&
		    ipoa->ipoa_boundif != IFSCOPE_NONE) {
			if (ipsec4_getpolicybyinterface(m, IPSEC_DIR_OUTBOUND,
			    &flags, ipoa, &sp) != 0) {
				goto bad;
			}
		}
	}
#endif            

	VERIFY(ro != NULL);

	if (flags & IP_OUTARGS) {
		  
                                                            
                                                      
     
		if ((ipobf.select_srcif = (!(flags & IP_FORWARDING) &&
		    (ipoa->ipoa_flags & IPOAF_SELECT_SRCIF)))) {
			ipf_pktopts.ippo_flags |= IPPOF_SELECT_SRCIF;
		}

		if ((ipoa->ipoa_flags & IPOAF_BOUND_IF) &&
		    ipoa->ipoa_boundif != IFSCOPE_NONE) {
			ifscope = ipoa->ipoa_boundif;
			ipf_pktopts.ippo_flags |=
			    (IPPOF_BOUND_IF | (ifscope << IPPOF_SHIFT_IFSCOPE));
		}

		                                               
		ipobf.srcbound = !!(ipoa->ipoa_flags & IPOAF_BOUND_SRCADDR);
		if (ipobf.srcbound) {
			ipf_pktopts.ippo_flags |= IPPOF_BOUND_SRCADDR;
		}
	} else {
		ipobf.select_srcif = FALSE;
		ipobf.srcbound = FALSE;
		ifscope = IFSCOPE_NONE;
		if (flags & IP_OUTARGS) {
			ipoa->ipoa_boundif = IFSCOPE_NONE;
			ipoa->ipoa_flags &= ~(IPOAF_SELECT_SRCIF |
			    IPOAF_BOUND_IF | IPOAF_BOUND_SRCADDR);
		}
	}

	if (flags & IP_OUTARGS) {
		if (ipoa->ipoa_flags & IPOAF_NO_CELLULAR) {
			ipobf.nocell = TRUE;
			ipf_pktopts.ippo_flags |= IPPOF_NO_IFT_CELLULAR;
		}
		if (ipoa->ipoa_flags & IPOAF_NO_EXPENSIVE) {
			ipobf.noexpensive = TRUE;
			ipf_pktopts.ippo_flags |= IPPOF_NO_IFF_EXPENSIVE;
		}
		if (ipoa->ipoa_flags & IPOAF_NO_CONSTRAINED) {
			ipobf.noconstrained = TRUE;
			ipf_pktopts.ippo_flags |= IPPOF_NO_IFF_CONSTRAINED;
		}
		if (ipoa->ipoa_flags & IPOAF_AWDL_UNRESTRICTED) {
			ipobf.awdl_unrestricted = TRUE;
		}
		adv = &ipoa->ipoa_flowadv;
		adv->code = FADV_SUCCESS;
		ipoa->ipoa_retflags = 0;
	}

#if IPSEC
	if (ipsec_bypass == 0 && !(flags & IP_NOIPSEC)) {
		so = ipsec_getsocket(m);
		if (so != NULL) {
			(void) ipsec_setsocket(m, NULL);
		}
	}
#endif            

#if DUMMYNET
	if (args.fwa_pf_rule != NULL) {
		                             
		ip = mtod(m, struct ip *);
		hlen = IP_VHL_HL(ip->ip_vhl) << 2;
		pkt_dst = ip->ip_dst;
		if (ro->ro_rt != NULL) {
			RT_LOCK_SPIN(ro->ro_rt);
			ia = (struct in_ifaddr *)ro->ro_rt->rt_ifa;
			if (ia) {
				                            
				RT_CONVERT_LOCK(ro->ro_rt);
				IFA_ADDREF(&ia->ia_ifa);
			}
			RT_UNLOCK(ro->ro_rt);
		}

		if (args.fwa_pf_rule != NULL) {
			goto sendit;
		}
	}
#endif               

loopit:
	packets_processed++;
	ipobf.isbroadcast = FALSE;
	ipobf.didfilter = FALSE;

	VERIFY(m->m_flags & M_PKTHDR);
	  
                                                              
    
	if (!SLIST_EMPTY(&m->m_pkthdr.tags)) {
		inject_filter_ref = ipf_get_inject_filter(m);
	} else {
		inject_filter_ref = NULL;
	}

	if (opt) {
		m = ip_insertoptions(m, opt, &len);
		hlen = len;
		                      
		if (m != m0) {
			if (m0 == packetlist) {
				packetlist = m;
			}
			m0 = m;
		}
	}
	ip = mtod(m, struct ip *);

	pkt_dst = ip->ip_dst;

	  
                                                               
                                
    
	if (IN_ZERONET(ntohl(pkt_dst.s_addr))) {
		error = EHOSTUNREACH;
		goto bad;
	}

	  
                      
    
	if (!(flags & (IP_FORWARDING | IP_RAWOUTPUT))) {
		ip->ip_vhl = IP_MAKE_VHL(IPVERSION, hlen >> 2);
		ip->ip_off &= IP_DF;
		if (rfc6864 && IP_OFF_IS_ATOMIC(ip->ip_off)) {
			                                                                 
			ip->ip_id = 0;
		} else {
			ip->ip_id = ip_randomid();
		}
		OSAddAtomic(1, &ipstat.ips_localout);
	} else {
		hlen = IP_VHL_HL(ip->ip_vhl) << 2;
	}

#if DEBUG
	                                                      
	if (forge_ce != 0 &&
	    ((ip->ip_tos & IPTOS_ECN_MASK) == IPTOS_ECN_ECT1 ||
	    (ip->ip_tos & IPTOS_ECN_MASK) == IPTOS_ECN_ECT0)) {
		ip->ip_tos = (ip->ip_tos & ~IPTOS_ECN_MASK) | IPTOS_ECN_CE;
		forge_ce--;
	}
#endif            

	KERNEL_DEBUG(DBG_LAYER_BEG, ip->ip_dst.s_addr, ip->ip_src.s_addr,
	    ip->ip_p, ip->ip_off, ip->ip_len);

	dst = SIN(&ro->ro_dst);

	  
                               
                                            
                                                    
                                                                    
                    
    

	if (ro->ro_rt != NULL) {
		if (ROUTE_UNUSABLE(ro) && ip->ip_src.s_addr != INADDR_ANY &&
		    !(flags & (IP_ROUTETOIF | IP_FORWARDING))) {
			src_ia = ifa_foraddr(ip->ip_src.s_addr);
			if (src_ia == NULL) {
				error = EADDRNOTAVAIL;
				goto bad;
			}
			IFA_REMREF(&src_ia->ia_ifa);
			src_ia = NULL;
		}
		  
                                                          
                                                       
                                                        
                                            
     
		if (ROUTE_UNUSABLE(ro) || dst->sin_family != AF_INET ||
		    dst->sin_addr.s_addr != pkt_dst.s_addr) {
			ROUTE_RELEASE(ro);
		}

		  
                                                          
                                                         
                     
     
		if (!ipobf.select_srcif && ro->ro_rt != NULL &&
		    RT_GENID_OUTOFSYNC(ro->ro_rt)) {
			RT_GENID_SYNC(ro->ro_rt);
		}
	}
	if (ro->ro_rt == NULL) {
		bzero(dst, sizeof(*dst));
		dst->sin_family = AF_INET;
		dst->sin_len = sizeof(*dst);
		dst->sin_addr = pkt_dst;
	}
	  
                                 
                                 
    
	if (flags & IP_ROUTETOIF) {
		if (ia != NULL) {
			IFA_REMREF(&ia->ia_ifa);
		}
		if ((ia = ifatoia(ifa_ifwithdstaddr(sintosa(dst)))) == NULL) {
			ia = ifatoia(ifa_ifwithnet(sintosa(dst)));
			if (ia == NULL) {
				OSAddAtomic(1, &ipstat.ips_noroute);
				error = ENETUNREACH;
				                                          
				goto bad;
			}
		}
		ifp = ia->ia_ifp;
		ip->ip_ttl = 1;
		ipobf.isbroadcast = in_broadcast(dst->sin_addr, ifp);
		  
                                                      
                                                            
     
		if ((ifp->if_flags & IFF_LOOPBACK) &&
		    !IN_MULTICAST(ntohl(pkt_dst.s_addr))) {
			m->m_pkthdr.rcvif = ifp;
			ip_setsrcifaddr_info(m, ifp->if_index, NULL);
			ip_setdstifaddr_info(m, ifp->if_index, NULL);
		}
	} else if (IN_MULTICAST(ntohl(pkt_dst.s_addr)) &&
	    imo != NULL && (ifp = imo->imo_multicast_ifp) != NULL) {
		  
                                                   
                                           
     
		ipobf.isbroadcast = FALSE;
		if (ia != NULL) {
			IFA_REMREF(&ia->ia_ifa);
		}

		                                 
		IFP_TO_IA(ifp, ia);
	} else {
		struct ifaddr *ia0 = NULL;
		boolean_t cloneok = FALSE;
		  
                                                              
                                                              
                                                            
                                                            
                                                             
                                          
     
		if (ipobf.select_srcif &&
		    ip->ip_src.s_addr != INADDR_ANY && (ROUTE_UNUSABLE(ro) ||
		    !(ro->ro_flags & ROF_SRCIF_SELECTED))) {
			                               
			ia0 = in_selectsrcif(ip, ro, ifscope);

			  
                                                   
                                                
                                                       
            
      
			if (ia0 != NULL &&
			    IP_CHECK_RESTRICTIONS(ia0->ifa_ifp, ipobf)) {
				IFA_REMREF(ia0);
				ia0 = NULL;
				error = EHOSTUNREACH;
				if (flags & IP_OUTARGS) {
					ipoa->ipoa_retflags |= IPOARF_IFDENIED;
				}
				goto bad;
			}

			  
                                                      
                                                      
                                                        
                                                   
                                                  
                  
      
			if (ia0 == NULL && (!(flags & IP_RAWOUTPUT) ||
			    ipobf.srcbound) && ifscope != lo_ifp->if_index) {
				error = EADDRNOTAVAIL;
				goto bad;
			}

			  
                                                        
                                                          
                                                          
                                                       
                                                          
                                                       
                                                     
                                                         
                                                         
                                                        
                                                      
                                          
      
			if (ia0 != NULL) {
				if (ifscope == IFSCOPE_NONE) {
					ifscope = ia0->ifa_ifp->if_index;
				}
				cloneok = (!(flags & IP_RAWOUTPUT) &&
				    !(IN_LINKLOCAL(ntohl(ip->ip_dst.s_addr))));
			}
		}

		  
                                                            
                                                             
                                                          
                                                             
                                                            
                                                              
                                              
     
		if (ro->ro_rt == NULL) {
			uint32_t ign = RTF_PRCLONING;
			  
                                                   
                                                       
                                                      
                                                     
                                                     
                                                    
                                                       
                                                      
                                                      
                                                       
                                                     
                                                      
                                             
     
                                                       
                                                    
      
			if (cloneok || dst->sin_addr.s_addr == INADDR_BROADCAST) {
				ign &= ~RTF_PRCLONING;
			}

			  
                                                     
                                                    
                                                  
                                                       
                                                    
                                                    
                                                
      
			if (ifscope == lo_ifp->if_index) {
				rtalloc_ign(ro, ign);
			} else {
				rtalloc_scoped_ign(ro, ign, ifscope);
			}

			  
                                                           
                                                               
                                     
      
			if (ro->ro_rt != NULL) {
				RT_LOCK_SPIN(ro->ro_rt);
				if (IP_CHECK_RESTRICTIONS(ro->ro_rt->rt_ifp,
				    ipobf)) {
					RT_UNLOCK(ro->ro_rt);
					ROUTE_RELEASE(ro);
					if (flags & IP_OUTARGS) {
						ipoa->ipoa_retflags |=
						    IPOARF_IFDENIED;
					}
				} else {
					RT_UNLOCK(ro->ro_rt);
				}
			}
		}

		if (ro->ro_rt == NULL) {
			OSAddAtomic(1, &ipstat.ips_noroute);
			error = EHOSTUNREACH;
			if (ia0 != NULL) {
				IFA_REMREF(ia0);
				ia0 = NULL;
			}
			goto bad;
		}

		if (ia != NULL) {
			IFA_REMREF(&ia->ia_ifa);
		}
		RT_LOCK_SPIN(ro->ro_rt);
		ia = ifatoia(ro->ro_rt->rt_ifa);
		if (ia != NULL) {
			                            
			RT_CONVERT_LOCK(ro->ro_rt);
			IFA_ADDREF(&ia->ia_ifa);
		}
		  
                                                           
                                                           
     
		ifp = ro->ro_rt->rt_ifp;
		ro->ro_rt->rt_use++;
		if (ro->ro_rt->rt_flags & RTF_GATEWAY) {
			dst = SIN(ro->ro_rt->rt_gateway);
		}
		if (ro->ro_rt->rt_flags & RTF_HOST) {
			                                               
			ipobf.isbroadcast =
			    !!(ro->ro_rt->rt_flags & RTF_BROADCAST);
		} else {
			                            
			RT_CONVERT_LOCK(ro->ro_rt);
			ipobf.isbroadcast = in_broadcast(dst->sin_addr, ifp);
		}
		  
                                                         
                                                         
                                                           
                                                      
                                                         
                                                      
                                                            
     
		if (ia != NULL && (ifp->if_flags & IFF_LOOPBACK) &&
		    !IN_MULTICAST(ntohl(pkt_dst.s_addr))) {
			uint32_t srcidx;

			m->m_pkthdr.rcvif = ia->ia_ifa.ifa_ifp;

			if (ia0 != NULL) {
				srcidx = ia0->ifa_ifp->if_index;
			} else if ((ro->ro_flags & ROF_SRCIF_SELECTED) &&
			    ro->ro_srcia != NULL) {
				srcidx = ro->ro_srcia->ifa_ifp->if_index;
			} else {
				srcidx = 0;
			}

			ip_setsrcifaddr_info(m, srcidx, NULL);
			ip_setdstifaddr_info(m, 0, ia);
		}
		RT_UNLOCK(ro->ro_rt);
		if (ia0 != NULL) {
			IFA_REMREF(ia0);
			ia0 = NULL;
		}
	}

	if (IN_MULTICAST(ntohl(pkt_dst.s_addr))) {
		struct ifnet *srcifp = NULL;
		struct in_multi *inm;
		u_int32_t vif = 0;
		u_int8_t ttl = IP_DEFAULT_MULTICAST_TTL;
		u_int8_t loop = IP_DEFAULT_MULTICAST_LOOP;

		m->m_flags |= M_MCAST;
		  
                                                          
                                                            
                                                   
     
		dst = SIN(&ro->ro_dst);
		  
                                                     
     
		if (imo != NULL) {
			IMO_LOCK(imo);
			vif = imo->imo_multicast_vif;
			ttl = imo->imo_multicast_ttl;
			loop = imo->imo_multicast_loop;
			if (!(flags & IP_RAWOUTPUT)) {
				ip->ip_ttl = ttl;
			}
			if (imo->imo_multicast_ifp != NULL) {
				ifp = imo->imo_multicast_ifp;
			}
			IMO_UNLOCK(imo);
		} else if (!(flags & IP_RAWOUTPUT)) {
			vif = -1;
			ip->ip_ttl = ttl;
		}
		  
                                                            
     
		if (imo == NULL || vif == -1) {
			if (!(ifp->if_flags & IFF_MULTICAST)) {
				OSAddAtomic(1, &ipstat.ips_noroute);
				error = ENETUNREACH;
				goto bad;
			}
		}
		  
                                                     
                           
     
		if (ip->ip_src.s_addr == INADDR_ANY) {
			struct in_ifaddr *ia1;
			lck_rw_lock_shared(in_ifaddr_rwlock);
			TAILQ_FOREACH(ia1, &in_ifaddrhead, ia_link) {
				IFA_LOCK_SPIN(&ia1->ia_ifa);
				if (ia1->ia_ifp == ifp) {
					ip->ip_src = IA_SIN(ia1)->sin_addr;
					srcifp = ifp;
					IFA_UNLOCK(&ia1->ia_ifa);
					break;
				}
				IFA_UNLOCK(&ia1->ia_ifa);
			}
			lck_rw_done(in_ifaddr_rwlock);
			if (ip->ip_src.s_addr == INADDR_ANY) {
				error = ENETUNREACH;
				goto bad;
			}
		}

		in_multihead_lock_shared();
		IN_LOOKUP_MULTI(&pkt_dst, ifp, inm);
		in_multihead_lock_done();
		if (inm != NULL && (imo == NULL || loop)) {
			  
                                                     
                                                       
                                        
      
			if (!TAILQ_EMPTY(&ipv4_filters)
#if NECP
			    && !necp_packet_should_skip_filters(m)
#endif        
			    ) {
				struct ipfilter *filter;
				int seen = (inject_filter_ref == NULL);

				if (imo != NULL) {
					ipf_pktopts.ippo_flags |=
					    IPPOF_MCAST_OPTS;
					ipf_pktopts.ippo_mcast_ifnet = ifp;
					ipf_pktopts.ippo_mcast_ttl = ttl;
					ipf_pktopts.ippo_mcast_loop = loop;
				}

				ipf_ref();

				  
                                         
                      
       
#if BYTE_ORDER != BIG_ENDIAN
				HTONS(ip->ip_len);
				HTONS(ip->ip_off);
#endif
				TAILQ_FOREACH(filter, &ipv4_filters, ipf_link) {
					if (seen == 0) {
						if ((struct ipfilter *)
						    inject_filter_ref == filter) {
							seen = 1;
						}
					} else if (filter->ipf_filter.
					    ipf_output != NULL) {
						errno_t result;
						result = filter->ipf_filter.
						    ipf_output(filter->
						    ipf_filter.cookie,
						    (mbuf_t *)&m, ippo);
						if (result == EJUSTRETURN) {
							ipf_unref();
							INM_REMREF(inm);
							goto done;
						}
						if (result != 0) {
							ipf_unref();
							INM_REMREF(inm);
							goto bad;
						}
					}
				}

				                                 
				ip = mtod(m, struct ip *);
#if BYTE_ORDER != BIG_ENDIAN
				NTOHS(ip->ip_len);
				NTOHS(ip->ip_off);
#endif
				ipf_unref();
				ipobf.didfilter = TRUE;
			}
			ip_mloopback(srcifp, ifp, m, dst, hlen);
		}
		if (inm != NULL) {
			INM_REMREF(inm);
		}
		  
                                                          
                                                           
                                                         
                                                          
                                                          
                                                 
     
		if (ip->ip_ttl == 0 || ifp->if_flags & IFF_LOOPBACK) {
			m_freem(m);
			goto done;
		}

		goto sendit;
	}
	  
                                                    
                          
    
	if (ip->ip_src.s_addr == INADDR_ANY) {
		IFA_LOCK_SPIN(&ia->ia_ifa);
		ip->ip_src = IA_SIN(ia)->sin_addr;
		IFA_UNLOCK(&ia->ia_ifa);
	}

	  
                                  
                                      
                  
    
	if (ipobf.isbroadcast) {
		if (!(ifp->if_flags & IFF_BROADCAST)) {
			error = EADDRNOTAVAIL;
			goto bad;
		}
		if (!(flags & IP_ALLOWBROADCAST)) {
			error = EACCES;
			goto bad;
		}
		                                                     
		if ((u_short)ip->ip_len > ifp->if_mtu) {
			error = EMSGSIZE;
			goto bad;
		}
		m->m_flags |= M_BCAST;
	} else {
		m->m_flags &= ~M_BCAST;
	}

sendit:
#if PF
	                                   
	if (PF_IS_ENABLED) {
		int rc;

		m0 = m;                     
#if DUMMYNET
		args.fwa_m = m;
		args.fwa_oif = ifp;
		args.fwa_ro = ro;
		args.fwa_dst = dst;
		args.fwa_oflags = flags;
		if (flags & IP_OUTARGS) {
			args.fwa_ipoa = ipoa;
		}
		rc = pf_af_hook(ifp, mppn, &m, AF_INET, FALSE, &args);
#else               
		rc = pf_af_hook(ifp, mppn, &m, AF_INET, FALSE, NULL);
#endif               
		if (rc != 0 || m == NULL) {
			                             
			m = *mppn;

			                                                    
			if (packetlist == m0) {
				packetlist = m;
			}

			if (m != NULL) {
				m0 = m;
				                              
				goto loopit;
			} else if (packetlist != NULL) {
				                                         
				goto sendchain;
			}
			                              
			goto done;
		}
		m0 = m;
		ip = mtod(m, struct ip *);
		pkt_dst = ip->ip_dst;
		hlen = IP_VHL_HL(ip->ip_vhl) << 2;
	}
#endif         
	  
                                                                        
    
	if (IN_LINKLOCAL(ntohl(ip->ip_src.s_addr)) ||
	    IN_LINKLOCAL(ntohl(ip->ip_dst.s_addr))) {
		ip_linklocal_stat.iplls_out_total++;
		if (ip->ip_ttl != MAXTTL) {
			ip_linklocal_stat.iplls_out_badttl++;
			ip->ip_ttl = MAXTTL;
		}
	}

	if (!ipobf.didfilter &&
	    !TAILQ_EMPTY(&ipv4_filters)
#if NECP
	    && !necp_packet_should_skip_filters(m)
#endif        
	    ) {
		struct ipfilter *filter;
		int seen = (inject_filter_ref == NULL);
		ipf_pktopts.ippo_flags &= ~IPPOF_MCAST_OPTS;

		  
                                                     
                                                    
                                   
     
		if (m->m_pkthdr.csum_flags & CSUM_TSO_IPV4) {
			error = EMSGSIZE;
			goto bad;
		}

		ipf_ref();

		                                                        
#if BYTE_ORDER != BIG_ENDIAN
		HTONS(ip->ip_len);
		HTONS(ip->ip_off);
#endif
		TAILQ_FOREACH(filter, &ipv4_filters, ipf_link) {
			if (seen == 0) {
				if ((struct ipfilter *)inject_filter_ref ==
				    filter) {
					seen = 1;
				}
			} else if (filter->ipf_filter.ipf_output) {
				errno_t result;
				result = filter->ipf_filter.
				    ipf_output(filter->ipf_filter.cookie,
				    (mbuf_t *)&m, ippo);
				if (result == EJUSTRETURN) {
					ipf_unref();
					goto done;
				}
				if (result != 0) {
					ipf_unref();
					goto bad;
				}
			}
		}
		                                 
		ip = mtod(m, struct ip *);
#if BYTE_ORDER != BIG_ENDIAN
		NTOHS(ip->ip_len);
		NTOHS(ip->ip_off);
#endif
		ipf_unref();
	}

#if NECP
	                                                                          
	necp_matched_policy_id = necp_ip_output_find_policy_match(m,
	    flags, (flags & IP_OUTARGS) ? ipoa : NULL, ro ? ro->ro_rt : NULL, &necp_result, &necp_result_parameter);
	if (necp_matched_policy_id) {
		necp_mark_packet_from_ip(m, necp_matched_policy_id);
		switch (necp_result) {
		case NECP_KERNEL_POLICY_RESULT_PASS:
			if (necp_result_parameter.pass_flags & NECP_KERNEL_POLICY_PASS_NO_SKIP_IPSEC) {
				break;
			}
			                                       
			if (!necp_packet_is_allowed_over_interface(m, ifp)) {
				error = EHOSTUNREACH;
				OSAddAtomic(1, &ipstat.ips_necp_policy_drop);
				goto bad;
			}
			goto skip_ipsec;
		case NECP_KERNEL_POLICY_RESULT_DROP:
		case NECP_KERNEL_POLICY_RESULT_SOCKET_DIVERT:
			                                                           
			error = EHOSTUNREACH;
			OSAddAtomic(1, &ipstat.ips_necp_policy_drop);
			goto bad;
		case NECP_KERNEL_POLICY_RESULT_IP_TUNNEL: {
			                                                          
			struct ifnet *policy_ifp = necp_get_ifnet_from_result_parameter(&necp_result_parameter);
			if (policy_ifp == ifp) {
				                                       
				if (!necp_packet_is_allowed_over_interface(m, ifp)) {
					error = EHOSTUNREACH;
					OSAddAtomic(1, &ipstat.ips_necp_policy_drop);
					goto bad;
				}
				goto skip_ipsec;
			} else {
				if (necp_packet_can_rebind_to_ifnet(m, policy_ifp, &necp_route, AF_INET)) {
					                                       
					if (!necp_packet_is_allowed_over_interface(m, policy_ifp)) {
						error = EHOSTUNREACH;
						OSAddAtomic(1, &ipstat.ips_necp_policy_drop);
						goto bad;
					}

					  
                                        
                                    
                                               
                                                                                                                               
        
					if (ipoa != NULL &&
					    (ipoa->ipoa_flags & IPOAF_REDO_QOSMARKING_POLICY) &&
					    net_qos_policy_restricted != 0) {
						bool qos_marking = (ipoa->ipoa_flags & IPOAF_QOSMARKING_ALLOWED) ? TRUE : FALSE;
						qos_marking = necp_lookup_current_qos_marking(&ipoa->qos_marking_gencount, NULL, policy_ifp, necp_result_parameter.route_rule_id, qos_marking);
						if (qos_marking) {
							ipoa->ipoa_flags |= IPOAF_QOSMARKING_ALLOWED;
						} else {
							ipoa->ipoa_flags &= ~IPOAF_QOSMARKING_ALLOWED;
						}
					}

					                                                                             
					ifp = policy_ifp;
					ro = &necp_route;
					goto skip_ipsec;
				} else {
					error = ENETUNREACH;
					OSAddAtomic(1, &ipstat.ips_necp_policy_drop);
					goto bad;
				}
			}
		}
		default:
			break;
		}
	}
	                                                    
	if (!necp_packet_is_allowed_over_interface(m, ifp)) {
		error = EHOSTUNREACH;
		OSAddAtomic(1, &ipstat.ips_necp_policy_drop);
		goto bad;
	}
#endif           

#if IPSEC
	if (ipsec_bypass != 0 || (flags & IP_NOIPSEC)) {
		goto skip_ipsec;
	}

	KERNEL_DEBUG(DBG_FNC_IPSEC4_OUTPUT | DBG_FUNC_START, 0, 0, 0, 0, 0);

	if (sp == NULL) {
		                            
		if (so != NULL) {
			sp = ipsec4_getpolicybysock(m, IPSEC_DIR_OUTBOUND,
			    so, &error);
		} else {
			sp = ipsec4_getpolicybyaddr(m, IPSEC_DIR_OUTBOUND,
			    flags, &error);
		}
		if (sp == NULL) {
			IPSEC_STAT_INCREMENT(ipsecstat.out_inval);
			KERNEL_DEBUG(DBG_FNC_IPSEC4_OUTPUT | DBG_FUNC_END,
			    0, 0, 0, 0, 0);
			goto bad;
		}
	}

	error = 0;

	                  
	switch (sp->policy) {
	case IPSEC_POLICY_DISCARD:
	case IPSEC_POLICY_GENERATE:
		  
                                   
     
		IPSEC_STAT_INCREMENT(ipsecstat.out_polvio);
		KERNEL_DEBUG(DBG_FNC_IPSEC4_OUTPUT | DBG_FUNC_END,
		    1, 0, 0, 0, 0);
		goto bad;

	case IPSEC_POLICY_BYPASS:
	case IPSEC_POLICY_NONE:
		                          
		KERNEL_DEBUG(DBG_FNC_IPSEC4_OUTPUT | DBG_FUNC_END,
		    2, 0, 0, 0, 0);
		goto skip_ipsec;

	case IPSEC_POLICY_IPSEC:
		if (sp->req == NULL) {
			                      
			error = key_spdacquire(sp);
			KERNEL_DEBUG(DBG_FNC_IPSEC4_OUTPUT | DBG_FUNC_END,
			    3, 0, 0, 0, 0);
			goto bad;
		}
		if (sp->ipsec_if) {
			                                            
			if (sp->ipsec_if == ifp) {
				goto skip_ipsec;
			}
			goto bad;
		}
		break;

	case IPSEC_POLICY_ENTRUST:
	default:
		printf("ip_output: Invalid policy found. %d\n", sp->policy);
	}
	{
		ipsec_state.m = m;
		if (flags & IP_ROUTETOIF) {
			bzero(&ipsec_state.ro, sizeof(ipsec_state.ro));
		} else {
			route_copyout((struct route *)&ipsec_state.ro, ro, sizeof(struct route));
		}
		ipsec_state.dst = SA(dst);

		ip->ip_sum = 0;

		  
        
                                                              
     
		if (m->m_pkthdr.csum_flags & CSUM_DELAY_DATA) {
			in_delayed_cksum(m);
		}

#if BYTE_ORDER != BIG_ENDIAN
		HTONS(ip->ip_len);
		HTONS(ip->ip_off);
#endif

		DTRACE_IP6(send, struct mbuf *, m, struct inpcb *, NULL,
		    struct ip *, ip, struct ifnet *, ifp,
		    struct ip *, ip, struct ip6_hdr *, NULL);

		error = ipsec4_output(&ipsec_state, sp, flags);
		if (ipsec_state.tunneled == 6) {
			m0 = m = NULL;
			error = 0;
			goto bad;
		}

		m0 = m = ipsec_state.m;

#if DUMMYNET
		  
                                                   
                                             
     
		if (ro == &saved_route &&
		    (!(flags & IP_ROUTETOIF) || ipsec_state.tunneled)) {
			ROUTE_RELEASE(ro);
		}
#endif               

		if (flags & IP_ROUTETOIF) {
			  
                                                      
                   
      
			if (ipsec_state.tunneled) {
				flags &= ~IP_ROUTETOIF;
				ro = (struct route *)&ipsec_state.ro;
			}
		} else {
			ro = (struct route *)&ipsec_state.ro;
		}
		dst = SIN(ipsec_state.dst);
		if (error) {
			                                                 
			m0 = NULL;
			switch (error) {
			case EHOSTUNREACH:
			case ENETUNREACH:
			case EMSGSIZE:
			case ENOBUFS:
			case ENOMEM:
				break;
			default:
				printf("ip4_output (ipsec): error code %d\n", error);
				OS_FALLTHROUGH;
			case ENOENT:
				                                              
				error = 0;
				break;
			}
			KERNEL_DEBUG(DBG_FNC_IPSEC4_OUTPUT | DBG_FUNC_END,
			    4, 0, 0, 0, 0);
			goto bad;
		}
	}

	                                                                      
	ip = mtod(m, struct ip *);

#ifdef _IP_VHL
	hlen = IP_VHL_HL(ip->ip_vhl) << 2;
#else               
	hlen = ip->ip_hl << 2;
#endif               
	                                                                   
	if (ROUTE_UNUSABLE(ro)) {
		ROUTE_RELEASE(ro);
		VERIFY(src_ia == NULL);
		if (ip->ip_src.s_addr != INADDR_ANY &&
		    !(flags & (IP_ROUTETOIF | IP_FORWARDING)) &&
		    (src_ia = ifa_foraddr(ip->ip_src.s_addr)) == NULL) {
			error = EADDRNOTAVAIL;
			KERNEL_DEBUG(DBG_FNC_IPSEC4_OUTPUT | DBG_FUNC_END,
			    5, 0, 0, 0, 0);
			goto bad;
		}
		if (src_ia != NULL) {
			IFA_REMREF(&src_ia->ia_ifa);
			src_ia = NULL;
		}
	}

	if (ro->ro_rt == NULL) {
		if (!(flags & IP_ROUTETOIF)) {
			printf("%s: can't update route after "
			    "IPsec processing\n", __func__);
			error = EHOSTUNREACH;            
			KERNEL_DEBUG(DBG_FNC_IPSEC4_OUTPUT | DBG_FUNC_END,
			    6, 0, 0, 0, 0);
			goto bad;
		}
	} else {
		if (ia != NULL) {
			IFA_REMREF(&ia->ia_ifa);
		}
		RT_LOCK_SPIN(ro->ro_rt);
		ia = ifatoia(ro->ro_rt->rt_ifa);
		if (ia != NULL) {
			                            
			RT_CONVERT_LOCK(ro->ro_rt);
			IFA_ADDREF(&ia->ia_ifa);
		}
		ifp = ro->ro_rt->rt_ifp;
		RT_UNLOCK(ro->ro_rt);
	}

	                             
#if BYTE_ORDER != BIG_ENDIAN
	NTOHS(ip->ip_len);
	NTOHS(ip->ip_off);
#endif
	KERNEL_DEBUG(DBG_FNC_IPSEC4_OUTPUT | DBG_FUNC_END,
	    7, 0xff, 0xff, 0xff, 0xff);

	                           
	if (!TAILQ_EMPTY(&ipv4_filters)
#if NECP
	    && !necp_packet_should_skip_filters(m)
#endif        
	    ) {
		struct ipfilter *filter;

		ipf_pktopts.ippo_flags &= ~IPPOF_MCAST_OPTS;

		  
                                                     
                                                    
                                   
     
		if (m->m_pkthdr.csum_flags & CSUM_TSO_IPV4) {
			error = EMSGSIZE;
			goto bad;
		}

		ipf_ref();

		                                                        
#if BYTE_ORDER != BIG_ENDIAN
		HTONS(ip->ip_len);
		HTONS(ip->ip_off);
#endif
		TAILQ_FOREACH(filter, &ipv4_filters, ipf_link) {
			if (filter->ipf_filter.ipf_output) {
				errno_t result;
				result = filter->ipf_filter.
				    ipf_output(filter->ipf_filter.cookie,
				    (mbuf_t *)&m, ippo);
				if (result == EJUSTRETURN) {
					ipf_unref();
					goto done;
				}
				if (result != 0) {
					ipf_unref();
					goto bad;
				}
			}
		}
		                                 
		ip = mtod(m, struct ip *);
#if BYTE_ORDER != BIG_ENDIAN
		NTOHS(ip->ip_len);
		NTOHS(ip->ip_off);
#endif
		ipf_unref();
	}
skip_ipsec:
#endif            


	                                             
	if (!(ifp->if_flags & IFF_LOOPBACK) &&
	    ((ntohl(ip->ip_src.s_addr) >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET ||
	    (ntohl(ip->ip_dst.s_addr) >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET)) {
		OSAddAtomic(1, &ipstat.ips_badaddr);
		error = EADDRNOTAVAIL;
		goto bad;
	}

	if (ipoa != NULL) {
		u_int8_t dscp = ip->ip_tos >> IPTOS_DSCP_SHIFT;

		error = set_packet_qos(m, ifp,
		    ipoa->ipoa_flags & IPOAF_QOSMARKING_ALLOWED ? TRUE : FALSE,
		    ipoa->ipoa_sotc, ipoa->ipoa_netsvctype, &dscp);
		if (error == 0) {
			ip->ip_tos &= IPTOS_ECN_MASK;
			ip->ip_tos |= dscp << IPTOS_DSCP_SHIFT;
		} else {
			printf("%s if_dscp_for_mbuf() error %d\n", __func__, error);
			error = 0;
		}
	}

	ip_output_checksum(ifp, m, (IP_VHL_HL(ip->ip_vhl) << 2),
	    ip->ip_len, &sw_csum);

	interface_mtu = ifp->if_mtu;

	if (INTF_ADJUST_MTU_FOR_CLAT46(ifp)) {
		interface_mtu = IN6_LINKMTU(ifp);
		                                                  
		interface_mtu -= CLAT46_HDR_EXPANSION_OVERHD;
	}

	  
                                                             
                                                             
    
	if ((u_short)ip->ip_len <= interface_mtu || TSO_IPV4_OK(ifp, m) ||
	    (!(ip->ip_off & IP_DF) && (ifp->if_hwassist & CSUM_FRAGMENT))) {
#if BYTE_ORDER != BIG_ENDIAN
		HTONS(ip->ip_len);
		HTONS(ip->ip_off);
#endif

		ip->ip_sum = 0;
		if (sw_csum & CSUM_DELAY_IP) {
			ip->ip_sum = ip_cksum_hdr_out(m, hlen);
			sw_csum &= ~CSUM_DELAY_IP;
			m->m_pkthdr.csum_flags &= ~CSUM_DELAY_IP;
		}

#if IPSEC
		                                                      
		if (ipsec_bypass == 0 && !(flags & IP_NOIPSEC)) {
			ipsec_delaux(m);
		}
#endif            
		if ((m->m_pkthdr.csum_flags & CSUM_TSO_IPV4) &&
		    (m->m_pkthdr.tso_segsz > 0)) {
			scnt += m->m_pkthdr.len / m->m_pkthdr.tso_segsz;
		} else {
			scnt++;
		}

		if (packetchain == 0) {
			if (ro->ro_rt != NULL && nstat_collect) {
				nstat_route_tx(ro->ro_rt, scnt,
				    m->m_pkthdr.len, 0);
			}

			error = dlil_output(ifp, PF_INET, m, ro->ro_rt,
			    SA(dst), 0, adv);
			if (dlil_verbose && error) {
				printf("dlil_output error on interface %s: %d\n",
				    ifp->if_xname, error);
			}
			scnt = 0;
			goto done;
		} else {
			  
                                            
                           
      
			bytecnt += m->m_pkthdr.len;
			mppn = &m->m_nextpkt;
			m = m->m_nextpkt;
			if (m == NULL) {
#if PF
sendchain:
#endif         
				if (pktcnt > ip_maxchainsent) {
					ip_maxchainsent = pktcnt;
				}
				if (ro->ro_rt != NULL && nstat_collect) {
					nstat_route_tx(ro->ro_rt, scnt,
					    bytecnt, 0);
				}

				error = dlil_output(ifp, PF_INET, packetlist,
				    ro->ro_rt, SA(dst), 0, adv);
				if (dlil_verbose && error) {
					printf("dlil_output error on interface %s: %d\n",
					    ifp->if_xname, error);
				}
				pktcnt = 0;
				scnt = 0;
				bytecnt = 0;
				goto done;
			}
			m0 = m;
			pktcnt++;
			goto loopit;
		}
	}

	VERIFY(interface_mtu != 0);
	  
                                                  
                                                      
                                                                
    
	if ((ip->ip_off & IP_DF) || pktcnt > 0 ||
	    (m->m_pkthdr.csum_flags & CSUM_TSO_IPV4)) {
		error = EMSGSIZE;
		  
                                                     
                                                      
                                                       
                                                    
                                    
     
		if (ro->ro_rt) {
			RT_LOCK_SPIN(ro->ro_rt);
			if ((ro->ro_rt->rt_flags & (RTF_UP | RTF_HOST)) &&
			    !(ro->ro_rt->rt_rmx.rmx_locks & RTV_MTU) &&
			    (ro->ro_rt->rt_rmx.rmx_mtu > interface_mtu)) {
				ro->ro_rt->rt_rmx.rmx_mtu = interface_mtu;
			}
			RT_UNLOCK(ro->ro_rt);
		}
		if (pktcnt > 0) {
			m0 = packetlist;
		}
		OSAddAtomic(1, &ipstat.ips_cantfrag);
		goto bad;
	}

	  
                                                            
                                                                    
                                                                   
    
	if (packetchain == 0 && IS_INTF_CLAT46(ifp)) {
		  
                                                     
                                                             
                                 
     
		if (ip->ip_p == IPPROTO_UDP &&
		    !(m->m_pkthdr.csum_flags & (CSUM_UDP | CSUM_PARTIAL))) {
			struct udphdr *uh = NULL;

			if (m->m_len < hlen + sizeof(struct udphdr)) {
				m = m_pullup(m, hlen + sizeof(struct udphdr));
				if (m == NULL) {
					error = ENOBUFS;
					m0 = m;
					goto bad;
				}
				m0 = m;
				ip = mtod(m, struct ip *);
			}
			  
                                                                
               
      
			uh = (struct udphdr *)(void *)((caddr_t)ip + hlen);
			if (uh->uh_sum == 0) {
				uh->uh_sum = inet_cksum(m, IPPROTO_UDP, hlen,
				    ip->ip_len - hlen);
				if (uh->uh_sum == 0) {
					uh->uh_sum = 0xffff;
				}
			}
		}
	}

	error = ip_fragment(m, ifp, interface_mtu, sw_csum);
	if (error != 0) {
		m0 = m = NULL;
		goto bad;
	}

	KERNEL_DEBUG(DBG_LAYER_END, ip->ip_dst.s_addr,
	    ip->ip_src.s_addr, ip->ip_p, ip->ip_off, ip->ip_len);

	for (m = m0; m; m = m0) {
		m0 = m->m_nextpkt;
		m->m_nextpkt = 0;
#if IPSEC
		                                                      
		if (ipsec_bypass == 0 && !(flags & IP_NOIPSEC)) {
			ipsec_delaux(m);
		}
#endif            
		if (error == 0) {
			if ((packetchain != 0) && (pktcnt > 0)) {
				panic("%s: mix of packet in packetlist is "
				    "wrong=%p", __func__, packetlist);
				                
			}
			if (ro->ro_rt != NULL && nstat_collect) {
				nstat_route_tx(ro->ro_rt, 1,
				    m->m_pkthdr.len, 0);
			}
			error = dlil_output(ifp, PF_INET, m, ro->ro_rt,
			    SA(dst), 0, adv);
			if (dlil_verbose && error) {
				printf("dlil_output error on interface %s: %d\n",
				    ifp->if_xname, error);
			}
		} else {
			m_freem(m);
		}
	}

	if (error == 0) {
		OSAddAtomic(1, &ipstat.ips_fragmented);
	}

done:
	if (ia != NULL) {
		IFA_REMREF(&ia->ia_ifa);
		ia = NULL;
	}
#if IPSEC
	ROUTE_RELEASE(&ipsec_state.ro);
	if (sp != NULL) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		    printf("DP ip_output call free SP:%x\n", sp));
		key_freesp(sp, KEY_SADB_UNLOCKED);
	}
#endif            
#if NECP
	ROUTE_RELEASE(&necp_route);
#endif           
#if DUMMYNET
	ROUTE_RELEASE(&saved_route);
#endif               

	KERNEL_DEBUG(DBG_FNC_IP_OUTPUT | DBG_FUNC_END, error, 0, 0, 0, 0);
	if (ip_output_measure) {
		net_perf_measure_time(&net_perf, &start_tv, packets_processed);
		net_perf_histogram(&net_perf, packets_processed);
	}
	return error;
bad:
	if (pktcnt > 0) {
		m0 = packetlist;
	}
	m_freem_list(m0);
	goto done;

#undef ipsec_state
#undef args
#undef sro_fwd
#undef saved_route
#undef ipf_pktopts
#undef IP_CHECK_RESTRICTIONS
}

int
ip_fragment(struct mbuf *m, struct ifnet *ifp, uint32_t mtu, int sw_csum)
{
	struct ip *ip, *mhip;
	int len, hlen, mhlen, firstlen, off, error = 0;
	struct mbuf **mnext = &m->m_nextpkt, *m0;
	int nfrags = 1;

	ip = mtod(m, struct ip *);
#ifdef _IP_VHL
	hlen = IP_VHL_HL(ip->ip_vhl) << 2;
#else               
	hlen = ip->ip_hl << 2;
#endif               

	  
                                                   
                                                         
                      
    
	if (IS_INTF_CLAT46(ifp)) {
		mtu -= sizeof(struct ip6_frag);
	}

	firstlen = len = (mtu - hlen) & ~7;
	if (len < 8) {
		m_freem(m);
		return EMSGSIZE;
	}

	  
                                                    
                                        
    
	if ((m->m_pkthdr.csum_flags & CSUM_DELAY_DATA) &&
	    !(ifp->if_hwassist & CSUM_IP_FRAGS)) {
		in_delayed_cksum(m);
	}

	  
                                                        
                                                                   
    
	m0 = m;
	mhlen = sizeof(struct ip);
	for (off = hlen + len; off < (u_short)ip->ip_len; off += len) {
		MGETHDR(m, M_DONTWAIT, MT_HEADER);                  
		if (m == NULL) {
			error = ENOBUFS;
			OSAddAtomic(1, &ipstat.ips_odropped);
			goto sendorfree;
		}
		m->m_flags |= (m0->m_flags & M_MCAST) | M_FRAG;
		m->m_data += max_linkhdr;
		mhip = mtod(m, struct ip *);
		*mhip = *ip;
		if (hlen > sizeof(struct ip)) {
			mhlen = ip_optcopy(ip, mhip) + sizeof(struct ip);
			mhip->ip_vhl = IP_MAKE_VHL(IPVERSION, mhlen >> 2);
		}
		m->m_len = mhlen;
		mhip->ip_off = (u_short)(((off - hlen) >> 3) + (ip->ip_off & ~IP_MF));
		if (ip->ip_off & IP_MF) {
			mhip->ip_off |= IP_MF;
		}
		if (off + len >= (u_short)ip->ip_len) {
			len = (u_short)ip->ip_len - off;
		} else {
			mhip->ip_off |= IP_MF;
		}
		mhip->ip_len = htons((u_short)(len + mhlen));
		m->m_next = m_copy(m0, off, len);
		if (m->m_next == NULL) {
			(void) m_free(m);
			error = ENOBUFS;                 
			OSAddAtomic(1, &ipstat.ips_odropped);
			goto sendorfree;
		}
		m->m_pkthdr.len = mhlen + len;
		m->m_pkthdr.rcvif = NULL;
		m->m_pkthdr.csum_flags = m0->m_pkthdr.csum_flags;

		M_COPY_CLASSIFIER(m, m0);
		M_COPY_PFTAG(m, m0);
		M_COPY_NECPTAG(m, m0);

#if BYTE_ORDER != BIG_ENDIAN
		HTONS(mhip->ip_off);
#endif

		mhip->ip_sum = 0;
		if (sw_csum & CSUM_DELAY_IP) {
			mhip->ip_sum = ip_cksum_hdr_out(m, mhlen);
			m->m_pkthdr.csum_flags &= ~CSUM_DELAY_IP;
		}
		*mnext = m;
		mnext = &m->m_nextpkt;
		nfrags++;
	}
	OSAddAtomic(nfrags, &ipstat.ips_ofragments);

	                                               
	m->m_flags |= M_LASTFRAG;
	m0->m_flags |= M_FIRSTFRAG | M_FRAG;
	m0->m_pkthdr.csum_data = nfrags;

	  
                                                            
                                                            
    
	m = m0;
	m_adj(m, hlen + firstlen - (u_short)ip->ip_len);
	m->m_pkthdr.len = hlen + firstlen;
	ip->ip_len = htons((u_short)m->m_pkthdr.len);
	ip->ip_off |= IP_MF;

#if BYTE_ORDER != BIG_ENDIAN
	HTONS(ip->ip_off);
#endif

	ip->ip_sum = 0;
	if (sw_csum & CSUM_DELAY_IP) {
		ip->ip_sum = ip_cksum_hdr_out(m, hlen);
		m->m_pkthdr.csum_flags &= ~CSUM_DELAY_IP;
	}
sendorfree:
	if (error) {
		m_freem_list(m0);
	}

	return error;
}

static void
ip_out_cksum_stats(int proto, u_int32_t len)
{
	switch (proto) {
	case IPPROTO_TCP:
		tcp_out_cksum_stats(len);
		break;
	case IPPROTO_UDP:
		udp_out_cksum_stats(len);
		break;
	default:
		                                        
		break;
	}
}

  
                                                                  
  
                                                                 
                           
  
                                                                
   
uint32_t
in_finalize_cksum(struct mbuf *m, uint32_t hoff, uint32_t csum_flags)
{
	unsigned char buf[15 << 2] __attribute__((aligned(8)));
	struct ip *ip;
	uint32_t offset, _hlen, mlen, hlen, len, sw_csum;
	uint16_t csum, ip_len;

	_CASSERT(sizeof(csum) == sizeof(uint16_t));
	VERIFY(m->m_flags & M_PKTHDR);

	sw_csum = (csum_flags & m->m_pkthdr.csum_flags);

	if ((sw_csum &= (CSUM_DELAY_IP | CSUM_DELAY_DATA)) == 0) {
		goto done;
	}

	mlen = m->m_pkthdr.len;                                             

	                                                   
	if (mlen < (hoff + sizeof(*ip))) {
		panic("%s: mbuf %p pkt len (%u) < hoff+ip_hdr "
		    "(%u+%u)\n", __func__, m, mlen, hoff,
		    (uint32_t)sizeof(*ip));
		                
	}

	  
                                                                   
                                                                    
                                                                 
                      
    
	if ((sw_csum & CSUM_DELAY_IP) || (hoff + sizeof(*ip)) > m->m_len ||
	    !IP_HDR_ALIGNED_P(mtod(m, caddr_t) + hoff)) {
		m_copydata(m, hoff, sizeof(*ip), (caddr_t)buf);
		ip = (struct ip *)(void *)buf;
		_hlen = sizeof(*ip);
	} else {
		ip = (struct ip *)(void *)(m->m_data + hoff);
		_hlen = 0;
	}

	hlen = IP_VHL_HL(ip->ip_vhl) << 2;                                 

	                  
	if (mlen < (hoff + hlen)) {
		panic("%s: mbuf %p pkt too short (%d) for IP header (%u), "
		    "hoff %u", __func__, m, mlen, hlen, hoff);
		                
	}

	  
                                                                   
                                                                  
                                                                 
                                                                 
                                                                 
                                                                  
                                                             
    
	ip_len = ip->ip_len;
	if (ip_len != (mlen - hoff)) {
		ip_len = OSSwapInt16(ip_len);
		if (ip_len != (mlen - hoff)) {
			printf("%s: mbuf 0x%llx proto %d IP len %d (%x) "
			    "[swapped %d (%x)] doesn't match actual packet "
			    "length; %d is used instead\n", __func__,
			    (uint64_t)VM_KERNEL_ADDRPERM(m), ip->ip_p,
			    ip->ip_len, ip->ip_len, ip_len, ip_len,
			    (mlen - hoff));
			if (mlen - hoff > UINT16_MAX) {
				panic("%s: mlen %u - hoff %u > 65535",
				    __func__, mlen, hoff);
			}
			ip_len = (uint16_t)(mlen - hoff);
		}
	}

	len = ip_len - hlen;                                           

	if (sw_csum & CSUM_DELAY_DATA) {
		uint16_t ulpoff;

		  
                                                            
                                                           
                                                   
     
		if ((m->m_pkthdr.csum_flags & (CSUM_PARTIAL | CSUM_DATA_VALID)) ==
		    (CSUM_PARTIAL | CSUM_DATA_VALID)) {
			  
                                                 
                                                
      
			m->m_pkthdr.csum_data = (m->m_pkthdr.csum_tx_stuff -
			    m->m_pkthdr.csum_tx_start);
		}

		ulpoff = (m->m_pkthdr.csum_data & 0xffff);                      
		offset = hoff + hlen;                                   

		if (mlen < (ulpoff + sizeof(csum))) {
			panic("%s: mbuf %p pkt len (%u) proto %d invalid ULP "
			    "cksum offset (%u) cksum flags 0x%x\n", __func__,
			    m, mlen, ip->ip_p, ulpoff, m->m_pkthdr.csum_flags);
			                
		}

		csum = inet_cksum(m, 0, offset, len);

		                  
		ip_out_cksum_stats(ip->ip_p, len);

		                     
		if (csum == 0 &&
		    (m->m_pkthdr.csum_flags & (CSUM_UDP | CSUM_ZERO_INVERT))) {
			csum = 0xffff;
		}

		                                               
		offset += ulpoff;
		if (offset + sizeof(csum) > m->m_len) {
			m_copyback(m, offset, sizeof(csum), &csum);
		} else if (IP_HDR_ALIGNED_P(mtod(m, char *) + hoff)) {
			*(uint16_t *)(void *)(mtod(m, char *) + offset) = csum;
		} else {
			bcopy(&csum, (mtod(m, char *) + offset), sizeof(csum));
		}
		m->m_pkthdr.csum_flags &= ~(CSUM_DELAY_DATA | CSUM_DATA_VALID |
		    CSUM_PARTIAL | CSUM_ZERO_INVERT);
	}

	if (sw_csum & CSUM_DELAY_IP) {
		                                           
		VERIFY(_hlen == sizeof(*ip));
		if (_hlen != hlen) {
			VERIFY(hlen <= sizeof(buf));
			m_copydata(m, hoff, hlen, (caddr_t)buf);
			ip = (struct ip *)(void *)buf;
			_hlen = hlen;
		}

		  
                                                       
                                                     
                                                      
                                                       
     
		ip->ip_len = htons(ip_len);
		ip->ip_sum = 0;
		csum = in_cksum_hdr_opt(ip);

		                  
		ipstat.ips_snd_swcsum++;
		ipstat.ips_snd_swcsum_bytes += hlen;

		  
                                                       
                                                     
     
		offset = hoff + offsetof(struct ip, ip_sum);
		if (offset + sizeof(csum) > m->m_len) {
			m_copyback(m, offset, sizeof(csum), &csum);
		} else if (IP_HDR_ALIGNED_P(mtod(m, char *) + hoff)) {
			*(uint16_t *)(void *)(mtod(m, char *) + offset) = csum;
		} else {
			bcopy(&csum, (mtod(m, char *) + offset), sizeof(csum));
		}
		m->m_pkthdr.csum_flags &= ~CSUM_DELAY_IP;
	}

done:
	return sw_csum;
}

  
                                           
                                                           
                                                                  
  
                                                                    
   
static struct mbuf *
ip_insertoptions(struct mbuf *m, struct mbuf *opt, int *phlen)
{
	struct ipoption *p = mtod(opt, struct ipoption *);
	struct mbuf *n;
	struct ip *ip = mtod(m, struct ip *);
	unsigned optlen;

	optlen = opt->m_len - sizeof(p->ipopt_dst);
	if (optlen + (u_short)ip->ip_len > IP_MAXPACKET) {
		return m;                                  
	}
	if (p->ipopt_dst.s_addr) {
		ip->ip_dst = p->ipopt_dst;
	}
	if (m->m_flags & M_EXT || m->m_data - optlen < m->m_pktdat) {
		MGETHDR(n, M_DONTWAIT, MT_HEADER);                  
		if (n == NULL) {
			return m;
		}
		n->m_pkthdr.rcvif = 0;
		n->m_pkthdr.len = m->m_pkthdr.len + optlen;
		m->m_len -= sizeof(struct ip);
		m->m_data += sizeof(struct ip);
		n->m_next = m;
		m = n;
		m->m_len = optlen + sizeof(struct ip);
		m->m_data += max_linkhdr;
		(void) memcpy(mtod(m, void *), ip, sizeof(struct ip));
	} else {
		m->m_data -= optlen;
		m->m_len += optlen;
		m->m_pkthdr.len += optlen;
		ovbcopy((caddr_t)ip, mtod(m, caddr_t), sizeof(struct ip));
	}
	ip = mtod(m, struct ip *);
	bcopy(p->ipopt_list, ip + 1, optlen);
	*phlen = sizeof(struct ip) + optlen;
	ip->ip_vhl = IP_MAKE_VHL(IPVERSION, *phlen >> 2);
	ip->ip_len += optlen;
	return m;
}

  
                              
                                                  
   
static int
ip_optcopy(struct ip *ip, struct ip *jp)
{
	u_char *cp, *dp;
	int opt, optlen, cnt;

	cp = (u_char *)(ip + 1);
	dp = (u_char *)(jp + 1);
	cnt = (IP_VHL_HL(ip->ip_vhl) << 2) - sizeof(struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == IPOPT_EOL) {
			break;
		}
		if (opt == IPOPT_NOP) {
			                                                    
			*dp++ = IPOPT_NOP;
			optlen = 1;
			continue;
		}
#if DIAGNOSTIC
		if (cnt < IPOPT_OLEN + sizeof(*cp)) {
			panic("malformed IPv4 option passed to ip_optcopy");
			                
		}
#endif
		optlen = cp[IPOPT_OLEN];
#if DIAGNOSTIC
		if (optlen < IPOPT_OLEN + sizeof(*cp) || optlen > cnt) {
			panic("malformed IPv4 option passed to ip_optcopy");
			                
		}
#endif
		                                                           
		if (optlen > cnt) {
			optlen = cnt;
		}
		if (IPOPT_COPIED(opt)) {
			bcopy(cp, dp, optlen);
			dp += optlen;
		}
	}
	for (optlen = (int)(dp - (u_char *)(jp + 1)); optlen & 0x3; optlen++) {
		*dp++ = IPOPT_EOL;
	}
	return optlen;
}

  
                               
   
int
ip_ctloutput(struct socket *so, struct sockopt *sopt)
{
	struct  inpcb *inp = sotoinpcb(so);
	int     error, optval;
	lck_mtx_t *mutex_held = NULL;

	error = optval = 0;
	if (sopt->sopt_level != IPPROTO_IP) {
		return EINVAL;
	}

	switch (sopt->sopt_dir) {
	case SOPT_SET:
		mutex_held = socket_getlock(so, PR_F_WILLUNLOCK);
		  
                                               
                                               
                                      
     
		if (inp->inp_sndinprog_cnt > 0) {
			inp->inp_sndingprog_waiters++;

			while (inp->inp_sndinprog_cnt > 0) {
				msleep(&inp->inp_sndinprog_cnt, mutex_held,
				    PSOCK | PCATCH, "inp_sndinprog_cnt", NULL);
			}
			inp->inp_sndingprog_waiters--;
		}
		switch (sopt->sopt_name) {
#ifdef notyet
		case IP_RETOPTS:
#endif
		case IP_OPTIONS: {
			struct mbuf *m;

			if (sopt->sopt_valsize > MLEN) {
				error = EMSGSIZE;
				break;
			}
			MGET(m, sopt->sopt_p != kernproc ? M_WAIT : M_DONTWAIT,
			    MT_HEADER);
			if (m == NULL) {
				error = ENOBUFS;
				break;
			}
			m->m_len = (int32_t)sopt->sopt_valsize;
			error = sooptcopyin(sopt, mtod(m, char *),
			    m->m_len, m->m_len);
			if (error) {
				m_freem(m);
				break;
			}

			return ip_pcbopts(sopt->sopt_name,
			           &inp->inp_options, m);
		}

		case IP_TOS:
		case IP_TTL:
		case IP_RECVOPTS:
		case IP_RECVRETOPTS:
		case IP_RECVDSTADDR:
		case IP_RECVIF:
		case IP_RECVTTL:
		case IP_RECVPKTINFO:
		case IP_RECVTOS:
		case IP_DONTFRAG:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error) {
				break;
			}

			switch (sopt->sopt_name) {
			case IP_TOS:
				if (optval > UINT8_MAX) {
					error = EINVAL;
					break;
				}
				inp->inp_ip_tos = (uint8_t)optval;
				break;

			case IP_TTL:
				if (optval > UINT8_MAX) {
					error = EINVAL;
					break;
				}
				inp->inp_ip_ttl = (uint8_t)optval;
				break;
#define OPTSET(bit) do {                                                \
	if (optval) {                                                   \
	    inp->inp_flags |= bit;                                      \
	} else {                                                        \
	    inp->inp_flags &= ~bit;                                     \
	}                                                               \
} while (0)

#define OPTSET2(bit) do {                                               \
	if (optval) {                                                   \
	    inp->inp_flags2 |= bit;                                     \
	} else {                                                        \
	    inp->inp_flags2 &= ~bit;                                    \
	}                                                               \
} while (0)

			case IP_RECVOPTS:
				OPTSET(INP_RECVOPTS);
				break;

			case IP_RECVRETOPTS:
				OPTSET(INP_RECVRETOPTS);
				break;

			case IP_RECVDSTADDR:
				OPTSET(INP_RECVDSTADDR);
				break;

			case IP_RECVIF:
				OPTSET(INP_RECVIF);
				break;

			case IP_RECVTTL:
				OPTSET(INP_RECVTTL);
				break;

			case IP_RECVPKTINFO:
				OPTSET(INP_PKTINFO);
				break;

			case IP_RECVTOS:
				OPTSET(INP_RECVTOS);
				break;

			case IP_DONTFRAG:
				                                           
				if (!(inp->inp_vflag & INP_IPV4)) {
					error = EINVAL;
					break;
				}
				OPTSET2(INP2_DONTFRAG);
				break;
#undef OPTSET
#undef OPTSET2
			}
			break;
		  
                                                           
            
     
		case IP_MULTICAST_IF:
		case IP_MULTICAST_IFINDEX:
		case IP_MULTICAST_VIF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_ADD_MEMBERSHIP:
		case IP_DROP_MEMBERSHIP:
		case IP_ADD_SOURCE_MEMBERSHIP:
		case IP_DROP_SOURCE_MEMBERSHIP:
		case IP_BLOCK_SOURCE:
		case IP_UNBLOCK_SOURCE:
		case IP_MSFILTER:
		case MCAST_JOIN_GROUP:
		case MCAST_LEAVE_GROUP:
		case MCAST_JOIN_SOURCE_GROUP:
		case MCAST_LEAVE_SOURCE_GROUP:
		case MCAST_BLOCK_SOURCE:
		case MCAST_UNBLOCK_SOURCE:
			error = inp_setmoptions(inp, sopt);
			break;

		case IP_PORTRANGE:
			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));
			if (error) {
				break;
			}

			switch (optval) {
			case IP_PORTRANGE_DEFAULT:
				inp->inp_flags &= ~(INP_LOWPORT);
				inp->inp_flags &= ~(INP_HIGHPORT);
				break;

			case IP_PORTRANGE_HIGH:
				inp->inp_flags &= ~(INP_LOWPORT);
				inp->inp_flags |= INP_HIGHPORT;
				break;

			case IP_PORTRANGE_LOW:
				inp->inp_flags &= ~(INP_HIGHPORT);
				inp->inp_flags |= INP_LOWPORT;
				break;

			default:
				error = EINVAL;
				break;
			}
			break;

#if IPSEC
		case IP_IPSEC_POLICY: {
			caddr_t req = NULL;
			size_t len = 0;
			int priv;
			struct mbuf *m;
			int optname;

			if ((error = soopt_getm(sopt, &m)) != 0) {          
				break;
			}
			if ((error = soopt_mcopyin(sopt, m)) != 0) {          
				break;
			}
			priv = (proc_suser(sopt->sopt_p) == 0);
			if (m) {
				req = mtod(m, caddr_t);
				len = m->m_len;
			}
			optname = sopt->sopt_name;
			error = ipsec4_set_policy(inp, optname, req, len, priv);
			m_freem(m);
			break;
		}
#endif            

#if TRAFFIC_MGT
		case IP_TRAFFIC_MGT_BACKGROUND: {
			unsigned background = 0;

			error = sooptcopyin(sopt, &background,
			    sizeof(background), sizeof(background));
			if (error) {
				break;
			}

			if (background) {
				socket_set_traffic_mgt_flags_locked(so,
				    TRAFFIC_MGT_SO_BACKGROUND);
			} else {
				socket_clear_traffic_mgt_flags_locked(so,
				    TRAFFIC_MGT_SO_BACKGROUND);
			}

			break;
		}
#endif                  

		  
                                                          
                                                            
                                                             
                                                             
                                                         
                                                             
                                                              
                                                                 
                                                               
                                                              
                                                             
                                                          
                                                           
                                                              
                                                                
                                                 
     
		case IP_BOUND_IF:
			                                           
			if (!(inp->inp_vflag & INP_IPV4)) {
				error = EINVAL;
				break;
			}

			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));

			if (error) {
				break;
			}

			error = inp_bindif(inp, optval, NULL);
			break;

		case IP_NO_IFT_CELLULAR:
			                                           
			if (!(inp->inp_vflag & INP_IPV4)) {
				error = EINVAL;
				break;
			}

			error = sooptcopyin(sopt, &optval, sizeof(optval),
			    sizeof(optval));

			if (error) {
				break;
			}

			                                  
			if (!optval && INP_NO_CELLULAR(inp)) {
				error = EINVAL;
				break;
			}

			error = so_set_restrictions(so,
			    SO_RESTRICT_DENY_CELLULAR);
			break;

		case IP_OUT_IF:
			                                 
			error = EINVAL;
			break;

		default:
			error = ENOPROTOOPT;
			break;
		}
		break;

	case SOPT_GET:
		switch (sopt->sopt_name) {
		case IP_OPTIONS:
		case IP_RETOPTS:
			if (inp->inp_options) {
				error = sooptcopyout(sopt,
				    mtod(inp->inp_options, char *),
				    inp->inp_options->m_len);
			} else {
				sopt->sopt_valsize = 0;
			}
			break;

		case IP_TOS:
		case IP_TTL:
		case IP_RECVOPTS:
		case IP_RECVRETOPTS:
		case IP_RECVDSTADDR:
		case IP_RECVIF:
		case IP_RECVTTL:
		case IP_PORTRANGE:
		case IP_RECVPKTINFO:
		case IP_RECVTOS:
		case IP_DONTFRAG:
			switch (sopt->sopt_name) {
			case IP_TOS:
				optval = inp->inp_ip_tos;
				break;

			case IP_TTL:
				optval = inp->inp_ip_ttl;
				break;

#define OPTBIT(bit)     (inp->inp_flags & bit ? 1 : 0)
#define OPTBIT2(bit)    (inp->inp_flags2 & bit ? 1 : 0)
			case IP_RECVOPTS:
				optval = OPTBIT(INP_RECVOPTS);
				break;

			case IP_RECVRETOPTS:
				optval = OPTBIT(INP_RECVRETOPTS);
				break;

			case IP_RECVDSTADDR:
				optval = OPTBIT(INP_RECVDSTADDR);
				break;

			case IP_RECVIF:
				optval = OPTBIT(INP_RECVIF);
				break;

			case IP_RECVTTL:
				optval = OPTBIT(INP_RECVTTL);
				break;

			case IP_PORTRANGE:
				if (inp->inp_flags & INP_HIGHPORT) {
					optval = IP_PORTRANGE_HIGH;
				} else if (inp->inp_flags & INP_LOWPORT) {
					optval = IP_PORTRANGE_LOW;
				} else {
					optval = 0;
				}
				break;

			case IP_RECVPKTINFO:
				optval = OPTBIT(INP_PKTINFO);
				break;

			case IP_RECVTOS:
				optval = OPTBIT(INP_RECVTOS);
				break;
			case IP_DONTFRAG:
				optval = OPTBIT2(INP2_DONTFRAG);
				break;
			}
			error = sooptcopyout(sopt, &optval, sizeof(optval));
			break;

		case IP_MULTICAST_IF:
		case IP_MULTICAST_IFINDEX:
		case IP_MULTICAST_VIF:
		case IP_MULTICAST_TTL:
		case IP_MULTICAST_LOOP:
		case IP_MSFILTER:
			error = inp_getmoptions(inp, sopt);
			break;

#if IPSEC
		case IP_IPSEC_POLICY: {
			error = 0;                                         
			break;
		}
#endif            

#if TRAFFIC_MGT
		case IP_TRAFFIC_MGT_BACKGROUND: {
			unsigned background = (so->so_flags1 &
			    SOF1_TRAFFIC_MGT_SO_BACKGROUND) ? 1 : 0;
			return sooptcopyout(sopt, &background,
			           sizeof(background));
		}
#endif                  

		case IP_BOUND_IF:
			if (inp->inp_flags & INP_BOUND_IF) {
				optval = inp->inp_boundifp->if_index;
			}
			error = sooptcopyout(sopt, &optval, sizeof(optval));
			break;

		case IP_NO_IFT_CELLULAR:
			optval = INP_NO_CELLULAR(inp) ? 1 : 0;
			error = sooptcopyout(sopt, &optval, sizeof(optval));
			break;

		case IP_OUT_IF:
			optval = (inp->inp_last_outifp != NULL) ?
			    inp->inp_last_outifp->if_index : 0;
			error = sooptcopyout(sopt, &optval, sizeof(optval));
			break;

		default:
			error = ENOPROTOOPT;
			break;
		}
		break;
	}
	return error;
}

  
                                                            
                                                             
                                             
   
static int
ip_pcbopts(int optname, struct mbuf **pcbopt, struct mbuf *m)
{
#pragma unused(optname)
	int cnt, optlen;
	u_char *cp;
	u_char opt;

	                              
	if (*pcbopt) {
		(void) m_free(*pcbopt);
	}
	*pcbopt = 0;
	if (m == (struct mbuf *)0 || m->m_len == 0) {
		  
                                           
     
		if (m) {
			(void) m_free(m);
		}
		return 0;
	}

	if (m->m_len % sizeof(int32_t)) {
		goto bad;
	}

	  
                                                          
                                           
                                   
    
	if (m->m_data + m->m_len + sizeof(struct in_addr) >= &m->m_dat[MLEN]) {
		goto bad;
	}
	cnt = m->m_len;
	m->m_len += sizeof(struct in_addr);
	cp = mtod(m, u_char *) + sizeof(struct in_addr);
	ovbcopy(mtod(m, caddr_t), (caddr_t)cp, (unsigned)cnt);
	bzero(mtod(m, caddr_t), sizeof(struct in_addr));

	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL) {
			break;
		}
		if (opt == IPOPT_NOP) {
			optlen = 1;
		} else {
			if (cnt < IPOPT_OLEN + sizeof(*cp)) {
				goto bad;
			}
			optlen = cp[IPOPT_OLEN];
			if (optlen < IPOPT_OLEN + sizeof(*cp) || optlen > cnt) {
				goto bad;
			}
		}
		switch (opt) {
		default:
			break;

		case IPOPT_LSRR:
		case IPOPT_SSRR:
			  
                                      
                  
                                                   
                                                      
                                                         
                                                         
      
			if (optlen < IPOPT_MINOFF - 1 + sizeof(struct in_addr)) {
				goto bad;
			}
			if (optlen > UINT8_MAX) {
				goto bad;
			}
			m->m_len -= sizeof(struct in_addr);
			cnt -= sizeof(struct in_addr);
			optlen -= sizeof(struct in_addr);
			cp[IPOPT_OLEN] = (uint8_t)optlen;
			  
                                             
      
			bcopy((caddr_t)&cp[IPOPT_OFFSET + 1], mtod(m, caddr_t),
			    sizeof(struct in_addr));
			  
                                    
                                    
      
			ovbcopy((caddr_t)(&cp[IPOPT_OFFSET + 1] +
			    sizeof(struct in_addr)),
			    (caddr_t)&cp[IPOPT_OFFSET + 1],
			    (unsigned)cnt - (IPOPT_MINOFF - 1));
			break;
		}
	}
	if (m->m_len > MAX_IPOPTLEN + sizeof(struct in_addr)) {
		goto bad;
	}
	*pcbopt = m;
	return 0;

bad:
	(void) m_free(m);
	return EINVAL;
}

void
ip_moptions_init(void)
{
	PE_parse_boot_argn("ifa_debug", &imo_debug, sizeof(imo_debug));

	vm_size_t imo_size = (imo_debug == 0) ? sizeof(struct ip_moptions) :
	    sizeof(struct ip_moptions_dbg);

	imo_zone = zone_create(IMO_ZONE_NAME, imo_size, ZC_ZFREE_CLEARMEM);
}

void
imo_addref(struct ip_moptions *imo, int locked)
{
	if (!locked) {
		IMO_LOCK(imo);
	} else {
		IMO_LOCK_ASSERT_HELD(imo);
	}

	if (++imo->imo_refcnt == 0) {
		panic("%s: imo %p wraparound refcnt\n", __func__, imo);
		                
	} else if (imo->imo_trace != NULL) {
		(*imo->imo_trace)(imo, TRUE);
	}

	if (!locked) {
		IMO_UNLOCK(imo);
	}
}

void
imo_remref(struct ip_moptions *imo)
{
	int i;

	IMO_LOCK(imo);
	if (imo->imo_refcnt == 0) {
		panic("%s: imo %p negative refcnt", __func__, imo);
		                
	} else if (imo->imo_trace != NULL) {
		(*imo->imo_trace)(imo, FALSE);
	}

	--imo->imo_refcnt;
	if (imo->imo_refcnt > 0) {
		IMO_UNLOCK(imo);
		return;
	}

	for (i = 0; i < imo->imo_num_memberships; ++i) {
		struct in_mfilter *imf;

		imf = imo->imo_mfilters ? &imo->imo_mfilters[i] : NULL;
		if (imf != NULL) {
			imf_leave(imf);
		}

		(void) in_leavegroup(imo->imo_membership[i], imf);

		if (imf != NULL) {
			imf_purge(imf);
		}

		INM_REMREF(imo->imo_membership[i]);
		imo->imo_membership[i] = NULL;
	}
	imo->imo_num_memberships = 0;
	if (imo->imo_mfilters != NULL) {
		FREE(imo->imo_mfilters, M_INMFILTER);
		imo->imo_mfilters = NULL;
	}
	if (imo->imo_membership != NULL) {
		FREE(imo->imo_membership, M_IPMOPTS);
		imo->imo_membership = NULL;
	}
	IMO_UNLOCK(imo);

	lck_mtx_destroy(&imo->imo_lock, ifa_mtx_grp);

	if (!(imo->imo_debug & IFD_ALLOC)) {
		panic("%s: imo %p cannot be freed", __func__, imo);
		                
	}
	zfree(imo_zone, imo);
}

static void
imo_trace(struct ip_moptions *imo, int refhold)
{
	struct ip_moptions_dbg *imo_dbg = (struct ip_moptions_dbg *)imo;
	ctrace_t *tr;
	u_int32_t idx;
	u_int16_t *cnt;

	if (!(imo->imo_debug & IFD_DEBUG)) {
		panic("%s: imo %p has no debug structure", __func__, imo);
		                
	}
	if (refhold) {
		cnt = &imo_dbg->imo_refhold_cnt;
		tr = imo_dbg->imo_refhold;
	} else {
		cnt = &imo_dbg->imo_refrele_cnt;
		tr = imo_dbg->imo_refrele;
	}

	idx = atomic_add_16_ov(cnt, 1) % IMO_TRACE_HIST_SIZE;
	ctrace_record(&tr[idx]);
}

struct ip_moptions *
ip_allocmoptions(zalloc_flags_t how)
{
	struct ip_moptions *imo;

	imo = zalloc_flags(imo_zone, how | Z_ZERO);
	if (imo != NULL) {
		lck_mtx_init(&imo->imo_lock, ifa_mtx_grp, ifa_mtx_attr);
		imo->imo_debug |= IFD_ALLOC;
		if (imo_debug != 0) {
			imo->imo_debug |= IFD_DEBUG;
			imo->imo_trace = imo_trace;
		}
		IMO_ADDREF(imo);
	}

	return imo;
}

  
                                                                         
                                                                      
                                                                           
                                                                          
                              
   
static void
ip_mloopback(struct ifnet *srcifp, struct ifnet *origifp, struct mbuf *m,
    struct sockaddr_in *dst, int hlen)
{
	struct mbuf *copym;
	struct ip *ip;

	if (lo_ifp == NULL) {
		return;
	}

	  
                                                          
                                                             
                                                                
                         
    
	copym = m_copym_mode(m, 0, M_COPYALL, M_DONTWAIT, M_COPYM_COPY_HDR);
	if (copym != NULL && ((copym->m_flags & M_EXT) || copym->m_len < hlen)) {
		copym = m_pullup(copym, hlen);
	}

	if (copym == NULL) {
		return;
	}

	  
                                                           
                                                        
    
	ip = mtod(copym, struct ip *);
#if BYTE_ORDER != BIG_ENDIAN
	HTONS(ip->ip_len);
	HTONS(ip->ip_off);
#endif
	ip->ip_sum = 0;
	ip->ip_sum = ip_cksum_hdr_out(copym, hlen);

	  
                                                             
                                                          
                                                                
    
	if (hwcksum_rx) {
		copym->m_pkthdr.csum_flags &= ~(CSUM_PARTIAL | CSUM_ZERO_INVERT);
		copym->m_pkthdr.csum_flags |=
		    CSUM_DATA_VALID | CSUM_PSEUDO_HDR;
		copym->m_pkthdr.csum_data = 0xffff;
	} else if (copym->m_pkthdr.csum_flags & CSUM_DELAY_DATA) {
#if BYTE_ORDER != BIG_ENDIAN
		NTOHS(ip->ip_len);
#endif
		in_delayed_cksum(copym);
#if BYTE_ORDER != BIG_ENDIAN
		HTONS(ip->ip_len);
#endif
	}

	  
                                                                
                                                                 
                                                            
                 
    
	copym->m_pkthdr.rcvif = origifp;

	  
                                                                     
                                                               
    
	if (srcifp == NULL) {
		struct in_ifaddr *ia;

		lck_rw_lock_shared(in_ifaddr_rwlock);
		TAILQ_FOREACH(ia, INADDR_HASH(ip->ip_src.s_addr), ia_hash) {
			IFA_LOCK_SPIN(&ia->ia_ifa);
			if (IA_SIN(ia)->sin_addr.s_addr == ip->ip_src.s_addr) {
				srcifp = ia->ia_ifp;
				IFA_UNLOCK(&ia->ia_ifa);
				break;
			}
			IFA_UNLOCK(&ia->ia_ifa);
		}
		lck_rw_done(in_ifaddr_rwlock);
	}
	if (srcifp != NULL) {
		ip_setsrcifaddr_info(copym, srcifp->if_index, NULL);
	}
	ip_setdstifaddr_info(copym, origifp->if_index, NULL);

	dlil_output(lo_ifp, PF_INET, copym, NULL, SA(dst), 0, NULL);
}

  
                                                                          
                                                                      
                                                                        
                                                                        
                                                                        
                                     
  
                                                           
   
static struct ifaddr *
in_selectsrcif(struct ip *ip, struct route *ro, unsigned int ifscope)
{
	struct ifaddr *ifa = NULL;
	struct in_addr src = ip->ip_src;
	struct in_addr dst = ip->ip_dst;
	struct ifnet *rt_ifp;
	char s_src[MAX_IPv4_STR_LEN], s_dst[MAX_IPv4_STR_LEN];

	VERIFY(src.s_addr != INADDR_ANY);

	if (ip_select_srcif_debug) {
		(void) inet_ntop(AF_INET, &src.s_addr, s_src, sizeof(s_src));
		(void) inet_ntop(AF_INET, &dst.s_addr, s_dst, sizeof(s_dst));
	}

	if (ro->ro_rt != NULL) {
		RT_LOCK(ro->ro_rt);
	}

	rt_ifp = (ro->ro_rt != NULL) ? ro->ro_rt->rt_ifp : NULL;

	  
                                                                 
                                                                 
                                                                 
                                                                
                                    
    
	if (ifscope != IFSCOPE_NONE || ro->ro_rt != NULL) {
		unsigned int scope = ifscope;

		  
                                                              
                                                               
                                                              
                                                           
                                                             
                                                              
                                               
     
		if (scope == IFSCOPE_NONE) {
			scope = rt_ifp->if_index;
			if (scope != get_primary_ifscope(AF_INET) &&
			    ROUTE_UNUSABLE(ro)) {
				scope = get_primary_ifscope(AF_INET);
			}
		}

		ifa = (struct ifaddr *)ifa_foraddr_scoped(src.s_addr, scope);

		if (ifa == NULL && ip->ip_p != IPPROTO_UDP &&
		    ip->ip_p != IPPROTO_TCP && ipforwarding) {
			  
                                                       
                                                     
                                                     
                                                        
      
			ifa = (struct ifaddr *)ifa_foraddr(src.s_addr);
			if (ifa != NULL) {
				IFA_REMREF(ifa);
				ifa = NULL;
				ifscope = IFSCOPE_NONE;
			}
		}

		if (ip_select_srcif_debug && ifa != NULL) {
			if (ro->ro_rt != NULL) {
				printf("%s->%s ifscope %d->%d ifa_if %s "
				    "ro_if %s\n", s_src, s_dst, ifscope,
				    scope, if_name(ifa->ifa_ifp),
				    if_name(rt_ifp));
			} else {
				printf("%s->%s ifscope %d->%d ifa_if %s\n",
				    s_src, s_dst, ifscope, scope,
				    if_name(ifa->ifa_ifp));
			}
		}
	}

	  
                                                                      
                                                                 
   
                                             
                                                                 
                                                           
                                                              
                    
    
	if (ifa == NULL && ifscope == IFSCOPE_NONE) {
		ifa = (struct ifaddr *)ifa_foraddr(src.s_addr);

		  
                                                           
                                                         
                                                               
                                                      
     
		if (ifa != NULL && ro->ro_rt == NULL) {
			struct rtentry *rt;
			struct sockaddr_in sin;
			struct ifaddr *oifa = NULL;

			bzero(&sin, sizeof(sin));
			sin.sin_family = AF_INET;
			sin.sin_len = sizeof(sin);
			sin.sin_addr = dst;

			lck_mtx_lock(rnh_lock);
			if ((rt = rt_lookup(TRUE, SA(&sin), NULL,
			    rt_tables[AF_INET], IFSCOPE_NONE)) != NULL) {
				RT_LOCK(rt);
				  
                                               
                                               
                                             
                
       
				if (ifa->ifa_ifp != rt->rt_ifp) {
					oifa = ifa;
					ifa = rt->rt_ifa;
					IFA_ADDREF(ifa);
					RT_UNLOCK(rt);
				} else {
					RT_UNLOCK(rt);
				}
				rtfree_locked(rt);
			}
			lck_mtx_unlock(rnh_lock);

			if (oifa != NULL) {
				struct ifaddr *iifa;

				  
                                             
                                             
                             
       
				iifa = (struct ifaddr *)ifa_foraddr_scoped(
					src.s_addr, ifa->ifa_ifp->if_index);

				if (iifa != NULL) {
					  
                                       
                                      
                                      
        
					IFA_REMREF(oifa);
					IFA_REMREF(ifa);
					ifa = iifa;
				} else if (!ipforwarding ||
				    (rt->rt_flags & RTF_GATEWAY)) {
					  
                                        
                                         
                                          
                                        
                                 
        
					IFA_REMREF(ifa);
					ifa = oifa;
				} else {
					  
                                            
                                         
                                           
                                           
                                            
                                           
                                          
                                  
        
					IFA_REMREF(oifa);
				}
			}
		} else if (ifa != NULL && ro->ro_rt != NULL &&
		    !(ro->ro_rt->rt_flags & RTF_GATEWAY) &&
		    ifa->ifa_ifp != ro->ro_rt->rt_ifp && ipforwarding) {
			  
                                                          
                                                       
                                                        
                                                       
      
			IFA_REMREF(ifa);
			ifa = ro->ro_rt->rt_ifa;
			IFA_ADDREF(ifa);
		}

		if (ip_select_srcif_debug && ifa != NULL) {
			printf("%s->%s ifscope %d ifa_if %s\n",
			    s_src, s_dst, ifscope, if_name(ifa->ifa_ifp));
		}
	}

	if (ro->ro_rt != NULL) {
		RT_LOCK_ASSERT_HELD(ro->ro_rt);
	}
	  
                                                                    
                                                                  
                                                                     
                          
    
	if (ro->ro_rt != NULL &&
	    (ifa == NULL || (ifa->ifa_ifp != rt_ifp && rt_ifp != lo_ifp) ||
	    !(ro->ro_rt->rt_flags & RTF_UP))) {
		if (ip_select_srcif_debug) {
			if (ifa != NULL) {
				printf("%s->%s ifscope %d ro_if %s != "
				    "ifa_if %s (cached route cleared)\n",
				    s_src, s_dst, ifscope, if_name(rt_ifp),
				    if_name(ifa->ifa_ifp));
			} else {
				printf("%s->%s ifscope %d ro_if %s "
				    "(no ifa_if found)\n",
				    s_src, s_dst, ifscope, if_name(rt_ifp));
			}
		}

		RT_UNLOCK(ro->ro_rt);
		ROUTE_RELEASE(ro);

		  
                                                             
                                                           
                                                            
                                                            
                                                             
                                                           
                                                              
                                                        
     
		if (IN_LINKLOCAL(ntohl(dst.s_addr)) &&
		    !IN_LINKLOCAL(ntohl(src.s_addr)) && ifa != NULL) {
			IFA_REMREF(ifa);
			ifa = NULL;
		}
	}

	if (ip_select_srcif_debug && ifa == NULL) {
		printf("%s->%s ifscope %d (neither ro_if/ifa_if found)\n",
		    s_src, s_dst, ifscope);
	}

	  
                                                                  
                                                                  
                                                                 
                                                                   
                                                                   
                                                         
    
	if (ro->ro_rt != NULL && (!IN_LINKLOCAL(ntohl(dst.s_addr)) ||
	    (ro->ro_rt->rt_gateway->sa_family == AF_LINK &&
	    SDL(ro->ro_rt->rt_gateway)->sdl_alen != 0))) {
		if (ifa != NULL) {
			IFA_ADDREF(ifa);                       
		}
		if (ro->ro_srcia != NULL) {
			IFA_REMREF(ro->ro_srcia);
		}
		ro->ro_srcia = ifa;
		ro->ro_flags |= ROF_SRCIF_SELECTED;
		RT_GENID_SYNC(ro->ro_rt);
	}

	if (ro->ro_rt != NULL) {
		RT_UNLOCK(ro->ro_rt);
	}

	return ifa;
}

  
                                                                    
                                                                        
                  
  
                                               
                                 
                               
                                                              
                                                         
  
               
   
void
ip_output_checksum(struct ifnet *ifp, struct mbuf *m, int hlen, int ip_len,
    uint32_t *sw_csum)
{
	int tso = TSO_IPV4_OK(ifp, m);
	uint32_t hwcap = ifp->if_hwassist;

	m->m_pkthdr.csum_flags |= CSUM_IP;

	if (!hwcksum_tx) {
		                                                               
		*sw_csum = (CSUM_DELAY_DATA | CSUM_DELAY_IP) &
		    m->m_pkthdr.csum_flags;
	} else {
		                                             
		*sw_csum = m->m_pkthdr.csum_flags &
		    ~IF_HWASSIST_CSUM_FLAGS(hwcap);
	}

	if (hlen != sizeof(struct ip)) {
		*sw_csum |= ((CSUM_DELAY_DATA | CSUM_DELAY_IP) &
		    m->m_pkthdr.csum_flags);
	} else if (!(*sw_csum & CSUM_DELAY_DATA) && (hwcap & CSUM_PARTIAL)) {
		int interface_mtu = ifp->if_mtu;

		if (INTF_ADJUST_MTU_FOR_CLAT46(ifp)) {
			interface_mtu = IN6_LINKMTU(ifp);
			                                                  
			interface_mtu -= CLAT46_HDR_EXPANSION_OVERHD;
		}

		  
                                                               
                                                                
                                                                
                                        
     
		if (hwcksum_tx && !tso &&
		    ((m->m_pkthdr.csum_flags & CSUM_TCP) ||
		    ((hwcap & CSUM_ZERO_INVERT) &&
		    (m->m_pkthdr.csum_flags & CSUM_ZERO_INVERT))) &&
		    ip_len <= interface_mtu) {
			uint16_t start = sizeof(struct ip);
			uint16_t ulpoff = m->m_pkthdr.csum_data & 0xffff;
			m->m_pkthdr.csum_flags |=
			    (CSUM_DATA_VALID | CSUM_PARTIAL);
			m->m_pkthdr.csum_tx_stuff = (ulpoff + start);
			m->m_pkthdr.csum_tx_start = start;
			                                  
			*sw_csum = CSUM_DELAY_IP;
		} else {
			*sw_csum |= (CSUM_DELAY_DATA & m->m_pkthdr.csum_flags);
		}
	}

	if (*sw_csum & CSUM_DELAY_DATA) {
		in_delayed_cksum(m);
		*sw_csum &= ~CSUM_DELAY_DATA;
	}

	if (hwcksum_tx) {
		  
                                                     
                                                          
     
		m->m_pkthdr.csum_flags =
		    ((m->m_pkthdr.csum_flags &
		    (IF_HWASSIST_CSUM_FLAGS(hwcap) | CSUM_DATA_VALID)) |
		    (m->m_pkthdr.csum_flags & ~IF_HWASSIST_CSUM_MASK));
	} else {
		                                                          
		m->m_pkthdr.csum_flags = 0;
	}
}

  
                                   
   
int
ip_gre_output(struct mbuf *m)
{
	struct route ro;
	int error;

	bzero(&ro, sizeof(ro));

	error = ip_output(m, NULL, &ro, 0, NULL, NULL);

	ROUTE_RELEASE(&ro);

	return error;
}

static int
sysctl_reset_ip_output_stats SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error, i;

	i = ip_output_measure;
	error = sysctl_handle_int(oidp, &i, 0, req);
	if (error || req->newptr == USER_ADDR_NULL) {
		goto done;
	}
	                   
	if (i < 0 || i > 1) {
		error = EINVAL;
		goto done;
	}
	if (ip_output_measure != i && i == 1) {
		net_perf_initialize(&net_perf, ip_output_measure_bins);
	}
	ip_output_measure = i;
done:
	return error;
}

static int
sysctl_ip_output_measure_bins SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error;
	uint64_t i;

	i = ip_output_measure_bins;
	error = sysctl_handle_quad(oidp, &i, 0, req);
	if (error || req->newptr == USER_ADDR_NULL) {
		goto done;
	}
	                   
	if (!net_perf_validate_bins(i)) {
		error = EINVAL;
		goto done;
	}
	ip_output_measure_bins = i;
done:
	return error;
}

static int
sysctl_ip_output_getperf SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	if (req->oldptr == USER_ADDR_NULL) {
		req->oldlen = (size_t)sizeof(struct ipstat);
	}

	return SYSCTL_OUT(req, &net_perf, MIN(sizeof(net_perf), req->oldlen));
}
