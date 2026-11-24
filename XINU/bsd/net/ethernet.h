/*
 * Copyright (c) 2000-2017 Apple Inc. All rights reserved.
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
  
                                              
  
   

#ifndef _NET_ETHERNET_H_
#define _NET_ETHERNET_H_
#ifndef DRIVERKIT
#include <sys/appleapiopts.h>
#include <sys/types.h>                        
#else
#include <sys/_types.h>
#include <sys/_types/_u_char.h>
#include <sys/_types/_u_short.h>
#endif                

  
                                                    
   
#define ETHER_ADDR_LEN          6

  
                                         
   
#define ETHER_TYPE_LEN          2

  
                                                 
   
#define ETHER_CRC_LEN           4

  
                                     
   
#define ETHER_HDR_LEN           (ETHER_ADDR_LEN*2+ETHER_TYPE_LEN)

  
                             
   
#define ETHER_MIN_LEN           64

  
                             
   
#define ETHER_MAX_LEN           1518

  
                                                             
                                                            
                                                                 
                                    
   
#define ETHER_ALIGN             2                                               

  
                                    
   
#define ETHER_IS_VALID_LEN(foo) \
	((foo) >= ETHER_MIN_LEN && (foo) <= ETHER_MAX_LEN)

  
                                         
   
typedef struct  ether_header {
	u_char  ether_dhost[ETHER_ADDR_LEN];
	u_char  ether_shost[ETHER_ADDR_LEN];
	u_short ether_type;
} ether_header_t;

  
                                          
   
typedef struct  ether_addr {
	u_char octet[ETHER_ADDR_LEN];
} ether_addr_t;

#define ether_addr_octet octet

#define ETHERTYPE_PUP           0x0200                    
#define ETHERTYPE_IP            0x0800                   
#define ETHERTYPE_ARP           0x0806                                 
#define ETHERTYPE_REVARP        0x8035                                         
#define ETHERTYPE_VLAN          0x8100                                
#define ETHERTYPE_IPV6          0x86dd            
#define ETHERTYPE_PAE           0x888e                        
#define ETHERTYPE_RSN_PREAUTH   0x88c7                                        
#define ETHERTYPE_PTP           0x88f7                                         
#define ETHERTYPE_LOOPBACK      0x9000                               
                                      
#define ETHERTYPE_IEEE802154    0x0809                

  
                                                                       
                                                    
                                                                           
   
#define ETHERTYPE_TRAIL         0x1000                              
#define ETHERTYPE_NTRAILER      16

#define ETHERMTU        (ETHER_MAX_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)
#define ETHERMIN        (ETHER_MIN_LEN-ETHER_HDR_LEN-ETHER_CRC_LEN)

#ifndef DRIVERKIT
#ifdef KERNEL_PRIVATE
  
                                                 
   

struct  ether_addr *ether_aton(const char *);

#ifdef BSD_KERNEL_PRIVATE
extern u_char   etherbroadcastaddr[ETHER_ADDR_LEN];

#if defined (__arm__)

#include <string.h>

static __inline__ int
_ether_cmp(const void * a, const void * b)
{
	return memcmp(a, b, ETHER_ADDR_LEN);
}

#else              

static __inline__ int
_ether_cmp(const void * a, const void * b)
{
	const u_int16_t * a_s = (const u_int16_t *)a;
	const u_int16_t * b_s = (const u_int16_t *)b;

	if (a_s[0] != b_s[0]
	    || a_s[1] != b_s[1]
	    || a_s[2] != b_s[2]) {
		return 1;
	}
	return 0;
}

#endif              
#endif                         

#define ETHER_IS_MULTICAST(addr) (*(addr) & 0x01)                              

#endif                     

#ifndef KERNEL
#include <sys/cdefs.h>

  
                                                
   
__BEGIN_DECLS

int     ether_hostton(const char *, struct ether_addr *);
int     ether_line(const char *, struct ether_addr *, char *);
char    *ether_ntoa(const struct ether_addr *);
struct  ether_addr *ether_aton(const char *);
int     ether_ntohost(char *, const struct ether_addr *);
__END_DECLS
#endif              
#endif                

#endif                        
