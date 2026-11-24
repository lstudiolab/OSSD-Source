/*
 * Copyright (c) 2010-2020 Apple Inc. All rights reserved.
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
/*-
 * Copyright (c) 2007-2009 Bruce Simpson.
 * Copyright (c) 2005 Robert N. M. Watson.
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

  
                                                                     
   

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>
#include <sys/tree.h>
#include <sys/mcache.h>

#include <kern/zalloc.h>

#include <pexpert/pexpert.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/net_api_stats.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/igmp_var.h>

  
                                                                   
                        
                      
                 
                 
                  
                   
                
                      
                     
                     
  
                                                                        
                     
   
static void     imf_commit(struct in_mfilter *);
static int      imf_get_source(struct in_mfilter *imf,
    const struct sockaddr_in *psin,
    struct in_msource **);
static struct in_msource *
imf_graft(struct in_mfilter *, const uint8_t,
    const struct sockaddr_in *);
static int      imf_prune(struct in_mfilter *, const struct sockaddr_in *);
static void     imf_rollback(struct in_mfilter *);
static void     imf_reap(struct in_mfilter *);
static int      imo_grow(struct ip_moptions *, uint16_t);
static size_t   imo_match_group(const struct ip_moptions *,
    const struct ifnet *, const struct sockaddr_in *);
static struct in_msource *
imo_match_source(const struct ip_moptions *, const size_t,
    const struct sockaddr_in *);
static void     ims_merge(struct ip_msource *ims,
    const struct in_msource *lims, const int rollback);
static int      in_getmulti(struct ifnet *, const struct in_addr *,
    struct in_multi **);
static int      in_joingroup(struct ifnet *, const struct in_addr *,
    struct in_mfilter *, struct in_multi **);
static int      inm_get_source(struct in_multi *inm, const in_addr_t haddr,
    const int noalloc, struct ip_msource **pims);
static int      inm_is_ifp_detached(const struct in_multi *);
static int      inm_merge(struct in_multi *,           struct in_mfilter *);
static void     inm_reap(struct in_multi *);
static struct ip_moptions *
inp_findmoptions(struct inpcb *);
static int      inp_get_source_filters(struct inpcb *, struct sockopt *);
static struct ifnet *
inp_lookup_mcast_ifp(const struct inpcb *,
    const struct sockaddr_in *, const struct in_addr);
static int      inp_block_unblock_source(struct inpcb *, struct sockopt *);
static int      inp_set_multicast_if(struct inpcb *, struct sockopt *);
static int      inp_set_source_filters(struct inpcb *, struct sockopt *);
static int      sysctl_ip_mcast_filters SYSCTL_HANDLER_ARGS;
static struct ifnet * ip_multicast_if(struct in_addr *, unsigned int *);
static __inline__ int ip_msource_cmp(const struct ip_msource *,
    const struct ip_msource *);

SYSCTL_NODE(_net_inet_ip, OID_AUTO, mcast, CTLFLAG_RW | CTLFLAG_LOCKED, 0, "IPv4 multicast");

static u_long in_mcast_maxgrpsrc = IP_MAX_GROUP_SRC_FILTER;
SYSCTL_LONG(_net_inet_ip_mcast, OID_AUTO, maxgrpsrc,
    CTLFLAG_RW | CTLFLAG_LOCKED, &in_mcast_maxgrpsrc, "Max source filters per group");

static u_int in_mcast_maxsocksrc = IP_MAX_SOCK_SRC_FILTER;
SYSCTL_UINT(_net_inet_ip_mcast, OID_AUTO, maxsocksrc,
    CTLFLAG_RW | CTLFLAG_LOCKED, &in_mcast_maxsocksrc, IP_MAX_SOCK_SRC_FILTER,
    "Max source filters per socket");

int in_mcast_loop = IP_DEFAULT_MULTICAST_LOOP;
SYSCTL_INT(_net_inet_ip_mcast, OID_AUTO, loop, CTLFLAG_RW | CTLFLAG_LOCKED,
    &in_mcast_loop, 0, "Loopback multicast datagrams by default");

SYSCTL_NODE(_net_inet_ip_mcast, OID_AUTO, filters,
    CTLFLAG_RD | CTLFLAG_LOCKED, sysctl_ip_mcast_filters,
    "Per-interface stack-wide source filters");

RB_GENERATE_PREV(ip_msource_tree, ip_msource, ims_link, ip_msource_cmp);

#define INM_TRACE_HIST_SIZE     32                                 

             
__private_extern__ unsigned int inm_trace_hist_size = INM_TRACE_HIST_SIZE;

struct in_multi_dbg {
	struct in_multi         inm;                                  
	u_int16_t               inm_refhold_cnt;                      
	u_int16_t               inm_refrele_cnt;                       
	  
                                                        
    
	ctrace_t                inm_refhold[INM_TRACE_HIST_SIZE];
	ctrace_t                inm_refrele[INM_TRACE_HIST_SIZE];
	  
                      
    
	TAILQ_ENTRY(in_multi_dbg) inm_trash_link;
};

                                                                
static TAILQ_HEAD(, in_multi_dbg) inm_trash_head;
static decl_lck_mtx_data(, inm_trash_lock);


#if DEBUG
static unsigned int inm_debug = 1;                                       
#else
static unsigned int inm_debug;                                            
#endif             
#define INM_ZONE_NAME           "in_multi"                     
static struct zone *inm_zone;                                          

static ZONE_DECLARE(ipms_zone, "ip_msource", sizeof(struct ip_msource),
    ZC_ZFREE_CLEARMEM);
static ZONE_DECLARE(inms_zone, "in_msource", sizeof(struct in_msource),
    ZC_ZFREE_CLEARMEM);

                                                         
static lck_attr_t       *in_multihead_lock_attr;
static lck_grp_t        *in_multihead_lock_grp;
static lck_grp_attr_t   *in_multihead_lock_grp_attr;

static decl_lck_rw_data(, in_multihead_lock);
struct in_multihead in_multihead;

static struct in_multi *in_multi_alloc(zalloc_flags_t);
static void in_multi_free(struct in_multi *);
static void in_multi_attach(struct in_multi *);
static void inm_trace(struct in_multi *, int);

static struct ip_msource *ipms_alloc(zalloc_flags_t);
static void ipms_free(struct ip_msource *);
static struct in_msource *inms_alloc(zalloc_flags_t);
static void inms_free(struct in_msource *);

static __inline int
ip_msource_cmp(const struct ip_msource *a, const struct ip_msource *b)
{
	if (a->ims_haddr < b->ims_haddr) {
		return -1;
	}
	if (a->ims_haddr == b->ims_haddr) {
		return 0;
	}
	return 1;
}

  
                                                          
   
static __inline__ int
inm_is_ifp_detached(const struct in_multi *inm)
{
	VERIFY(inm->inm_ifma != NULL);
	VERIFY(inm->inm_ifp == inm->inm_ifma->ifma_ifp);

	return !ifnet_is_attached(inm->inm_ifp, 0);
}

  
                                                                
                                    
   
static __inline__ void
imf_init(struct in_mfilter *imf, const uint8_t st0, const uint8_t st1)
{
	memset(imf, 0, sizeof(struct in_mfilter));
	RB_INIT(&imf->imf_sources);
	imf->imf_st[0] = st0;
	imf->imf_st[1] = st1;
}

  
                                                                  
   
static int
imo_grow(struct ip_moptions *imo, uint16_t newmax)
{
	struct in_multi         **nmships;
	struct in_multi         **omships;
	struct in_mfilter        *nmfilters;
	struct in_mfilter        *omfilters;
	uint16_t                  idx;
	uint16_t                  oldmax;

	IMO_LOCK_ASSERT_HELD(imo);

	nmships = NULL;
	nmfilters = NULL;
	omships = imo->imo_membership;
	omfilters = imo->imo_mfilters;
	oldmax = imo->imo_max_memberships;
	if (newmax == 0) {
		newmax = ((oldmax + 1) * 2) - 1;
	}

	if (newmax > IP_MAX_MEMBERSHIPS) {
		return ETOOMANYREFS;
	}

	if ((nmships = (struct in_multi **)_REALLOC(omships,
	    sizeof(struct in_multi *) * newmax, M_IPMOPTS,
	    M_WAITOK | M_ZERO)) == NULL) {
		return ENOMEM;
	}

	imo->imo_membership = nmships;

	if ((nmfilters = (struct in_mfilter *)_REALLOC(omfilters,
	    sizeof(struct in_mfilter) * newmax, M_INMFILTER,
	    M_WAITOK | M_ZERO)) == NULL) {
		return ENOMEM;
	}

	imo->imo_mfilters = nmfilters;

	                                                     
	for (idx = oldmax; idx < newmax; idx++) {
		imf_init(&nmfilters[idx], MCAST_UNDEFINED, MCAST_EXCLUDE);
	}

	imo->imo_max_memberships = newmax;

	return 0;
}

  
                                                                   
                                                                  
                                                       
   
static size_t
imo_match_group(const struct ip_moptions *imo, const struct ifnet *ifp,
    const struct sockaddr_in *group)
{
	struct in_multi *pinm;
	int               idx;
	int               nmships;

	IMO_LOCK_ASSERT_HELD(__DECONST(struct ip_moptions *, imo));


	                                                     
	if (imo->imo_membership == NULL || imo->imo_num_memberships == 0) {
		return -1;
	}

	nmships = imo->imo_num_memberships;
	for (idx = 0; idx < nmships; idx++) {
		pinm = imo->imo_membership[idx];
		if (pinm == NULL) {
			continue;
		}
		INM_LOCK(pinm);
		if ((ifp == NULL || (pinm->inm_ifp == ifp)) &&
		    in_hosteq(pinm->inm_addr, group->sin_addr)) {
			INM_UNLOCK(pinm);
			break;
		}
		INM_UNLOCK(pinm);
	}
	if (idx >= nmships) {
		idx = -1;
	}

	return idx;
}

  
                                                                 
                                                             
  
                                                               
                                                     
   
static struct in_msource *
imo_match_source(const struct ip_moptions *imo, const size_t gidx,
    const struct sockaddr_in *src)
{
	struct ip_msource        find;
	struct in_mfilter       *imf;
	struct ip_msource       *ims;

	IMO_LOCK_ASSERT_HELD(__DECONST(struct ip_moptions *, imo));

	VERIFY(src->sin_family == AF_INET);
	VERIFY(gidx != (size_t)-1 && gidx < imo->imo_num_memberships);

	                                                   
	if (imo->imo_mfilters == NULL) {
		return NULL;
	}
	imf = &imo->imo_mfilters[gidx];

	                                                
	find.ims_haddr = ntohl(src->sin_addr.s_addr);
	ims = RB_FIND(ip_msource_tree, &imf->imf_sources, &find);

	return (struct in_msource *)ims;
}

  
                                                                             
  
                                                                            
                                                                             
   
int
imo_multi_filter(const struct ip_moptions *imo, const struct ifnet *ifp,
    const struct sockaddr_in *group, const struct sockaddr_in *src)
{
	size_t gidx;
	struct in_msource *ims;
	int mode;

	IMO_LOCK_ASSERT_HELD(__DECONST(struct ip_moptions *, imo));
	VERIFY(ifp != NULL);

	gidx = imo_match_group(imo, ifp, group);
	if (gidx == (size_t)-1) {
		return MCAST_NOTGMEMBER;
	}

	  
                                                      
                                                        
                                                         
                                                            
                                                            
                                                            
                                               
    
	mode = imo->imo_mfilters[gidx].imf_st[1];
	ims = imo_match_source(imo, gidx, src);

	if ((ims == NULL && mode == MCAST_INCLUDE) ||
	    (ims != NULL && ims->imsl_st[0] != mode)) {
		return MCAST_NOTSMEMBER;
	}

	return MCAST_PASS;
}

int
imo_clone(struct inpcb *from_inp, struct inpcb *to_inp)
{
	int i, err = 0;
	struct ip_moptions *from;
	struct ip_moptions *to;

	from = inp_findmoptions(from_inp);
	if (from == NULL) {
		return ENOMEM;
	}

	to = inp_findmoptions(to_inp);
	if (to == NULL) {
		IMO_REMREF(from);
		return ENOMEM;
	}

	IMO_LOCK(from);
	IMO_LOCK(to);

	to->imo_multicast_ifp = from->imo_multicast_ifp;
	to->imo_multicast_vif = from->imo_multicast_vif;
	to->imo_multicast_ttl = from->imo_multicast_ttl;
	to->imo_multicast_loop = from->imo_multicast_loop;

	  
                                                              
                                           
    
	for (i = 0; i < to->imo_num_memberships; ++i) {
		struct in_mfilter *imf;

		imf = to->imo_mfilters ? &to->imo_mfilters[i] : NULL;
		if (imf != NULL) {
			imf_leave(imf);
		}

		(void) in_leavegroup(to->imo_membership[i], imf);

		if (imf != NULL) {
			imf_purge(imf);
		}

		INM_REMREF(to->imo_membership[i]);
		to->imo_membership[i] = NULL;
	}
	to->imo_num_memberships = 0;

	VERIFY(to->imo_max_memberships != 0 && from->imo_max_memberships != 0);
	if (to->imo_max_memberships < from->imo_max_memberships) {
		  
                                                          
                                                          
     
		err = imo_grow(to, from->imo_max_memberships);
		if (err != 0) {
			goto done;
		}
	}
	VERIFY(to->imo_max_memberships >= from->imo_max_memberships);

	  
                                                           
                                                             
    
	for (i = 0; i < from->imo_num_memberships; i++) {
		to->imo_membership[i] =
		    in_addmulti(&from->imo_membership[i]->inm_addr,
		    from->imo_membership[i]->inm_ifp);
		if (to->imo_membership[i] == NULL) {
			break;
		}
		to->imo_num_memberships++;
	}
	VERIFY(to->imo_num_memberships == from->imo_num_memberships);

done:
	IMO_UNLOCK(to);
	IMO_REMREF(to);
	IMO_UNLOCK(from);
	IMO_REMREF(from);

	return err;
}

  
                                                                      
                                
                                                                             
                                      
                                                                      
   
static int
in_getmulti(struct ifnet *ifp, const struct in_addr *group,
    struct in_multi **pinm)
{
	struct sockaddr_in       gsin;
	struct ifmultiaddr      *ifma;
	struct in_multi         *inm;
	int                     error;

	in_multihead_lock_shared();
	IN_LOOKUP_MULTI(group, ifp, inm);
	if (inm != NULL) {
		INM_LOCK(inm);
		VERIFY(inm->inm_reqcnt >= 1);
		inm->inm_reqcnt++;
		VERIFY(inm->inm_reqcnt != 0);
		*pinm = inm;
		INM_UNLOCK(inm);
		in_multihead_lock_done();
		  
                                                 
                                                  
     
		return 0;
	}
	in_multihead_lock_done();

	bzero(&gsin, sizeof(gsin));
	gsin.sin_family = AF_INET;
	gsin.sin_len = sizeof(struct sockaddr_in);
	gsin.sin_addr = *group;

	  
                                                     
                                                     
    
	error = if_addmulti(ifp, (struct sockaddr *)&gsin, &ifma);
	if (error != 0) {
		return error;
	}

	  
                                                              
    
	in_multihead_lock_exclusive();
	IFMA_LOCK(ifma);
	if ((inm = ifma->ifma_protospec) != NULL) {
		VERIFY(ifma->ifma_addr != NULL);
		VERIFY(ifma->ifma_addr->sa_family == AF_INET);
		INM_ADDREF(inm);                        
		IFMA_UNLOCK(ifma);
		INM_LOCK(inm);
		VERIFY(inm->inm_ifma == ifma);
		VERIFY(inm->inm_ifp == ifp);
		VERIFY(in_hosteq(inm->inm_addr, *group));
		if (inm->inm_debug & IFD_ATTACHED) {
			VERIFY(inm->inm_reqcnt >= 1);
			inm->inm_reqcnt++;
			VERIFY(inm->inm_reqcnt != 0);
			*pinm = inm;
			INM_UNLOCK(inm);
			in_multihead_lock_done();
			IFMA_REMREF(ifma);
			  
                                                
                                                 
                                                 
                      
      
			return 0;
		}
		  
                                                              
                                                             
                                                             
                                             
     
		in_multi_attach(inm);
		VERIFY((inm->inm_debug &
		    (IFD_ATTACHED | IFD_TRASHED)) == IFD_ATTACHED);
		*pinm = inm;
		INM_UNLOCK(inm);
		in_multihead_lock_done();
		IFMA_REMREF(ifma);
		return 0;
	}
	IFMA_UNLOCK(ifma);

	  
                                                                
                                                               
                                                            
   
                                                                  
    
	inm = in_multi_alloc(Z_WAITOK);

	INM_LOCK(inm);
	inm->inm_addr = *group;
	inm->inm_ifp = ifp;
	inm->inm_igi = IGMP_IFINFO(ifp);
	VERIFY(inm->inm_igi != NULL);
	IGI_ADDREF(inm->inm_igi);
	inm->inm_ifma = ifma;                                                 
	inm->inm_state = IGMP_NOT_MEMBER;
	  
                                                                  
    
	inm->inm_scq.ifq_maxlen = IGMP_MAX_STATE_CHANGES;
	inm->inm_st[0].iss_fmode = MCAST_UNDEFINED;
	inm->inm_st[1].iss_fmode = MCAST_UNDEFINED;
	RB_INIT(&inm->inm_srcs);
	*pinm = inm;
	in_multi_attach(inm);
	VERIFY((inm->inm_debug & (IFD_ATTACHED | IFD_TRASHED)) == IFD_ATTACHED);
	INM_ADDREF_LOCKED(inm);                         
	INM_UNLOCK(inm);

	IFMA_LOCK(ifma);
	VERIFY(ifma->ifma_protospec == NULL);
	ifma->ifma_protospec = inm;
	IFMA_UNLOCK(ifma);
	in_multihead_lock_done();

	return 0;
}

  
                                             
                         
                      
   
void
inm_clear_recorded(struct in_multi *inm)
{
	struct ip_msource       *ims;

	INM_LOCK_ASSERT_HELD(inm);

	RB_FOREACH(ims, ip_msource_tree, &inm->inm_srcs) {
		if (ims->ims_stp) {
			ims->ims_stp = 0;
			--inm->inm_st[1].iss_rec;
		}
	}
	VERIFY(inm->inm_st[1].iss_rec == 0);
}

  
                                                              
                                                  
  
                               
                                                                      
  
                                                           
                                                          
                                                                
                                                                 
                                                                
                                                     
  
                                                                         
                                                                  
                                                       
   
int
inm_record_source(struct in_multi *inm, const in_addr_t naddr)
{
	struct ip_msource        find;
	struct ip_msource       *ims, *nims;

	INM_LOCK_ASSERT_HELD(inm);

	find.ims_haddr = ntohl(naddr);
	ims = RB_FIND(ip_msource_tree, &inm->inm_srcs, &find);
	if (ims && ims->ims_stp) {
		return 0;
	}
	if (ims == NULL) {
		if (inm->inm_nsrc == in_mcast_maxgrpsrc) {
			return -ENOSPC;
		}
		nims = ipms_alloc(Z_WAITOK);
		nims->ims_haddr = find.ims_haddr;
		RB_INSERT(ip_msource_tree, &inm->inm_srcs, nims);
		++inm->inm_nsrc;
		ims = nims;
	}

	  
                                                       
                 
    
	++ims->ims_stp;
	++inm->inm_st[1].iss_rec;

	return 1;
}

  
                                                            
                            
                                                                      
                   
  
                                        
                                                    
  
                                             
   
static int
imf_get_source(struct in_mfilter *imf, const struct sockaddr_in *psin,
    struct in_msource **plims)
{
	struct ip_msource        find;
	struct ip_msource       *ims;
	struct in_msource       *lims;
	int                      error;

	error = 0;
	ims = NULL;
	lims = NULL;

	                            
	find.ims_haddr = ntohl(psin->sin_addr.s_addr);
	ims = RB_FIND(ip_msource_tree, &imf->imf_sources, &find);
	lims = (struct in_msource *)ims;
	if (lims == NULL) {
		if (imf->imf_nsrc == in_mcast_maxsocksrc) {
			return ENOSPC;
		}
		lims = inms_alloc(Z_WAITOK);
		lims->ims_haddr = find.ims_haddr;
		lims->imsl_st[0] = MCAST_UNDEFINED;
		RB_INSERT(ip_msource_tree, &imf->imf_sources,
		    (struct ip_msource *)lims);
		++imf->imf_nsrc;
	}

	*plims = lims;

	return error;
}

  
                                                                 
                                                                
  
                                                              
  
                                                             
  
                                             
   
static struct in_msource *
imf_graft(struct in_mfilter *imf, const uint8_t st1,
    const struct sockaddr_in *psin)
{
	struct in_msource       *lims;

	lims = inms_alloc(Z_WAITOK);
	lims->ims_haddr = ntohl(psin->sin_addr.s_addr);
	lims->imsl_st[0] = MCAST_UNDEFINED;
	lims->imsl_st[1] = st1;
	RB_INSERT(ip_msource_tree, &imf->imf_sources,
	    (struct ip_msource *)lims);
	++imf->imf_nsrc;

	return lims;
}

  
                                                                 
                                                                
  
                                                             
  
                                                                  
  
                                             
   
static int
imf_prune(struct in_mfilter *imf, const struct sockaddr_in *psin)
{
	struct ip_msource        find;
	struct ip_msource       *ims;
	struct in_msource       *lims;

	                            
	find.ims_haddr = ntohl(psin->sin_addr.s_addr);
	ims = RB_FIND(ip_msource_tree, &imf->imf_sources, &find);
	if (ims == NULL) {
		return ENOENT;
	}
	lims = (struct in_msource *)ims;
	lims->imsl_st[1] = MCAST_UNDEFINED;
	return 0;
}

  
                                                           
  
                                             
   
static void
imf_rollback(struct in_mfilter *imf)
{
	struct ip_msource       *ims, *tims;
	struct in_msource       *lims;

	RB_FOREACH_SAFE(ims, ip_msource_tree, &imf->imf_sources, tims) {
		lims = (struct in_msource *)ims;
		if (lims->imsl_st[0] == lims->imsl_st[1]) {
			                     
			continue;
		} else if (lims->imsl_st[0] != MCAST_UNDEFINED) {
			                                            
			lims->imsl_st[1] = lims->imsl_st[0];
		} else {
			                            
			IGMP_PRINTF(("%s: free inms 0x%llx\n", __func__,
			    (uint64_t)VM_KERNEL_ADDRPERM(lims)));
			RB_REMOVE(ip_msource_tree, &imf->imf_sources, ims);
			inms_free(lims);
			imf->imf_nsrc--;
		}
	}
	imf->imf_st[1] = imf->imf_st[0];
}

  
                                                    
  
                                             
   
void
imf_leave(struct in_mfilter *imf)
{
	struct ip_msource       *ims;
	struct in_msource       *lims;

	RB_FOREACH(ims, ip_msource_tree, &imf->imf_sources) {
		lims = (struct in_msource *)ims;
		lims->imsl_st[1] = MCAST_UNDEFINED;
	}
	imf->imf_st[1] = MCAST_INCLUDE;
}

  
                                                    
  
                                             
   
static void
imf_commit(struct in_mfilter *imf)
{
	struct ip_msource       *ims;
	struct in_msource       *lims;

	RB_FOREACH(ims, ip_msource_tree, &imf->imf_sources) {
		lims = (struct in_msource *)ims;
		lims->imsl_st[0] = lims->imsl_st[1];
	}
	imf->imf_st[0] = imf->imf_st[1];
}

  
                                                          
  
                                             
   
static void
imf_reap(struct in_mfilter *imf)
{
	struct ip_msource       *ims, *tims;
	struct in_msource       *lims;

	RB_FOREACH_SAFE(ims, ip_msource_tree, &imf->imf_sources, tims) {
		lims = (struct in_msource *)ims;
		if ((lims->imsl_st[0] == MCAST_UNDEFINED) &&
		    (lims->imsl_st[1] == MCAST_UNDEFINED)) {
			IGMP_PRINTF(("%s: free inms 0x%llx\n", __func__,
			    (uint64_t)VM_KERNEL_ADDRPERM(lims)));
			RB_REMOVE(ip_msource_tree, &imf->imf_sources, ims);
			inms_free(lims);
			imf->imf_nsrc--;
		}
	}
}

  
                                 
  
                                             
   
void
imf_purge(struct in_mfilter *imf)
{
	struct ip_msource       *ims, *tims;
	struct in_msource       *lims;

	RB_FOREACH_SAFE(ims, ip_msource_tree, &imf->imf_sources, tims) {
		lims = (struct in_msource *)ims;
		IGMP_PRINTF(("%s: free inms 0x%llx\n", __func__,
		    (uint64_t)VM_KERNEL_ADDRPERM(lims)));
		RB_REMOVE(ip_msource_tree, &imf->imf_sources, ims);
		inms_free(lims);
		imf->imf_nsrc--;
	}
	imf->imf_st[0] = imf->imf_st[1] = MCAST_UNDEFINED;
	VERIFY(RB_EMPTY(&imf->imf_sources));
}

  
                                                       
  
                                            
                                                        
                                                             
                                                                         
  
                                                                  
   
static int
inm_get_source(struct in_multi *inm, const in_addr_t haddr,
    const int noalloc, struct ip_msource **pims)
{
	struct ip_msource        find;
	struct ip_msource       *ims, *nims;
#ifdef IGMP_DEBUG
	struct in_addr ia;
	char buf[MAX_IPv4_STR_LEN];
#endif
	INM_LOCK_ASSERT_HELD(inm);

	find.ims_haddr = haddr;
	ims = RB_FIND(ip_msource_tree, &inm->inm_srcs, &find);
	if (ims == NULL && !noalloc) {
		if (inm->inm_nsrc == in_mcast_maxgrpsrc) {
			return ENOSPC;
		}
		nims = ipms_alloc(Z_WAITOK);
		nims->ims_haddr = haddr;
		RB_INSERT(ip_msource_tree, &inm->inm_srcs, nims);
		++inm->inm_nsrc;
		ims = nims;
#ifdef IGMP_DEBUG
		ia.s_addr = htonl(haddr);
		inet_ntop(AF_INET, &ia, buf, sizeof(buf));
		IGMP_PRINTF(("%s: allocated %s as 0x%llx\n", __func__,
		    buf, (uint64_t)VM_KERNEL_ADDRPERM(ims)));
#endif
	}

	*pims = ims;
	return 0;
}

  
                                                              
                                              
                                                          
                                                         
                                          
                              
   
uint8_t
ims_get_mode(const struct in_multi *inm, const struct ip_msource *ims,
    uint8_t t)
{
	INM_LOCK_ASSERT_HELD(__DECONST(struct in_multi *, inm));

	t = !!t;
	if (inm->inm_st[t].iss_ex > 0 &&
	    inm->inm_st[t].iss_ex == ims->ims_st[t].ex) {
		return MCAST_EXCLUDE;
	} else if (ims->ims_st[t].in > 0 && ims->ims_st[t].ex == 0) {
		return MCAST_INCLUDE;
	}
	return MCAST_UNDEFINED;
}

  
                                                    
                                                             
   
static void
ims_merge(struct ip_msource *ims, const struct in_msource *lims,
    const int rollback)
{
	int n = rollback ? -1 : 1;
#ifdef IGMP_DEBUG
	struct in_addr ia;

	ia.s_addr = htonl(ims->ims_haddr);
#endif

	if (lims->imsl_st[0] == MCAST_EXCLUDE) {
		IGMP_INET_PRINTF(ia,
		    ("%s: t1 ex -= %d on %s\n",
		    __func__, n, _igmp_inet_buf));
		ims->ims_st[1].ex -= n;
	} else if (lims->imsl_st[0] == MCAST_INCLUDE) {
		IGMP_INET_PRINTF(ia,
		    ("%s: t1 in -= %d on %s\n",
		    __func__, n, _igmp_inet_buf));
		ims->ims_st[1].in -= n;
	}

	if (lims->imsl_st[1] == MCAST_EXCLUDE) {
		IGMP_INET_PRINTF(ia,
		    ("%s: t1 ex += %d on %s\n",
		    __func__, n, _igmp_inet_buf));
		ims->ims_st[1].ex += n;
	} else if (lims->imsl_st[1] == MCAST_INCLUDE) {
		IGMP_INET_PRINTF(ia,
		    ("%s: t1 in += %d on %s\n",
		    __func__, n, _igmp_inet_buf));
		ims->ims_st[1].in += n;
	}
}

  
                                                                   
                                           
  
                                                        
                                                    
  
                                                                    
                                                                    
                                    
  
                                                                 
                               
   
static int
inm_merge(struct in_multi *inm,           struct in_mfilter *imf)
{
	struct ip_msource       *ims, *nims = NULL;
	struct in_msource       *lims;
	int                      schanged, error;
	int                      nsrc0, nsrc1;

	INM_LOCK_ASSERT_HELD(inm);

	schanged = 0;
	error = 0;
	nsrc1 = nsrc0 = 0;

	  
                                                      
                                                          
                                                           
                                                      
                                        
    
	RB_FOREACH(ims, ip_msource_tree, &imf->imf_sources) {
		lims = (struct in_msource *)ims;
		if (lims->imsl_st[0] == imf->imf_st[0]) {
			nsrc0++;
		}
		if (lims->imsl_st[1] == imf->imf_st[1]) {
			nsrc1++;
		}
		if (lims->imsl_st[0] == lims->imsl_st[1]) {
			continue;
		}
		error = inm_get_source(inm, lims->ims_haddr, 0, &nims);
		++schanged;
		if (error) {
			break;
		}
		ims_merge(nims, lims, 0);
	}
	if (error) {
		struct ip_msource *bims;

		RB_FOREACH_REVERSE_FROM(ims, ip_msource_tree, nims) {
			lims = (struct in_msource *)ims;
			if (lims->imsl_st[0] == lims->imsl_st[1]) {
				continue;
			}
			(void) inm_get_source(inm, lims->ims_haddr, 1, &bims);
			if (bims == NULL) {
				continue;
			}
			ims_merge(bims, lims, 1);
		}
		goto out_reap;
	}

	IGMP_PRINTF(("%s: imf filters in-mode: %d at t0, %d at t1\n",
	    __func__, nsrc0, nsrc1));

	                                                                     
	if (imf->imf_st[0] == imf->imf_st[1] &&
	    imf->imf_st[1] == MCAST_INCLUDE) {
		if (nsrc1 == 0) {
			IGMP_PRINTF(("%s: --in on inm at t1\n", __func__));
			--inm->inm_st[1].iss_in;
		}
	}

	                                              
	if (imf->imf_st[0] != imf->imf_st[1]) {
		IGMP_PRINTF(("%s: imf transition %d to %d\n",
		    __func__, imf->imf_st[0], imf->imf_st[1]));

		if (imf->imf_st[0] == MCAST_EXCLUDE) {
			IGMP_PRINTF(("%s: --ex on inm at t1\n", __func__));
			--inm->inm_st[1].iss_ex;
		} else if (imf->imf_st[0] == MCAST_INCLUDE) {
			IGMP_PRINTF(("%s: --in on inm at t1\n", __func__));
			--inm->inm_st[1].iss_in;
		}

		if (imf->imf_st[1] == MCAST_EXCLUDE) {
			IGMP_PRINTF(("%s: ex++ on inm at t1\n", __func__));
			inm->inm_st[1].iss_ex++;
		} else if (imf->imf_st[1] == MCAST_INCLUDE && nsrc1 > 0) {
			IGMP_PRINTF(("%s: in++ on inm at t1\n", __func__));
			inm->inm_st[1].iss_in++;
		}
	}

	  
                                                       
                                                    
                            
                                                                    
                                                     
                                                        
    
	if (inm->inm_st[1].iss_ex > 0) {
		IGMP_PRINTF(("%s: transition to EX\n", __func__));
		inm->inm_st[1].iss_fmode = MCAST_EXCLUDE;
	} else if (inm->inm_st[1].iss_in > 0) {
		IGMP_PRINTF(("%s: transition to IN\n", __func__));
		inm->inm_st[1].iss_fmode = MCAST_INCLUDE;
	} else {
		IGMP_PRINTF(("%s: transition to UNDEF\n", __func__));
		inm->inm_st[1].iss_fmode = MCAST_UNDEFINED;
	}

	                                                                 
	if (imf->imf_st[0] == MCAST_EXCLUDE && nsrc0 == 0) {
		if ((imf->imf_st[1] != MCAST_EXCLUDE) ||
		    (imf->imf_st[1] == MCAST_EXCLUDE && nsrc1 > 0)) {
			IGMP_PRINTF(("%s: --asm on inm at t1\n", __func__));
			--inm->inm_st[1].iss_asm;
		}
	}

	                                                             
	if (imf->imf_st[1] == MCAST_EXCLUDE && nsrc1 == 0) {
		IGMP_PRINTF(("%s: asm++ on inm at t1\n", __func__));
		inm->inm_st[1].iss_asm++;
	}

	IGMP_PRINTF(("%s: merged imf 0x%llx to inm 0x%llx\n", __func__,
	    (uint64_t)VM_KERNEL_ADDRPERM(imf),
	    (uint64_t)VM_KERNEL_ADDRPERM(inm)));
	inm_print(inm);

out_reap:
	if (schanged > 0) {
		IGMP_PRINTF(("%s: sources changed; reaping\n", __func__));
		inm_reap(inm);
	}
	return error;
}

  
                                                     
                                                         
   
void
inm_commit(struct in_multi *inm)
{
	struct ip_msource       *ims;

	INM_LOCK_ASSERT_HELD(inm);

	IGMP_PRINTF(("%s: commit inm 0x%llx\n", __func__,
	    (uint64_t)VM_KERNEL_ADDRPERM(inm)));
	IGMP_PRINTF(("%s: pre commit:\n", __func__));
	inm_print(inm);

	RB_FOREACH(ims, ip_msource_tree, &inm->inm_srcs) {
		ims->ims_st[0] = ims->ims_st[1];
	}
	inm->inm_st[0] = inm->inm_st[1];
}

  
                                                         
   
static void
inm_reap(struct in_multi *inm)
{
	struct ip_msource       *ims, *tims;

	INM_LOCK_ASSERT_HELD(inm);

	RB_FOREACH_SAFE(ims, ip_msource_tree, &inm->inm_srcs, tims) {
		if (ims->ims_st[0].ex > 0 || ims->ims_st[0].in > 0 ||
		    ims->ims_st[1].ex > 0 || ims->ims_st[1].in > 0 ||
		    ims->ims_stp != 0) {
			continue;
		}
		IGMP_PRINTF(("%s: free ims 0x%llx\n", __func__,
		    (uint64_t)VM_KERNEL_ADDRPERM(ims)));
		RB_REMOVE(ip_msource_tree, &inm->inm_srcs, ims);
		ipms_free(ims);
		inm->inm_nsrc--;
	}
}

  
                                                        
   
void
inm_purge(struct in_multi *inm)
{
	struct ip_msource       *ims, *tims;

	INM_LOCK_ASSERT_HELD(inm);

	RB_FOREACH_SAFE(ims, ip_msource_tree, &inm->inm_srcs, tims) {
		IGMP_PRINTF(("%s: free ims 0x%llx\n", __func__,
		    (uint64_t)VM_KERNEL_ADDRPERM(ims)));
		RB_REMOVE(ip_msource_tree, &inm->inm_srcs, ims);
		ipms_free(ims);
		inm->inm_nsrc--;
	}
}

  
                                            
  
                                         
                                                                    
  
                                                                    
                    
   
static int
in_joingroup(struct ifnet *ifp, const struct in_addr *gina,
              struct in_mfilter *imf, struct in_multi **pinm)
{
	struct in_mfilter        timf;
	struct in_multi         *inm = NULL;
	int                      error = 0;
	struct igmp_tparams      itp;

	IGMP_INET_PRINTF(*gina, ("%s: join %s on 0x%llx(%s))\n", __func__,
	    _igmp_inet_buf, (uint64_t)VM_KERNEL_ADDRPERM(ifp), if_name(ifp)));

	bzero(&itp, sizeof(itp));
	*pinm = NULL;

	  
                                                   
                                             
    
	if (imf == NULL) {
		imf_init(&timf, MCAST_UNDEFINED, MCAST_EXCLUDE);
		imf = &timf;
	}

	error = in_getmulti(ifp, gina, &inm);
	if (error) {
		IGMP_PRINTF(("%s: in_getmulti() failure\n", __func__));
		return error;
	}

	IGMP_PRINTF(("%s: merge inm state\n", __func__));

	INM_LOCK(inm);
	error = inm_merge(inm, imf);
	if (error) {
		IGMP_PRINTF(("%s: failed to merge inm state\n", __func__));
		goto out_inm_release;
	}

	IGMP_PRINTF(("%s: doing igmp downcall\n", __func__));
	error = igmp_change_state(inm, &itp);
	if (error) {
		IGMP_PRINTF(("%s: failed to update source\n", __func__));
		imf_rollback(imf);
		goto out_inm_release;
	}

out_inm_release:
	if (error) {
		IGMP_PRINTF(("%s: dropping ref on 0x%llx\n", __func__,
		    (uint64_t)VM_KERNEL_ADDRPERM(inm)));
		INM_UNLOCK(inm);
		INM_REMREF(inm);
	} else {
		INM_UNLOCK(inm);
		*pinm = inm;                                          
	}

	                                                       
	igmp_set_timeout(&itp);

	return error;
}

  
                                             
                                       
  
                                         
  
                                                                     
                                           
   
int
in_leavegroup(struct in_multi *inm,           struct in_mfilter *imf)
{
	struct in_mfilter        timf;
	int                      error, lastref;
	struct igmp_tparams      itp;

	bzero(&itp, sizeof(itp));
	error = 0;

	INM_LOCK_ASSERT_NOTHELD(inm);

	in_multihead_lock_exclusive();
	INM_LOCK(inm);

	IGMP_INET_PRINTF(inm->inm_addr,
	    ("%s: leave inm 0x%llx, %s/%s%d, imf 0x%llx\n", __func__,
	    (uint64_t)VM_KERNEL_ADDRPERM(inm), _igmp_inet_buf,
	    (inm_is_ifp_detached(inm) ? "null" : inm->inm_ifp->if_name),
	    inm->inm_ifp->if_unit, (uint64_t)VM_KERNEL_ADDRPERM(imf)));

	  
                                                   
                                             
    
	if (imf == NULL) {
		imf_init(&timf, MCAST_EXCLUDE, MCAST_UNDEFINED);
		imf = &timf;
	}

	  
                                                
   
                                                             
                                                             
                                      
    
	IGMP_PRINTF(("%s: merge inm state\n", __func__));

	error = inm_merge(inm, imf);
	KASSERT(error == 0, ("%s: failed to merge inm state\n", __func__));

	IGMP_PRINTF(("%s: doing igmp downcall\n", __func__));
	error = igmp_change_state(inm, &itp);
#if IGMP_DEBUG
	if (error) {
		IGMP_PRINTF(("%s: failed igmp downcall\n", __func__));
	}
#endif
	lastref = in_multi_detach(inm);
	VERIFY(!lastref || (!(inm->inm_debug & IFD_ATTACHED) &&
	    inm->inm_reqcnt == 0));
	INM_UNLOCK(inm);
	in_multihead_lock_done();

	if (lastref) {
		INM_REMREF(inm);                                   
	}
	                                                       
	igmp_set_timeout(&itp);

	return error;
}

  
                                                        
                                                     
                                                
   
struct in_multi *
in_addmulti(struct in_addr *ap, struct ifnet *ifp)
{
	struct in_multi *pinm = NULL;
	int error;

	KASSERT(IN_LOCAL_GROUP(ntohl(ap->s_addr)),
	    ("%s: %s not in 224.0.0.0/24\n", __func__, inet_ntoa(*ap)));

	error = in_joingroup(ifp, ap, NULL, &pinm);
	VERIFY(pinm != NULL || error != 0);

	return pinm;
}

  
                                                                        
                                                
   
void
in_delmulti(struct in_multi *inm)
{
	(void) in_leavegroup(inm, NULL);
}

  
                                                        
                                                             
  
                                                                  
                                      
  
                                                                      
   
static int
inp_block_unblock_source(struct inpcb *inp, struct sockopt *sopt)
{
	struct group_source_req          gsr;
	struct sockaddr_in              *gsa, *ssa;
	struct ifnet                    *ifp;
	struct in_mfilter               *imf;
	struct ip_moptions              *imo;
	struct in_msource               *ims;
	struct in_multi                 *inm;
	size_t                           idx;
	uint8_t                          fmode;
	int                              error, doblock;
	unsigned int                     ifindex = 0;
	struct igmp_tparams              itp;

	bzero(&itp, sizeof(itp));
	ifp = NULL;
	error = 0;
	doblock = 0;

	memset(&gsr, 0, sizeof(struct group_source_req));
	gsa = (struct sockaddr_in *)&gsr.gsr_group;
	ssa = (struct sockaddr_in *)&gsr.gsr_source;

	switch (sopt->sopt_name) {
	case IP_BLOCK_SOURCE:
	case IP_UNBLOCK_SOURCE: {
		struct ip_mreq_source    mreqs;

		error = sooptcopyin(sopt, &mreqs,
		    sizeof(struct ip_mreq_source),
		    sizeof(struct ip_mreq_source));
		if (error) {
			return error;
		}

		gsa->sin_family = AF_INET;
		gsa->sin_len = sizeof(struct sockaddr_in);
		gsa->sin_addr = mreqs.imr_multiaddr;

		ssa->sin_family = AF_INET;
		ssa->sin_len = sizeof(struct sockaddr_in);
		ssa->sin_addr = mreqs.imr_sourceaddr;

		if (!in_nullhost(mreqs.imr_interface)) {
			ifp = ip_multicast_if(&mreqs.imr_interface, &ifindex);
		}

		if (sopt->sopt_name == IP_BLOCK_SOURCE) {
			doblock = 1;
		}

		IGMP_INET_PRINTF(mreqs.imr_interface,
		    ("%s: imr_interface = %s, ifp = 0x%llx\n", __func__,
		    _igmp_inet_buf, (uint64_t)VM_KERNEL_ADDRPERM(ifp)));
		break;
	}

	case MCAST_BLOCK_SOURCE:
	case MCAST_UNBLOCK_SOURCE:
		error = sooptcopyin(sopt, &gsr,
		    sizeof(struct group_source_req),
		    sizeof(struct group_source_req));
		if (error) {
			return error;
		}

		if (gsa->sin_family != AF_INET ||
		    gsa->sin_len != sizeof(struct sockaddr_in)) {
			return EINVAL;
		}

		if (ssa->sin_family != AF_INET ||
		    ssa->sin_len != sizeof(struct sockaddr_in)) {
			return EINVAL;
		}

		ifnet_head_lock_shared();
		if (gsr.gsr_interface == 0 ||
		    (u_int)if_index < gsr.gsr_interface) {
			ifnet_head_done();
			return EADDRNOTAVAIL;
		}

		ifp = ifindex2ifnet[gsr.gsr_interface];
		ifnet_head_done();

		if (ifp == NULL) {
			return EADDRNOTAVAIL;
		}

		if (sopt->sopt_name == MCAST_BLOCK_SOURCE) {
			doblock = 1;
		}
		break;

	default:
		IGMP_PRINTF(("%s: unknown sopt_name %d\n",
		    __func__, sopt->sopt_name));
		return EOPNOTSUPP;
	}

	if (!IN_MULTICAST(ntohl(gsa->sin_addr.s_addr))) {
		return EINVAL;
	}

	  
                                                    
    
	imo = inp_findmoptions(inp);
	if (imo == NULL) {
		return ENOMEM;
	}

	IMO_LOCK(imo);
	idx = imo_match_group(imo, ifp, gsa);
	if (idx == (size_t)-1 || imo->imo_mfilters == NULL) {
		error = EADDRNOTAVAIL;
		goto out_imo_locked;
	}

	VERIFY(imo->imo_mfilters != NULL);
	imf = &imo->imo_mfilters[idx];
	inm = imo->imo_membership[idx];

	  
                                               
                                              
    
	fmode = imf->imf_st[0];
	if (fmode != MCAST_EXCLUDE) {
		error = EINVAL;
		goto out_imo_locked;
	}

	  
                                   
                                            
                                              
                                             
    
	ims = imo_match_source(imo, idx, ssa);
	if ((ims != NULL && doblock) || (ims == NULL && !doblock)) {
		IGMP_INET_PRINTF(ssa->sin_addr,
		    ("%s: source %s %spresent\n", __func__,
		    _igmp_inet_buf, doblock ? "" : "not "));
		error = EADDRNOTAVAIL;
		goto out_imo_locked;
	}

	  
                                                  
    
	if (doblock) {
		IGMP_PRINTF(("%s: %s source\n", __func__, "block"));
		ims = imf_graft(imf, fmode, ssa);
		if (ims == NULL) {
			error = ENOMEM;
		}
	} else {
		IGMP_PRINTF(("%s: %s source\n", __func__, "allow"));
		error = imf_prune(imf, ssa);
	}

	if (error) {
		IGMP_PRINTF(("%s: merge imf state failed\n", __func__));
		goto out_imf_rollback;
	}

	  
                                                
    
	INM_LOCK(inm);
	IGMP_PRINTF(("%s: merge inm state\n", __func__));
	error = inm_merge(inm, imf);
	if (error) {
		IGMP_PRINTF(("%s: failed to merge inm state\n", __func__));
		INM_UNLOCK(inm);
		goto out_imf_rollback;
	}

	IGMP_PRINTF(("%s: doing igmp downcall\n", __func__));
	error = igmp_change_state(inm, &itp);
	INM_UNLOCK(inm);
#if IGMP_DEBUG
	if (error) {
		IGMP_PRINTF(("%s: failed igmp downcall\n", __func__));
	}
#endif

out_imf_rollback:
	if (error) {
		imf_rollback(imf);
	} else {
		imf_commit(imf);
	}

	imf_reap(imf);

out_imo_locked:
	IMO_UNLOCK(imo);
	IMO_REMREF(imo);                                     

	                                                       
	igmp_set_timeout(&itp);

	return error;
}

  
                                                                  
  
                                                                 
                                                             
   
static struct ip_moptions *
inp_findmoptions(struct inpcb *inp)
{
	struct ip_moptions       *imo;
	struct in_multi         **immp;
	struct in_mfilter        *imfp;
	size_t                    idx;

	if ((imo = inp->inp_moptions) != NULL) {
		IMO_ADDREF(imo);                        
		return imo;
	}

	imo = ip_allocmoptions(Z_WAITOK);
	if (imo == NULL) {
		return NULL;
	}

	immp = _MALLOC(sizeof(*immp) * IP_MIN_MEMBERSHIPS, M_IPMOPTS,
	    M_WAITOK | M_ZERO);
	if (immp == NULL) {
		IMO_REMREF(imo);
		return NULL;
	}

	imfp = _MALLOC(sizeof(struct in_mfilter) * IP_MIN_MEMBERSHIPS,
	    M_INMFILTER, M_WAITOK | M_ZERO);
	if (imfp == NULL) {
		_FREE(immp, M_IPMOPTS);
		IMO_REMREF(imo);
		return NULL;
	}

	imo->imo_multicast_ifp = NULL;
	imo->imo_multicast_addr.s_addr = INADDR_ANY;
	imo->imo_multicast_vif = -1;
	imo->imo_multicast_ttl = IP_DEFAULT_MULTICAST_TTL;
	imo->imo_multicast_loop = !!in_mcast_loop;
	imo->imo_num_memberships = 0;
	imo->imo_max_memberships = IP_MIN_MEMBERSHIPS;
	imo->imo_membership = immp;

	                                          
	for (idx = 0; idx < IP_MIN_MEMBERSHIPS; idx++) {
		imf_init(&imfp[idx], MCAST_UNDEFINED, MCAST_EXCLUDE);
	}

	imo->imo_mfilters = imfp;
	inp->inp_moptions = imo;                                             
	IMO_ADDREF(imo);                        

	return imo;
}
  
                                                                         
   
static int
inp_get_source_filters(struct inpcb *inp, struct sockopt *sopt)
{
	struct __msfilterreq64  msfr = {}, msfr64;
	struct __msfilterreq32  msfr32;
	struct sockaddr_in      *gsa;
	struct ifnet            *ifp;
	struct ip_moptions      *imo;
	struct in_mfilter       *imf;
	struct ip_msource       *ims;
	struct in_msource       *lims;
	struct sockaddr_in      *psin;
	struct sockaddr_storage *ptss;
	struct sockaddr_storage *tss;
	int                      error;
	size_t                   idx;
	uint32_t                 nsrcs, ncsrcs;
	user_addr_t              tmp_ptr;

	imo = inp->inp_moptions;
	VERIFY(imo != NULL);

	if (IS_64BIT_PROCESS(current_proc())) {
		error = sooptcopyin(sopt, &msfr64,
		    sizeof(struct __msfilterreq64),
		    sizeof(struct __msfilterreq64));
		if (error) {
			return error;
		}
		                                  
		memcpy(&msfr, &msfr64, sizeof(msfr64));
	} else {
		error = sooptcopyin(sopt, &msfr32,
		    sizeof(struct __msfilterreq32),
		    sizeof(struct __msfilterreq32));
		if (error) {
			return error;
		}
		                                  
		memcpy(&msfr, &msfr32, sizeof(msfr32));
	}

	ifnet_head_lock_shared();
	if (msfr.msfr_ifindex == 0 || (u_int)if_index < msfr.msfr_ifindex) {
		ifnet_head_done();
		return EADDRNOTAVAIL;
	}

	ifp = ifindex2ifnet[msfr.msfr_ifindex];
	ifnet_head_done();

	if (ifp == NULL) {
		return EADDRNOTAVAIL;
	}

	if ((size_t) msfr.msfr_nsrcs >
	    UINT32_MAX / sizeof(struct sockaddr_storage)) {
		msfr.msfr_nsrcs = UINT32_MAX / sizeof(struct sockaddr_storage);
	}

	if (msfr.msfr_nsrcs > in_mcast_maxsocksrc) {
		msfr.msfr_nsrcs = in_mcast_maxsocksrc;
	}

	IMO_LOCK(imo);
	  
                               
    
	gsa = (struct sockaddr_in *)&msfr.msfr_group;

	idx = imo_match_group(imo, ifp, gsa);
	if (idx == (size_t)-1 || imo->imo_mfilters == NULL) {
		IMO_UNLOCK(imo);
		return EADDRNOTAVAIL;
	}
	imf = &imo->imo_mfilters[idx];

	  
                                          
    
	if (imf->imf_st[1] == MCAST_UNDEFINED) {
		IMO_UNLOCK(imo);
		return EAGAIN;
	}
	msfr.msfr_fmode = imf->imf_st[1];

	  
                                                              
                                   
                                                         
                                                          
                              
    

	if (IS_64BIT_PROCESS(current_proc())) {
		tmp_ptr = CAST_USER_ADDR_T(msfr64.msfr_srcs);
	} else {
		tmp_ptr = CAST_USER_ADDR_T(msfr32.msfr_srcs);
	}

	tss = NULL;
	if (tmp_ptr != USER_ADDR_NULL && msfr.msfr_nsrcs > 0) {
		tss = _MALLOC((size_t) msfr.msfr_nsrcs * sizeof(*tss),
		    M_TEMP, M_WAITOK | M_ZERO);
		if (tss == NULL) {
			IMO_UNLOCK(imo);
			return ENOBUFS;
		}
	}

	  
                                          
                                                                
    
	nsrcs = msfr.msfr_nsrcs;
	ncsrcs = 0;
	ptss = tss;
	RB_FOREACH(ims, ip_msource_tree, &imf->imf_sources) {
		lims = (struct in_msource *)ims;
		if (lims->imsl_st[0] == MCAST_UNDEFINED ||
		    lims->imsl_st[0] != imf->imf_st[0]) {
			continue;
		}
		if (tss != NULL && nsrcs > 0) {
			psin = (struct sockaddr_in *)ptss;
			psin->sin_family = AF_INET;
			psin->sin_len = sizeof(struct sockaddr_in);
			psin->sin_addr.s_addr = htonl(lims->ims_haddr);
			psin->sin_port = 0;
			++ptss;
			--nsrcs;
			++ncsrcs;
		}
	}

	IMO_UNLOCK(imo);

	if (tss != NULL) {
		error = copyout(tss, CAST_USER_ADDR_T(tmp_ptr), ncsrcs * sizeof(*tss));
		FREE(tss, M_TEMP);
		if (error) {
			return error;
		}
	}

	msfr.msfr_nsrcs = ncsrcs;
	if (IS_64BIT_PROCESS(current_proc())) {
		msfr64.msfr_ifindex = msfr.msfr_ifindex;
		msfr64.msfr_fmode   = msfr.msfr_fmode;
		msfr64.msfr_nsrcs   = msfr.msfr_nsrcs;
		memcpy(&msfr64.msfr_group, &msfr.msfr_group,
		    sizeof(struct sockaddr_storage));
		error = sooptcopyout(sopt, &msfr64,
		    sizeof(struct __msfilterreq64));
	} else {
		msfr32.msfr_ifindex = msfr.msfr_ifindex;
		msfr32.msfr_fmode   = msfr.msfr_fmode;
		msfr32.msfr_nsrcs   = msfr.msfr_nsrcs;
		memcpy(&msfr32.msfr_group, &msfr.msfr_group,
		    sizeof(struct sockaddr_storage));
		error = sooptcopyout(sopt, &msfr32,
		    sizeof(struct __msfilterreq32));
	}

	return error;
}

  
                                                                    
   
int
inp_getmoptions(struct inpcb *inp, struct sockopt *sopt)
{
	struct ip_mreqn          mreqn;
	struct ip_moptions      *imo;
	struct ifnet            *ifp;
	struct in_ifaddr        *ia;
	int                      error, optval;
	unsigned int             ifindex;
	u_char                   coptval;

	imo = inp->inp_moptions;
	  
                                                        
                                     
    
	if (SOCK_PROTO(inp->inp_socket) == IPPROTO_DIVERT ||
	    (SOCK_TYPE(inp->inp_socket) != SOCK_RAW &&
	    SOCK_TYPE(inp->inp_socket) != SOCK_DGRAM)) {
		return EOPNOTSUPP;
	}

	error = 0;
	switch (sopt->sopt_name) {
	case IP_MULTICAST_IF:
		memset(&mreqn, 0, sizeof(struct ip_mreqn));
		if (imo != NULL) {
			IMO_LOCK(imo);
			ifp = imo->imo_multicast_ifp;
			if (!in_nullhost(imo->imo_multicast_addr)) {
				mreqn.imr_address = imo->imo_multicast_addr;
			} else if (ifp != NULL) {
				mreqn.imr_ifindex = ifp->if_index;
				IFP_TO_IA(ifp, ia);
				if (ia != NULL) {
					IFA_LOCK_SPIN(&ia->ia_ifa);
					mreqn.imr_address =
					    IA_SIN(ia)->sin_addr;
					IFA_UNLOCK(&ia->ia_ifa);
					IFA_REMREF(&ia->ia_ifa);
				}
			}
			IMO_UNLOCK(imo);
		}
		if (sopt->sopt_valsize == sizeof(struct ip_mreqn)) {
			error = sooptcopyout(sopt, &mreqn,
			    sizeof(struct ip_mreqn));
		} else {
			error = sooptcopyout(sopt, &mreqn.imr_address,
			    sizeof(struct in_addr));
		}
		break;

	case IP_MULTICAST_IFINDEX:
		if (imo != NULL) {
			IMO_LOCK(imo);
		}
		if (imo == NULL || imo->imo_multicast_ifp == NULL) {
			ifindex = 0;
		} else {
			ifindex = imo->imo_multicast_ifp->if_index;
		}
		if (imo != NULL) {
			IMO_UNLOCK(imo);
		}
		error = sooptcopyout(sopt, &ifindex, sizeof(ifindex));
		break;

	case IP_MULTICAST_TTL:
		if (imo == NULL) {
			optval = coptval = IP_DEFAULT_MULTICAST_TTL;
		} else {
			IMO_LOCK(imo);
			optval = coptval = imo->imo_multicast_ttl;
			IMO_UNLOCK(imo);
		}
		if (sopt->sopt_valsize == sizeof(u_char)) {
			error = sooptcopyout(sopt, &coptval, sizeof(u_char));
		} else {
			error = sooptcopyout(sopt, &optval, sizeof(int));
		}
		break;

	case IP_MULTICAST_LOOP:
		if (imo == 0) {
			optval = coptval = IP_DEFAULT_MULTICAST_LOOP;
		} else {
			IMO_LOCK(imo);
			optval = coptval = imo->imo_multicast_loop;
			IMO_UNLOCK(imo);
		}
		if (sopt->sopt_valsize == sizeof(u_char)) {
			error = sooptcopyout(sopt, &coptval, sizeof(u_char));
		} else {
			error = sooptcopyout(sopt, &optval, sizeof(int));
		}
		break;

	case IP_MSFILTER:
		if (imo == NULL) {
			error = EADDRNOTAVAIL;
		} else {
			error = inp_get_source_filters(inp, sopt);
		}
		break;

	default:
		error = ENOPROTOOPT;
		break;
	}

	return error;
}

  
                                                             
                                                                      
  
                                                               
                                                                   
                                                                 
                                                                  
  
                                                                     
                                                     
  
                                                                   
                                                         
                                                           
                                                              
                                                
  
                                         
  
   
static struct ifnet *
inp_lookup_mcast_ifp(const struct inpcb *inp,
    const struct sockaddr_in *gsin, const struct in_addr ina)
{
	struct ifnet    *ifp;
	unsigned int     ifindex = 0;

	VERIFY(gsin->sin_family == AF_INET);
	VERIFY(IN_MULTICAST(ntohl(gsin->sin_addr.s_addr)));

	ifp = NULL;
	if (!in_nullhost(ina)) {
		struct in_addr new_ina;
		memcpy(&new_ina, &ina, sizeof(struct in_addr));
		ifp = ip_multicast_if(&new_ina, &ifindex);
	} else {
		struct route ro;
		unsigned int ifscope = IFSCOPE_NONE;

		if (inp != NULL && (inp->inp_flags & INP_BOUND_IF)) {
			ifscope = inp->inp_boundifp->if_index;
		}

		bzero(&ro, sizeof(ro));
		memcpy(&ro.ro_dst, gsin, sizeof(struct sockaddr_in));
		rtalloc_scoped_ign(&ro, 0, ifscope);
		if (ro.ro_rt != NULL) {
			ifp = ro.ro_rt->rt_ifp;
			VERIFY(ifp != NULL);
		} else {
			struct in_ifaddr *ia;
			struct ifnet *mifp;

			mifp = NULL;
			lck_rw_lock_shared(in_ifaddr_rwlock);
			TAILQ_FOREACH(ia, &in_ifaddrhead, ia_link) {
				IFA_LOCK_SPIN(&ia->ia_ifa);
				mifp = ia->ia_ifp;
				IFA_UNLOCK(&ia->ia_ifa);
				if (!(mifp->if_flags & IFF_LOOPBACK) &&
				    (mifp->if_flags & IFF_MULTICAST)) {
					ifp = mifp;
					break;
				}
			}
			lck_rw_done(in_ifaddr_rwlock);
		}
		ROUTE_RELEASE(&ro);
	}

	return ifp;
}

  
                                                        
  
                                                                              
                                                                            
                                                                              
                                  
   
int
inp_join_group(struct inpcb *inp, struct sockopt *sopt)
{
	struct group_source_req          gsr;
	struct sockaddr_in              *gsa, *ssa;
	struct ifnet                    *ifp;
	struct in_mfilter               *imf;
	struct ip_moptions              *imo;
	struct in_multi                 *inm = NULL;
	struct in_msource               *lims;
	size_t                           idx;
	int                              error, is_new;
	struct igmp_tparams              itp;

	bzero(&itp, sizeof(itp));
	ifp = NULL;
	imf = NULL;
	error = 0;
	is_new = 0;

	memset(&gsr, 0, sizeof(struct group_source_req));
	gsa = (struct sockaddr_in *)&gsr.gsr_group;
	gsa->sin_family = AF_UNSPEC;
	ssa = (struct sockaddr_in *)&gsr.gsr_source;
	ssa->sin_family = AF_UNSPEC;

	switch (sopt->sopt_name) {
	case IP_ADD_MEMBERSHIP:
	case IP_ADD_SOURCE_MEMBERSHIP: {
		struct ip_mreq_source    mreqs;

		if (sopt->sopt_name == IP_ADD_MEMBERSHIP) {
			error = sooptcopyin(sopt, &mreqs,
			    sizeof(struct ip_mreq),
			    sizeof(struct ip_mreq));
			  
                                              
                                                  
      
			mreqs.imr_interface = mreqs.imr_sourceaddr;
			mreqs.imr_sourceaddr.s_addr = INADDR_ANY;
		} else if (sopt->sopt_name == IP_ADD_SOURCE_MEMBERSHIP) {
			error = sooptcopyin(sopt, &mreqs,
			    sizeof(struct ip_mreq_source),
			    sizeof(struct ip_mreq_source));
		}
		if (error) {
			IGMP_PRINTF(("%s: error copyin IP_ADD_MEMBERSHIP/"
			    "IP_ADD_SOURCE_MEMBERSHIP %d err=%d\n",
			    __func__, sopt->sopt_name, error));
			return error;
		}

		gsa->sin_family = AF_INET;
		gsa->sin_len = sizeof(struct sockaddr_in);
		gsa->sin_addr = mreqs.imr_multiaddr;

		if (sopt->sopt_name == IP_ADD_SOURCE_MEMBERSHIP) {
			ssa->sin_family = AF_INET;
			ssa->sin_len = sizeof(struct sockaddr_in);
			ssa->sin_addr = mreqs.imr_sourceaddr;
		}

		if (!IN_MULTICAST(ntohl(gsa->sin_addr.s_addr))) {
			return EINVAL;
		}

		ifp = inp_lookup_mcast_ifp(inp, gsa, mreqs.imr_interface);
		IGMP_INET_PRINTF(mreqs.imr_interface,
		    ("%s: imr_interface = %s, ifp = 0x%llx\n", __func__,
		    _igmp_inet_buf, (uint64_t)VM_KERNEL_ADDRPERM(ifp)));
		break;
	}

	case MCAST_JOIN_GROUP:
	case MCAST_JOIN_SOURCE_GROUP:
		if (sopt->sopt_name == MCAST_JOIN_GROUP) {
			error = sooptcopyin(sopt, &gsr,
			    sizeof(struct group_req),
			    sizeof(struct group_req));
		} else if (sopt->sopt_name == MCAST_JOIN_SOURCE_GROUP) {
			error = sooptcopyin(sopt, &gsr,
			    sizeof(struct group_source_req),
			    sizeof(struct group_source_req));
		}
		if (error) {
			return error;
		}

		if (gsa->sin_family != AF_INET ||
		    gsa->sin_len != sizeof(struct sockaddr_in)) {
			return EINVAL;
		}

		  
                                                         
                                                             
     
		gsa->sin_port = 0;
		if (sopt->sopt_name == MCAST_JOIN_SOURCE_GROUP) {
			if (ssa->sin_family != AF_INET ||
			    ssa->sin_len != sizeof(struct sockaddr_in)) {
				return EINVAL;
			}
			ssa->sin_port = 0;
		}

		if (!IN_MULTICAST(ntohl(gsa->sin_addr.s_addr))) {
			return EINVAL;
		}

		ifnet_head_lock_shared();
		if (gsr.gsr_interface == 0 ||
		    (u_int)if_index < gsr.gsr_interface) {
			ifnet_head_done();
			return EADDRNOTAVAIL;
		}
		ifp = ifindex2ifnet[gsr.gsr_interface];
		ifnet_head_done();

		break;

	default:
		IGMP_PRINTF(("%s: unknown sopt_name %d\n",
		    __func__, sopt->sopt_name));
		return EOPNOTSUPP;
	}

	if (ifp == NULL || (ifp->if_flags & IFF_MULTICAST) == 0) {
		return EADDRNOTAVAIL;
	}

	INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_mcast_join_total);
	  
                                                        
    
	if (inp->inp_lport == htons(5353)) {
		INC_ATOMIC_INT64_LIM(net_api_stats.nas_socket_mcast_join_os_total);
	}

	imo = inp_findmoptions(inp);
	if (imo == NULL) {
		return ENOMEM;
	}

	IMO_LOCK(imo);
	idx = imo_match_group(imo, ifp, gsa);
	if (idx == (size_t)-1) {
		is_new = 1;
	} else {
		inm = imo->imo_membership[idx];
		imf = &imo->imo_mfilters[idx];
		if (ssa->sin_family != AF_UNSPEC) {
			  
                                                        
                                                       
                                                 
      
			if (imf->imf_st[1] != MCAST_INCLUDE) {
				error = EINVAL;
				goto out_imo_locked;
			}
			  
                           
     
                                                   
                                                 
                                                  
                                                  
     
                                                      
                                                      
                                              
     
                                                
                                                  
                                                
      
			lims = imo_match_source(imo, idx, ssa);
			if (lims != NULL     
                                                           ) {
				error = EADDRNOTAVAIL;
				goto out_imo_locked;
			}
		} else {
			  
                                               
                                               
                                             
                                         
                                           
     
                                                       
                                                  
                                                      
                                                   
                                                 
                                                
      
			error = EINVAL;
			                                       
			if (imf->imf_st[1] == MCAST_EXCLUDE) {
				error = EADDRINUSE;
			}
			goto out_imo_locked;
		}
	}

	  
                                                  
    

	if (is_new) {
		if (imo->imo_num_memberships == imo->imo_max_memberships) {
			error = imo_grow(imo, 0);
			if (error) {
				goto out_imo_locked;
			}
		}
		  
                                                      
                                                     
                                               
     
		idx = imo->imo_num_memberships;
		imo->imo_membership[idx] = NULL;
		imo->imo_num_memberships++;
		VERIFY(imo->imo_mfilters != NULL);
		imf = &imo->imo_mfilters[idx];
		VERIFY(RB_EMPTY(&imf->imf_sources));
	}

	  
                                                      
                                                      
                                                            
                                                                  
    
	if (ssa->sin_family != AF_UNSPEC) {
		                                  
		if (is_new) {
			IGMP_PRINTF(("%s: new join w/source\n", __func__));
			imf_init(imf, MCAST_UNDEFINED, MCAST_INCLUDE);
		} else {
			IGMP_PRINTF(("%s: %s source\n", __func__, "allow"));
		}
		lims = imf_graft(imf, MCAST_INCLUDE, ssa);
		if (lims == NULL) {
			IGMP_PRINTF(("%s: merge imf state failed\n",
			    __func__));
			error = ENOMEM;
			goto out_imo_free;
		}
	} else {
		                                                        
		if (is_new) {
			IGMP_PRINTF(("%s: new join w/o source\n", __func__));
			imf_init(imf, MCAST_UNDEFINED, MCAST_EXCLUDE);
		}
	}

	  
                                                
    
	if (is_new) {
		  
                                                                            
                                                                     
                                                                            
                                                           
     
		IMO_ADDREF_LOCKED(imo);
		IMO_UNLOCK(imo);
		socket_unlock(inp->inp_socket, 0);

		VERIFY(inm == NULL);
		error = in_joingroup(ifp, &gsa->sin_addr, imf, &inm);

		socket_lock(inp->inp_socket, 0);
		IMO_REMREF(imo);
		IMO_LOCK(imo);

		VERIFY(inm != NULL || error != 0);
		if (error) {
			goto out_imo_free;
		}
		imo->imo_membership[idx] = inm;                          
	} else {
		IGMP_PRINTF(("%s: merge inm state\n", __func__));
		INM_LOCK(inm);
		error = inm_merge(inm, imf);
		if (error) {
			IGMP_PRINTF(("%s: failed to merge inm state\n",
			    __func__));
			INM_UNLOCK(inm);
			goto out_imf_rollback;
		}
		IGMP_PRINTF(("%s: doing igmp downcall\n", __func__));
		error = igmp_change_state(inm, &itp);
		INM_UNLOCK(inm);
		if (error) {
			IGMP_PRINTF(("%s: failed igmp downcall\n",
			    __func__));
			goto out_imf_rollback;
		}
	}

out_imf_rollback:
	if (error) {
		imf_rollback(imf);
		if (is_new) {
			imf_purge(imf);
		} else {
			imf_reap(imf);
		}
	} else {
		imf_commit(imf);
	}

out_imo_free:
	if (error && is_new) {
		VERIFY(inm == NULL);
		imo->imo_membership[idx] = NULL;
		--imo->imo_num_memberships;
	}

out_imo_locked:
	IMO_UNLOCK(imo);
	IMO_REMREF(imo);                                     

	                                                       
	igmp_set_timeout(&itp);

	return error;
}

  
                                                                     
  
                                                                           
                                                                 
   
int
inp_leave_group(struct inpcb *inp, struct sockopt *sopt)
{
	struct group_source_req          gsr;
	struct ip_mreq_source            mreqs;
	struct sockaddr_in              *gsa, *ssa;
	struct ifnet                    *ifp;
	struct in_mfilter               *imf;
	struct ip_moptions              *imo;
	struct in_msource               *ims;
	struct in_multi                 *inm = NULL;
	size_t                           idx;
	int                              error, is_final;
	unsigned int                     ifindex = 0;
	struct igmp_tparams              itp;

	bzero(&itp, sizeof(itp));
	ifp = NULL;
	error = 0;
	is_final = 1;

	memset(&gsr, 0, sizeof(struct group_source_req));
	gsa = (struct sockaddr_in *)&gsr.gsr_group;
	ssa = (struct sockaddr_in *)&gsr.gsr_source;

	switch (sopt->sopt_name) {
	case IP_DROP_MEMBERSHIP:
	case IP_DROP_SOURCE_MEMBERSHIP:
		if (sopt->sopt_name == IP_DROP_MEMBERSHIP) {
			error = sooptcopyin(sopt, &mreqs,
			    sizeof(struct ip_mreq),
			    sizeof(struct ip_mreq));
			  
                                              
                                            
                      
      
			mreqs.imr_interface = mreqs.imr_sourceaddr;
			mreqs.imr_sourceaddr.s_addr = INADDR_ANY;
		} else if (sopt->sopt_name == IP_DROP_SOURCE_MEMBERSHIP) {
			error = sooptcopyin(sopt, &mreqs,
			    sizeof(struct ip_mreq_source),
			    sizeof(struct ip_mreq_source));
		}
		if (error) {
			return error;
		}

		gsa->sin_family = AF_INET;
		gsa->sin_len = sizeof(struct sockaddr_in);
		gsa->sin_addr = mreqs.imr_multiaddr;

		if (sopt->sopt_name == IP_DROP_SOURCE_MEMBERSHIP) {
			ssa->sin_family = AF_INET;
			ssa->sin_len = sizeof(struct sockaddr_in);
			ssa->sin_addr = mreqs.imr_sourceaddr;
		}
		  
                                                          
                                                   
                                           
                                                         
                                            
     
		if (!in_nullhost(mreqs.imr_interface)) {
			ifp = ip_multicast_if(&mreqs.imr_interface, &ifindex);
		}

		IGMP_INET_PRINTF(mreqs.imr_interface,
		    ("%s: imr_interface = %s, ifp = 0x%llx\n", __func__,
		    _igmp_inet_buf, (uint64_t)VM_KERNEL_ADDRPERM(ifp)));

		break;

	case MCAST_LEAVE_GROUP:
	case MCAST_LEAVE_SOURCE_GROUP:
		if (sopt->sopt_name == MCAST_LEAVE_GROUP) {
			error = sooptcopyin(sopt, &gsr,
			    sizeof(struct group_req),
			    sizeof(struct group_req));
		} else if (sopt->sopt_name == MCAST_LEAVE_SOURCE_GROUP) {
			error = sooptcopyin(sopt, &gsr,
			    sizeof(struct group_source_req),
			    sizeof(struct group_source_req));
		}
		if (error) {
			return error;
		}

		if (gsa->sin_family != AF_INET ||
		    gsa->sin_len != sizeof(struct sockaddr_in)) {
			return EINVAL;
		}

		if (sopt->sopt_name == MCAST_LEAVE_SOURCE_GROUP) {
			if (ssa->sin_family != AF_INET ||
			    ssa->sin_len != sizeof(struct sockaddr_in)) {
				return EINVAL;
			}
		}

		ifnet_head_lock_shared();
		if (gsr.gsr_interface == 0 ||
		    (u_int)if_index < gsr.gsr_interface) {
			ifnet_head_done();
			return EADDRNOTAVAIL;
		}

		ifp = ifindex2ifnet[gsr.gsr_interface];
		ifnet_head_done();
		break;

	default:
		IGMP_PRINTF(("%s: unknown sopt_name %d\n",
		    __func__, sopt->sopt_name));
		return EOPNOTSUPP;
	}

	if (!IN_MULTICAST(ntohl(gsa->sin_addr.s_addr))) {
		return EINVAL;
	}

	  
                                                
    
	imo = inp_findmoptions(inp);
	if (imo == NULL) {
		return ENOMEM;
	}

	IMO_LOCK(imo);
	idx = imo_match_group(imo, ifp, gsa);
	if (idx == (size_t)-1) {
		error = EADDRNOTAVAIL;
		goto out_locked;
	}
	inm = imo->imo_membership[idx];
	imf = &imo->imo_mfilters[idx];

	if (ssa->sin_family != AF_UNSPEC) {
		IGMP_PRINTF(("%s: opt=%d is_final=0\n", __func__,
		    sopt->sopt_name));
		is_final = 0;
	}

	  
                                                  
    

	  
                                                              
                                                                     
    
	if (is_final) {
		imf_leave(imf);
	} else {
		if (imf->imf_st[0] == MCAST_EXCLUDE) {
			error = EADDRNOTAVAIL;
			goto out_locked;
		}
		ims = imo_match_source(imo, idx, ssa);
		if (ims == NULL) {
			IGMP_INET_PRINTF(ssa->sin_addr,
			    ("%s: source %s %spresent\n", __func__,
			    _igmp_inet_buf, "not "));
			error = EADDRNOTAVAIL;
			goto out_locked;
		}
		IGMP_PRINTF(("%s: %s source\n", __func__, "block"));
		error = imf_prune(imf, ssa);
		if (error) {
			IGMP_PRINTF(("%s: merge imf state failed\n",
			    __func__));
			goto out_locked;
		}
	}

	  
                                                
    


	if (is_final) {
		  
                                                  
                                                  
                            
     
		(void) in_leavegroup(inm, imf);
	} else {
		IGMP_PRINTF(("%s: merge inm state\n", __func__));
		INM_LOCK(inm);
		error = inm_merge(inm, imf);
		if (error) {
			IGMP_PRINTF(("%s: failed to merge inm state\n",
			    __func__));
			INM_UNLOCK(inm);
			goto out_imf_rollback;
		}

		IGMP_PRINTF(("%s: doing igmp downcall\n", __func__));
		error = igmp_change_state(inm, &itp);
		if (error) {
			IGMP_PRINTF(("%s: failed igmp downcall\n", __func__));
		}
		INM_UNLOCK(inm);
	}

out_imf_rollback:
	if (error) {
		imf_rollback(imf);
	} else {
		imf_commit(imf);
	}

	imf_reap(imf);

	if (is_final) {
		                                             
		VERIFY(inm == imo->imo_membership[idx]);
		imo->imo_membership[idx] = NULL;

		  
                                                   
     
		IMO_ADDREF_LOCKED(imo);
		IMO_UNLOCK(imo);
		socket_unlock(inp->inp_socket, 0);

		INM_REMREF(inm);

		socket_lock(inp->inp_socket, 0);
		IMO_REMREF(imo);
		IMO_LOCK(imo);

		for (++idx; idx < imo->imo_num_memberships; ++idx) {
			imo->imo_membership[idx - 1] = imo->imo_membership[idx];
			imo->imo_mfilters[idx - 1] = imo->imo_mfilters[idx];
		}
		imo->imo_num_memberships--;
	}

out_locked:
	IMO_UNLOCK(imo);
	IMO_REMREF(imo);                                     

	                                                       
	igmp_set_timeout(&itp);

	return error;
}

  
                                                                  
  
                                                                         
                                                                      
                                                               
                                                               
   
static int
inp_set_multicast_if(struct inpcb *inp, struct sockopt *sopt)
{
	struct in_addr           addr;
	struct ip_mreqn          mreqn;
	struct ifnet            *ifp;
	struct ip_moptions      *imo;
	int                      error = 0;
	unsigned int             ifindex = 0;

	bzero(&addr, sizeof(addr));
	if (sopt->sopt_valsize == sizeof(struct ip_mreqn)) {
		  
                                               
                                      
     
		error = sooptcopyin(sopt, &mreqn, sizeof(struct ip_mreqn),
		    sizeof(struct ip_mreqn));
		if (error) {
			return error;
		}

		ifnet_head_lock_shared();
		if (mreqn.imr_ifindex < 0 || if_index < mreqn.imr_ifindex) {
			ifnet_head_done();
			return EINVAL;
		}

		if (mreqn.imr_ifindex == 0) {
			ifp = NULL;
		} else {
			ifp = ifindex2ifnet[mreqn.imr_ifindex];
			if (ifp == NULL) {
				ifnet_head_done();
				return EADDRNOTAVAIL;
			}
		}
		ifnet_head_done();
	} else {
		  
                                                
                                       
     
		error = sooptcopyin(sopt, &addr, sizeof(struct in_addr),
		    sizeof(struct in_addr));
		if (error) {
			return error;
		}
		if (in_nullhost(addr)) {
			ifp = NULL;
		} else {
			ifp = ip_multicast_if(&addr, &ifindex);
			if (ifp == NULL) {
				IGMP_INET_PRINTF(addr,
				    ("%s: can't find ifp for addr=%s\n",
				    __func__, _igmp_inet_buf));
				return EADDRNOTAVAIL;
			}
		}
	}

	                                                       
	if (ifp != NULL && (ifp->if_flags & IFF_MULTICAST) == 0) {
		return EOPNOTSUPP;
	}

	imo = inp_findmoptions(inp);
	if (imo == NULL) {
		return ENOMEM;
	}

	IMO_LOCK(imo);
	imo->imo_multicast_ifp = ifp;
	if (ifindex) {
		imo->imo_multicast_addr = addr;
	} else {
		imo->imo_multicast_addr.s_addr = INADDR_ANY;
	}
	IMO_UNLOCK(imo);
	IMO_REMREF(imo);                                     

	return 0;
}

  
                                                                         
   
static int
inp_set_source_filters(struct inpcb *inp, struct sockopt *sopt)
{
	struct __msfilterreq64   msfr = {}, msfr64;
	struct __msfilterreq32   msfr32;
	struct sockaddr_in      *gsa;
	struct ifnet            *ifp;
	struct in_mfilter       *imf;
	struct ip_moptions      *imo;
	struct in_multi         *inm;
	size_t                   idx;
	int                      error;
	uint64_t                 tmp_ptr;
	struct igmp_tparams      itp;

	bzero(&itp, sizeof(itp));

	if (IS_64BIT_PROCESS(current_proc())) {
		error = sooptcopyin(sopt, &msfr64,
		    sizeof(struct __msfilterreq64),
		    sizeof(struct __msfilterreq64));
		if (error) {
			return error;
		}
		                                  
		memcpy(&msfr, &msfr64, sizeof(msfr64));
	} else {
		error = sooptcopyin(sopt, &msfr32,
		    sizeof(struct __msfilterreq32),
		    sizeof(struct __msfilterreq32));
		if (error) {
			return error;
		}
		                                  
		memcpy(&msfr, &msfr32, sizeof(msfr32));
	}

	if ((size_t) msfr.msfr_nsrcs >
	    UINT32_MAX / sizeof(struct sockaddr_storage)) {
		msfr.msfr_nsrcs = UINT32_MAX / sizeof(struct sockaddr_storage);
	}

	if (msfr.msfr_nsrcs > in_mcast_maxsocksrc) {
		return ENOBUFS;
	}

	if ((msfr.msfr_fmode != MCAST_EXCLUDE &&
	    msfr.msfr_fmode != MCAST_INCLUDE)) {
		return EINVAL;
	}

	if (msfr.msfr_group.ss_family != AF_INET ||
	    msfr.msfr_group.ss_len != sizeof(struct sockaddr_in)) {
		return EINVAL;
	}

	gsa = (struct sockaddr_in *)&msfr.msfr_group;
	if (!IN_MULTICAST(ntohl(gsa->sin_addr.s_addr))) {
		return EINVAL;
	}

	gsa->sin_port = 0;                       

	ifnet_head_lock_shared();
	if (msfr.msfr_ifindex == 0 || (u_int)if_index < msfr.msfr_ifindex) {
		ifnet_head_done();
		return EADDRNOTAVAIL;
	}

	ifp = ifindex2ifnet[msfr.msfr_ifindex];
	ifnet_head_done();
	if (ifp == NULL) {
		return EADDRNOTAVAIL;
	}

	  
                                                   
    
	imo = inp_findmoptions(inp);
	if (imo == NULL) {
		return ENOMEM;
	}

	IMO_LOCK(imo);
	idx = imo_match_group(imo, ifp, gsa);
	if (idx == (size_t)-1 || imo->imo_mfilters == NULL) {
		error = EADDRNOTAVAIL;
		goto out_imo_locked;
	}
	inm = imo->imo_membership[idx];
	imf = &imo->imo_mfilters[idx];

	  
                                                  
    

	imf->imf_st[1] = (uint8_t)msfr.msfr_fmode;

	  
                                             
                                                  
                                                    
                                                
    
	if (msfr.msfr_nsrcs > 0) {
		struct in_msource       *lims;
		struct sockaddr_in      *psin;
		struct sockaddr_storage *kss, *pkss;
		int                      i;

		if (IS_64BIT_PROCESS(current_proc())) {
			tmp_ptr = msfr64.msfr_srcs;
		} else {
			tmp_ptr = CAST_USER_ADDR_T(msfr32.msfr_srcs);
		}

		IGMP_PRINTF(("%s: loading %lu source list entries\n",
		    __func__, (unsigned long)msfr.msfr_nsrcs));
		kss = _MALLOC((size_t) msfr.msfr_nsrcs * sizeof(*kss),
		    M_TEMP, M_WAITOK);
		if (kss == NULL) {
			error = ENOMEM;
			goto out_imo_locked;
		}
		error = copyin(CAST_USER_ADDR_T(tmp_ptr), kss,
		    (size_t) msfr.msfr_nsrcs * sizeof(*kss));
		if (error) {
			FREE(kss, M_TEMP);
			goto out_imo_locked;
		}

		  
                                                
                                                  
                            
     
		imf_leave(imf);
		imf->imf_st[1] = (uint8_t)msfr.msfr_fmode;

		  
                                                       
                                                     
                                                      
                                                  
                                                    
    
                                                        
                                                    
                                            
     
		for (i = 0, pkss = kss; (u_int)i < msfr.msfr_nsrcs;
		    i++, pkss++) {
			psin = (struct sockaddr_in *)pkss;
			if (psin->sin_family != AF_INET) {
				error = EAFNOSUPPORT;
				break;
			}
			if (psin->sin_len != sizeof(struct sockaddr_in)) {
				error = EINVAL;
				break;
			}
			error = imf_get_source(imf, psin, &lims);
			if (error) {
				break;
			}
			lims->imsl_st[1] = imf->imf_st[1];
		}
		FREE(kss, M_TEMP);
	}

	if (error) {
		goto out_imf_rollback;
	}

	  
                                                
    
	INM_LOCK(inm);
	IGMP_PRINTF(("%s: merge inm state\n", __func__));
	error = inm_merge(inm, imf);
	if (error) {
		IGMP_PRINTF(("%s: failed to merge inm state\n", __func__));
		INM_UNLOCK(inm);
		goto out_imf_rollback;
	}

	IGMP_PRINTF(("%s: doing igmp downcall\n", __func__));
	error = igmp_change_state(inm, &itp);
	INM_UNLOCK(inm);
#ifdef IGMP_DEBUG
	if (error) {
		IGMP_PRINTF(("%s: failed igmp downcall\n", __func__));
	}
#endif

out_imf_rollback:
	if (error) {
		imf_rollback(imf);
	} else {
		imf_commit(imf);
	}

	imf_reap(imf);

out_imo_locked:
	IMO_UNLOCK(imo);
	IMO_REMREF(imo);                                     

	                                                       
	igmp_set_timeout(&itp);

	return error;
}

  
                                                                 
  
                                                                    
                                                                       
                                                                          
                                                                       
                                                                         
   
int
inp_setmoptions(struct inpcb *inp, struct sockopt *sopt)
{
	struct ip_moptions      *imo;
	int                      error;
	unsigned int             ifindex;
	struct ifnet            *ifp;

	error = 0;

	  
                                                        
                                     
    
	if (SOCK_PROTO(inp->inp_socket) == IPPROTO_DIVERT ||
	    (SOCK_TYPE(inp->inp_socket) != SOCK_RAW &&
	    SOCK_TYPE(inp->inp_socket) != SOCK_DGRAM)) {
		return EOPNOTSUPP;
	}

	switch (sopt->sopt_name) {
	case IP_MULTICAST_IF:
		error = inp_set_multicast_if(inp, sopt);
		break;

	case IP_MULTICAST_IFINDEX:
		  
                                                         
     
		error = sooptcopyin(sopt, &ifindex, sizeof(ifindex),
		    sizeof(ifindex));
		if (error) {
			break;
		}

		imo = inp_findmoptions(inp);
		if (imo == NULL) {
			error = ENOMEM;
			break;
		}
		  
                                                    
                                                    
                                                  
     
		if (ifindex == 0) {
			IMO_LOCK(imo);
			imo->imo_multicast_ifp = NULL;
			IMO_UNLOCK(imo);
			IMO_REMREF(imo);                                     
			break;
		}

		ifnet_head_lock_shared();
		                                                               
		if ((unsigned int)if_index < ifindex) {
			ifnet_head_done();
			IMO_REMREF(imo);                                     
			error = ENXIO;                             
			break;
		}
		ifp = ifindex2ifnet[ifindex];
		ifnet_head_done();

		                                                               
		if (ifp == NULL || !(ifp->if_flags & IFF_MULTICAST)) {
			IMO_REMREF(imo);                                     
			error = EADDRNOTAVAIL;
			break;
		}
		IMO_LOCK(imo);
		imo->imo_multicast_ifp = ifp;
		  
                                                              
                                                            
                                                              
                                                               
                                       
     
		imo->imo_multicast_addr.s_addr = INADDR_ANY;
		IMO_UNLOCK(imo);
		IMO_REMREF(imo);                                     
		break;

	case IP_MULTICAST_TTL: {
		u_char ttl;

		  
                                                            
                                                         
                                                           
                                      
     
		if (sopt->sopt_valsize == sizeof(u_char)) {
			error = sooptcopyin(sopt, &ttl, sizeof(u_char),
			    sizeof(u_char));
			if (error) {
				break;
			}
		} else {
			u_int ittl;

			error = sooptcopyin(sopt, &ittl, sizeof(u_int),
			    sizeof(u_int));
			if (error) {
				break;
			}
			if (ittl > 255) {
				error = EINVAL;
				break;
			}
			ttl = (u_char)ittl;
		}
		imo = inp_findmoptions(inp);
		if (imo == NULL) {
			error = ENOMEM;
			break;
		}
		IMO_LOCK(imo);
		imo->imo_multicast_ttl = ttl;
		IMO_UNLOCK(imo);
		IMO_REMREF(imo);                                     
		break;
	}

	case IP_MULTICAST_LOOP: {
		u_char loop;

		  
                                                          
                                                                
                                                       
                                                          
     
		if (sopt->sopt_valsize == sizeof(u_char)) {
			error = sooptcopyin(sopt, &loop, sizeof(u_char),
			    sizeof(u_char));
			if (error) {
				break;
			}
		} else {
			u_int iloop;

			error = sooptcopyin(sopt, &iloop, sizeof(u_int),
			    sizeof(u_int));
			if (error) {
				break;
			}
			loop = (u_char)iloop;
		}
		imo = inp_findmoptions(inp);
		if (imo == NULL) {
			error = ENOMEM;
			break;
		}
		IMO_LOCK(imo);
		imo->imo_multicast_loop = !!loop;
		IMO_UNLOCK(imo);
		IMO_REMREF(imo);                                     
		break;
	}

	case IP_ADD_MEMBERSHIP:
	case IP_ADD_SOURCE_MEMBERSHIP:
	case MCAST_JOIN_GROUP:
	case MCAST_JOIN_SOURCE_GROUP:
		error = inp_join_group(inp, sopt);
		break;

	case IP_DROP_MEMBERSHIP:
	case IP_DROP_SOURCE_MEMBERSHIP:
	case MCAST_LEAVE_GROUP:
	case MCAST_LEAVE_SOURCE_GROUP:
		error = inp_leave_group(inp, sopt);
		break;

	case IP_BLOCK_SOURCE:
	case IP_UNBLOCK_SOURCE:
	case MCAST_BLOCK_SOURCE:
	case MCAST_UNBLOCK_SOURCE:
		error = inp_block_unblock_source(inp, sopt);
		break;

	case IP_MSFILTER:
		error = inp_set_source_filters(inp, sopt);
		break;

	default:
		error = EOPNOTSUPP;
		break;
	}

	return error;
}

  
                                                                      
                             
                                                            
                          
                          
   
static int
sysctl_ip_mcast_filters SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp)

	struct in_addr                   src = {}, group;
	struct ifnet                    *ifp;
	struct in_multi                 *inm;
	struct in_multistep             step;
	struct ip_msource               *ims;
	int                             *name;
	int                              retval = 0;
	u_int                            namelen;
	uint32_t                         fmode, ifindex;

	name = (int *)arg1;
	namelen = (u_int)arg2;

	if (req->newptr != USER_ADDR_NULL) {
		return EPERM;
	}

	if (namelen != 2) {
		return EINVAL;
	}

	ifindex = name[0];
	ifnet_head_lock_shared();
	if (ifindex <= 0 || ifindex > (u_int)if_index) {
		IGMP_PRINTF(("%s: ifindex %u out of range\n",
		    __func__, ifindex));
		ifnet_head_done();
		return ENOENT;
	}

	group.s_addr = name[1];
	if (!IN_MULTICAST(ntohl(group.s_addr))) {
		IGMP_INET_PRINTF(group,
		    ("%s: group %s is not multicast\n",
		    __func__, _igmp_inet_buf));
		ifnet_head_done();
		return EINVAL;
	}

	ifp = ifindex2ifnet[ifindex];
	ifnet_head_done();
	if (ifp == NULL) {
		IGMP_PRINTF(("%s: no ifp for ifindex %u\n", __func__, ifindex));
		return ENOENT;
	}

	in_multihead_lock_shared();
	IN_FIRST_MULTI(step, inm);
	while (inm != NULL) {
		INM_LOCK(inm);
		if (inm->inm_ifp != ifp) {
			goto next;
		}

		if (!in_hosteq(inm->inm_addr, group)) {
			goto next;
		}

		fmode = inm->inm_st[1].iss_fmode;
		retval = SYSCTL_OUT(req, &fmode, sizeof(uint32_t));
		if (retval != 0) {
			INM_UNLOCK(inm);
			break;                     
		}
		RB_FOREACH(ims, ip_msource_tree, &inm->inm_srcs) {
#ifdef IGMP_DEBUG
			struct in_addr ina;
			ina.s_addr = htonl(ims->ims_haddr);
			IGMP_INET_PRINTF(ina,
			    ("%s: visit node %s\n", __func__, _igmp_inet_buf));
#endif
			  
                                              
      
			if (fmode != ims_get_mode(inm, ims, 1)) {
				IGMP_PRINTF(("%s: skip non-in-mode\n",
				    __func__));
				continue;                          
			}
			src.s_addr = htonl(ims->ims_haddr);
			retval = SYSCTL_OUT(req, &src, sizeof(struct in_addr));
			if (retval != 0) {
				break;                        
			}
		}
next:
		INM_UNLOCK(inm);
		IN_NEXT_MULTI(step, inm);
	}
	in_multihead_lock_done();

	return retval;
}

  
      
                                                           
                                                                   
                                                                
                            
   
  
                                                                              
   
static struct ifnet *
ip_multicast_if(struct in_addr *a, unsigned int *ifindexp)
{
	unsigned int ifindex;
	struct ifnet *ifp;

	if (ifindexp != NULL) {
		*ifindexp = 0;
	}
	if (ntohl(a->s_addr) >> 24 == 0) {
		ifindex = ntohl(a->s_addr) & 0xffffff;
		ifnet_head_lock_shared();
		                                                               
		if ((unsigned int)if_index < ifindex) {
			ifnet_head_done();
			return NULL;
		}
		ifp = ifindex2ifnet[ifindex];
		ifnet_head_done();
		if (ifp != NULL && ifindexp != NULL) {
			*ifindexp = ifindex;
		}
	} else {
		INADDR_TO_IFP(*a, ifp);
	}
	return ifp;
}

void
in_multi_init(void)
{
	PE_parse_boot_argn("ifa_debug", &inm_debug, sizeof(inm_debug));

	                                                     
	in_multihead_lock_grp_attr = lck_grp_attr_alloc_init();
	in_multihead_lock_grp = lck_grp_alloc_init("in_multihead",
	    in_multihead_lock_grp_attr);
	in_multihead_lock_attr = lck_attr_alloc_init();
	lck_rw_init(&in_multihead_lock, in_multihead_lock_grp,
	    in_multihead_lock_attr);

	lck_mtx_init(&inm_trash_lock, in_multihead_lock_grp,
	    in_multihead_lock_attr);
	TAILQ_INIT(&inm_trash_head);

	vm_size_t inm_size = (inm_debug == 0) ? sizeof(struct in_multi) :
	    sizeof(struct in_multi_dbg);
	inm_zone = zone_create(INM_ZONE_NAME, inm_size, ZC_ZFREE_CLEARMEM);
}

static struct in_multi *
in_multi_alloc(zalloc_flags_t how)
{
	struct in_multi *inm;

	inm = zalloc_flags(inm_zone, how | Z_ZERO);
	if (inm != NULL) {
		lck_mtx_init(&inm->inm_lock, in_multihead_lock_grp,
		    in_multihead_lock_attr);
		inm->inm_debug |= IFD_ALLOC;
		if (inm_debug != 0) {
			inm->inm_debug |= IFD_DEBUG;
			inm->inm_trace = inm_trace;
		}
	}
	return inm;
}

static void
in_multi_free(struct in_multi *inm)
{
	INM_LOCK(inm);
	if (inm->inm_debug & IFD_ATTACHED) {
		panic("%s: attached inm=%p is being freed", __func__, inm);
		                
	} else if (inm->inm_ifma != NULL) {
		panic("%s: ifma not NULL for inm=%p", __func__, inm);
		                
	} else if (!(inm->inm_debug & IFD_ALLOC)) {
		panic("%s: inm %p cannot be freed", __func__, inm);
		                
	} else if (inm->inm_refcount != 0) {
		panic("%s: non-zero refcount inm=%p", __func__, inm);
		                
	} else if (inm->inm_reqcnt != 0) {
		panic("%s: non-zero reqcnt inm=%p", __func__, inm);
		                
	}

	                                                  
	IF_DRAIN(&inm->inm_scq);

	inm->inm_debug &= ~IFD_ALLOC;
	if ((inm->inm_debug & (IFD_DEBUG | IFD_TRASHED)) ==
	    (IFD_DEBUG | IFD_TRASHED)) {
		lck_mtx_lock(&inm_trash_lock);
		TAILQ_REMOVE(&inm_trash_head, (struct in_multi_dbg *)inm,
		    inm_trash_link);
		lck_mtx_unlock(&inm_trash_lock);
		inm->inm_debug &= ~IFD_TRASHED;
	}
	INM_UNLOCK(inm);

	lck_mtx_destroy(&inm->inm_lock, in_multihead_lock_grp);
	zfree(inm_zone, inm);
}

static void
in_multi_attach(struct in_multi *inm)
{
	in_multihead_lock_assert(LCK_RW_ASSERT_EXCLUSIVE);
	INM_LOCK_ASSERT_HELD(inm);

	if (inm->inm_debug & IFD_ATTACHED) {
		panic("%s: Attempt to attach an already attached inm=%p",
		    __func__, inm);
		                
	} else if (inm->inm_debug & IFD_TRASHED) {
		panic("%s: Attempt to reattach a detached inm=%p",
		    __func__, inm);
		                
	}

	inm->inm_reqcnt++;
	VERIFY(inm->inm_reqcnt == 1);
	INM_ADDREF_LOCKED(inm);
	inm->inm_debug |= IFD_ATTACHED;
	  
                                                    
                                                
    
	if ((inm->inm_debug & (IFD_DEBUG | IFD_TRASHED)) ==
	    (IFD_DEBUG | IFD_TRASHED)) {
		                                          
		INM_CONVERT_LOCK(inm);
		lck_mtx_lock(&inm_trash_lock);
		TAILQ_REMOVE(&inm_trash_head, (struct in_multi_dbg *)inm,
		    inm_trash_link);
		lck_mtx_unlock(&inm_trash_lock);
		inm->inm_debug &= ~IFD_TRASHED;
	}

	LIST_INSERT_HEAD(&in_multihead, inm, inm_link);
}

int
in_multi_detach(struct in_multi *inm)
{
	in_multihead_lock_assert(LCK_RW_ASSERT_EXCLUSIVE);
	INM_LOCK_ASSERT_HELD(inm);

	if (inm->inm_reqcnt == 0) {
		panic("%s: inm=%p negative reqcnt", __func__, inm);
		                
	}

	--inm->inm_reqcnt;
	if (inm->inm_reqcnt > 0) {
		return 0;
	}

	if (!(inm->inm_debug & IFD_ATTACHED)) {
		panic("%s: Attempt to detach an unattached record inm=%p",
		    __func__, inm);
		                
	} else if (inm->inm_debug & IFD_TRASHED) {
		panic("%s: inm %p is already in trash list", __func__, inm);
		                
	}

	  
                                  
    
	inm->inm_debug &= ~IFD_ATTACHED;
	LIST_REMOVE(inm, inm_link);

	if (inm->inm_debug & IFD_DEBUG) {
		                                          
		INM_CONVERT_LOCK(inm);
		lck_mtx_lock(&inm_trash_lock);
		TAILQ_INSERT_TAIL(&inm_trash_head,
		    (struct in_multi_dbg *)inm, inm_trash_link);
		lck_mtx_unlock(&inm_trash_lock);
		inm->inm_debug |= IFD_TRASHED;
	}

	return 1;
}

void
inm_addref(struct in_multi *inm, int locked)
{
	if (!locked) {
		INM_LOCK_SPIN(inm);
	} else {
		INM_LOCK_ASSERT_HELD(inm);
	}

	if (++inm->inm_refcount == 0) {
		panic("%s: inm=%p wraparound refcnt", __func__, inm);
		                
	} else if (inm->inm_trace != NULL) {
		(*inm->inm_trace)(inm, TRUE);
	}
	if (!locked) {
		INM_UNLOCK(inm);
	}
}

void
inm_remref(struct in_multi *inm, int locked)
{
	struct ifmultiaddr *ifma;
	struct igmp_ifinfo *igi;

	if (!locked) {
		INM_LOCK_SPIN(inm);
	} else {
		INM_LOCK_ASSERT_HELD(inm);
	}

	if (inm->inm_refcount == 0 || (inm->inm_refcount == 1 && locked)) {
		panic("%s: inm=%p negative/missing refcnt", __func__, inm);
		                
	} else if (inm->inm_trace != NULL) {
		(*inm->inm_trace)(inm, FALSE);
	}

	--inm->inm_refcount;
	if (inm->inm_refcount > 0) {
		if (!locked) {
			INM_UNLOCK(inm);
		}
		return;
	}

	  
                                                                      
                                                                     
                                                                      
                                                                  
                                                                     
                                                                        
                                                                     
                                                                      
                                                  
    
	++inm->inm_refcount;
	INM_UNLOCK(inm);
	in_multihead_lock_shared();
	INM_LOCK_SPIN(inm);
	--inm->inm_refcount;
	if (inm->inm_refcount > 0) {
		                                                             
		INM_UNLOCK(inm);
		in_multihead_lock_done();
		                                         
		if (locked) {
			INM_LOCK(inm);
		}
		return;
	}
	inm_purge(inm);
	ifma = inm->inm_ifma;
	inm->inm_ifma = NULL;
	inm->inm_ifp = NULL;
	igi = inm->inm_igi;
	inm->inm_igi = NULL;
	INM_UNLOCK(inm);
	IFMA_LOCK_SPIN(ifma);
	ifma->ifma_protospec = NULL;
	IFMA_UNLOCK(ifma);
	in_multihead_lock_done();

	in_multi_free(inm);
	if_delmulti_ifma(ifma);
	                                                          
	IFMA_REMREF(ifma);

	if (igi != NULL) {
		IGI_REMREF(igi);
	}
}

static void
inm_trace(struct in_multi *inm, int refhold)
{
	struct in_multi_dbg *inm_dbg = (struct in_multi_dbg *)inm;
	ctrace_t *tr;
	u_int32_t idx;
	u_int16_t *cnt;

	if (!(inm->inm_debug & IFD_DEBUG)) {
		panic("%s: inm %p has no debug structure", __func__, inm);
		                
	}
	if (refhold) {
		cnt = &inm_dbg->inm_refhold_cnt;
		tr = inm_dbg->inm_refhold;
	} else {
		cnt = &inm_dbg->inm_refrele_cnt;
		tr = inm_dbg->inm_refrele;
	}

	idx = atomic_add_16_ov(cnt, 1) % INM_TRACE_HIST_SIZE;
	ctrace_record(&tr[idx]);
}

void
in_multihead_lock_exclusive(void)
{
	lck_rw_lock_exclusive(&in_multihead_lock);
}

void
in_multihead_lock_shared(void)
{
	lck_rw_lock_shared(&in_multihead_lock);
}

void
in_multihead_lock_assert(int what)
{
#if !MACH_ASSERT
#pragma unused(what)
#endif
	LCK_RW_ASSERT(&in_multihead_lock, what);
}

void
in_multihead_lock_done(void)
{
	lck_rw_done(&in_multihead_lock);
}

static struct ip_msource *
ipms_alloc(zalloc_flags_t how)
{
	return zalloc_flags(ipms_zone, how | Z_ZERO);
}

static void
ipms_free(struct ip_msource *ims)
{
	zfree(ipms_zone, ims);
}

static struct in_msource *
inms_alloc(zalloc_flags_t how)
{
	return zalloc_flags(inms_zone, how | Z_ZERO);
}

static void
inms_free(struct in_msource *inms)
{
	zfree(inms_zone, inms);
}

#ifdef IGMP_DEBUG

static const char *inm_modestrs[] = { "un\n", "in", "ex" };

static const char *
inm_mode_str(const int mode)
{
	if (mode >= MCAST_UNDEFINED && mode <= MCAST_EXCLUDE) {
		return inm_modestrs[mode];
	}
	return "??";
}

static const char *inm_statestrs[] = {
	"not-member\n",
	"silent\n",
	"reporting\n",
	"idle\n",
	"lazy\n",
	"sleeping\n",
	"awakening\n",
	"query-pending\n",
	"sg-query-pending\n",
	"leaving"
};

static const char *
inm_state_str(const int state)
{
	if (state >= IGMP_NOT_MEMBER && state <= IGMP_LEAVING_MEMBER) {
		return inm_statestrs[state];
	}
	return "??";
}

  
                                             
   
void
inm_print(const struct in_multi *inm)
{
	int t;
	char buf[MAX_IPv4_STR_LEN];

	INM_LOCK_ASSERT_HELD(__DECONST(struct in_multi *, inm));

	if (igmp_debug == 0) {
		return;
	}

	inet_ntop(AF_INET, &inm->inm_addr, buf, sizeof(buf));
	printf("%s: --- begin inm 0x%llx ---\n", __func__,
	    (uint64_t)VM_KERNEL_ADDRPERM(inm));
	printf("addr %s ifp 0x%llx(%s) ifma 0x%llx\n",
	    buf,
	    (uint64_t)VM_KERNEL_ADDRPERM(inm->inm_ifp),
	    if_name(inm->inm_ifp),
	    (uint64_t)VM_KERNEL_ADDRPERM(inm->inm_ifma));
	printf("timer %u state %s refcount %u scq.len %u\n",
	    inm->inm_timer,
	    inm_state_str(inm->inm_state),
	    inm->inm_refcount,
	    inm->inm_scq.ifq_len);
	printf("igi 0x%llx nsrc %lu sctimer %u scrv %u\n",
	    (uint64_t)VM_KERNEL_ADDRPERM(inm->inm_igi),
	    inm->inm_nsrc,
	    inm->inm_sctimer,
	    inm->inm_scrv);
	for (t = 0; t < 2; t++) {
		printf("t%d: fmode %s asm %u ex %u in %u rec %u\n", t,
		    inm_mode_str(inm->inm_st[t].iss_fmode),
		    inm->inm_st[t].iss_asm,
		    inm->inm_st[t].iss_ex,
		    inm->inm_st[t].iss_in,
		    inm->inm_st[t].iss_rec);
	}
	printf("%s: --- end inm 0x%llx ---\n", __func__,
	    (uint64_t)VM_KERNEL_ADDRPERM(inm));
}

#else

void
inm_print(__unused const struct in_multi *inm)
{
}

#endif
