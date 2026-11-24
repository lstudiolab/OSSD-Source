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
 * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
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
 * Copyright (c) 1982, 1986, 1991, 1993
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
 *	@(#)in.c	8.2 (Berkeley) 11/15/93
 */


#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sockio.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/kern_event.h>
#include <sys/mcache.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>

#include <kern/locks.h>
#include <kern/zalloc.h>
#include <kern/clock.h>
#include <libkern/OSAtomic.h>
#include <machine/machine_routines.h>
#include <mach/boolean.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_var.h>
#include <net/route.h>
#include <net/if_dl.h>
#include <net/kpi_protocol.h>
#include <net/nwk_wq.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/icmp6.h>
#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_var.h>

#include <netinet6/nd6.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/mld6_var.h>
#include <netinet6/in6_ifattach.h>
#include <netinet6/scope6_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/in6_pcb.h>

#include <net/net_osdep.h>

#include <net/dlil.h>
#include <net/if_llatbl.h>

#if PF
#include <net/pfvar.h>
#endif         

  
                                             
   
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
const struct in6_addr in6addr_nodelocal_allnodes =
    IN6ADDR_NODELOCAL_ALLNODES_INIT;
const struct in6_addr in6addr_linklocal_allnodes =
    IN6ADDR_LINKLOCAL_ALLNODES_INIT;
const struct in6_addr in6addr_linklocal_allrouters =
    IN6ADDR_LINKLOCAL_ALLROUTERS_INIT;
const struct in6_addr in6addr_linklocal_allv2routers =
    IN6ADDR_LINKLOCAL_ALLV2ROUTERS_INIT;
const struct in6_addr in6addr_multicast_prefix =
    IN6ADDR_MULTICAST_PREFIX;

const struct in6_addr in6mask0 = IN6MASK0;
const struct in6_addr in6mask7 = IN6MASK7;
const struct in6_addr in6mask8 = IN6MASK8;
const struct in6_addr in6mask16 = IN6MASK16;
const struct in6_addr in6mask32 = IN6MASK32;
const struct in6_addr in6mask64 = IN6MASK64;
const struct in6_addr in6mask96 = IN6MASK96;
const struct in6_addr in6mask128 = IN6MASK128;

const struct sockaddr_in6 sa6_any = {
	.sin6_len = sizeof(sa6_any),
	.sin6_family = AF_INET6,
	.sin6_port = 0,
	.sin6_flowinfo = 0,
	.sin6_addr = IN6ADDR_ANY_INIT,
	.sin6_scope_id = 0
};

static int in6ctl_associd(struct socket *, u_long, caddr_t);
static int in6ctl_connid(struct socket *, u_long, caddr_t);
static int in6ctl_conninfo(struct socket *, u_long, caddr_t);
static int in6ctl_llstart(struct ifnet *, u_long, caddr_t);
static int in6ctl_llstop(struct ifnet *);
static int in6ctl_cgastart(struct ifnet *, u_long, caddr_t);
static int in6ctl_gifaddr(struct ifnet *, struct in6_ifaddr *, u_long,
    struct in6_ifreq *);
static int in6ctl_gifstat(struct ifnet *, u_long, struct in6_ifreq *);
static int in6ctl_alifetime(struct in6_ifaddr *, u_long, struct in6_ifreq *,
    boolean_t);
static int in6ctl_aifaddr(struct ifnet *, struct in6_aliasreq *);
static void in6ctl_difaddr(struct ifnet *, struct in6_ifaddr *);
static int in6_autoconf(struct ifnet *, int);
static int in6_setrouter(struct ifnet *, ipv6_router_mode_t);
static int in6_ifinit(struct ifnet *, struct in6_ifaddr *, int);
static int in6_ifaupdate_aux(struct in6_ifaddr *, struct ifnet *, int);
static void in6_unlink_ifa(struct in6_ifaddr *, struct ifnet *);
static struct in6_ifaddr *in6_ifaddr_alloc(zalloc_flags_t);
static void in6_ifaddr_attached(struct ifaddr *);
static void in6_ifaddr_detached(struct ifaddr *);
static void in6_ifaddr_free(struct ifaddr *);
static void in6_ifaddr_trace(struct ifaddr *, int);
#if defined(__LP64__)
static void in6_cgareq_32_to_64(const struct in6_cgareq_32 *,
    struct in6_cgareq_64 *);
#else
static void in6_cgareq_64_to_32(const struct in6_cgareq_64 *,
    struct in6_cgareq_32 *);
#endif
static struct in6_aliasreq *in6_aliasreq_to_native(void *, int,
    struct in6_aliasreq *);
static int in6_to_kamescope(struct sockaddr_in6 *, struct ifnet *);
static int in6_getassocids(struct socket *, uint32_t *, user_addr_t);
static int in6_getconnids(struct socket *, sae_associd_t, uint32_t *,
    user_addr_t);

static void in6_if_up_dad_start(struct ifnet *);

#define IA6_HASH_INIT(ia) {                                      \
	(ia)->ia6_hash.tqe_next = (void *)(uintptr_t)-1;         \
	(ia)->ia6_hash.tqe_prev = (void *)(uintptr_t)-1;         \
}

#define IA6_IS_HASHED(ia)                                        \
	(!((ia)->ia6_hash.tqe_next == (void *)(uintptr_t)-1 ||   \
	(ia)->ia6_hash.tqe_prev == (void *)(uintptr_t)-1))

static void in6_iahash_remove(struct in6_ifaddr *);
static void in6_iahash_insert(struct in6_ifaddr *);
static void in6_iahash_insert_ptp(struct in6_ifaddr *);

extern lck_mtx_t *nd6_mutex;

#define IN6IFA_TRACE_HIST_SIZE  32                                 

             
__private_extern__ unsigned int in6ifa_trace_hist_size = IN6IFA_TRACE_HIST_SIZE;

struct in6_ifaddr_dbg {
	struct in6_ifaddr       in6ifa;                                 
	struct in6_ifaddr       in6ifa_old;                                   
	u_int16_t               in6ifa_refhold_cnt;                          
	u_int16_t               in6ifa_refrele_cnt;                          
	  
                           
    
	ctrace_t                in6ifa_alloc;
	ctrace_t                in6ifa_free;
	  
                                                        
    
	ctrace_t                in6ifa_refhold[IN6IFA_TRACE_HIST_SIZE];
	ctrace_t                in6ifa_refrele[IN6IFA_TRACE_HIST_SIZE];
	  
                      
    
	TAILQ_ENTRY(in6_ifaddr_dbg) in6ifa_trash_link;
};

                                                                     
static TAILQ_HEAD(, in6_ifaddr_dbg) in6ifa_trash_head;
static decl_lck_mtx_data(, in6ifa_trash_lock);

#if DEBUG
static unsigned int in6ifa_debug = 1;                                    
#else
static unsigned int in6ifa_debug;                                         
#endif             
static struct zone *in6ifa_zone;                                         
#define IN6IFA_ZONE_NAME        "in6_ifaddr"                   

struct eventhandler_lists_ctxt in6_evhdlr_ctxt;
struct eventhandler_lists_ctxt in6_clat46_evhdlr_ctxt;
  
                                                      
                                 
   
static void
in6_ifloop_request(int cmd, struct ifaddr *ifa)
{
	struct sockaddr_in6 all1_sa;
	struct rtentry *nrt = NULL;
	int e;

	bzero(&all1_sa, sizeof(all1_sa));
	all1_sa.sin6_family = AF_INET6;
	all1_sa.sin6_len = sizeof(struct sockaddr_in6);
	all1_sa.sin6_addr = in6mask128;

	  
                                                             
                                                                    
                                                                    
                                                                   
                                                                    
                                                                   
                                                                     
    
	lck_mtx_lock(rnh_lock);
	e = rtrequest_locked(cmd, ifa->ifa_addr, ifa->ifa_addr,
	    (struct sockaddr *)&all1_sa, RTF_UP | RTF_HOST | RTF_LLINFO, &nrt);
	if (e != 0) {
		log(LOG_ERR, "in6_ifloop_request: "
		    "%s operation failed for %s (errno=%d)\n",
		    cmd == RTM_ADD ? "ADD" : "DELETE",
		    ip6_sprintf(&((struct in6_ifaddr *)ifa)->ia_addr.sin6_addr),
		    e);
	}

	if (nrt != NULL) {
		RT_LOCK(nrt);
	}
	  
                                                                
             
                                                              
                                                                      
                            
    
	if (cmd == RTM_ADD && nrt && ifa != nrt->rt_ifa) {
		rtsetifa(nrt, ifa);
	}

	  
                                                                     
                                                                       
                                                                 
                             
    
	if (nrt != NULL) {
		rt_newaddrmsg(cmd, ifa, e, nrt);
		if (cmd == RTM_DELETE) {
			RT_UNLOCK(nrt);
			rtfree_locked(nrt);
		} else {
			                                  
			RT_REMREF_LOCKED(nrt);
			RT_UNLOCK(nrt);
		}
	}
	lck_mtx_unlock(rnh_lock);
}

  
                                                                        
                                                                         
                                                                      
                                                                       
            
   
static void
in6_ifaddloop(struct ifaddr *ifa)
{
	struct rtentry *rt;

	  
                                                              
                                                        
    
	rt = rtalloc1(ifa->ifa_addr, 0, 0);
	if (rt != NULL) {
		RT_LOCK(rt);
	}
	if (rt == NULL || (rt->rt_flags & RTF_HOST) == 0 ||
	    (rt->rt_ifp->if_flags & IFF_LOOPBACK) == 0) {
		if (rt != NULL) {
			RT_REMREF_LOCKED(rt);
			RT_UNLOCK(rt);
		}
		in6_ifloop_request(RTM_ADD, ifa);
	} else if (rt != NULL) {
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
	}
}

  
                                                                   
                
   
static void
in6_ifremloop(struct ifaddr *ifa)
{
	struct in6_ifaddr *ia;
	struct rtentry *rt;
	int ia_count = 0;

	  
                                                    
                                                                  
                                                                        
                                                                   
                                                           
                                                                       
                                                     
    

	  
                                                                     
                                                            
                              
                                                        
    
	lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
	TAILQ_FOREACH(ia, IN6ADDR_HASH(IFA_IN6(ifa)), ia6_hash) {
		IFA_LOCK(&ia->ia_ifa);
		if (IN6_ARE_ADDR_EQUAL(IFA_IN6(ifa), &ia->ia_addr.sin6_addr)) {
			ia_count++;
			if (ia_count > 1) {
				IFA_UNLOCK(&ia->ia_ifa);
				break;
			}
		}
		IFA_UNLOCK(&ia->ia_ifa);
	}
	lck_rw_done(&in6_ifaddr_rwlock);

	if (ia_count == 1) {
		  
                                                              
                                                           
                                                               
                                                                 
                                                             
                                                               
                                
     
		rt = rtalloc1(ifa->ifa_addr, 0, 0);
		if (rt != NULL) {
			RT_LOCK(rt);
			if ((rt->rt_flags & RTF_HOST) != 0 &&
			    (rt->rt_ifp->if_flags & IFF_LOOPBACK) != 0) {
				RT_REMREF_LOCKED(rt);
				RT_UNLOCK(rt);
				in6_ifloop_request(RTM_DELETE, ifa);
			} else {
				RT_UNLOCK(rt);
			}
		}
	}
}


int
in6_mask2len(struct in6_addr *mask, u_char *lim0)
{
	int x = 0, y;
	u_char *lim = lim0, *p;

	                              
	if (lim0 == NULL || lim0 - (u_char *)mask > sizeof(*mask)) {
		lim = (u_char *)mask + sizeof(*mask);
	}
	for (p = (u_char *)mask; p < lim; x++, p++) {
		if (*p != 0xff) {
			break;
		}
	}
	y = 0;
	if (p < lim) {
		for (y = 0; y < 8; y++) {
			if ((*p & (0x80 >> y)) == 0) {
				break;
			}
		}
	}

	  
                                                               
                   
    
	if (p < lim) {
		if (y != 0 && (*p & (0x00ff >> y)) != 0) {
			return -1;
		}
		for (p = p + 1; p < lim; p++) {
			if (*p != 0) {
				return -1;
			}
		}
	}

	return x * 8 + y;
}

void
in6_len2mask(struct in6_addr *mask, int len)
{
	int i;

	bzero(mask, sizeof(*mask));
	for (i = 0; i < len / 8; i++) {
		mask->s6_addr8[i] = 0xff;
	}
	if (len % 8) {
		mask->s6_addr8[i] = (0xff00 >> (len % 8)) & 0xff;
	}
}

void
in6_aliasreq_64_to_32(struct in6_aliasreq_64 *src, struct in6_aliasreq_32 *dst)
{
	bzero(dst, sizeof(*dst));
	bcopy(src->ifra_name, dst->ifra_name, sizeof(dst->ifra_name));
	dst->ifra_addr = src->ifra_addr;
	dst->ifra_dstaddr = src->ifra_dstaddr;
	dst->ifra_prefixmask = src->ifra_prefixmask;
	dst->ifra_flags = src->ifra_flags;
	dst->ifra_lifetime.ia6t_expire = (u_int32_t)src->ifra_lifetime.ia6t_expire;
	dst->ifra_lifetime.ia6t_preferred = (u_int32_t)src->ifra_lifetime.ia6t_preferred;
	dst->ifra_lifetime.ia6t_vltime = src->ifra_lifetime.ia6t_vltime;
	dst->ifra_lifetime.ia6t_pltime = src->ifra_lifetime.ia6t_pltime;
}

void
in6_aliasreq_32_to_64(struct in6_aliasreq_32 *src, struct in6_aliasreq_64 *dst)
{
	bzero(dst, sizeof(*dst));
	bcopy(src->ifra_name, dst->ifra_name, sizeof(dst->ifra_name));
	dst->ifra_addr = src->ifra_addr;
	dst->ifra_dstaddr = src->ifra_dstaddr;
	dst->ifra_prefixmask = src->ifra_prefixmask;
	dst->ifra_flags = src->ifra_flags;
	dst->ifra_lifetime.ia6t_expire = src->ifra_lifetime.ia6t_expire;
	dst->ifra_lifetime.ia6t_preferred = src->ifra_lifetime.ia6t_preferred;
	dst->ifra_lifetime.ia6t_vltime = src->ifra_lifetime.ia6t_vltime;
	dst->ifra_lifetime.ia6t_pltime = src->ifra_lifetime.ia6t_pltime;
}

#if defined(__LP64__)
static void
in6_cgareq_32_to_64(const struct in6_cgareq_32 *src,
    struct in6_cgareq_64 *dst)
{
	bzero(dst, sizeof(*dst));
	bcopy(src->cgar_name, dst->cgar_name, sizeof(dst->cgar_name));
	dst->cgar_flags = src->cgar_flags;
	bcopy(src->cgar_cgaprep.cga_modifier.octets,
	    dst->cgar_cgaprep.cga_modifier.octets,
	    sizeof(dst->cgar_cgaprep.cga_modifier.octets));
	dst->cgar_cgaprep.cga_security_level =
	    src->cgar_cgaprep.cga_security_level;
	dst->cgar_lifetime.ia6t_expire = src->cgar_lifetime.ia6t_expire;
	dst->cgar_lifetime.ia6t_preferred = src->cgar_lifetime.ia6t_preferred;
	dst->cgar_lifetime.ia6t_vltime = src->cgar_lifetime.ia6t_vltime;
	dst->cgar_lifetime.ia6t_pltime = src->cgar_lifetime.ia6t_pltime;
	dst->cgar_collision_count = src->cgar_collision_count;
}
#endif

#if !defined(__LP64__)
static void
in6_cgareq_64_to_32(const struct in6_cgareq_64 *src,
    struct in6_cgareq_32 *dst)
{
	bzero(dst, sizeof(*dst));
	bcopy(src->cgar_name, dst->cgar_name, sizeof(dst->cgar_name));
	dst->cgar_flags = src->cgar_flags;
	bcopy(src->cgar_cgaprep.cga_modifier.octets,
	    dst->cgar_cgaprep.cga_modifier.octets,
	    sizeof(dst->cgar_cgaprep.cga_modifier.octets));
	dst->cgar_cgaprep.cga_security_level =
	    src->cgar_cgaprep.cga_security_level;
	dst->cgar_lifetime.ia6t_expire = src->cgar_lifetime.ia6t_expire;
	dst->cgar_lifetime.ia6t_preferred = src->cgar_lifetime.ia6t_preferred;
	dst->cgar_lifetime.ia6t_vltime = src->cgar_lifetime.ia6t_vltime;
	dst->cgar_lifetime.ia6t_pltime = src->cgar_lifetime.ia6t_pltime;
	dst->cgar_collision_count = src->cgar_collision_count;
}
#endif

static struct in6_aliasreq *
in6_aliasreq_to_native(void *data, int data_is_64, struct in6_aliasreq *dst)
{
#if defined(__LP64__)
	if (data_is_64) {
		bcopy(data, dst, sizeof(*dst));
	} else {
		in6_aliasreq_32_to_64((struct in6_aliasreq_32 *)data,
		    (struct in6_aliasreq_64 *)dst);
	}
#else
	if (data_is_64) {
		in6_aliasreq_64_to_32((struct in6_aliasreq_64 *)data,
		    (struct in6_aliasreq_32 *)dst);
	} else {
		bcopy(data, dst, sizeof(*dst));
	}
#endif               
	return dst;
}

void
in6_cgareq_copy_from_user(const void *user_data, int user_is_64,
    struct in6_cgareq *cgareq)
{
#if defined(__LP64__)
	if (user_is_64) {
		bcopy(user_data, cgareq, sizeof(*cgareq));
	} else {
		in6_cgareq_32_to_64((const struct in6_cgareq_32 *)user_data,
		    (struct in6_cgareq_64 *)cgareq);
	}
#else
	if (user_is_64) {
		in6_cgareq_64_to_32((const struct in6_cgareq_64 *)user_data,
		    (struct in6_cgareq_32 *)cgareq);
	} else {
		bcopy(user_data, cgareq, sizeof(*cgareq));
	}
#endif               
}

static __attribute__((noinline)) int
in6ctl_associd(struct socket *so, u_long cmd, caddr_t data)
{
	int error = 0;
	union {
		struct so_aidreq32 a32;
		struct so_aidreq64 a64;
	} u;

	VERIFY(so != NULL);

	switch (cmd) {
	case SIOCGASSOCIDS32: {                                 
		bcopy(data, &u.a32, sizeof(u.a32));
		error = in6_getassocids(so, &u.a32.sar_cnt, u.a32.sar_aidp);
		if (error == 0) {
			bcopy(&u.a32, data, sizeof(u.a32));
		}
		break;
	}

	case SIOCGASSOCIDS64: {                                 
		bcopy(data, &u.a64, sizeof(u.a64));
		error = in6_getassocids(so, &u.a64.sar_cnt, u.a64.sar_aidp);
		if (error == 0) {
			bcopy(&u.a64, data, sizeof(u.a64));
		}
		break;
	}

	default:
		VERIFY(0);
		                
	}

	return error;
}

static __attribute__((noinline)) int
in6ctl_connid(struct socket *so, u_long cmd, caddr_t data)
{
	int error = 0;
	union {
		struct so_cidreq32 c32;
		struct so_cidreq64 c64;
	} u;

	VERIFY(so != NULL);

	switch (cmd) {
	case SIOCGCONNIDS32: {                                  
		bcopy(data, &u.c32, sizeof(u.c32));
		error = in6_getconnids(so, u.c32.scr_aid, &u.c32.scr_cnt,
		    u.c32.scr_cidp);
		if (error == 0) {
			bcopy(&u.c32, data, sizeof(u.c32));
		}
		break;
	}

	case SIOCGCONNIDS64: {                                  
		bcopy(data, &u.c64, sizeof(u.c64));
		error = in6_getconnids(so, u.c64.scr_aid, &u.c64.scr_cnt,
		    u.c64.scr_cidp);
		if (error == 0) {
			bcopy(&u.c64, data, sizeof(u.c64));
		}
		break;
	}

	default:
		VERIFY(0);
		                
	}

	return error;
}

static __attribute__((noinline)) int
in6ctl_conninfo(struct socket *so, u_long cmd, caddr_t data)
{
	int error = 0;
	union {
		struct so_cinforeq32 ci32;
		struct so_cinforeq64 ci64;
	} u;

	VERIFY(so != NULL);

	switch (cmd) {
	case SIOCGCONNINFO32: {                                   
		bcopy(data, &u.ci32, sizeof(u.ci32));
		error = in6_getconninfo(so, u.ci32.scir_cid, &u.ci32.scir_flags,
		    &u.ci32.scir_ifindex, &u.ci32.scir_error, u.ci32.scir_src,
		    &u.ci32.scir_src_len, u.ci32.scir_dst, &u.ci32.scir_dst_len,
		    &u.ci32.scir_aux_type, u.ci32.scir_aux_data,
		    &u.ci32.scir_aux_len);
		if (error == 0) {
			bcopy(&u.ci32, data, sizeof(u.ci32));
		}
		break;
	}

	case SIOCGCONNINFO64: {                                   
		bcopy(data, &u.ci64, sizeof(u.ci64));
		error = in6_getconninfo(so, u.ci64.scir_cid, &u.ci64.scir_flags,
		    &u.ci64.scir_ifindex, &u.ci64.scir_error, u.ci64.scir_src,
		    &u.ci64.scir_src_len, u.ci64.scir_dst, &u.ci64.scir_dst_len,
		    &u.ci64.scir_aux_type, u.ci64.scir_aux_data,
		    &u.ci64.scir_aux_len);
		if (error == 0) {
			bcopy(&u.ci64, data, sizeof(u.ci64));
		}
		break;
	}

	default:
		VERIFY(0);
		                
	}

	return error;
}

static __attribute__((noinline)) int
in6ctl_llstart(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct in6_aliasreq sifra, *ifra = NULL;
	boolean_t is64;
	int error = 0;

	VERIFY(ifp != NULL);

	switch (cmd) {
	case SIOCLL_START_32:                                       
	case SIOCLL_START_64:                                       
		is64 = (cmd == SIOCLL_START_64);
		  
                                                            
                                                             
                                                                
                                                 
     
		ifra = in6_aliasreq_to_native(data, is64, &sifra);

		  
                                                              
                                                                 
                                     
     
		if (ifra->ifra_addr.sin6_family == AF_INET6 &&
		                                          
		    (ifra->ifra_dstaddr.sin6_len == 0 ||
		    ifra->ifra_dstaddr.sin6_family == AF_INET6)) {
			                                                     
			error = in6_ifattach_aliasreq(ifp, NULL, ifra);
		} else {
			error = in6_ifattach_aliasreq(ifp, NULL, NULL);
		}
		if (error == 0) {
			in6_if_up_dad_start(ifp);
		}
		break;

	default:
		VERIFY(0);
		                
	}

	return error;
}

static __attribute__((noinline)) int
in6ctl_llstop(struct ifnet *ifp)
{
	struct in6_ifaddr *ia;
	struct nd_prefix pr0, *pr;

	VERIFY(ifp != NULL);

	                                                
	lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
	boolean_t from_begining = TRUE;
	while (from_begining) {
		from_begining = FALSE;
		TAILQ_FOREACH(ia, &in6_ifaddrhead, ia6_link) {
			if (ia->ia_ifa.ifa_ifp != ifp) {
				continue;
			}
			IFA_LOCK(&ia->ia_ifa);
			if (IN6_IS_ADDR_LINKLOCAL(&ia->ia_addr.sin6_addr)) {
				IFA_ADDREF_LOCKED(&ia->ia_ifa);             
				IFA_UNLOCK(&ia->ia_ifa);
				lck_rw_done(&in6_ifaddr_rwlock);
				in6_purgeaddr(&ia->ia_ifa);
				IFA_REMREF(&ia->ia_ifa);                    
				lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
				  
                                                   
                                    
                                                
                           
       
				from_begining = TRUE;
				break;
			}
			IFA_UNLOCK(&ia->ia_ifa);
		}
	}
	lck_rw_done(&in6_ifaddr_rwlock);

	                                  
	bzero(&pr0, sizeof(pr0));
	pr0.ndpr_plen = 64;
	pr0.ndpr_ifp = ifp;
	pr0.ndpr_prefix.sin6_addr.s6_addr16[0] = IPV6_ADDR_INT16_ULL;
	(void)in6_setscope(&pr0.ndpr_prefix.sin6_addr, ifp, NULL);
	pr = nd6_prefix_lookup(&pr0, ND6_PREFIX_EXPIRY_UNSPEC);
	if (pr) {
		lck_mtx_lock(nd6_mutex);
		NDPR_LOCK(pr);
		prelist_remove(pr);
		NDPR_UNLOCK(pr);
		NDPR_REMREF(pr);                                     
		lck_mtx_unlock(nd6_mutex);
	}

	return 0;
}

  
                                                    
   
static __attribute__((noinline)) int
in6ctl_cgastart(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct in6_cgareq llcgasr;
	int is64, error = 0;

	VERIFY(ifp != NULL);

	switch (cmd) {
	case SIOCLL_CGASTART_32:                                  
	case SIOCLL_CGASTART_64:                                  
		is64 = (cmd == SIOCLL_CGASTART_64);
		  
                                                              
                                                             
                                                                
                                                 
     
		in6_cgareq_copy_from_user(data, is64, &llcgasr);

		  
                                                       
                                                     
                                                  
             
     
		error = in6_ifattach_llcgareq(ifp, &llcgasr);
		if (error == 0) {
			in6_if_up_dad_start(ifp);
		}
		break;

	default:
		VERIFY(0);
		                
	}

	return error;
}

  
                                                                       
                                                                          
                   
   
static __attribute__((noinline)) int
in6ctl_gifaddr(struct ifnet *ifp, struct in6_ifaddr *ia, u_long cmd,
    struct in6_ifreq *ifr)
{
	struct sockaddr_in6 addr;
	int error = 0;

	VERIFY(ifp != NULL);

	if (ia == NULL) {
		return EADDRNOTAVAIL;
	}

	switch (cmd) {
	case SIOCGIFADDR_IN6:                                 
		IFA_LOCK(&ia->ia_ifa);
		bcopy(&ia->ia_addr, &addr, sizeof(addr));
		IFA_UNLOCK(&ia->ia_ifa);
		if ((error = sa6_recoverscope(&addr, TRUE)) != 0) {
			break;
		}
		bcopy(&addr, &ifr->ifr_addr, sizeof(addr));
		break;

	case SIOCGIFDSTADDR_IN6:                              
		if (!(ifp->if_flags & IFF_POINTOPOINT)) {
			error = EINVAL;
			break;
		}
		  
                                                           
              
     
		IFA_LOCK(&ia->ia_ifa);
		bcopy(&ia->ia_dstaddr, &addr, sizeof(addr));
		IFA_UNLOCK(&ia->ia_ifa);
		if ((error = sa6_recoverscope(&addr, TRUE)) != 0) {
			break;
		}
		bcopy(&addr, &ifr->ifr_dstaddr, sizeof(addr));
		break;

	default:
		VERIFY(0);
		                
	}

	return error;
}

  
                                                                       
                                                                          
                   
   
static __attribute__((noinline)) int
in6ctl_gifstat(struct ifnet *ifp, u_long cmd, struct in6_ifreq *ifr)
{
	int error = 0, index;

	VERIFY(ifp != NULL);
	index = ifp->if_index;

	switch (cmd) {
	case SIOCGIFSTAT_IN6:                                 
		                                                 
		if (IN6_IFEXTRA(ifp) == NULL) {
			                            
			bzero(&ifr->ifr_ifru.ifru_stat,
			    sizeof(ifr->ifr_ifru.ifru_stat));
		} else {
			bcopy(&IN6_IFEXTRA(ifp)->in6_ifstat,
			    &ifr->ifr_ifru.ifru_stat,
			    sizeof(ifr->ifr_ifru.ifru_stat));
		}
		break;

	case SIOCGIFSTAT_ICMP6:                               
		                                                 
		if (IN6_IFEXTRA(ifp) == NULL) {
			                            
			bzero(&ifr->ifr_ifru.ifru_icmp6stat,
			    sizeof(ifr->ifr_ifru.ifru_icmp6stat));
		} else {
			bcopy(&IN6_IFEXTRA(ifp)->icmp6_ifstat,
			    &ifr->ifr_ifru.ifru_icmp6stat,
			    sizeof(ifr->ifr_ifru.ifru_icmp6stat));
		}
		break;

	default:
		VERIFY(0);
		                
	}

	return error;
}

  
                                                                       
                                                                          
                   
   
static __attribute__((noinline)) int
in6ctl_alifetime(struct in6_ifaddr *ia, u_long cmd, struct in6_ifreq *ifr,
    boolean_t p64)
{
	uint64_t timenow = net_uptime();
	struct in6_addrlifetime ia6_lt;
	struct timeval caltime;
	int error = 0;

	if (ia == NULL) {
		return EADDRNOTAVAIL;
	}

	switch (cmd) {
	case SIOCGIFALIFETIME_IN6:                            
		IFA_LOCK(&ia->ia_ifa);
		                                                    
		in6ifa_getlifetime(ia, &ia6_lt, 1);
		if (p64) {
			struct in6_addrlifetime_64 lt;

			bzero(&lt, sizeof(lt));
			lt.ia6t_expire = ia6_lt.ia6t_expire;
			lt.ia6t_preferred = ia6_lt.ia6t_preferred;
			lt.ia6t_vltime = ia6_lt.ia6t_vltime;
			lt.ia6t_pltime = ia6_lt.ia6t_pltime;
			bcopy(&lt, &ifr->ifr_ifru.ifru_lifetime, sizeof(ifr->ifr_ifru.ifru_lifetime));
		} else {
			struct in6_addrlifetime_32 lt;

			bzero(&lt, sizeof(lt));
			lt.ia6t_expire = (uint32_t)ia6_lt.ia6t_expire;
			lt.ia6t_preferred = (uint32_t)ia6_lt.ia6t_preferred;
			lt.ia6t_vltime = (uint32_t)ia6_lt.ia6t_vltime;
			lt.ia6t_pltime = (uint32_t)ia6_lt.ia6t_pltime;
			bcopy(&lt, &ifr->ifr_ifru.ifru_lifetime, sizeof(ifr->ifr_ifru.ifru_lifetime));
		}
		IFA_UNLOCK(&ia->ia_ifa);
		break;

	case SIOCSIFALIFETIME_IN6:                            
		getmicrotime(&caltime);

		                                           
		if (p64) {
			struct in6_addrlifetime_64 lt;

			bcopy(&ifr->ifr_ifru.ifru_lifetime, &lt, sizeof(lt));
			if (lt.ia6t_vltime != ND6_INFINITE_LIFETIME &&
			    lt.ia6t_vltime + caltime.tv_sec < caltime.tv_sec) {
				error = EINVAL;
				break;
			}
			if (lt.ia6t_pltime != ND6_INFINITE_LIFETIME &&
			    lt.ia6t_pltime + caltime.tv_sec < caltime.tv_sec) {
				error = EINVAL;
				break;
			}
		} else {
			struct in6_addrlifetime_32 lt;

			bcopy(&ifr->ifr_ifru.ifru_lifetime, &lt, sizeof(lt));
			if (lt.ia6t_vltime != ND6_INFINITE_LIFETIME &&
			    lt.ia6t_vltime + caltime.tv_sec < caltime.tv_sec) {
				error = EINVAL;
				break;
			}
			if (lt.ia6t_pltime != ND6_INFINITE_LIFETIME &&
			    lt.ia6t_pltime + caltime.tv_sec < caltime.tv_sec) {
				error = EINVAL;
				break;
			}
		}

		IFA_LOCK(&ia->ia_ifa);
		if (p64) {
			struct in6_addrlifetime_64 lt;

			bcopy(&ifr->ifr_ifru.ifru_lifetime, &lt, sizeof(lt));
			ia6_lt.ia6t_expire = lt.ia6t_expire;
			ia6_lt.ia6t_preferred = lt.ia6t_preferred;
			ia6_lt.ia6t_vltime = lt.ia6t_vltime;
			ia6_lt.ia6t_pltime = lt.ia6t_pltime;
		} else {
			struct in6_addrlifetime_32 lt;

			bcopy(&ifr->ifr_ifru.ifru_lifetime, &lt, sizeof(lt));
			ia6_lt.ia6t_expire = (uint32_t)lt.ia6t_expire;
			ia6_lt.ia6t_preferred = (uint32_t)lt.ia6t_preferred;
			ia6_lt.ia6t_vltime = lt.ia6t_vltime;
			ia6_lt.ia6t_pltime = lt.ia6t_pltime;
		}
		                
		if (ia6_lt.ia6t_vltime != ND6_INFINITE_LIFETIME) {
			ia6_lt.ia6t_expire = timenow + ia6_lt.ia6t_vltime;
		} else {
			ia6_lt.ia6t_expire = 0;
		}

		if (ia6_lt.ia6t_pltime != ND6_INFINITE_LIFETIME) {
			ia6_lt.ia6t_preferred = timenow + ia6_lt.ia6t_pltime;
		} else {
			ia6_lt.ia6t_preferred = 0;
		}

		in6ifa_setlifetime(ia, &ia6_lt);
		IFA_UNLOCK(&ia->ia_ifa);
		break;

	default:
		VERIFY(0);
		                
	}

	return error;
}

static int
in6ctl_clat46start(struct ifnet *ifp)
{
	struct nd_prefix *pr = NULL;
	struct nd_prefix *next = NULL;
	struct in6_ifaddr *ia6 = NULL;
	int error = 0;

	if (ifp == lo_ifp) {
		return EINVAL;
	}
	  
                                                                  
                            
                                                                        
    
	lck_mtx_lock(nd6_mutex);
	for (pr = nd_prefix.lh_first; pr; pr = next) {
		next = pr->ndpr_next;

		NDPR_LOCK(pr);
		if (pr->ndpr_ifp != ifp) {
			NDPR_UNLOCK(pr);
			continue;
		}

		if (IN6_IS_ADDR_LINKLOCAL(&pr->ndpr_prefix.sin6_addr)) {
			NDPR_UNLOCK(pr);
			continue;          
		}

		if (pr->ndpr_raf_auto == 0) {
			NDPR_UNLOCK(pr);
			continue;
		}

		if (pr->ndpr_stateflags & NDPRF_DEFUNCT) {
			NDPR_UNLOCK(pr);
			continue;
		}

		if ((pr->ndpr_stateflags & NDPRF_CLAT46) == 0
		    && pr->ndpr_vltime != 0) {
			NDPR_ADDREF(pr);                                                
			NDPR_UNLOCK(pr);
			break;
		} else {
			NDPR_UNLOCK(pr);
			continue;
		}
	}
	lck_mtx_unlock(nd6_mutex);

	if (pr != NULL) {
		if ((ia6 = in6_pfx_newpersistaddr(pr, FALSE, &error,
		    TRUE, CLAT46_COLLISION_COUNT_OFFSET)) == NULL) {
			nd6log0(error,
			    "Could not configure CLAT46 address on"
			    " interface %s.\n", ifp->if_xname);
		} else {
			IFA_LOCK(&ia6->ia_ifa);
			NDPR_LOCK(pr);
			ia6->ia6_ndpr = pr;
			NDPR_ADDREF(pr);                         
			pr->ndpr_stateflags |= NDPRF_CLAT46;
			pr->ndpr_addrcnt++;
			VERIFY(pr->ndpr_addrcnt != 0);
			NDPR_UNLOCK(pr);
			IFA_UNLOCK(&ia6->ia_ifa);
			IFA_REMREF(&ia6->ia_ifa);
			ia6 = NULL;
			  
                                                   
                                                    
                                               
      
			lck_mtx_lock(nd6_mutex);
			pfxlist_onlink_check();
			lck_mtx_unlock(nd6_mutex);
		}
		NDPR_REMREF(pr);
	}
	return error;
}

#define ifa2ia6(ifa)    ((struct in6_ifaddr *)(void *)(ifa))

  
                                              
  
                                                  
  
                                                                      
                                                                  
                                                                      
                                    
  
                                                                         
                                                  
   
int
in6_control(struct socket *so, u_long cmd, caddr_t data, struct ifnet *ifp,
    struct proc *p)
{
	struct in6_ifreq *ifr = (struct in6_ifreq *)(void *)data;
	struct in6_aliasreq sifra, *ifra = NULL;
	struct in6_ifaddr *ia = NULL;
	struct sockaddr_in6 sin6, *sa6 = NULL;
	boolean_t privileged = (proc_suser(p) == 0);
	boolean_t p64 = proc_is64bit(p);
	boolean_t so_unlocked = FALSE;
	int intval, error = 0;

	                                                          
	VERIFY(so != NULL || p == kernproc);

	  
                                                       
    
	switch (cmd) {
	case SIOCAADDRCTL_POLICY:                                  
	case SIOCDADDRCTL_POLICY:                                  
		if (!privileged) {
			return EPERM;
		}
		return in6_src_ioctl(cmd, data);
	                

	case SIOCDRADD_IN6_32:                                       
	case SIOCDRADD_IN6_64:                                       
	case SIOCDRDEL_IN6_32:                                       
	case SIOCDRDEL_IN6_64:                                       
		if (!privileged) {
			return EPERM;
		}
		return defrtrlist_ioctl(cmd, data);
	                

	case SIOCGASSOCIDS32:                                   
	case SIOCGASSOCIDS64:                                   
		return in6ctl_associd(so, cmd, data);
	                

	case SIOCGCONNIDS32:                                    
	case SIOCGCONNIDS64:                                    
		return in6ctl_connid(so, cmd, data);
	                

	case SIOCGCONNINFO32:                                     
	case SIOCGCONNINFO64:                                     
		return in6ctl_conninfo(so, cmd, data);
		                
	}

	  
                                                                
                                                 
    
	if (ifp == NULL) {
		return ENXIO;
	}

	  
                                                           
                                                              
                       
    
	if (so != NULL) {
		socket_unlock(so, 0);
		so_unlocked = TRUE;
	}

	  
                                                       
    
	switch (cmd) {
	case SIOCAUTOCONF_START:                              
		if (!privileged) {
			error = EPERM;
			goto done;
		}
		error = in6_autoconf(ifp, TRUE);
		goto done;

	case SIOCAUTOCONF_STOP:                               
		if (!privileged) {
			error = EPERM;
			goto done;
		}
		error = in6_autoconf(ifp, FALSE);
		goto done;

	case SIOCLL_START_32:                                       
	case SIOCLL_START_64:                                       
		if (!privileged) {
			error = EPERM;
			goto done;
		}
		error = in6ctl_llstart(ifp, cmd, data);
		goto done;

	case SIOCLL_STOP:                                     
		if (!privileged) {
			error = EPERM;
			goto done;
		}
		error = in6ctl_llstop(ifp);
		goto done;

	case SIOCCLAT46_START:                                
		if (!privileged) {
			error = EPERM;
			goto done;
		}
		error = in6ctl_clat46start(ifp);
		if (error == 0) {
			if_set_eflags(ifp, IFEF_CLAT46);
		}
		goto done;

	case SIOCCLAT46_STOP:                                 
		if (!privileged) {
			error = EPERM;
			goto done;
		}

		  
                                                        
                                                              
             
                                                     
     
		if_clear_eflags(ifp, IFEF_CLAT46);
		goto done;
	case SIOCGETROUTERMODE_IN6:                           
		intval = ifp->if_ipv6_router_mode;
		bcopy(&intval, &((struct in6_ifreq *)(void *)data)->ifr_intval,
		    sizeof(intval));
		goto done;
	case SIOCSETROUTERMODE_IN6:                           
		if (!privileged) {
			error = EPERM;
			goto done;
		}
		bcopy(&((struct in6_ifreq *)(void *)data)->ifr_intval,
		    &intval, sizeof(intval));
		switch (intval) {
		case IPV6_ROUTER_MODE_DISABLED:
		case IPV6_ROUTER_MODE_EXCLUSIVE:
		case IPV6_ROUTER_MODE_HYBRID:
			break;
		default:
			error = EINVAL;
			goto done;
		}
		error = in6_setrouter(ifp, (ipv6_router_mode_t)intval);
		goto done;

	case SIOCPROTOATTACH_IN6_32:                                
	case SIOCPROTOATTACH_IN6_64:                                
		if (!privileged) {
			error = EPERM;
			goto done;
		}
		error = in6_domifattach(ifp);
		goto done;

	case SIOCPROTODETACH_IN6:                             
		if (!privileged) {
			error = EPERM;
			goto done;
		}
		                                            
		in6_purgeif(ifp);

		if ((error = proto_unplumb(PF_INET6, ifp))) {
			log(LOG_ERR, "SIOCPROTODETACH_IN6: %s error=%d\n",
			    if_name(ifp), error);
		}
		goto done;

	case SIOCSNDFLUSH_IN6:                                
	case SIOCSPFXFLUSH_IN6:                               
	case SIOCSRTRFLUSH_IN6:                               
	case SIOCSDEFIFACE_IN6_32:                                 
	case SIOCSDEFIFACE_IN6_64:                                 
	case SIOCSIFINFO_FLAGS:                                
	case SIOCGIFCGAPREP_IN6_32:                               
	case SIOCGIFCGAPREP_IN6_64:                               
	case SIOCSIFCGAPREP_IN6_32:                               
	case SIOCSIFCGAPREP_IN6_64:                               
		if (!privileged) {
			error = EPERM;
			goto done;
		}
		OS_FALLTHROUGH;
	case OSIOCGIFINFO_IN6:                                  
	case SIOCGIFINFO_IN6:                                   
	case SIOCGDRLST_IN6_32:                                   
	case SIOCGDRLST_IN6_64:                                   
	case SIOCGPRLST_IN6_32:                                   
	case SIOCGPRLST_IN6_64:                                   
	case SIOCGNBRINFO_IN6_32:                                  
	case SIOCGNBRINFO_IN6_64:                                  
	case SIOCGDEFIFACE_IN6_32:                                 
	case SIOCGDEFIFACE_IN6_64:                                 
		error = nd6_ioctl(cmd, data, ifp);
		goto done;

	case SIOCSIFPREFIX_IN6:                                                
	case SIOCDIFPREFIX_IN6:                                                
	case SIOCAIFPREFIX_IN6:                                                
	case SIOCCIFPREFIX_IN6:                                                
	case SIOCSGIFPREFIX_IN6:                                               
	case SIOCGIFPREFIX_IN6:                                                
		log(LOG_NOTICE,
		    "prefix ioctls are now invalidated. "
		    "please use ifconfig.\n");
		error = EOPNOTSUPP;
		goto done;

	case SIOCSSCOPE6:                                                  
	case SIOCGSCOPE6:                                                  
	case SIOCGSCOPE6DEF:                                               
		error = EOPNOTSUPP;
		goto done;

	case SIOCLL_CGASTART_32:                                  
	case SIOCLL_CGASTART_64:                                  
		if (!privileged) {
			error = EPERM;
		} else {
			error = in6ctl_cgastart(ifp, cmd, data);
		}
		goto done;

	case SIOCGIFSTAT_IN6:                                 
	case SIOCGIFSTAT_ICMP6:                               
		error = in6ctl_gifstat(ifp, cmd, ifr);
		goto done;
	}

	  
                                                                
    
	switch (cmd) {
	case SIOCSIFADDR_IN6:                                              
	case SIOCSIFDSTADDR_IN6:                                           
	case SIOCSIFNETMASK_IN6:                                           
		  
                                                          
                                                             
     
		                                                    
		error = EOPNOTSUPP;
		goto done;

	case SIOCAIFADDR_IN6_32:                                    
	case SIOCAIFADDR_IN6_64:                                    
		if (!privileged) {
			error = EPERM;
			goto done;
		}
		  
                                                            
                                                             
                                                                
                                                 
     
		ifra = in6_aliasreq_to_native(data,
		    (cmd == SIOCAIFADDR_IN6_64), &sifra);
		bcopy(&ifra->ifra_addr, &sin6, sizeof(sin6));
		sa6 = &sin6;
		break;

	case SIOCDIFADDR_IN6:                                 
	case SIOCSIFALIFETIME_IN6:                            
		if (!privileged) {
			error = EPERM;
			goto done;
		}
		OS_FALLTHROUGH;
	case SIOCGIFADDR_IN6:                                 
	case SIOCGIFDSTADDR_IN6:                              
	case SIOCGIFNETMASK_IN6:                              
	case SIOCGIFAFLAG_IN6:                                
	case SIOCGIFALIFETIME_IN6:                            
		bcopy(&ifr->ifr_addr, &sin6, sizeof(sin6));
		sa6 = &sin6;
		break;
	case SIOCGIFDSTADDR:
	case SIOCSIFDSTADDR:
	case SIOCGIFBRDADDR:
	case SIOCSIFBRDADDR:
	case SIOCGIFNETMASK:
	case SIOCSIFNETMASK:
	case SIOCGIFADDR:
	case SIOCSIFADDR:
	case SIOCAIFADDR:
	case SIOCDIFADDR:
		                                                           
		error = EINVAL;
		goto done;
	}

	  
                                                  
   
                                                                        
                                                                     
                                                                      
                                                               
                                                         
                                                              
                                                        
                                                                 
    
	VERIFY(ia == NULL);
	if (sa6 != NULL && sa6->sin6_family == AF_INET6) {
		if (IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr)) {
			if (sa6->sin6_addr.s6_addr16[1] == 0) {
				                                         
				sa6->sin6_addr.s6_addr16[1] =
				    htons(ifp->if_index);
			} else if (sa6->sin6_addr.s6_addr16[1] !=
			    htons(ifp->if_index)) {
				error = EINVAL;                          
				goto done;
			}
			if (sa6->sin6_scope_id) {
				if (sa6->sin6_scope_id !=
				    (u_int32_t)ifp->if_index) {
					error = EINVAL;
					goto done;
				}
				sa6->sin6_scope_id = 0;                     
			}
		}
		  
                                                           
                                                             
                                                         
                                                
     
		ia = in6ifa_ifpwithaddr(ifp, &sa6->sin6_addr);
	}

	  
                                                   
    
	switch (cmd) {
	case SIOCDIFADDR_IN6:                                 
		if (ia == NULL) {
			error = EADDRNOTAVAIL;
			goto done;
		}
		OS_FALLTHROUGH;
	case SIOCAIFADDR_IN6_32:                                    
	case SIOCAIFADDR_IN6_64:                                    
		VERIFY(sa6 != NULL);
		  
                                                                
                                                              
                                            
     
		if (sa6->sin6_family != AF_INET6 ||
		    sa6->sin6_len != sizeof(struct sockaddr_in6)) {
			error = EAFNOSUPPORT;
			goto done;
		}
		break;
	}

	  
                                               
    
	switch (cmd) {
	case SIOCGIFADDR_IN6:                                 
	                                                              
	              
	case SIOCGIFDSTADDR_IN6:                              
		error = in6ctl_gifaddr(ifp, ia, cmd, ifr);
		break;

	case SIOCGIFNETMASK_IN6:                              
		if (ia != NULL) {
			IFA_LOCK(&ia->ia_ifa);
			bcopy(&ia->ia_prefixmask, &ifr->ifr_addr,
			    sizeof(struct sockaddr_in6));
			IFA_UNLOCK(&ia->ia_ifa);
		} else {
			error = EADDRNOTAVAIL;
		}
		break;

	case SIOCGIFAFLAG_IN6:                                
		if (ia != NULL) {
			IFA_LOCK(&ia->ia_ifa);
			bcopy(&ia->ia6_flags, &ifr->ifr_ifru.ifru_flags6,
			    sizeof(ifr->ifr_ifru.ifru_flags6));
			IFA_UNLOCK(&ia->ia_ifa);
		} else {
			error = EADDRNOTAVAIL;
		}
		break;

	case SIOCGIFALIFETIME_IN6:                            
	case SIOCSIFALIFETIME_IN6:                            
		error = in6ctl_alifetime(ia, cmd, ifr, p64);
		break;

	case SIOCAIFADDR_IN6_32:                                    
	case SIOCAIFADDR_IN6_64:                                    
		error = in6ctl_aifaddr(ifp, ifra);
		break;

	case SIOCDIFADDR_IN6:
		in6ctl_difaddr(ifp, ia);
		break;

	default:
		error = ifnet_ioctl(ifp, PF_INET6, cmd, data);
		break;
	}

done:
	if (ia != NULL) {
		IFA_REMREF(&ia->ia_ifa);
	}
	if (so_unlocked) {
		socket_lock(so, 0);
	}

	return error;
}

static __attribute__((noinline)) int
in6ctl_aifaddr(struct ifnet *ifp, struct in6_aliasreq *ifra)
{
	int i, error, addtmp;
	uint8_t plen;
	struct nd_prefix pr0, *pr;
	struct in6_ifaddr *ia;

	VERIFY(ifp != NULL && ifra != NULL);
	ia = NULL;

	                                                               
	error = in6_domifattach(ifp);
	if (error == 0) {
		                                         
		error = in6_ifattach_aliasreq(ifp, NULL, NULL);
		if (error != 0) {
			goto done;
		}

		in6_if_up_dad_start(ifp);
	} else if (error != EEXIST) {
		goto done;
	}

	  
                                                                      
                
    
	error = in6_update_ifa(ifp, ifra, 0, &ia);
	if (error != 0) {
		goto done;
	}
	VERIFY(ia != NULL);

	                                                    
	plen = (uint8_t)in6_mask2len(&ifra->ifra_prefixmask.sin6_addr, NULL);
	if (plen == 128) {
		goto done;
	}

	  
                                                                       
                                                                      
                                      
    

	  
                                                                        
                        
    
	bzero(&pr0, sizeof(pr0));
	pr0.ndpr_plen = plen;
	pr0.ndpr_ifp = ifp;
	pr0.ndpr_prefix = ifra->ifra_addr;
	pr0.ndpr_mask = ifra->ifra_prefixmask.sin6_addr;

	                                
	for (i = 0; i < 4; i++) {
		pr0.ndpr_prefix.sin6_addr.s6_addr32[i] &=
		    ifra->ifra_prefixmask.sin6_addr.s6_addr32[i];
	}

	  
                                                                        
                                                               
                                                                       
                                                          
    
	pr0.ndpr_raf_onlink = 1;                              
	pr0.ndpr_raf_auto = !!(ifra->ifra_flags & IN6_IFF_AUTOCONF);
	if (ifra->ifra_flags & (IN6_IFF_AUTOCONF | IN6_IFF_DYNAMIC)) {
		pr0.ndpr_vltime = ifra->ifra_lifetime.ia6t_vltime;
		pr0.ndpr_pltime = ifra->ifra_lifetime.ia6t_pltime;
	} else {
		pr0.ndpr_vltime = ND6_INFINITE_LIFETIME;
		pr0.ndpr_pltime = ND6_INFINITE_LIFETIME;
	}
	pr0.ndpr_stateflags |= NDPRF_STATIC;
	lck_mtx_init(&pr0.ndpr_lock, ifa_mtx_grp, ifa_mtx_attr);

	                                     
	if ((pr = nd6_prefix_lookup(&pr0, ND6_PREFIX_EXPIRY_NEVER)) == NULL) {
		  
                                                             
           
     
		error = nd6_prelist_add(&pr0, NULL, &pr, FALSE);
		if (error != 0) {
			goto done;
		}

		if (pr == NULL) {
			log(LOG_ERR, "%s: nd6_prelist_add okay, but"
			    " no prefix.\n", __func__);
			error = EINVAL;
			goto done;
		}
	}

	IFA_LOCK(&ia->ia_ifa);

	                                      
	addtmp = FALSE;
	if (ia->ia6_ndpr == NULL) {
		NDPR_LOCK(pr);
		++pr->ndpr_addrcnt;
		VERIFY(pr->ndpr_addrcnt != 0);
		ia->ia6_ndpr = pr;
		NDPR_ADDREF(pr);                         

		  
                                                           
                                                         
     
		if ((ia->ia6_flags & IN6_IFF_AUTOCONF) != 0 &&
		    ip6_use_tempaddr &&
		    pr->ndpr_addrcnt == 1 &&
		    (!IN6_IS_ADDR_UNIQUE_LOCAL(&ia->ia_addr.sin6_addr)
		    || ip6_ula_use_tempaddr)) {
			addtmp = true;
		}
		NDPR_UNLOCK(pr);
	}

	IFA_UNLOCK(&ia->ia_ifa);

	if (addtmp) {
		int e;
		e = in6_tmpifadd(ia, 1);
		if (e != 0) {
			log(LOG_NOTICE, "%s: failed to create a"
			    " temporary address, error=%d\n",
			    __func__, e);
		}
	}

	  
                                                                      
                                                     
    
	lck_mtx_lock(nd6_mutex);
	pfxlist_onlink_check();
	lck_mtx_unlock(nd6_mutex);

	                                                 
	NDPR_REMREF(pr);

done:
	if (ia != NULL) {
		IFA_REMREF(&ia->ia_ifa);
	}
	return error;
}

static __attribute__((noinline)) void
in6ctl_difaddr(struct ifnet *ifp, struct in6_ifaddr *ia)
{
	int i = 0;
	struct nd_prefix pr0, *pr;

	VERIFY(ifp != NULL && ia != NULL);

	  
                                                          
                                                        
                                                         
                                                          
                                                               
                                                          
                        
                                                          
    
	IFA_LOCK(&ia->ia_ifa);
	bzero(&pr0, sizeof(pr0));
	pr0.ndpr_ifp = ifp;
	pr0.ndpr_plen = (uint8_t)in6_mask2len(&ia->ia_prefixmask.sin6_addr, NULL);
	if (pr0.ndpr_plen == 128) {
		IFA_UNLOCK(&ia->ia_ifa);
		goto purgeaddr;
	}
	pr0.ndpr_prefix = ia->ia_addr;
	pr0.ndpr_mask = ia->ia_prefixmask.sin6_addr;
	for (i = 0; i < 4; i++) {
		pr0.ndpr_prefix.sin6_addr.s6_addr32[i] &=
		    ia->ia_prefixmask.sin6_addr.s6_addr32[i];
	}
	IFA_UNLOCK(&ia->ia_ifa);

	if ((pr = nd6_prefix_lookup(&pr0, ND6_PREFIX_EXPIRY_UNSPEC)) != NULL) {
		IFA_LOCK(&ia->ia_ifa);
		NDPR_LOCK(pr);
		if (pr->ndpr_addrcnt == 1) {
			                              
			pr->ndpr_expire = 1;
		}
		NDPR_UNLOCK(pr);
		IFA_UNLOCK(&ia->ia_ifa);

		                                             
		NDPR_REMREF(pr);
	}

purgeaddr:
	in6_purgeaddr(&ia->ia_ifa);
}

static __attribute__((noinline)) int
in6_autoconf(struct ifnet *ifp, int enable)
{
	int error = 0;

	VERIFY(ifp != NULL);

	if (ifp->if_flags & IFF_LOOPBACK) {
		return EINVAL;
	}

	if (enable) {
		  
                                                     
                                                     
                                                        
                                                    
     
		ifnet_lock_exclusive(ifp);
		if (ifp->if_ipv6_router_mode == IPV6_ROUTER_MODE_EXCLUSIVE) {
			if_clear_eflags(ifp, IFEF_ACCEPT_RTADV);
			error = EBUSY;
		} else {
			if_set_eflags(ifp, IFEF_ACCEPT_RTADV);
		}
		ifnet_lock_done(ifp);
	} else {
		struct in6_ifaddr *ia = NULL;

		if_clear_eflags(ifp, IFEF_ACCEPT_RTADV);

		                                                  
		lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
		boolean_t from_begining = TRUE;
		while (from_begining) {
			from_begining = FALSE;
			TAILQ_FOREACH(ia, &in6_ifaddrhead, ia6_link) {
				if (ia->ia_ifa.ifa_ifp != ifp) {
					continue;
				}
				IFA_LOCK(&ia->ia_ifa);
				if (ia->ia6_flags & IN6_IFF_AUTOCONF) {
					IFA_ADDREF_LOCKED(&ia->ia_ifa);             
					IFA_UNLOCK(&ia->ia_ifa);
					lck_rw_done(&in6_ifaddr_rwlock);
					in6_purgeaddr(&ia->ia_ifa);
					IFA_REMREF(&ia->ia_ifa);                    
					lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
					  
                                                    
                                     
                                                 
                            
        
					from_begining = TRUE;
					break;
				}
				IFA_UNLOCK(&ia->ia_ifa);
			}
		}
		lck_rw_done(&in6_ifaddr_rwlock);
	}
	return error;
}

  
                                                                            
                                                                               
                                                  
   
static __attribute__((noinline)) int
in6_setrouter(struct ifnet *ifp, ipv6_router_mode_t mode)
{
	int                     error = 0;
	ipv6_router_mode_t      prev_mode;

	VERIFY(ifp != NULL);

	if (ifp->if_flags & IFF_LOOPBACK) {
		return ENODEV;
	}

	prev_mode = ifp->if_ipv6_router_mode;
	if (prev_mode == mode) {
		                                           
		return 0;
	}
	if (mode == IPV6_ROUTER_MODE_EXCLUSIVE) {
		struct nd_ifinfo *ndi = NULL;

		ndi = ND_IFINFO(ifp);
		if (ndi != NULL && ndi->initialized) {
			lck_mtx_lock(&ndi->lock);
			if (ndi->flags & ND6_IFF_PROXY_PREFIXES) {
				                                              
				ndi->flags &= ~ND6_IFF_PROXY_PREFIXES;
				lck_mtx_unlock(&ndi->lock);
				(void) nd6_if_prproxy(ifp, FALSE);
			} else {
				lck_mtx_unlock(&ndi->lock);
			}
		}
	}

	ifp->if_ipv6_router_mode = mode;
	lck_mtx_lock(nd6_mutex);
	defrouter_select(ifp, NULL);
	lck_mtx_unlock(nd6_mutex);
	if_allmulti(ifp, (mode == IPV6_ROUTER_MODE_EXCLUSIVE));
	if (mode == IPV6_ROUTER_MODE_EXCLUSIVE ||
	    (prev_mode == IPV6_ROUTER_MODE_EXCLUSIVE
	    && mode == IPV6_ROUTER_MODE_DISABLED)) {
		error = in6_autoconf(ifp, FALSE);
	}
	return error;
}

static int
in6_to_kamescope(struct sockaddr_in6 *sin6, struct ifnet *ifp)
{
	struct sockaddr_in6 tmp;
	int error, id;

	VERIFY(sin6 != NULL);
	tmp = *sin6;

	error = in6_recoverscope(&tmp, &sin6->sin6_addr, ifp);
	if (error != 0) {
		return error;
	}

	id = in6_addr2scopeid(ifp, &tmp.sin6_addr);
	if (tmp.sin6_scope_id == 0) {
		tmp.sin6_scope_id = id;
	} else if (tmp.sin6_scope_id != id) {
		return EINVAL;                         
	}
	error = in6_embedscope(&tmp.sin6_addr, &tmp, NULL, NULL, NULL);
	if (error != 0) {
		return error;
	}

	tmp.sin6_scope_id = 0;
	*sin6 = tmp;
	return 0;
}

  
                                                                         
                             
   
#define IN6_IFF_CLR_ADDR_FLAG_MASK      (~(IN6_IFF_DEPRECATED | IN6_IFF_DETACHED | IN6_IFF_DUPLICATED))

static int
in6_ifaupdate_aux(struct in6_ifaddr *ia, struct ifnet *ifp, int ifaupflags)
{
	struct sockaddr_in6 mltaddr, mltmask;
	struct in6_addr llsol;
	struct ifaddr *ifa;
	struct in6_multi *in6m_sol;
	struct in6_multi_mship *imm;
	struct rtentry *rt;
	int delay, error = 0;

	VERIFY(ifp != NULL && ia != NULL);
	ifa = &ia->ia_ifa;
	in6m_sol = NULL;

	nd6log2(debug, "%s - %s ifp %s ia6_flags 0x%x ifaupflags 0x%x\n",
	    __func__,
	    ip6_sprintf(&ia->ia_addr.sin6_addr),
	    if_name(ia->ia_ifp),
	    ia->ia6_flags,
	    ifaupflags);

	  
                                                            
                       
    
	ia->ia6_flags &= IN6_IFF_CLR_ADDR_FLAG_MASK;

	  
                                                                     
                                                                  
                   
    
	if (in6if_do_dad(ifp)) {
		in6_ifaddr_set_dadprogress(ia);
		  
                                                                      
                                                
     
		if (ia->ia6_flags & IN6_IFF_OPTIMISTIC) {
			ifaupflags &= ~IN6_IFAUPDATE_DADDELAY;
		} else {
			ifaupflags |= IN6_IFAUPDATE_DADDELAY;
		}
	} else {
		  
                                                    
                                                  
                                      
     
		ia->ia6_flags &= ~IN6_IFF_DADPROGRESS;
	}

	                                     
	if ((ifp->if_flags & IFF_MULTICAST) != 0) {
		                                                   
		bzero(&llsol, sizeof(struct in6_addr));
		llsol.s6_addr32[0] = IPV6_ADDR_INT32_MLL;
		llsol.s6_addr32[1] = 0;
		llsol.s6_addr32[2] = htonl(1);
		llsol.s6_addr32[3] = ia->ia_addr.sin6_addr.s6_addr32[3];
		llsol.s6_addr8[12] = 0xff;
		if ((error = in6_setscope(&llsol, ifp, NULL)) != 0) {
			                            
			log(LOG_ERR, "%s: in6_setscope failed\n", __func__);
			goto unwind;
		}
		delay = 0;
		if ((ifaupflags & IN6_IFAUPDATE_DADDELAY)) {
			  
                                                   
                                               
                                                     
                                        
      
			delay = random() % MAX_RTR_SOLICITATION_DELAY;
		}
		imm = in6_joingroup(ifp, &llsol, &error, delay);
		if (imm == NULL) {
			nd6log(info,
			    "%s: addmulti failed for %s on %s (errno=%d)\n",
			    __func__, ip6_sprintf(&llsol), if_name(ifp),
			    error);
			VERIFY(error != 0);
			goto unwind;
		}
		in6m_sol = imm->i6mm_maddr;
		                                      
		IN6M_ADDREF(in6m_sol);

		IFA_LOCK_SPIN(ifa);
		LIST_INSERT_HEAD(&ia->ia6_memberships, imm, i6mm_chain);
		IFA_UNLOCK(ifa);

		bzero(&mltmask, sizeof(mltmask));
		mltmask.sin6_len = sizeof(struct sockaddr_in6);
		mltmask.sin6_family = AF_INET6;
		mltmask.sin6_addr = in6mask32;
#define MLTMASK_LEN  4                                         

		  
                                      
     
		bzero(&mltaddr, sizeof(mltaddr));
		mltaddr.sin6_len = sizeof(struct sockaddr_in6);
		mltaddr.sin6_family = AF_INET6;
		mltaddr.sin6_addr = in6addr_linklocal_allnodes;
		if ((error = in6_setscope(&mltaddr.sin6_addr, ifp, NULL)) != 0) {
			goto unwind;                           
		}
		  
                                                  
                                                                 
                                                                
                            
     
		rt = rtalloc1_scoped((struct sockaddr *)&mltaddr, 0, 0UL,
		    ia->ia_ifp->if_index);
		if (rt) {
			if (memcmp(&mltaddr.sin6_addr, &((struct sockaddr_in6 *)
			    (void *)rt_key(rt))->sin6_addr, MLTMASK_LEN)) {
				rtfree(rt);
				rt = NULL;
			}
		}
		if (!rt) {
			error = rtrequest_scoped(RTM_ADD,
			    (struct sockaddr *)&mltaddr,
			    (struct sockaddr *)&ia->ia_addr,
			    (struct sockaddr *)&mltmask, RTF_UP | RTF_CLONING,
			    NULL, ia->ia_ifp->if_index);
			if (error) {
				goto unwind;
			}
		} else {
			rtfree(rt);
		}

		imm = in6_joingroup(ifp, &mltaddr.sin6_addr, &error, 0);
		if (!imm) {
			nd6log(info,
			    "%s: addmulti failed for %s on %s (errno=%d)\n",
			    __func__, ip6_sprintf(&mltaddr.sin6_addr),
			    if_name(ifp), error);
			VERIFY(error != 0);
			goto unwind;
		}
		IFA_LOCK_SPIN(ifa);
		LIST_INSERT_HEAD(&ia->ia6_memberships, imm, i6mm_chain);
		IFA_UNLOCK(ifa);

		  
                                        
     
#define hostnamelen     strlen(hostname)
		delay = 0;
		if ((ifaupflags & IN6_IFAUPDATE_DADDELAY)) {
			  
                                                        
                                             
      
			delay = random() % MAX_RTR_SOLICITATION_DELAY;
		}
		lck_mtx_lock(&hostname_lock);
		int n = in6_nigroup(ifp, hostname, hostnamelen, &mltaddr.sin6_addr);
		lck_mtx_unlock(&hostname_lock);
		if (n == 0) {
			imm = in6_joingroup(ifp, &mltaddr.sin6_addr, &error,
			    delay);                 
			if (!imm) {
				nd6log(info,
				    "%s: addmulti failed for %s on %s "
				    "(errno=%d)\n",
				    __func__, ip6_sprintf(&mltaddr.sin6_addr),
				    if_name(ifp), error);
				                                  
				error = 0;
			} else {
				IFA_LOCK_SPIN(ifa);
				LIST_INSERT_HEAD(&ia->ia6_memberships,
				    imm, i6mm_chain);
				IFA_UNLOCK(ifa);
			}
		}
#undef hostnamelen

		  
                                            
                                     
     
		mltaddr.sin6_addr = in6addr_nodelocal_allnodes;
		if ((error = in6_setscope(&mltaddr.sin6_addr, ifp, NULL)) != 0) {
			goto unwind;                           
		}
		                                              
		rt = rtalloc1_scoped((struct sockaddr *)&mltaddr, 0, 0UL,
		    ia->ia_ifp->if_index);
		if (rt) {
			if (memcmp(&mltaddr.sin6_addr, &((struct sockaddr_in6 *)
			    (void *)rt_key(rt))->sin6_addr, MLTMASK_LEN)) {
				rtfree(rt);
				rt = NULL;
			}
		}
		if (!rt) {
			error = rtrequest_scoped(RTM_ADD,
			    (struct sockaddr *)&mltaddr,
			    (struct sockaddr *)&ia->ia_addr,
			    (struct sockaddr *)&mltmask, RTF_UP | RTF_CLONING,
			    NULL, ia->ia_ifp->if_index);
			if (error) {
				goto unwind;
			}
		} else {
			rtfree(rt);
		}

		imm = in6_joingroup(ifp, &mltaddr.sin6_addr, &error, 0);
		if (!imm) {
			nd6log(info,
			    "%s: addmulti failed for %s on %s (errno=%d)\n",
			    __func__, ip6_sprintf(&mltaddr.sin6_addr),
			    if_name(ifp), error);
			VERIFY(error != 0);
			goto unwind;
		}
		IFA_LOCK(ifa);
		LIST_INSERT_HEAD(&ia->ia6_memberships, imm, i6mm_chain);
		IFA_UNLOCK(ifa);
#undef  MLTMASK_LEN

		  
                            
     
		bzero(&mltmask, sizeof(mltmask));
		mltmask.sin6_len = sizeof(struct sockaddr_in6);
		mltmask.sin6_family = AF_INET6;
		mltmask.sin6_addr = in6mask8;
#define MLTMASK_LEN_8_BITS  1                                                 

		bzero(&mltaddr, sizeof(mltaddr));
		mltaddr.sin6_len = sizeof(struct sockaddr_in6);
		mltaddr.sin6_family = AF_INET6;
		mltaddr.sin6_addr = in6addr_multicast_prefix;

		rt = rtalloc1_scoped((struct sockaddr *)&mltaddr, 0, 0UL,
		    ia->ia_ifp->if_index);
		if (rt) {
			if (memcmp(&mltaddr.sin6_addr, &((struct sockaddr_in6 *)
			    (void *)rt_key(rt))->sin6_addr, MLTMASK_LEN_8_BITS)) {
				rtfree(rt);
				rt = NULL;
			}
		}
		if (!rt) {
			error = rtrequest_scoped(RTM_ADD,
			    (struct sockaddr *)&mltaddr,
			    (struct sockaddr *)&ia->ia_addr,
			    (struct sockaddr *)&mltmask, RTF_UP | RTF_CLONING,
			    NULL, ia->ia_ifp->if_index);
			if (error) {
				goto unwind;
			}
		} else {
			rtfree(rt);
		}
	}
#undef  MLTMASK_LEN_8_BITS

	                                                                  
	++nd6_sched_timeout_want;

	  
                    
                                             
                                            
                                                             
    
	IFA_LOCK_SPIN(ifa);
	if (in6if_do_dad(ifp) && (ia->ia6_flags & IN6_IFF_NODAD) == 0 &&
	    (ia->ia6_flags & IN6_IFF_DADPROGRESS) != 0) {
		int mindelay, maxdelay;
		int *delayptr, delayval;

		IFA_UNLOCK(ifa);
		delayptr = NULL;
		  
                                                           
                                                             
                                                
     
		if ((ifaupflags & IN6_IFAUPDATE_DADDELAY) &&
		    !(ia->ia6_flags & IN6_IFF_SWIFTDAD)) {
			  
                                                    
                                                       
                                                      
                                                        
                                                      
            
      
			mindelay = 0;
			if (in6m_sol != NULL) {
				IN6M_LOCK(in6m_sol);
				if (in6m_sol->in6m_state ==
				    MLD_REPORTING_MEMBER) {
					mindelay = in6m_sol->in6m_timer;
				}
				IN6M_UNLOCK(in6m_sol);
			}
			maxdelay = MAX_RTR_SOLICITATION_DELAY * hz;
			if (maxdelay - mindelay == 0) {
				delayval = 0;
			} else {
				delayval =
				    (random() % (maxdelay - mindelay)) +
				    mindelay;
			}
			delayptr = &delayval;
		}

		nd6_dad_start((struct ifaddr *)ia, delayptr);
	} else {
		IFA_UNLOCK(ifa);
	}

	goto done;

unwind:
	VERIFY(error != 0);
	in6_purgeaddr(&ia->ia_ifa);

done:
	                                             
	if (in6m_sol != NULL) {
		IN6M_REMREF(in6m_sol);
	}
	return error;
}

  
                                                                             
                                                                           
                                                                            
   
int
in6_update_ifa(struct ifnet *ifp, struct in6_aliasreq *ifra, int ifaupflags,
    struct in6_ifaddr **iar)
{
	struct in6_addrlifetime ia6_lt;
	struct in6_ifaddr *ia;
	struct ifaddr *ifa;
	struct ifaddr *xifa;
	struct in6_addrlifetime *lt;
	uint64_t timenow;
	int plen, error;

	                                                   
	VERIFY(ifp != NULL && ifra != NULL && iar != NULL);
	ia = NULL;
	ifa = NULL;
	error = 0;

	  
                                                               
                                
    
	if (ifra->ifra_addr.sin6_family != AF_INET6 ||
	    ifra->ifra_addr.sin6_len != sizeof(struct sockaddr_in6)) {
		error = EAFNOSUPPORT;
		goto unwind;
	}

	                                                            
	if (ifra->ifra_prefixmask.sin6_len == 0 ||
	    ifra->ifra_prefixmask.sin6_len > sizeof(struct sockaddr_in6)) {
		error = EINVAL;
		goto unwind;
	}

	                                                                      
	plen = (uint8_t)in6_mask2len(&ifra->ifra_prefixmask.sin6_addr,
	    (u_char *)&ifra->ifra_prefixmask + ifra->ifra_prefixmask.sin6_len);
	if (plen <= 0) {
		error = EINVAL;
		goto unwind;
	}

	                        
	lt = &ifra->ifra_lifetime;
	if (lt->ia6t_pltime > lt->ia6t_vltime) {
		log(LOG_INFO,
		    "%s: pltime 0x%x > vltime 0x%x for %s\n", __func__,
		    lt->ia6t_pltime, lt->ia6t_vltime,
		    ip6_sprintf(&ifra->ifra_addr.sin6_addr));
		error = EINVAL;
		goto unwind;
	}
	if (lt->ia6t_vltime == 0) {
		  
                                                            
                                           
     
		log(LOG_INFO, "%s: valid lifetime is 0 for %s\n", __func__,
		    ip6_sprintf(&ifra->ifra_addr.sin6_addr));
	}

	  
                                                                    
                                                                    
                 
    
	struct sockaddr_in6 lookup_address = ifra->ifra_addr;
	if (IN6_IS_ADDR_LINKLOCAL(&lookup_address.sin6_addr)) {
		if (lookup_address.sin6_addr.s6_addr16[1] == 0) {
			                                         
			lookup_address.sin6_addr.s6_addr16[1] =
			    htons(ifp->if_index);
		} else if (lookup_address.sin6_addr.s6_addr16[1] !=
		    htons(ifp->if_index)) {
			error = EINVAL;                          
			goto done;
		}
		if (lookup_address.sin6_scope_id != 0 &&
		    lookup_address.sin6_scope_id !=
		    (u_int32_t)ifp->if_index) {
			error = EINVAL;
			goto done;
		}
	}

	ia = in6ifa_ifpwithaddr(ifp, &lookup_address.sin6_addr);
	if (ia != NULL) {
		ifa = &ia->ia_ifa;
	}

	  
                                                                    
    
	if ((ifp->if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT)) != 0) {
		switch (ifra->ifra_dstaddr.sin6_family) {
		case AF_INET6:
			if (plen != 128) {
				                                           
				log(LOG_INFO,
				    "%s: prefix length < 128 with"
				    " explicit dstaddr.\n", __func__);
				error = EINVAL;
				goto unwind;
			}
			break;

		case AF_UNSPEC:
			break;

		default:
			error = EAFNOSUPPORT;
			goto unwind;
		}
	} else if (ifra->ifra_dstaddr.sin6_family != AF_UNSPEC) {
		log(LOG_INFO,
		    "%s: dstaddr valid only on p2p and loopback interfaces.\n",
		    __func__);
		error = EINVAL;
		goto unwind;
	}

	timenow = net_uptime();

	if (ia == NULL) {
		zalloc_flags_t how;

		                                                           
		ifaupflags |= IN6_IFAUPDATE_NEWADDR;

		                                                           
		how = (ifaupflags & IN6_IFAUPDATE_NOWAIT) ? Z_NOWAIT : Z_WAITOK;
		ia = in6_ifaddr_alloc(how);
		if (ia == NULL) {
			error = ENOBUFS;
			goto unwind;
		}

		ifa = &ia->ia_ifa;

		  
                                            
    
                                                               
                                                                  
                                                                 
                                                                  
                                                               
                                                               
                                                              
    
                                         
     
		IFA_ADDREF(ifa);                                         
		IA6_HASH_INIT(ia);
		ifa->ifa_addr = (struct sockaddr *)&ia->ia_addr;
		if (ifra->ifra_dstaddr.sin6_family == AF_INET6 ||
		    (ifp->if_flags & (IFF_POINTOPOINT | IFF_LOOPBACK)) != 0) {
			ifa->ifa_dstaddr = (struct sockaddr *)&ia->ia_dstaddr;
		}
		ifa->ifa_netmask = (struct sockaddr *)&ia->ia_prefixmask;
		ifa->ifa_ifp = ifp;
		ifa->ifa_metric = ifp->if_metric;
		ifa->ifa_rtrequest = nd6_rtrequest;

		LIST_INIT(&ia->ia6_memberships);
		ia->ia_addr.sin6_family = AF_INET6;
		ia->ia_addr.sin6_len = sizeof(ia->ia_addr);
		ia->ia_addr.sin6_addr = ifra->ifra_addr.sin6_addr;
		ia->ia_prefixmask.sin6_family = AF_INET6;
		ia->ia_prefixmask.sin6_len = sizeof(ia->ia_prefixmask);
		ia->ia_prefixmask.sin6_addr = ifra->ifra_prefixmask.sin6_addr;
		error = in6_to_kamescope(&ia->ia_addr, ifp);
		if (error != 0) {
			goto unwind;
		}
		if (ifa->ifa_dstaddr != NULL) {
			ia->ia_dstaddr = ifra->ifra_dstaddr;
			error = in6_to_kamescope(&ia->ia_dstaddr, ifp);
			if (error != 0) {
				goto unwind;
			}
		}

		                              
		ifnet_lock_exclusive(ifp);
		ifaupflags |= IN6_IFAUPDATE_1STADDR;
		TAILQ_FOREACH(xifa, &ifp->if_addrlist, ifa_list) {
			IFA_LOCK_SPIN(xifa);
			if (xifa->ifa_addr->sa_family != AF_INET6) {
				IFA_UNLOCK(xifa);
				ifaupflags &= ~IN6_IFAUPDATE_1STADDR;
				break;
			}
			IFA_UNLOCK(xifa);
		}

		IFA_LOCK_SPIN(ifa);
		if_attach_ifa(ifp, ifa);                                     
		IFA_UNLOCK(ifa);
		ifnet_lock_done(ifp);

		lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
		TAILQ_INSERT_TAIL(&in6_ifaddrhead, ia, ia6_link);
		IFA_ADDREF(ifa);                                
		lck_rw_done(&in6_ifaddr_rwlock);
	} else {
		ifa = &ia->ia_ifa;
		ifaupflags &= ~(IN6_IFAUPDATE_NEWADDR | IN6_IFAUPDATE_1STADDR);
	}

	VERIFY(ia != NULL && ifa == &ia->ia_ifa);
	IFA_LOCK(ifa);

	  
                                                                     
                                                                      
                                   
    
	ia->ia6_updatetime = ia->ia6_createtime = timenow;
	ia6_lt = *lt;
	if (ia6_lt.ia6t_vltime != ND6_INFINITE_LIFETIME) {
		ia6_lt.ia6t_expire = timenow + ia6_lt.ia6t_vltime;
	} else {
		ia6_lt.ia6t_expire = 0;
	}
	if (ia6_lt.ia6t_pltime != ND6_INFINITE_LIFETIME) {
		ia6_lt.ia6t_preferred = timenow + ia6_lt.ia6t_pltime;
	} else {
		ia6_lt.ia6t_preferred = 0;
	}
	in6ifa_setlifetime(ia, &ia6_lt);

	  
                                                                  
                                 
    
	if ((ia->ia6_flags & IN6_IFF_DEPRECATED) != 0) {
		ia->ia6_lifetime.ia6ti_pltime = 0;
		ia->ia6_lifetime.ia6ti_preferred = timenow;
	}

	  
                                
    
	ia->ia_plen = plen;
	ia->ia6_flags = ifra->ifra_flags;

	                                                               
	IFA_UNLOCK(ifa);

	                                                     
	error = in6_ifinit(ifp, ia, ifaupflags);
	if (error != 0) {
		goto unwind;
	}

	                                                                       
	error = in6_ifaupdate_aux(ia, ifp, ifaupflags);
	if (error != 0) {
		goto unwind;
	}

	                                                        
	VERIFY(error == 0);
	(void) ifnet_notify_address(ifp, AF_INET6);
	goto done;

unwind:
	VERIFY(error != 0);
	if (ia != NULL) {
		VERIFY(ifa == &ia->ia_ifa);
		IFA_REMREF(ifa);
		ia = NULL;
	}

done:
	*iar = ia;
	return error;
}

void
in6_purgeaddr(struct ifaddr *ifa)
{
	struct ifnet *ifp = ifa->ifa_ifp;
	struct in6_ifaddr *ia = (struct in6_ifaddr *)ifa;
	struct in6_multi_mship *imm;

	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_NOTOWNED);

	                         
	nd6_dad_stop(ifa);

	  
                                                                
                                                       
    
	IFA_LOCK(ifa);
	if ((ia->ia_flags & IFA_ROUTE) && ia->ia_plen == 128) {
		int error, rtf;

		IFA_UNLOCK(ifa);
		rtf = (ia->ia_dstaddr.sin6_family == AF_INET6) ? RTF_HOST : 0;
		error = rtinit(&(ia->ia_ifa), RTM_DELETE, rtf);
		if (error != 0) {
			log(LOG_ERR, "in6_purgeaddr: failed to remove "
			    "a route to the p2p destination: %s on %s, "
			    "errno=%d\n",
			    ip6_sprintf(&ia->ia_addr.sin6_addr), if_name(ifp),
			    error);
			                       
		}
		IFA_LOCK_SPIN(ifa);
		ia->ia_flags &= ~IFA_ROUTE;
	}
	IFA_UNLOCK(ifa);

	                                                      
	in6_ifremloop(&(ia->ia_ifa));

	  
                                                                
    
	IFA_LOCK(ifa);
	while ((imm = ia->ia6_memberships.lh_first) != NULL) {
		LIST_REMOVE(imm, i6mm_chain);
		IFA_UNLOCK(ifa);
		in6_leavegroup(imm);
		IFA_LOCK(ifa);
	}
	IFA_UNLOCK(ifa);

	                                                 
	in6_unlink_ifa(ia, ifp);
	in6_post_msg(ifp, KEV_INET6_ADDR_DELETED, ia, NULL);

	(void) ifnet_notify_address(ifp, AF_INET6);
}

static void
in6_unlink_ifa(struct in6_ifaddr *ia, struct ifnet *ifp)
{
	struct in6_ifaddr *nia;
	struct ifaddr *ifa;
	int unlinked;

	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_NOTOWNED);

	ifa = &ia->ia_ifa;
	IFA_ADDREF(ifa);

	ifnet_lock_exclusive(ifp);
	IFA_LOCK(ifa);
	if (ifa->ifa_debug & IFD_ATTACHED) {
		if_detach_ifa(ifp, ifa);
	}
	IFA_UNLOCK(ifa);
	ifnet_lock_done(ifp);

	unlinked = 0;
	lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
	TAILQ_FOREACH(nia, &in6_ifaddrhead, ia6_link) {
		if (ia == nia) {
			TAILQ_REMOVE(&in6_ifaddrhead, ia, ia6_link);
			IFA_LOCK(ifa);
			if (IA6_IS_HASHED(ia)) {
				in6_iahash_remove(ia);
			}
			IFA_UNLOCK(ifa);
			unlinked = 1;
			break;
		}
	}

	  
                                                   
                                 
                                                    
                                       
                           
    
	IFA_LOCK(ifa);
	  
                                                       
               
                                                           
                    
                                                      
                                               
                                                    
                           
   
                                                 
    
	if (!IN6_IS_ADDR_LINKLOCAL(&ia->ia_addr.sin6_addr)) {
		if (ia->ia6_ndpr == NULL) {
			log(LOG_NOTICE, "in6_unlink_ifa: IPv6 address "
			    "0x%llx has no prefix\n",
			    (uint64_t)VM_KERNEL_ADDRPERM(ia));
		} else {
			struct nd_prefix *pr = ia->ia6_ndpr;
			ia->ia6_flags &= ~IN6_IFF_AUTOCONF;
			ia->ia6_ndpr = NULL;
			NDPR_LOCK(pr);
			VERIFY(pr->ndpr_addrcnt != 0);
			pr->ndpr_addrcnt--;
			if (ia->ia6_flags & IN6_IFF_CLAT46) {
				pr->ndpr_stateflags &= ~NDPRF_CLAT46;
			}
			NDPR_UNLOCK(pr);
			NDPR_REMREF(pr);                                    
		}
	}
	IFA_UNLOCK(ifa);
	lck_rw_done(&in6_ifaddr_rwlock);

	if ((ia->ia6_flags & IN6_IFF_AUTOCONF) != 0) {
		lck_mtx_lock(nd6_mutex);
		pfxlist_onlink_check();
		lck_mtx_unlock(nd6_mutex);
	}
	  
                                                         
                                                                       
                                                                    
    
	if (unlinked) {
		IFA_REMREF(ifa);
	}

	                                             
	IFA_REMREF(ifa);

	                             
	routegenid_inet6_update();
}

void
in6_purgeif(struct ifnet *ifp)
{
	struct in6_ifaddr *ia;

	if (ifp == NULL) {
		return;
	}

	LCK_MTX_ASSERT(nd6_mutex, LCK_MTX_ASSERT_NOTOWNED);

	lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
	boolean_t from_begining = TRUE;
	while (from_begining) {
		from_begining = FALSE;
		TAILQ_FOREACH(ia, &in6_ifaddrhead, ia6_link) {
			if (ia->ia_ifa.ifa_ifp != ifp) {
				continue;
			}
			IFA_ADDREF(&ia->ia_ifa);                    
			lck_rw_done(&in6_ifaddr_rwlock);
			in6_purgeaddr(&ia->ia_ifa);
			IFA_REMREF(&ia->ia_ifa);                    
			lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
			  
                                           
                                                     
                                               
                          
      
			from_begining = TRUE;
			break;
		}
	}
	lck_rw_done(&in6_ifaddr_rwlock);

	in6_ifdetach(ifp);
}

  
                                                                       
   
static int
in6_ifinit(struct ifnet *ifp, struct in6_ifaddr *ia, int ifaupflags)
{
	int error;
	struct ifaddr *ifa;

	error = 0;
	ifa = &ia->ia_ifa;

	lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
	IFA_LOCK(&ia->ia_ifa);
	if (IA6_IS_HASHED(ia)) {
		in6_iahash_remove(ia);
	}
	if ((ifp->if_flags & IFF_POINTOPOINT)) {
		in6_iahash_insert_ptp(ia);
	} else {
		in6_iahash_insert(ia);
	}
	IFA_UNLOCK(&ia->ia_ifa);
	lck_rw_done(&in6_ifaddr_rwlock);

	  
                                                                
                                                                   
                                         
    
	if ((ifaupflags & IN6_IFAUPDATE_1STADDR) != 0) {
		error = ifnet_ioctl(ifp, PF_INET6, SIOCSIFADDR, ia);
		if (error != 0) {
			if (error != EOPNOTSUPP) {
				goto failed;
			}
			error = 0;
		}
	}

	IFA_LOCK(ifa);

	  
                 
                                                                
                                                                 
                 
    
	if (!(ia->ia_flags & IFA_ROUTE) && ia->ia_plen == 128 &&
	    ia->ia_dstaddr.sin6_family == AF_INET6) {
		IFA_UNLOCK(ifa);
		error = rtinit(ifa, RTM_ADD, RTF_UP | RTF_HOST);
		if (error != 0) {
			goto failed;
		}
		IFA_LOCK(ifa);
		ia->ia_flags |= IFA_ROUTE;
	}
	IFA_LOCK_ASSERT_HELD(ifa);
	if (ia->ia_plen < 128) {
		  
                                                                
     
		ia->ia_flags |= RTF_CLONING;
	}

	IFA_UNLOCK(ifa);

	                                                                      
	if ((ifaupflags & IN6_IFAUPDATE_NEWADDR) != 0) {
		in6_ifaddloop(ifa);
	}

	                             
	routegenid_inet6_update();

	VERIFY(error == 0);
	return 0;
failed:
	VERIFY(error != 0);
	lck_rw_lock_exclusive(&in6_ifaddr_rwlock);
	IFA_LOCK(&ia->ia_ifa);
	if (IA6_IS_HASHED(ia)) {
		in6_iahash_remove(ia);
	}
	IFA_UNLOCK(&ia->ia_ifa);
	lck_rw_done(&in6_ifaddr_rwlock);

	return error;
}

void
in6_purgeaddrs(struct ifnet *ifp)
{
	in6_purgeif(ifp);
}

  
                                                                      
   
struct in6_ifaddr *
in6ifa_ifpforlinklocal(struct ifnet *ifp, int ignoreflags)
{
	struct ifaddr *ifa;

	ifnet_lock_shared(ifp);
	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list)
	{
		IFA_LOCK_SPIN(ifa);
		if (ifa->ifa_addr->sa_family != AF_INET6) {
			IFA_UNLOCK(ifa);
			continue;
		}
		if (IN6_IS_ADDR_LINKLOCAL(IFA_IN6(ifa))) {
			if ((((struct in6_ifaddr *)ifa)->ia6_flags &
			    ignoreflags) != 0) {
				IFA_UNLOCK(ifa);
				continue;
			}
			IFA_ADDREF_LOCKED(ifa);                 
			IFA_UNLOCK(ifa);
			break;
		}
		IFA_UNLOCK(ifa);
	}
	ifnet_lock_done(ifp);

	return (struct in6_ifaddr *)ifa;
}

struct in6_ifaddr *
in6ifa_ifpwithflag(struct ifnet * ifp, int flag)
{
	struct ifaddr *ifa;

	ifnet_lock_shared(ifp);
	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list)
	{
		IFA_LOCK_SPIN(ifa);
		if (ifa->ifa_addr->sa_family != AF_INET6) {
			IFA_UNLOCK(ifa);
			continue;
		}
		if ((((struct in6_ifaddr *)ifa)->ia6_flags & flag) == flag) {
			IFA_ADDREF_LOCKED(ifa);
			IFA_UNLOCK(ifa);
			break;
		}
		IFA_UNLOCK(ifa);
	}
	ifnet_lock_done(ifp);

	return (struct in6_ifaddr *)ifa;
}

  
                                                                            
   
struct in6_ifaddr *
in6ifa_ifpwithaddr(struct ifnet *ifp, struct in6_addr *addr)
{
	struct ifaddr *ifa;

	ifnet_lock_shared(ifp);
	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list)
	{
		IFA_LOCK_SPIN(ifa);
		if (ifa->ifa_addr->sa_family != AF_INET6) {
			IFA_UNLOCK(ifa);
			continue;
		}
		if (IN6_ARE_ADDR_EQUAL(addr, IFA_IN6(ifa))) {
			IFA_ADDREF_LOCKED(ifa);                 
			IFA_UNLOCK(ifa);
			break;
		}
		IFA_UNLOCK(ifa);
	}
	ifnet_lock_done(ifp);

	return (struct in6_ifaddr *)ifa;
}

struct in6_ifaddr *
in6ifa_prproxyaddr(struct in6_addr *addr)
{
	struct in6_ifaddr *ia;

	lck_rw_lock_shared(&in6_ifaddr_rwlock);
	TAILQ_FOREACH(ia, IN6ADDR_HASH(addr), ia6_hash) {
		IFA_LOCK(&ia->ia_ifa);
		if (IN6_ARE_ADDR_EQUAL(addr, IFA_IN6(&ia->ia_ifa))) {
			IFA_ADDREF_LOCKED(&ia->ia_ifa);                 
			IFA_UNLOCK(&ia->ia_ifa);
			break;
		}
		IFA_UNLOCK(&ia->ia_ifa);
	}
	lck_rw_done(&in6_ifaddr_rwlock);

	if (ia != NULL && !nd6_prproxy_ifaddr(ia)) {
		IFA_REMREF(&ia->ia_ifa);
		ia = NULL;
	}

	return ia;
}

void
in6ifa_getlifetime(struct in6_ifaddr *ia6, struct in6_addrlifetime *t_dst,
    int iscalendar)
{
	struct in6_addrlifetime_i *t_src = &ia6->ia6_lifetime;
	struct timeval caltime;

	t_dst->ia6t_vltime = t_src->ia6ti_vltime;
	t_dst->ia6t_pltime = t_src->ia6ti_pltime;
	t_dst->ia6t_expire = 0;
	t_dst->ia6t_preferred = 0;

	                                    
	getmicrotime(&caltime);
	t_src->ia6ti_base_calendartime +=
	    NET_CALCULATE_CLOCKSKEW(caltime,
	    t_src->ia6ti_base_calendartime, net_uptime(),
	    t_src->ia6ti_base_uptime);

	if (iscalendar) {
		if (t_src->ia6ti_expire != 0 &&
		    t_src->ia6ti_vltime != ND6_INFINITE_LIFETIME) {
			t_dst->ia6t_expire = t_src->ia6ti_base_calendartime +
			    t_src->ia6ti_expire - t_src->ia6ti_base_uptime;
		}

		if (t_src->ia6ti_preferred != 0 &&
		    t_src->ia6ti_pltime != ND6_INFINITE_LIFETIME) {
			t_dst->ia6t_preferred = t_src->ia6ti_base_calendartime +
			    t_src->ia6ti_preferred - t_src->ia6ti_base_uptime;
		}
	} else {
		if (t_src->ia6ti_expire != 0 &&
		    t_src->ia6ti_vltime != ND6_INFINITE_LIFETIME) {
			t_dst->ia6t_expire = t_src->ia6ti_expire;
		}

		if (t_src->ia6ti_preferred != 0 &&
		    t_src->ia6ti_pltime != ND6_INFINITE_LIFETIME) {
			t_dst->ia6t_preferred = t_src->ia6ti_preferred;
		}
	}
}

void
in6ifa_setlifetime(struct in6_ifaddr *ia6, struct in6_addrlifetime *t_src)
{
	struct in6_addrlifetime_i *t_dst = &ia6->ia6_lifetime;
	struct timeval caltime;

	                                    
	getmicrotime(&caltime);
	t_dst->ia6ti_base_calendartime +=
	    NET_CALCULATE_CLOCKSKEW(caltime,
	    t_dst->ia6ti_base_calendartime, net_uptime(),
	    t_dst->ia6ti_base_uptime);

	                                     
	t_dst->ia6ti_expire = t_src->ia6t_expire;
	t_dst->ia6ti_preferred = t_src->ia6t_preferred;
	t_dst->ia6ti_vltime = t_src->ia6t_vltime;
	t_dst->ia6ti_pltime = t_src->ia6t_pltime;
}

  
                                                              
   
char *
ip6_sprintf(const struct in6_addr *addr)
{
	static const char digits[] = "0123456789abcdef";
	static int ip6round = 0;
	static char ip6buf[8][48];

	int i;
	char *cp;
	const u_short *a = (const u_short *)addr;
	const u_char *d;
	u_char n;
	int dcolon = 0;
	int zpad = 0;

	ip6round = (ip6round + 1) & 7;
	cp = ip6buf[ip6round];

	for (i = 0; i < 8; i++) {
		if (dcolon == 1) {
			if (*a == 0) {
				if (i == 7) {
					*cp++ = ':';
				}
				a++;
				continue;
			} else {
				dcolon = 2;
			}
		}
		if (*a == 0) {
			if (dcolon == 0 && *(a + 1) == 0) {
				if (i == 0) {
					*cp++ = ':';
				}
				*cp++ = ':';
				dcolon = 1;
			} else {
				*cp++ = '0';
				*cp++ = ':';
			}
			a++;
			continue;
		}
		d = (const u_char *)a;
		zpad = 0;
		if ((n = *d >> 4) != 0) {
			*cp++ = digits[n];
			zpad = 1;
		}
		if ((n = *d++ & 0xf) != 0 || zpad) {
			*cp++ = digits[n];
			zpad = 1;
		}
		if ((n = *d >> 4) != 0 || zpad) {
			*cp++ = digits[n];
			zpad = 1;
		}
		if ((n = *d & 0xf) != 0 || zpad) {
			*cp++ = digits[n];
		}
		*cp++ = ':';
		a++;
	}
	*--cp = 0;
	return ip6buf[ip6round];
}

int
in6addr_local(struct in6_addr *in6)
{
	struct rtentry *rt;
	struct sockaddr_in6 sin6;
	int local = 0;

	if (IN6_IS_ADDR_LOOPBACK(in6) || IN6_IS_SCOPE_LINKLOCAL(in6)) {
		return 1;
	}

	sin6.sin6_family = AF_INET6;
	sin6.sin6_len = sizeof(sin6);
	bcopy(in6, &sin6.sin6_addr, sizeof(*in6));
	rt = rtalloc1((struct sockaddr *)&sin6, 0, 0);

	if (rt != NULL) {
		RT_LOCK_SPIN(rt);
		if (rt->rt_gateway->sa_family == AF_LINK) {
			local = 1;
		}
		RT_UNLOCK(rt);
		rtfree(rt);
	} else {
		local = in6_localaddr(in6);
	}
	return local;
}

int
in6_localaddr(struct in6_addr *in6)
{
	struct in6_ifaddr *ia;

	if (IN6_IS_ADDR_LOOPBACK(in6) || IN6_IS_ADDR_LINKLOCAL(in6) || IN6_IS_ADDR_MC_UNICAST_BASED_LINKLOCAL(in6)) {
		return 1;
	}

	lck_rw_lock_shared(&in6_ifaddr_rwlock);
	TAILQ_FOREACH(ia, &in6_ifaddrhead, ia6_link) {
		IFA_LOCK_SPIN(&ia->ia_ifa);
		if (IN6_ARE_MASKED_ADDR_EQUAL(in6, &ia->ia_addr.sin6_addr,
		    &ia->ia_prefixmask.sin6_addr)) {
			IFA_UNLOCK(&ia->ia_ifa);
			lck_rw_done(&in6_ifaddr_rwlock);
			return 1;
		}
		IFA_UNLOCK(&ia->ia_ifa);
	}
	lck_rw_done(&in6_ifaddr_rwlock);
	return 0;
}

  
                                                    
                 
   
int
in6_matchlen(struct in6_addr *src, struct in6_addr *dst)
{
	int match = 0;
	u_char *s = (u_char *)src, *d = (u_char *)dst;
	u_char *lim = s + 16, r;

	while (s < lim) {
		if ((r = (*d++ ^ *s++)) != 0) {
			while (r < 128) {
				match++;
				r <<= 1;
			}
			break;
		} else {
			match += 8;
		}
	}
	return match;
}

                                
int
in6_are_prefix_equal(struct in6_addr *p1, struct in6_addr *p2, int len)
{
	int bytelen, bitlen;

	                  
	if (0 > len || len > 128) {
		log(LOG_ERR, "%s: invalid prefix length(%d)\n", __func__, len);
		return 0;
	}

	bytelen = len / 8;
	bitlen = len % 8;

	if (bcmp(&p1->s6_addr, &p2->s6_addr, bytelen)) {
		return 0;
	}
	if (bitlen != 0 &&
	    p1->s6_addr[bytelen] >> (8 - bitlen) !=
	    p2->s6_addr[bytelen] >> (8 - bitlen)) {
		return 0;
	}

	return 1;
}

void
in6_prefixlen2mask(struct in6_addr *maskp, int len)
{
	u_char maskarray[8] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
	int bytelen, bitlen, i;

	                  
	if (0 > len || len > 128) {
		log(LOG_ERR, "%s: invalid prefix length(%d)\n", __func__, len);
		return;
	}

	bzero(maskp, sizeof(*maskp));
	bytelen = len / 8;
	bitlen = len % 8;
	for (i = 0; i < bytelen; i++) {
		maskp->s6_addr[i] = 0xff;
	}
	if (bitlen) {
		maskp->s6_addr[bytelen] = maskarray[bitlen - 1];
	}
}

  
                                                
   
struct in6_ifaddr *
in6_ifawithscope(struct ifnet *oifp, struct in6_addr *dst)
{
	int dst_scope = in6_addrscope(dst), src_scope, best_scope = 0;
	int blen = -1;
	struct ifaddr *ifa;
	struct ifnet *ifp;
	struct in6_ifaddr *ifa_best = NULL;

	if (oifp == NULL) {
		return NULL;
	}

	  
                                                                     
                                                                   
                                           
    
	ifnet_head_lock_shared();
	TAILQ_FOREACH(ifp, &ifnet_head, if_list) {
		  
                                                            
                        
     
		if (in6_addr2scopeid(ifp, dst) != in6_addr2scopeid(oifp, dst)) {
			continue;
		}

		ifnet_lock_shared(ifp);
		TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
			int tlen = -1, dscopecmp, bscopecmp, matchcmp;

			IFA_LOCK(ifa);
			if (ifa->ifa_addr->sa_family != AF_INET6) {
				IFA_UNLOCK(ifa);
				continue;
			}
			src_scope = in6_addrscope(IFA_IN6(ifa));

			  
                                                
                               
      
			if (((struct in6_ifaddr *)ifa)->ia6_flags &
			    (IN6_IFF_NOTREADY | IN6_IFF_CLAT46)) {
				IFA_UNLOCK(ifa);
				continue;
			}
			                                               
			if (((struct in6_ifaddr *)ifa)->ia6_flags &
			    IN6_IFF_ANYCAST) {
				IFA_UNLOCK(ifa);
				continue;
			}
			if (((struct in6_ifaddr *)ifa)->ia6_flags &
			    IN6_IFF_DETACHED) {
				IFA_UNLOCK(ifa);
				continue;
			}
			  
                                           
                     
      
			if (ifa_best == NULL) {
				goto replace;
			}

			  
                                                    
                                         
      

			  
                                                  
                                               
                                                 
                                                      
                                                    
                    
                                                  
                                       
                                 
                                
         
                                   
                                
      
			if (IN6_ARE_SCOPE_CMP(best_scope, dst_scope) < 0 &&
			    IN6_ARE_SCOPE_CMP(src_scope, dst_scope) >= 0) {
				goto replace;          
			}
			if (IN6_ARE_SCOPE_CMP(src_scope, dst_scope) < 0 &&
			    IN6_ARE_SCOPE_CMP(best_scope, dst_scope) >= 0) {
				IFA_UNLOCK(ifa);
				continue;          
			}
			  
                                                    
                                                     
                                                    
                              
      
			if (((struct in6_ifaddr *)ifa)->ia6_flags &
			    IN6_IFF_DEPRECATED) {
				  
                                         
                                  
       
				if (!ip6_use_deprecated) {
					IFA_UNLOCK(ifa);
					continue;
				}
				  
                                                
                                                   
       
				if ((ifa_best->ia6_flags & IN6_IFF_DEPRECATED)
				    == 0) {
					IFA_UNLOCK(ifa);
					continue;
				}
			}

			  
                                                  
                                                  
                                                      
                                    
      
			if ((ifa_best->ia6_flags & IN6_IFF_DEPRECATED) &&
			    (((struct in6_ifaddr *)ifa)->ia6_flags &
			    IN6_IFF_DEPRECATED) == 0) {
				goto replace;
			}

			  
                                                  
                                                
                                                 
                                                     
                                                        
                                                       
                                         
      
			if (ip6_use_tempaddr) {
				struct in6_ifaddr *ifat;

				ifat = (struct in6_ifaddr *)ifa;
				if ((ifa_best->ia6_flags &
				    (IN6_IFF_AUTOCONF | IN6_IFF_TEMPORARY))
				    == IN6_IFF_AUTOCONF &&
				    (ifat->ia6_flags &
				    (IN6_IFF_AUTOCONF | IN6_IFF_TEMPORARY))
				    == (IN6_IFF_AUTOCONF | IN6_IFF_TEMPORARY)) {
					goto replace;
				}
				if ((ifa_best->ia6_flags &
				    (IN6_IFF_AUTOCONF | IN6_IFF_TEMPORARY))
				    == (IN6_IFF_AUTOCONF | IN6_IFF_TEMPORARY) &&
				    (ifat->ia6_flags &
				    (IN6_IFF_AUTOCONF | IN6_IFF_TEMPORARY))
				    == IN6_IFF_AUTOCONF) {
					IFA_UNLOCK(ifa);
					continue;
				}
			}

			  
                                       
                                                    
                                             
                                                
                                         
                                                   
                                                          
                                                         
                                                     
                                                    
            
                                                    
                       
                                                    
                            
                                                     
                  
                                                      
     
                                               
                                                        
                                                
                                           
                                                        
                                                   
                                                   
                                          
                                                      
                                                 
                                                       
                                                
                                                       
                                     
                                                   
                                                        
     
                                               
                                                   
                                                   
                                                   
                                                   
                                                   
                                                   
                                                   
                                                   
                                                   
                                                   
                                                    
      
			dscopecmp = IN6_ARE_SCOPE_CMP(src_scope, dst_scope);
			bscopecmp = IN6_ARE_SCOPE_CMP(src_scope, best_scope);

			if (bscopecmp == 0) {
				struct ifnet *bifp = ifa_best->ia_ifp;

				if (bifp == oifp && ifp != oifp) {          
					IFA_UNLOCK(ifa);
					continue;
				}
				if (bifp != oifp && ifp == oifp) {          
					goto replace;
				}

				  
                                            
                                                
                                       
                                               
                       
       
				tlen = in6_matchlen(IFA_IN6(ifa), dst);
				matchcmp = tlen - blen;
				if (matchcmp > 0) {          
					goto replace;
				}
				IFA_UNLOCK(ifa);
				continue;          
			}
			if (dscopecmp > 0) {
				if (bscopecmp > 0) {          
					IFA_UNLOCK(ifa);
					continue;
				}
				goto replace;          
			}
			if (dscopecmp < 0) {
				if (bscopecmp > 0) {          
					goto replace;
				}
				IFA_UNLOCK(ifa);
				continue;          
			}

			                             
			if (bscopecmp < 0) {
				goto replace;          
			}
replace:
			IFA_ADDREF_LOCKED(ifa);                   
			blen = tlen >= 0 ? tlen :
			    in6_matchlen(IFA_IN6(ifa), dst);
			best_scope =
			    in6_addrscope(&ifa2ia6(ifa)->ia_addr.sin6_addr);
			IFA_UNLOCK(ifa);
			if (ifa_best) {
				IFA_REMREF(&ifa_best->ia_ifa);
			}
			ifa_best = (struct in6_ifaddr *)ifa;
		}
		ifnet_lock_done(ifp);
	}
	ifnet_head_done();

	                                              
	if (ifa_best == NULL) {
		ip6stat.ip6s_sources_none++;
	} else {
		IFA_LOCK_SPIN(&ifa_best->ia_ifa);
		if (oifp == ifa_best->ia_ifp) {
			ip6stat.ip6s_sources_sameif[best_scope]++;
		} else {
			ip6stat.ip6s_sources_otherif[best_scope]++;
		}

		if (best_scope == dst_scope) {
			ip6stat.ip6s_sources_samescope[best_scope]++;
		} else {
			ip6stat.ip6s_sources_otherscope[best_scope]++;
		}

		if ((ifa_best->ia6_flags & IN6_IFF_DEPRECATED) != 0) {
			ip6stat.ip6s_sources_deprecated[best_scope]++;
		}
		IFA_UNLOCK(&ifa_best->ia_ifa);
	}

	return ifa_best;
}

  
                                                                   
                                                            
   
struct in6_ifaddr *
in6_ifawithifp(struct ifnet *ifp, struct in6_addr *dst)
{
	int dst_scope = in6_addrscope(dst), blen = -1, tlen;
	struct ifaddr *ifa;
	struct in6_ifaddr *besta = NULL;
	struct in6_ifaddr *dep[2];                                   

	dep[0] = dep[1] = NULL;

	  
                                                  
                               
                                                             
                                                               
    
	ifnet_lock_shared(ifp);
	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
		IFA_LOCK(ifa);
		if (ifa->ifa_addr->sa_family != AF_INET6) {
			IFA_UNLOCK(ifa);
			continue;
		}
		if (ifa2ia6(ifa)->ia6_flags & IN6_IFF_ANYCAST) {
			IFA_UNLOCK(ifa);
			continue;                                               
		}
		if (ifa2ia6(ifa)->ia6_flags & (IN6_IFF_NOTREADY | IN6_IFF_CLAT46)) {
			IFA_UNLOCK(ifa);
			continue;                               
		}
		if (ifa2ia6(ifa)->ia6_flags & IN6_IFF_DETACHED) {
			IFA_UNLOCK(ifa);
			continue;
		}
		if (ifa2ia6(ifa)->ia6_flags & IN6_IFF_DEPRECATED) {
			if (ip6_use_deprecated) {
				IFA_ADDREF_LOCKED(ifa);                 
				IFA_UNLOCK(ifa);
				if (dep[0] != NULL) {
					IFA_REMREF(&dep[0]->ia_ifa);
				}
				dep[0] = (struct in6_ifaddr *)ifa;
			} else {
				IFA_UNLOCK(ifa);
			}
			continue;
		}

		if (dst_scope == in6_addrscope(IFA_IN6(ifa))) {
			  
                                            
      
			if (besta) {
				if (blen == -1) {
					IFA_UNLOCK(ifa);
					IFA_LOCK(&besta->ia_ifa);
					blen = in6_matchlen(
						&besta->ia_addr.sin6_addr, dst);
					IFA_UNLOCK(&besta->ia_ifa);
					IFA_LOCK(ifa);
				}
				tlen = in6_matchlen(IFA_IN6(ifa), dst);
				if (tlen > blen) {
					blen = tlen;
					IFA_ADDREF_LOCKED(ifa);                
					IFA_UNLOCK(ifa);
					IFA_REMREF(&besta->ia_ifa);
					besta = (struct in6_ifaddr *)ifa;
				} else {
					IFA_UNLOCK(ifa);
				}
			} else {
				besta = (struct in6_ifaddr *)ifa;
				IFA_ADDREF_LOCKED(ifa);                
				IFA_UNLOCK(ifa);
			}
		} else {
			IFA_UNLOCK(ifa);
		}
	}
	if (besta) {
		ifnet_lock_done(ifp);
		if (dep[0] != NULL) {
			IFA_REMREF(&dep[0]->ia_ifa);
		}
		return besta;
	}

	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
		IFA_LOCK(ifa);
		if (ifa->ifa_addr->sa_family != AF_INET6) {
			IFA_UNLOCK(ifa);
			continue;
		}
		if (ifa2ia6(ifa)->ia6_flags & IN6_IFF_ANYCAST) {
			IFA_UNLOCK(ifa);
			continue;                                               
		}
		if (ifa2ia6(ifa)->ia6_flags & (IN6_IFF_NOTREADY | IN6_IFF_CLAT46)) {
			IFA_UNLOCK(ifa);
			continue;                               
		}
		if (ifa2ia6(ifa)->ia6_flags & IN6_IFF_DETACHED) {
			IFA_UNLOCK(ifa);
			continue;
		}
		if (ifa2ia6(ifa)->ia6_flags & IN6_IFF_DEPRECATED) {
			if (ip6_use_deprecated) {
				IFA_ADDREF_LOCKED(ifa);                 
				IFA_UNLOCK(ifa);
				if (dep[1] != NULL) {
					IFA_REMREF(&dep[1]->ia_ifa);
				}
				dep[1] = (struct in6_ifaddr *)ifa;
			} else {
				IFA_UNLOCK(ifa);
			}
			continue;
		}
		IFA_ADDREF_LOCKED(ifa);                 
		IFA_UNLOCK(ifa);
		ifnet_lock_done(ifp);
		if (dep[0] != NULL) {
			IFA_REMREF(&dep[0]->ia_ifa);
		}
		if (dep[1] != NULL) {
			IFA_REMREF(&dep[1]->ia_ifa);
		}
		return (struct in6_ifaddr *)ifa;
	}
	ifnet_lock_done(ifp);

	                                                                
	if (dep[0]) {
		if (dep[1] != NULL) {
			IFA_REMREF(&dep[1]->ia_ifa);
		}
		return dep[0];
	}
	if (dep[1]) {
		return dep[1];
	}

	return NULL;
}

  
                                             
   
static void
in6_if_up_dad_start(struct ifnet *ifp)
{
	struct ifaddr *ifa;
	struct nd_ifinfo *ndi = NULL;

	ndi = ND_IFINFO(ifp);
	VERIFY((NULL != ndi) && (TRUE == ndi->initialized));
	if (!(ndi->flags & ND6_IFF_DAD)) {
		return;
	}

	                                              
	ifnet_lock_exclusive(ifp);
	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
		struct in6_ifaddr *ia6;

		IFA_LOCK_SPIN(ifa);
		if (ifa->ifa_addr->sa_family != AF_INET6) {
			IFA_UNLOCK(ifa);
			continue;
		}
		ia6 = (struct in6_ifaddr *)ifa;
		if (ia6->ia6_flags & IN6_IFF_DADPROGRESS) {
			int delay = 0;                                     
			IFA_UNLOCK(ifa);
			nd6_dad_start(ifa, &delay);
		} else {
			IFA_UNLOCK(ifa);
		}
	}
	ifnet_lock_done(ifp);
}

int
in6if_do_dad(
	struct ifnet *ifp)
{
	struct nd_ifinfo *ndi = NULL;

	if ((ifp->if_flags & IFF_LOOPBACK) != 0) {
		return 0;
	}

	ndi = ND_IFINFO(ifp);
	VERIFY((NULL != ndi) && (TRUE == ndi->initialized));
	if (!(ndi->flags & ND6_IFF_DAD)) {
		return 0;
	}

	  
                                                      
                                               
   
                                                       
                                                          
                                               
    
	if (ifp->if_eflags &
	    (IFEF_IPV6_ND6ALT | IFEF_LOCALNET_PRIVATE | IFEF_DIRECTLINK)) {
		return 0;
	}

	if (ifp->if_family == IFNET_FAMILY_IPSEC ||
	    ifp->if_family == IFNET_FAMILY_UTUN) {
		  
                                                           
                                            
     
		return 0;
	}

	switch (ifp->if_type) {
#if IFT_DUMMY
	case IFT_DUMMY:
#endif
	case IFT_FAITH:
		  
                                                        
                                                             
                                                             
                                        
     
		return 0;
	default:
		  
                                                           
                                                          
                                                            
                                                  
                                                        
                                                              
                                                  
     
		if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) !=
		    (IFF_UP | IFF_RUNNING)) {
			return 0;
		}

		return 1;
	}
}

  
                                                                 
                 
   
void
in6_setmaxmtu(void)
{
	u_int32_t maxmtu = 0;
	struct ifnet *ifp;

	ifnet_head_lock_shared();
	TAILQ_FOREACH(ifp, &ifnet_head, if_list) {
		struct nd_ifinfo *ndi = NULL;

		if ((ndi = ND_IFINFO(ifp)) != NULL && !ndi->initialized) {
			ndi = NULL;
		}
		if (ndi != NULL) {
			lck_mtx_lock(&ndi->lock);
		}
		if ((ifp->if_flags & IFF_LOOPBACK) == 0 &&
		    IN6_LINKMTU(ifp) > maxmtu) {
			maxmtu = IN6_LINKMTU(ifp);
		}
		if (ndi != NULL) {
			lck_mtx_unlock(&ndi->lock);
		}
	}
	ifnet_head_done();
	if (maxmtu) {                                            
		in6_maxmtu = maxmtu;
	}
}
  
                                                                               
                                                                      
                                                                        
                                                                         
                                                           
   
int
in6_if2idlen(struct ifnet *ifp)
{
	switch (ifp->if_type) {
	case IFT_ETHER:                      
	case IFT_IEEE8023ADLAG:                                 
#ifdef IFT_PROPVIRTUAL
	case IFT_PROPVIRTUAL:                                       
#endif
#ifdef IFT_L2VLAN
	case IFT_L2VLAN:                   
#endif
#ifdef IFT_IEEE80211
	case IFT_IEEE80211:                
#endif
#ifdef IFT_MIP
	case IFT_MIP:              
#endif
		return 64;
	case IFT_FDDI:                       
		return 64;
	case IFT_ISO88025:                                          
		return 64;
	case IFT_PPP:                        
		return 64;
	case IFT_ARCNET:                     
		return 64;
	case IFT_FRELAY:                     
		return 64;
	case IFT_IEEE1394:                   
		return 64;
	case IFT_GIF:
		return 64;                                     
	case IFT_LOOP:
		return 64;                                      
	case IFT_OTHER:
		return 64;                             
	case IFT_CELLULAR:
		return 64;                                   
	case IFT_BRIDGE:
		return 64;                                      
	case IFT_6LOWPAN:
		return 64;                 
	default:
		  
                       
                                                                 
                                                                 
                                                                
                                                              
                                                             
                                                                
                                                            
                                   
     
		log(LOG_NOTICE, "%s: unknown link type (%d)\n", __func__,
		    ifp->if_type);
		return 64;
	}
}
  
                                                                      
                                   
   
void
in6_sin6_2_sin(struct sockaddr_in *sin, struct sockaddr_in6 *sin6)
{
	bzero(sin, sizeof(*sin));
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_family = AF_INET;
	sin->sin_port = sin6->sin6_port;
	sin->sin_addr.s_addr = sin6->sin6_addr.s6_addr32[3];
}

                                                                   
void
in6_sin_2_v4mapsin6(struct sockaddr_in *sin, struct sockaddr_in6 *sin6)
{
	bzero(sin6, sizeof(*sin6));
	sin6->sin6_len = sizeof(struct sockaddr_in6);
	sin6->sin6_family = AF_INET6;
	sin6->sin6_port = sin->sin_port;
	sin6->sin6_addr.s6_addr32[0] = 0;
	sin6->sin6_addr.s6_addr32[1] = 0;
	if (sin->sin_addr.s_addr) {
		sin6->sin6_addr.s6_addr32[2] = IPV6_ADDR_INT32_SMP;
		sin6->sin6_addr.s6_addr32[3] = sin->sin_addr.s_addr;
	} else {
		sin6->sin6_addr.s6_addr32[2] = 0;
		sin6->sin6_addr.s6_addr32[3] = 0;
	}
}

                                            
void
in6_sin6_2_sin_in_sock(struct sockaddr *nam)
{
	struct sockaddr_in *sin_p;
	struct sockaddr_in6 sin6;

	  
                                                  
                   
    
	sin6 = *(struct sockaddr_in6 *)(void *)nam;
	sin_p = (struct sockaddr_in *)(void *)nam;
	in6_sin6_2_sin(sin_p, &sin6);
}

                                                                     
int
in6_sin_2_v4mapsin6_in_sock(struct sockaddr **nam)
{
	struct sockaddr_in *sin_p;
	struct sockaddr_in6 *sin6_p;

	MALLOC(sin6_p, struct sockaddr_in6 *, sizeof(*sin6_p), M_SONAME,
	    M_WAITOK);
	if (sin6_p == NULL) {
		return ENOBUFS;
	}
	sin_p = (struct sockaddr_in *)(void *)*nam;
	in6_sin_2_v4mapsin6(sin_p, sin6_p);
	FREE(*nam, M_SONAME);
	*nam = (struct sockaddr *)sin6_p;

	return 0;
}

  
                                              
  
                                                                          
                                                                        
                                                                   
                                     
   
void
in6_post_msg(struct ifnet *ifp, u_int32_t event_code, struct in6_ifaddr *ifa,
    uint8_t *mac)
{
	struct kev_msg ev_msg;
	struct kev_in6_data in6_event_data;
	struct in6_addrlifetime ia6_lt;

	bzero(&in6_event_data, sizeof(struct kev_in6_data));
	bzero(&ev_msg, sizeof(struct kev_msg));
	ev_msg.vendor_code      = KEV_VENDOR_APPLE;
	ev_msg.kev_class        = KEV_NETWORK_CLASS;
	ev_msg.kev_subclass     = KEV_INET6_SUBCLASS;
	ev_msg.event_code       = event_code;

	if (ifa) {
		IFA_LOCK(&ifa->ia_ifa);
		in6_event_data.ia_addr          = ifa->ia_addr;
		in6_event_data.ia_net           = ifa->ia_net;
		in6_event_data.ia_dstaddr       = ifa->ia_dstaddr;
		in6_event_data.ia_prefixmask    = ifa->ia_prefixmask;
		in6_event_data.ia_plen          = ifa->ia_plen;
		in6_event_data.ia6_flags        = (u_int32_t)ifa->ia6_flags;

		                                                    
		in6ifa_getlifetime(ifa, &ia6_lt, 1);
		in6_event_data.ia_lifetime.ia6t_expire = (u_int32_t)ia6_lt.ia6t_expire;
		in6_event_data.ia_lifetime.ia6t_preferred = (u_int32_t)ia6_lt.ia6t_preferred;
		in6_event_data.ia_lifetime.ia6t_vltime = ia6_lt.ia6t_vltime;
		in6_event_data.ia_lifetime.ia6t_pltime = ia6_lt.ia6t_pltime;
		IFA_UNLOCK(&ifa->ia_ifa);
	}

	if (ifp != NULL) {
		(void) strlcpy(&in6_event_data.link_data.if_name[0],
		    ifp->if_name, IFNAMSIZ);
		in6_event_data.link_data.if_family = ifp->if_family;
		in6_event_data.link_data.if_unit  = (u_int32_t)ifp->if_unit;
	}

	if (mac != NULL) {
		memcpy(&in6_event_data.ia_mac, mac,
		    sizeof(in6_event_data.ia_mac));
	}

	ev_msg.dv[0].data_ptr    = &in6_event_data;
	ev_msg.dv[0].data_length = sizeof(in6_event_data);
	ev_msg.dv[1].data_length = 0;

	dlil_post_complete_msg(NULL, &ev_msg);
}

  
                             
   
void
in6_ifaddr_init(void)
{
	in6_cga_init();
	in6_multi_init();

	PE_parse_boot_argn("ifa_debug", &in6ifa_debug, sizeof(in6ifa_debug));

	vm_size_t in6ifa_size = (in6ifa_debug == 0) ? sizeof(struct in6_ifaddr) :
	    sizeof(struct in6_ifaddr_dbg);

	in6ifa_zone = zone_create(IN6IFA_ZONE_NAME, in6ifa_size, ZC_ZFREE_CLEARMEM);

	lck_mtx_init(&in6ifa_trash_lock, ifa_mtx_grp, ifa_mtx_attr);
	TAILQ_INIT(&in6ifa_trash_head);
}

static struct in6_ifaddr *
in6_ifaddr_alloc(zalloc_flags_t how)
{
	struct in6_ifaddr *in6ifa;

	in6ifa = zalloc_flags(in6ifa_zone, how | Z_ZERO);
	if (in6ifa != NULL) {
		in6ifa->ia_ifa.ifa_free = in6_ifaddr_free;
		in6ifa->ia_ifa.ifa_debug |= IFD_ALLOC;
		in6ifa->ia_ifa.ifa_del_wc = &in6ifa->ia_ifa.ifa_debug;
		in6ifa->ia_ifa.ifa_del_waiters = 0;
		ifa_lock_init(&in6ifa->ia_ifa);
		if (in6ifa_debug != 0) {
			struct in6_ifaddr_dbg *in6ifa_dbg =
			    (struct in6_ifaddr_dbg *)in6ifa;
			in6ifa->ia_ifa.ifa_debug |= IFD_DEBUG;
			in6ifa->ia_ifa.ifa_trace = in6_ifaddr_trace;
			in6ifa->ia_ifa.ifa_attached = in6_ifaddr_attached;
			in6ifa->ia_ifa.ifa_detached = in6_ifaddr_detached;
			ctrace_record(&in6ifa_dbg->in6ifa_alloc);
		}
	}

	return in6ifa;
}

static void
in6_ifaddr_free(struct ifaddr *ifa)
{
	IFA_LOCK_ASSERT_HELD(ifa);

	if (ifa->ifa_refcnt != 0) {
		panic("%s: ifa %p bad ref cnt", __func__, ifa);
		                
	} else if (!(ifa->ifa_debug & IFD_ALLOC)) {
		panic("%s: ifa %p cannot be freed", __func__, ifa);
		                
	}
	if (ifa->ifa_debug & IFD_DEBUG) {
		struct in6_ifaddr_dbg *in6ifa_dbg =
		    (struct in6_ifaddr_dbg *)ifa;
		ctrace_record(&in6ifa_dbg->in6ifa_free);
		bcopy(&in6ifa_dbg->in6ifa, &in6ifa_dbg->in6ifa_old,
		    sizeof(struct in6_ifaddr));
		if (ifa->ifa_debug & IFD_TRASHED) {
			                                          
			IFA_CONVERT_LOCK(ifa);
			lck_mtx_lock(&in6ifa_trash_lock);
			TAILQ_REMOVE(&in6ifa_trash_head, in6ifa_dbg,
			    in6ifa_trash_link);
			lck_mtx_unlock(&in6ifa_trash_lock);
			ifa->ifa_debug &= ~IFD_TRASHED;
		}
	}
	IFA_UNLOCK(ifa);
	ifa_lock_destroy(ifa);
	bzero(ifa, sizeof(struct in6_ifaddr));
	zfree(in6ifa_zone, ifa);
}

static void
in6_ifaddr_attached(struct ifaddr *ifa)
{
	struct in6_ifaddr_dbg *in6ifa_dbg = (struct in6_ifaddr_dbg *)ifa;

	IFA_LOCK_ASSERT_HELD(ifa);

	if (!(ifa->ifa_debug & IFD_DEBUG)) {
		panic("%s: ifa %p has no debug structure", __func__, ifa);
		                
	}
	if (ifa->ifa_debug & IFD_TRASHED) {
		                                          
		IFA_CONVERT_LOCK(ifa);
		lck_mtx_lock(&in6ifa_trash_lock);
		TAILQ_REMOVE(&in6ifa_trash_head, in6ifa_dbg, in6ifa_trash_link);
		lck_mtx_unlock(&in6ifa_trash_lock);
		ifa->ifa_debug &= ~IFD_TRASHED;
	}
}

static void
in6_ifaddr_detached(struct ifaddr *ifa)
{
	struct in6_ifaddr_dbg *in6ifa_dbg = (struct in6_ifaddr_dbg *)ifa;

	IFA_LOCK_ASSERT_HELD(ifa);

	if (!(ifa->ifa_debug & IFD_DEBUG)) {
		panic("%s: ifa %p has no debug structure", __func__, ifa);
		                
	} else if (ifa->ifa_debug & IFD_TRASHED) {
		panic("%s: ifa %p is already in trash list", __func__, ifa);
		                
	}
	ifa->ifa_debug |= IFD_TRASHED;
	                                          
	IFA_CONVERT_LOCK(ifa);
	lck_mtx_lock(&in6ifa_trash_lock);
	TAILQ_INSERT_TAIL(&in6ifa_trash_head, in6ifa_dbg, in6ifa_trash_link);
	lck_mtx_unlock(&in6ifa_trash_lock);
}

static void
in6_ifaddr_trace(struct ifaddr *ifa, int refhold)
{
	struct in6_ifaddr_dbg *in6ifa_dbg = (struct in6_ifaddr_dbg *)ifa;
	ctrace_t *tr;
	u_int32_t idx;
	u_int16_t *cnt;

	if (!(ifa->ifa_debug & IFD_DEBUG)) {
		panic("%s: ifa %p has no debug structure", __func__, ifa);
		                
	}
	if (refhold) {
		cnt = &in6ifa_dbg->in6ifa_refhold_cnt;
		tr = in6ifa_dbg->in6ifa_refhold;
	} else {
		cnt = &in6ifa_dbg->in6ifa_refrele_cnt;
		tr = in6ifa_dbg->in6ifa_refrele;
	}

	idx = atomic_add_16_ov(cnt, 1) % IN6IFA_TRACE_HIST_SIZE;
	ctrace_record(&tr[idx]);
}

  
                                                  
   
static int
in6_getassocids(struct socket *so, uint32_t *cnt, user_addr_t aidp)
{
	struct in6pcb *in6p = sotoin6pcb(so);
	sae_associd_t aid;

	if (in6p == NULL || in6p->inp_state == INPCB_STATE_DEAD) {
		return EINVAL;
	}

	                                          
	aid = SAE_ASSOCID_ANY;
	*cnt = 0;

	                                     
	if (aidp == USER_ADDR_NULL) {
		return 0;
	}

	return copyout(&aid, aidp, sizeof(aid));
}

  
                                                 
   
static int
in6_getconnids(struct socket *so, sae_associd_t aid, uint32_t *cnt,
    user_addr_t cidp)
{
	struct in6pcb *in6p = sotoin6pcb(so);
	sae_connid_t cid;

	if (in6p == NULL || in6p->inp_state == INPCB_STATE_DEAD) {
		return EINVAL;
	}

	if (aid != SAE_ASSOCID_ANY && aid != SAE_ASSOCID_ALL) {
		return EINVAL;
	}

	                                             
	*cnt = ((so->so_state & SS_ISCONNECTED) ? 1 : 0);

	                                     
	if (cidp == USER_ADDR_NULL) {
		return 0;
	}

	                                                
	cid = ((*cnt != 0) ? 1 : SAE_CONNID_ANY);

	return copyout(&cid, cidp, sizeof(cid));
}

  
                                                  
   
int
in6_getconninfo(struct socket *so, sae_connid_t cid, uint32_t *flags,
    uint32_t *ifindex, int32_t *soerror, user_addr_t src, socklen_t *src_len,
    user_addr_t dst, socklen_t *dst_len, uint32_t *aux_type,
    user_addr_t aux_data, uint32_t *aux_len)
{
	struct in6pcb *in6p = sotoin6pcb(so);
	struct sockaddr_in6 sin6;
	struct ifnet *ifp = NULL;
	int error = 0;
	u_int32_t copy_len = 0;

	  
                                                            
                                                         
    
	if (in6p == NULL) {
		error = EINVAL;
		goto out;
	}

	if (cid != SAE_CONNID_ANY && cid != SAE_CONNID_ALL && cid != 1) {
		error = EINVAL;
		goto out;
	}

	ifp = in6p->in6p_last_outifp;
	*ifindex = ((ifp != NULL) ? ifp->if_index : 0);
	*soerror = so->so_error;
	*flags = 0;
	if (so->so_state & SS_ISCONNECTED) {
		*flags |= (CIF_CONNECTED | CIF_PREFERRED);
	}
	if (in6p->in6p_flags & INP_BOUND_IF) {
		*flags |= CIF_BOUND_IF;
	}
	if (!(in6p->in6p_flags & INP_IN6ADDR_ANY)) {
		*flags |= CIF_BOUND_IP;
	}
	if (!(in6p->in6p_flags & INP_ANONPORT)) {
		*flags |= CIF_BOUND_PORT;
	}

	bzero(&sin6, sizeof(sin6));
	sin6.sin6_len = sizeof(sin6);
	sin6.sin6_family = AF_INET6;

	                             
	sin6.sin6_port = in6p->in6p_lport;
	in6_recoverscope(&sin6, &in6p->in6p_laddr, NULL);
	if (*src_len == 0) {
		*src_len = sin6.sin6_len;
	} else {
		if (src != USER_ADDR_NULL) {
			copy_len = min(*src_len, sizeof(sin6));
			error = copyout(&sin6, src, copy_len);
			if (error != 0) {
				goto out;
			}
			*src_len = copy_len;
		}
	}

	                                  
	sin6.sin6_port = in6p->in6p_fport;
	in6_recoverscope(&sin6, &in6p->in6p_faddr, NULL);
	if (*dst_len == 0) {
		*dst_len = sin6.sin6_len;
	} else {
		if (dst != USER_ADDR_NULL) {
			copy_len = min(*dst_len, sizeof(sin6));
			error = copyout(&sin6, dst, copy_len);
			if (error != 0) {
				goto out;
			}
			*dst_len = copy_len;
		}
	}

	if (SOCK_PROTO(so) == IPPROTO_TCP) {
		struct conninfo_tcp tcp_ci;

		*aux_type = CIAUX_TCP;
		if (*aux_len == 0) {
			*aux_len = sizeof(tcp_ci);
		} else {
			if (aux_data != USER_ADDR_NULL) {
				copy_len = min(*aux_len, sizeof(tcp_ci));
				bzero(&tcp_ci, sizeof(tcp_ci));
				tcp_getconninfo(so, &tcp_ci);
				error = copyout(&tcp_ci, aux_data, copy_len);
				if (error != 0) {
					goto out;
				}
				*aux_len = copy_len;
			}
		}
	} else {
		*aux_type = 0;
		*aux_len = 0;
	}

out:
	return error;
}

  
                    
  
                                                                        
                                                                        
                                                                        
                                                                       
                                                      
  
                                                                  
   
static __attribute__((unused)) void
in6ioctl_cassert(void)
{
	  
                                                              
                                                          
    
	switch ((u_long)0) {
	case 0:

	                            
	case SIOCAADDRCTL_POLICY:
	case SIOCDADDRCTL_POLICY:
	case SIOCDRADD_IN6_32:
	case SIOCDRADD_IN6_64:
	case SIOCDRDEL_IN6_32:
	case SIOCDRDEL_IN6_64:
		;
	}
}

struct in6_llentry {
	struct llentry          base;
};

#define        IN6_LLTBL_DEFAULT_HSIZE 32
#define        IN6_LLTBL_HASH(k, h) \
	((((((((k) >> 8) ^ (k)) >> 8) ^ (k)) >> 8) ^ (k)) & ((h) - 1))

  
                                  
   
static void
in6_lltable_destroy_lle_unlocked(struct llentry *lle)
{
	LLE_LOCK_DESTROY(lle);
	LLE_REQ_DESTROY(lle);
	FREE(lle, M_LLTABLE);
}

  
                                                      
                 
   
static void
in6_lltable_destroy_lle(struct llentry *lle)
{
	LLE_WUNLOCK(lle);
	             
	                                   
	in6_lltable_destroy_lle_unlocked(lle);
}


static struct llentry *
in6_lltable_new(const struct in6_addr *addr6, uint16_t flags)
{
#pragma unused(flags)
	struct in6_llentry *lle;

	MALLOC(lle, struct in6_llentry *, sizeof(struct in6_llentry), M_LLTABLE, M_NOWAIT | M_ZERO);
	if (lle == NULL) {                                            
		return NULL;
	}

	lle->base.r_l3addr.addr6 = *addr6;
	lle->base.lle_refcnt = 1;
	lle->base.lle_free = in6_lltable_destroy_lle;
	LLE_LOCK_INIT(&lle->base);
	LLE_REQ_INIT(&lle->base);
#if 0
	             
	lle->base.lle_timer = thread_call_allocate(nd6_llinfo_timer, lle);

	if (lle->base.lle_timer == NULL) {
		printf("lle_timer thread call could not be allocated.\n");
		LLE_LOCK_DESTROY(&lle->base);
		LLE_REQ_DESTROY(&lle->base);
		FREE(lle, M_LLTABLE);
		return NULL;
	}
#endif
	return &lle->base;
}

static int
in6_lltable_match_prefix(const struct sockaddr *saddr,
    const struct sockaddr *smask, uint16_t flags, struct llentry *lle)
{
	const struct in6_addr *addr, *mask, *lle_addr;

	addr = &((const struct sockaddr_in6 *)(const void *)saddr)->sin6_addr;
	mask = &((const struct sockaddr_in6 *)(const void *)smask)->sin6_addr;
	lle_addr = &lle->r_l3addr.addr6;

	if (IN6_ARE_MASKED_ADDR_EQUAL(lle_addr, addr, mask) == 0) {
		return 0;
	}

	if (lle->la_flags & LLE_IFADDR) {
		  
                                                          
                                                          
                   
     
		if (IN6_ARE_ADDR_EQUAL(addr, lle_addr) &&
		    (flags & LLE_STATIC) != 0) {
			return 1;
		}
		return 0;
	}

	                                                                       
	if ((flags & LLE_STATIC) || !(lle->la_flags & LLE_STATIC)) {
		return 1;
	}

	return 0;
}

static void
in6_lltable_free_entry(struct lltable *llt, struct llentry *lle)
{
	struct ifnet *ifp;

	LLE_WLOCK_ASSERT(lle);
	KASSERT(llt != NULL, ("lltable is NULL"));

	                             
	if ((lle->la_flags & LLE_LINKED) != 0) {
		ifp = llt->llt_ifp;
		if_afdata_wlock_assert(ifp, llt->llt_af);
		lltable_unlink_entry(llt, lle);
	}

#if 0
	             
	if (thread_call_cancel(lle->lle_timer) == TRUE) {
		LLE_REMREF(lle);
	}
#endif
	llentry_free(lle);
}

static int
in6_lltable_rtcheck(struct ifnet *ifp,
    uint16_t flags, const struct sockaddr *l3addr)
{
#pragma unused(flags)
	struct rtentry *rt;

	KASSERT(l3addr->sa_family == AF_INET6,
	    ("sin_family %d", l3addr->sa_family));
	                                            
	rt = rtalloc1(__DECONST(struct sockaddr *, l3addr), 0, 0);
	if (rt == NULL || (rt->rt_flags & RTF_GATEWAY) || rt->rt_ifp != ifp) {
		struct ifaddr *ifa;
		  
                                             
                                           
     
		                                                    
		ifa = ifaof_ifpforaddr(__DECONST(struct sockaddr *, l3addr), ifp);
		if (ifa != NULL) {
			IFA_REMREF(ifa);
			if (rt != NULL) {
				rtfree(rt);
			}
			return 0;
		}
		log(LOG_INFO, "IPv6 address: \"%s\" is not on the network\n",
		    ip6_sprintf(&((const struct sockaddr_in6 *)(const void *)l3addr)->sin6_addr));
		if (rt != NULL) {
			rtfree(rt);
		}
		return EINVAL;
	}
	rtfree(rt);
	return 0;
}

static inline uint32_t
in6_lltable_hash_dst(const struct in6_addr *dst, uint32_t hsize)
{
	return IN6_LLTBL_HASH(dst->s6_addr32[3], hsize);
}

static uint32_t
in6_lltable_hash(const struct llentry *lle, uint32_t hsize)
{
	return in6_lltable_hash_dst(&lle->r_l3addr.addr6, hsize);
}

static void
in6_lltable_fill_sa_entry(const struct llentry *lle, struct sockaddr *sa)
{
	struct sockaddr_in6 *sin6;

	sin6 = (struct sockaddr_in6 *)(void *)sa;
	bzero(sin6, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(*sin6);
	sin6->sin6_addr = lle->r_l3addr.addr6;
}

static inline struct llentry *
in6_lltable_find_dst(struct lltable *llt, const struct in6_addr *dst)
{
	struct llentry *lle;
	struct llentries *lleh;
	u_int hashidx;

	hashidx = in6_lltable_hash_dst(dst, llt->llt_hsize);
	lleh = &llt->lle_head[hashidx];
	LIST_FOREACH(lle, lleh, lle_next) {
		if (lle->la_flags & LLE_DELETED) {
			continue;
		}
		if (IN6_ARE_ADDR_EQUAL(&lle->r_l3addr.addr6, dst)) {
			break;
		}
	}

	return lle;
}

static void
in6_lltable_delete_entry(struct lltable *llt, struct llentry *lle)
{
#pragma unused(llt)
	lle->la_flags |= LLE_DELETED;
	EVENTHANDLER_INVOKE(NULL, lle_event, lle, LLENTRY_DELETED);
#ifdef DIAGNOSTIC
	log(LOG_INFO, "ifaddr cache = %p is deleted\n", lle);
#endif
	llentry_free(lle);
}

static struct llentry *
in6_lltable_alloc(struct lltable *llt, uint16_t flags,
    const struct sockaddr *l3addr)
{
	const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)(const void *)l3addr;
	struct ifnet *ifp = llt->llt_ifp;
	struct llentry *lle;

	KASSERT(l3addr->sa_family == AF_INET6,
	    ("sin_family %d", l3addr->sa_family));

	  
                                                   
                                                         
                
    
	if (!(flags & LLE_IFADDR) &&
	    in6_lltable_rtcheck(ifp, flags, l3addr) != 0) {
		return NULL;
	}

	lle = in6_lltable_new(&sin6->sin6_addr, flags);
	if (lle == NULL) {
		log(LOG_INFO, "lla_lookup: new lle malloc failed\n");
		return NULL;
	}
	lle->la_flags = (uint16_t)flags;
	if ((flags & LLE_IFADDR) == LLE_IFADDR) {
		lltable_set_entry_addr(ifp, lle, LLADDR(SDL(ifp->if_lladdr->ifa_addr)));
		lle->la_flags |= LLE_STATIC;
	}

	if ((lle->la_flags & LLE_STATIC) != 0) {
		lle->ln_state = ND6_LLINFO_REACHABLE;
	}

	return lle;
}

static struct llentry *
in6_lltable_lookup(struct lltable *llt, uint16_t flags,
    const struct sockaddr *l3addr)
{
	const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)(const void *)l3addr;
	struct llentry *lle;

	IF_AFDATA_LOCK_ASSERT(llt->llt_ifp, llt->llt_af);
	KASSERT(l3addr->sa_family == AF_INET6,
	    ("sin_family %d", l3addr->sa_family));

	lle = in6_lltable_find_dst(llt, &sin6->sin6_addr);

	if (lle == NULL) {
		return NULL;
	}

	KASSERT((flags & (LLE_UNLOCKED | LLE_EXCLUSIVE)) !=
	    (LLE_UNLOCKED | LLE_EXCLUSIVE), ("wrong lle request flags: 0x%X",
	    flags));

	if (flags & LLE_UNLOCKED) {
		return lle;
	}

	if (flags & LLE_EXCLUSIVE) {
		LLE_WLOCK(lle);
	} else {
		LLE_RLOCK(lle);
	}
	return lle;
}

static int
in6_lltable_dump_entry(struct lltable *llt, struct llentry *lle,
    struct sysctl_req *wr)
{
	struct ifnet *ifp = llt->llt_ifp;
	                   
	struct {
		struct rt_msghdr        rtm;
		struct sockaddr_in6     sin6;
		  
                                           
     
#ifdef __LP64__
		uint32_t                pad;
#endif
		struct sockaddr_dl      sdl;
	} ndpc;
	struct sockaddr_dl *sdl;
	int error;

	bzero(&ndpc, sizeof(ndpc));
	                          
	if ((lle->la_flags & LLE_DELETED) == LLE_DELETED) {
		return 0;
	}
	                                                      
	lltable_fill_sa_entry(lle,
	    (struct sockaddr *)&ndpc.sin6);
	  
                          
                      
                               
                        
    
	ndpc.rtm.rtm_msglen = sizeof(ndpc);
	ndpc.rtm.rtm_version = RTM_VERSION;
	ndpc.rtm.rtm_type = RTM_GET;
	ndpc.rtm.rtm_flags = RTF_UP;
	ndpc.rtm.rtm_addrs = RTA_DST | RTA_GATEWAY;

	             
	if (lle->la_flags & LLE_PUB) {
		ndpc.rtm.rtm_flags |= RTF_ANNOUNCE;
	}
	sdl = &ndpc.sdl;
	sdl->sdl_family = AF_LINK;
	sdl->sdl_len = sizeof(*sdl);
	sdl->sdl_index = ifp->if_index;
	sdl->sdl_type = ifp->if_type;
	if ((lle->la_flags & LLE_VALID) == LLE_VALID) {
		sdl->sdl_alen = ifp->if_addrlen;
		bcopy(&lle->ll_addr, LLADDR(sdl), ifp->if_addrlen);
	} else {
		sdl->sdl_alen = 0;
		bzero(LLADDR(sdl), ifp->if_addrlen);
	}
	if (lle->la_expire != 0) {
		clock_sec_t secs;
		clock_usec_t usecs;

		clock_get_calendar_microtime(&secs, &usecs);
		ndpc.rtm.rtm_rmx.rmx_expire = (int32_t)(lle->la_expire +
		    lle->lle_remtime / hz +
		    secs - net_uptime());
	}
	ndpc.rtm.rtm_flags |= (RTF_HOST | RTF_LLDATA);
	if (lle->la_flags & LLE_STATIC) {
		ndpc.rtm.rtm_flags |= RTF_STATIC;
	}
	if (lle->la_flags & LLE_IFADDR) {
		ndpc.rtm.rtm_flags |= RTF_PINNED;
	}
	if (lle->ln_router != 0) {
		ndpc.rtm.rtm_flags |= RTF_GATEWAY;
	}
	ndpc.rtm.rtm_rmx.rmx_pksent = lle->la_asked;
	                                     
	ndpc.rtm.rtm_rmx.rmx_state = lle->ln_state;
	ndpc.rtm.rtm_index = ifp->if_index;
	error = SYSCTL_OUT(wr, &ndpc, sizeof(ndpc));

	return error;
}

struct lltable *
in6_lltattach(struct ifnet *ifp)
{
	struct lltable *llt;

	llt = lltable_allocate_htbl(IN6_LLTBL_DEFAULT_HSIZE);
	llt->llt_af = AF_INET6;
	llt->llt_ifp = ifp;

	llt->llt_lookup = in6_lltable_lookup;
	llt->llt_alloc_entry = in6_lltable_alloc;
	llt->llt_delete_entry = in6_lltable_delete_entry;
	llt->llt_dump_entry = in6_lltable_dump_entry;
	llt->llt_hash = in6_lltable_hash;
	llt->llt_fill_sa_entry = in6_lltable_fill_sa_entry;
	llt->llt_free_entry = in6_lltable_free_entry;
	llt->llt_match_prefix = in6_lltable_match_prefix;
	lltable_link(llt);

	return llt;
}

void
in6_ip6_to_sockaddr(const struct in6_addr *ip6, u_int16_t port,
    struct sockaddr_in6 *sin6, u_int32_t maxlen)
{
	if (maxlen < sizeof(struct sockaddr_in6)) {
		return;
	}

	*sin6 = (struct sockaddr_in6) {
		.sin6_family = AF_INET6,
		.sin6_len = sizeof(*sin6),
		.sin6_port = port,
		.sin6_addr = *ip6,
	};

	if (IN6_IS_SCOPE_EMBED(&sin6->sin6_addr)) {
		sin6->sin6_scope_id = ntohs(sin6->sin6_addr.s6_addr16[1]);
		sin6->sin6_addr.s6_addr16[1] = 0;
	}
}

                 
struct in6_event {
	in6_evhdlr_code_t in6_event_code;
	struct ifnet *in6_ifp;
	struct in6_addr in6_address;
	uint32_t val;
};

struct in6_event2kev in6_event2kev_array[IN6_EVENT_MAX] = {
	{
		.in6_event_code = IN6_ADDR_MARKED_DUPLICATED,
		.in6_event_kev_subclass = KEV_ND6_SUBCLASS,
		.in6_event_kev_code = KEV_ND6_DAD_FAILURE,
		.in6_event_str = "IN6_ADDR_MARKED_DUPLICATED",
	},
	{
		.in6_event_code = IN6_ADDR_MARKED_DETACHED,
		.in6_event_kev_subclass = KEV_ND6_SUBCLASS,
		.in6_event_kev_code = KEV_ND6_ADDR_DETACHED,
		.in6_event_str = "IN6_ADDR_MARKED_DETACHED",
	},
	{
		.in6_event_code = IN6_ADDR_MARKED_DEPRECATED,
		.in6_event_kev_subclass = KEV_ND6_SUBCLASS,
		.in6_event_kev_code = KEV_ND6_ADDR_DEPRECATED,
		.in6_event_str = "IN6_ADDR_MARKED_DEPRECATED",
	},
	{
		.in6_event_code = IN6_NDP_RTR_EXPIRY,
		.in6_event_kev_subclass = KEV_ND6_SUBCLASS,
		.in6_event_kev_code = KEV_ND6_RTR_EXPIRED,
		.in6_event_str = "IN6_NDP_RTR_EXPIRY",
	},
	{
		.in6_event_code = IN6_NDP_PFX_EXPIRY,
		.in6_event_kev_subclass = KEV_ND6_SUBCLASS,
		.in6_event_kev_code = KEV_ND6_PFX_EXPIRED,
		.in6_event_str = "IN6_NDP_PFX_EXPIRY",
	},
	{
		.in6_event_code = IN6_NDP_ADDR_EXPIRY,
		.in6_event_kev_subclass = KEV_ND6_SUBCLASS,
		.in6_event_kev_code = KEV_ND6_ADDR_EXPIRED,
		.in6_event_str = "IN6_NDP_ADDR_EXPIRY",
	},
};

void
in6_eventhdlr_callback(struct eventhandler_entry_arg arg0 __unused,
    in6_evhdlr_code_t in6_ev_code, struct ifnet *ifp,
    struct in6_addr *p_addr6, uint32_t val)
{
	struct kev_msg ev_msg;
	struct kev_nd6_event nd6_event;

	bzero(&ev_msg, sizeof(ev_msg));
	bzero(&nd6_event, sizeof(nd6_event));

	nd6log0(info, "%s Event %s received for %s\n",
	    __func__, in6_event2kev_array[in6_ev_code].in6_event_str,
	    ip6_sprintf(p_addr6));

	ev_msg.vendor_code      = KEV_VENDOR_APPLE;
	ev_msg.kev_class        = KEV_NETWORK_CLASS;
	ev_msg.kev_subclass     =
	    in6_event2kev_array[in6_ev_code].in6_event_kev_subclass;
	ev_msg.event_code       =
	    in6_event2kev_array[in6_ev_code].in6_event_kev_code;

	nd6_event.link_data.if_family = ifp->if_family;
	nd6_event.link_data.if_unit = ifp->if_unit;
	strlcpy(nd6_event.link_data.if_name, ifp->if_name,
	    sizeof(nd6_event.link_data.if_name));

	VERIFY(p_addr6 != NULL);
	bcopy(p_addr6, &nd6_event.in6_address,
	    sizeof(nd6_event.in6_address));
	nd6_event.val = val;

	ev_msg.dv[0].data_ptr = &nd6_event;
	ev_msg.dv[0].data_length = sizeof(nd6_event);

	kev_post_msg(&ev_msg);
}

static void
in6_event_callback(void *arg)
{
	struct in6_event *p_in6_ev = (struct in6_event *)arg;

	EVENTHANDLER_INVOKE(&in6_evhdlr_ctxt, in6_event,
	    p_in6_ev->in6_event_code, p_in6_ev->in6_ifp,
	    &p_in6_ev->in6_address, p_in6_ev->val);
}

struct in6_event_nwk_wq_entry {
	struct nwk_wq_entry nwk_wqe;
	struct in6_event in6_ev_arg;
};

void
in6_event_enqueue_nwk_wq_entry(in6_evhdlr_code_t in6_event_code,
    struct ifnet *ifp, struct in6_addr *p_addr6,
    uint32_t val)
{
	struct in6_event_nwk_wq_entry *p_in6_ev = NULL;

	MALLOC(p_in6_ev, struct in6_event_nwk_wq_entry *,
	    sizeof(struct in6_event_nwk_wq_entry),
	    M_NWKWQ, M_WAITOK | M_ZERO);

	p_in6_ev->nwk_wqe.func = in6_event_callback;
	p_in6_ev->nwk_wqe.is_arg_managed = TRUE;
	p_in6_ev->nwk_wqe.arg = &p_in6_ev->in6_ev_arg;

	p_in6_ev->in6_ev_arg.in6_event_code = in6_event_code;
	p_in6_ev->in6_ev_arg.in6_ifp = ifp;
	if (p_addr6 != NULL) {
		bcopy(p_addr6, &p_in6_ev->in6_ev_arg.in6_address,
		    sizeof(p_in6_ev->in6_ev_arg.in6_address));
	}
	p_in6_ev->in6_ev_arg.val = val;

	nwk_wq_enqueue((struct nwk_wq_entry*)p_in6_ev);
}

  
                                                
   
static void
in6_iahash_remove(struct in6_ifaddr *ia)
{
	LCK_RW_ASSERT(&in6_ifaddr_rwlock, LCK_RW_ASSERT_EXCLUSIVE);
	IFA_LOCK_ASSERT_HELD(&ia->ia_ifa);

	if (!IA6_IS_HASHED(ia)) {
		panic("%s: attempt to remove wrong ia %p from ipv6 hash table\n", __func__, ia);
		                
	}
	TAILQ_REMOVE(IN6ADDR_HASH(&ia->ia_addr.sin6_addr), ia, ia6_hash);
	IA6_HASH_INIT(ia);
	if (IFA_REMREF_LOCKED(&ia->ia_ifa) == NULL) {
		panic("%s: unexpected (missing) refcnt ifa=%p", __func__,
		    &ia->ia_ifa);
		                
	}
}

  
                                                
   
static void
in6_iahash_insert(struct in6_ifaddr *ia)
{
	LCK_RW_ASSERT(&in6_ifaddr_rwlock, LCK_RW_ASSERT_EXCLUSIVE);
	IFA_LOCK_ASSERT_HELD(&ia->ia_ifa);

	if (ia->ia_addr.sin6_family != AF_INET6) {
		panic("%s: attempt to insert wrong ia %p into hash table\n", __func__, ia);
		                
	} else if (IA6_IS_HASHED(ia)) {
		panic("%s: attempt to double-insert ia %p into hash table\n", __func__, ia);
		                
	}
	TAILQ_INSERT_HEAD(IN6ADDR_HASH(&ia->ia_addr.sin6_addr),
	    ia, ia6_hash);
	IFA_ADDREF_LOCKED(&ia->ia_ifa);
}

  
                                                                          
                                                                         
                                                                             
                                                                 
                       
  
                                                
   
static void
in6_iahash_insert_ptp(struct in6_ifaddr *ia)
{
	struct in6_ifaddr *tmp_ifa;
	struct ifnet *tmp_ifp;

	LCK_RW_ASSERT(&in6_ifaddr_rwlock, LCK_RW_ASSERT_EXCLUSIVE);
	IFA_LOCK_ASSERT_HELD(&ia->ia_ifa);

	if (ia->ia_addr.sin6_family != AF_INET6) {
		panic("%s: attempt to insert wrong ia %p into hash table\n", __func__, ia);
		                
	} else if (IA6_IS_HASHED(ia)) {
		panic("%s: attempt to double-insert ia %p into hash table\n", __func__, ia);
		                
	}
	IFA_UNLOCK(&ia->ia_ifa);
	TAILQ_FOREACH(tmp_ifa, IN6ADDR_HASH(&ia->ia_addr.sin6_addr), ia6_hash) {
		IFA_LOCK(&tmp_ifa->ia_ifa);
		                                                     
		if (IN6_ARE_ADDR_EQUAL(&tmp_ifa->ia_addr.sin6_addr, &ia->ia_addr.sin6_addr)) {
			IFA_UNLOCK(&tmp_ifa->ia_ifa);
			break;
		}
		IFA_UNLOCK(&tmp_ifa->ia_ifa);
	}
	tmp_ifp = (tmp_ifa == NULL) ? NULL : tmp_ifa->ia_ifp;

	IFA_LOCK(&ia->ia_ifa);
	if (tmp_ifp == NULL) {
		TAILQ_INSERT_HEAD(IN6ADDR_HASH(&ia->ia_addr.sin6_addr),
		    ia, ia6_hash);
	} else {
		TAILQ_INSERT_TAIL(IN6ADDR_HASH(&ia->ia_addr.sin6_addr),
		    ia, ia6_hash);
	}
	IFA_ADDREF_LOCKED(&ia->ia_ifa);
}
