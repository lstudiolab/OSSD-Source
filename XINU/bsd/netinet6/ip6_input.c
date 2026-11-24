/*
 * Copyright (c) 2003-2020 Apple Inc. All rights reserved.
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
 * Copyright (c) 1982, 1986, 1988, 1993
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
 *	@(#)ip_input.c	8.2 (Berkeley) 1/4/94
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/kauth.h>
#include <sys/mcache.h>

#include <mach/mach_time.h>
#include <mach/sdt.h>
#include <pexpert/pexpert.h>
#include <dev/random/randomdev.h>

#include <net/if.h>
#include <net/if_var.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <net/kpi_protocol.h>
#include <net/ntstat.h>
#include <net/init.h>
#include <net/net_osdep.h>
#include <net/net_perf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#if INET
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#endif           
#include <netinet/kpi_ipfilter_var.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <netinet6/in6_var.h>
#include <netinet6/ip6_var.h>
#include <netinet/in_pcb.h>
#include <netinet/icmp6.h>
#include <netinet6/in6_ifattach.h>
#include <netinet6/nd6.h>
#include <netinet6/scope6_var.h>
#include <netinet6/ip6protosw.h>

#if IPSEC
#include <netinet6/ipsec.h>
#include <netinet6/ipsec6.h>
extern int ipsec_bypass;
#endif            

#if DUMMYNET
#include <netinet/ip_dummynet.h>
#endif               

                           
#include "loop.h"

#if PF
#include <net/pfvar.h>
#endif         

#include <os/log.h>

struct ip6protosw *ip6_protox[IPPROTO_MAX];

static lck_grp_attr_t   *in6_ifaddr_rwlock_grp_attr;
static lck_grp_t        *in6_ifaddr_rwlock_grp;
static lck_attr_t       *in6_ifaddr_rwlock_attr;
decl_lck_rw_data(, in6_ifaddr_rwlock);

                                    
struct in6_ifaddrhead in6_ifaddrhead;
struct in6_ifaddrhashhead * in6_ifaddrhashtbl;
uint32_t in6_ifaddrhmask;

#define IN6ADDR_NHASH    61
u_int32_t in6addr_nhash = 0;                                       
u_int32_t in6addr_hashp = 0;                                          


#define IN6_IFSTAT_REQUIRE_ALIGNED_64(f)        \
	_CASSERT(!(offsetof(struct in6_ifstat, f) % sizeof (uint64_t)))

#define ICMP6_IFSTAT_REQUIRE_ALIGNED_64(f)      \
	_CASSERT(!(offsetof(struct icmp6_ifstat, f) % sizeof (uint64_t)))

struct ip6stat ip6stat;

decl_lck_mtx_data(, proxy6_lock);
decl_lck_mtx_data(static, dad6_mutex_data);
decl_lck_mtx_data(static, nd6_mutex_data);
decl_lck_mtx_data(static, prefix6_mutex_data);
lck_mtx_t               *dad6_mutex = &dad6_mutex_data;
lck_mtx_t               *nd6_mutex = &nd6_mutex_data;
lck_mtx_t               *prefix6_mutex = &prefix6_mutex_data;
#ifdef ENABLE_ADDRSEL
decl_lck_mtx_data(static, addrsel_mutex_data);
lck_mtx_t               *addrsel_mutex = &addrsel_mutex_data;
#endif
static lck_attr_t       *ip6_mutex_attr;
static lck_grp_t        *ip6_mutex_grp;
static lck_grp_attr_t   *ip6_mutex_grp_attr;

extern int loopattach_done;
extern void addrsel_policy_init(void);

static int sysctl_reset_ip6_input_stats SYSCTL_HANDLER_ARGS;
static int sysctl_ip6_input_measure_bins SYSCTL_HANDLER_ARGS;
static int sysctl_ip6_input_getperf SYSCTL_HANDLER_ARGS;
static void ip6_init_delayed(void);
static int ip6_hopopts_input(u_int32_t *, u_int32_t *, struct mbuf **, int *);

static void in6_ifaddrhashtbl_init(void);

#if NSTF
extern void stfattach(void);
#endif           

SYSCTL_DECL(_net_inet6_ip6);

static uint32_t ip6_adj_clear_hwcksum = 0;
SYSCTL_UINT(_net_inet6_ip6, OID_AUTO, adj_clear_hwcksum,
    CTLFLAG_RW | CTLFLAG_LOCKED, &ip6_adj_clear_hwcksum, 0,
    "Invalidate hwcksum info when adjusting length");

static uint32_t ip6_adj_partial_sum = 1;
SYSCTL_UINT(_net_inet6_ip6, OID_AUTO, adj_partial_sum,
    CTLFLAG_RW | CTLFLAG_LOCKED, &ip6_adj_partial_sum, 0,
    "Perform partial sum adjustment of trailing bytes at IP layer");

static int ip6_input_measure = 0;
SYSCTL_PROC(_net_inet6_ip6, OID_AUTO, input_perf,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &ip6_input_measure, 0, sysctl_reset_ip6_input_stats, "I", "Do time measurement");

static uint64_t ip6_input_measure_bins = 0;
SYSCTL_PROC(_net_inet6_ip6, OID_AUTO, input_perf_bins,
    CTLTYPE_QUAD | CTLFLAG_RW | CTLFLAG_LOCKED, &ip6_input_measure_bins, 0,
    sysctl_ip6_input_measure_bins, "I",
    "bins for chaining performance data histogram");

static net_perf_t net_perf;
SYSCTL_PROC(_net_inet6_ip6, OID_AUTO, input_perf_data,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED,
    0, 0, sysctl_ip6_input_getperf, "S,net_perf",
    "IP6 input performance data (struct net_perf, net/net_perf.h)");

  
                                                                             
                                  
  
                                        
                               
                                                                            
                                                                                     
                                 
  
                                 
                                                                              
                                                                                
                                                                         
                                 
  
                                 
                                                                          
                                                     
  
                                                                                            
                                                                                                    
  
                                                                                  
                                                                                        
   
#define IP6_CHECKINTERFACE_WEAK_ES       0
#define IP6_CHECKINTERFACE_HYBRID_ES     1
#define IP6_CHECKINTERFACE_STRONG_ES     2

static int ip6_checkinterface = IP6_CHECKINTERFACE_HYBRID_ES;

static int sysctl_ip6_checkinterface SYSCTL_HANDLER_ARGS;
SYSCTL_PROC(_net_inet6_ip6, OID_AUTO, check_interface,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    0, 0, sysctl_ip6_checkinterface, "I", "Verify packet arrives on correct interface");

#if (DEBUG || DEVELOPMENT)
#define IP6_CHECK_IFDEBUG 1
#else
#define IP6_CHECK_IFDEBUG 0
#endif                             
static int ip6_checkinterface_debug = IP6_CHECK_IFDEBUG;
SYSCTL_INT(_net_inet6_ip6, OID_AUTO, checkinterface_debug, CTLFLAG_RW | CTLFLAG_LOCKED,
    &ip6_checkinterface_debug, IP6_CHECK_IFDEBUG, "");

typedef enum ip6_check_if_result {
	IP6_CHECK_IF_NONE = 0,
	IP6_CHECK_IF_OURS = 1,
	IP6_CHECK_IF_DROP = 2,
	IP6_CHECK_IF_FORWARD = 3
} ip6_check_if_result_t;

static ip6_check_if_result_t ip6_input_check_interface(struct mbuf *, struct ip6_hdr *, struct ifnet *, struct route_in6 *rin6, struct ifnet **);

  
                                                                          
                                                                             
                                                                              
                                                                          
                        
  
                                                                             
                                                                               
                                                       
   
#if defined(__i386__) || defined(__x86_64__)
#define IP6_HDR_ALIGNMENT_FIXUP(_m, _ifp, _action) do { } while (0)
#else                               
#define IP6_HDR_ALIGNMENT_FIXUP(_m, _ifp, _action) do {                 \
	if (!IP6_HDR_ALIGNED_P(mtod(_m, caddr_t))) {                    \
	        struct mbuf *_n;                                        \
	        struct ifnet *__ifp = (_ifp);                           \
	        atomic_add_64(&(__ifp)->if_alignerrs, 1);               \
	        if (((_m)->m_flags & M_PKTHDR) &&                       \
	            (_m)->m_pkthdr.pkt_hdr != NULL)                     \
	                (_m)->m_pkthdr.pkt_hdr = NULL;                  \
	        _n = m_defrag_offset(_m, max_linkhdr, M_NOWAIT);        \
	        if (_n == NULL) {                                       \
	                ip6stat.ip6s_toosmall++;                        \
	                m_freem(_m);                                    \
	                (_m) = NULL;                                    \
	                _action;                                        \
	        } else {                                                \
	                VERIFY(_n != (_m));                             \
	                (_m) = _n;                                      \
	        }                                                       \
	}                                                               \
} while (0)
#endif                               

static void
ip6_proto_input(protocol_family_t protocol, mbuf_t packet)
{
#pragma unused(protocol)
#if INET
	struct timeval start_tv;
	if (ip6_input_measure) {
		net_perf_start_time(&net_perf, &start_tv);
	}
#endif           
	ip6_input(packet);
#if INET
	if (ip6_input_measure) {
		net_perf_measure_time(&net_perf, &start_tv, 1);
		net_perf_histogram(&net_perf, 1);
	}
#endif           
}

  
                                                         
                                                                          
   
void
ip6_init(struct ip6protosw *pp, struct domain *dp)
{
	static int ip6_initialized = 0;
	struct protosw *pr;
	struct timeval tv;
	int i;
	domain_unguard_t unguard;

	domain_proto_mtx_lock_assert_held();
	VERIFY((pp->pr_flags & (PR_INITIALIZED | PR_ATTACHED)) == PR_ATTACHED);

	_CASSERT((sizeof(struct ip6_hdr) +
	    sizeof(struct icmp6_hdr)) <= _MHLEN);

	if (ip6_initialized) {
		return;
	}
	ip6_initialized = 1;

	eventhandler_lists_ctxt_init(&in6_evhdlr_ctxt);
	(void)EVENTHANDLER_REGISTER(&in6_evhdlr_ctxt, in6_event,
	    in6_eventhdlr_callback, eventhandler_entry_dummy_arg,
	    EVENTHANDLER_PRI_ANY);

	eventhandler_lists_ctxt_init(&in6_clat46_evhdlr_ctxt);
	(void)EVENTHANDLER_REGISTER(&in6_clat46_evhdlr_ctxt, in6_clat46_event,
	    in6_clat46_eventhdlr_callback, eventhandler_entry_dummy_arg,
	    EVENTHANDLER_PRI_ANY);

	for (i = 0; i < IN6_EVENT_MAX; i++) {
		VERIFY(in6_event2kev_array[i].in6_event_code == i);
	}

	pr = pffindproto_locked(PF_INET6, IPPROTO_RAW, SOCK_RAW);
	if (pr == NULL) {
		panic("%s: Unable to find [PF_INET6,IPPROTO_RAW,SOCK_RAW]\n",
		    __func__);
		                
	}

	                                                              
	for (i = 0; i < IPPROTO_MAX; i++) {
		ip6_protox[i] = (struct ip6protosw *)pr;
	}
	  
                                                                      
                                                         
    
	VERIFY(dp == inet6domain && dp->dom_family == PF_INET6);
	TAILQ_FOREACH(pr, &dp->dom_protosw, pr_entry) {
		VERIFY(pr->pr_domain == dp);
		if (pr->pr_protocol != 0 && pr->pr_protocol != IPPROTO_RAW) {
			                                                  
			if (pr->pr_protocol < IPPROTO_MAX) {
				ip6_protox[pr->pr_protocol] =
				    (struct ip6protosw *)pr;
			}
		}
	}

	ip6_mutex_grp_attr  = lck_grp_attr_alloc_init();

	ip6_mutex_grp = lck_grp_alloc_init("ip6", ip6_mutex_grp_attr);
	ip6_mutex_attr = lck_attr_alloc_init();

	lck_mtx_init(dad6_mutex, ip6_mutex_grp, ip6_mutex_attr);
	lck_mtx_init(nd6_mutex, ip6_mutex_grp, ip6_mutex_attr);
	lck_mtx_init(prefix6_mutex, ip6_mutex_grp, ip6_mutex_attr);
	scope6_init(ip6_mutex_grp, ip6_mutex_attr);

#ifdef ENABLE_ADDRSEL
	lck_mtx_init(addrsel_mutex, ip6_mutex_grp, ip6_mutex_attr);
#endif

	lck_mtx_init(&proxy6_lock, ip6_mutex_grp, ip6_mutex_attr);

	in6_ifaddr_rwlock_grp_attr = lck_grp_attr_alloc_init();
	in6_ifaddr_rwlock_grp = lck_grp_alloc_init("in6_ifaddr_rwlock",
	    in6_ifaddr_rwlock_grp_attr);
	in6_ifaddr_rwlock_attr = lck_attr_alloc_init();
	lck_rw_init(&in6_ifaddr_rwlock, in6_ifaddr_rwlock_grp,
	    in6_ifaddr_rwlock_attr);

	TAILQ_INIT(&in6_ifaddrhead);
	in6_ifaddrhashtbl_init();

	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_receive);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_hdrerr);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_toobig);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_noroute);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_addrerr);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_protounknown);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_truncated);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_discard);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_deliver);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_forward);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_request);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_discard);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_fragok);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_fragfail);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_fragcreat);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_reass_reqd);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_reass_ok);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_reass_fail);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_mcast);
	IN6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_mcast);

	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_msg);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_error);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_dstunreach);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_adminprohib);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_timeexceed);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_paramprob);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_pkttoobig);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_echo);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_echoreply);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_routersolicit);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_routeradvert);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_neighborsolicit);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_neighboradvert);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_redirect);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_mldquery);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_mldreport);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_in_mlddone);

	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_msg);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_error);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_dstunreach);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_adminprohib);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_timeexceed);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_paramprob);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_pkttoobig);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_echo);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_echoreply);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_routersolicit);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_routeradvert);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_neighborsolicit);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_neighboradvert);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_redirect);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_mldquery);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_mldreport);
	ICMP6_IFSTAT_REQUIRE_ALIGNED_64(ifs6_out_mlddone);

	getmicrotime(&tv);
	ip6_desync_factor =
	    (RandomULong() ^ tv.tv_usec) % MAX_TEMP_DESYNC_FACTOR;

	PE_parse_boot_argn("ip6_checkinterface", &i, sizeof(i));
	switch (i) {
	case IP6_CHECKINTERFACE_WEAK_ES:
	case IP6_CHECKINTERFACE_HYBRID_ES:
	case IP6_CHECKINTERFACE_STRONG_ES:
		ip6_checkinterface = i;
		break;
	default:
		break;
	}

	in6_ifaddr_init();
	ip6_moptions_init();
	nd6_init();
	frag6_init();
	icmp6_init(NULL, dp);
	addrsel_policy_init();

	  
                                                                
                                                                    
                                                              
    
	net_init_add(ip6_init_delayed);

	unguard = domain_unguard_deploy();
	i = proto_register_input(PF_INET6, ip6_proto_input, NULL, 0);
	if (i != 0) {
		panic("%s: failed to register PF_INET6 protocol: %d\n",
		    __func__, i);
		                
	}
	domain_unguard_release(unguard);
}

static void
ip6_init_delayed(void)
{
	(void) in6_ifattach_prelim(lo_ifp);

	                                                                  
	timeout(in6_tmpaddrtimer, NULL,
	    (ip6_temp_preferred_lifetime - ip6_desync_factor -
	    ip6_temp_regen_advance) * hz);

#if NSTF
	stfattach();
#endif           
}

static void
ip6_input_adjust(struct mbuf *m, struct ip6_hdr *ip6, uint32_t plen,
    struct ifnet *inifp)
{
	boolean_t adjust = TRUE;
	uint32_t tot_len = sizeof(*ip6) + plen;

	ASSERT(m_pktlen(m) > tot_len);

	  
                                                              
                                                           
                                                            
                          
    
	if (ip6_adj_clear_hwcksum &&
	    (m->m_pkthdr.csum_flags & CSUM_DATA_VALID) &&
	    !(inifp->if_flags & IFF_LOOPBACK) &&
	    !(m->m_pkthdr.pkt_flags & PKTF_LOOP)) {
		m->m_pkthdr.csum_flags &= ~CSUM_DATA_VALID;
		m->m_pkthdr.csum_data = 0;
		ip6stat.ip6s_adj_hwcsum_clr++;
	}

	  
                                                          
                                                           
                                                          
                                                            
                                                      
    
	if (ip6_adj_partial_sum &&
	    (m->m_pkthdr.csum_flags & (CSUM_DATA_VALID | CSUM_PARTIAL)) ==
	    (CSUM_DATA_VALID | CSUM_PARTIAL)) {
		m->m_pkthdr.csum_rx_val = m_adj_sum16(m,
		    m->m_pkthdr.csum_rx_start, m->m_pkthdr.csum_rx_start,
		    (tot_len - m->m_pkthdr.csum_rx_start),
		    m->m_pkthdr.csum_rx_val);
	} else if ((m->m_pkthdr.csum_flags &
	    (CSUM_DATA_VALID | CSUM_PARTIAL)) ==
	    (CSUM_DATA_VALID | CSUM_PARTIAL)) {
		  
                                                           
                                                         
                                                          
                                                            
                                                            
                                                        
                                                            
                                         
     
		if (ip6->ip6_nxt == IPPROTO_TCP ||
		    ip6->ip6_nxt == IPPROTO_UDP) {
			adjust = FALSE;
		} else {
			m->m_pkthdr.csum_flags &= ~CSUM_DATA_VALID;
			m->m_pkthdr.csum_data = 0;
			ip6stat.ip6s_adj_hwcsum_clr++;
		}
	}

	if (adjust) {
		ip6stat.ip6s_adj++;
		if (m->m_len == m->m_pkthdr.len) {
			m->m_len = tot_len;
			m->m_pkthdr.len = tot_len;
		} else {
			m_adj(m, tot_len - m->m_pkthdr.len);
		}
	}
}
static ip6_check_if_result_t
ip6_input_check_interface(struct mbuf *m, struct ip6_hdr *ip6, struct ifnet *inifp, struct route_in6 *rin6, struct ifnet **deliverifp)
{
	struct in6_ifaddr *ia6 = NULL;
	struct in6_addr tmp_dst = ip6->ip6_dst;                                     
	struct in6_ifaddr *best_ia6 = NULL;
	ip6_check_if_result_t result = IP6_CHECK_IF_NONE;

	*deliverifp = NULL;

	  
                                                 
    
	lck_rw_lock_shared(&in6_ifaddr_rwlock);
	TAILQ_FOREACH(ia6, IN6ADDR_HASH(&tmp_dst), ia6_hash) {
		  
                                    
     
		if (IN6_ARE_ADDR_EQUAL(&ia6->ia_addr.sin6_addr, &tmp_dst)) {
			if ((ia6->ia6_flags & (IN6_IFF_NOTREADY | IN6_IFF_CLAT46))) {
				continue;
			}
			best_ia6 = ia6;
			if (ia6->ia_ifp == inifp) {
				  
                                                             
                           
       
				break;
			}
			  
                                                                  
               
      
		}
	}
	if (best_ia6 != NULL) {
		if (best_ia6->ia_ifp != inifp && ip6_forwarding == 0 &&
		    ((ip6_checkinterface == IP6_CHECKINTERFACE_HYBRID_ES &&
		    (best_ia6->ia_ifp->if_family == IFNET_FAMILY_IPSEC ||
		    best_ia6->ia_ifp->if_family == IFNET_FAMILY_UTUN)) ||
		    ip6_checkinterface == IP6_CHECKINTERFACE_STRONG_ES)) {
			  
                                                                
                 
      
			result = IP6_CHECK_IF_DROP;
		} else {
			result = IP6_CHECK_IF_OURS;
			*deliverifp = best_ia6->ia_ifp;
			ip6_setdstifaddr_info(m, 0, best_ia6);
		}
	}
	lck_rw_done(&in6_ifaddr_rwlock);

	if (result == IP6_CHECK_IF_NONE) {
		  
                             
     
		struct sockaddr_in6 *dst6;

		dst6 = SIN6(&rin6->ro_dst);
		dst6->sin6_len = sizeof(struct sockaddr_in6);
		dst6->sin6_family = AF_INET6;
		dst6->sin6_addr = ip6->ip6_dst;

		rtalloc_scoped_ign((struct route *)rin6,
		    RTF_PRCLONING, IFSCOPE_NONE);
		if (rin6->ro_rt != NULL) {
			RT_LOCK_SPIN(rin6->ro_rt);
		}

#define rt6_key(r) (SIN6((r)->rt_nodes->rn_key))

		  
                                                                     
                                                              
                                               
                                                                    
                                                                       
                                                                      
                                                          
    
                                                                         
                                                                    
                                                                  
                                                                         
                                                                      
                                                                      
                                                                       
                                                                     
                                                                      
                                  
     
		if (rin6->ro_rt != NULL &&
		    (rin6->ro_rt->rt_flags & (RTF_HOST | RTF_GATEWAY)) == RTF_HOST &&
#if RTF_WASCLONED
		    !(rin6->ro_rt->rt_flags & RTF_WASCLONED) &&
#endif
		    rin6->ro_rt->rt_ifp->if_type == IFT_LOOP) {
			ia6 = (struct in6_ifaddr *)rin6->ro_rt->rt_ifa;
			  
                                                            
                                   
     
                                                               
                                                           
                            
      
			if (!(ia6->ia6_flags & IN6_IFF_NOTREADY)) {
				                           
				result = IP6_CHECK_IF_OURS;
				*deliverifp = ia6->ia_ifp;                     
				  
                                                
       
				(void) ip6_setdstifaddr_info(m, 0, ia6);
			}
		}

		if (rin6->ro_rt != NULL) {
			RT_UNLOCK(rin6->ro_rt);
		}
	}

	if (result == IP6_CHECK_IF_NONE) {
		if (ip6_forwarding == 0) {
			result = IP6_CHECK_IF_DROP;
		} else {
			result = IP6_CHECK_IF_FORWARD;
			ip6_setdstifaddr_info(m, inifp->if_index, NULL);
		}
	}

	if (result == IP6_CHECK_IF_OURS && *deliverifp != inifp) {
		ASSERT(*deliverifp != NULL);
		ip6stat.ip6s_rcv_if_weak_match++;

		                                                      
		if (ip6_checkinterface_debug != IP6_CHECKINTERFACE_WEAK_ES && ip6_forwarding != 0) {
			char src_str[MAX_IPv6_STR_LEN];
			char dst_str[MAX_IPv6_STR_LEN];

			inet_ntop(AF_INET6, &ip6->ip6_src, src_str, sizeof(src_str));
			inet_ntop(AF_INET6, &ip6->ip6_dst, dst_str, sizeof(dst_str));
			os_log_info(OS_LOG_DEFAULT,
			    "%s: weak ES interface match to %s for packet from %s to %s proto %u received via %s",
			    __func__, (*deliverifp)->if_xname, src_str, dst_str, ip6->ip6_nxt, inifp->if_xname);
		}
	} else if (result == IP6_CHECK_IF_DROP) {
		ip6stat.ip6s_rcv_if_no_match++;
		if (ip6_checkinterface_debug > 0) {
			char src_str[MAX_IPv6_STR_LEN];
			char dst_str[MAX_IPv6_STR_LEN];

			inet_ntop(AF_INET6, &ip6->ip6_src, src_str, sizeof(src_str));
			inet_ntop(AF_INET6, &ip6->ip6_dst, dst_str, sizeof(dst_str));
			os_log(OS_LOG_DEFAULT,
			    "%s: no interface match for packet from %s to %s proto %u received via %s",
			    __func__, src_str, dst_str, ip6->ip6_nxt, inifp->if_xname);
		}
	}

	return result;
}

void
ip6_input(struct mbuf *m)
{
	struct ip6_hdr *ip6;
	int off = sizeof(struct ip6_hdr), nest;
	u_int32_t plen;
	u_int32_t rtalert = ~0;
	int nxt = 0, ours = 0;
	struct ifnet *inifp, *deliverifp = NULL;
	ipfilter_t inject_ipfref = NULL;
	int seen = 1;
#if DUMMYNET
	struct m_tag *tag;
	struct ip_fw_args args = {};
#endif               
	struct route_in6 rin6 = {};

	  
                                                                   
              
    
	MBUF_INPUT_CHECK(m, m->m_pkthdr.rcvif);
	inifp = m->m_pkthdr.rcvif;
	VERIFY(inifp != NULL);

	                                                  
	IP6_HDR_ALIGNMENT_FIXUP(m, inifp, return );

	m->m_pkthdr.pkt_flags &= ~PKTF_FORWARDED;
#if IPSEC
	  
                                                    
                               
    
	m->m_flags &= ~M_AUTHIPHDR;
	m->m_flags &= ~M_AUTHIPDGM;
#endif            

	  
                                                                 
    
	ip6_delaux(m);

#if DUMMYNET
	if ((tag = m_tag_locate(m, KERNEL_MODULE_TAG_ID,
	    KERNEL_TAG_TYPE_DUMMYNET, NULL)) != NULL) {
		struct dn_pkt_tag       *dn_tag;

		dn_tag = (struct dn_pkt_tag *)(tag + 1);

		args.fwa_pf_rule = dn_tag->dn_pf_rule;

		m_tag_delete(m, tag);
	}

	if (args.fwa_pf_rule) {
		ip6 = mtod(m, struct ip6_hdr *);                              

		goto check_with_pf;
	}
#endif               

	  
                                                              
    
	inject_ipfref = ipf_get_inject_filter(m);
	if (inject_ipfref != NULL) {
		ip6 = mtod(m, struct ip6_hdr *);
		nxt = ip6->ip6_nxt;
		seen = 0;
		goto injectit;
	} else {
		seen = 1;
	}

	  
                   
    
	if (m->m_flags & M_EXT) {
		if (m->m_next != NULL) {
			ip6stat.ip6s_mext2m++;
		} else {
			ip6stat.ip6s_mext1++;
		}
	} else {
#define M2MMAX  (sizeof (ip6stat.ip6s_m2m) / sizeof (ip6stat.ip6s_m2m[0]))
		if (m->m_next != NULL) {
			if (m->m_pkthdr.pkt_flags & PKTF_LOOP) {
				         
				ip6stat.ip6s_m2m[ifnet_index(lo_ifp)]++;
			} else if (inifp->if_index < M2MMAX) {
				ip6stat.ip6s_m2m[inifp->if_index]++;
			} else {
				ip6stat.ip6s_m2m[0]++;
			}
		} else {
			ip6stat.ip6s_m1++;
		}
#undef M2MMAX
	}

	  
                                                                   
    
	if (inifp->if_eflags & IFEF_IPV6_DISABLED) {
		goto bad;
	}

	in6_ifstat_inc_na(inifp, ifs6_in_receive);
	ip6stat.ip6s_total++;

	  
                                                            
                                                        
                                                               
    
	if (m->m_next != NULL && m->m_pkthdr.len < MCLBYTES) {
		struct mbuf *n;

		MGETHDR(n, M_DONTWAIT, MT_HEADER);                  
		if (n) {
			M_COPY_PKTHDR(n, m);
		}
		if (n && m->m_pkthdr.len > MHLEN) {
			MCLGET(n, M_DONTWAIT);
			if ((n->m_flags & M_EXT) == 0) {
				m_freem(n);
				n = NULL;
			}
		}
		if (n == NULL) {
			goto bad;
		}

		m_copydata(m, 0, m->m_pkthdr.len, mtod(n, caddr_t));
		n->m_len = m->m_pkthdr.len;
		m_freem(m);
		m = n;
	}
	IP6_EXTHDR_CHECK(m, 0, sizeof(struct ip6_hdr), { goto done; });

	if (m->m_len < sizeof(struct ip6_hdr)) {
		if ((m = m_pullup(m, sizeof(struct ip6_hdr))) == 0) {
			ip6stat.ip6s_toosmall++;
			in6_ifstat_inc(inifp, ifs6_in_hdrerr);
			goto done;
		}
	}

	ip6 = mtod(m, struct ip6_hdr *);

	if ((ip6->ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION) {
		ip6stat.ip6s_badvers++;
		in6_ifstat_inc(inifp, ifs6_in_hdrerr);
		goto bad;
	}

	ip6stat.ip6s_nxthist[ip6->ip6_nxt]++;

	  
                                              
    
	if (!(m->m_pkthdr.pkt_flags & PKTF_LOOP) &&
	    IN6_IS_ADDR_LOOPBACK(&ip6->ip6_src)) {
		ip6stat.ip6s_badscope++;
		in6_ifstat_inc(inifp, ifs6_in_addrerr);
		goto bad;
	}
	if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_src) ||
	    IN6_IS_ADDR_UNSPECIFIED(&ip6->ip6_dst)) {
		  
                                                                 
     
		ip6stat.ip6s_badscope++;
		in6_ifstat_inc(inifp, ifs6_in_addrerr);
		goto bad;
	}
	if (IN6_IS_ADDR_MC_INTFACELOCAL(&ip6->ip6_dst) &&
	    !(m->m_pkthdr.pkt_flags & PKTF_LOOP)) {
		  
                                                           
                                                            
                                                          
                                        
     
		ip6stat.ip6s_badscope++;
		in6_ifstat_inc(inifp, ifs6_in_addrerr);
		goto bad;
	}

	  
                                                                
                                                                      
                                                                        
                                             
   
                                                                  
                                                                
                                                                     
                                                                
                                         
    
	if (IN6_IS_ADDR_V4MAPPED(&ip6->ip6_src) ||
	    IN6_IS_ADDR_V4MAPPED(&ip6->ip6_dst)) {
		ip6stat.ip6s_badscope++;
		in6_ifstat_inc(inifp, ifs6_in_addrerr);
		goto bad;
	}
#if 0
	  
                                                                
   
                                                                    
                                                                   
                                       
    
	if (IN6_IS_ADDR_V4COMPAT(&ip6->ip6_src) ||
	    IN6_IS_ADDR_V4COMPAT(&ip6->ip6_dst)) {
		ip6stat.ip6s_badscope++;
		in6_ifstat_inc(inifp, ifs6_in_addrerr);
		goto bad;
	}
#endif

	  
                                                                      
                                                                  
                                                                  
                                       
                                                               
                                                           
                                                                    
                                                                    
                                      
    
	if (nstat_collect) {
		struct rtentry *rte =
		    ifnet_cached_rtlookup_inet6(inifp, &ip6->ip6_src);
		if (rte != NULL) {
			nstat_route_rx(rte, 1, m->m_pkthdr.len, 0);
			rtfree(rte);
		}
	}

#if DUMMYNET
check_with_pf:
#endif               
#if PF
	                                  
	if (PF_IS_ENABLED) {
		int error;
#if DUMMYNET
		error = pf_af_hook(inifp, NULL, &m, AF_INET6, TRUE, &args);
#else                
		error = pf_af_hook(inifp, NULL, &m, AF_INET6, TRUE, NULL);
#endif                
		if (error != 0 || m == NULL) {
			if (m != NULL) {
				panic("%s: unexpected packet %p\n",
				    __func__, m);
				                
			}
			                             
			goto done;
		}
		ip6 = mtod(m, struct ip6_hdr *);
	}
#endif         

	                                                            
	if (!(inifp->if_flags & IFF_LOOPBACK) &&
	    !(m->m_pkthdr.pkt_flags & PKTF_LOOP)) {
		if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_src) &&
		    ip6->ip6_src.s6_addr16[1]) {
			ip6stat.ip6s_badscope++;
			goto bad;
		}
		if (IN6_IS_SCOPE_EMBED(&ip6->ip6_dst) &&
		    ip6->ip6_dst.s6_addr16[1]) {
			ip6stat.ip6s_badscope++;
			goto bad;
		}
	}

	if (m->m_pkthdr.pkt_flags & PKTF_IFAINFO) {
		if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_src)) {
			ip6->ip6_src.s6_addr16[1] =
			    htons(m->m_pkthdr.src_ifindex);
		}
		if (IN6_IS_SCOPE_EMBED(&ip6->ip6_dst)) {
			ip6->ip6_dst.s6_addr16[1] =
			    htons(m->m_pkthdr.dst_ifindex);
		}
	} else {
		if (IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_src)) {
			ip6->ip6_src.s6_addr16[1] = htons(inifp->if_index);
		}
		if (IN6_IS_SCOPE_EMBED(&ip6->ip6_dst)) {
			ip6->ip6_dst.s6_addr16[1] = htons(inifp->if_index);
		}
	}

	  
                   
    
	if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
		struct  in6_multi *in6m = NULL;

		in6_ifstat_inc_na(inifp, ifs6_in_mcast);
		  
                                                               
                       
     
		in6_multihead_lock_shared();
		IN6_LOOKUP_MULTI(&ip6->ip6_dst, inifp, in6m);
		in6_multihead_lock_done();
		if (in6m != NULL) {
			IN6M_REMREF(in6m);
			ours = 1;
		} else if (!nd6_prproxy) {
			ip6stat.ip6s_notmember++;
			ip6stat.ip6s_cantforward++;
			in6_ifstat_inc(inifp, ifs6_in_discard);
			goto bad;
		}
		deliverifp = inifp;
		  
                                                                        
                                                                       
                                                              
     
		if (deliverifp != NULL) {
			struct in6_ifaddr *ia6 = NULL;

			ia6 = in6_ifawithifp(deliverifp, &ip6->ip6_dst);
			if (ia6 != NULL) {
				(void) ip6_setdstifaddr_info(m, 0, ia6);
				IFA_REMREF(&ia6->ia_ifa);
			} else {
				(void) ip6_setdstifaddr_info(m, inifp->if_index, NULL);
			}
		}
		goto hbhcheck;
	} else {
		  
                  
     
		ip6_check_if_result_t check_if_result = IP6_CHECK_IF_NONE;
		check_if_result = ip6_input_check_interface(m, ip6, inifp, &rin6, &deliverifp);
		ASSERT(check_if_result != IP6_CHECK_IF_NONE);
		if (check_if_result == IP6_CHECK_IF_OURS) {
			ours = 1;
			goto hbhcheck;
		} else if (check_if_result == IP6_CHECK_IF_DROP) {
			goto bad;
		}
	}

	  
                                                                    
                           
    
	if (!ip6_forwarding) {
		ip6stat.ip6s_cantforward++;
		in6_ifstat_inc(inifp, ifs6_in_discard);
		  
                                                            
                                              
                                        
     
		if (inifp->if_type == IFT_CELLULAR &&
		    ip6->ip6_nxt == IPPROTO_ICMPV6) {
			in6_ifstat_inc(inifp, ifs6_cantfoward_icmp6);
		}
		goto bad;
	}

hbhcheck:
	  
                                                        
                                             
                                                                     
    
	plen = (u_int32_t)ntohs(ip6->ip6_plen);
	if (ip6->ip6_nxt == IPPROTO_HOPOPTS) {
		struct ip6_hbh *hbh;

		  
                                                               
                                                             
                                              
     
		m->m_pkthdr.pkt_flags |= PKTF_HBH_CHKED;
		if (ip6_hopopts_input(&plen, &rtalert, &m, &off)) {
#if 0                             
			in6_ifstat_inc(inifp, ifs6_in_discard);
#endif
			goto done;                                     
		}

		                    
		ip6 = mtod(m, struct ip6_hdr *);

		  
                                                               
                                                              
                             
     
		if (ip6->ip6_plen == 0 && plen == 0) {
			  
                                                  
                                                     
                                                     
      
			ip6stat.ip6s_badoptions++;
			in6_ifstat_inc(inifp, ifs6_in_discard);
			in6_ifstat_inc(inifp, ifs6_in_hdrerr);
			icmp6_error(m, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER,
			    (int)((caddr_t)&ip6->ip6_plen - (caddr_t)ip6));
			goto done;
		}
		                                                         
		hbh = (struct ip6_hbh *)(ip6 + 1);
		nxt = hbh->ip6h_nxt;

		  
                                                           
                                                          
                                                                  
                                                            
            
     
		if (rtalert != ~0 && ip6_forwarding) {
			switch (rtalert) {
			case IP6OPT_RTALERT_MLD:
				ours = 1;
				break;
			default:
				  
                                                   
                        
       
				break;
			}
		}
	} else {
		nxt = ip6->ip6_nxt;
	}

	  
                                                
                                                                
                                        
                                          
    
	if (m->m_pkthdr.len - sizeof(struct ip6_hdr) < plen) {
		ip6stat.ip6s_tooshort++;
		in6_ifstat_inc(inifp, ifs6_in_truncated);
		goto bad;
	}
	if (m->m_pkthdr.len > sizeof(struct ip6_hdr) + plen) {
		ip6_input_adjust(m, ip6, plen, inifp);
	}

	  
                         
    
	if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
		if (!ours && nd6_prproxy) {
			  
                                                    
                                                    
                                                        
                                                   
                      
      
			ours = nd6_prproxy_isours(m, ip6, NULL, IFSCOPE_NONE);
			VERIFY(!ours ||
			    (m->m_pkthdr.pkt_flags & PKTF_PROXY_DST));
		}
		if (!ours) {
			goto bad;
		}
	} else if (!ours) {
		  
                                                            
                                                           
                                                          
                                                              
                                                             
                                                             
                                                             
                                                                
                                                             
                        
     
		if ((m = ip6_forward(m, &rin6, 0)) == NULL) {
			goto done;
		}
		VERIFY(rin6.ro_rt != NULL);
		VERIFY(m->m_pkthdr.pkt_flags & PKTF_PROXY_DST);
		deliverifp = rin6.ro_rt->rt_ifp;
		ours = 1;
	}

	ip6 = mtod(m, struct ip6_hdr *);

	  
                                                                  
                                                                   
                                                                
   
                                                                  
                                                                     
           
    
	if (IN6_IS_ADDR_V4MAPPED(&ip6->ip6_src) ||
	    IN6_IS_ADDR_V4MAPPED(&ip6->ip6_dst)) {
		ip6stat.ip6s_badscope++;
		in6_ifstat_inc(inifp, ifs6_in_addrerr);
		goto bad;
	}

	  
                                       
    
	ip6stat.ip6s_delivered++;
	in6_ifstat_inc_na(deliverifp, ifs6_in_deliver);

injectit:
	nest = 0;

	  
                                                                  
                                                                
                                                                 
    
	IP6_HDR_ALIGNMENT_FIXUP(m, inifp, return );

	while (nxt != IPPROTO_DONE) {
		struct ipfilter *filter;
		int (*pr_input)(struct mbuf **, int *, int);

		  
                                                              
                                                     
     
		if (nxt == IPPROTO_HOPOPTS &&
		    (m->m_pkthdr.pkt_flags & PKTF_HBH_CHKED) == 0) {
			  
                                                    
                           
      
			ip6stat.ip6s_badoptions++;
			goto bad;
		}

		if (ip6_hdrnestlimit && (++nest > ip6_hdrnestlimit)) {
			ip6stat.ip6s_toomanyhdr++;
			goto bad;
		}

		  
                                                       
                                                   
     
		if (m->m_pkthdr.len < off) {
			ip6stat.ip6s_tooshort++;
			in6_ifstat_inc(inifp, ifs6_in_truncated);
			goto bad;
		}

#if IPSEC
		  
                                                                
                                                                 
                                
     
		if ((ipsec_bypass == 0) &&
		    (ip6_protox[nxt]->pr_flags & PR_LASTHDR) != 0) {
			if (ipsec6_in_reject(m, NULL)) {
				IPSEC_STAT_INCREMENT(ipsec6stat.in_polvio);
				goto bad;
			}
		}
#endif            

		  
                   
     
		if (!TAILQ_EMPTY(&ipv6_filters) && !IFNET_IS_INTCOPROC(inifp)) {
			ipf_ref();
			TAILQ_FOREACH(filter, &ipv6_filters, ipf_link) {
				if (seen == 0) {
					if ((struct ipfilter *)inject_ipfref ==
					    filter) {
						seen = 1;
					}
				} else if (filter->ipf_filter.ipf_input) {
					errno_t result;

					result = filter->ipf_filter.ipf_input(
						filter->ipf_filter.cookie,
						(mbuf_t *)&m, off, (uint8_t)nxt);
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
			ipf_unref();
		}

		DTRACE_IP6(receive, struct mbuf *, m, struct inpcb *, NULL,
		    struct ip6_hdr *, ip6, struct ifnet *, inifp,
		    struct ip *, NULL, struct ip6_hdr *, ip6);

		if ((pr_input = ip6_protox[nxt]->pr_input) == NULL) {
			m_freem(m);
			m = NULL;
			nxt = IPPROTO_DONE;
		} else if (!(ip6_protox[nxt]->pr_flags & PR_PROTOLOCK)) {
			lck_mtx_lock(inet6_domain_mutex);
			nxt = pr_input(&m, &off, nxt);
			lck_mtx_unlock(inet6_domain_mutex);
		} else {
			nxt = pr_input(&m, &off, nxt);
		}
	}
done:
	ROUTE_RELEASE(&rin6);
	return;
bad:
	m_freem(m);
	goto done;
}

void
ip6_setsrcifaddr_info(struct mbuf *m, uint32_t src_idx, struct in6_ifaddr *ia6)
{
	VERIFY(m->m_flags & M_PKTHDR);

	  
                                                              
                                                                
                                             
    
	if (ia6 != NULL) {
		m->m_pkthdr.pkt_flags |= PKTF_IFAINFO;
		m->m_pkthdr.src_ifindex = ia6->ia_ifp->if_index;

		                                       
		m->m_pkthdr.src_iff = (ia6->ia6_flags & 0xffff);
	} else {
		m->m_pkthdr.src_iff = 0;
		m->m_pkthdr.src_ifindex = (uint16_t)src_idx;
		if (src_idx != 0) {
			m->m_pkthdr.pkt_flags |= PKTF_IFAINFO;
		}
	}
}

void
ip6_setdstifaddr_info(struct mbuf *m, uint32_t dst_idx, struct in6_ifaddr *ia6)
{
	VERIFY(m->m_flags & M_PKTHDR);

	  
                                                                   
                                                                
                                             
    
	if (ia6 != NULL) {
		m->m_pkthdr.pkt_flags |= PKTF_IFAINFO;
		m->m_pkthdr.dst_ifindex = ia6->ia_ifp->if_index;

		                                       
		m->m_pkthdr.dst_iff = (ia6->ia6_flags & 0xffff);
	} else {
		m->m_pkthdr.dst_iff = 0;
		m->m_pkthdr.dst_ifindex = (uint16_t)dst_idx;
		if (dst_idx != 0) {
			m->m_pkthdr.pkt_flags |= PKTF_IFAINFO;
		}
	}
}

int
ip6_getsrcifaddr_info(struct mbuf *m, uint32_t *src_idx, uint32_t *ia6f)
{
	VERIFY(m->m_flags & M_PKTHDR);

	if (!(m->m_pkthdr.pkt_flags & PKTF_IFAINFO)) {
		return -1;
	}

	if (src_idx != NULL) {
		*src_idx = m->m_pkthdr.src_ifindex;
	}

	if (ia6f != NULL) {
		*ia6f = m->m_pkthdr.src_iff;
	}

	return 0;
}

int
ip6_getdstifaddr_info(struct mbuf *m, uint32_t *dst_idx, uint32_t *ia6f)
{
	VERIFY(m->m_flags & M_PKTHDR);

	if (!(m->m_pkthdr.pkt_flags & PKTF_IFAINFO)) {
		return -1;
	}

	if (dst_idx != NULL) {
		*dst_idx = m->m_pkthdr.dst_ifindex;
	}

	if (ia6f != NULL) {
		*ia6f = m->m_pkthdr.dst_iff;
	}

	return 0;
}

  
                                                                           
                                                             
   
static int
ip6_hopopts_input(uint32_t *plenp, uint32_t *rtalertp, struct mbuf **mp,
    int *offp)
{
	struct mbuf *m = *mp;
	int off = *offp, hbhlen;
	struct ip6_hbh *hbh;
	u_int8_t *opt;

	                                            
	IP6_EXTHDR_CHECK(m, off, sizeof(*hbh), return (-1));
	hbh = (struct ip6_hbh *)(mtod(m, caddr_t) + off);
	hbhlen = (hbh->ip6h_len + 1) << 3;

	IP6_EXTHDR_CHECK(m, off, hbhlen, return (-1));
	hbh = (struct ip6_hbh *)(mtod(m, caddr_t) + off);
	off += hbhlen;
	hbhlen -= sizeof(struct ip6_hbh);
	opt = (u_int8_t *)hbh + sizeof(struct ip6_hbh);

	if (ip6_process_hopopts(m, (u_int8_t *)hbh + sizeof(struct ip6_hbh),
	    hbhlen, rtalertp, plenp) < 0) {
		return -1;
	}

	*offp = off;
	*mp = m;
	return 0;
}

  
                                                                    
                                                                 
                                                                     
                                                                            
  
                                                                              
                                                                          
                                                           
   
int
ip6_process_hopopts(struct mbuf *m, u_int8_t *opthead, int hbhlen,
    u_int32_t *rtalertp, u_int32_t *plenp)
{
	struct ip6_hdr *ip6;
	int optlen = 0;
	u_int8_t *opt = opthead;
	u_int16_t rtalert_val;
	u_int32_t jumboplen;
	const int erroff = sizeof(struct ip6_hdr) + sizeof(struct ip6_hbh);

	for (; hbhlen > 0; hbhlen -= optlen, opt += optlen) {
		switch (*opt) {
		case IP6OPT_PAD1:
			optlen = 1;
			break;
		case IP6OPT_PADN:
			if (hbhlen < IP6OPT_MINLEN) {
				ip6stat.ip6s_toosmall++;
				goto bad;
			}
			optlen = *(opt + 1) + 2;
			break;
		case IP6OPT_ROUTER_ALERT:
			                                      
			if (hbhlen < IP6OPT_RTALERT_LEN) {
				ip6stat.ip6s_toosmall++;
				goto bad;
			}
			if (*(opt + 1) != IP6OPT_RTALERT_LEN - 2) {
				              
				icmp6_error(m, ICMP6_PARAM_PROB,
				    ICMP6_PARAMPROB_HEADER,
				    (int)(erroff + opt + 1 - opthead));
				return -1;
			}
			optlen = IP6OPT_RTALERT_LEN;
			bcopy((caddr_t)(opt + 2), (caddr_t)&rtalert_val, 2);
			*rtalertp = ntohs(rtalert_val);
			break;
		case IP6OPT_JUMBO:
			                                      
			if (hbhlen < IP6OPT_JUMBO_LEN) {
				ip6stat.ip6s_toosmall++;
				goto bad;
			}
			if (*(opt + 1) != IP6OPT_JUMBO_LEN - 2) {
				              
				icmp6_error(m, ICMP6_PARAM_PROB,
				    ICMP6_PARAMPROB_HEADER,
				    (int)(erroff + opt + 1 - opthead));
				return -1;
			}
			optlen = IP6OPT_JUMBO_LEN;

			  
                                                 
                                              
      
			ip6 = mtod(m, struct ip6_hdr *);
			if (ip6->ip6_plen) {
				ip6stat.ip6s_badoptions++;
				icmp6_error(m, ICMP6_PARAM_PROB,
				    ICMP6_PARAMPROB_HEADER,
				    (int)(erroff + opt - opthead));
				return -1;
			}

			  
                                                   
                                   
      
			bcopy(opt + 2, &jumboplen, sizeof(jumboplen));
			jumboplen = (u_int32_t)htonl(jumboplen);

#if 1
			  
                                                  
                                                    
               
                                                   
                                                    
                                                   
      
			if (*plenp != 0) {
				ip6stat.ip6s_badoptions++;
				icmp6_error(m, ICMP6_PARAM_PROB,
				    ICMP6_PARAMPROB_HEADER,
				    (int)(erroff + opt + 2 - opthead));
				return -1;
			}
#endif

			  
                                                     
      
			if (jumboplen <= IPV6_MAXPACKET) {
				ip6stat.ip6s_badoptions++;
				icmp6_error(m, ICMP6_PARAM_PROB,
				    ICMP6_PARAMPROB_HEADER,
				    (int)(erroff + opt + 2 - opthead));
				return -1;
			}
			*plenp = jumboplen;

			break;
		default:                                    
			if (hbhlen < IP6OPT_MINLEN) {
				ip6stat.ip6s_toosmall++;
				goto bad;
			}
			optlen = ip6_unknown_opt(opt, m,
			    erroff + opt - opthead);
			if (optlen == -1) {
				return -1;
			}
			optlen += 2;
			break;
		}
	}

	return 0;

bad:
	m_freem(m);
	return -1;
}

  
                             
                                                                             
                                                                              
                                                        
   
int
ip6_unknown_opt(uint8_t *optp, struct mbuf *m, size_t off)
{
	struct ip6_hdr *ip6;

	switch (IP6OPT_TYPE(*optp)) {
	case IP6OPT_TYPE_SKIP:                        
		return (int)*(optp + 1);

	case IP6OPT_TYPE_DISCARD:                             
		m_freem(m);
		return -1;

	case IP6OPT_TYPE_FORCEICMP:                                    
		ip6stat.ip6s_badoptions++;
		icmp6_error(m, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_OPTION, (int)off);
		return -1;

	case IP6OPT_TYPE_ICMP:                                   
		ip6stat.ip6s_badoptions++;
		ip6 = mtod(m, struct ip6_hdr *);
		if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst) ||
		    (m->m_flags & (M_BCAST | M_MCAST))) {
			m_freem(m);
		} else {
			icmp6_error(m, ICMP6_PARAM_PROB,
			    ICMP6_PARAMPROB_OPTION, (int)off);
		}
		return -1;
	}

	m_freem(m);                                  
	return -1;
}

  
                                          
                                                     
  
                                    
                                                                          
                                                                     
                                                                             
                                     
  
                                                                       
                             
                                                                        
                                              
   
struct mbuf **
ip6_savecontrol_v4(struct inpcb *inp, struct mbuf *m, struct mbuf **mp,
    int *v4only)
{
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);

	if ((inp->inp_socket->so_options & SO_TIMESTAMP) != 0) {
		struct timeval tv;

		getmicrotime(&tv);
		mp = sbcreatecontrol_mbuf((caddr_t)&tv, sizeof(tv),
		    SCM_TIMESTAMP, SOL_SOCKET, mp);
		if (*mp == NULL) {
			return NULL;
		}
	}
	if ((inp->inp_socket->so_options & SO_TIMESTAMP_MONOTONIC) != 0) {
		uint64_t time;

		time = mach_absolute_time();
		mp = sbcreatecontrol_mbuf((caddr_t)&time, sizeof(time),
		    SCM_TIMESTAMP_MONOTONIC, SOL_SOCKET, mp);
		if (*mp == NULL) {
			return NULL;
		}
	}
	if ((inp->inp_socket->so_options & SO_TIMESTAMP_CONTINUOUS) != 0) {
		uint64_t time;

		time = mach_continuous_time();
		mp = sbcreatecontrol_mbuf((caddr_t)&time, sizeof(time),
		    SCM_TIMESTAMP_CONTINUOUS, SOL_SOCKET, mp);
		if (*mp == NULL) {
			return NULL;
		}
	}
	if ((inp->inp_socket->so_flags & SOF_RECV_TRAFFIC_CLASS) != 0) {
		int tc = m_get_traffic_class(m);

		mp = sbcreatecontrol_mbuf((caddr_t)&tc, sizeof(tc),
		    SO_TRAFFIC_CLASS, SOL_SOCKET, mp);
		if (*mp == NULL) {
			return NULL;
		}
	}

#define IS2292(inp, x, y)       (((inp)->inp_flags & IN6P_RFC2292) ? (x) : (y))
	if ((ip6->ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION) {
		if (v4only != NULL) {
			*v4only = 1;
		}

		                                         
		if ((inp->inp_flags & IN6P_TCLASS) != 0) {
			struct ip *ip_header = mtod(m, struct ip *);

			int tclass = (int)(ip_header->ip_tos);
			mp = sbcreatecontrol_mbuf((caddr_t)&tclass, sizeof(tclass),
			    IPV6_TCLASS, IPPROTO_IPV6, mp);
			if (*mp == NULL) {
				return NULL;
			}
		}

		                                          
		if ((inp->inp_flags & IN6P_PKTINFO) != 0
#if CONTENT_FILTER
		                                                   
		    || (inp->inp_socket->so_cfil_db != NULL)
#endif
		    ) {
			struct in6_pktinfo pi6 = {
				.ipi6_addr = IN6ADDR_V4MAPPED_INIT,
				.ipi6_ifindex = (m && m->m_pkthdr.rcvif) ? m->m_pkthdr.rcvif->if_index : 0,
			};

			struct ip *ip_header = mtod(m, struct ip *);
			bcopy(&ip_header->ip_dst, &pi6.ipi6_addr.s6_addr32[3], sizeof(struct in_addr));

			mp = sbcreatecontrol_mbuf((caddr_t)&pi6,
			    sizeof(struct in6_pktinfo),
			    IS2292(inp, IPV6_2292PKTINFO, IPV6_PKTINFO),
			    IPPROTO_IPV6, mp);
			if (*mp == NULL) {
				return NULL;
			}
		}
		return mp;
	}

	                     
	if ((inp->inp_flags & IN6P_PKTINFO) != 0
#if CONTENT_FILTER
	                                                   
	    || (inp->inp_socket->so_cfil_db != NULL)
#endif
	    ) {
		struct in6_pktinfo pi6;

		bcopy(&ip6->ip6_dst, &pi6.ipi6_addr, sizeof(struct in6_addr));
		in6_clearscope(&pi6.ipi6_addr);          
		pi6.ipi6_ifindex =
		    (m && m->m_pkthdr.rcvif) ? m->m_pkthdr.rcvif->if_index : 0;

		mp = sbcreatecontrol_mbuf((caddr_t)&pi6,
		    sizeof(struct in6_pktinfo),
		    IS2292(inp, IPV6_2292PKTINFO, IPV6_PKTINFO),
		    IPPROTO_IPV6, mp);
		if (*mp == NULL) {
			return NULL;
		}
	}

	if ((inp->inp_flags & IN6P_HOPLIMIT) != 0) {
		int hlim = ip6->ip6_hlim & 0xff;

		mp = sbcreatecontrol_mbuf((caddr_t)&hlim, sizeof(int),
		    IS2292(inp, IPV6_2292HOPLIMIT, IPV6_HOPLIMIT),
		    IPPROTO_IPV6, mp);
		if (*mp == NULL) {
			return NULL;
		}
	}

	if (v4only != NULL) {
		*v4only = 0;
	}
	return mp;
}

int
ip6_savecontrol(struct inpcb *in6p, struct mbuf *m, struct mbuf **mp)
{
	struct mbuf **np;
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	int v4only = 0;

	*mp = NULL;
	np = ip6_savecontrol_v4(in6p, m, mp, &v4only);
	if (np == NULL) {
		goto no_mbufs;
	}

	mp = np;
	if (v4only) {
		return 0;
	}

	if ((in6p->inp_flags & IN6P_TCLASS) != 0) {
		u_int32_t flowinfo;
		int tclass;

		flowinfo = (u_int32_t)ntohl(ip6->ip6_flow & IPV6_FLOWINFO_MASK);
		flowinfo >>= 20;

		tclass = flowinfo & 0xff;
		mp = sbcreatecontrol_mbuf((caddr_t)&tclass, sizeof(tclass),
		    IPV6_TCLASS, IPPROTO_IPV6, mp);
		if (*mp == NULL) {
			goto no_mbufs;
		}
	}

	  
                                                                   
                                                                     
                                                                     
                            
                                                        
    
	if ((in6p->inp_flags & IN6P_HOPOPTS) != 0) {
		  
                                                              
                                                               
                                                        
                                                             
                           
     
		ip6 = mtod(m, struct ip6_hdr *);
		if (ip6->ip6_nxt == IPPROTO_HOPOPTS) {
			struct ip6_hbh *hbh;
			int hbhlen = 0;
			hbh = (struct ip6_hbh *)(ip6 + 1);
			hbhlen = (hbh->ip6h_len + 1) << 3;

			  
                                             
                                                        
                                                    
              
                                                 
      
			mp = sbcreatecontrol_mbuf((caddr_t)hbh, hbhlen,
			    IS2292(in6p, IPV6_2292HOPOPTS, IPV6_HOPOPTS),
			    IPPROTO_IPV6, mp);

			if (*mp == NULL) {
				goto no_mbufs;
			}
		}
	}

	if ((in6p->inp_flags & (IN6P_RTHDR | IN6P_DSTOPTS)) != 0) {
		int nxt = ip6->ip6_nxt, off = sizeof(struct ip6_hdr);

		  
                                                      
                                                        
                              
                                                  
                                 
     
		while (1) {                                                 
			struct ip6_ext *ip6e = NULL;
			int elen;

			  
                                                    
                             
      
			switch (nxt) {
			case IPPROTO_DSTOPTS:
			case IPPROTO_ROUTING:
			case IPPROTO_HOPOPTS:
			case IPPROTO_AH:                      
				break;
			default:
				goto loopend;
			}

			if (off + sizeof(*ip6e) > m->m_len) {
				goto loopend;
			}
			ip6e = (struct ip6_ext *)(mtod(m, caddr_t) + off);
			if (nxt == IPPROTO_AH) {
				elen = (ip6e->ip6e_len + 2) << 2;
			} else {
				elen = (ip6e->ip6e_len + 1) << 3;
			}
			if (off + elen > m->m_len) {
				goto loopend;
			}

			switch (nxt) {
			case IPPROTO_DSTOPTS:
				if (!(in6p->inp_flags & IN6P_DSTOPTS)) {
					break;
				}

				mp = sbcreatecontrol_mbuf((caddr_t)ip6e, elen,
				    IS2292(in6p, IPV6_2292DSTOPTS,
				    IPV6_DSTOPTS), IPPROTO_IPV6, mp);
				if (*mp == NULL) {
					goto no_mbufs;
				}
				break;
			case IPPROTO_ROUTING:
				if (!(in6p->inp_flags & IN6P_RTHDR)) {
					break;
				}

				mp = sbcreatecontrol_mbuf((caddr_t)ip6e, elen,
				    IS2292(in6p, IPV6_2292RTHDR, IPV6_RTHDR),
				    IPPROTO_IPV6, mp);
				if (*mp == NULL) {
					goto no_mbufs;
				}
				break;
			case IPPROTO_HOPOPTS:
			case IPPROTO_AH:                      
				break;

			default:
				  
                                                   
                                                 
                                                
                    
       
				goto loopend;
			}

			                                   
			off += elen;
			nxt = ip6e->ip6e_nxt;
			ip6e = NULL;
		}
loopend:
		;
	}
	return 0;
no_mbufs:
	ip6stat.ip6s_pktdropcntrl++;
	                                              
	return ENOBUFS;
}
#undef IS2292

void
ip6_notify_pmtu(struct inpcb *in6p, struct sockaddr_in6 *dst, u_int32_t *mtu)
{
	struct socket *so;
	struct mbuf *m_mtu;
	struct ip6_mtuinfo mtuctl;

	so =  in6p->inp_socket;

	if ((in6p->inp_flags & IN6P_MTU) == 0) {
		return;
	}

	if (mtu == NULL) {
		return;
	}

#ifdef DIAGNOSTIC
	if (so == NULL) {                                                 
		panic("ip6_notify_pmtu: socket is NULL");
		                
	}
#endif

	if (IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr) &&
	    (so->so_proto == NULL || so->so_proto->pr_protocol == IPPROTO_TCP)) {
		return;
	}

	if (!IN6_IS_ADDR_UNSPECIFIED(&in6p->in6p_faddr) &&
	    !IN6_ARE_ADDR_EQUAL(&in6p->in6p_faddr, &dst->sin6_addr)) {
		return;
	}

	bzero(&mtuctl, sizeof(mtuctl));                                    
	mtuctl.ip6m_mtu = *mtu;
	mtuctl.ip6m_addr = *dst;
	if (sa6_recoverscope(&mtuctl.ip6m_addr, TRUE)) {
		return;
	}

	if ((m_mtu = sbcreatecontrol((caddr_t)&mtuctl, sizeof(mtuctl),
	    IPV6_PATHMTU, IPPROTO_IPV6)) == NULL) {
		return;
	}

	if (sbappendaddr(&so->so_rcv, SA(dst), NULL, m_mtu, NULL) == 0) {
		return;
	}
	sorwakeup(so);
}

  
                                                            
                       
                                   
                          
                                                                   
                 
                                               
                                                                  
                                                                   
                                                              
   
char *
ip6_get_prevhdr(struct mbuf *m, int off)
{
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);

	if (off == sizeof(struct ip6_hdr)) {
		return (char *)&ip6->ip6_nxt;
	} else {
		int len, nxt;
		struct ip6_ext *ip6e = NULL;

		nxt = ip6->ip6_nxt;
		len = sizeof(struct ip6_hdr);
		while (len < off) {
			ip6e = (struct ip6_ext *)(mtod(m, caddr_t) + len);

			switch (nxt) {
			case IPPROTO_FRAGMENT:
				len += sizeof(struct ip6_frag);
				break;
			case IPPROTO_AH:
				len += (ip6e->ip6e_len + 2) << 2;
				break;
			default:
				len += (ip6e->ip6e_len + 1) << 3;
				break;
			}
			nxt = ip6e->ip6e_nxt;
		}
		if (ip6e) {
			return (char *)&ip6e->ip6e_nxt;
		} else {
			return NULL;
		}
	}
}

  
                                               
   
int
ip6_nexthdr(struct mbuf *m, int off, int proto, int *nxtp)
{
	struct ip6_hdr ip6;
	struct ip6_ext ip6e;
	struct ip6_frag fh;

	                  
	VERIFY(m != NULL);
	if ((m->m_flags & M_PKTHDR) == 0 || m->m_pkthdr.len < off) {
		return -1;
	}

	switch (proto) {
	case IPPROTO_IPV6:
		if (m->m_pkthdr.len < off + sizeof(ip6)) {
			return -1;
		}
		m_copydata(m, off, sizeof(ip6), (caddr_t)&ip6);
		if (nxtp) {
			*nxtp = ip6.ip6_nxt;
		}
		off += sizeof(ip6);
		return off;

	case IPPROTO_FRAGMENT:
		  
                                                       
                                                
     
		if (m->m_pkthdr.len < off + sizeof(fh)) {
			return -1;
		}
		m_copydata(m, off, sizeof(fh), (caddr_t)&fh);
		                                                             
		if (fh.ip6f_offlg & IP6F_OFF_MASK) {
			return -1;
		}
		if (nxtp) {
			*nxtp = fh.ip6f_nxt;
		}
		off += sizeof(struct ip6_frag);
		return off;

	case IPPROTO_AH:
		if (m->m_pkthdr.len < off + sizeof(ip6e)) {
			return -1;
		}
		m_copydata(m, off, sizeof(ip6e), (caddr_t)&ip6e);
		if (nxtp) {
			*nxtp = ip6e.ip6e_nxt;
		}
		off += (ip6e.ip6e_len + 2) << 2;
		return off;

	case IPPROTO_HOPOPTS:
	case IPPROTO_ROUTING:
	case IPPROTO_DSTOPTS:
		if (m->m_pkthdr.len < off + sizeof(ip6e)) {
			return -1;
		}
		m_copydata(m, off, sizeof(ip6e), (caddr_t)&ip6e);
		if (nxtp) {
			*nxtp = ip6e.ip6e_nxt;
		}
		off += (ip6e.ip6e_len + 1) << 3;
		return off;

	case IPPROTO_NONE:
	case IPPROTO_ESP:
	case IPPROTO_IPCOMP:
		             
		return -1;

	default:
		return -1;
	}
}

  
                                                                          
   
int
ip6_lasthdr(struct mbuf *m, int off, int proto, int *nxtp)
{
	int newoff;
	int nxt;

	if (!nxtp) {
		nxt = -1;
		nxtp = &nxt;
	}
	while (1) {
		newoff = ip6_nexthdr(m, off, proto, nxtp);
		if (newoff < 0) {
			return off;
		} else if (newoff < off) {
			return -1;                 
		} else if (newoff == off) {
			return newoff;
		}

		off = newoff;
		proto = *nxtp;
	}
}

boolean_t
ip6_pkt_has_ulp(struct mbuf *m)
{
	int off = 0, nxt = IPPROTO_NONE;

	off = ip6_lasthdr(m, 0, IPPROTO_IPV6, &nxt);
	if (off < 0 || m->m_pkthdr.len < off) {
		return FALSE;
	}

	switch (nxt) {
	case IPPROTO_TCP:
		if (off + sizeof(struct tcphdr) > m->m_pkthdr.len) {
			return FALSE;
		}
		break;
	case IPPROTO_UDP:
		if (off + sizeof(struct udphdr) > m->m_pkthdr.len) {
			return FALSE;
		}
		break;
	case IPPROTO_ICMPV6:
		if (off + sizeof(uint32_t) > m->m_pkthdr.len) {
			return FALSE;
		}
		break;
	case IPPROTO_NONE:
		return TRUE;
	case IPPROTO_ESP:
		return TRUE;
	case IPPROTO_IPCOMP:
		return TRUE;
	default:
		return FALSE;
	}
	return TRUE;
}

struct ip6aux *
ip6_addaux(struct mbuf *m)
{
	struct m_tag            *tag;

	                                       
	tag = m_tag_locate(m, KERNEL_MODULE_TAG_ID,
	    KERNEL_TAG_TYPE_INET6, NULL);
	if (tag == NULL) {
		                    
		tag = m_tag_create(KERNEL_MODULE_TAG_ID, KERNEL_TAG_TYPE_INET6,
		    sizeof(struct ip6aux), M_DONTWAIT, m);

		                           
		if (tag) {
			m_tag_prepend(m, tag);
		}
	}

	return tag ? (struct ip6aux *)(tag + 1) : NULL;
}

struct ip6aux *
ip6_findaux(struct mbuf *m)
{
	struct m_tag    *tag;

	tag = m_tag_locate(m, KERNEL_MODULE_TAG_ID,
	    KERNEL_TAG_TYPE_INET6, NULL);

	return tag ? (struct ip6aux *)(tag + 1) : NULL;
}

void
ip6_delaux(struct mbuf *m)
{
	struct m_tag    *tag;

	tag = m_tag_locate(m, KERNEL_MODULE_TAG_ID,
	    KERNEL_TAG_TYPE_INET6, NULL);
	if (tag) {
		m_tag_delete(m, tag);
	}
}

  
                 
   
void
ip6_drain(void)
{
	frag6_drain();                         
	in6_rtqdrain();                                     
	nd6_drain(NULL);                                
}

  
                         
   

u_char  inet6ctlerrmap[PRC_NCMDS] = {
	0, 0, 0, 0,
	0, EMSGSIZE, EHOSTDOWN, EHOSTUNREACH,
	EHOSTUNREACH, EHOSTUNREACH, ECONNREFUSED, ECONNREFUSED,
	EMSGSIZE, EHOSTUNREACH, 0, 0,
	0, 0, 0, 0,
	ENOPROTOOPT
};

static int
sysctl_reset_ip6_input_stats SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error, i;

	i = ip6_input_measure;
	error = sysctl_handle_int(oidp, &i, 0, req);
	if (error || req->newptr == USER_ADDR_NULL) {
		goto done;
	}
	                   
	if (i < 0 || i > 1) {
		error = EINVAL;
		goto done;
	}
	if (ip6_input_measure != i && i == 1) {
		net_perf_initialize(&net_perf, ip6_input_measure_bins);
	}
	ip6_input_measure = i;
done:
	return error;
}

static int
sysctl_ip6_input_measure_bins SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error;
	uint64_t i;

	i = ip6_input_measure_bins;
	error = sysctl_handle_quad(oidp, &i, 0, req);
	if (error || req->newptr == USER_ADDR_NULL) {
		goto done;
	}
	                   
	if (!net_perf_validate_bins(i)) {
		error = EINVAL;
		goto done;
	}
	ip6_input_measure_bins = i;
done:
	return error;
}

static int
sysctl_ip6_input_getperf SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	if (req->oldptr == USER_ADDR_NULL) {
		req->oldlen = (size_t)sizeof(struct net_perf);
	}

	return SYSCTL_OUT(req, &net_perf, MIN(sizeof(net_perf), req->oldlen));
}


  
                                             
   
static void
in6_ifaddrhashtbl_init(void)
{
	int i, k, p;

	if (in6_ifaddrhashtbl != NULL) {
		return;
	}

	PE_parse_boot_argn("ina6ddr_nhash", &in6addr_nhash,
	    sizeof(in6addr_nhash));
	if (in6addr_nhash == 0) {
		in6addr_nhash = IN6ADDR_NHASH;
	}

	MALLOC(in6_ifaddrhashtbl, struct in6_ifaddrhashhead *,
	    in6addr_nhash * sizeof(*in6_ifaddrhashtbl),
	    M_IFADDR, M_WAITOK | M_ZERO);
	if (in6_ifaddrhashtbl == NULL) {
		panic("in6_ifaddrhashtbl allocation failed");
	}

	  
                                                               
    
	k = (in6addr_nhash % 2 == 0) ? in6addr_nhash + 1 : in6addr_nhash + 2;
	for (;;) {
		p = 1;
		for (i = 3; i * i <= k; i += 2) {
			if (k % i == 0) {
				p = 0;
			}
		}
		if (p == 1) {
			break;
		}
		k += 2;
	}
	in6addr_hashp = k;
}

static int
sysctl_ip6_checkinterface SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error, i;

	i = ip6_checkinterface;
	error = sysctl_handle_int(oidp, &i, 0, req);
	if (error || req->newptr == USER_ADDR_NULL) {
		return error;
	}

	switch (i) {
	case IP6_CHECKINTERFACE_WEAK_ES:
	case IP6_CHECKINTERFACE_HYBRID_ES:
	case IP6_CHECKINTERFACE_STRONG_ES:
		if (ip6_checkinterface != i) {
			ip6_checkinterface = i;
			os_log(OS_LOG_DEFAULT, "%s: ip6_checkinterface is now %d\n",
			    __func__, ip6_checkinterface);
		}
		break;
	default:
		error = EINVAL;
		break;
	}
	return error;
}
