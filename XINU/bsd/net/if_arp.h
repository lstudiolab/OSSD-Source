/*
 * Copyright (c) 2000-2016 Apple Inc. All rights reserved.
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
/*
 * Copyright (c) 1986, 1993
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
 *	@(#)if_arp.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD: src/sys/net/if_arp.h,v 1.14.2.1 2000/07/11 20:46:55 archie Exp $
 */

#ifndef _NET_IF_ARP_H_
#define _NET_IF_ARP_H_
#include <stdint.h>
#include <sys/appleapiopts.h>
#include <netinet/in.h>

  
                               
  
                                                                  
                                                                  
                                                                   
                                                               
                                                              
                                                      
   
struct  arphdr {
	u_short ar_hrd;                                         
#define ARPHRD_ETHER    1                                     
#define ARPHRD_IEEE802  6                                       
#define ARPHRD_FRELAY   15                                       
#define ARPHRD_IEEE1394 24                                     
#define ARPHRD_IEEE1394_EUI64 27                      
	u_short ar_pro;                                         
	u_char  ar_hln;                                         
	u_char  ar_pln;                                         
	u_short ar_op;                       
#define ARPOP_REQUEST   1                                       
#define ARPOP_REPLY     2                                         
#define ARPOP_REVREQUEST 3                                                   
#define ARPOP_REVREPLY  4                                             
#define ARPOP_INVREQUEST 8                                    
#define ARPOP_INVREPLY  9                                      
  
                                             
                                
   
#ifdef COMMENT_ONLY
	u_char  ar_sha[];                                    
	u_char  ar_spa[];                                    
	u_char  ar_tha[];                                    
	u_char  ar_tpa[];                                    
#endif
};

  
                    
   
struct arpreq {
	struct  sockaddr arp_pa;                                      
	struct  sockaddr arp_ha;                                      
	int     arp_flags;                                 
};
                                          
#define ATF_INUSE       0x01                      
#define ATF_COM         0x02                                        
#define ATF_PERM        0x04                         
#define ATF_PUBL        0x08                                                
#define ATF_USETRAILERS 0x10                                

struct arpstat {
	                                
	uint32_t txrequests;                                              
	uint32_t txreplies;                                              
	uint32_t txannounces;                                                  
	uint32_t rxrequests;                                                  
	uint32_t rxreplies;                                                  
	uint32_t received;                                                   

	                                        
	uint32_t txconflicts;                                      
	uint32_t invalidreqs;                                          
	uint32_t reqnobufs;                                                
	uint32_t dropped;                                                      
	uint32_t purged;                                                        
	uint32_t timeouts;                                           
	                                             
	uint32_t dupips;                                          

	                        
	uint32_t inuse;                                                
	uint32_t txurequests;                                         
	uint32_t held;                                                     
};

#ifdef BSD_KERNEL_PRIVATE
extern struct arpstat arpstat;
#endif                         

#endif                      
