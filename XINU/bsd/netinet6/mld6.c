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
/*-
 * Copyright (c) 2009 Bruce Simpson.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1988 Stephen Deering.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Stephen Deering of Stanford University.
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
 *	@(#)igmp.c	8.1 (Berkeley) 7/19/93
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mcache.h>

#include <dev/random/randomdev.h>

#include <kern/zalloc.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/scope6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/mld6.h>
#include <netinet6/mld6_var.h>

                                          
static lck_attr_t       *mld_mtx_attr;
static lck_grp_t        *mld_mtx_grp;
static lck_grp_attr_t   *mld_mtx_grp_attr;

  
                                  
  
                                                                     
                                                                              
                                                                      
                                                                              
                                                             
  
                                                                           
                                                                          
                                                                             
                                                                             
                                                                             
                                               
  
                                                                             
                                                                            
                                                                         
                                                                             
                                                                            
                                                                           
  
                                     
  
                                                   
  
                                                                        
                                         
   
static decl_lck_mtx_data(, mld_mtx);

SLIST_HEAD(mld_in6m_relhead, in6_multi);

static void     mli_initvar(struct mld_ifinfo *, struct ifnet *, int);
static struct mld_ifinfo *mli_alloc(zalloc_flags_t);
static void     mli_free(struct mld_ifinfo *);
static void     mli_delete(const struct ifnet *, struct mld_in6m_relhead *);
static void     mld_dispatch_packet(struct mbuf *);
static void     mld_final_leave(struct in6_multi *, struct mld_ifinfo *,
    struct mld_tparams *);
static int      mld_handle_state_change(struct in6_multi *, struct mld_ifinfo *,
    struct mld_tparams *);
static int      mld_initial_join(struct in6_multi *, struct mld_ifinfo *,
    struct mld_tparams *, const int);
#ifdef MLD_DEBUG
static const char *     mld_rec_type_to_str(const int);
#endif
static uint32_t mld_set_version(struct mld_ifinfo *, const int);
static void     mld_flush_relq(struct mld_ifinfo *, struct mld_in6m_relhead *);
static void     mld_dispatch_queue_locked(struct mld_ifinfo *, struct ifqueue *, int);
static int      mld_v1_input_query(struct ifnet *, const struct ip6_hdr *,
              struct mld_hdr *);
static int      mld_v1_input_report(struct ifnet *, struct mbuf *,
    const struct ip6_hdr *,           struct mld_hdr *);
static void     mld_v1_process_group_timer(struct in6_multi *, const int);
static void     mld_v1_process_querier_timers(struct mld_ifinfo *);
static int      mld_v1_transmit_report(struct in6_multi *, const uint8_t);
static uint32_t mld_v1_update_group(struct in6_multi *, const int);
static void     mld_v2_cancel_link_timers(struct mld_ifinfo *);
static uint32_t mld_v2_dispatch_general_query(struct mld_ifinfo *);
static struct mbuf *
mld_v2_encap_report(struct ifnet *, struct mbuf *);
static int      mld_v2_enqueue_filter_change(struct ifqueue *,
    struct in6_multi *);
static int      mld_v2_enqueue_group_record(struct ifqueue *,
    struct in6_multi *, const int, const int, const int,
    const int);
static int      mld_v2_input_query(struct ifnet *, const struct ip6_hdr *,
    struct mbuf *, const int, const int);
static int      mld_v2_merge_state_changes(struct in6_multi *,
    struct ifqueue *);
static void     mld_v2_process_group_timers(struct mld_ifinfo *,
    struct ifqueue *, struct ifqueue *,
    struct in6_multi *, const int);
static int      mld_v2_process_group_query(struct in6_multi *,
    int, struct mbuf *, const int);
static int      sysctl_mld_gsr SYSCTL_HANDLER_ARGS;
static int      sysctl_mld_ifinfo SYSCTL_HANDLER_ARGS;
static int      sysctl_mld_v2enable SYSCTL_HANDLER_ARGS;

static int mld_timeout_run;                                                
static void mld_timeout(void *);
static void mld_sched_timeout(void);

  
                                                      
   
static struct timeval mld_gsrdelay = {.tv_sec = 10, .tv_usec = 0};
static LIST_HEAD(, mld_ifinfo) mli_head;

static int querier_present_timers_running6;
static int interface_timers_running6;
static int state_change_timers_running6;
static int current_state_timers_running6;

static unsigned int mld_mli_list_genid;
  
                         
   
#define MLD_LOCK()                      \
	lck_mtx_lock(&mld_mtx)
#define MLD_LOCK_ASSERT_HELD()          \
	LCK_MTX_ASSERT(&mld_mtx, LCK_MTX_ASSERT_OWNED)
#define MLD_LOCK_ASSERT_NOTHELD()       \
	LCK_MTX_ASSERT(&mld_mtx, LCK_MTX_ASSERT_NOTOWNED)
#define MLD_UNLOCK()                    \
	lck_mtx_unlock(&mld_mtx)

#define MLD_ADD_DETACHED_IN6M(_head, _in6m) {                           \
	SLIST_INSERT_HEAD(_head, _in6m, in6m_dtle);                     \
}

#define MLD_REMOVE_DETACHED_IN6M(_head) {                               \
	struct in6_multi *_in6m, *_inm_tmp;                             \
	SLIST_FOREACH_SAFE(_in6m, _head, in6m_dtle, _inm_tmp) {         \
	        SLIST_REMOVE(_head, _in6m, in6_multi, in6m_dtle);       \
	        IN6M_REMREF(_in6m);                                     \
	}                                                               \
	VERIFY(SLIST_EMPTY(_head));                                     \
}

static ZONE_DECLARE(mli_zone, "mld_ifinfo",
    sizeof(struct mld_ifinfo), ZC_ZFREE_CLEARMEM);

SYSCTL_DECL(_net_inet6);                                             

SYSCTL_NODE(_net_inet6, OID_AUTO, mld, CTLFLAG_RW | CTLFLAG_LOCKED, 0,
    "IPv6 Multicast Listener Discovery");
SYSCTL_PROC(_net_inet6_mld, OID_AUTO, gsrdelay,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &mld_gsrdelay.tv_sec, 0, sysctl_mld_gsr, "I",
    "Rate limit for MLDv2 Group-and-Source queries in seconds");

SYSCTL_NODE(_net_inet6_mld, OID_AUTO, ifinfo, CTLFLAG_RD | CTLFLAG_LOCKED,
    sysctl_mld_ifinfo, "Per-interface MLDv2 state");

static int      mld_v1enable = 1;
SYSCTL_INT(_net_inet6_mld, OID_AUTO, v1enable, CTLFLAG_RW | CTLFLAG_LOCKED,
    &mld_v1enable, 0, "Enable fallback to MLDv1");

static int      mld_v2enable = 1;
SYSCTL_PROC(_net_inet6_mld, OID_AUTO, v2enable,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &mld_v2enable, 0, sysctl_mld_v2enable, "I",
    "Enable MLDv2 (debug purposes only)");

static int      mld_use_allow = 1;
SYSCTL_INT(_net_inet6_mld, OID_AUTO, use_allow, CTLFLAG_RW | CTLFLAG_LOCKED,
    &mld_use_allow, 0, "Use ALLOW/BLOCK for RFC 4604 SSM joins/leaves");

#ifdef MLD_DEBUG
int mld_debug = 0;
SYSCTL_INT(_net_inet6_mld, OID_AUTO,
    debug, CTLFLAG_RW | CTLFLAG_LOCKED, &mld_debug, 0, "");
#endif
  
                                                    
   
struct mld_raopt {
	struct ip6_hbh          hbh;
	struct ip6_opt          pad;
	struct ip6_opt_router   ra;
} __packed;

  
                                         
   
static struct mld_raopt mld_ra = {
	.hbh = { .ip6h_nxt = 0, .ip6h_len = 0 },
	.pad = { .ip6o_type = IP6OPT_PADN, .ip6o_len = 0 },
	.ra = {
		.ip6or_type = (u_int8_t)IP6OPT_ROUTER_ALERT,
		.ip6or_len = (u_int8_t)(IP6OPT_RTALERT_LEN - 2),
		.ip6or_value =  {((IP6OPT_RTALERT_MLD >> 8) & 0xFF),
			         (IP6OPT_RTALERT_MLD & 0xFF) }
	}
};
static struct ip6_pktopts mld_po;

                                                                  
#define vt_nrecs        pkt_mpriv.__mpriv_u.__mpriv32[0].__mpriv32_u.__val16[0]

static __inline void
mld_save_context(struct mbuf *m, struct ifnet *ifp)
{
	m->m_pkthdr.rcvif = ifp;
}

static __inline void
mld_scrub_context(struct mbuf *m)
{
	m->m_pkthdr.rcvif = NULL;
}

  
                                              
                    
   
static __inline struct ifnet *
mld_restore_context(struct mbuf *m)
{
	return m->m_pkthdr.rcvif;
}

  
                                                                     
   
static int
sysctl_mld_gsr SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error;
	int i;

	MLD_LOCK();

	i = (int)mld_gsrdelay.tv_sec;

	error = sysctl_handle_int(oidp, &i, 0, req);
	if (error || !req->newptr) {
		goto out_locked;
	}

	if (i < -1 || i >= 60) {
		error = EINVAL;
		goto out_locked;
	}

	mld_gsrdelay.tv_sec = i;

out_locked:
	MLD_UNLOCK();
	return error;
}
  
                                                          
                          
  
   
static int
sysctl_mld_ifinfo SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp)
	int                     *name;
	int                      error;
	u_int                    namelen;
	struct ifnet            *ifp;
	struct mld_ifinfo       *mli;
	struct mld_ifinfo_u     mli_u;

	name = (int *)arg1;
	namelen = arg2;

	if (req->newptr != USER_ADDR_NULL) {
		return EPERM;
	}

	if (namelen != 1) {
		return EINVAL;
	}

	MLD_LOCK();

	if (name[0] <= 0 || name[0] > (u_int)if_index) {
		error = ENOENT;
		goto out_locked;
	}

	error = ENOENT;

	ifnet_head_lock_shared();
	ifp = ifindex2ifnet[name[0]];
	ifnet_head_done();
	if (ifp == NULL) {
		goto out_locked;
	}

	bzero(&mli_u, sizeof(mli_u));

	LIST_FOREACH(mli, &mli_head, mli_link) {
		MLI_LOCK(mli);
		if (ifp != mli->mli_ifp) {
			MLI_UNLOCK(mli);
			continue;
		}

		mli_u.mli_ifindex = mli->mli_ifp->if_index;
		mli_u.mli_version = mli->mli_version;
		mli_u.mli_v1_timer = mli->mli_v1_timer;
		mli_u.mli_v2_timer = mli->mli_v2_timer;
		mli_u.mli_flags = mli->mli_flags;
		mli_u.mli_rv = mli->mli_rv;
		mli_u.mli_qi = mli->mli_qi;
		mli_u.mli_qri = mli->mli_qri;
		mli_u.mli_uri = mli->mli_uri;
		MLI_UNLOCK(mli);

		error = SYSCTL_OUT(req, &mli_u, sizeof(mli_u));
		break;
	}

out_locked:
	MLD_UNLOCK();
	return error;
}

static int
sysctl_mld_v2enable SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error;
	int i;
	struct mld_ifinfo *mli;
	struct mld_tparams mtp = { .qpt = 0, .it = 0, .cst = 0, .sct = 0 };

	MLD_LOCK();

	i = mld_v2enable;

	error = sysctl_handle_int(oidp, &i, 0, req);
	if (error || !req->newptr) {
		goto out_locked;
	}

	if (i < 0 || i > 1) {
		error = EINVAL;
		goto out_locked;
	}

	mld_v2enable = i;
	  
                                                                      
                                                                
                                                                        
    
	if (mld_v2enable == 1) {
		goto out_locked;
	}

	LIST_FOREACH(mli, &mli_head, mli_link) {
		MLI_LOCK(mli);
		if (mld_set_version(mli, MLD_VERSION_1) > 0) {
			mtp.qpt = 1;
		}
		MLI_UNLOCK(mli);
	}

out_locked:
	MLD_UNLOCK();

	mld_set_timeout(&mtp);

	return error;
}

  
                                                     
  
                                          
                                                               
                                                                
                               
                                                            
                                                                     
               
   
static void
mld_dispatch_queue_locked(struct mld_ifinfo *mli, struct ifqueue *ifq, int limit)
{
	struct mbuf *m;

	MLD_LOCK_ASSERT_HELD();

	if (mli != NULL) {
		MLI_LOCK_ASSERT_HELD(mli);
	}

	for (;;) {
		IF_DEQUEUE(ifq, m);
		if (m == NULL) {
			break;
		}
		MLD_PRINTF(("%s: dispatch 0x%llx from 0x%llx\n", __func__,
		    (uint64_t)VM_KERNEL_ADDRPERM(ifq),
		    (uint64_t)VM_KERNEL_ADDRPERM(m)));

		if (mli != NULL) {
			MLI_UNLOCK(mli);
		}
		MLD_UNLOCK();

		mld_dispatch_packet(m);

		MLD_LOCK();
		if (mli != NULL) {
			MLI_LOCK(mli);
		}

		if (--limit == 0) {
			break;
		}
	}

	if (mli != NULL) {
		MLI_LOCK_ASSERT_HELD(mli);
	}
}

  
                                             
  
                                                        
                                                                 
                                                                      
                              
                                                       
  
                                                              
                                                                 
   
static __inline__ int
mld_is_addr_reported(const struct in6_addr *addr)
{
	VERIFY(IN6_IS_ADDR_MULTICAST(addr));

	if (IPV6_ADDR_MC_SCOPE(addr) == IPV6_ADDR_SCOPE_NODELOCAL) {
		return 0;
	}

	if (IPV6_ADDR_MC_SCOPE(addr) == IPV6_ADDR_SCOPE_LINKLOCAL && !IN6_IS_ADDR_UNICAST_BASED_MULTICAST(addr)) {
		struct in6_addr tmp = *addr;
		in6_clearscope(&tmp);
		if (IN6_ARE_ADDR_EQUAL(&tmp, &in6addr_linklocal_allnodes)) {
			return 0;
		}
	}

	return 1;
}

  
                                                        
   
struct mld_ifinfo *
mld_domifattach(struct ifnet *ifp, zalloc_flags_t how)
{
	struct mld_ifinfo *mli;

	MLD_PRINTF(("%s: called for ifp 0x%llx(%s)\n", __func__,
	    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));

	mli = mli_alloc(how);
	if (mli == NULL) {
		return NULL;
	}

	MLD_LOCK();

	MLI_LOCK(mli);
	mli_initvar(mli, ifp, 0);
	mli->mli_debug |= IFD_ATTACHED;
	MLI_ADDREF_LOCKED(mli);                                    
	MLI_ADDREF_LOCKED(mli);                                  
	MLI_UNLOCK(mli);
	ifnet_lock_shared(ifp);
	mld6_initsilent(ifp, mli);
	ifnet_lock_done(ifp);

	LIST_INSERT_HEAD(&mli_head, mli, mli_link);
	mld_mli_list_genid++;

	MLD_UNLOCK();

	MLD_PRINTF(("%s: allocate mld_ifinfo for ifp 0x%llx(%s)\n",
	    __func__, (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));

	return mli;
}

  
                                                                     
                                                        
   
void
mld_domifreattach(struct mld_ifinfo *mli)
{
	struct ifnet *ifp;

	MLD_LOCK();

	MLI_LOCK(mli);
	VERIFY(!(mli->mli_debug & IFD_ATTACHED));
	ifp = mli->mli_ifp;
	VERIFY(ifp != NULL);
	mli_initvar(mli, ifp, 1);
	mli->mli_debug |= IFD_ATTACHED;
	MLI_ADDREF_LOCKED(mli);                                    
	MLI_UNLOCK(mli);
	ifnet_lock_shared(ifp);
	mld6_initsilent(ifp, mli);
	ifnet_lock_done(ifp);

	LIST_INSERT_HEAD(&mli_head, mli, mli_link);
	mld_mli_list_genid++;

	MLD_UNLOCK();

	MLD_PRINTF(("%s: reattached mld_ifinfo for ifp 0x%llx(%s)\n",
	    __func__, (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
}

  
                        
   
void
mld_domifdetach(struct ifnet *ifp)
{
	SLIST_HEAD(, in6_multi) in6m_dthead;

	SLIST_INIT(&in6m_dthead);

	MLD_PRINTF(("%s: called for ifp 0x%llx(%s)\n", __func__,
	    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));

	MLD_LOCK();
	mli_delete(ifp, (struct mld_in6m_relhead *)&in6m_dthead);
	MLD_UNLOCK();

	                                                                
	MLD_REMOVE_DETACHED_IN6M(&in6m_dthead);
}

  
                                                                         
                                                                            
                                                                        
                     
   
static void
mli_delete(const struct ifnet *ifp, struct mld_in6m_relhead *in6m_dthead)
{
	struct mld_ifinfo *mli, *tmli;

	MLD_LOCK_ASSERT_HELD();

	LIST_FOREACH_SAFE(mli, &mli_head, mli_link, tmli) {
		MLI_LOCK(mli);
		if (mli->mli_ifp == ifp) {
			  
                                            
      
			IF_DRAIN(&mli->mli_gq);
			IF_DRAIN(&mli->mli_v1q);
			mld_flush_relq(mli, in6m_dthead);
			VERIFY(SLIST_EMPTY(&mli->mli_relinmhead));
			mli->mli_debug &= ~IFD_ATTACHED;
			MLI_UNLOCK(mli);

			LIST_REMOVE(mli, mli_link);
			MLI_REMREF(mli);                                 
			mld_mli_list_genid++;
			return;
		}
		MLI_UNLOCK(mli);
	}
	panic("%s: mld_ifinfo not found for ifp %p(%s)\n", __func__,
	    ifp, ifp->if_xname);
}

__private_extern__ void
mld6_initsilent(struct ifnet *ifp, struct mld_ifinfo *mli)
{
	ifnet_lock_assert(ifp, IFNET_LCK_ASSERT_OWNED);

	MLI_LOCK_ASSERT_NOTHELD(mli);
	MLI_LOCK(mli);
	if (!(ifp->if_flags & IFF_MULTICAST) &&
	    (ifp->if_eflags & (IFEF_IPV6_ND6ALT | IFEF_LOCALNET_PRIVATE))) {
		mli->mli_flags |= MLIF_SILENT;
	} else {
		mli->mli_flags &= ~MLIF_SILENT;
	}
	MLI_UNLOCK(mli);
}

static void
mli_initvar(struct mld_ifinfo *mli, struct ifnet *ifp, int reattach)
{
	MLI_LOCK_ASSERT_HELD(mli);

	mli->mli_ifp = ifp;
	if (mld_v2enable) {
		mli->mli_version = MLD_VERSION_2;
	} else {
		mli->mli_version = MLD_VERSION_1;
	}
	mli->mli_flags = 0;
	mli->mli_rv = MLD_RV_INIT;
	mli->mli_qi = MLD_QI_INIT;
	mli->mli_qri = MLD_QRI_INIT;
	mli->mli_uri = MLD_URI_INIT;

	if (mld_use_allow) {
		mli->mli_flags |= MLIF_USEALLOW;
	}
	if (!reattach) {
		SLIST_INIT(&mli->mli_relinmhead);
	}

	  
                                                       
    
	mli->mli_gq.ifq_maxlen = MLD_MAX_RESPONSE_PACKETS;
	mli->mli_v1q.ifq_maxlen = MLD_MAX_RESPONSE_PACKETS;
}

static struct mld_ifinfo *
mli_alloc(zalloc_flags_t how)
{
	struct mld_ifinfo *mli = zalloc_flags(mli_zone, how | Z_ZERO);
	if (mli != NULL) {
		lck_mtx_init(&mli->mli_lock, mld_mtx_grp, mld_mtx_attr);
		mli->mli_debug |= IFD_ALLOC;
	}
	return mli;
}

static void
mli_free(struct mld_ifinfo *mli)
{
	MLI_LOCK(mli);
	if (mli->mli_debug & IFD_ATTACHED) {
		panic("%s: attached mli=%p is being freed", __func__, mli);
		                
	} else if (mli->mli_ifp != NULL) {
		panic("%s: ifp not NULL for mli=%p", __func__, mli);
		                
	} else if (!(mli->mli_debug & IFD_ALLOC)) {
		panic("%s: mli %p cannot be freed", __func__, mli);
		                
	} else if (mli->mli_refcnt != 0) {
		panic("%s: non-zero refcnt mli=%p", __func__, mli);
		                
	}
	mli->mli_debug &= ~IFD_ALLOC;
	MLI_UNLOCK(mli);

	lck_mtx_destroy(&mli->mli_lock, mld_mtx_grp);
	zfree(mli_zone, mli);
}

void
mli_addref(struct mld_ifinfo *mli, int locked)
{
	if (!locked) {
		MLI_LOCK_SPIN(mli);
	} else {
		MLI_LOCK_ASSERT_HELD(mli);
	}

	if (++mli->mli_refcnt == 0) {
		panic("%s: mli=%p wraparound refcnt", __func__, mli);
		                
	}
	if (!locked) {
		MLI_UNLOCK(mli);
	}
}

void
mli_remref(struct mld_ifinfo *mli)
{
	SLIST_HEAD(, in6_multi) in6m_dthead;
	struct ifnet *ifp;

	MLI_LOCK_SPIN(mli);

	if (mli->mli_refcnt == 0) {
		panic("%s: mli=%p negative refcnt", __func__, mli);
		                
	}

	--mli->mli_refcnt;
	if (mli->mli_refcnt > 0) {
		MLI_UNLOCK(mli);
		return;
	}

	ifp = mli->mli_ifp;
	mli->mli_ifp = NULL;
	IF_DRAIN(&mli->mli_gq);
	IF_DRAIN(&mli->mli_v1q);
	SLIST_INIT(&in6m_dthead);
	mld_flush_relq(mli, (struct mld_in6m_relhead *)&in6m_dthead);
	VERIFY(SLIST_EMPTY(&mli->mli_relinmhead));
	MLI_UNLOCK(mli);

	                                                                
	MLD_REMOVE_DETACHED_IN6M(&in6m_dthead);

	MLD_PRINTF(("%s: freeing mld_ifinfo for ifp 0x%llx(%s)\n",
	    __func__, (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));

	mli_free(mli);
}

  
                                                              
                                                                       
  
                                                                         
                                                 
   
static int
mld_v1_input_query(struct ifnet *ifp, const struct ip6_hdr *ip6,
              struct mld_hdr *mld)
{
	struct mld_ifinfo       *mli;
	struct in6_multi        *inm;
	int                      err = 0, is_general_query;
	uint16_t                 timer;
	struct mld_tparams       mtp = { .qpt = 0, .it = 0, .cst = 0, .sct = 0 };

	MLD_LOCK_ASSERT_NOTHELD();

	is_general_query = 0;

	if (!mld_v1enable) {
		MLD_PRINTF(("%s: ignore v1 query %s on ifp 0x%llx(%s)\n",
		    __func__, ip6_sprintf(&mld->mld_addr),
		    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
		goto done;
	}

	  
                                                        
                                  
    
	if (!IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_src)) {
		MLD_PRINTF(("%s: ignore v1 query src %s on ifp 0x%llx(%s)\n",
		    __func__, ip6_sprintf(&ip6->ip6_src),
		    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
		goto done;
	}

	  
                                                        
              
    
	if (IN6_IS_ADDR_UNSPECIFIED(&mld->mld_addr)) {
		  
                         
                                                            
     
		struct in6_addr          dst;

		dst = ip6->ip6_dst;
		in6_clearscope(&dst);
		if (!IN6_ARE_ADDR_EQUAL(&dst, &in6addr_linklocal_allnodes)) {
			err = EINVAL;
			goto done;
		}
		is_general_query = 1;
	} else {
		  
                                                           
                                             
     
		(void)in6_setscope(&mld->mld_addr, ifp, NULL);
	}

	  
                                            
    
	mli = MLD_IFINFO(ifp);
	VERIFY(mli != NULL);

	MLI_LOCK(mli);
	mtp.qpt = mld_set_version(mli, MLD_VERSION_1);
	MLI_UNLOCK(mli);

	timer = ntohs(mld->mld_maxdelay) / MLD_TIMER_SCALE;
	if (timer == 0) {
		timer = 1;
	}

	if (is_general_query) {
		struct in6_multistep step;

		MLD_PRINTF(("%s: process v1 general query on ifp 0x%llx(%s)\n",
		    __func__, (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
		  
                                            
                                      
     
		in6_multihead_lock_shared();
		IN6_FIRST_MULTI(step, inm);
		while (inm != NULL) {
			IN6M_LOCK(inm);
			if (inm->in6m_ifp == ifp) {
				mtp.cst += mld_v1_update_group(inm, timer);
			}
			IN6M_UNLOCK(inm);
			IN6_NEXT_MULTI(step, inm);
		}
		in6_multihead_lock_done();
	} else {
		  
                                
                                                          
                                            
     
		in6_multihead_lock_shared();
		IN6_LOOKUP_MULTI(&mld->mld_addr, ifp, inm);
		in6_multihead_lock_done();

		if (inm != NULL) {
			IN6M_LOCK(inm);
			MLD_PRINTF(("%s: process v1 query %s on "
			    "ifp 0x%llx(%s)\n", __func__,
			    ip6_sprintf(&mld->mld_addr),
			    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
			mtp.cst = mld_v1_update_group(inm, timer);
			IN6M_UNLOCK(inm);
			IN6M_REMREF(inm);                            
		}
		                                                              
		in6_clearscope(&mld->mld_addr);
	}
done:
	mld_set_timeout(&mtp);

	return err;
}

  
                                                                    
  
                                                                           
                                                                      
                                 
  
                                                                    
                                                                     
                                                                   
                                              
  
                                                       
                                    
   
static uint32_t
mld_v1_update_group(struct in6_multi *inm, const int timer)
{
	IN6M_LOCK_ASSERT_HELD(inm);

	MLD_PRINTF(("%s: %s/%s timer=%d\n", __func__,
	    ip6_sprintf(&inm->in6m_addr),
	    if_name(inm->in6m_ifp), timer));

	switch (inm->in6m_state) {
	case MLD_NOT_MEMBER:
	case MLD_SILENT_MEMBER:
		break;
	case MLD_REPORTING_MEMBER:
		if (inm->in6m_timer != 0 &&
		    inm->in6m_timer <= timer) {
			MLD_PRINTF(("%s: REPORTING and timer running, "
			    "skipping.\n", __func__));
			break;
		}
		OS_FALLTHROUGH;
	case MLD_SG_QUERY_PENDING_MEMBER:
	case MLD_G_QUERY_PENDING_MEMBER:
	case MLD_IDLE_MEMBER:
	case MLD_LAZY_MEMBER:
	case MLD_AWAKENING_MEMBER:
		MLD_PRINTF(("%s: ->REPORTING\n", __func__));
		inm->in6m_state = MLD_REPORTING_MEMBER;
		inm->in6m_timer = MLD_RANDOM_DELAY(timer);
		break;
	case MLD_SLEEPING_MEMBER:
		MLD_PRINTF(("%s: ->AWAKENING\n", __func__));
		inm->in6m_state = MLD_AWAKENING_MEMBER;
		break;
	case MLD_LEAVING_MEMBER:
		break;
	}

	return inm->in6m_timer;
}

  
                                                      
                                   
  
                                                                           
  
                                                                           
   
static int
mld_v2_input_query(struct ifnet *ifp, const struct ip6_hdr *ip6,
    struct mbuf *m, const int off, const int icmp6len)
{
	struct mld_ifinfo       *mli;
	struct mldv2_query      *mld;
	struct in6_multi        *inm;
	uint32_t                 maxdelay, nsrc, qqi, timer;
	int                      err = 0, is_general_query;
	uint8_t                  qrv;
	struct mld_tparams       mtp = { .qpt = 0, .it = 0, .cst = 0, .sct = 0 };

	MLD_LOCK_ASSERT_NOTHELD();

	is_general_query = 0;

	if (!mld_v2enable) {
		MLD_PRINTF(("%s: ignore v2 query %s on ifp 0x%llx(%s)\n",
		    __func__, ip6_sprintf(&ip6->ip6_src),
		    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
		goto done;
	}

	  
                                                        
                                  
    
	if (!IN6_IS_SCOPE_LINKLOCAL(&ip6->ip6_src)) {
		MLD_PRINTF(("%s: ignore v1 query src %s on ifp 0x%llx(%s)\n",
		    __func__, ip6_sprintf(&ip6->ip6_src),
		    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
		goto done;
	}

	MLD_PRINTF(("%s: input v2 query on ifp 0x%llx(%s)\n", __func__,
	    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));

	mld = (struct mldv2_query *)(mtod(m, uint8_t *) + off);

	maxdelay = ntohs(mld->mld_maxdelay);                                
	if (maxdelay > SHRT_MAX) {
		maxdelay = (MLD_MRC_MANT((uint16_t)maxdelay) | 0x1000) <<
		    (MLD_MRC_EXP((uint16_t)maxdelay) + 3);
	}
	timer = maxdelay / MLD_TIMER_SCALE;
	if (timer == 0) {
		timer = 1;
	}

	qrv = MLD_QRV(mld->mld_misc);
	if (qrv < 2) {
		MLD_PRINTF(("%s: clamping qrv %d to %d\n", __func__,
		    qrv, MLD_RV_INIT));
		qrv = MLD_RV_INIT;
	}

	qqi = mld->mld_qqi;
	if (qqi >= 128) {
		qqi = MLD_QQIC_MANT(mld->mld_qqi) <<
		    (MLD_QQIC_EXP(mld->mld_qqi) + 3);
	}

	nsrc = ntohs(mld->mld_numsrc);
	if (nsrc > MLD_MAX_GS_SOURCES) {
		err = EMSGSIZE;
		goto done;
	}
	if (icmp6len < sizeof(struct mldv2_query) +
	    (nsrc * sizeof(struct in6_addr))) {
		err = EMSGSIZE;
		goto done;
	}

	  
                                                                 
                                         
    
	if (IN6_IS_ADDR_UNSPECIFIED(&mld->mld_addr)) {
		  
                                                     
                           
     
		if (nsrc > 0) {
			err = EINVAL;
			goto done;
		}
		is_general_query = 1;
	} else {
		  
                                                           
                                                         
                                                        
     
		(void)in6_setscope(&mld->mld_addr, ifp, NULL);
	}

	mli = MLD_IFINFO(ifp);
	VERIFY(mli != NULL);

	MLI_LOCK(mli);
	  
                                                        
                                                                 
                                                        
    
	if (mli->mli_version != MLD_VERSION_2) {
		MLI_UNLOCK(mli);
		goto done;
	}

	mtp.qpt = mld_set_version(mli, MLD_VERSION_2);
	mli->mli_rv = qrv;
	mli->mli_qi = qqi;
	mli->mli_qri = MAX(timer, MLD_QRI_MIN);

	MLD_PRINTF(("%s: qrv %d qi %d qri %d\n", __func__, mli->mli_rv,
	    mli->mli_qi, mli->mli_qri));

	if (is_general_query) {
		  
                         
    
                                                    
                                                  
    
                                                 
                                                  
                                    
                                          
     
		MLD_PRINTF(("%s: process v2 general query on ifp 0x%llx(%s)\n",
		    __func__, (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
		if (mli->mli_v2_timer == 0 || mli->mli_v2_timer >= timer) {
			mtp.it = mli->mli_v2_timer = MLD_RANDOM_DELAY(timer);
		}
		MLI_UNLOCK(mli);
	} else {
		MLI_UNLOCK(mli);
		  
                                                             
    
                                                   
                                                            
                                                      
                             
     
		in6_multihead_lock_shared();
		IN6_LOOKUP_MULTI(&mld->mld_addr, ifp, inm);
		in6_multihead_lock_done();
		if (inm == NULL) {
			goto done;
		}

		IN6M_LOCK(inm);
		if (nsrc > 0) {
			if (!ratecheck(&inm->in6m_lastgsrtv,
			    &mld_gsrdelay)) {
				MLD_PRINTF(("%s: GS query throttled.\n",
				    __func__));
				IN6M_UNLOCK(inm);
				IN6M_REMREF(inm);                            
				goto done;
			}
		}
		MLD_PRINTF(("%s: process v2 group query on ifp 0x%llx(%s)\n",
		    __func__, (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
		  
                                                 
                                                 
                                      
                                         
                                              
     
		MLI_LOCK(mli);
		mtp.it = mli->mli_v2_timer;
		MLI_UNLOCK(mli);
		if (mtp.it == 0 || mtp.it >= timer) {
			(void) mld_v2_process_group_query(inm, timer, m, off);
			mtp.cst = inm->in6m_timer;
		}
		IN6M_UNLOCK(inm);
		IN6M_REMREF(inm);                            
		                                                              
		in6_clearscope(&mld->mld_addr);
	}
done:
	if (mtp.it > 0) {
		MLD_PRINTF(("%s: v2 general query response scheduled in "
		    "T+%d seconds on ifp 0x%llx(%s)\n", __func__, mtp.it,
		    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
	}
	mld_set_timeout(&mtp);

	return err;
}

  
                                                                       
         
                                                             
   
static int
mld_v2_process_group_query(struct in6_multi *inm, int timer, struct mbuf *m0,
    const int off)
{
	struct mldv2_query      *mld;
	int                      retval;
	uint16_t                 nsrc;

	IN6M_LOCK_ASSERT_HELD(inm);

	retval = 0;
	mld = (struct mldv2_query *)(mtod(m0, uint8_t *) + off);

	switch (inm->in6m_state) {
	case MLD_NOT_MEMBER:
	case MLD_SILENT_MEMBER:
	case MLD_SLEEPING_MEMBER:
	case MLD_LAZY_MEMBER:
	case MLD_AWAKENING_MEMBER:
	case MLD_IDLE_MEMBER:
	case MLD_LEAVING_MEMBER:
		return retval;
	case MLD_REPORTING_MEMBER:
	case MLD_G_QUERY_PENDING_MEMBER:
	case MLD_SG_QUERY_PENDING_MEMBER:
		break;
	}

	nsrc = ntohs(mld->mld_numsrc);

	  
                                             
                                                             
                                                                 
                                  
    
	if (nsrc == 0) {
		if (inm->in6m_state == MLD_G_QUERY_PENDING_MEMBER ||
		    inm->in6m_state == MLD_SG_QUERY_PENDING_MEMBER) {
			in6m_clear_recorded(inm);
			timer = min(inm->in6m_timer, timer);
		}
		inm->in6m_state = MLD_G_QUERY_PENDING_MEMBER;
		inm->in6m_timer = MLD_RANDOM_DELAY(timer);
		return retval;
	}

	  
                                                                  
                                                                
    
	if (inm->in6m_state == MLD_G_QUERY_PENDING_MEMBER) {
		timer = min(inm->in6m_timer, timer);
		inm->in6m_timer = MLD_RANDOM_DELAY(timer);
		return retval;
	}

	  
                                                                 
                                                                 
                                 
                                                                    
                                                                    
                                                                   
                                                   
                                                              
                             
    
	if (inm->in6m_nsrc > 0) {
		struct mbuf             *m;
		uint8_t                 *sp;
		int                      i, nrecorded;
		int                      soff;

		m = m0;
		soff = off + sizeof(struct mldv2_query);
		nrecorded = 0;
		for (i = 0; i < nsrc; i++) {
			sp = mtod(m, uint8_t *) + soff;
			retval = in6m_record_source(inm,
			    (const struct in6_addr *)(void *)sp);
			if (retval < 0) {
				break;
			}
			nrecorded += retval;
			soff += sizeof(struct in6_addr);
			if (soff >= m->m_len) {
				soff = soff - m->m_len;
				m = m->m_next;
				if (m == NULL) {
					break;
				}
			}
		}
		if (nrecorded > 0) {
			MLD_PRINTF(("%s: schedule response to SG query\n",
			    __func__));
			inm->in6m_state = MLD_SG_QUERY_PENDING_MEMBER;
			inm->in6m_timer = MLD_RANDOM_DELAY(timer);
		}
	}

	return retval;
}

  
                                                   
                                                         
  
                                                                         
                                                 
   
static int
mld_v1_input_report(struct ifnet *ifp, struct mbuf *m,
    const struct ip6_hdr *ip6,           struct mld_hdr *mld)
{
	struct in6_addr          src, dst;
	struct in6_ifaddr       *ia;
	struct in6_multi        *inm;

	if (!mld_v1enable) {
		MLD_PRINTF(("%s: ignore v1 report %s on ifp 0x%llx(%s)\n",
		    __func__, ip6_sprintf(&mld->mld_addr),
		    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
		return 0;
	}

	if ((ifp->if_flags & IFF_LOOPBACK) ||
	    (m->m_pkthdr.pkt_flags & PKTF_LOOP)) {
		return 0;
	}

	  
                                                                  
                                              
    
	src = ip6->ip6_src;
	in6_clearscope(&src);
	if (!IN6_IS_SCOPE_LINKLOCAL(&src) && !IN6_IS_ADDR_UNSPECIFIED(&src)) {
		MLD_PRINTF(("%s: ignore v1 query src %s on ifp 0x%llx(%s)\n",
		    __func__, ip6_sprintf(&ip6->ip6_src),
		    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
		return EINVAL;
	}

	  
                                                                
                                                    
    
	dst = ip6->ip6_dst;
	in6_clearscope(&dst);
	if (!IN6_IS_ADDR_MULTICAST(&mld->mld_addr) ||
	    !IN6_ARE_ADDR_EQUAL(&mld->mld_addr, &dst)) {
		MLD_PRINTF(("%s: ignore v1 query dst %s on ifp 0x%llx(%s)\n",
		    __func__, ip6_sprintf(&ip6->ip6_dst),
		    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
		return EINVAL;
	}

	  
                                                              
                                                           
                                                              
                          
   
                                                               
                                                           
                                      
    
	ia = in6ifa_ifpforlinklocal(ifp, IN6_IFF_NOTREADY | IN6_IFF_ANYCAST);
	if (ia != NULL) {
		IFA_LOCK(&ia->ia_ifa);
		if ((IN6_ARE_ADDR_EQUAL(&ip6->ip6_src, IA6_IN6(ia)))) {
			IFA_UNLOCK(&ia->ia_ifa);
			IFA_REMREF(&ia->ia_ifa);
			return 0;
		}
		IFA_UNLOCK(&ia->ia_ifa);
		IFA_REMREF(&ia->ia_ifa);
	} else if (IN6_IS_ADDR_UNSPECIFIED(&src)) {
		return 0;
	}

	MLD_PRINTF(("%s: process v1 report %s on ifp 0x%llx(%s)\n",
	    __func__, ip6_sprintf(&mld->mld_addr),
	    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));

	  
                                                                 
                                                                    
    
	if (!IN6_IS_ADDR_UNSPECIFIED(&mld->mld_addr)) {
		(void)in6_setscope(&mld->mld_addr, ifp, NULL);
	}

	  
                             
                                                                  
                                                                  
                                                              
    
	in6_multihead_lock_shared();
	IN6_LOOKUP_MULTI(&mld->mld_addr, ifp, inm);
	in6_multihead_lock_done();

	if (inm != NULL) {
		struct mld_ifinfo *mli;

		IN6M_LOCK(inm);
		mli = inm->in6m_mli;
		VERIFY(mli != NULL);

		MLI_LOCK(mli);
		  
                                                   
                                                       
     
		if (mli->mli_version == MLD_VERSION_2) {
			MLI_UNLOCK(mli);
			IN6M_UNLOCK(inm);
			IN6M_REMREF(inm);                            
			goto out;
		}
		MLI_UNLOCK(mli);

		inm->in6m_timer = 0;

		switch (inm->in6m_state) {
		case MLD_NOT_MEMBER:
		case MLD_SILENT_MEMBER:
		case MLD_SLEEPING_MEMBER:
			break;
		case MLD_REPORTING_MEMBER:
		case MLD_IDLE_MEMBER:
		case MLD_AWAKENING_MEMBER:
			MLD_PRINTF(("%s: report suppressed for %s on "
			    "ifp 0x%llx(%s)\n", __func__,
			    ip6_sprintf(&mld->mld_addr),
			    (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));
			OS_FALLTHROUGH;
		case MLD_LAZY_MEMBER:
			inm->in6m_state = MLD_LAZY_MEMBER;
			break;
		case MLD_G_QUERY_PENDING_MEMBER:
		case MLD_SG_QUERY_PENDING_MEMBER:
		case MLD_LEAVING_MEMBER:
			break;
		}
		IN6M_UNLOCK(inm);
		IN6M_REMREF(inm);                            
	}

out:
	                                                              
	in6_clearscope(&mld->mld_addr);

	return 0;
}

  
                  
  
                                                                    
                       
                                                                
                                                           
                                                        
  
                                                          
   
int
mld_input(struct mbuf *m, int off, int icmp6len)
{
	struct ifnet    *ifp = NULL;
	struct ip6_hdr  *ip6 = NULL;
	struct mld_hdr  *mld = NULL;
	int              mldlen = 0;

	MLD_PRINTF(("%s: called w/mbuf (0x%llx,%d)\n", __func__,
	    (uint64_t)VM_KERNEL_ADDRPERM(m), off));

	ifp = m->m_pkthdr.rcvif;

	                                 
	mld = (struct mld_hdr *)(mtod(m, uint8_t *) + off);
	if (mld->mld_type == MLD_LISTENER_QUERY &&
	    icmp6len >= sizeof(struct mldv2_query)) {
		mldlen = sizeof(struct mldv2_query);
	} else {
		mldlen = sizeof(struct mld_hdr);
	}
	                                                      
	IP6_EXTHDR_CHECK(m, off, mldlen, return IPPROTO_DONE);
	IP6_EXTHDR_GET(mld, struct mld_hdr *, m, off, mldlen);
	if (mld == NULL) {
		icmp6stat.icp6s_badlen++;
		return IPPROTO_DONE;
	}
	ip6 = mtod(m, struct ip6_hdr *);

	  
                                                              
                                                        
    
	switch (mld->mld_type) {
	case MLD_LISTENER_QUERY:
		icmp6_ifstat_inc(ifp, ifs6_in_mldquery);
		if (icmp6len == sizeof(struct mld_hdr)) {
			if (mld_v1_input_query(ifp, ip6, mld) != 0) {
				return 0;
			}
		} else if (icmp6len >= sizeof(struct mldv2_query)) {
			if (mld_v2_input_query(ifp, ip6, m, off,
			    icmp6len) != 0) {
				return 0;
			}
		}
		break;
	case MLD_LISTENER_REPORT:
		icmp6_ifstat_inc(ifp, ifs6_in_mldreport);
		if (mld_v1_input_report(ifp, m, ip6, mld) != 0) {
			return 0;
		}
		break;
	case MLDV2_LISTENER_REPORT:
		icmp6_ifstat_inc(ifp, ifs6_in_mldreport);
		break;
	case MLD_LISTENER_DONE:
		icmp6_ifstat_inc(ifp, ifs6_in_mlddone);
		break;
	default:
		break;
	}

	return 0;
}

  
                                                                          
                                                                        
   
void
mld_set_timeout(struct mld_tparams *mtp)
{
	MLD_LOCK_ASSERT_NOTHELD();
	VERIFY(mtp != NULL);

	if (mtp->qpt != 0 || mtp->it != 0 || mtp->cst != 0 || mtp->sct != 0) {
		MLD_LOCK();
		if (mtp->qpt != 0) {
			querier_present_timers_running6 = 1;
		}
		if (mtp->it != 0) {
			interface_timers_running6 = 1;
		}
		if (mtp->cst != 0) {
			current_state_timers_running6 = 1;
		}
		if (mtp->sct != 0) {
			state_change_timers_running6 = 1;
		}
		mld_sched_timeout();
		MLD_UNLOCK();
	}
}

  
                                     
   
static void
mld_timeout(void *arg)
{
#pragma unused(arg)
	struct ifqueue           scq;                             
	struct ifqueue           qrq;                               
	struct ifnet            *ifp;
	struct mld_ifinfo       *mli;
	struct in6_multi        *inm;
	int                      uri_sec = 0;
	unsigned int genid = mld_mli_list_genid;

	SLIST_HEAD(, in6_multi) in6m_dthead;

	SLIST_INIT(&in6m_dthead);

	  
                                                                  
                                                                 
                                
    
	net_update_uptime();

	MLD_LOCK();

	MLD_PRINTF(("%s: qpt %d, it %d, cst %d, sct %d\n", __func__,
	    querier_present_timers_running6, interface_timers_running6,
	    current_state_timers_running6, state_change_timers_running6));

	  
                                           
    
	if (querier_present_timers_running6) {
		querier_present_timers_running6 = 0;
		LIST_FOREACH(mli, &mli_head, mli_link) {
			MLI_LOCK(mli);
			mld_v1_process_querier_timers(mli);
			if (mli->mli_v1_timer > 0) {
				querier_present_timers_running6 = 1;
			}
			MLI_UNLOCK(mli);
		}
	}

	  
                                                  
    
	if (interface_timers_running6) {
		MLD_PRINTF(("%s: interface timers running\n", __func__));
		interface_timers_running6 = 0;
		mli = LIST_FIRST(&mli_head);

		while (mli != NULL) {
			if (mli->mli_flags & MLIF_PROCESSED) {
				mli = LIST_NEXT(mli, mli_link);
				continue;
			}

			MLI_LOCK(mli);
			if (mli->mli_version != MLD_VERSION_2) {
				MLI_UNLOCK(mli);
				mli = LIST_NEXT(mli, mli_link);
				continue;
			}
			  
                                         
                                              
                              
                                                
                                                    
                             
      
			MLI_ADDREF_LOCKED(mli);

			if (mli->mli_v2_timer == 0) {
				                 
			} else if (--mli->mli_v2_timer == 0) {
				if (mld_v2_dispatch_general_query(mli) > 0) {
					interface_timers_running6 = 1;
				}
			} else {
				interface_timers_running6 = 1;
			}
			mli->mli_flags |= MLIF_PROCESSED;
			MLI_UNLOCK(mli);
			MLI_REMREF(mli);

			if (genid != mld_mli_list_genid) {
				MLD_PRINTF(("%s: MLD information list changed "
				    "in the middle of iteration! Restart iteration.\n",
				    __func__));
				mli = LIST_FIRST(&mli_head);
				genid = mld_mli_list_genid;
			} else {
				mli = LIST_NEXT(mli, mli_link);
			}
		}

		LIST_FOREACH(mli, &mli_head, mli_link)
		mli->mli_flags &= ~MLIF_PROCESSED;
	}



	if (!current_state_timers_running6 &&
	    !state_change_timers_running6) {
		goto out_locked;
	}

	current_state_timers_running6 = 0;
	state_change_timers_running6 = 0;

	MLD_PRINTF(("%s: state change timers running\n", __func__));

	memset(&qrq, 0, sizeof(struct ifqueue));
	qrq.ifq_maxlen = MLD_MAX_G_GS_PACKETS;

	memset(&scq, 0, sizeof(struct ifqueue));
	scq.ifq_maxlen = MLD_MAX_STATE_CHANGE_PACKETS;

	  
                                                      
                                                        
    
	mli = LIST_FIRST(&mli_head);

	while (mli != NULL) {
		struct in6_multistep step;

		if (mli->mli_flags & MLIF_PROCESSED) {
			mli = LIST_NEXT(mli, mli_link);
			continue;
		}

		MLI_LOCK(mli);
		ifp = mli->mli_ifp;
		uri_sec = MLD_RANDOM_DELAY(mli->mli_uri);
		MLI_UNLOCK(mli);

		in6_multihead_lock_shared();
		IN6_FIRST_MULTI(step, inm);
		while (inm != NULL) {
			IN6M_LOCK(inm);
			if (inm->in6m_ifp != ifp) {
				goto next;
			}

			MLI_LOCK(mli);
			switch (mli->mli_version) {
			case MLD_VERSION_1:
				mld_v1_process_group_timer(inm,
				    mli->mli_version);
				break;
			case MLD_VERSION_2:
				mld_v2_process_group_timers(mli, &qrq,
				    &scq, inm, uri_sec);
				break;
			}
			MLI_UNLOCK(mli);
next:
			IN6M_UNLOCK(inm);
			IN6_NEXT_MULTI(step, inm);
		}
		in6_multihead_lock_done();

		  
                                        
                                             
                             
                                               
                                                   
                            
     
		MLI_LOCK(mli);
		MLI_ADDREF_LOCKED(mli);
		if (mli->mli_version == MLD_VERSION_1) {
			mld_dispatch_queue_locked(mli, &mli->mli_v1q, 0);
		} else if (mli->mli_version == MLD_VERSION_2) {
			MLI_UNLOCK(mli);
			mld_dispatch_queue_locked(NULL, &qrq, 0);
			mld_dispatch_queue_locked(NULL, &scq, 0);
			VERIFY(qrq.ifq_len == 0);
			VERIFY(scq.ifq_len == 0);
			MLI_LOCK(mli);
		}
		  
                                                           
                                                     
     
		IF_DRAIN(&mli->mli_v1q);
		  
                                                            
                                                            
                                                            
                         
     
		mld_flush_relq(mli, (struct mld_in6m_relhead *)&in6m_dthead);
		VERIFY(SLIST_EMPTY(&mli->mli_relinmhead));
		mli->mli_flags |= MLIF_PROCESSED;
		MLI_UNLOCK(mli);
		MLI_REMREF(mli);

		IF_DRAIN(&qrq);
		IF_DRAIN(&scq);

		if (genid != mld_mli_list_genid) {
			MLD_PRINTF(("%s: MLD information list changed "
			    "in the middle of iteration! Restart iteration.\n",
			    __func__));
			mli = LIST_FIRST(&mli_head);
			genid = mld_mli_list_genid;
		} else {
			mli = LIST_NEXT(mli, mli_link);
		}
	}

	LIST_FOREACH(mli, &mli_head, mli_link)
	mli->mli_flags &= ~MLIF_PROCESSED;

out_locked:
	                                            
	mld_timeout_run = 0;
	mld_sched_timeout();
	MLD_UNLOCK();

	                                                                
	MLD_REMOVE_DETACHED_IN6M(&in6m_dthead);
}

static void
mld_sched_timeout(void)
{
	MLD_LOCK_ASSERT_HELD();

	if (!mld_timeout_run &&
	    (querier_present_timers_running6 || current_state_timers_running6 ||
	    interface_timers_running6 || state_change_timers_running6)) {
		mld_timeout_run = 1;
		timeout(mld_timeout, NULL, hz);
	}
}

  
                                                          
  
                                   
   
static void
mld_flush_relq(struct mld_ifinfo *mli, struct mld_in6m_relhead *in6m_dthead)
{
	struct in6_multi *inm;

again:
	MLI_LOCK_ASSERT_HELD(mli);
	inm = SLIST_FIRST(&mli->mli_relinmhead);
	if (inm != NULL) {
		int lastref;

		SLIST_REMOVE_HEAD(&mli->mli_relinmhead, in6m_nrele);
		MLI_UNLOCK(mli);

		in6_multihead_lock_exclusive();
		IN6M_LOCK(inm);
		VERIFY(inm->in6m_nrelecnt != 0);
		inm->in6m_nrelecnt--;
		lastref = in6_multi_detach(inm);
		VERIFY(!lastref || (!(inm->in6m_debug & IFD_ATTACHED) &&
		    inm->in6m_reqcnt == 0));
		IN6M_UNLOCK(inm);
		in6_multihead_lock_done();
		                         
		IN6M_REMREF(inm);
		                             
		if (lastref) {
			  
                                                
                                                 
                                                  
                                                   
                                                  
                      
      
			MLD_ADD_DETACHED_IN6M(in6m_dthead, inm);
		}
		MLI_LOCK(mli);
		goto again;
	}
}

  
                                  
                                              
   
static void
mld_v1_process_group_timer(struct in6_multi *inm, const int mld_version)
{
#pragma unused(mld_version)
	int report_timer_expired;

	MLD_LOCK_ASSERT_HELD();
	IN6M_LOCK_ASSERT_HELD(inm);
	MLI_LOCK_ASSERT_HELD(inm->in6m_mli);

	if (inm->in6m_timer == 0) {
		report_timer_expired = 0;
	} else if (--inm->in6m_timer == 0) {
		report_timer_expired = 1;
	} else {
		current_state_timers_running6 = 1;
		                                
		return;
	}

	switch (inm->in6m_state) {
	case MLD_NOT_MEMBER:
	case MLD_SILENT_MEMBER:
	case MLD_IDLE_MEMBER:
	case MLD_LAZY_MEMBER:
	case MLD_SLEEPING_MEMBER:
	case MLD_AWAKENING_MEMBER:
		break;
	case MLD_REPORTING_MEMBER:
		if (report_timer_expired) {
			inm->in6m_state = MLD_IDLE_MEMBER;
			(void) mld_v1_transmit_report(inm,
			    MLD_LISTENER_REPORT);
			IN6M_LOCK_ASSERT_HELD(inm);
			MLI_LOCK_ASSERT_HELD(inm->in6m_mli);
		}
		break;
	case MLD_G_QUERY_PENDING_MEMBER:
	case MLD_SG_QUERY_PENDING_MEMBER:
	case MLD_LEAVING_MEMBER:
		break;
	}
}

  
                                     
                                              
                                
   
static void
mld_v2_process_group_timers(struct mld_ifinfo *mli,
    struct ifqueue *qrq, struct ifqueue *scq,
    struct in6_multi *inm, const int uri_sec)
{
	int query_response_timer_expired;
	int state_change_retransmit_timer_expired;

	MLD_LOCK_ASSERT_HELD();
	IN6M_LOCK_ASSERT_HELD(inm);
	MLI_LOCK_ASSERT_HELD(mli);
	VERIFY(mli == inm->in6m_mli);

	query_response_timer_expired = 0;
	state_change_retransmit_timer_expired = 0;

	  
                                                              
                                                              
                                                                
                                                             
    
	if (inm->in6m_timer == 0) {
		query_response_timer_expired = 0;
	} else if (--inm->in6m_timer == 0) {
		query_response_timer_expired = 1;
	} else {
		current_state_timers_running6 = 1;
		                                
	}

	if (inm->in6m_sctimer == 0) {
		state_change_retransmit_timer_expired = 0;
	} else if (--inm->in6m_sctimer == 0) {
		state_change_retransmit_timer_expired = 1;
	} else {
		state_change_timers_running6 = 1;
		                                
	}

	                                                     
	if (!state_change_retransmit_timer_expired &&
	    !query_response_timer_expired) {
		return;
	}

	switch (inm->in6m_state) {
	case MLD_NOT_MEMBER:
	case MLD_SILENT_MEMBER:
	case MLD_SLEEPING_MEMBER:
	case MLD_LAZY_MEMBER:
	case MLD_AWAKENING_MEMBER:
	case MLD_IDLE_MEMBER:
		break;
	case MLD_G_QUERY_PENDING_MEMBER:
	case MLD_SG_QUERY_PENDING_MEMBER:
		  
                                                   
                                                     
                                             
                            
     
		if (query_response_timer_expired) {
			int retval;

			retval = mld_v2_enqueue_group_record(qrq, inm, 0, 1,
			    (inm->in6m_state == MLD_SG_QUERY_PENDING_MEMBER),
			    0);
			MLD_PRINTF(("%s: enqueue record = %d\n",
			    __func__, retval));
			inm->in6m_state = MLD_REPORTING_MEMBER;
			in6m_clear_recorded(inm);
		}
		OS_FALLTHROUGH;
	case MLD_REPORTING_MEMBER:
	case MLD_LEAVING_MEMBER:
		if (state_change_retransmit_timer_expired) {
			  
                                              
                                                       
                                                   
                      
      
			if (--inm->in6m_scrv > 0) {
				inm->in6m_sctimer = (uint16_t)uri_sec;
				state_change_timers_running6 = 1;
				                                
			}
			  
                                                     
                                             
                                                       
                                               
                     
      
			(void) mld_v2_merge_state_changes(inm, scq);

			in6m_commit(inm);
			MLD_PRINTF(("%s: T1 -> T0 for %s/%s\n", __func__,
			    ip6_sprintf(&inm->in6m_addr),
			    if_name(inm->in6m_ifp)));

			  
                                                     
                                       
                                                  
                                                  
                               
      
			if (inm->in6m_state == MLD_LEAVING_MEMBER &&
			    inm->in6m_scrv == 0) {
				inm->in6m_state = MLD_NOT_MEMBER;
				  
                                           
                                         
                                            
                                           
                                            
                                          
                         
       
				VERIFY(inm->in6m_nrelecnt != 0);
				SLIST_INSERT_HEAD(&mli->mli_relinmhead,
				    inm, in6m_nrele);
			}
		}
		break;
	}
}

  
                                                        
                       
   
static uint32_t
mld_set_version(struct mld_ifinfo *mli, const int mld_version)
{
	int old_version_timer;

	MLI_LOCK_ASSERT_HELD(mli);

	MLD_PRINTF(("%s: switching to v%d on ifp 0x%llx(%s)\n", __func__,
	    mld_version, (uint64_t)VM_KERNEL_ADDRPERM(mli->mli_ifp),
	    if_name(mli->mli_ifp)));

	if (mld_version == MLD_VERSION_1) {
		  
                                                             
                              
     
		old_version_timer = (mli->mli_rv * mli->mli_qi) + mli->mli_qri;
		mli->mli_v1_timer = old_version_timer;
	}

	if (mli->mli_v1_timer > 0 && mli->mli_version != MLD_VERSION_1) {
		mli->mli_version = MLD_VERSION_1;
		mld_v2_cancel_link_timers(mli);
	}

	MLI_LOCK_ASSERT_HELD(mli);

	return mli->mli_v1_timer;
}

  
                                                                
                                                                     
  
                                                                       
                                                                         
                                                                         
                    
   
static void
mld_v2_cancel_link_timers(struct mld_ifinfo *mli)
{
	struct ifnet            *ifp;
	struct in6_multi        *inm;
	struct in6_multistep    step;

	MLI_LOCK_ASSERT_HELD(mli);

	MLD_PRINTF(("%s: cancel v2 timers on ifp 0x%llx(%s)\n", __func__,
	    (uint64_t)VM_KERNEL_ADDRPERM(mli->mli_ifp), if_name(mli->mli_ifp)));

	  
                                                               
                                                          
                                                                 
    
	mli->mli_v2_timer = 0;

	  
                                                              
                                            
    
	ifp = mli->mli_ifp;
	MLI_UNLOCK(mli);

	in6_multihead_lock_shared();
	IN6_FIRST_MULTI(step, inm);
	while (inm != NULL) {
		IN6M_LOCK(inm);
		if (inm->in6m_ifp != ifp) {
			goto next;
		}

		switch (inm->in6m_state) {
		case MLD_NOT_MEMBER:
		case MLD_SILENT_MEMBER:
		case MLD_IDLE_MEMBER:
		case MLD_LAZY_MEMBER:
		case MLD_SLEEPING_MEMBER:
		case MLD_AWAKENING_MEMBER:
			  
                                                      
                                    
      
			break;
		case MLD_LEAVING_MEMBER:
			  
                                               
                                           
                                                
                                                     
                                                    
                                                      
                        
      
			VERIFY(inm->in6m_nrelecnt != 0);
			MLI_LOCK(mli);
			SLIST_INSERT_HEAD(&mli->mli_relinmhead, inm,
			    in6m_nrele);
			MLI_UNLOCK(mli);
			OS_FALLTHROUGH;
		case MLD_G_QUERY_PENDING_MEMBER:
		case MLD_SG_QUERY_PENDING_MEMBER:
			in6m_clear_recorded(inm);
			OS_FALLTHROUGH;
		case MLD_REPORTING_MEMBER:
			inm->in6m_state = MLD_REPORTING_MEMBER;
			break;
		}
		  
                                                       
                                                 
     
		inm->in6m_sctimer = 0;
		inm->in6m_timer = 0;
		IF_DRAIN(&inm->in6m_scq);
next:
		IN6M_UNLOCK(inm);
		IN6_NEXT_MULTI(step, inm);
	}
	in6_multihead_lock_done();

	MLI_LOCK(mli);
}

  
                                                              
                                
   
static void
mld_v1_process_querier_timers(struct mld_ifinfo *mli)
{
	MLI_LOCK_ASSERT_HELD(mli);

	if (mld_v2enable && mli->mli_version != MLD_VERSION_2 &&
	    --mli->mli_v1_timer == 0) {
		  
                                                          
     
		MLD_PRINTF(("%s: transition from v%d -> v%d on 0x%llx(%s)\n",
		    __func__, mli->mli_version, MLD_VERSION_2,
		    (uint64_t)VM_KERNEL_ADDRPERM(mli->mli_ifp),
		    if_name(mli->mli_ifp)));
		mli->mli_version = MLD_VERSION_2;
	}
}

  
                                        
   
static int
mld_v1_transmit_report(struct in6_multi *in6m, const uint8_t type)
{
	struct ifnet            *ifp;
	struct in6_ifaddr       *ia;
	struct ip6_hdr          *ip6;
	struct mbuf             *mh, *md;
	struct mld_hdr          *mld;
	int                     error = 0;

	IN6M_LOCK_ASSERT_HELD(in6m);
	MLI_LOCK_ASSERT_HELD(in6m->in6m_mli);

	ifp = in6m->in6m_ifp;
	                                                        
	ia = in6ifa_ifpforlinklocal(ifp, IN6_IFF_NOTREADY | IN6_IFF_ANYCAST);

	MGETHDR(mh, M_DONTWAIT, MT_HEADER);
	if (mh == NULL) {
		if (ia != NULL) {
			IFA_REMREF(&ia->ia_ifa);
		}
		return ENOMEM;
	}
	MGET(md, M_DONTWAIT, MT_DATA);
	if (md == NULL) {
		m_free(mh);
		if (ia != NULL) {
			IFA_REMREF(&ia->ia_ifa);
		}
		return ENOMEM;
	}
	mh->m_next = md;

	  
                                                              
                                                              
                                           
    
	MH_ALIGN(mh, sizeof(struct ip6_hdr));
	mh->m_pkthdr.len = sizeof(struct ip6_hdr) + sizeof(struct mld_hdr);
	mh->m_len = sizeof(struct ip6_hdr);

	ip6 = mtod(mh, struct ip6_hdr *);
	ip6->ip6_flow = 0;
	ip6->ip6_vfc &= ~IPV6_VERSION_MASK;
	ip6->ip6_vfc |= IPV6_VERSION;
	ip6->ip6_nxt = IPPROTO_ICMPV6;
	if (ia != NULL) {
		IFA_LOCK(&ia->ia_ifa);
	}
	ip6->ip6_src = ia ? ia->ia_addr.sin6_addr : in6addr_any;
	if (ia != NULL) {
		IFA_UNLOCK(&ia->ia_ifa);
		IFA_REMREF(&ia->ia_ifa);
		ia = NULL;
	}
	ip6->ip6_dst = in6m->in6m_addr;

	md->m_len = sizeof(struct mld_hdr);
	mld = mtod(md, struct mld_hdr *);
	mld->mld_type = type;
	mld->mld_code = 0;
	mld->mld_cksum = 0;
	mld->mld_maxdelay = 0;
	mld->mld_reserved = 0;
	mld->mld_addr = in6m->in6m_addr;
	in6_clearscope(&mld->mld_addr);
	mld->mld_cksum = in6_cksum(mh, IPPROTO_ICMPV6,
	    sizeof(struct ip6_hdr), sizeof(struct mld_hdr));

	mld_save_context(mh, ifp);
	mh->m_flags |= M_MLDV1;

	  
                                                              
                                                                 
                                                              
                                                                     
               
                                                               
                                              
    
	if (IF_QFULL(&in6m->in6m_mli->mli_v1q)) {
		MLD_PRINTF(("%s: v1 outbound queue full\n", __func__));
		error = ENOMEM;
		m_freem(mh);
	} else {
		IF_ENQUEUE(&in6m->in6m_mli->mli_v1q, mh);
		VERIFY(error == 0);
	}

	return error;
}

  
                                                                        
  
                                                                         
                                                                     
                                                                     
                                                                         
                
  
                                                                
                                                                        
                                                                 
                                                             
  
                                                                     
                                                                 
                                                                
                                                                     
                                                           
   
int
mld_change_state(struct in6_multi *inm, struct mld_tparams *mtp,
    const int delay)
{
	struct mld_ifinfo *mli;
	struct ifnet *ifp;
	int error = 0;

	VERIFY(mtp != NULL);
	bzero(mtp, sizeof(*mtp));

	IN6M_LOCK_ASSERT_HELD(inm);
	VERIFY(inm->in6m_mli != NULL);
	MLI_LOCK_ASSERT_NOTHELD(inm->in6m_mli);

	  
                                                                  
                                             
    
	VERIFY(inm->in6m_ifma != NULL);
	ifp = inm->in6m_ifma->ifma_ifp;
	  
                                                                    
    
	VERIFY(inm->in6m_ifp == ifp);

	mli = MLD_IFINFO(ifp);
	VERIFY(mli != NULL);

	  
                                                              
                                                            
                              
    
	if (inm->in6m_st[1].iss_fmode != inm->in6m_st[0].iss_fmode) {
		MLD_PRINTF(("%s: inm transition %d -> %d\n", __func__,
		    inm->in6m_st[0].iss_fmode, inm->in6m_st[1].iss_fmode));
		if (inm->in6m_st[0].iss_fmode == MCAST_UNDEFINED) {
			MLD_PRINTF(("%s: initial join\n", __func__));
			error = mld_initial_join(inm, mli, mtp, delay);
			goto out;
		} else if (inm->in6m_st[1].iss_fmode == MCAST_UNDEFINED) {
			MLD_PRINTF(("%s: final leave\n", __func__));
			mld_final_leave(inm, mli, mtp);
			goto out;
		}
	} else {
		MLD_PRINTF(("%s: filter set change\n", __func__));
	}

	error = mld_handle_state_change(inm, mli, mtp);
out:
	return error;
}

  
                                             
  
                        
                                                                    
                                                       
                                                                   
                                    
  
                                                                    
                                                              
   
static int
mld_initial_join(struct in6_multi *inm, struct mld_ifinfo *mli,
    struct mld_tparams *mtp, const int delay)
{
	struct ifnet            *ifp;
	struct ifqueue          *ifq;
	int                      error, retval, syncstates;
	int                      odelay;

	IN6M_LOCK_ASSERT_HELD(inm);
	MLI_LOCK_ASSERT_NOTHELD(mli);
	VERIFY(mtp != NULL);

	MLD_PRINTF(("%s: initial join %s on ifp 0x%llx(%s)\n",
	    __func__, ip6_sprintf(&inm->in6m_addr),
	    (uint64_t)VM_KERNEL_ADDRPERM(inm->in6m_ifp),
	    if_name(inm->in6m_ifp)));

	error = 0;
	syncstates = 1;

	ifp = inm->in6m_ifp;

	MLI_LOCK(mli);
	VERIFY(mli->mli_ifp == ifp);

	  
                           
                             
                                           
                                                       
                            
                                                        
                                        
                                                        
                                        
    
	if ((ifp->if_flags & IFF_LOOPBACK) ||
	    (mli->mli_flags & MLIF_SILENT) ||
	    (IFNET_IS_CELLULAR(ifp) &&
	    (IN6_IS_ADDR_MC_LINKLOCAL(&inm->in6m_addr) || IN6_IS_ADDR_MC_UNICAST_BASED_LINKLOCAL(&inm->in6m_addr))) ||
	    !mld_is_addr_reported(&inm->in6m_addr)) {
		MLD_PRINTF(("%s: not kicking state machine for silent group\n",
		    __func__));
		inm->in6m_state = MLD_SILENT_MEMBER;
		inm->in6m_timer = 0;
	} else {
		  
                                               
                                              
                                                   
                                                   
                                                  
                                          
     
		if (mli->mli_version == MLD_VERSION_2 &&
		    inm->in6m_state == MLD_LEAVING_MEMBER) {
			VERIFY(inm->in6m_nrelecnt != 0);
			SLIST_INSERT_HEAD(&mli->mli_relinmhead, inm,
			    in6m_nrele);
		}

		inm->in6m_state = MLD_REPORTING_MEMBER;

		switch (mli->mli_version) {
		case MLD_VERSION_1:
			  
                                             
                                           
                                            
                                                
                                             
      
			odelay = MLD_RANDOM_DELAY(MLD_V1_MAX_RI);
			if (delay) {
				inm->in6m_timer = max(delay, odelay);
				mtp->cst = 1;
			} else {
				inm->in6m_state = MLD_IDLE_MEMBER;
				error = mld_v1_transmit_report(inm,
				    MLD_LISTENER_REPORT);

				IN6M_LOCK_ASSERT_HELD(inm);
				MLI_LOCK_ASSERT_HELD(mli);

				if (error == 0) {
					inm->in6m_timer = odelay;
					mtp->cst = 1;
				}
			}
			break;

		case MLD_VERSION_2:
			  
                                                    
                                               
      
			syncstates = 0;

			  
                                                   
                                                   
                                                      
                              
      
			ifq = &inm->in6m_scq;
			IF_DRAIN(ifq);
			retval = mld_v2_enqueue_group_record(ifq, inm, 1,
			    0, 0, (mli->mli_flags & MLIF_USEALLOW));
			mtp->cst = (ifq->ifq_len > 0);
			MLD_PRINTF(("%s: enqueue record = %d\n",
			    __func__, retval));
			if (retval <= 0) {
				error = retval * -1;
				break;
			}

			  
                                                   
                                                    
                                                    
                                                    
     
                                                    
                                                     
      
			VERIFY(mli->mli_rv > 1);
			inm->in6m_scrv = (uint16_t)mli->mli_rv;
			if (delay) {
				if (inm->in6m_sctimer > 1) {
					inm->in6m_sctimer =
					    MIN(inm->in6m_sctimer, (uint16_t)delay);
				} else {
					inm->in6m_sctimer = (uint16_t)delay;
				}
			} else {
				inm->in6m_sctimer = 1;
			}
			mtp->sct = 1;
			error = 0;
			break;
		}
	}
	MLI_UNLOCK(mli);

	  
                                                       
                                                            
                                                            
    
	if (syncstates) {
		in6m_commit(inm);
		MLD_PRINTF(("%s: T1 -> T0 for %s/%s\n", __func__,
		    ip6_sprintf(&inm->in6m_addr),
		    if_name(inm->in6m_ifp)));
	}

	return error;
}

  
                                                            
   
static int
mld_handle_state_change(struct in6_multi *inm, struct mld_ifinfo *mli,
    struct mld_tparams *mtp)
{
	struct ifnet            *ifp;
	int                      retval = 0;

	IN6M_LOCK_ASSERT_HELD(inm);
	MLI_LOCK_ASSERT_NOTHELD(mli);
	VERIFY(mtp != NULL);

	MLD_PRINTF(("%s: state change for %s on ifp 0x%llx(%s)\n",
	    __func__, ip6_sprintf(&inm->in6m_addr),
	    (uint64_t)VM_KERNEL_ADDRPERM(inm->in6m_ifp),
	    if_name(inm->in6m_ifp)));

	ifp = inm->in6m_ifp;

	MLI_LOCK(mli);
	VERIFY(mli->mli_ifp == ifp);

	if ((ifp->if_flags & IFF_LOOPBACK) ||
	    (mli->mli_flags & MLIF_SILENT) ||
	    !mld_is_addr_reported(&inm->in6m_addr) ||
	    (mli->mli_version != MLD_VERSION_2)) {
		MLI_UNLOCK(mli);
		if (!mld_is_addr_reported(&inm->in6m_addr)) {
			MLD_PRINTF(("%s: not kicking state machine for silent "
			    "group\n", __func__));
		}
		MLD_PRINTF(("%s: nothing to do\n", __func__));
		in6m_commit(inm);
		MLD_PRINTF(("%s: T1 -> T0 for %s/%s\n", __func__,
		    ip6_sprintf(&inm->in6m_addr),
		    if_name(inm->in6m_ifp)));
		goto done;
	}

	IF_DRAIN(&inm->in6m_scq);

	retval = mld_v2_enqueue_group_record(&inm->in6m_scq, inm, 1, 0, 0,
	    (mli->mli_flags & MLIF_USEALLOW));
	mtp->cst = (inm->in6m_scq.ifq_len > 0);
	MLD_PRINTF(("%s: enqueue record = %d\n", __func__, retval));
	if (retval <= 0) {
		MLI_UNLOCK(mli);
		retval *= -1;
		goto done;
	} else {
		retval = 0;
	}

	  
                                                      
                                
    
	inm->in6m_scrv = (uint16_t)mli->mli_rv;
	inm->in6m_sctimer = 1;
	mtp->sct = 1;
	MLI_UNLOCK(mli);

done:
	return retval;
}

  
                                                   
  
                        
                                                                   
                                                                
                                             
   
static void
mld_final_leave(struct in6_multi *inm, struct mld_ifinfo *mli,
    struct mld_tparams *mtp)
{
	int syncstates = 1;

	IN6M_LOCK_ASSERT_HELD(inm);
	MLI_LOCK_ASSERT_NOTHELD(mli);
	VERIFY(mtp != NULL);

	MLD_PRINTF(("%s: final leave %s on ifp 0x%llx(%s)\n",
	    __func__, ip6_sprintf(&inm->in6m_addr),
	    (uint64_t)VM_KERNEL_ADDRPERM(inm->in6m_ifp),
	    if_name(inm->in6m_ifp)));

	switch (inm->in6m_state) {
	case MLD_NOT_MEMBER:
	case MLD_SILENT_MEMBER:
	case MLD_LEAVING_MEMBER:
		                                          
		MLD_PRINTF(("%s: not kicking state machine for silent group\n",
		    __func__));
		break;
	case MLD_REPORTING_MEMBER:
	case MLD_IDLE_MEMBER:
	case MLD_G_QUERY_PENDING_MEMBER:
	case MLD_SG_QUERY_PENDING_MEMBER:
		MLI_LOCK(mli);
		if (mli->mli_version == MLD_VERSION_1) {
			if (inm->in6m_state == MLD_G_QUERY_PENDING_MEMBER ||
			    inm->in6m_state == MLD_SG_QUERY_PENDING_MEMBER) {
				panic("%s: MLDv2 state reached, not MLDv2 "
				    "mode\n", __func__);
				                
			}
			                                              
			mtp->cst = (mld_v1_transmit_report(inm,
			    MLD_LISTENER_DONE) == 0);

			IN6M_LOCK_ASSERT_HELD(inm);
			MLI_LOCK_ASSERT_HELD(mli);

			inm->in6m_state = MLD_NOT_MEMBER;
		} else if (mli->mli_version == MLD_VERSION_2) {
			  
                                               
                                               
                                              
                                                
      
			IF_DRAIN(&inm->in6m_scq);
			inm->in6m_timer = 0;
			inm->in6m_scrv = (uint16_t)mli->mli_rv;
			MLD_PRINTF(("%s: Leaving %s/%s with %d "
			    "pending retransmissions.\n", __func__,
			    ip6_sprintf(&inm->in6m_addr),
			    if_name(inm->in6m_ifp),
			    inm->in6m_scrv));
			if (inm->in6m_scrv == 0) {
				inm->in6m_state = MLD_NOT_MEMBER;
				inm->in6m_sctimer = 0;
			} else {
				int retval;
				  
                                              
                                         
                                         
                                    
       
				IN6M_ADDREF_LOCKED(inm);
				VERIFY(inm->in6m_debug & IFD_ATTACHED);
				inm->in6m_reqcnt++;
				VERIFY(inm->in6m_reqcnt >= 1);
				inm->in6m_nrelecnt++;
				VERIFY(inm->in6m_nrelecnt != 0);

				retval = mld_v2_enqueue_group_record(
					&inm->in6m_scq, inm, 1, 0, 0,
					(mli->mli_flags & MLIF_USEALLOW));
				mtp->cst = (inm->in6m_scq.ifq_len > 0);
				KASSERT(retval != 0,
				    ("%s: enqueue record = %d\n", __func__,
				    retval));

				inm->in6m_state = MLD_LEAVING_MEMBER;
				inm->in6m_sctimer = 1;
				mtp->sct = 1;
				syncstates = 0;
			}
		}
		MLI_UNLOCK(mli);
		break;
	case MLD_LAZY_MEMBER:
	case MLD_SLEEPING_MEMBER:
	case MLD_AWAKENING_MEMBER:
		                                             
		break;
	}

	if (syncstates) {
		in6m_commit(inm);
		MLD_PRINTF(("%s: T1 -> T0 for %s/%s\n", __func__,
		    ip6_sprintf(&inm->in6m_addr),
		    if_name(inm->in6m_ifp)));
		inm->in6m_st[1].iss_fmode = MCAST_UNDEFINED;
		MLD_PRINTF(("%s: T1 now MCAST_UNDEFINED for 0x%llx/%s\n",
		    __func__, (uint64_t)VM_KERNEL_ADDRPERM(&inm->in6m_addr),
		    if_name(inm->in6m_ifp)));
	}
}

  
                                                           
  
                                                                  
                                                                     
  
                                                                    
                                                                      
                                                                   
                        
                                                  
  
                                                                   
                                                                   
                                       
  
                                                                        
                                                                          
                                                                           
  
                                                                    
                                                                           
  
                                                                        
                                                                 
                              
   
static int
mld_v2_enqueue_group_record(struct ifqueue *ifq, struct in6_multi *inm,
    const int is_state_change, const int is_group_query,
    const int is_source_query, const int use_block_allow)
{
	struct mldv2_record      mr;
	struct mldv2_record     *pmr;
	struct ifnet            *ifp;
	struct ip6_msource      *ims, *nims;
	struct mbuf             *m0, *m, *md;
	int                      error, is_filter_list_change;
	int                      minrec0len, m0srcs, msrcs, nbytes, off;
	int                      record_has_sources;
	int                      now;
	uint8_t                  type;
	uint8_t                  mode;

	IN6M_LOCK_ASSERT_HELD(inm);
	MLI_LOCK_ASSERT_HELD(inm->in6m_mli);

	error = 0;
	ifp = inm->in6m_ifp;
	is_filter_list_change = 0;
	m = NULL;
	m0 = NULL;
	m0srcs = 0;
	msrcs = 0;
	nbytes = 0;
	nims = NULL;
	record_has_sources = 1;
	pmr = NULL;
	type = MLD_DO_NOTHING;
	mode = (uint8_t)inm->in6m_st[1].iss_fmode;

	  
                                                           
                                                         
                                     
    
	if (inm->in6m_st[0].iss_asm > 0 && inm->in6m_st[1].iss_asm > 0 &&
	    inm->in6m_nsrc == 0) {
		record_has_sources = 0;
	}

	if (is_state_change) {
		  
                                 
                                                      
                                         
                                                            
                                                        
                                             
    
                                                        
                                                             
                                                      
    
                                                    
                                                          
                                          
     
		if (mode != inm->in6m_st[0].iss_fmode) {
			if (mode == MCAST_EXCLUDE) {
				MLD_PRINTF(("%s: change to EXCLUDE\n",
				    __func__));
				type = MLD_CHANGE_TO_EXCLUDE_MODE;
			} else {
				MLD_PRINTF(("%s: change to INCLUDE\n",
				    __func__));
				if (use_block_allow) {
					  
           
                                      
                                      
                                          
                                    
                                    
                        
        
					if (mode == MCAST_UNDEFINED) {
						type = MLD_BLOCK_OLD_SOURCES;
					} else {
						type = MLD_ALLOW_NEW_SOURCES;
					}
				} else {
					type = MLD_CHANGE_TO_INCLUDE_MODE;
					if (mode == MCAST_UNDEFINED) {
						record_has_sources = 0;
					}
				}
			}
		} else {
			if (record_has_sources) {
				is_filter_list_change = 1;
			} else {
				type = MLD_DO_NOTHING;
			}
		}
	} else {
		  
                                  
     
		if (mode == MCAST_EXCLUDE) {
			type = MLD_MODE_IS_EXCLUDE;
		} else if (mode == MCAST_INCLUDE) {
			type = MLD_MODE_IS_INCLUDE;
			VERIFY(inm->in6m_st[1].iss_asm == 0);
		}
	}

	  
                                                               
    
	if (is_filter_list_change) {
		return mld_v2_enqueue_filter_change(ifq, inm);
	}

	if (type == MLD_DO_NOTHING) {
		MLD_PRINTF(("%s: nothing to do for %s/%s\n",
		    __func__, ip6_sprintf(&inm->in6m_addr),
		    if_name(inm->in6m_ifp)));
		return 0;
	}

	  
                                                               
                                                        
                 
    
	minrec0len = sizeof(struct mldv2_record);
	if (record_has_sources) {
		minrec0len += sizeof(struct in6_addr);
	}
	MLD_PRINTF(("%s: queueing %s for %s/%s\n", __func__,
	    mld_rec_type_to_str(type),
	    ip6_sprintf(&inm->in6m_addr),
	    if_name(inm->in6m_ifp)));

	  
                                                               
                                                                    
                                    
                                                           
                                                              
                        
    
	m0 = ifq->ifq_tail;
	if (!is_group_query &&
	    m0 != NULL &&
	    (m0->m_pkthdr.vt_nrecs + 1 <= MLD_V2_REPORT_MAXRECS) &&
	    (m0->m_pkthdr.len + minrec0len) <
	    (ifp->if_mtu - MLD_MTUSPACE)) {
		m0srcs = (ifp->if_mtu - m0->m_pkthdr.len -
		    sizeof(struct mldv2_record)) /
		    sizeof(struct in6_addr);
		m = m0;
		MLD_PRINTF(("%s: use existing packet\n", __func__));
	} else {
		if (IF_QFULL(ifq)) {
			MLD_PRINTF(("%s: outbound queue full\n", __func__));
			return -ENOMEM;
		}
		m = NULL;
		m0srcs = (ifp->if_mtu - MLD_MTUSPACE -
		    sizeof(struct mldv2_record)) / sizeof(struct in6_addr);
		if (!is_state_change && !is_group_query) {
			m = m_getcl(M_DONTWAIT, MT_DATA, M_PKTHDR);
		}
		if (m == NULL) {
			m = m_gethdr(M_DONTWAIT, MT_DATA);
		}
		if (m == NULL) {
			return -ENOMEM;
		}

		mld_save_context(m, ifp);

		MLD_PRINTF(("%s: allocated first packet\n", __func__));
	}

	  
                        
                                                   
    
	mr.mr_type = type;
	mr.mr_datalen = 0;
	mr.mr_numsrc = 0;
	mr.mr_addr = inm->in6m_addr;
	in6_clearscope(&mr.mr_addr);
	if (!m_append(m, sizeof(struct mldv2_record), (void *)&mr)) {
		if (m != m0) {
			m_freem(m);
		}
		MLD_PRINTF(("%s: m_append() failed.\n", __func__));
		return -ENOMEM;
	}
	nbytes += sizeof(struct mldv2_record);

	  
                                                           
                                                             
                                                              
                                                                
                                                                 
                       
   
                                                          
                                                            
                                                           
                                                            
   
                                                                  
                            
    
	if (record_has_sources) {
		if (m == m0) {
			md = m_last(m);
			pmr = (struct mldv2_record *)(mtod(md, uint8_t *) +
			    md->m_len - nbytes);
		} else {
			md = m_getptr(m, 0, &off);
			pmr = (struct mldv2_record *)(mtod(md, uint8_t *) +
			    off);
		}
		msrcs = 0;
		RB_FOREACH_SAFE(ims, ip6_msource_tree, &inm->in6m_srcs,
		    nims) {
			MLD_PRINTF(("%s: visit node %s\n", __func__,
			    ip6_sprintf(&ims->im6s_addr)));
			now = im6s_get_mode(inm, ims, 1);
			MLD_PRINTF(("%s: node is %d\n", __func__, now));
			if ((now != mode) ||
			    (now == mode &&
			    (!use_block_allow && mode == MCAST_UNDEFINED))) {
				MLD_PRINTF(("%s: skip node\n", __func__));
				continue;
			}
			if (is_source_query && ims->im6s_stp == 0) {
				MLD_PRINTF(("%s: skip unrecorded node\n",
				    __func__));
				continue;
			}
			MLD_PRINTF(("%s: append node\n", __func__));
			if (!m_append(m, sizeof(struct in6_addr),
			    (void *)&ims->im6s_addr)) {
				if (m != m0) {
					m_freem(m);
				}
				MLD_PRINTF(("%s: m_append() failed.\n",
				    __func__));
				return -ENOMEM;
			}
			nbytes += sizeof(struct in6_addr);
			++msrcs;
			if (msrcs == m0srcs) {
				break;
			}
		}
		MLD_PRINTF(("%s: msrcs is %d this packet\n", __func__,
		    msrcs));
		pmr->mr_numsrc = htons((uint16_t)msrcs);
		nbytes += (msrcs * sizeof(struct in6_addr));
	}

	if (is_source_query && msrcs == 0) {
		MLD_PRINTF(("%s: no recorded sources to report\n", __func__));
		if (m != m0) {
			m_freem(m);
		}
		return 0;
	}

	  
                                        
    
	if (m != m0) {
		MLD_PRINTF(("%s: enqueueing first packet\n", __func__));
		m->m_pkthdr.vt_nrecs = 1;
		IF_ENQUEUE(ifq, m);
	} else {
		m->m_pkthdr.vt_nrecs++;
	}
	  
                                                          
    
	if (!record_has_sources) {
		return nbytes;
	}

	  
                                                              
                                                          
                                   
    
	while (nims != NULL) {
		if (IF_QFULL(ifq)) {
			MLD_PRINTF(("%s: outbound queue full\n", __func__));
			return -ENOMEM;
		}
		m = m_getcl(M_DONTWAIT, MT_DATA, M_PKTHDR);
		if (m == NULL) {
			m = m_gethdr(M_DONTWAIT, MT_DATA);
		}
		if (m == NULL) {
			return -ENOMEM;
		}
		mld_save_context(m, ifp);
		md = m_getptr(m, 0, &off);
		pmr = (struct mldv2_record *)(mtod(md, uint8_t *) + off);
		MLD_PRINTF(("%s: allocated next packet\n", __func__));

		if (!m_append(m, sizeof(struct mldv2_record), (void *)&mr)) {
			if (m != m0) {
				m_freem(m);
			}
			MLD_PRINTF(("%s: m_append() failed.\n", __func__));
			return -ENOMEM;
		}
		m->m_pkthdr.vt_nrecs = 1;
		nbytes += sizeof(struct mldv2_record);

		m0srcs = (ifp->if_mtu - MLD_MTUSPACE -
		    sizeof(struct mldv2_record)) / sizeof(struct in6_addr);

		msrcs = 0;
		RB_FOREACH_FROM(ims, ip6_msource_tree, nims) {
			MLD_PRINTF(("%s: visit node %s\n",
			    __func__, ip6_sprintf(&ims->im6s_addr)));
			now = im6s_get_mode(inm, ims, 1);
			if ((now != mode) ||
			    (now == mode &&
			    (!use_block_allow && mode == MCAST_UNDEFINED))) {
				MLD_PRINTF(("%s: skip node\n", __func__));
				continue;
			}
			if (is_source_query && ims->im6s_stp == 0) {
				MLD_PRINTF(("%s: skip unrecorded node\n",
				    __func__));
				continue;
			}
			MLD_PRINTF(("%s: append node\n", __func__));
			if (!m_append(m, sizeof(struct in6_addr),
			    (void *)&ims->im6s_addr)) {
				if (m != m0) {
					m_freem(m);
				}
				MLD_PRINTF(("%s: m_append() failed.\n",
				    __func__));
				return -ENOMEM;
			}
			++msrcs;
			if (msrcs == m0srcs) {
				break;
			}
		}
		pmr->mr_numsrc = htons((uint16_t)msrcs);
		nbytes += (msrcs * sizeof(struct in6_addr));

		MLD_PRINTF(("%s: enqueueing next packet\n", __func__));
		IF_ENQUEUE(ifq, m);
	}

	return nbytes;
}

  
                                            
                                                          
                                                
   
typedef enum {
	REC_NONE = 0x00,                             
	REC_ALLOW = 0x01,                          
	REC_BLOCK = 0x02,                          
	REC_FULL = REC_ALLOW | REC_BLOCK
} rectype_t;

  
                                                                 
  
                                                                       
                                                                       
                                                                  
                                                           
  
                                                                        
                                                                      
                                                        
  
                                                                           
                                                                         
                                                                      
                
  
                                                                        
                                                                 
                              
   
static int
mld_v2_enqueue_filter_change(struct ifqueue *ifq, struct in6_multi *inm)
{
	static const int MINRECLEN =
	    sizeof(struct mldv2_record) + sizeof(struct in6_addr);
	struct ifnet            *ifp;
	struct mldv2_record      mr;
	struct mldv2_record     *pmr;
	struct ip6_msource      *ims, *nims;
	struct mbuf             *m, *m0, *md;
	int                      m0srcs, nbytes, npbytes, off, rsrcs, schanged;
	int                      nallow, nblock;
	uint8_t                  mode, now, then;
	rectype_t                crt, drt, nrt;

	IN6M_LOCK_ASSERT_HELD(inm);

	if (inm->in6m_nsrc == 0 ||
	    (inm->in6m_st[0].iss_asm > 0 && inm->in6m_st[1].iss_asm > 0)) {
		return 0;
	}

	ifp = inm->in6m_ifp;                                   
	mode = (uint8_t)inm->in6m_st[1].iss_fmode;                              
	crt = REC_NONE;                                
	drt = REC_NONE;                                           
	nrt = REC_NONE;                                   
	m0srcs = 0;                                                        
	npbytes = 0;                                         
	nbytes = 0;                                                            
	rsrcs = 0;                                               
	schanged = 0;                                                 
	nallow = 0;                                           
	nblock = 0;                                           
	nims = NULL;                                

	  
                                         
                                                        
                                                   
                                                                  
                                              
    
	while (drt != REC_FULL) {
		do {
			m0 = ifq->ifq_tail;
			if (m0 != NULL &&
			    (m0->m_pkthdr.vt_nrecs + 1 <=
			    MLD_V2_REPORT_MAXRECS) &&
			    (m0->m_pkthdr.len + MINRECLEN) <
			    (ifp->if_mtu - MLD_MTUSPACE)) {
				m = m0;
				m0srcs = (ifp->if_mtu - m0->m_pkthdr.len -
				    sizeof(struct mldv2_record)) /
				    sizeof(struct in6_addr);
				MLD_PRINTF(("%s: use previous packet\n",
				    __func__));
			} else {
				m = m_getcl(M_DONTWAIT, MT_DATA, M_PKTHDR);
				if (m == NULL) {
					m = m_gethdr(M_DONTWAIT, MT_DATA);
				}
				if (m == NULL) {
					MLD_PRINTF(("%s: m_get*() failed\n",
					    __func__));
					return -ENOMEM;
				}
				m->m_pkthdr.vt_nrecs = 0;
				mld_save_context(m, ifp);
				m0srcs = (ifp->if_mtu - MLD_MTUSPACE -
				    sizeof(struct mldv2_record)) /
				    sizeof(struct in6_addr);
				npbytes = 0;
				MLD_PRINTF(("%s: allocated new packet\n",
				    __func__));
			}
			  
                                               
                                 
                                                
                                                
                            
      
			memset(&mr, 0, sizeof(mr));
			mr.mr_addr = inm->in6m_addr;
			in6_clearscope(&mr.mr_addr);
			if (!m_append(m, sizeof(mr), (void *)&mr)) {
				if (m != m0) {
					m_freem(m);
				}
				MLD_PRINTF(("%s: m_append() failed\n",
				    __func__));
				return -ENOMEM;
			}
			npbytes += sizeof(struct mldv2_record);
			if (m != m0) {
				                                 
				md = m_getptr(m, npbytes -
				    sizeof(struct mldv2_record), &off);
				pmr = (struct mldv2_record *)(mtod(md,
				    uint8_t *) + off);
			} else {
				                                             
				md = m_last(m);
				pmr = (struct mldv2_record *)(mtod(md,
				    uint8_t *) + md->m_len -
				    sizeof(struct mldv2_record));
			}
			  
                                                 
                                              
                                                    
                                       
                                                    
                                               
                                       
      
			rsrcs = 0;
			if (nims == NULL) {
				nims = RB_MIN(ip6_msource_tree,
				    &inm->in6m_srcs);
			}
			RB_FOREACH_FROM(ims, ip6_msource_tree, nims) {
				MLD_PRINTF(("%s: visit node %s\n", __func__,
				    ip6_sprintf(&ims->im6s_addr)));
				now = im6s_get_mode(inm, ims, 1);
				then = im6s_get_mode(inm, ims, 0);
				MLD_PRINTF(("%s: mode: t0 %d, t1 %d\n",
				    __func__, then, now));
				if (now == then) {
					MLD_PRINTF(("%s: skip unchanged\n",
					    __func__));
					continue;
				}
				if (mode == MCAST_EXCLUDE &&
				    now == MCAST_INCLUDE) {
					MLD_PRINTF(("%s: skip IN src on EX "
					    "group\n", __func__));
					continue;
				}
				nrt = (rectype_t)now;
				if (nrt == REC_NONE) {
					nrt = (rectype_t)(~mode & REC_FULL);
				}
				if (schanged++ == 0) {
					crt = nrt;
				} else if (crt != nrt) {
					continue;
				}
				if (!m_append(m, sizeof(struct in6_addr),
				    (void *)&ims->im6s_addr)) {
					if (m != m0) {
						m_freem(m);
					}
					MLD_PRINTF(("%s: m_append() failed\n",
					    __func__));
					return -ENOMEM;
				}
				nallow += !!(crt == REC_ALLOW);
				nblock += !!(crt == REC_BLOCK);
				if (++rsrcs == m0srcs) {
					break;
				}
			}
			  
                                                 
                                    
      
			if (rsrcs == 0) {
				npbytes -= sizeof(struct mldv2_record);
				if (m != m0) {
					MLD_PRINTF(("%s: m_free(m)\n",
					    __func__));
					m_freem(m);
				} else {
					MLD_PRINTF(("%s: m_adj(m, -mr)\n",
					    __func__));
					m_adj(m, -((int)sizeof(
						    struct mldv2_record)));
				}
				continue;
			}
			npbytes += (rsrcs * sizeof(struct in6_addr));
			if (crt == REC_ALLOW) {
				pmr->mr_type = MLD_ALLOW_NEW_SOURCES;
			} else if (crt == REC_BLOCK) {
				pmr->mr_type = MLD_BLOCK_OLD_SOURCES;
			}
			pmr->mr_numsrc = htons((uint16_t)rsrcs);
			  
                                                  
                                         
      
			m->m_pkthdr.vt_nrecs++;
			if (m != m0) {
				IF_ENQUEUE(ifq, m);
			}
			nbytes += npbytes;
		} while (nims != NULL);
		drt |= crt;
		crt = (~crt & REC_FULL);
	}

	MLD_PRINTF(("%s: queued %d ALLOW_NEW, %d BLOCK_OLD\n", __func__,
	    nallow, nblock));

	return nbytes;
}

static int
mld_v2_merge_state_changes(struct in6_multi *inm, struct ifqueue *ifscq)
{
	struct ifqueue  *gq;
	struct mbuf     *m;                                       
	struct mbuf     *m0;                                              
	struct mbuf     *mt;                                             
	struct mbuf     *n;
	int              docopy, domerge;
	u_int            recslen;

	IN6M_LOCK_ASSERT_HELD(inm);

	docopy = 0;
	domerge = 0;
	recslen = 0;

	  
                                                                 
                                                            
    
	if (inm->in6m_scrv > 0) {
		docopy = 1;
	}

	gq = &inm->in6m_scq;
#ifdef MLD_DEBUG
	if (gq->ifq_head == NULL) {
		MLD_PRINTF(("%s: WARNING: queue for inm 0x%llx is empty\n",
		    __func__, (uint64_t)VM_KERNEL_ADDRPERM(inm)));
	}
#endif

	  
                                                              
                                                          
    
	m = gq->ifq_head;
	while (m != NULL) {
		  
                                                     
                                                        
                                                  
                                                              
                                                      
                        
     
		domerge = 0;
		mt = ifscq->ifq_tail;
		if (mt != NULL) {
			recslen = m_length(m);

			if ((mt->m_pkthdr.vt_nrecs +
			    m->m_pkthdr.vt_nrecs <=
			    MLD_V2_REPORT_MAXRECS) &&
			    (mt->m_pkthdr.len + recslen <=
			    (inm->in6m_ifp->if_mtu - MLD_MTUSPACE))) {
				domerge = 1;
			}
		}

		if (!domerge && IF_QFULL(gq)) {
			MLD_PRINTF(("%s: outbound queue full, skipping whole "
			    "packet 0x%llx\n", __func__,
			    (uint64_t)VM_KERNEL_ADDRPERM(m)));
			n = m->m_nextpkt;
			if (!docopy) {
				IF_REMQUEUE(gq, m);
				m_freem(m);
			}
			m = n;
			continue;
		}

		if (!docopy) {
			MLD_PRINTF(("%s: dequeueing 0x%llx\n", __func__,
			    (uint64_t)VM_KERNEL_ADDRPERM(m)));
			n = m->m_nextpkt;
			IF_REMQUEUE(gq, m);
			m0 = m;
			m = n;
		} else {
			MLD_PRINTF(("%s: copying 0x%llx\n", __func__,
			    (uint64_t)VM_KERNEL_ADDRPERM(m)));
			m0 = m_dup(m, M_NOWAIT);
			if (m0 == NULL) {
				return ENOMEM;
			}
			m0->m_nextpkt = NULL;
			m = m->m_nextpkt;
		}

		if (!domerge) {
			MLD_PRINTF(("%s: queueing 0x%llx to ifscq 0x%llx)\n",
			    __func__, (uint64_t)VM_KERNEL_ADDRPERM(m0),
			    (uint64_t)VM_KERNEL_ADDRPERM(ifscq)));
			IF_ENQUEUE(ifscq, m0);
		} else {
			struct mbuf *mtl;                                   

			MLD_PRINTF(("%s: merging 0x%llx with ifscq tail "
			    "0x%llx)\n", __func__,
			    (uint64_t)VM_KERNEL_ADDRPERM(m0),
			    (uint64_t)VM_KERNEL_ADDRPERM(mt)));

			mtl = m_last(mt);
			m0->m_flags &= ~M_PKTHDR;
			mt->m_pkthdr.len += recslen;
			mt->m_pkthdr.vt_nrecs +=
			    m0->m_pkthdr.vt_nrecs;

			mtl->m_next = m0;
		}
	}

	return 0;
}

  
                                            
   
static uint32_t
mld_v2_dispatch_general_query(struct mld_ifinfo *mli)
{
	struct ifnet            *ifp;
	struct in6_multi        *inm;
	struct in6_multistep    step;
	int                      retval;

	MLI_LOCK_ASSERT_HELD(mli);

	VERIFY(mli->mli_version == MLD_VERSION_2);

	ifp = mli->mli_ifp;
	MLI_UNLOCK(mli);

	in6_multihead_lock_shared();
	IN6_FIRST_MULTI(step, inm);
	while (inm != NULL) {
		IN6M_LOCK(inm);
		if (inm->in6m_ifp != ifp) {
			goto next;
		}

		switch (inm->in6m_state) {
		case MLD_NOT_MEMBER:
		case MLD_SILENT_MEMBER:
			break;
		case MLD_REPORTING_MEMBER:
		case MLD_IDLE_MEMBER:
		case MLD_LAZY_MEMBER:
		case MLD_SLEEPING_MEMBER:
		case MLD_AWAKENING_MEMBER:
			inm->in6m_state = MLD_REPORTING_MEMBER;
			MLI_LOCK(mli);
			retval = mld_v2_enqueue_group_record(&mli->mli_gq,
			    inm, 0, 0, 0, 0);
			MLI_UNLOCK(mli);
			MLD_PRINTF(("%s: enqueue record = %d\n",
			    __func__, retval));
			break;
		case MLD_G_QUERY_PENDING_MEMBER:
		case MLD_SG_QUERY_PENDING_MEMBER:
		case MLD_LEAVING_MEMBER:
			break;
		}
next:
		IN6M_UNLOCK(inm);
		IN6_NEXT_MULTI(step, inm);
	}
	in6_multihead_lock_done();

	MLI_LOCK(mli);
	mld_dispatch_queue_locked(mli, &mli->mli_gq, MLD_MAX_RESPONSE_BURST);
	MLI_LOCK_ASSERT_HELD(mli);

	  
                                                        
    
	if (mli->mli_gq.ifq_head != NULL) {
		mli->mli_v2_timer = 1 + MLD_RANDOM_DELAY(
			MLD_RESPONSE_BURST_INTERVAL);
	}

	return mli->mli_v2_timer;
}

  
                                                         
  
                                                       
   
static void
mld_dispatch_packet(struct mbuf *m)
{
	struct ip6_moptions     *im6o;
	struct ifnet            *ifp;
	struct ifnet            *oifp = NULL;
	struct mbuf             *m0;
	struct mbuf             *md;
	struct ip6_hdr          *ip6;
	struct mld_hdr          *mld;
	int                      error;
	int                      off;
	int                      type;

	MLD_PRINTF(("%s: transmit 0x%llx\n", __func__,
	    (uint64_t)VM_KERNEL_ADDRPERM(m)));

	  
                                         
    
	ifp = mld_restore_context(m);
	if (ifp == NULL || !ifnet_is_attached(ifp, 0)) {
		MLD_PRINTF(("%s: dropped 0x%llx as ifindex %u went away.\n",
		    __func__, (uint64_t)VM_KERNEL_ADDRPERM(m),
		    (u_int)if_index));
		m_freem(m);
		ip6stat.ip6s_noroute++;
		return;
	}

	im6o = ip6_allocmoptions(Z_WAITOK);
	if (im6o == NULL) {
		m_freem(m);
		return;
	}

	im6o->im6o_multicast_hlim  = 1;
	im6o->im6o_multicast_loop = 0;
	im6o->im6o_multicast_ifp = ifp;

	if (m->m_flags & M_MLDV1) {
		m0 = m;
	} else {
		m0 = mld_v2_encap_report(ifp, m);
		if (m0 == NULL) {
			MLD_PRINTF(("%s: dropped 0x%llx\n", __func__,
			    (uint64_t)VM_KERNEL_ADDRPERM(m)));
			  
                                                       
      
			IM6O_REMREF(im6o);
			ip6stat.ip6s_odropped++;
			return;
		}
	}

	mld_scrub_context(m0);
	m->m_flags &= ~(M_PROTOFLAGS);
	m0->m_pkthdr.rcvif = lo_ifp;

	ip6 = mtod(m0, struct ip6_hdr *);
	(void)in6_setscope(&ip6->ip6_dst, ifp, NULL);

	  
                                                            
                             
    
	md = m_getptr(m0, sizeof(struct ip6_hdr), &off);
	mld = (struct mld_hdr *)(mtod(md, uint8_t *) + off);
	type = mld->mld_type;

	if (ifp->if_eflags & IFEF_TXSTART) {
		  
                                              
                                             
     
		(void) m_set_service_class(m0, MBUF_SC_CTL);
	}

	error = ip6_output(m0, &mld_po, NULL, IPV6_UNSPECSRC, im6o,
	    &oifp, NULL);

	IM6O_REMREF(im6o);

	if (error) {
		MLD_PRINTF(("%s: ip6_output(0x%llx) = %d\n", __func__,
		    (uint64_t)VM_KERNEL_ADDRPERM(m0), error));
		if (oifp != NULL) {
			ifnet_release(oifp);
		}
		return;
	}

	icmp6stat.icp6s_outhist[type]++;
	if (oifp != NULL) {
		icmp6_ifstat_inc(oifp, ifs6_out_msg);
		switch (type) {
		case MLD_LISTENER_REPORT:
		case MLDV2_LISTENER_REPORT:
			icmp6_ifstat_inc(oifp, ifs6_out_mldreport);
			break;
		case MLD_LISTENER_DONE:
			icmp6_ifstat_inc(oifp, ifs6_out_mlddone);
			break;
		}
		ifnet_release(oifp);
	}
}

  
                               
  
                                                                   
                                                            
  
                                                               
                     
   
static struct mbuf *
mld_v2_encap_report(struct ifnet *ifp, struct mbuf *m)
{
	struct mbuf             *mh;
	struct mldv2_report     *mld;
	struct ip6_hdr          *ip6;
	struct in6_ifaddr       *ia;
	int                      mldreclen;

	VERIFY(m->m_flags & M_PKTHDR);

	  
                                                      
    
	ia = in6ifa_ifpforlinklocal(ifp, IN6_IFF_NOTREADY | IN6_IFF_ANYCAST);
	if (ia == NULL) {
		MLD_PRINTF(("%s: warning: ia is NULL\n", __func__));
	}

	MGETHDR(mh, M_DONTWAIT, MT_HEADER);
	if (mh == NULL) {
		if (ia != NULL) {
			IFA_REMREF(&ia->ia_ifa);
		}
		m_freem(m);
		return NULL;
	}
	MH_ALIGN(mh, sizeof(struct ip6_hdr) + sizeof(struct mldv2_report));

	mldreclen = m_length(m);
	MLD_PRINTF(("%s: mldreclen is %d\n", __func__, mldreclen));

	mh->m_len = sizeof(struct ip6_hdr) + sizeof(struct mldv2_report);
	mh->m_pkthdr.len = sizeof(struct ip6_hdr) +
	    sizeof(struct mldv2_report) + mldreclen;

	ip6 = mtod(mh, struct ip6_hdr *);
	ip6->ip6_flow = 0;
	ip6->ip6_vfc &= ~IPV6_VERSION_MASK;
	ip6->ip6_vfc |= IPV6_VERSION;
	ip6->ip6_nxt = IPPROTO_ICMPV6;
	if (ia != NULL) {
		IFA_LOCK(&ia->ia_ifa);
	}
	ip6->ip6_src = ia ? ia->ia_addr.sin6_addr : in6addr_any;
	if (ia != NULL) {
		IFA_UNLOCK(&ia->ia_ifa);
		IFA_REMREF(&ia->ia_ifa);
		ia = NULL;
	}
	ip6->ip6_dst = in6addr_linklocal_allv2routers;
	                                    

	mld = (struct mldv2_report *)(ip6 + 1);
	mld->mld_type = MLDV2_LISTENER_REPORT;
	mld->mld_code = 0;
	mld->mld_cksum = 0;
	mld->mld_v2_reserved = 0;
	mld->mld_v2_numrecs = htons(m->m_pkthdr.vt_nrecs);
	m->m_pkthdr.vt_nrecs = 0;
	m->m_flags &= ~M_PKTHDR;

	mh->m_next = m;
	mld->mld_cksum = in6_cksum(mh, IPPROTO_ICMPV6,
	    sizeof(struct ip6_hdr), sizeof(struct mldv2_report) + mldreclen);
	return mh;
}

#ifdef MLD_DEBUG
static const char *
mld_rec_type_to_str(const int type)
{
	switch (type) {
	case MLD_CHANGE_TO_EXCLUDE_MODE:
		return "TO_EX";
	case MLD_CHANGE_TO_INCLUDE_MODE:
		return "TO_IN";
	case MLD_MODE_IS_EXCLUDE:
		return "MODE_EX";
	case MLD_MODE_IS_INCLUDE:
		return "MODE_IN";
	case MLD_ALLOW_NEW_SOURCES:
		return "ALLOW_NEW";
	case MLD_BLOCK_OLD_SOURCES:
		return "BLOCK_OLD";
	default:
		break;
	}
	return "unknown";
}
#endif

void
mld_init(void)
{
	MLD_PRINTF(("%s: initializing\n", __func__));

	                                                
	mld_mtx_grp_attr = lck_grp_attr_alloc_init();
	mld_mtx_grp = lck_grp_alloc_init("mld_mtx\n", mld_mtx_grp_attr);
	mld_mtx_attr = lck_attr_alloc_init();
	lck_mtx_init(&mld_mtx, mld_mtx_grp, mld_mtx_attr);

	ip6_initpktopts(&mld_po);
	mld_po.ip6po_hlim = 1;
	mld_po.ip6po_hbh = &mld_ra.hbh;
	mld_po.ip6po_prefer_tempaddr = IP6PO_TEMPADDR_NOTPREFER;
	mld_po.ip6po_flags = IP6PO_DONTFRAG;
	LIST_INIT(&mli_head);
}
