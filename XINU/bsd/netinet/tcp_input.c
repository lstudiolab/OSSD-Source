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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993, 1994, 1995
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
 *	@(#)tcp_input.c	8.12 (Berkeley) 5/24/95
 * $FreeBSD: src/sys/netinet/tcp_input.c,v 1.107.2.16 2001/08/22 00:59:12 silby Exp $
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/proc.h>                                      
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/syslog.h>
#include <sys/mcache.h>
#if XNU_TARGET_OS_OSX
#include <sys/kasl.h>
#endif                        
#include <sys/kauth.h>
#include <kern/cpu_number.h>                                              

#include <machine/endian.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/ntstat.h>
#include <net/content_filter.h>
#include <net/dlil.h>
#include <net/multi_layer_pkt_log.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>                           
#include <netinet/in_var.h>
#include <netinet/icmp_var.h>                         
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <mach/sdt.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet6/nd6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet/tcp.h>
#include <netinet/tcp_cache.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_seq.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_cc.h>
#include <dev/random/randomdev.h>
#include <kern/zalloc.h>
#include <netinet6/tcp6_var.h>
#include <netinet/tcpip.h>
#if TCPDEBUG
#include <netinet/tcp_debug.h>
u_char tcp_saveipgen[40];                                                  
struct tcphdr tcp_savetcp;
#endif               
#include <netinet/tcp_log.h>

#if IPSEC
#include <netinet6/ipsec.h>
#include <netinet6/ipsec6.h>
#include <netkey/key.h>
#endif          

#include <sys/kdebug.h>
#if MPTCP
#include <netinet/mptcp_var.h>
#include <netinet/mptcp.h>
#include <netinet/mptcp_opt.h>
#endif            

#include <corecrypto/ccaes.h>

#define DBG_LAYER_BEG           NETDBG_CODE(DBG_NETTCP, 0)
#define DBG_LAYER_END           NETDBG_CODE(DBG_NETTCP, 2)
#define DBG_FNC_TCP_INPUT       NETDBG_CODE(DBG_NETTCP, (3 << 8))
#define DBG_FNC_TCP_NEWCONN     NETDBG_CODE(DBG_NETTCP, (7 << 8))

#define TCP_RTT_HISTORY_EXPIRE_TIME     (60 * TCP_RETRANSHZ)
#define TCP_RECV_THROTTLE_WIN   (5 * TCP_RETRANSHZ)
#define TCP_STRETCHACK_ENABLE_PKTCNT    2000

struct  tcpstat tcpstat;

SYSCTL_SKMEM_TCP_INT(OID_AUTO, flow_control_response,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_flow_control_response, 1,
    "Improved response to Flow-control events");

static int log_in_vain = 0;
SYSCTL_INT(_net_inet_tcp, OID_AUTO, log_in_vain,
    CTLFLAG_RW | CTLFLAG_LOCKED, &log_in_vain, 0,
    "Log all incoming TCP connections");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, ack_strategy,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_ack_strategy, TCP_ACK_STRATEGY_MODERN,
    "Revised TCP ACK-strategy, avoiding stretch-ACK implementation");

static int blackhole = 0;
SYSCTL_INT(_net_inet_tcp, OID_AUTO, blackhole,
    CTLFLAG_RW | CTLFLAG_LOCKED, &blackhole, 0,
    "Do not send RST when dropping refused connections");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, aggressive_rcvwnd_inc,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_aggressive_rcvwnd_inc, 1,
    "Be more aggressive about increasing the receive-window.");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, delayed_ack,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_delack_enabled, 3,
    "Delay ACK to try and piggyback it onto a data packet");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, recvbg, CTLFLAG_RW | CTLFLAG_LOCKED,
    int, tcp_recv_bg, 0, "Receive background");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, drop_synfin,
    CTLFLAG_RW | CTLFLAG_LOCKED, static int, drop_synfin, 1,
    "Drop TCP packets with SYN+FIN set");

SYSCTL_NODE(_net_inet_tcp, OID_AUTO, reass, CTLFLAG_RW | CTLFLAG_LOCKED, 0,
    "TCP Segment Reassembly Queue");

static int tcp_reass_overflows = 0;
SYSCTL_INT(_net_inet_tcp_reass, OID_AUTO, overflows,
    CTLFLAG_RD | CTLFLAG_LOCKED, &tcp_reass_overflows, 0,
    "Global number of TCP Segment Reassembly Queue Overflows");


SYSCTL_SKMEM_TCP_INT(OID_AUTO, slowlink_wsize, CTLFLAG_RW | CTLFLAG_LOCKED,
    __private_extern__ int, slowlink_wsize, 8192,
    "Maximum advertised window size for slowlink");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, maxseg_unacked,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, maxseg_unacked, 8,
    "Maximum number of outstanding segments left unacked");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, rfc3465, CTLFLAG_RW | CTLFLAG_LOCKED,
    int, tcp_do_rfc3465, 1, "");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, rfc3465_lim2,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_do_rfc3465_lim2, 1,
    "Appropriate bytes counting w/ L=2*SMSS");

int rtt_samples_per_slot = 20;

int tcp_acc_iaj_high_thresh = ACC_IAJ_HIGH_THRESH;
u_int32_t tcp_autorcvbuf_inc_shift = 3;
SYSCTL_SKMEM_TCP_INT(OID_AUTO, recv_allowed_iaj,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_allowed_iaj, ALLOWED_IAJ,
    "Allowed inter-packet arrival jiter");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, doautorcvbuf,
    CTLFLAG_RW | CTLFLAG_LOCKED, u_int32_t, tcp_do_autorcvbuf, 1,
    "Enable automatic socket buffer tuning");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, autotunereorder,
    CTLFLAG_RW | CTLFLAG_LOCKED, u_int32_t, tcp_autotune_reorder, 1,
    "Enable automatic socket buffer tuning even when reordering is present");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, autorcvbufmax,
    CTLFLAG_RW | CTLFLAG_LOCKED, u_int32_t, tcp_autorcvbuf_max, 2 * 1024 * 1024,
    "Maximum receive socket buffer size");

int tcp_disable_access_to_stats = 1;
SYSCTL_INT(_net_inet_tcp, OID_AUTO, disable_access_to_stats,
    CTLFLAG_RW | CTLFLAG_LOCKED, &tcp_disable_access_to_stats, 0,
    "Disable access to tcpstat");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, challengeack_limit,
    CTLFLAG_RW | CTLFLAG_LOCKED, uint32_t, tcp_challengeack_limit, 10,
    "Maximum number of challenge ACKs per connection per second");

                   
SYSCTL_SKMEM_TCP_INT(OID_AUTO, do_rfc5961,
    CTLFLAG_RW | CTLFLAG_LOCKED, static int, tcp_do_rfc5961, 1,
    "Enable/Disable full RFC 5961 compliance");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, do_better_lr,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_do_better_lr, 1,
    "Improved TCP Loss Recovery");

extern int tcp_acc_iaj_high;
extern int tcp_acc_iaj_react_limit;

int tcprexmtthresh = 3;

u_int32_t tcp_now;
struct timeval tcp_uptime;                                                
lck_spin_t *tcp_uptime_lock;                                               

struct inpcbhead tcb;
#define tcb6    tcb                                     
struct inpcbinfo tcbinfo;

static void tcp_dooptions(struct tcpcb *, u_char *, int, struct tcphdr *,
    struct tcpopt *);
static void tcp_finalize_options(struct tcpcb *, struct tcpopt *, unsigned int);
static void tcp_pulloutofband(struct socket *,
    struct tcphdr *, struct mbuf *, int);
static void tcp_xmit_timer(struct tcpcb *, int, u_int32_t, tcp_seq);
static inline unsigned int tcp_maxmtu(struct rtentry *);
static inline int tcp_stretch_ack_enable(struct tcpcb *tp, int thflags);
static inline void tcp_adaptive_rwtimo_check(struct tcpcb *, int);

#if TRAFFIC_MGT
static inline void update_iaj_state(struct tcpcb *tp, uint32_t tlen,
    int reset_size);
static inline void compute_iaj(struct tcpcb *tp);
static inline void compute_iaj_meat(struct tcpcb *tp, uint32_t cur_iaj);
#endif                  

static inline unsigned int tcp_maxmtu6(struct rtentry *);
unsigned int get_maxmtu(struct rtentry *);

static void tcp_sbrcv_grow(struct tcpcb *tp, struct sockbuf *sb,
    struct tcpopt *to, uint32_t tlen);
void tcp_sbrcv_trim(struct tcpcb *tp, struct sockbuf *sb);
static void tcp_sbsnd_trim(struct sockbuf *sbsnd);
static inline void tcp_sbrcv_tstmp_check(struct tcpcb *tp);
static inline void tcp_sbrcv_reserve(struct tcpcb *tp, struct sockbuf *sb,
    u_int32_t newsize, u_int32_t idealsize, u_int32_t rcvbuf_max);
static void tcp_bad_rexmt_restore_state(struct tcpcb *tp, struct tcphdr *th);
static void tcp_compute_rtt(struct tcpcb *tp, struct tcpopt *to,
    struct tcphdr *th);
static void tcp_early_rexmt_check(struct tcpcb *tp, struct tcphdr *th);
static void tcp_bad_rexmt_check(struct tcpcb *tp, struct tcphdr *th,
    struct tcpopt *to);
  
                                                    
                                    
   
#define TCPTV_RCVNOTS_QUANTUM 100
#define TCP_RCVNOTS_BYTELEVEL 204800

  
                                                
                    
   
#define TCP_EARLY_REXMT_WIN (60 * TCP_RETRANSHZ)                 
#define TCP_EARLY_REXMT_LIMIT 10

#define log_in_vain_log( a ) { log a; }

int tcp_rcvunackwin = TCPTV_UNACKWIN;
int tcp_maxrcvidle = TCPTV_MAXRCVIDLE;
SYSCTL_SKMEM_TCP_INT(OID_AUTO, rcvsspktcnt, CTLFLAG_RW | CTLFLAG_LOCKED,
    int, tcp_rcvsspktcnt, TCP_RCV_SS_PKTCOUNT, "packets to be seen before receiver stretches acks");

#define DELAY_ACK(tp, th) \
	(CC_ALGO(tp)->delay_ack != NULL && CC_ALGO(tp)->delay_ack(tp, th))

static int tcp_dropdropablreq(struct socket *head);
static void tcp_newreno_partial_ack(struct tcpcb *tp, struct tcphdr *th);
static void update_base_rtt(struct tcpcb *tp, uint32_t rtt);
void tcp_set_background_cc(struct socket *so);
void tcp_set_foreground_cc(struct socket *so);
static void tcp_set_new_cc(struct socket *so, uint16_t cc_index);
static void tcp_bwmeas_check(struct tcpcb *tp);

#if TRAFFIC_MGT
void
reset_acc_iaj(struct tcpcb *tp)
{
	tp->acc_iaj = 0;
	CLEAR_IAJ_STATE(tp);
}

static inline void
update_iaj_state(struct tcpcb *tp, uint32_t size, int rst_size)
{
	if (rst_size > 0) {
		tp->iaj_size = 0;
	}
	if (tp->iaj_size == 0 || size >= tp->iaj_size) {
		tp->iaj_size = size;
		tp->iaj_rcv_ts = tcp_now;
		tp->iaj_small_pkt = 0;
	}
}

                                                                    
                                                                           
                                                                       
   
static inline int
isqrt(unsigned int val)
{
	unsigned int sqrt_cache[11] = {0, 1, 4, 9, 16, 25, 36, 49, 64, 81, 100};
	unsigned int temp, g = 0, b = 0x8000, bshft = 15;
	if (val <= 100) {
		for (g = 0; g <= 10; ++g) {
			if (sqrt_cache[g] > val) {
				g--;
				break;
			} else if (sqrt_cache[g] == val) {
				break;
			}
		}
	} else {
		do {
			temp = (((g << 1) + b) << (bshft--));
			if (val >= temp) {
				g += b;
				val -= temp;
			}
			b >>= 1;
		} while (b > 0 && val > 0);
	}
	return g;
}

static inline void
compute_iaj(struct tcpcb *tp)
{
	compute_iaj_meat(tp, (tcp_now - tp->iaj_rcv_ts));
}

static inline void
compute_iaj_meat(struct tcpcb *tp, uint32_t cur_iaj)
{
	                                                            
                                                                   
    
#define MAX_ACC_IAJ (tcp_acc_iaj_high_thresh + tcp_acc_iaj_react_limit)
#define IAJ_DIV_SHIFT 4
#define IAJ_ROUNDUP_CONST (1 << (IAJ_DIV_SHIFT - 1))

	uint32_t allowed_iaj, acc_iaj = 0;

	uint32_t mean, temp;
	int32_t cur_iaj_dev;

	cur_iaj_dev = (cur_iaj - tp->avg_iaj);

	                                                                 
                                                                
                             
    
	allowed_iaj = tp->avg_iaj + tp->std_dev_iaj;
	if (allowed_iaj < tcp_allowed_iaj) {
		allowed_iaj = tcp_allowed_iaj;
	}

	                                                               
                                                             
                                                               
                                                                
                          
    
	if (tp->iaj_pktcnt > IAJ_IGNORE_PKTCNT) {
		if (cur_iaj <= allowed_iaj) {
			if (tp->acc_iaj >= 2) {
				acc_iaj = tp->acc_iaj - 2;
			} else {
				acc_iaj = 0;
			}
		} else {
			acc_iaj = tp->acc_iaj + (cur_iaj - allowed_iaj);
		}

		if (acc_iaj > MAX_ACC_IAJ) {
			acc_iaj = MAX_ACC_IAJ;
		}
		tp->acc_iaj = acc_iaj;
	}

	                                                             
                                                                   
                                                                
   
                                                     
                         
    
	tp->avg_iaj = (((tp->avg_iaj << IAJ_DIV_SHIFT) - tp->avg_iaj)
	    + cur_iaj + IAJ_ROUNDUP_CONST) >> IAJ_DIV_SHIFT;

	                                                                 
                               
    
	temp = tp->std_dev_iaj * tp->std_dev_iaj;
	mean = (((temp << IAJ_DIV_SHIFT) - temp)
	    + (cur_iaj_dev * cur_iaj_dev)
	    + IAJ_ROUNDUP_CONST) >> IAJ_DIV_SHIFT;

	tp->std_dev_iaj = isqrt(mean);

	DTRACE_TCP3(iaj, struct tcpcb *, tp, uint32_t, cur_iaj,
	    uint32_t, allowed_iaj);

	return;
}
#endif                  

  
                                                     
                                                                      
                                                                    
                                                                  
                                                       
   
static boolean_t
tcp_is_ack_ratelimited(struct tcpcb *tp)
{
	boolean_t ret = TRUE;
	uint32_t now = tcp_now;
	int32_t diff = 0;

	diff = timer_diff(now, 0, tp->t_challengeack_last, 0);
	                                        
                                              
                         
    
	if (tp->t_challengeack_last == 0 || diff >= 1000) {
		tp->t_challengeack_last = now;
		tp->t_challengeack_count = 0;
		ret = FALSE;
	} else if (tp->t_challengeack_count < tcp_challengeack_limit) {
		ret = FALSE;
	}

	                               
	if (ret == FALSE && (tp->t_challengeack_count + 1 > 0)) {
		tp->t_challengeack_count++;
	}

	return ret;
}

                                                             
                             
   
static void
tcp_bwmeas_check(struct tcpcb *tp)
{
	int32_t bw_meas_bytes;
	uint32_t bw, bytes, elapsed_time;

	if (SEQ_LEQ(tp->snd_una, tp->t_bwmeas->bw_start)) {
		return;
	}

	bw_meas_bytes = tp->snd_una - tp->t_bwmeas->bw_start;
	if ((tp->t_flagsext & TF_BWMEAS_INPROGRESS) &&
	    bw_meas_bytes >= (int32_t)(tp->t_bwmeas->bw_size)) {
		bytes = bw_meas_bytes;
		elapsed_time = tcp_now - tp->t_bwmeas->bw_ts;
		if (elapsed_time > 0) {
			bw = bytes / elapsed_time;
			if (bw > 0) {
				if (tp->t_bwmeas->bw_sndbw > 0) {
					tp->t_bwmeas->bw_sndbw =
					    (((tp->t_bwmeas->bw_sndbw << 3)
					    - tp->t_bwmeas->bw_sndbw)
					    + bw) >> 3;
				} else {
					tp->t_bwmeas->bw_sndbw = bw;
				}

				                             
				if (tp->t_bwmeas->bw_sndbw_max == 0) {
					tp->t_bwmeas->bw_sndbw_max =
					    tp->t_bwmeas->bw_sndbw;
				} else {
					tp->t_bwmeas->bw_sndbw_max =
					    max(tp->t_bwmeas->bw_sndbw,
					    tp->t_bwmeas->bw_sndbw_max);
				}
			}
		}
		tp->t_flagsext &= ~(TF_BWMEAS_INPROGRESS);
	}
}

static int
tcp_reass(struct tcpcb *tp, struct tcphdr *th, int *tlenp, struct mbuf *m,
    struct ifnet *ifp, int *dowakeup)
{
	struct tseg_qent *q;
	struct tseg_qent *p = NULL;
	struct tseg_qent *nq;
	struct tseg_qent *te = NULL;
	struct inpcb *inp = tp->t_inpcb;
	struct socket *so = inp->inp_socket;
	int flags = 0;
	u_int16_t qlimit;
	boolean_t cell = IFNET_IS_CELLULAR(ifp);
	boolean_t wifi = (!cell && IFNET_IS_WIFI(ifp));
	boolean_t wired = (!wifi && IFNET_IS_WIRED(ifp));
	boolean_t dsack_set = FALSE;

	  
                                               
                                                 
    
	if (th == NULL) {
		goto present;
	}

	  
                                                                  
                                                                   
                                                  
    
	tcp_reset_stretch_ack(tp);
	tp->t_forced_acks = TCP_FORCED_ACKS_COUNT;

#if TRAFFIC_MGT
	if (tp->acc_iaj > 0) {
		reset_acc_iaj(tp);
	}
#endif                  

	if (th->th_seq != tp->rcv_nxt) {
		struct mbuf *tmp = m;
		while (tmp != NULL) {
			if (mbuf_class_under_pressure(tmp)) {
				m_freem(m);
				tcp_reass_overflows++;
				tcpstat.tcps_rcvmemdrop++;
				*tlenp = 0;
				return 0;
			}

			tmp = tmp->m_next;
		}
	}

	  
                                                                   
                                                                    
                                                                  
                                                                  
                                
    
	qlimit = min(max(100, so->so_rcv.sb_hiwat >> 10),
	    (tcp_autorcvbuf_max >> 10));
	if (th->th_seq != tp->rcv_nxt &&
	    (tp->t_reassqlen + 1) >= qlimit) {
		tcp_reass_overflows++;
		tcpstat.tcps_rcvmemdrop++;
		m_freem(m);
		*tlenp = 0;
		return 0;
	}

	                                                                     
	te = (struct tseg_qent *) zalloc(tcp_reass_zone);
	if (te == NULL) {
		tcpstat.tcps_rcvmemdrop++;
		m_freem(m);
		return 0;
	}
	tp->t_reassqlen++;

	  
                                                    
    
	LIST_FOREACH(q, &tp->t_segq, tqe_q) {
		if (SEQ_GT(q->tqe_th->th_seq, th->th_seq)) {
			break;
		}
		p = q;
	}

	  
                                                           
                                                             
                                                      
    
	if (p != NULL) {
		int i;
		                                                     
		i = p->tqe_th->th_seq + p->tqe_len - th->th_seq;
		if (i > 0) {
			if (i > 1) {
				  
                                          
                                
       
				tp->t_dsack_lseq = th->th_seq;
				tp->t_dsack_rseq = th->th_seq +
				    min(i, *tlenp);

				  
                                             
                                              
       
				dsack_set = TRUE;
			}
			if (i >= *tlenp) {
				tcpstat.tcps_rcvduppack++;
				tcpstat.tcps_rcvdupbyte += *tlenp;
				if (nstat_collect) {
					nstat_route_rx(inp->inp_route.ro_rt,
					    1, *tlenp,
					    NSTAT_RX_FLAG_DUPLICATE);
					INP_ADD_STAT(inp, cell, wifi, wired,
					    rxpackets, 1);
					INP_ADD_STAT(inp, cell, wifi, wired,
					    rxbytes, *tlenp);
					tp->t_stat.rxduplicatebytes += *tlenp;
					inp_set_activity_bitmap(inp);
				}
				m_freem(m);
				zfree(tcp_reass_zone, te);
				te = NULL;
				tp->t_reassqlen--;
				  
                                     
                                           
                                     
                 
       
				goto present;
			}
			m_adj(m, i);
			*tlenp -= i;
			th->th_seq += i;
		}
	}
	tp->t_rcvoopack++;
	tcpstat.tcps_rcvoopack++;
	tcpstat.tcps_rcvoobyte += *tlenp;
	if (nstat_collect) {
		nstat_route_rx(inp->inp_route.ro_rt, 1, *tlenp,
		    NSTAT_RX_FLAG_OUT_OF_ORDER);
		INP_ADD_STAT(inp, cell, wifi, wired, rxpackets, 1);
		INP_ADD_STAT(inp, cell, wifi, wired, rxbytes, *tlenp);
		tp->t_stat.rxoutoforderbytes += *tlenp;
		inp_set_activity_bitmap(inp);
	}

	  
                                                      
                                                 
    
	while (q) {
		int i = (th->th_seq + *tlenp) - q->tqe_th->th_seq;
		if (i <= 0) {
			break;
		}

		  
                                                         
                                                    
                                                           
                                  
     
		if (i > 1 && !dsack_set) {
			if (tp->t_dsack_lseq == 0) {
				tp->t_dsack_lseq = q->tqe_th->th_seq;
				tp->t_dsack_rseq =
				    tp->t_dsack_lseq + min(i, q->tqe_len);
			} else {
				  
                                            
                                            
                                         
       
				tp->t_dsack_rseq =
				    tp->t_dsack_rseq + min(i, q->tqe_len);
			}
		}
		if (i < q->tqe_len) {
			q->tqe_th->th_seq += i;
			q->tqe_len -= i;
			m_adj(q->tqe_m, i);
			break;
		}

		nq = LIST_NEXT(q, tqe_q);
		LIST_REMOVE(q, tqe_q);
		m_freem(q->tqe_m);
		zfree(tcp_reass_zone, q);
		tp->t_reassqlen--;
		q = nq;
	}

	                                                    
	te->tqe_m = m;
	te->tqe_th = th;
	te->tqe_len = *tlenp;

	if (p == NULL) {
		LIST_INSERT_HEAD(&tp->t_segq, te, tqe_q);
	} else {
		LIST_INSERT_AFTER(p, te, tqe_q);
	}

present:
	  
                                                   
                             
    
	if (!TCPS_HAVEESTABLISHED(tp->t_state)) {
		return 0;
	}
	q = LIST_FIRST(&tp->t_segq);
	if (!q || q->tqe_th->th_seq != tp->rcv_nxt) {
		return 0;
	}

	  
                                                                
                                                        
                    
    
	if (tp->t_flagsext & TF_REASS_INPROG) {
		return 0;
	}

	tp->t_flagsext |= TF_REASS_INPROG;
	                                                            
	tcpstat.tcps_recovered_pkts++;

	do {
		tp->rcv_nxt += q->tqe_len;
		flags = q->tqe_th->th_flags & TH_FIN;
		LIST_REMOVE(q, tqe_q);
		if (so->so_state & SS_CANTRCVMORE) {
			m_freem(q->tqe_m);
		} else {
			  
                                                          
                                                          
                                      
      
			struct tcphdr saved_tcphdr = {};

			so_recv_data_stat(so, q->tqe_m, 0);           
			memcpy(&saved_tcphdr, th, sizeof(struct tcphdr));

			if (q->tqe_th->th_flags & TH_PUSH) {
				tp->t_flagsext |= TF_LAST_IS_PSH;
			} else {
				tp->t_flagsext &= ~TF_LAST_IS_PSH;
			}

			if (sbappendstream_rcvdemux(so, q->tqe_m)) {
				*dowakeup = 1;
			}
			th = &saved_tcphdr;
		}
		zfree(tcp_reass_zone, q);
		tp->t_reassqlen--;
		q = LIST_FIRST(&tp->t_segq);
	} while (q && q->tqe_th->th_seq == tp->rcv_nxt);
	tp->t_flagsext &= ~TF_REASS_INPROG;

	if ((inp->inp_vflag & INP_IPV6) != 0) {
		KERNEL_DEBUG(DBG_LAYER_BEG,
		    ((inp->inp_fport << 16) | inp->inp_lport),
		    (((inp->in6p_laddr.s6_addr16[0] & 0xffff) << 16) |
		    (inp->in6p_faddr.s6_addr16[0] & 0xffff)),
		    0, 0, 0);
	} else {
		KERNEL_DEBUG(DBG_LAYER_BEG,
		    ((inp->inp_fport << 16) | inp->inp_lport),
		    (((inp->inp_laddr.s_addr & 0xffff) << 16) |
		    (inp->inp_faddr.s_addr & 0xffff)),
		    0, 0, 0);
	}

	return flags;
}

  
                                                                        
                                  
   
static void
tcp_reduce_congestion_window(struct tcpcb *tp)
{
	  
                                    
                                   
                               
    
	if (CC_ALGO(tp)->pre_fr != NULL) {
		CC_ALGO(tp)->pre_fr(tp);
	}
	ENTER_FASTRECOVERY(tp);
	if (tp->t_flags & TF_SENTFIN) {
		tp->snd_recover = tp->snd_max - 1;
	} else {
		tp->snd_recover = tp->snd_max;
	}
	tp->t_timer[TCPT_REXMT] = 0;
	tp->t_timer[TCPT_PTO] = 0;
	tp->t_rtttime = 0;
	if (tp->t_flagsext & TF_CWND_NONVALIDATED) {
		tcp_cc_adjust_nonvalidated_cwnd(tp);
	} else {
		tp->snd_cwnd = tp->snd_ssthresh +
		    tp->t_maxseg * tcprexmtthresh;
	}
}

  
                                                                              
                                                                              
                                                                                 
  
                                                                         
                                                              
  
                                                                  
                                   
   
static inline void
tcp_adaptive_rwtimo_check(struct tcpcb *tp, int tlen)
{
	struct ifnet *outifp = tp->t_inpcb->inp_last_outifp;

	if ((tp->t_adaptive_rtimo > 0 ||
	    (outifp != NULL &&
	    (outifp->if_eflags & IFEF_PROBE_CONNECTIVITY)))
	    && tlen > 0 &&
	    tp->t_state == TCPS_ESTABLISHED) {
		tp->t_timer[TCPT_KEEP] = OFFSET_FROM_START(tp,
		    (TCP_REXMTVAL(tp) << 1));
		tp->t_flagsext |= TF_DETECT_READSTALL;
		tp->t_rtimo_probes = 0;
	}
}

inline void
tcp_keepalive_reset(struct tcpcb *tp)
{
	tp->t_timer[TCPT_KEEP] = OFFSET_FROM_START(tp,
	    TCP_CONN_KEEPIDLE(tp));
	tp->t_flagsext &= ~(TF_DETECT_READSTALL);
	tp->t_rtimo_probes = 0;
}

  
                                                
                                                             
   
int
tcp6_input(struct mbuf **mp, int *offp, int proto)
{
#pragma unused(proto)
	struct mbuf *m = *mp;
	uint32_t ia6_flags;
	struct ifnet *ifp = m->m_pkthdr.rcvif;

	IP6_EXTHDR_CHECK(m, *offp, sizeof(struct tcphdr), return IPPROTO_DONE);

	                                                                  
	MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(m);

	  
                                    
                                
    
	if (ip6_getdstifaddr_info(m, NULL, &ia6_flags) == 0) {
		if (ia6_flags & IN6_IFF_ANYCAST) {
			struct ip6_hdr *ip6;

			ip6 = mtod(m, struct ip6_hdr *);
			icmp6_error(m, ICMP6_DST_UNREACH,
			    ICMP6_DST_UNREACH_ADDR,
			    (caddr_t)&ip6->ip6_dst - (caddr_t)ip6);

			IF_TCP_STATINC(ifp, icmp6unreach);

			return IPPROTO_DONE;
		}
	}

	tcp_input(m, *offp);
	return IPPROTO_DONE;
}

                                                                    
                                                                   
                                                                      
   
u_int8_t
tcp_cansbgrow(struct sockbuf *sb)
{
	                                                                  
                                                                
                   
    
	u_int32_t mblim = ((nmbclusters >> 1) << (MCLSHIFT - MSIZESHIFT));

	                                                                   
                               
    

	u_int32_t sbspacelim = ((nmbclusters >> 4) << MCLSHIFT);

	if ((total_sbmb_cnt < mblim) &&
	    (sb->sb_hiwat < sbspacelim)) {
		return 1;
	} else {
		OSIncrementAtomic64(&sbmb_limreached);
	}
	return 0;
}

static void
tcp_sbrcv_reserve(struct tcpcb *tp, struct sockbuf *sbrcv,
    u_int32_t newsize, u_int32_t idealsize, u_int32_t rcvbuf_max)
{
	                                   
	newsize = min(newsize, rcvbuf_max);

	                                             
                                               
                                   
    
	newsize = min(newsize, TCP_MAXWIN << tp->rcv_scale);

	                                
	if (newsize > sbrcv->sb_hiwat &&
	    (sbreserve(sbrcv, newsize) == 1)) {
		sbrcv->sb_idealsize = min(max(sbrcv->sb_idealsize,
		    (idealsize != 0) ? idealsize : newsize), rcvbuf_max);

		                                              
                 
     
		sbrcv->sb_idealsize = min(sbrcv->sb_idealsize,
		    TCP_MAXWIN << tp->rcv_scale);
	}
}

  
                                                             
                                                           
                                                      
   
static void
tcp_sbrcv_grow(struct tcpcb *tp, struct sockbuf *sbrcv,
    struct tcpopt *to, uint32_t pktlen)
{
	struct socket *so = sbrcv->sb_so;

	  
                                            
                                                           
                                                     
                                                           
             
    
	if (tcp_do_autorcvbuf == 0 ||
	    (sbrcv->sb_flags & SB_AUTOSIZE) == 0 ||
	    tcp_cansbgrow(sbrcv) == 0 ||
	    sbrcv->sb_hiwat >= tcp_autorcvbuf_max ||
	    (tp->t_flagsext & TF_RECV_THROTTLE) ||
	    (so->so_flags1 & SOF1_EXTEND_BK_IDLE_WANTED) ||
	    (!tcp_autotune_reorder && !LIST_EMPTY(&tp->t_segq))) {
		                                                   
		goto out;
	}

	if (!TSTMP_SUPPORTED(tp)) {
		  
                                                          
                                                       
                                                      
                         
     
		if (TSTMP_GEQ(tcp_now,
		    tp->rfbuf_ts + TCPTV_RCVNOTS_QUANTUM)) {
			if (tp->rfbuf_cnt + pktlen >= TCP_RCVNOTS_BYTELEVEL) {
				tcp_sbrcv_reserve(tp, sbrcv,
				    tcp_autorcvbuf_max, 0,
				    tcp_autorcvbuf_max);
			}
			goto out;
		} else {
			tp->rfbuf_cnt += pktlen;
			return;
		}
	} else if (to->to_tsecr != 0) {
		  
                                            
                                        
                                       
                                                   
                                                       
                                                        
                 
     
		if (TSTMP_GEQ(to->to_tsecr, tp->rfbuf_ts)) {
			if (tcp_aggressive_rcvwnd_inc) {
				tp->rfbuf_cnt += pktlen;
			}

			if ((tcp_aggressive_rcvwnd_inc == 0 &&
			    tp->rfbuf_cnt + pktlen > (sbrcv->sb_hiwat -
			    (sbrcv->sb_hiwat >> 1))) ||
			    (tcp_aggressive_rcvwnd_inc &&
			    tp->rfbuf_cnt > tp->rfbuf_space)) {
				int32_t rcvbuf_inc;
				uint32_t idealsize;

				if (tcp_aggressive_rcvwnd_inc == 0) {
					int32_t min_incr;

					tp->rfbuf_cnt += pktlen;
					  
                                         
                                           
                                           
                                              
                                         
       
                                           
                                          
                                              
                                             
        
					rcvbuf_inc = (tp->rfbuf_cnt << 1)
					    - sbrcv->sb_hiwat;

					  
                                              
                
        
					min_incr = tp->t_maxseg << tcp_autorcvbuf_inc_shift;
					if (rcvbuf_inc < min_incr) {
						rcvbuf_inc = min_incr;
					}

					idealsize = (tp->rfbuf_cnt << 1);
				} else {
					if (tp->rfbuf_cnt > tp->rfbuf_space + (tp->rfbuf_space >> 1)) {
						rcvbuf_inc = (tp->rfbuf_cnt << 2) - sbrcv->sb_hiwat;
						idealsize = (tp->rfbuf_cnt << 2);
					} else {
						rcvbuf_inc = (tp->rfbuf_cnt << 1) - sbrcv->sb_hiwat;
						idealsize = (tp->rfbuf_cnt << 1);
					}
				}

				tp->rfbuf_space = tp->rfbuf_cnt;

				if (rcvbuf_inc > 0) {
					rcvbuf_inc =
					    (rcvbuf_inc / tp->t_maxseg) * tp->t_maxseg;

					tcp_sbrcv_reserve(tp, sbrcv,
					    sbrcv->sb_hiwat + rcvbuf_inc,
					    idealsize, tcp_autorcvbuf_max);
				}
			}
			                                             
			if (tp->t_bwmeas != NULL && tp->rfbuf_cnt > 0 &&
			    TSTMP_GT(tcp_now, tp->rfbuf_ts)) {
				u_int32_t rcv_bw;
				rcv_bw = tp->rfbuf_cnt /
				    (int)(tcp_now - tp->rfbuf_ts);
				if (tp->t_bwmeas->bw_rcvbw_max == 0) {
					tp->t_bwmeas->bw_rcvbw_max = rcv_bw;
				} else {
					tp->t_bwmeas->bw_rcvbw_max = max(
						tp->t_bwmeas->bw_rcvbw_max, rcv_bw);
				}
			}
			goto out;
		} else {
			tp->rfbuf_cnt += pktlen;
			return;
		}
	}
out:
	                             
	tp->rfbuf_ts = tcp_now;
	tp->rfbuf_cnt = 0;
	return;
}

                                                                      
                                                                        
                                                                     
                   
   
void
tcp_sbrcv_trim(struct tcpcb *tp, struct sockbuf *sbrcv)
{
	if (tcp_do_autorcvbuf == 1 && sbrcv->sb_idealsize > 0 &&
	    sbrcv->sb_hiwat > sbrcv->sb_idealsize) {
		int32_t trim;
		                                                            
		u_int32_t diff = sbrcv->sb_hiwat - sbrcv->sb_idealsize;

		                                            
                     
     
		u_int32_t advwin = tp->rcv_adv - tp->rcv_nxt;

		                                                  
                                                               
                                                     
                                     
     
		u_int32_t leave = max(advwin, (sbrcv->sb_idealsize >> 4));

		                                                           
                                   
     
		if (leave == 0) {
			leave = tp->t_maxseg << tcp_autorcvbuf_inc_shift;
		}

		trim = sbrcv->sb_hiwat - (sbrcv->sb_cc + leave);
		trim = imin(trim, (int32_t)diff);

		if (trim > 0) {
			sbreserve(sbrcv, (sbrcv->sb_hiwat - trim));
		}
	}
}

                                                                   
                                                                 
                                     
                                                                      
                                                                        
   
void
tcp_sbsnd_trim(struct sockbuf *sbsnd)
{
	if (((sbsnd->sb_flags & (SB_AUTOSIZE | SB_TRIM)) ==
	    (SB_AUTOSIZE | SB_TRIM)) &&
	    (sbsnd->sb_idealsize > 0) &&
	    (sbsnd->sb_hiwat > sbsnd->sb_idealsize)) {
		u_int32_t trim = 0;
		if (sbsnd->sb_cc <= sbsnd->sb_idealsize) {
			trim = sbsnd->sb_hiwat - sbsnd->sb_idealsize;
		} else {
			trim = sbsnd->sb_hiwat - sbsnd->sb_cc;
		}
		sbreserve(sbsnd, (sbsnd->sb_hiwat - trim));
	}
	if (sbsnd->sb_hiwat <= sbsnd->sb_idealsize) {
		sbsnd->sb_flags &= ~(SB_TRIM);
	}
}

  
                                                            
                                                           
                                                            
                                                      
                                                               
   
static inline void
tcp_sbrcv_tstmp_check(struct tcpcb *tp)
{
	struct socket *so = tp->t_inpcb->inp_socket;
	u_int32_t newsize = 2 * tcp_recvspace;
	struct sockbuf *sbrcv = &so->so_rcv;

	if ((tp->t_flags & (TF_REQ_TSTMP | TF_RCVD_TSTMP)) !=
	    (TF_REQ_TSTMP | TF_RCVD_TSTMP) &&
	    (sbrcv->sb_flags & SB_AUTOSIZE) != 0) {
		tcp_sbrcv_reserve(tp, sbrcv, newsize, 0, newsize);
	}
}

                                                               
                                                               
                                                              
                                    
                                                                         
                                 
                                                                    
                                                                     
                                                                        
  
                                                                           
                                                                            
                                                                            
                                                                           
                                                                                
                                                                   
                                                                          
         
   
static inline int
tcp_stretch_ack_enable(struct tcpcb *tp, int thflags)
{
	if (tp->rcv_by_unackwin >= (maxseg_unacked * tp->t_maxseg) &&
	    TSTMP_GEQ(tp->rcv_unackwin, tcp_now)) {
		tp->t_flags |= TF_STREAMING_ON;
	} else {
		tp->t_flags &= ~TF_STREAMING_ON;
	}

	                                                               
	if (TSTMP_GT(tcp_now, tp->rcv_unackwin + tcp_maxrcvidle)) {
		tp->t_flags &= ~TF_STREAMING_ON;
	}

	  
                                                             
             
    
	if (thflags & ~TH_ACK) {
		tp->t_flags &= ~TF_STREAMING_ON;
	}

	if (tp->t_flagsext & TF_DISABLE_STRETCHACK) {
		if (tp->rcv_nostrack_pkts >= TCP_STRETCHACK_ENABLE_PKTCNT) {
			tp->t_flagsext &= ~TF_DISABLE_STRETCHACK;
			tp->rcv_nostrack_pkts = 0;
			tp->rcv_nostrack_ts = 0;
		} else {
			tp->rcv_nostrack_pkts++;
		}
	}

	if (!(tp->t_flagsext & (TF_NOSTRETCHACK | TF_DISABLE_STRETCHACK)) &&
	    (tp->t_flags & TF_STREAMING_ON) &&
	    (!(tp->t_flagsext & TF_RCVUNACK_WAITSS) ||
	    (tp->rcv_waitforss >= tcp_rcvsspktcnt))) {
		return 1;
	}

	return 0;
}

  
                                                                   
                                                                
                                                                    
                                                 
   
void
tcp_reset_stretch_ack(struct tcpcb *tp)
{
	tp->t_flags &= ~(TF_STRETCHACK | TF_STREAMING_ON);
	tp->rcv_by_unackwin = 0;
	tp->rcv_by_unackhalfwin = 0;
	tp->rcv_unackwin = tcp_now + tcp_rcvunackwin;

	  
                                                                 
                                                                    
                                                                 
                                           
    
	tp->t_flagsext |= TF_RCVUNACK_WAITSS;
	tp->rcv_waitforss = 0;
}

  
                                                          
                                                  
  
                                                            
                                                           
                                                           
                                                         
  
                                                              
               
   
int
tcp_detect_bad_rexmt(struct tcpcb *tp, struct tcphdr *th,
    struct tcpopt *to, u_int32_t rxtime)
{
	int32_t tdiff, bad_rexmt_win;
	bad_rexmt_win = (tp->t_srtt >> (TCP_RTT_SHIFT + 1));

	                                                             
	if (TCP_ECN_ENABLED(tp) && (th->th_flags & TH_ECE)) {
		return 0;
	}
	if (TSTMP_SUPPORTED(tp)) {
		if (rxtime > 0 && (to->to_flags & TOF_TS) && to->to_tsecr != 0 &&
		    TSTMP_LT(to->to_tsecr, rxtime)) {
			return 1;
		}
	} else {
		if ((tp->t_rxtshift == 1 || (tp->t_flagsext & TF_SENT_TLPROBE)) &&
		    rxtime > 0) {
			tdiff = (int32_t)(tcp_now - rxtime);
			if (tdiff < bad_rexmt_win) {
				return 1;
			}
		}
	}
	return 0;
}


  
                                                        
                
   
static void
tcp_bad_rexmt_restore_state(struct tcpcb *tp, struct tcphdr *th)
{
	if (TSTMP_SUPPORTED(tp)) {
		u_int32_t fsize, acked;
		fsize = tp->snd_max - th->th_ack;
		acked = BYTES_ACKED(th, tp);

		  
                                         
                           
     
		tp->snd_ssthresh = tp->snd_ssthresh_prev;

		                                           
		if (CC_ALGO(tp)->cwnd_init != NULL) {
			CC_ALGO(tp)->cwnd_init(tp);
		}

		tp->snd_cwnd = fsize + min(acked, tp->snd_cwnd);
	} else {
		tp->snd_cwnd = tp->snd_cwnd_prev;
		tp->snd_ssthresh = tp->snd_ssthresh_prev;
		if (tp->t_flags & TF_WASFRECOVERY) {
			ENTER_FASTRECOVERY(tp);
		}

		                                                  
		tp->t_lossflightsize = 0;
	}
	tp->snd_cwnd = max(tp->snd_cwnd, tcp_initial_cwnd(tp));
	tp->snd_recover = tp->snd_recover_prev;
	tp->snd_nxt = tp->snd_max;

	                                                          
	tcp_bad_rexmt_fix_sndbuf(tp);

	  
                                                  
                                                  
                                                 
                                                  
          
    
	tp->t_flagsext |= TF_RECOMPUTE_RTT;
	tp->t_badrexmt_time = tcp_now;
	tp->t_rtttime = 0;
}

  
                                                                      
                                                                          
                
  
                                                                        
                                                                       
                                             
   
static void
tcp_bad_rexmt_check(struct tcpcb *tp, struct tcphdr *th, struct tcpopt *to)
{
	if (tp->t_rxtshift > 0 &&
	    tcp_detect_bad_rexmt(tp, th, to, tp->t_rxtstart)) {
		++tcpstat.tcps_sndrexmitbad;
		tcp_bad_rexmt_restore_state(tp, th);
		tcp_ccdbg_trace(tp, th, TCP_CC_BAD_REXMT_RECOVERY);
	} else if ((tp->t_flagsext & TF_SENT_TLPROBE) && tp->t_tlphighrxt > 0 &&
	    SEQ_GEQ(th->th_ack, tp->t_tlphighrxt) &&
	    !tcp_detect_bad_rexmt(tp, th, to, tp->t_tlpstart)) {
		  
                                                   
                              
     
		if (tcp_rxtseg_dsack_for_tlp(tp)) {
			  
                                               
                
      
			tcp_rxtseg_clean(tp);
			goto out;
		}

		  
                                                      
                                                    
                            
     
		++tcpstat.tcps_tlp_recoverlastpkt;
		if (!IN_FASTRECOVERY(tp)) {
			tcp_reduce_congestion_window(tp);
			EXIT_FASTRECOVERY(tp);
		}
		tcp_ccdbg_trace(tp, th, TCP_CC_TLP_RECOVER_LASTPACKET);
	} else if (tcp_rxtseg_detect_bad_rexmt(tp, th->th_ack)) {
		  
                                                            
                                                 
     
		tcpstat.tcps_dsack_badrexmt++;
		tcp_bad_rexmt_restore_state(tp, th);
		tcp_ccdbg_trace(tp, th, TCP_CC_DSACK_BAD_REXMT);
		tcp_rxtseg_clean(tp);
	}
out:
	tp->t_flagsext &= ~(TF_SENT_TLPROBE);
	tp->t_tlphighrxt = 0;
	tp->t_tlpstart = 0;

	  
                                                              
                                                              
                                                             
                             
    
	if (tp->t_rxtshift > 0 && (tp->t_flags & TF_BLACKHOLE) &&
	    tp->t_pmtud_start_ts > 0 && TSTMP_SUPPORTED(tp)) {
		if ((to->to_flags & TOF_TS) && to->to_tsecr != 0
		    && TSTMP_LT(to->to_tsecr, tp->t_pmtud_start_ts)) {
			tcp_pmtud_revert_segment_size(tp);
		}
	}
	if (tp->t_pmtud_start_ts > 0) {
		tp->t_pmtud_start_ts = 0;
	}

	tp->t_pmtud_lastseg_size = 0;
}

  
                                                                    
  
                                                                       
                                                                           
                                                              
  
                                                                          
                                                                         
                                                                         
                     
   
static void
tcp_early_rexmt_check(struct tcpcb *tp, struct tcphdr *th)
{
	u_int32_t obytes, snd_off;
	int32_t snd_len;
	struct socket *so = tp->t_inpcb->inp_socket;

	if ((SACK_ENABLED(tp) || tp->t_early_rexmt_count < TCP_EARLY_REXMT_LIMIT) &&
	    SEQ_GT(tp->snd_max, tp->snd_una) &&
	    (tp->t_dupacks == 1 || (SACK_ENABLED(tp) && !TAILQ_EMPTY(&tp->snd_holes)))) {
		  
                                        
                                              
                                            
                                            
                           
     
		if (SACK_ENABLED(tp) &&
		    !TAILQ_EMPTY(&tp->snd_holes)) {
			obytes = (tp->snd_max - tp->snd_fack) +
			    tp->sackhint.sack_bytes_rexmit;
		} else {
			obytes = (tp->snd_max - tp->snd_una);
		}

		  
                                               
                                          
                                              
                      
                                         
                                          
                                     
     
		snd_off = tp->snd_max - tp->snd_una;
		snd_len = min(so->so_snd.sb_cc, tp->snd_wnd) - snd_off;
		if (obytes < (tp->t_maxseg << 2) &&
		    snd_len <= 0) {
			u_int32_t osegs;

			osegs = obytes / tp->t_maxseg;
			if ((osegs * tp->t_maxseg) < obytes) {
				osegs++;
			}

			  
                                             
                                           
                                              
                                       
     
                                            
                                             
                                       
                                          
                                           
      
			osegs += tp->t_dupacks;
			if (osegs < 4) {
				tp->t_rexmtthresh =
				    ((osegs - 1) > 1) ? (osegs - 1) : 1;
				tp->t_rexmtthresh =
				    min(tp->t_rexmtthresh, tcprexmtthresh);
				tp->t_rexmtthresh =
				    max(tp->t_rexmtthresh, tp->t_dupacks);

				if (tp->t_early_rexmt_count == 0) {
					tp->t_early_rexmt_win = tcp_now;
				}

				if (tp->t_flagsext & TF_SENT_TLPROBE) {
					tcpstat.tcps_tlp_recovery++;
					tcp_ccdbg_trace(tp, th,
					    TCP_CC_TLP_RECOVERY);
				} else {
					tcpstat.tcps_early_rexmt++;
					tp->t_early_rexmt_count++;
					tcp_ccdbg_trace(tp, th,
					    TCP_CC_EARLY_RETRANSMIT);
				}
			}
		}
	}

	  
                                                                 
                                                                
                                                         
                                                          
    
	tp->t_flagsext &= ~(TF_SENT_TLPROBE);
	tp->t_tlphighrxt = 0;
	tp->t_tlpstart = 0;
}

static boolean_t
tcp_tfo_syn(struct tcpcb *tp, struct tcpopt *to)
{
	u_char out[CCAES_BLOCK_SIZE];
	unsigned char len;

	if (!(to->to_flags & (TOF_TFO | TOF_TFOREQ)) ||
	    !(tcp_fastopen & TCP_FASTOPEN_SERVER)) {
		return FALSE;
	}

	if ((to->to_flags & TOF_TFOREQ)) {
		tp->t_tfo_flags |= TFO_F_OFFER_COOKIE;

		tp->t_tfo_stats |= TFO_S_COOKIEREQ_RECV;
		tcpstat.tcps_tfo_cookie_req_rcv++;
		return FALSE;
	}

	                                                                      
	tcp_tfo_gen_cookie(tp->t_inpcb, out, sizeof(out));

	len = *to->to_tfo - TCPOLEN_FASTOPEN_REQ;
	to->to_tfo++;
	if (memcmp(out, to->to_tfo, len)) {
		                                                                
		tp->t_tfo_flags |= TFO_F_OFFER_COOKIE;

		tp->t_tfo_stats |= TFO_S_COOKIE_INVALID;
		tcpstat.tcps_tfo_cookie_invalid++;
		return FALSE;
	}

	if (OSIncrementAtomic(&tcp_tfo_halfcnt) >= tcp_tfo_backlog) {
		                                                        
		OSDecrementAtomic(&tcp_tfo_halfcnt);
		return FALSE;
	}

	tp->t_tfo_flags |= TFO_F_COOKIE_VALID;

	tp->t_tfo_stats |= TFO_S_SYNDATA_RCV;
	tcpstat.tcps_tfo_syn_data_rcv++;

	return TRUE;
}

static void
tcp_tfo_synack(struct tcpcb *tp, struct tcpopt *to)
{
	if (to->to_flags & TOF_TFO) {
		unsigned char len = *to->to_tfo - TCPOLEN_FASTOPEN_REQ;

		  
                                                                 
                                        
     
		VERIFY(len <= TFO_COOKIE_LEN_MAX);

		to->to_tfo++;

		tcp_cache_set_cookie(tp, to->to_tfo, len);
		tcp_heuristic_tfo_success(tp);

		tp->t_tfo_stats |= TFO_S_COOKIE_RCV;
		tcpstat.tcps_tfo_cookie_rcv++;
		if (tp->t_tfo_flags & TFO_F_COOKIE_SENT) {
			tcpstat.tcps_tfo_cookie_wrong++;
			tp->t_tfo_stats |= TFO_S_COOKIE_WRONG;
		}
	} else {
		  
                                                                 
                                                              
                                                             
                                    
     
		if (!(tp->t_flagsext & TF_FASTOPEN_FORCE_ENABLE) &&
		    tp->t_tfo_flags & TFO_F_SYN_LOSS) {
			tp->t_tfo_stats |= TFO_S_SYN_LOSS;
			tcpstat.tcps_tfo_syn_loss++;

			tcp_heuristic_tfo_loss(tp);
		} else {
			if (tp->t_tfo_flags & TFO_F_COOKIE_REQ) {
				tp->t_tfo_stats |= TFO_S_NO_COOKIE_RCV;
				tcpstat.tcps_tfo_no_cookie_rcv++;
			}

			tcp_heuristic_tfo_success(tp);
		}
	}
}

static void
tcp_tfo_rcv_probe(struct tcpcb *tp, int tlen)
{
	if (tlen != 0) {
		return;
	}

	tp->t_tfo_probe_state = TFO_PROBE_PROBING;

	  
                                                                     
                                                                        
    
	tp->t_timer[TCPT_KEEP] = OFFSET_FROM_START(tp, (TCP_REXMTVAL(tp)));
}

static void
tcp_tfo_rcv_data(struct tcpcb *tp)
{
	                                                               
	if (tp->t_tfo_probe_state >= TFO_PROBE_PROBING) {
		tp->t_tfo_probe_state = TFO_PROBE_NONE;
	}
}

static void
tcp_tfo_rcv_ack(struct tcpcb *tp, struct tcphdr *th)
{
	if (tp->t_tfo_probe_state == TFO_PROBE_PROBING &&
	    tp->t_tfo_probes > 0) {
		if (th->th_seq == tp->rcv_nxt) {
			                              
			tp->t_tfo_probe_state = TFO_PROBE_NONE;
		} else if (SEQ_GT(th->th_seq, tp->rcv_nxt)) {
			                                             
			tp->t_tfo_probe_state = TFO_PROBE_WAIT_DATA;
			tp->t_timer[TCPT_KEEP] = OFFSET_FROM_START(tp,
			    TCP_REXMTVAL(tp));
		}
	}
}

  
                              
   
static inline bool
tcp_update_window(struct tcpcb *tp, int thflags, struct tcphdr * th,
    u_int32_t tiwin, int tlen)
{
	                                                      
	if ((thflags & TH_ACK) &&
	    (SEQ_LT(tp->snd_wl1, th->th_seq) ||
	    (tp->snd_wl1 == th->th_seq && (SEQ_LT(tp->snd_wl2, th->th_ack) ||
	    (tp->snd_wl2 == th->th_ack && tiwin > tp->snd_wnd))))) {
		                                       
		if (tlen == 0 &&
		    tp->snd_wl2 == th->th_ack && tiwin > tp->snd_wnd) {
			tcpstat.tcps_rcvwinupd++;
		}
		tp->snd_wnd = tiwin;
		tp->snd_wl1 = th->th_seq;
		tp->snd_wl2 = th->th_ack;
		if (tp->snd_wnd > tp->max_sndwnd) {
			tp->max_sndwnd = tp->snd_wnd;
		}

		if (tp->t_inpcb->inp_socket->so_flags & SOF_MP_SUBFLOW) {
			mptcp_update_window_wakeup(tp);
		}
		return true;
	}
	return false;
}

static void
tcp_handle_wakeup(struct socket *so, int read_wakeup, int write_wakeup)
{
	if (read_wakeup != 0) {
		sorwakeup(so);
	}
	if (write_wakeup != 0) {
		sowwakeup(so);
	}
}

static void
tcp_update_snd_una(struct tcpcb *tp, uint32_t ack)
{
	tp->snd_una = ack;
	if (SACK_ENABLED(tp) && SEQ_LT(tp->send_highest_sack, tp->snd_una)) {
		tp->send_highest_sack = tp->snd_una;

		                                                   
		tp->t_new_dupacks = 0;
	}
}

static bool
tcp_syn_data_valid(struct tcpcb *tp, struct tcphdr *tcp_hdr, int tlen)
{
	              
	if (tlen <= 0) {
		return false;
	}

	                                    
	if (tcp_hdr->th_seq != tp->irs) {
		return false;
	}

	                                              
	if (tp->t_inpcb->inp_stat->rxbytes > INT32_MAX) {
		return false;
	}

	return true;
}

void
tcp_input(struct mbuf *m, int off0)
{
	int exiting_fr = 0;
	struct tcphdr *th;
	struct ip *ip = NULL;
	struct inpcb *inp;
	u_char *optp = NULL;
	int optlen = 0;
	int tlen, off;
	int drop_hdrlen;
	struct tcpcb *tp = 0;
	int thflags;
	struct socket *so = 0;
	int todrop, acked, ourfinisacked, needoutput = 0;
	int read_wakeup = 0;
	int write_wakeup = 0;
	struct in_addr laddr;
	struct in6_addr laddr6;
	int dropsocket = 0;
	int iss = 0, nosock = 0;
	u_int32_t tiwin, sack_bytes_acked = 0, sack_bytes_newly_acked = 0;
	struct tcpopt to;                                            
#if TCPDEBUG
	short ostate = 0;
#endif
	u_char ip_ecn = IPTOS_ECN_NOTECT;
	unsigned int ifscope;
	uint8_t isconnected, isdisconnected;
	struct ifnet *ifp = m->m_pkthdr.rcvif;
	int segment_count = m->m_pkthdr.seg_cnt ? : 1;
	int win;
	u_int16_t pf_tag = 0;
#if MPTCP
	struct mptcb *mp_tp = NULL;
#endif            
	boolean_t cell = IFNET_IS_CELLULAR(ifp);
	boolean_t wifi = (!cell && IFNET_IS_WIFI(ifp));
	boolean_t wired = (!wifi && IFNET_IS_WIRED(ifp));
	boolean_t recvd_dsack = FALSE;
	struct tcp_respond_args tra;
	int prev_t_state;
	boolean_t check_cfil = cfil_filter_present();
	bool findpcb_iterated = false;
	  
                                                                       
                                                                       
                               
    
	struct tcphdr saved_tcphdr = {};
	  
                                      
                                                                        
                                
    
#define MAX_IPWORDS ((sizeof(struct ip) + MAX_IPOPTLEN) / sizeof(uint32_t))
	uint32_t saved_hdr[MAX_IPWORDS];

#define TCP_INC_VAR(stat, npkts) do {                   \
	        stat += npkts;                          \
} while (0)

	if (tcp_ack_strategy == TCP_ACK_STRATEGY_LEGACY) {
		segment_count = 1;
	}
	TCP_INC_VAR(tcpstat.tcps_rcvtotal, segment_count);

	struct ip6_hdr *ip6 = NULL;
	int isipv6;
	struct proc *kernel_proc = current_proc();

	KERNEL_DEBUG(DBG_FNC_TCP_INPUT | DBG_FUNC_START, 0, 0, 0, 0, 0);

	isipv6 = (mtod(m, struct ip *)->ip_v == 6) ? 1 : 0;
	bzero((char *)&to, sizeof(to));

	if (m->m_flags & M_PKTHDR) {
		pf_tag = m_pftag(m)->pftag_tag;
	}

	if (isipv6) {
		  
                                          
                           
     
		MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(m);

		                                                        
		ip6 = mtod(m, struct ip6_hdr *);
		tlen = sizeof(*ip6) + ntohs(ip6->ip6_plen) - off0;
		th = (struct tcphdr *)(void *)((caddr_t)ip6 + off0);

		if (tcp_input_checksum(AF_INET6, m, th, off0, tlen)) {
			TCP_LOG_DROP_PKT(ip6, th, ifp, "IPv6 bad tcp checksum");
			goto dropnosock;
		}

		KERNEL_DEBUG(DBG_LAYER_BEG, ((th->th_dport << 16) | th->th_sport),
		    (((ip6->ip6_src.s6_addr16[0]) << 16) | (ip6->ip6_dst.s6_addr16[0])),
		    th->th_seq, th->th_ack, th->th_win);
		  
                                                           
                                                              
                                                        
    
                                                           
                                  
     
		if (IN6_IS_ADDR_UNSPECIFIED(&ip6->ip6_src)) {
			              
			IF_TCP_STATINC(ifp, unspecv6);
			TCP_LOG_DROP_PKT(ip6, th, ifp, "src IPv6 address unspecified");
			goto dropnosock;
		}
		DTRACE_TCP5(receive, struct mbuf *, m, struct inpcb *, NULL,
		    struct ip6_hdr *, ip6, struct tcpcb *, NULL,
		    struct tcphdr *, th);

		ip_ecn = (ntohl(ip6->ip6_flow) >> 20) & IPTOS_ECN_MASK;
	} else {
		  
                                                  
                                             
     
		if (off0 > sizeof(struct ip)) {
			ip_stripoptions(m);
			off0 = sizeof(struct ip);
		}
		if (m->m_len < sizeof(struct tcpiphdr)) {
			if ((m = m_pullup(m, sizeof(struct tcpiphdr))) == 0) {
				tcpstat.tcps_rcvshort++;
				return;
			}
		}

		                                                                  
		MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(m);

		ip = mtod(m, struct ip *);
		th = (struct tcphdr *)(void *)((caddr_t)ip + off0);
		tlen = ip->ip_len;

		if (tcp_input_checksum(AF_INET, m, th, off0, tlen)) {
			TCP_LOG_DROP_PKT(ip, th, ifp, "IPv4 bad tcp checksum");
			goto dropnosock;
		}

		                                               
		ip->ip_v = IPVERSION;
		ip_ecn = (ip->ip_tos & IPTOS_ECN_MASK);

		DTRACE_TCP5(receive, struct mbuf *, m, struct inpcb *, NULL,
		    struct ip *, ip, struct tcpcb *, NULL, struct tcphdr *, th);

		KERNEL_DEBUG(DBG_LAYER_BEG, ((th->th_dport << 16) | th->th_sport),
		    (((ip->ip_src.s_addr & 0xffff) << 16) | (ip->ip_dst.s_addr & 0xffff)),
		    th->th_seq, th->th_ack, th->th_win);
	}

#define TCP_LOG_HDR (isipv6 ? (void *)ip6 : (void *)ip)

	  
                                      
                                           
    
	off = th->th_off << 2;
	if (off < sizeof(struct tcphdr) || off > tlen) {
		tcpstat.tcps_rcvbadoff++;
		IF_TCP_STATINC(ifp, badformat);
		TCP_LOG_DROP_PKT(TCP_LOG_HDR, th, ifp, "bad tcp offset");
		goto dropnosock;
	}
	tlen -= off;                                            
	if (off > sizeof(struct tcphdr)) {
		if (isipv6) {
			IP6_EXTHDR_CHECK(m, off0, off, return );
			ip6 = mtod(m, struct ip6_hdr *);
			th = (struct tcphdr *)(void *)((caddr_t)ip6 + off0);
		} else {
			if (m->m_len < sizeof(struct ip) + off) {
				if ((m = m_pullup(m, sizeof(struct ip) + off)) == 0) {
					tcpstat.tcps_rcvshort++;
					return;
				}
				ip = mtod(m, struct ip *);
				th = (struct tcphdr *)(void *)((caddr_t)ip + off0);
			}
		}
		optlen = off - sizeof(struct tcphdr);
		optp = (u_char *)(th + 1);
		  
                                                      
                                                             
                                                        
                                                      
                          
     
		if ((optlen == TCPOLEN_TSTAMP_APPA ||
		    (optlen > TCPOLEN_TSTAMP_APPA &&
		    optp[TCPOLEN_TSTAMP_APPA] == TCPOPT_EOL)) &&
		    *(u_int32_t *)(void *)optp == htonl(TCPOPT_TSTAMP_HDR) &&
		    (th->th_flags & TH_SYN) == 0) {
			to.to_flags |= TOF_TS;
			to.to_tsval = ntohl(*(u_int32_t *)(void *)(optp + 4));
			to.to_tsecr = ntohl(*(u_int32_t *)(void *)(optp + 8));
			optp = NULL;                                  
		}
	}
	thflags = th->th_flags;

	  
                                                        
                                                              
   
                                                 
    
	if ((thflags & (TH_SYN | TH_FIN)) == (TH_SYN | TH_FIN)) {
		IF_TCP_STATINC(ifp, synfin);
		TCP_LOG_DROP_PKT(TCP_LOG_HDR, th, ifp, "drop SYN FIN");
		goto dropnosock;
	}

	  
                                                                      
                                                                      
                                         
                                                           
                                                               
                               
    
	drop_hdrlen = off0 + off;

	                                                                       
                                  
    
	calculate_tcp_clock();

	  
                                                                     
                                                                     
                                                                  
                                                                    
                           
    
	if ((m->m_pkthdr.pkt_flags & PKTF_LOOP) || m->m_pkthdr.rcvif == NULL) {
		ifscope = IFSCOPE_NONE;
	} else {
		ifscope = m->m_pkthdr.rcvif->if_index;
	}

	  
                                                        
    

#if BYTE_ORDER != BIG_ENDIAN
	NTOHL(th->th_seq);
	NTOHL(th->th_ack);
	NTOHS(th->th_win);
	NTOHS(th->th_urp);
#endif

	  
                           
    
findpcb:

	isconnected = FALSE;
	isdisconnected = FALSE;

	if (isipv6) {
		inp = in6_pcblookup_hash(&tcbinfo, &ip6->ip6_src, th->th_sport,
		    &ip6->ip6_dst, th->th_dport, 1,
		    m->m_pkthdr.rcvif);
	} else {
		inp = in_pcblookup_hash(&tcbinfo, ip->ip_src, th->th_sport,
		    ip->ip_dst, th->th_dport, 1, m->m_pkthdr.rcvif);
	}

	  
                                                                 
                                                                
                                                                  
                           
    
	if (inp != NULL && (inp->inp_flags & INP_BOUND_IF)) {
		ifscope = inp->inp_boundifp->if_index;
	}

	  
                                                          
                                                  
                                                              
                                                    
    
	if (inp == NULL) {
		if (log_in_vain) {
			char dbuf[MAX_IPv6_STR_LEN], sbuf[MAX_IPv6_STR_LEN];

			if (isipv6) {
				inet_ntop(AF_INET6, &ip6->ip6_dst, dbuf, sizeof(dbuf));
				inet_ntop(AF_INET6, &ip6->ip6_src, sbuf, sizeof(sbuf));
			} else {
				inet_ntop(AF_INET, &ip->ip_dst, dbuf, sizeof(dbuf));
				inet_ntop(AF_INET, &ip->ip_src, sbuf, sizeof(sbuf));
			}
			switch (log_in_vain) {
			case 1:
				if (thflags & TH_SYN) {
					log(LOG_INFO,
					    "Connection attempt to TCP %s:%d from %s:%d\n",
					    dbuf, ntohs(th->th_dport),
					    sbuf,
					    ntohs(th->th_sport));
				}
				break;
			case 2:
				log(LOG_INFO,
				    "Connection attempt to TCP %s:%d from %s:%d flags:0x%x\n",
				    dbuf, ntohs(th->th_dport), sbuf,
				    ntohs(th->th_sport), thflags);
				break;
			case 3:
			case 4:
				if ((thflags & TH_SYN) && !(thflags & TH_ACK) &&
				    !(m->m_flags & (M_BCAST | M_MCAST)) &&
				    ((isipv6 && !IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &ip6->ip6_src)) ||
				    (!isipv6 && ip->ip_dst.s_addr != ip->ip_src.s_addr))) {
					log_in_vain_log((LOG_INFO,
					    "Stealth Mode connection attempt to TCP %s:%d from %s:%d\n",
					    dbuf, ntohs(th->th_dport),
					    sbuf,
					    ntohs(th->th_sport)));
				}
				break;
			default:
				break;
			}
		}
		if (blackhole) {
			if (m->m_pkthdr.rcvif && m->m_pkthdr.rcvif->if_type != IFT_LOOP) {
				switch (blackhole) {
				case 1:
					if (thflags & TH_SYN) {
						TCP_LOG_DROP_PKT(TCP_LOG_HDR, th, ifp, "blackhole 1 syn for closed port");
						goto dropnosock;
					}
					break;
				case 2:
					TCP_LOG_DROP_PKT(TCP_LOG_HDR, th, ifp, "blackhole 2 closed port");
					goto dropnosock;
				default:
					TCP_LOG_DROP_PKT(TCP_LOG_HDR, th, ifp, "blackhole closed port");
					goto dropnosock;
				}
			}
		}
		IF_TCP_STATINC(ifp, noconnnolist);
		TCP_LOG_DROP_PKT(TCP_LOG_HDR, th, ifp, "closed port");
		goto dropwithresetnosock;
	}
	so = inp->inp_socket;
	if (so == NULL) {
		                                                              
                                               
                                                                               
                                                                            
     
		inp = NULL;
#if TEMPDEBUG
		printf("tcp_input: no more socket for inp=%x. This shouldn't happen\n", inp);
#endif
		TCP_LOG_DROP_PKT(TCP_LOG_HDR, th, ifp, "inp_socket NULL");
		goto dropnosock;
	}

	socket_lock(so, 1);
	if (in_pcb_checkstate(inp, WNT_RELEASE, 1) == WNT_STOPUSING) {
		socket_unlock(so, 1);
		inp = NULL;                                 
		TCP_LOG_DROP_PKT(TCP_LOG_HDR, th, ifp, "inp state WNT_STOPUSING");
		goto dropnosock;
	}

	if (!isipv6 && inp->inp_faddr.s_addr != INADDR_ANY) {
		if (inp->inp_faddr.s_addr != ip->ip_src.s_addr ||
		    inp->inp_laddr.s_addr != ip->ip_dst.s_addr ||
		    inp->inp_fport != th->th_sport ||
		    inp->inp_lport != th->th_dport) {
			os_log_error(OS_LOG_DEFAULT, "%s 5-tuple does not match: %u:%u %u:%u\n",
			    __func__,
			    ntohs(inp->inp_fport), ntohs(th->th_sport),
			    ntohs(inp->inp_lport), ntohs(th->th_dport));
			if (findpcb_iterated) {
				goto drop;
			}
			findpcb_iterated = true;
			socket_unlock(so, 1);
			inp = NULL;
			goto findpcb;
		}
	} else if (isipv6 && !IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_faddr)) {
		if (!IN6_ARE_ADDR_EQUAL(&inp->in6p_faddr, &ip6->ip6_src) ||
		    !IN6_ARE_ADDR_EQUAL(&inp->in6p_laddr, &ip6->ip6_dst) ||
		    inp->inp_fport != th->th_sport ||
		    inp->inp_lport != th->th_dport) {
			os_log_error(OS_LOG_DEFAULT, "%s 5-tuple does not match: %u:%u %u:%u\n",
			    __func__,
			    ntohs(inp->inp_fport), ntohs(th->th_sport),
			    ntohs(inp->inp_lport), ntohs(th->th_dport));
			if (findpcb_iterated) {
				goto drop;
			}
			findpcb_iterated = true;
			socket_unlock(so, 1);
			inp = NULL;
			goto findpcb;
		}
	}

	tp = intotcpcb(inp);
	if (tp == NULL) {
		IF_TCP_STATINC(ifp, noconnlist);
		TCP_LOG_DROP_PKT(TCP_LOG_HDR, th, ifp, "tp is NULL");
		goto dropwithreset;
	}

	                                                                  
	if (to.to_flags & TOF_TS) {
		to.to_tsecr -= tp->t_ts_offset;
	}

	TCP_LOG_TH_FLAGS(TCP_LOG_HDR, th, tp, false, ifp);

	if (tp->t_state == TCPS_CLOSED) {
		TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "tp state TCPS_CLOSED");
		goto drop;
	}

#if NECP
	if (so->so_state & SS_ISCONNECTED) {
		                                                             
		                                                         
		if (!necp_socket_is_allowed_to_send_recv(inp, ifp, pf_tag, NULL, NULL, NULL, NULL)) {
			TCP_LOG_DROP_NECP(TCP_LOG_HDR, th, intotcpcb(inp), false);
			IF_TCP_STATINC(ifp, badformat);
			goto drop;
		}
	} else {
		  
                                                                      
                                                                       
                                                                   
                                                          
                                                                
                      
                                                                 
                                                               
               
     
		(void) inp_update_policy(inp);

		if (isipv6) {
			if (!necp_socket_is_allowed_to_send_recv_v6(inp,
			    th->th_dport, th->th_sport, &ip6->ip6_dst,
			    &ip6->ip6_src, ifp, pf_tag, NULL, NULL, NULL, NULL)) {
				TCP_LOG_DROP_NECP(TCP_LOG_HDR, th, intotcpcb(inp), false);
				IF_TCP_STATINC(ifp, badformat);
				goto drop;
			}
		} else {
			if (!necp_socket_is_allowed_to_send_recv_v4(inp,
			    th->th_dport, th->th_sport, &ip->ip_dst, &ip->ip_src,
			    ifp, pf_tag, NULL, NULL, NULL, NULL)) {
				TCP_LOG_DROP_NECP(TCP_LOG_HDR, th, intotcpcb(inp), false);
				IF_TCP_STATINC(ifp, badformat);
				goto drop;
			}
		}
	}
#endif           

	prev_t_state = tp->t_state;

	                                                      
	if ((thflags & TH_ACCEPT) == 0) {
		TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "rfc5961 TH_ACCEPT == 0");
		goto drop;
	}

	                                             
	if ((thflags & TH_SYN) == 0) {
		tiwin = th->th_win << tp->snd_scale;
	} else {
		tiwin = th->th_win;
	}

	                                                            
	if (tp->t_state == TCPS_LISTEN &&
	    (so->so_options & SO_ACCEPTCONN) == 0) {
		TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "closing a listening socket");
		goto drop;
	}

	if (so->so_options & (SO_DEBUG | SO_ACCEPTCONN)) {
#if TCPDEBUG
		if (so->so_options & SO_DEBUG) {
			ostate = tp->t_state;
			if (isipv6) {
				bcopy((char *)ip6, (char *)tcp_saveipgen,
				    sizeof(*ip6));
			} else {
				bcopy((char *)ip, (char *)tcp_saveipgen, sizeof(*ip));
			}
			tcp_savetcp = *th;
		}
#endif
		if (so->so_options & SO_ACCEPTCONN) {
			struct tcpcb *tp0 = tp;
			struct socket *so2;
			struct socket *oso;
			struct sockaddr_storage from;
			struct sockaddr_storage to2;
			struct inpcb *oinp = sotoinpcb(so);
			struct ifnet *head_ifscope;
			unsigned int head_nocell, head_recvanyif,
			    head_noexpensive, head_awdl_unrestricted,
			    head_intcoproc_allowed, head_external_port,
			    head_noconstrained;

			                                               
			head_ifscope = (inp->inp_flags & INP_BOUND_IF) ?
			    inp->inp_boundifp : NULL;
			                                                    
			head_nocell = INP_NO_CELLULAR(inp);
			                                               
			head_recvanyif = (inp->inp_flags & INP_RECV_ANYIF);
			                                                     
			head_noexpensive = INP_NO_EXPENSIVE(inp);
			head_noconstrained = INP_NO_CONSTRAINED(inp);
			head_awdl_unrestricted = INP_AWDL_UNRESTRICTED(inp);
			head_intcoproc_allowed = INP_INTCOPROC_ALLOWED(inp);
			head_external_port = (inp->inp_flags2 & INP2_EXTERNAL_PORT);

			  
                                                                       
                                                                   
                                                                       
                                                            
      
			if ((thflags & (TH_RST | TH_ACK | TH_SYN)) != TH_SYN) {
				IF_TCP_STATINC(ifp, listbadsyn);

				if (thflags & TH_RST) {
					TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "SYN with RST");
					goto drop;
				}
				if (thflags & TH_ACK) {
					TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "SYN with ACK");
					tp = NULL;
					tcpstat.tcps_badsyn++;
					goto dropwithreset;
				}

				                                         
				tcpstat.tcps_badsyn++;
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "bad SYN");
				goto drop;
			}
			KERNEL_DEBUG(DBG_FNC_TCP_NEWCONN | DBG_FUNC_START, 0, 0, 0, 0, 0);
			if (th->th_dport == th->th_sport) {
				if (isipv6) {
					if (IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst,
					    &ip6->ip6_src)) {
						TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "bad tuple same port");
						goto drop;
					}
				} else if (ip->ip_dst.s_addr == ip->ip_src.s_addr) {
					TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "bad tuple same IPv4 address");
					goto drop;
				}
			}
			  
                                                       
                                                           
                                  
     
                                                         
                   
      
			if (m->m_flags & (M_BCAST | M_MCAST)) {
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "mbuf M_BCAST | M_MCAST");
				goto drop;
			}
			if (isipv6) {
				if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst) ||
				    IN6_IS_ADDR_MULTICAST(&ip6->ip6_src)) {
					TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "IN6_IS_ADDR_MULTICAST");
					goto drop;
				}
			} else if (IN_MULTICAST(ntohl(ip->ip_dst.s_addr)) ||
			    IN_MULTICAST(ntohl(ip->ip_src.s_addr)) ||
			    ip->ip_src.s_addr == htonl(INADDR_BROADCAST) ||
			    in_broadcast(ip->ip_dst, m->m_pkthdr.rcvif)) {
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "multicast or broadcast address");
				goto drop;
			}


			  
                                         
                                                  
                                                        
                          
                                                   
                                                         
                                                          
                                                     
                       
     
                                                         
                                                       
                
      
			if (isipv6 && !ip6_use_deprecated) {
				uint32_t ia6_flags;

				if (ip6_getdstifaddr_info(m, NULL,
				    &ia6_flags) == 0) {
					if (ia6_flags & IN6_IFF_DEPRECATED) {
						tp = NULL;
						IF_TCP_STATINC(ifp, deprecate6);
						TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "deprecated IPv6 address");
						goto dropwithreset;
					}
				}
			}
			if (so->so_filt || check_cfil) {
				if (isipv6) {
					struct sockaddr_in6     *sin6 = (struct sockaddr_in6*)&from;

					sin6->sin6_len = sizeof(*sin6);
					sin6->sin6_family = AF_INET6;
					sin6->sin6_port = th->th_sport;
					sin6->sin6_flowinfo = 0;
					sin6->sin6_addr = ip6->ip6_src;
					sin6->sin6_scope_id = 0;

					sin6 = (struct sockaddr_in6*)&to2;

					sin6->sin6_len = sizeof(struct sockaddr_in6);
					sin6->sin6_family = AF_INET6;
					sin6->sin6_port = th->th_dport;
					sin6->sin6_flowinfo = 0;
					sin6->sin6_addr = ip6->ip6_dst;
					sin6->sin6_scope_id = 0;
				} else {
					struct sockaddr_in *sin = (struct sockaddr_in*)&from;

					sin->sin_len = sizeof(*sin);
					sin->sin_family = AF_INET;
					sin->sin_port = th->th_sport;
					sin->sin_addr = ip->ip_src;

					sin = (struct sockaddr_in*)&to2;

					sin->sin_len = sizeof(struct sockaddr_in);
					sin->sin_family = AF_INET;
					sin->sin_port = th->th_dport;
					sin->sin_addr = ip->ip_dst;
				}
			}

			if (so->so_filt) {
				so2 = sonewconn(so, 0, (struct sockaddr*)&from);
			} else {
				so2 = sonewconn(so, 0, NULL);
			}
			if (so2 == 0) {
				tcpstat.tcps_listendrop++;
				if (tcp_dropdropablreq(so)) {
					if (so->so_filt) {
						so2 = sonewconn(so, 0, (struct sockaddr*)&from);
					} else {
						so2 = sonewconn(so, 0, NULL);
					}
				}
				if (!so2) {
					TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, " listen drop");
					goto drop;
				}
			}

			                                                  
			inp = (struct inpcb *)so2->so_pcb;
			tp = intotcpcb(inp);

			oso = so;
			socket_unlock(so, 0);                                                      

			so = so2;
			socket_lock(so, 1);
			  
                                          
                                           
                                              
                                             
                                              
                                             
                                                 
                                                 
                                        
      
			dropsocket++;
			  
                                                    
                                                      
                                                    
                                       
      
			if (head_ifscope != NULL) {
				inp->inp_flags |= INP_BOUND_IF;
				inp->inp_boundifp = head_ifscope;
			} else {
				inp->inp_flags &= ~INP_BOUND_IF;
			}
			  
                                         
      
			if (head_nocell) {
				inp_set_nocellular(inp);
			}
			if (head_noexpensive) {
				inp_set_noexpensive(inp);
			}
			if (head_noconstrained) {
				inp_set_noconstrained(inp);
			}
			if (head_awdl_unrestricted) {
				inp_set_awdl_unrestricted(inp);
			}
			if (head_intcoproc_allowed) {
				inp_set_intcoproc_allowed(inp);
			}
			  
                                                
      
			if (head_recvanyif) {
				inp->inp_flags |= INP_RECV_ANYIF;
			} else {
				inp->inp_flags &= ~INP_RECV_ANYIF;
			}

			if (head_external_port) {
				inp->inp_flags2 |= INP2_EXTERNAL_PORT;
			}
			if (isipv6) {
				inp->in6p_laddr = ip6->ip6_dst;
			} else {
				inp->inp_vflag &= ~INP_IPV6;
				inp->inp_vflag |= INP_IPV4;
				inp->inp_laddr = ip->ip_dst;
			}
			inp->inp_lport = th->th_dport;
			if (in_pcbinshash(inp, 0) != 0) {
				  
                                                 
                                     
       
				if (isipv6) {
					inp->in6p_laddr = in6addr_any;
				} else {
					inp->inp_laddr.s_addr = INADDR_ANY;
				}
				inp->inp_lport = 0;
				socket_lock(oso, 0);                               
				socket_unlock(oso, 1);
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, " in_pcbinshash failed");
				goto drop;
			}
			socket_lock(oso, 0);
			if (isipv6) {
				  
                                                
              
                                             
                                             
                                                 
                                                  
                                                  
                 
                                                
                                                
                                      
       
				inp->inp_flags |=
				    oinp->inp_flags & INP_CONTROLOPTS;
				if (oinp->in6p_outputopts) {
					inp->in6p_outputopts =
					    ip6_copypktopts(oinp->in6p_outputopts,
					    M_NOWAIT);
				}
			} else {
				inp->inp_options = ip_srcroute();
				inp->inp_ip_tos = oinp->inp_ip_tos;
			}
#if IPSEC
			                                       
			if (sotoinpcb(oso)->inp_sp) {
				int error = 0;
				                                                                     
				if (inp->inp_sp != NULL) {
					error = ipsec_init_policy(so, &inp->inp_sp);
				}
				if (error != 0 || ipsec_copy_policy(sotoinpcb(oso)->inp_sp, inp->inp_sp)) {
					printf("tcp_input: could not copy policy\n");
				}
			}
#endif
			                                      
			DTRACE_TCP4(state__change, void, NULL, struct inpcb *, inp,
			    struct tcpcb *, tp, int32_t, TCPS_LISTEN);
			tp->t_state = TCPS_LISTEN;
			tp->t_flags |= tp0->t_flags & (TF_NOPUSH | TF_NOOPT | TF_NODELAY);
			tp->t_flagsext |= (tp0->t_flagsext & (TF_RXTFINDROP | TF_NOTIMEWAIT | TF_FASTOPEN));
			tp->t_keepinit = tp0->t_keepinit;
			tp->t_keepcnt = tp0->t_keepcnt;
			tp->t_keepintvl = tp0->t_keepintvl;
			tp->t_adaptive_wtimo = tp0->t_adaptive_wtimo;
			tp->t_adaptive_rtimo = tp0->t_adaptive_rtimo;
			tp->t_inpcb->inp_ip_ttl = tp0->t_inpcb->inp_ip_ttl;
			if ((so->so_flags & SOF_NOTSENT_LOWAT) != 0) {
				tp->t_notsent_lowat = tp0->t_notsent_lowat;
			}
			tp->t_inpcb->inp_flags2 |=
			    tp0->t_inpcb->inp_flags2 & INP2_KEEPALIVE_OFFLOAD;

			                                            
			socket_unlock(oso, 1);

			tcp_set_max_rwinscale(tp, so);

#if CONTENT_FILTER
			if (check_cfil) {
				int error = cfil_sock_attach(so2, (struct sockaddr*)&to2, (struct sockaddr*)&from,
				    CFS_CONNECTION_DIR_IN);
				if (error != 0) {
					TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, " cfil_sock_attach failed");
					goto drop;
				}
			}
#endif                     

			KERNEL_DEBUG(DBG_FNC_TCP_NEWCONN | DBG_FUNC_END, 0, 0, 0, 0, 0);
		}
	}
	socket_lock_assert_owned(so);

	if (net_mpklog_enabled && (m->m_pkthdr.rcvif->if_xflags & IFXF_MPK_LOG)) {
		MPKL_TCP_INPUT(tcp_mpkl_log_object,
		    ntohs(tp->t_inpcb->inp_lport), ntohs(tp->t_inpcb->inp_fport),
		    th->th_seq, th->th_ack, tlen, thflags,
		    so->last_pid, so->so_log_seqn++);
	}

	if (tp->t_state == TCPS_ESTABLISHED && tlen > 0) {
		  
                                                          
                                                          
                                                         
                                                                 
     
		if ((tp->t_flagsext & TF_RCVUNACK_WAITSS) != 0) {
			TCP_INC_VAR(tp->rcv_waitforss, segment_count);
		}
		if (tcp_stretch_ack_enable(tp, thflags)) {
			tp->t_flags |= TF_STRETCHACK;
			tp->t_flagsext &= ~(TF_RCVUNACK_WAITSS);
			tp->rcv_waitforss = 0;
		} else {
			tp->t_flags &= ~(TF_STRETCHACK);
		}
		if (TSTMP_GT(tp->rcv_unackwin - (tcp_rcvunackwin >> 1), tcp_now)) {
			tp->rcv_by_unackhalfwin += (tlen + off);
			tp->rcv_by_unackwin += (tlen + off);
		} else {
			tp->rcv_unackwin = tcp_now + tcp_rcvunackwin;
			tp->rcv_by_unackwin = tp->rcv_by_unackhalfwin + tlen + off;
			tp->rcv_by_unackhalfwin = tlen + off;
		}
	}

	  
                                                                    
                                                        
    
	if (thflags & TH_CWR) {
		tp->ecn_flags &= ~TE_SENDECE;
		tp->t_ecn_recv_cwr++;
	}

	  
                                                                       
                                                 
                                    
                                        
                                      
                                     
   
                                                                        
    
	if (ip_ecn == IPTOS_ECN_CE && tp->t_state == TCPS_ESTABLISHED &&
	    TCP_ECN_ENABLED(tp) && tlen > 0 &&
	    SEQ_GEQ(th->th_seq, tp->last_ack_sent) &&
	    SEQ_LT(th->th_seq, tp->last_ack_sent + tp->rcv_wnd)) {
		tp->t_ecn_recv_ce++;
		tcpstat.tcps_ecn_recv_ce++;
		INP_INC_IFNET_STAT(inp, ecn_recv_ce);
		                                                         
		tp->ecn_flags |= TE_RECV_ECN_CE;
		tp->ecn_flags |= TE_SENDECE;
	}

	  
                                                             
                                                                
                                                                   
                                        
    
	if (tp->t_state == TCPS_ESTABLISHED &&
	    (tp->ecn_flags & TE_SETUPSENT) &&
	    (ip_ecn == IPTOS_ECN_CE || (thflags & TH_CWR))) {
		tcp_reset_stretch_ack(tp);
		tp->t_forced_acks = TCP_FORCED_ACKS_COUNT;
		CLEAR_IAJ_STATE(tp);
	}

	if (ip_ecn == IPTOS_ECN_CE && tp->t_state == TCPS_ESTABLISHED &&
	    !TCP_ECN_ENABLED(tp) && !(tp->ecn_flags & TE_CEHEURI_SET)) {
		tcpstat.tcps_ecn_fallback_ce++;
		tcp_heuristic_ecn_aggressive(tp);
		tp->ecn_flags |= TE_CEHEURI_SET;
	}

	if (tp->t_state == TCPS_ESTABLISHED && TCP_ECN_ENABLED(tp) &&
	    ip_ecn == IPTOS_ECN_CE && !(tp->ecn_flags & TE_CEHEURI_SET)) {
		if (inp->inp_stat->rxpackets < ECN_MIN_CE_PROBES) {
			tp->t_ecn_recv_ce_pkt++;
		} else if (tp->t_ecn_recv_ce_pkt > ECN_MAX_CE_RATIO) {
			tcpstat.tcps_ecn_fallback_ce++;
			tcp_heuristic_ecn_aggressive(tp);
			tp->ecn_flags |= TE_CEHEURI_SET;
			INP_INC_IFNET_STAT(inp, ecn_fallback_ce);
		} else {
			                                                      
                                     
      
			tp->ecn_flags |= TE_CEHEURI_SET;
		}
	}

	                                                                    
	tp->t_rcvtime = tcp_now;

	  
                                   
                                         
    
	if (TCPS_HAVEESTABLISHED(tp->t_state)) {
		tcp_keepalive_reset(tp);

		if (tp->t_mpsub) {
			mptcp_reset_keepalive(tp);
		}
	}

	  
                                           
                                                    
    
	if (tp->t_state != TCPS_LISTEN && optp) {
		tcp_dooptions(tp, optp, optlen, th, &to);
	}
#if MPTCP
	if (tp->t_state != TCPS_LISTEN && (so->so_flags & SOF_MP_SUBFLOW) &&
	    mptcp_input_preproc(tp, m, th, drop_hdrlen) != 0) {
		tp->t_flags |= TF_ACKNOW;
		(void) tcp_output(tp);
		tcp_check_timer_state(tp);
		socket_unlock(so, 1);
		return;
	}
#endif            
	if (tp->t_state == TCPS_SYN_SENT && (thflags & TH_SYN)) {
		if (!(thflags & TH_ACK) ||
		    (SEQ_GT(th->th_ack, tp->iss) &&
		    SEQ_LEQ(th->th_ack, tp->snd_max))) {
			tcp_finalize_options(tp, &to, ifscope);
		}
	}

#if TRAFFIC_MGT
	  
                                                               
                                                               
                                                               
                                                                  
                                                             
                                                                
                                                               
                                                                    
                                      
    
	if (tp->t_state == TCPS_ESTABLISHED &&
	    (thflags & (TH_SYN | TH_FIN | TH_RST | TH_URG | TH_ACK | TH_ECE | TH_PUSH)) == TH_ACK &&
	    ((tp->t_flags & TF_NEEDFIN) == 0) &&
	    ((to.to_flags & TOF_TS) == 0 ||
	    TSTMP_GEQ(to.to_tsval, tp->ts_recent)) &&
	    th->th_seq == tp->rcv_nxt && LIST_EMPTY(&tp->t_segq)) {
		int seg_size = tlen;
		if (tp->iaj_pktcnt <= IAJ_IGNORE_PKTCNT) {
			TCP_INC_VAR(tp->iaj_pktcnt, segment_count);
		}

		if (tp->iaj_size == 0 || seg_size > tp->iaj_size ||
		    (seg_size == tp->iaj_size && tp->iaj_rcv_ts == 0)) {
			  
                                              
                                                   
                                                
      
			update_iaj_state(tp, seg_size, 0);
		} else {
			if (seg_size == tp->iaj_size) {
				  
                                          
                                       
       
				compute_iaj(tp);
			}
			if (seg_size < tp->iaj_size) {
				  
                                               
                                            
                                             
                                               
                                           
                                        
                                             
             
       
				TCP_INC_VAR(tp->iaj_small_pkt, segment_count);
				if (tp->iaj_small_pkt > RESET_IAJ_SIZE_THRESH) {
					update_iaj_state(tp, seg_size, 1);
				} else {
					CLEAR_IAJ_STATE(tp);
				}
			} else {
				update_iaj_state(tp, seg_size, 0);
			}
		}
	} else {
		CLEAR_IAJ_STATE(tp);
	}
#endif                  

	  
                                                     
                                                      
                                                       
                                               
                                                       
                                                     
                                                       
                                                      
                                                  
                                                     
                                                    
                                                          
                                                       
                                                          
                  
    
	if (tp->t_state == TCPS_ESTABLISHED &&
	    !(so->so_state & SS_CANTRCVMORE) &&
	    (thflags & TH_FLAGS) == TH_ACK &&
	    ((tp->t_flags & TF_NEEDFIN) == 0) &&
	    ((to.to_flags & TOF_TS) == 0 ||
	    TSTMP_GEQ(to.to_tsval, tp->ts_recent)) &&
	    th->th_seq == tp->rcv_nxt &&
	    tiwin && tiwin == tp->snd_wnd &&
	    tp->snd_nxt == tp->snd_max) {
		  
                                                              
                          
                                                           
                                                             
     
		if ((to.to_flags & TOF_TS) != 0 &&
		    SEQ_LEQ(th->th_seq, tp->last_ack_sent)) {
			tp->ts_recent_age = tcp_now;
			tp->ts_recent = to.to_tsval;
		}

		if (tlen == 0) {
			if (SEQ_GT(th->th_ack, tp->snd_una) &&
			    SEQ_LEQ(th->th_ack, tp->snd_max) &&
			    tp->snd_cwnd >= tp->snd_ssthresh &&
			    (!IN_FASTRECOVERY(tp) &&
			    ((!(SACK_ENABLED(tp)) &&
			    tp->t_dupacks < tp->t_rexmtthresh) ||
			    (SACK_ENABLED(tp) && to.to_nsacks == 0 &&
			    TAILQ_EMPTY(&tp->snd_holes))))) {
				  
                                               
       
				++tcpstat.tcps_predack;

				tcp_bad_rexmt_check(tp, th, &to);

				                         
				tcp_compute_rtt(tp, &to, th);

				VERIFY(SEQ_GEQ(th->th_ack, tp->snd_una));
				acked = BYTES_ACKED(th, tp);
				tcpstat.tcps_rcvackpack++;
				tcpstat.tcps_rcvackbyte += acked;

				  
                                               
                                      
                                    
                                              
       
				if (CC_ALGO(tp)->congestion_avd != NULL) {
					CC_ALGO(tp)->congestion_avd(tp, th);
				}
				tcp_ccdbg_trace(tp, th, TCP_CC_INSEQ_ACK_RCVD);
				sbdrop(&so->so_snd, acked);
				tcp_sbsnd_trim(&so->so_snd);

				if (SEQ_GT(tp->snd_una, tp->snd_recover) &&
				    SEQ_LEQ(th->th_ack, tp->snd_recover)) {
					tp->snd_recover = th->th_ack - 1;
				}

				tcp_update_snd_una(tp, th->th_ack);

				TCP_RESET_REXMT_STATE(tp);

				  
                                                   
                 
       
				tp->snd_wl2 = th->th_ack;

				if (tp->t_dupacks > 0) {
					tp->t_dupacks = 0;
					tp->t_rexmtthresh = tcprexmtthresh;
					tp->t_new_dupacks = 0;
				}

				tp->sackhint.sack_bytes_acked = 0;

				  
                                              
                                                
                                                 
                                       
                                        
                                        
                                             
       
				if (tp->snd_una == tp->snd_max) {
					tp->t_timer[TCPT_REXMT] = 0;
					tp->t_timer[TCPT_PTO] = 0;
				} else if (tp->t_timer[TCPT_PERSIST] == 0) {
					tp->t_timer[TCPT_REXMT] = OFFSET_FROM_START(tp, tp->t_rxtcur);
				}
				if (!SLIST_EMPTY(&tp->t_rxt_segments) &&
				    !TCP_DSACK_SEQ_IN_WINDOW(tp,
				    tp->t_dsack_lastuna, tp->snd_una)) {
					tcp_rxtseg_clean(tp);
				}

				if ((tp->t_flagsext & TF_MEASURESNDBW) != 0 &&
				    tp->t_bwmeas != NULL) {
					tcp_bwmeas_check(tp);
				}

				write_wakeup = 1;
				if (!SLIST_EMPTY(&tp->t_notify_ack)) {
					tcp_notify_acknowledgement(tp, so);
				}

				if ((so->so_snd.sb_cc) || (tp->t_flags & TF_ACKNOW)) {
					(void) tcp_output(tp);
				}

				tcp_tfo_rcv_ack(tp, th);

				m_freem(m);

				tcp_check_timer_state(tp);

				tcp_handle_wakeup(so, read_wakeup, write_wakeup);

				socket_unlock(so, 1);
				KERNEL_DEBUG(DBG_FNC_TCP_INPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
				return;
			}
		} else if (th->th_ack == tp->snd_una && LIST_EMPTY(&tp->t_segq) &&
		    tlen <= tcp_sbspace(tp)) {
			  
                                             
                                              
                                             
      

			                                           
			if (SACK_ENABLED(tp) && tp->rcv_numsacks) {
				tcp_clean_sackreport(tp);
			}
			++tcpstat.tcps_preddat;
			tp->rcv_nxt += tlen;
			  
                                                     
             
      
			tp->snd_wl1 = th->th_seq;
			  
                                                    
              
      
			tp->rcv_up = tp->rcv_nxt;
			TCP_INC_VAR(tcpstat.tcps_rcvpack, segment_count);
			tcpstat.tcps_rcvbyte += tlen;
			if (nstat_collect) {
				INP_ADD_STAT(inp, cell, wifi, wired,
				    rxpackets, 1);
				INP_ADD_STAT(inp, cell, wifi, wired, rxbytes,
				    tlen);
				inp_set_activity_bitmap(inp);
			}

			  
                                                   
                                                  
                                    
      
			if (tp->t_flags & TF_STREAMING_ON) {
				tcp_compute_rtt(tp, &to, th);
			}

			tcp_sbrcv_grow(tp, &so->so_rcv, &to, tlen);

			  
                                
      
			so_recv_data_stat(so, m, 0);
			m_adj(m, drop_hdrlen);                           

			if (isipv6) {
				memcpy(&saved_hdr, ip6, sizeof(struct ip6_hdr));
				ip6 = (struct ip6_hdr *)&saved_hdr[0];
			} else {
				memcpy(&saved_hdr, ip, ip->ip_hl << 2);
				ip = (struct ip *)&saved_hdr[0];
			}
			memcpy(&saved_tcphdr, th, sizeof(struct tcphdr));

			if (th->th_flags & TH_PUSH) {
				tp->t_flagsext |= TF_LAST_IS_PSH;
			} else {
				tp->t_flagsext &= ~TF_LAST_IS_PSH;
			}

			if (sbappendstream_rcvdemux(so, m)) {
				mptcp_handle_input(so);
				read_wakeup = 1;
			}
			th = &saved_tcphdr;

			if (isipv6) {
				KERNEL_DEBUG(DBG_LAYER_END, ((th->th_dport << 16) | th->th_sport),
				    (((ip6->ip6_src.s6_addr16[0]) << 16) | (ip6->ip6_dst.s6_addr16[0])),
				    th->th_seq, th->th_ack, th->th_win);
			} else {
				KERNEL_DEBUG(DBG_LAYER_END, ((th->th_dport << 16) | th->th_sport),
				    (((ip->ip_src.s_addr & 0xffff) << 16) | (ip->ip_dst.s_addr & 0xffff)),
				    th->th_seq, th->th_ack, th->th_win);
			}
			TCP_INC_VAR(tp->t_unacksegs, segment_count);
			if (DELAY_ACK(tp, th)) {
				if ((tp->t_flags & TF_DELACK) == 0) {
					tp->t_flags |= TF_DELACK;
					tp->t_timer[TCPT_DELACK] = OFFSET_FROM_START(tp, tcp_delack);
				}
			} else {
				tp->t_flags |= TF_ACKNOW;
				tcp_output(tp);
			}

			tcp_adaptive_rwtimo_check(tp, tlen);

			if (tlen > 0) {
				tcp_tfo_rcv_data(tp);
			}

			tcp_check_timer_state(tp);

			tcp_handle_wakeup(so, read_wakeup, write_wakeup);

			socket_unlock(so, 1);
			KERNEL_DEBUG(DBG_FNC_TCP_INPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
			return;
		}
	}

	  
                                                
                                     
                                                   
                                        
    
	socket_lock_assert_owned(so);
	win = tcp_sbspace(tp);
	if (win < 0) {
		win = 0;
	} else {                                       
		if (tp->t_flags & TF_SLOWLINK && slowlink_wsize > 0) {
			win = min(win, slowlink_wsize);
		}
	}
	tp->rcv_wnd = imax(win, (int)(tp->rcv_adv - tp->rcv_nxt));
#if MPTCP
	  
                                                        
                                             
    
	if ((tp->t_mpflags & TMPF_MPTCP_TRUE) && (mp_tp = tptomptp(tp))) {
		socket_lock_assert_owned(mptetoso(mp_tp->mpt_mpte));
		int64_t recwin_conn = (int64_t)(mp_tp->mpt_rcvadv - mp_tp->mpt_rcvnxt);

		VERIFY(recwin_conn < INT32_MAX && recwin_conn > INT32_MIN);
		if (recwin_conn > 0 && tp->rcv_wnd > (uint32_t)recwin_conn) {
			tp->rcv_wnd = (uint32_t)recwin_conn;
			tcpstat.tcps_mp_reducedwin++;
		}
	}
#endif            

	switch (tp->t_state) {
	  
                                                          
                                
                                        
                                                                        
                                                                   
                                                                  
                          
    
	case TCPS_LISTEN: {
		struct sockaddr_in *sin;
		struct sockaddr_in6 *sin6;

		socket_lock_assert_owned(so);

		                                                                 
		tp->t_log_flags = 0;
		tp->t_flagsext &= ~TF_LOGGED_CONN_SUMMARY;

		if (isipv6) {
			MALLOC(sin6, struct sockaddr_in6 *, sizeof *sin6,
			    M_SONAME, M_NOWAIT);
			if (sin6 == NULL) {
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "LISTEN malloc M_SONAME failed");
				goto drop;
			}
			bzero(sin6, sizeof(*sin6));
			sin6->sin6_family = AF_INET6;
			sin6->sin6_len = sizeof(*sin6);
			sin6->sin6_addr = ip6->ip6_src;
			sin6->sin6_port = th->th_sport;
			laddr6 = inp->in6p_laddr;
			if (IN6_IS_ADDR_UNSPECIFIED(&inp->in6p_laddr)) {
				inp->in6p_laddr = ip6->ip6_dst;
			}
			if (in6_pcbconnect(inp, (struct sockaddr *)sin6,
			    kernel_proc)) {
				inp->in6p_laddr = laddr6;
				FREE(sin6, M_SONAME);
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, " LISTEN in6_pcbconnect failed");
				goto drop;
			}
			FREE(sin6, M_SONAME);
		} else {
			socket_lock_assert_owned(so);
			MALLOC(sin, struct sockaddr_in *, sizeof *sin, M_SONAME,
			    M_NOWAIT);
			if (sin == NULL) {
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "LISTEN malloc M_SONAME failed");
				goto drop;
			}
			sin->sin_family = AF_INET;
			sin->sin_len = sizeof(*sin);
			sin->sin_addr = ip->ip_src;
			sin->sin_port = th->th_sport;
			bzero((caddr_t)sin->sin_zero, sizeof(sin->sin_zero));
			laddr = inp->inp_laddr;
			if (inp->inp_laddr.s_addr == INADDR_ANY) {
				inp->inp_laddr = ip->ip_dst;
			}
			if (in_pcbconnect(inp, (struct sockaddr *)sin, kernel_proc,
			    IFSCOPE_NONE, NULL)) {
				inp->inp_laddr = laddr;
				FREE(sin, M_SONAME);
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, " LISTEN in_pcbconnect failed");
				goto drop;
			}
			FREE(sin, M_SONAME);
		}

		tcp_dooptions(tp, optp, optlen, th, &to);
		tcp_finalize_options(tp, &to, ifscope);

		if (tfo_enabled(tp) && tcp_tfo_syn(tp, &to)) {
			isconnected = TRUE;
		}

		if (iss) {
			tp->iss = iss;
		} else {
			tp->iss = tcp_new_isn(tp);
		}
		tp->irs = th->th_seq;
		tcp_sendseqinit(tp);
		tcp_rcvseqinit(tp);
		tp->snd_recover = tp->snd_una;
		  
                                                 
                             
                                    
     
		tp->snd_wnd = tiwin;                             
		tp->max_sndwnd = tp->snd_wnd;
		tp->t_flags |= TF_ACKNOW;
		tp->t_unacksegs = 0;
		DTRACE_TCP4(state__change, void, NULL, struct inpcb *, inp,
		    struct tcpcb *, tp, int32_t, TCPS_SYN_RECEIVED);
		tp->t_state = TCPS_SYN_RECEIVED;
		tp->t_timer[TCPT_KEEP] = OFFSET_FROM_START(tp,
		    TCP_CONN_KEEPINIT(tp));
		tp->t_connect_time = tcp_now;
		dropsocket = 0;                                  

		if (inp->inp_flowhash == 0) {
			inp->inp_flowhash = inp_calc_flowhash(inp);
		}
		                                
		if (inp->inp_flow == 0 &&
		    inp->in6p_flags & IN6P_AUTOFLOWLABEL) {
			inp->inp_flow &= ~IPV6_FLOWLABEL_MASK;
			inp->inp_flow |=
			    (htonl(ip6_randomflowlabel()) & IPV6_FLOWLABEL_MASK);
		}

		                                      
		so->so_flags &= ~(SOF_INCOMP_INPROGRESS);
		tcpstat.tcps_accepts++;
		if ((thflags & (TH_ECE | TH_CWR)) == (TH_ECE | TH_CWR)) {
			                   
			tp->ecn_flags |= (TE_SETUPRECEIVED | TE_SENDIPECT);
		}

		  
                                                   
     
		TCP_LOG_CONNECT(tp, false, 0);

		tcp_add_fsw_flow(tp, ifp);

		goto trimthenstep6;
	}

	  
                                                             
                                        
    
	case TCPS_SYN_RECEIVED:
		if ((thflags & TH_ACK) &&
		    (SEQ_LEQ(th->th_ack, tp->snd_una) ||
		    SEQ_GT(th->th_ack, tp->snd_max))) {
			IF_TCP_STATINC(ifp, ooopacket);
			TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "SYN_RECEIVED bad ACK");
			goto dropwithreset;
		}

		  
                                                     
                                                           
                                                         
                                                           
     
		if ((thflags & TH_SYN) &&
		    (tp->irs == th->th_seq) &&
		    !(to.to_flags & TOF_SCALE)) {
			tp->t_flags &= ~TF_RCVD_SCALE;
		}
		break;

	  
                             
                                                                
                                                    
                                              
                                               
                                      
                                                
                                                                   
                                                
                                                                 
    
	case TCPS_SYN_SENT:
		if ((thflags & TH_ACK) &&
		    (SEQ_LEQ(th->th_ack, tp->iss) ||
		    SEQ_GT(th->th_ack, tp->snd_max))) {
			IF_TCP_STATINC(ifp, ooopacket);
			TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "SYN_SENT bad ACK");
			goto dropwithreset;
		}
		if (thflags & TH_RST) {
			if ((thflags & TH_ACK) != 0) {
				if (tfo_enabled(tp) &&
				    !(tp->t_flagsext & TF_FASTOPEN_FORCE_ENABLE)) {
					tcp_heuristic_tfo_rst(tp);
				}
				if ((tp->ecn_flags & (TE_SETUPSENT | TE_RCVD_SYN_RST)) == TE_SETUPSENT) {
					  
                                  
                                   
                               
        
					if (tp->t_flags & TF_LOCAL) {
						tp->ecn_flags |= TE_RCVD_SYN_RST;
						goto drop;
					} else {
						tcp_heuristic_ecn_synrst(tp);
					}
				}
				soevent(so,
				    (SO_FILT_HINT_LOCKED |
				    SO_FILT_HINT_CONNRESET));
				tp = tcp_drop(tp, ECONNREFUSED);
			}
			TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "SYN_SENT got RST");
			goto drop;
		}
		if ((thflags & TH_SYN) == 0) {
			TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "SYN_SENT no SYN");
			goto drop;
		}
		tp->snd_wnd = th->th_win;                                
		tp->max_sndwnd = tp->snd_wnd;

		tp->irs = th->th_seq;
		tcp_rcvseqinit(tp);
		if (thflags & TH_ACK) {
			tcpstat.tcps_connects++;

			if ((thflags & (TH_ECE | TH_CWR)) == (TH_ECE)) {
				                       
				tp->ecn_flags |= TE_SETUPRECEIVED;
				if (TCP_ECN_ENABLED(tp)) {
					tcp_heuristic_ecn_success(tp);
					tcpstat.tcps_ecn_client_success++;
				}
			} else {
				if (tp->ecn_flags & TE_SETUPSENT &&
				    tp->t_rxtshift == 0) {
					tcp_heuristic_ecn_success(tp);
					tcpstat.tcps_ecn_not_supported++;
				}
				if (tp->ecn_flags & TE_SETUPSENT &&
				    tp->t_rxtshift > 0) {
					tcp_heuristic_ecn_loss(tp);
				}

				                           
				tp->ecn_flags &= ~TE_SENDIPECT;
			}

			                                           
			if (TCP_WINDOW_SCALE_ENABLED(tp)) {
				tp->snd_scale = tp->requested_s_scale;
				tp->rcv_scale = tp->request_r_scale;
			}

			tp->rcv_adv += min(tp->rcv_wnd, TCP_MAXWIN << tp->rcv_scale);
			tp->snd_una++;                            
			if (SEQ_LT(tp->snd_nxt, tp->snd_una)) {
				tp->snd_nxt = tp->snd_una;
			}

			  
                                                     
                        
                                                          
                                   
      
			if (SEQ_GT(tp->snd_nxt, th->th_ack)) {
				  
                              
                                                 
                                     
       
				if (!(tp->t_flagsext & TF_FASTOPEN_FORCE_ENABLE) &&
				    (tp->t_tfo_stats & TFO_S_SYN_DATA_SENT) &&
				    tp->snd_max == th->th_ack + 1 &&
				    tp->snd_max > tp->snd_una + 1) {
					tcp_heuristic_tfo_middlebox(tp);

					so->so_error = ENODATA;
					soevent(so,
					    (SO_FILT_HINT_LOCKED | SO_FILT_HINT_MP_SUB_ERROR));

					tp->t_tfo_stats |= TFO_S_ONE_BYTE_PROXY;
				}

				tp->snd_max = tp->snd_nxt = th->th_ack;
			}

			  
                                                       
                                     
      
			TCP_INC_VAR(tp->t_unacksegs, segment_count);
			if (DELAY_ACK(tp, th) && tlen != 0) {
				if ((tp->t_flags & TF_DELACK) == 0) {
					tp->t_flags |= TF_DELACK;
					tp->t_timer[TCPT_DELACK] = OFFSET_FROM_START(tp, tcp_delack);
				}
			} else {
				tp->t_flags |= TF_ACKNOW;
			}
			  
                                              
                  
                               
                              
      
			tp->t_starttime = tcp_now;
			tcp_sbrcv_tstmp_check(tp);
			if (tp->t_flags & TF_NEEDFIN) {
				DTRACE_TCP4(state__change, void, NULL,
				    struct inpcb *, inp,
				    struct tcpcb *, tp, int32_t,
				    TCPS_FIN_WAIT_1);
				tp->t_state = TCPS_FIN_WAIT_1;
				tp->t_flags &= ~TF_NEEDFIN;
				thflags &= ~TH_SYN;

				TCP_LOG_CONNECTION_SUMMARY(tp);
			} else {
				DTRACE_TCP4(state__change, void, NULL,
				    struct inpcb *, inp, struct tcpcb *,
				    tp, int32_t, TCPS_ESTABLISHED);
				tp->t_state = TCPS_ESTABLISHED;
				tp->t_timer[TCPT_KEEP] =
				    OFFSET_FROM_START(tp,
				    TCP_CONN_KEEPIDLE(tp));
				if (nstat_collect) {
					nstat_route_connect_success(
						inp->inp_route.ro_rt);
				}
				  
                                             
                                        
                                        
       
				inp_count_sndbytes(inp, th->th_ack);
			}
			tp->t_forced_acks = TCP_FORCED_ACKS_COUNT;
#if MPTCP
			  
                                                         
                                                     
      
			if ((!(tp->t_mpflags & TMPF_MPTCP_TRUE)) &&
			    (tp->t_mpflags & TMPF_SENT_JOIN)) {
				isconnected = FALSE;
			} else
#endif            
			isconnected = TRUE;

			if ((tp->t_tfo_flags & (TFO_F_COOKIE_REQ | TFO_F_COOKIE_SENT)) ||
			    (tp->t_tfo_stats & TFO_S_SYN_DATA_SENT)) {
				tcp_tfo_synack(tp, &to);

				if ((tp->t_tfo_stats & TFO_S_SYN_DATA_SENT) &&
				    SEQ_LT(tp->snd_una, th->th_ack)) {
					tp->t_tfo_stats |= TFO_S_SYN_DATA_ACKED;
					tcpstat.tcps_tfo_syn_data_acked++;
#if MPTCP
					if (so->so_flags & SOF_MP_SUBFLOW) {
						so->so_flags1 |= SOF1_TFO_REWIND;
					}
#endif
					tcp_tfo_rcv_probe(tp, tlen);
				}
			}
		} else {
			  
                                                          
                    
                          
                                     
                                       
      
			tp->t_flags |= TF_ACKNOW;
			tp->t_timer[TCPT_REXMT] = 0;
			DTRACE_TCP4(state__change, void, NULL, struct inpcb *, inp,
			    struct tcpcb *, tp, int32_t, TCPS_SYN_RECEIVED);
			tp->t_state = TCPS_SYN_RECEIVED;

			  
                                                       
                                                       
                          
      
			tcp_disable_tfo(tp);
		}

trimthenstep6:
		  
                                                         
                                         
                               
     
		th->th_seq++;
		if (tlen > tp->rcv_wnd) {
			todrop = tlen - tp->rcv_wnd;
			m_adj(m, -todrop);
			tlen = tp->rcv_wnd;
			thflags &= ~TH_FIN;
			tcpstat.tcps_rcvpackafterwin++;
			tcpstat.tcps_rcvbyteafterwin += todrop;
		}
		tp->snd_wl1 = th->th_seq - 1;
		tp->rcv_up = th->th_seq;
		  
                                                            
                                                        
                                                              
                                                         
                             
     
		if (thflags & TH_ACK) {
			goto process_ACK;
		}
		goto step6;
	  
                                                     
                              
   
                                                               
    
	case TCPS_LAST_ACK:
	case TCPS_CLOSING:
	case TCPS_TIME_WAIT:
		break;                                  

	                                                          
                                                                  
                                                                     
                                                                      
                                           
    
	case TCPS_ESTABLISHED:
		if (thflags & TH_SYN && tlen <= 0) {
			                                                           
			if (tcp_is_ack_ratelimited(tp)) {
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "ESTABLISHED rfc5961 rate limited");
				goto drop;
			} else {
				                        
				tcpstat.tcps_synchallenge++;
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "ESTABLISHED rfc5961 challenge ACK");
				goto dropafterack;
			}
		}
		break;
	}

	  
                                         
                                                                     
                                                                   
                                                                
                                                                 
                                                             
                                                                 
                          
                                     
                                                
                                                             
                                                      
                                                           
   
   
                                                           
                                     
                    
                                                             
                                                             
                                                    
                                                          
                                                              
                                                             
                                                                 
                                                                 
                                                             
                                                                  
                                                                 
                                                                  
                                                               
                                                                
                                                             
                                                              
                                                          
                                                              
                                                                
                                                      
                                                            
                                                                  
                                      
                                                              
                                               
                                                
   
                                                                 
   
                 
                                                                  
                                                                  
                                                               
                 
   
                                                                
              
                          
                                            
                                                            
                                                              
                                                         
                                
                  
                       
                                                      
                  
   
                                                                      
                                                                       
                                                                        
                                                                 
   
    
	if (thflags & TH_RST) {
		if ((SEQ_GEQ(th->th_seq, tp->last_ack_sent) &&
		    SEQ_LT(th->th_seq, tp->last_ack_sent + tp->rcv_wnd)) ||
		    (tp->rcv_wnd == 0 &&
		    ((tp->last_ack_sent == th->th_seq) ||
		    ((tp->last_ack_sent - 1) == th->th_seq)))) {
			if (tp->last_ack_sent == th->th_seq) {
				switch (tp->t_state) {
				case TCPS_SYN_RECEIVED:
					IF_TCP_STATINC(ifp, rstinsynrcv);
					so->so_error = ECONNREFUSED;
					goto close;

				case TCPS_ESTABLISHED:
					if (TCP_ECN_ENABLED(tp) &&
					    tp->snd_una == tp->iss + 1 &&
					    SEQ_GT(tp->snd_max, tp->snd_una)) {
						  
                                       
                                       
                                
         
						tcp_heuristic_ecn_droprst(tp);
					}
					OS_FALLTHROUGH;
				case TCPS_FIN_WAIT_1:
				case TCPS_CLOSE_WAIT:
				case TCPS_FIN_WAIT_2:
					so->so_error = ECONNRESET;
close:
					soevent(so,
					    (SO_FILT_HINT_LOCKED |
					    SO_FILT_HINT_CONNRESET));

					tcpstat.tcps_drops++;
					tp = tcp_close(tp);
					break;

				case TCPS_CLOSING:
				case TCPS_LAST_ACK:
					tp = tcp_close(tp);
					break;

				case TCPS_TIME_WAIT:
					break;
				}
			} else {
				tcpstat.tcps_badrst++;
				                                           
				if (tcp_is_ack_ratelimited(tp)) {
					TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "ESTABLISHED rfc5961 rate limited");
					goto drop;
				} else {
					                        
					tcpstat.tcps_rstchallenge++;
					TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "ESTABLISHED rfc5961 challenge ACK");
					goto dropafterack;
				}
			}
		}
		goto drop;
	}

	  
                                                               
                                          
    
	if ((to.to_flags & TOF_TS) != 0 && tp->ts_recent &&
	    TSTMP_LT(to.to_tsval, tp->ts_recent)) {
		                                                     
		if ((int)(tcp_now - tp->ts_recent_age) > TCP_PAWS_IDLE) {
			  
                                                    
                                                          
                                                      
                                                 
                                                      
                                                 
                                                    
                                                       
                                    
      
			tp->ts_recent = 0;
		} else {
			tcpstat.tcps_rcvduppack++;
			tcpstat.tcps_rcvdupbyte += tlen;
			tp->t_pawsdrop++;
			tcpstat.tcps_pawsdrop++;

			  
                                                      
                                                         
                                           
     
                                                          
                                              
      
			if (TCP_ECN_ENABLED(tp) &&
			    inp->inp_stat->rxbytes < 2147483648) {
				INP_INC_IFNET_STAT(inp, ecn_fallback_reorder);
				tcpstat.tcps_ecn_fallback_reorder++;
				tcp_heuristic_ecn_aggressive(tp);
			}

			if (nstat_collect) {
				nstat_route_rx(tp->t_inpcb->inp_route.ro_rt,
				    1, tlen, NSTAT_RX_FLAG_DUPLICATE);
				INP_ADD_STAT(inp, cell, wifi, wired,
				    rxpackets, 1);
				INP_ADD_STAT(inp, cell, wifi, wired,
				    rxbytes, tlen);
				tp->t_stat.rxduplicatebytes += tlen;
				inp_set_activity_bitmap(inp);
			}
			if (tlen > 0) {
				goto dropafterack;
			}
			goto drop;
		}
	}

	  
                                                                  
                                                               
                                                               
                                                                
                              
    
	if (tp->t_state == TCPS_SYN_RECEIVED && SEQ_LT(th->th_seq, tp->irs)) {
		IF_TCP_STATINC(ifp, dospacket);
		TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "SYN_RECEIVED bad SEQ");
		goto dropwithreset;
	}

	  
                                                             
                                              
    
	todrop = tp->rcv_nxt - th->th_seq;
	if (todrop > 0) {
		boolean_t is_syn_set = FALSE;

		if (thflags & TH_SYN) {
			is_syn_set = TRUE;
			thflags &= ~TH_SYN;
			th->th_seq++;
			if (th->th_urp > 1) {
				th->th_urp--;
			} else {
				thflags &= ~TH_URG;
			}
			todrop--;
		}
		  
                                                         
                                                          
                                                             
     
		if (todrop > tlen
		    || (todrop == tlen && (thflags & TH_FIN) == 0)) {
			  
                                                      
                                                      
                           
      
			thflags &= ~TH_FIN;

			  
                                                     
                                            
     
                                                       
                                                         
      
			if (is_syn_set) {
				if (!tcp_is_ack_ratelimited(tp)) {
					tcpstat.tcps_synchallenge++;
					tp->t_flags |= TF_ACKNOW;
				}
			} else {
				tp->t_flags |= TF_ACKNOW;
			}

			if (todrop == 1) {
				                               
				soevent(so, SO_FILT_HINT_LOCKED |
				    SO_FILT_HINT_KEEPALIVE);
			}
			todrop = tlen;
			tcpstat.tcps_rcvduppack++;
			tcpstat.tcps_rcvdupbyte += todrop;
		} else {
			tcpstat.tcps_rcvpartduppack++;
			tcpstat.tcps_rcvpartdupbyte += todrop;
		}

		if (todrop > 1) {
			  
                                                    
                                         
      
			tp->t_dsack_lseq = th->th_seq;
			tp->t_dsack_rseq = th->th_seq + todrop;
			tp->t_flags |= TF_ACKNOW;
		}
		if (nstat_collect) {
			nstat_route_rx(tp->t_inpcb->inp_route.ro_rt, 1,
			    todrop, NSTAT_RX_FLAG_DUPLICATE);
			INP_ADD_STAT(inp, cell, wifi, wired, rxpackets, 1);
			INP_ADD_STAT(inp, cell, wifi, wired, rxbytes, todrop);
			tp->t_stat.rxduplicatebytes += todrop;
			inp_set_activity_bitmap(inp);
		}
		drop_hdrlen += todrop;                                    
		th->th_seq += todrop;
		tlen -= todrop;
		if (th->th_urp > todrop) {
			th->th_urp -= todrop;
		} else {
			thflags &= ~TH_URG;
			th->th_urp = 0;
		}
	}

	  
                                                           
                                               
                                                               
                                            
                                                               
                                                                
                                              
    
	if (tlen) {
		boolean_t close_it = FALSE;

		if (!(so->so_flags & SOF_MP_SUBFLOW) && (so->so_state & SS_NOFDREF) &&
		    tp->t_state > TCPS_CLOSE_WAIT) {
			TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "SS_NOFDREF");
			close_it = TRUE;
		}

		if ((so->so_flags & SOF_MP_SUBFLOW) && (mptetoso(tptomptp(tp)->mpt_mpte)->so_state & SS_NOFDREF) &&
		    tp->t_state > TCPS_CLOSE_WAIT) {
			TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "SOF_MP_SUBFLOW SS_NOFDREF");
			close_it = TRUE;
		}

		if ((so->so_flags & SOF_DEFUNCT) && tp->t_state > TCPS_FIN_WAIT_1) {
			TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "SOF_DEFUNCT");
			close_it = TRUE;
		}

		if (so->so_state & SS_CANTRCVMORE) {
			TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "SS_CANTRCVMORE");
			close_it = TRUE;
		}

		if (close_it) {
			tp = tcp_close(tp);
			tcpstat.tcps_rcvafterclose++;
			IF_TCP_STATINC(ifp, cleanup);
			goto dropwithreset;
		}
	}

	  
                                                    
                                                  
    
	todrop = (th->th_seq + tlen) - (tp->rcv_nxt + tp->rcv_wnd);
	if (todrop > 0) {
		tcpstat.tcps_rcvpackafterwin++;
		if (todrop >= tlen) {
			tcpstat.tcps_rcvbyteafterwin += tlen;
			  
                                             
                                                 
                                            
                                  
      
			if (thflags & TH_SYN &&
			    tp->t_state == TCPS_TIME_WAIT &&
			    SEQ_GT(th->th_seq, tp->rcv_nxt)) {
				iss = tcp_new_isn(tp);
				tp = tcp_close(tp);
				socket_unlock(so, 1);
				goto findpcb;
			}
			  
                                                   
                                                      
                                                  
                                               
              
      
			if (tp->rcv_wnd == 0 && th->th_seq == tp->rcv_nxt) {
				tp->t_flags |= TF_ACKNOW;
				tcpstat.tcps_rcvwinprobe++;
			} else {
				goto dropafterack;
			}
		} else {
			tcpstat.tcps_rcvbyteafterwin += todrop;
		}
		m_adj(m, -todrop);
		tlen -= todrop;
		thflags &= ~(TH_PUSH | TH_FIN);
	}

	  
                                                             
                         
         
                                                             
                                                               
                                                             
                                                             
                                                        
                                                      
                                             
                           
                                             
                                                          
                                                             
                                                              
                                                   
    
	if ((to.to_flags & TOF_TS) != 0 &&
	    SEQ_LEQ(th->th_seq, tp->last_ack_sent) &&
	    SEQ_LEQ(tp->last_ack_sent, th->th_seq + tlen +
	    ((thflags & (TH_SYN | TH_FIN)) != 0))) {
		tp->ts_recent_age = tcp_now;
		tp->ts_recent = to.to_tsval;
	}

	  
                                                       
                                                     
   
                        
                                                        
                                        
    
	if (thflags & TH_SYN) {
		if (!tcp_syn_data_valid(tp, th, tlen)) {
			tcpstat.tcps_badsyn++;
			                                       
			if (tcp_is_ack_ratelimited(tp)) {
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "rfc5961 bad SYN rate limited");
				goto drop;
			} else {
				                        
				tcpstat.tcps_synchallenge++;
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "rfc5961 bad SYN challenge ack");
				goto dropafterack;
			}
		} else {
			  
                                    
                                                     
      
			th->th_seq++;
			thflags &= ~TH_SYN;
		}
	}

	  
                                                               
                                                             
                                                   
    
	if ((thflags & TH_ACK) == 0) {
		if (tp->t_state == TCPS_SYN_RECEIVED) {
			if ((tfo_enabled(tp))) {
				  
                                               
                    
                                                  
                                                 
                                              
                                                   
                                               
                                                
                                              
                                                
               
       
				tp->snd_nxt = tp->snd_una;

				  
                                                  
                                                   
       
				if (th->th_flags & TH_SYN) {
					needoutput = 1;
				}
			}

			goto step6;
		} else if (tp->t_flags & TF_ACKNOW) {
			TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "bad ACK");
			goto dropafterack;
		} else {
			TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "bad ACK");
			goto drop;
		}
	}

	  
                   
    

	switch (tp->t_state) {
	  
                                                         
                                              
                              
    
	case TCPS_SYN_RECEIVED:

		tcpstat.tcps_connects++;

		                        
		if (TCP_WINDOW_SCALE_ENABLED(tp)) {
			tp->snd_scale = tp->requested_s_scale;
			tp->rcv_scale = tp->request_r_scale;
			tp->snd_wnd = th->th_win << tp->snd_scale;
			tp->max_sndwnd = tp->snd_wnd;
			tiwin = tp->snd_wnd;
		}
		  
                      
                                      
                                     
     
		tp->t_starttime = tcp_now;
		tcp_sbrcv_tstmp_check(tp);
		if (tp->t_flags & TF_NEEDFIN) {
			DTRACE_TCP4(state__change, void, NULL,
			    struct inpcb *, inp,
			    struct tcpcb *, tp, int32_t, TCPS_FIN_WAIT_1);
			tp->t_state = TCPS_FIN_WAIT_1;
			tp->t_flags &= ~TF_NEEDFIN;

			TCP_LOG_CONNECTION_SUMMARY(tp);
		} else {
			DTRACE_TCP4(state__change, void, NULL,
			    struct inpcb *, inp,
			    struct tcpcb *, tp, int32_t, TCPS_ESTABLISHED);
			tp->t_state = TCPS_ESTABLISHED;
			tp->t_timer[TCPT_KEEP] = OFFSET_FROM_START(tp,
			    TCP_CONN_KEEPIDLE(tp));
			if (nstat_collect) {
				nstat_route_connect_success(
					tp->t_inpcb->inp_route.ro_rt);
			}

			  
                                                    
                                              
                        
      
			inp_count_sndbytes(inp, th->th_ack);
		}
		tp->t_forced_acks = TCP_FORCED_ACKS_COUNT;
		  
                                                           
                                                          
     
		if (tlen == 0 && (thflags & TH_FIN) == 0) {
			if (isipv6) {
				memcpy(&saved_hdr, ip6, sizeof(struct ip6_hdr));
				ip6 = (struct ip6_hdr *)&saved_hdr[0];
			} else {
				memcpy(&saved_hdr, ip, ip->ip_hl << 2);
				ip = (struct ip *)&saved_hdr[0];
			}
			memcpy(&saved_tcphdr, th, sizeof(struct tcphdr));
			(void) tcp_reass(tp, (struct tcphdr *)0, &tlen,
			    NULL, ifp, &read_wakeup);
			th = &saved_tcphdr;
		}
		tp->snd_wl1 = th->th_seq - 1;

#if MPTCP
		  
                                                                 
                                           
     
		if ((!(tp->t_mpflags & TMPF_MPTCP_TRUE)) &&
		    (tp->t_mpflags & TMPF_SENT_JOIN)) {
			isconnected = FALSE;
		} else
#endif            
		isconnected = TRUE;
		if ((tp->t_tfo_flags & TFO_F_COOKIE_VALID)) {
			                                      
			isconnected = FALSE;

			OSDecrementAtomic(&tcp_tfo_halfcnt);

			                                                 
			VERIFY(tcp_tfo_halfcnt >= 0);

			tp->t_tfo_flags &= ~TFO_F_COOKIE_VALID;
		}

		  
                                                                
                                                            
                                                                
                                                         
                                                             
                                                              
                                                              
    
                                                                
                                                     
     
		if (so->so_snd.sb_cc) {
			tp->snd_una++;                    
			if (SEQ_LT(tp->snd_nxt, tp->snd_una)) {
				tp->snd_nxt = tp->snd_una;
			}

			  
                                                 
                                                  
                                      
     
                                                  
                                                  
                                             
                                   
      
			if (SACK_ENABLED(tp) &&
			    (to.to_nsacks > 0 || !TAILQ_EMPTY(&tp->snd_holes))) {
				tcp_sack_doack(tp, &to, th, &sack_bytes_acked, &sack_bytes_newly_acked);
			}

			goto process_ACK;
		}

		OS_FALLTHROUGH;

	  
                                                               
                                     
                                           
                                                   
                                                             
                                                                        
    
	case TCPS_ESTABLISHED:
	case TCPS_FIN_WAIT_1:
	case TCPS_FIN_WAIT_2:
	case TCPS_CLOSE_WAIT:
	case TCPS_CLOSING:
	case TCPS_LAST_ACK:
	case TCPS_TIME_WAIT:
		if (SEQ_GT(th->th_ack, tp->snd_max)) {
			tcpstat.tcps_rcvacktoomuch++;
			if (tcp_is_ack_ratelimited(tp)) {
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "rfc5961 rcvacktoomuch");
				goto drop;
			} else {
				goto dropafterack;
			}
		}
		if (SEQ_LT(th->th_ack, tp->snd_una - tp->max_sndwnd)) {
			if (tcp_is_ack_ratelimited(tp)) {
				TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "rfc5961 bad ACK");
				goto drop;
			} else {
				goto dropafterack;
			}
		}
		if (SACK_ENABLED(tp) && to.to_nsacks > 0) {
			recvd_dsack = tcp_sack_process_dsack(tp, &to, th);
			  
                                                 
                                                
                                                    
      
			if (recvd_dsack &&
			    SEQ_LEQ(th->th_ack, tp->snd_una) &&
			    to.to_nsacks == 0) {
				tcp_bad_rexmt_check(tp, th, &to);
				goto drop;
			}
		}

		if (SACK_ENABLED(tp) &&
		    (to.to_nsacks > 0 || !TAILQ_EMPTY(&tp->snd_holes))) {
			tcp_sack_doack(tp, &to, th, &sack_bytes_acked, &sack_bytes_newly_acked);
		}

#if MPTCP
		if (tp->t_mpuna && SEQ_GEQ(th->th_ack, tp->t_mpuna)) {
			if (tp->t_mpflags & TMPF_PREESTABLISHED) {
				                                    
				tp->t_mpuna = 0;
				if (tp->t_mpflags & TMPF_JOINED_FLOW) {
					if (tp->t_mpflags & TMPF_SENT_JOIN) {
						tp->t_mpflags &=
						    ~TMPF_PREESTABLISHED;
						tp->t_mpflags |=
						    TMPF_MPTCP_TRUE;

						tp->t_timer[TCPT_JACK_RXMT] = 0;
						tp->t_mprxtshift = 0;
						isconnected = TRUE;
					} else {
						isconnected = FALSE;
					}
				} else {
					isconnected = TRUE;
				}
			}
		}
#endif            

		tcp_tfo_rcv_ack(tp, th);

		  
                                            
                                          
                                                         
    
                                                             
                                                         
                       
     
		if (SEQ_LEQ(th->th_ack, tp->snd_una)) {
			if (tlen == 0 && (tiwin == tp->snd_wnd ||
			    (to.to_nsacks > 0 && sack_bytes_acked > 0))) {
				uint32_t old_dupacks;
				  
                                              
                                           
                                            
                                             
                            
       
				if ((thflags & TH_FIN) &&
				    !TCPS_HAVERCVDFIN(tp->t_state)) {
					break;
				}
process_dupack:
				old_dupacks = tp->t_dupacks;
#if MPTCP
				  
                                          
                                        
       
				if (to.to_flags & TOF_MPTCP) {
					goto drop;
				}

				if ((isconnected) && (tp->t_mpflags & TMPF_JOINED_FLOW)) {
					mptcplog((LOG_DEBUG, "MPTCP "
					    "Sockets: bypass ack recovery\n"),
					    MPTCP_SOCKET_DBG,
					    MPTCP_LOGLVL_VERBOSE);
					break;
				}
#endif            
				  
                                              
                                             
                                             
                                          
                
       
				if (tp->ecn_flags & TE_INRECOVERY) {
					tp->ecn_flags &= ~TE_INRECOVERY;
				}

				tcpstat.tcps_rcvdupack++;
				if (SACK_ENABLED(tp) && tcp_do_better_lr) {
					tp->t_dupacks += max(1, sack_bytes_acked / tp->t_maxseg);
				} else {
					++tp->t_dupacks;
				}

				tp->sackhint.sack_bytes_acked += sack_bytes_acked;

				if (SACK_ENABLED(tp) && tcp_do_better_lr) {
					tp->t_new_dupacks += (sack_bytes_newly_acked / tp->t_maxseg);

					if (tp->t_new_dupacks >= tp->t_rexmtthresh && IN_FASTRECOVERY(tp)) {
						                                      
						tcp_sack_lost_rexmit(tp);

						  
                                         
                                        
                                    
         
						if (CC_ALGO(tp)->pre_fr != NULL) {
							CC_ALGO(tp)->pre_fr(tp);
						}

						ENTER_FASTRECOVERY(tp);

						if (tp->t_flags & TF_SENTFIN) {
							tp->snd_recover = tp->snd_max - 1;
						} else {
							tp->snd_recover = tp->snd_max;
						}
						tp->t_rtttime = 0;

						if (TCP_ECN_ENABLED(tp)) {
							tp->ecn_flags |= TE_SENDCWR;
						}

						if (tp->t_flagsext & TF_CWND_NONVALIDATED) {
							tcp_cc_adjust_nonvalidated_cwnd(tp);
						} else {
							tp->snd_cwnd = tp->snd_ssthresh;
						}
					}
				}

				  
                                             
                       
       
				if (tp->t_early_rexmt_count > 0 &&
				    TSTMP_GEQ(tcp_now,
				    (tp->t_early_rexmt_win +
				    TCP_EARLY_REXMT_WIN))) {
					tp->t_early_rexmt_count = 0;
				}

				  
                                               
                                              
                                             
       
				if (!IN_FASTRECOVERY(tp)) {
					tcp_early_rexmt_check(tp, th);
				}

				  
                                            
                                         
                                          
                                      
                                      
              
      
                                          
                                             
                                               
                                             
                         
      
                                               
                                                   
                                                 
                                             
               
       
				if (tp->t_timer[TCPT_REXMT] == 0 ||
				    (th->th_ack != tp->snd_una && sack_bytes_acked == 0)) {
					tp->t_dupacks = 0;
					tp->t_rexmtthresh = tcprexmtthresh;
					tp->t_new_dupacks = 0;
				} else if ((tp->t_dupacks > tp->t_rexmtthresh && (!tcp_do_better_lr || old_dupacks >= tp->t_rexmtthresh)) ||
				    IN_FASTRECOVERY(tp)) {
					  
                                            
                                          
                                       
                           
        
					if (SACK_ENABLED(tp) && !IN_FASTRECOVERY(tp) &&
					    (tp->t_flagsext &
					    (TF_PKTS_REORDERED | TF_DELAY_RECOVERY)) ==
					    (TF_PKTS_REORDERED | TF_DELAY_RECOVERY)) {
						  
                                              
                                          
         
						break;
					}

					  
                                                
                                                    
                                                  
                                              
                
        
					if (SACK_ENABLED(tp) && IN_FASTRECOVERY(tp)) {
						int awnd;

						  
                                                    
                                                 
                                                       
                
         
						awnd = (tp->snd_nxt - tp->snd_fack) + tp->sackhint.sack_bytes_rexmit;
						if (awnd < tp->snd_ssthresh) {
							tp->snd_cwnd += tp->t_maxseg;
							if (tp->snd_cwnd > tp->snd_ssthresh) {
								tp->snd_cwnd = tp->snd_ssthresh;
							}
						}
					} else {
						tp->snd_cwnd += tp->t_maxseg;
					}

					                                
					if (tiwin > tp->snd_wnd) {
						tcp_update_window(tp, thflags,
						    th, tiwin, tlen);
					}
					tcp_ccdbg_trace(tp, th,
					    TCP_CC_IN_FASTRECOVERY);

					(void) tcp_output(tp);

					goto drop;
				} else if ((!tcp_do_better_lr && tp->t_dupacks == tp->t_rexmtthresh) ||
				    (tcp_do_better_lr && tp->t_dupacks >= tp->t_rexmtthresh)) {
					tcp_seq onxt = tp->snd_nxt;

					  
                                     
                                    
                                          
                                        
                 
        
					if (SACK_ENABLED(tp)) {
						if (IN_FASTRECOVERY(tp)) {
							tp->t_dupacks = 0;
							break;
						} else if (tp->t_flagsext & TF_DELAY_RECOVERY) {
							break;
						}
					} else {
						if (SEQ_LEQ(th->th_ack, tp->snd_recover)) {
							tp->t_dupacks = 0;
							break;
						}
					}
					if (tp->t_flags & TF_SENTFIN) {
						tp->snd_recover = tp->snd_max - 1;
					} else {
						tp->snd_recover = tp->snd_max;
					}
					tp->t_timer[TCPT_PTO] = 0;
					tp->t_rtttime = 0;

					  
                                      
                                        
                                   
                 
        
					if (SACK_ENABLED(tp) &&
					    (tp->t_flagsext &
					    (TF_PKTS_REORDERED | TF_DELAY_RECOVERY))
					    == TF_PKTS_REORDERED &&
					    !IN_FASTRECOVERY(tp) &&
					    tp->t_reorderwin > 0 &&
					    (tp->t_state == TCPS_ESTABLISHED ||
					    tp->t_state == TCPS_FIN_WAIT_1)) {
						tp->t_timer[TCPT_DELAYFR] =
						    OFFSET_FROM_START(tp,
						    tp->t_reorderwin);
						tp->t_flagsext |= TF_DELAY_RECOVERY;
						tcpstat.tcps_delay_recovery++;
						tcp_ccdbg_trace(tp, th,
						    TCP_CC_DELAY_FASTRECOVERY);
						break;
					}

					tcp_rexmt_save_state(tp);
					  
                                        
                                       
                                   
        
					if (CC_ALGO(tp)->pre_fr != NULL) {
						CC_ALGO(tp)->pre_fr(tp);
					}
					ENTER_FASTRECOVERY(tp);
					tp->t_timer[TCPT_REXMT] = 0;
					if (TCP_ECN_ENABLED(tp)) {
						tp->ecn_flags |= TE_SENDCWR;
					}

					if (SACK_ENABLED(tp)) {
						tcpstat.tcps_sack_recovery_episode++;
						tp->t_sack_recovery_episode++;
						tp->sack_newdata = tp->snd_nxt;
						if (tcp_do_better_lr) {
							tp->snd_cwnd = tp->snd_ssthresh;
						} else {
							tp->snd_cwnd = tp->t_maxseg;
						}
						tp->t_flagsext &= ~TF_CWND_NONVALIDATED;

						                                
						if (tiwin > tp->snd_wnd) {
							tcp_update_window(tp, thflags, th, tiwin, tlen);
						}

						tcp_ccdbg_trace(tp, th, TCP_CC_ENTER_FASTRECOVERY);
						(void) tcp_output(tp);
						goto drop;
					}
					tp->snd_nxt = th->th_ack;
					tp->snd_cwnd = tp->t_maxseg;

					                                
					if (tiwin > tp->snd_wnd) {
						tcp_update_window(tp, thflags, th, tiwin, tlen);
					}

					(void) tcp_output(tp);
					if (tp->t_flagsext & TF_CWND_NONVALIDATED) {
						tcp_cc_adjust_nonvalidated_cwnd(tp);
					} else {
						tp->snd_cwnd = tp->snd_ssthresh + tp->t_maxseg * tp->t_dupacks;
					}
					if (SEQ_GT(onxt, tp->snd_nxt)) {
						tp->snd_nxt = onxt;
					}

					tcp_ccdbg_trace(tp, th, TCP_CC_ENTER_FASTRECOVERY);
					goto drop;
				} else if (ALLOW_LIMITED_TRANSMIT(tp) &&
				    (!(SACK_ENABLED(tp)) || sack_bytes_acked > 0) &&
				    (so->so_snd.sb_cc - (tp->snd_max - tp->snd_una)) > 0) {
					u_int32_t incr = (tp->t_maxseg * tp->t_dupacks);

					                                                  
                                                         
        
					tp->snd_cwnd += incr;
					tcpstat.tcps_limited_txt++;
					(void) tcp_output(tp);

					tcp_ccdbg_trace(tp, th, TCP_CC_LIMITED_TRANSMIT);

					                                   
					tp->snd_cwnd -= incr;
				}
			}
			break;
		}
		  
                                                     
                                                     
     
		if (IN_FASTRECOVERY(tp)) {
			if (SEQ_LT(th->th_ack, tp->snd_recover)) {
				  
                                        
                                           
                                      
       
				if (tp->ecn_flags & TE_INRECOVERY) {
					goto process_ACK;
				}

				if (SACK_ENABLED(tp)) {
					tcp_sack_partialack(tp, th);
				} else {
					tcp_newreno_partial_ack(tp, th);
				}
				tcp_ccdbg_trace(tp, th, TCP_CC_PARTIAL_ACK);
			} else {
				if (tcp_cubic_minor_fixes) {
					exiting_fr = 1;
				}
				EXIT_FASTRECOVERY(tp);
				if (CC_ALGO(tp)->post_fr != NULL) {
					CC_ALGO(tp)->post_fr(tp, th);
				}
				tp->t_pipeack = 0;
				tcp_clear_pipeack_state(tp);
				tcp_ccdbg_trace(tp, th,
				    TCP_CC_EXIT_FASTRECOVERY);
			}
		} else if ((tp->t_flagsext &
		    (TF_PKTS_REORDERED | TF_DELAY_RECOVERY))
		    == (TF_PKTS_REORDERED | TF_DELAY_RECOVERY)) {
			  
                                                    
                                             
                                               
                                                    
                                                   
                   
      
			if (SEQ_GEQ(th->th_ack, tp->snd_recover) ||
			    TAILQ_EMPTY(&tp->snd_holes)) {
				tp->t_timer[TCPT_DELAYFR] = 0;
				tp->t_flagsext &= ~TF_DELAY_RECOVERY;
				EXIT_FASTRECOVERY(tp);
				tcp_ccdbg_trace(tp, th,
				    TCP_CC_EXIT_FASTRECOVERY);
			}
		} else {
			  
                                             
                            
      
			tp->t_dupacks = 0;
			tp->t_rexmtthresh = tcprexmtthresh;
			tp->t_new_dupacks = 0;
		}

process_ACK:
		VERIFY(SEQ_GEQ(th->th_ack, tp->snd_una));
		acked = BYTES_ACKED(th, tp);
		tcpstat.tcps_rcvackpack++;
		tcpstat.tcps_rcvackbyte += acked;

		  
                                                   
                         
    
                                                  
                                                       
                     
     
		tcp_bad_rexmt_check(tp, th, &to);

		                         
		tcp_compute_rtt(tp, &to, th);

		  
                                                      
                                                            
                                                          
                                                      
     
		TCP_RESET_REXMT_STATE(tp);
		TCPT_RANGESET(tp->t_rxtcur, TCP_REXMTVAL(tp),
		    tp->t_rttmin, TCPTV_REXMTMAX,
		    TCP_ADD_REXMTSLOP(tp));
		if (th->th_ack == tp->snd_max) {
			tp->t_timer[TCPT_REXMT] = 0;
			tp->t_timer[TCPT_PTO] = 0;
			needoutput = 1;
		} else if (tp->t_timer[TCPT_PERSIST] == 0) {
			tp->t_timer[TCPT_REXMT] = OFFSET_FROM_START(tp,
			    tp->t_rxtcur);
		}

		if ((prev_t_state == TCPS_SYN_SENT ||
		    prev_t_state == TCPS_SYN_RECEIVED) &&
		    tp->t_state == TCPS_ESTABLISHED) {
			TCP_LOG_RTT_INFO(tp);
		}

		  
                                                      
                
     
		if (acked == 0) {
			goto step6;
		}

		  
                                                                
                                                    
     
		if ((tp->t_tfo_stats & TFO_S_SYN_DATA_SENT) &&
		    !(tp->t_tfo_flags & TFO_F_NO_SNDPROBING) &&
		    !(th->th_flags & TH_SYN)) {
			tp->t_tfo_flags |= TFO_F_NO_SNDPROBING;
		}

		  
                                                         
                                                             
     
		if ((thflags & TH_ECE) != 0 && TCP_ECN_ENABLED(tp) &&
		    (tp->ecn_flags & TE_SENDIPECT)) {
			  
                                                
              
      
			if (!IN_FASTRECOVERY(tp)) {
				tcp_reduce_congestion_window(tp);
				tp->ecn_flags |= (TE_INRECOVERY | TE_SENDCWR);
				  
                                             
                       
       
				tp->ecn_flags |= TE_RECV_ECN_ECE;
				INP_INC_IFNET_STAT(inp, ecn_recv_ece);
				tcpstat.tcps_ecn_recv_ece++;
				tcp_ccdbg_trace(tp, th, TCP_CC_ECN_RCVD);
			}
		}

		  
                                                        
                                                        
                                                             
    
                                                             
                     
     
		if (!IN_FASTRECOVERY(tp) && !exiting_fr) {
			if (CC_ALGO(tp)->ack_rcvd != NULL) {
				CC_ALGO(tp)->ack_rcvd(tp, th);
			}
			tcp_ccdbg_trace(tp, th, TCP_CC_ACK_RCVD);
		}
		if (acked > so->so_snd.sb_cc) {
			tp->snd_wnd -= so->so_snd.sb_cc;
			sbdrop(&so->so_snd, (int)so->so_snd.sb_cc);
			ourfinisacked = 1;
		} else {
			sbdrop(&so->so_snd, acked);
			tcp_sbsnd_trim(&so->so_snd);
			tp->snd_wnd -= acked;
			ourfinisacked = 0;
		}
		                           
		if (!IN_FASTRECOVERY(tp) &&
		    SEQ_GT(tp->snd_una, tp->snd_recover) &&
		    SEQ_LEQ(th->th_ack, tp->snd_recover)) {
			tp->snd_recover = th->th_ack - 1;
		}

		if (IN_FASTRECOVERY(tp) &&
		    SEQ_GEQ(th->th_ack, tp->snd_recover)) {
			EXIT_FASTRECOVERY(tp);
		}

		tcp_update_snd_una(tp, th->th_ack);

		if (SACK_ENABLED(tp)) {
			if (SEQ_GT(tp->snd_una, tp->snd_recover)) {
				tp->snd_recover = tp->snd_una;
			}
		}
		if (SEQ_LT(tp->snd_nxt, tp->snd_una)) {
			tp->snd_nxt = tp->snd_una;
		}
		if (!SLIST_EMPTY(&tp->t_rxt_segments) &&
		    !TCP_DSACK_SEQ_IN_WINDOW(tp, tp->t_dsack_lastuna,
		    tp->snd_una)) {
			tcp_rxtseg_clean(tp);
		}
		if ((tp->t_flagsext & TF_MEASURESNDBW) != 0 &&
		    tp->t_bwmeas != NULL) {
			tcp_bwmeas_check(tp);
		}

		write_wakeup = 1;

		if (!SLIST_EMPTY(&tp->t_notify_ack)) {
			tcp_notify_acknowledgement(tp, so);
		}

		switch (tp->t_state) {
		  
                                                      
                                                             
                           
     
		case TCPS_FIN_WAIT_1:
			if (ourfinisacked) {
				  
                                   
                                           
                                                      
                                               
                          
       
				if (so->so_state & SS_CANTRCVMORE) {
					tp->t_timer[TCPT_2MSL] = OFFSET_FROM_START(tp,
					    TCP_CONN_MAXIDLE(tp));
					isconnected = FALSE;
					isdisconnected = TRUE;
				}
				DTRACE_TCP4(state__change, void, NULL,
				    struct inpcb *, inp,
				    struct tcpcb *, tp,
				    int32_t, TCPS_FIN_WAIT_2);
				tp->t_state = TCPS_FIN_WAIT_2;
				                                               
                              
       
			}
			break;

		  
                                                       
                                                          
                                                     
                 
     
		case TCPS_CLOSING:
			if (ourfinisacked) {
				DTRACE_TCP4(state__change, void, NULL,
				    struct inpcb *, inp,
				    struct tcpcb *, tp,
				    int32_t, TCPS_TIME_WAIT);
				tp->t_state = TCPS_TIME_WAIT;
				tcp_canceltimers(tp);
				if (tp->t_flagsext & TF_NOTIMEWAIT) {
					tp->t_flags |= TF_CLOSING;
				} else {
					add_to_time_wait(tp, 2 * tcp_msl);
				}
				isconnected = FALSE;
				isdisconnected = TRUE;
			}
			break;

		  
                                                           
                                                           
                                                    
                                       
     
		case TCPS_LAST_ACK:
			if (ourfinisacked) {
				tp = tcp_close(tp);
				goto drop;
			}
			break;

		  
                                                         
                                                        
                                     
     
		case TCPS_TIME_WAIT:
			add_to_time_wait(tp, 2 * tcp_msl);
			goto dropafterack;
		}

		  
                                                
                                                  
                                                 
                                                
                                                   
                                                   
                                                   
               
     
		if (tp->t_state == TCPS_ESTABLISHED &&
		    SACK_ENABLED(tp) && sack_bytes_acked > 0 &&
		    to.to_nsacks > 0 && tp->t_dupacks == 0 &&
		    SEQ_LEQ(th->th_ack, tp->snd_una) && tlen == 0 &&
		    !(tp->t_flagsext & TF_PKTS_REORDERED)) {
			tcpstat.tcps_sack_ackadv++;
			goto process_dupack;
		}
	}

step6:
	  
                              
    
	if (tcp_update_window(tp, thflags, th, tiwin, tlen)) {
		needoutput = 1;
	}

	  
                              
    
	if ((thflags & TH_URG) && th->th_urp &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0) {
		  
                                                   
                                           
                                             
                                                    
     
		if (th->th_urp + so->so_rcv.sb_cc > sb_max) {
			th->th_urp = 0;                          
			thflags &= ~TH_URG;                      
			goto dodata;                             
		}
		  
                                                       
                                                       
                                                               
                                                  
                                       
    
                                              
                                                
                                           
                                               
                                                    
                                        
     
		if (SEQ_GT(th->th_seq + th->th_urp, tp->rcv_up)) {
			tp->rcv_up = th->th_seq + th->th_urp;
			so->so_oobmark = so->so_rcv.sb_cc +
			    (tp->rcv_up - tp->rcv_nxt) - 1;
			if (so->so_oobmark == 0) {
				so->so_state |= SS_RCVATMARK;
			}
			sohasoutofband(so);
			tp->t_oobflags &= ~(TCPOOB_HAVEDATA | TCPOOB_HADDATA);
		}
		  
                                                              
                                                              
                                                           
                              
     
		if (th->th_urp <= (u_int32_t)tlen
#if SO_OOBINLINE
		    && (so->so_options & SO_OOBINLINE) == 0
#endif
		    ) {
			tcp_pulloutofband(so, th, m,
			    drop_hdrlen);                                
		}
	} else {
		  
                                        
                                      
                             
     
		if (SEQ_GT(tp->rcv_nxt, tp->rcv_up)) {
			tp->rcv_up = tp->rcv_nxt;
		}
	}
dodata:

	                                                                       
                                                                           
           
    
	if (isconnected) {
		soisconnected(so);
	} else if (isdisconnected) {
		soisdisconnected(so);
	}

	                                                                            
                          
    
	if (inp->inp_state == INPCB_STATE_DEAD) {
		                                                            
		TCP_LOG_DROP_PCB(TCP_LOG_HDR, th, tp, false, "INPCB_STATE_DEAD");
		goto drop;
	}

	  
                                                                       
                                                             
                                                                 
                                                           
                                                               
                                            
   
                                                                       
                        
    
	if ((tlen || (thflags & TH_FIN)) &&
	    TCPS_HAVERCVDFIN(tp->t_state) == 0 &&
	    (TCPS_HAVEESTABLISHED(tp->t_state) ||
	    (tp->t_state == TCPS_SYN_RECEIVED &&
	    (tp->t_tfo_flags & TFO_F_COOKIE_VALID)))) {
		tcp_seq save_start = th->th_seq;
		tcp_seq save_end = th->th_seq + tlen;
		m_adj(m, drop_hdrlen);                           
		  
                                                               
                                                                  
                                                               
                                                                 
                                                               
                                                         
                 
                                                       
                                                   
                               
     
		if (th->th_seq == tp->rcv_nxt && LIST_EMPTY(&tp->t_segq)) {
			TCP_INC_VAR(tp->t_unacksegs, segment_count);
			  
                                                   
                                                  
                                    
      
			if (tp->t_flags & TF_STREAMING_ON) {
				tcp_compute_rtt(tp, &to, th);
			}

			if (DELAY_ACK(tp, th) &&
			    ((tp->t_flags & TF_ACKNOW) == 0)) {
				if ((tp->t_flags & TF_DELACK) == 0) {
					tp->t_flags |= TF_DELACK;
					tp->t_timer[TCPT_DELACK] =
					    OFFSET_FROM_START(tp, tcp_delack);
				}
			} else {
				tp->t_flags |= TF_ACKNOW;
			}
			tp->rcv_nxt += tlen;
			thflags = th->th_flags & TH_FIN;
			TCP_INC_VAR(tcpstat.tcps_rcvpack, segment_count);
			tcpstat.tcps_rcvbyte += tlen;
			if (nstat_collect) {
				INP_ADD_STAT(inp, cell, wifi, wired,
				    rxpackets, 1);
				INP_ADD_STAT(inp, cell, wifi, wired,
				    rxbytes, tlen);
				inp_set_activity_bitmap(inp);
			}
			tcp_sbrcv_grow(tp, &so->so_rcv, &to, tlen);
			so_recv_data_stat(so, m, drop_hdrlen);

			if (isipv6) {
				memcpy(&saved_hdr, ip6, sizeof(struct ip6_hdr));
				ip6 = (struct ip6_hdr *)&saved_hdr[0];
			} else {
				memcpy(&saved_hdr, ip, ip->ip_hl << 2);
				ip = (struct ip *)&saved_hdr[0];
			}
			memcpy(&saved_tcphdr, th, sizeof(struct tcphdr));

			if (th->th_flags & TH_PUSH) {
				tp->t_flagsext |= TF_LAST_IS_PSH;
			} else {
				tp->t_flagsext &= ~TF_LAST_IS_PSH;
			}

			if (sbappendstream_rcvdemux(so, m)) {
				read_wakeup = 1;
			}
			th = &saved_tcphdr;
		} else {
			if (isipv6) {
				memcpy(&saved_hdr, ip6, sizeof(struct ip6_hdr));
				ip6 = (struct ip6_hdr *)&saved_hdr[0];
			} else {
				memcpy(&saved_hdr, ip, ip->ip_hl << 2);
				ip = (struct ip *)&saved_hdr[0];
			}

			if (tcp_autotune_reorder) {
				tcp_sbrcv_grow(tp, &so->so_rcv, &to, tlen);
			}

			memcpy(&saved_tcphdr, th, sizeof(struct tcphdr));
			thflags = tcp_reass(tp, th, &tlen, m, ifp, &read_wakeup);
			th = &saved_tcphdr;
			tp->t_flags |= TF_ACKNOW;
		}

		if ((tlen > 0 || (th->th_flags & TH_FIN)) && SACK_ENABLED(tp)) {
			if (th->th_flags & TH_FIN) {
				save_end++;
			}
			tcp_update_sack_list(tp, save_start, save_end);
		}

		tcp_adaptive_rwtimo_check(tp, tlen);

		if (tlen > 0) {
			tcp_tfo_rcv_data(tp);
		}

		if (tp->t_flags & TF_DELACK) {
			if (isipv6) {
				KERNEL_DEBUG(DBG_LAYER_END, ((th->th_dport << 16) | th->th_sport),
				    (((ip6->ip6_src.s6_addr16[0]) << 16) | (ip6->ip6_dst.s6_addr16[0])),
				    th->th_seq, th->th_ack, th->th_win);
			} else {
				KERNEL_DEBUG(DBG_LAYER_END, ((th->th_dport << 16) | th->th_sport),
				    (((ip->ip_src.s_addr & 0xffff) << 16) | (ip->ip_dst.s_addr & 0xffff)),
				    th->th_seq, th->th_ack, th->th_win);
			}
		}
	} else {
		if ((so->so_flags & SOF_MP_SUBFLOW) && tlen == 0 &&
		    (m->m_pkthdr.pkt_flags & PKTF_MPTCP_DFIN) &&
		    (m->m_pkthdr.pkt_flags & PKTF_MPTCP)) {
			m_adj(m, drop_hdrlen);                           
			  
                                                                    
                                                                      
      
			m->m_pkthdr.mp_rlen = 0;
			mptcp_input(tptomptp(tp)->mpt_mpte, m);
			tp->t_flags |= TF_ACKNOW;
		} else {
			m_freem(m);
		}
		thflags &= ~TH_FIN;
	}

	  
                                                        
                                   
    
	if (thflags & TH_FIN) {
		if (TCPS_HAVERCVDFIN(tp->t_state) == 0) {
			socantrcvmore(so);
			  
                                        
                                          
                                                
                                                
                                               
      
			TCP_INC_VAR(tp->t_unacksegs, segment_count);
			tp->t_flags |= TF_ACKNOW;
			tp->rcv_nxt++;
		}
		switch (tp->t_state) {
		  
                                           
                                
     
		case TCPS_SYN_RECEIVED:
			tp->t_starttime = tcp_now;
			OS_FALLTHROUGH;
		case TCPS_ESTABLISHED:
			DTRACE_TCP4(state__change, void, NULL, struct inpcb *, inp,
			    struct tcpcb *, tp, int32_t, TCPS_CLOSE_WAIT);
			tp->t_state = TCPS_CLOSE_WAIT;
			break;

		  
                                                           
                             
     
		case TCPS_FIN_WAIT_1:
			DTRACE_TCP4(state__change, void, NULL, struct inpcb *, inp,
			    struct tcpcb *, tp, int32_t, TCPS_CLOSING);
			tp->t_state = TCPS_CLOSING;
			break;

		  
                                                   
                                                        
                     
     
		case TCPS_FIN_WAIT_2:
			DTRACE_TCP4(state__change, void, NULL,
			    struct inpcb *, inp,
			    struct tcpcb *, tp,
			    int32_t, TCPS_TIME_WAIT);
			tp->t_state = TCPS_TIME_WAIT;
			tcp_canceltimers(tp);
			tp->t_flags |= TF_ACKNOW;
			if (tp->t_flagsext & TF_NOTIMEWAIT) {
				tp->t_flags |= TF_CLOSING;
			} else {
				add_to_time_wait(tp, 2 * tcp_msl);
			}
			soisdisconnected(so);
			break;

		  
                                                          
     
		case TCPS_TIME_WAIT:
			add_to_time_wait(tp, 2 * tcp_msl);
			break;
		}
	}
#if TCPDEBUG
	if (so->so_options & SO_DEBUG) {
		tcp_trace(TA_INPUT, ostate, tp, (void *)tcp_saveipgen,
		    &tcp_savetcp, 0);
	}
#endif

	if (read_wakeup) {
		mptcp_handle_input(so);
	}

	  
                              
    
	if (needoutput || (tp->t_flags & TF_ACKNOW)) {
		(void) tcp_output(tp);
	}

	tcp_check_timer_state(tp);

	tcp_handle_wakeup(so, read_wakeup, write_wakeup);

	socket_unlock(so, 1);
	KERNEL_DEBUG(DBG_FNC_TCP_INPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
	return;

dropafterack:
	  
                                                            
                                                     
   
                                                       
                                                      
                          
   
                                                           
                                                         
                                                        
                                                     
                                                          
                                                            
    
	if (tp->t_state == TCPS_SYN_RECEIVED && (thflags & TH_ACK) &&
	    (SEQ_GT(tp->snd_una, th->th_ack) ||
	    SEQ_GT(th->th_ack, tp->snd_max))) {
		IF_TCP_STATINC(ifp, dospacket);
		goto dropwithreset;
	}
#if TCPDEBUG
	if (so->so_options & SO_DEBUG) {
		tcp_trace(TA_DROP, ostate, tp, (void *)tcp_saveipgen,
		    &tcp_savetcp, 0);
	}
#endif
	m_freem(m);
	tp->t_flags |= TF_ACKNOW;

	(void) tcp_output(tp);

	tcp_handle_wakeup(so, read_wakeup, write_wakeup);

	                                                                                 
	socket_unlock(so, 1);
	KERNEL_DEBUG(DBG_FNC_TCP_INPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
	return;
dropwithresetnosock:
	nosock = 1;
dropwithreset:
	  
                                              
                                                 
                                                                   
    
	if ((thflags & TH_RST) || m->m_flags & (M_BCAST | M_MCAST)) {
		goto drop;
	}
	if (isipv6) {
		if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst) ||
		    IN6_IS_ADDR_MULTICAST(&ip6->ip6_src)) {
			goto drop;
		}
	} else if (IN_MULTICAST(ntohl(ip->ip_dst.s_addr)) ||
	    IN_MULTICAST(ntohl(ip->ip_src.s_addr)) ||
	    ip->ip_src.s_addr == htonl(INADDR_BROADCAST) ||
	    in_broadcast(ip->ip_dst, m->m_pkthdr.rcvif)) {
		goto drop;
	}
	                                                

#if TCPDEBUG
	if (tp == 0 || (tp->t_inpcb->inp_socket->so_options & SO_DEBUG)) {
		tcp_trace(TA_DROP, ostate, tp, (void *)tcp_saveipgen,
		    &tcp_savetcp, 0);
	}
#endif
	bzero(&tra, sizeof(tra));
	tra.ifscope = ifscope;
	tra.awdl_unrestricted = 1;
	tra.intcoproc_allowed = 1;
	if (thflags & TH_ACK) {
		                                                             
		tcp_respond(tp, mtod(m, void *), th, m, (tcp_seq)0, th->th_ack,
		    TH_RST, &tra);
	} else {
		if (thflags & TH_SYN) {
			tlen++;
		}
		                                                             
		tcp_respond(tp, mtod(m, void *), th, m, th->th_seq + tlen,
		    (tcp_seq)0, TH_RST | TH_ACK, &tra);
	}
	                                        
	if (dropsocket) {
		(void) soabort(so);
		socket_unlock(so, 1);
	} else if ((inp != NULL) && (nosock == 0)) {
		tcp_handle_wakeup(so, read_wakeup, write_wakeup);

		socket_unlock(so, 1);
	}
	KERNEL_DEBUG(DBG_FNC_TCP_INPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
	return;
dropnosock:
	nosock = 1;
drop:
	  
                                                   
    
#if TCPDEBUG
	if (tp == 0 || (tp->t_inpcb->inp_socket->so_options & SO_DEBUG)) {
		tcp_trace(TA_DROP, ostate, tp, (void *)tcp_saveipgen,
		    &tcp_savetcp, 0);
	}
#endif
	m_freem(m);
	                                        
	if (dropsocket) {
		(void) soabort(so);
		socket_unlock(so, 1);
	} else if (nosock == 0) {
		tcp_handle_wakeup(so, read_wakeup, write_wakeup);

		socket_unlock(so, 1);
	}
	KERNEL_DEBUG(DBG_FNC_TCP_INPUT | DBG_FUNC_END, 0, 0, 0, 0, 0);
	return;
}

  
                                         
   
static void
tcp_dooptions(struct tcpcb *tp, u_char *cp, int cnt, struct tcphdr *th,
    struct tcpopt *to)
{
	u_short mss = 0;
	int opt, optlen;

	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[0];
		if (opt == TCPOPT_EOL) {
			break;
		}
		if (opt == TCPOPT_NOP) {
			optlen = 1;
		} else {
			if (cnt < 2) {
				break;
			}
			optlen = cp[1];
			if (optlen < 2 || optlen > cnt) {
				break;
			}
		}
		switch (opt) {
		default:
			continue;

		case TCPOPT_MAXSEG:
			if (optlen != TCPOLEN_MAXSEG) {
				continue;
			}
			if (!(th->th_flags & TH_SYN)) {
				continue;
			}
			bcopy((char *) cp + 2, (char *) &mss, sizeof(mss));
			NTOHS(mss);
			to->to_mss = mss;
			to->to_flags |= TOF_MSS;
			break;

		case TCPOPT_WINDOW:
			if (optlen != TCPOLEN_WINDOW) {
				continue;
			}
			if (!(th->th_flags & TH_SYN)) {
				continue;
			}
			to->to_flags |= TOF_SCALE;
			to->to_requested_s_scale = min(cp[2], TCP_MAX_WINSHIFT);
			break;

		case TCPOPT_TIMESTAMP:
			if (optlen != TCPOLEN_TIMESTAMP) {
				continue;
			}
			to->to_flags |= TOF_TS;
			bcopy((char *)cp + 2,
			    (char *)&to->to_tsval, sizeof(to->to_tsval));
			NTOHL(to->to_tsval);
			bcopy((char *)cp + 6,
			    (char *)&to->to_tsecr, sizeof(to->to_tsecr));
			NTOHL(to->to_tsecr);
			to->to_tsecr -= tp->t_ts_offset;
			                                                      
			if (!(tp->t_flags & TF_REQ_TSTMP)) {
				tp->t_flags |= TF_REQ_TSTMP;
			}
			break;
		case TCPOPT_SACK_PERMITTED:
			if (optlen != TCPOLEN_SACK_PERMITTED) {
				continue;
			}
			if (th->th_flags & TH_SYN) {
				to->to_flags |= TOF_SACK;
			}
			break;
		case TCPOPT_SACK:
			if (optlen <= 2 || (optlen - 2) % TCPOLEN_SACK != 0) {
				continue;
			}
			to->to_nsacks = (optlen - 2) / TCPOLEN_SACK;
			to->to_sacks = cp + 2;
			tcpstat.tcps_sack_rcv_blocks++;

			break;
		case TCPOPT_FASTOPEN:
			if (optlen == TCPOLEN_FASTOPEN_REQ) {
				if (tp->t_state != TCPS_LISTEN) {
					continue;
				}

				to->to_flags |= TOF_TFOREQ;
			} else {
				if (optlen < TCPOLEN_FASTOPEN_REQ ||
				    (optlen - TCPOLEN_FASTOPEN_REQ) > TFO_COOKIE_LEN_MAX ||
				    (optlen - TCPOLEN_FASTOPEN_REQ) < TFO_COOKIE_LEN_MIN) {
					continue;
				}
				if (tp->t_state != TCPS_LISTEN &&
				    tp->t_state != TCPS_SYN_SENT) {
					continue;
				}

				to->to_flags |= TOF_TFO;
				to->to_tfo = cp + 1;
			}

			break;
#if MPTCP
		case TCPOPT_MULTIPATH:
			tcp_do_mptcp_options(tp, cp, th, to, optlen);
			break;
#endif            
		}
	}
}

static void
tcp_finalize_options(struct tcpcb *tp, struct tcpopt *to, unsigned int ifscope)
{
	if (to->to_flags & TOF_TS) {
		tp->t_flags |= TF_RCVD_TSTMP;
		tp->ts_recent = to->to_tsval;
		tp->ts_recent_age = tcp_now;
	}
	if (to->to_flags & TOF_MSS) {
		tcp_mss(tp, to->to_mss, ifscope);
	}
	if (SACK_ENABLED(tp)) {
		if (!(to->to_flags & TOF_SACK)) {
			tp->t_flagsext &= ~(TF_SACK_ENABLE);
		} else {
			tp->t_flags |= TF_SACK_PERMIT;
		}
	}
	if (to->to_flags & TOF_SCALE) {
		tp->t_flags |= TF_RCVD_SCALE;
		tp->requested_s_scale = to->to_requested_s_scale;

		                                                         
		if (tp->request_r_scale > 0) {
			tp->t_flags |= TF_REQ_SCALE;
		}
	}
}

  
                                            
                                              
                                                  
                       
  
                                         
   
static void
tcp_pulloutofband(struct socket *so, struct tcphdr *th, struct mbuf *m, int off)
{
	int cnt = off + th->th_urp - 1;

	while (cnt >= 0) {
		if (m->m_len > cnt) {
			char *cp = mtod(m, caddr_t) + cnt;
			struct tcpcb *tp = sototcpcb(so);

			tp->t_iobc = *cp;
			tp->t_oobflags |= TCPOOB_HAVEDATA;
			bcopy(cp + 1, cp, (unsigned)(m->m_len - cnt - 1));
			m->m_len--;
			if (m->m_flags & M_PKTHDR) {
				m->m_pkthdr.len--;
			}
			return;
		}
		cnt -= m->m_len;
		m = m->m_next;
		if (m == 0) {
			break;
		}
	}
	panic("tcp_pulloutofband");
}

uint32_t
get_base_rtt(struct tcpcb *tp)
{
	struct rtentry *rt = tp->t_inpcb->inp_route.ro_rt;
	return (rt == NULL) ? 0 : rt->rtt_min;
}

                                                                      
                                                    
   
void
update_base_rtt(struct tcpcb *tp, uint32_t rtt)
{
	u_int32_t base_rtt, i;
	struct rtentry *rt;

	if ((rt = tp->t_inpcb->inp_route.ro_rt) == NULL) {
		return;
	}
	if (rt->rtt_expire_ts == 0) {
		RT_LOCK_SPIN(rt);
		if (rt->rtt_expire_ts != 0) {
			RT_UNLOCK(rt);
			goto update;
		}
		rt->rtt_expire_ts = tcp_now;
		rt->rtt_index = 0;
		rt->rtt_hist[0] = rtt;
		rt->rtt_min = rtt;
		RT_UNLOCK(rt);
		return;
	}
update:
#if TRAFFIC_MGT
	  
                                                     
                                                 
                                                           
    
	if ((tp->t_flagsext & TF_RECV_THROTTLE) &&
	    (int)(tcp_now - tp->t_recv_throttle_ts) >= TCP_RECV_THROTTLE_WIN) {
		base_rtt = rt->rtt_min;
		if (tp->t_rttcur <= (base_rtt + target_qdelay)) {
			tp->t_flagsext &= ~TF_RECV_THROTTLE;
			tp->t_recv_throttle_ts = 0;
		}
	}
#endif                  
	if ((int)(tcp_now - rt->rtt_expire_ts) >=
	    TCP_RTT_HISTORY_EXPIRE_TIME) {
		RT_LOCK_SPIN(rt);
		                                             
		if ((int)(tcp_now - rt->rtt_expire_ts) >=
		    TCP_RTT_HISTORY_EXPIRE_TIME) {
			rt->rtt_index++;
			if (rt->rtt_index >= NRTT_HIST) {
				rt->rtt_index = 0;
			}
			rt->rtt_hist[rt->rtt_index] = rtt;
			rt->rtt_expire_ts = tcp_now;
		} else {
			rt->rtt_hist[rt->rtt_index] =
			    min(rt->rtt_hist[rt->rtt_index], rtt);
		}
		                                             
		rt->rtt_min = 0;
		for (i = 0; i < NRTT_HIST; ++i) {
			if (rt->rtt_hist[i] != 0 &&
			    (rt->rtt_min == 0 ||
			    rt->rtt_hist[i] < rt->rtt_min)) {
				rt->rtt_min = rt->rtt_hist[i];
			}
		}
		RT_UNLOCK(rt);
	} else {
		rt->rtt_hist[rt->rtt_index] =
		    min(rt->rtt_hist[rt->rtt_index], rtt);
		if (rt->rtt_min == 0) {
			rt->rtt_min = rtt;
		} else {
			rt->rtt_min = min(rt->rtt_min, rtt);
		}
	}
}

  
                                                                        
                                                                      
                              
  
                                                                 
                                
  
                                                                     
                                                                     
                        
   
static void
tcp_compute_rtt(struct tcpcb *tp, struct tcpopt *to, struct tcphdr *th)
{
	int rtt = 0;
	VERIFY(to != NULL && th != NULL);
	if (tp->t_rtttime != 0 && SEQ_GT(th->th_ack, tp->t_rtseq)) {
		u_int32_t pipe_ack_val;
		rtt = tcp_now - tp->t_rtttime;
		  
                                                        
                    
     
		if (SEQ_GT(th->th_ack, tp->t_pipeack_lastuna)) {
			pipe_ack_val = th->th_ack - tp->t_pipeack_lastuna;
			                       
			tp->t_pipeack_sample[tp->t_pipeack_ind++] =
			    pipe_ack_val;
			tp->t_pipeack_ind %= TCP_PIPEACK_SAMPLE_COUNT;

			                                            
			pipe_ack_val = tcp_get_max_pipeack(tp);
			tp->t_pipeack = (pipe_ack_val >
			    tcp_initial_cwnd(tp)) ?
			    pipe_ack_val : 0;
		}
		                               
		tp->t_rtttime = 0;
	}
	if (((to->to_flags & TOF_TS) != 0) &&
	    (to->to_tsecr != 0) &&
	    TSTMP_GEQ(tcp_now, to->to_tsecr)) {
		tcp_xmit_timer(tp, (tcp_now - to->to_tsecr),
		    to->to_tsecr, th->th_ack);
	} else if (rtt > 0) {
		tcp_xmit_timer(tp, rtt, 0, th->th_ack);
	}
}

  
                                                               
                   
   
static void
tcp_xmit_timer(struct tcpcb *tp, int rtt,
    u_int32_t tsecr, tcp_seq th_ack)
{
	int delta;
	int old_srtt = tp->t_srtt;
	int old_rttvar = tp->t_rttvar;
	bool log_rtt = false;

	  
                                                         
                                                         
                                            
                             
    
	if (tp->t_inpcb->inp_last_outifp != NULL &&
	    (tp->t_inpcb->inp_last_outifp->if_eflags & IFEF_AWDL) &&
	    th_ack == tp->iss + 1) {
		return;
	}

	if (tp->t_flagsext & TF_RECOMPUTE_RTT) {
		if (SEQ_GT(th_ack, tp->snd_una) &&
		    SEQ_LEQ(th_ack, tp->snd_max) &&
		    (tsecr == 0 ||
		    TSTMP_GEQ(tsecr, tp->t_badrexmt_time))) {
			  
                                   
                                            
                                     
      
			tp->t_flagsext &= ~(TF_RECOMPUTE_RTT);
			tp->t_badrexmt_time = 0;
			tp->t_srtt = max(tp->t_srtt_prev, rtt);
			tp->t_srtt = tp->t_srtt << TCP_RTT_SHIFT;
			tp->t_rttvar = max(tp->t_rttvar_prev, (rtt >> 1));
			tp->t_rttvar = tp->t_rttvar << TCP_RTTVAR_SHIFT;

			if (tp->t_rttbest > (tp->t_srtt + tp->t_rttvar)) {
				tp->t_rttbest = tp->t_srtt + tp->t_rttvar;
			}

			goto compute_rto;
		} else {
			return;
		}
	}

	tcpstat.tcps_rttupdated++;
	tp->t_rttupdated++;

	if (rtt > 0) {
		tp->t_rttcur = rtt;
		update_base_rtt(tp, rtt);
	}

	if (tp->t_srtt != 0) {
		  
                                                        
                                                            
                                                            
                                                       
            
    
                                                     
                                                           
                                                       
                                              
                                                  
     
		delta = (rtt << TCP_DELTA_SHIFT)
		    - (tp->t_srtt >> (TCP_RTT_SHIFT - TCP_DELTA_SHIFT));

		if ((tp->t_srtt += delta) <= 0) {
			tp->t_srtt = 1;
		}

		  
                                                       
                                                       
                                                           
                                                          
                                                   
                                                        
                                                        
                            
     
		if (delta < 0) {
			delta = -delta;
		}
		delta -= tp->t_rttvar >> (TCP_RTTVAR_SHIFT - TCP_DELTA_SHIFT);
		if ((tp->t_rttvar += delta) <= 0) {
			tp->t_rttvar = 1;
		}
		if (tp->t_rttbest == 0 ||
		    tp->t_rttbest > (tp->t_srtt + tp->t_rttvar)) {
			tp->t_rttbest = tp->t_srtt + tp->t_rttvar;
		}
	} else {
		  
                                                     
                                                   
                                  
     
		tp->t_srtt = rtt << TCP_RTT_SHIFT;
		tp->t_rttvar = rtt << (TCP_RTTVAR_SHIFT - 1);
		tp->t_rttbest = tp->t_srtt + tp->t_rttvar;
	}

compute_rto:
	nstat_route_rtt(tp->t_inpcb->inp_route.ro_rt, tp->t_srtt,
	    tp->t_rttvar);

	  
                                                     
                                                           
                                                         
                                                          
                                                         
                                                           
                                               
                                                         
                                                  
    
	TCPT_RANGESET(tp->t_rxtcur, TCP_REXMTVAL(tp),
	    max(tp->t_rttmin, rtt + 2), TCPTV_REXMTMAX,
	    TCP_ADD_REXMTSLOP(tp));

	  
                                                              
                                                              
                                                                
                                                               
                                                  
    
	tp->t_softerror = 0;

	if (log_rtt) {
		TCP_LOG_RTT_INFO(tp);
	}

	TCP_LOG_RTT_CHANGE(tp, old_srtt, old_rttvar);
}

static inline unsigned int
tcp_maxmtu(struct rtentry *rt)
{
	unsigned int maxmtu;
	int interface_mtu = 0;

	RT_LOCK_ASSERT_HELD(rt);
	interface_mtu = rt->rt_ifp->if_mtu;

	if (rt_key(rt)->sa_family == AF_INET &&
	    INTF_ADJUST_MTU_FOR_CLAT46(rt->rt_ifp)) {
		interface_mtu = IN6_LINKMTU(rt->rt_ifp);
		                                                  
		interface_mtu -= CLAT46_HDR_EXPANSION_OVERHD;
	}

	if (rt->rt_rmx.rmx_mtu == 0) {
		maxmtu = interface_mtu;
	} else {
		maxmtu = MIN(rt->rt_rmx.rmx_mtu, interface_mtu);
	}

	return maxmtu;
}

static inline unsigned int
tcp_maxmtu6(struct rtentry *rt)
{
	unsigned int maxmtu;
	struct nd_ifinfo *ndi = NULL;

	RT_LOCK_ASSERT_HELD(rt);
	if ((ndi = ND_IFINFO(rt->rt_ifp)) != NULL && !ndi->initialized) {
		ndi = NULL;
	}
	if (ndi != NULL) {
		lck_mtx_lock(&ndi->lock);
	}
	if (rt->rt_rmx.rmx_mtu == 0) {
		maxmtu = IN6_LINKMTU(rt->rt_ifp);
	} else {
		maxmtu = MIN(rt->rt_rmx.rmx_mtu, IN6_LINKMTU(rt->rt_ifp));
	}
	if (ndi != NULL) {
		lck_mtx_unlock(&ndi->lock);
	}

	return maxmtu;
}

unsigned int
get_maxmtu(struct rtentry *rt)
{
	unsigned int maxmtu = 0;

	RT_LOCK_ASSERT_NOTHELD(rt);

	RT_LOCK(rt);

	if (rt_key(rt)->sa_family == AF_INET6) {
		maxmtu = tcp_maxmtu6(rt);
	} else {
		maxmtu = tcp_maxmtu(rt);
	}

	RT_UNLOCK(rt);

	return maxmtu;
}

  
                                                
                                              
                                                          
                                                           
                                                                         
                                                                   
                                                                        
                                                                         
                                                                     
                                                                      
                                                                 
                                                                
                        
  
                                                              
                                                                
                                                               
                                                                    
                  
  
                                                                    
                                                            
  
   
void
tcp_mss(struct tcpcb *tp, int offer, unsigned int input_ifscope)
{
	struct rtentry *rt;
	struct ifnet *ifp;
	int rtt, mss;
	u_int32_t bufsize;
	struct inpcb *inp;
	struct socket *so;
	int origoffer = offer;
	u_int32_t sb_max_corrected;
	int isnetlocal = 0;
	int isipv6;
	int min_protoh;

	inp = tp->t_inpcb;

	so = inp->inp_socket;
	  
                                                                                  
    
	if ((so->so_state & SS_DEFUNCT) || tp->t_state == TCPS_CLOSED) {
		return;
	}

	isipv6 = ((inp->inp_vflag & INP_IPV6) != 0) ? 1 : 0;
	min_protoh = isipv6 ? sizeof(struct ip6_hdr) + sizeof(struct tcphdr)
	    : sizeof(struct tcpiphdr);

	if (isipv6) {
		rt = tcp_rtlookup6(inp, input_ifscope);
	} else {
		rt = tcp_rtlookup(inp, input_ifscope);
	}
	isnetlocal = (tp->t_flags & TF_LOCAL);

	if (rt == NULL) {
		tp->t_maxopd = tp->t_maxseg = isipv6 ? tcp_v6mssdflt : tcp_mssdflt;
		return;
	}
	ifp = rt->rt_ifp;
	  
                                  
                                                           
                                                                
                                                          
                               
    
	if (ifp->if_type == IFT_PPP && slowlink_wsize > 0 &&
	    ifp->if_baudrate > 9600 && ifp->if_baudrate <= 128000) {
		tp->t_flags |= TF_SLOWLINK;
	}

	  
                                                                 
    
	if (offer == -1) {
		offer = rt->rt_rmx.rmx_filler[0];
	}
	  
                                                              
                                    
    
	if (offer == 0) {
		offer = isipv6 ? tcp_v6mssdflt : tcp_mssdflt;
	} else {
		  
                                                    
                        
     
		offer = max(offer, tcp_minmss);
		  
                                                      
                                                      
                                                       
                                           
     
		offer = max(offer, 64);
	}
	rt->rt_rmx.rmx_filler[0] = offer;

	  
                                                     
                                                  
                                                  
    
	if (tp->t_srtt == 0 && (rtt = rt->rt_rmx.rmx_rtt) != 0) {
		tcp_getrt_rtt(tp, rt);
	} else {
		tp->t_rttmin = isnetlocal ? tcp_TCPTV_MIN : TCPTV_REXMTMIN;
	}

	mss = (isipv6 ? tcp_maxmtu6(rt) : tcp_maxmtu(rt));

#if NECP
	                                                               
	mss = necp_socket_get_effective_mtu(inp, mss);
#endif           

	mss -= min_protoh;

	if (rt->rt_rmx.rmx_mtu == 0) {
		if (isipv6) {
			if (!isnetlocal) {
				mss = min(mss, tcp_v6mssdflt);
			}
		} else if (!isnetlocal) {
			mss = min(mss, tcp_mssdflt);
		}
	}

	mss = min(mss, offer);
	  
                                                        
                                                          
                                                        
                                                          
                                                             
    
	tp->t_maxopd = mss;

	  
                                                                
                               
    
	if ((tp->t_flags & (TF_REQ_TSTMP | TF_NOOPT)) == TF_REQ_TSTMP &&
	    (origoffer == -1 ||
	    (tp->t_flags & TF_RCVD_TSTMP) == TF_RCVD_TSTMP)) {
		mss -= TCPOLEN_TSTAMP_APPA;
	}

#if MPTCP
	mss -= mptcp_adj_mss(tp, FALSE);
#endif            
	tp->t_maxseg = mss;

	  
                                                               
                                                            
    
	sb_max_corrected = (sb_max * (u_int64_t)MCLBYTES) / (MSIZE + MCLBYTES);

	  
                                                          
                                                            
                                                      
                                                  
                                        
    
#if RTV_SPIPE
	bufsize = rt->rt_rmx.rmx_sendpipe;
	if (bufsize < so->so_snd.sb_hiwat)
#endif
	bufsize = so->so_snd.sb_hiwat;
	if (bufsize < mss) {
		mss = bufsize;
	} else {
		bufsize = (((bufsize + (u_int64_t)mss - 1) / (u_int64_t)mss) * (u_int64_t)mss);
		if (bufsize > sb_max_corrected) {
			bufsize = sb_max_corrected;
		}
		(void)sbreserve(&so->so_snd, bufsize);
	}
	tp->t_maxseg = mss;

	ASSERT(tp->t_maxseg);

	  
                                                                    
             
    
	tcp_update_mss_locked(so, ifp);

#if RTV_RPIPE
	bufsize = rt->rt_rmx.rmx_recvpipe;
	if (bufsize < so->so_rcv.sb_hiwat)
#endif
	bufsize = so->so_rcv.sb_hiwat;
	if (bufsize > mss) {
		bufsize = (((bufsize + (u_int64_t)mss - 1) / (u_int64_t)mss) * (u_int64_t)mss);
		if (bufsize > sb_max_corrected) {
			bufsize = sb_max_corrected;
		}
		(void)sbreserve(&so->so_rcv, bufsize);
	}

	set_tcp_stream_priority(so);

	if (rt->rt_rmx.rmx_ssthresh) {
		  
                                              
                                               
                                                   
                        
     
		tp->snd_ssthresh = max(2 * mss, rt->rt_rmx.rmx_ssthresh);
		tcpstat.tcps_usedssthresh++;
	} else {
		tp->snd_ssthresh = TCP_MAXWIN << TCP_MAX_WINSHIFT;
	}

	  
                                                            
                              
    
	if (CC_ALGO(tp)->cwnd_init != NULL) {
		CC_ALGO(tp)->cwnd_init(tp);
	}

	tcp_ccdbg_trace(tp, NULL, TCP_CC_CWND_INIT);

	                                      
	RT_UNLOCK(rt);
}

  
                                                       
   
int
tcp_mssopt(struct tcpcb *tp)
{
	struct rtentry *rt;
	int mss;
	int isipv6;
	int min_protoh;

	isipv6 = ((tp->t_inpcb->inp_vflag & INP_IPV6) != 0) ? 1 : 0;
	min_protoh = isipv6 ? sizeof(struct ip6_hdr) + sizeof(struct tcphdr)
	    : sizeof(struct tcpiphdr);

	if (isipv6) {
		rt = tcp_rtlookup6(tp->t_inpcb, IFSCOPE_NONE);
	} else {
		rt = tcp_rtlookup(tp->t_inpcb, IFSCOPE_NONE);
	}
	if (rt == NULL) {
		return isipv6 ? tcp_v6mssdflt : tcp_mssdflt;
	}
	  
                                  
                                                                     
                                                                          
                                                                  
    
	if (rt->rt_ifp->if_type == IFT_PPP && slowlink_wsize > 0 &&
	    rt->rt_ifp->if_baudrate > 9600 && rt->rt_ifp->if_baudrate <= 128000) {
		tp->t_flags |= TF_SLOWLINK;
	}

	mss = (isipv6 ? tcp_maxmtu6(rt) : tcp_maxmtu(rt));
	                                      
	RT_UNLOCK(rt);

#if NECP
	                                                               
	mss = necp_socket_get_effective_mtu(tp->t_inpcb, mss);
#endif           

	return mss - min_protoh;
}

  
                                                            
                                                            
                                                                    
                    
   
static void
tcp_newreno_partial_ack(struct tcpcb *tp, struct tcphdr *th)
{
	tcp_seq onxt = tp->snd_nxt;
	u_int32_t  ocwnd = tp->snd_cwnd;
	tp->t_timer[TCPT_REXMT] = 0;
	tp->t_timer[TCPT_PTO] = 0;
	tp->t_rtttime = 0;
	tp->snd_nxt = th->th_ack;
	  
                                                          
                                                            
               
    
	tp->snd_cwnd = tp->t_maxseg + BYTES_ACKED(th, tp);
	(void) tcp_output(tp);
	tp->snd_cwnd = ocwnd;
	if (SEQ_GT(onxt, tp->snd_nxt)) {
		tp->snd_nxt = onxt;
	}
	  
                                                              
                    
    
	if (tp->snd_cwnd > BYTES_ACKED(th, tp)) {
		tp->snd_cwnd -= BYTES_ACKED(th, tp);
	} else {
		tp->snd_cwnd = 0;
	}
	tp->snd_cwnd += tp->t_maxseg;
}

  
                                                                 
                                                               
                                                           
                                                                  
                                           
  
                                                 
   
static int
tcp_dropdropablreq(struct socket *head)
{
	struct socket *so, *sonext;
	unsigned int i, j, qlen;
	static u_int32_t rnd = 0;
	static u_int64_t old_runtime;
	static unsigned int cur_cnt, old_cnt;
	u_int64_t now_sec;
	struct inpcb *inp = NULL;
	struct tcpcb *tp;

	if ((head->so_options & SO_ACCEPTCONN) == 0) {
		return 0;
	}

	if (TAILQ_EMPTY(&head->so_incomp)) {
		return 0;
	}

	so_acquire_accept_list(head, NULL);
	socket_unlock(head, 0);

	  
                                                    
                                                          
                                                        
              
    
	TAILQ_FOREACH_SAFE(so, &head->so_incomp, so_list, sonext) {
		inp = sotoinpcb(so);
		tp = intotcpcb(inp);
		if (tp != NULL && tp->t_state == TCPS_CLOSED &&
		    so->so_head != NULL &&
		    (so->so_state & (SS_INCOMP | SS_CANTSENDMORE | SS_CANTRCVMORE)) ==
		    (SS_INCOMP | SS_CANTSENDMORE | SS_CANTRCVMORE)) {
			  
                                                
                                                     
                                                  
                               
      
			if (socket_try_lock(so)) {
				so->so_usecount++;
				goto found_victim;
			} else {
				continue;
			}
		}
	}

	so = TAILQ_FIRST(&head->so_incomp);

	now_sec = net_uptime();
	if ((i = (now_sec - old_runtime)) != 0) {
		old_runtime = now_sec;
		old_cnt = cur_cnt / i;
		cur_cnt = 0;
	}

	qlen = head->so_incqlen;
	if (rnd == 0) {
		rnd = RandomULong();
	}

	if (++cur_cnt > qlen || old_cnt > qlen) {
		rnd = (314159 * rnd + 66329) & 0xffff;
		j = ((qlen + 1) * rnd) >> 16;

		while (j-- && so) {
			so = TAILQ_NEXT(so, so_list);
		}
	}
	                                                                     
	while (so) {
		inp = (struct inpcb *)so->so_pcb;

		sonext = TAILQ_NEXT(so, so_list);

		if (in_pcb_checkstate(inp, WNT_ACQUIRE, 0) != WNT_STOPUSING) {
			  
                                                
                                              
                                                  
                                                 
           
      
			if (socket_try_lock(so)) {
				so->so_usecount++;
				if ((so->so_usecount == 2) &&
				    (so->so_state & SS_INCOMP) &&
				    !(so->so_flags & SOF_INCOMP_INPROGRESS)) {
					break;
				} else {
					  
                                      
                             
        
					in_pcb_checkstate(inp, WNT_RELEASE, 1);
					socket_unlock(so, 1);
				}
			} else {
				  
                                    
                                         
                                            
                                          
       
				in_pcb_checkstate(inp, WNT_RELEASE, 1);
			}
		}
		so = sonext;
	}
	if (so == NULL) {
		socket_lock(head, 0);
		so_release_accept_list(head);
		return 0;
	}

	                                                                   

	if (in_pcb_checkstate(inp, WNT_RELEASE, 1) == WNT_STOPUSING) {
		socket_unlock(so, 1);
		socket_lock(head, 0);
		so_release_accept_list(head);
		return 0;
	}

found_victim:
	if (so->so_usecount != 2 || !(so->so_state & SS_INCOMP)) {
		                                                   
		socket_unlock(so, 1);
		socket_lock(head, 0);
		so_release_accept_list(head);
		return 0;
	}

	socket_lock(head, 0);
	TAILQ_REMOVE(&head->so_incomp, so, so_list);
	head->so_incqlen--;
	head->so_qlen--;
	so->so_state &= ~SS_INCOMP;
	so->so_flags |= SOF_OVERFLOW;
	so->so_head = NULL;
	so_release_accept_list(head);
	socket_unlock(head, 0);

	socket_lock_assert_owned(so);
	tp = sototcpcb(so);

	tcp_close(tp);
	if (inp->inp_wantcnt > 0 && inp->inp_wantcnt != WNT_STOPUSING) {
		  
                                                          
                                                         
                                                   
                                
                                                   
     
		VERIFY(so->so_usecount > 0);
		so->so_usecount--;
		socket_unlock(so, 1);
	} else {
		  
                                                   
                                                    
                         
     
		socket_unlock(so, 0);

		lck_rw_lock_exclusive(tcbinfo.ipi_lock);

		socket_lock(so, 0);
		                                                    
		VERIFY(so->so_usecount > 0);
		so->so_usecount--;

		if (so->so_usecount != 1 ||
		    (inp->inp_wantcnt > 0 &&
		    inp->inp_wantcnt != WNT_STOPUSING)) {
			  
                                             
                                               
                                               
                             
      
			socket_unlock(so, 1);
		} else {
			                                               
			VERIFY(so->so_usecount > 0);
			so->so_usecount--;

			in_pcbdispose(inp);
		}
		lck_rw_done(tcbinfo.ipi_lock);
	}
	tcpstat.tcps_drops++;

	socket_lock(head, 0);
	return 1;
}

                                                   
void
tcp_set_background_cc(struct socket *so)
{
	tcp_set_new_cc(so, TCP_CC_ALGO_BACKGROUND_INDEX);
}

                                                   
void
tcp_set_foreground_cc(struct socket *so)
{
	if (tcp_use_newreno) {
		tcp_set_new_cc(so, TCP_CC_ALGO_NEWRENO_INDEX);
	} else {
		tcp_set_new_cc(so, TCP_CC_ALGO_CUBIC_INDEX);
	}
}

static void
tcp_set_new_cc(struct socket *so, uint16_t cc_index)
{
	struct inpcb *inp = sotoinpcb(so);
	struct tcpcb *tp = intotcpcb(inp);
	u_char old_cc_index = 0;
	if (tp->tcp_cc_index != cc_index) {
		old_cc_index = tp->tcp_cc_index;

		if (CC_ALGO(tp)->cleanup != NULL) {
			CC_ALGO(tp)->cleanup(tp);
		}
		tp->tcp_cc_index = cc_index;

		tcp_cc_allocate_state(tp);

		if (CC_ALGO(tp)->switch_to != NULL) {
			CC_ALGO(tp)->switch_to(tp, old_cc_index);
		}

		tcp_ccdbg_trace(tp, NULL, TCP_CC_CHANGE_ALGO);
	}
}

void
tcp_set_recv_bg(struct socket *so)
{
	if (!IS_TCP_RECV_BG(so)) {
		so->so_flags1 |= SOF1_TRAFFIC_MGT_TCP_RECVBG;
	}
}

void
tcp_clear_recv_bg(struct socket *so)
{
	if (IS_TCP_RECV_BG(so)) {
		so->so_flags1 &= ~(SOF1_TRAFFIC_MGT_TCP_RECVBG);
	}
}

void
inp_fc_throttle_tcp(struct inpcb *inp)
{
	struct tcpcb *tp = inp->inp_ppcb;

	if (!tcp_flow_control_response) {
		return;
	}

	  
                                               
                              
    
	if (CC_ALGO(tp)->pre_fr != NULL) {
		CC_ALGO(tp)->pre_fr(tp);
	}
}

void
inp_fc_unthrottle_tcp(struct inpcb *inp)
{
	struct tcpcb *tp = inp->inp_ppcb;

	if (tcp_flow_control_response) {
		if (CC_ALGO(tp)->post_fr != NULL) {
			CC_ALGO(tp)->post_fr(tp, NULL);
		}

		tp->t_bytes_acked = 0;

		  
                                                      
                                                 
                                                        
                                 
     
		TCP_RESET_REXMT_STATE(tp);

		tp->t_flagsext &= ~TF_CWND_NONVALIDATED;

		  
                                                
                                              
                                         
     
		tcp_output(tp);
		return;
	}

	  
                                               
                              
    
	if (CC_ALGO(tp)->pre_fr != NULL) {
		CC_ALGO(tp)->pre_fr(tp);
	}

	tp->snd_cwnd = tp->snd_ssthresh;
	tp->t_flagsext &= ~TF_CWND_NONVALIDATED;
	  
                                              
                               
    
	tp->t_bytes_acked = 0;

	                                                    
                                                
                                                       
                                
    
	TCP_RESET_REXMT_STATE(tp);

	  
                                               
                                             
                                        
    
	tcp_output(tp);
}

static int
tcp_getstat SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)

	int error;
	struct tcpstat *stat;
	stat = &tcpstat;
#if XNU_TARGET_OS_OSX
	struct tcpstat zero_stat;

	if (tcp_disable_access_to_stats &&
	    !kauth_cred_issuser(kauth_cred_get())) {
		bzero(&zero_stat, sizeof(zero_stat));
		stat = &zero_stat;
	}

#endif                        

	if (req->oldptr == 0) {
		req->oldlen = (size_t)sizeof(struct tcpstat);
	}

	error = SYSCTL_OUT(req, stat, MIN(sizeof(tcpstat), req->oldlen));

	return error;
}

  
                                         
   
int
tcp_input_checksum(int af, struct mbuf *m, struct tcphdr *th, int off, int tlen)
{
	struct ifnet *ifp = m->m_pkthdr.rcvif;

	switch (af) {
	case AF_INET: {
		struct ip *ip = mtod(m, struct ip *);
		struct ipovly *ipov = (struct ipovly *)ip;

		                                                                
		ASSERT((ip->ip_hl << 2) == sizeof(*ip));

		if ((hwcksum_rx || (ifp->if_flags & IFF_LOOPBACK) ||
		    (m->m_pkthdr.pkt_flags & PKTF_LOOP)) &&
		    (m->m_pkthdr.csum_flags & CSUM_DATA_VALID)) {
			if (m->m_pkthdr.csum_flags & CSUM_PSEUDO_HDR) {
				th->th_sum = m->m_pkthdr.csum_rx_val;
			} else {
				uint32_t sum = m->m_pkthdr.csum_rx_val;
				uint32_t start = m->m_pkthdr.csum_rx_start;
				int32_t trailer = (m_pktlen(m) - (off + tlen));

				  
                                                  
                                                  
                                               
                                                  
                                                  
                                                  
                                                   
                                                
                                                
                                                 
                                                  
                         
       
				ASSERT(trailer >= 0);
				if ((m->m_pkthdr.csum_flags & CSUM_PARTIAL) &&
				    ((start != 0 && start != off) || trailer)) {
					uint32_t swbytes = (uint32_t)trailer;

					if (start < off) {
						ip->ip_len += sizeof(*ip);
#if BYTE_ORDER != BIG_ENDIAN
						HTONS(ip->ip_len);
						HTONS(ip->ip_off);
#endif                               
					}
					                         
					sum = m_adj_sum16(m, start, off,
					    tlen, sum);
					if (off > start) {
						swbytes += (off - start);
					} else {
						swbytes += (start - off);
					}

					if (start < off) {
#if BYTE_ORDER != BIG_ENDIAN
						NTOHS(ip->ip_off);
						NTOHS(ip->ip_len);
#endif                               
						ip->ip_len -= sizeof(*ip);
					}

					if (swbytes != 0) {
						tcp_in_cksum_stats(swbytes);
					}
					if (trailer != 0) {
						m_adj(m, -trailer);
					}
				}

				                         
				th->th_sum = in_pseudo(ip->ip_src.s_addr,
				    ip->ip_dst.s_addr,
				    sum + htonl(tlen + IPPROTO_TCP));
			}
			th->th_sum ^= 0xffff;
		} else {
			uint16_t ip_sum;
			int len;
			char b[9];

			bcopy(ipov->ih_x1, b, sizeof(ipov->ih_x1));
			bzero(ipov->ih_x1, sizeof(ipov->ih_x1));
			ip_sum = ipov->ih_len;
			ipov->ih_len = (u_short)tlen;
#if BYTE_ORDER != BIG_ENDIAN
			HTONS(ipov->ih_len);
#endif
			len = sizeof(struct ip) + tlen;
			th->th_sum = in_cksum(m, len);
			bcopy(b, ipov->ih_x1, sizeof(ipov->ih_x1));
			ipov->ih_len = ip_sum;

			tcp_in_cksum_stats(len);
		}
		break;
	}
	case AF_INET6: {
		struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);

		if ((hwcksum_rx || (ifp->if_flags & IFF_LOOPBACK) ||
		    (m->m_pkthdr.pkt_flags & PKTF_LOOP)) &&
		    (m->m_pkthdr.csum_flags & CSUM_DATA_VALID)) {
			if (m->m_pkthdr.csum_flags & CSUM_PSEUDO_HDR) {
				th->th_sum = m->m_pkthdr.csum_rx_val;
			} else {
				uint32_t sum = m->m_pkthdr.csum_rx_val;
				uint32_t start = m->m_pkthdr.csum_rx_start;
				int32_t trailer = (m_pktlen(m) - (off + tlen));

				  
                                                  
                                                  
                                                 
                                                   
                   
       
				ASSERT(trailer >= 0);
				if ((m->m_pkthdr.csum_flags & CSUM_PARTIAL) &&
				    (start != off || trailer != 0)) {
					uint16_t s = 0, d = 0;
					uint32_t swbytes = (uint32_t)trailer;

					if (IN6_IS_SCOPE_EMBED(&ip6->ip6_src)) {
						s = ip6->ip6_src.s6_addr16[1];
						ip6->ip6_src.s6_addr16[1] = 0;
					}
					if (IN6_IS_SCOPE_EMBED(&ip6->ip6_dst)) {
						d = ip6->ip6_dst.s6_addr16[1];
						ip6->ip6_dst.s6_addr16[1] = 0;
					}

					                         
					sum = m_adj_sum16(m, start, off,
					    tlen, sum);
					if (off > start) {
						swbytes += (off - start);
					} else {
						swbytes += (start - off);
					}

					if (IN6_IS_SCOPE_EMBED(&ip6->ip6_src)) {
						ip6->ip6_src.s6_addr16[1] = s;
					}
					if (IN6_IS_SCOPE_EMBED(&ip6->ip6_dst)) {
						ip6->ip6_dst.s6_addr16[1] = d;
					}

					if (swbytes != 0) {
						tcp_in6_cksum_stats(swbytes);
					}
					if (trailer != 0) {
						m_adj(m, -trailer);
					}
				}

				th->th_sum = in6_pseudo(
					&ip6->ip6_src, &ip6->ip6_dst,
					sum + htonl(tlen + IPPROTO_TCP));
			}
			th->th_sum ^= 0xffff;
		} else {
			tcp_in6_cksum_stats(tlen);
			th->th_sum = in6_cksum(m, IPPROTO_TCP, off, tlen);
		}
		break;
	}
	default:
		VERIFY(0);
		                
	}

	if (th->th_sum != 0) {
		tcpstat.tcps_rcvbadsum++;
		IF_TCP_STATINC(ifp, badformat);
		return -1;
	}

	return 0;
}


SYSCTL_PROC(_net_inet_tcp, TCPCTL_STATS, stats,
    CTLTYPE_STRUCT | CTLFLAG_RD | CTLFLAG_LOCKED, 0, 0, tcp_getstat,
    "S,tcpstat", "TCP statistics (struct tcpstat, netinet/tcp_var.h)");

static int
sysctl_rexmtthresh SYSCTL_HANDLER_ARGS
{
#pragma unused(arg1, arg2)

	int error, val = tcprexmtthresh;

	error = sysctl_handle_int(oidp, &val, 0, req);
	if (error || !req->newptr) {
		return error;
	}

	  
                                          
                                       
                    
    

	if (val < 2 || val > 3) {
		return EINVAL;
	}

	tcprexmtthresh = val;

	return 0;
}

SYSCTL_PROC(_net_inet_tcp, OID_AUTO, rexmt_thresh, CTLTYPE_INT | CTLFLAG_RW |
    CTLFLAG_LOCKED, &tcprexmtthresh, 0, &sysctl_rexmtthresh, "I",
    "Duplicate ACK Threshold for Fast Retransmit");
