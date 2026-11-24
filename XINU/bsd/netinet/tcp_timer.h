/*
 * Copyright (c) 2000-2014 Apple Computer, Inc. All rights reserved.
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
 * Copyright (c) 1982, 1986, 1993
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
 *	@(#)tcp_timer.h	8.1 (Berkeley) 6/10/93
 * $FreeBSD: src/sys/netinet/tcp_timer.h,v 1.18 1999/12/29 04:41:03 peter Exp $
 */

#ifndef _NETINET_TCP_TIMER_H_
#define _NETINET_TCP_TIMER_H_
#include <sys/appleapiopts.h>

#ifdef BSD_KERNEL_PRIVATE
#include <kern/thread_call.h>
#endif                         

                                                                    
#define TCPT_NTIMERS_EXT        4

  
                                 
  
                                                                           
                                                                           
                                                                          
                                                                          
                                                                          
                                                
  
                                                         
                                                         
                                                         
                                                               
                                                             
                                                             
                                                                    
                                                             
                           
  
                                                                          
                                                                      
  
                                                                 
                                                                       
                                                                            
                                                                        
                                                                      
                                                                       
                                                                        
                                                              
                                                                 
                                                                      
                                 
  
                                                                
                                                                       
                                                                 
                                                                    
                                                                         
                                                                    
                      
                                        
                                                                       
                                                                       
                                                                
                                                                    
  
                                                                            
                                                                        
   
#ifdef BSD_KERNEL_PRIVATE

#define TCPT_PTO        0                          
#define TCPT_DELAYFR    1                                                  
#define TCPT_REXMT      2                       
#define TCPT_DELACK     3                        
#define TCPT_PERSIST    4                                   
#define TCPT_KEEP       5                       
#define TCPT_2MSL       6                                   
#if MPTCP
#define TCPT_JACK_RXMT  7                                          
#define TCPT_CELLICON   8                                             
#define TCPT_MAX        8
#else            
#define TCPT_MAX        6
#endif             

#define TCPT_NONE       (TCPT_MAX + 1)
#define TCPT_NTIMERS    (TCPT_MAX + 1)

                          
#define TCPT_REXMT_EXT          0
#define TCPT_PERSIST_EXT        1
#define TCPT_KEEP_EXT           2
#define TCPT_2MSL_EXT           3
#define TCPT_DELACK_EXT         4

#else                          
#define TCPT_REXMT      0                               
#define TCPT_PERSIST    1                                           
#define TCPT_KEEP       2                               
#define TCPT_2MSL       3                                           
#define TCPT_DELACK     4                                      
#if MPTCP
#define TCPT_JACK_RXMT  5                                          
#define TCPT_MAX        5
#else            
#define TCPT_MAX        4
#endif             
#define TCPT_NONE       (TCPT_MAX + 1)
#define TCPT_NTIMERS    (TCPT_MAX + 1)

#endif                         

#ifdef BSD_KERNEL_PRIVATE
  
                  
   
#define TCPTV_MSL       ( 15*TCP_RETRANSHZ)                           
#define TCPTV_SRTTBASE  0                                                   
#define TCPTV_RTOBASE   (  1*TCP_RETRANSHZ)                                 
#define TCPTV_SRTTDFLT  (  1*TCP_RETRANSHZ)                                 
#define TCPTV_PERSMIN   (  5*TCP_RETRANSHZ)                                 
#define TCPTV_PERSMAX   ( 60*TCP_RETRANSHZ)                                   

extern int tcptv_persmin_val;

#define TCPTV_KEEP_INIT ( 75*TCP_RETRANSHZ)                             
#define TCPTV_KEEP_IDLE (120*60*TCP_RETRANSHZ)                           
#define TCPTV_KEEPINTVL ( 75*TCP_RETRANSHZ)                                 
#define TCPTV_KEEPCNT   8                                                   

#define TCPTV_REXMTMAX  ( 64*TCP_RETRANSHZ )                         
#define TCPTV_REXMTMIN  ( TCP_RETRANSHZ/33 )                                             

  
                                                              
                             
   
#define TCPTV_UNACKWIN  ( TCP_RETRANSHZ/10 )

                                                                   
#define TCPTV_MAXRCVIDLE (TCP_RETRANSHZ/5 )

  
                                                                  
                                                              
                                                      
                          
   
#define TCP_RCV_SS_PKTCOUNT     512

#define TCPTV_TWTRUNC   8                                              

#define TCP_LINGERTIME  120                                           

#define TCP_MAXRXTSHIFT 12                                       

#ifdef  TCPTIMERS
static char *tcptimers[] =
{ "REXMT", "PERSIST", "KEEP", "2MSL", "DELACK"};
#endif                

  
                                                                          
                                                 
  
                                                                       
                         
  
                                                                      
   
#define IS_TIMER_HZ_500MS(i)    ((i) >= TCPT_PERSIST)
#define IS_TIMER_HZ_100MS(i)    ((i) >= TCPT_REXMT && (i) < TCPT_PERSIST)
#define IS_TIMER_HZ_10MS(i)     ((i) < TCPT_REXMT)

struct tcptimerlist;

struct tcptimerentry {
	LIST_ENTRY(tcptimerentry) le;                             
	uint32_t timer_start;                                             
	uint16_t index;                                                            
	uint16_t mode;                                                     
	uint32_t runtime;                                                          
};

LIST_HEAD(timerlisthead, tcptimerentry);

struct tcptimerlist {
	struct timerlisthead lhead;                           
	lck_mtx_t *mtx;                                       
	lck_attr_t *mtx_attr;                         
	lck_grp_t *mtx_grp;                                 
	lck_grp_attr_t *mtx_grp_attr;                               
	thread_call_t call;                     
	uint32_t runtime;                                                    
	uint32_t schedtime;                                                
	uint32_t entries;                                          
	uint32_t maxentries;                                           

	                                              
	boolean_t running;                                                  
	boolean_t scheduled;                                         
#define TCP_TIMERLIST_10MS_MODE 0x1
#define TCP_TIMERLIST_100MS_MODE 0x2
#define TCP_TIMERLIST_500MS_MODE 0x4
	uint32_t mode;                                         
	uint32_t pref_mode;                                             
	uint32_t pref_offset;                                             
	uint32_t idleruns;                                                               
	struct tcptimerentry *next_te;                                           
	u_int16_t probe_if_index;                                                
};

                                                                           
#define TCP_FASTMODE_IDLERUN_MAX 10

  
                                                              
                                                           
                                                                
                                                                  
                                                               
                         
  
                                                               
                                                                    
                                                                        
                                                                       
                                                                     
             
   
#define TCPTV_REXMTSLOP ( TCP_RETRANSHZ/5 )                            

                                                                            
#define TCP_ADD_REXMTSLOP(tp) (tp->t_state >= TCPS_ESTABLISHED)

#define TCPT_RANGESET(tv, value, tvmin, tvmax, addslop) do { \
	(tv) = ((addslop) ? tcp_rexmt_slop : 0) + (value); \
	if ((uint32_t)(tv) < (uint32_t)(tvmin)) \
	        (tv) = (tvmin); \
	else if ((uint32_t)(tv) > (uint32_t)(tvmax)) \
	        (tv) = (tvmax); \
} while(0)

#define TCP_CONN_KEEPIDLE(tp) \
	((tp)->t_keepidle && \
	((tp)->t_inpcb->inp_socket->so_options & SO_KEEPALIVE) ? \
	        (tp)->t_keepidle : (uint32_t)tcp_keepidle)
#define TCP_CONN_KEEPINIT(tp) \
	(((tp)->t_keepinit > 0) ? (tp)->t_keepinit : (uint32_t)tcp_keepinit)
#define TCP_CONN_KEEPCNT(tp) \
	(((tp)->t_keepcnt > 0) ? (tp)->t_keepcnt : (uint32_t)tcp_keepcnt)
#define TCP_CONN_KEEPINTVL(tp) \
	(((tp)->t_keepintvl > 0) ? (tp)->t_keepintvl : (uint32_t)tcp_keepintvl)
#define TCP_CONN_MAXIDLE(tp) \
	(TCP_CONN_KEEPCNT(tp) * TCP_CONN_KEEPINTVL(tp))

#define TCP_IDLETIMEOUT(tp) \
	(((TCP_ADD_REXMTSLOP(tp)) ? 0 : tcp_rexmt_slop) + tp->t_rxtcur)

TAILQ_HEAD(tcptailq, tcpcb);

extern int tcp_keepinit;                                          
extern int tcp_keepidle;                                                
extern int tcp_keepintvl;                                          
extern int tcp_keepcnt;                                   
extern int tcp_delack;                                 
extern int tcp_maxpersistidle;
extern int tcp_msl;
extern int tcp_ttl;                                            
extern int tcp_backoff[];
extern int tcp_rexmt_slop;
extern u_int32_t tcp_max_persist_timeout;                                                       

#define OFFSET_FROM_START(tp, off) ((tcp_now + (off)) - (tp)->tentry.timer_start)

#endif                         
#endif                             
