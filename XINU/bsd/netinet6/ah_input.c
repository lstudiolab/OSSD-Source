/*
 * Copyright (c) 2008-2020 Apple Inc. All rights reserved.
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
#include <sys/mbuf.h>
#include <sys/mcache.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_ipsec.h>
#include <net/route.h>
#include <kern/cpu_number.h>
#include <kern/locks.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_ecn.h>
#include <netinet/in_pcb.h>
#include <netinet6/ip6_ecn.h>

#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet/icmp6.h>
#include <netinet6/ip6protosw.h>

#include <netinet6/ipsec.h>
#include <netinet6/ipsec6.h>
#include <netinet6/ah.h>
#include <netinet6/ah6.h>
#include <netkey/key.h>
#include <netkey/keydb.h>
#if IPSEC_DEBUG
#include <netkey/key_debug.h>
#else
#define KEYDEBUG(lev, arg)
#endif

#include <net/kpi_protocol.h>
#include <netinet/kpi_ipfilter_var.h>
#include <mach/sdt.h>

#include <net/net_osdep.h>

#define IPLEN_FLIPPED

extern lck_mtx_t  *sadb_mutex;

#if INET
void
ah4_input(struct mbuf *m, int off)
{
	struct ip *ip;
	struct ah *ah;
	u_int32_t spi;
	const struct ah_algorithm *algo;
	size_t siz;
	size_t siz1;
	u_char *cksum;
	struct secasvar *sav = NULL;
	u_int16_t nxt;
	u_int8_t hlen;
	size_t stripsiz = 0;
	sa_family_t ifamily;

	if (m->m_len < off + sizeof(struct newah)) {
		m = m_pullup(m, off + sizeof(struct newah));
		if (!m) {
			ipseclog((LOG_DEBUG, "IPv4 AH input: can't pullup;"
			    "dropping the packet for simplicity\n"));
			IPSEC_STAT_INCREMENT(ipsecstat.in_inval);
			goto fail;
		}
	}

	                                                                  
	MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(m);

	ip = mtod(m, struct ip *);
	ah = (struct ah *)(void *)(((caddr_t)ip) + off);
	nxt = ah->ah_nxt;
#ifdef _IP_VHL
	hlen = (u_int8_t)(IP_VHL_HL(ip->ip_vhl) << 2);
#else
	hlen = (u_int8_t)(ip->ip_hl << 2);
#endif

	                      
	spi = ah->ah_spi;

	if ((sav = key_allocsa(AF_INET,
	    (caddr_t)&ip->ip_src, (caddr_t)&ip->ip_dst,
	    IPPROTO_AH, spi)) == 0) {
		ipseclog((LOG_WARNING,
		    "IPv4 AH input: no key association found for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		IPSEC_STAT_INCREMENT(ipsecstat.in_nosa);
		goto fail;
	}
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
	    printf("DP ah4_input called to allocate SA:0x%llx\n",
	    (uint64_t)VM_KERNEL_ADDRPERM(sav)));
	if (sav->state != SADB_SASTATE_MATURE
	    && sav->state != SADB_SASTATE_DYING) {
		ipseclog((LOG_DEBUG,
		    "IPv4 AH input: non-mature/dying SA found for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		IPSEC_STAT_INCREMENT(ipsecstat.in_badspi);
		goto fail;
	}

	algo = ah_algorithm_lookup(sav->alg_auth);
	if (!algo) {
		ipseclog((LOG_DEBUG, "IPv4 AH input: "
		    "unsupported authentication algorithm for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		IPSEC_STAT_INCREMENT(ipsecstat.in_badspi);
		goto fail;
	}

	siz = (*algo->sumsiz)(sav);
	siz1 = ((siz + 3) & ~(4 - 1));

	  
                                
    
	{
		int sizoff;

		sizoff = (sav->flags & SADB_X_EXT_OLD) ? 0 : 4;

		  
                                                               
                                                                     
                
                                                            
                                                                         
                                    
    
                                                   
                                                                  
                                                                     
                                                                
                          
                                                                      
                                           
                                                                       
                                                                        
            
    
                                                
     
		if (siz1 < siz) {
			ipseclog((LOG_NOTICE, "sum length too short in IPv4 AH input "
			    "(%u, should be at least %u): %s\n",
			    (u_int32_t)siz1, (u_int32_t)siz,
			    ipsec4_logpacketstr(ip, spi)));
			IPSEC_STAT_INCREMENT(ipsecstat.in_inval);
			goto fail;
		}
		if ((ah->ah_len << 2) - sizoff != siz1) {
			ipseclog((LOG_NOTICE, "sum length mismatch in IPv4 AH input "
			    "(%d should be %u): %s\n",
			    (ah->ah_len << 2) - sizoff, (u_int32_t)siz1,
			    ipsec4_logpacketstr(ip, spi)));
			IPSEC_STAT_INCREMENT(ipsecstat.in_inval);
			goto fail;
		}

		if (m->m_len < off + sizeof(struct ah) + sizoff + siz1) {
			VERIFY((off + sizeof(struct ah) + sizoff + siz1) <= INT_MAX);
			m = m_pullup(m, (int)(off + sizeof(struct ah) + sizoff + siz1));
			if (!m) {
				ipseclog((LOG_DEBUG, "IPv4 AH input: can't pullup\n"));
				IPSEC_STAT_INCREMENT(ipsecstat.in_inval);
				goto fail;
			}
			                                                              
			MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(m);

			ip = mtod(m, struct ip *);
			ah = (struct ah *)(void *)(((caddr_t)ip) + off);
		}
	}

	  
                              
    
	if ((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay[0] != NULL) {
		if (ipsec_chkreplay(ntohl(((struct newah *)ah)->ah_seq), sav, 0)) {
			;         
		} else {
			IPSEC_STAT_INCREMENT(ipsecstat.in_ahreplay);
			ipseclog((LOG_WARNING,
			    "replay packet in IPv4 AH input: %s %s\n",
			    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
			goto fail;
		}
	}

	  
                                                          
                           
    
	cksum = _MALLOC(siz1, M_TEMP, M_NOWAIT);
	if (!cksum) {
		ipseclog((LOG_DEBUG, "IPv4 AH input: "
		    "couldn't alloc temporary region for cksum\n"));
		IPSEC_STAT_INCREMENT(ipsecstat.in_inval);
		goto fail;
	}

	  
                                                            
                                                      
    
	if ((ip->ip_len + hlen) > UINT16_MAX) {
		ipseclog((LOG_DEBUG, "IPv4 AH input: "
		    "bad length ip header len %u, total len %u\n",
		    ip->ip_len, hlen));
		IPSEC_STAT_INCREMENT(ipsecstat.in_inval);
		goto fail;
	}

	ip->ip_len = htons((u_int16_t)(ip->ip_len + hlen));
	ip->ip_off = htons(ip->ip_off);
	if (ah4_calccksum(m, (caddr_t)cksum, siz1, algo, sav)) {
		FREE(cksum, M_TEMP);
		IPSEC_STAT_INCREMENT(ipsecstat.in_inval);
		goto fail;
	}
	IPSEC_STAT_INCREMENT(ipsecstat.in_ahhist[sav->alg_auth]);
	  
                   
    
	ip->ip_len = ntohs(ip->ip_len) - hlen;
	ip->ip_off = ntohs(ip->ip_off);

	{
		caddr_t sumpos = NULL;

		if (sav->flags & SADB_X_EXT_OLD) {
			              
			sumpos = (caddr_t)(ah + 1);
		} else {
			              
			sumpos = (caddr_t)(((struct newah *)ah) + 1);
		}

		if (bcmp(sumpos, cksum, siz) != 0) {
			ipseclog((LOG_WARNING,
			    "checksum mismatch in IPv4 AH input: %s %s\n",
			    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
			FREE(cksum, M_TEMP);
			IPSEC_STAT_INCREMENT(ipsecstat.in_ahauthfail);
			goto fail;
		}
	}

	FREE(cksum, M_TEMP);

	m->m_flags |= M_AUTHIPHDR;
	m->m_flags |= M_AUTHIPDGM;

	if (m->m_flags & M_AUTHIPHDR && m->m_flags & M_AUTHIPDGM) {
		IPSEC_STAT_INCREMENT(ipsecstat.in_ahauthsucc);
	} else {
		ipseclog((LOG_WARNING,
		    "authentication failed in IPv4 AH input: %s %s\n",
		    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
		IPSEC_STAT_INCREMENT(ipsecstat.in_ahauthfail);
		goto fail;
	}

	  
                           
    
	if ((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay[0] != NULL) {
		if (ipsec_updatereplay(ntohl(((struct newah *)ah)->ah_seq), sav, 0)) {
			IPSEC_STAT_INCREMENT(ipsecstat.in_ahreplay);
			goto fail;
		}
	}

	                                                  
	if (sav->flags & SADB_X_EXT_OLD) {
		              
		stripsiz = sizeof(struct ah) + siz1;
	} else {
		              
		stripsiz = sizeof(struct newah) + siz1;
	}
	if (ipsec4_tunnel_validate(m, (int)(off + stripsiz), nxt, sav, &ifamily)) {
		ifaddr_t ifa;
		struct sockaddr_storage addr;
		struct sockaddr_in *ipaddr;

		  
                                                
                                        
    
                           
                               
     
		u_int8_t tos, otos;
		int sum;

		if (ifamily == AF_INET6) {
			ipseclog((LOG_NOTICE, "ipsec tunnel protocol mismatch "
			    "in IPv4 AH input: %s\n", ipsec_logsastr(sav)));
			goto fail;
		}
		tos = ip->ip_tos;
		m_adj(m, (int)(off + stripsiz));
		if (m->m_len < sizeof(*ip)) {
			m = m_pullup(m, sizeof(*ip));
			if (!m) {
				IPSEC_STAT_INCREMENT(ipsecstat.in_inval);
				goto fail;
			}
		}
		ip = mtod(m, struct ip *);
		otos = ip->ip_tos;
		                        
		if (ip_ecn_egress(ip4_ipsec_ecn, &tos, &ip->ip_tos) == 0) {
			IPSEC_STAT_INCREMENT(ipsecstat.in_inval);
			goto fail;
		}

		if (otos != ip->ip_tos) {
			sum = ~ntohs(ip->ip_sum) & 0xffff;
			sum += (~otos & 0xffff) + ip->ip_tos;
			sum = (sum >> 16) + (sum & 0xffff);
			sum += (sum >> 16);                
			ip->ip_sum = htons(~sum & 0xffff);
		}

		if (!key_checktunnelsanity(sav, AF_INET,
		    (caddr_t)&ip->ip_src, (caddr_t)&ip->ip_dst)) {
			ipseclog((LOG_NOTICE, "ipsec tunnel address mismatch "
			    "in IPv4 AH input: %s %s\n",
			    ipsec4_logpacketstr(ip, spi), ipsec_logsastr(sav)));
			IPSEC_STAT_INCREMENT(ipsecstat.in_inval);
			goto fail;
		}

#if 1
		  
                                                     
                              
    
                                  
                                                        
                                                        
                           
    
                           
                                                     
    
                    
                                                            
                                     
                                                        
                                                      
                                
     
		m->m_flags &= ~M_AUTHIPHDR;
		m->m_flags &= ~M_AUTHIPDGM;
#endif

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_AH, spi) != 0 ||
		    ipsec_addhist(m, IPPROTO_IPV4, 0) != 0) {
			IPSEC_STAT_INCREMENT(ipsecstat.in_nomem);
			goto fail;
		}

		bzero(&addr, sizeof(addr));
		ipaddr = (__typeof__(ipaddr)) & addr;
		ipaddr->sin_family = AF_INET;
		ipaddr->sin_len = sizeof(*ipaddr);
		ipaddr->sin_addr = ip->ip_dst;

		                                                                    
		ifa = ifa_ifwithaddr((struct sockaddr *)&addr);
		if (ifa) {
			m->m_pkthdr.rcvif = ifa->ifa_ifp;
			IFA_REMREF(ifa);
		}

		                            
		lck_mtx_lock(sadb_mutex);
		ifnet_t ipsec_if = sav->sah->ipsec_if;
		if (ipsec_if != NULL) {
			                                                                           
			ifnet_reference(ipsec_if);
		}
		lck_mtx_unlock(sadb_mutex);
		if (ipsec_if != NULL) {
			errno_t inject_error = ipsec_inject_inbound_packet(ipsec_if, m);
			ifnet_release(ipsec_if);
			if (inject_error == 0) {
				m = NULL;
				goto done;
			} else {
				goto fail;
			}
		}

		if (proto_input(PF_INET, m) != 0) {
			goto fail;
		}
		nxt = IPPROTO_DONE;
	} else {
		  
                  
     

		ip = mtod(m, struct ip *);
		  
                                             
                                                    
     
		ovbcopy((caddr_t)ip, (caddr_t)(((u_char *)ip) + stripsiz), off);
		m->m_data += stripsiz;
		m->m_len -= stripsiz;
		m->m_pkthdr.len -= stripsiz;

		if (m->m_len < sizeof(*ip)) {
			m = m_pullup(m, sizeof(*ip));
			if (m == NULL) {
				IPSEC_STAT_INCREMENT(ipsecstat.in_inval);
				goto fail;
			}
		}
		ip = mtod(m, struct ip *);
#ifdef IPLEN_FLIPPED
		ip->ip_len = (u_short)(ip->ip_len - stripsiz);
#else
		ip->ip_len = htons(ntohs(ip->ip_len) - stripsiz);
#endif
		ip->ip_p = (u_char)nxt;
		                                                                     

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_AH, spi) != 0) {
			IPSEC_STAT_INCREMENT(ipsecstat.in_nomem);
			goto fail;
		}

		DTRACE_IP6(receive, struct mbuf *, m, struct inpcb *, NULL,
		    struct ip *, ip, struct ifnet *, m->m_pkthdr.rcvif,
		    struct ip *, ip, struct ip6_hdr *, NULL);

		if (nxt != IPPROTO_DONE) {
			                            
			lck_mtx_lock(sadb_mutex);
			ifnet_t ipsec_if = sav->sah->ipsec_if;
			if (ipsec_if != NULL) {
				                                                                           
				ifnet_reference(ipsec_if);
			}
			lck_mtx_unlock(sadb_mutex);
			if (ipsec_if != NULL) {
				ip->ip_len = htons(ip->ip_len + hlen);
				ip->ip_off = htons(ip->ip_off);
				ip->ip_sum = 0;
				ip->ip_sum = ip_cksum_hdr_in(m, hlen);
				errno_t inject_error = ipsec_inject_inbound_packet(ipsec_if, m);
				ifnet_release(ipsec_if);
				if (inject_error == 0) {
					m = NULL;
					goto done;
				} else {
					goto fail;
				}
			}

			if ((ip_protox[nxt]->pr_flags & PR_LASTHDR) != 0 &&
			    ipsec4_in_reject(m, NULL)) {
				IPSEC_STAT_INCREMENT(ipsecstat.in_polvio);
				goto fail;
			}
			ip_proto_dispatch_in(m, off, (u_int8_t)nxt, 0);
		} else {
			m_freem(m);
		}
		m = NULL;
	}
done:
	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		    printf("DP ah4_input call free SA:0x%llx\n",
		    (uint64_t)VM_KERNEL_ADDRPERM(sav)));
		key_freesav(sav, KEY_SADB_UNLOCKED);
	}
	IPSEC_STAT_INCREMENT(ipsecstat.in_success);
	return;

fail:
	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		    printf("DP ah4_input call free SA:0x%llx\n",
		    (uint64_t)VM_KERNEL_ADDRPERM(sav)));
		key_freesav(sav, KEY_SADB_UNLOCKED);
	}
	if (m) {
		m_freem(m);
	}
	return;
}
#endif           

int
ah6_input(struct mbuf **mp, int *offp, int proto)
{
#pragma unused(proto)
	struct mbuf *m = *mp;
	int off = *offp;
	struct ip6_hdr *ip6 = NULL;
	struct ah *ah = NULL;
	u_int32_t spi = 0;
	const struct ah_algorithm *algo = NULL;
	size_t siz = 0;
	size_t siz1 = 0;
	u_char *cksum = NULL;
	struct secasvar *sav = NULL;
	u_int16_t nxt = IPPROTO_DONE;
	size_t stripsiz = 0;
	sa_family_t ifamily = AF_UNSPEC;

	IP6_EXTHDR_CHECK(m, off, sizeof(struct ah), {return IPPROTO_DONE;});
	ah = (struct ah *)(void *)(mtod(m, caddr_t) + off);
	                                                                  
	MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(m);

	ip6 = mtod(m, struct ip6_hdr *);
	nxt = ah->ah_nxt;

	                       
	spi = ah->ah_spi;

	if (ntohs(ip6->ip6_plen) == 0) {
		ipseclog((LOG_ERR, "IPv6 AH input: "
		    "AH with IPv6 jumbogram is not supported.\n"));
		IPSEC_STAT_INCREMENT(ipsec6stat.in_inval);
		goto fail;
	}

	if ((sav = key_allocsa(AF_INET6,
	    (caddr_t)&ip6->ip6_src, (caddr_t)&ip6->ip6_dst,
	    IPPROTO_AH, spi)) == 0) {
		ipseclog((LOG_WARNING,
		    "IPv6 AH input: no key association found for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		IPSEC_STAT_INCREMENT(ipsec6stat.in_nosa);
		goto fail;
	}
	KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
	    printf("DP ah6_input called to allocate SA:0x%llx\n",
	    (uint64_t)VM_KERNEL_ADDRPERM(sav)));
	if (sav->state != SADB_SASTATE_MATURE
	    && sav->state != SADB_SASTATE_DYING) {
		ipseclog((LOG_DEBUG,
		    "IPv6 AH input: non-mature/dying SA found for spi %u; ",
		    (u_int32_t)ntohl(spi)));
		IPSEC_STAT_INCREMENT(ipsec6stat.in_badspi);
		goto fail;
	}

	algo = ah_algorithm_lookup(sav->alg_auth);
	if (!algo) {
		ipseclog((LOG_DEBUG, "IPv6 AH input: "
		    "unsupported authentication algorithm for spi %u\n",
		    (u_int32_t)ntohl(spi)));
		IPSEC_STAT_INCREMENT(ipsec6stat.in_badspi);
		goto fail;
	}

	siz = (*algo->sumsiz)(sav);
	siz1 = ((siz + 3) & ~(4 - 1));

	  
                                
    
	{
		int sizoff;

		sizoff = (sav->flags & SADB_X_EXT_OLD) ? 0 : 4;

		  
                                                                    
                 
     
		if (siz1 < siz) {
			ipseclog((LOG_NOTICE, "sum length too short in IPv6 AH input "
			    "(%u, should be at least %u): %s\n",
			    (u_int32_t)siz1, (u_int32_t)siz,
			    ipsec6_logpacketstr(ip6, spi)));
			IPSEC_STAT_INCREMENT(ipsec6stat.in_inval);
			goto fail;
		}
		if ((ah->ah_len << 2) - sizoff != siz1) {
			ipseclog((LOG_NOTICE, "sum length mismatch in IPv6 AH input "
			    "(%d should be %u): %s\n",
			    (ah->ah_len << 2) - sizoff, (u_int32_t)siz1,
			    ipsec6_logpacketstr(ip6, spi)));
			IPSEC_STAT_INCREMENT(ipsec6stat.in_inval);
			goto fail;
		}
		VERIFY((sizeof(struct ah) + sizoff + siz1) <= INT_MAX);
		IP6_EXTHDR_CHECK(m, off, (int)(sizeof(struct ah) + sizoff + siz1),
		    {return IPPROTO_DONE;});
		ip6 = mtod(m, struct ip6_hdr *);
		ah = (struct ah *)(void *)(mtod(m, caddr_t) + off);
	}

	  
                              
    
	if ((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay[0] != NULL) {
		if (ipsec_chkreplay(ntohl(((struct newah *)ah)->ah_seq), sav, 0)) {
			;         
		} else {
			IPSEC_STAT_INCREMENT(ipsec6stat.in_ahreplay);
			ipseclog((LOG_WARNING,
			    "replay packet in IPv6 AH input: %s %s\n",
			    ipsec6_logpacketstr(ip6, spi),
			    ipsec_logsastr(sav)));
			goto fail;
		}
	}

	  
                                                          
                           
    
	cksum = _MALLOC(siz1, M_TEMP, M_NOWAIT);
	if (!cksum) {
		ipseclog((LOG_DEBUG, "IPv6 AH input: "
		    "couldn't alloc temporary region for cksum\n"));
		IPSEC_STAT_INCREMENT(ipsec6stat.in_inval);
		goto fail;
	}

	if (ah6_calccksum(m, (caddr_t)cksum, siz1, algo, sav)) {
		FREE(cksum, M_TEMP);
		IPSEC_STAT_INCREMENT(ipsec6stat.in_inval);
		goto fail;
	}
	IPSEC_STAT_INCREMENT(ipsec6stat.in_ahhist[sav->alg_auth]);

	{
		caddr_t sumpos = NULL;

		if (sav->flags & SADB_X_EXT_OLD) {
			              
			sumpos = (caddr_t)(ah + 1);
		} else {
			              
			sumpos = (caddr_t)(((struct newah *)ah) + 1);
		}

		if (bcmp(sumpos, cksum, siz) != 0) {
			ipseclog((LOG_WARNING,
			    "checksum mismatch in IPv6 AH input: %s %s\n",
			    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
			FREE(cksum, M_TEMP);
			IPSEC_STAT_INCREMENT(ipsec6stat.in_ahauthfail);
			goto fail;
		}
	}

	FREE(cksum, M_TEMP);

	m->m_flags |= M_AUTHIPHDR;
	m->m_flags |= M_AUTHIPDGM;

	if (m->m_flags & M_AUTHIPHDR && m->m_flags & M_AUTHIPDGM) {
		IPSEC_STAT_INCREMENT(ipsec6stat.in_ahauthsucc);
	} else {
		ipseclog((LOG_WARNING,
		    "authentication failed in IPv6 AH input: %s %s\n",
		    ipsec6_logpacketstr(ip6, spi), ipsec_logsastr(sav)));
		IPSEC_STAT_INCREMENT(ipsec6stat.in_ahauthfail);
		goto fail;
	}

	  
                           
    
	if ((sav->flags & SADB_X_EXT_OLD) == 0 && sav->replay[0] != NULL) {
		if (ipsec_updatereplay(ntohl(((struct newah *)ah)->ah_seq), sav, 0)) {
			IPSEC_STAT_INCREMENT(ipsec6stat.in_ahreplay);
			goto fail;
		}
	}

	                                                  
	if (sav->flags & SADB_X_EXT_OLD) {
		              
		stripsiz = sizeof(struct ah) + siz1;
	} else {
		              
		stripsiz = sizeof(struct newah) + siz1;
	}
	if (ipsec6_tunnel_validate(m, (int)(off + stripsiz), nxt, sav, &ifamily)) {
		ifaddr_t ifa;
		struct sockaddr_storage addr;
		struct sockaddr_in6 *ip6addr;
		  
                                                
                                           
    
                           
                               
     
		u_int32_t flowinfo;                   

		if (ifamily == AF_INET) {
			ipseclog((LOG_NOTICE, "ipsec tunnel protocol mismatch "
			    "in IPv6 AH input: %s\n", ipsec_logsastr(sav)));
			goto fail;
		}

		flowinfo = ip6->ip6_flow;
		m_adj(m, (int)(off + stripsiz));
		if (m->m_len < sizeof(*ip6)) {
			  
                                                          
                               
      
			m = m_pullup(m, sizeof(*ip6));
			if (!m) {
				IPSEC_STAT_INCREMENT(ipsec6stat.in_inval);
				goto fail;
			}
		}
		ip6 = mtod(m, struct ip6_hdr *);
		                        
		if (ip6_ecn_egress(ip6_ipsec_ecn, &flowinfo, &ip6->ip6_flow) == 0) {
			IPSEC_STAT_INCREMENT(ipsec6stat.in_inval);
			goto fail;
		}
		if (!key_checktunnelsanity(sav, AF_INET6,
		    (caddr_t)&ip6->ip6_src, (caddr_t)&ip6->ip6_dst)) {
			ipseclog((LOG_NOTICE, "ipsec tunnel address mismatch "
			    "in IPv6 AH input: %s %s\n",
			    ipsec6_logpacketstr(ip6, spi),
			    ipsec_logsastr(sav)));
			IPSEC_STAT_INCREMENT(ipsec6stat.in_inval);
			goto fail;
		}

		  
                                                     
                                
     
		m->m_flags &= ~M_AUTHIPHDR;
		m->m_flags &= ~M_AUTHIPDGM;

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_AH, spi) != 0 ||
		    ipsec_addhist(m, IPPROTO_IPV6, 0) != 0) {
			IPSEC_STAT_INCREMENT(ipsec6stat.in_nomem);
			goto fail;
		}

		bzero(&addr, sizeof(addr));
		ip6addr = (__typeof__(ip6addr)) & addr;
		ip6addr->sin6_family = AF_INET6;
		ip6addr->sin6_len = sizeof(*ip6addr);
		ip6addr->sin6_addr = ip6->ip6_dst;

		                                                                    
		ifa = ifa_ifwithaddr((struct sockaddr *)&addr);
		if (ifa) {
			m->m_pkthdr.rcvif = ifa->ifa_ifp;
			IFA_REMREF(ifa);
		}

		                            
		lck_mtx_lock(sadb_mutex);
		ifnet_t ipsec_if = sav->sah->ipsec_if;
		if (ipsec_if != NULL) {
			                                                                           
			ifnet_reference(ipsec_if);
		}
		lck_mtx_unlock(sadb_mutex);
		if (ipsec_if != NULL) {
			errno_t inject_error = ipsec_inject_inbound_packet(ipsec_if, m);
			ifnet_release(ipsec_if);
			if (inject_error == 0) {
				m = NULL;
				nxt = IPPROTO_DONE;
				goto done;
			} else {
				goto fail;
			}
		}

		if (proto_input(PF_INET6, m) != 0) {
			goto fail;
		}
		nxt = IPPROTO_DONE;
	} else {
		  
                  
     
		char *prvnxtp;

		  
                                                         
                                              
                                                             
     
		prvnxtp = ip6_get_prevhdr(m, off);          
		*prvnxtp = (u_int8_t)nxt;

		ip6 = mtod(m, struct ip6_hdr *);
		  
                                             
                                           
     
		ovbcopy((caddr_t)ip6, ((caddr_t)ip6) + stripsiz, off);
		m->m_data += stripsiz;
		m->m_len -= stripsiz;
		m->m_pkthdr.len -= stripsiz;
		ip6 = mtod(m, struct ip6_hdr *);
		                   
		ip6->ip6_plen = htons((u_int16_t)(ntohs(ip6->ip6_plen) - stripsiz));

		key_sa_recordxfer(sav, m);
		if (ipsec_addhist(m, IPPROTO_AH, spi) != 0) {
			IPSEC_STAT_INCREMENT(ipsec6stat.in_nomem);
			goto fail;
		}

		                            
		lck_mtx_lock(sadb_mutex);
		ifnet_t ipsec_if = sav->sah->ipsec_if;
		if (ipsec_if != NULL) {
			                                                                           
			ifnet_reference(ipsec_if);
		}
		lck_mtx_unlock(sadb_mutex);
		if (ipsec_if != NULL) {
			errno_t inject_error = ipsec_inject_inbound_packet(ipsec_if, m);
			ifnet_release(ipsec_if);
			if (inject_error == 0) {
				m = NULL;
				nxt = IPPROTO_DONE;
				goto done;
			} else {
				goto fail;
			}
		}
	}

done:
	*offp = off;
	*mp = m;
	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		    printf("DP ah6_input call free SA:0x%llx\n",
		    (uint64_t)VM_KERNEL_ADDRPERM(sav)));
		key_freesav(sav, KEY_SADB_UNLOCKED);
	}
	IPSEC_STAT_INCREMENT(ipsec6stat.in_success);
	return nxt;

fail:
	if (sav) {
		KEYDEBUG(KEYDEBUG_IPSEC_STAMP,
		    printf("DP ah6_input call free SA:0x%llx\n",
		    (uint64_t)VM_KERNEL_ADDRPERM(sav)));
		key_freesav(sav, KEY_SADB_UNLOCKED);
	}
	if (m) {
		m_freem(m);
		*mp = NULL;
	}
	return IPPROTO_DONE;
}

void
ah6_ctlinput(int cmd, struct sockaddr *sa, void *d)
{
	const struct newah *ahp;
	struct newah ah;
	struct secasvar *sav;
	struct ip6_hdr *ip6;
	struct mbuf *m;
	struct ip6ctlparam *ip6cp = NULL;
	int off = 0;
	struct sockaddr_in6 *sa6_src, *sa6_dst;

	if (sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(struct sockaddr_in6)) {
		return;
	}
	if ((unsigned)cmd >= PRC_NCMDS) {
		return;
	}

	                                                
	if (d != NULL) {
		ip6cp = (struct ip6ctlparam *)d;
		m = ip6cp->ip6c_m;
		ip6 = ip6cp->ip6c_ip6;
		off = ip6cp->ip6c_off;
	} else {
		m = NULL;
		ip6 = NULL;
	}

	if (ip6) {
		  
                                              
                         
     

		                                                      
		if (m->m_pkthdr.len < off + sizeof(ah)) {
			return;
		}

		if (m->m_len < off + sizeof(ah)) {
			  
                               
                                      
      
			m_copydata(m, off, sizeof(ah), (caddr_t)&ah);
			ahp = &ah;
		} else {
			ahp = (struct newah *)(void *)(mtod(m, caddr_t) + off);
		}

		if (cmd == PRC_MSGSIZE) {
			int valid = 0;

			  
                                                         
                                              
      
			sa6_src = ip6cp->ip6c_src;
			sa6_dst = (struct sockaddr_in6 *)(void *)sa;
			sav = key_allocsa(AF_INET6,
			    (caddr_t)&sa6_src->sin6_addr,
			    (caddr_t)&sa6_dst->sin6_addr,
			    IPPROTO_AH, ahp->ah_spi);
			if (sav) {
				if (sav->state == SADB_SASTATE_MATURE ||
				    sav->state == SADB_SASTATE_DYING) {
					valid++;
				}
				key_freesav(sav, KEY_SADB_UNLOCKED);
			}

			                             

			  
                                                         
                                         
                                              
                                       
                                           
      
			icmp6_mtudisc_update((struct ip6ctlparam *)d, valid);
		}

		                                        
	} else {
		                                     
	}
}
