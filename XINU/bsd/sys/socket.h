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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/* Copyright (c) 1998, 1999 Apple Computer, Inc. All Rights Reserved */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1985, 1986, 1988, 1993, 1994
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
 *	@(#)socket.h	8.4 (Berkeley) 2/21/94
 * $FreeBSD: src/sys/sys/socket.h,v 1.39.2.7 2001/07/03 11:02:01 ume Exp $
 */
  
                                                                      
                                                                          
                                                                        
               
   

#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_

#include <sys/types.h>
#include <sys/cdefs.h>
#include <machine/_param.h>
#include <net/net_kev.h>

#ifdef PRIVATE
#include <sys/param.h>
#include <uuid/uuid.h>
#endif              

#ifndef KERNEL
#include <Availability.h>
#endif

  
                                                                    
   

  
              
   

#include <sys/_types/_gid_t.h>
#include <sys/_types/_off_t.h>
#include <sys/_types/_pid_t.h>
#include <sys/_types/_sa_family_t.h>
#include <sys/_types/_socklen_t.h>

                                                                        
#include <sys/_types/_size_t.h>

                                                                        
#include <sys/_types/_ssize_t.h>

  
                                                                          
   
#include <sys/_types/_iovec_t.h>

  
        
   
#define SOCK_STREAM     1                                  
#define SOCK_DGRAM      2                                    
#define SOCK_RAW        3                                           
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define SOCK_RDM        4                                               
#endif                                              
#define SOCK_SEQPACKET  5                                            

  
                           
   
#define SO_DEBUG        0x0001                                                
#define SO_ACCEPTCONN   0x0002                                       
#define SO_REUSEADDR    0x0004                                         
#define SO_KEEPALIVE    0x0008                                      
#define SO_DONTROUTE    0x0010                                            
#define SO_BROADCAST    0x0020                                                
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define SO_USELOOPBACK  0x0040                                             
#define SO_LINGER       0x0080                                                          
#else
#define SO_LINGER       0x1080                                                            
#endif                                              
#define SO_OOBINLINE    0x0100                                               
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define SO_REUSEPORT    0x0200                                                
#define SO_TIMESTAMP    0x0400                                                
#define SO_TIMESTAMP_MONOTONIC  0x0800                                                        
#ifndef __APPLE__
#define SO_ACCEPTFILTER 0x1000                                         
#else
#define SO_DONTTRUNC    0x2000                                         
                                                             
#define SO_WANTMORE     0x4000                                                     
#define SO_WANTOOBFLAG  0x8000                                                      

#ifdef PRIVATE
#define SO_NOWAKEFROMSLEEP      0x10000                                            
#define SO_NOAPNFALLBK          0x20000                                                
#define SO_TIMESTAMP_CONTINUOUS 0x40000                                                   
#endif

#endif                    
#endif                                              

  
                                              
   
#define SO_SNDBUF       0x1001                                
#define SO_RCVBUF       0x1002                                   
#define SO_SNDLOWAT     0x1003                                   
#define SO_RCVLOWAT     0x1004                                      
#define SO_SNDTIMEO     0x1005                            
#define SO_RCVTIMEO     0x1006                               
#define SO_ERROR        0x1007                                          
#define SO_TYPE         0x1008                               
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define SO_LABEL        0x1010                          
#define SO_PEERLABEL    0x1011                          
#ifdef __APPLE__
#define SO_NREAD        0x1020                                                
#define SO_NKE          0x1021                                               
#define SO_NOSIGPIPE    0x1022                                          
#define SO_NOADDRERR    0x1023                                                                              
#define SO_NWRITE       0x1024                                                                          
#define SO_REUSESHAREUID        0x1025                                                                      
#ifdef __APPLE_API_PRIVATE
#define SO_NOTIFYCONFLICT       0x1026                                                                                     
#define SO_UPCALLCLOSEWAIT      0x1027                                                     
#endif
#define SO_LINGER_SEC   0x1080                                                            
#ifdef PRIVATE
#define SO_RESTRICTIONS 0x1081                                    
#define  SO_RESTRICT_DENY_IN    0x1                                  
#define  SO_RESTRICT_DENY_OUT   0x2                                   
#define  SO_RESTRICT_DENY_CELLULAR 0x4                                       
#define  SO_RESTRICT_DENY_EXPENSIVE 0x8                                          
#define  SO_RESTRICT_DENY_CONSTRAINED 0x10                                          
#endif              
#define SO_RANDOMPORT   0x1082                                               
#define SO_NP_EXTENSIONS        0x1083                                       
#endif

#ifdef PRIVATE
#define SO_EXECPATH     0x1085                                                  

  
                                                         
  
               
                                                                
                                                                    
                                                                    
                            
  
           
                                                                         
                                                                       
                                                                         
                                    
  
           
                                                                     
                                          
  
           
                                                                     
                                                                       
                                                                     
                                     
  
            
                                                                       
                                                                      
                    
  
           
                                                                        
                                                                         
                                                       
  
           
                                                                      
                                                                        
                                          
  
           
                                                                     
                                                                      
                  
  
           
                                                                        
                                                                        
                  
  
            
                                                                      
                                                                    
                                                                         
       
   
#define SO_TRAFFIC_CLASS        0x1086                                   
#define SO_TC_BK_SYS    100                               
#define SO_TC_BK        200
#define SO_TC_BE        0
#define SO_TC_RD        300
#define SO_TC_OAM       400
#define SO_TC_AV        500
#define SO_TC_RV        600
#define SO_TC_VI        700
#define SO_TC_VO        800
#define SO_TC_CTL       900                                
#define SO_TC_MAX       10                                              
#ifdef XNU_KERNEL_PRIVATE
#define _SO_TC_BK       1                               
#define _SO_TC_VI       2                               
#define _SO_TC_VO       3                               
#define _SO_TC_MAX      4                               

#define SO_VALID_TC(c)                                                  \
	(c == SO_TC_BK_SYS || c == SO_TC_BK || c == SO_TC_BE ||         \
	c == SO_TC_RD || c == SO_TC_OAM || c == SO_TC_AV ||             \
	c == SO_TC_RV || c == SO_TC_VI || c == SO_TC_VO ||              \
	c == SO_TC_CTL || c == SO_TC_NETSVC_SIG)

#define SO_TC_UNSPEC    ((int)-1)                                                

#define SO_TC_SIG       SO_TC_VI                                       

#define SOTCIX_BK_SYS   0
#define SOTCIX_BK       1
#define SOTCIX_BE       2
#define SOTCIX_RD       3
#define SOTCIX_OAM      4
#define SOTCIX_AV       5
#define SOTCIX_RV       6
#define SOTCIX_VI       7
#define SOTCIX_VO       8
#define SOTCIX_CTL      9
#endif                         

                                            
#define TRAFFIC_MGT_SO_BACKGROUND       0x0001                         
#define TRAFFIC_MGT_TCP_RECVBG          0x0002                                             

#define SO_RECV_TRAFFIC_CLASS   0x1087                                            
#define SO_TRAFFIC_CLASS_DBG    0x1088                                                     
#define SO_OPTION_UNUSED_0      0x1089                                        
#define SO_PRIVILEGED_TRAFFIC_CLASS 0x1090                                           
#define SO_DEFUNCTIT    0x1091                                                          
#define SO_DEFUNCTOK    0x1100                                
#define SO_ISDEFUNCT    0x1101                                  

#define SO_OPPORTUNISTIC        0x1102                                        

  
                                                                          
                                                                            
                                  
   
#define SO_FLUSH        0x1103                                       
#define  SO_TC_ALL      (-1)

#define SO_RECV_ANYIF   0x1104                                               
#define SO_TRAFFIC_MGT_BACKGROUND       0x1105                                     

#define SO_FLOW_DIVERT_TOKEN    0x1106                         

#define SO_DELEGATED            0x1107                                      
#define SO_DELEGATED_UUID       0x1108                                       
#define SO_NECP_ATTRIBUTES      0x1109                                                      
#define SO_CFIL_SOCK_ID         0x1110                                                     
#define SO_NECP_CLIENTUUID      0x1111                        
#endif              
#define SO_NUMRCVPKT            0x1112                                                    
#ifdef PRIVATE
#define SO_AWDL_UNRESTRICTED    0x1113                                          
#define SO_EXTENDED_BK_IDLE     0x1114                                                                      
#define SO_MARK_CELLFALLBACK    0x1115                                          
#endif              
#define SO_NET_SERVICE_TYPE     0x1116                            

#ifdef PRIVATE
#define SO_QOSMARKING_POLICY_OVERRIDE   0x1117           
#define SO_INTCOPROC_ALLOW              0x1118                                                    
#endif              

#define SO_NETSVC_MARKING_LEVEL    0x1119                                            

#ifdef PRIVATE
#define SO_NECP_LISTENUUID         0x1120                                     
#define SO_MPKL_SEND_INFO          0x1122                                  
#define SO_STATISTICS_EVENT        0x1123                                                         
#define SO_WANT_KEV_SOCKET_CLOSED  0x1124                                                
#endif              
  
                                                      
  
                                                                          
                                                                            
                                                                            
  
                                                                        
                                                             
                                                                           
                                                                          
                                                                           
                                                                          
                                                                    
                                                           
  
                                                             
                                                                
                                                                         
                                                                   
                                                                  
                                                      
  
                                                                          
                                                                        
                                                                             
                                                                             
                           
  
                                                                            
                                                                             
           
  
                                                                          
                                                                                
                                                                            
                                            
  
                      
                                                                     
                                               
  
                      
                                                                  
                                                                         
                           
  
                      
                                                                     
                                                                      
                                                                     
                                     
  
                       
                                                                       
                                                                      
                    
  
                      
                                                                        
                                                                       
                                                          
  
                      
                                                                      
                                                                        
                       
  
                      
                                                                     
                                                                     
                   
  
                       
                                                                      
                                                                      
  
                      
                                                                        
                                                             
             
   

#define NET_SERVICE_TYPE_BE     0                  
#define NET_SERVICE_TYPE_BK     1                                  
#define NET_SERVICE_TYPE_SIG    2                
#define NET_SERVICE_TYPE_VI     3                        
#define NET_SERVICE_TYPE_VO     4                        
#define NET_SERVICE_TYPE_RV     5                                        
#define NET_SERVICE_TYPE_AV     6                                       
#define NET_SERVICE_TYPE_OAM    7                                                 
#define NET_SERVICE_TYPE_RD     8                      

#if PRIVATE
#define _NET_SERVICE_TYPE_COUNT 9
#define _NET_SERVICE_TYPE_UNSPEC        ((int)-1)

#define IS_VALID_NET_SERVICE_TYPE(c)                            \
	(c >= NET_SERVICE_TYPE_BE && c <= NET_SERVICE_TYPE_RD)

extern const int sotc_by_netservicetype[_NET_SERVICE_TYPE_COUNT];

  
                                                                      
                                                                          
                                             
   
#define SO_TC_NET_SERVICE_OFFSET        10000
#define SO_TC_NETSVC_SIG        (SO_TC_NET_SERVICE_OFFSET + NET_SERVICE_TYPE_SIG)
#endif              

                                                            
#define NETSVC_MRKNG_UNKNOWN            0                                                        
#define NETSVC_MRKNG_LVL_L2             1                                                               
#define NETSVC_MRKNG_LVL_L3L2_ALL       2                                                                                   
#define NETSVC_MRKNG_LVL_L3L2_BK        3                                                                           
                                                                                  


typedef __uint32_t sae_associd_t;
#define SAE_ASSOCID_ANY 0
#define SAE_ASSOCID_ALL ((sae_associd_t)(-1ULL))

typedef __uint32_t sae_connid_t;
#define SAE_CONNID_ANY  0
#define SAE_CONNID_ALL  ((sae_connid_t)(-1ULL))

                                
#define CONNECT_RESUME_ON_READ_WRITE    0x1                                     
#define CONNECT_DATA_IDEMPOTENT         0x2                         
#define CONNECT_DATA_AUTHENTICATED      0x4                                                          

                        
typedef struct sa_endpoints {
	unsigned int            sae_srcif;                                     
	const struct sockaddr   *sae_srcaddr;                                
	socklen_t               sae_srcaddrlen;                             
	const struct sockaddr   *sae_dstaddr;                            
	socklen_t               sae_dstaddrlen;                                  
} sa_endpoints_t;
#endif                                              

  
                                                 
   
struct  linger {
	int     l_onoff;                                   
	int     l_linger;                                
};

#ifndef __APPLE__
struct  accept_filter_arg {
	char    af_name[16];
	char    af_arg[256 - 16];
};
#endif

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#ifdef __APPLE__

  
                                                               
   
struct so_np_extensions {
	u_int32_t       npx_flags;
	u_int32_t       npx_mask;
};

#define SONPX_SETOPTSHUT        0x000000001                                                      


#ifdef KERNEL_PRIVATE
#define SONPX_MASK_VALID                (SONPX_SETOPTSHUT)
#define IS_SO_TC_BACKGROUND(_tc_) ((_tc_) == SO_TC_BK || (_tc_) == SO_TC_BK_SYS)
#define IS_SO_TC_BACKGROUNDSYSTEM(_tc_) ((_tc_) == SO_TC_BK_SYS)
#endif                     

#endif
#endif

  
                                                                 
   
#define SOL_SOCKET      0xffff                                        


  
                    
   
#define AF_UNSPEC       0                                
#define AF_UNIX         1                                          
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define AF_LOCAL        AF_UNIX                                     
#endif                                              
#define AF_INET         2                                                 
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define AF_IMPLINK      3                                          
#define AF_PUP          4                                            
#define AF_CHAOS        5                                        
#define AF_NS           6                                       
#define AF_ISO          7                                  
#define AF_OSI          AF_ISO
#define AF_ECMA         8                                                    
#define AF_DATAKIT      9                                      
#define AF_CCITT        10                                             
#define AF_SNA          11                           
#define AF_DECnet       12                          
#define AF_DLI          13                                                  
#define AF_LAT          14                       
#define AF_HYLINK       15                                    
#define AF_APPLETALK    16                              
#define AF_ROUTE        17                                             
#define AF_LINK         18                                        
#define pseudo_AF_XTP   19                                                     
#define AF_COIP         20                                                     
#define AF_CNT          21                                               
#define pseudo_AF_RTIP  22                                              
#define AF_IPX          23                                            
#define AF_SIP          24                                            
#define pseudo_AF_PIP   25                                             
#define AF_NDRV         27                                               
#define AF_ISDN         28                                                       
#define AF_E164         AF_ISDN                                         
#define pseudo_AF_KEY   29                                                    
#endif                                              
#define AF_INET6        30                        
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define AF_NATM         31                                     
#define AF_SYSTEM       32                                         
#define AF_NETBIOS      33                           
#define AF_PPP          34                                              
#define pseudo_AF_HDRCMPLT 35                                                
                                                                   
#ifdef PRIVATE
#define AF_AFP  36                                       
#else
#define AF_RESERVED_36  36                                               
#endif
#define AF_IEEE80211    37                                        
#define AF_UTUN         38
#ifdef PRIVATE
#define AF_MULTIPATH    39
#endif              
#define AF_VSOCK        40                              
#define AF_MAX          41
#endif                                              

  
                                                          
   
struct sockaddr {
	__uint8_t       sa_len;                           
	sa_family_t     sa_family;                                
	char            sa_data[14];                                            
};

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define SOCK_MAXADDRLEN 255                                             

  
                                            
                              
   
struct sockproto {
	__uint16_t      sp_family;                                  
	__uint16_t      sp_protocol;                          
};
#endif                                              

  
                                                                  
   
#define _SS_MAXSIZE     128
#define _SS_ALIGNSIZE   (sizeof(__int64_t))
#define _SS_PAD1SIZE    \
	        (_SS_ALIGNSIZE - sizeof(__uint8_t) - sizeof(sa_family_t))
#define _SS_PAD2SIZE    \
	        (_SS_MAXSIZE - sizeof(__uint8_t) - sizeof(sa_family_t) - \
	                        _SS_PAD1SIZE - _SS_ALIGNSIZE)

  
                         
   
struct sockaddr_storage {
	__uint8_t       ss_len;                             
	sa_family_t     ss_family;                                
	char                    __ss_pad1[_SS_PAD1SIZE];
	__int64_t       __ss_align;                                            
	char                    __ss_pad2[_SS_PAD2SIZE];
};

  
                                                       
   
#define PF_UNSPEC       AF_UNSPEC
#define PF_LOCAL        AF_LOCAL
#define PF_UNIX         PF_LOCAL                                    
#define PF_INET         AF_INET
#define PF_IMPLINK      AF_IMPLINK
#define PF_PUP          AF_PUP
#define PF_CHAOS        AF_CHAOS
#define PF_NS           AF_NS
#define PF_ISO          AF_ISO
#define PF_OSI          AF_ISO
#define PF_ECMA         AF_ECMA
#define PF_DATAKIT      AF_DATAKIT
#define PF_CCITT        AF_CCITT
#define PF_SNA          AF_SNA
#define PF_DECnet       AF_DECnet
#define PF_DLI          AF_DLI
#define PF_LAT          AF_LAT
#define PF_HYLINK       AF_HYLINK
#define PF_APPLETALK    AF_APPLETALK
#define PF_ROUTE        AF_ROUTE
#define PF_LINK         AF_LINK
#define PF_XTP          pseudo_AF_XTP                                        
#define PF_COIP         AF_COIP
#define PF_CNT          AF_CNT
#define PF_SIP          AF_SIP
#define PF_IPX          AF_IPX                                    
#define PF_RTIP         pseudo_AF_RTIP                              
#define PF_PIP          pseudo_AF_PIP
#define PF_NDRV         AF_NDRV
#define PF_ISDN         AF_ISDN
#define PF_KEY          pseudo_AF_KEY
#define PF_INET6        AF_INET6
#define PF_NATM         AF_NATM
#define PF_SYSTEM       AF_SYSTEM
#define PF_NETBIOS      AF_NETBIOS
#define PF_PPP          AF_PPP
#ifdef PRIVATE
#define PF_AFP          AF_AFP
#else
#define PF_RESERVED_36  AF_RESERVED_36
#endif
#define PF_UTUN         AF_UTUN
#ifdef PRIVATE
#define PF_MULTIPATH    AF_MULTIPATH
#endif              
#define PF_VSOCK        AF_VSOCK
#define PF_MAX          AF_MAX

  
                                          
   
#define PF_VLAN         ((uint32_t)0x766c616e)              
#define PF_BOND         ((uint32_t)0x626f6e64)              
#ifdef KERNEL_PRIVATE
#define PF_BRIDGE       ((uint32_t)0x62726467)              
#define PF_802154       ((uint32_t)0x38313534)              
#endif                     

  
                                                   
  
                                   
                                  
  
                                                               
   
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define NET_MAXID       AF_MAX
#endif                                             

#ifdef KERNEL_PRIVATE
#define CTL_NET_NAMES { \
	{ 0, 0 }, \
	{ "local", CTLTYPE_NODE }, \
	{ "inet", CTLTYPE_NODE }, \
	{ "implink", CTLTYPE_NODE }, \
	{ "pup", CTLTYPE_NODE }, \
	{ "chaos", CTLTYPE_NODE }, \
	{ "xerox_ns", CTLTYPE_NODE }, \
	{ "iso", CTLTYPE_NODE }, \
	{ "emca", CTLTYPE_NODE }, \
	{ "datakit", CTLTYPE_NODE }, \
	{ "ccitt", CTLTYPE_NODE }, \
	{ "ibm_sna", CTLTYPE_NODE }, \
	{ "decnet", CTLTYPE_NODE }, \
	{ "dec_dli", CTLTYPE_NODE }, \
	{ "lat", CTLTYPE_NODE }, \
	{ "hylink", CTLTYPE_NODE }, \
	{ "appletalk", CTLTYPE_NODE }, \
	{ "route", CTLTYPE_NODE }, \
	{ "link_layer", CTLTYPE_NODE }, \
	{ "xtp", CTLTYPE_NODE }, \
	{ "coip", CTLTYPE_NODE }, \
	{ "cnt", CTLTYPE_NODE }, \
	{ "rtip", CTLTYPE_NODE }, \
	{ "ipx", CTLTYPE_NODE }, \
	{ "sip", CTLTYPE_NODE }, \
	{ "pip", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ "ndrv", CTLTYPE_NODE }, \
	{ "isdn", CTLTYPE_NODE }, \
	{ "key", CTLTYPE_NODE }, \
	{ "inet6", CTLTYPE_NODE }, \
	{ "natm", CTLTYPE_NODE }, \
	{ "sys", CTLTYPE_NODE }, \
	{ "netbios", CTLTYPE_NODE }, \
	{ "ppp", CTLTYPE_NODE }, \
	{ "hdrcomplete", CTLTYPE_NODE }, \
	{ "vsock", CTLTYPE_NODE }, \
}
#endif                     

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
  
                           
  
                                       
                                        
                                     
                                               
   
#define NET_RT_DUMP             1                                    
#define NET_RT_FLAGS            2                                     
#define NET_RT_IFLIST           3                                  
#define NET_RT_STAT             4                               
#define NET_RT_TRASH            5                                              
#define NET_RT_IFLIST2          6                                          
#define NET_RT_DUMP2            7                                    
#ifdef PRIVATE
#define NET_RT_DUMPX            8                    
#define NET_RT_DUMPX_FLAGS      9                    
#endif              
  
                                                  
                                                 
   
#define NET_RT_FLAGS_PRIV       10
#define NET_RT_MAXID            11
#endif                                             

#ifdef PRIVATE
                                                        
#define SO_STATISTICS_EVENT_ENTER_CELLFALLBACK (1 << 0)
#define SO_STATISTICS_EVENT_EXIT_CELLFALLBACK  (1 << 1)
#define SO_STATISTICS_EVENT_RESERVED_1         (1 << 2)
#define SO_STATISTICS_EVENT_RESERVED_2         (1 << 3)
#endif              


#ifdef KERNEL_PRIVATE
#define CTL_NET_RT_NAMES { \
	{ 0, 0 }, \
	{ "dump", CTLTYPE_STRUCT }, \
	{ "flags", CTLTYPE_STRUCT }, \
	{ "iflist", CTLTYPE_STRUCT }, \
	{ "stat", CTLTYPE_STRUCT }, \
	{ "trash", CTLTYPE_INT }, \
	{ "iflist2", CTLTYPE_STRUCT }, \
	{ "dump2", CTLTYPE_STRUCT }, \
	{ "dumpx", CTLTYPE_STRUCT }, \
	{ "dumpx_flags", CTLTYPE_STRUCT }, \
}

#endif                     

  
                                              
   
#define SOMAXCONN       128

  
                                                      
                                                         
   
struct msghdr {
	void            *msg_name;                                  
	socklen_t       msg_namelen;                               
	struct          iovec *msg_iov;                                 
	int             msg_iovlen;                                      
	void            *msg_control;                                        
	socklen_t       msg_controllen;                                      
	int             msg_flags;                                           
};

#ifdef PRIVATE
  
                                                         
  
                                                                       
               
  
                                                                          
                                                                       
   
struct msghdr_x {
	void            *msg_name;                            
	socklen_t       msg_namelen;                         
	struct iovec    *msg_iov;                                 
	int             msg_iovlen;                                
	void            *msg_control;                                  
	socklen_t       msg_controllen;                                
	int             msg_flags;                                     
	size_t          msg_datalen;                                          
};
#endif              

#ifdef XNU_KERNEL_PRIVATE
  
                                                   
                                                
                                                  
   

struct user_msghdr {
	user_addr_t     msg_name;                                     
	socklen_t       msg_namelen;                                 
	user_addr_t     msg_iov;                                          
	int             msg_iovlen;                                        
	user_addr_t     msg_control;                                           
	socklen_t       msg_controllen;                                        
	int             msg_flags;                                             
};

  
                                      
                                            
   

struct user64_msghdr {
	user64_addr_t   msg_name;                                     
	socklen_t       msg_namelen;                                 
	user64_addr_t   msg_iov;                                          
	int             msg_iovlen;                                        
	user64_addr_t   msg_control;                                           
	socklen_t       msg_controllen;                                        
	int             msg_flags;                                             
};

  
                                       
                                            
   

struct user32_msghdr {
	user32_addr_t   msg_name;                             
	socklen_t       msg_namelen;                         
	user32_addr_t   msg_iov;                                  
	int             msg_iovlen;                                
	user32_addr_t   msg_control;                                   
	socklen_t       msg_controllen;                                
	int             msg_flags;                                     
};

  
                                                     
                                                
                                                  
   

struct user_msghdr_x {
	user_addr_t     msg_name;                             
	socklen_t       msg_namelen;                         
	user_addr_t     msg_iov;                                  
	int             msg_iovlen;                                
	user_addr_t     msg_control;                                   
	socklen_t       msg_controllen;                                
	int             msg_flags;                                     
	size_t          msg_datalen;                                          
};

  
                                       
                                              
   

struct user64_msghdr_x {
	user64_addr_t   msg_name;                             
	socklen_t       msg_namelen;                         
	user64_addr_t   msg_iov;                                  
	int             msg_iovlen;                                
	user64_addr_t   msg_control;                                   
	socklen_t       msg_controllen;                                
	int             msg_flags;                                     
	user64_size_t   msg_datalen;                                          
};

  
                                        
                                              
   

struct user32_msghdr_x {
	user32_addr_t   msg_name;                             
	socklen_t       msg_namelen;                         
	user32_addr_t   msg_iov;                                  
	int             msg_iovlen;                                
	user32_addr_t   msg_control;                                   
	socklen_t       msg_controllen;                                
	int             msg_flags;                                     
	user32_size_t   msg_datalen;                                          
};

  
                                                         
                                                
                                                  
   

struct user_sa_endpoints {
	unsigned int    sae_srcif;                                     
	user_addr_t     sae_srcaddr;                                 
	socklen_t       sae_srcaddrlen;                             
	user_addr_t     sae_dstaddr;                             
	socklen_t       sae_dstaddrlen;                                  
};

  
                                           
                                                  
   

struct user64_sa_endpoints {
	unsigned int    sae_srcif;                                     
	user64_addr_t   sae_srcaddr;                                 
	socklen_t       sae_srcaddrlen;                             
	user64_addr_t   sae_dstaddr;                             
	socklen_t       sae_dstaddrlen;                                  
};

  
                                            
                                                  
   

struct user32_sa_endpoints {
	unsigned int    sae_srcif;                                     
	user32_addr_t   sae_srcaddr;                                 
	socklen_t       sae_srcaddrlen;                             
	user32_addr_t   sae_dstaddr;                             
	socklen_t       sae_dstaddrlen;                                  
};

#endif                         

#define MSG_OOB         0x1                                           
#define MSG_PEEK        0x2                                           
#define MSG_DONTROUTE   0x4                                                    
#define MSG_EOR         0x8                                        
#define MSG_TRUNC       0x10                                                
#define MSG_CTRUNC      0x20                                                   
#define MSG_WAITALL     0x40                                                
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define MSG_DONTWAIT    0x80                                                    
#define MSG_EOF         0x100                                          
#ifdef __APPLE__
#ifndef PRIVATE
#ifdef __APPLE_API_OBSOLETE
#define MSG_WAITSTREAM  0x200                                                             
#endif
#else
#define MSG_WAITSTREAM  0x200                                                             
#endif
#define MSG_FLUSH       0x400                                                              
#define MSG_HOLD        0x800                                                 
#define MSG_SEND        0x1000                                                      
#define MSG_HAVEMORE    0x2000                                     
#define MSG_RCVMORE     0x4000                                           
#endif
#ifdef KERNEL_PRIVATE
#define MSG_COMPAT      0x8000                          
#endif                     
#define MSG_NEEDSA      0x10000                                                                 
#ifdef KERNEL_PRIVATE
#define MSG_NBIO        0x20000                                           
#define MSG_SKIPCFIL    0x40000                                       
#endif
#endif                                              

#if __DARWIN_C_LEVEL >= 200809L
#define MSG_NOSIGNAL    0x80000                                             
#endif                       

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#ifdef  KERNEL
#define MSG_USEUPCALL   0x80000000                                         
#endif
#endif                                              

  
                                                           
                                                        
                                                      
                                                    
   
struct cmsghdr {
	socklen_t       cmsg_len;                                                 
	int             cmsg_level;                                     
	int             cmsg_type;                                        
                                             
};

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#ifndef __APPLE__
  
                                                                    
                                                                   
        
   
#define CMGROUP_MAX 16

  
                                                               
                                                               
                                                              
                                                                  
                         
   
struct cmsgcred {
	pid_t   cmcred_pid;                                         
	uid_t   cmcred_uid;                                              
	uid_t   cmcred_euid;                                                  
	gid_t   cmcred_gid;                                              
	short   cmcred_ngroups;                               
	gid_t   cmcred_groups[CMGROUP_MAX];                 
};
#endif
#endif                                              

                                                             
#define CMSG_DATA(cmsg)         ((unsigned char *)(cmsg) + \
	__DARWIN_ALIGN32(sizeof(struct cmsghdr)))

  
                                                                             
                                  
   
#define CMSG_FIRSTHDR(mhdr) \
	((mhdr)->msg_controllen >= sizeof(struct cmsghdr) ? \
	    (struct cmsghdr *)(mhdr)->msg_control : \
	    (struct cmsghdr *)0L)


  
                                                                  
                                                                                  
   
#define CMSG_NXTHDR(mhdr, cmsg)                                         \
	((char *)(cmsg) == (char *)0L ? CMSG_FIRSTHDR(mhdr) :           \
	    ((((unsigned char *)(cmsg) +                                \
	    __DARWIN_ALIGN32((__uint32_t)(cmsg)->cmsg_len) +            \
	    __DARWIN_ALIGN32(sizeof(struct cmsghdr))) >                 \
	    ((unsigned char *)(mhdr)->msg_control +                     \
	    (mhdr)->msg_controllen)) ?                                  \
	        (struct cmsghdr *)0L            :                       \
	        (struct cmsghdr *)(void *)((unsigned char *)(cmsg) +    \
	            __DARWIN_ALIGN32((__uint32_t)(cmsg)->cmsg_len))))

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
                        
#define CMSG_SPACE(l)           (__DARWIN_ALIGN32(sizeof(struct cmsghdr)) + __DARWIN_ALIGN32(l))
#define CMSG_LEN(l)             (__DARWIN_ALIGN32(sizeof(struct cmsghdr)) + (l))

#ifdef KERNEL
#define CMSG_ALIGN(n)   ((typeof(n))__DARWIN_ALIGN32(n))
#endif
#endif                                              

                                           
#define SCM_RIGHTS                      0x01                                      
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define SCM_TIMESTAMP                   0x02                                    
#define SCM_CREDS                       0x03                                         
#define SCM_TIMESTAMP_MONOTONIC         0x04                              

#ifdef PRIVATE
#define SCM_TIMESTAMP_CONTINUOUS        0x07                              
#define SCM_MPKL_SEND_INFO              0x08                                                                             
#define SCM_MPKL_RECV_INFO              0x09                                                                               
#endif              

#ifdef KERNEL_PRIVATE
  
                                   
   
struct osockaddr {
	__uint16_t      sa_family;                          
	char    sa_data[14];                                                  
};

  
                                         
   
struct omsghdr {
	void            *msg_name;                                    
	socklen_t       msg_namelen;                                 
	struct  iovec   *msg_iov;                                         
	int             msg_iovlen;                                        
	void            *msg_accrights;                                      
	int             msg_accrightslen;
};

#define SA(s)   ((struct sockaddr *)(void *)(s))
#endif                     
#endif                                              

  
                                                          
   
#define SHUT_RD         0                                               
#define SHUT_WR         1                                               
#define SHUT_RDWR       2                                         

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
  
                                    
   
struct sf_hdtr {
	struct iovec *headers;                                                    
	int hdr_cnt;                                          
	struct iovec *trailers;                                                    
	int trl_cnt;                                           
};

#ifdef KERNEL

                              
struct user_sf_hdtr {
	user_addr_t headers;                                                      
	int hdr_cnt;                                          
	user_addr_t trailers;                                                      
	int trl_cnt;                                           
};

                                         
struct user64_sf_hdtr {
	user64_addr_t headers;                                                    
	int hdr_cnt;                                          
	user64_addr_t trailers;                                                    
	int trl_cnt;                                           
};

                                          
struct user32_sf_hdtr {
	user32_addr_t headers;                                                    
	int hdr_cnt;                                          
	user32_addr_t trailers;                                                    
	int trl_cnt;                                           
};

#endif             

#endif                                              

#ifdef PRIVATE
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)

  
                              
   
struct so_aidreq {
	__uint32_t      sar_cnt;                                    
	sae_associd_t   *sar_aidp;                                    
};

#ifdef BSD_KERNEL_PRIVATE
struct so_aidreq32 {
	__uint32_t      sar_cnt;
	user32_addr_t   sar_aidp;
};

struct so_aidreq64 {
	__uint32_t      sar_cnt;
	user64_addr_t   sar_aidp __attribute__((aligned(8)));
};
#endif                         

  
                             
   
struct so_cidreq {
	sae_associd_t   scr_aid;                            
	__uint32_t      scr_cnt;                                   
	sae_connid_t    *scr_cidp;                                   
};

#ifdef BSD_KERNEL_PRIVATE
struct so_cidreq32 {
	sae_associd_t   scr_aid;
	__uint32_t      scr_cnt;
	user32_addr_t   scr_cidp;
};

struct so_cidreq64 {
	sae_associd_t   scr_aid;
	__uint32_t      scr_cnt;
	user64_addr_t   scr_cidp __attribute__((aligned(8)));
};
#endif                         

  
                              
   
struct so_cinforeq {
	sae_connid_t    scir_cid;                                  
	__uint32_t      scir_flags;                                  
	__uint32_t      scir_ifindex;                                          
	__int32_t       scir_error;                                    
	struct sockaddr *scir_src;                                  
	socklen_t       scir_src_len;                                   
	struct sockaddr *scir_dst;                                       
	socklen_t       scir_dst_len;                                        
	__uint32_t      scir_aux_type;                                     
	void            *scir_aux_data;                       
	__uint32_t      scir_aux_len;                             
};

#ifdef BSD_KERNEL_PRIVATE
struct so_cinforeq32 {
	sae_connid_t    scir_cid;
	__uint32_t      scir_flags;
	__uint32_t      scir_ifindex;
	__int32_t       scir_error;
	user32_addr_t   scir_src;
	socklen_t       scir_src_len;
	user32_addr_t   scir_dst;
	socklen_t       scir_dst_len;
	__uint32_t      scir_aux_type;
	user32_addr_t   scir_aux_data;
	__uint32_t      scir_aux_len;
};

struct so_cinforeq64 {
	sae_connid_t    scir_cid;
	__uint32_t      scir_flags;
	__uint32_t      scir_ifindex;
	__int32_t       scir_error;
	user64_addr_t   scir_src        __attribute__((aligned(8)));
	socklen_t       scir_src_len;
	user64_addr_t   scir_dst        __attribute__((aligned(8)));
	socklen_t       scir_dst_len;
	__uint32_t      scir_aux_type;
	user64_addr_t   scir_aux_data   __attribute__((aligned(8)));
	__uint32_t      scir_aux_len;
};
#endif                         

                                 
#define CIF_CONNECTING          0x1                                   
#define CIF_CONNECTED           0x2                                    
#define CIF_DISCONNECTING       0x4                                      
#define CIF_DISCONNECTED        0x8                                
#define CIF_BOUND_IF            0x10                               
#define CIF_BOUND_IP            0x20                                
#define CIF_BOUND_PORT          0x40                             
#define CIF_PREFERRED           0x80                                         
#define CIF_MP_CAPABLE          0x100                                    
#define CIF_MP_READY            0x200                                     
#define CIF_MP_DEGRADED         0x400                                          
#define CIF_MP_ACTIVE           0x800                                   

                                                
#define CIAUX_TCP       0x1                                              
#define CIAUX_MPTCP     0x2                                                       

  
                                   
   
struct so_cordreq {
	sae_connid_t    sco_cid;                                   
	__uint32_t      sco_rank;                                               
};

  
                                              
   
struct netpolicy_event_data {
	__uint64_t      eupid;                                    
	__uint64_t      epid;                              
	uuid_t          euuid;                              
};

  
                                     
   
struct kev_netpolicy_ifdenied {
	struct netpolicy_event_data     ev_data;
	__uint32_t ev_if_functional_type;
};

  
                                          
   
struct kev_netpolicy_netdenied {
	struct netpolicy_event_data     ev_data;
	__uint32_t ev_network_type;
};

  
                                                      
   
struct netsvctype_dscp_map {
	int             netsvctype;
	u_int8_t        dscp;                                 
};

  
                                                          
   
struct so_mpkl_send_info {
	uuid_t          mpkl_uuid;
	__uint8_t       mpkl_proto;                                        
};

struct so_mpkl_recv_info {
	__uint32_t      mpkl_seq;
	__uint8_t       mpkl_proto;                                        
};

#ifndef KERNEL
__BEGIN_DECLS

extern int peeloff(int s, sae_associd_t);
extern int socket_delegate(int, int, int, pid_t);

  
                                                                
                                                                    
  
                                                                                
                                                                       
  
                                                               
  
                                                              
  
                                                            
  
                                                                             
                                                                               
                                                                               
  
                                                                             
                                                                              
                                                                      
                                                          
  
                                                                          
                                                                          
  
                                                                       
                              
  
                                                                  
   
ssize_t recvmsg_x(int s, const struct msghdr_x *msgp, u_int cnt, int flags);

  
                                                          
                                                                    
  
                                                                                
                                                                       
  
                                                               
  
                                                              
  
                                                            
  
                                                                       
                                                                              
  
                                                                            
                                       
  
                                                                       
                                
                                                                   
  
                                                                        
  
                                                                   
                              
  
                                                                  
   
ssize_t sendmsg_x(int s, const struct msghdr_x *msgp, u_int cnt, int flags);
__END_DECLS
#endif              
#endif                                              
#endif              

#ifndef KERNEL
__BEGIN_DECLS

int     accept(int, struct sockaddr * __restrict, socklen_t * __restrict)
__DARWIN_ALIAS_C(accept);
int     bind(int, const struct sockaddr *, socklen_t) __DARWIN_ALIAS(bind);
int     connect(int, const struct sockaddr *, socklen_t) __DARWIN_ALIAS_C(connect);
int     getpeername(int, struct sockaddr * __restrict, socklen_t * __restrict)
__DARWIN_ALIAS(getpeername);
int     getsockname(int, struct sockaddr * __restrict, socklen_t * __restrict)
__DARWIN_ALIAS(getsockname);
int     getsockopt(int, int, int, void * __restrict, socklen_t * __restrict);
int     listen(int, int) __DARWIN_ALIAS(listen);
ssize_t recv(int, void *, size_t, int) __DARWIN_ALIAS_C(recv);
ssize_t recvfrom(int, void *, size_t, int, struct sockaddr * __restrict,
    socklen_t * __restrict) __DARWIN_ALIAS_C(recvfrom);
ssize_t recvmsg(int, struct msghdr *, int) __DARWIN_ALIAS_C(recvmsg);
ssize_t send(int, const void *, size_t, int) __DARWIN_ALIAS_C(send);
ssize_t sendmsg(int, const struct msghdr *, int) __DARWIN_ALIAS_C(sendmsg);
ssize_t sendto(int, const void *, size_t,
    int, const struct sockaddr *, socklen_t) __DARWIN_ALIAS_C(sendto);
int     setsockopt(int, int, int, const void *, socklen_t);
int     shutdown(int, int);
int     sockatmark(int) __OSX_AVAILABLE_STARTING(__MAC_10_5, __IPHONE_2_0);
int     socket(int, int, int);
int     socketpair(int, int, int, int *) __DARWIN_ALIAS(socketpair);

#if !defined(_POSIX_C_SOURCE)
int     sendfile(int, int, off_t, off_t *, struct sf_hdtr *, int);
#endif                        

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
void    pfctlinput(int, struct sockaddr *);

__API_AVAILABLE(macosx(10.11), ios(9.0), tvos(9.0), watchos(2.0))
int connectx(int, const sa_endpoints_t *, sae_associd_t, unsigned int,
    const struct iovec *, unsigned int, size_t *, sae_connid_t *);

__API_AVAILABLE(macosx(10.11), ios(9.0), tvos(9.0), watchos(2.0))
int disconnectx(int, sae_associd_t, sae_connid_t);
#endif                                              
__END_DECLS
#endif              

#ifdef KERNEL
#include <sys/kpi_socket.h>
#endif

#endif                      
