/*
 * Copyright (c) 2010-2020 Apple Inc. All rights reserved.
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
#include <sys/protosw.h>
#include <sys/mcache.h>
#include <sys/sysctl.h>

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <netinet/ip6.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_cc.h>

#include <libkern/OSAtomic.h>

                                                                     
                                                                         
                                                      
   

int tcp_ledbat_init(struct tcpcb *tp);
int tcp_ledbat_cleanup(struct tcpcb *tp);
void tcp_ledbat_cwnd_init(struct tcpcb *tp);
void tcp_ledbat_congestion_avd(struct tcpcb *tp, struct tcphdr *th);
void tcp_ledbat_ack_rcvd(struct tcpcb *tp, struct tcphdr *th);
void tcp_ledbat_pre_fr(struct tcpcb *tp);
void tcp_ledbat_post_fr(struct tcpcb *tp, struct tcphdr *th);
void tcp_ledbat_after_idle(struct tcpcb *tp);
void tcp_ledbat_after_timeout(struct tcpcb *tp);
static int tcp_ledbat_delay_ack(struct tcpcb *tp, struct tcphdr *th);
void tcp_ledbat_switch_cc(struct tcpcb *tp, uint16_t old_cc_index);

struct tcp_cc_algo tcp_cc_ledbat = {
	.name = "ledbat",
	.init = tcp_ledbat_init,
	.cleanup = tcp_ledbat_cleanup,
	.cwnd_init = tcp_ledbat_cwnd_init,
	.congestion_avd = tcp_ledbat_congestion_avd,
	.ack_rcvd = tcp_ledbat_ack_rcvd,
	.pre_fr = tcp_ledbat_pre_fr,
	.post_fr = tcp_ledbat_post_fr,
	.after_idle = tcp_ledbat_after_idle,
	.after_timeout = tcp_ledbat_after_timeout,
	.delay_ack = tcp_ledbat_delay_ack,
	.switch_to = tcp_ledbat_switch_cc
};

                                                                     
                                                                       
                                                                 
                                                                 
                     
  
                                                                   
                     
   
SYSCTL_SKMEM_TCP_INT(OID_AUTO, bg_target_qdelay, CTLFLAG_RW | CTLFLAG_LOCKED,
    int, target_qdelay, 100, "Target queuing delay");

                                                                  
                                                                     
                                                                   
                                                                    
                                                                       
  
                                                               
                                     
  
                                                                             
                                                                       
                                                                       
                                                                          
  
                                                                             
                     
   
SYSCTL_SKMEM_TCP_INT(OID_AUTO, bg_allowed_increase, CTLFLAG_RW | CTLFLAG_LOCKED,
    int, allowed_increase, 8,
    "Additive constant used to calculate max allowed congestion window");

                                                  
SYSCTL_SKMEM_TCP_INT(OID_AUTO, bg_tether_shift, CTLFLAG_RW | CTLFLAG_LOCKED,
    int, tether_shift, 1, "Tether shift for max allowed congestion window");

                                                                         
                                                                  
                                                                        
                                                                                   
   
SYSCTL_SKMEM_TCP_INT(OID_AUTO, bg_ss_fltsz, CTLFLAG_RW | CTLFLAG_LOCKED,
    uint32_t, bg_ss_fltsz, 2, "Initial congestion window for background transport");

extern int rtt_samples_per_slot;

static void
update_cwnd(struct tcpcb *tp, uint32_t incr)
{
	uint32_t max_allowed_cwnd = 0, flight_size = 0;
	uint32_t base_rtt;

	base_rtt = get_base_rtt(tp);

	                                                          
                                           
    
	if (base_rtt == 0 || tp->t_rttcur == 0) {
		tp->snd_cwnd += incr;
		goto check_max;
	}

	if (tp->t_rttcur <= (base_rtt + target_qdelay)) {
		  
                                                          
                                                 
    
                                                    
                                                             
                                                                
                          
     
		if (tp->bg_ssthresh < tp->snd_cwnd) {
			tp->bg_ssthresh = tp->snd_cwnd;
		}
		tp->snd_cwnd += incr;
	} else {
		                                                           
                                                              
                           
     
		uint32_t redwin;

		redwin = tp->snd_cwnd >> 3;
		tp->snd_cwnd -= redwin;
		if (tp->snd_cwnd < bg_ss_fltsz * tp->t_maxseg) {
			tp->snd_cwnd = bg_ss_fltsz * tp->t_maxseg;
		}

		                                                               
                                            
     
		if (tp->bg_ssthresh > tp->snd_cwnd) {
			tp->bg_ssthresh = tp->snd_cwnd;
		}
	}
check_max:
	                                                         
                                                 
    
	flight_size = tp->snd_max - tp->snd_una;

	max_allowed_cwnd = (allowed_increase * tp->t_maxseg)
	    + (flight_size << tether_shift);
	tp->snd_cwnd = min(tp->snd_cwnd, max_allowed_cwnd);
	return;
}

int
tcp_ledbat_init(struct tcpcb *tp)
{
#pragma unused(tp)
	OSIncrementAtomic((volatile SInt32 *)&tcp_cc_ledbat.num_sockets);
	return 0;
}

int
tcp_ledbat_cleanup(struct tcpcb *tp)
{
#pragma unused(tp)
	OSDecrementAtomic((volatile SInt32 *)&tcp_cc_ledbat.num_sockets);
	return 0;
}

                                                    
  
   

void
tcp_ledbat_cwnd_init(struct tcpcb *tp)
{
	tp->snd_cwnd = tp->t_maxseg * bg_ss_fltsz;
	tp->bg_ssthresh = tp->snd_ssthresh;
}

                                                                      
                                                                             
                                                           
   
void
tcp_ledbat_congestion_avd(struct tcpcb *tp, struct tcphdr *th)
{
	int acked = 0;
	u_int32_t incr = 0;

	acked = BYTES_ACKED(th, tp);
	tp->t_bytes_acked += acked;
	if (tp->t_bytes_acked > tp->snd_cwnd) {
		tp->t_bytes_acked -= tp->snd_cwnd;
		incr = tp->t_maxseg;
	}

	if (tp->snd_cwnd < tp->snd_wnd && incr > 0) {
		update_cwnd(tp, incr);
	}
}
                              
   
void
tcp_ledbat_ack_rcvd(struct tcpcb *tp, struct tcphdr *th)
{
	  
                                         
   
                                                  
                                                   
                                                   
                          
   
                                                     
                                                   
                                                   
    

	uint32_t cw = tp->snd_cwnd;
	uint32_t incr = tp->t_maxseg;
	uint32_t acked = 0;

	acked = BYTES_ACKED(th, tp);
	tp->t_bytes_acked += acked;
	if (cw >= tp->bg_ssthresh) {
		                          
		if (tp->t_bytes_acked < cw) {
			                              
			incr = 0;
		}
	} else {
		  
                                           
                                             
                                      
    
                                          
     
		u_int abc_lim;

		abc_lim = (tp->snd_nxt == tp->snd_max) ? incr * 2 : incr;

		incr = ulmin(acked, abc_lim);
	}
	if (tp->t_bytes_acked >= cw) {
		tp->t_bytes_acked -= cw;
	}
	if (incr > 0) {
		update_cwnd(tp, incr);
	}
}

void
tcp_ledbat_pre_fr(struct tcpcb *tp)
{
	uint32_t win;

	win = min(tp->snd_wnd, tp->snd_cwnd) /
	    2 / tp->t_maxseg;
	if (win < 2) {
		win = 2;
	}
	tp->snd_ssthresh = win * tp->t_maxseg;
	if (tp->bg_ssthresh > tp->snd_ssthresh) {
		tp->bg_ssthresh = tp->snd_ssthresh;
	}

	tcp_cc_resize_sndbuf(tp);
}

void
tcp_ledbat_post_fr(struct tcpcb *tp, struct tcphdr *th)
{
	int32_t ss;

	if (th) {
		ss = tp->snd_max - th->th_ack;
	} else {
		ss = tp->snd_max - tp->snd_una;
	}

	  
                                                   
                                    
   
                                                     
                                                  
                                                   
                                    
   
                                                    
                                                   
                                                             
    
	if (ss < (int32_t)tp->snd_ssthresh) {
		tp->snd_cwnd = max(ss, tp->t_maxseg) + tp->t_maxseg;
	} else {
		tp->snd_cwnd = tp->snd_ssthresh;
	}
	tp->t_bytes_acked = 0;
}

  
                                                         
                                                          
                                      
   
void
tcp_ledbat_after_idle(struct tcpcb *tp)
{
	                                 
	tp->snd_cwnd = tp->t_maxseg * bg_ss_fltsz;
}

                                                               
                                                                
                                                                 
                                                               
                                                                 
                  
   
void
tcp_ledbat_after_timeout(struct tcpcb *tp)
{
	if (tp->t_state >= TCPS_ESTABLISHED) {
		u_int win = min(tp->snd_wnd, tp->snd_cwnd) / 2 / tp->t_maxseg;
		if (win < 2) {
			win = 2;
		}
		tp->snd_ssthresh = win * tp->t_maxseg;

		if (tp->bg_ssthresh > tp->snd_ssthresh) {
			tp->bg_ssthresh = tp->snd_ssthresh;
		}

		tp->snd_cwnd = tp->t_maxseg;
		tcp_cc_resize_sndbuf(tp);
	}
}

  
                                               
                           
                                               
                                                                             
                                                                          
                                                                    
                                                           
                                                                             
                                                                               
                                          
  
   

static int
tcp_ledbat_delay_ack(struct tcpcb *tp, struct tcphdr *th)
{
	if (tcp_ack_strategy == TCP_ACK_STRATEGY_MODERN) {
		return tcp_cc_delay_ack(tp, th);
	} else {
		if ((tp->t_flags & TF_RXWIN0SENT) == 0 &&
		    (th->th_flags & TH_PUSH) == 0 && (tp->t_unacksegs == 1)) {
			return 1;
		}
		return 0;
	}
}

                                                                    
                     
   
void
tcp_ledbat_switch_cc(struct tcpcb *tp, uint16_t old_cc_index)
{
#pragma unused(old_cc_index)
	uint32_t cwnd;

	if (tp->bg_ssthresh == 0 || tp->bg_ssthresh > tp->snd_ssthresh) {
		tp->bg_ssthresh = tp->snd_ssthresh;
	}

	cwnd = min(tp->snd_wnd, tp->snd_cwnd);

	if (tp->snd_cwnd > tp->bg_ssthresh) {
		cwnd = cwnd / tp->t_maxseg;
	} else {
		cwnd = cwnd / 2 / tp->t_maxseg;
	}

	if (cwnd < bg_ss_fltsz) {
		cwnd = bg_ss_fltsz;
	}

	tp->snd_cwnd = cwnd * tp->t_maxseg;
	tp->t_bytes_acked = 0;

	OSIncrementAtomic((volatile SInt32 *)&tcp_cc_ledbat.num_sockets);
}
