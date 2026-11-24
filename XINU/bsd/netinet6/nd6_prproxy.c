/*
 * Copyright (c) 2011-2020 Apple Inc. All rights reserved.
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
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/sysctl.h>
#include <sys/mcache.h>
#include <sys/protosw.h>

#include <kern/queue.h>
#include <kern/zalloc.h>

#include <net/if.h>
#include <net/if_var.h>
#include <net/if_types.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/nd6.h>
#include <netinet6/scope6_var.h>

struct nd6_prproxy_prelist {
	SLIST_ENTRY(nd6_prproxy_prelist) ndprl_le;
	struct nd_prefix *ndprl_pr;                         
	struct nd_prefix *ndprl_up;                                        
	struct ifnet    *ndprl_fwd_ifp;                                 
	boolean_t       ndprl_sol;                                      
	struct in6_addr ndprl_sol_saddr;                                 
};

  
                                   
   
struct nd6_prproxy_solsrc {
	TAILQ_ENTRY(nd6_prproxy_solsrc) solsrc_tqe;
	struct in6_addr solsrc_saddr;                                         
	struct ifnet    *solsrc_ifp;                                           
};

  
                                  
   
struct nd6_prproxy_soltgt {
	RB_ENTRY(nd6_prproxy_soltgt) soltgt_link;                    
	struct soltgt_key_s {
		struct in6_addr taddr;                                       
	} soltgt_key;
	u_int64_t       soltgt_expire;                               
	u_int32_t       soltgt_cnt;                                        
	TAILQ_HEAD(, nd6_prproxy_solsrc) soltgt_q;
};

SLIST_HEAD(nd6_prproxy_prelist_head, nd6_prproxy_prelist);

static void nd6_prproxy_prelist_setroute(boolean_t enable,
    struct nd6_prproxy_prelist_head *, struct nd6_prproxy_prelist_head *);
static struct nd6_prproxy_prelist *nd6_ndprl_alloc(zalloc_flags_t);
static void nd6_ndprl_free(struct nd6_prproxy_prelist *);
static struct nd6_prproxy_solsrc *nd6_solsrc_alloc(int);
static void nd6_solsrc_free(struct nd6_prproxy_solsrc *);
static boolean_t nd6_solsrc_enq(struct nd_prefix *, struct ifnet *,
    struct in6_addr *, struct in6_addr *);
static boolean_t nd6_solsrc_deq(struct nd_prefix *, struct in6_addr *,
    struct in6_addr *, struct ifnet **);
static struct nd6_prproxy_soltgt *nd6_soltgt_alloc(int);
static void nd6_soltgt_free(struct nd6_prproxy_soltgt *);
static void nd6_soltgt_prune(struct nd6_prproxy_soltgt *, u_int32_t);
static __inline int soltgt_cmp(const struct nd6_prproxy_soltgt *,
    const struct nd6_prproxy_soltgt *);
static void nd6_prproxy_sols_purge(struct nd_prefix *, u_int64_t);

RB_PROTOTYPE_SC_PREV(__private_extern__, prproxy_sols_tree, nd6_prproxy_soltgt,
    soltgt_link, soltgt_cmp);

  
                                                              
   
#define ND6_TGT_SOLS_EXPIRE                     5

  
                                                                   
   
#define ND6_MAX_SRC_SOLS_DEFAULT                4

  
                                                                  
   
#define ND6_MAX_TGT_SOLS_DEFAULT                8

static u_int32_t nd6_max_tgt_sols = ND6_MAX_TGT_SOLS_DEFAULT;
static u_int32_t nd6_max_src_sols = ND6_MAX_SRC_SOLS_DEFAULT;

static ZONE_DECLARE(ndprl_zone, "nd6_prproxy_prelist",
    sizeof(struct nd6_prproxy_prelist), ZC_ZFREE_CLEARMEM);                                  

static ZONE_DECLARE(solsrc_zone, "nd6_prproxy_solsrc",
    sizeof(struct nd6_prproxy_solsrc), ZC_ZFREE_CLEARMEM);                                  

static ZONE_DECLARE(soltgt_zone, "nd6_prproxy_soltgt",
    sizeof(struct nd6_prproxy_soltgt), ZC_ZFREE_CLEARMEM);                                  

                                             
RB_GENERATE_PREV(prproxy_sols_tree, nd6_prproxy_soltgt,
    soltgt_link, soltgt_cmp);

                                                             
u_int32_t nd6_prproxy;

extern lck_mtx_t *nd6_mutex;

SYSCTL_DECL(_net_inet6_icmp6);

SYSCTL_UINT(_net_inet6_icmp6, OID_AUTO, nd6_maxsolstgt,
    CTLFLAG_RW | CTLFLAG_LOCKED, &nd6_max_tgt_sols, ND6_MAX_TGT_SOLS_DEFAULT,
    "maximum number of outstanding solicited targets per prefix");

SYSCTL_UINT(_net_inet6_icmp6, OID_AUTO, nd6_maxproxiedsol,
    CTLFLAG_RW | CTLFLAG_LOCKED, &nd6_max_src_sols, ND6_MAX_SRC_SOLS_DEFAULT,
    "maximum number of outstanding solicitations per target");

SYSCTL_UINT(_net_inet6_icmp6, OID_AUTO, prproxy_cnt,
    CTLFLAG_RD | CTLFLAG_LOCKED, &nd6_prproxy, 0,
    "total number of proxied prefixes");

static struct nd6_prproxy_prelist *
nd6_ndprl_alloc(zalloc_flags_t how)
{
	return zalloc_flags(ndprl_zone, how | Z_ZERO);
}

static void
nd6_ndprl_free(struct nd6_prproxy_prelist *ndprl)
{
	zfree(ndprl_zone, ndprl);
}

  
                                                                           
                                                                            
                                                                          
                                                                    
   
static void
nd6_prproxy_prelist_setroute(boolean_t enable,
    struct nd6_prproxy_prelist_head *up_head,
    struct nd6_prproxy_prelist_head *down_head)
{
	struct nd6_prproxy_prelist *up, *down, *ndprl_tmp;
	struct nd_prefix *pr;

	LCK_MTX_ASSERT(&proxy6_lock, LCK_MTX_ASSERT_OWNED);
	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_NOTOWNED);

	SLIST_FOREACH_SAFE(up, up_head, ndprl_le, ndprl_tmp) {
		struct rtentry *rt;
		boolean_t prproxy, set_allmulti = FALSE;
		int allmulti_sw = FALSE;
		struct ifnet *ifp = NULL;

		SLIST_REMOVE(up_head, up, nd6_prproxy_prelist, ndprl_le);
		pr = up->ndprl_pr;
		VERIFY(up->ndprl_up == NULL);

		NDPR_LOCK(pr);
		ifp = pr->ndpr_ifp;
		prproxy = (pr->ndpr_stateflags & NDPRF_PRPROXY);
		VERIFY(!prproxy || ((pr->ndpr_stateflags & NDPRF_ONLINK) &&
		    !(pr->ndpr_stateflags & NDPRF_IFSCOPE)));

		nd6_prproxy_sols_reap(pr);
		VERIFY(pr->ndpr_prproxy_sols_cnt == 0);
		VERIFY(RB_EMPTY(&pr->ndpr_prproxy_sols));

		if (enable && pr->ndpr_allmulti_cnt == 0) {
			nd6_prproxy++;
			pr->ndpr_allmulti_cnt++;
			set_allmulti = TRUE;
			allmulti_sw = TRUE;
		} else if (!enable && pr->ndpr_allmulti_cnt > 0) {
			nd6_prproxy--;
			pr->ndpr_allmulti_cnt--;
			set_allmulti = TRUE;
			allmulti_sw = FALSE;
		}

		if ((rt = pr->ndpr_rt) != NULL) {
			if ((enable && prproxy) || (!enable && !prproxy)) {
				RT_ADDREF(rt);
			} else {
				rt = NULL;
			}
			NDPR_UNLOCK(pr);
		} else {
			NDPR_UNLOCK(pr);
		}

		                                                        
		if (set_allmulti && ifp != NULL) {
			if_allmulti(ifp, allmulti_sw);
		}


		NDPR_REMREF(pr);
		if (rt != NULL) {
			rt_set_proxy(rt, enable);
			rtfree(rt);
		}
		nd6_ndprl_free(up);
	}

	SLIST_FOREACH_SAFE(down, down_head, ndprl_le, ndprl_tmp) {
		struct nd_prefix *pr_up;
		struct rtentry *rt;
		boolean_t prproxy, set_allmulti = FALSE;
		int allmulti_sw = FALSE;
		struct ifnet *ifp = NULL;

		SLIST_REMOVE(down_head, down, nd6_prproxy_prelist, ndprl_le);
		pr = down->ndprl_pr;
		pr_up = down->ndprl_up;
		VERIFY(pr_up != NULL);

		NDPR_LOCK(pr_up);
		ifp = pr->ndpr_ifp;
		prproxy = (pr_up->ndpr_stateflags & NDPRF_PRPROXY);
		VERIFY(!prproxy || ((pr_up->ndpr_stateflags & NDPRF_ONLINK) &&
		    !(pr_up->ndpr_stateflags & NDPRF_IFSCOPE)));
		NDPR_UNLOCK(pr_up);

		NDPR_LOCK(pr);
		if (enable && pr->ndpr_allmulti_cnt == 0) {
			pr->ndpr_allmulti_cnt++;
			set_allmulti = TRUE;
			allmulti_sw = TRUE;
		} else if (!enable && pr->ndpr_allmulti_cnt > 0) {
			pr->ndpr_allmulti_cnt--;
			set_allmulti = TRUE;
			allmulti_sw = FALSE;
		}

		if ((rt = pr->ndpr_rt) != NULL) {
			if ((enable && prproxy) || (!enable && !prproxy)) {
				RT_ADDREF(rt);
			} else {
				rt = NULL;
			}
			NDPR_UNLOCK(pr);
		} else {
			NDPR_UNLOCK(pr);
		}
		if (set_allmulti && ifp != NULL) {
			if_allmulti(ifp, allmulti_sw);
		}

		NDPR_REMREF(pr);
		NDPR_REMREF(pr_up);
		if (rt != NULL) {
			rt_set_proxy(rt, enable);
			rtfree(rt);
		}
		nd6_ndprl_free(down);
	}
}

  
                                                                   
                                                           
   
int
nd6_if_prproxy(struct ifnet *ifp, boolean_t enable)
{
	SLIST_HEAD(, nd6_prproxy_prelist) up_head;
	SLIST_HEAD(, nd6_prproxy_prelist) down_head;
	struct nd6_prproxy_prelist *up, *down;
	struct nd_prefix *pr;

	                                                                       
	ifnet_lock_shared(ifp);
	if (enable && (ifp->if_ipv6_router_mode == IPV6_ROUTER_MODE_EXCLUSIVE)) {
		ifnet_lock_done(ifp);
		return EBUSY;
	}
	ifnet_lock_done(ifp);

	SLIST_INIT(&up_head);
	SLIST_INIT(&down_head);

	  
                                                    
    
	lck_mtx_lock(&proxy6_lock);

	  
                                                                 
                                                               
    
	lck_mtx_lock(nd6_mutex);
	for (pr = nd_prefix.lh_first; pr; pr = pr->ndpr_next) {
		NDPR_LOCK(pr);
		if (IN6_IS_ADDR_LINKLOCAL(&pr->ndpr_prefix.sin6_addr) ||
		    (!enable && !(pr->ndpr_stateflags & NDPRF_PRPROXY)) ||
		    (enable && (pr->ndpr_stateflags & NDPRF_PRPROXY)) ||
		    (pr->ndpr_stateflags & NDPRF_IFSCOPE) ||
		    pr->ndpr_ifp != ifp) {
			NDPR_UNLOCK(pr);
			continue;
		}

		  
                                                       
                                                      
                                                         
                                                     
                                 
     
		if (enable && (pr->ndpr_stateflags & NDPRF_ONLINK) &&
		    nd6_need_cache(ifp)) {
			pr->ndpr_stateflags |= NDPRF_PRPROXY;
			NDPR_ADDREF(pr);
			NDPR_UNLOCK(pr);
		} else if (!enable) {
			pr->ndpr_stateflags &= ~NDPRF_PRPROXY;
			NDPR_ADDREF(pr);
			NDPR_UNLOCK(pr);
		} else {
			NDPR_UNLOCK(pr);
			pr = NULL;                            
		}

		if (pr == NULL) {
			break;
		}

		up = nd6_ndprl_alloc(Z_WAITOK);
		if (up == NULL) {
			NDPR_REMREF(pr);
			continue;
		}

		up->ndprl_pr = pr;                                     
		SLIST_INSERT_HEAD(&up_head, up, ndprl_le);
	}

	  
                                                                      
                                                                        
                                                                      
    
	SLIST_FOREACH(up, &up_head, ndprl_le) {
		struct nd_prefix *fwd;
		struct in6_addr pr_addr;
		u_char pr_len;

		pr = up->ndprl_pr;

		NDPR_LOCK(pr);
		bcopy(&pr->ndpr_prefix.sin6_addr, &pr_addr, sizeof(pr_addr));
		pr_len = pr->ndpr_plen;
		NDPR_UNLOCK(pr);

		for (fwd = nd_prefix.lh_first; fwd; fwd = fwd->ndpr_next) {
			NDPR_LOCK(fwd);
			if (!(fwd->ndpr_stateflags & NDPRF_ONLINK) ||
			    !(fwd->ndpr_stateflags & NDPRF_IFSCOPE) ||
			    fwd->ndpr_plen != pr_len ||
			    !in6_are_prefix_equal(&fwd->ndpr_prefix.sin6_addr,
			    &pr_addr, pr_len)) {
				NDPR_UNLOCK(fwd);
				continue;
			}
			NDPR_UNLOCK(fwd);

			down = nd6_ndprl_alloc(Z_WAITOK);
			if (down == NULL) {
				continue;
			}

			NDPR_ADDREF(fwd);
			down->ndprl_pr = fwd;
			NDPR_ADDREF(pr);
			down->ndprl_up = pr;
			SLIST_INSERT_HEAD(&down_head, down, ndprl_le);
		}
	}
	lck_mtx_unlock(nd6_mutex);

	  
                                                                   
    
	nd6_prproxy_prelist_setroute(enable,
	    (struct nd6_prproxy_prelist_head *)&up_head,
	    (struct nd6_prproxy_prelist_head *)&down_head);

	VERIFY(SLIST_EMPTY(&up_head));
	VERIFY(SLIST_EMPTY(&down_head));

	lck_mtx_unlock(&proxy6_lock);

	return 0;
}

  
                                                                         
                                                                       
                                                                     
   
boolean_t
nd6_prproxy_isours(struct mbuf *m, struct ip6_hdr *ip6, struct route_in6 *ro6,
    unsigned int ifscope)
{
	struct rtentry *rt;
	boolean_t ours = FALSE;

	if (ip6->ip6_hlim != IPV6_MAXHLIM || ip6->ip6_nxt != IPPROTO_ICMPV6) {
		goto done;
	}

	if (IN6_IS_ADDR_MC_NODELOCAL(&ip6->ip6_dst) ||
	    IN6_IS_ADDR_MC_LINKLOCAL(&ip6->ip6_dst)) {
		VERIFY(ro6 == NULL);
		ours = TRUE;
		goto done;
	} else if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst)) {
		goto done;
	}

	if (ro6 == NULL) {
		goto done;
	}

	if ((rt = ro6->ro_rt) != NULL) {
		RT_LOCK(rt);
	}

	if (ROUTE_UNUSABLE(ro6)) {
		if (rt != NULL) {
			RT_UNLOCK(rt);
		}

		ROUTE_RELEASE(ro6);

		                                                         
		VERIFY(IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst,
		    &ro6->ro_dst.sin6_addr));

		rtalloc_scoped_ign((struct route *)ro6, RTF_PRCLONING, ifscope);
		if ((rt = ro6->ro_rt) == NULL) {
			goto done;
		}

		RT_LOCK(rt);
	}

	ours = (rt->rt_flags & RTF_PROXY) ? TRUE : FALSE;
	RT_UNLOCK(rt);

done:
	if (ours) {
		m->m_pkthdr.pkt_flags |= PKTF_PROXY_DST;
	}

	return ours;
}

  
                                                                   
                                                                   
                                        
   
void
nd6_proxy_find_fwdroute(struct ifnet *ifp, struct route_in6 *ro6)
{
	struct in6_addr *dst6 = &ro6->ro_dst.sin6_addr;
	struct ifnet *fwd_ifp = NULL;
	struct nd_prefix *pr;
	struct rtentry *rt;

	if ((rt = ro6->ro_rt) != NULL) {
		RT_LOCK(rt);
		if (!(rt->rt_flags & RTF_PROXY) || rt->rt_ifp == ifp) {
			nd6log2(debug, "%s: found incorrect prefix "
			    "proxy route for dst %s on %s\n", if_name(ifp),
			    ip6_sprintf(dst6),
			    if_name(rt->rt_ifp));
			RT_UNLOCK(rt);
			                      
		} else {
			RT_UNLOCK(rt);
			  
                                                      
                                                      
                                              
      
			return;
		}
	}

	  
                                                                 
                                                               
                                                                    
                                                                 
               
    
	lck_mtx_lock(nd6_mutex);
	for (pr = nd_prefix.lh_first; pr; pr = pr->ndpr_next) {
		struct in6_addr pr_addr;
		struct nd_prefix *fwd;
		u_char pr_len;

		NDPR_LOCK(pr);
		if (!(pr->ndpr_stateflags & NDPRF_ONLINK) ||
		    !(pr->ndpr_stateflags & NDPRF_PRPROXY) ||
		    !IN6_ARE_MASKED_ADDR_EQUAL(&pr->ndpr_prefix.sin6_addr,
		    dst6, &pr->ndpr_mask)) {
			NDPR_UNLOCK(pr);
			continue;
		}

		VERIFY(!(pr->ndpr_stateflags & NDPRF_IFSCOPE));
		bcopy(&pr->ndpr_prefix.sin6_addr, &pr_addr, sizeof(pr_addr));
		pr_len = pr->ndpr_plen;
		NDPR_UNLOCK(pr);

		for (fwd = nd_prefix.lh_first; fwd; fwd = fwd->ndpr_next) {
			NDPR_LOCK(fwd);
			if (!(fwd->ndpr_stateflags & NDPRF_ONLINK) ||
			    fwd->ndpr_ifp == ifp ||
			    fwd->ndpr_plen != pr_len ||
			    !in6_are_prefix_equal(&fwd->ndpr_prefix.sin6_addr,
			    &pr_addr, pr_len)) {
				NDPR_UNLOCK(fwd);
				continue;
			}

			fwd_ifp = fwd->ndpr_ifp;
			NDPR_UNLOCK(fwd);
			break;
		}
		break;
	}
	lck_mtx_unlock(nd6_mutex);

	lck_mtx_lock(rnh_lock);
	ROUTE_RELEASE_LOCKED(ro6);

	  
                                                                  
                                                               
                               
    
	if ((rt = rtalloc1_scoped_locked(SA(&ro6->ro_dst), 0,
	    RTF_CLONING | RTF_PRCLONING, IFSCOPE_NONE)) != NULL) {
		RT_LOCK(rt);
		if (rt->rt_ifp != fwd_ifp || !(rt->rt_flags & RTF_PROXY)) {
			rt->rt_flags |= RTF_CONDEMNED;
			RT_UNLOCK(rt);
			(void) rtrequest_locked(RTM_DELETE, rt_key(rt),
			    rt->rt_gateway, rt_mask(rt), rt->rt_flags, NULL);
			rtfree_locked(rt);
			rt = NULL;
		} else {
			nd6log2(debug, "%s: found prefix proxy route "
			    "for dst %s\n", if_name(rt->rt_ifp),
			    ip6_sprintf(dst6));
			RT_UNLOCK(rt);
			ro6->ro_rt = rt;                                     
			lck_mtx_unlock(rnh_lock);
			return;
		}
	}
	VERIFY(rt == NULL && ro6->ro_rt == NULL);

	  
                                                                     
    
	if (fwd_ifp != NULL && (rt = rtalloc1_scoped_locked(SA(&ro6->ro_dst), 1,
	    RTF_PRCLONING, fwd_ifp->if_index)) != NULL) {
		RT_LOCK(rt);
		if (!(rt->rt_flags & RTF_PROXY)) {
			RT_UNLOCK(rt);
			rtfree_locked(rt);
			rt = NULL;
		} else {
			nd6log2(debug, "%s: allocated prefix proxy "
			    "route for dst %s\n", if_name(rt->rt_ifp),
			    ip6_sprintf(dst6));
			RT_UNLOCK(rt);
			ro6->ro_rt = rt;                                     
		}
	}
	VERIFY(rt != NULL || ro6->ro_rt == NULL);

	if (fwd_ifp == NULL || rt == NULL) {
		nd6log2(error, "%s: failed to find forwarding prefix "
		    "proxy entry for dst %s\n", if_name(ifp),
		    ip6_sprintf(dst6));
	}
	lck_mtx_unlock(rnh_lock);
}

  
                                                                          
                                                                          
                         
   
void
nd6_prproxy_prelist_update(struct nd_prefix *pr_cur, struct nd_prefix *pr_up)
{
	SLIST_HEAD(, nd6_prproxy_prelist) up_head;
	SLIST_HEAD(, nd6_prproxy_prelist) down_head;
	struct nd6_prproxy_prelist *up, *down;
	struct nd_prefix *pr;
	struct in6_addr pr_addr;
	boolean_t enable;
	u_char pr_len;

	SLIST_INIT(&up_head);
	SLIST_INIT(&down_head);
	VERIFY(pr_cur != NULL);

	LCK_MTX_ASSERT(&proxy6_lock, LCK_MTX_ASSERT_OWNED);

	  
                                                                   
                                                                   
                                                               
    
	lck_mtx_lock(nd6_mutex);
	if (pr_up == NULL) {
		NDPR_LOCK(pr_cur);
		bcopy(&pr_cur->ndpr_prefix.sin6_addr, &pr_addr,
		    sizeof(pr_addr));
		pr_len = pr_cur->ndpr_plen;
		NDPR_UNLOCK(pr_cur);

		for (pr = nd_prefix.lh_first; pr; pr = pr->ndpr_next) {
			NDPR_LOCK(pr);
			if (!(pr->ndpr_stateflags & NDPRF_ONLINK) ||
			    !(pr->ndpr_stateflags & NDPRF_PRPROXY) ||
			    pr->ndpr_plen != pr_len ||
			    !in6_are_prefix_equal(&pr->ndpr_prefix.sin6_addr,
			    &pr_addr, pr_len)) {
				NDPR_UNLOCK(pr);
				continue;
			}
			NDPR_UNLOCK(pr);
			break;
		}

		if ((pr_up = pr) == NULL) {
			lck_mtx_unlock(nd6_mutex);
			goto done;
		}
		NDPR_LOCK(pr_up);
	} else {
		NDPR_LOCK(pr_up);
		bcopy(&pr_up->ndpr_prefix.sin6_addr, &pr_addr,
		    sizeof(pr_addr));
		pr_len = pr_up->ndpr_plen;
	}
	NDPR_LOCK_ASSERT_HELD(pr_up);
	  
                                                                
                                                                 
                                   
    
	VERIFY(!(pr_up->ndpr_stateflags & NDPRF_IFSCOPE));
	enable = (pr_up->ndpr_stateflags & NDPRF_PRPROXY);
	NDPR_UNLOCK(pr_up);

	up = nd6_ndprl_alloc(Z_WAITOK);
	if (up == NULL) {
		lck_mtx_unlock(nd6_mutex);
		goto done;
	}

	NDPR_ADDREF(pr_up);
	up->ndprl_pr = pr_up;
	SLIST_INSERT_HEAD(&up_head, up, ndprl_le);

	  
                                                                      
                                                                        
                                                                      
    
	for (pr = nd_prefix.lh_first; pr; pr = pr->ndpr_next) {
		NDPR_LOCK(pr);
		if (!(pr->ndpr_stateflags & NDPRF_ONLINK) ||
		    !(pr->ndpr_stateflags & NDPRF_IFSCOPE) ||
		    pr->ndpr_plen != pr_len ||
		    !in6_are_prefix_equal(&pr->ndpr_prefix.sin6_addr,
		    &pr_addr, pr_len)) {
			NDPR_UNLOCK(pr);
			continue;
		}
		NDPR_UNLOCK(pr);

		down = nd6_ndprl_alloc(Z_WAITOK);
		if (down == NULL) {
			continue;
		}

		NDPR_ADDREF(pr);
		down->ndprl_pr = pr;
		NDPR_ADDREF(pr_up);
		down->ndprl_up = pr_up;
		SLIST_INSERT_HEAD(&down_head, down, ndprl_le);
	}
	lck_mtx_unlock(nd6_mutex);

	  
                                                                   
    
	nd6_prproxy_prelist_setroute(enable,
	    (struct nd6_prproxy_prelist_head *)&up_head,
	    (struct nd6_prproxy_prelist_head *)&down_head);

done:
	VERIFY(SLIST_EMPTY(&up_head));
	VERIFY(SLIST_EMPTY(&down_head));
}

  
                                                                   
                                  
   
boolean_t
nd6_prproxy_ifaddr(struct in6_ifaddr *ia)
{
	struct nd_prefix *pr;
	struct in6_addr addr, pr_mask;
	u_int32_t pr_len;
	boolean_t proxied = FALSE;

	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_NOTOWNED);

	IFA_LOCK(&ia->ia_ifa);
	bcopy(&ia->ia_addr.sin6_addr, &addr, sizeof(addr));
	bcopy(&ia->ia_prefixmask.sin6_addr, &pr_mask, sizeof(pr_mask));
	pr_len = ia->ia_plen;
	IFA_UNLOCK(&ia->ia_ifa);

	lck_mtx_lock(nd6_mutex);
	for (pr = nd_prefix.lh_first; pr; pr = pr->ndpr_next) {
		NDPR_LOCK(pr);
		if ((pr->ndpr_stateflags & NDPRF_ONLINK) &&
		    (pr->ndpr_stateflags & NDPRF_PRPROXY) &&
		    in6_are_prefix_equal(&pr->ndpr_prefix.sin6_addr,
		    &addr, pr_len)) {
			NDPR_UNLOCK(pr);
			proxied = TRUE;
			break;
		}
		NDPR_UNLOCK(pr);
	}
	lck_mtx_unlock(nd6_mutex);

	return proxied;
}

  
                                                   
  
                                                                       
                                                                         
                                                                           
                                                                          
                          
   
void
nd6_prproxy_ns_output(struct ifnet *ifp, struct ifnet *exclifp,
    struct in6_addr *daddr, struct in6_addr *taddr, struct llinfo_nd6 *ln)
{
	SLIST_HEAD(, nd6_prproxy_prelist) ndprl_head;
	struct nd6_prproxy_prelist *ndprl, *ndprl_tmp;
	struct nd_prefix *pr, *fwd;
	struct ifnet *fwd_ifp;
	struct in6_addr pr_addr;
	u_char pr_len;

	  
                                                               
                                                             
    
	if (exclifp != NULL && exclifp == ifp) {
		exclifp = NULL;
	}

	if (exclifp == NULL) {
		nd6log2(debug, "%s: sending NS who has %s on ALL\n",
		    if_name(ifp), ip6_sprintf(taddr));
	} else {
		nd6log2(debug, "%s: sending NS who has %s on ALL "
		    "(except %s)\n", if_name(ifp),
		    ip6_sprintf(taddr), if_name(exclifp));
	}

	SLIST_INIT(&ndprl_head);

	lck_mtx_lock(nd6_mutex);

	for (pr = nd_prefix.lh_first; pr; pr = pr->ndpr_next) {
		NDPR_LOCK(pr);
		if (!(pr->ndpr_stateflags & NDPRF_ONLINK) ||
		    !(pr->ndpr_stateflags & NDPRF_PRPROXY) ||
		    !IN6_ARE_MASKED_ADDR_EQUAL(&pr->ndpr_prefix.sin6_addr,
		    taddr, &pr->ndpr_mask)) {
			NDPR_UNLOCK(pr);
			continue;
		}

		VERIFY(!(pr->ndpr_stateflags & NDPRF_IFSCOPE));
		bcopy(&pr->ndpr_prefix.sin6_addr, &pr_addr, sizeof(pr_addr));
		pr_len = pr->ndpr_plen;
		NDPR_UNLOCK(pr);

		for (fwd = nd_prefix.lh_first; fwd; fwd = fwd->ndpr_next) {
			NDPR_LOCK(fwd);
			if (!(fwd->ndpr_stateflags & NDPRF_ONLINK) ||
			    fwd->ndpr_ifp == ifp || fwd->ndpr_ifp == exclifp ||
			    fwd->ndpr_plen != pr_len ||
			    !in6_are_prefix_equal(&fwd->ndpr_prefix.sin6_addr,
			    &pr_addr, pr_len)) {
				NDPR_UNLOCK(fwd);
				continue;
			}

			fwd_ifp = fwd->ndpr_ifp;
			NDPR_UNLOCK(fwd);

			ndprl = nd6_ndprl_alloc(Z_WAITOK);
			if (ndprl == NULL) {
				continue;
			}

			NDPR_ADDREF(fwd);
			ndprl->ndprl_pr = fwd;
			ndprl->ndprl_fwd_ifp = fwd_ifp;

			SLIST_INSERT_HEAD(&ndprl_head, ndprl, ndprl_le);
		}
		break;
	}

	lck_mtx_unlock(nd6_mutex);

	SLIST_FOREACH_SAFE(ndprl, &ndprl_head, ndprl_le, ndprl_tmp) {
		SLIST_REMOVE(&ndprl_head, ndprl, nd6_prproxy_prelist, ndprl_le);

		pr = ndprl->ndprl_pr;
		fwd_ifp = ndprl->ndprl_fwd_ifp;

		if ((fwd_ifp->if_eflags & IFEF_IPV6_ND6ALT) != 0) {
			NDPR_REMREF(pr);
			nd6_ndprl_free(ndprl);
			continue;
		}

		NDPR_LOCK(pr);
		if (pr->ndpr_stateflags & NDPRF_ONLINK) {
			NDPR_UNLOCK(pr);
			nd6log2(debug,
			    "%s: Sending cloned NS who has %s, originally "
			    "on %s\n", if_name(fwd_ifp),
			    ip6_sprintf(taddr), if_name(ifp));

			nd6_ns_output(fwd_ifp, daddr, taddr, NULL, NULL);
		} else {
			NDPR_UNLOCK(pr);
		}
		NDPR_REMREF(pr);

		nd6_ndprl_free(ndprl);
	}
	VERIFY(SLIST_EMPTY(&ndprl_head));

	nd6_ns_output(ifp, daddr, taddr, ln, NULL);
}

  
                                                  
  
                                                                       
                                                                         
                                                                           
                                                           
   
void
nd6_prproxy_ns_input(struct ifnet *ifp, struct in6_addr *saddr,
    char *lladdr, int lladdrlen, struct in6_addr *daddr,
    struct in6_addr *taddr, uint8_t *nonce)
{
	SLIST_HEAD(, nd6_prproxy_prelist) ndprl_head;
	struct nd6_prproxy_prelist *ndprl, *ndprl_tmp;
	struct nd_prefix *pr, *fwd;
	struct ifnet *fwd_ifp;
	struct in6_addr pr_addr;
	u_char pr_len;
	boolean_t solrec = FALSE;

	SLIST_INIT(&ndprl_head);

	lck_mtx_lock(nd6_mutex);

	for (pr = nd_prefix.lh_first; pr; pr = pr->ndpr_next) {
		NDPR_LOCK(pr);
		if (!(pr->ndpr_stateflags & NDPRF_ONLINK) ||
		    !(pr->ndpr_stateflags & NDPRF_PRPROXY) ||
		    !IN6_ARE_MASKED_ADDR_EQUAL(&pr->ndpr_prefix.sin6_addr,
		    taddr, &pr->ndpr_mask)) {
			NDPR_UNLOCK(pr);
			continue;
		}

		VERIFY(!(pr->ndpr_stateflags & NDPRF_IFSCOPE));
		bcopy(&pr->ndpr_prefix.sin6_addr, &pr_addr, sizeof(pr_addr));
		pr_len = pr->ndpr_plen;

		  
                                                          
                                                               
                                            
     
		if ((solrec = !IN6_IS_ADDR_UNSPECIFIED(saddr)) &&
		    !nd6_solsrc_enq(pr, ifp, saddr, taddr)) {
			NDPR_UNLOCK(pr);
			solrec = FALSE;
			break;                                
		} else {
			NDPR_UNLOCK(pr);
		}

		for (fwd = nd_prefix.lh_first; fwd; fwd = fwd->ndpr_next) {
			NDPR_LOCK(fwd);
			if (!(fwd->ndpr_stateflags & NDPRF_ONLINK) ||
			    fwd->ndpr_ifp == ifp ||
			    fwd->ndpr_plen != pr_len ||
			    !in6_are_prefix_equal(&fwd->ndpr_prefix.sin6_addr,
			    &pr_addr, pr_len)) {
				NDPR_UNLOCK(fwd);
				continue;
			}

			fwd_ifp = fwd->ndpr_ifp;
			NDPR_UNLOCK(fwd);

			ndprl = nd6_ndprl_alloc(Z_WAITOK);
			if (ndprl == NULL) {
				continue;
			}

			NDPR_ADDREF(fwd);
			ndprl->ndprl_pr = fwd;
			ndprl->ndprl_fwd_ifp = fwd_ifp;
			ndprl->ndprl_sol = solrec;

			SLIST_INSERT_HEAD(&ndprl_head, ndprl, ndprl_le);
		}
		break;
	}

	lck_mtx_unlock(nd6_mutex);

	  
                                                              
                                                               
                                                                
                                                                 
                                                               
                                                                 
                                        
    
	if (solrec) {
		VERIFY(!IN6_IS_ADDR_UNSPECIFIED(saddr));
		nd6_cache_lladdr(ifp, saddr, lladdr, lladdrlen,
		    ND_NEIGHBOR_SOLICIT, 0);
	}

	SLIST_FOREACH_SAFE(ndprl, &ndprl_head, ndprl_le, ndprl_tmp) {
		SLIST_REMOVE(&ndprl_head, ndprl, nd6_prproxy_prelist, ndprl_le);

		pr = ndprl->ndprl_pr;
		fwd_ifp = ndprl->ndprl_fwd_ifp;

		if ((fwd_ifp->if_eflags & IFEF_IPV6_ND6ALT) != 0) {
			NDPR_REMREF(pr);
			nd6_ndprl_free(ndprl);
			continue;
		}

		NDPR_LOCK(pr);
		if (pr->ndpr_stateflags & NDPRF_ONLINK) {
			NDPR_UNLOCK(pr);
			nd6log2(debug,
			    "%s: Forwarding NS (%s) from %s to %s who "
			    "has %s, originally on %s\n", if_name(fwd_ifp),
			    ndprl->ndprl_sol ? "NUD/AR" :
			    "DAD", ip6_sprintf(saddr), ip6_sprintf(daddr),
			    ip6_sprintf(taddr), if_name(ifp));

			nd6_ns_output(fwd_ifp, ndprl->ndprl_sol ? taddr : NULL,
			    taddr, NULL, nonce);
		} else {
			NDPR_UNLOCK(pr);
		}
		NDPR_REMREF(pr);

		nd6_ndprl_free(ndprl);
	}
	VERIFY(SLIST_EMPTY(&ndprl_head));
}

  
                                                  
  
                                                                       
                                                                              
                                                                          
                                                                 
   
void
nd6_prproxy_na_input(struct ifnet *ifp, struct in6_addr *saddr,
    struct in6_addr *daddr0, struct in6_addr *taddr, int flags)
{
	SLIST_HEAD(, nd6_prproxy_prelist) ndprl_head;
	struct nd6_prproxy_prelist *ndprl, *ndprl_tmp;
	struct nd_prefix *pr;
	struct ifnet *fwd_ifp;
	struct in6_addr daddr;

	SLIST_INIT(&ndprl_head);


	lck_mtx_lock(nd6_mutex);

	for (pr = nd_prefix.lh_first; pr; pr = pr->ndpr_next) {
		NDPR_LOCK(pr);
		if (!(pr->ndpr_stateflags & NDPRF_ONLINK) ||
		    !(pr->ndpr_stateflags & NDPRF_PRPROXY) ||
		    !IN6_ARE_MASKED_ADDR_EQUAL(&pr->ndpr_prefix.sin6_addr,
		    taddr, &pr->ndpr_mask)) {
			NDPR_UNLOCK(pr);
			continue;
		}

		VERIFY(!(pr->ndpr_stateflags & NDPRF_IFSCOPE));
		  
                                                              
                                                            
                                                             
                                                                 
     
		if (!IN6_IS_ADDR_MULTICAST(daddr0)) {
			fwd_ifp = NULL;
			bzero(&daddr, sizeof(daddr));
			if (!nd6_solsrc_deq(pr, taddr, &daddr, &fwd_ifp)) {
				NDPR_UNLOCK(pr);
				break;                        
			}
			VERIFY(!IN6_IS_ADDR_UNSPECIFIED(&daddr) && fwd_ifp);
			NDPR_UNLOCK(pr);

			ndprl = nd6_ndprl_alloc(Z_WAITOK);
			if (ndprl == NULL) {
				break;                        
			}
			ndprl->ndprl_fwd_ifp = fwd_ifp;
			ndprl->ndprl_sol = TRUE;
			ndprl->ndprl_sol_saddr = *(&daddr);

			SLIST_INSERT_HEAD(&ndprl_head, ndprl, ndprl_le);
		} else {
			struct nd_prefix *fwd;
			struct in6_addr pr_addr;
			u_char pr_len;

			bcopy(&pr->ndpr_prefix.sin6_addr, &pr_addr,
			    sizeof(pr_addr));
			pr_len = pr->ndpr_plen;
			NDPR_UNLOCK(pr);

			for (fwd = nd_prefix.lh_first; fwd;
			    fwd = fwd->ndpr_next) {
				NDPR_LOCK(fwd);
				if (!(fwd->ndpr_stateflags & NDPRF_ONLINK) ||
				    fwd->ndpr_ifp == ifp ||
				    fwd->ndpr_plen != pr_len ||
				    !in6_are_prefix_equal(
					    &fwd->ndpr_prefix.sin6_addr,
					    &pr_addr, pr_len)) {
					NDPR_UNLOCK(fwd);
					continue;
				}

				fwd_ifp = fwd->ndpr_ifp;
				NDPR_UNLOCK(fwd);

				ndprl = nd6_ndprl_alloc(Z_WAITOK);
				if (ndprl == NULL) {
					continue;
				}

				NDPR_ADDREF(fwd);
				ndprl->ndprl_pr = fwd;
				ndprl->ndprl_fwd_ifp = fwd_ifp;

				SLIST_INSERT_HEAD(&ndprl_head, ndprl, ndprl_le);
			}
		}
		break;
	}

	lck_mtx_unlock(nd6_mutex);

	SLIST_FOREACH_SAFE(ndprl, &ndprl_head, ndprl_le, ndprl_tmp) {
		boolean_t send_na;

		SLIST_REMOVE(&ndprl_head, ndprl, nd6_prproxy_prelist, ndprl_le);

		pr = ndprl->ndprl_pr;
		fwd_ifp = ndprl->ndprl_fwd_ifp;

		if (ndprl->ndprl_sol) {
			VERIFY(pr == NULL);
			daddr = *(&ndprl->ndprl_sol_saddr);
			VERIFY(!IN6_IS_ADDR_UNSPECIFIED(&daddr));
			send_na = (in6_setscope(&daddr, fwd_ifp, NULL) == 0);
		} else {
			VERIFY(pr != NULL);
			daddr = *daddr0;
			NDPR_LOCK(pr);
			send_na = ((pr->ndpr_stateflags & NDPRF_ONLINK) &&
			    in6_setscope(&daddr, fwd_ifp, NULL) == 0);
			NDPR_UNLOCK(pr);
		}

		if (send_na) {
			if (!ndprl->ndprl_sol) {
				nd6log2(debug,
				    "%s: Forwarding NA (DAD) from %s to %s "
				    "tgt is %s, originally on %s\n",
				    if_name(fwd_ifp),
				    ip6_sprintf(saddr), ip6_sprintf(&daddr),
				    ip6_sprintf(taddr), if_name(ifp));
			} else {
				nd6log2(debug,
				    "%s: Forwarding NA (NUD/AR) from %s to "
				    "%s (was %s) tgt is %s, originally on "
				    "%s\n", if_name(fwd_ifp),
				    ip6_sprintf(saddr),
				    ip6_sprintf(&daddr), ip6_sprintf(daddr0),
				    ip6_sprintf(taddr), if_name(ifp));
			}

			nd6_na_output(fwd_ifp, &daddr, taddr, flags, 1, NULL);
		}

		if (pr != NULL) {
			NDPR_REMREF(pr);
		}

		nd6_ndprl_free(ndprl);
	}
	VERIFY(SLIST_EMPTY(&ndprl_head));
}

static struct nd6_prproxy_solsrc *
nd6_solsrc_alloc(int how)
{
	return zalloc_flags(solsrc_zone, how | Z_ZERO);
}

static void
nd6_solsrc_free(struct nd6_prproxy_solsrc *ssrc)
{
	zfree(solsrc_zone, ssrc);
}

static void
nd6_prproxy_sols_purge(struct nd_prefix *pr, u_int64_t max_stgt)
{
	struct nd6_prproxy_soltgt *soltgt, *tmp;
	u_int64_t expire = (max_stgt > 0) ? net_uptime() : 0;

	NDPR_LOCK_ASSERT_HELD(pr);

	                                                            
	RB_FOREACH_SAFE(soltgt, prproxy_sols_tree,
	    &pr->ndpr_prproxy_sols, tmp) {
		VERIFY(pr->ndpr_prproxy_sols_cnt > 0);
		if (expire == 0 || soltgt->soltgt_expire <= expire ||
		    soltgt->soltgt_cnt == 0) {
			pr->ndpr_prproxy_sols_cnt--;
			RB_REMOVE(prproxy_sols_tree,
			    &pr->ndpr_prproxy_sols, soltgt);
			nd6_soltgt_free(soltgt);
		}
	}

	if (max_stgt == 0 || pr->ndpr_prproxy_sols_cnt < max_stgt) {
		VERIFY(max_stgt != 0 || (pr->ndpr_prproxy_sols_cnt == 0 &&
		    RB_EMPTY(&pr->ndpr_prproxy_sols)));
		return;
	}

	                                                                     
	RB_FOREACH_SAFE(soltgt, prproxy_sols_tree,
	    &pr->ndpr_prproxy_sols, tmp) {
		VERIFY(pr->ndpr_prproxy_sols_cnt > 0);
		pr->ndpr_prproxy_sols_cnt--;
		RB_REMOVE(prproxy_sols_tree, &pr->ndpr_prproxy_sols, soltgt);
		nd6_soltgt_free(soltgt);
		if (pr->ndpr_prproxy_sols_cnt < max_stgt) {
			break;
		}
	}
}

  
                                                     
                                                 
   
void
nd6_prproxy_sols_reap(struct nd_prefix *pr)
{
	nd6_prproxy_sols_purge(pr, 0);
}

  
                                                                 
                                                 
   
void
nd6_prproxy_sols_prune(struct nd_prefix *pr, u_int32_t max_stgt)
{
	nd6_prproxy_sols_purge(pr, max_stgt);
}

  
                                                                 
   
static boolean_t
nd6_solsrc_enq(struct nd_prefix *pr, struct ifnet *ifp,
    struct in6_addr *saddr, struct in6_addr *taddr)
{
	struct nd6_prproxy_soltgt find, *soltgt;
	struct nd6_prproxy_solsrc *ssrc;
	u_int32_t max_stgt = nd6_max_tgt_sols;
	u_int32_t max_ssrc = nd6_max_src_sols;

	NDPR_LOCK_ASSERT_HELD(pr);
	VERIFY(!(pr->ndpr_stateflags & NDPRF_IFSCOPE));
	VERIFY((pr->ndpr_stateflags & (NDPRF_ONLINK | NDPRF_PRPROXY)) ==
	    (NDPRF_ONLINK | NDPRF_PRPROXY));
	VERIFY(!IN6_IS_ADDR_UNSPECIFIED(saddr));

	ssrc = nd6_solsrc_alloc(M_WAITOK);
	if (ssrc == NULL) {
		return FALSE;
	}

	ssrc->solsrc_saddr = *saddr;
	ssrc->solsrc_ifp = ifp;

	find.soltgt_key.taddr = *taddr;                         

	soltgt = RB_FIND(prproxy_sols_tree, &pr->ndpr_prproxy_sols, &find);
	if (soltgt == NULL) {
		if (max_stgt != 0 && pr->ndpr_prproxy_sols_cnt >= max_stgt) {
			VERIFY(!RB_EMPTY(&pr->ndpr_prproxy_sols));
			nd6_prproxy_sols_prune(pr, max_stgt);
			VERIFY(pr->ndpr_prproxy_sols_cnt < max_stgt);
		}

		soltgt = nd6_soltgt_alloc(M_WAITOK);
		if (soltgt == NULL) {
			nd6_solsrc_free(ssrc);
			return FALSE;
		}

		soltgt->soltgt_key.taddr = *taddr;
		VERIFY(soltgt->soltgt_cnt == 0);
		VERIFY(TAILQ_EMPTY(&soltgt->soltgt_q));

		pr->ndpr_prproxy_sols_cnt++;
		VERIFY(pr->ndpr_prproxy_sols_cnt != 0);
		RB_INSERT(prproxy_sols_tree, &pr->ndpr_prproxy_sols, soltgt);
	}

	if (max_ssrc != 0 && soltgt->soltgt_cnt >= max_ssrc) {
		VERIFY(!TAILQ_EMPTY(&soltgt->soltgt_q));
		nd6_soltgt_prune(soltgt, max_ssrc);
		VERIFY(soltgt->soltgt_cnt < max_ssrc);
	}

	soltgt->soltgt_cnt++;
	VERIFY(soltgt->soltgt_cnt != 0);
	TAILQ_INSERT_TAIL(&soltgt->soltgt_q, ssrc, solsrc_tqe);
	if (soltgt->soltgt_cnt == 1) {
		soltgt->soltgt_expire = net_uptime() + ND6_TGT_SOLS_EXPIRE;
	}

	return TRUE;
}

  
                                                                  
   
static boolean_t
nd6_solsrc_deq(struct nd_prefix *pr, struct in6_addr *taddr,
    struct in6_addr *daddr, struct ifnet **ifp)
{
	struct nd6_prproxy_soltgt find, *soltgt;
	struct nd6_prproxy_solsrc *ssrc;

	NDPR_LOCK_ASSERT_HELD(pr);
	VERIFY(!(pr->ndpr_stateflags & NDPRF_IFSCOPE));
	VERIFY((pr->ndpr_stateflags & (NDPRF_ONLINK | NDPRF_PRPROXY)) ==
	    (NDPRF_ONLINK | NDPRF_PRPROXY));

	bzero(daddr, sizeof(*daddr));
	*ifp = NULL;

	find.soltgt_key.taddr = *taddr;                         

	soltgt = RB_FIND(prproxy_sols_tree, &pr->ndpr_prproxy_sols, &find);
	if (soltgt == NULL || soltgt->soltgt_cnt == 0) {
		VERIFY(soltgt == NULL || TAILQ_EMPTY(&soltgt->soltgt_q));
		return FALSE;
	}

	VERIFY(soltgt->soltgt_cnt != 0);
	--soltgt->soltgt_cnt;
	ssrc = TAILQ_FIRST(&soltgt->soltgt_q);
	VERIFY(ssrc != NULL);
	TAILQ_REMOVE(&soltgt->soltgt_q, ssrc, solsrc_tqe);
	*daddr = *(&ssrc->solsrc_saddr);
	*ifp = ssrc->solsrc_ifp;
	nd6_solsrc_free(ssrc);

	return TRUE;
}

static struct nd6_prproxy_soltgt *
nd6_soltgt_alloc(int how)
{
	struct nd6_prproxy_soltgt *soltgt;

	soltgt = zalloc_flags(soltgt_zone, how | Z_ZERO);
	if (soltgt != NULL) {
		TAILQ_INIT(&soltgt->soltgt_q);
	}
	return soltgt;
}

static void
nd6_soltgt_free(struct nd6_prproxy_soltgt *soltgt)
{
	struct nd6_prproxy_solsrc *ssrc, *tssrc;

	TAILQ_FOREACH_SAFE(ssrc, &soltgt->soltgt_q, solsrc_tqe, tssrc) {
		VERIFY(soltgt->soltgt_cnt > 0);
		soltgt->soltgt_cnt--;
		TAILQ_REMOVE(&soltgt->soltgt_q, ssrc, solsrc_tqe);
		nd6_solsrc_free(ssrc);
	}

	VERIFY(soltgt->soltgt_cnt == 0);
	VERIFY(TAILQ_EMPTY(&soltgt->soltgt_q));

	zfree(soltgt_zone, soltgt);
}

static void
nd6_soltgt_prune(struct nd6_prproxy_soltgt *soltgt, u_int32_t max_ssrc)
{
	while (soltgt->soltgt_cnt >= max_ssrc) {
		struct nd6_prproxy_solsrc *ssrc;

		VERIFY(soltgt->soltgt_cnt != 0);
		--soltgt->soltgt_cnt;
		ssrc = TAILQ_FIRST(&soltgt->soltgt_q);
		VERIFY(ssrc != NULL);
		TAILQ_REMOVE(&soltgt->soltgt_q, ssrc, solsrc_tqe);
		nd6_solsrc_free(ssrc);
	}
}

  
                                             
  
                                                                        
                                                                      
   
static __inline int
soltgt_cmp(const struct nd6_prproxy_soltgt *a,
    const struct nd6_prproxy_soltgt *b)
{
	return memcmp(&a->soltgt_key, &b->soltgt_key, sizeof(a->soltgt_key));
}
