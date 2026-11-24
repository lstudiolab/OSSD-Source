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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

  
                                 
  
                                                                           
                                                                            
                                                                            
                                                                             
                                                                           
                                                                           
                                                                              
                          
  
                                                                             
                                                                             
                                                                              
                                                                           
                                       
  
                                                                          
                                                                              
                                                                             
                                                                              
                                                                               
                                                                              
                                                                              
                                                                             
                                                                        
  
                                                                         
                                                                          
                                                                          
                                                                          
                                                                              
                                                                 
  
                                                                              
                                                                           
                            
  
                                                                             
                                           
  
                                                                           
                                                                         
                                                                        
                                         
  
                                                                          
                                                                          
                                                                        
                                                                       
                                                                       
                                                                      
                                                                         
                                                                     
                                         
  
                                                                             
                                                                            
                                                                           
                                                                           
                                                                            
                                                                               
  
               
  
                                                           
  
                                                                           
                                                                   
                          
  
                                                                       
                                                                  
                                                                 
               
  
                                                                              
                                                                              
                                              
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/tree.h>
#include <sys/sysctl.h>
#include <sys/mcache.h>
#include <sys/protosw.h>

#include <dev/random/randomdev.h>

#include <net/if_dl.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_llreach.h>
#include <net/dlil.h>
#include <net/kpi_interface.h>
#include <net/route.h>

#include <kern/assert.h>
#include <kern/locks.h>
#include <kern/zalloc.h>

#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

static ZONE_DECLARE(iflr_zone, "if_llreach", sizeof(struct if_llreach),
    ZC_ZFREE_CLEARMEM);

static struct if_llreach *iflr_alloc(zalloc_flags_t);
static void iflr_free(struct if_llreach *);
static __inline int iflr_cmp(const struct if_llreach *,
    const struct if_llreach *);
static __inline int iflr_reachable(struct if_llreach *, int, u_int64_t);
static int sysctl_llreach_ifinfo SYSCTL_HANDLER_ARGS;

                                                   
RB_GENERATE_PREV(ll_reach_tree, if_llreach, lr_link, iflr_cmp);

SYSCTL_DECL(_net_link_generic_system);

SYSCTL_NODE(_net_link_generic_system, OID_AUTO, llreach_info,
    CTLFLAG_RD | CTLFLAG_LOCKED, sysctl_llreach_ifinfo,
    "Per-interface tree of source link-layer reachability records");

  
                                                                  
   
#define LL_COMPUTE_RTIME(x)     ND_COMPUTE_RTIME(x)

void
ifnet_llreach_ifattach(struct ifnet *ifp, boolean_t reuse)
{
	lck_rw_lock_exclusive(&ifp->if_llreach_lock);
	                                                        
	if (!reuse) {
		RB_INIT(&ifp->if_ll_srcs);
	}
	lck_rw_done(&ifp->if_llreach_lock);
}

void
ifnet_llreach_ifdetach(struct ifnet *ifp)
{
#pragma unused(ifp)
	  
                                                           
                                                          
                                             
    
}

  
                                              
  
                                                                        
                                                                      
   
static __inline int
iflr_cmp(const struct if_llreach *a, const struct if_llreach *b)
{
	return memcmp(&a->lr_key, &b->lr_key, sizeof(a->lr_key));
}

static __inline int
iflr_reachable(struct if_llreach *lr, int cmp_delta, u_int64_t tval)
{
	u_int64_t now;
	u_int64_t expire;

	now = net_uptime();                                         
	  
                                                              
    
	expire = lr->lr_lastrcvd + lr->lr_reachable;
	  
                                                         
                                                      
                     
    
	if (!cmp_delta) {
		return expire >= now;
	}
	  
                                                         
                                                              
                                                             
            
    
	return (expire >= now) && (now - tval) < lr->lr_reachable;
}

int
ifnet_llreach_reachable(struct if_llreach *lr)
{
	  
                                                     
    
	return iflr_reachable(lr, 0, 0);
}

int
ifnet_llreach_reachable_delta(struct if_llreach *lr, u_int64_t tval)
{
	  
                                                     
    
	return iflr_reachable(lr, 1, tval);
}

void
ifnet_llreach_set_reachable(struct ifnet *ifp, u_int16_t llproto, void *addr,
    unsigned int alen)
{
	struct if_llreach find, *lr;

	VERIFY(alen == IF_LLREACH_MAXLEN);                   

	find.lr_key.proto = llproto;
	bcopy(addr, &find.lr_key.addr, IF_LLREACH_MAXLEN);

	lck_rw_lock_shared(&ifp->if_llreach_lock);
	lr = RB_FIND(ll_reach_tree, &ifp->if_ll_srcs, &find);
	if (lr == NULL) {
		lck_rw_done(&ifp->if_llreach_lock);
		return;
	}
	  
                                                                
    
	lr->lr_lastrcvd = net_uptime();
	lck_rw_done(&ifp->if_llreach_lock);
}

struct if_llreach *
ifnet_llreach_alloc(struct ifnet *ifp, u_int16_t llproto, void *addr,
    unsigned int alen, u_int32_t llreach_base)
{
	struct if_llreach find, *lr;
	struct timeval cnow;

	if (llreach_base == 0) {
		return NULL;
	}

	VERIFY(alen == IF_LLREACH_MAXLEN);                   

	find.lr_key.proto = llproto;
	bcopy(addr, &find.lr_key.addr, IF_LLREACH_MAXLEN);

	lck_rw_lock_shared(&ifp->if_llreach_lock);
	lr = RB_FIND(ll_reach_tree, &ifp->if_ll_srcs, &find);
	if (lr != NULL) {
found:
		IFLR_LOCK(lr);
		VERIFY(lr->lr_reqcnt >= 1);
		lr->lr_reqcnt++;
		VERIFY(lr->lr_reqcnt != 0);
		IFLR_ADDREF_LOCKED(lr);                         
		lr->lr_lastrcvd = net_uptime();                             
		IFLR_UNLOCK(lr);
		lck_rw_done(&ifp->if_llreach_lock);
		return lr;
	}

	if (!lck_rw_lock_shared_to_exclusive(&ifp->if_llreach_lock)) {
		lck_rw_lock_exclusive(&ifp->if_llreach_lock);
	}

	LCK_RW_ASSERT(&ifp->if_llreach_lock, LCK_RW_ASSERT_EXCLUSIVE);

	                                                       
	lr = RB_FIND(ll_reach_tree, &ifp->if_ll_srcs, &find);
	if (lr != NULL) {
		goto found;
	}

	lr = iflr_alloc(Z_WAITOK);

	IFLR_LOCK(lr);
	lr->lr_reqcnt++;
	VERIFY(lr->lr_reqcnt == 1);
	IFLR_ADDREF_LOCKED(lr);                                  
	IFLR_ADDREF_LOCKED(lr);                                 
	lr->lr_lastrcvd = net_uptime();                                     
	lr->lr_baseup = lr->lr_lastrcvd;                         
	getmicrotime(&cnow);
	lr->lr_basecal = cnow.tv_sec;                                   
	lr->lr_basereachable = llreach_base;
	lr->lr_reachable = LL_COMPUTE_RTIME(lr->lr_basereachable * 1000);
	lr->lr_debug |= IFD_ATTACHED;
	lr->lr_ifp = ifp;
	lr->lr_key.proto = llproto;
	bcopy(addr, &lr->lr_key.addr, IF_LLREACH_MAXLEN);
	lr->lr_rssi = IFNET_RSSI_UNKNOWN;
	lr->lr_lqm = IFNET_LQM_THRESH_UNKNOWN;
	lr->lr_npm = IFNET_NPM_THRESH_UNKNOWN;
	RB_INSERT(ll_reach_tree, &ifp->if_ll_srcs, lr);
	IFLR_UNLOCK(lr);
	lck_rw_done(&ifp->if_llreach_lock);

	return lr;
}

void
ifnet_llreach_free(struct if_llreach *lr)
{
	struct ifnet *ifp;

	                                                
	ifp = lr->lr_ifp;

	lck_rw_lock_exclusive(&ifp->if_llreach_lock);
	IFLR_LOCK(lr);
	if (lr->lr_reqcnt == 0) {
		panic("%s: lr=%p negative reqcnt", __func__, lr);
		                
	}
	--lr->lr_reqcnt;
	if (lr->lr_reqcnt > 0) {
		IFLR_UNLOCK(lr);
		lck_rw_done(&ifp->if_llreach_lock);
		IFLR_REMREF(lr);                                
		return;
	}
	if (!(lr->lr_debug & IFD_ATTACHED)) {
		panic("%s: Attempt to detach an unattached llreach lr=%p",
		    __func__, lr);
		                
	}
	lr->lr_debug &= ~IFD_ATTACHED;
	RB_REMOVE(ll_reach_tree, &ifp->if_ll_srcs, lr);
	IFLR_UNLOCK(lr);
	lck_rw_done(&ifp->if_llreach_lock);

	IFLR_REMREF(lr);                                         
	IFLR_REMREF(lr);                                        
}

u_int64_t
ifnet_llreach_up2calexp(struct if_llreach *lr, u_int64_t uptime)
{
	u_int64_t calendar = 0;

	if (uptime != 0) {
		struct timeval cnow;
		u_int64_t unow;

		getmicrotime(&cnow);                               
		unow = net_uptime();                                
		  
                                                      
                                                  
                                                        
     
		lr->lr_basecal += (cnow.tv_sec - lr->lr_basecal) -
		    (unow - lr->lr_baseup);

		calendar = lr->lr_basecal + lr->lr_reachable +
		    (uptime - lr->lr_baseup);
	}

	return calendar;
}

u_int64_t
ifnet_llreach_up2upexp(struct if_llreach *lr, u_int64_t uptime)
{
	return lr->lr_reachable + uptime;
}

int
ifnet_llreach_get_defrouter(struct ifnet *ifp, sa_family_t af,
    struct ifnet_llreach_info *iflri)
{
	struct radix_node_head *rnh;
	struct sockaddr_storage dst_ss, mask_ss;
	struct rtentry *rt;
	int error = ESRCH;

	VERIFY(ifp != NULL && iflri != NULL &&
	    (af == AF_INET || af == AF_INET6));

	bzero(iflri, sizeof(*iflri));

	if ((rnh = rt_tables[af]) == NULL) {
		return error;
	}

	bzero(&dst_ss, sizeof(dst_ss));
	bzero(&mask_ss, sizeof(mask_ss));
	dst_ss.ss_family = af;
	dst_ss.ss_len = (af == AF_INET) ? sizeof(struct sockaddr_in) :
	    sizeof(struct sockaddr_in6);

	lck_mtx_lock(rnh_lock);
	rt = rt_lookup(TRUE, SA(&dst_ss), SA(&mask_ss), rnh, ifp->if_index);
	if (rt != NULL) {
		struct rtentry *gwrt;

		RT_LOCK(rt);
		if ((rt->rt_flags & RTF_GATEWAY) &&
		    (gwrt = rt->rt_gwroute) != NULL &&
		    rt_key(rt)->sa_family == rt_key(gwrt)->sa_family &&
		    (gwrt->rt_flags & RTF_UP)) {
			RT_UNLOCK(rt);
			RT_LOCK(gwrt);
			if (gwrt->rt_llinfo_get_iflri != NULL) {
				(*gwrt->rt_llinfo_get_iflri)(gwrt, iflri);
				error = 0;
			}
			RT_UNLOCK(gwrt);
		} else {
			RT_UNLOCK(rt);
		}
		rtfree_locked(rt);
	}
	lck_mtx_unlock(rnh_lock);

	return error;
}

static struct if_llreach *
iflr_alloc(zalloc_flags_t how)
{
	struct if_llreach *lr = zalloc_flags(iflr_zone, how | Z_ZERO);

	if (lr) {
		lck_mtx_init(&lr->lr_lock, ifnet_lock_group, ifnet_lock_attr);
		lr->lr_debug |= IFD_ALLOC;
	}
	return lr;
}

static void
iflr_free(struct if_llreach *lr)
{
	IFLR_LOCK(lr);
	if (lr->lr_debug & IFD_ATTACHED) {
		panic("%s: attached lr=%p is being freed", __func__, lr);
		                
	} else if (!(lr->lr_debug & IFD_ALLOC)) {
		panic("%s: lr %p cannot be freed", __func__, lr);
		                
	} else if (lr->lr_refcnt != 0) {
		panic("%s: non-zero refcount lr=%p", __func__, lr);
		                
	} else if (lr->lr_reqcnt != 0) {
		panic("%s: non-zero reqcnt lr=%p", __func__, lr);
		                
	}
	lr->lr_debug &= ~IFD_ALLOC;
	IFLR_UNLOCK(lr);

	lck_mtx_destroy(&lr->lr_lock, ifnet_lock_group);
	zfree(iflr_zone, lr);
}

void
iflr_addref(struct if_llreach *lr, int locked)
{
	if (!locked) {
		IFLR_LOCK(lr);
	} else {
		IFLR_LOCK_ASSERT_HELD(lr);
	}

	if (++lr->lr_refcnt == 0) {
		panic("%s: lr=%p wraparound refcnt", __func__, lr);
		                
	}
	if (!locked) {
		IFLR_UNLOCK(lr);
	}
}

void
iflr_remref(struct if_llreach *lr)
{
	IFLR_LOCK(lr);
	if (lr->lr_refcnt == 0) {
		panic("%s: lr=%p negative refcnt", __func__, lr);
		                
	}
	--lr->lr_refcnt;
	if (lr->lr_refcnt > 0) {
		IFLR_UNLOCK(lr);
		return;
	}
	IFLR_UNLOCK(lr);

	iflr_free(lr);                     
}

void
ifnet_lr2ri(struct if_llreach *lr, struct rt_reach_info *ri)
{
	struct if_llreach_info lri;

	IFLR_LOCK_ASSERT_HELD(lr);

	bzero(ri, sizeof(*ri));
	ifnet_lr2lri(lr, &lri);
	ri->ri_refcnt = lri.lri_refcnt;
	ri->ri_probes = lri.lri_probes;
	ri->ri_rcv_expire = lri.lri_expire;
	ri->ri_rssi = lri.lri_rssi;
	ri->ri_lqm = lri.lri_lqm;
	ri->ri_npm = lri.lri_npm;
}

void
ifnet_lr2iflri(struct if_llreach *lr, struct ifnet_llreach_info *iflri)
{
	IFLR_LOCK_ASSERT_HELD(lr);

	bzero(iflri, sizeof(*iflri));
	  
                                                                
    
	iflri->iflri_refcnt = lr->lr_reqcnt;
	iflri->iflri_probes = lr->lr_probes;
	iflri->iflri_rcv_expire = ifnet_llreach_up2upexp(lr, lr->lr_lastrcvd);
	iflri->iflri_curtime = net_uptime();
	switch (lr->lr_key.proto) {
	case ETHERTYPE_IP:
		iflri->iflri_netproto = PF_INET;
		break;
	case ETHERTYPE_IPV6:
		iflri->iflri_netproto = PF_INET6;
		break;
	default:
		  
                                                   
                                                   
                          
     
		iflri->iflri_netproto = PF_UNSPEC;
		break;
	}
	bcopy(&lr->lr_key.addr, &iflri->iflri_addr, IF_LLREACH_MAXLEN);
	iflri->iflri_rssi = lr->lr_rssi;
	iflri->iflri_lqm = lr->lr_lqm;
	iflri->iflri_npm = lr->lr_npm;
}

void
ifnet_lr2lri(struct if_llreach *lr, struct if_llreach_info *lri)
{
	IFLR_LOCK_ASSERT_HELD(lr);

	bzero(lri, sizeof(*lri));
	  
                                                                
    
	lri->lri_refcnt = lr->lr_reqcnt;
	lri->lri_ifindex = lr->lr_ifp->if_index;
	lri->lri_probes = lr->lr_probes;
	lri->lri_expire = ifnet_llreach_up2calexp(lr, lr->lr_lastrcvd);
	lri->lri_proto = lr->lr_key.proto;
	bcopy(&lr->lr_key.addr, &lri->lri_addr, IF_LLREACH_MAXLEN);
	lri->lri_rssi = lr->lr_rssi;
	lri->lri_lqm = lr->lr_lqm;
	lri->lri_npm = lr->lr_npm;
}

static int
sysctl_llreach_ifinfo SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp)
	int             *name, retval = 0;
	unsigned int    namelen;
	uint32_t        ifindex;
	struct if_llreach *lr;
	struct if_llreach_info lri = {};
	struct ifnet    *ifp;

	name = (int *)arg1;
	namelen = (unsigned int)arg2;

	if (req->newptr != USER_ADDR_NULL) {
		return EPERM;
	}

	if (namelen != 1) {
		return EINVAL;
	}

	ifindex = name[0];
	ifnet_head_lock_shared();
	if (ifindex <= 0 || ifindex > (u_int)if_index) {
		printf("%s: ifindex %u out of range\n", __func__, ifindex);
		ifnet_head_done();
		return ENOENT;
	}

	ifp = ifindex2ifnet[ifindex];
	ifnet_head_done();
	if (ifp == NULL) {
		printf("%s: no ifp for ifindex %u\n", __func__, ifindex);
		return ENOENT;
	}

	lck_rw_lock_shared(&ifp->if_llreach_lock);
	RB_FOREACH(lr, ll_reach_tree, &ifp->if_ll_srcs) {
		                                         
		IFLR_LOCK(lr);
		ifnet_lr2lri(lr, &lri);
		IFLR_UNLOCK(lr);

		if ((retval = SYSCTL_OUT(req, &lri, sizeof(lri))) != 0) {
			break;
		}
	}
	lck_rw_done(&ifp->if_llreach_lock);

	return retval;
}
