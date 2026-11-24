/*
 * Copyright (c) 2004-2018 Apple Inc. All rights reserved.
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
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Scooter Morris at Genentech Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *	@(#)lockf.h	8.1 (Berkeley) 6/11/93
 * $FreeBSD: src/sys/sys/lockf.h,v 1.16 2004/04/07 04:19:49 imp Exp $
 */

#ifndef _SYS_LOCKF_H_
#define _SYS_LOCKF_H_

#include <sys/queue.h>
#include <sys/cdefs.h>

struct vnop_advlock_args;
struct vnode;

#if IMPORTANCE_INHERITANCE
#define LF_NOT_BOOSTED  0
#define LF_BOOSTED      1
#endif                             

  
                                                                           
                                                                           
                                                                              
                                                                               
                                                                                
   
TAILQ_HEAD(locklist, lockf);

struct lockf {
	short   lf_flags;                                                    
	short   lf_type;                                             
#if IMPORTANCE_INHERITANCE
	int     lf_boosted;                                               
#endif
	off_t   lf_start;                                                
	off_t   lf_end;                                                         
	caddr_t lf_id;                                                       
	struct  lockf **lf_head;                                                   
	struct  vnode *lf_vnode;                                   
	struct  lockf *lf_next;                                                 
	struct  locklist lf_blkhd;                                             
	TAILQ_ENTRY(lockf) lf_block;                                  
	struct  proc *lf_owner;                                                
};

__BEGIN_DECLS

#ifdef KERNEL_PRIVATE
void    lf_init(void);
int     lf_advlock(struct vnop_advlock_args *);
int     lf_assert(struct vnop_advlock_args *, void **);
void    lf_commit(void *, int);
void    lf_abort_advlocks(vnode_t);

#ifdef LOCKF_DEBUG
void    lf_print(char *, struct lockf *);
void    lf_printlist(char *, struct lockf *);
#endif
#endif                     

__END_DECLS

#endif                     
