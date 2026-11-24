/*
 * Copyright (c) 2008-2020 Apple Inc. All rights reserved.
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
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
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
 */

#ifndef DRIVERKIT
#ifndef __KAME_NETINET_IN_H_INCLUDED_
#error "do not include netinet6/in6.h directly, include netinet/in.h. " \
        " see RFC2553"
#endif
#endif                

#ifndef _NETINET6_IN6_H_
#define _NETINET6_IN6_H_
#ifndef DRIVERKIT
#include <sys/appleapiopts.h>
#else
#include <sys/_types/_in_port_t.h>
#endif                

#include <sys/_types.h>
#include <sys/_types/_sa_family_t.h>

  
                                               
                                                                             
                                                           
   
#define __KAME__
#define __KAME_VERSION          "2009/apple-darwin"

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
  
                                 
  
                                                                             
                                                          
  
                                                                       
                                                  
  
                                                                  
                                                             
                                        
  
                                                                   
                            
  
                                                              
  
                                                                   
                                                      
  
                                                                
                                                              
                                                           
                                                                          
                                                         
   

#define IPV6PORT_RESERVED       1024
#define IPV6PORT_ANONMIN        49152
#define IPV6PORT_ANONMAX        65535
#define IPV6PORT_RESERVEDMIN    600
#define IPV6PORT_RESERVEDMAX    (IPV6PORT_RESERVED-1)
#endif                                             

  
               
   
typedef struct in6_addr {
	union {
		__uint8_t   __u6_addr8[16];
		__uint16_t  __u6_addr16[8];
		__uint32_t  __u6_addr32[4];
	} __u6_addr;                                             
} in6_addr_t;

#define s6_addr   __u6_addr.__u6_addr8
#ifdef KERNEL                        
#define s6_addr8  __u6_addr.__u6_addr8
#define s6_addr16 __u6_addr.__u6_addr16
#define s6_addr32 __u6_addr.__u6_addr32
#endif

#define INET6_ADDRSTRLEN        46

  
                          
   
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define SIN6_LEN
#endif                                             
struct sockaddr_in6 {
	__uint8_t       sin6_len;                                               
	sa_family_t     sin6_family;                                
	in_port_t       sin6_port;                                              
	__uint32_t      sin6_flowinfo;                            
	struct in6_addr sin6_addr;                       
	__uint32_t      sin6_scope_id;                        
};

#ifdef KERNEL                        
  
                             
   
#define IN6MASK0        {{{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }}}
#define IN6MASK7        {{{ 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6MASK8        {{{ 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6MASK16       {{{ 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6MASK32       {{{ 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, \
	                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6MASK64       {{{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
	                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6MASK96       {{{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
	                    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6MASK128      {{{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, \
	                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }}}
#endif

#ifndef PLATFORM_DriverKit

#ifdef KERNEL_PRIVATE
extern const struct sockaddr_in6 sa6_any;

extern const struct in6_addr in6mask0;
extern const struct in6_addr in6mask7;
extern const struct in6_addr in6mask16;
extern const struct in6_addr in6mask32;
extern const struct in6_addr in6mask64;
extern const struct in6_addr in6mask96;
extern const struct in6_addr in6mask128;

#define SIN6(s)         ((struct sockaddr_in6 *)(void *)s)
#define satosin6(sa)    SIN6(sa)
#define sin6tosa(sin6)  ((struct sockaddr *)(void *)(sin6))
#define SIN6IFSCOPE(s)  SIN6(s)
#endif                     

#ifdef KERNEL                        
  
                                              
   
#if BYTE_ORDER == BIG_ENDIAN
#define IPV6_ADDR_INT32_ONE     1
#define IPV6_ADDR_INT32_TWO     2
#define IPV6_ADDR_INT32_MNL     0xff010000
#define IPV6_ADDR_INT32_MLL     0xff020000
#define IPV6_ADDR_INT32_SMP     0x0000ffff
#define IPV6_ADDR_INT16_ULL     0xfe80
#define IPV6_ADDR_INT16_USL     0xfec0
#define IPV6_ADDR_INT16_MLL     0xff02
#elif BYTE_ORDER == LITTLE_ENDIAN
#define IPV6_ADDR_INT32_ONE     0x01000000
#define IPV6_ADDR_INT32_TWO     0x02000000
#define IPV6_ADDR_INT32_MNL     0x000001ff
#define IPV6_ADDR_INT32_MLL     0x000002ff
#define IPV6_ADDR_INT32_SMP     0xffff0000
#define IPV6_ADDR_INT16_ULL     0x80fe
#define IPV6_ADDR_INT16_USL     0xc0fe
#define IPV6_ADDR_INT16_MLL     0x02ff
#endif
#endif

  
                                                           
   
#define IN6ADDR_ANY_INIT \
	{{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6ADDR_LOOPBACK_INIT \
	{{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}}
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define IN6ADDR_NODELOCAL_ALLNODES_INIT \
	{{{ 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}}
#define IN6ADDR_INTFACELOCAL_ALLNODES_INIT \
	{{{ 0xff, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}}
#define IN6ADDR_LINKLOCAL_ALLNODES_INIT \
	{{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }}}
#define IN6ADDR_LINKLOCAL_ALLROUTERS_INIT \
	{{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 }}}
#define IN6ADDR_LINKLOCAL_ALLV2ROUTERS_INIT \
	{{{ 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16 }}}
#define IN6ADDR_V4MAPPED_INIT \
	{{{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
	    0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00 }}}
#define IN6ADDR_MULTICAST_PREFIX        IN6MASK8
#endif                                             

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
extern const struct in6_addr in6addr_nodelocal_allnodes;
extern const struct in6_addr in6addr_linklocal_allnodes;
extern const struct in6_addr in6addr_linklocal_allrouters;
extern const struct in6_addr in6addr_linklocal_allv2routers;
#endif                                             

  
           
                                                                            
                                                                         
                    
   
#ifdef KERNEL
#define IN6_ARE_ADDR_EQUAL(a, b) \
	(bcmp(&(a)->s6_addr[0], &(b)->s6_addr[0], \
	sizeof (struct in6_addr)) == 0)
#else
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define IN6_ARE_ADDR_EQUAL(a, b) \
	(memcmp(&(a)->s6_addr[0], &(b)->s6_addr[0], sizeof (struct in6_addr)) \
	== 0)
#endif                                             
#endif

#ifdef KERNEL                                     
                                                                 
#define SA6_ARE_ADDR_EQUAL(a, b) \
	(((a)->sin6_scope_id == 0 || (b)->sin6_scope_id == 0 || \
	((a)->sin6_scope_id == (b)->sin6_scope_id)) && \
	(bcmp(&(a)->sin6_addr, &(b)->sin6_addr, sizeof (struct in6_addr)) == 0))
#endif

  
              
   
#define IN6_IS_ADDR_UNSPECIFIED(a)      \
	((*(const __uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[8]) == 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[12]) == 0))

  
           
   
#define IN6_IS_ADDR_LOOPBACK(a)         \
	((*(const __uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[8]) == 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[12]) == ntohl(1)))

  
                  
   
#define IN6_IS_ADDR_V4COMPAT(a)         \
	((*(const __uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[8]) == 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[12]) != 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[12]) != ntohl(1)))

  
         
   
#define IN6_IS_ADDR_V4MAPPED(a)               \
	((*(const __uint32_t *)(const void *)(&(a)->s6_addr[0]) == 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[4]) == 0) && \
	(*(const __uint32_t *)(const void *)(&(a)->s6_addr[8]) == \
	ntohl(0x0000ffff)))

  
       
   
#define IN6_IS_ADDR_6TO4(x)     (ntohs((x)->s6_addr16[0]) == 0x2002)

  
                    
   

#ifdef KERNEL                        
#define IPV6_ADDR_SCOPE_NODELOCAL       0x01
#define IPV6_ADDR_SCOPE_INTFACELOCAL    0x01
#define IPV6_ADDR_SCOPE_LINKLOCAL       0x02
#define IPV6_ADDR_SCOPE_SITELOCAL       0x05
#define IPV6_ADDR_SCOPE_ORGLOCAL        0x08                                
#define IPV6_ADDR_SCOPE_GLOBAL          0x0e
#else
#define __IPV6_ADDR_SCOPE_NODELOCAL     0x01
#define __IPV6_ADDR_SCOPE_INTFACELOCAL  0x01
#define __IPV6_ADDR_SCOPE_LINKLOCAL     0x02
#define __IPV6_ADDR_SCOPE_SITELOCAL     0x05
#define __IPV6_ADDR_SCOPE_ORGLOCAL      0x08                                
#define __IPV6_ADDR_SCOPE_GLOBAL        0x0e
#endif

  
                
                                                                           
   
#define IN6_IS_ADDR_LINKLOCAL(a)        \
	(((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0x80))
#define IN6_IS_ADDR_SITELOCAL(a)        \
	(((a)->s6_addr[0] == 0xfe) && (((a)->s6_addr[1] & 0xc0) == 0xc0))

  
            
   
#define IN6_IS_ADDR_MULTICAST(a)        ((a)->s6_addr[0] == 0xff)

#define IPV6_ADDR_MC_FLAGS(a)           ((a)->s6_addr[1] & 0xf0)

#define IPV6_ADDR_MC_FLAGS_TRANSIENT            0x10
#define IPV6_ADDR_MC_FLAGS_PREFIX               0x20
#define IPV6_ADDR_MC_FLAGS_UNICAST_BASED        (IPV6_ADDR_MC_FLAGS_TRANSIENT | IPV6_ADDR_MC_FLAGS_PREFIX)

#define IN6_IS_ADDR_UNICAST_BASED_MULTICAST(a)  \
	(IN6_IS_ADDR_MULTICAST(a) &&            \
	(IPV6_ADDR_MC_FLAGS(a) == IPV6_ADDR_MC_FLAGS_UNICAST_BASED))

  
                                                     
   
#define IN6_IS_ADDR_UNIQUE_LOCAL(a) \
	(((a)->s6_addr[0] == 0xfc) || ((a)->s6_addr[0] == 0xfd))

#ifdef KERNEL                        
#define IPV6_ADDR_MC_SCOPE(a)           ((a)->s6_addr[1] & 0x0f)
#else
#define __IPV6_ADDR_MC_SCOPE(a)         ((a)->s6_addr[1] & 0x0f)
#endif

  
                  
   
#ifdef KERNEL                                 
#define IN6_IS_ADDR_MC_UNICAST_BASED_LINKLOCAL(a)                               \
	(IN6_IS_ADDR_MULTICAST(a) &&                                            \
	(IPV6_ADDR_MC_FLAGS(a) == IPV6_ADDR_MC_FLAGS_UNICAST_BASED) &&          \
	(IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_LINKLOCAL))
#define IN6_IS_ADDR_MC_NODELOCAL(a)     \
	(IN6_IS_ADDR_MULTICAST(a) &&    \
	(IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_NODELOCAL))
#define IN6_IS_ADDR_MC_INTFACELOCAL(a)  \
	(IN6_IS_ADDR_MULTICAST(a) &&    \
	(IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_INTFACELOCAL))
#define IN6_IS_ADDR_MC_LINKLOCAL(a)                                             \
	(IN6_IS_ADDR_MULTICAST(a) &&                                            \
	(IPV6_ADDR_MC_FLAGS(a) != IPV6_ADDR_MC_FLAGS_UNICAST_BASED) &&          \
	(IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_LINKLOCAL))
#define IN6_IS_ADDR_MC_SITELOCAL(a)     \
	(IN6_IS_ADDR_MULTICAST(a) &&    \
	(IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_SITELOCAL))
#define IN6_IS_ADDR_MC_ORGLOCAL(a)      \
	(IN6_IS_ADDR_MULTICAST(a) &&    \
	(IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_ORGLOCAL))
#define IN6_IS_ADDR_MC_GLOBAL(a)        \
	(IN6_IS_ADDR_MULTICAST(a) &&    \
	(IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_GLOBAL))
#else
#define IN6_IS_ADDR_MC_NODELOCAL(a)     \
	(IN6_IS_ADDR_MULTICAST(a) &&    \
	(__IPV6_ADDR_MC_SCOPE(a) == __IPV6_ADDR_SCOPE_NODELOCAL))
#define IN6_IS_ADDR_MC_LINKLOCAL(a)                                             \
	(IN6_IS_ADDR_MULTICAST(a) &&                                            \
	(IPV6_ADDR_MC_FLAGS(a) != IPV6_ADDR_MC_FLAGS_UNICAST_BASED) &&          \
	(__IPV6_ADDR_MC_SCOPE(a) == __IPV6_ADDR_SCOPE_LINKLOCAL))
#define IN6_IS_ADDR_MC_SITELOCAL(a)     \
	(IN6_IS_ADDR_MULTICAST(a) &&    \
	(__IPV6_ADDR_MC_SCOPE(a) == __IPV6_ADDR_SCOPE_SITELOCAL))
#define IN6_IS_ADDR_MC_ORGLOCAL(a)      \
	(IN6_IS_ADDR_MULTICAST(a) &&    \
	(__IPV6_ADDR_MC_SCOPE(a) == __IPV6_ADDR_SCOPE_ORGLOCAL))
#define IN6_IS_ADDR_MC_GLOBAL(a)        \
	(IN6_IS_ADDR_MULTICAST(a) &&    \
	(__IPV6_ADDR_MC_SCOPE(a) == __IPV6_ADDR_SCOPE_GLOBAL))
#endif

#ifdef KERNEL                    
  
             
   
#define IN6_IS_SCOPE_LINKLOCAL(a)       \
	((IN6_IS_ADDR_LINKLOCAL(a)) ||  \
	(IN6_IS_ADDR_MC_LINKLOCAL(a)))

#define IN6_IS_SCOPE_EMBED(a)                   \
	((IN6_IS_ADDR_LINKLOCAL(a)) ||          \
	(IN6_IS_ADDR_MC_LINKLOCAL(a)) ||        \
	(IN6_IS_ADDR_MC_INTFACELOCAL(a)))

#define IFA6_IS_DEPRECATED(a, t) \
	((a)->ia6_lifetime.ia6ti_preferred != 0 && \
	(a)->ia6_lifetime.ia6ti_preferred < (t))
#define IFA6_IS_INVALID(a, t) \
	((a)->ia6_lifetime.ia6ti_expire != 0 && \
	(a)->ia6_lifetime.ia6ti_expire < (t))
#endif             

#ifdef PRIVATE
struct route_in6_old {
	void            *ro_rt;
	uint32_t        ro_flags;
	struct sockaddr_in6 ro_dst;
};
#endif              

#ifdef BSD_KERNEL_PRIVATE
#include <net/if_llatbl.h>
#include <sys/eventhandler.h>

  
                      
  
                                                            
                                                         
                                       
   
struct route_in6 {
	  
                                                             
                                                              
                          
    
	struct rtentry  *ro_rt;
	struct  llentry *ro_lle;

	struct ifaddr   *ro_srcia;
	uint32_t        ro_flags;                        
	struct sockaddr_in6 ro_dst;
};
#endif                         

  
                                                        
                                                             
   
                
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
  
                                                                        
                 
                 
                  
                 
                 
                 
               
  
                                                             
                                                           
                           
  
                                                    
                                                           
                           
  
                                                   
                                         
   
#ifdef BSD_KERNEL_PRIVATE
#define __APPLE_USE_RFC_3542 1
#endif                         

#if defined(__APPLE_USE_RFC_3542) && defined(__APPLE_USE_RFC_2292)
#error "__APPLE_USE_RFC_3542 and __APPLE_USE_RFC_2292 cannot be both defined"
#endif

#if 0                                                              
#define IPV6_OPTIONS            1                                         
#define IPV6_RECVOPTS           5                                          
#define IPV6_RECVRETOPTS        6                                           
#define IPV6_RECVDSTADDR        7                                          
#define IPV6_RETOPTS            8                                     
#endif        
#define IPV6_SOCKOPT_RESERVED1  3                               
#endif                                             
#define IPV6_UNICAST_HOPS       4                     
#define IPV6_MULTICAST_IF       9                                         
#define IPV6_MULTICAST_HOPS     10                                      
#define IPV6_MULTICAST_LOOP     11                                        
#define IPV6_JOIN_GROUP         12                                        
#define IPV6_LEAVE_GROUP        13                                         

#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#define IPV6_PORTRANGE          14                                           
#define ICMP6_FILTER            18                                 
#define IPV6_2292PKTINFO        19                                       
#define IPV6_2292HOPLIMIT       20                      
#define IPV6_2292NEXTHOP        21                          
#define IPV6_2292HOPOPTS        22                              
#define IPV6_2292DSTOPTS        23                              
#define IPV6_2292RTHDR          24                                

                                                              
#define IPV6_2292PKTOPTIONS     25

#ifdef __APPLE_USE_RFC_2292
#define IPV6_PKTINFO    IPV6_2292PKTINFO
#define IPV6_HOPLIMIT   IPV6_2292HOPLIMIT
#define IPV6_NEXTHOP    IPV6_2292NEXTHOP
#define IPV6_HOPOPTS    IPV6_2292HOPOPTS
#define IPV6_DSTOPTS    IPV6_2292DSTOPTS
#define IPV6_RTHDR      IPV6_2292RTHDR
#define IPV6_PKTOPTIONS IPV6_2292PKTOPTIONS
#endif                           

#define IPV6_CHECKSUM           26                                          
#endif                                             
#define IPV6_V6ONLY             27                                             
#if !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
#ifndef KERNEL
#define IPV6_BINDV6ONLY         IPV6_V6ONLY
#endif             


#if 1            
#define IPV6_IPSEC_POLICY       28                                      
#endif        
#define IPV6_FAITH              29                 

#if 1                   
#define IPV6_FW_ADD             30                                   
#define IPV6_FW_DEL             31                                        
#define IPV6_FW_FLUSH           32                                
#define IPV6_FW_ZERO            33                                           
#define IPV6_FW_GET             34                                     
#endif        

  
                                                                  
                                                                     
                                                           
   
#define IPV6_RECVTCLASS         35                                      
#define IPV6_TCLASS             36                                    

#ifdef __APPLE_USE_RFC_3542
                                              
  
                                         
                                                              
                                                           
                         
   
#define IPV6_RTHDRDSTOPTS       57

  
                          
                                                             
                                                       
                         
   
#define IPV6_RECVPKTINFO        61

#define IPV6_RECVHOPLIMIT       37                           
#define IPV6_RECVRTHDR          38                                
#define IPV6_RECVHOPOPTS        39                                   
#define IPV6_RECVDSTOPTS        40                                        
#ifdef KERNEL
#define IPV6_RECVRTHDRDSTOPTS   41                                         
#endif

#define IPV6_USE_MIN_MTU        42                                            
#define IPV6_RECVPATHMTU        43                                    

  
                                                         
                          
   
#define IPV6_PATHMTU            44

#if 0                                       
                                                                    
#define IPV6_REACHCONF          45
#endif
                                                   
#define IPV6_3542PKTINFO        46                                     
#define IPV6_3542HOPLIMIT       47                          
#define IPV6_3542NEXTHOP        48                              
#define IPV6_3542HOPOPTS        49                                      
#define IPV6_3542DSTOPTS        50                                            
#define IPV6_3542RTHDR          51                                     

#define IPV6_PKTINFO    IPV6_3542PKTINFO
#define IPV6_HOPLIMIT   IPV6_3542HOPLIMIT
#define IPV6_NEXTHOP    IPV6_3542NEXTHOP
#define IPV6_HOPOPTS    IPV6_3542HOPOPTS
#define IPV6_DSTOPTS    IPV6_3542DSTOPTS
#define IPV6_RTHDR      IPV6_3542RTHDR

#define IPV6_AUTOFLOWLABEL      59                                           

#define IPV6_DONTFRAG           62                                       

                                                            
#define IPV6_PREFER_TEMPADDR    63

  
                                                                         
                                                               
   
#define IPV6_MSFILTER           74                            
#endif                           

#define IPV6_BOUND_IF           125                                   

#ifdef PRIVATE
#define IPV6_NO_IFT_CELLULAR    6969                            
#define IPV6_OUT_IF             9696                            
#endif              

                                                                               

#define IPV6_RTHDR_LOOSE        0                                       
#define IPV6_RTHDR_STRICT       1                                   
#define IPV6_RTHDR_TYPE_0       0                                 

  
                                  
   
#define IPV6_DEFAULT_MULTICAST_HOPS 1                                         
#define IPV6_DEFAULT_MULTICAST_LOOP 1                                         

  
                                                                             
                                                                        
                                         
   
#define IPV6_MIN_MEMBERSHIPS    31
#define IPV6_MAX_MEMBERSHIPS    4095

  
                                                               
                                   
   
#define IPV6_MAX_GROUP_SRC_FILTER       512                            
#define IPV6_MAX_SOCK_SRC_FILTER        128                                   

  
                                                               
   
struct ipv6_mreq {
	struct in6_addr ipv6mr_multiaddr;
	unsigned int    ipv6mr_interface;
};

  
                                                      
   
struct in6_pktinfo {
	struct in6_addr ipi6_addr;                                
	unsigned int    ipi6_ifindex;                                  
};

  
                                                        
   
struct ip6_mtuinfo {
	struct sockaddr_in6 ip6m_addr;                            
	uint32_t ip6m_mtu;
};

  
                               
                                                                          
   
#define IPV6_PORTRANGE_DEFAULT  0                          
#define IPV6_PORTRANGE_HIGH     1                                             
#define IPV6_PORTRANGE_LOW      2                                       

  
                                           
  
                                  
                                                         
   
#define IPV6PROTO_MAXID (IPPROTO_PIM + 1)                                   

  
                              
   
#define IPV6CTL_FORWARDING      1                          
#define IPV6CTL_SENDREDIRECTS   2                                               
#define IPV6CTL_DEFHLIM         3                              
#ifdef notyet
#define IPV6CTL_DEFMTU          4                        
#endif
#define IPV6CTL_FORWSRCRT       5                                         
#define IPV6CTL_STATS           6                  
#define IPV6CTL_MRTSTATS        7                                       
#define IPV6CTL_MRTPROTO        8                                       
#define IPV6CTL_MAXFRAGPACKETS  9                                         
#define IPV6CTL_SOURCECHECK     10                                        
#define IPV6CTL_SOURCECHECK_LOGINT 11                                  
#define IPV6CTL_ACCEPT_RTADV    12
#define IPV6CTL_KEEPFAITH       13                      
#define IPV6CTL_LOG_INTERVAL    14
#define IPV6CTL_HDRNESTLIMIT    15
#define IPV6CTL_DAD_COUNT       16
#define IPV6CTL_AUTO_FLOWLABEL  17
#define IPV6CTL_DEFMCASTHLIM    18
#define IPV6CTL_GIF_HLIM        19                                             
#define IPV6CTL_KAME_VERSION    20
#define IPV6CTL_USE_DEPRECATED  21                                           
#define IPV6CTL_RR_PRUNE        22                                             
#if 0                 
#define IPV6CTL_MAPPED_ADDR     23
#endif
#define IPV6CTL_V6ONLY          24
#define IPV6CTL_RTEXPIRE        25                                        
#define IPV6CTL_RTMINEXPIRE     26                                         
#define IPV6CTL_RTMAXCACHE      27                                            

#define IPV6CTL_USETEMPADDR     32                                              
#define IPV6CTL_TEMPPLTIME      33                                           
#define IPV6CTL_TEMPVLTIME      34                                       
#define IPV6CTL_AUTO_LINKLOCAL  35                                            
#define IPV6CTL_RIP6STATS       36                         
#define IPV6CTL_PREFER_TEMPADDR 37                                        
#define IPV6CTL_ADDRCTLPOLICY   38                                            
#define IPV6CTL_USE_DEFAULTZONE 39                                  

#define IPV6CTL_MAXFRAGS        41                         
#define IPV6CTL_MCAST_PMTU      44                                            

#define IPV6CTL_NEIGHBORGCTHRESH 46
#define IPV6CTL_MAXIFPREFIXES   47
#define IPV6CTL_MAXIFDEFROUTERS 48
#define IPV6CTL_MAXDYNROUTES    49
#define ICMPV6CTL_ND6_ONLINKNSRFC4861   50
#define IPV6CTL_ULA_USETEMPADDR 51


                                                                        
                                                                               
#define IPV6CTL_MAXID           51

#ifdef BSD_KERNEL_PRIVATE
#define CTL_IPV6PROTO_NAMES { \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, \
	{ "tcp6", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "udp6", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, \
	{ "ip6", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, \
	{ "ipsec6", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "icmp6", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
}
  
                             
   
#define M_AUTHIPHDR     M_PROTO2
#define M_DECRYPTED     M_PROTO3
#define M_AUTHIPDGM     M_PROTO5

struct cmsghdr;
struct mbuf;
struct ifnet;
struct in6_aliasreq;
struct lltable;

extern struct lltable * in6_lltattach(struct ifnet *ifp);
extern uint16_t in6_pseudo(const struct in6_addr *, const struct in6_addr *,
    uint32_t);
extern u_int16_t inet6_cksum(struct mbuf *, uint32_t, uint32_t, uint32_t);
extern u_int16_t inet6_cksum_buffer(const uint8_t *, uint32_t, uint32_t,
    uint32_t);

#define in6_cksum(_m, _n, _o, _l)                       \
	inet6_cksum(_m, _n, _o, _l)
#define in6_cksum_buffer(_b, _n, _o, _l)                \
	inet6_cksum_buffer(_b, _n, _o, _l)

extern int in6_addrscope(struct in6_addr *);
extern struct in6_ifaddr *in6_ifawithscope(struct ifnet *, struct in6_addr *);
extern struct in6_ifaddr *in6_ifawithifp(struct ifnet *, struct in6_addr *);

struct sockaddr;

extern void in6_sin6_2_sin(struct sockaddr_in *sin, struct sockaddr_in6 *sin6);
extern void in6_sin_2_v4mapsin6(struct sockaddr_in *sin,
    struct sockaddr_in6 *sin6);
extern void in6_sin6_2_sin_in_sock(struct sockaddr *nam);
extern int in6_sin_2_v4mapsin6_in_sock(struct sockaddr **nam);

extern uint32_t in6_finalize_cksum(struct mbuf *, uint32_t, int32_t,
    int32_t, uint32_t);

#define in6_delayed_cksum(_m)                   \
	((void) in6_finalize_cksum(_m, 0, 0, -1, CSUM_DELAY_IPV6_DATA))
#define in6_delayed_cksum_offset(_m, _o, _s, _p)        \
	((void) in6_finalize_cksum(_m, _o, _s, _p, CSUM_DELAY_IPV6_DATA))

                          
extern struct eventhandler_lists_ctxt in6_evhdlr_ctxt;
  
                                              
                                            
                                                     
                               
   
typedef enum {
	                    
	  
                                                   
                                                         
                                                         
                  
    
	IN6_ADDR_MARKED_DUPLICATED,
	IN6_ADDR_MARKED_DETACHED,
	IN6_ADDR_MARKED_DEPRECATED,

	                   
	IN6_NDP_RTR_EXPIRY,
	IN6_NDP_PFX_EXPIRY,
	IN6_NDP_ADDR_EXPIRY,

	                                                      
	         
	IN6_EVENT_MAX,
} in6_evhdlr_code_t;

struct in6_event2kev {
	in6_evhdlr_code_t       in6_event_code;
	uint32_t                in6_event_kev_subclass;
	uint32_t                in6_event_kev_code;
	const char              *in6_event_str;
};
extern struct in6_event2kev in6_event2kev_array[];
extern void in6_eventhdlr_callback(struct eventhandler_entry_arg, in6_evhdlr_code_t,
    struct ifnet *, struct in6_addr *, uint32_t);
extern void in6_event_enqueue_nwk_wq_entry(in6_evhdlr_code_t,
    struct ifnet *, struct in6_addr *, uint32_t);

typedef void (*in6_event_fn) (struct eventhandler_entry_arg, in6_evhdlr_code_t,
    struct ifnet *, struct in6_addr *, uint32_t);
EVENTHANDLER_DECLARE(in6_event, in6_event_fn);
#endif                         

#ifdef PRIVATE
                   
typedef enum in6_clat46_evhdlr_code_t {
	IN6_CLAT46_EVENT_V4_FLOW,
	IN6_CLAT46_EVENT_V6_ADDR_CONFFAIL,
} in6_clat46_evhdlr_code_t;

struct kev_netevent_clat46_data {
	in6_clat46_evhdlr_code_t clat46_event_code;
	pid_t epid;
	uuid_t euuid;
};
#endif              

#ifdef BSD_KERNEL_PRIVATE
                   
extern struct eventhandler_lists_ctxt in6_clat46_evhdlr_ctxt;
extern void in6_clat46_eventhdlr_callback(struct eventhandler_entry_arg,
    in6_clat46_evhdlr_code_t, pid_t, uuid_t);
extern void in6_clat46_event_enqueue_nwk_wq_entry(in6_clat46_evhdlr_code_t,
    pid_t, uuid_t);

typedef void (*in6_clat46_event_fn) (struct eventhandler_entry_arg, in6_clat46_evhdlr_code_t,
    pid_t, uuid_t);
EVENTHANDLER_DECLARE(in6_clat46_event, in6_clat46_event_fn);
#endif                         

#ifdef KERNEL_PRIVATE
                                     
extern int in6_localaddr(struct in6_addr *);
extern int in6addr_local(struct in6_addr *);
#endif                     

#ifndef KERNEL
__BEGIN_DECLS
struct cmsghdr;

extern int inet6_option_space(int);
extern int inet6_option_init(void *, struct cmsghdr **, int);
extern int inet6_option_append(struct cmsghdr *, const __uint8_t *, int, int);
extern __uint8_t *inet6_option_alloc(struct cmsghdr *, int, int, int);
extern int inet6_option_next(const struct cmsghdr *, __uint8_t **);
extern int inet6_option_find(const struct cmsghdr *, __uint8_t **, int);

extern size_t inet6_rthdr_space(int, int);
extern struct cmsghdr *inet6_rthdr_init(void *, int);
extern int inet6_rthdr_add(struct cmsghdr *, const struct in6_addr *,
    unsigned int);
extern int inet6_rthdr_lasthop(struct cmsghdr *, unsigned int);
#if 0                          
extern int inet6_rthdr_reverse(const struct cmsghdr *, struct cmsghdr *);
#endif
extern int inet6_rthdr_segments(const struct cmsghdr *);
extern struct in6_addr *inet6_rthdr_getaddr(struct cmsghdr *, int);
extern int inet6_rthdr_getflags(const struct cmsghdr *, int);

extern int inet6_opt_init(void *, socklen_t);
extern int inet6_opt_append(void *, socklen_t, int, __uint8_t, socklen_t,
    __uint8_t, void **);
extern int inet6_opt_finish(void *, socklen_t, int);
extern int inet6_opt_set_val(void *, int, void *, socklen_t);

extern int inet6_opt_next(void *, socklen_t, int, __uint8_t *, socklen_t *,
    void **);
extern int inet6_opt_find(void *, socklen_t, int, __uint8_t, socklen_t *,
    void **);
extern int inet6_opt_get_val(void *, int, void *, socklen_t);
extern socklen_t inet6_rth_space(int, int);
extern void *inet6_rth_init(void *, socklen_t, int, int);
extern int inet6_rth_add(void *, const struct in6_addr *);
extern int inet6_rth_reverse(const void *, void *);
extern int inet6_rth_segments(const void *);
extern struct in6_addr *inet6_rth_getaddr(const void *, int);

__END_DECLS
#endif              
#endif                         
#endif                                             
#endif                        
