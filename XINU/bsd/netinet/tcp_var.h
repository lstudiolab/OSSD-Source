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
 * Copyright (c) 1982, 1986, 1993, 1994, 1995
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
 *	@(#)tcp_var.h	8.4 (Berkeley) 5/24/95
 * $FreeBSD: src/sys/netinet/tcp_var.h,v 1.56.2.8 2001/08/22 00:59:13 silby Exp $
 */

#ifndef _NETINET_TCP_VAR_H_
#define _NETINET_TCP_VAR_H_
#include <sys/types.h>
#include <sys/appleapiopts.h>
#include <sys/queue.h>
#include <netinet/in_pcb.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#if !KERNEL
#include <TargetConditionals.h>
#endif

#if defined(__LP64__)
#define _TCPCB_PTR(x)                   u_int32_t
#define _TCPCB_LIST_HEAD(name, type)    \
struct name {                           \
	u_int32_t	lh_first;       \
}
#else
#define _TCPCB_PTR(x)                   x
#define _TCPCB_LIST_HEAD(name, type)    LIST_HEAD(name, type)
#endif

#ifdef KERNEL_PRIVATE

#define TCP_RETRANSHZ   1000                                            
                                                                
#define TCP_TIMER_10MS_QUANTUM  (TCP_RETRANSHZ/100)                 
#define TCP_TIMER_100MS_QUANTUM   (TCP_RETRANSHZ/10)                  
#define TCP_TIMER_500MS_QUANTUM   (TCP_RETRANSHZ/2)                  

#define TCP_RETRANSHZ_TO_USEC 1000

#define N_TIME_WAIT_SLOTS   128                                 

                                                                       
                                                 
   
#define MIN_IAJ_WIN 16

                                                                                 
                                                                                   
                                                      
   
#define ALLOWED_IAJ 5

                                                                              
   
#define IAJ_IGNORE_PKTCNT 40

                                                                               
                                                                             
                                    
   
#define ACC_IAJ_HIGH_THRESH 100

                                                                           
                     
   
#define ACC_IAJ_REACT_LIMIT 200

                                                                          
                                                                             
                                                                           
                                                        
   
#define RESET_IAJ_SIZE_THRESH 20

  
                                                                           
                                                                      
                                                                         
                                                                       
                                                                           
                                                                              
                                    
  
                                                                         
                                                                     
  
                                                                      
                                                       
   
#define TCP_ADAPTIVE_TIMEOUT_MAX        10

#define TCP_CONNECTIVITY_PROBES_MAX     5

  
                            
   

                             
struct tseg_qent {
	LIST_ENTRY(tseg_qent) tqe_q;
	int     tqe_len;                                             
	struct  tcphdr *tqe_th;                                      
	struct  mbuf    *tqe_m;                                   
};
LIST_HEAD(tsegqe_head, tseg_qent);

struct sackblk {
	tcp_seq start;                                           
	tcp_seq end;                             
};

struct sackhole {
	tcp_seq start;                                     
	tcp_seq end;                             
	tcp_seq rxmit;                                                        
	u_int32_t rxmit_start;                                         
	TAILQ_ENTRY(sackhole) scblink;                          
};

struct sackhint {
	struct sackhole *nexthole;
	int     sack_bytes_rexmit;
	int sack_bytes_acked;
};

struct tcp_rxt_seg {
	tcp_seq rx_start;
	tcp_seq rx_end;
	u_int16_t rx_count;
	u_int16_t rx_flags;
#define TCP_RXT_SPURIOUS        0x1                                      
#define TCP_RXT_DSACK_FOR_TLP   0x2
	SLIST_ENTRY(tcp_rxt_seg) rx_link;
};

struct tcp_notify_ack_marker {
	tcp_seq notify_snd_una;                                           
	tcp_notify_ack_id_t notify_id;
	SLIST_ENTRY(tcp_notify_ack_marker) notify_next;
};

struct tcptemp {
	u_char  tt_ipgen[40];                                                  
	struct  tcphdr tt_t;
};

struct bwmeas {
	tcp_seq bw_start;                                    
	uint32_t bw_ts;                                                    
	uint32_t bw_size;                                                        
	uint32_t bw_minsizepkts;                                 
	uint32_t bw_maxsizepkts;                                 
	uint32_t bw_minsize;                           
	uint32_t bw_maxsize;                           
	uint32_t bw_sndbw;                                   
	uint32_t bw_sndbw_max;                              
	uint32_t bw_rcvbw_max;                                      
};

                                   
struct mpt_dsn_map {
	uint64_t                mpt_dsn;                                
	uint32_t                mpt_sseq;                               
	uint16_t                mpt_len;                               
	uint16_t                mpt_csum;                                 
	uint8_t                 mpt_dfin;                            
};
#define tcp6cb          tcpcb                                     

struct tcp_ccstate {
	union {
		struct tcp_cubic_state {
			u_int32_t tc_last_max;                         
			u_int32_t tc_epoch_start;                      
			u_int32_t tc_origin_point;                                      
			u_int32_t tc_tcp_win;                       
			u_int32_t tc_tcp_bytes_acked;                  
			u_int32_t tc_avg_lastmax;                          
			u_int32_t tc_mean_deviation;                              
			float     tc_epoch_period;                  
		} _cubic_state_;
#define cub_last_max __u__._cubic_state_.tc_last_max
#define cub_epoch_start __u__._cubic_state_.tc_epoch_start
#define cub_origin_point __u__._cubic_state_.tc_origin_point
#define cub_tcp_win __u__._cubic_state_.tc_tcp_win
#define cub_tcp_bytes_acked __u__._cubic_state_.tc_tcp_bytes_acked
#define cub_epoch_period __u__._cubic_state_.tc_epoch_period
#define cub_avg_lastmax __u__._cubic_state_.tc_avg_lastmax
#define cub_mean_dev __u__._cubic_state_.tc_mean_deviation
	} __u__;
};

  
                                          
                                              
   
struct tcpcb {
	struct  tsegqe_head t_segq;
	int     t_dupacks;                                             
	int     t_state;                                              
	uint32_t t_timer[TCPT_NTIMERS];                 
	struct tcptimerentry tentry;                             

	struct  inpcb *t_inpcb;                                           
	uint32_t        t_flags;
#define TF_ACKNOW       0x00001                                   
#define TF_DELACK       0x00002                                       
#define TF_NODELAY      0x00004                                              
#define TF_NOOPT        0x00008                                    
#define TF_SENTFIN      0x00010                            
#define TF_REQ_SCALE    0x00020                                               
#define TF_RCVD_SCALE   0x00040                                               
#define TF_REQ_TSTMP    0x00080                                           
#define TF_RCVD_TSTMP   0x00100                                              
#define TF_SACK_PERMIT  0x00200                                           
#define TF_NEEDSYN      0x00400                                                                                        
#define TF_NEEDFIN      0x00800                                        
#define TF_NOPUSH       0x01000                         
#define TF_REQ_CC       0x02000                                   
#define TF_RCVD_CC      0x04000                                       
#define TF_SENDCCNEW    0x08000                     
#define TF_MORETOCOME   0x10000                                               
#define TF_LOCAL        0x20000                                                 
#define TF_RXWIN0SENT   0x40000                                                
#define TF_SLOWLINK     0x80000                                               
#define TF_LASTIDLE     0x100000                                            
#define TF_FASTRECOVERY 0x200000                                      
#define TF_WASFRECOVERY 0x400000                                          
#define TF_SIGNATURE    0x800000                                           
#define TF_MAXSEGSNT    0x1000000                                                 
#define TF_STREAMING_ON 0x2000000                                        
#define TF_PMTUD        0x4000000                                                           
#define TF_CLOSING      0x8000000                              
#define TF_TSO          0x10000000                                                               
#define TF_BLACKHOLE    0x20000000                                                   
#define TF_TIMER_ONLIST 0x40000000                                    
#define TF_STRETCHACK   0x80000000                                           

	tcp_seq snd_una;                                         
	tcp_seq snd_max;                                                
                                                                 
                                    
	tcp_seq snd_nxt;                               
	tcp_seq snd_up;                                          

	tcp_seq snd_wl1;                                                  
	tcp_seq snd_wl2;                                                  
	tcp_seq iss;                                                      
	tcp_seq irs;                                                         

	tcp_seq rcv_nxt;                                  
	tcp_seq rcv_adv;                                       
	uint32_t        rcv_wnd;                            
	uint32_t        t_last_recwin;
	tcp_seq rcv_up;                                             

	uint32_t        snd_wnd;                         
	uint32_t        snd_cwnd;                                         
	uint32_t        snd_ssthresh;                                 
                                                                 
                                                 
                                    
	tcp_seq snd_recover;                                                  

	uint32_t        t_maxopd;                             
	uint32_t        t_rcvtime;                                               
	uint32_t        t_sndtime;                                               
	uint32_t        t_starttime;                                         
	int     t_rtttime;                                                              
	tcp_seq t_rtseq;                                                 

	uint32_t rfbuf_ts;                                                     
	uint32_t rfbuf_cnt;                                                     
	uint32_t rfbuf_space;                                                      

	int            t_rxtcur;                                              
	unsigned int   t_maxseg;                                  
	int            t_srtt;                                        
	int            t_rttvar;                                         

	uint64_t t_accsleep_ms;                                                     
	uint16_t t_reassqlen;                                           
	uint16_t t_rxtshift;                                              
	uint32_t t_rttmin;                                       
	uint32_t t_rttbest;                                      
	uint32_t t_rttcur;                                            
	uint32_t t_rttupdated;                                           
	uint32_t t_rxt_conndroptime;                                                            
	uint32_t t_rxtstart;                                                      
	uint32_t max_sndwnd;                                                 

	int     t_softerror;                                                 
                      
	char    t_oobflags;                            
	char    t_iobc;                                      
#define TCPOOB_HAVEDATA 0x01
#define TCPOOB_HADDATA  0x02
                        
	u_int8_t        snd_scale;                                          
	u_int8_t        rcv_scale;                                          
	u_int8_t        request_r_scale;                             
	u_int8_t        requested_s_scale;
	u_int8_t        tcp_cc_index;                                              
	u_int8_t        t_adaptive_rtimo;                                                   
	u_int8_t        t_adaptive_wtimo;                                                    
	u_int8_t        t_stretchack_delayed;                            

                                                                   
	u_int16_t       t_early_rexmt_count;                            
	u_int32_t       t_early_rexmt_win;                                       

	u_int32_t       ts_recent;                               

	u_int32_t       ts_recent_age;                         
	tcp_seq last_ack_sent;
                        
	u_int32_t       t_bytes_acked;                                   

	int             t_lastchain;                                                    
	uint16_t        t_unacksegs;                                                         

	  
                              
                                                                      
                                       
    
#define TCP_FORCED_ACKS_COUNT 16
	uint16_t        t_forced_acks;                                                     
	uint8_t         t_rexmtthresh;                                                          
	uint8_t         t_rtimo_probes;                                           
	uint32_t        t_persist_timeout;                                                      
	uint32_t        t_persist_stop;                                                        
	uint32_t        t_notsent_lowat;                                    

                                              
	u_int32_t       rcv_unackwin;                                           
	u_int32_t       rcv_by_unackwin;                                                    
	u_int32_t       rcv_by_unackhalfwin;
	u_int32_t       rcv_nostrack_ts;                                                            
	u_int32_t       rcv_nostrack_pkts;                                                  
	u_int16_t       rcv_waitforss;                                          

               
	u_int16_t       ecn_flags;
#define TE_SETUPSENT            0x0001                                                      
#define TE_SETUPRECEIVED        0x0002                                                          
#define TE_SENDIPECT            0x0004                                                                         
#define TE_SENDCWR              0x0008                                                                              
#define TE_SENDECE              0x0010                                                                      
#define TE_INRECOVERY           0x0020                                                       
#define TE_RECV_ECN_CE          0x0040                                                  
#define TE_RECV_ECN_ECE         0x0080                                         
#define TE_LOST_SYN             0x0100                               
#define TE_LOST_SYNACK          0x0200                                   
#define TE_ECN_MODE_ENABLE      0x0400                                     
#define TE_ECN_MODE_DISABLE     0x0800                                      
#define TE_ENABLE_ECN           0x1000                                 
#define TE_ECN_ON               (TE_SETUPSENT | TE_SETUPRECEIVED)                                                                
#define TE_CEHEURI_SET          0x2000                                             
#define TE_CLIENT_SETUP         0x4000                              
#define TE_RCVD_SYN_RST         0x8000                                                 

	u_int32_t       t_ecn_recv_ce;                                    
	u_int32_t       t_ecn_recv_cwr;                                

                                       
	u_int32_t       snd_cwnd_prev;                                
	u_int32_t       snd_ssthresh_prev;                                   
	tcp_seq snd_recover_prev;                                            
	int     t_srtt_prev;                                          
	int     t_rttvar_prev;                                          
	u_int32_t       t_badrexmt_time;                               

                              
	u_int32_t       t_reorderwin;                                  

                        
	int16_t snd_numholes;                                               
	tcp_seq sack_newdata;                                               
                                                                          
	TAILQ_HEAD(sackhole_head, sackhole) snd_holes;
	                              
	tcp_seq snd_fack;                                                       
	int     rcv_numsacks;                                             
	struct sackblk sackblks[MAX_SACK_BLKS];                              
	struct sackhint sackhint;                                 
	tcp_seq         send_highest_sack;                                                                               
	int             t_new_dupacks;                                               

	struct mbuf     *t_pktlist_head;                                     
	struct mbuf     *t_pktlist_tail;                                    
	u_int32_t       t_pktlist_sentlen;                                    

	u_int32_t       t_keepidle;                                                        
	u_int32_t       t_keepinit;                                          
                                                                     
	u_int32_t       t_keepintvl;                                     
	u_int32_t       t_keepcnt;                                             

	u_int32_t       tso_max_segment_size;                                         
	u_int16_t       t_pmtud_lastseg_size;                                      
	u_int32_t       t_pmtud_saved_maxopd;                                                               
	u_int32_t       t_pmtud_start_ts;                                              

	struct{
		u_int32_t       rxduplicatebytes;
		u_int32_t       rxoutoforderbytes;
		u_int32_t       txretransmitbytes;
		u_int16_t       synrxtshift;
		u_int16_t       rxmitsyns;
		u_int16_t       unused_pad_to_8;
		u_int32_t       rxmitpkts;
		uint32_t        delayed_acks_sent;
		uint32_t        acks_delayed;
	} t_stat;
	u_int8_t        t_notify_ack_count;
	u_int8_t        t_ecn_recv_ce_pkt;                                                                       
	u_int32_t       t_cached_maxopd;                                                          

	uint32_t        bg_ssthresh;                                                            
	uint32_t        t_flagsext;                                                          
#define TF_RXTFINDROP           0x1                                                                     
#define TF_RCVUNACK_WAITSS      0x2                                                                
#define TF_BWMEAS_INPROGRESS    0x4                                                
#define TF_MEASURESNDBW         0x8                                                     
#define TF_LAST_IS_PSH          0x10                                                                                                  
#define TF_SACK_ENABLE          0x20                                 
#define TF_RECOMPUTE_RTT        0x40                                                         
#define TF_DETECT_READSTALL     0x80                                                              
#define TF_RECV_THROTTLE        0x100                                        
#define TF_NOSTRETCHACK         0x200                                       
#define TF_NOTIMEWAIT           0x800                                           
#define TF_SENT_TLPROBE         0x1000                                
#define TF_PKTS_REORDERED       0x2000                                   
#define TF_DELAY_RECOVERY       0x4000                                   
#define TF_FORCE                0x8000                                
#define TF_DISABLE_STRETCHACK   0x10000                                       
#define TF_NOBLACKHOLE_DETECTION 0x20000                                              
#define TF_RESCUE_RXT           0x80000                                     
#define TF_CWND_NONVALIDATED    0x100000                                
#define TF_IF_PROBING           0x200000                                             
#define TF_FASTOPEN             0x400000                                     
#define TF_REASS_INPROG         0x800000                                       
#define TF_FASTOPEN_FORCE_ENABLE 0x1000000                                     
#define TF_LOGGED_CONN_SUMMARY  0x2000000                                          

#if TRAFFIC_MGT
	                                        
	uint32_t        iaj_rcv_ts;                                                               
	uint16_t        iaj_size;                                                       
	uint8_t         iaj_small_pkt;                                                      
	uint8_t         t_pipeack_ind;                                             
	uint16_t        iaj_pktcnt;                                                              
	uint16_t        acc_iaj;                                     
	uint32_t        avg_iaj;                          
	uint32_t        std_dev_iaj;                                    
#endif                  
	struct bwmeas   *t_bwmeas;                                                   
	tcp_seq         t_idleat;                                         
	TAILQ_ENTRY(tcpcb) t_twentry;                                         
	struct tcp_ccstate      *t_ccstate;                                           
                                   
	tcp_seq         t_tlphighrxt;                                  
	u_int32_t       t_tlpstart;                                   
                               
	tcp_seq         t_dsack_lseq;                                    
	tcp_seq         t_dsack_rseq;                                     
                             
	SLIST_HEAD(tcp_rxt_seghead, tcp_rxt_seg) t_rxt_segments;
	tcp_seq         t_dsack_lastuna;                                                        
                                                                        
#define TCP_PIPEACK_SAMPLE_COUNT        3
	u_int32_t       t_pipeack_sample[TCP_PIPEACK_SAMPLE_COUNT];                                          
	tcp_seq         t_pipeack_lastuna;                                           
	u_int32_t       t_pipeack;
	u_int32_t       t_lossflightsize;

#if MPTCP
	u_int32_t       t_mpflags;                                           

#define TMPF_PREESTABLISHED     0x00000001                                    
#define TMPF_SND_KEYS           0x00000002                                         
#define TMPF_MPTCP_TRUE         0x00000004                                    
#define TMPF_MPTCP_RCVD_KEY     0x00000008                                
#define TMPF_SND_MPPRIO         0x00000010                               
#define TMPF_SND_REM_ADDR       0x00000020                               
#define TMPF_RCVD_DACK          0x00000040                          
#define TMPF_JOINED_FLOW        0x00000080                                
#define TMPF_BACKUP_PATH        0x00000100                            
#define TMPF_MPTCP_ACKNOW       0x00000200                    
#define TMPF_SEND_DSN           0x00000400                       
#define TMPF_SEND_DFIN          0x00000800                    
#define TMPF_RECV_DFIN          0x00001000                    
#define TMPF_SENT_JOIN          0x00002000                
#define TMPF_RECVD_JOIN         0x00004000                    
#define TMPF_RESET              0x00008000               
#define TMPF_TCP_FALLBACK       0x00010000                      
#define TMPF_FASTCLOSERCV       0x00020000                                
#define TMPF_EMBED_DSN          0x00040000                         
#define TMPF_MPTCP_READY        0x00080000                                   
#define TMPF_INFIN_SENT         0x00100000                            
#define TMPF_SND_MPFAIL         0x00200000                                    
#define TMPF_SND_JACK           0x00400000                      
#define TMPF_TFO_REQUEST        0x00800000                    

#define TMPF_MPTCP_SIGNALS      (TMPF_SND_MPPRIO | TMPF_SND_REM_ADDR | TMPF_SND_MPFAIL | TMPF_SND_KEYS | TMPF_SND_JACK)

	tcp_seq                 t_mpuna;                                     
	struct mptcb            *t_mptcb;                                 
	struct mptsub           *t_mpsub;                                         
	struct mpt_dsn_map      t_rcv_map;                                
	u_int8_t                t_local_aid;                                    
	u_int8_t                t_rem_aid;                                      
	u_int8_t                t_mprxtshift;                            
#endif            

#define TFO_F_OFFER_COOKIE      0x01                             
#define TFO_F_COOKIE_VALID      0x02                                   
#define TFO_F_COOKIE_REQ        0x04                                    
#define TFO_F_COOKIE_SENT       0x08                                          
#define TFO_F_SYN_LOSS          0x10                                                                        
#define TFO_F_NO_SNDPROBING     0x20                                                                          
#define TFO_F_HEURISTIC_DONE    0x40                                                 
	u_int8_t                t_tfo_flags;
#define TFO_S_SYNDATA_RCV       0x01                                 
#define TFO_S_COOKIEREQ_RECV    0x02                                  
#define TFO_S_COOKIE_SENT       0x04                                      
#define TFO_S_COOKIE_INVALID    0x08                                     
#define TFO_S_COOKIE_REQ        0x10                                          
#define TFO_S_COOKIE_RCV        0x20                                     
#define TFO_S_SYN_DATA_SENT     0x40                    
#define TFO_S_SYN_DATA_ACKED    0x80                                                
#define TFO_S_SYN_LOSS          0x0100                                                      
#define TFO_S_COOKIE_WRONG      0x0200                                          
#define TFO_S_NO_COOKIE_RCV     0x0400                                               
#define TFO_S_HEURISTICS_DISABLE 0x0800                                                     
#define TFO_S_SEND_BLACKHOLE    0x1000                                               
#define TFO_S_RECV_BLACKHOLE    0x2000                                               
#define TFO_S_ONE_BYTE_PROXY    0x4000                                                                
	u_int16_t               t_tfo_stats;

	u_int8_t                t_tfo_probes;                             
  
                                                                          
  
                            
                                  
                                                                 
  
                                               
                         
                                                                 
                                                      
                                                                 
                                              
                                                                 
                                     
                                                            
                                                               
  
                                 
                         
                                                                 
                                                             
                                                                
   
#define TFO_PROBE_NONE          0                      
#define TFO_PROBE_PROBING       1                                                   
#define TFO_PROBE_WAIT_DATA     2                                       
	u_int8_t                t_tfo_probe_state;

	u_int32_t       t_rcvoopack;                                               
	u_int32_t       t_pawsdrop;                                               
	u_int32_t       t_sack_recovery_episode;                             
	u_int32_t       t_reordered_pkts;                             
	u_int32_t       t_dsack_sent;                                        
	u_int32_t       t_dsack_recvd;                                             
	SLIST_HEAD(, tcp_notify_ack_marker) t_notify_ack;                                                
	u_int32_t       t_recv_throttle_ts;                                        
	u_int32_t       t_rxt_minimum_timeout;                                        
	uint32_t        t_challengeack_last;                                                  
	uint32_t        t_challengeack_count;                                                 

	u_int32_t       t_log_flags;                                  
	u_int32_t       t_connect_time;                                               

	uint32_t        t_comp_gencnt;                                           
	uint32_t        t_comp_lastinc;                                                                                        
#define TCP_COMP_CHANGE_RATE    5                                                                                                                   

	uint32_t        t_ts_offset;                                                                

	uuid_t          t_fsw_uuid;
	uuid_t          t_flow_uuid;
};

#define IN_FASTRECOVERY(tp)     (tp->t_flags & TF_FASTRECOVERY)
#define SACK_ENABLED(tp)        (tp->t_flagsext & TF_SACK_ENABLE)

  
                                                                          
                                                                    
                                                                 
                                                                    
                                                         
   
#define ENTER_FASTRECOVERY(_tp_) do {                                           \
	(_tp_)->t_flags |= TF_FASTRECOVERY;                                     \
	if (INP_IS_FLOW_CONTROLLED((_tp_)->t_inpcb))                            \
	        inp_reset_fc_state((_tp_)->t_inpcb);                            \
	if (!SLIST_EMPTY(&tp->t_rxt_segments))                                  \
	        tcp_rxtseg_clean(tp);                                           \
	(_tp_)->t_new_dupacks = 0;                                              \
} while(0)

#define EXIT_FASTRECOVERY(_tp_) do {                                            \
	(_tp_)->t_flags &= ~TF_FASTRECOVERY;                                    \
	(_tp_)->t_dupacks = 0;                                                  \
	(_tp_)->t_new_dupacks = 0;                                              \
	(_tp_)->t_rexmtthresh = (uint8_t)tcprexmtthresh;                        \
	(_tp_)->t_bytes_acked = 0;                                              \
	(_tp_)->ecn_flags &= ~TE_INRECOVERY;                                    \
	(_tp_)->t_timer[TCPT_PTO] = 0;                                          \
	(_tp_)->t_flagsext &= ~TF_RESCUE_RXT;                                   \
	(_tp_)->t_lossflightsize = 0;                                           \
	(_tp_)->sackhint.sack_bytes_acked = 0;                                  \
} while(0)

  
                                                          
                                                           
   
extern int tcprexmtthresh;
#define ALLOW_LIMITED_TRANSMIT(_tp_)                                            \
	((_tp_)->t_dupacks > 0 &&                                               \
	(_tp_)->t_dupacks < (_tp_)->t_rexmtthresh &&                            \
	((_tp_)->t_flagsext & (TF_PKTS_REORDERED|TF_DELAY_RECOVERY))            \
	    != (TF_PKTS_REORDERED|TF_DELAY_RECOVERY))

  
                                                          
                   
   
#define TSTMP_SUPPORTED(_tp_) \
	(((_tp_)->t_flags & (TF_REQ_TSTMP|TF_RCVD_TSTMP)) == \
	        (TF_REQ_TSTMP|TF_RCVD_TSTMP))

  
                                                             
                  
   
#define TCP_WINDOW_SCALE_ENABLED(_tp_) \
	(((_tp_)->t_flags & (TF_RCVD_SCALE|TF_REQ_SCALE)) == \
	        (TF_RCVD_SCALE|TF_REQ_SCALE))

                               
#define TCP_ECN_ENABLED(_tp_) \
	(((_tp_)->ecn_flags & (TE_ECN_ON)) == (TE_ECN_ON))

  
                                          
   
#define BYTES_ACKED(_th_, _tp_) \
	((_th_)->th_ack - (_tp_)->snd_una)

                                                   
#define TCP_SEND_DSACK_OPT(_tp_) \
	((_tp_)->t_dsack_lseq > 0 && (_tp_)->t_dsack_rseq > 0)

  
                                                                           
                                                                          
                                                                       
  
                                                                        
                                                                      
                                                                          
                                                                       
                                                     
   
#define TCP_DSACK_MAX_SEND_WINDOW(_tp_) (MIN((_tp_)->snd_wnd, tcp_autosndbuf_max))
#define TCP_DSACK_SEQ_IN_WINDOW(_tp_, _seq_, _una_) \
    (SEQ_LEQ((_seq_), (_tp_)->snd_max) && \
    SEQ_GEQ((_seq_), ((_una_) - TCP_DSACK_MAX_SEND_WINDOW(_tp_))))

#define TCP_RESET_REXMT_STATE(_tp_) do { \
	(_tp_)->t_rxtshift = 0; \
	(_tp_)->t_rxtstart = 0; \
	mptcp_reset_rexmit_state((_tp_)); \
} while(0);

#define TCP_IF_STATE_CHANGED(tp, probe_if_index)                        \
	(probe_if_index > 0 && tp->t_inpcb->inp_last_outifp != NULL &&  \
	probe_if_index == tp->t_inpcb->inp_last_outifp->if_index)

  
                                                                  
                                                        
                                                         
                    
   
struct tcpopt {
	u_int32_t       to_flags;                                              
#define TOF_TS          0x0001                         
#define TOF_MSS         0x0010
#define TOF_SCALE       0x0020
#define TOF_SIGNATURE   0x0040                                
#define TOF_SIGLEN      0x0080                                        
#define TOF_SACK        0x0100                                     
#define TOF_MPTCP       0x0200                                   
#define TOF_TFO         0x0400                                 
#define TOF_TFOREQ      0x0800                                  
	u_int32_t               to_tsval;
	u_int32_t               to_tsecr;
	u_int16_t       to_mss;
	u_int8_t        to_requested_s_scale;
	u_int8_t        to_nsacks;                                 
	u_char          *to_sacks;                                            
	u_char          *to_tfo;                                       
};

#define intotcpcb(ip)   ((struct tcpcb *)(ip)->inp_ppcb)
#define sototcpcb(so)   (intotcpcb(sotoinpcb(so)))

                          
#define TFO_COOKIE_LEN_MIN      4
#define TFO_COOKIE_LEN_DEFAULT  8
#define TFO_COOKIE_LEN_MAX      16

  
                                                                
                                                          
                                                        
                                                         
                                                        
                                                          
                                              
                                                        
                                                 
                                                             
                                                            
                                                            
                                                             
                 
   
#define TCP_REXMTVAL(tp) \
	max((tp)->t_rttmin, (((tp)->t_srtt >> (TCP_RTT_SHIFT - TCP_DELTA_SHIFT))  \
	  + (tp)->t_rttvar) >> TCP_DELTA_SHIFT)

  
                                                  
                               
   
struct otcpcb {
#else
struct tseg_qent;
_TCPCB_LIST_HEAD(tsegqe_head, tseg_qent);

struct tcpcb {
#endif                     
#if defined(KERNEL_PRIVATE)
	u_int32_t t_segq;
#else
	struct  tsegqe_head t_segq;
#endif                     
	int     t_dupacks;                                             
	u_int32_t unused;                                               

	int     t_timer[TCPT_NTIMERS_EXT];                      

	_TCPCB_PTR(struct inpcb *) t_inpcb;                                       
	int     t_state;                                              
	u_int   t_flags;
#define TF_ACKNOW       0x00001                                   
#define TF_DELACK       0x00002                                       
#define TF_NODELAY      0x00004                                              
#define TF_NOOPT        0x00008                                    
#define TF_SENTFIN      0x00010                            
#define TF_REQ_SCALE    0x00020                                               
#define TF_RCVD_SCALE   0x00040                                               
#define TF_REQ_TSTMP    0x00080                                           
#define TF_RCVD_TSTMP   0x00100                                              
#define TF_SACK_PERMIT  0x00200                                           
#define TF_NEEDSYN      0x00400                                                                                        
#define TF_NEEDFIN      0x00800                                        
#define TF_NOPUSH       0x01000                         
#define TF_REQ_CC       0x02000                                   
#define TF_RCVD_CC      0x04000                                       
#define TF_SENDCCNEW    0x08000                              
#define TF_MORETOCOME   0x10000                                               
#define TF_LQ_OVERFLOW  0x20000                                    
#define TF_RXWIN0SENT   0x40000                                                
#define TF_SLOWLINK     0x80000                                               

	int     t_force;                                             

	tcp_seq snd_una;                                         
	tcp_seq snd_max;                                                
                                                                 
                                    
	tcp_seq snd_nxt;                               
	tcp_seq snd_up;                                          

	tcp_seq snd_wl1;                                                  
	tcp_seq snd_wl2;                                                  
	tcp_seq iss;                                                      
	tcp_seq irs;                                                         

	tcp_seq rcv_nxt;                                  
	tcp_seq rcv_adv;                                       
	u_int32_t rcv_wnd;                                  
	tcp_seq rcv_up;                                             

	u_int32_t snd_wnd;                               
	u_int32_t snd_cwnd;                                               
	u_int32_t snd_ssthresh;                                       
                                                                 
                                                 
                                    
	u_int   t_maxopd;                                     

	u_int32_t t_rcvtime;                                                     
	u_int32_t t_starttime;                                               
	int     t_rtttime;                                   
	tcp_seq t_rtseq;                                                 

	int     t_rxtcur;                                                     
	u_int   t_maxseg;                                         
	int     t_srtt;                                               
	int     t_rttvar;                                                

	int     t_rxtshift;                                               
	u_int   t_rttmin;                                        
	u_int32_t t_rttupdated;                                          
	u_int32_t max_sndwnd;                                                

	int     t_softerror;                                                 
                      
	char    t_oobflags;                            
	char    t_iobc;                                      
#define TCPOOB_HAVEDATA 0x01
#define TCPOOB_HADDATA  0x02
                        
	u_char  snd_scale;                                                  
	u_char  rcv_scale;                                                  
	u_char  request_r_scale;                                    
	u_char  requested_s_scale;
	u_int32_t ts_recent;                                     

	u_int32_t ts_recent_age;                               
	tcp_seq last_ack_sent;
                        
	tcp_cc  cc_send;                                           
	tcp_cc  cc_recv;                                              
	tcp_seq snd_recover;                                          
                  
	u_int32_t snd_cwnd_prev;                                      
	u_int32_t snd_ssthresh_prev;                                      
	u_int32_t t_badrxtwin;                                              
};

#define        tcps_ecn_setup  tcps_ecn_client_success
#define        tcps_sent_cwr   tcps_ecn_recv_ece
#define        tcps_sent_ece   tcps_ecn_sent_ece

  
                  
                                               
                                         
   
struct  tcpstat {
	u_int32_t       tcps_connattempt;                                  
	u_int32_t       tcps_accepts;                                     
	u_int32_t       tcps_connects;                                       
	u_int32_t       tcps_drops;                                      
	u_int32_t       tcps_conndrops;                                            
	u_int32_t       tcps_closed;                                               
	u_int32_t       tcps_segstimed;                                             
	u_int32_t       tcps_rttupdated;                                
	u_int32_t       tcps_delack;                                   
	u_int32_t       tcps_timeoutdrop;                                          
	u_int32_t       tcps_rexmttimeo;                                 
	u_int32_t       tcps_persisttimeo;                            
	u_int32_t       tcps_keeptimeo;                                 
	u_int32_t       tcps_keepprobe;                                    
	u_int32_t       tcps_keepdrops;                                               

	u_int32_t       tcps_sndtotal;                                  
	u_int32_t       tcps_sndpack;                                  
	u_int32_t       tcps_sndbyte;                                
	u_int32_t       tcps_sndrexmitpack;                                     
	u_int32_t       tcps_sndrexmitbyte;                                   
	u_int32_t       tcps_sndacks;                                      
	u_int32_t       tcps_sndprobe;                                  
	u_int32_t       tcps_sndurg;                                            
	u_int32_t       tcps_sndwinup;                                               
	u_int32_t       tcps_sndctrl;                                                   

	u_int32_t       tcps_rcvtotal;                                      
	u_int32_t       tcps_rcvpack;                                             
	u_int32_t       tcps_rcvbyte;                                           
	u_int32_t       tcps_rcvbadsum;                                                
	u_int32_t       tcps_rcvbadoff;                                               
	u_int32_t       tcps_rcvmemdrop;                                                
	u_int32_t       tcps_rcvshort;                                          
	u_int32_t       tcps_rcvduppack;                                             
	u_int32_t       tcps_rcvdupbyte;                                           
	u_int32_t       tcps_rcvpartduppack;                                          
	u_int32_t       tcps_rcvpartdupbyte;                                         
	u_int32_t       tcps_rcvoopack;                                            
	u_int32_t       tcps_rcvoobyte;                                          
	u_int32_t       tcps_rcvpackafterwin;                                       
	u_int32_t       tcps_rcvbyteafterwin;                                
	u_int32_t       tcps_rcvafterclose;                                     
	u_int32_t       tcps_rcvwinprobe;                                      
	u_int32_t       tcps_rcvdupack;                                  
	u_int32_t       tcps_rcvacktoomuch;                                    
	u_int32_t       tcps_rcvackpack;                              
	u_int32_t       tcps_rcvackbyte;                                      
	u_int32_t       tcps_rcvwinupd;                                         
	u_int32_t       tcps_pawsdrop;                                            
	u_int32_t       tcps_predack;                                              
	u_int32_t       tcps_preddat;                                                   
	u_int32_t       tcps_pcbcachemiss;
	u_int32_t       tcps_cachedrtt;                                                
	u_int32_t       tcps_cachedrttvar;                                       
	u_int32_t       tcps_cachedssthresh;                                       
	u_int32_t       tcps_usedrtt;                                                 
	u_int32_t       tcps_usedrttvar;                                              
	u_int32_t       tcps_usedssthresh;                                             
	u_int32_t       tcps_persistdrop;                                     
	u_int32_t       tcps_badsyn;                                               
	u_int32_t       tcps_mturesent;                                           
	u_int32_t       tcps_listendrop;                                    
	u_int32_t       tcps_synchallenge;                                        
	u_int32_t       tcps_rstchallenge;                                        

	                                        
	u_int32_t       tcps_minmssdrops;                                         

	u_int32_t       tcps_sndrexmitbad;                                              
	u_int32_t       tcps_badrst;                                            

	u_int32_t       tcps_sc_added;                                       
	u_int32_t       tcps_sc_retransmitted;                                        
	u_int32_t       tcps_sc_dupsyn;                                   
	u_int32_t       tcps_sc_dropped;                                       
	u_int32_t       tcps_sc_completed;                                          
	u_int32_t       tcps_sc_bucketoverflow;                                    
	u_int32_t       tcps_sc_cacheoverflow;                                
	u_int32_t       tcps_sc_reset;                                               
	u_int32_t       tcps_sc_stale;                                               
	u_int32_t       tcps_sc_aborted;                                    
	u_int32_t       tcps_sc_badack;                                     
	u_int32_t       tcps_sc_unreach;                                       
	u_int32_t       tcps_sc_zonefail;                            
	u_int32_t       tcps_sc_sendcookie;                          
	u_int32_t       tcps_sc_recvcookie;                              

	u_int32_t       tcps_hc_added;                                        
	u_int32_t       tcps_hc_bucketoverflow;                                     

	                        
	u_int32_t       tcps_sack_recovery_episode;                             
	u_int32_t       tcps_sack_rexmits;                                      
	u_int32_t       tcps_sack_rexmit_bytes;                                 
	u_int32_t       tcps_sack_rcv_blocks;                                           
	u_int32_t       tcps_sack_send_blocks;                                          
	u_int32_t       tcps_sack_sboverflow;                                      

	u_int32_t       tcps_bg_rcvtotal;                                              
	u_int32_t       tcps_rxtfindrop;                                                
	u_int32_t       tcps_fcholdpacket;                                                    

	u_int32_t       tcps_limited_txt;                                  
	u_int32_t       tcps_early_rexmt;                                  
	u_int32_t       tcps_sack_ackadv;                                                    

	                            
	u_int32_t       tcps_rcv_swcsum;                                            
	u_int32_t       tcps_rcv_swcsum_bytes;                                    
	u_int32_t       tcps_rcv6_swcsum;                                            
	u_int32_t       tcps_rcv6_swcsum_bytes;                                    
	u_int32_t       tcps_snd_swcsum;                                             
	u_int32_t       tcps_snd_swcsum_bytes;                                     
	u_int32_t       tcps_snd6_swcsum;                                             
	u_int32_t       tcps_snd6_swcsum_bytes;                                     
	u_int32_t       tcps_unused_1;
	u_int32_t       tcps_unused_2;
	u_int32_t       tcps_unused_3;

	                         
	u_int32_t       tcps_invalid_mpcap;                                     
	u_int32_t       tcps_invalid_joins;                              
	u_int32_t       tcps_mpcap_fallback;                                 
	u_int32_t       tcps_join_fallback;                                
	u_int32_t       tcps_estab_fallback;                            
	u_int32_t       tcps_invalid_opt;                                
	u_int32_t       tcps_mp_outofwin;                                 
                                                                  
	u_int32_t       tcps_mp_reducedwin;                                 
	u_int32_t       tcps_mp_badcsum;                          
	u_int32_t       tcps_mp_oodata;                                
	u_int32_t       tcps_mp_switches;                                     
	u_int32_t       tcps_mp_rcvtotal;                                   
	u_int32_t       tcps_mp_rcvbytes;                                     
	u_int32_t       tcps_mp_sndpacks;                                      
	u_int32_t       tcps_mp_sndbytes;                                 
	u_int32_t       tcps_join_rxmts;                                  
	u_int32_t       tcps_tailloss_rto;                                
	u_int32_t       tcps_reordered_pkts;                          
	u_int32_t       tcps_recovered_pkts;                              
	u_int32_t       tcps_pto;                                  
	u_int32_t       tcps_rto_after_pto;                            
	u_int32_t       tcps_tlp_recovery;                                     
	u_int32_t       tcps_tlp_recoverlastpkt;                            
	u_int32_t       tcps_ecn_client_success;                                            
	u_int32_t       tcps_ecn_recv_ece;                                  
	u_int32_t       tcps_ecn_sent_ece;                                 
	u_int32_t       tcps_detect_reordering;                            
	u_int32_t       tcps_delay_recovery;                             
	u_int32_t       tcps_avoid_rxmt;                                        
	u_int32_t       tcps_unnecessary_rxmt;                                     
	u_int32_t       tcps_nostretchack;                                                          
	u_int32_t       tcps_rescue_rxmt;                                   
	u_int32_t       tcps_pto_in_recovery;                                           
	u_int32_t       tcps_pmtudbh_reverted;                                                       

	                              
	u_int32_t       tcps_dsack_disable;                                                
	u_int32_t       tcps_dsack_ackloss;                                       
	u_int32_t       tcps_dsack_badrexmt;                                        
	u_int32_t       tcps_dsack_sent;                                     
	u_int32_t       tcps_dsack_recvd;                                          
	u_int32_t       tcps_dsack_recvd_old;                                               

	                                   
	u_int32_t       tcps_mp_sel_symtomsd;                     
	u_int32_t       tcps_mp_sel_rtt;                               
	u_int32_t       tcps_mp_sel_rto;                               
	u_int32_t       tcps_mp_sel_peer;                                     
	u_int32_t       tcps_mp_num_probes;                                
	u_int32_t       tcps_mp_verdowngrade;                                
	u_int32_t       tcps_drop_after_sleep;                                
	u_int32_t       tcps_probe_if;                                                          
	u_int32_t       tcps_probe_if_conflict;                                             

	u_int32_t       tcps_ecn_client_setup;                                            
	u_int32_t       tcps_ecn_server_setup;                                            
	u_int32_t       tcps_ecn_server_success;                                            
	u_int32_t       tcps_ecn_lost_synack;                                    
	u_int32_t       tcps_ecn_lost_syn;                                   
	u_int32_t       tcps_ecn_not_supported;                                       
	u_int32_t       tcps_ecn_recv_ce;                                         
	u_int32_t       tcps_ecn_conn_recv_ce;                                                      
	u_int32_t       tcps_ecn_conn_recv_ece;                                                      
	u_int32_t       tcps_ecn_conn_plnoce;                                                                            
	u_int32_t       tcps_ecn_conn_pl_ce;                                                                          
	u_int32_t       tcps_ecn_conn_nopl_ce;                                                                           
	u_int32_t       tcps_ecn_fallback_synloss;                                                       
	u_int32_t       tcps_ecn_fallback_reorder;                                                                     
	u_int32_t       tcps_ecn_fallback_ce;                                                                     

	                            
	u_int32_t       tcps_tfo_syn_data_rcv;                                             
	u_int32_t       tcps_tfo_cookie_req_rcv;                                   
	u_int32_t       tcps_tfo_cookie_sent;                                           
	u_int32_t       tcps_tfo_cookie_invalid;                                    
	u_int32_t       tcps_tfo_cookie_req;                                       
	u_int32_t       tcps_tfo_cookie_rcv;                                      
	u_int32_t       tcps_tfo_syn_data_sent;                           
	u_int32_t       tcps_tfo_syn_data_acked;                                    
	u_int32_t       tcps_tfo_syn_loss;                                                 
	u_int32_t       tcps_tfo_blackhole;                                             
	u_int32_t       tcps_tfo_cookie_wrong;                                    
	u_int32_t       tcps_tfo_no_cookie_rcv;                                               
	u_int32_t       tcps_tfo_heuristics_disable;                                         
	u_int32_t       tcps_tfo_sndblackhole;                                                   
	u_int32_t       tcps_mss_to_default;                                                        
	u_int32_t       tcps_mss_to_medium;                                                        
	u_int32_t       tcps_mss_to_low;                                                        
	u_int32_t       tcps_ecn_fallback_droprst;                                                        
	u_int32_t       tcps_ecn_fallback_droprxmt;                                                          
	u_int32_t       tcps_ecn_fallback_synrst;                                        

	u_int32_t       tcps_mptcp_rcvmemdrop;                                                
	u_int32_t       tcps_mptcp_rcvduppack;                                             
	u_int32_t       tcps_mptcp_rcvpackafterwin;                                           

	                          
	u_int32_t       tcps_timer_drift_le_1_ms;                                              
	u_int32_t       tcps_timer_drift_le_10_ms;                                              
	u_int32_t       tcps_timer_drift_le_20_ms;                                              
	u_int32_t       tcps_timer_drift_le_50_ms;                                              
	u_int32_t       tcps_timer_drift_le_100_ms;                                              
	u_int32_t       tcps_timer_drift_le_200_ms;                                              
	u_int32_t       tcps_timer_drift_le_500_ms;                                              
	u_int32_t       tcps_timer_drift_le_1000_ms;                                              
	u_int32_t       tcps_timer_drift_gt_1000_ms;                                          

	u_int32_t       tcps_mptcp_handover_attempt;                                                            
	u_int32_t       tcps_mptcp_interactive_attempt;                                                            
	u_int32_t       tcps_mptcp_aggregate_attempt;                                                            
	u_int32_t       tcps_mptcp_fp_handover_attempt;                                                           
	u_int32_t       tcps_mptcp_fp_interactive_attempt;
	u_int32_t       tcps_mptcp_fp_aggregate_attempt;
	u_int32_t       tcps_mptcp_heuristic_fallback;                                                                          
	u_int32_t       tcps_mptcp_fp_heuristic_fallback;                                                      
	u_int32_t       tcps_mptcp_handover_success_wifi;                                                                                         
	u_int32_t       tcps_mptcp_handover_success_cell;                                                                                         
	u_int32_t       tcps_mptcp_interactive_success;                                                                                 
	u_int32_t       tcps_mptcp_aggregate_success;                                                   
	u_int32_t       tcps_mptcp_fp_handover_success_wifi;                                                         
	u_int32_t       tcps_mptcp_fp_handover_success_cell;
	u_int32_t       tcps_mptcp_fp_interactive_success;
	u_int32_t       tcps_mptcp_fp_aggregate_success;
	u_int32_t       tcps_mptcp_handover_cell_from_wifi;                                                                                        
	u_int32_t       tcps_mptcp_handover_wifi_from_cell;                                                                                        
	u_int32_t       tcps_mptcp_interactive_cell_from_wifi;                                                                                        
	u_int64_t       tcps_mptcp_handover_cell_bytes;                                                                                                          
	u_int64_t       tcps_mptcp_interactive_cell_bytes;                                                
	u_int64_t       tcps_mptcp_aggregate_cell_bytes;
	u_int64_t       tcps_mptcp_handover_all_bytes;                                                      
	u_int64_t       tcps_mptcp_interactive_all_bytes;
	u_int64_t       tcps_mptcp_aggregate_all_bytes;
	u_int32_t       tcps_mptcp_back_to_wifi;                                                                                                             
	u_int32_t       tcps_mptcp_wifi_proxy;                                                                                  
	u_int32_t       tcps_mptcp_cell_proxy;                                                                                  

	                            
	u_int32_t       tcps_ka_offload_drops;                                                         

	u_int32_t       tcps_mptcp_triggered_cell;                                                                            
};


struct tcpstat_local {
	u_int64_t badformat;
	u_int64_t unspecv6;
	u_int64_t synfin;
	u_int64_t badformatipsec;
	u_int64_t noconnnolist;
	u_int64_t noconnlist;
	u_int64_t listbadsyn;
	u_int64_t icmp6unreach;
	u_int64_t deprecate6;
	u_int64_t ooopacket;
	u_int64_t rstinsynrcv;
	u_int64_t dospacket;
	u_int64_t cleanup;
	u_int64_t synwindow;
};

#pragma pack(4)

  
                                                     
                                                                    
                                        
   

struct  xtcpcb {
	u_int32_t       xt_len;
#ifdef KERNEL_PRIVATE
	struct  inpcb_compat    xt_inp;
#else
	struct  inpcb   xt_inp;
#endif
#ifdef KERNEL_PRIVATE
	struct  otcpcb  xt_tp;
#else
	struct  tcpcb   xt_tp;
#endif
	struct  xsocket xt_socket;
	u_quad_t        xt_alignment_hack;
};

#if XNU_TARGET_OS_OSX || KERNEL || !(TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR)

struct  xtcpcb64 {
	u_int32_t               xt_len;
	struct xinpcb64         xt_inpcb;

	u_int64_t t_segq;
	int     t_dupacks;                                             

	int t_timer[TCPT_NTIMERS_EXT];                  

	int     t_state;                                              
	u_int   t_flags;

	int     t_force;                                             

	tcp_seq snd_una;                                         
	tcp_seq snd_max;                                                
                                                                 
                                    
	tcp_seq snd_nxt;                               
	tcp_seq snd_up;                                          

	tcp_seq snd_wl1;                                                  
	tcp_seq snd_wl2;                                                  
	tcp_seq iss;                                                      
	tcp_seq irs;                                                         

	tcp_seq rcv_nxt;                                  
	tcp_seq rcv_adv;                                       
	u_int32_t rcv_wnd;                                  
	tcp_seq rcv_up;                                             

	u_int32_t snd_wnd;                               
	u_int32_t snd_cwnd;                                               
	u_int32_t snd_ssthresh;                                       
                                                                 
                                                 
                                    
	u_int   t_maxopd;                                     

	u_int32_t t_rcvtime;                                                     
	u_int32_t t_starttime;                                               
	int     t_rtttime;                                   
	tcp_seq t_rtseq;                                                 

	int     t_rxtcur;                                                     
	u_int   t_maxseg;                                         
	int     t_srtt;                                               
	int     t_rttvar;                                                

	int     t_rxtshift;                                               
	u_int   t_rttmin;                                        
	u_int32_t t_rttupdated;                                          
	u_int32_t max_sndwnd;                                                

	int     t_softerror;                                                 
                      
	char    t_oobflags;                            
	char    t_iobc;                                      
                        
	u_char  snd_scale;                                                  
	u_char  rcv_scale;                                                  
	u_char  request_r_scale;                                    
	u_char  requested_s_scale;
	u_int32_t ts_recent;                                     

	u_int32_t ts_recent_age;                               
	tcp_seq last_ack_sent;
                        
	tcp_cc  cc_send;                                           
	tcp_cc  cc_recv;                                              
	tcp_seq snd_recover;                                          
                  
	u_int32_t snd_cwnd_prev;                                      
	u_int32_t snd_ssthresh_prev;                                      
	u_int32_t t_badrxtwin;                                              

	u_quad_t                xt_alignment_hack;
};

#endif                                                                                 

#ifdef PRIVATE

struct  xtcpcb_n {
	u_int32_t               xt_len;
	u_int32_t                       xt_kind;                               

	u_int64_t t_segq;
	int     t_dupacks;                                             

	int t_timer[TCPT_NTIMERS_EXT];                  

	int     t_state;                                              
	u_int   t_flags;

	int     t_force;                                             

	tcp_seq snd_una;                                         
	tcp_seq snd_max;                                                
                                                                 
                                    
	tcp_seq snd_nxt;                               
	tcp_seq snd_up;                                          

	tcp_seq snd_wl1;                                                  
	tcp_seq snd_wl2;                                                  
	tcp_seq iss;                                                      
	tcp_seq irs;                                                         

	tcp_seq rcv_nxt;                                  
	tcp_seq rcv_adv;                                       
	u_int32_t rcv_wnd;                                  
	tcp_seq rcv_up;                                             

	u_int32_t snd_wnd;                               
	u_int32_t snd_cwnd;                                               
	u_int32_t snd_ssthresh;                                       
                                                                 
                                                 
                                    
	u_int   t_maxopd;                                     

	u_int32_t t_rcvtime;                                                     
	u_int32_t t_starttime;                                               
	int     t_rtttime;                                   
	tcp_seq t_rtseq;                                                 

	int     t_rxtcur;                                                     
	u_int   t_maxseg;                                         
	int     t_srtt;                                               
	int     t_rttvar;                                                

	int     t_rxtshift;                                               
	u_int   t_rttmin;                                        
	u_int32_t t_rttupdated;                                          
	u_int32_t max_sndwnd;                                                

	int     t_softerror;                                                 
	                      
	char    t_oobflags;                            
	char    t_iobc;                                      
	                        
	u_char  snd_scale;                                                  
	u_char  rcv_scale;                                                  
	u_char  request_r_scale;                                    
	u_char  requested_s_scale;
	u_int32_t ts_recent;                                     

	u_int32_t ts_recent_age;                               
	tcp_seq last_ack_sent;
	                        
	tcp_cc  cc_send;                                           
	tcp_cc  cc_recv;                                              
	tcp_seq snd_recover;                                          
	                  
	u_int32_t snd_cwnd_prev;                                      
	u_int32_t snd_ssthresh_prev;                                      
};

  
                                                                      
                                                                     
                                                                
                                                                       
                                                            
                                                                       
                                                                       
                                                       
   
#define TCP_RTT_SCALE           32                                             
#define TCP_RTT_SHIFT           5                                         
#define TCP_RTTVAR_SCALE        16                                         
#define TCP_RTTVAR_SHIFT        4                                     
#define TCP_DELTA_SHIFT         2                            


  
                                                                                           
                                       
   
struct  xtcpprogress_indicators {
	u_int32_t       xp_numflows;                                       
	u_int32_t       xp_conn_probe_fails;                                      
	u_int32_t       xp_read_probe_fails;                                      
	u_int32_t       xp_write_probe_fails;                                
	u_int32_t       xp_recentflows;                                      
	u_int32_t       xp_recentflows_unacked;                                                       
	u_int64_t       xp_recentflows_rxbytes;                                             
	u_int64_t       xp_recentflows_txbytes;                                                
	u_int64_t       xp_recentflows_rxooo;                                                            
	u_int64_t       xp_recentflows_rxdup;                                                         
	u_int64_t       xp_recentflows_retx;                                                     
	u_int64_t       xp_reserved1;                          
	u_int64_t       xp_reserved2;                          
	u_int64_t       xp_reserved3;                          
	u_int64_t       xp_reserved4;                          
};

struct tcpprogressreq {
	u_int64_t       ifindex;                                                             
	u_int64_t       recentflow_maxduration;                                                                             
	u_int64_t       filter_flags;                                                                                            
	u_int64_t       xp_reserved2;                          
};

#endif              

#pragma pack()

  
                               
   
#define TCPCTL_DO_RFC1323       1                                    
#define TCPCTL_DO_RFC1644       2                                    
#define TCPCTL_MSSDFLT          3                        
#define TCPCTL_STATS            4                                   
#define TCPCTL_RTTDFLT          5                                 
#define TCPCTL_KEEPIDLE         6                                 
#define TCPCTL_KEEPINTVL        7                                        
#define TCPCTL_SENDSPACE        8                              
#define TCPCTL_RECVSPACE        9                                 
#define TCPCTL_KEEPINIT         10                                        
#define TCPCTL_PCBLIST          11                                        
#define TCPCTL_DELACKTIME       12                                           
#define TCPCTL_V6MSSDFLT        13                                
#define TCPCTL_MAXID            14

#ifdef BSD_KERNEL_PRIVATE
#include <sys/bitstring.h>

#define TCP_PKTLIST_CLEAR(tp) {                                         \
	(tp)->t_pktlist_head = (tp)->t_pktlist_tail = NULL;             \
	(tp)->t_lastchain = (tp)->t_pktlist_sentlen = 0;                \
}

extern int tcp_TCPTV_MIN;

#ifdef SYSCTL_DECL
SYSCTL_DECL(_net_inet_tcp);
#endif                  

extern  struct inpcbhead tcb;                                                
extern  struct inpcbinfo tcbinfo;
extern  struct tcpstat tcpstat;                     
extern  int tcp_mssdflt;                 
extern  int tcp_minmss;
#define TCP_FASTOPEN_SERVER 0x01
#define TCP_FASTOPEN_CLIENT 0x02

extern int tcp_tfo_halfcnt;
extern int tcp_tfo_backlog;
extern int tcp_fastopen;
extern int ss_fltsz_local;
extern int target_qdelay;
extern u_int32_t tcp_now;                                            
extern struct timeval tcp_uptime;
extern lck_spin_t *tcp_uptime_lock;
extern int tcp_delack_enabled;
extern int maxseg_unacked;
extern int tcp_use_newreno;
extern struct zone *tcp_reass_zone;
extern struct zone *tcp_rxt_seg_zone;
extern int tcp_ecn_outbound;
extern int tcp_ecn_inbound;
extern uint32_t tcp_do_autorcvbuf;
extern uint32_t tcp_autorcvbuf_max;
extern int tcp_recv_bg;
extern int tcp_do_ack_compression;
extern int tcp_randomize_timestamps;
  
                                                                                    
              
   
#define TCP_ACK_COMPRESSION_DUMMY 1

extern int tcp_do_better_lr;
extern int tcp_cubic_minor_fixes;
extern int tcp_cubic_rfc_compliant;
extern int tcp_flow_control_response;

struct protosw;
struct domain;

struct tcp_respond_args {
	unsigned int ifscope;
	unsigned int nocell:1,
	    noexpensive:1,
	    awdl_unrestricted:1,
	    intcoproc_allowed:1,
	    keep_alive:1,
	    noconstrained:1;
};

void     tcp_canceltimers(struct tcpcb *);
struct tcpcb *
tcp_close(struct tcpcb *);
void     tcp_ctlinput(int, struct sockaddr *, void *, struct ifnet *);
int      tcp_ctloutput(struct socket *, struct sockopt *);
struct tcpcb *
tcp_drop(struct tcpcb *, int);
void     tcp_drain(void);
void     tcp_getrt_rtt(struct tcpcb *tp, struct rtentry *rt);
void     tcp_init(struct protosw *, struct domain *);
void     tcp_input(struct mbuf *, int);
void     tcp_mss(struct tcpcb *, int, unsigned int);
int      tcp_mssopt(struct tcpcb *);
void     tcp_drop_syn_sent(struct inpcb *, int);
void     tcp_mtudisc(struct inpcb *, int);
struct tcpcb *
tcp_newtcpcb(struct inpcb *);
int      tcp_output(struct tcpcb *);
void     tcp_respond(struct tcpcb *, void *, struct tcphdr *, struct mbuf *,
    tcp_seq, tcp_seq, uint8_t, struct tcp_respond_args *);
struct rtentry *
tcp_rtlookup(struct inpcb *, unsigned int);
void     tcp_setpersist(struct tcpcb *);
void     tcp_gc(struct inpcbinfo *);
void     tcp_itimer(struct inpcbinfo *ipi);
void     tcp_check_timer_state(struct tcpcb *tp);
void     tcp_run_timerlist(void *arg1, void *arg2);
void     tcp_sched_timers(struct tcpcb *tp);

struct tcptemp *tcp_maketemplate(struct tcpcb *);
void     tcp_fillheaders(struct tcpcb *, void *, void *);
struct tcpcb *tcp_timers(struct tcpcb *, int);
void     tcp_trace(int, int, struct tcpcb *, void *, struct tcphdr *, int);

void tcp_fill_info(struct tcpcb *, struct tcp_info *);
void tcp_sack_doack(struct tcpcb *, struct tcpopt *, struct tcphdr *,
    u_int32_t *, uint32_t *);
extern boolean_t tcp_sack_process_dsack(struct tcpcb *, struct tcpopt *,
    struct tcphdr *);
int tcp_detect_bad_rexmt(struct tcpcb *, struct tcphdr *, struct tcpopt *,
    u_int32_t rxtime);
void     tcp_update_sack_list(struct tcpcb *tp, tcp_seq rcv_laststart, tcp_seq rcv_lastend);
void     tcp_clean_sackreport(struct tcpcb *tp);
void     tcp_sack_adjust(struct tcpcb *tp);
struct sackhole *tcp_sack_output(struct tcpcb *tp, int *sack_bytes_rexmt);
void     tcp_sack_partialack(struct tcpcb *, struct tcphdr *);
void     tcp_free_sackholes(struct tcpcb *tp);
void     tcp_sack_lost_rexmit(struct tcpcb *tp);
int32_t  tcp_sbspace(struct tcpcb *tp);
void     tcp_set_tso(struct tcpcb *tp, struct ifnet *ifp);
void     tcp_set_ecn(struct tcpcb *tp, struct ifnet *ifp);
int      tcp_flight_size(struct tcpcb *tp);
void     tcp_reset_stretch_ack(struct tcpcb *tp);
extern void tcp_get_ports_used(u_int32_t, int, u_int32_t, bitstr_t *);
uint32_t tcp_count_opportunistic(unsigned int ifindex, u_int32_t flags);
uint32_t tcp_find_anypcb_byaddr(struct ifaddr *ifa);
void tcp_set_max_rwinscale(struct tcpcb *tp, struct socket *so);
struct bwmeas* tcp_bwmeas_alloc(struct tcpcb *tp);
void tcp_bwmeas_free(struct tcpcb *tp);
extern int32_t timer_diff(uint32_t t1, uint32_t toff1, uint32_t t2, uint32_t toff2);

extern void tcp_set_background_cc(struct socket *);
extern void tcp_set_foreground_cc(struct socket *);
extern void tcp_set_recv_bg(struct socket *);
extern void tcp_clear_recv_bg(struct socket *);
extern boolean_t tcp_sack_byte_islost(struct tcpcb *tp);
#define IS_TCP_RECV_BG(_so)     \
	((_so)->so_flags1 & SOF1_TRAFFIC_MGT_TCP_RECVBG)

#if TRAFFIC_MGT
#define CLEAR_IAJ_STATE(_tp_) (_tp_)->iaj_rcv_ts = 0
void     reset_acc_iaj(struct tcpcb *tp);
#endif                  

int      tcp_lock(struct socket *, int, void *);
int      tcp_unlock(struct socket *, int, void *);
void     calculate_tcp_clock(void);

extern void tcp_keepalive_reset(struct tcpcb *);
extern uint32_t get_base_rtt(struct tcpcb *tp);

#ifdef _KERN_LOCKS_H_
lck_mtx_t *      tcp_getlock(struct socket *, int);
#else
void *   tcp_getlock(struct socket *, int);
#endif

extern  struct pr_usrreqs tcp_usrreqs;
extern  u_int32_t tcp_sendspace;
extern  u_int32_t tcp_recvspace;
tcp_seq tcp_new_isn(struct tcpcb *);

extern int tcp_input_checksum(int, struct mbuf *, struct tcphdr *, int, int);
extern void tcp_getconninfo(struct socket *, struct conninfo_tcp *);
extern void add_to_time_wait(struct tcpcb *, uint32_t delay);
extern void tcp_pmtud_revert_segment_size(struct tcpcb *tp);
extern void tcp_rxtseg_insert(struct tcpcb *, tcp_seq, tcp_seq);
extern struct tcp_rxt_seg *tcp_rxtseg_find(struct tcpcb *, tcp_seq, tcp_seq);
extern void tcp_rxtseg_set_spurious(struct tcpcb *tp, tcp_seq start, tcp_seq end);
extern void tcp_rxtseg_clean(struct tcpcb *);
extern boolean_t tcp_rxtseg_detect_bad_rexmt(struct tcpcb *, tcp_seq);
extern boolean_t tcp_rxtseg_dsack_for_tlp(struct tcpcb *);
extern u_int32_t tcp_rxtseg_total_size(struct tcpcb *tp);
extern void tcp_rexmt_save_state(struct tcpcb *tp);
extern void tcp_interface_send_probe(u_int16_t if_index_available);
extern void tcp_probe_connectivity(struct ifnet *ifp, u_int32_t enable);
extern void tcp_get_connectivity_status(struct tcpcb *,
    struct tcp_conn_status *);

extern void tcp_clear_keep_alive_offload(struct socket *so);
extern void tcp_fill_keepalive_offload_frames(struct ifnet *,
    struct ifnet_keepalive_offload_frame *, u_int32_t, size_t, u_int32_t *);
extern int tcp_notify_kao_timeout(ifnet_t ifp,
    struct ifnet_keepalive_offload_frame *frame);

extern boolean_t tfo_enabled(const struct tcpcb *tp);
extern void tcp_disable_tfo(struct tcpcb *tp);
extern void tcp_tfo_gen_cookie(struct inpcb *inp, u_char *out, size_t blk_size);
#define TCP_FASTOPEN_KEYLEN 16
extern int tcp_freeq(struct tcpcb *tp);
extern errno_t tcp_notify_ack_id_valid(struct tcpcb *, struct socket *, u_int32_t);
extern errno_t tcp_add_notify_ack_marker(struct tcpcb *, u_int32_t);
extern void tcp_notify_ack_free(struct tcpcb *);
extern void tcp_notify_acknowledgement(struct tcpcb *, struct socket *);
extern void tcp_get_notify_ack_count(struct tcpcb *,
    struct tcp_notify_ack_complete *);
extern void tcp_get_notify_ack_ids(struct tcpcb *tp,
    struct tcp_notify_ack_complete *);
extern void tcp_update_mss_locked(struct socket *, struct ifnet *);

extern int get_tcp_inp_list(struct inpcb **, int, inp_gen_t);
extern bool tcp_notify_ack_active(struct socket *so);

#if MPTCP
extern int mptcp_input_preproc(struct tcpcb *tp, struct mbuf *m,
    struct tcphdr *th, int drop_hdrlen);
extern uint16_t mptcp_output_csum(struct mbuf *m, uint64_t dss_val,
    uint32_t sseq, uint16_t dlen);
extern int mptcp_adj_mss(struct tcpcb *, boolean_t);
extern void mptcp_insert_rmap(struct tcpcb *tp, struct mbuf *m, struct tcphdr *th);
#endif

__private_extern__ void tcp_update_stats_per_flow(
	struct ifnet_stats_per_flow *, struct ifnet *);

#define TCP_ACK_STRATEGY_LEGACY 0
#define TCP_ACK_STRATEGY_MODERN 1

extern int tcp_ack_strategy;

#define tcp_add_fsw_flow(...)
#define tcp_del_fsw_flow(...)

#endif                         

#endif                          
