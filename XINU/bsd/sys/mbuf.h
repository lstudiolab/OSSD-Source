/*
 * Copyright (c) 1999-2020 Apple Inc. All rights reserved.
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
/* Copyright (c) 1998, 1999 Apple Computer, Inc. All Rights Reserved */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * Copyright (c) 1994 NeXT Computer, Inc. All rights reserved.
 *
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
 *	@(#)mbuf.h	8.3 (Berkeley) 1/21/94
 */
  
                                                                      
                                                                          
                                                                        
               
   

#ifndef _SYS_MBUF_H_
#define _SYS_MBUF_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>
#include <sys/_types/_u_int32_t.h>                
#include <sys/_types/_u_int64_t.h>                
#include <sys/_types/_u_short.h>              

#ifdef KERNEL
#include <sys/kpi_mbuf.h>
#endif

#ifdef XNU_KERNEL_PRIVATE
#include <sys/lock.h>
#include <sys/queue.h>
#include <machine/endian.h>
  
                                                             
                                                                      
                                                                        
                                                                        
                                                               
   

  
                                                                          
                                                                            
                                                                       
   
#define _MLEN           (MSIZE - sizeof(struct m_hdr))                       
#define _MHLEN          (_MLEN - sizeof(struct pkthdr))                        

#define NMBPGSHIFT      (PAGE_SHIFT - MSIZESHIFT)
#define NMBPG           (1 << NMBPGSHIFT)                                

#define NCLPGSHIFT      (PAGE_SHIFT - MCLSHIFT)
#define NCLPG           (1 << NCLPGSHIFT)                             

#define NBCLPGSHIFT     (PAGE_SHIFT - MBIGCLSHIFT)
#define NBCLPG          (1 << NBCLPGSHIFT)                                

#define NMBPCLSHIFT     (MCLSHIFT - MSIZESHIFT)
#define NMBPCL          (1 << NMBPCLSHIFT)                             

#define NCLPJCLSHIFT    (M16KCLSHIFT - MCLSHIFT)
#define NCLPJCL         (1 << NCLPJCLSHIFT)                               

#define NCLPBGSHIFT     (MBIGCLSHIFT - MCLSHIFT)
#define NCLPBG          (1 << NCLPBGSHIFT)                              

#define NMBPBGSHIFT     (MBIGCLSHIFT - MSIZESHIFT)
#define NMBPBG          (1 << NMBPBGSHIFT)                                 

  
                             
                                                                   
                                                              
                                                                   
   
#define mtod(m, t)      ((t)m_mtod(m))
#define mtodo(m, o)     ((void *)(mtod(m, uint8_t *) + (o)))
#define dtom(x)         m_dtom(x)

                                       
struct m_hdr {
	struct mbuf     *mh_next;                                 
	struct mbuf     *mh_nextpkt;                                    
	caddr_t         mh_data;                              
	int32_t         mh_len;                                          
	u_int16_t       mh_type;                                       
	u_int16_t       mh_flags;                             
#if __arm__ && (__BIGGEST_ALIGNMENT__ > 4)
                                                                          
                                                                           
                                                                              
                                                                              
                                                                          
                             
   
} __attribute__((aligned(8)));
#else
};
#endif

  
                                                
   
struct m_tag {
	uint64_t               m_tag_cookie;                       
#ifndef __LP64__
	uint32_t               pad;                                         
#endif                
	SLIST_ENTRY(m_tag)      m_tag_link;                              
	uint16_t               m_tag_type;                               
	uint16_t               m_tag_len;                          
	uint32_t               m_tag_id;                      
};

#define M_TAG_ALIGN(len) \
	(P2ROUNDUP(len, sizeof (u_int64_t)) + sizeof (struct m_tag))

#define M_TAG_VALID_PATTERN     0xfeedfacefeedfaceULL
#define M_TAG_FREE_PATTERN      0xdeadbeefdeadbeefULL

  
                                                                  
                                                               
   
struct m_taghdr {
#ifndef __LP64__
	u_int32_t               pad;                                         
#endif                
	u_int64_t               refcnt;                                          
};

  
                                                          
   
struct m_drvaux_tag {
	u_int32_t       da_family;                               
	u_int32_t       da_subfamily;                               
	u_int32_t       da_reserved;                    
	u_int32_t       da_length;                                    
};

                                          
#define PF_TAG_GENERATED                0x1                              
#define PF_TAG_FRAGCACHE                0x2
#define PF_TAG_TRANSLATE_LOCALHOST      0x4
#if PF_ECN
#define PF_TAG_HDR_INET                 0x8                             
#define PF_TAG_HDR_INET6                0x10                            
#endif             
#define PF_TAG_REASSEMBLED              0x20                               
#define PF_TAG_REFRAGMENTED             0x40                                
  
              
   
struct pf_mtag {
	u_int16_t       pftag_flags;                      
	u_int16_t       pftag_rtableid;                                 
	u_int16_t       pftag_tag;
	u_int16_t       pftag_routed;
#if PF_ECN
	void            *pftag_hdr;                                         
#endif             
};

                             
#define PF_TAG_ID_SYSTEM_SERVICE        0xff00
#define PF_TAG_ID_STACK_DROP            0xff01

  
                  
   
struct pf_fragment_tag {
	uint32_t ft_id;                      
	uint16_t ft_hdrlen;                                       
	uint16_t ft_unfragpartlen;                                         
	uint16_t ft_extoff;                                        
	uint16_t ft_maxlen;                                      
};

  
               
   
struct tcp_pktinfo {
	union {
		struct {
			uint32_t segsz;                                       
			uint32_t start_seq;                                  
			pid_t     pid;
			pid_t     e_pid;
		} __tx;
		struct {
			uint8_t  seg_cnt;                                 
		} __rx;
	} __offload;
#define tso_segsz       proto_mtag.__pr_u.tcp.tm_tcp.__offload.__tx.segsz
#define tx_start_seq    proto_mtag.__pr_u.tcp.tm_tcp.__offload.__tx.start_seq
#define tx_tcp_pid      proto_mtag.__pr_u.tcp.tm_tcp.__offload.__tx.pid
#define tx_tcp_e_pid    proto_mtag.__pr_u.tcp.tm_tcp.__offload.__tx.e_pid
#define seg_cnt         proto_mtag.__pr_u.tcp.tm_tcp.__offload.__rx.seg_cnt
};

  
                 
   
struct mptcp_pktinfo {
	uint64_t       mtpi_dsn;                                       
	uint32_t       mtpi_rel_seq;                            
	uint16_t       mtpi_length;                           
	uint16_t       mtpi_csum;
#define mp_dsn          proto_mtag.__pr_u.tcp.tm_mptcp.mtpi_dsn
#define mp_rseq         proto_mtag.__pr_u.tcp.tm_mptcp.mtpi_rel_seq
#define mp_rlen         proto_mtag.__pr_u.tcp.tm_mptcp.mtpi_length
#define mp_csum         proto_mtag.__pr_u.tcp.tm_mptcp.mtpi_csum
};

  
                                                                    
                                                                    
                                                                     
                                                                        
   
struct tcp_mtag {
	union {
		struct tcp_pktinfo      tm_tcp;                            
		struct mptcp_pktinfo    tm_mptcp;                           
	};
};

struct udp_mtag {
	pid_t     _pid;
	pid_t     _e_pid;
#define tx_udp_pid      proto_mtag.__pr_u.udp._pid
#define tx_udp_e_pid    proto_mtag.__pr_u.udp._e_pid
};

struct rawip_mtag {
	pid_t     _pid;
	pid_t     _e_pid;
#define tx_rawip_pid    proto_mtag.__pr_u.rawip._pid
#define tx_rawip_e_pid  proto_mtag.__pr_u.rawip._e_pid
};

struct driver_mtag_ {
	uintptr_t               _drv_tx_compl_arg;
	uintptr_t               _drv_tx_compl_data;
	kern_return_t           _drv_tx_status;
	uint16_t                _drv_flowid;
#define drv_tx_compl_arg        builtin_mtag._drv_mtag._drv_tx_compl_arg
#define drv_tx_compl_data       builtin_mtag._drv_mtag._drv_tx_compl_data
#define drv_tx_status           builtin_mtag._drv_mtag._drv_tx_status
#define drv_flowid              builtin_mtag._drv_mtag._drv_flowid
};

  
                                                                       
  
                                                                      
                                                                      
                                                                   
                                     
   
struct proto_mtag_ {
	union {
		struct tcp_mtag tcp;                              
		struct udp_mtag         udp;                      
		struct rawip_mtag       rawip;                              
	} __pr_u;
};

  
                          
   
struct necp_mtag_ {
	u_int32_t       necp_policy_id;
	u_int32_t       necp_skip_policy_id;
	u_int32_t       necp_route_rule_id;
	u_int16_t       necp_last_interface_index;
	u_int16_t       necp_app_id;
};

union builtin_mtag {
	struct {
		struct proto_mtag_ _proto_mtag;                                     
		struct pf_mtag  _pf_mtag;                            
		struct necp_mtag_ _necp_mtag;                        
	} _net_mtag;
	struct driver_mtag_ _drv_mtag;
#define necp_mtag builtin_mtag._net_mtag._necp_mtag
#define proto_mtag builtin_mtag._net_mtag._proto_mtag
#define driver_mtag builtin_mtag._drv_mtag
};

  
                                                                           
   
struct pkthdr {
	struct ifnet *rcvif;                               
	                                         
	void    *pkt_hdr;                                             
	int32_t len;                                             
	                                     
	                                                             
	u_int32_t csum_flags;                                         
	union {
		struct {
			u_int16_t val;                       
			u_int16_t start;                            
		} _csum_rx;
#define csum_rx_val     _csum_rx.val
#define csum_rx_start   _csum_rx.start
		struct {
			u_int16_t start;                            
			u_int16_t stuff;                            
		} _csum_tx;
#define csum_tx_start   _csum_tx.start
#define csum_tx_stuff   _csum_tx.stuff
		  
                                              
                                                    
     
		u_int32_t csum_data;
	};
	u_int16_t vlan_tag;                                            
	  
                          
   
                                                                   
                                                                    
                                                                 
                                                  
   
                                                                      
                                                                   
                                                                     
                   
   
                                                                   
                                                                 
                                                                      
                                                                  
                                                                      
                                                                 
                                                                      
                                                                      
                                                                  
                                                                       
                                                                      
                                                                      
                                                                       
                                                                      
                                                                  
    
	u_int8_t pkt_proto;                                
	u_int8_t pkt_flowsrc;                               
	u_int32_t pkt_flowid;                        
	u_int32_t pkt_flags;                                        
	u_int32_t pkt_svc;                                  

	u_int32_t pkt_compl_context;                                           

	union {
		struct {
			u_int16_t src;                                       
			u_int16_t src_flags;                              
			u_int16_t dst;                                       
			u_int16_t dst_flags;                              
		} _pkt_iaif;
#define src_ifindex     _pkt_iaif.src
#define src_iff         _pkt_iaif.src_flags
#define dst_ifindex     _pkt_iaif.dst
#define dst_iff         _pkt_iaif.dst_flags
		u_int64_t pkt_ifainfo;                                  
		struct {
			u_int32_t if_data;                               
			u_int32_t sndbuf_data;                             
		} _pkt_bsr;                                                          
#define bufstatus_if    _pkt_bsr.if_data
#define bufstatus_sndbuf        _pkt_bsr.sndbuf_data
	};
	u_int64_t pkt_timestamp;                                                     

	  
                                
    
	SLIST_HEAD(packet_tags, m_tag) tags;                            
	union builtin_mtag builtin_mtag;

	uint32_t comp_gencnt;
	uint32_t padding;
	  
                                                                     
                                                                   
                                                                 
                                                    
    
	struct {
		union {
			u_int8_t        __mpriv8[16];
			u_int16_t       __mpriv16[8];
			struct {
				union {
					u_int8_t        __val8[4];
					u_int16_t       __val16[2];
					u_int32_t       __val32;
				} __mpriv32_u;
			}               __mpriv32[4];
			u_int64_t       __mpriv64[2];
		} __mpriv_u;
	} pkt_mpriv __attribute__((aligned(4)));
#define pkt_mpriv_hash  pkt_mpriv.__mpriv_u.__mpriv32[0].__mpriv32_u.__val32
#define pkt_mpriv_flags pkt_mpriv.__mpriv_u.__mpriv32[1].__mpriv32_u.__val32
#define pkt_mpriv_srcid pkt_mpriv.__mpriv_u.__mpriv32[2].__mpriv32_u.__val32
#define pkt_mpriv_fidx  pkt_mpriv.__mpriv_u.__mpriv32[3].__mpriv32_u.__val32

	u_int32_t redzone;                            
	u_int32_t pkt_compl_callbacks;                                   
};

  
                                                                             
                                                                       
                                                                            
                                                                       
                                                      
   
#define FLOWSRC_INPCB           1                                       
#define FLOWSRC_IFNET           2                                           
#define FLOWSRC_PF              3                                    
#define FLOWSRC_CHANNEL         4                                         

  
                                                                    
                                                                           
                                                                      
                                                                             
             
                                                                                
                                                                            
                                         
                                                              
                                                                    
                                                                                
   

  
                                                                        
                                                                         
                                                                 
  
                                         
  
                                                                             
                                                                     
                                                                      
                                                                     
                                                                     
                                                                       
                                                                   
                                                                      
                                                               
  
                                                                         
                                                                    
                               
   
#define PKTF_FLOW_ID            0x1                                     
#define PKTF_FLOW_ADV           0x2                                           
#define PKTF_FLOW_LOCALSRC      0x4                                     
#define PKTF_FLOW_RAWSOCK       0x8                                            
#define PKTF_PRIO_PRIVILEGED    0x10                                       
#define PKTF_PROXY_DST          0x20                                            
#define PKTF_INET_RESOLVE       0x40                              
#define PKTF_INET6_RESOLVE      0x80                              
#define PKTF_RESOLVE_RTR        0x100                                    
#define PKTF_SKIP_PKTAP         0x200                                             
#define PKTF_MPTCP              0x800                                
#define PKTF_MPSO               0x1000                              
#define PKTF_LOOP               0x2000                         
#define PKTF_IFAINFO            0x4000                                         
#define PKTF_SO_BACKGROUND      0x8000                                      
#define PKTF_FORWARDED          0x10000                                         
#define PKTF_PRIV_GUARDED       0x20000                                   
#define PKTF_KEEPALIVE          0x40000                                        
#define PKTF_SO_REALTIME        0x80000                               
#define PKTF_VALID_UNSENT_DATA  0x100000                           
#define PKTF_TCP_REXMT          0x200000                                   
#define PKTF_REASSEMBLED        0x400000                             
#define PKTF_TX_COMPL_TS_REQ    0x800000                                        
#define PKTF_TS_VALID           0x1000000                             
#define PKTF_DRIVER_MTAG        0x2000000                                     
#define PKTF_NEW_FLOW           0x4000000                           
#define PKTF_START_SEQ          0x8000000                           
#define PKTF_LAST_PKT           0x10000000                              
#define PKTF_MPTCP_REINJ        0x20000000                                           
#define PKTF_MPTCP_DFIN         0x40000000                           
#define PKTF_HBH_CHKED          0x80000000                            

                                                               
#define PKTF_FLOW_MASK  \
	(PKTF_FLOW_ID | PKTF_FLOW_ADV | PKTF_FLOW_LOCALSRC | PKTF_FLOW_RAWSOCK)

  
                                                                             
   
typedef void (*m_ext_free_func_t)(caddr_t, u_int, caddr_t);
struct m_ext {
	caddr_t ext_buf;                                     
	m_ext_free_func_t ext_free;                                        
	u_int   ext_size;                                                 
	caddr_t ext_arg;                                                  
	struct ext_ref {
		struct mbuf *paired;
		u_int16_t minref;
		u_int16_t refcnt;
		u_int16_t prefcnt;
		u_int16_t flags;
		u_int32_t priv;
		uintptr_t ext_token;
	} *ext_refflags;
};

                                                          
typedef struct m_ext _m_ext_t;

  
                  
   
struct mbuf {
	struct m_hdr m_hdr;
	union {
		struct {
			struct pkthdr MH_pkthdr;                          
			union {
				struct m_ext MH_ext;                   
				char    MH_databuf[_MHLEN];
			} MH_dat;
		} MH;
		char    M_databuf[_MLEN];                                      
	} M_dat;
};

#define m_next          m_hdr.mh_next
#define m_len           m_hdr.mh_len
#define m_data          m_hdr.mh_data
#define m_type          m_hdr.mh_type
#define m_flags         m_hdr.mh_flags
#define m_nextpkt       m_hdr.mh_nextpkt
#define m_act           m_nextpkt
#define m_pkthdr        M_dat.MH.MH_pkthdr
#define m_ext           M_dat.MH.MH_dat.MH_ext
#define m_pktdat        M_dat.MH.MH_dat.MH_databuf
#define m_dat           M_dat.M_databuf
#define m_pktlen(_m)    ((_m)->m_pkthdr.len)
#define m_pftag(_m)     (&(_m)->m_pkthdr.builtin_mtag._net_mtag._pf_mtag)
#define m_necptag(_m)   (&(_m)->m_pkthdr.builtin_mtag._net_mtag._necp_mtag)

                          
#define M_EXT           0x0001                                       
#define M_PKTHDR        0x0002                       
#define M_EOR           0x0004                     
#define M_PROTO1        0x0008                         
#define M_PROTO2        0x0010                         
#define M_PROTO3        0x0020                         
#define M_LOOP          0x0040                                                  
#define M_PROTO5        0x0080                         

                                                  
#define M_BCAST         0x0100                                             
#define M_MCAST         0x0200                                             
#define M_FRAG          0x0400                                               
#define M_FIRSTFRAG     0x0800                                
#define M_LASTFRAG      0x1000                               
#define M_PROMISC       0x2000                                                     
#define M_HASFCS        0x4000                      
#define M_TAGHDR        0x8000                                               

  
                                       
   
#define M_PROTOFLAGS \
	(M_PROTO1|M_PROTO2|M_PROTO3|M_PROTO5)

                                        
#define M_COPYFLAGS                                                     \
	(M_PKTHDR|M_EOR|M_PROTO1|M_PROTO2|M_PROTO3 |                    \
	M_LOOP|M_PROTO5|M_BCAST|M_MCAST|M_FRAG |                        \
	M_FIRSTFRAG|M_LASTFRAG|M_PROMISC|M_HASFCS)

                                                                       
#define CSUM_IP                 0x0001                            
#define CSUM_TCP                0x0002                             
#define CSUM_UDP                0x0004                             
#define CSUM_IP_FRAGS           0x0008                                      
#define CSUM_FRAGMENT           0x0010                                        
#define CSUM_TCPIPV6            0x0020                                      
#define CSUM_UDPIPV6            0x0040                                      
#define CSUM_FRAGMENT_IPV6      0x0080                                          

#define CSUM_IP_CHECKED         0x0100                           
#define CSUM_IP_VALID           0x0200                                       
#define CSUM_DATA_VALID         0x0400                                        
#define CSUM_PSEUDO_HDR         0x0800                                        
#define CSUM_PARTIAL            0x1000                                        
#define CSUM_ZERO_INVERT        0x2000                                       

#define CSUM_DELAY_DATA         (CSUM_TCP | CSUM_UDP)
#define CSUM_DELAY_IP           (CSUM_IP)                                        
#define CSUM_DELAY_IPV6_DATA    (CSUM_TCPIPV6 | CSUM_UDPIPV6)
#define CSUM_DATA_IPV6_VALID    CSUM_DATA_VALID                               

#define CSUM_TX_FLAGS                                                   \
	(CSUM_DELAY_IP | CSUM_DELAY_DATA | CSUM_DELAY_IPV6_DATA |       \
	CSUM_DATA_VALID | CSUM_PARTIAL | CSUM_ZERO_INVERT)

#define CSUM_RX_FULL_FLAGS                                              \
	(CSUM_IP_CHECKED | CSUM_IP_VALID | CSUM_PSEUDO_HDR |            \
	CSUM_DATA_VALID)

#define CSUM_RX_FLAGS                                                   \
	(CSUM_RX_FULL_FLAGS | CSUM_PARTIAL)



  
                                                            
   

                      
#define CSUM_VLAN_TAG_VALID     0x00010000                                   

                                             
#define CSUM_ADJUST_DONE        0x00020000

                                
#define CSUM_VLAN_ENCAP_PRESENT    0x00040000                                       

                                                   
#define CSUM_TSO_IPV4           0x00100000                                                      
#define CSUM_TSO_IPV6           0x00200000                                                      

#define TSO_IPV4_OK(_ifp, _m)                                           \
    (((_ifp)->if_hwassist & IFNET_TSO_IPV4) &&                          \
    ((_m)->m_pkthdr.csum_flags & CSUM_TSO_IPV4))                        \

#define TSO_IPV4_NOTOK(_ifp, _m)                                        \
    (!((_ifp)->if_hwassist & IFNET_TSO_IPV4) &&                         \
    ((_m)->m_pkthdr.csum_flags & CSUM_TSO_IPV4))                        \

#define TSO_IPV6_OK(_ifp, _m)                                           \
    (((_ifp)->if_hwassist & IFNET_TSO_IPV6) &&                          \
    ((_m)->m_pkthdr.csum_flags & CSUM_TSO_IPV6))                        \

#define TSO_IPV6_NOTOK(_ifp, _m)                                        \
    (!((_ifp)->if_hwassist & IFNET_TSO_IPV6) &&                         \
    ((_m)->m_pkthdr.csum_flags & CSUM_TSO_IPV6))                        \

#endif                         

                
#define MT_FREE         0                                   
#define MT_DATA         1                                      
#define MT_HEADER       2                          
#define MT_SOCKET       3                             
#define MT_PCB          4                                   
#define MT_RTABLE       5                           
#define MT_HTABLE       6                            
#define MT_ATABLE       7                                      
#define MT_SONAME       8                        
#define MT_SOOPTS       10                          
#define MT_FTABLE       11                                      
#define MT_RIGHTS       12                         
#define MT_IFADDR       13                             
#define MT_CONTROL      14                                       
#define MT_OOBDATA      15                           
#define MT_TAG          16                                                
#define MT_MAX          32                   

#ifdef XNU_KERNEL_PRIVATE
  
                                       
  
                                          
                                                                 
  
                                             
                                                                  
                     
   

#if 1
#define MCHECK(m) m_mcheck(m)
#else
#define MCHECK(m)
#endif

#define MGET(m, how, type) ((m) = m_get((how), (type)))

#define MGETHDR(m, how, type)   ((m) = m_gethdr((how), (type)))

  
                       
                                                          
                                              
                                      
                                                                   
                                                            
  
                                                                
                                                                        
                                  
   
union mcluster {
	union   mcluster *mcl_next;
	char    mcl_buf[MCLBYTES];
};

#define MCLALLOC(p, how)        ((p) = m_mclalloc(how))

#define MCLFREE(p)              m_mclfree(p)

#define MCLGET(m, how)          ((m) = m_mclget(m, how))

  
                   
   
union mbigcluster {
	union mbigcluster       *mbc_next;
	char                    mbc_buf[MBIGCLBYTES];
};

  
                     
   
union m16kcluster {
	union m16kcluster       *m16kcl_next;
	char                    m16kcl_buf[M16KCLBYTES];
};

#define MCLHASREFERENCE(m)      m_mclhasreference(m)

  
                                        
                                                      
                                     
   

#define MFREE(m, n) ((n) = m_free(m))

  
                                    
                                                     
                                     
   
#define M_COPY_PKTHDR(to, from)         m_copy_pkthdr(to, from)

#define M_COPY_PFTAG(to, from)          m_copy_pftag(to, from)

#define M_COPY_NECPTAG(to, from)        m_copy_necptag(to, from)

#define M_COPY_CLASSIFIER(to, from)     m_copy_classifier(to, from)

  
                                                                            
                                                                           
                         
   
#define M_WRITABLE(m)   (((m)->m_flags & M_EXT) == 0 || !MCLHASREFERENCE(m))

  
                                                                        
                                                                         
                                     
   
#define MLEN            mbuf_get_mlen()                                   
#define MHLEN           mbuf_get_mhlen()                                          
#define MINCLSIZE       mbuf_get_minclsize()                                 
  
                                                                         
                                                                          
   
#define M_START(m)                                                      \
	(((m)->m_flags & M_EXT) ? (m)->m_ext.ext_buf :                  \
	 ((m)->m_flags & M_PKTHDR) ? &(m)->m_pktdat[0] :                \
	 &(m)->m_dat[0])

  
                                                                           
                                                        
   
#define M_SIZE(m)                                                       \
	(((m)->m_flags & M_EXT) ? (m)->m_ext.ext_size :                 \
	 ((m)->m_flags & M_PKTHDR) ? MHLEN :                            \
	 MLEN)

#define M_ALIGN(m, len)         m_align(m, len)
#define MH_ALIGN(m, len)        m_align(m, len)
#define MEXT_ALIGN(m, len)      m_align(m, len)

  
                                                                            
           
  
                                                                           
                                                                              
   
#define M_LEADINGSPACE(m)                                               \
	(M_WRITABLE(m) ? ((m)->m_data - M_START(m)) : 0)

  
                                                                          
  
                                                                           
                                                                              
   
#define M_TRAILINGSPACE(m)                                              \
	(M_WRITABLE(m) ?                                                \
	    ((M_START(m) + M_SIZE(m)) - ((m)->m_data + (m)->m_len)) : 0)

  
                                                   
                                                                  
                                                                     
                                 
   
#define M_PREPEND(m, plen, how, align)  \
    ((m) = m_prepend_2((m), (plen), (how), (align)))

                             
#define MCHTYPE(m, t)           m_mchtype(m, t)

                           
#define m_copy(m, o, l)         m_copym((m), (o), (l), M_DONTWAIT)

#define MBSHIFT         20                                       
#define MBSIZE          (1 << MBSHIFT)
#define GBSHIFT         30                                       
#define GBSIZE          (1 << GBSHIFT)

  
                                                                        
                                                                     
                                                                        
                   
  
                                                                      
                                                                  
   
#define M_STRUCT_GET(val, typ, m, off, len)                             \
do {                                                                    \
	struct mbuf *t;                                                 \
	int tmp;                                                        \
                                                                        \
	if ((m)->m_len >= (off) + (len)) {                              \
	        (val) = (typ)(mtod((m), caddr_t) + (off));              \
	} else {                                                        \
	        t = m_pulldown((m), (off), (len), &tmp);                \
	        if (t != NULL) {                                        \
	                if (t->m_len < tmp + (len))                     \
	                        panic("m_pulldown malfunction");        \
	                (val) = (typ)(mtod(t, caddr_t) + tmp);          \
	        } else {                                                \
	                (val) = (typ)NULL;                              \
	                (m) = NULL;                                     \
	        }                                                       \
	}                                                               \
} while (0)

#define M_STRUCT_GET0(val, typ, m, off, len)                            \
do {                                                                    \
	struct mbuf *t;                                                 \
                                                                        \
	if ((off) == 0 && ((m)->m_len >= (len))) {                      \
	        (val) = (typ)(void *)mtod(m, caddr_t);                  \
	} else {                                                        \
	        t = m_pulldown((m), (off), (len), NULL);                \
	        if (t != NULL) {                                        \
	                if (t->m_len < (len))                           \
	                        panic("m_pulldown malfunction");        \
	                (val) = (typ)(void *)mtod(t, caddr_t);          \
	        } else {                                                \
	                (val) = (typ)NULL;                              \
	                (m) = NULL;                                     \
	        }                                                       \
	}                                                               \
} while (0)

#define MBUF_INPUT_CHECK(m, rcvif)                                      \
do {                                                                    \
	if (!(m->m_flags & MBUF_PKTHDR) ||                              \
	    m->m_len < 0 ||                                             \
	    m->m_len > ((njcl > 0) ? njclbytes : MBIGCLBYTES) ||        \
	    m->m_type == MT_FREE ||                                     \
	    ((m->m_flags & M_EXT) != 0 && m->m_ext.ext_buf == NULL)) {  \
	        panic_plain("Failed mbuf validity check: mbuf %p len %d "  \
	            "type %d flags 0x%x data %p rcvif %s ifflags 0x%x",  \
	            m, m->m_len, m->m_type, m->m_flags,                    \
	            ((m->m_flags & M_EXT) ? m->m_ext.ext_buf : m->m_data), \
	            if_name(rcvif),                                     \
	            (rcvif->if_flags & 0xffff));                        \
	}                                                               \
} while (0)

  
                              
  
                                                              
                                        
  
                                                             
   
#define MBUFQ_HEAD(name)                                        \
struct name {                                                   \
	struct mbuf *mq_first;                                  \
	struct mbuf **mq_last;                                  \
}

#define MBUFQ_INIT(q)           do {                            \
	MBUFQ_FIRST(q) = NULL;                                  \
	(q)->mq_last = &MBUFQ_FIRST(q);                         \
} while (0)

#define MBUFQ_PREPEND(q, m)     do {                            \
	if ((MBUFQ_NEXT(m) = MBUFQ_FIRST(q)) == NULL)           \
	        (q)->mq_last = &MBUFQ_NEXT(m);                  \
	MBUFQ_FIRST(q) = (m);                                   \
} while (0)

#define MBUFQ_ENQUEUE(q, m)     do {                            \
	MBUFQ_NEXT(m) = NULL;                                   \
	*(q)->mq_last = (m);                                    \
	(q)->mq_last = &MBUFQ_NEXT(m);                          \
} while (0)

#define MBUFQ_ENQUEUE_MULTI(q, m, n)    do {                    \
	MBUFQ_NEXT(n) = NULL;                                   \
	*(q)->mq_last = (m);                                    \
	(q)->mq_last = &MBUFQ_NEXT(n);                          \
} while (0)

#define MBUFQ_DEQUEUE(q, m)     do {                            \
	if (((m) = MBUFQ_FIRST(q)) != NULL) {                   \
	        if ((MBUFQ_FIRST(q) = MBUFQ_NEXT(m)) == NULL)   \
	                (q)->mq_last = &MBUFQ_FIRST(q);         \
	        else                                            \
	                MBUFQ_NEXT(m) = NULL;                   \
	}                                                       \
} while (0)

#define MBUFQ_REMOVE(q, m)      do {                            \
	if (MBUFQ_FIRST(q) == (m)) {                            \
	        MBUFQ_DEQUEUE(q, m);                            \
	} else {                                                \
	        struct mbuf *_m = MBUFQ_FIRST(q);               \
	        while (MBUFQ_NEXT(_m) != (m))                   \
	                _m = MBUFQ_NEXT(_m);                    \
	        if ((MBUFQ_NEXT(_m) =                           \
	            MBUFQ_NEXT(MBUFQ_NEXT(_m))) == NULL)        \
	                (q)->mq_last = &MBUFQ_NEXT(_m);         \
	}                                                       \
} while (0)

#define MBUFQ_DRAIN(q)          do {                            \
	struct mbuf *__m0;                                      \
	while ((__m0 = MBUFQ_FIRST(q)) != NULL) {               \
	        MBUFQ_FIRST(q) = MBUFQ_NEXT(__m0);              \
	        MBUFQ_NEXT(__m0) = NULL;                        \
	        m_freem(__m0);                                  \
	}                                                       \
	(q)->mq_last = &MBUFQ_FIRST(q);                         \
} while (0)

#define MBUFQ_FOREACH(m, q)                                     \
	for ((m) = MBUFQ_FIRST(q);                              \
	    (m);                                                \
	    (m) = MBUFQ_NEXT(m))

#define MBUFQ_FOREACH_SAFE(m, q, tvar)                          \
	for ((m) = MBUFQ_FIRST(q);                              \
	    (m) && ((tvar) = MBUFQ_NEXT(m), 1);                 \
	    (m) = (tvar))

#define MBUFQ_EMPTY(q)          ((q)->mq_first == NULL)
#define MBUFQ_FIRST(q)          ((q)->mq_first)
#define MBUFQ_NEXT(m)           ((m)->m_nextpkt)
  
                                                                   
                                                                    
                                                                    
                                                
   
#define MBUFQ_LAST(head)                                        \
	(((head)->mq_last == &MBUFQ_FIRST(head)) ? NULL :       \
	((struct mbuf *)(void *)((char *)(head)->mq_last -      \
	     __builtin_offsetof(struct mbuf, m_nextpkt))))

#define max_linkhdr     (int)P2ROUNDUP(_max_linkhdr, sizeof (uint32_t))
#define max_protohdr    (int)P2ROUNDUP(_max_protohdr, sizeof (uint32_t))
#endif                         

  
                            
   
struct mbstat {
	u_int32_t       m_mbufs;                                           
	u_int32_t       m_clusters;                                           
	u_int32_t       m_spare;                         
	u_int32_t       m_clfree;                          
	u_int32_t       m_drops;                                        
	u_int32_t       m_wait;                                     
	u_int32_t       m_drain;                                               
	u_short         m_mtypes[256];                                      
	u_int32_t       m_mcfail;                                 
	u_int32_t       m_mpfail;                                  
	u_int32_t       m_msize;                               
	u_int32_t       m_mclbytes;                                    
	u_int32_t       m_minclsize;                                                  
	u_int32_t       m_mlen;                                        
	u_int32_t       m_mhlen;                                             
	u_int32_t       m_bigclusters;                                        
	u_int32_t       m_bigclfree;                       
	u_int32_t       m_bigmclbytes;                                 
	u_int32_t       m_forcedefunct;                                                 
};

                              
struct ombstat {
	u_int32_t       m_mbufs;                                           
	u_int32_t       m_clusters;                                           
	u_int32_t       m_spare;                         
	u_int32_t       m_clfree;                          
	u_int32_t       m_drops;                                        
	u_int32_t       m_wait;                                     
	u_int32_t       m_drain;                                               
	u_short         m_mtypes[256];                                      
	u_int32_t       m_mcfail;                                 
	u_int32_t       m_mpfail;                                  
	u_int32_t       m_msize;                               
	u_int32_t       m_mclbytes;                                    
	u_int32_t       m_minclsize;                                                  
	u_int32_t       m_mlen;                                        
	u_int32_t       m_mhlen;                                             
};

  
                         
   
#define MAX_MBUF_CNAME  15

#if defined(XNU_KERNEL_PRIVATE)
                                                              
struct omb_class_stat {
	char            mbcl_cname[MAX_MBUF_CNAME + 1];                 
	u_int32_t       mbcl_size;                       
	u_int32_t       mbcl_total;                               
	u_int32_t       mbcl_active;                             
	u_int32_t       mbcl_infree;                                
	u_int32_t       mbcl_slab_cnt;                            
	u_int64_t       mbcl_alloc_cnt;                                 
	u_int64_t       mbcl_free_cnt;                                 
	u_int64_t       mbcl_notified;                             
	u_int64_t       mbcl_purge_cnt;                         
	u_int64_t       mbcl_fail_cnt;                                
	u_int32_t       mbcl_ctotal;                                   
	u_int32_t       mbcl_release_cnt;                                
	  
                          
    
	u_int32_t       mbcl_mc_state;                               
	u_int32_t       mbcl_mc_cached;                          
	u_int32_t       mbcl_mc_waiter_cnt;                              
	u_int32_t       mbcl_mc_wretry_cnt;                         
	u_int32_t       mbcl_mc_nwretry_cnt;                                  
	u_int64_t       mbcl_reserved[4];                        
} __attribute__((__packed__));
#endif                         

typedef struct mb_class_stat {
	char            mbcl_cname[MAX_MBUF_CNAME + 1];                 
	u_int32_t       mbcl_size;                       
	u_int32_t       mbcl_total;                               
	u_int32_t       mbcl_active;                             
	u_int32_t       mbcl_infree;                                
	u_int32_t       mbcl_slab_cnt;                            
#if defined(KERNEL) || defined(__LP64__)
	u_int32_t       mbcl_pad;                    
#endif                         
	u_int64_t       mbcl_alloc_cnt;                                 
	u_int64_t       mbcl_free_cnt;                                 
	u_int64_t       mbcl_notified;                             
	u_int64_t       mbcl_purge_cnt;                         
	u_int64_t       mbcl_fail_cnt;                                
	u_int32_t       mbcl_ctotal;                                   
	u_int32_t       mbcl_release_cnt;                                
	  
                          
    
	u_int32_t       mbcl_mc_state;                               
	u_int32_t       mbcl_mc_cached;                          
	u_int32_t       mbcl_mc_waiter_cnt;                              
	u_int32_t       mbcl_mc_wretry_cnt;                         
	u_int32_t       mbcl_mc_nwretry_cnt;                                  
	u_int32_t       mbcl_peak_reported;                               
	u_int32_t       mbcl_reserved[7];                        
} mb_class_stat_t;

#define MCS_DISABLED    0                                          
#define MCS_ONLINE      1                            
#define MCS_PURGING     2                                  
#define MCS_OFFLINE     3                                        

#if defined(XNU_KERNEL_PRIVATE)
                                                              
struct omb_stat {
	u_int32_t               mbs_cnt;                               
	struct omb_class_stat   mbs_class[1];                    
} __attribute__((__packed__));
#endif                         

typedef struct mb_stat {
	u_int32_t       mbs_cnt;                               
#if defined(KERNEL) || defined(__LP64__)
	u_int32_t       mbs_pad;                     
#endif                         
	mb_class_stat_t mbs_class[1];                    
} mb_stat_t;

#ifdef PRIVATE
#define MLEAK_STACK_DEPTH       16                              

typedef struct mleak_trace_stat {
	u_int64_t       mltr_collisions;
	u_int64_t       mltr_hitcount;
	u_int64_t       mltr_allocs;
	u_int64_t       mltr_depth;
	u_int64_t       mltr_addr[MLEAK_STACK_DEPTH];
} mleak_trace_stat_t;

typedef struct mleak_stat {
	u_int32_t               ml_isaddr64;                     
	u_int32_t               ml_cnt;                               
	mleak_trace_stat_t      ml_trace[1];                     
} mleak_stat_t;

struct mleak_table {
	u_int32_t mleak_capture;                                      
	u_int32_t mleak_sample_factor;                     

	                                                           
	u_int64_t alloc_collisions;
	u_int64_t trace_collisions;

	                                                                       
	u_int64_t alloc_overwrites;
	u_int64_t trace_overwrites;

	                                                           
	u_int64_t alloc_recorded;
	u_int64_t trace_recorded;

	                                        
	u_int64_t outstanding_allocs;

	                                                                      
	u_int64_t total_conflicts;
};
#endif              

#ifdef KERNEL_PRIVATE
__BEGIN_DECLS

  
                     
   

extern struct mbstat mbstat;                                    

__END_DECLS
#endif                     

#ifdef XNU_KERNEL_PRIVATE
__BEGIN_DECLS

  
                             
   

                         
                                                               
#include        <sys/malloc.h>

struct mbuf;

                                  
#define M_COPYALL       1000000000

#define M_DONTWAIT      M_NOWAIT
#define M_WAIT          M_WAITOK

                                    
#define M_COPYM_NOOP_HDR        0                                            
#define M_COPYM_COPY_HDR        1                                        
#define M_COPYM_MOVE_HDR        2                                        
#define M_COPYM_MUST_COPY_HDR   3                                             
#define M_COPYM_MUST_MOVE_HDR   4                                             

extern void m_freem(struct mbuf *) __XNU_INTERNAL(m_freem);
extern u_int64_t mcl_to_paddr(char *);
extern void m_adj(struct mbuf *, int);
extern void m_cat(struct mbuf *, struct mbuf *);
extern void m_copydata(struct mbuf *, int, int, void *);
extern struct mbuf *m_copym(struct mbuf *, int, int, int);
extern struct mbuf *m_copym_mode(struct mbuf *, int, int, int, uint32_t);
extern struct mbuf *m_get(int, int);
extern struct mbuf *m_gethdr(int, int);
extern struct mbuf *m_getpacket(void);
extern struct mbuf *m_getpackets(int, int, int);
extern struct mbuf *m_mclget(struct mbuf *, int);
extern void *m_mtod(struct mbuf *);
extern struct mbuf *m_prepend_2(struct mbuf *, int, int, int);
extern struct mbuf *m_pullup(struct mbuf *, int);
extern struct mbuf *m_split(struct mbuf *, int, int);
extern void m_mclfree(caddr_t p);
extern int mbuf_get_class(struct mbuf *m);
extern bool mbuf_class_under_pressure(struct mbuf *m);

  
                                                                          
                                                                         
                                                                         
                                  
   
#if defined(__i386__) || defined(__x86_64__)
#define MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(_m)
#else                               
#define MBUF_STRICT_DATA_ALIGNMENT_CHECK_32(_m) do {                    \
	if (!IS_P2ALIGNED((_m)->m_data, sizeof (u_int32_t))) {          \
	        if (((_m)->m_flags & M_PKTHDR) &&                       \
	            (_m)->m_pkthdr.rcvif != NULL) {                     \
	                panic_plain("\n%s: mbuf %p data ptr %p is not " \
	                    "32-bit aligned [%s: alignerrs=%lld]\n",    \
	                    __func__, (_m), (_m)->m_data,               \
	                    if_name((_m)->m_pkthdr.rcvif),              \
	                    (_m)->m_pkthdr.rcvif->if_alignerrs);        \
	        } else {                                                \
	                panic_plain("\n%s: mbuf %p data ptr %p is not " \
	                    "32-bit aligned\n",                         \
	                    __func__, (_m), (_m)->m_data);              \
	        }                                                       \
	}                                                               \
} while (0)
#endif                               

                                                                 
#define MBUF_SC_MAX_CLASSES     10

  
                                                                
                                                              
  
                                  
               
  
                              
               
                
  
                              
               
               
                
  
                              
                
  
                                                                         
                                                                             
                                                                            
                                                                        
                                                                          
                     
   
#define MBUF_SCVAL(x)           ((x) & 0xffff)
#define MBUF_SCIDX(x)           ((((x) >> 16) & 0xff) >> 3)
#define MBUF_SC2TC(_sc)         (MBUF_SCVAL(_sc) >> 7)
#define MBUF_TC2SCVAL(_tc)      ((_tc) << 7)
#define IS_MBUF_SC_BACKGROUND(_sc) (((_sc) == MBUF_SC_BK_SYS) || \
	((_sc) == MBUF_SC_BK))
#define IS_MBUF_SC_REALTIME(_sc)        ((_sc) >= MBUF_SC_AV && (_sc) <= MBUF_SC_VO)
#define IS_MBUF_SC_BESTEFFORT(_sc)      ((_sc) == MBUF_SC_BE || \
    (_sc) == MBUF_SC_RD || (_sc) == MBUF_SC_OAM)

#define SCIDX_BK_SYS            MBUF_SCIDX(MBUF_SC_BK_SYS)
#define SCIDX_BK                MBUF_SCIDX(MBUF_SC_BK)
#define SCIDX_BE                MBUF_SCIDX(MBUF_SC_BE)
#define SCIDX_RD                MBUF_SCIDX(MBUF_SC_RD)
#define SCIDX_OAM               MBUF_SCIDX(MBUF_SC_OAM)
#define SCIDX_AV                MBUF_SCIDX(MBUF_SC_AV)
#define SCIDX_RV                MBUF_SCIDX(MBUF_SC_RV)
#define SCIDX_VI                MBUF_SCIDX(MBUF_SC_VI)
#define SCIDX_SIG               MBUF_SCIDX(MBUF_SC_SIG)
#define SCIDX_VO                MBUF_SCIDX(MBUF_SC_VO)
#define SCIDX_CTL               MBUF_SCIDX(MBUF_SC_CTL)

#define SCVAL_BK_SYS            MBUF_SCVAL(MBUF_SC_BK_SYS)
#define SCVAL_BK                MBUF_SCVAL(MBUF_SC_BK)
#define SCVAL_BE                MBUF_SCVAL(MBUF_SC_BE)
#define SCVAL_RD                MBUF_SCVAL(MBUF_SC_RD)
#define SCVAL_OAM               MBUF_SCVAL(MBUF_SC_OAM)
#define SCVAL_AV                MBUF_SCVAL(MBUF_SC_AV)
#define SCVAL_RV                MBUF_SCVAL(MBUF_SC_RV)
#define SCVAL_VI                MBUF_SCVAL(MBUF_SC_VI)
#define SCVAL_SIG               MBUF_SCVAL(MBUF_SC_SIG)
#define SCVAL_VO                MBUF_SCVAL(MBUF_SC_VO)
#define SCVAL_CTL               MBUF_SCVAL(MBUF_SC_CTL)

#define MBUF_VALID_SC(c)                                                \
	(c == MBUF_SC_BK_SYS || c == MBUF_SC_BK || c == MBUF_SC_BE ||   \
	c == MBUF_SC_RD || c == MBUF_SC_OAM || c == MBUF_SC_AV ||       \
	c == MBUF_SC_RV || c == MBUF_SC_VI || c == MBUF_SC_SIG ||       \
	c == MBUF_SC_VO || c == MBUF_SC_CTL)

#define MBUF_VALID_SCIDX(c)                                             \
	(c == SCIDX_BK_SYS || c == SCIDX_BK || c == SCIDX_BE ||         \
	c == SCIDX_RD || c == SCIDX_OAM || c == SCIDX_AV ||             \
	c == SCIDX_RV || c == SCIDX_VI || c == SCIDX_SIG ||             \
	c == SCIDX_VO || c == SCIDX_CTL)

#define MBUF_VALID_SCVAL(c)                                             \
	(c == SCVAL_BK_SYS || c == SCVAL_BK || c == SCVAL_BE ||         \
	c == SCVAL_RD || c == SCVAL_OAM || c == SCVAL_AV ||             \
	c == SCVAL_RV || c == SCVAL_VI || c == SCVAL_SIG ||             \
	c == SCVAL_VO || SCVAL_CTL)

extern unsigned char *mbutl;                               
extern unsigned char *embutl;                            
extern unsigned int nmbclusters;                                       
extern int njcl;                                          
extern int njclbytes;                                
extern int max_hdr;                                               
extern int max_datalen;                      

                                             
extern int _max_linkhdr;                                       

                                               
extern int _max_protohdr;                                    

__private_extern__ unsigned int mbuf_default_ncl(uint64_t);
__private_extern__ void mbinit(void);
__private_extern__ struct mbuf *m_clattach(struct mbuf *, int, caddr_t,
    void (*)(caddr_t, u_int, caddr_t), u_int, caddr_t, int, int);
__private_extern__ caddr_t m_bigalloc(int);
__private_extern__ void m_bigfree(caddr_t, u_int, caddr_t);
__private_extern__ struct mbuf *m_mbigget(struct mbuf *, int);
__private_extern__ caddr_t m_16kalloc(int);
__private_extern__ void m_16kfree(caddr_t, u_int, caddr_t);
__private_extern__ struct mbuf *m_m16kget(struct mbuf *, int);
__private_extern__ int m_reinit(struct mbuf *, int);
__private_extern__ struct mbuf *m_free(struct mbuf *) __XNU_INTERNAL(m_free);
__private_extern__ struct mbuf *m_getclr(int, int);
__private_extern__ struct mbuf *m_getptr(struct mbuf *, int, int *);
__private_extern__ unsigned int m_length(struct mbuf *);
__private_extern__ unsigned int m_length2(struct mbuf *, struct mbuf **);
__private_extern__ unsigned int m_fixhdr(struct mbuf *);
__private_extern__ struct mbuf *m_defrag(struct mbuf *, int);
__private_extern__ struct mbuf *m_defrag_offset(struct mbuf *, u_int32_t, int);
__private_extern__ struct mbuf *m_prepend(struct mbuf *, int, int);
__private_extern__ struct mbuf *m_copyup(struct mbuf *, int, int);
__private_extern__ struct mbuf *m_retry(int, int);
__private_extern__ struct mbuf *m_retryhdr(int, int);
__private_extern__ int m_freem_list(struct mbuf *);
__private_extern__ int m_append(struct mbuf *, int, caddr_t);
__private_extern__ struct mbuf *m_last(struct mbuf *);
__private_extern__ struct mbuf *m_devget(char *, int, int, struct ifnet *,
    void (*)(const void *, void *, size_t));
__private_extern__ struct mbuf *m_pulldown(struct mbuf *, int, int, int *);

__private_extern__ struct mbuf *m_getcl(int, int, int);
__private_extern__ caddr_t m_mclalloc(int);
__private_extern__ int m_mclhasreference(struct mbuf *);
__private_extern__ void m_copy_pkthdr(struct mbuf *, struct mbuf *);
__private_extern__ void m_copy_pftag(struct mbuf *, struct mbuf *);
__private_extern__ void m_copy_necptag(struct mbuf *, struct mbuf *);
__private_extern__ void m_copy_classifier(struct mbuf *, struct mbuf *);

__private_extern__ struct mbuf *m_dtom(void *);
__private_extern__ int m_mtocl(void *);
__private_extern__ union mcluster *m_cltom(int);

__private_extern__ void m_align(struct mbuf *, int);

__private_extern__ struct mbuf *m_normalize(struct mbuf *m);
__private_extern__ void m_mchtype(struct mbuf *m, int t);
__private_extern__ void m_mcheck(struct mbuf *);

__private_extern__ void m_copyback(struct mbuf *, int, int, const void *);
__private_extern__ struct mbuf *m_copyback_cow(struct mbuf *, int, int,
    const void *, int);
__private_extern__ int m_makewritable(struct mbuf **, int, int, int);
__private_extern__ struct mbuf *m_dup(struct mbuf *m, int how);
__private_extern__ struct mbuf *m_copym_with_hdrs(struct mbuf *, int, int, int,
    struct mbuf **, int *, uint32_t);
__private_extern__ struct mbuf *m_getpackethdrs(int, int);
__private_extern__ struct mbuf *m_getpacket_how(int);
__private_extern__ struct mbuf *m_getpackets_internal(unsigned int *, int,
    int, int, size_t);
__private_extern__ struct mbuf *m_allocpacket_internal(unsigned int *, size_t,
    unsigned int *, int, int, size_t);

__private_extern__ int m_ext_set_prop(struct mbuf *, uint32_t, uint32_t);
__private_extern__ uint32_t m_ext_get_prop(struct mbuf *);
__private_extern__ int m_ext_paired_is_active(struct mbuf *);
__private_extern__ void m_ext_paired_activate(struct mbuf *);

__private_extern__ void mbuf_drain(boolean_t);

  
                                                                      
                                                                        
                                                                      
                                                                         
                                                                          
                                                                           
  
                                                                      
                                                                         
                            
  
                                                 
                                               
  
                                                                         
                                                                         
                                                              
  
                    
                                    
                   
          
                                                           
                                     
   

#define KERNEL_MODULE_TAG_ID    0

enum {
	KERNEL_TAG_TYPE_NONE                    = 0,
	KERNEL_TAG_TYPE_DUMMYNET                = 1,
	KERNEL_TAG_TYPE_DIVERT                  = 2,
	KERNEL_TAG_TYPE_IPFORWARD               = 3,
	KERNEL_TAG_TYPE_IPFILT                  = 4,
	KERNEL_TAG_TYPE_MACLABEL                = 5,
	KERNEL_TAG_TYPE_MAC_POLICY_LABEL        = 6,
	KERNEL_TAG_TYPE_ENCAP                   = 8,
	KERNEL_TAG_TYPE_INET6                   = 9,
	KERNEL_TAG_TYPE_IPSEC                   = 10,
	KERNEL_TAG_TYPE_DRVAUX                  = 11,
	KERNEL_TAG_TYPE_CFIL_UDP                = 13,
	KERNEL_TAG_TYPE_PF_REASS                = 14,
};

                         
__private_extern__ struct  m_tag *m_tag_alloc(u_int32_t, u_int16_t, int, int);
__private_extern__ struct  m_tag *m_tag_create(u_int32_t, u_int16_t, int, int,
    struct mbuf *);
__private_extern__ void m_tag_free(struct m_tag *);
__private_extern__ void m_tag_prepend(struct mbuf *, struct m_tag *);
__private_extern__ void m_tag_unlink(struct mbuf *, struct m_tag *);
__private_extern__ void m_tag_delete(struct mbuf *, struct m_tag *);
__private_extern__ void m_tag_delete_chain(struct mbuf *, struct m_tag *);
__private_extern__ struct m_tag *m_tag_locate(struct mbuf *, u_int32_t,
    u_int16_t, struct m_tag *);
__private_extern__ struct m_tag *m_tag_copy(struct m_tag *, int);
__private_extern__ int m_tag_copy_chain(struct mbuf *, struct mbuf *, int);
__private_extern__ void m_tag_init(struct mbuf *, int);
__private_extern__ struct  m_tag *m_tag_first(struct mbuf *);
__private_extern__ struct  m_tag *m_tag_next(struct mbuf *, struct m_tag *);

__private_extern__ void m_scratch_init(struct mbuf *);
__private_extern__ u_int32_t m_scratch_get(struct mbuf *, u_int8_t **);

__private_extern__ void m_classifier_init(struct mbuf *, uint32_t);

__private_extern__ int m_set_service_class(struct mbuf *, mbuf_svc_class_t);
__private_extern__ mbuf_svc_class_t m_get_service_class(struct mbuf *);
__private_extern__ mbuf_svc_class_t m_service_class_from_idx(u_int32_t);
__private_extern__ mbuf_svc_class_t m_service_class_from_val(u_int32_t);
__private_extern__ int m_set_traffic_class(struct mbuf *, mbuf_traffic_class_t);
__private_extern__ mbuf_traffic_class_t m_get_traffic_class(struct mbuf *);

#define ADDCARRY(_x)  do {                                              \
	while (((_x) >> 16) != 0)                                       \
	        (_x) = ((_x) >> 16) + ((_x) & 0xffff);                  \
} while (0)

__private_extern__ u_int16_t m_adj_sum16(struct mbuf *, u_int32_t,
    u_int32_t, u_int32_t, u_int32_t);
__private_extern__ u_int16_t m_sum16(struct mbuf *, u_int32_t, u_int32_t);

__private_extern__ void m_set_ext(struct mbuf *, struct ext_ref *,
    m_ext_free_func_t, caddr_t);
__private_extern__ struct ext_ref *m_get_rfa(struct mbuf *);
__private_extern__ m_ext_free_func_t m_get_ext_free(struct mbuf *);
__private_extern__ caddr_t m_get_ext_arg(struct mbuf *);

__private_extern__ void m_do_tx_compl_callback(struct mbuf *, struct ifnet *);
__private_extern__ mbuf_tx_compl_func m_get_tx_compl_callback(u_int32_t);

__END_DECLS
#endif                         
#endif                     
