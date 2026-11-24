/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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
 * Copyright (c) 1982, 1986, 1993
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
 *	@(#)tcp.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD: src/sys/netinet/tcp.h,v 1.13.2.3 2001/03/01 22:08:42 jlemon Exp $
 */

#ifndef _NETINET_TCP_H_
#define _NETINET_TCP_H_
#ifndef DRIVERKIT
#include <sys/appleapiopts.h>
#endif                

#include <machine/endian.h>
#include <machine/types.h>                 

#ifndef DRIVERKIT
#include <sys/types.h>
#else
#include <sys/_types.h>
#endif                

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
typedef __uint32_t tcp_seq;
typedef __uint32_t tcp_cc;                                                

#define tcp6_seq        tcp_seq                                    
#define tcp6hdr         tcphdr                                     

  
              
                                
   
struct tcphdr {
	unsigned short  th_sport;                        
	unsigned short  th_dport;                             
	tcp_seq th_seq;                                      
	tcp_seq th_ack;                                             
#if __DARWIN_BYTE_ORDER == __DARWIN_LITTLE_ENDIAN
	unsigned int    th_x2:4,                      
	    th_off:4;                                    
#endif
#if __DARWIN_BYTE_ORDER == __DARWIN_BIG_ENDIAN
	unsigned int    th_off:4,                        
	    th_x2:4;                                  
#endif
	unsigned char   th_flags;
#define TH_FIN  0x01
#define TH_SYN  0x02
#define TH_RST  0x04
#define TH_PUSH 0x08
#define TH_ACK  0x10
#define TH_URG  0x20
#define TH_ECE  0x40
#define TH_CWR  0x80
#define TH_FLAGS        (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
#define TH_ACCEPT       (TH_FIN|TH_SYN|TH_RST|TH_ACK)

	unsigned short  th_win;                     
	unsigned short  th_sum;                       
	unsigned short  th_urp;                             
};

#define TCPOPT_EOL              0
#define TCPOPT_NOP              1
#define TCPOPT_MAXSEG           2
#define TCPOLEN_MAXSEG          4
#define TCPOPT_WINDOW           3
#define TCPOLEN_WINDOW          3
#define TCPOPT_SACK_PERMITTED   4                                 
#define TCPOLEN_SACK_PERMITTED  2
#define TCPOPT_SACK             5                                 
#define TCPOLEN_SACK            8                                      
#define TCPOPT_TIMESTAMP        8
#define TCPOLEN_TIMESTAMP       10
#define TCPOLEN_TSTAMP_APPA             (TCPOLEN_TIMESTAMP+2)                 
#define TCPOPT_TSTAMP_HDR               \
    (TCPOPT_NOP<<24|TCPOPT_NOP<<16|TCPOPT_TIMESTAMP<<8|TCPOLEN_TIMESTAMP)

#define MAX_TCPOPTLEN           40                                            

#define TCPOPT_CC               11                                        
#define TCPOPT_CCNEW            12
#define TCPOPT_CCECHO           13
#define    TCPOLEN_CC                   6
#define    TCPOLEN_CC_APPA              (TCPOLEN_CC+2)
#define    TCPOPT_CC_HDR(ccopt)         \
    (TCPOPT_NOP<<24|TCPOPT_NOP<<16|(ccopt)<<8|TCPOLEN_CC)
#define TCPOPT_SIGNATURE                19                               
#define    TCPOLEN_SIGNATURE            18
#if MPTCP
#define TCPOPT_MULTIPATH                30
#endif

#define TCPOPT_FASTOPEN                 34
#define TCPOLEN_FASTOPEN_REQ            2

                        
#define TCPOPT_SACK_PERMIT_HDR  \
(TCPOPT_NOP<<24|TCPOPT_NOP<<16|TCPOPT_SACK_PERMITTED<<8|TCPOLEN_SACK_PERMITTED)
#define TCPOPT_SACK_HDR         (TCPOPT_NOP<<24|TCPOPT_NOP<<16|TCPOPT_SACK<<8)
                             
#define MAX_SACK_BLKS   6                                                    

  
                                                                        
                                                                 
                       
   

#define TCP_MAX_SACK    4                                            


  
                                        
                                      
                                       
                                                                         
   
#define TCP_MSS 512

  
                                                                 
                                                        
                                                                     
                               
                                                                       
                                                                  
                                                 
   
#define TCP_MINMSS 216

  
                                         
                                         
                                                            
                                                                           
   
#define TCP6_MSS        1024

#define TCP_MAXWIN      65535                                            
#define TTCP_CLIENT_SND_WND     4096                                           

#define TCP_MAX_WINSHIFT        14                                

#define TCP_MAXHLEN     (0xf<<2)                                           
#define TCP_MAXOLEN     (TCP_MAXHLEN - sizeof(struct tcphdr))
                                
#endif                                             

  
                                                
   
#define TCP_NODELAY             0x01                                              
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define TCP_MAXSEG              0x02                                  
#define TCP_NOPUSH              0x04                                        
#define TCP_NOOPT               0x08                               
#define TCP_KEEPALIVE           0x10                                                     
#define TCP_CONNECTIONTIMEOUT   0x20                            
#define PERSIST_TIMEOUT         0x40                                       
                                                                    
                                                                         
                                    
#define TCP_RXT_CONNDROPTIME    0x80                                                   
                                                                              
                                    
#define TCP_RXT_FINDROP         0x100                                                
                                                                                 
                                                                            
                                                  
                                    
#define TCP_KEEPINTVL           0x101                                    
#define TCP_KEEPCNT             0x102                                          
#define TCP_SENDMOREACKS        0x103                                      
#define TCP_ENABLE_ECN          0x104                                   
#define TCP_FASTOPEN            0x105                                                   
#define TCP_CONNECTION_INFO     0x106                                

#ifdef PRIVATE
#define TCP_INFO                        0x200                                    
#define TCP_MEASURE_SND_BW              0x202                                                       
#endif              


#define TCP_NOTSENT_LOWAT       0x201                                           

#ifdef PRIVATE
#define TCP_MEASURE_BW_BURST    0x203                                                    
#define TCP_PEER_PID            0x204                                                     
#define TCP_ADAPTIVE_READ_TIMEOUT       0x205                                               
#define TCP_OPTION_UNUSED_0             0x206               
#define TCP_ADAPTIVE_WRITE_TIMEOUT      0x207                                                
#define TCP_NOTIMEWAIT                  0x208                                   
#define TCP_DISABLE_BLACKHOLE_DETECTION 0x209                                         
#define TCP_ECN_MODE                    0x210                                           
#define TCP_KEEPALIVE_OFFLOAD           0x211                                                  

  
                      
   
#define ECN_MODE_DEFAULT                0x0                                               
#define ECN_MODE_ENABLE                 0x1                                         
#define ECN_MODE_DISABLE                0x2                                          

  
                             
  
                                                                    
                                                                          
                                                                            
                                                                    
                                                                       
                                                                         
  
                                                                           
                                                                        
                                                                       
                                                                     
   

#define TCP_MAX_NOTIFY_ACK      10

typedef u_int32_t       tcp_notify_ack_id_t;

struct tcp_notify_ack_complete {
	u_int32_t       notify_pending;                    
	u_int32_t       notify_complete_count;
	tcp_notify_ack_id_t notify_complete_id[TCP_MAX_NOTIFY_ACK];
};

#define TCP_NOTIFY_ACKNOWLEDGEMENT      0x212                                         
#define MPTCP_SERVICE_TYPE              0x213                           
#define TCP_FASTOPEN_FORCE_HEURISTICS   0x214                                                    

#define MPTCP_SVCTYPE_HANDOVER          0                
#define MPTCP_SVCTYPE_INTERACTIVE       1
#define MPTCP_SVCTYPE_AGGREGATE         2
#define MPTCP_SVCTYPE_TARGET_BASED      3
#define MPTCP_SVCTYPE_PURE_HANDOVER     4
#define MPTCP_SVCTYPE_MAX               5

  
                                                              
                                                                        
       
   
#define TCP_RXT_MINIMUM_TIMEOUT         0x215

#define TCP_RXT_MINIMUM_TIMEOUT_LIMIT   (5 * 60)                         

#define MPTCP_ALTERNATE_PORT            0x216
#define MPTCP_FORCE_ENABLE              0x217
#define TCP_FASTOPEN_FORCE_ENABLE       0x218
#define MPTCP_EXPECTED_PROGRESS_TARGET  0x219

  
                                                                       
   
#pragma pack(4)

#define TCPI_OPT_TIMESTAMPS     0x01
#define TCPI_OPT_SACK           0x02
#define TCPI_OPT_WSCALE         0x04
#define TCPI_OPT_ECN            0x08

#define TCPI_FLAG_LOSSRECOVERY  0x01                                    
#define TCPI_FLAG_STREAMING_ON  0x02                                

struct tcp_conn_status {
	union {
		struct {
			unsigned int    probe_activated : 1;
			unsigned int    write_probe_failed : 1;
			unsigned int    read_probe_failed : 1;
			unsigned int    conn_probe_failed : 1;
		};
		uint32_t        pad_field;
	};
};

  
                                                                       
                        
   
struct tcp_info {
	u_int8_t        tcpi_state;                                         
	u_int8_t        tcpi_options;                                         
	u_int8_t        tcpi_snd_wscale;                                       
	u_int8_t        tcpi_rcv_wscale;                                       

	u_int32_t       tcpi_flags;                                                      

	u_int32_t       tcpi_rto;                                                                   
	u_int32_t       tcpi_snd_mss;                                           
	u_int32_t       tcpi_rcv_mss;                                              

	u_int32_t       tcpi_rttcur;                                          
	u_int32_t       tcpi_srtt;                                        
	u_int32_t       tcpi_rttvar;                              
	u_int32_t       tcpi_rttbest;                                    

	u_int32_t       tcpi_snd_ssthresh;                                 
	u_int32_t       tcpi_snd_cwnd;                                       

	u_int32_t       tcpi_rcv_space;                                      

	u_int32_t       tcpi_snd_wnd;                                        
	u_int32_t       tcpi_snd_nxt;                                  
	u_int32_t       tcpi_rcv_nxt;                                   

	int32_t         tcpi_last_outif;                                                     
	u_int32_t       tcpi_snd_sbbytes;                                                        

	u_int64_t       tcpi_txpackets __attribute__((aligned(8)));                             
	u_int64_t       tcpi_txbytes __attribute__((aligned(8)));
	                      
	u_int64_t       tcpi_txretransmitbytes __attribute__((aligned(8)));
	                               
	u_int64_t       tcpi_txunacked __attribute__((aligned(8)));
	                                              
	u_int64_t       tcpi_rxpackets __attribute__((aligned(8)));                                 
	u_int64_t       tcpi_rxbytes __attribute__((aligned(8)));
	                          
	u_int64_t       tcpi_rxduplicatebytes __attribute__((aligned(8)));
	                                    
	u_int64_t       tcpi_rxoutoforderbytes __attribute__((aligned(8)));
	                                       
	u_int64_t       tcpi_snd_bw __attribute__((aligned(8)));                                                 
	u_int8_t        tcpi_synrexmits;                                                      
	u_int8_t        tcpi_unused1;
	u_int16_t       tcpi_unused2;
	u_int64_t       tcpi_cell_rxpackets __attribute((aligned(8)));                                      
	u_int64_t       tcpi_cell_rxbytes __attribute((aligned(8)));                                      
	u_int64_t       tcpi_cell_txpackets __attribute((aligned(8)));                                         
	u_int64_t       tcpi_cell_txbytes __attribute((aligned(8)));                                         
	u_int64_t       tcpi_wifi_rxpackets __attribute((aligned(8)));                                   
	u_int64_t       tcpi_wifi_rxbytes __attribute((aligned(8)));                                   
	u_int64_t       tcpi_wifi_txpackets __attribute((aligned(8)));                                      
	u_int64_t       tcpi_wifi_txbytes __attribute((aligned(8)));                                      
	u_int64_t       tcpi_wired_rxpackets __attribute((aligned(8)));                                  
	u_int64_t       tcpi_wired_rxbytes __attribute((aligned(8)));                                  
	u_int64_t       tcpi_wired_txpackets __attribute((aligned(8)));                                     
	u_int64_t       tcpi_wired_txbytes __attribute((aligned(8)));                                     
	struct tcp_conn_status  tcpi_connstatus;                                  

	u_int16_t
	    tcpi_tfo_cookie_req:1,                            
	    tcpi_tfo_cookie_rcv:1,                           
	    tcpi_tfo_syn_loss:1,                                                
	    tcpi_tfo_syn_data_sent:1,                                     
	    tcpi_tfo_syn_data_acked:1,                                               
	    tcpi_tfo_syn_data_rcv:1,                                                       
	    tcpi_tfo_cookie_req_rcv:1,                                         
	    tcpi_tfo_cookie_sent:1,                                  
	    tcpi_tfo_cookie_invalid:1,                                            
	    tcpi_tfo_cookie_wrong:1,                                    
	    tcpi_tfo_no_cookie_rcv:1,                                                       
	    tcpi_tfo_heuristics_disable:1,                                     
	    tcpi_tfo_send_blackhole:1,                                           
	    tcpi_tfo_recv_blackhole:1,                                            
	    tcpi_tfo_onebyte_proxy:1;                                                           

	u_int16_t       tcpi_ecn_client_setup:1,                                                  
	    tcpi_ecn_server_setup:1,                                                              
	    tcpi_ecn_success:1,                                                  
	    tcpi_ecn_lost_syn:1,                                                     
	    tcpi_ecn_lost_synack:1,                                                      
	    tcpi_local_peer:1,                                                               
	    tcpi_if_cell:1,                                                
	    tcpi_if_wifi:1,                                            
	    tcpi_if_wired:1,                                                                           
	    tcpi_if_wifi_infra:1,                                                     
	    tcpi_if_wifi_awdl:1,                                            
	    tcpi_snd_background:1,                                                              
	    tcpi_rcv_background:1;                                                               

	u_int32_t       tcpi_ecn_recv_ce;                                     
	u_int32_t       tcpi_ecn_recv_cwr;                                     

	u_int32_t       tcpi_rcvoopack;                                            
	u_int32_t       tcpi_pawsdrop;                                            
	u_int32_t       tcpi_sack_recovery_episode;                             
	u_int32_t       tcpi_reordered_pkts;                          
	u_int32_t       tcpi_dsack_sent;                                     
	u_int32_t       tcpi_dsack_recvd;                                          
	u_int32_t       tcpi_flowhash;                                            

	u_int64_t       tcpi_txretransmitpackets __attribute__((aligned(8)));
};

struct tcp_measure_bw_burst {
	u_int32_t       min_burst_size;                                       
	u_int32_t       max_burst_size;                                       
};

  
                                                                           
   

struct info_tuple {
	u_int8_t        itpl_proto;
	union {
		struct sockaddr         _itpl_sa;
		struct sockaddr_in      _itpl_sin;
		struct sockaddr_in6     _itpl_sin6;
	} itpl_localaddr;
	union {
		struct sockaddr         _itpl_sa;
		struct sockaddr_in      _itpl_sin;
		struct sockaddr_in6     _itpl_sin6;
	} itpl_remoteaddr;
};

#define itpl_local_sa           itpl_localaddr._itpl_sa
#define itpl_local_sin          itpl_localaddr._itpl_sin
#define itpl_local_sin6         itpl_localaddr._itpl_sin6
#define itpl_remote_sa          itpl_remoteaddr._itpl_sa
#define itpl_remote_sin         itpl_remoteaddr._itpl_sin
#define itpl_remote_sin6        itpl_remoteaddr._itpl_sin6

  
                                                 
  
                                                                     
                                                                      
   
typedef struct conninfo_tcp {
	pid_t                   tcpci_peer_pid;                               
	struct tcp_info         tcpci_tcp_info;               
} conninfo_tcp_t;

#pragma pack()

struct mptcp_itf_stats {
	u_short  ifindex;
	uint16_t switches;
	uint32_t is_expensive:1;
	uint64_t mpis_txbytes __attribute__((aligned(8)));
	uint64_t mpis_rxbytes __attribute__((aligned(8)));
	uint64_t mpis_wifi_txbytes __attribute__((aligned(8)));
	uint64_t mpis_wifi_rxbytes __attribute__((aligned(8)));
	uint64_t mpis_wired_txbytes __attribute__((aligned(8)));
	uint64_t mpis_wired_rxbytes __attribute__((aligned(8)));
	uint64_t mpis_cell_txbytes __attribute__((aligned(8)));
	uint64_t mpis_cell_rxbytes __attribute__((aligned(8)));
};

                                                   
#define CONNINFO_MPTCP_VERSION  3
typedef struct conninfo_multipathtcp {
	uint32_t        mptcpci_subflow_count;
	uint32_t        mptcpci_switch_count;
	sae_connid_t    mptcpci_subflow_connids[4];

	uint64_t        mptcpci_init_rxbytes;
	uint64_t        mptcpci_init_txbytes;

#define MPTCP_ITFSTATS_SIZE     4
	struct mptcp_itf_stats mptcpci_itfstats[MPTCP_ITFSTATS_SIZE];

	uint32_t        mptcpci_flags;
#define MPTCPCI_FIRSTPARTY      0x01
} conninfo_multipathtcp_t;

#endif              

struct tcp_connection_info {
	u_int8_t        tcpi_state;                           
	u_int8_t        tcpi_snd_wscale;                                   
	u_int8_t        tcpi_rcv_wscale;                                      
	u_int8_t        __pad1;
	u_int32_t       tcpi_options;                              
#define TCPCI_OPT_TIMESTAMPS    0x00000001                         
#define TCPCI_OPT_SACK          0x00000002                   
#define TCPCI_OPT_WSCALE        0x00000004                             
#define TCPCI_OPT_ECN           0x00000008                  
	u_int32_t       tcpi_flags;                
#define TCPCI_FLAG_LOSSRECOVERY 0x00000001
#define TCPCI_FLAG_REORDERING_DETECTED  0x00000002
	u_int32_t       tcpi_rto;                                     
	u_int32_t       tcpi_maxseg;                                        
	u_int32_t       tcpi_snd_ssthresh;                                    
	u_int32_t       tcpi_snd_cwnd;                                       
	u_int32_t       tcpi_snd_wnd;                             
	u_int32_t       tcpi_snd_sbbytes;                                                            
	u_int32_t       tcpi_rcv_wnd;                               
	u_int32_t       tcpi_rttcur;                               
	u_int32_t       tcpi_srtt;                             
	u_int32_t       tcpi_rttvar;                      
	u_int32_t
	    tcpi_tfo_cookie_req:1,                                    
	    tcpi_tfo_cookie_rcv:1,                                   
	    tcpi_tfo_syn_loss:1,                                                        
	    tcpi_tfo_syn_data_sent:1,                                             
	    tcpi_tfo_syn_data_acked:1,                                                       
	    tcpi_tfo_syn_data_rcv:1,                                                               
	    tcpi_tfo_cookie_req_rcv:1,                                                 
	    tcpi_tfo_cookie_sent:1,                                          
	    tcpi_tfo_cookie_invalid:1,                                                    
	    tcpi_tfo_cookie_wrong:1,                                            
	    tcpi_tfo_no_cookie_rcv:1,                                                               
	    tcpi_tfo_heuristics_disable:1,                                             
	    tcpi_tfo_send_blackhole:1,                                                   
	    tcpi_tfo_recv_blackhole:1,                                                    
	    tcpi_tfo_onebyte_proxy:1,                                                                   
	    __pad2:17;
	u_int64_t       tcpi_txpackets __attribute__((aligned(8)));
	u_int64_t       tcpi_txbytes __attribute__((aligned(8)));
	u_int64_t       tcpi_txretransmitbytes __attribute__((aligned(8)));
	u_int64_t       tcpi_rxpackets __attribute__((aligned(8)));
	u_int64_t       tcpi_rxbytes __attribute__((aligned(8)));
	u_int64_t       tcpi_rxoutoforderbytes __attribute__((aligned(8)));
	u_int64_t       tcpi_txretransmitpackets __attribute__((aligned(8)));
};
#endif                                             

#endif
