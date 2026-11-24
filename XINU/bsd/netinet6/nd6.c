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

  
      
                    
                                                                
                                                                 
                                                         
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/protosw.h>
#include <sys/proc.h>
#include <sys/mcache.h>

#include <dev/random/randomdev.h>

#include <kern/queue.h>
#include <kern/zalloc.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_llreach.h>
#include <net/route.h>
#include <net/dlil.h>
#include <net/ntstat.h>
#include <net/net_osdep.h>
#include <net/nwk_wq.h>

#include <netinet/in.h>
#include <netinet/in_arp.h>
#include <netinet/if_ether.h>
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/scope6_var.h>
#include <netinet/icmp6.h>

#include <os/log.h>

#include "loop.h"

#define ND6_SLOWTIMER_INTERVAL          (60 * 60)                   
#define ND6_RECALC_REACHTM_INTERVAL     (60 * 120)                   

#define equal(a1, a2) (bcmp((caddr_t)(a1), (caddr_t)(a2), (a1)->sa_len) == 0)

                  
int     nd6_prune       = 1;                                   
int     nd6_prune_lazy  = 5;                                          
int     nd6_delay       = 5;                                         
int     nd6_umaxtries   = 3;                               
int     nd6_mmaxtries   = 3;                                 
int     nd6_useloopback = 1;                                                  
int     nd6_gctimer     = (60 * 60 * 24);                                      

                                                    
int nd6_maxndopt = 10;                                   

int nd6_maxqueuelen = 1;                                                       

#if ND6_DEBUG
int nd6_debug = 1;
#else
int nd6_debug = 0;
#endif

int nd6_optimistic_dad = ND6_OPTIMISTIC_DAD_DEFAULT;

                    
static int nd6_inuse, nd6_allocated;

  
                         
  
                                                                   
                                                                 
                                                                 
                                                        
  
                                                                   
                                                                   
                                                          
  
                          
  
                            
  
                                                               
                          
  
                                 
  
                                                                     
                                                                    
                                                                  
                                         
   
struct llinfo_nd6 llinfo_nd6 = {
	.ln_next = &llinfo_nd6,
	.ln_prev = &llinfo_nd6,
};

static lck_grp_attr_t   *nd_if_lock_grp_attr = NULL;
static lck_grp_t        *nd_if_lock_grp = NULL;
static lck_attr_t       *nd_if_lock_attr = NULL;

                            
struct nd_drhead nd_defrouter_list;
struct nd_prhead nd_prefix = { .lh_first = 0 };
struct nd_rtihead nd_rti_list;
  
                                                                         
                                                                         
                                                                         
                                                                       
                                                                           
                                                                             
                                                                     
                                                                         
                                                                         
                                                                      
                                                           
   
static int nd6_timeout_run;                                                  
static void nd6_timeout(void *);
int nd6_sched_timeout_want;                                                     
static boolean_t nd6_fast_timer_on = FALSE;

                                                                      
static boolean_t nd6_service_busy;
static void *nd6_service_wc = &nd6_service_busy;
static int nd6_service_waiters = 0;

int nd6_recalc_reachtm_interval = ND6_RECALC_REACHTM_INTERVAL;
static struct sockaddr_in6 all1_sa;

static int regen_tmpaddr(struct in6_ifaddr *);
extern lck_mtx_t *nd6_mutex;

static struct llinfo_nd6 *nd6_llinfo_alloc(zalloc_flags_t);
static void nd6_llinfo_free(void *);
static void nd6_llinfo_purge(struct rtentry *);
static void nd6_llinfo_get_ri(struct rtentry *, struct rt_reach_info *);
static void nd6_llinfo_get_iflri(struct rtentry *, struct ifnet_llreach_info *);
static void nd6_llinfo_refresh(struct rtentry *);
static uint64_t ln_getexpire(struct llinfo_nd6 *);

static void nd6_service(void *);
static void nd6_slowtimo(void *);
static int nd6_is_new_addr_neighbor(struct sockaddr_in6 *, struct ifnet *);
static int nd6_siocgdrlst(void *, int);
static int nd6_siocgprlst(void *, int);

static void nd6_router_select_rti_entries(struct ifnet *);
static void nd6_purge_interface_default_routers(struct ifnet *);
static void nd6_purge_interface_rti_entries(struct ifnet *);
static void nd6_purge_interface_prefixes(struct ifnet *);
static void nd6_purge_interface_llinfo(struct ifnet *);

static int nd6_sysctl_drlist SYSCTL_HANDLER_ARGS;
static int nd6_sysctl_prlist SYSCTL_HANDLER_ARGS;

  
                                                                         
   
#define LN_DEQUEUE(_ln) do {                                            \
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);                 \
	RT_LOCK_ASSERT_HELD((_ln)->ln_rt);                              \
	(_ln)->ln_next->ln_prev = (_ln)->ln_prev;                       \
	(_ln)->ln_prev->ln_next = (_ln)->ln_next;                       \
	(_ln)->ln_prev = (_ln)->ln_next = NULL;                         \
	(_ln)->ln_flags &= ~ND6_LNF_IN_USE;                             \
} while (0)

#define LN_INSERTHEAD(_ln) do {                                         \
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);                 \
	RT_LOCK_ASSERT_HELD((_ln)->ln_rt);                              \
	(_ln)->ln_next = llinfo_nd6.ln_next;                            \
	llinfo_nd6.ln_next = (_ln);                                     \
	(_ln)->ln_prev = &llinfo_nd6;                                   \
	(_ln)->ln_next->ln_prev = (_ln);                                \
	(_ln)->ln_flags |= ND6_LNF_IN_USE;                              \
} while (0)

static ZONE_DECLARE(llinfo_nd6_zone, "llinfo_nd6",
    sizeof(struct llinfo_nd6), ZC_ZFREE_CLEARMEM);

extern int tvtohz(struct timeval *);

static int nd6_init_done;

SYSCTL_DECL(_net_inet6_icmp6);

SYSCTL_PROC(_net_inet6_icmp6, ICMPV6CTL_ND6_DRLIST, nd6_drlist,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED, 0, 0,
    nd6_sysctl_drlist, "S,in6_defrouter", "");

SYSCTL_PROC(_net_inet6_icmp6, ICMPV6CTL_ND6_PRLIST, nd6_prlist,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED, 0, 0,
    nd6_sysctl_prlist, "S,in6_defrouter", "");

SYSCTL_DECL(_net_inet6_ip6);

static int ip6_maxchainsent = 0;
SYSCTL_INT(_net_inet6_ip6, OID_AUTO, maxchainsent,
    CTLFLAG_RW | CTLFLAG_LOCKED, &ip6_maxchainsent, 0,
    "use dlil_output_list");

SYSCTL_DECL(_net_inet6_icmp6);
int nd6_process_rti = ND6_PROCESS_RTI_DEFAULT;

SYSCTL_INT(_net_inet6_icmp6, OID_AUTO, nd6_process_rti, CTLFLAG_RW | CTLFLAG_LOCKED,
    &nd6_process_rti, 0,
    "Enable/disable processing of Route Information Option in the "
    "IPv6 Router Advertisement.");

void
nd6_init(void)
{
	int i;

	VERIFY(!nd6_init_done);

	all1_sa.sin6_family = AF_INET6;
	all1_sa.sin6_len = sizeof(struct sockaddr_in6);
	for (i = 0; i < sizeof(all1_sa.sin6_addr); i++) {
		all1_sa.sin6_addr.s6_addr[i] = 0xff;
	}

	                                               
	TAILQ_INIT(&nd_defrouter_list);
	TAILQ_INIT(&nd_rti_list);

	nd_if_lock_grp_attr = lck_grp_attr_alloc_init();
	nd_if_lock_grp = lck_grp_alloc_init("nd_if_lock", nd_if_lock_grp_attr);
	nd_if_lock_attr = lck_attr_alloc_init();

	nd6_nbr_init();
	nd6_rtr_init();

	nd6_init_done = 1;

	                 
	timeout(nd6_slowtimo, NULL, ND6_SLOWTIMER_INTERVAL * hz);
}

static struct llinfo_nd6 *
nd6_llinfo_alloc(zalloc_flags_t how)
{
	return zalloc_flags(llinfo_nd6_zone, how | Z_ZERO);
}

static void
nd6_llinfo_free(void *arg)
{
	struct llinfo_nd6 *ln = arg;

	if (ln->ln_next != NULL || ln->ln_prev != NULL) {
		panic("%s: trying to free %p when it is in use", __func__, ln);
		                
	}

	                                                  
	if (ln->ln_hold != NULL) {
		m_freem_list(ln->ln_hold);
		ln->ln_hold = NULL;
	}

	                                       
	VERIFY(ln->ln_rt->rt_llinfo == ln);
	if (ln->ln_rt->rt_llinfo_purge != NULL) {
		ln->ln_rt->rt_llinfo_purge(ln->ln_rt);
	}

	zfree(llinfo_nd6_zone, ln);
}

static void
nd6_llinfo_purge(struct rtentry *rt)
{
	struct llinfo_nd6 *ln = rt->rt_llinfo;

	RT_LOCK_ASSERT_HELD(rt);
	VERIFY(rt->rt_llinfo_purge == nd6_llinfo_purge && ln != NULL);

	if (ln->ln_llreach != NULL) {
		RT_CONVERT_LOCK(rt);
		ifnet_llreach_free(ln->ln_llreach);
		ln->ln_llreach = NULL;
	}
	ln->ln_lastused = 0;
}

static void
nd6_llinfo_get_ri(struct rtentry *rt, struct rt_reach_info *ri)
{
	struct llinfo_nd6 *ln = rt->rt_llinfo;
	struct if_llreach *lr = ln->ln_llreach;

	if (lr == NULL) {
		bzero(ri, sizeof(*ri));
		ri->ri_rssi = IFNET_RSSI_UNKNOWN;
		ri->ri_lqm = IFNET_LQM_THRESH_OFF;
		ri->ri_npm = IFNET_NPM_THRESH_UNKNOWN;
	} else {
		IFLR_LOCK(lr);
		                                       
		ifnet_lr2ri(lr, ri);
		                                                
		ri->ri_snd_expire =
		    ifnet_llreach_up2calexp(lr, ln->ln_lastused);
		IFLR_UNLOCK(lr);
	}
}

static void
nd6_llinfo_get_iflri(struct rtentry *rt, struct ifnet_llreach_info *iflri)
{
	struct llinfo_nd6 *ln = rt->rt_llinfo;
	struct if_llreach *lr = ln->ln_llreach;

	if (lr == NULL) {
		bzero(iflri, sizeof(*iflri));
		iflri->iflri_rssi = IFNET_RSSI_UNKNOWN;
		iflri->iflri_lqm = IFNET_LQM_THRESH_OFF;
		iflri->iflri_npm = IFNET_NPM_THRESH_UNKNOWN;
	} else {
		IFLR_LOCK(lr);
		                                            
		ifnet_lr2iflri(lr, iflri);
		                                              
		iflri->iflri_snd_expire =
		    ifnet_llreach_up2upexp(lr, ln->ln_lastused);
		IFLR_UNLOCK(lr);
	}
}

static void
nd6_llinfo_refresh(struct rtentry *rt)
{
	struct llinfo_nd6 *ln = rt->rt_llinfo;
	uint64_t timenow = net_uptime();
	  
                                                       
                           
    
	if (!ln || ln->ln_expire == 0 ||
	    (rt->rt_flags & RTF_STATIC) ||
	    !(rt->rt_flags & RTF_LLINFO)) {
		return;
	}

	if ((ln->ln_state > ND6_LLINFO_INCOMPLETE) &&
	    (ln->ln_state < ND6_LLINFO_PROBE)) {
		if (ln->ln_expire > timenow) {
			ln_setexpire(ln, timenow);
			ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_PROBE);
		}
	}
	return;
}

const char *
ndcache_state2str(short ndp_state)
{
	const char *ndp_state_str = "UNKNOWN";
	switch (ndp_state) {
	case ND6_LLINFO_PURGE:
		ndp_state_str = "ND6_LLINFO_PURGE";
		break;
	case ND6_LLINFO_NOSTATE:
		ndp_state_str = "ND6_LLINFO_NOSTATE";
		break;
	case ND6_LLINFO_INCOMPLETE:
		ndp_state_str = "ND6_LLINFO_INCOMPLETE";
		break;
	case ND6_LLINFO_REACHABLE:
		ndp_state_str = "ND6_LLINFO_REACHABLE";
		break;
	case ND6_LLINFO_STALE:
		ndp_state_str = "ND6_LLINFO_STALE";
		break;
	case ND6_LLINFO_DELAY:
		ndp_state_str = "ND6_LLINFO_DELAY";
		break;
	case ND6_LLINFO_PROBE:
		ndp_state_str = "ND6_LLINFO_PROBE";
		break;
	default:
		                       
		break;
	}
	return ndp_state_str;
}

void
ln_setexpire(struct llinfo_nd6 *ln, uint64_t expiry)
{
	ln->ln_expire = expiry;
}

static uint64_t
ln_getexpire(struct llinfo_nd6 *ln)
{
	struct timeval caltime;
	uint64_t expiry;

	if (ln->ln_expire != 0) {
		struct rtentry *rt = ln->ln_rt;

		VERIFY(rt != NULL);
		                                    
		getmicrotime(&caltime);

		rt->base_calendartime +=
		    NET_CALCULATE_CLOCKSKEW(caltime,
		    rt->base_calendartime, net_uptime(), rt->base_uptime);

		expiry = rt->base_calendartime +
		    ln->ln_expire - rt->base_uptime;
	} else {
		expiry = 0;
	}
	return expiry;
}

void
nd6_ifreset(struct ifnet *ifp)
{
	struct nd_ifinfo *ndi = ND_IFINFO(ifp);
	VERIFY(NULL != ndi);
	VERIFY(ndi->initialized);

	LCK_MTX_ASSERT(&ndi->lock, LCK_MTX_ASSERT_OWNED);
	ndi->linkmtu = ifp->if_mtu;
	ndi->chlim = IPV6_DEFHLIM;
	ndi->basereachable = REACHABLE_TIME;
	ndi->reachable = ND_COMPUTE_RTIME(ndi->basereachable);
	ndi->retrans = RETRANS_TIMER;
}

void
nd6_ifattach(struct ifnet *ifp)
{
	struct nd_ifinfo *ndi = ND_IFINFO(ifp);

	VERIFY(NULL != ndi);
	if (!ndi->initialized) {
		lck_mtx_init(&ndi->lock, nd_if_lock_grp, nd_if_lock_attr);
		ndi->flags = ND6_IFF_PERFORMNUD;
		ndi->flags |= ND6_IFF_DAD;
		ndi->initialized = TRUE;
	}

	lck_mtx_lock(&ndi->lock);

	if (!(ifp->if_flags & IFF_MULTICAST)) {
		ndi->flags |= ND6_IFF_IFDISABLED;
	}

	nd6_ifreset(ifp);
	lck_mtx_unlock(&ndi->lock);
	nd6_setmtu(ifp);

	nd6log0(info,
	    "Reinit'd ND information for interface %s\n",
	    if_name(ifp));
	return;
}

#if 0
  
                                                                             
          
   
void
nd6_ifdetach(struct nd_ifinfo *nd)
{
	                            
	FREE(nd, M_IP6NDP);
}
#endif

void
nd6_setmtu(struct ifnet *ifp)
{
	struct nd_ifinfo *ndi = ND_IFINFO(ifp);
	u_int32_t oldmaxmtu, maxmtu;

	if ((NULL == ndi) || (FALSE == ndi->initialized)) {
		return;
	}

	lck_mtx_lock(&ndi->lock);
	oldmaxmtu = ndi->maxmtu;

	  
                                                                  
                                                                
                                                                   
                                                                   
                                                                
                                                                  
                                                                  
                                                                
                                         
    
	maxmtu = ndi->maxmtu = ifp->if_mtu;

	  
                                                                 
                                                                     
                                                                     
                                                             
    
	if (oldmaxmtu >= IPV6_MMTU && ndi->maxmtu < IPV6_MMTU) {
		log(LOG_NOTICE, "nd6_setmtu: "
		    "new link MTU on %s (%u) is too small for IPv6\n",
		    if_name(ifp), (uint32_t)ndi->maxmtu);
	}
	ndi->linkmtu = ifp->if_mtu;
	lck_mtx_unlock(&ndi->lock);

	                                          
	if (maxmtu > in6_maxmtu) {
		in6_setmaxmtu();
	}
}

void
nd6_option_init(void *opt, int icmp6len, union nd_opts *ndopts)
{
	bzero(ndopts, sizeof(*ndopts));
	ndopts->nd_opts_search = (struct nd_opt_hdr *)opt;
	ndopts->nd_opts_last =
	    (struct nd_opt_hdr *)(((u_char *)opt) + icmp6len);

	if (icmp6len == 0) {
		ndopts->nd_opts_done = 1;
		ndopts->nd_opts_search = NULL;
	}
}

  
                      
   
struct nd_opt_hdr *
nd6_option(union nd_opts *ndopts)
{
	struct nd_opt_hdr *nd_opt;
	int olen;

	if (!ndopts) {
		panic("ndopts == NULL in nd6_option\n");
	}
	if (!ndopts->nd_opts_last) {
		panic("uninitialized ndopts in nd6_option\n");
	}
	if (!ndopts->nd_opts_search) {
		return NULL;
	}
	if (ndopts->nd_opts_done) {
		return NULL;
	}

	nd_opt = ndopts->nd_opts_search;

	                                               
	if ((caddr_t)&nd_opt->nd_opt_len >= (caddr_t)ndopts->nd_opts_last) {
		bzero(ndopts, sizeof(*ndopts));
		return NULL;
	}

	olen = nd_opt->nd_opt_len << 3;
	if (olen == 0) {
		  
                                                  
                                                     
     
		bzero(ndopts, sizeof(*ndopts));
		return NULL;
	}

	ndopts->nd_opts_search = (struct nd_opt_hdr *)((caddr_t)nd_opt + olen);
	if (ndopts->nd_opts_search > ndopts->nd_opts_last) {
		                                                
		bzero(ndopts, sizeof(*ndopts));
		return NULL;
	} else if (ndopts->nd_opts_search == ndopts->nd_opts_last) {
		                                      
		ndopts->nd_opts_done = 1;
		ndopts->nd_opts_search = NULL;
	}
	return nd_opt;
}

  
                             
                                                                        
                                     
   
int
nd6_options(union nd_opts *ndopts)
{
	struct nd_opt_hdr *nd_opt;
	int i = 0;

	if (ndopts == NULL) {
		panic("ndopts == NULL in nd6_options");
	}
	if (ndopts->nd_opts_last == NULL) {
		panic("uninitialized ndopts in nd6_options");
	}
	if (ndopts->nd_opts_search == NULL) {
		return 0;
	}

	while (1) {
		nd_opt = nd6_option(ndopts);
		if (nd_opt == NULL && ndopts->nd_opts_last == NULL) {
			  
                                                   
                                                      
      
			icmp6stat.icp6s_nd_badopt++;
			bzero(ndopts, sizeof(*ndopts));
			return -1;
		}

		if (nd_opt == NULL) {
			goto skip1;
		}

		switch (nd_opt->nd_opt_type) {
		case ND_OPT_SOURCE_LINKADDR:
		case ND_OPT_TARGET_LINKADDR:
		case ND_OPT_MTU:
		case ND_OPT_REDIRECTED_HEADER:
		case ND_OPT_NONCE:
			if (ndopts->nd_opt_array[nd_opt->nd_opt_type]) {
				nd6log(error,
				    "duplicated ND6 option found (type=%d)\n",
				    nd_opt->nd_opt_type);
				               
			} else {
				ndopts->nd_opt_array[nd_opt->nd_opt_type] =
				    nd_opt;
			}
			break;
		case ND_OPT_PREFIX_INFORMATION:
			if (ndopts->nd_opt_array[nd_opt->nd_opt_type] == 0) {
				ndopts->nd_opt_array[nd_opt->nd_opt_type] =
				    nd_opt;
			}
			ndopts->nd_opts_pi_end =
			    (struct nd_opt_prefix_info *)nd_opt;
			break;
		case ND_OPT_RDNSS:
		case ND_OPT_DNSSL:
		case ND_OPT_CAPTIVE_PORTAL:
			            
			break;
		case ND_OPT_ROUTE_INFO:
			if (nd6_process_rti) {
				if (ndopts->nd_opt_array[nd_opt->nd_opt_type] == 0) {
					ndopts->nd_opt_array[nd_opt->nd_opt_type]
					        = nd_opt;
				}
				ndopts->nd_opts_rti_end =
				    (struct nd_opt_route_info *)nd_opt;
				break;
			}
			OS_FALLTHROUGH;
		default:
			  
                                               
                                                     
      
			nd6log(debug,
			    "nd6_options: unsupported option %d - "
			    "option ignored\n", nd_opt->nd_opt_type);
		}

skip1:
		i++;
		if (i > nd6_maxndopt) {
			icmp6stat.icp6s_nd_toomanyopt++;
			nd6log(info, "too many loop in nd opt\n");
			break;
		}

		if (ndopts->nd_opts_done) {
			break;
		}
	}

	return 0;
}

struct nd6svc_arg {
	int draining;
	uint32_t killed;
	uint32_t aging_lazy;
	uint32_t aging;
	uint32_t sticky;
	uint32_t found;
};


static void
nd6_service_neighbor_cache(struct nd6svc_arg *ap, uint64_t timenow)
{
	struct llinfo_nd6 *ln;
	struct ifnet *ifp = NULL;
	boolean_t send_nc_failure_kev = FALSE;
	struct radix_node_head  *rnh = rt_tables[AF_INET6];

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);
again:
	  
                                                                   
                      
                           
                                                    
                                                                      
                             
   
                                                                         
                                                   
   
                                                                         
                                       
                                                                      
                                                                       
              
   
                                                                       
                                                                       
                                             
    
	if (send_nc_failure_kev && ifp != NULL &&
	    ifp->if_addrlen == IF_LLREACH_MAXLEN) {
		struct kev_msg ev_msg;
		struct kev_nd6_ndfailure nd6_ndfailure;
		bzero(&ev_msg, sizeof(ev_msg));
		bzero(&nd6_ndfailure, sizeof(nd6_ndfailure));
		ev_msg.vendor_code      = KEV_VENDOR_APPLE;
		ev_msg.kev_class        = KEV_NETWORK_CLASS;
		ev_msg.kev_subclass     = KEV_ND6_SUBCLASS;
		ev_msg.event_code       = KEV_ND6_NDFAILURE;

		nd6_ndfailure.link_data.if_family = ifp->if_family;
		nd6_ndfailure.link_data.if_unit = ifp->if_unit;
		strlcpy(nd6_ndfailure.link_data.if_name,
		    ifp->if_name,
		    sizeof(nd6_ndfailure.link_data.if_name));
		ev_msg.dv[0].data_ptr = &nd6_ndfailure;
		ev_msg.dv[0].data_length =
		    sizeof(nd6_ndfailure);
		dlil_post_complete_msg(NULL, &ev_msg);
	}

	send_nc_failure_kev = FALSE;
	ifp = NULL;
	  
                                                                  
                                                                    
                                                                  
                                                                  
                                                                      
                                                                   
                                                                     
                                                                    
                                                                     
                            
    
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);

	ln = llinfo_nd6.ln_next;
	while (ln != NULL && ln != &llinfo_nd6) {
		struct rtentry *rt;
		struct sockaddr_in6 *dst;
		struct llinfo_nd6 *next;
		u_int32_t retrans, flags;
		struct nd_ifinfo *ndi = NULL;
		boolean_t is_router = FALSE;

		                                              
		next = ln->ln_next;
		rt = ln->ln_rt;
		RT_LOCK(rt);

		                                      
		if (ln->ln_flags & ND6_LNF_TIMER_SKIP) {
			RT_UNLOCK(rt);
			ln = next;
			continue;
		}
		ap->found++;

		                                     
		if ((ifp = rt->rt_ifp) == NULL) {
			panic("%s: ln(%p) rt(%p) rt_ifp == NULL", __func__,
			    ln, rt);
			                
		}

		                                          
		if ((struct llinfo_nd6 *)rt->rt_llinfo != ln) {
			panic("%s: rt_llinfo(%p) is not equal to ln(%p)",
			    __func__, rt->rt_llinfo, ln);
			                
		}

		                                 
		dst = SIN6(rt_key(rt));
		if (dst == NULL) {
			panic("%s: rt(%p) key is NULL ln(%p)", __func__,
			    rt, ln);
			                
		}

		                                             
		ln->ln_flags |= ND6_LNF_TIMER_SKIP;

		if (ln->ln_expire == 0 || (rt->rt_flags & RTF_STATIC)) {
			ap->sticky++;
		} else if (ap->draining && (rt->rt_refcnt == 0)) {
			  
                                                      
                                              
      
			if (ln->ln_state > ND6_LLINFO_INCOMPLETE) {
				ND6_CACHE_STATE_TRANSITION(ln, (short)ND6_LLINFO_STALE);
			} else {
				ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_PURGE);
			}
			ln_setexpire(ln, timenow);
		}

		  
                                                             
                                                             
                                                          
                                                       
     
		if (ln->ln_expire > timenow) {
			switch (ln->ln_state) {
			case ND6_LLINFO_STALE:
				ap->aging_lazy++;
				break;
			default:
				ap->aging++;
				break;
			}
			RT_UNLOCK(rt);
			ln = next;
			continue;
		}

		ndi = ND_IFINFO(ifp);
		VERIFY(ndi->initialized);
		retrans = ndi->retrans;
		flags = ndi->flags;

		RT_LOCK_ASSERT_HELD(rt);
		is_router = (rt->rt_flags & RTF_ROUTER) ? TRUE : FALSE;

		switch (ln->ln_state) {
		case ND6_LLINFO_INCOMPLETE:
			if (ln->ln_asked < nd6_mmaxtries) {
				struct ifnet *exclifp = ln->ln_exclifp;
				ln->ln_asked++;
				ln_setexpire(ln, timenow + retrans / 1000);
				RT_ADDREF_LOCKED(rt);
				RT_UNLOCK(rt);
				lck_mtx_unlock(rnh_lock);
				if (ip6_forwarding) {
					nd6_prproxy_ns_output(ifp, exclifp,
					    NULL, &dst->sin6_addr, ln);
				} else {
					nd6_ns_output(ifp, NULL,
					    &dst->sin6_addr, ln, NULL);
				}
				RT_REMREF(rt);
				ap->aging++;
				lck_mtx_lock(rnh_lock);
			} else {
				struct mbuf *m = ln->ln_hold;
				ln->ln_hold = NULL;
				send_nc_failure_kev = is_router;
				if (m != NULL) {
					RT_ADDREF_LOCKED(rt);
					RT_UNLOCK(rt);
					lck_mtx_unlock(rnh_lock);

					struct mbuf *mnext;
					while (m) {
						mnext = m->m_nextpkt;
						m->m_nextpkt = NULL;
						m->m_pkthdr.rcvif = ifp;
						icmp6_error_flag(m, ICMP6_DST_UNREACH,
						    ICMP6_DST_UNREACH_ADDR, 0, 0);
						m = mnext;
					}
				} else {
					RT_ADDREF_LOCKED(rt);
					RT_UNLOCK(rt);
					lck_mtx_unlock(rnh_lock);
				}

				  
                                               
                       
       
				route_event_enqueue_nwk_wq_entry(rt, NULL,
				    ROUTE_LLENTRY_UNREACH, NULL, FALSE);
				nd6_free(rt);
				ap->killed++;
				lck_mtx_lock(rnh_lock);
				  
                                                          
                                            
                                                            
            
      
                                                                   
                                                                    
                  
       
				if (is_router) {
					struct route_event rt_ev;
					route_event_init(&rt_ev, rt, NULL, ROUTE_LLENTRY_UNREACH);
					(void) rnh->rnh_walktree(rnh, route_event_walktree, (void *)&rt_ev);
				}
				rtfree_locked(rt);
			}
			LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);
			goto again;

		case ND6_LLINFO_REACHABLE:
			if (ln->ln_expire != 0) {
				ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_STALE);
				ln_setexpire(ln, timenow + nd6_gctimer);
				ap->aging_lazy++;
				  
                                               
                       
       
				route_event_enqueue_nwk_wq_entry(rt, NULL,
				    ROUTE_LLENTRY_STALE, NULL, TRUE);

				RT_ADDREF_LOCKED(rt);
				RT_UNLOCK(rt);
				if (is_router) {
					struct route_event rt_ev;
					route_event_init(&rt_ev, rt, NULL, ROUTE_LLENTRY_STALE);
					(void) rnh->rnh_walktree(rnh, route_event_walktree, (void *)&rt_ev);
				}
				rtfree_locked(rt);
			} else {
				RT_UNLOCK(rt);
			}
			break;

		case ND6_LLINFO_STALE:
		case ND6_LLINFO_PURGE:
			                                      
			if (ln->ln_expire != 0) {
				RT_ADDREF_LOCKED(rt);
				RT_UNLOCK(rt);
				lck_mtx_unlock(rnh_lock);
				nd6_free(rt);
				ap->killed++;
				lck_mtx_lock(rnh_lock);
				rtfree_locked(rt);
				goto again;
			} else {
				RT_UNLOCK(rt);
			}
			break;

		case ND6_LLINFO_DELAY:
			if ((flags & ND6_IFF_PERFORMNUD) != 0) {
				                 
				ln->ln_asked = 1;
				ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_PROBE);
				ln_setexpire(ln, timenow + retrans / 1000);
				RT_ADDREF_LOCKED(rt);
				RT_UNLOCK(rt);
				lck_mtx_unlock(rnh_lock);
				nd6_ns_output(ifp, &dst->sin6_addr,
				    &dst->sin6_addr, ln, NULL);
				RT_REMREF(rt);
				ap->aging++;
				lck_mtx_lock(rnh_lock);
				goto again;
			}
			ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_STALE);                  
			ln_setexpire(ln, timenow + nd6_gctimer);
			RT_UNLOCK(rt);
			ap->aging_lazy++;
			break;

		case ND6_LLINFO_PROBE:
			if (ln->ln_asked < nd6_umaxtries) {
				ln->ln_asked++;
				ln_setexpire(ln, timenow + retrans / 1000);
				RT_ADDREF_LOCKED(rt);
				RT_UNLOCK(rt);
				lck_mtx_unlock(rnh_lock);
				nd6_ns_output(ifp, &dst->sin6_addr,
				    &dst->sin6_addr, ln, NULL);
				RT_REMREF(rt);
				ap->aging++;
				lck_mtx_lock(rnh_lock);
			} else {
				is_router = (rt->rt_flags & RTF_ROUTER) ? TRUE : FALSE;
				send_nc_failure_kev = is_router;
				RT_ADDREF_LOCKED(rt);
				RT_UNLOCK(rt);
				lck_mtx_unlock(rnh_lock);
				nd6_free(rt);
				ap->killed++;

				  
                                               
                       
       
				route_event_enqueue_nwk_wq_entry(rt, NULL,
				    ROUTE_LLENTRY_UNREACH, NULL, FALSE);

				lck_mtx_lock(rnh_lock);
				  
                                                          
                                            
                                                            
            
      
                                                                   
                                                                    
                  
       
				if (is_router) {
					struct route_event rt_ev;
					route_event_init(&rt_ev, rt, NULL, ROUTE_LLENTRY_UNREACH);
					(void) rnh->rnh_walktree(rnh,
					    route_event_walktree, (void *)&rt_ev);
				}
				rtfree_locked(rt);
			}
			LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);
			goto again;

		default:
			RT_UNLOCK(rt);
			break;
		}
		ln = next;
	}
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);

	                                         
	ln = llinfo_nd6.ln_next;
	while (ln != NULL && ln != &llinfo_nd6) {
		struct rtentry *rt = ln->ln_rt;
		struct llinfo_nd6 *next = ln->ln_next;

		RT_LOCK_SPIN(rt);
		if (ln->ln_flags & ND6_LNF_TIMER_SKIP) {
			ln->ln_flags &= ~ND6_LNF_TIMER_SKIP;
		}
		RT_UNLOCK(rt);
		ln = next;
	}
}

static void
nd6_service_expired_default_router(struct nd6svc_arg *ap, uint64_t timenow)
{
	struct nd_defrouter *dr = NULL;
	struct nd_defrouter *ndr = NULL;
	struct nd_drhead nd_defrouter_tmp;
	                                
	TAILQ_INIT(&nd_defrouter_tmp);

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(nd6_mutex);

	TAILQ_FOREACH_SAFE(dr, &nd_defrouter_list, dr_entry, ndr) {
		ap->found++;
		if (dr->expire != 0 && dr->expire < timenow) {
			VERIFY(dr->ifp != NULL);
			in6_ifstat_inc(dr->ifp, ifs6_defrtr_expiry_cnt);
			if ((dr->stateflags & NDDRF_INELIGIBLE) == 0) {
				in6_event_enqueue_nwk_wq_entry(IN6_NDP_RTR_EXPIRY, dr->ifp,
				    &dr->rtaddr, dr->rtlifetime);
			}
			if (dr->ifp != NULL &&
			    dr->ifp->if_type == IFT_CELLULAR) {
				  
                                                
                                      
                                               
                                                         
                                     
                                                 
                                               
                 
                                             
                           
                                              
                                                  
                                                       
                                           
                                                      
                                               
                                                     
                                                         
                                                        
                                           
       
				dr->expire += dr->rtlifetime;
				nd6log2(debug,
				    "%s: Refreshing expired default router entry "
				    "%s for interface %s\n", __func__,
				    ip6_sprintf(&dr->rtaddr), if_name(dr->ifp));
			} else {
				ap->killed++;
				  
                                                
                                   
                                                 
                                           
                                                    
                   
                                                        
       
				TAILQ_REMOVE(&nd_defrouter_list, dr, dr_entry);
				TAILQ_INSERT_TAIL(&nd_defrouter_tmp, dr, dr_entry);
			}
		} else {
			if (dr->expire == 0 || (dr->stateflags & NDDRF_STATIC)) {
				ap->sticky++;
			} else {
				ap->aging_lazy++;
			}
		}
	}

	  
                                               
                                                   
                                                         
                                                
                                       
   
                                                  
                                            
                    
    
	TAILQ_FOREACH_SAFE(dr, &nd_defrouter_tmp, dr_entry, ndr) {
		TAILQ_REMOVE(&nd_defrouter_tmp, dr, dr_entry);
		defrtrlist_del(dr, NULL);
		NDDR_REMREF(dr);                                   
	}

	                                                    
	  
                                                                
                                                                 
                                                                   
                        
    
	if (trigger_v6_defrtr_select) {
		defrouter_select(NULL, NULL);
		trigger_v6_defrtr_select = FALSE;
	}
	lck_mtx_unlock(nd6_mutex);
}

static void
nd6_service_expired_route_info(struct nd6svc_arg *ap, uint64_t timenow)
{
	struct nd_route_info *rti = NULL;
	struct nd_route_info *rti_next = NULL;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(nd6_mutex);
	nd6_rti_list_wait(__func__);

	TAILQ_FOREACH_SAFE(rti, &nd_rti_list, nd_rti_entry, rti_next) {
		struct nd_defrouter *dr = NULL;
		struct nd_defrouter *ndr = NULL;
		struct nd_route_info rti_tmp = {};

		rti_tmp.nd_rti_prefix = rti->nd_rti_prefix;
		rti_tmp.nd_rti_prefixlen = rti->nd_rti_prefixlen;
		TAILQ_INIT(&rti_tmp.nd_rti_router_list);

		TAILQ_FOREACH_SAFE(dr, &rti->nd_rti_router_list, dr_entry, ndr) {
			ap->found++;
			if (dr->expire != 0 && dr->expire < timenow) {
				VERIFY(dr->ifp != NULL);
				if (dr->ifp != NULL &&
				    dr->ifp->if_type == IFT_CELLULAR) {
					  
                                                
                                                         
        
					dr->expire += dr->rtlifetime;
					nd6log2(debug,
					    "%s: Refreshing expired default router entry "
					    "%s for interface %s\n", __func__,
					    ip6_sprintf(&dr->rtaddr), if_name(dr->ifp));
				} else {
					ap->killed++;
					  
                                                     
                                    
                                                         
        
					TAILQ_REMOVE(&rti->nd_rti_router_list, dr, dr_entry);
					TAILQ_INSERT_TAIL(&rti_tmp.nd_rti_router_list, dr, dr_entry);
				}
			} else {
				if (dr->expire == 0 || (dr->stateflags & NDDRF_STATIC)) {
					ap->sticky++;
				} else {
					ap->aging_lazy++;
				}
			}
		}

		  
                                                
                                                    
                                                          
                                                 
                                        
    
                                                   
                                             
                     
     
		TAILQ_FOREACH_SAFE(dr, &rti_tmp.nd_rti_router_list, dr_entry, ndr) {
			TAILQ_REMOVE(&rti_tmp.nd_rti_router_list, dr, dr_entry);
			defrtrlist_del(dr, &rti->nd_rti_router_list);
			NDDR_REMREF(dr);                                   
		}

		  
                                                                  
                                                                 
     
		if (TAILQ_EMPTY(&rti->nd_rti_router_list)) {
			TAILQ_REMOVE(&nd_rti_list, rti, nd_rti_entry);
			ndrti_free(rti);
		}
	}

	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_OWNED);
	nd6_rti_list_signal_done();
	lck_mtx_unlock(nd6_mutex);
}


  
                                           
  
         
                                                          
  
              
                                                               
                                                            
                               
  
                                                             
                                                  
   
static boolean_t
nd6_handle_duplicated_ip6_addr(struct in6_ifaddr *ia6)
{
	uint8_t collision_count;
	int error = 0;
	struct in6_ifaddr *new_ia6;
	struct nd_prefix *pr;
	struct ifnet *ifp;

	LCK_RW_ASSERT(&in6_ifaddr_rwlock, LCK_RW_ASSERT_EXCLUSIVE);
	IFA_LOCK_ASSERT_HELD(&ia6->ia_ifa);

	                                
	collision_count = ia6->ia6_cga_collision_count;
	if (collision_count >= ip6_cga_conflict_retries) {
		return FALSE;
	}

	                                               
	pr = ia6->ia6_ndpr;
	if (pr == NULL) {
		return FALSE;
	}
	NDPR_ADDREF(pr);
	ifp = pr->ndpr_ifp;
	log(LOG_DEBUG,
	    "%s: %s duplicated (collision count %d)\n",
	    ifp->if_xname, ip6_sprintf(&ia6->ia_addr.sin6_addr),
	    collision_count);

	                            
	IFA_UNLOCK(&ia6->ia_ifa);
	lck_rw_done(&in6_ifaddr_rwlock);
	in6_purgeaddr(&ia6->ia_ifa);

	                                                     
	collision_count++;
	new_ia6 = in6_pfx_newpersistaddr(pr, 1, &error, FALSE, collision_count);
	if (new_ia6 != NULL) {
		log(LOG_DEBUG,
		    "%s: %s new (collision count %d)\n",
		    ifp->if_xname, ip6_sprintf(&new_ia6->ia_addr.sin6_addr),
		    collision_count);
		IFA_LOCK(&new_ia6->ia_ifa);
		NDPR_LOCK(pr);
		new_ia6->ia6_ndpr = pr;
		NDPR_ADDREF(pr);                         
		pr->ndpr_addrcnt++;
		VERIFY(pr->ndpr_addrcnt != 0);
		NDPR_UNLOCK(pr);
		IFA_UNLOCK(&new_ia6->ia_ifa);
		IFA_REMREF(&new_ia6->ia_ifa);
	} else {
		log(LOG_ERR, "%s: in6_pfx_newpersistaddr failed %d\n",
		    __func__, error);
	}

	                                    
	NDPR_REMREF(pr);
	return TRUE;
}

static boolean_t
secured_address_is_duplicated(int flags)
{
#define _IN6_IFF_DUPLICATED_AUTOCONF_SECURED                            \
	(IN6_IFF_DUPLICATED | IN6_IFF_AUTOCONF | IN6_IFF_SECURED)
	return (flags & _IN6_IFF_DUPLICATED_AUTOCONF_SECURED) ==
	       _IN6_IFF_DUPLICATED_AUTOCONF_SECURED;
}

static void
nd6_service_ip6_addr(struct nd6svc_arg *ap, uint64_t timenow)
{
	struct in6_ifaddr *ia6 = NULL;
	struct in6_ifaddr *nia6 = NULL;
	  
                               
                                                             
                                                                   
                                                           
    

addrloop:
	lck_rw_lock_exclusive(&in6_ifaddr_rwlock);

	TAILQ_FOREACH_SAFE(ia6, &in6_ifaddrhead, ia6_link, nia6) {
		int oldflags = ia6->ia6_flags;
		ap->found++;
		IFA_LOCK(&ia6->ia_ifa);
		  
                                                 
                                                   
                                                 
                                                
     
		IFA_ADDREF_LOCKED(&ia6->ia_ifa);

		                                          
		if (secured_address_is_duplicated(ia6->ia6_flags) &&
		    nd6_handle_duplicated_ip6_addr(ia6)) {
			  
                                               
                                               
                                              
                              
      
			IFA_REMREF(&ia6->ia_ifa);
			goto addrloop;
		}

		                            
		if (IFA6_IS_INVALID(ia6, timenow)) {
			  
                                               
                                                        
                                                         
                                                
                                                     
                                                    
                                                       
                   
      
			if (ip6_use_tempaddr &&
			    (ia6->ia6_flags & IN6_IFF_TEMPORARY) != 0) {
				  
                                          
                                               
                                                 
                                                 
                                                
                                            
       
				IFA_UNLOCK(&ia6->ia_ifa);
				lck_rw_done(&in6_ifaddr_rwlock);
				(void) regen_tmpaddr(ia6);
			} else {
				IFA_UNLOCK(&ia6->ia_ifa);
				lck_rw_done(&in6_ifaddr_rwlock);
			}

			  
                                           
                                                     
                                               
                          
      
			in6_purgeaddr(&ia6->ia_ifa);
			ap->killed++;

			if ((ia6->ia6_flags & IN6_IFF_TEMPORARY) == 0) {
				in6_ifstat_inc(ia6->ia_ifa.ifa_ifp, ifs6_addr_expiry_cnt);
				in6_event_enqueue_nwk_wq_entry(IN6_NDP_ADDR_EXPIRY,
				    ia6->ia_ifa.ifa_ifp, &ia6->ia_addr.sin6_addr,
				    0);
			}
			                                         
			IFA_REMREF(&ia6->ia_ifa);
			goto addrloop;
		}
		  
                                                             
                                                                
                                                                
                                          
     
		if (IFA6_IS_INVALID(ia6, timenow + 3 * nd6_prune_lazy)) {
			ap->aging++;
		} else {
			ap->aging_lazy++;
		}
		IFA_LOCK_ASSERT_HELD(&ia6->ia_ifa);
		if (IFA6_IS_DEPRECATED(ia6, timenow)) {
			ia6->ia6_flags |= IN6_IFF_DEPRECATED;

			if ((oldflags & IN6_IFF_DEPRECATED) == 0) {
				  
                                                              
                          
                                                                
                                                                    
                
       
				if ((ia6->ia6_flags & IN6_IFF_TEMPORARY) == 0) {
					in6_event_enqueue_nwk_wq_entry(IN6_ADDR_MARKED_DEPRECATED,
					    ia6->ia_ifa.ifa_ifp, &ia6->ia_addr.sin6_addr,
					    0);
				}
			}
			  
                                                        
                                       
      
			if (ip6_use_tempaddr &&
			    (ia6->ia6_flags & IN6_IFF_TEMPORARY) != 0 &&
			    (oldflags & IN6_IFF_DEPRECATED) == 0) {
				                    
				IFA_UNLOCK(&ia6->ia_ifa);
				lck_rw_done(&in6_ifaddr_rwlock);
				if (regen_tmpaddr(ia6) == 0) {
					  
                                  
                  
                                         
                                         
                                      
                                         
                                   
                                          
                                           
                                          
        
					                             
					IFA_REMREF(&ia6->ia_ifa);
					goto addrloop;
				}
				lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
			} else {
				IFA_UNLOCK(&ia6->ia_ifa);
			}
		} else {
			  
                                                   
                
      
			ia6->ia6_flags &= ~IN6_IFF_DEPRECATED;
			IFA_UNLOCK(&ia6->ia_ifa);
		}
		LCK_RW_ASSERT(&in6_ifaddr_rwlock, LCK_RW_ASSERT_EXCLUSIVE);
		                                         
		IFA_REMREF(&ia6->ia_ifa);
	}
	lck_rw_done(&in6_ifaddr_rwlock);
}

static void
nd6_service_expired_prefix(struct nd6svc_arg *ap, uint64_t timenow)
{
	struct nd_prefix *pr = NULL;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(nd6_mutex);
	                        
	pr = nd_prefix.lh_first;
	while (pr != NULL) {
		ap->found++;
		  
                           
                                                             
                             
     
		NDPR_LOCK(pr);
		if (pr->ndpr_stateflags & NDPRF_PROCESSED_SERVICE ||
		    pr->ndpr_stateflags & NDPRF_DEFUNCT) {
			pr->ndpr_stateflags |= NDPRF_PROCESSED_SERVICE;
			NDPR_UNLOCK(pr);
			pr = pr->ndpr_next;
			continue;
		}
		if (pr->ndpr_expire != 0 && pr->ndpr_expire < timenow) {
			  
                                                  
                                                 
      
			pr->ndpr_stateflags |= NDPRF_PROCESSED_SERVICE;
			NDPR_ADDREF(pr);
			prelist_remove(pr);
			NDPR_UNLOCK(pr);

			in6_ifstat_inc(pr->ndpr_ifp, ifs6_pfx_expiry_cnt);
			in6_event_enqueue_nwk_wq_entry(IN6_NDP_PFX_EXPIRY,
			    pr->ndpr_ifp, &pr->ndpr_prefix.sin6_addr,
			    0);
			NDPR_REMREF(pr);
			pfxlist_onlink_check();
			pr = nd_prefix.lh_first;
			ap->killed++;
		} else {
			if (pr->ndpr_expire == 0 ||
			    (pr->ndpr_stateflags & NDPRF_STATIC)) {
				ap->sticky++;
			} else {
				ap->aging_lazy++;
			}
			pr->ndpr_stateflags |= NDPRF_PROCESSED_SERVICE;
			NDPR_UNLOCK(pr);
			pr = pr->ndpr_next;
		}
	}
	LIST_FOREACH(pr, &nd_prefix, ndpr_entry) {
		NDPR_LOCK(pr);
		pr->ndpr_stateflags &= ~NDPRF_PROCESSED_SERVICE;
		NDPR_UNLOCK(pr);
	}
	lck_mtx_unlock(nd6_mutex);
}


  
                                                                   
   
static void
nd6_service(void *arg)
{
	struct nd6svc_arg *ap = arg;
	uint64_t timenow;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);
	  
                                                           
                                                 
    
	while (nd6_service_busy) {
		nd6log2(debug, "%s: %s is blocked by %d waiters\n",
		    __func__, ap->draining ? "drainer" : "timer",
		    nd6_service_waiters);
		nd6_service_waiters++;
		(void) msleep(nd6_service_wc, rnh_lock, (PZERO - 1),
		    __func__, NULL);
		LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);
	}

	                                                    
	nd6_service_busy = TRUE;
	net_update_uptime();
	timenow = net_uptime();

	                                                
	nd6_service_neighbor_cache(ap, timenow);

	  
                                                   
                                                  
    
	lck_mtx_unlock(rnh_lock);

	                                                
	nd6_service_expired_default_router(ap, timenow);
	                                                           
	nd6_service_expired_route_info(ap, timenow);

	                                                         
	nd6_service_ip6_addr(ap, timenow);

	                                               
	nd6_service_expired_prefix(ap, timenow);

	lck_mtx_lock(rnh_lock);
	                                  
	nd6_service_busy = FALSE;
	if (nd6_service_waiters > 0) {
		nd6_service_waiters = 0;
		wakeup(nd6_service_wc);
	}
}

static int nd6_need_draining = 0;

void
nd6_drain(void *arg)
{
#pragma unused(arg)
	nd6log2(debug, "%s: draining ND6 entries\n", __func__);

	lck_mtx_lock(rnh_lock);
	nd6_need_draining = 1;
	nd6_sched_timeout(NULL, NULL);
	lck_mtx_unlock(rnh_lock);
}

  
                                                                       
                                                                       
                                                                   
                                                                    
   
static void
nd6_timeout(void *arg)
{
	struct nd6svc_arg sarg;
	uint32_t buf;

	lck_mtx_lock(rnh_lock);
	bzero(&sarg, sizeof(sarg));
	if (nd6_need_draining != 0) {
		nd6_need_draining = 0;
		sarg.draining = 1;
	}
	nd6_service(&sarg);
	nd6log2(debug, "%s: found %u, aging_lazy %u, aging %u, "
	    "sticky %u, killed %u\n", __func__, sarg.found, sarg.aging_lazy,
	    sarg.aging, sarg.sticky, sarg.killed);
	                                            
	nd6_timeout_run--;
	VERIFY(nd6_timeout_run >= 0 && nd6_timeout_run < 2);
	if (arg == &nd6_fast_timer_on) {
		nd6_fast_timer_on = FALSE;
	}
	if (sarg.aging_lazy > 0 || sarg.aging > 0 || nd6_sched_timeout_want) {
		struct timeval atv, ltv, *leeway;
		int lazy = nd6_prune_lazy;

		if (sarg.aging > 0 || lazy < 1) {
			atv.tv_usec = 0;
			atv.tv_sec = nd6_prune;
			leeway = NULL;
		} else {
			VERIFY(lazy >= 1);
			atv.tv_usec = 0;
			atv.tv_sec = MAX(nd6_prune, lazy);
			ltv.tv_usec = 0;
			read_frandom(&buf, sizeof(buf));
			ltv.tv_sec = MAX(buf % lazy, 1) * 2;
			leeway = &ltv;
		}
		nd6_sched_timeout(&atv, leeway);
	} else if (nd6_debug) {
		nd6log2(debug, "%s: not rescheduling timer\n", __func__);
	}
	lck_mtx_unlock(rnh_lock);
}

void
nd6_sched_timeout(struct timeval *atv, struct timeval *ltv)
{
	struct timeval tv;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);
	if (atv == NULL) {
		tv.tv_usec = 0;
		tv.tv_sec = MAX(nd6_prune, 1);
		atv = &tv;
		ltv = NULL;                        
	}
	                                      
	if (nd6_timeout_run == 0) {
		if (ltv == NULL) {
			nd6log2(debug, "%s: timer scheduled in "
			    "T+%llus.%lluu (demand %d)\n", __func__,
			    (uint64_t)atv->tv_sec, (uint64_t)atv->tv_usec,
			    nd6_sched_timeout_want);
			nd6_fast_timer_on = TRUE;
			timeout(nd6_timeout, &nd6_fast_timer_on, tvtohz(atv));
		} else {
			nd6log2(debug, "%s: timer scheduled in "
			    "T+%llus.%lluu with %llus.%lluu leeway "
			    "(demand %d)\n", __func__, (uint64_t)atv->tv_sec,
			    (uint64_t)atv->tv_usec, (uint64_t)ltv->tv_sec,
			    (uint64_t)ltv->tv_usec, nd6_sched_timeout_want);
			nd6_fast_timer_on = FALSE;
			timeout_with_leeway(nd6_timeout, NULL,
			    tvtohz(atv), tvtohz(ltv));
		}
		nd6_timeout_run++;
		nd6_sched_timeout_want = 0;
	} else if (nd6_timeout_run == 1 && ltv == NULL &&
	    nd6_fast_timer_on == FALSE) {
		nd6log2(debug, "%s: fast timer scheduled in "
		    "T+%llus.%lluu (demand %d)\n", __func__,
		    (uint64_t)atv->tv_sec, (uint64_t)atv->tv_usec,
		    nd6_sched_timeout_want);
		nd6_fast_timer_on = TRUE;
		nd6_sched_timeout_want = 0;
		nd6_timeout_run++;
		timeout(nd6_timeout, &nd6_fast_timer_on, tvtohz(atv));
	} else {
		if (ltv == NULL) {
			nd6log2(debug, "%s: not scheduling timer: "
			    "timers %d, fast_timer %d, T+%llus.%lluu\n",
			    __func__, nd6_timeout_run, nd6_fast_timer_on,
			    (uint64_t)atv->tv_sec, (uint64_t)atv->tv_usec);
		} else {
			nd6log2(debug, "%s: not scheduling timer: "
			    "timers %d, fast_timer %d, T+%llus.%lluu "
			    "with %llus.%lluu leeway\n", __func__,
			    nd6_timeout_run, nd6_fast_timer_on,
			    (uint64_t)atv->tv_sec, (uint64_t)atv->tv_usec,
			    (uint64_t)ltv->tv_sec, (uint64_t)ltv->tv_usec);
		}
	}
}

  
                                               
   
void
nd6_post_msg(u_int32_t code, struct nd_prefix_list *prefix_list,
    u_int32_t list_length, u_int32_t mtu)
{
	struct kev_msg ev_msg;
	struct kev_nd6_ra_data nd6_ra_msg_data;
	struct nd_prefix_list *itr = prefix_list;

	bzero(&ev_msg, sizeof(struct kev_msg));
	ev_msg.vendor_code      = KEV_VENDOR_APPLE;
	ev_msg.kev_class        = KEV_NETWORK_CLASS;
	ev_msg.kev_subclass     = KEV_ND6_SUBCLASS;
	ev_msg.event_code       = code;

	bzero(&nd6_ra_msg_data, sizeof(nd6_ra_msg_data));

	if (mtu > 0 && mtu >= IPV6_MMTU) {
		nd6_ra_msg_data.mtu = mtu;
		nd6_ra_msg_data.flags |= KEV_ND6_DATA_VALID_MTU;
	}

	if (list_length > 0 && prefix_list != NULL) {
		nd6_ra_msg_data.list_length = list_length;
		nd6_ra_msg_data.flags |= KEV_ND6_DATA_VALID_PREFIX;
	}

	while (itr != NULL && nd6_ra_msg_data.list_index < list_length) {
		bcopy(&itr->pr.ndpr_prefix, &nd6_ra_msg_data.prefix.prefix,
		    sizeof(nd6_ra_msg_data.prefix.prefix));
		nd6_ra_msg_data.prefix.raflags = itr->pr.ndpr_raf;
		nd6_ra_msg_data.prefix.prefixlen = itr->pr.ndpr_plen;
		nd6_ra_msg_data.prefix.origin = PR_ORIG_RA;
		nd6_ra_msg_data.prefix.vltime = itr->pr.ndpr_vltime;
		nd6_ra_msg_data.prefix.pltime = itr->pr.ndpr_pltime;
		nd6_ra_msg_data.prefix.expire = ndpr_getexpire(&itr->pr);
		nd6_ra_msg_data.prefix.flags = itr->pr.ndpr_stateflags;
		nd6_ra_msg_data.prefix.refcnt = itr->pr.ndpr_addrcnt;
		nd6_ra_msg_data.prefix.if_index = itr->pr.ndpr_ifp->if_index;

		                         
		ev_msg.dv[0].data_ptr           = &nd6_ra_msg_data;
		ev_msg.dv[0].data_length        = sizeof(nd6_ra_msg_data);
		ev_msg.dv[1].data_length        = 0;
		dlil_post_complete_msg(NULL, &ev_msg);

		                                  
		bzero(&nd6_ra_msg_data.prefix, sizeof(nd6_ra_msg_data.prefix));
		itr = itr->next;
		nd6_ra_msg_data.list_index++;
	}
}

  
                                                      
   
static int
regen_tmpaddr(struct in6_ifaddr *ia6)
{
	struct ifaddr *ifa;
	struct ifnet *ifp;
	struct in6_ifaddr *public_ifa6 = NULL;
	uint64_t timenow = net_uptime();

	ifp = ia6->ia_ifa.ifa_ifp;
	ifnet_lock_shared(ifp);
	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
		struct in6_ifaddr *it6;

		IFA_LOCK(ifa);
		if (ifa->ifa_addr->sa_family != AF_INET6) {
			IFA_UNLOCK(ifa);
			continue;
		}
		it6 = (struct in6_ifaddr *)ifa;

		                                   
		if ((it6->ia6_flags & IN6_IFF_AUTOCONF) == 0) {
			IFA_UNLOCK(ifa);
			continue;
		}
		                                                        
		if (it6->ia6_ndpr == NULL || it6->ia6_ndpr != ia6->ia6_ndpr) {
			IFA_UNLOCK(ifa);
			continue;
		}
		  
                                                            
                                                              
                                                                 
                                                                
                   
     
		if ((it6->ia6_flags & IN6_IFF_TEMPORARY) != 0 &&
		    !IFA6_IS_DEPRECATED(it6, timenow)) {
			IFA_UNLOCK(ifa);
			if (public_ifa6 != NULL) {
				IFA_REMREF(&public_ifa6->ia_ifa);
			}
			public_ifa6 = NULL;
			break;
		}

		  
                                                               
                                                               
                                                                
                             
     
		if (!IFA6_IS_DEPRECATED(it6, timenow)) {
			IFA_ADDREF_LOCKED(ifa);                      
			IFA_UNLOCK(ifa);
			if (public_ifa6 != NULL) {
				IFA_REMREF(&public_ifa6->ia_ifa);
			}
			public_ifa6 = it6;
		} else {
			IFA_UNLOCK(ifa);
		}
	}
	ifnet_lock_done(ifp);

	if (public_ifa6 != NULL) {
		int e;

		if ((e = in6_tmpifadd(public_ifa6, 0)) != 0) {
			log(LOG_NOTICE, "regen_tmpaddr: failed to create a new"
			    " tmp addr,errno=%d\n", e);
			IFA_REMREF(&public_ifa6->ia_ifa);
			return -1;
		}
		IFA_REMREF(&public_ifa6->ia_ifa);
		return 0;
	}

	return -1;
}

static void
nd6_purge_interface_default_routers(struct ifnet *ifp)
{
	struct nd_defrouter *dr = NULL;
	struct nd_defrouter *ndr = NULL;
	struct nd_drhead nd_defrouter_tmp = {};

	TAILQ_INIT(&nd_defrouter_tmp);

	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_OWNED);

	TAILQ_FOREACH_SAFE(dr, &nd_defrouter_list, dr_entry, ndr) {
		if (dr->ifp != ifp) {
			continue;
		}
		  
                                              
                                 
                                               
                                         
                                                  
                 
                                                       
    
                                                      
                                 
                                                   
                                                   
                                             
                   
                                               
                                    
    
                                                     
                                                 
     
		TAILQ_REMOVE(&nd_defrouter_list, dr, dr_entry);

		if (dr->stateflags & NDDRF_INSTALLED) {
			TAILQ_INSERT_TAIL(&nd_defrouter_tmp, dr, dr_entry);
		} else {
			TAILQ_INSERT_HEAD(&nd_defrouter_tmp, dr, dr_entry);
		}
	}

	  
                                                  
                                            
                    
   
                                                   
                                                   
                                   
    
	TAILQ_FOREACH_SAFE(dr, &nd_defrouter_tmp, dr_entry, ndr) {
		TAILQ_REMOVE(&nd_defrouter_tmp, dr, dr_entry);
		defrtrlist_del(dr, NULL);
		NDDR_REMREF(dr);                                   
	}
}

static void
nd6_purge_interface_prefixes(struct ifnet *ifp)
{
	boolean_t removed = FALSE;
	struct nd_prefix *pr = NULL;
	struct nd_prefix *npr = NULL;

	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_OWNED);

	                                         
	for (pr = nd_prefix.lh_first; pr; pr = npr) {
		NDPR_LOCK(pr);
		npr = pr->ndpr_next;
		if (pr->ndpr_ifp == ifp &&
		    !(pr->ndpr_stateflags & NDPRF_DEFUNCT)) {
			  
                                                     
                                                      
                                                   
                                                   
      
			pr->ndpr_addrcnt = 0;

			  
                                                   
                                                    
                                                     
                                                          
                
                                
      
			NDPR_ADDREF(pr);
			prelist_remove(pr);
			NDPR_UNLOCK(pr);
			NDPR_REMREF(pr);
			removed = TRUE;
			npr = nd_prefix.lh_first;
		} else {
			NDPR_UNLOCK(pr);
		}
	}
	if (removed) {
		pfxlist_onlink_check();
	}
}

static void
nd6_router_select_rti_entries(struct ifnet *ifp)
{
	struct nd_route_info *rti = NULL;
	struct nd_route_info *rti_next = NULL;

	nd6_rti_list_wait(__func__);

	TAILQ_FOREACH_SAFE(rti, &nd_rti_list, nd_rti_entry, rti_next) {
		defrouter_select(ifp, &rti->nd_rti_router_list);
	}

	nd6_rti_list_signal_done();
}

static void
nd6_purge_interface_rti_entries(struct ifnet *ifp)
{
	struct nd_route_info *rti = NULL;
	struct nd_route_info *rti_next = NULL;

	nd6_rti_list_wait(__func__);

	TAILQ_FOREACH_SAFE(rti, &nd_rti_list, nd_rti_entry, rti_next) {
		struct nd_route_info rti_tmp = {};
		struct nd_defrouter *dr = NULL;
		struct nd_defrouter *ndr = NULL;

		rti_tmp.nd_rti_prefix = rti->nd_rti_prefix;
		rti_tmp.nd_rti_prefixlen = rti->nd_rti_prefixlen;
		TAILQ_INIT(&rti_tmp.nd_rti_router_list);

		TAILQ_FOREACH_SAFE(dr, &rti->nd_rti_router_list, dr_entry, ndr) {
			  
                                                            
                                    
      
			if (ifp != NULL && dr->ifp != ifp) {
				continue;
			}

			  
                                             
                                  
                                                        
     
                                                       
                                  
                                                    
                                            
                                              
                    
                                                
                                     
     
                                                      
                                                  
      

			TAILQ_REMOVE(&rti->nd_rti_router_list, dr, dr_entry);
			if (dr->stateflags & NDDRF_INSTALLED) {
				TAILQ_INSERT_TAIL(&rti_tmp.nd_rti_router_list, dr, dr_entry);
			} else {
				TAILQ_INSERT_HEAD(&rti_tmp.nd_rti_router_list, dr, dr_entry);
			}
		}

		  
                                                   
                                             
             
    
                                                    
                                                    
                                    
     
		TAILQ_FOREACH_SAFE(dr, &rti_tmp.nd_rti_router_list, dr_entry, ndr) {
			TAILQ_REMOVE(&rti_tmp.nd_rti_router_list, dr, dr_entry);
			defrtrlist_del(dr, &rti->nd_rti_router_list);
			NDDR_REMREF(dr);                                   
		}
		  
                                                                  
                                                                 
     
		if (TAILQ_EMPTY(&rti->nd_rti_router_list)) {
			TAILQ_REMOVE(&nd_rti_list, rti, nd_rti_entry);
			ndrti_free(rti);
		}
	}

	nd6_rti_list_signal_done();
}

static void
nd6_purge_interface_llinfo(struct ifnet *ifp)
{
	struct llinfo_nd6 *ln = NULL;
	                                                   
                                                        
                                     
    
again:
	lck_mtx_lock(rnh_lock);
	ln = llinfo_nd6.ln_next;
	while (ln != NULL && ln != &llinfo_nd6) {
		struct rtentry *rt;
		struct llinfo_nd6 *nln;

		nln = ln->ln_next;
		rt = ln->ln_rt;
		RT_LOCK(rt);
		if (rt->rt_gateway != NULL &&
		    rt->rt_gateway->sa_family == AF_LINK &&
		    SDL(rt->rt_gateway)->sdl_index == ifp->if_index) {
			RT_ADDREF_LOCKED(rt);
			RT_UNLOCK(rt);
			lck_mtx_unlock(rnh_lock);
			  
                                                   
                                                 
                                                   
                                                   
                                                   
                                                   
                                                    
      
			nd6_free(rt);
			RT_REMREF(rt);
			LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
			goto again;
		} else {
			RT_UNLOCK(rt);
		}
		ln = nln;
	}
	lck_mtx_unlock(rnh_lock);
}

  
                                                                           
                 
   
void
nd6_purge(struct ifnet *ifp)
{
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(nd6_mutex);

	                                                 
	nd6_purge_interface_default_routers(ifp);

	                                         
	nd6_purge_interface_prefixes(ifp);

	                                               
	nd6_purge_interface_rti_entries(ifp);

	lck_mtx_unlock(nd6_mutex);

	                                               
	if (nd6_defifindex == ifp->if_index) {
		nd6_setdefaultiface(0);
	}

	  
                                                               
                                 
                                                                
                                     
    
	lck_mtx_lock(nd6_mutex);
	                                 
	defrouter_select(ifp, NULL);
	lck_mtx_unlock(nd6_mutex);

	                                              
	nd6_purge_interface_llinfo(ifp);
}

  
                                                                    
                                                                   
                                                                   
                                                                    
                                                        
   
struct rtentry *
nd6_lookup(struct in6_addr *addr6, int create, struct ifnet *ifp, int rt_locked)
{
	struct rtentry *rt;
	struct sockaddr_in6 sin6;
	unsigned int ifscope;

	bzero(&sin6, sizeof(sin6));
	sin6.sin6_len = sizeof(struct sockaddr_in6);
	sin6.sin6_family = AF_INET6;
	sin6.sin6_addr = *addr6;

	ifscope = (ifp != NULL) ? ifp->if_index : IFSCOPE_NONE;
	if (rt_locked) {
		LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);
		rt = rtalloc1_scoped_locked(SA(&sin6), create, 0, ifscope);
	} else {
		rt = rtalloc1_scoped(SA(&sin6), create, 0, ifscope);
	}

	if (rt != NULL) {
		RT_LOCK(rt);
		if ((rt->rt_flags & RTF_LLINFO) == 0) {
			  
                                             
                                                   
                                               
                                                  
      
			if (create) {
				RT_UNLOCK(rt);
				if (rt_locked) {
					rtfree_locked(rt);
				} else {
					rtfree(rt);
				}
				rt = NULL;
			}
		}
	}
	if (rt == NULL) {
		if (create && ifp) {
			struct ifaddr *ifa;
			u_int32_t ifa_flags;
			int e;

			  
                                                 
                                                  
                                           
                                                       
                                   
      
			ifa = ifaof_ifpforaddr(SA(&sin6), ifp);
			if (ifa == NULL) {
				return NULL;
			}

			  
                                                  
                                              
                                                
                                                 
      
			if (!rt_locked) {
				lck_mtx_lock(rnh_lock);
			}
			IFA_LOCK_SPIN(ifa);
			ifa_flags = ifa->ifa_flags;
			IFA_UNLOCK(ifa);
			if ((e = rtrequest_scoped_locked(RTM_ADD,
			    SA(&sin6), ifa->ifa_addr, SA(&all1_sa),
			    (ifa_flags | RTF_HOST | RTF_LLINFO) &
			    ~RTF_CLONING, &rt, ifscope)) != 0) {
				if (e != EEXIST) {
					log(LOG_ERR, "%s: failed to add route "
					    "for a neighbor(%s), errno=%d\n",
					    __func__, ip6_sprintf(addr6), e);
				}
			}
			if (!rt_locked) {
				lck_mtx_unlock(rnh_lock);
			}
			IFA_REMREF(ifa);
			if (rt == NULL) {
				return NULL;
			}

			RT_LOCK(rt);
			if (rt->rt_llinfo) {
				struct llinfo_nd6 *ln = rt->rt_llinfo;
				struct nd_ifinfo *ndi = ND_IFINFO(rt->rt_ifp);

				VERIFY((NULL != ndi) && (TRUE == ndi->initialized));
				  
                                              
                                                  
                               
       
				if (ndi->flags & ND6_IFF_PERFORMNUD) {
					ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_NOSTATE);
				} else {
					ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_REACHABLE);
					ln_setexpire(ln, 0);
				}
			}
		} else {
			return NULL;
		}
	}
	RT_LOCK_ASSERT_HELD(rt);
	  
                             
                                                                   
                                                                   
                                                               
                                           
                                                                  
                                                              
                                                               
                                                         
              
                                                                 
                                                                     
                                                                     
                               
   
                                                                      
                                                                 
                                                                   
                                 
    
	if ((rt->rt_flags & RTF_GATEWAY) || (rt->rt_flags & RTF_LLINFO) == 0 ||
	    rt->rt_gateway->sa_family != AF_LINK || rt->rt_llinfo == NULL ||
	    (ifp && rt->rt_ifa->ifa_ifp != ifp &&
	    !(rt->rt_flags & RTF_PROXY))) {
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
		if (create) {
			log(LOG_DEBUG, "%s: failed to lookup %s "
			    "(if = %s)\n", __func__, ip6_sprintf(addr6),
			    ifp ? if_name(ifp) : "unspec");
			                           
		}
		return NULL;
	}
	  
                                                             
    
	return rt;
}

  
                                                                   
                                                                     
                                                      
   
static int
nd6_is_new_addr_neighbor(struct sockaddr_in6 *addr, struct ifnet *ifp)
{
	struct nd_prefix *pr;
	struct ifaddr *dstaddr;

	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_OWNED);

	  
                                              
                                                                
    
	if (IN6_IS_ADDR_LINKLOCAL(&addr->sin6_addr)) {
		struct sockaddr_in6 sin6_copy;
		u_int32_t zone;

		  
                                                              
                   
     
		sin6_copy = *addr;
		if (sa6_recoverscope(&sin6_copy, FALSE)) {
			return 0;                                
		}
		if (in6_setscope(&sin6_copy.sin6_addr, ifp, &zone)) {
			return 0;
		}
		if (sin6_copy.sin6_scope_id == zone) {
			return 1;
		} else {
			return 0;
		}
	}

	  
                                                
                            
                                                                      
             
    
	for (pr = nd_prefix.lh_first; pr; pr = pr->ndpr_next) {
		NDPR_LOCK(pr);
		if (pr->ndpr_ifp != ifp) {
			NDPR_UNLOCK(pr);
			continue;
		}
		if (!(pr->ndpr_stateflags & NDPRF_ONLINK)) {
			NDPR_UNLOCK(pr);
			continue;
		}
		if (IN6_ARE_MASKED_ADDR_EQUAL(&pr->ndpr_prefix.sin6_addr,
		    &addr->sin6_addr, &pr->ndpr_mask)) {
			NDPR_UNLOCK(pr);
			return 1;
		}
		NDPR_UNLOCK(pr);
	}

	  
                                                               
                                                      
    
	dstaddr = ifa_ifwithdstaddr(SA(addr));
	if (dstaddr != NULL) {
		if (dstaddr->ifa_ifp == ifp) {
			IFA_REMREF(dstaddr);
			return 1;
		}
		IFA_REMREF(dstaddr);
		dstaddr = NULL;
	}

	return 0;
}


  
                                                                        
                                                          
   
int
nd6_is_addr_neighbor(struct sockaddr_in6 *addr, struct ifnet *ifp,
    int rt_locked)
{
	struct rtentry *rt;

	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(nd6_mutex);
	if (nd6_is_new_addr_neighbor(addr, ifp)) {
		lck_mtx_unlock(nd6_mutex);
		return 1;
	}
	lck_mtx_unlock(nd6_mutex);

	  
                                                                  
                          
    
	if ((rt = nd6_lookup(&addr->sin6_addr, 0, ifp, rt_locked)) != NULL) {
		RT_LOCK_ASSERT_HELD(rt);
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
		return 1;
	}

	return 0;
}

  
                            
                                                                           
                                                                               
                           
   
void
nd6_free(struct rtentry *rt)
{
	struct llinfo_nd6 *ln = NULL;
	struct in6_addr in6 = {};
	struct nd_defrouter *dr = NULL;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	RT_LOCK_ASSERT_NOTHELD(rt);
	lck_mtx_lock(nd6_mutex);

	RT_LOCK(rt);
	RT_ADDREF_LOCKED(rt);                  
	ln = rt->rt_llinfo;
	in6 = SIN6(rt_key(rt))->sin6_addr;

	  
                                                            
                                                            
                         
    
	rt->rt_flags |= RTF_CONDEMNED;

	  
                                                                     
                                                                     
                                                                      
    
	                                                              
	dr = defrouter_lookup(NULL, &SIN6(rt_key(rt))->sin6_addr, rt->rt_ifp);

	if ((ln && ln->ln_router) || dr) {
		  
                                                         
                                   
                                                   
     
		RT_UNLOCK(rt);
		lck_mtx_unlock(nd6_mutex);
		rt6_flush(&in6, rt->rt_ifp);
		lck_mtx_lock(nd6_mutex);
	} else {
		RT_UNLOCK(rt);
	}

	if (dr) {
		NDDR_REMREF(dr);
		  
                                                       
                                                         
              
     

		  
                                                       
                                                   
                        
                                           
                                         
     
		RT_LOCK_SPIN(rt);
		ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_INCOMPLETE);

		  
                                                 
                                                   
                                                    
                   
     
		RT_UNLOCK(rt);
		pfxlist_onlink_check();

		  
                                
     
		defrouter_select(rt->rt_ifp, NULL);

		                                                                  
		nd6_router_select_rti_entries(rt->rt_ifp);
	}
	RT_LOCK_ASSERT_NOTHELD(rt);
	lck_mtx_unlock(nd6_mutex);
	  
                                                                   
                                                                 
                  
    
	(void) rtrequest(RTM_DELETE, rt_key(rt), NULL, rt_mask(rt), 0, NULL);

	                                       
	rtfree(rt);
}

void
nd6_rtrequest(int req, struct rtentry *rt, struct sockaddr *sa)
{
#pragma unused(sa)
	struct sockaddr *gate = rt->rt_gateway;
	struct llinfo_nd6 *ln = rt->rt_llinfo;
	static struct sockaddr_dl null_sdl =
	{ .sdl_len = sizeof(null_sdl), .sdl_family = AF_LINK };
	struct ifnet *ifp = rt->rt_ifp;
	struct ifaddr *ifa;
	uint64_t timenow;
	char buf[MAX_IPv6_STR_LEN];
	struct nd_ifinfo *ndi = ND_IFINFO(rt->rt_ifp);

	VERIFY((NULL != ndi) && (TRUE == ndi->initialized));
	VERIFY(nd6_init_done);
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);
	RT_LOCK_ASSERT_HELD(rt);

	  
                                                                
                                                                 
                                
    
	if (nd6_sched_timeout_want) {
		nd6_sched_timeout(NULL, NULL);
	}

	if (rt->rt_flags & RTF_GATEWAY) {
		return;
	}

	if (!nd6_need_cache(ifp) && !(rt->rt_flags & RTF_HOST)) {
		  
                                                          
                                                              
                                                             
                                                           
                                    
     
		return;
	}

	if (req == RTM_RESOLVE) {
		int no_nd_cache;

		if (!nd6_need_cache(ifp)) {                   
			no_nd_cache = 1;
		} else {
			struct sockaddr_in6 sin6;

			rtkey_to_sa6(rt, &sin6);
			  
                                                   
                                                 
                        
      
			RT_ADDREF_LOCKED(rt);
			RT_UNLOCK(rt);
			no_nd_cache = !nd6_is_addr_neighbor(&sin6, ifp, 1);
			RT_LOCK(rt);
			RT_REMREF_LOCKED(rt);
		}

		  
                                                            
                                                       
                                                         
                                                              
                                                               
                                                               
                                                              
                                                           
                                                            
                                                             
                                                                
                        
     
		if (no_nd_cache) {
			rt->rt_flags &= ~RTF_LLINFO;
			return;
		}
	}

	timenow = net_uptime();

	switch (req) {
	case RTM_ADD:
		  
                                          
    
                                          
                                                          
                                              
     
		if ((rt->rt_flags & RTF_CLONING) ||
		    ((rt->rt_flags & RTF_LLINFO) && ln == NULL)) {
			  
                                                    
                                                         
                                             
                                      
      
			if (rt_setgate(rt, rt_key(rt), SA(&null_sdl)) == 0) {
				gate = rt->rt_gateway;
				SDL(gate)->sdl_type = ifp->if_type;
				SDL(gate)->sdl_index = ifp->if_index;
				  
                                           
                   
       
				if (ln != NULL) {
					ln_setexpire(ln,
					    (ifp->if_eflags & IFEF_IPV6_ND6ALT)
					    ? 0 : MAX(timenow, 1));
				}
			}
			if (rt->rt_flags & RTF_CLONING) {
				break;
			}
		}
		  
                                                                  
                                                         
    
                                                           
                                                       
                                                        
                                                         
                                                                
                                                
                                                       
                                                             
                                       
                                                                
                                                        
                                                          
                                                              
     
		OS_FALLTHROUGH;
	case RTM_RESOLVE:
		if (!(ifp->if_flags & (IFF_POINTOPOINT | IFF_LOOPBACK))) {
			  
                                                       
                                                          
      
			if (gate->sa_family != AF_LINK ||
			    gate->sa_len < sizeof(null_sdl)) {
				                                       
				if (req == RTM_RESOLVE) {
					log(LOG_ERR, "%s: route to %s has bad "
					    "gateway address (sa_family %u "
					    "sa_len %u) on %s\n", __func__,
					    inet_ntop(AF_INET6,
					    &SIN6(rt_key(rt))->sin6_addr, buf,
					    sizeof(buf)), gate->sa_family,
					    gate->sa_len, if_name(ifp));
				}
				break;
			}
			SDL(gate)->sdl_type = ifp->if_type;
			SDL(gate)->sdl_index = ifp->if_index;
		}
		if (ln != NULL) {
			break;                                      
		}
		  
                                                                
                           
     
		rt->rt_llinfo = ln = nd6_llinfo_alloc(Z_WAITOK);

		nd6_allocated++;
		rt->rt_llinfo_get_ri    = nd6_llinfo_get_ri;
		rt->rt_llinfo_get_iflri = nd6_llinfo_get_iflri;
		rt->rt_llinfo_purge     = nd6_llinfo_purge;
		rt->rt_llinfo_free      = nd6_llinfo_free;
		rt->rt_llinfo_refresh   = nd6_llinfo_refresh;
		rt->rt_flags |= RTF_LLINFO;
		ln->ln_rt = rt;
		                                                
		  
                                            
                                                 
                             
     
		if ((req == RTM_ADD) ||
		    !(ndi->flags & ND6_IFF_PERFORMNUD)) {
			  
                                                
                                                 
                                        
      
			ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_REACHABLE);
			ln_setexpire(ln, 0);
		} else {
			  
                                                
                                                    
      
			ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_NOSTATE);
			                                                      
			ln_setexpire(ln, (ifp->if_eflags & IFEF_IPV6_ND6ALT) ?
			    0 : MAX(timenow, 1));
		}
		LN_INSERTHEAD(ln);
		nd6_inuse++;

		                                                              
		nd6_sched_timeout(NULL, NULL);

		  
                                                          
                                                              
                                                             
                                                                
             
     
		if (ip6_neighborgcthresh > 0 &&
		    nd6_inuse >= ip6_neighborgcthresh) {
			int i;

			for (i = 0; i < 10 && llinfo_nd6.ln_prev != ln; i++) {
				struct llinfo_nd6 *ln_end = llinfo_nd6.ln_prev;
				struct rtentry *rt_end = ln_end->ln_rt;

				                                 
				RT_LOCK(rt_end);
				LN_DEQUEUE(ln_end);
				LN_INSERTHEAD(ln_end);

				if (ln_end->ln_expire == 0) {
					RT_UNLOCK(rt_end);
					continue;
				}
				if (ln_end->ln_state > ND6_LLINFO_INCOMPLETE) {
					ND6_CACHE_STATE_TRANSITION(ln_end, ND6_LLINFO_STALE);
				} else {
					ND6_CACHE_STATE_TRANSITION(ln_end, ND6_LLINFO_PURGE);
				}
				ln_setexpire(ln_end, timenow);
				RT_UNLOCK(rt_end);
			}
		}

		  
                                                      
                      
     
		ifa = (struct ifaddr *)in6ifa_ifpwithaddr(rt->rt_ifp,
		    &SIN6(rt_key(rt))->sin6_addr);
		if (ifa != NULL) {
			caddr_t macp = nd6_ifptomac(ifp);
			ln_setexpire(ln, 0);
			ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_REACHABLE);
			if (macp != NULL) {
				Bcopy(macp, LLADDR(SDL(gate)), ifp->if_addrlen);
				SDL(gate)->sdl_alen = ifp->if_addrlen;
			}
			if (nd6_useloopback) {
				if (rt->rt_ifp != lo_ifp) {
					  
                                          
        
					if (rt->rt_llinfo_purge != NULL) {
						rt->rt_llinfo_purge(rt);
					}

					  
                                      
                   
        
					if (rt->rt_if_ref_fn != NULL) {
						rt->rt_if_ref_fn(lo_ifp, 1);
						rt->rt_if_ref_fn(rt->rt_ifp,
						    -1);
					}
				}
				rt->rt_ifp = lo_ifp;
				  
                                          
                                            
       
				if (!(rt->rt_rmx.rmx_locks & RTV_MTU)) {
					rt->rt_rmx.rmx_mtu = rt->rt_ifp->if_mtu;
				}
				  
                                              
                                    
                                         
                                                
                                                    
                               
       
				if (ifa != rt->rt_ifa) {
					rtsetifa(rt, ifa);
				}
			}
			IFA_REMREF(ifa);
		} else if (rt->rt_flags & RTF_ANNOUNCE) {
			ln_setexpire(ln, 0);
			ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_REACHABLE);

			                                                
			if (ifp->if_flags & IFF_MULTICAST) {
				struct in6_addr llsol;
				struct in6_multi *in6m;
				int error;

				llsol = SIN6(rt_key(rt))->sin6_addr;
				llsol.s6_addr32[0] = IPV6_ADDR_INT32_MLL;
				llsol.s6_addr32[1] = 0;
				llsol.s6_addr32[2] = htonl(1);
				llsol.s6_addr8[12] = 0xff;
				if (in6_setscope(&llsol, ifp, NULL)) {
					break;
				}
				error = in6_mc_join(ifp, &llsol,
				    NULL, &in6m, 0);
				if (error) {
					nd6log(error, "%s: failed to join "
					    "%s (errno=%d)\n", if_name(ifp),
					    ip6_sprintf(&llsol), error);
				} else {
					IN6M_REMREF(in6m);
				}
			}
		}
		break;

	case RTM_DELETE:
		if (ln == NULL) {
			break;
		}
		                                                      
		if ((rt->rt_flags & RTF_ANNOUNCE) &&
		    (ifp->if_flags & IFF_MULTICAST)) {
			struct in6_addr llsol;
			struct in6_multi *in6m;

			llsol = SIN6(rt_key(rt))->sin6_addr;
			llsol.s6_addr32[0] = IPV6_ADDR_INT32_MLL;
			llsol.s6_addr32[1] = 0;
			llsol.s6_addr32[2] = htonl(1);
			llsol.s6_addr8[12] = 0xff;
			if (in6_setscope(&llsol, ifp, NULL) == 0) {
				in6_multihead_lock_shared();
				IN6_LOOKUP_MULTI(&llsol, ifp, in6m);
				in6_multihead_lock_done();
				if (in6m != NULL) {
					in6_mc_leave(in6m, NULL);
					IN6M_REMREF(in6m);
				}
			}
		}
		nd6_inuse--;
		  
                                                            
                                                          
                                                            
                                                        
     
		if (ln->ln_flags & ND6_LNF_IN_USE) {
			LN_DEQUEUE(ln);
		}

		  
                                       
     
		if (rt->rt_llinfo_purge != NULL) {
			rt->rt_llinfo_purge(rt);
		}

		rt->rt_flags &= ~RTF_LLINFO;
		if (ln->ln_hold != NULL) {
			m_freem_list(ln->ln_hold);
			ln->ln_hold = NULL;
		}
	}
}

static int
nd6_siocgdrlst(void *data, int data_is_64)
{
	struct in6_drlist_32 *drl_32;
	struct nd_defrouter *dr;
	int i = 0;

	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_OWNED);

	dr = TAILQ_FIRST(&nd_defrouter_list);

	                                         
	                        
	if (data_is_64) {
		struct in6_drlist_64 *drl_64;

		drl_64 = _MALLOC(sizeof(*drl_64), M_TEMP, M_WAITOK | M_ZERO);
		if (drl_64 == NULL) {
			return ENOMEM;
		}

		                                 
		bcopy(data, drl_64, sizeof(drl_64->ifname));

		while (dr && i < DRLSTSIZ) {
			drl_64->defrouter[i].rtaddr = dr->rtaddr;
			if (IN6_IS_ADDR_LINKLOCAL(
				    &drl_64->defrouter[i].rtaddr)) {
				                                           
				drl_64->defrouter[i].rtaddr.s6_addr16[1] = 0;
			} else {
				log(LOG_ERR,
				    "default router list contains a "
				    "non-linklocal address(%s)\n",
				    ip6_sprintf(&drl_64->defrouter[i].rtaddr));
			}
			drl_64->defrouter[i].flags = dr->flags;
			drl_64->defrouter[i].rtlifetime = (u_short)dr->rtlifetime;
			drl_64->defrouter[i].expire = (u_long)nddr_getexpire(dr);
			drl_64->defrouter[i].if_index = dr->ifp->if_index;
			i++;
			dr = TAILQ_NEXT(dr, dr_entry);
		}
		bcopy(drl_64, data, sizeof(*drl_64));
		_FREE(drl_64, M_TEMP);
		return 0;
	}

	                        
	drl_32 = _MALLOC(sizeof(*drl_32), M_TEMP, M_WAITOK | M_ZERO);
	if (drl_32 == NULL) {
		return ENOMEM;
	}

	                                 
	bcopy(data, drl_32, sizeof(drl_32->ifname));

	while (dr != NULL && i < DRLSTSIZ) {
		drl_32->defrouter[i].rtaddr = dr->rtaddr;
		if (IN6_IS_ADDR_LINKLOCAL(&drl_32->defrouter[i].rtaddr)) {
			                                           
			drl_32->defrouter[i].rtaddr.s6_addr16[1] = 0;
		} else {
			log(LOG_ERR,
			    "default router list contains a "
			    "non-linklocal address(%s)\n",
			    ip6_sprintf(&drl_32->defrouter[i].rtaddr));
		}
		drl_32->defrouter[i].flags = dr->flags;
		drl_32->defrouter[i].rtlifetime = (u_short)dr->rtlifetime;
		drl_32->defrouter[i].expire = (u_int32_t)nddr_getexpire(dr);
		drl_32->defrouter[i].if_index = dr->ifp->if_index;
		i++;
		dr = TAILQ_NEXT(dr, dr_entry);
	}
	bcopy(drl_32, data, sizeof(*drl_32));
	_FREE(drl_32, M_TEMP);
	return 0;
}

  
                                                      
                                                              
                                        
   
static int
nd6_siocgprlst(void *data, int data_is_64)
{
	struct in6_prlist_32 *prl_32;
	struct nd_prefix *pr;
	int i = 0;

	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_OWNED);

	pr = nd_prefix.lh_first;

	                                         
	                        
	if (data_is_64) {
		struct in6_prlist_64 *prl_64;

		prl_64 = _MALLOC(sizeof(*prl_64), M_TEMP, M_WAITOK | M_ZERO);
		if (prl_64 == NULL) {
			return ENOMEM;
		}

		                                 
		bcopy(data, prl_64, sizeof(prl_64->ifname));

		while (pr && i < PRLSTSIZ) {
			struct nd_pfxrouter *pfr;
			int j;

			NDPR_LOCK(pr);
			(void) in6_embedscope(&prl_64->prefix[i].prefix,
			    &pr->ndpr_prefix, NULL, NULL, NULL);
			prl_64->prefix[i].raflags = pr->ndpr_raf;
			prl_64->prefix[i].prefixlen = pr->ndpr_plen;
			prl_64->prefix[i].vltime = pr->ndpr_vltime;
			prl_64->prefix[i].pltime = pr->ndpr_pltime;
			prl_64->prefix[i].if_index = pr->ndpr_ifp->if_index;
			prl_64->prefix[i].expire = (u_long)ndpr_getexpire(pr);

			pfr = pr->ndpr_advrtrs.lh_first;
			j = 0;
			while (pfr) {
				if (j < DRLSTSIZ) {
#define RTRADDR prl_64->prefix[i].advrtr[j]
					RTRADDR = pfr->router->rtaddr;
					if (IN6_IS_ADDR_LINKLOCAL(&RTRADDR)) {
						                        
						RTRADDR.s6_addr16[1] = 0;
					} else {
						log(LOG_ERR,
						    "a router(%s) advertises "
						    "a prefix with "
						    "non-link local address\n",
						    ip6_sprintf(&RTRADDR));
					}
#undef RTRADDR
				}
				j++;
				pfr = pfr->pfr_next;
			}
			ASSERT(j <= USHRT_MAX);
			prl_64->prefix[i].advrtrs = (u_short)j;
			prl_64->prefix[i].origin = PR_ORIG_RA;
			NDPR_UNLOCK(pr);

			i++;
			pr = pr->ndpr_next;
		}
		bcopy(prl_64, data, sizeof(*prl_64));
		_FREE(prl_64, M_TEMP);
		return 0;
	}

	                        
	prl_32 = _MALLOC(sizeof(*prl_32), M_TEMP, M_WAITOK | M_ZERO);
	if (prl_32 == NULL) {
		return ENOMEM;
	}

	                                 
	bcopy(data, prl_32, sizeof(prl_32->ifname));

	while (pr && i < PRLSTSIZ) {
		struct nd_pfxrouter *pfr;
		int j;

		NDPR_LOCK(pr);
		(void) in6_embedscope(&prl_32->prefix[i].prefix,
		    &pr->ndpr_prefix, NULL, NULL, NULL);
		prl_32->prefix[i].raflags = pr->ndpr_raf;
		prl_32->prefix[i].prefixlen = pr->ndpr_plen;
		prl_32->prefix[i].vltime = pr->ndpr_vltime;
		prl_32->prefix[i].pltime = pr->ndpr_pltime;
		prl_32->prefix[i].if_index = pr->ndpr_ifp->if_index;
		prl_32->prefix[i].expire = (u_int32_t)ndpr_getexpire(pr);

		pfr = pr->ndpr_advrtrs.lh_first;
		j = 0;
		while (pfr) {
			if (j < DRLSTSIZ) {
#define RTRADDR prl_32->prefix[i].advrtr[j]
				RTRADDR = pfr->router->rtaddr;
				if (IN6_IS_ADDR_LINKLOCAL(&RTRADDR)) {
					                        
					RTRADDR.s6_addr16[1] = 0;
				} else {
					log(LOG_ERR,
					    "a router(%s) advertises "
					    "a prefix with "
					    "non-link local address\n",
					    ip6_sprintf(&RTRADDR));
				}
#undef RTRADDR
			}
			j++;
			pfr = pfr->pfr_next;
		}
		ASSERT(j <= USHRT_MAX);
		prl_32->prefix[i].advrtrs = (u_short)j;
		prl_32->prefix[i].origin = PR_ORIG_RA;
		NDPR_UNLOCK(pr);

		i++;
		pr = pr->ndpr_next;
	}
	bcopy(prl_32, data, sizeof(*prl_32));
	_FREE(prl_32, M_TEMP);
	return 0;
}

int
nd6_ioctl(u_long cmd, caddr_t data, struct ifnet *ifp)
{
	struct nd_defrouter *dr;
	struct nd_prefix *pr;
	struct rtentry *rt;
	int error = 0;

	VERIFY(ifp != NULL);

	switch (cmd) {
	case SIOCGDRLST_IN6_32:                                   
	case SIOCGDRLST_IN6_64:                                   
		  
                                                   
     
		lck_mtx_lock(nd6_mutex);
		error = nd6_siocgdrlst(data, cmd == SIOCGDRLST_IN6_64);
		lck_mtx_unlock(nd6_mutex);
		break;

	case SIOCGPRLST_IN6_32:                                   
	case SIOCGPRLST_IN6_64:                                   
		  
                                                   
     
		lck_mtx_lock(nd6_mutex);
		error = nd6_siocgprlst(data, cmd == SIOCGPRLST_IN6_64);
		lck_mtx_unlock(nd6_mutex);
		break;

	case OSIOCGIFINFO_IN6:                                  
	case SIOCGIFINFO_IN6: {                                 
		u_int32_t linkmtu;
		struct in6_ondireq *ondi = (struct in6_ondireq *)(void *)data;
		struct nd_ifinfo *ndi;
		  
                                                      
                                                   
     
		ndi = ND_IFINFO(ifp);
		if ((NULL == ndi) || (FALSE == ndi->initialized)) {
			error = EINVAL;
			break;
		}
		lck_mtx_lock(&ndi->lock);
		linkmtu = IN6_LINKMTU(ifp);
		bcopy(&linkmtu, &ondi->ndi.linkmtu, sizeof(linkmtu));
		bcopy(&ndi->maxmtu, &ondi->ndi.maxmtu,
		    sizeof(u_int32_t));
		bcopy(&ndi->basereachable, &ondi->ndi.basereachable,
		    sizeof(u_int32_t));
		bcopy(&ndi->reachable, &ondi->ndi.reachable,
		    sizeof(u_int32_t));
		bcopy(&ndi->retrans, &ondi->ndi.retrans,
		    sizeof(u_int32_t));
		bcopy(&ndi->flags, &ondi->ndi.flags,
		    sizeof(u_int32_t));
		bcopy(&ndi->recalctm, &ondi->ndi.recalctm,
		    sizeof(int));
		ondi->ndi.chlim = ndi->chlim;
		  
                                                         
                       
     
		ondi->ndi.receivedra = (uint8_t)ndi->ndefrouters;
		ondi->ndi.collision_count = (uint8_t)ndi->cga_collision_count;
		lck_mtx_unlock(&ndi->lock);
		break;
	}

	case SIOCSIFINFO_FLAGS: {                              
		  
                                                 
                                                 
                                       
                           
     
		struct in6_ndireq *cndi = (struct in6_ndireq *)(void *)data;
		u_int32_t oflags, flags;
		struct nd_ifinfo *ndi = ND_IFINFO(ifp);

		                                                         
		if ((NULL == ndi) || !ndi->initialized) {
			error = EINVAL;
			break;
		}

		lck_mtx_lock(&ndi->lock);
		oflags = ndi->flags;
		bcopy(&cndi->ndi.flags, &(ndi->flags), sizeof(flags));
		flags = ndi->flags;
		lck_mtx_unlock(&ndi->lock);

		if (oflags == flags) {
			break;
		}

		error = nd6_setifinfo(ifp, oflags, flags);
		break;
	}

	case SIOCSNDFLUSH_IN6:                                
		                               
		  
                                                     
                                                   
    
                                               
                                                                 
     
		lck_mtx_lock(nd6_mutex);
		defrouter_reset();
		defrouter_select(ifp, NULL);
		lck_mtx_unlock(nd6_mutex);
		                                     
		break;

	case SIOCSPFXFLUSH_IN6: {                             
		                                                
		struct nd_prefix *next = NULL;

		lck_mtx_lock(nd6_mutex);
		for (pr = nd_prefix.lh_first; pr; pr = next) {
			struct in6_ifaddr *ia = NULL;
			bool iterate_pfxlist_again = false;

			next = pr->ndpr_next;

			NDPR_LOCK(pr);
			if (IN6_IS_ADDR_LINKLOCAL(&pr->ndpr_prefix.sin6_addr)) {
				NDPR_UNLOCK(pr);
				continue;          
			}
			if (ifp != lo_ifp && pr->ndpr_ifp != ifp) {
				NDPR_UNLOCK(pr);
				continue;
			}
			                                                    
			NDPR_ADDREF(pr);
			NDPR_UNLOCK(pr);
			lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
			bool from_begining = true;
			while (from_begining) {
				from_begining = false;
				TAILQ_FOREACH(ia, &in6_ifaddrhead, ia6_link) {
					IFA_LOCK(&ia->ia_ifa);
					if ((ia->ia6_flags & IN6_IFF_AUTOCONF) == 0) {
						IFA_UNLOCK(&ia->ia_ifa);
						continue;
					}

					if (ia->ia6_ndpr == pr) {
						IFA_ADDREF_LOCKED(&ia->ia_ifa);
						IFA_UNLOCK(&ia->ia_ifa);
						lck_rw_done(&in6_ifaddr_rwlock);
						lck_mtx_unlock(nd6_mutex);
						in6_purgeaddr(&ia->ia_ifa);
						IFA_REMREF(&ia->ia_ifa);
						lck_mtx_lock(nd6_mutex);
						lck_rw_lock_exclusive(
							&in6_ifaddr_rwlock);
						  
                                   
                                
                    
                                           
                                           
                                              
         
						iterate_pfxlist_again = true;
						from_begining = true;
						break;
					}
					IFA_UNLOCK(&ia->ia_ifa);
				}
			}
			lck_rw_done(&in6_ifaddr_rwlock);
			NDPR_LOCK(pr);
			prelist_remove(pr);
			NDPR_UNLOCK(pr);
			pfxlist_onlink_check();
			NDPR_REMREF(pr);
			if (iterate_pfxlist_again) {
				next = nd_prefix.lh_first;
			}
		}
		lck_mtx_unlock(nd6_mutex);
		break;
	}

	case SIOCSRTRFLUSH_IN6: {                             
		                                   
		struct nd_defrouter *next;
		struct nd_drhead nd_defrouter_tmp;

		TAILQ_INIT(&nd_defrouter_tmp);
		lck_mtx_lock(nd6_mutex);
		if ((dr = TAILQ_FIRST(&nd_defrouter_list)) != NULL) {
			  
                                                  
                                                  
      
			for (dr = TAILQ_NEXT(dr, dr_entry); dr; dr = next) {
				next = TAILQ_NEXT(dr, dr_entry);
				if (ifp == lo_ifp || dr->ifp == ifp) {
					  
                                                 
                                    
                                                  
                                            
                                                     
                    
                                                        
        
					TAILQ_REMOVE(&nd_defrouter_list, dr, dr_entry);
					TAILQ_INSERT_TAIL(&nd_defrouter_tmp, dr, dr_entry);
				}
			}

			dr = TAILQ_FIRST(&nd_defrouter_list);
			if (ifp == lo_ifp ||
			    dr->ifp == ifp) {
				TAILQ_REMOVE(&nd_defrouter_list, dr, dr_entry);
				TAILQ_INSERT_TAIL(&nd_defrouter_tmp, dr, dr_entry);
			}
		}

		  
                                                            
                                               
                                                  
                                                 
                                        
    
                                                   
                                             
                     
     
		TAILQ_FOREACH_SAFE(dr, &nd_defrouter_tmp, dr_entry, next) {
			TAILQ_REMOVE(&nd_defrouter_tmp, dr, dr_entry);
			defrtrlist_del(dr, NULL);
			NDDR_REMREF(dr);                                   
		}

		                                                                    
		nd6_purge_interface_rti_entries((ifp == lo_ifp) ? NULL : ifp);

		lck_mtx_unlock(nd6_mutex);
		break;
	}

	case SIOCGNBRINFO_IN6_32: {                                
		struct llinfo_nd6 *ln;
		struct in6_nbrinfo_32 nbi_32;
		struct in6_addr nb_addr;                            

		bcopy(data, &nbi_32, sizeof(nbi_32));
		nb_addr = nbi_32.addr;
		  
                                                 
                                                 
     
		if (IN6_IS_ADDR_LINKLOCAL(&nbi_32.addr) ||
		    IN6_IS_ADDR_MC_LINKLOCAL(&nbi_32.addr)) {
			u_int16_t *idp =
			    (u_int16_t *)(void *)&nb_addr.s6_addr[2];

			if (*idp == 0) {
				*idp = htons(ifp->if_index);
			}
		}

		                                                
		if ((rt = nd6_lookup(&nb_addr, 0, ifp, 0)) == NULL) {
			error = EINVAL;
			break;
		}
		RT_LOCK_ASSERT_HELD(rt);
		ln = rt->rt_llinfo;
		nbi_32.state = ln->ln_state;
		nbi_32.asked = ln->ln_asked;
		nbi_32.isrouter = ln->ln_router;
		nbi_32.expire = (int)ln_getexpire(ln);
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
		bcopy(&nbi_32, data, sizeof(nbi_32));
		break;
	}

	case SIOCGNBRINFO_IN6_64: {                                
		struct llinfo_nd6 *ln;
		struct in6_nbrinfo_64 nbi_64;
		struct in6_addr nb_addr;                            

		bcopy(data, &nbi_64, sizeof(nbi_64));
		nb_addr = nbi_64.addr;
		  
                                                 
                                                 
     
		if (IN6_IS_ADDR_LINKLOCAL(&nbi_64.addr) ||
		    IN6_IS_ADDR_MC_LINKLOCAL(&nbi_64.addr)) {
			u_int16_t *idp =
			    (u_int16_t *)(void *)&nb_addr.s6_addr[2];

			if (*idp == 0) {
				*idp = htons(ifp->if_index);
			}
		}

		                                                
		if ((rt = nd6_lookup(&nb_addr, 0, ifp, 0)) == NULL) {
			error = EINVAL;
			break;
		}
		RT_LOCK_ASSERT_HELD(rt);
		ln = rt->rt_llinfo;
		nbi_64.state = ln->ln_state;
		nbi_64.asked = ln->ln_asked;
		nbi_64.isrouter = ln->ln_router;
		nbi_64.expire = (int)ln_getexpire(ln);
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
		bcopy(&nbi_64, data, sizeof(nbi_64));
		break;
	}

	case SIOCGDEFIFACE_IN6_32:                                 
	case SIOCGDEFIFACE_IN6_64: {                               
		struct in6_ndifreq_64 *ndif_64 =
		    (struct in6_ndifreq_64 *)(void *)data;
		struct in6_ndifreq_32 *ndif_32 =
		    (struct in6_ndifreq_32 *)(void *)data;

		if (cmd == SIOCGDEFIFACE_IN6_64) {
			u_int64_t j = nd6_defifindex;
			__nochk_bcopy(&j, &ndif_64->ifindex, sizeof(j));
		} else {
			bcopy(&nd6_defifindex, &ndif_32->ifindex,
			    sizeof(u_int32_t));
		}
		break;
	}

	case SIOCSDEFIFACE_IN6_32:                                 
	case SIOCSDEFIFACE_IN6_64: {                               
		struct in6_ndifreq_64 *ndif_64 =
		    (struct in6_ndifreq_64 *)(void *)data;
		struct in6_ndifreq_32 *ndif_32 =
		    (struct in6_ndifreq_32 *)(void *)data;
		u_int32_t idx;

		if (cmd == SIOCSDEFIFACE_IN6_64) {
			u_int64_t j;
			__nochk_bcopy(&ndif_64->ifindex, &j, sizeof(j));
			idx = (u_int32_t)j;
		} else {
			bcopy(&ndif_32->ifindex, &idx, sizeof(idx));
		}

		error = nd6_setdefaultiface(idx);
		return error;
		                
	}
	case SIOCGIFCGAPREP_IN6_32:
	case SIOCGIFCGAPREP_IN6_64: {
		                        
		union {
			struct in6_cgareq_32    *cga32;
			struct in6_cgareq_64    *cga64;
			void                    *data;
		} cgareq_u;
		struct nd_ifinfo        *ndi;
		struct in6_cga_modifier *ndi_cga_mod;
		struct in6_cga_modifier *req_cga_mod;

		ndi = ND_IFINFO(ifp);
		if ((NULL == ndi) || !ndi->initialized) {
			error = EINVAL;
			break;
		}
		cgareq_u.data = data;
		req_cga_mod = (cmd == SIOCGIFCGAPREP_IN6_64)
		    ? &(cgareq_u.cga64->cgar_cgaprep.cga_modifier)
		    : &(cgareq_u.cga32->cgar_cgaprep.cga_modifier);
		lck_mtx_lock(&ndi->lock);
		ndi_cga_mod = &(ndi->local_cga_modifier);
		bcopy(ndi_cga_mod, req_cga_mod, sizeof(*req_cga_mod));
		lck_mtx_unlock(&ndi->lock);
		break;
	}
	case SIOCSIFCGAPREP_IN6_32:
	case SIOCSIFCGAPREP_IN6_64:
	{
		                        
		struct in6_cgareq       cgareq;
		int                     is64;
		struct nd_ifinfo        *ndi;
		struct in6_cga_modifier *ndi_cga_mod;
		struct in6_cga_modifier *req_cga_mod;

		ndi = ND_IFINFO(ifp);
		if ((NULL == ndi) || !ndi->initialized) {
			error = EINVAL;
			break;
		}
		is64 = (cmd == SIOCSIFCGAPREP_IN6_64);
		in6_cgareq_copy_from_user(data, is64, &cgareq);
		req_cga_mod = &cgareq.cgar_cgaprep.cga_modifier;
		lck_mtx_lock(&ndi->lock);
		ndi_cga_mod = &(ndi->local_cga_modifier);
		bcopy(req_cga_mod, ndi_cga_mod, sizeof(*ndi_cga_mod));
		ndi->cga_initialized = TRUE;
		ndi->cga_collision_count = 0;
		lck_mtx_unlock(&ndi->lock);
		break;
	}
	default:
		break;
	}
	return error;
}

  
                                                            
                                                           
   
void
nd6_cache_lladdr(struct ifnet *ifp, struct in6_addr *from, char *lladdr,
    int lladdrlen, int type, int code)
{
#pragma unused(lladdrlen)
	struct rtentry *rt = NULL;
	struct llinfo_nd6 *ln = NULL;
	int is_newentry;
	struct sockaddr_dl *sdl = NULL;
	int do_update;
	int olladdr;
	int llchange;
	short newstate = 0;
	uint64_t timenow;
	boolean_t sched_timeout = FALSE;
	struct nd_ifinfo *ndi = NULL;

	if (ifp == NULL) {
		panic("ifp == NULL in nd6_cache_lladdr");
	}
	if (from == NULL) {
		panic("from == NULL in nd6_cache_lladdr");
	}

	                                                     
	if (IN6_IS_ADDR_UNSPECIFIED(from)) {
		return;
	}

	  
                                                                  
               
    
	timenow = net_uptime();

	rt = nd6_lookup(from, 0, ifp, 0);
	if (rt == NULL) {
		if ((rt = nd6_lookup(from, 1, ifp, 0)) == NULL) {
			return;
		}
		RT_LOCK_ASSERT_HELD(rt);
		is_newentry = 1;
	} else {
		RT_LOCK_ASSERT_HELD(rt);
		                                     
		if (rt->rt_flags & RTF_STATIC) {
			RT_REMREF_LOCKED(rt);
			RT_UNLOCK(rt);
			return;
		}
		is_newentry = 0;
	}

	if ((rt->rt_flags & (RTF_GATEWAY | RTF_LLINFO)) != RTF_LLINFO) {
fail:
		RT_UNLOCK(rt);
		nd6_free(rt);
		rtfree(rt);
		return;
	}
	ln = (struct llinfo_nd6 *)rt->rt_llinfo;
	if (ln == NULL) {
		goto fail;
	}
	if (rt->rt_gateway == NULL) {
		goto fail;
	}
	if (rt->rt_gateway->sa_family != AF_LINK) {
		goto fail;
	}
	sdl = SDL(rt->rt_gateway);

	olladdr = (sdl->sdl_alen) ? 1 : 0;
	if (olladdr && lladdr) {
		if (bcmp(lladdr, LLADDR(sdl), ifp->if_addrlen)) {
			llchange = 1;
		} else {
			llchange = 0;
		}
	} else {
		llchange = 0;
	}

	  
                                                 
                
                
                        
                 
                       
                                      
                         
    

	if (lladdr != NULL) {                              
		  
                                     
                                         
     
		sdl->sdl_alen = ifp->if_addrlen;
		bcopy(lladdr, LLADDR(sdl), ifp->if_addrlen);

		                                           
		nd6_llreach_alloc(rt, ifp, LLADDR(sdl), sdl->sdl_alen, FALSE);
	}

	if (is_newentry == 0) {
		if ((!olladdr && lladdr != NULL) ||              
		    (olladdr && lladdr != NULL && llchange)) {           
			do_update = 1;
			newstate = ND6_LLINFO_STALE;
		} else {                                             
			do_update = 0;
		}
	} else {
		do_update = 1;
		if (lladdr == NULL) {                            
			newstate = ND6_LLINFO_NOSTATE;
		} else {                                         
			newstate = ND6_LLINFO_STALE;
		}
	}

	  
                                           
                                               
                            
    
	ndi = ND_IFINFO(ifp);
	VERIFY((NULL != ndi) && (TRUE == ndi->initialized));

	if (ndi && !(ndi->flags & ND6_IFF_PERFORMNUD)) {
		newstate = ND6_LLINFO_REACHABLE;
		ln_setexpire(ln, 0);
	}

	if (do_update) {
		  
                                            
     
		ND6_CACHE_STATE_TRANSITION(ln, newstate);

		if ((ln->ln_state == ND6_LLINFO_STALE) ||
		    (ln->ln_state == ND6_LLINFO_REACHABLE)) {
			struct mbuf *m = ln->ln_hold;
			  
                                              
                                                   
                                                        
                  
      
			if (ln->ln_state == ND6_LLINFO_STALE) {
				ln_setexpire(ln, timenow + nd6_gctimer);
			}

			ln->ln_hold = NULL;
			if (m != NULL) {
				struct sockaddr_in6 sin6;

				rtkey_to_sa6(rt, &sin6);
				  
                                               
                                           
       
				RT_UNLOCK(rt);
				nd6_output_list(ifp, ifp, m, &sin6, rt, NULL);
				RT_LOCK(rt);
			}
		} else if (ln->ln_state == ND6_LLINFO_INCOMPLETE) {
			                      
			ln_setexpire(ln, timenow);
			sched_timeout = TRUE;
		}
	}

	  
                                  
   
                                   
                      
                                      
                                      
   
                 
                                                                   
                                                       
                                                                    
                                                                       
                                                                 
                                           
                                                                   
   
                                                          
              
                      
                      
                      
                     
                     
                          
                           
   
                       
    
	switch (type & 0xff) {
	case ND_NEIGHBOR_SOLICIT:
		  
                                                
     
		if (is_newentry) {                 
			ln->ln_router = 0;
		}
		break;
	case ND_REDIRECT:
		  
                                                                 
                                                              
                                                       
     
		if (code == ND_REDIRECT_ROUTER) {
			ln->ln_router = 1;
		} else if (is_newentry) {            
			ln->ln_router = 0;
		}
		break;
	case ND_ROUTER_SOLICIT:
		  
                                           
     
		ln->ln_router = 0;
		break;
	case ND_ROUTER_ADVERT:
		  
                                           
     
		if ((!is_newentry && (olladdr || lladdr)) ||               
		    (is_newentry && lladdr)) {                           
			ln->ln_router = 1;
		}
		break;
	}

	if (do_update) {
		int route_ev_code = 0;

		if (llchange) {
			route_ev_code = ROUTE_LLENTRY_CHANGED;
		} else {
			route_ev_code = ROUTE_LLENTRY_RESOLVED;
		}

		                                                               
		route_event_enqueue_nwk_wq_entry(rt, NULL, route_ev_code, NULL, TRUE);

		if (ln->ln_router || (rt->rt_flags & RTF_ROUTER)) {
			struct radix_node_head  *rnh = NULL;
			struct route_event rt_ev;
			route_event_init(&rt_ev, rt, NULL, llchange ? ROUTE_LLENTRY_CHANGED :
			    ROUTE_LLENTRY_RESOLVED);
			  
                                              
                                               
                                                     
      
			RT_UNLOCK(rt);
			lck_mtx_lock(rnh_lock);

			rnh = rt_tables[AF_INET6];
			if (rnh != NULL) {
				(void) rnh->rnh_walktree(rnh, route_event_walktree,
				    (void *)&rt_ev);
			}
			lck_mtx_unlock(rnh_lock);
			RT_LOCK(rt);
		}
	}

	  
                                                               
                                                                       
                                                  
                                                                      
         
   
                                                                     
                                 
    
	if (do_update && ln->ln_router) {
		  
                                                            
                                                         
     
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
		lck_mtx_lock(nd6_mutex);
		defrouter_select(ifp, NULL);
		nd6_router_select_rti_entries(ifp);
		lck_mtx_unlock(nd6_mutex);
	} else {
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
	}
	if (sched_timeout) {
		lck_mtx_lock(rnh_lock);
		nd6_sched_timeout(NULL, NULL);
		lck_mtx_unlock(rnh_lock);
	}
}

static void
nd6_slowtimo(void *arg)
{
#pragma unused(arg)
	struct nd_ifinfo *nd6if = NULL;
	struct ifnet *ifp = NULL;

	ifnet_head_lock_shared();
	for (ifp = ifnet_head.tqh_first; ifp;
	    ifp = ifp->if_link.tqe_next) {
		nd6if = ND_IFINFO(ifp);
		if ((NULL == nd6if) || (FALSE == nd6if->initialized)) {
			continue;
		}

		lck_mtx_lock(&nd6if->lock);
		if (nd6if->basereachable &&                          
		    (nd6if->recalctm -= ND6_SLOWTIMER_INTERVAL) <= 0) {
			  
                                                   
                                                        
                                                          
                       
      
			nd6if->recalctm = nd6_recalc_reachtm_interval;
			nd6if->reachable =
			    ND_COMPUTE_RTIME(nd6if->basereachable);
		}
		lck_mtx_unlock(&nd6if->lock);
	}
	ifnet_head_done();
	timeout(nd6_slowtimo, NULL, ND6_SLOWTIMER_INTERVAL * hz);
}

int
nd6_output(struct ifnet *ifp, struct ifnet *origifp, struct mbuf *m0,
    struct sockaddr_in6 *dst, struct rtentry *hint0, struct flowadv *adv)
{
	return nd6_output_list(ifp, origifp, m0, dst, hint0, adv);
}

  
                    
  
                                                                               
                            
   
#define senderr(e) { error = (e); goto bad; }
int
nd6_output_list(struct ifnet *ifp, struct ifnet *origifp, struct mbuf *m0,
    struct sockaddr_in6 *dst, struct rtentry *hint0, struct flowadv *adv)
{
	struct rtentry *rt = hint0, *hint = hint0;
	struct llinfo_nd6 *ln = NULL;
	int error = 0;
	uint64_t timenow;
	struct rtentry *rtrele = NULL;
	struct nd_ifinfo *ndi = NULL;

	if (rt != NULL) {
		RT_LOCK_SPIN(rt);
		RT_ADDREF_LOCKED(rt);
	}

	if (IN6_IS_ADDR_MULTICAST(&dst->sin6_addr) || !nd6_need_cache(ifp)) {
		if (rt != NULL) {
			RT_UNLOCK(rt);
		}
		goto sendpkt;
	}

	  
                                                                     
                                                                     
                                                                      
                                                                     
                                                                     
                        
   
                                                                        
                                                                     
                                                     
   
                                                                    
                               
    
	if (rt != NULL) {
		  
                                                                
                                                              
             
     
		RT_LOCK_ASSERT_HELD(rt);
		if (!(rt->rt_flags & RTF_UP)) {
			RT_REMREF_LOCKED(rt);
			RT_UNLOCK(rt);
			if ((hint = rt = rtalloc1_scoped(SA(dst), 1, 0,
			    ifp->if_index)) != NULL) {
				RT_LOCK_SPIN(rt);
				if (rt->rt_ifp != ifp) {
					                     
					RT_UNLOCK(rt);
					error = nd6_output_list(ifp, origifp, m0,
					    dst, rt, adv);
					rtfree(rt);
					return error;
				}
			} else {
				senderr(EHOSTUNREACH);
			}
		}

		if (rt->rt_flags & RTF_GATEWAY) {
			struct rtentry *gwrt;
			struct in6_ifaddr *ia6 = NULL;
			struct sockaddr_in6 gw6;

			rtgw_to_sa6(rt, &gw6);
			  
                                                    
                                               
      
			RT_UNLOCK(rt);

			  
                                                   
                                                    
                                                          
                                                   
                                                 
                                                      
      
			if (!nd6_is_addr_neighbor(&gw6, ifp, 0) ||
			    (ia6 = in6ifa_ifpwithaddr(ifp, &gw6.sin6_addr))) {
				  
                                              
                                          
                                                 
       
				if (ia6 != NULL) {
					IFA_REMREF(&ia6->ia_ifa);
				}
				if ((ifp->if_flags & IFF_POINTOPOINT) == 0) {
					senderr(EHOSTUNREACH);
				}
				goto sendpkt;
			}

			RT_LOCK_SPIN(rt);
			gw6 = *(SIN6(rt->rt_gateway));

			                                  
			if (!(rt->rt_flags & RTF_UP)) {
				RT_UNLOCK(rt);
				senderr(EHOSTUNREACH);
			}

			                                             
			if ((gwrt = rt->rt_gwroute) == NULL) {
				RT_UNLOCK(rt);
				goto lookup;
			}
			                            
			RT_CONVERT_LOCK(rt);

			  
                                                  
                                               
                                        
      
			RT_LOCK_SPIN(gwrt);
			if (!(gwrt->rt_flags & RTF_UP)) {
				rt->rt_gwroute = NULL;
				RT_UNLOCK(gwrt);
				RT_UNLOCK(rt);
				rtfree(gwrt);
lookup:
				lck_mtx_lock(rnh_lock);
				gwrt = rtalloc1_scoped_locked(SA(&gw6), 1, 0,
				    ifp->if_index);

				RT_LOCK(rt);
				  
                                              
                                            
                                           
       
				if (!(rt->rt_flags & RTF_UP) ||
				    gwrt == NULL || gwrt == rt ||
				    !equal(SA(&gw6), rt->rt_gateway)) {
					if (gwrt == rt) {
						RT_REMREF_LOCKED(gwrt);
						gwrt = NULL;
					}
					RT_UNLOCK(rt);
					if (gwrt != NULL) {
						rtfree_locked(gwrt);
					}
					lck_mtx_unlock(rnh_lock);
					senderr(EHOSTUNREACH);
				}
				VERIFY(gwrt != NULL);
				  
                                                  
                                                
                    
       
				rt_set_gwroute(rt, rt_key(rt), gwrt);
				RT_UNLOCK(rt);
				lck_mtx_unlock(rnh_lock);
				                                              
				rtrele = rt;
				rt = gwrt;
			} else {
				RT_ADDREF_LOCKED(gwrt);
				RT_UNLOCK(gwrt);
				RT_UNLOCK(rt);
				                                              
				rtrele = rt;
				rt = gwrt;
			}
			VERIFY(rt == gwrt);

			  
                                                     
                                                      
                                                        
                                                        
      
			RT_LOCK_SPIN(hint);
			if ((hint->rt_flags & (RTF_WASCLONED | RTF_UP)) ==
			    (RTF_WASCLONED | RTF_UP)) {
				struct rtentry *prt = hint->rt_parent;
				VERIFY(prt != NULL);

				RT_CONVERT_LOCK(hint);
				RT_ADDREF(prt);
				RT_UNLOCK(hint);
				rt_revalidate_gwroute(prt, rt);
				RT_REMREF(prt);
			} else {
				RT_UNLOCK(hint);
			}

			RT_LOCK_SPIN(rt);
			                                            
			if (!(rt->rt_flags & RTF_UP)) {
				RT_UNLOCK(rt);
				rtfree(rt);
				rt = NULL;
				                               
				senderr(EHOSTUNREACH);
			}
		}

		                            
		RT_CONVERT_LOCK(rt);
	}

	  
                                                           
                     
                                                                  
                                                
    

	                                                
	if (rt && (rt->rt_flags & RTF_LLINFO) != 0) {
		ln = rt->rt_llinfo;
	} else {
		struct sockaddr_in6 sin6;
		  
                                                
     
		sin6 = *dst;
		sin6.sin6_scope_id = 0;
		  
                                                                
                                                               
                                                         
                                                         
                                        
     
		if (rt != NULL) {
			RT_UNLOCK(rt);
		}
		if (nd6_is_addr_neighbor(&sin6, ifp, 0)) {
			                                                       
			if (rt != NULL) {
				                        
				if (rt == hint0) {
					RT_REMREF(rt);
				} else {
					rtfree(rt);
				}
			}
			                                                
			rt = nd6_lookup(&dst->sin6_addr, 1, ifp, 0);
			if (rt != NULL) {
				RT_LOCK_ASSERT_HELD(rt);
				ln = rt->rt_llinfo;
			}
		} else if (rt != NULL) {
			RT_LOCK(rt);
		}
	}

	if (!ln || !rt) {
		if (rt != NULL) {
			RT_UNLOCK(rt);
		}
		ndi = ND_IFINFO(ifp);
		VERIFY(ndi != NULL && ndi->initialized);
		lck_mtx_lock(&ndi->lock);
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0 &&
		    !(ndi->flags & ND6_IFF_PERFORMNUD)) {
			lck_mtx_unlock(&ndi->lock);
			log(LOG_DEBUG,
			    "nd6_output: can't allocate llinfo for %s "
			    "(ln=0x%llx, rt=0x%llx)\n",
			    ip6_sprintf(&dst->sin6_addr),
			    (uint64_t)VM_KERNEL_ADDRPERM(ln),
			    (uint64_t)VM_KERNEL_ADDRPERM(rt));
			senderr(EIO);                         
		}
		lck_mtx_unlock(&ndi->lock);

		goto sendpkt;                    
	}

	net_update_uptime();
	timenow = net_uptime();

	                                                                      
	if ((ifp->if_flags & IFF_POINTOPOINT) != 0 &&
	    ln->ln_state < ND6_LLINFO_REACHABLE) {
		ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_STALE);
		ln_setexpire(ln, timenow + nd6_gctimer);
	}

	  
                                                                
                                                                     
                                                         
                                                    
                    
    
	if (ln->ln_state == ND6_LLINFO_STALE) {
		ln->ln_asked = 0;
		ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_DELAY);
		ln_setexpire(ln, timenow + nd6_delay);
		                                           
		_CASSERT(ND6_LLINFO_DELAY > ND6_LLINFO_INCOMPLETE);
	}

	  
                                                                 
                                                           
                    
    
	if (ln->ln_state > ND6_LLINFO_INCOMPLETE) {
		RT_UNLOCK(rt);
		  
                                                           
                                                        
                                                            
                                                          
                                                          
                                       
     
		if (ln->ln_state == ND6_LLINFO_DELAY ||
		    (ln->ln_expire != 0 && ip6_neighborgcthresh > 0 &&
		    nd6_inuse >= (ip6_neighborgcthresh >> 1))) {
			lck_mtx_lock(rnh_lock);
			if (ln->ln_state == ND6_LLINFO_DELAY) {
				nd6_sched_timeout(NULL, NULL);
			}
			if (ln->ln_expire != 0 && ip6_neighborgcthresh > 0 &&
			    nd6_inuse >= (ip6_neighborgcthresh >> 1)) {
				RT_LOCK_SPIN(rt);
				if (ln->ln_flags & ND6_LNF_IN_USE) {
					LN_DEQUEUE(ln);
					LN_INSERTHEAD(ln);
				}
				RT_UNLOCK(rt);
			}
			lck_mtx_unlock(rnh_lock);
		}
		goto sendpkt;
	}

	  
                                                                 
                                                                   
                                                             
    
	if (rt->rt_flags & RTF_PROXY) {
		ln->ln_exclifp = ((origifp == ifp) ? NULL : origifp);
	}

	  
                                                            
                                                           
               
   
                                                                     
                                                                       
                
    
	if (ln->ln_state == ND6_LLINFO_NOSTATE) {
		ND6_CACHE_STATE_TRANSITION(ln, ND6_LLINFO_INCOMPLETE);
	}
	if (ln->ln_hold) {
		m_freem_list(ln->ln_hold);
	}
	ln->ln_hold = m0;
	if (!ND6_LLINFO_PERMANENT(ln) && ln->ln_asked == 0) {
		ln->ln_asked++;
		ndi = ND_IFINFO(ifp);
		VERIFY(ndi != NULL && ndi->initialized);
		lck_mtx_lock(&ndi->lock);
		ln_setexpire(ln, timenow + ndi->retrans / 1000);
		lck_mtx_unlock(&ndi->lock);
		RT_UNLOCK(rt);
		                                              
		if (ip6_forwarding) {
			nd6_prproxy_ns_output(ifp, origifp, NULL,
			    &dst->sin6_addr, ln);
		} else {
			nd6_ns_output(ifp, NULL, &dst->sin6_addr, ln, NULL);
		}
		lck_mtx_lock(rnh_lock);
		nd6_sched_timeout(NULL, NULL);
		lck_mtx_unlock(rnh_lock);
	} else {
		RT_UNLOCK(rt);
	}
	  
                                                          
                                                       
                                                           
                                                         
                                                         
                                      
    
	if (ln->ln_expire != 0 && ip6_neighborgcthresh > 0 &&
	    nd6_inuse >= (ip6_neighborgcthresh >> 1)) {
		lck_mtx_lock(rnh_lock);
		RT_LOCK_SPIN(rt);
		if (ln->ln_flags & ND6_LNF_IN_USE) {
			LN_DEQUEUE(ln);
			LN_INSERTHEAD(ln);
		}
		                                    
		if (rt == hint0) {
			RT_REMREF_LOCKED(rt);
			RT_UNLOCK(rt);
		} else {
			RT_UNLOCK(rt);
			rtfree_locked(rt);
		}
		rt = NULL;                                       
		lck_mtx_unlock(rnh_lock);
	}
	error = 0;
	goto release;

sendpkt:
	if (rt != NULL) {
		RT_LOCK_ASSERT_NOTHELD(rt);
	}

	                                                                       
	if (ifp->if_eflags & IFEF_IPV6_DISABLED) {
		error = ENETDOWN;                    
		goto bad;
	}

	if (ifp->if_flags & IFF_LOOPBACK) {
		                                                    
		m0->m_pkthdr.rcvif = origifp;
		error = dlil_output(origifp, PF_INET6, m0, (caddr_t)rt,
		    SA(dst), 0, adv);
		goto release;
	} else {
		                                                        
		struct ip6_hdr *ip6 = mtod(m0, struct ip6_hdr *);

		if ((IN6_IS_ADDR_LOOPBACK(&ip6->ip6_src) ||
		    IN6_IS_ADDR_LOOPBACK(&ip6->ip6_dst))) {
			ip6stat.ip6s_badscope++;
			error = EADDRNOTAVAIL;
			goto bad;
		}
	}

	if (rt != NULL) {
		RT_LOCK_SPIN(rt);
		                        
		if (rt->rt_llinfo != NULL) {
			nd6_llreach_use(rt->rt_llinfo);
		}
		RT_UNLOCK(rt);
	}

	struct mbuf *mcur = m0;
	uint32_t pktcnt = 0;

	while (mcur) {
		if (hint != NULL && nstat_collect) {
			int scnt;

			if ((mcur->m_pkthdr.csum_flags & CSUM_TSO_IPV6) &&
			    (mcur->m_pkthdr.tso_segsz > 0)) {
				scnt = mcur->m_pkthdr.len / mcur->m_pkthdr.tso_segsz;
			} else {
				scnt = 1;
			}

			nstat_route_tx(hint, scnt, mcur->m_pkthdr.len, 0);
		}
		pktcnt++;

		mcur->m_pkthdr.rcvif = NULL;
		mcur = mcur->m_nextpkt;
	}
	if (pktcnt > ip6_maxchainsent) {
		ip6_maxchainsent = pktcnt;
	}
	error = dlil_output(ifp, PF_INET6, m0, (caddr_t)rt, SA(dst), 0, adv);
	goto release;

bad:
	if (m0 != NULL) {
		m_freem_list(m0);
	}

release:
	                                                 
	if (rt != NULL) {
		RT_LOCK_SPIN(rt);
		if (rt == hint0) {
			RT_REMREF_LOCKED(rt);
			RT_UNLOCK(rt);
		} else {
			RT_UNLOCK(rt);
			rtfree(rt);
		}
	}
	                                               
	if (rtrele != NULL) {
		RT_LOCK_SPIN(rtrele);
		if (rtrele == hint0) {
			RT_REMREF_LOCKED(rtrele);
			RT_UNLOCK(rtrele);
		} else {
			RT_UNLOCK(rtrele);
			rtfree(rtrele);
		}
	}
	return error;
}
#undef senderr

int
nd6_need_cache(struct ifnet *ifp)
{
	  
                                                                 
                                              
   
                 
                                        
    
	switch (ifp->if_type) {
	case IFT_ARCNET:
	case IFT_ETHER:
	case IFT_FDDI:
	case IFT_IEEE1394:
	case IFT_L2VLAN:
	case IFT_IEEE8023ADLAG:
#if IFT_IEEE80211
	case IFT_IEEE80211:
#endif
	case IFT_GIF:                                     
	case IFT_PPP:
#if IFT_TUNNEL
	case IFT_TUNNEL:
#endif
	case IFT_BRIDGE:
	case IFT_CELLULAR:
	case IFT_6LOWPAN:
		return 1;
	default:
		return 0;
	}
}

int
nd6_storelladdr(struct ifnet *ifp, struct rtentry *rt, struct mbuf *m,
    struct sockaddr *dst, u_char *desten)
{
	int i;
	struct sockaddr_dl *sdl;

	if (m->m_flags & M_MCAST) {
		switch (ifp->if_type) {
		case IFT_ETHER:
		case IFT_FDDI:
		case IFT_L2VLAN:
		case IFT_IEEE8023ADLAG:
#if IFT_IEEE80211
		case IFT_IEEE80211:
#endif
		case IFT_BRIDGE:
			ETHER_MAP_IPV6_MULTICAST(&SIN6(dst)->sin6_addr, desten);
			return 1;
		case IFT_IEEE1394:
			for (i = 0; i < ifp->if_addrlen; i++) {
				desten[i] = ~0;
			}
			return 1;
		case IFT_ARCNET:
			*desten = 0;
			return 1;
		default:
			return 0;                            
		}
	}

	if (rt == NULL) {
		                                                        
		return 0;                            
	}
	RT_LOCK(rt);
	if (rt->rt_gateway->sa_family != AF_LINK) {
		printf("nd6_storelladdr: something odd happens\n");
		RT_UNLOCK(rt);
		return 0;                            
	}
	sdl = SDL(rt->rt_gateway);
	if (sdl->sdl_alen == 0) {
		                                                               
		printf("nd6_storelladdr: sdl_alen == 0\n");
		RT_UNLOCK(rt);
		return 0;                            
	}

	bcopy(LLADDR(sdl), desten, sdl->sdl_alen);
	RT_UNLOCK(rt);
	return 1;
}

  
                                                                       
                                                                       
                                                                      
               
   
errno_t
nd6_lookup_ipv6(ifnet_t  ifp, const struct sockaddr_in6 *ip6_dest,
    struct sockaddr_dl *ll_dest, size_t ll_dest_len, route_t hint,
    mbuf_t packet)
{
	route_t route = hint;
	errno_t result = 0;
	struct sockaddr_dl *sdl = NULL;
	size_t  copy_len;

	if (ifp == NULL || ip6_dest == NULL) {
		return EINVAL;
	}

	if (ip6_dest->sin6_family != AF_INET6) {
		return EAFNOSUPPORT;
	}

	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING)) {
		return ENETDOWN;
	}

	if (hint != NULL) {
		  
                                                      
                                               
     
		result = route_to_gwroute((const struct sockaddr *)ip6_dest,
		    hint, &route);
		if (result != 0) {
			return result;
		}
		if (route != NULL) {
			RT_LOCK_ASSERT_HELD(route);
		}
	}

	if ((packet != NULL && (packet->m_flags & M_MCAST) != 0) ||
	    ((ifp->if_flags & IFF_MULTICAST) &&
	    IN6_IS_ADDR_MULTICAST(&ip6_dest->sin6_addr))) {
		if (route != NULL) {
			RT_UNLOCK(route);
		}
		result = dlil_resolve_multi(ifp,
		    (const struct sockaddr *)ip6_dest,
		    SA(ll_dest), ll_dest_len);
		if (route != NULL) {
			RT_LOCK(route);
		}
		goto release;
	} else if (route == NULL) {
		  
                    
                                                 
                                
     
		lck_mtx_lock(rnh_lock);
		route = rt_lookup(TRUE,
		    __DECONST(struct sockaddr *, ip6_dest), NULL,
		    rt_tables[AF_INET6], ifp->if_index);
		lck_mtx_unlock(rnh_lock);

		if (route != NULL) {
			RT_LOCK(route);
		}
	}

	if (route == NULL) {
		  
                                                          
                                                 
     
		result = ENOBUFS;
		goto release;
	}

	if (route->rt_gateway->sa_family != AF_LINK) {
		nd6log0(error, "%s: route %s on %s%d gateway address not AF_LINK\n",
		    __func__, ip6_sprintf(&ip6_dest->sin6_addr),
		    route->rt_ifp->if_name, route->rt_ifp->if_unit);
		result = EADDRNOTAVAIL;
		goto release;
	}

	sdl = SDL(route->rt_gateway);
	if (sdl->sdl_alen == 0) {
		                                                               
		nd6log(error, "%s: route %s on %s%d sdl_alen == 0\n", __func__,
		    ip6_sprintf(&ip6_dest->sin6_addr), route->rt_ifp->if_name,
		    route->rt_ifp->if_unit);
		result = EHOSTUNREACH;
		goto release;
	}

	copy_len = sdl->sdl_len <= ll_dest_len ? sdl->sdl_len : ll_dest_len;
	bcopy(sdl, ll_dest, copy_len);

release:
	if (route != NULL) {
		if (route == hint) {
			RT_REMREF_LOCKED(route);
			RT_UNLOCK(route);
		} else {
			RT_UNLOCK(route);
			rtfree(route);
		}
	}
	return result;
}

#if (DEVELOPMENT || DEBUG)

static int sysctl_nd6_lookup_ipv6 SYSCTL_HANDLER_ARGS;
SYSCTL_PROC(_net_inet6_icmp6, OID_AUTO, nd6_lookup_ipv6,
    CTLTYPE_STRUCT | CTLFLAG_RW | CTLFLAG_LOCKED, 0, 0,
    sysctl_nd6_lookup_ipv6, "S", "");

int
sysctl_nd6_lookup_ipv6 SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int error = 0;
	struct nd6_lookup_ipv6_args nd6_lookup_ipv6_args;
	ifnet_t ifp = NULL;

	  
                                      
    
	error = proc_suser(current_proc());
	if (error != 0) {
		nd6log0(error, "%s: proc_suser() error %d\n",
		    __func__, error);
		goto done;
	}
	if (req->oldptr == USER_ADDR_NULL) {
		req->oldidx = sizeof(struct nd6_lookup_ipv6_args);
	}
	if (req->newptr == USER_ADDR_NULL) {
		goto done;
	}
	if (req->oldlen != sizeof(struct nd6_lookup_ipv6_args) ||
	    req->newlen != sizeof(struct nd6_lookup_ipv6_args)) {
		error = EINVAL;
		nd6log0(error, "%s: bad req, error %d\n",
		    __func__, error);
		goto done;
	}
	error = SYSCTL_IN(req, &nd6_lookup_ipv6_args,
	    sizeof(struct nd6_lookup_ipv6_args));
	if (error != 0) {
		nd6log0(error, "%s: SYSCTL_IN() error %d\n",
		    __func__, error);
		goto done;
	}

	if (nd6_lookup_ipv6_args.ll_dest_len > sizeof(nd6_lookup_ipv6_args.ll_dest_)) {
		error = EINVAL;
		nd6log0(error, "%s: bad ll_dest_len, error %d\n",
		    __func__, error);
		goto done;
	}

	                                       
	nd6_lookup_ipv6_args.ifname[IFNAMSIZ - 1] = 0;

	error = ifnet_find_by_name(nd6_lookup_ipv6_args.ifname, &ifp);
	if (error != 0) {
		nd6log0(error, "%s: ifnet_find_by_name() error %d\n",
		    __func__, error);
		goto done;
	}

	error = nd6_lookup_ipv6(ifp, &nd6_lookup_ipv6_args.ip6_dest,
	    &nd6_lookup_ipv6_args.ll_dest_._sdl,
	    nd6_lookup_ipv6_args.ll_dest_len, NULL, NULL);
	if (error != 0) {
		nd6log0(error, "%s: nd6_lookup_ipv6() error %d\n",
		    __func__, error);
		goto done;
	}

	error = SYSCTL_OUT(req, &nd6_lookup_ipv6_args,
	    sizeof(struct nd6_lookup_ipv6_args));
	if (error != 0) {
		nd6log0(error, "%s: SYSCTL_OUT() error %d\n",
		    __func__, error);
		goto done;
	}
done:
	return error;
}

#endif                              

int
nd6_setifinfo(struct ifnet *ifp, u_int32_t before, u_int32_t after)
{
	uint32_t b, a;
	int err = 0;

	  
                             
    
	if ((before & ND6_IFF_IFDISABLED) ||
	    (after & ND6_IFF_IFDISABLED)) {
		b = (before & ND6_IFF_IFDISABLED);
		a = (after & ND6_IFF_IFDISABLED);

		if (b != a && (err = nd6_if_disable(ifp,
		    ((int32_t)(a - b) > 0))) != 0) {
			goto done;
		}
	}

	  
                                 
    
	if ((before & ND6_IFF_PROXY_PREFIXES) ||
	    (after & ND6_IFF_PROXY_PREFIXES)) {
		b = (before & ND6_IFF_PROXY_PREFIXES);
		a = (after & ND6_IFF_PROXY_PREFIXES);

		if (b != a && (err = nd6_if_prproxy(ifp,
		    ((int32_t)(a - b) > 0))) != 0) {
			goto done;
		}
	}
done:
	return err;
}

  
                                                         
                                                              
   
int
nd6_if_disable(struct ifnet *ifp, boolean_t enable)
{
	if (enable) {
		if_set_eflags(ifp, IFEF_IPV6_DISABLED);
	} else {
		if_clear_eflags(ifp, IFEF_IPV6_DISABLED);
	}

	return 0;
}

static int
nd6_sysctl_drlist SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	char pbuf[MAX_IPv6_STR_LEN];
	struct nd_defrouter *dr;
	int error = 0;

	if (req->newptr != USER_ADDR_NULL) {
		return EPERM;
	}

	                                         
	lck_mtx_lock(nd6_mutex);
	if (proc_is64bit(req->p)) {
		struct in6_defrouter_64 d;

		bzero(&d, sizeof(d));
		d.rtaddr.sin6_family = AF_INET6;
		d.rtaddr.sin6_len = sizeof(d.rtaddr);

		TAILQ_FOREACH(dr, &nd_defrouter_list, dr_entry) {
			d.rtaddr.sin6_addr = dr->rtaddr;
			if (in6_recoverscope(&d.rtaddr,
			    &dr->rtaddr, dr->ifp) != 0) {
				log(LOG_ERR, "scope error in default router "
				    "list (%s)\n", inet_ntop(AF_INET6,
				    &dr->rtaddr, pbuf, sizeof(pbuf)));
			}
			d.flags = dr->flags;
			d.stateflags = dr->stateflags;
			d.rtlifetime = (u_short)dr->rtlifetime;
			d.expire = (int)nddr_getexpire(dr);
			d.if_index = dr->ifp->if_index;
			error = SYSCTL_OUT(req, &d, sizeof(d));
			if (error != 0) {
				break;
			}
		}
	} else {
		struct in6_defrouter_32 d;

		bzero(&d, sizeof(d));
		d.rtaddr.sin6_family = AF_INET6;
		d.rtaddr.sin6_len = sizeof(d.rtaddr);

		TAILQ_FOREACH(dr, &nd_defrouter_list, dr_entry) {
			d.rtaddr.sin6_addr = dr->rtaddr;
			if (in6_recoverscope(&d.rtaddr,
			    &dr->rtaddr, dr->ifp) != 0) {
				log(LOG_ERR, "scope error in default router "
				    "list (%s)\n", inet_ntop(AF_INET6,
				    &dr->rtaddr, pbuf, sizeof(pbuf)));
			}
			d.flags = dr->flags;
			d.stateflags = dr->stateflags;
			d.rtlifetime = (u_short)dr->rtlifetime;
			d.expire = (int)nddr_getexpire(dr);
			d.if_index = dr->ifp->if_index;
			error = SYSCTL_OUT(req, &d, sizeof(d));
			if (error != 0) {
				break;
			}
		}
	}
	lck_mtx_unlock(nd6_mutex);
	return error;
}

static int
nd6_sysctl_prlist SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	char pbuf[MAX_IPv6_STR_LEN];
	struct nd_pfxrouter *pfr;
	struct sockaddr_in6 s6;
	struct nd_prefix *pr;
	int error = 0;

	if (req->newptr != USER_ADDR_NULL) {
		return EPERM;
	}

	bzero(&s6, sizeof(s6));
	s6.sin6_family = AF_INET6;
	s6.sin6_len = sizeof(s6);

	                                         
	lck_mtx_lock(nd6_mutex);
	if (proc_is64bit(req->p)) {
		struct in6_prefix_64 p;

		bzero(&p, sizeof(p));
		p.origin = PR_ORIG_RA;

		LIST_FOREACH(pr, &nd_prefix, ndpr_entry) {
			NDPR_LOCK(pr);
			p.prefix = pr->ndpr_prefix;
			if (in6_recoverscope(&p.prefix,
			    &pr->ndpr_prefix.sin6_addr, pr->ndpr_ifp) != 0) {
				log(LOG_ERR, "scope error in "
				    "prefix list (%s)\n", inet_ntop(AF_INET6,
				    &p.prefix.sin6_addr, pbuf, sizeof(pbuf)));
			}
			p.raflags = pr->ndpr_raf;
			p.prefixlen = pr->ndpr_plen;
			p.vltime = pr->ndpr_vltime;
			p.pltime = pr->ndpr_pltime;
			p.if_index = pr->ndpr_ifp->if_index;
			p.expire = (u_long)ndpr_getexpire(pr);
			p.refcnt = pr->ndpr_addrcnt;
			p.flags = pr->ndpr_stateflags;
			p.advrtrs = 0;
			LIST_FOREACH(pfr, &pr->ndpr_advrtrs, pfr_entry)
			p.advrtrs++;
			error = SYSCTL_OUT(req, &p, sizeof(p));
			if (error != 0) {
				NDPR_UNLOCK(pr);
				break;
			}
			LIST_FOREACH(pfr, &pr->ndpr_advrtrs, pfr_entry) {
				s6.sin6_addr = pfr->router->rtaddr;
				if (in6_recoverscope(&s6, &pfr->router->rtaddr,
				    pfr->router->ifp) != 0) {
					log(LOG_ERR,
					    "scope error in prefix list (%s)\n",
					    inet_ntop(AF_INET6, &s6.sin6_addr,
					    pbuf, sizeof(pbuf)));
				}
				error = SYSCTL_OUT(req, &s6, sizeof(s6));
				if (error != 0) {
					break;
				}
			}
			NDPR_UNLOCK(pr);
			if (error != 0) {
				break;
			}
		}
	} else {
		struct in6_prefix_32 p;

		bzero(&p, sizeof(p));
		p.origin = PR_ORIG_RA;

		LIST_FOREACH(pr, &nd_prefix, ndpr_entry) {
			NDPR_LOCK(pr);
			p.prefix = pr->ndpr_prefix;
			if (in6_recoverscope(&p.prefix,
			    &pr->ndpr_prefix.sin6_addr, pr->ndpr_ifp) != 0) {
				log(LOG_ERR,
				    "scope error in prefix list (%s)\n",
				    inet_ntop(AF_INET6, &p.prefix.sin6_addr,
				    pbuf, sizeof(pbuf)));
			}
			p.raflags = pr->ndpr_raf;
			p.prefixlen = pr->ndpr_plen;
			p.vltime = pr->ndpr_vltime;
			p.pltime = pr->ndpr_pltime;
			p.if_index = pr->ndpr_ifp->if_index;
			p.expire = (u_int32_t)ndpr_getexpire(pr);
			p.refcnt = pr->ndpr_addrcnt;
			p.flags = pr->ndpr_stateflags;
			p.advrtrs = 0;
			LIST_FOREACH(pfr, &pr->ndpr_advrtrs, pfr_entry)
			p.advrtrs++;
			error = SYSCTL_OUT(req, &p, sizeof(p));
			if (error != 0) {
				NDPR_UNLOCK(pr);
				break;
			}
			LIST_FOREACH(pfr, &pr->ndpr_advrtrs, pfr_entry) {
				s6.sin6_addr = pfr->router->rtaddr;
				if (in6_recoverscope(&s6, &pfr->router->rtaddr,
				    pfr->router->ifp) != 0) {
					log(LOG_ERR,
					    "scope error in prefix list (%s)\n",
					    inet_ntop(AF_INET6, &s6.sin6_addr,
					    pbuf, sizeof(pbuf)));
				}
				error = SYSCTL_OUT(req, &s6, sizeof(s6));
				if (error != 0) {
					break;
				}
			}
			NDPR_UNLOCK(pr);
			if (error != 0) {
				break;
			}
		}
	}
	lck_mtx_unlock(nd6_mutex);

	return error;
}

void
in6_ifaddr_set_dadprogress(struct in6_ifaddr *ia)
{
	struct ifnet* ifp = ia->ia_ifp;
	uint32_t flags = IN6_IFF_TENTATIVE;
	uint32_t optdad = nd6_optimistic_dad;
	struct nd_ifinfo *ndi = NULL;

	ndi = ND_IFINFO(ifp);
	VERIFY((NULL != ndi) && (TRUE == ndi->initialized));
	if (!(ndi->flags & ND6_IFF_DAD)) {
		return;
	}

	if (optdad) {
		if (ifp->if_ipv6_router_mode == IPV6_ROUTER_MODE_EXCLUSIVE) {
			optdad = 0;
		} else {
			lck_mtx_lock(&ndi->lock);
			if ((ndi->flags & ND6_IFF_REPLICATED) != 0) {
				optdad = 0;
			}
			lck_mtx_unlock(&ndi->lock);
		}
	}

	if (optdad) {
		if ((optdad & ND6_OPTIMISTIC_DAD_LINKLOCAL) &&
		    IN6_IS_ADDR_LINKLOCAL(&ia->ia_addr.sin6_addr)) {
			flags = IN6_IFF_OPTIMISTIC;
		} else if ((optdad & ND6_OPTIMISTIC_DAD_AUTOCONF) &&
		    (ia->ia6_flags & IN6_IFF_AUTOCONF)) {
			if (ia->ia6_flags & IN6_IFF_TEMPORARY) {
				if (optdad & ND6_OPTIMISTIC_DAD_TEMPORARY) {
					flags = IN6_IFF_OPTIMISTIC;
				}
			} else if (ia->ia6_flags & IN6_IFF_SECURED) {
				if (optdad & ND6_OPTIMISTIC_DAD_SECURED) {
					flags = IN6_IFF_OPTIMISTIC;
				}
			} else {
				  
                                            
                                                    
           
                                                     
                                              
           
                                                        
                                                  
                   
       
				flags = IN6_IFF_OPTIMISTIC;
			}
		} else if ((optdad & ND6_OPTIMISTIC_DAD_DYNAMIC) &&
		    (ia->ia6_flags & IN6_IFF_DYNAMIC)) {
			if (ia->ia6_flags & IN6_IFF_TEMPORARY) {
				if (optdad & ND6_OPTIMISTIC_DAD_TEMPORARY) {
					flags = IN6_IFF_OPTIMISTIC;
				}
			} else {
				flags = IN6_IFF_OPTIMISTIC;
			}
		} else if ((optdad & ND6_OPTIMISTIC_DAD_MANUAL) &&
		    (ia->ia6_flags & IN6_IFF_OPTIMISTIC)) {
			  
                     
                                              
                                                  
      
			if (!IN6_IS_ADDR_LINKLOCAL(&ia->ia_addr.sin6_addr) &&
			    !(ia->ia6_flags & IN6_IFF_AUTOCONF) &&
			    !(ia->ia6_flags & IN6_IFF_DYNAMIC)) {
				flags = IN6_IFF_OPTIMISTIC;
			}
		}
	}

	ia->ia6_flags &= ~(IN6_IFF_DUPLICATED | IN6_IFF_DADPROGRESS);
	ia->ia6_flags |= flags;

	nd6log2(debug, "%s - %s ifp %s ia6_flags 0x%x\n",
	    __func__,
	    ip6_sprintf(&ia->ia_addr.sin6_addr),
	    if_name(ia->ia_ifp),
	    ia->ia6_flags);
}
