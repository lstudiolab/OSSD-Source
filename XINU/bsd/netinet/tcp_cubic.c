/*
 * Copyright (c) 2013-2020 Apple Inc. All rights reserved.
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
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/protosw.h>
#include <sys/socketvar.h>
#include <sys/syslog.h>

#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <netinet/ip6.h>

#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_fsm.h>
#include <netinet/tcp_var.h>
#include <netinet/tcp_cc.h>
#include <netinet/tcpip.h>
#include <netinet/tcp_seq.h>
#include <kern/task.h>
#include <libkern/OSAtomic.h>

static int tcp_cubic_init(struct tcpcb *tp);
static int tcp_cubic_cleanup(struct tcpcb *tp);
static void tcp_cubic_cwnd_init_or_reset(struct tcpcb *tp);
static void tcp_cubic_congestion_avd(struct tcpcb *tp, struct tcphdr *th);
static void tcp_cubic_ack_rcvd(struct tcpcb *tp, struct tcphdr *th);
static void tcp_cubic_pre_fr(struct tcpcb *tp);
static void tcp_cubic_post_fr(struct tcpcb *tp, struct tcphdr *th);
static void tcp_cubic_after_timeout(struct tcpcb *tp);
static int tcp_cubic_delay_ack(struct tcpcb *tp, struct tcphdr *th);
static void tcp_cubic_switch_cc(struct tcpcb *tp, u_int16_t old_index);
static uint32_t tcp_cubic_update(struct tcpcb *tp, u_int32_t rtt);
static inline void tcp_cubic_clear_state(struct tcpcb *tp);


extern float cbrtf(float x);

struct tcp_cc_algo tcp_cc_cubic = {
	.name = "cubic",
	.init = tcp_cubic_init,
	.cleanup = tcp_cubic_cleanup,
	.cwnd_init = tcp_cubic_cwnd_init_or_reset,
	.congestion_avd = tcp_cubic_congestion_avd,
	.ack_rcvd = tcp_cubic_ack_rcvd,
	.pre_fr = tcp_cubic_pre_fr,
	.post_fr = tcp_cubic_post_fr,
	.after_idle = tcp_cubic_cwnd_init_or_reset,
	.after_timeout = tcp_cubic_after_timeout,
	.delay_ack = tcp_cubic_delay_ack,
	.switch_to = tcp_cubic_switch_cc
};

static float tcp_cubic_backoff = 0.2f;                                     
static float tcp_cubic_coeff = 0.4f;
static float tcp_cubic_fast_convergence_factor = 0.875f;

static float tcp_cubic_beta = 0.8f;

SYSCTL_SKMEM_TCP_INT(OID_AUTO, cubic_tcp_friendliness, CTLFLAG_RW | CTLFLAG_LOCKED,
    static int, tcp_cubic_tcp_friendliness, 0, "Enable TCP friendliness");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, cubic_fast_convergence, CTLFLAG_RW | CTLFLAG_LOCKED,
    static int, tcp_cubic_fast_convergence, 0, "Enable fast convergence");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, cubic_use_minrtt, CTLFLAG_RW | CTLFLAG_LOCKED,
    static int, tcp_cubic_use_minrtt, 0, "use a min of 5 sec rtt");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, cubic_minor_fixes, CTLFLAG_RW | CTLFLAG_LOCKED,
    int, tcp_cubic_minor_fixes, 1, "Minor fixes to TCP Cubic");

SYSCTL_SKMEM_TCP_INT(OID_AUTO, cubic_rfc_compliant, CTLFLAG_RW | CTLFLAG_LOCKED,
    int, tcp_cubic_rfc_compliant, 1, "RFC Compliance for TCP Cubic");

static int
tcp_cubic_init(struct tcpcb *tp)
{
	OSIncrementAtomic((volatile SInt32 *)&tcp_cc_cubic.num_sockets);

	if (tcp_cubic_rfc_compliant) {
		tcp_cubic_backoff = 0.3f;                                     
		tcp_cubic_fast_convergence_factor = 0.85f;
		tcp_cubic_beta = 0.7f;
	} else {
		tcp_cubic_backoff = 0.2f;                                     
		tcp_cubic_fast_convergence_factor = 0.875f;
		tcp_cubic_beta = 0.8f;
	}

	VERIFY(tp->t_ccstate != NULL);
	tcp_cubic_clear_state(tp);
	return 0;
}

static int
tcp_cubic_cleanup(struct tcpcb *tp)
{
#pragma unused(tp)
	OSDecrementAtomic((volatile SInt32 *)&tcp_cc_cubic.num_sockets);
	return 0;
}

  
                                                                       
                  
   
static void
tcp_cubic_cwnd_init_or_reset(struct tcpcb *tp)
{
	VERIFY(tp->t_ccstate != NULL);

	tcp_cubic_clear_state(tp);
	tcp_cc_cwnd_init_or_reset(tp);
	tp->t_pipeack = 0;
	tcp_clear_pipeack_state(tp);

	                                             
	tp->t_bytes_acked = 0;

	  
                                                               
                                                                    
                                                                    
                                                                  
                                                             
    
	if (tp->t_inpcb->inp_stat->txbytes <= tcp_initial_cwnd(tp) &&
	    tp->snd_ssthresh < (TCP_MAXWIN << TCP_MAX_WINSHIFT)) {
		tp->snd_ssthresh = TCP_MAXWIN << TCP_MAX_WINSHIFT;
	}

	                                                      
	tp->t_ccstate->cub_last_max = tp->snd_ssthresh;
}

  
                                                                     
                                          
  
                                
   
static uint32_t
tcp_cubic_update(struct tcpcb *tp, u_int32_t rtt)
{
	float K, var;
	u_int32_t elapsed_time, win;

	win = min(tp->snd_cwnd, tp->snd_wnd);
	if (tp->t_ccstate->cub_last_max == 0) {
		tp->t_ccstate->cub_last_max = tp->snd_ssthresh;
	}

	if (tp->t_ccstate->cub_epoch_start == 0) {
		  
                                                             
                                                        
                             
     
		tp->t_ccstate->cub_epoch_start = tcp_now;
		if (tp->t_ccstate->cub_epoch_start == 0) {
			tp->t_ccstate->cub_epoch_start = 1;
		}
		if (win < tp->t_ccstate->cub_last_max) {
			  
                                                  
                                                     
                                               
      
			if (tcp_cubic_minor_fixes) {
				K = ((float)tp->t_ccstate->cub_last_max - win) / tp->t_maxseg / tcp_cubic_coeff;
			} else {
				K = (tp->t_ccstate->cub_last_max - win) / tp->t_maxseg / tcp_cubic_coeff;
			}
			K = cbrtf(K);
			tp->t_ccstate->cub_epoch_period = K * TCP_RETRANSHZ;
			                  
			tp->t_ccstate->cub_origin_point = tp->t_ccstate->cub_last_max;
		} else {
			tp->t_ccstate->cub_epoch_period = 0;
			tp->t_ccstate->cub_origin_point = win;
		}
	}

	VERIFY(tp->t_ccstate->cub_origin_point > 0);
	  
                                                                 
                                
    
	elapsed_time = timer_diff(tcp_now, 0, tp->t_ccstate->cub_epoch_start, 0);

	if (tcp_cubic_use_minrtt) {
		elapsed_time += max(tcp_cubic_use_minrtt, rtt);
	} else {
		elapsed_time += rtt;
	}
	var = (elapsed_time  - tp->t_ccstate->cub_epoch_period) / TCP_RETRANSHZ;
	var = var * var * var * (tcp_cubic_coeff * tp->t_maxseg);

	return (u_int32_t)(tp->t_ccstate->cub_origin_point + var);
}

  
                                                                          
                                                                         
                                       
  
                                                                        
                                                                         
                                                                    
                                                                       
                                                                       
                                                           
  
                                                                       
                                                                   
              
   
#define TCP_CUBIC_ENABLE_TCPMODE(_tp_) \
	((!soissrcrealtime((_tp_)->t_inpcb->inp_socket) && \
	(_tp_)->t_ccstate->cub_mean_dev > (tp->t_maxseg << 1)) ? 1 : 0)

  
                                                                 
                                                              
  
                                                                      
                      
  
                                                                                          
  
   
static uint32_t
tcp_cubic_tcpwin(struct tcpcb *tp, struct tcphdr *th)
{
	if (tp->t_ccstate->cub_tcp_win == 0) {
		                                                                   
                                   
     
		tp->t_ccstate->cub_tcp_win = min(tp->snd_cwnd, tp->snd_wnd);
		if (tcp_cubic_minor_fixes) {
			tp->t_ccstate->cub_tcp_bytes_acked = BYTES_ACKED(th, tp);
		} else {
			tp->t_ccstate->cub_tcp_bytes_acked = 0;
		}
	} else {
		tp->t_ccstate->cub_tcp_bytes_acked += BYTES_ACKED(th, tp);

		if (tcp_cubic_minor_fixes) {
			  
                                                                     
                                                                   
      
			while (tp->t_ccstate->cub_tcp_bytes_acked >= tp->snd_cwnd) {
				                                                    
				tp->t_ccstate->cub_tcp_bytes_acked -= tp->snd_cwnd;

				if (tp->snd_cwnd >= tp->t_ccstate->cub_last_max || !tcp_cubic_rfc_compliant) {
					tp->t_ccstate->cub_tcp_win += tp->t_maxseg;
				} else {
					                                              
					float ai_factor = (float)3 * (1 - tcp_cubic_beta) / (1 + tcp_cubic_beta);

					tp->t_ccstate->cub_tcp_win += (uint32_t)(tp->t_maxseg * ai_factor);
				}
			}
		} else {
			if (tp->t_ccstate->cub_tcp_bytes_acked >= tp->t_ccstate->cub_tcp_win) {
				tp->t_ccstate->cub_tcp_bytes_acked -= tp->t_ccstate->cub_tcp_win;
				tp->t_ccstate->cub_tcp_win += tp->t_maxseg;
			}
		}
	}
	return tp->t_ccstate->cub_tcp_win;
}

static uint32_t
tcp_round_to(uint32_t val, uint32_t round)
{
	if (tcp_cubic_minor_fixes) {
		  
                                                                      
                                                                 
     
		return ((val + (round / 2)) / round) * round;
	} else {
		return (val / round) * round;
	}
}

  
                                                               
   
static void
tcp_cubic_congestion_avd(struct tcpcb *tp, struct tcphdr *th)
{
	u_int32_t cubic_target_win, tcp_win, rtt;
	u_int64_t incr_win = UINT32_MAX;

	                                                              
	if (tcp_cc_is_cwnd_nonvalidated(tp) != 0) {
		return;
	}

	tp->t_bytes_acked += BYTES_ACKED(th, tp);

	rtt = get_base_rtt(tp);
	  
                                                          
                                                              
                    
    
	cubic_target_win = tcp_cubic_update(tp, rtt);

	                                                                    
	tcp_win = tcp_cubic_tcpwin(tp, th);

	if (tp->snd_cwnd < tcp_win && tcp_cubic_minor_fixes == 0 && TCP_CUBIC_ENABLE_TCPMODE(tp)) {
		                                               
		if (tp->t_bytes_acked >= tp->snd_cwnd) {
			tp->t_bytes_acked -= tp->snd_cwnd;
			tp->snd_cwnd = min(tcp_win, TCP_MAXWIN << tp->snd_scale);
		}
	} else {
		if (cubic_target_win > tp->snd_cwnd) {
			  
                                                  
                                                       
                                                   
                                                    
                              
      
			incr_win = (uint64_t)tp->snd_cwnd * tp->t_maxseg;
			incr_win /= (cubic_target_win - tp->snd_cwnd);
			if (!tcp_cubic_minor_fixes) {
				if (incr_win > 0 &&
				    tp->t_bytes_acked >= incr_win) {
					tp->t_bytes_acked -= incr_win;
					tp->snd_cwnd =
					    min((tp->snd_cwnd + tp->t_maxseg),
					    TCP_MAXWIN << tp->snd_scale);
				}
			}
		}
	}

	if (tcp_cubic_minor_fixes) {
		tcp_win = tcp_round_to(tcp_win, tp->t_maxseg);

		if (tp->snd_cwnd < tcp_win) {
			uint64_t tcp_incr_win;

			tcp_incr_win = (uint64_t)tp->snd_cwnd * tp->t_maxseg;
			tcp_incr_win /= (tcp_win - tp->snd_cwnd);

			if (tcp_incr_win < incr_win) {
				                                               
				incr_win = tcp_incr_win;
			}
		}

		if (incr_win > 0 && tp->t_bytes_acked >= incr_win) {
			tp->t_bytes_acked -= incr_win;
			tp->snd_cwnd = min(tp->snd_cwnd + tp->t_maxseg, TCP_MAXWIN << tp->snd_scale);
		}
	}
}

static void
tcp_cubic_ack_rcvd(struct tcpcb *tp, struct tcphdr *th)
{
	                                                                  
	if (tcp_cc_is_cwnd_nonvalidated(tp) != 0) {
		return;
	}

	if (tp->snd_cwnd >= tp->snd_ssthresh) {
		                                
		tcp_cubic_congestion_avd(tp, th);
	} else {
		  
                                                  
                            
     
		uint32_t acked, abc_lim, incr;

		acked = BYTES_ACKED(th, tp);
		if (tcp_cubic_minor_fixes) {
			  
                                                                     
                                                              
      
			abc_lim = tcp_initial_cwnd(tp);
		} else {
			abc_lim = (tp->snd_nxt == tp->snd_max) ? 2 * tp->t_maxseg : tp->t_maxseg;
		}
		incr = min(acked, abc_lim);

		tp->snd_cwnd += incr;
		tp->snd_cwnd = min(tp->snd_cwnd, TCP_MAXWIN << tp->snd_scale);
	}
}

static void
tcp_cubic_pre_fr(struct tcpcb *tp)
{
	u_int32_t win, avg;
	int32_t dev;
	tp->t_ccstate->cub_epoch_start = 0;
	tp->t_ccstate->cub_tcp_win = 0;
	tp->t_ccstate->cub_tcp_bytes_acked = 0;

	win = min(tp->snd_cwnd, tp->snd_wnd);
	if (tp->t_flagsext & TF_CWND_NONVALIDATED) {
		tp->t_lossflightsize = tp->snd_max - tp->snd_una;
		if (tcp_flow_control_response) {
			win = max(tp->t_pipeack, tp->t_lossflightsize);
		} else {
			win = (max(tp->t_pipeack, tp->t_lossflightsize)) >> 1;
		}
	} else {
		tp->t_lossflightsize = 0;
	}
	  
                                                               
                 
   
                                                                  
                                                              
                                                                    
                                                                   
                                   
    
	if (win < tp->t_ccstate->cub_last_max && tcp_cubic_minor_fixes) {
		tp->t_ccstate->cub_last_max = (uint32_t)((float)win * tcp_cubic_fast_convergence_factor);
	} else {
		tp->t_ccstate->cub_last_max = win;
	}

	if (tp->t_ccstate->cub_last_max == 0) {
		  
                                                       
                                                          
              
     
		tp->t_ccstate->cub_last_max = tp->snd_max - tp->snd_una;
	}

	  
                                                      
                                         
    
	if (tp->t_ccstate->cub_avg_lastmax == 0) {
		tp->t_ccstate->cub_avg_lastmax = tp->t_ccstate->cub_last_max;
	} else {
		  
                                              
                                                  
     
		avg = tp->t_ccstate->cub_avg_lastmax;
		avg = (avg << 6) - avg;
		tp->t_ccstate->cub_avg_lastmax =
		    (avg + tp->t_ccstate->cub_last_max) >> 6;
	}

	                                      
	dev = tp->t_ccstate->cub_avg_lastmax - tp->t_ccstate->cub_last_max;

	                             
	if (dev < 0) {
		dev = -dev;
	}

	if (tp->t_ccstate->cub_mean_dev == 0) {
		tp->t_ccstate->cub_mean_dev = dev;
	} else {
		dev = dev + ((tp->t_ccstate->cub_mean_dev << 4)
		    - tp->t_ccstate->cub_mean_dev);
		tp->t_ccstate->cub_mean_dev = dev >> 4;
	}

	                                                           
	win = (u_int32_t)(win - (win * tcp_cubic_backoff));
	win = tcp_round_to(win, tp->t_maxseg);
	if (win < 2 * tp->t_maxseg) {
		win =  2 * tp->t_maxseg;
	}
	tp->snd_ssthresh = win;
	tcp_cc_resize_sndbuf(tp);
}

static void
tcp_cubic_post_fr(struct tcpcb *tp, struct tcphdr *th)
{
	uint32_t flight_size = 0;
	uint32_t ack;

	if (th != NULL) {
		ack = th->th_ack;
	} else {
		ack = tp->snd_una;
	}

	if (SEQ_LEQ(ack, tp->snd_max) && (!tcp_cubic_minor_fixes || tcp_flow_control_response)) {
		flight_size = tp->snd_max - ack;
	} else if (tcp_cubic_minor_fixes) {
		  
                                                                    
                                                                    
                                                                     
                                                                         
    
                                                              
                                                             
    
                                                                      
                              
     
		flight_size = tp->snd_ssthresh;
	}

	  
                                                               
                                                          
    
	if (SACK_ENABLED(tp) && tp->t_lossflightsize > 0 && !tcp_cubic_minor_fixes) {
		u_int32_t total_rxt_size = 0, ncwnd;
		  
                                                            
                                    
     
		total_rxt_size = tcp_rxtseg_total_size(tp);
		ncwnd = max(tp->t_pipeack, tp->t_lossflightsize);
		if (total_rxt_size <= ncwnd) {
			ncwnd = ncwnd - total_rxt_size;
		}

		  
                                                          
                             
     
		ncwnd = min(ncwnd, (tp->t_maxseg << 6));
		ncwnd = ncwnd >> 1;
		flight_size = max(ncwnd, flight_size);
	}
	  
                                                                    
                                        
   
                                                                 
                                                                
                                                                 
                                                                  
    
	if (flight_size < tp->snd_ssthresh) {
		tp->snd_cwnd = max(flight_size, tp->t_maxseg)
		    + tp->t_maxseg;
	} else {
		tp->snd_cwnd = tp->snd_ssthresh;
	}
	tp->t_ccstate->cub_tcp_win = 0;
	tp->t_ccstate->cub_tcp_bytes_acked = 0;
}

static void
tcp_cubic_after_timeout(struct tcpcb *tp)
{
	VERIFY(tp->t_ccstate != NULL);

	  
                                                                 
                                                               
                                
    
	if (tp->t_state < TCPS_ESTABLISHED &&
	    ((int)(tp->snd_max - tp->snd_una) <= 1)) {
		return;
	}

	if (!IN_FASTRECOVERY(tp)) {
		tcp_cubic_clear_state(tp);
		tcp_cubic_pre_fr(tp);
	}

	  
                                                                   
                                             
    
	tp->snd_cwnd = tp->t_maxseg;
}

static int
tcp_cubic_delay_ack(struct tcpcb *tp, struct tcphdr *th)
{
	return tcp_cc_delay_ack(tp, th);
}

  
                                                                     
                                                                        
                                                                        
                                                                            
   
static void
tcp_cubic_switch_cc(struct tcpcb *tp, uint16_t old_cc_index)
{
#pragma unused(old_cc_index)
	tcp_cubic_cwnd_init_or_reset(tp);

	OSIncrementAtomic((volatile SInt32 *)&tcp_cc_cubic.num_sockets);
}

static inline void
tcp_cubic_clear_state(struct tcpcb *tp)
{
	tp->t_ccstate->cub_last_max = 0;
	tp->t_ccstate->cub_epoch_start = 0;
	tp->t_ccstate->cub_origin_point = 0;
	tp->t_ccstate->cub_tcp_win = 0;
	tp->t_ccstate->cub_tcp_bytes_acked = 0;
	tp->t_ccstate->cub_epoch_period = 0;
}
