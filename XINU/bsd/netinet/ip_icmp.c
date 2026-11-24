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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1982, 1986, 1988, 1993
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
 *	@(#)ip_icmp.c	8.2 (Berkeley) 1/4/94
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/mcache.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <machine/endian.h>

#include <net/if.h>
#include <net/route.h>
#include <net/content_filter.h>

#define _IP_VHL
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <netinet/icmp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>

#if IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#endif

#if NECP
#include <net/necp.h>
#endif           


  
                                                                  
                                                             
                                   
   

struct  icmpstat icmpstat;
SYSCTL_STRUCT(_net_inet_icmp, ICMPCTL_STATS, stats,
    CTLFLAG_RD | CTLFLAG_LOCKED,
    &icmpstat, icmpstat, "");

static int      icmpmaskrepl = 0;
SYSCTL_INT(_net_inet_icmp, ICMPCTL_MASKREPL, maskrepl,
    CTLFLAG_RW | CTLFLAG_LOCKED,
    &icmpmaskrepl, 0, "");

static int      icmptimestamp = 0;
SYSCTL_INT(_net_inet_icmp, ICMPCTL_TIMESTAMP, timestamp,
    CTLFLAG_RW | CTLFLAG_LOCKED,
    &icmptimestamp, 0, "");

static int      drop_redirect = 1;
SYSCTL_INT(_net_inet_icmp, OID_AUTO, drop_redirect,
    CTLFLAG_RW | CTLFLAG_LOCKED,
    &drop_redirect, 0, "");

static int      log_redirect = 0;
SYSCTL_INT(_net_inet_icmp, OID_AUTO, log_redirect,
    CTLFLAG_RW | CTLFLAG_LOCKED,
    &log_redirect, 0, "");

const static int icmp_datalen = 8;

#if ICMP_BANDLIM
                                                                                  
#ifndef CONFIG_ICMP_BANDLIM
#if XNU_TARGET_OS_OSX
#define CONFIG_ICMP_BANDLIM 250
#else                         
#define CONFIG_ICMP_BANDLIM 50
#endif                         
#endif                          

  
                                                                         
                                             
   

static int      icmplim = CONFIG_ICMP_BANDLIM;
SYSCTL_INT(_net_inet_icmp, ICMPCTL_ICMPLIM, icmplim, CTLFLAG_RW | CTLFLAG_LOCKED,
    &icmplim, 0, "");
#else                   
static int      icmplim = -1;
SYSCTL_INT(_net_inet_icmp, ICMPCTL_ICMPLIM, icmplim, CTLFLAG_RD | CTLFLAG_LOCKED,
    &icmplim, 0, "");
#endif                   

static int      icmplim_random_incr = CONFIG_ICMP_BANDLIM;
SYSCTL_INT(_net_inet_icmp, ICMPCTL_ICMPLIM_INCR, icmplim_random_incr, CTLFLAG_RW | CTLFLAG_LOCKED,
    &icmplim_random_incr, 0, "");

  
                             
   

static int      icmpbmcastecho = 1;
SYSCTL_INT(_net_inet_icmp, OID_AUTO, bmcastecho, CTLFLAG_RW | CTLFLAG_LOCKED,
    &icmpbmcastecho, 0, "");

#if (DEBUG | DEVELOPMENT)
static int      icmpprintfs = 0;
SYSCTL_INT(_net_inet_icmp, OID_AUTO, verbose, CTLFLAG_RW | CTLFLAG_LOCKED,
    &icmpprintfs, 0, "");
#endif

static void     icmp_reflect(struct mbuf *);
static void     icmp_send(struct mbuf *, struct mbuf *);

  
                                         
                                
   
void
icmp_error(
	struct mbuf *n,
	int type,
	int code,
	u_int32_t dest,
	u_int32_t nextmtu)
{
	struct ip *oip = NULL;
	struct ip *nip = NULL;
	struct icmp *icp = NULL;
	struct mbuf *m = NULL;
	u_int32_t oiphlen = 0;
	u_int32_t icmplen = 0;
	u_int32_t icmpelen = 0;
	u_int32_t nlen = 0;

	VERIFY((u_int)type <= ICMP_MAXTYPE);
	VERIFY(code <= UINT8_MAX);

	                                                                  
	MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(n);

	if (type != ICMP_REDIRECT) {
		icmpstat.icps_error++;
	}
	  
                     
                                          
                                                            
                                         
                                                    
    
	if (n->m_flags & (M_BCAST | M_MCAST)) {
		goto freeit;
	}

	  
                                                               
                  
    
	if (n->m_len < sizeof(struct ip) + ICMP_MINLEN) {
		goto freeit;
	}

	oip = mtod(n, struct ip *);
	oiphlen = IP_VHL_HL(oip->ip_vhl) << 2;
	if (n->m_len < oiphlen + ICMP_MINLEN) {
		goto freeit;
	}

#if (DEBUG | DEVELOPMENT)
	if (icmpprintfs > 1) {
		printf("icmp_error(0x%llx, %x, %d)\n",
		    (uint64_t)VM_KERNEL_ADDRPERM(oip), type, code);
	}
#endif

	if (oip->ip_off & ~(IP_MF | IP_DF)) {
		goto freeit;
	}

	if (oip->ip_p == IPPROTO_ICMP && type != ICMP_REDIRECT &&
	    n->m_len >= oiphlen + ICMP_MINLEN &&
	    !ICMP_INFOTYPE(((struct icmp *)(void *)((caddr_t)oip + oiphlen))->
	    icmp_type)) {
		icmpstat.icps_oldicmp++;
		goto freeit;
	}

	  
                                                                  
                                   
                                                       
                               
    
	nlen = m_length(n);
	if (oip->ip_p == IPPROTO_TCP) {
		struct tcphdr *th = NULL;
		u_int16_t tcphlen = 0;

		  
                                               
                                             
                                                  
     
		if (oiphlen + sizeof(struct tcphdr) > n->m_len &&
		    n->m_next == NULL) {
			goto stdreply;
		}

		  
                                                 
             
     
		if (n->m_len < (oiphlen + sizeof(struct tcphdr)) &&
		    (n = m_pullup(n, (oiphlen + sizeof(struct tcphdr)))) == NULL) {
			goto freeit;
		}

		  
                                                   
                                                    
     
		oip = mtod(n, struct ip *);
		th = (struct tcphdr *)(void *)((caddr_t)oip + oiphlen);

		if (th != ((struct tcphdr *)P2ROUNDDOWN(th,
		    sizeof(u_int32_t))) ||
		    ((th->th_off << 2) > UINT16_MAX)) {
			goto freeit;
		}
		tcphlen = (uint16_t)(th->th_off << 2);

		                   
		if (tcphlen < sizeof(struct tcphdr)) {
			goto freeit;
		}
		if (oip->ip_len < (oiphlen + tcphlen)) {
			goto freeit;
		}
		if ((oiphlen + tcphlen) > n->m_len && n->m_next == NULL) {
			goto stdreply;
		}
		if (n->m_len < (oiphlen + tcphlen) &&
		    (n = m_pullup(n, (oiphlen + tcphlen))) == NULL) {
			goto freeit;
		}

		  
                                                   
                                                    
     
		oip = mtod(n, struct ip *);
		th = (struct tcphdr *)(void *)((caddr_t)oip + oiphlen);

		icmpelen = max(tcphlen, min(icmp_datalen,
		    (oip->ip_len - oiphlen)));
	} else {
stdreply:       icmpelen = max(ICMP_MINLEN, min(icmp_datalen,
		    (oip->ip_len - oiphlen)));
	}

	icmplen = min(oiphlen + icmpelen, nlen);
	if (icmplen < sizeof(struct ip)) {
		goto freeit;
	}

	  
                                 
                                                        
                                                                   
    
	if (MHLEN > (sizeof(struct ip) + ICMP_MINLEN + icmplen)) {
		m = m_gethdr(M_DONTWAIT, MT_HEADER);                
	} else {
		m = m_getcl(M_DONTWAIT, MT_DATA, M_PKTHDR);
	}

	if (m == NULL) {
		goto freeit;
	}

	  
                                                  
                                                            
           
    
	icmplen = min(icmplen, (u_int)M_TRAILINGSPACE(m) -
	    (u_int)(sizeof(struct ip) - ICMP_MINLEN));
	m_align(m, ICMP_MINLEN + icmplen);
	m->m_len = ICMP_MINLEN + icmplen;                               

	icp = mtod(m, struct icmp *);
	icmpstat.icps_outhist[type]++;
	icp->icmp_type = (u_char)type;
	if (type == ICMP_REDIRECT) {
		icp->icmp_gwaddr.s_addr = dest;
	} else {
		icp->icmp_void = 0;
		  
                                                         
                            
     
		if (type == ICMP_PARAMPROB) {
			icp->icmp_pptr = (u_char)code;
			code = 0;
		} else if (type == ICMP_UNREACH &&
		    code == ICMP_UNREACH_NEEDFRAG && nextmtu != 0) {
			icp->icmp_nextmtu = htons((uint16_t)nextmtu);
		}
	}

	icp->icmp_code = (u_char)code;

	  
                                               
                                                 
    
	m_copydata(n, 0, icmplen, (caddr_t)&icp->icmp_ip);
	nip = &icp->icmp_ip;

	  
                                             
    
#if BYTE_ORDER != BIG_ENDIAN
	HTONS(nip->ip_len);
	HTONS(nip->ip_off);
#endif
	  
                                                                    
                             
    
	m->m_data -= sizeof(struct ip);
	m->m_len += sizeof(struct ip);
	m->m_pkthdr.len = m->m_len;
	m->m_pkthdr.rcvif = n->m_pkthdr.rcvif;
	nip = mtod(m, struct ip *);
	bcopy((caddr_t)oip, (caddr_t)nip, sizeof(struct ip));
	nip->ip_len = (uint16_t)m->m_len;
	nip->ip_vhl = IP_VHL_BORING;
	nip->ip_p = IPPROTO_ICMP;
	nip->ip_tos = 0;
	nip->ip_off = 0;
	icmp_reflect(m);
freeit:
	m_freem(n);
}

  
                                   
   
void
icmp_input(struct mbuf *m, int hlen)
{
	struct sockaddr_in icmpsrc, icmpdst, icmpgw;
	struct icmp *icp;
	struct ip *ip = mtod(m, struct ip *);
	int icmplen;
	int i;
	struct in_ifaddr *ia;
	void (*ctlfunc)(int, struct sockaddr *, void *, struct ifnet *);
	int code;
	boolean_t should_log_redirect = false;

	                                                                  
	MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(m);

	icmplen = ip->ip_len;

	  
                                            
                                                      
    
#if (DEBUG | DEVELOPMENT)
	if (icmpprintfs > 2) {
		char src_str[MAX_IPv4_STR_LEN];
		char dst_str[MAX_IPv4_STR_LEN];

		inet_ntop(AF_INET, &ip->ip_src, src_str, sizeof(src_str));
		inet_ntop(AF_INET, &ip->ip_dst, dst_str, sizeof(dst_str));
		printf("%s: from %s to %s, len %d\n",
		    __func__, src_str, dst_str, icmplen);
	}
#endif
	if (icmplen < ICMP_MINLEN) {
		icmpstat.icps_tooshort++;
		goto freeit;
	}
	i = hlen + min(icmplen, ICMP_ADVLENMIN);
	if (m->m_len < i && (m = m_pullup(m, i)) == 0) {
		icmpstat.icps_tooshort++;
		return;
	}
	ip = mtod(m, struct ip *);
	m->m_len -= hlen;
	m->m_data += hlen;
	icp = mtod(m, struct icmp *);
	if (in_cksum(m, icmplen)) {
		icmpstat.icps_checksum++;
		goto freeit;
	}
	m->m_len += hlen;
	m->m_data -= hlen;

#if (DEBUG | DEVELOPMENT)
	if (icmpprintfs > 2) {
		printf("icmp_input, type %d code %d\n", icp->icmp_type,
		    icp->icmp_code);
	}
#endif

	  
                                     
    
	if (icp->icmp_type > ICMP_MAXTYPE) {
		goto raw;
	}

	                
	bzero(&icmpsrc, sizeof(icmpsrc));
	icmpsrc.sin_len = sizeof(struct sockaddr_in);
	icmpsrc.sin_family = AF_INET;
	bzero(&icmpdst, sizeof(icmpdst));
	icmpdst.sin_len = sizeof(struct sockaddr_in);
	icmpdst.sin_family = AF_INET;
	bzero(&icmpgw, sizeof(icmpgw));
	icmpgw.sin_len = sizeof(struct sockaddr_in);
	icmpgw.sin_family = AF_INET;

	icmpstat.icps_inhist[icp->icmp_type]++;
	code = icp->icmp_code;
	switch (icp->icmp_type) {
	case ICMP_UNREACH:
		switch (code) {
		case ICMP_UNREACH_NET:
		case ICMP_UNREACH_HOST:
		case ICMP_UNREACH_SRCFAIL:
		case ICMP_UNREACH_NET_UNKNOWN:
		case ICMP_UNREACH_HOST_UNKNOWN:
		case ICMP_UNREACH_ISOLATED:
		case ICMP_UNREACH_TOSNET:
		case ICMP_UNREACH_TOSHOST:
		case ICMP_UNREACH_HOST_PRECEDENCE:
		case ICMP_UNREACH_PRECEDENCE_CUTOFF:
			code = PRC_UNREACH_NET;
			break;

		case ICMP_UNREACH_NEEDFRAG:
			code = PRC_MSGSIZE;
			break;

		  
                                            
                                        
     
		case ICMP_UNREACH_PROTOCOL:
		case ICMP_UNREACH_PORT:
			code = PRC_UNREACH_PORT;
			break;

		case ICMP_UNREACH_NET_PROHIB:
		case ICMP_UNREACH_HOST_PROHIB:
		case ICMP_UNREACH_FILTER_PROHIB:
			code = PRC_UNREACH_ADMIN_PROHIB;
			break;

		default:
			goto badcode;
		}
		goto deliver;

	case ICMP_TIMXCEED:
		if (code > 1) {
			goto badcode;
		}
		code += PRC_TIMXCEED_INTRANS;
		goto deliver;

	case ICMP_PARAMPROB:
		if (code > 1) {
			goto badcode;
		}
		code = PRC_PARAMPROB;
		goto deliver;

	case ICMP_SOURCEQUENCH:
		if (code) {
			goto badcode;
		}
		code = PRC_QUENCH;
deliver:
		  
                                                         
     
		if (icmplen < ICMP_ADVLENMIN || icmplen < ICMP_ADVLEN(icp)
		    || IP_VHL_HL(icp->icmp_ip.ip_vhl) <
		    (sizeof(struct ip) >> 2) ||
		    (m = m_pullup(m, hlen + ICMP_ADVLEN(icp))) == NULL) {
			icmpstat.icps_badlen++;
			goto freeit;
		}

		ip = mtod(m, struct ip *);
		icp = (struct icmp *)(void *)(mtod(m, uint8_t *) + hlen);

#if BYTE_ORDER != BIG_ENDIAN
		NTOHS(icp->icmp_ip.ip_len);
#endif

		                                                     
		if (IN_MULTICAST(ntohl(icp->icmp_ip.ip_dst.s_addr))) {
			goto badcode;
		}
#if (DEBUG | DEVELOPMENT)
		if (icmpprintfs > 2) {
			printf("deliver to protocol %d\n",
			    icp->icmp_ip.ip_p);
		}
#endif
		icmpsrc.sin_addr = icp->icmp_ip.ip_dst;

		  
                                                          
                               
     
		ctlfunc = ip_protox[icp->icmp_ip.ip_p]->pr_ctlinput;

		if (ctlfunc) {
			LCK_MTX_ASSERT(inet_domain_mutex, LCK_MTX_ASSERT_OWNED);

			lck_mtx_unlock(inet_domain_mutex);

			(*ctlfunc)(code, (struct sockaddr *)&icmpsrc,
			    (void *)&icp->icmp_ip, m->m_pkthdr.rcvif);

			lck_mtx_lock(inet_domain_mutex);
		}
		break;

badcode:
		icmpstat.icps_badcode++;
		break;

	case ICMP_ECHO:
		if ((m->m_flags & (M_MCAST | M_BCAST))) {
			if (icmpbmcastecho == 0) {
				icmpstat.icps_bmcastecho++;
				break;
			}
		}

		  
                    
                                                                           
                                                           
     
		if ((IN_LOCAL_GROUP(ntohl(ip->ip_dst.s_addr)) ||
		    in_broadcast(ip->ip_dst, m->m_pkthdr.rcvif)) &&
		    in_localaddr(ip->ip_src) == 0) {
			icmpstat.icps_bmcastecho++;
#if (DEBUG | DEVELOPMENT)
			if (icmpprintfs > 0) {
				char src_str[MAX_IPv4_STR_LEN];
				char dst_str[MAX_IPv4_STR_LEN];

				inet_ntop(AF_INET, &ip->ip_src, src_str, sizeof(src_str));
				inet_ntop(AF_INET, &ip->ip_dst, dst_str, sizeof(dst_str));
				printf("%s: non local (B|M)CAST %s to %s, len %d\n",
				    __func__, src_str, dst_str, icmplen);
			}
#endif
			break;
		}

		icp->icmp_type = ICMP_ECHOREPLY;
#if ICMP_BANDLIM
		if (badport_bandlim(BANDLIM_ICMP_ECHO)) {
			goto freeit;
		} else
#endif
		goto reflect;

	case ICMP_TSTAMP:
		if (icmptimestamp == 0) {
			break;
		}

		if (!icmpbmcastecho
		    && (m->m_flags & (M_MCAST | M_BCAST)) != 0) {
			icmpstat.icps_bmcasttstamp++;
			break;
		}
		if (icmplen < ICMP_TSLEN) {
			icmpstat.icps_badlen++;
			break;
		}
		icp->icmp_type = ICMP_TSTAMPREPLY;
		icp->icmp_rtime = iptime();
		icp->icmp_ttime = icp->icmp_rtime;                            
#if ICMP_BANDLIM
		if (badport_bandlim(BANDLIM_ICMP_TSTAMP)) {
			goto freeit;
		} else
#endif
		goto reflect;

	case ICMP_MASKREQ:
		if (icmpmaskrepl == 0) {
			break;
		}
		  
                                                       
                                                          
     
		if (icmplen < ICMP_MASKLEN) {
			break;
		}
		switch (ip->ip_dst.s_addr) {
		case INADDR_BROADCAST:
		case INADDR_ANY:
			icmpdst.sin_addr = ip->ip_src;
			break;

		default:
			icmpdst.sin_addr = ip->ip_dst;
		}
		ia = (struct in_ifaddr *)ifaof_ifpforaddr(
			(struct sockaddr *)&icmpdst, m->m_pkthdr.rcvif);
		if (ia == 0) {
			break;
		}
		IFA_LOCK(&ia->ia_ifa);
		if (ia->ia_ifp == 0) {
			IFA_UNLOCK(&ia->ia_ifa);
			IFA_REMREF(&ia->ia_ifa);
			ia = NULL;
			break;
		}
		icp->icmp_type = ICMP_MASKREPLY;
		icp->icmp_mask = ia->ia_sockmask.sin_addr.s_addr;
		if (ip->ip_src.s_addr == 0) {
			if (ia->ia_ifp->if_flags & IFF_BROADCAST) {
				ip->ip_src = satosin(&ia->ia_broadaddr)->sin_addr;
			} else if (ia->ia_ifp->if_flags & IFF_POINTOPOINT) {
				ip->ip_src = satosin(&ia->ia_dstaddr)->sin_addr;
			}
		}
		IFA_UNLOCK(&ia->ia_ifa);
		IFA_REMREF(&ia->ia_ifa);
reflect:
		ip->ip_len += hlen;                                      
		icmpstat.icps_reflect++;
		icmpstat.icps_outhist[icp->icmp_type]++;
		icmp_reflect(m);
		return;

	case ICMP_REDIRECT:
		if (drop_redirect) {
			break;
		}
		if (code > 3) {
			goto badcode;
		}
		if (icmplen < ICMP_ADVLENMIN || icmplen < ICMP_ADVLEN(icp) ||
		    IP_VHL_HL(icp->icmp_ip.ip_vhl) < (sizeof(struct ip) >> 2)) {
			icmpstat.icps_badlen++;
			break;
		}

#if (DEBUG | DEVELOPMENT)
		should_log_redirect = log_redirect || (icmpprintfs > 0);
#else
		should_log_redirect = log_redirect;
#endif
		  
                                             
                                             
                                                  
                                                
                                            
     
		icmpgw.sin_addr = ip->ip_src;
		icmpdst.sin_addr = icp->icmp_gwaddr;

		if (should_log_redirect) {
			char src_str[MAX_IPv4_STR_LEN];
			char dst_str[MAX_IPv4_STR_LEN];
			char gw_str[MAX_IPv4_STR_LEN];

			inet_ntop(AF_INET, &ip->ip_src, src_str, sizeof(src_str));
			inet_ntop(AF_INET, &icp->icmp_ip.ip_dst, dst_str, sizeof(dst_str));
			inet_ntop(AF_INET, &icp->icmp_gwaddr, gw_str, sizeof(gw_str));
			printf("%s: redirect dst %s to %s from %s\n", __func__,
			    dst_str, gw_str, src_str);
		}
		icmpsrc.sin_addr = icp->icmp_ip.ip_dst;
		rtredirect(m->m_pkthdr.rcvif, (struct sockaddr *)&icmpsrc,
		    (struct sockaddr *)&icmpdst, NULL, RTF_GATEWAY | RTF_HOST,
		    (struct sockaddr *)&icmpgw, NULL);
		pfctlinput(PRC_REDIRECT_HOST, (struct sockaddr *)&icmpsrc);
#if IPSEC
		key_sa_routechange((struct sockaddr *)&icmpsrc);
#endif
		break;

	  
                                           
                                              
    
	case ICMP_ECHOREPLY:
	case ICMP_ROUTERADVERT:
	case ICMP_ROUTERSOLICIT:
	case ICMP_TSTAMPREPLY:
	case ICMP_IREQREPLY:
	case ICMP_MASKREPLY:
	default:
		break;
	}

raw:
	rip_input(m, hlen);
	return;

freeit:
	m_freem(m);
}

  
                                           
   
static void
icmp_reflect(struct mbuf *m)
{
	struct ip *ip = mtod(m, struct ip *);
	struct sockaddr_in icmpdst;
	struct in_ifaddr *ia;
	struct in_addr t;
	struct mbuf *opts = NULL;
	int optlen = (IP_VHL_HL(ip->ip_vhl) << 2) - sizeof(struct ip);

	if (!in_canforward(ip->ip_src) &&
	    ((ntohl(ip->ip_src.s_addr) & IN_CLASSA_NET) !=
	    (IN_LOOPBACKNET << IN_CLASSA_NSHIFT))) {
		m_freem(m);                             
		goto done;                                                
	}
	t = ip->ip_dst;
	ip->ip_dst = ip->ip_src;
	  
                                                        
                                                           
                                                    
                              
    
	lck_rw_lock_shared(in_ifaddr_rwlock);
	TAILQ_FOREACH(ia, INADDR_HASH(t.s_addr), ia_hash) {
		IFA_LOCK(&ia->ia_ifa);
		if (t.s_addr == IA_SIN(ia)->sin_addr.s_addr) {
			IFA_ADDREF_LOCKED(&ia->ia_ifa);
			IFA_UNLOCK(&ia->ia_ifa);
			goto match;
		}
		IFA_UNLOCK(&ia->ia_ifa);
	}
	  
                                                            
                                                             
                                                      
    
	for (ia = in_ifaddrhead.tqh_first; ia; ia = ia->ia_link.tqe_next) {
		IFA_LOCK(&ia->ia_ifa);
		if (ia->ia_ifp && (ia->ia_ifp->if_flags & IFF_BROADCAST) &&
		    t.s_addr == satosin(&ia->ia_broadaddr)->sin_addr.s_addr) {
			IFA_ADDREF_LOCKED(&ia->ia_ifa);
			IFA_UNLOCK(&ia->ia_ifa);
			break;
		}
		IFA_UNLOCK(&ia->ia_ifa);
	}
match:
	lck_rw_done(in_ifaddr_rwlock);

	                
	bzero(&icmpdst, sizeof(icmpdst));
	icmpdst.sin_len = sizeof(struct sockaddr_in);
	icmpdst.sin_family = AF_INET;
	icmpdst.sin_addr = t;
	if ((ia == (struct in_ifaddr *)0) && m->m_pkthdr.rcvif) {
		ia = (struct in_ifaddr *)ifaof_ifpforaddr(
			(struct sockaddr *)&icmpdst, m->m_pkthdr.rcvif);
	}
	  
                                                                
                                                        
    
	if (ia == (struct in_ifaddr *)0) {
		lck_rw_lock_shared(in_ifaddr_rwlock);
		ia = in_ifaddrhead.tqh_first;
		if (ia == (struct in_ifaddr *)0) {                              
			lck_rw_done(in_ifaddr_rwlock);
			m_freem(m);
			goto done;
		}
		IFA_ADDREF(&ia->ia_ifa);
		lck_rw_done(in_ifaddr_rwlock);
	}
	IFA_LOCK_SPIN(&ia->ia_ifa);
	t = IA_SIN(ia)->sin_addr;
	IFA_UNLOCK(&ia->ia_ifa);
	ip->ip_src = t;
	ip->ip_ttl = (u_char)ip_defttl;
	IFA_REMREF(&ia->ia_ifa);
	ia = NULL;

	if (optlen > 0) {
		u_char *cp;
		int opt, cnt;
		u_int len;

		  
                                                          
                                                  
     
		cp = (u_char *) (ip + 1);
		if ((opts = ip_srcroute()) == 0 &&
		    (opts = m_gethdr(M_DONTWAIT, MT_HEADER))) {             
			opts->m_len = sizeof(struct in_addr);
			mtod(opts, struct in_addr *)->s_addr = 0;
		}
		if (opts) {
#if (DEBUG | DEVELOPMENT)
			if (icmpprintfs > 1) {
				printf("icmp_reflect optlen %d rt %d => ",
				    optlen, opts->m_len);
			}
#endif
			for (cnt = optlen; cnt > 0; cnt -= len, cp += len) {
				opt = cp[IPOPT_OPTVAL];
				if (opt == IPOPT_EOL) {
					break;
				}
				if (opt == IPOPT_NOP) {
					len = 1;
				} else {
					if (cnt < IPOPT_OLEN + sizeof(*cp)) {
						break;
					}
					len = cp[IPOPT_OLEN];
					if (len < IPOPT_OLEN + sizeof(*cp) ||
					    len > cnt) {
						break;
					}
				}
				  
                                                       
       
				if (opt == IPOPT_RR || opt == IPOPT_TS ||
				    opt == IPOPT_SECURITY) {
					bcopy((caddr_t)cp,
					    mtod(opts, caddr_t) + opts->m_len, len);
					opts->m_len += len;
				}
			}
			                                   
			cnt = opts->m_len % 4;
			if (cnt) {
				for (; cnt < 4; cnt++) {
					*(mtod(opts, caddr_t) + opts->m_len) =
					    IPOPT_EOL;
					opts->m_len++;
				}
			}
#if (DEBUG | DEVELOPMENT)
			if (icmpprintfs > 1) {
				printf("%d\n", opts->m_len);
			}
#endif
		}
		  
                                                            
                                                
     
		ip->ip_len -= optlen;
		ip->ip_vhl = IP_VHL_BORING;
		m->m_len -= optlen;
		if (m->m_flags & M_PKTHDR) {
			m->m_pkthdr.len -= optlen;
		}
		optlen += sizeof(struct ip);
		bcopy((caddr_t)ip + optlen, (caddr_t)(ip + 1),
		    (unsigned)(m->m_len - sizeof(struct ip)));
	}
	m->m_flags &= ~(M_BCAST | M_MCAST);
	icmp_send(m, opts);
done:
	if (opts) {
		(void)m_free(opts);
	}
}

  
                                            
                              
   
static void
icmp_send(struct mbuf *m, struct mbuf *opts)
{
	struct ip *ip = mtod(m, struct ip *);
	int hlen;
	struct icmp *icp;
	struct route ro;
	struct ip_out_args ipoa;

	bzero(&ipoa, sizeof(ipoa));
	ipoa.ipoa_boundif = IFSCOPE_NONE;
	ipoa.ipoa_flags = IPOAF_SELECT_SRCIF | IPOAF_BOUND_SRCADDR;
	ipoa.ipoa_sotc = SO_TC_UNSPEC;
	ipoa.ipoa_netsvctype = _NET_SERVICE_TYPE_UNSPEC;

	if (!(m->m_pkthdr.pkt_flags & PKTF_LOOP) && m->m_pkthdr.rcvif != NULL) {
		ipoa.ipoa_boundif = m->m_pkthdr.rcvif->if_index;
		ipoa.ipoa_flags |= IPOAF_BOUND_IF;
	}

	hlen = IP_VHL_HL(ip->ip_vhl) << 2;
	m->m_data += hlen;
	m->m_len -= hlen;
	icp = mtod(m, struct icmp *);
	icp->icmp_cksum = 0;
	icp->icmp_cksum = in_cksum(m, ip->ip_len - hlen);
	m->m_data -= hlen;
	m->m_len += hlen;
	m->m_pkthdr.rcvif = NULL;
	m->m_pkthdr.csum_data = 0;
	m->m_pkthdr.csum_flags = 0;
#if (DEBUG | DEVELOPMENT)
	if (icmpprintfs > 2) {
		char src_str[MAX_IPv4_STR_LEN];
		char dst_str[MAX_IPv4_STR_LEN];

		inet_ntop(AF_INET, &ip->ip_src, src_str, sizeof(src_str));
		inet_ntop(AF_INET, &ip->ip_dst, dst_str, sizeof(dst_str));
		printf("%s: dst %s src %s\n", __func__, dst_str, src_str);
	}
#endif
	bzero(&ro, sizeof ro);
	(void) ip_output(m, opts, &ro, IP_OUTARGS, NULL, &ipoa);
	ROUTE_RELEASE(&ro);
}

u_int32_t
iptime(void)
{
	struct timeval atv;
	u_int32_t t;

	getmicrotime(&atv);
	t = (atv.tv_sec % (24 * 60 * 60)) * 1000 + atv.tv_usec / 1000;
	return htonl(t);
}

#if 1
  
                                                                      
                                                                       
                                                       
   
int
ip_next_mtu(int mtu, int dir)
{
	static int mtutab[] = {
		65535, 32000, 17914, 8166, 4352, 2002, 1492, 1006, 508, 296,
		68, 0
	};
	int i;

	for (i = 0; i < (sizeof mtutab) / (sizeof mtutab[0]); i++) {
		if (mtu >= mtutab[i]) {
			break;
		}
	}

	if (dir < 0) {
		if (i == 0) {
			return 0;
		} else {
			return mtutab[i - 1];
		}
	} else {
		if (mtutab[i] == 0) {
			return 0;
		} else if (mtu > mtutab[i]) {
			return mtutab[i];
		} else {
			return mtutab[i + 1];
		}
	}
}
#endif

#if ICMP_BANDLIM

  
                                                     
                                                                           
                 
  
                                                                      
                                                                      
                 
  
                                                                   
                                                                      
                                                                        
                                                                     
                                
   

boolean_t
badport_bandlim(int which)
{
	static uint64_t lticks[BANDLIM_MAX + 1];
	static int lpackets[BANDLIM_MAX + 1];
	uint64_t time;
	uint64_t secs;
	static boolean_t is_initialized = FALSE;
	static int icmplim_random;
	const char *bandlimittype[] = {
		"Limiting icmp unreach response",
		"Limiting icmp ping response",
		"Limiting icmp tstamp response",
		"Limiting closed port RST response",
		"Limiting open port RST response"
	};

	                                                                    

	if (icmplim <= 0 || which > BANDLIM_MAX || which < 0) {
		return false;
	}

	if (is_initialized == FALSE) {
		if (icmplim_random_incr > 0 &&
		    icmplim <= INT32_MAX - (icmplim_random_incr + 1)) {
			icmplim_random = icmplim + (random() % icmplim_random_incr) + 1;
		}
		is_initialized = TRUE;
	}

	time = net_uptime();
	secs = time - lticks[which];

	  
                                                         
    

	if (secs > 1) {
		if (lpackets[which] > icmplim_random) {
			printf("%s from %d to %d packets per second\n",
			    bandlimittype[which],
			    lpackets[which],
			    icmplim_random
			    );
		}
		lticks[which] = time;
		lpackets[which] = 0;
	}

	  
                     
    
	if (++lpackets[which] > icmplim_random) {
		  
                                                                 
                                              
                                                               
                                       
     
		if ((random() % (lpackets[which] - icmplim_random)) != 0) {
			return true;
		}
	}
	return false;
}

#endif

#if __APPLE__

  
                                        
                           
             
                           
   

#include <netinet/ip_icmp.h>
#include <netinet/in_pcb.h>

extern u_int32_t rip_sendspace;
extern u_int32_t rip_recvspace;
extern struct inpcbinfo ripcbinfo;

int rip_abort(struct socket *);
int rip_bind(struct socket *, struct sockaddr *, struct proc *);
int rip_connect(struct socket *, struct sockaddr *, struct proc *);
int rip_detach(struct socket *);
int rip_disconnect(struct socket *);
int rip_shutdown(struct socket *);

__private_extern__ int icmp_dgram_send(struct socket *so, int flags, struct mbuf *m, struct sockaddr *nam, struct mbuf *control, struct proc *p);
__private_extern__ int icmp_dgram_attach(struct socket *so, int proto, struct proc *p);
__private_extern__ int icmp_dgram_ctloutput(struct socket *so, struct sockopt *sopt);

__private_extern__ struct pr_usrreqs icmp_dgram_usrreqs = {
	.pru_abort =            rip_abort,
	.pru_attach =           icmp_dgram_attach,
	.pru_bind =             rip_bind,
	.pru_connect =          rip_connect,
	.pru_control =          in_control,
	.pru_detach =           rip_detach,
	.pru_disconnect =       rip_disconnect,
	.pru_peeraddr =         in_getpeeraddr,
	.pru_send =             icmp_dgram_send,
	.pru_shutdown =         rip_shutdown,
	.pru_sockaddr =         in_getsockaddr,
	.pru_sosend =           sosend,
	.pru_soreceive =        soreceive,
};

                                                            
__private_extern__ int
icmp_dgram_attach(struct socket *so, __unused int proto, struct proc *p)
{
	struct inpcb *inp;
	int error;

	inp = sotoinpcb(so);
	if (inp) {
		panic("icmp_dgram_attach");
	}

	error = soreserve(so, rip_sendspace, rip_recvspace);
	if (error) {
		return error;
	}
	error = in_pcballoc(so, &ripcbinfo, p);
	if (error) {
		return error;
	}
	inp = (struct inpcb *)so->so_pcb;
	inp->inp_vflag |= INP_IPV4;
	inp->inp_ip_p = IPPROTO_ICMP;
	inp->inp_ip_ttl = (u_char)ip_defttl;
	return 0;
}

  
                                   
   
__private_extern__ int
icmp_dgram_ctloutput(struct socket *so, struct sockopt *sopt)
{
	int     error;

	if (sopt->sopt_level != IPPROTO_IP) {
		return EINVAL;
	}

	switch (sopt->sopt_name) {
	case IP_OPTIONS:
	case IP_HDRINCL:
	case IP_TOS:
	case IP_TTL:
	case IP_RECVOPTS:
	case IP_RECVRETOPTS:
	case IP_RECVDSTADDR:
	case IP_RETOPTS:
	case IP_MULTICAST_IF:
	case IP_MULTICAST_IFINDEX:
	case IP_MULTICAST_TTL:
	case IP_MULTICAST_LOOP:
	case IP_ADD_MEMBERSHIP:
	case IP_DROP_MEMBERSHIP:
	case IP_MULTICAST_VIF:
	case IP_PORTRANGE:
	case IP_RECVIF:
	case IP_IPSEC_POLICY:
	case IP_STRIPHDR:
	case IP_RECVTTL:
	case IP_BOUND_IF:
	case IP_DONTFRAG:
	case IP_NO_IFT_CELLULAR:
		error = rip_ctloutput(so, sopt);
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

__private_extern__ int
icmp_dgram_send(struct socket *so, int flags, struct mbuf *m,
    struct sockaddr *nam, struct mbuf *control, struct proc *p)
{
	struct ip *ip;
	struct inpcb *inp = sotoinpcb(so);
	int hlen;
	struct icmp *icp;
	struct in_ifaddr *ia = NULL;
	int icmplen;
	int error = EINVAL;
	int inp_flags = inp ? inp->inp_flags : 0;

	if (inp == NULL
#if NECP
	    || (necp_socket_should_use_flow_divert(inp))
#endif           
	    ) {
		if (inp != NULL) {
			error = EPROTOTYPE;
		}
		goto bad;
	}

#if CONTENT_FILTER
	  
                                                                          
    
	if (so->so_cfil_db && nam == NULL) {
		cfil_dgram_peek_socket_state(m, &inp_flags);
	}
#endif

	if ((inp_flags & INP_HDRINCL) != 0) {
		                                                              
		MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(m);
		  
                                                        
                
     
		ip = mtod(m, struct ip *);

		hlen = IP_VHL_HL(ip->ip_vhl) << 2;
		                        
		if (m->m_pkthdr.len < hlen + ICMP_MINLEN) {
			goto bad;
		}
		               
		if (IP_VHL_V(ip->ip_vhl) != 4) {
			goto bad;
		}
		if (hlen < 20 || hlen > 40 || ip->ip_len != m->m_pkthdr.len) {
			goto bad;
		}
		                                               
		if ((ip->ip_off & ~IP_DF) != 0) {
			goto bad;
		}
		                                                      
		if (ip->ip_p != IPPROTO_ICMP) {
			goto bad;
		}
		  
                                                       
                    
     
		if (ip->ip_src.s_addr != INADDR_ANY) {
			socket_unlock(so, 0);
			lck_rw_lock_shared(in_ifaddr_rwlock);
			if (TAILQ_EMPTY(&in_ifaddrhead)) {
				lck_rw_done(in_ifaddr_rwlock);
				socket_lock(so, 0);
				goto bad;
			}
			TAILQ_FOREACH(ia, INADDR_HASH(ip->ip_src.s_addr),
			    ia_hash) {
				IFA_LOCK(&ia->ia_ifa);
				if (IA_SIN(ia)->sin_addr.s_addr ==
				    ip->ip_src.s_addr) {
					IFA_UNLOCK(&ia->ia_ifa);
					lck_rw_done(in_ifaddr_rwlock);
					socket_lock(so, 0);
					goto ours;
				}
				IFA_UNLOCK(&ia->ia_ifa);
			}
			lck_rw_done(in_ifaddr_rwlock);
			socket_lock(so, 0);
			goto bad;
		}
ours:
		                                          
		ip->ip_sum = 0;

		icp = (struct icmp *)(void *)(((char *)m->m_data) + hlen);
		icmplen = m->m_pkthdr.len - hlen;
	} else {
		if ((icmplen = m->m_pkthdr.len) < ICMP_MINLEN) {
			goto bad;
		}
		icp = mtod(m, struct icmp *);
	}
	  
                                                
    
	if (icp->icmp_code != 0) {
		goto bad;
	}
	switch (icp->icmp_type) {
	case ICMP_ECHO:
		break;
	case ICMP_TSTAMP:
		if (icmplen != 20) {
			goto bad;
		}
		break;
	case ICMP_MASKREQ:
		if (icmplen != 12) {
			goto bad;
		}
		break;
	default:
		goto bad;
	}
	return rip_send(so, flags, m, nam, control, p);
bad:
	VERIFY(error != 0);

	if (m != NULL) {
		m_freem(m);
	}
	if (control != NULL) {
		m_freem(control);
	}

	return error;
}

#endif                
