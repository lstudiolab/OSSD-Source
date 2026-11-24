/*
 * Copyright (c) 2000-2016 Apple Inc. All rights reserved.
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
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/mcache.h>
#include <sys/errno.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_encap.h>

#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/ip6protosw.h>

#include <net/net_osdep.h>

#ifndef __APPLE__
#include <sys/kernel.h>
#include <sys/malloc.h>
MALLOC_DEFINE(M_NETADDR, "Export Host", "Export host address structure");
#endif

static void encap_init(struct protosw *, struct domain *);
static void encap_add_locked(struct encaptab *);
static int mask_match(const struct encaptab *, const struct sockaddr *,
    const struct sockaddr *);
static void encap_fillarg(struct mbuf *, void *arg);

#ifndef LIST_HEAD_INITIALIZER
                                  
LIST_HEAD(, encaptab) encaptab;
#else
LIST_HEAD(, encaptab) encaptab = LIST_HEAD_INITIALIZER(&encaptab);
#endif

decl_lck_rw_data(static, encaptab_lock);

static void
encap_init(struct protosw *pp, struct domain *dp)
{
#pragma unused(dp)
	static int encap_initialized = 0;
	lck_grp_attr_t *encaptab_grp_attrib = NULL;
	lck_attr_t *encaptab_lck_attrib = NULL;
	lck_grp_t *encaptab_lck_group = NULL;

	VERIFY((pp->pr_flags & (PR_INITIALIZED | PR_ATTACHED)) == PR_ATTACHED);

	                                                                     
	if (encap_initialized) {
		return;
	}

	encaptab_grp_attrib = lck_grp_attr_alloc_init();
	encaptab_lck_group = lck_grp_alloc_init("encaptab lock", encaptab_grp_attrib);
	lck_grp_attr_free(encaptab_grp_attrib);

	encaptab_lck_attrib = lck_attr_alloc_init();
	lck_rw_init(&encaptab_lock, encaptab_lck_group, encaptab_lck_attrib);

	lck_grp_free(encaptab_lck_group);
	lck_attr_free(encaptab_lck_attrib);

	encap_initialized = 1;
#if 0
	  
                                                                  
                                                                  
                                                             
                                                                   
                 
    
	LIST_INIT(&encaptab);
#endif
}

void
encap4_init(struct protosw *pp, struct domain *dp)
{
	encap_init(pp, dp);
}

void
encap6_init(struct ip6protosw *pp, struct domain *dp)
{
	encap_init((struct protosw *)pp, dp);
}

#if INET
void
encap4_input(struct mbuf *m, int off)
{
	int proto;
	struct ip *ip;
	struct sockaddr_in s, d;
	const struct protosw *psw;
	struct encaptab *ep, *match;
	int prio, matchprio;
	void *match_arg = NULL;

#ifndef __APPLE__
	va_start(ap, m);
	off = va_arg(ap, int);
	proto = va_arg(ap, int);
	va_end(ap);
#endif

	                                                                  
	MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(m);

	ip = mtod(m, struct ip *);
#ifdef __APPLE__
	proto = ip->ip_p;
#endif

	bzero(&s, sizeof(s));
	s.sin_family = AF_INET;
	s.sin_len = sizeof(struct sockaddr_in);
	s.sin_addr = ip->ip_src;
	bzero(&d, sizeof(d));
	d.sin_family = AF_INET;
	d.sin_len = sizeof(struct sockaddr_in);
	d.sin_addr = ip->ip_dst;

	match = NULL;
	matchprio = 0;

	lck_rw_lock_shared(&encaptab_lock);
	for (ep = LIST_FIRST(&encaptab); ep; ep = LIST_NEXT(ep, chain)) {
		if (ep->af != AF_INET) {
			continue;
		}
		if (ep->proto >= 0 && ep->proto != proto) {
			continue;
		}
		if (ep->func) {
			prio = (*ep->func)(m, off, proto, ep->arg);
		} else {
			  
                                                       
           
      
			prio = mask_match(ep, (struct sockaddr *)&d,
			    (struct sockaddr *)&s);
		}

		  
                                                         
                                                               
                                                              
                                                                  
                                                         
    
                                                          
                                                      
                                                     
                                                        
                                     
                                     
    
                                                        
                                                      
                                     
     
		if (prio <= 0) {
			continue;
		}
		if (prio > matchprio) {
			matchprio = prio;
			match = ep;
			psw = (const struct protosw *)match->psw;
			match_arg = ep->arg;
		}
	}
	lck_rw_unlock_shared(&encaptab_lock);

	if (match) {
		                                             
		if (psw && psw->pr_input) {
			encap_fillarg(m, match_arg);
			(*psw->pr_input)(m, off);
		} else {
			m_freem(m);
		}
		return;
	}

	                                       
	rip_input(m, off);
}
#endif

int
encap6_input(struct mbuf **mp, int *offp, int proto)
{
	struct mbuf *m = *mp;
	struct ip6_hdr *ip6;
	struct sockaddr_in6 s, d;
	const struct ip6protosw *psw;
	struct encaptab *ep, *match;
	int prio, matchprio;
	void *match_arg = NULL;

	                                                                  
	MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(m);

	ip6 = mtod(m, struct ip6_hdr *);
	bzero(&s, sizeof(s));
	s.sin6_family = AF_INET6;
	s.sin6_len = sizeof(struct sockaddr_in6);
	s.sin6_addr = ip6->ip6_src;
	bzero(&d, sizeof(d));
	d.sin6_family = AF_INET6;
	d.sin6_len = sizeof(struct sockaddr_in6);
	d.sin6_addr = ip6->ip6_dst;

	match = NULL;
	matchprio = 0;

	lck_rw_lock_shared(&encaptab_lock);
	for (ep = LIST_FIRST(&encaptab); ep; ep = LIST_NEXT(ep, chain)) {
		if (ep->af != AF_INET6) {
			continue;
		}
		if (ep->proto >= 0 && ep->proto != proto) {
			continue;
		}
		if (ep->func) {
			prio = (*ep->func)(m, *offp, proto, ep->arg);
		} else {
			  
                                                       
           
      
			prio = mask_match(ep, (struct sockaddr *)&d,
			    (struct sockaddr *)&s);
		}

		                                        
		if (prio <= 0) {
			continue;
		}
		if (prio > matchprio) {
			matchprio = prio;
			match = ep;
			psw = (const struct ip6protosw *)match->psw;
			match_arg = ep->arg;
		}
	}
	lck_rw_unlock_shared(&encaptab_lock);

	if (match) {
		                   
		if (psw && psw->pr_input) {
			encap_fillarg(m, match_arg);
			return (*psw->pr_input)(mp, offp, proto);
		} else {
			m_freem(m);
			return IPPROTO_DONE;
		}
	}

	                                       
	return rip6_input(mp, offp, proto);
}

static void
encap_add_locked(struct encaptab *ep)
{
	LCK_RW_ASSERT(&encaptab_lock, LCK_RW_ASSERT_EXCLUSIVE);
	LIST_INSERT_HEAD(&encaptab, ep, chain);
}

  
                                                                          
                                                             
                                                                       
   
const struct encaptab *
encap_attach(int af, int proto, const struct sockaddr *sp,
    const struct sockaddr *sm, const struct sockaddr *dp,
    const struct sockaddr *dm, const struct protosw *psw, void *arg)
{
	struct encaptab *ep = NULL;
	struct encaptab *new_ep = NULL;
	int error;

	                          
	if (sp->sa_len > sizeof(new_ep->src) || dp->sa_len > sizeof(new_ep->dst)) {
		error = EINVAL;
		goto fail;
	}
	if (sp->sa_len != dp->sa_len) {
		error = EINVAL;
		goto fail;
	}
	if (af != sp->sa_family || af != dp->sa_family) {
		error = EINVAL;
		goto fail;
	}

	new_ep = _MALLOC(sizeof(*new_ep), M_NETADDR, M_WAITOK | M_ZERO);
	if (new_ep == NULL) {
		error = ENOBUFS;
		goto fail;
	}

	                                                                    
	lck_rw_lock_exclusive(&encaptab_lock);
	for (ep = LIST_FIRST(&encaptab); ep; ep = LIST_NEXT(ep, chain)) {
		if (ep->af != af) {
			continue;
		}
		if (ep->proto != proto) {
			continue;
		}
		if (ep->src.ss_len != sp->sa_len ||
		    bcmp(&ep->src, sp, sp->sa_len) != 0 ||
		    bcmp(&ep->srcmask, sm, sp->sa_len) != 0) {
			continue;
		}
		if (ep->dst.ss_len != dp->sa_len ||
		    bcmp(&ep->dst, dp, dp->sa_len) != 0 ||
		    bcmp(&ep->dstmask, dm, dp->sa_len) != 0) {
			continue;
		}

		error = EEXIST;
		goto fail_locked;
	}

	new_ep->af = af;
	new_ep->proto = proto;
	bcopy(sp, &new_ep->src, sp->sa_len);
	bcopy(sm, &new_ep->srcmask, sp->sa_len);
	bcopy(dp, &new_ep->dst, dp->sa_len);
	bcopy(dm, &new_ep->dstmask, dp->sa_len);
	new_ep->psw = psw;
	new_ep->arg = arg;

	encap_add_locked(new_ep);
	lck_rw_unlock_exclusive(&encaptab_lock);

	error = 0;
	return new_ep;

fail_locked:
	lck_rw_unlock_exclusive(&encaptab_lock);
	if (new_ep != NULL) {
		_FREE(new_ep, M_NETADDR);
	}
fail:
	return NULL;
}

const struct encaptab *
encap_attach_func( int af, int proto,
    int (*func)(const struct mbuf *, int, int, void *),
    const struct protosw *psw, void *arg)
{
	struct encaptab *ep;
	int error;

	                          
	if (!func) {
		error = EINVAL;
		goto fail;
	}

	ep = _MALLOC(sizeof(*ep), M_NETADDR, M_WAITOK | M_ZERO);          
	if (ep == NULL) {
		error = ENOBUFS;
		goto fail;
	}

	ep->af = af;
	ep->proto = proto;
	ep->func = func;
	ep->psw = psw;
	ep->arg = arg;

	lck_rw_lock_exclusive(&encaptab_lock);
	encap_add_locked(ep);
	lck_rw_unlock_exclusive(&encaptab_lock);

	error = 0;
	return ep;

fail:
	return NULL;
}

int
encap_detach(const struct encaptab *cookie)
{
	const struct encaptab *ep = cookie;
	struct encaptab *p;

	lck_rw_lock_exclusive(&encaptab_lock);
	for (p = LIST_FIRST(&encaptab); p; p = LIST_NEXT(p, chain)) {
		if (p == ep) {
			LIST_REMOVE(p, chain);
			lck_rw_unlock_exclusive(&encaptab_lock);
			_FREE(p, M_NETADDR);           
			return 0;
		}
	}
	lck_rw_unlock_exclusive(&encaptab_lock);

	return EINVAL;
}

static int
mask_match(const struct encaptab *ep, const struct sockaddr *sp,
    const struct sockaddr *dp)
{
	struct sockaddr_storage s;
	struct sockaddr_storage d;
	int i;
	const u_int8_t *p, *q;
	u_int8_t *r;
	int matchlen;

	if (sp->sa_len > sizeof(s) || dp->sa_len > sizeof(d)) {
		return 0;
	}
	if (sp->sa_family != ep->af || dp->sa_family != ep->af) {
		return 0;
	}
	if (sp->sa_len != ep->src.ss_len || dp->sa_len != ep->dst.ss_len) {
		return 0;
	}

	matchlen = 0;

	p = (const u_int8_t *)sp;
	q = (const u_int8_t *)&ep->srcmask;
	r = (u_int8_t *)&s;
	for (i = 0; i < sp->sa_len; i++) {
		r[i] = p[i] & q[i];
		                  
		matchlen += (q[i] ? 8 : 0);
	}

	p = (const u_int8_t *)dp;
	q = (const u_int8_t *)&ep->dstmask;
	r = (u_int8_t *)&d;
	for (i = 0; i < dp->sa_len; i++) {
		r[i] = p[i] & q[i];
		                        
		matchlen += (q[i] ? 8 : 0);
	}

	                                                                   
	s.ss_len = sp->sa_len;
	s.ss_family = sp->sa_family;
	d.ss_len = dp->sa_len;
	d.ss_family = dp->sa_family;

	if (bcmp(&s, &ep->src, ep->src.ss_len) == 0 &&
	    bcmp(&d, &ep->dst, ep->dst.ss_len) == 0) {
		return matchlen;
	} else {
		return 0;
	}
}

struct encaptabtag {
	void*                   *arg;
};

static void
encap_fillarg(
	struct mbuf *m,
	void *arg)
{
	struct m_tag    *tag;
	struct encaptabtag *et;

	tag = m_tag_create(KERNEL_MODULE_TAG_ID, KERNEL_TAG_TYPE_ENCAP,
	    sizeof(struct encaptabtag), M_WAITOK, m);

	if (tag != NULL) {
		et = (struct encaptabtag*)(tag + 1);
		et->arg = arg;
		m_tag_prepend(m, tag);
	}
}

void *
encap_getarg(struct mbuf *m)
{
	struct m_tag    *tag;
	struct encaptabtag *et;
	void *p = NULL;

	tag = m_tag_locate(m, KERNEL_MODULE_TAG_ID, KERNEL_TAG_TYPE_ENCAP, NULL);
	if (tag) {
		et = (struct encaptabtag*)(tag + 1);
		p = et->arg;
		m_tag_delete(m, tag);
	}

	return p;
}
