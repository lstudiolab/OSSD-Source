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
 *	@(#)in_pcb.c	8.2 (Berkeley) 1/4/94
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>
#include <sys/priv.h>
#include <kern/locks.h>
#include <sys/random.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/restricted_in_port.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>

#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/in6_pcb.h>
#include <netinet6/ip6_var.h>
#include <netinet6/scope6_var.h>
#include <netinet6/nd6.h>

#include <net/net_osdep.h>

#include "loop.h"

SYSCTL_DECL(_net_inet6_ip6);

static int ip6_select_srcif_debug = 0;
SYSCTL_INT(_net_inet6_ip6, OID_AUTO, select_srcif_debug,
    CTLFLAG_RW | CTLFLAG_LOCKED, &ip6_select_srcif_debug, 0,
    "log source interface selection debug info");

static int ip6_select_srcaddr_debug = 0;
SYSCTL_INT(_net_inet6_ip6, OID_AUTO, select_srcaddr_debug,
    CTLFLAG_RW | CTLFLAG_LOCKED, &ip6_select_srcaddr_debug, 0,
    "log source address selection debug info");

static int ip6_select_src_expensive_secondary_if = 0;
SYSCTL_INT(_net_inet6_ip6, OID_AUTO, select_src_expensive_secondary_if,
    CTLFLAG_RW | CTLFLAG_LOCKED, &ip6_select_src_expensive_secondary_if, 0,
    "allow source interface selection to use expensive secondaries");

static int ip6_select_src_strong_end = 1;
SYSCTL_INT(_net_inet6_ip6, OID_AUTO, select_src_strong_end,
    CTLFLAG_RW | CTLFLAG_LOCKED, &ip6_select_src_strong_end, 0,
    "limit source address selection to outgoing interface");

#define ADDR_LABEL_NOTAPP (-1)
struct in6_addrpolicy defaultaddrpolicy;

int ip6_prefer_tempaddr = 1;

int ip6_cga_conflict_retries = IPV6_CGA_CONFLICT_RETRIES_DEFAULT;

#ifdef ENABLE_ADDRSEL
extern lck_mtx_t *addrsel_mutex;
#define ADDRSEL_LOCK()          lck_mtx_lock(addrsel_mutex)
#define ADDRSEL_UNLOCK()        lck_mtx_unlock(addrsel_mutex)
#else
#define ADDRSEL_LOCK()
#define ADDRSEL_UNLOCK()
#endif
extern int      udp_use_randomport;
extern int      tcp_use_randomport;

static int selectroute(struct sockaddr_in6 *, struct sockaddr_in6 *,
    struct ip6_pktopts *, struct ip6_moptions *, struct in6_ifaddr **,
    struct route_in6 *, struct ifnet **, struct rtentry **, int, int,
    struct ip6_out_args *ip6oa);
static int in6_selectif(struct sockaddr_in6 *, struct ip6_pktopts *,
    struct ip6_moptions *, struct route_in6 *ro,
    struct ip6_out_args *, struct ifnet **);
static void init_policy_queue(void);
static int add_addrsel_policyent(const struct in6_addrpolicy *);
#ifdef ENABLE_ADDRSEL
static int delete_addrsel_policyent(const struct in6_addrpolicy *);
#endif
static int walk_addrsel_policy(int (*)(const struct in6_addrpolicy *, void *),
    void *);
static int dump_addrsel_policyent(const struct in6_addrpolicy *, void *);
static struct in6_addrpolicy *match_addrsel_policy(struct sockaddr_in6 *);
void addrsel_policy_init(void);

#define SASEL_DO_DBG(inp) \
	(ip6_select_srcaddr_debug && (inp) != NULL && \
	    (inp)->inp_socket != NULL && \
	    ((inp)->inp_socket->so_options & SO_DEBUG))

#define SASEL_LOG(fmt, ...) \
do { \
	if (srcsel_debug) \
	        printf("%s:%d " fmt "\n",\
	            __FUNCTION__, __LINE__, ##__VA_ARGS__); \
} while (0); \

  
                                                                    
                                          
                                                                    
                                        
   
#define REPLACE(r) do {\
	SASEL_LOG("REPLACE r %d ia %s ifp1 %s\n", \
	    (r), s_src, ifp1->if_xname); \
	srcrule = (r); \
	goto replace; \
} while (0)

#define NEXTSRC(r) do {\
	SASEL_LOG("NEXTSRC r %d ia %s ifp1 %s\n", \
	    (r), s_src, ifp1->if_xname); \
	goto next;                                                      \
} while (0)

#define BREAK(r) do { \
	SASEL_LOG("BREAK r %d ia %s ifp1 %s\n", \
	    (r), s_src, ifp1->if_xname); \
	srcrule = (r); \
	goto out;                                                    \
} while (0)


struct ifaddr *
in6_selectsrc_core_ifa(struct sockaddr_in6 *addr, struct ifnet *ifp, int srcsel_debug)
{
	int err = 0;
	struct ifnet *src_ifp = NULL;
	struct in6_addr src_storage = {};
	struct in6_addr *in6 = NULL;
	struct ifaddr *ifa = NULL;

	if ((in6 = in6_selectsrc_core(addr,
	    (ip6_prefer_tempaddr ? IPV6_SRCSEL_HINT_PREFER_TMPADDR : 0),
	    ifp, 0, &src_storage, &src_ifp, &err, &ifa)) == NULL) {
		if (err == 0) {
			err = EADDRNOTAVAIL;
		}
		VERIFY(src_ifp == NULL);
		if (ifa != NULL) {
			IFA_REMREF(ifa);
			ifa = NULL;
		}
		goto done;
	}

	if (src_ifp != ifp) {
		if (err == 0) {
			err = ENETUNREACH;
		}
		if (ifa != NULL) {
			IFA_REMREF(ifa);
			ifa = NULL;
		}
		goto done;
	}

	VERIFY(ifa != NULL);
	ifnet_lock_shared(ifp);
	if ((ifa->ifa_debug & IFD_DETACHING) != 0) {
		err = EHOSTUNREACH;
		ifnet_lock_done(ifp);
		IFA_REMREF(ifa);
		ifa = NULL;
		goto done;
	}
	ifnet_lock_done(ifp);

done:
	SASEL_LOG("Returned with error: %d", err);
	if (src_ifp != NULL) {
		ifnet_release(src_ifp);
	}
	return ifa;
}

struct in6_addr *
in6_selectsrc_core(struct sockaddr_in6 *dstsock, uint32_t hint_mask,
    struct ifnet *ifp, int srcsel_debug, struct in6_addr *src_storage,
    struct ifnet **sifp, int *errorp, struct ifaddr **ifapp)
{
	u_int32_t odstzone;
	int bestrule = IP6S_SRCRULE_0;
	struct in6_addrpolicy *dst_policy = NULL, *best_policy = NULL;
	struct in6_addr dst;
	struct in6_ifaddr *ia = NULL, *ia_best = NULL;
	char s_src[MAX_IPv6_STR_LEN] = {0};
	char s_dst[MAX_IPv6_STR_LEN] = {0};
	const struct in6_addr *tmp = NULL;
	int dst_scope = -1, best_scope = -1, best_matchlen = -1;
	uint64_t secs = net_uptime();
	VERIFY(dstsock != NULL);
	VERIFY(src_storage != NULL);
	VERIFY(ifp != NULL);

	if (sifp != NULL) {
		*sifp = NULL;
	}

	if (ifapp != NULL) {
		*ifapp = NULL;
	}

	dst = dstsock->sin6_addr;                                      

	if (srcsel_debug) {
		(void) inet_ntop(AF_INET6, &dst, s_dst, sizeof(s_src));

		tmp = &in6addr_any;
		(void) inet_ntop(AF_INET6, tmp, s_src, sizeof(s_src));
		printf("%s out src %s dst %s ifp %s\n",
		    __func__, s_src, s_dst, ifp->if_xname);
	}

	*errorp = in6_setscope(&dst, ifp, &odstzone);
	if (*errorp != 0) {
		src_storage = NULL;
		goto done;
	}

	lck_rw_lock_shared(&in6_ifaddr_rwlock);
	TAILQ_FOREACH(ia, &in6_ifaddrhead, ia6_link) {
		int new_scope = -1, new_matchlen = -1;
		struct in6_addrpolicy *new_policy = NULL;
		u_int32_t srczone = 0, osrczone, dstzone;
		struct in6_addr src;
		struct ifnet *ifp1 = ia->ia_ifp;
		int srcrule;

		if (srcsel_debug) {
			(void) inet_ntop(AF_INET6, &ia->ia_addr.sin6_addr,
			    s_src, sizeof(s_src));
		}

		IFA_LOCK(&ia->ia_ifa);

		  
                                              
     
		if (ia->ia6_flags & IN6_IFF_CLAT46) {
			SASEL_LOG("NEXT ia %s address on ifp1 %s skipped as it is "
			    "reserved for CLAT46", s_src, ifp1->if_xname);
			goto next;
		}

		  
                                                     
                                                      
                                          
     
		if (ip6_select_src_strong_end &&
		    ifp1 != ifp) {
			SASEL_LOG("NEXT ia %s ifp1 %s address is not on outgoing "
			    "interface \n", s_src, ifp1->if_xname);
			goto next;
		}

		  
                                                           
                                                            
                                             
                                                    
     
		if (in6_setscope(&dst, ifp1, &dstzone) ||
		    odstzone != dstzone) {
			SASEL_LOG("NEXT ia %s ifp1 %s odstzone %d != dstzone %d\n",
			    s_src, ifp1->if_xname, odstzone, dstzone);
			goto next;
		}
		src = ia->ia_addr.sin6_addr;
		if (in6_setscope(&src, ifp, &osrczone) ||
		    in6_setscope(&src, ifp1, &srczone) ||
		    osrczone != srczone) {
			SASEL_LOG("NEXT ia %s ifp1 %s osrczone %d != srczone %d\n",
			    s_src, ifp1->if_xname, osrczone, srczone);
			goto next;
		}
		                              
		if ((ia->ia6_flags &
		    (IN6_IFF_NOTREADY | IN6_IFF_ANYCAST | IN6_IFF_DETACHED))) {
			SASEL_LOG("NEXT ia %s ifp1 %s ia6_flags 0x%x\n",
			    s_src, ifp1->if_xname, ia->ia6_flags);
			goto next;
		}
		if (!ip6_use_deprecated && IFA6_IS_DEPRECATED(ia, secs)) {
			SASEL_LOG("NEXT ia %s ifp1 %s IFA6_IS_DEPRECATED\n",
			    s_src, ifp1->if_xname);
			goto next;
		}
		if (!nd6_optimistic_dad &&
		    (ia->ia6_flags & IN6_IFF_OPTIMISTIC) != 0) {
			SASEL_LOG("NEXT ia %s ifp1 %s IN6_IFF_OPTIMISTIC\n",
			    s_src, ifp1->if_xname);
			goto next;
		}
		                                 
		if (IN6_ARE_ADDR_EQUAL(&dst, &ia->ia_addr.sin6_addr)) {
			BREAK(IP6S_SRCRULE_1);                                          
		}
		if (ia_best == NULL) {
			REPLACE(IP6S_SRCRULE_0);
		}

		                                      
		if (dst_scope < 0) {
			dst_scope = in6_addrscope(&dst);
		}
		new_scope = in6_addrscope(&ia->ia_addr.sin6_addr);
		if (IN6_ARE_SCOPE_CMP(best_scope, new_scope) < 0) {
			if (IN6_ARE_SCOPE_CMP(best_scope, dst_scope) < 0) {
				REPLACE(IP6S_SRCRULE_2);
			}
			NEXTSRC(IP6S_SRCRULE_2);
		} else if (IN6_ARE_SCOPE_CMP(new_scope, best_scope) < 0) {
			if (IN6_ARE_SCOPE_CMP(new_scope, dst_scope) < 0) {
				NEXTSRC(IP6S_SRCRULE_2);
			}
			REPLACE(IP6S_SRCRULE_2);
		}

		  
                                                               
                                                   
     
		if (!IFA6_IS_DEPRECATED(ia_best, secs) &&
		    IFA6_IS_DEPRECATED(ia, secs)) {
			NEXTSRC(IP6S_SRCRULE_3);
		}
		if (IFA6_IS_DEPRECATED(ia_best, secs) &&
		    !IFA6_IS_DEPRECATED(ia, secs)) {
			REPLACE(IP6S_SRCRULE_3);
		}

		  
                                                              
                                              
     
		if ((ia_best->ia6_flags & IN6_IFF_OPTIMISTIC) == 0 &&
		    (ia->ia6_flags & IN6_IFF_OPTIMISTIC) != 0) {
			NEXTSRC(IP6S_SRCRULE_3);
		}
		if ((ia_best->ia6_flags & IN6_IFF_OPTIMISTIC) != 0 &&
		    (ia->ia6_flags & IN6_IFF_OPTIMISTIC) == 0) {
			REPLACE(IP6S_SRCRULE_3);
		}

		                                   
		  
                                                            
                
     

		                                       
		  
                                                         
                                                          
                                                                
                                          
     
		if (ip6_select_src_strong_end == 0) {
			if (ia_best->ia_ifp == ifp && ia->ia_ifp != ifp) {
				NEXTSRC(IP6S_SRCRULE_5);
			}
			if (ia_best->ia_ifp != ifp && ia->ia_ifp == ifp) {
				REPLACE(IP6S_SRCRULE_5);
			}
		}

		  
                                  
                                                   
     
		if (dst_policy == NULL) {
			dst_policy = in6_addrsel_lookup_policy(dstsock);
		}
		if (dst_policy->label != ADDR_LABEL_NOTAPP) {
			new_policy = in6_addrsel_lookup_policy(&ia->ia_addr);
			if (dst_policy->label == best_policy->label &&
			    dst_policy->label != new_policy->label) {
				NEXTSRC(IP6S_SRCRULE_6);
			}
			if (dst_policy->label != best_policy->label &&
			    dst_policy->label == new_policy->label) {
				REPLACE(IP6S_SRCRULE_6);
			}
		}

		  
                                        
                                                       
                                                                
                                    
     
		if (!(ia_best->ia6_flags & IN6_IFF_TEMPORARY) &&
		    (ia->ia6_flags & IN6_IFF_TEMPORARY)) {
			if (hint_mask & IPV6_SRCSEL_HINT_PREFER_TMPADDR) {
				REPLACE(IP6S_SRCRULE_7);
			} else {
				NEXTSRC(IP6S_SRCRULE_7);
			}
		}
		if ((ia_best->ia6_flags & IN6_IFF_TEMPORARY) &&
		    !(ia->ia6_flags & IN6_IFF_TEMPORARY)) {
			if (hint_mask & IPV6_SRCSEL_HINT_PREFER_TMPADDR) {
				NEXTSRC(IP6S_SRCRULE_7);
			} else {
				REPLACE(IP6S_SRCRULE_7);
			}
		}

		  
                                                   
                                  
     
		if ((ia_best->ia_ifp->if_flags & IFF_UP) &&
		    !(ia->ia_ifp->if_flags & IFF_UP)) {
			NEXTSRC(IP6S_SRCRULE_7x);
		}
		if (!(ia_best->ia_ifp->if_flags & IFF_UP) &&
		    (ia->ia_ifp->if_flags & IFF_UP)) {
			REPLACE(IP6S_SRCRULE_7x);
		}

		  
                                         
     
		new_matchlen = in6_matchlen(&ia->ia_addr.sin6_addr, &dst);
		if (best_matchlen < new_matchlen) {
			REPLACE(IP6S_SRCRULE_8);
		}
		if (new_matchlen < best_matchlen) {
			NEXTSRC(IP6S_SRCRULE_8);
		}

		  
                                                  
                               
     
		if (ifp1 != ifp && (ifp1->if_eflags & IFEF_EXPENSIVE) &&
		    ip6_select_src_expensive_secondary_if == 0) {
			SASEL_LOG("NEXT ia %s ifp1 %s IFEF_EXPENSIVE\n",
			    s_src, ifp1->if_xname);
			ip6stat.ip6s_sources_skip_expensive_secondary_if++;
			goto next;
		}
		SASEL_LOG("NEXT ia %s ifp1 %s last resort\n",
		    s_src, ifp1->if_xname);
		IFA_UNLOCK(&ia->ia_ifa);
		continue;

replace:
		  
                                                             
              
     
		if (ifp1 != ifp && (ifp1->if_eflags & IFEF_EXPENSIVE) &&
		    ip6_select_src_expensive_secondary_if == 0) {
			SASEL_LOG("NEXT ia %s ifp1 %s IFEF_EXPENSIVE\n",
			    s_src, ifp1->if_xname);
			ip6stat.ip6s_sources_skip_expensive_secondary_if++;
			goto next;
		}
		bestrule = srcrule;
		best_scope = (new_scope >= 0 ? new_scope :
		    in6_addrscope(&ia->ia_addr.sin6_addr));
		best_policy = (new_policy ? new_policy :
		    in6_addrsel_lookup_policy(&ia->ia_addr));
		best_matchlen = (new_matchlen >= 0 ? new_matchlen :
		    in6_matchlen(&ia->ia_addr.sin6_addr, &dst));
		SASEL_LOG("NEXT ia %s ifp1 %s best_scope %d new_scope %d dst_scope %d\n",
		    s_src, ifp1->if_xname, best_scope, new_scope, dst_scope);
		IFA_ADDREF_LOCKED(&ia->ia_ifa);                  
		IFA_UNLOCK(&ia->ia_ifa);
		if (ia_best != NULL) {
			IFA_REMREF(&ia_best->ia_ifa);
		}
		ia_best = ia;
		continue;

next:
		IFA_UNLOCK(&ia->ia_ifa);
		continue;

out:
		IFA_ADDREF_LOCKED(&ia->ia_ifa);                  
		IFA_UNLOCK(&ia->ia_ifa);
		if (ia_best != NULL) {
			IFA_REMREF(&ia_best->ia_ifa);
		}
		ia_best = ia;
		break;
	}

	lck_rw_done(&in6_ifaddr_rwlock);

	if ((ia = ia_best) == NULL) {
		if (*errorp == 0) {
			*errorp = EADDRNOTAVAIL;
		}
		src_storage = NULL;
		goto done;
	}

	if (sifp != NULL) {
		*sifp = ia->ia_ifa.ifa_ifp;
		ifnet_reference(*sifp);
	}

	IFA_LOCK_SPIN(&ia->ia_ifa);
	if (bestrule < IP6S_SRCRULE_COUNT) {
		ip6stat.ip6s_sources_rule[bestrule]++;
	}
	*src_storage = satosin6(&ia->ia_addr)->sin6_addr;
	IFA_UNLOCK(&ia->ia_ifa);

	if (ifapp != NULL) {
		*ifapp = &ia->ia_ifa;
	} else {
		IFA_REMREF(&ia->ia_ifa);
	}

done:
	if (srcsel_debug) {
		(void) inet_ntop(AF_INET6, &dst, s_dst, sizeof(s_src));

		tmp = (src_storage != NULL) ? src_storage : &in6addr_any;
		(void) inet_ntop(AF_INET6, tmp, s_src, sizeof(s_src));

		printf("%s out src %s dst %s dst_scope %d best_scope %d\n",
		    __func__, s_src, s_dst, dst_scope, best_scope);
	}

	return src_storage;
}

  
                                                                          
                                                                           
                                                                       
   
struct in6_addr *
in6_selectsrc(struct sockaddr_in6 *dstsock, struct ip6_pktopts *opts,
    struct inpcb *inp, struct route_in6 *ro,
    struct ifnet **ifpp, struct in6_addr *src_storage, unsigned int ifscope,
    int *errorp)
{
	struct ifnet *ifp = NULL;
	struct in6_pktinfo *pi = NULL;
	struct ip6_moptions *mopts;
	struct ip6_out_args ip6oa;
	boolean_t inp_debug = FALSE;
	uint32_t hint_mask = 0;
	int prefer_tempaddr = 0;
	struct ifnet *sifp = NULL;

	bzero(&ip6oa, sizeof(ip6oa));
	ip6oa.ip6oa_boundif = ifscope;
	ip6oa.ip6oa_flags = IP6OAF_SELECT_SRCIF;
	ip6oa.ip6oa_sotc = SO_TC_UNSPEC;
	ip6oa.ip6oa_netsvctype = _NET_SERVICE_TYPE_UNSPEC;

	*errorp = 0;
	if (ifpp != NULL) {
		*ifpp = NULL;
	}

	if (inp != NULL) {
		inp_debug = SASEL_DO_DBG(inp);
		mopts = inp->in6p_moptions;
		if (INP_NO_CELLULAR(inp)) {
			ip6oa.ip6oa_flags |= IP6OAF_NO_CELLULAR;
		}
		if (INP_NO_EXPENSIVE(inp)) {
			ip6oa.ip6oa_flags |= IP6OAF_NO_EXPENSIVE;
		}
		if (INP_NO_CONSTRAINED(inp)) {
			ip6oa.ip6oa_flags |= IP6OAF_NO_CONSTRAINED;
		}
		if (INP_AWDL_UNRESTRICTED(inp)) {
			ip6oa.ip6oa_flags |= IP6OAF_AWDL_UNRESTRICTED;
		}
		if (INP_INTCOPROC_ALLOWED(inp)) {
			ip6oa.ip6oa_flags |= IP6OAF_INTCOPROC_ALLOWED;
		}
	} else {
		mopts = NULL;
		                                             
		ip6oa.ip6oa_flags |= IP6OAF_INTCOPROC_ALLOWED |
		    IP6OAF_AWDL_UNRESTRICTED;
	}

	if (ip6oa.ip6oa_boundif != IFSCOPE_NONE) {
		ip6oa.ip6oa_flags |= IP6OAF_BOUND_IF;
	}

	  
                                                                
                                                                     
                                                                
                                                               
    
	if (opts && (pi = opts->ip6po_pktinfo) &&
	    !IN6_IS_ADDR_UNSPECIFIED(&pi->ipi6_addr)) {
		struct sockaddr_in6 srcsock;
		struct in6_ifaddr *ia6;

		                                
		if ((*errorp = in6_selectif(dstsock, opts, mopts, ro, &ip6oa,
		    &ifp)) != 0) {
			src_storage = NULL;
			goto done;
		}

		  
                                                             
                                                            
                                                              
                                                                 
                                    
     
		bzero(&srcsock, sizeof(srcsock));
		srcsock.sin6_family = AF_INET6;
		srcsock.sin6_len = sizeof(srcsock);
		srcsock.sin6_addr = pi->ipi6_addr;
		if (ifp != NULL) {
			*errorp = in6_setscope(&srcsock.sin6_addr, ifp, NULL);
			if (*errorp != 0) {
				src_storage = NULL;
				goto done;
			}
		}
		ia6 = (struct in6_ifaddr *)ifa_ifwithaddr((struct sockaddr *)
		    (&srcsock));
		if (ia6 == NULL) {
			*errorp = EADDRNOTAVAIL;
			src_storage = NULL;
			goto done;
		}
		IFA_LOCK_SPIN(&ia6->ia_ifa);
		if ((ia6->ia6_flags & (IN6_IFF_ANYCAST | IN6_IFF_NOTREADY | IN6_IFF_CLAT46)) ||
		    (inp && inp_restricted_send(inp, ia6->ia_ifa.ifa_ifp))) {
			IFA_UNLOCK(&ia6->ia_ifa);
			IFA_REMREF(&ia6->ia_ifa);
			*errorp = EHOSTUNREACH;
			src_storage = NULL;
			goto done;
		}

		*src_storage = satosin6(&ia6->ia_addr)->sin6_addr;
		IFA_UNLOCK(&ia6->ia_ifa);
		IFA_REMREF(&ia6->ia_ifa);
		goto done;
	}

	  
                                                                       
    
	if (inp != NULL && !IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_laddr)) {
		src_storage = &inp->in6p_laddr;
		goto done;
	}

	  
                                                                 
                                                       
    
	                                
	if ((*errorp = in6_selectif(dstsock, opts, mopts, ro, &ip6oa,
	    &ifp)) != 0) {
		src_storage = NULL;
		goto done;
	}

	VERIFY(ifp != NULL);

	if (opts == NULL ||
	    opts->ip6po_prefer_tempaddr == IP6PO_TEMPADDR_SYSTEM) {
		prefer_tempaddr = ip6_prefer_tempaddr;
	} else if (opts->ip6po_prefer_tempaddr == IP6PO_TEMPADDR_NOTPREFER) {
		prefer_tempaddr = 0;
	} else {
		prefer_tempaddr = 1;
	}

	if (prefer_tempaddr) {
		hint_mask |= IPV6_SRCSEL_HINT_PREFER_TMPADDR;
	}

	if (in6_selectsrc_core(dstsock, hint_mask, ifp, inp_debug, src_storage,
	    &sifp, errorp, NULL) == NULL) {
		src_storage = NULL;
		goto done;
	}

	VERIFY(sifp != NULL);

	if (inp && inp_restricted_send(inp, sifp)) {
		src_storage = NULL;
		*errorp = EHOSTUNREACH;
		ifnet_release(sifp);
		goto done;
	} else {
		ifnet_release(sifp);
	}

done:
	if (ifpp != NULL) {
		                                                       
		*ifpp = ifp;
	} else if (ifp != NULL) {
		ifnet_release(ifp);
	}
	return src_storage;
}

  
                                                                            
                                                                      
                                                                        
                                                                       
                                                                             
                                                                           
                  
  
                                                                         
                                                                            
                                                                    
                                                                
  
                                               
   
static int
selectroute(struct sockaddr_in6 *srcsock, struct sockaddr_in6 *dstsock,
    struct ip6_pktopts *opts, struct ip6_moptions *mopts,
    struct in6_ifaddr **retsrcia, struct route_in6 *ro,
    struct ifnet **retifp, struct rtentry **retrt, int clone,
    int norouteok, struct ip6_out_args *ip6oa)
{
	int error = 0;
	struct ifnet *ifp = NULL, *ifp0 = NULL;
	struct route_in6 *route = NULL;
	struct sockaddr_in6 *sin6_next;
	struct in6_pktinfo *pi = NULL;
	struct in6_addr *dst = &dstsock->sin6_addr;
	struct ifaddr *ifa = NULL;
	char s_src[MAX_IPv6_STR_LEN], s_dst[MAX_IPv6_STR_LEN];
	boolean_t select_srcif, proxied_ifa = FALSE, local_dst = FALSE;
	unsigned int ifscope = ((ip6oa != NULL) ?
	    ip6oa->ip6oa_boundif : IFSCOPE_NONE);
	boolean_t is_direct = FALSE;

	if (retifp != NULL) {
		*retifp = NULL;
	}

	if (retrt != NULL) {
		*retrt = NULL;
	}

	if (ip6_select_srcif_debug) {
		struct in6_addr src;
		src = (srcsock != NULL) ? srcsock->sin6_addr : in6addr_any;
		(void) inet_ntop(AF_INET6, &src, s_src, sizeof(s_src));
		(void) inet_ntop(AF_INET6, dst, s_dst, sizeof(s_dst));
	}

	  
                                                             
    
	if (IN6_IS_ADDR_UNSPECIFIED(dst)) {
		error = EHOSTUNREACH;
		goto done;
	}

	  
                                                        
                                                           
    
	select_srcif = (srcsock != NULL &&
	    !IN6_IS_ADDR_UNSPECIFIED(&srcsock->sin6_addr));

	  
                                                                            
                                                                                 
                                             
    
	if (ifscope != IFSCOPE_NONE &&
	    !(srcsock != NULL && IN6_IS_ADDR_LINKLOCAL(&srcsock->sin6_addr)) &&
	    !IN6_IS_ADDR_MULTICAST(dst) && !IN6_IS_ADDR_LINKLOCAL(dst)) {
		struct rtentry *temp_rt = NULL;

		lck_mtx_lock(rnh_lock);
		temp_rt = rt_lookup(TRUE, (struct sockaddr *)dstsock,
		    NULL, rt_tables[AF_INET6], ifscope);
		lck_mtx_unlock(rnh_lock);

		  
                                                    
                                                       
                                                            
                        
                                                    
                                                        
                                                       
               
     
		if (temp_rt != NULL) {
			if ((temp_rt->rt_flags & RTF_GATEWAY) == 0) {
				select_srcif = FALSE;
				is_direct = TRUE;
			}
			rtfree(temp_rt);
		}
	}

	if (ip6_select_srcif_debug) {
		printf("%s src %s dst %s ifscope %d is_direct %d select_srcif %d\n",
		    __func__, s_src, s_dst, ifscope, is_direct, select_srcif);
	}

	                                                                       
	if (opts != NULL && (pi = opts->ip6po_pktinfo) != NULL &&
	    pi->ipi6_ifindex != 0) {
		  
                                                         
     
		ifscope = pi->ipi6_ifindex;
		ifnet_head_lock_shared();
		                                                 
		ifp = ifp0 =
		    ((ifscope <= if_index) ? ifindex2ifnet[ifscope] : NULL);
		ifnet_head_done();
		if (norouteok || retrt == NULL || IN6_IS_ADDR_MC_LINKLOCAL(dst)) {
			  
                                                  
                                                   
                                                
                    
      
			if (ifp == NULL) {
				error = EHOSTUNREACH;
			}
			goto done;
		} else {
			goto getsrcif;
		}
	}

	  
                                                                      
                                                                 
    
	if (IN6_IS_ADDR_MULTICAST(dst) && mopts != NULL) {
		IM6O_LOCK(mopts);
		ifp = ifp0 = mopts->im6o_multicast_ifp;
		if (ifp != NULL && IN6_IS_ADDR_MC_LINKLOCAL(dst)) {
			IM6O_UNLOCK(mopts);
			goto done;                                                     
		}
		IM6O_UNLOCK(mopts);
	}

getsrcif:
	  
                                                              
                                                              
    
	if (ifscope == IFSCOPE_NONE) {
		ifscope = dstsock->sin6_scope_id;
	}

	  
                                                               
                                                             
                                                           
                                                           
                                                            
                                         
    
	if (!select_srcif) {
		goto getroute;
	} else if (!ROUTE_UNUSABLE(ro) && ro->ro_srcia != NULL &&
	    (ro->ro_flags & ROF_SRCIF_SELECTED)) {
		if (ro->ro_rt->rt_ifp->if_flags & IFF_LOOPBACK) {
			local_dst = TRUE;
		}
		ifa = ro->ro_srcia;
		IFA_ADDREF(ifa);                        
		goto getroute;
	}

	  
                                                                   
                                                              
                                                                 
                                                                
                                    
    
	if (ifscope != IFSCOPE_NONE || (ro != NULL && ro->ro_rt != NULL)) {
		unsigned int scope = ifscope;
		struct ifnet *rt_ifp;

		rt_ifp = (ro->ro_rt != NULL) ? ro->ro_rt->rt_ifp : NULL;

		  
                                                              
                                                               
                                                              
                                                             
                                                             
                                                              
                                               
     
		if (scope == IFSCOPE_NONE) {
			scope = rt_ifp->if_index;
			if (scope != get_primary_ifscope(AF_INET6) &&
			    ROUTE_UNUSABLE(ro)) {
				scope = get_primary_ifscope(AF_INET6);
			}
		}

		ifa = (struct ifaddr *)
		    ifa_foraddr6_scoped(&srcsock->sin6_addr, scope);

		  
                                                             
                                                            
                   
     
		if (ifa == NULL && ip6_forwarding && nd6_prproxy) {
			ifa = (struct ifaddr *)
			    ifa_foraddr6(&srcsock->sin6_addr);
			if (ifa != NULL && !(proxied_ifa =
			    nd6_prproxy_ifaddr((struct in6_ifaddr *)ifa))) {
				IFA_REMREF(ifa);
				ifa = NULL;
			}
		}

		if (ip6_select_srcif_debug && ifa != NULL) {
			if (ro->ro_rt != NULL) {
				printf("%s %s->%s ifscope %d->%d ifa_if %s "
				    "ro_if %s\n",
				    __func__,
				    s_src, s_dst, ifscope,
				    scope, if_name(ifa->ifa_ifp),
				    if_name(rt_ifp));
			} else {
				printf("%s %s->%s ifscope %d->%d ifa_if %s\n",
				    __func__,
				    s_src, s_dst, ifscope, scope,
				    if_name(ifa->ifa_ifp));
			}
		}
	}

	  
                                                                      
                                                                   
   
                                             
                                                                 
                                                             
                                                              
                    
    
	if (ifa == NULL && ifscope == IFSCOPE_NONE) {
		struct ifaddr *ifadst;

		                                                     
		ifadst = (struct ifaddr *)ifa_foraddr6(&dstsock->sin6_addr);
		if (ifadst != NULL) {
			local_dst = TRUE;
			IFA_REMREF(ifadst);
		}

		ifa = (struct ifaddr *)ifa_foraddr6(&srcsock->sin6_addr);

		if (ip6_select_srcif_debug && ifa != NULL) {
			printf("%s %s->%s ifscope %d ifa_if %s\n",
			    __func__,
			    s_src, s_dst, ifscope, if_name(ifa->ifa_ifp));
		} else if (ip6_select_srcif_debug) {
			printf("%s %s->%s ifscope %d ifa_if NULL\n",
			    __func__,
			    s_src, s_dst, ifscope);
		}
	}

getroute:
	if (ifa != NULL && !proxied_ifa && !local_dst) {
		ifscope = ifa->ifa_ifp->if_index;
	}

	  
                                                                      
                          
    
	if (opts != NULL && opts->ip6po_nexthop != NULL) {
		struct route_in6 *ron;

		sin6_next = satosin6(opts->ip6po_nexthop);

		                                                        
		if (sin6_next->sin6_family != AF_INET6) {
			error = EAFNOSUPPORT;                            
			goto done;
		}

		  
                                                                 
                                                            
     
		ron = &opts->ip6po_nextroute;
		if (ron->ro_rt != NULL) {
			RT_LOCK(ron->ro_rt);
		}
		if (ROUTE_UNUSABLE(ron) || (ron->ro_rt != NULL &&
		    (!(ron->ro_rt->rt_flags & RTF_LLINFO) ||
		    (select_srcif && (ifa == NULL ||
		    (ifa->ifa_ifp != ron->ro_rt->rt_ifp && !proxied_ifa))))) ||
		    !IN6_ARE_ADDR_EQUAL(&satosin6(&ron->ro_dst)->sin6_addr,
		    &sin6_next->sin6_addr)) {
			if (ron->ro_rt != NULL) {
				RT_UNLOCK(ron->ro_rt);
			}

			ROUTE_RELEASE(ron);
			*satosin6(&ron->ro_dst) = *sin6_next;
		}
		if (ron->ro_rt == NULL) {
			rtalloc_scoped((struct route *)ron, ifscope);
			if (ron->ro_rt != NULL) {
				RT_LOCK(ron->ro_rt);
			}
			if (ROUTE_UNUSABLE(ron) ||
			    !(ron->ro_rt->rt_flags & RTF_LLINFO) ||
			    !IN6_ARE_ADDR_EQUAL(&satosin6(rt_key(ron->ro_rt))->
			    sin6_addr, &sin6_next->sin6_addr)) {
				if (ron->ro_rt != NULL) {
					RT_UNLOCK(ron->ro_rt);
				}

				ROUTE_RELEASE(ron);
				error = EHOSTUNREACH;
				goto done;
			}
		}
		route = ron;
		ifp = ifp0 = ron->ro_rt->rt_ifp;

		  
                                                             
                                                      
                 
     
		if (!clone) {
			if (select_srcif) {
				                           
				goto validateroute;
			}
			RT_UNLOCK(ron->ro_rt);
			goto done;
		}
		RT_UNLOCK(ron->ro_rt);
	}

	  
                                                                      
                                                                   
                                                               
    
	if (ro == NULL) {
		goto done;
	}
	if (ro->ro_rt != NULL) {
		RT_LOCK_SPIN(ro->ro_rt);
	}
	if (ROUTE_UNUSABLE(ro) || (ro->ro_rt != NULL &&
	    (satosin6(&ro->ro_dst)->sin6_family != AF_INET6 ||
	    !IN6_ARE_ADDR_EQUAL(&satosin6(&ro->ro_dst)->sin6_addr, dst) ||
	    (select_srcif && (ifa == NULL ||
	    (ifa->ifa_ifp != ro->ro_rt->rt_ifp && !proxied_ifa)))))) {
		if (ro->ro_rt != NULL) {
			RT_UNLOCK(ro->ro_rt);
		}

		ROUTE_RELEASE(ro);
	}
	if (ro->ro_rt == NULL) {
		struct sockaddr_in6 *sa6;

		                                         
		bzero(&ro->ro_dst, sizeof(struct sockaddr_in6));
		sa6 = (struct sockaddr_in6 *)&ro->ro_dst;
		sa6->sin6_family = AF_INET6;
		sa6->sin6_len = sizeof(struct sockaddr_in6);
		sa6->sin6_addr = *dst;
		if (IN6_IS_ADDR_MC_LINKLOCAL(dst)) {
			ro->ro_rt = rtalloc1_scoped(
				&((struct route *)ro)->ro_dst, 0, 0, ifscope);
		} else {
			rtalloc_scoped((struct route *)ro, ifscope);
		}
		if (ro->ro_rt != NULL) {
			RT_LOCK_SPIN(ro->ro_rt);
		}
	}

	  
                                                       
                                                        
    
	if (opts != NULL && opts->ip6po_nexthop != NULL) {
		if (ro->ro_rt != NULL) {
			RT_UNLOCK(ro->ro_rt);
		}
		goto done;
	}

	if (ro->ro_rt != NULL) {
		RT_LOCK_ASSERT_HELD(ro->ro_rt);
		ifp = ifp0 = ro->ro_rt->rt_ifp;
	} else {
		error = EHOSTUNREACH;
	}
	route = ro;

validateroute:
	if (select_srcif) {
		boolean_t has_route = (route != NULL && route->ro_rt != NULL);
		boolean_t srcif_selected = FALSE;

		if (has_route) {
			RT_LOCK_ASSERT_HELD(route->ro_rt);
		}
		  
                                                               
                                                                 
                                                              
                                           
     
		if (has_route && (ifa == NULL ||
		    (ifa->ifa_ifp != ifp && ifp != lo_ifp) ||
		    !(route->ro_rt->rt_flags & RTF_UP))) {
			  
                                                     
                                                        
                                                        
                                    
      
			if (ifa != NULL && proxied_ifa &&
			    (route->ro_rt->rt_flags & (RTF_UP | RTF_PROXY)) ==
			    (RTF_UP | RTF_PROXY)) {
				srcif_selected = TRUE;
			} else {
				if (ip6_select_srcif_debug) {
					if (ifa != NULL) {
						printf("%s->%s ifscope %d "
						    "ro_if %s != ifa_if %s "
						    "(cached route cleared)\n",
						    s_src, s_dst,
						    ifscope, if_name(ifp),
						    if_name(ifa->ifa_ifp));
					} else {
						printf("%s->%s ifscope %d "
						    "ro_if %s (no ifa_if "
						    "found)\n", s_src, s_dst,
						    ifscope, if_name(ifp));
					}
				}
				RT_UNLOCK(route->ro_rt);
				ROUTE_RELEASE(route);
				error = EHOSTUNREACH;
				                                  
				route = NULL;
				ifp = NULL;                               
				has_route = FALSE;
			}
		} else if (has_route) {
			srcif_selected = TRUE;
		}

		if (srcif_selected) {
			VERIFY(has_route);
			if (ifa != route->ro_srcia ||
			    !(route->ro_flags & ROF_SRCIF_SELECTED)) {
				RT_CONVERT_LOCK(route->ro_rt);
				if (ifa != NULL) {
					IFA_ADDREF(ifa);                    
				}
				if (route->ro_srcia != NULL) {
					IFA_REMREF(route->ro_srcia);
				}
				route->ro_srcia = ifa;
				route->ro_flags |= ROF_SRCIF_SELECTED;
				RT_GENID_SYNC(route->ro_rt);
			}
			RT_UNLOCK(route->ro_rt);
		}
	} else {
		if (ro->ro_rt != NULL) {
			RT_UNLOCK(ro->ro_rt);
		}
		if (ifp != NULL && opts != NULL &&
		    opts->ip6po_pktinfo != NULL &&
		    opts->ip6po_pktinfo->ipi6_ifindex != 0) {
			  
                                                        
                                                         
                                                  
                                                      
                                
      
			if (!(ifp->if_flags & IFF_LOOPBACK) && ifp->if_index !=
			    opts->ip6po_pktinfo->ipi6_ifindex) {
				error = EHOSTUNREACH;
				goto done;
			}
		}
	}

done:
	  
                                     
    
#define CHECK_RESTRICTIONS(_ip6oa, _ifp)                        \
	((((_ip6oa)->ip6oa_flags & IP6OAF_NO_CELLULAR) &&       \
	    IFNET_IS_CELLULAR(_ifp)) ||                         \
	(((_ip6oa)->ip6oa_flags & IP6OAF_NO_EXPENSIVE) &&       \
	    IFNET_IS_EXPENSIVE(_ifp)) ||                        \
	(((_ip6oa)->ip6oa_flags & IP6OAF_NO_CONSTRAINED) &&     \
	    IFNET_IS_CONSTRAINED(_ifp)) ||                      \
	(!((_ip6oa)->ip6oa_flags & IP6OAF_INTCOPROC_ALLOWED) && \
	    IFNET_IS_INTCOPROC(_ifp)) ||                        \
	(!((_ip6oa)->ip6oa_flags & IP6OAF_AWDL_UNRESTRICTED) && \
	    IFNET_IS_AWDL_RESTRICTED(_ifp)))

	if (error == 0 && ip6oa != NULL &&
	    ((ifp && CHECK_RESTRICTIONS(ip6oa, ifp)) ||
	    (route && route->ro_rt &&
	    CHECK_RESTRICTIONS(ip6oa, route->ro_rt->rt_ifp)))) {
		if (route != NULL && route->ro_rt != NULL) {
			ROUTE_RELEASE(route);
			route = NULL;
		}
		ifp = NULL;                               
		error = EHOSTUNREACH;
		ip6oa->ip6oa_retflags |= IP6OARF_IFDENIED;
	}
#undef CHECK_RESTRICTIONS

	  
                                                               
    
	if (error == 0 &&
	    ifp != NULL && (ifp->if_eflags & IFEF_IPV6_DISABLED)) {
		error = ENETDOWN;
	}

	if (ifp == NULL && (route == NULL || route->ro_rt == NULL)) {
		  
                                                              
                                                       
     
		error = EHOSTUNREACH;
	}
	if (error == EHOSTUNREACH || error == ENETDOWN) {
		ip6stat.ip6s_noroute++;
	}

	  
                                                                 
                                                              
                                        
    
	ifp = ifp0;
	if (retifp != NULL) {
		if (ifp != NULL) {
			ifnet_reference(ifp);                   
		}
		*retifp = ifp;
	}

	if (retsrcia != NULL) {
		if (ifa != NULL) {
			IFA_ADDREF(ifa);                        
		}
		*retsrcia = (struct in6_ifaddr *)ifa;
	}

	if (error == 0) {
		if (retrt != NULL && route != NULL) {
			*retrt = route->ro_rt;                         
		}
	}
	if (ip6_select_srcif_debug) {
		printf("%s %s->%s ifscope %d ifa_if %s ro_if %s (error=%d)\n",
		    __func__,
		    s_src, s_dst, ifscope,
		    (ifa != NULL) ? if_name(ifa->ifa_ifp) : "NONE",
		    (ifp != NULL) ? if_name(ifp) : "NONE", error);
	}

	if (ifa != NULL) {
		IFA_REMREF(ifa);
	}

	return error;
}

  
                                                                          
                                                                             
                                                                       
   
int
in6_selectif(struct sockaddr_in6 *dstsock, struct ip6_pktopts *opts,
    struct ip6_moptions *mopts, struct route_in6 *ro,
    struct ip6_out_args *ip6oa, struct ifnet **retifp)
{
	int err = 0;
	struct route_in6 sro;
	struct rtentry *rt = NULL;

	if (ro == NULL) {
		bzero(&sro, sizeof(sro));
		ro = &sro;
	}

	if ((err = selectroute(NULL, dstsock, opts, mopts, NULL, ro, retifp,
	    &rt, 0, 1, ip6oa)) != 0) {
		goto done;
	}

	  
                                              
                                                            
                                                                  
             
                                                          
                      
                                                                    
                                          
                                                                      
                                    
                                                                
                                      
                                  
                                                                        
                                 
    
	if (rt && (rt->rt_flags & (RTF_REJECT | RTF_BLACKHOLE))) {
		err = ((rt->rt_flags & RTF_HOST) ? EHOSTUNREACH : ENETUNREACH);
		goto done;
	}

	  
                                                                       
                                                               
                                                             
                                                                
               
    
	if (rt != NULL && rt->rt_ifa != NULL && rt->rt_ifa->ifa_ifp != NULL &&
	    retifp != NULL) {
		ifnet_reference(rt->rt_ifa->ifa_ifp);
		if (*retifp != NULL) {
			ifnet_release(*retifp);
		}
		*retifp = rt->rt_ifa->ifa_ifp;
	}

done:
	if (ro == &sro) {
		VERIFY(rt == NULL || rt == ro->ro_rt);
		ROUTE_RELEASE(ro);
	}

	  
                                                            
                                                       
    
	return err;
}

  
                                                                          
                                                                             
                                                                       
  
                                               
   
int
in6_selectroute(struct sockaddr_in6 *srcsock, struct sockaddr_in6 *dstsock,
    struct ip6_pktopts *opts, struct ip6_moptions *mopts,
    struct in6_ifaddr **retsrcia, struct route_in6 *ro, struct ifnet **retifp,
    struct rtentry **retrt, int clone, struct ip6_out_args *ip6oa)
{
	return selectroute(srcsock, dstsock, opts, mopts, retsrcia, ro, retifp,
	           retrt, clone, 0, ip6oa);
}

  
                                                             
                                                 
                                                         
                                                                    
                                  
   
uint8_t
in6_selecthlim(struct in6pcb *in6p, struct ifnet *ifp)
{
	if (in6p && in6p->in6p_hops >= 0) {
		return (uint8_t)in6p->in6p_hops;
	} else if (NULL != ifp) {
		uint8_t chlim;
		struct nd_ifinfo *ndi = ND_IFINFO(ifp);
		if (ndi && ndi->initialized) {
			                                                
			chlim = ndi->chlim;
		} else {
			chlim = (uint8_t)ip6_defhlim;
		}
		return chlim;
	}

	return (uint8_t)ip6_defhlim;
}

  
                                                                   
                                      
   
int
in6_pcbsetport(struct in6_addr *laddr, struct inpcb *inp, struct proc *p,
    int locked)
{
	struct socket *so = inp->inp_socket;
	uint16_t lport = 0, first, last, *lastport, rand_port;
	int count, error = 0, wild = 0;
	boolean_t counting_down;
	bool found, randomport;
	struct inpcbinfo *pcbinfo = inp->inp_pcbinfo;
	kauth_cred_t cred;
#pragma unused(laddr)
	if (!locked) {                                                      
		if (!lck_rw_try_lock_exclusive(pcbinfo->ipi_lock)) {
			socket_unlock(inp->inp_socket, 0);
			lck_rw_lock_exclusive(pcbinfo->ipi_lock);
			socket_lock(inp->inp_socket, 0);
		}

		  
                                                        
                                                 
     
		if (inp->inp_lport != 0) {
			VERIFY(inp->inp_flags2 & INP2_INHASHLIST);
			lck_rw_done(pcbinfo->ipi_lock);

			  
                                                    
            
      
			return 0;
		}
	}

	                                                         
	if ((so->so_options & (SO_REUSEADDR | SO_REUSEPORT)) == 0) {
		wild = INPLOOKUP_WILDCARD;
	}

	randomport = (so->so_flags & SOF_BINDRANDOMPORT) > 0 ||
	    (so->so_type == SOCK_STREAM ? tcp_use_randomport :
	    udp_use_randomport) > 0;

	if (inp->inp_flags & INP_HIGHPORT) {
		first = (uint16_t)ipport_hifirstauto;                 
		last  = (uint16_t)ipport_hilastauto;
		lastport = &pcbinfo->ipi_lasthi;
	} else if (inp->inp_flags & INP_LOWPORT) {
		cred = kauth_cred_proc_ref(p);
		error = priv_check_cred(cred, PRIV_NETINET_RESERVEDPORT, 0);
		kauth_cred_unref(&cred);
		if (error != 0) {
			if (!locked) {
				lck_rw_done(pcbinfo->ipi_lock);
			}
			return error;
		}
		first = (uint16_t)ipport_lowfirstauto;              
		last  = (uint16_t)ipport_lowlastauto;              
		lastport = &pcbinfo->ipi_lastlow;
	} else {
		first = (uint16_t)ipport_firstauto;                   
		last  = (uint16_t)ipport_lastauto;
		lastport = &pcbinfo->ipi_lastport;
	}

	if (first == last) {
		randomport = false;
	}
	  
                                                            
                    
    
	found = false;
	if (first > last) {
		                   
		if (randomport) {
			read_frandom(&rand_port, sizeof(rand_port));
			*lastport = first - (rand_port % (first - last));
		}
		count = first - last;
		counting_down = TRUE;
	} else {
		                 
		if (randomport) {
			read_frandom(&rand_port, sizeof(rand_port));
			*lastport = first + (rand_port % (first - last));
		}
		count = last - first;
		counting_down = FALSE;
	}
	do {
		if (count-- < 0) {                            
			  
                                         
                     
      
			inp->in6p_laddr = in6addr_any;
			inp->in6p_last_outifp = NULL;
			if (!locked) {
				lck_rw_done(pcbinfo->ipi_lock);
			}
			return EAGAIN;
		}
		if (counting_down) {
			--*lastport;
			if (*lastport > first || *lastport < last) {
				*lastport = first;
			}
		} else {
			++*lastport;
			if (*lastport < first || *lastport > last) {
				*lastport = first;
			}
		}
		lport = htons(*lastport);

		  
                                                           
                                  
     
		if (IS_RESTRICTED_IN_PORT(lport)) {
			continue;
		}

		found = (in6_pcblookup_local(pcbinfo, &inp->in6p_laddr,
		    lport, wild) == NULL);
	} while (!found);

	inp->inp_lport = lport;
	inp->inp_flags |= INP_ANONPORT;

	if (in_pcbinshash(inp, 1) != 0) {
		inp->in6p_laddr = in6addr_any;
		inp->in6p_last_outifp = NULL;

		inp->inp_lport = 0;
		inp->inp_flags &= ~INP_ANONPORT;
		if (!locked) {
			lck_rw_done(pcbinfo->ipi_lock);
		}
		return EAGAIN;
	}

	if (!locked) {
		lck_rw_done(pcbinfo->ipi_lock);
	}
	return 0;
}

  
                                                                
                     
                                     
                                                                 
   
struct addrsel_policyent {
	TAILQ_ENTRY(addrsel_policyent) ape_entry;
	struct in6_addrpolicy ape_policy;
};

TAILQ_HEAD(addrsel_policyhead, addrsel_policyent);

struct addrsel_policyhead addrsel_policytab;

static void
init_policy_queue(void)
{
	TAILQ_INIT(&addrsel_policytab);
}

void
addrsel_policy_init(void)
{
	  
                                                       
    
	static const struct in6_addrpolicy defaddrsel[] = {
		                                                        
		{
			.addr = {
				.sin6_family = AF_INET6,
				.sin6_addr   = IN6ADDR_LOOPBACK_INIT,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.addrmask = {
				.sin6_family = AF_INET6,
				.sin6_addr   = IN6MASK128,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.preced   = 50,
			.label    = 0
		},

		                                                        
		{
			.addr = {
				.sin6_family = AF_INET6,
				.sin6_addr   = IN6ADDR_ANY_INIT,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.addrmask = {
				.sin6_family = AF_INET6,
				.sin6_addr   = IN6MASK0,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.preced   = 40,
			.label    = 1
		},

		                                                                 
		{
			.addr = {
				.sin6_family = AF_INET6,
				.sin6_addr   = IN6ADDR_V4MAPPED_INIT,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.addrmask = {
				.sin6_family = AF_INET6,
				.sin6_addr   = IN6MASK96,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.preced   = 35,
			.label    = 4
		},

		                                                      
		{
			.addr = {
				.sin6_family = AF_INET6,
				.sin6_addr   = {{{ 0x20, 0x02 }}},
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.addrmask = {
				.sin6_family = AF_INET6,
				.sin6_addr   = IN6MASK16,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.preced   = 30,
			.label    = 2
		},

		                                                       
		{
			.addr = {
				.sin6_family = AF_INET6,
				.sin6_addr   = {{{ 0x20, 0x01 }}},
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.addrmask = {
				.sin6_family = AF_INET6,
				.sin6_addr   = IN6MASK32,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.preced   = 5,
			.label    = 5
		},

		                                                                   
		{
			.addr = {
				.sin6_family = AF_INET6,
				.sin6_addr   = {{{ 0xfc }}},
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.addrmask = {
				.sin6_family = AF_INET6,
				.sin6_addr   = IN6MASK7,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.preced   = 3,
			.label    = 13
		},

		                                                            
		{
			.addr = {
				.sin6_family = AF_INET6,
				.sin6_addr = IN6ADDR_ANY_INIT,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.addrmask = {
				.sin6_family = AF_INET6,
				.sin6_addr = IN6MASK96,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.preced   = 1,
			.label    = 3
		},

		                                                                         
		{
			.addr = {
				.sin6_family = AF_INET6,
				.sin6_addr = {{{ 0xfe, 0xc0 }}},
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.addrmask = {
				.sin6_family = AF_INET6,
				.sin6_addr = IN6MASK16,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.preced   = 1,
			.label    = 11
		},

		                                                                    
		{
			.addr = {
				.sin6_family = AF_INET6,
				.sin6_addr = {{{ 0x3f, 0xfe }}},
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.addrmask = {
				.sin6_family = AF_INET6,
				.sin6_addr = IN6MASK16,
				.sin6_len    = sizeof(struct sockaddr_in6)
			},
			.preced   = 1,
			.label    = 12
		},
	};
	int i;

	init_policy_queue();

	                                         
	bzero(&defaultaddrpolicy, sizeof(defaultaddrpolicy));
	defaultaddrpolicy.label = ADDR_LABEL_NOTAPP;

	for (i = 0; i < sizeof(defaddrsel) / sizeof(defaddrsel[0]); i++) {
		add_addrsel_policyent(&defaddrsel[i]);
	}
}

struct in6_addrpolicy *
in6_addrsel_lookup_policy(struct sockaddr_in6 *key)
{
	struct in6_addrpolicy *match = NULL;

	ADDRSEL_LOCK();
	match = match_addrsel_policy(key);

	if (match == NULL) {
		match = &defaultaddrpolicy;
	} else {
		match->use++;
	}
	ADDRSEL_UNLOCK();

	return match;
}

static struct in6_addrpolicy *
match_addrsel_policy(struct sockaddr_in6 *key)
{
	struct addrsel_policyent *pent;
	struct in6_addrpolicy *bestpol = NULL, *pol;
	int matchlen, bestmatchlen = -1;
	u_char *mp, *ep, *k, *p, m;

	TAILQ_FOREACH(pent, &addrsel_policytab, ape_entry) {
		matchlen = 0;

		pol = &pent->ape_policy;
		mp = (u_char *)&pol->addrmask.sin6_addr;
		ep = mp + 16;                          
		k = (u_char *)&key->sin6_addr;
		p = (u_char *)&pol->addr.sin6_addr;
		for (; mp < ep && *mp; mp++, k++, p++) {
			m = *mp;
			if ((*k & m) != *p) {
				goto next;                
			}
			if (m == 0xff) {                                   
				matchlen += 8;
			} else {
				while (m >= 0x80) {
					matchlen++;
					m <<= 1;
				}
			}
		}

		                                                              
		if (bestpol == NULL ||
		    matchlen > bestmatchlen) {
			bestpol = pol;
			bestmatchlen = matchlen;
		}

next:
		continue;
	}

	return bestpol;
}

static int
add_addrsel_policyent(const struct in6_addrpolicy *newpolicy)
{
	struct addrsel_policyent *new, *pol;

	MALLOC(new, struct addrsel_policyent *, sizeof(*new), M_IFADDR,
	    M_WAITOK);

	ADDRSEL_LOCK();

	                       
	TAILQ_FOREACH(pol, &addrsel_policytab, ape_entry) {
		if (IN6_ARE_ADDR_EQUAL(&newpolicy->addr.sin6_addr,
		    &pol->ape_policy.addr.sin6_addr) &&
		    IN6_ARE_ADDR_EQUAL(&newpolicy->addrmask.sin6_addr,
		    &pol->ape_policy.addrmask.sin6_addr)) {
			ADDRSEL_UNLOCK();
			FREE(new, M_IFADDR);
			return EEXIST;                             
		}
	}

	bzero(new, sizeof(*new));

	                                
	new->ape_policy = *newpolicy;

	TAILQ_INSERT_TAIL(&addrsel_policytab, new, ape_entry);
	ADDRSEL_UNLOCK();

	return 0;
}
#ifdef ENABLE_ADDRSEL
static int
delete_addrsel_policyent(const struct in6_addrpolicy *key)
{
	struct addrsel_policyent *pol;


	ADDRSEL_LOCK();

	                                       
	TAILQ_FOREACH(pol, &addrsel_policytab, ape_entry) {
		if (IN6_ARE_ADDR_EQUAL(&key->addr.sin6_addr,
		    &pol->ape_policy.addr.sin6_addr) &&
		    IN6_ARE_ADDR_EQUAL(&key->addrmask.sin6_addr,
		    &pol->ape_policy.addrmask.sin6_addr)) {
			break;
		}
	}
	if (pol == NULL) {
		ADDRSEL_UNLOCK();
		return ESRCH;
	}

	TAILQ_REMOVE(&addrsel_policytab, pol, ape_entry);
	FREE(pol, M_IFADDR);
	pol = NULL;
	ADDRSEL_UNLOCK();

	return 0;
}
#endif                     

int
walk_addrsel_policy(int (*callback)(const struct in6_addrpolicy *, void *),
    void *w)
{
	struct addrsel_policyent *pol;
	int error = 0;

	ADDRSEL_LOCK();
	TAILQ_FOREACH(pol, &addrsel_policytab, ape_entry) {
		if ((error = (*callback)(&pol->ape_policy, w)) != 0) {
			ADDRSEL_UNLOCK();
			return error;
		}
	}
	ADDRSEL_UNLOCK();
	return error;
}
  
                                                                       
   
struct walkarg {
	struct sysctl_req *w_req;
};


static int
dump_addrsel_policyent(const struct in6_addrpolicy *pol, void *arg)
{
	int error = 0;
	struct walkarg *w = arg;

	error = SYSCTL_OUT(w->w_req, pol, sizeof(*pol));

	return error;
}

static int
in6_src_sysctl SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	struct walkarg w;

	if (req->newptr) {
		return EPERM;
	}
	bzero(&w, sizeof(w));
	w.w_req = req;

	return walk_addrsel_policy(dump_addrsel_policyent, &w);
}


SYSCTL_NODE(_net_inet6_ip6, IPV6CTL_ADDRCTLPOLICY, addrctlpolicy,
    CTLFLAG_RD | CTLFLAG_LOCKED, in6_src_sysctl, "");
int
in6_src_ioctl(u_long cmd, caddr_t data)
{
	int i;
	struct in6_addrpolicy ent0;

	if (cmd != SIOCAADDRCTL_POLICY && cmd != SIOCDADDRCTL_POLICY) {
		return EOPNOTSUPP;                       
	}
	bcopy(data, &ent0, sizeof(ent0));

	if (ent0.label == ADDR_LABEL_NOTAPP) {
		return EINVAL;
	}
	                                              
	if (in6_mask2len(&ent0.addrmask.sin6_addr, NULL) < 0) {
		return EINVAL;
	}
	                                                             
	for (i = 0; i < 4; i++) {
		ent0.addr.sin6_addr.s6_addr32[i] &=
		    ent0.addrmask.sin6_addr.s6_addr32[i];
	}
	ent0.use = 0;

	switch (cmd) {
	case SIOCAADDRCTL_POLICY:
#ifdef ENABLE_ADDRSEL
		return add_addrsel_policyent(&ent0);
#else
		return ENOTSUP;
#endif
	case SIOCDADDRCTL_POLICY:
#ifdef ENABLE_ADDRSEL
		return delete_addrsel_policyent(&ent0);
#else
		return ENOTSUP;
#endif
	}

	return 0;                                            
}

  
                                                                      
                                                                          
                                                  
                                                                   
                 
  
                                                                  
                          
  
                                                                              
                                                                          
                                                                               
                                                                         
   
int
in6_embedscope(struct in6_addr *in6, const struct sockaddr_in6 *sin6,
    struct in6pcb *in6p, struct ifnet **ifpp, struct ip6_pktopts *opt)
{
	struct ifnet *ifp = NULL;
	u_int32_t scopeid;
	struct ip6_pktopts *optp = NULL;

	*in6 = sin6->sin6_addr;
	scopeid = sin6->sin6_scope_id;
	if (ifpp != NULL) {
		*ifpp = NULL;
	}

	  
                                                                       
                                             
    

#ifdef ENABLE_DEFAULT_SCOPE
	if (scopeid == 0) {
		scopeid = scope6_addr2default(in6);
	}
#endif

	if (IN6_IS_SCOPE_LINKLOCAL(in6) || IN6_IS_ADDR_MC_INTFACELOCAL(in6)) {
		struct in6_pktinfo *pi;
		struct ifnet *im6o_multicast_ifp = NULL;

		if (in6p != NULL && IN6_IS_ADDR_MULTICAST(in6) &&
		    in6p->in6p_moptions != NULL) {
			IM6O_LOCK(in6p->in6p_moptions);
			im6o_multicast_ifp =
			    in6p->in6p_moptions->im6o_multicast_ifp;
			IM6O_UNLOCK(in6p->in6p_moptions);
		}

		if (opt != NULL) {
			optp = opt;
		} else if (in6p != NULL) {
			optp = in6p->in6p_outputopts;
		}
		  
                                             
     
		if (in6p != NULL && optp != NULL &&
		    (pi = optp->ip6po_pktinfo) != NULL &&
		    pi->ipi6_ifindex != 0) {
			                                                   
			if (ifpp != NULL) {
				ifnet_head_lock_shared();
				ifp = ifindex2ifnet[pi->ipi6_ifindex];
				ifnet_head_done();
			}
			in6->s6_addr16[1] = htons((uint16_t)pi->ipi6_ifindex);
		} else if (in6p != NULL && IN6_IS_ADDR_MULTICAST(in6) &&
		    in6p->in6p_moptions != NULL && im6o_multicast_ifp != NULL) {
			ifp = im6o_multicast_ifp;
			in6->s6_addr16[1] = htons(ifp->if_index);
		} else if (scopeid != 0) {
			  
                                                         
                                                        
                                              
      
			if (if_index < scopeid) {
				return ENXIO;                   
			}
			                                                   
			if (ifpp != NULL) {
				ifnet_head_lock_shared();
				ifp = ifindex2ifnet[scopeid];
				ifnet_head_done();
			}
			                                                 
			in6->s6_addr16[1] = htons(scopeid & 0xffff);
		}

		if (ifpp != NULL) {
			if (ifp != NULL) {
				ifnet_reference(ifp);                   
			}
			*ifpp = ifp;
		}
	}

	return 0;
}

  
                                                     
                                            
  
                                                                  
                          
   
int
in6_recoverscope(
	struct sockaddr_in6 *sin6,
	const struct in6_addr *in6,
	struct ifnet *ifp)
{
	u_int32_t scopeid;

	sin6->sin6_addr = *in6;

	  
                                                            
                                             
    

	sin6->sin6_scope_id = 0;
	if (IN6_IS_SCOPE_LINKLOCAL(in6) || IN6_IS_ADDR_MC_INTFACELOCAL(in6)) {
		  
                                             
     
		scopeid = ntohs(sin6->sin6_addr.s6_addr16[1]);
		if (scopeid) {
			  
                  
     
                                                         
                      
      
			if (if_index < scopeid) {
				return ENXIO;
			}
			if (ifp && ifp->if_index != scopeid) {
				return ENXIO;
			}
			sin6->sin6_addr.s6_addr16[1] = 0;
			sin6->sin6_scope_id = scopeid;
		}
	}

	return 0;
}
