/*
 * Copyright (c) 2006-2008 Apple Computer, Inc.  All Rights Reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
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
 * @APPLE_LICENSE_HEADER_END@
 */

#pragma D depends_on library darwin.d
#pragma D depends_on module mach_kernel
#pragma D depends_on provider tcp

  
            
   
inline int TH_FIN = 0x01;
#pragma D binding "1.0" TH_FIN
inline int TH_SYN = 0x02;
#pragma D binding "1.0" TH_SYN
inline int TH_RST = 0x04;
#pragma D binding "1.0" TH_RST
inline int TH_PUSH = 0x08;
#pragma D binding "1.0" TH_PUSH
inline int TH_ACK = 0x10;
#pragma D binding "1.0" TH_ACK
inline int TH_URG = 0x20;
#pragma D binding "1.0" TH_URG
inline int TH_ECE = 0x40;
#pragma D binding "1.0" TH_ECE
inline int TH_CWR = 0x80;
#pragma D binding "1.0" TH_CWR

  
             
   
inline int TCPS_CLOSED = 0;
#pragma D binding "1.0" TCPS_CLOSED
inline int TCPS_LISTEN = 1;
#pragma D binding "1.0" TCPS_LISTEN
inline int TCPS_SYN_SENT = 2;
#pragma D binding "1.0" TCPS_SYN_SENT
inline int TCPS_SYN_RECEIVED = 3;
#pragma D binding "1.0" TCPS_SYN_RECEIVED
inline int TCPS_ESTABLISHED = 4;
#pragma D binding "1.0" TCPS_ESTABLISHED
inline int TCPS_CLOSE_WAIT = 5;
#pragma D binding "1.0" TCPS_CLOSE_WAIT
inline int TCPS_FIN_WAIT_1 = 6;
#pragma D binding "1.0" TCPS_FIN_WAIT_1
inline int TCPS_CLOSING = 7;
#pragma D binding "1.0" TCPS_CLOSING
inline int TCPS_LAST_ACK = 8;
#pragma D binding "1.0" TCPS_LAST_ACK
inline int TCPS_FIN_WAIT_2 = 9;
#pragma D binding "1.0" TCPS_FIN_WAIT_2
inline int TCPS_TIME_WAIT = 10;
#pragma D binding "1.0" TCPS_TIME_WAIT

  
                                
   
inline int TCP_CC_CWND_INIT = 0;
#pragma D binding "1.0" TCP_CC_CWND_INIT
inline int TCP_CC_INSEQ_ACK_RCVD = 1;
#pragma D binding "1.0" TCP_CC_INSEQ_ACK_RCVD
inline int TCP_CC_ACK_RCVD = 2;
#pragma D binding "1.0" TCP_CC_ACK_RCVD
inline int TCP_CC_ENTER_FASTRECOVERY = 3;
#pragma D binding "1.0" TCP_CC_ENTER_FASTRECOVERY
inline int TCP_CC_IN_FASTRECOVERY = 4;
#pragma D binding "1.0" TCP_CC_IN_FASTRECOVERY
inline int TCP_CC_EXIT_FASTRECOVERY = 5;
#pragma D binding "1.0" TCP_CC_EXIT_FASTRECOVERY
inline int TCP_CC_PARTIAL_ACK = 6;
#pragma D binding "1.0" TCP_CC_PARTIAL_ACK
inline int TCP_CC_IDLE_TIMEOUT = 7;
#pragma D binding "1.0" TCP_CC_IDLE_TIMEOUT
inline int TCP_CC_REXMT_TIMEOUT = 8;
#pragma D binding "1.0" TCP_CC_REXMT_TIMEOUT
inline int TCP_CC_ECN_RCVD = 9;
#pragma D binding "1.0" TCP_CC_ECN_RCVD
inline int TCP_CC_BAD_REXMT_RECOVERY = 10;
#pragma D binding "1.0" TCP_CC_BAD_REXMT_RECOVERY
inline int TCP_CC_OUTPUT_ERROR = 11;
#pragma D binding "1.0" TCP_CC_OUTPUT_ERROR
inline int TCP_CC_CHANGE_ALGO = 12;
#pragma D binding "1.0" TCP_CC_CHANGE_ALGO
inline int TCP_CC_FLOW_CONTROL = 13;
#pragma D binding "1.0" TCP_CC_FLOW_CONTROL
inline int TCP_CC_SUSPEND = 14;
#pragma D binding "1.0" TCP_CC_SUSPEND
inline int TCP_CC_LIMITED_TRANSMIT = 15;
#pragma D binding "1.0" TCP_CC_LIMITED_TRANSMIT
inline int TCP_CC_EARLY_RETRANSMIT = 16;
#pragma D binding "1.0" TCP_CC_EARLY_RETRANSMIT


  
                                  
   
typedef struct tcpinfo {
	uint16_t tcp_sport;	                 
	uint16_t tcp_dport;	                      
	uint32_t tcp_seq;	                     
	uint32_t tcp_ack;	                            
	uint8_t tcp_offset;	                           
	uint8_t tcp_flags;	           
	uint16_t tcp_window;	                 
	uint16_t tcp_checksum;	              
	uint16_t tcp_urgent;	                         
	struct tcphdr *tcp_hdr;	                    
} tcpinfo_t;

#pragma D binding "1.0" translator
translator tcpinfo_t < struct tcphdr *T > {
	tcp_sport = ntohs(T->th_sport);
	tcp_dport = ntohs(T->th_dport);
	tcp_seq = ntohl(T->th_seq);
	tcp_ack = ntohl(T->th_ack);
	tcp_offset = T->th_off << 2;
	tcp_flags = T->th_flags;
	tcp_window = ntohs(T->th_win);
	tcp_checksum = ntohs(T->th_sum);
	tcp_urgent = ntohs(T->th_urp);
	tcp_hdr = T;
};

  
                                                              
   
typedef struct tcpsinfo {
	int tcps_local;		                                   
	int tcps_active;	                          
	string tcps_state;	                            
	u_int t_flags;		           
	uint32_t rcv_nxt;	                  
	uint32_t rcv_adv;	                       
	uint32_t rcv_wnd;	                    
	uint32_t snd_wnd;	                 
	uint32_t snd_cwnd;	                                  
	uint32_t snd_ssthresh;	                          
	uint32_t snd_una;	                         
	uint32_t snd_nxt;	               
	uint32_t snd_max;	              
	uint32_t snd_recover;	                              
	int	t_rxtcur;	                              
	u_int	t_maxseg;	                          
	u_int	t_rttbest;	                           
	int	rcv_numsacks;	                                   
	int	snd_numholes;	                                    
	struct tcpcb* tcpcb;	                                  
} tcpsinfo_t;

#pragma D binding "1.0" translator
translator tcpsinfo_t < struct tcpcb *T> {
	tcps_local = 0;		              
	tcps_active = 0;
	tcps_state = T ? 
		T->t_state == TCPS_CLOSED ? "state-closed" :
		T->t_state == TCPS_LISTEN ? "state-listen" :
		T->t_state == TCPS_SYN_SENT ? "state-syn-sent" :
		T->t_state == TCPS_SYN_RECEIVED ? "state-syn-received" :
		T->t_state == TCPS_ESTABLISHED ? "state-established" :
		T->t_state == TCPS_CLOSE_WAIT ? "state-close-wait" :
		T->t_state == TCPS_FIN_WAIT_1 ? "state-fin-wait1" :
		T->t_state == TCPS_CLOSING ? "state-closing" :
		T->t_state == TCPS_LAST_ACK ? "state-last-ack" :
		T->t_state == TCPS_FIN_WAIT_2 ? "state-fin-wait2" :
		T->t_state == TCPS_TIME_WAIT ? "state-time-wait" :
		"<unknown>" : "<null>";
	t_flags = T->t_flags;
	rcv_nxt = T->rcv_nxt;
	rcv_adv = T->rcv_adv;
	rcv_wnd = T->rcv_wnd;
	snd_wnd = T->snd_wnd;
	snd_cwnd = T->snd_cwnd;
	snd_ssthresh = T->snd_ssthresh;
	snd_una = T->snd_una;
	snd_nxt = T->snd_nxt;
	snd_max = T->snd_max;
	snd_recover = T->snd_recover;
	t_rxtcur = T->t_rxtcur;
	t_maxseg = T->t_maxseg;
	t_rttbest = T->t_rttbest;
	rcv_numsacks = T->rcv_numsacks;
	snd_numholes = T->snd_numholes;
	tcpcb = T;
};

  
                                                          
   
typedef struct tcpnsinfo {
	string tcps_state;	                            
} tcpnsinfo_t;

#pragma D binding "1.0" translator
translator tcpnsinfo_t < int32_t I > {
	tcps_state = I ? 
		I == TCPS_LISTEN ? "state-listen" :
		I == TCPS_SYN_SENT ? "state-syn-sent" :
		I == TCPS_SYN_RECEIVED ? "state-syn-received" :
		I == TCPS_ESTABLISHED ? "state-established" :
		I == TCPS_CLOSE_WAIT ? "state-close-wait" :
		I == TCPS_FIN_WAIT_1 ? "state-fin-wait1" :
		I == TCPS_CLOSING ? "state-closing" :
		I == TCPS_LAST_ACK ? "state-last-ack" :
		I == TCPS_FIN_WAIT_2 ? "state-fin-wait2" :
		I == TCPS_TIME_WAIT ? "state-time-wait" :
		"<unknown>" : "state-closed";
};

   
                                                                     
   
typedef struct tcpccevent {
	string tcp_cc;		                                               
} tcpccevent_t;

#pragma D binding "1.0" translator
translator tcpccevent_t < int32_t I > {
	tcp_cc = I ?
		I == TCP_CC_INSEQ_ACK_RCVD ? "inseq-ack-rcvd" :
		I == TCP_CC_ACK_RCVD ? "ack-rcvd" :
		I == TCP_CC_ENTER_FASTRECOVERY ? "enter-fastrecovery" :
		I == TCP_CC_EXIT_FASTRECOVERY ? "exit-fastrecovery" :
		I == TCP_CC_PARTIAL_ACK ? "partial-ack" :
		I == TCP_CC_IDLE_TIMEOUT ? "idle-timeout" :
		I == TCP_CC_REXMT_TIMEOUT ? "rexmt-timeout" :
		I == TCP_CC_ECN_RCVD ? "ecn-rcvd" :
		I == TCP_CC_BAD_REXMT_RECOVERY ? "bad-rexmt" :
		I == TCP_CC_OUTPUT_ERROR ? "output-error" :
		I == TCP_CC_CHANGE_ALGO ? "change-algo" :
		I == TCP_CC_FLOW_CONTROL ? "flow-control" :
		I == TCP_CC_SUSPEND ? "suspend" :
		I == TCP_CC_LIMITED_TRANSMIT ? "limited-transmit" :
		I == TCP_CC_EARLY_RETRANSMIT ? "early-rexmt" :
		"<unknown>" : "cwnd-init";
};
