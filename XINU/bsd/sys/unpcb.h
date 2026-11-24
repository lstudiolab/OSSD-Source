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
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)unpcb.h	8.1 (Berkeley) 6/2/93
 */

#ifndef _SYS_UNPCB_H_
#define _SYS_UNPCB_H_

#include <sys/appleapiopts.h>
#include <sys/queue.h>
#include <sys/un.h>
#include <sys/ucred.h>
#include <sys/socketvar.h>
#if !KERNEL && PRIVATE
#include <TargetConditionals.h>
#endif

  
                                       
                                        
  
                                                  
                                                   
                                                           
                             
  
                                                        
                                                                
                        
  
                                                                  
                                                                 
                                                              
                                                                         
                                                                    
                                                            
                                                               
  
                                                                   
                                                           
                                           
   

typedef u_quad_t        unp_gen_t;

#if defined(__LP64__)
struct _unpcb_list_entry {
	u_int32_t   le_next;
	u_int32_t   le_prev;
};
#define _UCPCB_LIST_HEAD(name, type)    \
struct name {                           \
	u_int32_t	lh_first;       \
};
#define _UNPCB_LIST_ENTRY(x)            struct _unpcb_list_entry
#define _UNPCB_PTR(x)                   u_int32_t
#else
#define _UCPCB_LIST_HEAD(name, type)    LIST_HEAD(name, type)
#define _UNPCB_LIST_ENTRY(x)            LIST_ENTRY(x)
#define _UNPCB_PTR(x)                   x
#endif

#ifdef PRIVATE
#ifndef KERNEL
_UCPCB_LIST_HEAD(unp_head, unpcb);
#else
LIST_HEAD(unp_head, unpcb);
#define sotounpcb(so)   ((struct unpcb *)((so)->so_pcb))

struct  unpcb {
	LIST_ENTRY(unpcb) unp_link;                                   
	struct socket   *unp_socket;                                
	struct vnode    *unp_vnode;                                  
	ino_t                   unp_ino;                               
	struct unpcb    *unp_conn;                                             
	struct unp_head unp_refs;                                           
	LIST_ENTRY(unpcb) unp_reflink;                             
	struct sockaddr_un *unp_addr;                                
	int             unp_cc;                                
	int             unp_mbcnt;                                
	unp_gen_t       unp_gencnt;                                            
	int             unp_flags;                 
	struct xucred   unp_peercred;                                        
	decl_lck_mtx_data(, unp_mtx);                       
	int             rw_thrcount;                                                              
};
#endif             

  
                      
  
                                                                   
                                                                     
                                                                  
       
  
                                                                      
                                                                   
                                                                    
                                                                   
   
#define UNP_HAVEPC                      0x0001
#define UNP_HAVEPCCACHED                0x0002
#define UNP_DONTDISCONNECT              0x0004
#define UNP_TRACE_MDNS                  0x1000

#ifdef KERNEL
struct  unpcb_compat {
#else             
#define unpcb_compat unpcb
struct  unpcb {
#endif             
	_UNPCB_LIST_ENTRY(unpcb_compat) unp_link;                                     
	_UNPCB_PTR(struct socket *)     unp_socket;                                 
	_UNPCB_PTR(struct vnode *)      unp_vnode;                                   
	u_int32_t                       unp_ino;                               
	_UNPCB_PTR(struct unpcb_compat *) unp_conn;                                            
#if defined(KERNEL)
	u_int32_t                       unp_refs;
#else
	struct unp_head                 unp_refs;                                           
#endif
	_UNPCB_LIST_ENTRY(unpcb_compat) unp_reflink;                               
	_UNPCB_PTR(struct sockaddr_un *) unp_addr;                                   
	int                             unp_cc;                                
	int                             unp_mbcnt;                                
	unp_gen_t                       unp_gencnt;                                            
};

                                                                
#ifdef  _SYS_SOCKETVAR_H_

#pragma pack(4)

struct  xunpcb {
	u_int32_t                       xu_len;                                       
	_UNPCB_PTR(struct unpcb_compat *) xu_unpp;                                  
	struct unpcb_compat             xu_unp;                              
	union {
		struct sockaddr_un      xuu_addr;                              
		char                    xu_dummy1[256];
	} xu_au;
#define xu_addr xu_au.xuu_addr
	union {
		struct sockaddr_un      xuu_caddr;                               
		char                    xu_dummy2[256];
	} xu_cau;
#define xu_caddr xu_cau.xuu_caddr
	struct xsocket                  xu_socket;
	u_quad_t                        xu_alignment_hack;
};

#if XNU_TARGET_OS_OSX || KERNEL || !(TARGET_OS_IPHONE && !TARGET_OS_SIMULATOR)

struct xunpcb64_list_entry {
	u_int64_t   le_next;
	u_int64_t   le_prev;
};

struct xunpcb64 {
	u_int32_t                       xu_len;                                       
	u_int64_t                       xu_unpp;                                    
	struct xunpcb64_list_entry      xunp_link;                                    
	u_int64_t                       xunp_socket;                                
	u_int64_t                       xunp_vnode;                                  
	u_int64_t                       xunp_ino;                              
	u_int64_t                       xunp_conn;                                             
	u_int64_t                       xunp_refs;                                          
	struct xunpcb64_list_entry      xunp_reflink;                              
	int                             xunp_cc;                                       
	int                             xunp_mbcnt;                               
	unp_gen_t                       xunp_gencnt;                                           
	int                             xunp_flags;                
	union {
		struct sockaddr_un              xuu_addr;
		char                            xu_dummy1[256];
	}                               xu_au;                                 
#define xunp_addr xu_au.xuu_addr
	union {
		struct sockaddr_un              xuu_caddr;
		char                            xu_dummy2[256];
	}                               xu_cau;                                  
#define xunp_caddr xu_cau.xuu_caddr
	struct xsocket64        xu_socket;
};

#endif                                                                                 

#pragma pack()

#endif                        

#endif              

struct  xunpgen {
	u_int32_t       xug_len;
	u_int           xug_count;
	unp_gen_t       xug_gen;
	so_gen_t        xug_sogen;
};

#endif                    
