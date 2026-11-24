/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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
 * Copyright (C) 1999 WIDE Project.
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
 * Copyright (c) 1982, 1986, 1988, 1991, 1993
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
 *	@(#)uipc_mbuf.c	8.4 (Berkeley) 2/14/95
 */
  
                                                                      
                                                                          
                                                                        
               
   


                          

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc_internal.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mcache.h>
#include <netinet/in.h>
#include <netinet/ip_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

  
                                                                    
                                               
                                                                            
                                                                                
  
                                                                   
  
                                                                       
   
struct mbuf *
m_pulldown(struct mbuf *m, int off, int len, int *offp)
{
	struct mbuf *n = NULL, *o = NULL;
	int hlen = 0, tlen = 0, olen = 0;
	int sharedcluster = 0;

	                              
	VERIFY(len >= 0 && off >= 0);

	if (m == NULL) {
		panic("m == NULL in m_pulldown()");
	}
	if (len > MCLBYTES) {
		m_freem(m);
		return NULL;                    
	}
	int tmp_len = 0;
	if (os_add_overflow(off, len, &tmp_len)) {
		m_free(m);
		return NULL;
	}

#ifdef PULLDOWN_DEBUG
	{
		struct mbuf *t;
		printf("before:");
		for (t = m; t; t = t->m_next) {
			printf(" %d", t->m_len);
		}
		printf("\n");
	}
#endif
	n = m;

	  
                                        
                                         
                                         
               
    
	while (n != NULL && off > 0) {
		if (n->m_len > off) {
			break;
		}
		off -= n->m_len;
		n = n->m_next;
	}

	                                     
	while (n != NULL && n->m_len == 0) {
		n = n->m_next;
	}

	if (!n) {
		m_freem(m);
		return NULL;                              
	}

	  
                                   
                                                      
   
                                                          
                                                        
                                                       
                                                          
              
   
                                                              
                                                                
                                 
    
	if ((off == 0 || offp) && len <= n->m_len - off) {
		goto ok;
	}

	  
                                                                  
                                                                    
                                         
                                                        
    
	if (len <= n->m_len - off) {
		o = m_copym(n, off, n->m_len - off, M_DONTWAIT);
		if (o == NULL) {
			m_freem(m);
			return NULL;                 
		}
		n->m_len = off;
		o->m_next = n->m_next;
		n->m_next = o;
		n = n->m_next;
		off = 0;
		goto ok;
	}

	  
                                                                    
                                                    
                                               
   
                                                       
    
	hlen = n->m_len - off;
	tlen = len - hlen;

	  
                                                           
                                              
    
	olen = 0;
	for (o = n->m_next; o != NULL; o = o->m_next) {
		olen += o->m_len;
	}
	if (hlen + olen < len) {
		m_freem(m);
		return NULL;                              
	}

	  
                     
                                                                
    
	if ((n->m_flags & M_EXT) == 0) {
		sharedcluster = 0;
	} else {
		if (m_get_ext_free(n) != NULL) {
			sharedcluster = 1;
		} else if (m_mclhasreference(n)) {
			sharedcluster = 1;
		} else {
			sharedcluster = 0;
		}
	}

	  
                                                              
                                                                       
                                                  
    
	if ((off == 0 || offp) && M_TRAILINGSPACE(n) >= tlen
	    && !sharedcluster) {
		m_copydata(n->m_next, 0, tlen, mtod(n, caddr_t) + n->m_len);
		n->m_len += tlen;
		m_adj(n->m_next, tlen);
		goto ok;
	}

	  
                                                                       
                                                                       
                                                                  
                                                         
    
	if ((off == 0 || offp) && M_LEADINGSPACE(n->m_next) >= hlen &&
	    (n->m_next->m_len + hlen) >= len && !sharedcluster) {
		n->m_next->m_data -= hlen;
		n->m_next->m_len += hlen;
		bcopy(mtod(n, caddr_t) + off, mtod(n->m_next, caddr_t), hlen);
		n->m_len -= hlen;
		n = n->m_next;
		off = 0;
		goto ok;
	}

	  
                                                                     
                
    
	MGET(o, M_DONTWAIT, m->m_type);
	if (o == NULL) {
		m_freem(m);
		return NULL;                 
	}
	if (len > MHLEN) {                                     
		MCLGET(o, M_DONTWAIT);
		if ((o->m_flags & M_EXT) == 0) {
			m_freem(m);
			m_free(o);
			return NULL;                 
		}
	}
	                                        
	o->m_len = hlen;
	bcopy(mtod(n, caddr_t) + off, mtod(o, caddr_t), hlen);
	n->m_len -= hlen;
	                                                 
	m_copydata(n->m_next, 0, tlen, mtod(o, caddr_t) + o->m_len);
	o->m_len += tlen;
	m_adj(n->m_next, tlen);
	o->m_next = n->m_next;
	n->m_next = o;
	n = o;
	off = 0;

ok:
#ifdef PULLDOWN_DEBUG
	{
		struct mbuf *t;
		printf("after:");
		for (t = m; t; t = t->m_next) {
			printf("%c%d", t == n ? '*' : ' ', t->m_len);
		}
		printf(" (off=%d)\n", off);
	}
#endif
	if (offp) {
		*offp = off;
	}
	return n;
}

  
                                                                         
                                      
   
struct m_tag *
m_tag_create(u_int32_t id, u_int16_t type, int len, int wait, struct mbuf *buf)
{
	struct m_tag *t = NULL;
	struct m_tag *p;

	if (len < 0) {
		return NULL;
	}

	if (len + sizeof(struct m_tag) + sizeof(struct m_taghdr) > MLEN) {
		return m_tag_alloc(id, type, len, wait);
	}

	  
                                                                 
                                                  
                                                                   
    
	p = SLIST_FIRST(&buf->m_pkthdr.tags);
	while (p != NULL) {
		                
		if (M_TAG_ALIGN(p->m_tag_len) +
		    sizeof(struct m_taghdr) > MLEN) {
			p = SLIST_NEXT(p, m_tag_link);
			continue;
		}

		VERIFY(p->m_tag_cookie == M_TAG_VALID_PATTERN);

		struct mbuf *m = m_dtom(p);
		struct m_taghdr *hdr = (struct m_taghdr *)(void *)m->m_data;

		VERIFY(IS_P2ALIGNED(hdr + 1, sizeof(u_int64_t)));
		VERIFY(m->m_flags & M_TAGHDR && !(m->m_flags & M_EXT));

		                                   
		if (M_TAG_ALIGN(len) <= MLEN - m->m_len) {
			t = (struct m_tag *)(void *)(m->m_data + m->m_len);
			VERIFY(IS_P2ALIGNED(t, sizeof(u_int64_t)));
			hdr->refcnt++;
			m->m_len += M_TAG_ALIGN(len);
			VERIFY(m->m_len <= MLEN);
			break;
		}

		p = SLIST_NEXT(p, m_tag_link);
	}

	if (t == NULL) {
		return m_tag_alloc(id, type, len, wait);
	}

	t->m_tag_cookie = M_TAG_VALID_PATTERN;
	t->m_tag_type = type;
	t->m_tag_len = (uint16_t)len;
	t->m_tag_id = id;
	if (len > 0) {
		bzero(t + 1, len);
	}
	return t;
}

                                                                     
struct m_tag *
m_tag_alloc(u_int32_t id, u_int16_t type, int len, int wait)
{
	struct m_tag *t;

	if (len < 0) {
		return NULL;
	}

	if (M_TAG_ALIGN(len) + sizeof(struct m_taghdr) <= MLEN) {
		struct mbuf *m = m_get(wait, MT_TAG);
		struct m_taghdr *hdr;

		if (m == NULL) {
			return NULL;
		}

		m->m_flags |= M_TAGHDR;

		hdr = (struct m_taghdr *)(void *)m->m_data;
		VERIFY(IS_P2ALIGNED(hdr + 1, sizeof(u_int64_t)));
		hdr->refcnt = 1;
		m->m_len += sizeof(struct m_taghdr);
		t = (struct m_tag *)(void *)(m->m_data + m->m_len);
		VERIFY(IS_P2ALIGNED(t, sizeof(u_int64_t)));
		m->m_len += M_TAG_ALIGN(len);
		VERIFY(m->m_len <= MLEN);
	} else if (len + sizeof(struct m_tag) <= MCLBYTES) {
		t = (struct m_tag *)(void *)m_mclalloc(wait);
	} else {
		t = NULL;
	}

	if (t == NULL) {
		return NULL;
	}

	VERIFY(IS_P2ALIGNED(t, sizeof(u_int64_t)));
	t->m_tag_cookie = M_TAG_VALID_PATTERN;
	t->m_tag_type = type;
	t->m_tag_len = (uint16_t)len;
	t->m_tag_id = id;
	if (len > 0) {
		bzero(t + 1, len);
	}
	return t;
}


                        
void
m_tag_free(struct m_tag *t)
{
	if (t == NULL) {
		return;
	}

	VERIFY(t->m_tag_cookie == M_TAG_VALID_PATTERN);

	if (M_TAG_ALIGN(t->m_tag_len) + sizeof(struct m_taghdr) <= MLEN) {
		struct mbuf * m = m_dtom(t);
		VERIFY(m->m_flags & M_TAGHDR);
		struct m_taghdr *hdr = (struct m_taghdr *)(void *)m->m_data;

		VERIFY(IS_P2ALIGNED(hdr + 1, sizeof(u_int64_t)));

		                                
		if (--hdr->refcnt == 0) {
			m_free(m);
			return;
		}

		                             
		u_int64_t *fill_ptr = (u_int64_t *)t;
		u_int64_t *end_ptr = (u_int64_t *)(t + 1);
		while (fill_ptr < end_ptr) {
			*fill_ptr = M_TAG_FREE_PATTERN;
			fill_ptr++;
		}
	} else {
		m_mclfree((caddr_t)t);
	}
}

                           
void
m_tag_prepend(struct mbuf *m, struct m_tag *t)
{
	VERIFY(m != NULL && t != NULL);

	SLIST_INSERT_HEAD(&m->m_pkthdr.tags, t, m_tag_link);
}

                          
void
m_tag_unlink(struct mbuf *m, struct m_tag *t)
{
	VERIFY(m->m_flags & M_PKTHDR);
	VERIFY(t != NULL && t->m_tag_cookie == M_TAG_VALID_PATTERN);

	SLIST_REMOVE(&m->m_pkthdr.tags, t, m_tag, m_tag_link);
}

                                   
void
m_tag_delete(struct mbuf *m, struct m_tag *t)
{
	m_tag_unlink(m, t);
	m_tag_free(t);
}

                                                                  
void
m_tag_delete_chain(struct mbuf *m, struct m_tag *t)
{
	struct m_tag *p, *q;

	VERIFY(m->m_flags & M_PKTHDR);

	if (t != NULL) {
		p = t;
	} else {
		p = SLIST_FIRST(&m->m_pkthdr.tags);
	}
	if (p == NULL) {
		return;
	}

	VERIFY(p->m_tag_cookie == M_TAG_VALID_PATTERN);
	while ((q = SLIST_NEXT(p, m_tag_link)) != NULL) {
		VERIFY(q->m_tag_cookie == M_TAG_VALID_PATTERN);
		m_tag_delete(m, q);
	}
	m_tag_delete(m, p);
}

                                                 
struct m_tag *
m_tag_locate(struct mbuf *m, u_int32_t id, u_int16_t type, struct m_tag *t)
{
	struct m_tag *p;

	VERIFY(m->m_flags & M_PKTHDR);

	if (t == NULL) {
		p = SLIST_FIRST(&m->m_pkthdr.tags);
	} else {
		VERIFY(t->m_tag_cookie == M_TAG_VALID_PATTERN);
		p = SLIST_NEXT(t, m_tag_link);
	}
	while (p != NULL) {
		VERIFY(p->m_tag_cookie == M_TAG_VALID_PATTERN);
		if (p->m_tag_id == id && p->m_tag_type == type) {
			return p;
		}
		p = SLIST_NEXT(p, m_tag_link);
	}
	return NULL;
}

                        
struct m_tag *
m_tag_copy(struct m_tag *t, int how)
{
	struct m_tag *p;

	VERIFY(t != NULL);

	p = m_tag_alloc(t->m_tag_id, t->m_tag_type, t->m_tag_len, how);
	if (p == NULL) {
		return NULL;
	}
	bcopy(t + 1, p + 1, t->m_tag_len);                    
	return p;
}

  
                                                                    
                                                                     
                                                                
                    
   
int
m_tag_copy_chain(struct mbuf *to, struct mbuf *from, int how)
{
	struct m_tag *p, *t, *tprev = NULL;

	VERIFY((to->m_flags & M_PKTHDR) && (from->m_flags & M_PKTHDR));

	m_tag_delete_chain(to, NULL);
	SLIST_FOREACH(p, &from->m_pkthdr.tags, m_tag_link) {
		VERIFY(p->m_tag_cookie == M_TAG_VALID_PATTERN);
		t = m_tag_copy(p, how);
		if (t == NULL) {
			m_tag_delete_chain(to, NULL);
			return 0;
		}
		if (tprev == NULL) {
			SLIST_INSERT_HEAD(&to->m_pkthdr.tags, t, m_tag_link);
		} else {
			SLIST_INSERT_AFTER(tprev, t, m_tag_link);
			tprev = t;
		}
	}
	return 1;
}

                                                    
void
m_tag_init(struct mbuf *m, int all)
{
	VERIFY(m->m_flags & M_PKTHDR);

	SLIST_INIT(&m->m_pkthdr.tags);
	  
                                                    
                                             
    
	if (all) {
		bzero(&m->m_pkthdr.builtin_mtag._net_mtag,
		    sizeof(m->m_pkthdr.builtin_mtag._net_mtag));
	}
}

                             
struct m_tag *
m_tag_first(struct mbuf *m)
{
	VERIFY(m->m_flags & M_PKTHDR);

	return SLIST_FIRST(&m->m_pkthdr.tags);
}

                            
struct m_tag *
m_tag_next(struct mbuf *m, struct m_tag *t)
{
#pragma unused(m)
	VERIFY(t != NULL);
	VERIFY(t->m_tag_cookie == M_TAG_VALID_PATTERN);

	return SLIST_NEXT(t, m_tag_link);
}

int
m_set_traffic_class(struct mbuf *m, mbuf_traffic_class_t tc)
{
	u_int32_t val = MBUF_TC2SCVAL(tc);                                

	return m_set_service_class(m, m_service_class_from_val(val));
}

mbuf_traffic_class_t
m_get_traffic_class(struct mbuf *m)
{
	return MBUF_SC2TC(m_get_service_class(m));
}

int
m_set_service_class(struct mbuf *m, mbuf_svc_class_t sc)
{
	int error = 0;

	VERIFY(m->m_flags & M_PKTHDR);

	if (MBUF_VALID_SC(sc)) {
		m->m_pkthdr.pkt_svc = sc;
	} else {
		error = EINVAL;
	}

	return error;
}

mbuf_svc_class_t
m_get_service_class(struct mbuf *m)
{
	mbuf_svc_class_t sc;

	VERIFY(m->m_flags & M_PKTHDR);

	if (MBUF_VALID_SC(m->m_pkthdr.pkt_svc)) {
		sc = m->m_pkthdr.pkt_svc;
	} else {
		sc = MBUF_SC_BE;
	}

	return sc;
}

mbuf_svc_class_t
m_service_class_from_idx(u_int32_t i)
{
	mbuf_svc_class_t sc = MBUF_SC_BE;

	switch (i) {
	case SCIDX_BK_SYS:
		return MBUF_SC_BK_SYS;

	case SCIDX_BK:
		return MBUF_SC_BK;

	case SCIDX_BE:
		return MBUF_SC_BE;

	case SCIDX_RD:
		return MBUF_SC_RD;

	case SCIDX_OAM:
		return MBUF_SC_OAM;

	case SCIDX_AV:
		return MBUF_SC_AV;

	case SCIDX_RV:
		return MBUF_SC_RV;

	case SCIDX_VI:
		return MBUF_SC_VI;

	case SCIDX_VO:
		return MBUF_SC_VO;

	case SCIDX_CTL:
		return MBUF_SC_CTL;

	default:
		break;
	}

	VERIFY(0);
	                
	return sc;
}

mbuf_svc_class_t
m_service_class_from_val(u_int32_t v)
{
	mbuf_svc_class_t sc = MBUF_SC_BE;

	switch (v) {
	case SCVAL_BK_SYS:
		return MBUF_SC_BK_SYS;

	case SCVAL_BK:
		return MBUF_SC_BK;

	case SCVAL_BE:
		return MBUF_SC_BE;

	case SCVAL_RD:
		return MBUF_SC_RD;

	case SCVAL_OAM:
		return MBUF_SC_OAM;

	case SCVAL_AV:
		return MBUF_SC_AV;

	case SCVAL_RV:
		return MBUF_SC_RV;

	case SCVAL_VI:
		return MBUF_SC_VI;

	case SCVAL_VO:
		return MBUF_SC_VO;

	case SCVAL_CTL:
		return MBUF_SC_CTL;

	default:
		break;
	}

	VERIFY(0);
	                
	return sc;
}

uint16_t
m_adj_sum16(struct mbuf *m, uint32_t start, uint32_t dataoff,
    uint32_t datalen, uint32_t sum)
{
	uint32_t total_sub = 0;                                        
	uint32_t mlen = m_pktlen(m);                              
	uint32_t bytes = (dataoff + datalen);                             
	int len;

	ASSERT(bytes <= mlen);

	  
                                                           
                                                            
                                  
    
	len = (dataoff - start);
	if (len > 0) {
		total_sub = m_sum16(m, start, len);
	} else if (len < 0) {
		sum += m_sum16(m, dataoff, -len);
	}

	  
                                                            
    
	len = (mlen - bytes);
	if (len > 0) {
		struct mbuf *m0 = m;
		uint32_t extra = m_sum16(m, bytes, len);
		uint32_t off = bytes, off0 = off;

		while (off > 0) {
			if (__improbable(m == NULL)) {
				panic("%s: invalid mbuf chain %p [off %u, "
				    "len %u]", __func__, m0, off0, len);
				                
			}
			if (off < m->m_len) {
				break;
			}
			off -= m->m_len;
			m = m->m_next;
		}

		                                                    
		if ((uintptr_t)(mtod(m, uint8_t *) + off) & 1) {
			total_sub += ((extra << 8) & 0xffff) | (extra >> 8);
		} else {
			total_sub += extra;
		}

		total_sub = (total_sub >> 16) + (total_sub & 0xffff);
	}

	  
                                                  
    
	if (total_sub != 0) {
		if (total_sub >= sum) {
			sum = ~(total_sub - sum) & 0xffff;
		} else {
			sum -= total_sub;
		}
	}

	                           
	sum = (sum >> 16) + (sum & 0xffff);                 
	sum = (sum >> 16) + (sum & 0xffff);                         
	sum = (sum >> 16) + (sum & 0xffff);                      

	return sum & 0xffff;
}

uint16_t
m_sum16(struct mbuf *m, uint32_t off, uint32_t len)
{
	int mlen;

	  
                
   
                                                               
                                                             
                   
    
	if ((mlen = m_length2(m, NULL)) < (off + len)) {
		panic("%s: mbuf %p len (%d) < off+len (%d+%d)\n", __func__,
		    m, mlen, off, len);
		                
	}

	return (uint16_t)os_cpu_in_cksum_mbuf(m, len, off, 0);
}
