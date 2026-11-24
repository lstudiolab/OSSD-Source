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
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *	@(#)in.h	8.3 (Berkeley) 1/3/94
 * $FreeBSD: src/sys/netinet/in.h,v 1.48.2.2 2001/04/21 14:53:06 ume Exp $
 */

#ifndef _NETINET_IN_H_
#define _NETINET_IN_H_

#ifndef DRIVERKIT
#include <sys/appleapiopts.h>
#include <stdint.h>                                  

#ifndef KERNEL
#include <Availability.h>
#endif

#else
#include <sys/_types.h>
#include <sys/_types/_sa_family_t.h>
#endif                

#include <sys/_types/_in_addr_t.h>
#include <sys/_types/_in_port_t.h>

#ifndef DRIVERKIT
  
                    
                                                                    
                                                  
   
#include <sys/socket.h>
#endif                

  
                                                                  
   
#include <machine/endian.h>
#include <sys/_endian.h>

  
                                                           
                                                       
   

  
                       
   
#define IPPROTO_IP              0                                 
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define IPPROTO_HOPOPTS 0                                           
#endif                                              
#define IPPROTO_ICMP            1                                             
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define IPPROTO_IGMP            2                                        
#define IPPROTO_GGP             3                                           
#define IPPROTO_IPV4            4                                       
#define IPPROTO_IPIP            IPPROTO_IPV4                           
#endif                                              
#define IPPROTO_TCP             6                        
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define IPPROTO_ST              7                                       
#define IPPROTO_EGP             8                                              
#define IPPROTO_PIGP            9                                             
#define IPPROTO_RCCMON          10                                      
#define IPPROTO_NVPII           11                                         
#define IPPROTO_PUP             12                       
#define IPPROTO_ARGUS           13                         
#define IPPROTO_EMCON           14                         
#define IPPROTO_XNET            15                                      
#define IPPROTO_CHAOS           16                        
#endif                                              
#define IPPROTO_UDP             17                                          
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define IPPROTO_MUX             18                                
#define IPPROTO_MEAS            19                                              
#define IPPROTO_HMP             20                                   
#define IPPROTO_PRM             21                                            
#define IPPROTO_IDP             22                           
#define IPPROTO_TRUNK1          23                           
#define IPPROTO_TRUNK2          24                           
#define IPPROTO_LEAF1           25                          
#define IPPROTO_LEAF2           26                          
#define IPPROTO_RDP             27                                 
#define IPPROTO_IRTP            28                                        
#define IPPROTO_TP              29                                             
#define IPPROTO_BLT             30                                      
#define IPPROTO_NSP             31                                    
#define IPPROTO_INP             32                                    
#define IPPROTO_SEP             33                                       
#define IPPROTO_3PC             34                                       
#define IPPROTO_IDPR            35                                              
#define IPPROTO_XTP             36                       
#define IPPROTO_DDP             37                                     
#define IPPROTO_CMTP            38                                             
#define IPPROTO_TPXX            39                                  
#define IPPROTO_IL              40                                         
#endif                                              
#define         IPPROTO_IPV6            41                              
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define IPPROTO_SDRP            42                                         
#define         IPPROTO_ROUTING 43                                      
#define         IPPROTO_FRAGMENT        44                                            
#define IPPROTO_IDRP            45                                      
#define         IPPROTO_RSVP            46                                        
#define IPPROTO_GRE             47                                          
#define IPPROTO_MHRP            48                                       
#define IPPROTO_BHA             49                       
#define IPPROTO_ESP             50                                          
#define IPPROTO_AH              51                                   
#define IPPROTO_INLSP           52                                             
#define IPPROTO_SWIPE           53                                      
#define IPPROTO_NHRP            54                                       
                       
#define IPPROTO_ICMPV6          58                         
#define IPPROTO_NONE            59                                      
#define IPPROTO_DSTOPTS         60                                          
#define IPPROTO_AHIP            61                                              
#define IPPROTO_CFTP            62                        
#define IPPROTO_HELLO           63                                            
#define IPPROTO_SATEXPAK        64                                         
#define IPPROTO_KRYPTOLAN       65                             
#define IPPROTO_RVD             66                                       
#define IPPROTO_IPPC            67                                        
#define IPPROTO_ADFS            68                                      
#define IPPROTO_SATMON          69                                     
#define IPPROTO_VISA            70                                 
#define IPPROTO_IPCV            71                                       
#define IPPROTO_CPNX            72                                              
#define IPPROTO_CPHB            73                                         
#define IPPROTO_WSN             74                                     
#define IPPROTO_PVP             75                                         
#define IPPROTO_BRSATMON        76                                              
#define IPPROTO_ND              77                                              
#define IPPROTO_WBMON           78                                       
#define IPPROTO_WBEXPAK         79                                  
#define IPPROTO_EON             80                            
#define IPPROTO_VMTP            81                        
#define IPPROTO_SVMTP           82                               
#define IPPROTO_VINES           83                                
#define IPPROTO_TTP             84                       
#define IPPROTO_IGP             85                              
#define IPPROTO_DGP             86                                            
#define IPPROTO_TCF             87                       
#define IPPROTO_IGRP            88                                  
#define IPPROTO_OSPFIGP         89                           
#define IPPROTO_SRPC            90                                       
#define IPPROTO_LARP            91                                             
#define IPPROTO_MTP             92                                       
#define IPPROTO_AX25            93                                
#define IPPROTO_IPEIP           94                                         
#define IPPROTO_MICP            95                                          
#define IPPROTO_SCCSP           96                                            
#define IPPROTO_ETHERIP         97                                             
#define IPPROTO_ENCAP           98                                        
#define IPPROTO_APES            99                                            
#define IPPROTO_GMTP            100                      
                                
#define IPPROTO_PIM             103                                             
#define IPPROTO_IPCOMP          108                                               
#define IPPROTO_PGM             113                      
#define IPPROTO_SCTP            132                       
                                                                   
                                                 
#ifdef PRIVATE
#define IPPROTO_QUIC            253                                           
#endif              
#define IPPROTO_DIVERT          254                                         
#endif                                              
#define IPPROTO_RAW             255                                

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define IPPROTO_MAX             256

                                                                              
#define IPPROTO_DONE            257
#endif                                             

  
                                 
  
                                                                       
                                                  
                                               
                                                            
  
                                                                   
                            
  
                                                              
  
                                                                          
                                                                             
                                                     
  
                                                                
                                                                    
                                                                         
                                                        
  
                                                                  
                                                          
  
                                                                 
                                                                        
                                                            
  
                                                                          
                                                                  
                                                     
  
                                                  
  
                                                                      
  
                                                 
  
                                            
                                            
                                                        
  
   

#define __DARWIN_IPPORT_RESERVED        1024

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
  
                                           
                                                               
                                           
                                                                   
   
#ifndef IPPORT_RESERVED
#define IPPORT_RESERVED         __DARWIN_IPPORT_RESERVED
#endif
#define IPPORT_USERRESERVED     5000

  
                                                               
   
#define IPPORT_HIFIRSTAUTO      49152
#define IPPORT_HILASTAUTO       65535

  
                                                                          
                                                                           
                                                                           
                        
   
#define IPPORT_RESERVEDSTART    600
#endif                                              

  
                                                        
   
struct in_addr {
	in_addr_t s_addr;
};

  
                                                    
                                                                   
                                                        
   
#define INADDR_ANY              (u_int32_t)0x00000000
#define INADDR_BROADCAST        (u_int32_t)0xffffffff                       

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define IN_CLASSA(i)            (((u_int32_t)(i) & 0x80000000) == 0)
#define IN_CLASSA_NET           0xff000000
#define IN_CLASSA_NSHIFT        24
#define IN_CLASSA_HOST          0x00ffffff
#define IN_CLASSA_MAX           128

#define IN_CLASSB(i)            (((u_int32_t)(i) & 0xc0000000) == 0x80000000)
#define IN_CLASSB_NET           0xffff0000
#define IN_CLASSB_NSHIFT        16
#define IN_CLASSB_HOST          0x0000ffff
#define IN_CLASSB_MAX           65536

#define IN_CLASSC(i)            (((u_int32_t)(i) & 0xe0000000) == 0xc0000000)
#define IN_CLASSC_NET           0xffffff00
#define IN_CLASSC_NSHIFT        8
#define IN_CLASSC_HOST          0x000000ff

#define IN_CLASSD(i)            (((u_int32_t)(i) & 0xf0000000) == 0xe0000000)
#define IN_CLASSD_NET           0xf0000000                                    
#define IN_CLASSD_NSHIFT        28                                            
#define IN_CLASSD_HOST          0x0fffffff                                    
#define IN_MULTICAST(i)         IN_CLASSD(i)

#define IN_EXPERIMENTAL(i)      (((u_int32_t)(i) & 0xf0000000) == 0xf0000000)
#define IN_BADCLASS(i)          (((u_int32_t)(i) & 0xf0000000) == 0xf0000000)

#define INADDR_LOOPBACK         (u_int32_t)0x7f000001

#ifndef KERNEL
#define INADDR_NONE             0xffffffff                             
#endif

#define INADDR_UNSPEC_GROUP     (u_int32_t)0xe0000000                  
#define INADDR_ALLHOSTS_GROUP   (u_int32_t)0xe0000001                  
#define INADDR_ALLRTRS_GROUP    (u_int32_t)0xe0000002                  
#define INADDR_ALLRPTS_GROUP    (u_int32_t)0xe0000016                           
#define INADDR_CARP_GROUP       (u_int32_t)0xe0000012                   
#define INADDR_PFSYNC_GROUP     (u_int32_t)0xe00000f0                    
#define INADDR_ALLMDNS_GROUP    (u_int32_t)0xe00000fb                    
#define INADDR_MAX_LOCAL_GROUP  (u_int32_t)0xe00000ff                    

#ifdef __APPLE__
#define IN_LINKLOCALNETNUM      (u_int32_t)0xA9FE0000                  
#define IN_LINKLOCAL(i)         (((u_int32_t)(i) & IN_CLASSB_NET) == IN_LINKLOCALNETNUM)
#define IN_LOOPBACK(i)          (((u_int32_t)(i) & 0xff000000) == 0x7f000000)
#define IN_ZERONET(i)           (((u_int32_t)(i) & 0xff000000) == 0)

#define IN_PRIVATE(i)   ((((u_int32_t)(i) & 0xff000000) == 0x0a000000) || \
	                 (((u_int32_t)(i) & 0xfff00000) == 0xac100000) || \
	                 (((u_int32_t)(i) & 0xffff0000) == 0xc0a80000))

#ifdef PRIVATE
#define IN_SHARED_ADDRESS_SPACE(i) ((((u_int32_t)(i)) & (u_int32_t)0xffc00000) \
	                            == (u_int32_t)0x64400000)

#define IN_DS_LITE(i) ((((u_int32_t)(i)) & (u_int32_t)0xfffffff8) == (u_int32_t)0xc0000000)

#define IN_6TO4_RELAY_ANYCAST(i) ((((u_int32_t)(i)) & (u_int32_t)IN_CLASSC_NET) == (u_int32_t)0xc0586300)
#endif

#define IN_LOCAL_GROUP(i)       (((u_int32_t)(i) & 0xffffff00) == 0xe0000000)

#define IN_ANY_LOCAL(i)         (IN_LINKLOCAL(i) || IN_LOCAL_GROUP(i))
#endif                

#define IN_LOOPBACKNET          127                                    
#endif                                              

  
                                  
   
struct sockaddr_in {
	__uint8_t       sin_len;
	sa_family_t     sin_family;
	in_port_t       sin_port;
	struct  in_addr sin_addr;
	char            sin_zero[8];
};

#define IN_ARE_ADDR_EQUAL(a, b) \
    (bcmp(&(a)->s_addr, &(b)->s_addr, \
	sizeof (struct in_addr)) == 0)

#ifdef PRIVATE
  
                                                                   
                                                                     
                                                                  
                                                       
   
struct sockaddr_inifscope {
	__uint8_t       sin_len;
	sa_family_t     sin_family;
	in_port_t       sin_port;
	struct  in_addr sin_addr;
	  
                                                              
                                                               
                                                             
                      
    
	union {
		char    sin_zero[8];
		struct {
			__uint32_t      ifscope;
		} _in_index;
	} un;
#define sin_scope_id    un._in_index.ifscope
};

#endif              

#define INET_ADDRSTRLEN                 16

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
  
                                         
                                                               
                                           
                                                                         
                                          
   
struct ip_opts {
	struct  in_addr ip_dst;                                      
	char    ip_opts[40];                                           
};

  
                                                      
                                                             
   
#define IP_OPTIONS              1                                         
#define IP_HDRINCL              2                                           
#define IP_TOS                  3                                             
#define IP_TTL                  4                              
#define IP_RECVOPTS             5                                           
#define IP_RECVRETOPTS          6                                            
#define IP_RECVDSTADDR          7                                           
#define IP_RETOPTS              8                                     
#define IP_MULTICAST_IF         9                                           
#define IP_MULTICAST_TTL        10                                         
#define IP_MULTICAST_LOOP       11                                              
#define IP_ADD_MEMBERSHIP       12                                            
#define IP_DROP_MEMBERSHIP      13                                             
#define IP_MULTICAST_VIF        14                                     
#define IP_RSVP_ON              15                              
#define IP_RSVP_OFF             16                               
#define IP_RSVP_VIF_ON          17                                
#define IP_RSVP_VIF_OFF         18                                  
#define IP_PORTRANGE            19                                             
#define IP_RECVIF               20                                           
               
#define IP_IPSEC_POLICY         21                                     
#define IP_FAITH                22                   
#ifdef __APPLE__
#define IP_STRIPHDR             23                                            
#endif
#define IP_RECVTTL              24                                            
#define IP_BOUND_IF             25                                     
#define IP_PKTINFO              26                                                           
#define IP_RECVPKTINFO          IP_PKTINFO                                   
#define IP_RECVTOS              27                                     
#define IP_DONTFRAG             28                              

#define IP_FW_ADD               40                                     
#define IP_FW_DEL               41                                          
#define IP_FW_FLUSH             42                                  
#define IP_FW_ZERO              43                                             
#define IP_FW_GET               44                                       
#define IP_FW_RESETLOG          45                               

                                                                                         
#define IP_OLD_FW_ADD           50                                     
#define IP_OLD_FW_DEL           51                                          
#define IP_OLD_FW_FLUSH         52                                  
#define IP_OLD_FW_ZERO          53                                             
#define IP_OLD_FW_GET           54                                       
#define IP_NAT__XXX                     55                                                    
#define IP_OLD_FW_RESETLOG      56                               

#define IP_DUMMYNET_CONFIGURE   60                                      
#define IP_DUMMYNET_DEL         61                                          
#define IP_DUMMYNET_FLUSH       62                       
#define IP_DUMMYNET_GET         64                                  

#define IP_TRAFFIC_MGT_BACKGROUND       65                                                         
#define IP_MULTICAST_IFINDEX    66                                             

                                                
#define IP_ADD_SOURCE_MEMBERSHIP        70                                     
#define IP_DROP_SOURCE_MEMBERSHIP       71                             
#define IP_BLOCK_SOURCE                 72                       
#define IP_UNBLOCK_SOURCE               73                         

                                                                            
#define IP_MSFILTER                     74                            

                                                  
#define MCAST_JOIN_GROUP                80                                 
#define MCAST_LEAVE_GROUP               81                                    
#define MCAST_JOIN_SOURCE_GROUP         82                                     
#define MCAST_LEAVE_SOURCE_GROUP        83                              
#define MCAST_BLOCK_SOURCE              84                       
#define MCAST_UNBLOCK_SOURCE            85                         

#ifdef PRIVATE
#define IP_NO_IFT_CELLULAR      6969                            
#define IP_NO_IFT_PDP           IP_NO_IFT_CELLULAR                 
#define IP_OUT_IF               9696                            
#endif              

  
                                  
   
#define IP_DEFAULT_MULTICAST_TTL  1                                           
#define IP_DEFAULT_MULTICAST_LOOP 1                                           

  
                                                                            
                                                                        
                                         
   
#define IP_MIN_MEMBERSHIPS      31
#define IP_MAX_MEMBERSHIPS      4095

  
                                                               
                                   
   
#define IP_MAX_GROUP_SRC_FILTER         512                            
#define IP_MAX_SOCK_SRC_FILTER          128                                   
#define IP_MAX_SOCK_MUTE_FILTER         128                             

#ifndef PLATFORM_DriverKit
  
                                                                   
   
struct ip_mreq {
	struct  in_addr imr_multiaddr;                                     
	struct  in_addr imr_interface;                                     
};

  
                                                                        
                                                                     
                                                                            
   
struct ip_mreqn {
	struct  in_addr imr_multiaddr;                                     
	struct  in_addr imr_address;                                       
	int             imr_ifindex;                                           
};

#pragma pack(4)
  
                                                                      
   
struct ip_mreq_source {
	struct  in_addr imr_multiaddr;                                     
	struct  in_addr imr_sourceaddr;                           
	struct  in_addr imr_interface;                                     
};

  
                                                                
                         
   
struct group_req {
	uint32_t                gr_interface;                        
	struct sockaddr_storage gr_group;                          
};

struct group_source_req {
	uint32_t                gsr_interface;                       
	struct sockaddr_storage gsr_group;                         
	struct sockaddr_storage gsr_source;                         
};

#ifndef __MSFILTERREQ_DEFINED
#define __MSFILTERREQ_DEFINED
  
                                                                            
                                                                          
                                              
   
struct __msfilterreq {
	uint32_t                 msfr_ifindex;                       
	uint32_t                 msfr_fmode;                               
	uint32_t                 msfr_nsrcs;                                   
	uint32_t                __msfr_align;
	struct sockaddr_storage  msfr_group;                       
	struct sockaddr_storage *msfr_srcs;
};

#ifdef BSD_KERNEL_PRIVATE
struct __msfilterreq32 {
	uint32_t                 msfr_ifindex;                       
	uint32_t                 msfr_fmode;                               
	uint32_t                 msfr_nsrcs;                                   
	uint32_t                __msfr_align;
	struct sockaddr_storage  msfr_group;                       
	user32_addr_t            msfr_srcs;
};

struct __msfilterreq64 {
	uint32_t                 msfr_ifindex;                       
	uint32_t                 msfr_fmode;                               
	uint32_t                 msfr_nsrcs;                                   
	uint32_t                __msfr_align;
	struct sockaddr_storage  msfr_group;                       
	user64_addr_t            msfr_srcs;
};
#endif                         
#endif                            

#pragma pack()
struct sockaddr;

#ifndef KERNEL
  
                                       
                                                                         
                                         
   
int     setipv4sourcefilter(int, struct in_addr, struct in_addr, uint32_t,
    uint32_t, struct in_addr *) __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_4_3);
int     getipv4sourcefilter(int, struct in_addr, struct in_addr, uint32_t *,
    uint32_t *, struct in_addr *) __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_4_3);
int     setsourcefilter(int, uint32_t, struct sockaddr *, socklen_t,
    uint32_t, uint32_t, struct sockaddr_storage *) __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_4_3);
int     getsourcefilter(int, uint32_t, struct sockaddr *, socklen_t,
    uint32_t *, uint32_t *, struct sockaddr_storage *) __OSX_AVAILABLE_STARTING(__MAC_10_7, __IPHONE_4_3);
#endif
#endif                         

  
                                                                          
   
#define MCAST_UNDEFINED 0                                   
#define MCAST_INCLUDE   1                                           
#define MCAST_EXCLUDE   2                                           

  
                             
                                                                          
   
#define IP_PORTRANGE_DEFAULT    0                          
#define IP_PORTRANGE_HIGH       1                                             
#define IP_PORTRANGE_LOW        2                                       


  
                                                                         
                             
  
                                                                           
                                                                            
                                                                        
                               
  
                                                                         
                                                                    
                                                                         
                                                                              
                                          
  
                                                                             
                                                                         
                                 
   
struct in_pktinfo {
	unsigned int    ipi_ifindex;                                   
	struct in_addr  ipi_spec_dst;                      
	struct in_addr  ipi_addr;                                  
};

  
                                          
  
                                  
                                                         
   
#define IPPROTO_MAXID   (IPPROTO_AH + 1)                                       

#ifdef BSD_KERNEL_PRIVATE
#define CTL_IPPROTO_NAMES { \
	{ "ip", CTLTYPE_NODE }, \
	{ "icmp", CTLTYPE_NODE }, \
	{ "igmp", CTLTYPE_NODE }, \
	{ "ggp", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "tcp", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ "egp", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "pup", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "udp", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "idp", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "ipsec", CTLTYPE_NODE }, \
}
#endif                         

  
                              
   
#define IPCTL_FORWARDING        1                          
#define IPCTL_SENDREDIRECTS     2                                               
#define IPCTL_DEFTTL            3                        
#ifdef notyet
#define IPCTL_DEFMTU            4                        
#endif
#define IPCTL_RTEXPIRE          5                                         
#define IPCTL_RTMINEXPIRE       6                                          
#define IPCTL_RTMAXCACHE        7                                             
#define IPCTL_SOURCEROUTE       8                                      
#define IPCTL_DIRECTEDBROADCAST 9                                              
#define IPCTL_INTRQMAXLEN       10                                      
#define IPCTL_INTRQDROPS        11                                    
#define IPCTL_STATS             12                            
#define IPCTL_ACCEPTSOURCEROUTE 13                                            
#define IPCTL_FASTFORWARDING    14                                       
#define IPCTL_KEEPFAITH         15                      
#define IPCTL_GIF_TTL           16                                            
#define IPCTL_MAXID             17

#ifdef BSD_KERNEL_PRIVATE

#define IPCTL_NAMES { \
	{ 0, 0 }, \
	{ "forwarding", CTLTYPE_INT }, \
	{ "redirect", CTLTYPE_INT }, \
	{ "ttl", CTLTYPE_INT }, \
	{ "mtu", CTLTYPE_INT }, \
	{ "rtexpire", CTLTYPE_INT }, \
	{ "rtminexpire", CTLTYPE_INT }, \
	{ "rtmaxcache", CTLTYPE_INT }, \
	{ "sourceroute", CTLTYPE_INT }, \
	{ "directed-broadcast", CTLTYPE_INT }, \
	{ "intr-queue-maxlen", CTLTYPE_INT }, \
	{ "intr-queue-drops", CTLTYPE_INT }, \
	{ "stats", CTLTYPE_STRUCT }, \
	{ "accept_sourceroute", CTLTYPE_INT }, \
	{ "fastforwarding", CTLTYPE_INT }, \
	{ "keepfaith", CTLTYPE_INT }, \
	{ "gifttl", CTLTYPE_INT }, \
}
#endif                         
#endif                                              

                 
#define __KAME_NETINET_IN_H_INCLUDED_
#include <netinet6/in6.h>
#undef __KAME_NETINET_IN_H_INCLUDED_

#ifdef PRIVATE
  
                                                                 
                                       
   
union sockaddr_in_4_6 {
	struct sockaddr         sa;
	struct sockaddr_in      sin;
	struct sockaddr_in6     sin6;
};
#define CLAT46_HDR_EXPANSION_OVERHD     (sizeof(struct ip6_hdr) - sizeof(struct ip))

  
                                         
   

#define _DSCP_DF        0                     

#define _DSCP_CS0       0                     
#define _DSCP_CS1       8                     
#define _DSCP_CS2       16                    
#define _DSCP_CS3       24                    
#define _DSCP_CS4       32                    
#define _DSCP_CS5       40                    
#define _DSCP_CS6       48                    
#define _DSCP_CS7       56                    

#define _DSCP_EF        46                    
#define _DSCP_VA        44                    

#define _DSCP_AF11      10                    
#define _DSCP_AF12      12                    
#define _DSCP_AF13      14                    
#define _DSCP_AF21      18                    
#define _DSCP_AF22      20                    
#define _DSCP_AF23      22                    
#define _DSCP_AF31      26                    
#define _DSCP_AF32      28                    
#define _DSCP_AF33      30                    
#define _DSCP_AF41      34                    
#define _DSCP_AF42      36                    
#define _DSCP_AF43      38                    

#define _DSCP_52        52                                          

#define _MAX_DSCP       63                           

#endif              

#ifndef PLATFORM_DriverKit
#ifdef KERNEL
#ifdef BSD_KERNEL_PRIVATE
#include <mach/boolean.h>

struct ip;
struct ifnet;
struct mbuf;

extern boolean_t in_broadcast(struct in_addr, struct ifnet *);
extern boolean_t in_canforward(struct in_addr);
extern u_int32_t in_netof(struct in_addr);

extern uint32_t os_cpu_in_cksum_mbuf(struct mbuf *m, int len, int off,
    uint32_t initial_sum);

extern uint16_t inet_cksum(struct mbuf *, uint32_t, uint32_t, uint32_t);
extern uint16_t inet_cksum_buffer(const void *, uint32_t, uint32_t, uint32_t);
extern uint16_t in_addword(uint16_t, uint16_t);
extern uint16_t in_pseudo(uint32_t, uint32_t, uint32_t);
extern uint16_t in_pseudo64(uint64_t, uint64_t, uint64_t);
extern uint16_t in_cksum_hdr_opt(const struct ip *);
extern uint16_t ip_cksum_hdr_dir(struct mbuf *, uint32_t, int);
extern uint16_t ip_cksum_hdr_dir_buffer(const void *, uint32_t, uint32_t, int);
extern uint32_t in_finalize_cksum(struct mbuf *, uint32_t, uint32_t);
extern uint16_t b_sum16(const void *buf, int len);
#if DEBUG || DEVELOPMENT
extern uint32_t in_cksum_mbuf_ref(struct mbuf *, int, int, uint32_t);
#endif                           

extern int in_getconninfo(struct socket *, sae_connid_t, uint32_t *,
    uint32_t *, int32_t *, user_addr_t, socklen_t *, user_addr_t, socklen_t *,
    uint32_t *, user_addr_t, uint32_t *);
extern struct in_ifaddr * inifa_ifpwithflag(struct ifnet *, uint32_t);
extern struct in_ifaddr * inifa_ifpclatv4(struct ifnet *);

#define in_cksum(_m, _l)                        \
	inet_cksum(_m, 0, 0, _l)
#define in_cksum_buffer(_b, _l)                 \
	inet_cksum_buffer(_b, 0, 0, _l)
#define ip_cksum_hdr_in(_m, _l)                 \
	ip_cksum_hdr_dir(_m, _l, 0)
#define ip_cksum_hdr_out(_m, _l)                \
	ip_cksum_hdr_dir(_m, _l, 1)

#define in_cksum_hdr(_ip)                       \
	(~b_sum16(_ip, sizeof (struct ip)) & 0xffff)

#define in_cksum_offset(_m, _o)         \
	((void) in_finalize_cksum(_m, _o, CSUM_DELAY_IP))
#define in_delayed_cksum(_m)            \
	((void) in_finalize_cksum(_m, 0, CSUM_DELAY_DATA))
#define in_delayed_cksum_offset(_m, _o) \
	((void) in_finalize_cksum(_m, _o, CSUM_DELAY_DATA))

#define in_hosteq(s, t) ((s).s_addr == (t).s_addr)
#define in_nullhost(x)  ((x).s_addr == INADDR_ANY)
#define in_allhosts(x)  ((x).s_addr == htonl(INADDR_ALLHOSTS_GROUP))

#define SIN(s)          ((struct sockaddr_in *)(void *)s)
#define satosin(sa)     SIN(sa)
#define sintosa(sin)    ((struct sockaddr *)(void *)(sin))
#define SINIFSCOPE(s)   ((struct sockaddr_inifscope *)(void *)(s))

#define IPTOS_UNSPEC                    (-1)                          
#define IPTOS_MASK                      0xFF                       
#endif                         

#ifdef KERNEL_PRIVATE
                                      
extern int in_localaddr(struct in_addr);
extern int inaddr_local(struct in_addr);

extern char     *inet_ntoa(struct in_addr);
extern char     *inet_ntoa_r(struct in_addr ina, char *buf,
    size_t buflen);
extern int      inet_pton(int af, const char *, void *);
#endif                     

#define MAX_IPv4_STR_LEN        16
#define MAX_IPv6_STR_LEN        64

extern int       inet_aton(const char *, struct in_addr *);                 
extern const char *inet_ntop(int, const void *, char *, socklen_t);                
#endif             

#ifndef KERNEL
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
__BEGIN_DECLS
int        bindresvport(int, struct sockaddr_in *);
struct sockaddr;
int        bindresvport_sa(int, struct sockaddr *);
__END_DECLS
#endif
#endif              
#endif                         
#endif                     
