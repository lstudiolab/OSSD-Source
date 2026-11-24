/*
 * Copyright (c) 2011-2019 Apple Inc. All rights reserved.
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
 * Copyright (c) 2002 Luigi Rizzo, Universita` di Pisa
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef __IP_FLOWID_H__
#define __IP_FLOWID_H__

#include <sys/types.h>
#include <netinet/in.h>

  
                                                                  
                     
   
struct ip_flow_id {
	u_int32_t       dst_ip;
	u_int32_t       src_ip;
	u_int16_t       dst_port;
	u_int16_t       src_port;
	u_int8_t        proto;
	u_int8_t        flags;                               
	u_int8_t        addr_type;                                    
	struct in6_addr dst_ip6;                                        
	struct in6_addr src_ip6;
	u_int32_t       flow_id6;
	u_int32_t       frag_id6;
};

#define IS_IP6_FLOW_ID(id)      ((id)->addr_type == 6)

#ifdef BSD_KERNEL_PRIVATE
struct route_in6;
struct sockaddr_in6;
struct pf_rule;

  
                                                                  
                                                              
                                                               
   
struct ip_fw_args {
	struct mbuf             *fwa_m;                                           
	struct ifnet            *fwa_oif;                                         
	struct pf_rule          *fwa_pf_rule;                                     
	struct ether_header     *fwa_eh;                                          
	int                     fwa_flags;                                        
	int                     fwa_oflags;                               
	union {
		struct ip_out_args  *_fwa_ipoa;                                      
		struct ip6_out_args *_fwa_ip6oa;                                    
	} fwa_ipoa_;
	union {
		struct route        *_fwa_ro;                                 
		struct route_in6    *_fwa_ro6;                                
	} fwa_ro_;
	union {
		struct sockaddr_in  *_fwa_dst;                                
		struct sockaddr_in6 *_fwa_dst6;                                    
	} fwa_dst_;
	struct route_in6        *fwa_ro6_pmtu;                       
	struct ifnet            *fwa_origifp;                        
	u_int32_t               fwa_mtu;                             
	u_int32_t               fwa_unfragpartlen;                       
	struct ip6_exthdrs      *fwa_exthdrs;                        
	struct ip_flow_id       fwa_id;                                           
	u_int32_t               fwa_cookie;
};
#define fwa_ipoa fwa_ipoa_._fwa_ipoa
#define fwa_ip6oa fwa_ipoa_._fwa_ip6oa
#define fwa_ro fwa_ro_._fwa_ro
#define fwa_ro6 fwa_ro_._fwa_ro6
#define fwa_dst fwa_dst_._fwa_dst
#define fwa_dst6 fwa_dst_._fwa_dst6

                                                                                
struct ip_fw_in_args {
	struct pf_rule          *fwai_pf_rule;                                  
};

#endif                         

#endif                      
