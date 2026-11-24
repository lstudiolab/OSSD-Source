/*
 * Copyright (c) 2019 Apple Inc. All rights reserved.
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

                                                                 

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/mbuf.h>
#include <sys/mcache.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/errno.h>

#include <kern/locks.h>

#include <net/raw_cb.h>
#include <net/route.h>

#include <net/pfkeyv2.h>
#include <netkey/keydb.h>
#include <netkey/key.h>
#include <netkey/keysock.h>
#include <netkey/key_debug.h>

extern lck_mtx_t *raw_mtx;
extern void key_init(struct protosw *, struct domain *);

struct sockaddr key_dst = { .sa_len = 2, .sa_family = PF_KEY, .sa_data = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, } };
struct sockaddr key_src = { .sa_len = 2, .sa_family = PF_KEY, .sa_data = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, } };

static void key_dinit(struct domain *);
static int key_sendup0(struct rawcb *, struct mbuf *, int);

struct pfkeystat pfkeystat;

static struct domain *keydomain = NULL;

extern lck_mtx_t *pfkey_stat_mutex;

  
               
   
int
#ifdef __APPLE__
                                   
key_output(struct mbuf *m, struct socket *so)
#else
#if __STDC__
key_output(struct mbuf *m, ...)
#else
key_output(m, va_alist)
struct mbuf *m;
va_dcl
#endif
#endif
{
	struct sadb_msg *msg;
	int len, error = 0;
#ifndef __APPLE__
	struct socket *so;
	va_list ap;

	va_start(ap, m);
	so = va_arg(ap, struct socket *);
	va_end(ap);
#endif

	if (m == 0) {
		panic("key_output: NULL pointer was passed.\n");
	}

	socket_unlock(so, 0);
	lck_mtx_lock(pfkey_stat_mutex);
	pfkeystat.out_total++;
	pfkeystat.out_bytes += m->m_pkthdr.len;
	lck_mtx_unlock(pfkey_stat_mutex);

	len = m->m_pkthdr.len;
	if (len < sizeof(struct sadb_msg)) {
#if IPSEC_DEBUG
		printf("key_output: Invalid message length.\n");
#endif
		PFKEY_STAT_INCREMENT(pfkeystat.out_tooshort);
		error = EINVAL;
		goto end;
	}

	if (m->m_len < sizeof(struct sadb_msg)) {
		if ((m = m_pullup(m, sizeof(struct sadb_msg))) == 0) {
#if IPSEC_DEBUG
			printf("key_output: can't pullup mbuf\n");
#endif
			PFKEY_STAT_INCREMENT(pfkeystat.out_nomem);
			error = ENOBUFS;
			goto end;
		}
	}

	if ((m->m_flags & M_PKTHDR) == 0) {
		panic("key_output: not M_PKTHDR ??");
	}

#if IPSEC_DEBUG
	KEYDEBUG(KEYDEBUG_KEY_DUMP, kdebug_mbuf(m));
#endif                           

	msg = mtod(m, struct sadb_msg *);
	PFKEY_STAT_INCREMENT(pfkeystat.out_msgtype[msg->sadb_msg_type]);
	if (len != PFKEY_UNUNIT64(msg->sadb_msg_len)) {
#if IPSEC_DEBUG
		printf("key_output: Invalid message length.\n");
#endif
		PFKEY_STAT_INCREMENT(pfkeystat.out_invlen);
		error = EINVAL;
		goto end;
	}

	error = key_parse(m, so);
	m = NULL;

end:
	if (m) {
		m_freem(m);
	}
	socket_lock(so, 0);
	return error;
}

  
                              
   
static int
key_sendup0(struct rawcb *rp, struct mbuf *m, int promisc)
{
	int error;

	if (promisc) {
		struct sadb_msg *pmsg;

		M_PREPEND(m, sizeof(struct sadb_msg), M_NOWAIT, 1);
		if (m && m->m_len < sizeof(struct sadb_msg)) {
			m = m_pullup(m, sizeof(struct sadb_msg));
		}
		if (!m) {
#if IPSEC_DEBUG
			printf("key_sendup0: cannot pullup\n");
#endif
			PFKEY_STAT_INCREMENT(pfkeystat.in_nomem);
			m_freem(m);
			return ENOBUFS;
		}
		m->m_pkthdr.len += sizeof(*pmsg);
		VERIFY(PFKEY_UNIT64(m->m_pkthdr.len) <= UINT16_MAX);

		pmsg = mtod(m, struct sadb_msg *);
		bzero(pmsg, sizeof(*pmsg));
		pmsg->sadb_msg_version = PF_KEY_V2;
		pmsg->sadb_msg_type = SADB_X_PROMISC;
		pmsg->sadb_msg_len = (u_int16_t)PFKEY_UNIT64(m->m_pkthdr.len);
		                  

		PFKEY_STAT_INCREMENT(pfkeystat.in_msgtype[pmsg->sadb_msg_type]);
	}

	if (!sbappendaddr(&rp->rcb_socket->so_rcv, (struct sockaddr *)&key_src,
	    m, NULL, &error)) {
#if IPSEC_DEBUG
		printf("key_sendup0: sbappendaddr failed\n");
#endif
		PFKEY_STAT_INCREMENT(pfkeystat.in_nomem);
	} else {
		sorwakeup(rp->rcb_socket);
	}
	return error;
}


                                                
int
key_sendup_mbuf(struct socket *so, struct mbuf *m, int target)
{
	struct mbuf *n;
	struct keycb *kp;
	int sendup;
	struct rawcb *rp;
	int error = 0;

	if (m == NULL) {
		panic("key_sendup_mbuf: NULL pointer was passed.\n");
	}
	if (so == NULL && target == KEY_SENDUP_ONE) {
		panic("key_sendup_mbuf: NULL pointer was passed.\n");
	}

	lck_mtx_lock(pfkey_stat_mutex);
	pfkeystat.in_total++;
	pfkeystat.in_bytes += m->m_pkthdr.len;
	lck_mtx_unlock(pfkey_stat_mutex);
	if (m->m_len < sizeof(struct sadb_msg)) {
#if 1
		m = m_pullup(m, sizeof(struct sadb_msg));
		if (m == NULL) {
			PFKEY_STAT_INCREMENT(pfkeystat.in_nomem);
			return ENOBUFS;
		}
#else
		                                               
#endif
	}
	if (m->m_len >= sizeof(struct sadb_msg)) {
		struct sadb_msg *msg;
		msg = mtod(m, struct sadb_msg *);
		PFKEY_STAT_INCREMENT(pfkeystat.in_msgtype[msg->sadb_msg_type]);
	}

	lck_mtx_lock(raw_mtx);
	LIST_FOREACH(rp, &rawcb_list, list)
	{
		if (rp->rcb_proto.sp_family != PF_KEY) {
			continue;
		}
		if (rp->rcb_proto.sp_protocol
		    && rp->rcb_proto.sp_protocol != PF_KEY_V2) {
			continue;
		}

		kp = (struct keycb *)rp;

		socket_lock(rp->rcb_socket, 1);
		  
                                                                 
                                           
                                                       
     
		if (((struct keycb *)rp)->kp_promisc) {
			if ((n = m_copy(m, 0, (int)M_COPYALL)) != NULL) {
				(void)key_sendup0(rp, n, 1);
				n = NULL;
			}
		}

		                                              
		if (so && sotorawcb(so) == rp) {
			socket_unlock(rp->rcb_socket, 1);
			continue;
		}

		sendup = 0;
		switch (target) {
		case KEY_SENDUP_ONE:
			                                 
			break;
		case KEY_SENDUP_ALL:
			sendup++;
			break;
		case KEY_SENDUP_REGISTERED:
			if (kp->kp_registered) {
				sendup++;
			}
			break;
		}
		PFKEY_STAT_INCREMENT(pfkeystat.in_msgtarget[target]);

		if (!sendup) {
			socket_unlock(rp->rcb_socket, 1);
			continue;
		} else {
			sendup = 0;                             
		}
		if ((n = m_copy(m, 0, (int)M_COPYALL)) == NULL) {
#if IPSEC_DEBUG
			printf("key_sendup: m_copy fail\n");
#endif
			m_freem(m);
			PFKEY_STAT_INCREMENT(pfkeystat.in_nomem);
			socket_unlock(rp->rcb_socket, 1);
			lck_mtx_unlock(raw_mtx);
			return ENOBUFS;
		}

		  
                                                         
                                           
                                                     
     
		key_sendup0(rp, n, 0);
		socket_unlock(rp->rcb_socket, 1);
		n = NULL;
	}

	lck_mtx_unlock(raw_mtx);
	if (so) {
		socket_lock(so, 1);
		error = key_sendup0(sotorawcb(so), m, 0);
		socket_unlock(so, 1);
		m = NULL;
	} else {
		error = 0;
		m_freem(m);
	}
	return error;
}

  
              
                                        
   
static int
key_abort(struct socket *so)
{
	int error;
	error = raw_usrreqs.pru_abort(so);
	return error;
}

  
               
                                         
   
static int
key_attach(struct socket *so, int proto, struct proc *p)
{
	struct keycb *kp;
	int error;

	if (sotorawcb(so) != 0) {
		return EISCONN;                 
	}
	kp = (struct keycb *)_MALLOC(sizeof(*kp), M_PCB,
	    M_WAITOK | M_ZERO);          
	if (kp == 0) {
		return ENOBUFS;
	}

	so->so_pcb = (caddr_t)kp;
	kp->kp_promisc = kp->kp_registered = 0;
	kp->kp_raw.rcb_laddr = &key_src;
	kp->kp_raw.rcb_faddr = &key_dst;

	error = raw_usrreqs.pru_attach(so, proto, p);
	kp = (struct keycb *)sotorawcb(so);
	if (error) {
		_FREE(kp, M_PCB);
		so->so_pcb = (caddr_t) 0;
		so->so_flags |= SOF_PCBCLEARING;
		printf("key_usrreq: key_usrreq results %d\n", error);
		return error;
	}

	                                                  
	if (kp->kp_raw.rcb_proto.sp_protocol == PF_KEY) {                  
		key_cb.key_count++;
	}
	key_cb.any_count++;
	soisconnected(so);
	so->so_options |= SO_USELOOPBACK;

	return 0;
}

  
             
                                       
   
static int
key_bind(struct socket *so, struct sockaddr *nam, struct proc *p)
{
	int error;
	error = raw_usrreqs.pru_bind(so, nam, p);                      
	return error;
}

  
                
                                          
   
static int
key_connect(struct socket *so, struct sockaddr *nam, struct proc *p)
{
	int error;
	error = raw_usrreqs.pru_connect(so, nam, p);                      
	return error;
}

  
               
                                         
   
static int
key_detach(struct socket *so)
{
	struct keycb *kp = (struct keycb *)sotorawcb(so);
	int error;

	if (kp != 0) {
		if (kp->kp_raw.rcb_proto.sp_protocol == PF_KEY) {                  
			key_cb.key_count--;
		}
		key_cb.any_count--;
		socket_unlock(so, 0);
		key_freereg(so);
		socket_lock(so, 0);
	}
	error = raw_usrreqs.pru_detach(so);
	return error;
}

  
                   
                                             
   
static int
key_disconnect(struct socket *so)
{
	int error;
	error = raw_usrreqs.pru_disconnect(so);
	return error;
}

  
                 
                                           
   
static int
key_peeraddr(struct socket *so, struct sockaddr **nam)
{
	int error;
	error = raw_usrreqs.pru_peeraddr(so, nam);
	return error;
}

  
             
                                       
   
static int
key_send(struct socket *so, int flags, struct mbuf *m, struct sockaddr *nam,
    struct mbuf *control, struct proc *p)
{
	int error;
	error = raw_usrreqs.pru_send(so, flags, m, nam, control, p);
	return error;
}

  
                 
                                           
   
static int
key_shutdown(struct socket *so)
{
	int error;
	error = raw_usrreqs.pru_shutdown(so);
	return error;
}

  
                 
                                           
   
static int
key_sockaddr(struct socket *so, struct sockaddr **nam)
{
	int error;
	error = raw_usrreqs.pru_sockaddr(so, nam);
	return error;
}

static struct pr_usrreqs key_usrreqs = {
	.pru_abort =            key_abort,
	.pru_attach =           key_attach,
	.pru_bind =             key_bind,
	.pru_connect =          key_connect,
	.pru_detach =           key_detach,
	.pru_disconnect =       key_disconnect,
	.pru_peeraddr =         key_peeraddr,
	.pru_send =             key_send,
	.pru_shutdown =         key_shutdown,
	.pru_sockaddr =         key_sockaddr,
	.pru_sosend =           sosend,
	.pru_soreceive =        soreceive,
};

            
SYSCTL_NODE(_net, PF_KEY, key, CTLFLAG_RW | CTLFLAG_LOCKED, 0, "Key Family");

  
                                                        
   

extern struct domain keydomain_s;

static struct protosw keysw[] = {
	{
		.pr_type =              SOCK_RAW,
		.pr_protocol =          PF_KEY_V2,
		.pr_flags =             PR_ATOMIC | PR_ADDR,
		.pr_output =            key_output,
		.pr_ctlinput =          raw_ctlinput,
		.pr_init =              key_init,
		.pr_usrreqs =           &key_usrreqs,
	}
};

static int key_proto_count = (sizeof(keysw) / sizeof(struct protosw));

struct domain keydomain_s = {
	.dom_family =           PF_KEY,
	.dom_name =             "key",
	.dom_init =             key_dinit,
	.dom_maxrtkey =         sizeof(struct key_cb),
};

static void
key_dinit(struct domain *dp)
{
	struct protosw *pr;
	int i;

	VERIFY(!(dp->dom_flags & DOM_INITIALIZED));
	VERIFY(keydomain == NULL);

	keydomain = dp;

	for (i = 0, pr = &keysw[0]; i < key_proto_count; i++, pr++) {
		net_add_proto(pr, dp, 1);
	}
}
