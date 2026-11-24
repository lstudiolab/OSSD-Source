/*
 * Copyright (c) 2009-2020 Apple Inc. All rights reserved.
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
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/icmp6.h>
#include <netinet6/nd6.h>
#include <netinet6/scope6_var.h>

#include <netinet/in_pcb.h>

#if IPSEC
#include <netinet6/ipsec.h>
#include <netinet6/ipsec6.h>
#include <netkey/key.h>
extern int ipsec_bypass;
#endif            

#include <net/net_osdep.h>

#if DUMMYNET
#include <netinet/ip_dummynet.h>
#endif               

#if PF
#include <net/pfvar.h>
#endif         

  
                                                            
                                                              
                                                                   
                      
  
                                                                    
                                                                   
                                                                  
                           
  
   

struct mbuf *
ip6_forward(struct mbuf *m, struct route_in6 *ip6forward_rt,
    int srcrt)
{
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	struct sockaddr_in6 *dst;
	struct rtentry *rt;
	int error, type = 0, code = 0;
	boolean_t proxy = FALSE;
	struct mbuf *mcopy = NULL;
	struct ifnet *ifp, *rcvifp, *origifp;                          
	u_int32_t inzone, outzone, len = 0, pktcnt = 0;
	struct in6_addr src_in6, dst_in6;
	uint64_t curtime = net_uptime();
#if IPSEC
	struct secpolicy *sp = NULL;
#endif
	unsigned int ifscope = IFSCOPE_NONE;
#if PF
	struct pf_mtag *pf_mtag;
	struct pf_fragment_tag *pf_ftagp, pf_ftag;
	boolean_t pf_ftag_valid = FALSE;
#endif         
	uint32_t mpktlen = 0;

	  
                                                                       
                                                                    
                                                                   
                                                                        
                                                                       
                                                                     
                                                                   
                                                                      
                                                                       
    
	if (!srcrt && nd6_prproxy &&
	    (rt = ip6forward_rt->ro_rt) != NULL && (rt->rt_flags & RTF_PROXY)) {
		nd6_proxy_find_fwdroute(m->m_pkthdr.rcvif, ip6forward_rt);
		if ((rt = ip6forward_rt->ro_rt) != NULL) {
			ifscope = rt->rt_ifp->if_index;
		}
	}

#if PF
	pf_mtag = pf_find_mtag(m);
	  
                                                              
                                      
    
	pf_ftagp = pf_find_fragment_tag(m);
	if (pf_ftagp != NULL) {
		ASSERT(pf_mtag->pftag_flags & PF_TAG_REASSEMBLED);
		pf_ftag = *pf_ftagp;
		pf_ftag_valid = TRUE;
		mpktlen = pf_ftag.ft_maxlen;
		ASSERT(mpktlen);
	}
	if (pf_mtag != NULL && pf_mtag->pftag_rtableid != IFSCOPE_NONE) {
		ifscope = pf_mtag->pftag_rtableid;
	}
	pf_mtag = NULL;
	pf_ftagp = NULL;
	  
                                                                    
                                                                   
                          
    
	if (ifscope != IFSCOPE_NONE && (rt = ip6forward_rt->ro_rt) != NULL) {
		RT_LOCK(rt);
		if (rt->rt_ifp->if_index != ifscope) {
			RT_UNLOCK(rt);
			ROUTE_RELEASE(ip6forward_rt);
			rt = NULL;
		} else {
			RT_UNLOCK(rt);
		}
	}
#endif         

#if IPSEC
	  
                           
    
	  
                                                              
                                      
    
	if (ipsec_bypass == 0) {
		if (ipsec6_in_reject(m, NULL)) {
			IPSEC_STAT_INCREMENT(ipsec6stat.in_polvio);
			m_freem(m);
			return NULL;
		}
	}
#endif          

	  
                                                    
                                                                     
                                         
    
	if ((m->m_flags & (M_BCAST | M_MCAST)) != 0 ||
	    IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst) ||
	    IN6_IS_ADDR_UNSPECIFIED(&ip6->ip6_src)) {
		ip6stat.ip6s_cantforward++;
		                                                     
		if (ip6_log_time + ip6_log_interval < curtime) {
			ip6_log_time = curtime;
			log(LOG_DEBUG,
			    "cannot forward "
			    "from %s to %s nxt %d received on %s\n",
			    ip6_sprintf(&ip6->ip6_src),
			    ip6_sprintf(&ip6->ip6_dst),
			    ip6->ip6_nxt,
			    if_name(m->m_pkthdr.rcvif));
		}
		m_freem(m);
		return NULL;
	}

	if (ip6->ip6_hlim <= IPV6_HLIMDEC) {
		                                                     
		icmp6_error_flag(m, ICMP6_TIME_EXCEEDED,
		    ICMP6_TIME_EXCEED_TRANSIT, 0, 0);
		return NULL;
	}

	  
                                                                  
                                                                  
                                                                  
                                
    
	if (!srcrt && nd6_prproxy) {
		VERIFY(!IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst));
		proxy = nd6_prproxy_isours(m, ip6, ip6forward_rt, ifscope);
		  
                                                         
                                                       
                                   
     
		if (proxy) {
			goto skip_ipsec;
		}
	}

	ip6->ip6_hlim -= IPV6_HLIMDEC;

	  
                                                        
                                                              
                                                    
                                                       
   
                                                               
                                   
    
	mcopy = m_copy(m, 0, imin(m->m_pkthdr.len, ICMPV6_PLD_MAXLEN));

#if IPSEC
	if (ipsec_bypass != 0) {
		goto skip_ipsec;
	}
	                                           
	sp = ipsec6_getpolicybyaddr(m, IPSEC_DIR_OUTBOUND, IP_FORWARDING,
	    &error);
	if (sp == NULL) {
		IPSEC_STAT_INCREMENT(ipsec6stat.out_inval);
		ip6stat.ip6s_cantforward++;
		if (mcopy) {
#if 0
			                      
#else
			m_freem(mcopy);
#endif
		}
		m_freem(m);
		return NULL;
	}

	error = 0;

	                  
	switch (sp->policy) {
	case IPSEC_POLICY_DISCARD:
	case IPSEC_POLICY_GENERATE:
		  
                                   
     
		IPSEC_STAT_INCREMENT(ipsec6stat.out_polvio);
		ip6stat.ip6s_cantforward++;
		key_freesp(sp, KEY_SADB_UNLOCKED);
		if (mcopy) {
#if 0
			                      
#else
			m_freem(mcopy);
#endif
		}
		m_freem(m);
		return NULL;

	case IPSEC_POLICY_BYPASS:
	case IPSEC_POLICY_NONE:
		                          
		key_freesp(sp, KEY_SADB_UNLOCKED);
		goto skip_ipsec;

	case IPSEC_POLICY_IPSEC:
		if (sp->req == NULL) {
			                           
			printf("ip6_forward: No IPsec request specified.\n");
			ip6stat.ip6s_cantforward++;
			key_freesp(sp, KEY_SADB_UNLOCKED);
			if (mcopy) {
#if 0
				                      
#else
				m_freem(mcopy);
#endif
			}
			m_freem(m);
			return NULL;
		}
		              
		break;

	case IPSEC_POLICY_ENTRUST:
	default:
		                        
		printf("ip6_forward: Invalid policy found. %d\n", sp->policy);
		key_freesp(sp, KEY_SADB_UNLOCKED);
		goto skip_ipsec;
	}

	{
		struct ipsec_output_state state;

		  
                                                       
                                   
                                                              
                                                            
    
                                                   
     
		bzero(&state, sizeof(state));
		state.m = m;
		state.dst = NULL;                                       

		error = ipsec6_output_tunnel(&state, sp, 0);
		key_freesp(sp, KEY_SADB_UNLOCKED);
		if (state.tunneled == 4) {
			ROUTE_RELEASE(&state.ro);
			return NULL;                                      
		}

		m = state.m;
		ROUTE_RELEASE(&state.ro);

		if (error) {
			                                                        
			switch (error) {
			case EHOSTUNREACH:
			case ENETUNREACH:
			case EMSGSIZE:
			case ENOBUFS:
			case ENOMEM:
				break;
			default:
				printf("ip6_output (ipsec): error code %d\n", error);
				OS_FALLTHROUGH;
			case ENOENT:
				                                              
				break;
			}
			ip6stat.ip6s_cantforward++;
			if (mcopy) {
#if 0
				                      
#else
				m_freem(mcopy);
#endif
			}
			m_freem(m);
			return NULL;
		}
	}
#endif            
skip_ipsec:

	dst = (struct sockaddr_in6 *)&ip6forward_rt->ro_dst;
	if ((rt = ip6forward_rt->ro_rt) != NULL) {
		RT_LOCK(rt);
		                                     
		RT_ADDREF_LOCKED(rt);
	}

	VERIFY(rt == NULL || rt == ip6forward_rt->ro_rt);
	if (!srcrt) {
		  
                                                             
     
		if (ROUTE_UNUSABLE(ip6forward_rt)) {
			if (rt != NULL) {
				                       
				RT_REMREF_LOCKED(rt);
				RT_UNLOCK(rt);
			}
			ROUTE_RELEASE(ip6forward_rt);

			                                                 
			rtalloc_scoped_ign((struct route *)ip6forward_rt,
			    RTF_PRCLONING, ifscope);
			if ((rt = ip6forward_rt->ro_rt) != NULL) {
				RT_LOCK(rt);
				                                     
				RT_ADDREF_LOCKED(rt);
			}
		}

		if (rt == NULL) {
			ip6stat.ip6s_noroute++;
			in6_ifstat_inc(m->m_pkthdr.rcvif, ifs6_in_noroute);
			if (mcopy) {
				icmp6_error(mcopy, ICMP6_DST_UNREACH,
				    ICMP6_DST_UNREACH_NOROUTE, 0);
			}
			m_freem(m);
			return NULL;
		}
		RT_LOCK_ASSERT_HELD(rt);
	} else if (ROUTE_UNUSABLE(ip6forward_rt) ||
	    !IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &dst->sin6_addr)) {
		if (rt != NULL) {
			                       
			RT_REMREF_LOCKED(rt);
			RT_UNLOCK(rt);
		}
		ROUTE_RELEASE(ip6forward_rt);

		bzero(dst, sizeof(*dst));
		dst->sin6_len = sizeof(struct sockaddr_in6);
		dst->sin6_family = AF_INET6;
		dst->sin6_addr = ip6->ip6_dst;

		rtalloc_scoped_ign((struct route *)ip6forward_rt,
		    RTF_PRCLONING, ifscope);
		if ((rt = ip6forward_rt->ro_rt) == NULL) {
			ip6stat.ip6s_noroute++;
			in6_ifstat_inc(m->m_pkthdr.rcvif, ifs6_in_noroute);
			if (mcopy) {
				icmp6_error(mcopy, ICMP6_DST_UNREACH,
				    ICMP6_DST_UNREACH_NOROUTE, 0);
			}
			m_freem(m);
			return NULL;
		}
		RT_LOCK(rt);
		                                     
		RT_ADDREF_LOCKED(rt);
	}

	  
                                                             
                                                                       
                                                                 
                                                                     
                                                                 
                                                                    
                                            
                                                   
    
	src_in6 = ip6->ip6_src;
	if (in6_setscope(&src_in6, rt->rt_ifp, &outzone)) {
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
		                                 
		ip6stat.ip6s_cantforward++;
		ip6stat.ip6s_badscope++;
		m_freem(m);
		return NULL;
	}
	if (in6_setscope(&src_in6, m->m_pkthdr.rcvif, &inzone)) {
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
		ip6stat.ip6s_cantforward++;
		ip6stat.ip6s_badscope++;
		m_freem(m);
		return NULL;
	}

	if (inzone != outzone && !proxy) {
		ip6stat.ip6s_cantforward++;
		ip6stat.ip6s_badscope++;
		in6_ifstat_inc(rt->rt_ifp, ifs6_in_discard);

		if (ip6_log_time + ip6_log_interval < curtime) {
			ip6_log_time = curtime;
			log(LOG_DEBUG,
			    "cannot forward "
			    "src %s, dst %s, nxt %d, rcvif %s, outif %s\n",
			    ip6_sprintf(&ip6->ip6_src),
			    ip6_sprintf(&ip6->ip6_dst),
			    ip6->ip6_nxt,
			    if_name(m->m_pkthdr.rcvif), if_name(rt->rt_ifp));
		}
		                       
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
		if (mcopy) {
			icmp6_error(mcopy, ICMP6_DST_UNREACH,
			    ICMP6_DST_UNREACH_BEYONDSCOPE, 0);
		}
		m_freem(m);
		return NULL;
	}

	  
                                                                    
                                                                       
                                                                       
                                                                   
                                                         
    
	dst_in6 = ip6->ip6_dst;
	if (in6_setscope(&dst_in6, m->m_pkthdr.rcvif, &inzone) != 0 ||
	    in6_setscope(&dst_in6, rt->rt_ifp, &outzone) != 0 ||
	    inzone != outzone) {
		RT_REMREF_LOCKED(rt);
		RT_UNLOCK(rt);
		ip6stat.ip6s_cantforward++;
		ip6stat.ip6s_badscope++;
		m_freem(m);
		return NULL;
	}

	if (mpktlen == 0) {
		mpktlen = m->m_pkthdr.len;
	}

	if (mpktlen > rt->rt_ifp->if_mtu) {
		in6_ifstat_inc(rt->rt_ifp, ifs6_in_toobig);
		if (mcopy) {
			uint32_t mtu;
#if IPSEC
			struct secpolicy *sp2;
			int ipsecerror;
			size_t ipsechdrsiz;
#endif

			mtu = rt->rt_ifp->if_mtu;
#if IPSEC
			  
                                                      
                                                      
                                                        
                                                 
                                          
      
			sp2 = ipsec6_getpolicybyaddr(mcopy, IPSEC_DIR_OUTBOUND,
			    IP_FORWARDING, &ipsecerror);
			if (sp2) {
				ipsechdrsiz = ipsec6_hdrsiz(mcopy,
				    IPSEC_DIR_OUTBOUND, NULL);
				if (ipsechdrsiz < mtu) {
					mtu -= ipsechdrsiz;
				}
				key_freesp(sp2, KEY_SADB_UNLOCKED);
			}
			  
                                           
                                                      
      
			if (mtu < IPV6_MMTU) {
				mtu = IPV6_MMTU;
			}
#endif
			                       
			RT_REMREF_LOCKED(rt);
			RT_UNLOCK(rt);
			icmp6_error(mcopy, ICMP6_PACKET_TOO_BIG, 0, mtu);
		} else {
			                       
			RT_REMREF_LOCKED(rt);
			RT_UNLOCK(rt);
		}
		m_freem(m);
		return NULL;
	}

	if (rt->rt_flags & RTF_GATEWAY) {
		dst = (struct sockaddr_in6 *)(void *)rt->rt_gateway;
	}

	  
                                                            
                                                                    
                                
                                                           
                                                             
                                                         
                           
    
	if (!proxy &&
	    ip6_sendredirects && rt->rt_ifp == m->m_pkthdr.rcvif && !srcrt &&
	    (rt->rt_flags & (RTF_DYNAMIC | RTF_MODIFIED)) == 0) {
		if ((rt->rt_ifp->if_flags & IFF_POINTOPOINT) != 0) {
			  
                                                        
                                                    
                                                     
                                                      
                                                       
     
                                                      
                                      
      
			RT_REMREF_LOCKED(rt);                          
			RT_UNLOCK(rt);
			icmp6_error(mcopy, ICMP6_DST_UNREACH,
			    ICMP6_DST_UNREACH_ADDR, 0);
			m_freem(m);
			return NULL;
		}
		type = ND_REDIRECT;
	}
	  
                                                              
                                                                 
                                                                  
                                                                      
                                                                  
                       
    
	if ((rt->rt_ifp->if_flags & IFF_LOOPBACK) != 0) {
		  
                                              
                                                              
                                                             
                                        
                                                            
                                                        
                    
     
#if 1
		if ((0))
#else
		if ((rt->rt_flags & (RTF_BLACKHOLE | RTF_REJECT)) == 0)
#endif
		{
			printf("ip6_forward: outgoing interface is loopback. "
			    "src %s, dst %s, nxt %d, rcvif %s, outif %s\n",
			    ip6_sprintf(&ip6->ip6_src),
			    ip6_sprintf(&ip6->ip6_dst),
			    ip6->ip6_nxt, if_name(m->m_pkthdr.rcvif),
			    if_name(rt->rt_ifp));
		}

		                                          
		origifp = rcvifp = m->m_pkthdr.rcvif;
	} else if (nd6_prproxy) {
		  
                                                                
                                                           
                                                             
                                      
     
		origifp = rcvifp = m->m_pkthdr.rcvif;
	} else {
		rcvifp = m->m_pkthdr.rcvif;
		origifp = rt->rt_ifp;
	}
	  
                                                  
                                                                
    
	in6_clearscope(&ip6->ip6_src);
	in6_clearscope(&ip6->ip6_dst);

	ifp = rt->rt_ifp;
	                                            
	RT_UNLOCK(rt);

	  
                                                              
    
	if (proxy) {
		VERIFY(m->m_pkthdr.pkt_flags & PKTF_PROXY_DST);
		                       
		RT_REMREF(rt);
		if (mcopy != NULL) {
			m_freem(mcopy);
		}
		return m;
	}

	                                                                
	m->m_pkthdr.pkt_flags |= PKTF_FORWARDED;

#if PF
	if (PF_IS_ENABLED) {
		  
                                                                
                                                             
                         
     
		if (pf_ftag_valid) {
			pf_copy_fragment_tag(m, &pf_ftag, M_DONTWAIT);
		}
#if DUMMYNET
		struct ip_fw_args args;
		bzero(&args, sizeof(args));

		args.fwa_m = m;
		args.fwa_oif = ifp;
		args.fwa_oflags = 0;
		args.fwa_ro6 = ip6forward_rt;
		args.fwa_ro6_pmtu = ip6forward_rt;
		args.fwa_mtu = rt->rt_ifp->if_mtu;
		args.fwa_dst6 = dst;
		args.fwa_origifp = origifp;
		                                   
		error = pf_af_hook(ifp, NULL, &m, AF_INET6, FALSE, &args);
#else                
		error = pf_af_hook(ifp, NULL, &m, AF_INET6, FALSE, NULL);
#endif                
		if (error != 0 || m == NULL) {
			if (m != NULL) {
				panic("%s: unexpected packet %p\n", __func__, m);
				                
			}
			                             
			goto senderr;
		}

		pf_mtag = pf_find_mtag(m);
		  
                                  
     
		if ((pf_mtag->pftag_flags & PF_TAG_REFRAGMENTED) != 0) {
			struct mbuf *t;

			pf_mtag->pftag_flags &= ~PF_TAG_REFRAGMENTED;
			                    
			t = m;
			while (t != NULL) {
				pktcnt++;
				len += m_pktlen(t);
				t = t->m_nextpkt;
			}

			  
                                                        
                    
      
			error = nd6_output(ifp, origifp, m, dst, rt, NULL);
			m = NULL;
			goto sent;
		}
		  
                                                      
                                                      
                                                   
                  
     
		ip6 = mtod(m, struct ip6_hdr *);
	}
#endif         

	len = m_pktlen(m);
	pktcnt = 1;
	error = nd6_output(ifp, origifp, m, dst, rt, NULL);
sent:
	if (error) {
		in6_ifstat_add(ifp, ifs6_out_discard, pktcnt);
		ip6stat.ip6s_cantforward += pktcnt;
	} else {
		  
                                                      
                                                     
                                                     
     
		rcvifp->if_fpackets += pktcnt;
		rcvifp->if_fbytes += len;

		ip6stat.ip6s_forward += pktcnt;
		in6_ifstat_add(ifp, ifs6_out_forward, pktcnt);
		if (type) {
			ip6stat.ip6s_redirectsent++;
		} else {
			if (mcopy) {
				goto freecopy;
			}
		}
	}
#if PF
senderr:
#endif         
	if (mcopy == NULL) {
		                       
		RT_REMREF(rt);
		return NULL;
	}
	switch (error) {
	case 0:
#if 1
		if (type == ND_REDIRECT) {
			icmp6_redirect_output(mcopy, rt);
			                       
			RT_REMREF(rt);
			return NULL;
		}
#endif
		goto freecopy;

	case EMSGSIZE:
		                                 
		goto freecopy;

	case ENOBUFS:
		                                                        
		goto freecopy;

	case ENETUNREACH:                                            
	case EHOSTUNREACH:
	case ENETDOWN:
	case EHOSTDOWN:
	default:
		type = ICMP6_DST_UNREACH;
		code = ICMP6_DST_UNREACH_ADDR;
		break;
	}
	icmp6_error(mcopy, type, code, 0);
	                       
	RT_REMREF(rt);
	return NULL;

freecopy:
	m_freem(mcopy);
	                       
	RT_REMREF(rt);
	return NULL;
}
