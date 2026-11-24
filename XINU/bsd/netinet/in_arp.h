/*
 * Copyright (c) 2009-2013 Apple Inc. All rights reserved.
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

#ifndef _NETINET_IN_ARP_H_
#define _NETINET_IN_ARP_H_
#ifdef KERNEL
#include <sys/kernel_types.h>

struct sockaddr;
struct sockaddr_dl;
struct sockaddr_in;

   
                            
                                                                      
                                                                    
                       
  
                                                                   
                                                          
                                                                  
                                                              
                                                                
                                                               
                                                        
                                                              
                                                   
                                                        
                                                           
                                                              
                                              
                                                                   
                                                                 
                                                                   
                                                                 
               
   
#ifdef BSD_KERNEL_PRIVATE
extern boolean_t arp_is_entry_probing(route_t p_route);
extern errno_t arp_lookup_ip(ifnet_t interface,
    const struct sockaddr_in *ip_dest, struct sockaddr_dl *ll_dest,
    size_t ll_dest_len, route_t hint, mbuf_t packet);
#define inet_arp_lookup arp_lookup_ip
#else
extern errno_t inet_arp_lookup(ifnet_t interface,
    const struct sockaddr_in *ip_dest, struct sockaddr_dl *ll_dest,
    size_t ll_dest_len, route_t hint, mbuf_t packet);
#endif                          

   
                                  
                                                                  
                                                                  
                                                                    
                                                                  
  
                                                                 
                                                                 
                                                
                                                             
                                                                
                                                                     
                                                               
                                                               
                                                           
   
#ifdef BSD_KERNEL_PRIVATE
extern errno_t arp_ip_handle_input(ifnet_t ifp, u_int16_t arpop,
    const struct sockaddr_dl *sender_hw, const struct sockaddr_in *sender_ip,
    const struct sockaddr_in *target_ip);
#define inet_arp_handle_input arp_ip_handle_input
#else
extern errno_t inet_arp_handle_input(ifnet_t ifp, u_int16_t arpop,
    const struct sockaddr_dl *sender_hw, const struct sockaddr_in *sender_ip,
    const struct sockaddr_in *target_ip);
#endif                          

   
                                 
                                                                       
                                                                
                                                            
                                                                    
                          
  
                                                                  
                                                                  
                                                               
                               
  
                                                                 
                                                                    
                                                       
  
                                                              
                                    
                                                             
                                          
   
#ifdef BSD_KERNEL_PRIVATE
                                                                
#define inet_arp_init_ifaddr    arp_ifinit
#else
extern void inet_arp_init_ifaddr(ifnet_t interface, ifaddr_t ipaddr);
#endif                          

#ifdef BSD_KERNEL_PRIVATE
extern void arp_init(void);
extern void in_arpdrain(void *);
extern void arp_llreach_set_reachable(struct ifnet *, void *, unsigned int);
#endif                         
#endif             
#endif                         
