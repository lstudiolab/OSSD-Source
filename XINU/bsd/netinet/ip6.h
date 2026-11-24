/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
 *	@(#)ip.h	8.1 (Berkeley) 6/10/93
 */

#ifndef _NETINET_IP6_H_
#define _NETINET_IP6_H_
#ifndef DRIVERKIT
#include <sys/appleapiopts.h>
#else
#include <sys/_types.h>
#include <netinet/in.h>
#include <machine/endian.h>
#endif                

  
                                              
           
   

struct ip6_hdr {
	union {
		struct ip6_hdrctl {
			u_int32_t ip6_un1_flow;                         
			u_int16_t ip6_un1_plen;                     
			u_int8_t  ip6_un1_nxt;                   
			u_int8_t  ip6_un1_hlim;                
		} ip6_un1;
		u_int8_t ip6_un2_vfc;                                         
	} ip6_ctlun;
	struct in6_addr ip6_src;                            
	struct in6_addr ip6_dst;                                 
} __attribute__((__packed__));

#define ip6_vfc         ip6_ctlun.ip6_un2_vfc
#define ip6_flow        ip6_ctlun.ip6_un1.ip6_un1_flow
#define ip6_plen        ip6_ctlun.ip6_un1.ip6_un1_plen
#define ip6_nxt         ip6_ctlun.ip6_un1.ip6_un1_nxt
#define ip6_hlim        ip6_ctlun.ip6_un1.ip6_un1_hlim
#define ip6_hops        ip6_ctlun.ip6_un1.ip6_un1_hlim

#define IPV6_VERSION            0x60
#define IPV6_VERSION_MASK       0xf0

#if BYTE_ORDER == BIG_ENDIAN
#define IPV6_FLOWINFO_MASK      0x0fffffff                               
#define IPV6_FLOWLABEL_MASK     0x000fffff                                
#define IPV6_FLOW_ECN_MASK      0x00300000                          
#else
#if BYTE_ORDER == LITTLE_ENDIAN
#define IPV6_FLOWINFO_MASK      0xffffff0f                               
#define IPV6_FLOWLABEL_MASK     0xffff0f00                                
#define IPV6_FLOW_ECN_MASK      0x00003000                          
#endif                    
#endif
#if 1
                                      
#define IP6TOS_CE               0x01                                
#define IP6TOS_ECT              0x02                               
#endif

  
                                                                       
   
#define IP6FLOW_DSCP_MASK       0x0fc00000
#define IP6FLOW_DSCP_SHIFT      22

  
                    
   

struct  ip6_ext {
	u_int8_t ip6e_nxt;
	u_int8_t ip6e_len;
} __attribute__((__packed__));

                               
                                                                    
struct ip6_hbh {
	u_int8_t ip6h_nxt;                       
	u_int8_t ip6h_len;                                       
	                         
} __attribute__((__packed__));

                                
                                                                    
struct ip6_dest {
	u_int8_t ip6d_nxt;                       
	u_int8_t ip6d_len;                                       
	                         
} __attribute__((__packed__));

                                     
#define IP6OPT_PAD1             0x00                    
#define IP6OPT_PADN             0x01                    
#define IP6OPT_JUMBO            0xC2                          
#define IP6OPT_NSAP_ADDR        0xC3                    
#define IP6OPT_TUNNEL_LIMIT     0x04                    
#ifndef KERNEL_PRIVATE
#define IP6OPT_RTALERT          0x05                                      
#endif
#define IP6OPT_ROUTER_ALERT     0x05                                           

#define IP6OPT_RTALERT_LEN      4
#define IP6OPT_RTALERT_MLD      0                                             
#define IP6OPT_RTALERT_RSVP     1                                              
#define IP6OPT_RTALERT_ACTNET   2                                            
#define IP6OPT_MINLEN           2

#define IP6OPT_EID              0x8a                    

#define IP6OPT_TYPE(o)          ((o) & 0xC0)
#define IP6OPT_TYPE_SKIP        0x00
#define IP6OPT_TYPE_DISCARD     0x40
#define IP6OPT_TYPE_FORCEICMP   0x80
#define IP6OPT_TYPE_ICMP        0xC0

#define IP6OPT_MUTABLE          0x20

                               
struct ip6_opt {
	u_int8_t ip6o_type;
	u_int8_t ip6o_len;
} __attribute__((__packed__));

                          
struct ip6_opt_jumbo {
	u_int8_t ip6oj_type;
	u_int8_t ip6oj_len;
	u_int8_t ip6oj_jumbo_len[4];
} __attribute__((__packed__));
#define IP6OPT_JUMBO_LEN        6

                         
struct ip6_opt_nsap {
	u_int8_t ip6on_type;
	u_int8_t ip6on_len;
	u_int8_t ip6on_src_nsap_len;
	u_int8_t ip6on_dst_nsap_len;
	                             
	                                  
}__attribute__((__packed__));

                         
struct ip6_opt_tunnel {
	u_int8_t ip6ot_type;
	u_int8_t ip6ot_len;
	u_int8_t ip6ot_encap_limit;
}__attribute__((__packed__));

                         
struct ip6_opt_router {
	u_int8_t ip6or_type;
	u_int8_t ip6or_len;
	u_int8_t ip6or_value[2];
}__attribute__((__packed__));
                                                 
#if BYTE_ORDER == BIG_ENDIAN
#define IP6_ALERT_MLD   0x0000
#define IP6_ALERT_RSVP  0x0001
#define IP6_ALERT_AN    0x0002
#else
#if BYTE_ORDER == LITTLE_ENDIAN
#define IP6_ALERT_MLD   0x0000
#define IP6_ALERT_RSVP  0x0100
#define IP6_ALERT_AN    0x0200
#endif                    
#endif

                    
struct ip6_rthdr {
	u_int8_t  ip6r_nxt;                      
	u_int8_t  ip6r_len;                                      
	u_int8_t  ip6r_type;                      
	u_int8_t  ip6r_segleft;                    
	                                            
} __attribute__((__packed__));

                                                    
struct ip6_rthdr0 {
	u_int8_t  ip6r0_nxt;                             
	u_int8_t  ip6r0_len;                                             
	u_int8_t  ip6r0_type;                            
	u_int8_t  ip6r0_segleft;                           
	u_int32_t  ip6r0_reserved;                          
	                                           
} __attribute__((__packed__));

                     
struct ip6_frag {
	u_int8_t  ip6f_nxt;                              
	u_int8_t  ip6f_reserved;                            
	u_int16_t ip6f_offlg;                                           
	u_int32_t ip6f_ident;                               
} __attribute__((__packed__));

#if BYTE_ORDER == BIG_ENDIAN
#define IP6F_OFF_MASK           0xfff8                                   
#define IP6F_RESERVED_MASK      0x0006                                   
#define IP6F_MORE_FRAG          0x0001                           
#else                                  
#define IP6F_OFF_MASK           0xf8ff                                   
#define IP6F_RESERVED_MASK      0x0600                                   
#define IP6F_MORE_FRAG          0x0100                           
#endif                                  

  
                                      
   
#define IPV6_MAXHLIM    255                           
#define IPV6_DEFHLIM    64                        
#define IPV6_FRAGTTL    60                                              
#define IPV6_HLIMDEC    1                                       

#define IPV6_MMTU       1280                                                
#define IPV6_MAXPACKET  65535                                                 
#define IPV6_MAXOPTHDR  2048                                                  

#ifdef BSD_KERNEL_PRIVATE
  
                                                                      
                                                              
                                                               
                                                                        
                                                                        
                                                             
   

#define IP6_EXTHDR_CHECK(m, off, hlen, action)                          \
do {                                                                    \
    if ((m)->m_next != NULL) {                                          \
	if (((m)->m_flags & M_LOOP) &&                                  \
	    ((m)->m_len < (off) + (hlen)) &&                            \
	    (((m) = m_pullup((m), (off) + (hlen))) == NULL)) {          \
	        ip6stat.ip6s_exthdrtoolong++;                           \
	        action;                                                 \
	} else if ((m)->m_flags & M_EXT) {                              \
	        if ((m)->m_len < (off) + (hlen)) {                      \
	                ip6stat.ip6s_exthdrtoolong++;                   \
	                m_freem(m);                                     \
	                (m) = NULL;                                     \
	                action;                                         \
	        }                                                       \
	} else {                                                        \
	        if ((m)->m_len < (off) + (hlen)) {                      \
	                ip6stat.ip6s_exthdrtoolong++;                   \
	                m_freem(m);                                     \
	                (m) = NULL;                                     \
	                action;                                         \
	        }                                                       \
	}                                                               \
    } else {                                                            \
	if ((m)->m_len < (off) + (hlen)) {                              \
	        ip6stat.ip6s_tooshort++;                                \
	        in6_ifstat_inc(m->m_pkthdr.rcvif, ifs6_in_truncated);   \
	        m_freem(m);                                             \
	        (m) = NULL;                                             \
	        action;                                                 \
	}                                                               \
    }                                                                   \
} while (0)

  
                                                                          
                                                                 
                                                                        
                   
                                                                            
                                                                  
   
#define IP6_EXTHDR_GET(val, typ, m, off, len)                           \
	M_STRUCT_GET(val, typ, m, off, len)

#define IP6_EXTHDR_GET0(val, typ, m, off, len)                          \
	M_STRUCT_GET0(val, typ, m, off, len)

#endif                         
#endif                       
