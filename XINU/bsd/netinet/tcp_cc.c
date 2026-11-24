/*
 * Copyright (c) 2013-2018 Apple Inc. All rights reserved.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/kern_control.h>
#include <sys/domain.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_cc.h>
#include <mach/sdt.h>
#include <libkern/OSAtomic.h>

static int tcp_cc_debug;
SYSCTL_INT(_net_inet_tcp, OID_AUTO, cc_debug, CTLFLAG_RW | CTLFLAG_LOCKED,
    &tcp_cc_debug, 0, "Enable debug data collection");

extern struct tcp_cc_algo tcp_cc_newreno;
SYSCTL_INT(_net_inet_tcp, OID_AUTO, newreno_sockets,
    CTLFLAG_RD | CTLFLAG_LOCKED, &tcp_cc_newreno.num_sockets,
    0, "Number of sockets using newreno");

extern struct tcp_cc_algo tcp_cc_ledbat;
SYSCTL_INT(_net_inet_tcp, OID_AUTO, background_sockets,
    CTLFLAG_RD | CTLFLAG_LOCKED, &tcp_cc_ledbat.num_sockets,
    0, "Number of sockets using background transport");

extern struct tcp_cc_algo tcp_cc_cubic;
SYSCTL_INT(_net_inet_tcp, OID_AUTO, cubic_sockets,
    CTLFLAG_RD | CTLFLAG_LOCKED, &tcp_cc_cubic.num_sockets,
    0, "Number of sockets using cubic");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, use_newreno,
    CTLFLAG_RW | CTLFLAG_LOCKED, int, tcp_use_newreno, 0,
    "Use TCP NewReno by default");

static int tcp_check_cwnd_nonvalidated = 1;
#if (DEBUG || DEVELOPMENT)
SYSCTL_INT(_net_inet_tcp, OID_AUTO, cwnd_nonvalidated,
    CTLFLAG_RW | CTLFLAG_LOCKED, &tcp_check_cwnd_nonvalidated, 0,
    "Check if congestion window is non-validated");
#endif                             

 #define SET_SNDSB_IDEAL_SIZE(sndsb, size) \
	sndsb->sb_idealsize = min(max(tcp_sendspace, tp->snd_ssthresh), \
	tcp_autosndbuf_max);

                                                                          
struct tcp_cc_algo* tcp_cc_algo_list[TCP_CC_ALGO_COUNT];
struct zone *tcp_cc_zone;

#define TCP_CCDBG_NOUNIT 0xffffffff
static kern_ctl_ref tcp_ccdbg_ctlref = NULL;
volatile UInt32 tcp_ccdbg_unit = TCP_CCDBG_NOUNIT;

void tcp_cc_init(void);
static void tcp_cc_control_register(void);
static errno_t tcp_ccdbg_control_connect(kern_ctl_ref kctl,
    struct sockaddr_ctl *sac, void **uinfo);
static errno_t tcp_ccdbg_control_disconnect(kern_ctl_ref kctl,
    u_int32_t unit, void *uinfo);
static struct tcp_cc_algo tcp_cc_algo_none;
  
                                                
   

void
tcp_cc_init(void)
{
	bzero(&tcp_cc_algo_list, sizeof(tcp_cc_algo_list));
	bzero(&tcp_cc_algo_none, sizeof(tcp_cc_algo_none));

	tcp_cc_algo_list[TCP_CC_ALGO_NONE] = &tcp_cc_algo_none;
	tcp_cc_algo_list[TCP_CC_ALGO_NEWRENO_INDEX] = &tcp_cc_newreno;
	tcp_cc_algo_list[TCP_CC_ALGO_BACKGROUND_INDEX] = &tcp_cc_ledbat;
	tcp_cc_algo_list[TCP_CC_ALGO_CUBIC_INDEX] = &tcp_cc_cubic;

	tcp_cc_control_register();
}

static void
tcp_cc_control_register(void)
{
	struct kern_ctl_reg ccdbg_control;
	errno_t err;

	bzero(&ccdbg_control, sizeof(ccdbg_control));
	strlcpy(ccdbg_control.ctl_name, TCP_CC_CONTROL_NAME,
	    sizeof(ccdbg_control.ctl_name));
	ccdbg_control.ctl_connect = tcp_ccdbg_control_connect;
	ccdbg_control.ctl_disconnect = tcp_ccdbg_control_disconnect;
	ccdbg_control.ctl_flags |= CTL_FLAG_PRIVILEGED;
	ccdbg_control.ctl_flags |= CTL_FLAG_REG_SOCK_STREAM;
	ccdbg_control.ctl_sendsize = 32 * 1024;

	err = ctl_register(&ccdbg_control, &tcp_ccdbg_ctlref);
	if (err != 0) {
		log(LOG_ERR, "failed to register tcp_cc debug control");
	}
}

                                                                
static errno_t
tcp_ccdbg_control_connect(kern_ctl_ref kctl, struct sockaddr_ctl *sac,
    void **uinfo)
{
#pragma unused(kctl)
#pragma unused(uinfo)

	UInt32 old_value = TCP_CCDBG_NOUNIT;
	UInt32 new_value = sac->sc_unit;

	if (tcp_ccdbg_unit != old_value) {
		return EALREADY;
	}

	if (OSCompareAndSwap(old_value, new_value, &tcp_ccdbg_unit)) {
		return 0;
	} else {
		return EALREADY;
	}
}

static errno_t
tcp_ccdbg_control_disconnect(kern_ctl_ref kctl, u_int32_t unit, void *uinfo)
{
#pragma unused(kctl, unit, uinfo)

	if (unit == tcp_ccdbg_unit) {
		UInt32 old_value = tcp_ccdbg_unit;
		UInt32 new_value = TCP_CCDBG_NOUNIT;
		if (tcp_ccdbg_unit == new_value) {
			return 0;
		}

		if (!OSCompareAndSwap(old_value, new_value,
		    &tcp_ccdbg_unit)) {
			log(LOG_DEBUG,
			    "failed to disconnect tcp_cc debug control");
		}
	}
	return 0;
}

inline void
tcp_ccdbg_trace(struct tcpcb *tp, struct tcphdr *th, int32_t event)
{
#if !CONFIG_DTRACE
#pragma unused(th)
#endif                     
	struct inpcb *inp = tp->t_inpcb;

	if (tcp_cc_debug && tcp_ccdbg_unit > 0) {
		struct tcp_cc_debug_state dbg_state;
		struct timespec tv;

		bzero(&dbg_state, sizeof(dbg_state));

		nanotime(&tv);
		                          
		dbg_state.ccd_tsns = (tv.tv_sec * 1000000000) + tv.tv_nsec;
		inet_ntop(SOCK_DOM(inp->inp_socket),
		    ((SOCK_DOM(inp->inp_socket) == PF_INET) ?
		    (void *)&inp->inp_laddr.s_addr :
		    (void *)&inp->in6p_laddr), dbg_state.ccd_srcaddr,
		    sizeof(dbg_state.ccd_srcaddr));
		dbg_state.ccd_srcport = ntohs(inp->inp_lport);
		inet_ntop(SOCK_DOM(inp->inp_socket),
		    ((SOCK_DOM(inp->inp_socket) == PF_INET) ?
		    (void *)&inp->inp_faddr.s_addr :
		    (void *)&inp->in6p_faddr), dbg_state.ccd_destaddr,
		    sizeof(dbg_state.ccd_destaddr));
		dbg_state.ccd_destport = ntohs(inp->inp_fport);

		dbg_state.ccd_snd_cwnd = tp->snd_cwnd;
		dbg_state.ccd_snd_wnd = tp->snd_wnd;
		dbg_state.ccd_snd_ssthresh = tp->snd_ssthresh;
		dbg_state.ccd_pipeack = tp->t_pipeack;
		dbg_state.ccd_rttcur = tp->t_rttcur;
		dbg_state.ccd_rxtcur = tp->t_rxtcur;
		dbg_state.ccd_srtt = tp->t_srtt >> TCP_RTT_SHIFT;
		dbg_state.ccd_event = event;
		dbg_state.ccd_sndcc = inp->inp_socket->so_snd.sb_cc;
		dbg_state.ccd_sndhiwat = inp->inp_socket->so_snd.sb_hiwat;
		dbg_state.ccd_bytes_acked = tp->t_bytes_acked;
		dbg_state.ccd_cc_index = tp->tcp_cc_index;
		switch (tp->tcp_cc_index) {
		case TCP_CC_ALGO_CUBIC_INDEX:
			dbg_state.u.cubic_state.ccd_last_max =
			    tp->t_ccstate->cub_last_max;
			dbg_state.u.cubic_state.ccd_tcp_win =
			    tp->t_ccstate->cub_tcp_win;
			dbg_state.u.cubic_state.ccd_avg_lastmax =
			    tp->t_ccstate->cub_avg_lastmax;
			dbg_state.u.cubic_state.ccd_mean_deviation =
			    tp->t_ccstate->cub_mean_dev;
			break;
		case TCP_CC_ALGO_BACKGROUND_INDEX:
			dbg_state.u.ledbat_state.led_base_rtt =
			    get_base_rtt(tp);
			break;
		default:
			break;
		}

		ctl_enqueuedata(tcp_ccdbg_ctlref, tcp_ccdbg_unit,
		    &dbg_state, sizeof(dbg_state), 0);
	}
	DTRACE_TCP5(cc, void, NULL, struct inpcb *, inp,
	    struct tcpcb *, tp, struct tcphdr *, th, int32_t, event);
}

void
tcp_cc_resize_sndbuf(struct tcpcb *tp)
{
	struct sockbuf *sb;
	  
                                                           
                                                        
                                       
    
	sb = &tp->t_inpcb->inp_socket->so_snd;
	if (sb->sb_hiwat > tp->snd_ssthresh &&
	    (sb->sb_flags & SB_AUTOSIZE)) {
		if (sb->sb_idealsize > tp->snd_ssthresh) {
			SET_SNDSB_IDEAL_SIZE(sb, tp->snd_ssthresh);
		}
		sb->sb_flags |= SB_TRIM;
	}
}

void
tcp_bad_rexmt_fix_sndbuf(struct tcpcb *tp)
{
	struct sockbuf *sb;
	sb = &tp->t_inpcb->inp_socket->so_snd;
	if ((sb->sb_flags & (SB_TRIM | SB_AUTOSIZE)) == (SB_TRIM | SB_AUTOSIZE)) {
		  
                                                         
                                                      
                       
     
		SET_SNDSB_IDEAL_SIZE(sb, tp->snd_ssthresh);
		if (sb->sb_hiwat <= sb->sb_idealsize) {
			sbreserve(sb, sb->sb_idealsize);
			sb->sb_flags &= ~SB_TRIM;
		}
	}
}

  
                                               
   
void
tcp_cc_cwnd_init_or_reset(struct tcpcb *tp)
{
	if (tp->t_flags & TF_LOCAL) {
		tp->snd_cwnd = tp->t_maxseg * ss_fltsz_local;
	} else {
		if (tcp_cubic_minor_fixes) {
			tp->snd_cwnd = tcp_initial_cwnd(tp);
		} else {
			                                                     
			tp->snd_cwnd = min(4 * tp->t_maxseg,
			    max(2 * tp->t_maxseg, TCP_CC_CWND_INIT_BYTES));
		}
	}
}

  
                                               
                                                                        
                                                                        
                               
                                                  
                                                  
                                                                           
                                                                          
                                                                    
                                                                  
                                                              
                                                   
                                                               
                           
                                                           
                              
                                                                       
   
int
tcp_cc_delay_ack(struct tcpcb *tp, struct tcphdr *th)
{
	switch (tcp_delack_enabled) {
	case 1:
	case 2:
		if ((tp->t_flags & TF_RXWIN0SENT) == 0 &&
		    (th->th_flags & TH_PUSH) == 0 &&
		    (tp->t_unacksegs == 1)) {
			return 1;
		}
		break;
	case 3:
		if (tcp_ack_strategy == TCP_ACK_STRATEGY_LEGACY) {
			if ((tp->t_flags & TF_RXWIN0SENT) == 0 &&
			    (th->th_flags & TH_PUSH) == 0 &&
			    ((tp->t_unacksegs == 1) ||
			    ((tp->t_flags & TF_STRETCHACK) &&
			    tp->t_unacksegs < maxseg_unacked))) {
				return 1;
			}
		} else {
			uint32_t recwin;

			                                              
			recwin = tcp_sbspace(tp);
			if (recwin > (uint32_t)(TCP_MAXWIN << tp->rcv_scale)) {
				recwin = (uint32_t)(TCP_MAXWIN << tp->rcv_scale);
			}

			                 
     
                                         
                                     
                                                               
                                             
                                                        
                                                                                      
                                                                     
                                                         
                                         
     
                                         
                                                          
                                                         
                               
      
			if ((tp->t_flags & TF_RXWIN0SENT) == 0 &&
			    tp->t_forced_acks == 0 &&
			    tp->t_inpcb->inp_socket->so_rcv.sb_cc > tp->t_inpcb->inp_socket->so_rcv.sb_lowat &&
			    recwin <= tp->t_last_recwin &&
			    (tp->rcv_nxt - tp->last_ack_sent <= tp->t_maxseg ||
			    recwin > (uint32_t)(tcp_recvspace + (tcp_recvspace >> 1))) &&
			    (tp->rcv_nxt - tp->last_ack_sent) < (recwin >> 2)) {
				tp->t_stat.acks_delayed++;
				return 1;
			}
		}
		break;
	}
	return 0;
}

void
tcp_cc_allocate_state(struct tcpcb *tp)
{
	if (tp->tcp_cc_index == TCP_CC_ALGO_CUBIC_INDEX &&
	    tp->t_ccstate == NULL) {
		tp->t_ccstate = (struct tcp_ccstate *)zalloc(tcp_cc_zone);

		  
                                                           
                                                      
                      
     
		if (tp->t_ccstate == NULL) {
			tp->tcp_cc_index = TCP_CC_ALGO_NEWRENO_INDEX;
		} else {
			bzero(tp->t_ccstate, sizeof(*tp->t_ccstate));
		}
	}
}

  
                                                                          
                                                           
   
#define TCP_STRETCHACK_DISABLE_WIN      (15 * 60 * TCP_RETRANSHZ)
void
tcp_cc_after_idle_stretchack(struct tcpcb *tp)
{
	int32_t tdiff;

	if (!(tp->t_flagsext & TF_DISABLE_STRETCHACK)) {
		return;
	}

	tdiff = timer_diff(tcp_now, 0, tp->rcv_nostrack_ts, 0);
	if (tdiff < 0) {
		tdiff = -tdiff;
	}

	if (tdiff > TCP_STRETCHACK_DISABLE_WIN) {
		tp->t_flagsext &= ~TF_DISABLE_STRETCHACK;
		tp->t_stretchack_delayed = 0;

		tcp_reset_stretch_ack(tp);
	}
}

  
                                                               
                            
   

inline uint32_t
tcp_cc_is_cwnd_nonvalidated(struct tcpcb *tp)
{
	struct socket *so = tp->t_inpcb->inp_socket;
	if (tp->t_pipeack == 0 || tcp_check_cwnd_nonvalidated == 0) {
		tp->t_flagsext &= ~TF_CWND_NONVALIDATED;
		return 0;
	}

	  
                                                                   
                                                               
                                          
    
	if (tp->t_pipeack >= (tp->snd_cwnd >> 1) ||
	    (so != NULL && so->so_snd.sb_cc > tp->snd_cwnd)) {
		tp->t_flagsext &= ~TF_CWND_NONVALIDATED;
	} else {
		tp->t_flagsext |= TF_CWND_NONVALIDATED;
	}
	return tp->t_flagsext & TF_CWND_NONVALIDATED;
}

  
                                                                      
         
   
inline void
tcp_cc_adjust_nonvalidated_cwnd(struct tcpcb *tp)
{
	tp->t_pipeack = tcp_get_max_pipeack(tp);
	tcp_clear_pipeack_state(tp);
	tp->snd_cwnd = (max(tp->t_pipeack, tp->t_lossflightsize) >> 1);
	if (tcp_cubic_minor_fixes) {
		tp->snd_cwnd = max(tp->snd_cwnd, tp->t_maxseg);
	} else {
		tp->snd_cwnd = max(tp->snd_cwnd, TCP_CC_CWND_INIT_BYTES);
	}
	tp->snd_cwnd += tp->t_maxseg * tcprexmtthresh;
	tp->t_flagsext &= ~TF_CWND_NONVALIDATED;
}

  
                                                                         
                                                                       
                                                                          
   
inline u_int32_t
tcp_get_max_pipeack(struct tcpcb *tp)
{
	u_int32_t max_pipeack = 0;
	max_pipeack = (tp->t_pipeack_sample[0] > tp->t_pipeack_sample[1]) ?
	    tp->t_pipeack_sample[0] : tp->t_pipeack_sample[1];
	max_pipeack = (tp->t_pipeack_sample[2] > max_pipeack) ?
	    tp->t_pipeack_sample[2] : max_pipeack;

	return max_pipeack;
}

inline void
tcp_clear_pipeack_state(struct tcpcb *tp)
{
	bzero(tp->t_pipeack_sample, sizeof(tp->t_pipeack_sample));
	tp->t_pipeack_ind = 0;
	tp->t_lossflightsize = 0;
}
