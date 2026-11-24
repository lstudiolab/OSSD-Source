/*
 * Copyright (c) 2000-2011 Apple Inc. All rights reserved.
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
 * Copyright (c) 1990, 1993
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
 *	@(#)if_dl.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD: src/sys/net/if_dl.h,v 1.10 2000/03/01 02:46:25 archie Exp $
 */

#ifndef _NET_IF_DL_H_
#define _NET_IF_DL_H_
#include <sys/appleapiopts.h>

#include <sys/types.h>

#ifdef BSD_KERNEL_PRIVATE
#define DLIL_SDLMAXLEN  64
#endif                         

  
                                                                
                                                                    
                                                                         
                                                     
  
                                                                          
                                     
  
                                                                         
                                                                           
                                                               
  
                                                                            
                                                                            
                                                                            
   

  
                                      
   
struct sockaddr_dl {
	u_char  sdl_len;                                      
	u_char  sdl_family;                  
	u_short sdl_index;                                                     
	u_char  sdl_type;                           
	u_char  sdl_nlen;                                                       
	u_char  sdl_alen;                                      
	u_char  sdl_slen;                                       
	char    sdl_data[12];                                       
                                                                    
#ifndef __APPLE__
	                   
	u_short sdl_rcf;                                    
	u_short sdl_route[16];                                  
#endif
};

#define LLADDR(s) ((caddr_t)((s)->sdl_data + (s)->sdl_nlen))
#ifdef KERNEL_PRIVATE
#define CONST_LLADDR(s) ((const u_char*)((s)->sdl_data + (s)->sdl_nlen))
#endif

#ifdef BSD_KERNEL_PRIVATE
#define SDL(s) ((struct sockaddr_dl *)(void *)s)
#endif

#ifndef KERNEL

#include <sys/cdefs.h>

__BEGIN_DECLS
void    link_addr(const char *, struct sockaddr_dl *);
char    *link_ntoa(const struct sockaddr_dl *);
__END_DECLS

#endif              

#endif
