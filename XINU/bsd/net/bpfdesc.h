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
 *  * OSSD MODIFICATION NOTICE: Modified by OSSD on 2026-11-24.
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * Copyright (c) 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.c) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence
 * Berkeley Laboratory.
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
 *      @(#)bpfdesc.h	8.1 (Berkeley) 6/10/93
 *
 * $FreeBSD: src/sys/net/bpfdesc.h,v 1.14.2.1 2000/03/19 05:55:36 rwatson Exp $
 */

#ifndef _NET_BPFDESC_H_
#define _NET_BPFDESC_H_
#include <sys/appleapiopts.h>
#ifdef KERNEL_PRIVATE
  
                                                                    
   

#include <sys/proc.h>
#include <sys/select.h>
#include <kern/thread_call.h>
#include <uuid/uuid.h>

  
                                                 
   
struct bpf_d {
	struct bpf_d    *bd_next;                                       
	  
                                                                
                                                          
                                                         
                                                           
                                                         
                                                        
                                                                   
    
	caddr_t         bd_sbuf;                        
	caddr_t         bd_hbuf;                       
	caddr_t         bd_fbuf;                       
	int             bd_slen;                                            
	int             bd_hlen;                                           
	u_int32_t       bd_scnt;                                               
	u_int32_t       bd_hcnt;                                              

	int             bd_bufsize;                                     
	int             bd_hbuf_read;                          
	int             bd_headdrop;                            

	struct bpf_if  *bd_bif;                                   
	u_int32_t       bd_rtout;                                    
	struct bpf_insn *bd_filter;                      
	u_int32_t       bd_rcount;                                      
	u_int32_t       bd_dcount;                                     

	u_char          bd_promisc;                                          
	u_char          bd_state;                                        
	u_char          bd_immediate;                                         
	int             bd_async;                                                                
	int             bd_sig;                                                   
#ifdef __APPLE__
	pid_t           bd_sigio;
#else
	struct sigio *  bd_sigio;                                      
#endif

#if BSD < 199103
	u_char          bd_selcoll;                                  
	int             bd_timedout;
	struct proc *   bd_selproc;                                        
#else
	u_char          bd_pad;                                 
	struct selinfo  bd_sel;                              
#endif
	int             bd_hdrcmplt;                                                   
	int             bd_seesent;                                              
	int             bd_oflags;                             
	thread_call_t   bd_thread_call;                                   
	int             bd_traffic_class;                            
	int             bd_flags;                  

	int             bd_refcnt;
#define BPF_REF_HIST    4                                                    
	void            *bd_ref_lr[BPF_REF_HIST];
	void            *bd_unref_lr[BPF_REF_HIST];
	int             bd_next_ref_lr;
	int             bd_next_unref_lr;

	struct proc     *bd_opened_by;
	uuid_t          bd_uuid;
};

                         
#define BPF_IDLE                0                                                 
#define BPF_WAITING             1                                                   
#define BPF_TIMED_OUT   2                                                       
#define BPF_DRAINING    3                                                                

                                             
#define bpf_ready(bd)   ((bd)->bd_hlen != 0 ||          \
	                 (((bd)->bd_immediate || (bd)->bd_state == BPF_TIMED_OUT) && \
	                  (bd)->bd_slen != 0))

                         
#define BPF_EXTENDED_HDR        0x0001                                        
#define BPF_WANT_PKTAP          0x0002                                     
#define BPF_FINALIZE_PKTAP      0x0004                                     
#define BPF_KNOTE               0x0008                            
#define BPF_DETACHING           0x0010                               
#define BPF_DETACHED            0x0020                         
#define BPF_CLOSING             0x0040                             
#define BPF_TRUNCATE            0x0080                                   
#define BPF_PKTHDRV2            0x0100                              

  
                                                               
   
struct bpf_if {
	struct bpf_if *bif_next;                                    
	struct bpf_d *bif_dlist;                             
	u_int bif_dlt;                                       
	u_int bif_hdrlen;                                                    
	u_int bif_exthdrlen;                                      
	struct ifnet *bif_ifp;                                       
	bpf_send_func   bif_send;
	bpf_tap_func    bif_tap;
};

#endif                     
#endif
