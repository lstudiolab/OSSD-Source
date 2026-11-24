/*
 * Copyright (c) 2017 Apple Inc. All rights reserved.
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
 * Copyright (c) 2008, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

   
        
                                                     
                                                
                                      
                                        
                                     
                                            
                                              
   


#include <sys/types.h>
#include <sys/queue.h>
#include <sys/domain.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/icmp6.h>
#include <sys/errno.h>
#include <libkern/libkern.h>


#include <net/sixxlowpan.h>
#include <net/frame802154.h>

errno_t
compress_hdr_hc1(struct frame802154 *, u_int8_t *,
    long *, size_t *, u_int8_t *);
errno_t
uncompress_hdr_hc1(struct frame802154 *, u_int8_t *,
    uint16_t, long *, size_t *, u_int8_t *);



   
                         
     
   

   
                                   
     
   
                                               
#define SICSLOWPAN_UDP_PORT_MIN                     0xF0B0
#define SICSLOWPAN_UDP_PORT_MAX                     0xF0BF                  

         

   
                             
     
   
#define SICSLOWPAN_COMPRESSION_IPV6        0
#define SICSLOWPAN_COMPRESSION_HC1         1
#define SICSLOWPAN_COMPRESSION_HC06        2
         

   
                           
     
   
#define SICSLOWPAN_DISPATCH_IPV6                    0x41                    
#define SICSLOWPAN_DISPATCH_HC1                     0x42                    
#define SICSLOWPAN_DISPATCH_IPHC                    0x60                     
#define SICSLOWPAN_DISPATCH_FRAG1                   0xc0               
#define SICSLOWPAN_DISPATCH_FRAGN                   0xe0               
         

                      
     
   
#define SICSLOWPAN_HC1_NH_UDP                       0x02
#define SICSLOWPAN_HC1_NH_TCP                       0x06
#define SICSLOWPAN_HC1_NH_ICMP6                     0x04
         

                                                   
     
   
#define SICSLOWPAN_HC_UDP_ALL_C                     0xE0
         

   
                      
     
   
  
                                                       
                                             
   
#define SICSLOWPAN_IPHC_FL_C                        0x10
#define SICSLOWPAN_IPHC_TC_C                        0x08
#define SICSLOWPAN_IPHC_NH_C                        0x04
#define SICSLOWPAN_IPHC_TTL_1                       0x01
#define SICSLOWPAN_IPHC_TTL_64                      0x02
#define SICSLOWPAN_IPHC_TTL_255                     0x03
#define SICSLOWPAN_IPHC_TTL_I                       0x00


                                                           
#define SICSLOWPAN_IPHC_CID                         0x80

#define SICSLOWPAN_IPHC_SAC                         0x40
#define SICSLOWPAN_IPHC_SAM_00                      0x00
#define SICSLOWPAN_IPHC_SAM_01                      0x10
#define SICSLOWPAN_IPHC_SAM_10                      0x20
#define SICSLOWPAN_IPHC_SAM_11                      0x30

#define SICSLOWPAN_IPHC_SAM_BIT                     4

#define SICSLOWPAN_IPHC_M                           0x08
#define SICSLOWPAN_IPHC_DAC                         0x04
#define SICSLOWPAN_IPHC_DAM_00                      0x00
#define SICSLOWPAN_IPHC_DAM_01                      0x01
#define SICSLOWPAN_IPHC_DAM_10                      0x02
#define SICSLOWPAN_IPHC_DAM_11                      0x03

#define SICSLOWPAN_IPHC_DAM_BIT                     0

                               
#define SICSLOWPAN_IPHC_ADDR_CONTEXT_LL             0
                                            
#define SICSLOWPAN_IPHC_MCAST_RANGE                 0xA0
         

                 
#define SICSLOWPAN_NHC_MASK                         0xF0
#define SICSLOWPAN_NHC_EXT_HDR                      0xE0

   
                                                       
     
   
   
                                                       
     
   
#define SICSLOWPAN_NHC_UDP_MASK                     0xF8
#define SICSLOWPAN_NHC_UDP_ID                       0xF0
#define SICSLOWPAN_NHC_UDP_CHECKSUMC                0x04
#define SICSLOWPAN_NHC_UDP_CHECKSUMI                0x00
                                                                    
#define SICSLOWPAN_NHC_UDP_CS_P_00  0xF0                 
#define SICSLOWPAN_NHC_UDP_CS_P_01  0xF1                                                      
#define SICSLOWPAN_NHC_UDP_CS_P_10  0xF2                                                        
#define SICSLOWPAN_NHC_UDP_CS_P_11  0xF3                                          
         


   
                                     
     
   

#define SICSLOWPAN_IPV6_HDR_LEN                     1                
#define SICSLOWPAN_HC1_HDR_LEN                      3
#define SICSLOWPAN_HC1_HC_UDP_HDR_LEN               7
#define SICSLOWPAN_FRAG1_HDR_LEN                    4
#define SICSLOWPAN_FRAGN_HDR_LEN                    5

                                                       
#define SICSLOWPAN_MIN_COMP_HDR_LEN                 7

                                                      
#define SICSLOWPAN_MIN_UNCOMP_HDR_LEN               40


#define UIP_IPH_LEN    40
#define UIP_UDPH_LEN    8                            
#define UIP_TCPH_LEN   20                            
#define UIP_ICMPH_LEN   4                             

         

   
                                  
                                                        
                                                         
                                
   
                                  
                               
                     
                       
        

   
                                                
  
                                                         
                                                       
                                                        
            
   
                                 
                         
                         
                    
        

   
                                
   
                                        
                         
                             
                                
                    
                      
                           
        

   
                                                         
                                     
   
struct sicslowpan_addr_context {
	uint8_t used;                                    
	uint8_t number;
	uint8_t prefix[8];
};

   
                                               
     
   

   
                                                  
                          
                                                       
                                                                  
                                             
   
#define sicslowpan_is_iid_16_bit_compressable(a) \
((((a)->u16[4]) == 0) &&                       \
(((a)->u8[10]) == 0)&&                      \
(((a)->u8[11]) == 0xff)&&                           \
(((a)->u8[12]) == 0xfe)&&                           \
(((a)->u8[13]) == 0))

   
                                                 
                                                    
                                                     
         
                              
   
#define sicslowpan_is_mcast_addr_decompressable(a) \
(((*a & 0x01) == 0) &&                           \
((*(a + 1) == 0x01) || (*(a + 1) == 0x02)))

   
                                                   
                                                    
                                                  
                 
   
#define sicslowpan_is_mcast_addr_compressable(a) \
((((a)->u16[1]) == 0) &&                       \
(((a)->u16[2]) == 0) &&                       \
(((a)->u16[3]) == 0) &&                       \
(((a)->u16[4]) == 0) &&                       \
(((a)->u16[5]) == 0) &&                       \
(((a)->u16[6]) == 0) &&                       \
(((a)->u8[14]) == 0) &&                       \
((((a)->u8[15]) == 1) || (((a)->u8[15]) == 2)))

                          
#define sicslowpan_is_mcast_addr_compressable48(a) \
((((a)->u16[1]) == 0) &&                       \
(((a)->u16[2]) == 0) &&                       \
(((a)->u16[3]) == 0) &&                       \
(((a)->u16[4]) == 0) &&                       \
(((a)->u8[10]) == 0))

                     
#define sicslowpan_is_mcast_addr_compressable32(a) \
((((a)->u16[1]) == 0) &&                       \
(((a)->u16[2]) == 0) &&                       \
(((a)->u16[3]) == 0) &&                       \
(((a)->u16[4]) == 0) &&                       \
(((a)->u16[5]) == 0) &&                       \
(((a)->u8[12]) == 0))

                
#define sicslowpan_is_mcast_addr_compressable8(a) \
((((a)->u8[1]) == 2) &&                        \
(((a)->u16[1]) == 0) &&                       \
(((a)->u16[2]) == 0) &&                       \
(((a)->u16[3]) == 0) &&                       \
(((a)->u16[4]) == 0) &&                       \
(((a)->u16[5]) == 0) &&                       \
(((a)->u16[6]) == 0) &&                       \
(((a)->u8[14]) == 0))

#define uip_is_addr_mac_addr_based(a, m) \
((((a)->s6_addr[8])  == (((m)[0]) ^ 0x02)) &&        \
(((a)->s6_addr[9])  == (m)[1]) &&            \
(((a)->s6_addr[10]) == (m)[2]) &&            \
(((a)->s6_addr[11]) == (m)[3]) &&            \
(((a)->s6_addr[12]) == (m)[4]) &&            \
(((a)->s6_addr[13]) == (m)[5]) &&            \
(((a)->s6_addr[14]) == (m)[6]) &&            \
(((a)->s6_addr[15]) == (m)[7]))

   
                                                     
  
                                            
  
                   
   
#define uip_ip6addr(addr, addr0, addr1, addr2, addr3, addr4, addr5, addr6, addr7) do {\
(addr)->s6_addr[0] = htons(addr0);                                      \
(addr)->s6_addr[1] = htons(addr1);                                      \
(addr)->s6_addr[2] = htons(addr2);                                      \
(addr)->s6_addr[3] = htons(addr3);                                      \
(addr)->s6_addr[4] = htons(addr4);                                      \
(addr)->s6_addr[5] = htons(addr5);                                      \
(addr)->s6_addr[6] = htons(addr6);                                      \
(addr)->s6_addr[7] = htons(addr7);                                      \
} while(0)

   
                                                      
  
                                            
  
                   
   
#define uip_ip6addr_u8(addr, addr0, addr1, addr2, addr3, addr4, addr5, addr6, addr7, addr8, addr9, addr10, addr11, addr12, addr13, addr14, addr15) do {\
(addr)->s6_addr[0] = addr0;                                       \
(addr)->s6_addr[1] = addr1;                                       \
(addr)->s6_addr[2] = addr2;                                       \
(addr)->s6_addr[3] = addr3;                                       \
(addr)->s6_addr[4] = addr4;                                       \
(addr)->s6_addr[5] = addr5;                                       \
(addr)->s6_addr[6] = addr6;                                       \
(addr)->s6_addr[7] = addr7;                                       \
(addr)->s6_addr[8] = addr8;                                       \
(addr)->s6_addr[9] = addr9;                                       \
(addr)->s6_addr[10] = addr10;                                     \
(addr)->s6_addr[11] = addr11;                                     \
(addr)->s6_addr[12] = addr12;                                     \
(addr)->s6_addr[13] = addr13;                                     \
(addr)->s6_addr[14] = addr14;                                     \
(addr)->s6_addr[15] = addr15;                                     \
} while(0)



                                     
typedef struct uip_802154_shortaddr {
	uint8_t addr[2];
} uip_802154_shortaddr;
                                     
typedef struct uip_802154_longaddr {
	uint8_t addr[8];
} uip_802154_longaddr;

                            
typedef struct uip_80211_addr {
	uint8_t addr[6];
} uip_80211_addr;

                           
typedef struct uip_eth_addr {
	uint8_t addr[6];
} uip_eth_addr;
typedef uip_802154_longaddr uip_lladdr_t;

#define UIP_802154_SHORTADDR_LEN 2
#define UIP_802154_LONGADDR_LEN  8
#define UIP_LLADDR_LEN UIP_802154_LONGADDR_LEN


#define GET16(ptr) (((uint16_t)(((u_int8_t *)ptr)[0] << 8)) | (((u_int8_t *)ptr)[1]))
#define SET16(ptr, value) do {     \
((u_int8_t *)ptr)[0] = ((value) >> 8) & 0xff; \
((u_int8_t *)ptr)[1] = (value) & 0xff;    \
} while(0)

                                          
      
   
#define PACKETBUF_FRAG_DISPATCH_SIZE 0               
#define PACKETBUF_FRAG_TAG           2               
#define PACKETBUF_FRAG_OFFSET        4              

#define PACKETBUF_HC1_DISPATCH       0            
#define PACKETBUF_HC1_ENCODING       1            
#define PACKETBUF_HC1_TTL            2            

#define PACKETBUF_HC1_HC_UDP_DISPATCH      0            
#define PACKETBUF_HC1_HC_UDP_HC1_ENCODING  1            
#define PACKETBUF_HC1_HC_UDP_UDP_ENCODING  2            
#define PACKETBUF_HC1_HC_UDP_TTL           3            
#define PACKETBUF_HC1_HC_UDP_PORTS         4            
#define PACKETBUF_HC1_HC_UDP_CHKSUM        5             


#define LINKADDR_SIZE 8
typedef union {
	unsigned char u8[LINKADDR_SIZE];
	uint16_t u16;
} linkaddr_t;

static void
uip_ds6_set_addr_iid(struct in6_addr *ipaddr, uip_lladdr_t *lladdr)
{
	                                                        
                                
#if (UIP_LLADDR_LEN == 8)
	memcpy(ipaddr->s6_addr + 8, lladdr, UIP_LLADDR_LEN);
	ipaddr->s6_addr[8] ^= 0x02;
#elif (UIP_LLADDR_LEN == 6)
	memcpy(ipaddr->s6_addr + 8, lladdr, 3);
	ipaddr->s6_addr[11] = 0xff;
	ipaddr->s6_addr[12] = 0xfe;
	memcpy(ipaddr->s6_addr + 13, (uint8_t *)lladdr + 3, 3);
	ipaddr->s6_addr[8] ^= 0x02;
#else
#error uip-ds6.c cannot build interface address when UIP_LLADDR_LEN is not 6 or 8
#endif
}

static errno_t
compress_hdr_ipv6(__unused struct frame802154 *ieee02154hdr,
    __unused u_int8_t *payload,
    long *hdroffset, size_t *hdrlen, u_int8_t *hdrbuf)
{
	  
                                                                     
    
	*hdroffset = -SICSLOWPAN_IPV6_HDR_LEN;
	*hdrlen = SICSLOWPAN_IPV6_HDR_LEN;
	hdrbuf[0] = SICSLOWPAN_DISPATCH_IPV6;

	return 0;
}


#if 0
                                                                        
                                                     
                                                                        
                                                                        
   
                                                     
  
                                                                     
                                                                        
                  
  
  
                                                                    
                   
                                 
                    
                                         
                                                    
                                                                      
             
                                             
                                                                       
                                                                    
  
                              
                                        
                                                                   
            
                                                                 
                                                                  
                                                                    
                                                                    
                                                                    
        
                                                                    
               
  
                       
                                                          
            
                                                                 
                                                                  
                                                                    
                                                                    
                                                                    
                                                               
                                                                    
               
  
                                                                      
                       
   
#endif
errno_t
compress_hdr_hc1(struct frame802154 *ieee02154hdr, u_int8_t *payload,
    long *hdroffset, size_t *hdrlen, u_int8_t *hdrbuf)
{
	struct ip6_hdr *ip6 = (struct ip6_hdr *)(payload);

	if (*hdrlen < SICSLOWPAN_MIN_COMP_HDR_LEN) {
		return EINVAL;
	}

	*hdroffset = 0;

	  
                                                     
               
    
	if ((ip6->ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION ||
	    !IN6_IS_ADDR_LINKLOCAL(&ip6->ip6_src) ||
	    !uip_is_addr_mac_addr_based(&ip6->ip6_src, ieee02154hdr->src_addr) ||
	    !IN6_IS_ADDR_LINKLOCAL(&ip6->ip6_dst) ||
	    !uip_is_addr_mac_addr_based(&ip6->ip6_dst,
	    ieee02154hdr->dest_addr) ||
	    (ip6->ip6_nxt != IPPROTO_ICMPV6 &&
	    ip6->ip6_nxt != IPPROTO_UDP &&
	    ip6->ip6_nxt != IPPROTO_TCP)) {
		  
                  
                                                       
                                                           
     
		return compress_hdr_ipv6(ieee02154hdr, payload, hdroffset, hdrlen, hdrbuf);
	} else {
		  
                 
                          
                                                         
                                                            
     
		hdrbuf[PACKETBUF_HC1_DISPATCH] = SICSLOWPAN_DISPATCH_HC1;

		switch (ip6->ip6_nxt) {
		case IPPROTO_ICMPV6:
			                          
			hdrbuf[PACKETBUF_HC1_ENCODING] = 0xFC;
			hdrbuf[PACKETBUF_HC1_TTL] = ip6->ip6_hlim;
			*hdrlen = SICSLOWPAN_HC1_HDR_LEN;
			*hdroffset = sizeof(struct ip6_hdr);
			break;

		case IPPROTO_TCP:
			                          
			hdrbuf[PACKETBUF_HC1_ENCODING] = 0xFE;
			hdrbuf[PACKETBUF_HC1_TTL] = ip6->ip6_hlim;
			*hdrlen = SICSLOWPAN_HC1_HDR_LEN;
			*hdroffset = sizeof(struct ip6_hdr);
			break;

		case IPPROTO_UDP: {
			struct udphdr *udp = (struct udphdr *)(uintptr_t)(ip6 + 1);

			  
                                                               
                                                             
                                                              
      
			printf("source/remote ports %u/%u\n", ntohs(udp->uh_sport), ntohs(udp->uh_dport));
			if (ntohs(udp->uh_sport) >= SICSLOWPAN_UDP_PORT_MIN &&
			    ntohs(udp->uh_sport) < SICSLOWPAN_UDP_PORT_MAX &&
			    ntohs(udp->uh_dport) >= SICSLOWPAN_UDP_PORT_MIN &&
			    ntohs(udp->uh_dport) < SICSLOWPAN_UDP_PORT_MAX) {
				                  
				hdrbuf[PACKETBUF_HC1_HC_UDP_HC1_ENCODING] = 0xFB;

				                                                        
				hdrbuf[PACKETBUF_HC1_HC_UDP_UDP_ENCODING] = 0xE0;
				hdrbuf[PACKETBUF_HC1_HC_UDP_TTL] = ip6->ip6_hlim;

				hdrbuf[PACKETBUF_HC1_HC_UDP_PORTS] =
				    (uint8_t)((ntohs(udp->uh_sport) - SICSLOWPAN_UDP_PORT_MIN) << 4) +
				    (uint8_t)((ntohs(udp->uh_dport) - SICSLOWPAN_UDP_PORT_MIN));

				memcpy(&hdrbuf[PACKETBUF_HC1_HC_UDP_CHKSUM], &udp->uh_sum, 2);
				*hdrlen = SICSLOWPAN_HC1_HC_UDP_HDR_LEN;
				*hdroffset = sizeof(struct ip6_hdr) + sizeof(struct udphdr);
			} else {
				                          
				hdrbuf[PACKETBUF_HC1_ENCODING] = 0xFA;
				hdrbuf[PACKETBUF_HC1_TTL] = ip6->ip6_hlim;
				*hdrlen = SICSLOWPAN_HC1_HDR_LEN;
				*hdroffset = sizeof(struct ip6_hdr);
			}
			break;
		}
		}
	}
	return 0;
}


                                                                        
   
                                                             
                 
  
                                                                     
       
                                                                        
                                                        
                                                                              
                                    
  
                                                                      
                                                                     
            
   
errno_t
uncompress_hdr_hc1(struct frame802154 *frame, u_int8_t *payload,
    uint16_t ip_len, long *hdroffset, size_t *hdrlen, u_int8_t *hdrbuf)
{
	struct ip6_hdr *ip6 = (struct ip6_hdr *)hdrbuf;

	if (payload[PACKETBUF_HC1_DISPATCH] == SICSLOWPAN_DISPATCH_IPV6) {
		*hdroffset = -SICSLOWPAN_IPV6_HDR_LEN;
		*hdrlen = SICSLOWPAN_IPV6_HDR_LEN;
		return 0;
	}

	*hdroffset = 0;

	                                        
	ip6->ip6_flow = 0;
	ip6->ip6_vfc = IPV6_VERSION;

	                               
	uip_ip6addr_u8(&ip6->ip6_src, 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ip6->ip6_src,
	    (uip_lladdr_t *)frame->src_addr);

	uip_ip6addr_u8(&ip6->ip6_dst, 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ip6->ip6_dst,
	    (uip_lladdr_t *)frame->dest_addr);

	*hdrlen = UIP_IPH_LEN;

	                       
	switch (payload[PACKETBUF_HC1_ENCODING] & 0x06) {
	case SICSLOWPAN_HC1_NH_ICMP6:
		ip6->ip6_nxt = IPPROTO_ICMPV6;
		ip6->ip6_hlim = payload[PACKETBUF_HC1_TTL];
		*hdroffset = SICSLOWPAN_HC1_HDR_LEN;
		break;

	case SICSLOWPAN_HC1_NH_TCP:
		ip6->ip6_nxt = IPPROTO_TCP;
		ip6->ip6_hlim = payload[PACKETBUF_HC1_TTL];
		*hdroffset = SICSLOWPAN_HC1_HDR_LEN;
		break;

	case SICSLOWPAN_HC1_NH_UDP:
		ip6->ip6_nxt = IPPROTO_UDP;
		if (payload[PACKETBUF_HC1_HC_UDP_HC1_ENCODING] & 0x01) {
			struct udphdr *udp = (struct udphdr *)(uintptr_t)ip6;

			                                          
			if (payload[PACKETBUF_HC1_HC_UDP_UDP_ENCODING] !=
			    SICSLOWPAN_HC_UDP_ALL_C) {
				printf("sicslowpan (uncompress_hdr), packet not supported");
				return EINVAL;
			}
			            

			ip6->ip6_hlim = payload[PACKETBUF_HC1_HC_UDP_TTL];
			                              
			udp->uh_sport =
			    htons(SICSLOWPAN_UDP_PORT_MIN + (payload[PACKETBUF_HC1_HC_UDP_PORTS] >> 4));
			udp->uh_dport =
			    htons(SICSLOWPAN_UDP_PORT_MIN + (payload[PACKETBUF_HC1_HC_UDP_PORTS] & 0x0F));

			memcpy(&udp->uh_sum, &payload[PACKETBUF_HC1_HC_UDP_CHKSUM], 2);
			*hdrlen += UIP_UDPH_LEN;
			*hdroffset = SICSLOWPAN_HC1_HC_UDP_HDR_LEN;
		} else {
			ip6->ip6_hlim = payload[PACKETBUF_HC1_TTL];
			*hdroffset = SICSLOWPAN_HC1_HDR_LEN;
		}
		break;

	default:
		                                 
		return EINVAL;
	}

	                      
	if (ip_len == 0) {
		size_t len = frame->payload_len - *hdroffset + *hdrlen - sizeof(struct ip6_hdr);

		                                     
		SET16(&ip6->ip6_plen, len);
	} else {
		                            
		SET16(&ip6->ip6_plen, ip_len - UIP_IPH_LEN);
	}
	                                
	if (ip6->ip6_nxt == IPPROTO_UDP) {
		struct udphdr *udp = (struct udphdr *)(uintptr_t)ip6;

		memcpy(&udp->uh_ulen, &ip6->ip6_plen, 2);
	}
	return 0;
}

errno_t
sixxlowpan_compress(struct frame802154 *ieee02154hdr, u_int8_t *payload)
{
	long hdroffset;
	size_t hdrlen;
	u_int8_t hdrbuf[128];
	errno_t error;

	bzero(hdrbuf, sizeof(hdrbuf));
	hdrlen = sizeof(hdrbuf);

	error = compress_hdr_hc1(ieee02154hdr, payload,
	    &hdroffset, &hdrlen, hdrbuf);
	if (error != 0) {
		return error;
	}

	if (hdroffset < 0) {
		  
                                                 
                          
     
		memmove(&payload[hdrlen],
		    &payload[0],
		    ieee02154hdr->payload_len);
		memcpy(&payload[0], hdrbuf, hdrlen);

		ieee02154hdr->payload_len += hdrlen;
	} else if (hdroffset > 0) {
		  
                                                   
    
                                                            
                                           
     
		memmove(&payload[hdrlen],
		    &payload[hdroffset],
		    ieee02154hdr->payload_len - hdroffset);
		memcpy(&payload[0], hdrbuf, hdrlen);

		ieee02154hdr->payload_len += hdrlen - hdroffset;
	}

	return 0;
}

errno_t
sixxlowpan_uncompress(struct frame802154 *ieee02154hdr, u_int8_t *payload)
{
	long hdroffset;
	size_t hdrlen;
	u_int8_t hdrbuf[128];
	errno_t error;

	bzero(hdrbuf, sizeof(hdrbuf));
	hdrlen = sizeof(hdrbuf);

	error = uncompress_hdr_hc1(ieee02154hdr, (u_int8_t *)payload,
	    0, &hdroffset, &hdrlen, hdrbuf);

	if (error != 0) {
		return error;
	}

	if (hdroffset < 0) {
		  
                                                    
                          
     
		if (ieee02154hdr->payload_len < hdrlen) {
			return EINVAL;
		}
		memmove(&payload[0],
		    &payload[hdrlen],
		    ieee02154hdr->payload_len - hdrlen);
		ieee02154hdr->payload_len -= hdrlen;
	} else {
		  
                                                   
                                           
    
                                                  
                                                                
     
		if (ieee02154hdr->payload_len < hdroffset) {
			return EINVAL;
		}
		memmove(payload + hdrlen,
		    payload + hdroffset,
		    ieee02154hdr->payload_len - hdroffset);
		memcpy(payload, hdrbuf, hdrlen);
		ieee02154hdr->payload_len += hdrlen - hdroffset;
	}

	return 0;
}

errno_t
sixxlowpan_output(struct frame802154 *ieee02154hdr, u_int8_t *payload)
{
	errno_t error = 0;

	error = sixxlowpan_compress(ieee02154hdr, payload);
	if (error != 0) {
		goto done;
	}

	  
                        
    

done:
	return error;
}

errno_t
sixxlowpan_input(struct frame802154 *ieee02154hdr, u_int8_t *payload)
{
	errno_t error = 0;

	error = sixxlowpan_uncompress(ieee02154hdr, payload);
	if (error != 0) {
		goto done;
	}

	  
                        
    

done:
	return error;
}
