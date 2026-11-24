/*
 * Copyright (c) 2007-2020 Apple Inc. All rights reserved.
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
 * Copyright 2001 Niels Provos <provos@citi.umich.edu>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/filio.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/random.h>
#include <sys/mcache.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/bpf.h>
#include <net/route.h>
#include <net/if_pflog.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_fsm.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>

#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

#include <net/pfvar.h>

struct pf_frent {
	LIST_ENTRY(pf_frent)    fr_next;
	struct mbuf             *fr_m;
#define fr_ip           fr_u.fru_ipv4
#define fr_ip6          fr_u.fru_ipv6
	union {
		struct ip       *fru_ipv4;
		struct ip6_hdr  *fru_ipv6;
	} fr_u;
	struct ip6_frag         fr_ip6f_opt;
	uint16_t                fr_ip6f_hlen;                            
	uint16_t                fr_ip6f_extoff;                                        
};

struct pf_frcache {
	LIST_ENTRY(pf_frcache) fr_next;
	uint16_t        fr_off;
	uint16_t        fr_end;
};

#define PFFRAG_SEENLAST 0x0001                                               
#define PFFRAG_NOBUFFER 0x0002                                            
#define PFFRAG_DROP     0x0004                                  
#define BUFFER_FRAGMENTS(fr)    (!((fr)->fr_flags & PFFRAG_NOBUFFER))

struct pf_fragment {
	RB_ENTRY(pf_fragment) fr_entry;
	TAILQ_ENTRY(pf_fragment) frag_next;
	struct pf_addr  fr_srcx;
	struct pf_addr  fr_dstx;
	u_int8_t        fr_p;                                          
	u_int8_t        fr_flags;                         
	u_int16_t       fr_max;                                
#define fr_id           fr_uid.fru_id4
#define fr_id6          fr_uid.fru_id6
	union {
		u_int16_t       fru_id4;
		u_int32_t       fru_id6;
	} fr_uid;
	int             fr_af;
	u_int32_t       fr_timeout;
#define fr_queue        fr_u.fru_queue
#define fr_cache        fr_u.fru_cache
	union {
		LIST_HEAD(pf_fragq, pf_frent) fru_queue;                       
		LIST_HEAD(pf_cacheq, pf_frcache) fru_cache;                  
	} fr_u;
	uint32_t        fr_csum_flags;                      
	uint32_t        fr_csum;                                    
	uint16_t        fr_ip6_maxlen;                                                   
};

static TAILQ_HEAD(pf_fragqueue, pf_fragment)    pf_fragqueue;
static TAILQ_HEAD(pf_cachequeue, pf_fragment)   pf_cachequeue;

static __inline int  pf_frag_compare(struct pf_fragment *,
    struct pf_fragment *);
static RB_HEAD(pf_frag_tree, pf_fragment)       pf_frag_tree, pf_cache_tree;
RB_PROTOTYPE_SC(__private_extern__, pf_frag_tree, pf_fragment, fr_entry,
    pf_frag_compare);
RB_GENERATE(pf_frag_tree, pf_fragment, fr_entry, pf_frag_compare);

                        
static void pf_ip6hdr2key(struct pf_fragment *, struct ip6_hdr *,
    struct ip6_frag *);
static void pf_ip2key(struct pf_fragment *, struct ip *);
static void pf_remove_fragment(struct pf_fragment *);
static void pf_flush_fragments(void);
static void pf_free_fragment(struct pf_fragment *);
static struct pf_fragment *pf_find_fragment_by_key(struct pf_fragment *,
    struct pf_frag_tree *);
static __inline struct pf_fragment *
pf_find_fragment_by_ipv4_header(struct ip *, struct pf_frag_tree *);
static struct mbuf *pf_reassemble(struct mbuf *, struct pf_fragment **,
    struct pf_frent *, int);
static struct mbuf *pf_fragcache(struct mbuf **, struct ip *,
    struct pf_fragment **, int, int, int *);
static int pf_normalize_tcpopt(struct pf_rule *, int, struct pfi_kif *,
    struct pf_pdesc *, pbuf_t *, struct tcphdr *, int, int *);
static __inline struct pf_fragment *
pf_find_fragment_by_ipv6_header(struct ip6_hdr *, struct ip6_frag *,
    struct pf_frag_tree *);
static struct mbuf *pf_reassemble6(struct mbuf **, struct pf_fragment **,
    struct pf_frent *, int);
static struct mbuf *pf_frag6cache(struct mbuf **, struct ip6_hdr*,
    struct ip6_frag *, struct pf_fragment **, int, int, int, int *);

#define DPFPRINTF(x) do {                               \
	if (pf_status.debug >= PF_DEBUG_MISC) {         \
	        printf("%s: ", __func__);               \
	        printf x ;                              \
	}                                               \
} while (0)

             
struct pool              pf_frent_pl, pf_frag_pl;
static struct pool       pf_cache_pl, pf_cent_pl;
struct pool              pf_state_scrub_pl;

static int               pf_nfrents, pf_ncache;

void
pf_normalize_init(void)
{
	pool_init(&pf_frent_pl, sizeof(struct pf_frent), 0, 0, 0, "pffrent",
	    NULL);
	pool_init(&pf_frag_pl, sizeof(struct pf_fragment), 0, 0, 0, "pffrag",
	    NULL);
	pool_init(&pf_cache_pl, sizeof(struct pf_fragment), 0, 0, 0,
	    "pffrcache", NULL);
	pool_init(&pf_cent_pl, sizeof(struct pf_frcache), 0, 0, 0, "pffrcent",
	    NULL);
	pool_init(&pf_state_scrub_pl, sizeof(struct pf_state_scrub), 0, 0, 0,
	    "pfstscr", NULL);

	pool_sethiwat(&pf_frag_pl, PFFRAG_FRAG_HIWAT);
	pool_sethardlimit(&pf_frent_pl, PFFRAG_FRENT_HIWAT, NULL, 0);
	pool_sethardlimit(&pf_cache_pl, PFFRAG_FRCACHE_HIWAT, NULL, 0);
	pool_sethardlimit(&pf_cent_pl, PFFRAG_FRCENT_HIWAT, NULL, 0);

	TAILQ_INIT(&pf_fragqueue);
	TAILQ_INIT(&pf_cachequeue);
}

#if 0
void
pf_normalize_destroy(void)
{
	pool_destroy(&pf_state_scrub_pl);
	pool_destroy(&pf_cent_pl);
	pool_destroy(&pf_cache_pl);
	pool_destroy(&pf_frag_pl);
	pool_destroy(&pf_frent_pl);
}
#endif

int
pf_normalize_isempty(void)
{
	return TAILQ_EMPTY(&pf_fragqueue) && TAILQ_EMPTY(&pf_cachequeue);
}

static __inline int
pf_frag_compare(struct pf_fragment *a, struct pf_fragment *b)
{
	int     diff;

	if ((diff = a->fr_af - b->fr_af)) {
		return diff;
	} else if ((diff = a->fr_p - b->fr_p)) {
		return diff;
	} else {
		struct pf_addr *sa = &a->fr_srcx;
		struct pf_addr *sb = &b->fr_srcx;
		struct pf_addr *da = &a->fr_dstx;
		struct pf_addr *db = &b->fr_dstx;

		switch (a->fr_af) {
#ifdef INET
		case AF_INET:
			if ((diff = a->fr_id - b->fr_id)) {
				return diff;
			} else if (sa->v4addr.s_addr < sb->v4addr.s_addr) {
				return -1;
			} else if (sa->v4addr.s_addr > sb->v4addr.s_addr) {
				return 1;
			} else if (da->v4addr.s_addr < db->v4addr.s_addr) {
				return -1;
			} else if (da->v4addr.s_addr > db->v4addr.s_addr) {
				return 1;
			}
			break;
#endif
		case AF_INET6:
			if ((diff = a->fr_id6 - b->fr_id6)) {
				return diff;
			} else if (sa->addr32[3] < sb->addr32[3]) {
				return -1;
			} else if (sa->addr32[3] > sb->addr32[3]) {
				return 1;
			} else if (sa->addr32[2] < sb->addr32[2]) {
				return -1;
			} else if (sa->addr32[2] > sb->addr32[2]) {
				return 1;
			} else if (sa->addr32[1] < sb->addr32[1]) {
				return -1;
			} else if (sa->addr32[1] > sb->addr32[1]) {
				return 1;
			} else if (sa->addr32[0] < sb->addr32[0]) {
				return -1;
			} else if (sa->addr32[0] > sb->addr32[0]) {
				return 1;
			} else if (da->addr32[3] < db->addr32[3]) {
				return -1;
			} else if (da->addr32[3] > db->addr32[3]) {
				return 1;
			} else if (da->addr32[2] < db->addr32[2]) {
				return -1;
			} else if (da->addr32[2] > db->addr32[2]) {
				return 1;
			} else if (da->addr32[1] < db->addr32[1]) {
				return -1;
			} else if (da->addr32[1] > db->addr32[1]) {
				return 1;
			} else if (da->addr32[0] < db->addr32[0]) {
				return -1;
			} else if (da->addr32[0] > db->addr32[0]) {
				return 1;
			}
			break;
		default:
			VERIFY(!0 && "only IPv4 and IPv6 supported!");
			break;
		}
	}
	return 0;
}

void
pf_purge_expired_fragments(void)
{
	struct pf_fragment *frag;
	u_int32_t expire = pf_time_second() -
	    pf_default_rule.timeout[PFTM_FRAG];

	while ((frag = TAILQ_LAST(&pf_fragqueue, pf_fragqueue)) != NULL) {
		VERIFY(BUFFER_FRAGMENTS(frag));
		if (frag->fr_timeout > expire) {
			break;
		}

		switch (frag->fr_af) {
		case AF_INET:
			DPFPRINTF(("expiring IPv4 %d(0x%llx) from queue.\n",
			    ntohs(frag->fr_id),
			    (uint64_t)VM_KERNEL_ADDRPERM(frag)));
			break;
		case AF_INET6:
			DPFPRINTF(("expiring IPv6 %d(0x%llx) from queue.\n",
			    ntohl(frag->fr_id6),
			    (uint64_t)VM_KERNEL_ADDRPERM(frag)));
			break;
		default:
			VERIFY(0 && "only IPv4 and IPv6 supported");
			break;
		}
		pf_free_fragment(frag);
	}

	while ((frag = TAILQ_LAST(&pf_cachequeue, pf_cachequeue)) != NULL) {
		VERIFY(!BUFFER_FRAGMENTS(frag));
		if (frag->fr_timeout > expire) {
			break;
		}

		switch (frag->fr_af) {
		case AF_INET:
			DPFPRINTF(("expiring IPv4 %d(0x%llx) from cache.\n",
			    ntohs(frag->fr_id),
			    (uint64_t)VM_KERNEL_ADDRPERM(frag)));
			break;
		case AF_INET6:
			DPFPRINTF(("expiring IPv6 %d(0x%llx) from cache.\n",
			    ntohl(frag->fr_id6),
			    (uint64_t)VM_KERNEL_ADDRPERM(frag)));
			break;
		default:
			VERIFY(0 && "only IPv4 and IPv6 supported");
			break;
		}
		pf_free_fragment(frag);
		VERIFY(TAILQ_EMPTY(&pf_cachequeue) ||
		    TAILQ_LAST(&pf_cachequeue, pf_cachequeue) != frag);
	}
}

  
                                                        
   

static void
pf_flush_fragments(void)
{
	struct pf_fragment      *frag;
	int                      goal;

	goal = pf_nfrents * 9 / 10;
	DPFPRINTF(("trying to free > %d frents\n",
	    pf_nfrents - goal));
	while (goal < pf_nfrents) {
		frag = TAILQ_LAST(&pf_fragqueue, pf_fragqueue);
		if (frag == NULL) {
			break;
		}
		pf_free_fragment(frag);
	}


	goal = pf_ncache * 9 / 10;
	DPFPRINTF(("trying to free > %d cache entries\n",
	    pf_ncache - goal));
	while (goal < pf_ncache) {
		frag = TAILQ_LAST(&pf_cachequeue, pf_cachequeue);
		if (frag == NULL) {
			break;
		}
		pf_free_fragment(frag);
	}
}

                                                    

static void
pf_free_fragment(struct pf_fragment *frag)
{
	struct pf_frent         *frent;
	struct pf_frcache       *frcache;

	                        
	if (BUFFER_FRAGMENTS(frag)) {
		for (frent = LIST_FIRST(&frag->fr_queue); frent;
		    frent = LIST_FIRST(&frag->fr_queue)) {
			LIST_REMOVE(frent, fr_next);

			m_freem(frent->fr_m);
			pool_put(&pf_frent_pl, frent);
			pf_nfrents--;
		}
	} else {
		for (frcache = LIST_FIRST(&frag->fr_cache); frcache;
		    frcache = LIST_FIRST(&frag->fr_cache)) {
			LIST_REMOVE(frcache, fr_next);

			VERIFY(LIST_EMPTY(&frag->fr_cache) ||
			    LIST_FIRST(&frag->fr_cache)->fr_off >
			    frcache->fr_end);

			pool_put(&pf_cent_pl, frcache);
			pf_ncache--;
		}
	}

	pf_remove_fragment(frag);
}

static void
pf_ip6hdr2key(struct pf_fragment *key, struct ip6_hdr *ip6,
    struct ip6_frag *fh)
{
	key->fr_p = fh->ip6f_nxt;
	key->fr_id6 = fh->ip6f_ident;
	key->fr_af = AF_INET6;
	key->fr_srcx.v6addr = ip6->ip6_src;
	key->fr_dstx.v6addr = ip6->ip6_dst;
}

static void
pf_ip2key(struct pf_fragment *key, struct ip *ip)
{
	key->fr_p = ip->ip_p;
	key->fr_id = ip->ip_id;
	key->fr_af = AF_INET;
	key->fr_srcx.v4addr.s_addr = ip->ip_src.s_addr;
	key->fr_dstx.v4addr.s_addr = ip->ip_dst.s_addr;
}

static struct pf_fragment *
pf_find_fragment_by_key(struct pf_fragment *key, struct pf_frag_tree *tree)
{
	struct pf_fragment *frag;

	frag = RB_FIND(pf_frag_tree, tree, key);
	if (frag != NULL) {
		                                                    
		frag->fr_timeout = pf_time_second();
		if (BUFFER_FRAGMENTS(frag)) {
			TAILQ_REMOVE(&pf_fragqueue, frag, frag_next);
			TAILQ_INSERT_HEAD(&pf_fragqueue, frag, frag_next);
		} else {
			TAILQ_REMOVE(&pf_cachequeue, frag, frag_next);
			TAILQ_INSERT_HEAD(&pf_cachequeue, frag, frag_next);
		}
	}

	return frag;
}

static __inline struct pf_fragment *
pf_find_fragment_by_ipv4_header(struct ip *ip, struct pf_frag_tree *tree)
{
	struct pf_fragment key;
	pf_ip2key(&key, ip);
	return pf_find_fragment_by_key(&key, tree);
}

                                                                       
static void
pf_remove_fragment(struct pf_fragment *frag)
{
	if (BUFFER_FRAGMENTS(frag)) {
		RB_REMOVE(pf_frag_tree, &pf_frag_tree, frag);
		TAILQ_REMOVE(&pf_fragqueue, frag, frag_next);
		pool_put(&pf_frag_pl, frag);
	} else {
		RB_REMOVE(pf_frag_tree, &pf_cache_tree, frag);
		TAILQ_REMOVE(&pf_cachequeue, frag, frag_next);
		pool_put(&pf_cache_pl, frag);
	}
}

#define FR_IP_OFF(fr)   ((ntohs((fr)->fr_ip->ip_off) & IP_OFFMASK) << 3)
static struct mbuf *
pf_reassemble(struct mbuf *m0, struct pf_fragment **frag,
    struct pf_frent *frent, int mff)
{
	struct mbuf     *m = m0, *m2;
	struct pf_frent *frea, *next;
	struct pf_frent *frep = NULL;
	struct ip       *ip = frent->fr_ip;
	uint32_t         hlen = ip->ip_hl << 2;
	u_int16_t        off = (ntohs(ip->ip_off) & IP_OFFMASK) << 3;
	u_int16_t        ip_len = ntohs(ip->ip_len) - ip->ip_hl * 4;
	u_int16_t        fr_max = ip_len + off;
	uint32_t         csum, csum_flags;

	VERIFY(*frag == NULL || BUFFER_FRAGMENTS(*frag));

	  
                                                                    
                                                                  
                     
   
                                                                  
                                                                     
                                                                    
                                                                   
                                                                    
                                                                  
    
	if (ip->ip_p == IPPROTO_UDP && hlen == sizeof(struct ip) &&
	    (m->m_pkthdr.csum_flags &
	    (CSUM_DATA_VALID | CSUM_PARTIAL | CSUM_PSEUDO_HDR)) ==
	    (CSUM_DATA_VALID | CSUM_PARTIAL)) {
		uint32_t start = m->m_pkthdr.csum_rx_start;
		int32_t trailer = (m_pktlen(m) - ntohs(ip->ip_len));
		uint32_t swbytes = (uint32_t)trailer;

		csum = m->m_pkthdr.csum_rx_val;

		ASSERT(trailer >= 0);
		if ((start != 0 && start != hlen) || trailer != 0) {
#if BYTE_ORDER != BIG_ENDIAN
			if (start < hlen) {
				HTONS(ip->ip_len);
				HTONS(ip->ip_off);
			}
#endif                               
			                         
			csum = m_adj_sum16(m, start, hlen,
			    (ip->ip_len - hlen), csum);
			if (hlen > start) {
				swbytes += (hlen - start);
			} else {
				swbytes += (start - hlen);
			}
#if BYTE_ORDER != BIG_ENDIAN
			if (start < hlen) {
				NTOHS(ip->ip_off);
				NTOHS(ip->ip_len);
			}
#endif                               
		}
		csum_flags = m->m_pkthdr.csum_flags;

		if (swbytes != 0) {
			udp_in_cksum_stats(swbytes);
		}
		if (trailer != 0) {
			m_adj(m, -trailer);
		}
	} else {
		csum = 0;
		csum_flags = 0;
	}

	                         
	m->m_pkthdr.csum_flags &= ~CSUM_DATA_VALID;

	                         
	m->m_data += hlen;
	m->m_len -= hlen;

	                                                   
	if (*frag == NULL) {
		*frag = pool_get(&pf_frag_pl, PR_NOWAIT);
		if (*frag == NULL) {
			pf_flush_fragments();
			*frag = pool_get(&pf_frag_pl, PR_NOWAIT);
			if (*frag == NULL) {
				goto drop_fragment;
			}
		}

		(*frag)->fr_flags = 0;
		(*frag)->fr_max = 0;
		(*frag)->fr_af = AF_INET;
		(*frag)->fr_srcx.v4addr = frent->fr_ip->ip_src;
		(*frag)->fr_dstx.v4addr = frent->fr_ip->ip_dst;
		(*frag)->fr_p = frent->fr_ip->ip_p;
		(*frag)->fr_id = frent->fr_ip->ip_id;
		(*frag)->fr_timeout = pf_time_second();
		if (csum_flags != 0) {
			(*frag)->fr_csum_flags = csum_flags;
			(*frag)->fr_csum = csum;
		}
		LIST_INIT(&(*frag)->fr_queue);

		RB_INSERT(pf_frag_tree, &pf_frag_tree, *frag);
		TAILQ_INSERT_HEAD(&pf_fragqueue, *frag, frag_next);

		                                        
		frep = NULL;
		goto insert;
	}

	  
                                                           
                                                                  
                                                             
    
	if (csum_flags != 0 && csum_flags == (*frag)->fr_csum_flags) {
		(*frag)->fr_csum += csum;
	} else if ((*frag)->fr_csum_flags != 0) {
		(*frag)->fr_csum_flags = 0;
	}

	  
                                          
                                            
    
	LIST_FOREACH(frea, &(*frag)->fr_queue, fr_next) {
		if (FR_IP_OFF(frea) > off) {
			break;
		}
		frep = frea;
	}

	VERIFY(frep != NULL || frea != NULL);

	if (frep != NULL &&
	    FR_IP_OFF(frep) + ntohs(frep->fr_ip->ip_len) - frep->fr_ip->ip_hl *
	    4 > off) {
		u_int16_t       precut;

		precut = FR_IP_OFF(frep) + ntohs(frep->fr_ip->ip_len) -
		    frep->fr_ip->ip_hl * 4 - off;
		if (precut >= ip_len) {
			goto drop_fragment;
		}
		m_adj(frent->fr_m, precut);
		DPFPRINTF(("overlap -%d\n", precut));
		                               
		ip->ip_off = htons(ntohs(ip->ip_off) + (precut >> 3));
		off = (ntohs(ip->ip_off) & IP_OFFMASK) << 3;
		ip_len -= precut;
		ip->ip_len = htons(ip_len);
	}

	for (; frea != NULL && ip_len + off > FR_IP_OFF(frea);
	    frea = next) {
		u_int16_t       aftercut;

		aftercut = ip_len + off - FR_IP_OFF(frea);
		DPFPRINTF(("adjust overlap %d\n", aftercut));
		if (aftercut < ntohs(frea->fr_ip->ip_len) - frea->fr_ip->ip_hl
		    * 4) {
			frea->fr_ip->ip_len =
			    htons(ntohs(frea->fr_ip->ip_len) - aftercut);
			frea->fr_ip->ip_off = htons(ntohs(frea->fr_ip->ip_off) +
			    (aftercut >> 3));
			m_adj(frea->fr_m, aftercut);
			break;
		}

		                                                     
		next = LIST_NEXT(frea, fr_next);
		m_freem(frea->fr_m);
		LIST_REMOVE(frea, fr_next);
		pool_put(&pf_frent_pl, frea);
		pf_nfrents--;
	}

insert:
	                              
	if ((*frag)->fr_max < fr_max) {
		(*frag)->fr_max = fr_max;
	}
	                              
	if (!mff) {
		(*frag)->fr_flags |= PFFRAG_SEENLAST;
	}

	if (frep == NULL) {
		LIST_INSERT_HEAD(&(*frag)->fr_queue, frent, fr_next);
	} else {
		LIST_INSERT_AFTER(frep, frent, fr_next);
	}

	                                            
	if (!((*frag)->fr_flags & PFFRAG_SEENLAST)) {
		return NULL;
	}

	                                   
	off = 0;
	for (frep = LIST_FIRST(&(*frag)->fr_queue); frep; frep = next) {
		next = LIST_NEXT(frep, fr_next);

		off += ntohs(frep->fr_ip->ip_len) - frep->fr_ip->ip_hl * 4;
		if (off < (*frag)->fr_max &&
		    (next == NULL || FR_IP_OFF(next) != off)) {
			DPFPRINTF(("missing fragment at %d, next %d, max %d\n",
			    off, next == NULL ? -1 : FR_IP_OFF(next),
			    (*frag)->fr_max));
			return NULL;
		}
	}
	DPFPRINTF(("%d < %d?\n", off, (*frag)->fr_max));
	if (off < (*frag)->fr_max) {
		return NULL;
	}

	                          
	frent = LIST_FIRST(&(*frag)->fr_queue);
	VERIFY(frent != NULL);
	if ((frent->fr_ip->ip_hl << 2) + off > IP_MAXPACKET) {
		DPFPRINTF(("drop: too big: %d\n", off));
		pf_free_fragment(*frag);
		*frag = NULL;
		return NULL;
	}
	next = LIST_NEXT(frent, fr_next);

	                         
	ip = frent->fr_ip;
	m = frent->fr_m;
	m2 = m->m_next;
	m->m_next = NULL;
	m_cat(m, m2);
	pool_put(&pf_frent_pl, frent);
	pf_nfrents--;
	for (frent = next; frent != NULL; frent = next) {
		next = LIST_NEXT(frent, fr_next);

		m2 = frent->fr_m;
		pool_put(&pf_frent_pl, frent);
		pf_nfrents--;
		m_cat(m, m2);
	}

	ip->ip_src = (*frag)->fr_srcx.v4addr;
	ip->ip_dst = (*frag)->fr_dstx.v4addr;

	if ((*frag)->fr_csum_flags != 0) {
		csum = (*frag)->fr_csum;

		ADDCARRY(csum);

		m->m_pkthdr.csum_rx_val = csum;
		m->m_pkthdr.csum_rx_start = sizeof(struct ip);
		m->m_pkthdr.csum_flags = (*frag)->fr_csum_flags;
	} else if ((m->m_pkthdr.rcvif->if_flags & IFF_LOOPBACK) ||
	    (m->m_pkthdr.pkt_flags & PKTF_LOOP)) {
		                                      
		m->m_pkthdr.csum_data = 0xffff;
		m->m_pkthdr.csum_flags =
		    CSUM_DATA_VALID | CSUM_PSEUDO_HDR |
		    CSUM_IP_CHECKED | CSUM_IP_VALID;
	}

	                                
	pf_remove_fragment(*frag);
	*frag = NULL;

	hlen = ip->ip_hl << 2;
	ip->ip_len = htons(off + hlen);
	m->m_len += hlen;
	m->m_data -= hlen;

	                                                               
	                                       
	if (m->m_flags & M_PKTHDR) {
		int plen = 0;
		for (m2 = m; m2; m2 = m2->m_next) {
			plen += m2->m_len;
		}
		m->m_pkthdr.len = plen;
	}

	DPFPRINTF(("complete: 0x%llx(%d)\n",
	    (uint64_t)VM_KERNEL_ADDRPERM(m), ntohs(ip->ip_len)));
	return m;

drop_fragment:
	                                    
	pool_put(&pf_frent_pl, frent);
	pf_nfrents--;
	m_freem(m);
	return NULL;
}

static struct mbuf *
pf_fragcache(struct mbuf **m0, struct ip *h, struct pf_fragment **frag, int mff,
    int drop, int *nomem)
{
	struct mbuf             *m = *m0;
	struct pf_frcache       *frp, *fra, *cur = NULL;
	int                      ip_len = ntohs(h->ip_len) - (h->ip_hl << 2);
	u_int16_t                off = ntohs(h->ip_off) << 3;
	u_int16_t                fr_max = ip_len + off;
	int                      hosed = 0;

	VERIFY(*frag == NULL || !BUFFER_FRAGMENTS(*frag));

	                                              
	if (*frag == NULL) {
		*frag = pool_get(&pf_cache_pl, PR_NOWAIT);
		if (*frag == NULL) {
			pf_flush_fragments();
			*frag = pool_get(&pf_cache_pl, PR_NOWAIT);
			if (*frag == NULL) {
				goto no_mem;
			}
		}

		                                
		cur = pool_get(&pf_cent_pl, PR_NOWAIT);
		if (cur == NULL) {
			pool_put(&pf_cache_pl, *frag);
			*frag = NULL;
			goto no_mem;
		}
		pf_ncache++;

		(*frag)->fr_flags = PFFRAG_NOBUFFER;
		(*frag)->fr_max = 0;
		(*frag)->fr_af = AF_INET;
		(*frag)->fr_srcx.v4addr = h->ip_src;
		(*frag)->fr_dstx.v4addr = h->ip_dst;
		(*frag)->fr_p = h->ip_p;
		(*frag)->fr_id = h->ip_id;
		(*frag)->fr_timeout = pf_time_second();

		cur->fr_off = off;
		cur->fr_end = fr_max;
		LIST_INIT(&(*frag)->fr_cache);
		LIST_INSERT_HEAD(&(*frag)->fr_cache, cur, fr_next);

		RB_INSERT(pf_frag_tree, &pf_cache_tree, *frag);
		TAILQ_INSERT_HEAD(&pf_cachequeue, *frag, frag_next);

		DPFPRINTF(("fragcache[%d]: new %d-%d\n", h->ip_id, off,
		    fr_max));

		goto pass;
	}

	  
                                          
                                            
    
	frp = NULL;
	LIST_FOREACH(fra, &(*frag)->fr_cache, fr_next) {
		if (fra->fr_off > off) {
			break;
		}
		frp = fra;
	}

	VERIFY(frp != NULL || fra != NULL);

	if (frp != NULL) {
		int     precut;

		precut = frp->fr_end - off;
		if (precut >= ip_len) {
			                                      
			DPFPRINTF(("fragcache[%d]: dead (%d-%d) %d-%d\n",
			    h->ip_id, frp->fr_off, frp->fr_end, off, fr_max));
			goto drop_fragment;
		}
		if (precut == 0) {
			                                           
			DPFPRINTF(("fragcache[%d]: adjacent (%d-%d) %d-%d\n",
			    h->ip_id, frp->fr_off, frp->fr_end, off, fr_max));
			frp->fr_end = fr_max;
		} else if (precut > 0) {
			  
                                                    
                                            
                                                     
                                                    
                                             
      

			DPFPRINTF(("fragcache[%d]: chop %d (%d-%d) %d-%d\n",
			    h->ip_id, precut, frp->fr_off, frp->fr_end, off,
			    fr_max));

			off += precut;
			fr_max -= precut;
			                                                    
			frp->fr_end = fr_max;

			if (!drop) {
				  
                                   
                                                    
                                                  
                                                    
                                                
                                           
       
				*m0 = m_copym(m, 0, h->ip_hl << 2, M_NOWAIT);
				if (*m0 == NULL) {
					goto no_mem;
				}
				VERIFY((*m0)->m_next == NULL);
				m_adj(m, precut + (h->ip_hl << 2));
				m_cat(*m0, m);
				m = *m0;
				if (m->m_flags & M_PKTHDR) {
					int plen = 0;
					struct mbuf *t;
					for (t = m; t; t = t->m_next) {
						plen += t->m_len;
					}
					m->m_pkthdr.len = plen;
				}


				h = mtod(m, struct ip *);


				VERIFY((int)m->m_len ==
				    ntohs(h->ip_len) - precut);
				h->ip_off = htons(ntohs(h->ip_off) +
				    (precut >> 3));
				h->ip_len = htons(ntohs(h->ip_len) - precut);
			} else {
				hosed++;
			}
		} else {
			                                      

			DPFPRINTF(("fragcache[%d]: gap %d (%d-%d) %d-%d\n",
			    h->ip_id, -precut, frp->fr_off, frp->fr_end, off,
			    fr_max));

			cur = pool_get(&pf_cent_pl, PR_NOWAIT);
			if (cur == NULL) {
				goto no_mem;
			}
			pf_ncache++;

			cur->fr_off = off;
			cur->fr_end = fr_max;
			LIST_INSERT_AFTER(frp, cur, fr_next);
		}
	}

	if (fra != NULL) {
		int     aftercut;
		int     merge = 0;

		aftercut = fr_max - fra->fr_off;
		if (aftercut == 0) {
			                        
			DPFPRINTF(("fragcache[%d]: adjacent %d-%d (%d-%d)\n",
			    h->ip_id, off, fr_max, fra->fr_off, fra->fr_end));
			fra->fr_off = off;
			merge = 1;
		} else if (aftercut > 0) {
			                                                
			DPFPRINTF(("fragcache[%d]: chop %d %d-%d (%d-%d)\n",
			    h->ip_id, aftercut, off, fr_max, fra->fr_off,
			    fra->fr_end));
			fra->fr_off = off;
			fr_max -= aftercut;

			merge = 1;

			if (!drop) {
				m_adj(m, -aftercut);
				if (m->m_flags & M_PKTHDR) {
					int plen = 0;
					struct mbuf *t;
					for (t = m; t; t = t->m_next) {
						plen += t->m_len;
					}
					m->m_pkthdr.len = plen;
				}
				h = mtod(m, struct ip *);
				VERIFY((int)m->m_len ==
				    ntohs(h->ip_len) - aftercut);
				h->ip_len = htons(ntohs(h->ip_len) - aftercut);
			} else {
				hosed++;
			}
		} else if (frp == NULL) {
			                                      
			DPFPRINTF(("fragcache[%d]: gap %d %d-%d (%d-%d)\n",
			    h->ip_id, -aftercut, off, fr_max, fra->fr_off,
			    fra->fr_end));

			cur = pool_get(&pf_cent_pl, PR_NOWAIT);
			if (cur == NULL) {
				goto no_mem;
			}
			pf_ncache++;

			cur->fr_off = off;
			cur->fr_end = fr_max;
			LIST_INSERT_BEFORE(fra, cur, fr_next);
		}


		                                                             
		if (merge) {
			if (cur && fra->fr_off <= cur->fr_end) {
				                                       
				DPFPRINTF(("fragcache[%d]: adjacent(merge "
				    "%d-%d) %d-%d (%d-%d)\n",
				    h->ip_id, cur->fr_off, cur->fr_end, off,
				    fr_max, fra->fr_off, fra->fr_end));
				fra->fr_off = cur->fr_off;
				LIST_REMOVE(cur, fr_next);
				pool_put(&pf_cent_pl, cur);
				pf_ncache--;
				cur = NULL;
			} else if (frp && fra->fr_off <= frp->fr_end) {
				                                       
				VERIFY(cur == NULL);
				DPFPRINTF(("fragcache[%d]: adjacent(merge "
				    "%d-%d) %d-%d (%d-%d)\n",
				    h->ip_id, frp->fr_off, frp->fr_end, off,
				    fr_max, fra->fr_off, fra->fr_end));
				fra->fr_off = frp->fr_off;
				LIST_REMOVE(frp, fr_next);
				pool_put(&pf_cent_pl, frp);
				pf_ncache--;
				frp = NULL;
			}
		}
	}

	if (hosed) {
		  
                                                         
                                                          
                                                              
     
		goto drop_fragment;
	}


pass:
	                              
	if ((*frag)->fr_max < fr_max) {
		(*frag)->fr_max = fr_max;
	}

	                              
	if (!mff) {
		(*frag)->fr_flags |= PFFRAG_SEENLAST;
	}

	                                            
	if (((*frag)->fr_flags & PFFRAG_SEENLAST) &&
	    LIST_FIRST(&(*frag)->fr_cache)->fr_off == 0 &&
	    LIST_FIRST(&(*frag)->fr_cache)->fr_end == (*frag)->fr_max) {
		                                
		DPFPRINTF(("fragcache[%d]: done 0-%d\n", h->ip_id,
		    (*frag)->fr_max));
		pf_free_fragment(*frag);
		*frag = NULL;
	}

	return m;

no_mem:
	*nomem = 1;

	                                           
	if (!mff && *frag != NULL) {
		(*frag)->fr_flags |= PFFRAG_SEENLAST;
	}

	m_freem(m);
	return NULL;

drop_fragment:

	                                           
	if (!mff && *frag != NULL) {
		(*frag)->fr_flags |= PFFRAG_SEENLAST;
	}

	if (drop) {
		                                                     
		if (((*frag)->fr_flags & PFFRAG_DROP) == 0) {
			DPFPRINTF(("fragcache[%d]: dropping overall fragment\n",
			    h->ip_id));
		}
		(*frag)->fr_flags |= PFFRAG_DROP;
	}

	m_freem(m);
	return NULL;
}

#define FR_IP6_OFF(fr) \
	(ntohs((fr)->fr_ip6f_opt.ip6f_offlg & IP6F_OFF_MASK))
#define FR_IP6_PLEN(fr) (ntohs((fr)->fr_ip6->ip6_plen))
struct mbuf *
pf_reassemble6(struct mbuf **m0, struct pf_fragment **frag,
    struct pf_frent *frent, int mff)
{
	struct mbuf *m, *m2;
	struct pf_frent *frea, *frep, *next;
	struct ip6_hdr *ip6;
	struct ip6_frag *ip6f;
	int plen, off, fr_max, pktlen;
	uint32_t uoff, csum, csum_flags;

	VERIFY(*frag == NULL || BUFFER_FRAGMENTS(*frag));
	m = *m0;
	frep = NULL;
	ip6 = frent->fr_ip6;
	ip6f = &frent->fr_ip6f_opt;
	off = FR_IP6_OFF(frent);
	uoff = frent->fr_ip6f_hlen;
	plen = FR_IP6_PLEN(frent);
	fr_max = off + plen - (frent->fr_ip6f_hlen - sizeof(*ip6));
	pktlen = plen + sizeof(*ip6);

	DPFPRINTF(("0x%llx IPv6 frag plen %u off %u fr_ip6f_hlen %u "
	    "fr_max %u m_len %u\n", (uint64_t)VM_KERNEL_ADDRPERM(m), plen, off,
	    frent->fr_ip6f_hlen, fr_max, m->m_len));

	  
                                                                  
                                    
   
                                                                  
                                                                  
                                                                  
    
	if (ip6f->ip6f_nxt == IPPROTO_UDP &&
	    uoff == (sizeof(*ip6) + sizeof(*ip6f)) &&
	    (m->m_pkthdr.csum_flags &
	    (CSUM_DATA_VALID | CSUM_PARTIAL | CSUM_PSEUDO_HDR)) ==
	    (CSUM_DATA_VALID | CSUM_PARTIAL)) {
		uint32_t start = m->m_pkthdr.csum_rx_start;
		uint32_t ip_len = (sizeof(*ip6) + ntohs(ip6->ip6_plen));
		int32_t trailer = (m_pktlen(m) - ip_len);
		uint32_t swbytes = (uint32_t)trailer;

		csum = m->m_pkthdr.csum_rx_val;

		ASSERT(trailer >= 0);
		if (start != uoff || trailer != 0) {
			uint16_t s = 0, d = 0;

			if (IN6_IS_SCOPE_EMBED(&ip6->ip6_src)) {
				s = ip6->ip6_src.s6_addr16[1];
				ip6->ip6_src.s6_addr16[1] = 0;
			}
			if (IN6_IS_SCOPE_EMBED(&ip6->ip6_dst)) {
				d = ip6->ip6_dst.s6_addr16[1];
				ip6->ip6_dst.s6_addr16[1] = 0;
			}

			                         
			csum = m_adj_sum16(m, start, uoff,
			    (ip_len - uoff), csum);
			if (uoff > start) {
				swbytes += (uoff - start);
			} else {
				swbytes += (start - uoff);
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

	                                                  
	m->m_data += frent->fr_ip6f_hlen;
	m->m_len -= frent->fr_ip6f_hlen;

	                                                   
	if (*frag == NULL) {
		*frag = pool_get(&pf_frag_pl, PR_NOWAIT);
		if (*frag == NULL) {
			pf_flush_fragments();
			*frag = pool_get(&pf_frag_pl, PR_NOWAIT);
			if (*frag == NULL) {
				goto drop_fragment;
			}
		}

		(*frag)->fr_flags = 0;
		(*frag)->fr_max = 0;
		(*frag)->fr_ip6_maxlen = pktlen;
		(*frag)->fr_af = AF_INET6;
		(*frag)->fr_srcx.v6addr = frent->fr_ip6->ip6_src;
		(*frag)->fr_dstx.v6addr = frent->fr_ip6->ip6_dst;
		(*frag)->fr_p = frent->fr_ip6f_opt.ip6f_nxt;
		(*frag)->fr_id6 = frent->fr_ip6f_opt.ip6f_ident;
		(*frag)->fr_timeout = pf_time_second();
		if (csum_flags != 0) {
			(*frag)->fr_csum_flags = csum_flags;
			(*frag)->fr_csum = csum;
		}
		LIST_INIT(&(*frag)->fr_queue);

		RB_INSERT(pf_frag_tree, &pf_frag_tree, *frag);
		TAILQ_INSERT_HEAD(&pf_fragqueue, *frag, frag_next);

		                                        
		frep = NULL;
		goto insert;
	}

	                                                       
	if (pktlen > (*frag)->fr_ip6_maxlen) {
		(*frag)->fr_ip6_maxlen = pktlen;
	}
	  
                                                           
                                                                  
                                                             
    
	if (csum_flags != 0 && csum_flags == (*frag)->fr_csum_flags) {
		(*frag)->fr_csum += csum;
	} else if ((*frag)->fr_csum_flags != 0) {
		(*frag)->fr_csum_flags = 0;
	}

	  
                                          
                                            
    
	LIST_FOREACH(frea, &(*frag)->fr_queue, fr_next) {
		if (FR_IP6_OFF(frea) > off) {
			break;
		}
		frep = frea;
	}

	VERIFY(frep != NULL || frea != NULL);

	if (frep != NULL &&
	    FR_IP6_OFF(frep) + FR_IP6_PLEN(frep) - frep->fr_ip6f_hlen > off) {
		u_int16_t precut;

		precut = FR_IP6_OFF(frep) + FR_IP6_PLEN(frep) -
		    frep->fr_ip6f_hlen - off;
		if (precut >= plen) {
			goto drop_fragment;
		}
		m_adj(frent->fr_m, precut);
		DPFPRINTF(("overlap -%d\n", precut));
		                               
		frent->fr_ip6f_opt.ip6f_offlg =
		    htons(ntohs(frent->fr_ip6f_opt.ip6f_offlg) +
		    (precut >> 3));
		off = FR_IP6_OFF(frent);
		plen -= precut;
		ip6->ip6_plen = htons(plen);
	}

	for (; frea != NULL && plen + off > FR_IP6_OFF(frea); frea = next) {
		u_int16_t       aftercut;

		aftercut = plen + off - FR_IP6_OFF(frea);
		DPFPRINTF(("adjust overlap %d\n", aftercut));
		if (aftercut < FR_IP6_PLEN(frea) - frea->fr_ip6f_hlen) {
			frea->fr_ip6->ip6_plen = htons(FR_IP6_PLEN(frea) -
			    aftercut);
			frea->fr_ip6f_opt.ip6f_offlg =
			    htons(ntohs(frea->fr_ip6f_opt.ip6f_offlg) +
			    (aftercut >> 3));
			m_adj(frea->fr_m, aftercut);
			break;
		}

		                                                     
		next = LIST_NEXT(frea, fr_next);
		m_freem(frea->fr_m);
		LIST_REMOVE(frea, fr_next);
		pool_put(&pf_frent_pl, frea);
		pf_nfrents--;
	}

insert:
	                              
	if ((*frag)->fr_max < fr_max) {
		(*frag)->fr_max = fr_max;
	}
	                              
	if (!mff) {
		(*frag)->fr_flags |= PFFRAG_SEENLAST;
	}

	if (frep == NULL) {
		LIST_INSERT_HEAD(&(*frag)->fr_queue, frent, fr_next);
	} else {
		LIST_INSERT_AFTER(frep, frent, fr_next);
	}

	                                            
	if (!((*frag)->fr_flags & PFFRAG_SEENLAST)) {
		return NULL;
	}

	                                   
	off = 0;
	for (frep = LIST_FIRST(&(*frag)->fr_queue); frep; frep = next) {
		next = LIST_NEXT(frep, fr_next);
		off += FR_IP6_PLEN(frep) - (frent->fr_ip6f_hlen - sizeof *ip6);
		DPFPRINTF(("frep at %d, next %d, max %d\n",
		    off, next == NULL ? -1 : FR_IP6_OFF(next),
		    (*frag)->fr_max));
		if (off < (*frag)->fr_max &&
		    (next == NULL || FR_IP6_OFF(next) != off)) {
			DPFPRINTF(("missing fragment at %d, next %d, max %d\n",
			    off, next == NULL ? -1 : FR_IP6_OFF(next),
			    (*frag)->fr_max));
			return NULL;
		}
	}
	DPFPRINTF(("%d < %d?\n", off, (*frag)->fr_max));
	if (off < (*frag)->fr_max) {
		return NULL;
	}

	                          
	frent = LIST_FIRST(&(*frag)->fr_queue);
	VERIFY(frent != NULL);
	if (frent->fr_ip6f_hlen + off > IP_MAXPACKET) {
		DPFPRINTF(("drop: too big: %d\n", off));
		pf_free_fragment(*frag);
		*frag = NULL;
		return NULL;
	}

	ASSERT(*frag != NULL);
	ASSERT(frent != NULL);
	next = LIST_NEXT(frent, fr_next);
	if (next == NULL) {
		DPFPRINTF(("drop: atomic fragment\n"));
		pf_free_fragment(*frag);
		*frag = NULL;
		return NULL;
	}

	                                                            
	uint16_t hdrlen, unfragpartlen, extoff, maxlen;
	uint32_t id;

	                                                               
	hdrlen = frent->fr_ip6f_hlen - sizeof(struct ip6_frag);
	  
                                                                      
                        
    
	unfragpartlen = next->fr_ip6f_hlen - sizeof(struct ip6_frag);
	extoff = frent->fr_ip6f_extoff;
	maxlen = (*frag)->fr_ip6_maxlen;
	id = (*frag)->fr_id6;

	ip6 = frent->fr_ip6;
	ip6->ip6_nxt = (*frag)->fr_p;
	ip6->ip6_plen = htons(off);
	ip6->ip6_src = (*frag)->fr_srcx.v6addr;
	ip6->ip6_dst = (*frag)->fr_dstx.v6addr;

	if ((*frag)->fr_csum_flags != 0) {
		csum = (*frag)->fr_csum;

		ADDCARRY(csum);

		m->m_pkthdr.csum_rx_val = csum;
		m->m_pkthdr.csum_rx_start = sizeof(struct ip6_hdr);
		m->m_pkthdr.csum_flags = (*frag)->fr_csum_flags;
	} else if ((m->m_pkthdr.rcvif->if_flags & IFF_LOOPBACK) ||
	    (m->m_pkthdr.pkt_flags & PKTF_LOOP)) {
		                                      
		m->m_pkthdr.csum_data = 0xffff;
		m->m_pkthdr.csum_flags = CSUM_DATA_VALID | CSUM_PSEUDO_HDR;
	}

	                                
	pf_remove_fragment(*frag);
	*frag = NULL;

	m = frent->fr_m;
	m->m_len += sizeof(struct ip6_hdr);
	m->m_data -= sizeof(struct ip6_hdr);
	memmove(m->m_data, ip6, sizeof(struct ip6_hdr));

	next = LIST_NEXT(frent, fr_next);
	pool_put(&pf_frent_pl, frent);
	pf_nfrents--;
	for (frent = next; next != NULL; frent = next) {
		m2 = frent->fr_m;

		m_cat(m, m2);
		next = LIST_NEXT(frent, fr_next);
		pool_put(&pf_frent_pl, frent);
		pf_nfrents--;
	}

	                                       
	if (m->m_flags & M_PKTHDR) {
		int len = 0;
		for (m2 = m; m2; m2 = m2->m_next) {
			len += m2->m_len;
		}
		m->m_pkthdr.len = len;
	}

	DPFPRINTF(("complete: 0x%llx ip6_plen %d m_pkthdr.len %d\n",
	    (uint64_t)VM_KERNEL_ADDRPERM(m), ntohs(ip6->ip6_plen),
	    m->m_pkthdr.len));

	                             
	struct m_tag *mtag;
	struct pf_fragment_tag *ftag;
	mtag = m_tag_create(KERNEL_MODULE_TAG_ID, KERNEL_TAG_TYPE_PF_REASS,
	    sizeof(*ftag), M_NOWAIT, m);
	if (mtag == NULL) {
		                    
		m_freem(m);
		return NULL;
	}
	ftag = (struct pf_fragment_tag *)(mtag + 1);
	ftag->ft_hdrlen = hdrlen;
	ftag->ft_unfragpartlen = unfragpartlen;
	ftag->ft_extoff = extoff;
	ftag->ft_maxlen = maxlen;
	ftag->ft_id = id;
	m_tag_prepend(m, mtag);

	struct pf_mtag *pftag = pf_get_mtag(m);
	ASSERT(pftag != NULL);
	pftag->pftag_flags |= PF_TAG_REASSEMBLED;
	return m;

drop_fragment:
	                                    
	pool_put(&pf_frent_pl, frent);
	--pf_nfrents;
	m_freem(m);
	return NULL;
}

static struct mbuf *
pf_frag6cache(struct mbuf **m0, struct ip6_hdr *h, struct ip6_frag *fh,
    struct pf_fragment **frag, int hlen, int mff, int drop, int *nomem)
{
	struct mbuf *m = *m0;
	u_int16_t plen, off, fr_max;
	struct pf_frcache *frp, *fra, *cur = NULL;
	int hosed = 0;

	VERIFY(*frag == NULL || !BUFFER_FRAGMENTS(*frag));
	m = *m0;
	off = ntohs(fh->ip6f_offlg & IP6F_OFF_MASK);
	plen = ntohs(h->ip6_plen) - (hlen - sizeof *h);

	  
                                                                  
                                                               
                                                             
                                                                  
                                                   
   
                                                                          
                                                                          
                                  
                                                                         
                                   
                                                                         
                                   
                                                                           
                                                      
    
	fr_max = off + plen;

	DPFPRINTF(("0x%llx plen %u off %u fr_max %u\n",
	    (uint64_t)VM_KERNEL_ADDRPERM(m), plen, off, fr_max));

	                                              
	if (*frag == NULL) {
		*frag = pool_get(&pf_cache_pl, PR_NOWAIT);
		if (*frag == NULL) {
			pf_flush_fragments();
			*frag = pool_get(&pf_cache_pl, PR_NOWAIT);
			if (*frag == NULL) {
				goto no_mem;
			}
		}

		                                
		cur = pool_get(&pf_cent_pl, PR_NOWAIT);
		if (cur == NULL) {
			pool_put(&pf_cache_pl, *frag);
			*frag = NULL;
			goto no_mem;
		}
		pf_ncache++;

		(*frag)->fr_flags = PFFRAG_NOBUFFER;
		(*frag)->fr_max = 0;
		(*frag)->fr_af = AF_INET6;
		(*frag)->fr_srcx.v6addr = h->ip6_src;
		(*frag)->fr_dstx.v6addr = h->ip6_dst;
		(*frag)->fr_p = fh->ip6f_nxt;
		(*frag)->fr_id6 = fh->ip6f_ident;
		(*frag)->fr_timeout = pf_time_second();

		cur->fr_off = off;
		cur->fr_end = fr_max;
		LIST_INIT(&(*frag)->fr_cache);
		LIST_INSERT_HEAD(&(*frag)->fr_cache, cur, fr_next);

		RB_INSERT(pf_frag_tree, &pf_cache_tree, *frag);
		TAILQ_INSERT_HEAD(&pf_cachequeue, *frag, frag_next);

		DPFPRINTF(("frag6cache[%d]: new %d-%d\n", ntohl(fh->ip6f_ident),
		    off, fr_max));

		goto pass;
	}

	  
                                          
                                            
    
	frp = NULL;
	LIST_FOREACH(fra, &(*frag)->fr_cache, fr_next) {
		if (fra->fr_off > off) {
			break;
		}
		frp = fra;
	}

	VERIFY(frp != NULL || fra != NULL);

	if (frp != NULL) {
		int precut;

		precut = frp->fr_end - off;
		if (precut >= plen) {
			                                      
			DPFPRINTF(("frag6cache[%u]: dead (%d-%d) %d-%d\n",
			    ntohl(fh->ip6f_ident), frp->fr_off, frp->fr_end,
			    off, fr_max));
			goto drop_fragment;
		}
		if (precut == 0) {
			                                           
			DPFPRINTF(("frag6cache[%u]: adjacent (%d-%d) %d-%d\n",
			    ntohl(fh->ip6f_ident), frp->fr_off, frp->fr_end,
			    off, fr_max));
			frp->fr_end = fr_max;
		} else if (precut > 0) {
			                                                 
                                            
                                                     
                                                    
                                             
      

			DPFPRINTF(("frag6cache[%u]: chop %d (%d-%d) %d-%d\n",
			    ntohl(fh->ip6f_ident), precut, frp->fr_off,
			    frp->fr_end, off, fr_max));

			off += precut;
			fr_max -= precut;
			                                                    
			frp->fr_end = fr_max;

			if (!drop) {
				                               
                                                    
                                                  
                                                    
                                                
                                           
       
				*m0 = m_copym(m, 0, hlen, M_NOWAIT);
				if (*m0 == NULL) {
					goto no_mem;
				}
				VERIFY((*m0)->m_next == NULL);
				m_adj(m, precut + hlen);
				m_cat(*m0, m);
				m = *m0;
				if (m->m_flags & M_PKTHDR) {
					int pktlen = 0;
					struct mbuf *t;
					for (t = m; t; t = t->m_next) {
						pktlen += t->m_len;
					}
					m->m_pkthdr.len = pktlen;
				}

				h = mtod(m, struct ip6_hdr *);

				VERIFY((int)m->m_len ==
				    ntohs(h->ip6_plen) - precut);
				fh->ip6f_offlg &= ~IP6F_OFF_MASK;
				fh->ip6f_offlg |=
				    htons(ntohs(fh->ip6f_offlg & IP6F_OFF_MASK)
				    + (precut >> 3));
				h->ip6_plen = htons(ntohs(h->ip6_plen) -
				    precut);
			} else {
				hosed++;
			}
		} else {
			                                      

			DPFPRINTF(("frag6cache[%u]: gap %d (%d-%d) %d-%d\n",
			    ntohl(fh->ip6f_ident), -precut, frp->fr_off,
			    frp->fr_end, off, fr_max));

			cur = pool_get(&pf_cent_pl, PR_NOWAIT);
			if (cur == NULL) {
				goto no_mem;
			}
			pf_ncache++;

			cur->fr_off = off;
			cur->fr_end = fr_max;
			LIST_INSERT_AFTER(frp, cur, fr_next);
		}
	}

	if (fra != NULL) {
		int     aftercut;
		int     merge = 0;

		aftercut = fr_max - fra->fr_off;
		if (aftercut == 0) {
			                        
			DPFPRINTF(("frag6cache[%u]: adjacent %d-%d (%d-%d)\n",
			    ntohl(fh->ip6f_ident), off, fr_max, fra->fr_off,
			    fra->fr_end));
			fra->fr_off = off;
			merge = 1;
		} else if (aftercut > 0) {
			                                                
			DPFPRINTF(("frag6cache[%u]: chop %d %d-%d (%d-%d)\n",
			    ntohl(fh->ip6f_ident), aftercut, off, fr_max,
			    fra->fr_off, fra->fr_end));
			fra->fr_off = off;
			fr_max -= aftercut;

			merge = 1;

			if (!drop) {
				m_adj(m, -aftercut);
				if (m->m_flags & M_PKTHDR) {
					int pktlen = 0;
					struct mbuf *t;
					for (t = m; t; t = t->m_next) {
						pktlen += t->m_len;
					}
					m->m_pkthdr.len = pktlen;
				}
				h = mtod(m, struct ip6_hdr *);
				VERIFY((int)m->m_len ==
				    ntohs(h->ip6_plen) - aftercut);
				h->ip6_plen =
				    htons(ntohs(h->ip6_plen) - aftercut);
			} else {
				hosed++;
			}
		} else if (frp == NULL) {
			                                      
			DPFPRINTF(("frag6cache[%u]: gap %d %d-%d (%d-%d)\n",
			    ntohl(fh->ip6f_ident), -aftercut, off, fr_max,
			    fra->fr_off, fra->fr_end));

			cur = pool_get(&pf_cent_pl, PR_NOWAIT);
			if (cur == NULL) {
				goto no_mem;
			}
			pf_ncache++;

			cur->fr_off = off;
			cur->fr_end = fr_max;
			LIST_INSERT_BEFORE(fra, cur, fr_next);
		}

		                                                             
		if (merge) {
			if (cur && fra->fr_off <= cur->fr_end) {
				                                       
				DPFPRINTF(("frag6cache[%u]: adjacent(merge "
				    "%d-%d) %d-%d (%d-%d)\n",
				    ntohl(fh->ip6f_ident), cur->fr_off,
				    cur->fr_end, off, fr_max, fra->fr_off,
				    fra->fr_end));
				fra->fr_off = cur->fr_off;
				LIST_REMOVE(cur, fr_next);
				pool_put(&pf_cent_pl, cur);
				pf_ncache--;
				cur = NULL;
			} else if (frp && fra->fr_off <= frp->fr_end) {
				                                       
				VERIFY(cur == NULL);
				DPFPRINTF(("frag6cache[%u]: adjacent(merge "
				    "%d-%d) %d-%d (%d-%d)\n",
				    ntohl(fh->ip6f_ident), frp->fr_off,
				    frp->fr_end, off, fr_max, fra->fr_off,
				    fra->fr_end));
				fra->fr_off = frp->fr_off;
				LIST_REMOVE(frp, fr_next);
				pool_put(&pf_cent_pl, frp);
				pf_ncache--;
				frp = NULL;
			}
		}
	}

	if (hosed) {
		  
                                                         
                                                          
                                                              
     
		goto drop_fragment;
	}

pass:
	                              
	if ((*frag)->fr_max < fr_max) {
		(*frag)->fr_max = fr_max;
	}

	                              
	if (!mff) {
		(*frag)->fr_flags |= PFFRAG_SEENLAST;
	}

	                                            
	if (((*frag)->fr_flags & PFFRAG_SEENLAST) &&
	    LIST_FIRST(&(*frag)->fr_cache)->fr_off == 0 &&
	    LIST_FIRST(&(*frag)->fr_cache)->fr_end == (*frag)->fr_max) {
		                                
		DPFPRINTF(("frag6cache[%u]: done 0-%d\n",
		    ntohl(fh->ip6f_ident), (*frag)->fr_max));
		pf_free_fragment(*frag);
		*frag = NULL;
	}

	return m;

no_mem:
	*nomem = 1;

	                                           
	if (!mff && *frag != NULL) {
		(*frag)->fr_flags |= PFFRAG_SEENLAST;
	}

	m_freem(m);
	return NULL;

drop_fragment:

	                                           
	if (!mff && *frag != NULL) {
		(*frag)->fr_flags |= PFFRAG_SEENLAST;
	}

	if (drop) {
		                                                     
		if (((*frag)->fr_flags & PFFRAG_DROP) == 0) {
			DPFPRINTF(("frag6cache[%u]: dropping overall fragment\n",
			    ntohl(fh->ip6f_ident)));
		}
		(*frag)->fr_flags |= PFFRAG_DROP;
	}

	m_freem(m);
	return NULL;
}

int
pf_refragment6(struct ifnet *ifp, pbuf_t **pbufp, struct pf_fragment_tag *ftag)
{
	struct mbuf        *m;
	uint32_t           frag_id;
	uint16_t           hdrlen, extoff, maxlen, unfragpartlen;
	uint8_t            proto;
	int                error, action;
	uint8_t            *lexthdrsp;
	struct route_in6   ip6route;
	struct route_in6   *ro;
	struct sockaddr_in6     *dst;
	struct ip6_hdr *hdr;
	struct pf_mtag *mtag;
	struct m_tag *tag;

	if (pbufp == NULL || !pbuf_is_valid(*pbufp) || ftag == NULL) {
		panic("pf_route6: invalid parameters");
		                 
	}
	m = pbuf_to_mbuf(*pbufp, FALSE);
	hdr = mtod(m, struct ip6_hdr *);
	mtag = pf_find_mtag(m);
	hdrlen = ftag->ft_hdrlen - sizeof(struct ip6_hdr);
	extoff = ftag->ft_extoff;
	maxlen = ftag->ft_maxlen;
	frag_id = ftag->ft_id;
	unfragpartlen = ftag->ft_unfragpartlen;
	tag = (struct m_tag *)(void *)ftag;
	tag = tag - 1;
	m_tag_delete(m, tag);
	ftag = NULL;
	tag = NULL;
	mtag->pftag_flags &= ~PF_TAG_REASSEMBLED;
	ro = &ip6route;
	bzero((caddr_t)ro, sizeof(*ro));
	dst = (struct sockaddr_in6 *)&ro->ro_dst;
	dst->sin6_family = AF_INET6;
	dst->sin6_len = sizeof(*dst);
	dst->sin6_addr = hdr->ip6_dst;

	if (extoff) {
		int off;
		struct mbuf *mexthdr;

		                                                           
		mexthdr = m_getptr(m, extoff +
		    offsetof(struct ip6_ext, ip6e_nxt), &off);
		ASSERT(mexthdr != NULL);
		lexthdrsp = (mtod(mexthdr, uint8_t *) + off);
		proto = *lexthdrsp;
		if (proto == IPPROTO_DSTOPTS) {
			struct ip6_ext ext;
			if (!pf_pull_hdr(*pbufp, off, &ext, sizeof(ext), NULL,
			    NULL, AF_INET6)) {
				DPFPRINTF(("pkt too short"));
				action = PF_DROP;
				goto done;
			}
			proto = ext.ip6e_nxt;
		}
	} else {
		lexthdrsp = NULL;
		proto = hdr->ip6_nxt;
	}

	  
                                                               
                        
    
	maxlen = maxlen & ~7;

	error = ip6_do_fragmentation(&m, hdrlen, NULL, unfragpartlen,
	    hdr, lexthdrsp, maxlen, proto, frag_id);

	if (error == 0) {
		  
                                                           
                                                                 
     
		mtag->pftag_flags |= PF_TAG_REFRAGMENTED;
		action = PF_PASS;
		pbuf_init_mbuf(*pbufp, m, ifp);
	} else {
		DPFPRINTF(("refragment error %d", error));
		action = PF_DROP;
		goto done;
	}
done:
	return action;
}

int
pf_normalize_ip(pbuf_t *pbuf, int dir, struct pfi_kif *kif, u_short *reason,
    struct pf_pdesc *pd)
{
	struct mbuf             *m;
	struct pf_rule          *r;
	struct pf_frent         *frent;
	struct pf_fragment      *frag = NULL;
	struct ip               *h = pbuf->pb_data;
	int                      mff = (ntohs(h->ip_off) & IP_MF);
	int                      hlen = h->ip_hl << 2;
	u_int16_t                fragoff = (ntohs(h->ip_off) & IP_OFFMASK) << 3;
	u_int16_t                fr_max;
	int                      ip_len;
	int                      ip_off;
	int                      asd = 0;
	struct pf_ruleset       *ruleset = NULL;
	struct ifnet            *ifp = pbuf->pb_ifp;

	r = TAILQ_FIRST(pf_main_ruleset.rules[PF_RULESET_SCRUB].active.ptr);
	while (r != NULL) {
		r->evaluations++;
		if (pfi_kif_match(r->kif, kif) == r->ifnot) {
			r = r->skip[PF_SKIP_IFP].ptr;
		} else if (r->direction && r->direction != dir) {
			r = r->skip[PF_SKIP_DIR].ptr;
		} else if (r->af && r->af != AF_INET) {
			r = r->skip[PF_SKIP_AF].ptr;
		} else if (r->proto && r->proto != h->ip_p) {
			r = r->skip[PF_SKIP_PROTO].ptr;
		} else if (PF_MISMATCHAW(&r->src.addr,
		    (struct pf_addr *)&h->ip_src.s_addr, AF_INET,
		    r->src.neg, kif)) {
			r = r->skip[PF_SKIP_SRC_ADDR].ptr;
		} else if (PF_MISMATCHAW(&r->dst.addr,
		    (struct pf_addr *)&h->ip_dst.s_addr, AF_INET,
		    r->dst.neg, NULL)) {
			r = r->skip[PF_SKIP_DST_ADDR].ptr;
		} else {
			if (r->anchor == NULL) {
				break;
			} else {
				pf_step_into_anchor(&asd, &ruleset,
				    PF_RULESET_SCRUB, &r, NULL, NULL);
			}
		}
		if (r == NULL && pf_step_out_of_anchor(&asd, &ruleset,
		    PF_RULESET_SCRUB, &r, NULL, NULL)) {
			break;
		}
	}

	if (r == NULL || r->action == PF_NOSCRUB) {
		return PF_PASS;
	} else {
		r->packets[dir == PF_OUT]++;
		r->bytes[dir == PF_OUT] += pd->tot_len;
	}

	                               
	if (hlen < (int)sizeof(struct ip)) {
		goto drop;
	}

	if (hlen > ntohs(h->ip_len)) {
		goto drop;
	}

	                                                   
	if (r->rule_flag & PFRULE_NODF && h->ip_off & htons(IP_DF)) {
		u_int16_t ipoff = h->ip_off;

		h->ip_off &= htons(~IP_DF);
		h->ip_sum = pf_cksum_fixup(h->ip_sum, ipoff, h->ip_off, 0);
	}

	                                   
	if (!fragoff && !mff) {
		goto no_fragment;
	}

	  
                                                            
                                                             
                                         
    
	if (h->ip_off & htons(IP_DF)) {
		DPFPRINTF(("IP_DF\n"));
		goto bad;
	}

	ip_len = ntohs(h->ip_len) - hlen;
	ip_off = (ntohs(h->ip_off) & IP_OFFMASK) << 3;

	                                      
	if (mff && (ip_len & 0x7)) {
		DPFPRINTF(("mff and %d\n", ip_len));
		goto bad;
	}

	                            
	if (fragoff + ip_len > IP_MAXPACKET) {
		DPFPRINTF(("max packet %d\n", fragoff + ip_len));
		goto bad;
	}
	fr_max = fragoff + ip_len;

	if ((r->rule_flag & (PFRULE_FRAGCROP | PFRULE_FRAGDROP)) == 0) {
		                                       

		frag = pf_find_fragment_by_ipv4_header(h, &pf_frag_tree);
		                                               
		if (frag != NULL && (frag->fr_flags & PFFRAG_SEENLAST) &&
		    fr_max > frag->fr_max) {
			goto bad;
		}

		if ((m = pbuf_to_mbuf(pbuf, TRUE)) == NULL) {
			REASON_SET(reason, PFRES_MEMORY);
			return PF_DROP;
		}

		VERIFY(!pbuf_is_valid(pbuf));

		                                              
		h = mtod(m, struct ip *);

		                                         
		frent = pool_get(&pf_frent_pl, PR_NOWAIT);
		if (frent == NULL) {
			REASON_SET(reason, PFRES_MEMORY);
			m_freem(m);
			return PF_DROP;
		}
		pf_nfrents++;
		frent->fr_ip = h;
		frent->fr_m = m;

		                                                         
		DPFPRINTF(("reass IPv4 frag %d @ %d-%d\n", ntohs(h->ip_id),
		    fragoff, fr_max));
		m = pf_reassemble(m, &frag, frent, mff);

		if (m == NULL) {
			return PF_DROP;
		}

		VERIFY(m->m_flags & M_PKTHDR);
		pbuf_init_mbuf(pbuf, m, ifp);

		                                           
		pd->pf_mtag = pf_find_mtag_pbuf(pbuf);
#if 0
                                 
#if DIAGNOSTIC
		if (pd->pf_mtag == NULL) {
			printf("%s: pf_find_mtag returned NULL(1)\n", __func__);
			if ((pd->pf_mtag = pf_get_mtag(m)) == NULL) {
				m_freem(m);
				m = NULL;
				goto no_mem;
			}
		}
#endif
#endif

		h = mtod(m, struct ip *);

		if (frag != NULL && (frag->fr_flags & PFFRAG_DROP)) {
			goto drop;
		}
	} else {
		                                                            
		int     nomem = 0;

		if (dir == PF_OUT && (pd->pf_mtag->pftag_flags & PF_TAG_FRAGCACHE)) {
			  
                                              
                                                 
                                              
      
			goto fragment_pass;
		}

		frag = pf_find_fragment_by_ipv4_header(h, &pf_cache_tree);

		                                               
		if (frag != NULL && (frag->fr_flags & PFFRAG_SEENLAST) &&
		    fr_max > frag->fr_max) {
			if (r->rule_flag & PFRULE_FRAGDROP) {
				frag->fr_flags |= PFFRAG_DROP;
			}
			goto bad;
		}

		if ((m = pbuf_to_mbuf(pbuf, TRUE)) == NULL) {
			REASON_SET(reason, PFRES_MEMORY);
			goto bad;
		}

		VERIFY(!pbuf_is_valid(pbuf));

		                                              
		h = mtod(m, struct ip *);

		m = pf_fragcache(&m, h, &frag, mff,
		    (r->rule_flag & PFRULE_FRAGDROP) ? 1 : 0, &nomem);
		if (m == NULL) {
			                                                      
			if (nomem) {
				goto no_mem;
			}
			goto drop;
		}

		VERIFY(m->m_flags & M_PKTHDR);
		pbuf_init_mbuf(pbuf, m, ifp);

		                                                 
		pd->pf_mtag = pf_find_mtag_pbuf(pbuf);
#if 0
                                 
#if DIAGNOSTIC
		if (pd->pf_mtag == NULL) {
			printf("%s: pf_find_mtag returned NULL(2)\n", __func__);
			if ((pd->pf_mtag = pf_get_mtag(m)) == NULL) {
				m_freem(m);
				m = NULL;
				goto no_mem;
			}
		}
#endif
#endif
		if (dir == PF_IN) {
			pd->pf_mtag->pftag_flags |= PF_TAG_FRAGCACHE;
		}

		if (frag != NULL && (frag->fr_flags & PFFRAG_DROP)) {
			goto drop;
		}

		goto fragment_pass;
	}

no_fragment:
	                                                    
	if (h->ip_off & ~htons(IP_DF)) {
		u_int16_t ipoff = h->ip_off;

		h->ip_off &= htons(IP_DF);
		h->ip_sum = pf_cksum_fixup(h->ip_sum, ipoff, h->ip_off, 0);
	}

	                                                           
	if (r->min_ttl && h->ip_ttl < r->min_ttl) {
		u_int16_t ip_ttl = h->ip_ttl;

		h->ip_ttl = r->min_ttl;
		h->ip_sum = pf_cksum_fixup(h->ip_sum, ip_ttl, h->ip_ttl, 0);
	}
	if (r->rule_flag & PFRULE_RANDOMID) {
		u_int16_t oip_id = h->ip_id;

		if (rfc6864 && IP_OFF_IS_ATOMIC(ntohs(h->ip_off))) {
			h->ip_id = 0;
		} else {
			h->ip_id = ip_randomid();
		}
		h->ip_sum = pf_cksum_fixup(h->ip_sum, oip_id, h->ip_id, 0);
	}
	if ((r->rule_flag & (PFRULE_FRAGCROP | PFRULE_FRAGDROP)) == 0) {
		pd->flags |= PFDESC_IP_REAS;
	}

	return PF_PASS;

fragment_pass:
	                                                           
	if (r->min_ttl && h->ip_ttl < r->min_ttl) {
		u_int16_t ip_ttl = h->ip_ttl;

		h->ip_ttl = r->min_ttl;
		h->ip_sum = pf_cksum_fixup(h->ip_sum, ip_ttl, h->ip_ttl, 0);
	}
	if ((r->rule_flag & (PFRULE_FRAGCROP | PFRULE_FRAGDROP)) == 0) {
		pd->flags |= PFDESC_IP_REAS;
	}
	return PF_PASS;

no_mem:
	REASON_SET(reason, PFRES_MEMORY);
	if (r != NULL && r->log && pbuf_is_valid(pbuf)) {
		PFLOG_PACKET(kif, h, pbuf, AF_INET, dir, *reason, r,
		    NULL, NULL, pd);
	}
	return PF_DROP;

drop:
	REASON_SET(reason, PFRES_NORM);
	if (r != NULL && r->log && pbuf_is_valid(pbuf)) {
		PFLOG_PACKET(kif, h, pbuf, AF_INET, dir, *reason, r,
		    NULL, NULL, pd);
	}
	return PF_DROP;

bad:
	DPFPRINTF(("dropping bad IPv4 fragment\n"));

	                               
	if (frag != NULL) {
		pf_free_fragment(frag);
	}

	REASON_SET(reason, PFRES_FRAG);
	if (r != NULL && r->log && pbuf_is_valid(pbuf)) {
		PFLOG_PACKET(kif, h, pbuf, AF_INET, dir, *reason, r, NULL, NULL, pd);
	}

	return PF_DROP;
}

static __inline struct pf_fragment *
pf_find_fragment_by_ipv6_header(struct ip6_hdr *ip6, struct ip6_frag *fh,
    struct pf_frag_tree *tree)
{
	struct pf_fragment key;
	pf_ip6hdr2key(&key, ip6, fh);
	return pf_find_fragment_by_key(&key, tree);
}

int
pf_normalize_ip6(pbuf_t *pbuf, int dir, struct pfi_kif *kif,
    u_short *reason, struct pf_pdesc *pd)
{
	struct mbuf             *m = NULL;
	struct pf_rule          *r;
	struct ip6_hdr          *h = pbuf->pb_data;
	int                      extoff;
	int                      off;
	struct ip6_ext           ext;
	struct ip6_opt           opt;
	struct ip6_opt_jumbo     jumbo;
	int                      optend;
	int                      ooff;
	struct ip6_frag          frag;
	u_int32_t                jumbolen = 0, plen;
	u_int16_t                fragoff = 0;
	u_int8_t                 proto;
	int                      terminal;
	struct pf_frent         *frent;
	struct pf_fragment      *pff = NULL;
	int                      mff = 0, rh_cnt = 0;
	u_int16_t                fr_max;
	int                      asd = 0;
	struct pf_ruleset       *ruleset = NULL;
	struct ifnet            *ifp = pbuf->pb_ifp;

	r = TAILQ_FIRST(pf_main_ruleset.rules[PF_RULESET_SCRUB].active.ptr);
	while (r != NULL) {
		r->evaluations++;
		if (pfi_kif_match(r->kif, kif) == r->ifnot) {
			r = r->skip[PF_SKIP_IFP].ptr;
		} else if (r->direction && r->direction != dir) {
			r = r->skip[PF_SKIP_DIR].ptr;
		} else if (r->af && r->af != AF_INET6) {
			r = r->skip[PF_SKIP_AF].ptr;
		}
#if 0                    
		else if (r->proto && r->proto != h->ip6_nxt) {
			r = r->skip[PF_SKIP_PROTO].ptr;
		}
#endif
		else if (PF_MISMATCHAW(&r->src.addr,
		    (struct pf_addr *)(uintptr_t)&h->ip6_src, AF_INET6,
		    r->src.neg, kif)) {
			r = r->skip[PF_SKIP_SRC_ADDR].ptr;
		} else if (PF_MISMATCHAW(&r->dst.addr,
		    (struct pf_addr *)(uintptr_t)&h->ip6_dst, AF_INET6,
		    r->dst.neg, NULL)) {
			r = r->skip[PF_SKIP_DST_ADDR].ptr;
		} else {
			if (r->anchor == NULL) {
				break;
			} else {
				pf_step_into_anchor(&asd, &ruleset,
				    PF_RULESET_SCRUB, &r, NULL, NULL);
			}
		}
		if (r == NULL && pf_step_out_of_anchor(&asd, &ruleset,
		    PF_RULESET_SCRUB, &r, NULL, NULL)) {
			break;
		}
	}

	if (r == NULL || r->action == PF_NOSCRUB) {
		return PF_PASS;
	} else {
		r->packets[dir == PF_OUT]++;
		r->bytes[dir == PF_OUT] += pd->tot_len;
	}

	                               
	if ((uint32_t)(sizeof(struct ip6_hdr) + IPV6_MAXPACKET) <
	    pbuf->pb_packet_len) {
		goto drop;
	}

	extoff = 0;
	off = sizeof(struct ip6_hdr);
	proto = h->ip6_nxt;
	terminal = 0;
	do {
		pd->proto = proto;
		switch (proto) {
		case IPPROTO_FRAGMENT:
			goto fragment;
		case IPPROTO_AH:
		case IPPROTO_ROUTING:
		case IPPROTO_DSTOPTS:
			if (!pf_pull_hdr(pbuf, off, &ext, sizeof(ext), NULL,
			    NULL, AF_INET6)) {
				goto shortpkt;
			}
			extoff = off;
			  
                     
                                           
                                                  
      
			if (proto == IPPROTO_ROUTING) {
				const struct ip6_rthdr *rh =
				    (const struct ip6_rthdr *)&ext;
				if (rh_cnt++) {
					goto drop;
				}
				if (rh->ip6r_type == IPV6_RTHDR_TYPE_0) {
					goto drop;
				}
			} else if (proto == IPPROTO_AH) {
				off += (ext.ip6e_len + 2) * 4;
			} else {
				off += (ext.ip6e_len + 1) * 8;
			}
			proto = ext.ip6e_nxt;
			break;
		case IPPROTO_HOPOPTS:
			if (!pf_pull_hdr(pbuf, off, &ext, sizeof(ext), NULL,
			    NULL, AF_INET6)) {
				goto shortpkt;
			}
			extoff = off;
			optend = off + (ext.ip6e_len + 1) * 8;
			ooff = off + sizeof(ext);
			do {
				if (!pf_pull_hdr(pbuf, ooff, &opt.ip6o_type,
				    sizeof(opt.ip6o_type), NULL, NULL,
				    AF_INET6)) {
					goto shortpkt;
				}
				if (opt.ip6o_type == IP6OPT_PAD1) {
					ooff++;
					continue;
				}
				if (!pf_pull_hdr(pbuf, ooff, &opt, sizeof(opt),
				    NULL, NULL, AF_INET6)) {
					goto shortpkt;
				}
				if ((ooff + (int) sizeof(opt) + opt.ip6o_len) >
				    optend) {
					goto drop;
				}
				switch (opt.ip6o_type) {
				case IP6OPT_JUMBO:
					if (h->ip6_plen != 0) {
						goto drop;
					}
					if (!pf_pull_hdr(pbuf, ooff, &jumbo,
					    sizeof(jumbo), NULL, NULL,
					    AF_INET6)) {
						goto shortpkt;
					}
					memcpy(&jumbolen, jumbo.ip6oj_jumbo_len,
					    sizeof(jumbolen));
					jumbolen = ntohl(jumbolen);
					if (jumbolen <= IPV6_MAXPACKET) {
						goto drop;
					}
					if ((sizeof(struct ip6_hdr) +
					    jumbolen) != pbuf->pb_packet_len) {
						goto drop;
					}
					break;
				default:
					break;
				}
				ooff += sizeof(opt) + opt.ip6o_len;
			} while (ooff < optend);

			off = optend;
			proto = ext.ip6e_nxt;
			break;
		default:
			terminal = 1;
			break;
		}
	} while (!terminal);

	                                                       
	if (ntohs(h->ip6_plen) == 0) {
		plen = jumbolen;
	} else {
		plen = ntohs(h->ip6_plen);
	}
	if (plen == 0) {
		goto drop;
	}
	if ((uint32_t)(sizeof(struct ip6_hdr) + plen) > pbuf->pb_packet_len) {
		goto shortpkt;
	}

	                                                           
	if (r->min_ttl && h->ip6_hlim < r->min_ttl) {
		h->ip6_hlim = r->min_ttl;
	}

	return PF_PASS;

fragment:
	plen = ntohs(h->ip6_plen);
	                                                
	if (plen == 0 || jumbolen) {
		goto drop;
	}

	if (!pf_pull_hdr(pbuf, off, &frag, sizeof(frag), NULL, NULL, AF_INET6)) {
		goto shortpkt;
	}
	fragoff = ntohs(frag.ip6f_offlg & IP6F_OFF_MASK);
	pd->proto = frag.ip6f_nxt;
	mff = ntohs(frag.ip6f_offlg & IP6F_MORE_FRAG);
	off += sizeof(frag);
	if (fragoff + (plen - off) > IPV6_MAXPACKET) {
		goto badfrag;
	}

	fr_max = fragoff + plen - (off - sizeof(struct ip6_hdr));
                         
                                                                  
                                                                      
                         

	if ((r->rule_flag & (PFRULE_FRAGCROP | PFRULE_FRAGDROP)) == 0) {
		                                       
		pd->flags |= PFDESC_IP_REAS;

		pff = pf_find_fragment_by_ipv6_header(h, &frag,
		    &pf_frag_tree);

		                                               
		if (pff != NULL && (pff->fr_flags & PFFRAG_SEENLAST) &&
		    fr_max > pff->fr_max) {
			goto badfrag;
		}

		if ((m = pbuf_to_mbuf(pbuf, TRUE)) == NULL) {
			REASON_SET(reason, PFRES_MEMORY);
			return PF_DROP;
		}

		                                              
		h = mtod(m, struct ip6_hdr *);

		                                         
		frent = pool_get(&pf_frent_pl, PR_NOWAIT);
		if (frent == NULL) {
			REASON_SET(reason, PFRES_MEMORY);
			return PF_DROP;
		}

		pf_nfrents++;
		frent->fr_ip6 = h;
		frent->fr_m = m;
		frent->fr_ip6f_opt = frag;
		frent->fr_ip6f_extoff = extoff;
		frent->fr_ip6f_hlen = off;
		                                                           
		if (pd->proto == IPPROTO_DSTOPTS) {
			if (!pf_pull_hdr(pbuf, off, &ext, sizeof(ext), NULL,
			    NULL, AF_INET6)) {
				goto shortpkt;
			}
			frent->fr_ip6f_hlen += (ext.ip6e_len + 1) * 8;
		}

		                                                         
		DPFPRINTF(("reass IPv6 frag %d @ %d-%d\n",
		    ntohl(frag.ip6f_ident), fragoff, fr_max));
		m = pf_reassemble6(&m, &pff, frent, mff);

		if (m == NULL) {
			return PF_DROP;
		}

		pbuf_init_mbuf(pbuf, m, ifp);
		h = pbuf->pb_data;

		if (pff != NULL && (pff->fr_flags & PFFRAG_DROP)) {
			goto drop;
		}
	} else if (dir == PF_IN ||
	    !(pd->pf_mtag->pftag_flags & PF_TAG_FRAGCACHE)) {
		                                                           
		int nomem = 0;

		pff = pf_find_fragment_by_ipv6_header(h, &frag,
		    &pf_cache_tree);

		                                               
		if (pff != NULL && (pff->fr_flags & PFFRAG_SEENLAST) &&
		    fr_max > pff->fr_max) {
			if (r->rule_flag & PFRULE_FRAGDROP) {
				pff->fr_flags |= PFFRAG_DROP;
			}
			goto badfrag;
		}

		if ((m = pbuf_to_mbuf(pbuf, TRUE)) == NULL) {
			goto no_mem;
		}

		                                              
		h = mtod(m, struct ip6_hdr *);

		m = pf_frag6cache(&m, h, &frag, &pff, off, mff,
		    (r->rule_flag & PFRULE_FRAGDROP) ? 1 : 0, &nomem);
		if (m == NULL) {
			                                                       
			if (nomem) {
				goto no_mem;
			}
			goto drop;
		}

		pbuf_init_mbuf(pbuf, m, ifp);
		pd->pf_mtag = pf_find_mtag_pbuf(pbuf);
		h = pbuf->pb_data;

		if (dir == PF_IN) {
			pd->pf_mtag->pftag_flags |= PF_TAG_FRAGCACHE;
		}

		if (pff != NULL && (pff->fr_flags & PFFRAG_DROP)) {
			goto drop;
		}
	}

	                                                           
	if (r->min_ttl && h->ip6_hlim < r->min_ttl) {
		h->ip6_hlim = r->min_ttl;
	}
	return PF_PASS;

no_mem:
	REASON_SET(reason, PFRES_MEMORY);
	goto dropout;

shortpkt:
	REASON_SET(reason, PFRES_SHORT);
	goto dropout;

drop:
	REASON_SET(reason, PFRES_NORM);
	goto dropout;

badfrag:
	DPFPRINTF(("dropping bad IPv6 fragment\n"));
	REASON_SET(reason, PFRES_FRAG);
	goto dropout;

dropout:
	if (pff != NULL) {
		pf_free_fragment(pff);
	}
	if (r != NULL && r->log && pbuf_is_valid(pbuf)) {
		PFLOG_PACKET(kif, h, pbuf, AF_INET6, dir, *reason, r, NULL, NULL, pd);
	}
	return PF_DROP;
}

int
pf_normalize_tcp(int dir, struct pfi_kif *kif, pbuf_t *pbuf, int ipoff,
    int off, void *h, struct pf_pdesc *pd)
{
#pragma unused(ipoff, h)
	struct pf_rule  *r, *rm = NULL;
	struct tcphdr   *th = pd->hdr.tcp;
	int              rewrite = 0;
	int              asd = 0;
	u_short          reason;
	u_int8_t         flags;
	sa_family_t      af = pd->af;
	struct pf_ruleset *ruleset = NULL;
	union pf_state_xport sxport, dxport;

	sxport.port = th->th_sport;
	dxport.port = th->th_dport;

	r = TAILQ_FIRST(pf_main_ruleset.rules[PF_RULESET_SCRUB].active.ptr);
	while (r != NULL) {
		r->evaluations++;
		if (pfi_kif_match(r->kif, kif) == r->ifnot) {
			r = r->skip[PF_SKIP_IFP].ptr;
		} else if (r->direction && r->direction != dir) {
			r = r->skip[PF_SKIP_DIR].ptr;
		} else if (r->af && r->af != af) {
			r = r->skip[PF_SKIP_AF].ptr;
		} else if (r->proto && r->proto != pd->proto) {
			r = r->skip[PF_SKIP_PROTO].ptr;
		} else if (PF_MISMATCHAW(&r->src.addr, pd->src, af,
		    r->src.neg, kif)) {
			r = r->skip[PF_SKIP_SRC_ADDR].ptr;
		} else if (r->src.xport.range.op &&
		    !pf_match_xport(r->src.xport.range.op, r->proto_variant,
		    &r->src.xport, &sxport)) {
			r = r->skip[PF_SKIP_SRC_PORT].ptr;
		} else if (PF_MISMATCHAW(&r->dst.addr, pd->dst, af,
		    r->dst.neg, NULL)) {
			r = r->skip[PF_SKIP_DST_ADDR].ptr;
		} else if (r->dst.xport.range.op &&
		    !pf_match_xport(r->dst.xport.range.op, r->proto_variant,
		    &r->dst.xport, &dxport)) {
			r = r->skip[PF_SKIP_DST_PORT].ptr;
		} else if (r->os_fingerprint != PF_OSFP_ANY &&
		    !pf_osfp_match(pf_osfp_fingerprint(pd, pbuf, off, th),
		    r->os_fingerprint)) {
			r = TAILQ_NEXT(r, entries);
		} else {
			if (r->anchor == NULL) {
				rm = r;
				break;
			} else {
				pf_step_into_anchor(&asd, &ruleset,
				    PF_RULESET_SCRUB, &r, NULL, NULL);
			}
		}
		if (r == NULL && pf_step_out_of_anchor(&asd, &ruleset,
		    PF_RULESET_SCRUB, &r, NULL, NULL)) {
			break;
		}
	}

	if (rm == NULL || rm->action == PF_NOSCRUB) {
		return PF_PASS;
	} else {
		r->packets[dir == PF_OUT]++;
		r->bytes[dir == PF_OUT] += pd->tot_len;
	}

	if (rm->rule_flag & PFRULE_REASSEMBLE_TCP) {
		pd->flags |= PFDESC_TCP_NORM;
	}

	flags = th->th_flags;
	if (flags & TH_SYN) {
		                    
		if (flags & TH_RST) {
			goto tcp_drop;
		}

		if (flags & TH_FIN) {
			flags &= ~TH_FIN;
		}
	} else {
		                    
		if (!(flags & (TH_ACK | TH_RST))) {
			goto tcp_drop;
		}
	}

	if (!(flags & TH_ACK)) {
		                                              
		if ((flags & TH_FIN) || (flags & TH_PUSH) || (flags & TH_URG)) {
			goto tcp_drop;
		}
	}

	                                     
	if (th->th_off < (sizeof(struct tcphdr) >> 2)) {
		goto tcp_drop;
	}

	                                                         
	if (flags != th->th_flags || th->th_x2 != 0) {
		u_int16_t       ov, nv;

		ov = *(u_int16_t *)(&th->th_ack + 1);
		th->th_flags = flags;
		th->th_x2 = 0;
		nv = *(u_int16_t *)(&th->th_ack + 1);

		th->th_sum = pf_cksum_fixup(th->th_sum, ov, nv, 0);
		rewrite = 1;
	}

	                                                 
	if (!(flags & TH_URG) && th->th_urp) {
		th->th_sum = pf_cksum_fixup(th->th_sum, th->th_urp, 0, 0);
		th->th_urp = 0;
		rewrite = 1;
	}

	                                              
	                     
	if (r->max_mss) {
		int rv = pf_normalize_tcpopt(r, dir, kif, pd, pbuf, th, off,
		    &rewrite);
		if (rv == PF_DROP) {
			return rv;
		}
		pbuf = pd->mp;
	}

	if (rewrite) {
		if (pf_lazy_makewritable(pd, pbuf,
		    off + sizeof(*th)) == NULL) {
			REASON_SET(&reason, PFRES_MEMORY);
			if (r->log) {
				PFLOG_PACKET(kif, h, pbuf, AF_INET, dir, reason,
				    r, 0, 0, pd);
			}
			return PF_DROP;
		}

		pbuf_copy_back(pbuf, off, sizeof(*th), th);
	}

	return PF_PASS;

tcp_drop:
	REASON_SET(&reason, PFRES_NORM);
	if (rm != NULL && r->log) {
		PFLOG_PACKET(kif, h, pbuf, AF_INET, dir, reason, r, NULL, NULL, pd);
	}
	return PF_DROP;
}

int
pf_normalize_tcp_init(pbuf_t *pbuf, int off, struct pf_pdesc *pd,
    struct tcphdr *th, struct pf_state_peer *src, struct pf_state_peer *dst)
{
#pragma unused(dst)
	u_int32_t tsval, tsecr;
	u_int8_t hdr[60];
	u_int8_t *opt;

	VERIFY(src->scrub == NULL);

	src->scrub = pool_get(&pf_state_scrub_pl, PR_NOWAIT);
	if (src->scrub == NULL) {
		return 1;
	}
	bzero(src->scrub, sizeof(*src->scrub));

	switch (pd->af) {
#if INET
	case AF_INET: {
		struct ip *h = pbuf->pb_data;
		src->scrub->pfss_ttl = h->ip_ttl;
		break;
	}
#endif           
	case AF_INET6: {
		struct ip6_hdr *h = pbuf->pb_data;
		src->scrub->pfss_ttl = h->ip6_hlim;
		break;
	}
	}


	  
                                                                  
                                                                    
    
	if ((th->th_flags & TH_SYN) == 0) {
		return 0;
	}


	if (th->th_off > (sizeof(struct tcphdr) >> 2) && src->scrub &&
	    pf_pull_hdr(pbuf, off, hdr, th->th_off << 2, NULL, NULL, pd->af)) {
		                             
		int hlen;
		opt = hdr + sizeof(struct tcphdr);
		hlen = (th->th_off << 2) - sizeof(struct tcphdr);
		while (hlen >= TCPOLEN_TIMESTAMP) {
			switch (*opt) {
			case TCPOPT_EOL:                         
			case TCPOPT_NOP:
				opt++;
				hlen--;
				break;
			case TCPOPT_TIMESTAMP:
				if (opt[1] >= TCPOLEN_TIMESTAMP) {
					src->scrub->pfss_flags |=
					    PFSS_TIMESTAMP;
					src->scrub->pfss_ts_mod =
					    htonl(random());

					                                
					memcpy(&tsval, &opt[2],
					    sizeof(u_int32_t));
					memcpy(&tsecr, &opt[6],
					    sizeof(u_int32_t));
					src->scrub->pfss_tsval0 = ntohl(tsval);
					src->scrub->pfss_tsval = ntohl(tsval);
					src->scrub->pfss_tsecr = ntohl(tsecr);
					getmicrouptime(&src->scrub->pfss_last);
				}
				OS_FALLTHROUGH;
			default:
				hlen -= MAX(opt[1], 2);
				opt += MAX(opt[1], 2);
				break;
			}
		}
	}

	return 0;
}

void
pf_normalize_tcp_cleanup(struct pf_state *state)
{
	if (state->src.scrub) {
		pool_put(&pf_state_scrub_pl, state->src.scrub);
	}
	if (state->dst.scrub) {
		pool_put(&pf_state_scrub_pl, state->dst.scrub);
	}

	                                                              
}

int
pf_normalize_tcp_stateful(pbuf_t *pbuf, int off, struct pf_pdesc *pd,
    u_short *reason, struct tcphdr *th, struct pf_state *state,
    struct pf_state_peer *src, struct pf_state_peer *dst, int *writeback)
{
	struct timeval uptime;
	u_int32_t tsval = 0, tsecr = 0;
	u_int tsval_from_last;
	u_int8_t hdr[60];
	u_int8_t *opt;
	int copyback = 0;
	int got_ts = 0;

	VERIFY(src->scrub || dst->scrub);

	  
                                                                      
                                                                
                        
    
	switch (pd->af) {
#if INET
	case AF_INET: {
		if (src->scrub) {
			struct ip *h = pbuf->pb_data;
			if (h->ip_ttl > src->scrub->pfss_ttl) {
				src->scrub->pfss_ttl = h->ip_ttl;
			}
			h->ip_ttl = src->scrub->pfss_ttl;
		}
		break;
	}
#endif           
	case AF_INET6: {
		if (src->scrub) {
			struct ip6_hdr *h = pbuf->pb_data;
			if (h->ip6_hlim > src->scrub->pfss_ttl) {
				src->scrub->pfss_ttl = h->ip6_hlim;
			}
			h->ip6_hlim = src->scrub->pfss_ttl;
		}
		break;
	}
	}

	if (th->th_off > (sizeof(struct tcphdr) >> 2) &&
	    ((src->scrub && (src->scrub->pfss_flags & PFSS_TIMESTAMP)) ||
	    (dst->scrub && (dst->scrub->pfss_flags & PFSS_TIMESTAMP))) &&
	    pf_pull_hdr(pbuf, off, hdr, th->th_off << 2, NULL, NULL, pd->af)) {
		                             
		int hlen;
		opt = hdr + sizeof(struct tcphdr);
		hlen = (th->th_off << 2) - sizeof(struct tcphdr);
		while (hlen >= TCPOLEN_TIMESTAMP) {
			switch (*opt) {
			case TCPOPT_EOL:                         
			case TCPOPT_NOP:
				opt++;
				hlen--;
				break;
			case TCPOPT_TIMESTAMP:
				  
                                                
                                                
                        
       

				if (got_ts) {
					                                 
					if (pf_status.debug >= PF_DEBUG_MISC) {
						DPFPRINTF(("multiple TS??"));
						pf_print_state(state);
						printf("\n");
					}
					REASON_SET(reason, PFRES_TS);
					return PF_DROP;
				}
				if (opt[1] >= TCPOLEN_TIMESTAMP) {
					memcpy(&tsval, &opt[2],
					    sizeof(u_int32_t));
					if (tsval && src->scrub &&
					    (src->scrub->pfss_flags &
					    PFSS_TIMESTAMP)) {
						tsval = ntohl(tsval);
						pf_change_a(&opt[2],
						    &th->th_sum,
						    htonl(tsval +
						    src->scrub->pfss_ts_mod),
						    0);
						copyback = 1;
					}

					                                      
					memcpy(&tsecr, &opt[6],
					    sizeof(u_int32_t));
					if (tsecr && dst->scrub &&
					    (dst->scrub->pfss_flags &
					    PFSS_TIMESTAMP)) {
						tsecr = ntohl(tsecr)
						    - dst->scrub->pfss_ts_mod;
						pf_change_a(&opt[6],
						    &th->th_sum, htonl(tsecr),
						    0);
						copyback = 1;
					}
					got_ts = 1;
				}
				OS_FALLTHROUGH;
			default:
				hlen -= MAX(opt[1], 2);
				opt += MAX(opt[1], 2);
				break;
			}
		}
		if (copyback) {
			                                                    
			int optoff = off + sizeof(*th);
			int optlen = (th->th_off << 2) - sizeof(*th);
			if (pf_lazy_makewritable(pd, pbuf, optoff + optlen) ==
			    NULL) {
				REASON_SET(reason, PFRES_MEMORY);
				return PF_DROP;
			}
			*writeback = optoff + optlen;
			pbuf_copy_back(pbuf, optoff, optlen, hdr + sizeof(*th));
		}
	}


	  
                                                                 
                                                                  
                                                                   
                                                                  
                                                  
    
#define TS_MAX_IDLE     (24*24*60*60)
#define TS_MAX_CONN     (12*24*60*60)                                           

	getmicrouptime(&uptime);
	if (src->scrub && (src->scrub->pfss_flags & PFSS_PAWS) &&
	    (uptime.tv_sec - src->scrub->pfss_last.tv_sec > TS_MAX_IDLE ||
	    pf_time_second() - state->creation > TS_MAX_CONN)) {
		if (pf_status.debug >= PF_DEBUG_MISC) {
			DPFPRINTF(("src idled out of PAWS\n"));
			pf_print_state(state);
			printf("\n");
		}
		src->scrub->pfss_flags = (src->scrub->pfss_flags & ~PFSS_PAWS)
		    | PFSS_PAWS_IDLED;
	}
	if (dst->scrub && (dst->scrub->pfss_flags & PFSS_PAWS) &&
	    uptime.tv_sec - dst->scrub->pfss_last.tv_sec > TS_MAX_IDLE) {
		if (pf_status.debug >= PF_DEBUG_MISC) {
			DPFPRINTF(("dst idled out of PAWS\n"));
			pf_print_state(state);
			printf("\n");
		}
		dst->scrub->pfss_flags = (dst->scrub->pfss_flags & ~PFSS_PAWS)
		    | PFSS_PAWS_IDLED;
	}

	if (got_ts && src->scrub && dst->scrub &&
	    (src->scrub->pfss_flags & PFSS_PAWS) &&
	    (dst->scrub->pfss_flags & PFSS_PAWS)) {
		  
                                                  
                                                       
                                                  
                                                         
                                                     
                                                      
                                                        
                                                       
                                                        
                                                       
                                                         
                                         
    
                      
                                                         
                                                      
                                                       
                                                      
                                                    
                                                    
                                                   
                                                    
                                                     
                                                      
                                                        
                                                      
                                                
                                                   
                                                         
                                                        
                                                       
                                                      
                                                      
                           
    
                                                         
                                                         
                                                          
                                                         
                                                   
                                                       
                                                   
                                  
                                                      
                                                         
                                                   
                                                    
                                                        
                                                       
                                                        
                                                       
                                                        
                                      
    
                                                  
                                                        
                                                      
                                                   
                                                        
                                     
     
		struct timeval delta_ts;
		int ts_fudge;


		  
                                                        
                                                         
                                                       
                 
     
		if ((ts_fudge = state->rule.ptr->timeout[PFTM_TS_DIFF]) == 0) {
			ts_fudge = pf_default_rule.timeout[PFTM_TS_DIFF];
		}


		                                                  
#define TS_MAXFREQ      1100                                                    
#define TS_MICROSECS    1000000                                      
		timersub(&uptime, &src->scrub->pfss_last, &delta_ts);
		tsval_from_last = (delta_ts.tv_sec + ts_fudge) * TS_MAXFREQ;
		tsval_from_last += delta_ts.tv_usec / (TS_MICROSECS / TS_MAXFREQ);


		if ((src->state >= TCPS_ESTABLISHED &&
		    dst->state >= TCPS_ESTABLISHED) &&
		    (SEQ_LT(tsval, dst->scrub->pfss_tsecr) ||
		    SEQ_GT(tsval, src->scrub->pfss_tsval + tsval_from_last) ||
		    (tsecr && (SEQ_GT(tsecr, dst->scrub->pfss_tsval) ||
		    SEQ_LT(tsecr, dst->scrub->pfss_tsval0))))) {
			  
                                                        
     
                                                         
                                                      
                         
      

			DPFPRINTF(("Timestamp failed %c%c%c%c\n",
			    SEQ_LT(tsval, dst->scrub->pfss_tsecr) ? '0' : ' ',
			    SEQ_GT(tsval, src->scrub->pfss_tsval +
			    tsval_from_last) ? '1' : ' ',
			    SEQ_GT(tsecr, dst->scrub->pfss_tsval) ? '2' : ' ',
			    SEQ_LT(tsecr, dst->scrub->pfss_tsval0)? '3' : ' '));
			DPFPRINTF((" tsval: %u  tsecr: %u  +ticks: %u  "
			    "idle: %lus %ums\n",
			    tsval, tsecr, tsval_from_last, delta_ts.tv_sec,
			    delta_ts.tv_usec / 1000));
			DPFPRINTF((" src->tsval: %u  tsecr: %u\n",
			    src->scrub->pfss_tsval, src->scrub->pfss_tsecr));
			DPFPRINTF((" dst->tsval: %u  tsecr: %u  tsval0: %u\n",
			    dst->scrub->pfss_tsval, dst->scrub->pfss_tsecr,
			    dst->scrub->pfss_tsval0));
			if (pf_status.debug >= PF_DEBUG_MISC) {
				pf_print_state(state);
				pf_print_flags(th->th_flags);
				printf("\n");
			}
			REASON_SET(reason, PFRES_TS);
			return PF_DROP;
		}

		                                                            
	} else if (!got_ts && (th->th_flags & TH_RST) == 0 &&
	    ((src->state == TCPS_ESTABLISHED && dst->state == TCPS_ESTABLISHED)
	    || pd->p_len > 0 || (th->th_flags & TH_SYN)) &&
	    src->scrub && dst->scrub &&
	    (src->scrub->pfss_flags & PFSS_PAWS) &&
	    (dst->scrub->pfss_flags & PFSS_PAWS)) {
		  
                                                              
          
                                                            
                                                         
                                                              
                                                             
                                                              
                                                              
                                                            
                                                              
                
    
                                                            
                                                             
    
                                                              
                                                            
                                                                 
                                                               
                                                               
                                                            
                                                                
                                                                 
             
     

		if (pd->p_len > 0 && (src->scrub->pfss_flags & PFSS_DATA_TS)) {
			  
                                                       
                                            
      
			if (pf_status.debug >= PF_DEBUG_MISC) {
				DPFPRINTF(("Did not receive expected RFC1323 "
				    "timestamp\n"));
				pf_print_state(state);
				pf_print_flags(th->th_flags);
				printf("\n");
			}
			REASON_SET(reason, PFRES_TS);
			return PF_DROP;
		}
	}


	  
                                                                 
                                                                    
                                                                         
                                                                      
                                                                    
                                                 
    
	if (pd->p_len > 0 && src->scrub && (src->scrub->pfss_flags &
	    (PFSS_TIMESTAMP | PFSS_DATA_TS | PFSS_DATA_NOTS)) == PFSS_TIMESTAMP) {
		if (got_ts) {
			src->scrub->pfss_flags |= PFSS_DATA_TS;
		} else {
			src->scrub->pfss_flags |= PFSS_DATA_NOTS;
			if (pf_status.debug >= PF_DEBUG_MISC && dst->scrub &&
			    (dst->scrub->pfss_flags & PFSS_TIMESTAMP)) {
				                                               
				DPFPRINTF(("Broken RFC1323 stack did not "
				    "timestamp data packet. Disabled PAWS "
				    "security.\n"));
				pf_print_state(state);
				pf_print_flags(th->th_flags);
				printf("\n");
			}
		}
	}


	  
                      
    
	if (got_ts && src->scrub && PFSS_TIMESTAMP == (src->scrub->pfss_flags &
	    (PFSS_PAWS_IDLED | PFSS_TIMESTAMP))) {
		getmicrouptime(&src->scrub->pfss_last);
		if (SEQ_GEQ(tsval, src->scrub->pfss_tsval) ||
		    (src->scrub->pfss_flags & PFSS_PAWS) == 0) {
			src->scrub->pfss_tsval = tsval;
		}

		if (tsecr) {
			if (SEQ_GEQ(tsecr, src->scrub->pfss_tsecr) ||
			    (src->scrub->pfss_flags & PFSS_PAWS) == 0) {
				src->scrub->pfss_tsecr = tsecr;
			}

			if ((src->scrub->pfss_flags & PFSS_PAWS) == 0 &&
			    (SEQ_LT(tsval, src->scrub->pfss_tsval0) ||
			    src->scrub->pfss_tsval0 == 0)) {
				                                         
				src->scrub->pfss_tsval0 = tsval;
			}

			                                                   
			if ((src->scrub->pfss_flags & PFSS_PAWS) == 0) {
				src->scrub->pfss_flags |= PFSS_PAWS;
			}
		}
	}

	                                                    
	return 0;
}

static int
pf_normalize_tcpopt(struct pf_rule *r, int dir, struct pfi_kif *kif,
    struct pf_pdesc *pd, pbuf_t *pbuf, struct tcphdr *th, int off,
    int *rewrptr)
{
#pragma unused(dir, kif)
	sa_family_t af = pd->af;
	u_int16_t       *mss;
	int             thoff;
	int             opt, cnt, optlen = 0;
	int             rewrite = 0;
	u_char          opts[MAX_TCPOPTLEN];
	u_char          *optp = opts;

	thoff = th->th_off << 2;
	cnt = thoff - sizeof(struct tcphdr);

	if (cnt > 0 && !pf_pull_hdr(pbuf, off + sizeof(*th), opts, cnt,
	    NULL, NULL, af)) {
		return PF_DROP;
	}

	for (; cnt > 0; cnt -= optlen, optp += optlen) {
		opt = optp[0];
		if (opt == TCPOPT_EOL) {
			break;
		}
		if (opt == TCPOPT_NOP) {
			optlen = 1;
		} else {
			if (cnt < 2) {
				break;
			}
			optlen = optp[1];
			if (optlen < 2 || optlen > cnt) {
				break;
			}
		}
		switch (opt) {
		case TCPOPT_MAXSEG:
			mss = (u_int16_t *)(void *)(optp + 2);
			if ((ntohs(*mss)) > r->max_mss) {
				  
                      
                                                 
                                                  
       
				if (pbuf->pb_ifp ||
				    !(*pbuf->pb_csum_flags & CSUM_TCP)) {
					th->th_sum = pf_cksum_fixup(th->th_sum,
					    *mss, htons(r->max_mss), 0);
				}
				*mss = htons(r->max_mss);
				rewrite = 1;
			}
			break;
		default:
			break;
		}
	}

	if (rewrite) {
		u_short reason;

		VERIFY(pbuf == pd->mp);

		if (pf_lazy_makewritable(pd, pd->mp,
		    off + sizeof(*th) + thoff) == NULL) {
			REASON_SET(&reason, PFRES_MEMORY);
			if (r->log) {
				PFLOG_PACKET(kif, h, pbuf, AF_INET, dir, reason,
				    r, 0, 0, pd);
			}
			return PF_DROP;
		}

		*rewrptr = 1;
		pbuf_copy_back(pd->mp, off + sizeof(*th), thoff - sizeof(*th), opts);
	}

	return PF_PASS;
}
