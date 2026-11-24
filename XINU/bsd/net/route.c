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
 * Copyright (c) 1980, 1986, 1991, 1993
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
 *	@(#)route.c	8.2 (Berkeley) 11/15/93
 * $FreeBSD: src/sys/net/route.c,v 1.59.2.3 2001/07/29 19:18:02 ume Exp $
 */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/domain.h>
#include <sys/stat.h>
#include <sys/ubc.h>
#include <sys/vnode.h>
#include <sys/syslog.h>
#include <sys/queue.h>
#include <sys/mcache.h>
#include <sys/priv.h>
#include <sys/protosw.h>
#include <sys/sdt.h>
#include <sys/kernel.h>
#include <kern/locks.h>
#include <kern/zalloc.h>

#include <net/dlil.h>
#include <net/if.h>
#include <net/route.h>
#include <net/ntstat.h>
#include <net/nwk_wq.h>
#if NECP
#include <net/necp.h>
#endif           

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/in_arp.h>

#include <netinet6/ip6_var.h>
#include <netinet6/in6_var.h>
#include <netinet6/nd6.h>

#include <net/if_dl.h>

#include <libkern/OSAtomic.h>
#include <libkern/OSDebug.h>

#include <pexpert/pexpert.h>

#if CONFIG_MACF
#include <sys/kauth.h>
#endif

  
                         
  
                                                                           
                                                                               
                                                         
  
                                                          
  
                                                                         
                                                                             
                                                                         
                                                                           
                                                                           
  
                                                                            
                                                                           
                                               
  
                                                                            
                                                                              
                                                                           
                                                                      
  
                                                                          
  
             
  
                                   
  
                                                    
  
                                                                  
  
                                                                 
  
                                                        
  
                                                                  
                                                                 
                                                                 
                                                                      
                                                           
                                                          
  
                                     
  
                                                                 
  
                                                                  
                                                               
  
                                                                    
                                                                    
                                                                 
                                                                 
                                                                    
                                                                 
                        
  
                                                                       
                                                                      
                                                
  
           
  
                                                     
  
                    
  
                                
  
                                                           
  
                                                                            
  
                                                                       
                                                                       
                                                              
  
                                                                         
                                                                          
                                                    
  
                                                                          
                                                                          
                                                                           
                                                                      
                                                                         
                                            
  
                                                                      
                                                                        
                                                                         
                                                                         
                                                                
   

#define equal(a1, a2) (bcmp((caddr_t)(a1), (caddr_t)(a2), (a1)->sa_len) == 0)

extern void kdp_set_gateway_mac(void *gatewaymac);

__private_extern__ struct rtstat rtstat  = {
	.rts_badredirect = 0,
	.rts_dynamic = 0,
	.rts_newgateway = 0,
	.rts_unreach = 0,
	.rts_wildcard = 0,
	.rts_badrtgwroute = 0
};
struct radix_node_head *rt_tables[AF_MAX + 1];

decl_lck_mtx_data(, rnh_lock_data);                                      
lck_mtx_t               *rnh_lock = &rnh_lock_data;
static lck_attr_t       *rnh_lock_attr;
static lck_grp_t        *rnh_lock_grp;
static lck_grp_attr_t   *rnh_lock_grp_attr;

                                                      
static lck_attr_t       *rte_mtx_attr;
static lck_grp_t        *rte_mtx_grp;
static lck_grp_attr_t   *rte_mtx_grp_attr;

int rttrash = 0;                                                       

boolean_t trigger_v6_defrtr_select = FALSE;
unsigned int rte_debug = 0;

                                  
#define RTD_DEBUG       0x1                                                   
#define RTD_TRACE       0x2                                             
#define RTD_NO_FREE     0x4                                                 

#define RTE_NAME                "rtentry"                                      

static struct zone *rte_zone;                                                 
#define RTE_ZONE_MAX            65536                                         
#define RTE_ZONE_NAME           RTE_NAME                                  

#define RTD_INUSE               0xFEEDFACE                           
#define RTD_FREED               0xDEADBEEF                          

#define MAX_SCOPE_ADDR_STR_LEN  (MAX_IPv6_STR_LEN + 6)

             
__private_extern__ unsigned int ctrace_stack_size = CTRACE_STACK_SIZE;
__private_extern__ unsigned int ctrace_hist_size = CTRACE_HIST_SIZE;

  
                                      
   
struct rtentry_dbg {
	struct rtentry  rtd_entry;                                   
	struct rtentry  rtd_entry_saved;                                   
	uint32_t        rtd_inuse;                                          
	uint16_t        rtd_refhold_cnt;                                
	uint16_t        rtd_refrele_cnt;                                  
	uint32_t        rtd_lock_cnt;                                   
	uint32_t        rtd_unlock_cnt;                                   
	  
                           
    
	ctrace_t        rtd_alloc;
	ctrace_t        rtd_free;
	  
                                                
    
	ctrace_t        rtd_refhold[CTRACE_HIST_SIZE];
	ctrace_t        rtd_refrele[CTRACE_HIST_SIZE];
	  
                                        
    
	ctrace_t        rtd_lock[CTRACE_HIST_SIZE];
	ctrace_t        rtd_unlock[CTRACE_HIST_SIZE];
	  
                      
    
	TAILQ_ENTRY(rtentry_dbg) rtd_trash_link;
};

                                                       
static TAILQ_HEAD(, rtentry_dbg) rttrash_head;

static void rte_lock_init(struct rtentry *);
static void rte_lock_destroy(struct rtentry *);
static inline struct rtentry *rte_alloc_debug(void);
static inline void rte_free_debug(struct rtentry *);
static inline void rte_lock_debug(struct rtentry_dbg *);
static inline void rte_unlock_debug(struct rtentry_dbg *);
static void rt_maskedcopy(const struct sockaddr *,
    struct sockaddr *, const struct sockaddr *);
static void rtable_init(void **);
static inline void rtref_audit(struct rtentry_dbg *);
static inline void rtunref_audit(struct rtentry_dbg *);
static struct rtentry *rtalloc1_common_locked(struct sockaddr *, int, uint32_t,
    unsigned int);
static int rtrequest_common_locked(int, struct sockaddr *,
    struct sockaddr *, struct sockaddr *, int, struct rtentry **,
    unsigned int);
static struct rtentry *rtalloc1_locked(struct sockaddr *, int, uint32_t);
static void rtalloc_ign_common_locked(struct route *, uint32_t, unsigned int);
static inline void sin6_set_ifscope(struct sockaddr *, unsigned int);
static inline void sin6_set_embedded_ifscope(struct sockaddr *, unsigned int);
static inline unsigned int sin6_get_embedded_ifscope(struct sockaddr *);
static struct sockaddr *ma_copy(int, struct sockaddr *,
    struct sockaddr_storage *, unsigned int);
static struct sockaddr *sa_trim(struct sockaddr *, int);
static struct radix_node *node_lookup(struct sockaddr *, struct sockaddr *,
    unsigned int);
static struct radix_node *node_lookup_default(int);
static struct rtentry *rt_lookup_common(boolean_t, boolean_t, struct sockaddr *,
    struct sockaddr *, struct radix_node_head *, unsigned int);
static int rn_match_ifscope(struct radix_node *, void *);
static struct ifaddr *ifa_ifwithroute_common_locked(int,
    const struct sockaddr *, const struct sockaddr *, unsigned int);
static struct rtentry *rte_alloc(void);
static void rte_free(struct rtentry *);
static void rtfree_common(struct rtentry *, boolean_t);
static void rte_if_ref(struct ifnet *, int);
static void rt_set_idleref(struct rtentry *);
static void rt_clear_idleref(struct rtentry *);
static void route_event_callback(void *);
static void rt_str4(struct rtentry *, char *, uint32_t, char *, uint32_t);
static void rt_str6(struct rtentry *, char *, uint32_t, char *, uint32_t);
static boolean_t route_ignore_protocol_cloning_for_dst(struct rtentry *, struct sockaddr *);

uint32_t route_genid_inet = 0;
uint32_t route_genid_inet6 = 0;

#define ASSERT_SINIFSCOPE(sa) {                                         \
	if ((sa)->sa_family != AF_INET ||                               \
	    (sa)->sa_len < sizeof (struct sockaddr_in))                 \
	        panic("%s: bad sockaddr_in %p\n", __func__, sa);        \
}

#define ASSERT_SIN6IFSCOPE(sa) {                                        \
	if ((sa)->sa_family != AF_INET6 ||                              \
	    (sa)->sa_len < sizeof (struct sockaddr_in6))                \
	        panic("%s: bad sockaddr_in6 %p\n", __func__, sa);       \
}

  
                                                                     
                                                                          
   
struct matchleaf_arg {
	unsigned int    ifscope;                             
};

  
                                                                
                                   
   
static struct sockaddr sin_def = {
	.sa_len = sizeof(struct sockaddr_in),
	.sa_family = AF_INET,
	.sa_data = { 0, }
};

static struct sockaddr_in6 sin6_def = {
	.sin6_len = sizeof(struct sockaddr_in6),
	.sin6_family = AF_INET6,
	.sin6_port = 0,
	.sin6_flowinfo = 0,
	.sin6_addr = IN6ADDR_ANY_INIT,
	.sin6_scope_id = 0
};

  
                                                                  
                                                                  
                                      
   
static unsigned int primary_ifscope = IFSCOPE_NONE;
static unsigned int primary6_ifscope = IFSCOPE_NONE;

#define INET_DEFAULT(sa)        \
	((sa)->sa_family == AF_INET && SIN(sa)->sin_addr.s_addr == 0)

#define INET6_DEFAULT(sa)                                               \
	((sa)->sa_family == AF_INET6 &&                                 \
	IN6_IS_ADDR_UNSPECIFIED(&SIN6(sa)->sin6_addr))

#define SA_DEFAULT(sa)  (INET_DEFAULT(sa) || INET6_DEFAULT(sa))
#define RT(r)           ((struct rtentry *)r)
#define RN(r)           ((struct radix_node *)r)
#define RT_HOST(r)      (RT(r)->rt_flags & RTF_HOST)

unsigned int rt_verbose = 0;
#if (DEVELOPMENT || DEBUG)
SYSCTL_DECL(_net_route);
SYSCTL_UINT(_net_route, OID_AUTO, verbose, CTLFLAG_RW | CTLFLAG_LOCKED,
    &rt_verbose, 0, "");
#endif                             

static void
rtable_init(void **table)
{
	struct domain *dom;

	domain_proto_mtx_lock_assert_held();

	TAILQ_FOREACH(dom, &domains, dom_entry) {
		if (dom->dom_rtattach != NULL) {
			dom->dom_rtattach(&table[dom->dom_family],
			    dom->dom_rtoffset);
		}
	}
}

  
                           
   
void
route_init(void)
{
	int size;

	_CASSERT(offsetof(struct route, ro_rt) ==
	    offsetof(struct route_in6, ro_rt));
	_CASSERT(offsetof(struct route, ro_lle) ==
	    offsetof(struct route_in6, ro_lle));
	_CASSERT(offsetof(struct route, ro_srcia) ==
	    offsetof(struct route_in6, ro_srcia));
	_CASSERT(offsetof(struct route, ro_flags) ==
	    offsetof(struct route_in6, ro_flags));
	_CASSERT(offsetof(struct route, ro_dst) ==
	    offsetof(struct route_in6, ro_dst));

	PE_parse_boot_argn("rte_debug", &rte_debug, sizeof(rte_debug));
	if (rte_debug != 0) {
		rte_debug |= RTD_DEBUG;
	}

	rnh_lock_grp_attr = lck_grp_attr_alloc_init();
	rnh_lock_grp = lck_grp_alloc_init("route", rnh_lock_grp_attr);
	rnh_lock_attr = lck_attr_alloc_init();
	lck_mtx_init(rnh_lock, rnh_lock_grp, rnh_lock_attr);

	rte_mtx_grp_attr = lck_grp_attr_alloc_init();
	rte_mtx_grp = lck_grp_alloc_init(RTE_NAME, rte_mtx_grp_attr);
	rte_mtx_attr = lck_attr_alloc_init();

	lck_mtx_lock(rnh_lock);
	rn_init();                                                       
	lck_mtx_unlock(rnh_lock);
	rtable_init((void **)rt_tables);

	if (rte_debug & RTD_DEBUG) {
		size = sizeof(struct rtentry_dbg);
	} else {
		size = sizeof(struct rtentry);
	}

	rte_zone = zone_create(RTE_ZONE_NAME, size, ZC_NOENCRYPT);

	TAILQ_INIT(&rttrash_head);
}

  
                                                                       
                                                                    
                                                               
   
boolean_t
rt_primary_default(struct rtentry *rt, struct sockaddr *dst)
{
	return SA_DEFAULT(dst) && !(rt->rt_flags & RTF_IFSCOPE);
}

  
                                                                   
   
void
set_primary_ifscope(int af, unsigned int ifscope)
{
	if (af == AF_INET) {
		primary_ifscope = ifscope;
	} else {
		primary6_ifscope = ifscope;
	}
}

  
                                                                      
   
unsigned int
get_primary_ifscope(int af)
{
	return af == AF_INET ? primary_ifscope : primary6_ifscope;
}

  
                                             
   
void
sin_set_ifscope(struct sockaddr *sa, unsigned int ifscope)
{
	                                     
	ASSERT_SINIFSCOPE(sa);

	SINIFSCOPE(sa)->sin_scope_id = ifscope;
}

  
                                            
   
static inline void
sin6_set_ifscope(struct sockaddr *sa, unsigned int ifscope)
{
	                                      
	ASSERT_SIN6IFSCOPE(sa);

	SIN6IFSCOPE(sa)->sin6_scope_id = ifscope;
}

  
                                                          
   
unsigned int
sin_get_ifscope(struct sockaddr *sa)
{
	                                     
	ASSERT_SINIFSCOPE(sa);

	return SINIFSCOPE(sa)->sin_scope_id;
}

  
                                                           
   
unsigned int
sin6_get_ifscope(struct sockaddr *sa)
{
	                                      
	ASSERT_SIN6IFSCOPE(sa);

	return SIN6IFSCOPE(sa)->sin6_scope_id;
}

static inline void
sin6_set_embedded_ifscope(struct sockaddr *sa, unsigned int ifscope)
{
	                                      
	ASSERT_SIN6IFSCOPE(sa);
	VERIFY(IN6_IS_SCOPE_EMBED(&(SIN6(sa)->sin6_addr)));

	SIN6(sa)->sin6_addr.s6_addr16[1] = htons(ifscope);
}

static inline unsigned int
sin6_get_embedded_ifscope(struct sockaddr *sa)
{
	                                      
	ASSERT_SIN6IFSCOPE(sa);

	return ntohs(SIN6(sa)->sin6_addr.s6_addr16[1]);
}

  
                                                                           
  
                                                                             
                                                                          
                                                                              
                                                                          
                               
   
struct sockaddr *
sa_copy(struct sockaddr *src, struct sockaddr_storage *dst,
    unsigned int *pifscope)
{
	int af = src->sa_family;
	unsigned int ifscope = (pifscope != NULL) ? *pifscope : IFSCOPE_NONE;

	VERIFY(af == AF_INET || af == AF_INET6);

	bzero(dst, sizeof(*dst));

	if (af == AF_INET) {
		bcopy(src, dst, sizeof(struct sockaddr_in));
		dst->ss_len = sizeof(struct sockaddr_in);
		if (pifscope == NULL || ifscope != IFSCOPE_NONE) {
			sin_set_ifscope(SA(dst), ifscope);
		}
	} else {
		bcopy(src, dst, sizeof(struct sockaddr_in6));
		dst->ss_len = sizeof(struct sockaddr_in6);
		if (pifscope != NULL &&
		    IN6_IS_SCOPE_EMBED(&SIN6(dst)->sin6_addr)) {
			unsigned int eifscope;
			  
                                                    
                                                     
                                                  
                                  
      
			eifscope = sin6_get_embedded_ifscope(SA(dst));
			if (eifscope != IFSCOPE_NONE && ifscope == IFSCOPE_NONE) {
				ifscope = eifscope;
			}
			if (ifscope != IFSCOPE_NONE) {
				                                           
				sin6_set_ifscope(SA(dst), ifscope);
			} else {
				                                                
				ifscope = sin6_get_ifscope(SA(dst));
			}
			  
                                                     
                                                    
      
			if (ifscope != IFSCOPE_NONE && eifscope != ifscope) {
				sin6_set_embedded_ifscope(SA(dst), ifscope);
			}
		} else if (pifscope == NULL || ifscope != IFSCOPE_NONE) {
			sin6_set_ifscope(SA(dst), ifscope);
		}
	}

	if (pifscope != NULL) {
		*pifscope = (af == AF_INET) ? sin_get_ifscope(SA(dst)) :
		    sin6_get_ifscope(SA(dst));
	}

	return SA(dst);
}

  
                                                                   
   
static struct sockaddr *
ma_copy(int af, struct sockaddr *src, struct sockaddr_storage *dst,
    unsigned int ifscope)
{
	VERIFY(af == AF_INET || af == AF_INET6);

	bzero(dst, sizeof(*dst));
	rt_maskedcopy(src, SA(dst), src);

	  
                                                             
                                                                  
                                                               
                                                            
                                                            
                                                           
                                                      
    
	if (af == AF_INET) {
		SINIFSCOPE(dst)->sin_scope_id = ifscope;
		SINIFSCOPE(dst)->sin_len =
		    offsetof(struct sockaddr_inifscope, sin_scope_id) +
		    sizeof(SINIFSCOPE(dst)->sin_scope_id);
	} else {
		SIN6IFSCOPE(dst)->sin6_scope_id = ifscope;
		SIN6IFSCOPE(dst)->sin6_len =
		    offsetof(struct sockaddr_in6, sin6_scope_id) +
		    sizeof(SIN6IFSCOPE(dst)->sin6_scope_id);
	}

	return SA(dst);
}

  
                                                            
   
static struct sockaddr *
sa_trim(struct sockaddr *sa, int skip)
{
	caddr_t cp, base = (caddr_t)sa + skip;

	if (sa->sa_len <= skip) {
		return sa;
	}

	for (cp = base + (sa->sa_len - skip); cp > base && cp[-1] == 0;) {
		cp--;
	}

	sa->sa_len = (cp - base) + skip;
	if (sa->sa_len < skip) {
		                                       
		panic("%s: broken logic (sa_len %d < skip %d )", __func__,
		    sa->sa_len, skip);
		                
	} else if (sa->sa_len == skip) {
		                                                        
		sa->sa_len = 0;
	}

	return sa;
}

  
                                                                          
                                                                         
                                                                            
                                                                          
                                                                          
                              
  
                     
                                                                            
                                     
                                                                         
                                                                         
   
struct sockaddr *
rtm_scrub(int type, int idx, struct sockaddr *hint, struct sockaddr *sa,
    void *buf, uint32_t buflen, kauth_cred_t *credp)
{
	struct sockaddr_storage *ss = (struct sockaddr_storage *)buf;
	struct sockaddr *ret = sa;

	VERIFY(buf != NULL && buflen >= sizeof(*ss));
	bzero(buf, buflen);

	switch (idx) {
	case RTAX_DST:
		  
                                                            
                                                
     
		if (sa->sa_family == AF_INET &&
		    SINIFSCOPE(sa)->sin_scope_id != IFSCOPE_NONE) {
			ret = sa_copy(sa, ss, NULL);
		} else if (sa->sa_family == AF_INET6 &&
		    SIN6IFSCOPE(sa)->sin6_scope_id != IFSCOPE_NONE) {
			ret = sa_copy(sa, ss, NULL);
		}
		break;

	case RTAX_NETMASK: {
		int skip, af;
		  
                                                              
                                                             
                                                                
                                                           
                                                              
                                                              
                                                             
                                                                
                                                               
                                                             
                                                              
                                                               
     
		if (hint == NULL ||
		    ((af = hint->sa_family) != AF_INET && af != AF_INET6)) {
			break;                     
		}
		skip = (af == AF_INET) ?
		    offsetof(struct sockaddr_in, sin_addr) :
		    offsetof(struct sockaddr_in6, sin6_addr);

		if (sa->sa_len > skip && sa->sa_len <= sizeof(*ss)) {
			bcopy(sa, ss, sa->sa_len);
			  
                                                     
                                                    
                                                    
      
			if (hint->sa_family == AF_INET) {
				SINIFSCOPE(ss)->sin_scope_id = IFSCOPE_NONE;
			} else {
				SIN6IFSCOPE(ss)->sin6_scope_id = IFSCOPE_NONE;
			}
			ret = sa_trim(SA(ss), skip);

			  
                                                        
                                                       
                                   
      
			if (hint->sa_family == AF_INET6 &&
			    type != RTM_GET && type != RTM_GET2) {
				SA(ret)->sa_len = sizeof(struct sockaddr_in6);
			}
		}
		break;
	}
	case RTAX_GATEWAY: {
		  
                                                               
    
                                                              
                                                    
     
		if ((sa->sa_family != AF_LINK) || (SDL(sa)->sdl_alen == 0)) {
			break;
		}
		OS_FALLTHROUGH;
	}

	case RTAX_IFP: {
		if (sa->sa_family == AF_LINK && credp) {
			struct sockaddr_dl *sdl = SDL(buf);
			const void *bytes;
			size_t size;

			                                                      
			VERIFY(buflen >= sa->sa_len);

			bcopy(sa, sdl, sa->sa_len);
			bytes = dlil_ifaddr_bytes(sdl, &size, credp);
			if (bytes != CONST_LLADDR(sdl)) {
				VERIFY(sdl->sdl_alen == size);
				bcopy(bytes, LLADDR(sdl), size);
			}
			ret = (struct sockaddr *)sdl;
		}
		break;
	}
	default:
		break;
	}

	return ret;
}

  
                                                            
                                                          
   
static int
rn_match_ifscope(struct radix_node *rn, void *arg)
{
	struct rtentry *rt = (struct rtentry *)rn;
	struct matchleaf_arg *ma = arg;
	int af = rt_key(rt)->sa_family;

	if (!(rt->rt_flags & RTF_IFSCOPE) || (af != AF_INET && af != AF_INET6)) {
		return 0;
	}

	return af == AF_INET ?
	       (SINIFSCOPE(rt_key(rt))->sin_scope_id == ma->ifscope) :
	       (SIN6IFSCOPE(rt_key(rt))->sin6_scope_id == ma->ifscope);
}

  
                                                
   
void
routegenid_update(void)
{
	routegenid_inet_update();
	routegenid_inet6_update();
}

void
routegenid_inet_update(void)
{
	atomic_add_32(&route_genid_inet, 1);
}

void
routegenid_inet6_update(void)
{
	atomic_add_32(&route_genid_inet6, 1);
}

  
                           
   
void
rtalloc(struct route *ro)
{
	rtalloc_ign(ro, 0);
}

void
rtalloc_scoped(struct route *ro, unsigned int ifscope)
{
	rtalloc_scoped_ign(ro, 0, ifscope);
}

static void
rtalloc_ign_common_locked(struct route *ro, uint32_t ignore,
    unsigned int ifscope)
{
	struct rtentry *rt;

	if ((rt = ro->ro_rt) != NULL) {
		RT_LOCK_SPIN(rt);
		if (rt->rt_ifp != NULL && !ROUTE_UNUSABLE(ro)) {
			RT_UNLOCK(rt);
			return;
		}
		RT_UNLOCK(rt);
		ROUTE_RELEASE_LOCKED(ro);                                  
	}
	ro->ro_rt = rtalloc1_common_locked(&ro->ro_dst, 1, ignore, ifscope);
	if (ro->ro_rt != NULL) {
		RT_GENID_SYNC(ro->ro_rt);
		RT_LOCK_ASSERT_NOTHELD(ro->ro_rt);
	}
}

void
rtalloc_ign(struct route *ro, uint32_t ignore)
{
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(rnh_lock);
	rtalloc_ign_common_locked(ro, ignore, IFSCOPE_NONE);
	lck_mtx_unlock(rnh_lock);
}

void
rtalloc_scoped_ign(struct route *ro, uint32_t ignore, unsigned int ifscope)
{
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(rnh_lock);
	rtalloc_ign_common_locked(ro, ignore, ifscope);
	lck_mtx_unlock(rnh_lock);
}

static struct rtentry *
rtalloc1_locked(struct sockaddr *dst, int report, uint32_t ignflags)
{
	return rtalloc1_common_locked(dst, report, ignflags, IFSCOPE_NONE);
}

struct rtentry *
rtalloc1_scoped_locked(struct sockaddr *dst, int report, uint32_t ignflags,
    unsigned int ifscope)
{
	return rtalloc1_common_locked(dst, report, ignflags, ifscope);
}

static boolean_t
route_ignore_protocol_cloning_for_dst(struct rtentry *rt, struct sockaddr *dst)
{
	  
                                                      
                
    
	if (dst->sa_family != AF_INET6) {
		return FALSE;
	}

	  
                                                          
                       
                                                               
                                                          
                                                          
    
	if (!IFNET_IS_COMPANION_LINK(rt->rt_ifp) &&
	    (rt->rt_flags & RTF_GATEWAY) &&
	    (rt->rt_flags & RTF_PRCLONING) &&
	    SA_DEFAULT(rt_key(rt)) &&
	    IN6_IS_ADDR_UNIQUE_LOCAL(&SIN6(dst)->sin6_addr)) {
		return TRUE;
	}
	return FALSE;
}

struct rtentry *
rtalloc1_common_locked(struct sockaddr *dst, int report, uint32_t ignflags,
    unsigned int ifscope)
{
	struct radix_node_head *rnh = rt_tables[dst->sa_family];
	struct rtentry *rt, *newrt = NULL;
	struct rt_addrinfo info;
	uint32_t nflags;
	int  err = 0, msgtype = RTM_MISS;

	if (rnh == NULL) {
		goto unreachable;
	}

	  
                                                                        
                                                                     
    
	rt = rt_lookup(FALSE, dst, NULL, rnh, ifscope);
	if (rt == NULL) {
		goto unreachable;
	}

	  
                                                                
                                                                      
                                                 
                                                                          
          
                                                                     
                                                                       
                                                                     
                                                        
   
                                                                      
                                                                        
    
	if (route_ignore_protocol_cloning_for_dst(rt, dst)) {
		ignflags |= RTF_PRCLONING;
	}

	RT_LOCK_SPIN(rt);
	newrt = rt;
	nflags = rt->rt_flags & ~ignflags;
	RT_UNLOCK(rt);

	if (report && (nflags & (RTF_CLONING | RTF_PRCLONING))) {
		  
                                                     
                                             
                                           
     
		err = rtrequest_locked(RTM_RESOLVE, dst, NULL, NULL, 0, &newrt);
		if (err) {
			  
                                                  
                                                   
                                                  
                   
      
			newrt = rt;
			goto miss;
		}

		  
                                                               
                                                              
                                     
     
		rtfree_locked(rt);

		  
                                                             
                                                 
                                                             
                     
    
                                                                
                                          
                                                                  
                      
     
		if ((newrt->rt_flags & (RTF_HOST | RTF_LLINFO)) ==
		    (RTF_HOST | RTF_LLINFO)) {
			struct rtentry *defrt = NULL;
			struct sockaddr_storage def_key;

			bzero(&def_key, sizeof(def_key));
			def_key.ss_len = rt_key(newrt)->sa_len;
			def_key.ss_family = rt_key(newrt)->sa_family;

			defrt = rtalloc1_scoped_locked((struct sockaddr *)&def_key,
			    0, 0, newrt->rt_ifp->if_index);

			if (defrt) {
				if (equal(rt_key(newrt), defrt->rt_gateway)) {
					newrt->rt_flags |= RTF_ROUTER;
				}
				rtfree_locked(defrt);
			}
		}

		if ((rt = newrt) && (rt->rt_flags & RTF_XRESOLVE)) {
			  
                                      
                                           
      
			msgtype = RTM_RESOLVE;
			goto miss;
		}
	}
	goto done;

unreachable:
	  
                                                      
                                                    
    
	rtstat.rts_unreach++;

miss:
	if (report) {
		  
                                                       
                 
                                                      
     
		bzero((caddr_t)&info, sizeof(info));
		info.rti_info[RTAX_DST] = dst;
		rt_missmsg(msgtype, &info, 0, err);
	}
done:
	return newrt;
}

struct rtentry *
rtalloc1(struct sockaddr *dst, int report, uint32_t ignflags)
{
	struct rtentry *entry;
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(rnh_lock);
	entry = rtalloc1_locked(dst, report, ignflags);
	lck_mtx_unlock(rnh_lock);
	return entry;
}

struct rtentry *
rtalloc1_scoped(struct sockaddr *dst, int report, uint32_t ignflags,
    unsigned int ifscope)
{
	struct rtentry *entry;
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(rnh_lock);
	entry = rtalloc1_scoped_locked(dst, report, ignflags, ifscope);
	lck_mtx_unlock(rnh_lock);
	return entry;
}

  
                                            
                                                                 
   
void
rtfree_locked(struct rtentry *rt)
{
	rtfree_common(rt, TRUE);
}

static void
rtfree_common(struct rtentry *rt, boolean_t locked)
{
	struct radix_node_head *rnh;

	LCK_MTX_ASSERT(rnh_lock, locked ?
	    LCK_MTX_ASSERT_OWNED : LCK_MTX_ASSERT_NOTOWNED);

	  
                                                                 
                                                                   
    
	RT_LOCK_SPIN(rt);
	if (rtunref(rt) > 0) {
		RT_UNLOCK(rt);
		return;
	}

	  
                                                                 
                                                                 
                                                                   
                         
    
	if (!locked) {
		  
                                                               
                                                                 
                                                               
                                                                
                                                                
                                                               
                                             
     
		RT_ADDREF_LOCKED(rt);
		RT_UNLOCK(rt);
		lck_mtx_lock(rnh_lock);
		RT_LOCK_SPIN(rt);
		if (rtunref(rt) > 0) {
			                                   
			RT_UNLOCK(rt);
			goto done;
		}
	}

	  
                                                         
                                                        
    
	RT_CONVERT_LOCK(rt);

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);

	                                       
	if (rt->rt_refcnt != 0) {
		panic("rt %p invalid refcnt %d", rt, rt->rt_refcnt);
		                
	}
	                                                         
	VERIFY(!(rt->rt_flags & RTF_IFREF));

	  
                                         
                                                                
    
	rnh = rt_tables[rt_key(rt)->sa_family];

	  
                                                                 
                                                                  
                                                                     
                                                                 
                                                                     
                                                                  
    
	if (rnh != NULL && rnh->rnh_close != NULL) {
		rnh->rnh_close((struct radix_node *)rt, rnh);
	}

	  
                                                                
                                        
    
	if (!(rt->rt_flags & RTF_UP)) {
		struct rtentry *rt_parent;
		struct ifaddr *rt_ifa;

		rt->rt_flags |= RTF_DEAD;
		if (rt->rt_nodes->rn_flags & (RNF_ACTIVE | RNF_ROOT)) {
			panic("rt %p freed while in radix tree\n", rt);
			                
		}
		  
                                                              
                                                      
     
		(void) OSDecrementAtomic(&rttrash);
		if (rte_debug & RTD_DEBUG) {
			TAILQ_REMOVE(&rttrash_head, (struct rtentry_dbg *)rt,
			    rtd_trash_link);
		}

		  
                                                  
                                  
     
		if ((rt_parent = rt->rt_parent) != NULL) {
			rt->rt_parent = NULL;
		}

		if ((rt_ifa = rt->rt_ifa) != NULL) {
			rt->rt_ifa = NULL;
		}

		  
                                           
     
		if (rt->rt_llinfo != NULL) {
			if (rt->rt_llinfo_free != NULL) {
				(*rt->rt_llinfo_free)(rt->rt_llinfo);
			} else {
				R_Free(rt->rt_llinfo);
			}
			rt->rt_llinfo = NULL;
		}

		                                        
		eventhandler_lists_ctxt_destroy(&rt->rt_evhdlr_ctxt);

		  
                                                    
                                             
     
		RT_UNLOCK(rt);
		rte_lock_destroy(rt);

		if (rt_parent != NULL) {
			rtfree_locked(rt_parent);
		}

		if (rt_ifa != NULL) {
			IFA_REMREF(rt_ifa);
		}

		  
                                                                 
                                                             
              
     
		R_Free(rt_key(rt));

		  
                                                     
     
		nstat_route_detach(rt);

		  
                                     
     
		rte_free(rt);
	} else {
		  
                                                         
                                                        
                             
     
		RT_UNLOCK(rt);
	}
done:
	if (!locked) {
		lck_mtx_unlock(rnh_lock);
	}
}

void
rtfree(struct rtentry *rt)
{
	rtfree_common(rt, FALSE);
}

  
                                                           
                                                                 
                          
   
int
rtunref(struct rtentry *p)
{
	RT_LOCK_ASSERT_HELD(p);

	if (p->rt_refcnt == 0) {
		panic("%s(%p) bad refcnt\n", __func__, p);
		                
	} else if (--p->rt_refcnt == 0) {
		  
                                                            
                                                              
                                                              
                                                        
     
		rt_clear_idleref(p);
	}

	if (rte_debug & RTD_DEBUG) {
		rtunref_audit((struct rtentry_dbg *)p);
	}

	                      
	return p->rt_refcnt;
}

static inline void
rtunref_audit(struct rtentry_dbg *rte)
{
	uint16_t idx;

	if (rte->rtd_inuse != RTD_INUSE) {
		panic("rtunref: on freed rte=%p\n", rte);
		                
	}
	idx = atomic_add_16_ov(&rte->rtd_refrele_cnt, 1) % CTRACE_HIST_SIZE;
	if (rte_debug & RTD_TRACE) {
		ctrace_record(&rte->rtd_refrele[idx]);
	}
}

  
                                         
   
void
rtref(struct rtentry *p)
{
	RT_LOCK_ASSERT_HELD(p);

	VERIFY((p->rt_flags & RTF_DEAD) == 0);
	if (++p->rt_refcnt == 0) {
		panic("%s(%p) bad refcnt\n", __func__, p);
		                
	} else if (p->rt_refcnt == 1) {
		  
                                                   
                                     
     
		rt_set_idleref(p);
	}

	if (rte_debug & RTD_DEBUG) {
		rtref_audit((struct rtentry_dbg *)p);
	}
}

static inline void
rtref_audit(struct rtentry_dbg *rte)
{
	uint16_t idx;

	if (rte->rtd_inuse != RTD_INUSE) {
		panic("rtref_audit: on freed rte=%p\n", rte);
		                
	}
	idx = atomic_add_16_ov(&rte->rtd_refhold_cnt, 1) % CTRACE_HIST_SIZE;
	if (rte_debug & RTD_TRACE) {
		ctrace_record(&rte->rtd_refhold[idx]);
	}
}

void
rtsetifa(struct rtentry *rt, struct ifaddr *ifa)
{
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);

	RT_LOCK_ASSERT_HELD(rt);

	if (rt->rt_ifa == ifa) {
		return;
	}

	                                          
	RT_CONVERT_LOCK(rt);

	                         
	if (rt->rt_ifa) {
		IFA_REMREF(rt->rt_ifa);
	}

	                
	rt->rt_ifa = ifa;

	                                 
	if (rt->rt_ifa) {
		IFA_ADDREF(rt->rt_ifa);
	}
}

  
                                               
                                               
                                                    
                                  
   
void
rtredirect(struct ifnet *ifp, struct sockaddr *dst, struct sockaddr *gateway,
    struct sockaddr *netmask, int flags, struct sockaddr *src,
    struct rtentry **rtp)
{
	struct rtentry *rt = NULL;
	int error = 0;
	short *stat = 0;
	struct rt_addrinfo info;
	struct ifaddr *ifa = NULL;
	unsigned int ifscope = (ifp != NULL) ? ifp->if_index : IFSCOPE_NONE;
	struct sockaddr_storage ss;
	int af = src->sa_family;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(rnh_lock);

	  
                                                          
                                        
    
	if ((af == AF_INET) || (af == AF_INET6)) {
		src = sa_copy(src, &ss, &ifscope);
	}

	  
                                                               
                                                              
                                       
    
	if ((ifa = ifa_ifwithnet_scoped(gateway, ifscope)) == NULL) {
		error = ENETUNREACH;
		goto out;
	}

	                                                                   
	rt = rtalloc1_scoped_locked(dst, 0, RTF_CLONING | RTF_PRCLONING, ifscope);
	if (rt != NULL) {
		RT_LOCK(rt);
	}

	  
                                                               
                                                               
                                                               
                                                            
                                                            
                                                           
    
	if (!(flags & RTF_DONE) && rt != NULL &&
	    (!equal(src, rt->rt_gateway) || !equal(rt->rt_ifa->ifa_addr,
	    ifa->ifa_addr))) {
		error = EINVAL;
	} else {
		IFA_REMREF(ifa);
		if ((ifa = ifa_ifwithaddr(gateway))) {
			IFA_REMREF(ifa);
			ifa = NULL;
			error = EHOSTUNREACH;
		}
	}

	if (ifa) {
		IFA_REMREF(ifa);
		ifa = NULL;
	}

	if (error) {
		if (rt != NULL) {
			RT_UNLOCK(rt);
		}
		goto done;
	}

	  
                                                           
                                                          
                                                           
                                            
    
	if ((rt == NULL) || (rt_mask(rt) != NULL && rt_mask(rt)->sa_len < 2)) {
		goto create;
	}
	  
                                        
                                
    
	RT_LOCK_ASSERT_HELD(rt);
	if (rt->rt_flags & RTF_GATEWAY) {
		if (((rt->rt_flags & RTF_HOST) == 0) && (flags & RTF_HOST)) {
			  
                                                  
                                                  
                                                 
                                           
      
create:
			if (rt != NULL) {
				RT_UNLOCK(rt);
			}
			flags |=  RTF_GATEWAY | RTF_DYNAMIC;
			error = rtrequest_scoped_locked(RTM_ADD, dst,
			    gateway, netmask, flags, NULL, ifscope);
			stat = &rtstat.rts_dynamic;
		} else {
			  
                                                
                                                      
      
			rt->rt_flags |= RTF_MODIFIED;
			flags |= RTF_MODIFIED;
			stat = &rtstat.rts_newgateway;
			  
                                                      
      
			error = rt_setgate(rt, rt_key(rt), gateway);
			RT_UNLOCK(rt);
		}
	} else {
		RT_UNLOCK(rt);
		error = EHOSTUNREACH;
	}
done:
	if (rt != NULL) {
		RT_LOCK_ASSERT_NOTHELD(rt);
		if (!error) {
			                                                 
			route_event_enqueue_nwk_wq_entry(rt, NULL, ROUTE_ENTRY_REFRESH, NULL, FALSE);
			if (rtp) {
				*rtp = rt;
			} else {
				rtfree_locked(rt);
			}
		} else {
			rtfree_locked(rt);
		}
	}
out:
	if (error) {
		rtstat.rts_badredirect++;
	} else {
		if (stat != NULL) {
			(*stat)++;
		}

		if (af == AF_INET) {
			routegenid_inet_update();
		} else if (af == AF_INET6) {
			routegenid_inet6_update();
		}
	}
	lck_mtx_unlock(rnh_lock);
	bzero((caddr_t)&info, sizeof(info));
	info.rti_info[RTAX_DST] = dst;
	info.rti_info[RTAX_GATEWAY] = gateway;
	info.rti_info[RTAX_NETMASK] = netmask;
	info.rti_info[RTAX_AUTHOR] = src;
	rt_missmsg(RTM_REDIRECT, &info, flags, error);
}

  
                                 
   
int
rtioctl(unsigned long req, caddr_t data, struct proc *p)
{
#pragma unused(p, req, data)
	return ENXIO;
}

struct ifaddr *
ifa_ifwithroute(
	int flags,
	const struct sockaddr   *dst,
	const struct sockaddr *gateway)
{
	struct ifaddr *ifa;

	lck_mtx_lock(rnh_lock);
	ifa = ifa_ifwithroute_locked(flags, dst, gateway);
	lck_mtx_unlock(rnh_lock);

	return ifa;
}

struct ifaddr *
ifa_ifwithroute_locked(int flags, const struct sockaddr *dst,
    const struct sockaddr *gateway)
{
	return ifa_ifwithroute_common_locked((flags & ~RTF_IFSCOPE), dst,
	           gateway, IFSCOPE_NONE);
}

struct ifaddr *
ifa_ifwithroute_scoped_locked(int flags, const struct sockaddr *dst,
    const struct sockaddr *gateway, unsigned int ifscope)
{
	if (ifscope != IFSCOPE_NONE) {
		flags |= RTF_IFSCOPE;
	} else {
		flags &= ~RTF_IFSCOPE;
	}

	return ifa_ifwithroute_common_locked(flags, dst, gateway, ifscope);
}

static struct ifaddr *
ifa_ifwithroute_common_locked(int flags, const struct sockaddr *dst,
    const struct sockaddr *gw, unsigned int ifscope)
{
	struct ifaddr *ifa = NULL;
	struct rtentry *rt = NULL;
	struct sockaddr_storage dst_ss, gw_ss;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);

	  
                                                     
                                                    
                                      
    
	if (dst != NULL &&
	    ((dst->sa_family == AF_INET) ||
	    (dst->sa_family == AF_INET6))) {
		dst = sa_copy(SA((uintptr_t)dst), &dst_ss, NULL);
	}

	if (gw != NULL &&
	    ((gw->sa_family == AF_INET) ||
	    (gw->sa_family == AF_INET6))) {
		gw = sa_copy(SA((uintptr_t)gw), &gw_ss, NULL);
	}

	if (!(flags & RTF_GATEWAY)) {
		  
                                              
                                         
                                         
                                             
                                  
     
		if (flags & RTF_HOST) {
			ifa = ifa_ifwithdstaddr(dst);
		}
		if (ifa == NULL) {
			ifa = ifa_ifwithaddr_scoped(gw, ifscope);
		}
	} else {
		  
                                             
                                             
                                  
     
		ifa = ifa_ifwithdstaddr(gw);
	}
	if (ifa == NULL) {
		ifa = ifa_ifwithnet_scoped(gw, ifscope);
	}
	if (ifa == NULL) {
		                                                              
		rt = rtalloc1_scoped_locked((struct sockaddr *)(size_t)dst,
		    0, 0, ifscope);
		if (rt != NULL) {
			RT_LOCK_SPIN(rt);
			ifa = rt->rt_ifa;
			if (ifa != NULL) {
				                            
				RT_CONVERT_LOCK(rt);
				IFA_ADDREF(ifa);
			}
			RT_REMREF_LOCKED(rt);
			RT_UNLOCK(rt);
			rt = NULL;
		}
	}
	  
                                                              
                                                          
                                                   
    
	if (ifa != NULL && ifa->ifa_addr->sa_family != dst->sa_family) {
		struct ifaddr *newifa;
		                                                  
		newifa = ifaof_ifpforaddr(dst, ifa->ifa_ifp);
		if (newifa != NULL) {
			IFA_REMREF(ifa);
			ifa = newifa;
		}
	}
	  
                                                             
                                                              
                                                              
                                                           
                                                            
                                                            
    
	if ((ifa == NULL ||
	    !equal(ifa->ifa_addr, (struct sockaddr *)(size_t)gw)) &&
	    (rt = rtalloc1_scoped_locked((struct sockaddr *)(size_t)gw,
	    0, 0, ifscope)) != NULL) {
		if (ifa != NULL) {
			IFA_REMREF(ifa);
		}
		RT_LOCK_SPIN(rt);
		ifa = rt->rt_ifa;
		if (ifa != NULL) {
			                            
			RT_CONVERT_LOCK(rt);
			IFA_ADDREF(ifa);
		}
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
	}
	  
                                                               
                                                             
                                
    
	if ((flags & RTF_IFSCOPE) &&
	    ifa != NULL && ifa->ifa_ifp->if_index != ifscope) {
		IFA_REMREF(ifa);
		ifa = NULL;
	}

	  
                                                                
                               
    
	if (ifa != NULL &&
	    ifa->ifa_addr->sa_family != dst->sa_family) {
		IFA_REMREF(ifa);
		ifa = NULL;
	}

	return ifa;
}

static int rt_fixdelete(struct radix_node *, void *);
static int rt_fixchange(struct radix_node *, void *);

struct rtfc_arg {
	struct rtentry *rt0;
	struct radix_node_head *rnh;
};

int
rtrequest_locked(int req, struct sockaddr *dst, struct sockaddr *gateway,
    struct sockaddr *netmask, int flags, struct rtentry **ret_nrt)
{
	return rtrequest_common_locked(req, dst, gateway, netmask,
	           (flags & ~RTF_IFSCOPE), ret_nrt, IFSCOPE_NONE);
}

int
rtrequest_scoped_locked(int req, struct sockaddr *dst,
    struct sockaddr *gateway, struct sockaddr *netmask, int flags,
    struct rtentry **ret_nrt, unsigned int ifscope)
{
	if (ifscope != IFSCOPE_NONE) {
		flags |= RTF_IFSCOPE;
	} else {
		flags &= ~RTF_IFSCOPE;
	}

	return rtrequest_common_locked(req, dst, gateway, netmask,
	           flags, ret_nrt, ifscope);
}

  
                                                                       
               
  
                                                                          
                                                                           
                                                                         
                                                                         
                                                                        
                                                                      
                                                                       
                                                                         
                                                                         
                                                                          
   
static int
rtrequest_common_locked(int req, struct sockaddr *dst0,
    struct sockaddr *gateway, struct sockaddr *netmask, int flags,
    struct rtentry **ret_nrt, unsigned int ifscope)
{
	int error = 0;
	struct rtentry *rt;
	struct radix_node *rn;
	struct radix_node_head *rnh;
	struct ifaddr *ifa = NULL;
	struct sockaddr *ndst, *dst = dst0;
	struct sockaddr_storage ss, mask;
	struct timeval caltime;
	int af = dst->sa_family;
	void (*ifa_rtrequest)(int, struct rtentry *, struct sockaddr *);

#define senderr(x) { error = x; goto bad; }

	DTRACE_ROUTE6(rtrequest, int, req, struct sockaddr *, dst0,
	    struct sockaddr *, gateway, struct sockaddr *, netmask,
	    int, flags, unsigned int, ifscope);

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);

#if !(DEVELOPMENT || DEBUG)
	  
                                                                 
    
	flags &= ~RTF_GLOBAL;
#endif                              

	  
                                                                
    
	if ((rnh = rt_tables[af]) == NULL) {
		senderr(ESRCH);
	}
	  
                                                           
                         
    
	if (flags & RTF_HOST) {
		netmask = NULL;
	}

	  
                                                                     
                                                                     
                                                                 
                                                                     
                                                                        
                                                                     
                                                                  
                                                                      
                                                                 
                                                                 
                           
    
	if (req != RTM_RESOLVE && ((af == AF_INET) || (af == AF_INET6))) {
		                                                        
		dst = sa_copy(dst, &ss, &ifscope);

		                                                            
		if (netmask != NULL) {
			netmask = ma_copy(af, netmask, &mask, ifscope);
		}

		if (ifscope != IFSCOPE_NONE) {
			flags |= RTF_IFSCOPE;
		}
	} else if ((flags & RTF_IFSCOPE) &&
	    (af != AF_INET && af != AF_INET6)) {
		senderr(EINVAL);
	}

	if (ifscope == IFSCOPE_NONE) {
		flags &= ~RTF_IFSCOPE;
	}

	switch (req) {
	case RTM_DELETE: {
		struct rtentry *gwrt = NULL;
		boolean_t was_router = FALSE;
		uint32_t old_rt_refcnt = 0;
		  
                                                 
                                                           
     
		if ((rn = rnh->rnh_deladdr(dst, netmask, rnh)) == NULL) {
			senderr(ESRCH);
		}
		if (rn->rn_flags & (RNF_ACTIVE | RNF_ROOT)) {
			panic("rtrequest delete");
			                
		}
		rt = (struct rtentry *)rn;

		RT_LOCK(rt);
		old_rt_refcnt = rt->rt_refcnt;
		rt->rt_flags &= ~RTF_UP;
		  
                                                           
                                                   
     
		rt_clear_idleref(rt);
		  
                                                              
                                                               
                                                               
                                                                 
                                                              
                                                              
                            
     
		RT_ADDREF_LOCKED(rt);

		  
                                                             
     
		rt->rt_flags |= RTF_CONDEMNED;

		  
                                  
     
		if (rt->rt_flags & RTF_ROUTER) {
			was_router = TRUE;
			VERIFY(rt->rt_flags & RTF_HOST);
			rt->rt_flags &= ~RTF_ROUTER;
		}

		  
                                                              
    
                                                                   
                                                               
     
		if (old_rt_refcnt != 0) {
			route_event_enqueue_nwk_wq_entry(rt, NULL,
			    ROUTE_ENTRY_DELETED, NULL, TRUE);
		}

		  
                                                         
                                                        
     
		if ((rt->rt_flags & (RTF_CLONING | RTF_PRCLONING)) &&
		    rt_mask(rt)) {
			RT_UNLOCK(rt);
			rnh->rnh_walktree_from(rnh, dst, rt_mask(rt),
			    rt_fixdelete, rt);
			RT_LOCK(rt);
		}

		if (was_router) {
			struct route_event rt_ev;
			route_event_init(&rt_ev, rt, NULL, ROUTE_LLENTRY_DELETED);
			RT_UNLOCK(rt);
			(void) rnh->rnh_walktree(rnh,
			    route_event_walktree, (void *)&rt_ev);
			RT_LOCK(rt);
		}

		  
                                                
     
		if ((gwrt = rt->rt_gwroute) != NULL) {
			rt->rt_gwroute = NULL;
		}

		  
                                                       
     
		if ((ifa = rt->rt_ifa) != NULL) {
			IFA_LOCK_SPIN(ifa);
			ifa_rtrequest = ifa->ifa_rtrequest;
			IFA_UNLOCK(ifa);
			if (ifa_rtrequest != NULL) {
				ifa_rtrequest(RTM_DELETE, rt, NULL);
			}
			                              
			ifa = NULL;
		}

		  
                                                 
                                 
     
		(void) OSIncrementAtomic(&rttrash);
		if (rte_debug & RTD_DEBUG) {
			TAILQ_INSERT_TAIL(&rttrash_head,
			    (struct rtentry_dbg *)rt, rtd_trash_link);
		}

		  
                                                     
                                                      
     
		if (rt_primary_default(rt, rt_key(rt))) {
			set_primary_ifscope(rt_key(rt)->sa_family,
			    IFSCOPE_NONE);
			if ((rt->rt_flags & RTF_STATIC) &&
			    rt_key(rt)->sa_family == PF_INET6) {
				trigger_v6_defrtr_select = TRUE;
			}
		}

#if NECP
		  
                                                   
                                        
     
		if (SA_DEFAULT(rt_key(rt))) {
			if (rt->rt_ifp != NULL) {
				ifnet_touch_lastupdown(rt->rt_ifp);
			}
			necp_update_all_clients();
		}
#endif           

		RT_UNLOCK(rt);

		  
                                                        
                                                           
                                                        
                                                  
     
		if (gwrt != NULL) {
			rtfree_locked(gwrt);
		}

		  
                                                 
                                                         
              
     
		if (ret_nrt != NULL) {
			                                                      
			*ret_nrt = rt;
		} else {
			                                         
			rtfree_locked(rt);
		}
		if (af == AF_INET) {
			routegenid_inet_update();
		} else if (af == AF_INET6) {
			routegenid_inet6_update();
		}
		break;
	}
	case RTM_RESOLVE:
		if (ret_nrt == NULL || (rt = *ret_nrt) == NULL) {
			senderr(EINVAL);
		}
		  
                                                               
                                                     
                                                         
                                               
     
		if (rt->rt_flags & RTF_REJECT) {
			if (rt->rt_flags & RTF_HOST) {
				senderr(EHOSTUNREACH);
			} else {
				senderr(ENETUNREACH);
			}
		}
		  
                                                             
                                                               
                                                              
                                                            
                                                               
                                                            
                                                              
               
     
		ifa = rt->rt_ifa;
		IFA_ADDREF(ifa);
		flags = rt->rt_flags &
		    ~(RTF_CLONING | RTF_PRCLONING | RTF_STATIC);
		flags |= RTF_WASCLONED;
		gateway = rt->rt_gateway;
		if ((netmask = rt->rt_genmask) == NULL) {
			flags |= RTF_HOST;
		}

		if (af != AF_INET && af != AF_INET6) {
			goto makeroute;
		}

		  
                                                       
                                                        
                                                      
                                                          
                                                       
                                     
     
		if ((af == AF_INET &&
		    IN_LINKLOCAL(ntohl(SIN(dst)->sin_addr.s_addr))) ||
		    (rt->rt_flags & RTF_PROXY)) {
			ifscope = IFSCOPE_NONE;
			flags &= ~RTF_IFSCOPE;
			  
                                                   
                                                     
      
			flags |= RTF_NOIFREF;
		} else {
			if (flags & RTF_IFSCOPE) {
				ifscope = (af == AF_INET) ?
				    sin_get_ifscope(rt_key(rt)) :
				    sin6_get_ifscope(rt_key(rt));
			} else {
				ifscope = rt->rt_ifp->if_index;
				flags |= RTF_IFSCOPE;
			}
			VERIFY(ifscope != IFSCOPE_NONE);
		}

		  
                                                        
                                                          
     
		dst = sa_copy(dst, &ss, (ifscope == IFSCOPE_NONE) ?
		    NULL : &ifscope);

		                                                            
		if (netmask != NULL) {
			netmask = ma_copy(af, netmask, &mask, ifscope);
		}

		goto makeroute;

	case RTM_ADD:
		if ((flags & RTF_GATEWAY) && !gateway) {
			panic("rtrequest: RTF_GATEWAY but no gateway");
			                
		}
		if (flags & RTF_IFSCOPE) {
			ifa = ifa_ifwithroute_scoped_locked(flags, dst0,
			    gateway, ifscope);
		} else {
			ifa = ifa_ifwithroute_locked(flags, dst0, gateway);
		}
		if (ifa == NULL) {
			senderr(ENETUNREACH);
		}
makeroute:
		  
                                                     
                                      
     
		if ((rt = rte_alloc()) == NULL) {
			senderr(ENOBUFS);
		}
		Bzero(rt, sizeof(*rt));
		rte_lock_init(rt);
		eventhandler_lists_ctxt_init(&rt->rt_evhdlr_ctxt);
		getmicrotime(&caltime);
		rt->base_calendartime = caltime.tv_sec;
		rt->base_uptime = net_uptime();
		RT_LOCK(rt);
		rt->rt_flags = RTF_UP | flags;

		  
                                           
     
		switch (af) {
		case AF_INET:
			rt->rt_tree_genid = &route_genid_inet;
			break;
		case AF_INET6:
			rt->rt_tree_genid = &route_genid_inet6;
			break;
		default:
			break;
		}

		  
                                                               
                                         
     
		if ((error = rt_setgate(rt, dst, gateway)) != 0) {
			int tmp = error;
			RT_UNLOCK(rt);
			nstat_route_detach(rt);
			rte_lock_destroy(rt);
			rte_free(rt);
			senderr(tmp);
		}

		  
                                                         
     
		ndst = rt_key(rt);

		  
                                                                
     
		if (netmask) {
			rt_maskedcopy(dst, ndst, netmask);
		} else {
			Bcopy(dst, ndst, dst->sa_len);
		}

		  
                                                  
                                                         
                                                        
     
		rtsetifa(rt, ifa);
		rt->rt_ifp = rt->rt_ifa->ifa_ifp;

		                                                                

		rn = rnh->rnh_addaddr((caddr_t)ndst, (caddr_t)netmask,
		    rnh, rt->rt_nodes);
		if (rn == 0) {
			struct rtentry *rt2;
			  
                                                      
                                                       
                                                 
                                                    
                                   
      
			if (flags & RTF_IFSCOPE) {
				rt2 = rtalloc1_scoped_locked(dst0, 0,
				    RTF_CLONING | RTF_PRCLONING, ifscope);
			} else {
				rt2 = rtalloc1_locked(dst, 0,
				    RTF_CLONING | RTF_PRCLONING);
			}
			if (rt2 && rt2->rt_parent) {
				  
                                           
                                         
       
				(void) rtrequest_locked(RTM_DELETE, rt_key(rt2),
				    rt2->rt_gateway, rt_mask(rt2),
				    rt2->rt_flags, 0);
				rtfree_locked(rt2);
				rn = rnh->rnh_addaddr((caddr_t)ndst,
				    (caddr_t)netmask, rnh, rt->rt_nodes);
			} else if (rt2) {
				                               
				rtfree_locked(rt2);
			}
		}

		  
                                            
                                                
     
		if (rn == NULL) {
			                         
			rt_set_gwroute(rt, rt_key(rt), NULL);
			if (rt->rt_ifa) {
				IFA_REMREF(rt->rt_ifa);
				rt->rt_ifa = NULL;
			}
			R_Free(rt_key(rt));
			RT_UNLOCK(rt);
			nstat_route_detach(rt);
			rte_lock_destroy(rt);
			rte_free(rt);
			senderr(EEXIST);
		}

		rt->rt_parent = NULL;

		  
                                                              
                                                              
                                                                  
                                                            
                                                           
                                                            
                                   
     
		if (req == RTM_RESOLVE) {
			RT_LOCK_SPIN(*ret_nrt);
			VERIFY((*ret_nrt)->rt_expire == 0 ||
			    (*ret_nrt)->rt_rmx.rmx_expire != 0);
			VERIFY((*ret_nrt)->rt_expire != 0 ||
			    (*ret_nrt)->rt_rmx.rmx_expire == 0);
			rt->rt_rmx = (*ret_nrt)->rt_rmx;
			rt_setexpire(rt, (*ret_nrt)->rt_expire);
			if ((*ret_nrt)->rt_flags &
			    (RTF_CLONING | RTF_PRCLONING)) {
				rt->rt_parent = (*ret_nrt);
				RT_ADDREF_LOCKED(*ret_nrt);
			}
			RT_UNLOCK(*ret_nrt);
		}

		  
                                                       
                                 
     
		IFA_LOCK_SPIN(ifa);
		ifa_rtrequest = ifa->ifa_rtrequest;
		IFA_UNLOCK(ifa);
		if (ifa_rtrequest != NULL) {
			ifa_rtrequest(req, rt, SA(ret_nrt ? *ret_nrt : NULL));
		}
		IFA_REMREF(ifa);
		ifa = NULL;

		  
                                                      
                                                      
     
		if (rt_primary_default(rt, rt_key(rt))) {
			set_primary_ifscope(rt_key(rt)->sa_family,
			    rt->rt_ifp->if_index);
		}

#if NECP
		  
                                                   
                                        
     
		if (SA_DEFAULT(rt_key(rt))) {
			  
                                                                         
                                            
                                              
                                                                               
                                                               
      
			if (req == RTM_ADD && (rt->rt_flags & RTF_HOST) == 0) {
				rt->rt_flags |= RTF_GLOBAL;
			}
			if (rt->rt_ifp != NULL) {
				ifnet_touch_lastupdown(rt->rt_ifp);
			}
			necp_update_all_clients();
		}
#endif           

		  
                                            
                                        
     
		if (ret_nrt) {
			*ret_nrt = rt;
			RT_ADDREF_LOCKED(rt);
		}

		if (af == AF_INET) {
			routegenid_inet_update();
		} else if (af == AF_INET6) {
			routegenid_inet6_update();
		}

		RT_GENID_SYNC(rt);

		  
                                                         
                                                              
                                  
     
		if ((rt->rt_flags & RTF_GATEWAY) && rt->rt_gwroute != NULL) {
			rt_set_gwroute(rt, rt_key(rt), rt->rt_gwroute);
		}

		if (req == RTM_ADD &&
		    !(rt->rt_flags & RTF_HOST) && rt_mask(rt) != NULL) {
			struct rtfc_arg arg;
			arg.rnh = rnh;
			arg.rt0 = rt;
			RT_UNLOCK(rt);
			rnh->rnh_walktree_from(rnh, rt_key(rt), rt_mask(rt),
			    rt_fixchange, &arg);
		} else {
			RT_UNLOCK(rt);
		}

		nstat_route_new_entry(rt);
		break;
	}
bad:
	if (ifa) {
		IFA_REMREF(ifa);
	}
	return error;
}
#undef senderr

int
rtrequest(int req, struct sockaddr *dst, struct sockaddr *gateway,
    struct sockaddr *netmask, int flags, struct rtentry **ret_nrt)
{
	int error;
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(rnh_lock);
	error = rtrequest_locked(req, dst, gateway, netmask, flags, ret_nrt);
	lck_mtx_unlock(rnh_lock);
	return error;
}

int
rtrequest_scoped(int req, struct sockaddr *dst, struct sockaddr *gateway,
    struct sockaddr *netmask, int flags, struct rtentry **ret_nrt,
    unsigned int ifscope)
{
	int error;
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);
	lck_mtx_lock(rnh_lock);
	error = rtrequest_scoped_locked(req, dst, gateway, netmask, flags,
	    ret_nrt, ifscope);
	lck_mtx_unlock(rnh_lock);
	return error;
}

  
                                                                          
                                                                      
                                                                       
                                                                          
                                                                 
   
static int
rt_fixdelete(struct radix_node *rn, void *vp)
{
	struct rtentry *rt = (struct rtentry *)rn;
	struct rtentry *rt0 = vp;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);

	RT_LOCK(rt);
	if (rt->rt_parent == rt0 &&
	    !(rt->rt_flags & (RTF_CLONING | RTF_PRCLONING))) {
		  
                                                       
                                                       
                                
     
		RT_UNLOCK(rt);
		return rtrequest_locked(RTM_DELETE, rt_key(rt), NULL,
		           rt_mask(rt), rt->rt_flags, NULL);
	}
	RT_UNLOCK(rt);
	return 0;
}

  
                                                                         
                                                                       
                                                                         
                                                                           
                                                                        
                                                                      
                                      
  
                                                                          
                                                                            
                    
   
static int
rt_fixchange(struct radix_node *rn, void *vp)
{
	struct rtentry *rt = (struct rtentry *)rn;
	struct rtfc_arg *ap = vp;
	struct rtentry *rt0 = ap->rt0;
	struct radix_node_head *rnh = ap->rnh;
	u_char *xk1, *xm1, *xk2, *xmp;
	int i, len;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);

	RT_LOCK(rt);

	if (!rt->rt_parent ||
	    (rt->rt_flags & (RTF_CLONING | RTF_PRCLONING))) {
		RT_UNLOCK(rt);
		return 0;
	}

	if (rt->rt_parent == rt0) {
		goto delete_rt;
	}

	  
                                                             
                            
    
	len = imin(rt_key(rt0)->sa_len, rt_key(rt)->sa_len);

	xk1 = (u_char *)rt_key(rt0);
	xm1 = (u_char *)rt_mask(rt0);
	xk2 = (u_char *)rt_key(rt);

	  
                                                                    
                                                                      
                                               
    
	if ((xmp = (u_char *)rt_mask(rt->rt_parent)) != NULL) {
		int mlen = rt_mask(rt->rt_parent)->sa_len;
		if (mlen > rt_mask(rt0)->sa_len) {
			RT_UNLOCK(rt);
			return 0;
		}

		for (i = rnh->rnh_treetop->rn_offset; i < mlen; i++) {
			if ((xmp[i] & ~(xmp[i] ^ xm1[i])) != xmp[i]) {
				RT_UNLOCK(rt);
				return 0;
			}
		}
	}

	for (i = rnh->rnh_treetop->rn_offset; i < len; i++) {
		if ((xk2[i] & xm1[i]) != xk1[i]) {
			RT_UNLOCK(rt);
			return 0;
		}
	}

	  
                                                                  
                                                            
    
delete_rt:
	  
                                                                    
                                                                    
    
	RT_UNLOCK(rt);
	return rtrequest_locked(RTM_DELETE, rt_key(rt), NULL,
	           rt_mask(rt), rt->rt_flags, NULL);
}

  
                                                                    
                                                                     
                                                                      
                                                                        
                                                                     
                                                                     
                                  
   
#define SA_SIZE(x) (-(-((uintptr_t)(x)) & -(32)))

  
                                                                   
                                                                   
                                                                    
                                                                    
                      
   
int
rt_setgate(struct rtentry *rt, struct sockaddr *dst, struct sockaddr *gate)
{
	int dlen = SA_SIZE(dst->sa_len), glen = SA_SIZE(gate->sa_len);
	struct radix_node_head *rnh = NULL;
	boolean_t loop = FALSE;

	if (dst->sa_family != AF_INET && dst->sa_family != AF_INET6) {
		return EINVAL;
	}

	rnh = rt_tables[dst->sa_family];
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);
	RT_LOCK_ASSERT_HELD(rt);

	  
                                                               
                                                              
    
	if (rt->rt_flags & RTF_CONDEMNED) {
		return EBUSY;
	}

	                                    
	RT_ADDREF_LOCKED(rt);

	if (rt->rt_flags & RTF_GATEWAY) {
		if ((dst->sa_len == gate->sa_len) &&
		    (dst->sa_family == AF_INET || dst->sa_family == AF_INET6)) {
			struct sockaddr_storage dst_ss, gate_ss;

			(void) sa_copy(dst, &dst_ss, NULL);
			(void) sa_copy(gate, &gate_ss, NULL);

			loop = equal(SA(&dst_ss), SA(&gate_ss));
		} else {
			loop = (dst->sa_len == gate->sa_len &&
			    equal(dst, gate));
		}
	}

	  
                                                                       
                                                                  
    
	if (((rt->rt_flags & (RTF_HOST | RTF_GATEWAY | RTF_LLINFO)) ==
	    RTF_GATEWAY) && loop) {
		                       
		RT_REMREF_LOCKED(rt);
		return EADDRNOTAVAIL;
	}

	  
                                                          
                                                     
                          
    
	if (((rt->rt_flags & (RTF_HOST | RTF_GATEWAY | RTF_LLINFO)) ==
	    (RTF_HOST | RTF_GATEWAY)) && loop) {
		  
                                                           
                                                
     
		if (rt_key(rt) != NULL) {
			  
                                                      
                                                         
                                              
      
			RT_UNLOCK(rt);
			(void) rtrequest_locked(RTM_DELETE, rt_key(rt),
			    rt->rt_gateway, rt_mask(rt), rt->rt_flags, NULL);
			RT_LOCK(rt);
		}
		                       
		RT_REMREF_LOCKED(rt);
		return EADDRNOTAVAIL;
	}

	  
                                                           
                                                       
    
	if (rt->rt_flags & RTF_GATEWAY) {
		struct rtentry *gwrt;
		unsigned int ifscope;

		if (dst->sa_family == AF_INET) {
			ifscope = sin_get_ifscope(dst);
		} else if (dst->sa_family == AF_INET6) {
			ifscope = sin6_get_ifscope(dst);
		} else {
			ifscope = IFSCOPE_NONE;
		}

		RT_UNLOCK(rt);
		  
                                                              
                                                             
                                                    
     
		gwrt = rtalloc1_scoped_locked(gate, 1, RTF_PRCLONING, ifscope);
		if (gwrt != NULL) {
			RT_LOCK_ASSERT_NOTHELD(gwrt);
		}
		RT_LOCK(rt);

		  
                            
    
                                                               
                                                               
                                                           
                                                             
                                                          
                                                              
                                                              
                                                             
                                                               
                                                        
     
		if (gwrt == rt) {
			RT_REMREF_LOCKED(gwrt);
			                       
			RT_REMREF_LOCKED(rt);
			return EADDRINUSE;              
		}

		  
                                                              
                                                                 
                                                 
     
		if (ifscope != IFSCOPE_NONE && (rt->rt_flags & RTF_IFSCOPE) &&
		    gwrt != NULL && gwrt->rt_ifp != NULL &&
		    gwrt->rt_ifp->if_index != ifscope) {
			rtfree_locked(gwrt);                                 
			                       
			RT_REMREF_LOCKED(rt);
			return (rt->rt_flags & RTF_HOST) ?
			       EHOSTUNREACH : ENETUNREACH;
		}

		                                                 
		if (rt->rt_flags & RTF_CONDEMNED) {
			if (gwrt != NULL) {
				rtfree_locked(gwrt);
			}
			                       
			RT_REMREF_LOCKED(rt);
			return EBUSY;
		}

		                                                            
		rt_set_gwroute(rt, dst, gwrt);

		  
                                                             
                                                              
                                                           
                               
     
		if (rt_primary_default(rt, dst) && rt->rt_ifp != NULL) {
			set_primary_ifscope(dst->sa_family,
			    rt->rt_ifp->if_index);
		}

#if NECP
		  
                                                   
                                        
     
		if (SA_DEFAULT(dst)) {
			necp_update_all_clients();
		}
#endif           

		  
                                                           
                                                        
                                                         
                                                            
                                                            
                                             
     
		if ((dst->sa_family == AF_INET) &&
		    gwrt != NULL && gwrt->rt_gateway->sa_family == AF_LINK &&
		    (gwrt->rt_ifp->if_index == get_primary_ifscope(AF_INET) ||
		    get_primary_ifscope(AF_INET) == IFSCOPE_NONE)) {
			kdp_set_gateway_mac(SDL((void *)gwrt->rt_gateway)->
			    sdl_data);
		}

		                                       
		if (gwrt != NULL) {
			RT_REMREF(gwrt);
		}
	}

	  
                                                                     
                                                                     
                                                                   
                                                                 
                                                              
    
	if (rt->rt_gateway == NULL || glen > SA_SIZE(rt->rt_gateway->sa_len)) {
		caddr_t new;

		                                                         
		R_Malloc(new, caddr_t, dlen + glen);
		if (new == NULL) {
			                         
			rt_set_gwroute(rt, dst, NULL);
			                       
			RT_REMREF_LOCKED(rt);
			return ENOBUFS;
		}

		  
                                                          
                                                         
                                                            
     
		bzero(new, dlen + glen);
		Bcopy(dst, new, dst->sa_len);
		R_Free(rt_key(rt));                                       
		rt->rt_nodes->rn_key = new;
		rt->rt_gateway = (struct sockaddr *)(new + dlen);
	}

	  
                                                     
    
	Bcopy(gate, rt->rt_gateway, gate->sa_len);

	  
                                                        
    
	if ((rt->rt_flags & RTF_GATEWAY) && rt->rt_gwroute != NULL &&
	    (rt->rt_gwroute->rt_flags & RTF_IFSCOPE)) {
		if (rt->rt_gateway->sa_family == AF_INET &&
		    rt_key(rt->rt_gwroute)->sa_family == AF_INET) {
			sin_set_ifscope(rt->rt_gateway,
			    sin_get_ifscope(rt_key(rt->rt_gwroute)));
		} else if (rt->rt_gateway->sa_family == AF_INET6 &&
		    rt_key(rt->rt_gwroute)->sa_family == AF_INET6) {
			sin6_set_ifscope(rt->rt_gateway,
			    sin6_get_ifscope(rt_key(rt->rt_gwroute)));
		}
	}

	  
                                                              
                                                           
                                        
    
	if (!(rt->rt_flags & RTF_HOST) && rt_mask(rt) != 0) {
		struct rtfc_arg arg;
		arg.rnh = rnh;
		arg.rt0 = rt;
		RT_UNLOCK(rt);
		rnh->rnh_walktree_from(rnh, rt_key(rt), rt_mask(rt),
		    rt_fixchange, &arg);
		RT_LOCK(rt);
	}

	                       
	RT_REMREF_LOCKED(rt);
	return 0;
}

#undef SA_SIZE

void
rt_set_gwroute(struct rtentry *rt, struct sockaddr *dst, struct rtentry *gwrt)
{
	boolean_t gwrt_isrouter;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);
	RT_LOCK_ASSERT_HELD(rt);

	if (gwrt != NULL) {
		RT_ADDREF(gwrt);                              
	}
	  
                                                               
                                                              
                                                           
    
	if (rt->rt_gwroute != NULL) {
		struct rtentry *ogwrt = rt->rt_gwroute;

		VERIFY(rt != ogwrt);                      
		rt->rt_gwroute = NULL;
		RT_UNLOCK(rt);
		rtfree_locked(ogwrt);
		RT_LOCK(rt);
		VERIFY(rt->rt_gwroute == NULL);
	}

	  
                                        
    
	if ((rt->rt_gwroute = gwrt) != NULL) {
		RT_ADDREF(gwrt);                    

		if (rt->rt_flags & RTF_WASCLONED) {
			                                                
			gwrt_isrouter = (rt->rt_parent != NULL &&
			    SA_DEFAULT(rt_key(rt->rt_parent)) &&
			    !RT_HOST(rt->rt_parent));
		} else {
			gwrt_isrouter = (SA_DEFAULT(dst) && !RT_HOST(rt));
		}

		                                                             
		if (gwrt_isrouter && RT_HOST(gwrt) &&
		    !(gwrt->rt_flags & RTF_ROUTER)) {
			RT_LOCK(gwrt);
			gwrt->rt_flags |= RTF_ROUTER;
			RT_UNLOCK(gwrt);
		}

		RT_REMREF(gwrt);                              
	}
}

static void
rt_maskedcopy(const struct sockaddr *src, struct sockaddr *dst,
    const struct sockaddr *netmask)
{
	const char *netmaskp = &netmask->sa_data[0];
	const char *srcp = &src->sa_data[0];
	char *dstp = &dst->sa_data[0];
	const char *maskend = (char *)dst
	    + MIN(netmask->sa_len, src->sa_len);
	const char *srcend = (char *)dst + src->sa_len;

	dst->sa_len = src->sa_len;
	dst->sa_family = src->sa_family;

	while (dstp < maskend) {
		*dstp++ = *srcp++ & *netmaskp++;
	}
	if (dstp < srcend) {
		memset(dstp, 0, (size_t)(srcend - dstp));
	}
}

  
                                                                         
                                                                           
   
static struct radix_node *
node_lookup(struct sockaddr *dst, struct sockaddr *netmask,
    unsigned int ifscope)
{
	struct radix_node_head *rnh;
	struct radix_node *rn;
	struct sockaddr_storage ss, mask;
	int af = dst->sa_family;
	struct matchleaf_arg ma = { .ifscope = ifscope };
	rn_matchf_t *f = rn_match_ifscope;
	void *w = &ma;

	if (af != AF_INET && af != AF_INET6) {
		return NULL;
	}

	rnh = rt_tables[af];

	  
                                                       
                                                         
    
	dst = sa_copy(dst, &ss, (ifscope == IFSCOPE_NONE) ? NULL : &ifscope);

	                                                            
	if (netmask != NULL) {
		netmask = ma_copy(af, netmask, &mask, ifscope);
	}

	if (ifscope == IFSCOPE_NONE) {
		f = w = NULL;
	}

	rn = rnh->rnh_lookup_args(dst, netmask, rnh, f, w);
	if (rn != NULL && (rn->rn_flags & RNF_ROOT)) {
		rn = NULL;
	}

	return rn;
}

  
                                                        
   
static struct radix_node *
node_lookup_default(int af)
{
	struct radix_node_head *rnh;

	VERIFY(af == AF_INET || af == AF_INET6);
	rnh = rt_tables[af];

	return af == AF_INET ? rnh->rnh_lookup(&sin_def, NULL, rnh) :
	       rnh->rnh_lookup(&sin6_def, NULL, rnh);
}

boolean_t
rt_ifa_is_dst(struct sockaddr *dst, struct ifaddr *ifa)
{
	boolean_t result = FALSE;

	if (ifa == NULL || ifa->ifa_addr == NULL) {
		return result;
	}

	IFA_LOCK_SPIN(ifa);

	if (dst->sa_family == ifa->ifa_addr->sa_family &&
	    ((dst->sa_family == AF_INET &&
	    SIN(dst)->sin_addr.s_addr ==
	    SIN(ifa->ifa_addr)->sin_addr.s_addr) ||
	    (dst->sa_family == AF_INET6 &&
	    SA6_ARE_ADDR_EQUAL(SIN6(dst), SIN6(ifa->ifa_addr))))) {
		result = TRUE;
	}

	IFA_UNLOCK(ifa);

	return result;
}

  
                                                                           
                                                                        
                                                                        
                                                                          
                           
  
                                                                         
                                                                            
                                                                          
                                                                            
                                                                           
                                                                        
                                                                          
                                                                     
                                                       
  
                                                                             
                                                                              
                                                                             
                                                                            
                                                   
   
static struct rtentry *
rt_lookup_common(boolean_t lookup_only, boolean_t coarse, struct sockaddr *dst,
    struct sockaddr *netmask, struct radix_node_head *rnh, unsigned int ifscope)
{
	struct radix_node *rn0, *rn = NULL;
	int af = dst->sa_family;
	struct sockaddr_storage dst_ss;
	struct sockaddr_storage mask_ss;
	boolean_t dontcare;
#if (DEVELOPMENT || DEBUG)
	char dbuf[MAX_SCOPE_ADDR_STR_LEN], gbuf[MAX_IPv6_STR_LEN];
	char s_dst[MAX_IPv6_STR_LEN], s_netmask[MAX_IPv6_STR_LEN];
#endif
	VERIFY(!coarse || ifscope == IFSCOPE_NONE);

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);
	  
                                                                      
    
	if (nd6_sched_timeout_want) {
		nd6_sched_timeout(NULL, NULL);
	}

	if (!lookup_only) {
		netmask = NULL;
	}

	  
                            
    
	if (af != AF_INET && af != AF_INET6) {
		rn = rnh->rnh_matchaddr(dst, rnh);

		  
                                                           
                                                           
                                   
     
		if (rn != NULL && (rn->rn_flags & RNF_ROOT)) {
			rn = NULL;
		}
		if (rn != NULL) {
			RT_LOCK_SPIN(RT(rn));
			if (!(RT(rn)->rt_flags & RTF_CONDEMNED)) {
				RT_ADDREF_LOCKED(RT(rn));
				RT_UNLOCK(RT(rn));
			} else {
				RT_UNLOCK(RT(rn));
				rn = NULL;
			}
		}
		return RT(rn);
	}

	                                                                
	dst = sa_copy(dst, &dst_ss, &ifscope);
	if (netmask != NULL) {
		netmask = ma_copy(af, netmask, &mask_ss, ifscope);
	}
	dontcare = (ifscope == IFSCOPE_NONE);

#if (DEVELOPMENT || DEBUG)
	if (rt_verbose) {
		if (af == AF_INET) {
			(void) inet_ntop(af, &SIN(dst)->sin_addr.s_addr,
			    s_dst, sizeof(s_dst));
		} else {
			(void) inet_ntop(af, &SIN6(dst)->sin6_addr,
			    s_dst, sizeof(s_dst));
		}

		if (netmask != NULL && af == AF_INET) {
			(void) inet_ntop(af, &SIN(netmask)->sin_addr.s_addr,
			    s_netmask, sizeof(s_netmask));
		}
		if (netmask != NULL && af == AF_INET6) {
			(void) inet_ntop(af, &SIN6(netmask)->sin6_addr,
			    s_netmask, sizeof(s_netmask));
		} else {
			*s_netmask = '\0';
		}
		printf("%s (%d, %d, %s, %s, %u)\n",
		    __func__, lookup_only, coarse, s_dst, s_netmask, ifscope);
	}
#endif

	  
                        
   
                                                                 
                                                                    
                                                                  
                                         
    
	rn0 = rn = node_lookup(dst, netmask, IFSCOPE_NONE);

	  
                                                                
                                                                
                                                              
                                                                 
                                                                  
                  
    
	if (dontcare) {
		ifscope = get_primary_ifscope(af);
	}

	  
                                                                
   
                                                                  
                                                               
                                                                 
                                             
   
                                                          
                                                            
    
	if (rn != NULL) {
		struct rtentry *rt = RT(rn);
#if (DEVELOPMENT || DEBUG)
		if (rt_verbose) {
			rt_str(rt, dbuf, sizeof(dbuf), gbuf, sizeof(gbuf));
			printf("%s unscoped search %p to %s->%s->%s ifa_ifp %s\n",
			    __func__, rt,
			    dbuf, gbuf,
			    (rt->rt_ifp != NULL) ? rt->rt_ifp->if_xname : "",
			    (rt->rt_ifa->ifa_ifp != NULL) ?
			    rt->rt_ifa->ifa_ifp->if_xname : "");
		}
#endif
		if (!(rt->rt_ifp->if_flags & IFF_LOOPBACK) ||
		    (rt->rt_flags & RTF_GATEWAY)) {
			if (rt->rt_ifp->if_index != ifscope) {
				  
                                                
                                                  
                                                 
                                                
                                
      
                                              
                                
       
				rn = NULL;
				if (dontcare) {
					ifscope = rt->rt_ifp->if_index;
				} else if (ifscope != lo_ifp->if_index ||
				    rt_ifa_is_dst(dst, rt->rt_ifa) == FALSE) {
					rn0 = NULL;
				}
			} else if (!(rt->rt_flags & RTF_IFSCOPE)) {
				  
                                              
                                                
                                                 
                                                 
                    
       
				rn = NULL;
			}
		}
	}

	  
                                                                
                                                                    
                                                     
    
	if (rn == NULL) {
		rn = node_lookup(dst, netmask, ifscope);
#if (DEVELOPMENT || DEBUG)
		if (rt_verbose && rn != NULL) {
			struct rtentry *rt = RT(rn);

			rt_str(rt, dbuf, sizeof(dbuf), gbuf, sizeof(gbuf));
			printf("%s scoped search %p to %s->%s->%s ifa %s\n",
			    __func__, rt,
			    dbuf, gbuf,
			    (rt->rt_ifp != NULL) ? rt->rt_ifp->if_xname : "",
			    (rt->rt_ifa->ifa_ifp != NULL) ?
			    rt->rt_ifa->ifa_ifp->if_xname : "");
		}
#endif
	}
	  
                                                               
   
                                                    
                                                                  
                                                                     
                                                                
                                                               
                                                               
                                                               
                             
    
	if (rn == NULL || coarse || (rn0 != NULL &&
	    ((SA_DEFAULT(rt_key(RT(rn))) && !SA_DEFAULT(rt_key(RT(rn0)))) ||
	    (!RT_HOST(rn) && RT_HOST(rn0))))) {
		rn = rn0;
	}

	  
                                                              
                                                                
    
	if (rn == NULL && (rn = node_lookup_default(af)) != NULL &&
	    RT(rn)->rt_ifp->if_index != ifscope) {
		rn = NULL;
	}

	if (rn != NULL) {
		  
                                                      
                                                       
                                                          
                                                           
                                               
     
		RT_LOCK_SPIN(RT(rn));
		if (rt_validate(RT(rn))) {
			RT_ADDREF_LOCKED(RT(rn));
			RT_UNLOCK(RT(rn));
		} else {
			RT_UNLOCK(RT(rn));
			rn = NULL;
		}
	}
#if (DEVELOPMENT || DEBUG)
	if (rt_verbose) {
		if (rn == NULL) {
			printf("%s %u return NULL\n", __func__, ifscope);
		} else {
			struct rtentry *rt = RT(rn);

			rt_str(rt, dbuf, sizeof(dbuf), gbuf, sizeof(gbuf));

			printf("%s %u return %p to %s->%s->%s ifa_ifp %s\n",
			    __func__, ifscope, rt,
			    dbuf, gbuf,
			    (rt->rt_ifp != NULL) ? rt->rt_ifp->if_xname : "",
			    (rt->rt_ifa->ifa_ifp != NULL) ?
			    rt->rt_ifa->ifa_ifp->if_xname : "");
		}
	}
#endif
	return RT(rn);
}

struct rtentry *
rt_lookup(boolean_t lookup_only, struct sockaddr *dst, struct sockaddr *netmask,
    struct radix_node_head *rnh, unsigned int ifscope)
{
	return rt_lookup_common(lookup_only, FALSE, dst, netmask,
	           rnh, ifscope);
}

struct rtentry *
rt_lookup_coarse(boolean_t lookup_only, struct sockaddr *dst,
    struct sockaddr *netmask, struct radix_node_head *rnh)
{
	return rt_lookup_common(lookup_only, TRUE, dst, netmask,
	           rnh, IFSCOPE_NONE);
}

boolean_t
rt_validate(struct rtentry *rt)
{
	RT_LOCK_ASSERT_HELD(rt);

	if ((rt->rt_flags & (RTF_UP | RTF_CONDEMNED)) == RTF_UP) {
		int af = rt_key(rt)->sa_family;

		if (af == AF_INET) {
			(void) in_validate(RN(rt));
		} else if (af == AF_INET6) {
			(void) in6_validate(RN(rt));
		}
	} else {
		rt = NULL;
	}

	return rt != NULL;
}

  
                                         
                    
   
int
rtinit(struct ifaddr *ifa, int cmd, int flags)
{
	int error;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_NOTOWNED);

	lck_mtx_lock(rnh_lock);
	error = rtinit_locked(ifa, cmd, flags);
	lck_mtx_unlock(rnh_lock);

	return error;
}

int
rtinit_locked(struct ifaddr *ifa, int cmd, int flags)
{
	struct radix_node_head *rnh;
	uint8_t nbuf[128];                                
#if (DEVELOPMENT || DEBUG)
	char dbuf[MAX_IPv6_STR_LEN], gbuf[MAX_IPv6_STR_LEN];
	char abuf[MAX_IPv6_STR_LEN];
#endif
	struct rtentry *rt = NULL;
	struct sockaddr *dst;
	struct sockaddr *netmask;
	int error = 0;

	  
                                                              
                                                          
                                                        
    
	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);

	if (flags & RTF_HOST) {
		dst = ifa->ifa_dstaddr;
		netmask = NULL;
	} else {
		dst = ifa->ifa_addr;
		netmask = ifa->ifa_netmask;
	}

	if (dst->sa_len == 0) {
		log(LOG_ERR, "%s: %s failed, invalid dst sa_len %d\n",
		    __func__, rtm2str(cmd), dst->sa_len);
		error = EINVAL;
		goto done;
	}
	if (netmask != NULL && netmask->sa_len > sizeof(nbuf)) {
		log(LOG_ERR, "%s: %s failed, mask sa_len %d too large\n",
		    __func__, rtm2str(cmd), dst->sa_len);
		error = EINVAL;
		goto done;
	}

#if (DEVELOPMENT || DEBUG)
	if (dst->sa_family == AF_INET) {
		(void) inet_ntop(AF_INET, &SIN(dst)->sin_addr.s_addr,
		    abuf, sizeof(abuf));
	} else if (dst->sa_family == AF_INET6) {
		(void) inet_ntop(AF_INET6, &SIN6(dst)->sin6_addr,
		    abuf, sizeof(abuf));
	}
#endif                             

	if ((rnh = rt_tables[dst->sa_family]) == NULL) {
		error = EINVAL;
		goto done;
	}

	  
                                                                  
                                                                  
                                            
    
	if (cmd == RTM_DELETE) {
		  
                                                
                                          
                              
     
		if (netmask != NULL) {
			rt_maskedcopy(dst, SA(nbuf), netmask);
			dst = SA(nbuf);
		}
		  
                                                            
                                                             
                                                          
                                                            
                                                          
                       
     
		rt = rt_lookup_coarse(TRUE, dst, NULL, rnh);
		if (rt != NULL) {
#if (DEVELOPMENT || DEBUG)
			rt_str(rt, dbuf, sizeof(dbuf), gbuf, sizeof(gbuf));
#endif
			  
                                                           
                                                 
                       
      
			RT_LOCK(rt);
			if (rt->rt_ifa != ifa) {
				  
                                              
                                                
                                                 
                                                 
                                       
       
#if (DEVELOPMENT || DEBUG)
				if (rt_verbose) {
					log(LOG_DEBUG, "%s: not removing "
					    "route to %s->%s->%s, flags %b, "
					    "ifaddr %s, rt_ifa 0x%llx != "
					    "ifa 0x%llx\n", __func__, dbuf,
					    gbuf, ((rt->rt_ifp != NULL) ?
					    rt->rt_ifp->if_xname : ""),
					    rt->rt_flags, RTF_BITS, abuf,
					    (uint64_t)VM_KERNEL_ADDRPERM(
						    rt->rt_ifa),
					    (uint64_t)VM_KERNEL_ADDRPERM(ifa));
				}
#endif                             
				RT_REMREF_LOCKED(rt);
				RT_UNLOCK(rt);
				rt = NULL;
				error = ((flags & RTF_HOST) ?
				    EHOSTUNREACH : ENETUNREACH);
				goto done;
			} else if (rt->rt_flags & RTF_STATIC) {
				  
                                              
                                          
       
#if (DEVELOPMENT || DEBUG)
				if (rt_verbose) {
					log(LOG_DEBUG, "%s: not removing "
					    "static route to %s->%s->%s, "
					    "flags %b, ifaddr %s\n", __func__,
					    dbuf, gbuf, ((rt->rt_ifp != NULL) ?
					    rt->rt_ifp->if_xname : ""),
					    rt->rt_flags, RTF_BITS, abuf);
				}
#endif                             
				RT_REMREF_LOCKED(rt);
				RT_UNLOCK(rt);
				rt = NULL;
				error = EBUSY;
				goto done;
			}
#if (DEVELOPMENT || DEBUG)
			if (rt_verbose) {
				log(LOG_DEBUG, "%s: removing route to "
				    "%s->%s->%s, flags %b, ifaddr %s\n",
				    __func__, dbuf, gbuf,
				    ((rt->rt_ifp != NULL) ?
				    rt->rt_ifp->if_xname : ""),
				    rt->rt_flags, RTF_BITS, abuf);
			}
#endif                             
			RT_REMREF_LOCKED(rt);
			RT_UNLOCK(rt);
			rt = NULL;
		}
	}
	  
                         
    
	if ((error = rtrequest_locked(cmd, dst, ifa->ifa_addr, netmask,
	    flags | ifa->ifa_flags, &rt)) != 0) {
		goto done;
	}

	VERIFY(rt != NULL);
#if (DEVELOPMENT || DEBUG)
	rt_str(rt, dbuf, sizeof(dbuf), gbuf, sizeof(gbuf));
#endif                             
	switch (cmd) {
	case RTM_DELETE:
		  
                                                         
                                                       
                                                    
     
		RT_LOCK(rt);
		rt_newaddrmsg(cmd, ifa, error, rt);
		RT_UNLOCK(rt);
#if (DEVELOPMENT || DEBUG)
		if (rt_verbose) {
			log(LOG_DEBUG, "%s: removed route to %s->%s->%s, "
			    "flags %b, ifaddr %s\n", __func__, dbuf, gbuf,
			    ((rt->rt_ifp != NULL) ? rt->rt_ifp->if_xname : ""),
			    rt->rt_flags, RTF_BITS, abuf);
		}
#endif                             
		rtfree_locked(rt);
		break;

	case RTM_ADD:
		  
                                                         
                                                         
                                                            
                          
     
		RT_LOCK(rt);
		if (rt->rt_ifa != ifa) {
			void (*ifa_rtrequest)
			(int, struct rtentry *, struct sockaddr *);
#if (DEVELOPMENT || DEBUG)
			if (rt_verbose) {
				if (!(rt->rt_ifa->ifa_ifp->if_flags &
				    (IFF_POINTOPOINT | IFF_LOOPBACK))) {
					log(LOG_ERR, "%s: %s route to %s->%s->%s, "
					    "flags %b, ifaddr %s, rt_ifa 0x%llx != "
					    "ifa 0x%llx\n", __func__, rtm2str(cmd),
					    dbuf, gbuf, ((rt->rt_ifp != NULL) ?
					    rt->rt_ifp->if_xname : ""), rt->rt_flags,
					    RTF_BITS, abuf,
					    (uint64_t)VM_KERNEL_ADDRPERM(rt->rt_ifa),
					    (uint64_t)VM_KERNEL_ADDRPERM(ifa));
				}

				log(LOG_DEBUG, "%s: %s route to %s->%s->%s, "
				    "flags %b, ifaddr %s, rt_ifa was 0x%llx "
				    "now 0x%llx\n", __func__, rtm2str(cmd),
				    dbuf, gbuf, ((rt->rt_ifp != NULL) ?
				    rt->rt_ifp->if_xname : ""), rt->rt_flags,
				    RTF_BITS, abuf,
				    (uint64_t)VM_KERNEL_ADDRPERM(rt->rt_ifa),
				    (uint64_t)VM_KERNEL_ADDRPERM(ifa));
			}
#endif                             

			  
                                       
                                            
                            
      
			ifa_rtrequest = rt->rt_ifa->ifa_rtrequest;
			if (ifa_rtrequest != NULL) {
				ifa_rtrequest(RTM_DELETE, rt, NULL);
			}
			  
                          
      
			rtsetifa(rt, ifa);

			if (rt->rt_ifp != ifa->ifa_ifp) {
				  
                                         
       
				if (rt->rt_llinfo_purge != NULL) {
					rt->rt_llinfo_purge(rt);
				}
				  
                                                 
       
				if (rt->rt_if_ref_fn != NULL) {
					rt->rt_if_ref_fn(ifa->ifa_ifp, 1);
					rt->rt_if_ref_fn(rt->rt_ifp, -1);
				}
			}

			  
                                                
                    
      
			rt->rt_ifp = ifa->ifa_ifp;
			  
                                         
                                           
      
			if (!(rt->rt_rmx.rmx_locks & RTV_MTU)) {
				rt->rt_rmx.rmx_mtu = rt->rt_ifp->if_mtu;
				if (dst->sa_family == AF_INET &&
				    INTF_ADJUST_MTU_FOR_CLAT46(rt->rt_ifp)) {
					rt->rt_rmx.rmx_mtu = IN6_LINKMTU(rt->rt_ifp);
					                                                  
					rt->rt_rmx.rmx_mtu -= CLAT46_HDR_EXPANSION_OVERHD;
				}
			}

			  
                                               
                                             
      
			ifa_rtrequest = ifa->ifa_rtrequest;
			if (ifa_rtrequest != NULL) {
				ifa_rtrequest(RTM_ADD, rt, NULL);
			}
		} else {
#if (DEVELOPMENT || DEBUG)
			if (rt_verbose) {
				log(LOG_DEBUG, "%s: added route to %s->%s->%s, "
				    "flags %b, ifaddr %s\n", __func__, dbuf,
				    gbuf, ((rt->rt_ifp != NULL) ?
				    rt->rt_ifp->if_xname : ""), rt->rt_flags,
				    RTF_BITS, abuf);
			}
#endif                             
		}
		  
                                                       
     
		rt_newaddrmsg(cmd, ifa, error, rt);
		  
                                                       
                                                         
                                                         
                                                         
                                          
     
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
		break;

	default:
		VERIFY(0);
		                
	}
done:
	return error;
}

static void
rt_set_idleref(struct rtentry *rt)
{
	RT_LOCK_ASSERT_HELD(rt);

	  
                                                               
                                        
    
	if (rt->rt_parent != NULL && !(rt->rt_flags &
	    (RTF_NOIFREF | RTF_BROADCAST | RTF_MULTICAST)) &&
	    (rt->rt_flags & (RTF_UP | RTF_WASCLONED | RTF_IFREF)) ==
	    (RTF_UP | RTF_WASCLONED)) {
		rt_clear_idleref(rt);                                     
		rt->rt_if_ref_fn = rte_if_ref;
		                                          
		RT_CONVERT_LOCK(rt);
		rt->rt_if_ref_fn(rt->rt_ifp, 1);
		rt->rt_flags |= RTF_IFREF;
	}
}

void
rt_clear_idleref(struct rtentry *rt)
{
	RT_LOCK_ASSERT_HELD(rt);

	if (rt->rt_if_ref_fn != NULL) {
		VERIFY((rt->rt_flags & (RTF_NOIFREF | RTF_IFREF)) == RTF_IFREF);
		                                          
		RT_CONVERT_LOCK(rt);
		rt->rt_if_ref_fn(rt->rt_ifp, -1);
		rt->rt_flags &= ~RTF_IFREF;
		rt->rt_if_ref_fn = NULL;
	}
}

void
rt_set_proxy(struct rtentry *rt, boolean_t set)
{
	lck_mtx_lock(rnh_lock);
	RT_LOCK(rt);
	  
                                                 
                                                
    
	if (rt->rt_flags & (RTF_CLONING | RTF_PRCLONING)) {
		struct radix_node_head *rnh = rt_tables[rt_key(rt)->sa_family];

		if (set) {
			rt->rt_flags |= RTF_PROXY;
		} else {
			rt->rt_flags &= ~RTF_PROXY;
		}

		RT_UNLOCK(rt);
		if (rnh != NULL && rt_mask(rt)) {
			rnh->rnh_walktree_from(rnh, rt_key(rt), rt_mask(rt),
			    rt_fixdelete, rt);
		}
	} else {
		RT_UNLOCK(rt);
	}
	lck_mtx_unlock(rnh_lock);
}

static void
rte_lock_init(struct rtentry *rt)
{
	lck_mtx_init(&rt->rt_lock, rte_mtx_grp, rte_mtx_attr);
}

static void
rte_lock_destroy(struct rtentry *rt)
{
	RT_LOCK_ASSERT_NOTHELD(rt);
	lck_mtx_destroy(&rt->rt_lock, rte_mtx_grp);
}

void
rt_lock(struct rtentry *rt, boolean_t spin)
{
	RT_LOCK_ASSERT_NOTHELD(rt);
	if (spin) {
		lck_mtx_lock_spin(&rt->rt_lock);
	} else {
		lck_mtx_lock(&rt->rt_lock);
	}
	if (rte_debug & RTD_DEBUG) {
		rte_lock_debug((struct rtentry_dbg *)rt);
	}
}

void
rt_unlock(struct rtentry *rt)
{
	if (rte_debug & RTD_DEBUG) {
		rte_unlock_debug((struct rtentry_dbg *)rt);
	}
	lck_mtx_unlock(&rt->rt_lock);
}

static inline void
rte_lock_debug(struct rtentry_dbg *rte)
{
	uint32_t idx;

	RT_LOCK_ASSERT_HELD((struct rtentry *)rte);
	idx = atomic_add_32_ov(&rte->rtd_lock_cnt, 1) % CTRACE_HIST_SIZE;
	if (rte_debug & RTD_TRACE) {
		ctrace_record(&rte->rtd_lock[idx]);
	}
}

static inline void
rte_unlock_debug(struct rtentry_dbg *rte)
{
	uint32_t idx;

	RT_LOCK_ASSERT_HELD((struct rtentry *)rte);
	idx = atomic_add_32_ov(&rte->rtd_unlock_cnt, 1) % CTRACE_HIST_SIZE;
	if (rte_debug & RTD_TRACE) {
		ctrace_record(&rte->rtd_unlock[idx]);
	}
}

static struct rtentry *
rte_alloc(void)
{
	if (rte_debug & RTD_DEBUG) {
		return rte_alloc_debug();
	}

	return (struct rtentry *)zalloc(rte_zone);
}

static void
rte_free(struct rtentry *p)
{
	if (rte_debug & RTD_DEBUG) {
		rte_free_debug(p);
		return;
	}

	if (p->rt_refcnt != 0) {
		panic("rte_free: rte=%p refcnt=%d non-zero\n", p, p->rt_refcnt);
		                
	}

	zfree(rte_zone, p);
}

static void
rte_if_ref(struct ifnet *ifp, int cnt)
{
	struct kev_msg ev_msg;
	struct net_event_data ev_data;
	uint32_t old;

	                                        
	if (cnt < -1 || cnt > 1) {
		panic("%s: invalid count argument (%d)", __func__, cnt);
		                
	}
	old = atomic_add_32_ov(&ifp->if_route_refcnt, cnt);
	if (cnt < 0 && old == 0) {
		panic("%s: ifp=%p negative route refcnt!", __func__, ifp);
		                
	}
	  
                                                               
                                                             
                                                         
                                                        
                                                            
                                                              
                                                             
                                                             
                                                                
    
	if ((ifp->if_idle_flags & IFRF_IDLE_NOTIFY) && cnt < 0 && old == 1) {
		bzero(&ev_msg, sizeof(ev_msg));
		bzero(&ev_data, sizeof(ev_data));

		ev_msg.vendor_code      = KEV_VENDOR_APPLE;
		ev_msg.kev_class        = KEV_NETWORK_CLASS;
		ev_msg.kev_subclass     = KEV_DL_SUBCLASS;
		ev_msg.event_code       = KEV_DL_IF_IDLE_ROUTE_REFCNT;

		strlcpy(&ev_data.if_name[0], ifp->if_name, IFNAMSIZ);

		ev_data.if_family       = ifp->if_family;
		ev_data.if_unit         = ifp->if_unit;
		ev_msg.dv[0].data_length = sizeof(struct net_event_data);
		ev_msg.dv[0].data_ptr   = &ev_data;

		dlil_post_complete_msg(NULL, &ev_msg);
	}
}

static inline struct rtentry *
rte_alloc_debug(void)
{
	struct rtentry_dbg *rte;

	rte = ((struct rtentry_dbg *)zalloc(rte_zone));
	if (rte != NULL) {
		bzero(rte, sizeof(*rte));
		if (rte_debug & RTD_TRACE) {
			ctrace_record(&rte->rtd_alloc);
		}
		rte->rtd_inuse = RTD_INUSE;
	}
	return (struct rtentry *)rte;
}

static inline void
rte_free_debug(struct rtentry *p)
{
	struct rtentry_dbg *rte = (struct rtentry_dbg *)p;

	if (p->rt_refcnt != 0) {
		panic("rte_free: rte=%p refcnt=%d\n", p, p->rt_refcnt);
		                
	}
	if (rte->rtd_inuse == RTD_FREED) {
		panic("rte_free: double free rte=%p\n", rte);
		                
	} else if (rte->rtd_inuse != RTD_INUSE) {
		panic("rte_free: corrupted rte=%p\n", rte);
		                
	}
	bcopy((caddr_t)p, (caddr_t)&rte->rtd_entry_saved, sizeof(*p));
	                                                         
	bzero((caddr_t)p, offsetof(struct rtentry, rt_lock));

	rte->rtd_inuse = RTD_FREED;

	if (rte_debug & RTD_TRACE) {
		ctrace_record(&rte->rtd_free);
	}

	if (!(rte_debug & RTD_NO_FREE)) {
		zfree(rte_zone, p);
	}
}

void
ctrace_record(ctrace_t *tr)
{
	tr->th = current_thread();
	bzero(tr->pc, sizeof(tr->pc));
	(void) OSBacktrace(tr->pc, CTRACE_STACK_SIZE);
}

void
route_copyout(struct route *dst, const struct route *src, size_t length)
{
	                                                      
	bcopy(src, dst, length);

	                                                           
	if (dst->ro_rt != NULL) {
		RT_ADDREF(dst->ro_rt);
	}

	                                                         
	if (dst->ro_lle != NULL) {
		LLE_ADDREF(dst->ro_lle);
	}

	                                                            
	if (dst->ro_srcia != NULL) {
		IFA_ADDREF(dst->ro_srcia);
	}
}

void
route_copyin(struct route *src, struct route *dst, size_t length)
{
	  
                                       
                                                  
                              
    
	if (dst->ro_rt == NULL) {
		  
                                                
                                                         
     
		if (dst->ro_lle != NULL) {
			LLE_REMREF(dst->ro_lle);
		}
		  
                                                     
                                                    
     
		if (dst->ro_srcia != NULL) {
			IFA_REMREF(dst->ro_srcia);
		}
		  
                                                                  
                                                        
                                    
     
		bcopy(src, dst, length);
		goto done;
	}

	  
                                        
                                                                 
                                          
    
	if (dst->ro_rt == src->ro_rt) {
		dst->ro_flags = src->ro_flags;

		if (dst->ro_lle != src->ro_lle) {
			if (dst->ro_lle != NULL) {
				LLE_REMREF(dst->ro_lle);
			}
			dst->ro_lle = src->ro_lle;
		} else if (src->ro_lle != NULL) {
			LLE_REMREF(src->ro_lle);
		}

		if (dst->ro_srcia != src->ro_srcia) {
			if (dst->ro_srcia != NULL) {
				IFA_REMREF(dst->ro_srcia);
			}
			dst->ro_srcia = src->ro_srcia;
		} else if (src->ro_srcia != NULL) {
			IFA_REMREF(src->ro_srcia);
		}
		rtfree(src->ro_rt);
		goto done;
	}

	  
                                                  
                                                        
                                         
    
	if (src->ro_rt != NULL) {
		rtfree(dst->ro_rt);

		if (dst->ro_lle != NULL) {
			LLE_REMREF(dst->ro_lle);
		}
		if (dst->ro_srcia != NULL) {
			IFA_REMREF(dst->ro_srcia);
		}
		bcopy(src, dst, length);
		goto done;
	}

	  
                                                         
                                                          
    
	if (src->ro_srcia != NULL) {
		  
                                                          
                                                        
     
		IFA_REMREF(src->ro_srcia);
	}
	if (src->ro_lle != NULL) {
		  
                                                        
                                                  
     
		LLE_REMREF(src->ro_lle);
	}
done:
	                                                  
	src->ro_lle = NULL;
	src->ro_rt = NULL;
	src->ro_srcia = NULL;
}

  
                                                                  
  
                                                 
                                                                     
                                                  
                                                           
  
                                                                   
                                                   
   
#define senderr(e) { error = (e); goto bad; }
errno_t
route_to_gwroute(const struct sockaddr *net_dest, struct rtentry *hint0,
    struct rtentry **out_route)
{
	uint64_t timenow;
	struct rtentry *rt = hint0, *hint = hint0;
	errno_t error = 0;
	unsigned int ifindex;
	boolean_t gwroute;

	*out_route = NULL;

	if (rt == NULL) {
		return 0;
	}

	  
                                                                     
                                                                     
                                                                      
                                                                     
                                                                     
                        
   
                                                                        
                                                                     
                                                     
    
	RT_LOCK_SPIN(rt);
	ifindex = rt->rt_ifp->if_index;
	RT_ADDREF_LOCKED(rt);
	if (!(rt->rt_flags & RTF_UP)) {
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
		                                   
		hint = rt = rtalloc1_scoped((struct sockaddr *)
		    (size_t)net_dest, 1, 0, ifindex);
		if (hint != NULL) {
			RT_LOCK_SPIN(rt);
			ifindex = rt->rt_ifp->if_index;
		} else {
			senderr(EHOSTUNREACH);
		}
	}

	  
                                                      
                                                    
    
	RT_LOCK_ASSERT_HELD(rt);
	if ((gwroute = (rt->rt_flags & RTF_GATEWAY))) {
		struct rtentry *gwrt = rt->rt_gwroute;
		struct sockaddr_storage ss;
		struct sockaddr *gw = (struct sockaddr *)&ss;

		VERIFY(rt == hint);
		RT_ADDREF_LOCKED(hint);

		                                          
		if (gwrt == NULL) {
			bcopy(rt->rt_gateway, gw, MIN(sizeof(ss),
			    rt->rt_gateway->sa_len));
			gw->sa_len = MIN(sizeof(ss), rt->rt_gateway->sa_len);
			RT_UNLOCK(rt);
			goto lookup;
		}
		                            
		RT_CONVERT_LOCK(rt);

		  
                                                 
                                              
                                         
     
		RT_LOCK_SPIN(gwrt);
		if (!(gwrt->rt_flags & RTF_UP)) {
			rt->rt_gwroute = NULL;
			RT_UNLOCK(gwrt);
			bcopy(rt->rt_gateway, gw, MIN(sizeof(ss),
			    rt->rt_gateway->sa_len));
			gw->sa_len = MIN(sizeof(ss), rt->rt_gateway->sa_len);
			RT_UNLOCK(rt);
			rtfree(gwrt);
lookup:
			lck_mtx_lock(rnh_lock);
			gwrt = rtalloc1_scoped_locked(gw, 1, 0, ifindex);

			RT_LOCK(rt);
			  
                                             
                                           
                                          
      
			if (!(rt->rt_flags & RTF_UP) || gwrt == NULL ||
			    gwrt == rt || !equal(gw, rt->rt_gateway)) {
				if (gwrt == rt) {
					RT_REMREF_LOCKED(gwrt);
					gwrt = NULL;
				}
				VERIFY(rt == hint);
				RT_REMREF_LOCKED(hint);
				hint = NULL;
				RT_UNLOCK(rt);
				if (gwrt != NULL) {
					rtfree_locked(gwrt);
				}
				lck_mtx_unlock(rnh_lock);
				senderr(EHOSTUNREACH);
			}
			VERIFY(gwrt != NULL);
			  
                                                 
                                               
                   
      
			rt_set_gwroute(rt, rt_key(rt), gwrt);
			VERIFY(rt == hint);
			RT_REMREF_LOCKED(rt);                                  
			RT_UNLOCK(rt);
			lck_mtx_unlock(rnh_lock);
			rt = gwrt;
		} else {
			RT_ADDREF_LOCKED(gwrt);
			RT_UNLOCK(gwrt);
			VERIFY(rt == hint);
			RT_REMREF_LOCKED(rt);                                  
			RT_UNLOCK(rt);
			rt = gwrt;
		}
		VERIFY(rt == gwrt && rt != hint);

		  
                                                            
                                                             
                                                            
                                    
     
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

		                                                          
		if (hint == hint0) {
			RT_REMREF(hint);
		} else {
			rtfree(hint);
		}
		hint = NULL;

		                                            
		RT_LOCK_SPIN(rt);
		if (!(rt->rt_flags & RTF_UP)) {
			RT_UNLOCK(rt);
			senderr(EHOSTUNREACH);
		}
	}

	if (rt->rt_flags & RTF_REJECT) {
		VERIFY(rt->rt_expire == 0 || rt->rt_rmx.rmx_expire != 0);
		VERIFY(rt->rt_expire != 0 || rt->rt_rmx.rmx_expire == 0);
		timenow = net_uptime();
		if (rt->rt_expire == 0 || timenow < rt->rt_expire) {
			RT_UNLOCK(rt);
			senderr(!gwroute ? EHOSTDOWN : EHOSTUNREACH);
		}
	}

	                            
	RT_CONVERT_LOCK(rt);

	                                                
	*out_route = rt;
	return 0;

bad:
	                                                  
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
	return error;
}
#undef senderr

void
rt_revalidate_gwroute(struct rtentry *rt, struct rtentry *gwrt)
{
	VERIFY(gwrt != NULL);

	RT_LOCK_SPIN(rt);
	if ((rt->rt_flags & (RTF_GATEWAY | RTF_UP)) == (RTF_GATEWAY | RTF_UP) &&
	    rt->rt_ifp == gwrt->rt_ifp && rt->rt_gateway->sa_family ==
	    rt_key(gwrt)->sa_family && (rt->rt_gwroute == NULL ||
	    !(rt->rt_gwroute->rt_flags & RTF_UP))) {
		boolean_t isequal;
		VERIFY(rt->rt_flags & (RTF_CLONING | RTF_PRCLONING));

		if (rt->rt_gateway->sa_family == AF_INET ||
		    rt->rt_gateway->sa_family == AF_INET6) {
			struct sockaddr_storage key_ss, gw_ss;
			  
                                                      
                                                         
      
			(void) sa_copy(rt_key(gwrt), &key_ss, NULL);
			(void) sa_copy(rt->rt_gateway, &gw_ss, NULL);

			isequal = equal(SA(&key_ss), SA(&gw_ss));
		} else {
			isequal = equal(rt_key(gwrt), rt->rt_gateway);
		}

		                                       
		if (isequal) {
			RT_UNLOCK(rt);
			lck_mtx_lock(rnh_lock);
			RT_LOCK(rt);
			rt_set_gwroute(rt, rt_key(rt), gwrt);
			RT_UNLOCK(rt);
			lck_mtx_unlock(rnh_lock);
		} else {
			RT_UNLOCK(rt);
		}
	} else {
		RT_UNLOCK(rt);
	}
}

static void
rt_str4(struct rtentry *rt, char *ds, uint32_t dslen, char *gs, uint32_t gslen)
{
	VERIFY(rt_key(rt)->sa_family == AF_INET);

	if (ds != NULL) {
		(void) inet_ntop(AF_INET,
		    &SIN(rt_key(rt))->sin_addr.s_addr, ds, dslen);
		if (dslen >= MAX_SCOPE_ADDR_STR_LEN &&
		    SINIFSCOPE(rt_key(rt))->sin_scope_id != IFSCOPE_NONE) {
			char scpstr[16];

			snprintf(scpstr, sizeof(scpstr), "@%u",
			    SINIFSCOPE(rt_key(rt))->sin_scope_id);

			strlcat(ds, scpstr, dslen);
		}
	}

	if (gs != NULL) {
		if (rt->rt_flags & RTF_GATEWAY) {
			(void) inet_ntop(AF_INET,
			    &SIN(rt->rt_gateway)->sin_addr.s_addr, gs, gslen);
		} else if (rt->rt_ifp != NULL) {
			snprintf(gs, gslen, "link#%u", rt->rt_ifp->if_unit);
		} else {
			snprintf(gs, gslen, "%s", "link");
		}
	}
}

static void
rt_str6(struct rtentry *rt, char *ds, uint32_t dslen, char *gs, uint32_t gslen)
{
	VERIFY(rt_key(rt)->sa_family == AF_INET6);

	if (ds != NULL) {
		(void) inet_ntop(AF_INET6,
		    &SIN6(rt_key(rt))->sin6_addr, ds, dslen);
		if (dslen >= MAX_SCOPE_ADDR_STR_LEN &&
		    SIN6IFSCOPE(rt_key(rt))->sin6_scope_id != IFSCOPE_NONE) {
			char scpstr[16];

			snprintf(scpstr, sizeof(scpstr), "@%u",
			    SIN6IFSCOPE(rt_key(rt))->sin6_scope_id);

			strlcat(ds, scpstr, dslen);
		}
	}

	if (gs != NULL) {
		if (rt->rt_flags & RTF_GATEWAY) {
			(void) inet_ntop(AF_INET6,
			    &SIN6(rt->rt_gateway)->sin6_addr, gs, gslen);
		} else if (rt->rt_ifp != NULL) {
			snprintf(gs, gslen, "link#%u", rt->rt_ifp->if_unit);
		} else {
			snprintf(gs, gslen, "%s", "link");
		}
	}
}

void
rt_str(struct rtentry *rt, char *ds, uint32_t dslen, char *gs, uint32_t gslen)
{
	switch (rt_key(rt)->sa_family) {
	case AF_INET:
		rt_str4(rt, ds, dslen, gs, gslen);
		break;
	case AF_INET6:
		rt_str6(rt, ds, dslen, gs, gslen);
		break;
	default:
		if (ds != NULL) {
			bzero(ds, dslen);
		}
		if (gs != NULL) {
			bzero(gs, gslen);
		}
		break;
	}
}

void
route_event_init(struct route_event *p_route_ev, struct rtentry *rt,
    struct rtentry *gwrt, int route_ev_code)
{
	VERIFY(p_route_ev != NULL);
	bzero(p_route_ev, sizeof(*p_route_ev));

	p_route_ev->rt = rt;
	p_route_ev->gwrt = gwrt;
	p_route_ev->route_event_code = route_ev_code;
}

static void
route_event_callback(void *arg)
{
	struct route_event *p_rt_ev = (struct route_event *)arg;
	struct rtentry *rt = p_rt_ev->rt;
	eventhandler_tag evtag = p_rt_ev->evtag;
	int route_ev_code = p_rt_ev->route_event_code;

	if (route_ev_code == ROUTE_EVHDLR_DEREGISTER) {
		VERIFY(evtag != NULL);
		EVENTHANDLER_DEREGISTER(&rt->rt_evhdlr_ctxt, route_event,
		    evtag);
		rtfree(rt);
		return;
	}

	EVENTHANDLER_INVOKE(&rt->rt_evhdlr_ctxt, route_event, rt_key(rt),
	    route_ev_code, (struct sockaddr *)&p_rt_ev->rt_addr,
	    rt->rt_flags);

	                                                         
	rtfree(rt);
	                                       
}

int
route_event_walktree(struct radix_node *rn, void *arg)
{
	struct route_event *p_route_ev = (struct route_event *)arg;
	struct rtentry *rt = (struct rtentry *)rn;
	struct rtentry *gwrt = p_route_ev->rt;

	LCK_MTX_ASSERT(rnh_lock, LCK_MTX_ASSERT_OWNED);

	RT_LOCK(rt);

	                                            
	if (rt->rt_flags & RTPRF_OURS) {
		RT_UNLOCK(rt);
		return 0;
	}

	                                           
	if (!(rt->rt_flags & RTF_GATEWAY)) {
		RT_UNLOCK(rt);
		return 0;
	}

	if (rt->rt_gwroute != gwrt) {
		RT_UNLOCK(rt);
		return 0;
	}

	route_event_enqueue_nwk_wq_entry(rt, gwrt, p_route_ev->route_event_code,
	    NULL, TRUE);
	RT_UNLOCK(rt);

	return 0;
}

struct route_event_nwk_wq_entry {
	struct nwk_wq_entry nwk_wqe;
	struct route_event rt_ev_arg;
};

void
route_event_enqueue_nwk_wq_entry(struct rtentry *rt, struct rtentry *gwrt,
    uint32_t route_event_code, eventhandler_tag evtag, boolean_t rt_locked)
{
	struct route_event_nwk_wq_entry *p_rt_ev = NULL;
	struct sockaddr *p_gw_saddr = NULL;

	MALLOC(p_rt_ev, struct route_event_nwk_wq_entry *,
	    sizeof(struct route_event_nwk_wq_entry),
	    M_NWKWQ, M_WAITOK | M_ZERO);

	  
                                               
                                                     
                         
    
	if (route_event_code != ROUTE_EVHDLR_DEREGISTER) {
		                                                       
		if (rt_locked) {
			RT_ADDREF_LOCKED(rt);
		} else {
			RT_ADDREF(rt);
		}
	}

	p_rt_ev->rt_ev_arg.rt = rt;
	p_rt_ev->rt_ev_arg.gwrt = gwrt;
	p_rt_ev->rt_ev_arg.evtag = evtag;

	if (gwrt != NULL) {
		p_gw_saddr = gwrt->rt_gateway;
	} else {
		p_gw_saddr = rt->rt_gateway;
	}

	VERIFY(p_gw_saddr->sa_len <= sizeof(p_rt_ev->rt_ev_arg.rt_addr));
	bcopy(p_gw_saddr, &(p_rt_ev->rt_ev_arg.rt_addr), p_gw_saddr->sa_len);

	p_rt_ev->rt_ev_arg.route_event_code = route_event_code;
	p_rt_ev->nwk_wqe.func = route_event_callback;
	p_rt_ev->nwk_wqe.is_arg_managed = TRUE;
	p_rt_ev->nwk_wqe.arg = &p_rt_ev->rt_ev_arg;
	nwk_wq_enqueue((struct nwk_wq_entry*)p_rt_ev);
}

const char *
route_event2str(int route_event)
{
	const char *route_event_str = "ROUTE_EVENT_UNKNOWN";
	switch (route_event) {
	case ROUTE_STATUS_UPDATE:
		route_event_str = "ROUTE_STATUS_UPDATE";
		break;
	case ROUTE_ENTRY_REFRESH:
		route_event_str = "ROUTE_ENTRY_REFRESH";
		break;
	case ROUTE_ENTRY_DELETED:
		route_event_str = "ROUTE_ENTRY_DELETED";
		break;
	case ROUTE_LLENTRY_RESOLVED:
		route_event_str = "ROUTE_LLENTRY_RESOLVED";
		break;
	case ROUTE_LLENTRY_UNREACH:
		route_event_str = "ROUTE_LLENTRY_UNREACH";
		break;
	case ROUTE_LLENTRY_CHANGED:
		route_event_str = "ROUTE_LLENTRY_CHANGED";
		break;
	case ROUTE_LLENTRY_STALE:
		route_event_str = "ROUTE_LLENTRY_STALE";
		break;
	case ROUTE_LLENTRY_TIMEDOUT:
		route_event_str = "ROUTE_LLENTRY_TIMEDOUT";
		break;
	case ROUTE_LLENTRY_DELETED:
		route_event_str = "ROUTE_LLENTRY_DELETED";
		break;
	case ROUTE_LLENTRY_EXPIRED:
		route_event_str = "ROUTE_LLENTRY_EXPIRED";
		break;
	case ROUTE_LLENTRY_PROBED:
		route_event_str = "ROUTE_LLENTRY_PROBED";
		break;
	case ROUTE_EVHDLR_DEREGISTER:
		route_event_str = "ROUTE_EVHDLR_DEREGISTER";
		break;
	default:
		                                   
		break;
	}
	return route_event_str;
}

int
route_op_entitlement_check(struct socket *so,
    kauth_cred_t cred,
    int route_op_type,
    boolean_t allow_root)
{
	if (so != NULL) {
		if (route_op_type == ROUTE_OP_READ) {
			  
                                                 
                                                   
                       
      
			if (soopt_cred_check(so, PRIV_NET_RESTRICTED_ROUTE_NC_READ,
			    allow_root, false) == 0) {
				return 0;
			} else {
				return -1;
			}
		}
	} else if (cred != NULL) {
		uid_t uid = kauth_cred_getuid(cred);

		                       
		if (uid != 0 || !allow_root) {
			if (route_op_type == ROUTE_OP_READ) {
				if (priv_check_cred(cred,
				    PRIV_NET_RESTRICTED_ROUTE_NC_READ, 0) == 0) {
					return 0;
				} else {
					return -1;
				}
			}
		}
	}
	return -1;
}
