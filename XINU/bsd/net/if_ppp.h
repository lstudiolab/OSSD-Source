/*
 * Copyright (c) 2000 Apple Computer, Inc. All rights reserved.
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
 * if_ppp.h - Point-to-Point Protocol definitions.
 *
 * Copyright (c) 1989 Carnegie Mellon University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Carnegie Mellon University.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $FreeBSD: src/sys/net/if_ppp.h,v 1.14 1999/08/28 00:48:20 peter Exp $
 */

#ifndef _IF_PPP_H_
#define _IF_PPP_H_
#include <sys/appleapiopts.h>

                                         
#include <net/ppp_defs.h>
#include <net/if.h>

  
               
   
#define PPP_MTU         1500                                          
#define PPP_MAXMRU      65000                             
#define PPP_MAXMTU      16384                             

  
                             
   
#define SC_COMP_PROT    0x00000001                                         
#define SC_COMP_AC      0x00000002                                       
#define SC_COMP_TCP     0x00000004                                         
#define SC_NO_TCP_CCID  0x00000008                                          
#define SC_REJ_COMP_AC  0x00000010                                           
#define SC_REJ_COMP_TCP 0x00000020                                          
#define SC_CCP_OPEN     0x00000040                               
#define SC_CCP_UP       0x00000080                                            
#define SC_DEBUG        0x00010000                                 
#define SC_LOG_INPKT    0x00020000                                           
#define SC_LOG_OUTPKT   0x00040000                                     
#define SC_LOG_RAWIN    0x00080000                                  
#define SC_LOG_FLUSH    0x00100000                                 
#define SC_RCV_B7_0     0x01000000                                         
#define SC_RCV_B7_1     0x02000000                                         
#define SC_RCV_EVNP     0x04000000                                           
#define SC_RCV_ODDP     0x08000000                                          
#define SC_MASK         0x0fff00ff                                     

  
                                                  
   
#define SC_TIMEOUT      0x00000400                                        
#define SC_VJ_RESET     0x00000800                                   
#define SC_COMP_RUN     0x00001000                                         
#define SC_DECOMP_RUN   0x00002000                                           
#define SC_DC_ERROR     0x00004000                                           
#define SC_DC_FERROR    0x00008000                                       
#define SC_TBUSY        0x10000000                                             
#define SC_PKTLOST      0x20000000                                         
#define SC_FLUSH        0x40000000                                           
#define SC_ESCAPED      0x80000000                            

  
                     
   

struct npioctl {
	int         protocol;                                  
	enum NPmode mode;
};

                                                                          
struct ppp_option_data {
	u_char  *ptr;
	u_int   length;
	int     transmit;
};

struct ifpppstatsreq {
	char ifr_name[IFNAMSIZ];
	struct ppp_stats stats;
};

struct ifpppcstatsreq {
	char ifr_name[IFNAMSIZ];
	struct ppp_comp_stats stats;
};

  
                     
   

#define PPPIOCGFLAGS    _IOR('t', 90, int)                                   
#define PPPIOCSFLAGS    _IOW('t', 89, int)                                   
#define PPPIOCGASYNCMAP _IOR('t', 88, int)                         
#define PPPIOCSASYNCMAP _IOW('t', 87, int)                         
#define PPPIOCGUNIT     _IOR('t', 86, int)                               
#define PPPIOCGRASYNCMAP _IOR('t', 85, int)                                
#define PPPIOCSRASYNCMAP _IOW('t', 84, int)                                
#define PPPIOCGMRU      _IOR('t', 83, int)                                
#define PPPIOCSMRU      _IOW('t', 82, int)                                
#define PPPIOCSMAXCID   _IOW('t', 81, int)                              
#define PPPIOCGXASYNCMAP _IOR('t', 80, ext_accm)                        
#define PPPIOCSXASYNCMAP _IOW('t', 79, ext_accm)                        
#define PPPIOCXFERUNIT  _IO('t', 78)                                   
#define PPPIOCSCOMPRESS _IOW('t', 77, struct ppp_option_data)
#define PPPIOCGNPMODE   _IOWR('t', 76, struct npioctl)                  
#define PPPIOCSNPMODE   _IOW('t', 75, struct npioctl)                   
#define PPPIOCGIDLE     _IOR('t', 74, struct ppp_idle)                    
#define PPPIOCSPASS     _IOW('t', 71, struct bpf_program)                      
#define PPPIOCSACTIVE   _IOW('t', 70, struct bpf_program)                      

                                                                        
#define PPPIOCGMTU      _IOR('t', 73, int)                             
#define PPPIOCSMTU      _IOW('t', 72, int)                             

  
                                                                 
                                                     
   
#define SIOCGPPPSTATS   _IOWR('i', 123, struct ifpppstatsreq)
#define SIOCGPPPCSTATS  _IOWR('i', 122, struct ifpppcstatsreq)

#if !defined(ifr_mtu)
#define ifr_mtu ifr_ifru.ifru_metric
#endif

#endif                 
