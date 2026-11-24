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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mcache.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <kern/queue.h>
#include <kern/locks.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/icmp6.h>

#include <net/net_osdep.h>
#include <dev/random/randomdev.h>

  
                                                                   
   
#define IN6_IFSTAT_STRICT
struct  ip6asfrag {
	struct ip6asfrag *ip6af_down;
	struct ip6asfrag *ip6af_up;
	struct mbuf     *ip6af_m;
	int             ip6af_offset;                                         
	int             ip6af_frglen;                                 
	int             ip6af_off;                           
	u_int16_t       ip6af_mff;                                         
};

#define IP6_REASS_MBUF(ip6af) ((ip6af)->ip6af_m)

MBUFQ_HEAD(fq6_head);

static void frag6_save_context(struct mbuf *, int);
static void frag6_scrub_context(struct mbuf *);
static int frag6_restore_context(struct mbuf *);

static void frag6_icmp6_paramprob_error(struct fq6_head *);
static void frag6_icmp6_timeex_error(struct fq6_head *);

static void frag6_enq(struct ip6asfrag *, struct ip6asfrag *);
static void frag6_deq(struct ip6asfrag *);
static void frag6_insque(struct ip6q *, struct ip6q *);
static void frag6_remque(struct ip6q *);
static void frag6_purgef(struct ip6q *, struct fq6_head *, struct fq6_head *);
static void frag6_freef(struct ip6q *, struct fq6_head *, struct fq6_head *);

static int frag6_timeout_run;                                                
static void frag6_timeout(void *);
static void frag6_sched_timeout(void);

static struct ip6q *ip6q_alloc(int);
static void ip6q_free(struct ip6q *);
static void ip6q_updateparams(void);
static struct ip6asfrag *ip6af_alloc(int);
static void ip6af_free(struct ip6asfrag *);

decl_lck_mtx_data(static, ip6qlock);
static lck_attr_t       *ip6qlock_attr;
static lck_grp_t        *ip6qlock_grp;
static lck_grp_attr_t   *ip6qlock_grp_attr;

                                                             
static struct ip6q ip6q;                                           
static int ip6_maxfragpackets;                                           
static u_int32_t frag6_nfragpackets;                                      
static int ip6_maxfrags;                                                   
static u_int32_t frag6_nfrags;                                              
static u_int32_t ip6q_limit;                                       
static u_int32_t ip6q_count;                                               
static u_int32_t ip6af_limit;                                           
static u_int32_t ip6af_count;                                                   

static int sysctl_maxfragpackets SYSCTL_HANDLER_ARGS;
static int sysctl_maxfrags SYSCTL_HANDLER_ARGS;

SYSCTL_DECL(_net_inet6_ip6);

SYSCTL_PROC(_net_inet6_ip6, IPV6CTL_MAXFRAGPACKETS, maxfragpackets,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &ip6_maxfragpackets, 0,
    sysctl_maxfragpackets, "I",
    "Maximum number of IPv6 fragment reassembly queue entries");

SYSCTL_UINT(_net_inet6_ip6, OID_AUTO, fragpackets,
    CTLFLAG_RD | CTLFLAG_LOCKED, &frag6_nfragpackets, 0,
    "Current number of IPv6 fragment reassembly queue entries");

SYSCTL_PROC(_net_inet6_ip6, IPV6CTL_MAXFRAGS, maxfrags,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &ip6_maxfrags, 0,
    sysctl_maxfrags, "I", "Maximum number of IPv6 fragments allowed");

  
                                                       
   
void
frag6_init(void)
{
	                                                                
	_CASSERT(sizeof(struct ip6q) <= _MLEN);
	                                                                 
	_CASSERT(sizeof(struct ip6asfrag) <= _MLEN);

	                                         
	ip6qlock_grp_attr  = lck_grp_attr_alloc_init();
	ip6qlock_grp = lck_grp_alloc_init("ip6qlock", ip6qlock_grp_attr);
	ip6qlock_attr = lck_attr_alloc_init();
	lck_mtx_init(&ip6qlock, ip6qlock_grp, ip6qlock_attr);

	lck_mtx_lock(&ip6qlock);
	                                       
	ip6q.ip6q_next = ip6q.ip6q_prev = &ip6q;

	                         
	ip6_maxfragpackets = nmbclusters / 32;
	ip6_maxfrags = ip6_maxfragpackets * 2;
	ip6q_updateparams();
	lck_mtx_unlock(&ip6qlock);
}

static void
frag6_save_context(struct mbuf *m, int val)
{
	m->m_pkthdr.pkt_hdr = (void *)(uintptr_t)val;
}

static void
frag6_scrub_context(struct mbuf *m)
{
	m->m_pkthdr.pkt_hdr = NULL;
}

static int
frag6_restore_context(struct mbuf *m)
{
	return (int)m->m_pkthdr.pkt_hdr;
}

  
                                                                          
                                                                          
                                  
   
static void
frag6_icmp6_paramprob_error(struct fq6_head *diq6)
{
	LCK_MTX_ASSERT(&ip6qlock, LCK_MTX_ASSERT_NOTOWNED);

	if (!MBUFQ_EMPTY(diq6)) {
		struct mbuf *merr, *merr_tmp;
		int param;
		MBUFQ_FOREACH_SAFE(merr, diq6, merr_tmp) {
			MBUFQ_REMOVE(diq6, merr);
			MBUFQ_NEXT(merr) = NULL;
			param = frag6_restore_context(merr);
			frag6_scrub_context(merr);
			icmp6_error(merr, ICMP6_PARAM_PROB,
			    ICMP6_PARAMPROB_HEADER, param);
		}
	}
}

  
                                                       
                                       
   
static void
frag6_icmp6_timeex_error(struct fq6_head *diq6)
{
	LCK_MTX_ASSERT(&ip6qlock, LCK_MTX_ASSERT_NOTOWNED);

	if (!MBUFQ_EMPTY(diq6)) {
		struct mbuf *m, *m_tmp;
		MBUFQ_FOREACH_SAFE(m, diq6, m_tmp) {
			MBUFQ_REMOVE(diq6, m);
			MBUFQ_NEXT(m) = NULL;
			icmp6_error_flag(m, ICMP6_TIME_EXCEEDED,
			    ICMP6_TIME_EXCEED_REASSEMBLY, 0, 0);
		}
	}
}

  
                                                                         
                                                             
                                                                              
                                                      
  
                       
                                    
                                                            
                                                
                                                  
  
                         
                                                                 
                                                           
                              
                                                  
  
                                                                             
                                                            
  
                         
                                                              
                                                                    
                                                               
                                                  
  
                                                                          
   
  
                 
   
int
frag6_input(struct mbuf **mp, int *offp, int proto)
{
#pragma unused(proto)
	struct mbuf *m = *mp, *t = NULL;
	struct ip6_hdr *ip6 = NULL;
	struct ip6_frag *ip6f = NULL;
	struct ip6q *q6 = NULL;
	struct ip6asfrag *af6 = NULL, *ip6af = NULL, *af6dwn = NULL;
	int offset = *offp, i = 0, next = 0;
	u_int8_t nxt = 0;
	int first_frag = 0;
	int fragoff = 0, frgpartlen = 0;                                           
	struct ifnet *dstifp = NULL;
	u_int8_t ecn = 0, ecn0 = 0;
	uint32_t csum = 0, csum_flags = 0;
	struct fq6_head diq6 = {};
	int locked = 0;
	boolean_t drop_fragq = FALSE;

	VERIFY(m->m_flags & M_PKTHDR);

	MBUFQ_INIT(&diq6);                                                  

	                                                                  
	MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(m);

	IP6_EXTHDR_CHECK(m, offset, sizeof(struct ip6_frag), goto done);
	ip6 = mtod(m, struct ip6_hdr *);
	ip6f = (struct ip6_frag *)((caddr_t)ip6 + offset);

#ifdef IN6_IFSTAT_STRICT
	                                                   
	if (m->m_pkthdr.pkt_flags & PKTF_IFAINFO) {
		uint32_t idx;

		if (ip6_getdstifaddr_info(m, &idx, NULL) == 0) {
			if (idx > 0 && idx <= if_index) {
				ifnet_head_lock_shared();
				dstifp = ifindex2ifnet[idx];
				ifnet_head_done();
			}
		}
	}
#endif                        

	                                                                  
	if (dstifp == NULL) {
		dstifp = m->m_pkthdr.rcvif;
	}

	                                                   
	if (ip6->ip6_plen == 0) {
		icmp6_error(m, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER, offset);
		in6_ifstat_inc(dstifp, ifs6_reass_fail);
		m = NULL;
		goto done;
	}

	  
                                                      
                         
                                
                               
    
	if ((ip6f->ip6f_offlg & IP6F_MORE_FRAG) &&
	    (((ntohs(ip6->ip6_plen) - offset) & 0x7) != 0)) {
		icmp6_error(m, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER,
		    offsetof(struct ip6_hdr, ip6_plen));
		in6_ifstat_inc(dstifp, ifs6_reass_fail);
		m = NULL;
		goto done;
	}

	                                                                        
	if (ip6_maxfragpackets == 0 || ip6_maxfrags == 0) {
		ip6stat.ip6s_fragments++;
		ip6stat.ip6s_fragdropped++;
		in6_ifstat_inc(dstifp, ifs6_reass_fail);
		m_freem(m);
		m = NULL;
		goto done;
	}

	                                       
	offset += sizeof(struct ip6_frag);

	  
                                                                   
                                                                         
    
	if ((ip6f->ip6f_offlg & ~IP6F_RESERVED_MASK) == 0) {
		  
                                
                                          
                                          
                                                  
                                                   
              
                                                            
     
		m->m_pkthdr.pkt_flags |= PKTF_REASSEMBLED;
		ip6stat.ip6s_atmfrag_rcvd++;
		in6_ifstat_inc(dstifp, ifs6_atmfrag_rcvd);
		*mp = m;
		*offp = offset;
		return ip6f->ip6f_nxt;
	}

	  
                                                                  
                                    
   
                                                                  
                                                                  
                                                                  
    
	if (ip6f->ip6f_nxt == IPPROTO_UDP &&
	    offset == (sizeof(*ip6) + sizeof(*ip6f)) &&
	    (m->m_pkthdr.csum_flags &
	    (CSUM_DATA_VALID | CSUM_PARTIAL | CSUM_PSEUDO_HDR)) ==
	    (CSUM_DATA_VALID | CSUM_PARTIAL)) {
		uint32_t start = m->m_pkthdr.csum_rx_start;
		uint32_t ip_len = (sizeof(*ip6) + ntohs(ip6->ip6_plen));
		int32_t trailer = (m_pktlen(m) - ip_len);
		uint32_t swbytes = (uint32_t)trailer;

		csum = m->m_pkthdr.csum_rx_val;

		ASSERT(trailer >= 0);
		if (start != offset || trailer != 0) {
			uint16_t s = 0, d = 0;

			if (IN6_IS_SCOPE_EMBED(&ip6->ip6_src)) {
				s = ip6->ip6_src.s6_addr16[1];
				ip6->ip6_src.s6_addr16[1] = 0;
			}
			if (IN6_IS_SCOPE_EMBED(&ip6->ip6_dst)) {
				d = ip6->ip6_dst.s6_addr16[1];
				ip6->ip6_dst.s6_addr16[1] = 0;
			}

			                         
			csum = m_adj_sum16(m, start, offset,
			    (ip_len - offset), csum);
			if (offset > start) {
				swbytes += (offset - start);
			} else {
				swbytes += (start - offset);
			}

			if (IN6_IS_SCOPE_EMBED(&ip6->ip6_src)) {
				ip6->ip6_src.s6_addr16[1] = s;
			}
			if (IN6_IS_SCOPE_EMBED(&ip6->ip6_dst)) {
				ip6->ip6_dst.s6_addr16[1] = d;
			}
		}
		csum_flags = m->m_pkthdr.csum_flags;

		if (swbytes != 0) {
			udp_in6_cksum_stats(swbytes);
		}
		if (trailer != 0) {
			m_adj(m, -trailer);
		}
	} else {
		csum = 0;
		csum_flags = 0;
	}

	                         
	m->m_pkthdr.csum_flags &= ~CSUM_DATA_VALID;

	ip6stat.ip6s_fragments++;
	in6_ifstat_inc(dstifp, ifs6_reass_reqd);

	lck_mtx_lock(&ip6qlock);
	locked = 1;

	for (q6 = ip6q.ip6q_next; q6 != &ip6q; q6 = q6->ip6q_next) {
		if (ip6f->ip6f_ident == q6->ip6q_ident &&
		    IN6_ARE_ADDR_EQUAL(&ip6->ip6_src, &q6->ip6q_src) &&
		    IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &q6->ip6q_dst)) {
			break;
		}
	}

	if (q6 == &ip6q) {
		  
                                                               
            
                                                            
                                                           
                      
     
		first_frag = 1;

		q6 = ip6q_alloc(M_DONTWAIT);
		if (q6 == NULL) {
			goto dropfrag;
		}

		frag6_insque(q6, &ip6q);
		frag6_nfragpackets++;

		                                                           
		q6->ip6q_down   = q6->ip6q_up = (struct ip6asfrag *)q6;
#ifdef notyet
		q6->ip6q_nxtp   = (u_char *)nxtp;
#endif
		q6->ip6q_ident  = ip6f->ip6f_ident;
		q6->ip6q_ttl    = IPV6_FRAGTTL;
		q6->ip6q_src    = ip6->ip6_src;
		q6->ip6q_dst    = ip6->ip6_dst;
		q6->ip6q_ecn    =
		    (ntohl(ip6->ip6_flow) >> 20) & IPTOS_ECN_MASK;
		q6->ip6q_unfrglen = -1;                                        

		q6->ip6q_nfrag = 0;
		q6->ip6q_flags = 0;

		  
                                                     
                                                      
     
		if (csum_flags != 0) {
			q6->ip6q_csum = csum;
			q6->ip6q_csum_flags = csum_flags;
		}
	}

	if (q6->ip6q_flags & IP6QF_DIRTY) {
		goto dropfrag;
	}

	  
                                                      
                                                                   
    
	fragoff = ntohs(ip6f->ip6f_offlg & IP6F_OFF_MASK);
	if (fragoff == 0) {
		q6->ip6q_unfrglen = offset - sizeof(struct ip6_hdr) -
		    sizeof(struct ip6_frag);
		q6->ip6q_nxt = ip6f->ip6f_nxt;
	}

	  
                                                                  
            
                                                                      
    
	frgpartlen = sizeof(struct ip6_hdr) + ntohs(ip6->ip6_plen) - offset;
	if (q6->ip6q_unfrglen >= 0) {
		                                           
		if (q6->ip6q_unfrglen + fragoff + frgpartlen > IPV6_MAXPACKET) {
			lck_mtx_unlock(&ip6qlock);
			locked = 0;
			icmp6_error(m, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER,
			    offset - sizeof(struct ip6_frag) +
			    offsetof(struct ip6_frag, ip6f_offlg));
			m = NULL;
			goto done;
		}
	} else if (fragoff + frgpartlen > IPV6_MAXPACKET) {
		lck_mtx_unlock(&ip6qlock);
		locked = 0;
		icmp6_error(m, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER,
		    offset - sizeof(struct ip6_frag) +
		    offsetof(struct ip6_frag, ip6f_offlg));
		m = NULL;
		goto done;
	}
	  
                                                           
                                                    
    
	if (fragoff == 0) {
		  
                                                
                                                                  
                                                                   
                                                                 
                                                                    
     
		if (!ip6_pkt_has_ulp(m)) {
			lck_mtx_unlock(&ip6qlock);
			locked = 0;
			icmp6_error(m, ICMP6_PARAM_PROB,
			    ICMP6_PARAMPROB_FIRSTFRAG_INCOMP_HDR, 0);
			m = NULL;
			goto done;
		}
		for (af6 = q6->ip6q_down; af6 != (struct ip6asfrag *)q6;
		    af6 = af6dwn) {
			af6dwn = af6->ip6af_down;

			if (q6->ip6q_unfrglen + af6->ip6af_off + af6->ip6af_frglen >
			    IPV6_MAXPACKET) {
				struct mbuf *merr = IP6_REASS_MBUF(af6);
				struct ip6_hdr *ip6err;
				int erroff = af6->ip6af_offset;

				                           
				frag6_deq(af6);
				ip6af_free(af6);

				                     
				ip6err = mtod(merr, struct ip6_hdr *);

				  
                                               
                                    
       
				ip6err->ip6_src = q6->ip6q_src;
				ip6err->ip6_dst = q6->ip6q_dst;

				frag6_save_context(merr,
				    erroff - sizeof(struct ip6_frag) +
				    offsetof(struct ip6_frag, ip6f_offlg));

				MBUFQ_ENQUEUE(&diq6, merr);
			}
		}
	}

	ip6af = ip6af_alloc(M_DONTWAIT);
	if (ip6af == NULL) {
		goto dropfrag;
	}

	ip6af->ip6af_mff = ip6f->ip6f_offlg & IP6F_MORE_FRAG;
	ip6af->ip6af_off = fragoff;
	ip6af->ip6af_frglen = frgpartlen;
	ip6af->ip6af_offset = offset;
	IP6_REASS_MBUF(ip6af) = m;

	if (first_frag) {
		af6 = (struct ip6asfrag *)q6;
		goto insert;
	}

	  
                                                            
                                 
                                                         
    
	ecn = (ntohl(ip6->ip6_flow) >> 20) & IPTOS_ECN_MASK;
	ecn0 = q6->ip6q_ecn;
	if (ecn == IPTOS_ECN_CE) {
		if (ecn0 == IPTOS_ECN_NOTECT) {
			ip6af_free(ip6af);
			goto dropfrag;
		}
		if (ecn0 != IPTOS_ECN_CE) {
			q6->ip6q_ecn = IPTOS_ECN_CE;
		}
	}
	if (ecn == IPTOS_ECN_NOTECT && ecn0 != IPTOS_ECN_NOTECT) {
		ip6af_free(ip6af);
		goto dropfrag;
	}

	  
                                                    
    
	for (af6 = q6->ip6q_down; af6 != (struct ip6asfrag *)q6;
	    af6 = af6->ip6af_down) {
		if (af6->ip6af_off > ip6af->ip6af_off) {
			break;
		}
	}

	  
                                                             
                                                                  
                              
                                               
   
                                                                    
                                                     
    
	if (af6->ip6af_up != (struct ip6asfrag *)q6) {
		if (af6->ip6af_up->ip6af_off == ip6af->ip6af_off) {
			if (af6->ip6af_up->ip6af_frglen != ip6af->ip6af_frglen) {
				drop_fragq = TRUE;
			} else {
				  
                                                    
                                                       
                                      
       
				ip6af_free(ip6af);
				goto dropfrag;
			}
		} else {
			i = af6->ip6af_up->ip6af_off + af6->ip6af_up->ip6af_frglen
			    - ip6af->ip6af_off;
			if (i > 0) {
				drop_fragq = TRUE;
			}
		}
	}

	if (af6 != (struct ip6asfrag *)q6) {
		  
                                                                
                                                           
                         
     
		VERIFY(af6->ip6af_off != ip6af->ip6af_off);
		i = (ip6af->ip6af_off + ip6af->ip6af_frglen) - af6->ip6af_off;
		if (i > 0) {
			drop_fragq = TRUE;
		}
	}

	  
                                                           
                                                                  
                                                             
    
	if (csum_flags != 0 && csum_flags == q6->ip6q_csum_flags) {
		q6->ip6q_csum += csum;
	} else if (q6->ip6q_csum_flags != 0) {
		q6->ip6q_csum_flags = 0;
	}

insert:
	  
                                   
                                  
                                            
                                               
    
	frag6_enq(ip6af, af6->ip6af_up);
	frag6_nfrags++;
	q6->ip6q_nfrag++;

	  
                                                           
                                                            
           
                                                                    
                                                                 
                                                           
    
	if (drop_fragq) {
		struct fq6_head dfq6 = {0};
		MBUFQ_INIT(&dfq6);                              
		q6->ip6q_flags |= IP6QF_DIRTY;
		                                                
		frag6_purgef(q6, &dfq6, NULL);
		af6 = NULL;

		                                          
		if (!MBUFQ_EMPTY(&dfq6)) {
			MBUFQ_DRAIN(&dfq6);
		}
		VERIFY(MBUFQ_EMPTY(&dfq6));
		  
                                                    
                       
                                                        
                                                     
                  
                                                       
                                                     
                                                  
                       
     
		if (!MBUFQ_EMPTY(&diq6)) {
			MBUFQ_DRAIN(&diq6);
		}
		VERIFY(MBUFQ_EMPTY(&diq6));
		  
                                                 
                           
                                                              
                                                          
     
		*mp = NULL;
		lck_mtx_unlock(&ip6qlock);
		return IPPROTO_DONE;
	}
	next = 0;
	for (af6 = q6->ip6q_down; af6 != (struct ip6asfrag *)q6;
	    af6 = af6->ip6af_down) {
		if (af6->ip6af_off != next) {
			lck_mtx_unlock(&ip6qlock);
			locked = 0;
			m = NULL;
			goto done;
		}
		next += af6->ip6af_frglen;
	}
	if (af6->ip6af_up->ip6af_mff) {
		lck_mtx_unlock(&ip6qlock);
		locked = 0;
		m = NULL;
		goto done;
	}

	  
                                                  
    
	ip6af = q6->ip6q_down;
	t = m = IP6_REASS_MBUF(ip6af);
	af6 = ip6af->ip6af_down;
	frag6_deq(ip6af);
	while (af6 != (struct ip6asfrag *)q6) {
		af6dwn = af6->ip6af_down;
		frag6_deq(af6);
		while (t->m_next) {
			t = t->m_next;
		}
		t->m_next = IP6_REASS_MBUF(af6);
		m_adj(t->m_next, af6->ip6af_offset);
		ip6af_free(af6);
		af6 = af6dwn;
	}

	  
                                                                 
                                                                
                         
    
	if (q6->ip6q_csum_flags != 0) {
		csum = q6->ip6q_csum;

		ADDCARRY(csum);

		m->m_pkthdr.csum_rx_val = (u_int16_t)csum;
		m->m_pkthdr.csum_rx_start = sizeof(struct ip6_hdr);
		m->m_pkthdr.csum_flags = q6->ip6q_csum_flags;
	} else if ((m->m_pkthdr.rcvif->if_flags & IFF_LOOPBACK) ||
	    (m->m_pkthdr.pkt_flags & PKTF_LOOP)) {
		                                      
		m->m_pkthdr.csum_data = 0xffff;
		m->m_pkthdr.csum_flags = CSUM_DATA_VALID | CSUM_PSEUDO_HDR;
	}

	                                                                  
	offset = ip6af->ip6af_offset - sizeof(struct ip6_frag);
	ip6af_free(ip6af);
	ip6 = mtod(m, struct ip6_hdr *);
	ip6->ip6_plen = htons((uint16_t)(next + offset - sizeof(struct ip6_hdr)));
	ip6->ip6_src = q6->ip6q_src;
	ip6->ip6_dst = q6->ip6q_dst;
	if (q6->ip6q_ecn == IPTOS_ECN_CE) {
		ip6->ip6_flow |= htonl(IPTOS_ECN_CE << 20);
	}

	nxt = q6->ip6q_nxt;
#ifdef notyet
	*q6->ip6q_nxtp = (u_char)(nxt & 0xff);
#endif

	                         
	if (m->m_len >= offset + sizeof(struct ip6_frag)) {
		                                                        
		ovbcopy((caddr_t)ip6, (caddr_t)ip6 + sizeof(struct ip6_frag),
		    offset);
		m->m_data += sizeof(struct ip6_frag);
		m->m_len -= sizeof(struct ip6_frag);
	} else {
		                                                           
		if ((t = m_split(m, offset, M_DONTWAIT)) == NULL) {
			frag6_remque(q6);
			frag6_nfragpackets--;
			frag6_nfrags -= q6->ip6q_nfrag;
			ip6q_free(q6);
			goto dropfrag;
		}
		m_adj(t, sizeof(struct ip6_frag));
		m_cat(m, t);
	}

	  
                              
    
	{
		char *prvnxtp = ip6_get_prevhdr(m, offset);          
		*prvnxtp = nxt;
	}

	frag6_remque(q6);
	frag6_nfragpackets--;
	frag6_nfrags -= q6->ip6q_nfrag;
	ip6q_free(q6);

	if (m->m_flags & M_PKTHDR) {                               
		m_fixhdr(m);
		  
                               
                                          
                                          
                                                  
                                                   
              
     
		m->m_pkthdr.pkt_flags |= PKTF_REASSEMBLED;
	}
	ip6stat.ip6s_reassembled++;

	  
                                       
    
	*mp = m;
	*offp = offset;

	                                                                  
	frag6_sched_timeout();
	lck_mtx_unlock(&ip6qlock);
	in6_ifstat_inc(dstifp, ifs6_reass_ok);
	frag6_icmp6_paramprob_error(&diq6);
	VERIFY(MBUFQ_EMPTY(&diq6));
	return nxt;

done:
	VERIFY(m == NULL);
	*mp = m;
	if (!locked) {
		if (frag6_nfragpackets == 0) {
			frag6_icmp6_paramprob_error(&diq6);
			VERIFY(MBUFQ_EMPTY(&diq6));
			return IPPROTO_DONE;
		}
		lck_mtx_lock(&ip6qlock);
	}
	                                                                  
	frag6_sched_timeout();
	lck_mtx_unlock(&ip6qlock);
	frag6_icmp6_paramprob_error(&diq6);
	VERIFY(MBUFQ_EMPTY(&diq6));
	return IPPROTO_DONE;

dropfrag:
	ip6stat.ip6s_fragdropped++;
	                                                                  
	frag6_sched_timeout();
	lck_mtx_unlock(&ip6qlock);
	in6_ifstat_inc(dstifp, ifs6_reass_fail);
	m_freem(m);
	*mp = NULL;
	frag6_icmp6_paramprob_error(&diq6);
	VERIFY(MBUFQ_EMPTY(&diq6));
	return IPPROTO_DONE;
}

  
                                                                    
                                                                         
             
                                                                            
                                                                  
                                                                 
                  
                                                    
   
static void
frag6_purgef(struct ip6q *q6, struct fq6_head *dfq6, struct fq6_head *diq6)
{
	struct ip6asfrag *af6 = NULL;
	struct ip6asfrag *down6 = NULL;

	LCK_MTX_ASSERT(&ip6qlock, LCK_MTX_ASSERT_OWNED);

	for (af6 = q6->ip6q_down; af6 != (struct ip6asfrag *)q6;
	    af6 = down6) {
		struct mbuf *m = IP6_REASS_MBUF(af6);

		down6 = af6->ip6af_down;
		frag6_deq(af6);

		  
                                                    
                                                     
                                                      
                
     
		if (af6->ip6af_off == 0 && diq6 != NULL) {
			struct ip6_hdr *ip6;

			                    
			ip6 = mtod(m, struct ip6_hdr *);

			                                              
			ip6->ip6_src = q6->ip6q_src;
			ip6->ip6_dst = q6->ip6q_dst;
			MBUFQ_ENQUEUE(diq6, m);
		} else {
			MBUFQ_ENQUEUE(dfq6, m);
		}
		ip6af_free(af6);
	}
}

  
                                                                    
                                                                         
             
                                                                            
                                                                  
                                                                 
                  
                                                                         
   
static void
frag6_freef(struct ip6q *q6, struct fq6_head *dfq6, struct fq6_head *diq6)
{
	frag6_purgef(q6, dfq6, diq6);
	frag6_remque(q6);
	frag6_nfragpackets--;
	frag6_nfrags -= q6->ip6q_nfrag;
	ip6q_free(q6);
}

  
                                            
                                                    
   
void
frag6_enq(struct ip6asfrag *af6, struct ip6asfrag *up6)
{
	LCK_MTX_ASSERT(&ip6qlock, LCK_MTX_ASSERT_OWNED);

	af6->ip6af_up = up6;
	af6->ip6af_down = up6->ip6af_down;
	up6->ip6af_down->ip6af_up = af6;
	up6->ip6af_down = af6;
}

  
                                       
   
void
frag6_deq(struct ip6asfrag *af6)
{
	LCK_MTX_ASSERT(&ip6qlock, LCK_MTX_ASSERT_OWNED);

	af6->ip6af_up->ip6af_down = af6->ip6af_down;
	af6->ip6af_down->ip6af_up = af6->ip6af_up;
}

void
frag6_insque(struct ip6q *new, struct ip6q *old)
{
	LCK_MTX_ASSERT(&ip6qlock, LCK_MTX_ASSERT_OWNED);

	new->ip6q_prev = old;
	new->ip6q_next = old->ip6q_next;
	old->ip6q_next->ip6q_prev = new;
	old->ip6q_next = new;
}

void
frag6_remque(struct ip6q *p6)
{
	LCK_MTX_ASSERT(&ip6qlock, LCK_MTX_ASSERT_OWNED);

	p6->ip6q_prev->ip6q_next = p6->ip6q_next;
	p6->ip6q_next->ip6q_prev = p6->ip6q_prev;
}

  
                                      
                                     
                     
   
static void
frag6_timeout(void *arg)
{
#pragma unused(arg)
	struct fq6_head dfq6, diq6;
	struct fq6_head *diq6_tmp = NULL;
	struct ip6q *q6;

	MBUFQ_INIT(&dfq6);                              
	MBUFQ_INIT(&diq6);                                                  

	  
                                                                  
                                                                 
                                
    
	net_update_uptime();

	lck_mtx_lock(&ip6qlock);
	q6 = ip6q.ip6q_next;
	if (q6) {
		while (q6 != &ip6q) {
			--q6->ip6q_ttl;
			q6 = q6->ip6q_next;
			if (q6->ip6q_prev->ip6q_ttl == 0) {
				ip6stat.ip6s_fragtimeout++;
				                                              
				  
                                                              
                             
       
				diq6_tmp = (q6->ip6q_prev->ip6q_flags & IP6QF_DIRTY) ?
				    NULL : &diq6;
				frag6_freef(q6->ip6q_prev, &dfq6, diq6_tmp);
			}
		}
	}
	  
                                                  
                                               
                                        
    
	if (ip6_maxfragpackets >= 0) {
		while (frag6_nfragpackets > (unsigned)ip6_maxfragpackets &&
		    ip6q.ip6q_prev) {
			ip6stat.ip6s_fragoverflow++;
			                                              
			  
                                                             
                            
      
			diq6_tmp = (ip6q.ip6q_prev->ip6q_flags & IP6QF_DIRTY) ?
			    NULL : &diq6;
			frag6_freef(ip6q.ip6q_prev, &dfq6, diq6_tmp);
		}
	}
	                                                  
	frag6_timeout_run = 0;
	frag6_sched_timeout();
	lck_mtx_unlock(&ip6qlock);

	                                          
	if (!MBUFQ_EMPTY(&dfq6)) {
		MBUFQ_DRAIN(&dfq6);
	}

	frag6_icmp6_timeex_error(&diq6);

	VERIFY(MBUFQ_EMPTY(&dfq6));
	VERIFY(MBUFQ_EMPTY(&diq6));
}

static void
frag6_sched_timeout(void)
{
	LCK_MTX_ASSERT(&ip6qlock, LCK_MTX_ASSERT_OWNED);

	if (!frag6_timeout_run && frag6_nfragpackets > 0) {
		frag6_timeout_run = 1;
		timeout(frag6_timeout, NULL, hz);
	}
}

  
                                    
   
void
frag6_drain(void)
{
	struct fq6_head dfq6, diq6;
	struct fq6_head *diq6_tmp = NULL;

	MBUFQ_INIT(&dfq6);                              
	MBUFQ_INIT(&diq6);                                                  

	lck_mtx_lock(&ip6qlock);
	while (ip6q.ip6q_next != &ip6q) {
		ip6stat.ip6s_fragdropped++;
		                                              
		  
                                                            
                           
     
		diq6_tmp = (ip6q.ip6q_next->ip6q_flags & IP6QF_DIRTY) ?
		    NULL : &diq6;
		frag6_freef(ip6q.ip6q_next, &dfq6, diq6_tmp);
	}
	lck_mtx_unlock(&ip6qlock);

	                                          
	if (!MBUFQ_EMPTY(&dfq6)) {
		MBUFQ_DRAIN(&dfq6);
	}

	frag6_icmp6_timeex_error(&diq6);

	VERIFY(MBUFQ_EMPTY(&dfq6));
	VERIFY(MBUFQ_EMPTY(&diq6));
}

static struct ip6q *
ip6q_alloc(int how)
{
	struct mbuf *t;
	struct ip6q *q6;

	  
                                                                 
                                                                    
                                     
    
	if (ip6q_limit > 0 && ip6q_count > ip6q_limit) {
		return NULL;
	}

	t = m_get(how, MT_FTABLE);
	if (t != NULL) {
		atomic_add_32(&ip6q_count, 1);
		q6 = mtod(t, struct ip6q *);
		bzero(q6, sizeof(*q6));
	} else {
		q6 = NULL;
	}
	return q6;
}

static void
ip6q_free(struct ip6q *q6)
{
	(void) m_free(dtom(q6));
	atomic_add_32(&ip6q_count, -1);
}

static struct ip6asfrag *
ip6af_alloc(int how)
{
	struct mbuf *t;
	struct ip6asfrag *af6;

	  
                                                                 
                                                              
                                     
    
	if (ip6af_limit > 0 && ip6af_count > ip6af_limit) {
		return NULL;
	}

	t = m_get(how, MT_FTABLE);
	if (t != NULL) {
		atomic_add_32(&ip6af_count, 1);
		af6 = mtod(t, struct ip6asfrag *);
		bzero(af6, sizeof(*af6));
	} else {
		af6 = NULL;
	}
	return af6;
}

static void
ip6af_free(struct ip6asfrag *af6)
{
	(void) m_free(dtom(af6));
	atomic_add_32(&ip6af_count, -1);
}

static void
ip6q_updateparams(void)
{
	LCK_MTX_ASSERT(&ip6qlock, LCK_MTX_ASSERT_OWNED);
	  
                                
    
	if (ip6_maxfragpackets < 0) {
		ip6q_limit = 0;
	}
	if (ip6_maxfrags < 0) {
		ip6af_limit = 0;
	}
	  
                                       
    
	if (ip6_maxfragpackets > 0) {
		ip6q_limit = ip6_maxfragpackets;
	}
	if (ip6_maxfrags > 0) {
		ip6af_limit = ip6_maxfrags;
	}
	  
                                                                  
                                                                    
                                                  
    
	if (ip6_maxfragpackets == 0) {
		ip6q_limit = 1;
	}
	if (ip6_maxfrags == 0) {
		ip6af_limit = 1;
	}
	  
                                                                
    
	frag6_sched_timeout();
}

static int
sysctl_maxfragpackets SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error, i;

	lck_mtx_lock(&ip6qlock);
	i = ip6_maxfragpackets;
	error = sysctl_handle_int(oidp, &i, 0, req);
	if (error || req->newptr == USER_ADDR_NULL) {
		goto done;
	}
	                   
	if (i < -1 || i > (nmbclusters / 4)) {
		error = EINVAL;
		goto done;
	}
	ip6_maxfragpackets = i;
	ip6q_updateparams();
done:
	lck_mtx_unlock(&ip6qlock);
	return error;
}

static int
sysctl_maxfrags SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)
	int error, i;

	lck_mtx_lock(&ip6qlock);
	i = ip6_maxfrags;
	error = sysctl_handle_int(oidp, &i, 0, req);
	if (error || req->newptr == USER_ADDR_NULL) {
		goto done;
	}
	                   
	if (i < -1 || i > (nmbclusters / 4)) {
		error = EINVAL;
		goto done;
	}
	ip6_maxfrags = i;
	ip6q_updateparams();                                     
done:
	lck_mtx_unlock(&ip6qlock);
	return error;
}
