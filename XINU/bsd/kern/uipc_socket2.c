/*
 * Copyright (c) 1998-2020 Apple Inc. All rights reserved.
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
/* Copyright (c) 1995 NeXT Computer, Inc. All Rights Reserved */
/*
 * Copyright (c) 1982, 1986, 1988, 1990, 1993
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
 *	@(#)uipc_socket2.c	8.1 (Berkeley) 6/10/93
 */
  
                                                                      
                                                                          
                                                                        
               
   

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/domain.h>
#include <sys/kernel.h>
#include <sys/proc_internal.h>
#include <sys/kauth.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/mcache.h>
#include <sys/protosw.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/signalvar.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/unpcb.h>
#include <sys/ev.h>
#include <kern/locks.h>
#include <net/route.h>
#include <net/content_filter.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/tcp_var.h>
#include <sys/kdebug.h>
#include <libkern/OSAtomic.h>

#if CONFIG_MACF
#include <security/mac_framework.h>
#endif

#include <mach/vm_param.h>

#if MPTCP
#include <netinet/mptcp_var.h>
#endif

#define DBG_FNC_SBDROP          NETDBG_CODE(DBG_NETSOCK, 4)
#define DBG_FNC_SBAPPEND        NETDBG_CODE(DBG_NETSOCK, 5)

SYSCTL_DECL(_kern_ipc);

__private_extern__ u_int32_t net_io_policy_throttle_best_effort = 0;
SYSCTL_INT(_kern_ipc, OID_AUTO, throttle_best_effort,
    CTLFLAG_RW | CTLFLAG_LOCKED, &net_io_policy_throttle_best_effort, 0, "");

static inline void sbcompress(struct sockbuf *, struct mbuf *, struct mbuf *);
static struct socket *sonewconn_internal(struct socket *, int);
static int sbappendcontrol_internal(struct sockbuf *, struct mbuf *,
    struct mbuf *);
static void soevent_ifdenied(struct socket *);

static int sbappendrecord_common(struct sockbuf *sb, struct mbuf *m0, boolean_t nodrop);
static int sbappend_common(struct sockbuf *sb, struct mbuf *m, boolean_t nodrop);

  
                                                                 
   
static int soqlimitcompat = 1;
static int soqlencomp = 0;

  
                                                                              
                                                                            
                                                                              
   

u_int32_t       sb_max = SB_MAX;                                          
u_int32_t       high_sb_max = SB_MAX;

static  u_int32_t sb_efficiency = 8;                                   
int32_t total_sbmb_cnt __attribute__((aligned(8))) = 0;
int32_t total_sbmb_cnt_floor __attribute__((aligned(8))) = 0;
int32_t total_sbmb_cnt_peak __attribute__((aligned(8))) = 0;
int64_t sbmb_limreached __attribute__((aligned(8))) = 0;

u_int32_t net_io_policy_log = 0;                                       
#if CONFIG_PROC_UUID_POLICY
u_int32_t net_io_policy_uuid = 1;                                      
#endif                              

  
                                                 
                                                        
                                                        
                                              
                                                               
                                                               
                                                                        
                                                                   
                                                                     
                                                                           
                                                                         
                 
  
                                                  
                                                               
                                                                         
                                                                       
                                                                             
                                                               
                                                                
  
                                               
                                                   
  
                                               
                                                   
                                               
   
void
soisconnecting(struct socket *so)
{
	so->so_state &= ~(SS_ISCONNECTED | SS_ISDISCONNECTING);
	so->so_state |= SS_ISCONNECTING;

	sflt_notify(so, sock_evt_connecting, NULL);
}

void
soisconnected(struct socket *so)
{
	  
                                                                  
                                                                      
                            
    
	if (cfil_sock_connected_pending_verdict(so)) {
		return;
	}

	so->so_state &= ~(SS_ISCONNECTING | SS_ISDISCONNECTING | SS_ISCONFIRMING);
	so->so_state |= SS_ISCONNECTED;

	soreserve_preconnect(so, 0);

	sflt_notify(so, sock_evt_connected, NULL);

	if (so->so_head != NULL && (so->so_state & SS_INCOMP)) {
		struct socket *head = so->so_head;
		int locked = 0;

		  
                                                              
     
		if (head->so_proto->pr_getlock != NULL) {
			socket_lock(head, 1);
			so_acquire_accept_list(head, so);
			locked = 1;
		}
		if (so->so_head == head && (so->so_state & SS_INCOMP)) {
			so->so_state &= ~SS_INCOMP;
			so->so_state |= SS_COMP;
			TAILQ_REMOVE(&head->so_incomp, so, so_list);
			TAILQ_INSERT_TAIL(&head->so_comp, so, so_list);
			head->so_incqlen--;

			  
                                           
                                                
      
			if (locked != 0) {
				so_release_accept_list(head);
				socket_unlock(so, 0);
			}
			sorwakeup(head);
			wakeup_one((caddr_t)&head->so_timeo);

			if (locked != 0) {
				socket_unlock(head, 1);
				socket_lock(so, 0);
			}
		} else if (locked != 0) {
			so_release_accept_list(head);
			socket_unlock(head, 1);
		}
	} else {
		wakeup((caddr_t)&so->so_timeo);
		sorwakeup(so);
		sowwakeup(so);
		soevent(so, SO_FILT_HINT_LOCKED | SO_FILT_HINT_CONNECTED |
		    SO_FILT_HINT_CONNINFO_UPDATED);
	}
}

boolean_t
socanwrite(struct socket *so)
{
	return (so->so_state & SS_ISCONNECTED) ||
	       !(so->so_proto->pr_flags & PR_CONNREQUIRED) ||
	       (so->so_flags1 & SOF1_PRECONNECT_DATA);
}

void
soisdisconnecting(struct socket *so)
{
	so->so_state &= ~SS_ISCONNECTING;
	so->so_state |= (SS_ISDISCONNECTING | SS_CANTRCVMORE | SS_CANTSENDMORE);
	soevent(so, SO_FILT_HINT_LOCKED);
	sflt_notify(so, sock_evt_disconnecting, NULL);
	wakeup((caddr_t)&so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);
}

void
soisdisconnected(struct socket *so)
{
	so->so_state &= ~(SS_ISCONNECTING | SS_ISCONNECTED | SS_ISDISCONNECTING);
	so->so_state |= (SS_CANTRCVMORE | SS_CANTSENDMORE | SS_ISDISCONNECTED);
	soevent(so, SO_FILT_HINT_LOCKED | SO_FILT_HINT_DISCONNECTED |
	    SO_FILT_HINT_CONNINFO_UPDATED);
	sflt_notify(so, sock_evt_disconnected, NULL);
	wakeup((caddr_t)&so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);

#if CONTENT_FILTER
	                                                                   
	cfil_sock_notify_shutdown(so, SHUT_RDWR);
#endif                     
}

  
                                                                          
                                                                  
                              
   
void
sodisconnectwakeup(struct socket *so)
{
	so->so_state &= ~(SS_ISCONNECTING | SS_ISCONNECTED | SS_ISDISCONNECTING);
	so->so_state |= (SS_CANTRCVMORE | SS_CANTSENDMORE | SS_ISDISCONNECTED);
	soevent(so, SO_FILT_HINT_LOCKED | SO_FILT_HINT_DISCONNECTED |
	    SO_FILT_HINT_CONNINFO_UPDATED);
	wakeup((caddr_t)&so->so_timeo);
	sowwakeup(so);
	sorwakeup(so);

#if CONTENT_FILTER
	                                                                   
	cfil_sock_notify_shutdown(so, SHUT_RDWR);
#endif                     
}

  
                                                           
                                                          
                                                              
                                                              
                                                          
                                                              
   
static struct socket *
sonewconn_internal(struct socket *head, int connstatus)
{
	int so_qlen, error = 0;
	struct socket *so;
	lck_mtx_t *mutex_held;

	if (head->so_proto->pr_getlock != NULL) {
		mutex_held = (*head->so_proto->pr_getlock)(head, 0);
	} else {
		mutex_held = head->so_proto->pr_domain->dom_mtx;
	}
	LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);

	if (!soqlencomp) {
		  
                                                     
                                                 
     
		so_qlen = head->so_qlen;
	} else {
		  
                                                  
                                                   
                                                   
                                                  
                                                    
                                                  
     
		so_qlen = head->so_qlen - head->so_incqlen;
		if (head->so_incqlen > somaxconn) {
			so_qlen = somaxconn;
		}
	}

	if (so_qlen >=
	    (soqlimitcompat ? head->so_qlimit : (3 * head->so_qlimit / 2))) {
		return (struct socket *)0;
	}
	so = soalloc(1, SOCK_DOM(head), head->so_type);
	if (so == NULL) {
		return (struct socket *)0;
	}
	                                                 
	if (head->so_proto == NULL) {
		sodealloc(so);
		return (struct socket *)0;
	}

	so->so_type = head->so_type;
	so->so_options = head->so_options & ~SO_ACCEPTCONN;
	so->so_linger = head->so_linger;
	so->so_state = head->so_state | SS_NOFDREF;
	so->so_proto = head->so_proto;
	so->so_timeo = head->so_timeo;
	so->so_pgid  = head->so_pgid;
	kauth_cred_ref(head->so_cred);
	so->so_cred = head->so_cred;
	so->last_pid = head->last_pid;
	so->last_upid = head->last_upid;
	memcpy(so->last_uuid, head->last_uuid, sizeof(so->last_uuid));
	if (head->so_flags & SOF_DELEGATED) {
		so->e_pid = head->e_pid;
		so->e_upid = head->e_upid;
		memcpy(so->e_uuid, head->e_uuid, sizeof(so->e_uuid));
	}
	                                               
	so->so_flags = head->so_flags &
	    (SOF_NOSIGPIPE | SOF_NOADDRAVAIL | SOF_REUSESHAREUID |
	    SOF_NOTIFYCONFLICT | SOF_BINDRANDOMPORT | SOF_NPX_SETOPTSHUT |
	    SOF_NODEFUNCT | SOF_PRIVILEGED_TRAFFIC_CLASS | SOF_NOTSENT_LOWAT |
	    SOF_DELEGATED);
	so->so_flags1 |= SOF1_INBOUND;
	so->so_usecount = 1;
	so->next_lock_lr = 0;
	so->next_unlock_lr = 0;

	so->so_rcv.sb_flags |= SB_RECV;          
	so->so_rcv.sb_so = so->so_snd.sb_so = so;

	                                                       
	so->so_flags1 |=
	    head->so_flags1 & (SOF1_TRAFFIC_MGT_SO_BACKGROUND | SOF1_TC_NET_SERV_TYPE |
	    SOF1_QOSMARKING_ALLOWED | SOF1_QOSMARKING_POLICY_OVERRIDE);
	so->so_background_thread = head->so_background_thread;
	so->so_traffic_class = head->so_traffic_class;
	so->so_netsvctype = head->so_netsvctype;

	if (soreserve(so, head->so_snd.sb_hiwat, head->so_rcv.sb_hiwat)) {
		sodealloc(so);
		return (struct socket *)0;
	}
	so->so_rcv.sb_flags |= (head->so_rcv.sb_flags & SB_USRSIZE);
	so->so_snd.sb_flags |= (head->so_snd.sb_flags & SB_USRSIZE);

	  
                                                     
                                         
    
	if (head->so_proto->pr_unlock) {
		socket_unlock(head, 0);
	}
	if (((*so->so_proto->pr_usrreqs->pru_attach)(so, 0, NULL) != 0) ||
	    error) {
		sodealloc(so);
		if (head->so_proto->pr_unlock) {
			socket_lock(head, 0);
		}
		return (struct socket *)0;
	}
	if (head->so_proto->pr_unlock) {
		socket_lock(head, 0);
		  
                                                           
                                                         
     
		if ((head->so_options & SO_ACCEPTCONN) == 0) {
			so->so_state &= ~SS_NOFDREF;
			soclose(so);
			return (struct socket *)0;
		}
	}

	if (so->so_proto->pr_copy_last_owner != NULL) {
		(*so->so_proto->pr_copy_last_owner)(so, head);
	}
	atomic_add_32(&so->so_proto->pr_domain->dom_refs, 1);

	                                      
	so_acquire_accept_list(head, NULL);

	so->so_head = head;

	  
                                                             
                                                   
                                                          
                                                           
                 
    
	so->so_flags |= SOF_INCOMP_INPROGRESS;

	if (connstatus) {
		TAILQ_INSERT_TAIL(&head->so_comp, so, so_list);
		so->so_state |= SS_COMP;
	} else {
		TAILQ_INSERT_TAIL(&head->so_incomp, so, so_list);
		so->so_state |= SS_INCOMP;
		head->so_incqlen++;
	}
	head->so_qlen++;

	so_release_accept_list(head);

	                                             
	sflt_initsock(so);

	if (connstatus) {
		so->so_state |= connstatus;
		sorwakeup(head);
		wakeup((caddr_t)&head->so_timeo);
	}
	return so;
}


struct socket *
sonewconn(struct socket *head, int connstatus, const struct sockaddr *from)
{
	int error = sflt_connectin(head, from);
	if (error) {
		return NULL;
	}

	return sonewconn_internal(head, connstatus);
}

  
                                                                 
                                                                 
                                                                      
                                                                          
                                                                    
                                                                 
                                                         
   

void
socantsendmore(struct socket *so)
{
	so->so_state |= SS_CANTSENDMORE;
	soevent(so, SO_FILT_HINT_LOCKED | SO_FILT_HINT_CANTSENDMORE);
	sflt_notify(so, sock_evt_cantsendmore, NULL);
	sowwakeup(so);
}

void
socantrcvmore(struct socket *so)
{
	so->so_state |= SS_CANTRCVMORE;
	soevent(so, SO_FILT_HINT_LOCKED | SO_FILT_HINT_CANTRCVMORE);
	sflt_notify(so, sock_evt_cantrecvmore, NULL);
	sorwakeup(so);
}

  
                                                         
   
int
sbwait(struct sockbuf *sb)
{
	boolean_t nointr = (sb->sb_flags & SB_NOINTR);
	void *lr_saved = __builtin_return_address(0);
	struct socket *so = sb->sb_so;
	lck_mtx_t *mutex_held;
	struct timespec ts;
	int error = 0;

	if (so == NULL) {
		panic("%s: null so, sb=%p sb_flags=0x%x lr=%p\n",
		    __func__, sb, sb->sb_flags, lr_saved);
		                
	} else if (so->so_usecount < 1) {
		panic("%s: sb=%p sb_flags=0x%x sb_so=%p usecount=%d lr=%p "
		    "lrh= %s\n", __func__, sb, sb->sb_flags, so,
		    so->so_usecount, lr_saved, solockhistory_nr(so));
		                
	}

	if ((so->so_state & SS_DRAINING) || (so->so_flags & SOF_DEFUNCT)) {
		error = EBADF;
		if (so->so_flags & SOF_DEFUNCT) {
			SODEFUNCTLOG("%s[%d, %s]: defunct so 0x%llx [%d,%d] "
			    "(%d)\n", __func__, proc_selfpid(),
			    proc_best_name(current_proc()),
			    (uint64_t)VM_KERNEL_ADDRPERM(so),
			    SOCK_DOM(so), SOCK_TYPE(so), error);
		}
		return error;
	}

	if (so->so_proto->pr_getlock != NULL) {
		mutex_held = (*so->so_proto->pr_getlock)(so, PR_F_WILLUNLOCK);
	} else {
		mutex_held = so->so_proto->pr_domain->dom_mtx;
	}

	LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);

	ts.tv_sec = sb->sb_timeo.tv_sec;
	ts.tv_nsec = sb->sb_timeo.tv_usec * 1000;

	sb->sb_waiters++;
	VERIFY(sb->sb_waiters != 0);

	error = msleep((caddr_t)&sb->sb_cc, mutex_held,
	    nointr ? PSOCK : PSOCK | PCATCH,
	    nointr ? "sbwait_nointr" : "sbwait", &ts);

	VERIFY(sb->sb_waiters != 0);
	sb->sb_waiters--;

	if (so->so_usecount < 1) {
		panic("%s: 2 sb=%p sb_flags=0x%x sb_so=%p usecount=%d lr=%p "
		    "lrh= %s\n", __func__, sb, sb->sb_flags, so,
		    so->so_usecount, lr_saved, solockhistory_nr(so));
		                
	}

	if ((so->so_state & SS_DRAINING) || (so->so_flags & SOF_DEFUNCT)) {
		error = EBADF;
		if (so->so_flags & SOF_DEFUNCT) {
			SODEFUNCTLOG("%s[%d, %s]: defunct so 0x%llx [%d,%d] "
			    "(%d)\n", __func__, proc_selfpid(),
			    proc_best_name(current_proc()),
			    (uint64_t)VM_KERNEL_ADDRPERM(so),
			    SOCK_DOM(so), SOCK_TYPE(so), error);
		}
	}

	return error;
}

void
sbwakeup(struct sockbuf *sb)
{
	if (sb->sb_waiters > 0) {
		wakeup((caddr_t)&sb->sb_cc);
	}
}

  
                                               
                                         
                                           
   
void
sowakeup(struct socket *so, struct sockbuf *sb, struct socket *so2)
{
	if (so->so_flags & SOF_DEFUNCT) {
		SODEFUNCTLOG("%s[%d, %s]: defunct so 0x%llx [%d,%d] si 0x%x, "
		    "fl 0x%x [%s]\n", __func__, proc_selfpid(),
		    proc_best_name(current_proc()),
		    (uint64_t)VM_KERNEL_ADDRPERM(so), SOCK_DOM(so),
		    SOCK_TYPE(so), (uint32_t)sb->sb_sel.si_flags, sb->sb_flags,
		    (sb->sb_flags & SB_RECV) ? "rcv" : "snd");
	}

	sb->sb_flags &= ~SB_SEL;
	selwakeup(&sb->sb_sel);
	sbwakeup(sb);
	if (so->so_state & SS_ASYNC) {
		if (so->so_pgid < 0) {
			gsignal(-so->so_pgid, SIGIO);
		} else if (so->so_pgid > 0) {
			proc_signal(so->so_pgid, SIGIO);
		}
	}
	if (sb->sb_flags & SB_KNOTE) {
		KNOTE(&sb->sb_sel.si_note, SO_FILT_HINT_LOCKED);
	}
	if (sb->sb_flags & SB_UPCALL) {
		void (*sb_upcall)(struct socket *, void *, int);
		caddr_t sb_upcallarg;
		int lock = !(sb->sb_flags & SB_UPCALL_LOCK);

		sb_upcall = sb->sb_upcall;
		sb_upcallarg = sb->sb_upcallarg;
		                                                     
		so->so_upcallusecount++;

		if (lock) {
			if (so2) {
				struct unpcb *unp = sotounpcb(so2);
				unp->unp_flags |= UNP_DONTDISCONNECT;
				unp->rw_thrcount++;

				socket_unlock(so2, 0);
			}
			socket_unlock(so, 0);
		}
		(*sb_upcall)(so, sb_upcallarg, M_DONTWAIT);
		if (lock) {
			if (so2 && so > so2) {
				struct unpcb *unp;
				socket_lock(so2, 0);

				unp = sotounpcb(so2);
				unp->rw_thrcount--;
				if (unp->rw_thrcount == 0) {
					unp->unp_flags &= ~UNP_DONTDISCONNECT;
					wakeup(unp);
				}
			}

			socket_lock(so, 0);

			if (so2 && so < so2) {
				struct unpcb *unp;
				socket_lock(so2, 0);

				unp = sotounpcb(so2);
				unp->rw_thrcount--;
				if (unp->rw_thrcount == 0) {
					unp->unp_flags &= ~UNP_DONTDISCONNECT;
					wakeup(unp);
				}
			}
		}

		so->so_upcallusecount--;
		                                          
		if ((so->so_flags & SOF_CLOSEWAIT) &&
		    so->so_upcallusecount == 0) {
			wakeup((caddr_t)&so->so_upcallusecount);
		}
	}
#if CONTENT_FILTER
	  
                                                 
    
	if ((so->so_flags & SOF_CONTENT_FILTER) != 0) {
		if ((sb->sb_flags & SB_RECV)) {
			if (so->so_state & (SS_CANTRCVMORE)) {
				cfil_sock_notify_shutdown(so, SHUT_RD);
			}
		} else {
			if (so->so_state & (SS_CANTSENDMORE)) {
				cfil_sock_notify_shutdown(so, SHUT_WR);
			}
		}
	}
#endif                     
}

  
                                                   
  
                                                                    
                                                                  
                                                                  
                                                                        
                                          
  
                                                                     
                                                                  
                                                                           
                                                                    
                                                           
  
                                                                       
                                                                     
                                                             
                                                                        
                                                                      
                                                                   
                                                           
                                                                    
                                            
  
                                                                       
                                                                          
                                                                       
                                                                     
                                                                          
   

  
                       
           
   
int
soreserve(struct socket *so, uint32_t sndcc, uint32_t rcvcc)
{
	  
                                                   
                                             
                                                     
                                               
                                 
    
	uint64_t maxcc = (uint64_t)sb_max * MCLBYTES / (MSIZE + MCLBYTES);
	if (sndcc > maxcc) {
		sndcc = (uint32_t)maxcc;
	}
	if (rcvcc > maxcc) {
		rcvcc = (uint32_t)maxcc;
	}
	if (sbreserve(&so->so_snd, sndcc) == 0) {
		goto bad;
	} else {
		so->so_snd.sb_idealsize = sndcc;
	}

	if (sbreserve(&so->so_rcv, rcvcc) == 0) {
		goto bad2;
	} else {
		so->so_rcv.sb_idealsize = rcvcc;
	}

	if (so->so_rcv.sb_lowat == 0) {
		so->so_rcv.sb_lowat = 1;
	}
	if (so->so_snd.sb_lowat == 0) {
		so->so_snd.sb_lowat = MCLBYTES;
	}
	if (so->so_snd.sb_lowat > so->so_snd.sb_hiwat) {
		so->so_snd.sb_lowat = so->so_snd.sb_hiwat;
	}
	return 0;
bad2:
	so->so_snd.sb_flags &= ~SB_SEL;
	selthreadclear(&so->so_snd.sb_sel);
	sbrelease(&so->so_snd);
bad:
	return ENOBUFS;
}

void
soreserve_preconnect(struct socket *so, unsigned int pre_cc)
{
	                                                              
	so->so_snd.sb_preconn_hiwat = pre_cc;
	so->so_rcv.sb_preconn_hiwat = pre_cc;
}

  
                            
                                                               
                                                   
   
int
sbreserve(struct sockbuf *sb, u_int32_t cc)
{
	if ((u_quad_t)cc > (u_quad_t)sb_max * MCLBYTES / (MSIZE + MCLBYTES) ||
	    (cc > sb->sb_hiwat && (sb->sb_flags & SB_LIMITED))) {
		return 0;
	}
	sb->sb_hiwat = cc;
	sb->sb_mbmax = min(cc * sb_efficiency, sb_max);
	if (sb->sb_lowat > sb->sb_hiwat) {
		sb->sb_lowat = sb->sb_hiwat;
	}
	return 1;
}

  
                                                        
   
                                                               
void
sbrelease(struct sockbuf *sb)
{
	sbflush(sb);
	sb->sb_hiwat = 0;
	sb->sb_mbmax = 0;
}

  
                             
                           
  
                                                                     
                                                                    
                                                                       
                                                                    
                                                                  
                                                                     
                                                                  
                                                                      
                                                                     
                                                                         
                                                                    
                                                                      
                                      
  
                                                                 
                                                                   
                                                              
                                                                  
                                                               
   

  
                                                
                                                     
                                                     
                                                    
   
static int
sbappend_common(struct sockbuf *sb, struct mbuf *m, boolean_t nodrop)
{
	struct socket *so = sb->sb_so;

	if (m == NULL || (sb->sb_flags & SB_DROP)) {
		if (m != NULL && !nodrop) {
			m_freem(m);
		}
		return 0;
	}

	SBLASTRECORDCHK(sb, "sbappend 1");

	if (sb->sb_lastrecord != NULL && (sb->sb_mbtail->m_flags & M_EOR)) {
		return sbappendrecord_common(sb, m, nodrop);
	}

	if (SOCK_DOM(sb->sb_so) == PF_INET || SOCK_DOM(sb->sb_so) == PF_INET6) {
		ASSERT(nodrop == FALSE);
		if (sb->sb_flags & SB_RECV && !(m && m->m_flags & M_SKIPCFIL)) {
			int error = sflt_data_in(so, NULL, &m, NULL, 0);
			SBLASTRECORDCHK(sb, "sbappend 2");

#if CONTENT_FILTER
			if (error == 0) {
				error = cfil_sock_data_in(so, NULL, m, NULL, 0);
			}
#endif                     

			if (error != 0) {
				if (error != EJUSTRETURN) {
					m_freem(m);
				}
				return 0;
			}
		} else if (m) {
			m->m_flags &= ~M_SKIPCFIL;
		}
	}

	                                                            
	if (sb->sb_lastrecord == NULL) {
		sb->sb_lastrecord = m;
	}

	sbcompress(sb, m, sb->sb_mbtail);
	SBLASTRECORDCHK(sb, "sbappend 3");
	return 1;
}

int
sbappend(struct sockbuf *sb, struct mbuf *m)
{
	return sbappend_common(sb, m, FALSE);
}

int
sbappend_nodrop(struct sockbuf *sb, struct mbuf *m)
{
	return sbappend_common(sb, m, TRUE);
}

  
                                                                         
   
int
sbappendstream(struct sockbuf *sb, struct mbuf *m)
{
	struct socket *so = sb->sb_so;

	if (m == NULL || (sb->sb_flags & SB_DROP)) {
		if (m != NULL) {
			m_freem(m);
		}
		return 0;
	}

	if (m->m_nextpkt != NULL || (sb->sb_mb != sb->sb_lastrecord)) {
		panic("sbappendstream: nexpkt %p || mb %p != lastrecord %p\n",
		    m->m_nextpkt, sb->sb_mb, sb->sb_lastrecord);
		                
	}

	SBLASTMBUFCHK(sb, __func__);

	if (SOCK_DOM(sb->sb_so) == PF_INET || SOCK_DOM(sb->sb_so) == PF_INET6) {
		if (sb->sb_flags & SB_RECV && !(m && m->m_flags & M_SKIPCFIL)) {
			int error = sflt_data_in(so, NULL, &m, NULL, 0);
			SBLASTRECORDCHK(sb, "sbappendstream 1");

#if CONTENT_FILTER
			if (error == 0) {
				error = cfil_sock_data_in(so, NULL, m, NULL, 0);
			}
#endif                     

			if (error != 0) {
				if (error != EJUSTRETURN) {
					m_freem(m);
				}
				return 0;
			}
		} else if (m) {
			m->m_flags &= ~M_SKIPCFIL;
		}
	}

	sbcompress(sb, m, sb->sb_mbtail);
	sb->sb_lastrecord = sb->sb_mb;
	SBLASTRECORDCHK(sb, "sbappendstream 2");
	return 1;
}

#ifdef SOCKBUF_DEBUG
void
sbcheck(struct sockbuf *sb)
{
	struct mbuf *m;
	struct mbuf *n = 0;
	u_int32_t len = 0, mbcnt = 0;
	lck_mtx_t *mutex_held;

	if (sb->sb_so->so_proto->pr_getlock != NULL) {
		mutex_held = (*sb->sb_so->so_proto->pr_getlock)(sb->sb_so, 0);
	} else {
		mutex_held = sb->sb_so->so_proto->pr_domain->dom_mtx;
	}

	LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);

	if (sbchecking == 0) {
		return;
	}

	for (m = sb->sb_mb; m; m = n) {
		n = m->m_nextpkt;
		for (; m; m = m->m_next) {
			len += m->m_len;
			mbcnt += MSIZE;
			                                   
			if (m->m_flags & M_EXT) {
				mbcnt += m->m_ext.ext_size;
			}
		}
	}
	if (len != sb->sb_cc || mbcnt != sb->sb_mbcnt) {
		panic("cc %ld != %ld || mbcnt %ld != %ld\n", len, sb->sb_cc,
		    mbcnt, sb->sb_mbcnt);
	}
}
#endif

void
sblastrecordchk(struct sockbuf *sb, const char *where)
{
	struct mbuf *m = sb->sb_mb;

	while (m && m->m_nextpkt) {
		m = m->m_nextpkt;
	}

	if (m != sb->sb_lastrecord) {
		printf("sblastrecordchk: mb 0x%llx lastrecord 0x%llx "
		    "last 0x%llx\n",
		    (uint64_t)VM_KERNEL_ADDRPERM(sb->sb_mb),
		    (uint64_t)VM_KERNEL_ADDRPERM(sb->sb_lastrecord),
		    (uint64_t)VM_KERNEL_ADDRPERM(m));
		printf("packet chain:\n");
		for (m = sb->sb_mb; m != NULL; m = m->m_nextpkt) {
			printf("\t0x%llx\n", (uint64_t)VM_KERNEL_ADDRPERM(m));
		}
		panic("sblastrecordchk from %s", where);
	}
}

void
sblastmbufchk(struct sockbuf *sb, const char *where)
{
	struct mbuf *m = sb->sb_mb;
	struct mbuf *n;

	while (m && m->m_nextpkt) {
		m = m->m_nextpkt;
	}

	while (m && m->m_next) {
		m = m->m_next;
	}

	if (m != sb->sb_mbtail) {
		printf("sblastmbufchk: mb 0x%llx mbtail 0x%llx last 0x%llx\n",
		    (uint64_t)VM_KERNEL_ADDRPERM(sb->sb_mb),
		    (uint64_t)VM_KERNEL_ADDRPERM(sb->sb_mbtail),
		    (uint64_t)VM_KERNEL_ADDRPERM(m));
		printf("packet tree:\n");
		for (m = sb->sb_mb; m != NULL; m = m->m_nextpkt) {
			printf("\t");
			for (n = m; n != NULL; n = n->m_next) {
				printf("0x%llx ",
				    (uint64_t)VM_KERNEL_ADDRPERM(n));
			}
			printf("\n");
		}
		panic("sblastmbufchk from %s", where);
	}
}

  
                                                                  
   
static int
sbappendrecord_common(struct sockbuf *sb, struct mbuf *m0, boolean_t nodrop)
{
	struct mbuf *m;
	int space = 0;

	if (m0 == NULL || (sb->sb_flags & SB_DROP)) {
		if (m0 != NULL && nodrop == FALSE) {
			m_freem(m0);
		}
		return 0;
	}

	for (m = m0; m != NULL; m = m->m_next) {
		space += m->m_len;
	}

	if (space > sbspace(sb) && !(sb->sb_flags & SB_UNIX)) {
		if (nodrop == FALSE) {
			m_freem(m0);
		}
		return 0;
	}

	if (SOCK_DOM(sb->sb_so) == PF_INET || SOCK_DOM(sb->sb_so) == PF_INET6) {
		ASSERT(nodrop == FALSE);
		if (sb->sb_flags & SB_RECV && !(m0 && m0->m_flags & M_SKIPCFIL)) {
			int error = sflt_data_in(sb->sb_so, NULL, &m0, NULL,
			    sock_data_filt_flag_record);

#if CONTENT_FILTER
			if (error == 0) {
				error = cfil_sock_data_in(sb->sb_so, NULL, m0, NULL, 0);
			}
#endif                     

			if (error != 0) {
				SBLASTRECORDCHK(sb, "sbappendrecord 1");
				if (error != EJUSTRETURN) {
					m_freem(m0);
				}
				return 0;
			}
		} else if (m0) {
			m0->m_flags &= ~M_SKIPCFIL;
		}
	}

	  
                                          
    
	sballoc(sb, m0);
	SBLASTRECORDCHK(sb, "sbappendrecord 2");
	if (sb->sb_lastrecord != NULL) {
		sb->sb_lastrecord->m_nextpkt = m0;
	} else {
		sb->sb_mb = m0;
	}
	sb->sb_lastrecord = m0;
	sb->sb_mbtail = m0;

	m = m0->m_next;
	m0->m_next = 0;
	if (m && (m0->m_flags & M_EOR)) {
		m0->m_flags &= ~M_EOR;
		m->m_flags |= M_EOR;
	}
	sbcompress(sb, m, m0);
	SBLASTRECORDCHK(sb, "sbappendrecord 3");
	return 1;
}

int
sbappendrecord(struct sockbuf *sb, struct mbuf *m0)
{
	return sbappendrecord_common(sb, m0, FALSE);
}

int
sbappendrecord_nodrop(struct sockbuf *sb, struct mbuf *m0)
{
	return sbappendrecord_common(sb, m0, TRUE);
}

  
                                                                       
                                                                       
             
  
                                                           
   
struct mbuf *
sbconcat_mbufs(struct sockbuf *sb, struct sockaddr *asa, struct mbuf *m0, struct mbuf *control)
{
	struct mbuf *m = NULL, *n = NULL;
	int space = 0;

	if (m0 && (m0->m_flags & M_PKTHDR) == 0) {
		panic("sbconcat_mbufs");
	}

	if (m0) {
		space += m0->m_pkthdr.len;
	}
	for (n = control; n; n = n->m_next) {
		space += n->m_len;
		if (n->m_next == 0) {                                         
			break;
		}
	}

	if (asa != NULL) {
		if (asa->sa_len > MLEN) {
			return NULL;
		}
		space += asa->sa_len;
	}

	if (sb != NULL && space > sbspace(sb)) {
		return NULL;
	}

	if (n) {
		n->m_next = m0;                                          
	} else {
		control = m0;
	}

	if (asa != NULL) {
		MGET(m, M_DONTWAIT, MT_SONAME);
		if (m == 0) {
			if (n) {
				                                           
				n->m_next = NULL;
			}
			return NULL;
		}
		m->m_len = asa->sa_len;
		bcopy((caddr_t)asa, mtod(m, caddr_t), asa->sa_len);

		m->m_next = control;
	} else {
		m = control;
	}

	return m;
}

  
                                                     
                                                      
                                               
  
                                 
               
   
int
sbappendchain(struct sockbuf *sb, struct mbuf *m, int space)
{
	struct mbuf *n, *nlast;

	if (m == NULL) {
		return 0;
	}

	if (space != 0 && space > sbspace(sb)) {
		return 0;
	}

	for (n = m; n->m_next != NULL; n = n->m_next) {
		sballoc(sb, n);
	}
	sballoc(sb, n);
	nlast = n;

	if (sb->sb_lastrecord != NULL) {
		sb->sb_lastrecord->m_nextpkt = m;
	} else {
		sb->sb_mb = m;
	}
	sb->sb_lastrecord = m;
	sb->sb_mbtail = nlast;

	SBLASTMBUFCHK(sb, __func__);
	SBLASTRECORDCHK(sb, "sbappendadddr 2");
	return 1;
}

  
                                                 
               
  
                                         
           
                                                       
   
int
sbappendaddr(struct sockbuf *sb, struct sockaddr *asa, struct mbuf *m0,
    struct mbuf *control, int *error_out)
{
	int result = 0;
	boolean_t sb_unix = (sb->sb_flags & SB_UNIX);
	struct mbuf *mbuf_chain = NULL;

	if (error_out) {
		*error_out = 0;
	}

	if (m0 && (m0->m_flags & M_PKTHDR) == 0) {
		panic("sbappendaddrorfree");
	}

	if (sb->sb_flags & SB_DROP) {
		if (m0 != NULL) {
			m_freem(m0);
		}
		if (control != NULL && !sb_unix) {
			m_freem(control);
		}
		if (error_out != NULL) {
			*error_out = EINVAL;
		}
		return 0;
	}

	if (SOCK_DOM(sb->sb_so) == PF_INET || SOCK_DOM(sb->sb_so) == PF_INET6) {
		                                 
		if (sb->sb_flags & SB_RECV && !(m0 && m0->m_flags & M_SKIPCFIL)) {
			int error;
			error = sflt_data_in(sb->sb_so, asa, &m0, &control, 0);
			SBLASTRECORDCHK(sb, __func__);

#if CONTENT_FILTER
			if (error == 0) {
				error = cfil_sock_data_in(sb->sb_so, asa, m0, control,
				    0);
			}
#endif                     

			if (error) {
				if (error != EJUSTRETURN) {
					if (m0) {
						m_freem(m0);
					}
					if (control != NULL && !sb_unix) {
						m_freem(control);
					}
					if (error_out) {
						*error_out = error;
					}
				}
				return 0;
			}
		} else if (m0) {
			m0->m_flags &= ~M_SKIPCFIL;
		}
	}

	mbuf_chain = sbconcat_mbufs(sb, asa, m0, control);
	SBLASTRECORDCHK(sb, "sbappendadddr 1");
	result = sbappendchain(sb, mbuf_chain, 0);
	if (result == 0) {
		if (m0) {
			m_freem(m0);
		}
		if (control != NULL && !sb_unix) {
			m_freem(control);
		}
		if (error_out) {
			*error_out = ENOBUFS;
		}
	}

	return result;
}

inline boolean_t
is_cmsg_valid(struct mbuf *control, struct cmsghdr *cmsg)
{
	if (cmsg == NULL) {
		return FALSE;
	}

	if (cmsg->cmsg_len < sizeof(struct cmsghdr)) {
		return FALSE;
	}

	if ((uint8_t *)control->m_data >= (uint8_t *)cmsg + cmsg->cmsg_len) {
		return FALSE;
	}

	if ((uint8_t *)control->m_data + control->m_len <
	    (uint8_t *)cmsg + cmsg->cmsg_len) {
		return FALSE;
	}

	return TRUE;
}

static int
sbappendcontrol_internal(struct sockbuf *sb, struct mbuf *m0,
    struct mbuf *control)
{
	struct mbuf *m, *mlast, *n;
	int space = 0;

	if (control == 0) {
		panic("sbappendcontrol");
	}

	for (m = control;; m = m->m_next) {
		space += m->m_len;
		if (m->m_next == 0) {
			break;
		}
	}
	n = m;                                                           
	for (m = m0; m; m = m->m_next) {
		space += m->m_len;
	}
	if (space > sbspace(sb) && !(sb->sb_flags & SB_UNIX)) {
		return 0;
	}
	n->m_next = m0;                                                  
	SBLASTRECORDCHK(sb, "sbappendcontrol 1");

	for (m = control; m->m_next != NULL; m = m->m_next) {
		sballoc(sb, m);
	}
	sballoc(sb, m);
	mlast = m;

	if (sb->sb_lastrecord != NULL) {
		sb->sb_lastrecord->m_nextpkt = control;
	} else {
		sb->sb_mb = control;
	}
	sb->sb_lastrecord = control;
	sb->sb_mbtail = mlast;

	SBLASTMBUFCHK(sb, __func__);
	SBLASTRECORDCHK(sb, "sbappendcontrol 2");
	return 1;
}

int
sbappendcontrol(struct sockbuf *sb, struct mbuf *m0, struct mbuf *control,
    int *error_out)
{
	int result = 0;
	boolean_t sb_unix = (sb->sb_flags & SB_UNIX);

	if (error_out) {
		*error_out = 0;
	}

	if (sb->sb_flags & SB_DROP) {
		if (m0 != NULL) {
			m_freem(m0);
		}
		if (control != NULL && !sb_unix) {
			m_freem(control);
		}
		if (error_out != NULL) {
			*error_out = EINVAL;
		}
		return 0;
	}

	if (SOCK_DOM(sb->sb_so) == PF_INET || SOCK_DOM(sb->sb_so) == PF_INET6) {
		if (sb->sb_flags & SB_RECV && !(m0 && m0->m_flags & M_SKIPCFIL)) {
			int error;

			error = sflt_data_in(sb->sb_so, NULL, &m0, &control, 0);
			SBLASTRECORDCHK(sb, __func__);

#if CONTENT_FILTER
			if (error == 0) {
				error = cfil_sock_data_in(sb->sb_so, NULL, m0, control,
				    0);
			}
#endif                     

			if (error) {
				if (error != EJUSTRETURN) {
					if (m0) {
						m_freem(m0);
					}
					if (control != NULL && !sb_unix) {
						m_freem(control);
					}
					if (error_out) {
						*error_out = error;
					}
				}
				return 0;
			}
		} else if (m0) {
			m0->m_flags &= ~M_SKIPCFIL;
		}
	}

	result = sbappendcontrol_internal(sb, m0, control);
	if (result == 0) {
		if (m0) {
			m_freem(m0);
		}
		if (control != NULL && !sb_unix) {
			m_freem(control);
		}
		if (error_out) {
			*error_out = ENOBUFS;
		}
	}

	return result;
}

  
                                                                     
   
int
sbappendstream_rcvdemux(struct socket *so, struct mbuf *m)
{
	int ret = 0;

	if ((m != NULL) &&
	    m_pktlen(m) <= 0 &&
	    !((so->so_flags & SOF_MP_SUBFLOW) &&
	    (m->m_flags & M_PKTHDR) &&
	    (m->m_pkthdr.pkt_flags & PKTF_MPTCP_DFIN))) {
		m_freem(m);
		return ret;
	}

#if MPTCP
	if (so->so_flags & SOF_MP_SUBFLOW) {
		return sbappendmptcpstream_rcv(&so->so_rcv, m);
	} else
#endif            
	{
		return sbappendstream(&so->so_rcv, m);
	}
}

#if MPTCP
int
sbappendmptcpstream_rcv(struct sockbuf *sb, struct mbuf *m)
{
	struct socket *so = sb->sb_so;

	VERIFY(m == NULL || (m->m_flags & M_PKTHDR));
	                                                             
	VERIFY((sb->sb_flags & (SB_RECV | SB_NOCOMPRESS)) ==
	    (SB_RECV | SB_NOCOMPRESS));

	if (m == NULL || m_pktlen(m) == 0 || (sb->sb_flags & SB_DROP) ||
	    (so->so_state & SS_CANTRCVMORE)) {
		if (m && (m->m_flags & M_PKTHDR) &&
		    m_pktlen(m) == 0 &&
		    (m->m_pkthdr.pkt_flags & PKTF_MPTCP_DFIN)) {
			mptcp_input(tptomptp(sototcpcb(so))->mpt_mpte, m);
			return 1;
		} else if (m != NULL) {
			m_freem(m);
		}
		return 0;
	}
	                                                             
	VERIFY(so->so_flags & SOF_MP_SUBFLOW);

	if (m->m_nextpkt != NULL || (sb->sb_mb != sb->sb_lastrecord)) {
		panic("%s: nexpkt %p || mb %p != lastrecord %p\n", __func__,
		    m->m_nextpkt, sb->sb_mb, sb->sb_lastrecord);
		                
	}

	SBLASTMBUFCHK(sb, __func__);

	                                                          

	sbcompress(sb, m, sb->sb_mbtail);
	sb->sb_lastrecord = sb->sb_mb;
	SBLASTRECORDCHK(sb, __func__);
	return 1;
}
#endif            

  
                                        
                                    
                                         
   
static inline void
sbcompress(struct sockbuf *sb, struct mbuf *m, struct mbuf *n)
{
	int eor = 0, compress = (!(sb->sb_flags & SB_NOCOMPRESS));
	struct mbuf *o;

	if (m == NULL) {
		                                                        
		for (; n->m_next != NULL; n = n->m_next) {
			;
		}
		sb->sb_mbtail = n;
		goto done;
	}

	while (m != NULL) {
		eor |= m->m_flags & M_EOR;
		if (compress && m->m_len == 0 && (eor == 0 ||
		    (((o = m->m_next) || (o = n)) && o->m_type == m->m_type))) {
			if (sb->sb_lastrecord == m) {
				sb->sb_lastrecord = m->m_next;
			}
			m = m_free(m);
			continue;
		}
		if (compress && n != NULL && (n->m_flags & M_EOR) == 0 &&
#ifndef __APPLE__
		    M_WRITABLE(n) &&
#endif
		    m->m_len <= MCLBYTES / 4 &&                               
		    m->m_len <= M_TRAILINGSPACE(n) &&
		    n->m_type == m->m_type) {
			bcopy(mtod(m, caddr_t), mtod(n, caddr_t) + n->m_len,
			    (unsigned)m->m_len);
			n->m_len += m->m_len;
			sb->sb_cc += m->m_len;
			if (m->m_type != MT_DATA && m->m_type != MT_HEADER &&
			    m->m_type != MT_OOBDATA) {
				                              
				sb->sb_ctl += m->m_len;
			}

			                            
			if (sb->sb_flags & SB_SNDBYTE_CNT) {
				inp_incr_sndbytes_total(sb->sb_so,
				    m->m_len);
				inp_incr_sndbytes_unsent(sb->sb_so,
				    m->m_len);
			}
			m = m_free(m);
			continue;
		}
		if (n != NULL) {
			n->m_next = m;
		} else {
			sb->sb_mb = m;
		}
		sb->sb_mbtail = m;
		sballoc(sb, m);
		n = m;
		m->m_flags &= ~M_EOR;
		m = m->m_next;
		n->m_next = NULL;
	}
	if (eor != 0) {
		if (n != NULL) {
			n->m_flags |= eor;
		} else {
			printf("semi-panic: sbcompress\n");
		}
	}
done:
	SBLASTMBUFCHK(sb, __func__);
}

void
sb_empty_assert(struct sockbuf *sb, const char *where)
{
	if (!(sb->sb_cc == 0 && sb->sb_mb == NULL && sb->sb_mbcnt == 0 &&
	    sb->sb_mbtail == NULL && sb->sb_lastrecord == NULL)) {
		panic("%s: sb %p so %p cc %d mbcnt %d mb %p mbtail %p "
		    "lastrecord %p\n", where, sb, sb->sb_so, sb->sb_cc,
		    sb->sb_mbcnt, sb->sb_mb, sb->sb_mbtail,
		    sb->sb_lastrecord);
		                
	}
}

  
                               
                                          
   
void
sbflush(struct sockbuf *sb)
{
	void *lr_saved = __builtin_return_address(0);
	struct socket *so = sb->sb_so;

	                                                              
	if (so == NULL) {
		panic("%s: null so, sb=%p sb_flags=0x%x lr=%p\n",
		    __func__, sb, sb->sb_flags, lr_saved);
		                
	} else if (so->so_usecount < 0) {
		panic("%s: sb=%p sb_flags=0x%x sb_so=%p usecount=%d lr=%p "
		    "lrh= %s\n", __func__, sb, sb->sb_flags, so,
		    so->so_usecount, lr_saved, solockhistory_nr(so));
		                
	}

	  
                                                                 
                                                                
                                                              
   
                                                     
    
	(void) sblock(sb, SBL_WAIT | SBL_NOINTR | SBL_IGNDEFUNCT);
	VERIFY(sb->sb_flags & SB_LOCK);

	while (sb->sb_mbcnt > 0) {
		  
                                                               
                                          
     
		if (!sb->sb_cc && (sb->sb_mb == NULL || sb->sb_mb->m_len)) {
			break;
		}
		sbdrop(sb, (int)sb->sb_cc);
	}

	sb_empty_assert(sb, __func__);
	sbunlock(sb, TRUE);                             
}

  
                                           
                                               
                                                 
                                                     
                                                      
                                              
                                                        
                                                      
                
   
void
sbdrop(struct sockbuf *sb, int len)
{
	struct mbuf *m, *free_list, *ml;
	struct mbuf *next, *last;

	next = (m = sb->sb_mb) ? m->m_nextpkt : 0;
#if MPTCP
	if (m != NULL && len > 0 && !(sb->sb_flags & SB_RECV) &&
	    ((sb->sb_so->so_flags & SOF_MP_SUBFLOW) ||
	    (SOCK_CHECK_DOM(sb->sb_so, PF_MULTIPATH) &&
	    SOCK_CHECK_PROTO(sb->sb_so, IPPROTO_TCP))) &&
	    !(sb->sb_so->so_flags1 & SOF1_POST_FALLBACK_SYNC)) {
		mptcp_preproc_sbdrop(sb->sb_so, m, (unsigned int)len);
	}
	if (m != NULL && len > 0 && !(sb->sb_flags & SB_RECV) &&
	    (sb->sb_so->so_flags & SOF_MP_SUBFLOW) &&
	    (sb->sb_so->so_flags1 & SOF1_POST_FALLBACK_SYNC)) {
		mptcp_fallback_sbdrop(sb->sb_so, m, len);
	}
#endif            
	KERNEL_DEBUG((DBG_FNC_SBDROP | DBG_FUNC_START), sb, len, 0, 0, 0);

	free_list = last = m;
	ml = (struct mbuf *)0;

	while (len > 0) {
		if (m == NULL) {
			if (next == NULL) {
				  
                                                   
                                                  
                                                 
                                            
               
       
				                      
				printf("sbdrop - count not zero\n");
				len = 0;
				  
                                            
                                   
       
				sb->sb_cc = 0;
				sb->sb_mbcnt = 0;
				break;
			}
			m = last = next;
			next = m->m_nextpkt;
			continue;
		}
		if (m->m_len > len) {
			m->m_len -= len;
			m->m_data += len;
			sb->sb_cc -= len;
			                                
			if (sb->sb_flags & SB_SNDBYTE_CNT) {
				inp_decr_sndbytes_total(sb->sb_so, len);
			}
			if (m->m_type != MT_DATA && m->m_type != MT_HEADER &&
			    m->m_type != MT_OOBDATA) {
				sb->sb_ctl -= len;
			}
			break;
		}
		len -= m->m_len;
		sbfree(sb, m);

		ml = m;
		m = m->m_next;
	}
	while (m && m->m_len == 0) {
		sbfree(sb, m);

		ml = m;
		m = m->m_next;
	}
	if (ml) {
		ml->m_next = (struct mbuf *)0;
		last->m_nextpkt = (struct mbuf *)0;
		m_freem_list(free_list);
	}
	if (m) {
		sb->sb_mb = m;
		m->m_nextpkt = next;
	} else {
		sb->sb_mb = next;
	}

	  
                                                          
                                                        
                            
    
	m = sb->sb_mb;
	if (m == NULL) {
		sb->sb_mbtail = NULL;
		sb->sb_lastrecord = NULL;
	} else if (m->m_nextpkt == NULL) {
		sb->sb_lastrecord = m;
	}

#if CONTENT_FILTER
	cfil_sock_buf_update(sb);
#endif                     

	KERNEL_DEBUG((DBG_FNC_SBDROP | DBG_FUNC_END), sb, 0, 0, 0, 0);
}

  
                                           
                                         
   
void
sbdroprecord(struct sockbuf *sb)
{
	struct mbuf *m, *mn;

	m = sb->sb_mb;
	if (m) {
		sb->sb_mb = m->m_nextpkt;
		do {
			sbfree(sb, m);
			MFREE(m, mn);
			m = mn;
		} while (m);
	}
	SB_EMPTY_FIXUP(sb);
}

  
                                                        
                                                               
   
struct mbuf *
sbcreatecontrol(caddr_t p, int size, int type, int level)
{
	struct cmsghdr *cp;
	struct mbuf *m;

	if (CMSG_SPACE((u_int)size) > MLEN) {
		return (struct mbuf *)NULL;
	}
	if ((m = m_get(M_DONTWAIT, MT_CONTROL)) == NULL) {
		return (struct mbuf *)NULL;
	}
	cp = mtod(m, struct cmsghdr *);
	VERIFY(IS_P2ALIGNED(cp, sizeof(u_int32_t)));
	                     
	(void) memcpy(CMSG_DATA(cp), p, size);
	m->m_len = (int32_t)CMSG_SPACE(size);
	cp->cmsg_len = CMSG_LEN(size);
	cp->cmsg_level = level;
	cp->cmsg_type = type;
	return m;
}

struct mbuf **
sbcreatecontrol_mbuf(caddr_t p, int size, int type, int level, struct mbuf **mp)
{
	struct mbuf *m;
	struct cmsghdr *cp;

	if (*mp == NULL) {
		*mp = sbcreatecontrol(p, size, type, level);
		return mp;
	}

	if (CMSG_SPACE((u_int)size) + (*mp)->m_len > MLEN) {
		mp = &(*mp)->m_next;
		*mp = sbcreatecontrol(p, size, type, level);
		return mp;
	}

	m = *mp;

	cp = (struct cmsghdr *)(void *)(mtod(m, char *) + m->m_len);
	                                         
	VERIFY(IS_P2ALIGNED(cp, sizeof(u_int32_t)));
	m->m_len += (int32_t)CMSG_SPACE(size);

	                     
	(void) memcpy(CMSG_DATA(cp), p, size);
	cp->cmsg_len = CMSG_LEN(size);
	cp->cmsg_level = level;
	cp->cmsg_type = type;

	return mp;
}


  
                                                                     
                                               
   
int
pru_abort_notsupp(struct socket *so)
{
#pragma unused(so)
	return EOPNOTSUPP;
}

int
pru_accept_notsupp(struct socket *so, struct sockaddr **nam)
{
#pragma unused(so, nam)
	return EOPNOTSUPP;
}

int
pru_attach_notsupp(struct socket *so, int proto, struct proc *p)
{
#pragma unused(so, proto, p)
	return EOPNOTSUPP;
}

int
pru_bind_notsupp(struct socket *so, struct sockaddr *nam, struct proc *p)
{
#pragma unused(so, nam, p)
	return EOPNOTSUPP;
}

int
pru_connect_notsupp(struct socket *so, struct sockaddr *nam, struct proc *p)
{
#pragma unused(so, nam, p)
	return EOPNOTSUPP;
}

int
pru_connect2_notsupp(struct socket *so1, struct socket *so2)
{
#pragma unused(so1, so2)
	return EOPNOTSUPP;
}

int
pru_connectx_notsupp(struct socket *so, struct sockaddr *src,
    struct sockaddr *dst, struct proc *p, uint32_t ifscope,
    sae_associd_t aid, sae_connid_t *pcid, uint32_t flags, void *arg,
    uint32_t arglen, struct uio *uio, user_ssize_t *bytes_written)
{
#pragma unused(so, src, dst, p, ifscope, aid, pcid, flags, arg, arglen, uio, bytes_written)
	return EOPNOTSUPP;
}

int
pru_control_notsupp(struct socket *so, u_long cmd, caddr_t data,
    struct ifnet *ifp, struct proc *p)
{
#pragma unused(so, cmd, data, ifp, p)
	return EOPNOTSUPP;
}

int
pru_detach_notsupp(struct socket *so)
{
#pragma unused(so)
	return EOPNOTSUPP;
}

int
pru_disconnect_notsupp(struct socket *so)
{
#pragma unused(so)
	return EOPNOTSUPP;
}

int
pru_disconnectx_notsupp(struct socket *so, sae_associd_t aid, sae_connid_t cid)
{
#pragma unused(so, aid, cid)
	return EOPNOTSUPP;
}

int
pru_listen_notsupp(struct socket *so, struct proc *p)
{
#pragma unused(so, p)
	return EOPNOTSUPP;
}

int
pru_peeraddr_notsupp(struct socket *so, struct sockaddr **nam)
{
#pragma unused(so, nam)
	return EOPNOTSUPP;
}

int
pru_rcvd_notsupp(struct socket *so, int flags)
{
#pragma unused(so, flags)
	return EOPNOTSUPP;
}

int
pru_rcvoob_notsupp(struct socket *so, struct mbuf *m, int flags)
{
#pragma unused(so, m, flags)
	return EOPNOTSUPP;
}

int
pru_send_notsupp(struct socket *so, int flags, struct mbuf *m,
    struct sockaddr *addr, struct mbuf *control, struct proc *p)
{
#pragma unused(so, flags, m, addr, control, p)
	return EOPNOTSUPP;
}

int
pru_send_list_notsupp(struct socket *so, int flags, struct mbuf *m,
    struct sockaddr *addr, struct mbuf *control, struct proc *p)
{
#pragma unused(so, flags, m, addr, control, p)
	return EOPNOTSUPP;
}

  
                                                                   
                                       
   
int
pru_sense_null(struct socket *so, void *ub, int isstat64)
{
	if (isstat64 != 0) {
		struct stat64 *sb64;

		sb64 = (struct stat64 *)ub;
		sb64->st_blksize = so->so_snd.sb_hiwat;
	} else {
		struct stat *sb;

		sb = (struct stat *)ub;
		sb->st_blksize = so->so_snd.sb_hiwat;
	}

	return 0;
}


int
pru_sosend_notsupp(struct socket *so, struct sockaddr *addr, struct uio *uio,
    struct mbuf *top, struct mbuf *control, int flags)
{
#pragma unused(so, addr, uio, top, control, flags)
	return EOPNOTSUPP;
}

int
pru_sosend_list_notsupp(struct socket *so, struct uio **uio,
    u_int uiocnt, int flags)
{
#pragma unused(so, uio, uiocnt, flags)
	return EOPNOTSUPP;
}

int
pru_soreceive_notsupp(struct socket *so, struct sockaddr **paddr,
    struct uio *uio, struct mbuf **mp0, struct mbuf **controlp, int *flagsp)
{
#pragma unused(so, paddr, uio, mp0, controlp, flagsp)
	return EOPNOTSUPP;
}

int
pru_soreceive_list_notsupp(struct socket *so,
    struct recv_msg_elem *recv_msg_array, u_int uiocnt, int *flagsp)
{
#pragma unused(so, recv_msg_array, uiocnt, flagsp)
	return EOPNOTSUPP;
}

int
pru_shutdown_notsupp(struct socket *so)
{
#pragma unused(so)
	return EOPNOTSUPP;
}

int
pru_sockaddr_notsupp(struct socket *so, struct sockaddr **nam)
{
#pragma unused(so, nam)
	return EOPNOTSUPP;
}

int
pru_sopoll_notsupp(struct socket *so, int events, kauth_cred_t cred, void *wql)
{
#pragma unused(so, events, cred, wql)
	return EOPNOTSUPP;
}

int
pru_socheckopt_null(struct socket *so, struct sockopt *sopt)
{
#pragma unused(so, sopt)
	  
                                             
    
	return 0;
}

static int
pru_preconnect_null(struct socket *so)
{
#pragma unused(so)
	return 0;
}

void
pru_sanitize(struct pr_usrreqs *pru)
{
#define DEFAULT(foo, bar)       if ((foo) == NULL) (foo) = (bar)
	DEFAULT(pru->pru_abort, pru_abort_notsupp);
	DEFAULT(pru->pru_accept, pru_accept_notsupp);
	DEFAULT(pru->pru_attach, pru_attach_notsupp);
	DEFAULT(pru->pru_bind, pru_bind_notsupp);
	DEFAULT(pru->pru_connect, pru_connect_notsupp);
	DEFAULT(pru->pru_connect2, pru_connect2_notsupp);
	DEFAULT(pru->pru_connectx, pru_connectx_notsupp);
	DEFAULT(pru->pru_control, pru_control_notsupp);
	DEFAULT(pru->pru_detach, pru_detach_notsupp);
	DEFAULT(pru->pru_disconnect, pru_disconnect_notsupp);
	DEFAULT(pru->pru_disconnectx, pru_disconnectx_notsupp);
	DEFAULT(pru->pru_listen, pru_listen_notsupp);
	DEFAULT(pru->pru_peeraddr, pru_peeraddr_notsupp);
	DEFAULT(pru->pru_rcvd, pru_rcvd_notsupp);
	DEFAULT(pru->pru_rcvoob, pru_rcvoob_notsupp);
	DEFAULT(pru->pru_send, pru_send_notsupp);
	DEFAULT(pru->pru_send_list, pru_send_list_notsupp);
	DEFAULT(pru->pru_sense, pru_sense_null);
	DEFAULT(pru->pru_shutdown, pru_shutdown_notsupp);
	DEFAULT(pru->pru_sockaddr, pru_sockaddr_notsupp);
	DEFAULT(pru->pru_sopoll, pru_sopoll_notsupp);
	DEFAULT(pru->pru_soreceive, pru_soreceive_notsupp);
	DEFAULT(pru->pru_soreceive_list, pru_soreceive_list_notsupp);
	DEFAULT(pru->pru_sosend, pru_sosend_notsupp);
	DEFAULT(pru->pru_sosend_list, pru_sosend_list_notsupp);
	DEFAULT(pru->pru_socheckopt, pru_socheckopt_null);
	DEFAULT(pru->pru_preconnect, pru_preconnect_null);
#undef DEFAULT
}

  
                                                          
   

  
                                                            
   

int
sb_notify(struct sockbuf *sb)
{
	return sb->sb_waiters > 0 ||
	       (sb->sb_flags & (SB_SEL | SB_ASYNC | SB_UPCALL | SB_KNOTE));
}

  
                                                                         
                                                                       
                                                                  
                         
   
int
sbspace(struct sockbuf *sb)
{
	int pending = 0;
	int space = imin((int)(sb->sb_hiwat - sb->sb_cc),
	    (int)(sb->sb_mbmax - sb->sb_mbcnt));

	if (sb->sb_preconn_hiwat != 0) {
		space = imin((int)(sb->sb_preconn_hiwat - sb->sb_cc), space);
	}

	if (space < 0) {
		space = 0;
	}

	                                                            
#if CONTENT_FILTER
	pending = cfil_sock_data_space(sb);
#endif                     
	if (pending > space) {
		space = 0;
	} else {
		space -= pending;
	}

	return space;
}

                                                 
int
sosendallatonce(struct socket *so)
{
	return so->so_proto->pr_flags & PR_ATOMIC;
}

                                    
int
soreadable(struct socket *so)
{
	return so->so_rcv.sb_cc >= so->so_rcv.sb_lowat ||
	       ((so->so_state & SS_CANTRCVMORE)
#if CONTENT_FILTER
	       && cfil_sock_data_pending(&so->so_rcv) == 0
#endif                     
	       ) ||
	       so->so_comp.tqh_first || so->so_error;
}

                                   

int
sowriteable(struct socket *so)
{
	if ((so->so_state & SS_CANTSENDMORE) ||
	    so->so_error > 0) {
		return 1;
	}
	if (so_wait_for_if_feedback(so) || !socanwrite(so)) {
		return 0;
	}
	if (so->so_flags1 & SOF1_PRECONNECT_DATA) {
		return 1;
	}

	if (sbspace(&(so)->so_snd) >= (so)->so_snd.sb_lowat) {
		if (so->so_flags & SOF_NOTSENT_LOWAT) {
			if ((SOCK_DOM(so) == PF_INET6 ||
			    SOCK_DOM(so) == PF_INET) &&
			    so->so_type == SOCK_STREAM) {
				return tcp_notsent_lowat_check(so);
			}
#if MPTCP
			else if ((SOCK_DOM(so) == PF_MULTIPATH) &&
			    (SOCK_PROTO(so) == IPPROTO_TCP)) {
				return mptcp_notsent_lowat_check(so);
			}
#endif
			else {
				return 1;
			}
		} else {
			return 1;
		}
	}
	return 0;
}

                                                      

void
sballoc(struct sockbuf *sb, struct mbuf *m)
{
	u_int32_t cnt = 1;
	sb->sb_cc += m->m_len;
	if (m->m_type != MT_DATA && m->m_type != MT_HEADER &&
	    m->m_type != MT_OOBDATA) {
		sb->sb_ctl += m->m_len;
	}
	sb->sb_mbcnt += MSIZE;

	if (m->m_flags & M_EXT) {
		sb->sb_mbcnt += m->m_ext.ext_size;
		cnt += (m->m_ext.ext_size >> MSIZESHIFT);
	}
	OSAddAtomic(cnt, &total_sbmb_cnt);
	VERIFY(total_sbmb_cnt > 0);
	if (total_sbmb_cnt > total_sbmb_cnt_peak) {
		total_sbmb_cnt_peak = total_sbmb_cnt;
	}

	  
                                                     
                              
    
	if (sb->sb_flags & SB_SNDBYTE_CNT) {
		inp_incr_sndbytes_total(sb->sb_so, m->m_len);
		inp_incr_sndbytes_unsent(sb->sb_so, m->m_len);
	}
}

                                                   
void
sbfree(struct sockbuf *sb, struct mbuf *m)
{
	int cnt = -1;

	sb->sb_cc -= m->m_len;
	if (m->m_type != MT_DATA && m->m_type != MT_HEADER &&
	    m->m_type != MT_OOBDATA) {
		sb->sb_ctl -= m->m_len;
	}
	sb->sb_mbcnt -= MSIZE;
	if (m->m_flags & M_EXT) {
		sb->sb_mbcnt -= m->m_ext.ext_size;
		cnt -= (m->m_ext.ext_size >> MSIZESHIFT);
	}
	OSAddAtomic(cnt, &total_sbmb_cnt);
	VERIFY(total_sbmb_cnt >= 0);
	if (total_sbmb_cnt < total_sbmb_cnt_floor) {
		total_sbmb_cnt_floor = total_sbmb_cnt;
	}

	  
                                                         
                              
    
	if (sb->sb_flags & SB_SNDBYTE_CNT) {
		inp_decr_sndbytes_total(sb->sb_so, m->m_len);
	}
}

  
                                                         
                                                              
                                                      
   
int
sblock(struct sockbuf *sb, uint32_t flags)
{
	boolean_t nointr = ((sb->sb_flags & SB_NOINTR) || (flags & SBL_NOINTR));
	void *lr_saved = __builtin_return_address(0);
	struct socket *so = sb->sb_so;
	void * wchan;
	int error = 0;
	thread_t tp = current_thread();

	VERIFY((flags & SBL_VALID) == flags);

	                                                              
	if (so == NULL) {
		panic("%s: null so, sb=%p sb_flags=0x%x lr=%p\n",
		    __func__, sb, sb->sb_flags, lr_saved);
		                
	} else if (so->so_usecount < 0) {
		panic("%s: sb=%p sb_flags=0x%x sb_so=%p usecount=%d lr=%p "
		    "lrh= %s\n", __func__, sb, sb->sb_flags, so,
		    so->so_usecount, lr_saved, solockhistory_nr(so));
		                
	}

	  
                                                        
    
	if ((so->so_flags & SOF_CONTENT_FILTER) && sb->sb_cfil_thread == tp) {
		  
                                                      
                                
     
		if (!(so->so_flags & SOF_DEFUNCT) && !(sb->sb_flags & SB_LOCK)) {
			panic("%s: SB_LOCK not held for %p\n",
			    __func__, sb);
		}

		                             
		return 0;
	}

	if ((sb->sb_flags & SB_LOCK) && !(flags & SBL_WAIT)) {
		return EWOULDBLOCK;
	}
	  
                                                               
                                                                  
                                    
    
	wchan = (sb->sb_flags & SB_RECV) ?
	    &so->so_rcv.sb_flags : &so->so_snd.sb_flags;

	  
                                                               
                       
    
	while ((sb->sb_flags & SB_LOCK) ||
	    ((so->so_flags & SOF_CONTENT_FILTER) &&
	    sb->sb_cfil_thread != NULL)) {
		lck_mtx_t *mutex_held;

		  
                                                                  
                                                             
                                                              
                                                
     
		if (so->so_proto->pr_getlock != NULL) {
			mutex_held = (*so->so_proto->pr_getlock)(so, PR_F_WILLUNLOCK);
		} else {
			mutex_held = so->so_proto->pr_domain->dom_mtx;
		}

		LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);

		sb->sb_wantlock++;
		VERIFY(sb->sb_wantlock != 0);

		error = msleep(wchan, mutex_held,
		    nointr ? PSOCK : PSOCK | PCATCH,
		    nointr ? "sb_lock_nointr" : "sb_lock", NULL);

		VERIFY(sb->sb_wantlock != 0);
		sb->sb_wantlock--;

		if (error == 0 && (so->so_flags & SOF_DEFUNCT) &&
		    !(flags & SBL_IGNDEFUNCT)) {
			error = EBADF;
			SODEFUNCTLOG("%s[%d, %s]: defunct so 0x%llx [%d,%d] "
			    "(%d)\n", __func__, proc_selfpid(),
			    proc_best_name(current_proc()),
			    (uint64_t)VM_KERNEL_ADDRPERM(so),
			    SOCK_DOM(so), SOCK_TYPE(so), error);
		}

		if (error != 0) {
			return error;
		}
	}
	sb->sb_flags |= SB_LOCK;
	return 0;
}

  
                             
   
void
sbunlock(struct sockbuf *sb, boolean_t keeplocked)
{
	void *lr_saved = __builtin_return_address(0);
	struct socket *so = sb->sb_so;
	thread_t tp = current_thread();

	                                                              
	if (so == NULL) {
		panic("%s: null so, sb=%p sb_flags=0x%x lr=%p\n",
		    __func__, sb, sb->sb_flags, lr_saved);
		                
	} else if (so->so_usecount < 0) {
		panic("%s: sb=%p sb_flags=0x%x sb_so=%p usecount=%d lr=%p "
		    "lrh= %s\n", __func__, sb, sb->sb_flags, so,
		    so->so_usecount, lr_saved, solockhistory_nr(so));
		                
	}

	  
                                                        
    
	if ((so->so_flags & SOF_CONTENT_FILTER) && sb->sb_cfil_thread == tp) {
		  
                                                      
                                
     
		if (!(so->so_flags & SOF_DEFUNCT) &&
		    !(sb->sb_flags & SB_LOCK) &&
		    !(so->so_state & SS_DEFUNCT) &&
		    !(so->so_flags1 & SOF1_DEFUNCTINPROG)) {
			panic("%s: SB_LOCK not held for %p\n",
			    __func__, sb);
		}
		                                         
	} else {
		VERIFY((sb->sb_flags & SB_LOCK) ||
		    (so->so_state & SS_DEFUNCT) ||
		    (so->so_flags1 & SOF1_DEFUNCTINPROG));

		sb->sb_flags &= ~SB_LOCK;

		if (sb->sb_wantlock > 0) {
			  
                                                         
                                                       
                                                           
      
			wakeup((sb->sb_flags & SB_RECV) ? &so->so_rcv.sb_flags :
			    &so->so_snd.sb_flags);
		}
	}

	if (!keeplocked) {                          
		if (so->so_flags & SOF_MP_SUBFLOW || SOCK_DOM(so) == PF_MULTIPATH) {
			(*so->so_proto->pr_unlock)(so, 1, lr_saved);
		} else {
			lck_mtx_t *mutex_held;

			if (so->so_proto->pr_getlock != NULL) {
				mutex_held = (*so->so_proto->pr_getlock)(so, PR_F_WILLUNLOCK);
			} else {
				mutex_held = so->so_proto->pr_domain->dom_mtx;
			}

			LCK_MTX_ASSERT(mutex_held, LCK_MTX_ASSERT_OWNED);

			VERIFY(so->so_usecount > 0);
			so->so_usecount--;
			so->unlock_lr[so->next_unlock_lr] = lr_saved;
			so->next_unlock_lr = (so->next_unlock_lr + 1) % SO_LCKDBG_MAX;
			lck_mtx_unlock(mutex_held);
		}
	}
}

void
sorwakeup(struct socket *so)
{
	if (sb_notify(&so->so_rcv)) {
		sowakeup(so, &so->so_rcv, NULL);
	}
}

void
sowwakeup(struct socket *so)
{
	if (sb_notify(&so->so_snd)) {
		sowakeup(so, &so->so_snd, NULL);
	}
}

void
soevent(struct socket *so, long hint)
{
	if (so->so_flags & SOF_KNOTE) {
		KNOTE(&so->so_klist, hint);
	}

	soevupcall(so, hint);

	  
                                                   
                                                     
    
	if ((hint & SO_FILT_HINT_IFDENIED) &&
	    !(so->so_flags & SOF_MP_SUBFLOW) &&
	    !(so->so_restrictions & SO_RESTRICT_DENY_CELLULAR) &&
	    !(so->so_restrictions & SO_RESTRICT_DENY_EXPENSIVE) &&
	    !(so->so_restrictions & SO_RESTRICT_DENY_CONSTRAINED)) {
		soevent_ifdenied(so);
	}
}

void
soevupcall(struct socket *so, long hint)
{
	if (so->so_event != NULL) {
		caddr_t so_eventarg = so->so_eventarg;

		hint &= so->so_eventmask;
		if (hint != 0) {
			so->so_event(so, so_eventarg, hint);
		}
	}
}

static void
soevent_ifdenied(struct socket *so)
{
	struct kev_netpolicy_ifdenied ev_ifdenied;

	bzero(&ev_ifdenied, sizeof(ev_ifdenied));
	  
                                                                        
                                                                     
                                                                      
                        
    
	if (so->so_flags & SOF_DELEGATED) {
		ev_ifdenied.ev_data.eupid = so->e_upid;
		ev_ifdenied.ev_data.epid = so->e_pid;
		uuid_copy(ev_ifdenied.ev_data.euuid, so->e_uuid);
	} else {
		ev_ifdenied.ev_data.eupid = so->last_upid;
		ev_ifdenied.ev_data.epid = so->last_pid;
		uuid_copy(ev_ifdenied.ev_data.euuid, so->last_uuid);
	}

	if (++so->so_ifdenied_notifies > 1) {
		  
                                                           
                                                          
                                                         
     
		if (net_io_policy_log) {
			uuid_string_t buf;

			uuid_unparse(ev_ifdenied.ev_data.euuid, buf);
			log(LOG_DEBUG, "%s[%d]: so 0x%llx [%d,%d] epid %llu "
			    "euuid %s%s has %d redundant events supressed\n",
			    __func__, so->last_pid,
			    (uint64_t)VM_KERNEL_ADDRPERM(so), SOCK_DOM(so),
			    SOCK_TYPE(so), ev_ifdenied.ev_data.epid, buf,
			    ((so->so_flags & SOF_DELEGATED) ?
			    " [delegated]" : ""), so->so_ifdenied_notifies);
		}
	} else {
		if (net_io_policy_log) {
			uuid_string_t buf;

			uuid_unparse(ev_ifdenied.ev_data.euuid, buf);
			log(LOG_DEBUG, "%s[%d]: so 0x%llx [%d,%d] epid %llu "
			    "euuid %s%s event posted\n", __func__,
			    so->last_pid, (uint64_t)VM_KERNEL_ADDRPERM(so),
			    SOCK_DOM(so), SOCK_TYPE(so),
			    ev_ifdenied.ev_data.epid, buf,
			    ((so->so_flags & SOF_DELEGATED) ?
			    " [delegated]" : ""));
		}
		netpolicy_post_msg(KEV_NETPOLICY_IFDENIED, &ev_ifdenied.ev_data,
		    sizeof(ev_ifdenied));
	}
}

  
                                                                   
   
struct sockaddr *
dup_sockaddr(struct sockaddr *sa, int canwait)
{
	struct sockaddr *sa2;

	MALLOC(sa2, struct sockaddr *, sa->sa_len, M_SONAME,
	    canwait ? M_WAITOK : M_NOWAIT);
	if (sa2) {
		bcopy(sa, sa2, sa->sa_len);
	}
	return sa2;
}

  
                                                                          
                                                                        
                                                                    
                                                                 
                                                              
                                                        
   
void
sotoxsocket(struct socket *so, struct xsocket *xso)
{
	xso->xso_len = sizeof(*xso);
	xso->xso_so = (_XSOCKET_PTR(struct socket *))VM_KERNEL_ADDRPERM(so);
	xso->so_type = so->so_type;
	xso->so_options = (short)(so->so_options & 0xffff);
	xso->so_linger = so->so_linger;
	xso->so_state = so->so_state;
	xso->so_pcb = (_XSOCKET_PTR(caddr_t))VM_KERNEL_ADDRPERM(so->so_pcb);
	if (so->so_proto) {
		xso->xso_protocol = SOCK_PROTO(so);
		xso->xso_family = SOCK_DOM(so);
	} else {
		xso->xso_protocol = xso->xso_family = 0;
	}
	xso->so_qlen = so->so_qlen;
	xso->so_incqlen = so->so_incqlen;
	xso->so_qlimit = so->so_qlimit;
	xso->so_timeo = so->so_timeo;
	xso->so_error = so->so_error;
	xso->so_pgid = so->so_pgid;
	xso->so_oobmark = so->so_oobmark;
	sbtoxsockbuf(&so->so_snd, &xso->so_snd);
	sbtoxsockbuf(&so->so_rcv, &xso->so_rcv);
	xso->so_uid = kauth_cred_getuid(so->so_cred);
}


#if XNU_TARGET_OS_OSX

void
sotoxsocket64(struct socket *so, struct xsocket64 *xso)
{
	xso->xso_len = sizeof(*xso);
	xso->xso_so = (u_int64_t)VM_KERNEL_ADDRPERM(so);
	xso->so_type = so->so_type;
	xso->so_options = (short)(so->so_options & 0xffff);
	xso->so_linger = so->so_linger;
	xso->so_state = so->so_state;
	xso->so_pcb = (u_int64_t)VM_KERNEL_ADDRPERM(so->so_pcb);
	if (so->so_proto) {
		xso->xso_protocol = SOCK_PROTO(so);
		xso->xso_family = SOCK_DOM(so);
	} else {
		xso->xso_protocol = xso->xso_family = 0;
	}
	xso->so_qlen = so->so_qlen;
	xso->so_incqlen = so->so_incqlen;
	xso->so_qlimit = so->so_qlimit;
	xso->so_timeo = so->so_timeo;
	xso->so_error = so->so_error;
	xso->so_pgid = so->so_pgid;
	xso->so_oobmark = so->so_oobmark;
	sbtoxsockbuf(&so->so_snd, &xso->so_snd);
	sbtoxsockbuf(&so->so_rcv, &xso->so_rcv);
	xso->so_uid = kauth_cred_getuid(so->so_cred);
}

#endif                        

  
                                                                      
                                                                   
                                                                 
                                 
   
void
sbtoxsockbuf(struct sockbuf *sb, struct xsockbuf *xsb)
{
	xsb->sb_cc = sb->sb_cc;
	xsb->sb_hiwat = sb->sb_hiwat;
	xsb->sb_mbcnt = sb->sb_mbcnt;
	xsb->sb_mbmax = sb->sb_mbmax;
	xsb->sb_lowat = sb->sb_lowat;
	xsb->sb_flags = (short)sb->sb_flags;
	xsb->sb_timeo = (short)
	    ((sb->sb_timeo.tv_sec * hz) + sb->sb_timeo.tv_usec / tick);
	if (xsb->sb_timeo == 0 && sb->sb_timeo.tv_usec != 0) {
		xsb->sb_timeo = 1;
	}
}

  
                                                                            
                                                                     
   
inline int
soisthrottled(struct socket *so)
{
	return so->so_flags1 & SOF1_TRAFFIC_MGT_SO_BACKGROUND;
}

inline int
soisprivilegedtraffic(struct socket *so)
{
	return (so->so_flags & SOF_PRIVILEGED_TRAFFIC_CLASS) ? 1 : 0;
}

inline int
soissrcbackground(struct socket *so)
{
	return (so->so_flags1 & SOF1_TRAFFIC_MGT_SO_BACKGROUND) ||
	       IS_SO_TC_BACKGROUND(so->so_traffic_class);
}

inline int
soissrcrealtime(struct socket *so)
{
	return so->so_traffic_class >= SO_TC_AV &&
	       so->so_traffic_class <= SO_TC_VO;
}

inline int
soissrcbesteffort(struct socket *so)
{
	return so->so_traffic_class == SO_TC_BE ||
	       so->so_traffic_class == SO_TC_RD ||
	       so->so_traffic_class == SO_TC_OAM;
}

void
soclearfastopen(struct socket *so)
{
	if (so->so_flags1 & SOF1_PRECONNECT_DATA) {
		so->so_flags1 &= ~SOF1_PRECONNECT_DATA;
	}

	if (so->so_flags1 & SOF1_DATA_IDEMPOTENT) {
		so->so_flags1 &= ~SOF1_DATA_IDEMPOTENT;
	}
}

void
sonullevent(struct socket *so, void *arg, long hint)
{
#pragma unused(so, arg, hint)
}

  
                                                                      
                     
   
SYSCTL_NODE(_kern, KERN_IPC, ipc,
    CTLFLAG_RW | CTLFLAG_LOCKED | CTLFLAG_ANYBODY, 0, "IPC");

                                                                 

static int
sysctl_sb_max SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	u_int32_t new_value;
	int changed = 0;
	int error = sysctl_io_number(req, sb_max, sizeof(u_int32_t),
	    &new_value, &changed);
	if (!error && changed) {
		if (new_value > LOW_SB_MAX && new_value <= high_sb_max) {
			sb_max = new_value;
		} else {
			error = ERANGE;
		}
	}
	return error;
}

SYSCTL_PROC(_kern_ipc, KIPC_MAXSOCKBUF, maxsockbuf,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &sb_max, 0, &sysctl_sb_max, "IU", "Maximum socket buffer size");

SYSCTL_INT(_kern_ipc, KIPC_SOCKBUF_WASTE, sockbuf_waste_factor,
    CTLFLAG_RW | CTLFLAG_LOCKED, &sb_efficiency, 0, "");

SYSCTL_INT(_kern_ipc, KIPC_NMBCLUSTERS, nmbclusters,
    CTLFLAG_RD | CTLFLAG_LOCKED, &nmbclusters, 0, "");

SYSCTL_INT(_kern_ipc, OID_AUTO, njcl,
    CTLFLAG_RD | CTLFLAG_LOCKED, &njcl, 0, "");

SYSCTL_INT(_kern_ipc, OID_AUTO, njclbytes,
    CTLFLAG_RD | CTLFLAG_LOCKED, &njclbytes, 0, "");

SYSCTL_INT(_kern_ipc, KIPC_SOQLIMITCOMPAT, soqlimitcompat,
    CTLFLAG_RW | CTLFLAG_LOCKED, &soqlimitcompat, 1,
    "Enable socket queue limit compatibility");

  
                                
                                                               
                                                      
                                                              
                                                            
                                                            
                                                                     
                                                            
   

extern u_int32_t tcp_do_autorcvbuf;

static int
sysctl_soqlencomp SYSCTL_HANDLER_ARGS
{
#pragma unused(oidp, arg1, arg2)
	u_int32_t new_value;
	int changed = 0;
	int error = sysctl_io_number(req, soqlencomp, sizeof(u_int32_t),
	    &new_value, &changed);
	if (!error && changed) {
		soqlencomp = new_value;
		if (new_value != 0) {
			tcp_do_autorcvbuf = 0;
			tcptv_persmin_val = 6 * TCP_RETRANSHZ;
		}
	}
	return error;
}
SYSCTL_PROC(_kern_ipc, OID_AUTO, soqlencomp,
    CTLTYPE_INT | CTLFLAG_RW | CTLFLAG_LOCKED,
    &soqlencomp, 0, &sysctl_soqlencomp, "IU", "");

SYSCTL_INT(_kern_ipc, OID_AUTO, sbmb_cnt, CTLFLAG_RD | CTLFLAG_LOCKED,
    &total_sbmb_cnt, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, sbmb_cnt_peak, CTLFLAG_RD | CTLFLAG_LOCKED,
    &total_sbmb_cnt_peak, 0, "");
SYSCTL_INT(_kern_ipc, OID_AUTO, sbmb_cnt_floor, CTLFLAG_RD | CTLFLAG_LOCKED,
    &total_sbmb_cnt_floor, 0, "");
SYSCTL_QUAD(_kern_ipc, OID_AUTO, sbmb_limreached, CTLFLAG_RD | CTLFLAG_LOCKED,
    &sbmb_limreached, "");


SYSCTL_NODE(_kern_ipc, OID_AUTO, io_policy, CTLFLAG_RW, 0, "network IO policy");

SYSCTL_INT(_kern_ipc_io_policy, OID_AUTO, log, CTLFLAG_RW | CTLFLAG_LOCKED,
    &net_io_policy_log, 0, "");

#if CONFIG_PROC_UUID_POLICY
SYSCTL_INT(_kern_ipc_io_policy, OID_AUTO, uuid, CTLFLAG_RW | CTLFLAG_LOCKED,
    &net_io_policy_uuid, 0, "");
#endif                              
