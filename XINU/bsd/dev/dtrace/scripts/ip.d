/*
 * Copyright (c) 2006-2012 Apple Inc.  All Rights Reserved.
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
 * @APPLE_LICENSE_HEADER_END@
 */

#pragma D depends_on library darwin.d
#pragma D depends_on module mach_kernel
#pragma D depends_on provider ip

                                        

typedef struct pktinfo {
	struct mbuf *pkt_addr;	                                         
} pktinfo_t;

#pragma D binding "1.0" translator
translator pktinfo_t < struct mbuf *m > {
	pkt_addr = m;
};

typedef struct csinfo {
	uint8_t   ip_ver;
	uint16_t  dport;
	uint16_t  sport;
	string	  ip_daddr;
	string	  ip_saddr;
	uint8_t	  protocol;
	struct inpcb *cs_addr;	                                     
} csinfo_t;

#pragma D binding "1.0" translator
translator csinfo_t < struct inpcb *P > {
	cs_addr = P;
	ip_ver = (P != NULL) ? (((P->inp_vflag & 0x2) != 0) ? 6 : 4) : 0;
	dport = (P != NULL) ? ntohs(P->inp_fport) : 0;
	sport = (P != NULL) ? ntohs(P->inp_lport) : 0;
	ip_saddr = (P != NULL) ? (((P->inp_vflag & 0x2) != 0) ? 
			inet_ntoa6(&P->inp_dependladdr.inp6_local) :
			inet_ntoa((uint32_t *)&P->inp_dependladdr.inp46_local.ia46_addr4.s_addr)) : "<null>";
	ip_daddr = (P != NULL) ? (((P->inp_vflag & 0x2) != 0) ?
			inet_ntoa6(&P->inp_dependfaddr.inp6_foreign) :
			inet_ntoa((uint32_t *)&P->inp_dependfaddr.inp46_foreign.ia46_addr4.s_addr)) : "<null>";
	protocol = P->inp_ip_p;
};

typedef struct ipinfo {
	uint8_t  ip_ver;		                       
	uint16_t ip_plength;		                    
	string   ip_saddr;		                    
	string   ip_daddr;		                         
} ipinfo_t;

  
                                                                     
                                                                         
                                                                        
                                                        
   
#pragma D binding "1.0" translator
translator ipinfo_t < struct ip * ip > {
	ip_ver = (ip != NULL) ? ((*(uint8_t *) ip) & 0xf0) >> 4 : 0;
	ip_plength = (ip != NULL) ? 
		(ntohs(ip->ip_len) - (((*(uint8_t *) ip) & 0x0f) << 2)) : 0;
	ip_saddr = (ip != NULL) ? inet_ntoa((uint32_t *)&ip->ip_src.s_addr) : "<null>";
	ip_daddr = (ip != NULL) ? inet_ntoa((uint32_t *)&ip->ip_dst.s_addr) : "<null>";
};

#pragma D binding "1.0" translator
translator ipinfo_t < struct ip6_hdr *ip6 > {
	ip_ver = (ip6 != NULL) ? (ip6->ip6_ctlun.ip6_un2_vfc & 0xf0) >> 4 : 0;
	ip_plength = (ip6 != NULL) ? (ntohs(ip6->ip6_ctlun.ip6_un1.ip6_un1_plen)) : 0;
	ip_saddr = (ip6 != NULL) ? inet_ntoa6(&ip6->ip6_src) : "<null>";
	ip_daddr = (ip6 != NULL) ? inet_ntoa6(&ip6->ip6_dst) : "<null>";
};

  
                                                                       
                                                            
   
typedef uintptr_t void_ip_t;
#pragma D binding "1.0" translator
translator ipinfo_t < void_ip_t *i> {
	ip_ver = (i != NULL) ? (*(uint8_t *)i >> 4) : 0;
	ip_plength = (i != NULL) ? (((*(uint8_t *)i) >> 4 == 4) ? 
		ntohs(((struct ip *)i)->ip_len) - 
		(((*(uint8_t *)i) & 0x0f) << 2): 
		(((*(uint8_t *)i) >> 4 == 6) ? 
		ntohs(((struct ip6_hdr *)i)->ip6_ctlun.ip6_un1.ip6_un1_plen) : 0)) : 0;
	ip_saddr = (i != NULL) ? ((((*(uint8_t *)i)) >> 4 == 4) ? 
		inet_ntoa((uint32_t *)&(((struct ip *)i)->ip_src.s_addr)) : 
		((((*(uint8_t *)i) >> 4) == 6) ?
		inet_ntoa6(&((struct ip6_hdr *)i)->ip6_src) : "<unknown>")) : "<null>"; 
	ip_daddr = (i != NULL) ? (((*(uint8_t *)i) >> 4 == 4) ? 
		inet_ntoa((uint32_t *)&((struct ip*)i)->ip_dst.s_addr) : ((((*(uint8_t *)i) >> 4) == 6) ?
		inet_ntoa6(&((struct ip6_hdr *)i)->ip6_dst) : "<unknown>")) : "<null>";
};

typedef struct ifinfo {
	string    if_name;		                    
	int8_t    if_local;		                          
	int8_t    if_ipstack;		                
	struct ifnet *if_addr;		                          
	uint16_t  if_flags;		                                    
	uint32_t  if_eflags;		                    
	uint16_t  if_unit;
} ifinfo_t;

#pragma D binding "1.0" translator
translator ifinfo_t < struct ifnet *ifp > {
	if_name = (ifp != NULL) ? ifp->if_name : "<null>";
	if_unit = (ifp != NULL) ? ifp->if_unit : 0;
	if_local = 0;
	if_ipstack = 0;
	if_addr = ifp;
	if_flags = (ifp != NULL) ? ifp->if_flags : 0;
	if_eflags = (ifp != NULL) ? ifp->if_eflags : 0;
	
};

typedef struct ipv4info {
	uint8_t	  ipv4_ver;		                    
	uint8_t   ipv4_ihl;		                          
	uint8_t   ipv4_tos;		                           
	uint16_t  ipv4_length;		                               
	uint16_t  ipv4_ident;		                    
	uint8_t   ipv4_flags;		              
	uint16_t  ipv4_offset;		                     
	uint8_t   ipv4_ttl;		                  
	uint8_t   ipv4_protocol;	                         
	string    ipv4_protostr;	                                      
	uint16_t  ipv4_checksum;	                     
	in_addr_t ipv4_src;		                    
	in_addr_t ipv4_dst;		                         
	string    ipv4_saddr;		                            
	string    ipv4_daddr;		                                 
	struct ip *ipv4_hdr;		                           
} ipv4info_t;

#pragma D binding "1.0" translator
translator ipv4info_t < struct ip *ip > {
	ipv4_ver = (ip != NULL) ? (*(uint8_t *)ip & 0xf0) >> 4 : 0;
	ipv4_ihl = (ip != NULL) ? ((*(uint8_t *)ip & 0x0f) << 2) : 0;
	ipv4_tos = (ip!= NULL) ? ip->ip_tos : 0;
	ipv4_length = (ip != NULL) ? ntohs(ip->ip_len) : 0; 
	ipv4_ident = (ip != NULL) ? ip->ip_id : 0;
	ipv4_flags = (ip != NULL) ? (ntohs(ip->ip_off) & 0xe000) : 0;
	ipv4_offset = (ip != NULL) ? (ntohs(ip->ip_off) & 0x1fff) : 0;
	ipv4_ttl = (ip != NULL) ? ip->ip_ttl : 0;
	ipv4_protocol = (ip != NULL) ? ip->ip_p : 0;
	ipv4_protostr = (ip == NULL) ? "<null>" :
			(ip->ip_p == 1) ? "ICMP" :
			(ip->ip_p == 2) ? "IGMP" :
			(ip->ip_p == 4) ? "IP" :
			(ip->ip_p == 6) ? "TCP": 
			(ip->ip_p == 17) ? "UDP" : 
			(ip->ip_p == 50) ? "ESP": 
			(ip->ip_p == 51) ? "AH" : 
			(ip->ip_p == 58) ? "ICMPV6" : 
			(ip->ip_p == 255) ? "RAW" : stringof(ip->ip_p);
	ipv4_checksum = (ip != NULL) ? ntohs(ip->ip_sum) : 0;
	ipv4_src = (ip != NULL) ? ip->ip_src.s_addr : 0;
	ipv4_dst = (ip != NULL) ? ip->ip_dst.s_addr : 0;
	ipv4_saddr = (ip != NULL) ? inet_ntoa((uint32_t *)&ip->ip_src.s_addr) : "<null>";
	ipv4_daddr = (ip != NULL) ? inet_ntoa((uint32_t *)&ip->ip_dst.s_addr) : "<null>";
	ipv4_hdr = ip;
};

typedef struct ipv6info {
	uint8_t    ipv6_ver;		                    
	uint8_t    ipv6_tclass;		                   
	uint32_t   ipv6_flow;		                
	uint16_t   ipv6_plen;		                    
	uint8_t    ipv6_nexthdr;	                          
	string     ipv6_nextstr;	                                       
	uint8_t    ipv6_hlim;		               
	struct in6_addr *ipv6_src;	                                                
	struct in6_addr *ipv6_dst;	                                                     
	string     ipv6_saddr;		                            
	string     ipv6_daddr;		                                 
	struct ip6_hdr *ipv6_hdr;	                           
} ipv6info_t;

#pragma D binding "1.0" translator
translator ipv6info_t < struct ip6_hdr *ip6 > {
 	ipv6_ver = (ip6 != NULL) ? ip6->ip6_ctlun.ip6_un2_vfc : 10;
	ipv6_tclass = (ip6 != NULL) ? (ip6->ip6_ctlun.ip6_un1.ip6_un1_flow & 0x0ff00000) >> 20 : 0;
	ipv6_flow = (ip6 != NULL) ? (ip6->ip6_ctlun.ip6_un1.ip6_un1_flow & 0x000fffff) : 0;
	ipv6_plen = (ip6 != NULL) ? ntohs(ip6->ip6_ctlun.ip6_un1.ip6_un1_plen) : 0;
	ipv6_nexthdr = (ip6 != NULL) ? ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt : 0;
	ipv6_nextstr = (ip6 == NULL) ? "<null>" :
			(ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt == 1) ? "ICMP" :
			(ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt == 2) ? "IGMP" :
			(ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt == 4) ? "IP" :
			(ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt == 6) ? "TCP" :
			(ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt == 17) ? "UDP" :
			(ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt == 50) ? "ESP" :
			(ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt == 51) ? "AH" : 
			(ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt == 58) ? "ICMPV6" :
			(ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt == 255) ? "RAW" : 
			stringof(ip6->ip6_ctlun.ip6_un1.ip6_un1_nxt);
	ipv6_hlim = (ip6 != NULL) ? ip6->ip6_ctlun.ip6_un1.ip6_un1_hlim : 0;
	ipv6_src = (ip6 != NULL) ? (&ip6->ip6_src) : 0;
	ipv6_dst = (ip6 != NULL) ? (&ip6->ip6_dst) : 0;
	ipv6_saddr = (ip6 != NULL) ? inet_ntoa6(&ip6->ip6_src) : "<null>";
	ipv6_daddr = (ip6 != NULL) ? inet_ntoa6(&ip6->ip6_dst) : "<null>"; 
	ipv6_hdr = ip6;
};
