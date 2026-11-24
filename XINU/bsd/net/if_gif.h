/*
 * Copyright (c) 2000-2018 Apple Inc. All rights reserved.
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

  
           
   

#ifndef _NET_IF_GIF_H_
#define _NET_IF_GIF_H_
#include <sys/appleapiopts.h>

#include <netinet/in.h>
                                                               

#ifdef BSD_KERNEL_PRIVATE

extern void gif_init(void);

struct encaptab;

struct gif_softc {
	ifnet_t                 gif_if;                                       
	struct sockaddr *gif_psrc;                        
	struct sockaddr *gif_pdst;                        
#ifdef __APPLE__
	protocol_family_t gif_proto;                             
#endif
	union {
		struct route  gifscr_ro;             
		struct route_in6 gifscr_ro6;          
	} gifsc_gifscr;
	int             gif_flags;
#define IFGIF_DETACHING 0x1
	int             gif_called;
	const struct encaptab *encap_cookie4;
	const struct encaptab *encap_cookie6;
	TAILQ_ENTRY(gif_softc) gif_link;                           
	bpf_tap_mode    tap_mode;
	bpf_packet_func tap_callback;
	char    gif_ifname[IFNAMSIZ];
	decl_lck_mtx_data(, gif_lock);                                    
};

#define GIF_LOCK(_sc)           lck_mtx_lock(&(_sc)->gif_lock)
#define GIF_UNLOCK(_sc)         lck_mtx_unlock(&(_sc)->gif_lock)
#define GIF_LOCK_ASSERT(_sc)    LCK_MTX_ASSERT(&(_sc)->gif_lock,        \
    LCK_MTX_ASSERT_OWNED)

#define gif_ro gifsc_gifscr.gifscr_ro
#define gif_ro6 gifsc_gifscr.gifscr_ro6

#endif                         

#define GIF_MTU         (1280)                   
#define GIF_MTU_MIN     (1280)                   
#define GIF_MTU_MAX     (8192)                   

#endif                     
