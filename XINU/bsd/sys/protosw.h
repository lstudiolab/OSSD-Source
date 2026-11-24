/*
 * Copyright (c) 2000-2019 Apple Inc. All rights reserved.
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
/* Copyright (c) 1998, 1999 Apple Computer, Inc. All Rights Reserved */
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
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
 *	@(#)protosw.h	8.1 (Berkeley) 6/2/93
 * $FreeBSD: src/sys/sys/protosw.h,v 1.28.2.2 2001/07/03 11:02:01 ume Exp $
 */

#ifndef _SYS_PROTOSW_H_
#define _SYS_PROTOSW_H_

#include <sys/appleapiopts.h>
#include <sys/cdefs.h>

                            
#define PR_SLOWHZ       2                                               

  
                                            
                                               
                                                                         
                                                                
   
#define PRC_IFDOWN              0                                 
#define PRC_ROUTEDEAD           1                                             
#define PRC_IFUP                2                                       
#define PRC_QUENCH2             3                                              
#define PRC_QUENCH              4                                       
#define PRC_MSGSIZE             5                                     
#define PRC_HOSTDEAD            6                                    
#define PRC_HOSTUNREACH         7                                              
#define PRC_UNREACH_NET         8                                
#define PRC_UNREACH_HOST        9                             
#define PRC_UNREACH_PROTOCOL    10                                 
#define PRC_UNREACH_PORT        11                      
                                                           
#define PRC_UNREACH_SRCFAIL     13                               
#define PRC_REDIRECT_NET        14                                
#define PRC_REDIRECT_HOST       15                                 
#define PRC_REDIRECT_TOSNET     16                                              
#define PRC_REDIRECT_TOSHOST    17                                   
#define PRC_TIMXCEED_INTRANS    18                                              
#define PRC_TIMXCEED_REASS      19                                       
#define PRC_PARAMPROB           20                            
#define PRC_UNREACH_ADMIN_PROHIB        21                                            

#define PRC_NCMDS               22

#define PRC_IS_REDIRECT(cmd)    \
	((cmd) >= PRC_REDIRECT_NET && (cmd) <= PRC_REDIRECT_TOSHOST)

#ifdef BSD_KERNEL_PRIVATE
#include <sys/eventhandler.h>
#endif

#ifdef KERNEL_PRIVATE
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/queue.h>
#include <kern/locks.h>

                                                                        
struct mbuf;
struct proc;
struct sockaddr;
struct socket;
struct sockopt;
struct socket_filter;
struct uio;
struct ifnet;
#ifdef XNU_KERNEL_PRIVATE
struct domain_old;
struct proc;
#endif                         

#pragma pack(4)

#ifdef XNU_KERNEL_PRIVATE
  
                                
  
                                                                          
                                                                              
                                                                          
                                                                          
                                                                          
   
struct protosw_old {
#else
struct protosw {
#endif                          
	short   pr_type;                                          
	struct  domain *pr_domain;                                       
	short   pr_protocol;                                 
	unsigned int pr_flags;                         
	  
                           
    
	void    (*pr_input)                                                 
	(struct mbuf *, int len);
	int     (*pr_output)                                                 
	(struct mbuf *m, struct socket *so);
	void    (*pr_ctlinput)                                          
	(int, struct sockaddr *, void *, struct ifnet *);
	int     (*pr_ctloutput)                                          
	(struct socket *, struct sockopt *);
	  
                      
    
	void    *pr_ousrreq;
	  
                 
    
	void    (*pr_init)(void);                                
	void    (*pr_unused)(void);                                            
	void    (*pr_unused2)(void);                                           
	void    (*pr_drain)(void);                                           
	int     (*pr_sysctl)                                     
	(int *, u_int, void *, size_t *, void *, size_t);
#ifdef XNU_KERNEL_PRIVATE
	struct  pr_usrreqs_old *pr_usrreqs;                             
#else
	struct  pr_usrreqs *pr_usrreqs;                             
#endif                          
	int     (*pr_lock)                                              
	(struct socket *so, int refcnt, void *debug);
	int     (*pr_unlock)                                     
	(struct socket *so, int refcnt, void *debug);
	lck_mtx_t *(*pr_getlock)                                    
	(struct socket *so, int flags);
	  
                 
    
	TAILQ_HEAD(, socket_filter) pr_filter_head;
#ifdef XNU_KERNEL_PRIVATE
	struct protosw_old *pr_next;                          
#else
	struct protosw *pr_next;                              
#endif                          
	u_int32_t       reserved[1];                                
};

#pragma pack()

#ifdef XNU_KERNEL_PRIVATE
  
                         
  
                                                                   
                                                                         
  
                                                                   
                                                                         
                                           
  
                                                                  
                                                                      
                                                                            
                                                                  
                                                                
                                                  
  
                                                                
                   
  
                                                                        
                                                                       
                                                                     
                                                                       
                                                                   
                                                                      
  
                                                              
   
struct protosw {
	TAILQ_ENTRY(protosw) pr_entry;                       
	struct  domain *pr_domain;                                       
	struct protosw *pr_protosw;                          
	u_int16_t pr_type;                                        
	u_int16_t pr_protocol;                               
	u_int32_t pr_flags;                            
	  
                           
    
	void    (*pr_input)                                                 
	(struct mbuf *, int len);
	int     (*pr_output)                                                 
	(struct mbuf *m, struct socket *so);
	void    (*pr_ctlinput)                                          
	(int, struct sockaddr *, void *, struct ifnet *);
	int     (*pr_ctloutput)                                          
	(struct socket *, struct sockopt *);
	  
                      
    
	struct  pr_usrreqs *pr_usrreqs;                                   
	  
                 
    
	void    (*pr_init)                                       
	(struct protosw *, struct domain *);
	void    (*pr_drain)(void);                                           
	int     (*pr_sysctl)                                     
	(int *, u_int, void *, size_t *, void *, size_t);
	int     (*pr_lock)                                              
	(struct socket *so, int refcnt, void *debug);
	int     (*pr_unlock)                                     
	(struct socket *so, int refcnt, void *debug);
	lck_mtx_t *(*pr_getlock)                                    
	(struct socket *so, int flags);
	  
        
    
	TAILQ_HEAD(, socket_filter) pr_filter_head;
	struct protosw_old *pr_old;

	void    (*pr_update_last_owner)                               
	(struct socket *so, struct proc *p, struct proc *ep);

	void    (*pr_copy_last_owner)                                     
	(struct socket *so, struct socket *head);
};

  
                                              
   
#define PR_F_WILLUNLOCK 0x01                                                              

#endif                         

  
                       
                              
                                                      
                                                                           
                                                                        
                                                                          
           
   
#define PR_ATOMIC       0x01                                       
#define PR_ADDR         0x02                                       
#define PR_CONNREQUIRED 0x04                                         
#define PR_WANTRCVD     0x08                             
#define PR_RIGHTS       0x10                             
#define PR_IMPLOPCL     0x20                            
#define PR_LASTHDR      0x40                                           
#define PR_PROTOLOCK    0x80                                                 
#define PR_PCBLOCK      0x100                                          
#define PR_DISPOSE      0x200                                              
#ifdef BSD_KERNEL_PRIVATE
#define PR_INITIALIZED  0x400                                      
#define PR_ATTACHED     0x800                                         
#define PR_MULTICONN    0x1000                                       
#define PR_EVCONNINFO   0x2000                                         
#define PR_PRECONN_WRITE        0x4000                                          
#define PR_DATA_IDEMPOTENT      0x8000                                                          
#define PR_OLD          0x10000000                              

                                
#define PRF_USERFLAGS                                                   \
	(PR_ATOMIC|PR_ADDR|PR_CONNREQUIRED|PR_WANTRCVD|PR_RIGHTS|       \
	PR_IMPLOPCL|PR_LASTHDR|PR_PROTOLOCK|PR_PCBLOCK|PR_DISPOSE)
#endif                         

#ifdef BSD_KERNEL_PRIVATE
#ifdef PRCREQUESTS
char    *prcrequests[] = {
	"IFDOWN", "ROUTEDEAD", "IFUP", "DEC-BIT-QUENCH2",
	"QUENCH", "MSGSIZE", "HOSTDEAD", "#7",
	"NET-UNREACH", "HOST-UNREACH", "PROTO-UNREACH", "PORT-UNREACH",
	"#12", "SRCFAIL-UNREACH", "NET-REDIRECT", "HOST-REDIRECT",
	"TOSNET-REDIRECT", "TOSHOST-REDIRECT", "TX-INTRANS", "TX-REASS",
	"PARAMPROB", "ADMIN-UNREACH"
};
#endif                  

  
                                  
                                                                 
                                                                     
                                                                         
                                                         
                                                                         
                                                                     
               
                                                                          
                                         
                                                                     
   
#define PRCO_GETOPT     0
#define PRCO_SETOPT     1

#define PRCO_NCMDS      2

#ifdef PRCOREQUESTS
char    *prcorequests[] = {
	"GETOPT", "SETOPT",
};
#endif                   

  
                                                                           
                                                                          
                                                                             
                                                                           
                                                                             
   
#define PRU_ATTACH              0                                  
#define PRU_DETACH              1                                    
#define PRU_BIND                2                                   
#define PRU_LISTEN              3                                  
#define PRU_CONNECT             4                                         
#define PRU_ACCEPT              5                                        
#define PRU_DISCONNECT          6                                 
#define PRU_SHUTDOWN            7                                     
#define PRU_RCVD                8                                           
#define PRU_SEND                9                           
#define PRU_ABORT               10                                            
#define PRU_CONTROL             11                                          
#define PRU_SENSE               12                                
#define PRU_RCVOOB              13                                     
#define PRU_SENDOOB             14                                 
#define PRU_SOCKADDR            15                                  
#define PRU_PEERADDR            16                                
#define PRU_CONNECT2            17                               
                                      
#define PRU_FASTTIMO            18                         
#define PRU_SLOWTIMO            19                         
#define PRU_PROTORCV            20                              
#define PRU_PROTOSEND           21                         
                                     
#define PRU_SEND_EOF            22                          
#define PRU_NREQ                22

#ifdef PRUREQUESTS
char *prurequests[] = {
	"ATTACH", "DETACH", "BIND", "LISTEN",
	"CONNECT", "ACCEPT", "DISCONNECT", "SHUTDOWN",
	"RCVD", "SEND", "ABORT", "CONTROL",
	"SENSE", "RCVOOB", "SENDOOB", "SOCKADDR",
	"PEERADDR", "CONNECT2", "FASTTIMO", "SLOWTIMO",
	"PROTORCV", "PROTOSEND", "SEND_EOF",
};
#endif                  
#endif                         

struct stat;
struct ucred;
struct uio;
struct recv_msg_elem;

#ifdef XNU_KERNEL_PRIVATE
  
                              
  
                                                                          
                                                                              
                                                                          
                                                                  
   
struct pr_usrreqs_old {
#else
struct pr_usrreqs {
#endif                          
	int     (*pru_abort)(struct socket *so);
	int     (*pru_accept)(struct socket *so, struct sockaddr **nam);
	int     (*pru_attach)(struct socket *so, int proto, struct proc *p);
	int     (*pru_bind)(struct socket *so, struct sockaddr *nam,
	    struct proc *p);
	int     (*pru_connect)(struct socket *so, struct sockaddr *nam,
	    struct proc *p);
	int     (*pru_connect2)(struct socket *so1, struct socket *so2);
	int     (*pru_control)(struct socket *so, u_long cmd, caddr_t data,
	    struct ifnet *ifp, struct proc *p);
	int     (*pru_detach)(struct socket *so);
	int     (*pru_disconnect)(struct socket *so);
	int     (*pru_listen)(struct socket *so, struct proc *p);
	int     (*pru_peeraddr)(struct socket *so, struct sockaddr **nam);
	int     (*pru_rcvd)(struct socket *so, int flags);
	int     (*pru_rcvoob)(struct socket *so, struct mbuf *m, int flags);
	int     (*pru_send)(struct socket *so, int flags, struct mbuf *m,
	    struct sockaddr *addr, struct mbuf *control,
	    struct proc *p);
	int     (*pru_sense)(struct socket *so, void  *sb, int isstat64);
	int     (*pru_shutdown)(struct socket *so);
	int     (*pru_sockaddr)(struct socket *so, struct sockaddr **nam);
	int     (*pru_sosend)(struct socket *so, struct sockaddr *addr,
	    struct uio *uio, struct mbuf *top, struct mbuf *control,
	    int flags);
	int     (*pru_soreceive)(struct socket *so, struct sockaddr **paddr,
	    struct uio *uio, struct mbuf **mp0, struct mbuf **controlp,
	    int *flagsp);
	int     (*pru_sopoll)(struct socket *so, int events,
	    struct ucred *cred, void *);
};

#ifdef XNU_KERNEL_PRIVATE
  
                                                                           
                                                        
  
                                                                 
  
                                                                            
   
struct pr_usrreqs {
	uint32_t pru_flags;                               
	int     (*pru_abort)(struct socket *);
	int     (*pru_accept)(struct socket *, struct sockaddr **);
	int     (*pru_attach)(struct socket *, int proto, struct proc *);
	int     (*pru_bind)(struct socket *, struct sockaddr *, struct proc *);
	int     (*pru_connect)(struct socket *, struct sockaddr *,
	    struct proc *);
	int     (*pru_connect2)(struct socket *, struct socket *);
	int     (*pru_connectx)(struct socket *, struct sockaddr *,
	    struct sockaddr *, struct proc *, uint32_t,
	    sae_associd_t, sae_connid_t *, uint32_t, void *, uint32_t,
	    struct uio *, user_ssize_t *);
	int     (*pru_control)(struct socket *, u_long, caddr_t,
	    struct ifnet *, struct proc *);
	int     (*pru_detach)(struct socket *);
	int     (*pru_disconnect)(struct socket *);
	int     (*pru_disconnectx)(struct socket *,
	    sae_associd_t, sae_connid_t);
	int     (*pru_listen)(struct socket *, struct proc *);
	int     (*pru_peeraddr)(struct socket *, struct sockaddr **);
	int     (*pru_rcvd)(struct socket *, int);
	int     (*pru_rcvoob)(struct socket *, struct mbuf *, int);
	int     (*pru_send)(struct socket *, int, struct mbuf *,
	    struct sockaddr *, struct mbuf *, struct proc *);
	int     (*pru_send_list)(struct socket *, int, struct mbuf *,
	    struct sockaddr *, struct mbuf *, struct proc *);
#define PRUS_OOB        0x1
#define PRUS_EOF        0x2
#define PRUS_MORETOCOME 0x4
	int     (*pru_sense)(struct socket *, void *, int);
	int     (*pru_shutdown)(struct socket *);
	int     (*pru_sockaddr)(struct socket *, struct sockaddr **);
	int     (*pru_sopoll)(struct socket *, int, struct ucred *, void *);
	int     (*pru_soreceive)(struct socket *, struct sockaddr **,
	    struct uio *, struct mbuf **, struct mbuf **, int *);
	int     (*pru_soreceive_list)(struct socket *, struct recv_msg_elem *, u_int,
	    int *);
	int     (*pru_sosend)(struct socket *, struct sockaddr *,
	    struct uio *, struct mbuf *, struct mbuf *, int);
	int     (*pru_sosend_list)(struct socket *, struct uio **, u_int, int);
	int     (*pru_socheckopt)(struct socket *, struct sockopt *);
	int     (*pru_preconnect)(struct socket *so);
};

                           
#define PRUF_OLD        0x10000000                                   

#ifdef BSD_KERNEL_PRIVATE
  
                                                                     
   
extern u_int64_t _net_uptime;
#endif                         
#endif                         

__BEGIN_DECLS
extern int pru_abort_notsupp(struct socket *so);
extern int pru_accept_notsupp(struct socket *so, struct sockaddr **nam);
extern int pru_attach_notsupp(struct socket *so, int proto, struct proc *p);
extern int pru_bind_notsupp(struct socket *so, struct sockaddr *nam,
    struct proc *p);
extern int pru_connect_notsupp(struct socket *so, struct sockaddr *nam,
    struct proc *p);
extern int pru_connect2_notsupp(struct socket *so1, struct socket *so2);
#ifdef XNU_KERNEL_PRIVATE
extern int pru_connectx_notsupp(struct socket *, struct sockaddr *,
    struct sockaddr *, struct proc *, uint32_t, sae_associd_t,
    sae_connid_t *, uint32_t, void *, uint32_t, struct uio *, user_ssize_t *);
extern int pru_disconnectx_notsupp(struct socket *, sae_associd_t,
    sae_connid_t);
extern int pru_socheckopt_null(struct socket *, struct sockopt *);
#endif                         
extern int pru_control_notsupp(struct socket *so, u_long cmd, caddr_t data,
    struct ifnet *ifp, struct proc *p);
extern int pru_detach_notsupp(struct socket *so);
extern int pru_disconnect_notsupp(struct socket *so);
extern int pru_listen_notsupp(struct socket *so, struct proc *p);
extern int pru_peeraddr_notsupp(struct socket *so, struct sockaddr **nam);
extern int pru_rcvd_notsupp(struct socket *so, int flags);
extern int pru_rcvoob_notsupp(struct socket *so, struct mbuf *m, int flags);
extern int pru_send_notsupp(struct socket *so, int flags, struct mbuf *m,
    struct sockaddr *addr, struct mbuf *control, struct proc *p);
extern int pru_send_list_notsupp(struct socket *so, int flags, struct mbuf *m,
    struct sockaddr *addr, struct mbuf *control, struct proc *p);
extern int pru_sense_null(struct socket *so, void * sb, int isstat64);
extern int pru_shutdown_notsupp(struct socket *so);
extern int pru_sockaddr_notsupp(struct socket *so, struct sockaddr **nam);
extern int pru_sosend_notsupp(struct socket *so, struct sockaddr *addr,
    struct uio *uio, struct mbuf *top, struct mbuf *control, int flags);
extern int pru_sosend_list_notsupp(struct socket *so, struct uio **uio,
    u_int, int flags);
extern int pru_soreceive_notsupp(struct socket *so,
    struct sockaddr **paddr, struct uio *uio, struct mbuf **mp0,
    struct mbuf **controlp, int *flagsp);
extern int pru_soreceive_list_notsupp(struct socket *, struct recv_msg_elem *, u_int,
    int *);
extern int pru_sopoll_notsupp(struct socket *so, int events,
    struct ucred *cred, void *);
#ifdef XNU_KERNEL_PRIVATE
extern void pru_sanitize(struct pr_usrreqs *);
extern void domaininit(void);
extern void domainfin(void);
extern void pfctlinput(int, struct sockaddr *);
extern void pfctlinput2(int, struct sockaddr *, void *);
extern struct protosw *pffindproto_locked(int, int, int);
extern struct protosw *pffindprotonotype(int, int);
extern struct protosw *pffindtype(int, int);
extern struct protosw_old *pffindproto_old(int, int, int);
extern int net_add_proto(struct protosw *, struct domain *, int)
__XNU_INTERNAL(net_add_proto);
extern void net_init_proto(struct protosw *, struct domain *);
extern int net_del_proto(int, int, struct domain *)
__XNU_INTERNAL(net_del_proto);
extern int net_add_proto_old(struct protosw_old *, struct domain_old *);
extern int net_del_proto_old(int, int, struct domain_old *);
extern void net_update_uptime(void);
extern void net_update_uptime_with_time(const struct timeval *);
extern u_int64_t net_uptime(void);
extern u_int64_t net_uptime_ms(void);
extern void net_uptime2timeval(struct timeval *);
extern struct protosw *pffindproto(int family, int protocol, int type)
__XNU_INTERNAL(pffindproto);
#else
extern int net_add_proto(struct protosw *, struct domain *);
extern int net_del_proto(int, int, struct domain *);
extern struct protosw *pffindproto(int family, int protocol, int type);
#endif                         
__END_DECLS
#endif                     
#endif                        
