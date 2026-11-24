/*
 * Copyright (c) 2004-2014 Apple Inc. All rights reserved.
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
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1999, 2000 Jason L. Wright (jason@thought.net)
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
 *	This product includes software developed by Jason L. Wright
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * OpenBSD: if_bridge.h,v 1.14 2001/03/22 03:48:29 jason Exp
 *
 * $FreeBSD$
 */

  
                                                                
   

#ifndef _NET_IF_BRIDGEVAR_H_
#define _NET_IF_BRIDGEVAR_H_

#ifdef PRIVATE

#include <sys/queue.h>

#include <net/if.h>
#include <net/ethernet.h>
#include <netinet/in.h>

  
                                                                   
                                                            
   
#define BRDGADD                 0                                       
#define BRDGDEL                 1                                          
#define BRDGGIFFLGS             2                                         
#define BRDGSIFFLGS             3                                         
#define BRDGSCACHE              4                                       
#define BRDGGCACHE              5                                       
#define BRDGGIFS                6                                        
#define BRDGRTS                 7                                        
#define BRDGSADDR               8                                         
#define BRDGSTO                 9                                          
#define BRDGGTO                 10                                         
#define BRDGDADDR               11                                    
#define BRDGFLUSH               12                                        

#define BRDGGPRI                13                                    
#define BRDGSPRI                14                                    
#define BRDGGHT                 15                                      
#define BRDGSHT                 16                                      
#define BRDGGFD                 17                                         
#define BRDGSFD                 18                                         
#define BRDGGMA                 19                                   
#define BRDGSMA                 20                                   
#define BRDGSIFPRIO             21                                    
#define BRDGSIFCOST             22                                     
#define BRDGGFILT               23                                        
#define BRDGSFILT               24                                        
#define BRDGPURGE               25                                                                   
#define BRDGADDS                26                                           
#define BRDGDELS                27                                              
#define BRDGPARAM               28                                             
#define BRDGGRTE                29                                       
#define BRDGGIFSSTP             30                                                    
#define BRDGSPROTO              31                                    
#define BRDGSTXHC               32                                         
#define BRDGSIFAMAX             33                                            
#define BRDGGHOSTFILTER         34                                            
#define BRDGSHOSTFILTER         35                                            
#define BRDGGMACNATLIST         36                            


  
                                  
   
#pragma pack(4)

struct ifbreq {
	char            ifbr_ifsname[IFNAMSIZ];                     
	uint32_t        ifbr_ifsflags;                               
	uint32_t        ifbr_stpflags;                                   
	uint32_t        ifbr_path_cost;                                 
	uint8_t         ifbr_portno;                                       
	uint8_t         ifbr_priority;                                      
	uint8_t         ifbr_proto;                                         
	uint8_t         ifbr_role;                                      
	uint8_t         ifbr_state;                                      
	uint32_t        ifbr_addrcnt;                                      
	uint32_t        ifbr_addrmax;                                   
	uint32_t        ifbr_addrexceeded;                                     
	uint8_t         pad[32];
};

#pragma pack()

                              
#define IFBIF_LEARNING          0x0001                    
#define IFBIF_DISCOVER          0x0002                                         
#define IFBIF_STP               0x0004                                        
#define IFBIF_SPAN              0x0008                         
#define IFBIF_STICKY            0x0010                                  
#define IFBIF_BSTP_EDGE         0x0020                            
#define IFBIF_BSTP_AUTOEDGE     0x0040                                   
#define IFBIF_BSTP_PTP          0x0080                                 
#define IFBIF_BSTP_AUTOPTP      0x0100                                  
#define IFBIF_BSTP_ADMEDGE      0x0200                                     
#define IFBIF_BSTP_ADMCOST      0x0400                                  
#define IFBIF_PRIVATE           0x0800                               
#define IFBIF_MAC_NAT           0x8000                               

#define IFBIFBITS       "\020\001LEARNING\002DISCOVER\003STP\004SPAN" \
	"\005STICKY\006EDGE\007AUTOEDGE\010PTP"                       \
	"\011AUTOPTP\014PRIVATE"                                      \
	"\020MACNAT"

#define IFBIFMASK       ~(IFBIF_BSTP_EDGE|IFBIF_BSTP_AUTOEDGE|IFBIF_BSTP_PTP| \
	                IFBIF_BSTP_AUTOPTP|IFBIF_BSTP_ADMEDGE| \
	                IFBIF_BSTP_ADMCOST)                    

               
#define IFBF_FLUSHDYN           0x00                                      
#define IFBF_FLUSHALL           0x01                             

               
#define IFBF_FILT_USEIPF        0x00000001                              
                                                    
#define IFBF_FILT_MEMBER        0x00000002                              
                                                     
#define IFBF_FILT_ONLYIP        0x00000004                                 
                                                        
#define IFBF_FILT_MASK          0x00000007                           

  
                            
   

#pragma pack(4)

#ifndef XNU_KERNEL_PRIVATE

struct ifbifconf {
	uint32_t        ifbic_len;                       
	union {
		caddr_t ifbicu_buf;
		struct ifbreq *ifbicu_req;
#define ifbic_buf       ifbic_ifbicu.ifbicu_buf
#define ifbic_req       ifbic_ifbicu.ifbicu_req
	} ifbic_ifbicu;
};

#else                         

struct ifbifconf32 {
	uint32_t        ifbic_len;                       
	union {
		user32_addr_t   ifbicu_buf;
		user32_addr_t   ifbicu_req;
#define ifbic_buf       ifbic_ifbicu.ifbicu_buf
#define ifbic_req       ifbic_ifbicu.ifbicu_req
	} ifbic_ifbicu;
};

struct ifbifconf64 {
	uint32_t        ifbic_len;                       
	union {
		user64_addr_t   ifbicu_buf;
		user64_addr_t   ifbicu_req;
	} ifbic_ifbicu;
};
#endif                         

#pragma pack()

  
                          
   

#pragma pack(4)

#ifndef XNU_KERNEL_PRIVATE

struct ifbareq {
	char            ifba_ifsname[IFNAMSIZ];                     
	unsigned long   ifba_expire;                                     
	uint8_t         ifba_flags;                                
	uint8_t         ifba_dst[ETHER_ADDR_LEN];                         
	uint16_t        ifba_vlan;                           
};

#else                         

struct ifbareq32 {
	char            ifba_ifsname[IFNAMSIZ];                     
	uint32_t        ifba_expire;                                     
	uint8_t         ifba_flags;                                
	uint8_t         ifba_dst[ETHER_ADDR_LEN];                         
	uint16_t        ifba_vlan;                           
};

struct ifbareq64 {
	char            ifba_ifsname[IFNAMSIZ];                     
	uint64_t        ifba_expire;                                     
	uint8_t         ifba_flags;                                
	uint8_t         ifba_dst[ETHER_ADDR_LEN];                         
	uint16_t        ifba_vlan;                           
};
#endif                         

#pragma pack()

#define IFBAF_TYPEMASK  0x03                           
#define IFBAF_DYNAMIC   0x00                                     
#define IFBAF_STATIC    0x01                        
#define IFBAF_STICKY    0x02                        

#define IFBAFBITS       "\020\1STATIC\2STICKY"

  
                          
   

#pragma pack(4)

#ifndef XNU_KERNEL_PRIVATE

struct ifbaconf {
	uint32_t        ifbac_len;                       
	union {
		caddr_t ifbacu_buf;
		struct ifbareq *ifbacu_req;
#define ifbac_buf       ifbac_ifbacu.ifbacu_buf
#define ifbac_req       ifbac_ifbacu.ifbacu_req
	} ifbac_ifbacu;
};

#else                         

struct ifbaconf32 {
	uint32_t        ifbac_len;                       
	union {
		user32_addr_t   ifbacu_buf;
		user32_addr_t   ifbacu_req;
#define ifbac_buf       ifbac_ifbacu.ifbacu_buf
#define ifbac_req       ifbac_ifbacu.ifbacu_req
	} ifbac_ifbacu;
};

struct ifbaconf64 {
	uint32_t        ifbac_len;                       
	union {
		user64_addr_t   ifbacu_buf;
		user64_addr_t   ifbacu_req;
	} ifbac_ifbacu;
};
#endif                         

#pragma pack()

  
                              
   

#pragma pack(4)

struct ifbrparam {
	union {
		uint32_t ifbrpu_int32;
		uint16_t ifbrpu_int16;
		uint8_t ifbrpu_int8;
	} ifbrp_ifbrpu;
};

#pragma pack()

#define ifbrp_csize     ifbrp_ifbrpu.ifbrpu_int32                       
#define ifbrp_ctime     ifbrp_ifbrpu.ifbrpu_int32                             
#define ifbrp_prio      ifbrp_ifbrpu.ifbrpu_int16                            
#define ifbrp_proto     ifbrp_ifbrpu.ifbrpu_int8                             
#define ifbrp_txhc      ifbrp_ifbrpu.ifbrpu_int8                               
#define ifbrp_hellotime ifbrp_ifbrpu.ifbrpu_int8                              
#define ifbrp_fwddelay  ifbrp_ifbrpu.ifbrpu_int8                            
#define ifbrp_maxage    ifbrp_ifbrpu.ifbrpu_int8                           
#define ifbrp_cexceeded ifbrp_ifbrpu.ifbrpu_int32                            
                                                               
#define ifbrp_filter    ifbrp_ifbrpu.ifbrpu_int32                            

  
                                                   
   

#pragma pack(4)

#ifndef XNU_KERNEL_PRIVATE

struct ifbropreq {
	uint8_t         ifbop_holdcount;
	uint8_t         ifbop_maxage;
	uint8_t         ifbop_hellotime;
	uint8_t         ifbop_fwddelay;
	uint8_t         ifbop_protocol;
	uint16_t        ifbop_priority;
	uint16_t        ifbop_root_port;
	uint32_t        ifbop_root_path_cost;
	uint64_t        ifbop_bridgeid;
	uint64_t        ifbop_designated_root;
	uint64_t        ifbop_designated_bridge;
	struct timeval  ifbop_last_tc_time;
};

#else                         

struct ifbropreq32 {
	uint8_t         ifbop_holdcount;
	uint8_t         ifbop_maxage;
	uint8_t         ifbop_hellotime;
	uint8_t         ifbop_fwddelay;
	uint8_t         ifbop_protocol;
	uint16_t        ifbop_priority;
	uint16_t        ifbop_root_port;
	uint32_t        ifbop_root_path_cost;
	uint64_t        ifbop_bridgeid;
	uint64_t        ifbop_designated_root;
	uint64_t        ifbop_designated_bridge;
	struct timeval  ifbop_last_tc_time;
};

struct ifbropreq64 {
	uint8_t         ifbop_holdcount;
	uint8_t         ifbop_maxage;
	uint8_t         ifbop_hellotime;
	uint8_t         ifbop_fwddelay;
	uint8_t         ifbop_protocol;
	uint16_t        ifbop_priority;
	uint16_t        ifbop_root_port;
	uint32_t        ifbop_root_path_cost;
	uint64_t        ifbop_bridgeid;
	uint64_t        ifbop_designated_root;
	uint64_t        ifbop_designated_bridge;
	struct timeval  ifbop_last_tc_time;
};

#endif

#pragma pack()

  
                                                  
   

#pragma pack(4)

struct ifbpstpreq {
	uint8_t         ifbp_portno;                                    
	uint32_t        ifbp_fwd_trans;                                     
	uint32_t        ifbp_design_cost;                                   
	uint32_t        ifbp_design_port;                                   
	uint64_t        ifbp_design_bridge;                                   
	uint64_t        ifbp_design_root;                                   
};

#pragma pack()

  
                                   
   

#pragma pack(4)

#ifndef XNU_KERNEL_PRIVATE

struct ifbpstpconf {
	uint32_t        ifbpstp_len;                     
	union {
		caddr_t ifbpstpu_buf;
		struct ifbpstpreq *ifbpstpu_req;
	} ifbpstp_ifbpstpu;
#define ifbpstp_buf     ifbpstp_ifbpstpu.ifbpstpu_buf
#define ifbpstp_req     ifbpstp_ifbpstpu.ifbpstpu_req
};

#else                         

struct ifbpstpconf32 {
	uint32_t        ifbpstp_len;                     
	union {
		user32_addr_t   ifbpstpu_buf;
		user32_addr_t   ifbpstpu_req;
#define ifbpstp_buf     ifbpstp_ifbpstpu.ifbpstpu_buf
#define ifbpstp_req     ifbpstp_ifbpstpu.ifbpstpu_req
	} ifbpstp_ifbpstpu;
};

struct ifbpstpconf64 {
	uint32_t        ifbpstp_len;                     
	union {
		user64_addr_t   ifbpstpu_buf;
		user64_addr_t   ifbpstpu_req;
	} ifbpstp_ifbpstpu;
};

#endif                         

#pragma pack()

  
                             
   

#define IFBRHF_ENABLED  0x01
#define IFBRHF_HWSRC    0x02                                  
#define IFBRHF_IPSRC    0x04                                  

#pragma pack(4)

struct ifbrhostfilter {
	uint32_t        ifbrhf_flags;                      
	char            ifbrhf_ifsname[IFNAMSIZ];                           
	uint8_t         ifbrhf_hwsrca[ETHER_ADDR_LEN];
	uint32_t        ifbrhf_ipsrc;
};

#pragma pack()

  
                                         
   
struct bridge_hostfilter_stats {
	uint64_t        brhf_bad_ether_type;
	uint64_t        brhf_bad_ether_srchw_addr;

	uint64_t        brhf_ether_too_small;
	uint64_t        brhf_ether_pullup_failed;

	uint64_t        brhf_arp_ok;
	uint64_t        brhf_arp_too_small;
	uint64_t        brhf_arp_pullup_failed;
	uint64_t        brhf_arp_bad_hw_type;
	uint64_t        brhf_arp_bad_pro_type;
	uint64_t        brhf_arp_bad_hw_len;
	uint64_t        brhf_arp_bad_pro_len;
	uint64_t        brhf_arp_bad_op;
	uint64_t        brhf_arp_bad_sha;
	uint64_t        brhf_arp_bad_spa;

	uint64_t        brhf_ip_ok;
	uint64_t        brhf_ip_too_small;
	uint64_t        brhf_ip_pullup_failed;
	uint64_t        brhf_ip_bad_srcaddr;
	uint64_t        brhf_ip_bad_proto;

	uint64_t        brhf_dhcp_too_small;
	uint64_t        brhf_dhcp_bad_op;
	uint64_t        brhf_dhcp_bad_htype;
	uint64_t        brhf_dhcp_bad_hlen;
	uint64_t        brhf_dhcp_bad_chaddr;
	uint64_t        brhf_dhcp_bad_ciaddr;
};

#ifdef XNU_KERNEL_PRIVATE

extern u_int8_t bstp_etheraddr[ETHER_ADDR_LEN];

int     bridgeattach(int);

#endif                         


  
                     
   

#pragma pack(4)

union ifbrip {
	struct in_addr  ifbrip_addr;
	struct in6_addr ifbrip_addr6;
};

struct ifbrmne {
	char            ifbmne_ifname[IFNAMSIZ];                     
	uint64_t        ifbmne_expire;                                
	uint8_t         ifbmne_mac[ETHER_ADDR_LEN];                 
	uint8_t         ifbmne_reserved;
	uint8_t         ifbmne_af;                                       
	union ifbrip    ifbmne_ip;
};
#define ifbmne_ip_addr  ifbmne_ip.ifbrip_addr
#define ifbmne_ip6_addr ifbmne_ip.ifbrip_addr6

#ifndef XNU_KERNEL_PRIVATE

struct ifbrmnelist {
	uint32_t        ifbml_len;                                            
	uint16_t        ifbml_elsize;                              
	uint16_t        ifbml_pad;
	caddr_t         ifbml_buf;
};

#else                         

struct ifbrmnelist32 {
	uint32_t        ifbml_len;                       
	uint16_t        ifbml_elsize;                              
	uint16_t        ifbml_pad;
	user32_addr_t   ifbml_buf;
};

struct ifbrmnelist64 {
	uint32_t        ifbml_len;                       
	uint16_t        ifbml_elsize;                              
	uint16_t        ifbml_pad;
	user64_addr_t   ifbml_buf;
};

#endif                         

#pragma pack()

#endif              
#endif                            
