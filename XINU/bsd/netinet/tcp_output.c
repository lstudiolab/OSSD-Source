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
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1995
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
 *	@(#)tcp_output.c	8.4 (Berkeley) 5/24/95
 * $FreeBSD: src/sys/netinet/tcp_output.c,v 1.39.2.10 2001/07/07 04:30:38 silby Exp $
 */
  
                                                                      
                                                                          
                                                                        
               
   

#define _IP_VHL


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>

#include <net/route.h>
#include <net/ntstat.h>
#include <net/if_var.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/dlil.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/in_tclass.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <mach/sdt.h>
#include <netinet6/in6_pcb.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/tcp.h>
#define TCPOUTFLAGS
#include <netinet/tcp_cache.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_cc.h>
#if TCPDEBUG
#include <netinet/tcp_debug.h>
#endif
#include <netinet/tcp_log.h>
#include <sys/kdebug.h>
#include <mach/sdt.h>

#if IPSEC
#include <netinet6/ipsec.h>
#endif          

#if MPTCP
#include <netinet/mptcp_var.h>
#include <netinet/mptcp.h>
#include <netinet/mptcp_opt.h>
#include <netinet/mptcp_seq.h>
#endif

#include <corecrypto/ccaes.h>

#define DBG_LAYER_BEG           NETDBG_CODE(DBG_NETTCP, 1)
#define DBG_LAYER_END           NETDBG_CODE(DBG_NETTCP, 3)
#define DBG_FNC_TCP_OUTPUT      NETDBG_CODE(DBG_NETTCP, (4 << 8) | 1)

SYSCTL_SKMEM_TCP_INT(OID_AUTO, path_mtu_discovery,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, path_mtu_discovery, 1,
    "Enable Path MTU Discovery");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, local_slowstart_flightsize,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, ss_fltsz_local, 8,
    "Slow start flight size for local networks");

int     tcp_do_tso = 1;
SYSCTL_INT(_net_inet_tcp, OID_AUTO, tso, CTLFLAG_RW | CTLFLAG_LOCKED,
    &tcp_do_tso, 0, "Enable TCP Segmentation Offload");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, ecn_setup_percentage,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_ecn_setup_percentage, 100,
    "Max ECN setup percentage");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, do_ack_compression,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_do_ack_compression, 1,
    "Enable TCP ACK compression (on (cell only): 1, off: 0, on (all interfaces): 2)");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, ack_compression_rate,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_ack_compression_rate, TCP_COMP_CHANGE_RATE,
    "Rate at which we force sending new ACKs (in ms)");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, randomize_timestamps,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_randomize_timestamps, 1,
    "Randomize TCP timestamps to prevent tracking (on: 1, off: 0)");

static int
sysctl_change_ecn_setting SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	int i, err = 0, changed = 0;
	struct ifnet *ifp;

	err = sysctl_io_number(req, tcp_ecn_outbound, sizeof(int32_t),
	    &i, &changed);
	if (err != 0 || req->newptr == USER_ADDR_NULL) {
		return err;
	}

	if (changed) {
		if ((tcp_ecn_outbound == 0 || tcp_ecn_outbound == 1) &&
		    (i == 0 || i == 1)) {
			tcp_ecn_outbound = i;
			SYSCTL_SKMEM_UPDATE_FIELD(tcp.ecn_initiate_out, tcp_ecn_outbound);
			return err;
		}
		if (tcp_ecn_outbound == 2 && (i == 0 || i == 1)) {
			  
                                            
                                                     
          
      
			ifnet_head_lock_shared();
			TAILQ_FOREACH(ifp, &ifnet_head, if_link) {
				if (!IFNET_IS_CELLULAR(ifp)) {
					if_clear_eflags(ifp,
					    IFEF_ECN_ENABLE |
					    IFEF_ECN_DISABLE);
				}
			}
			ifnet_head_done();
		} else {
			  
                                          
                
      
			ifnet_head_lock_shared();
			TAILQ_FOREACH(ifp, &ifnet_head, if_link) {
				if (!IFNET_IS_CELLULAR(ifp)) {
					if_set_eflags(ifp, IFEF_ECN_ENABLE);
					if_clear_eflags(ifp, IFEF_ECN_DISABLE);
				}
			}
			ifnet_head_done();
		}
		tcp_ecn_outbound = i;
		SYSCTL_SKMEM_UPDATE_FIELD(tcp.ecn_initiate_out, tcp_ecn_outbound);
	}
	                                                  
	if (i == 2 || tcp_ecn_inbound == 2) {
		tcp_ecn_inbound = i;
		SYSCTL_SKMEM_UPDATE_FIELD(tcp.ecn_negotiate_in, tcp_ecn_inbound);
	}
	return err;
}

int     tcp_ecn_outbound = 2;
SYSCTL_PROC(_net_inet_tcp, OID_AUTO, ecn_initiate_out,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &tcp_ecn_outbound, 0,
    sysctl_change_ecn_setting, "IU",
    "Initiate ECN for outbound connections");

int     tcp_ecn_inbound = 2;
SYSCTL_PROC(_net_inet_tcp, OID_AUTO, ecn_negotiate_in,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED, &tcp_ecn_inbound, 0,
    sysctl_change_ecn_setting, "IU",
    "Initiate ECN for inbound connections");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, packetchain,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_packet_chaining, 50,
    "Enable TCP output packet chaining");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, socket_unlocked_on_output,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_output_unlocked, 1,
    "Unlock TCP when sending packets down to IP");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, min_iaj_win,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_min_iaj_win, MIN_IAJ_WIN,
    "Minimum recv win based on inter-packet arrival jitter");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, acc_iaj_react_limit,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_acc_iaj_react_limit,
    ACC_IAJ_REACT_LIMIT, "Accumulated IAJ when receiver starts to react");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, autosndbufinc,
    CTLFLAG_RW | CTLFLAG_LOCKED, uint32_t, tcp_autosndbuf_inc,
    8 * 1024, "Increment in send socket bufffer size");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, autosndbufmax,
    CTLFLAG_RW | CTLFLAG_LOCKED, uint32_t, tcp_autosndbuf_max, 2 * 1024 * 1024,
    "Maximum send socket buffer size");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, rtt_recvbg,
    CTLFLAG_RW | CTLFLAG_LOCKED, uint32_t, tcp_use_rtt_recvbg, 1,
    "Use RTT for bg recv algorithm");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, recv_throttle_minwin,
    CTLFLAG_RW | CTLFLAG_LOCKED, uint32_t, tcp_recv_throttle_minwin, 16 * 1024,
    "Minimum recv win for throttling");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, enable_tlp,
    CTLFLAG_RW | CTLFLAG_LOCKED,
    int32_t, tcp_enable_tlp, 1, "Enable Tail loss probe");

static int32_t packchain_newlist = 0;
static int32_t packchain_looped = 0;
static int32_t packchain_sent = 0;

                            
#if IPSEC
extern int ipsec_bypass;
#endif

extern int slowlink_wsize;                                            

extern u_int32_t dlil_filter_disable_tso_count;
extern u_int32_t kipf_count;

static int tcp_ip_output(struct socket *, struct tcpcb *, struct mbuf *,
    int, struct mbuf *, int, int, boolean_t);
static int tcp_recv_throttle(struct tcpcb *tp);

static int32_t
tcp_tfo_check(struct tcpcb *tp, int32_t len)
{
	struct socket *so = tp->t_inpcb->inp_socket;
	unsigned int optlen = 0;
	unsigned int cookie_len;

	if (tp->t_flags & TF_NOOPT) {
		goto fallback;
	}

	if (!(tp->t_flagsext & TF_FASTOPEN_FORCE_ENABLE) &&
	    !tcp_heuristic_do_tfo(tp)) {
		tp->t_tfo_stats |= TFO_S_HEURISTICS_DISABLE;
		tcpstat.tcps_tfo_heuristics_disable++;
		goto fallback;
	}

	if (so->so_flags1 & SOF1_DATA_AUTHENTICATED) {
		return len;
	}

	optlen += TCPOLEN_MAXSEG;

	if (tp->t_flags & TF_REQ_SCALE) {
		optlen += 4;
	}

#if MPTCP
	if ((so->so_flags & SOF_MP_SUBFLOW) && mptcp_enable &&
	    (tp->t_rxtshift <= mptcp_mpcap_retries ||
	    (tptomptp(tp)->mpt_mpte->mpte_flags & MPTE_FORCE_ENABLE))) {
		optlen += sizeof(struct mptcp_mpcapable_opt_common) + sizeof(mptcp_key_t);
	}
#endif            

	if (tp->t_flags & TF_REQ_TSTMP) {
		optlen += TCPOLEN_TSTAMP_APPA;
	}

	if (SACK_ENABLED(tp)) {
		optlen += TCPOLEN_SACK_PERMITTED;
	}

	                                           

	                                                             
	if (MAX_TCPOPTLEN - optlen < TCPOLEN_FASTOPEN_REQ) {
		goto fallback;
	}

	cookie_len = tcp_cache_get_cookie_len(tp);
	if (cookie_len == 0) {
		                                  
		return 0;
	}

	                                                                   
	if (MAX_TCPOPTLEN - optlen < cookie_len) {
		goto fallback;
	}

	                                                                 
	if (so->so_snd.sb_cc > (tp->t_maxopd - MAX_TCPOPTLEN)) {
		goto fallback;
	}

	                                                        
	return len;

fallback:
	tcp_disable_tfo(tp);
	return 0;
}

                                                                 
static unsigned int
tcp_tfo_write_cookie_rep(struct tcpcb *tp, unsigned int optlen, u_char *opt)
{
	u_char out[CCAES_BLOCK_SIZE];
	unsigned ret = 0;
	u_char *bp;

	if ((MAX_TCPOPTLEN - optlen) <
	    (TCPOLEN_FASTOPEN_REQ + TFO_COOKIE_LEN_DEFAULT)) {
		return ret;
	}

	tcp_tfo_gen_cookie(tp->t_inpcb, out, sizeof(out));

	bp = opt + optlen;

	*bp++ = TCPOPT_FASTOPEN;
	*bp++ = 2 + TFO_COOKIE_LEN_DEFAULT;
	memcpy(bp, out, TFO_COOKIE_LEN_DEFAULT);
	ret += 2 + TFO_COOKIE_LEN_DEFAULT;

	tp->t_tfo_stats |= TFO_S_COOKIE_SENT;
	tcpstat.tcps_tfo_cookie_sent++;

	return ret;
}

static unsigned int
tcp_tfo_write_cookie(struct tcpcb *tp, unsigned int optlen, int32_t len,
    u_char *opt)
{
	uint8_t tfo_len;
	struct socket *so = tp->t_inpcb->inp_socket;
	unsigned ret = 0;
	int res;
	u_char *bp;

	if (TCPOLEN_FASTOPEN_REQ > MAX_TCPOPTLEN - optlen) {
		return 0;
	}
	tfo_len = (uint8_t)(MAX_TCPOPTLEN - optlen - TCPOLEN_FASTOPEN_REQ);

	if (so->so_flags1 & SOF1_DATA_AUTHENTICATED) {
		                                           
		if (len > 0) {
			tp->t_tfo_stats |= TFO_S_SYN_DATA_SENT;
			tcpstat.tcps_tfo_syn_data_sent++;
		}

		return 0;
	}

	bp = opt + optlen;

	  
                                                                 
                                                                    
             
    
	res = tcp_cache_get_cookie(tp, bp + TCPOLEN_FASTOPEN_REQ, &tfo_len);
	if (res == 0) {
		*bp++ = TCPOPT_FASTOPEN;
		*bp++ = TCPOLEN_FASTOPEN_REQ;
		ret += TCPOLEN_FASTOPEN_REQ;

		tp->t_tfo_flags |= TFO_F_COOKIE_REQ;

		tp->t_tfo_stats |= TFO_S_COOKIE_REQ;
		tcpstat.tcps_tfo_cookie_req++;
	} else {
		*bp++ = TCPOPT_FASTOPEN;
		*bp++ = TCPOLEN_FASTOPEN_REQ + tfo_len;

		ret += TCPOLEN_FASTOPEN_REQ + tfo_len;

		tp->t_tfo_flags |= TFO_F_COOKIE_SENT;

		                                           
		if (len > 0) {
			tp->t_tfo_stats |= TFO_S_SYN_DATA_SENT;
			tcpstat.tcps_tfo_syn_data_sent++;
		}
	}

	return ret;
}

static inline bool
tcp_send_ecn_flags_on_syn(struct tcpcb *tp)
{
	return !(tp->ecn_flags & TE_SETUPSENT);
}

void
tcp_set_ecn(struct tcpcb *tp, struct ifnet *ifp)
{
	boolean_t inbound;

	  
                                
    
	if (tp->ecn_flags & TE_ECN_MODE_ENABLE) {
		tp->ecn_flags |= TE_ENABLE_ECN;
		goto check_heuristic;
	}

	if (tp->ecn_flags & TE_ECN_MODE_DISABLE) {
		tp->ecn_flags &= ~TE_ENABLE_ECN;
		return;
	}
	  
                                    
    
	if (ifp != NULL) {
		if (ifp->if_eflags & IFEF_ECN_ENABLE) {
			tp->ecn_flags |= TE_ENABLE_ECN;
			goto check_heuristic;
		}

		if (ifp->if_eflags & IFEF_ECN_DISABLE) {
			tp->ecn_flags &= ~TE_ENABLE_ECN;
			return;
		}
	}
	  
                                  
    
	inbound = (tp->t_inpcb->inp_socket->so_head != NULL);
	if ((inbound && tcp_ecn_inbound == 1) ||
	    (!inbound && tcp_ecn_outbound == 1)) {
		tp->ecn_flags |= TE_ENABLE_ECN;
		goto check_heuristic;
	} else {
		tp->ecn_flags &= ~TE_ENABLE_ECN;
	}

	return;

check_heuristic:
	if (!tcp_heuristic_do_ecn(tp)) {
		tp->ecn_flags &= ~TE_ENABLE_ECN;
	}

	  
                                                                 
                                                             
             
    
	if ((tp->ecn_flags & (TE_ECN_MODE_ENABLE | TE_ECN_MODE_DISABLE
	    | TE_ENABLE_ECN)) == TE_ENABLE_ECN) {
		  
                                                
                   
     
		if ((tp->iss % 100) >= tcp_ecn_setup_percentage) {
			tp->ecn_flags &= ~TE_ENABLE_ECN;
		}
	}
}

int
tcp_flight_size(struct tcpcb *tp)
{
	int ret;

	VERIFY(tp->sackhint.sack_bytes_acked >= 0);
	VERIFY(tp->sackhint.sack_bytes_rexmit >= 0);

	  
                                                                                      
    
	ret = tp->snd_nxt - tp->snd_una - tp->sackhint.sack_bytes_acked;

	if (ret < 0) {
		  
                                                                            
                                                                       
                      
     
		ret = 0;
	}

	return ret;
}

  
                                                                  
  
                       
                 
           
            
                
            
                        
                               
                             
                              
                        
                          
                         
                                                              
                         
                             
                               
                                
                              
                           
                          
                                                               
   
int
tcp_output(struct tcpcb *tp)
{
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
	int32_t len, recwin, sendwin, off;
	uint8_t flags;
	int error;
	struct mbuf *m;
	struct ip *ip = NULL;
	struct ip6_hdr *ip6 = NULL;
	struct tcphdr *th;
	u_char opt[TCP_MAXOLEN];
	unsigned int ipoptlen, optlen, hdrlen;
	int idle, sendalot, lost = 0;
	int i, sack_rxmit;
	int tso = 0;
	int sack_bytes_rxmt;
	tcp_seq old_snd_nxt = 0;
	struct sackhole *p;
#if IPSEC
	unsigned int ipsec_optlen = 0;
#endif            
	int    idle_time = 0;
	struct mbuf *packetlist = NULL;
	struct mbuf *tp_inp_options = inp->inp_depend4.inp4_options;
	int isipv6 = inp->inp_vflag & INP_IPV6;
	int packchain_listadd = 0;
	int so_options = so->so_options;
	struct rtentry *rt;
	u_int32_t svc_flags = 0, allocated_len;
	unsigned int sackoptlen = 0;
#if MPTCP
	boolean_t mptcp_acknow;
#endif            
	boolean_t cell = FALSE;
	boolean_t wifi = FALSE;
	boolean_t wired = FALSE;
	boolean_t sack_rescue_rxt = FALSE;
	int sotc = so->so_traffic_class;
	boolean_t do_not_compress = FALSE;
	boolean_t sack_rxmted = FALSE;

	  
                                                        
                                
                                                         
                                                           
    
	idle = (tp->t_flags & TF_LASTIDLE) || (tp->snd_max == tp->snd_una);

	                                                                       
                                            
    
	idle_time = tcp_now - tp->t_rcvtime;
	if (idle && idle_time >= TCP_IDLETIMEOUT(tp)) {
		if (CC_ALGO(tp)->after_idle != NULL &&
		    (tp->tcp_cc_index != TCP_CC_ALGO_CUBIC_INDEX ||
		    idle_time >= TCP_CC_CWND_NONVALIDATED_PERIOD)) {
			CC_ALGO(tp)->after_idle(tp);
			tcp_ccdbg_trace(tp, NULL, TCP_CC_IDLE_TIMEOUT);
		}

		  
                                                   
              
     
		if (!SLIST_EMPTY(&tp->t_rxt_segments)) {
			tcp_rxtseg_clean(tp);
		}

		                                                      
		tcp_cc_after_idle_stretchack(tp);
		tp->t_forced_acks = TCP_FORCED_ACKS_COUNT;
	}
	tp->t_flags &= ~TF_LASTIDLE;
	if (idle) {
		if (tp->t_flags & TF_MORETOCOME) {
			tp->t_flags |= TF_LASTIDLE;
			idle = 0;
		}
	}
#if MPTCP
	if (tp->t_mpflags & TMPF_RESET) {
		tcp_check_timer_state(tp);
		  
                                                   
                                                   
                                                    
     
		return 0;
	}
#endif            

again:
#if MPTCP
	mptcp_acknow = FALSE;
#endif
	do_not_compress = FALSE;

	KERNEL_DEBUG(DBG_FNC_TCP_OUTPUT | DBG_FUNC_START, 0, 0, 0, 0, 0);

	if (isipv6) {
		KERNEL_DEBUG(DBG_LAYER_BEG,
		    ((inp->inp_fport << 16) | inp->inp_lport),
		    (((inp->in6p_laddr.s6_addr16[0] & 0xffff) << 16) |
		    (inp->in6p_faddr.s6_addr16[0] & 0xffff)),
		    sendalot, 0, 0);
	} else {
		KERNEL_DEBUG(DBG_LAYER_BEG,
		    ((inp->inp_fport << 16) | inp->inp_lport),
		    (((inp->inp_laddr.s_addr & 0xffff) << 16) |
		    (inp->inp_faddr.s_addr & 0xffff)),
		    sendalot, 0, 0);
	}
	  
                                                                 
                                                                
                                                                  
                                                   
    
	rt = inp->inp_route.ro_rt;
	if (rt != NULL && ROUTE_UNUSABLE(&tp->t_inpcb->inp_route)) {
		struct ifnet *ifp;
		struct in_ifaddr *ia = NULL;
		struct in6_ifaddr *ia6 = NULL;
		int found_srcaddr = 0;

		                                      
		somultipages(so, FALSE);

		                                                   
		tp->t_flags &= ~TF_TSO;

		soif2kcl(so, FALSE);

		if (isipv6) {
			ia6 = ifa_foraddr6(&inp->in6p_laddr);
			if (ia6 != NULL) {
				found_srcaddr = 1;
			}
		} else {
			ia = ifa_foraddr(inp->inp_laddr.s_addr);
			if (ia != NULL) {
				found_srcaddr = 1;
			}
		}

		                                                  
		if (found_srcaddr == 0) {
			soevent(so,
			    (SO_FILT_HINT_LOCKED | SO_FILT_HINT_NOSRCADDR));

			if (tp->t_state >= TCPS_CLOSE_WAIT) {
				tcp_drop(tp, EADDRNOTAVAIL);
				return EADDRNOTAVAIL;
			}

			  
                                             
                                                   
                                                     
      
			if (tp->t_timer[TCPT_REXMT] == 0) {
				tp->t_timer[TCPT_REXMT] =
				    OFFSET_FROM_START(tp, tp->t_rxtcur);
				if (tp->t_timer[TCPT_PERSIST] != 0) {
					tp->t_timer[TCPT_PERSIST] = 0;
					tp->t_persist_stop = 0;
					TCP_RESET_REXMT_STATE(tp);
				}
			}

			if (tp->t_pktlist_head != NULL) {
				m_freem_list(tp->t_pktlist_head);
			}
			TCP_PKTLIST_CLEAR(tp);

			                                                       
			if (so->so_flags & SOF_NOADDRAVAIL) {
				tcp_drop(tp, EADDRNOTAVAIL);
				return EADDRNOTAVAIL;
			} else {
				tcp_check_timer_state(tp);
				return 0;                                                                
			}
		}
		if (ia != NULL) {
			IFA_REMREF(&ia->ia_ifa);
		}

		if (ia6 != NULL) {
			IFA_REMREF(&ia6->ia_ifa);
		}

		  
                                                            
                                                      
     
		RT_LOCK(rt);
		if ((ifp = rt->rt_ifp) != NULL) {
			somultipages(so, (ifp->if_hwassist & IFNET_MULTIPAGES));
			tcp_set_tso(tp, ifp);
			soif2kcl(so, (ifp->if_eflags & IFEF_2KCL));
			tcp_set_ecn(tp, ifp);
		}
		if (rt->rt_flags & RTF_UP) {
			RT_GENID_SYNC(rt);
		}
		  
                                                       
                                     
                          
                                                   
                               
     

		if (!path_mtu_discovery || ((rt != NULL) &&
		    (!(rt->rt_flags & RTF_UP) ||
		    (rt->rt_rmx.rmx_locks & RTV_MTU)))) {
			tp->t_flags &= ~TF_PMTUD;
		} else {
			tp->t_flags |= TF_PMTUD;
		}

		RT_UNLOCK(rt);
	}

	if (rt != NULL) {
		cell = IFNET_IS_CELLULAR(rt->rt_ifp);
		wifi = (!cell && IFNET_IS_WIFI(rt->rt_ifp));
		wired = (!wifi && IFNET_IS_WIRED(rt->rt_ifp));
	}

	  
                                                                   
                                                                   
                                                                  
    
	if (SACK_ENABLED(tp) && SEQ_LT(tp->snd_nxt, tp->snd_max)) {
		tcp_sack_adjust(tp);
	}
	sendalot = 0;
	off = tp->snd_nxt - tp->snd_una;
	sendwin = min(tp->snd_wnd, tp->snd_cwnd);

	if (tp->t_flags & TF_SLOWLINK && slowlink_wsize > 0) {
		sendwin = min(sendwin, slowlink_wsize);
	}

	flags = tcp_outflags[tp->t_state];
	  
                                                                 
                                                                 
                                                               
                                                               
                                                            
                            
    
	  
                                                      
    
	sack_rxmit = 0;
	sack_bytes_rxmt = 0;
	len = 0;
	p = NULL;
	if (SACK_ENABLED(tp) && IN_FASTRECOVERY(tp) &&
	    (p = tcp_sack_output(tp, &sack_bytes_rxmt))) {
		int32_t cwin;

		if (tcp_do_better_lr) {
			cwin = min(tp->snd_wnd, tp->snd_cwnd) - tcp_flight_size(tp);
			if (cwin <= 0 && sack_rxmted == FALSE) {
				                                               
				cwin = tp->t_maxseg;
			}

			sack_rxmted = TRUE;
		} else {
			cwin = min(tp->snd_wnd, tp->snd_cwnd) - sack_bytes_rxmt;
		}
		if (cwin < 0) {
			cwin = 0;
		}
		                                                        
		if (SEQ_GT(p->end, tp->snd_recover)) {
			  
                                                 
                                                     
                    
      
			if (SEQ_GEQ(p->rxmit, tp->snd_recover)) {
				  
                                                
                                              
                                              
                           
       
				p = NULL;
				goto after_sack_rexmit;
			} else {
				                                         
				len = ((int32_t)min(cwin,
				    tp->snd_recover - p->rxmit));
			}
		} else {
			len = ((int32_t)min(cwin, p->end - p->rxmit));
		}
		if (len > 0) {
			off = p->rxmit - tp->snd_una;
			sack_rxmit = 1;
			sendalot = 1;
			                                                                                                             
			tp->send_highest_sack = tp->snd_nxt;
			tp->t_new_dupacks = 0;
			tcpstat.tcps_sack_rexmits++;
			tcpstat.tcps_sack_rexmit_bytes +=
			    min(len, tp->t_maxseg);
		} else {
			len = 0;
		}
	}
after_sack_rexmit:
	  
                                                                   
                
    
	if (tp->t_flags & TF_NEEDFIN) {
		flags |= TH_FIN;
	}

	  
                                                        
                                             
                                               
                             
    
	if (tp->t_flagsext & TF_FORCE) {
		if (sendwin == 0) {
			  
                                              
                                            
                                           
                                            
                                               
                                            
                                             
                                             
                               
     
                                              
                                            
                                            
             
      
			if (off < so->so_snd.sb_cc) {
				flags &= ~TH_FIN;
			}
			sendwin = 1;
		} else {
			tp->t_timer[TCPT_PERSIST] = 0;
			tp->t_persist_stop = 0;
			TCP_RESET_REXMT_STATE(tp);
		}
	}

	  
                                                            
                                                              
                                                             
                                                              
                                                             
                                         
   
                                                                 
                                                               
              
   
                                                                   
                                     
    
	if (sack_rxmit == 0) {
		if (sack_bytes_rxmt == 0) {
			len = min(so->so_snd.sb_cc, sendwin) - off;
		} else {
			int32_t cwin;

			if (tcp_do_better_lr) {
				cwin = tp->snd_cwnd - tcp_flight_size(tp);
			} else {
				cwin = tp->snd_cwnd -
				    (tp->snd_nxt - tp->sack_newdata) -
				    sack_bytes_rxmt;
			}
			if (cwin < 0) {
				cwin = 0;
			}
			  
                                                      
                                                    
                                      
      
			len = min(so->so_snd.sb_cc, tp->snd_wnd) - off;
			  
                                         
                                                       
                                                   
                                                  
                                                    
                                         
      
			if (len > 0) {
				len = imin(len, cwin);
			} else {
				len = 0;
			}
			  
                                                  
                                                 
                                                  
                                  
      
			if (len == 0 && cwin > 0 &&
			    SEQ_LT(tp->snd_fack, tp->snd_recover) &&
			    !(tp->t_flagsext & TF_RESCUE_RXT)) {
				len = min((tp->snd_recover - tp->snd_fack),
				    tp->t_maxseg);
				len = imin(len, cwin);
				old_snd_nxt = tp->snd_nxt;
				sack_rescue_rxt = TRUE;
				tp->snd_nxt = tp->snd_recover - len;
				  
                                    
                                           
       
				if ((tp->t_flags & TF_SENTFIN) &&
				    tp->snd_max == tp->snd_recover) {
					tp->snd_nxt--;
				}

				off = tp->snd_nxt - tp->snd_una;
				sendalot = 0;
				tp->t_flagsext |= TF_RESCUE_RXT;
			}
		}
	}

	  
                                                                  
                                                                  
                                                                  
    
	if ((flags & TH_SYN) && SEQ_GT(tp->snd_nxt, tp->snd_una)) {
		if (tp->t_state == TCPS_SYN_RECEIVED && tfo_enabled(tp) && tp->snd_nxt == tp->snd_una + 1) {
			                                   
			off--;
			len++;
		} else {
			if (tp->t_state != TCPS_SYN_RECEIVED || tfo_enabled(tp)) {
				flags &= ~TH_SYN;
			}

			off--;
			len++;
			if (len > 0 && tp->t_state == TCPS_SYN_SENT) {
				while (inp->inp_sndinprog_cnt == 0 &&
				    tp->t_pktlist_head != NULL) {
					packetlist = tp->t_pktlist_head;
					packchain_listadd = tp->t_lastchain;
					packchain_sent++;
					TCP_PKTLIST_CLEAR(tp);

					error = tcp_ip_output(so, tp, packetlist,
					    packchain_listadd, tp_inp_options,
					    (so_options & SO_DONTROUTE),
					    (sack_rxmit || (sack_bytes_rxmt != 0)),
					    isipv6);
				}

				  
                                          
                   
       
				if (inp->inp_sndinprog_cnt == 0 &&
				    (tp->t_flags & TF_CLOSING)) {
					tp->t_flags &= ~TF_CLOSING;
					(void) tcp_close(tp);
				} else {
					tcp_check_timer_state(tp);
				}
				KERNEL_DEBUG(DBG_FNC_TCP_OUTPUT | DBG_FUNC_END,
				    0, 0, 0, 0, 0);
				return 0;
			}
		}
	}

	  
                                                           
                                                               
                                                  
   
                                                       
                                                              
             
    
	if ((flags & TH_SYN) && !tfo_enabled(tp)) {
		len = 0;
		flags &= ~TH_FIN;
	}

	  
                               
    
	if (flags & TH_RST) {
		len = 0;
	}

	if ((flags & TH_SYN) && tp->t_state <= TCPS_SYN_SENT && tfo_enabled(tp)) {
		len = tcp_tfo_check(tp, len);
	}

	  
                                                               
                                                             
                                                              
                                            
    
	if (len <= 0 && !(flags & TH_SYN)) {
		  
                                        
                                              
                                               
                                                   
                                                 
                                                  
                                                     
                                            
     
		len = 0;
		if (sendwin == 0) {
			tp->t_timer[TCPT_REXMT] = 0;
			tp->t_timer[TCPT_PTO] = 0;
			TCP_RESET_REXMT_STATE(tp);
			tp->snd_nxt = tp->snd_una;
			off = 0;
			if (tp->t_timer[TCPT_PERSIST] == 0) {
				tcp_setpersist(tp);
			}
		}
	}

	  
                                                             
                                                               
                                                         
                                                               
                                    
                                                                
                                                               
    
	if (!INP_WAIT_FOR_IF_FEEDBACK(inp) && !IN_FASTRECOVERY(tp) &&
	    (so->so_snd.sb_flags & (SB_AUTOSIZE | SB_TRIM)) == SB_AUTOSIZE &&
	    tcp_cansbgrow(&so->so_snd)) {
		if ((tp->snd_wnd / 4 * 5) >= so->so_snd.sb_hiwat &&
		    so->so_snd.sb_cc >= (so->so_snd.sb_hiwat / 8 * 7) &&
		    sendwin >= (so->so_snd.sb_cc - (tp->snd_nxt - tp->snd_una))) {
			if (sbreserve(&so->so_snd,
			    min(so->so_snd.sb_hiwat + tcp_autosndbuf_inc,
			    tcp_autosndbuf_max)) == 1) {
				so->so_snd.sb_idealsize = so->so_snd.sb_hiwat;
			}
		}
	}

	  
                                                                     
                                                                        
                                                        
   
                                                                
                                                                   
                                                                  
                                                                   
                                                                     
                                                                        
                      
   
                                                                      
                                                                        
                                                  
    
#if IPSEC
	  
                                                                    
                                                           
    
	if (ipsec_bypass == 0) {
		ipsec_optlen = (unsigned int)ipsec_hdrsiz_tcp(tp);
	}
#endif
	if (len > tp->t_maxseg) {
		if ((tp->t_flags & TF_TSO) && tcp_do_tso && hwcksum_tx &&
		    ip_use_randomid && kipf_count == 0 &&
		    dlil_filter_disable_tso_count == 0 &&
		    tp->rcv_numsacks == 0 && sack_rxmit == 0 &&
		    sack_bytes_rxmt == 0 &&
		    inp->inp_options == NULL &&
		    inp->in6p_options == NULL
#if IPSEC
		    && ipsec_optlen == 0
#endif
		    ) {
			tso = 1;
			sendalot = 0;
		} else {
			len = tp->t_maxseg;
			sendalot = 1;
			tso = 0;
		}
	} else {
		tso = 0;
	}

	                                                   
	if (tp->t_flagsext & TF_SENT_TLPROBE) {
		len = min(len, tp->t_maxseg);
		sendalot = 0;
		tso = 0;
	}

#if MPTCP
	if (so->so_flags & SOF_MP_SUBFLOW && off < 0) {
		os_log_error(mptcp_log_handle, "%s - %lx: offset is negative! len %d off %d\n",
		    __func__, (unsigned long)VM_KERNEL_ADDRPERM(tp->t_mpsub->mpts_mpte),
		    len, off);
	}

	if ((so->so_flags & SOF_MP_SUBFLOW) &&
	    !(tp->t_mpflags & TMPF_TCP_FALLBACK)) {
		int newlen = len;
		if (tp->t_state >= TCPS_ESTABLISHED &&
		    (tp->t_mpflags & TMPF_SND_MPPRIO ||
		    tp->t_mpflags & TMPF_SND_REM_ADDR ||
		    tp->t_mpflags & TMPF_SND_MPFAIL ||
		    tp->t_mpflags & TMPF_SND_KEYS ||
		    tp->t_mpflags & TMPF_SND_JACK)) {
			if (len > 0) {
				len = 0;
				tso = 0;
			}
			  
                                                        
                                              
      
			if (!(tp->t_mpflags & TMPF_PREESTABLISHED)) {
				sendalot = 1;
			}
			mptcp_acknow = TRUE;
		} else {
			mptcp_acknow = FALSE;
		}
		  
                                                             
                                                         
                                                             
                                                                
     
		if (len > 0 && off >= 0) {
			newlen = mptcp_adj_sendlen(so, off);
		}

		if (newlen < len) {
			len = newlen;
			if (len <= tp->t_maxseg) {
				tso = 0;
			}
		}
	}
#endif            

	if (sack_rxmit) {
		if (SEQ_LT(p->rxmit + len, tp->snd_una + so->so_snd.sb_cc)) {
			flags &= ~TH_FIN;
		}
	} else {
		if (SEQ_LT(tp->snd_nxt + len, tp->snd_una + so->so_snd.sb_cc)) {
			flags &= ~TH_FIN;
		}
	}
	  
                                                
                                            
                                                            
                                                              
                                                      
    
	recwin = tcp_sbspace(tp);

	if (!(so->so_flags & SOF_MP_SUBFLOW)) {
		if (recwin < (int32_t)(so->so_rcv.sb_hiwat / 4) &&
		    recwin < (int)tp->t_maxseg) {
			recwin = 0;
		}
	} else {
		struct mptcb *mp_tp = tptomptp(tp);
		struct socket *mp_so = mptetoso(mp_tp->mpt_mpte);

		if (recwin < (int32_t)(mp_so->so_rcv.sb_hiwat / 4) &&
		    recwin < (int)tp->t_maxseg) {
			recwin = 0;
		}
	}

#if TRAFFIC_MGT
	if (tcp_recv_bg == 1 || IS_TCP_RECV_BG(so)) {
		if (recwin > 0 && tcp_recv_throttle(tp)) {
			uint32_t min_iaj_win = tcp_min_iaj_win * tp->t_maxseg;
			uint32_t bg_rwintop = tp->rcv_adv;
			if (SEQ_LT(bg_rwintop, tp->rcv_nxt + min_iaj_win)) {
				bg_rwintop =  tp->rcv_nxt + min_iaj_win;
			}
			recwin = imin((int32_t)(bg_rwintop - tp->rcv_nxt),
			    recwin);
			if (recwin < 0) {
				recwin = 0;
			}
		}
	}
#endif                  

	if (recwin > (int32_t)(TCP_MAXWIN << tp->rcv_scale)) {
		recwin = (int32_t)(TCP_MAXWIN << tp->rcv_scale);
	}

	if (!(so->so_flags & SOF_MP_SUBFLOW)) {
		if (recwin < (int32_t)(tp->rcv_adv - tp->rcv_nxt)) {
			recwin = (int32_t)(tp->rcv_adv - tp->rcv_nxt);
		}
	} else {
		struct mptcb *mp_tp = tptomptp(tp);
		int64_t recwin_announced = (int64_t)(mp_tp->mpt_rcvadv - mp_tp->mpt_rcvnxt);

		                                                       
		VERIFY(recwin_announced < INT32_MAX && recwin_announced > INT32_MIN);
		if (recwin < (int32_t)recwin_announced) {
			recwin = (int32_t)recwin_announced;
		}
	}

	  
                                                                    
                                    
   
                                          
                           
                                               
                                                            
                                    
                                                              
                                                    
    
	if (len) {
		if (tp->t_flagsext & TF_FORCE) {
			goto send;
		}
		if (SEQ_LT(tp->snd_nxt, tp->snd_max)) {
			goto send;
		}
		if (sack_rxmit) {
			goto send;
		}

		  
                                                            
                                                                   
     
		if (tp->t_state == TCPS_SYN_RECEIVED &&
		    tfo_enabled(tp) &&
		    (tp->t_tfo_flags & TFO_F_COOKIE_VALID) &&
		    tp->snd_nxt == tp->iss + 1) {
			goto send;
		}

		  
                                                  
                        
     
		if (!INP_WAIT_FOR_IF_FEEDBACK(inp) ||
		    tp->t_state != TCPS_ESTABLISHED) {
			if (len >= tp->t_maxseg) {
				goto send;
			}

			if (!(tp->t_flags & TF_MORETOCOME) &&
			    (idle || tp->t_flags & TF_NODELAY ||
			    (tp->t_flags & TF_MAXSEGSNT) ||
			    ALLOW_LIMITED_TRANSMIT(tp)) &&
			    (tp->t_flags & TF_NOPUSH) == 0 &&
			    (len + off >= so->so_snd.sb_cc ||
			      
                                                         
                                                    
                                          
                              
          
			    so->so_flags & SOF_MP_SUBFLOW)) {
				goto send;
			}
			if (len >= tp->max_sndwnd / 2 && tp->max_sndwnd > 0) {
				goto send;
			}
		} else {
			tcpstat.tcps_fcholdpacket++;
		}
	}

	if (recwin > 0) {
		  
                                                    
                                               
                                 
     
		int32_t adv, oldwin = 0;
		adv = imin(recwin, (int)TCP_MAXWIN << tp->rcv_scale) -
		    (tp->rcv_adv - tp->rcv_nxt);

		if (SEQ_GT(tp->rcv_adv, tp->rcv_nxt)) {
			oldwin = tp->rcv_adv - tp->rcv_nxt;
		}

		if (tcp_ack_strategy == TCP_ACK_STRATEGY_LEGACY) {
			if (adv >= (int32_t) (2 * tp->t_maxseg)) {
				  
                                                   
                                                     
                                                   
                                                     
      
                                                      
                                                   
                                                 
      
                                                        
                                                        
                                        
       
				if (!(tp->t_flags & TF_STRETCHACK) &&
				    (tp->last_ack_sent != tp->rcv_nxt ||
				    ((oldwin + adv) >> tp->rcv_scale) >
				    (oldwin >> tp->rcv_scale))) {
					goto send;
				}
			}
		} else {
			if (adv >= (int32_t) (2 * tp->t_maxseg)) {
				  
                                                  
                                                    
       
				if (so->so_rcv.sb_cc < so->so_rcv.sb_lowat &&
				    (tp->last_ack_sent != tp->rcv_nxt ||
				    ((oldwin + adv) >> tp->rcv_scale) >
				    (oldwin >> tp->rcv_scale))) {
					goto send;
				}
			} else if (tp->t_flags & TF_DELACK) {
				  
                                           
                                              
                                                  
                                                   
      
                                              
                                          
       
				if (so->so_rcv.sb_cc == 0 &&
				    tp->last_ack_sent != tp->rcv_nxt &&
				    (tp->t_flagsext & TF_LAST_IS_PSH)) {
					goto send;
				}
			}
		}
		if (4 * adv >= (int32_t) so->so_rcv.sb_hiwat) {
			goto send;
		}

		  
                                                   
                                                  
                         
     
		if (tcp_ack_strategy == TCP_ACK_STRATEGY_LEGACY &&
		    (tp->t_flags & TF_STRETCHACK) &&
		    !(tp->t_flags & TF_DELACK)) {
			tp->t_flags |= TF_DELACK;
			tp->t_timer[TCPT_DELACK] =
			    OFFSET_FROM_START(tp, tcp_delack);
		}
	}

	  
                                                                    
                                                              
    
	if (tp->t_flags & TF_ACKNOW) {
		if (tp->t_forced_acks > 0) {
			tp->t_forced_acks--;
		}
		goto send;
	}
	if ((flags & TH_RST) || (flags & TH_SYN)) {
		goto send;
	}
	if (SEQ_GT(tp->snd_up, tp->snd_una)) {
		goto send;
	}
#if MPTCP
	if (mptcp_acknow) {
		goto send;
	}
#endif            
	  
                                                  
                                                      
    
	if ((flags & TH_FIN) &&
	    (!(tp->t_flags & TF_SENTFIN) || tp->snd_nxt == tp->snd_una)) {
		goto send;
	}
	  
                                                                    
                                                                  
                                         
    
	if (SACK_ENABLED(tp) && (tp->t_state >= TCPS_ESTABLISHED) &&
	    SEQ_GT(tp->snd_max, tp->snd_una) &&
	    tp->t_timer[TCPT_REXMT] == 0 &&
	    tp->t_timer[TCPT_PERSIST] == 0) {
		tp->t_timer[TCPT_REXMT] = OFFSET_FROM_START(tp,
		    tp->t_rxtcur);
		goto just_return;
	}
	  
                                                                  
                                                                 
                                                           
                                            
                                              
                                               
   
                             
                                        
               
                                                       
                           
                                     
                                                      
   
                                                                  
                                                               
                                                     
                                               
                               
    
	if (so->so_snd.sb_cc && tp->t_timer[TCPT_REXMT] == 0 &&
	    tp->t_timer[TCPT_PERSIST] == 0) {
		TCP_RESET_REXMT_STATE(tp);
		tcp_setpersist(tp);
	}
just_return:
	  
                                                         
                                                                        
    
	while (inp->inp_sndinprog_cnt == 0 &&
	    tp->t_pktlist_head != NULL) {
		packetlist = tp->t_pktlist_head;
		packchain_listadd = tp->t_lastchain;
		packchain_sent++;
		TCP_PKTLIST_CLEAR(tp);

		error = tcp_ip_output(so, tp, packetlist,
		    packchain_listadd,
		    tp_inp_options, (so_options & SO_DONTROUTE),
		    (sack_rxmit || (sack_bytes_rxmt != 0)), isipv6);
	}
	                                                      
	if (inp->inp_sndinprog_cnt == 0 &&
	    (tp->t_flags & TF_CLOSING)) {
		tp->t_flags &= ~TF_CLOSING;
		(void) tcp_close(tp);
	} else {
		tcp_check_timer_state(tp);
	}
	KERNEL_DEBUG(DBG_FNC_TCP_OUTPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
	return 0;

send:
	  
                                                             
                         
    
	if (len > 0) {
		do_not_compress = TRUE;

		if (len >= tp->t_maxseg) {
			tp->t_flags |= TF_MAXSEGSNT;
		} else {
			tp->t_flags &= ~TF_MAXSEGSNT;
		}
	}
	  
                                                        
                                         
                                                           
                                                           
                     
                                                               
    
	optlen = 0;
	if (isipv6) {
		hdrlen = sizeof(struct ip6_hdr) + sizeof(struct tcphdr);
	} else {
		hdrlen = sizeof(struct tcpiphdr);
	}
	if (flags & TH_SYN) {
		tp->snd_nxt = tp->iss;
		if ((tp->t_flags & TF_NOOPT) == 0) {
			u_short mss;

			opt[0] = TCPOPT_MAXSEG;
			opt[1] = TCPOLEN_MAXSEG;
			mss = htons((u_short) tcp_mssopt(tp));
			(void)memcpy(opt + 2, &mss, sizeof(mss));
			optlen = TCPOLEN_MAXSEG;

			if ((tp->t_flags & TF_REQ_SCALE) &&
			    ((flags & TH_ACK) == 0 ||
			    (tp->t_flags & TF_RCVD_SCALE))) {
				*((u_int32_t *)(void *)(opt + optlen)) = htonl(
					TCPOPT_NOP << 24 |
					        TCPOPT_WINDOW << 16 |
					        TCPOLEN_WINDOW << 8 |
					        tp->request_r_scale);
				optlen += 4;
			}
#if MPTCP
			if (mptcp_enable && (so->so_flags & SOF_MP_SUBFLOW)) {
				optlen = mptcp_setup_syn_opts(so, opt, optlen);
			}
#endif            
		}
	}

	  
                                                                 
                                                                  
                                                   
    
	if ((tp->t_flags & (TF_REQ_TSTMP | TF_NOOPT)) == TF_REQ_TSTMP &&
	    (flags & TH_RST) == 0 &&
	    ((flags & TH_ACK) == 0 ||
	    (tp->t_flags & TF_RCVD_TSTMP))) {
		u_int32_t *lp = (u_int32_t *)(void *)(opt + optlen);

		                                                               
		*lp++ = htonl(TCPOPT_TSTAMP_HDR);
		*lp++ = htonl(tcp_now + tp->t_ts_offset);
		*lp   = htonl(tp->ts_recent);
		optlen += TCPOLEN_TSTAMP_APPA;
	}

	if (SACK_ENABLED(tp) && ((tp->t_flags & TF_NOOPT) == 0)) {
		  
                                              
                                                     
                                                            
                                                        
                                            
     
		  
                                                      
                                                       
                                                       
                                                      
                                                          
     
		if ((flags & TH_SYN) &&
		    (!(flags & TH_ACK) || (tp->t_flags & TF_SACK_PERMIT))) {
			u_char *bp;
			bp = (u_char *)opt + optlen;

			*bp++ = TCPOPT_SACK_PERMITTED;
			*bp++ = TCPOLEN_SACK_PERMITTED;
			optlen += TCPOLEN_SACK_PERMITTED;
		}
	}
#if MPTCP
	if (so->so_flags & SOF_MP_SUBFLOW) {
		  
                                                                  
                                                            
                                                              
                                                               
     
		if (len != 0) {
			tp->t_mpflags |= (TMPF_SEND_DSN |
			    TMPF_MPTCP_ACKNOW);
		} else {
			tp->t_mpflags |= TMPF_MPTCP_ACKNOW;
		}
		optlen = mptcp_setup_opts(tp, off, &opt[0], optlen, flags,
		    len, &mptcp_acknow, &do_not_compress);
		tp->t_mpflags &= ~TMPF_SEND_DSN;
	}
#endif            

	if (tfo_enabled(tp) && !(tp->t_flags & TF_NOOPT) &&
	    (flags & (TH_SYN | TH_ACK)) == TH_SYN) {
		optlen += tcp_tfo_write_cookie(tp, optlen, len, opt);
	}

	if (tfo_enabled(tp) &&
	    (flags & (TH_SYN | TH_ACK)) == (TH_SYN | TH_ACK) &&
	    (tp->t_tfo_flags & TFO_F_OFFER_COOKIE)) {
		optlen += tcp_tfo_write_cookie_rep(tp, optlen, opt);
	}

	if (SACK_ENABLED(tp) && ((tp->t_flags & TF_NOOPT) == 0)) {
		  
                                                      
                                                      
                                               
    
                                                 
                                                  
                                                
                                                 
                                                    
                                                 
                                              
     
		if (TCPS_HAVEESTABLISHED(tp->t_state) &&
		    (tp->t_flags & TF_SACK_PERMIT) &&
		    (tp->rcv_numsacks > 0 || TCP_SEND_DSACK_OPT(tp)) &&
		    MAX_TCPOPTLEN - optlen - 2 >= TCPOLEN_SACK) {
			int nsack, padlen;
			u_char *bp = (u_char *)opt + optlen;
			u_int32_t *lp;

			nsack = (MAX_TCPOPTLEN - optlen - 2) / TCPOLEN_SACK;
			nsack = min(nsack, (tp->rcv_numsacks +
			    (TCP_SEND_DSACK_OPT(tp) ? 1 : 0)));
			sackoptlen = (2 + nsack * TCPOLEN_SACK);
			VERIFY(sackoptlen < UINT8_MAX);

			  
                                              
                                                
                                                      
      
			padlen = (MAX_TCPOPTLEN - optlen - sackoptlen) % 4;
			optlen += padlen;
			while (padlen-- > 0) {
				*bp++ = TCPOPT_NOP;
			}

			tcpstat.tcps_sack_send_blocks++;
			*bp++ = TCPOPT_SACK;
			*bp++ = (uint8_t)sackoptlen;
			lp = (u_int32_t *)(void *)bp;

			  
                                                 
                                                     
                                                 
                                
      
			if (TCP_SEND_DSACK_OPT(tp) && nsack > 0 &&
			    (tp->rcv_numsacks == 0 || nsack > 1)) {
				*lp++ = htonl(tp->t_dsack_lseq);
				*lp++ = htonl(tp->t_dsack_rseq);
				tcpstat.tcps_dsack_sent++;
				tp->t_dsack_sent++;
				nsack--;
			}
			VERIFY(nsack == 0 || tp->rcv_numsacks >= nsack);
			for (i = 0; i < nsack; i++) {
				struct sackblk sack = tp->sackblks[i];
				*lp++ = htonl(sack.start);
				*lp++ = htonl(sack.end);
			}
			optlen += sackoptlen;
		}
	}

	                                          
	if (optlen < MAX_TCPOPTLEN && (optlen % sizeof(u_int32_t))) {
		int pad = sizeof(u_int32_t) - (optlen % sizeof(u_int32_t));
		u_char *bp = (u_char *)opt + optlen;

		optlen += pad;
		while (pad) {
			*bp++ = TCPOPT_EOL;
			pad--;
		}
	}

	  
                         
                                                                    
                                                             
                              
                                                                    
                    
   
                                                                   
                                                                 
                                                                   
                                                                   
                                                                   
   
                                            
    
	if ((flags & (TH_SYN | TH_ACK)) == (TH_SYN | TH_ACK) &&
	    (tp->ecn_flags & TE_ENABLE_ECN)) {
		if (tp->ecn_flags & TE_SETUPRECEIVED) {
			if (tcp_send_ecn_flags_on_syn(tp)) {
				  
                                             
              
       
				flags |= TH_ECE;

				  
                                            
                                 
       
				tp->ecn_flags |= (TE_SETUPSENT | TE_SENDIPECT);
				tcpstat.tcps_ecn_server_setup++;
				tcpstat.tcps_ecn_server_success++;
			} else {
				  
                                              
                                         
                                              
                                              
      
                                   
                     
      
                                         
                                       
                                 
                                             
       
				if (tp->ecn_flags & TE_SETUPSENT) {
					tcpstat.tcps_ecn_lost_synack++;
					tcpstat.tcps_ecn_server_success--;
					tp->ecn_flags |= TE_LOST_SYNACK;
				}

				tp->ecn_flags &=
				    ~(TE_SETUPRECEIVED | TE_SENDIPECT |
				    TE_SENDCWR);
			}
		}
	} else if ((flags & (TH_SYN | TH_ACK)) == TH_SYN &&
	    (tp->ecn_flags & TE_ENABLE_ECN)) {
		if (tcp_send_ecn_flags_on_syn(tp)) {
			  
                                             
                   
      
			flags |= (TH_ECE | TH_CWR);
			tcpstat.tcps_ecn_client_setup++;
			tp->ecn_flags |= TE_CLIENT_SETUP;

			  
                                                      
                     
      
			tp->ecn_flags |= (TE_SETUPSENT | TE_SENDIPECT);
		} else {
			  
                                                  
                                                    
                                          
      
			if (tp->ecn_flags & TE_SETUPSENT) {
				tcpstat.tcps_ecn_lost_syn++;
				tp->ecn_flags |= TE_LOST_SYN;
			}
			tp->ecn_flags &= ~TE_SENDIPECT;
		}
	}

	  
                                            
                                                                  
                                                               
                                                                
                                                                
    
	if ((tp->ecn_flags & TE_SENDCWR) != 0 && len != 0 &&
	    !SEQ_LT(tp->snd_nxt, tp->snd_max) && !sack_rxmit) {
		flags |= TH_CWR;
		tp->ecn_flags &= ~TE_SENDCWR;
	}

	  
                                            
    
	if ((tp->ecn_flags & TE_SENDECE) != 0 && len == 0) {
		flags |= TH_ECE;
		tcpstat.tcps_ecn_sent_ece++;
	}


	hdrlen += optlen;

	                                  
	tp->t_dsack_lseq = 0;
	tp->t_dsack_rseq = 0;

	if (isipv6) {
		ipoptlen = ip6_optlen(inp);
	} else {
		if (tp_inp_options) {
			ipoptlen = tp_inp_options->m_len -
			    offsetof(struct ipoption, ipopt_list);
		} else {
			ipoptlen = 0;
		}
	}
#if IPSEC
	ipoptlen += ipsec_optlen;
#endif

	  
                                                   
                                                      
                                                    
                
   
                                                        
                                                      
                                                     
                                                       
                                                       
                                    
    
	if (len + optlen + ipoptlen > tp->t_maxopd) {
		  
                                    
                                
     
		flags &= ~TH_FIN;
		if (tso) {
			int32_t tso_maxlen;

			tso_maxlen = tp->tso_max_segment_size ?
			    tp->tso_max_segment_size : TCP_MAXWIN;

			if (len > tso_maxlen - hdrlen - optlen) {
				len = tso_maxlen - hdrlen - optlen;
				sendalot = 1;
			} else if (tp->t_flags & TF_NEEDFIN) {
				sendalot = 1;
			}

			if (len % (tp->t_maxopd - optlen) != 0) {
				len = len - (len % (tp->t_maxopd - optlen));
				sendalot = 1;
			}
		} else {
			len = tp->t_maxopd - optlen - ipoptlen;
			sendalot = 1;
		}
	}

	if (max_linkhdr + hdrlen > MCLBYTES) {
		panic("tcphdr too big");
	}

	                                                   
                                       
    
	if ((tp->t_flagsext & TF_MEASURESNDBW) != 0 &&
	    (tp->t_bwmeas != NULL) &&
	    (tp->t_flagsext & TF_BWMEAS_INPROGRESS) == 0) {
		tp->t_bwmeas->bw_size = min(min(
			    (so->so_snd.sb_cc - (tp->snd_max - tp->snd_una)),
			    tp->snd_cwnd), tp->snd_wnd);
		if (tp->t_bwmeas->bw_minsize > 0 &&
		    tp->t_bwmeas->bw_size < tp->t_bwmeas->bw_minsize) {
			tp->t_bwmeas->bw_size = 0;
		}
		if (tp->t_bwmeas->bw_maxsize > 0) {
			tp->t_bwmeas->bw_size = min(tp->t_bwmeas->bw_size,
			    tp->t_bwmeas->bw_maxsize);
		}
		if (tp->t_bwmeas->bw_size > 0) {
			tp->t_flagsext |= TF_BWMEAS_INPROGRESS;
			tp->t_bwmeas->bw_start = tp->snd_max;
			tp->t_bwmeas->bw_ts = tcp_now;
		}
	}

	VERIFY(inp->inp_flowhash != 0);
	  
                                                   
                                                  
                                              
    
	if (len) {
		                                                         
		if (tp->t_pmtud_lastseg_size == 0 && tp->snd_nxt == tp->snd_una) {
			ASSERT(len + optlen + ipoptlen <= IP_MAXPACKET);
			tp->t_pmtud_lastseg_size = (uint16_t)(len + optlen + ipoptlen);
		}
		if ((tp->t_flagsext & TF_FORCE) && len == 1) {
			tcpstat.tcps_sndprobe++;
		} else if (SEQ_LT(tp->snd_nxt, tp->snd_max) || sack_rxmit) {
			tcpstat.tcps_sndrexmitpack++;
			tcpstat.tcps_sndrexmitbyte += len;
			if (nstat_collect) {
				nstat_route_tx(inp->inp_route.ro_rt, 1,
				    len, NSTAT_TX_FLAG_RETRANSMIT);
				INP_ADD_STAT(inp, cell, wifi, wired,
				    txpackets, 1);
				INP_ADD_STAT(inp, cell, wifi, wired,
				    txbytes, len);
				tp->t_stat.txretransmitbytes += len;
				tp->t_stat.rxmitpkts++;
			}
		} else {
			tcpstat.tcps_sndpack++;
			tcpstat.tcps_sndbyte += len;

			if (nstat_collect) {
				INP_ADD_STAT(inp, cell, wifi, wired,
				    txpackets, 1);
				INP_ADD_STAT(inp, cell, wifi, wired,
				    txbytes, len);
			}
			inp_decr_sndbytes_unsent(so, len);
		}
		inp_set_activity_bitmap(inp);
#if MPTCP
		if (tp->t_mpflags & TMPF_MPTCP_TRUE) {
			tcpstat.tcps_mp_sndpacks++;
			tcpstat.tcps_mp_sndbytes += len;
		}
#endif            
		  
                                                    
                                                  
                                              
                                                    
                                      
                                                   
                                                              
                                                     
                                                          
                                    
     
		m = NULL;

		                                             
		allocated_len = MHLEN;
		if (MHLEN < hdrlen + max_linkhdr) {
			MGETHDR(m, M_DONTWAIT, MT_HEADER);
			if (m == NULL) {
				error = ENOBUFS;
				goto out;
			}
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_freem(m);
				error = ENOBUFS;
				goto out;
			}
			m->m_data += max_linkhdr;
			m->m_len = hdrlen;
			allocated_len = MCLBYTES;
		}
		if (len <= allocated_len - hdrlen - max_linkhdr) {
			if (m == NULL) {
				VERIFY(allocated_len <= MHLEN);
				MGETHDR(m, M_DONTWAIT, MT_HEADER);
				if (m == NULL) {
					error = ENOBUFS;
					goto out;
				}
				m->m_data += max_linkhdr;
				m->m_len = hdrlen;
			}
			                                                                 
			if (so->so_snd.sb_mb == NULL || off < 0) {
				if (m != NULL) {
					m_freem(m);
				}
				error = 0;                                 
				goto out;
			}
			m_copydata(so->so_snd.sb_mb, off, (int) len,
			    mtod(m, caddr_t) + hdrlen);
			m->m_len += len;
		} else {
			uint32_t copymode;
			  
                                                 
                                            
                        
      
			copymode = M_COPYM_MOVE_HDR;
#if MPTCP
			if (so->so_flags & SOF_MP_SUBFLOW) {
				copymode = M_COPYM_NOOP_HDR;
			}
#endif            
			if (m != NULL) {
				m->m_next = m_copym_mode(so->so_snd.sb_mb,
				    off, (int)len, M_DONTWAIT, copymode);
				if (m->m_next == NULL) {
					(void) m_free(m);
					error = ENOBUFS;
					goto out;
				}
			} else {
				  
                                        
                               
       
				if (so->so_snd.sb_mb == NULL) {
					error = 0;                                 
					goto out;
				}

				  
                                               
                                                    
                                                   
                                                   
       
				if ((m = m_copym_with_hdrs(so->so_snd.sb_mb,
				    off, len, M_DONTWAIT, NULL, NULL,
				    copymode)) == NULL) {
					error = ENOBUFS;
					goto out;
				}
				m->m_data += max_linkhdr;
				m->m_len = hdrlen;
			}
		}
		  
                                                     
                                                           
                                                 
                      
    
                                                     
     
		if (off + len == so->so_snd.sb_cc && !(flags & TH_SYN)) {
			flags |= TH_PUSH;
		}
	} else {
		if (tp->t_flags & TF_ACKNOW) {
			tcpstat.tcps_sndacks++;
		} else if (flags & (TH_SYN | TH_FIN | TH_RST)) {
			tcpstat.tcps_sndctrl++;
		} else if (SEQ_GT(tp->snd_up, tp->snd_una)) {
			tcpstat.tcps_sndurg++;
		} else {
			tcpstat.tcps_sndwinup++;
		}

		MGETHDR(m, M_DONTWAIT, MT_HEADER);                  
		if (m == NULL) {
			error = ENOBUFS;
			goto out;
		}
		if (MHLEN < (hdrlen + max_linkhdr)) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_freem(m);
				error = ENOBUFS;
				goto out;
			}
		}
		m->m_data += max_linkhdr;
		m->m_len = hdrlen;
	}
	m->m_pkthdr.rcvif = 0;

	                                                    
	if (flags & ~(TH_ACK)) {
		do_not_compress = TRUE;
	}

	if (tp->rcv_scale == 0) {
		do_not_compress = TRUE;
	}

	if (do_not_compress || (tcp_do_ack_compression == 1 && !cell) || __improbable(!tcp_do_ack_compression)) {
		m->m_pkthdr.comp_gencnt = 0;
	} else {
		if (TSTMP_LT(tp->t_comp_lastinc + tcp_ack_compression_rate, tcp_now)) {
			tp->t_comp_gencnt++;
			                                            
			if (tp->t_comp_gencnt <= TCP_ACK_COMPRESSION_DUMMY) {
				tp->t_comp_gencnt = TCP_ACK_COMPRESSION_DUMMY + 1;
			}
			tp->t_comp_lastinc = tcp_now;
		}
		m->m_pkthdr.comp_gencnt = tp->t_comp_gencnt;
	}

	if (isipv6) {
		ip6 = mtod(m, struct ip6_hdr *);
		th = (struct tcphdr *)(void *)(ip6 + 1);
		tcp_fillheaders(tp, ip6, th);
		if ((tp->ecn_flags & TE_SENDIPECT) != 0 && len &&
		    !SEQ_LT(tp->snd_nxt, tp->snd_max) && !sack_rxmit) {
			ip6->ip6_flow |= htonl(IPTOS_ECN_ECT0 << 20);
		}
		svc_flags |= PKT_SCF_IPV6;
#if PF_ECN
		m_pftag(m)->pftag_hdr = (void *)ip6;
		m_pftag(m)->pftag_flags |= PF_TAG_HDR_INET6;
#endif             
	} else {
		ip = mtod(m, struct ip *);
		th = (struct tcphdr *)(void *)(ip + 1);
		                                                      
		tcp_fillheaders(tp, ip, th);
		if ((tp->ecn_flags & TE_SENDIPECT) != 0 && len &&
		    !SEQ_LT(tp->snd_nxt, tp->snd_max) &&
		    !sack_rxmit && !(flags & TH_SYN)) {
			ip->ip_tos |= IPTOS_ECN_ECT0;
		}
#if PF_ECN
		m_pftag(m)->pftag_hdr = (void *)ip;
		m_pftag(m)->pftag_flags |= PF_TAG_HDR_INET;
#endif             
	}

	  
                                                  
                                                           
                                                                 
    
	if ((flags & TH_FIN) && (tp->t_flags & TF_SENTFIN) &&
	    tp->snd_nxt == tp->snd_max) {
		tp->snd_nxt--;
	}
	  
                                                      
                                                     
                                                      
                                                     
                                                      
                                                       
                                                     
                                                    
                                                    
                                                         
                                                      
   
                                                                
                    
    
	if (sack_rxmit == 0) {
		if (len || (flags & (TH_SYN | TH_FIN)) ||
		    tp->t_timer[TCPT_PERSIST]) {
			th->th_seq = htonl(tp->snd_nxt);
			if (len > 0) {
				m->m_pkthdr.tx_start_seq = tp->snd_nxt;
				m->m_pkthdr.pkt_flags |= PKTF_START_SEQ;
			}
			if (SEQ_LT(tp->snd_nxt, tp->snd_max)) {
				if (SACK_ENABLED(tp) && len > 1) {
					tcp_rxtseg_insert(tp, tp->snd_nxt,
					    (tp->snd_nxt + len - 1));
				}
				if (len > 0) {
					m->m_pkthdr.pkt_flags |=
					    PKTF_TCP_REXMT;
				}
			}
		} else {
			th->th_seq = htonl(tp->snd_max);
		}
	} else {
		th->th_seq = htonl(p->rxmit);
		if (len > 0) {
			m->m_pkthdr.pkt_flags |=
			    (PKTF_TCP_REXMT | PKTF_START_SEQ);
			m->m_pkthdr.tx_start_seq = p->rxmit;
		}
		tcp_rxtseg_insert(tp, p->rxmit, (p->rxmit + len - 1));
		p->rxmit += len;
		tp->sackhint.sack_bytes_rexmit += len;
	}
	th->th_ack = htonl(tp->rcv_nxt);
	tp->last_ack_sent = tp->rcv_nxt;
	if (optlen) {
		bcopy(opt, th + 1, optlen);
		th->th_off = (sizeof(struct tcphdr) + optlen) >> 2;
	}
	th->th_flags = flags;
	th->th_win = htons((u_short) (recwin >> tp->rcv_scale));
	tp->t_last_recwin = recwin;
	if (!(so->so_flags & SOF_MP_SUBFLOW)) {
		if (recwin > 0 && SEQ_LT(tp->rcv_adv, tp->rcv_nxt + recwin)) {
			tp->rcv_adv = tp->rcv_nxt + recwin;
		}
	} else {
		struct mptcb *mp_tp = tptomptp(tp);
		if (recwin > 0) {
			tp->rcv_adv = tp->rcv_nxt + recwin;
		}

		if (recwin > 0 && MPTCP_SEQ_LT(mp_tp->mpt_rcvadv, mp_tp->mpt_rcvnxt + recwin)) {
			mp_tp->mpt_rcvadv = mp_tp->mpt_rcvnxt + recwin;
		}
	}

	  
                                                                 
                                                                      
                                                                   
                                                                      
                                                                   
                   
    
	if (th->th_win == 0) {
		tp->t_flags |= TF_RXWIN0SENT;
	} else {
		tp->t_flags &= ~TF_RXWIN0SENT;
	}

	if (SEQ_GT(tp->snd_up, tp->snd_nxt)) {
		th->th_urp = htons((u_short)(tp->snd_up - tp->snd_nxt));
		th->th_flags |= TH_URG;
	} else {
		  
                                               
                                                           
                                                              
                       
     
		tp->snd_up = tp->snd_una;                                  
	}

	  
                                               
                                      
    
	m->m_pkthdr.len = hdrlen + len;                            

	  
                                                                   
                                                              
          
    
	if (tp->t_state != TCPS_ESTABLISHED &&
	    (tp->t_state == TCPS_CLOSING || tp->t_state == TCPS_TIME_WAIT
	    || tp->t_state == TCPS_LAST_ACK || (th->th_flags & TH_RST))) {
		m->m_pkthdr.pkt_flags |= PKTF_LAST_PKT;
	}

	if (isipv6) {
		  
                                                              
                   
     
		m->m_pkthdr.csum_flags = CSUM_TCPIPV6;
		m->m_pkthdr.csum_data = offsetof(struct tcphdr, th_sum);
		if (len + optlen) {
			th->th_sum = in_addword(th->th_sum,
			    htons((u_short)(optlen + len)));
		}
	} else {
		m->m_pkthdr.csum_flags = CSUM_TCP;
		m->m_pkthdr.csum_data = offsetof(struct tcphdr, th_sum);
		if (len + optlen) {
			th->th_sum = in_addword(th->th_sum,
			    htons((u_short)(optlen + len)));
		}
	}

	  
                                                    
                                                      
    
	if (tso) {
		if (isipv6) {
			m->m_pkthdr.csum_flags |= CSUM_TSO_IPV6;
		} else {
			m->m_pkthdr.csum_flags |= CSUM_TSO_IPV4;
		}

		m->m_pkthdr.tso_segsz = tp->t_maxopd - optlen;
	} else {
		m->m_pkthdr.tso_segsz = 0;
	}

	  
                                                            
                                                        
    
	if (!(tp->t_flagsext & TF_FORCE)
	    || tp->t_timer[TCPT_PERSIST] == 0) {
		tcp_seq startseq = tp->snd_nxt;

		  
                                                         
     
		if (flags & (TH_SYN | TH_FIN)) {
			if (flags & TH_SYN) {
				tp->snd_nxt++;
			}
			if ((flags & TH_FIN) &&
			    !(tp->t_flags & TF_SENTFIN)) {
				tp->snd_nxt++;
				tp->t_flags |= TF_SENTFIN;
			}
		}
		if (sack_rxmit) {
			goto timer;
		}
		if (sack_rescue_rxt == TRUE) {
			tp->snd_nxt = old_snd_nxt;
			sack_rescue_rxt = FALSE;
			tcpstat.tcps_pto_in_recovery++;
		} else {
			tp->snd_nxt += len;
		}
		if (SEQ_GT(tp->snd_nxt, tp->snd_max)) {
			tp->snd_max = tp->snd_nxt;
			tp->t_sndtime = tcp_now;
			  
                                                        
                                    
      
			if (tp->t_rtttime == 0) {
				tp->t_rtttime = tcp_now;
				tp->t_rtseq = startseq;
				tcpstat.tcps_segstimed++;

				                                          
				tp->t_pipeack_lastuna = tp->snd_una;
			}
		}

		  
                                               
                                                
     
timer:
		if (tp->t_timer[TCPT_REXMT] == 0 &&
		    ((sack_rxmit && tp->snd_nxt != tp->snd_max) ||
		    tp->snd_nxt != tp->snd_una || (flags & TH_FIN))) {
			if (tp->t_timer[TCPT_PERSIST]) {
				tp->t_timer[TCPT_PERSIST] = 0;
				tp->t_persist_stop = 0;
				TCP_RESET_REXMT_STATE(tp);
			}
			tp->t_timer[TCPT_REXMT] =
			    OFFSET_FROM_START(tp, tp->t_rxtcur);
		}

		  
                                                     
                                                  
                                            
    
                                                    
     
		if (tcp_enable_tlp && len != 0 && tp->t_state == TCPS_ESTABLISHED &&
		    SACK_ENABLED(tp) && !IN_FASTRECOVERY(tp) &&
		    tp->snd_nxt == tp->snd_max &&
		    SEQ_GT(tp->snd_nxt, tp->snd_una) &&
		    tp->t_rxtshift == 0 &&
		    (tp->t_flagsext & (TF_SENT_TLPROBE | TF_PKTS_REORDERED)) == 0) {
			uint32_t pto, srtt;

			if (tcp_do_better_lr) {
				srtt = tp->t_srtt >> TCP_RTT_SHIFT;
				pto = 2 * srtt;
				if ((tp->snd_max - tp->snd_una) <= tp->t_maxseg) {
					pto += tcp_delack;
				} else {
					pto += 2;
				}
			} else {
				  
                                                     
                                                       
                                                   
                                    
       
				srtt = tp->t_srtt >> TCP_RTT_SHIFT;
				pto = ((TCP_REXMTVAL(tp)) * 3) >> 1;
				pto = max(2 * srtt, pto);
				if ((tp->snd_max - tp->snd_una) == tp->t_maxseg) {
					pto = max(pto,
					    (((3 * pto) >> 2) + tcp_delack * 2));
				} else {
					pto = max(10, pto);
				}
			}

			                                                 
			if (tp->t_rxtcur < pto) {
				pto = tp->t_rxtcur;
			}

			tp->t_timer[TCPT_PTO] = OFFSET_FROM_START(tp, pto);
		}
	} else {
		  
                                                     
                                                       
     
		int xlen = len;
		if (flags & TH_SYN) {
			++xlen;
		}
		if ((flags & TH_FIN) &&
		    !(tp->t_flags & TF_SENTFIN)) {
			++xlen;
			tp->t_flags |= TF_SENTFIN;
		}
		if (SEQ_GT(tp->snd_nxt + xlen, tp->snd_max)) {
			tp->snd_max = tp->snd_nxt + len;
			tp->t_sndtime = tcp_now;
		}
	}

#if TCPDEBUG
	  
          
    
	if (so_options & SO_DEBUG) {
		tcp_trace(TA_OUTPUT, tp->t_state, tp, mtod(m, void *), th, 0);
	}
#endif

	  
                                                  
                                                   
                                                
                                                          
    
	  
                                                                  
                                
    
	if (isipv6) {
		  
                                                            
                                                        
                                                         
                        
     
		ip6->ip6_hlim = in6_selecthlim(inp, inp->in6p_route.ro_rt ?
		    inp->in6p_route.ro_rt->rt_ifp : NULL);

		                                  
		KERNEL_DEBUG(DBG_LAYER_BEG,
		    ((inp->inp_fport << 16) | inp->inp_lport),
		    (((inp->in6p_laddr.s6_addr16[0] & 0xffff) << 16) |
		    (inp->in6p_faddr.s6_addr16[0] & 0xffff)),
		    sendalot, 0, 0);
	} else {
		ASSERT(m->m_pkthdr.len <= IP_MAXPACKET);
		ip->ip_len = (u_short)m->m_pkthdr.len;
		ip->ip_ttl = inp->inp_ip_ttl;            
		ip->ip_tos |= (inp->inp_ip_tos & ~IPTOS_ECN_MASK);         
		KERNEL_DEBUG(DBG_LAYER_BEG,
		    ((inp->inp_fport << 16) | inp->inp_lport),
		    (((inp->inp_laddr.s_addr & 0xffff) << 16) |
		    (inp->inp_faddr.s_addr & 0xffff)), 0, 0, 0);
	}

	  
                                      
                                                        
                                                     
                                                          
                      
                                                               
                               
    
	if (!isipv6) {
		if (path_mtu_discovery && (tp->t_flags & TF_PMTUD)) {
			ip->ip_off |= IP_DF;
		}
	}

#if NECP
	{
		necp_kernel_policy_id policy_id;
		necp_kernel_policy_id skip_policy_id;
		u_int32_t route_rule_id;
		u_int32_t pass_flags;
		if (!necp_socket_is_allowed_to_send_recv(inp, NULL, 0, &policy_id, &route_rule_id, &skip_policy_id, &pass_flags)) {
			TCP_LOG_DROP_NECP(isipv6 ? (void *)ip6 : (void *)ip, th, tp, true);
			m_freem(m);
			error = EHOSTUNREACH;
			goto out;
		}
		necp_mark_packet_from_socket(m, inp, policy_id, route_rule_id, skip_policy_id, pass_flags);

		if (net_qos_policy_restricted != 0) {
			necp_socket_update_qos_marking(inp, inp->inp_route.ro_rt, route_rule_id);
		}
	}
#endif           

#if IPSEC
	if (inp->inp_sp != NULL) {
		ipsec_setsocket(m, so);
	}
#endif          

	  
                                                                                                            
    
	lost = 0;

	  
                                                         
                               
    
	m->m_pkthdr.pkt_flowsrc = FLOWSRC_INPCB;
	m->m_pkthdr.pkt_flowid = inp->inp_flowhash;
	m->m_pkthdr.pkt_flags |= (PKTF_FLOW_ID | PKTF_FLOW_LOCALSRC | PKTF_FLOW_ADV);
	m->m_pkthdr.pkt_proto = IPPROTO_TCP;
	m->m_pkthdr.tx_tcp_pid = so->last_pid;
	if (so->so_flags & SOF_DELEGATED) {
		m->m_pkthdr.tx_tcp_e_pid = so->e_pid;
	} else {
		m->m_pkthdr.tx_tcp_e_pid = 0;
	}

	m->m_nextpkt = NULL;

	if (inp->inp_last_outifp != NULL &&
	    !(inp->inp_last_outifp->if_flags & IFF_LOOPBACK)) {
		                                    
                                 
                                                           
                                       
                             
                                                        
     
		if (len == 0 && (inp->inp_last_outifp->if_eflags & (IFEF_TXSTART | IFEF_NOACKPRI)) == IFEF_TXSTART) {
			if (th->th_flags == TH_ACK &&
			    tp->snd_una == tp->snd_max &&
			    tp->t_timer[TCPT_REXMT] == 0) {
				svc_flags |= PKT_SCF_TCP_ACK;
			}
			if (th->th_flags & TH_SYN) {
				svc_flags |= PKT_SCF_TCP_SYN;
			}
		}
		set_packet_service_class(m, so, sotc, svc_flags);
	} else {
		  
                                                
                  
     
		(void) m_set_service_class(m, so_tc2msc(sotc));
	}

	TCP_LOG_TH_FLAGS(isipv6 ? (void *)ip6 : (void *)ip, th, tp, true,
	    inp->inp_last_outifp != NULL ? inp->inp_last_outifp :
	    inp->inp_boundifp);

	tp->t_pktlist_sentlen += len;
	tp->t_lastchain++;

	if (isipv6) {
		DTRACE_TCP5(send, struct mbuf *, m, struct inpcb *, inp,
		    struct ip6 *, ip6, struct tcpcb *, tp, struct tcphdr *,
		    th);
	} else {
		DTRACE_TCP5(send, struct mbuf *, m, struct inpcb *, inp,
		    struct ip *, ip, struct tcpcb *, tp, struct tcphdr *, th);
	}

	if (tp->t_pktlist_head != NULL) {
		tp->t_pktlist_tail->m_nextpkt = m;
		tp->t_pktlist_tail = m;
	} else {
		packchain_newlist++;
		tp->t_pktlist_head = tp->t_pktlist_tail = m;
	}

	if (sendalot == 0 || (tp->t_state != TCPS_ESTABLISHED) ||
	    (tp->snd_cwnd <= (tp->snd_wnd / 8)) ||
	    (tp->t_flags & TF_ACKNOW) ||
	    (tp->t_flagsext & TF_FORCE) ||
	    tp->t_lastchain >= tcp_packet_chaining) {
		error = 0;
		while (inp->inp_sndinprog_cnt == 0 &&
		    tp->t_pktlist_head != NULL) {
			packetlist = tp->t_pktlist_head;
			packchain_listadd = tp->t_lastchain;
			packchain_sent++;
			lost = tp->t_pktlist_sentlen;
			TCP_PKTLIST_CLEAR(tp);

			error = tcp_ip_output(so, tp, packetlist,
			    packchain_listadd, tp_inp_options,
			    (so_options & SO_DONTROUTE),
			    (sack_rxmit || (sack_bytes_rxmt != 0)), isipv6);
			if (error) {
				  
                                           
                                              
                                             
                            
       
				lost += tp->t_pktlist_sentlen;
				break;
			} else {
				lost = 0;
			}
		}
		                                                      
		if (inp->inp_sndinprog_cnt == 0 &&
		    (tp->t_flags & TF_CLOSING)) {
			tp->t_flags &= ~TF_CLOSING;
			(void) tcp_close(tp);
			return 0;
		}
	} else {
		error = 0;
		packchain_looped++;
		tcpstat.tcps_sndtotal++;

		goto again;
	}
	if (error) {
		  
                                                       
                                                           
                                                            
                                                           
                                                             
     
		if (!(tp->t_flagsext & TF_FORCE)
		    || tp->t_timer[TCPT_PERSIST] == 0) {
			  
                                              
                                            
      
			if ((flags & TH_SYN) == 0) {
				if (sack_rxmit) {
					if (SEQ_GT((p->rxmit - lost),
					    tp->snd_una)) {
						p->rxmit -= lost;

						if (SEQ_LT(p->rxmit, p->start)) {
							p->rxmit = p->start;
						}
					} else {
						lost = p->rxmit - tp->snd_una;
						p->rxmit = tp->snd_una;

						if (SEQ_LT(p->rxmit, p->start)) {
							p->rxmit = p->start;
						}
					}
					tp->sackhint.sack_bytes_rexmit -= lost;
					if (tp->sackhint.sack_bytes_rexmit < 0) {
						tp->sackhint.sack_bytes_rexmit = 0;
					}
				} else {
					if (SEQ_GT((tp->snd_nxt - lost),
					    tp->snd_una)) {
						tp->snd_nxt -= lost;
					} else {
						tp->snd_nxt = tp->snd_una;
					}
				}
			}
		}
out:
		if (tp->t_pktlist_head != NULL) {
			m_freem_list(tp->t_pktlist_head);
		}
		TCP_PKTLIST_CLEAR(tp);

		if (error == ENOBUFS) {
			  
                                               
                                                  
                                              
      
			if (tp->t_timer[TCPT_REXMT] == 0 &&
			    tp->t_timer[TCPT_PERSIST] == 0 &&
			    (len != 0 || (flags & (TH_SYN | TH_FIN)) != 0 ||
			    so->so_snd.sb_cc > 0)) {
				tp->t_timer[TCPT_REXMT] =
				    OFFSET_FROM_START(tp, tp->t_rxtcur);
			}
			tp->snd_cwnd = tp->t_maxseg;
			tp->t_bytes_acked = 0;
			tcp_check_timer_state(tp);
			KERNEL_DEBUG(DBG_FNC_TCP_OUTPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);

			tcp_ccdbg_trace(tp, NULL, TCP_CC_OUTPUT_ERROR);
			return 0;
		}
		if (error == EMSGSIZE) {
			  
                                                   
                                                      
                                                    
                     
     
                                                  
                                                   
                                               
                                                        
                       
      
			if (tso) {
				tp->t_flags &= ~TF_TSO;
			}

			tcp_mtudisc(inp, 0);
			tcp_check_timer_state(tp);

			KERNEL_DEBUG(DBG_FNC_TCP_OUTPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
			return 0;
		}
		  
                                                        
                                                               
     
		if ((error == EHOSTUNREACH || error == ENETDOWN || error == EADDRNOTAVAIL) &&
		    TCPS_HAVERCVDSYN(tp->t_state) &&
		    !inp_restricted_send(inp, inp->inp_last_outifp)) {
			tp->t_softerror = error;
			error = 0;
		}
		tcp_check_timer_state(tp);
		KERNEL_DEBUG(DBG_FNC_TCP_OUTPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
		return error;
	}

	tcpstat.tcps_sndtotal++;

	KERNEL_DEBUG(DBG_FNC_TCP_OUTPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
	if (sendalot) {
		goto again;
	}

	tcp_check_timer_state(tp);

	return 0;
}

static int
tcp_ip_output(struct socket *so, struct tcpcb *tp, struct mbuf *pkt,
    int cnt, struct mbuf *opt, int flags, int sack_in_progress, boolean_t isipv6)
{
	int error = 0;
	boolean_t chain;
	boolean_t unlocked = FALSE;
	boolean_t ifdenied = FALSE;
	struct inpcb *inp = tp->t_inpcb;
	struct ip_out_args ipoa;
	struct route ro;
	struct ifnet *outif = NULL;
	bool check_qos_marking_again = (so->so_flags1 & SOF1_QOSMARKING_POLICY_OVERRIDE) ? FALSE : TRUE;

	bzero(&ipoa, sizeof(ipoa));
	ipoa.ipoa_boundif = IFSCOPE_NONE;
	ipoa.ipoa_flags = IPOAF_SELECT_SRCIF | IPOAF_BOUND_SRCADDR;
	ipoa.ipoa_sotc = SO_TC_UNSPEC;
	ipoa.ipoa_netsvctype = _NET_SERVICE_TYPE_UNSPEC;
	struct ip6_out_args ip6oa;
	struct route_in6 ro6;

	bzero(&ip6oa, sizeof(ip6oa));
	ip6oa.ip6oa_boundif = IFSCOPE_NONE;
	ip6oa.ip6oa_flags = IP6OAF_SELECT_SRCIF | IP6OAF_BOUND_SRCADDR;
	ip6oa.ip6oa_sotc = SO_TC_UNSPEC;
	ip6oa.ip6oa_netsvctype = _NET_SERVICE_TYPE_UNSPEC;

	struct flowadv *adv =
	    (isipv6 ? &ip6oa.ip6oa_flowadv : &ipoa.ipoa_flowadv);

	                                                                
	if (inp->inp_flags & INP_BOUND_IF) {
		if (isipv6) {
			ip6oa.ip6oa_boundif = inp->inp_boundifp->if_index;
			ip6oa.ip6oa_flags |= IP6OAF_BOUND_IF;
		} else {
			ipoa.ipoa_boundif = inp->inp_boundifp->if_index;
			ipoa.ipoa_flags |= IPOAF_BOUND_IF;
		}
	}

	if (INP_NO_CELLULAR(inp)) {
		if (isipv6) {
			ip6oa.ip6oa_flags |=  IP6OAF_NO_CELLULAR;
		} else {
			ipoa.ipoa_flags |=  IPOAF_NO_CELLULAR;
		}
	}
	if (INP_NO_EXPENSIVE(inp)) {
		if (isipv6) {
			ip6oa.ip6oa_flags |=  IP6OAF_NO_EXPENSIVE;
		} else {
			ipoa.ipoa_flags |=  IPOAF_NO_EXPENSIVE;
		}
	}
	if (INP_NO_CONSTRAINED(inp)) {
		if (isipv6) {
			ip6oa.ip6oa_flags |=  IP6OAF_NO_CONSTRAINED;
		} else {
			ipoa.ipoa_flags |=  IPOAF_NO_CONSTRAINED;
		}
	}
	if (INP_AWDL_UNRESTRICTED(inp)) {
		if (isipv6) {
			ip6oa.ip6oa_flags |=  IP6OAF_AWDL_UNRESTRICTED;
		} else {
			ipoa.ipoa_flags |=  IPOAF_AWDL_UNRESTRICTED;
		}
	}
	if (INP_INTCOPROC_ALLOWED(inp) && isipv6) {
		ip6oa.ip6oa_flags |=  IP6OAF_INTCOPROC_ALLOWED;
	}
	if (isipv6) {
		ip6oa.ip6oa_sotc = so->so_traffic_class;
		ip6oa.ip6oa_netsvctype = so->so_netsvctype;
		ip6oa.qos_marking_gencount = inp->inp_policyresult.results.qos_marking_gencount;
	} else {
		ipoa.ipoa_sotc = so->so_traffic_class;
		ipoa.ipoa_netsvctype = so->so_netsvctype;
		ipoa.qos_marking_gencount = inp->inp_policyresult.results.qos_marking_gencount;
	}
	if ((so->so_flags1 & SOF1_QOSMARKING_ALLOWED)) {
		if (isipv6) {
			ip6oa.ip6oa_flags |= IP6OAF_QOSMARKING_ALLOWED;
		} else {
			ipoa.ipoa_flags |= IPOAF_QOSMARKING_ALLOWED;
		}
	}
	if (check_qos_marking_again) {
		if (isipv6) {
			ip6oa.ip6oa_flags |= IP6OAF_REDO_QOSMARKING_POLICY;
		} else {
			ipoa.ipoa_flags |= IPOAF_REDO_QOSMARKING_POLICY;
		}
	}
	if (isipv6) {
		flags |= IPV6_OUTARGS;
	} else {
		flags |= IP_OUTARGS;
	}

	                                                       
	if (isipv6) {
		in6p_route_copyout(inp, &ro6);
	} else {
		inp_route_copyout(inp, &ro);
	}

	  
                                                      
                         
    
	tp->last_ack_sent = tp->rcv_nxt;
	tp->t_flags &= ~(TF_ACKNOW | TF_DELACK);
	tp->t_timer[TCPT_DELACK] = 0;
	tp->t_unacksegs = 0;

	                                                        
	inp->inp_sndinprog_cnt++;

	  
                                             
                                                 
                                                           
             
                              
                                     
                                          
    
	if (tcp_output_unlocked && !so->so_upcallusecount &&
	    (tp->t_state == TCPS_ESTABLISHED) && (sack_in_progress == 0) &&
	    !IN_FASTRECOVERY(tp) && !(so->so_flags & SOF_MP_SUBFLOW)) {
		unlocked = TRUE;
		socket_unlock(so, 0);
	}

	  
                                            
                              
                                
                                                      
    

	chain = tcp_packet_chaining > 1
#if IPSEC
	    && ipsec_bypass
#endif
	;                                         

	while (pkt != NULL) {
		struct mbuf *npkt = pkt->m_nextpkt;

		if (!chain) {
			pkt->m_nextpkt = NULL;
			  
                                                         
                                                      
                                                       
                                                        
                                                          
                                                        
                                                        
                                                        
                               
      
			cnt = 0;
		}
		if (isipv6) {
			error = ip6_output_list(pkt, cnt,
			    inp->in6p_outputopts, &ro6, flags, NULL, NULL,
			    &ip6oa);
			ifdenied = (ip6oa.ip6oa_retflags & IP6OARF_IFDENIED);
		} else {
			error = ip_output_list(pkt, cnt, opt, &ro, flags, NULL,
			    &ipoa);
			ifdenied = (ipoa.ipoa_retflags & IPOARF_IFDENIED);
		}

		if (chain || error) {
			  
                                                    
                                                   
                                                      
      
			if (!chain) {
				m_freem_list(npkt);
			}
			break;
		}
		pkt = npkt;
	}

	if (unlocked) {
		socket_lock(so, 0);
	}

	  
                                                                
                                                                 
                        
   
                                                            
             
    
	if (((adv->code == FADV_FLOW_CONTROLLED && !IN_FASTRECOVERY(tp)) ||
	    adv->code == FADV_SUSPENDED) &&
	    !(tp->t_flags & TF_CLOSING) &&
	    tp->t_state == TCPS_ESTABLISHED &&
	    SEQ_GT(tp->snd_max, tp->snd_una)) {
		int rc;
		rc = inp_set_fc_state(inp, adv->code);

		if (rc == 1) {
			tcp_ccdbg_trace(tp, NULL,
			    ((adv->code == FADV_FLOW_CONTROLLED) ?
			    TCP_CC_FLOW_CONTROL : TCP_CC_SUSPEND));
		}
	}

	  
                                                       
                                                      
              
    
	if (adv->code == FADV_SUSPENDED) {
		error = ENOBUFS;
	}

	VERIFY(inp->inp_sndinprog_cnt > 0);
	if (--inp->inp_sndinprog_cnt == 0) {
		inp->inp_flags &= ~(INP_FC_FEEDBACK);
		if (inp->inp_sndingprog_waiters > 0) {
			wakeup(&inp->inp_sndinprog_cnt);
		}
	}

	if (isipv6) {
		  
                                                                 
                                                                     
                       
     
		if (ip6oa.ip6oa_flags & IP6OAF_BOUND_IF) {
			outif = ifindex2ifnet[ip6oa.ip6oa_boundif];
		} else if (ro6.ro_rt != NULL) {
			outif = ro6.ro_rt->rt_ifp;
		}
	} else {
		if (ro.ro_rt != NULL) {
			outif = ro.ro_rt->rt_ifp;
		}
	}
	if (check_qos_marking_again) {
		uint32_t qos_marking_gencount;
		bool allow_qos_marking;
		if (isipv6) {
			qos_marking_gencount = ip6oa.qos_marking_gencount;
			allow_qos_marking = ip6oa.ip6oa_flags & IP6OAF_QOSMARKING_ALLOWED ? TRUE : FALSE;
		} else {
			qos_marking_gencount = ipoa.qos_marking_gencount;
			allow_qos_marking = ipoa.ipoa_flags & IPOAF_QOSMARKING_ALLOWED ? TRUE : FALSE;
		}
		inp->inp_policyresult.results.qos_marking_gencount = qos_marking_gencount;
		if (allow_qos_marking == TRUE) {
			inp->inp_socket->so_flags1 |= SOF1_QOSMARKING_ALLOWED;
		} else {
			inp->inp_socket->so_flags1 &= ~SOF1_QOSMARKING_ALLOWED;
		}
	}

	if (outif != NULL && outif != inp->inp_last_outifp) {
		                                
		if (so->so_snd.sb_cc > 0 && so->so_snd.sb_flags & SB_SNDBYTE_CNT) {
			inp_decr_sndbytes_total(so, so->so_snd.sb_cc);
			inp_decr_sndbytes_allunsent(so, tp->snd_una);
			so->so_snd.sb_flags &= ~SB_SNDBYTE_CNT;
		}
		inp->inp_last_outifp = outif;
	}

	if (error != 0 && ifdenied &&
	    (INP_NO_CELLULAR(inp) || INP_NO_EXPENSIVE(inp) || INP_NO_CONSTRAINED(inp))) {
		soevent(so,
		    (SO_FILT_HINT_LOCKED | SO_FILT_HINT_IFDENIED));
	}

	                                            
	if (isipv6) {
		in6p_route_copyin(inp, &ro6);
	} else {
		inp_route_copyin(inp, &ro);
	}

	if (tp->t_state < TCPS_ESTABLISHED && tp->t_rxtshift == 0 &&
	    tp->t_inpcb->inp_route.ro_rt != NULL) {
		                                                  
                               
     
		tcp_getrt_rtt(tp, tp->t_inpcb->in6p_route.ro_rt);
		tp->t_timer[TCPT_REXMT] = OFFSET_FROM_START(tp, tp->t_rxtcur);
	}
	return error;
}

int tcptv_persmin_val = TCPTV_PERSMIN;

void
tcp_setpersist(struct tcpcb *tp)
{
	int t = ((tp->t_srtt >> 2) + tp->t_rttvar) >> 1;

	                                                      
                                                             
                                                      
                      
    

	if (tp->t_persist_timeout != 0 &&
	    tp->t_timer[TCPT_PERSIST] == 0 &&
	    tp->t_persist_stop == 0) {
		tp->t_persist_stop = tcp_now + tp->t_persist_timeout;
	}

	  
                                    
    
	TCPT_RANGESET(tp->t_timer[TCPT_PERSIST],
	    t * tcp_backoff[tp->t_rxtshift],
	    tcptv_persmin_val, TCPTV_PERSMAX, 0);
	tp->t_timer[TCPT_PERSIST] = OFFSET_FROM_START(tp, tp->t_timer[TCPT_PERSIST]);

	if (tp->t_rxtshift < TCP_MAXRXTSHIFT) {
		tp->t_rxtshift++;
	}
}

static int
tcp_recv_throttle(struct tcpcb *tp)
{
	uint32_t base_rtt, newsize;
	struct sockbuf *sbrcv = &tp->t_inpcb->inp_socket->so_rcv;

	if (tcp_use_rtt_recvbg == 1 &&
	    TSTMP_SUPPORTED(tp)) {
		  
                                                     
                                            
     

		  
                                                           
                                                     
     
		if (tp->t_flagsext & TF_RECV_THROTTLE) {
			return 1;
		}

		base_rtt = get_base_rtt(tp);

		if (base_rtt != 0 && tp->t_rttcur != 0) {
			  
                                                
                                   
      
			if (tp->t_rttcur > (base_rtt + target_qdelay)) {
				tp->t_flagsext |= TF_RECV_THROTTLE;
				if (tp->t_recv_throttle_ts == 0) {
					tp->t_recv_throttle_ts = tcp_now;
				}
				  
                                            
                       
       
				if (sbrcv->sb_idealsize >
				    tcp_recv_throttle_minwin) {
					newsize = sbrcv->sb_idealsize >> 1;
					                           
					newsize =
					    max(newsize,
					    tcp_recv_throttle_minwin);
					sbrcv->sb_idealsize = newsize;
				}
				return 1;
			} else {
				return 0;
			}
		}
	}

	  
                                                        
                                       
    
	if (tp->acc_iaj > tcp_acc_iaj_react_limit) {
		return 1;
	}

	return 0;
}
